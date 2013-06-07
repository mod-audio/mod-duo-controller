
#ifndef CONFIG_H
#define CONFIG_H


////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO HARDWARE

//// Slots count
// One slot is a set of display, knob, footswitch and led
#define SLOTS_COUNT         4

//// CPU pins
// defines the port and of CPU power button
#define CPU_BUTTON_PORT     1
#define CPU_BUTTON_PIN      23

// defines the port and of CPU power status
#define CPU_STATUS_PORT     1
#define CPU_STATUS_PIN      24

//// Cooler pins
#define COOLER_PORT         1
#define COOLER_PIN          22

//// ARM reset pin
// this pin is used to block/unblock the ARM reset
// to block the pin must be set to 0
// to unblock the pin must be configured to input
#define ARM_RESET_PORT      2
#define ARM_RESET_PIN       24

//// LEDs configuration
// Amount of LEDS
#define LEDS_COUNT          SLOTS_COUNT

// LEDs ports and pins definitions
// format definition: {R_PORT, R_PIN, G_PORT, G_PIN, B_PORT, B_PIN}
#define LED0_PINS           {3, 30, 3, 31, 3, 29}
#define LED1_PINS           {3, 27, 3, 28, 3, 26}
#define LED2_PINS           {3, 24, 3, 25, 3, 23}
#define LED3_PINS           {3, 21, 3, 22, 3, 20}

//// GLCDs configurations
// Amount of displays
#define GLCD_COUNT          SLOTS_COUNT

//// Actuators configuration
// Actuators IDs
enum {ENCODER0, ENCODER1, ENCODER2, ENCODER3, FOOTSWITCH0, FOOTSWITCH1, FOOTSWITCH2, FOOTSWITCH3};

// Amount of footswitches
#define FOOTSWITCHES_COUNT  SLOTS_COUNT

// Footswitches ports and pins definitions
// button definition: {BUTTON_PORT, BUTTON_PIN}
#define FOOTSWITCH0_PINS    {2, 17}
#define FOOTSWITCH1_PINS    {2, 16}
#define FOOTSWITCH2_PINS    {2, 15}
#define FOOTSWITCH3_PINS    {2, 14}

// Amount of encoders
#define ENCODERS_COUNT      SLOTS_COUNT

// Encoders ports and pins definitions
// encoder definition: {ENC_BUTTON_PORT, ENC_BUTTON_PIN, ENC_CHA_PORT, ENC_CHA_PIN, ENC_CHB_PORT, ENC4_CH_PIN}
#define ENCODER0_PINS       {0, 8, 2, 28, 2, 3}
#define ENCODER1_PINS       {0, 4, 2, 25, 2, 0}
#define ENCODER2_PINS       {0, 5, 2, 26, 2, 1}
#define ENCODER3_PINS       {0, 7, 2, 27, 2, 2}


////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO FIRMWARE

//// Protocol commands configuration
//// Receive
#define SAY_CMD             "say %s ..."
// led <led_id> <red> <green> <blue>
#define LED_CMD             "led %i %i %i %i ..."
// control_add <effect_instance> <symbol> <label> <control_prop> <unit> <value> <max> <min> <hw_type> <hw_id> <actuator_type> <actuator_id> [scale_point_count] {[scale_point1_label] [scale_point1_value]}...
#define CONTROL_ADD_CMD     "control_add %i %s %s %i %s %f %f %f %i %i %i %i ..."
// control_rm <effect_instance> <symbol>
#define CONTROL_REMOVE_CMD  "control_rm %i %s"
#define CONTROL_GET_CMD
#define CONTROL_SET_CMD
#define BYPASS_ADD_CMD
#define BYPASS_REMOVE_CMD
#define BYPASS_SET_CMD
#define BYPASS_GET_CMD

//// Send
#define PEDALBOARD_LOAD_CMD
#define HARDWARE_CONNECTED_CMD
#define HARDWARE_DISCONNECTED_CMD

//// Tools configuration
// enumeration of tools (identification)
enum {TOOL_SYSTEM, TOOL_TUNER, TOOL_PEAKMETER, TOOL_NAVEG};

// setup of tools on displays
#define TOOL_DISPLAY0       TOOL_SYSTEM
#define TOOL_DISPLAY1       TOOL_TUNER
#define TOOL_DISPLAY2       TOOL_PEAKMETER
#define TOOL_DISPLAY3       TOOL_NAVEG

//// Control propertires definitions
#define CONTROL_PROP_LINEAR         0
#define CONTROL_PROP_LOGARITHMIC    1
#define CONTROL_PROP_ENUMERATION    2
#define CONTROL_PROP_TOGGLED        3
#define CONTROL_PROP_TRIGGER        4
#define CONTROL_PROP_TAP_TEMPO      5

//// Allocation and free macros
// these macros should be used in replacement to default
// malloc and free functions of stdlib.h
#include "FreeRTOS.h"
#define MALLOC(n)   pvPortMalloc(n)
#define FREE(n)     vPortFree(n)

#endif
