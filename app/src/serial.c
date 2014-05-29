
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "serial.h"
#include "utils.h"
#include "config.h"
#include "hardware.h"

#include "device.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define UART_COUNT      4
#define UART0           ((LPC_UART_TypeDef *)LPC_UART0)
#define UART1           ((LPC_UART_TypeDef *)LPC_UART1)
#define UART2           ((LPC_UART_TypeDef *)LPC_UART2)
#define UART3           ((LPC_UART_TypeDef *)LPC_UART3)

#define FIFO_TRIGGER    8


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

#define GET_UART(port)      ((port) == 0 ? UART0 : \
                             (port) == 1 ? UART1 : \
                             (port) == 2 ? UART2 : \
                             (port) == 3 ? UART3 : 0)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static void (*g_callback[UART_COUNT])(uint8_t port);
static ringbuff_t *g_rx_buffer[UART_COUNT], *g_tx_buffer[UART_COUNT];


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

static void uart_receive(uint8_t port)
{
    LPC_UART_TypeDef *uart = GET_UART(port);

    uint32_t count, written;
    uint8_t buffer[FIFO_TRIGGER];

    // reads from uart and puts on ring buffer
    // keeps one byte on FIFO to force CTI interrupt
    count = UART_Receive(uart, buffer, FIFO_TRIGGER-1, NONE_BLOCKING);

    // writes the data to ring buffer
    written = ringbuff_write(g_rx_buffer[port], buffer, count);

    // checks if all data fits on ring buffer
    if (written != count)
    {
        // invokes the callback because the buffer is full
        g_callback[port](port);

        // writes the data left
        count = count - written;
        written = ringbuff_write(g_rx_buffer[port], &buffer[written], count);
    }
}

static void uart_transmit(uint8_t port)
{
    LPC_UART_TypeDef *uart = GET_UART(port);

    // Disable THRE interrupt
    UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

    // Wait for FIFO buffer empty, transfer UART_TX_FIFO_SIZE bytes of data
    // Wait until THR empty
    while (UART_CheckBusy(uart) == SET);

    // checks if is the control chain serial
    if (port == CONTROL_CHAIN_SERIAL)
        hardware_485_direction(TRANSMISSION);

    uint32_t count;
    uint8_t buffer[UART_TX_FIFO_SIZE];

    count = ringbuff_read(g_tx_buffer[port], buffer, UART_TX_FIFO_SIZE);
    if (count > 0) UART_Send(uart, buffer, count, NONE_BLOCKING);

    // Enable THRE interrupt if buffer is not empty
    if (!ringbuf_is_empty(g_tx_buffer[port]))
    {
        UART_IntConfig(uart, UART_INTCFG_THRE, ENABLE);
    }
    else
    {
        // checks if is the control chain serial
        if (port == CONTROL_CHAIN_SERIAL)
        {
            while (UART_CheckBusy(uart) == SET);
            hardware_485_direction(RECEPTION);
        }
    }
}

static void uart_handler(uint8_t port)
{
    uint32_t intsrc, tmp, status;

    LPC_UART_TypeDef *uart = GET_UART(port);

    // Determine the interrupt source
    intsrc = UART_GetIntId(uart);
    tmp = intsrc & UART_IIR_INTID_MASK;

    // Receive Line Status
    if (tmp == UART_IIR_INTID_RLS)
    {
        // Check line status
        status = UART_GetLineStatus(uart);

        // Mask out the Receive Ready and Transmit Holding empty status
        status &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE);

        // If any error exist, calls serial_error function
        if (status)
        {
            // if happen framing error flush out the FIFO
            if ((status & UART_LSR_FE) || (status & UART_LSR_RXFE))
            {
                uint8_t buffer[UART_TX_FIFO_SIZE];
                UART_Receive(uart, buffer, UART_TX_FIFO_SIZE, NONE_BLOCKING);
            }

            serial_error(port, status);
            return;
        }
    }

    // Receive Data Available or Character time-out
    if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI))
    {
        uart_receive(port);

        // Character time-out
        if (tmp == UART_IIR_INTID_CTI)
        {
            g_callback[port](port);
        }
    }

    // Transmit Holding Empty
    if (tmp == UART_IIR_INTID_THRE)
    {
        uart_transmit(port);
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void serial_init(uint8_t port, uint32_t baudrate, uint8_t priority)
{
    // UART Configuration structure variable
    UART_CFG_Type UARTConfigStruct;

    // UART FIFO configuration Struct variable
    UART_FIFO_CFG_Type UARTFIFOConfigStruct;

    LPC_UART_TypeDef *uart;
    IRQn_Type irq;

    switch (port)
    {
        case 0:
            // Initialize UART0 pin connect
            // P0.2: U0_TXD
            // P0.3: U0_RXD
            PINSEL_SetPinFunc(0, 2, 1);
            PINSEL_SetPinFunc(0, 3, 1);
            uart = UART0;
            irq = UART0_IRQn;
            break;

        case 1:
            // Initialize UART1 pin connect
            // P0.15: U1_TXD
            // P0.16: U1_RXD
            PINSEL_SetPinFunc(0, 15, 1);
            PINSEL_SetPinFunc(0, 16, 1);
            uart = UART1;
            irq = UART1_IRQn;
            break;

        case 2:
            // Initialize UART2 pin connect
            // P0.10: U2_TXD
            // P0.11: U2_RXD
            PINSEL_SetPinFunc(0, 10, 1);
            PINSEL_SetPinFunc(0, 11, 1);
            uart = UART2;
            irq = UART2_IRQn;
            break;

        case 3:
            // Initialize UART3 pin connect
            // P0.25: U3_TXD
            // P0.26: U3_RXD
            PINSEL_SetPinFunc(0, 25, 3);
            PINSEL_SetPinFunc(0, 26, 3);
            uart = UART3;
            irq = UART3_IRQn;
            break;
    }

    // Initialize UART Configuration parameter structure to default state:
    // Baudrate = 9600bps
    // 8 data bit
    // 1 Stop bit
    // None parity
    UART_ConfigStructInit(&UARTConfigStruct);
    UARTConfigStruct.Baud_rate = baudrate;

    // Initialize UART peripheral with given to corresponding parameter
    UART_Init(uart, &UARTConfigStruct);

    // Initialize FIFOConfigStruct to default state:
    // - FIFO_DMAMode = DISABLE
    // - FIFO_Level = UART_FIFO_TRGLEV0
    // - FIFO_ResetRxBuf = ENABLE
    // - FIFO_ResetTxBuf = ENABLE
    // - FIFO_State = ENABLE
    UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);

    // FIFO Trigger
    #if FIFO_TRIGGER == 1
    UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV0;
    #elif FIFO_TRIGGER == 4
    UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV1;
    #elif FIFO_TRIGGER == 8
    UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV2;
    #elif FIFO_TRIGGER == 14
    UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV3;
    #endif

    // Initialize FIFO for UART peripheral
    UART_FIFOConfig(uart, &UARTFIFOConfigStruct);

    // Enable UART Transmit
    UART_TxCmd(uart, ENABLE);

    // Enable UART Rx interrupt
    UART_IntConfig(uart, UART_INTCFG_RBR, ENABLE);

    // Enable UART line status interrupt
    UART_IntConfig(uart, UART_INTCFG_RLS, ENABLE);

    // set priority
    NVIC_SetPriority(irq, (priority << 3));

    // Enable Interrupt for UART channel
    NVIC_EnableIRQ(irq);

    // creates the ring buffers
    g_rx_buffer[port] = ringbuf_create(SERIAL_RX_BUFFER_SIZE+1);
    g_tx_buffer[port] = ringbuf_create(SERIAL_TX_BUFFER_SIZE+1);
}

void serial_set_callback(uint8_t port, void (*receive_cb)(uint8_t _port))
{
    g_callback[port] = receive_cb;
}

uint32_t serial_send(uint8_t port, const uint8_t *data, uint32_t data_size)
{
    LPC_UART_TypeDef *uart = GET_UART(port);

    // Temporarily lock out UART transmit interrupts during this
    // read so the UART transmit interrupt won't cause problems
    // with the index values
    UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

    uint32_t written, to_write, index;
    written = ringbuff_write(g_tx_buffer[port], data, data_size);
    to_write = data_size - written;
    index = written;

    uart_transmit(port);

    // waits until all data be sent
    while (to_write > 0)
    {
        if (!ringbuf_is_full(g_tx_buffer[port]))
        {
            written = ringbuff_write(g_tx_buffer[port], &data[index], to_write);
            to_write -= written;
            index += written;
        }
    }

    return data_size;
}

uint32_t serial_read(uint8_t port, uint8_t *data, uint32_t data_size)
{
    LPC_UART_TypeDef *uart = GET_UART(port);

    // Temporarily lock out UART receive interrupts during this
    // read so the UART receive interrupt won't cause problems
    // with the index values
    UART_IntConfig(uart, UART_INTCFG_RBR, DISABLE);

    uint32_t count;
    count = ringbuff_read(g_rx_buffer[port], data, data_size);

    // Re-enable UART interrupts
    UART_IntConfig(uart, UART_INTCFG_RBR, ENABLE);

    return count;
}

void UART0_IRQHandler(void)
{
    uart_handler(0);
}

void UART1_IRQHandler(void)
{
    uart_handler(1);
}

void UART2_IRQHandler(void)
{
    uart_handler(2);
}

void UART3_IRQHandler(void)
{
    uart_handler(3);
}
