
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
#define DISABLE_ECHO        "stty -echo"
#define SET_SP1_VAR         "export PS1=\"\""

#define PEEK_SIZE           3
#define LINE_BUFFER_SIZE    32
#define RESPONSE_TIMEOUT    (CLI_RESPONSE_TIMEOUT / portTICK_RATE_MS)
#define BOOT_TIMEOUT        (1000 / portTICK_RATE_MS)


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
    "Password:",
    NULL
};

enum {UBOOT_STARTING, UBOOT_HITKEY, KERNEL_STARTING, LOGIN, PASSWORD, SHELL_CONFIG, N_BOOT_STEPS};


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
static uint8_t g_boot_step, g_waiting_response, g_restore;
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

// this callback is called from a ISR
static void serial_cb(serial_t *serial)
{
    portBASE_TYPE xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    if (g_boot_step < N_BOOT_STEPS)
    {
        uint32_t len = strlen(g_boot_steps[g_boot_step]);

        // search for boot message
        int32_t found = ringbuff_search(serial->rx_buffer, (uint8_t *) g_boot_steps[g_boot_step], len);

        // doesn't need to retrieve data, so flush them out
        ringbuff_flush(serial->rx_buffer);

        // wake task if boot message match or not required (NULL)
        if (!g_boot_steps[g_boot_step] || found >= 0)
        {
            xSemaphoreGiveFromISR(g_received_sem, &xHigherPriorityTaskWoken);
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
        }
    }
    else if (g_waiting_response)
    {
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

        // remove new line from response
        if (g_received[read-2] == '\r')
            g_received[read-2] = 0;

        // all remaining data on buffer are not useful
        ringbuff_flush(serial->rx_buffer);

        xSemaphoreGiveFromISR(g_received_sem, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
        // check if received login message when it's not waiting for response neither in booting process
        uint32_t len = strlen(g_boot_steps[LOGIN]);
        int32_t found = ringbuff_search(serial->rx_buffer, (uint8_t *) g_boot_steps[LOGIN], len);
        if (found >= 0)
        {
            g_boot_step = LOGIN;
            xSemaphoreGiveFromISR(g_received_sem, &xHigherPriorityTaskWoken);
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
        }

        ringbuff_flush(serial->rx_buffer);
    }
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

    // vSemaphoreCreateBinary is created as available which makes
    // first xSemaphoreTake pass even if semaphore has not been given
    // http://sourceforge.net/p/freertos/discussion/382005/thread/04bfabb9
    xSemaphoreTake(g_received_sem, 0);
    xSemaphoreTake(g_response_sem, 0);
}

void cli_process(void)
{
    static portTickType xTicksToWait = BOOT_TIMEOUT;
    portBASE_TYPE xReturn;

    xReturn = xSemaphoreTake(g_received_sem, xTicksToWait);

    // check if it's booting
    if (g_boot_step < N_BOOT_STEPS)
    {
        // if got timeout ...
        if (xReturn == pdFALSE)
        {
            // and still in the first step ...
            if (g_boot_step == 0)
            {
                // force login step
                g_boot_step = LOGIN;
            }

            // and already tried login ...
            else if (g_boot_step == LOGIN)
            {
                // assume boot is done
                g_boot_step = N_BOOT_STEPS;
            }

            // send new line to force interrupt
            cli_command(NULL, CLI_DISCARD_RESPONSE);
            return;
        }

        switch (g_boot_step)
        {
            case UBOOT_STARTING:
                xTicksToWait = portMAX_DELAY;
                break;

            case UBOOT_HITKEY:
                if (g_restore)
                {
                    // stop auto boot and run restore
                    cli_command(NULL, CLI_RETRIEVE_RESPONSE);
                    cli_command("run boot_restore", CLI_RETRIEVE_RESPONSE);
                    g_restore = 0;
                }
                break;

            case LOGIN:
                cli_command(MOD_USER, CLI_DISCARD_RESPONSE);
                break;

            case PASSWORD:
                cli_command(MOD_PASSWORD, CLI_DISCARD_RESPONSE);
                break;

            case SHELL_CONFIG:
                xTicksToWait = portMAX_DELAY;
                cli_command(DISABLE_ECHO, CLI_DISCARD_RESPONSE);
                cli_command(SET_SP1_VAR, CLI_DISCARD_RESPONSE);
                break;
        }

        g_boot_step++;
    }

    // check if it's waiting command response
    else if (g_waiting_response)
    {
        g_waiting_response = 0;
        strcpy(g_response, g_received);
        xSemaphoreGive(g_response_sem);
    }
}

const char* cli_command(const char *command, uint8_t response_action)
{
    g_waiting_response = response_action;

    // default response
    g_response[0] = 0;

    // send command
    if (command)
    {
        serial_send(CLI_SERIAL, (uint8_t *) command, strlen(command));

        // mutes command outputs if not waiting for response
        if (response_action == CLI_DISCARD_RESPONSE)
            serial_send(CLI_SERIAL, (uint8_t *) " &> /dev/null", 13);
    }
    serial_send(CLI_SERIAL, (uint8_t *) NEW_LINE, 2);

    // take semaphore to wait for response
    if (response_action == CLI_RETRIEVE_RESPONSE)
    {
        if (xSemaphoreTake(g_response_sem, RESPONSE_TIMEOUT) == pdTRUE)
            return g_response;
    }

    return NULL;
}

const char* cli_systemctl(const char *command, const char *service)
{
    if (!command || !service) return NULL;

    // build command
    serial_send(CLI_SERIAL, (uint8_t *) "systemctl ", 10);
    serial_send(CLI_SERIAL, (uint8_t *) command, strlen(command));
    serial_send(CLI_SERIAL, (uint8_t *) service, strlen(service));

    const char *response = cli_command(NULL, CLI_RETRIEVE_RESPONSE);
    // default response
    if (!response) strcpy(g_response, "unknown");

    return g_response;
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

void cli_restore(void)
{
    g_boot_step = 0;
    g_restore = 1;
    cli_command("reboot", CLI_DISCARD_RESPONSE);
}
