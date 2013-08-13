
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
#define ARROW_DOWN_VT100    "\x1B[B"

#define GRUB_SELECT_FIRST   ARROW_DOWN_VT100 NEW_LINE
#define GRUB_SELECT_SECOND  ARROW_DOWN_VT100 ARROW_DOWN_VT100 NEW_LINE

#define GRUB_TEXT           "GNU GRUB"
#define LOGIN_TEXT          "mod login:"
#define PASSWORD_TEXT       "Password:"
#define PROMPT_TEXT         "[root@mod"

#define MOD_LOGIN           "root" NEW_LINE
#define MOD_PASSWORD        "mod" NEW_LINE


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

// defines the stages of CLI
enum {GRUB_STAGE, BOOT_STAGE, LOGIN_STAGE, PASSWORD_STAGE, PROMPT_STAGE};


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
static uint8_t g_new_data;


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
    static uint8_t stage;
    char *pstr;

    if (!g_new_data) return;

    switch (stage)
    {
        case GRUB_STAGE:
            pstr = strstr(g_line_buffer, GRUB_TEXT);
            if (pstr)
            {
                comm_linux_send(GRUB_SELECT_FIRST);
                clear_buffer();
                stage = LOGIN_STAGE;
            }
            break;

        case BOOT_STAGE:
            // TODO: parsing the boot information and feedback the user
            break;

        case LOGIN_STAGE:
            pstr = strstr(g_line_buffer, LOGIN_TEXT);
            if (pstr)
            {
                comm_linux_send(MOD_LOGIN);
                clear_buffer();
                stage = PASSWORD_STAGE;
            }
            break;

        case PASSWORD_STAGE:
            pstr = strstr(g_line_buffer, PASSWORD_TEXT);
            if (pstr)
            {
                comm_linux_send(MOD_PASSWORD);
                clear_buffer();
                stage = PROMPT_STAGE;
            }
            break;

        case PROMPT_STAGE:
            break;
    }

    // if new line clear the buffer
    pstr = strstr(g_line_buffer, NEW_LINE);
    if (pstr) clear_buffer();

    g_new_data = 0;
}
