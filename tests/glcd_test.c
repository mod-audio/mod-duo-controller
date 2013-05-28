

#include "glcd.h"
#include "fonts.h"

static const uint8_t mod_mini[] = {
    32,
    32,
    0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xC0,0xE0,0x60,0x60,0x30,0x30,0x30,0x30,0x30,
	0x30,0x30,0x30,0x30,0x30,0x60,0x60,0xE0,0xC0,0xC0,0x80,0x00,0x00,0x00,0x00,0x00,
	0x00,0xE0,0xF8,0x3E,0x0F,0x03,0x01,0x00,0x00,0x00,0x80,0xBF,0xBF,0xB3,0xA0,0xA1,
	0xBF,0xBF,0xBF,0xBF,0xBF,0x80,0x00,0x00,0x00,0x01,0x03,0x0F,0x3E,0xF8,0xE0,0x00,
	0x00,0x03,0x0F,0x3E,0x78,0xE0,0xC0,0x80,0x80,0x1F,0x3F,0x61,0x40,0x40,0x5F,0x5E,
	0x40,0x40,0x40,0x40,0x61,0x3F,0x1F,0x80,0x80,0xC0,0xE0,0x78,0x3E,0x0F,0x03,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x03,0x03,0x03,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x03,0x03,0x03,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00
};

void glcd_test(void)
{
    glcd_set_pixel(0, 63, 0, GLCD_BLACK);
    glcd_hline(0, 0, 32, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_hline(0, 0, 33, DISPLAY_WIDTH, GLCD_WHITE);
    glcd_hline(0, 0, 34, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    glcd_hline(0, 0, 35, DISPLAY_WIDTH, GLCD_WHITE_BLACK);
    glcd_hline(1, 0, 51, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_vline(0, 64, 0, DISPLAY_HEIGHT, GLCD_BLACK);
    glcd_vline(0, 65, 0, DISPLAY_HEIGHT, GLCD_WHITE);
    glcd_vline(0, 66, 0, DISPLAY_HEIGHT, GLCD_BLACK_WHITE);
    glcd_vline(0, 67, 0, DISPLAY_HEIGHT, GLCD_WHITE_BLACK);
    glcd_rect(0, 0, 0, 10, 10, GLCD_BLACK_WHITE);
    glcd_rect(1, 10, 0, 4, 4, GLCD_BLACK);
    glcd_line(1, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, GLCD_BLACK);
    glcd_line(1, 127, 0, 0, DISPLAY_HEIGHT, GLCD_BLACK_WHITE);
    glcd_rect_fill(2, 0, 0, 10, 32, GLCD_BLACK_WHITE);
    glcd_rect_fill(2, 10, 0, 10, 32, GLCD_WHITE_BLACK);
    glcd_rect_fill(2, 20, 0, 10, 32, GLCD_CHESS);
    glcd_rect_fill(2, 30, 0, 10, 32, GLCD_BLACK);
    glcd_rect_invert(2, 0, 12, DISPLAY_WIDTH, 3);
    glcd_rect_invert(2, 0, 18, DISPLAY_WIDTH, 5);
    glcd_rect_invert(1, 20, 15, 88, 34);
    glcd_draw_image(3, 128-32, 0, mod_mini, GLCD_WHITE);
    glcd_draw_image(2, 128-32, 64-32, mod_mini, GLCD_BLACK);
    glcd_set_pixel(3, 0, 40, GLCD_BLACK);
    glcd_set_pixel(3, 0, 45, GLCD_BLACK);
    glcd_set_pixel(3, 0, 50, GLCD_BLACK);
    glcd_set_pixel(3, 0, 55, GLCD_BLACK);
    glcd_text(2, 0, 40, "ABCpqyrs123M", NULL, GLCD_BLACK);
    glcd_text(2, 0, 50, "ABCpqyrs123M", NULL, GLCD_WHITE);
    glcd_text(3, 2, 50, "One Two Three My", NULL, GLCD_BLACK);
    glcd_text(3, 2, 40, "One Two Three My", alterebro15, GLCD_BLACK);
    glcd_text(3, 0, 20, "ABCpqyrs123M", alterebro15, GLCD_BLACK);
    glcd_text(3, 0, 10, "ABCpqyrs123M", alterebro15, GLCD_BLACK);
    glcd_text(3, 0, 0, "ABCpqyrs123M", alterebro15, GLCD_BLACK);
    glcd_update();
    while(1);
}
