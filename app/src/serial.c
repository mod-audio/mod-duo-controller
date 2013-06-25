
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "serial.h"

#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_pinsel.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define UART_COUNT      2
#define UART0           ((LPC_UART_TypeDef *)LPC_UART0)
#define UART1           ((LPC_UART_TypeDef *)LPC_UART1)
#define UART2           ((LPC_UART_TypeDef *)LPC_UART2)

#define FIFO_LENGTH     8


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

const uint32_t BAUDRATE[] = {
    SERIAL0_BAUDRATE,
    SERIAL1_BAUDRATE
};

const uint32_t PRIORITIES[] = {
    SERIAL0_PRIORITY,
    SERIAL1_PRIORITY
};


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

typedef struct {
    volatile uint32_t tx_head;                      // UART Tx ring buffer head index
    volatile uint32_t tx_tail;                      // UART Tx ring buffer tail index
    volatile uint32_t rx_head;                      // UART Rx ring buffer head index
    volatile uint32_t rx_tail;                      // UART Rx ring buffer tail index
    volatile uint8_t  tx[SERIAL_TX_BUFFER_SIZE];    // UART Tx data ring buffer
    volatile uint8_t  rx[SERIAL_RX_BUFFER_SIZE];    // UART Rx data ring buffer
} UART_RING_BUFFER_T;


/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/

// Buf masks
#define BUF_RX_MASK                     (SERIAL_RX_BUFFER_SIZE-1)
#define BUF_TX_MASK                     (SERIAL_TX_BUFFER_SIZE-1)
// Check buf is full or not
#define BUF_IS_FULL(head,tail,mask)     ((tail & mask) == ((head+1) & mask))
// Check buf is empty
#define BUF_IS_EMPTY(head,tail,mask)    ((head & mask) == (tail & mask))
// Reset buf
#define BUF_RESET(bufidx)	            (bufidx = 0)
// Increment buf
#define BUF_INCR(bufidx,mask)	        (bufidx = (bufidx+1) & mask)
// Buf size
#define BUF_SIZE(head,tail,mask)	    ((head - tail) & mask)

// Get uart port
#define GET_UART(port)              (port == 0 ? UART0 : port == 1 ? UART1 : UART2)
#define GET_UART_IRQ(port)          (port == 0 ? UART0_IRQn : port == 1 ? UART1_IRQn : UART2_IRQn)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static UART_RING_BUFFER_T g_ringbuf[UART_COUNT];
static uint8_t g_transmit_status[UART_COUNT];
static void (*g_callback[UART_COUNT])(serial_msg_t *msg);


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
	uint8_t tmpc, i = 0;
	uint32_t len;

	while(1)
    {
		// Call UART read function in UART driver
		len = UART_Receive(GET_UART(port), &tmpc, 1, NONE_BLOCKING);

        // If data received
		if (len)
        {
			// Check if buffer is more space
            // If no more space, remaining character will be trimmed out
			if (!BUF_IS_FULL(g_ringbuf[port].rx_head, g_ringbuf[port].rx_tail, BUF_RX_MASK))
            {
				g_ringbuf[port].rx[g_ringbuf[port].rx_head] = tmpc;
				BUF_INCR(g_ringbuf[port].rx_head, BUF_RX_MASK);
			}
		}
		// no more data
		else break;

        // leaves one byte on FIFO to force CTI interrupt
        if (++i == (FIFO_LENGTH-1)) break;
	}
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

	while (!BUF_IS_EMPTY(g_ringbuf[port].tx_head, g_ringbuf[port].tx_tail, BUF_TX_MASK))
    {
        // Move a piece of data into the transmit FIFO
    	if (UART_Send(uart, (uint8_t *)&g_ringbuf[port].tx[g_ringbuf[port].tx_tail], 1, NONE_BLOCKING))
        {
            // Update transmit ring FIFO tail pointer
            BUF_INCR(g_ringbuf[port].tx_tail, BUF_TX_MASK);
    	}
        else break;
    }

    // If there is no more data to send, disable the transmit
    // interrupt - else enable it or keep it enabled
	if (BUF_IS_EMPTY(g_ringbuf[port].tx_head, g_ringbuf[port].tx_tail, BUF_TX_MASK))
    {
    	UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

        // Reset Tx Interrupt state
    	g_transmit_status[port] = RESET;
    }
    else
    {
      	// Set Tx Interrupt state
		g_transmit_status[port] = SET;
    	UART_IntConfig(uart, UART_INTCFG_THRE, ENABLE);
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void serial_init(void)
{
    // UART Configuration structure variable
	UART_CFG_Type UARTConfigStruct;

    // UART FIFO configuration Struct variable
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;

    LPC_UART_TypeDef *uart;

    uint8_t i;
    for (i = 0; i < UART_COUNT; i++)
    {
        // Do not enable transmit interrupt here, since it is handled by
        // UART_Send() function, just to reset Tx Interrupt state for the
        // first time
        g_transmit_status[i] = RESET;

        // Reset ring buf head and tail idx
        BUF_RESET(g_ringbuf[i].rx_head);
        BUF_RESET(g_ringbuf[i].rx_tail);
        BUF_RESET(g_ringbuf[i].tx_head);
        BUF_RESET(g_ringbuf[i].tx_tail);

        if (i == 0)
        {
            // Initialize UART0 pin connect
            // P0.2: U0_TXD
            // P0.3: U0_RXD
            PINSEL_ConfigPin(0, 2, 1);
            PINSEL_ConfigPin(0, 3, 1);
        }
        else if (i == 1)
        {
            // Initialize UART1 pin connect
            // P0.15: U1_TXD
            // P0.16: U1_RXD
            PINSEL_ConfigPin(0, 15, 1);
            PINSEL_ConfigPin(0, 16, 1);
        }

        uart = GET_UART(i);

        // Initialize UART Configuration parameter structure to default state:
        // Baudrate = 9600bps
        // 8 data bit
        // 1 Stop bit
        // None parity
        UART_ConfigStructInit(&UARTConfigStruct);
        UARTConfigStruct.Baud_rate = BAUDRATE[i];

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
        NVIC_SetPriority(GET_UART_IRQ(i), (PRIORITIES[i] << 3));

        // Enable Interrupt for UART channel
        NVIC_EnableIRQ(GET_UART_IRQ(i));
    }
}


void serial_set_callback(uint8_t port, void (*receive_cb)(serial_msg_t *msg))
{
    g_callback[port] = receive_cb;
}


uint32_t serial_send(uint8_t port, uint8_t *txbuf, uint32_t buflen)
{
    uint8_t *data = (uint8_t *) txbuf;
    uint32_t bytes = 0;
    LPC_UART_TypeDef *uart = GET_UART(port);

	// Temporarily lock out UART transmit interrupts during this
    // read so the UART transmit interrupt won't cause problems
    // with the index values
    UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

	// Loop until transmit run buffer is full or until n_bytes
	// expires
	while ((buflen > 0) && (!BUF_IS_FULL(g_ringbuf[port].tx_head, g_ringbuf[port].tx_tail, BUF_TX_MASK)))
	{
		// Write data from buffer into ring buffer
		g_ringbuf[port].tx[g_ringbuf[port].tx_head] = *data;
		data++;

		// Increment head pointer
		BUF_INCR(g_ringbuf[port].tx_head, BUF_TX_MASK);

		// Increment data count and decrement buffer size count
		bytes++;
		buflen--;
	}

    // Check if current Tx interrupt enable is reset,
    // that means the Tx interrupt must be re-enabled
    // due to call UART_IntTransmit() function to trigger
    // this interrupt type
	if (g_transmit_status[port] == RESET)
    {
		uart_transmit(port);
	}
    // Otherwise, re-enables Tx Interrupt
	else
    {
		UART_IntConfig(uart, UART_INTCFG_THRE, ENABLE);
	}

    return bytes;
}


uint32_t serial_read(uint8_t port, uint8_t *rxbuf, uint32_t buflen)
{
    uint8_t *data = (uint8_t *) rxbuf;
    uint32_t bytes = 0;
    LPC_UART_TypeDef *uart = GET_UART(port);

	// Temporarily lock out UART receive interrupts during this
	// read so the UART receive interrupt won't cause problems
	// with the index values
	UART_IntConfig(uart, UART_INTCFG_RBR, DISABLE);

	// Loop until receive buffer ring is empty or
    // until max_bytes expires
	while ((buflen > 0) && (!BUF_IS_EMPTY(g_ringbuf[port].rx_head, g_ringbuf[port].rx_tail, BUF_RX_MASK)))
	{
		// Read data from ring buffer into user buffer
		*data = g_ringbuf[port].rx[g_ringbuf[port].rx_tail];
		data++;

		// Update tail pointer
		BUF_INCR(g_ringbuf[port].rx_tail, BUF_RX_MASK);

		// Increment data count and decrement buffer size count
		bytes++;
		buflen--;
	}

	// Re-enable UART interrupts
	UART_IntConfig(uart, UART_INTCFG_RBR, ENABLE);

    return bytes;
}


void UART0_IRQHandler(void)
{
	uint32_t intsrc, tmp, tmp1;
    const uint8_t port = 0;
    serial_msg_t msg;

	// Determine the interrupt source
	intsrc = UART_GetIntId(UART0);
	tmp = intsrc & UART_IIR_INTID_MASK;

	// Receive Line Status
	if (tmp == UART_IIR_INTID_RLS)
    {
		// Check line status
		tmp1 = UART_GetLineStatus(UART0);

		// Mask out the Receive Ready and Transmit Holding empty status
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE);

		// If any error exist, loop forever
		if (tmp1) while (1);
	}

	// Receive Data Available or Character time-out
	if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI))
    {
        uart_receive(port);

        // Character time-out
        if (tmp == UART_IIR_INTID_CTI)
        {
            msg.port = port;
            msg.data_size = BUF_SIZE(g_ringbuf[port].rx_head, g_ringbuf[port].rx_tail, BUF_RX_MASK);
            g_callback[port](&msg);
        }
	}

	// Transmit Holding Empty
	if (tmp == UART_IIR_INTID_THRE)
    {
        uart_transmit(port);
	}
}


void UART1_IRQHandler(void)
{
	uint32_t intsrc, tmp, tmp1;
    const uint8_t port = 1;
    serial_msg_t msg;

	// Determine the interrupt source
	intsrc = UART_GetIntId(UART1);
	tmp = intsrc & UART_IIR_INTID_MASK;

	// Receive Line Status
	if (tmp == UART_IIR_INTID_RLS)
    {
		// Check line status
		tmp1 = UART_GetLineStatus(UART1);

		// Mask out the Receive Ready and Transmit Holding empty status
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE);

		// If any error exist, loop forever
		if (tmp1) while (1);
	}

	// Receive Data Available or Character time-out
	if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI))
    {
        uart_receive(port);

        // Character time-out
        if (tmp == UART_IIR_INTID_CTI)
        {
            msg.port = port;
            msg.data_size = BUF_SIZE(g_ringbuf[port].rx_head, g_ringbuf[port].rx_tail, BUF_RX_MASK);
            g_callback[port](&msg);
        }
	}

	// Transmit Holding Empty
	if (tmp == UART_IIR_INTID_THRE)
    {
        uart_transmit(port);
	}
}


void UART2_IRQHandler(void)
{
	uint32_t intsrc, tmp, tmp1;
    const uint8_t port = 2;
    serial_msg_t msg;

	// Determine the interrupt source
	intsrc = UART_GetIntId(UART2);
	tmp = intsrc & UART_IIR_INTID_MASK;

	// Receive Line Status
	if (tmp == UART_IIR_INTID_RLS)
    {
		// Check line status
		tmp1 = UART_GetLineStatus(UART2);

		// Mask out the Receive Ready and Transmit Holding empty status
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE);

		// If any error exist, loop forever
		if (tmp1) while (1);
	}

	// Receive Data Available or Character time-out
	if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI))
    {
        uart_receive(port);

        // Character time-out
        if (tmp == UART_IIR_INTID_CTI)
        {
            msg.port = port;
            msg.data_size = BUF_SIZE(g_ringbuf[port].rx_head, g_ringbuf[port].rx_tail, BUF_RX_MASK);
            g_callback[port](&msg);
        }
	}

	// Transmit Holding Empty
	if (tmp == UART_IIR_INTID_THRE)
    {
        uart_transmit(port);
	}
}

