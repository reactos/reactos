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

#include <stdio.h>
#include <stdlib.h>

#include "nfs41_compound.h"
#include "nfs41_xdr.h"
#include "nfs41_ops.h"
#include "recovery.h"
#include "name_cache.h"
#include "daemon_debug.h"
#include "rpc/rpc.h"
#include "rpc/auth_sspi.h"

int compound_error(int status)
{
    if (status != NFS4_OK)
        dprintf(1, "COMPOUND failed with status %d.\n", status);
    return status;
}

void compound_init(
    nfs41_compound *compound,
    nfs_argop4 *argops,
    nfs_resop4 *resops,
    const char *tag)
{
    /* initialize args */
    compound->args.tag_len = (uint32_t)strlen(tag);
    memcpy(compound->args.tag, tag, compound->args.tag_len);
    compound->args.minorversion = 1;
    compound->args.argarray_count = 0;
    compound->args.argarray = argops;

    /* initialize results */
    ZeroMemory(&compound->res, sizeof(nfs41_compound_res));
    compound->res.tag_len = NFS4_OPAQUE_LIMIT;
    compound->res.resarray_count = 0;
    compound->res.resarray = resops;
}

void compound_add_op(
    nfs41_compound *compound,
    uint32_t opnum,
    void *arg,
    void *res)
{
    const uint32_t i = compound->args.argarray_count++;
    const uint32_t j = compound->res.resarray_count++;
    compound->args.argarray[i].op = opnum;
    compound->args.argarray[i].arg = arg;
    compound->res.resarray[j].op = opnum;
    compound->res.resarray[j].res = res;
}

/* Due to the possibility of replays, we might get a response to a different
 * call than the one we're expecting.  If we don't have a way to check for
 * this, we'll likely crash trying to decode into the wrong structures.
 * This function copies the number of operations and all of the operation
 * numbers from the compound arguments into the response, so we can verify
 * them on decode and fail before doing any damage. */
static void set_expected_res(
    nfs41_compound *compound)
{
    uint32_t i;
    compound->res.resarray_count = compound->args.argarray_count;
    for (i = 0; i < compound->res.resarray_count; i++)
        compound->res.resarray[i].op = compound->args.argarray[i].op;
}


static int create_new_rpc_auth(nfs41_session *session, uint32_t op,
                               nfs41_secinfo_info *secinfo)
{
    AUTH *auth = NULL;
    int status = ERROR_NETWORK_UNREACHABLE, i;
    uint32_t sec_flavor;

    for (i = 0; i < MAX_SECINFOS; i++) { 
        if (!secinfo[i].sec_flavor && !secinfo[i].type)
            goto out;
        if (secinfo[i].sec_flavor == RPCSEC_GSS) {
            auth = authsspi_create_default(session->client->rpc->rpc, 
                        session->client->rpc->server_name, secinfo[i].type);
            if (auth == NULL) {
                eprintf("handle_wrongsecinfo_noname: authsspi_create_default for "
                        "gsstype %s failed\n", gssauth_string(secinfo[i].type));
                continue;
            }
            sec_flavor = secinfo[i].type;
        } else {
            char machname[MAXHOSTNAMELEN + 1];
            gid_t gids[1];
            if (gethostname(machname, sizeof(machname)) == -1) {
                eprintf("nfs41_rpc_clnt_create: gethostname failed\n");
                continue;
            }
            machname[sizeof(machname) - 1] = '\0';
            auth = authsys_create(machname, session->client->rpc->uid, 
                        session->client->rpc->gid, 0, gids);
            if (auth == NULL) {
                eprintf("handle_wrongsecinfo_noname: authsys_create failed\n");
                continue;
            }
            sec_flavor = AUTH_SYS;
        }
        AcquireSRWLockExclusive(&session->client->rpc->lock);
        session->client->rpc->sec_flavor = sec_flavor;
        session->client->rpc->rpc->cl_auth = auth;
        ReleaseSRWLockExclusive(&session->client->rpc->lock);
        status = 0;
        break;
    }
out:
    return status;
}

int compound_encode_send_decode(
    nfs41_session *session,
    nfs41_compound *compound,
    bool_t try_recovery)
{
    int status, retry_count = 0, delayby = 0, secinfo_status;
    nfs41_sequence_args *args = (nfs41_sequence_args *)
        compound->args.argarray[0].arg;
    uint32_t saved_sec_flavor;
    AUTH *saved_auth;
    int op1 = compound->args.argarray[0].op;

retry:
    /* send compound */
    retry_count++;
    set_expected_res(compound);
    status = nfs41_send_compound(session->client->rpc,
        (char *)&compound->args, (char *)&compound->res);
    // bump sequence number if sequence op succeeded.
    if (compound->res.resarray_count > 0 && 
            compound->res.resarray[0].op == OP_SEQUENCE) {
        nfs41_sequence_res *seq = 
            (nfs41_sequence_res *)compound->res.resarray[0].res;
        if (seq->sr_status == NFS4_OK) {
            // returned slotid must be the same we sent
            if (seq->sr_resok4.sr_slotid != args->sa_slotid) {
                eprintf("[session] sr_slotid=%d != sa_slotid=%d\n",
                    seq->sr_resok4.sr_slotid, args->sa_slotid);
                status = NFS4ERR_IO;
                goto out_free_slot;
            }
            // returned sessionid must be the same we sent
            if (memcmp(seq->sr_resok4.sr_sessionid, args->sa_sessionid, 
                    NFS4_SESSIONID_SIZE)) {
                eprintf("[session] sr_sessionid != sa_sessionid\n");
                print_hexbuf(1, (unsigned char *)"sr_sessionid", 
                    seq->sr_resok4.sr_sessionid, NFS4_SESSIONID_SIZE);
                print_hexbuf(1, (unsigned char *)"sa_sessionid", 
                    args->sa_sessionid, NFS4_SESSIONID_SIZE);
                status = NFS4ERR_IO;
                goto out_free_slot;
            }
            if (seq->sr_resok4.sr_status_flags) 
                print_sr_status_flags(1, seq->sr_resok4.sr_status_flags);

            nfs41_session_bump_seq(session, args->sa_slotid,
                seq->sr_resok4.sr_target_highest_slotid);

            /* check sequence status flags for state revocation */
            if (try_recovery && seq->sr_resok4.sr_status_flags)
                nfs41_recover_sequence_flags(session,
                    seq->sr_resok4.sr_status_flags);
        }
    }

    if (status) {
        eprintf("nfs41_send_compound failed %d for seqid=%d, slotid=%d\n", 
            status, args->sa_sequenceid, args->sa_slotid);
        status = NFS4ERR_IO;
        goto out_free_slot;
    }

    if (compound->res.status != NFS4_OK)
        dprintf(1, "\n################ %s ################\n\n",
            nfs_error_string(compound->res.status));

    switch (compound->res.status) {
    case NFS4_OK:
        break;

    case NFS4ERR_STALE_CLIENTID:
        if (!try_recovery)
            goto out;
        if (!nfs41_recovery_start_or_wait(session->client))
            goto do_retry;
        // try to create a new client
        status = nfs41_client_renew(session->client);

        nfs41_recovery_finish(session->client);
        if (status) {
            eprintf("nfs41_client_renew() failed with %d\n", status);
            status = ERROR_BAD_NET_RESP;
            goto out;
        }
        if (op1 == OP_CREATE_SESSION) {
            nfs41_create_session_args *csa = (nfs41_create_session_args*)
                compound->args.argarray[0].arg;
            AcquireSRWLockShared(&session->client->exid_lock);
            csa->csa_clientid = session->client->clnt_id;
            csa->csa_sequence = session->client->seq_id;
            AcquireSRWLockShared(&session->client->exid_lock);
        }
        goto do_retry;

    case NFS4ERR_BADSESSION:
        if (!try_recovery)
            goto out;
        if (!nfs41_recovery_start_or_wait(session->client))
            goto do_retry;
        // try to create a new session
        status = nfs41_recover_session(session, FALSE);

        nfs41_recovery_finish(session->client);
        if (status) {
            eprintf("nfs41_recover_session() failed with %d\n", status);
            status = ERROR_BAD_NET_RESP;
            goto out;
        }
        goto do_retry;

    case NFS4ERR_EXPIRED: /* revoked by lease expiration */
    case NFS4ERR_BAD_STATEID:
    case NFS4ERR_STALE_STATEID: /* server reboot */
        if (op1 == OP_SEQUENCE)
            nfs41_session_free_slot(session, args->sa_slotid);
        if (try_recovery && nfs41_recover_stateid(session,
                &compound->args.argarray[compound->res.resarray_count-1]))
            goto do_retry;
        goto out;

    case NFS4ERR_BADSLOT:
        /* free the slot and retry with a new one */
        if (op1 != OP_SEQUENCE || nfs41_session_bad_slot(session, args))
            goto out;
        goto retry;

    case NFS4ERR_GRACE:
    case NFS4ERR_DELAY:
#define RETRY_INDEFINITELY
#ifndef RETRY_INDEFINITELY
#define NUMBER_2_RETRY 19
#endif

#ifndef RETRY_INDEFINITELY
        if (retry_count < NUMBER_2_RETRY) {
#endif
            if (op1 == OP_SEQUENCE)
                nfs41_session_free_slot(session, args->sa_slotid);
            if (compound->res.status == NFS4ERR_GRACE)
                delayby = 5000;
            else
                delayby = 500*retry_count;
            dprintf(1, "Compound returned %s: sleeping for %ums..\n", 
                (compound->res.status==NFS4ERR_GRACE)?"NFS4ERR_GRACE":"NFS4ERR_DELAY",
                delayby);
            Sleep(delayby);
            dprintf(1, "Attempting to resend compound.\n");
            goto do_retry;
#ifndef RETRY_INDEFINITELY
        }
#endif
        break;

    case NFS4ERR_FHEXPIRED: /* TODO: recover expired volatile filehandles */
        status = NFS4ERR_STALE; /* for now, treat them as ERR_STALE */
        /* no break */
    case NFS4ERR_STALE:
        {
            nfs_argop4 *argarray = compound->args.argarray;
            struct nfs41_name_cache *name_cache =
                session_name_cache(session);
            nfs41_putfh_args *putfh;
            uint32_t i, start = 0;

            /* NFS4ERR_STALE generally comes from a PUTFH operation. in
             * this case, remove its filehandle from the name cache. but
             * because COMPOUNDs are not atomic, a file can be removed
             * between PUTFH and the operation that uses it. in this
             * case, we can't tell which PUTFH operation is to blame, so
             * we must invalidate filehandles of all PUTFH operations in
             * the COMPOUND */

            if (argarray[compound->res.resarray_count-1].op == OP_PUTFH)
                start = compound->res.resarray_count-1;

            for (i = start; i < compound->res.resarray_count; i++) {
                if (argarray[i].op == OP_PUTFH) {
                    putfh = (nfs41_putfh_args*)argarray[i].arg;

                    if (!putfh->in_recovery && putfh->file->path)
                        nfs41_name_cache_remove_stale(name_cache,
                            session, putfh->file->path);
                }
            }
        }
        break;
    case NFS4ERR_WRONGSEC:
        {
            nfs41_secinfo_info secinfo[MAX_SECINFOS] = { 0 };
            uint32_t rcount = compound->res.resarray_count;
            nfs_argop4 *argarray = compound->args.argarray;
            uint32_t op = argarray[rcount-1].op;
            nfs41_putfh_args *putfh;
            nfs41_path_fh *file = NULL;
            switch(op) {
            case OP_PUTFH:
            case OP_RESTOREFH:
            case OP_LINK:
            case OP_RENAME:
            case OP_PUTROOTFH:
            case OP_LOOKUP:
            case OP_OPEN:
            case OP_SECINFO_NO_NAME:
            case OP_SECINFO:
                if (op1 == OP_SEQUENCE)
                    nfs41_session_free_slot(session, args->sa_slotid);
                /* from: 2.6.3.1.1.5.  Put Filehandle Operation + SECINFO/SECINFO_NO_NAME
                 * The NFSv4.1 server MUST NOT return NFS4ERR_WRONGSEC to a put
                 * filehandle operation that is immediately followed by SECINFO or
                 * SECINFO_NO_NAME.  The NFSv4.1 server MUST NOT return NFS4ERR_WRONGSEC
                 * from SECINFO or SECINFO_NO_NAME.
                 */
                if (op1 == OP_SEQUENCE &&
                        (argarray[1].op == OP_PUTFH || 
                        argarray[1].op == OP_PUTROOTFH) &&
                        (argarray[2].op == OP_SECINFO_NO_NAME ||
                        argarray[2].op == OP_SECINFO)) {
                    dprintf(1, "SECINFO: BROKEN SERVER\n");
                    goto out;
                }
                if (!try_recovery)
                    goto out;
                if (!nfs41_recovery_start_or_wait(session->client))
                    goto do_retry;

                saved_sec_flavor = session->client->rpc->sec_flavor;
                saved_auth = session->client->rpc->rpc->cl_auth;
                if (op == OP_LOOKUP || op == OP_OPEN) {
                    const nfs41_component *name;
                    nfs41_path_fh tmp = { 0 };                   
                    nfs41_getfh_res *getfh;
                    nfs41_lookup_args *largs;
                    nfs41_op_open_args *oargs;
                    if (argarray[rcount-2].op == OP_PUTFH) {
                        putfh = (nfs41_putfh_args *)argarray[rcount-2].arg;
                        file = putfh->file;
                    } else if (argarray[rcount-2].op == OP_GETATTR &&
                               argarray[rcount-3].op == OP_GETFH) {
                        getfh = (nfs41_getfh_res *)compound->res.resarray[rcount-3].res;
                        memcpy(&tmp.fh, getfh->fh, sizeof(nfs41_fh));
                        file = &tmp;
                    }
                    else {
                        nfs41_recovery_finish(session->client);
                        goto out;
                    }

                    if (op == OP_LOOKUP) {
                        largs = (nfs41_lookup_args *)argarray[rcount-1].arg;
                        name = largs->name;
                    } else if (op == OP_OPEN) {
                        oargs = (nfs41_op_open_args *)argarray[rcount-1].arg;
                        name = oargs->claim->u.null.filename;
                    }
                    secinfo_status = nfs41_secinfo(session, file, name, secinfo);
                    if (secinfo_status) {
                        eprintf("nfs41_secinfo failed with %d\n", secinfo_status);
                        nfs41_recovery_finish(session->client);
                        if (secinfo_status == NFS4ERR_BADSESSION) {
                            if (op1 == OP_SEQUENCE)
                                nfs41_session_free_slot(session, args->sa_slotid);
                            goto do_retry;
                        }
                        goto out_free_slot;
                    }
                }
                else {                    
                    if (op == OP_PUTFH) {
                        putfh = (nfs41_putfh_args *)argarray[rcount-1].arg;
                        file = putfh->file;
                    } 
                    secinfo_status = nfs41_secinfo_noname(session, file, secinfo);
                    if (secinfo_status) {
                        eprintf("nfs41_secinfo_noname failed with %d\n", 
                            secinfo_status);
                        nfs41_recovery_finish(session->client);
                        if (op1 == OP_SEQUENCE)
                            nfs41_session_free_slot(session, args->sa_slotid);
                        goto out_free_slot;
                    }
                }
                secinfo_status = create_new_rpc_auth(session, op, secinfo);
                if (!secinfo_status) {
                    auth_destroy(saved_auth);
                    nfs41_recovery_finish(session->client);
                    // Need to retry only 
                    goto do_retry;
                } else {
                    AcquireSRWLockExclusive(&session->client->rpc->lock);
                    session->client->rpc->sec_flavor = saved_sec_flavor;
                    session->client->rpc->rpc->cl_auth = saved_auth;
                    ReleaseSRWLockExclusive(&session->client->rpc->lock);
                    nfs41_recovery_finish(session->client);
                }                
                break;
            }
        }
    }
    if (compound->res.resarray[0].op == OP_SEQUENCE) {
        nfs41_sequence_res *seq = 
            (nfs41_sequence_res *)compound->res.resarray[0].res;
        if (seq->sr_status == NFS4_OK && session->client->rpc->needcb &&
                (seq->sr_resok4.sr_status_flags & SEQ4_STATUS_CB_PATH_DOWN)) {
            nfs41_session_free_slot(session, args->sa_slotid);
            nfs41_bind_conn_to_session(session->client->rpc,
                session->session_id, CDFC4_BACK_OR_BOTH);
            goto out;
        }
    }
out_free_slot:
    if (op1 == OP_SEQUENCE)
        nfs41_session_free_slot(session, args->sa_slotid);
out:
    return status;

do_retry:
    if (compound->res.resarray[0].op == OP_SEQUENCE)
        nfs41_session_get_slot(session, &args->sa_slotid,
            &args->sa_sequenceid, &args->sa_highest_slotid);
    goto retry;
}
