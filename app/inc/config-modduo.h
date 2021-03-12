
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
// SERIAL0 webgui
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
// SERIAL1 cli
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
// SERIAL3 (system callbacks)
#define SERIAL3
#define SERIAL3_BAUD_RATE       1500000
#define SERIAL3_PRIORITY        2
#define SERIAL3_RX_PORT         0
#define SERIAL3_RX_PIN          0
#define SERIAL3_RX_FUNC         2
#define SERIAL3_RX_BUFF_SIZE    32
#define SERIAL3_TX_PORT         0
#define SERIAL3_TX_PIN          1
#define SERIAL3_TX_FUNC         2
#define SERIAL3_TX_BUFF_SIZE    32
#define SERIAL3_HAS_OE          0

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
#define FOOTSWITCHES_ACTUATOR_COUNT SLOTS_COUNT

// Footswitches ports and pins definitions
// button definition: {BUTTON_PORT, BUTTON_PIN}
#define FOOTSWITCH0_PINS    {1, 29}
#define FOOTSWITCH1_PINS    {1, 28}

// Amount of encoders
#define ENCODERS_COUNT      SLOTS_COUNT

//total amount of actuators
#define TOTAL_ACTUATORS (ENCODERS_COUNT + FOOTSWITCHES_COUNT)

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

//// system serial configuration
// define the interface
#define SYSTEM_SERIAL               3

// define how many bytes will be allocated to rx/tx buffers
#define SYSTEM_COMM_RX_BUFF_SIZE    1024
#define SYSTEM_COMM_TX_BUFF_SIZE    512

//// Tools configuration
// navigation update time, this is only useful in tool mode
#define NAVEG_UPDATE_TIME   1500

// time in milliseconds to enter in tool mode (hold rotary encoder button)
#define TOOL_MODE_TIME      500

// the amount of pulses from the encoder that is equal to one up/down movement in a menu
#define SCROL_SENSITIVITY   0

#define ENCODER_ACCEL_STEP_1 2
#define ENCODER_ACCEL_STEP_2 3
#define ENCODER_ACCEL_STEP_3 5

// which display will show which tool
#define DISPLAY_TOOL_SYSTEM         0
#define DISPLAY_TOOL_TUNER          1
#define DISPLAY_TOOL_NAVIG          2
#define DISPLAY_TOOL_SYSTEM_SUBMENU 3


//// Screen definitions
// defines the default rotary text
#define SCREEN_ROTARY_DEFAULT_NAME      "ENDLESS KNOB #"
// defines the default foot text
#define SCREEN_FOOT_DEFAULT_NAME        "FOOT #"

//// System menu configuration
// includes the system menu callbacks
#include "system.h"
// defines the menu id's
#define ROOT_ID         (0 * 10)
#define INP_ID          (1 * 10)
#define OUTP_ID         (2 * 10)
#define TUNER_ID        (3 * 10)
#define SYSTEM_ID       (4 * 10)
#define INFO_ID         (7 * 10)
#define SERVICES_ID     (8 * 10)
#define VERSIONS_ID     (9 * 10)
#define UPGRADE_ID      (10 * 10)
#define VOLUME_ID       (11 * 10)
#define DEVICE_ID       (12 * 10)
#define PEDALBOARD_ID   (13 * 10)
#define BLUETOOTH_ID    (14 * 10)
#define BANKS_ID        (15 * 10)
#define DISP_BL_ID      (16 * 10)
#define BYPASS_ID       (17 * 10)
#define TEMPO_ID        (18 * 10)
#define PROFILES_ID     (19 * 10)
#define MIDI_ID         (20 * 10)

//pedalboard sub menu
#define PEDALBOARD_SAVE_ID   PEDALBOARD_ID+2
#define PEDALBOARD_RESET_ID  PEDALBOARD_ID+3

//bypass sub menu
#define BP1_ID              BYPASS_ID + 1
#define BP2_ID              BYPASS_ID + 2 
#define BP12_ID             BYPASS_ID + 3
#define BP_SELECT_ID        BYPASS_ID + 4

//tempo sub menu
#define GLOBAL_TEMPO_ID    TEMPO_ID + 1
#define BEATS_PER_BAR_ID   TEMPO_ID + 2

//inputs sub menu
#define IN1_VOLUME          VOLUME_ID+0
#define IN2_VOLUME          VOLUME_ID+1
#define STEREO_LINK_INP     INP_ID+3
#define EXP_CV_INP          INP_ID+2
#define CV_RANGE            INP_ID+4
#define EXP_MODE            INP_ID+5

//output sub menu
#define OUT1_VOLUME         VOLUME_ID+2
#define OUT2_VOLUME         VOLUME_ID+3
#define HP_VOLUME           VOLUME_ID+4
#define HP_MONITOR          OUTP_ID+1
#define STEREO_LINK_OUTP    OUTP_ID+3
#define HP_CV_OUTP          OUTP_ID+2
#define MASTER_VOL_LINK     OUTP_ID+4

//sync & midi sub menu
#define MIDI_CLK_SOURCE     MIDI_ID+2
#define MIDI_CLK_SEND       MIDI_ID+3
#define MIDI_SNAPSHOT       MIDI_ID+4
#define MIDI_PEDALBOARD     MIDI_ID+5
    
#define BLUETOOTH_DISCO_ID   BLUETOOTH_ID+2

#define DIALOG_ID           230

#define SYSTEM_MENU     \
    {"SETTINGS",                        MENU_LIST,      ROOT_ID,            -1,             NULL                       , 0},  \
    {"BANKS",                           MENU_SET,       BANKS_ID,           ROOT_ID,        system_banks_cb            , 0},  \
    {"CURRENT PEDALBOARD",              MENU_LIST,      PEDALBOARD_ID,      ROOT_ID,        NULL                       , 0},  \
    {"SAVE STATE",                      MENU_CONFIRM,   PEDALBOARD_SAVE_ID, PEDALBOARD_ID,  system_pedalboard_cb       , 0},  \
    {"RESET STATE",                     MENU_CONFIRM,   PEDALBOARD_RESET_ID,PEDALBOARD_ID,  system_pedalboard_cb       , 0},  \
    {"HARDWARE BYPASS",                 MENU_LIST,      BYPASS_ID,          ROOT_ID,        system_quick_bypass_cb     , 0},  \
    {"CHANNEL 1",                       MENU_TOGGLE,    BP1_ID,             BYPASS_ID,      system_bypass_cb           , 0},  \
    {"CHANNEL 2",                       MENU_TOGGLE,    BP2_ID,             BYPASS_ID,      system_bypass_cb           , 0},  \
    {"CHANNEL 1 & 2",                   MENU_TOGGLE,    BP12_ID,            BYPASS_ID,      system_bypass_cb           , 0},  \
    {"QUICK BYPASS CHANNEL(S)",         MENU_TOGGLE,    BP_SELECT_ID,       BYPASS_ID,      system_qbp_channel_cb      , 0},  \
    {"TEMPO & TRANSPORT",               MENU_LIST,      TEMPO_ID,           ROOT_ID,        system_play_cb             , 0},  \
    {"TEMPO",                           MENU_SET,       GLOBAL_TEMPO_ID,    TEMPO_ID,       system_tempo_cb            , 0},  \
    {"BEATS PER BAR",                   MENU_SET,       BEATS_PER_BAR_ID,   TEMPO_ID,       system_bpb_cb              , 0},  \
    {"TUNER",                           MENU_NONE,      TUNER_ID,           ROOT_ID,        system_tuner_cb            , 0},  \
    {"USER PROFILES",                   MENU_LIST,      PROFILES_ID,        ROOT_ID,        NULL                       , 0},  \
    {"USER PROFILE A",                  MENU_TOGGLE,    PROFILES_ID+1,      PROFILES_ID,    system_load_pro_cb         , 0},  \
    {"USER PROFILE B",                  MENU_TOGGLE,    PROFILES_ID+2,      PROFILES_ID,    system_load_pro_cb         , 0},  \
    {"USER PROFILE C",                  MENU_TOGGLE,    PROFILES_ID+3,      PROFILES_ID,    system_load_pro_cb         , 0},  \
    {"USER PROFILE D",                  MENU_TOGGLE,    PROFILES_ID+4,      PROFILES_ID,    system_load_pro_cb         , 0},  \
    {"OVERWRITE CURRENT PROFILE",       MENU_TOGGLE,    PROFILES_ID+5,      PROFILES_ID,    system_save_pro_cb         , 0},  \
    {"SYSTEM",                          MENU_LIST,      SYSTEM_ID,          ROOT_ID,        NULL                       , 0},  \
    {"INPUTS",                          MENU_LIST,      INP_ID,             SYSTEM_ID,      NULL                       , 0},  \
    {"< BACK TO SYSTEM",                MENU_RETURN,    INP_ID+1,           INP_ID,         NULL                       , 0},  \
    {"STEREO LINK",                     MENU_TOGGLE,    STEREO_LINK_INP,    INP_ID,         system_sl_in_cb            , 0},  \
    {"INPUT 1 GAIN",                    MENU_VOL,       IN1_VOLUME,         INP_ID,         system_volume_cb           , 0},  \
    {"INPUT 2 GAIN",                    MENU_VOL,       IN2_VOLUME,         INP_ID,         system_volume_cb           , 0},  \
    {"OUTPUTS",                         MENU_LIST,      OUTP_ID,            SYSTEM_ID,      NULL                       , 0},  \
    {"< BACK TO SYSTEM",                MENU_RETURN,    OUTP_ID+1,          OUTP_ID,        NULL                       , 0},  \
    {"STEREO LINK ",                    MENU_TOGGLE,    STEREO_LINK_OUTP,   OUTP_ID,        system_sl_out_cb           , 0},  \
    {"OUTPUT 1 VOLUME",                 MENU_VOL,       OUT1_VOLUME,        OUTP_ID,        system_volume_cb           , 0},  \
    {"OUTPUT 2 VOLUME",                 MENU_VOL,       OUT2_VOLUME,        OUTP_ID,        system_volume_cb           , 0},  \
    {"HEADPHONE VOLUME",                MENU_VOL,       HP_VOLUME,          OUTP_ID,        system_volume_cb           , 0},  \
    {"DIRECT HEADPHONE MONITOR",        MENU_TOGGLE,    HP_MONITOR,         OUTP_ID,        system_hp_bypass_cb        , 0},  \
    {"SYNC & MIDI",                     MENU_LIST,      MIDI_ID,            SYSTEM_ID,      NULL                       , 0},  \
    {"< BACK TO SYSTEM",                MENU_RETURN,    MIDI_ID+1,          MIDI_ID,        NULL                       , 0},  \
    {"CLOCK SOURCE",                    MENU_TOGGLE,    MIDI_CLK_SOURCE,    MIDI_ID,        system_midi_src_cb         , 0},  \
    {"SEND MIDI CLOCK",                 MENU_TOGGLE,    MIDI_CLK_SEND,      MIDI_ID,        system_midi_send_cb        , 0},  \
    {"SNAPSHOT NAV MIDI CHAN",          MENU_SET,       MIDI_SNAPSHOT,      MIDI_ID,        system_ss_prog_change_cb   , 0},  \
    {"PEDALBOARD NAV MIDI CHN",         MENU_SET,       MIDI_PEDALBOARD,    MIDI_ID,        system_pb_prog_change_cb   , 0},  \
    {"BLUETOOTH",                       MENU_LIST,      BLUETOOTH_ID,       SYSTEM_ID,      system_bluetooth_cb        , 1},  \
    {"< BACK TO SYSTEM",                MENU_RETURN,    BLUETOOTH_ID+1,     BLUETOOTH_ID,   NULL                       , 0},  \
    {"ENABLE DISCOVERY",                MENU_OK,        BLUETOOTH_DISCO_ID, BLUETOOTH_ID,   system_bluetooth_cb        , 0},  \
    {"STATUS:",                         MENU_NONE,      BLUETOOTH_ID+3,     BLUETOOTH_ID,   NULL                       , 0},  \
    {"NAME:",                           MENU_NONE,      BLUETOOTH_ID+4,     BLUETOOTH_ID,   NULL                       , 0},  \
    {"ADDRESS:",                        MENU_NONE,      BLUETOOTH_ID+5,     BLUETOOTH_ID,   NULL                       , 0},  \
    {"DISPLAY BRIGHTNESS",              MENU_TOGGLE,    DISP_BL_ID,         SYSTEM_ID,      system_display_cb          , 0},  \
    {"INFO",                            MENU_LIST,      INFO_ID,            SYSTEM_ID,      NULL                       , 0},  \
    {"< BACK TO SYSTEM",                MENU_RETURN,    INFO_ID+1,          INFO_ID,        NULL                       , 0},  \
    {"SERVICES",                        MENU_LIST,      SERVICES_ID,        INFO_ID,        system_services_cb         , 1},  \
    {"< BACK TO INFO",                  MENU_RETURN,    SERVICES_ID+1,      SERVICES_ID,    NULL                       , 0},  \
    {"JACK:",                           MENU_NONE,      SERVICES_ID+2,      SERVICES_ID,    NULL                       , 0},  \
    {"SSHD:",                           MENU_NONE,      SERVICES_ID+3,      SERVICES_ID,    NULL                       , 0},  \
    {"MOD-UI:",                         MENU_NONE,      SERVICES_ID+4,      SERVICES_ID,    NULL                       , 0},  \
    {"DNSMASQ:",                        MENU_NONE,      SERVICES_ID+5,      SERVICES_ID,    NULL                       , 0},  \
    {"VERSIONS",                        MENU_LIST,      VERSIONS_ID,        INFO_ID,        system_versions_cb         , 0},  \
    {"< BACK TO INFO",                  MENU_RETURN,    VERSIONS_ID+1,      VERSIONS_ID,    NULL                       , 0},  \
    {"VERSION:",                        MENU_NONE,      VERSIONS_ID+2,      VERSIONS_ID,    system_release_cb          , 0},  \
    {"RESTORE:",                        MENU_NONE,      VERSIONS_ID+3,      VERSIONS_ID,    NULL                       , 0},  \
    {"SYSTEM:",                         MENU_NONE,      VERSIONS_ID+4,      VERSIONS_ID,    NULL                       , 0},  \
    {"CONTROLLER:",                     MENU_NONE,      VERSIONS_ID+5,      VERSIONS_ID,    NULL                       , 0},  \
    {"DEVICE",                          MENU_LIST,      DEVICE_ID,          INFO_ID,        NULL                       , 0},  \
    {"< BACK TO INFO",                  MENU_RETURN,    DEVICE_ID+1,        DEVICE_ID,      NULL                       , 0},  \
    {"SERIAL NUMBER",                   MENU_OK,        DEVICE_ID+2,        DEVICE_ID,      system_tag_cb              , 0},  \
    {"SYSTEM UPGRADE",                  MENU_CONFIRM,   UPGRADE_ID,         SYSTEM_ID,      system_upgrade_cb          , 0},  \

//POPUP DEFINES
//PROFILE POPUP TXT
#define PROFILE_POPUP_LOAD_TXT    "The device is about to load a\nnew profile. Continue?"
#define PROFILE_POPUP_RELOAD_TXT  "Reload active user profile?\nThis will discard any unsaved\nchanges."
#define EXP_CV_POPUP_TXT          "The device is about to switch\ninput modes. To avoid damage,\ndisconnect all devices from\nthe CV/EXP port. Continue?"
#define HP_CV_POPUP_TXT           "The device is about to switch\noutput modes. To avoid damage,\ndisconnect all devices from\nthe CV/HP port. Continue?"
// popups text content, format : {menu_id, header_content, text_content}
#define POPUP_CONTENT   \
    {PEDALBOARD_ID, "pedalboard", "To access the selected menu\nitem, disconnect from the\ngraphical web interface."}, \
    {BANKS_ID, "Banks", "To access the selected menu\nitem, disconnect from the\ngraphical web interface."}, \
    {PEDALBOARD_SAVE_ID, "Save state", "Save current parameter values\nas the default for the active\npedalboard?"},         \
    {PEDALBOARD_RESET_ID, "Reset state", "Reset all parameter values to\nthe last saved state for the\nactive pedalboard?"},      \
    {BLUETOOTH_DISCO_ID, "Enable Bluetooth", "Bluetooth discovery mode is   now enabled for 2 minutes"},  \
    {UPGRADE_ID, "Start System Upgrade", "To start the system upgrade\nprocess, press and hold down\nthe leftmost button and press\nyes. "}, \
    {PROFILES_ID+1, "Load user profile A", PROFILE_POPUP_LOAD_TXT}, \
    {PROFILES_ID+2, "Load user profile B", PROFILE_POPUP_LOAD_TXT}, \
    {PROFILES_ID+3, "Load user profile C", PROFILE_POPUP_LOAD_TXT}, \
    {PROFILES_ID+4, "Load user profile D", PROFILE_POPUP_LOAD_TXT}, \
    {PROFILES_ID+5, "Overwrite user profile ", "Overwrite active user profile?"}, \
    {PROFILES_ID+6, "Reload user profile A", PROFILE_POPUP_RELOAD_TXT}, \
    {PROFILES_ID+7, "Reload user profile B", PROFILE_POPUP_RELOAD_TXT}, \
    {PROFILES_ID+8, "Reload user profile C", PROFILE_POPUP_RELOAD_TXT}, \
    {PROFILES_ID+9, "Reload user profile D", PROFILE_POPUP_RELOAD_TXT}, \
    {EXP_CV_INP, "Set input to EXP", EXP_CV_POPUP_TXT}, \
    {EXP_CV_INP+1, "Set input to CV",EXP_CV_POPUP_TXT}, \
    {HP_CV_OUTP, "Set output to HP", HP_CV_POPUP_TXT}, \
    {HP_CV_OUTP+1, "Set output to CV", HP_CV_POPUP_TXT}, \

#define MENU_LINE_CHARS     31

//// Foot functions leds colors
#define TOGGLED_COLOR               GREEN
#define TRIGGER_COLOR               GREEN
#define TRIGGER_PRESSED_COLOR       WHITE
#define TAP_TEMPO_COLOR             GREEN
#define ENUMERATED_COLOR            GREEN
#define ENUMERIATION_PRESSED_COLOR  WHITE
#define BYPASS_COLOR                RED
#define TRUE_BYPASS_COLOR           WHITE
#define PEDALBOARD_NEXT_COLOR       WHITE
#define PEDALBOARD_PREV_COLOR       WHITE
#define PEDALBOARD_LOADING_COLOR    GREEN
#define LED_LIST_COLOR_1            WHITE   
#define LED_LIST_COLOR_2            RED
#define LED_LIST_COLOR_3            YELLOW
#define LED_LIST_COLOR_4            CYAN
#define LED_LIST_COLOR_5            GREEN
#define LED_LIST_COLOR_6            MAGENTA
#define LED_LIST_COLOR_7            BLUE


#define LED_LIST_AMOUNT_OF_COLORS   7

//// Tap Tempo
// defines the time that the led will stay turned on (in milliseconds)
#define TAP_TEMPO_TIME_ON       100
// defines the default timeout value (in milliseconds)
#define TAP_TEMPO_DEFAULT_TIMEOUT 3000
// defines the difference in time the taps can have to be registered to the same sequence (in milliseconds)
#define TAP_TEMPO_TAP_HYSTERESIS 100
// defines the time (in milliseconds) that the tap can be over the maximum value to be registered
#define TAP_TEMPO_MAXVAL_OVERFLOW 50

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

//volume message timeout in a multiple of 500us
#define VOL_MESSAGE_TIMEOUT         200

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

#define DISPLAY_RIGHT 1
#define DISPLAY_LEFT  0

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
