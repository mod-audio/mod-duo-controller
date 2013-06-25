
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include <string.h>
#include <math.h>

#include "utils.h"


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
    char *copy = MALLOC(strlen(str) + 1);
    strcpy(copy, str);
    return copy;
}

void delay_us(uint32_t time)
{
    volatile uint32_t delay = (time*50);
    while (delay--);
}

void delay_ms(uint32_t time)
{
    while (time--) delay_us(900);
}

float convert_to_ms(const char *unit_from, float value)
{
    char unit[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit_from[i]; i++)
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
    for (i = 0; unit_to[i]; i++)
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
