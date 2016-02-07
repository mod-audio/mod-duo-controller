
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "system.h"
#include "config.h"
#include "data.h"
#include "naveg.h"
#include "hardware.h"
#include "actuator.h"
#include "comm.h"
#include "cli.h"
#include "screen.h"
#include "glcd_widget.h"
#include "glcd.h"

#include <string.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

// true bypass
#define TRUE_BYPASS_SET(ch,val) "amixer -D hw:MODDUO sset '" #ch " True-Bypass' " #val " > /dev/null"
#define TRUE_BYPASS_GET(ch)     "amixer -D hw:MODDUO sget '" #ch " True-Bypass' | grep -o \"\\[.*\\]\""
#define TRUE_BYPASS_TOGGLE(ch)  "amixer -D hw:MODDUO sset '" #ch " True-Bypass' toggle > /dev/null"


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

// systemctl services names
const char *systemctl_services[] = {
    "jack2",
    "mod-host",
    "mod-ui",
    "ttymidi",
    NULL
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

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/


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

static void update_status(char *item_to_update, const char *response)
{
    if (!item_to_update) return;

    char *pstr = strstr(item_to_update, ":");
    if (pstr && response)
    {
        pstr++;
        *pstr++ = ' ';
        strcpy(pstr, response);
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void system_true_bypass_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_reset_pedalboard_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_save_pedalboard_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_bluetooth_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_bluetooth_pair_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_services_cb(void *arg)
{
    menu_item_t *item = arg;

    uint8_t i = 0;
    while (systemctl_services[i])
    {
        const char *response;
        response = cli_systemctl("is-active ", systemctl_services[i]);
        update_status(item->data.list[i+1], response);
        screen_system_menu(item);
        i++;
    }
}

void system_restart_jack_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_restart_host_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_restart_ui_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_versions_cb(void *arg)
{
    UNUSED_PARAM(arg);
}

void system_restore_cb(void *arg)
{
    menu_item_t *item = arg;
    button_t *foot = (button_t *) hardware_actuators(FOOTSWITCH0);

    // check if YES option was chosen
    if (item->data.hover == 0)
    {
        uint8_t status = actuator_get_status(foot);

        // check if footswitch is pressed down
        if (BUTTON_PRESSED(status))
        {
            // remove all controls
            naveg_remove_control(ALL_EFFECTS, ALL_CONTROLS);

            // disable system menu
            naveg_toggle_tool(DISPLAY_TOOL_SYSTEM);

            // clear screens
            for (uint8_t i = 1; i < SLOTS_COUNT; i++)
                screen_clear(i);

            // start restore
            cli_restore();
        }
    }
}
