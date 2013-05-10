
#include "LPC177x_8x.h"

#define PIN_LED1 24
#define PIN_LED2 29
#define PIN_LED3 22

int main (void)
{
    volatile unsigned long delay;

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

    while(1)
    {
        LPC_GPIO3->SET = (1 << PIN_LED1);
        LPC_GPIO3->CLR = (1 << PIN_LED2);
        for (delay = 0; delay < 5000000; delay++);

        LPC_GPIO3->CLR = (1 << PIN_LED1);
        LPC_GPIO3->SET = (1 << PIN_LED2);
        for (delay = 0; delay < 5000000; delay++);
    }
}
