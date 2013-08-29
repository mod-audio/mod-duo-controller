
#include "usb.h"
#include "usbhw.h"
#include "usbcfg.h"
#include "usbcore.h"
#include "cdc.h"
#include "cdcuser.h"

#include "utils.h"

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)

static ringbuff_t *g_rx_buffer, *g_tx_buffer;
static void (*g_cdc_callback)(uint32_t msg_size);

static uint8_t end_of_message(uint8_t *buffer, uint32_t buffer_size)
{
    uint32_t i;

    for (i = 0; i < buffer_size; i++)
    {
        if (buffer[i] == 0) return 1;
    }

    return 0;
}


/*----------------------------------------------------------------------------
  CDC SendEncapsulatedCommand Request Callback
  Called automatically on CDC SEND_ENCAPSULATED_COMMAND Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendEncapsulatedCommand(void)
{
    return (1);
}

/*----------------------------------------------------------------------------
  CDC GetEncapsulatedResponse Request Callback
  Called automatically on CDC Get_ENCAPSULATED_RESPONSE Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetEncapsulatedResponse(void)
{
    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SetCommFeature Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetCommFeature(unsigned short wFeatureSelector)
{
    UNUSED_PARAM(wFeatureSelector);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC GetCommFeature Request Callback
  Called automatically on CDC Get_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetCommFeature(unsigned short wFeatureSelector)
{
    UNUSED_PARAM(wFeatureSelector);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC ClearCommFeature Request Callback
  Called automatically on CDC CLEAR_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_ClearCommFeature(unsigned short wFeatureSelector)
{
    UNUSED_PARAM(wFeatureSelector);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SetLineCoding Request Callback
  Called automatically on CDC SET_LINE_CODING Request
  Parameters:   none                    (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetLineCoding(void)
{
    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC GetLineCoding Request Callback
  Called automatically on CDC GET_LINE_CODING Request
  Parameters:   None                         (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetLineCoding(void)
{
    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SetControlLineState Request Callback
  Called automatically on CDC SET_CONTROL_LINE_STATE Request
  Parameters:   ControlSignalBitmap
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetControlLineState(unsigned short wControlSignalBitmap)
{
    UNUSED_PARAM(wControlSignalBitmap);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SendBreak Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   0xFFFF  start of Break
                0x0000  stop  of Break
                0x####  Duration of Break
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendBreak(unsigned short wDurationOfBreak)
{
    UNUSED_PARAM(wDurationOfBreak);

    /* ... add code to handle request */
    return (1);
}

void CDC_Init(void)
{
    g_rx_buffer = ringbuf_create(CDC_RX_BUFFER_SIZE);
    g_tx_buffer = ringbuf_create(CDC_TX_BUFFER_SIZE);
}

void CDC_SetMessageCallback(void (*callback)(uint32_t msg_size))
{
    g_cdc_callback = callback;
}

uint32_t CDC_GetMessage(uint8_t *msg_buffer, uint32_t msg_size)
{
    return ringbuff_read(g_rx_buffer, msg_buffer, msg_size);
}

void CDC_Send(const uint8_t *data, uint32_t data_size)
{
    // block until all bytes be sent
    while (ringbuff_size(g_tx_buffer) > 0);

    // puts the data into ring buffer
    ringbuff_write(g_tx_buffer, data, data_size);

    // forces send
    CDC_BulkIn();
}

void CDC_BulkOut(void)
{
    uint8_t buffer[USB_CDC_BUFSIZE];
    uint32_t bytes_read;

    bytes_read = USB_ReadEP(CDC_DEP_OUT, buffer);

    if (bytes_read > 0)
    {
        ringbuff_write(g_rx_buffer, buffer, bytes_read);
    }

    if (end_of_message(buffer, bytes_read))
    {
        (g_cdc_callback)(ringbuff_size(g_rx_buffer));
    }
}

void CDC_BulkIn(void)
{
    uint8_t buffer[USB_CDC_BUFSIZE];
    uint32_t bytes_to_write;

    bytes_to_write = ringbuff_read(g_tx_buffer, buffer, USB_CDC_BUFSIZE);

    if (bytes_to_write > 0)
    {
        USB_WriteEP(CDC_DEP_IN, buffer, bytes_to_write);
    }
}
