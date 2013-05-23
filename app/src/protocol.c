
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <string.h>

#include "protocol.h"
#include "utils.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define NOT_FOUND           (-1)
#define MANY_ARGUMENTS      (-2)
#define FEW_ARGUMENTS       (-3)
#define INVALID_ARGUMENT    (-4)


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

const char *g_error_messages[] = {
    MESSAGE_COMMAND_NOT_FOUND,
    MESSAGE_MANY_ARGUMENTS,
    MESSAGE_FEW_ARGUMENTS,
    MESSAGE_INVALID_ARGUMENT
};


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

typedef struct CMD_T {
    char** list;
    uint32_t count;
    void (*callback)(proto_t *proto);
} cmd_t;


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

static unsigned int g_index = 0;
static cmd_t g_commands[PROTOCOL_MAX_COMMANDS];


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static int not_is_wildcard(const char *str);


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

static int not_is_wildcard(const char *str)
{
    if (str && strchr(str, '%') == NULL) return 1;
    return 0;
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void protocol_parse(msg_t *msg)
{
    uint32_t i, j;
    int32_t index = NOT_FOUND;
    proto_t proto;

    proto.list = string_split(msg->data, ' ');
    proto.list_count = array_length(proto.list);
    proto.response = NULL;

    /* TODO: few arguments, many arguments, check invalid argument (wildcards) */

    if (proto.list_count > 0)
    {
        for (i = 0; i < g_index; i++)
        {
            /* Checks if counters match */
            if (proto.list_count == g_commands[i].count)
            {
                index = i;

                /* Checks the commands */
                for (j = 0; j < proto.list_count; j++)
                {
                    if (not_is_wildcard(g_commands[i].list[j]) &&
                        strcmp(g_commands[i].list[j], proto.list[j]) != 0)
                    {
                        index = NOT_FOUND;
                        break;
                    }
                }

                if (index >= 0) break;
            }
        }

        /* Protocol OK */
        if (index >= 0)
        {
            if (g_commands[index].callback)
            {
                g_commands[index].callback(&proto);

                if (proto.response)
                {
                    SEND_TO_SENDER(msg->sender_id, proto.response, proto.response_size);
                    FREE(proto.response);
                }
            }
        }
        /* Protocol error */
        else
        {
            SEND_TO_SENDER(msg->sender_id, g_error_messages[-index-1], strlen(g_error_messages[-index-1]));
        }

        FREE(proto.list);
    }
}


void protocol_add_command(const char *command, void (*callback)(proto_t *proto))
{
    if (g_index >= PROTOCOL_MAX_COMMANDS) while (1);

    char *cmd = strdup(command);
    g_commands[g_index].list = string_split(cmd, ' ');
    g_commands[g_index].count = array_length(g_commands[g_index].list);
    g_commands[g_index].callback = callback;
    g_index++;
}


void protocol_response(const char *response, proto_t *proto)
{
    proto->response_size = strlen(response);
    proto->response = MALLOC(proto->response_size + 1);
    strcpy(proto->response, response);
}
