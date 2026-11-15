/**************************************************************************
*
* Copyright 2017 by Andrey Butok. FNET Community.
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
*  SNTP Client configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_SNTP_CONFIG_H_

#define _FNET_SNTP_CONFIG_H_

/*! @addtogroup fnet_sntp_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_SNTP
 * @brief    SNTP client support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_SNTP
    #define FNET_CFG_SNTP                          (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_SNTP_PORT
 * @brief   Default SNTP port number (in network byte order).@n
 *          The SNTP client uses this port for sending and receiving of SNTP
 *          messages. @n
 *          It can be changed during the SNTP Client initialization by the
 *          @ref fnet_sntp_init() function.@n
 *          Default value is FNET_HTONS(123).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SNTP_PORT
    #define FNET_CFG_SNTP_PORT                       (FNET_HTONS(123U))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_SNTP_RETRANSMISSION_MAX
 * @brief   The maximum number of times the SNTP client will retransmit
 *          the request message to a SNTP server.@n
 *          Default value is @b @c 2.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SNTP_RETRANSMISSION_MAX
    #define FNET_CFG_SNTP_RETRANSMISSION_MAX         (2U)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_SNTP_RETRANSMISSION_TIMEOUT
 * @brief   Timeout for the response from the remote SNTP server (in seconds).@n
 *          @n
 *          If the SNTP client does not receive any response from a SNTP server,
 *          it sends new request message.@n
 *          Default value is @b @c 2.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SNTP_RETRANSMISSION_TIMEOUT
    #define FNET_CFG_SNTP_RETRANSMISSION_TIMEOUT     (2U)  /* seconds */
#endif

/*! @} */

#endif /* _FNET_SNTP_CONFIG_H_ */
