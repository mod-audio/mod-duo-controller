
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

enum {MODUSB_ENTRY, REGULAR_ENTRY, RESTORE_ENTRY, PENDRIVE_ENTRY, STOP_TIMEOUT = 0xFF};
enum {BLUETOOTH_NAME, BLUETOOTH_ADDRESS};
enum {GRUB_STAGE, KERNEL_STAGE, LOGIN_STAGE, PASSWORD_STAGE, WAIT_PROMPT_STAGE, PROMPT_READY_STAGE};


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

// initializes all resources needed by CLI
void cli_init(void);
// returns the console response
const char* cli_get_response(void);
// process the data received
void cli_process(void);
// execute a single command
void cli_command(const char *command);
// request systemctl command
void cli_systemctl(const char *command, const char *service);
// request package version information
void cli_package_version(const char *package_name);
// request bluetooth information
void cli_bluetooth(uint8_t what_info);


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
