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
*  FNET HTTP Server SSI API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_SSI_H_

#define _FNET_HTTP_SRV_SSI_H_

#if (FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_SSI)|| defined(__DOXYGEN__)

/*! @addtogroup fnet_http_srv
 @{ */

/**************************************************************************/ /*!
 * @brief SSI file extension. @n
 * All files that have this extension will be parsed by the SSI parser.
 * @showinitializer
 ******************************************************************************/
#define FNET_HTTP_SRV_SSI_EXTENSION     "shtml"

/**************************************************************************/ /*!
 * @brief Callback function prototype of the SSI parameters handler.
 *
 * @param query     SSI directive parameter string (null-terminated).@n
 *                  The parameter string is set to whatever appears
 *                  between SSI command tag and @c --\> in the SSI directive itself.
 * @param cookie    This parameter points to the value, initially set to zero,
 *                  which can be used to associate a custom information
 *                  with a connection instance. If application store context
 *                  information in the @c cookie, it will be preserved
 *                  for future calls for this request. This allows the
 *                  application to associate some request-specific state.
 *
 * @return This function must return:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     SSI directive will be eliminated from the result HTTP page.
 *
 * @see fnet_http_srv_ssi_t, fnet_http_srv_ssi_send_t
 *
 * The SSI parser invokes this callback function, when it meets SSI directive
 * in the HTML file and the SSI command name corresponds to the name
 * registered in the SSI table.@n
 * The @c query points to the SSI parameters string.
 * If SSI directive does not have any parameters, the @c query points to the
 * blank string.
 *
 ******************************************************************************/
typedef fnet_return_t(*fnet_http_srv_ssi_handle_t)(fnet_char_t *query, fnet_uint32_t *cookie);

/**************************************************************************/ /*!
 * @brief Callback function prototype of the SSI include function.
 *
 * @param buffer           Output buffer where SSI content will be copied to.
 *
 * @param buffer_size      Size of the output @c buffer.
 *
 * @param eof              Condition flag:
 *                         - @c FNET_FALSE =  there is still more data to include.
 *                           The @ref fnet_http_srv_ssi_send_t function will be called
 *                           during the next iteration again.
 *                         - @c FNET_TRUE =  no more data is available for the SSI to include.
 *                           It was the last call of the @ref fnet_http_srv_ssi_send_t
 *                           function for the current SSI.
 * @param cookie    This parameter points to the value,
 *                  which can be used to associate a custom information
 *                  with a connection instance. If application store context
 *                  information in the @c cookie, it will be preserved
 *                  for future calls for this request. This allows the
 *                  application to associate some request-specific state.
 *
 * @return This function returns the number of bytes actually written to the buffer,
 *         pointed to by @c buffer.@n
 *         The condition flag @c eof indicates, if the SSI end condition has occurred.
 *
 * @see fnet_http_srv_ssi_t, fnet_http_srv_ssi_handle_t
 *
 * This function creates SSI dynamic content.@n
 * An application should use the @c buffer as output buffer for the dynamic content
 * and set @c eof flag to @c 1 if no data are available to include.@n
 * The SSI parser invokes this callback function after successful call of the
 * @ref fnet_http_srv_ssi_handle_t function and continues to call this function repeatedly,
 * till the @c eof will be set to @c 1 or the return result is set to @c 0.
 *
 ******************************************************************************/
typedef fnet_size_t (*fnet_http_srv_ssi_send_t)(fnet_uint8_t *buffer, fnet_size_t buffer_size, fnet_bool_t *eof, fnet_uint32_t *cookie);

/**************************************************************************/ /*!
 * @brief SSI callback function table.
 *
 * The last table element must have all fields set to zero as the end-of-table mark.@n
 * @n
 * SSI (Server Side Includes) are directives that are placed in HTML pages,
 * and evaluated on the server, while the pages are being served.
 * They let a web server application add dynamically-generated content to
 * an existing HTML page.@n
 * SSI directives have the following format:@n
 *      @code <!--#command [parameter(s)]--> @endcode @n
 * There should be no spaces between the @c \<!--  and the @c #.@n
 * If, for any reason, a document containing SSI directives is served to
 * the client unparsed, the HTML comment format means the directive's
 * coding will not be visible.
 *
 * @see fnet_http_srv_params_t
 ******************************************************************************/
typedef struct
{
    fnet_char_t             *name;	    /**< @brief SSI command name. */
    fnet_http_srv_ssi_handle_t  handle;     /**< @brief Pointer to the SSI parameters
	                                     *   handler. It's optional. */
    fnet_http_srv_ssi_send_t    send;       /**< @brief Pointer to the SSI include function.
                                         * This function actually inserts dynamic content to
                                         * an existing HTML page. It's optional. */
} fnet_http_srv_ssi_t;
/*! @} */

#endif /* FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_SSI */

#endif
