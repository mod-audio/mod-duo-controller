
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
#include "images.h"
#include <string.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define FOOTER_NAME_WIDTH       50
#define FOOTER_VALUE_WIDTH      (DISPLAY_WIDTH - FOOTER_NAME_WIDTH - 10)

enum {BANKS_LIST, PEDALBOARD_LIST};

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

void screen_encoder(uint8_t display_id, control_t *control)
{    
    glcd_t *display = hardware_glcds(display_id);

    //TOOO NOT TRIGGERED HERE DEBUG
    screen_top_info(NULL, display_id, 1);

    //no control? 
    if (!control)
    {
        glcd_rect_fill(display, 0, 9, DISPLAY_WIDTH, 41, GLCD_WHITE);

        //draw back the title line on 16
        glcd_hline(display, 0, 16, DISPLAY_WIDTH, GLCD_BLACK);

        char text[sizeof(SCREEN_ROTARY_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_ROTARY_DEFAULT_NAME);
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)-1] = display_id + '1';
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = Terminal7x8;
        title.height = 0;
        title.width = 0;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 0;
        title.right_margin = 0;
        title.text = text;
        title.align = ALIGN_CENTER_NONE;
        title.y = 33 - (Terminal7x8[FONT_HEIGHT] / 2);
        widget_textbox(display, &title);
        return;
    }
    else 
    {
        //clear the area
        glcd_rect_fill(display, 0, 9, DISPLAY_WIDTH - 11, 15, GLCD_WHITE);
        glcd_rect_fill(display, 0, 24, DISPLAY_WIDTH, 26, GLCD_WHITE);

        //draw back the title line on 16
        glcd_hline(display, 0, 16, DISPLAY_WIDTH, GLCD_BLACK);
        
        // control title label
        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = Terminal5x7;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 2;
        title.right_margin = 14;
        title.height = 0;
        title.width = 0;
        
        title.y = 13;

        uint8_t char_cnt_name = strlen(control->label);
        //limit if to big
        if (char_cnt_name >= 19) char_cnt_name = 19;
        
        char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
        strncpy(title_str_bfr, control->label, char_cnt_name);
        title_str_bfr[char_cnt_name] = '\0';
        title.text = title_str_bfr;
        title.align = ALIGN_NONE_NONE;

        // clear the name area
        glcd_rect_fill(display, (((char_cnt_name > 16)?DISPLAY_WIDTH-9:DISPLAY_WIDTH) /2) - char_cnt_name*3-1, 12, ((6*char_cnt_name) +3), 9, GLCD_WHITE);

        title.x = (((char_cnt_name > 16)?DISPLAY_WIDTH-9:DISPLAY_WIDTH) /2) - char_cnt_name*3 + 1;
        widget_textbox(display, &title);

        // invert the name area
        glcd_rect_invert(display, (((char_cnt_name > 16)?DISPLAY_WIDTH-9:DISPLAY_WIDTH) /2) - char_cnt_name*3-1, 12, ((6*char_cnt_name) +3), 9);

        FREE(title_str_bfr);
    }

    // bar type control
    if (control->properties == CONTROL_PROP_LINEAR ||
        control->properties == CONTROL_PROP_LOGARITHMIC || 
        control->properties == CONTROL_PROP_INTEGER)
    {
        bar_t bar;
        bar.x = 4;
        bar.y = 23;
        bar.width = 116;
        bar.height = 14;
        bar.color = GLCD_BLACK;
        bar.step = control->step;
        bar.steps = control->steps - 1;

        char value_str[10] = {0};
        if ((control->value > 99.9) || (control->properties == CONTROL_PROP_INTEGER))
        {
            int_to_str(control->value, value_str, sizeof(value_str), 0);
        }
        else 
        {
            float_to_str((control->value), value_str, sizeof(value_str), 2);
        }

        //convert unit
        char tmp_unit[10];
        char tmp_unit_check[1] = "";
        strcpy(tmp_unit, control->unit);

        char *str_bfr = MALLOC(strlen(value_str)+strlen(control->unit)+1);
        strcpy(str_bfr, value_str);

        if (strcmp(tmp_unit_check, tmp_unit) == 0)
        {
            str_bfr[strlen(value_str)] = '\0';
        }
        else
        {
            strcat(str_bfr, " ");
            strcat(str_bfr, tmp_unit);
            str_bfr[strlen(value_str)+strlen(control->unit)+1] = '\0';
        }
        bar.label = str_bfr;

        widget_bar(display, &bar);
        FREE(str_bfr);
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
        list.y = 24;
        list.width = DISPLAY_WIDTH;
        list.height = 25;
        list.color = GLCD_BLACK;
        list.font = Terminal3x5;
        list.font_highlight = Terminal5x7;
        list.selected = control->step;
        list.count = control->scale_points_count;
        list.list = labels_list;
        list.line_space = 1;
        list.line_top_margin = 1;
        list.line_bottom_margin = 1;
        list.text_left_margin = 1;
        widget_listbox4(display, &list);
    }
    else if (control->properties == CONTROL_PROP_TOGGLED ||
             control->properties == CONTROL_PROP_BYPASS)
    {
        toggle_t toggle;
        toggle.x = 20;
        toggle.y = 26;
        toggle.width = 88;
        toggle.height = 21;
        toggle.color = GLCD_BLACK;
        toggle.value = (control->properties == CONTROL_PROP_TOGGLED)?control->value:!control->value;;
        widget_toggle(display, &toggle);
    }
}

void screen_controls_index(uint8_t display_id, uint8_t current, uint8_t max)
{
    char str_current[4], str_max[4];
    int_to_str(current, str_current, sizeof(str_current), 2);
    int_to_str(max, str_max, sizeof(str_max), 2);

    if (max == 0) return;

    glcd_t *display = hardware_glcds(display_id);

    //clear the part
    glcd_rect_fill(display, 119, 10, DISPLAY_WIDTH-119, 13, GLCD_WHITE);

    // draws the max field
    textbox_t index;
    index.color = GLCD_BLACK;
    index.mode = TEXT_SINGLE_LINE;
    index.font = Terminal3x5;
    index.height = 0;
    index.width = 0;
    index.bottom_margin = 0;
    index.left_margin = 0;
    index.right_margin = 0;
    index.align = ALIGN_NONE_NONE;
    index.top_margin = 0;
    index.text = str_current;
    index.x = 120;
    index.y = 11;
    widget_textbox(display, &index);

    // draws the current field
    index.y = 17;
    index.text = str_max;
    widget_textbox(display, &index);

    //invert the block
    glcd_rect_invert(display, 119, 10, DISPLAY_WIDTH-119, 13);
}

void screen_footer(uint8_t display_id, const char *name, const char *value, int16_t property)
{
    if (display_id > FOOTSWITCHES_COUNT) return;

    if (naveg_is_tool_mode(display_id)) return;

    glcd_t *display = hardware_glcds(display_id);

    // clear the footer area
    glcd_rect_fill(display, 0, 50, DISPLAY_WIDTH, DISPLAY_HEIGHT-50, GLCD_WHITE);
    glcd_hline(display, 0, 50, DISPLAY_WIDTH, GLCD_BLACK);

    // draws the name field
    textbox_t footer;
    footer.color = GLCD_BLACK;
    footer.mode = TEXT_SINGLE_LINE;
    footer.font = Terminal7x8;
    footer.height = 0;
    footer.width = 0;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 1;
    footer.right_margin = 1;
    footer.y = 53;

    if (name == NULL && value == NULL)
    {
        char text[sizeof(SCREEN_FOOT_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_FOOT_DEFAULT_NAME);
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)-1] = display_id + '1';
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)] = 0;

        footer.text = text;
        footer.align = ALIGN_CENTER_NONE;
        widget_textbox(display, &footer);
        return;
    }
    //if we are in toggle, trigger or byoass mode we dont have a value
    else if ((property & CONTROL_PROP_TOGGLED) || (property & CONTROL_PROP_BYPASS) || (property & CONTROL_PROP_TRIGGER) || (property & CONTROL_PROP_MOMENTARY_SW))
    {
        footer.text = name;
        footer.align = ALIGN_CENTER_NONE;
        widget_textbox(display, &footer);
    
        if (value[1] == 'N')
        {
            glcd_rect_invert(display, 0, 51, DISPLAY_WIDTH, DISPLAY_HEIGHT-51);
        }
    }
    else 
    {
        uint8_t char_cnt_name = strlen(name);
        uint8_t char_cnt_value = strlen(value);

        if ((char_cnt_value + char_cnt_name) > 15)
        {
            //both bigger then the limmit
            if ((char_cnt_value > 7)&&(char_cnt_name > 8))
            {
                char_cnt_name = 8;
                char_cnt_value = 7;
            }
            else if (char_cnt_value > 7)
            {
                char_cnt_value = 15 - char_cnt_name;
            }
            else if (char_cnt_name > 8)
            {
                char_cnt_name = 15 - char_cnt_value;
            }
        }

        char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
        char *value_str_bfr = (char *) MALLOC((char_cnt_value + 1) * sizeof(char));

        //draw name
        strncpy(title_str_bfr, name, char_cnt_name);
        title_str_bfr[char_cnt_name] = '\0';
        footer.text = title_str_bfr;
        footer.align = ALIGN_LEFT_NONE;
        widget_textbox(display, &footer);
        FREE(title_str_bfr);

        // draws the value field
        strncpy(value_str_bfr, value, char_cnt_value);
        value_str_bfr[char_cnt_value] =  '\0';
        footer.text = value_str_bfr;
        footer.width = 0;
        footer.align = ALIGN_RIGHT_NONE;
        widget_textbox(display, &footer);
        FREE(value_str_bfr);

        //if in banks menu, invert
        if (property == CONTROL_PROP_BANKS)
            glcd_rect_invert(display, DISPLAY_WIDTH - 10, 51, 10, DISPLAY_HEIGHT-52);
    }
}

void screen_top_info(const void *data, uint8_t display_id, uint8_t update)
{
    static char* pedalboard_name = NULL;
    uint8_t char_cnt = 0;
    glcd_t *display = hardware_glcds(display_id);

    if (update)
    {
        //TODO REMOVE DATA FROM HERE DEBUG
        if ((pedalboard_name == NULL) || (data == NULL))
        {
            pedalboard_name = (char *) MALLOC(29 * sizeof(char));
            strcpy(pedalboard_name, "DEFAULT");
            char_cnt = 7;
        }
        else 
        {
            const char **name_list = (const char**)data;

            // get first list name, copy it to our string buffer
            const char *name_string = *name_list;
            strncpy(pedalboard_name, name_string, 29);
            pedalboard_name[29] = 0; // strncpy might not have final null byte

            // go to next name in list
            name_string = *(++name_list);

            while (name_string && ((strlen(pedalboard_name) + strlen(name_string) + 1) < 29))
            {
                strcat(pedalboard_name, " ");
                strcat(pedalboard_name, name_string);
                name_string = *(++name_list);
                char_cnt++;
            }
            pedalboard_name[29] = 0;
        }
    }

    //we dont display inside a menu
    if (naveg_is_tool_mode(display_id)) return;

    // clear the name area
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 10, GLCD_WHITE);

    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal5x7;
    title.top_margin = 1;
    title.bottom_margin = 0;
    title.left_margin = 10;
    title.right_margin = 1;
    title.height = 0;
    title.width = 0;
    title.text = pedalboard_name;
    title.align = ALIGN_NONE_NONE;
    title.y = 1;
    title.x = ((DISPLAY_WIDTH / 2) - 3*char_cnt + 7);
    widget_textbox(display, &title);

    if (display_id)
    {
        icon_snapshot(display, title.x - 11, 1);
    }
    else 
    {
        icon_pedalboard(display, title.x - 11, 1);
    }

    //invert the top bar
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

    //TODO REMOVE ME DEBUG
    FREE(pedalboard_name);
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

            if (naveg_banks_mode_pb() == BANKS_LIST)
            {
                screen_bp_list("BANKS", bp_list);
            }
            else 
            {
                screen_bp_list(naveg_get_current_pb_name(), bp_list);
            }
            break;
    }
}

void screen_bp_list(const char *title, bp_list_t *list)
{
    if (!naveg_is_tool_mode(DISPLAY_RIGHT))
        return; 

    listbox_t list_box;
    textbox_t title_box;

    glcd_t *display = hardware_glcds(1);

    // clears the title
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    // draws the title
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = Terminal3x5;
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
        list_box.hover = list->hover - list->page_min;
        list_box.selected = list->selected - list->page_min;
        list_box.count = count;
        list_box.list = list->names;
        list_box.font = Terminal3x5;
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

    static menu_item_t *last_item = NULL;

    glcd_t *display;
    if (item->desc->id == ROOT_ID)
    {
        display = hardware_glcds(DISPLAY_TOOL_SYSTEM);
    }
    else if (item->desc->id == TUNER_ID)
    {
        return;
    }
    else
    {
        display = hardware_glcds(DISPLAY_RIGHT);
    }

    //we dont display a menu on the right screen when we are in the banks.
    if ((display == hardware_glcds(DISPLAY_RIGHT)) && (naveg_tool_is_on(DISPLAY_TOOL_NAVIG)))
    {
        return;
    } 

    // clear screen
    glcd_clear(display, GLCD_WHITE);

    // draws the title
    textbox_t title_box;
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = Terminal3x5;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.height = 0;
    title_box.width = 0;
    title_box.align = ALIGN_CENTER_TOP;
    title_box.text = item->name;

    if (item->desc->type != MENU_CONFIRM2)
    {
        if ((item->desc->type == MENU_NONE) || (item->desc->type == MENU_TOGGLE))
        {
            if (last_item)
            {
                title_box.text = last_item->name;
                if (title_box.text[strlen(item->name) - 1] == ']')
                    title_box.text = last_item->desc->name;
            }
        }
        else if (title_box.text[strlen(item->name) - 1] == ']')
        {
            title_box.text = item->desc->name;
        }
        widget_textbox(display, &title_box);
    }

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
    list.font = Terminal3x5;
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
    popup.font = Terminal3x5;

    switch (item->desc->type)
    {
        case MENU_LIST:
        case MENU_SELECT:
            list.hover = item->data.hover;
            list.selected = item->data.selected;
            list.count = item->data.list_count;
            list.list = item->data.list;
            widget_listbox(display, &list);
            if ((last_item && last_item->desc->id != TEMPO_ID) || item->desc->id == SYSTEM_ID || item->desc->id == BYPASS_ID)
                last_item = item;
            break;

        case MENU_CONFIRM:
        case MENU_CONFIRM2:
        case MENU_CANCEL:
        case MENU_OK:
        case MENU_MESSAGE:
            if (item->desc->type == MENU_CANCEL)
                popup.type = CANCEL_ONLY;
            else if (item->desc->type == MENU_OK)
                popup.type = OK_ONLY;
            else if (item->desc->type == MENU_MESSAGE)
                popup.type = EMPTY_POPUP;
            else
                popup.type = YES_NO;
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
            else if (last_item)
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
            if (last_item)
            {
                list.hover = last_item->data.hover;
                list.selected = last_item->data.selected;
                list.count = last_item->data.list_count;
                list.list = last_item->data.list;
                widget_listbox(display, &list);
            }
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
    glcd_t *display_img = hardware_glcds(display);
    glcd_draw_image(display_img, 0, 0, image, GLCD_BLACK);
}