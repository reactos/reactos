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
#include <stdio.h>

#include "daemon_debug.h"
#include "nfs41_ops.h"
#include "upcall.h"
#include "util.h"


/* NFS41_MOUNT */
static int parse_mount(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall) 
{
    int status;
    mount_upcall_args *args = &upcall->args.mount;

    status = get_name(&buffer, &length, &args->hostname);
    if(status) goto out;
    status = get_name(&buffer, &length, &args->path);
    if(status) goto out;
    status = safe_read(&buffer, &length, &args->sec_flavor, sizeof(DWORD));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->rsize, sizeof(DWORD));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->wsize, sizeof(DWORD));
    if (status) goto out;

    dprintf(1, "parsing NFS14_MOUNT: srv_name=%s root=%s sec_flavor=%s "
        "rsize=%d wsize=%d\n", args->hostname, args->path, 
        secflavorop2name(args->sec_flavor), args->rsize, args->wsize);
out:
    return status;
}

static int handle_mount(nfs41_upcall *upcall)
{
    int status;
    mount_upcall_args *args = &upcall->args.mount;
    nfs41_abs_path path;
    multi_addr4 addrs;
    nfs41_root *root;
    nfs41_client *client;
    nfs41_path_fh file;

    // resolve hostname,port
    status = nfs41_server_resolve(args->hostname, 2049, &addrs);
    if (status) {
        eprintf("nfs41_server_resolve() failed with %d\n", status);
        goto out;
    }

    if (upcall->root_ref != INVALID_HANDLE_VALUE) {
        /* use an existing root from a previous mount, but don't take an
         * extra reference; we'll only get one UNMOUNT upcall for each root */
        root = upcall->root_ref;
    } else {
        // create root
        status = nfs41_root_create(args->hostname, args->sec_flavor,
            args->wsize + WRITE_OVERHEAD, args->rsize + READ_OVERHEAD, &root);
        if (status) {
            eprintf("nfs41_root_create() failed %d\n", status);
            goto out;
        }
        root->uid = upcall->uid;
        root->gid = upcall->gid;
    }

    // find or create the client/session
    status = nfs41_root_mount_addrs(root, &addrs, 0, 0, &client);
    if (status) {
        eprintf("nfs41_root_mount_addrs() failed with %d\n", status);
        goto out_err;
    }

    // make a copy of the path for nfs41_lookup()
    InitializeSRWLock(&path.lock);
    if (FAILED(StringCchCopyA(path.path, NFS41_MAX_PATH_LEN, args->path))) {
        status = ERROR_FILENAME_EXCED_RANGE;
        goto out_err;
    }
    path.len = (unsigned short)strlen(path.path);

    // look up the mount path, and fail if it doesn't exist
    status = nfs41_lookup(root, client->session,
        &path, NULL, &file, NULL, NULL);
    if (status) {
        eprintf("nfs41_lookup('%s') failed with %d\n", path.path, status);
        status = ERROR_BAD_NETPATH;
        goto out_err;
    }

    nfs41_superblock_fs_attributes(file.fh.superblock, &args->FsAttrs);

    if (upcall->root_ref == INVALID_HANDLE_VALUE)
        nfs41_root_ref(root);
    upcall->root_ref = root;
    args->lease_time = client->session->lease_time;
out:
    return status;

out_err:
    if (upcall->root_ref == INVALID_HANDLE_VALUE)
        nfs41_root_deref(root);
    goto out;
}

static int marshall_mount(unsigned char *buffer, uint32_t *length, nfs41_upcall *upcall)
{
    mount_upcall_args *args = &upcall->args.mount;
    int status;
    dprintf(2, "NFS41_MOUNT: writing pointer to nfs41_root %p, version %d, "
        "lease_time %d\n", upcall->root_ref, NFS41D_VERSION, args->lease_time);
    status = safe_write(&buffer, length, &upcall->root_ref, sizeof(HANDLE));
    if (status) goto out;
    status = safe_write(&buffer, length, &NFS41D_VERSION, sizeof(DWORD));
    if (status) goto out;
    status = safe_write(&buffer, length, &args->lease_time, sizeof(DWORD));
    if (status) goto out;
    status = safe_write(&buffer, length, &args->FsAttrs, sizeof(args->FsAttrs));
out:
    return status;
}

static void cancel_mount(IN nfs41_upcall *upcall)
{
    if (upcall->root_ref != INVALID_HANDLE_VALUE)
        nfs41_root_deref(upcall->root_ref);
}

const nfs41_upcall_op nfs41_op_mount = {
    parse_mount,
    handle_mount,
    marshall_mount,
    cancel_mount
};


/* NFS41_UNMOUNT */
static int parse_unmount(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall) 
{
    dprintf(1, "parsing NFS41_UNMOUNT: root=%p\n", upcall->root_ref);
    return ERROR_SUCCESS;
}

static int handle_unmount(nfs41_upcall *upcall)
{
    /* release the original reference from nfs41_root_create() */
    nfs41_root_deref(upcall->root_ref);
    return ERROR_SUCCESS;
}

const nfs41_upcall_op nfs41_op_unmount = {
    parse_unmount,
    handle_unmount
};
