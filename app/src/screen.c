
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "screen.h"
#include "utils.h"
#include "glcd.h"
#include "glcd_widget.h"
#include "naveg.h"
#include "hardware.h"

#include <string.h>


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

static peakmeter_t g_peakmeter;
static tuner_t g_tuner = {0, NULL, 0, 1};


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


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/


// TODO: capitalize strings, truncate, replace underline by space

void screen_clear(uint8_t display)
{
    glcd_clear(display, GLCD_WHITE);
}

void screen_control(uint8_t display, control_t *control)
{
    if (!control)
    {
        glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 51, GLCD_WHITE);

        char text[16];
        strcpy(text, "ROTARY #");
        text[8] = display + '1';
        text[9] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = alterebro24;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 0;
        title.right_margin = 0;
        title.text = text;
        title.align = ALIGN_CENTER_NONE;
        title.y = 25 - (alterebro24[FONT_HEIGHT] / 2);
        widget_textbox(display, &title);
        return;
    }

    // clear the title area
    glcd_rect_fill(display, 0, 0, 113, 15, GLCD_WHITE);

    // control title label
    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = alterebro24;
    title.top_margin = 2;
    title.bottom_margin = 0;
    title.left_margin = 0;
    title.right_margin = 0;
    title.text = control->label;
    title.align = ALIGN_LEFT_TOP;
    widget_textbox(display, &title);

    // horizontal title line
    glcd_hline(display, 0, 16, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    // graph type control
    if (control->properties == CONTROL_PROP_LINEAR ||
        control->properties == CONTROL_PROP_LOGARITHMIC)
    {
        const char *unit;
        unit = (strcmp(control->unit, "none") == 0 ? NULL : control->unit);

        graph_t graph;
        graph.x = 0;
        graph.y = 18;
        graph.color = GLCD_BLACK;
        graph.font = alterebro24;
        graph.min = control->minimum;
        graph.max = control->maximum;
        graph.value = control->value;
        graph.unit = unit;
        graph.type = control->properties;
        widget_graph(display, &graph);
    }

    // list type control
    else if (control->properties == CONTROL_PROP_ENUMERATION)
    {
        char **labels_list;
        labels_list = MALLOC(control->scale_points_count * sizeof(char *));

        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
        {
            labels_list[i] = control->scale_points[i]->label;
        }

        listbox_t list;
        list.x = 0;
        list.y = 17;
        list.width = 128;
        list.height = 33;
        list.color = GLCD_BLACK;
        list.font = alterebro15;
        list.selected = control->step;
        list.count = control->scale_points_count;
        list.list = labels_list;
        list.line_space = 1;
        list.line_top_margin = 1;
        list.line_bottom_margin = 0;
        list.text_left_margin = 1;
        widget_listbox2(display, &list);

        FREE(labels_list);
    }
}

void screen_controls_index(uint8_t display, uint8_t current, uint8_t max)
{
    char str_current[4], str_max[4];
    int_to_str(current, str_current, sizeof(str_current), 2);
    int_to_str(max, str_max, sizeof(str_max), 2);

    if (max == 0) return;

    // vertical line index separator
    glcd_vline(display, 114, 0, 16, GLCD_BLACK_WHITE);

    // draws the max field
    textbox_t index;
    index.color = GLCD_BLACK;
    index.mode = TEXT_SINGLE_LINE;
    index.font = System5x7;
    index.bottom_margin = 0;
    index.left_margin = 0;
    index.right_margin = 1;
    index.align = ALIGN_RIGHT_TOP;
    index.top_margin = 8;
    index.text = str_max;
    widget_textbox(display, &index);

    // draws the current field
    index.top_margin = 0;
    index.text = str_current;
    widget_textbox(display, &index);
}

void screen_footer(uint8_t display, const char *name, const char *value)
{
    // horizontal footer line
    glcd_hline(display, 0, 51, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    // clear the footer area
    glcd_rect_fill(display, 0, 52, DISPLAY_WIDTH, DISPLAY_HEIGHT-52, GLCD_WHITE);

    if (name == NULL && value == NULL)
    {
        char text[8];
        strcpy(text, "FOOT #");
        text[6] = display + '1';
        text[7] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = alterebro24;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 0;
        title.right_margin = 0;
        title.text = text;
        title.align = ALIGN_CENTER_NONE;
        title.y = 52;
        widget_textbox(display, &title);
        return;
    }

    // draws the name field
    textbox_t footer;
    footer.color = GLCD_BLACK;
    footer.mode = TEXT_SINGLE_LINE;
    footer.font = alterebro24;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 0;
    footer.right_margin = 0;
    footer.text = name;
    footer.y = 52;
    footer.align = ALIGN_LEFT_NONE;
    widget_textbox(display, &footer);

    // draws the value field
    footer.right_margin = 4;
    footer.text = value;
    footer.align = ALIGN_RIGHT_NONE;
    widget_textbox(display, &footer);
}

void screen_tool(uint8_t display, uint8_t tool)
{
    bp_list_t *bp_list;

    switch (tool)
    {
        case TOOL_SYSTEM:
            naveg_reset_menu();
            naveg_enter(SYSTEM_DISPLAY);
            break;

        case TOOL_TUNER:
            g_tuner.frequency = 0.0;
            g_tuner.note = "?";
            g_tuner.cents = 0;
            widget_tuner(display, &g_tuner);
            break;

        case TOOL_PEAKMETER:
            g_peakmeter.value1 = -30.0;
            g_peakmeter.value2 = -30.0;
            g_peakmeter.value3 = -30.0;
            g_peakmeter.value4 = -30.0;
            widget_peakmeter(display, &g_peakmeter);
            break;

        case TOOL_NAVEG:
            bp_list = naveg_get_banks();
            if (bp_list) bp_list->hover = bp_list->selected;
            screen_bp_list("BANKS", bp_list);
            break;
    }
}

void screen_bp_list(const char *title, bp_list_t *list)
{
    listbox_t list_box;
    textbox_t title_box, empty;

    // clears the title
    glcd_rect_fill(NAVEG_DISPLAY, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    // draws the title
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = alterebro15;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.text = title;
    title_box.align = ALIGN_LEFT_TOP;
    widget_textbox(NAVEG_DISPLAY, &title_box);

    // title line separator
    glcd_hline(NAVEG_DISPLAY, 0, 9, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    // draws the list
    if (list)
    {
        uint8_t count = strarr_length(list->names);
        list_box.x = 0;
        list_box.y = 11;
        list_box.width = 128;
        list_box.height = 53;
        list_box.color = GLCD_BLACK;
        list_box.hover = list->hover;
        list_box.selected = list->selected;
        list_box.count = count;
        list_box.list = list->names;
        list_box.font = alterebro15;
        list_box.line_space = 1;
        list_box.line_top_margin = 1;
        list_box.line_bottom_margin = 0;
        list_box.text_left_margin = 2;
        widget_listbox(NAVEG_DISPLAY, &list_box);
    }
    else
    {
        empty.color = GLCD_BLACK;
        empty.mode = TEXT_SINGLE_LINE;
        empty.font = alterebro24;
        empty.top_margin = 0;
        empty.bottom_margin = 0;
        empty.left_margin = 0;
        empty.right_margin = 0;
        empty.text = "NO BANKS";
        empty.align = ALIGN_CENTER_MIDDLE;
        widget_textbox(NAVEG_DISPLAY, &empty);
    }
}

void screen_system_menu(menu_item_t *item)
{
    static menu_item_t *last_item;

    // clears the title
    glcd_rect_fill(SYSTEM_DISPLAY, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    // draws the title
    textbox_t title_box;
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = alterebro15;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.align = ALIGN_LEFT_TOP;
    title_box.text = item->name;
    if (item->desc->type == MENU_NONE || item->desc->type == MENU_ON_OFF)
        title_box.text = last_item->name;
    widget_textbox(SYSTEM_DISPLAY, &title_box);

    // title line separator
    glcd_hline(SYSTEM_DISPLAY, 0, 9, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    glcd_hline(SYSTEM_DISPLAY, 0, 10, DISPLAY_WIDTH, GLCD_WHITE);

    // menu list
    listbox_t list;
    list.x = 0;
    list.y = 11;
    list.width = 128;
    list.height = 53;
    list.color = GLCD_BLACK;
    list.font = alterebro15;
    list.line_space = 1;
    list.line_top_margin = 1;
    list.line_bottom_margin = 0;
    list.text_left_margin = 2;

    // popup
    popup_t popup;
    popup.x = 10;
    popup.y = 5;
    popup.width = DISPLAY_WIDTH - 20;
    popup.height = DISPLAY_HEIGHT - 10;
    popup.font = alterebro15;

    switch (item->desc->type)
    {
        case MENU_LIST:
        case MENU_SELECT:
            list.hover = item->data.hover;
            list.selected = item->data.selected;
            list.count = item->data.list_count;
            list.list = item->data.list;
            widget_listbox(SYSTEM_DISPLAY, &list);
            last_item = item;
            break;

        case MENU_CONFIRM:
        case MENU_CANCEL:
            popup.type = (item->desc->type == MENU_CONFIRM ? YES_NO : CANCEL_ONLY);
            popup.title = "Popup Title";
            popup.content = "popup content";
            popup.button_selected = item->data.hover;
            widget_popup(SYSTEM_DISPLAY, &popup);
            break;

        case MENU_NONE:
        case MENU_RETURN:
            list.hover = last_item->data.hover;
            list.selected = last_item->data.selected;
            list.count = last_item->data.list_count;
            list.list = last_item->data.list;
            widget_listbox(SYSTEM_DISPLAY, &list);
            break;

        case MENU_ON_OFF:
            strcpy(item->name, item->desc->name);
            strcat(item->name, (item->data.hover ? " ON" : "OFF"));

            list.hover = last_item->data.hover;
            list.selected = last_item->data.selected;
            list.count = last_item->data.list_count;
            list.list = last_item->data.list;
            widget_listbox(SYSTEM_DISPLAY, &list);
            break;
    }
}

void screen_peakmeter(uint8_t peakmeter, float value)
{
    switch (peakmeter)
    {
        case 0:
            g_peakmeter.value1 = value;
            break;

        case 1:
            g_peakmeter.value2 = value;
            break;

        case 2:
            g_peakmeter.value3 = value;
            break;

        case 3:
            g_peakmeter.value4 = value;
            break;
    }

    // checks if peakmeter is enable and update it
    if (naveg_is_tool_mode(PEAKMETER_DISPLAY))
        widget_peakmeter(PEAKMETER_DISPLAY, &g_peakmeter);
}

void screen_tuner(float frequency, char *note, int8_t cents)
{
    g_tuner.frequency = frequency;
    g_tuner.note = note;
    g_tuner.cents = cents;

    // checks if tuner is enable and update it
    if (naveg_is_tool_mode(TUNER_DISPLAY))
        widget_tuner(TUNER_DISPLAY, &g_tuner);
}

void screen_clipmeter(uint8_t display, uint8_t happened_now)
{
    static uint8_t check_timeout[GLCD_COUNT];
    static uint32_t last_time[GLCD_COUNT];
    uint32_t time_elapsed;

    if (display == 0xFF)
    {
        uint8_t i;
        for (i = 0; i < GLCD_COUNT; i++)
            screen_clipmeter(i, 0);
    }

    if (happened_now)
    {
        glcd_set_pixel(display, 126, 63, GLCD_BLACK);
        last_time[display] = hardware_time_stamp();
        check_timeout[display] = 1;
    }

    time_elapsed = hardware_time_stamp() - last_time[display];

    if (check_timeout[display] && time_elapsed > CLIPMETER_TIMEOUT)
    {
        glcd_set_pixel(display, 126, 63, GLCD_WHITE);
        check_timeout[display] = 0;
    }
}

void screen_xrun(uint8_t happened_now)
{
    static uint8_t check_timeout;
    static uint32_t last_time;
    uint32_t time_elapsed;

    if (happened_now)
    {
        glcd_set_pixel(XRUN_ICON_DISPLAY, 127, 63, GLCD_BLACK);
        last_time = hardware_time_stamp();
        check_timeout = 1;
    }

    time_elapsed = hardware_time_stamp() - last_time;

    if (check_timeout && time_elapsed > XRUN_TIMEOUT)
    {
        glcd_set_pixel(XRUN_ICON_DISPLAY, 127, 63, GLCD_WHITE);
        check_timeout = 0;
    }
}
