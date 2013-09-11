
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef HARDWARE_H
#define HARDWARE_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>

#include "led.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

enum {BYPASS, PROCESS};
enum {BLOCK, UNBLOCK};
enum {RECEPTION, TRANSMISSION};
enum {CPU_TURN_OFF, CPU_TURN_ON, CPU_REBOOT};
enum {CPU_OFF, CPU_ON};


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

// does the hardware setup
void hardware_setup(void);
// defines the cooler duty cycle
void hardware_cooler(uint8_t duty_cycle);
// returns the led object relative to led id
led_t *hardware_leds(uint8_t led_id);
// returns the actuator object relative to actuator id
void *hardware_actuators(uint8_t actuator_id);
// returns the timestamp (a variable increment in each millisecond)
uint32_t hardware_timestamp(void);
// set the true bypass value
void hardware_set_true_bypass(uint8_t value);
// get the true bypass value
uint8_t hardware_get_true_bypass(void);
// updates the headphone gain
void hardware_headphone(void);
// unblock/block the arm microcontroller reset
void hardware_reset(uint8_t unblock);
// changes the 485 direction
void hardware_485_direction(uint8_t direction);
// turn on, turn off, reset the cpu
void hardware_cpu_power(uint8_t power);
// return the cpu status
uint8_t hardware_cpu_status(void);


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
