
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "serial.h"
#include "device.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define UART0           ((LPC_UART_TypeDef *)LPC_UART0)
#define UART1           ((LPC_UART_TypeDef *)LPC_UART1)
#define UART2           ((LPC_UART_TypeDef *)LPC_UART2)
#define UART3           ((LPC_UART_TypeDef *)LPC_UART3)


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

#ifdef OUTPUT_ENABLE_ACTIVE_IN_HIGH
#define WRITE_MODE(s)   if (s->has_oe) {SET_PIN(s->oe_port, s->oe_pin); delay_us(OUTPUT_ENABLE_DELAY);}
#define READ_MODE(s)    if (s->has_oe) CLR_PIN(s->oe_port, s->oe_pin);
#elif  OUTPUT_ENABLE_ACTIVE_IN_LOW
#define WRITE_MODE(s)   if (s->has_oe) {CLR_PIN(s->oe_port, s->oe_pin); delay_us(OUTPUT_ENABLE_DELAY);}
#define READ_MODE(s)    if (s->has_oe) SET_PIN(s->oe_port, s->oe_pin);
#endif


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static serial_t *g_serial_instances[SERIAL_MAX_INSTANCES];


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

static void uart_receive(serial_t *serial)
{
    LPC_UART_TypeDef *uart = GET_UART(serial->uart_id);

    uint32_t count, written;
    uint8_t buffer[FIFO_TRIGGER];

    // reads from uart and puts on ring buffer
    // keeps one byte on FIFO to force CTI interrupt
    count = UART_Receive(uart, buffer, FIFO_TRIGGER-1, NONE_BLOCKING);

    // writes the data to ring buffer
    written = ringbuff_write(serial->rx_buffer, buffer, count);

    // checks if all data fits on ring buffer
    if (written != count)
    {
        // invokes the callback because the buffer is full
        if (serial->rx_callback) serial->rx_callback(serial);

        // writes the data left
        count = count - written;
        written = ringbuff_write(serial->rx_buffer, &buffer[written], count);
    }
}

static void uart_transmit(serial_t *serial)
{
    LPC_UART_TypeDef *uart = GET_UART(serial->uart_id);

    // Disable THRE interrupt
    UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

    // Wait for FIFO buffer empty, transfer UART_TX_FIFO_SIZE bytes of data
    // Wait until THR empty
    while (UART_CheckBusy(uart) == SET);

    // checks output enable pin
    WRITE_MODE(serial);

    uint32_t count;
    uint8_t buffer[UART_TX_FIFO_SIZE];

    count = ringbuff_read(serial->tx_buffer, buffer, UART_TX_FIFO_SIZE);
    if (count > 0) UART_Send(uart, buffer, count, NONE_BLOCKING);

    // Enable THRE interrupt if buffer is not empty
    if (!ringbuf_is_empty(serial->tx_buffer))
    {
        UART_IntConfig(uart, UART_INTCFG_THRE, ENABLE);
    }
    else
    {
        while (UART_CheckBusy(uart) == SET);
        READ_MODE(serial);
    }
}

static void uart_handler(serial_t *serial)
{
    uint32_t intsrc, tmp, status;

    if (!serial) return;

    LPC_UART_TypeDef *uart = GET_UART(serial->uart_id);

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

            serial_error(serial->uart_id, status);
            return;
        }
    }

    // Receive Data Available or Character time-out
    if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI))
    {
        serial->eof = 0;
        uart_receive(serial);

        // Character time-out
        if (tmp == UART_IIR_INTID_CTI)
        {
            serial->eof = 1;
            if (serial->rx_callback) serial->rx_callback(serial);
        }
    }

    // Transmit Holding Empty
    if (tmp == UART_IIR_INTID_THRE)
    {
        uart_transmit(serial);
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void serial_init(serial_t *serial)
{
    LPC_UART_TypeDef *uart;
    IRQn_Type irq;

    // pins setup
    PINSEL_SetPinFunc(serial->rx_port, serial->rx_pin, serial->rx_function);
    PINSEL_SetPinFunc(serial->tx_port, serial->tx_pin, serial->tx_function);

    switch (serial->uart_id)
    {
        case 0:
            uart = UART0;
            irq = UART0_IRQn;
            break;

        case 1:
            uart = UART1;
            irq = UART1_IRQn;
            break;

        case 2:
            uart = UART2;
            irq = UART2_IRQn;
            break;

        case 3:
            uart = UART3;
            irq = UART3_IRQn;
            break;
    }


    // UART Configuration structure variable
    UART_CFG_Type UARTConfigStruct;

    // Initialize UART Configuration parameter structure to default state:
    // Baudrate = 9600bps
    // 8 data bit
    // 1 Stop bit
    // None parity
    UART_ConfigStructInit(&UARTConfigStruct);
    UARTConfigStruct.Baud_rate = serial->baud_rate;

    // UART FIFO configuration Struct variable
    UART_FIFO_CFG_Type UARTFIFOConfigStruct;

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
    NVIC_SetPriority(irq, (serial->priority << 3));

    // Enable Interrupt for UART channel
    NVIC_EnableIRQ(irq);

    // creates the ring buffers
    serial->rx_buffer = ringbuf_create(serial->rx_buffer_size + 1);
    serial->tx_buffer = ringbuf_create(serial->tx_buffer_size + 1);

    // initializes the struct vars
    serial->rx_callback = 0;
    serial->eof = 0;

    // checks if output enable is used
    if (serial->has_oe) CONFIG_PIN_OUTPUT(serial->oe_port, serial->oe_pin);
    READ_MODE(serial);

    // stores a pointer to serial object
    g_serial_instances[serial->uart_id] = serial;
}

uint32_t serial_send(uint8_t uart_id, const uint8_t *data, uint32_t data_size)
{
    LPC_UART_TypeDef *uart = GET_UART(uart_id);
    serial_t *serial = g_serial_instances[uart_id];

    // Temporarily lock out UART transmit interrupts during this
    // read so the UART transmit interrupt won't cause problems
    // with the index values
    UART_IntConfig(uart, UART_INTCFG_THRE, DISABLE);

    uint32_t written, to_write, index;
    written = ringbuff_write(serial->tx_buffer, data, data_size);
    to_write = data_size - written;
    index = written;

    uart_transmit(serial);

    // waits until all data be sent
    while (to_write > 0)
    {
        if (!ringbuf_is_full(serial->tx_buffer))
        {
            written = ringbuff_write(serial->tx_buffer, &data[index], to_write);
            to_write -= written;
            index += written;
        }
    }

    return data_size;
}

uint32_t serial_read(uint8_t uart_id, uint8_t *data, uint32_t data_size)
{
    LPC_UART_TypeDef *uart = GET_UART(uart_id);
    serial_t *serial = g_serial_instances[uart_id];

    // Temporarily lock out UART receive interrupts during this
    // read so the UART receive interrupt won't cause problems
    // with the index values
    UART_IntConfig(uart, UART_INTCFG_RBR, DISABLE);

    uint32_t count;
    count = ringbuff_read(serial->rx_buffer, data, data_size);

    // Re-enable UART interrupts
    UART_IntConfig(uart, UART_INTCFG_RBR, ENABLE);

    return count;
}

void serial_set_callback(uint8_t uart_id, void (*receive_cb)(serial_t *serial))
{
    g_serial_instances[uart_id]->rx_callback = receive_cb;
}

void UART0_IRQHandler(void)
{
    uart_handler(g_serial_instances[0]);
}

void UART1_IRQHandler(void)
{
    uart_handler(g_serial_instances[1]);
}

void UART2_IRQHandler(void)
{
    uart_handler(g_serial_instances[2]);
}

void UART3_IRQHandler(void)
{
    uart_handler(g_serial_instances[3]);
}
