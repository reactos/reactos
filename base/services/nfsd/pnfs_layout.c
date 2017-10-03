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

#include "nfs41_ops.h"
#include "nfs41_callback.h"
#include "util.h"
#include "daemon_debug.h"


#define FLLVL 2 /* dprintf level for file layout logging */


/* pnfs_layout_list */
struct pnfs_layout_list {
    struct list_entry       head;
    CRITICAL_SECTION        lock;
};

#define state_entry(pos) list_container(pos, pnfs_layout_state, entry)
#define layout_entry(pos) list_container(pos, pnfs_layout, entry)
#define file_layout_entry(pos) list_container(pos, pnfs_file_layout, layout.entry)

static enum pnfs_status layout_state_create(
    IN const nfs41_fh *meta_fh,
    OUT pnfs_layout_state **layout_out)
{
    pnfs_layout_state *layout;
    enum pnfs_status status = PNFS_SUCCESS;

    layout = calloc(1, sizeof(pnfs_layout_state));
    if (layout == NULL) {
        status = PNFSERR_RESOURCES;
        goto out;
    }

    fh_copy(&layout->meta_fh, meta_fh);
    list_init(&layout->layouts);
    list_init(&layout->recalls);
    InitializeSRWLock(&layout->lock);
    InitializeConditionVariable(&layout->cond);

    *layout_out = layout;
out:
    return status;
}

static void file_layout_free(
    IN pnfs_file_layout *layout)
{
    if (layout->device) pnfs_file_device_put(layout->device);
    free(layout->filehandles.arr);
    free(layout);
}

static void layout_state_free_layouts(
    IN pnfs_layout_state *state)
{
    struct list_entry *entry, *tmp;
    list_for_each_tmp(entry, tmp, &state->layouts)
        file_layout_free(file_layout_entry(entry));
    list_init(&state->layouts);
}

static void layout_state_free_recalls(
    IN pnfs_layout_state *state)
{
    struct list_entry *entry, *tmp;
    list_for_each_tmp(entry, tmp, &state->recalls)
        free(layout_entry(entry));
    list_init(&state->recalls);
}

static void layout_state_free(
    IN pnfs_layout_state *state)
{
    layout_state_free_layouts(state);
    layout_state_free_recalls(state);
    free(state);
}

static int layout_entry_compare(
    IN const struct list_entry *entry,
    IN const void *value)
{
    const pnfs_layout_state *layout = state_entry(entry);
    const nfs41_fh *meta_fh = (const nfs41_fh*)value;
    const nfs41_fh *layout_fh = (const nfs41_fh*)&layout->meta_fh;
    const uint32_t diff = layout_fh->len - meta_fh->len;
    return diff ? diff : memcmp(layout_fh->fh, meta_fh->fh, meta_fh->len);
}

static enum pnfs_status layout_entry_find(
    IN struct pnfs_layout_list *layouts,
    IN const nfs41_fh *meta_fh,
    OUT struct list_entry **entry_out)
{
    *entry_out = list_search(&layouts->head, meta_fh, layout_entry_compare);
    return *entry_out ? PNFS_SUCCESS : PNFSERR_NO_LAYOUT;
}

enum pnfs_status pnfs_layout_list_create(
    OUT struct pnfs_layout_list **layouts_out)
{
    struct pnfs_layout_list *layouts;
    enum pnfs_status status = PNFS_SUCCESS;

    layouts = calloc(1, sizeof(struct pnfs_layout_list));
    if (layouts == NULL) {
        status = PNFSERR_RESOURCES;
        goto out;
    }
    list_init(&layouts->head);
    InitializeCriticalSection(&layouts->lock);
    *layouts_out = layouts;
out:
    return status;
}

void pnfs_layout_list_free(
    IN struct pnfs_layout_list *layouts)
{
    struct list_entry *entry, *tmp;

    EnterCriticalSection(&layouts->lock);

    list_for_each_tmp(entry, tmp, &layouts->head)
        layout_state_free(state_entry(entry));

    LeaveCriticalSection(&layouts->lock);
    DeleteCriticalSection(&layouts->lock);
    free(layouts);
}

static enum pnfs_status layout_state_find_or_create(
    IN struct pnfs_layout_list *layouts,
    IN const nfs41_fh *meta_fh,
    OUT pnfs_layout_state **layout_out)
{
    struct list_entry *entry;
    enum pnfs_status status;

    dprintf(FLLVL, "--> layout_state_find_or_create()\n");

    EnterCriticalSection(&layouts->lock);

    /* search for an existing layout */
    status = layout_entry_find(layouts, meta_fh, &entry);
    if (status) {
        /* create a new layout */
        pnfs_layout_state *layout;
        status = layout_state_create(meta_fh, &layout);
        if (status == PNFS_SUCCESS) {
            /* add it to the list */
            list_add_head(&layouts->head, &layout->entry);
            *layout_out = layout;

            dprintf(FLLVL, "<-- layout_state_find_or_create() "
                "returning new layout %p\n", layout);
        } else {
            dprintf(FLLVL, "<-- layout_state_find_or_create() "
                "returning %s\n", pnfs_error_string(status));
        }
    } else {
        *layout_out = state_entry(entry);

        dprintf(FLLVL, "<-- layout_state_find_or_create() "
            "returning existing layout %p\n", *layout_out);
    }

    LeaveCriticalSection(&layouts->lock);
    return status;
}

static enum pnfs_status layout_state_find_and_delete(
    IN struct pnfs_layout_list *layouts,
    IN const nfs41_fh *meta_fh)
{
    struct list_entry *entry;
    enum pnfs_status status;

    dprintf(FLLVL, "--> layout_state_find_and_delete()\n");

    EnterCriticalSection(&layouts->lock);

    status = layout_entry_find(layouts, meta_fh, &entry);
    if (status == PNFS_SUCCESS) {
        list_remove(entry);
        layout_state_free(state_entry(entry));
    }

    LeaveCriticalSection(&layouts->lock);

    dprintf(FLLVL, "<-- layout_state_find_and_delete() "
        "returning %s\n", pnfs_error_string(status));
    return status;
}


/* pnfs_file_layout */
static uint64_t range_max(
    IN const pnfs_layout *layout)
{
    uint64_t result = layout->offset + layout->length;
    return result < layout->offset ? NFS4_UINT64_MAX : result;
}

static bool_t layout_sanity_check(
    IN pnfs_file_layout *layout)
{
    /* prevent div/0 */
    if (layout->layout.length == 0 ||
        layout->layout.iomode < PNFS_IOMODE_READ ||
        layout->layout.iomode > PNFS_IOMODE_RW ||
        layout_unit_size(layout) == 0)
        return FALSE;

    /* put a cap on layout.length to prevent overflow */
    layout->layout.length = range_max(&layout->layout) - layout->layout.offset;
    return TRUE;
}

static int layout_filehandles_cmp(
    IN const pnfs_file_layout_handles *lhs,
    IN const pnfs_file_layout_handles *rhs)
{
    const uint32_t diff = rhs->count - lhs->count;
    return diff ? diff : memcmp(rhs->arr, lhs->arr,
        rhs->count * sizeof(nfs41_path_fh));
}

static bool_t layout_merge_segments(
    IN pnfs_file_layout *to,
    IN pnfs_file_layout *from)
{
    const uint64_t to_max = range_max(&to->layout);
    const uint64_t from_max = range_max(&from->layout);

    /* cannot merge a segment with itself */
    if (to == from)
        return FALSE;

    /* the ranges must meet or overlap */
    if (to_max < from->layout.offset || from_max < to->layout.offset)
        return FALSE;

    /* the following fields must match: */
    if (to->layout.iomode != from->layout.iomode ||
        to->layout.type != from->layout.type ||
        layout_filehandles_cmp(&to->filehandles, &from->filehandles) != 0 ||
        memcmp(to->deviceid, from->deviceid, PNFS_DEVICEID_SIZE) != 0 ||
        to->pattern_offset != from->pattern_offset ||
        to->first_index != from->first_index ||
        to->util != from->util)
        return FALSE;

    dprintf(FLLVL, "merging layout range {%llu, %llu} with {%llu, %llu}\n",
        to->layout.offset, to->layout.length,
        from->layout.offset, from->layout.length);

    /* calculate the union of the two ranges */
    to->layout.offset = min(to->layout.offset, from->layout.offset);
    to->layout.length = max(to_max, from_max) - to->layout.offset;
    return TRUE;
}

static enum pnfs_status layout_state_merge(
    IN pnfs_layout_state *state,
    IN pnfs_file_layout *from)
{
    struct list_entry *entry, *tmp;
    pnfs_file_layout *to;
    enum pnfs_status status = PNFSERR_NO_LAYOUT;

    /* attempt to merge the new segment with each existing segment */
    list_for_each_tmp(entry, tmp, &state->layouts) {
        to = file_layout_entry(entry);
        if (!layout_merge_segments(to, from))
            continue;

        /* on success, remove/free the new segment */
        list_remove(&from->layout.entry);
        file_layout_free(from);
        status = PNFS_SUCCESS;

        /* because the existing segment 'to' has grown, we may
         * be able to merge it with later segments */
        from = to;

        /* but if there could be io threads referencing this segment,
         * we can't free it until io is finished */
        if (state->io_count)
            break;
    }
    return status;
}

static void layout_ordered_insert(
    IN pnfs_layout_state *state,
    IN pnfs_layout *layout)
{
    struct list_entry *entry;
    list_for_each(entry, &state->layouts) {
        pnfs_layout *existing = layout_entry(entry);

        /* maintain an order of increasing offset */
        if (existing->offset < layout->offset)
            continue;

        /* when offsets are equal, prefer a longer segment first */
        if (existing->offset == layout->offset &&
            existing->length > layout->length)
            continue;

        list_add(&layout->entry, existing->entry.prev, &existing->entry);
        return;
    }

    list_add_tail(&state->layouts, &layout->entry);
}

static enum pnfs_status layout_update_range(
    IN OUT pnfs_layout_state *state,
    IN const struct list_entry *layouts)
{
    struct list_entry *entry, *tmp;
    pnfs_file_layout *layout;
    enum pnfs_status status = PNFSERR_NO_LAYOUT;

    list_for_each_tmp(entry, tmp, layouts) {
        layout = file_layout_entry(entry);

        /* don't know what to do with non-file layouts */
        if (layout->layout.type != PNFS_LAYOUTTYPE_FILE)
            continue;

        if (!layout_sanity_check(layout)) {
            file_layout_free(layout);
            continue;
        }

        /* attempt to merge the range with existing segments */
        status = layout_state_merge(state, layout);
        if (status) {
            dprintf(FLLVL, "saving new layout:\n");
            dprint_layout(FLLVL, layout);

            layout_ordered_insert(state, &layout->layout);
            status = PNFS_SUCCESS;
        }
    }
    return status;
}

static enum pnfs_status layout_update_stateid(
    IN OUT pnfs_layout_state *state,
    IN const stateid4 *stateid)
{
    enum pnfs_status status = PNFS_SUCCESS;

    if (state->stateid.seqid == 0) {
        /* save a new layout stateid */
        memcpy(&state->stateid, stateid, sizeof(stateid4));
    } else if (memcmp(&state->stateid.other, stateid->other, 
                        NFS4_STATEID_OTHER) == 0) {
        /* update an existing layout stateid */
        state->stateid.seqid = stateid->seqid;
    } else {
        status = PNFSERR_NO_LAYOUT;
    }
    return status;
}

static enum pnfs_status layout_update(
    IN OUT pnfs_layout_state *state,
    IN const pnfs_layoutget_res_ok *layoutget_res)
{
    enum pnfs_status status;

    /* update the layout ranges held by the client */
    status = layout_update_range(state, &layoutget_res->layouts);
    if (status) {
        eprintf("LAYOUTGET didn't return any file layouts\n");
        goto out;
    }
    /* update the layout stateid */
    status = layout_update_stateid(state, &layoutget_res->stateid);
    if (status) {
        eprintf("LAYOUTGET returned a new stateid when we already had one\n");
        goto out;
    }
    /* if a previous LAYOUTGET set return_on_close, don't overwrite it */
    if (!state->return_on_close)
        state->return_on_close = layoutget_res->return_on_close;
out:
    return status;
}

static enum pnfs_status file_layout_fetch(
    IN OUT pnfs_layout_state *state,
    IN nfs41_session *session,
    IN nfs41_path_fh *meta_file,
    IN stateid_arg *stateid,
    IN enum pnfs_iomode iomode,
    IN uint64_t offset,
    IN uint64_t minlength,
    IN uint64_t length)
{
    pnfs_layoutget_res_ok layoutget_res = { 0 };
    enum pnfs_status pnfsstat = PNFS_SUCCESS;
    enum nfsstat4 nfsstat;

    dprintf(FLLVL, "--> file_layout_fetch(%s, seqid=%u)\n",
        pnfs_iomode_string(iomode), state->stateid.seqid);

    list_init(&layoutget_res.layouts);

    /* drop the lock during the rpc call */
    ReleaseSRWLockExclusive(&state->lock);
    nfsstat = pnfs_rpc_layoutget(session, meta_file, stateid,
        iomode, offset, minlength, length, &layoutget_res);
    AcquireSRWLockExclusive(&state->lock);

    if (nfsstat) {
        dprintf(FLLVL, "pnfs_rpc_layoutget() failed with %s\n",
            nfs_error_string(nfsstat));
        pnfsstat = PNFSERR_NOT_SUPPORTED;
    }

    switch (nfsstat) {
    case NFS4_OK:
        /* use the LAYOUTGET results to update our view of the layout */
        pnfsstat = layout_update(state, &layoutget_res);
        break;

    case NFS4ERR_BADIOMODE:
        /* don't try RW again */
        if (iomode == PNFS_IOMODE_RW)
            state->status |= PNFS_LAYOUT_NOT_RW;
        break;

    case NFS4ERR_LAYOUTUNAVAILABLE:
    case NFS4ERR_UNKNOWN_LAYOUTTYPE:
    case NFS4ERR_BADLAYOUT:
        /* don't try again at all */
        state->status |= PNFS_LAYOUT_UNAVAILABLE;
        break;
    }

    dprintf(FLLVL, "<-- file_layout_fetch() returning %s\n",
        pnfs_error_string(pnfsstat));
    return pnfsstat;
}

/* returns PNFS_SUCCESS if the client holds valid layouts that cover
 * the entire range requested.  otherwise, returns PNFS_PENDING and
 * sets 'offset_missing' to the lowest offset that is not covered */
static enum pnfs_status layout_coverage_status(
    IN pnfs_layout_state *state,
    IN enum pnfs_iomode iomode,
    IN uint64_t offset,
    IN uint64_t length,
    OUT uint64_t *offset_missing)
{
    uint64_t position = offset;
    struct list_entry *entry;

    list_for_each(entry, &state->layouts) {
        /* if the current position intersects with a compatible
         * layout, move the position to the end of that layout */
        pnfs_layout *layout = layout_entry(entry);
        if (layout->iomode >= iomode &&
            layout->offset <= position &&
            position < layout->offset + layout->length)
            position = layout->offset + layout->length;
    }

    if (position >= offset + length)
        return PNFS_SUCCESS;

    *offset_missing = position;
    return PNFS_PENDING;
}

static enum pnfs_status layout_fetch(
    IN pnfs_layout_state *state,
    IN nfs41_session *session,
    IN nfs41_path_fh *meta_file,
    IN stateid_arg *stateid,
    IN enum pnfs_iomode iomode,
    IN uint64_t offset,
    IN uint64_t length)
{
    stateid_arg layout_stateid = { 0 };
    enum pnfs_status status = PNFS_PENDING;

    /* check for previous errors from LAYOUTGET */
    if ((state->status & PNFS_LAYOUT_UNAVAILABLE) ||
        ((state->status & PNFS_LAYOUT_NOT_RW) && iomode == PNFS_IOMODE_RW)) {
        status = PNFSERR_NO_LAYOUT;
        goto out;
    }

    /* wait for any pending LAYOUTGETs/LAYOUTRETURNs */
    while (state->pending)
        SleepConditionVariableSRW(&state->cond, &state->lock, INFINITE, 0);
    state->pending = TRUE;

    /* if there's an existing layout stateid, use it */
    if (state->stateid.seqid) {
        memcpy(&layout_stateid.stateid, &state->stateid, sizeof(stateid4));
        layout_stateid.type = STATEID_LAYOUT;
        stateid = &layout_stateid;
    }

    if ((state->status & PNFS_LAYOUT_NOT_RW) == 0) {
        /* try to get a RW layout first */
        status = file_layout_fetch(state, session, meta_file,
            stateid, PNFS_IOMODE_RW, offset, length, NFS4_UINT64_MAX);
    }

    if (status && iomode == PNFS_IOMODE_READ) {
        /* fall back on READ if necessary */
        status = file_layout_fetch(state, session, meta_file,
            stateid, iomode, offset, length, NFS4_UINT64_MAX);
    }

    state->pending = FALSE;
    WakeConditionVariable(&state->cond);
out:
    return status;
}

static enum pnfs_status device_status(
    IN pnfs_layout_state *state,
    IN uint64_t offset,
    IN uint64_t length,
    OUT unsigned char *deviceid)
{
    struct list_entry *entry;
    enum pnfs_status status = PNFS_SUCCESS;

    list_for_each(entry, &state->layouts) {
        pnfs_file_layout *layout = file_layout_entry(entry);

        if (layout->device == NULL) {
            /* copy missing deviceid */
            memcpy(deviceid, layout->deviceid, PNFS_DEVICEID_SIZE);
            status = PNFS_PENDING;
            break;
        }
    }
    return status;
}

static void device_assign(
    IN pnfs_layout_state *state,
    IN const unsigned char *deviceid,
    IN pnfs_file_device *device)
{
    struct list_entry *entry;
    list_for_each(entry, &state->layouts) {
        pnfs_file_layout *layout = file_layout_entry(entry);

        /* assign the device to any matching layouts */
        if (layout->device == NULL &&
            memcmp(layout->deviceid, deviceid, PNFS_DEVICEID_SIZE) == 0) {
            layout->device = device;

            /* XXX: only assign the device to a single segment, because
             * pnfs_file_device_get() only gives us a single reference */
            break;
        }
    }
}

static enum pnfs_status device_fetch(
    IN pnfs_layout_state *state,
    IN nfs41_session *session,
    IN unsigned char *deviceid)
{
    pnfs_file_device *device;
    enum pnfs_status status;

    /* drop the layoutstate lock for the rpc call */
    ReleaseSRWLockExclusive(&state->lock);
    status = pnfs_file_device_get(session,
        session->client->devices, deviceid, &device);
    AcquireSRWLockExclusive(&state->lock);

    if (status == PNFS_SUCCESS)
        device_assign(state, deviceid, device);
    return status;
}


/* nfs41_open_state */
static enum pnfs_status client_supports_pnfs(
    IN nfs41_client *client)
{
    enum pnfs_status status;
    AcquireSRWLockShared(&client->exid_lock);
    status = client->roles & EXCHGID4_FLAG_USE_PNFS_MDS
        ? PNFS_SUCCESS : PNFSERR_NOT_SUPPORTED;
    ReleaseSRWLockShared(&client->exid_lock);
    return status;
}

static enum pnfs_status fs_supports_layout(
    IN const nfs41_superblock *superblock,
    IN enum pnfs_layout_type type)
{
    const uint32_t flag = 1 << (type - 1);
    return (superblock->layout_types & flag) == 0
        ? PNFSERR_NOT_SUPPORTED : PNFS_SUCCESS;
}

static enum pnfs_status open_state_layout_cached(
    IN nfs41_open_state *state,
    OUT pnfs_layout_state **layout_out)
{
    enum pnfs_status status = PNFSERR_NO_LAYOUT;

    if (state->layout) {
        status = PNFS_SUCCESS;
        *layout_out = state->layout;

        dprintf(FLLVL, "pnfs_open_state_layout() found "
            "cached layout %p\n", *layout_out);
    }
    return status;
}

enum pnfs_status pnfs_layout_state_open(
    IN nfs41_open_state *state,
    OUT pnfs_layout_state **layout_out)
{
    struct pnfs_layout_list *layouts = state->session->client->layouts;
    nfs41_session *session = state->session;
    pnfs_layout_state *layout;
    enum pnfs_status status;

    dprintf(FLLVL, "--> pnfs_layout_state_open()\n");

    status = client_supports_pnfs(session->client);
    if (status)
        goto out;
    status = fs_supports_layout(state->file.fh.superblock, PNFS_LAYOUTTYPE_FILE);
    if (status)
        goto out;

    /* under shared lock, check open state for cached layouts */
    AcquireSRWLockShared(&state->lock);
    status = open_state_layout_cached(state, &layout);
    ReleaseSRWLockShared(&state->lock);

    if (status) {
        /* under exclusive lock, find or create a layout for this file */
        AcquireSRWLockExclusive(&state->lock);

        status = open_state_layout_cached(state, &layout);
        if (status) {
            status = layout_state_find_or_create(layouts, &state->file.fh, &layout);
            if (status == PNFS_SUCCESS) {
                LONG open_count = InterlockedIncrement(&layout->open_count);
                state->layout = layout;

                dprintf(FLLVL, "pnfs_layout_state_open() caching layout %p "
                    "(%u opens)\n", state->layout, open_count);
            }
        }

        ReleaseSRWLockExclusive(&state->lock);

        if (status)
            goto out;
    }

    *layout_out = layout;
out:
    dprintf(FLLVL, "<-- pnfs_layout_state_open() returning %s\n",
        pnfs_error_string(status));
    return status;
}

/* expects caller to hold an exclusive lock on pnfs_layout_state */
enum pnfs_status pnfs_layout_state_prepare(
    IN pnfs_layout_state *state,
    IN nfs41_session *session,
    IN nfs41_path_fh *meta_file,
    IN stateid_arg *stateid,
    IN enum pnfs_iomode iomode,
    IN uint64_t offset,
    IN uint64_t length)
{
    unsigned char deviceid[PNFS_DEVICEID_SIZE];
    struct list_entry *entry;
    uint64_t missing;
    enum pnfs_status status;

    /* fail if the range intersects any pending recalls */
    list_for_each(entry, &state->recalls) {
        const pnfs_layout *recall = layout_entry(entry);
        if (offset <= recall->offset + recall->length
            && recall->offset <= offset + length) {
            status = PNFSERR_LAYOUT_RECALLED;
            goto out;
        }
    }

    /* if part of the given range is not covered by a layout,
     * attempt to fetch it with LAYOUTGET */
    status = layout_coverage_status(state, iomode, offset, length, &missing);
    if (status == PNFS_PENDING) {
        status = layout_fetch(state, session, meta_file, stateid,
            iomode, missing, offset + length - missing);

        /* return pending because layout_fetch() dropped the lock */
        if (status == PNFS_SUCCESS)
            status = PNFS_PENDING;
        goto out;
    }

    /* if any layouts in the range are missing device info,
     * fetch them with GETDEVICEINFO */
    status = device_status(state, offset, length, deviceid);
    if (status == PNFS_PENDING) {
        status = device_fetch(state, session, deviceid);

        /* return pending because device_fetch() dropped the lock */
        if (status == PNFS_SUCCESS)
            status = PNFS_PENDING;
        goto out;
    }
out:
    return status;
}

static enum pnfs_status layout_return_status(
    IN const pnfs_layout_state *state)
{
    /* return the layout if we have a stateid */
    return state->stateid.seqid ? PNFS_SUCCESS : PNFS_PENDING;
}

static enum pnfs_status file_layout_return(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN pnfs_layout_state *state)
{
    enum pnfs_status status;
    enum nfsstat4 nfsstat;

    dprintf(FLLVL, "--> file_layout_return()\n");

    /* under shared lock, determine whether we need to return the layout */
    AcquireSRWLockShared(&state->lock);
    status = layout_return_status(state);
    ReleaseSRWLockShared(&state->lock);

    if (status != PNFS_PENDING)
        goto out;

    /* under exclusive lock, return the layout and reset status flags */
    AcquireSRWLockExclusive(&state->lock);

    /* wait for any pending LAYOUTGETs/LAYOUTRETURNs */
    while (state->pending)
        SleepConditionVariableSRW(&state->cond, &state->lock, INFINITE, 0);
    state->pending = TRUE;

    status = layout_return_status(state);
    if (status == PNFS_PENDING) {
        pnfs_layoutreturn_res layoutreturn_res = { 0 };
        stateid4 stateid;
        memcpy(&stateid, &state->stateid, sizeof(stateid));
            
        /* drop the lock during the rpc call */
        ReleaseSRWLockExclusive(&state->lock);
        nfsstat = pnfs_rpc_layoutreturn(session, file, PNFS_LAYOUTTYPE_FILE, 
            PNFS_IOMODE_ANY, 0, NFS4_UINT64_MAX, &stateid, &layoutreturn_res);
        AcquireSRWLockExclusive(&state->lock);

        if (nfsstat) {
            eprintf("pnfs_rpc_layoutreturn() failed with %s\n", 
                nfs_error_string(nfsstat));
            status = PNFSERR_NO_LAYOUT;
        } else {
            status = PNFS_SUCCESS;

            /* update the layout range held by the client */
            layout_state_free_layouts(state);

            /* 12.5.3. Layout Stateid: Once a client has no more
             * layouts on a file, the layout stateid is no longer
             * valid and MUST NOT be used. */
            ZeroMemory(&state->stateid, sizeof(stateid4));
        }
    }

    state->pending = FALSE;
    WakeConditionVariable(&state->cond);
    ReleaseSRWLockExclusive(&state->lock);

out:
    dprintf(FLLVL, "<-- file_layout_return() returning %s\n",
        pnfs_error_string(status));
    return status;
}

void pnfs_layout_state_close(
    IN nfs41_session *session,
    IN nfs41_open_state *state,
    IN bool_t remove)
{
    pnfs_layout_state *layout;
    bool_t return_layout;
    enum pnfs_status status;

    AcquireSRWLockExclusive(&state->lock);
    layout = state->layout;
    state->layout = NULL;
    ReleaseSRWLockExclusive(&state->lock);

    if (layout) {
        LONG open_count = InterlockedDecrement(&layout->open_count);

        AcquireSRWLockShared(&layout->lock);
        /* only return on close if it's the last close */
        return_layout = layout->return_on_close && (open_count <= 0);
        ReleaseSRWLockShared(&layout->lock);

        if (return_layout) {
            status = file_layout_return(session, &state->file, layout);
            if (status)
                eprintf("file_layout_return() failed with %s\n",
                    pnfs_error_string(status));
        }
    }

    if (remove && session->client->layouts) {
        /* free the layout when the file is removed */
        layout_state_find_and_delete(session->client->layouts, &state->file.fh);
    }
}


/* pnfs_layout_recall */
struct layout_recall {
    pnfs_layout layout;
    bool_t changed;
};
#define recall_entry(pos) list_container(pos, struct layout_recall, layout.entry)

static bool_t layout_recall_compatible(
    IN const pnfs_layout *layout,
    IN const pnfs_layout *recall)
{
    return layout->type == recall->type
        && layout->offset <= (recall->offset + recall->length)
        && recall->offset <= (layout->offset + layout->length)
        && (recall->iomode == PNFS_IOMODE_ANY ||
            layout->iomode == recall->iomode);
}

static pnfs_file_layout* layout_allocate_copy(
    IN const pnfs_file_layout *existing)
{
    /* allocate a segment to cover the end of the range */
    pnfs_file_layout *layout = calloc(1, sizeof(pnfs_file_layout));
    if (layout == NULL)
        goto out;

    memcpy(layout, existing, sizeof(pnfs_file_layout));

    /* XXX: don't use the device from existing layout;
     * we need to get a reference for ourselves */
    layout->device = NULL;

    /* allocate a copy of the filehandle array */
    layout->filehandles.arr = calloc(layout->filehandles.count,
        sizeof(nfs41_path_fh));
    if (layout->filehandles.arr == NULL)
        goto out_free;

    memcpy(layout->filehandles.arr, existing->filehandles.arr,
        layout->filehandles.count * sizeof(nfs41_path_fh));
out:
    return layout;

out_free:
    file_layout_free(layout);
    layout = NULL;
    goto out;
}

static void layout_recall_range(
    IN pnfs_layout_state *state,
    IN const pnfs_layout *recall)
{
    struct list_entry *entry, *tmp;
    list_for_each_tmp(entry, tmp, &state->layouts) {
        pnfs_file_layout *layout = file_layout_entry(entry);
        const uint64_t layout_end = layout->layout.offset + layout->layout.length;

        if (!layout_recall_compatible(&layout->layout, recall))
            continue;
        
        if (recall->offset > layout->layout.offset) {
            /* segment starts before recall; shrink length */
            layout->layout.length = recall->offset - layout->layout.offset;

            if (layout_end > recall->offset + recall->length) {
                /* middle chunk of the segment is recalled;
                 * allocate a new segment to cover the end */
                pnfs_file_layout *remainder = layout_allocate_copy(layout);
                if (remainder == NULL) {
                    /* silently ignore allocation errors here. behave
                     * as if we 'forgot' this last segment */
                } else {
                    layout->layout.offset = recall->offset + recall->length;
                    layout->layout.length = layout_end - layout->layout.offset;
                    layout_ordered_insert(state, &remainder->layout);
                }
            }
        } else {
            /* segment starts after recall */
            if (layout_end <= recall->offset + recall->length) {
                /* entire segment is recalled */
                list_remove(&layout->layout.entry);
                file_layout_free(layout);
            } else {
                /* beginning of segment is recalled; shrink offset/length */
                layout->layout.offset = recall->offset + recall->length;
                layout->layout.length = layout_end - layout->layout.offset;
            }
        }
    }
}

static void layout_state_deferred_recalls(
    IN pnfs_layout_state *state)
{
    struct list_entry *entry, *tmp;
    list_for_each_tmp(entry, tmp, &state->recalls) {
        /* process each deferred layout recall */
        pnfs_layout *recall = layout_entry(entry);
        layout_recall_range(state, recall);

        /* remove/free the recall entry */
        list_remove(&recall->entry);
        free(recall);
    }
}

static void layout_recall_entry_init(
    OUT struct layout_recall *lrc,
    IN const struct cb_layoutrecall_args *recall)
{
    list_init(&lrc->layout.entry);
    if (recall->recall.type == PNFS_RETURN_FILE) {
        lrc->layout.offset = recall->recall.args.file.offset;
        lrc->layout.length = recall->recall.args.file.length;
    } else {
        lrc->layout.offset = 0;
        lrc->layout.length = NFS4_UINT64_MAX;
    }
    lrc->layout.iomode = recall->iomode;
    lrc->layout.type = PNFS_LAYOUTTYPE_FILE;
    lrc->changed = recall->changed;
}

static enum pnfs_status layout_recall_merge(
    IN struct list_entry *list,
    IN pnfs_layout *from)
{
    struct list_entry *entry, *tmp;
    enum pnfs_status status = PNFSERR_NO_LAYOUT;

    /* attempt to merge the new recall with each existing recall */
    list_for_each_tmp(entry, tmp, list) {
        pnfs_layout *to = layout_entry(entry);
        const uint64_t to_max = to->offset + to->length;
        const uint64_t from_max = from->offset + from->length;

        /* the ranges must meet or overlap */
        if (to_max < from->offset || from_max < to->offset)
            continue;

        /* the following fields must match: */
        if (to->iomode != from->iomode || to->type != from->type)
            continue;

        dprintf(FLLVL, "merging recalled range {%llu, %llu} with {%llu, %llu}\n",
            to->offset, to->length, from->offset, from->length);

        /* calculate the union of the two ranges */
        to->offset = min(to->offset, from->offset);
        to->length = max(to_max, from_max) - to->offset;

        /* on success, remove/free the new segment */
        list_remove(&from->entry);
        free(from);
        status = PNFS_SUCCESS;

        /* because the existing segment 'to' has grown, we may
         * be able to merge it with later segments */
        from = to;
    }
    return status;
}

static enum pnfs_status file_layout_recall(
    IN pnfs_layout_state *state,
    IN const struct cb_layoutrecall_args *recall)
{
    const stateid4 *stateid = &recall->recall.args.file.stateid;
    enum pnfs_status status = PNFS_SUCCESS;

    /* under an exclusive lock, flag the layout as recalled */
    AcquireSRWLockExclusive(&state->lock);

    if (state->stateid.seqid == 0) {
        /* return NOMATCHINGLAYOUT if it wasn't actually granted */
        status = PNFSERR_NO_LAYOUT;
        goto out;
    }
    
    if (recall->recall.type == PNFS_RETURN_FILE) {
        /* detect races between CB_LAYOUTRECALL and LAYOUTGET/LAYOUTRETURN */
        if (stateid->seqid > state->stateid.seqid + 1) {
            /* the server has processed an outstanding LAYOUTGET or
             * LAYOUTRETURN; we must return ERR_DELAY until we get the
             * response and update our view of the layout */
            status = PNFS_PENDING;
            goto out;
        }

        /* save the updated seqid */
        state->stateid.seqid = stateid->seqid;
    }

    if (state->io_count) {
        /* save an entry for this recall, and process it once io finishes */
        struct layout_recall *lrc = calloc(1, sizeof(struct layout_recall));
        if (lrc == NULL) {
            /* on failure to allocate, we'll have to respond
             * to the CB_LAYOUTRECALL with NFS4ERR_DELAY */
            status = PNFS_PENDING;
            goto out;
        }
        layout_recall_entry_init(lrc, recall);
        if (layout_recall_merge(&state->recalls, &lrc->layout) != PNFS_SUCCESS)
            list_add_tail(&state->recalls, &lrc->layout.entry);
    } else {
        /* if there is no pending io, process the recall immediately */
        struct layout_recall lrc = { 0 };
        layout_recall_entry_init(&lrc, recall);
        layout_recall_range(state, &lrc.layout);
    }
out:
    ReleaseSRWLockExclusive(&state->lock);
    return status;
}

static enum pnfs_status file_layout_recall_file(
    IN nfs41_client *client,
    IN const struct cb_layoutrecall_args *recall)
{
    struct list_entry *entry;
    enum pnfs_status status;

    dprintf(FLLVL, "--> file_layout_recall_file()\n");

    EnterCriticalSection(&client->layouts->lock);

    status = layout_entry_find(client->layouts, &recall->recall.args.file.fh, &entry);
    if (status == PNFS_SUCCESS)
        status = file_layout_recall(state_entry(entry), recall);

    LeaveCriticalSection(&client->layouts->lock);

    dprintf(FLLVL, "<-- file_layout_recall_file() returning %s\n",
        pnfs_error_string(status));
    return status;
}

static bool_t fsid_matches(
    IN const nfs41_fsid *lhs,
    IN const nfs41_fsid *rhs)
{
    return lhs->major == rhs->major && lhs->minor == rhs->minor;
}

static enum pnfs_status file_layout_recall_fsid(
    IN nfs41_client *client,
    IN const struct cb_layoutrecall_args *recall)
{
    struct list_entry *entry;
    pnfs_layout_state *state;
    nfs41_fh *fh;
    enum pnfs_status status = PNFSERR_NO_LAYOUT;

    dprintf(FLLVL, "--> file_layout_recall_fsid(%llu, %llu)\n",
        recall->recall.args.fsid.major, recall->recall.args.fsid.minor);

    EnterCriticalSection(&client->layouts->lock);

    list_for_each(entry, &client->layouts->head) {
        state = state_entry(entry);
        /* no locks needed to read layout.meta_fh or superblock.fsid,
         * because they are only written once on creation */
        fh = &state->meta_fh;
        if (fsid_matches(&recall->recall.args.fsid, &fh->superblock->fsid))
            status = file_layout_recall(state, recall);
    }

    LeaveCriticalSection(&client->layouts->lock);

    /* bulk recalls require invalidation of cached device info */
    pnfs_file_device_list_invalidate(client->devices);

    dprintf(FLLVL, "<-- file_layout_recall_fsid() returning %s\n",
        pnfs_error_string(status));
    return status;
}

static enum pnfs_status file_layout_recall_all(
    IN nfs41_client *client,
    IN const struct cb_layoutrecall_args *recall)
{
    struct list_entry *entry;
    enum pnfs_status status = PNFSERR_NO_LAYOUT;

    dprintf(FLLVL, "--> file_layout_recall_all()\n");

    EnterCriticalSection(&client->layouts->lock);

    list_for_each(entry, &client->layouts->head)
        status = file_layout_recall(state_entry(entry), recall);

    LeaveCriticalSection(&client->layouts->lock);

    /* bulk recalls require invalidation of cached device info */
    pnfs_file_device_list_invalidate(client->devices);

    dprintf(FLLVL, "<-- file_layout_recall_all() returning %s\n",
        pnfs_error_string(status));
    return status;
}

enum pnfs_status pnfs_file_layout_recall(
    IN nfs41_client *client,
    IN const struct cb_layoutrecall_args *recall)
{
    enum pnfs_status status = PNFS_SUCCESS;

    dprintf(FLLVL, "--> pnfs_file_layout_recall(%u, %s, %u)\n",
        recall->recall.type, pnfs_iomode_string(recall->iomode),
        recall->changed);

    if (recall->type != PNFS_LAYOUTTYPE_FILE) {
        dprintf(FLLVL, "invalid layout type %u (%s)!\n",
            recall->type, pnfs_layout_type_string(recall->type));
        status = PNFSERR_NOT_SUPPORTED;
        goto out;
    }

    switch (recall->recall.type) {
    case PNFS_RETURN_FILE:
        status = file_layout_recall_file(client, recall);
        break;
    case PNFS_RETURN_FSID:
        status = file_layout_recall_fsid(client, recall);
        break;
    case PNFS_RETURN_ALL:
        status = file_layout_recall_all(client, recall);
        break;

    default:
        dprintf(FLLVL, "invalid return type %u!\n", recall->recall);
        status = PNFSERR_NOT_SUPPORTED;
        goto out;
    }
out:
    dprintf(FLLVL, "<-- pnfs_file_layout_recall() returning %s\n",
        pnfs_error_string(status));
    return status;
}

/* expects caller to hold a shared lock on pnfs_layout_state */
enum pnfs_status pnfs_layout_recall_status(
    IN const pnfs_layout_state *state,
    IN const pnfs_layout *layout)
{
    struct list_entry *entry;
    enum pnfs_status status = PNFS_SUCCESS;

    /* search for a pending recall that intersects with the given segment */
    list_for_each(entry, &state->recalls) {
        const struct layout_recall *recall = recall_entry(entry);
        if (!layout_recall_compatible(layout, &recall->layout))
            continue;

        if (recall->changed)
            status = PNFSERR_LAYOUT_CHANGED;
        else
            status = PNFSERR_LAYOUT_RECALLED;
        break;
    }
    return status;
}

void pnfs_layout_recall_fenced(
    IN pnfs_layout_state *state,
    IN const pnfs_layout *layout)
{
    struct layout_recall *lrc = calloc(1, sizeof(struct layout_recall));
    if (lrc == NULL)
        return;

    AcquireSRWLockExclusive(&state->lock);

    list_init(&lrc->layout.entry);
    lrc->layout.offset = layout->offset;
    lrc->layout.length = layout->length;
    lrc->layout.iomode = layout->iomode;
    lrc->layout.type = layout->type;
    lrc->changed = TRUE;

    if (layout_recall_merge(&state->recalls, &lrc->layout) != PNFS_SUCCESS)
        list_add_tail(&state->recalls, &lrc->layout.entry);

    ReleaseSRWLockExclusive(&state->lock);
}

/* expects caller to hold an exclusive lock on pnfs_layout_state */
void pnfs_layout_io_start(
    IN pnfs_layout_state *state)
{
    /* take a reference on the layout, so that it won't be recalled
     * until all io is finished */
    state->io_count++;
    dprintf(FLLVL, "pnfs_layout_io_start(): count -> %u\n",
        state->io_count);
}

void pnfs_layout_io_finished(
    IN pnfs_layout_state *state)
{
    AcquireSRWLockExclusive(&state->lock);

    /* return the reference to signify that an io request is finished */
    state->io_count--;
    dprintf(FLLVL, "pnfs_layout_io_finished() count -> %u\n",
        state->io_count);

    if (state->io_count > 0) /* more io pending */
        goto out_unlock;

    /* once all io is finished, process any layout recalls */
    layout_state_deferred_recalls(state);

    /* finish any segment merging that was delayed during io */
    if (!list_empty(&state->layouts))
        layout_state_merge(state, file_layout_entry(state->layouts.next));

out_unlock:
    ReleaseSRWLockExclusive(&state->lock);
}
