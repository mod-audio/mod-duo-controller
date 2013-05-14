
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "LPC177x_8x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lpc177x_8x_gpio.h"

#include "serial.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define PIN_LED1 20
#define PIN_LED2 24
#define PIN_LED3 28


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

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static void hardware_setup(void);
static void blink_led_task(void *pvParameters);


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


/*
************************************************************************************************************************
*           MAIN FUNCTION
************************************************************************************************************************
*/

int main(void)
{
    /* initialize hardware */
    hardware_setup();

    /* create task to blink led */
    xTaskCreate(blink_led_task, (signed char *)"Blink led", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* should never reach here! */
    for(;;);
}


/*
************************************************************************************************************************
*           LOCAL FUNCTIONS
************************************************************************************************************************
*/

static void serial_cb(void *arg)
{
    uint8_t buffer[32], *port = arg;
    uint16_t len;

    len = serial_read(*port, buffer, sizeof(buffer));
    serial_send(*port, buffer, len);
}

static void hardware_setup(void)
{
    volatile unsigned long delay;

    /* set system tick for 1ms interrupt */
    SystemCoreClockUpdate();

    /* pins direction configuration */
    GPIO_SetDir(1, (1 << 22), GPIO_DIRECTION_OUTPUT); // cooler
    GPIO_SetDir(1, (1 << 23), GPIO_DIRECTION_OUTPUT); // power on PC
    GPIO_SetDir(1, (1 << 24), GPIO_DIRECTION_INPUT); // reset ARM

    /* turn on cooler */
    GPIO_SetValue(1, (1 << 22));

    /* pulse power on PC */
    GPIO_ClearValue(1, (1 << 23));
    for (delay = 0; delay < 5000000; delay++);
    GPIO_SetValue(1, (1 << 23));
    for (delay = 0; delay < 5000000; delay++);

    /* leds pins direction configuration */
    GPIO_SetDir(3, (1 << PIN_LED1), GPIO_DIRECTION_OUTPUT); // led 1
    GPIO_SetDir(3, (1 << PIN_LED2), GPIO_DIRECTION_OUTPUT); // led 2
    LPC_GPIO3->DIR |= (1 << PIN_LED3);    // (out) led3

    /* leds initial state */
    GPIO_SetValue(3, (1 << PIN_LED1));
    GPIO_SetValue(3, (1 << PIN_LED2));
    LPC_GPIO3->SET = (1 << PIN_LED3);

    /* serial initialization */
    serial_init();
    serial_set_callback(SERIAL0, serial_cb);
    serial_set_callback(SERIAL1, serial_cb);
}

static void blink_led_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while(1)
    {
        GPIO_SetValue(3, (1 << PIN_LED1));
        GPIO_SetValue(3, (1 << PIN_LED2));
        LPC_GPIO3->CLR = (1 << PIN_LED3);
        vTaskDelay(500/portTICK_RATE_MS);

        GPIO_ClearValue(3, (1 << PIN_LED1));
        GPIO_ClearValue(3, (1 << PIN_LED2));
        LPC_GPIO3->SET = (1 << PIN_LED3);
        vTaskDelay(500/portTICK_RATE_MS);
    }
}

