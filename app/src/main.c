
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <string.h>
#include <stdio.h>

#include "LPC177x_8x.h"
#include "lpc177x_8x_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "utils.h"
#include "serial.h"
#include "protocol.h"


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


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static void hardware_setup(void);
static void serial_cb(serial_msg_t *msg);
static void blink_led_task(void *pvParameters);
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
    /* initialize hardware */
    hardware_setup();

    g_serial_queue = xQueueCreate(5, sizeof(serial_msg_t *));


    /* create tasks */
    xTaskCreate(blink_led_task, (signed char *)"Blink led", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(procotol_parser_task, (signed char *)"Protocol parser", 1000, NULL, 2, NULL);

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* should never reach here! */
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

    /* set system tick for 1ms interrupt */
    SystemCoreClockUpdate();

    /* pins direction configuration */
    GPIO_SetDir(1, (1 << 22), GPIO_DIRECTION_OUTPUT); // cooler
    GPIO_SetDir(1, (1 << 23), GPIO_DIRECTION_OUTPUT); // power on PC
    GPIO_SetDir(1, (1 << 24), GPIO_DIRECTION_INPUT); // reset ARM

    /* turn on cooler */
    GPIO_SetValue(1, (1 << 22));

    /* pulse power on PC */
    GPIO_ClearValue(1, (1 << 23));
    for (delay = 0; delay < 5000000; delay++);
    GPIO_SetValue(1, (1 << 23));
    for (delay = 0; delay < 5000000; delay++);

    /* leds pins direction configuration */
    GPIO_SetDir(3, (1 << PIN_LED1), GPIO_DIRECTION_OUTPUT); // led 1
    GPIO_SetDir(3, (1 << PIN_LED2), GPIO_DIRECTION_OUTPUT); // led 2
    LPC_GPIO3->DIR |= (1 << PIN_LED3);    // (out) led3

    /* leds initial state */
    GPIO_SetValue(3, (1 << PIN_LED1));
    GPIO_SetValue(3, (1 << PIN_LED2));
    LPC_GPIO3->SET = (1 << PIN_LED3);

    /* serial initialization */
    serial_init();
    serial_set_callback(SERIAL0, serial_cb);
    serial_set_callback(SERIAL1, serial_cb);

    /* protocol definitions */
    protocol_add_command("say %s", say_cb);
    protocol_add_command("led %i %i", led_cb);
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


/*
************************************************************************************************************************
*           TASKS
************************************************************************************************************************
*/

static void blink_led_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while(1)
    {
        GPIO_SetValue(3, (1 << PIN_LED1));
        GPIO_ClearValue(3, (1 << PIN_LED2));
        vTaskDelay(500/portTICK_RATE_MS);

        GPIO_ClearValue(3, (1 << PIN_LED1));
        GPIO_SetValue(3, (1 << PIN_LED2));
        vTaskDelay(500/portTICK_RATE_MS);
    }
}


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
    protocol_response("led_cb", proto);
}

