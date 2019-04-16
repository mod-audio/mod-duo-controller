
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


//TODO, TESTING PLEASE REMOVE ME LATER
#include "led.h"
//#include "hardware.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define FOOTER_NAME_WIDTH       ((DISPLAY_WIDTH * 50)/100)
#define FOOTER_VALUE_WIDTH      (DISPLAY_WIDTH - FOOTER_NAME_WIDTH)


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

void screen_clear(uint8_t display_id)
{
    glcd_clear(hardware_glcds(display_id), GLCD_WHITE);
}

void screen_control(uint8_t display_id, control_t *control)
{
    glcd_t *display = hardware_glcds(display_id);

    if (!control)
    {
        glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 51, GLCD_WHITE);

        char text[sizeof(SCREEN_ROTARY_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_ROTARY_DEFAULT_NAME);
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)-1] = display_id + '1';
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = alterebro24;
        title.height = 0;
        title.width = 0;
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
    title.height = 0;
    title.width = 113;
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
        graph.type =
            (control->properties == CONTROL_PROP_LOGARITHMIC ? GRAPH_TYPE_LOG : GRAPH_TYPE_LINEAR);
        widget_graph(display, &graph);
    }

    // integer type control
    else if (control->properties == CONTROL_PROP_INTEGER)
    {
        textbox_t value, unit;
        char value_str[32];

        // clear the text area
        glcd_rect_fill(display, 0, 17, 128, 33, GLCD_WHITE);

        // draws the value
        int_to_str(control->value, value_str, sizeof(value_str), 0);
        value.color = GLCD_BLACK;
        value.mode = TEXT_SINGLE_LINE;
        value.font = alterebro49;
        value.top_margin = 3;
        value.bottom_margin = 0;
        value.left_margin = 0;
        value.right_margin = (strcmp(control->unit, "none") == 0 ? 0 : 4);
        value.height = 0;
        value.width = 0;
        value.text = value_str;
        value.align = ALIGN_CENTER_MIDDLE;
        widget_textbox(display, &value);

        // draws the unit
        if (strcmp(control->unit, "none") != 0)
        {
            unit.color = GLCD_BLACK;
            unit.mode = TEXT_SINGLE_LINE;
            unit.font = alterebro24;
            unit.top_margin = 0;
            unit.bottom_margin = 0;
            unit.left_margin = 0;
            unit.right_margin = 0;
            unit.height = 0;
            unit.width = 0;
            unit.text = control->unit;
            unit.align = ALIGN_NONE_NONE;
            unit.x = value.x + value.width + 2;
            unit.y = value.y + value.height - unit.font[FONT_HEIGHT];
            widget_textbox(display, &unit);
        }
    }

    // list type control
    else if (control->properties == CONTROL_PROP_ENUMERATION ||
             control->properties == CONTROL_PROP_SCALE_POINTS)
    {
        static char *labels_list[128];

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
    }
}

void screen_controls_index(uint8_t display_id, uint8_t current, uint8_t max)
{
    char str_current[4], str_max[4];
    int_to_str(current, str_current, sizeof(str_current), 2);
    int_to_str(max, str_max, sizeof(str_max), 2);

    if (max == 0) return;

    glcd_t *display = hardware_glcds(display_id);

    // vertical line index separator
    glcd_vline(display, 114, 0, 16, GLCD_BLACK_WHITE);

    // draws the max field
    textbox_t index;
    index.color = GLCD_BLACK;
    index.mode = TEXT_SINGLE_LINE;
    index.font = System5x7;
    index.height = 0;
    index.width = 0;
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

void screen_footer(uint8_t display_id, const char *name, const char *value)
{
    glcd_t *display = hardware_glcds(display_id);

    // horizontal footer line
    glcd_hline(display, 0, 51, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    // clear the footer area
    glcd_rect_fill(display, 0, 52, DISPLAY_WIDTH, DISPLAY_HEIGHT-52, GLCD_WHITE);

    if (name == NULL && value == NULL)
    {
        char text[sizeof(SCREEN_FOOT_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_FOOT_DEFAULT_NAME);
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)-1] = display_id + '1';
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = alterebro24;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 0;
        title.right_margin = 0;
        title.height = 0;
        title.width = 0;
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
    footer.height = 0;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 0;
    footer.width = (value == NULL ? DISPLAY_WIDTH : FOOTER_NAME_WIDTH);
    footer.right_margin = 0;
    footer.text = name;
    footer.y = 52;
    footer.align = ALIGN_LEFT_NONE;
    widget_textbox(display, &footer);

    // draws the value field
    footer.width = FOOTER_VALUE_WIDTH;
    footer.right_margin = 0;
    footer.text = value;
    footer.align = ALIGN_RIGHT_NONE;
    widget_textbox(display, &footer);
}

void screen_tool(uint8_t tool, uint8_t display_id)
{
    bp_list_t *bp_list;
    glcd_t *display = hardware_glcds(display_id);

    switch (tool)
    {
        case DISPLAY_TOOL_SYSTEM:
            naveg_reset_menu();
            naveg_enter(DISPLAY_TOOL_SYSTEM);
            break;

        case DISPLAY_TOOL_TUNER:
            g_tuner.frequency = 0.0;
            g_tuner.note = "?";
            g_tuner.cents = 0;
            widget_tuner(display, &g_tuner);
            break;

        case DISPLAY_TOOL_NAVIG:
            bp_list = naveg_get_banks();
            if (bp_list && bp_list->selected == 0)
                bp_list->selected = 1;
            //if we already have a bank selected we enter that bank automaticly 
            else {
                bp_list->hover = bp_list->selected; 
                naveg_enter(1);
                return;
                 }
            screen_bp_list("BANKS", bp_list);
            break;
    }
}

void screen_bp_list(const char *title, bp_list_t *list)
{
    listbox_t list_box;
    textbox_t title_box;

    glcd_t *display = hardware_glcds(1);

    // clears the title
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    // draws the title
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = SMfont;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.height = 0;
    title_box.width = 0;
    title_box.text = title;
    title_box.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &title_box);

    // title line separator
    glcd_hline(display, 0, 9, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

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
        list_box.font = SMfont;
        list_box.line_space = 2;
        list_box.line_top_margin = 1;
        list_box.line_bottom_margin = 1;
        list_box.text_left_margin = 2;
        widget_listbox(display, &list_box);
    }
    else
    {
        if (naveg_ui_status())
        {
            textbox_t message;
            message.color = GLCD_BLACK;
            message.mode = TEXT_SINGLE_LINE;
            message.font = alterebro24;
            message.top_margin = 0;
            message.bottom_margin = 0;
            message.left_margin = 0;
            message.right_margin = 0;
            message.height = 0;
            message.width = 0;
            message.text = "To access banks here please disconnect from the graphical interface";
            message.align = ALIGN_CENTER_MIDDLE;
            widget_textbox(display, &message);
        }
    }
}

void screen_system_menu(menu_item_t *item)
{
    // return if system is disabled
    if (!naveg_is_tool_mode(DISPLAY_TOOL_SYSTEM))
        return;

    static menu_item_t *last_item;

    glcd_t *display;
    if (item->desc->id == ROOT_ID)
    {    
        display = hardware_glcds(DISPLAY_TOOL_SYSTEM);
    }
    else if (item->desc->id == TUNER_ID) return;
    else
    {
      display = hardware_glcds(1);  
    }

    // clear screen
    glcd_clear(display, GLCD_WHITE);
  
    // draws the title
    textbox_t title_box;
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = SMfont;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.height = 0;
    title_box.width = 0;
    title_box.align = ALIGN_CENTER_TOP;
    title_box.text = item->name;
    if ((item->desc->type == MENU_NONE) || (item->desc->type == MENU_TOGGLE))
    {
        title_box.text = last_item->name;
        if (title_box.text[strlen(item->name) - 1] == ']') title_box.text = last_item->desc->name; 
    }
    else if (title_box.text[strlen(item->name) - 1] == ']')
    {   
       title_box.text = item->desc->name;
    }
    widget_textbox(display, &title_box);

    // title line separator
    glcd_hline(display, 0, 9, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    glcd_hline(display, 0, 10, DISPLAY_WIDTH, GLCD_WHITE);

    // menu list
    listbox_t list;
    list.x = 0;
    list.y = 11;
    list.width = 128;
    list.height = 53;
    list.color = GLCD_BLACK;
    list.font = SMfont;
    list.line_space = 2;
    list.line_top_margin = 1;
    list.line_bottom_margin = 1;
    list.text_left_margin = 2;

    // popup
    popup_t popup;
    popup.x = 0;
    popup.y = 0;
    popup.width = DISPLAY_WIDTH;
    popup.height = DISPLAY_HEIGHT;
    popup.font = SMfont;

    switch (item->desc->type)
    {
        case MENU_LIST:
        case MENU_SELECT:
       
            list.hover = item->data.hover;
            list.selected = item->data.selected;
            list.count = item->data.list_count;
            list.list = item->data.list;
            widget_listbox(display, &list);
            if ((last_item->desc->id != TEMPO_ID)||(item->desc->id == SYSTEM_ID)||(item->desc->id == BYPASS_ID)) last_item = item;
            break;

        case MENU_CONFIRM:
        case MENU_CONFIRM2:
        case MENU_CANCEL:
        case MENU_OK:
            if (item->desc->type == MENU_CANCEL) popup.type = CANCEL_ONLY;
            else if (item->desc->type == MENU_OK) popup.type = OK_ONLY;
            else popup.type = YES_NO;
            popup.title = item->data.popup_header;
            popup.content = item->data.popup_content;
            popup.button_selected = item->data.hover;
            widget_popup(display, &popup);
            break;

        case MENU_TOGGLE:
            if (item->desc->parent_id == PROFILES_ID ||  item->desc->id == EXP_CV_INP || item->desc->id == HP_CV_OUTP)
            {    
                popup.type = YES_NO;
                popup.title = item->data.popup_header;
                popup.content = item->data.popup_content;
                popup.button_selected = item->data.hover;
                widget_popup(display, &popup);
            }
            else 
            {
                list.hover = last_item->data.hover;
                list.selected = last_item->data.selected;
                list.count = last_item->data.list_count;
                list.list = last_item->data.list;
                widget_listbox(display, &list);
            }
            break;

        case MENU_VOL:
        case MENU_SET:
        case MENU_NONE:
        case MENU_RETURN:
            list.hover = last_item->data.hover;
            list.selected = last_item->data.selected;
            list.count = last_item->data.list_count;
            list.list = last_item->data.list;
            widget_listbox(display, &list);
            break;
    }
}

void screen_tuner(float frequency, char *note, int8_t cents)
{
    g_tuner.frequency = frequency;
    g_tuner.note = note;
    g_tuner.cents = cents;

    // checks if tuner is enable and update it
    if (naveg_is_tool_mode(DISPLAY_TOOL_TUNER))
        widget_tuner(hardware_glcds(1), &g_tuner);
}

void screen_tuner_input(uint8_t input)
{
    g_tuner.input = input;

    // checks if tuner is enable and update it
    if (naveg_is_tool_mode(DISPLAY_TOOL_TUNER))
        widget_tuner(hardware_glcds(1), &g_tuner);
}

void screen_image(uint8_t display, const uint8_t *image)
{
    glcd_draw_image(hardware_glcds(display), 0, 0, image, GLCD_BLACK);
}
