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

#ifndef __NFS41_DAEMON_UPCALL_H__
#define __NFS41_DAEMON_UPCALL_H__

#include "nfs41_ops.h"
#include "from_kernel.h"

#define NFSD_VERSION_MISMATCH 116

/* structures for upcall arguments */
typedef struct __mount_upcall_args {
    const char *hostname;
    const char *path;
    DWORD       sec_flavor;
    DWORD       rsize;
    DWORD       wsize;
    DWORD       lease_time;
    FILE_FS_ATTRIBUTE_INFORMATION FsAttrs;
} mount_upcall_args;

typedef struct __open_upcall_args {
    nfs41_abs_path symlink;
    FILE_BASIC_INFO basic_info;
    FILE_STANDARD_INFO std_info;
    const char *path;
    ULONG access_mask;
    ULONG access_mode; 
    ULONG file_attrs;
    ULONG disposition;
    ULONG create_opts;
    LONG open_owner_id;
    DWORD mode;
    ULONGLONG changeattr;
    HANDLE srv_open;
    DWORD deleg_type;
    PFILE_FULL_EA_INFORMATION ea;
    BOOLEAN created;
    BOOLEAN symlink_embedded;
} open_upcall_args;

typedef struct __close_upcall_args {
    HANDLE srv_open;
    const char *path;
    BOOLEAN remove;
    BOOLEAN renamed;
} close_upcall_args;

typedef struct __readwrite_upcall_args {
    unsigned char *buffer;
    ULONGLONG offset;
    ULONG len;
    ULONG out_len;
    ULONGLONG ctime;
} readwrite_upcall_args;

typedef struct __lock_upcall_args {
    uint64_t offset;
    uint64_t length;
    BOOLEAN exclusive;
    BOOLEAN blocking;
    BOOLEAN acquired;
} lock_upcall_args;

typedef struct __unlock_upcall_args {
    uint32_t count;
    unsigned char *buf;
    uint32_t buf_len;
} unlock_upcall_args;

typedef struct __getattr_upcall_args {
    FILE_BASIC_INFO basic_info;
    FILE_STANDARD_INFO std_info;
    FILE_ATTRIBUTE_TAG_INFO tag_info;
    FILE_INTERNAL_INFORMATION intr_info;
    FILE_NETWORK_OPEN_INFORMATION network_info;
    int query_class;
    int buf_len;
    int query_reply_len;
    ULONGLONG ctime;
} getattr_upcall_args;

typedef struct __setattr_upcall_args {
    const char *path;
    nfs41_root *root;
    nfs41_open_state *state;
    unsigned char *buf;
    uint32_t buf_len;
    int set_class;
    ULONGLONG ctime;
} setattr_upcall_args;

typedef struct __getexattr_upcall_args {
    const char *path;
    unsigned char *buf;
    uint32_t buf_len;
    ULONG eaindex;
    unsigned char *ealist;
    uint32_t ealist_len;
    uint32_t overflow;
    BOOLEAN single;
    BOOLEAN restart;
} getexattr_upcall_args;


typedef struct __setexattr_upcall_args {
    const char *path;
    unsigned char *buf;
    uint32_t buf_len;
    uint32_t mode;
    ULONGLONG ctime;
} setexattr_upcall_args;

typedef struct __readdir_upcall_args {
    const char *filter;
    nfs41_root *root;
    nfs41_open_state *state;
    int buf_len;
    int query_class;
    int query_reply_len;
    BOOLEAN initial;
    BOOLEAN restart;
    BOOLEAN single;
    unsigned char *kbuf;
} readdir_upcall_args;

typedef struct __symlink_upcall_args {
    nfs41_abs_path target_get;
    char *target_set;
    const char *path;
    BOOLEAN set;
} symlink_upcall_args;

typedef struct __volume_upcall_args {
    FS_INFORMATION_CLASS query;
    int len;
    union {
        FILE_FS_SIZE_INFORMATION size;
        FILE_FS_FULL_SIZE_INFORMATION fullsize;
        FILE_FS_ATTRIBUTE_INFORMATION attribute;
    } info;
} volume_upcall_args;

typedef struct __getacl_upcall_args {
    SECURITY_INFORMATION query;
    PSECURITY_DESCRIPTOR sec_desc;
    DWORD sec_desc_len;
} getacl_upcall_args;

typedef struct __setacl_upcall_args {
    SECURITY_INFORMATION query;
    PSECURITY_DESCRIPTOR sec_desc;
    ULONGLONG ctime;
} setacl_upcall_args;

typedef union __upcall_args {
    mount_upcall_args       mount;
    open_upcall_args        open;
    close_upcall_args       close;
    readwrite_upcall_args   rw;
    lock_upcall_args        lock;
    unlock_upcall_args      unlock;
    getattr_upcall_args     getattr;
    getexattr_upcall_args   getexattr;
    setattr_upcall_args     setattr;
    setexattr_upcall_args   setexattr;
    readdir_upcall_args     readdir;
    symlink_upcall_args     symlink;
    volume_upcall_args      volume;
    getacl_upcall_args      getacl;
    setacl_upcall_args      setacl;
} upcall_args;

typedef struct __nfs41_upcall {
    uint64_t                xid;
    uint32_t                opcode;
    uint32_t                status;
    uint32_t                last_error;
    upcall_args             args;

    uid_t                   uid;
    gid_t                   gid;

    /* store referenced pointers with the upcall for
     * automatic dereferencing on upcall_cleanup();
     * see upcall_root_ref() and upcall_open_state_ref() */
    nfs41_root              *root_ref;
    nfs41_open_state        *state_ref;
} nfs41_upcall;


/* upcall operation interface */
typedef int (*upcall_parse_proc)(unsigned char*, uint32_t, nfs41_upcall*);
typedef int (*upcall_handle_proc)(nfs41_upcall*);
typedef int (*upcall_marshall_proc)(unsigned char*, uint32_t*, nfs41_upcall*);
typedef void (*upcall_cancel_proc)(nfs41_upcall*);
typedef void (*upcall_cleanup_proc)(nfs41_upcall*);

typedef struct __nfs41_upcall_op {
    upcall_parse_proc       parse;
    upcall_handle_proc      handle;
    upcall_marshall_proc    marshall;
    upcall_cancel_proc      cancel;
    upcall_cleanup_proc     cleanup;
} nfs41_upcall_op;


/* upcall.c */
int upcall_parse(
    IN unsigned char *buffer,
    IN uint32_t length,
    OUT nfs41_upcall *upcall);

int upcall_handle(
    IN nfs41_upcall *upcall);

void upcall_marshall(
    IN nfs41_upcall *upcall,
    OUT unsigned char *buffer,
    IN uint32_t length,
    OUT uint32_t *length_out);

void upcall_cancel(
    IN nfs41_upcall *upcall);

void upcall_cleanup(
    IN nfs41_upcall *upcall);

#endif /* !__NFS41_DAEMON_UPCALL_H__ */
