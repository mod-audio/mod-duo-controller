
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

static char g_back_to_bank[] = {"< Back to bank list"};


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

control_t *data_parse_control(char **data)
{
    control_t *control = NULL;
    uint32_t len = strarr_length(data);

    // checks if all data was received
    if (len >= 14)
    {
        control = (control_t *) MALLOC(sizeof(control_t));
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
        if (control->scale_points_count == 0) return control;

        control->scale_points = (scale_point_t **) MALLOC(sizeof(scale_point_t*) * control->scale_points_count);

        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
        {
            control->scale_points[i] = (scale_point_t *) MALLOC(sizeof(scale_point_t));
            control->scale_points[i]->label = str_duplicate(data[15 + (i*2)]);
            control->scale_points[i]->value = atof(data[16 + (i*2)]);
        }
    }

    return control;
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

bp_list_t *data_parse_banks_list(char **list_data, uint32_t list_count)
{
    if (!list_data || list_count == 0) return NULL;

    list_count /= 2;

    // creates a array of bank
    bp_list_t *bp_list;
    bp_list = (bp_list_t *) MALLOC(sizeof(bp_list_t));
    bp_list->hover = 0;
    bp_list->selected = 0;
    bp_list->count = list_count;
    bp_list->names = (char **) MALLOC(sizeof(char *) * (list_count + 1));
    bp_list->uids = (char **) MALLOC(sizeof(char *) * (list_count + 1));

    // fills the bp_list struct
    uint32_t i = 0, j = 0;
    while (list_data[i])
    {
        bp_list->names[j] = str_duplicate(list_data[i + 0]);
        bp_list->uids[j] = str_duplicate(list_data[i + 1]);
        i += 2;
        j++;
    }

    // does the list null terminated
    bp_list->names[j] = NULL;
    bp_list->uids[j] = NULL;

    return bp_list;
}

void data_free_banks_list(bp_list_t *bp_list)
{
    if (!bp_list) return;

    uint32_t i = 0;
    while (bp_list->names[i])
    {
        FREE(bp_list->names[i]);
        FREE(bp_list->uids[i]);
        i++;
    }

    FREE(bp_list);
}

bp_list_t *data_parse_pedalboards_list(char **list_data, uint32_t list_count)
{
    if (!list_data || list_count == 0) return NULL;

    list_count = (list_count / 2) + 1;

    // creates a array of bank
    bp_list_t *bp_list;
    bp_list = (bp_list_t *) MALLOC(sizeof(bp_list_t));
    bp_list->hover = 0;
    bp_list->selected = 1;
    bp_list->count = list_count;
    bp_list->names = (char **) MALLOC(sizeof(char *) * (list_count + 1));
    bp_list->uids = (char **) MALLOC(sizeof(char *) * (list_count + 1));

    // first line is 'back to banks list'
    bp_list->names[0] = g_back_to_bank;
    bp_list->uids[0] = NULL;

    // fills the bp_list struct
    uint32_t i = 0, j = 1;
    while (list_data[i])
    {
        bp_list->names[j] = str_duplicate(list_data[i + 0]);
        bp_list->uids[j] = str_duplicate(list_data[i + 1]);
        i += 2;
        j++;
    }

    // does the list null terminated
    bp_list->names[j] = NULL;
    bp_list->uids[j] = NULL;

    return bp_list;
}

void data_free_pedalboards_list(bp_list_t *bp_list)
{
    if (!bp_list) return;

    uint32_t i = 1;
    while (bp_list->names[i])
    {
        FREE(bp_list->names[i]);
        FREE(bp_list->uids[i]);
        i++;
    }

    FREE(bp_list);
}
