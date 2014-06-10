
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "chain.h"
#include "config.h"
#include "data.h"
#include "hardware.h"
#include "comm.h"
#include "utils.h"
#include "naveg.h"

#include <string.h>
#include <math.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

// this is the size of receive buffer
#define CONTROL_CHAIN_BUFFER_SIZE           32
// this is the minimal quantity of data receive or send by message, in other words, data_size = 0
#define CONTROL_CHAIN_MINIMAL_MSG_SIZE      9
// calculates the timeout count value
#define CONTROL_CHAIN_TIMEOUT_COUNT         (CONTROL_CHAIN_TIMEOUT / CONTROL_CHAIN_PERIOD)


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

typedef struct DEVICE_T {
    uint8_t hardware_type, hardware_id;
    uint32_t count;
} device_t;


/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static control_t *g_control_chain[CONTROL_CHAIN_MAX_DEVICES];
static uint8_t g_buffer[CONTROL_CHAIN_BUFFER_SIZE], g_buffer_idx, g_new_msg, g_waiting_response;
static device_t g_connected_devices[CONTROL_CHAIN_MAX_DEVICES];
static uint8_t g_data[CONTROL_CHAIN_BUFFER_SIZE - CONTROL_CHAIN_MINIMAL_MSG_SIZE];


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static uint8_t destination_ok(control_chain_t *chain);
static uint8_t checksum_ok(control_chain_t *chain, uint8_t *checksum_value);
static void parse_data(control_chain_t *chain);
static void update_control_value(control_chain_t *chain, uint8_t *actuators_list, uint8_t actuators_amount);
static void connected_device_add(uint8_t hw_type, uint8_t hw_id);
static void connected_device_remove(device_t *device);
static void send_request(device_t *device);
static void control_set(control_t *control);


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

// calculates the control value using the step
static void step_to_value(control_t *control)
{
    // about the calculation: http://lv2plug.in/ns/ext/port-props/#rangeSteps

    float p_step = ((float) control->step) / ((float) (control->steps - 1));
    switch (control->properties)
    {
        case CONTROL_PROP_LINEAR:
        case CONTROL_PROP_INTEGER:
            control->value = (p_step * (control->maximum - control->minimum)) + control->minimum;
            break;

        case CONTROL_PROP_LOGARITHMIC:
            control->value = control->minimum * pow(control->maximum / control->minimum, p_step);
            break;

        case CONTROL_PROP_ENUMERATION:
            control->value = control->scale_points[control->step]->value;
            break;
    }

    if (control->value > control->maximum) control->value = control->maximum;
    if (control->value < control->minimum) control->value = control->minimum;
}

// copy an command to buffer
static uint8_t copy_command(char *buffer, const char *command)
{
    uint8_t i = 0;
    const char *cmd = command;

    while (*cmd && (*cmd != '%' && *cmd != '.'))
    {
        buffer[i++] = *cmd;
        cmd++;
    }

    return i;
}

static uint8_t destination_ok(control_chain_t *chain)
{
    // master devices has hardware type and hardware id equal to zero
    if (chain->hw_type_destination == 0 && chain->hw_id_destination == 0) return 1;

    return 0;
}

static uint8_t checksum_ok(control_chain_t *chain, uint8_t *checksum_value)
{
    uint8_t checksum = 0;

    if (!chain) return 0;

    checksum += chain->start;
    checksum += chain->hw_type_origin;
    checksum += chain->hw_id_origin;
    checksum += chain->hw_type_destination;
    checksum += chain->hw_id_destination;
    checksum += chain->function;
    checksum += chain->data_size;

    uint32_t i;
    for (i = 0; i < chain->data_size; i++) checksum += chain->data[i];

    if (checksum_value) *checksum_value = checksum;

    return (checksum == chain->checksum);
}

static void parse_data(control_chain_t *chain)
{
    if (!destination_ok(chain) || !checksum_ok(chain, NULL)) return;

    uint32_t i, j;
    uint8_t data[sizeof(control_chain_t)];
    control_chain_t *msg_to_send;

    // points the struct to buffer
    msg_to_send = (control_chain_t *) data;

    uint8_t actuators_list[CONTROL_CHAIN_MAX_ACTUATORS_PER_DEVICES];

    switch (chain->function)
    {
        case CONTROL_CHAIN_REQUEST_CONNECTION:
            // adds the device to connected device list
            connected_device_add(chain->hw_type_origin, chain->hw_id_origin);

            // fills the message struct
            msg_to_send->start = CONTROL_CHAIN_PROTOCOL_START;
            msg_to_send->hw_type_origin = 0;
            msg_to_send->hw_id_origin = 0;
            msg_to_send->hw_type_destination = chain->hw_type_origin;
            msg_to_send->hw_id_destination = chain->hw_id_origin;
            msg_to_send->function = CONTROL_CHAIN_CONFIRM_CONNECTION;
            msg_to_send->data_size = 0;
            msg_to_send->data = NULL;
            msg_to_send->end = CONTROL_CHAIN_PROTOCOL_END;

            // message checksum
            uint8_t checksum;
            checksum_ok(msg_to_send, &checksum);
            msg_to_send->checksum = checksum;

            // sends the message
            comm_control_chain_send(data, CONTROL_CHAIN_MINIMAL_MSG_SIZE);
            break;

        case CONTROL_CHAIN_DATA_RESPONSE:
            for (i = 0, j = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
            {
                if (!g_control_chain[i]) continue;

                control_t *control = g_control_chain[i];

                // checks if received hardware type and id match with any assigned control
                if (control->hardware_type == chain->hw_type_origin &&
                    control->hardware_id == chain->hw_id_origin)
                {
                    // appends the control index to actuators list
                    actuators_list[j++] = i;

                    if (j == CONTROL_CHAIN_MAX_ACTUATORS_PER_DEVICES) break;
                }
            }

            update_control_value(chain, actuators_list, j);
            break;
    }
}

static void update_control_value(control_chain_t *chain, uint8_t *actuators_list, uint8_t actuators_amount)
{
    uint8_t count = 0, *pdata = chain->data;

    // searches the control in the connected devices list
    uint32_t i;
    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        if (g_connected_devices[i].hardware_type == chain->hw_type_origin &&
            g_connected_devices[i].hardware_id == chain->hw_id_origin )
        {
            // resets the counter for that a new request happens
            g_connected_devices[i].count = 0;
            break;
        }
    }

    if (actuators_amount == 0) return;

    float value;
    switch (chain->hw_type_origin)
    {
        case EXP_PEDAL_HW:
            while (count < chain->data_size)
            {
                uint8_t actuator_type = *pdata++;
                uint8_t actuator_id = *pdata++;

                uint8_t matches = 0;
                for (i = 0; i < actuators_amount; i++)
                {
                    control_t *control = g_control_chain[actuators_list[i]];
                    if (actuator_type == control->actuator_type && actuator_id == control->actuator_id)
                    {
                        matches++;
                        if (actuator_type == PEDAL)
                        {
                            float *pvalue;
                            pvalue = (float *) pdata;
                            value = *pvalue;
                            pdata += sizeof(float);

                            // rounds the step and checks if need update
                            int8_t step = ((value * control->steps) + 0.5);
                            if (step == control->step) continue;

                            // converts step to absolute value
                            control->step = step;
                            step_to_value(control);
                        }
                        else if (actuator_type == FOOT)
                        {
                            uint8_t value = *pdata++;
                            if (value == 0) continue;
                        }

                        control_set(control);
                    }
                }

                if (!matches) break;
                count = (pdata - chain->data);
            }
            break;
    }
}

static void connected_device_add(uint8_t hw_type, uint8_t hw_id)
{
    uint32_t i;

    // checks if this device is already on the list
    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        if (g_connected_devices[i].hardware_type == hw_type &&
            g_connected_devices[i].hardware_id == hw_id)
        {
            return;
        }
    }

    // searches a unused position in the list
    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        // adds the device on the first unused list position
        if (g_connected_devices[i].hardware_type == 0)
        {
            g_connected_devices[i].hardware_type = hw_type;
            g_connected_devices[i].hardware_id = hw_id;
            g_connected_devices[i].count = 0;

            // composes the command
            char buffer[64];
            i = copy_command(buffer, HW_CONNECTED_CMD);
            i += int_to_str(hw_type, &buffer[i], 8, 0);
            buffer[i++] = ' ';
            i += int_to_str(hw_id, &buffer[i], 8, 0);

            // sends the command to GUI
            comm_webgui_set_response_cb(NULL);
            comm_webgui_send(buffer, i);
            comm_webgui_wait_response();
        }
    }
}

static void connected_device_remove(device_t *device)
{
    char buffer[64];
    uint32_t i;

    // composes the command
    i = copy_command(buffer, HW_DISCONNECTED_CMD);
    i += int_to_str(device->hardware_type, &buffer[i], 8, 0);
    buffer[i++] = ' ';
    i += int_to_str(device->hardware_id, &buffer[i], 8, 0);

    // sends the command to GUI
    comm_webgui_set_response_cb(NULL);
    comm_webgui_send(buffer, i);
    comm_webgui_wait_response();

    // resets the device values
    device->hardware_type = 0;
    device->hardware_id = 0;
    device->count = 0;
}

static void send_request(device_t *device)
{
    uint8_t data[sizeof(control_chain_t)];
    control_chain_t *msg_to_send;
    msg_to_send = (control_chain_t *) data;

    // fills the message struct
    msg_to_send->start = CONTROL_CHAIN_PROTOCOL_START;
    msg_to_send->hw_type_origin = 0;
    msg_to_send->hw_id_origin = 0;
    msg_to_send->hw_type_destination = device->hardware_type;
    msg_to_send->hw_id_destination = device->hardware_id;
    msg_to_send->function = CONTROL_CHAIN_REQUEST_DATA;
    msg_to_send->data_size = 0;
    msg_to_send->data = NULL;
    msg_to_send->end = CONTROL_CHAIN_PROTOCOL_END;

    // message checksum
    uint8_t checksum;
    checksum_ok(msg_to_send, &checksum);
    msg_to_send->checksum = checksum;

    // sends the message
    comm_control_chain_send(data, CONTROL_CHAIN_MINIMAL_MSG_SIZE);
}

static void control_set_resp_cb(void *arg)
{
    arg = 0;
    g_waiting_response = 0;
}

static void control_set(control_t *control)
{

    switch (control->properties)
    {
        case CONTROL_PROP_TOGGLED:
        case CONTROL_PROP_BYPASS:
            if (control->value > 0) control->value = 0;
            else control->value = 1;
            break;

        case CONTROL_PROP_TRIGGER:
            control->value = 1;
            break;
    }

    char buffer[128];
    uint8_t i;

    i = copy_command(buffer, CONTROL_SET_CMD);

    // insert the instance on buffer
    i += int_to_str(control->effect_instance, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i++] = ' ';

    // insert the symbol on buffer
    strcpy(&buffer[i], control->symbol);
    i += strlen(control->symbol);
    buffer[i++] = ' ';

    // insert the value on buffer
    i += float_to_str(control->value, &buffer[i], sizeof(buffer) - i, 3);

    // sets the response callback
    g_waiting_response = 1;
    comm_webgui_set_response_cb(control_set_resp_cb);

    // send the data to GUI
    comm_webgui_send(buffer, i);
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void control_chain_init(void)
{
    uint32_t i;

    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        // initializes the control chain list
        g_control_chain[i] = NULL;

        // initialiazes the list of connected devices
        g_connected_devices[i].hardware_type = 0;
        g_connected_devices[i].hardware_id = 0;
        g_connected_devices[i].count = 0;
    }

    g_buffer_idx = 0;
    g_new_msg = 0;
}

void control_chain_add(control_t *control)
{
    uint32_t i;

    // checks if the device is connected
    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        if (g_connected_devices[i].hardware_type == control->hardware_type &&
            g_connected_devices[i].hardware_id == control->hardware_id)
        {
            break;
        }
    }

    // if cannot find the device delete the control
    if (i == CONTROL_CHAIN_MAX_DEVICES)
    {
        data_free_control(control);
        return;
    }

    // searches a empty position to add the control
    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        if (!g_control_chain[i])
        {
            // checks the type and sets the appropriated steps
            if (control->properties == CONTROL_PROP_INTEGER)
                control->steps = (control->maximum - control->minimum) + 1;

            g_control_chain[i] = control;
            break;
        }
    }
}

void control_chain_remove(int32_t effect_instance, const char *symbol)
{
    control_t *control;
    uint32_t i;
    uint8_t all_effects, all_controls;

    all_effects = (effect_instance == ALL_EFFECTS) ? 1 : 0;
    all_controls = (strcmp(symbol, ALL_CONTROLS) == 0) ? 1 : 0;

    // searches the control
    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        control = g_control_chain[i];
        if (control == NULL) continue;

        // checks the effect instance and symbol
        if (all_effects || control->effect_instance == effect_instance)
        {
            if (all_controls || strcmp(control->symbol, symbol) == 0)
            {
                data_free_control(control);
                g_control_chain[i] = NULL;
            }
        }
    }
}

void control_chain_append_data(uint8_t *data, uint32_t data_size)
{
    // if message don't fits in the buffer reset the index
    if ((g_buffer_idx + data_size) >= CONTROL_CHAIN_BUFFER_SIZE)
    {
        g_buffer_idx = 0;
        return;
    }

    // copies the chunk of data
    memcpy(&g_buffer[g_buffer_idx], data, data_size);
    g_buffer_idx += data_size;

    g_new_msg = 1;
}

void control_chain_process(void)
{
    static int32_t last_request = -1;
    uint32_t i;

    // search the next devices to do requests
    if (!g_waiting_response)
    {
        for (i = last_request + 1; i < CONTROL_CHAIN_MAX_DEVICES; i++)
        {
            if (g_connected_devices[i].hardware_type != 0)
            {
                send_request(&g_connected_devices[i]);
                last_request = i;
                break;
            }
        }
    }

    if (i == CONTROL_CHAIN_MAX_DEVICES) last_request = -1;

    // checks the timeout of all connected devices
    for (i = 0; i < CONTROL_CHAIN_MAX_DEVICES; i++)
    {
        // checks if the device is connected
        if (g_connected_devices[i].hardware_type != 0)
        {
            g_connected_devices[i].count++;

            // checks the timeout counter
            if (g_connected_devices[i].count >= CONTROL_CHAIN_TIMEOUT_COUNT)
            {
                connected_device_remove(&g_connected_devices[i]);
            }
        }
    }

    // if don't has new message returns
    if (!g_new_msg) return;

    control_chain_t *chain;

    // searches control chain messages in the buffer
    i = 0;
    while (i < g_buffer_idx)
    {
        if (g_buffer[i] == CONTROL_CHAIN_PROTOCOL_START)
        {
            chain = (control_chain_t *) g_buffer;
            uint8_t checksum = g_buffer[CONTROL_CHAIN_MINIMAL_MSG_SIZE + chain->data_size - 2];
            uint8_t end = g_buffer[CONTROL_CHAIN_MINIMAL_MSG_SIZE + chain->data_size - 1];

            if (end == CONTROL_CHAIN_PROTOCOL_END)
            {
                // copies the data
                if (chain->data_size > 0)
                    memcpy(g_data, &g_buffer[i + 7], chain->data_size);

                chain->data = g_data;
                chain->checksum = checksum;
                chain->end = end;
                parse_data(chain);
                i += CONTROL_CHAIN_MINIMAL_MSG_SIZE + chain->data_size;
            }
        }
        else i++;
    }

    // resets the variables
    g_buffer_idx = 0;
    g_new_msg = 0;
}
