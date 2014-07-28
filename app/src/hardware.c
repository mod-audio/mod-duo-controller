
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "hardware.h"
#include "config.h"
#include "utils.h"
#include "serial.h"
#include "led.h"
#include "actuator.h"
#include "tpa6130.h"
#include "ntc.h"

#include "device.h"


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
#ifdef LED0_PINS
    LED0_PINS,
#endif
#ifdef LED1_PINS
    LED1_PINS,
#endif    
#ifdef LED2_PINS    
    LED2_PINS,
#endif    
#ifdef LED3_PINS    
    LED3_PINS
#endif    
};

static const uint8_t *ENCODER_PINS[] = {
#ifdef ENCODER0_PINS
    (const uint8_t []) ENCODER0_PINS,
#endif    
#ifdef ENCODER1_PINS
    (const uint8_t []) ENCODER1_PINS,
#endif    
#ifdef ENCODER2_PINS
    (const uint8_t []) ENCODER2_PINS,
#endif    
#ifdef ENCODER3_PINS
    (const uint8_t []) ENCODER3_PINS
#endif    
};

static const uint8_t *FOOTSWITCH_PINS[] = {
#ifdef FOOTSWITCH0_PINS
    (const uint8_t []) FOOTSWITCH0_PINS,
#endif    
#ifdef FOOTSWITCH1_PINS
    (const uint8_t []) FOOTSWITCH1_PINS,
#endif    
#ifdef FOOTSWITCH2_PINS
    (const uint8_t []) FOOTSWITCH2_PINS,
#endif    
#ifdef FOOTSWITCH3_PINS
    (const uint8_t []) FOOTSWITCH3_PINS
#endif    
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

#ifdef ARM_RESET
#define UNBLOCK_ARM_RESET()     GPIO_SetDir(ARM_RESET_PORT, (1 << ARM_RESET_PIN), GPIO_DIRECTION_INPUT)
#define BLOCK_ARM_RESET()       GPIO_SetDir(ARM_RESET_PORT, (1 << ARM_RESET_PIN), GPIO_DIRECTION_OUTPUT); \
                                GPIO_ClearValue(ARM_RESET_PORT, (1 << ARM_RESET_PIN))
#endif

#define CPU_IS_ON()             (1 - ((FIO_ReadValue(CPU_STATUS_PORT) >> CPU_STATUS_PIN) & 1))
#define CPU_PULSE_BUTTON()      GPIO_ClearValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));    \
                                delay_ms(200);                                              \
                                GPIO_SetValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static serial_t g_serial[SERIAL_COUNT];
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

#ifdef COOLER
static void cooler_duty_cycle(uint8_t duty_cycle)
{
    if (duty_cycle == 0)
    {
        GPIO_ClearValue(COOLER_PORT, (1 << COOLER_PIN));
        g_cooler.state = 0;
    }
    else
    {
        if (duty_cycle >= COOLER_MAX_DC) duty_cycle = 0;
        GPIO_SetValue(COOLER_PORT, (1 << COOLER_PIN));
        g_cooler.state = 1;
    }

    g_cooler.duty_cycle = duty_cycle;
    g_cooler.counter = duty_cycle;
}

static void cooler_pwm(void)
{
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
#else
static void cooler_pwm(void) {}
#endif


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
    #ifdef ARM_RESET
    BLOCK_ARM_RESET();
    #endif

    // CPU power pins configuration
    #ifdef CPU_CONTROL
    GPIO_SetDir(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN), GPIO_DIRECTION_OUTPUT);
    GPIO_SetDir(CPU_STATUS_PORT, (1 << CPU_STATUS_PIN), GPIO_DIRECTION_INPUT);
    GPIO_SetValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));
    #endif

    // configures the cooler
    #ifdef COOLER
    GPIO_SetDir(COOLER_PORT, (1 << COOLER_PIN), GPIO_DIRECTION_OUTPUT);
    cooler_duty_cycle(COOLER_MAX_DC);
    #endif

    // true bypass
    #ifdef TRUE_BYPASS
    GPIO_SetDir(TRUE_BYPASS_PORT, (1 << TRUE_BYPASS_PIN), GPIO_DIRECTION_OUTPUT);
    hardware_set_true_bypass(BYPASS);
    #endif

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

    // ADC initialization
    ADC_Init(LPC_ADC, ADC_CLOCK);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOUS);

    // Headphone initialization (TPA and ADC)
    #ifdef HEADPHONE
    tpa6130_init();
    PINSEL_SetPinFunc(HEADPHONE_ADC_PORT, HEADPHONE_ADC_PIN, HEADPHONE_ADC_PIN_CONF);
    ADC_IntConfig(LPC_ADC, HEADPHONE_ADC_CHANNEL, DISABLE);
    ADC_ChannelCmd(LPC_ADC, HEADPHONE_ADC_CHANNEL, ENABLE);
    #endif

    // NTC initialization
    ntc_init();

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

    ////////////////////////////////////////////////////////////////
    // Serial initialization

    #ifdef SERIAL0
    g_serial[0].uart_id = 0;
    g_serial[0].baud_rate = SERIAL0_BAUD_RATE;
    g_serial[0].priority = SERIAL0_PRIORITY;
    g_serial[0].rx_port = SERIAL0_RX_PORT;
    g_serial[0].rx_pin = SERIAL0_RX_PIN;
    g_serial[0].rx_function = SERIAL0_RX_FUNC;
    g_serial[0].rx_buffer_size = SERIAL0_RX_BUFF_SIZE;
    g_serial[0].tx_port = SERIAL0_TX_PORT;
    g_serial[0].tx_pin = SERIAL0_TX_PIN;
    g_serial[0].tx_function = SERIAL0_TX_FUNC;
    g_serial[0].tx_buffer_size = SERIAL0_TX_BUFF_SIZE;
    g_serial[0].has_oe = SERIAL0_HAS_OE;
    #if SERIAL0_HAS_OE
    g_serial[0].oe_port = SERIAL0_OE_PORT;
    g_serial[0].oe_pin = SERIAL0_OE_PIN;
    #endif
    serial_init(&g_serial[0]);
    #endif

    #ifdef SERIAL1
    g_serial[1].uart_id = 1;
    g_serial[1].baud_rate = SERIAL1_BAUD_RATE;
    g_serial[1].priority = SERIAL1_PRIORITY;
    g_serial[1].rx_port = SERIAL1_RX_PORT;
    g_serial[1].rx_pin = SERIAL1_RX_PIN;
    g_serial[1].rx_function = SERIAL1_RX_FUNC;
    g_serial[1].rx_buffer_size = SERIAL1_RX_BUFF_SIZE;
    g_serial[1].tx_port = SERIAL1_TX_PORT;
    g_serial[1].tx_pin = SERIAL1_TX_PIN;
    g_serial[1].tx_function = SERIAL1_TX_FUNC;
    g_serial[1].tx_buffer_size = SERIAL1_TX_BUFF_SIZE;
    g_serial[1].has_oe = SERIAL1_HAS_OE;
    #if SERIAL1_HAS_OE
    g_serial[1].oe_port = SERIAL1_OE_PORT;
    g_serial[1].oe_pin = SERIAL1_OE_PIN;
    #endif
    serial_init(&g_serial[1]);
    #endif

    #ifdef SERIAL2
    g_serial[2].uart_id = 2;
    g_serial[2].baud_rate = SERIAL2_BAUD_RATE;
    g_serial[2].priority = SERIAL2_PRIORITY;
    g_serial[2].rx_port = SERIAL2_RX_PORT;
    g_serial[2].rx_pin = SERIAL2_RX_PIN;
    g_serial[2].rx_function = SERIAL2_RX_FUNC;
    g_serial[2].rx_buffer_size = SERIAL2_RX_BUFF_SIZE;
    g_serial[2].tx_port = SERIAL2_TX_PORT;
    g_serial[2].tx_pin = SERIAL2_TX_PIN;
    g_serial[2].tx_function = SERIAL2_TX_FUNC;
    g_serial[2].tx_buffer_size = SERIAL2_TX_BUFF_SIZE;
    g_serial[2].has_oe = SERIAL2_HAS_OE;
    #if SERIAL2_HAS_OE
    g_serial[2].oe_port = SERIAL2_OE_PORT;
    g_serial[2].oe_pin = SERIAL2_OE_PIN;
    #endif
    serial_init(&g_serial[2]);
    #endif

    #ifdef SERIAL3
    g_serial[3].uart_id = 3;
    g_serial[3].baud_rate = SERIAL3_BAUD_RATE;
    g_serial[3].priority = SERIAL3_PRIORITY;
    g_serial[3].rx_port = SERIAL3_RX_PORT;
    g_serial[3].rx_pin = SERIAL3_RX_PIN;
    g_serial[3].rx_function = SERIAL3_RX_FUNC;
    g_serial[3].rx_buffer_size = SERIAL3_RX_BUFF_SIZE;
    g_serial[3].tx_port = SERIAL3_TX_PORT;
    g_serial[3].tx_pin = SERIAL3_TX_PIN;
    g_serial[3].tx_function = SERIAL3_TX_FUNC;
    g_serial[3].tx_buffer_size = SERIAL3_TX_BUFF_SIZE;
    g_serial[3].has_oe = SERIAL3_HAS_OE;
    #if SERIAL3_HAS_OE
    g_serial[3].oe_port = SERIAL3_OE_PORT;
    g_serial[3].oe_pin = SERIAL3_OE_PIN;
    #endif
    serial_init(&g_serial[3]);
    #endif
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
#ifdef TRUE_BYPASS
    if (value == BYPASS)
        GPIO_ClearValue(TRUE_BYPASS_PORT, (1 << TRUE_BYPASS_PIN));
    else
        GPIO_SetValue(TRUE_BYPASS_PORT, (1 << TRUE_BYPASS_PIN));
#endif
    g_true_bypass = value;
}

uint8_t hardware_get_true_bypass(void)
{
    return g_true_bypass;
}

void hardware_headphone(void)
{
#ifdef HEADPHONE
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
#endif
}

void hardware_reset(uint8_t unblock)
{
#ifdef ARM_RESET
    if (unblock) UNBLOCK_ARM_RESET();
    else BLOCK_ARM_RESET();
#else
    (void)(unblock);
#endif
}

void hardware_cpu_power(uint8_t power)
{
#ifdef CPU_CONTROL
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
#else
    (void) (power);
#endif
}

uint8_t hardware_cpu_status(void)
{
#ifdef CPU_CONTROL
    return CPU_IS_ON();
#else
    return 0;
#endif
}

float hardware_temperature(void)
{
    float temp = ntc_read();

#ifdef COOLER
    static int32_t startup_time;

    // gets the timestamp to startup time
    if (startup_time == 0)
    {
        startup_time = g_counter;
        return temp;
    }

    // checks if the startup time is already reached
    if (((g_counter - startup_time) < COOLER_STARTUP_TIME) && startup_time > 0) return temp;

    // this is to mark that startup time already happened
    startup_time = -1;

    // calculates the duty cycle to current temperature
    float a, b, duty_cycle;

    a = (float) (COOLER_MAX_DC - COOLER_MIN_DC) / (float) (TEMPERATURE_MAX - TEMPERATURE_MIN);
    b = COOLER_MAX_DC - a*TEMPERATURE_MAX;

    duty_cycle = a*temp + b;

    if (duty_cycle > COOLER_MAX_DC) duty_cycle = COOLER_MAX_DC;
    if (duty_cycle < COOLER_MIN_DC) duty_cycle = COOLER_MIN_DC;

    cooler_duty_cycle(duty_cycle);
#endif

    return temp;
}

void TIMER0_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET)
    {
        // LEDs PWM
        leds_clock();

        // cooler
        cooler_pwm();
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
