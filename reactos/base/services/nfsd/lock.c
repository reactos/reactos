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

#include "daemon_debug.h"
#include "delegation.h"
#include "nfs41_ops.h"
#include "upcall.h"
#include "util.h"


#define LKLVL 2 /* dprintf level for lock logging */


static void lock_stateid_arg(
    IN nfs41_open_state *state,
    OUT stateid_arg *arg)
{
    arg->open = state;
    arg->delegation = NULL;

    AcquireSRWLockShared(&state->lock);
    if (state->locks.stateid.seqid) {
        memcpy(&arg->stateid, &state->locks.stateid, sizeof(stateid4));
        arg->type = STATEID_LOCK;
    } else if (state->do_close) {
        memcpy(&arg->stateid, &state->stateid, sizeof(stateid4));
        arg->type = STATEID_OPEN;
    } else {
        memset(&arg->stateid, 0, sizeof(stateid4));
        arg->type = STATEID_SPECIAL;
    }
    ReleaseSRWLockShared(&state->lock);
}

/* expects the caller to hold an exclusive lock on nfs41_open_state.lock */
static void lock_stateid_update(
    OUT nfs41_open_state *state,
    IN const stateid4 *stateid)
{
    if (state->locks.stateid.seqid == 0) {
        /* if it's a new lock stateid, copy it in */
        memcpy(&state->locks.stateid, stateid, sizeof(stateid4));
    } else if (stateid->seqid > state->locks.stateid.seqid) {
        /* update the seqid if it's more recent */
        state->locks.stateid.seqid = stateid->seqid;
    }
}

static void open_lock_add(
    IN nfs41_open_state *open,
    IN const stateid_arg *stateid,
    IN nfs41_lock_state *lock)
{
    AcquireSRWLockExclusive(&open->lock);

    if (stateid->type == STATEID_LOCK)
        lock_stateid_update(open, &stateid->stateid);

    lock->id = open->locks.counter++;
    list_add_tail(&open->locks.list, &lock->open_entry);

    ReleaseSRWLockExclusive(&open->lock);
}

static bool_t open_lock_delegate(
    IN nfs41_open_state *open,
    IN nfs41_lock_state *lock)
{
    bool_t delegated = FALSE;

    AcquireSRWLockExclusive(&open->lock);
    if (open->delegation.state) {
        nfs41_delegation_state *deleg = open->delegation.state;
        AcquireSRWLockShared(&deleg->lock);
        if (deleg->state.type == OPEN_DELEGATE_WRITE
            && deleg->status == DELEGATION_GRANTED) {
            lock->delegated = 1;
            lock->id = open->locks.counter++;
            list_add_tail(&open->locks.list, &lock->open_entry);
            delegated = TRUE;
        }
        ReleaseSRWLockShared(&deleg->lock);
    }
    ReleaseSRWLockExclusive(&open->lock);

    return delegated;
}

#define lock_entry(pos) list_container(pos, nfs41_lock_state, open_entry)

static int lock_range_cmp(const struct list_entry *entry, const void *value)
{
    const nfs41_lock_state *lhs = lock_entry(entry);
    const nfs41_lock_state *rhs = (const nfs41_lock_state*)value;
    if (lhs->offset != rhs->offset) return -1;
    if (lhs->length != rhs->length) return -1;
    return 0;
}

static int open_unlock_delegate(
    IN nfs41_open_state *open,
    IN const nfs41_lock_state *input)
{
    struct list_entry *entry;
    int status = ERROR_NOT_LOCKED;

    AcquireSRWLockExclusive(&open->lock);

    /* find lock state that matches this range */
    entry = list_search(&open->locks.list, input, lock_range_cmp);
    if (entry) {
        nfs41_lock_state *lock = lock_entry(entry);
        if (lock->delegated) {
            /* if the lock was delegated, remove/free it and return success */
            list_remove(entry);
            free(lock);
            status = NO_ERROR;
        } else
            status = ERROR_LOCKED;
    }

    ReleaseSRWLockExclusive(&open->lock);
    return status;
}

static void open_unlock_remove(
    IN nfs41_open_state *open,
    IN const stateid_arg *stateid,
    IN const nfs41_lock_state *input)
{
    struct list_entry *entry;

    AcquireSRWLockExclusive(&open->lock);
    if (stateid->type == STATEID_LOCK)
        lock_stateid_update(open, &stateid->stateid);

    /* find and remove the unlocked range */
    entry = list_search(&open->locks.list, input, lock_range_cmp);
    if (entry) {
        list_remove(entry);
        free(lock_entry(entry));
    }
    ReleaseSRWLockExclusive(&open->lock);
}


/* NFS41_LOCK */
static int parse_lock(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall)
{
    int status;
    lock_upcall_args *args = &upcall->args.lock;

    status = safe_read(&buffer, &length, &args->offset, sizeof(LONGLONG));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->length, sizeof(LONGLONG));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->exclusive, sizeof(BOOLEAN));
    if (status) goto out;
    status = safe_read(&buffer, &length, &args->blocking, sizeof(BOOLEAN));
    if (status) goto out;

    dprintf(1, "parsing NFS41_LOCK: offset=0x%llx length=0x%llx exclusive=%u "
            "blocking=%u\n", args->offset, args->length, args->exclusive, 
            args->blocking);
out:
    return status;
}

static __inline uint32_t get_lock_type(BOOLEAN exclusive, BOOLEAN blocking)
{
    return blocking == 0
        ? ( exclusive == 0 ? READ_LT : WRITE_LT )
        : ( exclusive == 0 ? READW_LT : WRITEW_LT );
}

static int handle_lock(nfs41_upcall *upcall)
{
    stateid_arg stateid;
    lock_upcall_args *args = &upcall->args.lock;
    nfs41_open_state *state = upcall->state_ref;
    nfs41_lock_state *lock;
    const uint32_t type = get_lock_type(args->exclusive, args->blocking);
    int status = NO_ERROR;

    /* 18.10.3. Operation 12: LOCK - Create Lock
     * "To lock the file from a specific offset through the end-of-file
     * (no matter how long the file actually is) use a length field equal
     * to NFS4_UINT64_MAX." */
    if (args->length >= NFS4_UINT64_MAX - args->offset)
        args->length = NFS4_UINT64_MAX;

    /* allocate the lock state */
    lock = calloc(1, sizeof(nfs41_lock_state));
    if (lock == NULL) {
        status = GetLastError();
        goto out;
    }
    lock->offset = args->offset;
    lock->length = args->length;
    lock->exclusive = args->exclusive;

    /* if we hold a write delegation, handle the lock locally */
    if (open_lock_delegate(state, lock)) {
        dprintf(LKLVL, "delegated lock { %llu, %llu }\n",
            lock->offset, lock->length);
        args->acquired = TRUE; /* for cancel_lock() */
        goto out;
    }

    /* open_to_lock_owner4 requires an open stateid; if we
     * have a delegation, convert it to an open stateid */
    status = nfs41_delegation_to_open(state, TRUE);
    if (status) {
        status = ERROR_FILE_INVALID;
        goto out_free;
    }

    EnterCriticalSection(&state->locks.lock);

    lock_stateid_arg(state, &stateid);

    status = nfs41_lock(state->session, &state->file, &state->owner,
        type, lock->offset, lock->length, FALSE, TRUE, &stateid);
    if (status) {
        dprintf(LKLVL, "nfs41_lock failed with %s\n",
            nfs_error_string(status));
        status = nfs_to_windows_error(status, ERROR_BAD_NET_RESP);
        LeaveCriticalSection(&state->locks.lock);
        goto out_free;
    }

    /* save lock state with the open */
    open_lock_add(state, &stateid, lock);
    LeaveCriticalSection(&state->locks.lock);

    args->acquired = TRUE; /* for cancel_lock() */
out:
    return status;

out_free:
    free(lock);
    goto out;
}

static void cancel_lock(IN nfs41_upcall *upcall)
{
    stateid_arg stateid;
    nfs41_lock_state input;
    lock_upcall_args *args = &upcall->args.lock;
    nfs41_open_state *state = upcall->state_ref;
    int status = NO_ERROR;

    dprintf(1, "--> cancel_lock()\n");

    /* can't do 'if (upcall->status)' here, because a handle_lock() success
     * could be overwritten by upcall_marshall() or allocation failure */
    if (!args->acquired)
        goto out;

    input.offset = args->offset;
    input.length = args->length;

    /* search for the range to unlock, and remove if delegated */
    status = open_unlock_delegate(state, &input);
    if (status != ERROR_LOCKED)
        goto out;

    EnterCriticalSection(&state->locks.lock);
    lock_stateid_arg(state, &stateid);

    status = nfs41_unlock(state->session, &state->file,
        args->offset, args->length, &stateid);

    open_unlock_remove(state, &stateid, &input);
    LeaveCriticalSection(&state->locks.lock);

    status = nfs_to_windows_error(status, ERROR_BAD_NET_RESP);
out:
    dprintf(1, "<-- cancel_lock() returning %d\n", status);
}


/* NFS41_UNLOCK */
static int parse_unlock(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall)
{
    int status;
    unlock_upcall_args *args = &upcall->args.unlock;

    status = safe_read(&buffer, &length, &args->count, sizeof(ULONG));
    if (status) goto out;

    args->buf = buffer;
    args->buf_len = length;

    dprintf(1, "parsing NFS41_UNLOCK: count=%u\n", args->count);
out:
    return status;
}

static int handle_unlock(nfs41_upcall *upcall)
{
    nfs41_lock_state input;
    stateid_arg stateid;
    unlock_upcall_args *args = &upcall->args.unlock;
    nfs41_open_state *state = upcall->state_ref;
    unsigned char *buf = args->buf;
    uint32_t buf_len = args->buf_len;
    uint32_t i;
    int status = NO_ERROR;

    for (i = 0; i < args->count; i++) {
        if (safe_read(&buf, &buf_len, &input.offset, sizeof(LONGLONG))) break;
        if (safe_read(&buf, &buf_len, &input.length, sizeof(LONGLONG))) break;

        /* do the same translation as LOCK, or the ranges won't match */
        if (input.length >= NFS4_UINT64_MAX - input.offset)
            input.length = NFS4_UINT64_MAX;

        /* search for the range to unlock, and remove if delegated */
        status = open_unlock_delegate(state, &input);
        if (status != ERROR_LOCKED)
            continue;

        EnterCriticalSection(&state->locks.lock);
        lock_stateid_arg(state, &stateid);

        status = nfs41_unlock(state->session, &state->file,
            input.offset, input.length, &stateid);

        open_unlock_remove(state, &stateid, &input);
        LeaveCriticalSection(&state->locks.lock);

        status = nfs_to_windows_error(status, ERROR_BAD_NET_RESP);
    }
    return status;
}


const nfs41_upcall_op nfs41_op_lock = {
    parse_lock,
    handle_lock,
    NULL,
    cancel_lock
};
const nfs41_upcall_op nfs41_op_unlock = {
    parse_unlock,
    handle_unlock
};
