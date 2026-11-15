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
*  FNET File System API.
*
***************************************************************************/

#ifndef _FNET_FS_H_

#define _FNET_FS_H_

#if FNET_CFG_FS || defined(__DOXYGEN__)

/*! @addtogroup fnet_fs
* The FNET File System (FS) API provides standard interface for
* file and directory manipulations. @n
* The FNET File System API is inspired by standard ANSI C and POSIX-based APIs. @n
*
* The following table summarizes the supported File System API functions:
* <table>
* <caption>File System API</caption>
* <tr class="fnet_td_grey">
* <th ALIGN=CENTER>Functions</th><th ALIGN=CENTER>Description</th>
* </tr>
* <tr>
* <td>@ref fnet_fs_init(), @ref fnet_fs_release()</td>
* <td>FNET file system interface initialization/release.</td>
* </tr>
* <tr>
* <td>@ref fnet_fs_mount(), @ref fnet_fs_unmount()</td>
* <td>Mount/unmount a file system image.</td>
* </tr>
* <tr>
* <td>@ref fnet_fs_fopen(), @ref fnet_fs_fopen_re(), fnet_fs_fclose()</td>
* <td>Open/close a file.</td>
* </tr>
* <tr>
* <td>@ref fnet_fs_fread(), @ref fnet_fs_fgetc()</td>
* <td>Read a file.</td>
* </tr>
* <tr>
* <td>@ref fnet_fs_rewind(), @ref fnet_fs_fseek(), @ref fnet_fs_feof(), @ref fnet_fs_ftell()</td>
* <td>Move/get a file pointer.</td>
* </tr>
* <tr>
* <td>@ref fnet_fs_opendir(), @ref fnet_fs_closedir()</td>
* <td>Open/close a directory.</td>
* </tr>
* <tr>
* <td>@ref fnet_fs_readdir(), @ref fnet_fs_rewinddir()</td>
* <td>Read a directory.</td>
* </tr>
* </table>
*
* The path name that points to a file or directory has the following format:@n
*   @verbatim /[mount name][/directory name]..[/file name] @endverbatim
*
* @n
* After the FS is initialized by calling the @ref fnet_fs_init()
* function, a user application should call a FS registration
* function (for example the @ref fnet_fs_rom_register() for the FNET ROM FS registration), and
* finally mount a FS image by the @ref fnet_fs_mount() function. Thereafter
* an application has access to files and directories on the mounted FS image.
*
* For example:
* @code
* ...
* // FNET FS initialization.
* if( fnet_fs_init( ) == FNET_OK)
* {
*    // Register ROM FS.
*    fnet_fs_rom_register( );
*
*    // Mount ROM FS image.
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
*       dir = fnet_fs_opendir("\rom");
*
*       if (dir)
*       {
*           fnet_memset(&ep, 0, sizeof(fnet_fs_dirent_t) );
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
* - @ref FNET_CFG_FS
* - @ref FNET_CFG_FS_MOUNT_MAX
* - @ref FNET_CFG_FS_MOUNT_NAME_MAX
* - @ref FNET_CFG_FS_DESC_MAX
*/
/** @{ */

/**************************************************************************/ /*!
 * @brief Directory descriptor. @n
 * This is the abstract key for accessing a directory.
 ******************************************************************************/
typedef void *fnet_fs_dir_t;

/**************************************************************************/ /*!
 * @brief File descriptor. @n
 * This is the abstract key for accessing a file.
 ******************************************************************************/
typedef void *fnet_fs_file_t;

/**************************************************************************/ /*!
 * @brief File-path splitter. @n
 * This symbol is used to delimit directories and files in a pathname string.
 ******************************************************************************/
#define FNET_FS_SPLITTER            '/'

/**************************************************************************/ /*!
 * @brief End-of-file condition. @n
 * This is a condition, where no data can be read from a data source.
 ******************************************************************************/
#define FNET_FS_EOF     (-1)

/**************************************************************************/ /*!
 * @brief Origin position. @n
 *        Used by @ref fnet_fs_fseek() function.
 *
 * @see fnet_fs_fseek()
 ******************************************************************************/
typedef enum
{
    FNET_FS_SEEK_SET = 0,   /**< @brief Origin is the start of the file.*/
    FNET_FS_SEEK_CUR,       /**< @brief Origin is the current position. */
    FNET_FS_SEEK_END        /**< @brief Origin is the end of the file. */
}
fnet_fs_seek_origin_t;

/**************************************************************************/ /*!
 * @brief Directory entry type.
 * @see fnet_fs_dirent_t
 ******************************************************************************/
typedef enum
{
    DT_UNKNOWN = 0,     /**< @brief Unspecified. */
    DT_REG,             /**< @brief A regular file. */
    DT_DIR              /**< @brief A directory. */
} fnet_fs_d_type_t;


/**************************************************************************/ /*!
 * @brief   This structure is used by the @ref fnet_fs_finfo() and
 *          the @ref fnet_fs_readdir() function to
 *          get information about directory entries (files or directories).
 * @see     fnet_fs_readdir(), fnet_fs_finfo()
 ******************************************************************************/
typedef struct
{
    fnet_uint32_t           d_ino;      /**< @brief Entry serial number. */
    fnet_fs_d_type_t        d_type;    /**< @brief Type of the entry defined by
                                        *   the  @ref fnet_fs_d_type_t.*/
    const fnet_char_t      *d_name;    /**< @brief Name of the entry (null-terminated
                                        *   string).*/
    fnet_size_t             d_size;     /**< @brief Size of the file entry. @n
                                        * If the entry is a directory this field is set to @c 0.*/
} fnet_fs_dirent_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the FNET File System Interface.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_release()
 *
 ******************************************************************************
 *
 * This function executes the initialization of the File System interface. @n
 * Only after a successful initialization the application may use FNET FS API
 * functions.
 *
 ******************************************************************************/
fnet_return_t fnet_fs_init(void);

/***************************************************************************/ /*!
 *
 * @brief    Releases the FNET File System Interface.
 *
 * @see fnet_fs_init()
 *
 ******************************************************************************
 *
 * This function releases the NET File System Interface.
 *
 ******************************************************************************/
void fnet_fs_release(void);

/***************************************************************************/ /*!
 *
 * @brief    Mounts a file system.
 *
 * @param fs_name       File System name associated with the mount point
 *                      (null-terminated string). @n
 *                      For the FNET ROM FS this parameter
 *                      should be set to the @ref FNET_FS_ROM_NAME value.
 *
 * @param mount_name    Mount point name (null-terminated string). @n
 *                      The name size must not exceed the
 *                      @ref FNET_CFG_FS_MOUNT_NAME_MAX value.
 *
 * @param arg           Pointer to the FS-specific mount argument.@n
 *                      For the FNET ROM FS this parameter is the pointer to
 *                      the @ref fnet_fs_rom_image_t structure.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_unmount()
 *
 ******************************************************************************
 *
 * This function instructs FNET File System interface that the file system
 * named by the @c fs_name is associated with a
 * mount point named by the @c mount_name and is ready to use.
 *
 ******************************************************************************/
fnet_return_t fnet_fs_mount( fnet_char_t *fs_name, const fnet_char_t *mount_name, const void *arg );

/***************************************************************************/ /*!
 *
 * @brief   Unmounts a file system.
 *
 * @param mount_name    Mount point name (null-terminated string).
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_mount()
 *
 ******************************************************************************
 *
 * This function instructs the FNET File System interface that the file
 * system should be disassociated from its mount point named by the
 * @c mount_name making it no longer accessible.
 *
 ******************************************************************************/
fnet_return_t fnet_fs_unmount( const fnet_char_t *mount_name );

/***************************************************************************/ /*!
 *
 * @brief    Opens a directory descriptor.
 *
 * @param dirname  The directory pathname.
 *
 * @return This function returns:
 *   - Directory descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_fs_closedir(),
 *
 ******************************************************************************
 *
 * This function opens and returns a directory descriptor corresponding
 * to the directory named by the @c dirname.
 *
 ******************************************************************************/
fnet_fs_dir_t fnet_fs_opendir( const fnet_char_t *dirname);

/***************************************************************************/ /*!
 *
 * @brief    Closes a directory descriptor.
 *
 * @param dir   Directory descriptor to be closed.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_opendir()
 *
 ******************************************************************************
 *
 * This function closes the directory descriptor reffered to by the @c dir.
 *
 ******************************************************************************/
fnet_return_t fnet_fs_closedir( fnet_fs_dir_t dir);

/***************************************************************************/ /*!
 *
 * @brief    Reads a directory entry.
 *
 * @param dir       Directory descriptor.
 *
 * @param dirent    Pointer to the entry information structure.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs or end of directory stream is reached.
 *
 * @see fnet_fs_opendir(), fnet_fs_rewinddir(), fnet_fs_finfo()
 *
 ******************************************************************************
 *
 * This function fills the @c dirent structure representing the directory
 * entry at the current position and increments the position to the next entry.@n
 * When the end of the directory is encountered, the @ref FNET_ERR is returned.@n
 * The function reads directory entries in sequence. All items in the directory
 * can be read by calling the @ref fnet_fs_readdir() function repeatedly.@n
 *
 ******************************************************************************/
fnet_return_t fnet_fs_readdir(fnet_fs_dir_t dir, fnet_fs_dirent_t *dirent);

/***************************************************************************/ /*!
 *
 * @brief    Resets a directory position.
 *
 * @param dir   Directory descriptor.
 *
 * @see fnet_fs_readdir()
 *
 ******************************************************************************
 *
 * This function resets the position of the directory descriptor @c dir to
 * the beginning of the directory, so that if the @ref fnet_fs_readdir()
 * function will be called it returns information about the first
 * entry in the directory again.
 *
 ******************************************************************************/
void fnet_fs_rewinddir( fnet_fs_dir_t dir );

/***************************************************************************/ /*!
 *
 * @brief    Opens a file descriptor.
 *
 * @param filename  The file pathname (null-terminated string).
 *
 * @param mode      String containing a file access modes. It can be:
 *                  - "r" = open a file for reading. The file must exist.
 *                  - "w" = create an empty file for writing. If a file with
 *                          the same name already exists its content is erased
 *                          and the file is treated as a new empty file.@n @n
 *                          The current version of FS API des not support this mode.
 *                  - "a" = append to a file. Writing operations append data
 *                          at the end of the file. The file is created if
 *                          it does not exist.@n @n
 *                          The current version of FS API des not support this mode.
 *                  - "r+" = open a file for update for both reading and writing.
 *                           The file must exist.@n @n
 *                           The current version of FS API does not support this mode.
 *                  - "w+" = create an empty file for both reading and writing.
 *                           If a file with the same name already exists its
 *                           content is erased and the file is treated as a
 *                           new empty file.@n @n
 *                           The current version of FS API does not support this mode.
 *                  - "a+" = open a file for reading and appending. All
 *                           writing operations are performed at the end
 *                           of the file. The initial file position for
 *                           reading is at the beginning of the file, but output
 *                           is always appended to the end of the file. The file
 *                           is created if it does not exist.@n @n
 *                           The current version of FS API does not support this mode.
 *
 * @return This function returns:
 *   - File descriptor @ref fnet_fs_file_t if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_fs_fopen_re(), fnet_fs_fclose()
 *
 ******************************************************************************
 *
 * This function opens the file, whose pathname is specified by the @c filename, and
 * associates it with a returned file descriptor. @n
 * The operations that are allowed on the file, and how these are performed,
 * is defined by the @c mode parameter.@n
 * NOTE: The current version of FS API supports the reading mode only.
 *
 ******************************************************************************/
fnet_fs_file_t fnet_fs_fopen(const fnet_char_t *filename, const fnet_char_t *mode);

/***************************************************************************/ /*!
 *
 * @brief    Opens a file relatively in an opened directory.
 *
 * @param filename  File pathnamem, which is relative to the @c dir directory
 *                  (null-terminated string).
 *
 * @param mode      String containing a file access modes. It can be:
 *                  - "r" = open a file for reading. The file must exist.
 *                  - "w" = create an empty file for writing. If a file with
 *                          the same name already exists its content is erased
 *                          and the file is treated as a new empty file.@n @n
 *                          The current version of FS API des not support this mode.
 *                  - "a" = append to a file. Writing operations append data
 *                          at the end of the file. The file is created if
 *                          it does not exist.@n @n
 *                          The current version of FS API des not support this mode.
 *                  - "r+" = open a file for update for both reading and writing.
 *                           The file must exist.@n @n
 *                           The current version of FS API does not support this mode.
 *                  - "w+" = create an empty file for both reading and writing.
 *                           If a file with the same name already exists its
 *                           content is erased and the file is treated as a
 *                           new empty file.@n @n
 *                           The current version of FS API does not support this mode.
 *                  - "a+" = open a file for reading and appending. All
 *                           writing operations are performed at the end
 *                           of the file. The initial file position for
 *                           reading is at the beginning of the file, but output
 *                           is always appended to the end of the file. The file
 *                           is created if it does not exist.@n @n
 *                           The current version of FS API does not support this mode.
 *
 * @param dir       Directory descriptor.
 *
 * @return This function returns:
 *   - File descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_fs_fopen(), fnet_fs_fclose()
 *
 ******************************************************************************
 *
 * This function opens the file whose pathname is specified by the @c filename,
 * which is relative to the  directory @c dir, and associates
 * it with a returned file descriptor. @n
 * The operations that are allowed on the file and how these are performed
 * are defined by the @c mode parameter.@n
 * NOTE: The current version of FS API supports the reading mode only.
 *
 ******************************************************************************/
fnet_fs_file_t fnet_fs_fopen_re(const fnet_char_t *filename, const fnet_char_t *mode, fnet_fs_dir_t dir);

/***************************************************************************/ /*!
 *
 * @brief    Closes a file descriptor.
 *
 * @param file  File descriptor to be closed.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_fopen(), fnet_fs_fopen_re()
 *
 ******************************************************************************
 *
 * This function closes the file associated with the @c file descriptor
 * and disassociates it.
 *
 ******************************************************************************/
fnet_return_t fnet_fs_fclose(fnet_fs_file_t file);

/***************************************************************************/ /*!
 *
 * @brief    Reads data from a file.
 *
 * @param buf   Pointer to the buffer to store the read data.
 *
 * @param size  Number of bytes to read.
 *
 * @param file  File descriptor.
 *
 * @return This function returns:
 *   - The total number of bytes successfully read.
 *
 * @see  fnet_fs_fgetc(), fnet_fs_rewind(), fnet_fs_feof()
 *
 ******************************************************************************
 *
 * This function reads an array of bytes from the @c file and stores them
 * in the block of memory specified by @c buf.@n
 * The position indicator of the @c file descriptor is advanced by the
 * total amount of bytes read.
 *
 ******************************************************************************/
fnet_size_t fnet_fs_fread(void *buf, fnet_size_t size, fnet_fs_file_t file);

/***************************************************************************/ /*!
 *
 * @brief    Resets a file position.
 *
 * @param file  File descriptor.
 *
 * @see fnet_fs_fseek()
 *
 ******************************************************************************
 *
 * This function sets the position indicator associated with the @c file
 * descriptor to the beginning of the file.
 *
 ******************************************************************************/
void fnet_fs_rewind( fnet_fs_file_t file );

/***************************************************************************/ /*!
 *
 * @brief    Checks the End-of-File indicator.
 *
 * @param file  File descriptor.
 *
 * @return This function returns:
 *   - @c FNET_TRUE in the case that the End-of-File is set.
 *   - @c FNET_FALSE in the case that the End-of-File is NOT set.
 *
 * @see fnet_fs_fread()
 *
 ******************************************************************************
 *
 * This function checks, whether the End-of-File indicator associated with
 * the @c file descriptor is set. @n
 * This indicator is generally set by a previous operation on the @c file
 * that reached the End-of-File. @n
 * Further read operations on the @c file - once the End-of-File has been
 * reached - will fail until either @ref fnet_fs_rewind() or @ref fnet_fs_fseek()
 * is successfully called to set the position indicator to a new value.
 *
 ******************************************************************************/
fnet_bool_t fnet_fs_feof(fnet_fs_file_t file);

/***************************************************************************/ /*!
 *
 * @brief    Gets a character from a file.
 *
 * @param file  File descriptor.
 *
 * @return This function returns:
 *   - The character read is returned as an fnet_int32_t value.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_fread()
 *
 ******************************************************************************
 *
 * This function returns the character currently pointed to by the internal
 * file position indicator of the specified @c file.@n
 * The internal file position indicator is advanced by one character
 * to point to the next character.
 *
 ******************************************************************************/
fnet_int32_t fnet_fs_fgetc(fnet_fs_file_t file);

/***************************************************************************/ /*!
 *
 * @brief    Changes the file-position indicator for the specified file.
 *
 * @param file      File descriptor, of which the position indicator should
 *                  be changed.
 *
 * @param offset    Specifies the number of bytes from @c origin,
 *                  where the position indicator should be placed.
 *                  It may be negative and positive.
 *
 * @param origin    Origin position defined by @ref fnet_fs_seek_origin_t.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_ftell()
 *
 ******************************************************************************
 *
 * This function sets the position indicator associated with the @c file
 * descriptor to a new position defined by adding @c offset to a reference
 * position specified by @c origin.
 *
 ******************************************************************************/
fnet_return_t fnet_fs_fseek (fnet_fs_file_t file, fnet_int32_t offset, fnet_fs_seek_origin_t origin);

/***************************************************************************/ /*!
 *
 * @brief    Gets the current position in a file.
 *
 * @param file  File descriptor.
 *
 * @return This function returns:
 *   - The current value of the position indicator if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_fseek()
 *
 ******************************************************************************
 *
 * This function returns the current value of the position indicator of
 * the @c file. The returned value corresponds to the number of bytes
 * from the beginning of the file.
 *
 ******************************************************************************/
fnet_int32_t fnet_fs_ftell (fnet_fs_file_t file);

/***************************************************************************/ /*!
 *
 * @brief    Gets a file information.
 *
 * @param file      File descriptor.
 *
 * @param dirent    Pointer to the entry/file information structure.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_fs_readdir(), fnet_fs_fopen()
 *
 ******************************************************************************
 *
 * This function fills the @c dirent structure with information about the @c file.
 * The information is defined by the @ref fnet_fs_dirent_t structure.
 *
 ******************************************************************************/
fnet_return_t fnet_fs_finfo (fnet_fs_file_t file, fnet_fs_dirent_t *dirent);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_FS */

#endif /* _FNET_FS_H_ */
