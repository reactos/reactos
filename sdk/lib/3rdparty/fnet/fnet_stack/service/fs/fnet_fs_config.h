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
*  FNET File System configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_FS_CONFIG_H_

#define _FNET_FS_CONFIG_H_

/** @addtogroup fnet_fs_config */
/** @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_FS
 * @brief    File System Interface support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_FS
    #if FNET_CFG_HTTP_SRV
        #define FNET_CFG_FS         (1)
    #else
        #define FNET_CFG_FS         (0)
    #endif
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FS_MOUNT_MAX
 * @brief    Maximum number of mount points. @n
 *           Default value is @b @c 2.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_FS_MOUNT_MAX
    #define FNET_CFG_FS_MOUNT_MAX           (2U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FS_MOUNT_NAME_MAX
 * @brief    Maximum size of a mount-point name. @n
 *           For example, for mount point named "rom" the name size is 3.@n
 *           Default value is @b @c 10.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_FS_MOUNT_NAME_MAX
    #define FNET_CFG_FS_MOUNT_NAME_MAX      (10U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FS_DESC_MAX
 * @brief    Maximum number of file and directory descriptors opened
 *           simultaneously.@n
 *           Default value is @b @c 5.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_FS_DESC_MAX
    #define FNET_CFG_FS_DESC_MAX            (5U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FS_ROM
 * @brief    FNET ROM File System support:
 *               - @b @c 1 = is enabled (Default value) .
 *               - @c 0 = is disabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_FS_ROM
    #define FNET_CFG_FS_ROM                 (1)
#endif

/** @} */

#endif /* _FNET_FS_CONFIG_H_ */
