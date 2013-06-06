
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "config.h"
#include "hardware.h"
#include "serial.h"
#include "protocol.h"
#include "glcd.h"
#include "led.h"


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

// local functions
static void serial_cb(serial_msg_t *msg);

// tasks
static void procotol_parser_task(void *pvParameters);
static void displays_update_task(void *pvParameters);
static void test_task(void *pvParameters);

// protocol callbacks
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

    // serial initialization and callbacks definitions
    serial_init();
    serial_set_callback(SERIAL0, serial_cb);
    serial_set_callback(SERIAL1, serial_cb);

    // displays initialization
    glcd_init();

    // protocol definitions
    protocol_add_command(SAY_CMD, say_cb);
    protocol_add_command(LED_CMD, led_cb);

    // create a queue to serial
    g_serial_queue = xQueueCreate(5, sizeof(serial_msg_t *));

    // create tasks
    xTaskCreate(procotol_parser_task, NULL, 1000, NULL, 3, NULL);
    xTaskCreate(displays_update_task, NULL, 1000, NULL, 2, NULL);
    xTaskCreate(test_task, NULL, 1000, NULL, 2, NULL);

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

// this callback is called from a ISR
static void serial_cb(serial_msg_t *msg)
{
    serial_msg_t *msg_copy;

    // does a copy of message
    msg_copy = pvPortMalloc(sizeof(serial_msg_t));
    if (!msg_copy) while (1);
    memcpy(msg_copy, msg, sizeof(serial_msg_t));

    // allocate memory to store the message
    msg_copy->data = pvPortMalloc(msg->data_size + 1);
    if (!msg_copy->data) while (1);

    // read the message and append a NULL terminator
    serial_read(msg_copy->port, msg_copy->data, msg_copy->data_size);
    msg_copy->data[msg_copy->data_size] = 0;

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

static void displays_update_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while (1)
    {
        glcd_update();
        taskYIELD();
    }
}

static void test_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    static unsigned int counter;
    char buffer[128];

    while (1)
    {
        int_to_str(counter, buffer, sizeof(buffer), 4);
        glcd_text(0, 0, 0, buffer, NULL, GLCD_BLACK);
        counter++;
        taskYIELD();
    }
}


/*
************************************************************************************************************************
*           PROTOCOL CALLBACKS
************************************************************************************************************************
*/

static void say_cb(proto_t *proto)
{
    protocol_response(strarr_join(&(proto->list[1])), proto);
}

static void led_cb(proto_t *proto)
{
    color_t color;
    led_t *led = hardware_leds(atoi(proto->list[1]));
    color.r = atoi(proto->list[2]);
    color.g = atoi(proto->list[3]);
    color.b = atoi(proto->list[4]);
    led_set_color(led, color);

    if (proto->list_count == 7)
    {
        led_blink(led, atoi(proto->list[5]), atoi(proto->list[6]));
    }

    protocol_response("resp 0", proto);
}
