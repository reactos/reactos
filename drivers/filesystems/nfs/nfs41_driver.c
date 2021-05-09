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

#define MINIRDR__NAME "Value is ignored, only fact of definition"
#include <rx.h>
#include <windef.h>
#include <winerror.h>

#include <ntstrsafe.h>

#ifdef __REACTOS__
#include <pseh/pseh2.h>
#endif

#include "nfs41_driver.h"
#include "nfs41_np.h"
#include "nfs41_debug.h"

#if defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7)
NTSTATUS NTAPI RtlUnicodeToUTF8N(CHAR *utf8_dest, ULONG utf8_bytes_max,
                                 ULONG *utf8_bytes_written,
                                 const WCHAR *uni_src, ULONG uni_bytes);
NTSTATUS NTAPI RtlUTF8ToUnicodeN(WCHAR *uni_dest, ULONG uni_bytes_max,
                                 ULONG *uni_bytes_written,
                                 const CHAR *utf8_src, ULONG utf8_bytes);
#endif /* defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7) */

#define USE_MOUNT_SEC_CONTEXT

/* debugging printout defines */
//#define DEBUG_FSDDISPATCH
#define DEBUG_MARSHAL_HEADER
#define DEBUG_MARSHAL_DETAIL
//#define DEBUG_OPEN
//#define DEBUG_CLOSE
//#define DEBUG_CACHE
#define DEBUG_INVALIDATE_CACHE
//#define DEBUG_READ
//#define DEBUG_WRITE
//#define DEBUG_DIR_QUERY
//#define DEBUG_FILE_QUERY
//#define DEBUG_FILE_SET
//#define DEBUG_ACL_QUERY
//#define DEBUG_ACL_SET
//#define DEBUG_EA_QUERY
//#define DEBUG_EA_SET
//#define DEBUG_LOCK
//#define DEBUG_MISC
#define DEBUG_TIME_BASED_COHERENCY
#define DEBUG_MOUNT
//#define DEBUG_VOLUME_QUERY

//#define ENABLE_TIMINGS
//#define ENABLE_INDV_TIMINGS
#ifdef ENABLE_TIMINGS
typedef struct __nfs41_timings {
    LONG tops, sops;
    LONGLONG ticks, size;
} nfs41_timings;

nfs41_timings lookup, readdir, open, close, getattr, setattr, getacl, setacl, volume,
    read, write, lock, unlock, setexattr, getexattr;
#endif
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD nfs41_driver_unload;
DRIVER_DISPATCH ( nfs41_FsdDispatch );

struct _MINIRDR_DISPATCH nfs41_ops;
PRDBSS_DEVICE_OBJECT nfs41_dev;

#define DISABLE_CACHING 0
#define ENABLE_READ_CACHING 1
#define ENABLE_WRITE_CACHING 2
#define ENABLE_READWRITE_CACHING 3

#define NFS41_MM_POOLTAG        ('nfs4')
#define NFS41_MM_POOLTAG_ACL    ('acls')
#define NFS41_MM_POOLTAG_MOUNT  ('mnts')
#define NFS41_MM_POOLTAG_OPEN   ('open')
#define NFS41_MM_POOLTAG_UP     ('upca')
#define NFS41_MM_POOLTAG_DOWN   ('down')

KEVENT upcallEvent;
FAST_MUTEX upcallLock, downcallLock, fcblistLock;
FAST_MUTEX xidLock;
FAST_MUTEX openOwnerLock;

LONGLONG xid = 0;
LONG open_owner_id = 1;

#define DECLARE_CONST_ANSI_STRING(_var, _string) \
    const CHAR _var ## _buffer[] = _string; \
    const ANSI_STRING _var = { sizeof(_string) - sizeof(CHAR), \
        sizeof(_string), (PCH) _var ## _buffer }
#define RELATIVE(wait) (-(wait))
#define NANOSECONDS(nanos) (((signed __int64)(nanos)) / 100L)
#define MICROSECONDS(micros) (((signed __int64)(micros)) * NANOSECONDS(1000L))
#define MILLISECONDS(milli) (((signed __int64)(milli)) * MICROSECONDS(1000L))
#define SECONDS(seconds) (((signed __int64)(seconds)) * MILLISECONDS(1000L))

DECLARE_CONST_ANSI_STRING(NfsV3Attributes, "NfsV3Attributes");
DECLARE_CONST_ANSI_STRING(NfsSymlinkTargetName, "NfsSymlinkTargetName");
DECLARE_CONST_ANSI_STRING(NfsActOnLink, "NfsActOnLink");

INLINE BOOL AnsiStrEq(
    IN const ANSI_STRING *lhs,
    IN const CHAR *rhs,
    IN const UCHAR rhs_len)
{
    return lhs->Length == rhs_len &&
        RtlCompareMemory(lhs->Buffer, rhs, rhs_len) == rhs_len;
}

typedef struct _nfs3_attrs {
    DWORD type, mode, nlink, uid, gid, filler1;
    LARGE_INTEGER size, used;
    struct {
        DWORD specdata1;
        DWORD specdata2;
    } rdev;
    LONGLONG fsid, fileid;
    LONGLONG atime, mtime, ctime;
} nfs3_attrs;
LARGE_INTEGER unix_time_diff; //needed to convert windows time to unix

enum ftype3 {
    NF3REG = 1,
    NF3DIR,
    NF3BLK,
    NF3CHR,
    NF3LNK,
    NF3SOCK,
    NF3FIFO
};

typedef enum _nfs41_updowncall_state {
    NFS41_WAITING_FOR_UPCALL,
    NFS41_WAITING_FOR_DOWNCALL,
    NFS41_DONE_PROCESSING,
    NFS41_NOT_WAITING
} nfs41_updowncall_state;

#ifdef __REACTOS__
#undef _errno
#undef errno
#endif

typedef struct _updowncall_entry {
    DWORD version;
    LONGLONG xid;
    DWORD opcode;
    NTSTATUS status;
    nfs41_updowncall_state state;
    FAST_MUTEX lock;
    LIST_ENTRY next;
    KEVENT cond;
    DWORD errno;
    BOOLEAN async_op;
    SECURITY_CLIENT_CONTEXT sec_ctx;
    PSECURITY_CLIENT_CONTEXT psec_ctx;
    HANDLE open_state;
    HANDLE session;
    PUNICODE_STRING filename;
    PVOID buf;
    ULONG buf_len;
    ULONGLONG ChangeTime;
    union {
        struct {
            PUNICODE_STRING srv_name;
            PUNICODE_STRING root;
            PFILE_FS_ATTRIBUTE_INFORMATION FsAttrs;
            DWORD sec_flavor;
            DWORD rsize;
            DWORD wsize;
            DWORD lease_time;
        } Mount;
        struct {
            PMDL MdlAddress;
            ULONGLONG offset;
            PRX_CONTEXT rxcontext;
        } ReadWrite;
        struct {
            LONGLONG offset;
            LONGLONG length;
            BOOLEAN exclusive;
            BOOLEAN blocking;
        } Lock;
        struct {
            ULONG count;
            LOWIO_LOCK_LIST locks;
        } Unlock;
        struct {
            FILE_BASIC_INFORMATION binfo;
            FILE_STANDARD_INFORMATION sinfo;
            UNICODE_STRING symlink;
            ULONG access_mask;
            ULONG access_mode;
            ULONG attrs;
            ULONG copts;
            ULONG disp;
            ULONG cattrs;
            LONG open_owner_id;
            DWORD mode;
            HANDLE srv_open;
            DWORD deleg_type;
            BOOLEAN symlink_embedded;
            PMDL EaMdl;
            PVOID EaBuffer;
        } Open;
        struct {
            HANDLE srv_open;
            BOOLEAN remove;
            BOOLEAN renamed;
        } Close;
        struct {
            PUNICODE_STRING filter;
            FILE_INFORMATION_CLASS InfoClass;
            BOOLEAN restart_scan;
            BOOLEAN return_single;
            BOOLEAN initial_query;
            PMDL mdl;
            PVOID mdl_buf;
        } QueryFile;
        struct {
            FILE_INFORMATION_CLASS InfoClass;
        } SetFile;
        struct {
            DWORD mode;
        } SetEa;
        struct {
            PVOID EaList;
            ULONG EaListLength;
            ULONG Overflow;
            ULONG EaIndex;
            BOOLEAN ReturnSingleEntry;
            BOOLEAN RestartScan;
        } QueryEa;
        struct {
            PUNICODE_STRING target;
            BOOLEAN set;
        } Symlink;
        struct {
            FS_INFORMATION_CLASS query;
        } Volume;
        struct {
            SECURITY_INFORMATION query;
        } Acl;
    } u;

} nfs41_updowncall_entry;

typedef struct _updowncall_list {
    LIST_ENTRY head;
} nfs41_updowncall_list;
nfs41_updowncall_list upcall, downcall;

typedef struct _nfs41_mount_entry {
    LIST_ENTRY next;
    LUID login_id;
    HANDLE authsys_session;
    HANDLE gss_session;
    HANDLE gssi_session;
    HANDLE gssp_session;
} nfs41_mount_entry;

typedef struct _nfs41_mount_list {
    LIST_ENTRY head;
} nfs41_mount_list;

#define nfs41_AddEntry(lock,list,pEntry)                    \
            ExAcquireFastMutex(&lock);                      \
            InsertTailList(&(list).head, &(pEntry)->next);  \
            ExReleaseFastMutex(&lock);
#define nfs41_RemoveFirst(lock,list,pEntry)                 \
            ExAcquireFastMutex(&lock);                      \
            pEntry = (IsListEmpty(&(list).head)             \
            ? NULL                                          \
            : RemoveHeadList(&(list).head));                \
            ExReleaseFastMutex(&lock);
#define nfs41_RemoveEntry(lock,pEntry)                      \
            ExAcquireFastMutex(&lock);                      \
            RemoveEntryList(&pEntry->next);                 \
            ExReleaseFastMutex(&lock);
#define nfs41_IsListEmpty(lock,list,flag)                   \
            ExAcquireFastMutex(&lock);                      \
            flag = IsListEmpty(&(list).head);               \
            ExReleaseFastMutex(&lock);
#define nfs41_GetFirstEntry(lock,list,pEntry)               \
            ExAcquireFastMutex(&lock);                      \
            pEntry = (IsListEmpty(&(list).head)             \
             ? NULL                                         \
             : (nfs41_updowncall_entry *)                   \
               (CONTAINING_RECORD((list).head.Flink,        \
                                  nfs41_updowncall_entry,   \
                                  next)));                  \
            ExReleaseFastMutex(&lock);
#define nfs41_GetFirstMountEntry(lock,list,pEntry)          \
            ExAcquireFastMutex(&lock);                      \
            pEntry = (IsListEmpty(&(list).head)             \
             ? NULL                                         \
             : (nfs41_mount_entry *)                        \
               (CONTAINING_RECORD((list).head.Flink,        \
                                  nfs41_mount_entry,        \
                                  next)));                  \
            ExReleaseFastMutex(&lock);

/* In order to cooperate with other network providers,
 * we only claim paths of the format '\\server\nfs4\path' */
DECLARE_CONST_UNICODE_STRING(NfsPrefix, L"\\nfs4");
DECLARE_CONST_UNICODE_STRING(AUTH_SYS_NAME, L"sys");
DECLARE_CONST_UNICODE_STRING(AUTHGSS_KRB5_NAME, L"krb5");
DECLARE_CONST_UNICODE_STRING(AUTHGSS_KRB5I_NAME, L"krb5i");
DECLARE_CONST_UNICODE_STRING(AUTHGSS_KRB5P_NAME, L"krb5p");
DECLARE_CONST_UNICODE_STRING(SLASH, L"\\");
DECLARE_CONST_UNICODE_STRING(EMPTY_STRING, L"");

#define SERVER_NAME_BUFFER_SIZE         1024
#define MOUNT_CONFIG_RW_SIZE_MIN        1024
#define MOUNT_CONFIG_RW_SIZE_DEFAULT    1048576
#define MOUNT_CONFIG_RW_SIZE_MAX        1048576
#define MAX_SEC_FLAVOR_LEN              12
#define UPCALL_TIMEOUT_DEFAULT          50  /* in seconds */

typedef struct _NFS41_MOUNT_CONFIG {
    DWORD ReadSize;
    DWORD WriteSize;
    BOOLEAN ReadOnly;
    BOOLEAN write_thru;
    BOOLEAN nocache;
    WCHAR srv_buffer[SERVER_NAME_BUFFER_SIZE];
    UNICODE_STRING SrvName;
    WCHAR mntpt_buffer[MAX_PATH];
    UNICODE_STRING MntPt;
    WCHAR sec_flavor[MAX_SEC_FLAVOR_LEN];
    UNICODE_STRING SecFlavor;
    DWORD timeout;
} NFS41_MOUNT_CONFIG, *PNFS41_MOUNT_CONFIG;

typedef struct _NFS41_NETROOT_EXTENSION {
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;
    DWORD                   nfs41d_version;
    BOOLEAN                 mounts_init;
    FAST_MUTEX              mountLock;
    nfs41_mount_list        mounts;
} NFS41_NETROOT_EXTENSION, *PNFS41_NETROOT_EXTENSION;
#define NFS41GetNetRootExtension(pNetRoot)      \
        (((pNetRoot) == NULL) ? NULL :          \
        (PNFS41_NETROOT_EXTENSION)((pNetRoot)->Context))

/* FileSystemName as reported by FileFsAttributeInfo query */
#define FS_NAME     L"NFS"
#define FS_NAME_LEN (sizeof(FS_NAME) - sizeof(WCHAR))
#define FS_ATTR_LEN (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + FS_NAME_LEN)

/* FileSystemName as reported by FileFsAttributeInfo query */
#define VOL_NAME     L"PnfsVolume"
#define VOL_NAME_LEN (sizeof(VOL_NAME) - sizeof(WCHAR))
#define VOL_ATTR_LEN (sizeof(FILE_FS_VOLUME_INFORMATION) + VOL_NAME_LEN)

typedef struct _NFS41_V_NET_ROOT_EXTENSION {
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;
    HANDLE                  session;
    FILE_FS_ATTRIBUTE_INFORMATION FsAttrs;
    DWORD                   sec_flavor;
    DWORD                   timeout;
    USHORT                  MountPathLen;
    BOOLEAN                 read_only;
    BOOLEAN                 write_thru;
    BOOLEAN                 nocache;
#define STORE_MOUNT_SEC_CONTEXT
#ifdef STORE_MOUNT_SEC_CONTEXT
    SECURITY_CLIENT_CONTEXT mount_sec_ctx;
#endif
} NFS41_V_NET_ROOT_EXTENSION, *PNFS41_V_NET_ROOT_EXTENSION;
#define NFS41GetVNetRootExtension(pVNetRoot)      \
        (((pVNetRoot) == NULL) ? NULL :           \
        (PNFS41_V_NET_ROOT_EXTENSION)((pVNetRoot)->Context))

typedef struct _NFS41_FCB {
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;
    FILE_BASIC_INFORMATION  BasicInfo;
    FILE_STANDARD_INFORMATION StandardInfo;
    BOOLEAN                 Renamed;
    BOOLEAN                 DeletePending;
    DWORD                   mode;
    ULONGLONG               changeattr;
} NFS41_FCB, *PNFS41_FCB;
#define NFS41GetFcbExtension(pFcb)      \
        (((pFcb) == NULL) ? NULL : (PNFS41_FCB)((pFcb)->Context))

typedef struct _NFS41_FOBX {
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;

    HANDLE nfs41_open_state;
    SECURITY_CLIENT_CONTEXT sec_ctx;
    PVOID acl;
    DWORD acl_len;
    LARGE_INTEGER time;
    DWORD deleg_type;
    BOOLEAN write_thru;
    BOOLEAN nocache;
} NFS41_FOBX, *PNFS41_FOBX;
#define NFS41GetFobxExtension(pFobx)  \
        (((pFobx) == NULL) ? NULL : (PNFS41_FOBX)((pFobx)->Context))

typedef struct _NFS41_SERVER_ENTRY {
    PMRX_SRV_CALL                 pRdbssSrvCall;
    WCHAR                         NameBuffer[SERVER_NAME_BUFFER_SIZE];
    UNICODE_STRING                Name;             // the server name.
} NFS41_SERVER_ENTRY, *PNFS41_SERVER_ENTRY;

typedef struct _NFS41_DEVICE_EXTENSION {
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;
    PRDBSS_DEVICE_OBJECT    DeviceObject;
    ULONG                   ActiveNodes;
    HANDLE                  SharedMemorySection;
    DWORD                   nfs41d_version;
    BYTE                    VolAttrs[VOL_ATTR_LEN];
    DWORD                   VolAttrsLen;
    HANDLE                  openlistHandle;
} NFS41_DEVICE_EXTENSION, *PNFS41_DEVICE_EXTENSION;

#define NFS41GetDeviceExtension(RxContext,pExt)        \
        PNFS41_DEVICE_EXTENSION pExt = (PNFS41_DEVICE_EXTENSION) \
        ((PBYTE)(RxContext->RxDeviceObject) + sizeof(RDBSS_DEVICE_OBJECT))

typedef struct _nfs41_fcb_list_entry {
    LIST_ENTRY next;
    PMRX_FCB fcb;
    HANDLE session;
    PNFS41_FOBX nfs41_fobx;
    ULONGLONG ChangeTime;
    BOOLEAN skip;
} nfs41_fcb_list_entry;

typedef struct _nfs41_fcb_list {
    LIST_ENTRY head;
} nfs41_fcb_list;
nfs41_fcb_list openlist;

typedef enum _NULMRX_STORAGE_TYPE_CODES {
    NTC_NFS41_DEVICE_EXTENSION      =   (NODE_TYPE_CODE)0xFC00,
} NFS41_STORAGE_TYPE_CODES;
#define RxDefineNode( node, type )          \
        node->NodeTypeCode = NTC_##type;    \
        node->NodeByteSize = sizeof(type);

#define RDR_NULL_STATE  0
#define RDR_UNLOADED    1
#define RDR_UNLOADING   2
#define RDR_LOADING     3
#define RDR_LOADED      4
#define RDR_STOPPED     5
#define RDR_STOPPING    6
#define RDR_STARTING    7
#define RDR_STARTED     8

nfs41_init_driver_state nfs41_init_state = NFS41_INIT_DRIVER_STARTABLE;
nfs41_start_driver_state nfs41_start_state = NFS41_START_DRIVER_STARTABLE;

NTSTATUS map_readwrite_errors(DWORD status);

void print_debug_header(
    PRX_CONTEXT RxContext)
{

    PIO_STACK_LOCATION IrpSp = RxContext->CurrentIrpSp;

    if (IrpSp) {
        DbgP("FileOject %p name %wZ access r=%d,w=%d,d=%d share r=%d,w=%d,d=%d\n",
            IrpSp->FileObject, &IrpSp->FileObject->FileName,
            IrpSp->FileObject->ReadAccess, IrpSp->FileObject->WriteAccess,
            IrpSp->FileObject->DeleteAccess, IrpSp->FileObject->SharedRead,
            IrpSp->FileObject->SharedWrite, IrpSp->FileObject->SharedDelete);
        print_file_object(0, IrpSp->FileObject);
        print_irps_flags(0, RxContext->CurrentIrpSp);
    } else
        DbgP("Couldn't print FileObject IrpSp is NULL\n");

    print_fo_all(1, RxContext);
    if (RxContext->CurrentIrp)
        print_irp_flags(0, RxContext->CurrentIrp);
}

/* convert strings from unicode -> ansi during marshalling to
 * save space in the upcall buffers and avoid extra copies */
INLINE ULONG length_as_utf8(
    PCUNICODE_STRING str)
{
    ULONG ActualCount = 0;
    RtlUnicodeToUTF8N(NULL, 0xffff, &ActualCount, str->Buffer, str->Length);
    return sizeof(str->MaximumLength) + ActualCount + sizeof(UNICODE_NULL);
}

NTSTATUS marshall_unicode_as_utf8(
    IN OUT unsigned char **pos,
    IN PCUNICODE_STRING str)
{
    ANSI_STRING ansi;
    ULONG ActualCount;
    NTSTATUS status;

    if (str->Length == 0) {
        status = STATUS_SUCCESS;
        ActualCount = 0;
        ansi.MaximumLength = 1;
        goto out_copy;
    }

    /* query the number of bytes required for the utf8 encoding */
    status = RtlUnicodeToUTF8N(NULL, 0xffff,
        &ActualCount, str->Buffer, str->Length);
    if (status) {
        print_error("RtlUnicodeToUTF8N('%wZ') failed with 0x%08X\n",
            str, status);
        goto out;
    }

    /* convert the string directly into the upcall buffer */
    ansi.Buffer = (PCHAR)*pos + sizeof(ansi.MaximumLength);
    ansi.MaximumLength = (USHORT)ActualCount + sizeof(UNICODE_NULL);
    status = RtlUnicodeToUTF8N(ansi.Buffer, ansi.MaximumLength,
        &ActualCount, str->Buffer, str->Length);
    if (status) {
        print_error("RtlUnicodeToUTF8N(%hu, '%wZ', %hu) failed with 0x%08X\n",
            ansi.MaximumLength, str, str->Length, status);
        goto out;
    }

out_copy:
    RtlCopyMemory(*pos, &ansi.MaximumLength, sizeof(ansi.MaximumLength));
    *pos += sizeof(ansi.MaximumLength);
    (*pos)[ActualCount] = '\0';
    *pos += ansi.MaximumLength;
out:
    return status;
}

NTSTATUS marshal_nfs41_header(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    header_len = sizeof(entry->version) + sizeof(entry->xid) +
        sizeof(entry->opcode) + 2 * sizeof(HANDLE);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    else
        *len = header_len;
    RtlCopyMemory(tmp, &entry->version, sizeof(entry->version));
    tmp += sizeof(entry->version);
    RtlCopyMemory(tmp, &entry->xid, sizeof(entry->xid));
    tmp += sizeof(entry->xid);
    RtlCopyMemory(tmp, &entry->opcode, sizeof(entry->opcode));
    tmp += sizeof(entry->opcode);
    RtlCopyMemory(tmp, &entry->session, sizeof(HANDLE));
    tmp += sizeof(HANDLE);
    RtlCopyMemory(tmp, &entry->open_state, sizeof(HANDLE));
    tmp += sizeof(HANDLE);

#ifdef DEBUG_MARSHAL_HEADER
    if (MmIsAddressValid(entry->filename))
        DbgP("[upcall header] xid=%lld opcode=%s filename=%wZ version=%d "
            "session=0x%x open_state=0x%x\n", entry->xid,
            opcode2string(entry->opcode), entry->filename,
            entry->version, entry->session, entry->open_state);
    else
        status = STATUS_INTERNAL_ERROR;
#endif
out:
    return status;
}

const char* secflavorop2name(
    DWORD sec_flavor)
{
    switch(sec_flavor) {
    case RPCSEC_AUTH_SYS:      return "AUTH_SYS";
    case RPCSEC_AUTHGSS_KRB5:  return "AUTHGSS_KRB5";
    case RPCSEC_AUTHGSS_KRB5I: return "AUTHGSS_KRB5I";
    case RPCSEC_AUTHGSS_KRB5P: return "AUTHGSS_KRB5P";
    }

    return "UNKNOWN FLAVOR";
}
NTSTATUS marshal_nfs41_mount(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    /* 03/25/2011: Kernel crash to nfsd not running but mount upcall cued up */
    if (!MmIsAddressValid(entry->u.Mount.srv_name) ||
            !MmIsAddressValid(entry->u.Mount.root)) {
        status = STATUS_INTERNAL_ERROR;
        goto out;
    }
    header_len = *len + length_as_utf8(entry->u.Mount.srv_name) +
        length_as_utf8(entry->u.Mount.root) + 3 * sizeof(DWORD);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    status = marshall_unicode_as_utf8(&tmp, entry->u.Mount.srv_name);
    if (status) goto out;
    status = marshall_unicode_as_utf8(&tmp, entry->u.Mount.root);
    if (status) goto out;
    RtlCopyMemory(tmp, &entry->u.Mount.sec_flavor, sizeof(DWORD));
    tmp += sizeof(DWORD);
    RtlCopyMemory(tmp, &entry->u.Mount.rsize, sizeof(DWORD));
    tmp += sizeof(DWORD);
    RtlCopyMemory(tmp, &entry->u.Mount.wsize, sizeof(DWORD));

    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_mount: server name=%wZ mount point=%wZ sec_flavor=%s "
         "rsize=%d wsize=%d\n", entry->u.Mount.srv_name, entry->u.Mount.root,
         secflavorop2name(entry->u.Mount.sec_flavor), entry->u.Mount.rsize,
         entry->u.Mount.wsize);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_unmount(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    return marshal_nfs41_header(entry, buf, buf_len, len);
}

NTSTATUS marshal_nfs41_open(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + length_as_utf8(entry->filename) +
        7 * sizeof(ULONG) + 2 * sizeof(HANDLE) +
        length_as_utf8(&entry->u.Open.symlink);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    status = marshall_unicode_as_utf8(&tmp, entry->filename);
    if (status) goto out;
    RtlCopyMemory(tmp, &entry->u.Open.access_mask,
        sizeof(entry->u.Open.access_mask));
    tmp += sizeof(entry->u.Open.access_mask);
    RtlCopyMemory(tmp, &entry->u.Open.access_mode,
        sizeof(entry->u.Open.access_mode));
    tmp += sizeof(entry->u.Open.access_mode);
    RtlCopyMemory(tmp, &entry->u.Open.attrs, sizeof(entry->u.Open.attrs));
    tmp += sizeof(entry->u.Open.attrs);
    RtlCopyMemory(tmp, &entry->u.Open.copts, sizeof(entry->u.Open.copts));
    tmp += sizeof(entry->u.Open.copts);
    RtlCopyMemory(tmp, &entry->u.Open.disp, sizeof(entry->u.Open.disp));
    tmp += sizeof(entry->u.Open.disp);
    RtlCopyMemory(tmp, &entry->u.Open.open_owner_id,
        sizeof(entry->u.Open.open_owner_id));
    tmp += sizeof(entry->u.Open.open_owner_id);
    RtlCopyMemory(tmp, &entry->u.Open.mode, sizeof(DWORD));
    tmp += sizeof(DWORD);
    RtlCopyMemory(tmp, &entry->u.Open.srv_open, sizeof(HANDLE));
    tmp += sizeof(HANDLE);
    status = marshall_unicode_as_utf8(&tmp, &entry->u.Open.symlink);
    if (status) goto out;

    _SEH2_TRY {
        if (entry->u.Open.EaMdl) {
            entry->u.Open.EaBuffer =
                MmMapLockedPagesSpecifyCache(entry->u.Open.EaMdl,
#ifndef __REACTOS__
                    UserMode, MmNonCached, NULL, TRUE, NormalPagePriority);
#else
                    UserMode, MmCached, NULL, TRUE, NormalPagePriority);
#endif
            if (entry->u.Open.EaBuffer == NULL) {
                print_error("MmMapLockedPagesSpecifyCache failed to map pages\n");
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto out;
            }
        }
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        print_error("Call to MmMapLocked failed due to exception 0x%x\n", _SEH2_GetExceptionCode());
        status = STATUS_ACCESS_DENIED;
        goto out;
    } _SEH2_END;
    RtlCopyMemory(tmp, &entry->u.Open.EaBuffer, sizeof(HANDLE));
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_open: name=%wZ mask=0x%x access=0x%x attrs=0x%x "
         "opts=0x%x dispo=0x%x open_owner_id=0x%x mode=%o srv_open=%p ea=%p\n",
         entry->filename, entry->u.Open.access_mask,
         entry->u.Open.access_mode, entry->u.Open.attrs, entry->u.Open.copts,
         entry->u.Open.disp, entry->u.Open.open_owner_id, entry->u.Open.mode,
         entry->u.Open.srv_open, entry->u.Open.EaBuffer);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_rw(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + sizeof(entry->buf_len) +
        sizeof(entry->u.ReadWrite.offset) + sizeof(HANDLE);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    RtlCopyMemory(tmp, &entry->buf_len, sizeof(entry->buf_len));
    tmp += sizeof(entry->buf_len);
    RtlCopyMemory(tmp, &entry->u.ReadWrite.offset,
        sizeof(entry->u.ReadWrite.offset));
    tmp += sizeof(entry->u.ReadWrite.offset);
    _SEH2_TRY {
        entry->u.ReadWrite.MdlAddress->MdlFlags |= MDL_MAPPING_CAN_FAIL;
        entry->buf =
            MmMapLockedPagesSpecifyCache(entry->u.ReadWrite.MdlAddress,
#ifndef __REACTOS__
                UserMode, MmNonCached, NULL, TRUE, NormalPagePriority);
#else
                UserMode, MmCached, NULL, TRUE, NormalPagePriority);
#endif
        if (entry->buf == NULL) {
            print_error("MmMapLockedPagesSpecifyCache failed to map pages\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto out;
        }
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        NTSTATUS code;
        code = _SEH2_GetExceptionCode();
        print_error("Call to MmMapLocked failed due to exception 0x%x\n", code);
        status = STATUS_ACCESS_DENIED;
        goto out;
    } _SEH2_END;
    RtlCopyMemory(tmp, &entry->buf, sizeof(HANDLE));
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_rw: len=%lu offset=%llu MdlAddress=%p Userspace=%p\n",
         entry->buf_len, entry->u.ReadWrite.offset,
         entry->u.ReadWrite.MdlAddress, entry->buf);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_lock(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + 2 * sizeof(LONGLONG) + 2 * sizeof(BOOLEAN);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    RtlCopyMemory(tmp, &entry->u.Lock.offset, sizeof(LONGLONG));
    tmp += sizeof(LONGLONG);
    RtlCopyMemory(tmp, &entry->u.Lock.length, sizeof(LONGLONG));
    tmp += sizeof(LONGLONG);
    RtlCopyMemory(tmp, &entry->u.Lock.exclusive, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    RtlCopyMemory(tmp, &entry->u.Lock.blocking, sizeof(BOOLEAN));
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_lock: offset=%llx length=%llx exclusive=%u "
         "blocking=%u\n", entry->u.Lock.offset, entry->u.Lock.length,
         entry->u.Lock.exclusive, entry->u.Lock.blocking);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_unlock(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;
    PLOWIO_LOCK_LIST lock;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + sizeof(ULONG) +
        entry->u.Unlock.count * 2 * sizeof(LONGLONG);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    RtlCopyMemory(tmp, &entry->u.Unlock.count, sizeof(ULONG));
    tmp += sizeof(ULONG);

    lock = &entry->u.Unlock.locks;
    while (lock) {
        RtlCopyMemory(tmp, &lock->ByteOffset, sizeof(LONGLONG));
        tmp += sizeof(LONGLONG);
        RtlCopyMemory(tmp, &lock->Length, sizeof(LONGLONG));
        tmp += sizeof(LONGLONG);
        lock = lock->Next;
    }
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_unlock: count=%u\n", entry->u.Unlock.count);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_close(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + sizeof(BOOLEAN) + sizeof(HANDLE);
    if (entry->u.Close.remove)
        header_len += length_as_utf8(entry->filename) +
            sizeof(BOOLEAN);

    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    RtlCopyMemory(tmp, &entry->u.Close.remove, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    RtlCopyMemory(tmp, &entry->u.Close.srv_open, sizeof(HANDLE));
    if (entry->u.Close.remove) {
        tmp += sizeof(HANDLE);
        status = marshall_unicode_as_utf8(&tmp, entry->filename);
        if (status) goto out;
        RtlCopyMemory(tmp, &entry->u.Close.renamed, sizeof(BOOLEAN));
    }
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_close: name=%wZ remove=%d srv_open=%p renamed=%d\n",
        entry->filename->Length?entry->filename:&SLASH,
        entry->u.Close.remove, entry->u.Close.srv_open, entry->u.Close.renamed);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_dirquery(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + 2 * sizeof(ULONG) + sizeof(HANDLE) +
        length_as_utf8(entry->u.QueryFile.filter) + 3 * sizeof(BOOLEAN);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    RtlCopyMemory(tmp, &entry->u.QueryFile.InfoClass, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, &entry->buf_len, sizeof(ULONG));
    tmp += sizeof(ULONG);
    status = marshall_unicode_as_utf8(&tmp, entry->u.QueryFile.filter);
    if (status) goto out;
    RtlCopyMemory(tmp, &entry->u.QueryFile.initial_query, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    RtlCopyMemory(tmp, &entry->u.QueryFile.restart_scan, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    RtlCopyMemory(tmp, &entry->u.QueryFile.return_single, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    _SEH2_TRY {
        entry->u.QueryFile.mdl_buf =
            MmMapLockedPagesSpecifyCache(entry->u.QueryFile.mdl,
#ifndef __REACTOS__
                UserMode, MmNonCached, NULL, TRUE, NormalPagePriority);
#else
                UserMode, MmCached, NULL, TRUE, NormalPagePriority);
#endif
        if (entry->u.QueryFile.mdl_buf == NULL) {
            print_error("MmMapLockedPagesSpecifyCache failed to map pages\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto out;
        }
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        NTSTATUS code;
        code = _SEH2_GetExceptionCode();
        print_error("Call to MmMapLocked failed due to exception 0x%x\n", code);
        status = STATUS_ACCESS_DENIED;
        goto out;
    } _SEH2_END;
    RtlCopyMemory(tmp, &entry->u.QueryFile.mdl_buf, sizeof(HANDLE));
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_dirquery: filter='%wZ'class=%d len=%d "
         "1st\\restart\\single=%d\\%d\\%d\n", entry->u.QueryFile.filter,
         entry->u.QueryFile.InfoClass, entry->buf_len,
         entry->u.QueryFile.initial_query, entry->u.QueryFile.restart_scan,
         entry->u.QueryFile.return_single);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_filequery(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + 2 * sizeof(ULONG);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    RtlCopyMemory(tmp, &entry->u.QueryFile.InfoClass, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, &entry->buf_len, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, &entry->session, sizeof(HANDLE));
    tmp += sizeof(HANDLE);
    RtlCopyMemory(tmp, &entry->open_state, sizeof(HANDLE));
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_filequery: class=%d\n", entry->u.QueryFile.InfoClass);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_fileset(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + length_as_utf8(entry->filename) +
        2 * sizeof(ULONG) + entry->buf_len;
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    status = marshall_unicode_as_utf8(&tmp, entry->filename);
    if (status) goto out;
    RtlCopyMemory(tmp, &entry->u.SetFile.InfoClass, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, &entry->buf_len, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, entry->buf, entry->buf_len);
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_fileset: filename='%wZ' class=%d\n",
        entry->filename, entry->u.SetFile.InfoClass);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_easet(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + length_as_utf8(entry->filename) +
        sizeof(ULONG) + entry->buf_len  + sizeof(DWORD);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    status = marshall_unicode_as_utf8(&tmp, entry->filename);
    if (status) goto out;
    RtlCopyMemory(tmp, &entry->u.SetEa.mode, sizeof(DWORD));
    tmp += sizeof(DWORD);
    RtlCopyMemory(tmp, &entry->buf_len, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, entry->buf, entry->buf_len);
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_easet: filename=%wZ, buflen=%d mode=0x%x\n",
        entry->filename, entry->buf_len, entry->u.SetEa.mode);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_eaget(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + length_as_utf8(entry->filename) +
        3 * sizeof(ULONG) + entry->u.QueryEa.EaListLength + 2 * sizeof(BOOLEAN);

    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    status = marshall_unicode_as_utf8(&tmp, entry->filename);
    if (status) goto out;
    RtlCopyMemory(tmp, &entry->u.QueryEa.EaIndex, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, &entry->u.QueryEa.RestartScan, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    RtlCopyMemory(tmp, &entry->u.QueryEa.ReturnSingleEntry, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    RtlCopyMemory(tmp, &entry->buf_len, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, &entry->u.QueryEa.EaListLength, sizeof(ULONG));
    tmp += sizeof(ULONG);
    if (entry->u.QueryEa.EaList && entry->u.QueryEa.EaListLength)
        RtlCopyMemory(tmp, entry->u.QueryEa.EaList,
            entry->u.QueryEa.EaListLength);
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_eaget: filename=%wZ, index=%d list_len=%d "
        "rescan=%d single=%d\n", entry->filename,
        entry->u.QueryEa.EaIndex, entry->u.QueryEa.EaListLength,
        entry->u.QueryEa.RestartScan, entry->u.QueryEa.ReturnSingleEntry);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_symlink(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + sizeof(BOOLEAN) + length_as_utf8(entry->filename);
    if (entry->u.Symlink.set)
        header_len += length_as_utf8(entry->u.Symlink.target);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    status = marshall_unicode_as_utf8(&tmp, entry->filename);
    if (status) goto out;
    RtlCopyMemory(tmp, &entry->u.Symlink.set, sizeof(BOOLEAN));
    tmp += sizeof(BOOLEAN);
    if (entry->u.Symlink.set) {
        status = marshall_unicode_as_utf8(&tmp, entry->u.Symlink.target);
        if (status) goto out;
    }
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_symlink: name %wZ symlink target %wZ\n",
         entry->filename,
         entry->u.Symlink.set?entry->u.Symlink.target : NULL);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_volume(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + sizeof(FS_INFORMATION_CLASS);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    RtlCopyMemory(tmp, &entry->u.Volume.query, sizeof(FS_INFORMATION_CLASS));
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_volume: class=%d\n", entry->u.Volume.query);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_getacl(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + sizeof(SECURITY_INFORMATION);
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    RtlCopyMemory(tmp, &entry->u.Acl.query, sizeof(SECURITY_INFORMATION));
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_getacl: class=0x%x\n", entry->u.Acl.query);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_setacl(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG header_len = 0;
    unsigned char *tmp = buf;

    status = marshal_nfs41_header(entry, tmp, buf_len, len);
    if (status) goto out;
    else tmp += *len;

    header_len = *len + sizeof(SECURITY_INFORMATION) +
        sizeof(ULONG) + entry->buf_len;
    if (header_len > buf_len) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    RtlCopyMemory(tmp, &entry->u.Acl.query, sizeof(SECURITY_INFORMATION));
    tmp += sizeof(SECURITY_INFORMATION);
    RtlCopyMemory(tmp, &entry->buf_len, sizeof(ULONG));
    tmp += sizeof(ULONG);
    RtlCopyMemory(tmp, entry->buf, entry->buf_len);
    *len = header_len;

#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("marshal_nfs41_setacl: class=0x%x sec_desc_len=%lu\n",
         entry->u.Acl.query, entry->buf_len);
#endif
out:
    return status;
}

NTSTATUS marshal_nfs41_shutdown(
    nfs41_updowncall_entry *entry,
    unsigned char *buf,
    ULONG buf_len,
    ULONG *len)
{
    return marshal_nfs41_header(entry, buf, buf_len, len);
}

void nfs41_invalidate_cache (
    IN PRX_CONTEXT RxContext)
{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    unsigned char *buf = LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    ULONG flag = DISABLE_CACHING;
    PMRX_SRV_OPEN srv_open;

    RtlCopyMemory(&srv_open, buf, sizeof(HANDLE));
#ifdef DEBUG_INVALIDATE_CACHE
    DbgP("nfs41_invalidate_cache: received srv_open=%p %wZ\n",
        srv_open, srv_open->pAlreadyPrefixedName);
#endif
    if (MmIsAddressValid(srv_open))
        RxIndicateChangeOfBufferingStateForSrvOpen(
            srv_open->pFcb->pNetRoot->pSrvCall, srv_open,
            srv_open->Key, ULongToPtr(flag));
}

NTSTATUS handle_upcall(
    IN PRX_CONTEXT RxContext,
    IN nfs41_updowncall_entry *entry,
    OUT ULONG *len)
{
    NTSTATUS status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG cbOut = LowIoContext->ParamsFor.IoCtl.OutputBufferLength;
    unsigned char *pbOut = LowIoContext->ParamsFor.IoCtl.pOutputBuffer;

    status = SeImpersonateClientEx(entry->psec_ctx, NULL);
    if (status != STATUS_SUCCESS) {
        print_error("SeImpersonateClientEx failed %x\n", status);
        goto out;
    }

    switch(entry->opcode) {
    case NFS41_SHUTDOWN:
        status = marshal_nfs41_shutdown(entry, pbOut, cbOut, len);
        KeSetEvent(&entry->cond, 0, FALSE);
        break;
    case NFS41_MOUNT:
        status = marshal_nfs41_mount(entry, pbOut, cbOut, len);
        break;
    case NFS41_UNMOUNT:
        status = marshal_nfs41_unmount(entry, pbOut, cbOut, len);
        break;
    case NFS41_OPEN:
        status = marshal_nfs41_open(entry, pbOut, cbOut, len);
        break;
    case NFS41_READ:
        status = marshal_nfs41_rw(entry, pbOut, cbOut, len);
        break;
    case NFS41_WRITE:
        status = marshal_nfs41_rw(entry, pbOut, cbOut, len);
        break;
    case NFS41_LOCK:
        status = marshal_nfs41_lock(entry, pbOut, cbOut, len);
        break;
    case NFS41_UNLOCK:
        status = marshal_nfs41_unlock(entry, pbOut, cbOut, len);
        break;
    case NFS41_CLOSE:
        status = marshal_nfs41_close(entry, pbOut, cbOut, len);
        break;
    case NFS41_DIR_QUERY:
        status = marshal_nfs41_dirquery(entry, pbOut, cbOut, len);
        break;
    case NFS41_FILE_QUERY:
        status = marshal_nfs41_filequery(entry, pbOut, cbOut, len);
        break;
    case NFS41_FILE_SET:
        status = marshal_nfs41_fileset(entry, pbOut, cbOut, len);
        break;
    case NFS41_EA_SET:
        status = marshal_nfs41_easet(entry, pbOut, cbOut, len);
        break;
    case NFS41_EA_GET:
        status = marshal_nfs41_eaget(entry, pbOut, cbOut, len);
        break;
    case NFS41_SYMLINK:
        status = marshal_nfs41_symlink(entry, pbOut, cbOut, len);
        break;
    case NFS41_VOLUME_QUERY:
        status = marshal_nfs41_volume(entry, pbOut, cbOut, len);
        break;
    case NFS41_ACL_QUERY:
        status = marshal_nfs41_getacl(entry, pbOut, cbOut, len);
        break;
    case NFS41_ACL_SET:
        status = marshal_nfs41_setacl(entry, pbOut, cbOut, len);
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        print_error("Unknown nfs41 ops %d\n", entry->opcode);
    }

    if (status == STATUS_SUCCESS)
        print_hexbuf(0, (unsigned char *)"upcall buffer", pbOut, *len);

out:
    return status;
}

NTSTATUS nfs41_UpcallCreate(
    IN DWORD opcode,
    IN PSECURITY_CLIENT_CONTEXT clnt_sec_ctx,
    IN HANDLE session,
    IN HANDLE open_state,
    IN DWORD version,
    IN PUNICODE_STRING filename,
    OUT nfs41_updowncall_entry **entry_out)
{
    NTSTATUS status = STATUS_SUCCESS;
    nfs41_updowncall_entry *entry;
    SECURITY_SUBJECT_CONTEXT sec_ctx;
    SECURITY_QUALITY_OF_SERVICE sec_qos;

    entry = RxAllocatePoolWithTag(NonPagedPool, sizeof(nfs41_updowncall_entry),
                NFS41_MM_POOLTAG_UP);
    if (entry == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    RtlZeroMemory(entry, sizeof(nfs41_updowncall_entry));
    entry->xid = InterlockedIncrement64(&xid);
    entry->opcode = opcode;
    entry->state = NFS41_WAITING_FOR_UPCALL;
    entry->session = session;
    entry->open_state = open_state;
    entry->version = version;
    if (filename && filename->Length) entry->filename = filename;
    else if (filename && !filename->Length) entry->filename = (PUNICODE_STRING)&SLASH;
    else entry->filename = (PUNICODE_STRING)&EMPTY_STRING;
    /*XXX KeInitializeEvent will bugcheck under verifier if allocated
     * from PagedPool? */
    KeInitializeEvent(&entry->cond, SynchronizationEvent, FALSE);
    ExInitializeFastMutex(&entry->lock);

    if (clnt_sec_ctx == NULL) {
        SeCaptureSubjectContext(&sec_ctx);
        sec_qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        sec_qos.ImpersonationLevel = SecurityImpersonation;
        sec_qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        sec_qos.EffectiveOnly = 0;
        status = SeCreateClientSecurityFromSubjectContext(&sec_ctx, &sec_qos,
                    1, &entry->sec_ctx);
        if (status != STATUS_SUCCESS) {
            print_error("nfs41_UpcallCreate: "
                "SeCreateClientSecurityFromSubjectContext failed with %x\n",
                status);
            RxFreePool(entry);
        } else
            entry->psec_ctx = &entry->sec_ctx;
        SeReleaseSubjectContext(&sec_ctx);
    } else
        entry->psec_ctx = clnt_sec_ctx;

    *entry_out = entry;
out:
    return status;
}

NTSTATUS nfs41_UpcallWaitForReply(
    IN nfs41_updowncall_entry *entry,
    IN DWORD secs)
{
    NTSTATUS status = STATUS_SUCCESS;

    nfs41_AddEntry(upcallLock, upcall, entry);
    KeSetEvent(&upcallEvent, 0, FALSE);
    if (!entry->async_op) {
        LARGE_INTEGER timeout;
        timeout.QuadPart = RELATIVE(SECONDS(secs));
        /* 02/03/2011 AGLO: it is not clear what the "right" waiting design
         * should be. Having non-interruptable waiting seems to be the right
         * approach. However, when things go wrong, the only wait to proceed
         * is a reboot (since "waits" are not interruptable we can't stop a
         * hung task. Having interruptable wait causes issues with security
         * context. For now, I'm making CLOSE non-interruptable but keeping
         * the rest interruptable so that we don't have to reboot all the time
         */
        /* 02/15/2011 cbodley: added NFS41_UNLOCK for the same reason. locking
         * tests were triggering an interrupted unlock, which led to a bugcheck
         * in CloseSrvOpen() */
#define MAKE_WAITONCLOSE_NONITERRUPTABLE
#ifdef MAKE_WAITONCLOSE_NONITERRUPTABLE
        if (entry->opcode == NFS41_CLOSE || entry->opcode == NFS41_UNLOCK)
            status = KeWaitForSingleObject(&entry->cond, Executive,
                        KernelMode, FALSE, &timeout);
        else {
            status = KeWaitForSingleObject(&entry->cond, Executive,
                        UserMode, TRUE, &timeout);
        }
        if (status != STATUS_SUCCESS) {
            print_wait_status(1, "[downcall]", status,
                opcode2string(entry->opcode), entry, entry->xid);
            if (status == STATUS_TIMEOUT)
                status = STATUS_NETWORK_UNREACHABLE;
        }
#else

        status = KeWaitForSingleObject(&entry->cond, Executive, KernelMode, FALSE, NULL);
#endif
        print_wait_status(0, "[downcall]", status, opcode2string(entry->opcode),
            entry, entry->xid);
    } else
        goto out;

    switch(status) {
    case STATUS_SUCCESS: break;
    case STATUS_USER_APC:
    case STATUS_ALERTED:
    default:
        ExAcquireFastMutex(&entry->lock);
        if (entry->state == NFS41_DONE_PROCESSING) {
            ExReleaseFastMutex(&entry->lock);
            break;
        }
        DbgP("[upcall] abandoning %s entry=%p xid=%lld\n",
            opcode2string(entry->opcode), entry, entry->xid);
        entry->state = NFS41_NOT_WAITING;
        ExReleaseFastMutex(&entry->lock);
        goto out;
    }
    nfs41_RemoveEntry(downcallLock, entry);
out:
    return status;
}

NTSTATUS nfs41_upcall(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    nfs41_updowncall_entry *entry = NULL;
    ULONG len = 0;
    PLIST_ENTRY pEntry;

process_upcall:
    nfs41_RemoveFirst(upcallLock, upcall, pEntry);
    if (pEntry) {
        entry = (nfs41_updowncall_entry *)CONTAINING_RECORD(pEntry,
                    nfs41_updowncall_entry, next);
        ExAcquireFastMutex(&entry->lock);
        nfs41_AddEntry(downcallLock, downcall, entry);
        status = handle_upcall(RxContext, entry, &len);
        if (status == STATUS_SUCCESS &&
                entry->state == NFS41_WAITING_FOR_UPCALL)
            entry->state = NFS41_WAITING_FOR_DOWNCALL;
        ExReleaseFastMutex(&entry->lock);
        if (status) {
            entry->status = status;
            KeSetEvent(&entry->cond, 0, FALSE);
            RxContext->InformationToReturn = 0;
        } else
            RxContext->InformationToReturn = len;
    }
    else {
        status = KeWaitForSingleObject(&upcallEvent, Executive, UserMode, TRUE,
            (PLARGE_INTEGER) NULL);
        print_wait_status(0, "[upcall]", status, NULL, NULL, 0);
        switch (status) {
        case STATUS_SUCCESS: goto process_upcall;
        case STATUS_USER_APC:
        case STATUS_ALERTED:
        default: goto out;
        }
    }
out:
    return status;
}

void unmarshal_nfs41_header(
    nfs41_updowncall_entry *tmp,
    unsigned char **buf)
{
    RtlZeroMemory(tmp, sizeof(nfs41_updowncall_entry));

    RtlCopyMemory(&tmp->xid, *buf, sizeof(tmp->xid));
    *buf += sizeof(tmp->xid);
    RtlCopyMemory(&tmp->opcode, *buf, sizeof(tmp->opcode));
    *buf += sizeof(tmp->opcode);
    RtlCopyMemory(&tmp->status, *buf, sizeof(tmp->status));
    *buf += sizeof(tmp->status);
    RtlCopyMemory(&tmp->errno, *buf, sizeof(tmp->errno));
    *buf += sizeof(tmp->errno);
#ifdef DEBUG_MARSHAL_HEADER
    DbgP("[downcall header] xid=%lld opcode=%s status=%d errno=%d\n", tmp->xid,
        opcode2string(tmp->opcode), tmp->status, tmp->errno);
#endif
}

void unmarshal_nfs41_mount(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    RtlCopyMemory(&cur->session, *buf, sizeof(HANDLE));
    *buf += sizeof(HANDLE);
    RtlCopyMemory(&cur->version, *buf, sizeof(DWORD));
    *buf += sizeof(DWORD);
    RtlCopyMemory(&cur->u.Mount.lease_time, *buf, sizeof(DWORD));
    *buf += sizeof(DWORD);
    RtlCopyMemory(cur->u.Mount.FsAttrs, *buf, sizeof(FILE_FS_ATTRIBUTE_INFORMATION));
#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("unmarshal_nfs41_mount: session pointer 0x%x version %d lease_time "
         "%d\n", cur->session, cur->version, cur->u.Mount.lease_time);
#endif
}

VOID unmarshal_nfs41_setattr(
    nfs41_updowncall_entry *cur,
    PULONGLONG dest_buf,
    unsigned char **buf)
{
    RtlCopyMemory(dest_buf, *buf, sizeof(ULONGLONG));
#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("unmarshal_nfs41_setattr: returned ChangeTime %llu\n", *dest_buf);
#endif
}

NTSTATUS unmarshal_nfs41_rw(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    NTSTATUS status = STATUS_SUCCESS;

    RtlCopyMemory(&cur->buf_len, *buf, sizeof(cur->buf_len));
    *buf += sizeof(cur->buf_len);
    RtlCopyMemory(&cur->ChangeTime, *buf, sizeof(ULONGLONG));
#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("unmarshal_nfs41_rw: returned len %lu ChangeTime %llu\n",
        cur->buf_len, cur->ChangeTime);
#endif
#if 1
    /* 08/27/2010: it looks like we really don't need to call
        * MmUnmapLockedPages() eventhough we called
        * MmMapLockedPagesSpecifyCache() as the MDL passed to us
        * is already locked.
        */
    _SEH2_TRY {
        MmUnmapLockedPages(cur->buf, cur->u.ReadWrite.MdlAddress);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        NTSTATUS code;
        code = _SEH2_GetExceptionCode();
        print_error("Call to MmUnmapLockedPages failed due to"
            " exception 0x%0x\n", code);
        status = STATUS_ACCESS_DENIED;
    } _SEH2_END;
#endif
    return status;
}

NTSTATUS unmarshal_nfs41_open(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    NTSTATUS status = STATUS_SUCCESS;

    _SEH2_TRY {
        if (cur->u.Open.EaBuffer)
            MmUnmapLockedPages(cur->u.Open.EaBuffer, cur->u.Open.EaMdl);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        print_error("MmUnmapLockedPages thrown exception=0x%0x\n", _SEH2_GetExceptionCode());
        status = cur->status = STATUS_ACCESS_DENIED;
        goto out;
    } _SEH2_END;

    RtlCopyMemory(&cur->u.Open.binfo, *buf, sizeof(FILE_BASIC_INFORMATION));
    *buf += sizeof(FILE_BASIC_INFORMATION);
    RtlCopyMemory(&cur->u.Open.sinfo, *buf, sizeof(FILE_STANDARD_INFORMATION));
    *buf += sizeof(FILE_STANDARD_INFORMATION);
    RtlCopyMemory(&cur->open_state, *buf, sizeof(HANDLE));
    *buf += sizeof(HANDLE);
    RtlCopyMemory(&cur->u.Open.mode, *buf, sizeof(DWORD));
    *buf += sizeof(DWORD);
    RtlCopyMemory(&cur->ChangeTime, *buf, sizeof(ULONGLONG));
    *buf += sizeof(ULONGLONG);
    RtlCopyMemory(&cur->u.Open.deleg_type, *buf, sizeof(DWORD));
    *buf += sizeof(DWORD);
    if (cur->errno == ERROR_REPARSE) {
        RtlCopyMemory(&cur->u.Open.symlink_embedded, *buf, sizeof(BOOLEAN));
        *buf += sizeof(BOOLEAN);
        RtlCopyMemory(&cur->u.Open.symlink.MaximumLength, *buf,
            sizeof(USHORT));
        *buf += sizeof(USHORT);
        cur->u.Open.symlink.Length = cur->u.Open.symlink.MaximumLength -
            sizeof(WCHAR);
        cur->u.Open.symlink.Buffer = RxAllocatePoolWithTag(NonPagedPool,
            cur->u.Open.symlink.MaximumLength, NFS41_MM_POOLTAG);
        if (cur->u.Open.symlink.Buffer == NULL) {
            cur->status = STATUS_INSUFFICIENT_RESOURCES;
            status = STATUS_UNSUCCESSFUL;
            goto out;
        }
        RtlCopyMemory(cur->u.Open.symlink.Buffer, *buf,
            cur->u.Open.symlink.MaximumLength);
#ifdef DEBUG_MARSHAL_DETAIL
        DbgP("unmarshal_nfs41_open: ERROR_REPARSE -> '%wZ'\n", &cur->u.Open.symlink);
#endif
    }
#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("unmarshal_nfs41_open: open_state 0x%x mode %o changeattr %llu "
        "deleg_type %d\n", cur->open_state, cur->u.Open.mode,
        cur->ChangeTime, cur->u.Open.deleg_type);
#endif
out:
    return status;
}

NTSTATUS unmarshal_nfs41_dirquery(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG buf_len;

    RtlCopyMemory(&buf_len, *buf, sizeof(ULONG));
#ifdef DEBUG_MARSHAL_DETAIL
    DbgP("unmarshal_nfs41_dirquery: reply size %d\n", buf_len);
#endif
    *buf += sizeof(ULONG);
    _SEH2_TRY {
        MmUnmapLockedPages(cur->u.QueryFile.mdl_buf, cur->u.QueryFile.mdl);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        NTSTATUS code;
        code = _SEH2_GetExceptionCode();
        print_error("MmUnmapLockedPages thrown exception=0x%0x\n", code);
        status = STATUS_ACCESS_DENIED;
    } _SEH2_END;
    if (buf_len > cur->buf_len)
        cur->status = STATUS_BUFFER_TOO_SMALL;
    cur->buf_len = buf_len;

    return status;
}

void unmarshal_nfs41_attrget(
    nfs41_updowncall_entry *cur,
    PVOID attr_value,
    ULONG *attr_len,
    unsigned char **buf)
{
    ULONG buf_len;
    RtlCopyMemory(&buf_len, *buf, sizeof(ULONG));
    if (buf_len > *attr_len) {
        cur->status = STATUS_BUFFER_TOO_SMALL;
        return;
    }
    *buf += sizeof(ULONG);
    *attr_len = buf_len;
    RtlCopyMemory(attr_value, *buf, buf_len);
    *buf += buf_len;
}

void unmarshal_nfs41_eaget(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    RtlCopyMemory(&cur->u.QueryEa.Overflow, *buf, sizeof(ULONG));
    *buf += sizeof(ULONG);
    RtlCopyMemory(&cur->buf_len, *buf, sizeof(ULONG));
    *buf += sizeof(ULONG);
    if (cur->u.QueryEa.Overflow != ERROR_INSUFFICIENT_BUFFER) {
        RtlCopyMemory(cur->buf, *buf, cur->buf_len);
        *buf += cur->buf_len;
    }
}

void unmarshal_nfs41_getattr(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    unmarshal_nfs41_attrget(cur, cur->buf, &cur->buf_len, buf);
    RtlCopyMemory(&cur->ChangeTime, *buf, sizeof(LONGLONG));
#ifdef DEBUG_MARSHAL_DETAIL
    if (cur->u.QueryFile.InfoClass == FileBasicInformation)
        DbgP("[unmarshal_nfs41_getattr] ChangeTime %llu\n", cur->ChangeTime);
#endif
}

NTSTATUS unmarshal_nfs41_getacl(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD buf_len;

    RtlCopyMemory(&buf_len, *buf, sizeof(DWORD));
    *buf += sizeof(DWORD);
    cur->buf = RxAllocatePoolWithTag(NonPagedPool,
        buf_len, NFS41_MM_POOLTAG_ACL);
    if (cur->buf == NULL) {
        cur->status = status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    RtlCopyMemory(cur->buf, *buf, buf_len);
    if (buf_len > cur->buf_len)
        cur->status = STATUS_BUFFER_TOO_SMALL;
    cur->buf_len = buf_len;

out:
    return status;
}

void unmarshal_nfs41_symlink(
    nfs41_updowncall_entry *cur,
    unsigned char **buf)
{
    if (cur->u.Symlink.set) return;

    RtlCopyMemory(&cur->u.Symlink.target->Length, *buf, sizeof(USHORT));
    *buf += sizeof(USHORT);
    if (cur->u.Symlink.target->Length >
            cur->u.Symlink.target->MaximumLength) {
        cur->status = STATUS_BUFFER_TOO_SMALL;
        return;
    }
    RtlCopyMemory(cur->u.Symlink.target->Buffer, *buf,
        cur->u.Symlink.target->Length);
    cur->u.Symlink.target->Length -= sizeof(UNICODE_NULL);
}

NTSTATUS nfs41_downcall(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG in_len = LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    unsigned char *buf = LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    PLIST_ENTRY pEntry;
    nfs41_updowncall_entry *tmp, *cur= NULL;
    BOOLEAN found = 0;

    print_hexbuf(0, (unsigned char *)"downcall buffer", buf, in_len);

    tmp = RxAllocatePoolWithTag(NonPagedPool, sizeof(nfs41_updowncall_entry),
            NFS41_MM_POOLTAG_DOWN);
    if (tmp == NULL) goto out;

    unmarshal_nfs41_header(tmp, &buf);

    ExAcquireFastMutex(&downcallLock);
    pEntry = &downcall.head;
    pEntry = pEntry->Flink;
    while (pEntry != NULL) {
        cur = (nfs41_updowncall_entry *)CONTAINING_RECORD(pEntry,
                nfs41_updowncall_entry, next);
        if (cur->xid == tmp->xid) {
            found = 1;
            break;
        }
        if (pEntry->Flink == &downcall.head)
            break;
        pEntry = pEntry->Flink;
    }
    ExReleaseFastMutex(&downcallLock);
    SeStopImpersonatingClient();
    if (!found) {
        print_error("Didn't find xid=%lld entry\n", tmp->xid);
        goto out_free;
    }

    ExAcquireFastMutex(&cur->lock);
    if (cur->state == NFS41_NOT_WAITING) {
        DbgP("[downcall] Nobody is waiting for this request!!!\n");
        switch(cur->opcode) {
        case NFS41_WRITE:
        case NFS41_READ:
            MmUnmapLockedPages(cur->buf, cur->u.ReadWrite.MdlAddress);
            break;
        case NFS41_DIR_QUERY:
            MmUnmapLockedPages(cur->u.QueryFile.mdl_buf,
                    cur->u.QueryFile.mdl);
            IoFreeMdl(cur->u.QueryFile.mdl);
            break;
        case NFS41_OPEN:
            if (cur->u.Open.EaMdl) {
                MmUnmapLockedPages(cur->u.Open.EaBuffer,
                        cur->u.Open.EaMdl);
                IoFreeMdl(cur->u.Open.EaMdl);
            }
            break;
        }
        ExReleaseFastMutex(&cur->lock);
        nfs41_RemoveEntry(downcallLock, cur);
        RxFreePool(cur);
        status = STATUS_UNSUCCESSFUL;
        goto out_free;
    }
    cur->state = NFS41_DONE_PROCESSING;
    cur->status = tmp->status;
    cur->errno = tmp->errno;
    status = STATUS_SUCCESS;

    if (!tmp->status) {
        switch (tmp->opcode) {
        case NFS41_MOUNT:
            unmarshal_nfs41_mount(cur, &buf);
            break;
        case NFS41_WRITE:
        case NFS41_READ:
            status = unmarshal_nfs41_rw(cur, &buf);
            break;
        case NFS41_OPEN:
            status = unmarshal_nfs41_open(cur, &buf);
            break;
        case NFS41_DIR_QUERY:
            status = unmarshal_nfs41_dirquery(cur, &buf);
            break;
        case NFS41_FILE_QUERY:
            unmarshal_nfs41_getattr(cur, &buf);
            break;
        case NFS41_EA_GET:
            unmarshal_nfs41_eaget(cur, &buf);
            break;
        case NFS41_SYMLINK:
            unmarshal_nfs41_symlink(cur, &buf);
            break;
        case NFS41_VOLUME_QUERY:
            unmarshal_nfs41_attrget(cur, cur->buf, &cur->buf_len, &buf);
            break;
        case NFS41_ACL_QUERY:
            status = unmarshal_nfs41_getacl(cur, &buf);
            break;
        case NFS41_FILE_SET:
            unmarshal_nfs41_setattr(cur, &cur->ChangeTime, &buf);
            break;
        case NFS41_EA_SET:
            unmarshal_nfs41_setattr(cur, &cur->ChangeTime, &buf);
            break;
        case NFS41_ACL_SET:
            unmarshal_nfs41_setattr(cur, &cur->ChangeTime, &buf);
            break;
        }
    }
    ExReleaseFastMutex(&cur->lock);
    if (cur->async_op) {
        if (cur->status == STATUS_SUCCESS) {
            cur->u.ReadWrite.rxcontext->StoredStatus = STATUS_SUCCESS;
            cur->u.ReadWrite.rxcontext->InformationToReturn =
                cur->buf_len;
        } else {
            cur->u.ReadWrite.rxcontext->StoredStatus =
                map_readwrite_errors(cur->status);
            cur->u.ReadWrite.rxcontext->InformationToReturn = 0;
        }
        nfs41_RemoveEntry(downcallLock, cur);
        RxLowIoCompletion(cur->u.ReadWrite.rxcontext);
        RxFreePool(cur);
    } else
        KeSetEvent(&cur->cond, 0, FALSE);

out_free:
    RxFreePool(tmp);
out:
    return status;
}

NTSTATUS nfs41_shutdown_daemon(
    DWORD version)
{
    NTSTATUS status = STATUS_SUCCESS;
    nfs41_updowncall_entry *entry = NULL;

    DbgEn();
    status = nfs41_UpcallCreate(NFS41_SHUTDOWN, NULL, INVALID_HANDLE_VALUE,
        INVALID_HANDLE_VALUE, version, NULL, &entry);
    if (status) goto out;

    status = nfs41_UpcallWaitForReply(entry, UPCALL_TIMEOUT_DEFAULT);
    SeDeleteClientSecurity(&entry->sec_ctx);
    if (status) goto out;

    RxFreePool(entry);
out:
    DbgEx();
    return status;
}

NTSTATUS SharedMemoryInit(
    OUT PHANDLE phSection)
{
    NTSTATUS status;
    HANDLE hSection;
    UNICODE_STRING SectionName;
    SECURITY_DESCRIPTOR SecurityDesc;
    OBJECT_ATTRIBUTES SectionAttrs;
    LARGE_INTEGER nSectionSize;

    DbgEn();

    RtlInitUnicodeString(&SectionName, NFS41_SHARED_MEMORY_NAME);

    /* XXX: setting dacl=NULL grants access to everyone */
    status = RtlCreateSecurityDescriptor(&SecurityDesc,
        SECURITY_DESCRIPTOR_REVISION);
    if (status) {
        print_error("RtlCreateSecurityDescriptor() failed with %08X\n", status);
        goto out;
    }
    status = RtlSetDaclSecurityDescriptor(&SecurityDesc, TRUE, NULL, FALSE);
    if (status) {
        print_error("RtlSetDaclSecurityDescriptor() failed with %08X\n", status);
        goto out;
    }

    InitializeObjectAttributes(&SectionAttrs, &SectionName,
        0, NULL, &SecurityDesc);

    nSectionSize.QuadPart = sizeof(NFS41NP_SHARED_MEMORY);

    status = ZwCreateSection(&hSection, SECTION_MAP_READ | SECTION_MAP_WRITE,
        &SectionAttrs, &nSectionSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    switch (status) {
    case STATUS_SUCCESS:
        break;
    case STATUS_OBJECT_NAME_COLLISION:
        DbgP("section already created; returning success\n");
        status = STATUS_SUCCESS;
        goto out;
    default:
        DbgP("ZwCreateSection failed with %08X\n", status);
        goto out;
    }
out:
    DbgEx();
    return status;
}

NTSTATUS SharedMemoryFree(
    IN HANDLE hSection)
{
    NTSTATUS status;
    DbgEn();
    status = ZwClose(hSection);
    DbgEx();
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Start(
#else
NTSTATUS nfs41_Start(
#endif
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT dev)
{
    NTSTATUS status;
    NFS41GetDeviceExtension(RxContext, DevExt);

    DbgEn();

    status = SharedMemoryInit(&DevExt->SharedMemorySection);
    if (status) {
        print_error("InitSharedMemory failed with %08X\n", status);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    InterlockedCompareExchange((PLONG)&nfs41_start_state,
        NFS41_START_DRIVER_STARTED,
        NFS41_START_DRIVER_START_IN_PROGRESS);
out:
    DbgEx();
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Stop(
#else
NTSTATUS nfs41_Stop(
#endif
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT dev)
{
    NTSTATUS status;
    NFS41GetDeviceExtension(RxContext, DevExt);
    DbgEn();
    status = SharedMemoryFree(DevExt->SharedMemorySection);
    DbgEx();
    return status;
}

NTSTATUS GetConnectionHandle(
    IN PUNICODE_STRING ConnectionName,
    IN PVOID EaBuffer,
    IN ULONG EaLength,
    OUT PHANDLE Handle)
{
    NTSTATUS status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;

#ifdef DEBUG_MOUNT
    DbgEn();
#endif
    InitializeObjectAttributes(&ObjectAttributes, ConnectionName,
        OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

    print_error("Len %d Buf %p\n", EaLength, EaBuffer);

    status = ZwCreateFile(Handle, SYNCHRONIZE, &ObjectAttributes,
        &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        EaBuffer, EaLength);

#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

NTSTATUS nfs41_GetConnectionInfoFromBuffer(
    IN PVOID Buffer,
    IN ULONG BufferLen,
    OUT PUNICODE_STRING pConnectionName,
    OUT PVOID *ppEaBuffer,
    OUT PULONG pEaLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT NameLength, EaPadding;
    ULONG EaLength, BufferLenExpected;
    PBYTE ptr;

    /* make sure buffer is at least big enough for header */
    if (BufferLen < sizeof(USHORT) + sizeof(USHORT) + sizeof(ULONG)) {
        status = STATUS_BAD_NETWORK_NAME;
        print_error("Invalid input buffer.\n");
        pConnectionName->Length = pConnectionName->MaximumLength = 0;
        *ppEaBuffer = NULL;
        *pEaLength = 0;
        goto out;
    }

    ptr = Buffer;
    NameLength = *(PUSHORT)ptr;
    ptr += sizeof(USHORT);
    EaPadding = *(PUSHORT)ptr;
    ptr += sizeof(USHORT);
    EaLength = *(PULONG)ptr;
    ptr += sizeof(ULONG);

    /* validate buffer length */
    BufferLenExpected = sizeof(USHORT) + sizeof(USHORT) + sizeof(ULONG) +
        NameLength + EaPadding + EaLength;
    if (BufferLen != BufferLenExpected) {
        status = STATUS_BAD_NETWORK_NAME;
        print_error("Received buffer of length %lu, but expected %lu bytes.\n",
            BufferLen, BufferLenExpected);
        pConnectionName->Length = pConnectionName->MaximumLength = 0;
        *ppEaBuffer = NULL;
        *pEaLength = 0;
        goto out;
    }

    pConnectionName->Buffer = (PWCH)ptr;
    pConnectionName->Length = NameLength - sizeof(WCHAR);
    pConnectionName->MaximumLength = NameLength;

    if (EaLength)
        *ppEaBuffer = ptr + NameLength + EaPadding;
    else
        *ppEaBuffer = NULL;
    *pEaLength = EaLength;

out:
    return status;
}

NTSTATUS nfs41_CreateConnection(
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE Handle = INVALID_HANDLE_VALUE;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PVOID Buffer = LowIoContext->ParamsFor.IoCtl.pInputBuffer, EaBuffer;
    ULONG BufferLen = LowIoContext->ParamsFor.IoCtl.InputBufferLength, EaLength;
    UNICODE_STRING FileName;
    BOOLEAN Wait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);

#ifdef DEBUG_MOUNT
    DbgEn();
#endif

    if (!Wait) {
        //just post right now!
        DbgP("returning STATUS_PENDING\n");
        *PostToFsp = TRUE;
        status = STATUS_PENDING;
        goto out;
    }

    status = nfs41_GetConnectionInfoFromBuffer(Buffer, BufferLen,
        &FileName, &EaBuffer, &EaLength);
    if (status != STATUS_SUCCESS)
        goto out;

    status = GetConnectionHandle(&FileName, EaBuffer, EaLength, &Handle);
    if (!status && Handle != INVALID_HANDLE_VALUE)
        ZwClose(Handle);
out:
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

#ifdef ENABLE_TIMINGS
void print_op_stat(
    const char *op_str,
    nfs41_timings *time, BOOLEAN clear)
{
    DbgP("%-9s: num_ops=%-10d delta_ticks=%-10d size=%-10d\n", op_str,
        time->tops, time->tops ? time->ticks/time->tops : 0,
        time->sops ? time->size/time->sops : 0);
    if (clear) {
        time->tops = 0;
        time->ticks = 0;
        time->size = 0;
        time->sops = 0;
    }
}
#endif
NTSTATUS nfs41_unmount(
    HANDLE session,
    DWORD version,
    DWORD timeout)
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    nfs41_updowncall_entry *entry;

#ifdef DEBUG_MOUNT
    DbgEn();
#endif
    status = nfs41_UpcallCreate(NFS41_UNMOUNT, NULL, session,
        INVALID_HANDLE_VALUE, version, NULL, &entry);
    SeDeleteClientSecurity(&entry->sec_ctx);
    if (status) goto out;

    nfs41_UpcallWaitForReply(entry, timeout);
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    print_op_stat("lookup", &lookup, 1);
    print_op_stat("open", &open, 1);
    print_op_stat("close", &close, 1);
    print_op_stat("volume", &volume, 1);
    print_op_stat("getattr", &getattr, 1);
    print_op_stat("setattr", &setattr, 1);
    print_op_stat("getexattr", &getexattr, 1);
    print_op_stat("setexattr", &setexattr, 1);
    print_op_stat("readdir", &readdir, 1);
    print_op_stat("getacl", &getacl, 1);
    print_op_stat("setacl", &setacl, 1);
    print_op_stat("read", &read, 1);
    print_op_stat("write", &write, 1);
    print_op_stat("lock", &lock, 1);
    print_op_stat("unlock", &unlock, 1);
#endif
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

NTSTATUS nfs41_DeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PWCHAR ConnectName = LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    ULONG ConnectNameLen = LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    HANDLE Handle;
    UNICODE_STRING FileName;
    PFILE_OBJECT pFileObject;
    BOOLEAN Wait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);

#ifdef DEBUG_MOUNT
    DbgEn();
#endif

    if (!Wait) {
        //just post right now!
        *PostToFsp = TRUE;
        DbgP("returning STATUS_PENDING\n");
        status = STATUS_PENDING;
        goto out;
    }

    FileName.Buffer = ConnectName;
    FileName.Length = (USHORT) ConnectNameLen - sizeof(WCHAR);
    FileName.MaximumLength = (USHORT) ConnectNameLen;

    status = GetConnectionHandle(&FileName, NULL, 0, &Handle);
    if (status != STATUS_SUCCESS)
        goto out;

    status = ObReferenceObjectByHandle(Handle, 0L, NULL, KernelMode,
                (PVOID *)&pFileObject, NULL);
    if (NT_SUCCESS(status)) {
        PV_NET_ROOT VNetRoot;

        // VNetRoot exists as FOBx in the FsContext2
        VNetRoot = (PV_NET_ROOT) pFileObject->FsContext2;
        // make sure the node looks right
        if (NodeType(VNetRoot) == RDBSS_NTC_V_NETROOT)
        {
#ifdef DEBUG_MOUNT
            DbgP("Calling RxFinalizeConnection for NetRoot %p from VNetRoot %p\n",
                VNetRoot->NetRoot, VNetRoot);
#endif
            status = RxFinalizeConnection(VNetRoot->NetRoot, VNetRoot, TRUE);
        }
        else
            status = STATUS_BAD_NETWORK_NAME;

        ObDereferenceObject(pFileObject);
    }
    ZwClose(Handle);
out:
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_DevFcbXXXControlFile(
#else
NTSTATUS nfs41_DevFcbXXXControlFile(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    UCHAR op = RxContext->MajorFunction;
    PLOWIO_CONTEXT io_ctx = &RxContext->LowIoContext;
    ULONG fsop = io_ctx->ParamsFor.FsCtl.FsControlCode, state;
    ULONG in_len = io_ctx->ParamsFor.IoCtl.InputBufferLength;
    DWORD *buf = io_ctx->ParamsFor.IoCtl.pInputBuffer;
    NFS41GetDeviceExtension(RxContext, DevExt);
    DWORD nfs41d_version = 0;

    //DbgEn();

    print_ioctl(0, op);
    switch(op) {
    case IRP_MJ_FILE_SYSTEM_CONTROL:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        print_fs_ioctl(0, fsop);
        switch (fsop) {
        case IOCTL_NFS41_INVALCACHE:
            nfs41_invalidate_cache(RxContext);
            status = STATUS_SUCCESS;
            break;
        case IOCTL_NFS41_READ:
            status = nfs41_upcall(RxContext);
            break;
        case IOCTL_NFS41_WRITE:
            status = nfs41_downcall(RxContext);
            break;
        case IOCTL_NFS41_ADDCONN:
            status = nfs41_CreateConnection(RxContext, &RxContext->PostRequest);
            break;
        case IOCTL_NFS41_DELCONN:
            if (RxContext->RxDeviceObject->NumberOfActiveFcbs > 0) {
                DbgP("device has open handles %d\n",
                    RxContext->RxDeviceObject->NumberOfActiveFcbs);
#ifdef __REACTOS__
                if (RxContext->RxDeviceObject->pRxNetNameTable != NULL)
                {
#define DUMP_FCB_TABLE_FROM_NETROOT(N)                                               \
{                                                                                    \
    USHORT Bucket2;                                                                  \
    BOOLEAN Release2 = FALSE;                                                        \
    if (!RxIsFcbTableLockAcquired(&(N)->FcbTable))                                   \
    {                                                                                \
        RxAcquireFcbTableLockExclusive(&(N)->FcbTable, TRUE);                        \
        Release2 = TRUE;                                                             \
    }                                                                                \
    for (Bucket2 = 0; Bucket2 < (N)->FcbTable.NumberOfBuckets; ++Bucket2)            \
    {                                                                                \
        PLIST_ENTRY Entry2;                                                          \
        for (Entry2 = (N)->FcbTable.HashBuckets[Bucket2].Flink;                      \
             Entry2 != &(N)->FcbTable.HashBuckets[Bucket2];                          \
             Entry2 = Entry2->Flink)                                                 \
        {                                                                            \
            PFCB Fcb;                                                                \
            Fcb = CONTAINING_RECORD(Entry2, FCB, FcbTableEntry.HashLinks);           \
            DbgP("Fcb: %p still has %d references\n", Fcb, Fcb->NodeReferenceCount); \
            DbgP("It is for: %wZ\n", &Fcb->FcbTableEntry.Path);                      \
        }                                                                            \
    }                                                                                \
    if (Release2)                                                                    \
    {                                                                                \
        RxReleaseFcbTableLock(&(N)->FcbTable);                                       \
    }                                                                                \
}
                    USHORT Bucket;
                    BOOLEAN Release = FALSE;

                    if (!RxIsPrefixTableLockAcquired(RxContext->RxDeviceObject->pRxNetNameTable))
                    {
                        RxAcquirePrefixTableLockExclusive(RxContext->RxDeviceObject->pRxNetNameTable, TRUE);
                        Release = TRUE;
                    }

                    for (Bucket = 0; Bucket < RxContext->RxDeviceObject->pRxNetNameTable->TableSize; ++Bucket)
                    {
                        PLIST_ENTRY Entry;

                        for (Entry = RxContext->RxDeviceObject->pRxNetNameTable->HashBuckets[Bucket].Flink;
                             Entry != &RxContext->RxDeviceObject->pRxNetNameTable->HashBuckets[Bucket];
                             Entry = Entry->Flink)
                        {
                            PVOID Container;

                            Container = CONTAINING_RECORD(Entry, RX_PREFIX_ENTRY, HashLinks)->ContainingRecord;
                            switch (NodeType(Container) & ~RX_SCAVENGER_MASK)
                            {
                                case RDBSS_NTC_NETROOT:
                                {
                                    PNET_ROOT NetRoot;

                                    NetRoot = Container;
                                    DUMP_FCB_TABLE_FROM_NETROOT(NetRoot);
                                    break;
                                }

                                case RDBSS_NTC_V_NETROOT:
                                {
                                    PV_NET_ROOT VNetRoot;

                                    VNetRoot = Container;
                                    if (VNetRoot->NetRoot != NULL)
                                    {
                                        PNET_ROOT NetRoot;

                                        NetRoot = VNetRoot->NetRoot;
                                        DUMP_FCB_TABLE_FROM_NETROOT(NetRoot);
                                    }
                                    break;
                                }

                                default:
                                {
                                    DbgP("Other node found: %x\n", NodeType(Container));
                                    break;
                                }
                            }
                        }
                    }

                    if (Release)
                    {
                        RxReleasePrefixTableLock(RxContext->RxDeviceObject->pRxNetNameTable);
                    }
#undef DUMP_FCB_TABLE_FROM_NETROOT
                }
                else
                {
                    DbgP("RxNetNameTable is NULL for: %p\n", RxContext->RxDeviceObject);
                }
#endif
                status = STATUS_REDIRECTOR_HAS_OPEN_HANDLES;
                break;
            }
            status = nfs41_DeleteConnection(RxContext, &RxContext->PostRequest);
            break;
        case IOCTL_NFS41_GETSTATE:
            state = RDR_NULL_STATE;

            if (io_ctx->ParamsFor.IoCtl.OutputBufferLength >= sizeof(ULONG)) {
                // map the states to control app's equivalents
                print_driver_state(nfs41_start_state);
                switch (nfs41_start_state) {
                case NFS41_START_DRIVER_STARTABLE:
                case NFS41_START_DRIVER_STOPPED:
                    state = RDR_STOPPED;
                    break;
                case NFS41_START_DRIVER_START_IN_PROGRESS:
                    state = RDR_STARTING;
                    break;
                case NFS41_START_DRIVER_STARTED:
                    state = RDR_STARTED;
                    break;
                }
                *(ULONG *)io_ctx->ParamsFor.IoCtl.pOutputBuffer = state;
                RxContext->InformationToReturn = sizeof(ULONG);
                status = STATUS_SUCCESS;
            } else
                status = STATUS_INVALID_PARAMETER;
            break;
        case IOCTL_NFS41_START:
            print_driver_state(nfs41_start_state);
            if (in_len >= sizeof(DWORD)) {
                RtlCopyMemory(&nfs41d_version, buf, sizeof(DWORD));
                DbgP("NFS41 Daemon sent start request with version %d\n",
                    nfs41d_version);
                DbgP("Currently used NFS41 Daemon version is %d\n",
                    DevExt->nfs41d_version);
                DevExt->nfs41d_version = nfs41d_version;
            }
            switch(nfs41_start_state) {
            case NFS41_START_DRIVER_STARTABLE:
                (nfs41_start_driver_state)InterlockedCompareExchange(
                              (PLONG)&nfs41_start_state,
                              NFS41_START_DRIVER_START_IN_PROGRESS,
                              NFS41_START_DRIVER_STARTABLE);
                    //lack of break is intentional
            case NFS41_START_DRIVER_START_IN_PROGRESS:
                status = RxStartMinirdr(RxContext, &RxContext->PostRequest);
                if (status == STATUS_REDIRECTOR_STARTED) {
                    DbgP("redirector started\n");
                    status = STATUS_SUCCESS;
                } else if (status == STATUS_PENDING &&
                            RxContext->PostRequest == TRUE) {
                    DbgP("RxStartMinirdr pending %08lx\n", status);
                    status = STATUS_MORE_PROCESSING_REQUIRED;
                }
                break;
            case NFS41_START_DRIVER_STARTED:
                status = STATUS_SUCCESS;
                break;
            default:
                status = STATUS_INVALID_PARAMETER;
            }
            break;
        case IOCTL_NFS41_STOP:
            if (nfs41_start_state == NFS41_START_DRIVER_STARTED)
                nfs41_shutdown_daemon(DevExt->nfs41d_version);
            if (RxContext->RxDeviceObject->NumberOfActiveFcbs > 0) {
                DbgP("device has open handles %d\n",
                    RxContext->RxDeviceObject->NumberOfActiveFcbs);
                status = STATUS_REDIRECTOR_HAS_OPEN_HANDLES;
                break;
            }

            state = (nfs41_start_driver_state)InterlockedCompareExchange(
                        (PLONG)&nfs41_start_state,
                        NFS41_START_DRIVER_STARTABLE,
                        NFS41_START_DRIVER_STARTED);

            status = RxStopMinirdr(RxContext, &RxContext->PostRequest);
            DbgP("RxStopMinirdr status %08lx\n", status);
            if (status == STATUS_PENDING && RxContext->PostRequest == TRUE )
                status = STATUS_MORE_PROCESSING_REQUIRED;
            break;
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
        };
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
    };

    //DbgEx();
    return status;
}

#ifndef __REACTOS__
NTSTATUS _nfs41_CreateSrvCall(
    PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext)
{
#else
NTSTATUS NTAPI _nfs41_CreateSrvCall(
    PVOID pContext)
{
    PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext = pContext;
#endif
    NTSTATUS status = STATUS_SUCCESS;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = pCallbackContext;
    PMRX_SRV_CALL pSrvCall;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure =
        (PMRX_SRVCALLDOWN_STRUCTURE)(SCCBC->SrvCalldownStructure);
    PNFS41_SERVER_ENTRY pServerEntry = NULL;

#ifdef DEBUG_MOUNT
    DbgEn();
#endif

    pSrvCall = SrvCalldownStructure->SrvCall;

    ASSERT( pSrvCall );
    ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );
    print_srv_call(0, pSrvCall);

    // validate the server name with the test name of 'pnfs'
#ifdef DEBUG_MOUNT
    DbgP("SrvCall: Connection Name Length: %d %wZ\n",
        pSrvCall->pSrvCallName->Length, pSrvCall->pSrvCallName);
#endif

    if (pSrvCall->pSrvCallName->Length > SERVER_NAME_BUFFER_SIZE) {
        print_error("Server name '%wZ' too long for server entry (max %u)\n",
            pSrvCall->pSrvCallName, SERVER_NAME_BUFFER_SIZE);
        status = STATUS_NAME_TOO_LONG;
        goto out;
    }

    /* Let's create our own representation of the server */
    pServerEntry = (PNFS41_SERVER_ENTRY)RxAllocatePoolWithTag(PagedPool,
        sizeof(NFS41_SERVER_ENTRY), NFS41_MM_POOLTAG);
    if (pServerEntry == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    RtlZeroMemory(pServerEntry, sizeof(NFS41_SERVER_ENTRY));

    pServerEntry->Name.Buffer = pServerEntry->NameBuffer;
    pServerEntry->Name.Length = pSrvCall->pSrvCallName->Length;
    pServerEntry->Name.MaximumLength = SERVER_NAME_BUFFER_SIZE;
    RtlCopyMemory(pServerEntry->Name.Buffer, pSrvCall->pSrvCallName->Buffer,
        pServerEntry->Name.Length);

    pCallbackContext->RecommunicateContext = pServerEntry;
#ifdef __REACTOS__
    InterlockedExchangePointer((void * volatile *)&pServerEntry->pRdbssSrvCall, pSrvCall);
#else
    InterlockedExchangePointer(&pServerEntry->pRdbssSrvCall, pSrvCall);
#endif

out:
    SCCBC->Status = status;
    SrvCalldownStructure->CallBack(SCCBC);

#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
VOID NTAPI _nfs41_CreateSrvCall_v(
    PVOID pCallbackContext)
{
    _nfs41_CreateSrvCall(pCallbackContext);
}
#endif

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_CreateSrvCall(
#else
NTSTATUS nfs41_CreateSrvCall(
#endif
    PMRX_SRV_CALL pSrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext)
{
    NTSTATUS status;

    ASSERT( pSrvCall );
    ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

    if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
        DbgP("executing with RDBSS context\n");
        status = _nfs41_CreateSrvCall(pCallbackContext);
    } else {
        status = RxDispatchToWorkerThread(nfs41_dev, DelayedWorkQueue,
#ifdef __REACTOS__
            _nfs41_CreateSrvCall_v, pCallbackContext);
#else
            _nfs41_CreateSrvCall, pCallbackContext);
#endif
        if (status != STATUS_SUCCESS) {
            print_error("RxDispatchToWorkerThread returned status %08lx\n",
                status);
            pCallbackContext->Status = status;
            pCallbackContext->SrvCalldownStructure->CallBack(pCallbackContext);
            status = STATUS_PENDING;
        }
    }
    /* RDBSS expects MRxCreateSrvCall to return STATUS_PENDING */
    if (status == STATUS_SUCCESS)
        status = STATUS_PENDING;

    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_SrvCallWinnerNotify(
#else
NTSTATUS nfs41_SrvCallWinnerNotify(
#endif
    IN OUT PMRX_SRV_CALL pSrvCall,
    IN BOOLEAN ThisMinirdrIsTheWinner,
    IN OUT PVOID pSrvCallContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFS41_SERVER_ENTRY pServerEntry;

    pServerEntry = (PNFS41_SERVER_ENTRY)pSrvCallContext;

    if (!ThisMinirdrIsTheWinner) {
        ASSERT(1);
        goto out;
    }

    pSrvCall->Context = pServerEntry;
out:
    return status;
}

NTSTATUS map_mount_errors(
    DWORD status)
{
    switch (status) {
    case NO_ERROR:              return STATUS_SUCCESS;
    case ERROR_NETWORK_UNREACHABLE: return STATUS_NETWORK_UNREACHABLE;
    case ERROR_BAD_NET_RESP:    return STATUS_UNEXPECTED_NETWORK_ERROR;
    case ERROR_BAD_NET_NAME:    return STATUS_BAD_NETWORK_NAME;
    case ERROR_BAD_NETPATH:     return STATUS_BAD_NETWORK_PATH;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INSUFFICIENT_RESOURCES\n", status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

NTSTATUS nfs41_mount(
    PNFS41_MOUNT_CONFIG config,
    DWORD sec_flavor,
    PHANDLE session,
    DWORD *version,
    PFILE_FS_ATTRIBUTE_INFORMATION FsAttrs)
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    nfs41_updowncall_entry *entry;

#ifdef DEBUG_MOUNT
    DbgEn();
    DbgP("Server Name %wZ Mount Point %wZ SecFlavor %d\n",
        &config->SrvName, &config->MntPt, sec_flavor);
#endif
    status = nfs41_UpcallCreate(NFS41_MOUNT, NULL, *session,
        INVALID_HANDLE_VALUE, *version, &config->MntPt, &entry);
    if (status) goto out;

    entry->u.Mount.srv_name = &config->SrvName;
    entry->u.Mount.root = &config->MntPt;
    entry->u.Mount.rsize = config->ReadSize;
    entry->u.Mount.wsize = config->WriteSize;
    entry->u.Mount.sec_flavor = sec_flavor;
    entry->u.Mount.FsAttrs = FsAttrs;

    status = nfs41_UpcallWaitForReply(entry, config->timeout);
    SeDeleteClientSecurity(&entry->sec_ctx);
    if (status) goto out;
    *session = entry->session;
    if (entry->u.Mount.lease_time > config->timeout)
        config->timeout = entry->u.Mount.lease_time;

    /* map windows ERRORs to NTSTATUS */
    status = map_mount_errors(entry->status);
    if (status == STATUS_SUCCESS)
        *version = entry->version;
    RxFreePool(entry);
out:
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

/* TODO: move mount config stuff to another file -cbodley */

void nfs41_MountConfig_InitDefaults(
    OUT PNFS41_MOUNT_CONFIG Config)
{
    RtlZeroMemory(Config, sizeof(NFS41_MOUNT_CONFIG));

    Config->ReadSize = MOUNT_CONFIG_RW_SIZE_DEFAULT;
    Config->WriteSize = MOUNT_CONFIG_RW_SIZE_DEFAULT;
    Config->ReadOnly = FALSE;
    Config->write_thru = FALSE;
    Config->nocache = FALSE;
    Config->SrvName.Length = 0;
    Config->SrvName.MaximumLength = SERVER_NAME_BUFFER_SIZE;
    Config->SrvName.Buffer = Config->srv_buffer;
    Config->MntPt.Length = 0;
    Config->MntPt.MaximumLength = MAX_PATH;
    Config->MntPt.Buffer = Config->mntpt_buffer;
    Config->SecFlavor.Length = 0;
    Config->SecFlavor.MaximumLength = MAX_SEC_FLAVOR_LEN;
    Config->SecFlavor.Buffer = Config->sec_flavor;
    RtlCopyUnicodeString(&Config->SecFlavor, &AUTH_SYS_NAME);
    Config->timeout = UPCALL_TIMEOUT_DEFAULT;
}

NTSTATUS nfs41_MountConfig_ParseBoolean(
    IN PFILE_FULL_EA_INFORMATION Option,
    IN PUNICODE_STRING usValue,
    OUT PBOOLEAN Value)
{
    NTSTATUS status = STATUS_SUCCESS;

    /* if no value is specified, assume TRUE
     * if a value is specified, it must be a '1' */
    if (Option->EaValueLength == 0 || *usValue->Buffer == L'1')
        *Value = TRUE;
    else
        *Value = FALSE;

    DbgP("    '%ls' -> '%wZ' -> %u\n",
        (LPWSTR)Option->EaName, usValue, *Value);
    return status;
}

NTSTATUS nfs41_MountConfig_ParseDword(
    IN PFILE_FULL_EA_INFORMATION Option,
    IN PUNICODE_STRING usValue,
    OUT PDWORD Value,
    IN DWORD Minimum,
    IN DWORD Maximum)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    LPWSTR Name = (LPWSTR)Option->EaName;

    if (Option->EaValueLength) {
        status = RtlUnicodeStringToInteger(usValue, 0, Value);
        if (status == STATUS_SUCCESS) {
#ifdef IMPOSE_MINMAX_RWSIZES
            if (*Value < Minimum)
                *Value = Minimum;
            if (*Value > Maximum)
                *Value = Maximum;
            DbgP("    '%ls' -> '%wZ' -> %lu\n", Name, usValue, *Value);
#endif
        }
        else
            print_error("Failed to convert %s='%wZ' to unsigned long.\n",
                Name, usValue);
    }

    return status;
}

NTSTATUS nfs41_MountConfig_ParseOptions(
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN OUT PNFS41_MOUNT_CONFIG Config)
{
    NTSTATUS  status = STATUS_SUCCESS;
    PFILE_FULL_EA_INFORMATION Option;
    LPWSTR Name;
    size_t NameLen;
    UNICODE_STRING  usValue;
    Option = EaBuffer;
    while (status == STATUS_SUCCESS) {
        Name = (LPWSTR)Option->EaName;
        NameLen = Option->EaNameLength/sizeof(WCHAR);

        usValue.Length = usValue.MaximumLength = Option->EaValueLength;
        usValue.Buffer = (PWCH)(Option->EaName +
            Option->EaNameLength + sizeof(WCHAR));

        if (wcsncmp(L"ro", Name, NameLen) == 0) {
            status = nfs41_MountConfig_ParseBoolean(Option, &usValue,
                &Config->ReadOnly);
        }
        else if (wcsncmp(L"writethru", Name, NameLen) == 0) {
            status = nfs41_MountConfig_ParseBoolean(Option, &usValue,
                &Config->write_thru);
        }
        else if (wcsncmp(L"nocache", Name, NameLen) == 0) {
            status = nfs41_MountConfig_ParseBoolean(Option, &usValue,
                &Config->nocache);
        }
        else if (wcsncmp(L"timeout", Name, NameLen) == 0) {
            status = nfs41_MountConfig_ParseDword(Option, &usValue,
                &Config->timeout, UPCALL_TIMEOUT_DEFAULT,
                UPCALL_TIMEOUT_DEFAULT);
        }
        else if (wcsncmp(L"rsize", Name, NameLen) == 0) {
            status = nfs41_MountConfig_ParseDword(Option, &usValue,
                &Config->ReadSize, MOUNT_CONFIG_RW_SIZE_MIN,
                MOUNT_CONFIG_RW_SIZE_MAX);
        }
        else if (wcsncmp(L"wsize", Name, NameLen) == 0) {
            status = nfs41_MountConfig_ParseDword(Option, &usValue,
                &Config->WriteSize, MOUNT_CONFIG_RW_SIZE_MIN,
                MOUNT_CONFIG_RW_SIZE_MAX);
        }
        else if (wcsncmp(L"srvname", Name, NameLen) == 0) {
            if (usValue.Length > Config->SrvName.MaximumLength)
                status = STATUS_NAME_TOO_LONG;
            else
                RtlCopyUnicodeString(&Config->SrvName, &usValue);
        }
        else if (wcsncmp(L"mntpt", Name, NameLen) == 0) {
            if (usValue.Length > Config->MntPt.MaximumLength)
                status = STATUS_NAME_TOO_LONG;
            else
                RtlCopyUnicodeString(&Config->MntPt, &usValue);
        }
        else if (wcsncmp(L"sec", Name, NameLen) == 0) {
            if (usValue.Length > Config->SecFlavor.MaximumLength)
                status = STATUS_NAME_TOO_LONG;
            else
                RtlCopyUnicodeString(&Config->SecFlavor, &usValue);
        }
        else {
            status = STATUS_INVALID_PARAMETER;
            print_error("Unrecognized option '%ls' -> '%wZ'\n",
                Name, usValue);
        }

        if (Option->NextEntryOffset == 0)
            break;

        Option = (PFILE_FULL_EA_INFORMATION)
            ((PBYTE)Option + Option->NextEntryOffset);
    }

    return status;
}

NTSTATUS has_nfs_prefix(
    IN PUNICODE_STRING SrvCallName,
    IN PUNICODE_STRING NetRootName)
{
    NTSTATUS status = STATUS_BAD_NETWORK_NAME;

    if (NetRootName->Length == SrvCallName->Length + NfsPrefix.Length) {
        const UNICODE_STRING NetRootPrefix = {
            NfsPrefix.Length,
            NetRootName->MaximumLength - SrvCallName->Length,
            &NetRootName->Buffer[SrvCallName->Length/2]
        };
        if (RtlCompareUnicodeString(&NetRootPrefix, &NfsPrefix, FALSE) == 0)
            status = STATUS_SUCCESS;
    }
    return status;
}

NTSTATUS map_sec_flavor(
    IN PUNICODE_STRING sec_flavor_name,
    OUT PDWORD sec_flavor)
{
    if (RtlCompareUnicodeString(sec_flavor_name, &AUTH_SYS_NAME, FALSE) == 0)
        *sec_flavor = RPCSEC_AUTH_SYS;
    else if (RtlCompareUnicodeString(sec_flavor_name, &AUTHGSS_KRB5_NAME, FALSE) == 0)
        *sec_flavor = RPCSEC_AUTHGSS_KRB5;
    else if (RtlCompareUnicodeString(sec_flavor_name, &AUTHGSS_KRB5I_NAME, FALSE) == 0)
        *sec_flavor = RPCSEC_AUTHGSS_KRB5I;
    else if (RtlCompareUnicodeString(sec_flavor_name, &AUTHGSS_KRB5P_NAME, FALSE) == 0)
        *sec_flavor = RPCSEC_AUTHGSS_KRB5P;
    else return STATUS_INVALID_PARAMETER;
    return STATUS_SUCCESS;
}

NTSTATUS nfs41_GetLUID(
    PLUID id)
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_SUBJECT_CONTEXT sec_ctx;
    SECURITY_QUALITY_OF_SERVICE sec_qos;
    SECURITY_CLIENT_CONTEXT clnt_sec_ctx;

    SeCaptureSubjectContext(&sec_ctx);
    sec_qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sec_qos.ImpersonationLevel = SecurityIdentification;
    sec_qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sec_qos.EffectiveOnly = 0;
    status = SeCreateClientSecurityFromSubjectContext(&sec_ctx, &sec_qos, 1,
                &clnt_sec_ctx);
    if (status) {
        print_error("nfs41_GetLUID: SeCreateClientSecurityFromSubjectContext "
             "failed %x\n", status);
        goto release_sec_ctx;
    }
    status = SeQueryAuthenticationIdToken(clnt_sec_ctx.ClientToken, id);
    if (status) {
        print_error("SeQueryAuthenticationIdToken failed %x\n", status);
        goto release_clnt_sec_ctx;
    }
release_clnt_sec_ctx:
    SeDeleteClientSecurity(&clnt_sec_ctx);
release_sec_ctx:
    SeReleaseSubjectContext(&sec_ctx);

    return status;
}

NTSTATUS nfs41_get_sec_ctx(
    IN enum _SECURITY_IMPERSONATION_LEVEL level,
    OUT PSECURITY_CLIENT_CONTEXT out_ctx)
{
    NTSTATUS status;
    SECURITY_SUBJECT_CONTEXT ctx;
    SECURITY_QUALITY_OF_SERVICE sec_qos;

    SeCaptureSubjectContext(&ctx);
    sec_qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    sec_qos.ImpersonationLevel = level;
    sec_qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sec_qos.EffectiveOnly = 0;
    status = SeCreateClientSecurityFromSubjectContext(&ctx, &sec_qos, 1, out_ctx);
    if (status != STATUS_SUCCESS) {
        print_error("SeCreateClientSecurityFromSubjectContext "
            "failed with %x\n", status);
    }
#ifdef DEBUG_MOUNT
    DbgP("Created client security token %p\n", out_ctx->ClientToken);
#endif
    SeReleaseSubjectContext(&ctx);

    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_CreateVNetRoot(
#else
NTSTATUS nfs41_CreateVNetRoot(
#endif
    IN OUT PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    NFS41_MOUNT_CONFIG *Config;
    __notnull PMRX_V_NET_ROOT pVNetRoot = (PMRX_V_NET_ROOT)
        pCreateNetRootContext->pVNetRoot;
    __notnull PMRX_NET_ROOT pNetRoot = pVNetRoot->pNetRoot;
    __notnull PMRX_SRV_CALL pSrvCall = pNetRoot->pSrvCall;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(pNetRoot);
    NFS41GetDeviceExtension(pCreateNetRootContext->RxContext,DevExt);
    DWORD nfs41d_version = DevExt->nfs41d_version;
    nfs41_mount_entry *existing_mount = NULL;
    LUID luid;
    BOOLEAN found_existing_mount = FALSE, found_matching_flavor = FALSE;

    ASSERT((NodeType(pNetRoot) == RDBSS_NTC_NETROOT) &&
        (NodeType(pNetRoot->pSrvCall) == RDBSS_NTC_SRVCALL));

#ifdef DEBUG_MOUNT
    DbgEn();
    print_srv_call(0, pSrvCall);
    print_net_root(0, pNetRoot);
    print_v_net_root(0, pVNetRoot);

    DbgP("pVNetRoot=%p pNetRoot=%p pSrvCall=%p\n", pVNetRoot, pNetRoot, pSrvCall);
    DbgP("pNetRoot=%wZ Type=%d pSrvCallName=%wZ VirtualNetRootStatus=0x%x "
        "NetRootStatus=0x%x\n", pNetRoot->pNetRootName,
        pNetRoot->Type, pSrvCall->pSrvCallName,
        pCreateNetRootContext->VirtualNetRootStatus,
        pCreateNetRootContext->NetRootStatus);
#endif

    if (pNetRoot->Type != NET_ROOT_DISK && pNetRoot->Type != NET_ROOT_WILD) {
        print_error("nfs41_CreateVNetRoot: Unsupported NetRoot Type %u\n",
            pNetRoot->Type);
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }

    pVNetRootContext->session = INVALID_HANDLE_VALUE;

    /* In order to cooperate with other network providers, we must
     * only claim paths of the form '\\server\nfs4\path' */
    status = has_nfs_prefix(pSrvCall->pSrvCallName, pNetRoot->pNetRootName);
    if (status) {
        print_error("nfs41_CreateVNetRoot: NetRootName %wZ doesn't match "
            "'\\nfs4'!\n", pNetRoot->pNetRootName);
        goto out;
    }
    pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_GOOD;
    pNetRoot->DeviceType = FILE_DEVICE_DISK;

    Config = RxAllocatePoolWithTag(NonPagedPool,
            sizeof(NFS41_MOUNT_CONFIG), NFS41_MM_POOLTAG);
    if (Config == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    nfs41_MountConfig_InitDefaults(Config);

    if (pCreateNetRootContext->RxContext->Create.EaLength) {
        /* parse the extended attributes for mount options */
        status = nfs41_MountConfig_ParseOptions(
            pCreateNetRootContext->RxContext->Create.EaBuffer,
            pCreateNetRootContext->RxContext->Create.EaLength,
            Config);
        if (status != STATUS_SUCCESS)
            goto out_free;
        pVNetRootContext->read_only = Config->ReadOnly;
        pVNetRootContext->write_thru = Config->write_thru;
        pVNetRootContext->nocache = Config->nocache;
    } else {
        /* use the SRV_CALL name (without leading \) as the hostname */
        Config->SrvName.Buffer = pSrvCall->pSrvCallName->Buffer + 1;
        Config->SrvName.Length =
            pSrvCall->pSrvCallName->Length - sizeof(WCHAR);
        Config->SrvName.MaximumLength =
            pSrvCall->pSrvCallName->MaximumLength - sizeof(WCHAR);
    }
    pVNetRootContext->MountPathLen = Config->MntPt.Length;
    pVNetRootContext->timeout = Config->timeout;

    status = map_sec_flavor(&Config->SecFlavor, &pVNetRootContext->sec_flavor);
    if (status != STATUS_SUCCESS) {
        DbgP("Invalid rpcsec security flavor %wZ\n", &Config->SecFlavor);
        goto out_free;
    }

    status = nfs41_GetLUID(&luid);
    if (status)
        goto out_free;

    if (!pNetRootContext->mounts_init) {
#ifdef DEBUG_MOUNT
        DbgP("Initializing mount array\n");
#endif
        ExInitializeFastMutex(&pNetRootContext->mountLock);
        InitializeListHead(&pNetRootContext->mounts.head);
        pNetRootContext->mounts_init = TRUE;
    } else {
        PLIST_ENTRY pEntry;

        ExAcquireFastMutex(&pNetRootContext->mountLock);
        pEntry = &pNetRootContext->mounts.head;
        pEntry = pEntry->Flink;
        while (pEntry != NULL) {
            existing_mount = (nfs41_mount_entry *)CONTAINING_RECORD(pEntry,
                    nfs41_mount_entry, next);
#ifdef DEBUG_MOUNT
            DbgP("comparing %x.%x with %x.%x\n", luid.HighPart, luid.LowPart,
                existing_mount->login_id.HighPart, existing_mount->login_id.LowPart);
#endif
            if (RtlEqualLuid(&luid, &existing_mount->login_id)) {
#ifdef DEBUG_MOUNT
                DbgP("Found a matching LUID entry\n");
#endif
                found_existing_mount = TRUE;
                switch(pVNetRootContext->sec_flavor) {
                case RPCSEC_AUTH_SYS:
                    if (existing_mount->authsys_session != INVALID_HANDLE_VALUE)
                        pVNetRootContext->session =
                            existing_mount->authsys_session;
                    break;
                case RPCSEC_AUTHGSS_KRB5:
                    if (existing_mount->gssi_session != INVALID_HANDLE_VALUE)
                        pVNetRootContext->session = existing_mount->gss_session;
                    break;
                case RPCSEC_AUTHGSS_KRB5I:
                    if (existing_mount->gss_session != INVALID_HANDLE_VALUE)
                        pVNetRootContext->session = existing_mount->gssi_session;
                    break;
                case RPCSEC_AUTHGSS_KRB5P:
                    if (existing_mount->gssp_session != INVALID_HANDLE_VALUE)
                        pVNetRootContext->session = existing_mount->gssp_session;
                    break;
                }
                if (pVNetRootContext->session &&
                        pVNetRootContext->session != INVALID_HANDLE_VALUE)
                    found_matching_flavor = 1;
                break;
            }
            if (pEntry->Flink == &pNetRootContext->mounts.head)
                break;
            pEntry = pEntry->Flink;
        }
        ExReleaseFastMutex(&pNetRootContext->mountLock);
#ifdef DEBUG_MOUNT
        if (!found_matching_flavor)
            DbgP("Didn't find matching security flavor\n");
#endif
    }

    /* send the mount upcall */
    status = nfs41_mount(Config, pVNetRootContext->sec_flavor,
        &pVNetRootContext->session, &nfs41d_version,
        &pVNetRootContext->FsAttrs);
    if (status != STATUS_SUCCESS) {
        BOOLEAN MountsEmpty;
        nfs41_IsListEmpty(pNetRootContext->mountLock,
            pNetRootContext->mounts, MountsEmpty);
        if (!found_existing_mount && MountsEmpty)
            pNetRootContext->mounts_init = FALSE;
        pVNetRootContext->session = INVALID_HANDLE_VALUE;
        goto out_free;
    }
    pVNetRootContext->timeout = Config->timeout;

    if (!found_existing_mount) {
        /* create a new mount entry and add it to the list */
        nfs41_mount_entry *entry;
        entry = RxAllocatePoolWithTag(NonPagedPool, sizeof(nfs41_mount_entry),
            NFS41_MM_POOLTAG_MOUNT);
        if (entry == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto out_free;
        }
        entry->authsys_session = entry->gss_session =
            entry->gssi_session = entry->gssp_session = INVALID_HANDLE_VALUE;
        switch (pVNetRootContext->sec_flavor) {
        case RPCSEC_AUTH_SYS:
            entry->authsys_session = pVNetRootContext->session; break;
        case RPCSEC_AUTHGSS_KRB5:
            entry->gss_session = pVNetRootContext->session; break;
        case RPCSEC_AUTHGSS_KRB5I:
            entry->gssi_session = pVNetRootContext->session; break;
        case RPCSEC_AUTHGSS_KRB5P:
            entry->gssp_session = pVNetRootContext->session; break;
        }
        RtlCopyLuid(&entry->login_id, &luid);
        nfs41_AddEntry(pNetRootContext->mountLock,
            pNetRootContext->mounts, entry);
    } else if (!found_matching_flavor) {
        ASSERT(existing_mount != NULL);
        /* modify existing mount entry */
#ifdef DEBUG_MOUNT
        DbgP("Using existing %d flavor session 0x%x\n",
            pVNetRootContext->sec_flavor);
#endif
        switch (pVNetRootContext->sec_flavor) {
        case RPCSEC_AUTH_SYS:
            existing_mount->authsys_session = pVNetRootContext->session; break;
        case RPCSEC_AUTHGSS_KRB5:
            existing_mount->gss_session = pVNetRootContext->session; break;
        case RPCSEC_AUTHGSS_KRB5I:
            existing_mount->gssi_session = pVNetRootContext->session; break;
        case RPCSEC_AUTHGSS_KRB5P:
            existing_mount->gssp_session = pVNetRootContext->session; break;
        }
    }
    pNetRootContext->nfs41d_version = nfs41d_version;
#ifdef DEBUG_MOUNT
    DbgP("Saving new session 0x%x\n", pVNetRootContext->session);
#endif
#ifdef STORE_MOUNT_SEC_CONTEXT
    status = nfs41_get_sec_ctx(SecurityImpersonation,
        &pVNetRootContext->mount_sec_ctx);
#endif

out_free:
    RxFreePool(Config);
out:
    pCreateNetRootContext->VirtualNetRootStatus = status;
    if (pNetRoot->Context == NULL)
        pCreateNetRootContext->NetRootStatus = status;
    pCreateNetRootContext->Callback(pCreateNetRootContext);

    /* RDBSS expects that MRxCreateVNetRoot returns STATUS_PENDING
     * on success or failure */
    status = STATUS_PENDING;
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
VOID NTAPI nfs41_ExtractNetRootName(
#else
VOID nfs41_ExtractNetRootName(
#endif
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL)
{
    ULONG length = FilePathName->Length;
    PWCH w = FilePathName->Buffer;
    PWCH wlimit = (PWCH)(((PCHAR)w)+length);
    PWCH wlow;

    w += (SrvCall->pSrvCallName->Length/sizeof(WCHAR));
    NetRootName->Buffer = wlow = w;
    /* parse the entire path into NetRootName */
#if USE_ENTIRE_PATH
    w = wlimit;
#else
    for (;;) {
        if (w >= wlimit)
            break;
        if ((*w == OBJ_NAME_PATH_SEPARATOR) && (w != wlow))
            break;
        w++;
    }
#endif
    NetRootName->Length = NetRootName->MaximumLength
                = (USHORT)((PCHAR)w - (PCHAR)wlow);
#ifdef DEBUG_MOUNT
    DbgP("In: pSrvCall %p PathName=%wZ SrvCallName=%wZ Out: NetRootName=%wZ\n",
        SrvCall, FilePathName, SrvCall->pSrvCallName, NetRootName);
#endif
    return;

}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_FinalizeSrvCall(
#else
NTSTATUS nfs41_FinalizeSrvCall(
#endif
    PMRX_SRV_CALL pSrvCall,
    BOOLEAN Force)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFS41_SERVER_ENTRY pServerEntry = (PNFS41_SERVER_ENTRY)(pSrvCall->Context);

#ifdef DEBUG_MOUNT
    DbgEn();
#endif
    print_srv_call(0, pSrvCall);

    if (pSrvCall->Context == NULL)
        goto out;

#ifndef __REACTOS__
    InterlockedCompareExchangePointer(&pServerEntry->pRdbssSrvCall,
        NULL, pSrvCall);
#else
    InterlockedCompareExchangePointer((void * volatile *)&pServerEntry->pRdbssSrvCall,
        NULL, pSrvCall);
#endif
    RxFreePool(pServerEntry);

    pSrvCall->Context = NULL;
out:
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_FinalizeNetRoot(
#else
NTSTATUS nfs41_FinalizeNetRoot(
#endif
    IN OUT PMRX_NET_ROOT pNetRoot,
    IN PBOOLEAN ForceDisconnect)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension((PMRX_NET_ROOT)pNetRoot);
    nfs41_updowncall_entry *tmp;
    nfs41_mount_entry *mount_tmp;

#ifdef DEBUG_MOUNT
    DbgEn();
    print_net_root(1, pNetRoot);
#endif

    if (pNetRoot->Type != NET_ROOT_DISK && pNetRoot->Type != NET_ROOT_WILD) {
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }

    if (pNetRootContext == NULL || !pNetRootContext->mounts_init) {
        print_error("nfs41_FinalizeNetRoot: No valid session established\n");
        goto out;
    }

    if (pNetRoot->NumberOfFcbs > 0 || pNetRoot->NumberOfSrvOpens > 0) {
        print_error("%d open Fcbs %d open SrvOpens\n", pNetRoot->NumberOfFcbs,
            pNetRoot->NumberOfSrvOpens);
        goto out;
    }

    do {
        nfs41_GetFirstMountEntry(pNetRootContext->mountLock,
            pNetRootContext->mounts, mount_tmp);
        if (mount_tmp == NULL)
            break;
#ifdef DEBUG_MOUNT
        DbgP("Removing entry luid %x.%x from mount list\n",
            mount_tmp->login_id.HighPart, mount_tmp->login_id.LowPart);
#endif
        if (mount_tmp->authsys_session != INVALID_HANDLE_VALUE) {
            status = nfs41_unmount(mount_tmp->authsys_session,
                pNetRootContext->nfs41d_version, UPCALL_TIMEOUT_DEFAULT);
            if (status)
                print_error("nfs41_unmount AUTH_SYS failed with %d\n", status);
        }
        if (mount_tmp->gss_session != INVALID_HANDLE_VALUE) {
            status = nfs41_unmount(mount_tmp->gss_session,
                pNetRootContext->nfs41d_version, UPCALL_TIMEOUT_DEFAULT);
            if (status)
                print_error("nfs41_unmount RPCSEC_GSS_KRB5 failed with %d\n",
                            status);
        }
        if (mount_tmp->gssi_session != INVALID_HANDLE_VALUE) {
            status = nfs41_unmount(mount_tmp->gssi_session,
                pNetRootContext->nfs41d_version, UPCALL_TIMEOUT_DEFAULT);
            if (status)
                print_error("nfs41_unmount RPCSEC_GSS_KRB5I failed with %d\n",
                            status);
        }
        if (mount_tmp->gssp_session != INVALID_HANDLE_VALUE) {
            status = nfs41_unmount(mount_tmp->gssp_session,
                pNetRootContext->nfs41d_version, UPCALL_TIMEOUT_DEFAULT);
            if (status)
                print_error("nfs41_unmount RPCSEC_GSS_KRB5P failed with %d\n",
                            status);
        }
        nfs41_RemoveEntry(pNetRootContext->mountLock, mount_tmp);
        RxFreePool(mount_tmp);
    } while (1);
    /* ignore any errors from unmount */
    status = STATUS_SUCCESS;

    // check if there is anything waiting in the upcall or downcall queue
    do {
        nfs41_GetFirstEntry(upcallLock, upcall, tmp);
        if (tmp != NULL) {
            DbgP("Removing entry from upcall list\n");
            nfs41_RemoveEntry(upcallLock, tmp);
            tmp->status = STATUS_INSUFFICIENT_RESOURCES;
            KeSetEvent(&tmp->cond, 0, FALSE);
        } else
            break;
    } while (1);

    do {
        nfs41_GetFirstEntry(downcallLock, downcall, tmp);
        if (tmp != NULL) {
            DbgP("Removing entry from downcall list\n");
            nfs41_RemoveEntry(downcallLock, tmp);
            tmp->status = STATUS_INSUFFICIENT_RESOURCES;
            KeSetEvent(&tmp->cond, 0, FALSE);
        } else
            break;
    } while (1);
out:
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_FinalizeVNetRoot(
#else
NTSTATUS nfs41_FinalizeVNetRoot(
#endif
    IN OUT PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN ForceDisconnect)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(pVNetRoot);
#ifdef DEBUG_MOUNT
    DbgEn();
    print_v_net_root(1, pVNetRoot);
#endif
    if (pVNetRoot->pNetRoot->Type != NET_ROOT_DISK &&
            pVNetRoot->pNetRoot->Type != NET_ROOT_WILD)
        status = STATUS_NOT_SUPPORTED;
#ifdef STORE_MOUNT_SEC_CONTEXT
    else if (pVNetRootContext->session != INVALID_HANDLE_VALUE) {
#ifdef DEBUG_MOUNT
        DbgP("nfs41_FinalizeVNetRoot: deleting security context: %p\n",
            pVNetRootContext->mount_sec_ctx.ClientToken);
#endif
        SeDeleteClientSecurity(&pVNetRootContext->mount_sec_ctx);
    }
#endif
#ifdef DEBUG_MOUNT
    DbgEx();
#endif
    return status;
}

BOOLEAN isDataAccess(
    ACCESS_MASK mask)
{
    if (mask & (FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA))
        return TRUE;
    return FALSE;
}

BOOLEAN isOpen2Create(
    ULONG disposition)
{
    if (disposition == FILE_CREATE || disposition == FILE_OPEN_IF ||
            disposition == FILE_OVERWRITE_IF || disposition == FILE_SUPERSEDE)
        return TRUE;
    return FALSE;
}

BOOLEAN isFilenameTooLong(
    PUNICODE_STRING name,
    PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext)
{
    PFILE_FS_ATTRIBUTE_INFORMATION attrs = &pVNetRootContext->FsAttrs;
    LONG len = attrs->MaximumComponentNameLength, count = 1, i;
    PWCH p = name->Buffer;
    for (i = 0; i < name->Length / 2; i++) {
        if (p[0] == L'\\') count = 1;
        else {
            if (p[0] == L'\0') return FALSE;
            if (count > len) return TRUE;
            count++;
        }
        p++;
    }
    return FALSE;
}

BOOLEAN isStream(
    PUNICODE_STRING name)
{
    LONG i;
    PWCH p = name->Buffer;
    for (i = 0; i < name->Length / 2; i++) {
        if (p[0] == L':') return TRUE;
        else if (p[0] == L'\0') return FALSE;
        p++;
    }
    return FALSE;
}

BOOLEAN areOpenParamsValid(NT_CREATE_PARAMETERS *params)
{
    /* from ms-fsa page 52 */
    if ((params->CreateOptions & FILE_DELETE_ON_CLOSE) &&
            !(params->DesiredAccess & DELETE))
        return FALSE;
    if ((params->CreateOptions & FILE_DIRECTORY_FILE) &&
            (params->Disposition == FILE_SUPERSEDE ||
                params->Disposition == FILE_OVERWRITE ||
                params->Disposition == FILE_OVERWRITE_IF))
        return FALSE;
    if ((params->CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING) &&
            (params->DesiredAccess & FILE_APPEND_DATA) &&
            !(params->DesiredAccess & FILE_WRITE_DATA))
        return FALSE;
    /* from ms-fsa 3.1.5.1.1 page 56 */
    if ((params->CreateOptions & FILE_DIRECTORY_FILE) &&
            (params->FileAttributes & FILE_ATTRIBUTE_TEMPORARY))
        return FALSE;
    return TRUE;
}

NTSTATUS map_open_errors(
    DWORD status,
    USHORT len)
{
    switch (status) {
    case NO_ERROR:                      return STATUS_SUCCESS;
    case ERROR_ACCESS_DENIED:
        if (len > 0)                    return STATUS_ACCESS_DENIED;
        else                            return STATUS_SUCCESS;
    case ERROR_INVALID_REPARSE_DATA:
    case ERROR_INVALID_NAME:            return STATUS_OBJECT_NAME_INVALID;
    case ERROR_FILE_EXISTS:             return STATUS_OBJECT_NAME_COLLISION;
    case ERROR_FILE_INVALID:            return STATUS_FILE_INVALID;
    case ERROR_FILE_NOT_FOUND:          return STATUS_OBJECT_NAME_NOT_FOUND;
    case ERROR_FILENAME_EXCED_RANGE:    return STATUS_NAME_TOO_LONG;
    case ERROR_NETWORK_ACCESS_DENIED:   return STATUS_NETWORK_ACCESS_DENIED;
    case ERROR_PATH_NOT_FOUND:          return STATUS_OBJECT_PATH_NOT_FOUND;
    case ERROR_BAD_NETPATH:             return STATUS_BAD_NETWORK_PATH;
    case ERROR_SHARING_VIOLATION:       return STATUS_SHARING_VIOLATION;
    case ERROR_REPARSE:                 return STATUS_REPARSE;
    case ERROR_TOO_MANY_LINKS:          return STATUS_TOO_MANY_LINKS;
    case ERROR_DIRECTORY:               return STATUS_FILE_IS_A_DIRECTORY;
    case ERROR_BAD_FILE_TYPE:           return STATUS_NOT_A_DIRECTORY;
    default:
        print_error("[ERROR] nfs41_Create: upcall returned %d returning "
            "STATUS_INSUFFICIENT_RESOURCES\n", status);
    case ERROR_OUTOFMEMORY:             return STATUS_INSUFFICIENT_RESOURCES;
    }
}

DWORD map_disposition_to_create_retval(
    DWORD disposition,
    DWORD errno)
{
    switch(disposition) {
    case FILE_SUPERSEDE:
        if (errno == ERROR_FILE_NOT_FOUND)  return FILE_CREATED;
        else                                return FILE_SUPERSEDED;
    case FILE_CREATE:                       return FILE_CREATED;
    case FILE_OPEN:                         return FILE_OPENED;
    case FILE_OPEN_IF:
        if (errno == ERROR_FILE_NOT_FOUND)  return FILE_CREATED;
        else                                return FILE_OPENED;
    case FILE_OVERWRITE:                    return FILE_OVERWRITTEN;
    case FILE_OVERWRITE_IF:
        if (errno == ERROR_FILE_NOT_FOUND)  return FILE_CREATED;
        else                                return FILE_OVERWRITTEN;
    default:
        print_error("unknown disposition %d\n", disposition);
        return FILE_OPENED;
    }
}

static BOOLEAN create_should_pass_ea(
    IN PFILE_FULL_EA_INFORMATION ea,
    IN ULONG disposition)
{
    /* don't pass cygwin EAs */
    if (AnsiStrEq(&NfsV3Attributes, ea->EaName, ea->EaNameLength)
        || AnsiStrEq(&NfsActOnLink, ea->EaName, ea->EaNameLength)
        || AnsiStrEq(&NfsSymlinkTargetName, ea->EaName, ea->EaNameLength))
        return FALSE;
    /* only set EAs on file creation */
    return disposition == FILE_SUPERSEDE || disposition == FILE_CREATE
        || disposition == FILE_OPEN_IF || disposition == FILE_OVERWRITE
        || disposition == FILE_OVERWRITE_IF;
}

NTSTATUS check_nfs41_create_args(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNT_CREATE_PARAMETERS params = &RxContext->Create.NtCreateParameters;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PFILE_FS_ATTRIBUTE_INFORMATION FsAttrs =
        &pVNetRootContext->FsAttrs;
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PMRX_FCB Fcb = RxContext->pFcb;
    __notnull PNFS41_FCB nfs41_fcb = (PNFS41_FCB)Fcb->Context;
    PFILE_FULL_EA_INFORMATION ea = (PFILE_FULL_EA_INFORMATION)
        RxContext->CurrentIrp->AssociatedIrp.SystemBuffer;

    if (Fcb->pNetRoot->Type != NET_ROOT_DISK &&
            Fcb->pNetRoot->Type != NET_ROOT_WILD) {
        print_error("nfs41_Create: Unsupported NetRoot Type %u\n",
            Fcb->pNetRoot->Type);
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }

    if (FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE )) {
        print_error("FCB_STATE_PAGING_FILE not implemented\n");
        status = STATUS_NOT_IMPLEMENTED;
        goto out;
    }

    if (!pNetRootContext->mounts_init) {
        print_error("nfs41_Create: No valid session established\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }

    if (isStream(SrvOpen->pAlreadyPrefixedName)) {
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }

    if (pVNetRootContext->read_only &&
            (params->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA))) {
        status = STATUS_NETWORK_ACCESS_DENIED;
        goto out;
    }

    /* if FCB was marked for deletion and opened multiple times, as soon
     * as first close happen, FCB transitions into delete_pending state
     * no more opens allowed
     */
    if (Fcb->OpenCount && nfs41_fcb->DeletePending) {
        status = STATUS_DELETE_PENDING;
        goto out;
    }

    /* ms-fsa: 3.1.5.1.2.1 page 68 */
    if (Fcb->OpenCount && nfs41_fcb->StandardInfo.DeletePending &&
            !(params->ShareAccess & FILE_SHARE_DELETE) &&
                (params->DesiredAccess & (FILE_EXECUTE | FILE_READ_DATA |
                    FILE_WRITE_DATA | FILE_APPEND_DATA))) {
        status = STATUS_SHARING_VIOLATION;
        goto out;
    }

    /* rdbss seems miss this sharing_violation check */
    if (Fcb->OpenCount && params->Disposition == FILE_SUPERSEDE) {
#ifdef __REACTOS__
        if ((!RxContext->CurrentIrpSp->FileObject->SharedRead &&
                (params->DesiredAccess & FILE_READ_DATA)) ||
            ((!RxContext->CurrentIrpSp->FileObject->SharedWrite &&
                (params->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA |
                    FILE_WRITE_ATTRIBUTES))) ||
            (!RxContext->CurrentIrpSp->FileObject->SharedDelete &&
                (params->DesiredAccess & DELETE)))) {
#else
        if ((!RxContext->CurrentIrpSp->FileObject->SharedRead &&
                (params->DesiredAccess & FILE_READ_DATA)) ||
            (!RxContext->CurrentIrpSp->FileObject->SharedWrite &&
                (params->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA |
                    FILE_WRITE_ATTRIBUTES)) ||
            (!RxContext->CurrentIrpSp->FileObject->SharedDelete &&
                (params->DesiredAccess & DELETE)))) {
#endif
            status = STATUS_SHARING_VIOLATION;
            goto out;
        }
    }
    if (isFilenameTooLong(SrvOpen->pAlreadyPrefixedName, pVNetRootContext)) {
        status = STATUS_OBJECT_NAME_INVALID;
        goto out;
    }

    if (!areOpenParamsValid(params)) {
        status = STATUS_INVALID_PARAMETER;
        goto out;
    }

    /* from ms-fsa 3.1.5.1.1 page 56 */
    if ((params->CreateOptions & FILE_DELETE_ON_CLOSE) &&
            (params->FileAttributes & FILE_ATTRIBUTE_READONLY)) {
        status = STATUS_CANNOT_DELETE;
        goto out;
    }

    if (ea) {
        /* ignore cygwin EAs when checking support and access */
        if (!AnsiStrEq(&NfsV3Attributes, ea->EaName, ea->EaNameLength) &&
            !AnsiStrEq(&NfsActOnLink, ea->EaName, ea->EaNameLength) &&
            !AnsiStrEq(&NfsSymlinkTargetName, ea->EaName, ea->EaNameLength)) {
            if (!(FsAttrs->FileSystemAttributes & FILE_SUPPORTS_EXTENDED_ATTRIBUTES)) {
                status = STATUS_EAS_NOT_SUPPORTED;
                goto out;
            }
        }
    } else if (RxContext->CurrentIrpSp->Parameters.Create.EaLength) {
        status = STATUS_INVALID_PARAMETER;
        goto out;
    }

out:
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Create(
#else
NTSTATUS nfs41_Create(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    nfs41_updowncall_entry *entry = NULL;
    PNT_CREATE_PARAMETERS params = &RxContext->Create.NtCreateParameters;
    PFILE_FULL_EA_INFORMATION ea = (PFILE_FULL_EA_INFORMATION)
        RxContext->CurrentIrp->AssociatedIrp.SystemBuffer;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PMRX_FCB Fcb = RxContext->pFcb;
    __notnull PNFS41_FCB nfs41_fcb = (PNFS41_FCB)Fcb->Context;
    PNFS41_FOBX nfs41_fobx = NULL;
    BOOLEAN oldDeletePending = nfs41_fcb->StandardInfo.DeletePending;
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

#ifdef DEBUG_OPEN
    DbgEn();
    print_debug_header(RxContext);
    print_nt_create_params(1, RxContext->Create.NtCreateParameters);
    if (ea) print_ea_info(0, ea);
#endif

    status = check_nfs41_create_args(RxContext);
    if (status) goto out;

#if defined(STORE_MOUNT_SEC_CONTEXT) && defined (USE_MOUNT_SEC_CONTEXT)
    status = nfs41_UpcallCreate(NFS41_OPEN, &pVNetRootContext->mount_sec_ctx,
#else
    status = nfs41_UpcallCreate(NFS41_OPEN, NULL,
#endif
        pVNetRootContext->session, INVALID_HANDLE_VALUE,
        pNetRootContext->nfs41d_version,
        SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Open.access_mask = params->DesiredAccess;
    entry->u.Open.access_mode = params->ShareAccess;
    entry->u.Open.attrs = params->FileAttributes;
    if (!(params->CreateOptions & FILE_DIRECTORY_FILE))
        entry->u.Open.attrs |= FILE_ATTRIBUTE_ARCHIVE;
    entry->u.Open.disp = params->Disposition;
    entry->u.Open.copts = params->CreateOptions;
    entry->u.Open.srv_open = SrvOpen;
    /* treat the NfsActOnLink ea as FILE_OPEN_REPARSE_POINT */
    if ((ea && AnsiStrEq(&NfsActOnLink, ea->EaName, ea->EaNameLength)) ||
            (entry->u.Open.access_mask & DELETE))
        entry->u.Open.copts |= FILE_OPEN_REPARSE_POINT;
    if (isDataAccess(params->DesiredAccess) || isOpen2Create(params->Disposition))
        entry->u.Open.open_owner_id = InterlockedIncrement(&open_owner_id);
    // if we are creating a file check if nfsv3attributes were passed in
    if (params->Disposition != FILE_OPEN && params->Disposition != FILE_OVERWRITE) {
        entry->u.Open.mode = 0777;
        if (ea && AnsiStrEq(&NfsV3Attributes, ea->EaName, ea->EaNameLength)) {
            nfs3_attrs *attrs = (nfs3_attrs *)(ea->EaName + ea->EaNameLength + 1);
#ifdef DEBUG_OPEN
            DbgP("creating file with mode %o\n", attrs->mode);
#endif
            entry->u.Open.mode = attrs->mode;
        }
        if (params->FileAttributes & FILE_ATTRIBUTE_READONLY)
            entry->u.Open.mode = 0444;
    }
    if (entry->u.Open.disp == FILE_CREATE && ea &&
            AnsiStrEq(&NfsSymlinkTargetName, ea->EaName, ea->EaNameLength)) {
        /* for a cygwin symlink, given as a unicode string */
        entry->u.Open.symlink.Buffer = (PWCH)(ea->EaName + ea->EaNameLength + 1);
        entry->u.Open.symlink.MaximumLength = entry->u.Open.symlink.Length = ea->EaValueLength;
    }
retry_on_link:
    if (ea && create_should_pass_ea(ea, params->Disposition)) {
        /* lock the extended attribute buffer for read access in user space */
        entry->u.Open.EaMdl = IoAllocateMdl(ea,
            RxContext->CurrentIrpSp->Parameters.Create.EaLength,
            FALSE, FALSE, NULL);
        if (entry->u.Open.EaMdl == NULL) {
            status = STATUS_INTERNAL_ERROR;
            RxFreePool(entry);
            goto out;
        }
        entry->u.Open.EaMdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
        MmProbeAndLockPages(entry->u.Open.EaMdl, KernelMode, IoModifyAccess);
    }

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
#ifndef USE_MOUNT_SEC_CONTEXT
    SeDeleteClientSecurity(&entry->sec_ctx);
#endif
    if (status) goto out;

    if (entry->u.Open.EaMdl) {
        MmUnlockPages(entry->u.Open.EaMdl);
        IoFreeMdl(entry->u.Open.EaMdl);
    }

    if (entry->status == NO_ERROR && entry->errno == ERROR_REPARSE) {
        /* symbolic link handling. when attempting to open a symlink when the
         * FILE_OPEN_REPARSE_POINT flag is not set, replace the filename with
         * the symlink target's by calling RxPrepareToReparseSymbolicLink()
         * and returning STATUS_REPARSE. the object manager will attempt to
         * open the new path, and return its handle for the original open */
        PRDBSS_DEVICE_OBJECT DeviceObject = RxContext->RxDeviceObject;
        PV_NET_ROOT VNetRoot = (PV_NET_ROOT)
            RxContext->pRelevantSrvOpen->pVNetRoot;
        PUNICODE_STRING VNetRootPrefix = &VNetRoot->PrefixEntry.Prefix;
        UNICODE_STRING AbsPath;
        PCHAR buf;
        BOOLEAN ReparseRequired;

        /* allocate the string for RxPrepareToReparseSymbolicLink(), and
         * format an absolute path "DeviceName+VNetRootName+symlink" */
        AbsPath.Length = DeviceObject->DeviceName.Length +
            VNetRootPrefix->Length + entry->u.Open.symlink.Length;
        AbsPath.MaximumLength = AbsPath.Length + sizeof(UNICODE_NULL);
        AbsPath.Buffer = RxAllocatePoolWithTag(NonPagedPool,
            AbsPath.MaximumLength, NFS41_MM_POOLTAG);
        if (AbsPath.Buffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto out_free;
        }

        buf = (PCHAR)AbsPath.Buffer;
        RtlCopyMemory(buf, DeviceObject->DeviceName.Buffer,
            DeviceObject->DeviceName.Length);
        buf += DeviceObject->DeviceName.Length;
        RtlCopyMemory(buf, VNetRootPrefix->Buffer, VNetRootPrefix->Length);
        buf += VNetRootPrefix->Length;
        RtlCopyMemory(buf, entry->u.Open.symlink.Buffer,
            entry->u.Open.symlink.Length);
        RxFreePool(entry->u.Open.symlink.Buffer);
        buf += entry->u.Open.symlink.Length;
        *(PWCHAR)buf = UNICODE_NULL;

        status = RxPrepareToReparseSymbolicLink(RxContext,
            entry->u.Open.symlink_embedded, &AbsPath, TRUE, &ReparseRequired);
#ifdef DEBUG_OPEN
        DbgP("RxPrepareToReparseSymbolicLink(%u, '%wZ') returned %08lX, "
            "FileName is '%wZ'\n", entry->u.Open.symlink_embedded,
            &AbsPath, status, &RxContext->CurrentIrpSp->FileObject->FileName);
#endif
        if (status == STATUS_SUCCESS) {
            /* if a reparse is not required, reopen the link itself.  this
             * happens with operations on cygwin symlinks, where the reparse
             * flag is not set */
            if (!ReparseRequired) {
                entry->u.Open.symlink.Length = 0;
                entry->u.Open.copts |= FILE_OPEN_REPARSE_POINT;
                goto retry_on_link;
            }
            status = STATUS_REPARSE;
        }
        goto out_free;
    }

    status = map_open_errors(entry->status,
                SrvOpen->pAlreadyPrefixedName->Length);
    if (status) {
#ifdef DEBUG_OPEN
        print_open_error(1, status);
#endif
        goto out_free;
    }

    if (!RxIsFcbAcquiredExclusive(Fcb)) {
        ASSERT(!RxIsFcbAcquiredShared(Fcb));
        RxAcquireExclusiveFcbResourceInMRx(Fcb);
    }

    RxContext->pFobx = RxCreateNetFobx(RxContext, SrvOpen);
    if (RxContext->pFobx == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out_free;
    }
#ifdef DEBUG_OPEN
    DbgP("nfs41_Create: created FOBX %p\n", RxContext->pFobx);
#endif
    nfs41_fobx = (PNFS41_FOBX)(RxContext->pFobx)->Context;
    nfs41_fobx->nfs41_open_state = entry->open_state;
#ifndef USE_MOUNT_SEC_CONTEXT
    status = nfs41_get_sec_ctx(SecurityImpersonation, &nfs41_fobx->sec_ctx);
    if (status)
        goto out_free;
#else
    RtlCopyMemory(&nfs41_fobx->sec_ctx, &pVNetRootContext->mount_sec_ctx,
        sizeof(nfs41_fobx->sec_ctx));
#endif

    // we get attributes only for data access and file (not directories)
    if (Fcb->OpenCount == 0 ||
            (Fcb->OpenCount > 0 &&
                nfs41_fcb->changeattr != entry->ChangeTime)) {
        FCB_INIT_PACKET InitPacket;
        RX_FILE_TYPE StorageType = FileTypeNotYetKnown;
        RtlCopyMemory(&nfs41_fcb->BasicInfo, &entry->u.Open.binfo,
            sizeof(entry->u.Open.binfo));
        RtlCopyMemory(&nfs41_fcb->StandardInfo, &entry->u.Open.sinfo,
            sizeof(entry->u.Open.sinfo));
        nfs41_fcb->mode = entry->u.Open.mode;
        nfs41_fcb->changeattr = entry->ChangeTime;
        if (((params->CreateOptions & FILE_DELETE_ON_CLOSE) &&
                !pVNetRootContext->read_only) || oldDeletePending)
            nfs41_fcb->StandardInfo.DeletePending = TRUE;

        RxFormInitPacket(InitPacket,
            &entry->u.Open.binfo.FileAttributes,
            &entry->u.Open.sinfo.NumberOfLinks,
            &entry->u.Open.binfo.CreationTime,
            &entry->u.Open.binfo.LastAccessTime,
            &entry->u.Open.binfo.LastWriteTime,
            &entry->u.Open.binfo.ChangeTime,
            &entry->u.Open.sinfo.AllocationSize,
            &entry->u.Open.sinfo.EndOfFile,
            &entry->u.Open.sinfo.EndOfFile);

        if (entry->u.Open.sinfo.Directory)
            StorageType = FileTypeDirectory;
        else
            StorageType = FileTypeFile;

        RxFinishFcbInitialization(Fcb, RDBSS_STORAGE_NTC(StorageType),
                                    &InitPacket);
    }
#ifdef DEBUG_OPEN
    else
        DbgP("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

    print_basic_info(1, &nfs41_fcb->BasicInfo);
    print_std_info(1, &nfs41_fcb->StandardInfo);
#endif

    /* aglo: 05/10/2012. it seems like always have to invalid the cache if the
     * file has been opened before and being opened again for data access.
     * If the file was opened before, RDBSS might have cached (unflushed) data
     * and by opening it again, we will not have the correct representation of
     * the file size and data content. fileio tests 208, 219, 221.
     */
    if (Fcb->OpenCount > 0 && (isDataAccess(params->DesiredAccess) ||
            nfs41_fcb->changeattr != entry->ChangeTime) &&
                !nfs41_fcb->StandardInfo.Directory) {
        ULONG flag = DISABLE_CACHING;
#ifdef DEBUG_OPEN
        DbgP("nfs41_Create: reopening (changed) file %wZ\n",
            SrvOpen->pAlreadyPrefixedName);
#endif
        RxChangeBufferingState((PSRV_OPEN)SrvOpen, ULongToPtr(flag), 1);
    }
    if (!nfs41_fcb->StandardInfo.Directory &&
            isDataAccess(params->DesiredAccess)) {
        nfs41_fobx->deleg_type = entry->u.Open.deleg_type;
#ifdef DEBUG_OPEN
        DbgP("nfs41_Create: received delegation %d\n", entry->u.Open.deleg_type);
#endif
        if (!(params->CreateOptions & FILE_WRITE_THROUGH) &&
                !pVNetRootContext->write_thru &&
                (entry->u.Open.deleg_type == 2 ||
                (params->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)))) {
#ifdef DEBUG_OPEN
            DbgP("nfs41_Create: enabling write buffering\n");
#endif
            SrvOpen->BufferingFlags |=
                (FCB_STATE_WRITECACHING_ENABLED |
                FCB_STATE_WRITEBUFFERING_ENABLED);
        } else if (params->CreateOptions & FILE_WRITE_THROUGH ||
                    pVNetRootContext->write_thru)
            nfs41_fobx->write_thru = TRUE;
        if (entry->u.Open.deleg_type >= 1 ||
                params->DesiredAccess & FILE_READ_DATA) {
#ifdef DEBUG_OPEN
            DbgP("nfs41_Create: enabling read buffering\n");
#endif
            SrvOpen->BufferingFlags |=
                (FCB_STATE_READBUFFERING_ENABLED |
                FCB_STATE_READCACHING_ENABLED);
        }
        if (pVNetRootContext->nocache ||
                (params->CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING)) {
#ifdef DEBUG_OPEN
            DbgP("nfs41_Create: disabling buffering\n");
#endif
            SrvOpen->BufferingFlags = FCB_STATE_DISABLE_LOCAL_BUFFERING;
            nfs41_fobx->nocache = TRUE;
        } else if (!entry->u.Open.deleg_type && !Fcb->OpenCount) {
            nfs41_fcb_list_entry *oentry;
#ifdef DEBUG_OPEN
            DbgP("nfs41_Create: received no delegations: srv_open=%p "
                "ctime=%llu\n", SrvOpen, entry->ChangeTime);
#endif
            oentry = RxAllocatePoolWithTag(NonPagedPool,
                sizeof(nfs41_fcb_list_entry), NFS41_MM_POOLTAG_OPEN);
            if (oentry == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto out_free;
            }
            oentry->fcb = RxContext->pFcb;
            oentry->nfs41_fobx = nfs41_fobx;
            oentry->session = pVNetRootContext->session;
            oentry->ChangeTime = entry->ChangeTime;
            oentry->skip = FALSE;
            nfs41_AddEntry(fcblistLock, openlist, oentry);
        }
    }

    if ((params->CreateOptions & FILE_DELETE_ON_CLOSE) &&
            !pVNetRootContext->read_only)
        nfs41_fcb->StandardInfo.DeletePending = TRUE;

    RxContext->Create.ReturnedCreateInformation =
        map_disposition_to_create_retval(params->Disposition, entry->errno);

    RxContext->pFobx->OffsetOfNextEaToReturn = 1;
#ifndef __REACTOS__
    RxContext->CurrentIrp->IoStatus.Information =
        RxContext->Create.ReturnedCreateInformation;
#endif
    status = RxContext->CurrentIrp->IoStatus.Status = STATUS_SUCCESS;

out_free:
    if (entry)
        RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    if ((params->DesiredAccess & FILE_READ_DATA) ||
            (params->DesiredAccess & FILE_WRITE_DATA) ||
            (params->DesiredAccess & FILE_APPEND_DATA) ||
            (params->DesiredAccess & FILE_EXECUTE)) {
        InterlockedIncrement(&open.tops);
        InterlockedAdd64(&open.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_Create open delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, open.tops, open.ticks);
#endif
    } else {
        InterlockedIncrement(&lookup.tops);
        InterlockedAdd64(&lookup.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_Create lookup delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, lookup.tops, lookup.ticks);
#endif
    }
#endif
#ifdef DEBUG_OPEN
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_CollapseOpen(
#else
NTSTATUS nfs41_CollapseOpen(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;
    DbgEn();
    DbgEx();
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_ShouldTryToCollapseThisOpen(
#else
NTSTATUS nfs41_ShouldTryToCollapseThisOpen(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    if (RxContext->pRelevantSrvOpen == NULL)
        return STATUS_SUCCESS;
    else return STATUS_MORE_PROCESSING_REQUIRED;
}

#ifdef __REACTOS__
ULONG NTAPI nfs41_ExtendForCache(
#else
ULONG nfs41_ExtendForCache(
#endif
    IN OUT PRX_CONTEXT RxContext,
    IN PLARGE_INTEGER pNewFileSize,
    OUT PLARGE_INTEGER pNewAllocationSize)
{
    NTSTATUS status = STATUS_SUCCESS;
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
#ifdef DEBUG_CACHE
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    DbgEn();
    print_debug_header(RxContext);
    DbgP("input: byte count 0x%x filesize 0x%x alloc size 0x%x\n",
        LowIoContext->ParamsFor.ReadWrite.ByteCount, *pNewFileSize,
        *pNewAllocationSize);
#endif
    pNewAllocationSize->QuadPart = pNewFileSize->QuadPart + 8192;
    nfs41_fcb->StandardInfo.AllocationSize.QuadPart =
        pNewAllocationSize->QuadPart;
    nfs41_fcb->StandardInfo.EndOfFile.QuadPart = pNewFileSize->QuadPart;
#ifdef DEBUG_CACHE
    DbgP("new filesize 0x%x new allocation size 0x%x\n", *pNewFileSize,
        *pNewAllocationSize);
#endif
#ifdef DEBUG_CACHE
    DbgEx();
#endif
    return status;
}

VOID nfs41_remove_fcb_entry(
    PMRX_FCB fcb)
{
    PLIST_ENTRY pEntry;
    nfs41_fcb_list_entry *cur;
    ExAcquireFastMutex(&fcblistLock);

    pEntry = openlist.head.Flink;
    while (!IsListEmpty(&openlist.head)) {
        cur = (nfs41_fcb_list_entry *)CONTAINING_RECORD(pEntry,
                nfs41_fcb_list_entry, next);
        if (cur->fcb == fcb) {
#ifdef DEBUG_CLOSE
            DbgP("nfs41_remove_srvopen_entry: Found match for fcb=%p\n", fcb);
#endif
            RemoveEntryList(pEntry);
            RxFreePool(cur);
            break;
        }
        if (pEntry->Flink == &openlist.head) {
#ifdef DEBUG_CLOSE
            DbgP("nfs41_remove_srvopen_entry: reached EOL looking for fcb "
                "%p\n", fcb);
#endif
            break;
        }
        pEntry = pEntry->Flink;
    }
    ExReleaseFastMutex(&fcblistLock);
}

NTSTATUS map_close_errors(
    DWORD status)
{
    switch (status) {
    case NO_ERROR:              return STATUS_SUCCESS;
    case ERROR_NETNAME_DELETED: return STATUS_NETWORK_NAME_DELETED;
    case ERROR_NOT_EMPTY:       return STATUS_DIRECTORY_NOT_EMPTY;
    case ERROR_FILE_INVALID:    return STATUS_FILE_INVALID;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INTERNAL_ERROR\n", status);
    case ERROR_INTERNAL_ERROR: return STATUS_INTERNAL_ERROR;
    }
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_CloseSrvOpen(
#else
NTSTATUS nfs41_CloseSrvOpen(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    nfs41_updowncall_entry *entry;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_CLOSE
    DbgEn();
    print_debug_header(RxContext);
#endif

    if (!nfs41_fobx->deleg_type && !nfs41_fcb->StandardInfo.Directory &&
            !RxContext->pFcb->OpenCount) {
        nfs41_remove_fcb_entry(RxContext->pFcb);
    }

    status = nfs41_UpcallCreate(NFS41_CLOSE, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Close.srv_open = SrvOpen;
    if (nfs41_fcb->StandardInfo.DeletePending)
        nfs41_fcb->DeletePending = TRUE;
    if (!RxContext->pFcb->OpenCount ||
            (nfs41_fcb->StandardInfo.DeletePending &&
                nfs41_fcb->StandardInfo.Directory))
        entry->u.Close.remove = nfs41_fcb->StandardInfo.DeletePending;
    if (!RxContext->pFcb->OpenCount)
        entry->u.Close.renamed = nfs41_fcb->Renamed;

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
#ifndef USE_MOUNT_SEC_CONTEXT
    SeDeleteClientSecurity(&nfs41_fobx->sec_ctx);
#endif
    if (status) goto out;

    /* map windows ERRORs to NTSTATUS */
    status = map_close_errors(entry->status);
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&close.tops);
    InterlockedAdd64(&close.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_CloseSrvOpen delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, close.tops, close.ticks);
#endif
#endif
#ifdef DEBUG_CLOSE
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Flush(
#else
NTSTATUS nfs41_Flush(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_DeallocateForFcb(
#else
NTSTATUS nfs41_DeallocateForFcb(
#endif
    IN OUT PMRX_FCB pFcb)
{
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_DeallocateForFobx(
#else
NTSTATUS nfs41_DeallocateForFobx(
#endif
    IN OUT PMRX_FOBX pFobx)
{
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(pFobx);
    if (nfs41_fobx->acl)
        RxFreePool(nfs41_fobx->acl);
    return STATUS_SUCCESS;
}

void print_debug_filedirquery_header(
    PRX_CONTEXT RxContext)
{
    print_debug_header(RxContext);
    DbgP("FileName='%wZ', InfoClass = %s\n",
        GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext),
        print_file_information_class(RxContext->Info.FileInformationClass));
}

void print_querydir_args(
    PRX_CONTEXT RxContext)
{
    print_debug_filedirquery_header(RxContext);
    DbgP("Filter='%wZ', Index=%d, Restart/Single/Specified/Init=%d/%d/%d/%d\n",
        &RxContext->pFobx->UnicodeQueryTemplate,
        RxContext->QueryDirectory.FileIndex,
        RxContext->QueryDirectory.RestartScan,
        RxContext->QueryDirectory.ReturnSingleEntry,
        RxContext->QueryDirectory.IndexSpecified,
        RxContext->QueryDirectory.InitialQuery);
}

NTSTATUS map_querydir_errors(
    DWORD status)
{
    switch (status) {
    case ERROR_ACCESS_DENIED:       return STATUS_ACCESS_DENIED;
    case ERROR_BUFFER_OVERFLOW:     return STATUS_BUFFER_OVERFLOW;
    case ERROR_FILE_NOT_FOUND:      return STATUS_NO_SUCH_FILE;
    case ERROR_NETNAME_DELETED:     return STATUS_NETWORK_NAME_DELETED;
    case ERROR_INVALID_PARAMETER:   return STATUS_INVALID_PARAMETER;
    case ERROR_NO_MORE_FILES:       return STATUS_NO_MORE_FILES;
    case ERROR_OUTOFMEMORY:         return STATUS_INSUFFICIENT_RESOURCES;
    case ERROR_FILENAME_EXCED_RANGE: return STATUS_NAME_TOO_LONG;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_NETWORK_RESPONSE\n", status);
    case ERROR_BAD_NET_RESP:        return STATUS_INVALID_NETWORK_RESPONSE;
    }
}

NTSTATUS check_nfs41_dirquery_args(
    IN PRX_CONTEXT RxContext)
{
    if (RxContext->Info.Buffer == NULL)
        return STATUS_INVALID_USER_BUFFER;
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_QueryDirectory(
#else
NTSTATUS nfs41_QueryDirectory(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    nfs41_updowncall_entry *entry;
    FILE_INFORMATION_CLASS InfoClass = RxContext->Info.FileInformationClass;
    PUNICODE_STRING Filter = &RxContext->pFobx->UnicodeQueryTemplate;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_DIR_QUERY
    DbgEn();
    print_querydir_args(RxContext);
#endif

    status = check_nfs41_dirquery_args(RxContext);
    if (status) goto out;

    switch (InfoClass) {
    /* classes handled in readdir_copy_entry() and readdir_size_for_entry() */
    case FileNamesInformation:
    case FileDirectoryInformation:
    case FileFullDirectoryInformation:
    case FileIdFullDirectoryInformation:
    case FileBothDirectoryInformation:
    case FileIdBothDirectoryInformation:
        break;
    default:
        print_error("nfs41_QueryDirectory: unhandled dir query class %d\n",
            InfoClass);
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }
    status = nfs41_UpcallCreate(NFS41_DIR_QUERY, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.QueryFile.InfoClass = InfoClass;
    entry->buf_len = RxContext->Info.LengthRemaining;
    entry->buf = RxContext->Info.Buffer;
    entry->u.QueryFile.mdl = IoAllocateMdl(RxContext->Info.Buffer,
        RxContext->Info.LengthRemaining, FALSE, FALSE, NULL);
    if (entry->u.QueryFile.mdl == NULL) {
        status = STATUS_INTERNAL_ERROR;
        RxFreePool(entry);
        goto out;
    }
    entry->u.QueryFile.mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
    MmProbeAndLockPages(entry->u.QueryFile.mdl, KernelMode, IoModifyAccess);

    entry->u.QueryFile.filter = Filter;
    entry->u.QueryFile.initial_query = RxContext->QueryDirectory.InitialQuery;
    entry->u.QueryFile.restart_scan = RxContext->QueryDirectory.RestartScan;
    entry->u.QueryFile.return_single = RxContext->QueryDirectory.ReturnSingleEntry;

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;
    MmUnlockPages(entry->u.QueryFile.mdl);

    if (entry->status == STATUS_BUFFER_TOO_SMALL) {
        DbgP("nfs41_QueryDirectory: buffer too small provided %d need %lu\n",
            RxContext->Info.LengthRemaining, entry->buf_len);
        RxContext->InformationToReturn = entry->buf_len;
        status = STATUS_BUFFER_TOO_SMALL;
    } else if (entry->status == STATUS_SUCCESS) {
#ifdef ENABLE_TIMINGS
        InterlockedIncrement(&readdir.sops);
        InterlockedAdd64(&readdir.size, entry->u.QueryFile.buf_len);
#endif
        RxContext->Info.LengthRemaining -= entry->buf_len;
        status = STATUS_SUCCESS;
    } else {
        /* map windows ERRORs to NTSTATUS */
        status = map_querydir_errors(entry->status);
    }
    IoFreeMdl(entry->u.QueryFile.mdl);
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&readdir.tops);
    InterlockedAdd64(&readdir.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_QueryDirectory delta = %d ops=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, readdir.tops, readdir.ticks);
#endif
#endif
#ifdef DEBUG_DIR_QUERY
    DbgEx();
#endif
    return status;
}

void print_queryvolume_args(
    PRX_CONTEXT RxContext)
{
    print_debug_header(RxContext);
    DbgP("FileName='%wZ', InfoClass = %s BufferLen = %d\n",
        GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext),
        print_fs_information_class(RxContext->Info.FileInformationClass),
        RxContext->Info.LengthRemaining);
}

NTSTATUS map_volume_errors(
    DWORD status)
{
    switch (status) {
    case ERROR_ACCESS_DENIED:       return STATUS_ACCESS_DENIED;
    case ERROR_VC_DISCONNECTED:     return STATUS_CONNECTION_DISCONNECTED;
    case ERROR_NETNAME_DELETED:     return STATUS_NETWORK_NAME_DELETED;
    case ERROR_INVALID_PARAMETER:   return STATUS_INVALID_PARAMETER;
    case ERROR_OUTOFMEMORY:         return STATUS_INSUFFICIENT_RESOURCES;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_NETWORK_RESPONSE\n", status);
    case ERROR_BAD_NET_RESP:        return STATUS_INVALID_NETWORK_RESPONSE;
    }
}

void nfs41_create_volume_info(PFILE_FS_VOLUME_INFORMATION pVolInfo, DWORD *len)
{
    DECLARE_CONST_UNICODE_STRING(VolName, VOL_NAME);

    RtlZeroMemory(pVolInfo, sizeof(FILE_FS_VOLUME_INFORMATION));
    pVolInfo->VolumeSerialNumber = 0xBABAFACE;
    pVolInfo->VolumeLabelLength = VolName.Length;
    RtlCopyMemory(&pVolInfo->VolumeLabel[0], (PVOID)VolName.Buffer,
        VolName.MaximumLength);
    *len = sizeof(FILE_FS_VOLUME_INFORMATION) + VolName.Length;
}

static BOOLEAN is_root_directory(
    PRX_CONTEXT RxContext)
{
    __notnull PV_NET_ROOT VNetRoot = (PV_NET_ROOT)
        RxContext->pRelevantSrvOpen->pVNetRoot;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(RxContext->pRelevantSrvOpen->pVNetRoot);

    /* calculate the root directory's length, including vnetroot prefix,
     * mount path, and a trailing \ */
    const USHORT RootPathLen = VNetRoot->PrefixEntry.Prefix.Length +
            pVNetRootContext->MountPathLen + sizeof(WCHAR);

    return RxContext->CurrentIrpSp->FileObject->FileName.Length <= RootPathLen;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_QueryVolumeInformation(
#else
NTSTATUS nfs41_QueryVolumeInformation(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    nfs41_updowncall_entry *entry;
    ULONG RemainingLength = RxContext->Info.LengthRemaining, SizeUsed;
    FS_INFORMATION_CLASS InfoClass = RxContext->Info.FsInformationClass;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
    NFS41GetDeviceExtension(RxContext, DevExt);

#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_VOLUME_QUERY
    DbgEn();
    print_queryvolume_args(RxContext);
#endif

    status = check_nfs41_dirquery_args(RxContext);
    if (status) goto out;

#ifdef __REACTOS__
    RtlZeroMemory(RxContext->Info.Buffer, RxContext->Info.LengthRemaining);
#endif // __REACTOS__

    switch (InfoClass) {
    case FileFsVolumeInformation:
        if ((ULONG)RxContext->Info.LengthRemaining >= DevExt->VolAttrsLen) {
            RtlCopyMemory(RxContext->Info.Buffer, DevExt->VolAttrs,
                DevExt->VolAttrsLen);
            RxContext->Info.LengthRemaining -= DevExt->VolAttrsLen;
            status = STATUS_SUCCESS;
        } else {
            RtlCopyMemory(RxContext->Info.Buffer, DevExt->VolAttrs,
                RxContext->Info.LengthRemaining);
            status = STATUS_BUFFER_OVERFLOW;
        }
        goto out;
    case FileFsDeviceInformation:
    {
        PFILE_FS_DEVICE_INFORMATION pDevInfo = RxContext->Info.Buffer;

        SizeUsed = sizeof(FILE_FS_DEVICE_INFORMATION);
        if (RemainingLength < SizeUsed) {
            status = STATUS_BUFFER_TOO_SMALL;
            RxContext->InformationToReturn = SizeUsed;
            goto out;
        }
        pDevInfo->DeviceType = RxContext->pFcb->pNetRoot->DeviceType;
        pDevInfo->Characteristics = FILE_REMOTE_DEVICE | FILE_DEVICE_IS_MOUNTED;
        RxContext->Info.LengthRemaining -= SizeUsed;
        status = STATUS_SUCCESS;
        goto out;
    }
    case FileAccessInformation:
        status = STATUS_NOT_SUPPORTED;
        goto out;

    case FileFsAttributeInformation:
        if (RxContext->Info.LengthRemaining < FS_ATTR_LEN) {
            RxContext->InformationToReturn = FS_ATTR_LEN;
            status = STATUS_BUFFER_TOO_SMALL;
            goto out;
        }

        /* on attribute queries for the root directory,
         * use cached volume attributes from mount */
        if (is_root_directory(RxContext)) {
            PFILE_FS_ATTRIBUTE_INFORMATION attrs =
                (PFILE_FS_ATTRIBUTE_INFORMATION)RxContext->Info.Buffer;
            DECLARE_CONST_UNICODE_STRING(FsName, FS_NAME);

            RtlCopyMemory(attrs, &pVNetRootContext->FsAttrs,
                sizeof(pVNetRootContext->FsAttrs));

            /* fill in the FileSystemName */
            RtlCopyMemory(attrs->FileSystemName, FsName.Buffer,
                FsName.MaximumLength); /* 'MaximumLength' to include null */
            attrs->FileSystemNameLength = FsName.Length;

            RxContext->Info.LengthRemaining -= FS_ATTR_LEN;
            goto out;
        }
        /* else fall through and send the upcall */
    case FileFsSizeInformation:
    case FileFsFullSizeInformation:
        break;

    default:
        print_error("nfs41_QueryVolumeInformation: unhandled class %d\n", InfoClass);
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }
    status = nfs41_UpcallCreate(NFS41_VOLUME_QUERY, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Volume.query = InfoClass;
    entry->buf = RxContext->Info.Buffer;
    entry->buf_len = RxContext->Info.LengthRemaining;

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    if (entry->status == STATUS_BUFFER_TOO_SMALL) {
        RxContext->InformationToReturn = entry->buf_len;
        status = STATUS_BUFFER_TOO_SMALL;
    } else if (entry->status == STATUS_SUCCESS) {
        if (InfoClass == FileFsAttributeInformation) {
            /* fill in the FileSystemName */
            PFILE_FS_ATTRIBUTE_INFORMATION attrs =
                (PFILE_FS_ATTRIBUTE_INFORMATION)RxContext->Info.Buffer;
            DECLARE_CONST_UNICODE_STRING(FsName, FS_NAME);

            RtlCopyMemory(attrs->FileSystemName, FsName.Buffer,
                FsName.MaximumLength); /* 'MaximumLength' to include null */
            attrs->FileSystemNameLength = FsName.Length;

            entry->buf_len = FS_ATTR_LEN;
        }
#ifdef ENABLE_TIMINGS
        InterlockedIncrement(&volume.sops);
        InterlockedAdd64(&volume.size, entry->u.Volume.buf_len);
#endif
        RxContext->Info.LengthRemaining -= entry->buf_len;
        status = STATUS_SUCCESS;
    } else {
        status = map_volume_errors(entry->status);
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&volume.tops);
    InterlockedAdd64(&volume.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_QueryVolumeInformation delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, volume.tops, volume.ticks);
#endif
#endif
#ifdef DEBUG_VOLUME_QUERY
    DbgEx();
#endif
    return status;
}

VOID nfs41_update_fcb_list(
    PMRX_FCB fcb,
    ULONGLONG ChangeTime)
{
    PLIST_ENTRY pEntry;
    nfs41_fcb_list_entry *cur;
    ExAcquireFastMutex(&fcblistLock);
    pEntry = openlist.head.Flink;
    while (!IsListEmpty(&openlist.head)) {
        cur = (nfs41_fcb_list_entry *)CONTAINING_RECORD(pEntry,
                nfs41_fcb_list_entry, next);
        if (cur->fcb == fcb &&
                cur->ChangeTime != ChangeTime) {
#if defined(DEBUG_FILE_SET) || defined(DEBUG_ACL_SET) || \
    defined(DEBUG_WRITE) || defined(DEBUG_EA_SET)
            DbgP("nfs41_update_fcb_list: Found match for fcb %p: updating "
                "%llu to %llu\n", fcb, cur->ChangeTime, ChangeTime);
#endif
            cur->ChangeTime = ChangeTime;
            break;
        }
        /* place an upcall for this srv_open */
        if (pEntry->Flink == &openlist.head) {
#if defined(DEBUG_FILE_SET) || defined(DEBUG_ACL_SET) || \
    defined(DEBUG_WRITE) || defined(DEBUG_EA_SET)
            DbgP("nfs41_update_fcb_list: reached EOL loooking for "
                "fcb=%p\n", fcb);
#endif
            break;
        }
        pEntry = pEntry->Flink;
    }
    ExReleaseFastMutex(&fcblistLock);
}

void print_nfs3_attrs(
    nfs3_attrs *attrs)
{
    DbgP("type=%d mode=%o nlink=%d size=%d atime=%x mtime=%x ctime=%x\n",
        attrs->type, attrs->mode, attrs->nlink, attrs->size, attrs->atime,
        attrs->mtime, attrs->ctime);
}

void file_time_to_nfs_time(
    IN const PLARGE_INTEGER file_time,
    OUT LONGLONG *nfs_time)
{
    LARGE_INTEGER diff = unix_time_diff;
    diff.QuadPart = file_time->QuadPart - diff.QuadPart;
    *nfs_time = diff.QuadPart / 10000000;
}

void create_nfs3_attrs(
    nfs3_attrs *attrs,
    PNFS41_FCB nfs41_fcb)
{
    RtlZeroMemory(attrs, sizeof(nfs3_attrs));
    if (nfs41_fcb->BasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        attrs->type = NF3LNK;
    else if (nfs41_fcb->StandardInfo.Directory)
        attrs->type = NF3DIR;
    else
        attrs->type = NF3REG;
    attrs->mode = nfs41_fcb->mode;
    attrs->nlink = nfs41_fcb->StandardInfo.NumberOfLinks;
    attrs->size.QuadPart = attrs->used.QuadPart =
        nfs41_fcb->StandardInfo.EndOfFile.QuadPart;
    file_time_to_nfs_time(&nfs41_fcb->BasicInfo.LastAccessTime, &attrs->atime);
    file_time_to_nfs_time(&nfs41_fcb->BasicInfo.ChangeTime, &attrs->mtime);
    file_time_to_nfs_time(&nfs41_fcb->BasicInfo.CreationTime, &attrs->ctime);
}


NTSTATUS map_setea_error(
    DWORD error)
{
    switch (error) {
    case NO_ERROR:                      return STATUS_SUCCESS;
    case ERROR_FILE_NOT_FOUND:          return STATUS_NO_EAS_ON_FILE;
    case ERROR_ACCESS_DENIED:           return STATUS_ACCESS_DENIED;
    case ERROR_NETWORK_ACCESS_DENIED:   return STATUS_NETWORK_ACCESS_DENIED;
    case ERROR_NETNAME_DELETED:         return STATUS_NETWORK_NAME_DELETED;
    case ERROR_FILE_TOO_LARGE:          return STATUS_EA_TOO_LARGE;
    case ERROR_BUFFER_OVERFLOW:         return STATUS_BUFFER_OVERFLOW;
    case STATUS_BUFFER_TOO_SMALL:
    case ERROR_INSUFFICIENT_BUFFER:     return STATUS_BUFFER_TOO_SMALL;
    case ERROR_INVALID_EA_HANDLE:       return STATUS_NONEXISTENT_EA_ENTRY;
    case ERROR_NO_MORE_FILES:           return STATUS_NO_MORE_EAS;
    case ERROR_EA_FILE_CORRUPT:         return STATUS_EA_CORRUPT_ERROR;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_PARAMETER\n", error);
    case ERROR_INVALID_PARAMETER:       return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS check_nfs41_setea_args(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(RxContext->pRelevantSrvOpen->pVNetRoot);
    __notnull PFILE_FS_ATTRIBUTE_INFORMATION FsAttrs =
        &pVNetRootContext->FsAttrs;
    __notnull PFILE_FULL_EA_INFORMATION ea =
        (PFILE_FULL_EA_INFORMATION)RxContext->Info.Buffer;

    status = check_nfs41_dirquery_args(RxContext);
    if (status) goto out;

    if (ea == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto out;
    }
    if (AnsiStrEq(&NfsActOnLink, ea->EaName, ea->EaNameLength) ||
        AnsiStrEq(&NfsSymlinkTargetName, ea->EaName, ea->EaNameLength)) {
        status = STATUS_INVALID_PARAMETER; /* only allowed on create */
        goto out;
    }
    /* ignore cygwin EAs when checking support */
    if (!(FsAttrs->FileSystemAttributes & FILE_SUPPORTS_EXTENDED_ATTRIBUTES)
        && !AnsiStrEq(&NfsV3Attributes, ea->EaName, ea->EaNameLength)) {
        status = STATUS_EAS_NOT_SUPPORTED;
        goto out;
    }
    if ((RxContext->pRelevantSrvOpen->DesiredAccess & FILE_WRITE_EA) == 0) {
        status = STATUS_ACCESS_DENIED;
        goto out;
    }
    if (pVNetRootContext->read_only) {
        print_error("check_nfs41_setattr_args: Read-only mount\n");
        status = STATUS_ACCESS_DENIED;
        goto out;
    }
out:
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_SetEaInformation(
#else
NTSTATUS nfs41_SetEaInformation(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_EAS_NOT_SUPPORTED;
    nfs41_updowncall_entry *entry;
    __notnull PFILE_FULL_EA_INFORMATION eainfo =
        (PFILE_FULL_EA_INFORMATION)RxContext->Info.Buffer;
    nfs3_attrs *attrs = NULL;
    ULONG buflen = RxContext->CurrentIrpSp->Parameters.SetEa.Length, error_offset;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_EA_SET
    DbgEn();
    print_debug_header(RxContext);
    print_ea_info(1, eainfo);
#endif

    status = check_nfs41_setea_args(RxContext);
    if (status) goto out;

    status = nfs41_UpcallCreate(NFS41_EA_SET, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    if (AnsiStrEq(&NfsV3Attributes, eainfo->EaName, eainfo->EaNameLength)) {
        attrs = (nfs3_attrs *)(eainfo->EaName + eainfo->EaNameLength + 1);
#ifdef DEBUG_EA_SET
        print_nfs3_attrs(attrs);
        DbgP("old mode is %o new mode is %o\n", nfs41_fcb->mode, attrs->mode);
#endif
        entry->u.SetEa.mode = attrs->mode;
    } else {
        entry->u.SetEa.mode = 0;
        status = IoCheckEaBufferValidity(eainfo, buflen, &error_offset);
        if (status) {
            RxFreePool(entry);
            goto out;
        }
    }
    entry->buf = eainfo;
    entry->buf_len = buflen;

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;
#ifdef ENABLE_TIMINGS
    if (entry->status == STATUS_SUCCESS) {
        InterlockedIncrement(&setexattr.sops);
        InterlockedAdd64(&setexattr.size, entry->u.SetEa.buf_len);
    }
#endif
    status = map_setea_error(entry->status);
    if (!status) {
        if (!nfs41_fobx->deleg_type && entry->ChangeTime &&
                (SrvOpen->DesiredAccess &
                (FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA)))
            nfs41_update_fcb_list(RxContext->pFcb, entry->ChangeTime);
        nfs41_fcb->changeattr = entry->ChangeTime;
        nfs41_fcb->mode = entry->u.SetEa.mode;
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&setexattr.tops);
    InterlockedAdd64(&setexattr.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_SetEaInformation delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, setexattr.tops, setexattr.ticks);
#endif
#endif
#ifdef DEBUG_EA_SET
    DbgEx();
#endif
    return status;
}

NTSTATUS check_nfs41_queryea_args(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(RxContext->pRelevantSrvOpen->pVNetRoot);
    __notnull PFILE_FS_ATTRIBUTE_INFORMATION FsAttrs =
        &pVNetRootContext->FsAttrs;
    PFILE_GET_EA_INFORMATION ea = (PFILE_GET_EA_INFORMATION)
            RxContext->CurrentIrpSp->Parameters.QueryEa.EaList;

    status = check_nfs41_dirquery_args(RxContext);
    if (status) goto out;

    if (!(FsAttrs->FileSystemAttributes & FILE_SUPPORTS_EXTENDED_ATTRIBUTES)) {
        if (ea == NULL) {
            status = STATUS_EAS_NOT_SUPPORTED;
            goto out;
        }
        /* ignore cygwin EAs when checking support */
        if (!AnsiStrEq(&NfsV3Attributes, ea->EaName, ea->EaNameLength) &&
            !AnsiStrEq(&NfsActOnLink, ea->EaName, ea->EaNameLength) &&
            !AnsiStrEq(&NfsSymlinkTargetName, ea->EaName, ea->EaNameLength)) {
            status = STATUS_EAS_NOT_SUPPORTED;
            goto out;
        }
    }
    if ((RxContext->pRelevantSrvOpen->DesiredAccess & FILE_READ_EA) == 0) {
        status = STATUS_ACCESS_DENIED;
        goto out;
    }
out:
    return status;
}

static NTSTATUS QueryCygwinSymlink(
    IN OUT PRX_CONTEXT RxContext,
    IN PFILE_GET_EA_INFORMATION query,
    OUT PFILE_FULL_EA_INFORMATION info)
{
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION VNetRootContext =
            NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION NetRootContext =
            NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FOBX Fobx = NFS41GetFobxExtension(RxContext->pFobx);
    nfs41_updowncall_entry *entry;
    UNICODE_STRING TargetName;
    const USHORT HeaderLen = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) +
        query->EaNameLength + 1;
    NTSTATUS status;

    if (RxContext->Info.LengthRemaining < HeaderLen) {
        status = STATUS_BUFFER_TOO_SMALL;
        RxContext->InformationToReturn = HeaderLen;
        goto out;
    }

    TargetName.Buffer = (PWCH)(info->EaName + query->EaNameLength + 1);
    TargetName.MaximumLength = (USHORT)min(RxContext->Info.LengthRemaining -
        HeaderLen, 0xFFFF);

    status = nfs41_UpcallCreate(NFS41_SYMLINK, &Fobx->sec_ctx,
        VNetRootContext->session, Fobx->nfs41_open_state,
        NetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Symlink.target = &TargetName;
    entry->u.Symlink.set = FALSE;

    status = nfs41_UpcallWaitForReply(entry, VNetRootContext->timeout);
    if (status) goto out;

    status = map_setea_error(entry->status);
    if (status == STATUS_SUCCESS) {
        info->NextEntryOffset = 0;
        info->Flags = 0;
        info->EaNameLength = query->EaNameLength;
        info->EaValueLength = TargetName.Length - sizeof(UNICODE_NULL);
        TargetName.Buffer[TargetName.Length/sizeof(WCHAR)] = UNICODE_NULL;
        RtlCopyMemory(info->EaName, query->EaName, query->EaNameLength);
        RxContext->Info.LengthRemaining = HeaderLen + info->EaValueLength;
    } else if (status == STATUS_BUFFER_TOO_SMALL) {
        RxContext->InformationToReturn = HeaderLen +
            entry->u.Symlink.target->Length;
    }
    RxFreePool(entry);
out:
    return status;
}

static NTSTATUS QueryCygwinEA(
    IN OUT PRX_CONTEXT RxContext,
    IN PFILE_GET_EA_INFORMATION query,
    OUT PFILE_FULL_EA_INFORMATION info)
{
    NTSTATUS status = STATUS_NONEXISTENT_EA_ENTRY;

    if (query == NULL)
        goto out;

    if (AnsiStrEq(&NfsSymlinkTargetName, query->EaName, query->EaNameLength)) {
        __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
        if (nfs41_fcb->BasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            status = QueryCygwinSymlink(RxContext, query, info);
            goto out;
        } else {
            const LONG LengthRequired = sizeof(FILE_FULL_EA_INFORMATION) +
                NfsSymlinkTargetName.Length - sizeof(CHAR);
            if (LengthRequired > RxContext->Info.LengthRemaining) {
                status = STATUS_BUFFER_TOO_SMALL;
                RxContext->InformationToReturn = LengthRequired;
                goto out;
            }
            info->NextEntryOffset = 0;
            info->Flags = 0;
            info->EaValueLength = 0;
            info->EaNameLength = (UCHAR)NfsActOnLink.Length;
            RtlCopyMemory(info->EaName, NfsSymlinkTargetName.Buffer,
                NfsSymlinkTargetName.Length);
            RxContext->Info.LengthRemaining = LengthRequired;
            status = STATUS_SUCCESS;
            goto out;
        }
    }

    if (AnsiStrEq(&NfsV3Attributes, query->EaName, query->EaNameLength)) {
        nfs3_attrs attrs;

        const LONG LengthRequired = sizeof(FILE_FULL_EA_INFORMATION) +
            NfsV3Attributes.Length + sizeof(nfs3_attrs) - sizeof(CHAR);
        if (LengthRequired > RxContext->Info.LengthRemaining) {
            status = STATUS_BUFFER_TOO_SMALL;
            RxContext->InformationToReturn = LengthRequired;
            goto out;
        }

        create_nfs3_attrs(&attrs, NFS41GetFcbExtension(RxContext->pFcb));
#ifdef DEBUG_EA_QUERY
        print_nfs3_attrs(&attrs);
#endif

        info->NextEntryOffset = 0;
        info->Flags = 0;
        info->EaNameLength = (UCHAR)NfsV3Attributes.Length;
        info->EaValueLength = sizeof(nfs3_attrs);
        RtlCopyMemory(info->EaName, NfsV3Attributes.Buffer,
            NfsV3Attributes.Length);
        RtlCopyMemory(info->EaName + info->EaNameLength + 1, &attrs,
            sizeof(nfs3_attrs));
        RxContext->Info.LengthRemaining = LengthRequired;
        status = STATUS_SUCCESS;
        goto out;
    }

    if (AnsiStrEq(&NfsActOnLink, query->EaName, query->EaNameLength)) {

        const LONG LengthRequired = sizeof(FILE_FULL_EA_INFORMATION) +
            query->EaNameLength - sizeof(CHAR);
        if (LengthRequired > RxContext->Info.LengthRemaining) {
            status = STATUS_BUFFER_TOO_SMALL;
            RxContext->InformationToReturn = LengthRequired;
            goto out;
        }

        info->NextEntryOffset = 0;
        info->Flags = 0;
        info->EaNameLength = query->EaNameLength;
        info->EaValueLength = 0;
        RtlCopyMemory(info->EaName, query->EaName, query->EaNameLength);
        RxContext->Info.LengthRemaining = LengthRequired;
        status = STATUS_SUCCESS;
        goto out;
    }
out:
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_QueryEaInformation(
#else
NTSTATUS nfs41_QueryEaInformation(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_EAS_NOT_SUPPORTED;
    nfs41_updowncall_entry *entry;
    PFILE_GET_EA_INFORMATION query = (PFILE_GET_EA_INFORMATION)
            RxContext->CurrentIrpSp->Parameters.QueryEa.EaList;
    ULONG buflen = RxContext->CurrentIrpSp->Parameters.QueryEa.Length;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
            NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
            NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_EA_QUERY
    DbgEn();
    print_debug_header(RxContext);
    print_get_ea(1, query);
#endif
    status = check_nfs41_queryea_args(RxContext);
    if (status) goto out;

    /* handle queries for cygwin EAs */
    status = QueryCygwinEA(RxContext, query,
        (PFILE_FULL_EA_INFORMATION)RxContext->Info.Buffer);
    if (status != STATUS_NONEXISTENT_EA_ENTRY)
        goto out;

    status = nfs41_UpcallCreate(NFS41_EA_GET, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->buf_len = buflen;
    entry->buf = RxContext->Info.Buffer;
    entry->u.QueryEa.EaList = query;
    entry->u.QueryEa.EaListLength = query == NULL ? 0 :
        RxContext->QueryEa.UserEaListLength;
    entry->u.QueryEa.EaIndex = RxContext->QueryEa.IndexSpecified ?
        RxContext->QueryEa.UserEaIndex : 0;
    entry->u.QueryEa.RestartScan = RxContext->QueryEa.RestartScan;
    entry->u.QueryEa.ReturnSingleEntry = RxContext->QueryEa.ReturnSingleEntry;

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    if (entry->status == STATUS_SUCCESS) {
        switch (entry->u.QueryEa.Overflow) {
        case ERROR_INSUFFICIENT_BUFFER:
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        case ERROR_BUFFER_OVERFLOW:
            status = RxContext->IoStatusBlock.Status = STATUS_BUFFER_OVERFLOW;
            break;
        default:
            RxContext->IoStatusBlock.Status = STATUS_SUCCESS;
            break;
        }
        RxContext->InformationToReturn = entry->buf_len;
#ifdef ENABLE_TIMINGS
        InterlockedIncrement(&getexattr.sops);
        InterlockedAdd64(&getexattr.size, entry->u.QueryEa.buf_len);
#endif
    } else {
        status = map_setea_error(entry->status);
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&getexattr.tops);
    InterlockedAdd64(&getexattr.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_QueryEaInformation delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, getexattr.tops, getexattr.ticks);
#endif
#endif
#ifdef DEBUG_EA_QUERY
    DbgEx();
#endif
    return status;
}

NTSTATUS map_query_acl_error(
    DWORD error)
{
    switch (error) {
    case NO_ERROR:                  return STATUS_SUCCESS;
    case ERROR_NOT_SUPPORTED:       return STATUS_NOT_SUPPORTED;
    case ERROR_ACCESS_DENIED:       return STATUS_ACCESS_DENIED;
    case ERROR_FILE_NOT_FOUND:      return STATUS_OBJECT_NAME_NOT_FOUND;
    case ERROR_INVALID_PARAMETER:   return STATUS_INVALID_PARAMETER;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_NETWORK_RESPONSE\n", error);
    case ERROR_BAD_NET_RESP:        return STATUS_INVALID_NETWORK_RESPONSE;
    }
}

NTSTATUS check_nfs41_getacl_args(
    PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_INFORMATION info_class =
        RxContext->CurrentIrpSp->Parameters.QuerySecurity.SecurityInformation;

    /* we don't support sacls */
    if (info_class == SACL_SECURITY_INFORMATION ||
            info_class == LABEL_SECURITY_INFORMATION) {
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }
    if (RxContext->CurrentIrp->UserBuffer == NULL &&
            RxContext->CurrentIrpSp->Parameters.QuerySecurity.Length)
        status = STATUS_INVALID_USER_BUFFER;
out:
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_QuerySecurityInformation(
#else
NTSTATUS nfs41_QuerySecurityInformation(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    nfs41_updowncall_entry *entry;
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    SECURITY_INFORMATION info_class =
        RxContext->CurrentIrpSp->Parameters.QuerySecurity.SecurityInformation;
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_ACL_QUERY
    DbgEn();
    print_debug_header(RxContext);
    print_acl_args(info_class);
#endif

    status = check_nfs41_getacl_args(RxContext);
    if (status) goto out;

    if (nfs41_fobx->acl && nfs41_fobx->acl_len) {
        LARGE_INTEGER current_time;
        KeQuerySystemTime(&current_time);
#ifdef DEBUG_ACL_QUERY
        DbgP("CurrentTime %lx Saved Acl time %lx\n",
            current_time.QuadPart, nfs41_fobx->time.QuadPart);
#endif
        if (current_time.QuadPart - nfs41_fobx->time.QuadPart <= 20*1000) {
            PSECURITY_DESCRIPTOR sec_desc = (PSECURITY_DESCRIPTOR)
                RxContext->CurrentIrp->UserBuffer;
            RtlCopyMemory(sec_desc, nfs41_fobx->acl, nfs41_fobx->acl_len);
            RxContext->IoStatusBlock.Information =
                RxContext->InformationToReturn = nfs41_fobx->acl_len;
            RxContext->IoStatusBlock.Status = status = STATUS_SUCCESS;
#ifdef ENABLE_TIMINGS
            InterlockedIncrement(&getacl.sops);
            InterlockedAdd64(&getacl.size, nfs41_fobx->acl_len);
#endif
        } else status = 1;
        RxFreePool(nfs41_fobx->acl);
        nfs41_fobx->acl = NULL;
        nfs41_fobx->acl_len = 0;
        if (!status)
            goto out;
    }

    status = nfs41_UpcallCreate(NFS41_ACL_QUERY, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Acl.query = info_class;
    /* we can't provide RxContext->CurrentIrp->UserBuffer to the upcall thread
     * because it becomes an invalid pointer with that execution context
     */
    entry->buf_len = RxContext->CurrentIrpSp->Parameters.QuerySecurity.Length;

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    if (entry->status == STATUS_BUFFER_TOO_SMALL) {
#ifdef DEBUG_ACL_QUERY
        DbgP("nfs41_QuerySecurityInformation: provided buffer size=%d but we "
             "need %lu\n",
             RxContext->CurrentIrpSp->Parameters.QuerySecurity.Length,
             entry->buf_len);
#endif
        status = STATUS_BUFFER_OVERFLOW;
        RxContext->InformationToReturn = entry->buf_len;

        /* Save ACL buffer */
        nfs41_fobx->acl = entry->buf;
        nfs41_fobx->acl_len = entry->buf_len;
        KeQuerySystemTime(&nfs41_fobx->time);
    } else if (entry->status == STATUS_SUCCESS) {
        PSECURITY_DESCRIPTOR sec_desc = (PSECURITY_DESCRIPTOR)
            RxContext->CurrentIrp->UserBuffer;
        RtlCopyMemory(sec_desc, entry->buf, entry->buf_len);
#ifdef ENABLE_TIMINGS
        InterlockedIncrement(&getacl.sops);
        InterlockedAdd64(&getacl.size, entry->u.Acl.buf_len);
#endif
        RxFreePool(entry->buf);
        nfs41_fobx->acl = NULL;
        nfs41_fobx->acl_len = 0;
        RxContext->IoStatusBlock.Information = RxContext->InformationToReturn =
            entry->buf_len;
        RxContext->IoStatusBlock.Status = status = STATUS_SUCCESS;
    } else {
        status = map_query_acl_error(entry->status);
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    /* only count getacl that we made an upcall for */
    if (status == STATUS_BUFFER_OVERFLOW) {
        InterlockedIncrement(&getacl.tops);
        InterlockedAdd64(&getacl.ticks, t2.QuadPart - t1.QuadPart);
    }
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_QuerySecurityInformation: delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, getacl.tops, getacl.ticks);
#endif
#endif
#ifdef DEBUG_ACL_QUERY
    DbgEx();
#endif
    return status;
}

NTSTATUS check_nfs41_setacl_args(
    PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(RxContext->pRelevantSrvOpen->pVNetRoot);
    SECURITY_INFORMATION info_class =
        RxContext->CurrentIrpSp->Parameters.SetSecurity.SecurityInformation;

    if (pVNetRootContext->read_only) {
        print_error("check_nfs41_setacl_args: Read-only mount\n");
        status = STATUS_ACCESS_DENIED;
        goto out;
    }
    /* we don't support sacls */
    if (info_class == SACL_SECURITY_INFORMATION  ||
            info_class == LABEL_SECURITY_INFORMATION) {
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }
out:
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_SetSecurityInformation(
#else
NTSTATUS nfs41_SetSecurityInformation(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    nfs41_updowncall_entry *entry;
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PSECURITY_DESCRIPTOR sec_desc =
        RxContext->CurrentIrpSp->Parameters.SetSecurity.SecurityDescriptor;
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
    SECURITY_INFORMATION info_class =
        RxContext->CurrentIrpSp->Parameters.SetSecurity.SecurityInformation;
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_ACL_SET
    DbgEn();
    print_debug_header(RxContext);
    print_acl_args(info_class);
#endif

    status = check_nfs41_setacl_args(RxContext);
    if (status) goto out;

    /* check that ACL is present */
    if (info_class & DACL_SECURITY_INFORMATION) {
        PACL acl;
        BOOLEAN present, dacl_default;
        status = RtlGetDaclSecurityDescriptor(sec_desc, &present, &acl,
                    &dacl_default);
        if (status) {
            DbgP("RtlGetDaclSecurityDescriptor failed %x\n", status);
            goto out;
        }
        if (present == FALSE) {
            DbgP("NO ACL present\n");
            goto out;
        }
    }

    status = nfs41_UpcallCreate(NFS41_ACL_SET, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Acl.query = info_class;
    entry->buf = sec_desc;
    entry->buf_len = RtlLengthSecurityDescriptor(sec_desc);
#ifdef ENABLE_TIMINGS
    InterlockedIncrement(&setacl.sops);
    InterlockedAdd64(&setacl.size, entry->u.Acl.buf_len);
#endif

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    status = map_query_acl_error(entry->status);
    if (!status) {
        if (!nfs41_fobx->deleg_type && entry->ChangeTime &&
                (SrvOpen->DesiredAccess &
                (FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA)))
            nfs41_update_fcb_list(RxContext->pFcb, entry->ChangeTime);
        nfs41_fcb->changeattr = entry->ChangeTime;
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&setacl.tops);
    InterlockedAdd64(&setacl.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_SetSecurityInformation delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, setacl.tops, setacl.ticks);
#endif
#endif
#ifdef DEBUG_ACL_SET
    DbgEx();
#endif
    return status;
}

NTSTATUS map_queryfile_error(
    DWORD error)
{
    switch (error) {
    case ERROR_ACCESS_DENIED:       return STATUS_ACCESS_DENIED;
    case ERROR_NETNAME_DELETED:     return STATUS_NETWORK_NAME_DELETED;
    case ERROR_INVALID_PARAMETER:   return STATUS_INVALID_PARAMETER;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_NETWORK_RESPONSE\n", error);
    case ERROR_BAD_NET_RESP:        return STATUS_INVALID_NETWORK_RESPONSE;
    }
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_QueryFileInformation(
#else
NTSTATUS nfs41_QueryFileInformation(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    FILE_INFORMATION_CLASS InfoClass = RxContext->Info.FileInformationClass;
    nfs41_updowncall_entry *entry;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_FILE_QUERY
    DbgEn();
    print_debug_filedirquery_header(RxContext);
#endif

    status = check_nfs41_dirquery_args(RxContext);
    if (status) goto out;

    switch (InfoClass) {
    case FileEaInformation:
    {
        PFILE_EA_INFORMATION info =
            (PFILE_EA_INFORMATION)RxContext->Info.Buffer;
        info->EaSize = 0;
        RxContext->Info.LengthRemaining -= sizeof(FILE_EA_INFORMATION);
        status = STATUS_SUCCESS;
        goto out;
    }
    case FileBasicInformation:
    case FileStandardInformation:
    case FileInternalInformation:
    case FileAttributeTagInformation:
    case FileNetworkOpenInformation:
        break;
    default:
        print_error("nfs41_QueryFileInformation: unhandled class %d\n", InfoClass);
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }

    status = nfs41_UpcallCreate(NFS41_FILE_QUERY, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.QueryFile.InfoClass = InfoClass;
    entry->buf = RxContext->Info.Buffer;
    entry->buf_len = RxContext->Info.LengthRemaining;

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    if (entry->status == STATUS_BUFFER_TOO_SMALL) {
        RxContext->InformationToReturn = entry->buf_len;
        status = STATUS_BUFFER_TOO_SMALL;
    } else if (entry->status == STATUS_SUCCESS) {
        BOOLEAN DeletePending = FALSE;
#ifdef ENABLE_TIMINGS
        InterlockedIncrement(&getattr.sops);
        InterlockedAdd64(&getattr.size, entry->u.QueryFile.buf_len);
#endif
        RxContext->Info.LengthRemaining -= entry->buf_len;
        status = STATUS_SUCCESS;

        switch (InfoClass) {
        case FileBasicInformation:
            RtlCopyMemory(&nfs41_fcb->BasicInfo, RxContext->Info.Buffer,
                sizeof(nfs41_fcb->BasicInfo));
#ifdef DEBUG_FILE_QUERY
            print_basic_info(1, &nfs41_fcb->BasicInfo);
#endif
            break;
        case FileStandardInformation:
            /* this a fix for RDBSS behaviour when it first calls ExtendForCache,
             * then it sends a file query irp for standard attributes and
             * expects to receive EndOfFile of value set by the ExtendForCache.
             * It seems to cache the filesize based on that instead of sending
             * a file size query for after doing the write.
             */
        {
            PFILE_STANDARD_INFORMATION std_info;
            std_info = (PFILE_STANDARD_INFORMATION)RxContext->Info.Buffer;
            if (nfs41_fcb->StandardInfo.AllocationSize.QuadPart >
                    std_info->AllocationSize.QuadPart) {
#ifdef DEBUG_FILE_QUERY
                DbgP("Old AllocationSize is bigger: saving %x\n",
                    nfs41_fcb->StandardInfo.AllocationSize.QuadPart);
#endif
                std_info->AllocationSize.QuadPart =
                    nfs41_fcb->StandardInfo.AllocationSize.QuadPart;
            }
            if (nfs41_fcb->StandardInfo.EndOfFile.QuadPart >
                    std_info->EndOfFile.QuadPart) {
#ifdef DEBUG_FILE_QUERY
                DbgP("Old EndOfFile is bigger: saving %x\n",
                    nfs41_fcb->StandardInfo.EndOfFile);
#endif
                std_info->EndOfFile.QuadPart =
                    nfs41_fcb->StandardInfo.EndOfFile.QuadPart;
            }
            std_info->DeletePending = nfs41_fcb->DeletePending;
        }
            if (nfs41_fcb->StandardInfo.DeletePending)
                DeletePending = TRUE;
            RtlCopyMemory(&nfs41_fcb->StandardInfo, RxContext->Info.Buffer,
                sizeof(nfs41_fcb->StandardInfo));
            nfs41_fcb->StandardInfo.DeletePending = DeletePending;
#ifdef DEBUG_FILE_QUERY
            print_std_info(1, &nfs41_fcb->StandardInfo);
#endif
            break;
        }
    } else {
        status = map_queryfile_error(entry->status);
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&getattr.tops);
    InterlockedAdd64(&getattr.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_QueryFileInformation delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, getattr.tops, getattr.ticks);
#endif
#endif
#ifdef DEBUG_FILE_QUERY
    DbgEx();
#endif
    return status;
}

NTSTATUS map_setfile_error(
    DWORD error)
{
    switch (error) {
    case NO_ERROR:                      return STATUS_SUCCESS;
    case ERROR_NOT_EMPTY:               return STATUS_DIRECTORY_NOT_EMPTY;
    case ERROR_FILE_EXISTS:             return STATUS_OBJECT_NAME_COLLISION;
    case ERROR_FILE_NOT_FOUND:          return STATUS_OBJECT_NAME_NOT_FOUND;
    case ERROR_PATH_NOT_FOUND:          return STATUS_OBJECT_PATH_NOT_FOUND;
    case ERROR_ACCESS_DENIED:           return STATUS_ACCESS_DENIED;
    case ERROR_FILE_INVALID:            return STATUS_FILE_INVALID;
    case ERROR_NOT_SAME_DEVICE:         return STATUS_NOT_SAME_DEVICE;
    case ERROR_NOT_SUPPORTED:           return STATUS_NOT_IMPLEMENTED;
    case ERROR_NETWORK_ACCESS_DENIED:   return STATUS_NETWORK_ACCESS_DENIED;
    case ERROR_NETNAME_DELETED:         return STATUS_NETWORK_NAME_DELETED;
    case ERROR_BUFFER_OVERFLOW:         return STATUS_INSUFFICIENT_RESOURCES;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_PARAMETER\n", error);
    case ERROR_INVALID_PARAMETER:       return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS check_nfs41_setattr_args(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    FILE_INFORMATION_CLASS InfoClass = RxContext->Info.FileInformationClass;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(RxContext->pRelevantSrvOpen->pVNetRoot);

    if (pVNetRootContext->read_only) {
        print_error("check_nfs41_setattr_args: Read-only mount\n");
        status = STATUS_ACCESS_DENIED;
        goto out;
    }

    /* http://msdn.microsoft.com/en-us/library/ff469355(v=PROT.10).aspx
     * http://msdn.microsoft.com/en-us/library/ff469424(v=PROT.10).aspx
     * If Open.GrantedAccess does not contain FILE_WRITE_DATA, the operation
     * MUST be failed with STATUS_ACCESS_DENIED.
     */
    if (InfoClass == FileAllocationInformation ||
            InfoClass == FileEndOfFileInformation) {
        if (!(RxContext->pRelevantSrvOpen->DesiredAccess & FILE_WRITE_DATA)) {
            status = STATUS_ACCESS_DENIED;
            goto out;
        }
    }
    status = check_nfs41_dirquery_args(RxContext);
    if (status) goto out;

    switch (InfoClass) {
    case FileRenameInformation:
    {
        PFILE_RENAME_INFORMATION rinfo =
            (PFILE_RENAME_INFORMATION)RxContext->Info.Buffer;
        UNICODE_STRING dst = { (USHORT)rinfo->FileNameLength,
            (USHORT)rinfo->FileNameLength, rinfo->FileName };
#ifdef DEBUG_FILE_SET
        DbgP("Attempting to rename to '%wZ'\n", &dst);
#endif
        if (isFilenameTooLong(&dst, pVNetRootContext)) {
            status = STATUS_OBJECT_NAME_INVALID;
            goto out;
        }
        if (rinfo->RootDirectory) {
            status = STATUS_INVALID_PARAMETER;
            goto out;
        }
        break;
    }
    case FileLinkInformation:
    {
        PFILE_LINK_INFORMATION linfo =
            (PFILE_LINK_INFORMATION)RxContext->Info.Buffer;
        UNICODE_STRING dst = { (USHORT)linfo->FileNameLength,
            (USHORT)linfo->FileNameLength, linfo->FileName };
#ifdef DEBUG_FILE_SET
        DbgP("Attempting to add link as '%wZ'\n", &dst);
#endif
        if (isFilenameTooLong(&dst, pVNetRootContext)) {
            status = STATUS_OBJECT_NAME_INVALID;
            goto out;
        }
        if (linfo->RootDirectory) {
            status = STATUS_INVALID_PARAMETER;
            goto out;
        }
        break;
    }
    case FileDispositionInformation:
    {
        PFILE_DISPOSITION_INFORMATION dinfo =
            (PFILE_DISPOSITION_INFORMATION)RxContext->Info.Buffer;
        __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
        if (dinfo->DeleteFile && nfs41_fcb->DeletePending) {
            status = STATUS_DELETE_PENDING;
            goto out;
        }
        break;
    }
    case FileBasicInformation:
    case FileAllocationInformation:
    case FileEndOfFileInformation:
        break;
    default:
        print_error("nfs41_SetFileInformation: unhandled class %d\n", InfoClass);
        status = STATUS_NOT_SUPPORTED;
    }

out:
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_SetFileInformation(
#else
NTSTATUS nfs41_SetFileInformation(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    nfs41_updowncall_entry *entry;
    FILE_INFORMATION_CLASS InfoClass = RxContext->Info.FileInformationClass;
    FILE_RENAME_INFORMATION rinfo;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_FILE_SET
    DbgEn();
    print_debug_filedirquery_header(RxContext);
#endif

    status = check_nfs41_setattr_args(RxContext);
    if (status) goto out;

    switch (InfoClass) {
    case FileDispositionInformation:
        {
            PFILE_DISPOSITION_INFORMATION dinfo =
                (PFILE_DISPOSITION_INFORMATION)RxContext->Info.Buffer;
            if (dinfo->DeleteFile) {
                nfs41_fcb->DeletePending = TRUE;
                // we can delete directories right away
                if (nfs41_fcb->StandardInfo.Directory)
                    break;
                nfs41_fcb->StandardInfo.DeletePending = TRUE;
                if (RxContext->pFcb->OpenCount > 1) {
                    rinfo.ReplaceIfExists = 0;
                    rinfo.RootDirectory = INVALID_HANDLE_VALUE;
                    rinfo.FileNameLength = 0;
                    rinfo.FileName[0] = L'\0';
                    InfoClass = FileRenameInformation;
                    nfs41_fcb->Renamed = TRUE;
                    break;
                }
            } else {
                /* section 4.3.3 of [FSBO]
                 * "file system behavior in the microsoft windows environment"
                 */
                if (nfs41_fcb->DeletePending) {
                    nfs41_fcb->DeletePending = 0;
                    nfs41_fcb->StandardInfo.DeletePending = 0;
                }
            }
            status = STATUS_SUCCESS;
            goto out;
        }
    case FileEndOfFileInformation:
        {
            PFILE_END_OF_FILE_INFORMATION info =
                (PFILE_END_OF_FILE_INFORMATION)RxContext->Info.Buffer;
            nfs41_fcb->StandardInfo.AllocationSize =
                nfs41_fcb->StandardInfo.EndOfFile = info->EndOfFile;
            break;
        }
    case FileRenameInformation:
        {
            /* noop if filename and destination are the same */
            PFILE_RENAME_INFORMATION prinfo =
                (PFILE_RENAME_INFORMATION)RxContext->Info.Buffer;
            const UNICODE_STRING dst = { (USHORT)prinfo->FileNameLength,
                (USHORT)prinfo->FileNameLength, prinfo->FileName };
            if (RtlCompareUnicodeString(&dst,
                    SrvOpen->pAlreadyPrefixedName, FALSE) == 0) {
                status = STATUS_SUCCESS;
                goto out;
            }
        }
    }

    status = nfs41_UpcallCreate(NFS41_FILE_SET, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.SetFile.InfoClass = InfoClass;

    /* original irp has infoclass for remove but we need to rename instead,
     * thus we changed the local variable infoclass */
    if (RxContext->Info.FileInformationClass == FileDispositionInformation &&
            InfoClass == FileRenameInformation) {
        entry->buf = &rinfo;
        entry->buf_len = sizeof(rinfo);
    } else {
        entry->buf = RxContext->Info.Buffer;
        entry->buf_len = RxContext->Info.Length;
    }
#ifdef ENABLE_TIMINGS
    InterlockedIncrement(&setattr.sops);
    InterlockedAdd64(&setattr.size, entry->u.SetFile.buf_len);
#endif

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    status = map_setfile_error(entry->status);
    if (!status) {
        if (!nfs41_fobx->deleg_type && entry->ChangeTime &&
                (SrvOpen->DesiredAccess &
                (FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA)))
            nfs41_update_fcb_list(RxContext->pFcb, entry->ChangeTime);
        nfs41_fcb->changeattr = entry->ChangeTime;
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&setattr.tops);
    InterlockedAdd64(&setattr.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_SetFileInformation delta = %d op=%d sum=%d\n",
        t2.QuadPart - t1.QuadPart, setattr.tops, setattr.ticks);
#endif
#endif
#ifdef DEBUG_FILE_SET
    DbgEx();
#endif
    return status;
}

NTSTATUS nfs41_SetFileInformationAtCleanup(
      IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status;
    DbgEn();
    status = nfs41_SetFileInformation(RxContext);
    DbgEx();
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_IsValidDirectory (
#else
NTSTATUS nfs41_IsValidDirectory (
#endif
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING DirectoryName)
{
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_ComputeNewBufferingState(
#else
NTSTATUS nfs41_ComputeNewBufferingState(
#endif
    IN OUT PMRX_SRV_OPEN pSrvOpen,
    IN PVOID pMRxContext,
    OUT ULONG *pNewBufferingState)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG flag = PtrToUlong(pMRxContext), oldFlags = pSrvOpen->BufferingFlags;

    switch(flag) {
    case DISABLE_CACHING:
        if (pSrvOpen->BufferingFlags &
            (FCB_STATE_READBUFFERING_ENABLED | FCB_STATE_READCACHING_ENABLED))
            pSrvOpen->BufferingFlags &=
                ~(FCB_STATE_READBUFFERING_ENABLED |
                  FCB_STATE_READCACHING_ENABLED);
        if (pSrvOpen->BufferingFlags &
            (FCB_STATE_WRITECACHING_ENABLED | FCB_STATE_WRITEBUFFERING_ENABLED))
            pSrvOpen->BufferingFlags &=
                ~(FCB_STATE_WRITECACHING_ENABLED |
                  FCB_STATE_WRITEBUFFERING_ENABLED);
        pSrvOpen->BufferingFlags |= FCB_STATE_DISABLE_LOCAL_BUFFERING;
        break;
    case ENABLE_READ_CACHING:
        pSrvOpen->BufferingFlags |=
            (FCB_STATE_READBUFFERING_ENABLED | FCB_STATE_READCACHING_ENABLED);
        break;
    case ENABLE_WRITE_CACHING:
        pSrvOpen->BufferingFlags |=
            (FCB_STATE_WRITECACHING_ENABLED | FCB_STATE_WRITEBUFFERING_ENABLED);
        break;
    case ENABLE_READWRITE_CACHING:
        pSrvOpen->BufferingFlags =
            (FCB_STATE_READBUFFERING_ENABLED | FCB_STATE_READCACHING_ENABLED |
            FCB_STATE_WRITECACHING_ENABLED | FCB_STATE_WRITEBUFFERING_ENABLED);
    }
#ifdef DEBUG_TIME_BASED_COHERENCY
    DbgP("nfs41_ComputeNewBufferingState: %wZ pSrvOpen %p Old %08x New %08x\n",
         pSrvOpen->pAlreadyPrefixedName, pSrvOpen, oldFlags,
         pSrvOpen->BufferingFlags);
    *pNewBufferingState = pSrvOpen->BufferingFlags;
#endif
    return status;
}

void print_readwrite_args(
    PRX_CONTEXT RxContext)
{
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    print_debug_header(RxContext);
    DbgP("Bytecount 0x%x Byteoffset 0x%x Buffer %p\n",
        LowIoContext->ParamsFor.ReadWrite.ByteCount,
        LowIoContext->ParamsFor.ReadWrite.ByteOffset,
        LowIoContext->ParamsFor.ReadWrite.Buffer);
}

void enable_caching(
    PMRX_SRV_OPEN SrvOpen,
    PNFS41_FOBX nfs41_fobx,
    ULONGLONG ChangeTime,
    HANDLE session)
{
    ULONG flag = 0;
    PLIST_ENTRY pEntry;
    nfs41_fcb_list_entry *cur;
    BOOLEAN found = FALSE;

    if (SrvOpen->DesiredAccess & FILE_READ_DATA)
        flag = ENABLE_READ_CACHING;
    if ((SrvOpen->DesiredAccess & FILE_WRITE_DATA) &&
            !nfs41_fobx->write_thru)
        flag = ENABLE_WRITE_CACHING;
    if ((SrvOpen->DesiredAccess & FILE_READ_DATA) &&
            (SrvOpen->DesiredAccess & FILE_WRITE_DATA) &&
            !nfs41_fobx->write_thru)
        flag = ENABLE_READWRITE_CACHING;

#if defined(DEBUG_TIME_BASED_COHERENCY) || \
        defined(DEBUG_WRITE) || defined(DEBUG_READ)
    print_caching_level(1, flag, SrvOpen->pAlreadyPrefixedName);
#endif

    if (!flag)
        return;

    RxChangeBufferingState((PSRV_OPEN)SrvOpen, ULongToPtr(flag), 1);

    ExAcquireFastMutex(&fcblistLock);
    pEntry = openlist.head.Flink;
    while (!IsListEmpty(&openlist.head)) {
        cur = (nfs41_fcb_list_entry *)CONTAINING_RECORD(pEntry,
                nfs41_fcb_list_entry, next);
        if (cur->fcb == SrvOpen->pFcb) {
#ifdef DEBUG_TIME_BASED_COHERENCY
            DbgP("enable_caching: Looked&Found match for fcb=%p %wZ\n",
                SrvOpen->pFcb, SrvOpen->pAlreadyPrefixedName);
#endif
            cur->skip = FALSE;
            found = TRUE;
            break;
        }
        if (pEntry->Flink == &openlist.head) {
#ifdef DEBUG_TIME_BASED_COHERENCY
            DbgP("enable_caching: reached EOL looking for fcb=%p %wZ\n",
                SrvOpen->pFcb, SrvOpen->pAlreadyPrefixedName);
#endif
            break;
        }
        pEntry = pEntry->Flink;
    }
    if (!found && nfs41_fobx->deleg_type) {
        nfs41_fcb_list_entry *oentry;
#ifdef DEBUG_TIME_BASED_COHERENCY
        DbgP("enable_caching: delegation recalled: srv_open=%p\n", SrvOpen);
#endif
        oentry = RxAllocatePoolWithTag(NonPagedPool,
            sizeof(nfs41_fcb_list_entry), NFS41_MM_POOLTAG_OPEN);
        if (oentry == NULL) return;
        oentry->fcb = SrvOpen->pFcb;
        oentry->session = session;
        oentry->nfs41_fobx = nfs41_fobx;
        oentry->ChangeTime = ChangeTime;
        oentry->skip = FALSE;
        InsertTailList(&openlist.head, &oentry->next);
        nfs41_fobx->deleg_type = 0;
    }
    ExReleaseFastMutex(&fcblistLock);
}

NTSTATUS map_readwrite_errors(
    DWORD status)
{
    switch (status) {
    case ERROR_ACCESS_DENIED:           return STATUS_ACCESS_DENIED;
    case ERROR_HANDLE_EOF:              return STATUS_END_OF_FILE;
    case ERROR_FILE_INVALID:            return STATUS_FILE_INVALID;
    case ERROR_INVALID_PARAMETER:       return STATUS_INVALID_PARAMETER;
    case ERROR_LOCK_VIOLATION:          return STATUS_FILE_LOCK_CONFLICT;
    case ERROR_NETWORK_ACCESS_DENIED:   return STATUS_NETWORK_ACCESS_DENIED;
    case ERROR_NETNAME_DELETED:         return STATUS_NETWORK_NAME_DELETED;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_NET_WRITE_FAULT\n", status);
    case ERROR_NET_WRITE_FAULT:         return STATUS_NET_WRITE_FAULT;
    }
}

NTSTATUS check_nfs41_read_args(
    IN PRX_CONTEXT RxContext)
{
    if (!RxContext->LowIoContext.ParamsFor.ReadWrite.Buffer)
        return STATUS_INVALID_USER_BUFFER;
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Read(
#else
NTSTATUS nfs41_Read(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    nfs41_updowncall_entry *entry;
    BOOLEAN async = FALSE;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
    DWORD io_delay;
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_READ
    DbgEn();
    print_readwrite_args(RxContext);
#endif
    status = check_nfs41_read_args(RxContext);
    if (status) goto out;

    status = nfs41_UpcallCreate(NFS41_READ, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.ReadWrite.MdlAddress = LowIoContext->ParamsFor.ReadWrite.Buffer;
    entry->buf_len = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    entry->u.ReadWrite.offset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    if (FlagOn(RxContext->CurrentIrpSp->FileObject->Flags,
            FO_SYNCHRONOUS_IO) == FALSE) {
        entry->u.ReadWrite.rxcontext = RxContext;
        async = entry->async_op = TRUE;
    }

    /* assume network speed is 100MB/s and disk speed is 100MB/s so add
     * time to transfer requested bytes over the network and read from disk
     */
    io_delay = pVNetRootContext->timeout + 2 * entry->buf_len / 104857600;
    status = nfs41_UpcallWaitForReply(entry, io_delay);
    if (status) goto out;

    if (async) {
#ifdef DEBUG_READ
        DbgP("This is asynchronous read, returning control back to the user\n");
#endif
        status = STATUS_PENDING;
        goto out;
    }

    if (entry->status == NO_ERROR) {
#ifdef ENABLE_TIMINGS
        InterlockedIncrement(&read.sops);
        InterlockedAdd64(&read.size, entry->u.ReadWrite.len);
#endif
        status = RxContext->CurrentIrp->IoStatus.Status = STATUS_SUCCESS;
        RxContext->IoStatusBlock.Information = entry->buf_len;

        if ((!BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,
                LOWIO_READWRITEFLAG_PAGING_IO) &&
                (SrvOpen->DesiredAccess & FILE_READ_DATA) &&
                !pVNetRootContext->nocache && !nfs41_fobx->nocache &&
                !(SrvOpen->BufferingFlags &
                (FCB_STATE_READBUFFERING_ENABLED |
                 FCB_STATE_READCACHING_ENABLED)))) {
            enable_caching(SrvOpen, nfs41_fobx, nfs41_fcb->changeattr,
                pVNetRootContext->session);
        }
    } else {
        status = map_readwrite_errors(entry->status);
        RxContext->CurrentIrp->IoStatus.Status = status;
        RxContext->IoStatusBlock.Information = 0;
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&read.tops);
    InterlockedAdd64(&read.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_Read delta = %d op=%d sum=%d\n", t2.QuadPart - t1.QuadPart,
        read.tops, read.ticks);
#endif
#endif
#ifdef DEBUG_READ
    DbgEx();
#endif
    return status;
}

NTSTATUS check_nfs41_write_args(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(RxContext->pRelevantSrvOpen->pVNetRoot);

    if (!RxContext->LowIoContext.ParamsFor.ReadWrite.Buffer) {
        status = STATUS_INVALID_USER_BUFFER;
        goto out;
    }

    if (pVNetRootContext->read_only) {
        print_error("check_nfs41_write_args: Read-only mount\n");
        status = STATUS_ACCESS_DENIED;
        goto out;
    }
out:
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Write(
#else
NTSTATUS nfs41_Write(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    nfs41_updowncall_entry *entry;
    BOOLEAN async = FALSE;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    __notnull PNFS41_FCB nfs41_fcb = NFS41GetFcbExtension(RxContext->pFcb);
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
    DWORD io_delay;
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifdef DEBUG_WRITE
    DbgEn();
    print_readwrite_args(RxContext);
#endif

    status = check_nfs41_write_args(RxContext);
    if (status) goto out;

    status = nfs41_UpcallCreate(NFS41_WRITE, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.ReadWrite.MdlAddress = LowIoContext->ParamsFor.ReadWrite.Buffer;
    entry->buf_len = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    entry->u.ReadWrite.offset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

    if (FlagOn(RxContext->CurrentIrpSp->FileObject->Flags,
            FO_SYNCHRONOUS_IO) == FALSE) {
        entry->u.ReadWrite.rxcontext = RxContext;
        async = entry->async_op = TRUE;
    }

    /* assume network speed is 100MB/s and disk speed is 100MB/s so add
     * time to transfer requested bytes over the network and write to disk
     */
    io_delay = pVNetRootContext->timeout + 2 * entry->buf_len / 104857600;
    status = nfs41_UpcallWaitForReply(entry, io_delay);
    if (status) goto out;

    if (async) {
#ifdef DEBUG_WRITE
        DbgP("This is asynchronous write, returning control back to the user\n");
#endif
        status = STATUS_PENDING;
        goto out;
    }

    if (entry->status == NO_ERROR) {
        //update cached file attributes
#ifdef ENABLE_TIMINGS
        InterlockedIncrement(&write.sops);
        InterlockedAdd64(&write.size, entry->u.ReadWrite.len);
#endif
        nfs41_fcb->StandardInfo.EndOfFile.QuadPart = entry->buf_len +
            entry->u.ReadWrite.offset;
        status = RxContext->CurrentIrp->IoStatus.Status = STATUS_SUCCESS;
        RxContext->IoStatusBlock.Information = entry->buf_len;
        nfs41_fcb->changeattr = entry->ChangeTime;

        //re-enable write buffering
        if (!BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,
                LOWIO_READWRITEFLAG_PAGING_IO) &&
                (SrvOpen->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) &&
                !pVNetRootContext->write_thru &&
                !pVNetRootContext->nocache &&
                !nfs41_fobx->write_thru && !nfs41_fobx->nocache &&
                !(SrvOpen->BufferingFlags &
                (FCB_STATE_WRITEBUFFERING_ENABLED |
                 FCB_STATE_WRITECACHING_ENABLED))) {
            enable_caching(SrvOpen, nfs41_fobx, nfs41_fcb->changeattr,
                pVNetRootContext->session);
        } else if (!nfs41_fobx->deleg_type)
            nfs41_update_fcb_list(RxContext->pFcb, entry->ChangeTime);

    } else {
        status = map_readwrite_errors(entry->status);
        RxContext->CurrentIrp->IoStatus.Status = status;
        RxContext->IoStatusBlock.Information = 0;
    }
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&write.tops);
    InterlockedAdd64(&write.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_Write delta = %d op=%d sum=%d\n", t2.QuadPart - t1.QuadPart,
        write.tops, write.ticks);
#endif
#endif
#ifdef DEBUG_WRITE
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_IsLockRealizable(
#else
NTSTATUS nfs41_IsLockRealizable(
#endif
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
#ifdef DEBUG_LOCK
    DbgEn();
    DbgP("offset 0x%llx, length 0x%llx, exclusive=%u, blocking=%u\n",
        ByteOffset->QuadPart,Length->QuadPart,
        BooleanFlagOn(LowIoLockFlags, SL_EXCLUSIVE_LOCK),
        !BooleanFlagOn(LowIoLockFlags, SL_FAIL_IMMEDIATELY));
#endif

    /* NFS lock operations with length=0 MUST fail with NFS4ERR_INVAL */
    if (Length->QuadPart == 0)
        status = STATUS_NOT_SUPPORTED;

#ifdef DEBUG_LOCK
    DbgEx();
#endif
    return status;
}

NTSTATUS map_lock_errors(
    DWORD status)
{
    switch (status) {
    case NO_ERROR:                  return STATUS_SUCCESS;
    case ERROR_NETNAME_DELETED:     return STATUS_NETWORK_NAME_DELETED;
    case ERROR_LOCK_FAILED:         return STATUS_LOCK_NOT_GRANTED;
    case ERROR_NOT_LOCKED:          return STATUS_RANGE_NOT_LOCKED;
    case ERROR_ATOMIC_LOCKS_NOT_SUPPORTED: return STATUS_UNSUCCESSFUL;
    case ERROR_OUTOFMEMORY:         return STATUS_INSUFFICIENT_RESOURCES;
    case ERROR_SHARING_VIOLATION:   return STATUS_SHARING_VIOLATION;
    case ERROR_FILE_INVALID:        return STATUS_FILE_INVALID;
    /* if we return ERROR_INVALID_PARAMETER, Windows translates that to
     * success!! */
    case ERROR_INVALID_PARAMETER:   return STATUS_LOCK_NOT_GRANTED;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_NETWORK_RESPONSE\n", status);
    case ERROR_BAD_NET_RESP:        return STATUS_INVALID_NETWORK_RESPONSE;
    }
}

void print_lock_args(
    PRX_CONTEXT RxContext)
{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    const ULONG flags = LowIoContext->ParamsFor.Locks.Flags;
    print_debug_header(RxContext);
    DbgP("offset 0x%llx, length 0x%llx, exclusive=%u, blocking=%u\n",
        LowIoContext->ParamsFor.Locks.ByteOffset,
        LowIoContext->ParamsFor.Locks.Length,
        BooleanFlagOn(flags, SL_EXCLUSIVE_LOCK),
        !BooleanFlagOn(flags, SL_FAIL_IMMEDIATELY));
}


/* use exponential backoff between polls for blocking locks */
#define MSEC_TO_RELATIVE_WAIT   (-10000)
#define MIN_LOCK_POLL_WAIT      (500 * MSEC_TO_RELATIVE_WAIT) /* 500ms */
#define MAX_LOCK_POLL_WAIT      (30000 * MSEC_TO_RELATIVE_WAIT) /* 30s */

void denied_lock_backoff(
    IN OUT PLARGE_INTEGER delay)
{
    if (delay->QuadPart == 0)
        delay->QuadPart = MIN_LOCK_POLL_WAIT;
    else
        delay->QuadPart <<= 1;

    if (delay->QuadPart < MAX_LOCK_POLL_WAIT)
        delay->QuadPart = MAX_LOCK_POLL_WAIT;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Lock(
#else
NTSTATUS nfs41_Lock(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    nfs41_updowncall_entry *entry;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    const ULONG flags = LowIoContext->ParamsFor.Locks.Flags;
#ifdef _MSC_VER
    LARGE_INTEGER poll_delay = {0};
#else
    LARGE_INTEGER poll_delay;
#endif
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif

#ifndef _MSC_VER
    poll_delay.QuadPart = 0;
#endif

#ifdef DEBUG_LOCK
    DbgEn();
    print_lock_args(RxContext);
#endif

/*  RxReleaseFcbResourceForThreadInMRx(RxContext, RxContext->pFcb,
        LowIoContext->ResourceThreadId); */

    status = nfs41_UpcallCreate(NFS41_LOCK, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Lock.offset = LowIoContext->ParamsFor.Locks.ByteOffset;
    entry->u.Lock.length = LowIoContext->ParamsFor.Locks.Length;
    entry->u.Lock.exclusive = BooleanFlagOn(flags, SL_EXCLUSIVE_LOCK);
    entry->u.Lock.blocking = !BooleanFlagOn(flags, SL_FAIL_IMMEDIATELY);

retry_upcall:
    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    /* blocking locks keep trying until it succeeds */
    if (entry->status == ERROR_LOCK_FAILED && entry->u.Lock.blocking) {
        denied_lock_backoff(&poll_delay);
        DbgP("returned ERROR_LOCK_FAILED; retrying in %llums\n",
            poll_delay.QuadPart / MSEC_TO_RELATIVE_WAIT);
        KeDelayExecutionThread(KernelMode, FALSE, &poll_delay);
        entry->state = NFS41_WAITING_FOR_UPCALL;
        goto retry_upcall;
    }

    status = map_lock_errors(entry->status);
    RxContext->CurrentIrp->IoStatus.Status = status;

    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&lock.tops);
    InterlockedAdd64(&lock.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_Lock delta = %d op=%d sum=%d\n", t2.QuadPart - t1.QuadPart,
        lock.tops, lock.ticks);
#endif
#endif
#ifdef DEBUG_LOCK
    DbgEx();
#endif
    return status;
}

void print_unlock_args(
    PRX_CONTEXT RxContext)
{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    print_debug_header(RxContext);
    if (LowIoContext->Operation == LOWIO_OP_UNLOCK_MULTIPLE) {
        PLOWIO_LOCK_LIST lock = LowIoContext->ParamsFor.Locks.LockList;
        DbgP("LOWIO_OP_UNLOCK_MULTIPLE:");
        while (lock) {
            DbgP(" (offset=%llu, length=%llu)", lock->ByteOffset, lock->Length);
            lock = lock->Next;
        }
        DbgP("\n");
    } else {
        DbgP("LOWIO_OP_UNLOCK: offset=%llu, length=%llu\n",
            LowIoContext->ParamsFor.Locks.ByteOffset,
            LowIoContext->ParamsFor.Locks.Length);
    }
}

__inline ULONG unlock_list_count(
    PLOWIO_LOCK_LIST lock)
{
    ULONG count = 0;
    while (lock) {
        count++;
        lock = lock->Next;
    }
    return count;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Unlock(
#else
NTSTATUS nfs41_Unlock(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    nfs41_updowncall_entry *entry;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    __notnull PNFS41_FOBX nfs41_fobx = NFS41GetFobxExtension(RxContext->pFobx);
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION pVNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
#ifdef ENABLE_TIMINGS
    LARGE_INTEGER t1, t2;
    t1 = KeQueryPerformanceCounter(NULL);
#endif
#ifdef DEBUG_LOCK
    DbgEn();
    print_lock_args(RxContext);
#endif

/*  RxReleaseFcbResourceForThreadInMRx(RxContext, RxContext->pFcb,
        LowIoContext->ResourceThreadId); */

    status = nfs41_UpcallCreate(NFS41_UNLOCK, &nfs41_fobx->sec_ctx,
        pVNetRootContext->session, nfs41_fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    if (LowIoContext->Operation == LOWIO_OP_UNLOCK_MULTIPLE) {
        entry->u.Unlock.count = unlock_list_count(
            LowIoContext->ParamsFor.Locks.LockList);
        RtlCopyMemory(&entry->u.Unlock.locks,
            LowIoContext->ParamsFor.Locks.LockList,
            sizeof(LOWIO_LOCK_LIST));
    } else {
        entry->u.Unlock.count = 1;
        entry->u.Unlock.locks.ByteOffset =
            LowIoContext->ParamsFor.Locks.ByteOffset;
        entry->u.Unlock.locks.Length =
            LowIoContext->ParamsFor.Locks.Length;
    }

    status = nfs41_UpcallWaitForReply(entry, pVNetRootContext->timeout);
    if (status) goto out;

    status = map_lock_errors(entry->status);
    RxContext->CurrentIrp->IoStatus.Status = status;
    RxFreePool(entry);
out:
#ifdef ENABLE_TIMINGS
    t2 = KeQueryPerformanceCounter(NULL);
    InterlockedIncrement(&unlock.tops);
    InterlockedAdd64(&unlock.ticks, t2.QuadPart - t1.QuadPart);
#ifdef ENABLE_INDV_TIMINGS
    DbgP("nfs41_Unlock delta = %d op=%d sum=%d\n", t2.QuadPart - t1.QuadPart,
        unlock.tops, unlock.ticks);
#endif
#endif
#ifdef DEBUG_LOCK
    DbgEx();
#endif
    return status;
}

NTSTATUS map_symlink_errors(
    NTSTATUS status)
{
    switch (status) {
    case NO_ERROR:                  return STATUS_SUCCESS;
    case ERROR_INVALID_REPARSE_DATA: return STATUS_IO_REPARSE_DATA_INVALID;
    case ERROR_NOT_A_REPARSE_POINT: return STATUS_NOT_A_REPARSE_POINT;
    case ERROR_ACCESS_DENIED:       return STATUS_ACCESS_DENIED;
    case ERROR_NOT_EMPTY:           return STATUS_DIRECTORY_NOT_EMPTY;
    case ERROR_OUTOFMEMORY:         return STATUS_INSUFFICIENT_RESOURCES;
    case ERROR_INSUFFICIENT_BUFFER: return STATUS_BUFFER_TOO_SMALL;
    case STATUS_BUFFER_TOO_SMALL:
    case ERROR_BUFFER_OVERFLOW:     return STATUS_BUFFER_OVERFLOW;
    default:
        print_error("failed to map windows error %d to NTSTATUS; "
            "defaulting to STATUS_INVALID_NETWORK_RESPONSE\n", status);
    case ERROR_BAD_NET_RESP:        return STATUS_INVALID_NETWORK_RESPONSE;
    }
}

void print_reparse_buffer(
    PREPARSE_DATA_BUFFER Reparse)
{
    UNICODE_STRING name;
    DbgP("ReparseTag:           %08X\n", Reparse->ReparseTag);
    DbgP("ReparseDataLength:    %8u\n", Reparse->ReparseDataLength);
    DbgP("Reserved:             %8u\n", Reparse->Reserved);
    DbgP("SubstituteNameOffset: %8u\n",
         Reparse->SymbolicLinkReparseBuffer.SubstituteNameOffset);
    DbgP("SubstituteNameLength: %8u\n",
         Reparse->SymbolicLinkReparseBuffer.SubstituteNameLength);
    DbgP("PrintNameOffset:      %8u\n",
         Reparse->SymbolicLinkReparseBuffer.PrintNameOffset);
    DbgP("PrintNameLength:      %8u\n",
         Reparse->SymbolicLinkReparseBuffer.PrintNameLength);
    DbgP("Flags:                %08X\n",
         Reparse->SymbolicLinkReparseBuffer.Flags);

    name.Buffer = &Reparse->SymbolicLinkReparseBuffer.PathBuffer[
        Reparse->SymbolicLinkReparseBuffer.SubstituteNameOffset/sizeof(WCHAR)];
    name.MaximumLength = name.Length =
        Reparse->SymbolicLinkReparseBuffer.SubstituteNameLength;
    DbgP("SubstituteName:       %wZ\n", &name);

    name.Buffer = &Reparse->SymbolicLinkReparseBuffer.PathBuffer[
        Reparse->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR)];
    name.MaximumLength = name.Length =
        Reparse->SymbolicLinkReparseBuffer.PrintNameLength;
    DbgP("PrintName:            %wZ\n", &name);
}

NTSTATUS check_nfs41_setreparse_args(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    __notnull XXCTL_LOWIO_COMPONENT *FsCtl = &RxContext->LowIoContext.ParamsFor.FsCtl;
    __notnull PREPARSE_DATA_BUFFER Reparse = (PREPARSE_DATA_BUFFER)FsCtl->pInputBuffer;
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION VNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    const ULONG HeaderLen = REPARSE_DATA_BUFFER_HEADER_SIZE;

    /* access checks */
    if (VNetRootContext->read_only) {
        status = STATUS_MEDIA_WRITE_PROTECTED;
        goto out;
    }
    if (!(SrvOpen->DesiredAccess & (FILE_WRITE_DATA|FILE_WRITE_ATTRIBUTES))) {
        status = STATUS_ACCESS_DENIED;
        goto out;
    }

    /* must have a filename longer than vnetroot name,
     * or it's trying to operate on the volume itself */
    if (is_root_directory(RxContext)) {
        status = STATUS_INVALID_PARAMETER;
        goto out;
    }
    if (FsCtl->pOutputBuffer != NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto out;
    }

    /* validate input buffer and length */
    if (!Reparse) {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto out;
    }

    if (FsCtl->InputBufferLength < HeaderLen ||
            FsCtl->InputBufferLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
        status = STATUS_IO_REPARSE_DATA_INVALID;
        goto out;
    }
    if (FsCtl->InputBufferLength != HeaderLen + Reparse->ReparseDataLength) {
        status = STATUS_IO_REPARSE_DATA_INVALID;
        goto out;
    }

    /* validate reparse tag */
    if (!IsReparseTagValid(Reparse->ReparseTag)) {
        status = STATUS_IO_REPARSE_TAG_INVALID;
        goto out;
    }
    if (Reparse->ReparseTag != IO_REPARSE_TAG_SYMLINK) {
        status = STATUS_IO_REPARSE_TAG_MISMATCH;
        goto out;
    }
out:
    return status;
}

NTSTATUS nfs41_SetReparsePoint(
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status;
    UNICODE_STRING TargetName;
    __notnull XXCTL_LOWIO_COMPONENT *FsCtl = &RxContext->LowIoContext.ParamsFor.FsCtl;
    __notnull PREPARSE_DATA_BUFFER Reparse = (PREPARSE_DATA_BUFFER)FsCtl->pInputBuffer;
    __notnull PNFS41_FOBX Fobx = NFS41GetFobxExtension(RxContext->pFobx);
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION VNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    nfs41_updowncall_entry *entry;

#ifdef DEBUG_SYMLINK
    DbgEn();
    print_debug_header(RxContext);
    print_reparse_buffer(Reparse);
#endif
    status = check_nfs41_setreparse_args(RxContext);
    if (status) goto out;

    TargetName.MaximumLength = TargetName.Length =
        Reparse->SymbolicLinkReparseBuffer.PrintNameLength;
    TargetName.Buffer = &Reparse->SymbolicLinkReparseBuffer.PathBuffer[
        Reparse->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR)];

    status = nfs41_UpcallCreate(NFS41_SYMLINK, &Fobx->sec_ctx,
        VNetRootContext->session, Fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Symlink.target = &TargetName;
    entry->u.Symlink.set = TRUE;

    status = nfs41_UpcallWaitForReply(entry, VNetRootContext->timeout);
    if (status) goto out;

    status = map_symlink_errors(entry->status);
    RxFreePool(entry);
out:
#ifdef DEBUG_SYMLINK
    DbgEx();
#endif
    return status;
}

NTSTATUS check_nfs41_getreparse_args(
    PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    XXCTL_LOWIO_COMPONENT *FsCtl = &RxContext->LowIoContext.ParamsFor.FsCtl;
    const USHORT HeaderLen = FIELD_OFFSET(REPARSE_DATA_BUFFER,
        SymbolicLinkReparseBuffer.PathBuffer);

    /* must have a filename longer than vnetroot name,
     * or it's trying to operate on the volume itself */
    if (is_root_directory(RxContext)) {
        status = STATUS_INVALID_PARAMETER;
        goto out;
    }
    /* ifs reparse tests expect STATUS_INVALID_PARAMETER,
     * but 'dir' passes a buffer here when querying symlinks
    if (FsCtl->pInputBuffer != NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto out;
    } */
    if (!FsCtl->pOutputBuffer) {
        status = STATUS_INVALID_USER_BUFFER;
        goto out;
    }
    if (!BooleanFlagOn(RxContext->pFcb->Attributes,
            FILE_ATTRIBUTE_REPARSE_POINT)) {
        status = STATUS_NOT_A_REPARSE_POINT;
        DbgP("FILE_ATTRIBUTE_REPARSE_POINT is not set!\n");
        goto out;
    }

    if (FsCtl->OutputBufferLength < HeaderLen) {
        RxContext->InformationToReturn = HeaderLen;
        status = STATUS_BUFFER_TOO_SMALL;
        goto out;
    }
out:
    return status;
}

NTSTATUS nfs41_GetReparsePoint(
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status;
    UNICODE_STRING TargetName;
    XXCTL_LOWIO_COMPONENT *FsCtl = &RxContext->LowIoContext.ParamsFor.FsCtl;
    __notnull PNFS41_FOBX Fobx = NFS41GetFobxExtension(RxContext->pFobx);
    __notnull PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    __notnull PNFS41_V_NET_ROOT_EXTENSION VNetRootContext =
        NFS41GetVNetRootExtension(SrvOpen->pVNetRoot);
    __notnull PNFS41_NETROOT_EXTENSION pNetRootContext =
        NFS41GetNetRootExtension(SrvOpen->pVNetRoot->pNetRoot);
    nfs41_updowncall_entry *entry;
    const USHORT HeaderLen = FIELD_OFFSET(REPARSE_DATA_BUFFER,
        SymbolicLinkReparseBuffer.PathBuffer);

#ifdef DEBUG_SYMLINK
    DbgEn();
#endif
    status = check_nfs41_getreparse_args(RxContext);
    if (status) goto out;

    TargetName.Buffer = (PWCH)((PBYTE)FsCtl->pOutputBuffer + HeaderLen);
    TargetName.MaximumLength = (USHORT)min(FsCtl->OutputBufferLength -
        HeaderLen, 0xFFFF);

    status = nfs41_UpcallCreate(NFS41_SYMLINK, &Fobx->sec_ctx,
        VNetRootContext->session, Fobx->nfs41_open_state,
        pNetRootContext->nfs41d_version, SrvOpen->pAlreadyPrefixedName, &entry);
    if (status) goto out;

    entry->u.Symlink.target = &TargetName;
    entry->u.Symlink.set = FALSE;

    status = nfs41_UpcallWaitForReply(entry, VNetRootContext->timeout);
    if (status) goto out;

    status = map_symlink_errors(entry->status);
    if (status == STATUS_SUCCESS) {
        /* fill in the output buffer */
        PREPARSE_DATA_BUFFER Reparse = (PREPARSE_DATA_BUFFER)
            FsCtl->pOutputBuffer;
        Reparse->ReparseTag = IO_REPARSE_TAG_SYMLINK;
        Reparse->ReparseDataLength = HeaderLen + TargetName.Length -
            REPARSE_DATA_BUFFER_HEADER_SIZE;
        Reparse->Reserved = 0;
        Reparse->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;
        /* PrintName and SubstituteName point to the same string */
        Reparse->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
        Reparse->SymbolicLinkReparseBuffer.SubstituteNameLength =
            TargetName.Length;
        Reparse->SymbolicLinkReparseBuffer.PrintNameOffset = 0;
        Reparse->SymbolicLinkReparseBuffer.PrintNameLength = TargetName.Length;
        print_reparse_buffer(Reparse);

        RxContext->IoStatusBlock.Information = HeaderLen + TargetName.Length;
    } else if (status == STATUS_BUFFER_TOO_SMALL) {
        RxContext->InformationToReturn = HeaderLen + TargetName.Length;
    }
    RxFreePool(entry);
out:
#ifdef DEBUG_SYMLINK
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_FsCtl(
#else
NTSTATUS nfs41_FsCtl(
#endif
    IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
#ifdef DEBUG_MISC
    DbgEn();
    print_debug_header(RxContext);
#endif
    switch (RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode) {
    case FSCTL_SET_REPARSE_POINT:
        status = nfs41_SetReparsePoint(RxContext);
        break;

    case FSCTL_GET_REPARSE_POINT:
        status = nfs41_GetReparsePoint(RxContext);
        break;
#ifdef DEBUG_MISC
    default:
        DbgP("FsControlCode: %d\n",
             RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode);
#endif
    }
#ifdef DEBUG_MISC
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_CompleteBufferingStateChangeRequest(
#else
NTSTATUS nfs41_CompleteBufferingStateChangeRequest(
#endif
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN SrvOpen,
    IN PVOID pContext)
{
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_FsdDispatch (
#else
NTSTATUS nfs41_FsdDispatch (
#endif
    IN PDEVICE_OBJECT dev,
    IN PIRP Irp)
{
#ifdef DEBUG_FSDDISPATCH
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
#endif
    NTSTATUS status;

#ifdef DEBUG_FSDDISPATCH
    DbgEn();
    DbgP("CURRENT IRP = %d.%d\n", IrpSp->MajorFunction, IrpSp->MinorFunction);
    if(IrpSp->FileObject)
        DbgP("FileOject %p Filename %wZ\n", IrpSp->FileObject,
                &IrpSp->FileObject->FileName);
#endif

    if (dev != (PDEVICE_OBJECT)nfs41_dev) {
        print_error("*** not ours ***\n");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto out;
    }

    status = RxFsdDispatch((PRDBSS_DEVICE_OBJECT)dev,Irp);
    /* AGLO: 08/05/2009 - looks like RxFsdDispatch frees IrpSp */

out:
#ifdef DEBUG_FSDDISPATCH
    DbgP("IoStatus status = 0x%x info = 0x%x\n", Irp->IoStatus.Status,
         Irp->IoStatus.Information);
    DbgEx();
#endif
    return status;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_Unimplemented(
#else
NTSTATUS nfs41_Unimplemented(
#endif
    PRX_CONTEXT RxContext)
{
    return STATUS_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
NTSTATUS NTAPI nfs41_AreFilesAliased(
#else
NTSTATUS nfs41_AreFilesAliased(
#endif
    PFCB a,
    PFCB b)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS nfs41_init_ops()
{
    DbgEn();

    ZeroAndInitializeNodeType(&nfs41_ops, RDBSS_NTC_MINIRDR_DISPATCH,
        sizeof(MINIRDR_DISPATCH));

    nfs41_ops.MRxFlags = (RDBSS_MANAGE_NET_ROOT_EXTENSION |
                            RDBSS_MANAGE_V_NET_ROOT_EXTENSION |
                            RDBSS_MANAGE_FCB_EXTENSION |
                            RDBSS_MANAGE_FOBX_EXTENSION);

    nfs41_ops.MRxSrvCallSize  = 0; // srvcall extension is not handled in rdbss
    nfs41_ops.MRxNetRootSize  = sizeof(NFS41_NETROOT_EXTENSION);
    nfs41_ops.MRxVNetRootSize = sizeof(NFS41_V_NET_ROOT_EXTENSION);
    nfs41_ops.MRxFcbSize      = sizeof(NFS41_FCB);
    nfs41_ops.MRxFobxSize     = sizeof(NFS41_FOBX);

    // Mini redirector cancel routine ..

    nfs41_ops.MRxCancel = NULL;

    //
    // Mini redirector Start/Stop. Each mini-rdr can be started or stopped
    // while the others continue to operate.
    //

    nfs41_ops.MRxStart                = nfs41_Start;
    nfs41_ops.MRxStop                 = nfs41_Stop;
    nfs41_ops.MRxDevFcbXXXControlFile = nfs41_DevFcbXXXControlFile;

    //
    // Mini redirector name resolution.
    //

    nfs41_ops.MRxCreateSrvCall       = nfs41_CreateSrvCall;
    nfs41_ops.MRxSrvCallWinnerNotify = nfs41_SrvCallWinnerNotify;
    nfs41_ops.MRxCreateVNetRoot      = nfs41_CreateVNetRoot;
    nfs41_ops.MRxExtractNetRootName  = nfs41_ExtractNetRootName;
    nfs41_ops.MRxFinalizeSrvCall     = nfs41_FinalizeSrvCall;
    nfs41_ops.MRxFinalizeNetRoot     = nfs41_FinalizeNetRoot;
    nfs41_ops.MRxFinalizeVNetRoot    = nfs41_FinalizeVNetRoot;

    //
    // File System Object Creation/Deletion.
    //

    nfs41_ops.MRxCreate            = nfs41_Create;
    nfs41_ops.MRxCollapseOpen      = nfs41_CollapseOpen;
    nfs41_ops.MRxShouldTryToCollapseThisOpen = nfs41_ShouldTryToCollapseThisOpen;
    nfs41_ops.MRxExtendForCache    = nfs41_ExtendForCache;
    nfs41_ops.MRxExtendForNonCache = nfs41_ExtendForCache;
    nfs41_ops.MRxCloseSrvOpen      = nfs41_CloseSrvOpen;
    nfs41_ops.MRxFlush             = nfs41_Flush;
    nfs41_ops.MRxDeallocateForFcb  = nfs41_DeallocateForFcb;
    nfs41_ops.MRxDeallocateForFobx = nfs41_DeallocateForFobx;
    nfs41_ops.MRxIsLockRealizable    = nfs41_IsLockRealizable;

    //
    // File System Objects query/Set
    //

    nfs41_ops.MRxQueryDirectory       = nfs41_QueryDirectory;
    nfs41_ops.MRxQueryVolumeInfo      = nfs41_QueryVolumeInformation;
    nfs41_ops.MRxQueryEaInfo          = nfs41_QueryEaInformation;
    nfs41_ops.MRxSetEaInfo            = nfs41_SetEaInformation;
    nfs41_ops.MRxQuerySdInfo          = nfs41_QuerySecurityInformation;
    nfs41_ops.MRxSetSdInfo            = nfs41_SetSecurityInformation;
    nfs41_ops.MRxQueryFileInfo        = nfs41_QueryFileInformation;
    nfs41_ops.MRxSetFileInfo          = nfs41_SetFileInformation;

    //
    // Buffering state change
    //

    nfs41_ops.MRxComputeNewBufferingState = nfs41_ComputeNewBufferingState;

    //
    // File System Object I/O
    //

    nfs41_ops.MRxLowIOSubmit[LOWIO_OP_READ]            = nfs41_Read;
    nfs41_ops.MRxLowIOSubmit[LOWIO_OP_WRITE]           = nfs41_Write;
    nfs41_ops.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = nfs41_Lock;
    nfs41_ops.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = nfs41_Lock;
    nfs41_ops.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = nfs41_Unlock;
    nfs41_ops.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = nfs41_Unlock;
    nfs41_ops.MRxLowIOSubmit[LOWIO_OP_FSCTL]           = nfs41_FsCtl;

    //
    // Miscellanous
    //

    nfs41_ops.MRxCompleteBufferingStateChangeRequest =
        nfs41_CompleteBufferingStateChangeRequest;
    nfs41_ops.MRxIsValidDirectory     = nfs41_IsValidDirectory;

    nfs41_ops.MRxTruncate = nfs41_Unimplemented;
    nfs41_ops.MRxZeroExtend = nfs41_Unimplemented;
    nfs41_ops.MRxAreFilesAliased = nfs41_AreFilesAliased;
    nfs41_ops.MRxQueryQuotaInfo = nfs41_Unimplemented;
    nfs41_ops.MRxSetQuotaInfo = nfs41_Unimplemented;
    nfs41_ops.MRxSetVolumeInfo = nfs41_Unimplemented;

    DbgR();
    return(STATUS_SUCCESS);
}

KSTART_ROUTINE fcbopen_main;
#ifdef __REACTOS__
VOID NTAPI fcbopen_main(PVOID ctx)
#else
VOID fcbopen_main(PVOID ctx)
#endif
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    DbgEn();
    timeout.QuadPart = RELATIVE(SECONDS(30));
    while(1) {
        PLIST_ENTRY pEntry;
        nfs41_fcb_list_entry *cur;
        status = KeDelayExecutionThread(KernelMode, TRUE, &timeout);
        ExAcquireFastMutex(&fcblistLock);
        pEntry = openlist.head.Flink;
        while (!IsListEmpty(&openlist.head)) {
            PNFS41_NETROOT_EXTENSION pNetRootContext;
            nfs41_updowncall_entry *entry;
            FILE_BASIC_INFORMATION binfo;
            PNFS41_FCB nfs41_fcb;
            cur = (nfs41_fcb_list_entry *)CONTAINING_RECORD(pEntry,
                    nfs41_fcb_list_entry, next);

#ifdef DEBUG_TIME_BASED_COHERENCY
            DbgP("fcbopen_main: Checking attributes for fcb=%p "
                "change_time=%llu skipping=%d\n", cur->fcb,
                cur->ChangeTime, cur->skip);
#endif
            if (cur->skip) goto out;
            pNetRootContext =
                NFS41GetNetRootExtension(cur->fcb->pNetRoot);
            /* place an upcall for this srv_open */
            status = nfs41_UpcallCreate(NFS41_FILE_QUERY,
                &cur->nfs41_fobx->sec_ctx, cur->session,
                cur->nfs41_fobx->nfs41_open_state,
                pNetRootContext->nfs41d_version, NULL, &entry);
            if (status) goto out;

            entry->u.QueryFile.InfoClass = FileBasicInformation;
            entry->buf = &binfo;
            entry->buf_len = sizeof(binfo);

            status = nfs41_UpcallWaitForReply(entry, UPCALL_TIMEOUT_DEFAULT);
            if (status) goto out;

            if (cur->ChangeTime != entry->ChangeTime) {
                ULONG flag = DISABLE_CACHING;
                PMRX_SRV_OPEN srv_open;
                PLIST_ENTRY psrvEntry;
#ifdef DEBUG_TIME_BASED_COHERENCY
                DbgP("fcbopen_main: old ctime=%llu new_ctime=%llu\n",
                    cur->ChangeTime, entry->ChangeTime);
#endif
                cur->ChangeTime = entry->ChangeTime;
                cur->skip = TRUE;
                psrvEntry = &cur->fcb->SrvOpenList;
                psrvEntry = psrvEntry->Flink;
                while (!IsListEmpty(&cur->fcb->SrvOpenList)) {
                    srv_open = (PMRX_SRV_OPEN)CONTAINING_RECORD(psrvEntry,
                            MRX_SRV_OPEN, SrvOpenQLinks);
                    if (srv_open->DesiredAccess &
                            (FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA)) {
#ifdef DEBUG_TIME_BASED_COHERENCY
                        DbgP("fcbopen_main: ************ Invalidate the cache %wZ"
                             "************\n", srv_open->pAlreadyPrefixedName);
#endif
                        RxIndicateChangeOfBufferingStateForSrvOpen(
                            cur->fcb->pNetRoot->pSrvCall, srv_open,
                            srv_open->Key, ULongToPtr(flag));
                    }
                    if (psrvEntry->Flink == &cur->fcb->SrvOpenList) {
#ifdef DEBUG_TIME_BASED_COHERENCY
                        DbgP("fcbopen_main: reached end of srvopen for fcb %p\n",
                            cur->fcb);
#endif
                        break;
                    }
                    psrvEntry = psrvEntry->Flink;
                };
            }
            nfs41_fcb = (PNFS41_FCB)cur->fcb->Context;
            nfs41_fcb->changeattr = entry->ChangeTime;
            RxFreePool(entry);
out:
            if (pEntry->Flink == &openlist.head) {
#ifdef DEBUG_TIME_BASED_COHERENCY
                DbgP("fcbopen_main: reached end of the fcb list\n");
#endif
                break;
            }
            pEntry = pEntry->Flink;
        }
        ExReleaseFastMutex(&fcblistLock);
    }
    DbgEx();
}

#ifdef __REACTOS__
NTSTATUS NTAPI DriverEntry(
#else
NTSTATUS DriverEntry(
#endif
    IN PDRIVER_OBJECT drv,
    IN PUNICODE_STRING path)
{
    NTSTATUS status;
    ULONG flags = 0, i;
    UNICODE_STRING dev_name, user_dev_name;
    PNFS41_DEVICE_EXTENSION dev_exts;
    TIME_FIELDS jan_1_1970 = {1970, 1, 1, 0, 0, 0, 0, 0};
    ACCESS_MASK mask = 0;
    OBJECT_ATTRIBUTES oattrs;

    DbgEn();

    status = RxDriverEntry(drv, path);
    if (status != STATUS_SUCCESS) {
        print_error("RxDriverEntry failed: %08lx\n", status);
        goto out;
    }

    RtlInitUnicodeString(&dev_name, NFS41_DEVICE_NAME);
    SetFlag(flags, RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS);

    status = nfs41_init_ops();
    if (status != STATUS_SUCCESS) {
        print_error("nfs41_init_ops failed to initialize dispatch table\n");
        goto out;
    }

    DbgP("calling RxRegisterMinirdr\n");
    status = RxRegisterMinirdr(&nfs41_dev, drv, &nfs41_ops, flags, &dev_name,
                sizeof(NFS41_DEVICE_EXTENSION),
                FILE_DEVICE_NETWORK_FILE_SYSTEM, FILE_REMOTE_DEVICE);
    if (status != STATUS_SUCCESS) {
        print_error("RxRegisterMinirdr failed: %08lx\n", status);
        goto out;
    }
#ifndef __REACTOS__
    nfs41_dev->Flags |= DO_BUFFERED_IO;
#endif

    dev_exts = (PNFS41_DEVICE_EXTENSION)
        ((PBYTE)(nfs41_dev) + sizeof(RDBSS_DEVICE_OBJECT));

    RxDefineNode(dev_exts, NFS41_DEVICE_EXTENSION);
    dev_exts->DeviceObject = nfs41_dev;
    nfs41_create_volume_info((PFILE_FS_VOLUME_INFORMATION)dev_exts->VolAttrs,
        &dev_exts->VolAttrsLen);

    RtlInitUnicodeString(&user_dev_name, NFS41_SHADOW_DEVICE_NAME);
    DbgP("calling IoCreateSymbolicLink %wZ %wZ\n", &user_dev_name, &dev_name);
    status = IoCreateSymbolicLink(&user_dev_name, &dev_name);
    if (status != STATUS_SUCCESS) {
        print_error("Device name IoCreateSymbolicLink failed: %08lx\n", status);
        goto out_unregister;
    }

    KeInitializeEvent(&upcallEvent, SynchronizationEvent, FALSE );
    ExInitializeFastMutex(&upcallLock);
    ExInitializeFastMutex(&downcallLock);
    ExInitializeFastMutex(&xidLock);
    ExInitializeFastMutex(&openOwnerLock);
    ExInitializeFastMutex(&fcblistLock);
    InitializeListHead(&upcall.head);
    InitializeListHead(&downcall.head);
    InitializeListHead(&openlist.head);
    InitializeObjectAttributes(&oattrs, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    status = PsCreateSystemThread(&dev_exts->openlistHandle, mask,
        &oattrs, NULL, NULL, &fcbopen_main, NULL);
    if (status != STATUS_SUCCESS)
        goto out_unregister;

    drv->DriverUnload = nfs41_driver_unload;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        drv->MajorFunction[i] = (PDRIVER_DISPATCH)nfs41_FsdDispatch;

    RtlTimeFieldsToTime(&jan_1_1970, &unix_time_diff);

out_unregister:
    if (status != STATUS_SUCCESS)
        RxUnregisterMinirdr(nfs41_dev);
out:
    DbgEx();
    return status;
}

#ifdef __REACTOS__
VOID NTAPI nfs41_driver_unload(IN PDRIVER_OBJECT drv)
#else
VOID nfs41_driver_unload(IN PDRIVER_OBJECT drv)
#endif
{
    PRX_CONTEXT RxContext;
    NTSTATUS    status;
    UNICODE_STRING dev_name, pipe_name;

    DbgEn();

    RxContext = RxCreateRxContext(NULL, nfs41_dev, RX_CONTEXT_FLAG_IN_FSP);
    if (RxContext == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto unload;
    }
    status = RxStopMinirdr(RxContext, &RxContext->PostRequest);
    RxDereferenceAndDeleteRxContext(RxContext);

unload:
    RtlInitUnicodeString(&dev_name, NFS41_SHADOW_DEVICE_NAME);
    status = IoDeleteSymbolicLink(&dev_name);
    if (status != STATUS_SUCCESS) {
        print_error("couldn't delete device symbolic link\n");
    }
    RtlInitUnicodeString(&pipe_name, NFS41_SHADOW_PIPE_NAME);
    status = IoDeleteSymbolicLink(&pipe_name);
    if (status != STATUS_SUCCESS) {
        print_error("couldn't delete pipe symbolic link\n");
    }
    RxUnload(drv);

    DbgP("driver unloaded %p\n", drv);
    DbgR();
}
