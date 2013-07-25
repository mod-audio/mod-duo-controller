
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
#include "actuator.h"
#include "data.h"
#include "naveg.h"
#include "screen.h"

#include "usb.h"
#include "usbhw.h"
#include "usbcore.h"
#include "cdcuser.h"


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

static xQueueHandle g_msg_queue, g_actuators_queue;


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

// local functions
static void serial_cb(uint8_t port);
static void actuators_cb(void *actuator);
static void usb_receive_cb(uint32_t msg_size);

// tasks
static void procotol_task(void *pvParameters);
static void displays_task(void *pvParameters);
static void actuators_task(void *pvParameters);

// protocol callbacks
static void say_cb(proto_t *proto);
static void led_cb(proto_t *proto);
static void control_add_cb(proto_t *proto);
static void control_rm_cb(proto_t *proto);
static void control_set_cb(proto_t *proto);
static void control_get_cb(proto_t *proto);
static void peakmeter_cb(proto_t *proto);
static void tuner_cb(proto_t *proto);
static void banks_cb(proto_t *proto);
static void pedalboards_cb(proto_t *proto);


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
    serial_init(1, SERIAL1_BAUDRATE, SERIAL1_PRIORITY);
    serial_set_callback(SERIAL1, serial_cb);

    // actuators callbacks
    actuator_set_event(hardware_actuators(ENCODER0), actuators_cb);
    actuator_set_event(hardware_actuators(ENCODER1), actuators_cb);
    actuator_set_event(hardware_actuators(ENCODER2), actuators_cb);
    actuator_set_event(hardware_actuators(ENCODER3), actuators_cb);
    actuator_enable_event(hardware_actuators(ENCODER0), EV_ALL_ENCODER_EVENTS);
    actuator_enable_event(hardware_actuators(ENCODER1), EV_ALL_ENCODER_EVENTS);
    actuator_enable_event(hardware_actuators(ENCODER2), EV_ALL_ENCODER_EVENTS);
    actuator_enable_event(hardware_actuators(ENCODER3), EV_ALL_ENCODER_EVENTS);
    actuator_set_event(hardware_actuators(FOOTSWITCH0), actuators_cb);
    actuator_set_event(hardware_actuators(FOOTSWITCH1), actuators_cb);
    actuator_set_event(hardware_actuators(FOOTSWITCH2), actuators_cb);
    actuator_set_event(hardware_actuators(FOOTSWITCH3), actuators_cb);
    actuator_enable_event(hardware_actuators(FOOTSWITCH0), EV_ALL_BUTTON_EVENTS);
    actuator_enable_event(hardware_actuators(FOOTSWITCH1), EV_ALL_BUTTON_EVENTS);
    actuator_enable_event(hardware_actuators(FOOTSWITCH2), EV_ALL_BUTTON_EVENTS);
    actuator_enable_event(hardware_actuators(FOOTSWITCH3), EV_ALL_BUTTON_EVENTS);

    // displays initialization
    glcd_init();

    // protocol definitions
    protocol_add_command(SAY_CMD, say_cb);
    protocol_add_command(LED_CMD, led_cb);
    protocol_add_command(CONTROL_ADD_CMD, control_add_cb);
    protocol_add_command(CONTROL_REMOVE_CMD, control_rm_cb);
    protocol_add_command(CONTROL_SET_CMD, control_set_cb);
    protocol_add_command(CONTROL_GET_CMD, control_get_cb);
    protocol_add_command(PEAKMETER_CMD, peakmeter_cb);
    protocol_add_command(TUNER_CMD, tuner_cb);
    protocol_add_command(BANKS_CMD, banks_cb);
    protocol_add_command(PEDALBOARDS_CMD, pedalboards_cb);

    // navegation initialization
    naveg_init();

    // CDC initialization
    CDC_Init();
    CDC_SetMessageCallback(usb_receive_cb);

    // usb initialization
    USB_Init(1);
    USB_Connect(USB_CONNECT);
    while (!USB_Configuration);

    // create the queues
    g_msg_queue = xQueueCreate(5, sizeof(msg_t *));
    g_actuators_queue = xQueueCreate(10, sizeof(uint8_t *));

    // create tasks
    xTaskCreate(procotol_task, NULL, 1000, NULL, 2, NULL);
    xTaskCreate(actuators_task, NULL, 1000, NULL, 2, NULL);
    xTaskCreate(displays_task, NULL, 1000, NULL, 1, NULL);

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
static void serial_cb(uint8_t port)
{
    uint8_t buffer[128];
    uint32_t read;

    read = serial_read(port, buffer, sizeof(buffer));
    serial_send(port, buffer, read);
}

// this callback is called from UART ISR in case of error
void serial_error(uint8_t port, uint32_t error)
{
    UNUSED_PARAM(port);
    UNUSED_PARAM(error);

    // FIXME: need feedback when an error happen
    while (1);
}

// this callback is called from a ISR
static void actuators_cb(void *actuator)
{
    // does a copy of actuator id and status
    uint8_t *actuator_info;
    actuator_info = (uint8_t *) pvPortMalloc(sizeof(uint8_t) * 3);

    // fills the actuator info vector
    actuator_info[0] = ((button_t *)(actuator))->type;
    actuator_info[1] = ((button_t *)(actuator))->id;
    actuator_info[2] = actuator_get_status(actuator);

    // sends the actuator to queue
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_actuators_queue, &actuator_info, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void usb_receive_cb(uint32_t msg_size)
{
    uint8_t *msg_copy;

    // does a copy of the message
    msg_copy = pvPortMalloc(msg_size);
    CDC_GetMessage(msg_copy);

    // creates a message struct
    msg_t *msg;
    msg = pvPortMalloc(sizeof(msg_t));
    msg->sender_id = 0;
    msg->data = (char*) msg_copy;
    msg->data_size = msg_size;

    // queue the message
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_msg_queue, &msg, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


/*
************************************************************************************************************************
*           TASKS
************************************************************************************************************************
*/

static void procotol_task(void *pvParameters)
{
    msg_t *msg;

    UNUSED_PARAM(pvParameters);

    while (1)
    {
        portBASE_TYPE xStatus;

        // takes the message from queue
        xStatus = xQueueReceive(g_msg_queue, &msg, portMAX_DELAY);

        // checks if message has successfully taken
        if (xStatus == pdPASS)
        {
            // parses the message
            protocol_parse(msg);

            // free the message memory
            vPortFree(msg->data);
            vPortFree(msg);
        }
    }
}

static void displays_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while (1)
    {
        taskENTER_CRITICAL();
        glcd_update();
        taskEXIT_CRITICAL();
        taskYIELD();
    }
}

static void actuators_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    uint8_t type, id, status;
    uint8_t *actuator_info;

    while (1)
    {
        portBASE_TYPE xStatus;

        // take the actuator from queue
        xStatus = xQueueReceive(g_actuators_queue, &actuator_info, portMAX_DELAY);

        // checks if actuator has successfully taken
        if (xStatus == pdPASS)
        {
            type = actuator_info[0];
            id = actuator_info[1];
            status = actuator_info[2];

            // encoders
            if (type == ROTARY_ENCODER)
            {
                if (BUTTON_CLICKED(status))
                {
                    naveg_next_control(id);
                    naveg_enter(id);
                }
                if (BUTTON_HOLD(status))
                {
                    naveg_toggle_tool(id);
                }
                if (ENCODER_TURNED_CW(status))
                {
                    naveg_inc_control(id);
                    naveg_down(id);
                }
                if (ENCODER_TURNED_ACW(status))
                {
                    naveg_dec_control(id);
                    naveg_up(id);
                }
            }

            // footswitches
            else if (type == BUTTON)
            {
                if (BUTTON_PRESSED(status))
                {
                    naveg_foot_change(id);
                }
            }

            vPortFree(actuator_info);
        }
    }
}


/*
************************************************************************************************************************
*           PROTOCOL CALLBACKS
************************************************************************************************************************
*/

static void say_cb(proto_t *proto)
{
    protocol_response(proto->list[1], proto);
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

static void control_add_cb(proto_t *proto)
{
    control_t *control = data_parse_control(proto->list);

    if (control->hardware_type == QUADRA_HW)
    {
        naveg_add_control(control);
    }
    // TODO: implement the others hardwares type

    protocol_response("resp 0", proto);
}

static void control_rm_cb(proto_t *proto)
{
    naveg_remove_control(atoi(proto->list[1]), proto->list[2]);
    protocol_response("resp 0", proto);
}

static void control_set_cb(proto_t *proto)
{
    naveg_set_control(atoi(proto->list[1]), proto->list[2], atof(proto->list[3]));
    protocol_response("resp 0", proto);
}

static void control_get_cb(proto_t *proto)
{
    float value;
    value = naveg_get_control(atoi(proto->list[1]), proto->list[2]);

    char resp[32];
    float_to_str(value, resp, sizeof(resp), 3);
    protocol_response(resp, proto);
}

static void peakmeter_cb(proto_t *proto)
{
    screen_set_peakmeter(atoi(proto->list[1]), atof(proto->list[2]));
    protocol_response("resp 0", proto);
}

static void tuner_cb(proto_t *proto)
{
    screen_set_tuner(atof(proto->list[1]), proto->list[2], atoi(proto->list[3]));
    protocol_response("resp 0", proto);
}

static void banks_cb(proto_t *proto)
{
    bp_list_t *bp_list = naveg_get_banks();

    // free the current list
    if (bp_list) data_free_banks_list(bp_list);

    // parses the list
    bp_list = data_parse_banks_list(&(proto->list[1]), proto->list_count);
    naveg_set_banks(bp_list);
}

static void pedalboards_cb(proto_t *proto)
{
    bp_list_t *bp_list = naveg_get_pedalboards();

    // free the current list
    if (bp_list) data_free_pedalboards_list(bp_list);

    // parses the list
    bp_list = data_parse_pedalboards_list(&(proto->list[1]), proto->list_count);
    naveg_set_pedalboards(bp_list);
}
