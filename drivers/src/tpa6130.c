
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "tpa6130.h"
#include "utils.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define I2C_DELAY               1
#define I2C_WRITE_CMD           0
#define I2C_READ_CMD            1

// registers map
#define CONTROL                 0x01
#define VOLUME_AND_MUTE         0x02
#define OUTPUT_IMPEDANCE        0x03
#define I2C_ADDRESS_VERSION     0x04
// control register bits definitions
#define HP_EN_L                 0x80
#define HP_EN_R                 0x40
#define STEREO_HP               0x00
#define DUAL_MONO_HP            0x10
#define BRIDGE_TIED_LOAD        0x20
#define SW_SHUTDOWN             0x01
#define THERMAL                 0x02
// volume and mute bits definitions
#define MUTE_L                  0x80
#define MUTE_R                  0x40
// output impedance register
#define HIZ_L                   0x02
#define HIZ_R                   0x01

#define VOLUME_MIN_DIGITAL      1
#define VOLUME_MAX_DIGITAL      63


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/

#define SET_SDA()           {SET_PIN(TPA6130_SDA_PORT, TPA6130_SDA_PIN); delay_us(1);}
#define CLR_SDA()           {CLR_PIN(TPA6130_SDA_PORT, TPA6130_SDA_PIN); delay_us(1);}
#define SET_SCL()           {SET_PIN(TPA6130_SCL_PORT, TPA6130_SCL_PIN); delay_us(1);}
#define CLR_SCL()           {CLR_PIN(TPA6130_SCL_PORT, TPA6130_SCL_PIN); delay_us(1);}
#define READ_SDA()          READ_PIN(TPA6130_SDA_PORT, TPA6130_SDA_PIN)
#define CONFIG_SDA_IN()     CONFIG_PIN_INPUT(TPA6130_SDA_PORT, TPA6130_SDA_PIN); delay_us(1);
#define CONFIG_SDA_OUT()    CONFIG_PIN_OUTPUT(TPA6130_SDA_PORT, TPA6130_SDA_PIN); delay_us(1);


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static uint8_t g_volume;


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL FUNCTIONS
************************************************************************************************************************
*/

static void i2c_start(void)
{
    SET_SDA()
    SET_SCL()
    CLR_SDA()
    CLR_SCL()
}

static void i2c_stop(void)
{
    CLR_SCL()
    CLR_SDA()
    SET_SCL()
    SET_SDA()
}

static void i2c_write(uint8_t data)
{
    uint8_t i, ack;

    for (i = 0; i < 8; i++)
    {
        if (data & 0x80) SET_SDA()
        else CLR_SDA()

        data <<= 1;
        SET_SCL()
        CLR_SCL()
    }

    CONFIG_SDA_IN();

    SET_SCL()
    ack = READ_SDA();
    CLR_SCL()
    CONFIG_SDA_OUT();
}

static void i2c_write_register(uint8_t address, uint8_t data)
{
    i2c_start();
    i2c_write(TPA6130_DEVICE_ADDRESS | I2C_WRITE_CMD);
    i2c_write(address);
    i2c_write(data);
    i2c_stop();
    delay_ms(1);
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void tpa6130_init(void)
{
    CONFIG_PIN_OUTPUT(TPA6130_SCL_PORT, TPA6130_SCL_PIN);
    CONFIG_PIN_OUTPUT(TPA6130_SDA_PORT, TPA6130_SDA_PIN);

    // load the default registers values
    i2c_write_register(CONTROL, TPA6130_CONTROL_DEFAULT);
    i2c_write_register(VOLUME_AND_MUTE, TPA6130_VOLUME_AND_MUTE_DEFAULT);
    i2c_write_register(OUTPUT_IMPEDANCE, TPA6130_OUTPUT_IMPEDANCE_DEFAULT);
}

void tpa6130_set_volume(uint8_t value)
{
    // check the bounds
    if (value > VOLUME_MAX_DIGITAL) value = VOLUME_MAX_DIGITAL;
    else if (value <= VOLUME_MIN_DIGITAL) value = (MUTE_L | MUTE_R);

    i2c_write_register(VOLUME_AND_MUTE, value);
    g_volume = value;
}

void tpa6130_mute(uint8_t mute)
{
    if (mute)
        i2c_write_register(VOLUME_AND_MUTE, g_volume | (MUTE_L | MUTE_R));
    else
        i2c_write_register(VOLUME_AND_MUTE, g_volume);
}
