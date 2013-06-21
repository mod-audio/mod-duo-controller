
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef  GLCD_WIDGET_H
#define  GLCD_WIDGET_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "glcd.h"
#include "fonts.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

typedef enum {ALIGN_LEFT_TOP, ALIGN_LEFT_MIDDLE, ALIGN_LEFT_BOTTOM,
              ALIGN_CENTER_TOP,ALIGN_CENTER_MIDDLE, ALIGN_CENTER_BOTTOM,
              ALIGN_RIGHT_TOP, ALIGN_RIGHT_MIDDLE, ALIGN_RIGHT_BOTTOM,
              ALIGN_NONE_NONE, ALIGN_LEFT_NONE, ALIGN_RIGHT_NONE, ALIGN_CENTER_NONE} align_t;

typedef enum {GRAPH_TYPE_LINEAR, GRAPH_TYPE_LOG, GRAPH_TYPE_V, GRAPH_TYPE_A} graph_type_t;


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/


typedef struct TEXTBOX_T {
    uint8_t x, y, text_color;
    align_t align;
    const char *text;
    const uint8_t *font;
    uint8_t top_margin, bottom_margin, right_margin, left_margin;
} textbox_t;

typedef struct LISTBOX_T {
    uint8_t x, y, width, height, color;
    uint8_t selected, count;
    const char **list;
    const uint8_t *font;
    uint8_t line_space, line_top_margin, line_bottom_margin;
    uint8_t text_left_margin;
} listbox_t;

typedef struct GRAPH_T {
    uint8_t x, y, width, height;
    uint8_t color;
    float min, max, value;
    const char *unit;
    const uint8_t *font;
    graph_type_t type;
} graph_t;

typedef struct PEAKMETER_T {
    float peak1, peak2, peak3, peak4;
    float value1, value2, value3, value4;
    const uint8_t *font;
} peakmeter_t;

typedef struct TUNER_T {
    const uint8_t *font;
    float frequency;
    char *note;
    int8_t cents;
} tuner_t;


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/

void widget_textbox(uint8_t display, textbox_t textbox);
void widget_listbox(uint8_t display, listbox_t listbox);
void widget_listbox2(uint8_t display, listbox_t listbox);
void widget_graph(uint8_t display, graph_t graph);
void widget_peakmeter(uint8_t display, peakmeter_t *pkm);
void widget_tuner(uint8_t display, tuner_t *tuner);


/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
