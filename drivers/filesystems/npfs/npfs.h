/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/npfs.h
 * PURPOSE:     Named Pipe FileSystem Header
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

#ifndef _NPFS_PCH_
#define _NPFS_PCH_

/* INCLUDES *******************************************************************/

/* System Headers */
#include <ntifs.h>
#include <ndk/obfuncs.h>
#include <pseh/pseh2.h>
//#define UNIMPLEMENTED
//#define DPRINT1 DbgPrint

#define NDEBUG
#include <debug.h>
#define TRACE(...) /* DPRINT1("%s: ", __FUNCTION__); DbgPrint(__VA_ARGS__) */

/* Allow Microsoft Extensions */
#ifdef _MSC_VER
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4100)
#endif

#define MIN_INDEXED_LENGTH 5
#define MAX_INDEXED_LENGTH 9

/* TYPEDEFS & DEFINES *********************************************************/

//
// Pool Tags for NPFS (from pooltag.txt)
//
//  Npf* -npfs.sys - Npfs Allocations
//  NpFc - npfs.sys - CCB, client control block
//  NpFf - npfs.sys - FCB, file control block
//  NpFC - npfs.sys - ROOT_DCB CCB
//  NpFD - npfs.sys - DCB, directory block
//  NpFg - npfs.sys - Global storage
//  NpFi - npfs.sys - NPFS client info buffer.
//  NpFn - npfs.sys - Name block
//  NpFq - npfs.sys - Query template buffer used for directory query
//  NpFr - npfs.sys - DATA_ENTRY records(read / write buffers)
//  NpFs - npfs.sys - Client security context
//  NpFw - npfs.sys - Write block
//  NpFW - npfs.sys - Write block
#define NPFS_CCB_TAG            'cFpN'
#define NPFS_ROOT_DCB_CCB_TAG   'CFpN'
#define NPFS_DCB_TAG            'DFpN'
#define NPFS_FCB_TAG            'fFpN'
#define NPFS_GLOBAL_TAG         'gFpN'
#define NPFS_CLIENT_INFO_TAG    'iFpN'
#define NPFS_NAME_BLOCK_TAG     'nFpN'
#define NPFS_QUERY_TEMPLATE_TAG 'qFpN'
#define NPFS_DATA_ENTRY_TAG     'rFpN'
#define NPFS_CLIENT_SEC_CTX_TAG 'sFpN'
#define NPFS_WAIT_BLOCK_TAG     'tFpN'
#define NPFS_WRITE_BLOCK_TAG    'wFpN'

//
// NPFS bugchecking support
//
// We define the NpBugCheck macro which triggers a NPFS_FILE_SYSTEM bugcheck
// containing the source file ID number and the line where it was emitted, as
// described in the MSDN article "Bug Check 0x25: NPFS_FILE_SYSTEM".
//
// The bugcheck emits 4 ULONGs; the first one is made, in its high word, by
// the current source file ID and in its low word, by the line number; the
// three other ones are user-defined.
//
// In order to avoid redefinition of the same file ID in different source files,
// we gather all of them here, so that you will have to add (or remove) a new
// one as soon as you add (or remove) a source file from the NPFS driver code.
//
// To use the NpBugCheck macro in a source file, define at its beginning
// the constant NPFS_BUGCHECK_FILE_ID with one of the following file IDs,
// then use the bugcheck macro wherever you want.
//
#define NPFS_BUGCHECK_CLEANUP   0x0001
#define NPFS_BUGCHECK_CLOSE     0x0002
#define NPFS_BUGCHECK_CREATE    0x0003
#define NPFS_BUGCHECK_DATASUP   0x0004
#define NPFS_BUGCHECK_FILEINFO  0x0005
#define NPFS_BUGCHECK_FILEOBSUP 0x0006
#define NPFS_BUGCHECK_FLUSHBUF  0x0007
#define NPFS_BUGCHECK_FSCTRL    0x0008
#define NPFS_BUGCHECK_MAIN      0x0009
#define NPFS_BUGCHECK_PREFXSUP  0x000a
#define NPFS_BUGCHECK_READ      0x000b
#define NPFS_BUGCHECK_READSUP   0x000c
#define NPFS_BUGCHECK_SECURSUP  0x000d
#define NPFS_BUGCHECK_SEINFO    0x000e
#define NPFS_BUGCHECK_STATESUP  0x000f
#define NPFS_BUGCHECK_STRUCSUP  0x0010
#define NPFS_BUGCHECK_VOLINFO   0x0011
#define NPFS_BUGCHECK_WAITSUP   0x0012
#define NPFS_BUGCHECK_WRITE     0x0013
#define NPFS_BUGCHECK_WRITESUP  0x0014

#define NpBugCheck(p1, p2, p3)                              \
    KeBugCheckEx(NPFS_FILE_SYSTEM,                          \
                 (NPFS_BUGCHECK_FILE_ID << 16) | __LINE__,  \
                 (p1), (p2), (p3))

/* Node Type Codes for NPFS */
#define NPFS_NTC_VCB            1
#define NPFS_NTC_ROOT_DCB       2
#define NPFS_NTC_FCB            4
#define NPFS_NTC_CCB            6
#define NPFS_NTC_NONPAGED_CCB   7
#define NPFS_NTC_ROOT_DCB_CCB   8
typedef USHORT NODE_TYPE_CODE, *PNODE_TYPE_CODE;

/* Data Queue States */
typedef enum _NP_DATA_QUEUE_STATE
{
    ReadEntries = 0,
    WriteEntries = 1,
    Empty = 2
} NP_DATA_QUEUE_STATE;

/* Data Queue Entry Types */
typedef enum _NP_DATA_QUEUE_ENTRY_TYPE
{
    Buffered = 0,
    Unbuffered
} NP_DATA_QUEUE_ENTRY_TYPE;

/* An Input or Output Data Queue. Each CCB has two of these. */
typedef struct _NP_DATA_QUEUE
{
    LIST_ENTRY Queue;
    ULONG QueueState;
    ULONG BytesInQueue;
    ULONG EntriesInQueue;
    ULONG QuotaUsed;
    ULONG ByteOffset;
    ULONG Quota;
} NP_DATA_QUEUE, *PNP_DATA_QUEUE;

/* The Entries that go into the Queue */
typedef struct _NP_DATA_QUEUE_ENTRY
{
    LIST_ENTRY QueueEntry;
    ULONG DataEntryType;
    PIRP Irp;
    ULONG QuotaInEntry;
    PSECURITY_CLIENT_CONTEXT ClientSecurityContext;
    ULONG DataSize;
} NP_DATA_QUEUE_ENTRY, *PNP_DATA_QUEUE_ENTRY;

/* A Wait Queue. Only the VCB has one of these. */
typedef struct _NP_WAIT_QUEUE
{
    LIST_ENTRY WaitList;
    KSPIN_LOCK WaitLock;
} NP_WAIT_QUEUE, *PNP_WAIT_QUEUE;

/* The Entries in the Queue above, one for each Waiter. */
typedef struct _NP_WAIT_QUEUE_ENTRY
{
    PIRP Irp;
    KDPC Dpc;
    KTIMER Timer;
    PNP_WAIT_QUEUE WaitQueue;
    UNICODE_STRING AliasName;
    PFILE_OBJECT FileObject;
} NP_WAIT_QUEUE_ENTRY, *PNP_WAIT_QUEUE_ENTRY;

/* The event buffer in the NonPaged CCB */
typedef struct _NP_EVENT_BUFFER
{
    PKEVENT Event;
} NP_EVENT_BUFFER, *PNP_EVENT_BUFFER;

/* The CCB for the Root DCB */
typedef struct _NP_ROOT_DCB_CCB
{
    NODE_TYPE_CODE NodeType;
    PVOID Unknown;
    ULONG Unknown2;
} NP_ROOT_DCB_CCB, *PNP_ROOT_DCB_FCB;

/* The header that both FCB and DCB share */
typedef struct _NP_CB_HEADER
{
    NODE_TYPE_CODE NodeType;
    LIST_ENTRY DcbEntry;
    PVOID ParentDcb;
    ULONG CurrentInstances;
    ULONG ServerOpenCount;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
} NP_CB_HEADER, *PNP_CB_HEADER;

/* The footer that both FCB and DCB share */
typedef struct _NP_CB_FOOTER
{
    UNICODE_STRING FullName;
    UNICODE_STRING ShortName;
    UNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry;
} NP_CB_FOOTER;

/* A Directory Control Block (DCB) */
typedef struct _NP_DCB
{
    /* Common Header */
    NP_CB_HEADER;

    /* DCB-specific data */
    LIST_ENTRY NotifyList;
    LIST_ENTRY NotifyList2;
    LIST_ENTRY FcbList;
#ifndef _WIN64
    ULONG Pad;
#endif

    /* Common Footer */
    NP_CB_FOOTER;
} NP_DCB, *PNP_DCB;

/* A File Control BLock (FCB) */
typedef struct _NP_FCB
{
    /* Common Header */
    NP_CB_HEADER;

    /* FCB-specific fields */
    ULONG MaximumInstances;
    USHORT NamedPipeConfiguration;
    USHORT NamedPipeType;
    LARGE_INTEGER Timeout;
    LIST_ENTRY CcbList;
#ifdef _WIN64
    PVOID Pad[2];
#endif

    /* Common Footer */
    NP_CB_FOOTER;
} NP_FCB, *PNP_FCB;

C_ASSERT(FIELD_OFFSET(NP_FCB, PrefixTableEntry) == FIELD_OFFSET(NP_DCB, PrefixTableEntry));

/* The nonpaged portion of the CCB */
typedef struct _NP_NONPAGED_CCB
{
    NODE_TYPE_CODE NodeType;
    PNP_EVENT_BUFFER EventBuffer[2];
    ERESOURCE Lock;
} NP_NONPAGED_CCB, *PNP_NONPAGED_CCB;

/* A Client Control Block (CCB) */
typedef struct _NP_CCB
{
    NODE_TYPE_CODE NodeType;
    UCHAR NamedPipeState;
    UCHAR ReadMode[2];
    UCHAR CompletionMode[2];
    SECURITY_QUALITY_OF_SERVICE ClientQos;
    LIST_ENTRY CcbEntry;
    PNP_FCB Fcb;
    PFILE_OBJECT FileObject[2];
    PEPROCESS Process;
    PVOID ClientSession;
    PNP_NONPAGED_CCB NonPagedCcb;
    NP_DATA_QUEUE DataQueue[2];
    PSECURITY_CLIENT_CONTEXT ClientContext;
    LIST_ENTRY IrpList;
} NP_CCB, *PNP_CCB;

/* A Volume Control Block (VCB) */
typedef struct _NP_VCB
{
    NODE_TYPE_CODE NodeType;
    ULONG ReferenceCount;
    PNP_DCB RootDcb;
    UNICODE_PREFIX_TABLE PrefixTable;
    ERESOURCE Lock;
    RTL_GENERIC_TABLE EventTable;
    NP_WAIT_QUEUE WaitQueue;
} NP_VCB, *PNP_VCB;

extern PNP_VCB NpVcb;

/* Defines an alias */
typedef struct _NPFS_ALIAS
{
    struct _NPFS_ALIAS *Next;
    PUNICODE_STRING TargetName;
    UNICODE_STRING Name;
} NPFS_ALIAS, *PNPFS_ALIAS;

/* Private structure used to enumerate the alias values */
typedef struct _NPFS_QUERY_VALUE_CONTEXT
{
    BOOLEAN SizeOnly;
    SIZE_T FullSize;
    ULONG NumberOfAliases;
    ULONG NumberOfEntries;
    PNPFS_ALIAS CurrentAlias;
    PUNICODE_STRING CurrentTargetName;
    PWCHAR CurrentStringPointer;
} NPFS_QUERY_VALUE_CONTEXT, *PNPFS_QUERY_VALUE_CONTEXT;

extern PNPFS_ALIAS NpAliasList;
extern PNPFS_ALIAS NpAliasListByLength[MAX_INDEXED_LENGTH + 1 - MIN_INDEXED_LENGTH];

/* This structure is actually a user-mode structure and should go into a share header */
typedef struct _NP_CLIENT_PROCESS
{
    PVOID Unknown;
    PVOID Process;
    USHORT DataLength;
    WCHAR Buffer[17];
} NP_CLIENT_PROCESS, *PNP_CLIENT_PROCESS;

/* FUNCTIONS ******************************************************************/

/* Functions to lock/unlock the global VCB lock */

FORCEINLINE
VOID
NpAcquireSharedVcb(VOID)
{
    /* Acquire the lock in shared mode */
    ExAcquireResourceSharedLite(&NpVcb->Lock, TRUE);
}

FORCEINLINE
VOID
NpAcquireExclusiveVcb(VOID)
{
    /* Acquire the lock in exclusive mode */
    ExAcquireResourceExclusiveLite(&NpVcb->Lock, TRUE);
}

FORCEINLINE
VOID
NpReleaseVcb(VOID)
{
    /* Release the lock */
    ExReleaseResourceLite(&NpVcb->Lock);
}

//
// Function to process deferred IRPs outside the VCB lock but still within the
// critical region
//
FORCEINLINE
VOID
NpCompleteDeferredIrps(IN PLIST_ENTRY DeferredList)
{
    PLIST_ENTRY ThisEntry, NextEntry;
    PIRP Irp;

    /* Loop the list */
    ThisEntry = DeferredList->Flink;
    while (ThisEntry != DeferredList)
    {
        /* Remember the next entry, but don't switch to it yet */
        NextEntry = ThisEntry->Flink;

        /* Complete the IRP for this entry */
        Irp = CONTAINING_RECORD(ThisEntry, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);

        /* And now switch to the next one */
        ThisEntry = NextEntry;
    }
}

LONG
NTAPI
NpCompareAliasNames(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2);

BOOLEAN
NTAPI
NpDeleteEventTableEntry(IN PRTL_GENERIC_TABLE Table,
                        IN PVOID Buffer);

VOID
NTAPI
NpInitializeWaitQueue(IN PNP_WAIT_QUEUE WaitQueue);

NTSTATUS
NTAPI
NpUninitializeDataQueue(IN PNP_DATA_QUEUE DataQueue);

PLIST_ENTRY
NTAPI
NpGetNextRealDataQueueEntry(IN PNP_DATA_QUEUE DataQueue,
                            IN PLIST_ENTRY List);

PIRP
NTAPI
NpRemoveDataQueueEntry(IN PNP_DATA_QUEUE DataQueue,
                       IN BOOLEAN Flag,
                       IN PLIST_ENTRY List);

NTSTATUS
NTAPI
NpAddDataQueueEntry(IN ULONG NamedPipeEnd,
                    IN PNP_CCB Ccb,
                    IN PNP_DATA_QUEUE DataQueue,
                    IN ULONG Who,
                    IN ULONG Type,
                    IN ULONG DataSize,
                    IN PIRP Irp,
                    IN PVOID Buffer,
                    IN ULONG ByteOffset);

VOID
NTAPI
NpCompleteStalledWrites(IN PNP_DATA_QUEUE DataQueue,
                        IN PLIST_ENTRY List);

NTSTATUS
NTAPI
NpInitializeDataQueue(IN PNP_DATA_QUEUE DataQueue,
                      IN ULONG Quota);

NTSTATUS
NTAPI
NpCreateCcb(IN PNP_FCB Fcb,
            IN PFILE_OBJECT FileObject,
            IN UCHAR State,
            IN UCHAR ReadMode,
            IN UCHAR CompletionMode,
            IN ULONG InQuota,
            IN ULONG OutQuota,
            OUT PNP_CCB *NewCcb);

NTSTATUS
NTAPI
NpCreateFcb(IN PNP_DCB Dcb,
            IN PUNICODE_STRING PipeName,
            IN ULONG MaximumInstances,
            IN LARGE_INTEGER Timeout,
            IN USHORT NamedPipeConfiguration,
            IN USHORT NamedPipeType,
            OUT PNP_FCB *NewFcb);

NTSTATUS
NTAPI
NpCreateRootDcb(VOID);

NTSTATUS
NTAPI
NpCreateRootDcbCcb(IN PNP_ROOT_DCB_FCB *NewRootCcb);

VOID
NTAPI
NpInitializeVcb(VOID);

VOID
NTAPI
NpDeleteCcb(IN PNP_CCB Ccb,
            IN PLIST_ENTRY ListEntry);

VOID
NTAPI
NpDeleteFcb(IN PNP_FCB Fcb,
            IN PLIST_ENTRY ListEntry);

NTSTATUS
NTAPI
NpFsdCreateNamedPipe(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdCreate(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdClose(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdCleanup(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdFileSystemControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp);

NTSTATUS
NTAPI
NpSetConnectedPipeState(IN PNP_CCB Ccb,
                        IN PFILE_OBJECT FileObject,
                        IN PLIST_ENTRY List);

NTSTATUS
NTAPI
NpSetListeningPipeState(IN PNP_CCB Ccb,
                        IN PIRP Irp,
                        IN PLIST_ENTRY List);

NTSTATUS
NTAPI
NpSetDisconnectedPipeState(IN PNP_CCB Ccb,
                           IN PLIST_ENTRY List);

NTSTATUS
NTAPI
NpSetClosingPipeState(IN PNP_CCB Ccb,
                      IN PIRP Irp,
                      IN ULONG NamedPipeEnd,
                      IN PLIST_ENTRY List);

VOID
NTAPI
NpFreeClientSecurityContext(IN PSECURITY_CLIENT_CONTEXT ClientContext);

NTSTATUS
NTAPI
NpImpersonateClientContext(IN PNP_CCB Ccb);

VOID
NTAPI
NpCopyClientContext(IN PNP_CCB Ccb,
                    IN PNP_DATA_QUEUE_ENTRY DataQueueEntry);

VOID
NTAPI
NpUninitializeSecurity(IN PNP_CCB Ccb);

NTSTATUS
NTAPI
NpInitializeSecurity(IN PNP_CCB Ccb,
                     IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
                     IN PETHREAD Thread);

NTSTATUS
NTAPI
NpGetClientSecurityContext(IN ULONG NamedPipeEnd,
                           IN PNP_CCB Ccb,
                           IN PETHREAD Thread,
                           IN PSECURITY_CLIENT_CONTEXT *Context);

VOID
NTAPI
NpSetFileObject(IN PFILE_OBJECT FileObject,
                IN PVOID PrimaryContext,
                IN PVOID Ccb,
                IN ULONG NamedPipeEnd);

NODE_TYPE_CODE
NTAPI
NpDecodeFileObject(IN PFILE_OBJECT FileObject,
                   OUT PVOID *PrimaryContext OPTIONAL,
                   OUT PNP_CCB *Ccb,
                   OUT PULONG NamedPipeEnd OPTIONAL);

PNP_FCB
NTAPI
NpFindPrefix(IN PUNICODE_STRING Name,
             IN ULONG CaseInsensitiveIndex,
             IN PUNICODE_STRING Prefix);

NTSTATUS
NTAPI
NpFindRelativePrefix(IN PNP_DCB Dcb,
                     IN PUNICODE_STRING Name,
                     IN ULONG CaseInsensitiveIndex,
                     IN PUNICODE_STRING Prefix,
                     OUT PNP_FCB *FoundFcb);

VOID
NTAPI
NpCheckForNotify(IN PNP_DCB Dcb,
                 IN BOOLEAN SecondList,
                 IN PLIST_ENTRY List);

NTSTATUS
NTAPI
NpAddWaiter(IN PNP_WAIT_QUEUE WaitQueue,
            IN LARGE_INTEGER WaitTime,
            IN PIRP Irp,
            IN PUNICODE_STRING AliasName);

NTSTATUS
NTAPI
NpCancelWaiter(IN PNP_WAIT_QUEUE WaitQueue,
               IN PUNICODE_STRING PipeName,
               IN NTSTATUS Status,
               IN PLIST_ENTRY ListEntry);


IO_STATUS_BLOCK
NTAPI
NpReadDataQueue(IN PNP_DATA_QUEUE DataQueue,
                IN BOOLEAN Peek,
                IN BOOLEAN ReadOverflowOperation,
                IN PVOID Buffer,
                IN ULONG BufferSize,
                IN ULONG Mode,
                IN PNP_CCB Ccb,
                IN PLIST_ENTRY List);


NTSTATUS
NTAPI
NpWriteDataQueue(IN PNP_DATA_QUEUE WriteQueue,
                 IN ULONG Mode,
                 IN PVOID OutBuffer,
                 IN ULONG OutBufferSize,
                 IN ULONG PipeType,
                 OUT PULONG BytesWritten,
                 IN PNP_CCB Ccb,
                 IN ULONG NamedPipeEnd,
                 IN PETHREAD Thread,
                 IN PLIST_ENTRY List);

NTSTATUS
NTAPI
NpFsdRead(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);

_Function_class_(FAST_IO_READ)
_IRQL_requires_same_
BOOLEAN
NTAPI
NpFastRead(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject);

_Function_class_(FAST_IO_WRITE)
_IRQL_requires_same_
BOOLEAN
NTAPI
NpFastWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject);


NTSTATUS
NTAPI
NpFsdWrite(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdSetInformation(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdQueryInformation(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp);


NTSTATUS
NTAPI
NpFsdQuerySecurityInfo(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdSetSecurityInfo(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp);

NTSTATUS
NTAPI
NpFsdQueryVolumeInformation(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp);

#endif /* _NPFS_PCH_ */
