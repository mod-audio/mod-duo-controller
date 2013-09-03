
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "cli.h"
#include "config.h"
#include "comm.h"
#include "utils.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define NEW_LINE            "\r\n"
#define ESCAPE              "\x1B"
#define ARROW_DOWN_VT100    ESCAPE "[B"

#define GRUB_TEXT           "GNU GRUB"
#define LOGIN_TEXT          "mod login:"
#define PASSWORD_TEXT       "Password:"
#define PROMPT_TEXT         "mod:"

#define MOD_LOGIN           "root" NEW_LINE
#define MOD_PASSWORD        "mod" NEW_LINE
#define ECHO_OFF_CMD        "stty -echo" NEW_LINE
#define PS1_SETUP_CMD       "PS1=" PROMPT_TEXT NEW_LINE
#define REBOOT_CMD          "reboot" NEW_LINE
#define JACK_BUSIZE_CMD     "jack_bufsize "
#define SYSTEMCTL_CMD       "systemctl "
#define PACMAN_Q_CMD        "pacman -Q "


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

// defines the stages of CLI
enum {GRUB_STAGE, BOOT_STAGE, LOGIN_STAGE, PASSWORD_STAGE, PROMPT_STAGE};

// grub entries
static const char *g_grub_entries[] = {
    NEW_LINE,
    ARROW_DOWN_VT100 NEW_LINE,
    ARROW_DOWN_VT100 ARROW_DOWN_VT100 NEW_LINE,
    ARROW_DOWN_VT100 ARROW_DOWN_VT100 ARROW_DOWN_VT100 NEW_LINE
};


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static char g_line_buffer[CLI_LINE_BUFFER_SIZE], g_response[CLI_RESPONSE_BUFFER_SIZE];
static uint32_t g_line_idx;
static uint8_t g_stage, g_new_data, g_waiting_response;
static uint8_t g_boot_aborted = 0, g_grub_entry = REGULAR_ENTRY;
static xSemaphoreHandle g_response_sem;


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL FUNCTIONS
************************************************************************************************************************
*/

static void clear_buffer(void)
{
    uint16_t i;

    // makes the buffer useless
    for (i = 0; i < g_line_idx; i += 3)
    {
        g_line_buffer[i] = 0;
    }

    g_line_idx = 0;
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void cli_init(void)
{
    vSemaphoreCreateBinary(g_response_sem);
}

void cli_append_data(const char *data, uint32_t data_size)
{
    if ((g_line_idx + data_size + 1) > CLI_LINE_BUFFER_SIZE)
    {
        clear_buffer();
        return;
    }

    memcpy(&g_line_buffer[g_line_idx], data, data_size);
    g_line_idx += data_size;
    g_line_buffer[g_line_idx] = 0;

    g_new_data = 1;
}

const char* cli_get_response(void)
{
    xSemaphoreTake(g_response_sem, (CLI_RESPONSE_TIMEOUT / portTICK_RATE_MS));
    return g_response;
}

void cli_process(void)
{
    char *pstr;

    if (!g_new_data) return;

    // TODO: need feedback on some stages

    switch (g_stage)
    {
        case GRUB_STAGE:
            pstr = strstr(g_line_buffer, GRUB_TEXT);
            if (pstr)
            {
                if (g_grub_entry == STOP_TIMEOUT)
                {
                    comm_linux_send(ESCAPE);
                    g_boot_aborted = 1;
                }
                else
                {
                    comm_linux_send(g_grub_entries[g_grub_entry]);
                    g_stage = LOGIN_STAGE;
                }

                clear_buffer();
            }
            break;

        case BOOT_STAGE:
            break;

        case LOGIN_STAGE:
            pstr = strstr(g_line_buffer, LOGIN_TEXT);
            if (pstr)
            {
                comm_linux_send(MOD_LOGIN);
                clear_buffer();
                g_stage = PASSWORD_STAGE;
            }
            break;

        case PASSWORD_STAGE:
            pstr = strstr(g_line_buffer, PASSWORD_TEXT);
            if (pstr)
            {
                comm_linux_send(MOD_PASSWORD);
                comm_linux_send(ECHO_OFF_CMD);
                comm_linux_send(PS1_SETUP_CMD);
                clear_buffer();
                g_stage = PROMPT_STAGE;
            }
            break;

        case PROMPT_STAGE:
            // reboot the cpu to finish the restore
            if (g_grub_entry == RESTORE_ENTRY) cli_reboot_cpu();

            // reset to regular grub entry
            g_grub_entry = REGULAR_ENTRY;

            // tries locate the prompt text
            char *pline = g_line_buffer;
            uint32_t resp_size = 0;
            do
            {
                pstr = strstr(pline, PROMPT_TEXT);
                if (pstr)
                {
                    resp_size = (pstr - pline);
                    if (resp_size > 0) break;
                    else pline += strlen(PROMPT_TEXT);
                }
            } while (pstr);

            // checks if found the response
            if (resp_size > 0)
            {
                // copies the console response
                strncpy(g_response, pline, resp_size);
                g_response[resp_size] = 0;

                // clear the buffer
                clear_buffer();

                // clears the flag
                g_waiting_response = 0;

                // unblock the task
                xSemaphoreGive(g_response_sem);
            }
            break;
    }

    if (!g_waiting_response)
    {
        // if new line clear the buffer
        pstr = strstr(g_line_buffer, NEW_LINE);
        if (pstr) clear_buffer();
    }

    g_new_data = 0;
}

void cli_grub_select(uint8_t entry)
{
    if (g_boot_aborted)
    {
        comm_linux_send(g_grub_entries[entry]);
        g_boot_aborted = 0;
        g_stage = LOGIN_STAGE;

        // if is restore entry jumps go direct to prompt stage
        if (entry == RESTORE_ENTRY) g_stage = PROMPT_STAGE;
    }
    else
    {
        g_grub_entry = entry;
        g_stage = GRUB_STAGE;
    }
}

void cli_reboot_cpu(void)
{
    comm_linux_send(REBOOT_CMD);
}

void cli_jack_set_bufsize(uint16_t bufsize)
{
    char buffer[32], bufsize_str[8];

    int_to_str(bufsize, bufsize_str, sizeof(bufsize_str), 0);
    strcpy(buffer, JACK_BUSIZE_CMD);
    strcat(buffer, bufsize_str);
    strcat(buffer, NEW_LINE);
    comm_linux_send(buffer);
}

void cli_systemctl(const char *parameters)
{
    char buffer[64];

    // copies the command
    strcpy(buffer, SYSTEMCTL_CMD);

    // copies the parameters
    if (parameters) strcat(buffer, parameters);
    strcat(buffer, NEW_LINE);

    // default response
    strcpy(g_response, "unknown");

    // sends the command
    g_waiting_response = 1;
    comm_linux_send(buffer);
}

void cli_package_version(const char *package_name)
{
    char buffer[64];

    if (!package_name) return;

    // copies the command
    strcpy(buffer, PACMAN_Q_CMD);
    strcat(buffer, package_name);
    strcat(buffer, " | cut -d' ' -f2");
    strcat(buffer, NEW_LINE);

    // default response
    strcpy(g_response, "unknown");

    // sends the command
    g_waiting_response = 1;
    comm_linux_send(buffer);
}
