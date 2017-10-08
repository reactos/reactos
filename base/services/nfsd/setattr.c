/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#include <windows.h>
#include <stdio.h>
#include <strsafe.h>

#include "from_kernel.h"
#include "nfs41_ops.h"
#include "delegation.h"
#include "name_cache.h"
#include "upcall.h"
#include "util.h"
#include "daemon_debug.h"


/* NFS41_FILE_SET */
static int parse_setattr(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall)
{
    int status;
    setattr_upcall_args *args = &upcall->args.setattr;

    status = get_name(&buffer, &length, &args->path);
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->set_class, sizeof(args->set_class));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->buf_len, sizeof(args->buf_len));
    if (status) goto out;

    args->buf = buffer;
    args->root = upcall->root_ref;
    args->state = upcall->state_ref;

    dprintf(1, "parsing NFS41_FILE_SET: filename='%s' info_class=%d "
        "buf_len=%d\n", args->path, args->set_class, args->buf_len);
out:
    return status;
}

static int handle_nfs41_setattr(setattr_upcall_args *args)
{
    PFILE_BASIC_INFO basic_info = (PFILE_BASIC_INFO)args->buf;
    nfs41_open_state *state = args->state;
    nfs41_superblock *superblock = state->file.fh.superblock;
    stateid_arg stateid;
    nfs41_file_info info = { 0 }, old_info = { 0 };
    int status = NO_ERROR, getattr_status;
	
	if (basic_info->FileAttributes) {
		info.hidden = basic_info->FileAttributes & FILE_ATTRIBUTE_HIDDEN ? 1 : 0;
		info.system = basic_info->FileAttributes & FILE_ATTRIBUTE_SYSTEM ? 1 : 0;
		info.archive = basic_info->FileAttributes & FILE_ATTRIBUTE_ARCHIVE ? 1 : 0;
		getattr_status = nfs41_attr_cache_lookup(session_name_cache(state->session),
			state->file.fh.fileid, &old_info);

		if (getattr_status || info.hidden != old_info.hidden) {
			info.attrmask.arr[0] = FATTR4_WORD0_HIDDEN;
			info.attrmask.count = 1;
		}
		if (getattr_status || info.archive != old_info.archive) {
			info.attrmask.arr[0] |= FATTR4_WORD0_ARCHIVE;
			info.attrmask.count = 1;
		}
		if (getattr_status || info.system != old_info.system) {
			info.attrmask.arr[1] = FATTR4_WORD1_SYSTEM;
			info.attrmask.count = 2;
		}
	}
    if (old_info.mode == 0444 && 
            ((basic_info->FileAttributes & FILE_ATTRIBUTE_READONLY) == 0)) {
        info.mode = 0644;
        info.attrmask.arr[1] |= FATTR4_WORD1_MODE;
        info.attrmask.count = 2;
    }

    if (superblock->cansettime) {
        /* set the time_delta so xdr_settime4() can decide
         * whether or not to use SET_TO_SERVER_TIME4 */
        info.time_delta = &superblock->time_delta;

        /* time_create */
        if (basic_info->CreationTime.QuadPart > 0) {
            file_time_to_nfs_time(&basic_info->CreationTime,
                &info.time_create);
            info.attrmask.arr[1] |= FATTR4_WORD1_TIME_CREATE;
            info.attrmask.count = 2;
        }
        /* time_access_set */
        if (basic_info->LastAccessTime.QuadPart > 0) {
            file_time_to_nfs_time(&basic_info->LastAccessTime,
                &info.time_access);
            info.attrmask.arr[1] |= FATTR4_WORD1_TIME_ACCESS_SET;
            info.attrmask.count = 2;
        }
        /* time_modify_set */
        if (basic_info->LastWriteTime.QuadPart > 0) {
            file_time_to_nfs_time(&basic_info->LastWriteTime,
                &info.time_modify);
            info.attrmask.arr[1] |= FATTR4_WORD1_TIME_MODIFY_SET;
            info.attrmask.count = 2;
        }
    }

    /* mode */
    if (basic_info->FileAttributes & FILE_ATTRIBUTE_READONLY) {
        info.mode = 0444;
        info.attrmask.arr[1] |= FATTR4_WORD1_MODE;
        info.attrmask.count = 2;
    }

    /* mask out unsupported attributes */
    nfs41_superblock_supported_attrs(superblock, &info.attrmask);

    if (!info.attrmask.count)
        goto out;

    /* break read delegations before SETATTR */
    nfs41_delegation_return(state->session, &state->file,
        OPEN_DELEGATE_READ, FALSE);

    nfs41_open_stateid_arg(state, &stateid);

    status = nfs41_setattr(state->session, &state->file, &stateid, &info);
    if (status) {
        dprintf(1, "nfs41_setattr() failed with error %s.\n",
            nfs_error_string(status));
        status = nfs_to_windows_error(status, ERROR_NOT_SUPPORTED);
    }
    args->ctime = info.change;
out:
    return status;
}

static int handle_nfs41_remove(setattr_upcall_args *args)
{
    nfs41_open_state *state = args->state;
    int status;

    /* break any delegations and truncate before REMOVE */
    nfs41_delegation_return(state->session, &state->file,
        OPEN_DELEGATE_WRITE, TRUE);

    status = nfs41_remove(state->session, &state->parent,
        &state->file.name, state->file.fh.fileid);
    if (status)
        dprintf(1, "nfs41_remove() failed with error %s.\n",
            nfs_error_string(status));

    return nfs_to_windows_error(status, ERROR_ACCESS_DENIED);
}

static void open_state_rename(
    OUT nfs41_open_state *state,
    IN const nfs41_abs_path *path)
{
    AcquireSRWLockExclusive(&state->path.lock);

    abs_path_copy(&state->path, path);
    last_component(state->path.path, state->path.path + state->path.len,
        &state->file.name);
    last_component(state->path.path, state->file.name.name,
        &state->parent.name);

    ReleaseSRWLockExclusive(&state->path.lock);
}

static int nfs41_abs_path_compare(
    IN const struct list_entry *entry,
    IN const void *value)
{
    nfs41_open_state *client = list_container(entry, nfs41_open_state, client_entry);
    const nfs41_abs_path *name = (const nfs41_abs_path *)value;
    if (client->path.len == name->len && 
            !strncmp(client->path.path, name->path, client->path.len))
        return NO_ERROR;
    return ERROR_FILE_NOT_FOUND;
}

static int is_dst_name_opened(nfs41_abs_path *dst_path, nfs41_session *dst_session)
{
    int status;
    nfs41_client *client = dst_session->client;

    EnterCriticalSection(&client->state.lock);
    if (list_search(&client->state.opens, dst_path, nfs41_abs_path_compare))
        status = TRUE;
    else
        status = FALSE;
    LeaveCriticalSection(&client->state.lock);

    return status;
}
static int handle_nfs41_rename(setattr_upcall_args *args)
{
    nfs41_open_state *state = args->state;
    nfs41_session *dst_session;
    PFILE_RENAME_INFO rename = (PFILE_RENAME_INFO)args->buf;
    nfs41_abs_path dst_path = { 0 };
    nfs41_path_fh dst_dir, dst;
    nfs41_component dst_name, *src_name;
    uint32_t depth = 0;
    int status;

    src_name = &state->file.name;

    if (rename->FileNameLength == 0) {
        /* start from state->path instead of args->path, in case we got
         * the file from a referred server */
        AcquireSRWLockShared(&state->path.lock);
        abs_path_copy(&dst_path, &state->path);
        ReleaseSRWLockShared(&state->path.lock);

        path_fh_init(&dst_dir, &dst_path);
        fh_copy(&dst_dir.fh, &state->parent.fh);

        create_silly_rename(&dst_path, &state->file.fh, &dst_name);
        dprintf(1, "silly rename: %s -> %s\n", src_name->name, dst_name.name);

        /* break any delegations and truncate before silly rename */
        nfs41_delegation_return(state->session, &state->file,
            OPEN_DELEGATE_WRITE, TRUE);

        status = nfs41_rename(state->session,
            &state->parent, src_name,
            &dst_dir, &dst_name);
        if (status) {
            dprintf(1, "nfs41_rename() failed with error %s.\n",
                nfs_error_string(status));
            status = nfs_to_windows_error(status, ERROR_ACCESS_DENIED);
        } else {
            /* rename state->path on success */
            open_state_rename(state, &dst_path);
        }
        goto out;
    }

    dst_path.len = (unsigned short)WideCharToMultiByte(CP_UTF8, 0,
        rename->FileName, rename->FileNameLength/sizeof(WCHAR),
        dst_path.path, NFS41_MAX_PATH_LEN, NULL, NULL);
    if (dst_path.len == 0) {
        eprintf("WideCharToMultiByte failed to convert destination "
            "filename %S.\n", rename->FileName);
        status = ERROR_INVALID_PARAMETER;
        goto out;
    }
    path_fh_init(&dst_dir, &dst_path);

    /* the destination path is absolute, so start from the root session */
    status = nfs41_lookup(args->root, nfs41_root_session(args->root),
        &dst_path, &dst_dir, &dst, NULL, &dst_session);

    while (status == ERROR_REPARSE) {
        if (++depth > NFS41_MAX_SYMLINK_DEPTH) {
            status = ERROR_TOO_MANY_LINKS;
            goto out;
        }

        /* replace the path with the symlink target's */
        status = nfs41_symlink_target(dst_session, &dst_dir, &dst_path);
        if (status) {
            eprintf("nfs41_symlink_target() for %s failed with %d\n", 
                dst_dir.path->path, status);
            goto out;
        }

        /* redo the lookup until it doesn't return REPARSE */
        status = nfs41_lookup(args->root, dst_session,
            &dst_path, &dst_dir, NULL, NULL, &dst_session);
    }

    /* get the components after lookup in case a referral changed its path */
    last_component(dst_path.path, dst_path.path + dst_path.len, &dst_name);
    last_component(dst_path.path, dst_name.name, &dst_dir.name);

    if (status == NO_ERROR) {
        if (!rename->ReplaceIfExists) {
            status = ERROR_FILE_EXISTS;
            goto out;
        }
        /* break any delegations and truncate the destination file */
        nfs41_delegation_return(dst_session, &dst,
            OPEN_DELEGATE_WRITE, TRUE);
    } else if (status != ERROR_FILE_NOT_FOUND) {
        dprintf(1, "nfs41_lookup('%s') failed to find destination "
            "directory with %d\n", dst_path.path, status);
        goto out;
    }

    /* http://tools.ietf.org/html/rfc5661#section-18.26.3
     * "Source and target directories MUST reside on the same
     * file system on the server." */
    if (state->parent.fh.superblock != dst_dir.fh.superblock) {
        status = ERROR_NOT_SAME_DEVICE;
        goto out;
    }

    status = is_dst_name_opened(&dst_path, dst_session);
    if (status) {
        /* AGLO: 03/21/2011: we can't handle rename of a file with a filename 
         * that is currently opened by this client
         */
        eprintf("handle_nfs41_rename: %s is opened\n", dst_path.path);
        status = ERROR_FILE_EXISTS;
        goto out;
    }

    /* break any delegations on the source file */
    nfs41_delegation_return(state->session, &state->file,
        OPEN_DELEGATE_WRITE, FALSE);

    status = nfs41_rename(state->session,
        &state->parent, src_name,
        &dst_dir, &dst_name);
    if (status) {
        dprintf(1, "nfs41_rename() failed with error %s.\n",
            nfs_error_string(status));
        status = nfs_to_windows_error(status, ERROR_ACCESS_DENIED);
    } else {
        /* rename state->path on success */
        open_state_rename(state, &dst_path);
    }
out:
    return status;
}

static int handle_nfs41_set_size(setattr_upcall_args *args)
{
    nfs41_file_info info = { 0 };
    stateid_arg stateid;
    /* note: this is called with either FILE_END_OF_FILE_INFO or
     * FILE_ALLOCATION_INFO, both of which contain a single LARGE_INTEGER */
    PLARGE_INTEGER size = (PLARGE_INTEGER)args->buf;
    nfs41_open_state *state = args->state;
    int status;

    /* break read delegations before SETATTR */
    nfs41_delegation_return(state->session, &state->file,
        OPEN_DELEGATE_READ, FALSE);

    nfs41_open_stateid_arg(state, &stateid);

    info.size = size->QuadPart;
    info.attrmask.count = 1;
    info.attrmask.arr[0] = FATTR4_WORD0_SIZE;

    dprintf(2, "calling setattr() with size=%lld\n", info.size);
    status = nfs41_setattr(state->session, &state->file, &stateid, &info);
    if (status) {
        dprintf(1, "nfs41_setattr() failed with error %s.\n",
            nfs_error_string(status));
        goto out;
    }

    /* update the last offset for LAYOUTCOMMIT */
    AcquireSRWLockExclusive(&state->lock);
    state->pnfs_last_offset = info.size ? info.size - 1 : 0;
    ReleaseSRWLockExclusive(&state->lock);
    args->ctime = info.change;
out:
    return status = nfs_to_windows_error(status, ERROR_NOT_SUPPORTED);
}

static int handle_nfs41_link(setattr_upcall_args *args)
{
    nfs41_open_state *state = args->state;
    PFILE_LINK_INFORMATION link = (PFILE_LINK_INFORMATION)args->buf;
    nfs41_session *dst_session;
    nfs41_abs_path dst_path = { 0 };
    nfs41_path_fh dst_dir, dst;
    nfs41_component dst_name;
    uint32_t depth = 0;
    nfs41_file_info info = { 0 };
    int status;

    dst_path.len = (unsigned short)WideCharToMultiByte(CP_UTF8, 0,
        link->FileName, link->FileNameLength/sizeof(WCHAR),
        dst_path.path, NFS41_MAX_PATH_LEN, NULL, NULL);
    if (dst_path.len == 0) {
        eprintf("WideCharToMultiByte failed to convert destination "
            "filename %S.\n", link->FileName);
        status = ERROR_INVALID_PARAMETER;
        goto out;
    }
    path_fh_init(&dst_dir, &dst_path);

    /* the destination path is absolute, so start from the root session */
    status = nfs41_lookup(args->root, nfs41_root_session(args->root),
        &dst_path, &dst_dir, &dst, NULL, &dst_session);

    while (status == ERROR_REPARSE) {
        if (++depth > NFS41_MAX_SYMLINK_DEPTH) {
            status = ERROR_TOO_MANY_LINKS;
            goto out;
        }

        /* replace the path with the symlink target's */
        status = nfs41_symlink_target(dst_session, &dst_dir, &dst_path);
        if (status) {
            eprintf("nfs41_symlink_target() for %s failed with %d\n", 
                dst_dir.path->path, status);
            goto out;
        }

        /* redo the lookup until it doesn't return REPARSE */
        status = nfs41_lookup(args->root, dst_session,
            &dst_path, &dst_dir, &dst, NULL, &dst_session);
    }

    /* get the components after lookup in case a referral changed its path */
    last_component(dst_path.path, dst_path.path + dst_path.len, &dst_name);
    last_component(dst_path.path, dst_name.name, &dst_dir.name);

    if (status == NO_ERROR) {
        if (!link->ReplaceIfExists) {
            status = ERROR_FILE_EXISTS;
            goto out;
        }
    } else if (status != ERROR_FILE_NOT_FOUND) {
        dprintf(1, "nfs41_lookup('%s') failed to find destination "
            "directory with %d\n", dst_path.path, status);
        goto out;
    }

    /* http://tools.ietf.org/html/rfc5661#section-18.9.3
     * "The existing file and the target directory must reside within
     * the same file system on the server." */
    if (state->file.fh.superblock != dst_dir.fh.superblock) {
        status = ERROR_NOT_SAME_DEVICE;
        goto out;
    }

    if (status == NO_ERROR) {
        /* break any delegations and truncate the destination file */
        nfs41_delegation_return(dst_session, &dst,
            OPEN_DELEGATE_WRITE, TRUE);

        /* LINK will return NFS4ERR_EXIST if the target file exists,
         * so we have to remove it ourselves */
        status = nfs41_remove(state->session,
            &dst_dir, &dst_name, dst.fh.fileid);
        if (status) {
            dprintf(1, "nfs41_remove() failed with error %s.\n",
                nfs_error_string(status));
            status = ERROR_FILE_EXISTS;
            goto out;
        }
    }

    /* break read delegations on the source file */
    nfs41_delegation_return(state->session, &state->file,
        OPEN_DELEGATE_READ, FALSE);

    status = nfs41_link(state->session, &state->file, &dst_dir, &dst_name, 
            &info);
    if (status) {
        dprintf(1, "nfs41_link() failed with error %s.\n",
            nfs_error_string(status));
        status = nfs_to_windows_error(status, ERROR_INVALID_PARAMETER);
    }
    args->ctime = info.change;
out:
    return status;
}

static int handle_setattr(nfs41_upcall *upcall)
{
    setattr_upcall_args *args = &upcall->args.setattr;
    int status;

    switch (args->set_class) {
    case FileBasicInformation:
        status = handle_nfs41_setattr(args);
        break;
    case FileDispositionInformation:
        status = handle_nfs41_remove(args);
        break;
    case FileRenameInformation:
        status = handle_nfs41_rename(args);
        break;
    case FileAllocationInformation:
    case FileEndOfFileInformation:
        status = handle_nfs41_set_size(args);
        break;
    case FileLinkInformation:
        status = handle_nfs41_link(args);
        break;
    default:
        eprintf("unknown set_file information class %d\n",
            args->set_class);
        status = ERROR_NOT_SUPPORTED;
        break;
    }

    return status;
}

static int marshall_setattr(unsigned char *buffer, uint32_t *length, nfs41_upcall *upcall)
{
    setattr_upcall_args *args = &upcall->args.setattr;
    return safe_write(&buffer, length, &args->ctime, sizeof(args->ctime));
}


const nfs41_upcall_op nfs41_op_setattr = {
    parse_setattr,
    handle_setattr,
    marshall_setattr
};
