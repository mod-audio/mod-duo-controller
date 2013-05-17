
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
#define COUNT_PRESCALE  ((uint32_t)1000 / PWM_PERIOD)


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
#define R_ON(led)       CLR_PIN(led->portR, led->pinR)
#define G_ON(led)       CLR_PIN(led->portG, led->pinG)
#define B_ON(led)       CLR_PIN(led->portB, led->pinB)

#define R_OFF(led)      SET_PIN(led->portR, led->pinR)
#define G_OFF(led)      SET_PIN(led->portG, led->pinG)
#define B_OFF(led)      SET_PIN(led->portB, led->pinB)

#elif defined LED_TURN_ON_WITH_ONE
#define R_ON(led)       SET_PIN(led->portR, led->pinR)
#define G_ON(led)       SET_PIN(led->portG, led->pinG)
#define B_ON(led)       SET_PIN(led->portB, led->pinB)

#define R_OFF(led)      CLR_PIN(led->portR, led->pinR)
#define G_OFF(led)      CLR_PIN(led->portG, led->pinG)
#define B_OFF(led)      CLR_PIN(led->portB, led->pinB)
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
static uint16_t g_counter_on[MAX_LEDS], g_counter_off[MAX_LEDS];


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

void led_init(led_t *led)
{
    // pins direction configuration
    CONFIG_PIN_OUTPUT(led->portR, led->pinR);
    CONFIG_PIN_OUTPUT(led->portG, led->pinG);
    CONFIG_PIN_OUTPUT(led->portB, led->pinB);

    // store the led pointer
    g_leds[g_leds_count] = led;

    // initialize the configurations
    led->control = 0;
    led->time_on = 0;
    led->time_off = 0;
    g_colors[g_leds_count] = BLACK;
    g_counter_on[g_leds_count] = 0;
    g_counter_off[g_leds_count] = 0;
    g_leds_count++;

    // initial color
    R_OFF(led);
    G_OFF(led);
    B_OFF(led);
    led->color = BLACK;
}


void led_set_color(led_t *led, const color_t color)
{
    led->color = color;
}


void led_blink(led_t *led, uint16_t time_on_ms, uint16_t time_off_ms)
{
    led->time_on = time_on_ms;
    led->time_off = time_off_ms;

    BLINK_DISABLE(led);

    // locate the led pointer
    uint8_t i;
    for (i = 0; i < g_leds_count; i++)
    {
        if (g_leds[i] == led) break;
    }

    // reload the counters
    if (i < g_leds_count)
    {
        g_counter_on[i] = time_on_ms;
        g_counter_off[i] = time_off_ms;
    }

    // checks if must be enable the blinker
    if (led->time_on > 0 && led->time_off > 0) BLINK_ENABLE(led);
    else if (led->time_on > 0 && led->time_off == 0) led_set_color(led, led->color);
    else led_set_color(led, BLACK);
}


void leds_clock(void)
{
    uint8_t i;
    led_t *led;
    static uint32_t count;

    for (i = 0; i < g_leds_count; i++)
    {
        led = g_leds[i];

        // colors verification
        if (led->color.r > 0 && led->color.r >= g_colors[i].r && BLINK_CHECK(led)) R_ON(led);
        else R_OFF(led);

        if (led->color.g > 0 && led->color.g >= g_colors[i].g && BLINK_CHECK(led)) G_ON(led);
        else G_OFF(led);

        if (led->color.b > 0 && led->color.b >= g_colors[i].b && BLINK_CHECK(led)) B_ON(led);
        else B_OFF(led);

        g_colors[i].r++;
        g_colors[i].g++;
        g_colors[i].b++;

        // only go ahead if passed 1ms
        if (count < COUNT_PRESCALE) continue;

        // time verification
        if (BLINK_IS_ENABLED(led))
        {
            // if there is at least one led turned on...
            if (STATE_IS_ON(led))
            {
                if (g_counter_on[i] > 0)
                {
                    g_counter_on[i]--;
                }
                else
                {
                    // reload the counter
                    g_counter_on[i] = led->time_on;

                    // turn off the leds and set state
                    STATE_SET_OFF(led);
                    R_OFF(led);
                    G_OFF(led);
                    B_OFF(led);
                }
            }
            // if all leds is turned off...
            else
            {
                if (g_counter_off[i] > 0)
                {
                    g_counter_off[i]--;
                }
                else
                {
                    // reload the counter
                    g_counter_off[i] = led->time_off;

                    // turn on the leds and set state
                    STATE_SET_ON(led);
                    led_set_color(led, led->color);
                }
            }
        }
    }

    if (count++ >= COUNT_PRESCALE) count = 0;
}
