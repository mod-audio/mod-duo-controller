
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "naveg.h"
#include "node.h"
#include "config.h"
#include "screen.h"
#include "utils.h"
#include "led.h"
#include "hardware.h"
#include "comm.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {TT_INIT, TT_COUNTING};
enum {TOOL_OFF, TOOL_ON};
enum {BANKS_LIST, PEDALBOARD_LIST};

#define MAX_CHARS_MENU_NAME     (128/4)
#define MAX_TOOLS               4


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static menu_desc_t g_menu_desc[] = {
    SYSTEM_MENU
    {NULL, 0, -1, -1, NULL, 0}
};

static const menu_popup_t g_menu_popups[] = {
    POPUP_CONTENT
    {-1, NULL}
};


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

struct TAP_TEMPO_T {
    uint32_t time, max;
    uint8_t state;
} g_tap_tempo[SLOTS_COUNT];

struct TOOL_T {
    uint8_t state, display;
} g_tool[MAX_TOOLS];


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

static control_t *g_controls[SLOTS_COUNT], *g_foots[SLOTS_COUNT];
static bp_list_t *g_banks, *g_naveg_pedalboards, *g_selected_pedalboards;
static uint8_t g_bp_state, g_current_bank, g_current_pedalboard;
static node_t *g_menu, *g_current_menu;
static menu_item_t *g_current_item;
static uint8_t g_max_items_list;
static bank_config_t g_bank_functions[BANK_FUNC_AMOUNT];
static uint8_t g_initialized, g_ui_connected;
static void (*g_update_cb)(void *data, int event);
static void *g_update_data;
static xSemaphoreHandle g_dialog_sem;


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static void display_control_add(control_t *control);
static void display_control_rm(int32_t effect_instance, const char *symbol);

static void foot_control_add(control_t *control);
static void foot_control_rm(int32_t effect_instance, const char *symbol);

static uint8_t bank_config_check(uint8_t foot);
static void bank_config_update(uint8_t bank_func_idx);
static void bank_config_footer(void);


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

static void tool_on(uint8_t tool, uint8_t display)
{
    g_tool[tool].state = TOOL_ON;
    g_tool[tool].display = display;
}

static void tool_off(uint8_t tool)
{
    g_tool[tool].state = TOOL_OFF;
}

static int tool_is_on(uint8_t tool)
{
    return g_tool[tool].state;
}

static void display_disable_all_tools(uint8_t display)
{
    int i;
    for (i = 0; i < MAX_TOOLS; i++)
    {
        if (g_tool[i].display == display)
            g_tool[i].state = TOOL_OFF;
    }
}

static int display_has_tool_enabled(uint8_t display)
{
    int i;
    for (i = 0; i < MAX_TOOLS; i++)
    {
        if (g_tool[i].display == display && g_tool[i].state == TOOL_ON)
            return 1;
    }

    return 0;
}

// search the control
static control_t *search_control(int32_t effect_instance, const char *symbol, uint8_t *display)
{
    uint8_t i;
    control_t *control;

    for (i = 0; i < SLOTS_COUNT; i++)
    {
        control = g_controls[i];
        if (control)
        {
            if (control->effect_instance == effect_instance &&
                strcmp(control->symbol, symbol) == 0)
            {
                (*display) = i;
                return control;
            }
        }
    }

    return NULL;
}

// calculates the control value using the step
static void step_to_value(control_t *control)
{
    // about the calculation: http://lv2plug.in/ns/ext/port-props/#rangeSteps

    float p_step = ((float) control->step) / ((float) (control->steps - 1));
    switch (control->properties)
    {
        case CONTROL_PROP_LINEAR:
        case CONTROL_PROP_INTEGER:
            control->value = (p_step * (control->maximum - control->minimum)) + control->minimum;
            break;

        case CONTROL_PROP_LOGARITHMIC:
            control->value = control->minimum * pow(control->maximum / control->minimum, p_step);
            break;

        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
            control->value = control->scale_points[control->step]->value;
            break;
    }

    if (control->value > control->maximum) control->value = control->maximum;
    if (control->value < control->minimum) control->value = control->minimum;
}

// copy an command to buffer
static uint8_t copy_command(char *buffer, const char *command)
{
    uint8_t i = 0;
    const char *cmd = command;

    while (*cmd && (*cmd != '%' && *cmd != '.'))
    {
        buffer[i++] = *cmd;
        cmd++;
    }

    return i;
}

// control assigned to display
static void display_control_add(control_t *control)
{
    uint8_t display;

    display = control->actuator_id;

    // checks if is already a control assigned in this display and remove it
    if (g_controls[display]) data_free_control(g_controls[display]);

    // assign the new control
    g_controls[display] = control;

    // calculates initial step
    switch (control->properties)
    {
        case CONTROL_PROP_LINEAR:
            control->step =
                (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);
            break;

        case CONTROL_PROP_LOGARITHMIC:
            if (control->minimum == 0.0)
                control->minimum = FLT_MIN;

            if (control->maximum == 0.0)
                control->maximum = FLT_MIN;

            if (control->value == 0.0)
                control->value = FLT_MIN;

            control->step =
                (control->steps - 1) * log(control->value / control->minimum) / log(control->maximum / control->minimum);
            break;

        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
            control->step = 0;
            uint8_t i;
            for (i = 0; i < control->scale_points_count; i++)
            {
                if (control->value == control->scale_points[i]->value)
                {
                    control->step = i;
                    break;
                }
            }
            control->steps = control->scale_points_count;
            break;

        case CONTROL_PROP_INTEGER:
            control->steps = (control->maximum - control->minimum) + 1;
            control->step =
                (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);
            break;
    }

    // if tool is enabled don't draws the control
    if (display_has_tool_enabled(display)) return;

    // update the control screen
    screen_control(display, control);

    // update the controls index screen
    screen_controls_index(display, control->control_index, control->controls_count);
}

// control removed from display
static void display_control_rm(int32_t effect_instance, const char *symbol)
{
    uint8_t display;

    control_t *control = search_control(effect_instance, symbol, &display);
    if (control)
    {
        data_free_control(control);
        g_controls[display] = NULL;
        if (!display_has_tool_enabled(display)) screen_control(display, NULL);
        return;
    }

    uint8_t all_effects, all_controls;
    all_effects = (effect_instance == ALL_EFFECTS) ? 1 : 0;
    all_controls = (strcmp(symbol, ALL_CONTROLS) == 0) ? 1 : 0;

    for (display = 0; display < SLOTS_COUNT; display++)
    {
        control = g_controls[display];

        if (all_effects || control->effect_instance == effect_instance)
        {
            if (all_controls || strcmp(control->symbol, symbol) == 0)
            {
                data_free_control(control);
                g_controls[display] = NULL;

                if (!display_has_tool_enabled(display)) screen_control(display, NULL);
            }
        }
    }
}

// control assigned to foot
static void foot_control_add(control_t *control)
{
    uint8_t i;

    // checks the actuator id
    if (control->actuator_id >= FOOTSWITCHES_COUNT) return;

    // checks if the actuator is used like bank function
    if (bank_config_check(control->actuator_id))
    {
        data_free_control(control);
        return;
    }

    // checks if the foot is already used by other control and not is state updating
    if (g_foots[control->actuator_id] && g_foots[control->actuator_id] != control)
    {
        data_free_control(control);
        return;
    }

    // stores the foot
    g_foots[control->actuator_id] = control;

    // default state of led blink (no blink)
    led_blink(hardware_leds(control->actuator_id), 0, 0);

    switch (control->properties)
    {
        // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
        case CONTROL_PROP_TOGGLED:
            // updates the led
            if (control->value <= 0)
                led_set_color(hardware_leds(control->actuator_id), BLACK);
            else
                led_set_color(hardware_leds(control->actuator_id), TOGGLED_COLOR);

            // if is in tool mode break
            if (display_has_tool_enabled(control->actuator_id)) break;

            // updates the footer
            screen_footer(control->actuator_id, control->label,
                         (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT));
            break;

        // trigger specification: http://lv2plug.in/ns/ext/port-props/#trigger
        case CONTROL_PROP_TRIGGER:
            // updates the led
            led_set_color(hardware_leds(control->actuator_id), TRIGGER_COLOR);

            // if is in tool mode break
            if (display_has_tool_enabled(control->actuator_id)) break;

            // updates the footer
            screen_footer(control->actuator_id, control->label, NULL);
            break;

        case CONTROL_PROP_TAP_TEMPO:
            // defines the led color
            led_set_color(hardware_leds(control->actuator_id), TAP_TEMPO_COLOR);

            // convert the time unit
            uint16_t time_ms = (uint16_t)(convert_to_ms(control->unit, control->value) + 0.5);

            // setup the led blink
            if (time_ms > TAP_TEMPO_TIME_ON)
                led_blink(hardware_leds(control->actuator_id), TAP_TEMPO_TIME_ON, time_ms - TAP_TEMPO_TIME_ON);
            else
                led_blink(hardware_leds(control->actuator_id), time_ms / 2, time_ms / 2);

            // calculates the maximum tap tempo value
            if (g_tap_tempo[control->actuator_id].state == TT_INIT)
            {
                uint32_t max;

                // time unit (ms, s)
                if (strcmp(control->unit, "ms") == 0 || strcmp(control->unit, "s") == 0)
                    max = (uint32_t)(convert_to_ms(control->unit, control->maximum) + 0.5);
                // frequency unit (bpm, Hz)
                else
                    max = (uint32_t)(convert_to_ms(control->unit, control->minimum) + 0.5);

                g_tap_tempo[control->actuator_id].max = max;
                g_tap_tempo[control->actuator_id].state = TT_COUNTING;
            }

            // if is in tool mode break
            if (display_has_tool_enabled(control->actuator_id)) break;

            // footer text composition
            char value_txt[32];
            i = int_to_str(control->value, value_txt, sizeof(value_txt), 0);
            value_txt[i++] = ' ';
            strcpy(&value_txt[i], control->unit);

            // updates the footer
            screen_footer(control->actuator_id, control->label, value_txt);
            break;

        case CONTROL_PROP_BYPASS:
            // updates the led
            if (control->value <= 0)
                led_set_color(hardware_leds(control->actuator_id), BYPASS_COLOR);
            else
                led_set_color(hardware_leds(control->actuator_id), BLACK);

            // if is in tool mode break
            if (display_has_tool_enabled(control->actuator_id)) break;

            // updates the footer
            screen_footer(control->actuator_id, control->label,
                         (control->value ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT));
            break;

        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
            // updates the led
            led_set_color(hardware_leds(control->actuator_id), ENUMERATED_COLOR);

            // locates the current value
            control->step = 0;
            for (i = 0; i < control->scale_points_count; i++)
            {
                if (control->value == control->scale_points[i]->value)
                {
                    control->step = i;
                    break;
                }
            }
            control->steps = control->scale_points_count;

            // if is in tool mode break
            if (display_has_tool_enabled(control->actuator_id)) break;

            // updates the footer
            screen_footer(control->actuator_id, control->label, control->scale_points[i]->label);
            break;
    }
}

// control removed from foot
static void foot_control_rm(int32_t effect_instance, const char *symbol)
{
    uint8_t i, all_effects, all_controls;

    all_effects = (effect_instance == ALL_EFFECTS) ? 1 : 0;
    all_controls = (strcmp(symbol, ALL_CONTROLS) == 0) ? 1 : 0;

    for (i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        // if there is no controls assigned, load the default screen
        if (!g_foots[i] && ! bank_config_check(i) && !display_has_tool_enabled(i))
        {
            screen_footer(i, NULL, NULL);
            continue;
        }

        // checks if effect_instance and symbol match
        if (all_effects || effect_instance == g_foots[i]->effect_instance)
        {
            if (all_controls || strcmp(symbol, g_foots[i]->symbol) == 0)
            {
                // remove the control
                data_free_control(g_foots[i]);
                g_foots[i] = NULL;

                // check if foot isn't being used to bank function
                if (! bank_config_check(i))
                {
                    // turn off the led
                    led_set_color(hardware_leds(i), BLACK);

                    // update the footer
                    if (!display_has_tool_enabled(i))
                        screen_footer(i, NULL, NULL);
                }
            }
        }
    }
}

static void control_set(uint8_t display, control_t *control)
{
    uint32_t delta, now;

    switch (control->properties)
    {
        case CONTROL_PROP_LINEAR:
        case CONTROL_PROP_INTEGER:
        case CONTROL_PROP_LOGARITHMIC:

            // update the screen
            if (!display_has_tool_enabled(display))
                screen_control(display, control);
            break;

        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
            if (control->actuator_type == KNOB)
            {
                // update the screen
                if (!display_has_tool_enabled(display))
                    screen_control(display, control);
            }
            else if (control->actuator_type == FOOT)
            {
                // increments the step
                control->step++;
                if (control->step >= control->scale_points_count) control->step = 0;

                // updates the value and the screen
                control->value = control->scale_points[control->step]->value;
                if (!display_has_tool_enabled(display))
                    screen_footer(control->actuator_id, control->label, control->scale_points[control->step]->label);
            }
            break;

        case CONTROL_PROP_TOGGLED:
        case CONTROL_PROP_BYPASS:
            if (control->value > 0) control->value = 0;
            else control->value = 1;

            // to update the footer and screen
            foot_control_add(control);
            break;

        case CONTROL_PROP_TRIGGER:
            control->value = 1;

            // to update the footer and screen
            foot_control_add(control);
            break;

        case CONTROL_PROP_TAP_TEMPO:
            now = hardware_timestamp();
            delta = now - g_tap_tempo[control->actuator_id].time;
            g_tap_tempo[control->actuator_id].time = now;

            if (g_tap_tempo[control->actuator_id].state == TT_COUNTING)
            {
                // checks the tap tempo timeout
                if (delta < g_tap_tempo[control->actuator_id].max)
                {
                    // converts and update the tap tempo value
                    control->value = convert_from_ms(control->unit, delta);

                    // checks the values bounds
                    if (control->value > control->maximum) control->value = control->maximum;
                    if (control->value < control->minimum) control->value = control->minimum;

                    // updates the foot
                    foot_control_add(control);
                }
            }
            break;
    }

    char buffer[128];
    uint8_t i;

    i = copy_command(buffer, CONTROL_SET_CMD);

    // insert the instance on buffer
    i += int_to_str(control->effect_instance, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i++] = ' ';

    // insert the symbol on buffer
    strcpy(&buffer[i], control->symbol);
    i += strlen(control->symbol);
    buffer[i++] = ' ';

    // insert the value on buffer
    i += float_to_str(control->value, &buffer[i], sizeof(buffer) - i, 3);
    buffer[i] = 0;

    // send the data to GUI
    comm_webgui_send(buffer, i);
}

static void parse_banks_list(void *data)
{
    char **list = data;
    uint32_t count = strarr_length(list) - 2;

    // free the current banks list
    if (g_banks) data_free_banks_list(g_banks);

    // parses the list
    g_banks = data_parse_banks_list(&list[2], count);
    if (g_banks) g_banks->selected = g_current_bank;
    naveg_set_banks(g_banks);
}

static void request_banks_list(void)
{
    g_bp_state = BANKS_LIST;

    // sets the response callback
    comm_webgui_set_response_cb(parse_banks_list);

    // sends the data to GUI
    comm_webgui_send(BANKS_CMD, strlen(BANKS_CMD));

    // waits the banks list be received
    comm_webgui_wait_response();
}

static void parse_pedalboards_list(void *data)
{
    char **list = data;
    uint32_t count = strarr_length(list) - 2;

    // free the navigation pedalboads list
    if (g_naveg_pedalboards && (!g_selected_pedalboards || g_selected_pedalboards != g_naveg_pedalboards))
        data_free_pedalboards_list(g_naveg_pedalboards);

    // parses the list
    g_naveg_pedalboards = data_parse_pedalboards_list(&list[2], count);
}

static void request_pedalboards_list(const char *bank_uid)
{
    uint8_t i;
    char buffer[128];

    i = copy_command((char *)buffer, PEDALBOARDS_CMD);

    // copy the bank uid
    const char *p = bank_uid;
    while (*p)
    {
        buffer[i++] = *p;
        p++;
    }
    buffer[i] = 0;

    // sets the response callback
    comm_webgui_set_response_cb(parse_pedalboards_list);

    // sends the data to GUI
    comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    comm_webgui_wait_response();
}

static void send_load_pedalboard(uint8_t bank_id, const char *pedalboard_uid)
{
    uint8_t i;
    char buffer[128];

    i = copy_command((char *)buffer, PEDALBOARD_CMD);

    // copy the bank id
    i += int_to_str(bank_id, &buffer[i], 8, 0);

    // inserts one space
    buffer[i++] = ' ';

    // copy the pedalboard uid
    const char *p = pedalboard_uid;
    while (*p)
    {
        buffer[i++] = *p;
        p++;
    }
    buffer[i] = 0;

    // send the data to GUI
    comm_webgui_send(buffer, i);
}

static void bp_enter(void)
{
    bp_list_t *bp_list;
    const char *title;

    if (naveg_ui_status())
    {
        tool_off(DISPLAY_TOOL_NAVIG);
        tool_on(DISPLAY_TOOL_SYSTEM, 0);
        screen_system_menu(g_current_item);
    }

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
        if (g_banks->hover == 0)
        {
            tool_off(DISPLAY_TOOL_NAVIG);
            tool_on(DISPLAY_TOOL_SYSTEM, 0);
            screen_system_menu(g_current_item);
            return;
        }

        request_pedalboards_list(g_banks->uids[g_banks->hover]);
        if (!g_naveg_pedalboards) return;

        // if reach here, received the pedalboards list
        g_bp_state = PEDALBOARD_LIST;
        g_naveg_pedalboards->hover = 0;
        bp_list = g_naveg_pedalboards;
        title = g_banks->names[g_banks->hover];

        // sets the selected pedalboard out of range (initial state)
        g_naveg_pedalboards->selected = g_naveg_pedalboards->count;

        // defines the selected pedalboard
        if (g_banks->selected == g_banks->hover)
        {
            g_naveg_pedalboards->selected = g_current_pedalboard;
        }
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        // checks if is the first option (back to banks list)
        if (g_naveg_pedalboards->hover == 0)
        {
            g_bp_state = BANKS_LIST;
            bp_list = g_banks;
            title = "BANKS";
        }
        else
        {
            // updates selected bank and pedalboard
            g_banks->selected = g_banks->hover;
            g_naveg_pedalboards->selected = g_naveg_pedalboards->hover;

            // request to GUI load the pedalboard
            send_load_pedalboard(g_banks->selected - 1, g_naveg_pedalboards->uids[g_naveg_pedalboards->selected]);

            // if select a pedalboard in other bank free the old pedalboards list
            if (g_current_bank != g_banks->selected)
            {
                if (g_selected_pedalboards) data_free_pedalboards_list(g_selected_pedalboards);
                g_selected_pedalboards = NULL;
            }

            // stores the current bank and pedalboard
            g_current_bank = g_banks->selected;
            g_current_pedalboard = g_naveg_pedalboards->selected;

            // sets the variables to update the screen
            title = g_banks->names[g_banks->hover];
            bp_list = g_naveg_pedalboards;

            // if has a valid pedalboards list update the screens
            if (g_selected_pedalboards) bank_config_footer();
        }
    }

    screen_bp_list(title, bp_list);
}

static void bp_up(void)
{
    bp_list_t *bp_list;
    const char *title;

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
        if (g_banks->hover > 0) g_banks->hover--;
        bp_list = g_banks;
        title = "BANKS";
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        if (g_naveg_pedalboards->hover > 0) g_naveg_pedalboards->hover--;
        bp_list = g_naveg_pedalboards;
        title = g_banks->names[g_banks->hover];
    }

    screen_bp_list(title, bp_list);
}

static void bp_down(void)
{
    bp_list_t *bp_list;
    const char *title;

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
        if (g_banks->hover < (g_banks->count - 1)) g_banks->hover++;
        bp_list = g_banks;
        title = "BANKS";
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        if (g_naveg_pedalboards->hover < (g_naveg_pedalboards->count - 1)) g_naveg_pedalboards->hover++;
        bp_list = g_naveg_pedalboards;
        title = g_banks->names[g_banks->hover];
    }

    screen_bp_list(title, bp_list);
}

static void menu_enter(void)
{
    uint8_t i;
    node_t *node = g_current_menu;
    menu_item_t *item = g_current_item;

    // checks the current item
    if (g_current_item->desc->type == MENU_LIST || g_current_item->desc->type == MENU_SELECT)
    {
        // locates the clicked item
        node = g_current_menu->first_child;
        for (i = 0; i < g_current_item->data.hover; i++) node = node->next;

        // gets the menu item
        item = node->data;

        // checks if is 'back to previous'
        if (item->desc->type == MENU_RETURN)
        {
            if (item->desc->action_cb)
                item->desc->action_cb(item, MENU_EV_ENTER);

            node = node->parent->parent;
            item = node->data;
        }

        // updates the current item
        if (!MENU_ITEM_IS_TOGGLE_TYPE(item) && item->desc->type != MENU_NONE) g_current_item = node->data;
    }
    else if (g_current_item->desc->type == MENU_CONFIRM || g_current_item->desc->type == MENU_CANCEL || g_current_item->desc->type == MENU_OK)
    {
        item = g_current_item;

        // calls the action callback
        if (g_current_item->desc->type == MENU_CONFIRM && item->desc->action_cb)
            item->desc->action_cb(item, MENU_EV_ENTER);

        // gets the menu item
        item = g_current_menu->data;
        g_current_item = item;
    }
    else if (g_current_item->desc->type == MENU_GRAPH)
    {
        // if got a click on graph screen go back to parent
        node = node->parent;
        item = node->data;

        // updates the current item
        g_current_item = node->data;
    }

    // FIXME: that's dirty, so dirty...
    if (item->desc->id == PEDALBOARD_ID)
    {
        if (naveg_ui_status())
            item->desc->type = MENU_OK;
        else
            item->desc->type = MENU_LIST;
    }

    // checks the selected item
    if (item->desc->type == MENU_LIST || item->desc->type == MENU_SELECT)
    {
        // changes the current menu
        g_current_menu = node;

        // initialize the counter
        item->data.list_count = 0;

        // adds the menu lines
        for (node = node->first_child; node; node = node->next)
        {
            menu_item_t *item_child = node->data;
            item->data.list[item->data.list_count++] = item_child->name;

            // checks if is toggle type and insert the right value
            if (item_child->desc->type == MENU_ON_OFF)
            {
                strcpy(item_child->name, item_child->desc->name);
                strcat(item_child->name, (item_child->data.hover ? " ON" : "OFF"));
            }
            else if (item_child->desc->type == MENU_YES_NO)
            {
                strcpy(item_child->name, item_child->desc->name);
                strcat(item_child->name, (item_child->data.hover ? "YES" : " NO"));
            }
            else if (item_child->desc->type == MENU_BYP_PROC)
            {
                strcpy(item_child->name, item_child->desc->name);
                strcat(item_child->name, (item_child->data.hover ? "  BYP" : "PROC"));
            }
        }

        // calls the action callback
        if (item->desc->action_cb) item->desc->action_cb(item, MENU_EV_ENTER);
    }
    else if (item->desc->type == MENU_CONFIRM)
    {
        // highlights the default button
        item->data.hover = 1;

        // defines the buttons count
        item->data.list_count = 2;

        // default popup content value
        item->data.popup_content = NULL;

        // locates the popup menu
        i = 0;
        while (g_menu_popups[i].popup_content)
        {
            if (item->desc->id == g_menu_popups[i].menu_id)
            {
                item->data.popup_content = g_menu_popups[i].popup_content;
                break;
            }
            i++;
        }
    }
    else if (item->desc->type == MENU_CANCEL || item->desc->type == MENU_OK)
    {
        // highlights the default button
        item->data.hover = 0;

        // defines the buttons count
        item->data.list_count = 1;

        // default popup content value
        item->data.popup_content = NULL;

        // locates the popup menu
        i = 0;
        while (g_menu_popups[i].popup_content)
        {
            if (item->desc->id == g_menu_popups[i].menu_id)
            {
                item->data.popup_content = g_menu_popups[i].popup_content;
                break;
            }
            i++;
        }

        // calls the action callback
        if (item->desc->action_cb) item->desc->action_cb(item, MENU_EV_ENTER);
    }
    else if (MENU_ITEM_IS_TOGGLE_TYPE(item))
    {
        item->data.hover = 1 - item->data.hover;

        // calls the action callback
        if (item->desc->action_cb) item->desc->action_cb(item, MENU_EV_ENTER);
    }
    else if (item->desc->type == MENU_NONE)
    {
        // checks if the parent item type is MENU_SELECT
        if (g_current_item->desc->type == MENU_SELECT)
        {
            // deselects all items
            for (i = 1; i < g_current_item->data.list_count; i++)
                deselect_item(g_current_item->data.list[i]);

            // selects the current item
            select_item(item->name);
        }

        // calls the action callback
        if (item->desc->action_cb) item->desc->action_cb(item, MENU_EV_ENTER);
    }
    else if (item->desc->type == MENU_GRAPH)
    {
        // change current menu
        g_current_menu = node;

        // calls the action callback
        if (item->desc->action_cb) item->desc->action_cb(item, MENU_EV_ENTER);
    }

    if (tool_is_on(DISPLAY_TOOL_SYSTEM) && !tool_is_on(DISPLAY_TOOL_NAVIG))
    {
        screen_system_menu(item);

        g_update_cb = NULL;
        g_update_data = NULL;
        if (item->desc->need_update)
        {
            g_update_cb = item->desc->action_cb;
            g_update_data = item;
        }
    }

    if (item->desc->type == MENU_CONFIRM2)
    {
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(g_dialog_sem, &xHigherPriorityTaskWoken);
    }
}

static void menu_up(void)
{
    menu_item_t *item = g_current_item;
    if (item->desc->type == MENU_GRAPH)
    {
        item->data.value -= item->data.step;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;
    }
    else
    {
        if (item->data.hover > 0) item->data.hover--;
    }

    if (item->desc->action_cb)
        item->desc->action_cb(item, MENU_EV_UP);

    screen_system_menu(item);
}

static void menu_down(void)
{
    menu_item_t *item = g_current_item;
    if (item->desc->type == MENU_GRAPH)
    {
        item->data.value += item->data.step;
        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
    }
    else
    {
        if (item->data.hover < (item->data.list_count - 1)) item->data.hover++;
    }

    if (item->desc->action_cb)
        item->desc->action_cb(item, MENU_EV_DOWN);

    screen_system_menu(item);
}

static void tuner_enter(void)
{
    static uint8_t input = 1;

    char buffer[128];
    uint32_t i = copy_command(buffer, TUNER_INPUT_CMD);

    // toggle the input
    input = (input == 1 ? 2 : 1);

    // inserts the input number
    i += int_to_str(input, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i] = 0;

    // send the data to GUI
    comm_webgui_send(buffer, i);

    // updates the screen
    screen_tuner_input(input);
}

static void create_menu_tree(node_t *parent, const menu_desc_t *desc)
{
    uint8_t i;
    menu_item_t *item;

    for (i = 0; g_menu_desc[i].name; i++)
    {
        if (desc->id == g_menu_desc[i].parent_id)
        {
            item = (menu_item_t *) MALLOC(sizeof(menu_item_t));
            item->data.hover = 0;
            item->data.selected = 0xFF;
            item->data.list_count = 0;
            item->data.list = (char **) MALLOC(g_max_items_list * sizeof(char *));
            item->desc = &g_menu_desc[i];
            item->name = MALLOC(MAX_CHARS_MENU_NAME);
            strcpy(item->name, g_menu_desc[i].name);

            node_t *node;
            node = node_child(parent, item);

            if (item->desc->type == MENU_LIST || item->desc->type == MENU_SELECT)
                create_menu_tree(node, &g_menu_desc[i]);
        }
    }
}

static void reset_menu_hover(node_t *menu_node)
{
    node_t *node;
    for (node = menu_node->first_child; node; node = node->next)
    {
        menu_item_t *item = node->data;
        if (item->desc->type == MENU_LIST || item->desc->type == MENU_SELECT) item->data.hover = 0;
        reset_menu_hover(node);
    }
}

static uint8_t bank_config_check(uint8_t foot)
{
    uint8_t i;

    for (i = 1; i < BANK_FUNC_AMOUNT; i++)
    {
        if (g_bank_functions[i].actuator_id == foot &&
            g_bank_functions[i].function != BANK_FUNC_NONE)
        {
            return i;
        }
    }

    return 0;
}

static void bank_config_update(uint8_t bank_func_idx)
{
    uint8_t i = bank_func_idx, current_pedalboard;

    current_pedalboard = g_current_pedalboard;

    switch (g_bank_functions[i].function)
    {
        case BANK_FUNC_TRUE_BYPASS:
            // TODO: toggle true bypass
            break;

        case BANK_FUNC_PEDALBOARD_NEXT:
            if (g_selected_pedalboards)
            {
                g_current_pedalboard++;

                if (g_current_pedalboard == g_selected_pedalboards->count)
                {
                    // if previous pedalboard function is not being used, does circular selection
                    // the minimum value is 1 because the option 0 is 'back to banks list'
                    if (g_bank_functions[BANK_FUNC_PEDALBOARD_PREV].function == BANK_FUNC_NONE) g_current_pedalboard = 1;
                    // if previous pedalboard function is being used, stops on maximum value
                    else g_current_pedalboard = g_selected_pedalboards->count - 1;
                }

                g_selected_pedalboards->selected = g_current_pedalboard;

                if (current_pedalboard != g_current_pedalboard)
                    send_load_pedalboard(g_current_bank - 1, g_selected_pedalboards->uids[g_selected_pedalboards->selected]);
            }
            break;

        case BANK_FUNC_PEDALBOARD_PREV:
            if (g_selected_pedalboards)
            {
                g_current_pedalboard--;

                if (g_current_pedalboard == 0)
                {
                    // if next pedalboard function is not being used, does circular selection
                    if (g_bank_functions[BANK_FUNC_PEDALBOARD_NEXT].function == BANK_FUNC_NONE)
                        g_current_pedalboard = g_selected_pedalboards->count - 1;
                    // if next pedalboard function is being used, stops on minimum value
                    // the minimum value is 1 because the option 0 is 'back to banks list'
                    else g_current_pedalboard = 1;
                }

                g_selected_pedalboards->selected = g_current_pedalboard;

                if (current_pedalboard != g_current_pedalboard)
                    send_load_pedalboard(g_current_bank - 1, g_selected_pedalboards->uids[g_selected_pedalboards->selected]);
            }
            break;
    }

    // updates the footers
    bank_config_footer();

    // updates the navigation menu if the current pedalboards list
    // is the same assigned to foot pedalboards navigation (bank config)
    if (g_selected_pedalboards && g_current_bank == g_banks->hover &&
        tool_is_on(DISPLAY_TOOL_NAVIG) && g_bp_state == PEDALBOARD_LIST)
    {
        g_naveg_pedalboards->selected = g_selected_pedalboards->selected;
        g_naveg_pedalboards->hover = g_selected_pedalboards->selected;
        screen_bp_list(g_banks->names[g_banks->hover], g_naveg_pedalboards);
    }
}

static void bank_config_footer(void)
{
    uint8_t bypass;
    char *pedalboard_name = g_selected_pedalboards ? g_selected_pedalboards->names[g_selected_pedalboards->selected] : NULL;

    // updates all footer screen with bank functions
    uint8_t i;
    for (i = 1; i < BANK_FUNC_AMOUNT; i++)
    {
        bank_config_t *bank_conf;
        bank_conf = &g_bank_functions[i];
        color_t color;

        switch (bank_conf->function)
        {
            case BANK_FUNC_TRUE_BYPASS:
                bypass = 0; // FIX: get true bypass state
                led_set_color(hardware_leds(bank_conf->actuator_id), bypass ? BLACK : TRUE_BYPASS_COLOR);

                if (display_has_tool_enabled(bank_conf->actuator_id)) break;
                screen_footer(bank_conf->actuator_id, TRUE_BYPASS_FOOTER_TEXT,
                            (bypass ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT));
                break;

            case BANK_FUNC_PEDALBOARD_NEXT:
                if (g_current_pedalboard == (g_selected_pedalboards->count - 1)) color = BLACK;
                else color = PEDALBOARD_NEXT_COLOR;

                led_set_color(hardware_leds(bank_conf->actuator_id), color);

                if (display_has_tool_enabled(bank_conf->actuator_id)) break;
                screen_footer(bank_conf->actuator_id, pedalboard_name, PEDALBOARD_NEXT_FOOTER_TEXT);
                break;

            case BANK_FUNC_PEDALBOARD_PREV:
                if (g_current_pedalboard == 1) color = BLACK;
                else color = PEDALBOARD_PREV_COLOR;

                led_set_color(hardware_leds(bank_conf->actuator_id), color);

                if (display_has_tool_enabled(bank_conf->actuator_id)) break;
                screen_footer(bank_conf->actuator_id, pedalboard_name, PEDALBOARD_PREV_FOOTER_TEXT);
                break;
        }
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void naveg_init(void)
{
    uint32_t i;

    // initialize the global variables
    for (i = 0; i < SLOTS_COUNT; i++)
    {
        // initialize the display controls pointers
        g_controls[i] = NULL;

        // initialize the foot controls pointers
        g_foots[i] = NULL;

        // initialize the tap tempo
        g_tap_tempo[i].state = TT_INIT;
    }

    g_banks = NULL;
    g_naveg_pedalboards = NULL;
    g_selected_pedalboards = NULL;
    g_current_pedalboard = 1;
    g_bp_state = BANKS_LIST;

    // initializes the bank functions
    for (i = 0; i < BANK_FUNC_AMOUNT; i++)
    {
        g_bank_functions[i].function = BANK_FUNC_NONE;
        g_bank_functions[i].hardware_type = 0xFF;
        g_bank_functions[i].hardware_id = 0xFF;
        g_bank_functions[i].actuator_type = 0xFF;
        g_bank_functions[i].actuator_id = 0xFF;
    }

    // counts the maximum items amount in menu lists
    uint8_t count = 0;
    for (i = 0; g_menu_desc[i].name; i++)
    {
        uint8_t j = i + 1;
        for (; g_menu_desc[j].name; j++)
        {
            if (g_menu_desc[i].id == g_menu_desc[j].parent_id) count++;
        }

        if (count > g_max_items_list) g_max_items_list = count;
        count = 0;
    }

    // adds one to line 'back to previous menu'
    g_max_items_list++;

    // creates the menu tree (recursively)
    const menu_desc_t root_desc = {"root", MENU_LIST, -1, -1, NULL, 0};
    g_menu = node_create(NULL);
    create_menu_tree(g_menu, &root_desc);

    // sets current menu
    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;

    // initialize update variables
    g_update_cb = NULL;
    g_update_data = NULL;

    g_initialized = 1;

    vSemaphoreCreateBinary(g_dialog_sem);
    xSemaphoreTake(g_dialog_sem, 0);
}

void naveg_initial_state(char *bank_uid, char *pedalboard_uid, char **pedalboards_list)
{
    if (!pedalboards_list)
    {
        if (g_banks)
        {
            g_banks->selected = 0;
            g_banks->hover = 0;
            g_current_bank = 0;
        }

        if (g_naveg_pedalboards)
        {
            g_naveg_pedalboards->selected = 0;
            g_naveg_pedalboards->hover = 0;
            g_current_pedalboard = 0;
        }

        return;
    }

    // sets the bank index
    uint8_t bank_id = atoi(bank_uid) + 1;
    g_current_bank = bank_id;
    if (g_banks)
    {
        g_banks->selected = bank_id;
        g_banks->hover = bank_id;
    }

    // checks and free the navigation pedalboads list
    if (g_naveg_pedalboards) data_free_pedalboards_list(g_naveg_pedalboards);
    if (g_selected_pedalboards) data_free_pedalboards_list(g_selected_pedalboards);

    // parses the list
    g_selected_pedalboards = NULL;
    g_naveg_pedalboards = data_parse_pedalboards_list(pedalboards_list, strarr_length(pedalboards_list));

    if (!g_naveg_pedalboards) return;

    // locates the selected pedalboard index
    uint8_t i;
    for (i = 0; i < g_naveg_pedalboards->count; i++)
    {
        if (strcmp(g_naveg_pedalboards->uids[i], pedalboard_uid) == 0)
        {
            g_naveg_pedalboards->selected = i;
            g_naveg_pedalboards->hover = i;
            g_current_pedalboard = i;
            break;
        }
    }
}

void naveg_ui_connection(uint8_t status)
{
    if (!g_initialized) return;

    if (status == UI_CONNECTED)
    {
        g_ui_connected = 1;
    }
    else
    {
        g_ui_connected = 0;

        // reset the banks and pedalboards state after return from ui connection
        if (g_banks) data_free_banks_list(g_banks);
        if (g_naveg_pedalboards) data_free_pedalboards_list(g_naveg_pedalboards);
        g_banks = NULL;
        g_naveg_pedalboards = NULL;
        g_selected_pedalboards = NULL;
    }


    if (tool_is_on(DISPLAY_TOOL_NAVIG))
        naveg_toggle_tool(DISPLAY_TOOL_NAVIG, 0);
}

void naveg_add_control(control_t *control)
{
    if (!g_initialized) return;
    if (!control) return;

    // first tries remove the control
    naveg_remove_control(control->effect_instance, control->symbol);

    switch (control->actuator_type)
    {
        case KNOB:
            display_control_add(control);
            break;

        case FOOT:
            foot_control_add(control);
            break;
    }
}

void naveg_remove_control(int32_t effect_instance, const char *symbol)
{
    if (!g_initialized) return;

    display_control_rm(effect_instance, symbol);
    foot_control_rm(effect_instance, symbol);
}

void naveg_inc_control(uint8_t display)
{
    if (!g_initialized) return;

    // if is in tool mode return
    if (display_has_tool_enabled(display)) return;

    control_t *control = g_controls[display];
    if (!control) return;

    // increments the step
    control->step++;
    if (control->step >= control->steps) control->step = control->steps - 1;

    // converts the step to absolute value
    step_to_value(control);

    // applies the control value
    control_set(display, control);
}

void naveg_dec_control(uint8_t display)
{
    if (!g_initialized) return;

    // if is in tool mode return
    if (display_has_tool_enabled(display)) return;

    control_t *control = g_controls[display];
    if (!control) return;

    // decrements the step
    control->step--;
    if (control->step < 0) control->step = 0;

    // converts the step to absolute value
    step_to_value(control);

    // applies the control value
    control_set(display, control);
}

void naveg_set_control(int32_t effect_instance, const char *symbol, float value)
{
    if (!g_initialized) return;

    uint8_t display;
    control_t *control;

    control = search_control(effect_instance, symbol, &display);

    if (control)
    {
        control->value = value;
        if (value < control->minimum) control->value = control->minimum;
        if (value > control->maximum) control->value = control->maximum;
        control_set(display, control);

        // updates the step value
        control->step =
            (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);
    }
}

float naveg_get_control(int32_t effect_instance, const char *symbol)
{
    if (!g_initialized) return 0.0;

    uint8_t display;
    control_t *control;

    control = search_control(effect_instance, symbol, &display);
    if (control) return control->value;

    return 0.0;
}

void naveg_next_control(uint8_t display)
{
    if (!g_initialized) return;

    // if is in tool mode return
    if (display_has_tool_enabled(display)) return;

    char buffer[128];
    uint8_t i;

    i = copy_command(buffer, CONTROL_NEXT_CMD);

    // FIXME: hardware type and hardware id must be deprecated

    // inserts the hardware type
    i += int_to_str(0, &buffer[i], 4, 0);
    buffer[i++] = ' ';

    // inserts the hardware id
    i += int_to_str(0, &buffer[i], 4, 0);
    buffer[i++] = ' ';

    // inserts the actuator type
    i += int_to_str(KNOB, &buffer[i], 4, 0);
    buffer[i++] = ' ';

    // inserts the actuator id
    i += int_to_str(display, &buffer[i], 4, 0);
    buffer[i] = 0;

    comm_webgui_send(buffer, i);
}

void naveg_foot_change(uint8_t foot)
{
    if (!g_initialized) return;

    // checks the foot id
    if (foot >= FOOTSWITCHES_COUNT) return;

    // checks if the foot is used like bank function
    uint8_t bank_func_idx = bank_config_check(foot);
    if (bank_func_idx)
    {
        bank_config_update(bank_func_idx);
        return;
    }

    // checks if there is assigned control
    if (g_foots[foot] == NULL) return;

    // send the foot value
    control_set(foot, g_foots[foot]);
}

void naveg_toggle_tool(uint8_t tool, uint8_t display)
{
    if (!g_initialized) return;

    // clears the display
    screen_clear(display);

    // changes the display to tool mode
    if (!tool_is_on(tool))
    {
        // action to do when the tool is enabled
        switch (tool)
        {
            case DISPLAY_TOOL_NAVIG:
                // initial state to banks/pedalboards navigation
                request_banks_list();
                break;

            case DISPLAY_TOOL_TUNER:
                comm_webgui_send(TUNER_ON_CMD, strlen(TUNER_ON_CMD));
                break;
        }

        // draws the tool
        tool_on(tool, display);
        screen_tool(tool, display);
    }
    // changes the display to control mode
    else
    {
        display_disable_all_tools(display);

        // action to do when the tool is disabled
        switch (tool)
        {
            case DISPLAY_TOOL_SYSTEM:
                g_update_cb = NULL;
                g_update_data = NULL;

                // force save gains when leave the menu
                system_save_gains_cb(NULL, MENU_EV_ENTER);
                break;

            case DISPLAY_TOOL_TUNER:
                comm_webgui_send(TUNER_OFF_CMD, strlen(TUNER_OFF_CMD));
                break;
        }

        control_t *control = g_controls[display];

        // draws the control
        screen_control(display, control);

        // draws the controls index
        if (control)
            screen_controls_index(display, control->control_index, control->controls_count);

        // checks the function assigned to foot and update the footer
        if (bank_config_check(display)) bank_config_footer();
        else if (g_foots[display]) foot_control_add(g_foots[display]);
        else screen_footer(display, NULL, NULL);
    }
}

uint8_t naveg_is_tool_mode(uint8_t display)
{
    return display_has_tool_enabled(display);
}

void naveg_set_banks(bp_list_t *bp_list)
{
    if (!g_initialized) return;

    g_banks = bp_list;
}

bp_list_t *naveg_get_banks(void)
{
    if (!g_initialized) return NULL;

    return g_banks;
}

void naveg_bank_config(bank_config_t *bank_conf)
{
    if (!g_initialized) return;

    // checks the function number
    if (bank_conf->function >= BANK_FUNC_AMOUNT) return;

    // checks the actuator type and actuator id
    if (bank_conf->actuator_type != FOOT || bank_conf->actuator_id >= FOOTSWITCHES_COUNT) return;

    // TODO: need check hardware_type and hardware_id

    uint8_t i;
    for (i = 1; i < BANK_FUNC_AMOUNT; i++)
    {
        // checks if the function is already assigned to an actuator
        if (bank_conf->function != BANK_FUNC_NONE &&
            bank_conf->function == g_bank_functions[i].function &&
            bank_conf->actuator_id != g_bank_functions[i].actuator_id)
        {
            // updates the screen and led
            led_set_color(hardware_leds(g_bank_functions[i].actuator_id), BLACK);
            if (!display_has_tool_enabled(g_bank_functions[i].actuator_id))
                screen_footer(g_bank_functions[i].actuator_id, NULL, NULL);

            // removes the function
            g_bank_functions[i].function = BANK_FUNC_NONE;
            g_bank_functions[i].hardware_type = 0xFF;
            g_bank_functions[i].hardware_id = 0xFF;
            g_bank_functions[i].actuator_type = 0xFF;
            g_bank_functions[i].actuator_id = 0xFF;
        }

        // checks if is replacing a function
        if ((g_bank_functions[i].function != bank_conf->function) &&
            (g_bank_functions[i].actuator_type == bank_conf->actuator_type) &&
            (g_bank_functions[i].actuator_id == bank_conf->actuator_id))
        {
            // removes the function
            g_bank_functions[i].function = BANK_FUNC_NONE;
            g_bank_functions[i].hardware_type = 0xFF;
            g_bank_functions[i].hardware_id = 0xFF;
            g_bank_functions[i].actuator_type = 0xFF;
            g_bank_functions[i].actuator_id = 0xFF;

            // if the new function is none, updates the screen and led
            if (bank_conf->function == BANK_FUNC_NONE)
            {
                led_set_color(hardware_leds(bank_conf->actuator_id), BLACK);
                if (!display_has_tool_enabled(bank_conf->actuator_id))
                    screen_footer(bank_conf->actuator_id, NULL, NULL);

                // checks if has control assigned in this foot
                // if yes, updates the footer screen
                if (g_foots[bank_conf->actuator_id])
                    foot_control_add(g_foots[bank_conf->actuator_id]);
            }
        }
    }

    // copies the bank function struct
    if (bank_conf->function != BANK_FUNC_NONE)
        memcpy(&g_bank_functions[bank_conf->function], bank_conf, sizeof(bank_config_t));

    // checks if has pedalboards navigation functions and set the pointer to pedalboards list
    if (bank_conf->function == BANK_FUNC_PEDALBOARD_NEXT ||
        bank_conf->function == BANK_FUNC_PEDALBOARD_PREV)
    {
        g_selected_pedalboards = g_naveg_pedalboards;
    }

    bank_config_footer();
}

void naveg_set_pedalboards(bp_list_t *bp_list)
{
    if (!g_initialized) return;

    g_naveg_pedalboards = bp_list;
}

bp_list_t *naveg_get_pedalboards(void)
{
    if (!g_initialized) return NULL;

    return g_naveg_pedalboards;
}

void naveg_enter(uint8_t display)
{
    if (!g_initialized) return;

    if (display_has_tool_enabled(display))
    {
        if (display == 0)
        {
            if (tool_is_on(DISPLAY_TOOL_NAVIG)) bp_enter();
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM)) menu_enter();
        }
        else if (display == 1)
        {
            if (tool_is_on(DISPLAY_TOOL_TUNER)) tuner_enter();
        }
    }
}

void naveg_up(uint8_t display)
{
    if (!g_initialized) return;

    if (display_has_tool_enabled(display))
    {
        if (display == 0)
        {
            if (tool_is_on(DISPLAY_TOOL_NAVIG)) bp_up();
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM)) menu_up();
        }
    }
}

void naveg_down(uint8_t display)
{
    if (!g_initialized) return;

    if (display_has_tool_enabled(display))
    {
        if (display == 0)
        {
            if (tool_is_on(DISPLAY_TOOL_NAVIG)) bp_down();
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM)) menu_down();
        }
    }
}

void naveg_reset_menu(void)
{
    if (!g_initialized) return;

    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;
    reset_menu_hover(g_menu);
}

int naveg_need_update(void)
{
    return (g_update_cb ? 1: 0);
}

void naveg_update(void)
{
    if (g_update_cb)
    {
        (*g_update_cb)(g_update_data, MENU_EV_ENTER);
        screen_system_menu(g_update_data);
    }
}

uint8_t naveg_dialog(const char *msg)
{
    static node_t *dummy_menu;
    static menu_desc_t desc = {NULL, MENU_CONFIRM2, -2, -2, NULL, 0};

    if (!dummy_menu)
    {
        menu_item_t *item;
        item = (menu_item_t *) MALLOC(sizeof(menu_item_t));
        item->data.hover = 0;
        item->data.selected = 0xFF;
        item->data.list_count = 2;
        item->data.list = NULL;
        item->data.popup_content = msg;
        item->desc = &desc;
        item->name = NULL;
        dummy_menu = node_create(item);
    }

    tool_on(DISPLAY_TOOL_SYSTEM, 0);
    g_current_menu = dummy_menu;
    g_current_item = dummy_menu->data;
    screen_system_menu(g_current_item);

    xSemaphoreTake(g_dialog_sem, portMAX_DELAY);

    naveg_toggle_tool(DISPLAY_TOOL_SYSTEM, DISPLAY_TOOL_SYSTEM);
    return g_current_item->data.hover;
}

uint8_t naveg_ui_status(void)
{
    return g_ui_connected;
}
