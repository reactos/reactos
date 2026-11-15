/**************************************************************************
*
* Copyright 2016-2018 by Andrey Butok. FNET Community.
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
*  On-chip Flash Module driver configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_FLASH_CONFIG_H_

#define _FNET_FLASH_CONFIG_H_

/*! @addtogroup fnet_flash_config */
/*! @{ */
/**************************************************************************/ /*!
 * @def      FNET_CFG_FLASH
 * @brief    Flash driver support:
 *               - @c 1 = is enabled. It is valid only if a platform-specific
 *                       Flash driver is availble (FNET_CFG_CPU_FLASH).
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_FLASH
    #define FNET_CFG_FLASH              (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FLASH_CACHE
 * @brief    Flash Write Cache support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 *           It is recommended to enable caching when an application may write
 *           data to unaligned addresses.
 ******************************************************************************/
#ifndef FNET_CFG_FLASH_CACHE
    #define FNET_CFG_FLASH_CACHE        (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FLASH_CACHE
 * @brief    The number of entries in cache.
 *           One entry contains @ref FNET_CFG_CPU_FLASH_PROGRAM_SIZE bytes of data.@n
 *           Default value is @b @c 2.
 ******************************************************************************/
#ifndef FNET_CFG_FLASH_CACHE_SIZE
    #define FNET_CFG_FLASH_CACHE_SIZE   (2)
#endif

/*! @} */

#endif /* _FNET_FLASH_CONFIG_H_ */
