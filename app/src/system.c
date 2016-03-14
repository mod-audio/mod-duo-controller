
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
#include "utils.h"

#include <string.h>
#include <stdlib.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/


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

const char *version_files[] = {
    "rootfs",
    "controller",
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

static void volume(menu_item_t *item, int event, const char *source, float min, float max, float step)
{
    cli_command("mod-amixer ", CLI_CACHE_ONLY);
    cli_command(source, CLI_CACHE_ONLY);
    cli_command(" vol ", CLI_CACHE_ONLY);

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        response = cli_command(NULL, CLI_RETRIEVE_RESPONSE);

        item->data.min = min;
        item->data.max = max;
        item->data.step = step;
        item->data.value = atof(response);
    }
    else if (event == MENU_EV_UP || event == MENU_EV_DOWN)
    {
        char value[8];
        float_to_str(item->data.value, value, sizeof value, 1);
        cli_command(value, CLI_DISCARD_RESPONSE);
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void system_true_bypass_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_reset_pedalboard_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_save_pedalboard_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_bluetooth_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_bluetooth_pair_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_services_cb(void *arg, int event)
{
    UNUSED_PARAM(event);

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

void system_restart_jack_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_restart_host_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_restart_ui_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);
    UNUSED_PARAM(event);
}

void system_versions_cb(void *arg, int event)
{
    UNUSED_PARAM(event);

    menu_item_t *item = arg;

    uint8_t i = 0;
    while (version_files[i])
    {
        const char *response;
        cli_command("mod-version ", CLI_CACHE_ONLY);
        response = cli_command(version_files[i], CLI_RETRIEVE_RESPONSE);
        update_status(item->data.list[i+1], response);
        screen_system_menu(item);
        i++;
    }
}

void system_upgrade_cb(void *arg, int event)
{
    UNUSED_PARAM(event);

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
            for (uint8_t i = 0; i < SLOTS_COUNT; i++)
                screen_clear(i);

            // start restore
            cli_restore(RESTORE_INIT);
        }
    }
}

void system_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;
    float min, max, step;
    const char *source;

    switch (item->desc->id)
    {
        case IN1_VOLUME:
            source = "in 1";
            min = -12.0;
            max = 12.0;
            step = 1.0;
            break;

        case IN2_VOLUME:
            source = "in 2";
            min = -12.0;
            max = 12.0;
            step = 1.0;
            break;

        case OUT1_VOLUME:
            source = "out 1";
            min = -127.0;
            max = 0.0;
            step = 0.5;
            break;

        case OUT2_VOLUME:
            source = "out 2";
            min = -127.0;
            max = 0.0;
            step = 0.5;
            break;

        case HP_VOLUME:
            source = "hp";
            min = -33.0;
            max = 12.0;
            step = 3.0;
            break;
    }

    volume(item, event, source, min, max, step);
}

void system_stage_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        char input[4];
        uint8_t n_input = ((IN1_STAGE_ID - item->desc->parent_id) == 0 ? 1 : 2);
        int_to_str(n_input, input, sizeof input, 0);

        cli_command("mod-amixer in ", CLI_CACHE_ONLY);
        cli_command(input, CLI_CACHE_ONLY);
        cli_command(" stg ", CLI_CACHE_ONLY);
        cli_command(item->desc->name, CLI_DISCARD_RESPONSE);
    }
}

void system_hp_bypass(void *arg, int event)
{
    menu_item_t *item = arg;

    static uint8_t bypass_state;

    if (event == MENU_EV_ENTER)
    {
        // sync bypass value when get into on headphone menu
        if (item->desc->id == HEADPHONE_ID)
        {
            const char *response = cli_command("mod-amixer hp byp", CLI_RETRIEVE_RESPONSE);

            bypass_state = 0;
            if (strcmp(response, "on") == 0)
                bypass_state = 1;
        }
        else
        {
            cli_command("mod-amixer hp byp toggle", CLI_DISCARD_RESPONSE);
            bypass_state = 1 - bypass_state;
            item->data.hover = bypass_state;
        }
    }
}
