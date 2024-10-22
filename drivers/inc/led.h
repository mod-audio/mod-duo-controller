
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef LED_H
#define LED_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "config.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

#define BLACK       ((const color_t){0x00, 0x00, 0x00})
#define WHITE       ((const color_t){0x64, 0xA0, 0xA0})
#define RED         ((const color_t){0x80, 0x00, 0x00})
#define GREEN       ((const color_t){0x00, 0xA0, 0x00})
#define BLUE        ((const color_t){0x00, 0x00, 0xA0})
#define YELLOW      ((const color_t){0x64, 0xA0, 0x00})
#define MAGENTA     ((const color_t){0x64, 0x00, 0xA0})
#define CYAN        ((const color_t){0x00, 0xA0, 0xA0})


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

// defines the max number of led_t variables (objects) that will be created
#define MAX_LEDS                LEDS_COUNT // LEDS_COUNT is defined in config.h

// defines the leds pwm clock in Hertz
#define LEDS_PWM_CLOCK_Hz       200000

// defines if LED_TURN_ON_WITH_ZERO or LED_TURN_ON_WITH_ONE
#define LED_TURN_ON_WITH_ZERO

// I/O macros configuration
// These configurations have been moved to config.h
#if 0
#define CONFIG_PIN_OUTPUT(port, pin)
#define SET_PIN(port, pin)
#define CLR_PIN(port, pin)
#endif


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct COLOR_T {
    uint8_t r, g, b;
} color_t;

typedef struct LED_PINS_T {
    uint8_t portR, pinR;
    uint8_t portG, pinG;
    uint8_t portB, pinB;
} led_pins_t;

typedef struct LED_T {
    uint8_t control;
    led_pins_t pins;
    color_t color;
    uint16_t time_on, time_off, counter;
} led_t;


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/

void led_init(led_t *led, const led_pins_t pins);
void led_set_color(led_t *led, const color_t color);
void led_blink(led_t *led, uint16_t time_on_ms, uint16_t time_off_ms);
void leds_clock(void);


/*
************************************************************************************************************************
*           MACRO'S
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/


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
