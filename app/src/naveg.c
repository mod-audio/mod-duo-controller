
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

#include <string.h>
#include <math.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {TT_INIT, TT_COUNTING};
enum {TOOL_OFF, TOOL_ON};
enum {BANKS_LIST, PEDALBOARD_LIST};

#define MAX_CHARS_MENU_NAME     (128/4)


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static const uint8_t g_tools_display[] = {
    TOOL_DISPLAY0,
    TOOL_DISPLAY1,
    TOOL_DISPLAY2,
    TOOL_DISPLAY3
};

static const menu_desc_t g_menu_desc[] = {
    SYSTEM_MENU
    {NULL, 0, -1, -1, NULL}
};


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

struct CONTROL_INDEX_T {
    uint8_t current, total;
} g_controls_index[SLOTS_COUNT];

struct TAP_TEMPO_T {
    uint32_t time, max;
    uint8_t state;
} g_tap_tempo[SLOTS_COUNT];

struct TOOL_T {
    node_t *node;
    uint8_t state;
} g_tool[SLOTS_COUNT];


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

static node_t *g_controls_list[SLOTS_COUNT], *g_current_control[SLOTS_COUNT];
static control_t *g_foots[SLOTS_COUNT];
static bp_list_t *g_banks, *g_pedalboards;
static uint8_t g_bp_state, g_current_pedalboard;
static node_t *g_menu, *g_current_menu;
menu_item_t *g_current_item;
uint8_t g_max_items_list;


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static void display_control_add(control_t *control);
static void display_control_rm(uint8_t effect_instance, const char *symbol);


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

// search for the control
static node_t *search_control(uint8_t effect_instance, const char *symbol, uint8_t *display)
{
    uint8_t i;
    node_t *node;
    control_t *control;

    for (i = 0; i < SLOTS_COUNT; i++)
    {
        for (node = g_controls_list[i]->first_child; node; node = node->next)
        {
            control = (control_t *) node->data;
            if (control->effect_instance == effect_instance)
            {
                if (strcmp(control->symbol, symbol) == 0)
                {
                    (*display) = i;
                    return node;
                }
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
            control->value = (p_step * (control->maximum - control->minimum)) + control->minimum;
            break;

        case CONTROL_PROP_LOGARITHMIC:
            control->value = control->minimum * pow(control->maximum / control->minimum, p_step);
            break;

        case CONTROL_PROP_ENUMERATION:
            control->value = control->scale_points[control->step]->value;
            break;
    }
}

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
    node_t *node;

    // checks if control is already in controls list
    node = search_control(control->effect_instance, control->symbol, &display);
    if (node)
    {
        if (display != control->actuator_id)
        {
            display_control_rm(control->effect_instance, control->symbol);
        }
        else
        {
            data_free_control(control);
            return;
        }
    }

    display = control->actuator_id;

    // adds the control to controls list
    node = node_child(g_controls_list[display], control);

    // makes the node the current control
    g_current_control[display] = node;

    // connect the control with the tool
    node->first_child = g_tool[display].node;

    // calculates initial step
    switch (control->properties)
    {
        case CONTROL_PROP_LINEAR:
            control->step =
                (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);
            break;

        case CONTROL_PROP_LOGARITHMIC:
            control->step =
                (control->steps - 1) * log(control->value / control->minimum) / log(control->maximum / control->minimum);
            break;

        case CONTROL_PROP_ENUMERATION:
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
    }

    // update the controls index value
    g_controls_index[display].total++;
    g_controls_index[display].current = g_controls_index[display].total;

    // if tool is enabled don't draws the control
    if (g_tool[display].state == TOOL_ON) return;

    // update the control screen
    screen_control(display, control);

    // update the controls index screen
    screen_controls_index(display, g_controls_index[display].current, g_controls_index[display].total);
}

// control removed from display
static void display_control_rm(uint8_t effect_instance, const char *symbol)
{
    uint8_t display;
    control_t *control;
    node_t *node;

    node = search_control(effect_instance, symbol, &display);

    // destroy the control
    if (node)
    {
        control = (control_t *) node->data;

        // update the controls index value
        g_controls_index[display].total--;

        // if node is the current control
        if (node == g_current_control[display])
        {
            // if is the last control
            if (g_controls_index[display].total == 0)
            {
                // no controls
                g_current_control[display] = NULL;
            }

            // load the next control
            naveg_next_control(display);
        }

        // update the controls index screen
        if (g_controls_index[display].current > g_controls_index[display].total)
        {
            g_controls_index[display].current = g_controls_index[display].total;
        }
        screen_controls_index(display, g_controls_index[display].current, g_controls_index[display].total);

        // free the memory
        data_free_control(control);
        node->first_child = NULL;
        node_destroy(node);
    }
}

// control assigned to foot
static void foot_control_add(control_t *control)
{
    if (control->actuator_id >= FOOTSWITCHES_COUNT ||
       (g_foots[control->actuator_id] && g_foots[control->actuator_id] != control)) return;

    // stores the foot
    g_foots[control->actuator_id] = control;

    switch (control->properties)
    {
        // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
        case CONTROL_PROP_TOGGLED:
            if (control->value <= 0)
                led_set_color(hardware_leds(control->actuator_id), BLACK);
            else
                led_set_color(hardware_leds(control->actuator_id), TOGGLED_COLOR);

            screen_footer(control->actuator_id, control->label, (control->value <= 0 ? "OFF" : "ON"));
            break;

        // trigger specification: http://lv2plug.in/ns/ext/port-props/#trigger
        case CONTROL_PROP_TRIGGER:
            led_set_color(hardware_leds(control->actuator_id), TRIGGER_COLOR);
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

            // footer text composition
            uint8_t i;
            char value_txt[32];
            i = int_to_str(control->value, value_txt, sizeof(value_txt), 0);
            value_txt[i++] = ' ';
            strcpy(&value_txt[i], control->unit);
            screen_footer(control->actuator_id, control->label, value_txt);

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
            break;

        case CONTROL_PROP_BYPASS:
            if (control->value <= 0)
                led_set_color(hardware_leds(control->actuator_id), BLACK);
            else
                led_set_color(hardware_leds(control->actuator_id), BYPASS_COLOR);
            screen_footer(control->actuator_id, control->label, (control->value ? "BYP" : "PROC"));
            break;
    }
}

// control removed from foot
static void foot_control_rm(uint8_t effect_instance, const char *symbol)
{
    uint8_t i;

    for (i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        if (g_foots[i] == NULL) continue;

        // checks if effect_instance and symbol match
        if ((effect_instance == g_foots[i]->effect_instance) &&
            (strcmp(symbol, g_foots[i]->symbol) == 0))
        {
            // turn off the led
            led_set_color(hardware_leds(i), BLACK);

            // remove the control
            data_free_control(g_foots[i]);
            g_foots[i] = NULL;

            // update the footer
            screen_footer(i, NULL, NULL);
        }
    }
}

static void control_set(uint8_t display, control_t *control)
{
    char buffer[128];
    uint8_t i;

    i = copy_command(buffer, CONTROL_SET_CMD);
    uint32_t delta, now;

    switch (control->properties)
    {
        case CONTROL_PROP_LINEAR:
        case CONTROL_PROP_LOGARITHMIC:
        case CONTROL_PROP_ENUMERATION:

            // update the screen
            screen_control(display, control);
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
            now = hardware_time_stamp();
            delta = now - g_tap_tempo[control->actuator_id].time;
            g_tap_tempo[control->actuator_id].time = now;

            if (g_tap_tempo[control->actuator_id].state == TT_COUNTING)
            {
                // checks the tap tempo timeout
                if (delta < g_tap_tempo[control->actuator_id].max)
                {
                    // converts and update the tap tempo value
                    control->value = convert_from_ms(control->unit, delta);
                    foot_control_add(control);
                }
            }
            break;
    }

    // insert the instance on buffer
    i += int_to_str(control->effect_instance, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i++] = ' ';

    // insert the symbol on buffer
    strcpy(&buffer[i], control->symbol);
    i += strlen(control->symbol);
    buffer[i++] = ' ';

    // insert the value on buffer
    i += float_to_str(control->value, &buffer[i], sizeof(buffer) - i, 3);

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
    naveg_set_banks(g_banks);
}

static void request_banks_list(void)
{
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

    // free the current pedalboads list
    if (g_pedalboards) data_free_pedalboards_list(g_pedalboards);

    // parses the list
    g_pedalboards = data_parse_pedalboards_list(&list[2], count);
    naveg_set_pedalboards(g_pedalboards);
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

    // sets the response callback
    comm_webgui_set_response_cb(parse_pedalboards_list);

    // sends the data to GUI
    comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    comm_webgui_wait_response();
}

static void send_load_pedalboard(const char *pedalboard_uid)
{
    uint8_t i;
    char buffer[128];

    i = copy_command((char *)buffer, PEDALBOARD_CMD);

    // copy the pedalboard uid
    const char *p = pedalboard_uid;
    while (*p)
    {
        buffer[i++] = *p;
        p++;
    }

    // send the data to GUI
    comm_webgui_send(buffer, i);
}

static void bp_enter(void)
{
    bp_list_t *bp_list;
    const char *title;

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
        request_pedalboards_list(g_banks->uids[g_banks->hover]);
        g_bp_state = PEDALBOARD_LIST;
        g_pedalboards->hover = 0;
        bp_list = g_pedalboards;
        title = g_banks->names[g_banks->hover];

        // sets the selected pedalboard out of range (initial state)
        g_pedalboards->selected = g_pedalboards->count;

        // defines the selected pedalboard
        if (g_banks->selected == g_banks->hover)
        {
            g_pedalboards->selected = g_current_pedalboard;
        }
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        // checks if is the first option (back to banks list)
        if (g_pedalboards->hover == 0)
        {
            g_bp_state = BANKS_LIST;
            bp_list = g_banks;
            title = "BANKS";
        }
        else
        {
            g_banks->selected = g_banks->hover;
            g_pedalboards->selected = g_pedalboards->hover;
            g_current_pedalboard = g_pedalboards->selected;
            bp_list = g_pedalboards;
            title = g_banks->names[g_banks->hover];
            send_load_pedalboard(g_pedalboards->uids[g_pedalboards->selected]);
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
        if (g_pedalboards->hover > 0) g_pedalboards->hover--;
        bp_list = g_pedalboards;
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
        if (g_pedalboards->hover < (g_pedalboards->count - 1)) g_pedalboards->hover++;
        bp_list = g_pedalboards;
        title = g_banks->names[g_banks->hover];
    }

    screen_bp_list(title, bp_list);
}

static void menu_enter(void)
{
    uint8_t i;
    node_t *node = g_current_menu;
    menu_item_t *item;

    // checks the current item
    if (g_current_item->desc->type == MENU_LIST)
    {
        // locates the clicked item
        node = g_current_menu->first_child;
        for (i = 0; i < g_current_item->data.hover; i++) node = node->next;

        // gets the menu item
        item = node->data;

        // checks if is 'back to previous'
        if (item->desc->type == MENU_RETURN)
        {
            node = node->parent->parent;
            item = node->data;
        }

        // updates the current item
        if (item->desc->type != MENU_ON_OFF && item->desc->type != MENU_NONE) g_current_item = node->data;
    }
    else if (g_current_item->desc->type == MENU_CONFIRM || g_current_item->desc->type == MENU_CANCEL)
    {
        // gets the menu item
        item = g_current_menu->data;
        g_current_item = item;
    }

    // checks the selected item
    if (item->desc->type == MENU_LIST)
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

            // checks if is ON / OFF type and insert the value
            if (item_child->desc->type == MENU_ON_OFF)
            {
                strcpy(item_child->name, item_child->desc->name);
                strcat(item_child->name, (item_child->data.hover ? " ON" : "OFF"));
            }
        }
    }
    else if (item->desc->type == MENU_CONFIRM)
    {
        // highlights the default button
        item->data.hover = 1;

        // defines the buttons count
        item->data.list_count = 2;
    }
    else if (item->desc->type == MENU_CANCEL)
    {
        // highlights the default button
        item->data.hover = 0;

        // defines the buttons count
        item->data.list_count = 1;
    }
    else if (item->desc->type == MENU_ON_OFF)
    {
        item->data.hover = 1 - item->data.hover;

        // calls the action callback
        if (item->desc->action_cb)
            item->desc->action_cb(item);

        // keeps the current item
        //item = g_current_item;
    }
    else if (item->desc->type == MENU_NONE)
    {
        // keeps the current item
        //item = g_current_item;
    }

    screen_system_menu(item);
}

static void menu_up(void)
{
    menu_item_t *item = g_current_item;
    if (item->data.hover > 0) item->data.hover--;
    screen_system_menu(item);
}

static void menu_down(void)
{
    menu_item_t *item = g_current_item;
    if (item->data.hover < (item->data.list_count - 1)) item->data.hover++;
    screen_system_menu(item);
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

            if (item->desc->type == MENU_LIST) create_menu_tree(node, &g_menu_desc[i]);
        }
    }
}

static void reset_menu_hover(node_t *menu_node)
{
    node_t *node;
    for (node = menu_node->first_child; node; node = node->next)
    {
        menu_item_t *item = node->data;
        if (item->desc->type == MENU_LIST) item->data.hover = 0;
        reset_menu_hover(node);
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

    // create the nodes
    for (i = 0; i < SLOTS_COUNT; i++)
    {
        // creates the controls list
        g_controls_list[i] = node_create(NULL);

        // initialize the tools
        g_tool[i].state = TOOL_OFF;
        g_tool[i].node = node_create(NULL);

        // initialize the current control
        g_current_control[i] = NULL;

        // initialize the controls index
        g_controls_index[i].current = 0;
        g_controls_index[i].total = 0;

        // initialize the control and footer screen
        screen_control(i, NULL);
        screen_footer(i, NULL, NULL);

        // initialize the foot controls pointers
        g_foots[i] = NULL;

        // initialize the tap tempo
        g_tap_tempo[i].state = TT_INIT;
    }

    g_banks = NULL;
    g_pedalboards = NULL;
    g_current_pedalboard = 1;

    // counts the maximum items amount in a list
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
    const menu_desc_t root_desc = {"root", MENU_LIST, -1, -1, NULL};
    g_menu = node_create(NULL);
    create_menu_tree(g_menu, &root_desc);

    // sets the current menu
    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;
}

void naveg_add_control(control_t *control)
{
    switch (control->actuator_type)
    {
        case KNOB:
            display_control_add(control);
            break;

        case FOOT:
            foot_control_add(control);
            break;

        case PEDAL:
            break;
    }
}

void naveg_remove_control(uint8_t effect_instance, const char *symbol)
{
    display_control_rm(effect_instance, symbol);
    foot_control_rm(effect_instance, symbol);
}

void naveg_inc_control(uint8_t display)
{
    node_t *node = g_current_control[display];
    if (!node) return;

    // if is in tool mode return
    if (g_tool[display].state == TOOL_ON) return;

    control_t *control = node->data;
    control->step++;
    if (control->step >= control->steps) control->step = control->steps - 1;

    // converts the step to absolute value
    step_to_value(control);

    control_set(display, control);
}

void naveg_dec_control(uint8_t display)
{
    node_t *node = g_current_control[display];
    if (!node) return;

    // if is in tool mode return
    if (g_tool[display].state == TOOL_ON) return;

    control_t *control = node->data;
    control->step--;
    if (control->step < 0) control->step = 0;

    // converts the step to absolute value
    step_to_value(control);

    control_set(display, control);
}

void naveg_set_control(uint8_t effect_instance, const char *symbol, float value)
{
    uint8_t display;
    node_t *node;
    control_t *control;

    node = search_control(effect_instance, symbol, &display);

    if (node)
    {
        control = (control_t *) node->data;
        control->value = value;
        if (value < control->minimum) control->value = control->minimum;
        if (value > control->maximum) control->value = control->maximum;
        control_set(display, control);

        control->step =
            (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);
    }
}

float naveg_get_control(uint8_t effect_instance, const char *symbol)
{
    uint8_t display;
    node_t *node;
    control_t *control;

    node = search_control(effect_instance, symbol, &display);

    if (node)
    {
        control = (control_t *) node->data;
        return control->value;
    }

    return 0.0;
}

control_t *naveg_next_control(uint8_t display)
{
    // if is in tool mode return
    if (g_tool[display].state == TOOL_ON) return NULL;

    // if there is no controls
    if (!g_current_control[display])
    {
        screen_control(display, NULL);
        return NULL;
    }

    // get the next control
    control_t *control;
    if (g_current_control[display]->next)
    {
        control = (control_t *) g_current_control[display]->next->data;
        g_current_control[display] = g_current_control[display]->next;
    }
    else
    {
        control = (control_t *) g_controls_list[display]->first_child->data;
        g_current_control[display] = g_controls_list[display]->first_child;
    }

    // update the control screen
    screen_control(display, control);

    // update the controls index
    g_controls_index[display].current++;
    if (g_controls_index[display].current > g_controls_index[display].total) g_controls_index[display].current = 1;
    screen_controls_index(display, g_controls_index[display].current, g_controls_index[display].total);

    return control;
}

void naveg_foot_change(uint8_t foot)
{
    if (foot >= FOOTSWITCHES_COUNT || g_foots[foot] == NULL) return;

    // send the foot value
    control_set(foot, g_foots[foot]);
}

void naveg_toggle_tool(uint8_t display)
{
    // clears the display
    screen_clear(display);

    // changes the display to tool mode
    if (g_tool[display].state == TOOL_OFF)
    {
        // initial state to banks/pedalboards navigation
        g_bp_state = BANKS_LIST;

        // requests the banks list
        if (display == NAVEG_DISPLAY) request_banks_list();

        // draws the tool
        g_tool[display].state = TOOL_ON;
        screen_tool(display, g_tools_display[display]);
    }
    // changes the display to control mode
    else
    {
        g_tool[display].state = TOOL_OFF;

        // checks if there is controls assigned
        control_t *control = NULL;
        if (g_current_control[display]) control = g_current_control[display]->data;

        // draws the control
        screen_control(display, control);

        // draws the controls index
        if (control)
            screen_controls_index(display, g_controls_index[display].current, g_controls_index[display].total);

        // draws the footer
        if (g_foots[display])
            foot_control_add(g_foots[display]);
        else
            screen_footer(display, NULL, NULL);
    }
}

uint8_t naveg_is_tool_mode(uint8_t display)
{
    return g_tool[display].state;
}

void naveg_set_banks(bp_list_t *bp_list)
{
    g_banks = bp_list;
}

bp_list_t *naveg_get_banks(void)
{
    return g_banks;
}

void naveg_set_pedalboards(bp_list_t *bp_list)
{
    g_pedalboards = bp_list;
}

bp_list_t *naveg_get_pedalboards(void)
{
    return g_pedalboards;
}

void naveg_enter(uint8_t display)
{
    if (g_tool[NAVEG_DISPLAY].state == TOOL_ON && display == NAVEG_DISPLAY) bp_enter();
    if (g_tool[SYSTEM_DISPLAY].state == TOOL_ON && display == SYSTEM_DISPLAY) menu_enter();
}

void naveg_up(uint8_t display)
{
    if (g_tool[NAVEG_DISPLAY].state == TOOL_ON && display == NAVEG_DISPLAY) bp_up();
    if (g_tool[SYSTEM_DISPLAY].state == TOOL_ON && display == SYSTEM_DISPLAY) menu_up();
}

void naveg_down(uint8_t display)
{
    if (g_tool[NAVEG_DISPLAY].state == TOOL_ON && display == NAVEG_DISPLAY) bp_down();
    if (g_tool[SYSTEM_DISPLAY].state == TOOL_ON && display == SYSTEM_DISPLAY) menu_down();
}

void naveg_reset_menu(void)
{
    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;
    reset_menu_hover(g_menu);
}
