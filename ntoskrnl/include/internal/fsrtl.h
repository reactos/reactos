/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/fsrtl.h
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
// Initialization Routines
//
BOOLEAN
NTAPI
FsRtlInitSystem(
    VOID
);

//
// Global data inside the File System Runtime Library
//
extern PERESOURCE FsRtlPagingIoResources;
extern PUCHAR _FsRtlLegalAnsiCharacterArray;
extern PAGED_LOOKASIDE_LIST FsRtlFileLockLookasideList;
