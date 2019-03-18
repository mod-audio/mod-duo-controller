
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

const char *versions_names[] = {
    "version",
    "restore",
    "system",
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

void system_pedalboard_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER && item->data.hover == 0)
    {
        switch (item->desc->id)
        {
            case PEDALBOARD_SAVE_ID:
                comm_webgui_send(PEDALBOARD_SAVE_CMD, strlen(PEDALBOARD_SAVE_CMD));
                break;

            case PEDALBOARD_RESET_ID:
                comm_webgui_send(PEDALBOARD_RESET_CMD, strlen(PEDALBOARD_RESET_CMD));
                break;
        }
    }
}

void system_bluetooth_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        if (item->desc->id == BLUETOOTH_ID)
        {
            response = cli_command("mod-bluetooth status", CLI_RETRIEVE_RESPONSE);
            update_status(item->data.list[2], response);
            response = cli_command("mod-bluetooth name", CLI_RETRIEVE_RESPONSE);
            update_status(item->data.list[3], response);
            response = cli_command("mod-bluetooth address", CLI_RETRIEVE_RESPONSE);
            update_status(item->data.list[4], response);
        }
        else if (item->desc->id == BLUETOOTH_DISCO_ID)
        {
            cli_command("mod-bluetooth discovery", CLI_DISCARD_RESPONSE);
        }
    }
}

void system_services_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        uint8_t i = 0;
        while (systemctl_services[i])
        {
            const char *response;
            response = cli_systemctl("is-active ", systemctl_services[i]);
            update_status(item->data.list[i+1], response);
            i++;
        }
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
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        char version[8];

        uint8_t i = 0;
        while (versions_names[i])
        {
            cli_command("mod-version ", CLI_CACHE_ONLY);
            response = cli_command(versions_names[i], CLI_RETRIEVE_RESPONSE);
            strncpy(version, response, (sizeof version) - 1);
            version[(sizeof version) - 1] = 0;
            update_status(item->data.list[i+1], version);
            screen_system_menu(item);
            i++;
        }
    }
}

void system_release_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        response = cli_command("mod-version release", CLI_RETRIEVE_RESPONSE);
        item->data.popup_content = response;
    }
}

void system_device_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        response = cli_command("cat /var/cache/mod/tag", CLI_RETRIEVE_RESPONSE);
        update_status(item->data.list[1], response);
    }
}

void system_tag_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        response = cli_command("cat /var/cache/mod/tag", CLI_RETRIEVE_RESPONSE);
        item->data.popup_content = response;
    }
}

void system_upgrade_cb(void *arg, int event)
{
    if (event == MENU_EV_ENTER)
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
                // start restore
                cli_restore(RESTORE_INIT);
            }
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
        default:
            source = '\0';
            min = 0;
            max = 0;
            step = 0;
            break;
    }

    volume(item, event, source, min, max, step);
}

void system_stage_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        char input[3] = {'1', ' ', 0};

        if (item->desc->id == IN2_STAGE_ID || item->desc->parent_id == IN2_STAGE_ID)
            input[0]++;

        cli_command("mod-amixer in ", CLI_CACHE_ONLY);
        cli_command(input, CLI_CACHE_ONLY);
        cli_command("stg ", CLI_CACHE_ONLY);

        if (item->desc->id == IN1_STAGE_ID ||
            item->desc->id == IN2_STAGE_ID)
        {
            const char *response = cli_command(NULL, CLI_RETRIEVE_RESPONSE);

            uint8_t i;
            for (i = 1; i < item->data.list_count; i++)
            {
                deselect_item(item->data.list[i]);
                if (strcmp(item->data.list[i], response) == 0)
                    select_item(item->data.list[i]);
            }
        }
        else
        {
            cli_command(item->desc->name, CLI_DISCARD_RESPONSE);
        }
    }
}

void system_hp_bypass_cb(void *arg, int event)
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

void system_save_gains_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);

    if (event == MENU_EV_ENTER)
    {
        cli_command("mod-amixer save", CLI_DISCARD_RESPONSE);
    }
}

void system_banks_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);

    if (event == MENU_EV_ENTER)
    {
        naveg_toggle_tool(DISPLAY_TOOL_NAVIG, 0);
    }
}

void system_display_cb(void *arg, int event)
{
    menu_item_t *item = arg;
    static int level = 2;

    if (event == MENU_EV_ENTER)
    {
        if (++level > MAX_BRIGHTNESS)
            level = 0;

        hardware_glcd_brightness(level);

        char str_buf[8];
        int_to_str((level * 25), str_buf, sizeof(str_buf), 0);

        strcpy(item->name, item->desc->name);
        strcat(item->name, "        ");
        strcat(item->name, str_buf);
        strcat(item->name, "%");
        naveg_settings_refresh();
    }
}
