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

#ifndef __NFS41_CALLBACK_H__
#define __NFS41_CALLBACK_H__

#include "wintirpc.h"
#include "rpc/rpc.h"
#include "nfs41_types.h"


enum nfs41_callback_proc {
    CB_NULL                 = 0,
    CB_COMPOUND             = 1,
};

enum nfs41_callback_op {
    OP_CB_GETATTR           = 3,
    OP_CB_RECALL            = 4,
    OP_CB_LAYOUTRECALL      = 5,
    OP_CB_NOTIFY            = 6,
    OP_CB_PUSH_DELEG        = 7,
    OP_CB_RECALL_ANY        = 8,
    OP_CB_RECALLABLE_OBJ_AVAIL = 9,
    OP_CB_RECALL_SLOT       = 10,
    OP_CB_SEQUENCE          = 11,
    OP_CB_WANTS_CANCELLED   = 12,
    OP_CB_NOTIFY_LOCK       = 13,
    OP_CB_NOTIFY_DEVICEID   = 14,
    OP_CB_ILLEGAL           = 10044
};

int nfs41_handle_callback(void *, void *, void *);

/* OP_CB_LAYOUTRECALL */
struct cb_recall_file {
    nfs41_fh                fh;
    uint64_t                offset;
    uint64_t                length;
    stateid4                stateid;
};
union cb_recall_file_args {
    struct cb_recall_file   file;
    nfs41_fsid              fsid;
};
struct cb_recall {
#ifdef __REACTOS__
    uint32_t type;
#else
    enum pnfs_return_type   type;
#endif
    union cb_recall_file_args args;
};
struct cb_layoutrecall_args {
#ifdef __REACTOS__
    uint32_t type;
    uint32_t iomode;
#else
    enum pnfs_return_type   type;
    enum pnfs_iomode        iomode;
#endif
    bool_t                  changed;
    struct cb_recall        recall;
};

struct cb_layoutrecall_res {
    enum_t                  status;
};

/* OP_CB_RECALL_SLOT */
struct cb_recall_slot_args {
    uint32_t                target_highest_slotid;
};

struct cb_recall_slot_res {
    enum_t                  status;
};

/* OP_CB_SEQUENCE */
struct cb_sequence_ref {
    uint32_t                sequenceid;
    uint32_t                slotid;
};
struct cb_sequence_ref_list {
    char                    sessionid[NFS4_SESSIONID_SIZE];
    struct cb_sequence_ref  *calls;
    uint32_t                call_count;
};
struct cb_sequence_args {
    char                    sessionid[NFS4_SESSIONID_SIZE];
    uint32_t                sequenceid;
    uint32_t                slotid;
    uint32_t                highest_slotid;
    bool_t                  cachethis;
    struct cb_sequence_ref_list *ref_lists;
    uint32_t                ref_list_count;
};

struct cb_sequence_res_ok {
    char                    sessionid[NFS4_SESSIONID_SIZE];
    uint32_t                sequenceid;
    uint32_t                slotid;
    uint32_t                highest_slotid;
    uint32_t                target_highest_slotid;
};
struct cb_sequence_res {
    enum_t                  status;
    struct cb_sequence_res_ok ok;
};

/* OP_CB_GETATTR */
struct cb_getattr_args {
    nfs41_fh                fh;
    bitmap4                 attr_request;
};

struct cb_getattr_res {
    enum_t                  status;
    nfs41_file_info         info;
};

/* OP_CB_RECALL */
struct cb_recall_args {
    stateid4                stateid;
    bool_t                  truncate;
    nfs41_fh                fh;
};

struct cb_recall_res {
    enum_t                  status;
};

/* OP_CB_NOTIFY */
struct cb_notify_args {
    uint32_t                target_highest_slotid;
};

struct cb_notify_res {
    enum_t                  status;
};

/* OP_CB_PUSH_DELEG */
struct cb_push_deleg_args {
    uint32_t                target_highest_slotid;
};

struct cb_push_deleg_res {
    enum_t                  status;
};

/* OP_CB_RECALL_ANY */
struct cb_recall_any_args {
    uint32_t                target_highest_slotid;
};

struct cb_recall_any_res {
    enum_t                  status;
};

/* OP_CB_RECALLABLE_OBJ_AVAIL */
struct cb_recallable_obj_avail_args {
    uint32_t                target_highest_slotid;
};

struct cb_recallable_obj_avail_res {
    enum_t                  status;
};

/* OP_CB_WANTS_CANCELLED */
struct cb_wants_cancelled_args {
    uint32_t                target_highest_slotid;
};

struct cb_wants_cancelled_res {
    enum_t                  status;
};

/* OP_CB_NOTIFY_LOCK */
struct cb_notify_lock_args {
    uint32_t                target_highest_slotid;
};

struct cb_notify_lock_res {
    enum_t                  status;
};

/* OP_CB_NOTIFY_DEVICEID */
enum notify_deviceid_type4 {
    NOTIFY_DEVICEID4_CHANGE = 1,
    NOTIFY_DEVICEID4_DELETE = 2
};
struct notify_deviceid4 {
    unsigned char           deviceid[16];
    enum notify_deviceid_type4 type;
#ifdef __REACTOS__
    uint32_t layouttype;
#else
    enum pnfs_layout_type   layouttype;
#endif
    bool_t                  immediate;
};
struct notify4 {
    bitmap4                 mask;
    char                    *list;
    uint32_t                len;
};
struct cb_notify_deviceid_args {
    struct notify4          *notify_list;
    uint32_t                notify_count;
    struct notify_deviceid4 *change_list;
    uint32_t                change_count;
};

struct cb_notify_deviceid_res {
    enum_t                  status;
};

/* CB_COMPOUND */
#define CB_COMPOUND_MAX_TAG         64
#define CB_COMPOUND_MAX_OPERATIONS  16

union cb_op_args {
    struct cb_layoutrecall_args layoutrecall;
    struct cb_recall_slot_args recall_slot;
    struct cb_sequence_args sequence;
    struct cb_getattr_args  getattr;
    struct cb_recall_args   recall;
    struct cb_notify_deviceid_args notify_deviceid;
};
struct cb_argop {
    enum_t                  opnum;
    union cb_op_args        args;
};
struct cb_compound_tag {
    char                    str[CB_COMPOUND_MAX_TAG];
    uint32_t                len;
};
struct cb_compound_args {
    struct cb_compound_tag  tag;
    uint32_t                minorversion;
    uint32_t                callback_ident; /* client MUST ignore */
    struct cb_argop         *argarray;
    uint32_t                argarray_count; /* <= CB_COMPOUND_MAX_OPERATIONS */
};

union cb_op_res {
    enum_t                  status; /* all results start with status */ 
    struct cb_layoutrecall_res layoutrecall;
    struct cb_recall_slot_res recall_slot;
    struct cb_sequence_res  sequence;
    struct cb_getattr_res   getattr;
    struct cb_recall_res    recall;
    struct cb_notify_deviceid_res notify_deviceid;
};
struct cb_resop {
    enum_t                  opnum;
    union cb_op_res         res;
    bool_t                  xdr_ok;
};
struct cb_compound_res {
    enum_t                  status;
    struct cb_compound_tag  tag;
    struct cb_resop         *resarray;
    uint32_t                resarray_count; /* <= CB_COMPOUND_MAX_OPERATIONS */
};


/* callback_xdr.c */
bool_t proc_cb_compound_args(XDR *xdr, struct cb_compound_args *args);
bool_t proc_cb_compound_res(XDR *xdr, struct cb_compound_res *res);

/* callback_server.c */
struct __nfs41_session;
void nfs41_callback_session_init(
    IN struct __nfs41_session *session);

#endif /* !__NFS41_CALLBACK_H__ */
