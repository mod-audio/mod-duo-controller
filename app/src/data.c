
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "data.h"
#include "config.h"
#include "utils.h"

#include <stdlib.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/


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


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void data_parse_control(char **data, control_t *control)
{
    uint32_t len = strarr_length(data);

    // checks if all data was received
    if (len >= 14)
    {
        control->effect_instance = atoi(data[1]);
        control->symbol = str_duplicate(data[2]);
        control->label = str_duplicate(data[3]);
        control->properties = atoi(data[4]);
        control->unit = str_duplicate(data[5]);
        control->value = atof(data[6]);
        control->maximum = atof(data[7]);
        control->minimum = atof(data[8]);
        control->steps = atoi(data[9]);
        control->hardware_type = atoi(data[10]);
        control->hardware_id = atoi(data[11]);
        control->actuator_type = atoi(data[12]);
        control->actuator_id = atoi(data[13]);
        control->scale_points_count = 0;
        control->scale_points = NULL;
    }

    // checks if has scale points
    if (len >= 15 && control->properties == CONTROL_PROP_ENUMERATION)
    {
        control->scale_points_count = atoi(data[14]);
        control->scale_points = (scale_point_t **) MALLOC(sizeof(scale_point_t*) * control->scale_points_count);

        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
        {
            control->scale_points[i] = (scale_point_t *) MALLOC(sizeof(scale_point_t));
            control->scale_points[i]->label = str_duplicate(data[15 + (i*2)]);
            control->scale_points[i]->value = atof(data[16 + (i*2)]);
        }
    }
}

void data_free_control(control_t *control)
{
    if (control)
    {
        FREE(control->symbol);
        FREE(control->label);
        FREE(control->unit);
    }

    uint8_t i;
    for (i = 0; i < control->scale_points_count; i++)
    {
        FREE(control->scale_points[i]->label);
        FREE(control->scale_points[i]);
    }

    if (control->scale_points) FREE(control->scale_points);

    FREE(control);
}
