
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

typedef enum {MENU_NONE, MENU_RETURN, MENU_LIST, MENU_SELECT, MENU_CONFIRM, MENU_CANCEL, MENU_ON_OFF,
              MENU_YES_NO, MENU_BYP_PROC} menu_types_t;


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
    uint8_t properties_mask;
    float value, minimum, maximum;
    int8_t step, steps;
    uint8_t controls_count, control_index;
    uint8_t scale_points_count;
    scale_point_t **scale_points;
    uint8_t properties;
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
    int16_t id, parent_id;
    void (*action_cb) (void *data);
    uint8_t need_update;
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
    int16_t menu_id;
    const char *popup_content;
} menu_popup_t;

typedef struct CONTROL_CHAIN_T {
    uint8_t start;
    uint8_t hw_type_origin, hw_id_origin;
    uint8_t hw_type_destination, hw_id_destination;
    uint8_t function;
    uint8_t data_size;
    uint8_t checksum;
    uint8_t end;
    uint8_t *data; // keep the data pointer at end of struct
} control_chain_t;


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

#define MENU_ITEM_IS_TOGGLE_TYPE(item)      (item->desc->type == MENU_ON_OFF || item->desc->type == MENU_YES_NO || \
                                             item->desc->type == MENU_BYP_PROC)


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
