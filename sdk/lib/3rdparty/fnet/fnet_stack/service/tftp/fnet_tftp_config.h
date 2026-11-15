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
*  TFTP service configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_TFTP_CONFIG_H_

#define _FNET_TFTP_CONFIG_H_

/*! @addtogroup fnet_tftp_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_TFTP_CLN
 * @brief    TFTP Client support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_CLN
    #define FNET_CFG_TFTP_CLN   (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TFTP_SRV
 * @brief    TFTP Server support:
 *               - @c 1..n = is enabled. Its value defines maximum number of
 *                   the TFTP Servers that can be run simultaneously.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_SRV
    #define FNET_CFG_TFTP_SRV   (0)
#endif

/******************************************************************************
 *              TFTP-client service config parameters
 ******************************************************************************/

/**************************************************************************/ /*!
 * @def     FNET_CFG_TFTP_CLN_PORT
 * @brief   TFTP server port number (in network byte order) used by TFTP-client
 *          service. @n
 *          Default value is FNET_HTONS(69).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_CLN_PORT
    #define FNET_CFG_TFTP_CLN_PORT              (FNET_HTONS(69u))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TFTP_CLN_TIMEOUT
 * @brief   Timeout for TFTP server response in seconds. @n
 *          If no response from a TFTP server is received during this timeout,
 *          the TFTP-client service is released automatically.@n
 *          Default value is @b @c 10.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_CLN_TIMEOUT
    #define FNET_CFG_TFTP_CLN_TIMEOUT           (10u) /*sec*/
#endif

/******************************************************************************
 *              TFTP-server service config parameters
 ******************************************************************************/

/**************************************************************************/ /*!
 * @def     FNET_CFG_TFTP_SRV_PORT
 * @brief   Default TFTP server port number (in network byte order) used by TFTP-server service.@n
 *          It can be changed during the TFTP server initialization by the
 *          @ref fnet_tftp_srv_init() function. @n
 *          Default value is FNET_HTONS(69).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_SRV_PORT
    #define FNET_CFG_TFTP_SRV_PORT              (FNET_HTONS(69u))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TFTP_SRV_TIMEOUT
 * @brief   Default timeout for TFTP client response in seconds. @n
 *          If no response from a TFTP client is received during this timeout,
 *          the last packet is retransmitted to the TFTP client automatically.@n
 *          It can be changed during the TFTP server initialization by the
 *          @ref fnet_tftp_srv_init() function. @n
 *          Default value is @b @c 3.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_SRV_TIMEOUT
    #define FNET_CFG_TFTP_SRV_TIMEOUT           (3u) /*sec*/
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TFTP_SRV_RETRANSMIT_MAX
 * @brief   Default maximum number of retransmissions. @n
 *          If no response from a TFTP client is received
 *          till maximum retransmission number is reached,
 *          the TFTP server cancels the data transfer.@n
 *          It can be changed during the TFTP server initialization by the
 *          @ref fnet_tftp_srv_init() function. @n
 *          Default value is @b @c 4.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_SRV_RETRANSMIT_MAX
    #define FNET_CFG_TFTP_SRV_RETRANSMIT_MAX    (4u)
#endif

/*! @} */

#ifdef FNET_CFG_TFTP_SRV_MAX
    #error "FNET_CFG_TFTP_SRV_MAX parameter is obsolete. Use FNET_CFG_TFTP_SRV."
#endif

#endif /* _FNET_TFTP_CONFIG_H_ */
