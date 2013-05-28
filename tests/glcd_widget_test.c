
#include "glcd_widget.h"
#include "glcd.h"
#include "fonts.h"

void glcd_widget_textbox_test(void)
{
    textbox_t text1;

    // display 0
    text1.text_color = GLCD_BLACK;
    text1.font = alterebro15;
    text1.top_margin = 0;
    text1.bottom_margin = 0;
    text1.left_margin = 0;
    text1.right_margin = 0;

    text1.text = "TextLT";
    text1.align = ALIGN_LEFT_TOP;
    widget_textbox(0, text1);

    text1.text = "TextCT";
    text1.align = ALIGN_CENTER_TOP;
    widget_textbox(0, text1);

    text1.text = "TextRT";
    text1.align = ALIGN_RIGHT_TOP;
    widget_textbox(0, text1);

    text1.text = "TextLM";
    text1.align = ALIGN_LEFT_MIDDLE;
    widget_textbox(0, text1);

    text1.text = "TextCM";
    text1.align = ALIGN_CENTER_MIDDLE;
    widget_textbox(0, text1);

    text1.text = "TextRM";
    text1.align = ALIGN_RIGHT_MIDDLE;
    widget_textbox(0, text1);

    text1.text = "TextLB";
    text1.align = ALIGN_LEFT_BOTTOM;
    widget_textbox(0, text1);

    text1.text = "TextCB";
    text1.align = ALIGN_CENTER_BOTTOM;
    widget_textbox(0, text1);

    text1.text = "TextRB";
    text1.align = ALIGN_RIGHT_BOTTOM;
    widget_textbox(0, text1);

    // display 1
    text1.text_color = GLCD_WHITE;
    text1.font = alterebro15;
    text1.top_margin = 1;
    text1.bottom_margin = 1;
    text1.left_margin = 1;
    text1.right_margin = 1;

    text1.text = "TextLT";
    text1.align = ALIGN_LEFT_TOP;
    widget_textbox(1, text1);

    text1.text = "TextCT";
    text1.align = ALIGN_CENTER_TOP;
    widget_textbox(1, text1);

    text1.text = "TextRT";
    text1.align = ALIGN_RIGHT_TOP;
    widget_textbox(1, text1);

    text1.text = "TextLM";
    text1.align = ALIGN_LEFT_MIDDLE;
    widget_textbox(1, text1);

    text1.text = "TextCM";
    text1.align = ALIGN_CENTER_MIDDLE;
    widget_textbox(1, text1);

    text1.text = "TextRM";
    text1.align = ALIGN_RIGHT_MIDDLE;
    widget_textbox(1, text1);

    text1.text = "TextLB";
    text1.align = ALIGN_LEFT_BOTTOM;
    widget_textbox(1, text1);

    text1.text = "TextCB";
    text1.align = ALIGN_CENTER_BOTTOM;
    widget_textbox(1, text1);

    text1.text = "TextRB";
    text1.align = ALIGN_RIGHT_BOTTOM;
    widget_textbox(1, text1);

    // display 2
    text1.text_color = GLCD_BLACK;
    text1.font = System5x7;
    text1.top_margin = 1;
    text1.bottom_margin = 1;
    text1.left_margin = 1;
    text1.right_margin = 1;

    text1.text = "TextLT";
    text1.align = ALIGN_LEFT_TOP;
    widget_textbox(2, text1);

    text1.text = "TextCT";
    text1.align = ALIGN_CENTER_TOP;
    widget_textbox(2, text1);

    text1.text = "TextRT";
    text1.align = ALIGN_RIGHT_TOP;
    widget_textbox(2, text1);

    text1.text = "TextLM";
    text1.align = ALIGN_LEFT_MIDDLE;
    widget_textbox(2, text1);

    text1.text = "TextCM";
    text1.align = ALIGN_CENTER_MIDDLE;
    widget_textbox(2, text1);

    text1.text = "TextRM";
    text1.align = ALIGN_RIGHT_MIDDLE;
    widget_textbox(2, text1);

    text1.text = "TextLB";
    text1.align = ALIGN_LEFT_BOTTOM;
    widget_textbox(2, text1);

    text1.text = "TextCB";
    text1.align = ALIGN_CENTER_BOTTOM;
    widget_textbox(2, text1);

    text1.text = "TextRB";
    text1.align = ALIGN_RIGHT_BOTTOM;
    widget_textbox(2, text1);


    glcd_update();

    while(1);
}


void glcd_widget_listbox_test(void)
{
    listbox_t list1;
    uint32_t i;
    const char *list_test[] = {"Fender_Champ", "JCM_800", "VOX_AC30", "Subway_Rocket", "Navy_Blue", "White_Horse", "Big_Bang"};

    list1.x = 0;
    list1.y = 9;
    list1.width = 124;
    list1.height = 40;
    list1.color = GLCD_BLACK;
    list1.selected = 2;
    list1.count = 4;
    list1.list = list_test;
    list1.font = alterebro15;
    list1.line_space = 1;
    list1.line_top_margin = 1;
    list1.line_bottom_margin = 0;
    list1.text_left_margin = 1;
    widget_listbox2(0, list1);

    list1.x = 0;
    list1.y = 9;
    list1.width = 124;
    list1.height = 40;
    list1.color = GLCD_BLACK;
    list1.count = 7;
    list1.list = list_test;
    list1.font = alterebro15;
    list1.line_space = 1;
    list1.line_top_margin = 1;
    list1.line_bottom_margin = 0;
    list1.text_left_margin = 1;

    i = 0;
    while(1)
    {
        list1.selected = i;
        widget_listbox(1, list1);
        DELAY_us(500000);

        i++;
        if (i == list1.count) i = 0;

        glcd_update();
    }
}


void glcd_widget_graph_test(void)
{
    float div = 100.0;

    while (1)
    {
        graph_t graph1;
        graph1.x = 0;
        graph1.y = 28;
        graph1.color = GLCD_BLACK;
        graph1.min = -10.0;
        graph1.max = 10.0;
        graph1.unit = "A";
        graph1.type = GRAPH_TYPE_LINEAR;
        graph1.font = alterebro24;
        graph1.value += ((graph1.max - graph1.min) / div);
        if (graph1.value > graph1.max) graph1.value = graph1.min;
        widget_graph(0, graph1);

        graph_t graph2;
        graph2.x = 0;
        graph2.y = 16;
        graph2.color = GLCD_BLACK;
        graph2.min = 20.0;
        graph2.max = 20000.0;
        graph2.unit = "Hz";
        graph2.type = GRAPH_TYPE_LOG;
        graph2.font = alterebro24;
        graph2.value += ((graph2.max - graph2.min) / div);
        if (graph2.value > graph2.max) graph2.value = graph2.min;
        widget_graph(1, graph2);

        graph_t graph3;
        graph3.x = 0;
        graph3.y = 5;
        graph3.color = GLCD_BLACK;
        graph3.min = -2.0;
        graph3.max = 3.0;
        graph3.unit = "X";
        graph3.type = GRAPH_TYPE_LINEAR;
        graph3.font = alterebro24;
        graph3.value += ((graph3.max - graph3.min) / div);
        if (graph3.value > graph3.max) graph3.value = graph3.min;
        widget_graph(2, graph3);

        graph_t graph4;
        graph4.x = 0;
        graph4.y = 15;
        graph4.color = GLCD_BLACK;
        graph4.min = -3.0;
        graph4.max = 1.5;
        graph4.unit = "mk";
        graph4.type = GRAPH_TYPE_LINEAR;
        graph4.font = alterebro24;
        graph4.value += ((graph4.max - graph4.min) / div);
        if (graph4.value > graph4.max) graph4.value = graph4.min;
        widget_graph(3, graph4);

        glcd_update();
    }
}
