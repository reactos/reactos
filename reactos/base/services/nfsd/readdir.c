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
#include <strsafe.h>
#include <stdlib.h>
#include "from_kernel.h"
#include "nfs41_ops.h"
#include "daemon_debug.h"
#include "upcall.h"
#include "util.h"


typedef union _FILE_DIR_INFO_UNION {
    ULONG NextEntryOffset;
    FILE_NAMES_INFORMATION fni;
    FILE_DIRECTORY_INFO fdi;
    FILE_FULL_DIR_INFO ffdi;
    FILE_ID_FULL_DIR_INFO fifdi;
    FILE_BOTH_DIR_INFORMATION fbdi;
    FILE_ID_BOTH_DIR_INFO fibdi;
} FILE_DIR_INFO_UNION, *PFILE_DIR_INFO_UNION;


/* NFS41_DIR_QUERY */
static int parse_readdir(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall)
{
    int status;
    readdir_upcall_args *args = &upcall->args.readdir;

    status = safe_read(&buffer, &length, &args->query_class, sizeof(args->query_class));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->buf_len, sizeof(args->buf_len));
    if (status) goto out;
    status = get_name(&buffer, &length, &args->filter);
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->initial, sizeof(args->initial));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->restart, sizeof(args->restart));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->single, sizeof(args->single));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->kbuf, sizeof(args->kbuf));
    if (status) goto out;
    args->root = upcall->root_ref;
    args->state = upcall->state_ref;

    dprintf(1, "parsing NFS41_DIR_QUERY: info_class=%d buf_len=%d "
        "filter='%s'\n\tInitial\\Restart\\Single %d\\%d\\%d buf=%p\n",
        args->query_class, args->buf_len, args->filter,
        args->initial, args->restart, args->single, args->kbuf);
out:
    return status;
}

#define FILTER_STAR '*'
#define FILTER_QM   '>'

static __inline const char* skip_stars(
    const char *filter)
{
    while (*filter == FILTER_STAR)
        filter++;
    return filter;
}

static int readdir_filter(
    const char *filter,
    const char *name)
{
    const char *f = filter, *n = name;

    while (*f && *n) {
        if (*f == FILTER_STAR) {
            f = skip_stars(f);
            if (*f == '\0')
                return 1;
            while (*n && !readdir_filter(f, n))
                n++;
        } else if (*f == FILTER_QM || *f == *n) {
            f++;
            n++;
        } else
            return 0;
    }
    return *f == *n || *skip_stars(f) == '\0';
}

static uint32_t readdir_size_for_entry(
    IN int query_class,
    IN uint32_t wname_size)
{
    uint32_t needed = wname_size;
    switch (query_class)
    {
    case FileDirectoryInformation:
        needed += FIELD_OFFSET(FILE_DIRECTORY_INFO, FileName);
        break;
    case FileIdFullDirectoryInformation:
        needed += FIELD_OFFSET(FILE_ID_FULL_DIR_INFO, FileName);
        break;
    case FileFullDirectoryInformation:
        needed += FIELD_OFFSET(FILE_FULL_DIR_INFO, FileName);
        break;
    case FileIdBothDirectoryInformation:
        needed += FIELD_OFFSET(FILE_ID_BOTH_DIR_INFO, FileName);
        break;
    case FileBothDirectoryInformation:
        needed += FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName);
        break;
    case FileNamesInformation:
        needed += FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName);
        break;
    default:
        eprintf("unhandled dir query class %d\n", query_class);
        return 0;
    }
    return needed;
}

static void readdir_copy_dir_info(
    IN nfs41_readdir_entry *entry,
    IN PFILE_DIR_INFO_UNION info)
{
    info->fdi.FileIndex = (ULONG)entry->attr_info.fileid;
    nfs_time_to_file_time(&entry->attr_info.time_create,
        &info->fdi.CreationTime);
    nfs_time_to_file_time(&entry->attr_info.time_access,
        &info->fdi.LastAccessTime);
    nfs_time_to_file_time(&entry->attr_info.time_modify,
        &info->fdi.LastWriteTime);
    /* XXX: was using 'change' attr, but that wasn't giving a time */
    nfs_time_to_file_time(&entry->attr_info.time_modify,
        &info->fdi.ChangeTime);
    info->fdi.EndOfFile.QuadPart =
        info->fdi.AllocationSize.QuadPart =
            entry->attr_info.size;
    info->fdi.FileAttributes = nfs_file_info_to_attributes(
        &entry->attr_info);
}

static void readdir_copy_shortname(
    IN LPCWSTR name,
    OUT LPWSTR name_out,
    OUT CCHAR *name_size_out)
{
    /* GetShortPathName returns number of characters, not including \0 */
    *name_size_out = (CCHAR)GetShortPathNameW(name, name_out, 12);
    if (*name_size_out) {
        *name_size_out++;
        *name_size_out *= sizeof(WCHAR);
    }
}

static void readdir_copy_full_dir_info(
    IN nfs41_readdir_entry *entry,
    IN PFILE_DIR_INFO_UNION info)
{
    readdir_copy_dir_info(entry, info);
    /* for files with the FILE_ATTRIBUTE_REPARSE_POINT attribute,
     * EaSize is used instead to specify its reparse tag. this makes
     * the 'dir' command to show files as <SYMLINK>, and triggers a
     * FSCTL_GET_REPARSE_POINT to query the symlink target
     */
    info->fifdi.EaSize = entry->attr_info.type == NF4LNK ?
        IO_REPARSE_TAG_SYMLINK : 0;
}

static void readdir_copy_both_dir_info(
    IN nfs41_readdir_entry *entry,
    IN LPWSTR wname,
    IN PFILE_DIR_INFO_UNION info)
{
    readdir_copy_full_dir_info(entry, info);
    readdir_copy_shortname(wname, info->fbdi.ShortName,
        &info->fbdi.ShortNameLength);
}

static void readdir_copy_filename(
    IN LPCWSTR name,
    IN uint32_t name_size,
    OUT LPWSTR name_out,
    OUT ULONG *name_size_out)
{
    *name_size_out = name_size;
    memcpy(name_out, name, name_size);
}

static int format_abs_path(
    IN const nfs41_abs_path *path,
    IN const nfs41_component *name,
    OUT nfs41_abs_path *path_out)
{
    /* format an absolute path 'parent\name' */
    int status = NO_ERROR;

    InitializeSRWLock(&path_out->lock);
    abs_path_copy(path_out, path);
    if (FAILED(StringCchPrintfA(path_out->path + path_out->len,
        NFS41_MAX_PATH_LEN - path_out->len, "\\%s", name->name))) {
        status = ERROR_FILENAME_EXCED_RANGE;
        goto out;
    }
    path_out->len += name->len + 1;
out:
    return status;
}

static int lookup_entry(
    IN nfs41_root *root,
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    OUT nfs41_readdir_entry *entry)
{
    nfs41_abs_path path;
    nfs41_component name;
    int status;

    name.name = entry->name;
    name.len = (unsigned short)entry->name_len - 1;

    status = format_abs_path(parent->path, &name, &path);
    if (status) goto out;

    status = nfs41_lookup(root, session, &path,
        NULL, NULL, &entry->attr_info, NULL);
    if (status) goto out;
out:
    return status;
}

static int lookup_symlink(
    IN nfs41_root *root,
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN const nfs41_component *name,
    OUT nfs41_file_info *info_out)
{
    nfs41_abs_path path;
    nfs41_path_fh file;
    nfs41_file_info info;
    int status;

    status = format_abs_path(parent->path, name, &path);
    if (status) goto out;

    file.path = &path;
    status = nfs41_lookup(root, session, &path, NULL, &file, &info, &session);
    if (status) goto out;

    last_component(path.path, path.path + path.len, &file.name);

    status = nfs41_symlink_follow(root, session, &file, &info);
    if (status) goto out;

    info_out->symlink_dir = info.type == NF4DIR;
out:
    return status;
}

static int readdir_copy_entry(
    IN readdir_upcall_args *args,
    IN nfs41_readdir_entry *entry,
    IN OUT unsigned char **dst_pos,
    IN OUT uint32_t *dst_len)
{
    int status = 0;
    WCHAR wname[NFS4_OPAQUE_LIMIT];
    uint32_t wname_len, wname_size, needed;
    PFILE_DIR_INFO_UNION info;

    wname_len = MultiByteToWideChar(CP_UTF8, 0,
        entry->name, entry->name_len, wname, NFS4_OPAQUE_LIMIT);
    wname_size = (wname_len - 1) * sizeof(WCHAR);

    needed = readdir_size_for_entry(args->query_class, wname_size);
    if (!needed || needed > *dst_len) {
        status = -1;
        goto out;
    }

    info = (PFILE_DIR_INFO_UNION)*dst_pos;
    info->NextEntryOffset = align8(needed);
    *dst_pos += info->NextEntryOffset;
    *dst_len -= info->NextEntryOffset;

    if (entry->attr_info.rdattr_error == NFS4ERR_MOVED) {
        entry->attr_info.type = NF4DIR; /* default to dir */
        /* look up attributes for referral entries, but ignore return value;
         * it's okay if lookup fails, we'll just write garbage attributes */
        lookup_entry(args->root, args->state->session,
            &args->state->file, entry);
    } else if (entry->attr_info.type == NF4LNK) {
        nfs41_component name;
        name.name = entry->name;
        name.len = (unsigned short)entry->name_len - 1;
        /* look up the symlink target to see whether it's a directory */
        lookup_symlink(args->root, args->state->session,
            &args->state->file, &name, &entry->attr_info);
    }

    switch (args->query_class)
    {
    case FileNamesInformation:
        info->fni.FileIndex = 0;
        readdir_copy_filename(wname, wname_size,
            info->fni.FileName, &info->fni.FileNameLength);
        break;
    case FileDirectoryInformation:
        readdir_copy_dir_info(entry, info);
        readdir_copy_filename(wname, wname_size,
            info->fdi.FileName, &info->fdi.FileNameLength);
        break;
    case FileFullDirectoryInformation:
        readdir_copy_full_dir_info(entry, info);
        readdir_copy_filename(wname, wname_size,
            info->ffdi.FileName, &info->ffdi.FileNameLength);
        break;
    case FileIdFullDirectoryInformation:
        readdir_copy_full_dir_info(entry, info);
        info->fibdi.FileId.QuadPart = (LONGLONG)entry->attr_info.fileid;
        readdir_copy_filename(wname, wname_size,
            info->fifdi.FileName, &info->fifdi.FileNameLength);
        break;
    case FileBothDirectoryInformation:
        readdir_copy_both_dir_info(entry, wname, info);
        readdir_copy_filename(wname, wname_size,
            info->fbdi.FileName, &info->fbdi.FileNameLength);
        break;
    case FileIdBothDirectoryInformation:
        readdir_copy_both_dir_info(entry, wname, info);
        info->fibdi.FileId.QuadPart = (LONGLONG)entry->attr_info.fileid;
        readdir_copy_filename(wname, wname_size,
            info->fibdi.FileName, &info->fibdi.FileNameLength);
        break;
    default:
        eprintf("unhandled dir query class %d\n", args->query_class);
        status = -1;
        break;
    }
out:
    return status;
}

#define COOKIE_DOT      ((uint64_t)-2)
#define COOKIE_DOTDOT   ((uint64_t)-1)

static int readdir_add_dots(
    IN readdir_upcall_args *args,
    IN OUT unsigned char *entry_buf,
    IN uint32_t entry_buf_len,
    OUT uint32_t *len_out,
    OUT uint32_t **last_offset)
{
    int status = 0;
    const uint32_t entry_len = (uint32_t)FIELD_OFFSET(nfs41_readdir_entry, name);
    nfs41_readdir_entry *entry;
    nfs41_open_state *state = args->state;

    *len_out = 0;
    *last_offset = NULL;
    switch (state->cookie.cookie) {
    case 0:
        if (entry_buf_len < entry_len + 2) {
            status = ERROR_BUFFER_OVERFLOW;
            dprintf(1, "not enough room for '.' entry. received %d need %d\n",
                    entry_buf_len, entry_len + 2);
            args->query_reply_len = entry_len + 2;
            goto out;
        }

        entry = (nfs41_readdir_entry*)entry_buf;
        ZeroMemory(&entry->attr_info, sizeof(nfs41_file_info));

        status = nfs41_cached_getattr(state->session,
            &state->file, &entry->attr_info);
        if (status) {
            dprintf(1, "failed to add '.' entry.\n");
            goto out;
        }
        entry->cookie = COOKIE_DOT;
        entry->name_len = 2;
        StringCbCopyA(entry->name, entry->name_len, ".");
        entry->next_entry_offset = entry_len + entry->name_len;

        entry_buf += entry->next_entry_offset;
        entry_buf_len -= entry->next_entry_offset;
        *len_out += entry->next_entry_offset;
        *last_offset = &entry->next_entry_offset;
        if (args->single)
            break;
        /* else no break! */
    case COOKIE_DOT:
        if (entry_buf_len < entry_len + 3) {
            status = ERROR_BUFFER_OVERFLOW;
            dprintf(1, "not enough room for '..' entry. received %d need %d\n",
                    entry_buf_len, entry_len);
            args->query_reply_len = entry_len + 2;
            goto out;
        }
        /* XXX: this skips '..' when listing root fh */
        if (state->file.name.len == 0)
            break;

        entry = (nfs41_readdir_entry*)entry_buf;
        ZeroMemory(&entry->attr_info, sizeof(nfs41_file_info));

        status = nfs41_cached_getattr(state->session,
            &state->parent, &entry->attr_info);
        if (status) {
            status = ERROR_FILE_NOT_FOUND;
            dprintf(1, "failed to add '..' entry.\n");
            goto out;
        }
        entry->cookie = COOKIE_DOTDOT;
        entry->name_len = 3;
        StringCbCopyA(entry->name, entry->name_len, "..");
        entry->next_entry_offset = entry_len + entry->name_len;

        entry_buf += entry->next_entry_offset;
        entry_buf_len -= entry->next_entry_offset;
        *len_out += entry->next_entry_offset;
        *last_offset = &entry->next_entry_offset;
        break;
    }
    if (state->cookie.cookie == COOKIE_DOTDOT ||
        state->cookie.cookie == COOKIE_DOT)
        ZeroMemory(&state->cookie, sizeof(nfs41_readdir_cookie));
out:
    return status;
}

static int handle_readdir(nfs41_upcall *upcall)
{
    int status;
    readdir_upcall_args *args = &upcall->args.readdir;
    nfs41_open_state *state = upcall->state_ref;
    unsigned char *entry_buf = NULL;
    uint32_t entry_buf_len;
    bitmap4 attr_request;
    bool_t eof;
    /* make sure we allocate enough space for one nfs41_readdir_entry */
    const uint32_t max_buf_len = max(args->buf_len,
        sizeof(nfs41_readdir_entry) + NFS41_MAX_COMPONENT_LEN);

    dprintf(1, "-> handle_nfs41_dirquery(%s,%d,%d,%d)\n",
        args->filter, args->initial, args->restart, args->single);

    args->query_reply_len = 0;

    if (args->initial || args->restart) {
        ZeroMemory(&state->cookie, sizeof(nfs41_readdir_cookie));
        if (!state->cookie.cookie)
            dprintf(1, "initializing the 1st readdir cookie\n");
        else if (args->restart)
            dprintf(1, "restarting; clearing previous cookie %llu\n",
                state->cookie.cookie);
        else if (args->initial)
            dprintf(1, "*** initial; clearing previous cookie %llu!\n",
                state->cookie.cookie);
    } else if (!state->cookie.cookie) {
        dprintf(1, "handle_nfs41_readdir: EOF\n");
        status = ERROR_NO_MORE_FILES;
        goto out;
    }

    entry_buf = calloc(max_buf_len, sizeof(unsigned char));
    if (entry_buf == NULL) {
        status = GetLastError();
        goto out_free_cookie;
    }
fetch_entries:
    entry_buf_len = max_buf_len;

    nfs41_superblock_getattr_mask(state->file.fh.superblock, &attr_request);
    attr_request.arr[0] |= FATTR4_WORD0_RDATTR_ERROR;

    if (strchr(args->filter, FILTER_STAR) || strchr(args->filter, FILTER_QM)) {
        /* use READDIR for wildcards */

        uint32_t dots_len = 0;
        uint32_t *dots_next_offset = NULL;

        if (args->filter[0] == '*' && args->filter[1] == '\0') {
            status = readdir_add_dots(args, entry_buf,
                entry_buf_len, &dots_len, &dots_next_offset);
            if (status)
                goto out_free_cookie;
            entry_buf_len -= dots_len;
        }

        if (dots_len && args->single) {
            dprintf(2, "skipping nfs41_readdir because the single query "
                "will use . or ..\n");
            entry_buf_len = 0;
            eof = 0;
        } else {
            dprintf(2, "calling nfs41_readdir with cookie %llu\n",
                state->cookie.cookie);
            status = nfs41_readdir(state->session, &state->file,
                &attr_request, &state->cookie, entry_buf + dots_len,
                &entry_buf_len, &eof);
            if (status) {
                dprintf(1, "nfs41_readdir failed with %s\n",
                    nfs_error_string(status));
                status = nfs_to_windows_error(status, ERROR_BAD_NET_RESP);
                goto out_free_cookie;
            }
        }

        if (!entry_buf_len && dots_next_offset)
            *dots_next_offset = 0;
        entry_buf_len += dots_len;
    } else {
        /* use LOOKUP for single files */
        nfs41_readdir_entry *entry = (nfs41_readdir_entry*)entry_buf;
        entry->cookie = 0;
        entry->name_len = (uint32_t)strlen(args->filter) + 1;
        StringCbCopyA(entry->name, entry->name_len, args->filter);
        entry->next_entry_offset = 0;

        status = lookup_entry(upcall->root_ref,
             state->session, &state->file, entry);
        if (status) {
            dprintf(1, "single_lookup failed with %d\n", status);
            goto out_free_cookie;
        }
        entry_buf_len = entry->name_len +
                FIELD_OFFSET(nfs41_readdir_entry, name);

        eof = 1;
    }

    status = args->initial ? ERROR_FILE_NOT_FOUND : ERROR_NO_MORE_FILES;

    if (entry_buf_len) {
        unsigned char *entry_pos = entry_buf;
        unsigned char *dst_pos = args->kbuf;
        uint32_t dst_len = args->buf_len;
        nfs41_readdir_entry *entry;
        PULONG offset, last_offset = NULL;

        for (;;) {
            entry = (nfs41_readdir_entry*)entry_pos;
            offset = (PULONG)dst_pos; /* ULONG NextEntryOffset */

            dprintf(2, "filter %s looking at %s with cookie %d\n",
                args->filter, entry->name, entry->cookie);
            if (readdir_filter((const char*)args->filter, entry->name)) {
                if (readdir_copy_entry(args, entry, &dst_pos, &dst_len)) {
                    eof = 0;
                    dprintf(2, "not enough space to copy entry %s (cookie %d)\n",
                        entry->name, entry->cookie);
                    break;
                }
                last_offset = offset;
                status = NO_ERROR;
            }
            state->cookie.cookie = entry->cookie;

            /* last entry we got from the server */
            if (!entry->next_entry_offset)
                break;

            /* we found our single entry, but the server has more */
            if (args->single && last_offset) {
                eof = 0;
                break;
            }
            entry_pos += entry->next_entry_offset;
        }
        args->query_reply_len = args->buf_len - dst_len;
        if (last_offset) {
            *last_offset = 0;
        } else if (!eof) {
            dprintf(1, "no entries matched; fetch more\n");
            goto fetch_entries;
        }
    }

    if (eof) {
        dprintf(1, "we don't need to save a cookie\n");
        goto out_free_cookie;
    } else
        dprintf(1, "saving cookie %llu\n", state->cookie.cookie);

out_free_entry:
    free(entry_buf);
out:
    dprintf(1, "<- handle_nfs41_dirquery(%s,%d,%d,%d) returning ",
        args->filter, args->initial, args->restart, args->single);
    if (status) {
        switch (status) {
        case ERROR_FILE_NOT_FOUND:
            dprintf(1, "ERROR_FILE_NOT_FOUND.\n");
            break;
        case ERROR_NO_MORE_FILES:
            dprintf(1, "ERROR_NO_MORE_FILES.\n");
            break;
        case ERROR_BUFFER_OVERFLOW:
            upcall->last_error = status;
            status = ERROR_SUCCESS;
            break;
        default:
            dprintf(1, "error code %d.\n", status);
            break;
        }
    } else {
        dprintf(1, "success!\n");
    }
    return status;
out_free_cookie:
    state->cookie.cookie = 0;
    goto out_free_entry;
}

static int marshall_readdir(unsigned char *buffer, uint32_t *length, nfs41_upcall *upcall)
{
    int status;
    readdir_upcall_args *args = &upcall->args.readdir;

    status = safe_write(&buffer, length, &args->query_reply_len, sizeof(args->query_reply_len));
    return status;
}


const nfs41_upcall_op nfs41_op_readdir = {
    parse_readdir,
    handle_readdir,
    marshall_readdir
};
