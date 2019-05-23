
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
    "sshd",
    "mod-ui",
    "dnsmasq",
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
#define ROUND(x)    ((x) > 0.0 ? (((float)(x)) + 0.5) : (((float)(x)) - 0.5))
#define MAP(x, Omin, Omax, Nmin, Nmax)      ( x - Omin ) * (Nmax -  Nmin)  / (Omax - Omin) + Nmin;


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/
float gains_volumes[5] = {};
uint8_t master_vol_port = 0;
uint8_t q_bypass = 0;
uint8_t bypass[4] = {};
//we boot with profile 5, this one doesn't (yet) exist, once there is a valid profile value here we dont need
//to check it everytime the menu is opened since the profiles can't chang without the MHI
uint8_t current_profile = 5;
uint8_t sl_out, sl_in;

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

static void get_item_value(void *data, menu_item_t *item)
{
    //TODO, PROFILES 
    char **list = data;
    switch (item->desc->id)
    {
        //quick bypass on
        case BP_SELECT_ID:
            item->data.value = atoi(list[2]);
            q_bypass = item->data.value;
        break;
        //bypass 1
        case BP1_ID:
            item->data.value = atoi(list[2]);
            bypass[0] = item->data.value;
        break;
        //bypass 2
        case BP2_ID:
            item->data.value = atoi(list[2]);
            bypass[1] = item->data.value;
        break;
        //bypass 1&2
        case BP12_ID:
            item->data.value = atoi(list[2]);
            bypass[2] = item->data.value;
        break;
        //global tempo
        case TEMPO_ID+1:
            item->data.value = atoi(list[2]);
        break;
        //abetone link beats per barr
        case ABLETON_LINK_ID:
            item->data.value = atoi(list[2]);
        break;
        //tuner mute
        case TUNER_ID:
            item->data.value = atoi(list[2]);
        break;
        //stereo link input on/off
        case STEREO_LINK_INP:
            item->data.value = atoi(list[2]);
            sl_in = item->data.value;
        break;
        //input 1 gain
        case IN1_VOLUME:
            item->data.value = atoi(list[2]);
            system_volume_cb(item, MENU_EV_UP);
        break;
        //input 2 gain
        case IN2_VOLUME:
            item->data.value = atoi(list[2]);
            system_volume_cb(item, MENU_EV_UP);
        break;
        //toggle input port exp/cv
        case EXP_CV_INP:
            item->data.value = atoi(list[2]);
        break;
        //stereo link output on/off
        case STEREO_LINK_OUTP:
            item->data.value = atoi(list[2]);
            sl_out = item->data.value;
        break;
        //output 1 volume
        case OUT1_VOLUME:
            item->data.value = atoi(list[2]);
            system_volume_cb(item, MENU_EV_UP);
        break;
        //output 2 volume
        case OUT2_VOLUME:
            item->data.value = atoi(list[2]);
            system_volume_cb(item, MENU_EV_UP);
        break;
        //master volume link
        case MASTER_VOL_LINK:
            item->data.value = atoi(list[2]);
            master_vol_port = item->data.value;
        break;
        //toggle output port hp/cv
        case HP_CV_OUTP:
            item->data.value = atoi(list[2]);
        break;
        //headphone volume
        case HP_VOLUME:
            item->data.value = atof(list[2]);
            system_volume_cb(item, MENU_EV_UP);
        break;
        //set midi clock source
        case MIDI_CLK_SOURCE:
            item->data.value = atoi(list[2]);
        break;
        //set midi clk send
        case MIDI_CLK_SEND:
            item->data.value = atoi(list[2]);
        break;
        //set midi snapshot channel change
        case MIDI_SNAPSHOT:
            item->data.value = atoi(list[2]);
        break;
        //set midi pedalboard channel change
        case MIDI_PEDALBOARD:
            item->data.value = atoi(list[2]);
        break;
        //set display backlight level
        case DISP_BL_ID:
            item->data.value = (atoi(list[2])/25);
        break;
        case PROFILES_ID+1:
        case PROFILES_ID+2:
        case PROFILES_ID+3:
        case PROFILES_ID+4:
            current_profile = atoi(list[2]);
            item->data.value = (current_profile == (item->desc->id - item->desc->parent_id))?1:0;
        break;
    }
}

void request_item_value(char *command, menu_item_t *item)
{
    // sets the response callback
    comm_webgui_set_response_cb(get_item_value, item);

    // sends the data to GUI
    comm_webgui_send(command, strlen(command));

    //wait till the data is set
    comm_webgui_wait_response();
}

//TODO CHECK IF WE CAN USE DYNAMIC MEMORY HERE
void set_item_value(char *command, uint8_t value)
{
    uint8_t i;
    char buffer[50];

    i = copy_command((char *)buffer, command);

    // copy the value
    char str_buf[8];
    int_to_str(value, str_buf, sizeof(str_buf), 0);
    const char *p = str_buf;
    while (*p)
    {
        buffer[i++] = *p;
        p++;
    }
    buffer[i] = 0;
 
    // sets the response callback
    comm_webgui_set_response_cb(NULL, NULL);

    // sends the data to GUI
    comm_webgui_send(buffer, i);
}

static void volume(menu_item_t *item, int event, const char *source, float min, float max, float step)
{
    char value[8];
    static const char *response = NULL;
    cli_command(NULL, CLI_DISCARD_RESPONSE);
    uint8_t dir = (source[0] == 'i') ? 0 : 1;
    if ((((event == MENU_EV_UP) || (event == MENU_EV_DOWN)) && (dir ? sl_out : sl_in)) && (item->desc->id != HP_VOLUME))
    {
        char vol_cmd[30];
        //change volume for both
        //PGA (input)
        if (!dir)
        {
            strcpy(vol_cmd, "amixer set 'PGA Gain' ");
            int cmd_gain = MAP(item->data.value, -12, 12, 0, 48)
            int_to_str(cmd_gain, value, sizeof value, 1);
            strcat(vol_cmd, value);
            cli_command(vol_cmd, CLI_DISCARD_RESPONSE); 
        }
        //DAC (output)
        else 
        {
            strcpy(vol_cmd, "amixer set DAC ");
            int cmd_gain = MAP(item->data.value, -60, 0, 135, 255)
            int_to_str(cmd_gain, value, sizeof value, 1);
            strcat(vol_cmd, value);
            cli_command(vol_cmd, CLI_DISCARD_RESPONSE);  
        }
    }
    else 
    {
        if ((event == MENU_EV_ENTER) || (event == MENU_EV_NONE))
        {
            cli_command("mod-amixer ", CLI_CACHE_ONLY);
            cli_command(source, CLI_CACHE_ONLY);
            cli_command(" vol ", CLI_CACHE_ONLY);
            response = cli_command(NULL, CLI_RETRIEVE_RESPONSE);
            
            char str[30];
            strcpy(str, response);

            item->data.min = min;
            item->data.max = max; 
            item->data.step = step;

            int res = 0;  // Initialize result 
            int sign = 1;  // Initialize sign as positive 
            int i = 0;  // Initialize index of first digit 
               
            // If number is negative, then update sign 
            if (str[0] == '-') 
            { 
                sign = -1;   
                i++;  // Also update index of first digit 
            } 
               
            // Iterate through all digits and update the result 
            for (; str[i] != '.'; ++i) 
                res = res*10 + (int)str[i] - 48; 
             
            // Return result with sign 
            item->data.value = sign*res;

        }
        else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
        {
            cli_command("mod-amixer ", CLI_CACHE_ONLY);
            cli_command(source, CLI_CACHE_ONLY);
            cli_command(" vol ", CLI_CACHE_ONLY);
            float_to_str(item->data.value, value, sizeof value, 1);
            cli_command(value, CLI_DISCARD_RESPONSE);        
        }
    }

    //save gains globaly for stereo link functions
    gains_volumes[item->desc->id - VOLUME_ID] = item->data.value;

    char str_bfr[8];
    float_to_str(item->data.value, str_bfr, sizeof(str_bfr), 1); 
    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = strlen(str_bfr);
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size - 2); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, str_bfr);
    strcat(item->name, "DB");

    //if stereo link is on we need to update the other menu item as well
    if ((((event == MENU_EV_UP) || (event == MENU_EV_DOWN)) && (dir ? sl_out : sl_in))&& (item->desc->id != HP_VOLUME))
    {
        if (strchr(source, '1')) naveg_update_gain(DISPLAY_RIGHT, item->desc->id + 1, item->data.value);
        else naveg_update_gain(DISPLAY_RIGHT, item->desc->id - 1, item->data.value);    
    }
    naveg_settings_refresh(DISPLAY_RIGHT);
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/
uint8_t system_get_current_profile(void)
{
    return current_profile;
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
        item->data.popup_header = "Firmware version";
        //naveg_settings_refresh(DISPLAY_RIGHT);
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
        char *txt = "The serial number of your     device is:                    ";
        response =  cli_command("cat /var/cache/mod/tag", CLI_RETRIEVE_RESPONSE);
        char * bfr = (char *) MALLOC(1 + strlen(txt)+ strlen(response));
        strcpy(bfr, txt);
        strcat(bfr, response);
        item->data.popup_content = bfr;
        item->data.popup_header = "serial number";
        //naveg_settings_refresh(DISPLAY_RIGHT);
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
                min = -60.0;
                max = 0.0;
                step = 2.0;
                break;

            case OUT2_VOLUME:
                source = "out 2";
                min = -60.0;
                max = 0.0;
                step = 2.0;
                break;

            case HP_VOLUME:
                source = "hp";
                min = -33.0;
                max = 12.0;
                step = 3.0;
                break;
            default:
                return;
                break;
        }

        volume(item, event, source, min, max, step);
}

void system_save_gains_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);

    if (event == MENU_EV_ENTER)
    {
        cli_command("mod-amixer save", CLI_DISCARD_RESPONSE);

        //TODO MAKE MOD_UI AWARE GAINS HAVE CHANGED 
    }
}

void system_banks_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);

    if (event == MENU_EV_ENTER)
    {
        naveg_toggle_tool(DISPLAY_TOOL_NAVIG, 1);
    }
}

void system_display_cb(void *arg, int event)
{
    menu_item_t *item = arg;
/*
    //get from mod-ui
    if (event == MENU_EV_NONE)
    {
        request_item_value(BRIGHTNESS_GET_CMD, item);
    }
    else if (event == MENU_EV_ENTER)
    {
        //set status in mod-ui
        if (++ item->data.value > MAX_BRIGHTNESS)
             item->data.value = 0;
        set_item_value(BRIGHTNESS_SET_CMD, (item->data.value* 25));
        hardware_glcd_brightness(item->data.value);
    }*/
    static int level = 2;
    
    if (event == MENU_EV_ENTER)
    {
        if (++level > MAX_BRIGHTNESS)
            level = 0;

        hardware_glcd_brightness(level);
    }

    item->data.value = level;
    
    char str_buf[8];
    int_to_str((item->data.value * 25), str_buf, sizeof(str_buf), 0);

    strcpy(item->name, item->desc->name);
    uint8_t q;       
    uint8_t value_size = strlen(str_buf);
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size - 1); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, str_buf);
    strcat(item->name, "%");

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_RIGHT);
    
}

void system_sl_in_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;

        set_item_value(SL_IN_SET_CMD, item->data.value);

        if (item->data.value == 1) 
        {
            char vol_cmd[30];
            char value[8];
            strcpy(vol_cmd, "amixer set 'PGA Gain' ");
            int cmd_gain = MAP(gains_volumes[IN1_VOLUME - VOLUME_ID], -12, 12, 0, 48)
            int_to_str(cmd_gain, value, sizeof value, 1);
            strcat(vol_cmd, value);
            cli_command(vol_cmd, CLI_DISCARD_RESPONSE);
            naveg_update_gain(DISPLAY_RIGHT, IN2_VOLUME, gains_volumes[IN1_VOLUME - VOLUME_ID]); 
        }
    }
    else 
    {
        //request_item_value(SL_IN_GET_CMD, item);
    }

    sl_in = item->data.value;
    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = 3;
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, (item->data.value ? " ON" : "OFF"));

    if (event == MENU_EV_ENTER) 
    {
        cli_command("mod-amixer save", CLI_DISCARD_RESPONSE);
        naveg_settings_refresh(DISPLAY_RIGHT);
    }
}

void system_sl_out_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;
        
        set_item_value(SL_OUT_SET_CMD, item->data.value);

        if (item->data.value == 1) 
        {
            char vol_cmd[30];
            char value[8];
            strcpy(vol_cmd, "amixer set DAC ");
            int cmd_gain = MAP(gains_volumes[OUT1_VOLUME - VOLUME_ID], -60, 0, 135, 255)
            int_to_str(cmd_gain, value, sizeof value, 1);
            strcat(vol_cmd, value);
            cli_command(vol_cmd, CLI_DISCARD_RESPONSE);
            naveg_update_gain(DISPLAY_RIGHT, OUT2_VOLUME, gains_volumes[OUT1_VOLUME - VOLUME_ID]);  
        }
    }
    else 
    {
        //request_item_value(SL_OUT_GET_CMD, item);
    }

    sl_out = item->data.value;
    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = 3;
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, (item->data.value ? " ON" : "OFF"));

    if (event == MENU_EV_ENTER)
    {
        cli_command("mod-amixer save", CLI_DISCARD_RESPONSE);
        naveg_settings_refresh(DISPLAY_RIGHT);
    }

    /*if ((!master_vol_port) && item->data.value)
    {
        master_vol_port = 1;
        naveg_menu_refresh(DISPLAY_RIGHT);
    }*/
}

void system_tuner_cb (void *arg, int event)
{    
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;
        set_item_value(TUNER_MUTE_SET_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(TUNER_MUTE_GET_CMD, item);
    }

    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = 8;
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, (item->data.value ? "MUTE [X]" : "MUTE [ ]"));

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_LEFT);   
}


void system_play_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;
        set_item_value(PLAY_SET_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(PLAY_GET_CMD, item);
    }

    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = 8;
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, ( item->data.value ? "PLAY [X]" : "PLAY [ ]"));

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_LEFT);   
}

void system_midi_src_cb (void *arg, int event)
{
    menu_item_t *item = arg;
    
    if (event == MENU_EV_ENTER)
    {
        if (item->data.value < 2) item->data.value++;
        else item->data.value = 0;
        set_item_value(MIDI_SRC_SET_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(MIDI_SRC_GET_CMD, item);
    }

    char str_bfr[13];
    if (item->data.value == 0) strcpy(str_bfr,"INTERNAL");
    else if (item->data.value == 1) strcpy(str_bfr,"MIDI");
    else if (item->data.value == 2) strcpy(str_bfr,"ABLETON LINK");

    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = strlen(str_bfr);
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, (str_bfr));

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_RIGHT); 
}

void system_midi_send_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;
        set_item_value(SEND_MIDI_CLK_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(GET_MIDI_CLK_ENABLE_CMD, item);
    }

    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = 3;
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, ( item->data.value? "[X]" : "[ ]"));

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_RIGHT); 
}

void system_ss_prog_change_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_item_value(MIDI_SNAPSHOT_SET_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(MIDI_SNAPSHOT_GET_CMD, item);
        item->data.min = 0;
        item->data.max = 16;
        item->data.step = 1;
    }

    char str_bfr[8];
    strcpy(item->name, item->desc->name);
    uint8_t q;
    int_to_str(((item->data.value)), str_bfr, 3, 0);
    if ((item->data.value) == 0) strcpy(str_bfr, "OFF");
    uint8_t value_size = strlen(str_bfr);
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, str_bfr);

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_RIGHT); 
}

void system_pb_prog_change_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_item_value(MIDI_PRGCH_SET_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(MIDI_PRGCH_GET_CMD, item);
        item->data.min = 0;
        item->data.max = 16;
        item->data.step = 1;
    }

    char str_bfr[8];
    strcpy(item->name, item->desc->name);
    uint8_t q;
    int_to_str(((item->data.value)), str_bfr, 3, 0);
    if ((item->data.value) == 0) strcpy(str_bfr, "OFF");
    uint8_t value_size = strlen(str_bfr);
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, str_bfr);

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_RIGHT); 
}

void system_tempo_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_item_value(TEMPO_SET_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(TEMPO_GET_CMD, item);
        item->data.min = 20;
        item->data.max = 220;
        item->data.step = 1;
    }

    char str_bfr[8];
    strcpy(item->name, item->desc->name);
    uint8_t q;
    int_to_str(((item->data.value)), str_bfr, sizeof(str_bfr), 0);
    uint8_t value_size = strlen(str_bfr);
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size - 4); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, str_bfr);
    strcat(item->name, " BPM");

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_RIGHT); 
}

void system_bpb_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_item_value(BPB_SET_CMD, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        request_item_value(BPB_GET_CMD, item);
        item->data.min = 1;
        item->data.max = 16;
        item->data.step = 1;
    }

    char str_bfr[8];
    strcpy(item->name, item->desc->name);
    uint8_t q;
    int_to_str(((item->data.value)), str_bfr, sizeof(str_bfr), 0);
    uint8_t value_size = strlen(str_bfr);
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size - 2); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, str_bfr);
    strcat(item->name, "/4");

    if (event == MENU_EV_ENTER) naveg_settings_refresh(DISPLAY_RIGHT); 
}

void system_bypass_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    char cmd_bfr[32];
    char channel[8];

    if (event == MENU_EV_ENTER)
    {   
        //toggle bypass 1 and 2
        if ((item->desc->id - (BYPASS_ID + 1)) == 2)
        {
            //check its own value
            if (item->data.value == 1)
            {
                item->data.value = 0;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
                bypass[0] = 0;
                bypass[1] = 0;
            }    
            else if (item->data.value == 0)
            {            
                item->data.value = 1;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
                bypass[0] = 1;
                bypass[1] = 1;
            }    
                //set channel 1
                strcpy(cmd_bfr, BYPASS_SET_CMD);
                int_to_str(0, channel, sizeof channel, 0);
                strcat(cmd_bfr, channel);
                strcat(cmd_bfr, " ");
                set_item_value(cmd_bfr, item->data.value);  
                //set channel 2
                strcpy(cmd_bfr, BYPASS_SET_CMD);
                int_to_str(1, channel, sizeof channel, 0);
                strcat(cmd_bfr, channel);
                strcat(cmd_bfr, " ");
                set_item_value(cmd_bfr, item->data.value);     
        }
        //toggle quick bypass
        else if ((item->desc->id - (BYPASS_ID + 1)) == 3)
        {
            if (bypass[item->desc->id - (BYPASS_ID + 1)] == 1)
            {
                item->data.value = 2;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
            }  
            else if (bypass[item->desc->id - (BYPASS_ID + 1)] == 2)
            {
                item->data.value = 0;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
            }
            else if (bypass[item->desc->id - (BYPASS_ID + 1)] == 0)
            {            
                item->data.value = 1;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
            }   
        }
        //toggle normal bypass
        else
        {
            if (bypass[item->desc->id - (BYPASS_ID + 1)] == 1)
            {
                item->data.value = 0;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
            }    
            else if (bypass[item->desc->id - (BYPASS_ID + 1)] == 0)
            {            
                item->data.value = 1;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
            }            

            strcpy(cmd_bfr, BYPASS_SET_CMD);
            int_to_str(item->desc->id - (BYPASS_ID), channel, sizeof channel, 0);
            strcat(cmd_bfr, channel);
            strcat(cmd_bfr, " ");
            set_item_value(cmd_bfr, item->data.value);   
        }
    }
    else if (event == MENU_EV_NONE)
    {
        //if normal bypass
        if (((item->desc->id - (BYPASS_ID + 1)) == 0) || ((item->desc->id - (BYPASS_ID + 1)) == 1))
        {
            strcpy(cmd_bfr, BYPASS_GET_CMD);
            int_to_str(item->desc->id - (BYPASS_ID + 1), channel, sizeof channel, 0);
            strcat(cmd_bfr, channel);
            request_item_value(cmd_bfr, item);
            bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
        }
        //if bypass 1 and 2
        else if ((item->desc->id - (BYPASS_ID + 1)) == 2)
        {
            if (bypass[0] && bypass[1])
            {
                item->data.value = 1;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
            }
            else
            {
                item->data.value = 0;
                bypass[item->desc->id - (BYPASS_ID + 1)] = item->data.value;
            }
        }
    }

    //copy bypass txt
    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = 3;
    uint8_t name_size = strlen(item->name);

    //add spaces
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }

    //if bypass select add the channels, else add [X] or  [ ]
    if (item->desc->id == BP_SELECT_ID)
    {
        char channel_value[4];
        switch (bypass[3])
        {
            case 0:
                    strcpy(channel_value, "  1");
                break;
            case 1:
                    strcpy(channel_value, "  2");
                break;
            case 2:
                    strcpy(channel_value, "1&2");
                break;
        }
        strcat(item->name, channel_value);
    }
    else strcat(item->name, ((item->data.value)? "[X]" : "[ ]"));

    if (event == MENU_EV_ENTER)  
    {
        naveg_menu_refresh(DISPLAY_LEFT);
        naveg_settings_refresh(DISPLAY_LEFT);
        naveg_bypass_refresh(bypass[0], bypass[1], bypass[3]);    
    }
}

void system_quick_bypass_cb (void *arg, int event)
{
    uint8_t q_bypass = 0;
    char cmd_bfr[32];
    char channel[8];

    menu_item_t *item = arg;
    
    if (event == MENU_EV_ENTER)
    {
        switch(bypass[3])
        {
            case (0):
                if (bypass[0]) q_bypass = 0;
                else q_bypass = 1;
                bypass[0] = q_bypass;
                strcpy(cmd_bfr, BYPASS_SET_CMD);
                int_to_str(0, channel, sizeof channel, 0);
                strcat(cmd_bfr, channel);
                strcat(cmd_bfr, " ");
                set_item_value(cmd_bfr, q_bypass);
            break;
            case (1):
                if (bypass[1]) q_bypass = 0;
                else q_bypass = 1;
                bypass[1] = q_bypass;
                strcpy(cmd_bfr, BYPASS_SET_CMD);
                int_to_str(1, channel, sizeof channel, 0);
                strcat(cmd_bfr, channel);
                strcat(cmd_bfr, " ");
                set_item_value(cmd_bfr, q_bypass);
            break;
            case (2):
                if ((bypass[0])&&(bypass[1])) q_bypass = 0;
                else q_bypass = 1;
                bypass[0] = q_bypass;
                bypass[1] = q_bypass;
                strcpy(cmd_bfr, BYPASS_SET_CMD);
                int_to_str(0, channel, sizeof channel, 0);
                strcat(cmd_bfr, channel);
                strcat(cmd_bfr, " ");
                set_item_value(cmd_bfr, q_bypass);
                strcpy(cmd_bfr, BYPASS_SET_CMD);
                int_to_str(1, channel, sizeof channel, 0);
                strcat(cmd_bfr, channel);
                strcat(cmd_bfr, " ");
                set_item_value(cmd_bfr, q_bypass);
            break;
        }
    }
    else if (event == MENU_EV_NONE)
    {
        switch(bypass[3])
        {
            case (0):
                if (bypass[0]) q_bypass = 1;
                else q_bypass = 0;
            break;
            case (1):
                if (bypass[1]) q_bypass = 1;
                else q_bypass = 0;
            break;
            case (2):
                if ((bypass[0])&&(bypass[1])) q_bypass = 1;
                else q_bypass = 0;
            break;
        }
    }

    strcpy(item->name, item->desc->name);
    uint8_t q;
    uint8_t value_size = 10;
    uint8_t name_size = strlen(item->name);
    for (q = 0; q < (31 - name_size - value_size); q++)
    {
        strcat(item->name, " ");  
    }
    strcat(item->name, (q_bypass ? "BYPASS [X]" : "BYPASS [ ]"));

    if (event == MENU_EV_ENTER)  
    {
        naveg_settings_refresh(DISPLAY_LEFT);
        naveg_bypass_refresh(bypass[0], bypass[1], bypass[3]);    
    }

}

void system_load_pro_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER && item->data.hover == 0)
    {
        current_profile = item->desc->id - item->desc->parent_id;
        item->data.value = current_profile;

        //set_item_value(LOAD_PROFILE_CMD, current_profile);

        naveg_menu_refresh(DISPLAY_LEFT);
        naveg_menu_refresh(DISPLAY_RIGHT);
    }
    else if (event == MENU_EV_NONE)
    {
        //first time getting the profile, need to check with mod-ui
        if (current_profile == 5)
        {
            //request_item_value(GET_PROFILE_CMD, item);
        }
        
        if ((item->desc->id - item->desc->parent_id) == current_profile)
        {
            item->data.value = current_profile;
            strcpy(item->name, item->desc->name);
            uint8_t value_size = 3;
            uint8_t name_size = strlen(item->name);
            uint8_t q;
            for (q = 0; q < (31 - name_size - value_size); q++)
            {
                strcat(item->name, " ");  
            }
            strcat(item->name, "[X]");
        }
        else strcpy(item->name, item->desc->name);
    }
    naveg_settings_refresh(DISPLAY_RIGHT);
}

void system_save_pro_cb(void *arg, int event)
{
   menu_item_t *item = arg;
    
    if (event == MENU_EV_ENTER && item->data.hover == 0)
    {
        set_item_value(STORE_PROFILE_CMD, current_profile);
    }
    else if (event == MENU_EV_NONE)
    {
        strcpy(item->name, item->desc->name);
        uint8_t value_size = 3;
        uint8_t name_size = strlen(item->name);
        uint8_t q;
        for (q = 0; q < (31 - name_size - value_size); q++)
        {
            strcat(item->name, " ");  
        }
        switch (current_profile)
        {
            case 1: strcat(item->name, ("[A]")); break;
            case 2: strcat(item->name, ("[B]")); break;
            case 3: strcat(item->name, ("[C]")); break;
            case 4: strcat(item->name, ("[D]")); break;
        }   
    }
}

