/**************************************************************************
*
* Copyright 2018 by Andrey Butok. FNET Community.
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
*  FNET benchmark service configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_BENCH_CONFIG_H_

#define _FNET_BENCH_CONFIG_H_

/*! @addtogroup fnet_bench_srv_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_BENCH_SRV
 * @brief    FNET benchmark server/receiver support:
 *               - @c 1..n = is enabled. Its value defines maximum number of the benchmark servers that can be run simultaneously.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_BENCH_SRV
    #define FNET_CFG_BENCH_SRV                       (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_BENCH_SRV_PORT
 * @brief   Default benchmark server port number (in network byte order).@n
 *          The benchmark server uses this port for sending and receiving of
 *          messages. @n
 *          It can be changed during the benchmark server initialization by the
 *          @ref fnet_bench_srv_init() function.@n
 *          Default value is FNET_HTONS(7007).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_BENCH_SRV_PORT
    #define FNET_CFG_BENCH_SRV_PORT                 (FNET_HTONS(7007))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_BENCH_SRV_BUFFER_SIZE
 * @brief   Defines the socket recieve buffer size.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_BENCH_SRV_BUFFER_SIZE
    #define FNET_CFG_BENCH_SRV_BUFFER_SIZE          (8*1024)
#endif

/*! @} */

/*! @addtogroup fnet_bench_cln_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_BENCH_CLN
 * @brief    FNET benchmark client/sender support:
 *               - @c 1..n = is enabled. Its value defines maximum number of the benchmark clients that can be run simultaneously.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_BENCH_CLN
    #define FNET_CFG_BENCH_CLN                       (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_BENCH_CLN_PORT
 * @brief   Default benchmark client port number (in network byte order).@n
 *          The benchmark client uses this port for sending and receiving of
 *          messages. @n
 *          It can be changed during the benchmark client initialization by the
 *          @ref fnet_bench_cln_init() function.@n
 *          Default value is FNET_HTONS(7007).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_BENCH_CLN_PORT
    #define FNET_CFG_BENCH_CLN_PORT                 (FNET_HTONS(7007))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_BENCH_CLN_BUFFER_SIZE
 * @brief   Defines the socket transmit buffer size.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_BENCH_CLN_BUFFER_SIZE
    #define FNET_CFG_BENCH_CLN_BUFFER_SIZE          (5*1024)
#endif

/*! @} */

#endif /* _FNET_BENCH_CONFIG_H_ */
