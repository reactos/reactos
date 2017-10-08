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
#include <stdlib.h>
#include <time.h>

#include "nfs41_ops.h"
#include "nfs41_compound.h"
#include "nfs41_xdr.h"
#include "name_cache.h"
#include "delegation.h"
#include "daemon_debug.h"
#include "util.h"

int nfs41_exchange_id(
    IN nfs41_rpc_clnt *rpc,
    IN client_owner4 *owner,
    IN uint32_t flags_in,
    OUT nfs41_exchange_id_res *res_out)
{
    int status = 0;
    nfs41_compound compound;
    nfs_argop4 argop;
    nfs_resop4 resop;
    nfs41_exchange_id_args ex_id;

    compound_init(&compound, &argop, &resop, "exchange_id");

    compound_add_op(&compound, OP_EXCHANGE_ID, &ex_id, res_out);
    ex_id.eia_clientowner = owner;
    ex_id.eia_flags = flags_in;
    ex_id.eia_state_protect.spa_how = SP4_NONE;
    ex_id.eia_client_impl_id = NULL;

    res_out->server_owner.so_major_id_len = NFS4_OPAQUE_LIMIT;
    res_out->server_scope_len = NFS4_OPAQUE_LIMIT;

    status = nfs41_send_compound(rpc, (char *)&compound.args,
        (char *)&compound.res);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

// AGLO: 10/07/2009 we might want lookup these values from the registry
static int set_fore_channel_attrs(
    IN nfs41_rpc_clnt *rpc,
    IN uint32_t max_req,
    OUT nfs41_channel_attrs *attrs)
{
    attrs->ca_headerpadsize = 0;
    attrs->ca_maxrequestsize = rpc->wsize;
    attrs->ca_maxresponsesize = rpc->rsize;
    attrs->ca_maxresponsesize_cached = NFS41_MAX_SERVER_CACHE;
    attrs->ca_maxoperations = 0xffffffff;
    attrs->ca_maxrequests = max_req;
    attrs->ca_rdma_ird = NULL;
    return 0;
}

// AGLO: 10/07/2009 we might want lookup these values from the registry
static int set_back_channel_attrs(
    IN nfs41_rpc_clnt *rpc,
    IN uint32_t max_req,
    OUT nfs41_channel_attrs *attrs)
{
    attrs->ca_headerpadsize = 0;
    attrs->ca_maxrequestsize = rpc->wsize;
    attrs->ca_maxresponsesize = rpc->rsize;
    attrs->ca_maxresponsesize_cached = NFS41_MAX_SERVER_CACHE;
    attrs->ca_maxoperations = 0xffffffff;
    attrs->ca_maxrequests = max_req;
    attrs->ca_rdma_ird = NULL;
    return 0;
}

int nfs41_create_session(nfs41_client *clnt, nfs41_session *session, bool_t try_recovery)
{
    int status = 0;
    nfs41_compound compound;
    nfs_argop4 argop;
    nfs_resop4 resop;
    nfs41_create_session_args req = { 0 };
    nfs41_create_session_res reply = { 0 };

    compound_init(&compound, &argop, &resop, "create_session");

    compound_add_op(&compound, OP_CREATE_SESSION, &req, &reply);

    AcquireSRWLockShared(&clnt->exid_lock);
    req.csa_clientid = clnt->clnt_id;
    req.csa_sequence = clnt->seq_id;
    ReleaseSRWLockShared(&clnt->exid_lock);
    req.csa_flags = session->flags;
    req.csa_cb_program = NFS41_RPC_CBPROGRAM;
    req.csa_cb_secparams[0].type = 0; /* AUTH_NONE */
    req.csa_cb_secparams[1].type = 1; /* AUTH_SYS */
    req.csa_cb_secparams[1].u.auth_sys.machinename = clnt->rpc->server_name;
    req.csa_cb_secparams[1].u.auth_sys.stamp = (uint32_t)time(NULL);

    // ca_maxrequests should be gotten from the rpc layer
    set_fore_channel_attrs(clnt->rpc,
        NFS41_MAX_RPC_REQS, &req.csa_fore_chan_attrs);
    set_back_channel_attrs(clnt->rpc,
        1, &req.csa_back_chan_attrs);
    
    reply.csr_sessionid = session->session_id;
    reply.csr_fore_chan_attrs = &session->fore_chan_attrs;
    reply.csr_back_chan_attrs = &session->back_chan_attrs;

    status = compound_encode_send_decode(session, &compound, try_recovery);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    print_hexbuf(1, (unsigned char *)"session id: ", session->session_id, NFS4_SESSIONID_SIZE);
    // check that csa_sequence is same as csr_sequence
    if (reply.csr_sequence != clnt->seq_id) {
        eprintf("ERROR: CREATE_SESSION: csa_sequence %d != "
            "csr_sequence %d\n", clnt->seq_id, reply.csr_sequence);
        status = NFS4ERR_SEQ_MISORDERED;
        goto out;
    } else clnt->seq_id++;

    if (reply.csr_flags != req.csa_flags) {
        eprintf("WARNING: requested session flags %x received %x\n",
            req.csa_flags, reply.csr_flags);
        if ((session->flags & CREATE_SESSION4_FLAG_CONN_BACK_CHAN) &&
                !(reply.csr_flags & CREATE_SESSION4_FLAG_CONN_BACK_CHAN))
            eprintf("WARNING: we asked to use this session for callbacks but "
                    "server refused\n");
        if ((session->flags & CREATE_SESSION4_FLAG_PERSIST) &&
            !(reply.csr_flags & CREATE_SESSION4_FLAG_PERSIST))
            eprintf("WARNING: we asked for persistent session but "
                    "server refused\n");
        session->flags = reply.csr_flags;
    }
    else
        dprintf(1, "session flags %x\n", reply.csr_flags);

    dprintf(1, "session fore_chan_attrs:\n"
        "  %-32s%d\n  %-32s%d\n  %-32s%d\n  %-32s%d\n  %-32s%d\n  %-32s%d\n",
        "headerpadsize", session->fore_chan_attrs.ca_headerpadsize,
        "maxrequestsize", session->fore_chan_attrs.ca_maxrequestsize,
        "maxresponsesize", session->fore_chan_attrs.ca_maxresponsesize,
        "maxresponsesize_cached", session->fore_chan_attrs.ca_maxresponsesize_cached,
        "maxoperations", session->fore_chan_attrs.ca_maxoperations,
        "maxrequests", session->fore_chan_attrs.ca_maxrequests);
    dprintf(1, "client supports %d max rpc slots, but server has %d\n", 
        session->table.max_slots, session->fore_chan_attrs.ca_maxrequests);
    /* use the server's ca_maxrequests unless it's bigger than our array */
    session->table.max_slots = min(session->table.max_slots,
        session->fore_chan_attrs.ca_maxrequests);
    status = 0;
out:
    return status;
}

enum nfsstat4 nfs41_bind_conn_to_session(
    IN nfs41_rpc_clnt *rpc,
    IN const unsigned char *sessionid,
    IN enum channel_dir_from_client4 dir)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argop;
    nfs_resop4 resop;
    nfs41_bind_conn_to_session_args bind_args = { 0 };
    nfs41_bind_conn_to_session_res bind_res = { 0 };

    compound_init(&compound, &argop, &resop, "bind_conn_to_session");

    compound_add_op(&compound, OP_BIND_CONN_TO_SESSION, &bind_args, &bind_res);
    bind_args.sessionid = (unsigned char *)sessionid;
    bind_args.dir = dir;

    status = nfs41_send_compound(rpc,
        (char*)&compound.args, (char*)&compound.res);
    if (status)
        goto out;

    compound_error(status = compound.res.status);

out:
    return status;
}

int nfs41_destroy_session(
    IN nfs41_session *session)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argop;
    nfs_resop4 resop;
    nfs41_destroy_session_args ds_args;
    nfs41_destroy_session_res ds_res;

    compound_init(&compound, &argop, &resop, "destroy_session");

    compound_add_op(&compound, OP_DESTROY_SESSION, &ds_args, &ds_res);
    ds_args.dsa_sessionid = session->session_id;

    /* don't attempt to recover from BADSESSION/STALE_CLIENTID */
    status = compound_encode_send_decode(session, &compound, FALSE);
    if (status)
        goto out;

    status = compound.res.status;
    if (status)
        eprintf("%s failed with status %d.\n",
            nfs_opnum_to_string(OP_DESTROY_SESSION), status);
out:
    return status;
}

int nfs41_destroy_clientid(
    IN nfs41_rpc_clnt *rpc,
    IN uint64_t clientid)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops;
    nfs_resop4 resops;
    nfs41_destroy_clientid_args dc_args;
    nfs41_destroy_clientid_res dc_res;

    compound_init(&compound, &argops, &resops, "destroy_clientid");

    compound_add_op(&compound, OP_DESTROY_CLIENTID, &dc_args, &dc_res);
    dc_args.dca_clientid = clientid;

    status = nfs41_send_compound(rpc, (char *)&compound.args,
        (char *)&compound.res);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

enum nfsstat4 nfs41_reclaim_complete(
    IN nfs41_session *session)
{
    enum nfsstat4 status = NFS4_OK;
    nfs41_compound compound;
    nfs_argop4 argops[2];
    nfs_resop4 resops[2];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_reclaim_complete_res reclaim_res;

    compound_init(&compound, argops, resops, "reclaim_complete");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_RECLAIM_COMPLETE, NULL, &reclaim_res);

    /* don't attempt to recover from BADSESSION */
    status = compound_encode_send_decode(session, &compound, FALSE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

static void open_delegation_return(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN open_delegation4 *delegation,
    IN bool_t try_recovery)
{
    stateid_arg stateid;
    int status;

    if (delegation->type == OPEN_DELEGATE_NONE ||
        delegation->type == OPEN_DELEGATE_NONE_EXT)
        return;

    /* return the delegation */
    stateid.open = NULL;
    stateid.delegation = NULL;
    stateid.type = STATEID_DELEG_FILE;
    memcpy(&stateid.stateid, &delegation->stateid, sizeof(stateid4));

    status = nfs41_delegreturn(session, file, &stateid, try_recovery);

    /* clear the delegation type returned by nfs41_open() */
    delegation->type = OPEN_DELEGATE_NONE;
}

static void open_update_cache(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN nfs41_path_fh *file,
    IN bool_t try_recovery,
    IN open_delegation4 *delegation,
    IN bool_t already_delegated,
    IN change_info4 *changeinfo,
    IN nfs41_getattr_res *dir_attrs,
    IN nfs41_getattr_res *file_attrs)
{
    struct nfs41_name_cache *cache = session_name_cache(session);
    uint32_t status;

    /* update the attributes of the parent directory */
    memcpy(&dir_attrs->info->attrmask, &dir_attrs->obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(cache, parent->fh.fileid, dir_attrs->info);

    /* add the file handle and attributes to the name cache */
    memcpy(&file_attrs->info->attrmask, &file_attrs->obj_attributes.attrmask,
        sizeof(bitmap4));
retry_cache_insert:
    AcquireSRWLockShared(&file->path->lock);
    status = nfs41_name_cache_insert(cache, file->path->path, &file->name,
        &file->fh, file_attrs->info, changeinfo,
        already_delegated ? OPEN_DELEGATE_NONE : delegation->type);
    ReleaseSRWLockShared(&file->path->lock);

    if (status == ERROR_TOO_MANY_OPEN_FILES) {
        /* the cache won't accept any more delegations; ask the client to
         * return a delegation to free up a slot in the attribute cache */
        status = nfs41_client_delegation_return_lru(session->client);
        if (status == NFS4_OK)
            goto retry_cache_insert;
    }

    if (status && delegation->type != OPEN_DELEGATE_NONE) {
        /* if we can't make room in the cache, return this
         * delegation immediately to free resources on the server */
        open_delegation_return(session, file, delegation, try_recovery);
        goto retry_cache_insert;
    }
}

int nfs41_open(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN nfs41_path_fh *file,
    IN state_owner4 *owner,
    IN open_claim4 *claim,
    IN uint32_t allow,
    IN uint32_t deny,
    IN uint32_t create,
    IN uint32_t how_mode,
    IN OPTIONAL nfs41_file_info *createattrs,
    IN bool_t try_recovery,
    OUT stateid4 *stateid,
    OUT open_delegation4 *delegation,
    OUT OPTIONAL nfs41_file_info *info)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[8];
    nfs_resop4 resops[8];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args[2];
    nfs41_putfh_res putfh_res[2];
    nfs41_op_open_args open_args;
    nfs41_op_open_res open_res;
    nfs41_getfh_res getfh_res;
    bitmap4 attr_request;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res, pgetattr_res;
    nfs41_savefh_res savefh_res;
    nfs41_restorefh_res restorefh_res;
    nfs41_file_info tmp_info, dir_info;
    bool_t current_fh_is_dir;
    bool_t already_delegated = delegation->type == OPEN_DELEGATE_READ
        || delegation->type == OPEN_DELEGATE_WRITE;

    /* depending on the claim type, OPEN expects CURRENT_FH set
     * to either the parent directory, or to the file itself */
    switch (claim->claim) {
    case CLAIM_NULL:
    case CLAIM_DELEGATE_CUR:
    case CLAIM_DELEGATE_PREV:
        /* CURRENT_FH: directory */
        current_fh_is_dir = TRUE;
        /* SEQUENCE; PUTFH(dir); SAVEFH; OPEN;
         * GETFH(file); GETATTR(file); RESTOREFH(dir); GETATTR */
        nfs41_superblock_getattr_mask(parent->fh.superblock, &attr_request);
        break;
    case CLAIM_PREVIOUS:
    case CLAIM_FH:
    case CLAIM_DELEG_CUR_FH:
    case CLAIM_DELEG_PREV_FH:
    default:
        /* CURRENT_FH: file being opened */
        current_fh_is_dir = FALSE;
        /* SEQUENCE; PUTFH(file); OPEN; GETATTR(file); PUTFH(dir); GETATTR */
        nfs41_superblock_getattr_mask(file->fh.superblock, &attr_request);
        break;
    }

    if (info == NULL)
        info = &tmp_info;

    attr_request.arr[0] |= FATTR4_WORD0_FSID;

    compound_init(&compound, argops, resops, "open");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 1);

    if (current_fh_is_dir) {
        /* CURRENT_FH: directory */
        compound_add_op(&compound, OP_PUTFH, &putfh_args[0], &putfh_res[0]);
        putfh_args[0].file = parent;
        putfh_args[0].in_recovery = 0;

        compound_add_op(&compound, OP_SAVEFH, NULL, &savefh_res);
    } else {
        /* CURRENT_FH: file being opened */
        compound_add_op(&compound, OP_PUTFH, &putfh_args[0], &putfh_res[0]);
        putfh_args[0].file = file;
        putfh_args[0].in_recovery = 0;
    }

    compound_add_op(&compound, OP_OPEN, &open_args, &open_res);
    open_args.seqid = 0;
#ifdef DISABLE_FILE_DELEGATIONS
    open_args.share_access = allow | OPEN4_SHARE_ACCESS_WANT_NO_DELEG;
#else
    open_args.share_access = allow;
#endif
    open_args.share_deny = deny; 
    open_args.owner = owner;
    open_args.openhow.opentype = create;
    open_args.openhow.how.mode = how_mode;
    open_args.openhow.how.createattrs = createattrs;
    if (how_mode == EXCLUSIVE4_1) {
        DWORD tid = GetCurrentThreadId();
        time((time_t*)open_args.openhow.how.createverf);
        memcpy(open_args.openhow.how.createverf+4, &tid, sizeof(tid));
        /* mask unsupported attributes */
        nfs41_superblock_supported_attrs_exclcreat(
            parent->fh.superblock, &createattrs->attrmask);
    } else if (createattrs) {
        /* mask unsupported attributes */
        nfs41_superblock_supported_attrs(
            parent->fh.superblock, &createattrs->attrmask);
    }
    open_args.claim = claim;
    open_res.resok4.stateid = stateid;
    open_res.resok4.delegation = delegation;

    if (current_fh_is_dir) {
        compound_add_op(&compound, OP_GETFH, NULL, &getfh_res);
        getfh_res.fh = &file->fh;
    }

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = &attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = info;

    if (current_fh_is_dir) {
        compound_add_op(&compound, OP_RESTOREFH, NULL, &restorefh_res);
    } else {
        compound_add_op(&compound, OP_PUTFH, &putfh_args[1], &putfh_res[1]);
        putfh_args[1].file = parent;
        putfh_args[1].in_recovery = 0;
    }

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &pgetattr_res);
    getattr_args.attr_request = &attr_request;
    pgetattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    pgetattr_res.info = &dir_info;

    status = compound_encode_send_decode(session, &compound, try_recovery);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    if (dir_info.type == NF4ATTRDIR) {
        file->fh.superblock = parent->fh.superblock;
        goto out;
    }

    /* fill in the file handle's fileid and superblock */
    file->fh.fileid = info->fileid;
    status = nfs41_superblock_for_fh(session, &info->fsid, &parent->fh, file);
    if (status)
        goto out;

    if (create == OPEN4_CREATE)
        nfs41_superblock_space_changed(file->fh.superblock);

    /* update the name/attr cache with the results */
    open_update_cache(session, parent, file, try_recovery, delegation,
        already_delegated, &open_res.resok4.cinfo, &pgetattr_res, &getattr_res);
out:
    return status;
}

int nfs41_create(
    IN nfs41_session *session,
    IN uint32_t type,
    IN nfs41_file_info *createattrs,
    IN OPTIONAL const char *symlink,
    IN nfs41_path_fh *parent,
    OUT nfs41_path_fh *file,
    OUT nfs41_file_info *info)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[8];
    nfs_resop4 resops[8];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_create_args create_args;
    nfs41_create_res create_res;
    nfs41_getfh_res getfh_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res, pgetattr_res;
    bitmap4 attr_request;
    nfs41_file_info dir_info;
    nfs41_savefh_res savefh_res;
    nfs41_restorefh_res restorefh_res;

    nfs41_superblock_getattr_mask(parent->fh.superblock, &attr_request);

    compound_init(&compound, argops, resops, "create");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 1);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = parent;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_SAVEFH, NULL, &savefh_res);

    compound_add_op(&compound, OP_CREATE, &create_args, &create_res);
    create_args.objtype.type = type;
    if (type == NF4LNK) {
        create_args.objtype.u.lnk.linkdata = symlink;
        create_args.objtype.u.lnk.linkdata_len = (uint32_t)strlen(symlink);
    }
    create_args.name = &file->name;
    create_args.createattrs = createattrs;
    nfs41_superblock_supported_attrs(
                parent->fh.superblock, &createattrs->attrmask);

    compound_add_op(&compound, OP_GETFH, NULL, &getfh_res);
    getfh_res.fh = &file->fh;

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = &attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = info;

    compound_add_op(&compound, OP_RESTOREFH, NULL, &restorefh_res);

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &pgetattr_res);
    getattr_args.attr_request = &attr_request;
    pgetattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    pgetattr_res.info = &dir_info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    /* fill in the file handle's fileid and superblock */
    file->fh.fileid = info->fileid;
    file->fh.superblock = parent->fh.superblock;

    /* update the attributes of the parent directory */
    memcpy(&dir_info.attrmask, &pgetattr_res.obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        parent->fh.fileid, &dir_info);

    /* add the new file handle and attributes to the name cache */
    memcpy(&info->attrmask, &getattr_res.obj_attributes.attrmask,
        sizeof(bitmap4));
    AcquireSRWLockShared(&file->path->lock);
    nfs41_name_cache_insert(session_name_cache(session),
        file->path->path, &file->name, &file->fh,
        info, &create_res.cinfo, OPEN_DELEGATE_NONE);
    ReleaseSRWLockShared(&file->path->lock);

    nfs41_superblock_space_changed(file->fh.superblock);
out:
    return status;
}

int nfs41_close(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN stateid_arg *stateid)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_op_close_args close_args;
    nfs41_op_close_res close_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res;
    bitmap4 attr_request;
    nfs41_file_info info;

    nfs41_superblock_getattr_mask(file->fh.superblock, &attr_request);

    compound_init(&compound, argops, resops, "close");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 1);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_CLOSE, &close_args, &close_res);
    close_args.stateid = stateid;

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = &attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = &info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    if (info.type == NF4NAMEDATTR)
        goto out;

    /* update the attributes of the parent directory */
    memcpy(&info.attrmask, &getattr_res.obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        file->fh.fileid, &info);
out:
    return status;
}

int nfs41_write(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN stateid_arg *stateid,
    IN unsigned char *data,
    IN uint32_t data_len,
    IN uint64_t offset,
    IN enum stable_how4 stable,
    OUT uint32_t *bytes_written,
    OUT nfs41_write_verf *verf,
    OUT nfs41_file_info *cinfo)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_write_args write_args;
    nfs41_write_res write_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res = {0};
    bitmap4 attr_request;
    nfs41_file_info info = { 0 }, *pinfo;

    nfs41_superblock_getattr_mask(file->fh.superblock, &attr_request);

    compound_init(&compound, argops, resops,
        stateid->stateid.seqid == 0 ? "ds write" : "write");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_WRITE, &write_args, &write_res);
    write_args.stateid = stateid;
    write_args.offset = offset;
    write_args.stable = stable;
    write_args.data_len = data_len;
    write_args.data = data;
    write_res.resok4.verf = verf;

    if (cinfo) pinfo = cinfo;
    else pinfo = &info;

    if (stable != UNSTABLE4) {
        /* if the write is stable, we can't rely on COMMIT to update
         * the attribute cache, so we do the GETATTR here */
        compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
        getattr_args.attr_request = &attr_request;
        getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
        getattr_res.info = pinfo;
    }

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    if (stable != UNSTABLE4 && pinfo->type != NF4NAMEDATTR) {
        /* update the attribute cache */
        memcpy(&pinfo->attrmask, &getattr_res.obj_attributes.attrmask,
            sizeof(bitmap4));
        nfs41_attr_cache_update(session_name_cache(session),
            file->fh.fileid, pinfo);
    }

    *bytes_written = write_res.resok4.count;

    /* we shouldn't ever see this, but a buggy server could
     * send us into an infinite loop. return NFS4ERR_IO */
    if (!write_res.resok4.count) {
        status = NFS4ERR_IO;
        eprintf("WRITE succeeded with count=0; returning %s\n",
            nfs_error_string(status));
    }

    nfs41_superblock_space_changed(file->fh.superblock);
out:
    return status;
}

int nfs41_read(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN stateid_arg *stateid,
    IN uint64_t offset,
    IN uint32_t count,
    OUT unsigned char *data_out,
    OUT uint32_t *data_len_out,
    OUT bool_t *eof_out)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_read_args read_args;
    nfs41_read_res read_res;

    compound_init(&compound, argops, resops,
        stateid->stateid.seqid == 0 ? "ds read" : "read");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_READ, &read_args, &read_res);
    read_args.stateid = stateid;
    read_args.offset = offset;
    read_args.count = count;
    read_res.resok4.data_len = count;
    read_res.resok4.data = data_out;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    *data_len_out = read_res.resok4.data_len;
    *eof_out = read_res.resok4.eof;

    /* we shouldn't ever see this, but a buggy server could
     * send us into an infinite loop. return NFS4ERR_IO */
    if (!read_res.resok4.data_len && !read_res.resok4.eof) {
        status = NFS4ERR_IO;
        eprintf("READ succeeded with len=0 and eof=0; returning %s\n",
            nfs_error_string(status));
    }
out:
    return status;
}

int nfs41_commit(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN uint64_t offset,
    IN uint32_t count,
    IN bool_t do_getattr,
    OUT nfs41_write_verf *verf,
    OUT nfs41_file_info *cinfo)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_commit_args commit_args;
    nfs41_commit_res commit_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res = {0};
    bitmap4 attr_request;
    nfs41_file_info info, *pinfo;

    compound_init(&compound, argops, resops,
        do_getattr ? "commit" : "ds commit");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 1);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_COMMIT, &commit_args, &commit_res);
    commit_args.offset = offset;
    commit_args.count = count;
    commit_res.verf = verf;

    /* send a GETATTR request to update the attribute cache,
     * but not if we're talking to a data server! */
    if (cinfo) pinfo = cinfo;
    else pinfo = &info;
    if (do_getattr) {
        nfs41_superblock_getattr_mask(file->fh.superblock, &attr_request);

        compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
        getattr_args.attr_request = &attr_request;
        getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
        getattr_res.info = pinfo;
    }

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    if (do_getattr) {
        /* update the attribute cache */
        memcpy(&pinfo->attrmask, &getattr_res.obj_attributes.attrmask,
            sizeof(bitmap4));
        nfs41_attr_cache_update(session_name_cache(session),
            file->fh.fileid, pinfo);
    }
    nfs41_superblock_space_changed(file->fh.superblock);
out:
    return status;
}

int nfs41_lock(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN state_owner4 *owner,
    IN uint32_t type,
    IN uint64_t offset,
    IN uint64_t length,
    IN bool_t reclaim,
    IN bool_t try_recovery,
    IN OUT stateid_arg *stateid)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_lock_args lock_args;
    nfs41_lock_res lock_res;

    compound_init(&compound, argops, resops, "lock");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_LOCK, &lock_args, &lock_res);
    lock_args.locktype = type;
    lock_args.reclaim = reclaim;
    lock_args.offset = offset;
    lock_args.length = length;
    if (stateid->type == STATEID_LOCK) {
        lock_args.locker.new_lock_owner = 0;
        lock_args.locker.u.lock_owner.lock_stateid = stateid;
        lock_args.locker.u.lock_owner.lock_seqid = 0; /* ignored */
    } else {
        lock_args.locker.new_lock_owner = 1;
        lock_args.locker.u.open_owner.open_seqid = 0; /* ignored */
        lock_args.locker.u.open_owner.open_stateid = stateid;
        lock_args.locker.u.open_owner.lock_seqid = 0; /* ignored */
        lock_args.locker.u.open_owner.lock_owner = owner;
    }
    lock_res.u.resok4.lock_stateid = &stateid->stateid;
    lock_res.u.denied.owner.owner_len = NFS4_OPAQUE_LIMIT;

    status = compound_encode_send_decode(session, &compound, try_recovery);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    stateid->type = STATEID_LOCK; /* returning a lock stateid */
out:
    return status;
}

int nfs41_unlock(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN uint64_t offset,
    IN uint64_t length,
    IN OUT stateid_arg *stateid)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_locku_args locku_args;
    nfs41_locku_res locku_res;

    compound_init(&compound, argops, resops, "unlock");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_LOCKU, &locku_args, &locku_res);
    /* 18.12.3: the server MUST accept any legal value for locktype */
    locku_args.locktype = READ_LT;
    locku_args.offset = offset;
    locku_args.length = length;
    locku_args.lock_stateid = stateid;
    locku_res.lock_stateid = &stateid->stateid;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

int nfs41_readdir(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN bitmap4 *attr_request,
    IN nfs41_readdir_cookie *cookie,
    OUT unsigned char *entries,
    IN OUT uint32_t *entries_len,
    OUT bool_t *eof_out)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_readdir_args readdir_args;
    nfs41_readdir_res readdir_res;

    compound_init(&compound, argops, resops, "readdir");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_READDIR, &readdir_args, &readdir_res);
    readdir_args.cookie.cookie = cookie->cookie;
    memcpy(readdir_args.cookie.verf, cookie->verf, NFS4_VERIFIER_SIZE);
    readdir_args.dircount = *entries_len;
    readdir_args.maxcount = *entries_len + sizeof(nfs41_readdir_res);
    readdir_args.attr_request = attr_request;
    readdir_res.reply.entries_len = *entries_len;
    readdir_res.reply.entries = entries;
    ZeroMemory(entries, readdir_args.dircount);

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    *entries_len = readdir_res.reply.entries_len;
    *eof_out = readdir_res.reply.eof;
    memcpy(cookie->verf, readdir_res.cookieverf, NFS4_VERIFIER_SIZE);
out:
    return status;
}

int nfs41_getattr(
    IN nfs41_session *session,
    IN OPTIONAL nfs41_path_fh *file,
    IN bitmap4 *attr_request,
    OUT nfs41_file_info *info)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res;

    compound_init(&compound, argops, resops, "getattr");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    if (file) {
        compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
        putfh_args.file = file;
        putfh_args.in_recovery = 0;
    } else {
        compound_add_op(&compound, OP_PUTROOTFH, NULL, &putfh_res);
    }

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    if (file) {
        /* update the name cache with whatever attributes we got */
        memcpy(&info->attrmask, &getattr_res.obj_attributes.attrmask,
            sizeof(bitmap4));
        nfs41_attr_cache_update(session_name_cache(session),
            file->fh.fileid, info);
    }
out:
    return status;
}

int nfs41_superblock_getattr(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN bitmap4 *attr_request,
    OUT nfs41_file_info *info,
    OUT bool_t *supports_named_attrs)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res;
    nfs41_openattr_args openattr_args;
    nfs41_openattr_res openattr_res;

    compound_init(&compound, argops, resops, "getfsattr");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = info;

    compound_add_op(&compound, OP_OPENATTR, &openattr_args, &openattr_res);
    openattr_args.createdir = 0;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    status = sequence_res.sr_status;
    if (status) goto out;
    status = putfh_res.status;
    if (status) goto out;
    status = getattr_res.status;
    if (status) goto out;

    switch (status = openattr_res.status) {
    case NFS4ERR_NOTSUPP:
        *supports_named_attrs = 0;
        status = NFS4_OK;
        break;

    case NFS4ERR_NOENT:
    case NFS4_OK:
        *supports_named_attrs = 1;
        status = NFS4_OK;
        break;
    }
out:
    return status;
}

int nfs41_remove(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN const nfs41_component *target,
    IN uint64_t fileid)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_remove_args remove_args;
    nfs41_remove_res remove_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res;
    bitmap4 attr_request;
    nfs41_file_info info;

    nfs41_superblock_getattr_mask(parent->fh.superblock, &attr_request);

    compound_init(&compound, argops, resops, "remove");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 1);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = parent;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_REMOVE, &remove_args, &remove_res);
    remove_args.target = target;

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = &attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = &info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    if (info.type == NF4ATTRDIR)
        goto out;

    /* update the attributes of the parent directory */
    memcpy(&info.attrmask, &getattr_res.obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        parent->fh.fileid, &info);

    /* remove the target file from the cache */
    AcquireSRWLockShared(&parent->path->lock);
    nfs41_name_cache_remove(session_name_cache(session),
        parent->path->path, target, fileid, &remove_res.cinfo);
    ReleaseSRWLockShared(&parent->path->lock);

    nfs41_superblock_space_changed(parent->fh.superblock);
out:
    return status;
}

int nfs41_rename(
    IN nfs41_session *session,
    IN nfs41_path_fh *src_dir,
    IN const nfs41_component *src_name,
    IN nfs41_path_fh *dst_dir,
    IN const nfs41_component *dst_name)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[8];
    nfs_resop4 resops[8];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args src_putfh_args;
    nfs41_putfh_res src_putfh_res;
    nfs41_savefh_res savefh_res;
    nfs41_putfh_args dst_putfh_args;
    nfs41_putfh_res dst_putfh_res;
    nfs41_rename_args rename_args;
    nfs41_rename_res rename_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res src_getattr_res, dst_getattr_res;
    nfs41_file_info src_info, dst_info;
    bitmap4 attr_request;
    nfs41_restorefh_res restorefh_res;

    nfs41_superblock_getattr_mask(src_dir->fh.superblock, &attr_request);

    compound_init(&compound, argops, resops, "rename");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 1);

    compound_add_op(&compound, OP_PUTFH, &src_putfh_args, &src_putfh_res);
    src_putfh_args.file = src_dir;
    src_putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_SAVEFH, NULL, &savefh_res);

    compound_add_op(&compound, OP_PUTFH, &dst_putfh_args, &dst_putfh_res);
    dst_putfh_args.file = dst_dir;
    dst_putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_RENAME, &rename_args, &rename_res);
    rename_args.oldname = src_name;
    rename_args.newname = dst_name;
    
    compound_add_op(&compound, OP_GETATTR, &getattr_args, &dst_getattr_res);
    getattr_args.attr_request = &attr_request;
    dst_getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    dst_getattr_res.info = &dst_info;

    compound_add_op(&compound, OP_RESTOREFH, NULL, &restorefh_res);

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &src_getattr_res);
    src_getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    src_getattr_res.info = &src_info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    /* update the attributes of the source directory */
    memcpy(&src_info.attrmask, &src_getattr_res.obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        src_dir->fh.fileid, &src_info);

    /* update the attributes of the destination directory */
    memcpy(&dst_info.attrmask, &dst_getattr_res.obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        dst_dir->fh.fileid, &dst_info);

    if (src_dir->path == dst_dir->path) {
        /* source and destination are the same, only lock it once */
        AcquireSRWLockShared(&src_dir->path->lock);
    } else if (src_dir->path < dst_dir->path) {
        /* lock the lowest memory address first */
        AcquireSRWLockShared(&src_dir->path->lock);
        AcquireSRWLockShared(&dst_dir->path->lock);
    } else {
        AcquireSRWLockShared(&dst_dir->path->lock);
        AcquireSRWLockShared(&src_dir->path->lock);
    }

    /* move/rename the target file's name cache entry */
    nfs41_name_cache_rename(session_name_cache(session),
        src_dir->path->path, src_name, &rename_res.source_cinfo,
        dst_dir->path->path, dst_name, &rename_res.target_cinfo);

    if (src_dir->path == dst_dir->path) {
        ReleaseSRWLockShared(&src_dir->path->lock);
    } else {
        ReleaseSRWLockShared(&src_dir->path->lock);
        ReleaseSRWLockShared(&dst_dir->path->lock);
    }
out:
    return status;
}

int nfs41_setattr(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN stateid_arg *stateid,
    IN nfs41_file_info *info)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_setattr_args setattr_args;
    nfs41_setattr_res setattr_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res;
    bitmap4 attr_request;

    compound_init(&compound, argops, resops, "setattr");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_SETATTR, &setattr_args, &setattr_res);
    setattr_args.stateid = stateid;
    setattr_args.info = info;

    nfs41_superblock_getattr_mask(file->fh.superblock, &attr_request);
    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = &attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    memcpy(&info->attrmask, &attr_request, sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        file->fh.fileid, info);

    if (setattr_res.attrsset.arr[0] & FATTR4_WORD0_SIZE)
        nfs41_superblock_space_changed(file->fh.superblock);
out:
    return status;
}

int nfs41_link(
    IN nfs41_session *session,
    IN nfs41_path_fh *src,
    IN nfs41_path_fh *dst_dir,
    IN const nfs41_component *target,
    OUT nfs41_file_info *cinfo)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[9];
    nfs_resop4 resops[9];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args[2];
    nfs41_putfh_res putfh_res[2];
    nfs41_savefh_res savefh_res;
    nfs41_link_args link_args;
    nfs41_link_res link_res;
    nfs41_lookup_args lookup_args;
    nfs41_lookup_res lookup_res;
    nfs41_getfh_res getfh_res;
    nfs41_getattr_args getattr_args[2];
    nfs41_getattr_res getattr_res[2];
    nfs41_file_info info = { 0 };
    nfs41_path_fh file;

    nfs41_superblock_getattr_mask(src->fh.superblock, &info.attrmask);
    nfs41_superblock_getattr_mask(dst_dir->fh.superblock, &cinfo->attrmask);
    cinfo->attrmask.arr[0] |= FATTR4_WORD0_FSID;

    compound_init(&compound, argops, resops, "link");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 1);

    /* PUTFH(src) */
    compound_add_op(&compound, OP_PUTFH, &putfh_args[0], &putfh_res[0]);
    putfh_args[0].file = src;
    putfh_args[0].in_recovery = 0;

    compound_add_op(&compound, OP_SAVEFH, NULL, &savefh_res);

    /* PUTFH(dst_dir) */
    compound_add_op(&compound, OP_PUTFH, &putfh_args[1], &putfh_res[1]);
    putfh_args[1].file = dst_dir;
    putfh_args[1].in_recovery = 0;

    compound_add_op(&compound, OP_LINK, &link_args, &link_res);
    link_args.newname = target;

    /* GETATTR(dst_dir) */
    compound_add_op(&compound, OP_GETATTR, &getattr_args[0], &getattr_res[0]);
    getattr_args[0].attr_request = &info.attrmask;
    getattr_res[0].obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res[0].info = &info;

    /* LOOKUP(target) */
    compound_add_op(&compound, OP_LOOKUP, &lookup_args, &lookup_res);
    lookup_args.name = target;

    /* GETATTR(target) */
    compound_add_op(&compound, OP_GETATTR, &getattr_args[1], &getattr_res[1]);
    getattr_args[1].attr_request = &cinfo->attrmask;
    getattr_res[1].obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res[1].info = cinfo;

    /* GETFH(target) */
    compound_add_op(&compound, OP_GETFH, NULL, &getfh_res);
    getfh_res.fh = &file.fh;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    /* fill in the file handle's fileid and superblock */
    file.fh.fileid = cinfo->fileid;
    status = nfs41_superblock_for_fh(session,
        &cinfo->fsid, &dst_dir->fh, &file);
    if (status)
        goto out;

    /* update the attributes of the destination directory */
    memcpy(&info.attrmask, &getattr_res[0].obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        info.fileid, &info);

    /* add the new file handle and attributes to the name cache */
    memcpy(&cinfo->attrmask, &getattr_res[1].obj_attributes.attrmask,
        sizeof(bitmap4));
    AcquireSRWLockShared(&dst_dir->path->lock);
    nfs41_name_cache_insert(session_name_cache(session),
        dst_dir->path->path, target, &file.fh,
        cinfo, &link_res.cinfo, OPEN_DELEGATE_NONE);
    ReleaseSRWLockShared(&dst_dir->path->lock);

    nfs41_superblock_space_changed(dst_dir->fh.superblock);
out:
    return status;
}

int nfs41_readlink(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN uint32_t max_len,
    OUT char *link_out,
    OUT uint32_t *len_out)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_readlink_res readlink_res;

    compound_init(&compound, argops, resops, "readlink");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_READLINK, NULL, &readlink_res);
    readlink_res.link_len = max_len - 1;
    readlink_res.link = link_out;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    link_out[readlink_res.link_len] = '\0';
    *len_out = readlink_res.link_len;
out:
    return status;
}

int nfs41_access(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN uint32_t requested,
    OUT uint32_t *supported OPTIONAL,
    OUT uint32_t *access OPTIONAL)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_access_args access_args;
    nfs41_access_res access_res;

    compound_init(&compound, argops, resops, "access");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_ACCESS, &access_args, &access_res);
    access_args.access = requested;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    if (supported)
        *supported = access_res.supported;
    if (access)
        *access = access_res.access;
out:
    return status;
}

int nfs41_send_sequence(
    IN nfs41_session *session)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[1];
    nfs_resop4 resops[1];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;

    compound_init(&compound, argops, resops, "sequence");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;
out:
    return status;
}

enum nfsstat4 nfs41_want_delegation(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN deleg_claim4 *claim,
    IN uint32_t want,
    IN bool_t try_recovery,
    OUT open_delegation4 *delegation)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_want_delegation_args wd_args;
    nfs41_want_delegation_res wd_res;

    compound_init(&compound, argops, resops, "want_delegation");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_WANT_DELEGATION, &wd_args, &wd_res);
    wd_args.claim = claim;
    wd_args.want = want;
    wd_res.delegation = delegation;

    status = compound_encode_send_decode(session, &compound, try_recovery);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

int nfs41_delegpurge(
    IN nfs41_session *session)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[2];
    nfs_resop4 resops[2];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_delegpurge_res dp_res;

    compound_init(&compound, argops, resops, "delegpurge");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_DELEGPURGE, NULL, &dp_res);

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

int nfs41_delegreturn(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN stateid_arg *stateid,
    IN bool_t try_recovery)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_delegreturn_args dr_args;
    nfs41_delegreturn_res dr_res;

    compound_init(&compound, argops, resops, "delegreturn");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_DELEGRETURN, &dr_args, &dr_res);
    dr_args.stateid = stateid;

    status = compound_encode_send_decode(session, &compound, try_recovery);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    AcquireSRWLockShared(&file->path->lock);
    nfs41_name_cache_delegreturn(session_name_cache(session),
        file->fh.fileid, file->path->path, &file->name);
    ReleaseSRWLockShared(&file->path->lock);
out:
    return status;
}

enum nfsstat4 nfs41_fs_locations(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN const nfs41_component *name,
    OUT fs_locations4 *locations)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_lookup_args lookup_args;
    nfs41_lookup_res lookup_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res;
    bitmap4 attr_request = { 1, { FATTR4_WORD0_FS_LOCATIONS } };
    nfs41_file_info info;

    compound_init(&compound, argops, resops, "fs_locations");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = parent;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_LOOKUP, &lookup_args, &lookup_res);
    lookup_args.name = name;

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = &attr_request;
    info.fs_locations = locations;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = &info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

int nfs41_secinfo(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN const nfs41_component *name,
    OUT nfs41_secinfo_info *secinfo)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_secinfo_args secinfo_args;
    nfs41_secinfo_noname_res secinfo_res;

    compound_init(&compound, argops, resops, "secinfo");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    if (file == NULL) 
        compound_add_op(&compound, OP_PUTROOTFH, NULL, &putfh_res);
    else {
        compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
        putfh_args.file = file;
        putfh_args.in_recovery = 0;
    }

    compound_add_op(&compound, OP_SECINFO, &secinfo_args, &secinfo_res);
    secinfo_args.name = name;
    secinfo_res.secinfo = secinfo;

    status = compound_encode_send_decode(session, &compound, FALSE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

int nfs41_secinfo_noname(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    OUT nfs41_secinfo_info *secinfo)
{
    int status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_secinfo_noname_args noname_args;
    nfs41_secinfo_noname_res noname_res;

    compound_init(&compound, argops, resops, "secinfo_no_name");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    if (file == NULL) 
        compound_add_op(&compound, OP_PUTROOTFH, NULL, &putfh_res);
    else {
        compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
        putfh_args.file = file;
        putfh_args.in_recovery = 0;
    }

    compound_add_op(&compound, OP_SECINFO_NO_NAME, &noname_args, &noname_res);
    noname_args.type = SECINFO_STYLE4_CURRENT_FH;
    noname_res.secinfo = secinfo;

    status = compound_encode_send_decode(session, &compound, FALSE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

enum nfsstat4 nfs41_free_stateid(
    IN nfs41_session *session,
    IN stateid4 *stateid)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[2];
    nfs_resop4 resops[2];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_free_stateid_args freestateid_args;
    nfs41_free_stateid_res freestateid_res;

    compound_init(&compound, argops, resops, "free_stateid");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_FREE_STATEID, &freestateid_args, &freestateid_res);
    freestateid_args.stateid = stateid;

    status = compound_encode_send_decode(session, &compound, FALSE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

enum nfsstat4 nfs41_test_stateid(
    IN nfs41_session *session,
    IN stateid_arg *stateid_array,
    IN uint32_t count,
    OUT uint32_t *status_array)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[2];
    nfs_resop4 resops[2];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_test_stateid_args teststateid_args;
    nfs41_test_stateid_res teststateid_res;

    compound_init(&compound, argops, resops, "test_stateid");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_TEST_STATEID, &teststateid_args, &teststateid_res);
    teststateid_args.stateids = stateid_array;
    teststateid_args.count = count;
    teststateid_res.resok.status = status_array;
    teststateid_res.resok.count = count;

    status = compound_encode_send_decode(session, &compound, FALSE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

enum nfsstat4 pnfs_rpc_layoutget(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN stateid_arg *stateid,
    IN enum pnfs_iomode iomode,
    IN uint64_t offset,
    IN uint64_t minlength,
    IN uint64_t length,
    OUT pnfs_layoutget_res_ok *layoutget_res_ok)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    pnfs_layoutget_args layoutget_args;
    pnfs_layoutget_res layoutget_res = { 0 };
    uint32_t i;
    struct list_entry *entry;

    compound_init(&compound, argops, resops, "layoutget");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_LAYOUTGET, &layoutget_args, &layoutget_res);
    layoutget_args.signal_layout_avail = 0;
    layoutget_args.layout_type = PNFS_LAYOUTTYPE_FILE;
    layoutget_args.iomode = iomode;
    layoutget_args.offset = offset;
    layoutget_args.minlength = minlength;
    layoutget_args.length = length;
    layoutget_args.stateid = stateid;
    layoutget_args.maxcount = session->fore_chan_attrs.ca_maxresponsesize - READ_OVERHEAD;

    layoutget_res.u.res_ok = layoutget_res_ok;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    /* point each file handle to the meta server's superblock */
    list_for_each(entry, &layoutget_res_ok->layouts) {
        pnfs_layout *base = list_container(entry, pnfs_layout, entry);
        if (base->type == PNFS_LAYOUTTYPE_FILE) {
            pnfs_file_layout *layout = (pnfs_file_layout*)base;
            for (i = 0; i < layout->filehandles.count; i++)
                layout->filehandles.arr[i].fh.superblock = file->fh.superblock;
        }
    }
out:
    return status;
}

enum nfsstat4 pnfs_rpc_layoutcommit(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN stateid4 *stateid,
    IN uint64_t offset,
    IN uint64_t length,
    IN OPTIONAL uint64_t *new_last_offset,
    IN OPTIONAL nfstime4 *new_time_modify,
    OUT nfs41_file_info *info)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    pnfs_layoutcommit_args lc_args;
    pnfs_layoutcommit_res lc_res;
    nfs41_getattr_args getattr_args;
    nfs41_getattr_res getattr_res;
    bitmap4 attr_request;

    nfs41_superblock_getattr_mask(file->fh.superblock, &attr_request);

    compound_init(&compound, argops, resops, "layoutcommit");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_LAYOUTCOMMIT, &lc_args, &lc_res);
    lc_args.offset = offset;
    lc_args.length = length;
    lc_args.stateid = stateid;
    lc_args.new_time = new_time_modify;
    lc_args.new_offset = new_last_offset;

    compound_add_op(&compound, OP_GETATTR, &getattr_args, &getattr_res);
    getattr_args.attr_request = &attr_request;
    getattr_res.obj_attributes.attr_vals_len = NFS4_OPAQUE_LIMIT;
    getattr_res.info = info;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    if (compound_error(status = compound.res.status))
        goto out;

    /* update the attribute cache */
    memcpy(&info->attrmask, &getattr_res.obj_attributes.attrmask,
        sizeof(bitmap4));
    nfs41_attr_cache_update(session_name_cache(session),
        file->fh.fileid, info);
out:
    return status;
}

enum nfsstat4 pnfs_rpc_layoutreturn(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN enum pnfs_layout_type type,
    IN enum pnfs_iomode iomode,
    IN uint64_t offset,
    IN uint64_t length,
    IN stateid4 *stateid,
    OUT pnfs_layoutreturn_res *layoutreturn_res)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[3];
    nfs_resop4 resops[3];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    pnfs_layoutreturn_args layoutreturn_args;

    compound_init(&compound, argops, resops, "layoutreturn");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = 0;

    compound_add_op(&compound, OP_LAYOUTRETURN, &layoutreturn_args, layoutreturn_res);
    layoutreturn_args.reclaim = 0;
    layoutreturn_args.type = type;
    layoutreturn_args.iomode = iomode;
    layoutreturn_args.return_type = PNFS_RETURN_FILE;
    layoutreturn_args.offset = offset;
    layoutreturn_args.length = length;
    layoutreturn_args.stateid = stateid;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

enum nfsstat4 pnfs_rpc_getdeviceinfo(
    IN nfs41_session *session,
    IN unsigned char *deviceid,
    OUT pnfs_file_device *device)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[2];
    nfs_resop4 resops[2];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    pnfs_getdeviceinfo_args getdeviceinfo_args;
    pnfs_getdeviceinfo_res getdeviceinfo_res;

    compound_init(&compound, argops, resops, "get_deviceinfo");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_GETDEVICEINFO,
        &getdeviceinfo_args, &getdeviceinfo_res);
    getdeviceinfo_args.deviceid = deviceid;
    getdeviceinfo_args.layout_type = PNFS_LAYOUTTYPE_FILE;
    getdeviceinfo_args.maxcount = NFS41_MAX_SERVER_CACHE; /* XXX */
    getdeviceinfo_args.notify_types.count = 0;
    getdeviceinfo_res.u.res_ok.device = device;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);
out:
    return status;
}

enum nfsstat4 nfs41_rpc_openattr(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN bool_t createdir,
    OUT nfs41_fh *fh_out)
{
    enum nfsstat4 status;
    nfs41_compound compound;
    nfs_argop4 argops[4];
    nfs_resop4 resops[4];
    nfs41_sequence_args sequence_args;
    nfs41_sequence_res sequence_res;
    nfs41_putfh_args putfh_args;
    nfs41_putfh_res putfh_res;
    nfs41_openattr_args openattr_args;
    nfs41_openattr_res openattr_res;
    nfs41_getfh_res getfh_res;

    compound_init(&compound, argops, resops, "openattr");

    compound_add_op(&compound, OP_SEQUENCE, &sequence_args, &sequence_res);
    nfs41_session_sequence(&sequence_args, session, 0);

    compound_add_op(&compound, OP_PUTFH, &putfh_args, &putfh_res);
    putfh_args.file = file;
    putfh_args.in_recovery = FALSE;

    compound_add_op(&compound, OP_OPENATTR, &openattr_args, &openattr_res);
    openattr_args.createdir = createdir;

    compound_add_op(&compound, OP_GETFH, NULL, &getfh_res);
    getfh_res.fh = fh_out;

    status = compound_encode_send_decode(session, &compound, TRUE);
    if (status)
        goto out;

    compound_error(status = compound.res.status);

    fh_out->superblock = file->fh.superblock;
out:
    return status;
}
