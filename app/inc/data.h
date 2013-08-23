
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef DATA_H
#define DATA_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

typedef enum {MENU_NONE, MENU_RETURN, MENU_LIST, MENU_SELECT, MENU_CONFIRM, MENU_CANCEL, MENU_ON_OFF} menu_types_t;


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct SCALE_POINT_T {
    char *label;
    float value;
} scale_point_t;

typedef struct CONTROL_T {
    uint8_t hardware_type, hardware_id;
    uint8_t actuator_type, actuator_id;
    char *label, *symbol, *unit;
    int8_t effect_instance;
    uint8_t properties;
    float value, minimum, maximum;
    int8_t step, steps;
    uint8_t scale_points_count;
    scale_point_t **scale_points;
} control_t;

typedef struct BP_LIST_T {
    char **names, **uids;
    uint8_t count, hover, selected;
} bp_list_t;

typedef struct BANK_CONFIG_T {
    uint8_t hardware_type, hardware_id;
    uint8_t actuator_type, actuator_id;
    uint8_t function;
} bank_config_t;

typedef struct MENU_DESC_T {
    const char *name;
    menu_types_t type;
    int8_t id, parent_id;
    void (*action_cb) (void *data);
} menu_desc_t;

typedef struct MENU_DATA_T {
    char **list;
    uint8_t list_count;
    uint8_t selected, hover;
    const char *popup_content;
} menu_data_t;

typedef struct MENU_ITEM_T {
    char *name;
    const menu_desc_t *desc;
    menu_data_t data;
} menu_item_t;

typedef struct MENU_POPUP_T {
    int8_t menu_id;
    const char *popup_content;
} menu_popup_t;


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


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

control_t * data_parse_control(char **data);
void data_free_control(control_t *control);
bp_list_t *data_parse_banks_list(char **list_data, uint32_t list_count);
void data_free_banks_list(bp_list_t *bp_list);
bp_list_t *data_parse_pedalboards_list(char **list_data, uint32_t list_count);
void data_free_pedalboards_list(bp_list_t *bp_list);


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
