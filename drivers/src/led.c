
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "led.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define __BLINK    0x01
#define __STATE    0x02

#define PWM_PERIOD      (((uint32_t)1E6) / (uint32_t)LEDS_PWM_CLOCK_Hz)
//dirty fix to get taptempo LED in sync, determined by experimentation
//was #define COUNT_PRESCALE ((uint32_t)1000 / PWM_PERIOD)
#define COUNT_PRESCALE  (((uint32_t)1000 / (2*PWM_PERIOD)) - 2)


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

#if defined LED_TURN_ON_WITH_ZERO
#define R_ON(led)       CLR_PIN(led->pins.portR, led->pins.pinR)
#define G_ON(led)       CLR_PIN(led->pins.portG, led->pins.pinG)
#define B_ON(led)       CLR_PIN(led->pins.portB, led->pins.pinB)

#define R_OFF(led)      SET_PIN(led->pins.portR, led->pins.pinR)
#define G_OFF(led)      SET_PIN(led->pins.portG, led->pins.pinG)
#define B_OFF(led)      SET_PIN(led->pins.portB, led->pins.pinB)

#elif defined LED_TURN_ON_WITH_ONE
#define R_ON(led)       SET_PIN(led->pins.portR, led->pins.pinR)
#define G_ON(led)       SET_PIN(led->pins.portG, led->pins.pinG)
#define B_ON(led)       SET_PIN(led->pins.portB, led->pins.pinB)

#define R_OFF(led)      CLR_PIN(led->pins.portR, led->pins.pinR)
#define G_OFF(led)      CLR_PIN(led->pins.portG, led->pins.pinG)
#define B_OFF(led)      CLR_PIN(led->pins.portB, led->pins.pinB)
#endif

#define BLINK_ENABLE(led)       (led->control |= (__BLINK))
#define BLINK_DISABLE(led)      (led->control &= (~__BLINK))
#define BLINK_IS_ENABLED(led)   (led->control & __BLINK)
#define BLINK_IS_DISABLED(led)  (!BLINK_IS_ENABLED(led))

#define STATE_SET_ON(led)       (led->control |= (__STATE))
#define STATE_SET_OFF(led)      (led->control &= (~__STATE))
#define STATE_IS_ON(led)        (led->control & __STATE)
#define STATE_IS_OFF(led)       (!STATE_IS_ON(led))

#define BLINK_CHECK(led)        (BLINK_IS_DISABLED(led) || (BLINK_IS_ENABLED(led) && STATE_IS_ON(led)))


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static uint8_t g_leds_count;
static led_t *g_leds[MAX_LEDS];
static color_t g_colors[MAX_LEDS];


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

void led_init(led_t *led, const led_pins_t pins)
{
    if (!led) return;

    // store the pins
    led->pins.portR = pins.portR;
    led->pins.portG = pins.portG;
    led->pins.portB = pins.portB;
    led->pins.pinR = pins.pinR;
    led->pins.pinG = pins.pinG;
    led->pins.pinB = pins.pinB;

    // pins direction configuration
    CONFIG_PIN_OUTPUT(pins.portR, pins.pinR);
    CONFIG_PIN_OUTPUT(pins.portG, pins.pinG);
    CONFIG_PIN_OUTPUT(pins.portB, pins.pinB);

    // store the configurations
    g_leds[g_leds_count] = led;
    g_colors[g_leds_count] = BLACK;
    g_leds_count++;

    // initialize the object configurations
    led->control = 0;
    led->time_on = 0;
    led->time_off = 0;
    led->counter = 0;

    // initial color
    R_OFF(led);
    G_OFF(led);
    B_OFF(led);
    led->color = BLACK;
}


void led_set_color(led_t *led, const color_t color)
{
    if (!led) return;

    led->color = color;
    leds_clock();
}


void led_blink(led_t *led, uint16_t time_on_ms, uint16_t time_off_ms)
{
    if (!led) return;

    BLINK_DISABLE(led);

    led->time_on = time_on_ms;
    led->time_off = time_off_ms;
    led->counter = time_on_ms;

    // locate the led pointer
    uint8_t i;
    for (i = 0; i < g_leds_count; i++)
    {
        if (g_leds[i] == led) break;
    }

    // if not found returns
    if (i >= g_leds_count) return;

    if (led->time_on > 0)
    {
        // synchronizes the colors
        g_colors[i] = BLACK;

        // set state on
        STATE_SET_ON(led);

        // enables the blinker
        if (led->time_off > 0) BLINK_ENABLE(led);

        return;
    }

    // turn off the leds
    led_set_color(led, BLACK);
    STATE_SET_OFF(led);
}


void leds_clock(void)
{
    uint8_t i;
    led_t *led;
    static uint32_t count;

    for (i = 0; i < g_leds_count; i++)
    {
        led = g_leds[i];
        if (!led) continue;

        // colors verification
        if (led->color.r > 0 && led->color.r >= g_colors[i].r && BLINK_CHECK(led)) R_ON(led);
        else R_OFF(led);

        if (led->color.g > 0 && led->color.g >= g_colors[i].g && BLINK_CHECK(led)) G_ON(led);
        else G_OFF(led);

        if (led->color.b > 0 && led->color.b >= g_colors[i].b && BLINK_CHECK(led)) B_ON(led);
        else B_OFF(led);

        // increment each color (overflow expected)
        g_colors[i].r++;
        g_colors[i].g++;
        g_colors[i].b++;

        // only go ahead if passed 1ms
        if (count < COUNT_PRESCALE) continue;

        // blink time verification
        if (BLINK_IS_ENABLED(led))
        {
            if (led->counter > 0)
            {
                led->counter--;
            }
            else
            {
                if (STATE_IS_ON(led))
                {
                    // load the counter with time off
                    led->counter = led->time_off;

                    // turn off the leds and set state
                    STATE_SET_OFF(led);
                    R_OFF(led);
                    G_OFF(led);
                    B_OFF(led);
                }
                else
                {
                    // load the counter with time on
                    led->counter = led->time_on;

                    // turn on the leds and set state
                    STATE_SET_ON(led);
                    led_set_color(led, led->color);

                    // synchronizes the colors
                    g_colors[i] = BLACK;
                }
            }
        }
    }

    if (count++ >= COUNT_PRESCALE) count = 0;
}
