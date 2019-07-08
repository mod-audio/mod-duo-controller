
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "hardware.h"
#include "config.h"
#include "utils.h"
#include "serial.h"
#include "actuator.h"
#include "task.h"
#include "device.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

// check in hardware_setup() what is the function of each timer
#define TIMER0_PRIORITY     3
#define TIMER1_PRIORITY     2

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


/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/

#define ABS(x)                  ((x) > 0 ? (x) : -(x))

#define CPU_IS_ON()             (1 - ((FIO_ReadValue(CPU_STATUS_PORT) >> CPU_STATUS_PIN) & 1))
#define CPU_PULSE_BUTTON()      GPIO_ClearValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));    \
                                delay_ms(200);                                              \
                                GPIO_SetValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static glcd_t g_glcd[GLCD_COUNT] = {GLCD0_CONFIG GLCD1_CONFIG GLCD2_CONFIG GLCD3_CONFIG};
static serial_t g_serial[SERIAL_COUNT];
static led_t g_leds[LEDS_COUNT];
static encoder_t g_encoders[ENCODERS_COUNT];
static button_t g_footswitches[FOOTSWITCHES_COUNT];
static uint32_t g_counter;
static int g_brightness;


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
    // set system core clock
    SystemCoreClockUpdate();

    // configure the peripherals power
    CLKPWR_ConfigPPWR(HW_CLK_PWR_CONTROL, ENABLE);

    // configure and set initial state of shutdown cpu button
    // note: CLR_PIN will make the coreboard reboot unless SET_PIN is called in less than 5s
    //       this is done in the beginning of cli_task()
    CONFIG_PIN_OUTPUT(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    CLR_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);

    // SLOTs initialization
    uint8_t i;
    for (i = 0; i < SLOTS_COUNT; i++)
    {
        // LEDs initialization
        led_init(&g_leds[i], LEDS_PINS[i]);

        // GLCD initialization
        glcd_init(&g_glcd[i]);

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

    // default glcd brightness
    g_brightness = MAX_BRIGHTNESS;

    ////////////////////////////////////////////////////////////////
    // Timer 0 configuration
    // this timer is used to LEDs PWM

    // timer structs declaration
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct ;
    // initialize timer 0, prescale count time of 10us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 10;
    // use channel 0, MR0
    TIM_MatchConfigStruct.MatchChannel = 0;
    // enable interrupt when MR0 matches the value in TC register
    TIM_MatchConfigStruct.IntOnMatch = TRUE;
    // enable reset on MR0: TIMER will reset if MR0 matches it
    TIM_MatchConfigStruct.ResetOnMatch = TRUE;
    // stop on MR0 if MR0 matches it
    TIM_MatchConfigStruct.StopOnMatch = FALSE;
    // set Match value, count value of 1 (1 * 10us = 10us --> 100 kHz)
    TIM_MatchConfigStruct.MatchValue = 1;
    // set configuration for Tim_config and Tim_MatchConfig
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
    // set priority
    NVIC_SetPriority(TIMER0_IRQn, TIMER0_PRIORITY);
    // enable interrupt for timer 0
    NVIC_EnableIRQ(TIMER0_IRQn);
    // to start timer
    TIM_Cmd(LPC_TIM0, ENABLE);

    ////////////////////////////////////////////////////////////////
    // Timer 1 configuration
    // this timer is for actuators clock and timestamp

    // initialize timer 1, prescale count time of 500us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 500;
    // use channel 1, MR1
    TIM_MatchConfigStruct.MatchChannel = 1;
    // enable interrupt when MR1 matches the value in TC register
    TIM_MatchConfigStruct.IntOnMatch = TRUE;
    // enable reset on MR1: TIMER will reset if MR1 matches it
    TIM_MatchConfigStruct.ResetOnMatch = TRUE;
    // stop on MR1 if MR1 matches it
    TIM_MatchConfigStruct.StopOnMatch = FALSE;
    // set Match value, count value of 1
    TIM_MatchConfigStruct.MatchValue = 1;
    // set configuration for Tim_config and Tim_MatchConfig
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &TIM_ConfigStruct);
    TIM_ConfigMatch(LPC_TIM1, &TIM_MatchConfigStruct);
    // set priority
    NVIC_SetPriority(TIMER1_IRQn, TIMER1_PRIORITY);
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

void hardware_eneble_serial_interupt(uint8_t serial_port)
{
    serial_enable_interupt(&g_serial[serial_port]);
}

glcd_t *hardware_glcds(uint8_t glcd_id)
{
    if (glcd_id >= GLCD_COUNT) return NULL;
    return &g_glcd[glcd_id];
}

void hardware_glcd_brightness(int level)
{
    g_brightness = level;
}

led_t *hardware_leds(uint8_t led_id)
{
    if (led_id >= LEDS_COUNT) return NULL;
    return &g_leds[led_id];
}

void *hardware_actuators(uint8_t actuator_id)
{
    if ((int8_t)actuator_id >= ENCODER0 && actuator_id < (ENCODER0 + ENCODERS_COUNT))
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

void hardware_coreboard_power(uint8_t state)
{
    // coreboard sometimes requires 1s pulse to initialize
    if (state == COREBOARD_INIT)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
        SET_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    }
    // coreboard requires 2s pulse to turn on
    else if (state == COREBOARD_TURN_ON)
    {
        CLR_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
        vTaskDelay(2000 / portTICK_RATE_MS);
        SET_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    }
    // coreboard requires 5s pulse to turn off
    else if (state == COREBOARD_TURN_OFF)
    {
        CLR_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
        vTaskDelay(5000 / portTICK_RATE_MS);
        SET_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    }
}

void TIMER0_IRQHandler(void)
{
    static int count = 1, state;

    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET)
    {
        // LEDs PWM
        leds_clock();

        if (g_brightness == 0)
        {
            count = 1;
            state = 1;
        }
        else if (g_brightness == MAX_BRIGHTNESS)
        {
            count = 1;
            state = 0;
        }

        if (--count == 0)
        {
            if (state)
            {
                count = MAX_BRIGHTNESS - g_brightness;
                glcd_backlight(&g_glcd[0], 0);
                glcd_backlight(&g_glcd[1], 0);
            }
            else
            {
                count = g_brightness;
                glcd_backlight(&g_glcd[0], 1);
                glcd_backlight(&g_glcd[1], 1);
            }

            state ^= 1;
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
