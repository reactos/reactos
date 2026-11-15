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
*  FNET Telnet Server API.
*
***************************************************************************/

#ifndef _FNET_TELNET_H_

#define _FNET_TELNET_H_

#if FNET_CFG_TELNET || defined(__DOXYGEN__)

/*! @addtogroup fnet_telnet
* The Telnet server provides a simple command-line interface for a
* remote host via a virtual terminal connection. @n
* Current version of the Telnet server supports maximum of one simultaneously
* connected Telnet client.@n
* @n
* After the FNET Telnet server is initialized by calling the @ref fnet_telnet_init()
* function, the user application should call the main service polling function
* @ref fnet_service_poll() periodically in background. @n
*
* For example:
* @code
*
*...
*
* const fnet_shell_t fapp_telnet_shell =
* {
*    fapp_telnet_cmd_table,
*    (sizeof(fapp_telnet_cmd_table) / sizeof(fnet_shell_command_t)),
*    "TELNET>",
*    fapp_shell_on_init,
* };
*
*...
*
* void main()
* {
*    fnet_telnet_params_t params;
*
*    ...
*
*    params.ip_address = FNET_HTONL(INADDR_ANY);
*    params.port = FNET_HTONS(0);       //Default port number.
*    params.shell= &fapp_telnet_shell;
*    params.cmd_line_buffer = fapp_telnet_cmd_line_buffer;
*    params.cmd_line_buffer_size = sizeof(fapp_telnet_cmd_line_buffer)
*
*    // Init Telnet server.
*    fapp_telnet_desc = fnet_telnet_init(&params);
*    if(fapp_telnet_desc)
*    {
*        fnet_printf("\n FNET Telnet server started.\n");
*        while(1)
*        {
*           // Do something.
*           ...
*           fnet_service_poll();
*        }
*    }
*    else
*    {
*        fnet_printf("\n FNET Telnet server initialization is failed.\n");
*    }
*
* }
* @endcode
* For Telnet usage example, refer to FNET demo application source code.@n
* @n
* Configuration parameters:
* - @ref FNET_CFG_TELNET
* - @ref FNET_CFG_TELNET_SESSION_MAX
* - @ref FNET_CFG_TELNET_PORT
* - @ref FNET_CFG_TELNET_SHELL_ECHO
* - @ref FNET_CFG_TELNET_SOCKET_BUF_SIZE
*/
/*! @{ */

/**************************************************************************/ /*!
 * @brief Input parameters for @ref fnet_telnet_init().
 ******************************************************************************/
typedef struct
{
    struct fnet_sockaddr address;    /**<  @brief Server socket address. @n
                                    * If server IP address is set to @c 0s, the server will listen to all current network interfaces. @n
                                    * If server address family is set to @c 0, it will be assigned to @ref AF_SUPPORTED. @n
                                    * If server port number is set to @c 0, it will be assigned to the default port number defined by @ref FNET_CFG_TELNET_PORT.*/
    const fnet_shell_t *shell;     /**< @brief Root-shell structure. */
} fnet_telnet_params_t;

/**************************************************************************/ /*!
 * @brief Telnet server descriptor.
 * @see fnet_telnet_init()
 ******************************************************************************/
typedef void *fnet_telnet_desc_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the Telnet Server service.
 *
 * @param params     Initialization parameters defined by @ref fnet_telnet_params_t.
 *
 * @return This function returns:
 *   - Telnet server descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_telnet_release()
 *
 ******************************************************************************
 *
 * This function initializes the Telnet server service. It allocates all
 * resources needed, and registers the Telnet server service in the polling list.@n
 * After the initialization, the user application should call the main polling
 * function  @ref fnet_service_poll() periodically to run the Telnet server in background.@n
 * The Telnet service executes parsing of user-entered commands received via
 * the Telnet protocol, and calls user-defined command functions,
 * if the parsing was successful.
 *
 ******************************************************************************/
fnet_telnet_desc_t fnet_telnet_init( fnet_telnet_params_t *params);

/***************************************************************************/ /*!
 *
 * @brief    Releases the Telnet Server service.
 *
 * @param desc     Telnet server descriptor to be unregistered.
 *
 * @see fnet_telnet_init()
 *
 ******************************************************************************
 *
 * This function releases the Telnet Server assigned to the @c desc
 * descriptor.@n
 * It releases all occupied resources, and unregisters the Telnet service from
 * the polling list.
 *
 ******************************************************************************/
void fnet_telnet_release(fnet_telnet_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Closes the currently active session of the Telnet Server.
 *
 * @param desc     Telnet server descriptor
 *
 ******************************************************************************
 *
 * This function closes the current Telnet session.@n
 * It can be used in a Telnet user-command to close the current
 * session. This is the alternative to closure of the Telnet-client terminal applicatioin.
 *
 ******************************************************************************/
void fnet_telnet_close_session(fnet_telnet_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the Telnet Server service is enabled or disabled.
 *
 * @param desc     Telnet server descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the Telnet Server is successfully initialized.
 *          - @ref FNET_FALSE if the Telnet Server is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the Telnet Server service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_telnet_is_enabled(fnet_telnet_desc_t desc);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif  /* FNET_CFG_TELNET */

#endif  /* _FNET_TELNET_H_ */
