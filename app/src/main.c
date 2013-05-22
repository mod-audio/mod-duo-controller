
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdlib.h>
#include <string.h>

#include "LPC177x_8x.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_timer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "config.h"
#include "serial.h"
#include "protocol.h"
#include "led.h"
#include "glcd.h"
#include "fonts.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define PIN_LED1 20
#define PIN_LED2 24
#define PIN_LED3 28


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

const uint8_t LEDS_RED_PORT[] = {
    LED1_RED_PORT,
    LED2_RED_PORT,
    LED3_RED_PORT,
    LED4_RED_PORT,
};

const uint8_t LEDS_RED_PIN[] = {
    LED1_RED_PIN,
    LED2_RED_PIN,
    LED3_RED_PIN,
    LED4_RED_PIN,
};

const uint8_t LEDS_GREEN_PORT[] = {
    LED1_GREEN_PORT,
    LED2_GREEN_PORT,
    LED3_GREEN_PORT,
    LED4_GREEN_PORT,
};

const uint8_t LEDS_GREEN_PIN[] = {
    LED1_GREEN_PIN,
    LED2_GREEN_PIN,
    LED3_GREEN_PIN,
    LED4_GREEN_PIN,
};

const uint8_t LEDS_BLUE_PORT[] = {
    LED1_BLUE_PORT,
    LED2_BLUE_PORT,
    LED3_BLUE_PORT,
    LED4_BLUE_PORT,
};

const uint8_t LEDS_BLUE_PIN[] = {
    LED1_BLUE_PIN,
    LED2_BLUE_PIN,
    LED3_BLUE_PIN,
    LED4_BLUE_PIN,
};

static const uint8_t mod_mini[] = {
    32,
    32,
    0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xC0,0xE0,0x60,0x60,0x30,0x30,0x30,0x30,0x30,
	0x30,0x30,0x30,0x30,0x30,0x60,0x60,0xE0,0xC0,0xC0,0x80,0x00,0x00,0x00,0x00,0x00,
	0x00,0xE0,0xF8,0x3E,0x0F,0x03,0x01,0x00,0x00,0x00,0x80,0xBF,0xBF,0xB3,0xA0,0xA1,
	0xBF,0xBF,0xBF,0xBF,0xBF,0x80,0x00,0x00,0x00,0x01,0x03,0x0F,0x3E,0xF8,0xE0,0x00,
	0x00,0x03,0x0F,0x3E,0x78,0xE0,0xC0,0x80,0x80,0x1F,0x3F,0x61,0x40,0x40,0x5F,0x5E,
	0x40,0x40,0x40,0x40,0x61,0x3F,0x1F,0x80,0x80,0xC0,0xE0,0x78,0x3E,0x0F,0x03,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x03,0x03,0x03,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x03,0x03,0x03,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00
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

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static xQueueHandle g_serial_queue;

static led_t leds[4];


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static void hardware_setup(void);
static void serial_cb(serial_msg_t *msg);
static void procotol_parser_task(void *pvParameters);

static void say_cb(proto_t *proto);
static void led_cb(proto_t *proto);


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
*           MAIN FUNCTION
************************************************************************************************************************
*/

int main(void)
{
    // initialize hardware
    hardware_setup();

    g_serial_queue = xQueueCreate(5, sizeof(serial_msg_t *));

    // create tasks
    xTaskCreate(procotol_parser_task, (signed char *)"Protocol parser", 1000, NULL, 2, NULL);

    // Start the scheduler
    vTaskStartScheduler();

    // should never reach here!
    for(;;);
}


/*
************************************************************************************************************************
*           LOCAL FUNCTIONS
************************************************************************************************************************
*/

static void hardware_setup(void)
{
    volatile unsigned long delay;

    // set system tick for 1ms interrupt
    SystemCoreClockUpdate();

    // pins direction configuration
    GPIO_SetDir(1, (1 << 23), GPIO_DIRECTION_OUTPUT); // power on PC
    GPIO_SetDir(1, (1 << 24), GPIO_DIRECTION_INPUT); // reset ARM

    // turn on cooler
    GPIO_SetDir(1, (1 << 22), GPIO_DIRECTION_OUTPUT); // cooler
    GPIO_SetValue(1, (1 << 22));

    // pulse power on PC
    GPIO_ClearValue(1, (1 << 23));
    for (delay = 0; delay < 5000000; delay++);
    GPIO_SetValue(1, (1 << 23));
    for (delay = 0; delay < 5000000; delay++);

    // leds initialization
    uint8_t i;
    for (i = 0; i < LEDS_COUNT; i++)
    {
        leds[i].portR = LEDS_RED_PORT[i];
        leds[i].pinR = LEDS_RED_PIN[i];
        leds[i].portG = LEDS_GREEN_PORT[i];
        leds[i].pinG = LEDS_GREEN_PIN[i];
        leds[i].portB = LEDS_BLUE_PORT[i];
        leds[i].pinB = LEDS_BLUE_PIN[i];
        led_init(&leds[i]);
    }

    // just for testing
    led_blink(&leds[0], 1000, 1000);
    led_blink(&leds[1], 500, 1000);
    led_blink(&leds[2], 300, 200);
    led_blink(&leds[3], 200, 100);

    // serial initialization
    serial_init();
    serial_set_callback(SERIAL0, serial_cb);
    serial_set_callback(SERIAL1, serial_cb);

    // protocol definitions
    protocol_add_command(SAY_CMD, say_cb);
    protocol_add_command(LED_CMD, led_cb);

    /////////////// Timer 0 configuration (used to LEDs PWM)
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct ;

	// Initialize timer 0, prescale count time of 10us
	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 10;
	// use channel 0, MR0
	TIM_MatchConfigStruct.MatchChannel = 0;
	// Enable interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct.IntOnMatch   = TRUE;
	//Enable reset on MR0: TIMER will reset if MR0 matches it
	TIM_MatchConfigStruct.ResetOnMatch = TRUE;
	//Stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct.StopOnMatch  = FALSE;
	// Set Match value, count value of 10 (5 * 10us = 50us --> 20 kHz)
	TIM_MatchConfigStruct.MatchValue   = 5;

	// Set configuration for Tim_config and Tim_MatchConfig
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
	TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
    // Set priority
	NVIC_SetPriority(TIMER0_IRQn, (2 << 3));
	// Enable interrupt for timer 0
	NVIC_EnableIRQ(TIMER0_IRQn);
	// To start timer
	TIM_Cmd(LPC_TIM0, ENABLE);

    glcd_init();
    glcd_set_pixel(0, 63, 0, GLCD_BLACK);
    glcd_hline(0, 0, 32, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_hline(0, 0, 33, DISPLAY_WIDTH, GLCD_WHITE);
    glcd_hline(0, 0, 34, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    glcd_hline(0, 0, 35, DISPLAY_WIDTH, GLCD_WHITE_BLACK);
    glcd_hline(1, 0, 51, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_vline(0, 64, 0, DISPLAY_HEIGHT, GLCD_BLACK);
    glcd_vline(0, 65, 0, DISPLAY_HEIGHT, GLCD_WHITE);
    glcd_vline(0, 66, 0, DISPLAY_HEIGHT, GLCD_BLACK_WHITE);
    glcd_vline(0, 67, 0, DISPLAY_HEIGHT, GLCD_WHITE_BLACK);
    glcd_rect(0, 0, 0, 10, 10, GLCD_BLACK_WHITE);
    glcd_rect(1, 10, 0, 4, 4, GLCD_BLACK);
    glcd_line(1, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, GLCD_BLACK);
    glcd_line(1, 127, 0, 0, DISPLAY_HEIGHT, GLCD_BLACK_WHITE);
    glcd_rect_fill(2, 0, 0, 10, 32, GLCD_BLACK_WHITE);
    glcd_rect_fill(2, 10, 0, 10, 32, GLCD_WHITE_BLACK);
    glcd_rect_fill(2, 20, 0, 10, 32, GLCD_CHESS);
    glcd_rect_fill(2, 30, 0, 10, 32, GLCD_BLACK);
    glcd_rect_invert(2, 0, 12, DISPLAY_WIDTH, 3);
    glcd_rect_invert(2, 0, 18, DISPLAY_WIDTH, 5);
    glcd_rect_invert(1, 20, 15, 88, 34);
    glcd_draw_image(3, 0, 0, mod_mini, GLCD_BLACK);
    glcd_draw_image(3, 32, 0, mod_mini, GLCD_WHITE);
    glcd_set_pixel(3, 0, 40, GLCD_BLACK);
    glcd_set_pixel(3, 0, 45, GLCD_BLACK);
    glcd_set_pixel(3, 0, 50, GLCD_BLACK);
    glcd_set_pixel(3, 0, 55, GLCD_BLACK);
    glcd_text(2, 0, 40, "ABCpqyrs123M", NULL, GLCD_BLACK);
    glcd_text(2, 0, 50, "ABCpqyrs123M", NULL, GLCD_WHITE);
    glcd_text(3, 2, 50, "One Two Three My", NULL, GLCD_BLACK);
    glcd_text(3, 2, 40, "One Two Three My", alterebro15, GLCD_BLACK);
    glcd_update();
}

// this callback is called from a ISR
static void serial_cb(serial_msg_t *msg)
{
    serial_msg_t *msg_copy;

    // does a copy of message
    msg_copy = pvPortMalloc(sizeof(serial_msg_t));
    if (!msg_copy) while (1);
    memcpy(msg_copy, msg, sizeof(serial_msg_t));

    // allocate memory to store the message
    msg_copy->data = pvPortMalloc(msg->data_size);
    if (!msg_copy->data) while (1);

    // read the message
    serial_read(msg_copy->port, msg_copy->data, msg_copy->data_size);

    // sends the message to queue
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_serial_queue, &msg_copy, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


void TIMER0_IRQHandler(void)
{
	if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)== SET)
	{
		leds_clock();
	}

	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

/*
************************************************************************************************************************
*           TASKS
************************************************************************************************************************
*/

static void procotol_parser_task(void *pvParameters)
{
    msg_t proto_msg;
    serial_msg_t *serial_msg;

    UNUSED_PARAM(pvParameters);

    while(1)
    {
        // take the message from queue
        xQueueReceive(g_serial_queue, &serial_msg, portMAX_DELAY);

        // convert serial_msg to proto_msg
        proto_msg.sender_id = (int) serial_msg->port;
        proto_msg.data = (char *) serial_msg->data;
        proto_msg.data_size = (uint32_t) serial_msg->data_size;
        protocol_parse(&proto_msg);

        // free the memory
        vPortFree(serial_msg->data);
        vPortFree(serial_msg);
    }
}


/*
************************************************************************************************************************
*           PROTOCOL CALLBACKS
************************************************************************************************************************
*/

static void say_cb(proto_t *proto)
{
    protocol_response("say_cb", proto);
}


static void led_cb(proto_t *proto)
{
    protocol_response("resp do led", proto);

    color_t color;
    color.r = atoi(proto->list[2]);
    color.g = atoi(proto->list[3]);
    color.b = atoi(proto->list[4]);
    led_set_color(&leds[atoi(proto->list[1])], color);
}
