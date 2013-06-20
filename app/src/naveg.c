
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
#include "serial.h"
#include "led.h"
#include "hardware.h"

#include <string.h>
#include <math.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {TT_INIT, TT_COUNTING};
enum {TOOL_OFF, TOOL_ON};


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

node_t *g_controls_list[SLOTS_COUNT], *g_current_control[SLOTS_COUNT];
control_t *g_foots[SLOTS_COUNT];


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

// control assigned to display
static void display_control_add(control_t *control)
{
    uint8_t display = control->actuator_id;

    // adds the control to controls list
    node_t *node = node_child(g_controls_list[display], control);
    // TODO: test if node is NULL

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

    // update the control screen
    screen_control(display, control);

    // update the controls index
    g_controls_index[display].total++;
    g_controls_index[display].current = g_controls_index[display].total;
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
    const char *cmd = CONTROL_SET_CMD;

    // locate the first token
    uint8_t i = 0;
    while (*cmd != '%')
    {
        buffer[i++] = *cmd;
        cmd++;
    }

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
    buffer[i] = 0;

    // send the data to GUI
    serial_send(SERIAL_WEBGUI, (uint8_t*)buffer, i);

}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void naveg_init(void)
{
    uint32_t i;

    screen_init();

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
    if (g_tool[display].state == TOOL_OFF)
    {
        g_tool[display].state = TOOL_ON;
        screen_tool(display, g_tools_display[display]);
    }
    else
    {
        g_tool[display].state = TOOL_OFF;
        control_t *control = NULL;
        if (g_current_control[display])
        {
            control = g_current_control[display]->data;
            screen_control(display, control);
            foot_control_add(control);
        }
        else
        {
            screen_control(display, NULL);
            screen_footer(display, NULL, NULL);
        }
    }
}

uint8_t naveg_is_tool_mode(uint8_t display)
{
    return g_tool[display].state;
}
