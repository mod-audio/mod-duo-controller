
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "ntc.h"
#include "device.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define TABLE_SIZE          (sizeof(g_table) / sizeof(uint16_t))
#define TABLE_MIN_TEMP      10
#define TABLE_MAX_TEMP      100

#ifndef ADC_CLOCK
#define ADC_CLOCK           100000
#endif


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

// table of ADC values to range 10°C ~ 100°C, step 1°C
const uint16_t g_table[] = {
    590, 604, 619, 634, 649, 665, 681, 698, 715, 732, 750, 768, 787, 806, 825, 845, 866, 887, 908, 930, 953, 975, 999,
    1023, 1047, 1072, 1097, 1123, 1150, 1177, 1204, 1232, 1261, 1290, 1319, 1349, 1380, 1411, 1442, 1474, 1507, 1540,
    1573, 1607, 1642, 1676, 1712, 1747, 1783, 1820, 1856, 1893, 1931, 1968, 2006, 2044, 2083, 2121, 2160, 2199, 2238,
    2277, 2316, 2355, 2394, 2434, 2473, 2512, 2550, 2589, 2627, 2666, 2704, 2741, 2779, 2816, 2853, 2889, 2925, 2960,
    2996, 3030, 3064, 3098, 3131, 3163, 3195, 3226, 3257, 3287, 3316
};


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

#define ABS(a)      ((a) > 0 ? (a) : -(a))


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

static uint16_t bisect(uint16_t target, const uint16_t *list, uint16_t list_size, uint16_t tolerance)
{
    uint16_t n, tries, delta;
    uint16_t best_n, inf_n, sup_n;

    // initialize the variables
    n = list_size / 2;
    best_n = n;
    inf_n = 0;
    sup_n = list_size;
    tries = n;
    delta = (uint16_t) (-1);

    while (tries--)
    {
        if (list[n] == target) return n;
        else if (list[n] > target)
        {
            sup_n = n;
            n = ((sup_n - inf_n) / 2);
        }
        else if (list[n] < target)
        {
            inf_n = n;
            n += ((sup_n - inf_n) / 2);
        }

        if (ABS(list[n] - target) < delta)
        {
            best_n = n;
            delta = ABS(list[n] - target);
        }

        if (delta <= tolerance) return best_n;
    }

    return best_n;
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void ntc_init(void)
{
    // ADC initialization
    ADC_Init(LPC_ADC, ADC_CLOCK);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOUS);

    // configurates the adc pin
    PINSEL_SetPinFunc(NTC_ADC_PORT, NTC_ADC_PIN, NTC_ADC_PIN_CONF);

    // disables the interrupt
    ADC_IntConfig(LPC_ADC, NTC_ADC_CHANNEL, DISABLE);

    // enables the adc channel
    ADC_ChannelCmd(LPC_ADC, NTC_ADC_CHANNEL, ENABLE);
}

float ntc_read(void)
{
    uint16_t adc_value;
    static float last_temp;

    // checks if conversion is not complete
    if (!ADC_ChannelGetStatus(LPC_ADC, NTC_ADC_CHANNEL, ADC_DATA_DONE)) return last_temp;

    // gets the adc value
    adc_value = ADC_ChannelGetData(LPC_ADC, NTC_ADC_CHANNEL);

    // checks the bounds values
    if (adc_value < g_table[0]) adc_value = g_table[0];
    if (adc_value > g_table[TABLE_SIZE-1]) adc_value = g_table[TABLE_SIZE-1];

    // table lookup for nearest adc value
    uint16_t index;
    index = bisect(adc_value, g_table, TABLE_SIZE, 10);

    // converts the index to temperature
    float temp;
    temp = (float) (TABLE_MAX_TEMP - index);
    last_temp = temp;

    return temp;
}
