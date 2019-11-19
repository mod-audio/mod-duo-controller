
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef NAVEG_H
#define NAVEG_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdbool.h>
#include <stdint.h>
#include "data.h"
#include "glcd_widget.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

#define ALL_EFFECTS     -1
#define ALL_CONTROLS    ":all"
enum {UI_DISCONNECTED, UI_CONNECTED};


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


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/
bool g_should_wait_for_webgui;
bool g_protocol_busy;
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

// initialize the navigation nodes and structs
void naveg_init(void);
// sets the initial state of banks/pedalboards navigation
void naveg_initial_state(uint16_t max_menu, uint16_t page_min, uint16_t page_max, char *bank_uid, char *pedalboard_uid, char **pedalboards_list);
// tells to navigation core the UI status
void naveg_ui_connection(uint8_t status);
// adds the control to end of the controls list
void naveg_add_control(control_t *control, uint8_t protocol);
// removes the control from controls list
void naveg_remove_control(uint8_t hw_id);
// increment the control value
void naveg_inc_control(uint8_t display);
// decrement the control value
void naveg_dec_control(uint8_t display);
// sets the control value
void naveg_set_control(uint8_t hw_id, float value);
// gets the control value
float naveg_get_control(uint8_t hw_id);
// change the foot value
void naveg_foot_change(uint8_t foot, uint8_t pressed);
// request the next control of the display
void naveg_next_control(uint8_t display);
// toggle between control and tool
void naveg_toggle_tool(uint8_t tool, uint8_t display);
// returns if display is in tool mode
uint8_t naveg_is_tool_mode(uint8_t display);
//toggle / set master volume 
void naveg_master_volume(uint8_t set);
//increments the mater volume
void naveg_set_master_volume(uint8_t set);
//returns if current state is master volume
uint8_t naveg_is_master_vol(void);
// stores the banks list
void naveg_set_banks(bp_list_t *bp_list);
// returns the banks list
bp_list_t *naveg_get_banks(void);
// configurates the bank
void naveg_bank_config(bank_config_t *bank_conf);
// stores the pedalboards list of current bank
void naveg_set_pedalboards(bp_list_t *bp_list);
// returns the pedalboards list of current bank
bp_list_t *naveg_get_pedalboards(void);
// runs the enter action on tool mode
void naveg_enter(uint8_t display);
// runs the up action on tool mode
void naveg_up(uint8_t display);
// runs the down action on tool mode
void naveg_down(uint8_t display);
// resets to root menu
void naveg_reset_menu(void);
// update the navigation screen if necessary
void naveg_update(void);
int naveg_need_update(void);
uint8_t naveg_dialog(const char *msg);
uint8_t naveg_ui_status(void);
void naveg_set_index(uint8_t update, uint8_t display, uint8_t new_index, uint8_t new_index_count);
//refreshes screen with current menu item
void naveg_settings_refresh(uint8_t display_id);
//returns if tap tempo enabled 
uint8_t naveg_tap_tempo_status(uint8_t id);
//updates all items in a menu
void naveg_menu_refresh(uint8_t display_id);
//updates a specific item in a menu
void naveg_update_gain(uint8_t display_id, uint8_t update_id, float value, float min, float max);

void naveg_bypass_refresh(uint8_t bypass_1, uint8_t bypass_2, uint8_t quick_bypass);

void naveg_menu_item_changed_cb(uint8_t item_ID, uint16_t value);

uint8_t naveg_dialog_status(void);

uint8_t naveg_tool_is_on(uint8_t tool);

uint8_t naveg_banks_mode_pb(void);

char* naveg_get_current_pb_name(void);
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
