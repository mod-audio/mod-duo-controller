
#ifndef CONFIG_H
#define CONFIG_H

////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO HARDWARE

//// Slots count
// One slot is a set of display, knob, footswitch and led
#define SLOTS_COUNT         4

//// LEDs configuration
// Amount of LEDS
#define LEDS_COUNT          SLOTS_COUNT
// LEDs ports and pins definitions
// format definition: ((const led_pins_t){RED_PORT, RED_PIN, GREEN_PORT, GREEN_PIN, BLUE_PORT, BLUE_PIN})
#define LED0_PINS           ((const led_pins_t){3, 30, 3, 31, 3, 29})
#define LED1_PINS           ((const led_pins_t){3, 27, 3, 28, 3, 26})
#define LED2_PINS           ((const led_pins_t){3, 24, 3, 25, 3, 23})
#define LED3_PINS           ((const led_pins_t){3, 21, 3, 22, 3, 20})

//// GLCDs configurations
// Amount of displays
#define GLCD_COUNT          SLOTS_COUNT

//// Footswitches configurations
// Amount of footswitches
#define FOOTSWITCHES_COUNT  SLOTS_COUNT


////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO FIRMWARE

//// Protocol commands configuration
// Receive
#define SAY_CMD         "say %s"
#define LED_CMD         "led %i %i %i %i"
#define MODE
#define PARAM_ADD
#define PARAM_REMOVE
#define PARAM_GET
#define BYPASS_ADD
#define BYPASS_REMOVE
#define BYPASS_SET
#define BYPASS_GET
// Send
#define PEDALBOARD_LOAD
#define HARDWARE_CONNECTED
#define HARDWARE_DISCONNECTED

//// Data structs definitions

#endif
