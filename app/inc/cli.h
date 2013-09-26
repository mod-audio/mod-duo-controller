
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

#define CLI_LINE_BUFFER_SIZE        512
#define CLI_RESPONSE_BUFFER_SIZE    256


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
// appends data to line buffer
void cli_append_data(const char *data, uint32_t data_size);
// returns the console response
const char* cli_get_response(void);
// process the data received
void cli_process(void);
// selects an grub entry
void cli_grub_select(uint8_t entry);
// reboot the CPU
void cli_reboot_cpu(void);
// sets the jack buffer size
void cli_jack_set_bufsize(const char* bufsize);
// gets the jack buffer size
void cli_jack_get_bufsize(void);
// requests information from systemctl
void cli_systemctl(const char *parameters);
// requests the package version from pacman
void cli_package_version(const char *package_name);
// requests bluetooth information
void cli_bluetooth(uint8_t what_info);
// requests information to check if usb controller has recognized by system
void cli_check_controller(void);
// retuns the boot stage
uint8_t cli_boot_stage(void);


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
