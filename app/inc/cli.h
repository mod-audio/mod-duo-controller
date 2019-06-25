
/*
************************************************************************************************************************
* CLI - Command Line Interface
************************************************************************************************************************
*/

#ifndef CLI_H
#define CLI_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

enum {CLI_DISCARD_RESPONSE, CLI_RETRIEVE_RESPONSE, CLI_CACHE_ONLY};
enum {BLUETOOTH_NAME, BLUETOOTH_ADDRESS};
enum {RESTORE_INIT, RESTORE_STATUS, RESTORE_CHECK_BOOT};
enum {NOT_LOGGED, LOGGED_ON_SYSTEM, LOGGED_ON_RESTORE};


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

#define LINE_BUFFER_SIZE    32

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

// initializes all resources needed by CLI
void cli_init(void);
// process received data
void cli_process(void);
// execute a single command and return its response if asked for (maximum LINE_BUFFER_SIZE+1 bytes)
const char* cli_command(const char *command, uint8_t response_action);
// execute systemctl command and return its status if any
const char* cli_systemctl(const char *command, const char *service);
// request package version information
void cli_package_version(const char *package_name);
// request bluetooth information
void cli_bluetooth(uint8_t what_info);
// set restore mode and return its status
uint8_t cli_restore(uint8_t status);


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
