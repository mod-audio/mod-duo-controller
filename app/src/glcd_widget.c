
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

#define ABS(x)      (x > 0 ? x : -x)
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

void widget_textbox(uint8_t display, textbox_t textbox)
{
    uint8_t text_width, text_height;
    uint8_t rect_x, rect_y, rect_width, rect_height;

    // TODO: create width limitation

    text_width = get_text_width(textbox.text, textbox.font);
    text_height = textbox.font[FONT_HEIGHT];
    rect_width = text_width + textbox.left_margin + textbox.right_margin;
    rect_height = text_height + textbox.top_margin + textbox.bottom_margin;

    switch (textbox.align)
    {
        case ALIGN_LEFT_TOP:
            textbox.x = textbox.left_margin;
            textbox.y = textbox.top_margin;
            rect_x = 0;
            rect_y = 0;
            break;

        case ALIGN_CENTER_TOP:
            textbox.x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox.y = textbox.top_margin;
            rect_x = textbox.x - textbox.left_margin;
            rect_y = 0;
            break;

        case ALIGN_RIGHT_TOP:
            textbox.x = DISPLAY_WIDTH - text_width - textbox.right_margin;
            textbox.y = textbox.top_margin;
            rect_x = textbox.x - textbox.left_margin;
            rect_y = 0;
            break;

        case ALIGN_LEFT_MIDDLE:
            textbox.x = textbox.left_margin;
            textbox.y = (DISPLAY_HEIGHT / 2) - (text_height / 2);
            rect_x = 0;
            rect_y = textbox.y - textbox.top_margin;
            break;

        case ALIGN_CENTER_MIDDLE:
            textbox.x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox.y = (DISPLAY_HEIGHT / 2) - (text_height / 2);
            rect_x = textbox.x - textbox.left_margin;
            rect_y = textbox.y - textbox.top_margin;
            break;

        case ALIGN_RIGHT_MIDDLE:
            textbox.x = DISPLAY_WIDTH - text_width - textbox.right_margin;
            textbox.y = (DISPLAY_HEIGHT / 2) - (text_height / 2);
            rect_x = textbox.x - textbox.left_margin;
            rect_y = textbox.y - textbox.top_margin;
            break;

        case ALIGN_LEFT_BOTTOM:
            textbox.x = textbox.left_margin;
            textbox.y = DISPLAY_HEIGHT - text_height - textbox.bottom_margin;
            rect_x = 0;
            rect_y = textbox.y - textbox.top_margin;
            break;

        case ALIGN_CENTER_BOTTOM:
            textbox.x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox.y = DISPLAY_HEIGHT - text_height - textbox.bottom_margin;
            rect_x = textbox.x - textbox.left_margin;
            rect_y = textbox.y - textbox.top_margin;
            break;

        case ALIGN_RIGHT_BOTTOM:
            textbox.x = DISPLAY_WIDTH - text_width - textbox.right_margin;
            textbox.y = DISPLAY_HEIGHT - text_height - textbox.bottom_margin;
            rect_x = textbox.x - textbox.left_margin;
            rect_y = textbox.y - textbox.top_margin;
            break;

        default:
        case ALIGN_NONE_NONE:
            rect_x = textbox.x;
            rect_y = textbox.y;
            textbox.x += textbox.left_margin;
            textbox.y += textbox.top_margin;
            break;

        case ALIGN_LEFT_NONE:
            textbox.x = textbox.left_margin;
            textbox.y += textbox.top_margin;
            rect_x = 0;
            rect_y = textbox.y;
            break;

        case ALIGN_RIGHT_NONE:
            textbox.x = DISPLAY_WIDTH - text_width - textbox.right_margin;
            textbox.y += textbox.top_margin;
            rect_x = textbox.x - textbox.left_margin;
            rect_y = textbox.y;
            break;

        case ALIGN_CENTER_NONE:
            textbox.x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox.y += textbox.top_margin;
            rect_x = textbox.x - textbox.left_margin;
            rect_y = textbox.y;
            break;

        // TODO: others NONE options
    }

    glcd_rect_fill(display, rect_x, rect_y, rect_width, rect_height, ~textbox.text_color);
    glcd_text(display, textbox.x, textbox.y, textbox.text, textbox.font, textbox.text_color);
}


void widget_listbox(uint8_t display, listbox_t listbox)
{
    uint8_t i, font_height, max_lines, y_line;
    uint8_t first_line, focus, center_focus, focus_height;

    glcd_rect_fill(display, listbox.x, listbox.y, listbox.width, listbox.height, ~listbox.color);

    font_height = listbox.font[FONT_HEIGHT];
    max_lines = listbox.height / (font_height + listbox.line_space);

    center_focus = (max_lines / 2) - (1 - (max_lines % 2));
    first_line = 0;

    if (listbox.selected > center_focus && listbox.count > max_lines)
    {
        first_line = listbox.selected - center_focus;
        if (first_line > ABS(listbox.count - max_lines))
        {
            first_line = ABS(listbox.count - max_lines);
        }
    }

    if (max_lines > listbox.count) max_lines = listbox.count;
    focus = listbox.selected - first_line;
    focus_height = font_height + listbox.line_top_margin + listbox.line_bottom_margin;
    y_line = listbox.y + listbox.line_space;

    for (i = 0; i < max_lines; i++)
    {
        if (i < listbox.count)
        {
            glcd_text(display, listbox.x + listbox.text_left_margin, y_line, listbox.list[first_line + i], listbox.font, listbox.color);

            if (i == focus)
            {
                glcd_rect_invert(display, listbox.x, y_line - listbox.line_top_margin, listbox.width, focus_height);
            }

            y_line += font_height + listbox.line_space;
        }
    }

}


void widget_listbox2(uint8_t display, listbox_t listbox) //FIXME: function hardcoded
{
    glcd_rect_fill(display, listbox.x, listbox.y, listbox.width, listbox.height, ~listbox.color);

    if (listbox.selected > 0)
    {
        glcd_text(display, listbox.x + listbox.text_left_margin, 19, listbox.list[listbox.selected-1], alterebro15, listbox.color);
    }

    if (listbox.selected < (listbox.count - 1))
    {
        glcd_text(display, listbox.x + listbox.text_left_margin, 41, listbox.list[listbox.selected+1], listbox.font, listbox.color);
    }

    glcd_text(display, listbox.x + listbox.text_left_margin + 2, 27, listbox.list[listbox.selected], alterebro24, listbox.color);
    glcd_rect_invert(display, listbox.x, 27, listbox.width, 13);
}


void widget_graph(uint8_t display, graph_t graph)
{
    const uint8_t *graph_table = NULL;
    uint8_t i, n = 0;
    float a, b, x, y, value, min, max;
    textbox_t value_box, unit_box;
    char value_str[16];

    // clear the graph area
    glcd_rect_fill(display, graph.x, graph.y, GRAPH_WIDTH, GRAPH_HEIGHT, ~graph.color);

    // init the value box
    float_to_str(graph.value, value_str, sizeof(value_str), 2);
    value_box.text = value_str;
    value_box.text_color = GLCD_BLACK;
    value_box.font = graph.font;
    value_box.top_margin = 0;
    value_box.bottom_margin = 0;
    value_box.left_margin = 2;
    value_box.right_margin = 2;
    value_box.align = ALIGN_NONE_NONE;
    value_box.x = graph.x;
    value_box.y = graph.y;

    // init the unit box
    unit_box.text_color = GLCD_BLACK;
    unit_box.font = graph.font;
    unit_box.top_margin = 0;
    unit_box.bottom_margin = 0;
    unit_box.left_margin = 0;
    unit_box.right_margin = 0;
    unit_box.text = graph.unit;
    unit_box.align = ALIGN_NONE_NONE;
    unit_box.x = value_box.x + get_text_width(value_str, graph.font) + 4;
    unit_box.y = value_box.y;

    // linear
    // y = a*x + b
    // y1 = 0, y2 = GRAPH_NUM_BARS, x1 = graph.min, x2 = graph.max
    // a = (y2 - y1) / (x2 - x1)
    // b = y1 - (a * x1)
    if (graph.type == 0)
    {
        graph_table = GraphLinTable;
        a = ((float) GRAPH_NUM_BARS) / (graph.max - graph.min);
        b = -(a * graph.min);
        value = a * graph.value + b;
        n = (uint8_t) ROUND(value);

        x = graph.x;

        uint8_t zero, height;

        // zero axis
        zero = (uint8_t) ROUND(b);
        if (zero > GRAPH_NUM_BARS) zero = GRAPH_NUM_BARS;

        for (i = 0; i < GRAPH_NUM_BARS; i++)
        {
            // calculates the y and height a
            if (i < zero)
            {
                y = graph.y + GRAPH_HEIGHT - zero;
                height = zero - graph_table[i];
            }
            else
            {
                y = graph.y + GRAPH_HEIGHT - graph_table[i];
                height = graph_table[i] - zero;
            }

            // draws the full column
            if (i < n)
            {
                glcd_rect_fill(display, x, y, GRAPH_BAR_WIDTH, height, graph.color);
            }
            // draws the empty column
            else
            {
                glcd_vline(display, x + 2, y, height, graph.color);
            }

            x += (GRAPH_BAR_WIDTH + GRAPH_BAR_SPACE);
        }

        // recalculate the textbox position if necessary
        if (zero > graph.font[FONT_HEIGHT])
        {
            unit_box.x = GRAPH_WIDTH - get_text_width(graph.unit, graph.font);
            unit_box.y = graph.y + GRAPH_HEIGHT - graph.font[FONT_HEIGHT];

            value_box.x = unit_box.x - get_text_width(value_box.text, graph.font) - 4;
            value_box.y = unit_box.y;
        }
    }

    // log
    // y = a * log(x) + b
    // y1 = 0, y2 = GRAPH_NUM_BARS, x1 = log(graph.min), x2 = log(graph.max)
    // a = (y2 - y1) / (x2 - x1)
    // b = y1 - (a * x1)
    else if (graph.type == 1)
    {
        graph_table = GraphLogTable;
        min = log2(graph.min);
        max = log2(graph.max);
        a = ((float) GRAPH_NUM_BARS) / (max - min);
        b = -(a * min);
        value = log2(graph.value);
        value = a * value + b;
        n = (uint8_t) ROUND(value);

        x = graph.x;

        for (i = 0; i < GRAPH_NUM_BARS; i++)
        {
            y = graph.y + GRAPH_HEIGHT - graph_table[i];

            if (i < n)
                glcd_rect_fill(display, x, y, GRAPH_BAR_WIDTH, graph_table[i], graph.color);
            else
                glcd_vline(display, x + 2, y, graph_table[i], graph.color);
            x += (GRAPH_BAR_WIDTH + GRAPH_BAR_SPACE);
        }
    }

    // draws the value box
    widget_textbox(display, value_box);

    // draws the unit box
    widget_textbox(display, unit_box);
}
