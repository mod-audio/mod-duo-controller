
/*
************************************************************************************************************************
*           driver for TPA6130A2
************************************************************************************************************************
*/

#ifndef TPA6130_H
#define TPA6130_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "lpc177x_8x_gpio.h"

/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

enum {UNMUTE, MUTE};


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

// I2C ports and pins definitions
#define TPA6130_SDA_PORT        HEADPHONE_SDA_PORT
#define TPA6130_SDA_PIN         HEADPHONE_SDA_PIN
#define TPA6130_SCL_PORT        HEADPHONE_SCL_PORT
#define TPA6130_SCL_PIN         HEADPHONE_SCL_PIN

// TPA6130 device address
#define TPA6130_DEVICE_ADDRESS  0xC0

// default register values
#define TPA6130_CONTROL_DEFAULT             0xC0
#define TPA6130_VOLUME_AND_MUTE_DEFAULT     0x0F
#define TPA6130_OUTPUT_IMPEDANCE_DEFAULT    0x00

// I/O macros configuration
#define CONFIG_PIN_OUTPUT(port, pin)    GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_OUTPUT)
#define CONFIG_PIN_INPUT(port, pin)     GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_INPUT)
#define SET_PIN(port, pin)              GPIO_SetValue((port), (1 << (pin)))
#define CLR_PIN(port, pin)              GPIO_ClearValue((port), (1 << (pin)))
#define READ_PIN(port, pin)             ((FIO_ReadValue(port) >> (pin)) & 1)


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

// initializes the device and set the default registers value
void tpa6130_init(void);
// set the volume, valid values are between 0 (min -59dB) and 63 (max 4dB)
// if value = 0 the channels will mute
void tpa6130_set_volume(uint8_t value);
// mute the channels without change the last volume setted
void tpa6130_mute(uint8_t mute);


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
