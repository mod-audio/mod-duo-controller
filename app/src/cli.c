
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "cli.h"
#include "config.h"
#include "serial.h"
#include "utils.h"
#include "hardware.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define NEW_LINE            "\r\n"

#define MOD_USER            "root"
#define MOD_PASSWORD        "mod"
#define ECHO_OFF_CMD        "stty -echo"
#define SYSTEMCTL_CMD       "systemctl "

#define PEEK_SIZE           3
#define LINE_BUFFER_SIZE    32
#define RESPONSE_TIMEOUT    (CLI_RESPONSE_TIMEOUT / portTICK_RATE_MS)


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static char *g_boot_steps[] = {
    "U-Boot",
    "Hit any key",
    "Starting kernel",
    "modduo login:",
    "Password:"
};

enum {UBOOT_STARTING, UBOOT_HITKEY, KERNEL_STARTING, LOGIN, PASSWORD, N_BOOT_STEPS};


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


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static char g_received[LINE_BUFFER_SIZE+1];
static char g_response[LINE_BUFFER_SIZE+1];
static uint8_t g_boot_step, g_boot_first_step_len, g_waiting_response;
static xSemaphoreHandle g_received_sem, g_response_sem;


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

// return zero if match
static uint8_t compare_peek(const char *str1, const char *str2)
{
    uint32_t i, sum = 0;

    for (i = 0; i < PEEK_SIZE; i++)
    {
        sum += *str1++ ^ *str2++;
    }

    return sum;
}

// this callback is called from a ISR
static void serial_cb(serial_t *serial)
{
    char peek[PEEK_SIZE];

    if (g_boot_step == 0)
    {
        // need to search for first boot message to sync data
        int32_t ahead = ringbuff_search(serial->rx_buffer, (uint8_t *) g_boot_steps[0], g_boot_first_step_len);
        if (ahead == -1)
        {
            // flush data if doesn't find first boot boot message
            ringbuff_flush(serial->rx_buffer);
        }
        else if (ahead > 0)
        {
            // remove bytes ahead of first boot message
            serial_read(CLI_SERIAL, NULL, ahead);
        }
    }

    if (g_boot_step < N_BOOT_STEPS)
    {
        // check if beginning of message doesn't match with wanted boot message
        ringbuff_peek(serial->rx_buffer, (uint8_t *) peek, PEEK_SIZE);
        if (compare_peek(peek, g_boot_steps[g_boot_step]) != 0)
        {
            ringbuff_flush(serial->rx_buffer);
            return;
        }
    }

    // try read data until find a new line
    uint32_t read;
    read = serial_read_until(CLI_SERIAL, (uint8_t *) g_received, LINE_BUFFER_SIZE, '\n');
    if (read == 0)
    {
        // if can't find new line force reading
        read = serial_read(CLI_SERIAL, (uint8_t *) g_received, LINE_BUFFER_SIZE);
    }

    // make message null termined
    g_received[read] = 0;

    // all remaining data on buffer are not useful
    ringbuff_flush(serial->rx_buffer);

    portBASE_TYPE xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(g_received_sem, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void cli_init(void)
{
    vSemaphoreCreateBinary(g_received_sem);
    vSemaphoreCreateBinary(g_response_sem);
    serial_set_callback(CLI_SERIAL, serial_cb);

    g_boot_first_step_len = strlen(g_boot_steps[0]);
}

const char* cli_get_response(void)
{
    if (xSemaphoreTake(g_response_sem, RESPONSE_TIMEOUT) == pdTRUE)
    {
        return g_response;
    }

    return NULL;
}

void cli_process(void)
{
    xSemaphoreTake(g_received_sem, portMAX_DELAY);

    // check if it's booting
    if (g_boot_step < N_BOOT_STEPS)
    {
        uint32_t len = strlen(g_boot_steps[g_boot_step]);
        if (strncmp(g_received, g_boot_steps[g_boot_step], len) == 0)
        {
            switch (g_boot_step)
            {
                case UBOOT_STARTING:
led_set_color(hardware_leds(0), RED);
                    break;

                case UBOOT_HITKEY:
led_set_color(hardware_leds(0), BLUE);
                    break;

                case KERNEL_STARTING:
led_set_color(hardware_leds(0), MAGENTA);
                    break;

                case LOGIN:
                    cli_command(MOD_USER);
led_set_color(hardware_leds(0), YELLOW);
                    break;

                case PASSWORD:
                    cli_command(MOD_PASSWORD);
led_set_color(hardware_leds(0), GREEN);
                    break;
            }

            g_boot_step++;
        }
    }

    // check if it's waiting command response
    else if (g_waiting_response)
    {
        strcpy(g_response, g_received);
        xSemaphoreGive(g_response_sem);
    }
}

void cli_command(const char *command)
{
    if (!command) return;

    // TODO: make sure HMI is logged into shell

    g_waiting_response = 1;
    serial_send(CLI_SERIAL, (uint8_t*) command, strlen(command));
    serial_send(CLI_SERIAL, (uint8_t*) NEW_LINE, sizeof(NEW_LINE) - 1);
}

void cli_systemctl(const char *parameters)
{
    if (!parameters) return;
}

void cli_package_version(const char *package_name)
{
    if (!package_name) return;
}

void cli_bluetooth(uint8_t what_info)
{
    // default response
    strcpy(g_response, "unknown");

    switch (what_info)
    {
        case BLUETOOTH_NAME:
            break;

        case BLUETOOTH_ADDRESS:
            break;
    }
}
