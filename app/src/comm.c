
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <string.h>

#include "comm.h"
#include "config.h"
#include "serial.h"

#include "usb.h"
#include "usbhw.h"
#include "usbcore.h"
#include "cdcuser.h"

#include "FreeRTOS.h"
#include "semphr.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define WEBGUI_MAX_SEM_COUNT   5


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


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static void (*g_webgui_response_cb)(void *data) = NULL;
static uint8_t g_webgui_blocked;
static xSemaphoreHandle g_webgui_sem = NULL;
static ringbuff_t *g_webgui_rx_rb;


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

#if WEBGUI_COMM == SERIAL
static void webgui_rx_cb(serial_t *serial)
{
    uint8_t buffer[WEBGUI_SERIAL_RX_BUFF_SIZE];
    uint32_t size = serial_read(serial->uart_id, buffer, WEBGUI_SERIAL_RX_BUFF_SIZE);
#elif WEBGUI_COMM == USB_CDC
static void webgui_rx_cb(uint8_t *buffer, uint32_t size)
{
#endif

    if (size > 0)
    {
        ringbuff_write(g_webgui_rx_rb, buffer, size);

        // check end of message
        uint32_t i;
        for (i = 0; i < size; i++)
        {
            if (buffer[i] == 0)
            {
                portBASE_TYPE xHigherPriorityTaskWoken;
                xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(g_webgui_sem, &xHigherPriorityTaskWoken);
            }
        }
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void comm_init(void)
{
    g_webgui_sem = xSemaphoreCreateCounting(WEBGUI_MAX_SEM_COUNT, 0);
    g_webgui_rx_rb = ringbuf_create(WEBGUI_COMM_RX_BUFF_SIZE);

#if WEBGUI_COMM == USB_CDC
    // usb initialization
    USB_Init(1);
    USB_Connect(USB_CONNECT);
    while (!USB_Configuration);

    // cdc initialization
    CDC_Init(WEBGUI_COMM_TX_BUFF_SIZE);
    CDC_SetRxCallback(webgui_rx_cb);
#elif WEBGUI_COMM == SERIAL
    serial_set_callback(WEBGUI_SERIAL, webgui_rx_cb);
#endif
}

void comm_linux_send(const char *msg)
{
    serial_send(CLI_SERIAL, (uint8_t*)msg, strlen(msg));
}

void comm_webgui_send(const char *data, uint32_t data_size)
{
#if WEBGUI_COMM == USB_CDC
    CDC_Send((const uint8_t*)data, data_size+1);
#elif WEBGUI_COMM == SERIAL
    serial_send(WEBGUI_SERIAL, (const uint8_t*)data, data_size+1);
#endif
}

ringbuff_t* comm_webgui_read(void)
{
    if (xSemaphoreTake(g_webgui_sem, portMAX_DELAY) == pdTRUE)
    {
        return g_webgui_rx_rb;
    }

    return NULL;
}

void comm_webgui_set_response_cb(void (*resp_cb)(void *data))
{
    g_webgui_response_cb = resp_cb;
}

void comm_webgui_response_cb(void *data)
{
    if (g_webgui_response_cb)
    {
        g_webgui_response_cb(data);
        g_webgui_response_cb = NULL;
    }

    g_webgui_blocked = 0;
}

void comm_webgui_wait_response(void)
{
    g_webgui_blocked = 1;
    while (g_webgui_blocked);
}

void comm_control_chain_send(const uint8_t *data, uint32_t data_size)
{
    serial_send(CONTROL_CHAIN_SERIAL, data, data_size);
}
