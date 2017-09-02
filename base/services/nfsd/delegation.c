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

#include "delegation.h"
#include "nfs41_ops.h"
#include "name_cache.h"
#include "util.h"
#include "daemon_debug.h"

#include <devioctl.h>
#include "nfs41_driver.h" /* for making downcall to invalidate cache */
#include "util.h"

#define DGLVL 2 /* dprintf level for delegation logging */


/* allocation and reference counting */
static int delegation_create(
    IN const nfs41_path_fh *parent,
    IN const nfs41_path_fh *file,
    IN const open_delegation4 *delegation,
    OUT nfs41_delegation_state **deleg_out)
{
    nfs41_delegation_state *state;
    int status = NO_ERROR;

    state = calloc(1, sizeof(nfs41_delegation_state));
    if (state == NULL) {
        status = GetLastError();
        goto out;
    }

    memcpy(&state->state, delegation, sizeof(open_delegation4));

    abs_path_copy(&state->path, file->path);
    path_fh_init(&state->file, &state->path);
    fh_copy(&state->file.fh, &file->fh);
    path_fh_init(&state->parent, &state->path);
    last_component(state->path.path, state->file.name.name,
        &state->parent.name);
    fh_copy(&state->parent.fh, &parent->fh);

    list_init(&state->client_entry);
    state->status = DELEGATION_GRANTED;
    InitializeSRWLock(&state->lock);
    InitializeConditionVariable(&state->cond);
    state->ref_count = 1;
    *deleg_out = state;
out:
    return status;
}

void nfs41_delegation_ref(
    IN nfs41_delegation_state *state)
{
    const LONG count = InterlockedIncrement(&state->ref_count);
    dprintf(DGLVL, "nfs41_delegation_ref(%s) count %d\n",
        state->path.path, count);
}

void nfs41_delegation_deref(
    IN nfs41_delegation_state *state)
{
    const LONG count = InterlockedDecrement(&state->ref_count);
    dprintf(DGLVL, "nfs41_delegation_deref(%s) count %d\n",
        state->path.path, count);
    if (count == 0)
        free(state);
}

#define open_entry(pos) list_container(pos, nfs41_open_state, client_entry)

static void delegation_remove(
    IN nfs41_client *client,
    IN nfs41_delegation_state *deleg)
{
    struct list_entry *entry;

    /* remove from the client's list */
    EnterCriticalSection(&client->state.lock);
    list_remove(&deleg->client_entry);

    /* remove from each associated open */
    list_for_each(entry, &client->state.opens) {
        nfs41_open_state *open = open_entry(entry);
        AcquireSRWLockExclusive(&open->lock);
        if (open->delegation.state == deleg) {
            /* drop the delegation reference */
            nfs41_delegation_deref(open->delegation.state);
            open->delegation.state = NULL;
        }
        ReleaseSRWLockExclusive(&open->lock);
    }
    LeaveCriticalSection(&client->state.lock);

    /* signal threads waiting on delegreturn */
    AcquireSRWLockExclusive(&deleg->lock);
    deleg->status = DELEGATION_RETURNED;
    WakeAllConditionVariable(&deleg->cond);
    ReleaseSRWLockExclusive(&deleg->lock);

    /* release the client's reference */
    nfs41_delegation_deref(deleg);
}


/* delegation return */
#define lock_entry(pos) list_container(pos, nfs41_lock_state, open_entry)

static bool_t has_delegated_locks(
    IN nfs41_open_state *open)
{
    struct list_entry *entry;
    list_for_each(entry, &open->locks.list) {
        if (lock_entry(entry)->delegated)
            return TRUE;
    }
    return FALSE;
}

static int open_deleg_cmp(const struct list_entry *entry, const void *value)
{
    nfs41_open_state *open = open_entry(entry);
    int result = -1;

    /* open must match the delegation and have state to reclaim */
    AcquireSRWLockShared(&open->lock);
    if (open->delegation.state != value) goto out;
    if (open->do_close && !has_delegated_locks(open)) goto out;
    result = 0;
out:
    ReleaseSRWLockShared(&open->lock);
    return result;
}

/* find the first open that needs recovery */
static nfs41_open_state* deleg_open_find(
    IN struct client_state *state,
    IN const nfs41_delegation_state *deleg)
{
    struct list_entry *entry;
    nfs41_open_state *open = NULL;

    EnterCriticalSection(&state->lock);
    entry = list_search(&state->opens, deleg, open_deleg_cmp);
    if (entry) {
        open = open_entry(entry);
        nfs41_open_state_ref(open); /* return a reference */
    }
    LeaveCriticalSection(&state->lock);
    return open;
}

/* find the first lock that needs recovery */
static bool_t deleg_lock_find(
    IN nfs41_open_state *open,
    OUT nfs41_lock_state *lock_out)
{
    struct list_entry *entry;
    bool_t found = FALSE;

    AcquireSRWLockShared(&open->lock);
    list_for_each(entry, &open->locks.list) {
        nfs41_lock_state *lock = lock_entry(entry);
        if (lock->delegated) {
            /* copy offset, length, type */
            lock_out->offset = lock->offset;
            lock_out->length = lock->length;
            lock_out->exclusive = lock->exclusive;
            lock_out->id = lock->id;
            found = TRUE;
            break;
        }
    }
    ReleaseSRWLockShared(&open->lock);
    return found;
}

/* find the matching lock by id, and reset lock.delegated */
static void deleg_lock_update(
    IN nfs41_open_state *open,
    IN const nfs41_lock_state *source)
{
    struct list_entry *entry;

    AcquireSRWLockExclusive(&open->lock);
    list_for_each(entry, &open->locks.list) {
        nfs41_lock_state *lock = lock_entry(entry);
        if (lock->id == source->id) {
            lock->delegated = FALSE;
            break;
        }
    }
    ReleaseSRWLockExclusive(&open->lock);
}

static int delegation_flush_locks(
    IN nfs41_open_state *open,
    IN bool_t try_recovery)
{
    stateid_arg stateid;
    nfs41_lock_state lock;
    int status = NFS4_OK;

    stateid.open = open;
    stateid.delegation = NULL;

    /* get the starting open/lock stateid */
    AcquireSRWLockShared(&open->lock);
    if (open->locks.stateid.seqid) {
        memcpy(&stateid.stateid, &open->locks.stateid, sizeof(stateid4));
        stateid.type = STATEID_LOCK;
    } else {
        memcpy(&stateid.stateid, &open->stateid, sizeof(stateid4));
        stateid.type = STATEID_OPEN;
    }
    ReleaseSRWLockShared(&open->lock);

    /* send LOCK requests for each delegated lock range */
    while (deleg_lock_find(open, &lock)) {
        status = nfs41_lock(open->session, &open->file,
            &open->owner, lock.exclusive ? WRITE_LT : READ_LT,
            lock.offset, lock.length, FALSE, try_recovery, &stateid);
        if (status)
            break;
        deleg_lock_update(open, &lock);
    }

    /* save the updated lock stateid */
    if (stateid.type == STATEID_LOCK) {
        AcquireSRWLockExclusive(&open->lock);
        if (open->locks.stateid.seqid == 0) {
            /* if it's a new lock stateid, copy it in */
            memcpy(&open->locks.stateid, &stateid.stateid, sizeof(stateid4));
        } else if (stateid.stateid.seqid > open->locks.stateid.seqid) {
            /* update the seqid if it's more recent */
            open->locks.stateid.seqid = stateid.stateid.seqid;
        }
        ReleaseSRWLockExclusive(&open->lock);
    }
    return status;
}

#pragma warning (disable : 4706) /* assignment within conditional expression */

static int delegation_return(
    IN nfs41_client *client,
    IN nfs41_delegation_state *deleg,
    IN bool_t truncate,
    IN bool_t try_recovery)
{
    stateid_arg stateid;
    nfs41_open_state *open;
    int status;

    if (deleg->srv_open) {
        /* make an upcall to the kernel: invalide data cache */
        HANDLE pipe;
        unsigned char inbuf[sizeof(HANDLE)], *buffer = inbuf; 
        DWORD inbuf_len = sizeof(HANDLE), outbuf_len, dstatus;
        uint32_t length;
        dprintf(1, "delegation_return: making a downcall for srv_open=%x\n", 
            deleg->srv_open);
        pipe = CreateFile(NFS41_USER_DEVICE_NAME_A, GENERIC_READ|GENERIC_WRITE, 
                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (pipe == INVALID_HANDLE_VALUE) {
            eprintf("delegation_return: Unable to open downcall pipe %d\n", 
                GetLastError());
            goto out_downcall;
        }
        length = inbuf_len;
        safe_write(&buffer, &length, &deleg->srv_open, sizeof(HANDLE));

        dstatus = DeviceIoControl(pipe, IOCTL_NFS41_INVALCACHE, inbuf, inbuf_len,
            NULL, 0, (LPDWORD)&outbuf_len, NULL);
        if (!dstatus)
            eprintf("IOCTL_NFS41_INVALCACHE failed %d\n", GetLastError());
        CloseHandle(pipe);
    }
out_downcall:

    /* recover opens and locks associated with the delegation */
    while (open = deleg_open_find(&client->state, deleg)) {
        status = nfs41_delegation_to_open(open, try_recovery);
        if (status == NFS4_OK)
            status = delegation_flush_locks(open, try_recovery);
        nfs41_open_state_deref(open);

        if (status)
            break;
    }

    /* return the delegation */
    stateid.type = STATEID_DELEG_FILE;
    stateid.open = NULL;
    stateid.delegation = deleg;
    AcquireSRWLockShared(&deleg->lock);
    memcpy(&stateid.stateid, &deleg->state.stateid, sizeof(stateid4));
    ReleaseSRWLockShared(&deleg->lock);

    status = nfs41_delegreturn(client->session,
        &deleg->file, &stateid, try_recovery);
    if (status == NFS4ERR_BADSESSION)
        goto out;

    delegation_remove(client, deleg);
out:
    return status;
}

/* open delegation */
int nfs41_delegation_granted(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN nfs41_path_fh *file,
    IN open_delegation4 *delegation,
    IN bool_t try_recovery,
    OUT nfs41_delegation_state **deleg_out)
{
    stateid_arg stateid;
    nfs41_client *client = session->client;
    nfs41_delegation_state *state;
    int status = NO_ERROR;

    if (delegation->type != OPEN_DELEGATE_READ &&
        delegation->type != OPEN_DELEGATE_WRITE)
        goto out;

    if (delegation->recalled) {
        status = NFS4ERR_DELEG_REVOKED;
        goto out_return;
    }

    /* allocate the delegation state */
    status = delegation_create(parent, file, delegation, &state);
    if (status)
        goto out_return;

    /* register the delegation with the client */
    EnterCriticalSection(&client->state.lock);
    /* XXX: check for duplicates by fh and stateid? */
    list_add_tail(&client->state.delegations, &state->client_entry);
    LeaveCriticalSection(&client->state.lock);

    nfs41_delegation_ref(state); /* return a reference */
    *deleg_out = state;
out:
    return status;

out_return: /* return the delegation on failure */
    memcpy(&stateid.stateid, &delegation->stateid, sizeof(stateid4));
    stateid.type = STATEID_DELEG_FILE;
    stateid.open = NULL;
    stateid.delegation = NULL;
    nfs41_delegreturn(session, file, &stateid, try_recovery);
    goto out;
}

#define deleg_entry(pos) list_container(pos, nfs41_delegation_state, client_entry)

static int deleg_file_cmp(const struct list_entry *entry, const void *value)
{
    const nfs41_fh *lhs = &deleg_entry(entry)->file.fh;
    const nfs41_fh *rhs = (const nfs41_fh*)value;
    if (lhs->superblock != rhs->superblock) return -1;
    if (lhs->fileid != rhs->fileid) return -1;
    return 0;
}

static bool_t delegation_compatible(
    IN enum open_delegation_type4 type,
    IN uint32_t create,
    IN uint32_t access,
    IN uint32_t deny)
{
    switch (type) {
    case OPEN_DELEGATE_WRITE:
        /* An OPEN_DELEGATE_WRITE delegation allows the client to handle,
         * on its own, all opens. */
        return TRUE;

    case OPEN_DELEGATE_READ:
        /* An OPEN_DELEGATE_READ delegation allows a client to handle,
         * on its own, requests to open a file for reading that do not
         * deny OPEN4_SHARE_ACCESS_READ access to others. */
        if (create == OPEN4_CREATE)
            return FALSE;
        if (access & OPEN4_SHARE_ACCESS_WRITE || deny & OPEN4_SHARE_DENY_READ)
            return FALSE;
        return TRUE;

    default:
        return FALSE;
    }
}

static int delegation_find(
    IN nfs41_client *client,
    IN const void *value,
    IN list_compare_fn cmp,
    OUT nfs41_delegation_state **deleg_out)
{
    struct list_entry *entry;
    int status = NFS4ERR_BADHANDLE;

    EnterCriticalSection(&client->state.lock);
    entry = list_search(&client->state.delegations, value, cmp);
    if (entry) {
        /* return a reference to the delegation */
        *deleg_out = deleg_entry(entry);
        nfs41_delegation_ref(*deleg_out);

        /* move to the 'most recently used' end of the list */
        list_remove(entry);
        list_add_tail(&client->state.delegations, entry);
        status = NFS4_OK;
    }
    LeaveCriticalSection(&client->state.lock);
    return status;
}

static int delegation_truncate(
    IN nfs41_delegation_state *deleg,
    IN nfs41_client *client,
    IN stateid_arg *stateid,
    IN nfs41_file_info *info)
{
    nfs41_superblock *superblock = deleg->file.fh.superblock;

    /* use SETATTR to truncate the file */
    info->attrmask.arr[1] |= FATTR4_WORD1_TIME_CREATE |
        FATTR4_WORD1_TIME_MODIFY_SET;

    get_nfs_time(&info->time_create);
    get_nfs_time(&info->time_modify);
    info->time_delta = &superblock->time_delta;

    /* mask out unsupported attributes */
    nfs41_superblock_supported_attrs(superblock, &info->attrmask);

    return nfs41_setattr(client->session, &deleg->file, stateid, info);
}

int nfs41_delegate_open(
    IN nfs41_open_state *state,
    IN uint32_t create,
    IN OPTIONAL nfs41_file_info *createattrs,
    OUT nfs41_file_info *info)
{
    nfs41_client *client = state->session->client;
    nfs41_path_fh *file = &state->file;
    uint32_t access = state->share_access;
    uint32_t deny = state->share_deny;
    nfs41_delegation_state *deleg;
    stateid_arg stateid;
    int status;

    /* search for a delegation with this filehandle */
    status = delegation_find(client, &file->fh, deleg_file_cmp, &deleg);
    if (status)
        goto out;

    AcquireSRWLockExclusive(&deleg->lock);
    if (deleg->status != DELEGATION_GRANTED) {
        /* the delegation is being returned, wait for it to finish */
        while (deleg->status != DELEGATION_RETURNED)
            SleepConditionVariableSRW(&deleg->cond, &deleg->lock, INFINITE, 0);
        status = NFS4ERR_BADHANDLE;
    }
    else if (!delegation_compatible(deleg->state.type, create, access, deny)) {
#ifdef DELEGATION_RETURN_ON_CONFLICT
        /* this open will conflict, start the delegation return */
        deleg->status = DELEGATION_RETURNING;
        status = NFS4ERR_DELEG_REVOKED;
#else
        status = NFS4ERR_BADHANDLE;
#endif
    } else if (create == OPEN4_CREATE) {
        /* copy the stateid for SETATTR */
        stateid.open = NULL;
        stateid.delegation = deleg;
        stateid.type = STATEID_DELEG_FILE;
        memcpy(&stateid.stateid, &deleg->state.stateid, sizeof(stateid4));
    }
    if (!status) {
        dprintf(1, "nfs41_delegate_open: updating srv_open from %x to %x\n", 
            deleg->srv_open, state->srv_open);
        deleg->srv_open = state->srv_open;
    }
    ReleaseSRWLockExclusive(&deleg->lock);

    if (status == NFS4ERR_DELEG_REVOKED)
        goto out_return;
    if (status)
        goto out_deleg;

    if (create == OPEN4_CREATE) {
        memcpy(info, createattrs, sizeof(nfs41_file_info));

        /* write delegations allow us to simulate OPEN4_CREATE with SETATTR */
        status = delegation_truncate(deleg, client, &stateid, info);
        if (status)
            goto out_deleg;
    }

    /* TODO: check access against deleg->state.permissions or send ACCESS */

    state->delegation.state = deleg;
    status = NFS4_OK;
out:
    return status;

out_return:
    delegation_return(client, deleg, create == OPEN4_CREATE, TRUE);

out_deleg:
    nfs41_delegation_deref(deleg);
    goto out;
}

int nfs41_delegation_to_open(
    IN nfs41_open_state *open,
    IN bool_t try_recovery)
{
    open_delegation4 ignore;
    open_claim4 claim;
    stateid4 open_stateid = { 0 };
    stateid_arg deleg_stateid;
    int status = NFS4_OK;

    AcquireSRWLockExclusive(&open->lock);
    if (open->delegation.state == NULL) /* no delegation to reclaim */
        goto out_unlock;

    if (open->do_close) /* already have an open stateid */
        goto out_unlock;

    /* if another thread is reclaiming the open stateid,
     * wait for it to finish before returning success */
    if (open->delegation.reclaim) {
        do {
            SleepConditionVariableSRW(&open->delegation.cond, &open->lock,
                INFINITE, 0);
        } while (open->delegation.reclaim);
        if (open->do_close)
            goto out_unlock;
    }
    open->delegation.reclaim = 1;

    AcquireSRWLockShared(&open->delegation.state->lock);
    deleg_stateid.open = open;
    deleg_stateid.delegation = NULL;
    deleg_stateid.type = STATEID_DELEG_FILE;
    memcpy(&deleg_stateid.stateid, &open->delegation.state->state.stateid,
        sizeof(stateid4));
    ReleaseSRWLockShared(&open->delegation.state->lock);

    ReleaseSRWLockExclusive(&open->lock);

    /* send OPEN with CLAIM_DELEGATE_CUR */
    claim.claim = CLAIM_DELEGATE_CUR;
    claim.u.deleg_cur.delegate_stateid = &deleg_stateid;
    claim.u.deleg_cur.name = &open->file.name;

    status = nfs41_open(open->session, &open->parent, &open->file,
        &open->owner, &claim, open->share_access, open->share_deny,
        OPEN4_NOCREATE, 0, NULL, try_recovery, &open_stateid, &ignore, NULL);

    AcquireSRWLockExclusive(&open->lock);
    if (status == NFS4_OK) {
        /* save the new open stateid */
        memcpy(&open->stateid, &open_stateid, sizeof(stateid4));
        open->do_close = 1;
    } else if (open->do_close && (status == NFS4ERR_BAD_STATEID ||
        status == NFS4ERR_STALE_STATEID || status == NFS4ERR_EXPIRED)) {
        /* something triggered client state recovery, and the open stateid
         * has already been reclaimed; see recover_stateid_delegation() */
        status = NFS4_OK;
    }
    open->delegation.reclaim = 0;

    /* signal anyone waiting on the open stateid */
    WakeAllConditionVariable(&open->delegation.cond);
out_unlock:
    ReleaseSRWLockExclusive(&open->lock);
    if (status)
        eprintf("nfs41_delegation_to_open(%p) failed with %s\n",
            open, nfs_error_string(status));
    return status;
}

void nfs41_delegation_remove_srvopen(
    IN nfs41_session *session,
    IN nfs41_path_fh *file)
{
    nfs41_delegation_state *deleg = NULL;

    /* find a delegation for this file */
    if (delegation_find(session->client, &file->fh, deleg_file_cmp, &deleg))
        return;
    dprintf(1, "nfs41_delegation_remove_srvopen: removing reference to "
        "srv_open=%x\n", deleg->srv_open);
    AcquireSRWLockExclusive(&deleg->lock);
    deleg->srv_open = NULL;
    ReleaseSRWLockExclusive(&deleg->lock);
    nfs41_delegation_deref(deleg);
}

/* synchronous delegation return */
#ifdef DELEGATION_RETURN_ON_CONFLICT
int nfs41_delegation_return(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
#ifndef __REACTOS__
    IN enum open_delegation_type4 access,
#else
    IN int access,
#endif
    IN bool_t truncate)
{
    nfs41_client *client = session->client;
    nfs41_delegation_state *deleg;
    int status;

    /* find a delegation for this file */
    status = delegation_find(client, &file->fh, deleg_file_cmp, &deleg);
    if (status)
        goto out;

    AcquireSRWLockExclusive(&deleg->lock);
    if (deleg->status == DELEGATION_GRANTED) {
        /* return unless delegation is write and access is read */
        if (deleg->state.type != OPEN_DELEGATE_WRITE
            || access != OPEN_DELEGATE_READ) {
            deleg->status = DELEGATION_RETURNING;
            status = NFS4ERR_DELEG_REVOKED;
        }
    } else {
        /* the delegation is being returned, wait for it to finish */
        while (deleg->status == DELEGATION_RETURNING)
            SleepConditionVariableSRW(&deleg->cond, &deleg->lock, INFINITE, 0);
        status = NFS4ERR_BADHANDLE;
    }
    ReleaseSRWLockExclusive(&deleg->lock);

    if (status == NFS4ERR_DELEG_REVOKED) {
        delegation_return(client, deleg, truncate, TRUE);
        status = NFS4_OK;
    }

    nfs41_delegation_deref(deleg);
out:
    return status;
}
#endif


/* asynchronous delegation recall */
struct recall_thread_args {
    nfs41_client            *client;
    nfs41_delegation_state  *delegation;
    bool_t                  truncate;
};

static unsigned int WINAPI delegation_recall_thread(void *args)
{
    struct recall_thread_args *recall = (struct recall_thread_args*)args;

    delegation_return(recall->client, recall->delegation, recall->truncate, TRUE);

    /* clean up thread arguments */
    nfs41_delegation_deref(recall->delegation);
    nfs41_root_deref(recall->client->root);
    free(recall);
    return 0;
}

static int deleg_stateid_cmp(const struct list_entry *entry, const void *value)
{
    const stateid4 *lhs = &deleg_entry(entry)->state.stateid;
    const stateid4 *rhs = (const stateid4*)value;
    return memcmp(lhs->other, rhs->other, NFS4_STATEID_OTHER);
}

int nfs41_delegation_recall(
    IN nfs41_client *client,
    IN nfs41_fh *fh,
    IN const stateid4 *stateid,
    IN bool_t truncate)
{
    nfs41_delegation_state *deleg;
    struct recall_thread_args *args;
    int status;

    dprintf(2, "--> nfs41_delegation_recall()\n");

    /* search for the delegation by stateid instead of filehandle;
     * deleg_file_cmp() relies on a proper superblock and fileid,
     * which we don't get with CB_RECALL */
    status = delegation_find(client, stateid, deleg_stateid_cmp, &deleg);
    if (status)
        goto out;

    AcquireSRWLockExclusive(&deleg->lock);
    if (deleg->state.recalled) {
        /* return BADHANDLE if we've already responded to CB_RECALL */
        status = NFS4ERR_BADHANDLE;
    } else {
        deleg->state.recalled = 1;

        if (deleg->status == DELEGATION_GRANTED) {
            /* start the delegation return */
            deleg->status = DELEGATION_RETURNING;
            status = NFS4ERR_DELEG_REVOKED;
        } /* else return NFS4_OK */
    }
    ReleaseSRWLockExclusive(&deleg->lock);

    if (status != NFS4ERR_DELEG_REVOKED)
        goto out_deleg;

    /* allocate thread arguments */
    args = calloc(1, sizeof(struct recall_thread_args));
    if (args == NULL) {
        status = NFS4ERR_SERVERFAULT;
        eprintf("nfs41_delegation_recall() failed to allocate arguments\n");
        goto out_deleg;
    }

    /* hold a reference on the root */
    nfs41_root_ref(client->root);
    args->client = client;
    args->delegation = deleg;
    args->truncate = truncate;

    /* the callback thread can't make rpc calls, so spawn a separate thread */
    if (_beginthreadex(NULL, 0, delegation_recall_thread, args, 0, NULL) == 0) {
        status = NFS4ERR_SERVERFAULT;
        eprintf("nfs41_delegation_recall() failed to start thread\n");
        goto out_args;
    }
    status = NFS4_OK;
out:
    dprintf(DGLVL, "<-- nfs41_delegation_recall() returning %s\n",
        nfs_error_string(status));
    return status;

out_args:
    free(args);
    nfs41_root_deref(client->root);
out_deleg:
    nfs41_delegation_deref(deleg);
    goto out;
}


static int deleg_fh_cmp(const struct list_entry *entry, const void *value)
{
    const nfs41_fh *lhs = &deleg_entry(entry)->file.fh;
    const nfs41_fh *rhs = (const nfs41_fh*)value;
    if (lhs->len != rhs->len) return -1;
    return memcmp(lhs->fh, rhs->fh, lhs->len);
}

int nfs41_delegation_getattr(
    IN nfs41_client *client,
    IN const nfs41_fh *fh,
    IN const bitmap4 *attr_request,
    OUT nfs41_file_info *info)
{
    nfs41_delegation_state *deleg;
    uint64_t fileid;
    int status;

    dprintf(2, "--> nfs41_delegation_getattr()\n");

    /* search for a delegation on this file handle */
    status = delegation_find(client, fh, deleg_fh_cmp, &deleg);
    if (status)
        goto out;

    AcquireSRWLockShared(&deleg->lock);
    fileid = deleg->file.fh.fileid;
    if (deleg->status != DELEGATION_GRANTED ||
        deleg->state.type != OPEN_DELEGATE_WRITE) {
        status = NFS4ERR_BADHANDLE;
    }
    ReleaseSRWLockShared(&deleg->lock);
    if (status)
        goto out_deleg;

    ZeroMemory(info, sizeof(nfs41_file_info));

    /* find attributes for the given fileid */
    status = nfs41_attr_cache_lookup(
        client_name_cache(client), fileid, info);
    if (status) {
        status = NFS4ERR_BADHANDLE;
        goto out_deleg;
    }
out_deleg:
    nfs41_delegation_deref(deleg);
out:
    dprintf(DGLVL, "<-- nfs41_delegation_getattr() returning %s\n",
        nfs_error_string(status));
    return status;
}


void nfs41_client_delegation_free(
    IN nfs41_client *client)
{
    struct list_entry *entry, *tmp;

    EnterCriticalSection(&client->state.lock);
    list_for_each_tmp (entry, tmp, &client->state.delegations) {
        list_remove(entry);
        nfs41_delegation_deref(deleg_entry(entry));
    }
    LeaveCriticalSection(&client->state.lock);
}


static int delegation_recovery_status(
    IN nfs41_delegation_state *deleg)
{
    int status = NFS4_OK;

    AcquireSRWLockExclusive(&deleg->lock);
    if (deleg->status == DELEGATION_GRANTED) {
        if (deleg->revoked) {
            deleg->status = DELEGATION_RETURNED;
            status = NFS4ERR_BADHANDLE;
        } else if (deleg->state.recalled) {
            deleg->status = DELEGATION_RETURNING;
            status = NFS4ERR_DELEG_REVOKED;
        }
    }
    ReleaseSRWLockExclusive(&deleg->lock);
    return status;
}

int nfs41_client_delegation_recovery(
    IN nfs41_client *client)
{
    struct list_entry *entry, *tmp;
    nfs41_delegation_state *deleg;
    int status = NFS4_OK;

    list_for_each_tmp(entry, tmp, &client->state.delegations) {
        deleg = list_container(entry, nfs41_delegation_state, client_entry);

        status = delegation_recovery_status(deleg);
        switch (status) {
        case NFS4ERR_DELEG_REVOKED:
            /* the delegation was reclaimed, but flagged as recalled;
             * return it with try_recovery=FALSE */
            status = delegation_return(client, deleg, FALSE, FALSE);
            break;

        case NFS4ERR_BADHANDLE:
            /* reclaim failed, so we have no delegation state on the server;
             * 'forget' the delegation without trying to return it */
            delegation_remove(client, deleg);
            status = NFS4_OK;
            break;
        }

        if (status == NFS4ERR_BADSESSION)
            goto out;
    }

    /* use DELEGPURGE to indicate that we're done reclaiming delegations */
    status = nfs41_delegpurge(client->session);

    /* support for DELEGPURGE is optional; ignore any errors but BADSESSION */
    if (status != NFS4ERR_BADSESSION)
        status = NFS4_OK;
out:
    return status;
}


int nfs41_client_delegation_return_lru(
    IN nfs41_client *client)
{
    struct list_entry *entry;
    nfs41_delegation_state *state = NULL;
    int status = NFS4ERR_BADHANDLE;

    /* starting from the least recently opened, find and return
     * the first delegation that's not 'in use' (currently open) */

    /* TODO: use a more robust algorithm, taking into account:
     *  -number of total opens
     *  -time since last operation on an associated open, or
     *  -number of operations/second over last n seconds */
    EnterCriticalSection(&client->state.lock);
    list_for_each(entry, &client->state.delegations) {
        state = deleg_entry(entry);

        /* skip if it's currently in use for an open; note that ref_count
         * can't go from 1 to 2 without holding client->state.lock */
        if (state->ref_count > 1)
            continue;

        AcquireSRWLockExclusive(&state->lock);
        if (state->status == DELEGATION_GRANTED) {
            /* start returning the delegation */
            state->status = DELEGATION_RETURNING;
            status = NFS4ERR_DELEG_REVOKED;
        }
        ReleaseSRWLockExclusive(&state->lock);

        if (status == NFS4ERR_DELEG_REVOKED)
            break;
    }
    LeaveCriticalSection(&client->state.lock);

    if (status == NFS4ERR_DELEG_REVOKED)
        status = delegation_return(client, state, FALSE, TRUE);
    return status;
}
