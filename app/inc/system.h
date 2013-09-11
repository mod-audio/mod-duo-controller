
/*
************************************************************************************************************************
* This library defines the system related functions, inclusive the system menu callbacks
************************************************************************************************************************
*/

#ifndef SYSTEM_H
#define SYSTEM_H


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

// this function checks if the boot will proceed normally or will be an alternative boot
void system_check_boot(void);

// system menu callbacks
void system_true_bypass_cb(void *arg);
void system_reset_pedalboard_cb(void *arg);
void system_save_pedalboard_cb(void *arg);
void system_bluetooth_cb(void *arg);
void system_bluetooth_pair_cb(void *arg);
void system_jack_quality_cb(void *arg);
void system_jack_normal_cb(void *arg);
void system_jack_performance_cb(void *arg);
void system_cpu_cb(void *arg);
void system_services_cb(void *arg);
void system_versions_cb(void *arg);
void system_restore_cb(void *arg);


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
