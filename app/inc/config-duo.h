
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "device.h"

////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO HARDWARE

//// GPIO macros
#define CONFIG_PIN_INPUT(port, pin)     GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_INPUT)
#define CONFIG_PIN_OUTPUT(port, pin)    GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_OUTPUT)
#define SET_PIN(port, pin)              GPIO_SetValue((port), (1 << (pin)))
#define CLR_PIN(port, pin)              GPIO_ClearValue((port), (1 << (pin)))
#define READ_PIN(port, pin)             ((FIO_ReadValue(port) >> (pin)) & 1)
#define CONFIG_PORT_INPUT(port)         FIO_ByteSetDir((port), 0, 0xFF, GPIO_DIRECTION_INPUT)
#define CONFIG_PORT_OUTPUT(port)        FIO_ByteSetDir((port), 0, 0xFF, GPIO_DIRECTION_OUTPUT)
#define WRITE_PORT(port, value)         FIO_ByteSetValue((port), 0, (uint8_t)(value)); \
                                        FIO_ByteClearValue((port), 0, (uint8_t)(~value))
#define READ_PORT(port)                 FIO_ByteReadValue((port), (0))

//// Serial definitions
// If the UART ISR (or callbacks) uses freeRTOS API, the priorities values must be
// equal or greater than configMAX_SYSCALL_INTERRUPT_PRIORITY
// SERIAL0
#define SERIAL0
#define SERIAL0_BAUD_RATE       1500000
#define SERIAL0_PRIORITY        1
#define SERIAL0_RX_PORT         0
#define SERIAL0_RX_PIN          3
#define SERIAL0_RX_FUNC         1
#define SERIAL0_RX_BUFF_SIZE    32
#define SERIAL0_TX_PORT         0
#define SERIAL0_TX_PIN          2
#define SERIAL0_TX_FUNC         1
#define SERIAL0_TX_BUFF_SIZE    32
#define SERIAL0_HAS_OE          0
// SERIAL1
#define SERIAL1
#define SERIAL1_BAUD_RATE       115200
#define SERIAL1_PRIORITY        2
#define SERIAL1_RX_PORT         0
#define SERIAL1_RX_PIN          16
#define SERIAL1_RX_FUNC         1
#define SERIAL1_RX_BUFF_SIZE    64
#define SERIAL1_TX_PORT         0
#define SERIAL1_TX_PIN          15
#define SERIAL1_TX_FUNC         1
#define SERIAL1_TX_BUFF_SIZE    64
#define SERIAL1_HAS_OE          0

//// Hardware peripheral definitions
// Clock power control
#define HW_CLK_PWR_CONTROL      CLKPWR_PCONP_PCTIM0 | CLKPWR_PCONP_PCTIM1 | \
                                CLKPWR_PCONP_PCUART0 | CLKPWR_PCONP_PCUART1 | \
                                CLKPWR_PCONP_PCSSP0 |   \
                                CLKPWR_PCONP_PCGPIO

//// Slots count
// One slot is a set of display, knob, footswitch and led
#define SLOTS_COUNT         2

//// LEDs configuration
// Amount of LEDS
#define LEDS_COUNT          SLOTS_COUNT

// LEDs ports and pins definitions
// format definition: {R_PORT, R_PIN, G_PORT, G_PIN, B_PORT, B_PIN}
#define LED0_PINS           {2, 1, 2, 2, 2, 0}
#define LED1_PINS           {2, 4, 2, 5, 2, 3}

//// GLCDs configurations
// GLCD driver, valid options: KS0108, UC1701
#define GLCD_DRIVER         UC1701
#define UC1701_REVERSE_COLUMNS
#define UC1701_REVERSE_ROWS

// Amount of displays
#define GLCD_COUNT          SLOTS_COUNT

// GCLD common definitions
// check the drivers header to see how to set the structure
#define GLCD_COMMON_CONFIG  .ssp_module = LPC_SSP0, .ssp_clock = 1000000, \
                            .ssp_clk_port = 1, .ssp_clk_pin = 20, .ssp_clk_func = 3, \
                            .ssp_mosi_port = 1, .ssp_mosi_pin = 24, .ssp_mosi_func = 3, \
                            .cd_port = 1, .cd_pin = 19

#define GLCD0_CONFIG    { GLCD_COMMON_CONFIG, \
                          .cs_port = 0, .cs_pin = 11, \
                          .rst_port = 0, .rst_pin = 10, \
                          .backlight_port = 1, .backlight_pin = 18 },

#define GLCD1_CONFIG    { GLCD_COMMON_CONFIG, \
                          .cs_port = 0, .cs_pin = 29, \
                          .rst_port = 0, .rst_pin = 30, \
                          .backlight_port = 1, .backlight_pin = 26 },

//// Actuators configuration
// Actuators IDs
enum {ENCODER0, ENCODER1, FOOTSWITCH0, FOOTSWITCH1};

// Amount of footswitches
#define FOOTSWITCHES_COUNT  SLOTS_COUNT

// Footswitches ports and pins definitions
// button definition: {BUTTON_PORT, BUTTON_PIN}
#define FOOTSWITCH0_PINS    {1, 29}
#define FOOTSWITCH1_PINS    {1, 28}

// Amount of encoders
#define ENCODERS_COUNT      SLOTS_COUNT

// Encoders ports and pins definitions
// encoder definition: {ENC_BUTTON_PORT, ENC_BUTTON_PIN, ENC_CHA_PORT, ENC_CHA_PIN, ENC_CHB_PORT, ENC_CHB_PIN}
#define ENCODER0_PINS       {0, 17, 0, 22, 0, 18}
#define ENCODER1_PINS       {2, 8, 2, 6, 2, 7}

#define SHUTDOWN_BUTTON_PORT    4
#define SHUTDOWN_BUTTON_PIN     28


////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO FIRMWARE

//// webgui configuration
// define the interface
#define WEBGUI_SERIAL               0

// define how many bytes will be allocated to rx/tx buffers
#define WEBGUI_COMM_RX_BUFF_SIZE    4096
#define WEBGUI_COMM_TX_BUFF_SIZE    512

//// Protocol commands configuration
// ping
#define PING_CMD                "ping"
// say <message>
#define SAY_CMD                 "say %s ..."
// led <led_id> <red> <green> <blue>
#define LED_CMD                 "led %i %i %i %i ..."
// glcd_text <glcd_id> <x_pos> <y_pos> <text>
#define GLCD_TEXT_CMD           "glcd_text %i %i %i %s"
// glcd_dialog <content>
#define GLCD_DIALOG_CMD         "glcd_dialog %s"
// glcd_draw <glcd_id> <x_pos> <y_pos> <data>
#define GLCD_DRAW_CMD           "glcd_draw %i %i %i %s"
// ui_con
#define GUI_CONNECTED_CMD       "ui_con"
// ui_dis
#define GUI_DISCONNECTED_CMD    "ui_dis"
// control_add <effect_instance> <symbol> <label> <control_prop> <unit> <value> <max> <min>
//             <steps> <hw_type> <hw_id> <actuator_type> <actuator_id> <controls_count> <control_index>
//             [scale_point_count] {[scale_point1_label] [scale_point1_value]}...
#define CONTROL_ADD_CMD         "control_add %i %s %s %i %s %f %f %f %i %i %i %i %i %i %i ..."
// control_rm <effect_instance> <symbol>
#define CONTROL_REMOVE_CMD      "control_rm %i %s"
// control_get <effect_instance> <symbol>
#define CONTROL_GET_CMD         "control_get %i %s"
// control_set <effect_instance> <symbol> <value>
#define CONTROL_SET_CMD         "control_set %i %s %f"
// control_next <hardware_type> <hardware_id> <actuator_type> <actuator_id>
#define CONTROL_NEXT_CMD        "control_next %i %i %i %i"
// initial_state <current_bank_uid> <current_pedalboard_uid> [current_pedalboards_list]
#define INITIAL_STATE_CMD       "initial_state %s %s ..."
// banks
#define BANKS_CMD               "banks"
// bank_config <hw_type> <hw_id> <actuator_type> <actuator_id> <function>
#define BANK_CONFIG_CMD         "bank_config %i %i %i %i %i"
// pedalboards <bank_uid>
#define PEDALBOARDS_CMD         "pedalboards %s"
// pedalboard <bank_id> <pedalboard_uid>
#define PEDALBOARD_CMD          "pedalboard %i %s"
// pedalboard_reset
#define PEDALBOARD_RESET_CMD    "pedalboard_reset"
// pedalboard_save
#define PEDALBOARD_SAVE_CMD     "pedalboard_save"
// tuner <frequency> <note> <cents>
#define TUNER_CMD               "tuner %f %s %i"
// tuner on
#define TUNER_ON_CMD            "tuner on"
// tuner off
#define TUNER_OFF_CMD           "tuner off"
// tuner_input <input>
#define TUNER_INPUT_CMD         "tuner_input %i"
// hw_con <hw_type> <hw_id>
#define HW_CONNECTED_CMD        "hw_con %i %i"
// hw_dis <hw_type> <hw_id>
#define HW_DISCONNECTED_CMD     "hw_dis %i %i"
// resp <status> ...
#define RESPONSE_CMD            "resp %i ..."
// reboot in restore mode
#define RESTORE_CMD             "restore"

//// Control propertires definitions
#define CONTROL_PROP_LINEAR         0
#define CONTROL_PROP_BYPASS         1
#define CONTROL_PROP_TAP_TEMPO      2
#define CONTROL_PROP_ENUMERATION    4
#define CONTROL_PROP_SCALE_POINTS   8
#define CONTROL_PROP_TRIGGER        16
#define CONTROL_PROP_TOGGLED        32
#define CONTROL_PROP_LOGARITHMIC    64
#define CONTROL_PROP_INTEGER        128

//// Banks functions definition
#define BANK_FUNC_NONE              0
#define BANK_FUNC_TRUE_BYPASS       1
#define BANK_FUNC_PEDALBOARD_NEXT   2
#define BANK_FUNC_PEDALBOARD_PREV   3
#define BANK_FUNC_AMOUNT            4

//// Tools configuration
// navigation update time, this is only useful in tool mode
#define NAVEG_UPDATE_TIME   1500

// time in milliseconds to enter in tool mode (hold rotary encoder button)
#define TOOL_MODE_TIME      500

// which display will show which tool
#define DISPLAY_TOOL_SYSTEM 0
#define DISPLAY_TOOL_TUNER  1
#define DISPLAY_TOOL_NAVIG  2

//// Screen definitions
// defines the default rotary text
#define SCREEN_ROTARY_DEFAULT_NAME      "KNOB #"
// defines the default foot text
#define SCREEN_FOOT_DEFAULT_NAME        "FOOT #"

//// System menu configuration
// includes the system menu callbacks
#include "system.h"
// defines the menu id's
#define ROOT_ID         (0 * 10)
#define VOL_GAIN_ID     (1 * 10)
#define IN1_ID          (2 * 10)
#define IN1_STAGE_ID    (3 * 10)
#define IN2_ID          (4 * 10)
#define IN2_STAGE_ID    (5 * 10)
#define HEADPHONE_ID    (6 * 10)
#define INFO_ID         (7 * 10)
#define SERVICES_ID     (8 * 10)
#define VERSIONS_ID     (9 * 10)
#define UPGRADE_ID      (10 * 10)
#define VOLUME_ID       (11 * 10)
#define DEVICE_ID       (12 * 10)
#define PEDALBOARD_ID   (13 * 10)
#define BLUETOOTH_ID    (14 * 10)
#define BANKS_ID        (15 * 10)

#define IN1_VOLUME      VOLUME_ID+0
#define IN2_VOLUME      VOLUME_ID+1
#define OUT1_VOLUME     VOLUME_ID+2
#define OUT2_VOLUME     VOLUME_ID+3
#define HP_VOLUME       VOLUME_ID+4

#define PEDALBOARD_SAVE_ID   PEDALBOARD_ID+2
#define PEDALBOARD_RESET_ID  PEDALBOARD_ID+3

#define BLUETOOTH_DISCO_ID   BLUETOOTH_ID+2


// menu definition format: {name, type, id, parent_id, action_callback, need_update}
#define SYSTEM_MENU     \
    {"SETTINGS",                        MENU_LIST,      ROOT_ID,            -1,             NULL                , 0},  \
    {"Banks",                           MENU_NONE,      BANKS_ID,           ROOT_ID,        system_banks_cb     , 0},  \
    {"Volume and Gains",                MENU_LIST,      VOL_GAIN_ID,        ROOT_ID,        NULL                , 0},  \
    {"< Back to SETTINGS",              MENU_RETURN,    VOL_GAIN_ID+1,      VOL_GAIN_ID,    system_save_gains_cb, 0},  \
    {"Input 1",                         MENU_LIST,      IN1_ID,             VOL_GAIN_ID,    NULL                , 0},  \
    {"< Back to Volume and Gains",      MENU_RETURN,    IN1_ID+1,           IN1_ID,         NULL                , 0},  \
    {"Stage",                           MENU_SELECT,    IN1_STAGE_ID,       IN1_ID,         system_stage_cb     , 0},  \
    {"< Back to Input 1",               MENU_RETURN,    IN1_STAGE_ID+1,     IN1_STAGE_ID,   NULL                , 0},  \
    {"Low",                             MENU_NONE,      IN1_STAGE_ID+2,     IN1_STAGE_ID,   system_stage_cb     , 0},  \
    {"Mid",                             MENU_NONE,      IN1_STAGE_ID+3,     IN1_STAGE_ID,   system_stage_cb     , 0},  \
    {"High",                            MENU_NONE,      IN1_STAGE_ID+4,     IN1_STAGE_ID,   system_stage_cb     , 0},  \
    {"Fine Adjust",                     MENU_GRAPH,     IN1_VOLUME,         IN1_ID,         system_volume_cb    , 0},  \
    {"Input 2",                         MENU_LIST,      IN2_ID,             VOL_GAIN_ID,    NULL                , 0},  \
    {"< Back to Volume and Gains",      MENU_RETURN,    IN2_ID+1,           IN2_ID,         NULL                , 0},  \
    {"Stage",                           MENU_SELECT,    IN2_STAGE_ID,       IN2_ID,         system_stage_cb     , 0},  \
    {"< Back to Input 2",               MENU_RETURN,    IN2_STAGE_ID+1,     IN2_STAGE_ID,   NULL                , 0},  \
    {"Low",                             MENU_NONE,      IN2_STAGE_ID+2,     IN2_STAGE_ID,   system_stage_cb     , 0},  \
    {"Mid",                             MENU_NONE,      IN2_STAGE_ID+3,     IN2_STAGE_ID,   system_stage_cb     , 0},  \
    {"High",                            MENU_NONE,      IN2_STAGE_ID+4,     IN2_STAGE_ID,   system_stage_cb     , 0},  \
    {"Fine Adjust",                     MENU_GRAPH,     IN2_VOLUME,         IN2_ID,         system_volume_cb    , 0},  \
    {"Output 1",                        MENU_GRAPH,     OUT1_VOLUME,        VOL_GAIN_ID,    system_volume_cb    , 0},  \
    {"Output 2",                        MENU_GRAPH,     OUT2_VOLUME,        VOL_GAIN_ID,    system_volume_cb    , 0},  \
    {"Headphone",                       MENU_LIST,      HEADPHONE_ID,       ROOT_ID,        system_hp_bypass_cb , 0},  \
    {"< Back to SETTINGS",              MENU_RETURN,    HEADPHONE_ID+1,     HEADPHONE_ID,   system_save_gains_cb, 0},  \
    {"Volume",                          MENU_GRAPH,     HP_VOLUME,          HEADPHONE_ID,   system_volume_cb    , 0},  \
    {"Direct Monitoring: ",             MENU_ON_OFF,    HEADPHONE_ID+3,     HEADPHONE_ID,   system_hp_bypass_cb , 0},  \
    {"Pedalboard",                      MENU_LIST,      PEDALBOARD_ID,      ROOT_ID,        NULL                , 0},  \
    {"< Back to SETTINGS",              MENU_RETURN,    PEDALBOARD_ID+1,    PEDALBOARD_ID,  NULL                , 0},  \
    {"Save State",                      MENU_CONFIRM,   PEDALBOARD_SAVE_ID, PEDALBOARD_ID,  system_pedalboard_cb, 0},  \
    {"Reset State",                     MENU_CONFIRM,   PEDALBOARD_RESET_ID,PEDALBOARD_ID,  system_pedalboard_cb, 0},  \
    {"Bluetooth",                       MENU_LIST,      BLUETOOTH_ID,       ROOT_ID,        system_bluetooth_cb , 1},  \
    {"< Back to SETTINGS",              MENU_RETURN,    BLUETOOTH_ID+1,     BLUETOOTH_ID,   NULL                , 0},  \
    {"Enable discovery",                MENU_OK,        BLUETOOTH_DISCO_ID, BLUETOOTH_ID,   system_bluetooth_cb , 0},  \
    {"Status:",                         MENU_NONE,      BLUETOOTH_ID+3,     BLUETOOTH_ID,   NULL                , 0},  \
    {"Name:",                           MENU_NONE,      BLUETOOTH_ID+4,     BLUETOOTH_ID,   NULL                , 0},  \
    {"Address:",                        MENU_NONE,      BLUETOOTH_ID+5,     BLUETOOTH_ID,   NULL                , 0},  \
    {"Info",                            MENU_LIST,      INFO_ID,            ROOT_ID,        NULL                , 0},  \
    {"< Back to SETTINGS",              MENU_RETURN,    INFO_ID+1,          INFO_ID,        NULL                , 0},  \
    {"Services",                        MENU_LIST,      SERVICES_ID,        INFO_ID,        system_services_cb  , 1},  \
    {"< Back to Info",                  MENU_RETURN,    SERVICES_ID+1,      SERVICES_ID,    NULL                , 0},  \
    {"jack:",                           MENU_NONE,      SERVICES_ID+2,      SERVICES_ID,    NULL                , 0},  \
    {"mod-host:",                       MENU_NONE,      SERVICES_ID+3,      SERVICES_ID,    NULL                , 0},  \
    {"mod-ui:",                         MENU_NONE,      SERVICES_ID+4,      SERVICES_ID,    NULL                , 0},  \
    {"ttymidi:",                        MENU_NONE,      SERVICES_ID+5,      SERVICES_ID,    NULL                , 0},  \
    {"Versions",                        MENU_LIST,      VERSIONS_ID,        INFO_ID,        system_versions_cb  , 0},  \
    {"< Back to Info",                  MENU_RETURN,    VERSIONS_ID+1,      VERSIONS_ID,    NULL                , 0},  \
    {"release:",                        MENU_NONE,      VERSIONS_ID+2,      VERSIONS_ID,    NULL                , 0},  \
    {"restore:",                        MENU_NONE,      VERSIONS_ID+3,      VERSIONS_ID,    NULL                , 0},  \
    {"system:",                         MENU_NONE,      VERSIONS_ID+4,      VERSIONS_ID,    NULL                , 0},  \
    {"controller:",                     MENU_NONE,      VERSIONS_ID+5,      VERSIONS_ID,    NULL                , 0},  \
    {"Device",                          MENU_LIST,      DEVICE_ID,          INFO_ID,        system_device_cb    , 0},  \
    {"< Back to Info",                  MENU_RETURN,    DEVICE_ID+1,        DEVICE_ID,      NULL                , 0},  \
    {"Serial Number:",                  MENU_OK,        DEVICE_ID+2,        DEVICE_ID,      system_tag_cb       , 0},  \
    {"System Upgrade",                  MENU_CONFIRM,   UPGRADE_ID,         ROOT_ID,        system_upgrade_cb   , 0},  \

// popups text content, format : {menu_id, text_content}
#define POPUP_CONTENT   \
    {PEDALBOARD_ID, "To access pedalboard options please disconnect from the graphical interface"}, \
    {PEDALBOARD_SAVE_ID, "Save all current pedalboard values as default?"},         \
    {PEDALBOARD_RESET_ID, "Reset all pedalboard values to last saved state?"},      \
    {BLUETOOTH_DISCO_ID, "Bluetooth discovery mode is now enabled for 2 minutes"},  \
    {UPGRADE_ID, "To proceed with system upgrade please keep pressed left footswitch and click YES."},

//// Foot functions leds colors
#define TOGGLED_COLOR           GREEN
#define TRIGGER_COLOR           GREEN
#define TAP_TEMPO_COLOR         GREEN
#define ENUMERATED_COLOR        GREEN
#define BYPASS_COLOR            RED
#define TRUE_BYPASS_COLOR       WHITE
#define PEDALBOARD_NEXT_COLOR   WHITE
#define PEDALBOARD_PREV_COLOR   WHITE

//// Tap Tempo
// defines the time that the led will stay turned on (in milliseconds)
#define TAP_TEMPO_TIME_ON       20

//// Toggled
// defines the toggled footer text
#define TOGGLED_ON_FOOTER_TEXT      "ON"
#define TOGGLED_OFF_FOOTER_TEXT     "OFF"

//// Bypass
// defines the bypass footer text
#define BYPASS_ON_FOOTER_TEXT       "OFF"
#define BYPASS_OFF_FOOTER_TEXT      "ON"

//// Bank configuration functions
// defines the true bypass footer text
#define TRUE_BYPASS_FOOTER_TEXT     "TRUE BYPASS"
// defines the next pedalboard footer text
#define PEDALBOARD_NEXT_FOOTER_TEXT "+"
// defines the previous pedalboard footer text
#define PEDALBOARD_PREV_FOOTER_TEXT "-"

//// Command line interface configurations
// defines the cli serial
#define CLI_SERIAL                  1
// defines how much time wait for console response (in milliseconds)
#define CLI_RESPONSE_TIMEOUT        500

//// Dynamic menory allocation
// defines the heap size (in bytes)
#define RTOS_HEAP_SIZE  (32 * 1024)
// these macros should be used in replacement to default malloc and free functions of stdlib.h
// The FREE function is NULL safe
#include "FreeRTOS.h"
#define MALLOC(n)       pvPortMalloc(n)
#define FREE(pv)        vPortFree(pv)


////////////////////////////////////////////////////////////////
////// DON'T CHANGE THIS DEFINES

//// Actuators types
#define NONE            0
#define FOOT            1
#define KNOB            2

// serial count definition
#define SERIAL_COUNT    4

// check serial rx buffer size
#ifndef SERIAL0_RX_BUFF_SIZE
#define SERIAL0_RX_BUFF_SIZE    0
#endif
#ifndef SERIAL1_RX_BUFF_SIZE
#define SERIAL1_RX_BUFF_SIZE    0
#endif
#ifndef SERIAL2_RX_BUFF_SIZE
#define SERIAL2_RX_BUFF_SIZE    0
#endif
#ifndef SERIAL3_RX_BUFF_SIZE
#define SERIAL3_RX_BUFF_SIZE    0
#endif

// check serial tx buffer size
#ifndef SERIAL0_TX_BUFF_SIZE
#define SERIAL0_TX_BUFF_SIZE    0
#endif
#ifndef SERIAL1_TX_BUFF_SIZE
#define SERIAL1_TX_BUFF_SIZE    0
#endif
#ifndef SERIAL2_TX_BUFF_SIZE
#define SERIAL2_TX_BUFF_SIZE    0
#endif
#ifndef SERIAL3_TX_BUFF_SIZE
#define SERIAL3_TX_BUFF_SIZE    0
#endif

#define SERIAL_MAX_RX_BUFF_SIZE     MAX(MAX(SERIAL0_RX_BUFF_SIZE, SERIAL1_RX_BUFF_SIZE), \
                                        MAX(SERIAL2_RX_BUFF_SIZE, SERIAL3_RX_BUFF_SIZE))

#define SERIAL_MAX_TX_BUFF_SIZE     MAX(MAX(SERIAL0_TX_BUFF_SIZE, SERIAL1_TX_BUFF_SIZE), \
                                        MAX(SERIAL2_TX_BUFF_SIZE, SERIAL3_TX_BUFF_SIZE))

// GLCD configurations definitions
#ifndef GLCD0_CONFIG
#define GLCD0_CONFIG
#endif

#ifndef GLCD1_CONFIG
#define GLCD1_CONFIG
#endif

#ifndef GLCD2_CONFIG
#define GLCD2_CONFIG
#endif

#ifndef GLCD3_CONFIG
#define GLCD3_CONFIG
#endif

// GLCD drivers definitions
#define KS0108      0
#define UC1701      1

// GLCD driver include
#if GLCD_DRIVER == KS0108
#include "ks0108.h"
#elif GLCD_DRIVER == UC1701
#include "uc1701.h"
#endif

#endif
