
#include "LPC177x_8x.h"
#include "FreeRTOS.h"
#include "task.h"

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)

#define PIN_LED1 24
#define PIN_LED2 29
#define PIN_LED3 22

void init_hardware()
{
    volatile unsigned long delay;

    /* set system tick for 1ms interrupt */
    SystemCoreClockUpdate();

    /* pins direction configuration */
    LPC_GPIO1->DIR |= (1 << 22);    // (out) cooler
    LPC_GPIO1->DIR |= (1 << 23);    // (out) power on pc
    LPC_GPIO2->DIR &= ~(1 << 24);   // (in) reset ARM

    LPC_GPIO1->SET = (1 << 22);

    LPC_GPIO1->CLR = (1 << 23);
    for (delay = 0; delay < 5000000; delay++);
    LPC_GPIO1->SET = (1 << 23);
    for (delay = 0; delay < 5000000; delay++);

    LPC_GPIO3->DIR |= (1 << PIN_LED1);    // (out) led1
    LPC_GPIO3->DIR |= (1 << PIN_LED2);    // (out) led2
    LPC_GPIO3->DIR |= (1 << PIN_LED3);    // (out) led3

    LPC_GPIO3->SET = (1<<PIN_LED1);
    LPC_GPIO3->SET = (1<<PIN_LED2);
}

void blink_led_task(void *pvParameters)
{
    UNUSED_PARAM(pvParameters);

    while(1)
    {
        LPC_GPIO3->SET = (1<<PIN_LED3);
        vTaskDelay(500/portTICK_RATE_MS);

        LPC_GPIO3->CLR = (1<<PIN_LED3);
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
