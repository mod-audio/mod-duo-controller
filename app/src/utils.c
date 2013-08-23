
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "FreeRTOS.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/


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

#define BUFFER_IS_FULL(rb)      (rb->tail == (rb->head + 1) % rb->size)
#define BUFFER_IS_EMPTY(rb)     (rb->head == rb->tail)
#define BUFFER_INC(rb,idx)      (rb->idx = (rb->idx + 1) % rb->size)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/


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

static char* reverse(char* str, uint32_t str_len)
{
    char *end = str + (str_len - 1);
    char *start = str, tmp;

    while (start < end)
    {
        tmp = *end;
        *end = *start;
        *start = tmp;

        start++;
        end--;
    }

    return str;
}

static void parse_quote(char *str)
{
    char *pquote, *pstr = str;

    while (*pstr)
    {
        if (*pstr == '"')
        {
            // shift the string to left
            pquote = pstr;
            while (*pquote)
            {
                *pquote = *(pquote+1);
                pquote++;
            }
        }
        pstr++;
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

char** strarr_split(char *str)
{
    uint32_t count;
    char *pstr, **list = NULL;
    const char token = ' ';
    uint8_t quote = 0;

    if (!str) return list;

    // count the tokens
    pstr = str;
    count = 1;
    while (*pstr)
    {
        if (*pstr == token && quote == 0)
        {
            count++;
        }
#ifdef ENABLE_QUOTATION_MARKS
        if (*pstr == '"')
        {
            if (quote == 0) quote = 1;
            else
            {
                if (*(pstr+1) == '"') pstr++;
                else quote = 0;
            }
        }
#endif
        pstr++;
    }

    // allocates memory to list
    list = MALLOC((count + 1) * sizeof(char *));
    if (list == NULL) return NULL;

    // fill the list pointers
    pstr = str;
    list[0] = pstr;
    count = 0;
    while (*pstr)
    {
        if (*pstr == token && quote == 0)
        {
            *pstr = '\0';
            list[++count] = pstr + 1;
        }
#ifdef ENABLE_QUOTATION_MARKS
        if (*pstr == '"')
        {
            if (quote == 0) quote = 1;
            else
            {
                if (*(pstr+1) == '"') pstr++;
                else quote = 0;
            }
        }
#endif
        pstr++;
    }

    list[++count] = NULL;

#ifdef ENABLE_QUOTATION_MARKS
    count = 0;
    while (list[count]) parse_quote(list[count++]);
#endif

    return list;
}


uint32_t strarr_length(char** const str_array)
{
    uint32_t count = 0;

    if (str_array) while (str_array[count]) count++;
    return count;
}


char* strarr_join(char **str_array)
{
    uint32_t i, len = strarr_length(str_array);

    if (!str_array) return NULL;

    for (i = 1; i < len; i++)
    {
        (*(str_array[i] - 1)) = ' ';
    }

    return (*str_array);
}


uint32_t int_to_str(int32_t num, char *string, uint32_t string_size, uint8_t zero_leading)
{
    char *pstr = string;
    uint8_t signal = 0;
    uint32_t str_len;

    if (!string) return 0;

    // exception case: number is zero
    if (num == 0)
    {
        *pstr++ = '0';
        if (zero_leading) zero_leading--;
    }

    // need minus signal?
    if (num < 0)
    {
        num = -num;
        signal = 1;
        string_size--;
    }

    // composes the string
    while (num)
    {
        *pstr++ = (num % 10) + '0';
        num /= 10;

        if (--string_size == 0) break;
        if (zero_leading) zero_leading--;
    }

    // checks buffer size
    if (string_size == 0)
    {
        *string = 0;
        return 0;
    }

    // fills the zeros leading
    while (zero_leading--) *pstr++ = '0';

    // put the minus if necessary
    if (signal) *pstr++ = '-';
    *pstr = 0;

    // invert the string characters
    str_len = (pstr - string);
    reverse(string, str_len);

    return str_len;
}

uint32_t float_to_str(float num, char *string, uint32_t string_size, uint8_t precision)
{
    double intp, fracp;

    if (!string) return 0;

    // TODO: check Nan and Inf

    // splits integer and fractional parts
    fracp = modf(num, &intp);

    // convert the integer part to string
    uint32_t int_len;
    int_len = int_to_str((int32_t)intp, string, string_size, 0);

    // checks if convertion fail
    if (int_len == 0)
    {
        *string = 0;
        return 0;
    }

    // convert to absolute value
    if (fracp < 0.0) fracp = -fracp;

    // adds one to avoid lost the leading zeros
    fracp += 1.0;

    // calculates the precision
    while (precision--)
    {
        fracp *= 10;
    }

    // convert the fractional part
    uint32_t frac_len;
    frac_len = int_to_str((int32_t)fracp, &string[int_len], string_size - int_len, 0);

    // checks if convertion fail
    if (frac_len == 0)
    {
        *string = 0;
        return 0;
    }

    // inserts the dot covering the extra one added
    string[int_len] = '.';

    return (int_len + frac_len);
}

char *str_duplicate(const char *str)
{
    if (!str) return NULL;

    char *copy = MALLOC(strlen(str) + 1);
    strcpy(copy, str);
    return copy;
}

void delay_us(volatile uint32_t time)
{
    register uint32_t _time asm ("r0");
    _time = time;

    __asm__ volatile
    (
        "1:\n\t"
            "mov r1, #0x0015\n\t"
            "2:\n\t"
                "sub r1, r1, #1\n\t"
                "cbz r1, 3f\n\t"
                "b 2b\n\t"
            "3:\n\t"
                "sub r0, r0, #1\n\t"
                "cbz r0, 4f\n\t"
                "b 1b\n\t"
            "4:\n\t"
    );
}

void delay_ms(volatile uint32_t time)
{
    register uint32_t _time asm ("r0");
    _time = time;

    __asm__ volatile
    (
        "1:\n\t"
            "mov r1, #0x5600\n\t"
            "2:\n\t"
                "sub r1, r1, #1\n\t"
                "cbz r1, 3f\n\t"
                "b 2b\n\t"
            "3:\n\t"
                "sub r0, r0, #1\n\t"
                "cbz r0, 4f\n\t"
                "b 1b\n\t"
            "4:\n\t"
    );
}

float convert_to_ms(const char *unit_from, float value)
{
    char unit[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit_from[i] && i < (sizeof(unit)-1); i++)
    {
        if (i == (sizeof(unit) - 1)) break;
        unit[i] = unit_from[i] | 0x20;
    }
    unit[i] = 0;

    if (strcmp(unit, "bpm") == 0)
    {
        return (60000.0 / value);
    }
    else if (strcmp(unit, "hz") == 0)
    {
        return (1000.0 / value);
    }
    else if (strcmp(unit, "s") == 0)
    {
        return (value * 1000.0);
    }
    else if (strcmp(unit, "ms") == 0)
    {
        return value;
    }

    return 0.0;
}

float convert_from_ms(const char *unit_to, float value)
{
    char unit[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit_to[i] && i < (sizeof(unit)-1); i++)
    {
        if (i == (sizeof(unit) - 1)) break;
        unit[i] = unit_to[i] | 0x20;
    }
    unit[i] = 0;

    if (strcmp(unit, "bpm") == 0)
    {
        return (60000.0 / value);
    }
    else if (strcmp(unit, "hz") == 0)
    {
        return (1000.0 / value);
    }
    else if (strcmp(unit, "s") == 0)
    {
        return (value / 1000.0);
    }
    else if (strcmp(unit, "ms") == 0)
    {
        return value;
    }

    return 0.0;
}

ringbuff_t *ringbuf_create(uint32_t buffer_size)
{
    ringbuff_t *rb = (ringbuff_t *) MALLOC(sizeof(ringbuff_t));
    rb->head = 0;
    rb->tail = 0;
    rb->size = buffer_size;
    rb->buffer = (uint8_t *) MALLOC(buffer_size);
    return rb;
}

uint32_t ringbuff_write(ringbuff_t *rb, const uint8_t *data, uint32_t data_size)
{
    uint32_t bytes = 0;
    const uint8_t *pdata = data;

    while (data_size > 0 && !BUFFER_IS_FULL(rb))
    {
        rb->buffer[rb->head] = *pdata++;
        BUFFER_INC(rb, head);

        data_size--;
        bytes++;
    }

    return bytes;
}

uint32_t ringbuff_read(ringbuff_t *rb, uint8_t *buffer, uint32_t buffer_size)
{
    uint32_t bytes = 0;
    uint8_t *data = buffer;

    while (buffer_size > 0 && !BUFFER_IS_EMPTY(rb))
    {
        *data++ = rb->buffer[rb->tail];
        BUFFER_INC(rb, tail);

        buffer_size--;
        bytes++;
    }

    return bytes;
}

uint32_t ringbuff_size(ringbuff_t *rb)
{
    return ((rb->head - rb->tail) % rb->size);
}
