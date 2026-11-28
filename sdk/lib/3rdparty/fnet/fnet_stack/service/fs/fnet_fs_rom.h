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
*  FNET ROM FS API.
*
***************************************************************************/
#ifndef _FNET_FS_ROM_H_

#define _FNET_FS_ROM_H_

#if (FNET_CFG_FS && FNET_CFG_FS_ROM) || defined(__DOXYGEN__)

#include "fnet_fs.h"

/*! @addtogroup fnet_fs_rom
* The FNET ROM File System provides read-only access to files
* and directories. @n
* The FNET project has the @ref how_to_generate_rom_fs "special GUI PC tool"
* that is used to create the ROM file-system image files.
*
* After the FS is initialized by calling the @ref fnet_fs_init()
* function, a user application should call the  @ref fnet_fs_rom_register()
* function  and finally mount a ROM FS Image by passing the @ref FNET_FS_ROM_NAME
* and the @ref fnet_fs_rom_image_t structure as arguments to the
* @ref fnet_fs_mount() function. @n
* Thereafter an application has access to files and directories on the mounted
* FNET ROM FS image.
*
* For example:
* @code
* ...
* // FNET FS initialization.
* if( fnet_fs_init( ) == FNET_OK)
* {
*    // Register FNET ROM FS.
*    fnet_fs_rom_register( );
*
*    // Mount FNET ROM FS image.
*    if( fnet_fs_mount( FNET_FS_ROM_NAME, "rom", (void *)&fnet_fs_image ) == FNET_ERR )
*           fnet_println("ERROR: FS image mount is failed!");
*    ...
*
*    // Print directory content.
*    {
*       fnet_fs_dirent_t ep;
*       fnet_fs_dir_t dir;
*       fnet_uint8_t name[FNET_CFG_FS_MOUNT_NAME_MAX+1];
*
*       // Open dir.
*       dir = fnet_fs_opendir("rom");
*
*       if (dir)
*       {
*           fnet_memset_zero(&ep, sizeof(fnet_fs_dirent_t) );
*           ep.d_name = name;
*           ep.d_name_size = sizeof(name);
*
*           // Print the dir content.
*           while ((fnet_fs_readdir (dir, &ep))==FNET_OK)
*               fnet_println ("%7s  -  %s", (ep.d_type == DT_DIR)?"<DIR>":"<FILE>",ep.d_name);
*
*           // Close dir.
*           fnet_fs_closedir(dir);
*       }
*       else
*           fnet_println("ERROR: The directory is failed!");
*    }
*    ...
* }
* else
* {
*    fnet_println("ERROR: FNET FS initialization is failed!");
* }
* ...
* @endcode
* @n
* Configuration parameters:
* - @ref FNET_CFG_FS_ROM
*/

/*! @{ */


/**************************************************************************/ /*!
 * @brief   FNET ROM file-system node.@n
 *          The node represents a file or a directory.
 * @see     fnet_fs_readdir()
 ******************************************************************************/
typedef struct fnet_fs_rom_node
{
    const fnet_char_t *name;               /**< @brief Name of a file or directory
                                    * (null-terminated string). */
    const fnet_uint8_t *data;      /**< @brief Pointer to a file-content buffer. @n
                                    * For a directory this field must be set to @c 0.*/
    fnet_size_t data_size;         /**< @brief Size of the file buffer pointed to
                                    * by the @c data field. @n
                                    * For a directory this field must be set to @c 0.*/
    const struct fnet_fs_rom_node *parent_node;    /**< @brief Pointer to the
	                                                 * parent directory. @n
	                                                 * For the root directory this field must be
	                                                 * set to @c 0.*/
} fnet_fs_rom_node_t;

/**************************************************************************/ /*!
 * @brief FNET ROM file-system image
 ******************************************************************************/
typedef struct fnet_fs_rom_image
{
    const fnet_char_t              *name;      /**< @brief File-system name (null-terminated string).@n
                                                * Should be set to the @ref FNET_FS_ROM_NAME. */
    fnet_uint32_t                   version;    /**< @brief File-system version the FS image
                                                * was generated for.*/
    const fnet_fs_rom_node_t       *nodes;     /**< @brief Array of file-system nodes.@n
                                                * The last node element must have
                                                * all fields set to zero
                                                * as the end-of-array mark.
                                                */
} fnet_fs_rom_image_t;

/**************************************************************************/ /*!
 * @brief FNET ROM file-system name string.
 ******************************************************************************/
#define FNET_FS_ROM_NAME     "fnet_rom"

/**************************************************************************/ /*!
 * @brief FNET ROM file-system current version.
 ******************************************************************************/
#define FNET_FS_ROM_VERSION  (2u)

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Registers the FNET ROM file system.
 *
 * @see fnet_fs_rom_unregister()
 *
 ******************************************************************************
 *
 * This function registers the FNET ROM file system within the FNET FS interface.
 *
 ******************************************************************************/
void fnet_fs_rom_register(void);

/***************************************************************************/ /*!
 *
 * @brief    Unregisters the FNET ROM file system.
 *
 * @see fnet_fs_rom_register()
 *
 ******************************************************************************
 *
 * This function unregisters the FNET ROM file system from the FNET FS interface.
 *
 ******************************************************************************/
void fnet_fs_rom_unregister(void);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_FS && FNET_CFG_FS_ROM */


#endif /* _FNET_FS_ROM_H_ */
