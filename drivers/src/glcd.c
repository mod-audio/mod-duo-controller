
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
#include "fonts.h"


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

// function bits definition
// this is used to block/unblock the glcd functions
enum {SET_PIXEL, CLEAR, HLINE, VLINE, LINE, RECT, RECT_FILL, RECT_INVERT, DRAW_IMAGE, TEXT};


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
#define READ_BUFFER(disp,x,y)           (((x) < DISPLAY_WIDTH && (y) < DISPLAY_HEIGHT) ? \
                                        g_buffer[(disp)][((y)/8)][(x)] : 0)
#define WRITE_BUFFER(disp,x,y,data)     if ((x) < DISPLAY_WIDTH && (y) < DISPLAY_HEIGHT) \
                                        g_buffer[(disp)][((y)/8)][(x)] = (data); g_need_update[(disp)] = 1

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

// block/unblock macros
#define BLOCK(disp,func)                g_blocked[(disp)] |= (1 << (func))
#define UNBLOCK(disp,func)              g_blocked[(disp)] &= ~(1 << (func))
#define IS_BLOCKED(disp,func)           (g_blocked[(disp)] & (1 << (func)))


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static uint8_t g_buffer[DISPLAY_COUNT][DISPLAY_HEIGHT/8][DISPLAY_WIDTH];
static uint8_t g_need_update[DISPLAY_COUNT];
static uint32_t g_blocked[DISPLAY_COUNT];


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

static uint8_t read_data(uint8_t display, uint8_t x, uint8_t y)
{
    uint8_t data, data_tmp, y_offset;

    y_offset = (y % 8);

    if (y_offset != 0)
    {
        // first page
        data_tmp = READ_BUFFER(display, x, y);
        data = (data_tmp >> y_offset);

        // second page
        data_tmp = READ_BUFFER(display, x, y+8);
        data |= (data_tmp << (8 - y_offset));

        return data;
    }
    else
    {
        return READ_BUFFER(display, x, y);
    }
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
        g_need_update[i] = 0;
        g_blocked[i] = 0;
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
    if (IS_BLOCKED(display,CLEAR)) return;
    BLOCK(display,CLEAR);

    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);

    UNBLOCK(display,CLEAR);
}

void glcd_set_pixel(uint8_t display, uint8_t x, uint8_t y, uint8_t color)
{
    uint8_t data;

    if (IS_BLOCKED(display,SET_PIXEL)) return;
    BLOCK(display,SET_PIXEL);

    data = READ_BUFFER(display, x, y);
    if (color == GLCD_BLACK) data |= (0x01 << (y % 8));
    else data &= ~(0x01 << (y % 8));
    WRITE_BUFFER(display, x, y, data);

    UNBLOCK(display,SET_PIXEL);
}

void glcd_hline(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    if (IS_BLOCKED(display,HLINE)) return;
    BLOCK(display,HLINE);

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
    }

    UNBLOCK(display,HLINE);
}

void glcd_vline(uint8_t display, uint8_t x, uint8_t y, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    if (IS_BLOCKED(display,VLINE)) return;
    BLOCK(display,VLINE);

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
    }

    UNBLOCK(display,VLINE);
}

void glcd_line(uint8_t display, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    uint8_t deltax, deltay, x, y, steep;
    int8_t error, ystep;

    if (IS_BLOCKED(display,LINE)) return;
    BLOCK(display,LINE);

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

    UNBLOCK(display,LINE);
}

void glcd_rect(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    if (IS_BLOCKED(display,RECT)) return;
    BLOCK(display,RECT);

    glcd_hline(display, x, y, width, color);
    glcd_hline(display, x, y+height-1, width, color);
    glcd_vline(display, x, y, height, color);
    glcd_vline(display, x+width-1, y, height, color);

    UNBLOCK(display,RECT);
}

void glcd_rect_fill(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    if (IS_BLOCKED(display,RECT_FILL)) return;
    BLOCK(display,RECT_FILL);

    while (width--)
    {
        if (color == GLCD_CHESS)
        {
            if ((i % 2) == 0) tmp = GLCD_BLACK_WHITE;
            else tmp = GLCD_WHITE_BLACK;
        }
        i++;

        glcd_vline(display, x++, y, height, tmp);
    }

    UNBLOCK(display,RECT_FILL);
}

void glcd_rect_invert(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t mask, page_offset, h, i, data, data_tmp, x_tmp;

    if (IS_BLOCKED(display,RECT_INVERT)) return;
    BLOCK(display,RECT_INVERT);

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

            data = READ_BUFFER(display, x_tmp, y);
            data_tmp = ~data;
            data = (data_tmp & mask) | (data & ~mask);
            WRITE_BUFFER(display, x_tmp, y, data);
        }
    }

    UNBLOCK(display,RECT_INVERT);
}

void glcd_draw_image(uint8_t display, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color)
{
    uint8_t i, j, height, width;
    char data;

    if (IS_BLOCKED(display,DRAW_IMAGE)) return;
    BLOCK(display,DRAW_IMAGE);

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

    UNBLOCK(display,DRAW_IMAGE);
}

void glcd_text(uint8_t display, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color)
{
    uint8_t i, j, x_tmp, y_tmp, c, bytes, data;
    uint16_t char_data_index, page;

    if (IS_BLOCKED(display,TEXT)) return;
    BLOCK(display,TEXT);

    // default font
    if (!font) font = FONT_DEFAULT;
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
                    data >>= ((j + 1) * 8) - height;
                    data = read_data(display, x_tmp, y_tmp) | data;
                }

                write_data(display, x_tmp, y_tmp, data);

                x_tmp++;
            }

            // draws the interchar space
            data = (color == GLCD_BLACK ? 0x00 : 0xFF);
            if (height > 8 && height < (j + 1) * 8)
            {
                data >>= ((j + 1) * 8) - height;
                data = read_data(display, x_tmp, y_tmp) | data;
            }

            if (*(text + 1) != '\0')
            {
                write_data(display, x_tmp, y_tmp, data);
            }

            y_tmp += 8;
        }

        x += width + 1;

        text++;
    }

    UNBLOCK(display,TEXT);
}

void glcd_update(void)
{
    uint8_t i;

    for (i = 0; i < DISPLAY_COUNT; i++)
    {
        if (g_need_update[i] && g_blocked[i] == 0)
        {
            switcher_channel(i);
            write_frame(i);
            g_need_update[i] = 0;
        }
    }
}
