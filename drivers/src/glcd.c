
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "glcd.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

// chip defines
#define CHIP_HEIGHT     64
#define CHIP_WIDTH      64

// displays instructions
#define DISPLAY_ON      0x3F
#define DISPLAY_OFF     0x3E
#define DISPLAY_START   0xC0
#define BUSY_FLAG       0x80
#define SET_PAGE        0xB8
#define SET_ADDRESS     0x40

// display timing defines (in nanoseconds)
#define DELAY_DDR       320    // Data Delay time (E high to valid read data)
#define DELAY_AS        140    // Address setup time (ctrl line changes to E HIGH
#define DELAY_DSW       200    // Data setup time (data lines setup to dropping E)
#define DELAY_WH        450    // E hi level width (minimum E hi pulse width)
#define DELAY_WL        450    // E lo level width (minimum E lo pulse width)

// fonts definitions
#define FONT_LENGTH             0
#define FONT_FIXED_WIDTH        2
#define FONT_HEIGHT             3
#define FONT_FIRST_CHAR         4
#define FONT_CHAR_COUNT         5
#define FONT_WIDTH_TABLE        6


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

// default font (system 5x7)
static const uint8_t g_default_font[] = {
    0x00, 0x00,     // size of zero indicates fixed width font, actual length is width * height
    0x05,           // width
    0x07,           // height
    0x20,           // first char
    0x60,           // char count

    // Fixed width; char width table not used !!!!

    // font data
    0x00, 0x00, 0x00, 0x00, 0x00,// (space)
    0x00, 0x00, 0x5F, 0x00, 0x00,// !
    0x00, 0x07, 0x00, 0x07, 0x00,// "
    0x14, 0x7F, 0x14, 0x7F, 0x14,// #
    0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
    0x23, 0x13, 0x08, 0x64, 0x62,// %
    0x36, 0x49, 0x55, 0x22, 0x50,// &
    0x00, 0x05, 0x03, 0x00, 0x00,// '
    0x00, 0x1C, 0x22, 0x41, 0x00,// (
    0x00, 0x41, 0x22, 0x1C, 0x00,// )
    0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
    0x08, 0x08, 0x3E, 0x08, 0x08,// +
    0x00, 0x50, 0x30, 0x00, 0x00,// ,
    0x08, 0x08, 0x08, 0x08, 0x08,// -
    0x00, 0x60, 0x60, 0x00, 0x00,// .
    0x20, 0x10, 0x08, 0x04, 0x02,// /
    0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
    0x00, 0x42, 0x7F, 0x40, 0x00,// 1
    0x42, 0x61, 0x51, 0x49, 0x46,// 2
    0x21, 0x41, 0x45, 0x4B, 0x31,// 3
    0x18, 0x14, 0x12, 0x7F, 0x10,// 4
    0x27, 0x45, 0x45, 0x45, 0x39,// 5
    0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
    0x01, 0x71, 0x09, 0x05, 0x03,// 7
    0x36, 0x49, 0x49, 0x49, 0x36,// 8
    0x06, 0x49, 0x49, 0x29, 0x1E,// 9
    0x00, 0x36, 0x36, 0x00, 0x00,// :
    0x00, 0x56, 0x36, 0x00, 0x00,// ;
    0x00, 0x08, 0x14, 0x22, 0x41,// <
    0x14, 0x14, 0x14, 0x14, 0x14,// =
    0x41, 0x22, 0x14, 0x08, 0x00,// >
    0x02, 0x01, 0x51, 0x09, 0x06,// ?
    0x32, 0x49, 0x79, 0x41, 0x3E,// @
    0x7E, 0x11, 0x11, 0x11, 0x7E,// A
    0x7F, 0x49, 0x49, 0x49, 0x36,// B
    0x3E, 0x41, 0x41, 0x41, 0x22,// C
    0x7F, 0x41, 0x41, 0x22, 0x1C,// D
    0x7F, 0x49, 0x49, 0x49, 0x41,// E
    0x7F, 0x09, 0x09, 0x01, 0x01,// F
    0x3E, 0x41, 0x41, 0x51, 0x32,// G
    0x7F, 0x08, 0x08, 0x08, 0x7F,// H
    0x00, 0x41, 0x7F, 0x41, 0x00,// I
    0x20, 0x40, 0x41, 0x3F, 0x01,// J
    0x7F, 0x08, 0x14, 0x22, 0x41,// K
    0x7F, 0x40, 0x40, 0x40, 0x40,// L
    0x7F, 0x02, 0x04, 0x02, 0x7F,// M
    0x7F, 0x04, 0x08, 0x10, 0x7F,// N
    0x3E, 0x41, 0x41, 0x41, 0x3E,// O
    0x7F, 0x09, 0x09, 0x09, 0x06,// P
    0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
    0x7F, 0x09, 0x19, 0x29, 0x46,// R
    0x46, 0x49, 0x49, 0x49, 0x31,// S
    0x01, 0x01, 0x7F, 0x01, 0x01,// T
    0x3F, 0x40, 0x40, 0x40, 0x3F,// U
    0x1F, 0x20, 0x40, 0x20, 0x1F,// V
    0x7F, 0x20, 0x18, 0x20, 0x7F,// W
    0x63, 0x14, 0x08, 0x14, 0x63,// X
    0x03, 0x04, 0x78, 0x04, 0x03,// Y
    0x61, 0x51, 0x49, 0x45, 0x43,// Z
    0x00, 0x00, 0x7F, 0x41, 0x41,// [
    0x02, 0x04, 0x08, 0x10, 0x20,// "\"
    0x41, 0x41, 0x7F, 0x00, 0x00,// ]
    0x04, 0x02, 0x01, 0x02, 0x04,// ^
    0x40, 0x40, 0x40, 0x40, 0x40,// _
    0x00, 0x01, 0x02, 0x04, 0x00,// `
    0x20, 0x54, 0x54, 0x54, 0x78,// a
    0x7F, 0x48, 0x44, 0x44, 0x38,// b
    0x38, 0x44, 0x44, 0x44, 0x20,// c
    0x38, 0x44, 0x44, 0x48, 0x7F,// d
    0x38, 0x54, 0x54, 0x54, 0x18,// e
    0x08, 0x7E, 0x09, 0x01, 0x02,// f
    0x08, 0x14, 0x54, 0x54, 0x3C,// g
    0x7F, 0x08, 0x04, 0x04, 0x78,// h
    0x00, 0x44, 0x7D, 0x40, 0x00,// i
    0x20, 0x40, 0x44, 0x3D, 0x00,// j
    0x00, 0x7F, 0x10, 0x28, 0x44,// k
    0x00, 0x41, 0x7F, 0x40, 0x00,// l
    0x7C, 0x04, 0x18, 0x04, 0x78,// m
    0x7C, 0x08, 0x04, 0x04, 0x78,// n
    0x38, 0x44, 0x44, 0x44, 0x38,// o
    0x7C, 0x14, 0x14, 0x14, 0x08,// p
    0x08, 0x14, 0x14, 0x18, 0x7C,// q
    0x7C, 0x08, 0x04, 0x04, 0x08,// r
    0x48, 0x54, 0x54, 0x54, 0x20,// s
    0x04, 0x3F, 0x44, 0x40, 0x20,// t
    0x3C, 0x40, 0x40, 0x20, 0x7C,// u
    0x1C, 0x20, 0x40, 0x20, 0x1C,// v
    0x3C, 0x40, 0x30, 0x40, 0x3C,// w
    0x44, 0x28, 0x10, 0x28, 0x44,// x
    0x0C, 0x50, 0x50, 0x50, 0x3C,// y
    0x44, 0x64, 0x54, 0x4C, 0x44,// z
    0x00, 0x08, 0x36, 0x41, 0x00,// {
    0x00, 0x00, 0x7F, 0x00, 0x00,// |
    0x00, 0x41, 0x36, 0x08, 0x00,// }
    0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
    0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
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

// operation modes macros
#define INSTRUCTION_MODE()              {CLR_PIN(GLCD_DI_PORT, GLCD_DI_PIN); CLR_PIN(GLCD_RW_PORT, GLCD_RW_PIN);}
#define STATUS_MODE()                   {CLR_PIN(GLCD_DI_PORT, GLCD_DI_PIN); SET_PIN(GLCD_RW_PORT, GLCD_RW_PIN);}
#define WRITE_DATA_MODE()               {SET_PIN(GLCD_DI_PORT, GLCD_DI_PIN); CLR_PIN(GLCD_RW_PORT, GLCD_RW_PIN);}
#define READ_DATA_MODE()                {SET_PIN(GLCD_DI_PORT, GLCD_DI_PIN); SET_PIN(GLCD_RW_PORT, GLCD_RW_PIN);}

// pins control macros
#define ENABLE_HIGH()                   SET_PIN(GLCD_EN_PORT, GLCD_EN_PIN)
#define ENABLE_LOW()                    CLR_PIN(GLCD_EN_PORT, GLCD_EN_PIN)
#define ENABLE_PULSE()                  {ENABLE_HIGH(); DELAY_ns(DELAY_WH); ENABLE_LOW();}
#define CS1_ON()                        SET_PIN(GLCD_CS1_PORT, GLCD_CS1_PIN)
#define CS1_OFF()                       CLR_PIN(GLCD_CS1_PORT, GLCD_CS1_PIN)
#define CS2_ON()                        SET_PIN(GLCD_CS2_PORT, GLCD_CS2_PIN)
#define CS2_OFF()                       CLR_PIN(GLCD_CS2_PORT, GLCD_CS2_PIN)

// buffer macros
#define READ_BUFFER(display,x,y)        g_buffer[display][(y/8)][x]
#define WRITE_BUFFER(display,x,y,data)  g_buffer[display][(y/8)][x] = data

// backlight macros
#if defined GLCD_BACKLIGHT_TURN_ON_WITH_ONE
#define BACKLIGHT_TURN_ON(port, pin)    SET_PIN(port, pin)
#define BACKLIGHT_TURN_OFF(port, pin)   CLR_PIN(port, pin)
#elif defined GLCD_BACKLIGHT_TURN_ON_WITH_ZERO
#define BACKLIGHT_TURN_ON(port, pin)    CLR_PIN(port, pin)
#define BACKLIGHT_TURN_OFF(port, pin)   SET_PIN(port, pin)
#endif

// general purpose macros
#define ABS_DIFF(a, b)                  ((a > b) ? (a - b) : (b - a))
#define SWAP(a, b)                      do{uint8_t t; t = a; a = b; b = t;} while(0)

// fonts macros
#define FONT_IS_MONO_SPACED(font)       ((font)[FONT_LENGTH] == 0 && (font)[FONT_LENGTH+1] == 0)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static uint8_t g_buffer[DISPLAY_COUNT][DISPLAY_HEIGHT/8][DISPLAY_WIDTH];


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

static void switcher_init(void)
{
    CONFIG_PIN_OUTPUT(SWITCHER_DIR_PORT, SWITCHER_DIR_PIN);
    CONFIG_PIN_OUTPUT(SWITCHER_CH0_PORT, SWITCHER_CH0_PIN);
    CONFIG_PIN_OUTPUT(SWITCHER_CH1_PORT, SWITCHER_CH1_PIN);
    CONFIG_PIN_OUTPUT(SWITCHER_CH2_PORT, SWITCHER_CH2_PIN);
    CONFIG_PIN_OUTPUT(SWITCHER_CH3_PORT, SWITCHER_CH3_PIN);

    // select output direction
    SET_PIN(SWITCHER_DIR_PORT, SWITCHER_DIR_PIN);

    // de-select all channels
    SET_PIN(SWITCHER_CH0_PORT, SWITCHER_CH0_PIN);
    SET_PIN(SWITCHER_CH1_PORT, SWITCHER_CH1_PIN);
    SET_PIN(SWITCHER_CH2_PORT, SWITCHER_CH2_PIN);
    SET_PIN(SWITCHER_CH3_PORT, SWITCHER_CH3_PIN);
}

static void switcher_channel(uint8_t channel)
{
    static uint8_t last_channel;

    switch (last_channel)
    {
        case 0:
            SET_PIN(SWITCHER_CH0_PORT, SWITCHER_CH0_PIN);
            break;

        case 1:
            SET_PIN(SWITCHER_CH1_PORT, SWITCHER_CH1_PIN);
            break;

        case 2:
            SET_PIN(SWITCHER_CH2_PORT, SWITCHER_CH2_PIN);
            break;

        case 3:
            SET_PIN(SWITCHER_CH3_PORT, SWITCHER_CH3_PIN);
            break;

        default:
            SET_PIN(SWITCHER_CH0_PORT, SWITCHER_CH0_PIN);
            SET_PIN(SWITCHER_CH1_PORT, SWITCHER_CH1_PIN);
            SET_PIN(SWITCHER_CH2_PORT, SWITCHER_CH2_PIN);
            SET_PIN(SWITCHER_CH3_PORT, SWITCHER_CH3_PIN);
    }

    last_channel = channel;

    switch (channel)
    {
        case 0:
            CLR_PIN(SWITCHER_CH0_PORT, SWITCHER_CH0_PIN);
            break;

        case 1:
            CLR_PIN(SWITCHER_CH1_PORT, SWITCHER_CH1_PIN);
            break;

        case 2:
            CLR_PIN(SWITCHER_CH2_PORT, SWITCHER_CH2_PIN);
            break;

        case 3:
            CLR_PIN(SWITCHER_CH3_PORT, SWITCHER_CH3_PIN);
            break;

        default:
            CLR_PIN(SWITCHER_CH0_PORT, SWITCHER_CH0_PIN);
            CLR_PIN(SWITCHER_CH1_PORT, SWITCHER_CH1_PIN);
            CLR_PIN(SWITCHER_CH2_PORT, SWITCHER_CH2_PIN);
            CLR_PIN(SWITCHER_CH3_PORT, SWITCHER_CH3_PIN);
    }


}

static void chip_select(uint8_t chip)
{
    if (chip == 0)
    {
        CS1_ON();
        CS2_OFF();
    }
    else if (chip == 1)
    {
        CS2_ON();
        CS1_OFF();
    }
}

static void write_instruction(uint8_t chip, uint8_t data)
{
    chip_select(chip);
    INSTRUCTION_MODE();
    WRITE_PORT(GLCD_DATABUS_PORT, data);
    DELAY_ns(DELAY_AS);
    ENABLE_PULSE();
}

static void write_data(uint8_t display, uint8_t x, uint8_t y, uint8_t data)
{
    uint8_t data_tmp, y_offset;

    y_offset = (y % 8);

    if (y_offset != 0)
    {
        // first page
        data_tmp = READ_BUFFER(display, x, y);
        data_tmp |= (data << y_offset);
        WRITE_BUFFER(display, x, y, data_tmp);

        // second page
        y += 8;
        data_tmp = READ_BUFFER(display, x, y);
        data_tmp |= data >> (8 - y_offset);
        WRITE_BUFFER(display, x, y, data_tmp);
    }
    else
    {
        WRITE_BUFFER(display, x, y, data);
    }
}

static void write_frame(uint8_t display)
{
    uint8_t chip, page, x;

    for (chip = 0 ; chip < DISPLAY_CHIP_COUNT; chip++)
    {
        for (page = 0; page < (DISPLAY_HEIGHT/8); page++)
        {
            write_instruction(chip, SET_PAGE | page);
            write_instruction(chip, SET_ADDRESS);

            for (x = 0; x < CHIP_WIDTH; x++)
            {
                // write data on display
                chip_select(chip);
                WRITE_DATA_MODE();
                DELAY_ns(DELAY_AS);
                WRITE_PORT(GLCD_DATABUS_PORT, g_buffer[display][page][x + (chip*CHIP_WIDTH)]);
                DELAY_ns(DELAY_WH);
                ENABLE_PULSE();
            }
        }
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void glcd_init(void)
{
    uint8_t i;

    switcher_init();
    switcher_channel(DISPLAY_COUNT);

    // direction pins configuration
    CONFIG_PIN_OUTPUT(GLCD_RST_PORT, GLCD_RST_PIN);
    CONFIG_PIN_OUTPUT(GLCD_RW_PORT, GLCD_RW_PIN);
    CONFIG_PIN_OUTPUT(GLCD_DI_PORT, GLCD_DI_PIN);
    CONFIG_PIN_OUTPUT(GLCD_EN_PORT, GLCD_EN_PIN);
    CONFIG_PIN_OUTPUT(GLCD_CS1_PORT, GLCD_CS1_PIN);
    CONFIG_PIN_OUTPUT(GLCD_CS2_PORT, GLCD_CS2_PIN);
    CONFIG_PORT_OUTPUT(GLCD_DATABUS_PORT);

    // backlight pins configuration
    CONFIG_PIN_OUTPUT(GLCD0_BACKLIGHT_PORT, GLCD0_BACKLIGHT_PIN);
    CONFIG_PIN_OUTPUT(GLCD1_BACKLIGHT_PORT, GLCD1_BACKLIGHT_PIN);
    CONFIG_PIN_OUTPUT(GLCD2_BACKLIGHT_PORT, GLCD2_BACKLIGHT_PIN);
    CONFIG_PIN_OUTPUT(GLCD3_BACKLIGHT_PORT, GLCD3_BACKLIGHT_PIN);
    BACKLIGHT_TURN_ON(GLCD0_BACKLIGHT_PORT, GLCD0_BACKLIGHT_PIN);
    BACKLIGHT_TURN_ON(GLCD1_BACKLIGHT_PORT, GLCD1_BACKLIGHT_PIN);
    BACKLIGHT_TURN_ON(GLCD2_BACKLIGHT_PORT, GLCD2_BACKLIGHT_PIN);
    BACKLIGHT_TURN_ON(GLCD3_BACKLIGHT_PORT, GLCD3_BACKLIGHT_PIN);

    // initial pins state
    ENABLE_LOW();
    CS1_OFF();
    CS2_OFF();
    INSTRUCTION_MODE();

    // reset
    CLR_PIN(GLCD_RST_PORT, GLCD_RST_PIN);
    DELAY_us(10000);
    SET_PIN(GLCD_RST_PORT, GLCD_RST_PIN);
    DELAY_us(50000);

    // chip initialization
    for (i = 0; i < DISPLAY_CHIP_COUNT; i++)
    {
        write_instruction(i, DISPLAY_ON);
        write_instruction(i, DISPLAY_START);
    }

    for (i = 0; i < DISPLAY_COUNT; i++)
    {
        glcd_clear(i, GLCD_WHITE);
        write_frame(i);
    }
}

void glcd_backlight(uint8_t display, uint8_t state)
{
    if (state)
    {
        if (display == 0) BACKLIGHT_TURN_ON(GLCD0_BACKLIGHT_PORT, GLCD0_BACKLIGHT_PIN);
        else if (display == 1) BACKLIGHT_TURN_ON(GLCD1_BACKLIGHT_PORT, GLCD1_BACKLIGHT_PIN);
        else if (display == 2) BACKLIGHT_TURN_ON(GLCD2_BACKLIGHT_PORT, GLCD2_BACKLIGHT_PIN);
        else if (display == 3) BACKLIGHT_TURN_ON(GLCD3_BACKLIGHT_PORT, GLCD3_BACKLIGHT_PIN);
    }
    else
    {
        if (display == 0) BACKLIGHT_TURN_OFF(GLCD0_BACKLIGHT_PORT, GLCD0_BACKLIGHT_PIN);
        else if (display == 1) BACKLIGHT_TURN_OFF(GLCD1_BACKLIGHT_PORT, GLCD1_BACKLIGHT_PIN);
        else if (display == 2) BACKLIGHT_TURN_OFF(GLCD2_BACKLIGHT_PORT, GLCD2_BACKLIGHT_PIN);
        else if (display == 3) BACKLIGHT_TURN_OFF(GLCD3_BACKLIGHT_PORT, GLCD3_BACKLIGHT_PIN);
    }
}

void glcd_clear(uint8_t display, uint8_t color)
{
    uint8_t i, j;

    for (i = 0; i < DISPLAY_HEIGHT/8; i++)
    {
        for (j = 0; j < DISPLAY_WIDTH; j++)
        {
            g_buffer[display][i][j] = color;
        }
    }
}

void glcd_set_pixel(uint8_t display, uint8_t x, uint8_t y, uint8_t color)
{
    if ((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT)) return;

    uint8_t data;
    data = READ_BUFFER(display, x, y);
    if (color == GLCD_BLACK) data |= (0x01 << (y % 8));
    else data &= ~(0x01 << (y % 8));
    WRITE_BUFFER(display, x, y, data);
}

void glcd_hline(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == GLCD_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = GLCD_BLACK;
            else tmp = GLCD_WHITE;
        }
        else if (color == GLCD_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = GLCD_WHITE;
            else tmp = GLCD_BLACK;
        }
        i++;

        glcd_set_pixel(display, x++, y, tmp);
        if (x >= DISPLAY_WIDTH) break;
    }
}

void glcd_vline(uint8_t display, uint8_t x, uint8_t y, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (height--)
    {
        if (color == GLCD_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = GLCD_BLACK;
            else tmp = GLCD_WHITE;
        }
        else if (color == GLCD_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = GLCD_WHITE;
            else tmp = GLCD_BLACK;
        }
        i++;

        glcd_set_pixel(display, x, y++, tmp);
        if (y >= DISPLAY_HEIGHT) break;
    }
}

void glcd_line(uint8_t display, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    uint8_t deltax, deltay, x, y, steep;
    int8_t error, ystep;

    steep = ABS_DIFF(y1, y2) > ABS_DIFF(x1, x2);

    if (steep)
    {
        SWAP(x1, y1);
        SWAP(x2, y2);
    }

    if (x1 > x2)
    {
        SWAP(x1, x2);
        SWAP(y1, y2);
    }

    deltax = x2 - x1;
    deltay = ABS_DIFF(y2, y1);
    error = deltax / 2;
    y = y1;
    if (y1 < y2) ystep = 1;
    else ystep = -1;

    uint8_t i = 0, tmp = color;

    for (x = x1; x <= x2; x++)
    {
        if (color == GLCD_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = GLCD_BLACK;
            else tmp = GLCD_WHITE;
        }
        else if (color == GLCD_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = GLCD_WHITE;
            else tmp = GLCD_BLACK;
        }
        i++;

        if (steep) glcd_set_pixel(display, y, x, tmp);
        else glcd_set_pixel(display, x, y, tmp);

        error = error - deltay;
        if (error < 0)
        {
            y = y + ystep;
            error = error + deltax;
        }
    }
}

void glcd_rect(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    glcd_hline(display, x, y, width, color);
    glcd_hline(display, x, y+height-1, width, color);
    glcd_vline(display, x, y, height, color);
    glcd_vline(display, x+width-1, y, height, color);
}

void glcd_rect_fill(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == GLCD_CHESS)
        {
            if ((i % 2) == 0) tmp = GLCD_BLACK_WHITE;
            else tmp = GLCD_WHITE_BLACK;
        }
        i++;

        glcd_vline(display, x++, y, height, tmp);
        if (x >= DISPLAY_WIDTH) break;
    }
}

void glcd_rect_invert(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t mask, page_offset, h, i, data, data_tmp, x_tmp;

    page_offset = y % 8;
    mask = 0xFF;
    if (height < (8 - page_offset))
    {
        mask >>= (8 - height);
        h = height;
    }
    else
    {
        h = 8 - page_offset;
    }
    mask <<= page_offset;

    // First do the fractional pages at the top of the region
    for (i = 0; i < width; i++)
    {
        x_tmp = x + i;
        if (x_tmp >= DISPLAY_WIDTH) break;

        data = READ_BUFFER(display, x_tmp, y);
        data_tmp = ~data;
        data = (data_tmp & mask) | (data & ~mask);
        WRITE_BUFFER(display, x_tmp, y, data);
    }

    // Now do the full pages
    while((h + 8) <= height)
    {
        h += 8;
        y += 8;

        for (i = 0; i < width; i++)
        {
            x_tmp = x + i;
            if (x_tmp >= DISPLAY_WIDTH) break;

            data = READ_BUFFER(display, x_tmp, y);
            WRITE_BUFFER(display, x_tmp, y, ~data);
        }
    }

    // Now do the fractional pages at the bottom of the region
    if (h < height)
    {
        mask = ~(0xFF << (height-h));
        y += 8;

        for (i = 0; i < width; i++)
        {
            x_tmp = x + i;
            if (x_tmp >= DISPLAY_WIDTH) break;

            data = READ_BUFFER(display, x_tmp, y);
            data_tmp = ~data;
            data = (data_tmp & mask) | (data & ~mask);
            WRITE_BUFFER(display, x_tmp, y, data);
        }
    }
}

void glcd_draw_image(uint8_t display, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color)
{
    uint8_t i, j, height, width;
    char data;

    width = (uint8_t) *image++;
    height = (uint8_t) *image++;

    for (j = 0; j < height; j += 8)
    {
        for(i = 0; i < width; i++)
        {
            data = *image++;
            if (color == GLCD_WHITE) data = ~data;
            WRITE_BUFFER(display, x+i, y+j, data);
        }
    }
}

void glcd_text(uint8_t display, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color)
{
    uint8_t i, j, x_tmp, y_tmp, c, bytes, data, mask;
    uint16_t char_data_index, page;

    if (!font) font = g_default_font;
    uint8_t width = font[FONT_FIXED_WIDTH];
    uint8_t height = font[FONT_HEIGHT];
    uint8_t first_char = font[FONT_FIRST_CHAR];
    uint8_t char_count = font[FONT_CHAR_COUNT];

    while (*text)
    {
        c = *text;
        if (c < first_char || c >= (first_char + char_count))
        {
            text++;
            continue;
        }

        c -= first_char;
        bytes = (height + 7) / 8;

        if (FONT_IS_MONO_SPACED(font))
        {
            char_data_index = (c * width * bytes) + FONT_WIDTH_TABLE;
        }
        else
        {
            width = font[FONT_WIDTH_TABLE + c];
            char_data_index = 0;
            for (i = 0; i < c; i++) char_data_index += font[FONT_WIDTH_TABLE + i];
            char_data_index = (char_data_index * bytes) + FONT_WIDTH_TABLE + char_count;
        }

        y_tmp = y;

        // draws each character piece
        for (j = 0; j < bytes; j++)
        {
            x_tmp = x;
            page = j * width;

            // draws the character
            for (i = 0; i < width; i++)
            {
                data = font[char_data_index + page + i];
                if (color == GLCD_WHITE) data = ~data;

                // if is the last piece of character...
                if (height > 8 && height < (j + 1) * 8)
                {
                    mask = 0xFF << ((y + height) % 8);
                    data >>= ((j + 1) * 8) - height;
                    data |= (READ_BUFFER(display, x_tmp, y_tmp) & mask);
                    WRITE_BUFFER(display, x_tmp, y_tmp, data);
                }
                else
                {
                    write_data(display, x_tmp, y_tmp, data);
                }

                x_tmp++;
            }

            // draws the interchar space
            data = (color == GLCD_BLACK ? 0x00 : 0xFF);
            if (height > 8 && height < (j + 1) * 8)
            {
                mask = 0xFF << ((y + height) % 8);
                data >>= ((j + 1) * 8) - height;
                data |= (READ_BUFFER(display, x_tmp, y_tmp) & mask);
                WRITE_BUFFER(display, x_tmp, y_tmp, data);
            }
            else if (((x + width) < DISPLAY_WIDTH) && (*(text + 1) != '\0'))
            {
                write_data(display, x_tmp, y_tmp, data);
            }

            y_tmp += 8;
        }

        x += width + 1;

        text++;
    }
}

void glcd_update(void)
{
    uint8_t i;

    for (i = 0; i < DISPLAY_COUNT; i++)
    {
        switcher_channel(i);
        write_frame(i);
    }
}
