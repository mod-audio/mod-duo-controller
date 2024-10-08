
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef COMM_H
#define COMM_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "utils.h"


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


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/


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

// initialize the communication
void comm_init(void);

//// webgui communication functions
// sends a message to webgui
void comm_webgui_send(const char *data, uint32_t data_size);
// read a message from webgui
ringbuff_t* comm_webgui_read(void);
// sets a function callback to webgui response
void comm_webgui_set_response_cb(void (*resp_cb)(void *data, menu_item_t *item), menu_item_t *item);
// invokes the response function callback
void comm_webgui_response_cb(void *data);
// blocks the execution until the webgui response be received
void comm_webgui_wait_response(void);
// clear the data in the buffer
void comm_webgui_clear(void);

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
