
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
uint8_t buttonsetL[] = {0,0};
uint8_t buttonsetR[2] = {0,0};
uint8_t potsetL[2] = {0,0};
uint8_t potsetR[2] = {0,0};

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

//new duo sibling screen functions
void screen_encoder_box(uint8_t display_id, control_t *control)
{ 
    glcd_t *display = hardware_glcds(display_id);

    if (!(display_id < 2)){return;}

    // clear the title area
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 20, GLCD_WHITE);

    // horizontal title line
    glcd_hline(display, 0, 20, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    

    if (!control)
    {

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
        title.align = ALIGN_CENTER_TOP;
        widget_textbox(display, &title);
        return;
    }

    //TODO make listbox3 function for listbox in top plane
     if (control->properties == CONTROL_PROP_ENUMERATION ||
             control->properties == CONTROL_PROP_SCALE_POINTS)
    {
        static char *labels_list[1024];

        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
        {
            labels_list[i] = control->scale_points[i]->label;
        }

        listbox_t list;
        list.x = 0;
        list.y = 0;
        list.width = 128;
        list.height = 7;
        list.color = GLCD_BLACK;
        list.font = FONT_DEFAULT;
        list.selected = control->step;
        list.count = control->scale_points_count;
        list.list = labels_list;
        list.line_space = 1;
        list.line_top_margin = 1;
        list.line_bottom_margin = 0;
        list.text_left_margin = 1;
        widget_listbox3(display, &list);
    }

    else 
    {
        textbox_t value;
        char value_str[32];

        // control title label
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
        title.text = control->label;
        title.align = ALIGN_LEFT_TOP;
        widget_textbox(display, &title);

        // draws the value
        float_to_str(control->value, value_str, sizeof(value_str), 2);
        value.color = GLCD_BLACK;
        value.mode = TEXT_SINGLE_LINE;
        value.font = alterebro24;
        value.top_margin = 0;
        value.bottom_margin = 0;
        value.left_margin = 0;
        value.right_margin = (strcmp(control->unit, "none") == 0 ? 0 : 4);
        value.height = 0;
        value.width = 0;
        value.text = value_str;
        value.align = ALIGN_RIGHT_TOP;
        widget_textbox(display, &value);

        // draws the slider
        slider_t slider;
        slider.x = 1;
        slider.y = 16;
        slider.width = 126;
        slider.color = GLCD_BLACK;
        slider.min = control->minimum;
        slider.max = control->maximum;
        slider.value = control->value;
        widget_slider(display, &slider);
        
    }
}

void screen_control_pot(uint8_t id, control_t *control)
{
    switch (id)
    {
        case 0:
            screen_pot_box_left(0, control);
        case 2:
            screen_pot_box_left(1, control);
        break;
        case 1:
            screen_pot_box_right(0, control);
        case 3:
            screen_pot_box_right(1, control);
        break;
        case 4:
            screen_pot_footer_left(0, control);
        case 6:
            screen_pot_footer_left(1, control);
        break;
        case 5:
            screen_pot_footer_right(0, control);
        case 7:
            screen_pot_footer_right(1, control);
        break;
    }
}

void screen_pot_box_left(uint8_t display_id, control_t *control)
{
    glcd_t *display = hardware_glcds(display_id);
    
    //clear area
    glcd_rect_fill(display, 0, 21, 64, 29, GLCD_WHITE);
    
    // horizontal  lines
    glcd_hline(display, 0, 20, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    glcd_hline(display, 0, 50, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    //Vertical line
    glcd_vline(display, 64, 20, 30, GLCD_BLACK_WHITE);

    textbox_t value;
    char value_str[32];

    if (!control)
    {
        char text[sizeof(SCREEN_POT_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_POT_DEFAULT_NAME);
        if (display_id ==0){text[sizeof(SCREEN_POT_DEFAULT_NAME)-1] = display_id + '1';}
        else {text[sizeof(SCREEN_POT_DEFAULT_NAME)-1] = display_id + '5';}
        text[sizeof(SCREEN_POT_DEFAULT_NAME)] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = alterebro15;
        title.height = 0;
        title.width = 0;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 0;
        title.right_margin = 0;
        title.text = text;
        title.align = ALIGN_LCENTER_MIDDLE;
        widget_textbox(display, &title);
        return;
    }

    // list type control CANT DO THAT (yet)
    if (control->properties == CONTROL_PROP_ENUMERATION ||
        control->properties == CONTROL_PROP_SCALE_POINTS)
    {
        return;
    }

    // control title label
    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = alterebro15;
    title.top_margin = 0;
    title.bottom_margin = 0;
    title.left_margin = 0;
    title.right_margin = 0;
    title.height = 0;
    title.width = 0;
    title.text = control->label;
    title.align = ALIGN_LEFT_MTOP;
    widget_textbox(display, &title);

    // draws the value
    int_to_str(control->value, value_str, sizeof(value_str), 0);
    value.color = GLCD_BLACK;
    value.mode = TEXT_SINGLE_LINE;
    value.font = alterebro15;
    value.top_margin = 0;
    value.bottom_margin = 0;
    value.left_margin = 0;
    value.right_margin = 0;
    value.height = 0;
    value.width = 0;
    value.text = value_str;
    value.align = ALIGN_LRIGHT_MBOTTOM;
    widget_textbox(display, &value); 

    //TODO MAKE GRAPH     
}

void screen_pot_box_right(uint8_t display_id, control_t *control)
{
    glcd_t *display = hardware_glcds(display_id);

    //clear area
    glcd_rect_fill(display,65 , 21, 63, 29, GLCD_WHITE);

    // horizontal  lines
    glcd_hline(display, 0, 20, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    glcd_hline(display, 0, 50, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    //Vertical line
    glcd_vline(display, 64, 20, 30, GLCD_BLACK_WHITE);

    textbox_t value;
    char value_str[32];

    if (!control)
    {
        char text[sizeof(SCREEN_POT_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_POT_DEFAULT_NAME);
        if (display_id ==0){text[sizeof(SCREEN_POT_DEFAULT_NAME)-1] = display_id + '2';}
        else {text[sizeof(SCREEN_POT_DEFAULT_NAME)-1] = display_id + '6';}
        text[sizeof(SCREEN_POT_DEFAULT_NAME)] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = alterebro15;
        title.height = 0;
        title.width = 0;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 0;
        title.right_margin = 0;
        title.text = text;
        title.align = ALIGN_RCENTER_MIDDLE;
        widget_textbox(display, &title);
        return;
    }

    // list type control CANT DO THAT (yet)
    if (control->properties == CONTROL_PROP_ENUMERATION ||
         control->properties == CONTROL_PROP_SCALE_POINTS)
    {
        return;
    }

    // control title label
    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = alterebro15;
    title.top_margin = 0;
    title.bottom_margin = 0;
    title.left_margin = 2;
    title.right_margin = 0;
    title.height = 0;
    title.width = 0;
    title.text = control->label;
    title.align = ALIGN_RLEFT_MTOP;
    widget_textbox(display, &title);

    // draws the value
    int_to_str(control->value, value_str, sizeof(value_str), 0);
    value.color = GLCD_BLACK;
    value.mode = TEXT_SINGLE_LINE;
    value.font = alterebro15;
    value.top_margin = 0;
    value.bottom_margin = 0;
    value.left_margin = 0;
    value.right_margin = (strcmp(control->unit, "none") == 0 ? 0 : 4);
    value.height = 0;
    value.width = 0;
    value.text = value_str;
    value.align = ALIGN_RIGHT_MBOTTOM;
    widget_textbox(display, &value);

    //TODO MAKE GRAPH    
}

void screen_footer(uint8_t id, const char *name, const char *value)
{
    switch (id)
    {
        case 0:
           screen_footer_button_left(0, name , value); 
        break;
        case 1:
            screen_footer_button_right(0, name , value);
        break;
        case 2:
            screen_footer_button_left(1, name , value);
        break;
        case 3:
            screen_footer_button_right(1, name , value);
        break;
    }
    //code is not being used right now 
/*
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
    */
}

//new duo sibling footer functions
void screen_pot_footer_left(uint8_t display_id, control_t *control)
{
    glcd_t *display = hardware_glcds(display_id);

    // clear the footer area
    glcd_rect_fill(display, 0, 51, 64, 13, GLCD_WHITE);

    // horizontal footer line
    glcd_hline(display, 0, 50, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    //vertival footer line
    glcd_vline(display, 64, 50, 14, GLCD_BLACK_WHITE);

    if (!control)
    {
        potsetL[display_id] = 0;

        if (potsetL[display_id] == 0 && buttonsetL[display_id] == 0)
        {
            //TODO decide on how to name when one of these is not assigned
            char *text = (SCREEN_FOOT_DEFAULT_NAME);
     
            textbox_t title;
            title.color = GLCD_BLACK;
            title.mode = TEXT_SINGLE_LINE;
            title.font = alterebro15;
            title.top_margin = 0;
            title.bottom_margin = 0;
            title.left_margin = 1;
            title.right_margin = 0;
            title.height = 0;
            title.width = 0;
            title.text = text;
            title.align = ALIGN_LCENTER_BOTTOM;
            title.y = 0;
            widget_textbox(display, &title);
            return;
        }
    }

    potsetL[display_id] = 1;

     // draws the name field
    textbox_t footer;
    footer.color = GLCD_BLACK;
    footer.mode = TEXT_SINGLE_LINE;
    footer.font = alterebro15;
    footer.height = 0;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 0;
    footer.width = 0;
    footer.right_margin = 0;
    footer.text = control->label;
    footer.y = 0;
    footer.align = ALIGN_LEFT_BOTTOM;
    widget_textbox(display, &footer);   
 
    // draws the value field
    footer.width = 0;
    footer.left_margin = 2;
    footer.text = control->unit;
    footer.align = ALIGN_LRIGHT_BOTTOM;
    widget_textbox(display, &footer);

    //TODO MAKE NICE VISUALISATION
}

void screen_pot_footer_right(uint8_t display_id, control_t *control)
{
    glcd_t *display = hardware_glcds(display_id);

    // clear the footer area
    glcd_rect_fill(display, 65, 51, 63, 13, GLCD_WHITE);

    // horizontal footer line
    glcd_hline(display, 0, 50, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    //vertival footer line
    glcd_vline(display, 64, 50, 13, GLCD_BLACK_WHITE);

     if (!control)
        {
            potsetR[display_id] = 0;

            //check if both are not assigned
           if (potsetR[display_id] == 0 && buttonsetR[display_id] == 0)
           {
               char *text = (SCREEN_FOOT_DEFAULT_NAME);
     
               textbox_t title;
               title.color = GLCD_BLACK;
               title.mode = TEXT_SINGLE_LINE;
               title.font = alterebro15;
               title.top_margin = 0;
               title.bottom_margin = 0;
               title.left_margin = 0;
               title.right_margin = 0;
               title.height = 0;
               title.width = 0;
               title.text = text;
               title.align = ALIGN_RCENTER_BOTTOM;
               title.y = 0;
               widget_textbox(display, &title);
               return;
            }
            else 
            {
                //DISPLAY THE button
            }
        }

    buttonsetR[display_id] = 1;

    // draws the name field
    textbox_t footer;
    footer.color = GLCD_BLACK;
    footer.mode = TEXT_SINGLE_LINE;
    footer.font = alterebro15;
    footer.height = 0;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 0;
    footer.width = 0;
    footer.right_margin = 0;
    footer.text = control->label;
    footer.y = 0;
    footer.align = ALIGN_RLEFT_BOTTOM;
    widget_textbox(display, &footer);   

    // draws the value field
    footer.width = 0;
    footer.left_margin = 2;
    footer.text = control->unit;
    footer.align = ALIGN_RIGHT_BOTTOM;
    widget_textbox(display, &footer);    


    //JTODO MAKE NICE VISUALISATION
}

void screen_footer_button_left(uint8_t display_id, const char *name, const char *value)
{
    glcd_t *display = hardware_glcds(display_id);

    // clear the footer area
    glcd_rect_fill(display, 0, 51, 64, 13, GLCD_WHITE);

    // horizontal footer line
    glcd_hline(display, 0, 50, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    //vertival footer line
    glcd_vline(display, 64, 50, 14, GLCD_BLACK_WHITE);

    if (name == NULL && value == NULL)
    {
        buttonsetL[display_id] = 0;

        if (potsetL[display_id] == 0 && buttonsetL[display_id] == 0)
        {
            char *text = (SCREEN_FOOT_DEFAULT_NAME);
     
            textbox_t title;
            title.color = GLCD_BLACK;
            title.mode = TEXT_SINGLE_LINE;
            title.font = alterebro15;
            title.top_margin = 0;
            title.bottom_margin = 0;
            title.left_margin = 1;
            title.right_margin = 0;
            title.height = 0;
            title.width = 0;
            title.text = text;
            title.align = ALIGN_LCENTER_BOTTOM;
            title.y = 0;
            widget_textbox(display, &title);
            return;
        }
    }

buttonsetL[display_id] = 1;

     // draws the name field
    textbox_t footer;
    footer.color = GLCD_BLACK;
    footer.mode = TEXT_SINGLE_LINE;
    footer.font = alterebro15;
    footer.height = 0;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 0;
    footer.width = 0;
    footer.right_margin = 0;
    footer.text = name;
    footer.y = 0;
    footer.align = ALIGN_LEFT_BOTTOM;
    widget_textbox(display, &footer);   
 
    // draws the value field
    footer.width = 0;
    footer.right_margin = 2;
    footer.text = value;
    footer.align = ALIGN_LRIGHT_BOTTOM;
    widget_textbox(display, &footer);
}

void screen_footer_button_right(uint8_t display_id, const char *name, const char *value)
{
    glcd_t *display = hardware_glcds(display_id);
    
    // clear the footer area
    glcd_rect_fill(display, 65, 51, 63, 13, GLCD_WHITE);

    // horizontal footer line
    glcd_hline(display, 0, 50, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    //vertival footer line
    glcd_vline(display, 64, 50, 13, GLCD_BLACK_WHITE);

     if (name == NULL && value == NULL)
        {
            buttonsetR[display_id] = 0;

            //check if both are not assigned
           if (potsetR[display_id] == 0 && buttonsetR[display_id] == 0)
           {
               char *text = (SCREEN_FOOT_DEFAULT_NAME);
     
               textbox_t title;
               title.color = GLCD_BLACK;
               title.mode = TEXT_SINGLE_LINE;
               title.font = alterebro15;
               title.top_margin = 0;
               title.bottom_margin = 0;
               title.left_margin = 0;
               title.right_margin = 0;
               title.height = 0;
               title.width = 0;
               title.text = text;
               title.align = ALIGN_RCENTER_BOTTOM;
               title.y = 0;
               widget_textbox(display, &title);
               return;
            }
            else 
            {
                //DISPLAY THE POT
            }
        }

    buttonsetR[display_id] = 1;

    // draws the name field
    textbox_t footer;
    footer.color = GLCD_BLACK;
    footer.mode = TEXT_SINGLE_LINE;
    footer.font = alterebro15;
    footer.height = 0;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 2;
    footer.width = 0;
    footer.right_margin = 0;
    footer.text = name;
    footer.y = 0;
    footer.align = ALIGN_RLEFT_BOTTOM;
    widget_textbox(display, &footer);   

    // draws the value field
    footer.width = 0;
    footer.left_margin = 2;
    footer.text = value;
    footer.align = ALIGN_RIGHT_BOTTOM;
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
            screen_bp_list("BANKS", bp_list);
            break;
    }
}

void screen_bp_list(const char *title, bp_list_t *list)
{
    listbox_t list_box;
    textbox_t title_box;

    glcd_t *display = hardware_glcds(0);

    // clears the title
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    // draws the title
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = alterebro15;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.height = 0;
    title_box.width = 0;
    title_box.text = title;
    title_box.align = ALIGN_LEFT_TOP;
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
        list_box.font = alterebro15;
        list_box.line_space = 1;
        list_box.line_top_margin = 1;
        list_box.line_bottom_margin = 0;
        list_box.text_left_margin = 2;
        widget_listbox(display, &list_box);
    }
    else
    {
        if (naveg_ui_status())
        {
            popup_t popup;
            popup.x = 0;
            popup.y = 0;
            popup.width = DISPLAY_WIDTH;
            popup.height = DISPLAY_HEIGHT - 1;
            popup.font = alterebro15;
            popup.title = 0;
            popup.content = "To access banks here please disconnect from the graphical interface";
            popup.button_selected = 0;
            popup.type = OK_ONLY;
            widget_popup(display, &popup);
        }
    }

}

void screen_system_menu(menu_item_t *item)
{
    // return if system is disabled
    if (!naveg_is_tool_mode(DISPLAY_TOOL_SYSTEM))
        return;

    if (naveg_is_tool_mode(DISPLAY_TOOL_NAVIG))
        return;

    static menu_item_t *last_item;

    glcd_t *display = hardware_glcds(DISPLAY_TOOL_SYSTEM);

    // clear screen
    glcd_clear(display, GLCD_WHITE);

    // draws the title
    textbox_t title_box;
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = alterebro15;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.height = 0;
    title_box.width = 0;
    title_box.align = ALIGN_LEFT_TOP;
    title_box.text = item->name;
    if (item->desc->type == MENU_NONE || MENU_ITEM_IS_TOGGLE_TYPE(item)) title_box.text = last_item->name;
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
    list.font = alterebro15;
    list.line_space = 1;
    list.line_top_margin = 1;
    list.line_bottom_margin = 0;
    list.text_left_margin = 2;

    // popup
    popup_t popup;
    popup.x = 0;
    popup.y = 0;
    popup.width = DISPLAY_WIDTH;
    popup.height = DISPLAY_HEIGHT - 1;
    popup.font = alterebro15;

    // graph
    graph_t graph;
    graph.x = 0;
    graph.y = 18;
    graph.color = GLCD_BLACK;
    graph.font = alterebro24;
    graph.min = item->data.min;
    graph.max = item->data.max;
    graph.value = item->data.value;
    graph.unit = "dB";
    graph.type = GRAPH_TYPE_LINEAR;

    switch (item->desc->type)
    {
        case MENU_LIST:
        case MENU_SELECT:
            list.hover = item->data.hover;
            list.selected = item->data.selected;
            list.count = item->data.list_count;
            list.list = item->data.list;
            widget_listbox(display, &list);
            last_item = item;
            break;

        case MENU_CONFIRM:
        case MENU_CONFIRM2:
        case MENU_CANCEL:
        case MENU_OK:
            if (item->desc->type == MENU_CANCEL) popup.type = CANCEL_ONLY;
            else if (item->desc->type == MENU_OK) popup.type = OK_ONLY;
            else popup.type = YES_NO;
            popup.title = item->desc->name;
            popup.content = item->data.popup_content;
            popup.button_selected = item->data.hover;
            widget_popup(display, &popup);
            break;

        case MENU_NONE:
        case MENU_RETURN:
            list.hover = last_item->data.hover;
            list.selected = last_item->data.selected;
            list.count = last_item->data.list_count;
            list.list = last_item->data.list;
            widget_listbox(display, &list);
            break;

        case MENU_ON_OFF:
        case MENU_YES_NO:
        case MENU_BYP_PROC:
            strcpy(item->name, item->desc->name);

            if (item->desc->type == MENU_ON_OFF)
                strcat(item->name, (item->data.hover ? " ON" : "OFF"));
            else if (item->desc->type == MENU_YES_NO)
                strcat(item->name, (item->data.hover ? "YES" : " NO"));
            else if (item->desc->type == MENU_BYP_PROC)
                strcat(item->name, (item->data.hover ? "BYP" : "PROC"));

            list.hover = last_item->data.hover;
            list.selected = last_item->data.selected;
            list.count = last_item->data.list_count;
            list.list = last_item->data.list;
            widget_listbox(display, &list);
            break;

        case MENU_GRAPH:
            widget_graph(display, &graph);
            screen_footer(DISPLAY_TOOL_SYSTEM, "Click to return", NULL);
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
