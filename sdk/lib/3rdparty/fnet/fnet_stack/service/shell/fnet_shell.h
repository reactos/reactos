/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  FNET Shell service API.
*
***************************************************************************/
#ifndef _FNET_SHELL_H_

#define _FNET_SHELL_H_

/*! @addtogroup fnet_shell
* The shell service provides application-specific command-line interface.@n
* It runs on asigned serial stream, parses the input text, decodes the command
* name and arguments and calls back user-specified functions assigned to
* each command. @n
* @n
* After the Shell service is initialized by calling the @ref fnet_shell_init()
* function, the user application should call the main service polling function
* @ref fnet_service_poll() periodically in background. @n
* @n
* For example:
* @code
* ...
* // ************************************************************************
* // *     The table of the main shell commands.
* // *************************************************************************
*  static const fnet_shell_command_t fapp_cmd_tab [] =
*  {
*      { "?", 0, 0, fnet_shell_help_cmd,"Display this help message.", ""},
*      { "set", 0, 2, fapp_set_cmd, "Set parameter.", "[<parameter> <value>]"},
*      { "show", 0, 0, fapp_show, "Show parameters.", "" },
*      { "info", 0, 0, fapp_netif_info_cmd, "Show interface info.", ""},
*      { "dhcp", 0, 1, fapp_dhcp_cln_cmd, "Start DHCP client.", "[release|reboot]"},
*      { "http", 0, 1, fapp_http_srv_cmd, "Start HTTP Server.", "[release]"},
*      { 0, 0, 0, 0, 0, 0},
*  };
* // ************************************************************************
* // *     The main shell control data structure.
* // *************************************************************************
* fnet_shell_t fapp_shell =
* {
*   fapp_cmd_tab,
*   "PROMT> ",
*   fapp_shell_on_init,
* };
* fnet_shell_desc_t fapp_shell_desc = 0; // Shell descriptor.
* ...
* ...
* main()
* {
*   fnet_shell_params_t shell_params;
*   ...
*   shell_params.shell = &fapp_shell;
*   shell_params.cmd_line_buffer = fapp_cmd_line_buffer;
*   shell_params.cmd_line_buffer_size = sizeof(fapp_cmd_line_buffer);
*   shell_params.stream = FNET_SERIAL_STREAM_DEFAULT; // Use default stream.
*   shell_params.echo = FNET_TRUE; // Enable echo.
*   ...
*   if((fapp_shell_desc = fnet_shell_init(&shell_params)) != 0)
*   {
*       fnet_printf("\n Shell started.\n");
*       while(1)
*       {
*           // Do something.
*           ...
*           fnet_service_poll();
*       }
*    }
*    else
*    {
*        fnet_printf("\n Shell initialization is failed.\n");
*    }
*   ...
* }
* @endcode
*
* For Shell service usage example, refer to FNET demo application source code.@n
* @n
* Configuration parameters:
* - @ref FNET_CFG_SHELL_MAX
* - @ref FNET_CFG_SHELL_ARGS_MAX
* - @ref FNET_CFG_SHELL_HELP_FORMAT
*/
/*! @{ */

/**************************************************************************/ /*!
 * @brief The quote (@c ') symbol is used to combine multiple words
 *        to one argument.
 * @showinitializer
 ******************************************************************************/
#define FNET_SHELL_QUOTE_SYMBOL      '\''

/**************************************************************************/ /*!
 * @brief The escape symbol (@c \).
 *
 * The escape character (@c \) provides the way to include the
 * @ref FNET_SHELL_QUOTE_SYMBOL quote symbol and
 * the @ref FNET_SHELL_COMMAND_SPLITTER split symbol
 * inside a string argument.@n
 * For example, to set the @c bootscript parameter to the value:
 * @code
 * dhcp ; tftp; set bootscript 'tftp\; go 0x20000'
 * @endcode
 * In the shell prompt, you should enter:
 * @code
 * FNET> set bootscript `dhcp \; info\; set bootscrpt \` tftp\\; go 0x20000\``
 * @endcode
 * @showinitializer
 ******************************************************************************/
#define FNET_SHELL_ESCAPE_SYMBOL      '\\'

/**************************************************************************/ /*!
 * @brief Split symbol used to split shell commands. @n
 *        User may enter several commands in one command line, but they must
 *        be split by this symbol.
 * @showinitializer
 ******************************************************************************/
#define FNET_SHELL_COMMAND_SPLITTER          ';'

/**************************************************************************/ /*!
 * @brief Shell states.@n
 * Used mainly for debugging purposes.
 ******************************************************************************/
typedef enum
{
    FNET_SHELL_STATE_DISABLED = 0,      /**< @brief The Shell service is not initialized.*/
    FNET_SHELL_STATE_INIT,              /**< @brief The Shell service is not initialized.*/
    FNET_SHELL_STATE_GET_USER_INPUT,    /**< @brief The Shell service is accepting user commnads.*/
    FNET_SHELL_STATE_EXEC_CMD,          /**< @brief The Shell service is executing user commnads.*/
    FNET_SHELL_STATE_BLOCKED,           /**< @brief The Shell service is blocked and ignores user commnads.*/
    FNET_SHELL_STATE_END_CMD            /**< @brief The Shell service finished command execution.*/
} fnet_shell_state_t;

/**************************************************************************/ /*!
 * @brief Shell service descriptor.
 * @see fnet_shell_init()
 ******************************************************************************/
typedef void *fnet_shell_desc_t;

/**************************************************************************/ /*!
 * @brief Command callback function prototype.
 *
 * @param desc      Shell descriptor.
 *
 * @param srgc      This parameter is a count of the arguments supplied to
 *                  the command function. @n
 *                  It is equal to @c 1 when the command was entered without
 *                  any argument.
 *
 * @param srgv      This parameter is an array of pointers to the strings
 *                  which are the command arguments.
 *                  The first array element points to the command name.
 *
 ******************************************************************************/
typedef void(*fnet_shell_cmd_function_t)( fnet_shell_desc_t desc, fnet_index_t argc, fnet_char_t **argv );

/**************************************************************************/ /*!
 * @brief Shell command control structure.
 *
 * This structure is used to define properties of a command that will be
 * supported by the shell.@n
 * An application should define the command table and pass it to the
 * @c fnet_shell_t structure. @n
 * The last table element must have all fields
 * set to zero as the end-of-table mark.
 * The good example of @ref fnet_shell_command_t usage is in the FNET Shell application.
 *
 * @see fnet_shell_t
 ******************************************************************************/
typedef struct
{
    const fnet_char_t           *name;          /**< @brief Command name (null-terminated string). */
    fnet_index_t                min_args;       /**< @brief Minimum number of arguments the command accepts.*/
    fnet_index_t                max_args;       /**< @brief Maximum number of arguments the command accepts.*/
    fnet_shell_cmd_function_t   cmd_ptr;        /**< @brief Pointer to the actual command function defined by
                                                *   the @ref fnet_shell_cmd_function_t type    */
    fnet_char_t                 *description;   /**< @brief Brief description of the command (null-terminated string). @n
                                                * This field is used by the @ref fnet_shell_help() function.@n*/
    fnet_char_t                 *syntax;        /**< @brief Syntax of the command (null-terminated string). @n
                                                * This field is used by the @ref fnet_shell_help() function.
                                                * The standard command line syntax information
                                                * which will be helpful to describe the possible command
                                                * line parameters in a help display is:
                                                * - @c [] = When a parameter is surrounded with square
                                                * brackets, this means the parameter is optional.
                                                * - @c <> = When a parameter is surrounded with angle
                                                * brackets, this means the parameter is required for
                                                * normal operations of command.
                                                * - @c | = The vertical bar means a choice between
                                                * parameter value is acceptable.*/
} fnet_shell_command_t;

/**************************************************************************/ /*!
 * @brief Shell main control structure.
 *
 * This structure defines shell-specific parameters.@n
 * The good example of @ref fnet_shell_t usage is in the FNET Shell application.
 *
 * @see fnet_shell_params_t
 ******************************************************************************/
typedef struct
{
    const fnet_shell_command_t      *cmd_table;     /**< @brief The pointer to the command table.@n
                                                    * The last table element must have all fields
                                                    * set to zero as the end-of-table mark.*/
    fnet_char_t                     *prompt_str;    /**< @brief Shell prompt (null-terminated string).*/
    void (*shell_init)( fnet_shell_desc_t shell_desc );/**< @brief Routine called during the shell initialization.
                                                    * It's called by the @ref fnet_shell_init() function.@n
                                                    * This parameter is optional and can be set to @c 0.*/
} fnet_shell_t;


/**************************************************************************/ /*!
 * @brief Input parameters for @ref fnet_shell_init().
 ******************************************************************************/
typedef struct
{
    const fnet_shell_t      *shell;                 /**< @brief Shell control structure. */
    fnet_char_t             *cmd_line_buffer;       /**< @brief Command-line buffer. */
    fnet_size_t             cmd_line_buffer_size;   /**< @brief Size of the command-line buffer.
                                                    * It defines the maximum length of the
                                                    * entered input-command line. */
    fnet_serial_stream_t    stream;                 /**< @brief Serial stream assigned to the shell. */
    fnet_bool_t             echo;                   /**< @brief Enable/disable terminal echo flag.
                                                    * When set to @c FNET_TRUE the echo is enabled,
                                                    * characters received by the shell are echoed
                                                    * back to the terminal.@n
                                                    * When set to @c FNET_FALSE the echo is disabled,
                                                    * characters are transferred to the terminal
                                                    * without echoing them to the terminal display. */
} fnet_shell_params_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the Shell service.
 *
 * @param params     Initialization parameters defined by @ref fnet_shell_params_t.
 *
 * @return This function returns:
 *   - Shell service descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_shell_release()
 *
 ******************************************************************************
 *
 * This function executes the initialization of the shell service according to
 * settings pointed to by @c params parameters. It allocates all
 * resources needed, and registers the shell service in the polling list.@n
 * After the initialization, the user application should call the main polling
 * function  @ref fnet_service_poll() periodically to run the Shell service in background.@n
 * The shell service runs on asigned serial stream, executes parsing of
 * the user-entered commands, and calls user-defined command functions,
 * if the parsing was successful.
 *
 ******************************************************************************/
fnet_shell_desc_t fnet_shell_init( fnet_shell_params_t *params);

/***************************************************************************/ /*!
 *
 * @brief    Releases the Shell service.
 *
 * @param desc     Shell service descriptor to be unregistered.
 *
 * @see fnet_shell_init()
 *
 ******************************************************************************
 *
 * This function releases the Shell service assigned to the @c desc
 * descriptor.@n
 * It releases all occupied resources, and unregisters the Shell service from
 * the polling list.
 *
 ******************************************************************************/
void fnet_shell_release(fnet_shell_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Prints the command-shell help message.
 *
 * @param desc   Shell service descriptor.
 *
 * @see fnet_shell_cmd_function_t
 *
 ******************************************************************************
 *
 * This function prints the list of commands supported by the shell,
 * defined by the @c desc descriptor.
 * For example:
 * @code
 * ...
 *  static const fnet_shell_command_t fapp_cmd_table [] =
 *  {
 *      { FNET_SHELL_CMD_TYPE_NORMAL, "?", 0, 0, fapp_help_cmd,"Display this help message.", ""},
 *      { FNET_SHELL_CMD_TYPE_NORMAL, "set", 0, 2, fapp_set_cmd,      "Set parameter.", "[<parameter> <value>]"},
 *      { FNET_SHELL_CMD_TYPE_NORMAL, "get", 0, 0, fapp_show,    "Get parameters.", "[<parameter>]" },
 *      { FNET_SHELL_CMD_TYPE_NORMAL, "info", 0, 0, fapp_netif_info_cmd,    "Show interface info.", ""},
 *      { FNET_SHELL_CMD_TYPE_NORMAL, "dhcp", 0, 1, fapp_dhcp_cln_cmd,    "Start DHCP client.", "[release|reboot]"},
 *      { FNET_SHELL_CMD_TYPE_NORMAL, "http", 0, 1, fapp_http_srv_cmd,    "Start HTTP Server.", "[release]"},
 *      { FNET_SHELL_CMD_TYPE_SHELL, "exp", 0, 1, &fapp_fs_shell,   "File Explorer submenu...", ""},
 *  };
 * ...
 * @endcode
 * Calling of the @ref fnet_shell_help() for the @c fapp_cmd_table structure prints:
 * @verbatim
>      ?                                 - Display this help message
>    set [<parameter> <value>]           - Set parameter
>    get [<parameter>]                   - Get parameters
>   info                                 - Show detailed status
>   dhcp [release]                       - Start DHCP client
>   http [release]                       - Start HTTP Server
>    exp                                 - File Explorer submenu...
 @endverbatim
 *
 ******************************************************************************/
void fnet_shell_help( fnet_shell_desc_t desc);


/***************************************************************************/ /*!
 *
 * @brief    Executes the command line script.
 *
 * @param desc      Shell service descriptor.
 *
 * @param script    Command line string (null-terminated) to be executed by the shell.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs, e.g. a wrong parameter or too big length of the script.
 *
 * @see fnet_shell_script_stop
 *
 ******************************************************************************
 *
 * This function executes a predefined command-line script pointed by the @c script. @n
 * The script string may have several commands but they must be split by
 * the @ref FNET_SHELL_COMMAND_SPLITTER symbol.
 *
 ******************************************************************************/
fnet_return_t fnet_shell_script(fnet_shell_desc_t desc, fnet_char_t *script );

/***************************************************************************/ /*!
 *
 * @brief    Stops execution of the shell command line.
 *
 * @param desc      Shell service descriptor.
 *
 ******************************************************************************
 *
 * This function stops execution of the all shell commands that are waiting
 * for execution. @n
 * This function is usually called when the execution of a critical command is
 * failed and no sense to execute the rest of the commands that were entered
 * by a user in one command line or executed by the @ref fnet_shell_script()
 * function.
 *
 ******************************************************************************/
void fnet_shell_script_stop( fnet_shell_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Blocks the shell to ignore user commands.
 *
 * @param desc      Shell service descriptor.
 *
 * @param on_ctrlc  Pointer to the callback function called on the [Ctrl]+[c]
 *                  button pressing. This parameter is optional and can be set to @ref FNET_NULL.
 *
 * @param cookie    Optional application-specific parameter. @n
 *                  It's passed to the @c on_ctrlc function as input parameter.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_shell_unblock
 *
 ******************************************************************************
 *
 * This function moves the shell to the blocking state.
 * In this state the shell ignore any commands entered by a user into a terminal
 * console. @n
 * The shell can be unblocked by the @ref fnet_shell_unblock() function.
 * Also the shell may be unblocked by pressing the [Ctrl]+[c] button combination
 * in a terminal console, after that the optional callback function,
 * pointed by the @c on_ctrlc() parameter, will be called.
 *
 ******************************************************************************/
fnet_return_t fnet_shell_block( fnet_shell_desc_t desc, void (*on_ctrlc)(fnet_shell_desc_t shl_desc, void *cookie), void *cookie);

/***************************************************************************/ /*!
 *
 * @brief    Unblocks the shell to accept user commands.
 *
 * @param desc      Shell service descriptor.
 *
 * @see fnet_shell_block
 *
 ******************************************************************************
 *
 * This function moves the shell from the blocked state to the normal state.
 * In this state the shell accepts all supported commands.@n
 * @n
 * NOTE: Also the shell may be unblocked by a user by pressing the [Ctrl]+[c]
 * buttons in a terminal console.
 *
 ******************************************************************************/
void fnet_shell_unblock( fnet_shell_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Prints formatted text to the shell stream.
 *
 * @param desc      Shell service descriptor.
 *
 * @param format       Format string.
 *
 * @return This function returns the number of characters that were
 *         successfully output.
 *
 * @see fnet_shell_putchar(), fnet_shell_getchar(), fnet_shell_println()
 *
 ******************************************************************************
 *
 * This function outputs formatted text to the shell stream. @n
 * The function takes one or more arguments. The first argument is a string parameter
 * called the "format string".
 * The optional arguments following @c format are items
 * (integers, characters or strings) that are to be converted to character strings
 * and inserted into the output of @c format at specified points.@n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flasgs can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 * @note
 * To save some bytes from all the hard coded strings the (s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");@n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_shell_printf(fnet_shell_desc_t desc, const fnet_char_t *format, ... );

/***************************************************************************/ /*!
 *
 * @brief    Prints formatted text to the shell stream and terminates the
 *           printed text by the line separator string.
 *
 * @param desc      Shell service descriptor.
 *
 * @param format       Format string.
 *
 * @return This function returns the number of characters that were
 *         successfully output.
 *
 * @see fnet_shell_putchar(), fnet_shell_getchar(), fnet_shell_printf()
 *
 ******************************************************************************
 *
 * This function outputs formatted text to the shell stream and terminates the
 * printed text by the line separator string. @n
 * The function takes one or more arguments. The first argument is a string parameter
 * called the "format string".
 * The optional arguments following @c format are items
 * (integers, characters or strings) that are to be converted to character strings
 * and inserted into the output of @c format at specified points.@n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flasgs can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 * @note
 * To save some bytes from all the hard coded strings the (s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");@n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_shell_println(fnet_shell_desc_t desc, const fnet_char_t *format, ... );

/***************************************************************************/ /*!
 *
 * @brief    Writes character to the shell stream.
 *
 * @param desc          Shell service descriptor.
 *
 * @param character     Character to be written.
 *
 * @see fnet_shell_getchar()
 *
 ******************************************************************************
 *
 * This function writes a single @c character to the shell stream.
 *
 ******************************************************************************/
void fnet_shell_putchar(fnet_shell_desc_t desc, fnet_char_t character);

/***************************************************************************/ /*!
 *
 * @brief    Reads character from the shell stream.
 *
 * @param desc          Shell service descriptor.
 *
 * @return This function returns:
 *          - character from the shell stream.
 *          - FNET_ERR if no charcter is available in the shell stream.
 *
 * @see fnet_shell_putchar()
 *
 ******************************************************************************
 *
 * This function reads a single character from the shell stream. @n
 * If no character is availble in the shell stream the FNET_ERR is returned.
 *
 ******************************************************************************/
fnet_int32_t fnet_shell_getchar(fnet_shell_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Switch to other command line set.
 *
 * @param desc          Shell service descriptor.
 * @param switch_shell  Other shell control structure to be switched to.@n
 *                      If FNET_NULL, it will be switched to original command line set.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs (desc is zero). @n
 *
 * @see fnet_shell_init()
 *
 ******************************************************************************
 *
 * This function switches to other command line set defined by @c switch_shell.
 * If @c switch_shell is zero, it switches to original command line set.
 *
 ******************************************************************************/
fnet_return_t fnet_shell_switch( fnet_shell_desc_t desc, const fnet_shell_t *switch_shell);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the [Ctrl]+[c] is received.
 *
 * @return This function returns:
 *   - @c 0 if [Ctrl]+[c] is not received/pressed.
 *   - @c 1 if [Ctrl]+[c] is received/pressed.
 *
 ******************************************************************************
 *
 * This function detects if the [Ctrl]+[c] command is received.@n
 * It can be used by blocking shell commands to detect that a user wants to
 * terminate the command activity.
 *
 ******************************************************************************/
fnet_bool_t fnet_shell_is_ctrlc (fnet_shell_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Looks for a shell command by its name.
 *
 * @param desc        Shell service descriptor.
 * @param name        Command name (null-terminated string).
 *
 * @return This function returns:
 *          - Pointer to the found shell command control structure.
 *          - @ref FNET_NULL if there is no any command with specified @c name.
 *
 * @see fnet_shell_command_t
 *
 ******************************************************************************
 *
 * This function is looking for a shell command by its name.
 *
 ******************************************************************************/
const fnet_shell_command_t *fnet_shell_get_command_by_name(fnet_shell_desc_t desc, const fnet_char_t *name);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif
