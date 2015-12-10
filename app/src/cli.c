
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "cli.h"
#include "config.h"
#include "comm.h"
#include "utils.h"
#include "hardware.h"
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

#define LOGIN_TEXT          "mod login:"
#define PASSWORD_TEXT       "Password:"
#define PROMPT_TEXT         "mod:"

#define MOD_LOGIN           "root" NEW_LINE
#define MOD_PASSWORD        "mod" NEW_LINE
#define ECHO_OFF_CMD        "stty -echo" NEW_LINE
#define PS1_SETUP_CMD       "PS1=" PROMPT_TEXT NEW_LINE
#define SYSTEMCTL_CMD       "systemctl "


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/


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
static uint8_t g_new_data, g_waiting_response;
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
    comm_linux_send(NEW_LINE);
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

    uint32_t len = strlen(g_response);

    // check if is new line
    if (g_response[len-2] == '\r' || g_response[len-2] == '\n')
        g_response[len-2] = 0;

    if (g_response[len-1] == '\r' || g_response[len-1] == '\n')
        g_response[len-1] = 0;

    return g_response;
}

void cli_process(void)
{
    char *pstr;

    if (!g_new_data) return;

    // TODO: parsing serial information

    if (!g_waiting_response)
    {
        // if new line clear buffer
        pstr = strstr(g_line_buffer, NEW_LINE);
        if (pstr) clear_buffer();
    }

    g_new_data = 0;
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
    if (!package_name) return;

    // TODO
}

void cli_bluetooth(uint8_t what_info)
{
    // default response
    strcpy(g_response, "unknown");

    switch (what_info)
    {
        case BLUETOOTH_NAME:
            break;

        case BLUETOOTH_ADDRESS:
            break;
    }
}
