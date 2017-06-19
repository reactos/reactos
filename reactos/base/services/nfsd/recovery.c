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

#include <time.h>

#include "recovery.h"
#include "delegation.h"
#include "nfs41_callback.h"
#include "nfs41_compound.h"
#include "nfs41_ops.h"
#include "daemon_debug.h"


/* session/client recovery uses a lock and condition variable in nfs41_client
 * to prevent multiple threads from attempting to recover at the same time */
bool_t nfs41_recovery_start_or_wait(
    IN nfs41_client *client)
{
    bool_t status = TRUE;

    EnterCriticalSection(&client->recovery.lock);

    if (!client->recovery.in_recovery) {
        dprintf(1, "Entering recovery mode for client %llu\n", client->clnt_id);
        client->recovery.in_recovery = TRUE;
    } else {
        status = FALSE;
        dprintf(1, "Waiting for recovery of client %llu\n", client->clnt_id);
        while (client->recovery.in_recovery)
            SleepConditionVariableCS(&client->recovery.cond,
                &client->recovery.lock, INFINITE);
        dprintf(1, "Woke up after recovery of client %llu\n", client->clnt_id);
    }

    LeaveCriticalSection(&client->recovery.lock);
    return status;
}

void nfs41_recovery_finish(
    IN nfs41_client *client)
{
    EnterCriticalSection(&client->recovery.lock);
    dprintf(1, "Finished recovery for client %llu\n", client->clnt_id);
    client->recovery.in_recovery = FALSE;
    WakeAllConditionVariable(&client->recovery.cond);
    LeaveCriticalSection(&client->recovery.lock);
}


/* session/client/state recovery */
int nfs41_recover_session(
    IN nfs41_session *session,
    IN bool_t client_state_lost)
{
    enum nfsstat4 status = NFS4_OK;

restart_recovery:
    /* recover the session */
    status = nfs41_session_renew(session);

    if (status == NFS4ERR_STALE_CLIENTID) {
        /* recover the client */
        client_state_lost = TRUE;
        status = nfs41_client_renew(session->client);
        if (status == NFS4_OK)
            goto restart_recovery; /* resume session recovery */

        eprintf("nfs41_client_renew() failed with %d\n", status);
    } else if (status) {
        eprintf("nfs41_session_renew() failed with %d\n", status);
    } else if (client_state_lost) {
        /* recover the client's state */
        status = nfs41_recover_client_state(session, session->client);
        if (status == NFS4ERR_BADSESSION)
            goto restart_recovery;
    }
    return status;
}

void nfs41_recover_sequence_flags(
    IN nfs41_session *session,
    IN uint32_t flags)
{
    const uint32_t revoked = flags &
        (SEQ4_STATUS_EXPIRED_ALL_STATE_REVOKED
        | SEQ4_STATUS_EXPIRED_SOME_STATE_REVOKED
        | SEQ4_STATUS_ADMIN_STATE_REVOKED
        | SEQ4_STATUS_RECALLABLE_STATE_REVOKED);
    const uint32_t restarted = flags &
        SEQ4_STATUS_RESTART_RECLAIM_NEEDED;

    /* no state recovery needed */
    if (revoked == 0 && restarted == 0)
        return;

    if (!nfs41_recovery_start_or_wait(session->client))
        return;

    if (revoked) {
        /* free stateids and attempt to recover them */
        nfs41_client_state_revoked(session, session->client, revoked);

        /* if RESTART_RECLAIM_NEEDED is also set, just do RECLAIM_COMPLETE */
        if (restarted) nfs41_reclaim_complete(session);

    } else if (restarted) {
        /* do server reboot state recovery */
        uint32_t status = nfs41_recover_client_state(session, session->client);
        if (status == NFS4ERR_BADSESSION) {
            /* recover the session and finish state recovery */
            nfs41_recover_session(session, TRUE);
        }
    }

    nfs41_recovery_finish(session->client);
}


/* client state recovery for server reboot or lease expiration */
static int recover_open_grace(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN nfs41_path_fh *file,
    IN state_owner4 *owner,
    IN uint32_t access,
    IN uint32_t deny,
    OUT stateid4 *stateid,
    OUT open_delegation4 *delegation)
{
    /* reclaim the open stateid with CLAIM_PREVIOUS */
    open_claim4 claim;
    claim.claim = CLAIM_PREVIOUS;
    claim.u.prev.delegate_type = delegation->type;

    return nfs41_open(session, parent, file, owner, &claim, access, deny, 
        OPEN4_NOCREATE, 0, NULL, FALSE, stateid, delegation, NULL);
}

static int recover_open_no_grace(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN nfs41_path_fh *file,
    IN state_owner4 *owner,
    IN uint32_t access,
    IN uint32_t deny,
    OUT stateid4 *stateid,
    OUT open_delegation4 *delegation)
{
    open_claim4 claim;
    int status;

    if (delegation->type != OPEN_DELEGATE_NONE) {
        /* attempt out-of-grace recovery with CLAIM_DELEGATE_PREV */
        claim.claim = CLAIM_DELEGATE_PREV;
        claim.u.deleg_prev.filename = &file->name;

        status = nfs41_open(session, parent, file, owner,
            &claim, access, deny, OPEN4_NOCREATE, 0, NULL, FALSE,
            stateid, delegation, NULL);
        if (status == NFS4_OK || status == NFS4ERR_BADSESSION)
            goto out;

        /* server support for CLAIM_DELEGATE_PREV is optional;
         * fall back to CLAIM_NULL on errors */
    }

    /* attempt out-of-grace recovery with CLAIM_NULL */
    claim.claim = CLAIM_NULL;
    claim.u.null.filename = &file->name;

    /* ask nicely for the delegation we had */
    if (delegation->type == OPEN_DELEGATE_READ)
        access |= OPEN4_SHARE_ACCESS_WANT_READ_DELEG;
    else if (delegation->type == OPEN_DELEGATE_WRITE)
        access |= OPEN4_SHARE_ACCESS_WANT_WRITE_DELEG;

    status = nfs41_open(session, parent, file, owner,
        &claim, access, deny, OPEN4_NOCREATE, 0, NULL, FALSE,
        stateid, delegation, NULL);
out:
    return status;
}

static int recover_open(
    IN nfs41_session *session,
    IN nfs41_open_state *open,
    IN OUT bool_t *grace)
{
    open_delegation4 delegation = { 0 };
    stateid4 stateid = { 0 };
    int status = NFS4ERR_BADHANDLE;

    /* check for an associated delegation */
    AcquireSRWLockExclusive(&open->lock);
    if (open->delegation.state) {
        nfs41_delegation_state *deleg = open->delegation.state;
        if (deleg->revoked) {
            /* reclaim the delegation along with the open */
            AcquireSRWLockShared(&deleg->lock);
            delegation.type = deleg->state.type;
            ReleaseSRWLockShared(&deleg->lock);
        } else if (deleg->state.recalled) {
            /* we'll need an open stateid regardless */
        } else if (list_empty(&open->locks.list)) {
            /* if there are locks, we need an open stateid to
             * reclaim them; otherwise, the open can be delegated */
            open->do_close = FALSE;
            status = NFS4_OK;
        }
    }
    ReleaseSRWLockExclusive(&open->lock);

    if (status == NFS4_OK) /* use existing delegation */
        goto out;

    if (*grace) {
        status = recover_open_grace(session, &open->parent, &open->file,
            &open->owner, open->share_access, open->share_deny,
            &stateid, &delegation);
        if (status == NFS4ERR_NO_GRACE) {
            *grace = FALSE;
            /* send RECLAIM_COMPLETE before any out-of-grace recovery */
            nfs41_reclaim_complete(session);
        }
    }
    if (!*grace) {
        status = recover_open_no_grace(session, &open->parent, &open->file,
            &open->owner, open->share_access, open->share_deny,
            &stateid, &delegation);
    }

    if (status)
        goto out;

    AcquireSRWLockExclusive(&open->lock);
    /* update the open stateid */
    memcpy(&open->stateid, &stateid, sizeof(stateid4));
    open->do_close = TRUE;

    if (open->delegation.state) {
        nfs41_delegation_state *deleg = open->delegation.state;
        if (deleg->revoked) {
            /* update delegation state */
            AcquireSRWLockExclusive(&deleg->lock);
            if (delegation.type != OPEN_DELEGATE_READ &&
                delegation.type != OPEN_DELEGATE_WRITE) {
                eprintf("recover_open() got delegation type %u, "
                    "expected %u\n", delegation.type, deleg->state.type);
            } else {
                memcpy(&deleg->state, &delegation, sizeof(open_delegation4));
                deleg->revoked = FALSE;
            }
            ReleaseSRWLockExclusive(&deleg->lock);
        }
    } else /* granted a new delegation? */
        nfs41_delegation_granted(session, &open->parent, &open->file,
            &delegation, FALSE, &open->delegation.state);
    ReleaseSRWLockExclusive(&open->lock);
out:
    return status;
}

static int recover_locks(
    IN nfs41_session *session,
    IN nfs41_open_state *open,
    IN OUT bool_t *grace)
{
    stateid_arg stateid;
    struct list_entry *entry;
    nfs41_lock_state *lock;
    int status = NFS4_OK;

    AcquireSRWLockExclusive(&open->lock);

    /* initialize the open stateid for the first lock request */
    memcpy(&stateid.stateid, &open->stateid, sizeof(stateid4));
    stateid.type = STATEID_OPEN;
    stateid.open = open;
    stateid.delegation = NULL;

    /* recover any locks for this open */
    list_for_each(entry, &open->locks.list) {
        lock = list_container(entry, nfs41_lock_state, open_entry);
        if (lock->delegated)
            continue;

        if (*grace) {
            status = nfs41_lock(session, &open->file, &open->owner, 
                lock->exclusive ? WRITE_LT : READ_LT, lock->offset, 
                lock->length, TRUE, FALSE, &stateid);
            if (status == NFS4ERR_NO_GRACE) {
                *grace = FALSE;
                /* send RECLAIM_COMPLETE before any out-of-grace recovery */
                nfs41_reclaim_complete(session);
            }
        }
        if (!*grace) {
            /* attempt out-of-grace recovery with a normal LOCK */
            status = nfs41_lock(session, &open->file, &open->owner, 
                lock->exclusive ? WRITE_LT : READ_LT, lock->offset, 
                lock->length, FALSE, FALSE, &stateid);
        }
        if (status == NFS4ERR_BADSESSION)
            break;
    }

    if (status != NFS4ERR_BADSESSION) {
        /* if we got a lock stateid back, save the lock with the open */
        if (stateid.type == STATEID_LOCK)
            memcpy(&open->locks.stateid, &stateid.stateid, sizeof(stateid4));
        else
            open->locks.stateid.seqid = 0;
    }

    ReleaseSRWLockExclusive(&open->lock);
    return status;
}

/* delegation recovery via WANT_DELEGATION */
static int recover_delegation_want(
    IN nfs41_session *session,
    IN nfs41_delegation_state *deleg,
    IN OUT bool_t *grace)
{
    deleg_claim4 claim;
    open_delegation4 delegation = { 0 };
    uint32_t want_flags = 0;
    int status = NFS4_OK;

    AcquireSRWLockShared(&deleg->lock);
    delegation.type = deleg->state.type;
    ReleaseSRWLockShared(&deleg->lock);

    if (delegation.type == OPEN_DELEGATE_READ)
        want_flags |= OPEN4_SHARE_ACCESS_WANT_READ_DELEG;
    else
        want_flags |= OPEN4_SHARE_ACCESS_WANT_WRITE_DELEG;

    if (*grace) {
        /* recover the delegation with WANT_DELEGATION/CLAIM_PREVIOUS */
        claim.claim = CLAIM_PREVIOUS;
        claim.prev_delegate_type = delegation.type;

        status = nfs41_want_delegation(session, &deleg->file, &claim, 
            want_flags, FALSE, &delegation);
        if (status == NFS4ERR_NO_GRACE) {
            *grace = FALSE;
            /* send RECLAIM_COMPLETE before any out-of-grace recovery */
            nfs41_reclaim_complete(session);
        }
    }
    if (!*grace) {
        /* attempt out-of-grace recovery with with CLAIM_DELEG_PREV_FH */
        claim.claim = CLAIM_DELEG_PREV_FH;

        status = nfs41_want_delegation(session, &deleg->file, &claim, 
            want_flags, FALSE, &delegation);
    }
    if (status)
        goto out;
    
    /* update delegation state */
    AcquireSRWLockExclusive(&deleg->lock);
    if (delegation.type != OPEN_DELEGATE_READ &&
            delegation.type != OPEN_DELEGATE_WRITE) {
        eprintf("recover_delegation_want() got delegation type %u, "
            "expected %u\n", delegation.type, deleg->state.type);
    } else {
        memcpy(&deleg->state, &delegation, sizeof(open_delegation4));
        deleg->revoked = FALSE;
    }
    ReleaseSRWLockExclusive(&deleg->lock);
out:
    return status;
}

/* delegation recovery via OPEN (requires corresponding CLOSE) */
static int recover_delegation_open(
    IN nfs41_session *session,
    IN nfs41_delegation_state *deleg,
    IN OUT bool_t *grace)
{
    state_owner4 owner;
    open_delegation4 delegation = { 0 };
    stateid_arg stateid;
    uint32_t access = OPEN4_SHARE_ACCESS_READ;
    uint32_t deny = OPEN4_SHARE_DENY_NONE;
    int status = NFS4_OK;

    /* choose the desired access mode based on delegation type */
    AcquireSRWLockShared(&deleg->lock);
    delegation.type = deleg->state.type;
    if (delegation.type == OPEN_DELEGATE_WRITE)
        access |= OPEN4_SHARE_ACCESS_WRITE | OPEN4_SHARE_ACCESS_WANT_WRITE_DELEG;
    else
        access |= OPEN4_SHARE_ACCESS_WANT_READ_DELEG;
    ReleaseSRWLockShared(&deleg->lock);

    /* construct a temporary open owner by concatenating the time
     * in seconds with the delegation pointer */
    time((time_t*)owner.owner);
    memcpy(owner.owner + sizeof(time_t), deleg, sizeof(deleg));
    owner.owner_len = sizeof(time_t) + sizeof(deleg);

    if (*grace) {
        status = recover_open_grace(session, &deleg->parent, &deleg->file,
            &owner, access, deny, &stateid.stateid, &delegation);
        if (status == NFS4ERR_NO_GRACE) {
            *grace = FALSE;
            /* send RECLAIM_COMPLETE before any out-of-grace recovery */
            nfs41_reclaim_complete(session);
        }
    }
    if (!*grace) {
        status = recover_open_no_grace(session, &deleg->parent, &deleg->file,
            &owner, access, deny, &stateid.stateid, &delegation);
    }
    if (status)
        goto out;

    /* update delegation state */
    AcquireSRWLockExclusive(&deleg->lock);
    if (delegation.type != OPEN_DELEGATE_READ &&
            delegation.type != OPEN_DELEGATE_WRITE) {
        eprintf("recover_delegation_open() got delegation type %u, "
            "expected %u\n", delegation.type, deleg->state.type);
    } else {
        memcpy(&deleg->state, &delegation, sizeof(open_delegation4));
        deleg->revoked = FALSE;
    }
    ReleaseSRWLockExclusive(&deleg->lock);

    /* send CLOSE to free the open stateid */
    stateid.open = NULL;
    stateid.delegation = NULL;
    stateid.type = STATEID_OPEN;
    nfs41_close(session, &deleg->file, &stateid);
out:
    return status;
}

static int recover_delegation(
    IN nfs41_session *session,
    IN nfs41_delegation_state *deleg,
    IN OUT bool_t *grace,
    IN OUT bool_t *want_supported)
{
    int status;

    /* 10.2.1. Delegation Recovery
     * When a client needs to reclaim a delegation and there is no
     * associated open, the client may use the CLAIM_PREVIOUS variant
     * of the WANT_DELEGATION operation.  However, since the server is
     * not required to support this operation, an alternative is to
     * reclaim via a dummy OPEN together with the delegation using an
     * OPEN of type CLAIM_PREVIOUS. */
    if (*want_supported)
        status = recover_delegation_want(session, deleg, grace);
    else
        status = NFS4ERR_NOTSUPP;

    if (status == NFS4ERR_NOTSUPP) {
        *want_supported = FALSE;
        status = recover_delegation_open(session, deleg, grace);
    }
    return status;
}

int nfs41_recover_client_state(
    IN nfs41_session *session,
    IN nfs41_client *client)
{
    const struct cb_layoutrecall_args recall = { PNFS_LAYOUTTYPE_FILE,
        PNFS_IOMODE_ANY, TRUE, { PNFS_RETURN_ALL } };
    struct client_state *state = &session->client->state;
    struct list_entry *entry;
    nfs41_open_state *open;
    nfs41_delegation_state *deleg;
    bool_t grace = TRUE;
    bool_t want_supported = TRUE;
    int status = NFS4_OK;

    EnterCriticalSection(&state->lock);

    /* flag all delegations as revoked until successful recovery;
     * recover_open() and recover_delegation_open() will only ask
     * for delegations when revoked = TRUE */
    list_for_each(entry, &state->delegations) {
        deleg = list_container(entry, nfs41_delegation_state, client_entry);
        deleg->revoked = TRUE;
    }

    /* recover each of the client's opens and associated delegations */
    list_for_each(entry, &state->opens) {
        open = list_container(entry, nfs41_open_state, client_entry);
        status = recover_open(session, open, &grace);
        if (status == NFS4_OK)
            status = recover_locks(session, open, &grace);
        if (status == NFS4ERR_BADSESSION)
            goto unlock;
    }

    /* recover delegations that weren't associated with any opens */
    list_for_each(entry, &state->delegations) {
        deleg = list_container(entry, nfs41_delegation_state, client_entry);
        if (deleg->revoked) {
            status = recover_delegation(session,
                deleg, &grace, &want_supported);
            if (status == NFS4ERR_BADSESSION)
                goto unlock;
        }
    }

    /* return any delegations that were reclaimed as 'recalled' */
    status = nfs41_client_delegation_recovery(client);
unlock:
    LeaveCriticalSection(&state->lock);

    /* revoke all of the client's layouts */
    pnfs_file_layout_recall(client, &recall);

    if (grace && status != NFS4ERR_BADSESSION) {
        /* send reclaim_complete, but don't fail on errors */
        nfs41_reclaim_complete(session);
    }
    return status;
}

static uint32_t stateid_array(
    IN struct list_entry *delegations,
    IN struct list_entry *opens,
    OUT stateid_arg **stateids_out,
    OUT uint32_t **statuses_out)
{
    struct list_entry *entry;
    nfs41_open_state *open;
    nfs41_delegation_state *deleg;
    stateid_arg *stateids = NULL;
    uint32_t *statuses = NULL;
    uint32_t i = 0, count = 0;

    /* count how many stateids the client needs to test */
    list_for_each(entry, delegations)
        count++;
    list_for_each(entry, opens)
        count += 3; /* open and potentially lock and layout */

    if (count == 0)
        goto out;

    /* allocate the stateid and status arrays */
    stateids = calloc(count, sizeof(stateid_arg));
    if (stateids == NULL)
        goto out_err;
    statuses = calloc(count, sizeof(uint32_t));
    if (statuses == NULL)
        goto out_err;
    memset(statuses, NFS4ERR_BAD_STATEID, count * sizeof(uint32_t));

    /* copy stateids into the array */
    list_for_each(entry, delegations) {
        deleg = list_container(entry, nfs41_delegation_state, client_entry);
        AcquireSRWLockShared(&deleg->lock);
        /* delegation stateid */
        memcpy(&stateids[i].stateid, &deleg->state.stateid, sizeof(stateid4));
        stateids[i].type = STATEID_DELEG_FILE;
        stateids[i].delegation = deleg;
        i++;
        ReleaseSRWLockShared(&deleg->lock);
    }

    list_for_each(entry, opens) {
        open = list_container(entry, nfs41_open_state, client_entry);

        AcquireSRWLockShared(&open->lock);
        /* open stateid */
        memcpy(&stateids[i].stateid, &open->stateid, sizeof(stateid4));
        stateids[i].type = STATEID_OPEN;
        stateids[i].open = open;
        i++;

        if (open->locks.stateid.seqid) { /* lock stateid? */
            memcpy(&stateids[i].stateid, &open->locks.stateid, sizeof(stateid4));
            stateids[i].type = STATEID_LOCK;
            stateids[i].open = open;
            i++;
        }

        if (open->layout) { /* layout stateid? */
            AcquireSRWLockShared(&open->layout->lock);
            if (open->layout->stateid.seqid) {
                memcpy(&stateids[i].stateid, &open->layout->stateid, sizeof(stateid4));
                stateids[i].type = STATEID_LAYOUT;
                stateids[i].open = open;
                i++;
            }
            ReleaseSRWLockShared(&open->layout->lock);
        }
        ReleaseSRWLockShared(&open->lock);
    }

    count = i;
    *stateids_out = stateids;
    *statuses_out = statuses;
out:
    return count;

out_err:
    free(stateids);
    free(statuses);
    count = 0;
    goto out;
}

void nfs41_client_state_revoked(
    IN nfs41_session *session,
    IN nfs41_client *client,
    IN uint32_t revoked)
{
    const struct cb_layoutrecall_args recall = { PNFS_LAYOUTTYPE_FILE,
        PNFS_IOMODE_ANY, TRUE, { PNFS_RETURN_ALL } };
    struct list_entry empty, *opens;
    struct client_state *clientstate = &session->client->state;
    stateid_arg *stateids = NULL;
    uint32_t *statuses = NULL;
    uint32_t i, count;
    bool_t grace = TRUE;
    bool_t want_supported = TRUE;

    EnterCriticalSection(&clientstate->lock);

    if (revoked == SEQ4_STATUS_RECALLABLE_STATE_REVOKED) {
        /* only delegations were revoked. use an empty list for opens */
        list_init(&empty);
        opens = &empty;
    } else {
        opens = &clientstate->opens;
    }

    /* get an array of the client's stateids */
    count = stateid_array(&clientstate->delegations,
        opens, &stateids, &statuses);
    if (count == 0)
        goto out;

    /* determine which stateids were revoked with TEST_STATEID */
    if ((revoked & SEQ4_STATUS_EXPIRED_ALL_STATE_REVOKED) == 0)
        nfs41_test_stateid(session, stateids, count, statuses);

    /* free all revoked stateids with FREE_STATEID */
    for (i = 0; i < count; i++)
        if (statuses[i])
            nfs41_free_stateid(session, &stateids[i].stateid);

    /* revoke all of the client's layouts */
    pnfs_file_layout_recall(client, &recall);

    /* recover the revoked stateids */
    for (i = 0; i < count; i++) {
        if (statuses[i]) {
            if (stateids[i].type == STATEID_DELEG_FILE)
                stateids[i].delegation->revoked = TRUE;
            else if (stateids[i].type == STATEID_OPEN)
                recover_open(session, stateids[i].open, &grace);
            else if (stateids[i].type == STATEID_LOCK)
                recover_locks(session, stateids[i].open, &grace);
        }
    }
    for (i = 0; i < count; i++) {
        /* delegations that weren't recovered by recover_open() */
        if (statuses[i] && stateids[i].type == STATEID_DELEG_FILE
            && stateids[i].delegation->revoked)
            recover_delegation(session, stateids[i].delegation,
                &grace, &want_supported);
    }

    nfs41_client_delegation_recovery(client);
out:
    LeaveCriticalSection(&clientstate->lock);
    free(stateids);
    free(statuses);
}


static bool_t recover_stateid_open(
    IN nfs_argop4 *argop,
    IN stateid_arg *stateid)
{
    bool_t retry = FALSE;

    if (stateid->open) {
        stateid4 *source = &stateid->open->stateid;

        /* if the source stateid is different, update and retry */
        AcquireSRWLockShared(&stateid->open->lock);
        if (memcmp(&stateid->stateid, source, sizeof(stateid4))) {
            memcpy(&stateid->stateid, source, sizeof(stateid4));
            retry = TRUE;
        }
        ReleaseSRWLockShared(&stateid->open->lock);
    }
    return retry;
}

static bool_t recover_stateid_lock(
    IN nfs_argop4 *argop,
    IN stateid_arg *stateid)
{
    bool_t retry = FALSE;

    if (stateid->open) {
        stateid4 *source = &stateid->open->locks.stateid;

        /* if the source stateid is different, update and retry */
        AcquireSRWLockShared(&stateid->open->lock);
        if (memcmp(&stateid->stateid, source, sizeof(stateid4))) {
            if (argop->op == OP_LOCK && source->seqid == 0) {
                /* resend LOCK with an open stateid */
                nfs41_lock_args *lock = (nfs41_lock_args*)argop->arg;
                lock->locker.new_lock_owner = 1;
                lock->locker.u.open_owner.open_stateid = stateid;
                lock->locker.u.open_owner.lock_owner = &stateid->open->owner;
                source = &stateid->open->stateid;
            }

            memcpy(&stateid->stateid, source, sizeof(stateid4));
            retry = TRUE;
        }
        ReleaseSRWLockShared(&stateid->open->lock);
    }
    return retry;
}

static bool_t recover_stateid_delegation(
    IN nfs_argop4 *argop,
    IN stateid_arg *stateid)
{
    bool_t retry = FALSE;

    if (stateid->open) {
        /* if the source stateid is different, update and retry */
        AcquireSRWLockShared(&stateid->open->lock);
        if (argop->op == OP_OPEN && stateid->open->do_close) {
            /* for nfs41_delegation_to_open(); if we've already reclaimed
             * an open stateid, just fail this OPEN with BAD_STATEID */
        } else if (stateid->open->delegation.state) {
            nfs41_delegation_state *deleg = stateid->open->delegation.state;
            stateid4 *source = &deleg->state.stateid;
            AcquireSRWLockShared(&deleg->lock);
            if (memcmp(&stateid->stateid, source, sizeof(stateid4))) {
                memcpy(&stateid->stateid, source, sizeof(stateid4));
                retry = TRUE;
            }
            ReleaseSRWLockShared(&deleg->lock);
        }
        ReleaseSRWLockShared(&stateid->open->lock);
    } else if (stateid->delegation) {
        nfs41_delegation_state *deleg = stateid->delegation;
        stateid4 *source = &deleg->state.stateid;
        AcquireSRWLockShared(&deleg->lock);
        if (memcmp(&stateid->stateid, source, sizeof(stateid4))) {
            memcpy(&stateid->stateid, source, sizeof(stateid4));
            retry = TRUE;
        }
        ReleaseSRWLockShared(&deleg->lock);
    }
    return retry;
}

bool_t nfs41_recover_stateid(
    IN nfs41_session *session,
    IN nfs_argop4 *argop)
{
    stateid_arg *stateid = NULL;

    /* get the stateid_arg from the operation's arguments */
    if (argop->op == OP_OPEN) {
        nfs41_op_open_args *open = (nfs41_op_open_args*)argop->arg;
        if (open->claim->claim == CLAIM_DELEGATE_CUR)
            stateid = open->claim->u.deleg_cur.delegate_stateid;
        else if (open->claim->claim == CLAIM_DELEG_CUR_FH)
            stateid = open->claim->u.deleg_cur_fh.delegate_stateid;
    } else if (argop->op == OP_CLOSE) {
        nfs41_op_close_args *close = (nfs41_op_close_args*)argop->arg;
        stateid = close->stateid;
    } else if (argop->op == OP_READ) {
        nfs41_read_args *read = (nfs41_read_args*)argop->arg;
        stateid = read->stateid;
    } else if (argop->op == OP_WRITE) {
        nfs41_write_args *write = (nfs41_write_args*)argop->arg;
        stateid = write->stateid;
    } else if (argop->op == OP_LOCK) {
        nfs41_lock_args *lock = (nfs41_lock_args*)argop->arg;
        if (lock->locker.new_lock_owner)
            stateid = lock->locker.u.open_owner.open_stateid;
        else
            stateid = lock->locker.u.lock_owner.lock_stateid;
    } else if (argop->op == OP_LOCKU) {
        nfs41_locku_args *locku = (nfs41_locku_args*)argop->arg;
        stateid = locku->lock_stateid;
    } else if (argop->op == OP_SETATTR) {
        nfs41_setattr_args *setattr = (nfs41_setattr_args*)argop->arg;
        stateid = setattr->stateid;
    } else if (argop->op == OP_LAYOUTGET) {
        pnfs_layoutget_args *lget = (pnfs_layoutget_args*)argop->arg;
        stateid = lget->stateid;
    } else if (argop->op == OP_DELEGRETURN) {
        nfs41_delegreturn_args *dr = (nfs41_delegreturn_args*)argop->arg;
        stateid = dr->stateid;
    }
    if (stateid == NULL)
        return FALSE;

    /* if there's recovery in progress, wait for it to finish */
    EnterCriticalSection(&session->client->recovery.lock);
    while (session->client->recovery.in_recovery)
        SleepConditionVariableCS(&session->client->recovery.cond,
            &session->client->recovery.lock, INFINITE);
    LeaveCriticalSection(&session->client->recovery.lock);

    switch (stateid->type) {
    case STATEID_OPEN:
        return recover_stateid_open(argop, stateid);

    case STATEID_LOCK:
        return recover_stateid_lock(argop, stateid);

    case STATEID_DELEG_FILE:
        return recover_stateid_delegation(argop, stateid);

    default:
        eprintf("%s can't recover stateid type %u\n",
            nfs_opnum_to_string(argop->op), stateid->type);
        break;
    }
    return FALSE;
}
