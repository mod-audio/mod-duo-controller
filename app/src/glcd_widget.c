
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "glcd_widget.h"
#include "utils.h"

#include <math.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define GRAPH_WIDTH         128
#define GRAPH_HEIGHT        32
#define GRAPH_BAR_WIDTH     3
#define GRAPH_BAR_SPACE     1
#define GRAPH_NUM_BARS      sizeof(GraphLinTable)
#define GRAPH_V_NUM_BARS    sizeof(GraphVTable)


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static const uint8_t GraphLinTable[] = {
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32
};

static const uint8_t GraphLogTable[] = {
     1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
     3,  3,  3,  4,  4,  5,  5,  6,  7,  8,
     9, 10, 11, 12, 13, 15, 17, 19, 21, 24,
    28, 32
};

static const uint8_t GraphVTable[] = {
    31, 29, 27, 25, 23, 21, 19, 17, 15, 13,
    11,  9,  7,  5,  3,  1,  1,  3,  5,  7,
     9, 11, 13, 15, 17, 19, 21, 23, 25, 27,
    29, 31
};

static const uint8_t GraphATable[] = {
     1,  3,  5,  7,  9, 11, 13, 15, 17, 19,
    21, 23, 25, 27, 29, 31, 31, 29, 27, 25,
    23, 21, 19, 17, 15, 13, 11,  9,  7,  5,
     3,  1
};

static const uint8_t GraphVPositionTable[] = {
      0,   4,   8,  12,  16,  20,  24,  28,  32,  36,
     40,  44,  48,  52,  56,  60,  67,  71,  75,  79,
     83,  87,  91,  95,  99, 103, 107, 111, 115, 119,
    123, 127
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

#define ABS(x)      ((x) > 0 ? (x) : -(x))
#define ROUND(x)    ((x) > 0.0 ? (((float)(x)) + 0.5) : (((float)(x)) - 0.5))


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

static uint8_t get_text_width(const char *text, const uint8_t *font)
{
    uint8_t text_width = 0;
    const char *ptext = text;

    if (FONT_IS_MONO_SPACED(font))
    {
        while (*ptext)
        {
            text_width += font[FONT_FIXED_WIDTH] + FONT_INTERCHAR_SPACE;
            ptext++;
        }
    }
    else
    {
        while (*ptext)
        {
            text_width += font[FONT_WIDTH_TABLE + ((*ptext) - font[FONT_FIRST_CHAR])] + FONT_INTERCHAR_SPACE;
            ptext++;
        }
    }

    text_width -= FONT_INTERCHAR_SPACE;

    return text_width;
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void widget_textbox(uint8_t display, textbox_t *textbox)
{
    uint8_t text_width, text_height;

    if (textbox->mode == TEXT_SINGLE_LINE)
    {
        text_width = get_text_width(textbox->text, textbox->font);
        text_height = textbox->font[FONT_HEIGHT];
    }
    else
    {
        text_width = textbox->width;
        text_height = textbox->height;
    }

    if (textbox->width == 0) textbox->width = text_width;
    else if (text_width > textbox->width) text_width = textbox->width;

    if (textbox->height == 0) textbox->height = text_height;

    switch (textbox->align)
    {
        case ALIGN_LEFT_TOP:
            textbox->x = textbox->left_margin;
            textbox->y = textbox->top_margin;
            break;

        case ALIGN_CENTER_TOP:
            textbox->x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox->y = textbox->top_margin;
            break;

        case ALIGN_RIGHT_TOP:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y = textbox->top_margin;
            break;

        case ALIGN_LEFT_MIDDLE:
            textbox->x = textbox->left_margin;
            textbox->y = (DISPLAY_HEIGHT / 2) - (text_height / 2);
            break;

        case ALIGN_CENTER_MIDDLE:
            textbox->x = ((DISPLAY_WIDTH / 2) - (text_width / 2)) + (textbox->left_margin - textbox->right_margin);
            textbox->y = ((DISPLAY_HEIGHT / 2) - (text_height / 2)) + (textbox->top_margin - textbox->bottom_margin);
            break;

        case ALIGN_RIGHT_MIDDLE:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y = (DISPLAY_HEIGHT / 2) - (text_height / 2);
            break;

        case ALIGN_LEFT_BOTTOM:
            textbox->x = textbox->left_margin;
            textbox->y = DISPLAY_HEIGHT - text_height - textbox->bottom_margin;
            break;

        case ALIGN_CENTER_BOTTOM:
            textbox->x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox->y = DISPLAY_HEIGHT - text_height - textbox->bottom_margin;
            break;

        case ALIGN_RIGHT_BOTTOM:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y = DISPLAY_HEIGHT - text_height - textbox->bottom_margin;
            break;

        default:
        case ALIGN_NONE_NONE:
            break;

        case ALIGN_LEFT_NONE:
            textbox->x = textbox->left_margin;
            textbox->y += textbox->top_margin;
            break;

        case ALIGN_RIGHT_NONE:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y += textbox->top_margin;
            break;

        case ALIGN_CENTER_NONE:
            textbox->x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox->y += textbox->top_margin;
            break;

        // TODO: others NONE options
    }

    // clear the text area
    glcd_rect_fill(display, textbox->x, textbox->y, textbox->width, textbox->height, GLCD_WHITE);

    uint8_t i = 0, index;
    const char *ptext = textbox->text;
    char buffer[DISPLAY_WIDTH/2];

    text_width = 0;
    text_height = 0;
    index = FONT_FIXED_WIDTH;

    while (*ptext)
    {
        // gets the index of the current character
        if (!FONT_IS_MONO_SPACED(textbox->font)) index = FONT_WIDTH_TABLE + ((*ptext) - textbox->font[FONT_FIRST_CHAR]);

        // calculates the text width
        text_width += textbox->font[index] + FONT_INTERCHAR_SPACE;

        // buffering
        buffer[i++] = *ptext;

        // checks the width limit
        if (text_width >= textbox->width)
        {
            // check whether is single line
            if (textbox->mode == TEXT_SINGLE_LINE) break;
            else buffer[i-1] = 0;

            glcd_text(display, textbox->x, textbox->y + text_height, buffer, textbox->font, textbox->color);
            text_height += textbox->font[FONT_HEIGHT] + 1;
            text_width = 0;
            i = 0;

            // checks the height limit
            if (text_height > textbox->height) break;
        }
        else ptext++;
    }

    // draws the line
    if (text_width > 0)
    {
        buffer[i] = 0;

        // checks the text width again
        text_width = get_text_width(buffer, textbox->font);
        if (text_width > textbox->width) buffer[i-1] = 0;

        glcd_text(display, textbox->x, textbox->y + text_height, buffer, textbox->font, textbox->color);
    }
}


void widget_listbox(uint8_t display, listbox_t *listbox)
{
    uint8_t i, font_height, max_lines, y_line;
    uint8_t first_line, focus, center_focus, focus_height;
    char aux[DISPLAY_WIDTH/2];
    const char *line_txt;

    glcd_rect_fill(display, listbox->x, listbox->y, listbox->width, listbox->height, ~listbox->color);

    font_height = listbox->font[FONT_HEIGHT];
    max_lines = listbox->height / (font_height + listbox->line_space);

    center_focus = (max_lines / 2) - (1 - (max_lines % 2));
    first_line = 0;

    if (listbox->hover > center_focus && listbox->count > max_lines)
    {
        first_line = listbox->hover - center_focus;
        if (first_line > ABS(listbox->count - max_lines))
        {
            first_line = ABS(listbox->count - max_lines);
        }
    }

    if (max_lines > listbox->count) max_lines = listbox->count;
    focus = listbox->hover - first_line;
    focus_height = font_height + listbox->line_top_margin + listbox->line_bottom_margin;
    y_line = listbox->y + listbox->line_space;

    for (i = 0; i < max_lines; i++)
    {
        if (i < listbox->count)
        {
            line_txt = listbox->list[first_line + i];

            if ((first_line + i) == listbox->selected)
            {
                uint8_t j = 0;
                aux[j++] = ' ';
                aux[j++] = '>';
                aux[j++] = ' ';
                while (*line_txt && j < sizeof(aux)-1) aux[j++] = *line_txt++;
                aux[j] = 0;
                line_txt = aux;
            }

            glcd_text(display, listbox->x + listbox->text_left_margin, y_line, line_txt, listbox->font, listbox->color);

            if (i == focus)
            {
                glcd_rect_invert(display, listbox->x, y_line - listbox->line_top_margin, listbox->width, focus_height);
            }

            y_line += font_height + listbox->line_space;
        }
    }
}


void widget_listbox2(uint8_t display, listbox_t *listbox) //FIXME: function hardcoded
{
    glcd_rect_fill(display, listbox->x, listbox->y, listbox->width, listbox->height, ~listbox->color);

    if (listbox->selected > 0)
    {
        glcd_text(display, listbox->x + listbox->text_left_margin, 19, listbox->list[listbox->selected-1], alterebro15, listbox->color);
    }

    if (listbox->selected < (listbox->count - 1))
    {
        glcd_text(display, listbox->x + listbox->text_left_margin, 41, listbox->list[listbox->selected+1], listbox->font, listbox->color);
    }

    glcd_text(display, listbox->x + listbox->text_left_margin + 2, 27, listbox->list[listbox->selected], alterebro24, listbox->color);
    glcd_rect_invert(display, listbox->x, 27, listbox->width, 13);
}


void widget_graph(uint8_t display, graph_t *graph)
{
    const uint8_t *graph_table = NULL;
    uint8_t i, n = 0;
    float a, b, x, y, value, min, max;
    textbox_t value_box, unit_box;
    char value_str[16];

    // clear the graph area
    glcd_rect_fill(display, graph->x, graph->y, GRAPH_WIDTH, GRAPH_HEIGHT, ~graph->color);

    // init the value box
    float_to_str(graph->value, value_str, sizeof(value_str), 2);
    value_box.mode = TEXT_SINGLE_LINE;
    value_box.text = value_str;
    value_box.color = GLCD_BLACK;
    value_box.font = graph->font;
    value_box.top_margin = 0;
    value_box.bottom_margin = 0;
    value_box.left_margin = 2;
    value_box.right_margin = 2;
    value_box.align = ALIGN_NONE_NONE;
    value_box.x = graph->x;
    value_box.y = graph->y;
    value_box.height = 0;
    value_box.width = 0;

    // init the unit box
    unit_box.mode = TEXT_SINGLE_LINE;
    unit_box.color = GLCD_BLACK;
    unit_box.font = graph->font;
    unit_box.top_margin = 0;
    unit_box.bottom_margin = 0;
    unit_box.left_margin = 0;
    unit_box.right_margin = 0;
    unit_box.text = graph->unit;
    unit_box.align = ALIGN_NONE_NONE;
    unit_box.x = value_box.x + get_text_width(value_str, graph->font) + 4;
    unit_box.y = value_box.y;
    unit_box.height = 0;
    unit_box.width = 0;

    // linear
    // y = a*x + b
    // y1 = 0, y2 = GRAPH_NUM_BARS, x1 = graph->min, x2 = graph->max
    // a = (y2 - y1) / (x2 - x1)
    // b = y1 - (a * x1)
    if (graph->type == GRAPH_TYPE_LINEAR)
    {
        graph_table = GraphLinTable;
        a = ((float) GRAPH_NUM_BARS) / (graph->max - graph->min);
        b = -(a * graph->min);
        value = a * graph->value + b;
        n = (uint8_t) ROUND(value);

        x = graph->x;

        uint8_t zero, height;

        // zero axis
        zero = (uint8_t) ROUND(b);
        if (zero > GRAPH_NUM_BARS) zero = GRAPH_NUM_BARS;

        for (i = 0; i < GRAPH_NUM_BARS; i++)
        {
            // calculates the y and height a
            if (i < zero)
            {
                y = graph->y + GRAPH_HEIGHT - zero;
                height = zero - graph_table[i];
            }
            else
            {
                y = graph->y + GRAPH_HEIGHT - graph_table[i];
                height = graph_table[i] - zero;
            }

            // draws the full column
            if (i < n)
            {
                glcd_rect_fill(display, x, y, GRAPH_BAR_WIDTH, height, graph->color);
            }
            // draws the empty column
            else
            {
                glcd_vline(display, x + 2, y, height, graph->color);
            }

            x += (GRAPH_BAR_WIDTH + GRAPH_BAR_SPACE);
        }

        // recalculate the textbox position if necessary
        if (zero > graph->font[FONT_HEIGHT])
        {
            unit_box.x = GRAPH_WIDTH - get_text_width(graph->unit, graph->font);
            unit_box.y = graph->y + GRAPH_HEIGHT - graph->font[FONT_HEIGHT];

            value_box.x = unit_box.x - get_text_width(value_box.text, graph->font) - 4;
            value_box.y = unit_box.y;
        }
    }

    // log
    // y = a * log(x) + b
    // y1 = 0, y2 = GRAPH_NUM_BARS, x1 = log(graph->min), x2 = log(graph->max)
    // a = (y2 - y1) / (x2 - x1)
    // b = y1 - (a * x1)
    else if (graph->type == GRAPH_TYPE_LOG)
    {
        graph_table = GraphLogTable;
        min = log2(graph->min);
        max = log2(graph->max);
        a = ((float) GRAPH_NUM_BARS) / (max - min);
        b = -(a * min);
        value = log2(graph->value);
        value = a * value + b;
        n = (uint8_t) ROUND(value);

        x = graph->x;

        for (i = 0; i < GRAPH_NUM_BARS; i++)
        {
            y = graph->y + GRAPH_HEIGHT - graph_table[i];

            if (i < n)
                glcd_rect_fill(display, x, y, GRAPH_BAR_WIDTH, graph_table[i], graph->color);
            else
                glcd_vline(display, x + 2, y, graph_table[i], graph->color);
            x += (GRAPH_BAR_WIDTH + GRAPH_BAR_SPACE);
        }
    }

    // draws the value box
    widget_textbox(display, &value_box);

    // draws the unit box
    widget_textbox(display, &unit_box);
}


// FIXME: this widget is hardcoded
void widget_peakmeter(uint8_t display, uint8_t pkm_id, peakmeter_t *pkm)
{
    uint8_t height, y_black, y_chess, y_peak, h_black, h_chess;
    const uint8_t h_black_max = 20, h_chess_max = 22;
    const uint8_t x_bar[] = {4, 30, 57, 83};
    const float h_max = 42.0, max_dB = 0.0, min_dB = -30.0;

    // calculates the bar height
    float value = pkm->value;
    if (value > max_dB) value = max_dB;
    if (value < min_dB) value = min_dB;
    value = ABS(min_dB) - ABS(value);
    height = (uint8_t) ROUND((h_max * value) / (max_dB - min_dB));

    // clean the peakmeter bar
    if (pkm_id == 0) glcd_rect_fill(display, 4, 13, 16, 42, GLCD_WHITE);
    else if (pkm_id == 1) glcd_rect_fill(display, 30, 13, 16, 42, GLCD_WHITE);
    else if (pkm_id == 2) glcd_rect_fill(display, 57, 13, 16, 42, GLCD_WHITE);
    else if (pkm_id == 3) glcd_rect_fill(display, 83, 13, 16, 42, GLCD_WHITE);

    // draws the black area
    if (height > h_chess_max)
    {
        h_black = height - h_chess_max;
        y_black = 13 + (h_black_max - h_black);
        glcd_rect_fill(display, x_bar[pkm_id], y_black, 16, h_black, GLCD_BLACK);
    }

    // draws the chess area
    if (height > 0)
    {
        h_chess = (height > h_chess_max ? h_chess_max : height);
        y_chess = 33 + (h_chess_max - h_chess);
        glcd_rect_fill(display, x_bar[pkm_id], y_chess, 16, h_chess, GLCD_CHESS);
    }

    // draws the peak
    if (pkm->peak > pkm->value)
    {
        y_peak = 13.0 + ABS(ROUND((h_max * pkm->peak) / (max_dB - min_dB)));
        glcd_hline(display, x_bar[pkm_id], y_peak, 16, GLCD_BLACK);
    }
}


void widget_tuner(uint8_t display, tuner_t *tuner)
{
    // draws the title
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_BLACK);
    textbox_t title;
    title.color = GLCD_WHITE;
    title.mode = TEXT_SINGLE_LINE;
    title.align = ALIGN_LEFT_TOP;
    title.top_margin = 1;
    title.bottom_margin = 0;
    title.left_margin = 2;
    title.right_margin = 0;
    title.height = 0;
    title.width = 0;
    title.font = alterebro15;
    title.text = "Tuner";
    widget_textbox(display, &title);
    glcd_hline(display, 0, 9, DISPLAY_WIDTH, GLCD_WHITE);

    // clears subtitles
    glcd_rect_fill(display, 0, 51, DISPLAY_WIDTH, 12, GLCD_WHITE);

    // draws the frequency subtitle
    char freq_str[16], input_str[16];
    uint8_t i = float_to_str(tuner->frequency, freq_str, sizeof(freq_str), 2);
    freq_str[i++] = 'H';
    freq_str[i++] = 'z';
    freq_str[i++] = 0;
    textbox_t freq, input;
    freq.color = GLCD_BLACK;
    freq.mode = TEXT_SINGLE_LINE;
    freq.align = ALIGN_LEFT_NONE;
    freq.y = 51;
    freq.height = 0;
    freq.width = 0;
    freq.top_margin = 0;
    freq.bottom_margin = 0;
    freq.left_margin = 1;
    freq.right_margin = 0;
    freq.font = alterebro24;
    freq.text = freq_str;
    widget_textbox(display, &freq);

    // draws the input subtitle
    i = 0;
    input_str[i++] = 'I';
    input_str[i++] = 'N';
    input_str[i++] = 'P';
    input_str[i++] = 'U';
    input_str[i++] = 'T';
    input_str[i++] = ' ';
    input_str[i++] = '0' + tuner->input;
    input_str[i++] = 0;
    input.color = GLCD_BLACK;
    input.mode = TEXT_SINGLE_LINE;
    input.align = ALIGN_RIGHT_NONE;
    input.y = 51;
    input.height = 0;
    input.width = 0;
    input.top_margin = 0;
    input.bottom_margin = 0;
    input.left_margin = 0;
    input.right_margin = 1;
    input.font = alterebro24;
    input.text = input_str;
    widget_textbox(display, &input);

    // constants configurations
    const uint8_t num_bars = 5, y_bars = 18, w_bar = 4, h_bar = 21, bars_space = 3;
    const uint8_t cents_range = 50;

    uint8_t x, n;

    // calculates the number of bars that need be filled
    n = (ABS(tuner->cents) + num_bars) / (cents_range / num_bars);

    // draws the left side bars
    for (i = 0, x = 1; i < num_bars; i++)
    {
        // checks if need fill the bar
        if (tuner->cents < 0 && i >= (num_bars - n))
            glcd_rect_fill(display, x, y_bars, w_bar, h_bar, GLCD_BLACK);
        else
            glcd_rect_fill(display, x, y_bars, w_bar, h_bar, GLCD_WHITE);

        glcd_rect(display, x, y_bars, w_bar, h_bar, GLCD_BLACK);

        x += w_bar + bars_space;
    }

    // draws the right side bars
    for (i = 0, x = 95; i < num_bars; i++)
    {
        // checks if need fill the bar
        if (tuner->cents > 0 && i < n)
            glcd_rect_fill(display, x, y_bars, w_bar, h_bar, GLCD_BLACK);
        else
            glcd_rect_fill(display, x, y_bars, w_bar, h_bar, GLCD_WHITE);

        glcd_rect(display, x, y_bars, w_bar, h_bar, GLCD_BLACK);

        x += w_bar + bars_space;
    }

    // draws the note box
    glcd_rect_fill(display, 36, 14, 56, 29, GLCD_WHITE);
    textbox_t note;
    note.color = GLCD_BLACK;
    note.mode = TEXT_SINGLE_LINE;
    note.align = ALIGN_CENTER_NONE;
    note.top_margin = 0;
    note.bottom_margin = 2;
    note.left_margin = 0;
    note.right_margin = 0;
    note.y = 15;
    note.height = 0;
    note.width = 0;
    note.font = alterebro49;
    note.text = tuner->note;
    widget_textbox(display, &note);

    // checks if is tuned
    if (n == 0) glcd_rect_invert(display, 36, 14, 56, 29);
    else glcd_rect(display, 36, 14, 56, 29, GLCD_BLACK);

    // fixes the font extra footer
    glcd_hline(display, 36, 43, 56, GLCD_WHITE);
    glcd_hline(display, 36, 44, 56, GLCD_WHITE);
}


void widget_popup(uint8_t display, popup_t *popup)
{
    // clears the popup area
    glcd_rect_fill(display, popup->x, popup->y, popup->width, popup->height, GLCD_WHITE);

    // draws the contour
    glcd_rect(display, popup->x, popup->y, popup->width, popup->height, GLCD_BLACK);

    // draws the title background
    glcd_rect_fill(display, popup->x, popup->y, popup->width, popup->font[FONT_HEIGHT] + 2, GLCD_BLACK);

    // draws the title text
    textbox_t title;
    title.color = GLCD_WHITE;
    title.mode = TEXT_SINGLE_LINE;
    title.align = ALIGN_NONE_NONE;
    title.top_margin = 0;
    title.bottom_margin = 0;
    title.left_margin = 0;
    title.right_margin = 0;
    title.x = popup->x + 1;
    title.y = popup->y + 1;
    title.height = 0;
    title.width = 0;
    title.font = popup->font;
    title.text = popup->title;
    widget_textbox(display, &title);

    // draws the content
    textbox_t content;
    content.color = GLCD_BLACK;
    content.mode = TEXT_MULTI_LINES;
    content.align = ALIGN_NONE_NONE;
    content.top_margin = 0;
    content.bottom_margin = 0;
    content.left_margin = 0;
    content.right_margin = 0;
    content.font = popup->font;
    content.x = title.x + 1;
    content.y = popup->y + popup->font[FONT_HEIGHT] + 3;
    content.width = popup->width - 4;
    content.height = (popup->font[FONT_HEIGHT] * 5); // FIXME: need be relative to popup height
    content.text = popup->content;
    widget_textbox(display, &content);

    uint8_t button_x, button_y, button_w, button_h;
    const char *button_text;

    // draws the buttons
    switch (popup->type)
    {
        case OK_ONLY:
        case CANCEL_ONLY:
            button_text = (popup->type == OK_ONLY ? "OK" : "CANCEL");
            button_w = get_text_width(button_text, popup->font) + 8;
            button_h = popup->font[FONT_HEIGHT] + 2;
            button_x = popup->x + (popup->width / 2) - (button_w / 2);
            button_y = popup->y + popup->height - button_h - 2;
            glcd_text(display, button_x + 4, button_y + 2, button_text, popup->font, GLCD_BLACK);

            if (popup->button_selected == 0)
                glcd_rect_invert(display, button_x+1, button_y+1, button_w-2, button_h-2);
            break;

        case OK_CANCEL:
        case YES_NO:
            button_text = (popup->type == OK_CANCEL ? "OK" : "YES");
            button_w = get_text_width(button_text, popup->font) + 8;
            button_h = popup->font[FONT_HEIGHT] + 2;
            button_x = popup->x + (popup->width / 4) - (button_w / 2);
            button_y = popup->y + popup->height - button_h - 2;
            glcd_text(display, button_x + 4, button_y + 2, button_text, popup->font, GLCD_BLACK);

            if (popup->button_selected == 0)
                glcd_rect_invert(display, button_x+1, button_y+1, button_w-2, button_h-2);

            button_text = (popup->type == OK_CANCEL ? "CANCEL" : "NO");
            button_w = get_text_width(button_text, popup->font) + 8;
            button_h = popup->font[FONT_HEIGHT] + 2;
            button_x = popup->x + popup->width - (popup->width / 4) - (button_w / 2);
            button_y = popup->y + popup->height - button_h - 2;
            glcd_text(display, button_x + 4, button_y + 2, button_text, popup->font, GLCD_BLACK);

            if (popup->button_selected == 1)
                glcd_rect_invert(display, button_x+1, button_y+1, button_w-2, button_h-2);
            break;
    }
}

