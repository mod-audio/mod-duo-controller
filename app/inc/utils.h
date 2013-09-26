
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef UTILS_H
#define UTILS_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "config.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

// uncomment the define below to enable the quotation marks evaluation on strarr_split parser
#define ENABLE_QUOTATION_MARKS


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct RINGBUFF_T {
    uint32_t head, tail;
    uint8_t *buffer;
    uint32_t size;
} ringbuff_t;


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           MACRO'S
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/

// splits the string in each whitespace occurrence and returns a array of strings NULL terminated
char** strarr_split(char *str);
// returns the string array length
uint32_t strarr_length(char **str_array);
// joins a string array in a single string
char* strarr_join(char** const str_array);

// converts integer to string and returns the string length
uint32_t int_to_str(int32_t num, char *string, uint32_t string_size, uint8_t zero_leading);
// converts float to string  and returns the string length
uint32_t float_to_str(float num, char *string, uint32_t string_size, uint8_t precision);

// duplicate a string (alternative to strdup)
char *str_duplicate(const char *str);

// delay functions
void delay_us(volatile uint32_t time);
void delay_ms(volatile uint32_t time);

// time convertion functions
// known units (not is case sensitive): bpm, hz, s, ms
float convert_to_ms(const char *unit_from, float value);
float convert_from_ms(const char *unit_to, float value);

// ring buffer functions
// ringbuf_create: allocates memory to ring buffer
ringbuff_t *ringbuf_create(uint32_t buffer_size);
// ringbuff_write: returns the number of bytes written
uint32_t ringbuff_write(ringbuff_t *rb, const uint8_t *data, uint32_t data_size);
// ringbuff_read: returns the number of bytes read
uint32_t ringbuff_read(ringbuff_t *rb, uint8_t *buffer, uint32_t buffer_size);
// ringbuff_read_until: read the ring buffer until find the token and returns the number of bytes read
uint32_t ringbuff_read_until(ringbuff_t *rb, uint8_t *buffer, uint32_t buffer_size, uint8_t token);
// ringbuff_size: returns the amount of unread bytes
uint32_t ringbuff_size(ringbuff_t *rb);
// ringbuff_is_full: returns non zero is buffer is full
uint32_t ringbuf_is_full(ringbuff_t *rb);
// ringbuff_is_empty: returns non zero is buffer is empty
uint32_t ringbuf_is_empty(ringbuff_t *rb);

// put "> " at begin of string
void select_item(char *item_str);
// remove "> " from begin of string
void deselect_item(char *item_str);


/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
