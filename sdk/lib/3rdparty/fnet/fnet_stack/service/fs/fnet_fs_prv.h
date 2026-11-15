/**************************************************************************
*
* Copyright 2008-2017 by Andrey Butok. FNET Community.
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
*  Private. File System API.
*
***************************************************************************/

#ifndef _FNET_FS_PRV_H_

#define _FNET_FS_PRV_H_

#include "fnet.h"

#if FNET_CFG_FS

#include "fnet_fs.h"

/* Type of access and method for the file. */
typedef enum
{
    FNET_FS_MODE_READ               = (1 << 0), /* Data can be read from the file.*/
    FNET_FS_MODE_WRITE              = (1 << 1), /* Data can be written to the file.*/
    FNET_FS_MODE_END                = (1 << 2), /* Move to the end of the file.*/
    FNET_FS_MODE_OPEN_EXISTING      = (1 << 3), /* Opens the file. The function fails if the file is not existing. (Default) */
    FNET_FS_MODE_OPEN_ALWAYS        = (1 << 4), /* Opens the file, if it is existing. If not, the function creates the new file. */
    FNET_FS_MODE_OPEN_TRUNC         = (1 << 5) /* If the file is existing, it is truncated and overwritten. */
}
fnet_fs_open_mode_t;

/* Mount point. */
struct fnet_fs_mount_point
{
    fnet_char_t name[FNET_CFG_FS_MOUNT_NAME_MAX + 1U];
    struct fnet_fs *fs;
    const void *arg;  /* Argument passed by mount(). */
};

/* Descriptor structure. */
struct fnet_fs_desc
{
    fnet_uint32_t id;   /* Owner FS Directory ID. */
    fnet_uint32_t pos;  /* Current position. */
    struct fnet_fs_mount_point *mount;  /* Pointer to the mount point. */
};

/* File operations. */
struct fnet_fs_file_operations
{
    fnet_return_t (*fopen)( struct fnet_fs_desc *file, const fnet_char_t *name, fnet_uint8_t mode, struct fnet_fs_desc *re_dir );
    fnet_size_t (*fread) (struct fnet_fs_desc *file, void *buf, fnet_size_t bytes);
    fnet_return_t (*fseek) (struct fnet_fs_desc *file, fnet_int32_t offset, fnet_fs_seek_origin_t origin);
    fnet_return_t (*finfo) (struct fnet_fs_desc *file, fnet_fs_dirent_t *dirent);
};

/* Dir operations. */
struct fnet_fs_dir_operations
{
    fnet_return_t (*opendir)( struct fnet_fs_desc *dir, const fnet_char_t *name );
    fnet_return_t (*readdir)( struct fnet_fs_desc *dir, fnet_fs_dirent_t *dirent );
};

/* FS operations. */
struct fnet_fs_operations
{
    fnet_return_t (*mount)( const void *arg );
    void (*unmount)( const void *arg );
};

/* FS control structure (for every fs).*/
struct fnet_fs
{
    const fnet_char_t *name;                                    /* FS uniqe name.*/
    const struct fnet_fs_operations          *operations;       /* FS operations.*/
    const struct fnet_fs_file_operations     *file_operations;  /* FS file operations.*/
    const struct fnet_fs_dir_operations      *dir_operations;   /* FS directory operations.*/
    struct fnet_fs                     *_next;                  /* Next FS in the list.*/
    struct fnet_fs                     *_prev;                  /* Previous FS in the list.*/
};

#if defined(__cplusplus)
extern "C" {
#endif

extern struct fnet_fs_mount_point fnet_fs_mount_list[FNET_CFG_FS_MOUNT_MAX];

void _fnet_fs_register( struct fnet_fs *fs );
void _fnet_fs_unregister( struct fnet_fs *fs );
struct fnet_fs *_fnet_fs_find_name( fnet_char_t *name );
fnet_int32_t _fnet_fs_path_cmp( const fnet_char_t **path, const fnet_char_t *name);

#if defined(__cplusplus)
}
#endif

#endif /* FNET_CFG_FS */

#endif
