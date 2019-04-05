
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
typedef enum {MENU_NONE, MENU_RETURN, MENU_LIST, MENU_SELECT, MENU_CONFIRM, MENU_CANCEL, MENU_TOGGLE, MENU_CONFIRM2, 
                MENU_OK, MENU_VOL, MENU_SET} menu_types_t;

enum {MENU_EV_ENTER, MENU_EV_UP, MENU_EV_DOWN, MENU_EV_NONE};



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
    int16_t effect_instance;
    uint8_t properties_mask;
    uint8_t properties;
    float value, minimum, maximum;
    int16_t step, steps;
    uint8_t controls_count, control_index;
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
    int16_t id, parent_id;
    void (*action_cb) (void *data, int event);
    uint8_t need_update;
} menu_desc_t;

typedef struct MENU_DATA_T {
    char **list;
    uint8_t list_count;
    uint8_t selected, hover;
    char *popup_header;
    const char *popup_content;

    // FIXME: need to be improved, not all menu items should have this vars (wasting memory)
    int16_t min, max, value, step;
} menu_data_t;

typedef struct MENU_ITEM_T {
    char *name;
    menu_desc_t *desc;
    menu_data_t data;
} menu_item_t;

typedef struct MENU_POPUP_T {
    int16_t menu_id;
    char *popup_header;
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
