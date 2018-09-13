
//
//  heappage.h
//

#ifndef _HEAP_PAGE_H_
#define _HEAP_PAGE_H_

//
//  #defining DEBUG_PAGE_HEAP will cause the page heap manager
//  to be compiled.  Only #define this flag if NOT kernel mode.
//  Probably want to define this just for checked-build (DBG).
//

#ifndef NTOS_KERNEL_RUNTIME
//    #if DBG
        #define DEBUG_PAGE_HEAP 1
//    #endif
#endif

#include "heappagi.h"

#ifndef DEBUG_PAGE_HEAP

//
//  These macro-based hooks should be defined to nothing so they
//  simply "go away" during compile if the debug heap manager is
//  not desired (retail builds).
//

#define IS_DEBUG_PAGE_HEAP_HANDLE( HeapHandle ) FALSE
#define IF_DEBUG_PAGE_HEAP_THEN_RETURN( Handle, ReturnThis )
#define IF_DEBUG_PAGE_HEAP_THEN_CALL( Handle, CallThis )
#define IF_DEBUG_PAGE_HEAP_THEN_BREAK( Handle, Text, ReturnThis )

#define HEAP_FLAG_PAGE_ALLOCS 0

#define RtlpDebugPageHeapValidate( HeapHandle, Flags, Address ) TRUE

#else // DEBUG_PAGE_HEAP

//
//  The following definitions and prototypes are the external interface
//  for hooking the debug heap manager in the retail heap manager.
//

#define HEAP_FLAG_PAGE_ALLOCS       0x01000000

#define HEAP_PROTECTION_ENABLED     0x02000000
#define HEAP_BREAK_WHEN_OUT_OF_VM   0x04000000
#define HEAP_NO_ALIGNMENT           0x08000000


#define IS_DEBUG_PAGE_HEAP_HANDLE( HeapHandle ) \
            (((PHEAP)(HeapHandle))->ForceFlags & HEAP_FLAG_PAGE_ALLOCS )


#define IF_DEBUG_PAGE_HEAP_THEN_RETURN( Handle, ReturnThis )                \
            {                                                               \
            if ( IS_DEBUG_PAGE_HEAP_HANDLE( Handle ))                       \
                {                                                           \
                return ReturnThis;                                          \
                }                                                           \
            }


#define IF_DEBUG_PAGE_HEAP_THEN_CALL( Handle, CallThis )                    \
            {                                                               \
            if ( IS_DEBUG_PAGE_HEAP_HANDLE( Handle ))                       \
                {                                                           \
                CallThis;                                                   \
                return;                                                     \
                }                                                           \
            }


#define IF_DEBUG_PAGE_HEAP_THEN_BREAK( Handle, Text, ReturnThis )           \
            {                                                               \
            if ( IS_DEBUG_PAGE_HEAP_HANDLE( Handle ))                       \
                {                                                           \
                RtlpDebugPageHeapBreak( Text );                             \
                return ReturnThis;                                          \
                }                                                           \
            }


PVOID
RtlpDebugPageHeapCreate(
    IN ULONG Flags,
    IN PVOID HeapBase,
    IN SIZE_T ReserveSize,
    IN SIZE_T CommitSize,
    IN PVOID Lock,
    IN PRTL_HEAP_PARAMETERS Parameters
    );

PVOID
RtlpDebugPageHeapAllocate(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

BOOLEAN
RtlpDebugPageHeapFree(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

PVOID
RtlpDebugPageHeapReAllocate(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN SIZE_T Size
    );

PVOID
RtlpDebugPageHeapDestroy(
    IN PVOID HeapHandle
    );

SIZE_T
RtlpDebugPageHeapSize(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

ULONG
RtlpDebugPageHeapGetProcessHeaps(
    ULONG NumberOfHeaps,
    PVOID *ProcessHeaps
    );

ULONG
RtlpDebugPageHeapCompact(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

BOOLEAN
RtlpDebugPageHeapValidate(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

NTSTATUS
RtlpDebugPageHeapWalk(
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    );

BOOLEAN
RtlpDebugPageHeapLock(
    IN PVOID HeapHandle
    );

BOOLEAN
RtlpDebugPageHeapUnlock(
    IN PVOID HeapHandle
    );

BOOLEAN
RtlpDebugPageHeapSetUserValue(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN PVOID UserValue
    );

BOOLEAN
RtlpDebugPageHeapGetUserInfo(
    IN  PVOID  HeapHandle,
    IN  ULONG  Flags,
    IN  PVOID  Address,
    OUT PVOID* UserValue,
    OUT PULONG UserFlags
    );

BOOLEAN
RtlpDebugPageHeapSetUserFlags(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    );

BOOLEAN
RtlpDebugPageHeapSerialize(
    IN PVOID HeapHandle
    );

NTSTATUS
RtlpDebugPageHeapExtend(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Base,
    IN SIZE_T Size
    );

NTSTATUS
RtlpDebugPageHeapZero(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

NTSTATUS
RtlpDebugPageHeapReset(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

NTSTATUS
RtlpDebugPageHeapUsage(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN OUT PRTL_HEAP_USAGE Usage
    );

BOOLEAN
RtlpDebugPageHeapIsLocked(
    IN PVOID HeapHandle
    );

VOID
RtlpDebugPageHeapBreak(
    PCH Text
    );


#endif // DEBUG_PAGE_HEAP

#endif // _HEAP_PAGE_H_
