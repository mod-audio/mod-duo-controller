
#ifndef CONFIG_H
#define CONFIG_H


////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO HARDWARE

//// Hardwares types (device identification)
#define QUADRA_HW       0
#define EXP_PEDAL_HW    1
#define XY_TABLET_HW    2

//// Actuators types
#define NONE            0
#define FOOT            1
#define KNOB            2
#define PEDAL           3

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
#define SAY_CMD             "say %s ..."
// led <led_id> <red> <green> <blue>
#define LED_CMD             "led %i %i %i %i ..."
// control_add <effect_instance> <symbol> <label> <control_prop> <unit> <value> <max> <min>
//             <step> <hw_type> <hw_id> <actuator_type> <actuator_id> [scale_point_count] {[scale_point1_label] [scale_point1_value]}...
#define CONTROL_ADD_CMD     "control_add %i %s %s %i %s %f %f %f %i %i %i %i %i ..."
// control_rm <effect_instance> <symbol>
#define CONTROL_REMOVE_CMD  "control_rm %i %s"
// control_get <effect_instance> <symbol>
#define CONTROL_GET_CMD     "control_get %i %s"
// control_set <effect_instance> <symbol> <value>
#define CONTROL_SET_CMD     "control_set %i %s %f"
// bypass_add <effect_instance> <hw_type> <hw_id> <actuator_type> <actuator_id> <bypass_value> <bypass_label>
#define BYPASS_ADD_CMD      "bypass_add %i %i %i %i %i %i %s"
// bypass_rm <effect_instance>
#define BYPASS_REMOVE_CMD   "bypass_rm %i"
// bypass_set <effect_instance> <bypass_value>
#define BYPASS_SET_CMD      "bypass_set %i %i"
// bypass_get <effect_instance>
#define BYPASS_GET_CMD      "bypass_get %i"
#define PEDALBOARD_LOAD_CMD
#define HARDWARE_CONNECTED_CMD
#define HARDWARE_DISCONNECTED_CMD
// peakmeter <peakmeter_number> <peakmeter_value>
#define PEAKMETER_CMD       "peakmeter %i %f"
// tuner <frequency> <cents>
#define TUNER_CMD           "tuner %f %s %i"

//// Tools configuration
// tools identification (don't change)
#define TOOL_SYSTEM         0
#define TOOL_TUNER          1
#define TOOL_PEAKMETER      2
#define TOOL_NAVEG          3

// time in milliseconds to enter in tool mode (hold rotary encoder button)
#define TOOL_MODE_TIME      2000

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

//// Serial names definition
#define SERIAL_WEBGUI       0
#define SERIAL_LINUX        1

//// Foot functions leds colors
#define TOGGLED_COLOR       GREEN
#define TRIGGER_COLOR       GREEN
#define TAP_TEMPO_COLOR     GREEN
#define BYPASS_COLOR        RED

//// TAP TEMPO
// defines the time that the led will stay turned on (in milliseconds)
#define TAP_TEMPO_TIME_ON   20

//// Allocation and free macros
// these macros should be used in replacement to default
// malloc and free functions of stdlib.h
#include "FreeRTOS.h"
#define MALLOC(n)   pvPortMalloc(n)
#define FREE(n)     vPortFree(n)

#endif
