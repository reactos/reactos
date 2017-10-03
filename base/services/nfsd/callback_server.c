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
#include "delegation.h"
#include "nfs41_callback.h"
#include "daemon_debug.h"


#define CBSLVL 2 /* dprintf level for callback server logging */


static const char g_server_tag[] = "ms-nfs41-callback";


/* callback session */
static void replay_cache_write(
    IN nfs41_cb_session *session,
    IN struct cb_compound_args *args,
    IN struct cb_compound_res *res,
    IN bool_t cachethis);

void nfs41_callback_session_init(
    IN nfs41_session *session)
{
    /* initialize the replay cache with status NFS4ERR_SEQ_MISORDERED */
    struct cb_compound_res res = { 0 };
    StringCchCopyA(res.tag.str, CB_COMPOUND_MAX_TAG, g_server_tag);
    res.tag.len = sizeof(g_server_tag);
    res.status = NFS4ERR_SEQ_MISORDERED;

    session->cb_session.cb_sessionid = session->session_id;

    replay_cache_write(&session->cb_session, NULL, &res, FALSE);
}


/* OP_CB_LAYOUTRECALL */
static enum_t handle_cb_layoutrecall(
    IN nfs41_rpc_clnt *rpc_clnt,
    IN struct cb_layoutrecall_args *args,
    OUT struct cb_layoutrecall_res *res)
{
    enum pnfs_status status;

    status = pnfs_file_layout_recall(rpc_clnt->client, args);
    switch (status) {
    case PNFS_PENDING:
        /* not enough information to process the recall yet */
        res->status = NFS4ERR_DELAY;
        break;
    default:
        /* forgetful model for layout recalls */
        res->status = NFS4ERR_NOMATCHING_LAYOUT;
        break;
    }

    dprintf(CBSLVL, "  OP_CB_LAYOUTRECALL { %s, %s, recall %u } %s\n",
        pnfs_layout_type_string(args->type),
        pnfs_iomode_string(args->iomode), args->recall.type,
        nfs_error_string(res->status));
    return res->status;
}

/* OP_CB_RECALL_SLOT */
static enum_t handle_cb_recall_slot(
    IN nfs41_rpc_clnt *rpc_clnt,
    IN struct cb_recall_slot_args *args,
    OUT struct cb_recall_slot_res *res)
{
    res->status = nfs41_session_recall_slot(rpc_clnt->client->session,
        args->target_highest_slotid);

    dprintf(CBSLVL, "  OP_CB_RECALL_SLOT { %u } %s\n",
        args->target_highest_slotid, nfs_error_string(res->status));
    return res->status;
}

/* OP_CB_SEQUENCE */
static enum_t handle_cb_sequence(
    IN nfs41_rpc_clnt *rpc_clnt,
    IN struct cb_sequence_args *args,
    OUT struct cb_sequence_res *res,
    OUT nfs41_cb_session **session_out,
    OUT bool_t *cachethis)
{
    nfs41_cb_session *cb_session = &rpc_clnt->client->session->cb_session;
    uint32_t status = NFS4_OK;
    res->status = NFS4_OK;

    *session_out = cb_session;

    /* validate the sessionid */
    if (memcmp(cb_session->cb_sessionid, args->sessionid,
            NFS4_SESSIONID_SIZE)) {
        eprintf("[cb] received sessionid doesn't match session\n");
        res->status = NFS4ERR_BADSESSION;
        goto out;
    }

    /* we only support 1 slot for the back channel so slotid MUST be 0 */
    if (args->slotid != 0) {
        eprintf("[cb] received unexpected slotid=%d\n", args->slotid);
        res->status = NFS4ERR_BADSLOT;
        goto out;
    }
    if (args->highest_slotid != 0) {
        eprintf("[cb] received unexpected highest_slotid=%d\n", 
            args->highest_slotid);
        res->status = NFS4ERR_BAD_HIGH_SLOT;
        goto out;
    }

    /* check for a retry with the same seqid */
    if (args->sequenceid == cb_session->cb_seqnum) {
        if (!cb_session->replay.res.length) {
            /* return success for sequence, but fail the next operation */
            res->status = NFS4_OK;
            status = NFS4ERR_RETRY_UNCACHED_REP;
        } else {
            /* return NFS4ERR_SEQ_FALSE_RETRY for all replays; if the retry
             * turns out to be valid, this response will be replaced anyway */
            status = res->status = NFS4ERR_SEQ_FALSE_RETRY;
        }
        goto out;
    }

    /* error on any unexpected seqids */
    if (args->sequenceid != cb_session->cb_seqnum+1) {
        eprintf("[cb] bad received seq#=%d, expected=%d\n", 
            args->sequenceid, cb_session->cb_seqnum+1);
        res->status = NFS4ERR_SEQ_MISORDERED;
        goto out;
    }

    cb_session->cb_seqnum = args->sequenceid;
    *cachethis = args->cachethis;

    memcpy(res->ok.sessionid, args->sessionid, NFS4_SESSIONID_SIZE);
    res->ok.sequenceid = args->sequenceid;
    res->ok.slotid = args->slotid;
    res->ok.highest_slotid = args->highest_slotid;
    res->ok.target_highest_slotid = args->highest_slotid;

out:
    dprintf(CBSLVL, "  OP_CB_SEQUENCE { seqid %u, slot %u, cachethis %d } "
        "%s\n", args->sequenceid, args->slotid, args->cachethis, 
        nfs_error_string(res->status));
    return status;
}

/* OP_CB_GETATTR */
static enum_t handle_cb_getattr(
    IN nfs41_rpc_clnt *rpc_clnt,
    IN struct cb_getattr_args *args,
    OUT struct cb_getattr_res *res)
{
    /* look up cached attributes for the given filehandle */
    res->status = nfs41_delegation_getattr(rpc_clnt->client,
        &args->fh, &args->attr_request, &res->info);
    return res->status;
}

/* OP_CB_RECALL */
static enum_t handle_cb_recall(
    IN nfs41_rpc_clnt *rpc_clnt,
    IN struct cb_recall_args *args,
    OUT struct cb_recall_res *res)
{
    /* return the delegation asynchronously */
    res->status = nfs41_delegation_recall(rpc_clnt->client,
        &args->fh, &args->stateid, args->truncate);
    return res->status;
}

/* OP_CB_NOTIFY_DEVICEID */
static enum_t handle_cb_notify_deviceid(
    IN nfs41_rpc_clnt *rpc_clnt,
    IN struct cb_notify_deviceid_args *args,
    OUT struct cb_notify_deviceid_res *res)
{
    uint32_t i;
    for (i = 0; i < args->change_count; i++) {
        pnfs_file_device_notify(rpc_clnt->client->devices,
            &args->change_list[i]);
    }
    res->status = NFS4_OK;
    return res->status;
}

static void replay_cache_write(
    IN nfs41_cb_session *session,
    IN OPTIONAL struct cb_compound_args *args,
    IN struct cb_compound_res *res,
    IN bool_t cachethis)
{
    XDR xdr;
    uint32_t i;

    session->replay.arg.length = 0;
    session->replay.res.length = 0;

    /* encode the reply directly into the replay cache */
    xdrmem_create(&xdr, (char*)session->replay.res.buffer,
        NFS41_MAX_SERVER_CACHE, XDR_ENCODE);

    /* always try to cache the result */
    if (proc_cb_compound_res(&xdr, res)) {
        session->replay.res.length = XDR_GETPOS(&xdr);

        if (args) {
            /* encode the arguments into the request cache */
            xdrmem_create(&xdr, (char*)session->replay.arg.buffer,
                NFS41_MAX_SERVER_CACHE, XDR_ENCODE);

            if (proc_cb_compound_args(&xdr, args))
                session->replay.arg.length = XDR_GETPOS(&xdr);
        }
    } else if (cachethis) {
        /* on failure, only return errors if caching was requested */
        res->status = NFS4ERR_REP_TOO_BIG_TO_CACHE;

        /* find the first operation that failed to encode */
        for (i = 0; i < res->resarray_count; i++) {
            if (!res->resarray[i].xdr_ok) {
                res->resarray[i].res.status = NFS4ERR_REP_TOO_BIG_TO_CACHE;
                res->resarray_count = i + 1;
                break;
            }
        }
    }
}

static bool_t replay_validate_args(
    IN struct cb_compound_args *args,
    IN const struct replay_cache *cache)
{
    char buffer[NFS41_MAX_SERVER_CACHE];
    XDR xdr;

    /* encode the current arguments into a temporary buffer */
    xdrmem_create(&xdr, buffer, NFS41_MAX_SERVER_CACHE, XDR_ENCODE);

    if (!proc_cb_compound_args(&xdr, args))
        return FALSE;

    /* must match the cached length */
    if (XDR_GETPOS(&xdr) != cache->length)
        return FALSE;

    /* must match the cached buffer contents */
    return memcmp(cache->buffer, buffer, cache->length) == 0;
}

static bool_t replay_validate_ops(
    IN const struct cb_compound_args *args,
    IN const struct cb_compound_res *res)
{
    uint32_t i;
    for (i = 0; i < res->resarray_count; i++) {
        /* can't have more operations than the request */
        if (i >= args->argarray_count)
            return FALSE;

        /* each opnum must match the request */
        if (args->argarray[i].opnum != res->resarray[i].opnum)
            return FALSE;

        if (res->resarray[i].res.status)
            break;
    }
    return TRUE;
}

static int replay_cache_read(
    IN nfs41_cb_session *session,
    IN struct cb_compound_args *args,
    OUT struct cb_compound_res **res_out)
{
    XDR xdr;
    struct cb_compound_res *replay;
    struct cb_compound_res *res = *res_out;
    uint32_t status = NFS4_OK;

    replay = calloc(1, sizeof(struct cb_compound_res));
    if (replay == NULL) {
        eprintf("[cb] failed to allocate replay buffer\n");
        status = NFS4ERR_SERVERFAULT;
        goto out;
    }

    /* decode the response from the replay cache */
    xdrmem_create(&xdr, (char*)session->replay.res.buffer,
        NFS41_MAX_SERVER_CACHE, XDR_DECODE);
    if (!proc_cb_compound_res(&xdr, replay)) {
        eprintf("[cb] failed to decode replay buffer\n");
        status = NFS4ERR_SEQ_FALSE_RETRY;
        goto out_free_replay;
    }

    /* if we cached the arguments, use them to validate the retry */
    if (session->replay.arg.length) {
        if (!replay_validate_args(args, &session->replay.arg)) {
            eprintf("[cb] retry attempt with different arguments\n");
            status = NFS4ERR_SEQ_FALSE_RETRY;
            goto out_free_replay;
        }
    } else { /* otherwise, comparing opnums is the best we can do */
        if (!replay_validate_ops(args, replay)) {
            eprintf("[cb] retry attempt with different operations\n");
            status = NFS4ERR_SEQ_FALSE_RETRY;
            goto out_free_replay;
        }
    }

    /* free previous response and replace it with the replay */
    xdr.x_op = XDR_FREE;
    proc_cb_compound_res(&xdr, res);

    dprintf(2, "[cb] retry: returning cached response\n");

    *res_out = replay;
out:
    return status;

out_free_replay:
    xdr.x_op = XDR_FREE;
    proc_cb_compound_res(&xdr, replay);
    goto out;
}

/* CB_COMPOUND */
static void handle_cb_compound(nfs41_rpc_clnt *rpc_clnt, cb_req *req, struct cb_compound_res **reply)
{
    struct cb_compound_args args = { 0 };
    struct cb_compound_res *res = NULL;
    struct cb_argop *argop;
    struct cb_resop *resop;
    XDR *xdr = (XDR*)req->xdr;
    nfs41_cb_session *session = NULL;
    bool_t cachethis = FALSE;
    uint32_t i, status = NFS4_OK;

    dprintf(CBSLVL, "--> handle_cb_compound()\n");

    /* decode the arguments */
    if (!proc_cb_compound_args(xdr, &args)) {
        status = NFS4ERR_BADXDR;
        eprintf("failed to decode compound arguments\n");
    }

    /* allocate the compound results */
    res = calloc(1, sizeof(struct cb_compound_res));
    if (res == NULL) {
        status = NFS4ERR_SERVERFAULT;
        goto out;
    }
    res->status = status;
    StringCchCopyA(res->tag.str, CB_COMPOUND_MAX_TAG, g_server_tag);
    res->tag.str[CB_COMPOUND_MAX_TAG-1] = 0;
    res->tag.len = (uint32_t)strlen(res->tag.str);
    res->resarray = calloc(args.argarray_count, sizeof(struct cb_resop));
    if (res->resarray == NULL) {
        res->status = NFS4ERR_SERVERFAULT;
        goto out;
    }

    dprintf(CBSLVL, "CB_COMPOUND('%s', %u)\n", args.tag.str, args.argarray_count);
    if (args.minorversion != 1) {
        res->status = NFS4ERR_MINOR_VERS_MISMATCH; //XXXXX
        eprintf("args.minorversion %u != 1\n", args.minorversion);
        goto out;
    }

    /* handle each operation in the compound */
    for (i = 0; i < args.argarray_count && res->status == NFS4_OK; i++) {
        argop = &args.argarray[i];
        resop = &res->resarray[i];
        resop->opnum = argop->opnum;
        res->resarray_count++;

        /* 20.9.3: The error NFS4ERR_SEQUENCE_POS MUST be returned
         * when CB_SEQUENCE is found in any position in a CB_COMPOUND 
         * beyond the first.  If any other operation is in the first 
         * position of CB_COMPOUND, NFS4ERR_OP_NOT_IN_SESSION MUST 
         * be returned.
         */
        if (i == 0 && argop->opnum != OP_CB_SEQUENCE) {
            res->status = resop->res.status = NFS4ERR_OP_NOT_IN_SESSION;
            break;
        }
        if (i != 0 && argop->opnum == OP_CB_SEQUENCE) {
            res->status = resop->res.status = NFS4ERR_SEQUENCE_POS;
            break;
        }
        if (status == NFS4ERR_RETRY_UNCACHED_REP) {
            res->status = resop->res.status = status;
            break;
        }

        switch (argop->opnum) {
        case OP_CB_LAYOUTRECALL:
            dprintf(1, "OP_CB_LAYOUTRECALL\n");
            res->status = handle_cb_layoutrecall(rpc_clnt,
                &argop->args.layoutrecall, &resop->res.layoutrecall);
            break;
        case OP_CB_RECALL_SLOT:
            dprintf(1, "OP_CB_RECALL_SLOT\n");
            res->status = handle_cb_recall_slot(rpc_clnt,
                &argop->args.recall_slot, &resop->res.recall_slot);
            break;
        case OP_CB_SEQUENCE:
            dprintf(1, "OP_CB_SEQUENCE\n");
            status = handle_cb_sequence(rpc_clnt, &argop->args.sequence,
                &resop->res.sequence, &session, &cachethis);

            if (status == NFS4ERR_SEQ_FALSE_RETRY) {
                /* replace the current results with the cached response */
                status = replay_cache_read(session, &args, &res);
                if (status) res->status = status;
                goto out;
            }

            if (status == NFS4_OK)
                res->status = resop->res.sequence.status;
            break;
        case OP_CB_GETATTR:
            dprintf(1, "OP_CB_GETATTR\n");
            res->status = handle_cb_getattr(rpc_clnt, 
                &argop->args.getattr, &resop->res.getattr);
            break;
        case OP_CB_RECALL:
            dprintf(1, "OP_CB_RECALL\n");
            res->status = handle_cb_recall(rpc_clnt,
                &argop->args.recall, &resop->res.recall);
            break;
        case OP_CB_NOTIFY:
            dprintf(1, "OP_CB_NOTIFY\n");
            res->status = NFS4ERR_NOTSUPP;
            break;
        case OP_CB_PUSH_DELEG:
            dprintf(1, "OP_CB_PUSH_DELEG\n");
            res->status = NFS4ERR_NOTSUPP;
            break;
        case OP_CB_RECALL_ANY:
            dprintf(1, "OP_CB_RECALL_ANY\n");
            res->status = NFS4ERR_NOTSUPP;
            break;
        case OP_CB_RECALLABLE_OBJ_AVAIL:
            dprintf(1, "OP_CB_RECALLABLE_OBJ_AVAIL\n");
            res->status = NFS4ERR_NOTSUPP;
            break;
        case OP_CB_WANTS_CANCELLED:
            dprintf(1, "OP_CB_WANTS_CANCELLED\n");
            res->status = NFS4ERR_NOTSUPP;
            break;
        case OP_CB_NOTIFY_LOCK:
            dprintf(1, "OP_CB_NOTIFY_LOCK\n");
            res->status = NFS4ERR_NOTSUPP;
            break;
        case OP_CB_NOTIFY_DEVICEID:
            dprintf(1, "OP_CB_NOTIFY_DEVICEID\n");
            res->status = NFS4_OK;
            break;
        case OP_CB_ILLEGAL:
            dprintf(1, "OP_CB_ILLEGAL\n");
            res->status = NFS4ERR_NOTSUPP;
            break;
        default:
            eprintf("operation %u not supported\n", argop->opnum);
            res->status = NFS4ERR_NOTSUPP;
            break;
        }
    }

    /* always attempt to cache the reply */
    if (session)
        replay_cache_write(session, &args, res, cachethis);
out:
    /* free the arguments */
    xdr->x_op = XDR_FREE;
    proc_cb_compound_args(xdr, &args);

    *reply = res;
    dprintf(CBSLVL, "<-- handle_cb_compound() returning %s (%u results)\n",
        nfs_error_string(res ? res->status : status),
        res ? res->resarray_count : 0);
}

#ifdef __REACTOS__
int nfs41_handle_callback(void *rpc_clnt, void *cb, void * dummy)
{
    struct cb_compound_res **reply = dummy;
#else
int nfs41_handle_callback(void *rpc_clnt, void *cb, struct cb_compound_res **reply)
{
#endif
    nfs41_rpc_clnt *rpc = (nfs41_rpc_clnt *)rpc_clnt;
    cb_req *request = (cb_req *)cb;
    uint32_t status = 0;

    dprintf(1, "nfs41_handle_callback: received call\n");
    if (request->rq_prog != NFS41_RPC_CBPROGRAM) {
        eprintf("invalid rpc program %u\n", request->rq_prog);
        status = 2;
        goto out;
    }

    switch (request->rq_proc) {
    case CB_NULL:
        dprintf(1, "CB_NULL\n");
        break;

    case CB_COMPOUND:
        dprintf(1, "CB_COMPOUND\n");
        handle_cb_compound(rpc, request, reply);
        break;

    default:
        dprintf(1, "invalid rpc procedure %u\n", request->rq_proc);
        status = 3;
        goto out;
    }
out:
    return status;
}
