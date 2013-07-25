
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "serial.h"
#include "utils.h"

#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_pinsel.h"


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

#define FIFO_LENGTH     8


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

    uint32_t count;
    uint8_t buffer[FIFO_LENGTH];

    // reads from uart and puts on ring buffer
    // keeps one byte on FIFO to force CTI interrupt
    count = UART_Receive(uart, buffer, FIFO_LENGTH-1, NONE_BLOCKING);
    ringbuff_write(g_rx_buffer[port], buffer, count);
}

static void uart_transmit(uint8_t port)
{
    LPC_UART_TypeDef *uart = GET_UART(port);

    // Disable THRE interrupt
    UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

	// Wait for FIFO buffer empty, transfer UART_TX_FIFO_SIZE bytes
    // of data or break whenever ring buffers are empty
	// Wait until THR empty
    while (UART_CheckBusy(uart) == SET);

    uint32_t count;
    uint8_t buffer[FIFO_LENGTH];

    count = ringbuff_read(g_tx_buffer[port], buffer, FIFO_LENGTH);
    if (count > 0) UART_Send(uart, buffer, count, NONE_BLOCKING);

    // Enable THRE interrupt
    UART_IntConfig(uart, UART_INTCFG_THRE, ENABLE);
}

static void uart_handler(uint8_t port)
{
	uint32_t intsrc, tmp, tmp1;

    LPC_UART_TypeDef *uart = GET_UART(port);

	// Determine the interrupt source
	intsrc = UART_GetIntId(uart);
	tmp = intsrc & UART_IIR_INTID_MASK;

	// Receive Line Status
	if (tmp == UART_IIR_INTID_RLS)
    {
		// Check line status
		tmp1 = UART_GetLineStatus(uart);

		// Mask out the Receive Ready and Transmit Holding empty status
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE);

		// If any error exist, calls serial_error function
		if (tmp1)
        {
            serial_error(port, tmp1);
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
            PINSEL_ConfigPin(0, 2, 1);
            PINSEL_ConfigPin(0, 3, 1);
            uart = UART0;
            irq = UART0_IRQn;
            break;

        case 1:
            // Initialize UART1 pin connect
            // P0.15: U1_TXD
            // P0.16: U1_RXD
            PINSEL_ConfigPin(0, 15, 1);
            PINSEL_ConfigPin(0, 16, 1);
            uart = UART1;
            irq = UART1_IRQn;
            break;

        case 2:
            // Initialize UART2 pin connect
            // P0.10: U2_TXD
            // P0.11: U2_RXD
            PINSEL_ConfigPin(0, 10, 2);
            PINSEL_ConfigPin(0, 11, 2);
            uart = UART2;
            irq = UART2_IRQn;
            break;

        case 3:
            // Initialize UART3 pin connect
            // P0.25: U3_TXD
            // P0.26: U3_RXD
            PINSEL_ConfigPin(0, 25, 2);
            PINSEL_ConfigPin(0, 26, 2);
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
    #if FIFO_LENGTH == 1
    UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV0;
    #elif FIFO_LENGTH == 4
    UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV1;
    #elif FIFO_LENGTH == 8
    UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV2;
    #elif FIFO_LENGTH == 14
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
    g_rx_buffer[port] = ringbuf_create(SERIAL_RX_BUFFER_SIZE);
    g_tx_buffer[port] = ringbuf_create(SERIAL_TX_BUFFER_SIZE);
}

void serial_set_callback(uint8_t port, void (*receive_cb)(uint8_t _port))
{
    g_callback[port] = receive_cb;
}

uint32_t serial_send(uint8_t port, uint8_t *data, uint32_t data_size)
{
    uint32_t count;
    LPC_UART_TypeDef *uart = GET_UART(port);

	// Temporarily lock out UART transmit interrupts during this
    // read so the UART transmit interrupt won't cause problems
    // with the index values
    UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

    count = ringbuff_write(g_tx_buffer[port], data, data_size);

    // checks if need forces the first send
    if (ringbuff_size(g_tx_buffer[port]) == data_size)
    {
        uart_transmit(port);
    }

    // Enable THRE interrupt
    UART_IntConfig(uart, UART_INTCFG_THRE, ENABLE);

    return count;
}

uint32_t serial_read(uint8_t port, uint8_t *data, uint32_t data_size)
{
    LPC_UART_TypeDef *uart = GET_UART(port);

	// Temporarily lock out UART receive interrupts during this
	// read so the UART receive interrupt won't cause problems
	// with the index values
	UART_IntConfig(uart, UART_INTCFG_RBR, DISABLE);

    uint32_t count;
    count  = ringbuff_read(g_rx_buffer[port], data, data_size);

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
