
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "hardware.h"
#include "config.h"
#include "utils.h"
#include "led.h"

#include "LPC177x_8x.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_timer.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/


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

struct COOLER_T {
    uint8_t duty_cycle;
    uint8_t counter, state;
} g_cooler;


/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/

#define CPU_POWER_ON()          GPIO_ClearValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));    \
                                delay_ms(200);                                              \
                                GPIO_SetValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN))

#define UNBLOCK_ARM_RESET()     GPIO_SetDir(ARM_RESET_PORT, (1 << ARM_RESET_PIN), GPIO_DIRECTION_INPUT)
#define BLOCK_ARM_RESET()       GPIO_SetDir(ARM_RESET_PORT, (1 << ARM_RESET_PIN), GPIO_DIRECTION_OUTPUT); \
                                GPIO_ClearValue(ARM_RESET_PORT, (1 << ARM_RESET_PIN))


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static led_t g_leds[4];


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

void hardware_setup(void)
{
    // set system tick for 1ms interrupt
    SystemCoreClockUpdate();

    // ARM reset
    // FIXME: after implement the unblock message, change this initial state to block
    UNBLOCK_ARM_RESET();

    // CPU power pins configuration
    GPIO_SetDir(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN), GPIO_DIRECTION_OUTPUT);
    GPIO_SetDir(CPU_STATUS_PORT, (1 << CPU_STATUS_PIN), GPIO_DIRECTION_OUTPUT);
    if (!hardware_cpu_status()) CPU_POWER_ON();

    // configures the cooler
    GPIO_SetDir(COOLER_PORT, (1 << COOLER_PIN), GPIO_DIRECTION_OUTPUT);
    hardware_cooler(100);

    // LEDs initialization
    led_init(&g_leds[0], (const led_pins_t)LED0_PINS);
    led_init(&g_leds[1], (const led_pins_t)LED1_PINS);
    led_init(&g_leds[2], (const led_pins_t)LED2_PINS);
    led_init(&g_leds[3], (const led_pins_t)LED3_PINS);

    ////////////////////////////////////////////////////////////////
    // Timer 0 configuration
    // this timer is used to LEDs PWM and cooler PWM

    // timer structs declaration
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct ;
	// initialize timer 0, prescale count time of 10us
	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 10;
	// use channel 0, MR0
	TIM_MatchConfigStruct.MatchChannel = 0;
	// enable interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct.IntOnMatch   = TRUE;
	// enable reset on MR0: TIMER will reset if MR0 matches it
	TIM_MatchConfigStruct.ResetOnMatch = TRUE;
	// stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct.StopOnMatch  = FALSE;
	// set Match value, count value of 10 (5 * 10us = 50us --> 20 kHz)
	TIM_MatchConfigStruct.MatchValue   = 5;
	// set configuration for Tim_config and Tim_MatchConfig
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
	TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
    // set priority
	NVIC_SetPriority(TIMER0_IRQn, (TIMER0_PRIORITY << 3));
	// enable interrupt for timer 0
	NVIC_EnableIRQ(TIMER0_IRQn);
	// to start timer
	TIM_Cmd(LPC_TIM0, ENABLE);
}

uint8_t hardware_cpu_status(void)
{
    uint32_t status = GPIO_ReadValue(CPU_STATUS_PORT);
    status = (status >> CPU_STATUS_PIN) & 1;
    return (1 - ((uint8_t) status));
}

void hardware_cooler(uint8_t duty_cycle)
{
    if (duty_cycle == 0)
    {
        GPIO_ClearValue(COOLER_PORT, (1 << COOLER_PIN));
        g_cooler.state = 0;
    }
    else
    {
        if (duty_cycle >= 100) duty_cycle = 0;
        GPIO_SetValue(COOLER_PORT, (1 << COOLER_PIN));
        g_cooler.state = 1;
    }

    g_cooler.duty_cycle = duty_cycle;
    g_cooler.counter = duty_cycle;
}

led_t *hardware_leds(uint8_t led)
{
    if (led > LEDS_COUNT) return NULL;
    return &g_leds[led];
}

void TIMER0_IRQHandler(void)
{
	if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET)
	{
        // LEDs PWM
		leds_clock();

        // cooler PWM
        if (g_cooler.duty_cycle)
        {
            g_cooler.counter--;
            if (g_cooler.counter == 0)
            {
                if (g_cooler.state)
                {
                    GPIO_ClearValue(COOLER_PORT, (1 << COOLER_PIN));
                    g_cooler.counter = 100 - g_cooler.duty_cycle;
                    g_cooler.state = 0;
                }
                else
                {
                    GPIO_SetValue(COOLER_PORT, (1 << COOLER_PIN));
                    g_cooler.counter = g_cooler.duty_cycle;
                    g_cooler.state = 1;
                }
            }
        }
	}

	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}
