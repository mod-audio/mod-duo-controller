
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
#include "cli.h"
#include "chain.h"

#include "usb.h"
#include "usbhw.h"
#include "usbcore.h"
#include "cdcuser.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define MSG_QUEUE_DEPTH     5


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
static uint8_t g_msg_buffer[CDC_RX_BUFFER_SIZE];


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
static void monitor_task(void *pvParameters);
static void setup_task(void *pvParameters);

// protocol callbacks
static void ping_cb(proto_t *proto);
static void say_cb(proto_t *proto);
static void led_cb(proto_t *proto);
static void gui_connection_cb(proto_t *proto);
static void control_add_cb(proto_t *proto);
static void control_rm_cb(proto_t *proto);
static void control_set_cb(proto_t *proto);
static void control_get_cb(proto_t *proto);
static void bank_config_cb(proto_t *proto);
static void clipmeter_cb(proto_t *proto);
static void peakmeter_cb(proto_t *proto);
static void tuner_cb(proto_t *proto);
static void xrun_cb(proto_t *proto);
static void resp_cb(proto_t *proto);


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

    // this task is used to setup the system and create the other tasks
    xTaskCreate(setup_task, NULL, 256, NULL, 1, NULL);

    // start the scheduler
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
    char buffer[SERIAL_RX_BUFFER_SIZE];
    uint32_t read_bytes;

    read_bytes = serial_read(port, (uint8_t*) buffer, sizeof(buffer));

    if (port == CLI_SERIAL)
        cli_append_data(buffer, read_bytes);
    else if (port == CONTROL_CHAIN_SERIAL)
        control_chain_append_data((uint8_t*) buffer, read_bytes);
}

// this callback is called from UART ISR in case of error
void serial_error(uint8_t port, uint32_t error)
{
    UNUSED_PARAM(port);
    UNUSED_PARAM(error);
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
    static uint32_t idx, msg_sizes[MSG_QUEUE_DEPTH];

    // stores the message size
    msg_sizes[idx] = msg_size;

    // queue the message size
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_msg_queue, &msg_sizes[idx], &xHigherPriorityTaskWoken);

    // goes to next msg_size position
    if (++idx == MSG_QUEUE_DEPTH) idx = 0;

    // forces switch to task after leave this callback
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


/*
************************************************************************************************************************
*           TASKS
************************************************************************************************************************
*/

static void procotol_task(void *pvParameters)
{
    uint32_t msg_size;

    UNUSED_PARAM(pvParameters);

    while (1)
    {
        portBASE_TYPE xStatus;

        // takes the message from queue
        xStatus = xQueueReceive(g_msg_queue, &msg_size, portMAX_DELAY);

        // checks if message has successfully taken
        if (xStatus == pdPASS)
        {
            // gets the message
            CDC_GetMessage(g_msg_buffer, msg_size);

            // parses the message
            msg_t msg;
            msg.sender_id = 0;
            msg.data = (char *) g_msg_buffer;
            msg.data_size = msg_size;
            protocol_parse(&msg);
        }
    }
}

static void displays_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while (1)
    {
        // update the glcd
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

static void monitor_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while (1)
    {
        // reset the screen icons
        screen_clipmeter(0xFF, 0);
        screen_xrun(0);

        // checks the headphone
        hardware_headphone();

        // process the command line
        cli_process();

        // process the control chain data
        control_chain_process();

        taskYIELD();
    }
}

static void setup_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    // displays initialization
    glcd_init();

    // serial initialization and callbacks definitions
    // serial 1 is used to commutication with linux console
    serial_init(SERIAL1, SERIAL1_BAUDRATE, SERIAL1_PRIORITY);
    serial_set_callback(SERIAL1, serial_cb);
    // serial 2 is used to external devices communication
    serial_init(SERIAL2, SERIAL2_BAUDRATE, SERIAL2_PRIORITY);
    serial_set_callback(SERIAL2, serial_cb);

    // create the queues
    g_msg_queue = xQueueCreate(MSG_QUEUE_DEPTH, sizeof(uint32_t *));
    g_actuators_queue = xQueueCreate(10, sizeof(uint8_t *));

    // create the tasks
    xTaskCreate(procotol_task, NULL, 512, NULL, 3, NULL);
    xTaskCreate(actuators_task, NULL, 256, NULL, 2, NULL);
    xTaskCreate(displays_task, NULL, 128, NULL, 1, NULL);
    xTaskCreate(monitor_task, NULL, 256, NULL, 1, NULL);

    // checks the system boot
    system_check_boot();

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
    actuator_enable_event(hardware_actuators(FOOTSWITCH0), EV_BUTTON_PRESSED);
    actuator_enable_event(hardware_actuators(FOOTSWITCH1), EV_BUTTON_PRESSED);
    actuator_enable_event(hardware_actuators(FOOTSWITCH2), EV_BUTTON_PRESSED);
    actuator_enable_event(hardware_actuators(FOOTSWITCH3), EV_BUTTON_PRESSED);

    // navegation initialization
    naveg_init();

    // protocol definitions
    protocol_add_command(PING_CMD, ping_cb);
    protocol_add_command(SAY_CMD, say_cb);
    protocol_add_command(LED_CMD, led_cb);
    protocol_add_command(GUI_CONNECTED_CMD, gui_connection_cb);
    protocol_add_command(GUI_DISCONNECTED_CMD, gui_connection_cb);
    protocol_add_command(CONTROL_ADD_CMD, control_add_cb);
    protocol_add_command(CONTROL_REMOVE_CMD, control_rm_cb);
    protocol_add_command(CONTROL_SET_CMD, control_set_cb);
    protocol_add_command(CONTROL_GET_CMD, control_get_cb);
    protocol_add_command(BANK_CONFIG_CMD, bank_config_cb);
    protocol_add_command(CLIPMETER_CMD, clipmeter_cb);
    protocol_add_command(PEAKMETER_CMD, peakmeter_cb);
    protocol_add_command(TUNER_CMD, tuner_cb);
    protocol_add_command(XRUN_CMD, xrun_cb);
    protocol_add_command(RESPONSE_CMD, resp_cb);

    // cdc initialization
    CDC_Init();
    CDC_SetMessageCallback(usb_receive_cb);

    // usb initialization
    USB_Init(1);
    USB_Connect(USB_CONNECT);
    while (!USB_Configuration);

    // CLI initialization
    cli_init();

    // deletes itself
    vTaskDelete(NULL);
}


/*
************************************************************************************************************************
*           PROTOCOL CALLBACKS
************************************************************************************************************************
*/

static void ping_cb(proto_t *proto)
{
    static uint8_t ping_ok;

    if (!ping_ok)
    {
        hardware_set_true_bypass(PROCESS);
        ping_ok = 1;
    }

    hardware_reset(UNBLOCK);
    protocol_response("resp 0", proto);
}

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

static void gui_connection_cb(proto_t *proto)
{
    protocol_response("resp 0", proto);
}

static void control_add_cb(proto_t *proto)
{
    control_t *control = data_parse_control(proto->list);
    naveg_add_control(control);

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

static void bank_config_cb(proto_t *proto)
{
    bank_config_t bank_func;
    bank_func.hardware_type = atoi(proto->list[1]);
    bank_func.hardware_id = atoi(proto->list[2]);
    bank_func.actuator_type = atoi(proto->list[3]);
    bank_func.actuator_id = atoi(proto->list[4]);
    bank_func.function = atoi(proto->list[5]);
    naveg_bank_config(&bank_func);
    protocol_response("resp 0", proto);
}

static void peakmeter_cb(proto_t *proto)
{
    screen_peakmeter(atoi(proto->list[1]), atof(proto->list[2]));
    protocol_response("resp 0", proto);
}

static void tuner_cb(proto_t *proto)
{
    screen_tuner(atof(proto->list[1]), proto->list[2], atoi(proto->list[3]));
    protocol_response("resp 0", proto);
}

static void clipmeter_cb(proto_t *proto)
{
    screen_clipmeter(atoi(proto->list[1]), 1);
    protocol_response("resp 0", proto);
}

static void xrun_cb(proto_t *proto)
{
    screen_xrun(1);
    protocol_response("resp 0", proto);
}

static void resp_cb(proto_t *proto)
{
    comm_webgui_response_cb(proto->list);
}

/*
************************************************************************************************************************
*           ERRORS CALLBACKS
************************************************************************************************************************
*/

// TODO: better error feedback for below functions

void HardFault_Handler(void)
{
    led_set_color(hardware_leds(0), CYAN);
    while (1);
}

void MemManage_Handler(void)
{
    led_set_color(hardware_leds(1), CYAN);
    while (1);
}

void BusFault_Handler(void)
{
    led_set_color(hardware_leds(2), CYAN);
    while (1);
}

void UsageFault_Handler(void)
{
    led_set_color(hardware_leds(3), CYAN);
    while (1);
}

void vApplicationMallocFailedHook(void)
{
    led_set_color(hardware_leds(0), RED);
    led_set_color(hardware_leds(1), RED);
    led_set_color(hardware_leds(2), RED);
    led_set_color(hardware_leds(3), RED);
    while (1);
}
