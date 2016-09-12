/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/fsrtl.h
 * PURPOSE:         Internal header for the File System Runtime Library
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

//
// Define this if you want debugging support
//
#define _FSRTL_DEBUG_                                   0x00

//
// These define the Debug Masks Supported
//
#define FSRTL_FASTIO_DEBUG                              0x01
#define FSRTL_OPLOCK_DEBUG                              0x02
#define FSRTL_TUNNEL_DEBUG                              0x04
#define FSRTL_MCB_DEBUG                                 0x08
#define FSRTL_NAME_DEBUG                                0x10
#define FSRTL_NOTIFY_DEBUG                              0x20
#define FSRTL_FILELOCK_DEBUG                            0x40
#define FSRTL_UNC_DEBUG                                 0x80
#define FSRTL_FILTER_DEBUG                              0x100
#define FSRTL_CONTEXT_DEBUG                             0x200

//
// Debug/Tracing support
//
#if _FSRTL_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define FSTRACE DbgPrintEx
#else
#define FSTRACE(x, ...)                                 \
    if (x & FsRtlpTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define FSTRACE(x, ...) DPRINT(__VA_ARGS__)
#endif

//
// Number of internal ERESOURCE structures allocated for callers to request
//
#define FSRTL_MAX_RESOURCES 16

//
// Number of maximum pair count per MCB
//
#define MAXIMUM_PAIR_COUNT  15

//
// Notifications flags
//
#define WATCH_TREE         0x01
#define NOTIFY_IMMEDIATELY 0x02
#define CLEANUP_IN_PROCESS 0x04
#define NOTIFY_LATER       0x08
#define WATCH_ROOT         0x10
#define DELETE_IN_PROCESS  0x20

//
// Internal structure for NOTIFY_SYNC
//
typedef struct _REAL_NOTIFY_SYNC
{
    FAST_MUTEX FastMutex;
    ULONG_PTR OwningThread;
    ULONG OwnerCount;
} REAL_NOTIFY_SYNC, * PREAL_NOTIFY_SYNC;

//
// Internal structure for notifications
//
typedef struct _NOTIFY_CHANGE
{
    PREAL_NOTIFY_SYNC NotifySync;
    PVOID FsContext;
    PVOID StreamID;
    PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback;
    PSECURITY_SUBJECT_CONTEXT SubjectContext;
    PSTRING FullDirectoryName;
    LIST_ENTRY NotifyList;
    LIST_ENTRY NotifyIrps;
    PFILTER_REPORT_CHANGE FilterCallback;
    USHORT Flags;
    UCHAR CharacterSize;
    ULONG CompletionFilter;
    PVOID AllocatedBuffer;
    PVOID Buffer;
    ULONG BufferLength;
    ULONG ThisBufferLength;
    ULONG DataLength;
    ULONG LastEntry;
    ULONG ReferenceCount;
    PEPROCESS OwningProcess;
} NOTIFY_CHANGE, *PNOTIFY_CHANGE;

//
// Internal structure for MCB Mapping pointer
//
typedef struct _INT_MAPPING
{
    VBN Vbn;
    LBN Lbn;
} INT_MAPPING, *PINT_MAPPING;

//
// Initialization Routines
//
VOID
NTAPI
FsRtlInitializeLargeMcbs(
    VOID
);

VOID
NTAPI
FsRtlInitializeTunnels(
    VOID
);

//
// File contexts Routines
//
VOID
NTAPI
FsRtlPTeardownPerFileObjectContexts(
    IN PFILE_OBJECT FileObject
);

BOOLEAN
NTAPI
FsRtlInitSystem(
    VOID
);

//
// Global data inside the File System Runtime Library
//
extern PERESOURCE FsRtlPagingIoResources;
extern PAGED_LOOKASIDE_LIST FsRtlFileLockLookasideList;
