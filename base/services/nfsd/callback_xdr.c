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

#include "nfs41_callback.h"
#include "nfs41_ops.h"
#include "util.h"
#include "daemon_debug.h"


#define CBXLVL 2 /* dprintf level for callback xdr logging */
#ifdef __REACTOS__
#define CBX_ERR(msg) dprintf((CBXLVL), "%s: failed at %s\n", __FUNCTION__, msg)
#else
#define CBX_ERR(msg) dprintf((CBXLVL), __FUNCTION__ ": failed at " msg "\n")
#endif

/* common types */
bool_t xdr_bitmap4(XDR *xdr, bitmap4 *bitmap);
bool_t xdr_fattr4(XDR *xdr, fattr4 *fattr);

static bool_t common_stateid(XDR *xdr, stateid4 *stateid)
{
    return xdr_u_int32_t(xdr, &stateid->seqid)
        && xdr_opaque(xdr, (char*)stateid->other, NFS4_STATEID_OTHER);
}

static bool_t common_fh(XDR *xdr, nfs41_fh *fh)
{
    return xdr_u_int32_t(xdr, &fh->len)
        && fh->len <= NFS4_FHSIZE
        && xdr_opaque(xdr, (char*)fh->fh, fh->len);
}

static bool_t common_fsid(XDR *xdr, nfs41_fsid *fsid)
{
    return xdr_u_int64_t(xdr, &fsid->major)
        && xdr_u_int64_t(xdr, &fsid->minor);
}

static bool_t common_notify4(XDR *xdr, struct notify4 *notify)
{
    return xdr_bitmap4(xdr, &notify->mask)
        && xdr_bytes(xdr, &notify->list, &notify->len, NFS4_OPAQUE_LIMIT);
}

/* OP_CB_LAYOUTRECALL */
static bool_t op_cb_layoutrecall_file(XDR *xdr, struct cb_recall_file *args)
{
    bool_t result;

    result = common_fh(xdr, &args->fh);
    if (!result) { CBX_ERR("layoutrecall_file.fh"); goto out; }

    result = xdr_u_int64_t(xdr, &args->offset);
    if (!result) { CBX_ERR("layoutrecall_file.offset"); goto out; }

    result = xdr_u_int64_t(xdr, &args->length);
    if (!result) { CBX_ERR("layoutrecall_file.length"); goto out; }

    result = common_stateid(xdr, &args->stateid);
    if (!result) { CBX_ERR("layoutrecall_file.stateid"); goto out; }
out:
    return result;
}

static bool_t op_cb_layoutrecall_fsid(XDR *xdr, union cb_recall_file_args *args)
{
    bool_t result;

    result = common_fsid(xdr, &args->fsid);
    if (!result) { CBX_ERR("layoutrecall_fsid.fsid"); goto out; }
out:
    return result;
}

static const struct xdr_discrim cb_layoutrecall_discrim[] = {
    { PNFS_RETURN_FILE,     (xdrproc_t)op_cb_layoutrecall_file },
    { PNFS_RETURN_FSID,     (xdrproc_t)op_cb_layoutrecall_fsid },
    { PNFS_RETURN_ALL,      (xdrproc_t)xdr_void },
    { 0,                    NULL_xdrproc_t }
};

static bool_t op_cb_layoutrecall_args(XDR *xdr, struct cb_layoutrecall_args *args)
{
    bool_t result;

    result = xdr_enum(xdr, (enum_t*)&args->type);
    if (!result) { CBX_ERR("layoutrecall_args.type"); goto out; }

    result = xdr_enum(xdr, (enum_t*)&args->iomode);
    if (!result) { CBX_ERR("layoutrecall_args.iomode"); goto out; }

    result = xdr_bool(xdr, &args->changed);
    if (!result) { CBX_ERR("layoutrecall_args.changed"); goto out; }

    result = xdr_union(xdr, (enum_t*)&args->recall.type,
        (char*)&args->recall.args, cb_layoutrecall_discrim, NULL_xdrproc_t);
    if (!result) { CBX_ERR("layoutrecall_args.recall"); goto out; }
out:
    return result;
}

static bool_t op_cb_layoutrecall_res(XDR *xdr, struct cb_layoutrecall_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("layoutrecall_res.status"); goto out; }
out:
    return result;
}


/* OP_CB_RECALL_SLOT */
static bool_t op_cb_recall_slot_args(XDR *xdr, struct cb_recall_slot_args *res)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("recall_slot.target_highest_slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_recall_slot_res(XDR *xdr, struct cb_recall_slot_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("recall_slot.status"); goto out; }
out:
    return result;
}


/* OP_CB_SEQUENCE */
static bool_t op_cb_sequence_ref(XDR *xdr, struct cb_sequence_ref *args)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &args->sequenceid);
    if (!result) { CBX_ERR("sequence_ref.sequenceid"); goto out; }

    result = xdr_u_int32_t(xdr, &args->slotid);
    if (!result) { CBX_ERR("sequence_ref.slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_sequence_ref_list(XDR *xdr, struct cb_sequence_ref_list *args)
{
    bool_t result;

    result = xdr_opaque(xdr, args->sessionid, NFS4_SESSIONID_SIZE);
    if (!result) { CBX_ERR("sequence_ref_list.sessionid"); goto out; }

    result = xdr_array(xdr, (char**)&args->calls, &args->call_count,
        64, sizeof(struct cb_sequence_ref), (xdrproc_t)op_cb_sequence_ref);
    if (!result) { CBX_ERR("sequence_ref_list.calls"); goto out; }
out:
    return result;
}

static bool_t op_cb_sequence_args(XDR *xdr, struct cb_sequence_args *args)
{
    bool_t result;

    result = xdr_opaque(xdr, args->sessionid, NFS4_SESSIONID_SIZE);
    if (!result) { CBX_ERR("sequence_args.sessionid"); goto out; }

    result = xdr_u_int32_t(xdr, &args->sequenceid);
    if (!result) { CBX_ERR("sequence_args.sequenceid"); goto out; }

    result = xdr_u_int32_t(xdr, &args->slotid);
    if (!result) { CBX_ERR("sequence_args.slotid"); goto out; }

    result = xdr_u_int32_t(xdr, &args->highest_slotid);
    if (!result) { CBX_ERR("sequence_args.highest_slotid"); goto out; }

    result = xdr_bool(xdr, &args->cachethis);
    if (!result) { CBX_ERR("sequence_args.cachethis"); goto out; }

    result = xdr_array(xdr, (char**)&args->ref_lists,
        &args->ref_list_count, 64, sizeof(struct cb_sequence_ref_list),
        (xdrproc_t)op_cb_sequence_ref_list);
    if (!result) { CBX_ERR("sequence_args.ref_lists"); goto out; }
out:
    return result;
}

static bool_t op_cb_sequence_res_ok(XDR *xdr, struct cb_sequence_res_ok *res)
{
    bool_t result;

    result = xdr_opaque(xdr, res->sessionid, NFS4_SESSIONID_SIZE);
    if (!result) { CBX_ERR("sequence_res.sessionid"); goto out; }

    result = xdr_u_int32_t(xdr, &res->sequenceid);
    if (!result) { CBX_ERR("sequence_res.sequenceid"); goto out; }

    result = xdr_u_int32_t(xdr, &res->slotid);
    if (!result) { CBX_ERR("sequence_res.slotid"); goto out; }

    result = xdr_u_int32_t(xdr, &res->highest_slotid);
    if (!result) { CBX_ERR("sequence_res.highest_slotid"); goto out; }

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("sequence_res.target_highest_slotid"); goto out; }
out:
    return result;
}

static const struct xdr_discrim cb_sequence_res_discrim[] = {
    { NFS4_OK,              (xdrproc_t)op_cb_sequence_res_ok },
    { 0,                    NULL_xdrproc_t }
};

static bool_t op_cb_sequence_res(XDR *xdr, struct cb_sequence_res *res)
{
    bool_t result;

    result = xdr_union(xdr, &res->status, (char*)&res->ok,
        cb_sequence_res_discrim, (xdrproc_t)xdr_void);
    if (!result) { CBX_ERR("seq:argop.args"); goto out; }
out:
    return result;
}

/* OP_CB_GETATTR */
static bool_t op_cb_getattr_args(XDR *xdr, struct cb_getattr_args *args)
{
    bool_t result;

    result = common_fh(xdr, &args->fh);
    if (!result) { CBX_ERR("getattr.fh"); goto out; }

    result = xdr_bitmap4(xdr, &args->attr_request);
    if (!result) { CBX_ERR("getattr.attr_request"); goto out; }
out:
    return result;
}

static bool_t info_to_fattr4(nfs41_file_info *info, fattr4 *fattr)
{
    XDR fattr_xdr;
    bool_t result = TRUE;

    /* encode nfs41_file_info into fattr4 */
    xdrmem_create(&fattr_xdr, (char*)fattr->attr_vals,
        NFS4_OPAQUE_LIMIT, XDR_ENCODE);
    
    /* The only attributes that the server can reliably
     * query via CB_GETATTR are size and change. */
    if (bitmap_isset(&info->attrmask, 0, FATTR4_WORD0_CHANGE)) {
        result = xdr_u_hyper(&fattr_xdr, &info->change);
        if (!result) { CBX_ERR("getattr.info.change"); goto out; }
        bitmap_set(&fattr->attrmask, 0, FATTR4_WORD0_CHANGE);
    }
    if (bitmap_isset(&info->attrmask, 0, FATTR4_WORD0_SIZE)) {
        result = xdr_u_hyper(&fattr_xdr, &info->size);
        if (!result) { CBX_ERR("getattr.info.size"); goto out; }
        bitmap_set(&fattr->attrmask, 0, FATTR4_WORD0_SIZE);
    }
    fattr->attr_vals_len = xdr_getpos(&fattr_xdr);
out:
    return result;
}

static bool_t op_cb_getattr_res(XDR *xdr, struct cb_getattr_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("getattr.status"); goto out; }

    if (res->status == NFS4_OK) {
        fattr4 fattr = { 0 };

        result = info_to_fattr4(&res->info, &fattr);
        if (!result) { goto out; }

        result = xdr_fattr4(xdr, &fattr);
        if (!result) { CBX_ERR("getattr.obj_attributes"); goto out; }
    }
out:
    return result;
}

/* OP_CB_RECALL */
static bool_t op_cb_recall_args(XDR *xdr, struct cb_recall_args *args)
{
    bool_t result;

    result = common_stateid(xdr, &args->stateid);
    if (!result) { CBX_ERR("recall.stateid"); goto out; }

    result = xdr_bool(xdr, &args->truncate);
    if (!result) { CBX_ERR("recall.truncate"); goto out; }

    result = common_fh(xdr, &args->fh);
    if (!result) { CBX_ERR("recall.fh"); goto out; }
out:
    return result;
}

static bool_t op_cb_recall_res(XDR *xdr, struct cb_recall_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("recall.status"); goto out; }
out:
    return result;
}

/* OP_CB_NOTIFY */
static bool_t op_cb_notify_args(XDR *xdr, struct cb_notify_args *res)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("notify.target_highest_slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_notify_res(XDR *xdr, struct cb_notify_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("notify.status"); goto out; }
out:
    return result;
}

/* OP_CB_PUSH_DELEG */
static bool_t op_cb_push_deleg_args(XDR *xdr, struct cb_push_deleg_args *res)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("push_deleg.target_highest_slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_push_deleg_res(XDR *xdr, struct cb_push_deleg_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("push_deleg.status"); goto out; }
out:
    return result;
}

/* OP_CB_RECALL_ANY */
static bool_t op_cb_recall_any_args(XDR *xdr, struct cb_recall_any_args *res)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("recall_any.target_highest_slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_recall_any_res(XDR *xdr, struct cb_recall_any_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("recall_any.status"); goto out; }
out:
    return result;
}

/* OP_CB_RECALLABLE_OBJ_AVAIL */
static bool_t op_cb_recallable_obj_avail_args(XDR *xdr, struct cb_recallable_obj_avail_args *res)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("recallable_obj_avail.target_highest_slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_recallable_obj_avail_res(XDR *xdr, struct cb_recallable_obj_avail_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("recallable_obj_avail.status"); goto out; }
out:
    return result;
}

/* OP_CB_WANTS_CANCELLED */
static bool_t op_cb_wants_cancelled_args(XDR *xdr, struct cb_wants_cancelled_args *res)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("wants_cancelled.target_highest_slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_wants_cancelled_res(XDR *xdr, struct cb_wants_cancelled_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("wants_cancelled.status"); goto out; }
out:
    return result;
}

/* OP_CB_NOTIFY_LOCK */
static bool_t op_cb_notify_lock_args(XDR *xdr, struct cb_notify_lock_args *res)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, &res->target_highest_slotid);
    if (!result) { CBX_ERR("notify_lock.target_highest_slotid"); goto out; }
out:
    return result;
}

static bool_t op_cb_notify_lock_res(XDR *xdr, struct cb_notify_lock_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("notify_lock.status"); goto out; }
out:
    return result;
}

/* OP_CB_NOTIFY_DEVICEID */
static bool_t cb_notify_deviceid_change(XDR *xdr, struct notify_deviceid4 *change)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, (uint32_t*)&change->layouttype);
    if (!result) { CBX_ERR("notify_deviceid.change.layouttype"); goto out; }

    result = xdr_opaque(xdr, (char*)change->deviceid, PNFS_DEVICEID_SIZE);
    if (!result) { CBX_ERR("notify_deviceid.change.deviceid"); goto out; }

    result = xdr_bool(xdr, &change->immediate);
    if (!result) { CBX_ERR("notify_deviceid.change.immediate"); goto out; }
out:
    return result;
}

static bool_t cb_notify_deviceid_delete(XDR *xdr, struct notify_deviceid4 *change)
{
    bool_t result;

    result = xdr_u_int32_t(xdr, (uint32_t*)&change->layouttype);
    if (!result) { CBX_ERR("notify_deviceid.delete.layouttype"); goto out; }

    result = xdr_opaque(xdr, (char*)change->deviceid, PNFS_DEVICEID_SIZE);
    if (!result) { CBX_ERR("notify_deviceid.delete.deviceid"); goto out; }
out:
    return result;
}

static bool_t op_cb_notify_deviceid_args(XDR *xdr, struct cb_notify_deviceid_args *args)
{
    XDR notify_xdr;
    uint32_t i, j, c;
    bool_t result;

    /* decode the generic notify4 list */
    result = xdr_array(xdr, (char**)&args->notify_list,
        &args->notify_count, CB_COMPOUND_MAX_OPERATIONS,
        sizeof(struct notify4), (xdrproc_t)common_notify4);
    if (!result) { CBX_ERR("notify_deviceid.notify_list"); goto out; }

    switch (xdr->x_op) {
    case XDR_FREE:
        free(args->change_list);
    case XDR_ENCODE:
        return TRUE;
    }

    /* count the number of device changes */
    args->change_count = 0;
    for (i = 0; i < args->notify_count; i++)
        args->change_count += args->notify_list[i].mask.count;

    args->change_list = calloc(args->change_count, sizeof(struct notify_deviceid4));
    if (args->change_list == NULL)
        return FALSE;

    c = 0;
    for (i = 0; i < args->notify_count; i++) {
        struct notify4 *notify = &args->notify_list[i];

        /* decode the device notifications out of the opaque buffer */
        xdrmem_create(&notify_xdr, notify->list, notify->len, XDR_DECODE);

        for (j = 0; j < notify->mask.count; j++) {
            struct notify_deviceid4 *change = &args->change_list[c++];
            change->type = notify->mask.arr[j];

            switch (change->type) {
            case NOTIFY_DEVICEID4_CHANGE:
                result = cb_notify_deviceid_change(&notify_xdr, change);
                if (!result) { CBX_ERR("notify_deviceid.change"); goto out; }
                break;
            case NOTIFY_DEVICEID4_DELETE:
                result = cb_notify_deviceid_delete(&notify_xdr, change);
                if (!result) { CBX_ERR("notify_deviceid.delete"); goto out; }
                break;
            }
        }
    }
out:
    return result;
}

static bool_t op_cb_notify_deviceid_res(XDR *xdr, struct cb_notify_deviceid_res *res)
{
    bool_t result;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("notify_deviceid.status"); goto out; }
out:
    return result;
}

/* CB_COMPOUND */
static bool_t cb_compound_tag(XDR *xdr, struct cb_compound_tag *args)
{
    return xdr_u_int32_t(xdr, &args->len)
        && args->len <= CB_COMPOUND_MAX_TAG
        && xdr_opaque(xdr, args->str, args->len);
}

static const struct xdr_discrim cb_argop_discrim[] = {
    { OP_CB_LAYOUTRECALL,   (xdrproc_t)op_cb_layoutrecall_args },
    { OP_CB_RECALL_SLOT,    (xdrproc_t)op_cb_recall_slot_args },
    { OP_CB_SEQUENCE,       (xdrproc_t)op_cb_sequence_args },
    { OP_CB_GETATTR,        (xdrproc_t)op_cb_getattr_args },
    { OP_CB_RECALL,         (xdrproc_t)op_cb_recall_args },
    { OP_CB_NOTIFY,         (xdrproc_t)op_cb_notify_args },
    { OP_CB_PUSH_DELEG,     (xdrproc_t)op_cb_push_deleg_args },
    { OP_CB_RECALL_ANY,     (xdrproc_t)op_cb_recall_any_args },
    { OP_CB_RECALLABLE_OBJ_AVAIL, (xdrproc_t)op_cb_recallable_obj_avail_args },
    { OP_CB_WANTS_CANCELLED, (xdrproc_t)op_cb_wants_cancelled_args },
    { OP_CB_NOTIFY_LOCK,     (xdrproc_t)op_cb_notify_lock_args },
    { OP_CB_NOTIFY_DEVICEID, (xdrproc_t)op_cb_notify_deviceid_args },
    { OP_CB_ILLEGAL,         NULL_xdrproc_t },
};

static bool_t cb_compound_argop(XDR *xdr, struct cb_argop *args)
{
    bool_t result;

    result = xdr_union(xdr, &args->opnum, (char*)&args->args,
        cb_argop_discrim, NULL_xdrproc_t);
    if (!result) { CBX_ERR("cmb:argop.args"); goto out; }
out:
    return result;
}

bool_t proc_cb_compound_args(XDR *xdr, struct cb_compound_args *args)
{
    bool_t result;

    result = cb_compound_tag(xdr, &args->tag);
    if (!result) { CBX_ERR("compound.tag"); goto out; }

    result = xdr_u_int32_t(xdr, &args->minorversion);
    if (!result) { CBX_ERR("compound.minorversion"); goto out; }

    /* "superfluous in NFSv4.1 and MUST be ignored by the client" */
    result = xdr_u_int32_t(xdr, &args->callback_ident);
    if (!result) { CBX_ERR("compound.callback_ident"); goto out; }

    result = xdr_array(xdr, (char**)&args->argarray,
        &args->argarray_count, CB_COMPOUND_MAX_OPERATIONS,
        sizeof(struct cb_argop), (xdrproc_t)cb_compound_argop);
    if (!result) { CBX_ERR("compound.argarray"); goto out; }
out:
    return result;
}

static const struct xdr_discrim cb_resop_discrim[] = {
    { OP_CB_LAYOUTRECALL,   (xdrproc_t)op_cb_layoutrecall_res },
    { OP_CB_RECALL_SLOT,    (xdrproc_t)op_cb_recall_slot_res },
    { OP_CB_SEQUENCE,       (xdrproc_t)op_cb_sequence_res },
    { OP_CB_GETATTR,        (xdrproc_t)op_cb_getattr_res },
    { OP_CB_RECALL,         (xdrproc_t)op_cb_recall_res },
    { OP_CB_NOTIFY,         (xdrproc_t)op_cb_notify_res },
    { OP_CB_PUSH_DELEG,     (xdrproc_t)op_cb_push_deleg_res },
    { OP_CB_RECALL_ANY,     (xdrproc_t)op_cb_recall_any_res },
    { OP_CB_RECALLABLE_OBJ_AVAIL, (xdrproc_t)op_cb_recallable_obj_avail_res },
    { OP_CB_WANTS_CANCELLED, (xdrproc_t)op_cb_wants_cancelled_res },
    { OP_CB_NOTIFY_LOCK,     (xdrproc_t)op_cb_notify_lock_res },
    { OP_CB_NOTIFY_DEVICEID, (xdrproc_t)op_cb_notify_deviceid_res },
    { OP_CB_ILLEGAL,         NULL_xdrproc_t },
};

static bool_t cb_compound_resop(XDR *xdr, struct cb_resop *res)
{
    /* save xdr encode/decode status to see which operation failed */
    res->xdr_ok = xdr_union(xdr, &res->opnum, (char*)&res->res,
        cb_resop_discrim, NULL_xdrproc_t);
    if (!res->xdr_ok) { CBX_ERR("resop.res"); goto out; }
out:
    return res->xdr_ok;
}

bool_t proc_cb_compound_res(XDR *xdr, struct cb_compound_res *res)
{
    bool_t result;

    if (res == NULL)
        return TRUE;

    result = xdr_enum(xdr, &res->status);
    if (!result) { CBX_ERR("compound_res.status"); goto out; }

    result = cb_compound_tag(xdr, &res->tag);
    if (!result) { CBX_ERR("compound_res.tag"); goto out; }

    result = xdr_array(xdr, (char**)&res->resarray,
        &res->resarray_count, CB_COMPOUND_MAX_OPERATIONS,
        sizeof(struct cb_resop), (xdrproc_t)cb_compound_resop);
    if (!result) { CBX_ERR("compound_res.resarray"); goto out; }
out:
    if (xdr->x_op == XDR_FREE)
        free(res);
    return result;
}
