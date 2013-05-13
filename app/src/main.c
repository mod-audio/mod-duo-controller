
#include "LPC177x_8x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lpc177x_8x_gpio.h"

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)

#define PIN_LED1 20
#define PIN_LED2 24
#define PIN_LED3 28

void init_hardware()
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
}

void blink_led_task(void *pvParameters)
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

int main(void)
{
    /* initialize hardware */
    init_hardware();

    /* create task to blink led */
    xTaskCreate(blink_led_task, (signed char *)"Blink led", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* should never reach here! */
    for(;;);
}
