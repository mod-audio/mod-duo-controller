
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
#include "actuator.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <float.h>

//reset actuator queue
void reset_queue(void);

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {TT_INIT, TT_COUNTING};
enum {TOOL_OFF, TOOL_ON};
enum {BANKS_LIST, PEDALBOARD_LIST};

#define MAX_CHARS_MENU_NAME     (128/4)
#define MAX_TOOLS               5

#define DIALOG_MAX_SEM_COUNT   1

#define LIST_PAGE_UP      	 (1 << 1)
#define LIST_WRAP_AROUND	 (1 << 2)
#define LIST_INITIAL_REQ	 (1 << 3)

#define PAGE_DIR_DOWN 		0
#define PAGE_DIR_UP			1
#define PAGE_DIR_INIT		2

#define CONTROL_PAGINATED	1
#define CONTROL_WRAP_AROUND	2
#define CONTROL_END_PAGE	4

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
    {-1, NULL, NULL}
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
#define MAP(x, Omin, Omax, Nmin, Nmax)      ( x - Omin ) * (Nmax -  Nmin)  / (Omax - Omin) + Nmin;

/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static control_t *g_controls[ENCODERS_COUNT], *g_foots[FOOTSWITCHES_COUNT];
static bp_list_t *g_banks, *g_naveg_pedalboards, g_footswitch_pedalboards;
static uint16_t g_bp_state, g_current_pedalboard, g_bp_first, g_pb_footswitches;
static node_t *g_menu, *g_current_menu, *g_current_main_menu;
static menu_item_t *g_current_item, *g_current_main_item;
static uint8_t g_max_items_list;
static bank_config_t g_bank_functions[BANK_FUNC_AMOUNT];
static uint8_t g_initialized, g_ui_connected;
static void (*g_update_cb)(void *data, int event);
static void *g_update_data;
static xSemaphoreHandle g_dialog_sem;
static uint8_t dialog_active = 0;
static int16_t g_current_bank;
static uint8_t g_force_update_pedalboard = 1;

// only enabled after "boot" command received
bool g_should_wait_for_webgui = false;

/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static void display_control_add(control_t *control);
static void display_control_rm(uint8_t hw_id);

static void foot_control_add(control_t *control);
static void foot_control_rm(uint8_t hw_id);

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

static int get_display_by_id(uint8_t id, uint8_t type)
{

    if (type == FOOT)
    {
    	return id;
    }
    //encoder
    return id;
}

static void display_disable_all_tools(uint8_t display)
{
    int i;
    if (tool_is_on(DISPLAY_TOOL_TUNER)) 
        comm_webgui_send(TUNER_OFF_CMD, strlen(TUNER_OFF_CMD));
    
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

void draw_all_foots(uint8_t display)
{
    uint8_t i;
    uint8_t foot = 0;
    if (display == 1)
        foot = 2;

    for (i = 0; i < 2; i++)
    {
        // checks the function assigned to foot and update the footer
        if (g_foots[foot + i])
            foot_control_add(g_foots[foot + i]);
        else
            screen_footer(foot + i, NULL, NULL);
    }
}

// search the control
static control_t *search_control(uint8_t hw_id, uint8_t *display)
{
    uint8_t i;
    control_t *control;

    for (i = 0; i < SLOTS_COUNT; i++)
    {
        control = g_controls[i];
        if (control)
        {
            if (hw_id == control->hw_id)
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

        case CONTROL_PROP_REVERSE_ENUM:
        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
            control->value = control->scale_points[control->step]->value;
            break;
    }

    if (control->value > control->maximum) control->value = control->maximum;
    if (control->value < control->minimum) control->value = control->minimum;
}

// control assigned to display
static void display_control_add(control_t *control)
{
    if (control->hw_id >= ENCODERS_COUNT) return;

    uint8_t display = control->hw_id;

    // checks if is already a control assigned in this display and remove it
    if (g_controls[display])
        data_free_control(g_controls[display]);

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

        case CONTROL_PROP_REVERSE_ENUM:
        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
            control->step = 0;
            uint8_t i;
            control->scroll_dir = 1;
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
}

// control removed from display
static void display_control_rm(uint8_t hw_id)
{
    uint8_t display;
    display = hw_id;

    if (hw_id > ENCODERS_COUNT) return;

    if (!g_controls[display])
    {
        if (!display_has_tool_enabled(display)) screen_control(display, NULL);
        return;
    }

    control_t *control = g_controls[display];

    if (control)
    {
        data_free_control(control);
        g_controls[display] = NULL;
        if (!display_has_tool_enabled(display))
            screen_control(display, NULL);
        return;
    }
    else if (!display_has_tool_enabled(display))
    {
        screen_control(display, NULL);
    }
}

// control assigned to foot
static void foot_control_add(control_t *control)
{
    uint8_t i;

    // checks if the actuator is used like bank function
    if (bank_config_check(control->hw_id - ENCODERS_COUNT))
    {
        data_free_control(control);
        return;
    }

    // checks if the foot is already used by other control and not is state updating
    if (g_foots[control->hw_id - ENCODERS_COUNT] && g_foots[control->hw_id - ENCODERS_COUNT] != control)
    {
        data_free_control(control);
        return;
    }

    // stores the foot
    g_foots[control->hw_id - ENCODERS_COUNT] = control;


    // default state of led blink (no blink)
    led_blink(hardware_leds(control->hw_id), 0, 0);

    switch (control->properties)
    {
        // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
        case CONTROL_PROP_TOGGLED:
            // updates the led
            if (control->value <= 0)
                led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BLACK);
            else
                led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TOGGLED_COLOR);

            // if is in tool mode break
            if (display_has_tool_enabled(control->hw_id)) break;

            // updates the footer
            screen_footer((control->hw_id - ENCODERS_COUNT), control->label,
                         (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT));
            break;

        // trigger specification: http://lv2plug.in/ns/ext/port-props/#trigger
        case CONTROL_PROP_TRIGGER:
            // updates the led
            led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TRIGGER_COLOR);

            // if is in tool mode break
            if (display_has_tool_enabled(control->hw_id - ENCODERS_COUNT)) break;

            // updates the footer
            screen_footer(control->hw_id - ENCODERS_COUNT, control->label, NULL);
            break;

        case CONTROL_PROP_TAP_TEMPO:
            // defines the led color
            led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TAP_TEMPO_COLOR);

            // convert the time unit
            uint16_t time_ms = (uint16_t)(convert_to_ms(control->unit, control->value) + 0.5);

            // setup the led blink
            if (time_ms > TAP_TEMPO_TIME_ON)
                led_blink(hardware_leds(control->hw_id - ENCODERS_COUNT), TAP_TEMPO_TIME_ON, time_ms - TAP_TEMPO_TIME_ON);
            else
                led_blink(hardware_leds(control->hw_id - ENCODERS_COUNT), time_ms / 2, time_ms / 2);

            // calculates the maximum tap tempo value
            if (g_tap_tempo[control->hw_id].state == TT_INIT)
            {
                uint32_t max;

                // time unit (ms, s)
                if (strcmp(control->unit, "ms") == 0 || strcmp(control->unit, "s") == 0)
                {
                    max = (uint32_t)(convert_to_ms(control->unit, control->maximum) + 0.5);
                    //makes sure we enforce a proper timeout
                    if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                        max = TAP_TEMPO_DEFAULT_TIMEOUT;
                }
                // frequency unit (bpm, Hz)
                else
                {
                    //prevent division by 0 case
                    if (control->minimum == 0)
                        max = TAP_TEMPO_DEFAULT_TIMEOUT;
                    else
                        max = (uint32_t)(convert_to_ms(control->unit, control->minimum) + 0.5);
                    //makes sure we enforce a proper timeout
                    if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                        max = TAP_TEMPO_DEFAULT_TIMEOUT;
                }

                g_tap_tempo[control->hw_id - ENCODERS_COUNT].max = max;
                g_tap_tempo[control->hw_id - ENCODERS_COUNT].state = TT_COUNTING;
            }

            // if is in tool mode break
            if (display_has_tool_enabled(control->hw_id - ENCODERS_COUNT)) break;

            // footer text composition
            char value_txt[32];
            //if unit=ms or unit=bpm -> use 0 decimal points
            if (strcasecmp(control->unit, "ms") == 0 || strcasecmp(control->unit, "bpm") == 0)
                i = int_to_str(control->value, value_txt, sizeof(value_txt), 0);
            //if unit=s or unit=hz or unit=something else-> use 2 decimal points
            else 
                i = float_to_str(control->value, value_txt, sizeof(value_txt), 2);
            //add space to footer
            value_txt[i++] = ' ';
            strcpy(&value_txt[i], control->unit);

            // updates the footer
            screen_footer((control->hw_id - ENCODERS_COUNT), control->label, value_txt);
            break;

        case CONTROL_PROP_BYPASS:
            // updates the led
            if (control->value <= 0)
                led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BYPASS_COLOR);
            else
                led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BLACK);

            // if is in tool mode break
            if (display_has_tool_enabled(control->hw_id - ENCODERS_COUNT)) break;

            // updates the footer
            screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                         (control->value ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT));
            break;

        case CONTROL_PROP_REVERSE_ENUM:
        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
            // updates the led
            led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), ENUMERATED_COLOR);

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
            if (display_has_tool_enabled(control->hw_id - ENCODERS_COUNT)) break;

            // updates the footer
            screen_footer((control->hw_id - ENCODERS_COUNT), control->label, control->scale_points[i]->label);
            break;
    }
}

// control removed from foot
static void foot_control_rm(uint8_t hw_id)
{
    uint8_t i;

    if (hw_id < ENCODERS_COUNT) return;

    for (i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        // if there is no controls assigned, load the default screen
        if (!g_foots[i] && ! bank_config_check(i) && !display_has_tool_enabled(i))
        {
            screen_footer(i, NULL, NULL);
            continue;
        }

        // checks if effect_instance and symbol match
        if (hw_id == g_foots[i]->hw_id)
        {
            // remove the control
            data_free_control(g_foots[i]);
            g_foots[i] = NULL;

            // check if foot isn't being used to bank function
            if (! bank_config_check(i))
            {
                // default state of led blink (no blink)
                led_blink(hardware_leds(i), 0, 0);

                // turn off the led
                led_set_color(hardware_leds(i), BLACK);

                // update the footer
                if (!display_has_tool_enabled(i))
                    screen_footer(i, NULL, NULL);
            }    
        }
    }
}

static void parse_control_page(void *data, menu_item_t *item)
{

	(void) item;
	char **list = data;

    control_t *control = data_parse_control(&list[1]);

    naveg_add_control(control);

    //if encoder, redraw the index
    if (control->hw_id < ENCODERS_COUNT)
    {
    	//draw index, no need to update so no data
    	naveg_set_index(0, control->hw_id, 0, 0);
    }
}

static void request_control_page(control_t *control, uint8_t dir)
{
    // sets the response callback
    comm_webgui_set_response_cb(parse_control_page, NULL);

    char buffer[20];
    memset(buffer, 0, sizeof buffer);
    uint8_t i;

    i = copy_command(buffer, CONTROL_PAGE_CMD); 

    // insert the hw_id on buffer
    i += int_to_str(control->hw_id, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    //bitmask for the page direction and wrap around
	uint8_t bitmask = 0;
	if (dir) bitmask |= LIST_PAGE_UP;
	if ((control->hw_id >= ENCODERS_COUNT) && (control->scale_points_flag & CONTROL_WRAP_AROUND)) bitmask |= LIST_WRAP_AROUND;

    // insert the direction on buffer
    i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

    // sends the data to GUI
    comm_webgui_send(buffer, i);

    // waits the banks list be received
    comm_webgui_wait_response();;
}

static void parse_banks_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;
    uint16_t count = strarr_length(&list[5]);

    // workaround freeze when opening menu
    delay_ms(20);

    // free the current banks list
    if (g_banks) data_free_banks_list(g_banks);

    // parses the list
    g_banks = data_parse_banks_list(&list[5], count);

    if (g_banks)
    {
    	g_banks->menu_max = (atoi(list[2]));
        g_banks->page_min = (atoi(list[3]));
        g_banks->page_max = (atoi(list[4]));
    }

    naveg_set_banks(g_banks);
}

//only toggled from the naveg toggle tool function
static void request_banks_list(uint8_t dir)
{
    g_bp_state = BANKS_LIST;

    // sets the response callback
    comm_webgui_set_response_cb(parse_banks_list, NULL);

    char buffer[40];
    memset(buffer, 0, 20);
    uint8_t i;

    i = copy_command(buffer, BANKS_CMD); 

    // insert the direction on buffer
    i += int_to_str(dir, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    //insert current bank, because first time we are entering the menu
    i += int_to_str(g_current_bank, &buffer[i], sizeof(buffer) - i, 0);

    // sends the data to GUI
    comm_webgui_send(buffer, i);

    // waits the banks list be received
    comm_webgui_wait_response();

    g_banks->hover = g_current_bank;
    g_banks->selected = g_current_bank;
}

//requested from the bp_up / bp_down functions when we reach the end of a page
static void request_next_bank_page(uint8_t dir)
{
	//we need to save our current hover and selected here, the parsing function will discard those
	uint8_t prev_hover = g_banks->hover;
	uint8_t prev_selected = g_banks->selected;

	//request the banks as usual
	g_bp_state = BANKS_LIST;

    // sets the response callback
    comm_webgui_set_response_cb(parse_banks_list, NULL);

    char buffer[40];
    memset(buffer, 0, sizeof buffer);
    uint8_t i;

    i = copy_command(buffer, BANKS_CMD); 

    // insert the direction on buffer
    i += int_to_str(dir, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    i += int_to_str(g_banks->hover, &buffer[i], sizeof(buffer) - i, 0);

    // sends the data to GUI
    comm_webgui_send(buffer, i);

    // waits the banks list be received
    comm_webgui_wait_response();

	//restore our previous hover / selected bank
	g_banks->hover = prev_hover;
	g_banks->selected = prev_selected;
}

//called from the request functions and the naveg_initail_state
static void parse_pedalboards_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;
    uint32_t count = strarr_length(&list[5]);

    // workaround freeze when opening menu
    delay_ms(20);

    // free the navigation pedalboads list
    if (g_naveg_pedalboards)
        data_free_pedalboards_list(g_naveg_pedalboards);

    // parses the list
    g_naveg_pedalboards = data_parse_pedalboards_list(&list[5], count);

    if (g_naveg_pedalboards)
    {
    	g_naveg_pedalboards->menu_max = (atoi(list[2]));
    	g_naveg_pedalboards->page_min = (atoi(list[3]));
    	g_naveg_pedalboards->page_max = (atoi(list[4])); 
    }
}

//requested when clicked on a back
static void request_pedalboards(uint8_t dir, uint16_t bank_uid)
{
	uint8_t i;
	char buffer[40];
	memset(buffer, 0, sizeof buffer);

    // sets the response callback
    comm_webgui_set_response_cb(parse_pedalboards_list, NULL);

    i = copy_command((char *)buffer, PEDALBOARDS_CMD);

    uint8_t bitmask = 0;
	if (dir == 1) {bitmask |= LIST_PAGE_UP;}
	else if (dir == 2) bitmask |= LIST_INITIAL_REQ;

    // insert the direction on buffer
    i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    if ((dir == 2) && (g_current_bank != g_banks->hover))
   	{
		// insert the current hover on buffer
    	i += int_to_str(0, &buffer[i], sizeof(buffer) - i, 0);
   	} 
    else
    {
    	// insert the current hover on buffer
    	i += int_to_str(g_naveg_pedalboards->hover - 1, &buffer[i], sizeof(buffer) - i, 0);
    }

    // inserts one space
    buffer[i++] = ' ';

    // copy the bank uid
    i += int_to_str((bank_uid), &buffer[i], sizeof(buffer) - i, 0);

    buffer[i++] = 0;

	uint16_t prev_hover = g_current_pedalboard;
	uint16_t prev_selected = g_current_pedalboard;

    if (g_naveg_pedalboards)
    {
    	prev_hover = g_naveg_pedalboards->hover;
    	prev_selected = g_naveg_pedalboards->selected;
    }
    
    // sends the data to GUI
    comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    comm_webgui_wait_response();

    if (g_naveg_pedalboards)
    {
    	g_naveg_pedalboards->hover = prev_hover;
    	g_naveg_pedalboards->selected = prev_selected;
    }
}

static void send_load_pedalboard(uint16_t bank_id, const char *pedalboard_uid)
{
    uint16_t i;
    char buffer[40];
    memset(buffer, 0, sizeof buffer);

    i = copy_command((char *)buffer, PEDALBOARD_CMD);

    // copy the bank id
    i += int_to_str(bank_id, &buffer[i], 8, 0);

    // inserts one space
    buffer[i++] = ' ';

	const char *p = pedalboard_uid;
    // copy the pedalboard uidf
    if (!*p) 
   	{
   		buffer[i++] = '0';
   	}
   	else
   	{
		while (*p)
		{
		    buffer[i++] = *p;
		    p++;
		}
	}
    buffer[i] = 0;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sets the response callback
    comm_webgui_set_response_cb(NULL, NULL);

    // send the data to GUI
    comm_webgui_send(buffer, i);

    // waits the pedalboard loaded message to be received
    comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

static void control_set(uint8_t id, control_t *control)
{
    uint32_t now, delta;

    switch (control->properties)
    {
        case CONTROL_PROP_LINEAR:
        case CONTROL_PROP_INTEGER:
        case CONTROL_PROP_LOGARITHMIC:
            if (control->hw_id < ENCODERS_COUNT)
            {
                // update the screen
                if (!display_has_tool_enabled(id))
                    screen_control(id, control);
            }
            break;

        case CONTROL_PROP_REVERSE_ENUM:
        case CONTROL_PROP_ENUMERATION:
        case CONTROL_PROP_SCALE_POINTS:
        	//encoder (pagination is done in the increment / decrement functions)
            if (control->hw_id < ENCODERS_COUNT)
            {
                // update the screen
                if (!display_has_tool_enabled(id))
                    screen_control(id, control);
            }
            //footswitch (need to check for pages here)
            else if ((ENCODERS_COUNT <= control->hw_id) && ( control->hw_id < FOOTSWITCHES_ACTUATOR_COUNT + ENCODERS_COUNT))
            {
                if (control->properties != CONTROL_PROP_REVERSE_ENUM)
                {
        			// increments the step
        			if (control->step < (control->steps - 1))
        			    control->step++;
        			//if we are reaching the end of the control
        			else if (control->scale_points_flag & CONTROL_END_PAGE)
        			{
        				//we wrap around so the step becomes 0 again
        				if (control->scale_points_flag & CONTROL_WRAP_AROUND)
        				{
        					request_control_page(control, 1);
        				}
        				//we are at max and dont wrap around
        				else return; 
        			}
        			//we need to request a new page
        			else if (control->scale_points_flag & CONTROL_PAGINATED) 
        			{
        				//request new data, a new control we be assigned after
        				request_control_page(control, 1);

        				//since a new control is assigned we can return
        				return;
        			}
                    else return;
                }
                else 
                {
        			// decrements the step
        			if (control->step > 0)
        			    control->step--;
        			//we are at the end of our list ask for more data
        			else if ((control->scale_points_flag & CONTROL_PAGINATED) || (control->scale_points_flag & CONTROL_WRAP_AROUND))
        			{
        			    //request new data, a new control we be assigned after
        			    request_control_page(control, 0);
	
	        	    	//since a new control is assigned we can return
        	    		return;
                	}
                    else return;
               	}


                // updates the value and the screen
                control->value = control->scale_points[control->step]->value;
                if (!display_has_tool_enabled(get_display_by_id(id, FOOT)))
                    screen_footer(control->hw_id - ENCODERS_COUNT, control->label, control->scale_points[control->step]->label);
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

            // to update the footer and screen
            foot_control_add(control);
            break;

        case CONTROL_PROP_TAP_TEMPO:
            now = hardware_timestamp();
            delta = now - g_tap_tempo[control->hw_id - ENCODERS_COUNT].time;
            g_tap_tempo[control->hw_id - ENCODERS_COUNT].time = now;

            if (g_tap_tempo[control->hw_id - ENCODERS_COUNT].state == TT_COUNTING)
            {
                // checks if delta almost suits maximum allowed value
                if ((delta > g_tap_tempo[control->hw_id - ENCODERS_COUNT].max) &&
                    ((delta - TAP_TEMPO_MAXVAL_OVERFLOW) < g_tap_tempo[control->hw_id - ENCODERS_COUNT].max))
                {
                    // sets delta to maxvalue if just slightly over, instead of doing nothing
                    delta = g_tap_tempo[control->hw_id - ENCODERS_COUNT].max;
                }

                // checks the tap tempo timeout
                if (delta <= g_tap_tempo[control->hw_id - ENCODERS_COUNT].max)
                {
                    //get current value of tap tempo in ms
                    float currentTapVal = convert_to_ms(control->unit, control->value);
                    //check if it should be added to running average
                    if (abs(currentTapVal - delta) < TAP_TEMPO_TAP_HYSTERESIS)
                    {
                        // converts and update the tap tempo value
                        control->value = (2*(control->value) + convert_from_ms(control->unit, delta)) / 3;
                    }
                    else
                    {
                        // converts and update the tap tempo value
                        control->value = convert_from_ms(control->unit, delta);
                    }

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

    // insert the hw_id on buffer
    i += int_to_str(control->hw_id, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i++] = ' ';

    // insert the value on buffer
    i += float_to_str(control->value, &buffer[i], sizeof(buffer) - i, 3);
    buffer[i] = 0;

    // send the data to GUI
    comm_webgui_send(buffer, i);

    //wait for a response from mod-ui
    if (g_should_wait_for_webgui) {
        comm_webgui_wait_response();
    }
}

static void bp_enter(void)
{
    const char *title;

    if (naveg_ui_status())
    {
        tool_off(DISPLAY_TOOL_NAVIG);
        tool_on(DISPLAY_TOOL_SYSTEM, 0);
        tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
        screen_system_menu(g_current_item);
    }

    if (!g_banks)
        return;

    if (g_bp_state == BANKS_LIST)
    {
        //make sure that we request the right page if we have a selected pedalboard
        if (g_current_bank == g_banks->hover)
            g_naveg_pedalboards->hover = g_current_pedalboard;

    	//index is relevent in our array so - page_min
        request_pedalboards(PAGE_DIR_INIT, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));

        // if reach here, received the pedalboards list
        g_bp_state = PEDALBOARD_LIST;
        //index is relevent in our array so - page_min
        title = g_banks->names[g_banks->hover - g_banks->page_min];
        g_bp_first = 1;
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        // checks if is the first option (back to banks list)
        if (g_naveg_pedalboards->hover == 0)
        {
            g_bp_state = BANKS_LIST;
            title = "BANKS";
        }
        else
        {
            g_bp_first=0;
            g_pb_footswitches = 1; 

            // request to GUI load the pedalboard
            //index is relevant in the array so - page_min, also the HMI array is always shifted right 1 because of back to banks, correct here
            send_load_pedalboard(atoi(g_banks->uids[g_banks->hover - g_banks->page_min]), g_naveg_pedalboards->uids[g_naveg_pedalboards->hover - g_naveg_pedalboards->page_min - 1]);

            g_current_pedalboard = g_naveg_pedalboards->hover;

            g_force_update_pedalboard = 1;

            // stores the current bank and pedalboard
            g_banks->selected = g_banks->hover;
            g_current_bank = g_banks->selected;

            // sets the variables to update the screen
            title = g_banks->names[g_banks->hover - g_banks->page_min];
        }
    }
    else
    {
        return;
    }

    //check if we need to display the selected item or out of bounds
    if (g_current_bank == g_banks->hover)
    {
        g_naveg_pedalboards->selected = g_current_pedalboard;
        g_naveg_pedalboards->hover = g_current_pedalboard;
    }
    else 
    {
        g_naveg_pedalboards->selected = g_naveg_pedalboards->menu_max + 1;
        g_naveg_pedalboards->hover = 0;
    }

    screen_bp_list(title, (g_bp_state == PEDALBOARD_LIST)? g_naveg_pedalboards : g_banks);
}

static void bp_up(void)
{
    bp_list_t *bp_list = 0;
    const char *title = 0;

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
    	//if we are nearing the final 3 items of the page, and we already have the end of the page in memory, or if we just need to go down
    	if(g_banks->page_min == 0) 
    	{
    		//check if we are not already at the end
    		if (g_banks->hover == 0) return;
    		else 
    		{
        		//we still have banks in our list
         		g_banks->hover--;

         		bp_list = g_banks;
        		title = "BANKS";
    		}
    	}
    	//are we comming from the bottom of the menu?
    	else
    	{
    		//we always keep 3 items in front of us, if not request new page
    		if (g_banks->hover <= (g_banks->page_min + 3))
    		{
    			g_banks->hover--;
        		title = "BANKS";

    			//request new page
    			request_next_bank_page(PAGE_DIR_DOWN);

                bp_list = g_banks;
    		}	
    		//we have items, just go up
    		else 
    		{
         		g_banks->hover--;
         		bp_list = g_banks;
        		title = "BANKS";
    		}
    	}
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
    	//are we reaching the bottom of the menu?
    	if(g_naveg_pedalboards->page_min == 0) 
    	{
    		//check if we are not already at the end
    		if (g_naveg_pedalboards->hover == 0) return;
    		else 
    		{
    			//go down till the end
    			//we still have items in our list
         		g_naveg_pedalboards->hover--;
         		bp_list = g_naveg_pedalboards;
        		title = g_banks->names[g_banks->hover - g_banks->page_min];
        	}
    	}
    	else
    	{
    		//we always keep 3 items in front of us, if not request new page, we add 4 because the bottom item is always "> back to banks" and we dont show that here
    		if (g_naveg_pedalboards->hover <= (g_naveg_pedalboards->page_min + 5))
    		{
    			g_naveg_pedalboards->hover--;
        		title = g_banks->names[g_banks->hover - g_banks->page_min];
    			//request new page
    			request_pedalboards(PAGE_DIR_DOWN, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));

                bp_list = g_naveg_pedalboards;
    		}	
    		//we have items, just go up
    		else 
    		{
         		g_naveg_pedalboards->hover--;
         		bp_list = g_naveg_pedalboards;
        		title = g_banks->names[g_banks->hover - g_banks->page_min];
    		}
    	}
    }
    else return;

    screen_bp_list(title, bp_list);
}

static void bp_down(void)
{
    bp_list_t *bp_list = 0;
    const char *title = 0;

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
    	//are we reaching the bottom of the menu?
    	if(g_banks->page_max == g_banks->menu_max) 
    	{
    		//check if we are not already at the end
    		if (g_banks->hover >= g_banks->menu_max -1) return;
    		else 
    		{
        		//we still have banks in our list
         		g_banks->hover++;
         		bp_list = g_banks;
        		title = "BANKS";
    		}
    	}
    	else 
    	{
    		//we always keep 3 items in front of us, if not request new page
    		if (g_banks->hover >= (g_banks->page_max - 4))
    		{
    			g_banks->hover++;
        		title = "BANKS";
    			//request new page
    			request_next_bank_page(PAGE_DIR_UP);

                bp_list = g_banks;
    		}	
    		//we have items, just go down
    		else 
    		{
         		g_banks->hover++;
         		bp_list = g_banks;
        		title = "BANKS";
    		}
    	}
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
    	//are we reaching the bottom of the menu, -1 because menu max is bigger then page_max
    	if(g_naveg_pedalboards->page_max == g_naveg_pedalboards->menu_max) 
    	{
    		//check if we are not already at the end, we dont need so substract by one, since the "> back to banks" item is added on parsing
    		if (g_naveg_pedalboards->hover == g_naveg_pedalboards->menu_max) return;
    		else 
    		{
    			//we still have items in our list
         		g_naveg_pedalboards->hover++;
         		bp_list = g_naveg_pedalboards;
        		title = g_banks->names[g_banks->hover - g_banks->page_min];
    		}
    	}
    	else 
    	{
    		//we always keep 3 items in front of us, if not request new page, we need to substract by 5, since the "> back to banks" item is added on parsing 
    		if (g_naveg_pedalboards->hover >= (g_naveg_pedalboards->page_max - 4))
    		{
        		title = g_banks->names[g_banks->hover - g_banks->page_min];
    			//request new page
    			request_pedalboards(PAGE_DIR_UP, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));

                g_naveg_pedalboards->hover++;
                bp_list = g_naveg_pedalboards;
    		}	
    		//we have items, just go down
    		else 
    		{
         		g_naveg_pedalboards->hover++;
         		bp_list = g_naveg_pedalboards;
        		title = g_banks->names[g_banks->hover - g_banks->page_min];
    		}
    	}
    }
    else return;

    screen_bp_list(title, bp_list);
}

static void menu_enter(uint8_t display_id)
{
    uint8_t i;
    node_t *node = (display_id || dialog_active) ? g_current_menu : g_current_main_menu;
    menu_item_t *item = (display_id || dialog_active) ? g_current_item : g_current_main_item;

    if (item->desc->type == MENU_LIST || item->desc->type == MENU_SELECT)
    {
        // locates the clicked item
        node = display_id ? g_current_menu->first_child : g_current_main_menu->first_child;
        for (i = 0; i < item->data.hover; i++) node = node->next;

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
        //extra check if we need to switch back to non-ui connected mode on the current-pb and banks menu
        else if (((item->desc->id == BANKS_ID) || (item->desc->id == PEDALBOARD_ID)) && (!naveg_ui_status()))
        {
            item->desc->type = ((item->desc->id == PEDALBOARD_ID) ? MENU_LIST : MENU_NONE);
        }

 		// updates the current item
       	if ((item->desc->type != MENU_TOGGLE) && (item->desc->type != MENU_NONE)) g_current_item = node->data;
       	// updates this specific toggle items (toggle items with pop-ups)
        if (item->desc->parent_id == PROFILES_ID) g_current_item = node->data;
    }
    else if (item->desc->type == MENU_CONFIRM || item->desc->type == MENU_CANCEL || item->desc->type == MENU_OK ||
            item->desc->parent_id == PROFILES_ID || item->desc->id == EXP_CV_INP || item->desc->id == HP_CV_OUTP)
    {
        // calls the action callback
        if ((item->desc->type != MENU_OK) && (item->desc->type != MENU_CANCEL) && (item->desc->action_cb))
            item->desc->action_cb(item, MENU_EV_ENTER);

        if ((item->desc->id == PEDALBOARD_ID) || (item->desc->id == BANKS_ID))
        {
            if (naveg_ui_status())
            {
                //change menu type to menu_ok to display pop-up
                item->desc->type = MENU_MESSAGE;
                g_current_item = item;
            }
            else
            {
                //reset the menu type to its original state
                item->desc->type = ((item->desc->id == PEDALBOARD_ID) ? MENU_LIST : MENU_NONE);
                g_current_item = item;
            }
        }
        else
        {
            // gets the menu item
            item = display_id ? g_current_menu->data : g_current_main_menu->data;
            g_current_item = item;
        }
    }

    //if the tuner is clicked either enable it or action_cb
    if (item->desc->id == TUNER_ID)
    {
        if (!tool_is_on(DISPLAY_TOOL_TUNER))
        {
            naveg_toggle_tool(DISPLAY_TOOL_TUNER, 1);
            return;
        }
        // calls the action callback
        else if (g_current_main_item->desc->action_cb) g_current_main_item->desc->action_cb(g_current_main_item, MENU_EV_ENTER);
    }

    // FIXME: that's dirty, so dirty...
    if ((item->desc->id == PEDALBOARD_ID) || (item->desc->id == BANKS_ID))
    {
        if (naveg_ui_status()) item->desc->type = MENU_MESSAGE;
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

            //all the menu items that have a value that needs to be updated when enterign the menu
            if ((item_child->desc->type == MENU_SET) || (item_child->desc->type == MENU_TOGGLE) || (item_child->desc->type == MENU_VOL) ||
                (item_child->desc->id == TEMPO_ID) || (item_child->desc->id == TUNER_ID) || (item_child->desc->id == BYPASS_ID) || (item_child->desc->id == BANKS_ID))
                {
                    //update the value with menu_ev_none
                   if (item_child->desc->action_cb) item_child->desc->action_cb(item_child, MENU_EV_NONE);
                }
            item->data.list[item->data.list_count++] = item_child->name;
        }

        //prevent toggling of these items.
        if ((item->desc->id != TEMPO_ID) && (item->desc->id != BYPASS_ID) && (item->desc->id != BANKS_ID))
        {
            // calls the action callback
            if ((item->desc->action_cb)) item->desc->action_cb(item, MENU_EV_ENTER);
        }
    }
    else if (item->desc->type == MENU_CONFIRM ||item->desc->type == MENU_OK || item->desc->parent_id == PROFILES_ID ||  item->desc->id == EXP_CV_INP || item->desc->id == HP_CV_OUTP || item->desc->type == MENU_MESSAGE)
    {
        if (item->desc->type == MENU_OK)
        {
    	    //if bleutooth activate right away
            if (item->desc->id == BLUETOOTH_DISCO_ID)
            {
                item->desc->action_cb(item, MENU_EV_ENTER);
            }
            
            // highlights the default button
            item->data.hover = 0;

            // defines the buttons count
            item->data.list_count = 1;
        }
        else if (item->desc->type == MENU_MESSAGE)
        {
            // highlights the default button
            item->data.hover = 0;

            // defines the buttons count
            item->data.list_count = 0;
        }
        else
        {
            // highlights the default button
            item->data.hover = 1;

            // defines the buttons count
            item->data.list_count = 2;
        }

        // default popup content value
        item->data.popup_content = NULL;
        // locates the popup menu
        i = 0;
        while (g_menu_popups[i].popup_content)
        {
            if (item->desc->id == g_menu_popups[i].menu_id)
            {
                //load/reload profile popups
                if ((item->desc->parent_id == PROFILES_ID) && (item->desc->id != PROFILES_ID+5))
                {
                    //set the 'reload' profile popups
                    if (item->data.value)
                    {
                        item->data.popup_content = g_menu_popups[i + 5].popup_content;
                        item->data.popup_header = g_menu_popups[i + 5].popup_header;
                    }
                    //set the 'load' profile popups
                    else
                    {
                        item->data.popup_content = g_menu_popups[i].popup_content;
                        item->data.popup_header = g_menu_popups[i].popup_header;
                    }
                }
                //save profile popups
                else if (item->desc->id == PROFILES_ID+5)
                {
                    item->data.popup_content = g_menu_popups[i].popup_content;

                    //add the to be saved profile char to the header of the popup
                    char *profile_char;

                    item->data.value = system_get_current_profile();

                    if (item->data.value ==1) profile_char = "A";
                    else if (item->data.value ==2) profile_char = "B";
                    else if (item->data.value ==3) profile_char = "C";
                    else if (item->data.value ==4) profile_char = "D";
                    else profile_char = "X";

                    char *txt =  g_menu_popups[i].popup_header;
                    char * str3 = (char *) malloc(1 + strlen(txt)+ strlen(profile_char) );
                    strcpy(str3, txt);
                    strcat(str3, profile_char);
                    item->data.popup_header = (str3);

                }
                //cv toggle popups
                else if ((item->desc->id == EXP_CV_INP) || (item->desc->id == HP_CV_OUTP))
                {
                    if (!item->data.value)
                    {
                        item->data.popup_content = g_menu_popups[i].popup_content;
                        item->data.popup_header = g_menu_popups[i].popup_header;
                    }
                    else
                    {
                        item->data.popup_content = g_menu_popups[i + 1].popup_content;
                        item->data.popup_header = g_menu_popups[i + 1].popup_header;
                    }
                }
                //other popups
                else
                {
                    item->data.popup_content = g_menu_popups[i].popup_content;
                    item->data.popup_header = g_menu_popups[i].popup_header;
                }

                break;
            }
            i++;
        }
    }
    else if (item->desc->type == MENU_TOGGLE)
    {
        // calls the action callback
        if (item->desc->action_cb) item->desc->action_cb(item, MENU_EV_ENTER);
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
        if ((item->desc->action_cb) && (item->desc->id != BANKS_ID))
        {
            item->desc->action_cb(item, MENU_EV_ENTER);
        }
    }
    else if (item->desc->type == MENU_NONE)
    {
        // checks if the parent item type is MENU_SELECT
        if (g_current_item->desc->type == MENU_SELECT)
        {
            // deselects all items
            for (i = 0; i < g_current_item->data.list_count; i++)
                deselect_item(g_current_item->data.list[i]);

            // selects the current item
            select_item(item->name);
        }

        // calls the action callback
        if (item->desc->action_cb)
            item->desc->action_cb(item, MENU_EV_ENTER);

        if (display_id)
        {
            item = g_current_menu->data;
            g_current_item = item;
        }
    }
    else if ((item->desc->type == MENU_VOL) || (item->desc->type == MENU_SET))
    {
        if (display_id)
        {
            static uint8_t toggle = 0;
            if (toggle == 0)
            {
                toggle = 1;
                // calls the action callback
                if ((item->desc->action_cb) && (item->desc->type != MENU_SET))
                    item->desc->action_cb(item, MENU_EV_ENTER);
            }
            else
            {
                toggle = 0;
                if (item->desc->type == MENU_VOL)
                    system_save_gains_cb(item, MENU_EV_ENTER);
                else
                    item->desc->action_cb(item, MENU_EV_ENTER);
                //resets the menu node
                item = g_current_menu->data;
                g_current_item = item;
            }
        }
    }

    if (item->desc->parent_id == DEVICE_ID && item->desc->action_cb)
        item->desc->action_cb(item, MENU_EV_ENTER);

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
        dialog_active = 0;
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(g_dialog_sem, &xHigherPriorityTaskWoken);
    }
}

static void menu_up(uint8_t display_id)
{
    menu_item_t *item = (display_id) ? g_current_item : g_current_main_item;

    if ((item->desc->type == MENU_VOL) || (item->desc->type == MENU_SET))
    {
        //substract one, if we reach the limit, value becomes the limit
        if ((item->data.value -= (item->data.step)) < item->data.min)
        {
            item->data.value = item->data.min;
        }
    }
    else
    {
        if (item->data.hover > 0)
            item->data.hover--;
    }

    if (item->desc->action_cb)
        item->desc->action_cb(item, MENU_EV_UP);

    screen_system_menu(item);
}


static void menu_down(uint8_t display_id)
{
    menu_item_t *item = (display_id) ? g_current_item : g_current_main_item;

    if ((item->desc->type == MENU_VOL) || (item->desc->type == MENU_SET))
    {
        //up one, if we reach the limit, value becomes the limit
        if ((item->data.value += (item->data.step)) > item->data.max)
        {
            item->data.value = item->data.max;
        }
    }
    else
    {
        if (item->data.hover < (item->data.list_count - 1))
            item->data.hover++;
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
        if (item->desc->type == MENU_LIST || item->desc->type == MENU_SELECT) 
            item->data.hover = 0;
        reset_menu_hover(node);
    }
}

static void parse_footswitch_pedalboards_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;

    uint32_t count = strarr_length(&list[5]);

    // workaround freeze when opening menu
    delay_ms(20);

    // parses the list
    bp_list_t *pb_list_bfr = data_parse_pedalboards_list(&list[5], count);

    if (pb_list_bfr)
    {
    	memcpy(&g_footswitch_pedalboards, pb_list_bfr, sizeof(bp_list_t));
    	g_footswitch_pedalboards.names = str_array_duplicate(pb_list_bfr->names, (g_naveg_pedalboards->page_max - g_naveg_pedalboards->page_min + 1));
    	g_footswitch_pedalboards.uids  = str_array_duplicate(pb_list_bfr->uids, (g_naveg_pedalboards->page_max - g_naveg_pedalboards->page_min + 1));
    }

    g_footswitch_pedalboards.menu_max = (atoi(list[2]));
    g_footswitch_pedalboards.page_min = (atoi(list[3]));
    g_footswitch_pedalboards.page_max = (atoi(list[4])); 

    data_free_pedalboards_list(pb_list_bfr);
}

static void request_footswitch_pedalboards(uint8_t dir)
{
	uint8_t i;
	char buffer[40];
	memset(buffer, 0, sizeof buffer);

	// sets the response callback
	comm_webgui_set_response_cb(parse_footswitch_pedalboards_list, NULL);

	//create the command
	i = copy_command((char *)buffer, PEDALBOARDS_CMD);

	uint8_t bitmask = 0;
	if (dir == 1) {bitmask |= LIST_PAGE_UP;}

	// insert the direction on buffer
	i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

	// inserts one space
	buffer[i++] = ' ';

	// insert the current hover on buffer
	i += int_to_str(g_current_pedalboard, &buffer[i], sizeof(buffer) - i, 0);

	// inserts one space
	buffer[i++] = ' ';

	// copy the bank uid
	i += int_to_str((g_current_bank), &buffer[i], sizeof(buffer) - i, 0);

    buffer[i++] = 0;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sends the data to GUI
    comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

static uint8_t bank_config_check(uint8_t foot)
{
    uint8_t i;

    for (i = 1; i < BANK_FUNC_AMOUNT; i++)
    {
        if (g_bank_functions[i].hw_id - ENCODERS_COUNT == foot &&
            g_bank_functions[i].function != BANK_FUNC_NONE)
        {
            return i;
        }
    }

    return 0;
}

static void bank_config_update(uint8_t bank_func_idx)
{
    uint8_t i = bank_func_idx;

    switch (g_bank_functions[i].function)
    {
        case BANK_FUNC_PEDALBOARD_NEXT:
            	//check if we need to request a new page
            	if (g_current_pedalboard >= g_footswitch_pedalboards.page_max - 1)
            	{
            		//we do not request a new page when there is none
            		if (g_footswitch_pedalboards.page_max == g_footswitch_pedalboards.menu_max)
            		{
            			if (g_current_pedalboard == g_footswitch_pedalboards.menu_max) return;
            			else g_current_pedalboard++;
            		}
            		else
           			{
 						g_current_pedalboard++;
            			request_footswitch_pedalboards(PAGE_DIR_UP);
            		}
            	}
            	else g_current_pedalboard++;

                send_load_pedalboard(g_current_bank, g_footswitch_pedalboards.uids[g_current_pedalboard - g_footswitch_pedalboards.page_min - 1]);

            break;

        case BANK_FUNC_PEDALBOARD_PREV:           
				//check if we are reaching the max of the page
            	if (g_current_pedalboard <= g_footswitch_pedalboards.page_min + 2)
            	{
            		if (g_footswitch_pedalboards.page_min == 0)
            		{
            			//we do not request a new page when there is none
            			if (g_current_pedalboard ==  1) return;
            			else g_current_pedalboard--;
            		}

            		else
            		{
            			g_current_pedalboard--;

            			request_footswitch_pedalboards(PAGE_DIR_DOWN);
            		}
            	}
            	//just go to the end
            	else g_current_pedalboard--;

                send_load_pedalboard(g_current_bank, g_footswitch_pedalboards.uids[g_current_pedalboard - g_footswitch_pedalboards.page_min - 1]);
            
            break;
    }
}

static void bank_config_footer(void)
{
    uint8_t bypass;

    char *pedalboard_name = NULL;

		if (g_force_update_pedalboard) 
		{
			//also put as footswitch pedalboards
			if (g_naveg_pedalboards)
			{
				memcpy(&g_footswitch_pedalboards, g_naveg_pedalboards, sizeof(bp_list_t));
				g_footswitch_pedalboards.names = str_array_duplicate(g_naveg_pedalboards->names, (g_naveg_pedalboards->page_max - g_naveg_pedalboards->page_min + 1));
				g_footswitch_pedalboards.uids  = str_array_duplicate(g_naveg_pedalboards->uids, (g_naveg_pedalboards->page_max - g_naveg_pedalboards->page_min + 1));
			}

			//index is relevant thats why - page_min
			pedalboard_name = g_footswitch_pedalboards.names[g_current_pedalboard - g_footswitch_pedalboards.page_min];
			g_force_update_pedalboard = 0;
		}
    //index is relevant thats why - page_min
    else pedalboard_name = g_footswitch_pedalboards.names[g_current_pedalboard - g_footswitch_pedalboards.page_min];

    // updates all footer screen with bank functions
    uint8_t i;
    for (i = 1; i < BANK_FUNC_AMOUNT; i++)
    {
        bank_config_t *bank_conf;
        bank_conf = &g_bank_functions[i];
        color_t color;

        // default state of led blink (no blink)
        led_blink(hardware_leds(bank_conf->hw_id - ENCODERS_COUNT), 0, 0);

        switch (bank_conf->function)
        {
            case BANK_FUNC_TRUE_BYPASS:
                bypass = 0; // FIX: get true bypass state
                led_set_color(hardware_leds(bank_conf->hw_id -  ENCODERS_COUNT), bypass ? BLACK : TRUE_BYPASS_COLOR);

                if (display_has_tool_enabled(bank_conf->hw_id - ENCODERS_COUNT)) break;
               	screen_footer(bank_conf->hw_id- ENCODERS_COUNT, TRUE_BYPASS_FOOTER_TEXT,
                            (bypass ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT));
                break;

            case BANK_FUNC_PEDALBOARD_NEXT:
                if (g_current_pedalboard == (g_footswitch_pedalboards.menu_max)) color = BLACK;
                else color = PEDALBOARD_NEXT_COLOR;

                led_set_color(hardware_leds(bank_conf->hw_id - ENCODERS_COUNT), color);

                if (display_has_tool_enabled(bank_conf->hw_id - ENCODERS_COUNT)) break;
                screen_footer(bank_conf->hw_id - ENCODERS_COUNT, pedalboard_name, PEDALBOARD_NEXT_FOOTER_TEXT);
                break;

            case BANK_FUNC_PEDALBOARD_PREV:
                if (g_current_pedalboard == 1) color = BLACK;
                else color = PEDALBOARD_PREV_COLOR;

                led_set_color(hardware_leds(bank_conf->hw_id - ENCODERS_COUNT), color);

                if (display_has_tool_enabled(bank_conf->hw_id - ENCODERS_COUNT)) break;
                screen_footer(bank_conf->hw_id - ENCODERS_COUNT, pedalboard_name , PEDALBOARD_PREV_FOOTER_TEXT);
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
    g_bp_state = BANKS_LIST;

    // initializes the bank functions
    for (i = 0; i < BANK_FUNC_AMOUNT; i++)
    {
        g_bank_functions[i].function = BANK_FUNC_NONE;
        g_bank_functions[i].hw_id = 0xFF;
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
    // vSemaphoreCreateBinary is created as available which makes
    // first xSemaphoreTake pass even if semaphore has not been given
    // http://sourceforge.net/p/freertos/discussion/382005/thread/04bfabb9
    xSemaphoreTake(g_dialog_sem, 0);
}

void naveg_initial_state(uint16_t max_menu, uint16_t page_min, uint16_t page_max, char *bank_uid, char *pedalboard_uid, char **pedalboards_list)
{
    if (!pedalboards_list)
    {
        if (g_banks)
        {
            g_banks->selected = 0;
            g_banks->hover = 0;
            g_banks->page_min = 0;
            g_banks->page_max = 0;
            g_banks->menu_max = 0;
            g_current_bank = 0;
        }

        if (g_naveg_pedalboards)
        {
            g_naveg_pedalboards->selected = 0;
            g_naveg_pedalboards->hover = 0;
            g_naveg_pedalboards->page_min = 0;
            g_naveg_pedalboards->page_max = 0;
            g_naveg_pedalboards->menu_max = 0;
            g_current_pedalboard = 0;
        }

        return;
    }

    // sets the bank index
    uint16_t bank_id = atoi(bank_uid);
    g_current_bank = bank_id;
    if (g_banks)
    {
        g_banks->selected = bank_id;
        g_banks->hover = bank_id;
    }

    // checks and free the navigation pedalboads list
    if (g_naveg_pedalboards) data_free_pedalboards_list(g_naveg_pedalboards);

    // parses the list
    g_naveg_pedalboards = data_parse_pedalboards_list(pedalboards_list, strarr_length(pedalboards_list));

    if (!g_naveg_pedalboards) return;

    g_naveg_pedalboards->page_min = page_min;
    g_naveg_pedalboards->page_max = page_max;
    g_naveg_pedalboards->menu_max = max_menu;

    g_current_pedalboard = atoi(pedalboard_uid) + 1;

    g_naveg_pedalboards->hover = g_current_pedalboard;
    g_naveg_pedalboards->selected = g_current_pedalboard;

    //also put as footswitch pedalboards
    if (g_naveg_pedalboards)
    {
    	memcpy(&g_footswitch_pedalboards, g_naveg_pedalboards, sizeof(bp_list_t));
    	g_footswitch_pedalboards.names = str_array_duplicate(g_naveg_pedalboards->names, (g_naveg_pedalboards->page_max - g_naveg_pedalboards->page_min + 1));
    	g_footswitch_pedalboards.uids  = str_array_duplicate(g_naveg_pedalboards->uids, (g_naveg_pedalboards->page_max - g_naveg_pedalboards->page_min + 1));
    }
}

void naveg_ui_connection(uint8_t status)
{
    if (!g_initialized) return;

    if (status == UI_CONNECTED)
    {
        g_ui_connected = 1;
        //turn of footswitch navigation internal value
        system_update_menu_value(BANKS_ID, 0);
    }
    else
    {
        g_ui_connected = 0;

        // reset the banks and pedalboards state after return from ui connection
        if (g_banks) data_free_banks_list(g_banks);
        if (g_naveg_pedalboards) data_free_pedalboards_list(g_naveg_pedalboards);
        g_banks = NULL;
        g_naveg_pedalboards = NULL;
    }


    if ((tool_is_on(DISPLAY_TOOL_NAVIG)) || tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
        naveg_toggle_tool(DISPLAY_TOOL_SYSTEM, 0);
        node_t *node = g_current_main_menu;

        //sets the pedalboard items back to original
        for (node = node->first_child; node; node = node->next)
        {
            // gets the menu item
            menu_item_t *item = node->data;

            if ((item->desc->id == PEDALBOARD_ID) || (item->desc->id == BANKS_ID))
            {
                item->desc->type = ((item->desc->id == PEDALBOARD_ID) ? MENU_LIST : MENU_NONE);
                g_current_item = item;
            }
        }
    }

}

void naveg_add_control(control_t *control)
{
    if (!g_initialized) return;
    if (!control) return;

    // first tries remove the control
    naveg_remove_control(control->hw_id);

    if (control->hw_id < ENCODERS_COUNT)
    {
        display_control_add(control);
    }
    else
    {
        control->scroll_dir = 2;
        foot_control_add(control);     
    }
}

void naveg_remove_control(uint8_t hw_id)
{
    if (!g_initialized) return;

    if ((hw_id == 0) || (hw_id == 1)) display_control_rm(hw_id);
    else foot_control_rm(hw_id);
}

void naveg_inc_control(uint8_t display)
{
    if (!g_initialized) return;

    // if is in tool mode return
    if (display_has_tool_enabled(display)) return;

    control_t *control = g_controls[display];
    if (!control) return;

    if  (((control->properties == CONTROL_PROP_ENUMERATION) || (control->properties == CONTROL_PROP_SCALE_POINTS) 
    	|| (control->properties == CONTROL_PROP_REVERSE_ENUM)) && (control->scale_points_flag & CONTROL_PAGINATED))
    {
        // increments the step
        if (control->step < (control->steps - 2))
            control->step++;
        //we are at the end of our list ask for more data
        else
        {
        	//request new data, a new control we be assigned after
        	request_control_page(control, 1);

        	//since a new control is assigned we can return
        	return;
        }    	
    }
    else
    {
        // increments the step
        if (control->step < (control->steps - 1))
            control->step++;
        else
            return;
    }
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
    
    if  (((control->properties == CONTROL_PROP_ENUMERATION) || (control->properties == CONTROL_PROP_SCALE_POINTS) ||
     (control->properties == CONTROL_PROP_REVERSE_ENUM)) && (control->scale_points_flag & CONTROL_PAGINATED))
    {
        // decrements the step
        if (control->step > 1)
            control->step--;
        //we are at the end of our list ask for more data
        else
        {
        	//request new data, a new control we be assigned after
        	request_control_page(control, 0);

        	//since a new control is assigned we can return
        	return;
        }
    }
    else
    {
        // decrements the step
        if (control->step > 0)
            control->step--;
        else
            return;
    }
    // converts the step to absolute value
    step_to_value(control);

    // applies the control value
    control_set(display, control);
}

void naveg_set_control(uint8_t hw_id, float value)
{
    if (!g_initialized) return;


    uint8_t id;
    control_t *control = NULL;

    //encoder
    if (hw_id < ENCODERS_COUNT)
    {
        control = g_controls[hw_id];
        id = hw_id;
    }
    //button
    else if (hw_id < FOOTSWITCHES_ACTUATOR_COUNT + ENCODERS_COUNT)
    {
        control = g_foots[hw_id - ENCODERS_COUNT];
        id = hw_id - ENCODERS_COUNT;
    }

    if (control)
    {
        control->value = value;
        if (value < control->minimum)
            control->value = control->minimum;
        if (value > control->maximum)
            control->value = control->maximum;

        // updates the step value
        control->step =
            (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);


        //encoder
        if (hw_id < ENCODERS_COUNT)
        {
            if (!display_has_tool_enabled(hw_id))
            {
                screen_control(id, control);
            }
        }
        //button
        else if (hw_id < FOOTSWITCHES_ACTUATOR_COUNT + ENCODERS_COUNT)
        {
            // default state of led blink (no blink)
            led_blink(hardware_leds(control->hw_id - ENCODERS_COUNT), 0, 0);

            uint8_t i;
            switch (control->properties)
            {
            // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
            case CONTROL_PROP_TOGGLED:
                // updates the led
                if (control->value <= 0)
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BLACK);
                else
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TOGGLED_COLOR);

                // if is in tool mode break
                if (display_has_tool_enabled(get_display_by_id(control->hw_id - ENCODERS_COUNT, FOOT)))
                    break;

                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                             (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT));
                break;

            // trigger specification: http://lv2plug.in/ns/ext/port-props/#trigger
            case CONTROL_PROP_TRIGGER:
                // updates the led
                //check if its assigned to a trigger and if the button is released
                if (!control->scroll_dir)
                {
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BLACK);
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TRIGGER_COLOR); //TRIGGER_COLOR
                    return;
                }
                else if (control->scroll_dir == 2)
                {
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TRIGGER_COLOR);
                }
                else
                {
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BLACK);
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TRIGGER_PRESSED_COLOR);
                }

                // updates the led

                // if is in tool mode break
                if (display_has_tool_enabled(get_display_by_id(control->hw_id - ENCODERS_COUNT, FOOT)))
                    break;

                // updates the footer (a getto fix here, the screen.c file did not regognize the NULL pointer so it did not allign the text properly, TODO fix this)
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label, BYPASS_ON_FOOTER_TEXT);
                break;

            case CONTROL_PROP_TAP_TEMPO:
                // defines the led color
                led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TAP_TEMPO_COLOR);

                // convert the time unit
                uint16_t time_ms = (uint16_t)(convert_to_ms(control->unit, control->value) + 0.5);

                // setup the led blink
                if (time_ms > TAP_TEMPO_TIME_ON)
                {
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TAP_TEMPO_COLOR);
                     led_blink(hardware_leds(control->hw_id - ENCODERS_COUNT), TAP_TEMPO_TIME_ON, time_ms - TAP_TEMPO_TIME_ON);
                }
                else
                {
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), TAP_TEMPO_COLOR);
                    led_blink(hardware_leds(control->hw_id - ENCODERS_COUNT), time_ms / 2, time_ms / 2);
                }

                // calculates the maximum tap tempo value
                if (g_tap_tempo[control->hw_id - ENCODERS_COUNT].state == TT_INIT)
                {
                    uint32_t max;

                    // time unit (ms, s)
                    if (strcmp(control->unit, "ms") == 0 || strcmp(control->unit, "s") == 0)
                    {
                        max = (uint32_t)(convert_to_ms(control->unit, control->maximum) + 0.5);
                        //makes sure we enforce a proper timeout
                        if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                            max = TAP_TEMPO_DEFAULT_TIMEOUT;
                    }
                    // frequency unit (bpm, Hz)
                    else
                    {
                        //prevent division by 0 case
                        if (control->minimum == 0)
                            max = TAP_TEMPO_DEFAULT_TIMEOUT;
                        else
                            max = (uint32_t)(convert_to_ms(control->unit, control->minimum) + 0.5);
                        //makes sure we enforce a proper timeout
                        if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                            max = TAP_TEMPO_DEFAULT_TIMEOUT;
                    }

                    g_tap_tempo[control->hw_id - ENCODERS_COUNT].max = max;
                    g_tap_tempo[control->hw_id - ENCODERS_COUNT].state = TT_COUNTING;
                }

                // if is in tool mode break
                if (display_has_tool_enabled(get_display_by_id(control->hw_id - ENCODERS_COUNT, FOOT)))
                    break;

                // footer text composition
                char value_txt[32];

                //if unit=ms or unit=bpm -> use 0 decimal points
                if (strcasecmp(control->unit, "ms") == 0 || strcasecmp(control->unit, "bpm") == 0)
                    i = int_to_str(control->value, value_txt, sizeof(value_txt), 0);
                //if unit=s or unit=hz or unit=something else-> use 2 decimal points
                else
                    i = float_to_str(control->value, value_txt, sizeof(value_txt), 2);

                //add space to footer
                value_txt[i++] = ' ';
                strcpy(&value_txt[i], control->unit);

                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label, value_txt);
                break;

            case CONTROL_PROP_BYPASS:
                // updates the led
                if (control->value <= 0)
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BYPASS_COLOR);
                else
                    led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), BLACK);

                // if is in tool mode break
                if (display_has_tool_enabled(get_display_by_id(control->hw_id - ENCODERS_COUNT, FOOT)))
                    break;

                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                             (control->value ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT));
                break;

            case CONTROL_PROP_REVERSE_ENUM:
            case CONTROL_PROP_ENUMERATION:
            case CONTROL_PROP_SCALE_POINTS:
                // updates the led
                led_set_color(hardware_leds(control->hw_id - ENCODERS_COUNT), ENUMERATED_COLOR);

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
                if (display_has_tool_enabled(get_display_by_id(i, FOOT)))
                    break;

                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label, control->scale_points[i]->label);
                break;
            }
        }
    }
}

float naveg_get_control(uint8_t hw_id)
{
    if (!g_initialized) return 0.0;

    uint8_t display;
    control_t *control;

    control = search_control(hw_id, &display);
    if (control) return control->value;

    return 0.0;
}

void naveg_next_control(uint8_t display)
{
    if (!g_initialized) return;

    // if is in tool mode return
    if (display_has_tool_enabled(display)) return;

    //char buffer[128];
    char * buffer = (char *) MALLOC(128 * sizeof(char));
    uint8_t i;

    i = copy_command(buffer, CONTROL_NEXT_CMD);

    // inserts the hw id
    i += int_to_str(display, &buffer[i], 4, 0);
    buffer[i] = 0;

    comm_webgui_send(buffer, i);

    FREE(buffer);
}

void naveg_foot_change(uint8_t foot, uint8_t pressed)
{
    if (!g_initialized) return;

    // checks the foot id
    if (foot >= FOOTSWITCHES_COUNT) return;

    if (display_has_tool_enabled(get_display_by_id(foot, FOOT))) return;

    //detect a release action which we dont use right now for all actuator modes
    if ((!pressed)) return; 
    // && (g_foots[foot]->properties != CONTROL_PROP_TRIGGER)) return;

    // checks if the foot is used like bank function
    uint8_t bank_func_idx = bank_config_check(foot);

    if (bank_func_idx)
    {
        bank_config_update(bank_func_idx);
        return;
    }        

    // checks if there is assigned control
    if (g_foots[foot] == NULL) return;

    g_foots[foot]->scroll_dir = pressed;

    control_set(foot, g_foots[foot]);
}


void naveg_toggle_tool(uint8_t tool, uint8_t display)
{
    if (!g_initialized) return;
    static uint8_t banks_loaded = 0;
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
                if (!banks_loaded) request_banks_list(2);
                banks_loaded = 1;
                tool_off(DISPLAY_TOOL_SYSTEM_SUBMENU);
                display = 1;
                g_bp_state = BANKS_LIST;
                break;
            case DISPLAY_TOOL_TUNER:
                display_disable_all_tools(display);
                comm_webgui_send(TUNER_ON_CMD, strlen(TUNER_ON_CMD));
                break;
            case DISPLAY_TOOL_SYSTEM:
                screen_clear(1);
                display_disable_all_tools(DISPLAY_LEFT);
                display_disable_all_tools(DISPLAY_RIGHT);
                tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
        }

        // draws the tool
        tool_on(tool, display);
        screen_tool(tool, display);

        if (tool == DISPLAY_TOOL_SYSTEM)
        {
            //already enter banks menu on display 1
            menu_enter(1);
            g_current_main_menu = g_current_menu;
            g_current_main_item = g_current_item;
            menu_enter(0);
        }
    }
    // changes the display to control mode
    else
    {
        // action to do when the tool is disabled
        switch (tool)
        {
            case DISPLAY_TOOL_SYSTEM:
                display_disable_all_tools(display);
                display_disable_all_tools(1);
                g_update_cb = NULL;
                g_update_data = NULL;
                banks_loaded = 0;
                // force save gains when leave the menu
                system_save_gains_cb(NULL, MENU_EV_ENTER);
                break;
            case DISPLAY_TOOL_NAVIG:
                tool_off(DISPLAY_TOOL_NAVIG);
                tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
                return;
                break;

            case DISPLAY_TOOL_TUNER:
                comm_webgui_send(TUNER_OFF_CMD, strlen(TUNER_OFF_CMD));
                tool_off(DISPLAY_TOOL_TUNER);

                if (tool_is_on(DISPLAY_TOOL_SYSTEM))
                {
                   tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1); 
                   return;
                } 
                
                break;
        }

        //clear previous commands in the buffer
        comm_webgui_clear();

        control_t *control = g_controls[display];

        // draws the control
        screen_control(display, control);

        //draw the index (do not update values)
        naveg_set_index(0, display, 0, 0);

        // checks the function assigned to foot and update the footer
        if (bank_config_check(display)) bank_config_footer();
        else if (g_foots[display]) foot_control_add(g_foots[display]);
        else screen_footer(display, NULL, NULL);

        if (tool == DISPLAY_TOOL_SYSTEM)
        {
            screen_clear(1);
            control = g_controls[1];
            display = 1; 
     
            // draws the control
            screen_control(display, control);
    
            //draw the index (do not update values)
            naveg_set_index(0, display, 0, 0);

            // checks the function assigned to foot and update the footer
            if (bank_config_check(display)) bank_config_footer();
            else if (g_foots[display]) foot_control_add(g_foots[display]);
            else screen_footer(display, NULL, NULL);
        }
    }
}

uint8_t naveg_is_tool_mode(uint8_t display)
{
    return display_has_tool_enabled(display);
}

uint8_t naveg_tool_is_on(uint8_t tool)
{
    return tool_is_on(tool);
}

void naveg_set_banks(bp_list_t *bp_list)
{
    if (!g_initialized) return;

    g_banks = bp_list;
}

bp_list_t *naveg_get_banks(void)
{
    if (!g_initialized) return NULL;

    return (g_bp_state == BANKS_LIST) ? g_banks : g_naveg_pedalboards;
}

void naveg_bank_config(bank_config_t *bank_conf)
{
	 if (!g_initialized) return;

    // checks the function number
    if (bank_conf->function >= BANK_FUNC_AMOUNT) return;

    // checks the actuator type and actuator id
    if (bank_conf->hw_id > ENCODERS_COUNT + FOOTSWITCHES_COUNT) return;

    uint8_t i;
    for (i = 1; i < BANK_FUNC_AMOUNT; i++)
    {
        // checks if the function is already assigned to an actuator
        if (bank_conf->function != BANK_FUNC_NONE &&
            bank_conf->function == g_bank_functions[i].function &&
            bank_conf->hw_id != g_bank_functions[i].hw_id)
        {
            // default state of led blink (no blink)
            led_blink(hardware_leds(g_bank_functions[i].hw_id - ENCODERS_COUNT), 0, 0);

            // updates the screen and led
            led_set_color(hardware_leds(g_bank_functions[i].hw_id - ENCODERS_COUNT), BLACK);
            if (!display_has_tool_enabled(g_bank_functions[i].hw_id))
                screen_footer(g_bank_functions[i].hw_id - ENCODERS_COUNT, NULL, NULL);

            // removes the function
            g_bank_functions[i].function = BANK_FUNC_NONE;
            g_bank_functions[i].hw_id = 0xFF;
        }

        // checks if is replacing a function
        if ((g_bank_functions[i].function != bank_conf->function) &&
            (g_bank_functions[i].hw_id == bank_conf->hw_id))
        {
            // removes the function
            g_bank_functions[i].function = BANK_FUNC_NONE;
            g_bank_functions[i].hw_id = 0xFF;

            // if the new function is none, updates the screen and led
            if (bank_conf->function == BANK_FUNC_NONE)
            {
                // default state of led blink (no blink)
                led_blink(hardware_leds(bank_conf->hw_id - ENCODERS_COUNT), 0, 0);

                led_set_color(hardware_leds(bank_conf->hw_id - ENCODERS_COUNT), BLACK);
                if (!display_has_tool_enabled(bank_conf->hw_id))
                    screen_footer(bank_conf->hw_id - ENCODERS_COUNT, NULL, NULL);

                // checks if has control assigned in this foot
                // if yes, updates the footer screen
                if (g_foots[bank_conf->hw_id - ENCODERS_COUNT])
                    foot_control_add(g_foots[bank_conf->hw_id - ENCODERS_COUNT]);
            }
        }
    }

    // copies the bank function struct
    if (bank_conf->function != BANK_FUNC_NONE)
        memcpy(&g_bank_functions[bank_conf->function], bank_conf, sizeof(bank_config_t));

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
    if ((!g_initialized)&&(g_update_cb)) return;

    if (display_has_tool_enabled(display))
    {
        if (display == 0)
        {
            if (tool_is_on(DISPLAY_TOOL_TUNER) || dialog_active)
            {
                menu_enter(display);
            }
            else if (tool_is_on(DISPLAY_TOOL_NAVIG))
            {
                if (!naveg_ui_status())
                    system_banks_cb(g_current_main_item, MENU_EV_ENTER);
            }
            else if (g_current_item->desc->parent_id == ROOT_ID)
            {
                // calls the action callback
                if ((g_current_item->desc->action_cb)) g_current_item->desc->action_cb(g_current_item, MENU_EV_ENTER);
            }
        }
        else if (display == 1)
        {
            if (tool_is_on(DISPLAY_TOOL_TUNER)) tuner_enter();
            else if (tool_is_on(DISPLAY_TOOL_NAVIG)) bp_enter();
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM_SUBMENU))
            {
                if ((g_current_menu != g_menu) && (g_current_item->desc->id != ROOT_ID))  menu_enter(display);
            }
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
            //we dont use this knob in dialog mode
            if (dialog_active) return;

            //this is the top item, we cant go more up
            if (tool_is_on(DISPLAY_TOOL_NAVIG)) return;

            if (tool_is_on(DISPLAY_TOOL_TUNER))
            {
                    naveg_toggle_tool((tool_is_on(DISPLAY_TOOL_TUNER) ? DISPLAY_TOOL_TUNER : DISPLAY_TOOL_NAVIG), display);
                    tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
                    g_current_menu = g_current_main_menu;
                    g_current_item = g_current_main_item;
                    menu_up(display);
                    menu_enter(display);
            }
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM))
            {
                if (((g_current_menu == g_menu) || (g_current_item->desc->id == ROOT_ID)) && (dialog_active != 1))
                {
                    g_current_main_menu = g_current_menu;
                    g_current_main_item = g_current_item;
                }
                menu_up(display);
                if ((dialog_active != 1) || tool_is_on(DISPLAY_TOOL_NAVIG)) menu_enter(display);
            }
        }
        else if (display == 1)
        {
            if (tool_is_on(DISPLAY_TOOL_NAVIG)) bp_up();
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM_SUBMENU))
            {
                if ((g_current_menu != g_menu) || (g_current_item->desc->id != ROOT_ID)) menu_up(display);
            }
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
            //we dont use this knob in dialog mode
            if (dialog_active) return;

            if ( (tool_is_on(DISPLAY_TOOL_TUNER)) || (tool_is_on(DISPLAY_TOOL_NAVIG)) )
            {
                    naveg_toggle_tool((tool_is_on(DISPLAY_TOOL_TUNER) ? DISPLAY_TOOL_TUNER : DISPLAY_TOOL_NAVIG), display);
                    tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
                    g_current_menu = g_current_main_menu;
                    g_current_item = g_current_main_item;
                    menu_down(display);
                    menu_enter(display);
            }
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM))
            {
                if (((g_current_menu == g_menu) || (g_current_item->desc->id == ROOT_ID)) && (dialog_active != 1))
                {
                    g_current_main_menu = g_current_menu;
                    g_current_main_item = g_current_item;
                }
                menu_down(display);
                if (dialog_active != 1) menu_enter(display);
            }
        }
        else if (display == 1)
        {
            if (tool_is_on(DISPLAY_TOOL_NAVIG)) bp_down();
            else if (tool_is_on(DISPLAY_TOOL_SYSTEM_SUBMENU))
            {
                if ((g_current_menu != g_menu) || (g_current_item->desc->id != ROOT_ID)) menu_down(display);
            }
        }
    }
}

void naveg_reset_menu(void)
{
    if (!g_initialized) return;

    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;
    g_current_main_menu = g_menu;
    g_current_main_item = g_menu->first_child->data;
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
    static node_t *dummy_menu = NULL;
    static menu_desc_t desc = {NULL, MENU_CONFIRM2, DIALOG_ID, DIALOG_ID, NULL, 0};
    
    if (!dummy_menu)
    {
        menu_item_t *item;
        item = (menu_item_t *) MALLOC(sizeof(menu_item_t));
        item->data.hover = 0;
        item->data.selected = 0xFF;
        item->data.list_count = 2;
        item->data.list = NULL;
        item->data.popup_content = msg;
        item->data.popup_header = "selftest";
        item->desc = &desc;
        item->name = NULL;
        dummy_menu = node_create(item);
    }

    display_disable_all_tools(DISPLAY_LEFT);
    tool_on(DISPLAY_TOOL_SYSTEM, DISPLAY_LEFT);
    tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, DISPLAY_RIGHT);

    g_current_menu = dummy_menu;
    g_current_item = dummy_menu->data;

    screen_system_menu(g_current_item);

    dialog_active = 1;

    if (xSemaphoreTake(g_dialog_sem, portMAX_DELAY) == pdTRUE)
    {
        dialog_active = 0;
        display_disable_all_tools(DISPLAY_LEFT);
        display_disable_all_tools(DISPLAY_RIGHT);

        g_update_cb = NULL;
        g_update_data = NULL;

        g_current_main_menu = g_menu;
        g_current_main_item = g_menu->first_child->data;
        reset_menu_hover(g_menu);

        screen_clear(DISPLAY_RIGHT);

        return g_current_item->data.hover;
    }
    //we can never get here, portMAX_DELAY means wait indefinatly I'm adding this to remove a compiler warning
    else
    {
        //ERROR
        return -1; 
    }
}

uint8_t naveg_ui_status(void)
{
    return g_ui_connected;
}

uint8_t naveg_dialog_status(void)
{
    return dialog_active;
}

char* naveg_get_current_pb_name(void)
{
    return g_banks->names[g_banks->hover];
}

void naveg_set_index(uint8_t update, uint8_t display, uint8_t new_index, uint8_t new_index_count)
{
    static uint8_t index[ENCODERS_COUNT] = {};
    static uint8_t index_count[ENCODERS_COUNT] = {};

    if (update)
    {
        index[display] = new_index;
        index_count[display] = new_index_count;
    }

    if ((g_controls[display]) && !display_has_tool_enabled(display))
    {
        screen_controls_index(display, index[display], index_count[display]);
    }

    return;
}

uint8_t naveg_tap_tempo_status(uint8_t id)
{
    if (g_tap_tempo[id].state == TT_INIT) return 0;
    else return 1;
}

uint8_t naveg_banks_mode_pb(void)
{
    return g_bp_state;
}

void naveg_settings_refresh(uint8_t display_id)
{
    display_id ? screen_system_menu(g_current_item) : screen_system_menu(g_current_main_item);
}

void naveg_menu_refresh(uint8_t display_id)
{
    node_t *node = display_id ? g_current_menu : g_current_main_menu;

    //updates all items in a menu
    for (node = node->first_child; node; node = node->next)
    {
        // gets the menu item
        menu_item_t *item = node->data;

        // calls the action callback
        if ((item->desc->action_cb)) item->desc->action_cb(item, MENU_EV_NONE);
    }
    naveg_settings_refresh(display_id);
}

//the menu refresh is to slow for the gains so this one is added that only updates the set value.
void naveg_update_gain(uint8_t display_id, uint8_t update_id, float value, float min, float max)
{
    node_t *node = display_id ? g_current_menu : g_current_main_menu;

    //updates all items in a menu
    for (node = node->first_child; node; node = node->next)
    {
        // gets the menu item
        menu_item_t *item = node->data;

        // updates the value
        if ((item->desc->id == update_id))
        {
            item->data.value = value;

            char str_buf[8];
            float value_bfr = MAP(value, min, max, 0, 100);
            int_to_str(value_bfr, str_buf, sizeof(str_buf), 0);
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
        }
    }
}

void naveg_menu_item_changed_cb(uint8_t item_ID, uint8_t value)
{
    //set value in system.c
    system_update_menu_value(item_ID, value);

    //are we inside the menu? if so we need to update
    if (tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
        //menu update for left or right? or both? 

        //if left menu item, no need to change right
        if((item_ID == TUNER_ID) || (item_ID == TEMPO_ID) || (item_ID == BANKS_ID))
        {
            naveg_menu_refresh(DISPLAY_LEFT);
            return;
        }
        
        //otherwise update right for sure
        else 
        {
            if (!tool_is_on(DISPLAY_TOOL_TUNER) && !tool_is_on(DISPLAY_TOOL_NAVIG))
            {
                naveg_menu_refresh(DISPLAY_RIGHT);
            }

            //for bypass, left might change as well, we update just in case
            if (((item_ID - BYPASS_ID) < 10) && ((item_ID - BYPASS_ID) > 0) )
            {
               naveg_menu_refresh(DISPLAY_LEFT); 
            }
        } 
    }

    //when we are not in the menu, did we change the master volume link?
        //TODO update the master volume link widget
}
