
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "hardware.h"
#include "config.h"
#include "utils.h"
#include "led.h"
#include "actuator.h"
#include "tpa6130.h"

#include "LPC177x_8x.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_timer.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_adc.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

// check in hardware_setup() what is the function of each timer
#define TIMER0_PRIORITY     3
#define TIMER1_PRIORITY     2

// defines the headphone update period
#define HEADPHONE_UPDATE_PERIOD     (1000 / HEADPHONE_UPDATE_FRENQUENCY)
// defines how much values will be used to calculates the mean
#define HEADPHONE_MEAN_DEPTH        5


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static const led_pins_t LEDS_PINS[] = {
    LED0_PINS,
    LED1_PINS,
    LED2_PINS,
    LED3_PINS
};

static const uint8_t *ENCODER_PINS[] = {
    (const uint8_t []) ENCODER0_PINS,
    (const uint8_t []) ENCODER1_PINS,
    (const uint8_t []) ENCODER2_PINS,
    (const uint8_t []) ENCODER3_PINS
};

static const uint8_t *FOOTSWITCH_PINS[] = {
    (const uint8_t []) FOOTSWITCH0_PINS,
    (const uint8_t []) FOOTSWITCH1_PINS,
    (const uint8_t []) FOOTSWITCH2_PINS,
    (const uint8_t []) FOOTSWITCH3_PINS
};


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

#define ABS(x)                  ((x) > 0 ? (x) : -(x))
#define UNBLOCK_ARM_RESET()     GPIO_SetDir(ARM_RESET_PORT, (1 << ARM_RESET_PIN), GPIO_DIRECTION_INPUT)
#define BLOCK_ARM_RESET()       GPIO_SetDir(ARM_RESET_PORT, (1 << ARM_RESET_PIN), GPIO_DIRECTION_OUTPUT); \
                                GPIO_ClearValue(ARM_RESET_PORT, (1 << ARM_RESET_PIN))

#define CPU_IS_ON()             (1 - ((FIO_ReadValue(CPU_STATUS_PORT) >> CPU_STATUS_PIN) & 1))
#define CPU_PULSE_BUTTON()      GPIO_ClearValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));    \
                                delay_ms(200);                                              \
                                GPIO_SetValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static led_t g_leds[LEDS_COUNT];
static encoder_t g_encoders[ENCODERS_COUNT];
static button_t g_footswitches[FOOTSWITCHES_COUNT];
static uint32_t g_counter;
static uint8_t g_true_bypass;


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
    BLOCK_ARM_RESET();

    // CPU power pins configuration
    GPIO_SetDir(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN), GPIO_DIRECTION_OUTPUT);
    GPIO_SetDir(CPU_STATUS_PORT, (1 << CPU_STATUS_PIN), GPIO_DIRECTION_INPUT);
    GPIO_SetValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));

    // configures the cooler
    GPIO_SetDir(COOLER_PORT, (1 << COOLER_PIN), GPIO_DIRECTION_OUTPUT);
    hardware_cooler(100);

    // true bypass
    GPIO_SetDir(TRUE_BYPASS_PORT, (1 << TRUE_BYPASS_PIN), GPIO_DIRECTION_OUTPUT);
    hardware_set_true_bypass(BYPASS);

    // RS485 direction
    GPIO_SetDir(RS485_DIR_PORT, (1 << RS485_DIR_PIN), GPIO_DIRECTION_OUTPUT);
    hardware_485_direction(RECEPTION);

    // SLOTs initialization
    uint8_t i;
    for (i = 0; i < SLOTS_COUNT; i++)
    {
        // LEDs initialization
        led_init(&g_leds[i], LEDS_PINS[i]);

        // actuators creation
        actuator_create(ROTARY_ENCODER, i, hardware_actuators(ENCODER0 + i));
        actuator_create(BUTTON, i, hardware_actuators(FOOTSWITCH0 + i));

        // actuators pins configuration
        actuator_set_pins(hardware_actuators(ENCODER0 + i), ENCODER_PINS[i]);
        actuator_set_pins(hardware_actuators(FOOTSWITCH0 + i), FOOTSWITCH_PINS[i]);

        // actuators properties
        actuator_set_prop(hardware_actuators(ENCODER0 + i), ENCODER_STEPS, 3);
        actuator_set_prop(hardware_actuators(ENCODER0 + i), BUTTON_HOLD_TIME, TOOL_MODE_TIME);
    }

    // Headphone initialization (TPA and ADC)
    tpa6130_init();
    // ADC configuration
    PINSEL_ConfigPin(HEADPHONE_ADC_PORT, HEADPHONE_ADC_PIN, HEADPHONE_ADC_PIN_CONF);
    ADC_Init(LPC_ADC, HEADPHONE_ADC_CLOCK);
    ADC_IntConfig(LPC_ADC, HEADPHONE_ADC_CHANNEL, DISABLE);
    ADC_ChannelCmd(LPC_ADC, HEADPHONE_ADC_CHANNEL, ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);

    ////////////////////////////////////////////////////////////////
    // Timer 0 configuration
    // this timer is used to LEDs PWM and cooler PWM

    // timer structs declaration
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct ;
    // initialize timer 0, prescale count time of 10us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue    = 10;
    // use channel 0, MR0
    TIM_MatchConfigStruct.MatchChannel = 0;
    // enable interrupt when MR0 matches the value in TC register
    TIM_MatchConfigStruct.IntOnMatch   = TRUE;
    // enable reset on MR0: TIMER will reset if MR0 matches it
    TIM_MatchConfigStruct.ResetOnMatch = TRUE;
    // stop on MR0 if MR0 matches it
    TIM_MatchConfigStruct.StopOnMatch  = FALSE;
    // set Match value, count value of 5 (5 * 10us = 50us --> 20 kHz)
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

    ////////////////////////////////////////////////////////////////
    // Timer 1 configuration
    // this timer is for actuators clock and timestamp

    // initialize timer 1, prescale count time of 500us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue    = 500;
    // use channel 1, MR1
    TIM_MatchConfigStruct.MatchChannel = 1;
    // enable interrupt when MR1 matches the value in TC register
    TIM_MatchConfigStruct.IntOnMatch   = TRUE;
    // enable reset on MR1: TIMER will reset if MR1 matches it
    TIM_MatchConfigStruct.ResetOnMatch = TRUE;
    // stop on MR1 if MR1 matches it
    TIM_MatchConfigStruct.StopOnMatch  = FALSE;
    // set Match value, count value of 1
    TIM_MatchConfigStruct.MatchValue   = 1;
    // set configuration for Tim_config and Tim_MatchConfig
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &TIM_ConfigStruct);
    TIM_ConfigMatch(LPC_TIM1, &TIM_MatchConfigStruct);
    // set priority
    NVIC_SetPriority(TIMER1_IRQn, (TIMER1_PRIORITY << 3));
    // enable interrupt for timer 1
    NVIC_EnableIRQ(TIMER1_IRQn);
    // to start timer
    TIM_Cmd(LPC_TIM1, ENABLE);
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

led_t *hardware_leds(uint8_t led_id)
{
    if (led_id > LEDS_COUNT) return NULL;
    return &g_leds[led_id];
}

void *hardware_actuators(uint8_t actuator_id)
{
    if ((int8_t)actuator_id >= ENCODER0 && actuator_id < (ENCODER0 + FOOTSWITCHES_COUNT))
    {
        return (&g_encoders[actuator_id - ENCODER0]);
    }

    if ((int8_t)actuator_id >= FOOTSWITCH0 && actuator_id < (FOOTSWITCH0 + FOOTSWITCHES_COUNT))
    {
        return (&g_footswitches[actuator_id - FOOTSWITCH0]);
    }

    return NULL;
}

uint32_t hardware_timestamp(void)
{
    return g_counter;
}

void hardware_set_true_bypass(uint8_t value)
{
    if (value)
        GPIO_SetValue(TRUE_BYPASS_PORT, (1 << TRUE_BYPASS_PIN));
    else
        GPIO_ClearValue(TRUE_BYPASS_PORT, (1 << TRUE_BYPASS_PIN));

    g_true_bypass = value;
}

uint8_t hardware_get_true_bypass(void)
{
    return g_true_bypass;
}

void hardware_headphone(void)
{
    static uint32_t adc_values[HEADPHONE_MEAN_DEPTH], adc_idx;

    // stores the last adc samples
    if (ADC_ChannelGetStatus(LPC_ADC, HEADPHONE_ADC_CHANNEL, ADC_DATA_DONE))
    {
        adc_values[adc_idx] = ADC_ChannelGetData(LPC_ADC, HEADPHONE_ADC_CHANNEL);
        if (++adc_idx == HEADPHONE_MEAN_DEPTH) adc_idx = 0;
    }

    static uint32_t last_volume, last_counter;

    // checks if need update the headphone volume
    if ((g_counter - last_counter) >= HEADPHONE_UPDATE_PERIOD)
    {
        last_counter = g_counter;

        uint32_t i, volume = 0;
        for (i = 0; i < HEADPHONE_MEAN_DEPTH; i++)
        {
            volume += adc_values[i];
        }

        volume /= HEADPHONE_MEAN_DEPTH;
        volume = 63 - (volume >> 6);

        // checks the minimal volume variation
        if (ABS(volume - last_volume) >= HEADPHONE_MINIMAL_VARIATION)
        {
            tpa6130_set_volume(volume);
            last_volume = volume;
        }
    }
}

void hardware_reset(uint8_t unblock)
{
    if (unblock) UNBLOCK_ARM_RESET();
    else BLOCK_ARM_RESET();
}

void hardware_485_direction(uint8_t direction)
{
    if (direction == RECEPTION)
        GPIO_ClearValue(RS485_DIR_PORT, (1 << RS485_DIR_PIN));
    else
        GPIO_SetValue(RS485_DIR_PORT, (1 << RS485_DIR_PIN));

    delay_us(100);
}

void hardware_cpu_power(uint8_t power)
{
    switch (power)
    {
        case CPU_TURN_OFF:
            if (CPU_IS_ON()) CPU_PULSE_BUTTON();
            break;

        case CPU_TURN_ON:
            if (!CPU_IS_ON()) CPU_PULSE_BUTTON();
            break;

        case CPU_REBOOT:
            if (CPU_IS_ON())
            {
                CPU_PULSE_BUTTON();
                while (CPU_IS_ON());
                delay_ms(100);
                CPU_PULSE_BUTTON();
            }
            else CPU_PULSE_BUTTON();
            break;
    }
}

uint8_t hardware_cpu_status(void)
{
    return CPU_IS_ON();
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

void TIMER1_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR1_INT) == SET)
    {
        actuators_clock();
        g_counter++;
    }

    TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
}
