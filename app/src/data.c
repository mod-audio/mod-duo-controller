
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
static char g_back_to_settings[] = {"< Back to SETTINGS"};


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
    const uint32_t min_params = 16;

    // checks if all data was received
    if (len >= min_params)
    {
        control = (control_t *) MALLOC(sizeof(control_t));
        if (!control) goto error;

        // fills the control struct
        control->effect_instance = atoi(data[1]);
        control->symbol = str_duplicate(data[2]);
        control->label = str_duplicate(data[3]);
        control->properties_mask = atoi(data[4]);
        control->unit = str_duplicate(data[5]);
        control->value = atof(data[6]);
        control->maximum = atof(data[7]);
        control->minimum = atof(data[8]);
        control->steps = atoi(data[9]);
        control->hardware_type = atoi(data[10]);
        control->hardware_id = atoi(data[11]);
        control->actuator_type = atoi(data[12]);
        control->actuator_id = atoi(data[13]);
        control->controls_count = atoi(data[14]);
        control->control_index = atoi(data[15]);
        control->scale_points_count = 0;
        control->scale_points = NULL;

        // checks the memory allocation
        if (!control->symbol || !control->label || !control->unit) goto error;
    }

    control->properties = 0;
    if (CONTROL_PROP_ENUMERATION & control->properties_mask)
        control->properties = CONTROL_PROP_ENUMERATION;
    else if (CONTROL_PROP_SCALE_POINTS & control->properties_mask)
        control->properties = CONTROL_PROP_SCALE_POINTS;
    else if (CONTROL_PROP_TAP_TEMPO & control->properties_mask)
        control->properties = CONTROL_PROP_TAP_TEMPO;
    else if (CONTROL_PROP_BYPASS & control->properties_mask)
        control->properties = CONTROL_PROP_BYPASS;
    else if (CONTROL_PROP_TRIGGER & control->properties_mask)
        control->properties = CONTROL_PROP_TRIGGER;
    else if (CONTROL_PROP_TOGGLED & control->properties_mask)
        control->properties = CONTROL_PROP_TOGGLED;
    else if (CONTROL_PROP_LOGARITHMIC & control->properties_mask)
        control->properties = CONTROL_PROP_LOGARITHMIC;
    else if (CONTROL_PROP_INTEGER & control->properties_mask)
        control->properties = CONTROL_PROP_INTEGER;

    // checks if has scale points
    uint8_t i = 0;
    if (len >= (min_params+1) && (control->properties == CONTROL_PROP_ENUMERATION ||
        control->properties == CONTROL_PROP_SCALE_POINTS))
    {
        control->scale_points_count = atoi(data[min_params]);
        if (control->scale_points_count == 0) return control;

        control->scale_points = (scale_point_t **) MALLOC(sizeof(scale_point_t*) * control->scale_points_count);
        if (!control->scale_points) goto error;

        // initializes the scale points pointers
        for (i = 0; i < control->scale_points_count; i++) control->scale_points[i] = NULL;

        for (i = 0; i < control->scale_points_count; i++)
        {
            control->scale_points[i] = (scale_point_t *) MALLOC(sizeof(scale_point_t));
            control->scale_points[i]->label = str_duplicate(data[(min_params + 1) + (i*2)]);
            control->scale_points[i]->value = atof(data[(min_params + 2) + (i*2)]);

            if (!control->scale_points[i] || !control->scale_points[i]->label) goto error;
        }
    }

    return control;

error:
    data_free_control(control);
    return NULL;
}

void data_free_control(control_t *control)
{
    if (!control) return;

    FREE(control->symbol);
    FREE(control->label);
    FREE(control->unit);

    if (control->scale_points)
    {
        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
        {
            if (control->scale_points[i])
            {
                FREE(control->scale_points[i]->label);
                FREE(control->scale_points[i]);
            }
        }

        FREE(control->scale_points);
    }

    FREE(control);
}

bp_list_t *data_parse_banks_list(char **list_data, uint32_t list_count)
{
    if (!list_data || list_count == 0 || (list_count % 2)) return NULL;

    list_count = (list_count / 2) + 1;

    // creates a array of bank
    bp_list_t *bp_list = (bp_list_t *) MALLOC(sizeof(bp_list_t));
    if (!bp_list) goto error;

    bp_list->hover = 0;
    bp_list->selected = 0;
    bp_list->count = list_count;
    bp_list->names = (char **) MALLOC(sizeof(char *) * (list_count + 1));
    bp_list->uids = (char **) MALLOC(sizeof(char *) * (list_count + 1));

    // checks memory allocation
    if (!bp_list->names || !bp_list->uids) goto error;

    uint32_t i = 0, j = 1;

    // initializes the pointers
    for (i = 0; i < (list_count + 1); i++)
    {
        bp_list->names[i] = NULL;
        bp_list->uids[i] = NULL;
    }

    // first line is 'back to banks list'
    bp_list->names[0] = g_back_to_settings;
    bp_list->uids[0] = NULL;

    // fills the bp_list struct
    i = 0;
    while (list_data[i])
    {
        bp_list->names[j] = str_duplicate(list_data[i + 0]);
        bp_list->uids[j] = str_duplicate(list_data[i + 1]);

        // checks memory allocation
        if (!bp_list->names[j] || !bp_list->uids[j]) goto error;

        i += 2;
        j++;
    }

    return bp_list;

error:
    data_free_banks_list(bp_list);
    return NULL;
}

void data_free_banks_list(bp_list_t *bp_list)
{
    if (!bp_list) return;

    uint32_t i;

    if (bp_list->names)
    {
        for (i = 1; i < bp_list->count; i++)
            FREE(bp_list->names[i]);

        FREE(bp_list->names);
    }

    if (bp_list->uids)
    {
        for (i = 1; i < bp_list->count; i++)
            FREE(bp_list->uids[i]);

        FREE(bp_list->uids);
    }

    FREE(bp_list);
}

bp_list_t *data_parse_pedalboards_list(char **list_data, uint32_t list_count)
{
    if (!list_data || list_count == 0 || (list_count % 2)) return NULL;

    list_count = (list_count / 2) + 1;

    // creates a array of bank
    bp_list_t *bp_list = (bp_list_t *) MALLOC(sizeof(bp_list_t));
    if (!bp_list) goto error;

    bp_list->hover = 0;
    bp_list->selected = 1;
    bp_list->count = list_count;
    bp_list->names = (char **) MALLOC(sizeof(char *) * (list_count + 1));
    bp_list->uids = (char **) MALLOC(sizeof(char *) * (list_count + 1));

    // checks memory allocation
    if (!bp_list->names || !bp_list->uids) goto error;

    uint32_t i = 0, j = 1;

    // initializes the pointers
    for (i = 0; i < (list_count + 1); i++)
    {
        bp_list->names[i] = NULL;
        bp_list->uids[i] = NULL;
    }

    // first line is 'back to banks list'
    bp_list->names[0] = g_back_to_bank;
    bp_list->uids[0] = NULL;

    // fills the bp_list struct
    i = 0;
    while (list_data[i])
    {
        bp_list->names[j] = str_duplicate(list_data[i + 0]);
        bp_list->uids[j] = str_duplicate(list_data[i + 1]);

        // checks memory allocation
        if (!bp_list->names[j] || !bp_list->uids[j]) goto error;

        i += 2;
        j++;
    }

    return bp_list;

error:
    data_free_pedalboards_list(bp_list);
    return NULL;
}

void data_free_pedalboards_list(bp_list_t *bp_list)
{
    if (!bp_list) return;

    uint32_t i;

    if (bp_list->names)
    {
        for (i = 1; i < bp_list->count; i++)
            FREE(bp_list->names[i]);

        FREE(bp_list->names);
    }

    if (bp_list->uids)
    {
        for (i = 1; i < bp_list->count; i++)
            FREE(bp_list->uids[i]);

        FREE(bp_list->uids);
    }

    FREE(bp_list);
}
