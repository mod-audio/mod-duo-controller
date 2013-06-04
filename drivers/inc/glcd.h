
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef  GLCD_H
#define  GLCD_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "lpc177x_8x_gpio.h"
#include "config.h"
#include "utils.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

// colors
#define GLCD_WHITE          0
#define GLCD_BLACK          1
#define GLCD_BLACK_WHITE    2
#define GLCD_WHITE_BLACK    3
#define GLCD_CHESS          4

// displays
#define ALL_DISPLAYS        0xFF
#define DISPLAY_0           0x00
#define DISPLAY_1           0x01
#define DISPLAY_2           0x02
#define DISPLAY_3           0x03

// backlight
#define BACKLIGHT_ON        1
#define BACKLIGHT_OFF       0


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

// display configuration
#define DISPLAY_COUNT           GLCD_COUNT // GLCD_COUNT is defined in config.h
#define DISPLAY_WIDTH           128
#define DISPLAY_HEIGHT          64
#define DISPLAY_CHIP_COUNT      2

// delay macros definition
#define DELAY_ns(time)          do {volatile uint32_t __delay = (time/10); while (__delay--);} while(0)
#define DELAY_us(time)          delay_us(time)

// I/O macros configuration
#define CONFIG_PIN_INPUT(port, pin)     GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_INPUT)
#define CONFIG_PIN_OUTPUT(port, pin)    GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_OUTPUT)
#define CONFIG_PORT_INPUT(port)         FIO_ByteSetDir((port), 0, 0xFF, GPIO_DIRECTION_INPUT)
#define CONFIG_PORT_OUTPUT(port)        FIO_ByteSetDir((port), 0, 0xFF, GPIO_DIRECTION_OUTPUT)
#define SET_PIN(port, pin)              GPIO_SetValue((port), (1 << (pin)))
#define CLR_PIN(port, pin)              GPIO_ClearValue((port), (1 << (pin)))
#define WRITE_PORT(port, value)         FIO_ByteSetValue((port), 0, (uint8_t)(value)); \
                                        FIO_ByteClearValue((port), 0, (uint8_t)(~value))
#define READ_PORT(port)                 FIO_ByteReadValue((port), (0))

// display ports and pins definitions
#define GLCD_DATABUS_PORT       1
#define GLCD_DI_PORT            1
#define GLCD_DI_PIN             17
#define GLCD_EN_PORT            1
#define GLCD_EN_PIN             16
#define GLCD_RW_PORT            1
#define GLCD_RW_PIN             20
#define GLCD_CS1_PORT           1
#define GLCD_CS1_PIN            18
#define GLCD_CS2_PORT           1
#define GLCD_CS2_PIN            19
#define GLCD_RST_PORT           1
#define GLCD_RST_PIN            15

// display backlight turn on definition
#define GLCD_BACKLIGHT_TURN_ON_WITH_ONE

// display backlight ports and pins
#define GLCD0_BACKLIGHT_PORT    2
#define GLCD0_BACKLIGHT_PIN     7
#define GLCD1_BACKLIGHT_PORT    2
#define GLCD1_BACKLIGHT_PIN     4
#define GLCD2_BACKLIGHT_PORT    2
#define GLCD2_BACKLIGHT_PIN     6
#define GLCD3_BACKLIGHT_PORT    2
#define GLCD3_BACKLIGHT_PIN     5

// switcher ports and pins definitions
#define SWITCHER_DIR_PORT       1
#define SWITCHER_DIR_PIN        21
#define SWITCHER_CH0_PORT       1
#define SWITCHER_CH0_PIN        11
#define SWITCHER_CH1_PORT       1
#define SWITCHER_CH1_PIN        8
#define SWITCHER_CH2_PORT       1
#define SWITCHER_CH2_PIN        10
#define SWITCHER_CH3_PORT       1
#define SWITCHER_CH3_PIN        9


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/


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

void glcd_init(void);
void glcd_backlight(uint8_t display, uint8_t state);
void glcd_clear(uint8_t display, uint8_t color);
void glcd_set_pixel(uint8_t display, uint8_t x, uint8_t y, uint8_t color);
void glcd_hline(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t color);
void glcd_vline(uint8_t display, uint8_t x, uint8_t y, uint8_t height, uint8_t color);
void glcd_line(uint8_t display, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void glcd_rect(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void glcd_rect_fill(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void glcd_rect_invert(uint8_t display, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void glcd_draw_image(uint8_t display, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color);
void glcd_text(uint8_t display, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color);
void glcd_update(void);


/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/

#ifndef DELAY_ns
#error "DELAY_ns macro must be defined"
#endif

#ifndef DELAY_us
#error "DELAY_us macro must be defined"
#endif

#if ! defined GLCD_DATABUS_PORT
#error "GLCD_DATABUS_PORT must be defined"
#endif
#if (! defined GLCD_DI_PORT) || (! defined GLCD_DI_PIN)
#error "GLCD_DI_PORT and GLCD_DI_PIN must be defined"
#endif
#if (! defined GLCD_EN_PORT) || (! defined GLCD_EN_PIN)
#error "GLCD_EN_PORT and GLCD_EN_PIN must be defined"
#endif
#if (! defined GLCD_RW_PORT) || (! defined GLCD_RW_PIN)
#error "GLCD_RW_PORT and GLCD_RW_PIN must be defined"
#endif
#if (! defined GLCD_CS1_PORT) || (! defined GLCD_CS1_PIN)
#error "GLCD_CS1_PORT and GLCD_CS1_PIN must be defined"
#endif
#if (! defined GLCD_CS2_PORT) || (! defined GLCD_CS2_PIN)
#error "GLCD_CS2_PORT and GLCD_CS2_PIN must be defined"
#endif
#if (! defined GLCD_RST_PORT) || (! defined GLCD_RST_PIN)
#error "GLCD_RST_PORT and GLCD_RST_PIN must be defined"
#endif


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
