
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
#include "comm.h"
#include "chain.h"


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
#define TASK_NAME(name)     ((const signed char * const) (name))


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static xQueueHandle g_actuators_queue;
static uint8_t g_msg_buffer[WEBGUI_COMM_RX_BUFF_SIZE];
static uint8_t g_ui_communication_started;


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

// local functions
static void serial_cb(serial_t *serial);
static void actuators_cb(void *actuator);

// tasks
static void procotol_task(void *pvParameters);
static void displays_task(void *pvParameters);
static void actuators_task(void *pvParameters);
//static void monitor_task(void *pvParameters);
//static void chain_task(void *pvParameters);
static void setup_task(void *pvParameters);

// protocol callbacks
static void ping_cb(proto_t *proto);
static void say_cb(proto_t *proto);
static void led_cb(proto_t *proto);
static void glcd_text_cb(proto_t *proto);
static void glcd_draw_cb(proto_t *proto);
static void gui_connection_cb(proto_t *proto);
static void control_add_cb(proto_t *proto);
static void control_rm_cb(proto_t *proto);
static void control_set_cb(proto_t *proto);
static void control_get_cb(proto_t *proto);
static void initial_state_cb(proto_t *proto);
static void bank_config_cb(proto_t *proto);
static void clipmeter_cb(proto_t *proto);
static void peakmeter_cb(proto_t *proto);
static void tuner_cb(proto_t *proto);
static void xrun_cb(proto_t *proto);
static void resp_cb(proto_t *proto);
//static void chain_cb(proto_t *proto);


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
//led_set_color(hardware_leds(0), GREEN);
//led_set_color(hardware_leds(1), YELLOW);
//led_blink(hardware_leds(0), 500, 500);
//glcd_backlight(hardware_glcds(0), GLCD_BACKLIGHT_OFF);
//glcd_backlight(hardware_glcds(1), GLCD_BACKLIGHT_OFF);
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
static void serial_cb(serial_t *serial)
{
    uint32_t read_bytes;
    uint8_t uart_id, buffer[SERIAL_MAX_RX_BUFF_SIZE];

    uart_id = serial->uart_id;
    read_bytes = serial_read(uart_id, buffer, sizeof(buffer));

    if (uart_id == CLI_SERIAL)
    {
        cli_append_data((const char*)buffer, read_bytes);
    }
}

// this callback is called from UART ISR in case of error
void serial_error(uint8_t uart_id, uint32_t error)
{
    UNUSED_PARAM(uart_id);
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


/*
************************************************************************************************************************
*           TASKS
************************************************************************************************************************
*/

static void procotol_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while (1)
    {
        uint32_t msg_size;

        // blocks until receive a new message
        ringbuff_t *rb = comm_webgui_read();
        msg_size = ringbuff_read_until(rb, g_msg_buffer, WEBGUI_COMM_RX_BUFF_SIZE, 0);

        // parses the message
        if (msg_size > 0)
        {
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
        taskENTER_CRITICAL();

        // update the GLCDs
        uint8_t i;
        for (i = 0; i < GLCD_COUNT; i++)
            glcd_update(hardware_glcds(i));

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
static void monitor_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while (1)
    {
        // reset the screen icons
        screen_clipmeter(0xFF, 0);
        screen_xrun(0);

        // hardware verifications
        hardware_headphone();
        hardware_temperature();

        // process the command line
        cli_process();

        // TODO: timer for navigation update
        //naveg_update();

        taskYIELD();
    }
}
*/
/*
static void chain_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while (1)
    {
        // As soon as a message is received from control chain the message will be
        // directed to UI. The below function implements a binary semaphore internally.
        uint32_t read = chain_dev2ui_pop(g_msg_buffer, WEBGUI_COMM_RX_BUFF_SIZE);

        // sends the message to UI. Ignores the last byte (read - 1) to prevent send the \0 twice.
        if (read) comm_webgui_send((const char*)g_msg_buffer, read-1);

        taskYIELD();
    }
}
*/
static void setup_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    // set serial callbacks
    serial_set_callback(CLI_SERIAL, serial_cb);
//    serial_set_callback(CONTROL_CHAIN_SERIAL, serial_cb);

    // initialize the communication resources
    comm_init();

    // initialize the control chain
//    chain_init();

    // create the queues
    g_actuators_queue = xQueueCreate(10, sizeof(uint8_t *));

    // create the tasks
    xTaskCreate(procotol_task, TASK_NAME("proto"), 512, NULL, 3, NULL);
//    xTaskCreate(chain_task, TASK_NAME("chain"), 256, NULL, 4, NULL);
    xTaskCreate(actuators_task, TASK_NAME("act"), 256, NULL, 2, NULL);
    xTaskCreate(displays_task, TASK_NAME("disp"), 128, NULL, 1, NULL);
//    xTaskCreate(monitor_task, TASK_NAME("mon"), 256, NULL, 1, NULL);

    // checks the system boot
//    system_check_boot();

    // first boot screen feedback
    screen_boot_feedback(0);

    // actuators callbacks
    uint8_t i;
    for (i = 0; i < SLOTS_COUNT; i++)
    {
        actuator_set_event(hardware_actuators(ENCODER0 + i), actuators_cb);
        actuator_enable_event(hardware_actuators(ENCODER0 + i), EV_ALL_ENCODER_EVENTS);
        actuator_set_event(hardware_actuators(FOOTSWITCH0 + i), actuators_cb);
        actuator_enable_event(hardware_actuators(FOOTSWITCH0 + i), EV_BUTTON_PRESSED);
    }

    // protocol definitions
    protocol_add_command(PING_CMD, ping_cb);
    protocol_add_command(SAY_CMD, say_cb);
    protocol_add_command(LED_CMD, led_cb);
    protocol_add_command(GLCD_TEXT_CMD, glcd_text_cb);
    protocol_add_command(GLCD_DRAW_CMD, glcd_draw_cb);
    protocol_add_command(GUI_CONNECTED_CMD, gui_connection_cb);
    protocol_add_command(GUI_DISCONNECTED_CMD, gui_connection_cb);
    protocol_add_command(CONTROL_ADD_CMD, control_add_cb);
    protocol_add_command(CONTROL_REMOVE_CMD, control_rm_cb);
    protocol_add_command(CONTROL_SET_CMD, control_set_cb);
    protocol_add_command(CONTROL_GET_CMD, control_get_cb);
    protocol_add_command(INITIAL_STATE_CMD, initial_state_cb);
    protocol_add_command(BANK_CONFIG_CMD, bank_config_cb);
    protocol_add_command(CLIPMETER_CMD, clipmeter_cb);
    protocol_add_command(PEAKMETER_CMD, peakmeter_cb);
    protocol_add_command(TUNER_CMD, tuner_cb);
    protocol_add_command(XRUN_CMD, xrun_cb);
    protocol_add_command(RESPONSE_CMD, resp_cb);
//    protocol_add_command(CHAIN_CMD, chain_cb);

    // CLI initialization
    cli_init();

    // the grub is only for mod quadra
    // this will be deprecated in the future
#ifdef CLI_GRUB
    while (cli_boot_stage() < LOGIN_STAGE);
    screen_boot_feedback(1);

    while (cli_boot_stage() < PROMPT_READY_STAGE);
    screen_boot_feedback(2);

    while (!g_ui_communication_started);
    screen_boot_feedback(3);
#endif

    // init the navigation
    naveg_init();

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
    if (!g_ui_communication_started)
    {
        hardware_set_true_bypass(PROCESS);
        g_ui_communication_started = 1;
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

static void glcd_text_cb(proto_t *proto)
{
    uint8_t glcd_id, x, y;
    glcd_id = atoi(proto->list[1]);
    x = atoi(proto->list[2]);
    y = atoi(proto->list[3]);

    if (glcd_id >= GLCD_COUNT) return;

    glcd_text(hardware_glcds(glcd_id), x, y, proto->list[4], NULL, GLCD_BLACK);
    protocol_response("resp 0", proto);
}

static void glcd_draw_cb(proto_t *proto)
{
    uint8_t glcd_id, x, y;
    glcd_id = atoi(proto->list[1]);
    x = atoi(proto->list[2]);
    y = atoi(proto->list[3]);

    if (glcd_id >= GLCD_COUNT) return;

    str_to_hex(proto->list[4], g_msg_buffer, sizeof(g_msg_buffer));
    glcd_draw_image(hardware_glcds(glcd_id), x, y, g_msg_buffer, GLCD_BLACK);

    protocol_response("resp 0", proto);
}

static void gui_connection_cb(proto_t *proto)
{
    if (strcmp(proto->list[0], GUI_CONNECTED_CMD) == 0)
        naveg_ui_connection(UI_CONNECTED);
    else
        naveg_ui_connection(UI_DISCONNECTED);

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
    strcpy(resp, "resp 0 ");

    float_to_str(value, &resp[strlen(resp)], 8, 3);
    protocol_response(resp, proto);
}

static void initial_state_cb(proto_t *proto)
{
    naveg_initial_state(proto->list[1], proto->list[2], &(proto->list[3]));
    protocol_response("resp 0", proto);
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
    screen_peakmeter(atoi(proto->list[1]), atof(proto->list[2]), atof(proto->list[3]));
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
static void chain_cb(proto_t *proto)
{
    chain_ui2dev(proto->list[1]);
    protocol_response("resp 0", proto);
}
*/


/*
************************************************************************************************************************
*           ERRORS CALLBACKS
************************************************************************************************************************
*/

// TODO: better error feedback for below functions

void HardFault_Handler(void)
{
    led_set_color(hardware_leds(0), CYAN);
    hardware_reset(UNBLOCK);
    while (1);
}

void MemManage_Handler(void)
{
    HardFault_Handler();
}

void BusFault_Handler(void)
{
    HardFault_Handler();
}

void UsageFault_Handler(void)
{
    HardFault_Handler();
}

void vApplicationMallocFailedHook(void)
{
    led_set_color(hardware_leds(1), CYAN);
    hardware_reset(UNBLOCK);
    while (1);
}

void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName)
{
    UNUSED_PARAM(pxTask);
    glcd_t *glcd0 =  hardware_glcds(0);
    glcd_clear(glcd0, GLCD_WHITE);
    glcd_text(glcd0, 0, 0, "stack overflow", NULL, GLCD_BLACK);
    glcd_text(glcd0, 0, 10, (const char *) pcTaskName, NULL, GLCD_BLACK);
    glcd_update(glcd0);
    while (1);
}
