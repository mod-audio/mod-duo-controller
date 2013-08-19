
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "cli.h"
#include "config.h"
#include "comm.h"

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
#define PROMPT_TEXT         "[root@mod"

#define MOD_LOGIN           "root" NEW_LINE
#define MOD_PASSWORD        "mod" NEW_LINE
#define REBOOT_CMD          "reboot" NEW_LINE


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

static char g_line_buffer[CLI_LINE_BUFFER_SIZE];
static uint32_t g_line_idx;
static uint8_t g_stage, g_new_data;
static uint8_t g_boot_aborted = 0, g_grub_entry = REGULAR_ENTRY;


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

void clear_buffer(void)
{
    g_line_buffer[0] = 0;
    g_line_idx = 0;
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

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
                clear_buffer();
                g_stage = PROMPT_STAGE;
            }
            break;

        case PROMPT_STAGE:
            // reboot the cpu to finish the restore
            if (g_grub_entry == RESTORE_ENTRY) cli_reboot_cpu();

            // reset to regular grub entry
            g_grub_entry = REGULAR_ENTRY;
            break;
    }

    // if new line clear the buffer
    pstr = strstr(g_line_buffer, NEW_LINE);
    if (pstr) clear_buffer();

    g_new_data = 0;
}

void cli_grub_select(uint8_t entry)
{
    if (g_boot_aborted)
    {
        comm_linux_send(g_grub_entries[entry]);
        g_boot_aborted = 0;
        g_stage = LOGIN_STAGE;
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
