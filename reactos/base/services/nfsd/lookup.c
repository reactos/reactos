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
#include <time.h>

#include "nfs41_compound.h"
#include "nfs41_ops.h"
#include "name_cache.h"
#include "util.h"
#include "daemon_debug.h"


#define LULVL 2 /* dprintf level for lookup logging */


#define MAX_LOOKUP_COMPONENTS 8

/* map NFS4ERR_MOVED to an arbitrary windows error */
#define ERROR_FILESYSTEM_ABSENT ERROR_DEVICE_REMOVED

struct lookup_referral {
    nfs41_path_fh           parent;
    nfs41_component         name;
};


typedef struct __nfs41_lookup_component_args {
    nfs41_sequence_args     sequence;
    nfs41_putfh_args        putfh;
    nfs41_lookup_args       lookup[MAX_LOOKUP_COMPONENTS];
    nfs41_getattr_args      getrootattr;
    nfs41_getattr_args      getattr[MAX_LOOKUP_COMPONENTS];
    bitmap4                 attr_request;
} nfs41_lookup_component_args;

typedef struct __nfs41_lookup_component_res {
    nfs41_sequence_res      sequence;
    nfs41_putfh_res         putfh;
    nfs41_lookup_res        lookup[MAX_LOOKUP_COMPONENTS];
    nfs41_getfh_res         getrootfh;
    nfs41_getfh_res         getfh[MAX_LOOKUP_COMPONENTS];
    nfs41_path_fh           root;
    nfs41_path_fh           file[MAX_LOOKUP_COMPONENTS];
    nfs41_getattr_res       getrootattr;
    nfs41_getattr_res       getattr[MAX_LOOKUP_COMPONENTS];
    nfs41_file_info         rootinfo;
    nfs41_file_info         info[MAX_LOOKUP_COMPONENTS];
    struct lookup_referral  *referral;
} nfs41_lookup_component_res;


static void init_component_args(
    IN nfs41_lookup_component_args *args,
    IN nfs41_lookup_component_res *res,
    IN nfs41_abs_path *path,
    IN struct lookup_referral *referral)
{
    uint32_t i;

    args->attr_request.count = 2;
    args->attr_request.arr[0] = FATTR4_WORD0_TYPE
        | FATTR4_WORD0_CHANGE | FATTR4_WORD0_SIZE
        | FATTR4_WORD0_FSID | FATTR4_WORD0_FILEID
        | FATTR4_WORD0_HIDDEN | FATTR4_WORD0_ARCHIVE;
    args->attr_request.arr[1] = FATTR4_WORD1_MODE
        | FATTR4_WORD1_NUMLINKS | FATTR4_WORD1_SYSTEM
        | FATTR4_WORD1_TIME_ACCESS | FATTR4_WORD1_TIME_CREATE
        | FATTR4_WORD1_TIME_MODIFY;

    args->getrootattr.attr_request = &args->attr_request;
    res->root.path = path;
    res->getrootfh.fh = &res->root.fh;
    res->getrootattr.info = &res->rootinfo;
    res->getrootattr.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    res->referral = referral;

    for (i = 0; i < MAX_LOOKUP_COMPONENTS; i++) {
        args->getattr[i].attr_request = &args->attr_request;
        res->file[i].path = path;
        args->lookup[i].name = &res->file[i].name;
        res->getfh[i].fh = &res->file[i].fh;
        res->getattr[i].info = &res->info[i];
        res->getattr[i].obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    }
}

static int lookup_rpc(
    IN nfs41_session *session,
    IN nfs41_path_fh *dir,
    IN uint32_t component_count,
    IN nfs41_lookup_component_args *args,
    OUT nfs41_lookup_component_res *res)
{
    int status;
    uint32_t i;
    nfs41_compound compound;
    nfs_argop4 argops[4+MAX_LOOKUP_COMPONENTS*3];
    nfs_resop4 resops[4+MAX_LOOKUP_COMPONENTS*3];

    compound_init(&compound, argops, resops, "lookup");

    compound_add_op(&compound, OP_SEQUENCE, &args->sequence, &res->sequence);
    nfs41_session_sequence(&args->sequence, session, 0);

    if (dir == &res->root) {
        compound_add_op(&compound, OP_PUTROOTFH, NULL, &res->putfh);
        compound_add_op(&compound, OP_GETFH, NULL, &res->getrootfh);
        compound_add_op(&compound, OP_GETATTR, &args->getrootattr, 
            &res->getrootattr);
    } else {
        args->putfh.file = dir;
        compound_add_op(&compound, OP_PUTFH, &args->putfh, &res->putfh);
    }

    for (i = 0; i < component_count; i++) {
        compound_add_op(&compound, OP_LOOKUP, &args->lookup[i], &res->lookup[i]);
        compound_add_op(&compound, OP_GETFH, NULL, &res->getfh[i]);
        compound_add_op(&compound, OP_GETATTR, &args->getattr[i], 
            &res->getattr[i]);
    }

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

static int map_lookup_error(int status, bool_t last_component)
{
    switch (status) {
    case NFS4ERR_NOENT:
        if (last_component)     return ERROR_FILE_NOT_FOUND;
        else                    return ERROR_PATH_NOT_FOUND;
    case NFS4ERR_SYMLINK:       return ERROR_REPARSE;
    case NFS4ERR_MOVED:         return ERROR_FILESYSTEM_ABSENT;
    default: return nfs_to_windows_error(status, ERROR_FILE_NOT_FOUND);
    }
}

static int server_lookup(
    IN nfs41_session *session,
    IN nfs41_path_fh *dir,
    IN const char *path,
    IN const char *path_end,
    IN uint32_t count,
    IN nfs41_lookup_component_args *args,
    IN nfs41_lookup_component_res *res,
    OUT OPTIONAL nfs41_path_fh **parent_out,
    OUT OPTIONAL nfs41_path_fh **target_out,
    OUT OPTIONAL nfs41_file_info *info_out)
{
    nfs41_path_fh *file, *parent;
    uint32_t i = 0;
    int status;

    if (parent_out) *parent_out = NULL;
    if (target_out) *target_out = NULL;

    lookup_rpc(session, dir, count, args, res);

    status = res->sequence.sr_status;       if (status) goto out;
    status = res->putfh.status;             if (status) goto out;
    status = res->getrootfh.status;         if (status) goto out;
    status = res->getrootattr.status;       if (status) goto out;

    if (dir == &res->root) {
        nfs41_component name = { 0 };

        /* fill in the file handle's fileid and superblock */
        dir->fh.fileid = res->getrootattr.info->fileid;
        status = nfs41_superblock_for_fh(session,
            &res->getrootattr.info->fsid, NULL, dir);
        if (status)
            goto out;

        /* get the name of the parent (empty if its the root) */
        last_component(path, count ? args->lookup[0].name->name : path_end, &name);

        /* add the file handle and attributes to the name cache */
        memcpy(&res->getrootattr.info->attrmask,
            &res->getrootattr.obj_attributes.attrmask, sizeof(bitmap4));
        nfs41_name_cache_insert(session_name_cache(session), path, &name,
            &dir->fh, res->getrootattr.info, NULL, OPEN_DELEGATE_NONE);
    }
    file = dir;

    if (count == 0) {
        if (target_out)
            *target_out = dir;
        if (info_out)
            memcpy(info_out, res->getrootattr.info, sizeof(nfs41_file_info));
    } else if (count == 1) {
        if (parent_out)
            *parent_out = dir;
    }

    for (i = 0; i < count; i++) {
        if (res->lookup[i].status == NFS4ERR_SYMLINK) {
            /* return the symlink as the parent file */
            last_component(path, args->lookup[i].name->name, &file->name);
            if (parent_out) *parent_out = file;
        } else if (res->lookup[i].status == NFS4ERR_NOENT) {
            /* insert a negative lookup entry */
            nfs41_name_cache_insert(session_name_cache(session), path,
                args->lookup[i].name, NULL, NULL, NULL, OPEN_DELEGATE_NONE);
        }
        status = res->lookup[i].status;     if (status) break;

        if (res->getfh[i].status == NFS4ERR_MOVED) {
            /* save enough information to follow the referral */
            path_fh_copy(&res->referral->parent, file);
            res->referral->name.name = args->lookup[i].name->name;
            res->referral->name.len = args->lookup[i].name->len;
        }
        status = res->getfh[i].status;      if (status) break;
        status = res->getattr[i].status;    if (status) break;

        parent = file;
        file = &res->file[i];

        /* fill in the file handle's fileid and superblock */
        file->fh.fileid = res->getattr[i].info->fileid;
        status = nfs41_superblock_for_fh(session,
            &res->getattr[i].info->fsid, &parent->fh, file);
        if (status)
            break;

        /* add the file handle and attributes to the name cache */
        memcpy(&res->getattr[i].info->attrmask,
            &res->getattr[i].obj_attributes.attrmask, sizeof(bitmap4));
        nfs41_name_cache_insert(session_name_cache(session),
            path, args->lookup[i].name, &res->file[i].fh,
            res->getattr[i].info, NULL, OPEN_DELEGATE_NONE);

        if (i == count-1) {
            if (target_out)
                *target_out = file;
            if (info_out)
                memcpy(info_out, res->getattr[i].info, sizeof(nfs41_file_info));
        } else if (i == count-2) {
            if (parent_out)
                *parent_out = file;
        }
    }
out:
    return map_lookup_error(status, i == count-1);
}

static uint32_t max_lookup_components(
    IN const nfs41_session *session)
{
    const uint32_t comps = (session->fore_chan_attrs.ca_maxoperations - 4) / 3;
    return min(comps, MAX_LOOKUP_COMPONENTS);
}

static uint32_t get_component_array(
    IN OUT const char **path_pos,
    IN const char *path_end,
    IN uint32_t max_components,
    OUT nfs41_path_fh *components,
    OUT uint32_t *component_count)
{
    uint32_t i;

    for (i = 0; i < max_components; i++) {
        if (!next_component(*path_pos, path_end, &components[i].name))
            break;
        *path_pos = components[i].name.name + components[i].name.len;
    }

    *component_count = i;
    return i;
}

static int server_lookup_loop(
    IN nfs41_session *session,
    IN OPTIONAL nfs41_path_fh *parent_in,
    IN nfs41_abs_path *path,
    IN const char *path_pos,
    IN struct lookup_referral *referral,
    OUT OPTIONAL nfs41_path_fh *parent_out,
    OUT OPTIONAL nfs41_path_fh *target_out,
    OUT OPTIONAL nfs41_file_info *info_out)
{
    nfs41_lookup_component_args args = { 0 };
    nfs41_lookup_component_res res = { 0 };
    nfs41_path_fh *dir, *parent, *target;
    const char *path_end;
    const uint32_t max_components = max_lookup_components(session);
    uint32_t count;
    int status = NO_ERROR;

    init_component_args(&args, &res, path, referral);
    parent = NULL;
    target = NULL;

    path_end = path->path + path->len;
    dir = parent_in ? parent_in : &res.root;

    while (get_component_array(&path_pos, path_end,
        max_components, res.file, &count)) {

        status = server_lookup(session, dir, path->path, path_end, count,
            &args, &res, &parent, &target, info_out);

        if (status == ERROR_REPARSE) {
            /* copy the component name of the symlink */
            if (parent_out && parent) {
                const ptrdiff_t offset = parent->name.name - path->path;
                parent_out->name.name = parent_out->path->path + offset;
                parent_out->name.len = parent->name.len;
            }
            goto out_parent;
        }
        if (status == ERROR_FILE_NOT_FOUND && is_last_component(path_pos, path_end))
            goto out_parent;
        if (status)
            goto out;

        dir = target;
    }

    if (dir == &res.root && (target_out || info_out)) {
        /* didn't get any components, so we just need the root */
        status = server_lookup(session, dir, path->path, path_end,
            0, &args, &res, &parent, &target, info_out);
        if (status)
            goto out;
    }

    if (target_out && target) fh_copy(&target_out->fh, &target->fh);
out_parent:
    if (parent_out && parent) fh_copy(&parent_out->fh, &parent->fh);
out:
    return status;
}


static void referral_locations_free(
    IN fs_locations4 *locations)
{
    uint32_t i;
    if (locations->locations) {
        for (i = 0; i < locations->location_count; i++)
            free(locations->locations[i].servers);
        free(locations->locations);
    }
}

static int referral_resolve(
    IN nfs41_root *root,
    IN nfs41_session *session_in,
    IN struct lookup_referral *referral,
    OUT nfs41_abs_path *path_out,
    OUT nfs41_session **session_out)
{
    char rest_of_path[NFS41_MAX_PATH_LEN];
    fs_locations4 locations = { 0 };
    const fs_location4 *location;
    nfs41_client *client;
    int status;

    /* get fs_locations */
    status = nfs41_fs_locations(session_in, &referral->parent,
        &referral->name, &locations);
    if (status) {
        eprintf("nfs41_fs_locations() failed with %s\n",
            nfs_error_string(status));
        status = nfs_to_windows_error(status, ERROR_PATH_NOT_FOUND);
        goto out;
    }

    /* mount the first location available */
    status = nfs41_root_mount_referral(root, &locations, &location, &client);
    if (status) {
        eprintf("nfs41_root_mount_referral() failed with %d\n",
            status);
        goto out;
    }

    /* format a new path from that location's root */
    if (FAILED(StringCchCopyA(rest_of_path, NFS41_MAX_PATH_LEN,
            referral->name.name + referral->name.len))) {
        status = ERROR_FILENAME_EXCED_RANGE;
        goto out;
    }

    AcquireSRWLockExclusive(&path_out->lock);
    abs_path_copy(path_out, &location->path);
    if (FAILED(StringCchCatA(path_out->path, NFS41_MAX_PATH_LEN, rest_of_path)))
        status = ERROR_FILENAME_EXCED_RANGE;
    path_out->len = path_out->len + (unsigned short)strlen(rest_of_path);
    ReleaseSRWLockExclusive(&path_out->lock);

    if (session_out) *session_out = client->session;
out:
    referral_locations_free(&locations);
    return status;
}

int nfs41_lookup(
    IN nfs41_root *root,
    IN nfs41_session *session,
    IN OUT nfs41_abs_path *path_inout,
    OUT OPTIONAL nfs41_path_fh *parent_out,
    OUT OPTIONAL nfs41_path_fh *target_out,
    OUT OPTIONAL nfs41_file_info *info_out,
    OUT nfs41_session **session_out)
{
    nfs41_abs_path path;
    struct nfs41_name_cache *cache = session_name_cache(session);
    nfs41_path_fh parent, target, *server_start;
    const char *path_pos, *path_end;
    struct lookup_referral referral;
    bool_t negative = 0;
    int status;

    if (session_out) *session_out = session;

    InitializeSRWLock(&path.lock);

    /* to avoid holding this lock over multiple rpcs,
     * make a copy of the path and use that instead */
    AcquireSRWLockShared(&path_inout->lock);
    abs_path_copy(&path, path_inout);
    ReleaseSRWLockShared(&path_inout->lock);

    path_pos = path.path;
    path_end = path.path + path.len;

    dprintf(LULVL, "--> nfs41_lookup('%s')\n", path.path);

    if (parent_out == NULL) parent_out = &parent;
    if (target_out == NULL) target_out = &target;
    parent_out->fh.len = target_out->fh.len = 0;

    status = nfs41_name_cache_lookup(cache, path_pos, path_end, &path_pos,
        &parent_out->fh, &target_out->fh, info_out, &negative);
    if (status == NO_ERROR || negative)
        goto out;

    if (parent_out->fh.len) {
        /* start where the name cache left off */
        if (&parent != parent_out) {
            /* must make a copy for server_start, because
             * server_lookup_loop() will overwrite parent_out */
            path_fh_copy(&parent, parent_out);
        }
        server_start = &parent;
    } else {
        /* start with PUTROOTFH */
        server_start = NULL;
    }

    status = server_lookup_loop(session, server_start,
        &path, path_pos, &referral, parent_out, target_out, info_out);

    if (status == ERROR_FILESYSTEM_ABSENT) {
        nfs41_session *new_session;

        /* create a session to the referred server and
         * reformat the path relative to that server's root */
        status = referral_resolve(root, session,
            &referral, path_inout, &new_session);
        if (status) {
            eprintf("referral_resolve() failed with %d\n", status);
            goto out;
        }

        /* update the positions of the parent and target components */
        last_component(path_inout->path, path_inout->path + path_inout->len,
            &target_out->name);
        last_component(path_inout->path, target_out->name.name,
            &parent_out->name);

        if (session_out) *session_out = new_session;

        /* look up the new path */
        status = nfs41_lookup(root, new_session, path_inout,
            parent_out, target_out, info_out, session_out);
    }
out:
    dprintf(LULVL, "<-- nfs41_lookup() returning %d\n", status);
    return status;
}
