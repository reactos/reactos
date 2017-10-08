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

#include "nfs41_ops.h"
#include "upcall.h"
#include "util.h"
#include "daemon_debug.h"


static int abs_path_link(
    OUT nfs41_abs_path *path,
    IN char *path_pos,
    IN const char *link,
    IN uint32_t link_len)
{
    nfs41_component name;
    const char *path_max = path->path + NFS41_MAX_PATH_LEN;
    const char *link_pos = link;
    const char *link_end = link + link_len;
    int status = NO_ERROR;

    /* if link is an absolute path, start path_pos at the beginning */
    if (is_delimiter(*link))
        path_pos = path->path;

    /* copy each component of link into the path */
    while (next_component(link_pos, link_end, &name)) {
        link_pos = name.name + name.len;

        if (is_delimiter(*path_pos))
            path_pos++;

        /* handle special components . and .. */
        if (name.len == 1 && name.name[0] == '.')
            continue;
        if (name.len == 2 && name.name[0] == '.' && name.name[1] == '.') {
            /* back path_pos up by one component */
            if (!last_component(path->path, path_pos, &name)) {
                eprintf("symlink with .. that points below server root!\n");
                status = ERROR_BAD_NETPATH;
                goto out;
            }
            path_pos = (char*)prev_delimiter(name.name, path->path);
            continue;
        }

        /* copy the component and add a \ */
        if (FAILED(StringCchCopyNA(path_pos, path_max-path_pos, name.name, 
                name.len))) {
            status = ERROR_BUFFER_OVERFLOW;
            goto out;
        }
        path_pos += name.len;
        if (FAILED(StringCchCopyNA(path_pos, path_max-path_pos, "\\", 1))) {
            status = ERROR_BUFFER_OVERFLOW;
            goto out;
        }
    }

    /* make sure the path is null terminated */
    if (path_pos == path_max) {
        status = ERROR_BUFFER_OVERFLOW;
        goto out;
    }
    *path_pos = '\0';
out:
    path->len = (unsigned short)(path_pos - path->path);
    return status;
}

int nfs41_symlink_target(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    OUT nfs41_abs_path *target)
{
    char link[NFS41_MAX_PATH_LEN];
    const nfs41_abs_path *path = file->path;
    ptrdiff_t path_offset;
    uint32_t link_len;
    int status;

    /* read the link */
    status = nfs41_readlink(session, file, NFS41_MAX_PATH_LEN, link, &link_len);
    if (status) {
        eprintf("nfs41_readlink() for %s failed with %s\n", file->path->path, 
            nfs_error_string(status));
        status = ERROR_PATH_NOT_FOUND;
        goto out;
    }

    dprintf(2, "--> nfs41_symlink_target('%s', '%s')\n", path->path, link);

    /* append any components after the symlink */
    if (FAILED(StringCchCatA(link, NFS41_MAX_PATH_LEN,
            file->name.name + file->name.len))) {
        status = ERROR_BUFFER_OVERFLOW;
        goto out;
    }
    link_len = (uint32_t)strlen(link);

    /* overwrite the last component of the path; get the starting offset */
    path_offset = file->name.name - path->path;

    /* copy the path and update it with the results from link */
    if (target != path) {
        target->len = path->len;
        if (FAILED(StringCchCopyNA(target->path, NFS41_MAX_PATH_LEN,
                path->path, path->len))) {
            status = ERROR_BUFFER_OVERFLOW;
            goto out;
        }
    }
    status = abs_path_link(target, target->path + path_offset, link, link_len);
    if (status) {
        eprintf("abs_path_link() for path %s with link %s failed with %d\n", 
            target->path, link, status);
        goto out;
    }
out:
    dprintf(2, "<-- nfs41_symlink_target('%s') returning %d\n",
        target->path, status);
    return status;
}

int nfs41_symlink_follow(
    IN nfs41_root *root,
    IN nfs41_session *session,
    IN nfs41_path_fh *symlink,
    OUT nfs41_file_info *info)
{
    nfs41_abs_path path;
    nfs41_path_fh file;
    uint32_t depth = 0;
    int status = NO_ERROR;

    file.path = &path;
    InitializeSRWLock(&path.lock);

    dprintf(2, "--> nfs41_symlink_follow('%s')\n", symlink->path->path);

    do {
        if (++depth > NFS41_MAX_SYMLINK_DEPTH) {
            status = ERROR_TOO_MANY_LINKS;
            goto out;
        }

        /* construct the target path */
        status = nfs41_symlink_target(session, symlink, &path);
        if (status) goto out;

        dprintf(2, "looking up '%s'\n", path.path);

        last_component(path.path, path.path + path.len, &file.name);

        /* get attributes for the target */
        status = nfs41_lookup(root, session, &path,
            NULL, &file, info, &session);
        if (status) goto out;

        symlink = &file;
    } while (info->type == NF4LNK);
out:
    dprintf(2, "<-- nfs41_symlink_follow() returning %d\n", status);
    return status;
}


/* NFS41_SYMLINK */
static int parse_symlink(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall)
{
    symlink_upcall_args *args = &upcall->args.symlink;
    int status;

    status = get_name(&buffer, &length, &args->path);
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->set, sizeof(BOOLEAN));
    if (status) goto out;

    if (args->set)
        status = get_name(&buffer, &length, &args->target_set);
    else
        args->target_set = NULL;

    dprintf(1, "parsing NFS41_SYMLINK: path='%s' set=%u target='%s'\n",
        args->path, args->set, args->target_set);
out:
    return status;
}

static int handle_symlink(nfs41_upcall *upcall)
{
    symlink_upcall_args *args = &upcall->args.symlink;
    nfs41_open_state *state = upcall->state_ref;
    int status = NO_ERROR;

    if (args->set) {
        nfs41_file_info info, createattrs;

        /* don't send windows slashes to the server */
        char *p;
        for (p = args->target_set; *p; p++) if (*p == '\\') *p = '/';

        if (state->file.fh.len) {
            /* the check in handle_open() didn't catch that we're creating
             * a symlink, so we have to remove the file it already created */
            eprintf("handle_symlink: attempting to create a symlink when "
                "the file=%s was already created on open; sending REMOVE "
                "first\n", state->file.path->path);
            status = nfs41_remove(state->session, &state->parent,
                &state->file.name, state->file.fh.fileid);
            if (status) {
                eprintf("nfs41_remove() for symlink=%s failed with %s\n",
                    args->target_set, nfs_error_string(status));
                status = map_symlink_errors(status);
                goto out;
            }
        }

        /* create the symlink */
        createattrs.attrmask.count = 2;
        createattrs.attrmask.arr[0] = 0;
        createattrs.attrmask.arr[1] = FATTR4_WORD1_MODE;
        createattrs.mode = 0777;
        status = nfs41_create(state->session, NF4LNK, &createattrs,
            args->target_set, &state->parent, &state->file, &info);
        if (status) {
            eprintf("nfs41_create() for symlink=%s failed with %s\n",
                args->target_set, nfs_error_string(status));
            status = map_symlink_errors(status);
            goto out;
        }
    } else {
        uint32_t len;

        /* read the link */
        status = nfs41_readlink(state->session, &state->file,
            NFS41_MAX_PATH_LEN, args->target_get.path, &len);
        if (status) {
            eprintf("nfs41_readlink() for filename=%s failed with %s\n",
                state->file.path->path, nfs_error_string(status));
            status = map_symlink_errors(status);
            goto out;
        }
        args->target_get.len = (unsigned short)len;
        dprintf(2, "returning symlink target '%s'\n", args->target_get.path);
    }
out:
    return status;
}

static int marshall_symlink(unsigned char *buffer, uint32_t *length, nfs41_upcall *upcall)
{
    symlink_upcall_args *args = &upcall->args.symlink;
    unsigned short len = (args->target_get.len + 1) * sizeof(WCHAR);
    int status = NO_ERROR;

    if (args->set)
        goto out;

    status = safe_write(&buffer, length, &len, sizeof(len));
    if (status) goto out;

    if (*length <= len || !MultiByteToWideChar(CP_UTF8, 0,
            args->target_get.path, args->target_get.len,
            (LPWSTR)buffer, len / sizeof(WCHAR))) {
        status = ERROR_BUFFER_OVERFLOW;
        goto out;
    }
out:
    return status;
}


const nfs41_upcall_op nfs41_op_symlink = {
    parse_symlink,
    handle_symlink,
    marshall_symlink
};
