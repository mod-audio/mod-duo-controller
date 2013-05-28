
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
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


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

char** string_split(char *str, const char token)
{
    uint32_t count;
    char *pstr, **list = NULL;

    if (!str) return list;

    // count the tokens
    pstr = str;
    count = 1;
    while (*pstr)
    {
        if (*pstr == token) count++;
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
        if (*pstr == token)
        {
            *pstr = '\0';
            list[++count] = pstr + 1;
        }
        pstr++;
    }

    list[++count] = NULL;

    return list;
}


uint32_t array_length(char **str_array)
{
    uint32_t count = 0;

    while (str_array[count]) count++;
    return count;
}


uint32_t int_to_str(int32_t num, char *string, uint32_t string_size)
{
    char *pstr = string;
    uint8_t signal = 0;
    uint32_t str_len;

    if (!string) return 0;

    if (num == 0)
    {
        *pstr++ = '0';
        *pstr = 0;
        return 1;
    }

    if (num < 0)
    {
        num = -num;
        signal = 1;
        string_size--;
    }

    while (num)
    {
        *pstr++ = (num % 10) + '0';
        num /= 10;

        if (--string_size == 0) break;
    }

    if (string_size == 0)
    {
        *string = 0;
        return 0;
    }

    if (signal) *pstr++ = '-';
    *pstr = 0;

    str_len = (pstr - string);
    reverse(string, str_len);

    return str_len;
}

uint32_t float_to_str(float num, char *string, uint32_t string_size, uint8_t precision)
{
    double intp, fracp;

    if (!string) return 0;

    // splits integer and fractional parts
    fracp = modf(num, &intp);

    // convert the integer part to string
    uint32_t int_len;
    int_len = int_to_str((int32_t)intp, string, string_size);

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
    frac_len = int_to_str((int32_t)fracp, &string[int_len], string_size - int_len);

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
