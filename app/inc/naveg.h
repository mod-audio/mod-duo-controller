
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
void naveg_initial_state(char *bank_uid, char *pedalboard_uid, char **pedalboards_list);
// tells to navigation core the UI status
void naveg_ui_connection(uint8_t status);
// adds the control to end of the controls list
void naveg_add_control(control_t *control);
// removes the control from controls list
void naveg_remove_control(int32_t effect_instance, const char *symbol);
// increment the control value
void naveg_inc_control(uint8_t display);
// decrement the control value
void naveg_dec_control(uint8_t display);
// sets the control value
void naveg_set_control(int32_t effect_instance, const char *symbol, float value);
// gets the control value
float naveg_get_control(int32_t effect_instance, const char *symbol);
// request the next control of the display
void naveg_next_control(uint8_t display);
// change the foot value
void naveg_foot_change(uint8_t foot);
// toggle between control and tool
void naveg_toggle_tool(uint8_t tool, uint8_t display);
// returns if display is in tool mode
uint8_t naveg_is_tool_mode(uint8_t display);
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
uint8_t naveg_dialog(const char *msg);
uint8_t naveg_ui_status(void);


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
