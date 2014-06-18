
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "chain.h"
#include "comm.h"
#include "utils.h"

#include "FreeRTOS.h"
#include "semphr.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {SYNC, DESTINATION, ORIGIN, FUNCTION, DATA_SIZE_LSB, DATA_SIZE_MSB, DATA, MSG_OK, CHECK, UNKNOWN = 0xFF};


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

static ringbuff_t *g_proxy_rb = 0;
static xSemaphoreHandle g_cc_msg_sem;


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

// copy an command to buffer
static uint8_t copy_command(char *buffer, const char *command)
{
    uint8_t i = 0;
    const char *cmd = command;

    while (*cmd && (*cmd != '%' && *cmd != '.'))
    {
        if (buffer) buffer[i] = *cmd;
        i++;
        cmd++;
    }

    return i;
}

static void encode(uint8_t byte)
{
    if (byte == CONTROL_CHAIN_ESCAPE_BYTE)
    {
        ringbuff_write(g_proxy_rb, &byte, 1);
        ringbuff_write(g_proxy_rb, &byte, 1);
    }
    else if (byte == CONTROL_CHAIN_END_BYTE)
    {
        byte = CONTROL_CHAIN_ESCAPE_BYTE;
        ringbuff_write(g_proxy_rb, &byte, 1);
        byte = ~CONTROL_CHAIN_END_BYTE;
        ringbuff_write(g_proxy_rb, &byte, 1);
    }
    else
    {
        ringbuff_write(g_proxy_rb, &byte, 1);
    }
}

static uint8_t decode(uint8_t byte, uint8_t *decoded)
{
    static uint8_t escape;

    (*decoded) = byte;

    if (escape)
    {
        if (byte == CONTROL_CHAIN_ESCAPE_BYTE)
            (*decoded) = CONTROL_CHAIN_ESCAPE_BYTE;
        else if (byte == (uint8_t)(~CONTROL_CHAIN_END_BYTE))
            (*decoded) = CONTROL_CHAIN_END_BYTE;

        escape = 0;
    }
    else if (byte == CONTROL_CHAIN_ESCAPE_BYTE)
    {
        escape = 1;
        return 0;
    }

    return 1;
}

static uint8_t chain_fake_fsm(uint8_t byte)
{
    static uint8_t state = UNKNOWN;
    static uint16_t data_size, data_received;
    uint16_t tmp;

    if (byte == CONTROL_CHAIN_SYNC_BYTE) state = SYNC;

    uint8_t _state = state;
    switch (state)
    {
        case SYNC:
            data_size = 0;
            data_received = 0;
            state++;
            break;

        case DESTINATION:
            state++;
            break;

        case ORIGIN:
            state++;
            break;

        case FUNCTION:
            state++;
            break;

        case DATA_SIZE_LSB:
            data_size = byte;
            state++;
            break;

        case DATA_SIZE_MSB:
            tmp = byte;
            tmp <<= 8;
            data_size |= tmp;
            state++;
            if (data_size == 0) state++;
            break;

        case DATA:
            data_received++;
            if (data_received == data_size) state = CHECK;
            break;

        case CHECK:
            state = UNKNOWN;
            return MSG_OK;
    }

    return _state;
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void chain_init(void)
{
    vSemaphoreCreateBinary(g_cc_msg_sem);
    g_proxy_rb = ringbuf_create(CONTROL_CHAIN_BUFFER_SIZE);
}

void chain_dev2ui_push(const uint8_t* data_chunk, uint32_t data_size)
{
    uint32_t i;

    if (!g_proxy_rb) return;

    for (i = 0; i < data_size; i++)
    {
        uint8_t state = chain_fake_fsm(data_chunk[i]);

        // check whether is sync state
        if (state == SYNC)
        {
            ringbuff_write(g_proxy_rb, (uint8_t*)CHAIN_CMD, copy_command(0, CHAIN_CMD));
        }

        // encode the byte if is in a valid state
        if (state != UNKNOWN)
            encode(data_chunk[i]);

        // check whether received a message
        if (state == MSG_OK)
        {
            uint8_t aux = CONTROL_CHAIN_END_BYTE;
            ringbuff_write(g_proxy_rb, &aux, 1);
            xSemaphoreGive(g_cc_msg_sem);
        }
    }
}

uint32_t chain_dev2ui_pop(uint8_t *buffer, uint32_t buffer_size)
{
    uint32_t read = 0;

    if (g_proxy_rb && xSemaphoreTake(g_cc_msg_sem, portMAX_DELAY) == pdTRUE)
    {
        read = ringbuff_read_until(g_proxy_rb, buffer, buffer_size, CONTROL_CHAIN_END_BYTE);
    }

    return read;
}

void chain_ui2dev(const char *buffer)
{
    const char *pbuffer = buffer;
    uint8_t byte;

    while (*pbuffer)
    {
        if (decode(*pbuffer, &byte))
            comm_control_chain_send(&byte, 1);
        
        pbuffer++;
    }
}
