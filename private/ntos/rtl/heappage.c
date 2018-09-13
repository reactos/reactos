/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name:

    heappage.c

Abstract:

    Implementation of NT RtlHeap family of APIs for debugging
    applications with heap usage bugs.  Each allocation returned to
    the calling app is placed at the end of a virtual page such that
    the following virtual page is protected (ie, NO_ACCESS).
    So, when the errant app attempts to reference or modify memory
    beyond the allocated portion of a heap block, an access violation
    is immediately caused.  This facilitates debugging the app
    because the access violation occurs at the exact point in the
    app where the heap corruption or abuse would occur.  Note that
    significantly more memory (pagefile) is required to run an app
    using this heap implementation as opposed to the retail heap
    manager.

Author:

    Tom McGuire (TomMcg) 06-Jan-1995
    Silviu Calinoiu (SilviuC) 22-Feb-2000

Revision History:

--*/

#include "ntrtlp.h"
#include "heappage.h"       // external interface (hooks) to debug heap manager
#include "heappagi.h"
#include "heappriv.h"

int __cdecl sprintf(char *, const char *, ...);

//
//  Remainder of entire file is wrapped with #ifdef DEBUG_PAGE_HEAP so that
//  it will compile away to nothing if DEBUG_PAGE_HEAP is not defined in
//  heappage.h
//

#ifdef DEBUG_PAGE_HEAP

//
// Page size
//

#if defined(_X86_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x1000
    #endif
    #define USER_ALIGNMENT 8
#elif defined(_MIPS_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x1000
    #endif
    #define USER_ALIGNMENT 8
#elif defined(_PPC_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x1000
    #endif
    #define USER_ALIGNMENT 8
#elif defined(_IA64_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x2000
    #endif
    #define USER_ALIGNMENT 16
#elif defined(_AXP64_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x2000
    #endif
    #define USER_ALIGNMENT 16
#elif defined(_ALPHA_)
    #ifndef PAGE_SIZE
    #define PAGE_SIZE   0x2000
    #endif
    #define USER_ALIGNMENT 8
#else
    #error  // platform not defined
#endif

//
// Few constants
//

#define DPH_HEAP_SIGNATURE       0xFFEEDDCC
#define FILL_BYTE                0xEE
#define HEAD_FILL_SIZE           0x10
#define RESERVE_SIZE             0x100000
#define VM_UNIT_SIZE             0x10000
#define POOL_SIZE                0x4000
#define INLINE                   __inline
#define MIN_FREE_LIST_LENGTH     8

//
// Few macros
//

#define ROUNDUP2( x, n ) ((( x ) + (( n ) - 1 )) & ~(( n ) - 1 ))

#if INTERNAL_DEBUG
#define DEBUG_CODE( a ) a
#else
#define DEBUG_CODE( a )
#endif

#define RETAIL_ASSERT( a ) ( (a) ? TRUE : \
    RtlpDebugPageHeapAssert( "Page heap: assert: (" #a ")\n" ))

#define DEBUG_ASSERT( a ) DEBUG_CODE( RETAIL_ASSERT( a ))

#define HEAP_HANDLE_FROM_ROOT( HeapRoot ) \
    ((PVOID)(((PCHAR)(HeapRoot)) - PAGE_SIZE ))

#define IF_GENERATE_EXCEPTION( Flags, Status ) {                \
    if (( Flags ) & HEAP_GENERATE_EXCEPTIONS )                  \
        RtlpDebugPageHeapException((ULONG)(Status));            \
    }

#define OUT_OF_VM_BREAK( Flags, szText ) {                      \
    if (( Flags ) & HEAP_BREAK_WHEN_OUT_OF_VM )                 \
        RtlpDebugPageHeapBreak(( szText ));                     \
    }

//
// List manipulation macros
//

#define ENQUEUE_HEAD( Node, Head, Tail ) {          \
            (Node)->pNextAlloc = (Head);            \
            if ((Head) == NULL )                    \
                (Tail) = (Node);                    \
            (Head) = (Node);                        \
            }

#define ENQUEUE_TAIL( Node, Head, Tail ) {          \
            if ((Tail) == NULL )                    \
                (Head) = (Node);                    \
            else                                    \
                (Tail)->pNextAlloc = (Node);        \
            (Tail) = (Node);                        \
            }

#define DEQUEUE_NODE( Node, Prev, Head, Tail ) {    \
            PVOID Next = (Node)->pNextAlloc;        \
            if ((Head) == (Node))                   \
                (Head) = Next;                      \
            if ((Tail) == (Node))                   \
                (Tail) = (Prev);                    \
            if ((Prev) != (NULL))                   \
                (Prev)->pNextAlloc = Next;          \
            }

//
// Bias/unbias pointer
//

#define BIAS_POINTER(p)      ((PVOID)((ULONG_PTR)(p) | (ULONG_PTR)0x01))
#define UNBIAS_POINTER(p)    ((PVOID)((ULONG_PTR)(p) & ~((ULONG_PTR)0x01)))
#define IS_BIASED_POINTER(p) ((PVOID)((ULONG_PTR)(p) & (ULONG_PTR)0x01))

//
// Protect/Unprotect heap structures macros
//

#define PROTECT_HEAP_STRUCTURES( HeapRoot ) {                           \
            if ((HeapRoot)->HeapFlags & HEAP_PROTECTION_ENABLED )       \
                RtlpDebugPageHeapProtectStructures( (HeapRoot) );       \
            }

#define UNPROTECT_HEAP_STRUCTURES( HeapRoot ) {                         \
            if ((HeapRoot)->HeapFlags & HEAP_PROTECTION_ENABLED )       \
                RtlpDebugPageHeapUnProtectStructures( (HeapRoot) );     \
            }

//
// RtlpDebugPageHeap
//
// Global variable that marks that page heap is enabled. It is set
// in \nt\base\ntdll\ldrinit.c by reading the GlobalFlag registry
// value (system wide or per process one) and checking if the 
// FLG_HEAP_PAGE_ALLOCS is set.
//

BOOLEAN RtlpDebugPageHeap;                    

//
// Internal version used to figure out what are people running
// in various VBLs.
//

PCHAR RtlpDphVersion = "03/14/2000";

//
// Page heaps list manipulation.
//
// We maintain a list of all page heaps in the process to support
// APIs like GetProcessHeaps. The list is also useful for debug
// extensions that need to iterate the heaps. The list is protected
// by RtlpDphHeapListCriticalSection lock.
//

BOOLEAN RtlpDphHeapListHasBeenInitialized;
RTL_CRITICAL_SECTION RtlpDphHeapListCriticalSection;
PDPH_HEAP_ROOT RtlpDphHeapListHead;
PDPH_HEAP_ROOT RtlpDphHeapListTail;
ULONG RtlpDphHeapListCount;

//
// `RtlpDebugPageHeapGlobalFlags' stores the global page heap flags.
// The value of this variable is copied into the per heap
// flags (ExtraFlags field) during heap creation.
//
// The initial value is so that by default we use page heap only with
// normal allocations. This way if system wide global flag for page
// heap is set the machine will still boot. After that we can enable
// page heap with "sudden death" for specific processes. The most useful
// flags for this case would be:
//
//    PAGE_HEAP_ENABLE_PAGE_HEAP       |
//    PAGE_HEAP_COLLECT_STACK_TRACES   ;
//
// If no flags specified the default is page heap light with
// stack trace collection.
//

ULONG RtlpDphGlobalFlags = PAGE_HEAP_COLLECT_STACK_TRACES;

//
// Page heap global flags.
//
// These values are read from registry in \nt\base\ntdll\ldrinit.c.
//

ULONG RtlpDphSizeRangeStart;
ULONG RtlpDphSizeRangeEnd;
ULONG RtlpDphDllRangeStart;
ULONG RtlpDphDllRangeEnd;
ULONG RtlpDphRandomProbability;
WCHAR RtlpDphTargetDlls [512];
UNICODE_STRING RtlpDphTargetDllsUnicode;

//
// `RtlpDphDebugLevel' controls debug messages in the code.
//
// (SilviuC): The value should always be zero for the retail bits.
//

#define DPH_DEBUG_INTERNAL_VALIDATION 0x0001
#define DPH_DEBUG_RESERVED_2          0x0002
#define DPH_DEBUG_RESERVED_4          0x0004
#define DPH_DEBUG_RESERVED_8          0x0008
#define DPH_DEBUG_DECOMMIT_RANGES     0x0010
#define DPH_DEBUG_BREAK_FOR_SIZE_ZERO 0x0020
#define DPH_DEBUG_BREAK_FOR_NULL_FREE 0x0040
#define DPH_DEBUG_NEVER_FREE          0x0080
#define DPH_DEBUG_SLOW_CHECKS         0x0100

ULONG RtlpDphDebugLevel;

//
// `RtlpDphGlobalCounter' contains process wide counters.
// The definition of counters is in `rtl\heappagi.h'
//
// `RtlpDphSizeCounter' contains size distribution for allocations
// using 128 bytes granularity.
//

#define BUMP_GLOBAL_COUNTER(n) InterlockedIncrement(&(RtlpDphGlobalCounter[n]))

#define BUMP_SIZE_COUNTER(Size) if (Size/128 <= MAX_SIZE_COUNTER_INDEX) {    \
        InterlockedIncrement(&(RtlpDphSizeCounter[Size/128]));               \
    }                                                                        \
    else {                                                                   \
        InterlockedIncrement(&(RtlpDphSizeCounter[MAX_SIZE_COUNTER_INDEX])); \
    }

#define MAX_GLOBAL_COUNTER_INDEX 15
#define MAX_SIZE_COUNTER_INDEX 64

ULONG RtlpDphGlobalCounter[MAX_GLOBAL_COUNTER_INDEX + 1];
ULONG RtlpDphSizeCounter[MAX_SIZE_COUNTER_INDEX + 1];

//
// Threshold for delaying a free operation in the normal heap.
// If we get over this limit we start actually freeing blocks.
//

SIZE_T RtlpDphDelayedFreeCacheSize = 256 * PAGE_SIZE;

//
// Process wide trace database and the maximum size it can
// grow to.
//

SIZE_T RtlpDphTraceDatabaseMaximumSize = 256 * PAGE_SIZE;
PRTL_TRACE_DATABASE RtlpDphTraceDatabase;

//
// Support for normal heap allocations
//
// In order to make better use of memory available page heap will
// allocate some of the block into a normal NT heap that it manages.
// We will call these blocks "normal blocks" as opposed to "page blocks".
//
// All normal blocks have the requested size increased by DPH_BLOCK_INFORMATION.
// The address returned is of course of the first byte after the block
// info structure. Upon free, blocks are checked for corruption and
// then released into the normal heap.
// 
// All these normal heap functions are called with the page heap
// lock acquired.
// 

PVOID
RtlpDphNormalHeapAllocate (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    SIZE_T Size
    );

BOOLEAN
RtlpDphNormalHeapFree (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    PVOID Block
    );

PVOID
RtlpDphNormalHeapReAllocate (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    PVOID OldBlock,
    SIZE_T Size
    );

SIZE_T
RtlpDphNormalHeapSize (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    PVOID Block
    );

BOOLEAN
RtlpDphNormalHeapSetUserFlags(
    IN PDPH_HEAP_ROOT Heap,
    IN ULONG Flags,
    IN PVOID Address,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    );

BOOLEAN
RtlpDphNormalHeapSetUserValue(
    IN PDPH_HEAP_ROOT Heap,
    IN ULONG Flags,
    IN PVOID Address,
    IN PVOID UserValue
    );

BOOLEAN
RtlpDphNormalHeapGetUserInfo(
    IN PDPH_HEAP_ROOT Heap,
    IN  ULONG  Flags,
    IN  PVOID  Address,
    OUT PVOID* UserValue,
    OUT PULONG UserFlags
    );

BOOLEAN
RtlpDphNormalHeapValidate(
    IN PDPH_HEAP_ROOT Heap,
    IN ULONG Flags,
    IN PVOID Address
    );

//
// Support for DPH_BLOCK_INFORMATION management
//
// This header information prefixes both the normal and page heap
// blocks.
//

VOID
RtlpDphReportCorruptedBlock (
    PVOID Block,
    ULONG Reason
    );

BOOLEAN
RtlpDphIsNormalHeapBlock (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    PULONG Reason,
    BOOLEAN CheckPattern
    );

BOOLEAN
RtlpDphIsNormalFreeHeapBlock (
    PVOID Block,
    PULONG Reason,
    BOOLEAN CheckPattern
    );

BOOLEAN
RtlpDphIsPageHeapBlock (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    PULONG Reason,
    BOOLEAN CheckPattern
    );

BOOLEAN
RtlpDphWriteNormalHeapBlockInformation (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    SIZE_T RequestedSize,
    SIZE_T ActualSize
    );

BOOLEAN
RtlpDphWritePageHeapBlockInformation (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    SIZE_T RequestedSize,
    SIZE_T ActualSize
    );

//
// Delayed free queue (of normal heap allocations) management
//

VOID
RtlpDphInitializeDelayedFreeQueue (
    );

VOID
RtlpDphAddToDelayedFreeQueue (
    PDPH_BLOCK_INFORMATION Info
    );

BOOLEAN
RtlpDphNeedToTrimDelayedFreeQueue (
    PSIZE_T TrimSize
    );

VOID
RtlpDphTrimDelayedFreeQueue (
    SIZE_T TrimSize,
    ULONG Flags
    );

VOID
RtlpDphFreeDelayedBlocksFromHeap (
    PVOID PageHeap,
    PVOID NormalHeap
    );

//
// Decision normal heap vs. page heap
//

RtlpDphShouldAllocateInPageHeap (
    PDPH_HEAP_ROOT Heap,
    SIZE_T Size
    );

//
// Stack trace detection for trace database.
//

PRTL_TRACE_BLOCK 
RtlpDphLogStackTrace (
    ULONG FramesToSkip
    );

//
//  Page heap general support functions
//

VOID
RtlpDebugPageHeapBreak(
    IN PCH Text
    );

BOOLEAN
RtlpDebugPageHeapAssert(
    IN PCH Text
    );

VOID
RtlpDebugPageHeapEnterCritSect(
    IN PDPH_HEAP_ROOT HeapRoot,
    IN ULONG          Flags
    );

INLINE
VOID
RtlpDebugPageHeapLeaveCritSect(
    IN PDPH_HEAP_ROOT HeapRoot
    );

VOID
RtlpDebugPageHeapException(
    IN ULONG ExceptionCode
    );

PVOID
RtlpDebugPageHeapPointerFromHandle(
    IN PVOID HeapHandle
    );

PCCH
RtlpDebugPageHeapProtectionText(
    IN     ULONG Access,
    IN OUT PCHAR Buffer
    );

//
// Virtual memory manipulation functions
//

BOOLEAN
RtlpDebugPageHeapRobustProtectVM(
    IN PVOID   VirtualBase,
    IN SIZE_T  VirtualSize,
    IN ULONG   NewAccess,
    IN BOOLEAN Recursion
    );

INLINE
BOOLEAN
RtlpDebugPageHeapProtectVM(
    IN PVOID   VirtualBase,
    IN SIZE_T  VirtualSize,
    IN ULONG   NewAccess
    );

INLINE
PVOID
RtlpDebugPageHeapAllocateVM(
    IN SIZE_T nSize
    );

INLINE
BOOLEAN
RtlpDebugPageHeapReleaseVM(
    IN PVOID pVirtual
    );

INLINE
BOOLEAN
RtlpDebugPageHeapCommitVM(
    IN PVOID pVirtual,
    IN SIZE_T nSize
    );

INLINE
BOOLEAN
RtlpDebugPageHeapDecommitVM(
    IN PVOID pVirtual,
    IN SIZE_T nSize
    );

//
// Target dlls logic
//
// RtlpDphTargetDllsLoadCallBack is called in ntdll\ldrapi.c 
// (LdrpLoadDll) whenever a new dll is loaded in the process
// space.
//

VOID
RtlpDphTargetDllsLogicInitialize (
    );

VOID
RtlpDphTargetDllsLoadCallBack (
    PUNICODE_STRING Name,
    PVOID Address,
    ULONG Size
    );

const WCHAR *
RtlpDphIsDllTargeted (
    const WCHAR * Name
    );

//
// Internal heap validation
//

VOID
RtlpDphInternalValidatePageHeap (
    PDPH_HEAP_ROOT Heap,
    PUCHAR ExemptAddress,
    SIZE_T ExemptSize
    );

/////////////////////////////////////////////////////////////////////
///////////////////////////////// Page heap general support functions
/////////////////////////////////////////////////////////////////////

VOID
RtlpDebugPageHeapBreak(
    IN PCH Text
    )
{
    DbgPrint( Text );
    DbgBreakPoint();
}

BOOLEAN
RtlpDebugPageHeapAssert(
    IN PCH Text
    )
{
    RtlpDebugPageHeapBreak( Text );
    return FALSE;
}

VOID
RtlpDebugPageHeapEnterCritSect(
    IN PDPH_HEAP_ROOT HeapRoot,
    IN ULONG          Flags
    )
{
    if (Flags & HEAP_NO_SERIALIZE) {

        if (! RtlTryEnterCriticalSection( HeapRoot->HeapCritSect )) {

            if (HeapRoot->nRemoteLockAcquired == 0) {

                //
                //  Another thread owns the CritSect.  This is an application
                //  bug since multithreaded access to heap was attempted with
                //  the HEAP_NO_SERIALIZE flag specified.
                //

                RtlpDebugPageHeapBreak( "Page heap: Multithreaded access with HEAP_NO_SERIALIZE\n" );

                //
                //  In the interest of allowing the errant app to continue,
                //  we'll force serialization and continue.
                //

                HeapRoot->HeapFlags &= ~HEAP_NO_SERIALIZE;

            }

            RtlEnterCriticalSection( HeapRoot->HeapCritSect );

        }
    }
    else {
        RtlEnterCriticalSection( HeapRoot->HeapCritSect );
    }
}

INLINE
VOID
RtlpDebugPageHeapLeaveCritSect(
    IN PDPH_HEAP_ROOT HeapRoot
    )
{
    RtlLeaveCriticalSection( HeapRoot->HeapCritSect );
}

VOID
RtlpDebugPageHeapException(
    IN ULONG ExceptionCode
    )
{
    EXCEPTION_RECORD ER;

    ER.ExceptionCode    = ExceptionCode;
    ER.ExceptionFlags   = 0;
    ER.ExceptionRecord  = NULL;
    ER.ExceptionAddress = RtlpDebugPageHeapException;
    ER.NumberParameters = 0;
    RtlRaiseException( &ER );
}

PVOID
RtlpDebugPageHeapPointerFromHandle(
    IN PVOID HeapHandle
    )
{
    try {
        if (((PHEAP)(HeapHandle))->ForceFlags & HEAP_FLAG_PAGE_ALLOCS) {

            PDPH_HEAP_ROOT HeapRoot = (PVOID)(((PCHAR)(HeapHandle)) + PAGE_SIZE );

            if (HeapRoot->Signature == DPH_HEAP_SIGNATURE) {
                return HeapRoot;
            }
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
    }

    RtlpDebugPageHeapBreak( "Page heap: Bad heap handle\n" );
    return NULL;
}

PCCH
RtlpDebugPageHeapProtectionText(
    IN     ULONG Access,
    IN OUT PCHAR Buffer
    )
{
    switch (Access) {
    case PAGE_NOACCESS:          return "PAGE_NOACCESS";
    case PAGE_READONLY:          return "PAGE_READONLY";
    case PAGE_READWRITE:         return "PAGE_READWRITE";
    case PAGE_WRITECOPY:         return "PAGE_WRITECOPY";
    case PAGE_EXECUTE:           return "PAGE_EXECUTE";
    case PAGE_EXECUTE_READ:      return "PAGE_EXECUTE_READ";
    case PAGE_EXECUTE_READWRITE: return "PAGE_EXECUTE_READWRITE";
    case PAGE_EXECUTE_WRITECOPY: return "PAGE_EXECUTE_WRITECOPY";
    case PAGE_GUARD:             return "PAGE_GUARD";
    case 0:                      return "UNKNOWN";
    default:                     sprintf( Buffer, "0x%08X", Access );
        return Buffer;
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////// Virtual memory manipulation functions
/////////////////////////////////////////////////////////////////////

BOOLEAN
RtlpDebugPageHeapRobustProtectVM(
    IN PVOID   VirtualBase,
    IN SIZE_T  VirtualSize,
    IN ULONG   NewAccess,
    IN BOOLEAN Recursion
    )
{
    PVOID  CopyOfVirtualBase = VirtualBase;
    SIZE_T CopyOfVirtualSize = VirtualSize;
    ULONG  OldAccess;
    NTSTATUS Status;

    Status = ZwProtectVirtualMemory(
        NtCurrentProcess(),
        &CopyOfVirtualBase,
        &CopyOfVirtualSize,
        NewAccess,
        &OldAccess
        );

    if (NT_SUCCESS( Status ))
        return TRUE;

    if (! Recursion) {

        //
        //  We failed to change the protection on a range of memory.
        //  This can happen if if the range of memory spans more than
        //  one adjancent blocks allocated by separate calls to
        //  ZwAllocateVirtualMemory.  It also seems fails occasionally
        //  for reasons unknown to me, but always when attempting to
        //  change the protection on more than one page in a single call.
        //  So, fall back to changing pages individually in this range.
        //  This should be rare, so it should not be a performance problem.
        //

        PCHAR VirtualExtent = (PCHAR)ROUNDUP2((ULONG_PTR)((PCHAR)VirtualBase + VirtualSize ), PAGE_SIZE );
        PCHAR VirtualPage   = (PCHAR)((ULONG_PTR)VirtualBase & ~( PAGE_SIZE - 1 ));
        BOOLEAN SuccessAll  = TRUE;
        BOOLEAN SuccessOne;

        while (VirtualPage < VirtualExtent) {

            SuccessOne = RtlpDebugPageHeapRobustProtectVM(
                VirtualPage,
                PAGE_SIZE,
                NewAccess,
                TRUE
                );

            if (! SuccessOne) {
                SuccessAll = FALSE;
            }

            VirtualPage += PAGE_SIZE;

        }

        return SuccessAll;      // TRUE if all succeeded, FALSE if any failed
    }

    else {

        MEMORY_BASIC_INFORMATION mbi;
        CHAR OldProtectionText[ 12 ];     // big enough for "0x12345678"
        CHAR NewProtectionText[ 12 ];     // big enough for "0x12345678"

        mbi.Protect = 0;    // in case ZwQueryVirtualMemory fails

        ZwQueryVirtualMemory(
            NtCurrentProcess(),
            VirtualBase,
            MemoryBasicInformation,
            &mbi,
            sizeof( mbi ),
            NULL
            );

        DbgPrint(
            "Page heap: Failed changing VM at %08X size 0x%X\n"
            "          from %s to %s (Status %08X)\n",
            VirtualBase,
            VirtualSize,
            RtlpDebugPageHeapProtectionText( mbi.Protect, OldProtectionText ),
            RtlpDebugPageHeapProtectionText( NewAccess, NewProtectionText ),
            Status
            );
    }

    return FALSE;
}

INLINE
BOOLEAN
RtlpDebugPageHeapProtectVM(
    IN PVOID   VirtualBase,
    IN SIZE_T  VirtualSize,
    IN ULONG   NewAccess
    )
{
    return RtlpDebugPageHeapRobustProtectVM( VirtualBase, VirtualSize, NewAccess, FALSE );
}

INLINE
PVOID
RtlpDebugPageHeapAllocateVM(
    IN SIZE_T nSize
    )
{
    NTSTATUS Status;
    PVOID pVirtual;

    pVirtual = NULL;

    Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
        &pVirtual,
        0,
        &nSize,
        MEM_COMMIT,
        PAGE_NOACCESS );

    return NT_SUCCESS( Status ) ? pVirtual : NULL;
}

INLINE
BOOLEAN
RtlpDebugPageHeapReleaseVM(
    IN PVOID pVirtual
    )
{
    SIZE_T nSize = 0;

    return NT_SUCCESS( ZwFreeVirtualMemory( NtCurrentProcess(),
        &pVirtual,
        &nSize,
        MEM_RELEASE ));
}

INLINE
BOOLEAN
RtlpDebugPageHeapCommitVM(
    IN PVOID pVirtual,
    IN SIZE_T nSize
    )
{
    PCHAR pStart, pEnd, pCurrent;
    NTSTATUS Status;
    SIZE_T CommitSize;
    BOOLEAN Failed = FALSE;

    pStart = (PCHAR)((ULONG_PTR)pVirtual & ~(PAGE_SIZE - 1));
    pEnd = (PCHAR)(((ULONG_PTR)pVirtual + nSize) & ~(PAGE_SIZE - 1));

    for (pCurrent = pStart; pCurrent < pEnd; pCurrent += PAGE_SIZE) {

        CommitSize = PAGE_SIZE;

        Status = ZwAllocateVirtualMemory(
            NtCurrentProcess(),
            &pCurrent,
            0,
            &CommitSize,
            MEM_COMMIT,
            PAGE_NOACCESS);

        if (! NT_SUCCESS(Status)) {

            //
            // The call can fail in low memory conditions. In this case we
            // try to recover and will probably fail the original allocation.
            //

            if ((RtlpDphDebugLevel & DPH_DEBUG_DECOMMIT_RANGES)) {
                DbgPrint ("Page heap: Commit (%p) failed with %08X\n", pCurrent, Status);
                DbgBreakPoint();
            }

            Failed = TRUE;
            break;
        }
    }


    if (Failed) {

        //
        // We need to roll back whatever succeeded.
        //

        for (pCurrent -= PAGE_SIZE; pCurrent >= pStart && pCurrent < pEnd; pCurrent -= PAGE_SIZE) {

            CommitSize = PAGE_SIZE;

            Status = ZwFreeVirtualMemory(
                NtCurrentProcess(),
                &pCurrent,
                &CommitSize,
                MEM_DECOMMIT);

            if (! NT_SUCCESS(Status)) {

                //
                // There is now valid reason known to me for a correct free operation
                // failure. So, in this case we make a little bit of fuss about it.
                //

                DbgPrint ("Page heap: Decommit (%p) failed with %08X\n", pCurrent, Status);

                if ((RtlpDphDebugLevel & DPH_DEBUG_DECOMMIT_RANGES)) {
                    DbgBreakPoint();
                }
            }
        }
    }

    if (Failed) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

INLINE
BOOLEAN
RtlpDebugPageHeapDecommitVM(
    IN PVOID pVirtual,
    IN SIZE_T nSize
    )
{
    PCHAR pStart, pEnd, pCurrent;
    NTSTATUS Status;
    SIZE_T DecommitSize;
    BOOLEAN Failed = FALSE;

    pStart = (PCHAR)((ULONG_PTR)pVirtual & ~(PAGE_SIZE - 1));
    pEnd = (PCHAR)(((ULONG_PTR)pVirtual + nSize) & ~(PAGE_SIZE - 1));

    for (pCurrent = pStart; pCurrent < pEnd; pCurrent += PAGE_SIZE) {

        DecommitSize = PAGE_SIZE;

        Status = ZwFreeVirtualMemory(
            NtCurrentProcess(),
            &pCurrent,
            &DecommitSize,
            MEM_DECOMMIT);

        if (! NT_SUCCESS(Status)) {

            //
            // There is now valid reason known to me for a correct free operation
            // failure. So, in this case we make a little bit of fuss about it.
            //

            DbgPrint ("Page heap: Decommit (%p) failed with %08X\n", pCurrent, Status);

            if ((RtlpDphDebugLevel & DPH_DEBUG_DECOMMIT_RANGES)) {
                DbgBreakPoint();
            }

            Failed = TRUE;
        }
    }

    if (Failed) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

/////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Internal page heap functions
/////////////////////////////////////////////////////////////////////

PDPH_HEAP_BLOCK
RtlpDebugPageHeapTakeNodeFromUnusedList(
    IN PDPH_HEAP_ROOT pHeap
    )
{
    PDPH_HEAP_BLOCK pNode = pHeap->pUnusedNodeListHead;
    PDPH_HEAP_BLOCK pPrev = NULL;

    //
    //  UnusedNodeList is LIFO with most recent entry at head of list.
    //

    if (pNode) {

        DEQUEUE_NODE( pNode, pPrev, pHeap->pUnusedNodeListHead, pHeap->pUnusedNodeListTail );

        pHeap->nUnusedNodes -= 1;

    }

    return pNode;
}

VOID
RtlpDebugPageHeapReturnNodeToUnusedList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pNode
    )
{
    //
    //  UnusedNodeList is LIFO with most recent entry at head of list.
    //

    ENQUEUE_HEAD( pNode, pHeap->pUnusedNodeListHead, pHeap->pUnusedNodeListTail );

    pHeap->nUnusedNodes += 1;
}

PDPH_HEAP_BLOCK
RtlpDebugPageHeapFindBusyMem(
    IN  PDPH_HEAP_ROOT        pHeap,
    IN  PVOID                 pUserMem,
    OUT PDPH_HEAP_BLOCK *pPrevAlloc
    )
{
    PDPH_HEAP_BLOCK pNode = pHeap->pBusyAllocationListHead;
    PDPH_HEAP_BLOCK pPrev = NULL;

    while (pNode != NULL) {

        if (pNode->pUserAllocation == pUserMem) {

            if (pPrevAlloc)
                *pPrevAlloc = pPrev;

            return pNode;
        }

        pPrev = pNode;
        pNode = pNode->pNextAlloc;
    }

    return NULL;
}

VOID
RtlpDebugPageHeapRemoveFromAvailableList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pNode,
    IN PDPH_HEAP_BLOCK pPrev
    )
{
    DEQUEUE_NODE( pNode, pPrev, pHeap->pAvailableAllocationListHead, pHeap->pAvailableAllocationListTail );

    pHeap->nAvailableAllocations -= 1;
    pHeap->nAvailableAllocationBytesCommitted -= pNode->nVirtualBlockSize;
}

VOID
RtlpDebugPageHeapPlaceOnFreeList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pAlloc
    )
{
    //
    //  FreeAllocationList is stored FIFO to enhance finding
    //  reference-after-freed bugs by keeping previously freed
    //  allocations on the free list as long as possible.
    //

    pAlloc->pNextAlloc = NULL;

    ENQUEUE_TAIL( pAlloc, pHeap->pFreeAllocationListHead, pHeap->pFreeAllocationListTail );

    pHeap->nFreeAllocations += 1;
    pHeap->nFreeAllocationBytesCommitted += pAlloc->nVirtualBlockSize;
}

VOID
RtlpDebugPageHeapRemoveFromFreeList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pNode,
    IN PDPH_HEAP_BLOCK pPrev
    )
{
    DEQUEUE_NODE( pNode, pPrev, pHeap->pFreeAllocationListHead, pHeap->pFreeAllocationListTail );

    pHeap->nFreeAllocations -= 1;
    pHeap->nFreeAllocationBytesCommitted -= pNode->nVirtualBlockSize;

    pNode->StackTrace = NULL;
}

VOID
RtlpDebugPageHeapPlaceOnVirtualList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pNode
    )
{
    //
    //  VirtualStorageList is LIFO so that releasing VM blocks will
    //  occur in exact reverse order.
    //

    ENQUEUE_HEAD( pNode, pHeap->pVirtualStorageListHead, pHeap->pVirtualStorageListTail );

    pHeap->nVirtualStorageRanges += 1;
    pHeap->nVirtualStorageBytes += pNode->nVirtualBlockSize;
}

VOID
RtlpDebugPageHeapPlaceOnBusyList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pNode
    )
{
    //
    //  BusyAllocationList is LIFO to achieve better temporal locality
    //  of reference (older allocations are farther down the list).
    //

    ENQUEUE_HEAD( pNode, pHeap->pBusyAllocationListHead, pHeap->pBusyAllocationListTail );

    pHeap->nBusyAllocations += 1;
    pHeap->nBusyAllocationBytesCommitted  += pNode->nVirtualBlockSize;
    pHeap->nBusyAllocationBytesAccessible += pNode->nVirtualAccessSize;
}

VOID
RtlpDebugPageHeapRemoveFromBusyList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pNode,
    IN PDPH_HEAP_BLOCK pPrev
    )
{
    DEQUEUE_NODE( pNode, pPrev, pHeap->pBusyAllocationListHead, pHeap->pBusyAllocationListTail );

    pHeap->nBusyAllocations -= 1;
    pHeap->nBusyAllocationBytesCommitted  -= pNode->nVirtualBlockSize;
    pHeap->nBusyAllocationBytesAccessible -= pNode->nVirtualAccessSize;
}

PDPH_HEAP_BLOCK
RtlpDebugPageHeapSearchAvailableMemListForBestFit(
    IN  PDPH_HEAP_ROOT        pHeap,
    IN  SIZE_T                nSize,
    OUT PDPH_HEAP_BLOCK *pPrevAvailNode
    )
{
    PDPH_HEAP_BLOCK pAvail, pFound, pAvailPrev, pFoundPrev;
    SIZE_T nAvail, nFound;

    nFound     = 0x7FFFFFFF;
    pFound     = NULL;
    pFoundPrev = NULL;
    pAvailPrev = NULL;
    pAvail     = pHeap->pAvailableAllocationListHead;

    while (( pAvail != NULL ) && ( nFound > nSize )) {

        nAvail = pAvail->nVirtualBlockSize;

        if (( nAvail >= nSize ) && ( nAvail < nFound )) {
            nFound     = nAvail;
            pFound     = pAvail;
            pFoundPrev = pAvailPrev;
        }

        pAvailPrev = pAvail;
        pAvail     = pAvail->pNextAlloc;
    }

    *pPrevAvailNode = pFoundPrev;
    return pFound;
}

VOID
RtlpDebugPageHeapCoalesceNodeIntoAvailable(
    IN PDPH_HEAP_ROOT pHeap,
    IN PDPH_HEAP_BLOCK pNode
    )
{
    PDPH_HEAP_BLOCK pPrev;
    PDPH_HEAP_BLOCK pNext;
    PUCHAR pVirtual;
    SIZE_T nVirtual;

    pPrev = NULL;
    pNext = pHeap->pAvailableAllocationListHead;

    pVirtual = pNode->pVirtualBlock;
    nVirtual = pNode->nVirtualBlockSize;
    
    pHeap->nAvailableAllocationBytesCommitted += nVirtual;
    pHeap->nAvailableAllocations += 1;

    //
    //  Walk list to insertion point.
    //

    while (( pNext ) && ( pNext->pVirtualBlock < pVirtual )) {
        pPrev = pNext;
        pNext = pNext->pNextAlloc;
    }

    if (pPrev) {

        if (( pPrev->pVirtualBlock + pPrev->nVirtualBlockSize ) == pVirtual) {

            //
            //  pPrev and pNode are adjacent, so simply add size of
            //  pNode entry to pPrev entry.
            //

            pPrev->nVirtualBlockSize += nVirtual;

            RtlpDebugPageHeapReturnNodeToUnusedList( pHeap, pNode );

            pHeap->nAvailableAllocations--;

            pNode    = pPrev;
            pVirtual = pPrev->pVirtualBlock;
            nVirtual = pPrev->nVirtualBlockSize;

        }

        else {

            //
            //  pPrev and pNode are not adjacent, so insert the pNode
            //  block into the list after pPrev.
            //

            pNode->pNextAlloc = pPrev->pNextAlloc;
            pPrev->pNextAlloc = pNode;

        }
    }

    else {

        //
        //  pNode should be inserted at head of list.
        //

        pNode->pNextAlloc = pHeap->pAvailableAllocationListHead;
        pHeap->pAvailableAllocationListHead = pNode;

    }


    if (pNext) {

        if (( pVirtual + nVirtual ) == pNext->pVirtualBlock) {

            //
            //  pNode and pNext are adjacent, so simply add size of
            //  pNext entry to pNode entry and remove pNext entry
            //  from the list.
            //

            pNode->nVirtualBlockSize += pNext->nVirtualBlockSize;

            pNode->pNextAlloc = pNext->pNextAlloc;

            if (pHeap->pAvailableAllocationListTail == pNext) {
                pHeap->pAvailableAllocationListTail = pNode;
            }

            RtlpDebugPageHeapReturnNodeToUnusedList( pHeap, pNext );

            pHeap->nAvailableAllocations--;

        }
    }

    else {

        //
        //  pNode is tail of list.
        //

        pHeap->pAvailableAllocationListTail = pNode;

    }
}

VOID
RtlpDebugPageHeapCoalesceFreeIntoAvailable(
    IN PDPH_HEAP_ROOT pHeap,
    IN ULONG          nLeaveOnFreeList
    )
{
    PDPH_HEAP_BLOCK pNode = pHeap->pFreeAllocationListHead;
    SIZE_T               nFree = pHeap->nFreeAllocations;
    PDPH_HEAP_BLOCK pNext;

    DEBUG_ASSERT( nFree >= nLeaveOnFreeList );

    while (( pNode ) && ( nFree-- > nLeaveOnFreeList )) {

        pNext = pNode->pNextAlloc;  // preserve next pointer across shuffling

        RtlpDebugPageHeapRemoveFromFreeList( pHeap, pNode, NULL );

        RtlpDebugPageHeapCoalesceNodeIntoAvailable( pHeap, pNode );

        pNode = pNext;

    }

    DEBUG_ASSERT(( nFree = (volatile SIZE_T)( pHeap->nFreeAllocations )) >= nLeaveOnFreeList );
    DEBUG_ASSERT(( pNode != NULL ) || ( nFree == 0 ));

}

// forward
BOOLEAN
RtlpDebugPageHeapGrowVirtual(
    IN PDPH_HEAP_ROOT pHeap,
    IN SIZE_T         nSize
    );

PDPH_HEAP_BLOCK
RtlpDebugPageHeapFindAvailableMem(
    IN  PDPH_HEAP_ROOT        pHeap,
    IN  SIZE_T                nSize,
    OUT PDPH_HEAP_BLOCK *pPrevAvailNode,
    IN  BOOLEAN               bGrowVirtual
    )
{
    PDPH_HEAP_BLOCK pAvail;
    ULONG                nLeaveOnFreeList;

    //
    // If we use uncommitted ranges it is really important to
    // call FindAvailableMemory only with page aligned sizes.
    //

    if ((pHeap->ExtraFlags & PAGE_HEAP_SMART_MEMORY_USAGE)) {
        DEBUG_ASSERT ((nSize & ~(PAGE_SIZE - 1)) == nSize);
    }

    //
    //  First search existing AvailableList for a "best-fit" block
    //  (the smallest block that will satisfy the request).
    //

    pAvail = RtlpDebugPageHeapSearchAvailableMemListForBestFit(
        pHeap,
        nSize,
        pPrevAvailNode
        );

    while (( pAvail == NULL ) && ( pHeap->nFreeAllocations > MIN_FREE_LIST_LENGTH )) {

        //
        //  Failed to find sufficient memory on AvailableList.  Coalesce
        //  3/4 of the FreeList memory to the AvailableList and try again.
        //  Continue this until we have sufficient memory in AvailableList,
        //  or the FreeList length is reduced to MIN_FREE_LIST_LENGTH entries.
        //  We don't shrink the FreeList length below MIN_FREE_LIST_LENGTH
        //  entries to preserve the most recent MIN_FREE_LIST_LENGTH entries
        //  for reference-after-freed purposes.
        //

        nLeaveOnFreeList = pHeap->nFreeAllocations / 4;

        if (nLeaveOnFreeList < MIN_FREE_LIST_LENGTH)
            nLeaveOnFreeList = MIN_FREE_LIST_LENGTH;

        RtlpDebugPageHeapCoalesceFreeIntoAvailable( pHeap, nLeaveOnFreeList );

        pAvail = RtlpDebugPageHeapSearchAvailableMemListForBestFit(
            pHeap,
            nSize,
            pPrevAvailNode
            );

    }


    if (( pAvail == NULL ) && ( bGrowVirtual )) {

        //
        //  After coalescing FreeList into AvailableList, still don't have
        //  enough memory (large enough block) to satisfy request, so we
        //  need to allocate more VM.
        //

        if (RtlpDebugPageHeapGrowVirtual( pHeap, nSize )) {

            pAvail = RtlpDebugPageHeapSearchAvailableMemListForBestFit(
                pHeap,
                nSize,
                pPrevAvailNode
                );

            if (pAvail == NULL) {

                //
                //  Failed to satisfy request with more VM.  If remainder
                //  of free list combined with available list is larger
                //  than the request, we might still be able to satisfy
                //  the request by merging all of the free list onto the
                //  available list.  Note we lose our MIN_FREE_LIST_LENGTH
                //  reference-after-freed insurance in this case, but it
                //  is a rare case, and we'd prefer to satisfy the allocation.
                //

                if (( pHeap->nFreeAllocationBytesCommitted +
                    pHeap->nAvailableAllocationBytesCommitted ) >= nSize) {

                    RtlpDebugPageHeapCoalesceFreeIntoAvailable( pHeap, 0 );

                    pAvail = RtlpDebugPageHeapSearchAvailableMemListForBestFit(
                        pHeap,
                        nSize,
                        pPrevAvailNode
                        );
                }
            }
        }
    }

    //
    // If we use uncommitted ranges we need to commit the memory
    // range now. Note that the memory will be committed but
    // the protection on it will be N/A. 
    //

    if (pAvail && (pHeap->ExtraFlags & PAGE_HEAP_SMART_MEMORY_USAGE)) {

        BOOLEAN Success;

        //
        // (SilviuC): The memory here might be already committed if we use
        // it for the first time. Whenever we allocate virtual memory to grow
        // the heap we commit it. This is the reason the consumption does not 
        // decrease as spectacular as we expected. We need to fix it.
        // It affects 0x43 flags.
        //

        Success = RtlpDebugPageHeapCommitVM (pAvail->pVirtualBlock, nSize);

        if (!Success) {

            //
            // We did not manage to commit memory for this block. This
            // can happen in low memory conditions. We need to return
            // the node back into free pool (together with virtual space
            // region taken) and fail the current call.
            //

            RtlpDebugPageHeapPlaceOnFreeList( pHeap, pAvail );

            return NULL;
        }
    }

    return pAvail;
}

VOID
RtlpDebugPageHeapPlaceOnPoolList(
    IN PDPH_HEAP_ROOT       pHeap,
    IN PDPH_HEAP_BLOCK pNode
    )
{

    //
    //  NodePoolList is FIFO.
    //

    pNode->pNextAlloc = NULL;

    ENQUEUE_TAIL( pNode, pHeap->pNodePoolListHead, pHeap->pNodePoolListTail );

    pHeap->nNodePoolBytes += pNode->nVirtualBlockSize;
    pHeap->nNodePools     += 1;

}

VOID
RtlpDebugPageHeapAddNewPool(
    IN PDPH_HEAP_ROOT pHeap,
    IN PVOID          pVirtual,
    IN SIZE_T         nSize,
    IN BOOLEAN        bAddToPoolList
    )
{
    PDPH_HEAP_BLOCK pNode, pFirst;
    ULONG n, nCount;

    //
    //  Assume pVirtual points to committed block of nSize bytes.
    //

    pFirst = pVirtual;
    nCount = (ULONG)(nSize  / sizeof( DPH_HEAP_BLOCK ));

    for (n = nCount - 1, pNode = pFirst; n > 0; pNode++, n--)
        pNode->pNextAlloc = pNode + 1;

    pNode->pNextAlloc = NULL;

    //
    //  Now link this list into the tail of the UnusedNodeList
    //

    ENQUEUE_TAIL( pFirst, pHeap->pUnusedNodeListHead, pHeap->pUnusedNodeListTail );

    pHeap->pUnusedNodeListTail = pNode;

    pHeap->nUnusedNodes += nCount;

    if (bAddToPoolList) {

        //
        //  Now add an entry on the PoolList by taking a node from the
        //  UnusedNodeList, which should be guaranteed to be non-empty
        //  since we just added new nodes to it.
        //

        pNode = RtlpDebugPageHeapTakeNodeFromUnusedList( pHeap );

        DEBUG_ASSERT( pNode != NULL );

        pNode->pVirtualBlock     = pVirtual;
        pNode->nVirtualBlockSize = nSize;

        RtlpDebugPageHeapPlaceOnPoolList( pHeap, pNode );

    }
}

PDPH_HEAP_BLOCK
RtlpDebugPageHeapAllocateNode(
    IN PDPH_HEAP_ROOT pHeap
    )
{
    PDPH_HEAP_BLOCK pNode, pPrev, pReturn;
    PUCHAR pVirtual;
    SIZE_T nVirtual;
    SIZE_T nRequest;

    DEBUG_ASSERT( ! pHeap->InsideAllocateNode );
    DEBUG_CODE( pHeap->InsideAllocateNode = TRUE );

    pReturn = NULL;

    if (pHeap->pUnusedNodeListHead == NULL) {

        //
        //  We're out of nodes -- allocate new node pool
        //  from AvailableList.  Set bGrowVirtual to FALSE
        //  since growing virtual will require new nodes, causing
        //  recursion.  Note that simply calling FindAvailableMem
        //  might return some nodes to the pUnusedNodeList, even if
        //  the call fails, so we'll check that the UnusedNodeList
        //  is still empty before we try to use or allocate more
        //  memory.
        //

        nRequest = POOL_SIZE;

        pNode = RtlpDebugPageHeapFindAvailableMem(
            pHeap,
            nRequest,
            &pPrev,
            FALSE
            );

        if (( pHeap->pUnusedNodeListHead == NULL ) && ( pNode == NULL )) {

            //
            //  Reduce request size to PAGE_SIZE and see if
            //  we can find at least a page on the available
            //  list.
            //

            nRequest = PAGE_SIZE;

            pNode = RtlpDebugPageHeapFindAvailableMem(
                pHeap,
                nRequest,
                &pPrev,
                FALSE
                );

        }

        if (pHeap->pUnusedNodeListHead == NULL) {

            if (pNode == NULL) {

                //
                //  Insufficient memory on Available list.  Try allocating a
                //  new virtual block.
                //

                nRequest = POOL_SIZE;
                nVirtual = RESERVE_SIZE;
                pVirtual = RtlpDebugPageHeapAllocateVM( nVirtual );

                if (pVirtual == NULL) {

                    //
                    //  Unable to allocate full RESERVE_SIZE block,
                    //  so reduce request to single VM unit (64K)
                    //  and try again.
                    //

                    nVirtual = VM_UNIT_SIZE;
                    pVirtual = RtlpDebugPageHeapAllocateVM( nVirtual );

                    if (pVirtual == NULL) {

                        //
                        //  Can't allocate any VM.
                        //

                        goto EXIT;
                    }
                }
            }

            else {

                RtlpDebugPageHeapRemoveFromAvailableList( pHeap, pNode, pPrev );

                pVirtual = pNode->pVirtualBlock;
                nVirtual = pNode->nVirtualBlockSize;

            }

            //
            //  We now have allocated VM referenced by pVirtual,nVirtual.
            //  Make nRequest portion of VM accessible for new node pool.
            //

            if (! RtlpDebugPageHeapProtectVM( pVirtual, nRequest, PAGE_READWRITE )) {

                if (pNode == NULL) {
                    RtlpDebugPageHeapReleaseVM( pVirtual );
                }
                else {
                    RtlpDebugPageHeapCoalesceNodeIntoAvailable( pHeap, pNode );
                }

                goto EXIT;
            }

            //
            //  Now we have accessible memory for new pool.  Add the
            //  new memory to the pool.  If the new memory came from
            //  AvailableList versus fresh VM, zero the memory first.
            //

            if (pNode != NULL) {
                RtlZeroMemory( pVirtual, nRequest );
            }

            RtlpDebugPageHeapAddNewPool( pHeap, pVirtual, nRequest, TRUE );

            //
            //  If any memory remaining, put it on available list.
            //

            if (pNode == NULL) {

                //
                //  Memory came from new VM -- add appropriate list entries
                //  for new VM and add remainder of VM to free list.
                //

                pNode = RtlpDebugPageHeapTakeNodeFromUnusedList( pHeap );
                DEBUG_ASSERT( pNode != NULL );
                pNode->pVirtualBlock     = pVirtual;
                pNode->nVirtualBlockSize = nVirtual;
                RtlpDebugPageHeapPlaceOnVirtualList( pHeap, pNode );

                pNode = RtlpDebugPageHeapTakeNodeFromUnusedList( pHeap );
                DEBUG_ASSERT( pNode != NULL );
                pNode->pVirtualBlock     = pVirtual + nRequest;
                pNode->nVirtualBlockSize = nVirtual - nRequest;

                RtlpDebugPageHeapCoalesceNodeIntoAvailable( pHeap, pNode );

            }

            else {

                if (pNode->nVirtualBlockSize > nRequest) {

                    pNode->pVirtualBlock     += nRequest;
                    pNode->nVirtualBlockSize -= nRequest;

                    RtlpDebugPageHeapCoalesceNodeIntoAvailable( pHeap, pNode );
                }

                else {

                    //
                    //  Used up entire available block -- return node to
                    //  unused list.
                    //

                    RtlpDebugPageHeapReturnNodeToUnusedList( pHeap, pNode );

                }
            }
        }
    }

    pReturn = RtlpDebugPageHeapTakeNodeFromUnusedList( pHeap );
    DEBUG_ASSERT( pReturn != NULL );

    EXIT:

    DEBUG_CODE( pHeap->InsideAllocateNode = FALSE );
    return pReturn;
}

BOOLEAN
RtlpDebugPageHeapGrowVirtual(
    IN PDPH_HEAP_ROOT pHeap,
    IN SIZE_T         nSize
    )
{
    PDPH_HEAP_BLOCK pVirtualNode;
    PDPH_HEAP_BLOCK pAvailNode;
    PVOID  pVirtual;
    SIZE_T nVirtual;

    pVirtualNode = RtlpDebugPageHeapAllocateNode( pHeap );

    if (pVirtualNode == NULL) {
        return FALSE;
    }

    pAvailNode = RtlpDebugPageHeapAllocateNode( pHeap );

    if (pAvailNode == NULL) {
        RtlpDebugPageHeapReturnNodeToUnusedList( pHeap, pVirtualNode );
        return FALSE;
    }

    nSize    = ROUNDUP2( nSize, VM_UNIT_SIZE );
    nVirtual = ( nSize > RESERVE_SIZE ) ? nSize : RESERVE_SIZE;
    pVirtual = RtlpDebugPageHeapAllocateVM( nVirtual );

    if (( pVirtual == NULL ) && ( nSize < RESERVE_SIZE )) {
        nVirtual = nSize;
        pVirtual = RtlpDebugPageHeapAllocateVM( nVirtual );
    }

    if (pVirtual == NULL) {
        RtlpDebugPageHeapReturnNodeToUnusedList( pHeap, pVirtualNode );
        RtlpDebugPageHeapReturnNodeToUnusedList( pHeap, pAvailNode );
        return FALSE;
    }

    pVirtualNode->pVirtualBlock     = pVirtual;
    pVirtualNode->nVirtualBlockSize = nVirtual;
    RtlpDebugPageHeapPlaceOnVirtualList( pHeap, pVirtualNode );

    pAvailNode->pVirtualBlock     = pVirtual;
    pAvailNode->nVirtualBlockSize = nVirtual;
    RtlpDebugPageHeapCoalesceNodeIntoAvailable( pHeap, pAvailNode );

    return TRUE;
}

VOID
RtlpDebugPageHeapProtectStructures(
    IN PDPH_HEAP_ROOT pHeap
    )
{
    PDPH_HEAP_BLOCK pNode;

    //
    //  Assume CritSect is owned so we're the only thread twiddling
    //  the protection.
    //

    DEBUG_ASSERT( pHeap->HeapFlags & HEAP_PROTECTION_ENABLED );

    if (--pHeap->nUnProtectionReferenceCount == 0) {

        pNode = pHeap->pNodePoolListHead;

        while (pNode != NULL) {

            RtlpDebugPageHeapProtectVM( pNode->pVirtualBlock,
                pNode->nVirtualBlockSize,
                PAGE_READONLY );

            pNode = pNode->pNextAlloc;

        }
    }
}

VOID
RtlpDebugPageHeapUnProtectStructures(
    IN PDPH_HEAP_ROOT pHeap
    )
{
    PDPH_HEAP_BLOCK pNode;

    DEBUG_ASSERT( pHeap->HeapFlags & HEAP_PROTECTION_ENABLED );

    if (pHeap->nUnProtectionReferenceCount == 0) {

        pNode = pHeap->pNodePoolListHead;

        while (pNode != NULL) {

            RtlpDebugPageHeapProtectVM( pNode->pVirtualBlock,
                pNode->nVirtualBlockSize,
                PAGE_READWRITE );

            pNode = pNode->pNextAlloc;

        }
    }

    pHeap->nUnProtectionReferenceCount += 1;
}

/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Internal debug functions
/////////////////////////////////////////////////////////////////////

#if INTERNAL_DEBUG

VOID
RtlpDebugPageHeapVerifyList(
    IN PDPH_HEAP_BLOCK pListHead,
    IN PDPH_HEAP_BLOCK pListTail,
    IN SIZE_T               nExpectedLength,
    IN SIZE_T               nExpectedVirtual,
    IN PCCH                 pListName
    )
{
    PDPH_HEAP_BLOCK pPrev = NULL;
    PDPH_HEAP_BLOCK pNode = pListHead;
    PDPH_HEAP_BLOCK pTest = pListHead ? pListHead->pNextAlloc : NULL;
    ULONG                nNode = 0;
    SIZE_T               nSize = 0;

    while (pNode) {

        if (pNode == pTest) {
            DbgPrint( "Page heap: Internal %s list is circular\n", pListName );
            RtlpDebugPageHeapBreak( "" );
            return;
        }

        nNode += 1;
        nSize += pNode->nVirtualBlockSize;

        if (pTest) {
            pTest = pTest->pNextAlloc;
            if (pTest) {
                pTest = pTest->pNextAlloc;
            }
        }

        pPrev = pNode;
        pNode = pNode->pNextAlloc;

    }

    if (pPrev != pListTail) {
        DbgPrint( "Page heap: Internal %s list has incorrect tail pointer\n", pListName );
        RtlpDebugPageHeapBreak( "" );
    }

    if (( nExpectedLength != 0xFFFFFFFF ) && ( nExpectedLength != nNode )) {
        DbgPrint( "Page heap: Internal %s list has incorrect length\n", pListName );
        RtlpDebugPageHeapBreak( "" );
    }

    if (( nExpectedVirtual != 0xFFFFFFFF ) && ( nExpectedVirtual != nSize )) {
        DbgPrint( "Page heap: Internal %s list has incorrect virtual size\n", pListName );
        RtlpDebugPageHeapBreak( "" );
    }

}

VOID
RtlpDebugPageHeapVerifyIntegrity(
    IN PDPH_HEAP_ROOT pHeap
    )
{

    RtlpDebugPageHeapVerifyList(
        pHeap->pVirtualStorageListHead,
        pHeap->pVirtualStorageListTail,
        pHeap->nVirtualStorageRanges,
        pHeap->nVirtualStorageBytes,
        "VIRTUAL"
        );

    RtlpDebugPageHeapVerifyList(
        pHeap->pBusyAllocationListHead,
        pHeap->pBusyAllocationListTail,
        pHeap->nBusyAllocations,
        pHeap->nBusyAllocationBytesCommitted,
        "BUSY"
        );

    RtlpDebugPageHeapVerifyList(
        pHeap->pFreeAllocationListHead,
        pHeap->pFreeAllocationListTail,
        pHeap->nFreeAllocations,
        pHeap->nFreeAllocationBytesCommitted,
        "FREE"
        );

    RtlpDebugPageHeapVerifyList(
        pHeap->pAvailableAllocationListHead,
        pHeap->pAvailableAllocationListTail,
        pHeap->nAvailableAllocations,
        pHeap->nAvailableAllocationBytesCommitted,
        "AVAILABLE"
        );

    RtlpDebugPageHeapVerifyList(
        pHeap->pUnusedNodeListHead,
        pHeap->pUnusedNodeListTail,
        pHeap->nUnusedNodes,
        0xFFFFFFFF,
        "FREENODE"
        );

    RtlpDebugPageHeapVerifyList(
        pHeap->pNodePoolListHead,
        pHeap->pNodePoolListTail,
        pHeap->nNodePools,
        pHeap->nNodePoolBytes,
        "NODEPOOL"
        );
}

#endif // #if INTERNAL_DEBUG

/////////////////////////////////////////////////////////////////////
///////////////////////////// Exported page heap management functions
/////////////////////////////////////////////////////////////////////

//
//  Here's where the exported interface functions are defined.
//

//silviuc: i think this pragma works only for the next function defined
#if (( DPH_CAPTURE_STACK_TRACE ) && ( i386 ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

PVOID
RtlpDebugPageHeapCreate(
    IN ULONG  Flags,
    IN PVOID  HeapBase    OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize  OPTIONAL,
    IN PVOID  Lock        OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    )
{
    SYSTEM_BASIC_INFORMATION SystemInfo;
    PDPH_HEAP_BLOCK     Node;
    PDPH_HEAP_ROOT           HeapRoot;
    PVOID                    HeapHandle;
    PUCHAR                   pVirtual;
    SIZE_T                   nVirtual;
    SIZE_T                   Size;
    NTSTATUS                 Status;

    //
    // If `Parameters' is -1 then this is a recursive call to
    // RtlpDebugPageHeapCreate and we will return NULL so that
    // the normal heap manager will create a normal heap.
    // I agree this is a hack but we need this so that we maintain
    // a very loose dependency between the normal and page heap
    // manager.
    //

    if ((SIZE_T)Parameters == (SIZE_T)-1) {
        return NULL;                                        
    }

    //
    //  We don't handle heaps where HeapBase is already allocated
    //  from user or where Lock is provided by user.
    //

    DEBUG_ASSERT( HeapBase == NULL );
    DEBUG_ASSERT( Lock == NULL );

    if (( HeapBase != NULL ) || ( Lock != NULL ))
        return NULL;

    //
    //  Note that we simply ignore ReserveSize, CommitSize, and
    //  Parameters as we always have a growable heap with our
    //  own thresholds, etc.
    //

    ZwQuerySystemInformation( SystemBasicInformation,
        &SystemInfo,
        sizeof( SystemInfo ),
        NULL );

    RETAIL_ASSERT( SystemInfo.PageSize == PAGE_SIZE );
    RETAIL_ASSERT( SystemInfo.AllocationGranularity == VM_UNIT_SIZE );
    DEBUG_ASSERT(( PAGE_SIZE + POOL_SIZE + PAGE_SIZE ) < VM_UNIT_SIZE );

    nVirtual = RESERVE_SIZE;
    pVirtual = RtlpDebugPageHeapAllocateVM( nVirtual );

    if (pVirtual == NULL) {

        nVirtual = VM_UNIT_SIZE;
        pVirtual = RtlpDebugPageHeapAllocateVM( nVirtual );

        if (pVirtual == NULL) {
            OUT_OF_VM_BREAK( Flags, "Page heap: Insufficient memory to create heap\n" );
            IF_GENERATE_EXCEPTION( Flags, STATUS_NO_MEMORY );
            return NULL;
        }
    }

    if (! RtlpDebugPageHeapProtectVM( pVirtual, PAGE_SIZE + POOL_SIZE + PAGE_SIZE, PAGE_READWRITE )) {
        RtlpDebugPageHeapReleaseVM( pVirtual );
        IF_GENERATE_EXCEPTION( Flags, STATUS_NO_MEMORY );
        return NULL;
    }

    //
    //  Out of our initial allocation, the initial page is the fake
    //  retail HEAP structure.  The second page begins our DPH_HEAP
    //  structure followed by (POOL_SIZE-sizeof(DPH_HEAP)) bytes for
    //  the initial pool.  The next page contains out CRIT_SECT
    //  variable, which must always be READWRITE.  Beyond that, the
    //  remainder of the virtual allocation is placed on the available
    //  list.
    //
    //  |_____|___________________|_____|__ _ _ _ _ _ _ _ _ _ _ _ _ __|
    //
    //  ^pVirtual
    //
    //  ^FakeRetailHEAP
    //
    //        ^HeapRoot
    //
    //            ^InitialNodePool
    //
    //                            ^CRITICAL_SECTION
    //
    //                                  ^AvailableSpace
    //
    //
    //
    //  Our DPH_HEAP structure starts at the page following the
    //  fake retail HEAP structure pointed to by the "heap handle".
    //  For the fake HEAP structure, we'll fill it with 0xEEEEEEEE
    //  except for the Heap->Flags and Heap->ForceFlags fields,
    //  which we must set to include our HEAP_FLAG_PAGE_ALLOCS flag,
    //  and then we'll make the whole page read-only.
    //

    RtlFillMemory( pVirtual, PAGE_SIZE, FILL_BYTE );

    ((PHEAP)pVirtual)->Flags      = Flags | HEAP_FLAG_PAGE_ALLOCS;
    ((PHEAP)pVirtual)->ForceFlags = Flags | HEAP_FLAG_PAGE_ALLOCS;

    if (! RtlpDebugPageHeapProtectVM( pVirtual, PAGE_SIZE, PAGE_READONLY )) {
        RtlpDebugPageHeapReleaseVM( pVirtual );
        IF_GENERATE_EXCEPTION( Flags, STATUS_NO_MEMORY );
        return NULL;
    }

    HeapRoot = (PDPH_HEAP_ROOT)( pVirtual + PAGE_SIZE );

    HeapRoot->Signature    = DPH_HEAP_SIGNATURE;
    HeapRoot->HeapFlags    = Flags;
    HeapRoot->HeapCritSect = (PVOID)((PCHAR)HeapRoot + POOL_SIZE );

    //
    // Copy the page heap global flags into per heap flags.
    //

    HeapRoot->ExtraFlags = RtlpDphGlobalFlags;

    //
    // If the PAGE_HEAP_UNALIGNED_ALLOCATIONS bit is set
    // in ExtraFlags we will set the HEAP_NO_ALIGNMENT flag
    // in the HeapFlags. This last bit controls if allocations
    // will be aligned or not. The reason we do this transfer is
    // that ExtraFlags can be set from the registry whereas the
    // normal HeapFlags cannot.
    //

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_UNALIGNED_ALLOCATIONS)) {
        HeapRoot->HeapFlags |= HEAP_NO_ALIGNMENT;
    }

    //
    // Initialize the seed for the random generator used to decide
    // from where should we make allocations if random decision
    // flag is on.
    //

    {
        LARGE_INTEGER PerformanceCounter;

        PerformanceCounter.LowPart = 0xABCDDCBA;

        NtQueryPerformanceCounter (
            &PerformanceCounter,
            NULL);

        HeapRoot->Seed = PerformanceCounter.LowPart;
    }

    RtlZeroMemory (HeapRoot->Counter, sizeof(HeapRoot->Counter));

    //
    // Create the normal heap associated with the page heap.
    // The last parameter value (-1) is very important because
    // it stops the recursive call into page heap create.
    //

    HeapRoot->NormalHeap = RtlCreateHeap(

        Flags,
        HeapBase,
        ReserveSize,
        CommitSize,
        Lock,
        (PRTL_HEAP_PARAMETERS)-1 );

    //
    // Initialize heap lock.
    //

    RtlInitializeCriticalSection( HeapRoot->HeapCritSect );

    //
    //  On the page that contains our DPH_HEAP structure, use
    //  the remaining memory beyond the DPH_HEAP structure as
    //  pool for allocating heap nodes.
    //

    RtlpDebugPageHeapAddNewPool( HeapRoot,
        HeapRoot + 1,
        POOL_SIZE - sizeof( DPH_HEAP_ROOT ),
        FALSE
        );

    //
    //  Make initial PoolList entry by taking a node from the
    //  UnusedNodeList, which should be guaranteed to be non-empty
    //  since we just added new nodes to it.
    //

    Node = RtlpDebugPageHeapAllocateNode( HeapRoot );
    DEBUG_ASSERT( Node != NULL );
    Node->pVirtualBlock     = (PVOID)HeapRoot;
    Node->nVirtualBlockSize = POOL_SIZE;
    RtlpDebugPageHeapPlaceOnPoolList( HeapRoot, Node );

    //
    //  Make VirtualStorageList entry for initial VM allocation
    //

    Node = RtlpDebugPageHeapAllocateNode( HeapRoot );
    DEBUG_ASSERT( Node != NULL );
    Node->pVirtualBlock     = pVirtual;
    Node->nVirtualBlockSize = nVirtual;
    RtlpDebugPageHeapPlaceOnVirtualList( HeapRoot, Node );

    //
    //  Make AvailableList entry containing remainder of initial VM
    //  and add to (create) the AvailableList.
    //

    Node = RtlpDebugPageHeapAllocateNode( HeapRoot );
    DEBUG_ASSERT( Node != NULL );
    Node->pVirtualBlock     = pVirtual + ( PAGE_SIZE + POOL_SIZE + PAGE_SIZE );
    Node->nVirtualBlockSize = nVirtual - ( PAGE_SIZE + POOL_SIZE + PAGE_SIZE );
    RtlpDebugPageHeapCoalesceNodeIntoAvailable( HeapRoot, Node );

    //
    // Get heap creation stack trace.
    //

    HeapRoot->CreateStackTrace = RtlpDphLogStackTrace(1);

    //
    //  Initialize heap internal structure protection.
    //

    HeapRoot->nUnProtectionReferenceCount = 1;          // initialize

    //
    //  If this is the first heap creation in this process, then we
    //  need to initialize the process heap list critical section,
    // the global delayed free queue for normal blocks and the
    // trace database.
    //

    if (! RtlpDphHeapListHasBeenInitialized) {

        RtlpDphHeapListHasBeenInitialized = TRUE;

        RtlInitializeCriticalSection( &RtlpDphHeapListCriticalSection );
        RtlpDphInitializeDelayedFreeQueue ();

        //
        // Do not make fuss if the trace database creation fails.
        // This is something we can live with.
        //
        // The number of buckets is chosen to be a prime not too
        // close to a power of two (Knuth says so). Three possible
        // values are: 1567, 3089, 6263.
        //

        RtlpDphTraceDatabase = RtlTraceDatabaseCreate (
            6263, 
            RtlpDphTraceDatabaseMaximumSize,
            0,
            0,
            NULL);

#if DBG
        if (RtlpDphTraceDatabase == NULL) {
            DbgPrint ("Page heap: warning: failed to create trace database for %p",
                HeapRoot);
        }
#endif
        //
        // Create the Unicode string containing the target dlls. 
        // If no target dlls have been specified the string will
        // be initialized with the empty string.
        //

        RtlInitUnicodeString (
            &RtlpDphTargetDllsUnicode,
            RtlpDphTargetDlls);
        
        //
        // Initialize the target dlls logic
        //

        RtlpDphTargetDllsLogicInitialize ();
    }

    //
    //  Add this heap entry to the process heap linked list.
    //

    RtlEnterCriticalSection( &RtlpDphHeapListCriticalSection );

    if (RtlpDphHeapListHead == NULL) {
        RtlpDphHeapListHead = HeapRoot;
        RtlpDphHeapListTail = HeapRoot;
    }
    else {
        HeapRoot->pPrevHeapRoot = RtlpDphHeapListTail;
        UNPROTECT_HEAP_STRUCTURES(RtlpDphHeapListTail);
        RtlpDphHeapListTail->pNextHeapRoot = HeapRoot;
        PROTECT_HEAP_STRUCTURES(RtlpDphHeapListTail);
        RtlpDphHeapListTail                = HeapRoot;
    }

    PROTECT_HEAP_STRUCTURES( HeapRoot );                // now protected

    RtlpDphHeapListCount += 1;

    RtlLeaveCriticalSection( &RtlpDphHeapListCriticalSection );

    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));

    DbgPrint( "Page heap: process 0x%X created heap @ %p (%p, flags 0x%X)\n",
        NtCurrentTeb()->ClientId.UniqueProcess,
        HEAP_HANDLE_FROM_ROOT( HeapRoot ),
        HeapRoot->NormalHeap,
        HeapRoot->ExtraFlags);

    if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION)) {
        RtlpDphInternalValidatePageHeap (HeapRoot, NULL, 0);
    }

    return HEAP_HANDLE_FROM_ROOT( HeapRoot );       // same as pVirtual

}

PVOID
RtlpDebugPageHeapAllocate(
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T Size
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK pAvailNode;
    PDPH_HEAP_BLOCK pPrevAvailNode;
    PDPH_HEAP_BLOCK pBusyNode;
    SIZE_T               nBytesAllocate;
    SIZE_T               nBytesAccess;
    SIZE_T               nActual;
    PVOID                pVirtual;
    PVOID                pReturn;
    PUCHAR               pBlockHeader;
    ULONG Reason;
    BOOLEAN ForcePageHeap = FALSE;

    if (IS_BIASED_POINTER(HeapHandle)) {
        HeapHandle = UNBIAS_POINTER(HeapHandle);
        ForcePageHeap = TRUE;
    }

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if (HeapRoot == NULL)
        return NULL;

    //
    // Is zero size allocation ?
    //

    if (Size == 0) {

        if ((RtlpDphDebugLevel & DPH_DEBUG_BREAK_FOR_SIZE_ZERO)) {

            DbgPrint ("Page heap: request for zero sized block \n");
            DbgBreakPoint();
        }
    }

    //
    // Get the heap lock.
    //

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    //
    // We cannot validate the heap when a forced allocation into page heap
    // is requested due to accounting problems. Allocate is called in this way
    // from ReAllocate while the old node (just about to be freed) is in limbo
    // and is not accounted in any internal structure.
    //

    if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION) && !ForcePageHeap) {
        RtlpDphInternalValidatePageHeap (HeapRoot, NULL, 0);
    }

    Flags |= HeapRoot->HeapFlags;

    //
    // Compute alloc statistics. Note that we need to
    // take the heap lock for this and unprotect the
    // heap structures.
    //

    BUMP_GLOBAL_COUNTER (DPH_COUNTER_NO_OF_ALLOCS);
    BUMP_SIZE_COUNTER (Size);

    HeapRoot->Counter[DPH_COUNTER_NO_OF_ALLOCS] += 1;

    if (Size < 1024) {
        BUMP_GLOBAL_COUNTER (DPH_COUNTER_SIZE_BELOW_1K);
        HeapRoot->Counter[DPH_COUNTER_SIZE_BELOW_1K] += 1;
    }
    else if (Size < 4096) {
        BUMP_GLOBAL_COUNTER (DPH_COUNTER_SIZE_BELOW_4K);
        HeapRoot->Counter[DPH_COUNTER_SIZE_BELOW_4K] += 1;
    }
    else {
        BUMP_GLOBAL_COUNTER (DPH_COUNTER_SIZE_ABOVE_4K);
        HeapRoot->Counter[DPH_COUNTER_SIZE_ABOVE_4K] += 1;
    }

    //
    // Figure out if we need to minimize memory impact. This
    // might trigger an allocation in the normal heap.
    //

    if (! ForcePageHeap) {
        
        if (! (RtlpDphShouldAllocateInPageHeap (HeapRoot, Size))) {

            pReturn = RtlpDphNormalHeapAllocate (
                HeapRoot,
                Flags,
                Size); 

            goto EXIT;
        }
    }

    //
    // Check the heap a little bit on checked builds.
    //

    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));

    pReturn = NULL;

    //
    //  Validate requested size so we don't overflow
    //  while rounding up size computations.  We do this
    //  after we've acquired the critsect so we can still
    //  catch serialization problems.
    //

    if (Size > 0x7FFF0000) {
        OUT_OF_VM_BREAK( Flags, "Page heap: Invalid allocation size\n" );
        goto EXIT;
    }

    //
    //  Determine number of pages needed for READWRITE portion
    //  of allocation and add an extra page for the NO_ACCESS
    //  memory beyond the READWRITE page(s).
    //

    nBytesAccess  = ROUNDUP2( Size + sizeof(DPH_BLOCK_INFORMATION), PAGE_SIZE );
    nBytesAllocate = nBytesAccess + PAGE_SIZE;

    //
    //  RtlpDebugPageHeapFindAvailableMem will first attempt to satisfy
    //  the request from memory on the Available list.  If that fails,
    //  it will coalesce some of the Free list memory into the Available
    //  list and try again.  If that still fails, new VM is allocated and
    //  added to the Available list.  If that fails, the function will
    //  finally give up and return NULL.
    //

    pAvailNode = RtlpDebugPageHeapFindAvailableMem(
        HeapRoot,
        nBytesAllocate,
        &pPrevAvailNode,
        TRUE
        );

    if (pAvailNode == NULL) {
        OUT_OF_VM_BREAK( Flags, "Page heap: Unable to allocate virtual memory\n" );
        goto EXIT;
    }

    //
    //  Now can't call AllocateNode until pAvailNode is
    //  adjusted and/or removed from Avail list since AllocateNode
    //  might adjust the Avail list.
    //

    pVirtual = pAvailNode->pVirtualBlock;

    if (nBytesAccess > 0) {

        if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

            if (! RtlpDebugPageHeapProtectVM( (PUCHAR)pVirtual + PAGE_SIZE, nBytesAccess, PAGE_READWRITE )) {
                goto EXIT;
            }
        }
        else {

            if (! RtlpDebugPageHeapProtectVM( pVirtual, nBytesAccess, PAGE_READWRITE )) {
                goto EXIT;
            }
        }
    }

    //
    // If we use uncommitted ranges we need to decommit the protection
    // page at the end. BAckward overruns flag disables smart memory 
    // usage flag.
    //

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

        // nothing            

    }
    else {

        if ((HeapRoot->ExtraFlags & PAGE_HEAP_SMART_MEMORY_USAGE)) {

            RtlpDebugPageHeapDecommitVM (
                (PCHAR)pVirtual + nBytesAccess, 
                PAGE_SIZE);
        }
    }

    //
    //  pAvailNode (still on avail list) points to block large enough
    //  to satisfy request, but it might be large enough to split
    //  into two blocks -- one for request, remainder leave on
    //  avail list.
    //

    if (pAvailNode->nVirtualBlockSize > nBytesAllocate) {

        //
        //  Adjust pVirtualBlock and nVirtualBlock size of existing
        //  node in avail list.  The node will still be in correct
        //  address space order on the avail list.  This saves having
        //  to remove and then re-add node to avail list.  Note since
        //  we're changing sizes directly, we need to adjust the
        //  avail and busy list counters manually.
        //
        //  Note: since we're leaving at least one page on the
        //  available list, we are guaranteed that AllocateNode
        //  will not fail.
        //

        pAvailNode->pVirtualBlock                    += nBytesAllocate;
        pAvailNode->nVirtualBlockSize                -= nBytesAllocate;
        HeapRoot->nAvailableAllocationBytesCommitted -= nBytesAllocate;

        pBusyNode = RtlpDebugPageHeapAllocateNode( HeapRoot );

        DEBUG_ASSERT( pBusyNode != NULL );

        pBusyNode->pVirtualBlock     = pVirtual;
        pBusyNode->nVirtualBlockSize = nBytesAllocate;

    }

    else {

        //
        //  Entire avail block is needed, so simply remove it from avail list.
        //

        RtlpDebugPageHeapRemoveFromAvailableList( HeapRoot, pAvailNode, pPrevAvailNode );

        pBusyNode = pAvailNode;

    }

    //
    //  Now pBusyNode points to our committed virtual block.
    //

    if (HeapRoot->HeapFlags & HEAP_NO_ALIGNMENT)
        nActual = Size;
    else
        nActual = ROUNDUP2( Size, USER_ALIGNMENT );

    pBusyNode->nVirtualAccessSize = nBytesAccess;
    pBusyNode->nUserRequestedSize = Size;
    pBusyNode->nUserActualSize    = nActual;

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

        pBusyNode->pUserAllocation    = pBusyNode->pVirtualBlock
            + PAGE_SIZE;
    }
    else {

        pBusyNode->pUserAllocation    = pBusyNode->pVirtualBlock
            + pBusyNode->nVirtualAccessSize
            - nActual;
    }

    pBusyNode->UserValue          = NULL;
    pBusyNode->UserFlags          = Flags & HEAP_SETTABLE_USER_FLAGS;

    //
    //  RtlpDebugPageHeapAllocate gets called from RtlDebugAllocateHeap,
    //  which gets called from RtlAllocateHeapSlowly, which gets called
    //  from RtlAllocateHeap.  To keep from wasting lots of stack trace
    //  storage, we'll skip the bottom 3 entries, leaving RtlAllocateHeap
    //  as the first recorded entry.
    //

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_COLLECT_STACK_TRACES)) {

        pBusyNode->StackTrace = RtlpDphLogStackTrace(3);

        if (pBusyNode->StackTrace) {

            RtlTraceDatabaseLock (RtlpDphTraceDatabase);
            pBusyNode->StackTrace->UserCount += 1;
            pBusyNode->StackTrace->UserSize += pBusyNode->nUserRequestedSize;
            pBusyNode->StackTrace->UserContext = HeapRoot;
            RtlTraceDatabaseUnlock (RtlpDphTraceDatabase);
        }
    }
    else {
        pBusyNode->StackTrace = NULL;
    }

    RtlpDebugPageHeapPlaceOnBusyList( HeapRoot, pBusyNode );

    pReturn = pBusyNode->pUserAllocation;

    //
    //  For requests the specify HEAP_ZERO_MEMORY, we'll fill the
    //  user-requested portion of the block with zeros. For requests 
    //  that don't specify HEAP_ZERO_MEMORY, we fill the whole user block
    //  with DPH_PAGE_BLOCK_INFIX.
    //

    if ((Flags & HEAP_ZERO_MEMORY)) {

        //
        // SilviuC: The call below can be saved if we have a way
        // to figure out if the memory for the block was freshly
        // virtual allocated (this is already zeroed). This has
        // an impact for large allocations.
        //

        RtlZeroMemory( pBusyNode->pUserAllocation, Size );
    }
    else {

        RtlFillMemory( pBusyNode->pUserAllocation, Size, DPH_PAGE_BLOCK_INFIX);
    }

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

        // nothing

    }
    else {

        RtlpDphWritePageHeapBlockInformation (
            HeapRoot,
            pBusyNode->pUserAllocation,
            Size,
            nBytesAccess);
    }

    EXIT:

    if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION) && !ForcePageHeap) {
        RtlpDphInternalValidatePageHeap (HeapRoot, NULL, 0);
    }

    PROTECT_HEAP_STRUCTURES( HeapRoot );
    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    if (pReturn == NULL) {
        IF_GENERATE_EXCEPTION( Flags, STATUS_NO_MEMORY );
    }

    return pReturn;
}

BOOLEAN
RtlpDebugPageHeapFree(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    )
{

    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK Node, Prev;
    BOOLEAN              Success;
    PCH                  p;
    ULONG Reason;

    //
    // Check if null frees are of any concern.
    //

    if (Address == NULL) {

        if ((RtlpDphDebugLevel & DPH_DEBUG_BREAK_FOR_NULL_FREE)) {

            DbgPrint ("Page heap: attempt to free null pointer \n");
            DbgBreakPoint();
        }

        //
        // For C++ apps that delete NULL this is valid.
        //

        return TRUE;
    }

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if (HeapRoot == NULL)
        return FALSE;

    //
    // Acquire heap lock and unprotect heap structures.
    //

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION)) {
        RtlpDphInternalValidatePageHeap (HeapRoot, NULL, 0);
    }

    Flags |= HeapRoot->HeapFlags;

    //
    // Compute free statistics
    //

    BUMP_GLOBAL_COUNTER (DPH_COUNTER_NO_OF_FREES);
    HeapRoot->Counter[DPH_COUNTER_NO_OF_FREES] += 1;


    Success = FALSE;

    Node = RtlpDebugPageHeapFindBusyMem( HeapRoot, Address, &Prev );

    if (Node == NULL) {

        //
        // No wonder we did not find the block in the page heap
        // structures because the block was probably allocated
        // from the normal heap. Or there is a real bug.
        // If there is a bug NormalHeapFree will break into debugger.
        //

        Success = RtlpDphNormalHeapFree (

            HeapRoot,
            Flags,
            Address);

        goto EXIT;
    }

    //
    //  If tail was allocated, make sure filler not overwritten
    //

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

        if (Node->nVirtualAccessSize > 0) {
            RtlpDebugPageHeapProtectVM( Node->pVirtualBlock + PAGE_SIZE,
                Node->nVirtualAccessSize,
                PAGE_NOACCESS );
        }
    }
    else {

        //
        // (SilviuC): This can be done at the beginning of the function.
        //

        if (! (RtlpDphIsPageHeapBlock (HeapRoot, Address, &Reason, TRUE))) {

            RtlpDphReportCorruptedBlock (Address, Reason);
        }

        if (Node->nVirtualAccessSize > 0) {

            //
            // Mark the block as freed. The information is gone if we
            // will decommit the region but will remain if smart memory
            // flag is not set and can help debug failures.
            //
            
            {
                PDPH_BLOCK_INFORMATION Info = (PDPH_BLOCK_INFORMATION)(Node->pUserAllocation);

                Info -= 1;
                Info->StartStamp -= 1;
                Info->EndStamp -= 1;
            }

            RtlpDebugPageHeapProtectVM( Node->pVirtualBlock,
                Node->nVirtualAccessSize,
                PAGE_NOACCESS );
        }
    }

    RtlpDebugPageHeapRemoveFromBusyList( HeapRoot, Node, Prev );

    //
    // If we use uncommitted ranges we need to decommit the memory
    // range now for the allocation. Note that the next page (guard) 
    // was already decommitted when we allocated the block.
    //

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

        // nothing            

    }
    else {

        if ((HeapRoot->ExtraFlags & PAGE_HEAP_SMART_MEMORY_USAGE)) {

            RtlpDebugPageHeapDecommitVM (
                Node->pVirtualBlock, 
                Node->nVirtualAccessSize);
        }
    }


    RtlpDebugPageHeapPlaceOnFreeList( HeapRoot, Node );

    //
    //  RtlpDebugPageHeapFree gets called from RtlDebugFreeHeap, which
    //  gets called from RtlFreeHeapSlowly, which gets called from
    //  RtlFreeHeap.  To keep from wasting lots of stack trace storage,
    //  we'll skip the bottom 3 entries, leaving RtlFreeHeap as the
    //  first recorded entry.
    //

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_COLLECT_STACK_TRACES)) {

        if (Node->StackTrace) {

            RtlTraceDatabaseLock (RtlpDphTraceDatabase);

            if (Node->StackTrace->UserCount > 0) {
                Node->StackTrace->UserCount -= 1;
            }

            if (Node->StackTrace->UserSize >= Node->nUserRequestedSize) {
                Node->StackTrace->UserSize -= Node->nUserRequestedSize;
            }

            RtlTraceDatabaseUnlock (RtlpDphTraceDatabase);
        }

        Node->StackTrace = RtlpDphLogStackTrace(3);
    }
    else {
        Node->StackTrace = NULL;
    }

    Success = TRUE;

    EXIT:

    if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION)) {
        RtlpDphInternalValidatePageHeap (HeapRoot, NULL, 0);
    }

    PROTECT_HEAP_STRUCTURES( HeapRoot );
    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    if (! Success) {
        IF_GENERATE_EXCEPTION( Flags, STATUS_ACCESS_VIOLATION );
    }

    return Success;
}

PVOID
RtlpDebugPageHeapReAllocate(
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID  Address,
    IN SIZE_T Size
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK OldNode, OldPrev, NewNode;
    PVOID                NewAddress;
    PUCHAR               p;
    SIZE_T               CopyDataSize;
    ULONG                SaveFlags;
    BOOLEAN ReallocInNormalHeap = FALSE;
    ULONG Reason;
    BOOLEAN ForcePageHeap = FALSE;
    BOOLEAN OriginalAllocationInPageHeap = FALSE;

    if (IS_BIASED_POINTER(HeapHandle)) {
        HeapHandle = UNBIAS_POINTER(HeapHandle);
        ForcePageHeap = TRUE;
    }

    //
    // Is zero size allocation ?
    //

    if (Size == 0) {

        if ((RtlpDphDebugLevel & DPH_DEBUG_BREAK_FOR_SIZE_ZERO)) {

            DbgPrint ("Page heap: request for zero sized block \n");
            DbgBreakPoint();
        }
    }

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if (HeapRoot == NULL)
        return NULL;

    //
    // Get heap lock and unprotect heap structures.
    //

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION)) {
        RtlpDphInternalValidatePageHeap (HeapRoot, NULL, 0);
    }

    Flags |= HeapRoot->HeapFlags;

    //
    // Compute realloc statistics
    //

    BUMP_GLOBAL_COUNTER (DPH_COUNTER_NO_OF_REALLOCS);
    BUMP_SIZE_COUNTER (Size);

    HeapRoot->Counter[DPH_COUNTER_NO_OF_REALLOCS] += 1;

    if (Size < 1024) {
        BUMP_GLOBAL_COUNTER (DPH_COUNTER_SIZE_BELOW_1K);
        HeapRoot->Counter[DPH_COUNTER_SIZE_BELOW_1K] += 1;
    }
    else if (Size < 4096) {
        BUMP_GLOBAL_COUNTER (DPH_COUNTER_SIZE_BELOW_4K);
        HeapRoot->Counter[DPH_COUNTER_SIZE_BELOW_4K] += 1;
    }
    else {
        BUMP_GLOBAL_COUNTER (DPH_COUNTER_SIZE_ABOVE_4K);
        HeapRoot->Counter[DPH_COUNTER_SIZE_ABOVE_4K] += 1;
    }


    NewAddress = NULL;

    //
    //  Check Flags for non-moveable reallocation and fail it
    //  unconditionally.  Apps that specify this flag should be
    //  prepared to deal with failure anyway.
    //

    if (Flags & HEAP_REALLOC_IN_PLACE_ONLY) {
        goto EXIT;
    }

    //
    //  Validate requested size so we don't overflow
    //  while rounding up size computations.  We do this
    //  after we've acquired the critsect so we can still
    //  catch serialization problems.
    //

    if (Size > 0x7FFF0000) {
        OUT_OF_VM_BREAK( Flags, "Page heap: Invalid allocation size\n" );
        goto EXIT;
    }

    OldNode = RtlpDebugPageHeapFindBusyMem( HeapRoot, Address, &OldPrev );

    if (OldNode) {
        OriginalAllocationInPageHeap = TRUE;
    }

    if (OldNode == NULL) {

        //
        // No wonder we did not find the block in the page heap
        // structures because the block was probably allocated
        // from the normal heap. Or there is a real bug. If there 
        // is a bug NormalHeapReAllocate will break into debugger.
        //

        NewAddress = RtlpDphNormalHeapReAllocate (

            HeapRoot,
            Flags,
            Address,
            Size);

        goto EXIT;
    }

    //
    //  If tail was allocated, make sure filler not overwritten
    //

    if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

        // nothing
    }
    else {

        //
        // (SilviuC): This can be done at the beginning of the function.
        //

        if (! (RtlpDphIsPageHeapBlock (HeapRoot, Address, &Reason, TRUE))) {

            RtlpDphReportCorruptedBlock (Address, Reason);
        }
    }

    //
    //  Before allocating a new block, remove the old block from
    //  the busy list.  When we allocate the new block, the busy
    //  list pointers will change, possibly leaving our acquired
    //  Prev pointer invalid.
    //

    RtlpDebugPageHeapRemoveFromBusyList( HeapRoot, OldNode, OldPrev );

    //
    //  Allocate new memory for new requested size.  Use try/except
    //  to trap exception if Flags caused out-of-memory exception.
    //

    try {

        if (!ForcePageHeap && !(RtlpDphShouldAllocateInPageHeap (HeapRoot, Size))) {

            NewAddress = RtlpDphNormalHeapAllocate (
                HeapRoot, 
                Flags, 
                Size);

            ReallocInNormalHeap = TRUE;
        }
        else {

            //
            // Force the allocation in page heap by biasing
            // the heap handle. Validate the heap here since when we use
            // biased pointers validation inside Allocate is disabled.
            //

            if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION)) {
                RtlpDphInternalValidatePageHeap (HeapRoot, OldNode->pVirtualBlock, OldNode->nVirtualBlockSize);
            }

            NewAddress = RtlpDebugPageHeapAllocate( 
                BIAS_POINTER(HeapHandle), 
                Flags, 
                Size);

            
            if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION)) {
                RtlpDphInternalValidatePageHeap (HeapRoot, OldNode->pVirtualBlock, OldNode->nVirtualBlockSize);
            }

            ReallocInNormalHeap = FALSE;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
    }

    //
    // We managed to make a new allocation (normal or page heap).
    // Now we need to copy from old to new all sorts of stuff 
    // (contents, user flags/values).
    //

    if (NewAddress) {

        //
        // Copy old block contents into the new node.
        //

        CopyDataSize = OldNode->nUserRequestedSize;

        if (CopyDataSize > Size) {
            CopyDataSize = Size;
        }

        if (CopyDataSize > 0) {

            RtlCopyMemory(
                NewAddress,
                Address,
                CopyDataSize
                );
        }

        //
        // If new allocation was done in page heap we need to detect the new node
        // and copy over user flags/values.
        //

        if (! ReallocInNormalHeap) {

            NewNode = RtlpDebugPageHeapFindBusyMem( HeapRoot, NewAddress, NULL );

            //
            // This block could not be in normal heap therefore from this
            // respect the call above should always succeed.
            //

            DEBUG_ASSERT( NewNode != NULL );

            NewNode->UserValue = OldNode->UserValue;
            NewNode->UserFlags = ( Flags & HEAP_SETTABLE_USER_FLAGS ) ?
                ( Flags & HEAP_SETTABLE_USER_FLAGS ) :
            OldNode->UserFlags;

        }
        
        //
        // We need to cover the case where old allocation was in page heap.
        // In this case we still need to cleanup the old node and 
        // insert it back in free list. Actually the way the code is written
        // we take this code path only if original allocation was in page heap.
        // This is the reason for the assert.
        //


        RETAIL_ASSERT (OriginalAllocationInPageHeap);

        if (OriginalAllocationInPageHeap) {

            if (OldNode->nVirtualAccessSize > 0) {
                RtlpDebugPageHeapProtectVM( OldNode->pVirtualBlock,
                    OldNode->nVirtualAccessSize,
                    PAGE_NOACCESS );
            }

            //
            // If we use uncommitted ranges we need to decommit the memory
            // range now. Note that the next page (guard) was already decommitted
            // when we made the allocation.
            //

            if ((HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {

                // nothing            

            }
            else {

                if ((HeapRoot->ExtraFlags & PAGE_HEAP_SMART_MEMORY_USAGE)) {

                    RtlpDebugPageHeapDecommitVM (
                        OldNode->pVirtualBlock, 
                        OldNode->nVirtualAccessSize);
                }
            }

            RtlpDebugPageHeapPlaceOnFreeList( HeapRoot, OldNode );

            //
            // RtlpDebugPageHeapReAllocate gets called from RtlDebugReAllocateHeap,
            // which gets called from RtlReAllocateHeap.  To keep from wasting
            // lots of stack trace storage, we'll skip the bottom 2 entries,
            // leaving RtlReAllocateHeap as the first recorded entry in the
            // freed stack trace.
            //
            // Note. For realloc we need to do the accounting for free in the
            // trace block. The accounting for alloc is done in the real
            // alloc operation which always happens for page heap reallocs.
            //

            if ((HeapRoot->ExtraFlags & PAGE_HEAP_COLLECT_STACK_TRACES)) {

                if (OldNode->StackTrace) {

                    RtlTraceDatabaseLock (RtlpDphTraceDatabase);

                    if (OldNode->StackTrace->UserCount > 0) {
                        OldNode->StackTrace->UserCount -= 1;
                    }

                    if (OldNode->StackTrace->UserSize >= OldNode->nUserRequestedSize) {
                        OldNode->StackTrace->UserSize -= OldNode->nUserRequestedSize;
                    }

                    RtlTraceDatabaseUnlock (RtlpDphTraceDatabase);
                }

                OldNode->StackTrace = RtlpDphLogStackTrace(2);
            }
            else {
                OldNode->StackTrace = NULL;
            }
        }
    }

    else {

        //
        //  Failed to allocate a new block.  Return old block to busy list.
        //

        if (OriginalAllocationInPageHeap) {

            RtlpDebugPageHeapPlaceOnBusyList( HeapRoot, OldNode );
        }

    }

    EXIT:

    if ((RtlpDphDebugLevel & DPH_DEBUG_INTERNAL_VALIDATION)) {
        RtlpDphInternalValidatePageHeap (HeapRoot, NULL, 0);
    }

    PROTECT_HEAP_STRUCTURES( HeapRoot );
    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    if (NewAddress == NULL) {
        IF_GENERATE_EXCEPTION( Flags, STATUS_NO_MEMORY );
    }

    return NewAddress;
}

//silviuc: does this really work for all functions in between
#if (( DPH_CAPTURE_STACK_TRACE ) && ( i386 ) && ( FPO ))
#pragma optimize( "", on )      // restore original optimizations
#endif

PVOID
RtlpDebugPageHeapDestroy(
    IN PVOID HeapHandle
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_ROOT       PrevHeapRoot;
    PDPH_HEAP_ROOT       NextHeapRoot;
    PDPH_HEAP_BLOCK Node;
    PDPH_HEAP_BLOCK Next;
    ULONG                Flags;
    PUCHAR               p;
    ULONG Reason;
    PVOID NormalHeap;

    if (HeapHandle == RtlProcessHeap()) {
        RtlpDebugPageHeapBreak( "Page heap: Attempt to destroy process heap\n" );
        return NULL;
    }

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if (HeapRoot == NULL)
        return NULL;

    Flags = HeapRoot->HeapFlags | HEAP_NO_SERIALIZE;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    //
    // Save normal heap pointer for later.
    //

    NormalHeap = HeapRoot->NormalHeap;

    //
    //  Walk all busy allocations and check for tail fill corruption
    //

    Node = HeapRoot->pBusyAllocationListHead;

    while (Node) {

        if (! (HeapRoot->ExtraFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {
            if (! (RtlpDphIsPageHeapBlock (HeapRoot, Node->pUserAllocation, &Reason, TRUE))) {
                RtlpDphReportCorruptedBlock (Node->pUserAllocation, Reason);
            }
        }

        Node = Node->pNextAlloc;
    }

    //
    //  Remove this heap entry from the process heap linked list.
    //

    RtlEnterCriticalSection( &RtlpDphHeapListCriticalSection );

    if (HeapRoot->pPrevHeapRoot) {
        HeapRoot->pPrevHeapRoot->pNextHeapRoot = HeapRoot->pNextHeapRoot;
    }
    else {
        RtlpDphHeapListHead = HeapRoot->pNextHeapRoot;
    }

    if (HeapRoot->pNextHeapRoot) {
        HeapRoot->pNextHeapRoot->pPrevHeapRoot = HeapRoot->pPrevHeapRoot;
    }
    else {
        RtlpDphHeapListTail = HeapRoot->pPrevHeapRoot;
    }

    RtlpDphHeapListCount--;

    RtlLeaveCriticalSection( &RtlpDphHeapListCriticalSection );


    //
    //  Must release critical section before deleting it; otherwise,
    //  checked build Teb->CountOfOwnedCriticalSections gets out of sync.
    //

    RtlLeaveCriticalSection( HeapRoot->HeapCritSect );
    RtlDeleteCriticalSection( HeapRoot->HeapCritSect );

    //
    //  This is weird.  A virtual block might contain storage for
    //  one of the nodes necessary to walk this list.  In fact,
    //  we're guaranteed that the root node contains at least one
    //  virtual alloc node.
    //
    //  Each time we alloc new VM, we make that the head of the
    //  of the VM list, like a LIFO structure.  I think we're ok
    //  because no VM list node should be on a subsequently alloc'd
    //  VM -- only a VM list entry might be on its own memory (as
    //  is the case for the root node).  We read pNode->pNextAlloc
    //  before releasing the VM in case pNode existed on that VM.
    //  I think this is safe -- as long as the VM list is LIFO and
    //  we don't do any list reorganization.
    //

    Node = HeapRoot->pVirtualStorageListHead;

    while (Node) {
        Next = Node->pNextAlloc;
        if (! RtlpDebugPageHeapReleaseVM( Node->pVirtualBlock )) {
            RtlpDebugPageHeapBreak( "Page heap: Unable to release virtual memory\n" );
        }
        Node = Next;
    }

    //
    // Free all blocks in the delayed free queue that belong to the
    // normal heap just about to be destroyed. Note that this is
    // not a bug. The application freed the blocks correctly but
    // we delayed the free operation.
    //

    RtlpDphFreeDelayedBlocksFromHeap (HeapRoot, NormalHeap);

    //
    // Destroy normal heap. Note that this will not make a recursive
    // call into this function because this is not a page heap and
    // code in NT heap manager will detect this.
    //

    RtlDestroyHeap (NormalHeap);

    //
    //  That's it.  All the VM, including the root node, should now
    //  be released.  RtlDestroyHeap always returns NULL.
    //

    DbgPrint( "Page heap: process 0x%X destroyed heap @ %p (%p)\n",
        NtCurrentTeb()->ClientId.UniqueProcess,
        HeapRoot,
        NormalHeap);

    return NULL;
}

SIZE_T
RtlpDebugPageHeapSize(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK Node;
    SIZE_T               Size;

    Size = -1;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if (HeapRoot == NULL) {
        return Size;
    }

    Flags |= HeapRoot->HeapFlags;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    Node = RtlpDebugPageHeapFindBusyMem( HeapRoot, Address, NULL );

    if (Node == NULL) {

        //
        // No wonder we did not find the block in the page heap
        // structures because the block was probably allocated
        // from the normal heap. Or there is a real bug. If there
        // is a bug NormalHeapSize will break into debugger.
        //

        Size = RtlpDphNormalHeapSize (

            HeapRoot,
            Flags,
            Address);

        goto EXIT;
    }
    else {
        Size = Node->nUserRequestedSize;
    }

    EXIT:
    PROTECT_HEAP_STRUCTURES( HeapRoot );
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    if (Size == -1) {
        IF_GENERATE_EXCEPTION( Flags, STATUS_ACCESS_VIOLATION );
    }

    return Size;
}

ULONG       
RtlpDebugPageHeapGetProcessHeaps(
    ULONG NumberOfHeaps,
    PVOID *ProcessHeaps
    )
{
    PDPH_HEAP_ROOT HeapRoot;
    ULONG          Count;

    //
    //  Although we'd expect GetProcessHeaps never to be called
    //  before at least the very first heap creation, we should
    //  still be safe and initialize the critical section if
    //  necessary.
    //

    if (! RtlpDphHeapListHasBeenInitialized) {
        RtlpDphHeapListHasBeenInitialized = TRUE;
        RtlInitializeCriticalSection( &RtlpDphHeapListCriticalSection );
    }

    RtlEnterCriticalSection( &RtlpDphHeapListCriticalSection );

    if (RtlpDphHeapListCount <= NumberOfHeaps) {

        for (HeapRoot  = RtlpDphHeapListHead, Count = 0;
            HeapRoot != NULL;
            HeapRoot  = HeapRoot->pNextHeapRoot, Count += 1) {

            *ProcessHeaps++ = HEAP_HANDLE_FROM_ROOT( HeapRoot );
        }

        if (Count != RtlpDphHeapListCount) {
            RtlpDebugPageHeapBreak( "Page heap: BUG: process heap list count wrong\n" );
        }

    }
    else {

        //
        //  User's buffer is too small.  Return number of entries
        //  necessary for subsequent call to succeed.  Buffer
        //  remains untouched.
        //

        Count = RtlpDphHeapListCount;

    }

    RtlLeaveCriticalSection( &RtlpDphHeapListCriticalSection );

    return Count;
}

ULONG
RtlpDebugPageHeapCompact(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    PDPH_HEAP_ROOT HeapRoot;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if (HeapRoot == NULL)
        return 0;

    Flags |= HeapRoot->HeapFlags;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );

    //
    //  Don't do anything, but we did want to acquire the critsect
    //  in case this was called with HEAP_NO_SERIALIZE while another
    //  thread is in the heap code.
    //

    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    return 0;
}

BOOLEAN
RtlpDebugPageHeapValidate(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK Node;
    BOOLEAN Result = FALSE;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if (HeapRoot == NULL)
        return FALSE;

    Flags |= HeapRoot->HeapFlags;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    DEBUG_CODE( RtlpDebugPageHeapVerifyIntegrity( HeapRoot ));
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    Node = Address ? RtlpDebugPageHeapFindBusyMem( HeapRoot, Address, NULL ) : NULL;

    if (Node == NULL) {

        Result = RtlpDphNormalHeapValidate (
            HeapRoot,
            Flags,
            Address);
    }

    PROTECT_HEAP_STRUCTURES( HeapRoot );
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    if (Address) {
        if (Node) {
            return TRUE;
        }
        else {
            return Result;
        }
    }
    else {
        return TRUE;
    }
}

NTSTATUS
RtlpDebugPageHeapWalk(
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
RtlpDebugPageHeapLock(
    IN PVOID HeapHandle
    )
{
    PDPH_HEAP_ROOT HeapRoot;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );

    if (HeapRoot == NULL) {
        return FALSE;
    }

    RtlpDebugPageHeapEnterCritSect( HeapRoot, HeapRoot->HeapFlags );

    return TRUE;
}

BOOLEAN
RtlpDebugPageHeapUnlock(
    IN PVOID HeapHandle
    )
{
    PDPH_HEAP_ROOT HeapRoot;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );

    if (HeapRoot == NULL) {
        return FALSE;
    }

    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    return TRUE;
}

BOOLEAN
RtlpDebugPageHeapSetUserValue(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN PVOID UserValue
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK Node;
    BOOLEAN              Success;

    Success = FALSE;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if ( HeapRoot == NULL )
        return Success;

    Flags |= HeapRoot->HeapFlags;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    Node = RtlpDebugPageHeapFindBusyMem( HeapRoot, Address, NULL );

    if ( Node == NULL ) {

        //
        // If we cannot find the node in page heap structures it might be
        // because it has been allocated from normal heap.
        //

        Success = RtlpDphNormalHeapSetUserValue (
            HeapRoot,
            Flags,
            Address,
            UserValue);

        goto EXIT;
    }
    else {
        Node->UserValue = UserValue;
        Success = TRUE;
    }

    EXIT:
    PROTECT_HEAP_STRUCTURES( HeapRoot );
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    return Success;
}

BOOLEAN
RtlpDebugPageHeapGetUserInfo(
    IN  PVOID  HeapHandle,
    IN  ULONG  Flags,
    IN  PVOID  Address,
    OUT PVOID* UserValue,
    OUT PULONG UserFlags
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK Node;
    BOOLEAN              Success;

    Success = FALSE;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if ( HeapRoot == NULL )
        return Success;

    Flags |= HeapRoot->HeapFlags;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    Node = RtlpDebugPageHeapFindBusyMem( HeapRoot, Address, NULL );

    if ( Node == NULL ) {

        //
        // If we cannot find the node in page heap structures it might be
        // because it has been allocated from normal heap.
        //

        Success = RtlpDphNormalHeapGetUserInfo (
            HeapRoot,
            Flags,
            Address,
            UserValue,
            UserFlags);

        goto EXIT;
    }
    else {
        if ( UserValue != NULL )
            *UserValue = Node->UserValue;
        if ( UserFlags != NULL )
            *UserFlags = Node->UserFlags;
        Success = TRUE;
    }

    EXIT:
    PROTECT_HEAP_STRUCTURES( HeapRoot );
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    return Success;
}

BOOLEAN
RtlpDebugPageHeapSetUserFlags(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    )
{
    PDPH_HEAP_ROOT       HeapRoot;
    PDPH_HEAP_BLOCK Node;
    BOOLEAN              Success;

    Success = FALSE;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if ( HeapRoot == NULL )
        return Success;

    Flags |= HeapRoot->HeapFlags;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, Flags );
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    Node = RtlpDebugPageHeapFindBusyMem( HeapRoot, Address, NULL );

    if ( Node == NULL ) {

        //
        // If we cannot find the node in page heap structures it might be
        // because it has been allocated from normal heap.
        //

        Success = RtlpDphNormalHeapSetUserFlags (
            HeapRoot,
            Flags,
            Address,
            UserFlagsReset,
            UserFlagsSet);

        goto EXIT;
    }
    else {
        Node->UserFlags &= ~( UserFlagsReset );
        Node->UserFlags |=    UserFlagsSet;
        Success = TRUE;
    }

    EXIT:
    PROTECT_HEAP_STRUCTURES( HeapRoot );
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    return Success;
}

BOOLEAN
RtlpDebugPageHeapSerialize(
    IN PVOID HeapHandle
    )
{
    PDPH_HEAP_ROOT HeapRoot;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if ( HeapRoot == NULL )
        return FALSE;

    RtlpDebugPageHeapEnterCritSect( HeapRoot, 0 );
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    HeapRoot->HeapFlags &= ~HEAP_NO_SERIALIZE;

    PROTECT_HEAP_STRUCTURES( HeapRoot );
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    return TRUE;
}

NTSTATUS
RtlpDebugPageHeapExtend(
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID  Base,
    IN SIZE_T Size
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
RtlpDebugPageHeapZero(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
RtlpDebugPageHeapReset(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
RtlpDebugPageHeapUsage(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN OUT PRTL_HEAP_USAGE Usage
    )
{
    PDPH_HEAP_ROOT HeapRoot;

    //
    //  Partial implementation since this information is kind of meaningless.
    //

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if ( HeapRoot == NULL )
        return STATUS_INVALID_PARAMETER;

    if ( Usage->Length != sizeof( RTL_HEAP_USAGE ))
        return STATUS_INFO_LENGTH_MISMATCH;

    memset( Usage, 0, sizeof( RTL_HEAP_USAGE ));
    Usage->Length = sizeof( RTL_HEAP_USAGE );

    RtlpDebugPageHeapEnterCritSect( HeapRoot, 0 );
    UNPROTECT_HEAP_STRUCTURES( HeapRoot );

    Usage->BytesAllocated       = HeapRoot->nBusyAllocationBytesAccessible;
    Usage->BytesCommitted       = HeapRoot->nVirtualStorageBytes;
    Usage->BytesReserved        = HeapRoot->nVirtualStorageBytes;
    Usage->BytesReservedMaximum = HeapRoot->nVirtualStorageBytes;

    PROTECT_HEAP_STRUCTURES( HeapRoot );
    RtlpDebugPageHeapLeaveCritSect( HeapRoot );

    return STATUS_SUCCESS;
}

BOOLEAN
RtlpDebugPageHeapIsLocked(
    IN PVOID HeapHandle
    )
{
    PDPH_HEAP_ROOT HeapRoot;

    HeapRoot = RtlpDebugPageHeapPointerFromHandle( HeapHandle );
    if ( HeapRoot == NULL )
        return FALSE;

    if ( RtlTryEnterCriticalSection( HeapRoot->HeapCritSect )) {
        RtlLeaveCriticalSection( HeapRoot->HeapCritSect );
        return FALSE;
    }
    else {
        return TRUE;
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////// Page heap vs. normal heap decision making
/////////////////////////////////////////////////////////////////////

RtlpDphShouldAllocateInPageHeap (
    PDPH_HEAP_ROOT HeapRoot,
    SIZE_T Size
    )
{
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    NTSTATUS Status;
    ULONG Random;
    ULONG Percentage;

    //
    // If page heap is not enabled => normal heap.
    //
    
    if (! (HeapRoot->ExtraFlags & PAGE_HEAP_ENABLE_PAGE_HEAP)) {
        return FALSE;
    }

    //
    // If in size range => page heap
    //

    else if ((HeapRoot->ExtraFlags & PAGE_HEAP_USE_SIZE_RANGE)) {

        if (Size >= RtlpDphSizeRangeStart && Size <= RtlpDphSizeRangeEnd) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    
    //
    // If in dll range => page heap
    //

    else if ((HeapRoot->ExtraFlags & PAGE_HEAP_USE_DLL_RANGE)) {

        PVOID StackTrace[32];
        ULONG Count;
        ULONG Index;
        ULONG Hash;

        Count = RtlCaptureStackBackTrace (
            1,
            32,
            StackTrace,
            &Hash);

        //
        // (SilviuC): should read DllRange as PVOIDs
        //

        for (Index = 0; Index < Count; Index += 1) {
            if (PtrToUlong(StackTrace[Index]) >= RtlpDphDllRangeStart 
                && PtrToUlong(StackTrace[Index]) <= RtlpDphDllRangeEnd) {

                return TRUE;
            }
        }
        
        return FALSE;
    }
    
    //
    // If randomly decided => page heap
    //

    else if ((HeapRoot->ExtraFlags & PAGE_HEAP_USE_RANDOM_DECISION)) {

        Random = RtlRandom (& (HeapRoot->Seed));

        if ((Random % 100) < RtlpDphRandomProbability) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    
    //
    // If call not generated from one of the target dlls => normal heap
    //

    else if ((HeapRoot->ExtraFlags & PAGE_HEAP_USE_DLL_NAMES)) {

        // We return false. The calls generated from target
        // dlls will never get into this function and therefore
        // we just return false signalling that we do not want
        // page heap verification for the rest of the world.
        //
        return FALSE;
    }

    //
    // For all other cases we will allocate in the page heap.
    //

    else {
        return TRUE;
    }
}



/////////////////////////////////////////////////////////////////////
//////////////////////////////////// DPH_BLOCK_INFORMATION management
/////////////////////////////////////////////////////////////////////

VOID
RtlpDphReportCorruptedBlock (
    PVOID Block,
    ULONG Reason
    )
{
    DbgPrint ("Page heap: block @ %p is corrupted (reason %0X) \n", Block, Reason);

    if ((Reason & DPH_ERROR_CORRUPTED_INFIX_PATTERN)) {
        DbgPrint ("Page heap: reason: corrupted infix pattern for freed block \n");
    }
    if ((Reason & DPH_ERROR_CORRUPTED_START_STAMP)) {
        DbgPrint ("Page heap: reason: corrupted start stamp \n");
    }
    if ((Reason & DPH_ERROR_CORRUPTED_END_STAMP)) {
        DbgPrint ("Page heap: reason: corrupted end stamp \n");
    }
    if ((Reason & DPH_ERROR_CORRUPTED_HEAP_POINTER)) {
        DbgPrint ("Page heap: reason: corrupted heap pointer \n");
    }
    if ((Reason & DPH_ERROR_CORRUPTED_PREFIX_PATTERN)) {
        DbgPrint ("Page heap: reason: corrupted prefix pattern \n");
    }
    if ((Reason & DPH_ERROR_CORRUPTED_SUFFIX_PATTERN)) {
        DbgPrint ("Page heap: reason: corrupted suffix pattern \n");
    }
    if ((Reason & DPH_ERROR_RAISED_EXCEPTION)) {
        DbgPrint ("Page heap: reason: raised exception while probing \n");
    }

    DbgBreakPoint ();
}

BOOLEAN
RtlpDphIsPageHeapBlock (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    PULONG Reason,
    BOOLEAN CheckPattern
    )
{
    PDPH_BLOCK_INFORMATION Info;
    BOOLEAN Corrupted = FALSE;
    PUCHAR Current;
    PUCHAR FillStart;
    PUCHAR FillEnd;

    DEBUG_ASSERT (Reason != NULL);
    *Reason = 0;

    try {
        
        Info = (PDPH_BLOCK_INFORMATION)Block - 1;
        
        //
        // Start checking ...
        //

        if (Info->StartStamp != DPH_PAGE_BLOCK_START_STAMP_ALLOCATED) {
            *Reason |= DPH_ERROR_CORRUPTED_START_STAMP;
            Corrupted = TRUE;
        }
        
        if (Info->EndStamp != DPH_PAGE_BLOCK_END_STAMP_ALLOCATED) {
            *Reason |= DPH_ERROR_CORRUPTED_END_STAMP;
            Corrupted = TRUE;
        }

        if (Info->Heap != Heap) {
            *Reason |= DPH_ERROR_CORRUPTED_HEAP_POINTER;
            Corrupted = TRUE;
        }

        //
        // Check the block suffix byte pattern.
        //

        if (CheckPattern) {
            
            FillStart = (PUCHAR)Block + Info->RequestedSize;
            FillEnd = (PUCHAR)ROUNDUP2((ULONG_PTR)FillStart, PAGE_SIZE);

            for (Current = FillStart; Current < FillEnd; Current++) {

                if (*Current != DPH_PAGE_BLOCK_SUFFIX) {

                    *Reason |= DPH_ERROR_CORRUPTED_SUFFIX_PATTERN;
                    Corrupted = TRUE;
                    break;
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        *Reason |= DPH_ERROR_RAISED_EXCEPTION;
        Corrupted = TRUE;
    }

    if (Corrupted) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

BOOLEAN
RtlpDphIsNormalHeapBlock (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    PULONG Reason,
    BOOLEAN CheckPattern
    )
{
    PDPH_BLOCK_INFORMATION Info;
    BOOLEAN Corrupted = FALSE;
    PUCHAR Current;
    PUCHAR FillStart;
    PUCHAR FillEnd;

    DEBUG_ASSERT (Reason != NULL);
    *Reason = 0;

    Info = (PDPH_BLOCK_INFORMATION)Block - 1;

    try {

        if (Info->Heap != Heap) {
            *Reason |= DPH_ERROR_CORRUPTED_HEAP_POINTER;
            Corrupted = TRUE;
        }

        if (Info->StartStamp != DPH_NORMAL_BLOCK_START_STAMP_ALLOCATED) {
            *Reason |= DPH_ERROR_CORRUPTED_START_STAMP;
            Corrupted = TRUE;
        }
        
        if (Info->EndStamp != DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED) {
            *Reason |= DPH_ERROR_CORRUPTED_END_STAMP;
            Corrupted = TRUE;
        }

        //
        // Check the block suffix byte pattern.
        //

        if (CheckPattern) {
        
            FillStart = (PUCHAR)Block + Info->RequestedSize;
            FillEnd = FillStart + USER_ALIGNMENT;

            for (Current = FillStart; Current < FillEnd; Current++) {

                if (*Current != DPH_NORMAL_BLOCK_SUFFIX) {

                    *Reason |= DPH_ERROR_CORRUPTED_SUFFIX_PATTERN;
                    Corrupted = TRUE;
                    break;
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        *Reason |= DPH_ERROR_RAISED_EXCEPTION;
        Corrupted = TRUE;
    }

    if (Corrupted) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

BOOLEAN
RtlpDphIsNormalFreeHeapBlock (
    PVOID Block,
    PULONG Reason,
    BOOLEAN CheckPattern
    )
{
    PDPH_BLOCK_INFORMATION Info;
    BOOLEAN Corrupted = FALSE;
    PUCHAR Current;
    PUCHAR FillStart;
    PUCHAR FillEnd;

    DEBUG_ASSERT (Reason != NULL);
    *Reason = 0;

    Info = (PDPH_BLOCK_INFORMATION)Block - 1;

    try {

        //
        // If heap pointer is null we will just ignore this field.
        // This can happen during heap destroy operations where
        // the page heap got destroyed but the normal heap is still
        // alive.
        //

        if (Info->StartStamp != DPH_NORMAL_BLOCK_START_STAMP_FREE) {
            *Reason |= DPH_ERROR_CORRUPTED_START_STAMP;
            Corrupted = TRUE;
        }
        
        if (Info->EndStamp != DPH_NORMAL_BLOCK_END_STAMP_FREE) {
            *Reason |= DPH_ERROR_CORRUPTED_END_STAMP;
            Corrupted = TRUE;
        }

        //
        // Check the block suffix byte pattern.
        //
        
        if (CheckPattern) {
            
            FillStart = (PUCHAR)Block + Info->RequestedSize;
            FillEnd = FillStart + USER_ALIGNMENT;

            for (Current = FillStart; Current < FillEnd; Current++) {

                if (*Current != DPH_NORMAL_BLOCK_SUFFIX) {

                    *Reason |= DPH_ERROR_CORRUPTED_SUFFIX_PATTERN;
                    Corrupted = TRUE;
                    break;
                }
            }
        }
        
        //
        // Check the block infix byte pattern.
        //

        if (CheckPattern) {
            
            FillStart = (PUCHAR)Block;
            FillEnd = FillStart 
                + ((Info->RequestedSize > USER_ALIGNMENT) ? USER_ALIGNMENT : Info->RequestedSize);

            for (Current = FillStart; Current < FillEnd; Current++) {

                if (*Current != DPH_FREE_BLOCK_INFIX) {

                    *Reason |= DPH_ERROR_CORRUPTED_INFIX_PATTERN;
                    Corrupted = TRUE;
                    break;
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        *Reason |= DPH_ERROR_RAISED_EXCEPTION;
        Corrupted = TRUE;
    }

    if (Corrupted) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

BOOLEAN
RtlpDphWritePageHeapBlockInformation (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    SIZE_T RequestedSize,
    SIZE_T ActualSize
    )
{
    PDPH_BLOCK_INFORMATION Info;
    PUCHAR FillStart;
    PUCHAR FillEnd;
    ULONG Hash;
    
    //
    // Size and stamp information
    //

    Info = (PDPH_BLOCK_INFORMATION)Block - 1;
    
    Info->Heap = Heap;
    Info->RequestedSize = RequestedSize;
    Info->ActualSize = ActualSize;
    Info->StartStamp = DPH_PAGE_BLOCK_START_STAMP_ALLOCATED;
    Info->EndStamp = DPH_PAGE_BLOCK_END_STAMP_ALLOCATED;

    //
    // Fill the block suffix pattern.
    // We fill up to USER_ALIGNMENT bytes.
    //

    FillStart = (PUCHAR)Block + RequestedSize;
    FillEnd = (PUCHAR)ROUNDUP2((ULONG_PTR)FillStart, PAGE_SIZE);
    
    RtlFillMemory (FillStart, FillEnd - FillStart, DPH_PAGE_BLOCK_SUFFIX);

    //
    // Capture stack trace
    //

    if ((Heap->ExtraFlags & PAGE_HEAP_COLLECT_STACK_TRACES)) {
        Info->StackTrace = RtlpDphLogStackTrace (3);
    }
    else {
        Info->StackTrace = NULL;
    }

    return TRUE;
}

BOOLEAN
RtlpDphWriteNormalHeapBlockInformation (
    PDPH_HEAP_ROOT Heap,
    PVOID Block,
    SIZE_T RequestedSize,
    SIZE_T ActualSize
    )
{
    PDPH_BLOCK_INFORMATION Info;
    PUCHAR FillStart;
    PUCHAR FillEnd;
    ULONG Hash;
    ULONG Reason;

    Info = (PDPH_BLOCK_INFORMATION)Block - 1;

    //
    // Size and stamp information
    //

    Info->Heap = Heap;
    Info->RequestedSize = RequestedSize;
    Info->ActualSize = ActualSize;
    Info->StartStamp = DPH_NORMAL_BLOCK_START_STAMP_ALLOCATED;
    Info->EndStamp = DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED;

    Info->FreeQueue.Blink = NULL;
    Info->FreeQueue.Flink = NULL;

    //
    // Fill the block suffix pattern.
    // We fill only USER_ALIGNMENT bytes.
    //

    FillStart = (PUCHAR)Block + RequestedSize;
    FillEnd = FillStart + USER_ALIGNMENT;
    
    RtlFillMemory (FillStart, FillEnd - FillStart, DPH_NORMAL_BLOCK_SUFFIX);

    //
    // Capture stack trace
    //

    if ((Heap->ExtraFlags & PAGE_HEAP_COLLECT_STACK_TRACES)) {
        
        Info->StackTrace = RtlpDphLogStackTrace (4);

        if (Info->StackTrace) {
            
            RtlTraceDatabaseLock (RtlpDphTraceDatabase);
            ((PRTL_TRACE_BLOCK)(Info->StackTrace))->UserCount += 1;
            ((PRTL_TRACE_BLOCK)(Info->StackTrace))->UserSize += RequestedSize;
            ((PRTL_TRACE_BLOCK)(Info->StackTrace))->UserContext = Heap;
            RtlTraceDatabaseUnlock (RtlpDphTraceDatabase);
        }

    }
    else {
        Info->StackTrace = NULL;
    }
    
    return TRUE;
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////// Normal heap allocation/free functions
/////////////////////////////////////////////////////////////////////

PVOID
RtlpDphNormalHeapAllocate (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    SIZE_T Size
    )
{
    PVOID Block;
    PDPH_BLOCK_INFORMATION Info;
    ULONG Hash;
    SIZE_T ActualSize;
    SIZE_T RequestedSize;
    ULONG Reason;

    BUMP_GLOBAL_COUNTER (DPH_COUNTER_NO_OF_NORMAL_ALLOCS);
    BUMP_SIZE_COUNTER (Size);

    Heap->Counter[DPH_COUNTER_NO_OF_NORMAL_ALLOCS] += 1;

    RequestedSize = Size;
    ActualSize = Size + sizeof(DPH_BLOCK_INFORMATION) + USER_ALIGNMENT;

    Block = RtlAllocateHeap (
        Heap->NormalHeap,
        Flags,
        ActualSize); 

    if (Block == NULL) {

        //
        // (SilviuC): If we have memory pressure we might want 
        // to trim the delayed free queues. We do not do this
        // right now because the threshold is kind of small.
        //
        
        return NULL;
    }

    RtlpDphWriteNormalHeapBlockInformation (
        Heap,
        (PDPH_BLOCK_INFORMATION)Block + 1,
        RequestedSize,
        ActualSize);

    if (! (Flags & HEAP_ZERO_MEMORY)) {

        RtlFillMemory ((PDPH_BLOCK_INFORMATION)Block + 1, 
                       RequestedSize,
                       DPH_NORMAL_BLOCK_INFIX);
    }

    return (PVOID)((PDPH_BLOCK_INFORMATION)Block + 1);
}


BOOLEAN
RtlpDphNormalHeapFree (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    PVOID Block
    )
{
    PDPH_BLOCK_INFORMATION Info;
    BOOLEAN Success;
    ULONG Reason;
    ULONG Hash;
    SIZE_T TrimSize;

    BUMP_GLOBAL_COUNTER (DPH_COUNTER_NO_OF_NORMAL_FREES);
    Heap->Counter[DPH_COUNTER_NO_OF_NORMAL_FREES] += 1;

    Info = (PDPH_BLOCK_INFORMATION)Block - 1;
    
    if (! RtlpDphIsNormalHeapBlock(Heap, Block, &Reason, TRUE)) {
        
        RtlpDphReportCorruptedBlock (Block, Reason);

        return FALSE;
    }

    //
    // Save the free stack trace.
    //

    if ((Heap->ExtraFlags & PAGE_HEAP_COLLECT_STACK_TRACES)) {
        
        if (Info->StackTrace) {
            
            RtlTraceDatabaseLock (RtlpDphTraceDatabase);
            ((PRTL_TRACE_BLOCK)(Info->StackTrace))->UserCount -= 1;
            ((PRTL_TRACE_BLOCK)(Info->StackTrace))->UserSize -= Info->RequestedSize;
            RtlTraceDatabaseUnlock (RtlpDphTraceDatabase);
        }

        Info->StackTrace = RtlpDphLogStackTrace (3);
    }
    else {
        Info->StackTrace = NULL;
    }

    //
    // Mark the block as freed.
    //

    Info->StartStamp -= 1;
    Info->EndStamp -= 1;
    
    //
    // Wipe out all the information in the block so that it cannot
    // be used while free. The pattern looks like a kernel pointer
    // and if we are lucky enough the buggy code might use a value
    // from the block as a pointer and instantly access violate.
    //

    RtlFillMemory (
        Info + 1,
        Info->RequestedSize,
        DPH_FREE_BLOCK_INFIX);
    
    //
    // It is useful during debugging sessions to not free at
    // all so that you can detect use after free, etc.
    //

    if ((RtlpDphDebugLevel & DPH_DEBUG_NEVER_FREE)) {
        
        return TRUE;
    }
    
    //
    // Add block to the delayed free queue.
    //

    RtlpDphAddToDelayedFreeQueue (Info);
    
    //
    // If we are over the threshold we need to really free
    // some of the guys.
    //
    // (SilviuC): should make this threshold more fine tuned.
    //

    Success = TRUE;

    if (RtlpDphNeedToTrimDelayedFreeQueue(&TrimSize)) {

        RtlpDphTrimDelayedFreeQueue (TrimSize, Flags);
    }

    return Success;
}


PVOID
RtlpDphNormalHeapReAllocate (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    PVOID OldBlock,
    SIZE_T Size
    )
{
    PVOID Block;
    PDPH_BLOCK_INFORMATION Info;
    ULONG Hash;
    SIZE_T CopySize;
    ULONG Reason;

    BUMP_GLOBAL_COUNTER (DPH_COUNTER_NO_OF_NORMAL_REALLOCS);
    BUMP_SIZE_COUNTER (Size);

    Heap->Counter[DPH_COUNTER_NO_OF_NORMAL_REALLOCS] += 1;
    
    Info = (PDPH_BLOCK_INFORMATION)OldBlock - 1;

    if (! RtlpDphIsNormalHeapBlock(Heap, OldBlock, &Reason, TRUE)) {
        
        RtlpDphReportCorruptedBlock (OldBlock, Reason);

        return NULL;
    }

    //
    // Note that this operation will bump the counters for
    // normal allocations. Decided to leave this situation
    // as it is. 
    //

    Block = RtlpDphNormalHeapAllocate (Heap, Flags, Size);

    if (Block == NULL) {
        return NULL;
    }

    //
    // Copy old block stuff into the new block and then
    // free old block.
    //

    if (Size < Info->RequestedSize) {
        CopySize = Size;
    }
    else {
        CopySize = Info->RequestedSize;
    }

    RtlCopyMemory (Block, OldBlock, CopySize);

    //
    // Free the old guy.
    //

    RtlpDphNormalHeapFree (Heap, Flags, OldBlock);

    return Block;
}


SIZE_T
RtlpDphNormalHeapSize (
    PDPH_HEAP_ROOT Heap,
    ULONG Flags,
    PVOID Block
    )
{
    PDPH_BLOCK_INFORMATION Info;
    SIZE_T Result;
    ULONG Reason;

    Info = (PDPH_BLOCK_INFORMATION)Block - 1;

    if (! RtlpDphIsNormalHeapBlock(Heap, Block, &Reason, FALSE)) {
        
        //
        // We cannot stop here for a wrong block. 
        // The users might use this function to validate
        // if a block belongs to the heap or not. However 
        // they should use HeapValidate for that.
        //
        
#if DBG
        DbgPrint ("Page heap: warning: HeapSize called with "
            "invalid block @ %p (reason %0X) \n", Block, Reason);
#endif

        return (SIZE_T)-1;
    }

    Result = RtlSizeHeap (
        Heap->NormalHeap,
        Flags,
        Info); 

    if (Result == (SIZE_T)-1) {
        return Result;
    }
    else {
        return Result - sizeof(*Info) - USER_ALIGNMENT;
    }
}


BOOLEAN
RtlpDphNormalHeapSetUserFlags(
    IN PDPH_HEAP_ROOT Heap,
    IN ULONG Flags,
    IN PVOID Address,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    )
{
    BOOLEAN Success;
    ULONG Reason;

    if (! RtlpDphIsNormalHeapBlock(Heap, Address, &Reason, FALSE)) {
        
        RtlpDphReportCorruptedBlock (Address, Reason);

        return FALSE;
    }
    
    Success = RtlSetUserFlagsHeap (
        Heap->NormalHeap,
        Flags,
        (PDPH_BLOCK_INFORMATION)Address - 1,
        UserFlagsReset,
        UserFlagsSet);

    return Success;
}


BOOLEAN
RtlpDphNormalHeapSetUserValue(
    IN PDPH_HEAP_ROOT Heap,
    IN ULONG Flags,
    IN PVOID Address,
    IN PVOID UserValue
    )
{
    BOOLEAN Success;
    ULONG Reason;

    if (! RtlpDphIsNormalHeapBlock(Heap, Address, &Reason, FALSE)) {
        
        RtlpDphReportCorruptedBlock (Address, Reason);

        return FALSE;
    }
    
    Success = RtlSetUserValueHeap (
        Heap->NormalHeap,
        Flags,
        (PDPH_BLOCK_INFORMATION)Address - 1,
        UserValue);

    return Success;
}


BOOLEAN
RtlpDphNormalHeapGetUserInfo(
    IN PDPH_HEAP_ROOT Heap,
    IN  ULONG  Flags,
    IN  PVOID  Address,
    OUT PVOID* UserValue,
    OUT PULONG UserFlags
    )
{
    BOOLEAN Success;
    ULONG Reason;

    if (! RtlpDphIsNormalHeapBlock(Heap, Address, &Reason, FALSE)) {
        
        RtlpDphReportCorruptedBlock (Address, Reason);

        return FALSE;
    }
    
    Success = RtlGetUserInfoHeap (
        Heap->NormalHeap,
        Flags,
        (PDPH_BLOCK_INFORMATION)Address - 1,
        UserValue,
        UserFlags);

    return Success;
}


BOOLEAN
RtlpDphNormalHeapValidate(
    IN PDPH_HEAP_ROOT Heap,
    IN ULONG Flags,
    IN PVOID Address
    )
{
    BOOLEAN Success;
    ULONG Reason;

    if (Address == NULL) {
        
        //
        // Validation for the whole heap.
        //

        Success = RtlValidateHeap (
            Heap->NormalHeap,
            Flags,
            Address);
    }
    else {

        //
        // Validation for a heap block.
        //

        if (! RtlpDphIsNormalHeapBlock(Heap, Address, &Reason, TRUE)) {

            //
            // We cannot break in this case because the function might indeed
            // be called with invalid block. 
            //
            // (SilviuC): we  will leave this as a warning and delete it only
            // if it becomes annoying.
            //

#if DBG
            DbgPrint ("Page heap: warning: validate called with "
                      "invalid block @ %p (reason %0X) \n", Address, Reason);
#endif

            return FALSE;
        }

        Success = RtlValidateHeap (
            Heap->NormalHeap,
            Flags,
            (PDPH_BLOCK_INFORMATION)Address - 1);
    }

    return Success;
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////// Delayed free queue for normal heap
/////////////////////////////////////////////////////////////////////


RTL_CRITICAL_SECTION RtlpDphDelayedFreeQueueLock;

SIZE_T RtlpDphMemoryUsedByDelayedFreeBlocks;
SIZE_T RtlpDphNumberOfDelayedFreeBlocks;

LIST_ENTRY RtlpDphDelayedFreeQueue;

VOID
RtlpDphInitializeDelayedFreeQueue (
    )
{
    RtlInitializeCriticalSection (&RtlpDphDelayedFreeQueueLock);
    InitializeListHead (&RtlpDphDelayedFreeQueue);

    RtlpDphMemoryUsedByDelayedFreeBlocks = 0;
    RtlpDphNumberOfDelayedFreeBlocks = 0;
}


VOID
RtlpDphAddToDelayedFreeQueue (
    PDPH_BLOCK_INFORMATION Info
    )
{
    RtlEnterCriticalSection (&RtlpDphDelayedFreeQueueLock);

    InsertTailList (&(RtlpDphDelayedFreeQueue), &(Info->FreeQueue));
    
    RtlpDphMemoryUsedByDelayedFreeBlocks += Info->ActualSize;
    RtlpDphNumberOfDelayedFreeBlocks += 1;
    
    RtlLeaveCriticalSection (&RtlpDphDelayedFreeQueueLock);
}

BOOLEAN
RtlpDphNeedToTrimDelayedFreeQueue (
    PSIZE_T TrimSize
    )
{
    BOOLEAN Result;
    
    RtlEnterCriticalSection (&RtlpDphDelayedFreeQueueLock);

    if (RtlpDphMemoryUsedByDelayedFreeBlocks > RtlpDphDelayedFreeCacheSize) {
        
        *TrimSize = RtlpDphMemoryUsedByDelayedFreeBlocks - RtlpDphDelayedFreeCacheSize;

        if (*TrimSize < PAGE_SIZE) {
            *TrimSize = PAGE_SIZE;
        }

        Result = TRUE;
    }
    else {

        Result = FALSE;
    }

    RtlLeaveCriticalSection (&RtlpDphDelayedFreeQueueLock);
    return Result;
}

VOID
RtlpDphTrimDelayedFreeQueue (
    SIZE_T TrimSize,
    ULONG Flags
    )
/*++

Routine Description:

    This routine trims the delayed free queue (global per process).
    If trim size is zero it will trim up to a global threshold
    (RtlpDphDelayedFreeCacheSize) otherwise uses `TrimSize'. 
    
    Note. This function might become a little bit of a bottleneck
    because it is called by every free operation. Because of this
    it is better to always call RtlpDphNeedToTrimDelayedFreeQueue
    first.

Arguments:

    TrimSize: amount to trim (in bytes). If zero it trims down to
    a global threshold.
    
    Flags: flags for free operation.

Return Value:

    None.

Environment:

    Called from RtlpDphNormalXxx (normal heap management) routines.

--*/

{
    ULONG Reason;
    SIZE_T CurrentTrimmed = 0;
    PDPH_BLOCK_INFORMATION QueueBlock;
    PLIST_ENTRY ListEntry;

    RtlEnterCriticalSection (&RtlpDphDelayedFreeQueueLock);

    if (TrimSize == 0) {
        if (RtlpDphMemoryUsedByDelayedFreeBlocks > RtlpDphDelayedFreeCacheSize) {

            TrimSize = RtlpDphMemoryUsedByDelayedFreeBlocks - RtlpDphDelayedFreeCacheSize;
        }
    }

    while (TRUE) {
        
        //
        // Did we achieve our trimming goal?
        //

        if (CurrentTrimmed >= TrimSize) {
            break;  
        }
        
        //
        // The list can get empty since we remove blocks from it.
        //

        if (IsListEmpty(&RtlpDphDelayedFreeQueue)) {
            break;
        }

        ListEntry = RemoveHeadList (&RtlpDphDelayedFreeQueue);
        QueueBlock = CONTAINING_RECORD (ListEntry, DPH_BLOCK_INFORMATION, FreeQueue);

        if (! RtlpDphIsNormalFreeHeapBlock(QueueBlock + 1, &Reason, TRUE)) {

            RtlpDphReportCorruptedBlock (QueueBlock + 1, Reason);
        }

        RtlpDphMemoryUsedByDelayedFreeBlocks -= QueueBlock->ActualSize;
        RtlpDphNumberOfDelayedFreeBlocks -= 1;
        CurrentTrimmed += QueueBlock->ActualSize;

        RtlFreeHeap (((PDPH_HEAP_ROOT)(QueueBlock->Heap))->NormalHeap, Flags, QueueBlock); 
    }
    
    RtlLeaveCriticalSection (&RtlpDphDelayedFreeQueueLock);
}


VOID
RtlpDphFreeDelayedBlocksFromHeap (
    PVOID PageHeap,
    PVOID NormalHeap
    )
{
    ULONG Reason;
    PDPH_BLOCK_INFORMATION Block;
    PLIST_ENTRY Current;
    PLIST_ENTRY Next;

    RtlEnterCriticalSection (&RtlpDphDelayedFreeQueueLock);

    for (Current = RtlpDphDelayedFreeQueue.Flink;
         Current != &RtlpDphDelayedFreeQueue;
         Current = Next) {
        
        Next = Current->Flink;
        
        Block = CONTAINING_RECORD (Current, DPH_BLOCK_INFORMATION, FreeQueue);

        if (Block->Heap != PageHeap) {
            continue;
        }
        
        //
        // We need to delete this block;
        //

        RemoveEntryList (Current);
        Block = CONTAINING_RECORD (Current, DPH_BLOCK_INFORMATION, FreeQueue);

        //
        // Prevent probing of this field during RtlpDphIsNormalFreeBlock.
        //

        Block->Heap = 0;

        //
        // Check if the block about to be freed was touched.
        //

        if (! RtlpDphIsNormalFreeHeapBlock(Block + 1, &Reason, TRUE)) {

            RtlpDphReportCorruptedBlock (Block + 1, Reason);
        }

        RtlpDphMemoryUsedByDelayedFreeBlocks -= Block->ActualSize;
        RtlpDphNumberOfDelayedFreeBlocks -= 1;

        //
        // (SilviuC): Not sure what flags to use here. Zero should work
        // but I have to investigate.
        //

        RtlFreeHeap (NormalHeap, 0, Block); 
    }
    
    RtlLeaveCriticalSection (&RtlpDphDelayedFreeQueueLock);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stack trace detection
/////////////////////////////////////////////////////////////////////

PRTL_TRACE_BLOCK 
RtlpDphLogStackTrace (
    ULONG FramesToSkip
    )
{
    PVOID Trace [DPH_MAX_STACK_LENGTH];
    ULONG Hash;
    ULONG Count;
    PRTL_TRACE_BLOCK Block;
    BOOLEAN Result;

    Count = RtlCaptureStackBackTrace (
        1 + FramesToSkip, 
        DPH_MAX_STACK_LENGTH, 
        Trace, 
        &Hash);

    if (Count == 0 || RtlpDphTraceDatabase == NULL) {
        return NULL;
    }

    Result = RtlTraceDatabaseAdd (
        RtlpDphTraceDatabase,
        Count,
        Trace,
        &Block);

    if (Result == FALSE) {
        return NULL;
    }
    else {
        return Block;
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Target dlls logic
/////////////////////////////////////////////////////////////////////

RTL_CRITICAL_SECTION RtlpDphTargetDllsLock;
LIST_ENTRY RtlpDphTargetDllsList;
BOOLEAN RtlpDphTargetDllsInitialized;

typedef struct _DPH_TARGET_DLL {

    LIST_ENTRY List;
    UNICODE_STRING Name;
    PVOID StartAddress;
    PVOID EndAddress;

} DPH_TARGET_DLL, * PDPH_TARGET_DLL;

VOID
RtlpDphTargetDllsLogicInitialize (
    )
{
    RtlInitializeCriticalSection (&RtlpDphTargetDllsLock);
    InitializeListHead (&RtlpDphTargetDllsList);
    RtlpDphTargetDllsInitialized = TRUE;
}

VOID
RtlpDphTargetDllsLoadCallBack (
    PUNICODE_STRING Name,
    PVOID Address,
    ULONG Size
    )
//
// This function is not called right now but it will get called
// from \base\ntdll\ldrapi.c whenever a dll gets loaded. This
// gives page heap the opportunity to update per dll data structures
// that are not used right now for anything.
//
{
    PDPH_TARGET_DLL Descriptor;

    //
    // Get out if we are in some weird condition.
    //

    if (! RtlpDphTargetDllsInitialized) {
        return;
    }

    if (! RtlpDphIsDllTargeted (Name->Buffer)) {
        return;
    }

    Descriptor = RtlAllocateHeap (RtlProcessHeap(), 0, sizeof *Descriptor);

    if (Descriptor == NULL) {
        return;
    }

    if (! RtlCreateUnicodeString (&(Descriptor->Name), Name->Buffer)) {
        RtlFreeHeap (RtlProcessHeap(), 0, Descriptor);
        return;
    }

    Descriptor->StartAddress = Address;
    Descriptor->EndAddress = (PUCHAR)Address + Size;

    RtlEnterCriticalSection (&RtlpDphTargetDllsLock);
    InsertTailList (&(RtlpDphTargetDllsList), &(Descriptor->List));
    RtlLeaveCriticalSection (&RtlpDphTargetDllsLock);

    //
    // SilviuC: This message should be printed only if a target
    // dll has been identified.
    //

    DbgPrint("Page heap: loaded target dll %ws [%p - %p]\n", 
             Descriptor->Name.Buffer,
             Descriptor->StartAddress,
             Descriptor->EndAddress);
}

const WCHAR *
RtlpDphIsDllTargeted (
    const WCHAR * Name
    )
{
    const WCHAR * All;
    ULONG I, J;

    All = RtlpDphTargetDllsUnicode.Buffer;

    for (I = 0; All[I]; I += 1) {

        for (J = 0; All[I+J] && Name[J]; J += 1) {
            if (RtlUpcaseUnicodeChar(All[I+J]) != RtlUpcaseUnicodeChar(Name[J])) {
                break;
            }
        }

        if (Name[J]) {
            continue;
        }
        else {
            // we got to the end of string
            return &(All[I]);
        }
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Validation checks
/////////////////////////////////////////////////////////////////////

PDPH_HEAP_BLOCK
RtlpDphSearchBlockInList (
    PDPH_HEAP_BLOCK List,
    PUCHAR Address
    )
{
    PDPH_HEAP_BLOCK Current;

    for (Current = List; Current; Current = Current->pNextAlloc) {
        if (Current->pVirtualBlock == Address) {
            return Current;
        }
    }

    return NULL;
}

PVOID RtlpDphLastValidationStack;
PVOID RtlpDphCurrentValidationStack;

VOID
RtlpDphInternalValidatePageHeap (
    PDPH_HEAP_ROOT Heap,
    PUCHAR ExemptAddress,
    SIZE_T ExemptSize
    )
{
    PDPH_HEAP_BLOCK Range;
    PDPH_HEAP_BLOCK Node;
    PUCHAR Address;
    BOOLEAN FoundLeak;

    RtlpDphLastValidationStack = RtlpDphCurrentValidationStack;
    RtlpDphCurrentValidationStack = RtlpDphLogStackTrace (0);
    FoundLeak = FALSE;

    for (Range = Heap->pVirtualStorageListHead;
         Range != NULL;
         Range = Range->pNextAlloc) {

        Address = Range->pVirtualBlock;

        while (Address < Range->pVirtualBlock + Range->nVirtualBlockSize) {

            //
            // Ignore DPH_HEAP_ROOT structures.
            //

            if ((Address >= (PUCHAR)Heap - PAGE_SIZE) && (Address <  (PUCHAR)Heap + 5 * PAGE_SIZE)) {
                Address += PAGE_SIZE;
                continue;
            }
            
            //
            // Ignore exempt region (temporarily out of all structures).
            //

            if ((Address >= ExemptAddress) && (Address < ExemptAddress + ExemptSize)) {
                Address += PAGE_SIZE;
                continue;
            }

            Node = RtlpDphSearchBlockInList (Heap->pBusyAllocationListHead, Address);

            if (Node) {
                Address += Node->nVirtualBlockSize;
                continue;
            }
            
            Node = RtlpDphSearchBlockInList (Heap->pFreeAllocationListHead, Address);

            if (Node) {
                Address += Node->nVirtualBlockSize;
                continue;
            }
            
            Node = RtlpDphSearchBlockInList (Heap->pAvailableAllocationListHead, Address);

            if (Node) {
                Address += Node->nVirtualBlockSize;
                continue;
            }
            
            Node = RtlpDphSearchBlockInList (Heap->pNodePoolListHead, Address);

            if (Node) {
                Address += Node->nVirtualBlockSize;
                continue;
            }

            DbgPrint ("Block @ %p has been leaked \n", Address);
            FoundLeak = TRUE;

            Address += PAGE_SIZE;
        }
    }

    if (FoundLeak) {

        DbgPrint ("Page heap: Last stack @ %p, Current stack @ %p \n", 
            RtlpDphLastValidationStack,
            RtlpDphCurrentValidationStack);

        DbgBreakPoint ();
    }
}


#endif // DEBUG_PAGE_HEAP

//
// End of module
//
