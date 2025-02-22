/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    rtlfuncs.h

Abstract:

    Function definitions for the Run-Time Library

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _RTLFUNCS_H
#define _RTLFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <ntnls.h>
#include <rtltypes.h>
#include <pstypes.h>
#include <extypes.h>
#include "in6addr.h"
#include "inaddr.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NTOS_MODE_USER

//
// List Functions
//
FORCEINLINE
VOID
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

FORCEINLINE
VOID
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY OldFlink;
    OldFlink = ListHead->Flink;
    Entry->Flink = OldFlink;
    Entry->Blink = ListHead;
    OldFlink->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
VOID
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY OldBlink;
    OldBlink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = OldBlink;
    OldBlink->Flink = Entry;
    ListHead->Blink = Entry;
}

_Must_inspect_result_
FORCEINLINE
BOOLEAN
IsListEmpty(
    _In_ const LIST_ENTRY * ListHead
)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead
)
{
    PSINGLE_LIST_ENTRY FirstEntry;
    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL) {
        ListHead->Next = FirstEntry->Next;
    }

    return FirstEntry;
}

FORCEINLINE
VOID
PushEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead,
    _Inout_ PSINGLE_LIST_ENTRY Entry
)
{
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
}

FORCEINLINE
BOOLEAN
RemoveEntryList(
    _In_ PLIST_ENTRY Entry)
{
    PLIST_ENTRY OldFlink;
    PLIST_ENTRY OldBlink;

    OldFlink = Entry->Flink;
    OldBlink = Entry->Blink;
    OldFlink->Blink = OldBlink;
    OldBlink->Flink = OldFlink;
    return (BOOLEAN)(OldFlink == OldBlink);
}

FORCEINLINE
PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

FORCEINLINE
PLIST_ENTRY
RemoveTailList(
    _Inout_ PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

//
// Unicode string macros
//
_At_(UnicodeString->Buffer, _Post_equal_to_(Buffer))
_At_(UnicodeString->Length, _Post_equal_to_(0))
_At_(UnicodeString->MaximumLength, _Post_equal_to_(BufferSize))
FORCEINLINE
VOID
RtlInitEmptyUnicodeString(
    _Out_ PUNICODE_STRING UnicodeString,
    _When_(BufferSize != 0, _Notnull_) _Writable_bytes_(BufferSize) __drv_aliasesMem PWCHAR Buffer,
    _In_ USHORT BufferSize)
{
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = BufferSize;
    UnicodeString->Buffer = Buffer;
}

_At_(AnsiString->Buffer, _Post_equal_to_(Buffer))
_At_(AnsiString->Length, _Post_equal_to_(0))
_At_(AnsiString->MaximumLength, _Post_equal_to_(BufferSize))
FORCEINLINE
VOID
RtlInitEmptyAnsiString(
    _Out_ PANSI_STRING AnsiString,
    _When_(BufferSize != 0, _Notnull_) _Writable_bytes_(BufferSize) __drv_aliasesMem PCHAR Buffer,
    _In_ USHORT BufferSize)
{
    AnsiString->Length = 0;
    AnsiString->MaximumLength = BufferSize;
    AnsiString->Buffer = Buffer;
}

//
// LUID Macros
//
#define RtlEqualLuid(L1, L2) (((L1)->HighPart == (L2)->HighPart) && \
                              ((L1)->LowPart  == (L2)->LowPart))
FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(
    _In_ ULONG Ulong)
{
    LUID TempLuid;

    TempLuid.LowPart = Ulong;
    TempLuid.HighPart = 0;
    return TempLuid;
}

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8ToUnicodeN(
    _Out_writes_bytes_to_(UnicodeStringMaxByteCount, *UnicodeStringActualByteCount)
        PWSTR UnicodeStringDestination,
    _In_ ULONG UnicodeStringMaxByteCount,
    _Out_ PULONG UnicodeStringActualByteCount,
    _In_reads_bytes_(UTF8StringByteCount) PCCH UTF8StringSource,
    _In_ ULONG UTF8StringByteCount);

//
// ASSERT Macros
//
#ifndef ASSERT
#if DBG

#define ASSERT( exp ) \
    ((void)((!(exp)) ? \
        (RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, NULL ),FALSE) : \
        TRUE))

#define ASSERTMSG( msg, exp ) \
    ((void)((!(exp)) ? \
        (RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, (PCHAR)msg ),FALSE) : \
        TRUE))

#else

#define ASSERT( exp )         ((void) 0)
#define ASSERTMSG( msg, exp ) ((void) 0)

#endif
#endif

#ifdef NTOS_KERNEL_RUNTIME

//
// Executing RTL functions at DISPATCH_LEVEL or higher will result in a
// bugcheck.
//
#define RTL_PAGED_CODE PAGED_CODE

#else

//
// This macro does nothing in user mode
//
#define RTL_PAGED_CODE()

#endif

//
// RTL Splay Tree Functions
//
#ifndef RTL_USE_AVL_TABLES

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable(
    _Out_ PRTL_GENERIC_TABLE Table,
    _In_ PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    _In_opt_ PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    _In_opt_ PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ BOOLEAN Restart
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(
    _In_ PRTL_GENERIC_TABLE Table,
    _Inout_ PVOID *RestartKey
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ ULONG I
);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
    _In_ PRTL_GENERIC_TABLE Table
);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(
    _In_ PRTL_GENERIC_TABLE Table
);

#endif /* !RTL_USE_AVL_TABLES */

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(
    _Inout_ PRTL_SPLAY_LINKS Links
);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(
    _In_ PRTL_SPLAY_LINKS Links
);

NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay(
    _In_ PRTL_SPLAY_LINKS Links,
    _Inout_ PRTL_SPLAY_LINKS *Root
);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(
    _In_ PRTL_SPLAY_LINKS Links
);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(
    _In_ PRTL_SPLAY_LINKS Links
);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(
    _In_ PRTL_SPLAY_LINKS Links
);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(
    _In_ PRTL_SPLAY_LINKS Links
);

#define RtlIsLeftChild(Links) \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlIsRightChild(Links) \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlRightChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->RightChild

#define RtlIsRoot(Links) \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links))

#define RtlLeftChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->LeftChild

#define RtlParent(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->Parent

// FIXME: use inline function

#define RtlInitializeSplayLinks(Links)                  \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayLinks;                   \
        _SplayLinks = (PRTL_SPLAY_LINKS)(Links);        \
        _SplayLinks->Parent = _SplayLinks;              \
        _SplayLinks->LeftChild = NULL;                  \
        _SplayLinks->RightChild = NULL;                 \
    }

#define RtlInsertAsLeftChild(ParentLinks,ChildLinks)    \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->LeftChild = _SplayChild;          \
        _SplayChild->Parent = _SplayParent;             \
    }

#define RtlInsertAsRightChild(ParentLinks,ChildLinks)   \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->RightChild = _SplayChild;         \
        _SplayChild->Parent = _SplayParent;             \
    }

//
// RTL AVL Tree Functions
//
NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl(
    _Out_ PRTL_AVL_TABLE Table,
    _In_ PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    _In_opt_ PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    _In_opt_ PRTL_AVL_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ BOOLEAN Restart
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(
    _In_ PRTL_AVL_TABLE Table,
    _Inout_ PVOID *RestartKey
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *RestartKey
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory(
    _In_ PRTL_AVL_TABLE Table,
    _In_opt_ PRTL_AVL_MATCH_FUNCTION MatchFunction,
    _In_opt_ PVOID MatchData,
    _In_ ULONG NextFlag,
    _Inout_ PVOID *RestartKey,
    _Inout_ PULONG DeleteCount,
    _In_ PVOID Buffer
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ ULONG I
);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(
    _In_ PRTL_AVL_TABLE Table
);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(
    _In_ PRTL_AVL_TABLE Table
);

#ifdef RTL_USE_AVL_TABLES

#define RtlInitializeGenericTable               RtlInitializeGenericTableAvl
#define RtlInsertElementGenericTable            RtlInsertElementGenericTableAvl
#define RtlInsertElementGenericTableFull        RtlInsertElementGenericTableFullAvl
#define RtlDeleteElementGenericTable            RtlDeleteElementGenericTableAvl
#define RtlLookupElementGenericTable            RtlLookupElementGenericTableAvl
#define RtlLookupElementGenericTableFull        RtlLookupElementGenericTableFullAvl
#define RtlEnumerateGenericTable                RtlEnumerateGenericTableAvl
#define RtlEnumerateGenericTableWithoutSplaying RtlEnumerateGenericTableWithoutSplayingAvl
#define RtlGetElementGenericTable               RtlGetElementGenericTableAvl
#define RtlNumberGenericTableElements           RtlNumberGenericTableElementsAvl
#define RtlIsGenericTableEmpty                  RtlIsGenericTableEmptyAvl

#endif /* RTL_USE_AVL_TABLES */

//
// Exception and Error Functions
//
NTSYSAPI
PVOID
NTAPI
RtlAddVectoredExceptionHandler(
    _In_ ULONG FirstHandler,
    _In_ PVECTORED_EXCEPTION_HANDLER VectoredHandler
);

NTSYSAPI
ULONG
NTAPI
RtlRemoveVectoredExceptionHandler(
    _In_ PVOID VectoredHandlerHandle
);

NTSYSAPI
PVOID
NTAPI
RtlAddVectoredContinueHandler(
    _In_ ULONG FirstHandler,
    _In_ PVECTORED_EXCEPTION_HANDLER VectoredHandler
);

NTSYSAPI
ULONG
NTAPI
RtlRemoveVectoredContinueHandler(
    _In_ PVOID VectoredHandlerHandle
);

NTSYSAPI
VOID
NTAPI
RtlSetUnhandledExceptionFilter(
    _In_ PRTLP_UNHANDLED_EXCEPTION_FILTER TopLevelExceptionFilter
);

NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter(
    _In_ struct _EXCEPTION_POINTERS* ExceptionInfo
);

__analysis_noreturn
NTSYSAPI
VOID
NTAPI
RtlAssert(
    _In_ PVOID FailedAssertion,
    _In_ PVOID FileName,
    _In_ ULONG LineNumber,
    _In_opt_z_ PCHAR Message
);

NTSYSAPI
PVOID
NTAPI
RtlEncodePointer(
    _In_ PVOID Pointer
);

NTSYSAPI
PVOID
NTAPI
RtlDecodePointer(
    _In_ PVOID Pointer
);

NTSYSAPI
PVOID
NTAPI
RtlEncodeSystemPointer(
    _In_ PVOID Pointer
);

NTSYSAPI
PVOID
NTAPI
RtlDecodeSystemPointer(
    _In_ PVOID Pointer
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLastNtStatus(
    VOID
);

NTSYSAPI
ULONG
NTAPI
RtlGetLastWin32Error(
    VOID
);

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32Error(
    _In_ ULONG LastError
);

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32ErrorAndNtStatusFromNtStatus(
    _In_ NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadErrorMode(
    _In_ ULONG NewMode,
    _Out_opt_ PULONG OldMode
);

NTSYSAPI
ULONG
NTAPI
RtlGetThreadErrorMode(
    VOID
);

#endif /* NTOS_MODE_USER */

NTSYSAPI
VOID
NTAPI
RtlCaptureContext(
    _Out_ PCONTEXT ContextRecord
);

NTSYSAPI
BOOLEAN
NTAPI
RtlDispatchException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT Context
);

_IRQL_requires_max_(APC_LEVEL)
_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
    _In_ NTSTATUS Status
);

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
    _In_ NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
RtlMapSecurityErrorToNtStatus(
    _In_ ULONG SecurityError
);

NTSYSAPI
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord
);

DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlRaiseStatus(
    _In_ NTSTATUS Status
);

NTSYSAPI
VOID
NTAPI
RtlUnwind(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue
);

#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT 8

#ifdef _M_AMD64

NTSYSAPI
PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionEntry(
    _In_ DWORD64 ControlPc,
    _Out_ PDWORD64 ImageBase,
    _Inout_opt_ PUNWIND_HISTORY_TABLE HistoryTable
);

NTSYSAPI
PEXCEPTION_ROUTINE
NTAPI
RtlVirtualUnwind(
    _In_ ULONG HandlerType,
    _In_ ULONG64 ImageBase,
    _In_ ULONG64 ControlPc,
    _In_ PRUNTIME_FUNCTION FunctionEntry,
    _Inout_ PCONTEXT Context,
    _Outptr_ PVOID* HandlerData,
    _Out_ PULONG64 EstablisherFrame,
    _Inout_opt_ PKNONVOLATILE_CONTEXT_POINTERS ContextPointers
);

#endif // _M_AMD64

//
// Tracing Functions
//
NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
    _Out_writes_(Count - (Flags >> RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT)) PVOID *Callers,
    _In_ ULONG Count,
    _In_ ULONG Flags
);

NTSYSAPI
USHORT
NTAPI
RtlLogStackBackTrace(
    VOID
);

#ifdef NTOS_MODE_USER
//
// Heap Functions
//
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
    _In_ ULONG Flags,
    _In_opt_ PVOID BaseAddress,
    _In_opt_ SIZE_T SizeToReserve,
    _In_opt_ SIZE_T SizeToCommit,
    _In_opt_ PVOID Lock,
    _In_opt_ PRTL_HEAP_PARAMETERS Parameters
);

NTSYSAPI
ULONG
NTAPI
RtlCreateTagHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_opt_ PWSTR TagName,
    _In_ PWSTR TagSubName
);

ULONG
NTAPI
RtlCompactHeap(
    _In_ HANDLE Heap,
    _In_ ULONG Flags
);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlDebugCreateHeap(
    _In_ ULONG Flags,
    _In_opt_ PVOID BaseAddress,
    _In_opt_ SIZE_T SizeToReserve,
    _In_opt_ SIZE_T SizeToCommit,
    _In_opt_ PVOID Lock,
    _In_opt_ PRTL_HEAP_PARAMETERS Parameters
);

NTSYSAPI
HANDLE
NTAPI
RtlDestroyHeap(
    _In_ _Post_invalid_ HANDLE Heap
);

NTSYSAPI
ULONG
NTAPI
RtlExtendHeap(
    _In_ HANDLE Heap,
    _In_ ULONG Flags,
    _In_ PVOID P,
    _In_ SIZE_T Size
);

_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    _In_ HANDLE HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ _Post_invalid_ PVOID P
);

ULONG
NTAPI
RtlGetProcessHeaps(
    _In_ ULONG HeapCount,
    _Out_cap_(HeapCount) HANDLE *HeapArray
);

_Success_(return != 0)
BOOLEAN
NTAPI
RtlGetUserInfoHeap(
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _Inout_opt_ PVOID *UserValue,
    _Out_opt_ PULONG UserFlags
);

NTSYSAPI
PVOID
NTAPI
RtlProtectHeap(
    _In_ PVOID HeapHandle,
    _In_ BOOLEAN Protect
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryHeapInformation(
    _In_ PVOID HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _Out_ PVOID HeapInformation,
    _In_ SIZE_T HeapInformationLength,
    _When_(HeapInformationClass==HeapCompatibilityInformation, _On_failure_(_Out_opt_))
        _Out_opt_ PSIZE_T ReturnLength
);

_Ret_opt_z_
NTSYSAPI
PWSTR
NTAPI
RtlQueryTagHeap(
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ USHORT TagIndex,
    _In_ BOOLEAN ResetCounters,
    _Out_ PRTL_HEAP_TAG_INFO HeapTagInfo
);

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
NTSYSAPI
PVOID
NTAPI
RtlReAllocateHeap(
    _In_ HANDLE Heap,
    _In_opt_ ULONG Flags,
    _In_ _Post_invalid_ PVOID Ptr,
    _In_ SIZE_T Size
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetHeapInformation(
    _In_ PVOID HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _When_(HeapInformationClass==HeapCompatibilityInformation,_In_) PVOID HeapInformation,
    _In_ SIZE_T HeapInformationLength
);

NTSYSAPI
BOOLEAN
NTAPI
RtlLockHeap(
    _In_ HANDLE Heap
);

NTSYSAPI
ULONG
NTAPI
RtlMultipleAllocateHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ SIZE_T Size,
    _In_ ULONG Count,
    _Out_cap_(Count) _Deref_post_bytecap_(Size) PVOID * Array
);

NTSYSAPI
ULONG
NTAPI
RtlMultipleFreeHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ ULONG Count,
    _In_count_(Count) /* _Deref_ _Post_invalid_ */ PVOID * Array
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUsageHeap(
    _In_ HANDLE Heap,
    _In_ ULONG Flags,
    _Out_ PRTL_HEAP_USAGE Usage
);

NTSYSAPI
BOOLEAN
NTAPI
RtlUnlockHeap(
    _In_ HANDLE Heap
);

BOOLEAN
NTAPI
RtlSetUserValueHeap(
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ PVOID UserValue
);

BOOLEAN
NTAPI
RtlSetUserFlagsHeap(
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ ULONG UserFlagsReset,
    _In_ ULONG UserFlagsSet
);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateHeap(
    _In_ HANDLE Heap,
    _In_ ULONG Flags,
    _In_opt_ PVOID P
);

NTSYSAPI
NTSTATUS
NTAPI
RtlWalkHeap(
    _In_ HANDLE HeapHandle,
    _In_ PVOID HeapEntry
);

#define RtlGetProcessHeap() (NtCurrentPeb()->ProcessHeap)

#endif // NTOS_MODE_USER

#define NtCurrentPeb() (NtCurrentTeb()->ProcessEnvironmentBlock)

NTSYSAPI
SIZE_T
NTAPI
RtlSizeHeap(
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID MemoryPointer
);


//
// Security Functions
//
_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Inout_ PULONG BufferLength
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAceEx(
    _Inout_ PACL pAcl,
    _In_ ULONG dwAceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID pSid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedObjectAce(
    _Inout_ PACL pAcl,
    _In_ ULONG dwAceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ GUID *ObjectTypeGuid,
    _In_opt_ GUID *InheritedObjectTypeGuid,
    _In_ PSID pSid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAce(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAceEx(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedObjectAce(
    _Inout_ PACL pAcl,
    _In_ ULONG dwAceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ GUID *ObjectTypeGuid,
    _In_opt_ GUID *InheritedObjectTypeGuid,
    _In_ PSID pSid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG StartingAceIndex,
    _In_reads_bytes_(AceListLength) PVOID AceList,
    _In_ ULONG AceListLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAce(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid,
    _In_ BOOLEAN Success,
    _In_ BOOLEAN Failure
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAcquirePrivilege(
    _In_ PULONG Privilege,
    _In_ ULONG NumPriv,
    _In_ ULONG Flags,
    _Out_ PVOID *ReturnedState
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAceEx(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid,
    _In_ BOOLEAN Success,
    _In_ BOOLEAN Failure
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessObjectAce(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ GUID *ObjectTypeGuid,
    _In_opt_ GUID *InheritedObjectTypeGuid,
    _In_ PSID Sid,
    _In_ BOOLEAN Success,
    _In_ BOOLEAN Failure
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddMandatoryAce(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ULONG Flags,
    _In_ ULONG MandatoryFlags,
    _In_ UCHAR AceType,
    _In_ PSID LabelSid);

NTSYSAPI
NTSTATUS
NTAPI
RtlAdjustPrivilege(
    _In_ ULONG Privilege,
    _In_ BOOLEAN NewValue,
    _In_ BOOLEAN ForThread,
    _Out_ PBOOLEAN OldValue
);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateAndInitializeSid(
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount,
    _In_ ULONG SubAuthority0,
    _In_ ULONG SubAuthority1,
    _In_ ULONG SubAuthority2,
    _In_ ULONG SubAuthority3,
    _In_ ULONG SubAuthority4,
    _In_ ULONG SubAuthority5,
    _In_ ULONG SubAuthority6,
    _In_ ULONG SubAuthority7,
    _Outptr_ PSID *Sid
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreAllAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreAnyAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlCopyLuid (
    _Out_ PLUID DestinationLuid,
    _In_ PLUID SourceLuid
    );

NTSYSAPI
VOID
NTAPI
RtlCopyLuidAndAttributesArray(
    ULONG Count,
    PLUID_AND_ATTRIBUTES Src,
    PLUID_AND_ATTRIBUTES Dest
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySidAndAttributesArray(
    _In_ ULONG Count,
    _In_ PSID_AND_ATTRIBUTES Src,
    _In_ ULONG SidAreaSize,
    _In_ PSID_AND_ATTRIBUTES Dest,
    _In_ PSID SidArea,
    _Out_ PSID* RemainingSidArea,
    _Out_ PULONG RemainingSidAreaSize
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
    _Inout_ PUNICODE_STRING UnicodeString,
    _In_ PSID Sid,
    _In_ BOOLEAN AllocateDestinationString
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
    _In_ ULONG DestinationSidLength,
    _Out_writes_bytes_(DestinationSidLength) PSID DestinationSid,
    _In_ PSID SourceSid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
    PACL Acl,
    ULONG AclSize,
    ULONG AclRevision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptorRelative(
    _Out_ PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    _In_ ULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR pSourceSecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *pDestinationSecurityDescriptor
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
    PACL Acl,
    ULONG AceIndex
);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualPrefixSid(
    PSID Sid1,
    PSID Sid2
);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid (
    _In_ PSID Sid1,
    _In_ PSID Sid2
);

NTSYSAPI
BOOLEAN
NTAPI
RtlFirstFreeAce(
    PACL Acl,
    PACE* Ace
);

NTSYSAPI
PVOID
NTAPI
RtlFreeSid(
    _In_ _Post_invalid_ PSID Sid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
    PACL Acl,
    ULONG AceIndex,
    PVOID *Ace
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetControlSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR_CONTROL Control,
    _Out_ PULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN DaclPresent,
    _Out_ PACL *Dacl,
    _Out_ PBOOLEAN DaclDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN SaclPresent,
    _Out_ PACL* Sacl,
    _Out_ PBOOLEAN SaclDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PSID *Group,
    _Out_ PBOOLEAN GroupDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PSID *Owner,
    _Out_ PBOOLEAN OwnerDefaulted
);

NTSYSAPI
BOOLEAN
NTAPI
RtlGetSecurityDescriptorRMControl(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PUCHAR RMControl
);

NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlImpersonateSelf(IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
    _Out_ PSID Sid,
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount
);

NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(IN ULONG SubAuthorityCount);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlLengthSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
ULONG
NTAPI
RtlLengthSid(IN PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlMakeSelfRelativeSD(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSD,
    _Out_ PSECURITY_DESCRIPTOR SelfRelativeSD,
    _Inout_ PULONG BufferLength);

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping
);

#ifdef NTOS_MODE_USER

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationAcl(
    PACL Acl,
    PVOID Information,
    ULONG InformationLength,
    ACL_INFORMATION_CLASS InformationClass
);

#endif

NTSYSAPI
VOID
NTAPI
RtlReleasePrivilege(
    _In_ PVOID ReturnedState
);

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSYSAPI
NTSTATUS
NTAPI
RtlRemovePrivileges(
    _In_ HANDLE TokenHandle,
    _In_reads_opt_(PrivilegeCount) _When_(PrivilegeCount != 0, _Notnull_)
         PULONG PrivilegesToKeep,
    _In_ ULONG PrivilegeCount
);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
    _In_ PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Out_writes_bytes_to_opt_(*AbsoluteSecurityDescriptorSize, *AbsoluteSecurityDescriptorSize) PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Inout_ PULONG AbsoluteSecurityDescriptorSize,
    _Out_writes_bytes_to_opt_(*DaclSize, *DaclSize) PACL Dacl,
    _Inout_ PULONG DaclSize,
    _Out_writes_bytes_to_opt_(*SaclSize, *SaclSize) PACL Sacl,
    _Inout_ PULONG SaclSize,
    _Out_writes_bytes_to_opt_(*OwnerSize, *OwnerSize) PSID Owner,
    _Inout_ PULONG OwnerSize,
    _Out_writes_bytes_to_opt_(*PrimaryGroupSize, *PrimaryGroupSize) PSID PrimaryGroup,
    _Inout_ PULONG PrimaryGroupSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD2(
    _Inout_ PSECURITY_DESCRIPTOR SelfRelativeSD,
    _Out_ PULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetAttributesSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_DESCRIPTOR_CONTROL Control,
    _Out_ PULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetControlSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN DaclPresent,
    _In_opt_ PACL Dacl,
    _In_opt_ BOOLEAN DaclDefaulted
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID Group,
    _In_opt_ BOOLEAN GroupDefaulted
);

#ifdef NTOS_MODE_USER

NTSYSAPI
NTSTATUS
NTAPI
RtlSetInformationAcl(
    PACL Acl,
    PVOID Information,
    ULONG InformationLength,
    ACL_INFORMATION_CLASS InformationClass
);

#endif

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID Owner,
    _In_opt_ BOOLEAN OwnerDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN SaclPresent,
    _In_ PACL Sacl,
    _In_ BOOLEAN SaclDefaulted
);

NTSYSAPI
VOID
NTAPI
RtlSetSecurityDescriptorRMControl(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PUCHAR RMControl
);

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
    _In_ PSID Sid
);

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
    _In_ PSID Sid,
    _In_ ULONG SubAuthority
);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
    _In_reads_bytes_(SecurityDescriptorLength) PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    _In_ ULONG SecurityDescriptorLength,
    _In_ SECURITY_INFORMATION RequiredInformation
);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(IN PSID Sid);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidAcl(PACL Acl);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteSecurityObject(
    _In_ PSECURITY_DESCRIPTOR *ObjectDescriptor
);

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObject(
    _In_ PSECURITY_DESCRIPTOR ParentDescriptor,
    _In_ PSECURITY_DESCRIPTOR CreatorDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ HANDLE Token,
    _In_ PGENERIC_MAPPING GenericMapping
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQuerySecurityObject(
    _In_ PSECURITY_DESCRIPTOR ObjectDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR ResultantDescriptor,
    _In_ ULONG DescriptorLength,
    _Out_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSecurityObject(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR ModificationDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ HANDLE Token
);

//
// Single-Character Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlLargeIntegerToChar(
    _In_ PLARGE_INTEGER Value,
    _In_ ULONG Base,
    _In_ ULONG Length,
    _Out_ PCHAR String
);

NTSYSAPI
CHAR
NTAPI
RtlUpperChar(CHAR Source);

NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(WCHAR Source);

NTSYSAPI
WCHAR
NTAPI
RtlDowncaseUnicodeChar(IN WCHAR Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToChar(
    _In_ ULONG Value,
    _In_ ULONG Base,
    _In_ ULONG Length,
    _Out_ PCHAR String
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicode(
    _In_ ULONG Value,
    _In_opt_ ULONG Base,
    _In_opt_ ULONG Length,
    _Inout_ LPWSTR String
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_At_(String->MaximumLength, _Const_)
NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
    _In_ ULONG Value,
    _In_opt_ ULONG Base,
    _Inout_ PUNICODE_STRING String
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
    PCSZ String,
    ULONG Base,
    PULONG Value
);

//
// Byte Swap Functions
//
#ifdef NTOS_MODE_USER

unsigned short __cdecl _byteswap_ushort(unsigned short);
unsigned long  __cdecl _byteswap_ulong (unsigned long);
unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64);
#ifdef _MSC_VER
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)
#endif // _MSC_VER
#define RtlUshortByteSwap(_x) _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))

#endif // NTOS_MODE_USER

//
// Unicode->Ansi String Functions
//
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(IN PCUNICODE_STRING UnicodeString);

#ifdef NTOS_MODE_USER

#define RtlUnicodeStringToAnsiSize(STRING) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(STRING) :                     \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

//
// Unicode->OEM String Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
//_At_(DestinationString->Buffer, _Post_bytecount_(DestinationString->Length))
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
        POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToCountedOemString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
        POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToOemN(
    PCHAR OemString,
    ULONG OemSize,
    PULONG ResultSize,
    PCWCH UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToOemSize(IN PCUNICODE_STRING UnicodeString);

#ifdef NTOS_MODE_USER

#define RtlUnicodeStringToOemSize(STRING) (                             \
    NLS_MB_OEM_CODE_PAGE_TAG ?                                          \
    RtlxUnicodeStringToOemSize(STRING) :                                \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR)           \
)

#define RtlUnicodeStringToCountedOemSize(STRING) (                      \
    (ULONG)(RtlUnicodeStringToOemSize(STRING) - sizeof(ANSI_NULL))      \
)

#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToOemN(
    PCHAR OemString,
    ULONG OemSize,
    PULONG ResultSize,
    PCWCH UnicodeString,
    ULONG UnicodeSize
);

//
// Unicode->MultiByte String Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
    PCHAR MbString,
    ULONG MbSize,
    PULONG ResultSize,
    PCWCH UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToMultiByteN(
    PCHAR MbString,
    ULONG MbSize,
    PULONG ResultSize,
    PCWCH UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
    PULONG MbSize,
    PCWCH UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
ULONG
NTAPI
RtlxOemStringToUnicodeSize(IN PCOEM_STRING OemString);

//
// OEM to Unicode Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCOEM_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlOemToUnicodeN(
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInOemString) PCCH OemString,
    _In_ ULONG BytesInOemString
);

#ifdef NTOS_MODE_USER

#define RtlOemStringToUnicodeSize(STRING) (                             \
    NLS_MB_OEM_CODE_PAGE_TAG ?                                          \
    RtlxOemStringToUnicodeSize(STRING) :                                \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)              \
)

#define RtlOemStringToCountedUnicodeSize(STRING) (                      \
    (ULONG)(RtlOemStringToUnicodeSize(STRING) - sizeof(UNICODE_NULL))   \
)

#endif

//
// Ansi->Unicode String Functions
//
_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
WCHAR
NTAPI
RtlAnsiCharToUnicodeChar(
  _Inout_ PUCHAR *SourceCharacter);

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
    PCANSI_STRING AnsiString
);

#ifdef NTOS_MODE_USER

#define RtlAnsiStringToUnicodeSize(STRING) (                        \
    NLS_MB_CODE_PAGE_TAG ?                                          \
    RtlxAnsiStringToUnicodeSize(STRING) :                           \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)          \
)

#endif

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeStringFromAsciiz(
    _Out_ PUNICODE_STRING Destination,
    _In_ PCSZ Source
);

//
// Unicode String Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
    PUNICODE_STRING Destination,
    PCWSTR Source
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
    PUNICODE_STRING Destination,
    PCUNICODE_STRING Source
);

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
    _In_reads_(String1Length) PCWCH String1,
    _In_ SIZE_T String1Length,
    _In_reads_(String2Length) PCWCH String2,
    _In_ SIZE_T String2Length,
    _In_ BOOLEAN CaseInSensitive
);

NTSYSAPI
VOID
NTAPI
RtlCopyUnicodeString(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString
);

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);

#ifdef NTOS_MODE_USER

NTSYSAPI
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
    _Inout_ PUNICODE_STRING UniDest,
    _In_ PCUNICODE_STRING UniSource,
    _In_ BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING SourceString,
    _Out_ PUNICODE_STRING DestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlFindCharInUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING SearchString,
    _In_ PCUNICODE_STRING MatchString,
    _Out_ PUSHORT Position
);

//
// Memory Functions
//
#if defined(_M_AMD64)

FORCEINLINE
VOID
RtlFillMemoryUlong(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern)
{
    PULONG Address = (PULONG)Destination;
    if ((Length /= 4) != 0) {
        if (((ULONG64)Address & 4) != 0) {
            *Address = Pattern;
            if ((Length -= 1) == 0) {
                return;
            }
            Address += 1;
        }
        __stosq((PULONG64)(Address), Pattern | ((ULONG64)Pattern << 32), Length / 2);
        if ((Length & 1) != 0) Address[Length - 1] = Pattern;
    }
    return;
}

#define RtlFillMemoryUlonglong(Destination, Length, Pattern)                \
    __stosq((PULONG64)(Destination), Pattern, (Length) / 8)

#else

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlong(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern
);

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlonglong(
    _Out_ PVOID Destination,
    _In_ SIZE_T Length,
    _In_ ULONGLONG Pattern
);

#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlCopyMappedMemory(
    _Out_writes_bytes_all_(Size) PVOID Destination,
    _In_reads_bytes_(Size) const VOID *Source,
    _In_ SIZE_T Size
);

NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong(
    _In_ PVOID Source,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern
);

#ifndef RtlEqualMemory
#define RtlEqualMemory(Destination, Source, Length) \
    (!memcmp(Destination, Source, Length))
#endif

#define RtlCopyBytes RtlCopyMemory
#define RtlFillBytes RtlFillMemory
#define RtlZeroBytes RtlZeroMemory

#endif

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    _Inout_ _At_(UnicodeString->Buffer, __drv_freesMem(Mem))
        PUNICODE_STRING UnicodeString
);

NTSYSAPI
VOID
NTAPI
RtlEraseUnicodeString(
    _Inout_ PUNICODE_STRING String
);

NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
    _In_ CONST UNICODE_STRING *String,
    _In_ BOOLEAN CaseInSensitive,
    _In_ ULONG HashAlgorithm,
    _Out_ PULONG HashValue
);

_IRQL_requires_max_(DISPATCH_LEVEL)
_At_(DestinationString->Buffer, _Post_equal_to_(SourceString))
_When_(SourceString != NULL,
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(SourceString)))
_At_(DestinationString->MaximumLength, _Post_equal_to_(DestinationString->Length + sizeof(CHAR))))
_When_(SourceString == NULL,
_At_(DestinationString->Length, _Post_equal_to_(0))
_At_(DestinationString->MaximumLength, _Post_equal_to_(0)))
NTSYSAPI
VOID
NTAPI
RtlInitString(
    _Out_ PSTRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSTR SourceString
);

_IRQL_requires_max_(DISPATCH_LEVEL)
_At_(DestinationString->Buffer, _Post_equal_to_(SourceString))
_When_(SourceString != NULL,
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(SourceString) * sizeof(WCHAR)))
_At_(DestinationString->MaximumLength, _Post_equal_to_(DestinationString->Length + sizeof(WCHAR))))
_When_(SourceString == NULL,
_At_(DestinationString->Length, _Post_equal_to_(0))
_At_(DestinationString->MaximumLength, _Post_equal_to_(0)))
NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCWSTR SourceString
);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCWSTR SourceString
);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsTextUnicode(
    _In_ CONST VOID* Buffer,
    _In_ INT Size,
    _Inout_opt_ INT* Flags
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixString(
    _In_ const STRING *String1,
    _In_ const STRING *String2,
    _In_ BOOLEAN CaseInsensitive
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInsensitive
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlUpperString(
    _Inout_ PSTRING DestinationString,
    _In_ const STRING *SourceString
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareString(
    _In_ const STRING *String1,
    _In_ const STRING *String2,
    _In_ BOOLEAN CaseInSensitive
);

NTSYSAPI
VOID
NTAPI
RtlCopyString(
    _Out_ PSTRING DestinationString,
    _In_opt_ const STRING *SourceString
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
    _In_ const STRING *String1,
    _In_ const STRING *String2,
    _In_ BOOLEAN CaseInSensitive
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendStringToString(
    _Inout_ PSTRING Destination,
    _In_ const STRING *Source
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
        PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
    _In_ PCUNICODE_STRING String,
    _In_opt_ ULONG Base,
    _Out_ PULONG Value
);

NTSYSAPI
NTSTATUS
NTAPI
RtlValidateUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING String
);

#define RTL_SKIP_BUFFER_COPY    0x00000001

NTSYSAPI
NTSTATUS
NTAPI
RtlpEnsureBufferSize(
    _In_ ULONG Flags,
    _Inout_ PRTL_BUFFER Buffer,
    _In_ SIZE_T RequiredSize
);

#ifdef NTOS_MODE_USER

FORCEINLINE
VOID
RtlInitBuffer(
    _Inout_ PRTL_BUFFER Buffer,
    _In_ PUCHAR Data,
    _In_ ULONG DataSize
)
{
    Buffer->Buffer = Buffer->StaticBuffer = Data;
    Buffer->Size = Buffer->StaticSize = DataSize;
    Buffer->ReservedForAllocatedSize = 0;
    Buffer->ReservedForIMalloc = NULL;
}

FORCEINLINE
NTSTATUS
RtlEnsureBufferSize(
    _In_ ULONG Flags,
    _Inout_ PRTL_BUFFER Buffer,
    _In_ ULONG RequiredSize
)
{
    if (Buffer && RequiredSize <= Buffer->Size)
        return STATUS_SUCCESS;
    return RtlpEnsureBufferSize(Flags, Buffer, RequiredSize);
}

FORCEINLINE
VOID
RtlFreeBuffer(
    _Inout_ PRTL_BUFFER Buffer
)
{
    if (Buffer->Buffer != Buffer->StaticBuffer && Buffer->Buffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer->Buffer);
    Buffer->Buffer = Buffer->StaticBuffer;
    Buffer->Size = Buffer->StaticSize;
}

NTSYSAPI
VOID
NTAPI
RtlRunEncodeUnicodeString(
    _Inout_ PUCHAR Hash,
    _Inout_ PUNICODE_STRING String
);

NTSYSAPI
VOID
NTAPI
RtlRunDecodeUnicodeString(
    _In_ UCHAR Hash,
    _Inout_ PUNICODE_STRING String
);

#endif /* NTOS_MODE_USER */

//
// Ansi String Functions
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    _Inout_ _At_(AnsiString->Buffer, __drv_freesMem(Mem))
        PANSI_STRING AnsiString
);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
);

//
// OEM String Functions
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeOemString(
    _Inout_ _At_(OemString->Buffer, __drv_freesMem(Mem))
        POEM_STRING OemString
);

//
// MultiByte->Unicode String Functions
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInMultiByteString) const CHAR *MultiByteString,
    _In_ ULONG BytesInMultiByteString
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
    _Out_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInMultiByteString) const CHAR *MultiByteString,
    _In_ ULONG BytesInMultiByteString
);

//
// Atom Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAtomToAtomTable(
    _In_ PRTL_ATOM_TABLE AtomTable,
    _In_ PWSTR AtomName,
    _Out_ PRTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAtomTable(
    _In_ ULONG TableSize,
    _Inout_ PRTL_ATOM_TABLE *AtomTable
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAtomFromAtomTable(
    _In_ PRTL_ATOM_TABLE AtomTable,
    _In_ RTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyAtomTable(IN PRTL_ATOM_TABLE AtomTable);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryAtomInAtomTable(
    _In_ PRTL_ATOM_TABLE AtomTable,
    _In_ RTL_ATOM Atom,
    _Out_opt_ PULONG RefCount,
    _Out_opt_ PULONG PinCount,
    _Out_opt_z_bytecap_(*NameLength) PWSTR AtomName,
    _Inout_opt_ PULONG NameLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlPinAtomInAtomTable(
    _In_ PRTL_ATOM_TABLE AtomTable,
    _In_ RTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
RtlLookupAtomInAtomTable(
    _In_ PRTL_ATOM_TABLE AtomTable,
    _In_ PWSTR AtomName,
    _Out_ PRTL_ATOM Atom
);

//
// Process Management Functions
//
NTSYSAPI
PPEB
NTAPI
RtlGetCurrentPeb(
    VOID
);

NTSYSAPI
VOID
NTAPI
RtlAcquirePebLock(VOID);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessParameters (
    _Out_ PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
    _In_ PUNICODE_STRING ImagePathName,
    _In_opt_ PUNICODE_STRING DllPath,
    _In_opt_ PUNICODE_STRING CurrentDirectory,
    _In_opt_ PUNICODE_STRING CommandLine,
    _In_opt_ PWSTR Environment,
    _In_opt_ PUNICODE_STRING WindowTitle,
    _In_opt_ PUNICODE_STRING DesktopInfo,
    _In_opt_ PUNICODE_STRING ShellInfo,
    _In_opt_ PUNICODE_STRING RuntimeInfo
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserProcess(
    _In_ PUNICODE_STRING ImageFileName,
    _In_ ULONG Attributes,
    _In_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    _In_opt_ PSECURITY_DESCRIPTOR ProcessSecutityDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    _In_opt_ HANDLE ParentProcess,
    _In_ BOOLEAN CurrentDirectory,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE ExceptionPort,
    _Out_ PRTL_USER_PROCESS_INFORMATION ProcessInfo
);

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserThread(
    _In_ PVOID ThreadContext,
    _Out_ HANDLE *OutThreadHandle,
    _Reserved_ PVOID Reserved1,
    _Reserved_ PVOID Reserved2,
    _Reserved_ PVOID Reserved3,
    _Reserved_ PVOID Reserved4,
    _Reserved_ PVOID Reserved5,
    _Reserved_ PVOID Reserved6,
    _Reserved_ PVOID Reserved7,
    _Reserved_ PVOID Reserved8
);
#else
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN CreateSuspended,
    _In_ ULONG StackZeroBits,
    _In_ SIZE_T StackReserve,
    _In_ SIZE_T StackCommit,
    _In_ PTHREAD_START_ROUTINE StartAddress,
    _In_ PVOID Parameter,
    _Out_opt_ PHANDLE ThreadHandle,
    _Out_opt_ PCLIENT_ID ClientId
);
#endif

NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlDeNormalizeProcessParams(
    _In_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyProcessParameters(
    _In_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

NTSYSAPI
VOID
NTAPI
RtlExitUserThread(
    _In_ NTSTATUS Status);

NTSYSAPI
VOID
NTAPI
RtlInitializeContext(
    _In_ HANDLE ProcessHandle,
    _Out_ PCONTEXT ThreadContext,
    _In_opt_ PVOID ThreadStartParam,
    _In_ PTHREAD_START_ROUTINE ThreadStartAddress,
    _In_ PINITIAL_TEB InitialTeb
);

#ifdef _M_AMD64
typedef struct _WOW64_CONTEXT *PWOW64_CONTEXT;

NTSYSAPI
NTSTATUS
NTAPI
RtlWow64GetThreadContext(
    _In_ HANDLE ThreadHandle,
    _Inout_ PWOW64_CONTEXT ThreadContext
);


NTSYSAPI
NTSTATUS
NTAPI
RtlWow64SetThreadContext(
    _In_ HANDLE ThreadHandle,
    _In_ PWOW64_CONTEXT ThreadContext
);
#endif

NTSYSAPI
BOOLEAN
NTAPI
RtlIsThreadWithinLoaderCallout(VOID);

NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlNormalizeProcessParams(
    _In_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

NTSYSAPI
VOID
NTAPI
RtlReleasePebLock(VOID);

NTSYSAPI
NTSTATUS
NTAPI
RtlRemoteCall(
    _In_ HANDLE Process,
    _In_ HANDLE Thread,
    _In_ PVOID CallSite,
    _In_ ULONG ArgumentCount,
    _In_ PULONG Arguments,
    _In_ BOOLEAN PassContext,
    _In_ BOOLEAN AlreadySuspended
);

NTSYSAPI
NTSTATUS
__cdecl
RtlSetProcessIsCritical(
    _In_ BOOLEAN NewValue,
    _Out_opt_ PBOOLEAN OldValue,
    _In_ BOOLEAN NeedBreaks
);

NTSYSAPI
NTSTATUS
__cdecl
RtlSetThreadIsCritical(
    _In_ BOOLEAN NewValue,
    _Out_opt_ PBOOLEAN OldValue,
    _In_ BOOLEAN NeedBreaks
);

NTSYSAPI
ULONG
NTAPI
RtlGetCurrentProcessorNumber(
    VOID
);


//
// Thread Pool Functions
//
//
NTSTATUS
NTAPI
RtlSetThreadPoolStartFunc(
    _In_ PRTL_START_POOL_THREAD StartPoolThread,
    _In_ PRTL_EXIT_POOL_THREAD ExitPoolThread
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterWaitEx(
    _In_ HANDLE hWaitHandle,
    _In_opt_ HANDLE hCompletionEvent
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterWait(
    _In_ HANDLE hWaitHandle
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueueWorkItem(
    _In_ WORKERCALLBACKFUNC Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetIoCompletionCallback(
    _In_ HANDLE FileHandle,
    _In_ PIO_APC_ROUTINE Callback,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterWait(
    _In_ PHANDLE phNewWaitObject,
    _In_ HANDLE hObject,
    _In_ WAITORTIMERCALLBACKFUNC Callback,
    _In_ PVOID pvContext,
    _In_ ULONG ulMilliseconds,
    _In_ ULONG ulFlags
);

//
// Environment/Path Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironment(
    _In_ BOOLEAN Inherit,
    _Out_ PWSTR *Environment
);

NTSYSAPI
NTSTATUS
NTAPI
RtlComputePrivatizedDllName_U(
    _In_ PUNICODE_STRING DllName,
    _Inout_ PUNICODE_STRING RealName,
    _Inout_ PUNICODE_STRING LocalName
);

NTSYSAPI
VOID
NTAPI
RtlDestroyEnvironment(
    _In_ PWSTR Environment
);

NTSYSAPI
BOOLEAN
NTAPI
RtlDoesFileExists_U(
    _In_ PCWSTR FileName
);

NTSYSAPI
RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U(
    _In_ PCWSTR Path
);

NTSYSAPI
ULONG
NTAPI
RtlDosSearchPath_U(
    _In_ PCWSTR Path,
    _In_ PCWSTR FileName,
    _In_ PCWSTR Extension,
    _In_ ULONG BufferSize,
    _Out_ PWSTR Buffer,
    _Out_ PWSTR *PartName
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDosSearchPath_Ustr(
    _In_ ULONG Flags,
    _In_ PUNICODE_STRING PathString,
    _In_ PUNICODE_STRING FileNameString,
    _In_ PUNICODE_STRING ExtensionString,
    _In_ PUNICODE_STRING CallerBuffer,
    _Inout_opt_ PUNICODE_STRING DynamicString,
    _Out_opt_ PUNICODE_STRING* FullNameOut,
    _Out_opt_ PSIZE_T FilePartSize,
    _Out_opt_ PSIZE_T LengthNeeded
);

NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    _In_opt_z_ PCWSTR DosPathName,
    _Out_ PUNICODE_STRING NtPathName,
    _Out_opt_ PCWSTR *NtFileNamePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U DirectoryInfo
);


#define RTL_UNCHANGED_UNK_PATH  1
#define RTL_CONVERTED_UNC_PATH  2
#define RTL_CONVERTED_NT_PATH   3
#define RTL_UNCHANGED_DOS_PATH  4

NTSYSAPI
NTSTATUS
NTAPI
RtlNtPathNameToDosPathName(
    _In_ ULONG Flags,
    _Inout_ PRTL_UNICODE_STRING_BUFFER Path,
    _Out_opt_ PULONG PathType,
    _Out_opt_ PULONG Unknown
);


NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U(
    _In_ PCWSTR DosName,
    _Out_ PUNICODE_STRING NtName,
    _Out_ PCWSTR *PartName,
    _Out_ PRTL_RELATIVE_NAME_U RelativeName
);

_At_(Destination->Buffer, _Out_bytecap_(Destination->MaximumLength))
NTSYSAPI
NTSTATUS
NTAPI
RtlExpandEnvironmentStrings_U(
    _In_z_ PWSTR Environment,
    _In_ PUNICODE_STRING Source,
    _Inout_ PUNICODE_STRING Destination,
    _Out_ PULONG Length
);

NTSYSAPI
ULONG
NTAPI
RtlGetCurrentDirectory_U(
    _In_ ULONG MaximumLength,
    _Out_bytecap_(MaximumLength) PWSTR Buffer
);

NTSYSAPI
ULONG
NTAPI
RtlGetFullPathName_U(
    _In_ PCWSTR FileName,
    _In_ ULONG Size,
    _Out_z_bytecap_(Size) PWSTR Buffer,
    _Out_opt_ PWSTR *ShortName
);

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetFullPathName_UEx(
    _In_ PWSTR FileName,
    _In_ ULONG BufferLength,
    _Out_ PWSTR Buffer,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ RTL_PATH_TYPE *InputPathType
    );
#endif

NTSTATUS
NTAPI
RtlGetFullPathName_UstrEx(
    _In_ PUNICODE_STRING FileName,
    _In_opt_ PUNICODE_STRING StaticString,
    _In_opt_ PUNICODE_STRING DynamicString,
    _Out_opt_ PUNICODE_STRING *StringUsed,
    _Out_opt_ PSIZE_T FilePartSize,
    _Out_opt_ PBOOLEAN NameInvalid,
    _Out_ RTL_PATH_TYPE* PathType,
    _Out_opt_ PSIZE_T LengthNeeded
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeperators(
    _Reserved_ ULONG Flags,
    _In_ PCUNICODE_STRING PathString,
    _Out_ PULONG Length
);

NTSYSAPI
ULONG
NTAPI
RtlGetLongestNtPathLength(
    VOID
);

NTSYSAPI
ULONG
NTAPI
RtlIsDosDeviceName_U(
    _In_ PCWSTR Name
);

NTSYSAPI
ULONG
NTAPI
RtlIsDosDeviceName_Ustr(
    _In_ PCUNICODE_STRING Name
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(
    _In_ PCUNICODE_STRING Name,
    _Inout_opt_ POEM_STRING OemName,
    _Out_opt_ PBOOLEAN NameContainsSpaces
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
    _In_opt_ PWSTR Environment,
    _In_ PCUNICODE_STRING Name,
    _Out_ PUNICODE_STRING Value
);

VOID
NTAPI
RtlReleaseRelativeName(
    _In_ PRTL_RELATIVE_NAME_U RelativeName
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentDirectory_U(
    _In_ PUNICODE_STRING name
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVariable(
    _In_z_ PWSTR *Environment,
    _In_ PUNICODE_STRING Name,
    _In_ PUNICODE_STRING Value
);

//
// Critical Section/Resource Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection (
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    _In_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
);

NTSYSAPI
ULONG
NTAPI
RtlIsCriticalSectionLocked(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
ULONG
NTAPI
RtlIsCriticalSectionLockedByThread(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
BOOLEAN
NTAPI
RtlTryEnterCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
VOID
NTAPI
RtlpUnWaitCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpWaitForCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    _In_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceShared(
    _In_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
);

NTSYSAPI
VOID
NTAPI
RtlConvertExclusiveToShared(
    _In_ PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlConvertSharedToExclusive(
    _In_ PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlDeleteResource(
    _In_ PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlDumpResource(
    _In_ PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlInitializeResource(
    _In_ PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlReleaseResource(
    _In_ PRTL_RESOURCE Resource
);

//
// Compression Functions
//
NTSYSAPI //NT_RTL_COMPRESS_API
NTSTATUS
NTAPI
RtlCompressBuffer(
    _In_ USHORT CompressionFormatAndEngine,
    _In_reads_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONG UncompressedChunkSize,
    _Out_ PULONG FinalCompressedSize,
    _In_ PVOID WorkSpace
);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI //NT_RTL_COMPRESS_API
NTSTATUS
NTAPI
RtlDecompressBuffer(
    _In_ USHORT CompressionFormat,
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalUncompressedSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
    _In_ USHORT CompressionFormatAndEngine,
    _Out_ PULONG CompressBufferWorkSpaceSize,
    _Out_ PULONG CompressFragmentWorkSpaceSize
);

//
// Frame Functions
//
NTSYSAPI
VOID
NTAPI
RtlPopFrame(
    _In_ PTEB_ACTIVE_FRAME Frame
);

NTSYSAPI
VOID
NTAPI
RtlPushFrame(
    _In_ PTEB_ACTIVE_FRAME Frame
);

NTSYSAPI
PTEB_ACTIVE_FRAME
NTAPI
RtlGetFrame(
    VOID
);

//
// Debug Info Functions
//
NTSYSAPI
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(
    _In_ ULONG Size,
    _In_ BOOLEAN EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(IN PRTL_DEBUG_INFORMATION DebugBuffer);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    _In_ ULONG ProcessId,
    _In_ ULONG DebugInfoClassMask,
    _Inout_ PRTL_DEBUG_INFORMATION DebugBuffer
);

//
// Bitmap Functions
//
#ifdef NTOS_MODE_USER

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
);

NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
    _In_ PRTL_BITMAP BitMapHeader
);

NTSYSAPI
VOID
NTAPI
RtlClearBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
);

NTSYSAPI
VOID
NTAPI
RtlClearBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear
);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
);

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_ PULONG StartingIndex
);

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_writes_to_(SizeOfRunArray, return) PRTL_BITMAP_RUN RunArray,
    _In_range_(>, 0) ULONG SizeOfRunArray,
    _In_ BOOLEAN LocateLongestRuns
);

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex
);

NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit(
    _In_ ULONGLONG Value
);

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_ PULONG StartingIndex
);

NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit(
    _In_ ULONGLONG Value
);

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex
);

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex
);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
);

#ifdef __REACTOS__ // ReactOS improvement
_At_(BitMapHeader->SizeOfBitMap, _Post_equal_to_(SizeOfBitMap))
_At_(BitMapHeader->Buffer, _Post_equal_to_(BitMapBuffer))
#endif
NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
    _Out_ PRTL_BITMAP BitMapHeader,
    _In_opt_ __drv_aliasesMem PULONG BitMapBuffer,
    _In_opt_ ULONG SizeOfBitMap
);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
    _In_ PRTL_BITMAP BitMapHeader
);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
    _In_ PRTL_BITMAP BitMapHeader
);

NTSYSAPI
VOID
NTAPI
RtlSetBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
);

NTSYSAPI
VOID
NTAPI
RtlSetBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet
);

NTSYSAPI
VOID
NTAPI
RtlSetAllBits(
    _In_ PRTL_BITMAP BitMapHeader
);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
);

#if defined(_M_AMD64)
_Must_inspect_result_
FORCEINLINE
BOOLEAN
RtlCheckBit(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitPosition)
{
  return BitTest64((LONG64 CONST*)BitMapHeader->Buffer, (LONG64)BitPosition);
}
#else
#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP)/32]) >> ((BP)%32)) & 0x1)
#endif /* defined(_M_AMD64) */

#endif // NTOS_MODE_USER


//
// Timer Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimer(
    _In_ HANDLE TimerQueue,
    _In_ PHANDLE phNewTimer,
    _In_ WAITORTIMERCALLBACKFUNC Callback,
    _In_ PVOID Parameter,
    _In_ ULONG DueTime,
    _In_ ULONG Period,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimerQueue(PHANDLE TimerQueue);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimer(
    _In_ HANDLE TimerQueue,
    _In_ HANDLE Timer,
    _In_ HANDLE CompletionEvent
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpdateTimer(
    _In_ HANDLE TimerQueue,
    _In_ HANDLE Timer,
    _In_ ULONG DueTime,
    _In_ ULONG Period
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueueEx(
    _In_ HANDLE TimerQueue,
    _In_opt_ HANDLE CompletionEvent
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueue(HANDLE TimerQueue);

//
// SList functions
//
PSLIST_ENTRY
FASTCALL
InterlockedPushListSList(
    _Inout_ PSLIST_HEADER ListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY List,
    _Inout_ PSLIST_ENTRY ListEnd,
    _In_ ULONG Count
);

//
// Range List functions
//
NTSYSAPI
VOID
NTAPI
RtlInitializeRangeList(
    _Out_ PRTL_RANGE_LIST RangeList
);

NTSYSAPI
VOID
NTAPI
RtlFreeRangeList(
    _In_ PRTL_RANGE_LIST RangeList
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCopyRangeList(
    _Out_ PRTL_RANGE_LIST CopyRangeList,
    _In_ PRTL_RANGE_LIST RangeList
);

NTSYSAPI
NTSTATUS
NTAPI
RtlMergeRangeLists(
    _Out_ PRTL_RANGE_LIST MergedRangeList,
    _In_ PRTL_RANGE_LIST RangeList1,
    _In_ PRTL_RANGE_LIST RangeList2,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInvertRangeList(
    _Out_ PRTL_RANGE_LIST InvertedRangeList,
    _In_ PRTL_RANGE_LIST RangeList
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddRange(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ UCHAR Attributes,
    _In_ ULONG Flags,
    _In_opt_ PVOID UserData,
    _In_opt_ PVOID Owner
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRange(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ PVOID Owner
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteOwnersRanges(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _In_ _Maybenull_ PVOID Owner
);

NTSYSAPI
NTSTATUS
NTAPI
RtlFindRange(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Minimum,
    _In_ ULONGLONG Maximum,
    _In_ ULONG Length,
    _In_ ULONG Alignment,
    _In_ ULONG Flags,
    _In_ UCHAR AttributeAvailableMask,
    _In_opt_ PVOID Context,
    _In_opt_ PRTL_CONFLICT_RANGE_CALLBACK Callback,
    _Out_ PULONGLONG Start
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIsRangeAvailable(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ ULONG Flags,
    _In_ UCHAR AttributeAvailableMask,
    _In_opt_ PVOID Context,
    _In_opt_ PRTL_CONFLICT_RANGE_CALLBACK Callback,
    _Out_ PBOOLEAN Available
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetFirstRange(
    _In_ PRTL_RANGE_LIST RangeList,
    _Out_ PRTL_RANGE_LIST_ITERATOR Iterator,
    _Outptr_ PRTL_RANGE *Range
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetNextRange(
    _Inout_ PRTL_RANGE_LIST_ITERATOR Iterator,
    _Outptr_ PRTL_RANGE *Range,
    _In_ BOOLEAN MoveForwards
);

//
// Debug Functions
//
ULONG
__cdecl
DbgPrint(
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
);

NTSYSAPI
ULONG
__cdecl
DbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
);

NTSYSAPI
ULONG
NTAPI
DbgPrompt(
    _In_z_ PCCH Prompt,
    _Out_writes_bytes_(MaximumResponseLength) PCH Response,
    _In_ ULONG MaximumResponseLength
);

#undef DbgBreakPoint
VOID
NTAPI
DbgBreakPoint(
    VOID
);

VOID
NTAPI
DbgLoadImageSymbols(
    _In_ PSTRING Name,
    _In_ PVOID Base,
    _In_ ULONG_PTR ProcessId
);

VOID
NTAPI
DbgUnLoadImageSymbols(
    _In_ PSTRING Name,
    _In_ PVOID Base,
    _In_ ULONG_PTR ProcessId
);

VOID
NTAPI
DbgCommandString(
    _In_ PCCH Name,
    _In_ PCCH Command
);

//
// Generic Table Functions
//
#if defined(NTOS_MODE_USER) || defined(_NTIFS_)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(
    _In_ PRTL_GENERIC_TABLE Table
);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
);
#endif

//
// Handle Table Functions
//
NTSYSAPI
PRTL_HANDLE_TABLE_ENTRY
NTAPI
RtlAllocateHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _Inout_ PULONG Index
);

NTSYSAPI
VOID
NTAPI
RtlDestroyHandleTable(
    _Inout_ PRTL_HANDLE_TABLE HandleTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ PRTL_HANDLE_TABLE_ENTRY Handle
);

NTSYSAPI
VOID
NTAPI
RtlInitializeHandleTable(
    _In_ ULONG TableSize,
    _In_ ULONG HandleSize,
    _In_ PRTL_HANDLE_TABLE HandleTable
);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ PRTL_HANDLE_TABLE_ENTRY Handle
);

_Success_(return!=FALSE)
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidIndexHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ ULONG Index,
    _Out_ PRTL_HANDLE_TABLE_ENTRY *Handle
);

//
// PE Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlFindMessage(
    _In_ PVOID BaseAddress,
    _In_ ULONG Type,
    _In_ ULONG Language,
    _In_ ULONG MessageId,
    _Out_ PMESSAGE_RESOURCE_ENTRY *MessageResourceEntry
);

NTSYSAPI
ULONG
NTAPI
RtlGetNtGlobalFlags(VOID);

_Success_(return!=NULL)
NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
    _In_ PVOID BaseAddress,
    _In_ BOOLEAN MappedAsImage,
    _In_ USHORT Directory,
    _Out_ PULONG Size
);

NTSYSAPI
PVOID
NTAPI
RtlImageRvaToVa(
    _In_ PIMAGE_NT_HEADERS NtHeader,
    _In_ PVOID BaseAddress,
    _In_ ULONG Rva,
    _Inout_opt_ PIMAGE_SECTION_HEADER *SectionHeader
);

NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
    _In_ PVOID BaseAddress);

NTSYSAPI
NTSTATUS
NTAPI
RtlImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ ULONGLONG Size,
    _Out_ PIMAGE_NT_HEADERS *NtHeader
);

NTSYSAPI
PIMAGE_SECTION_HEADER
NTAPI
RtlImageRvaToSection(
    _In_ PIMAGE_NT_HEADERS NtHeader,
    _In_ PVOID BaseAddress,
    _In_ ULONG Rva
);

ULONG
NTAPI
LdrRelocateImageWithBias(
    _In_ PVOID BaseAddress,
    _In_ LONGLONG AdditionalBias,
    _In_opt_ PCSTR LoaderName,
    _In_ ULONG Success,
    _In_ ULONG Conflict,
    _In_ ULONG Invalid
);

//
// Activation Context Functions
//
#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContextEx(
    _In_ ULONG Flags,
    _In_ PTEB Teb,
    _In_ PVOID Context,
    _Out_ PULONG_PTR Cookie
);

NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContext(
    _In_ ULONG Flags,
    _In_ HANDLE Handle,
    _Out_ PULONG_PTR Cookie
);

NTSYSAPI
VOID
NTAPI
RtlAddRefActivationContext(
    _In_ PVOID Context
);

NTSYSAPI
PRTL_ACTIVATION_CONTEXT_STACK_FRAME
FASTCALL
RtlActivateActivationContextUnsafeFast(
    _In_ PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame,
    _In_ PVOID Context
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateActivationContextStack(
    _In_ PACTIVATION_CONTEXT_STACK *Stack
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateActivationContext(
    _In_ ULONG Flags,
    _In_ PACTIVATION_CONTEXT_DATA ActivationContextData,
    _In_ ULONG ExtraBytes,
    _In_ PVOID NotificationRoutine,
    _In_ PVOID NotificationContext,
    _Out_ PACTIVATION_CONTEXT *ActCtx
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetActiveActivationContext(
    _In_ PVOID *Context
);

NTSYSAPI
VOID
NTAPI
RtlReleaseActivationContext(
    _In_ HANDLE handle
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeactivateActivationContext(
    _In_ ULONG dwFlags,
    _In_ ULONG_PTR ulCookie
);

NTSYSAPI
VOID
NTAPI
RtlFreeActivationContextStack(
    _In_ PACTIVATION_CONTEXT_STACK Stack
);

NTSYSAPI
VOID
NTAPI
RtlFreeThreadActivationContextStack(VOID);

NTSYSAPI
PRTL_ACTIVATION_CONTEXT_STACK_FRAME
FASTCALL
RtlDeactivateActivationContextUnsafeFast(
    _In_ PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDosApplyFileIsolationRedirection_Ustr(
    _In_ ULONG Flags,
    _In_ PUNICODE_STRING OriginalName,
    _In_ PUNICODE_STRING Extension,
    _Inout_ PUNICODE_STRING StaticString,
    _Inout_ PUNICODE_STRING DynamicString,
    _Inout_ PUNICODE_STRING *NewName,
    _In_ PULONG NewFlags,
    _In_ PSIZE_T FileNameSize,
    _In_ PSIZE_T RequiredLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionString(
    _In_ ULONG dwFlags,
    _In_ const GUID *ExtensionGuid,
    _In_ ULONG SectionType,
    _In_ const UNICODE_STRING *SectionName,
    _Inout_ PVOID ReturnedData
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationActivationContext(
    _In_ DWORD dwFlags,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID pvSubInstance,
    _In_ ULONG ulInfoClass,
    _Out_bytecap_(cbBuffer) PVOID pvBuffer,
    _In_opt_ SIZE_T cbBuffer,
    _Out_opt_ SIZE_T *pcbWrittenOrRequired
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationActiveActivationContext(
    _In_ ULONG ulInfoClass,
    _Out_bytecap_(cbBuffer) PVOID pvBuffer,
    _In_opt_ SIZE_T cbBuffer,
    _Out_opt_ SIZE_T *pcbWrittenOrRequired
);

NTSYSAPI
NTSTATUS
NTAPI
RtlZombifyActivationContext(
    PVOID Context
);

//
// WOW64 Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirection(
    _In_ BOOLEAN Wow64FsEnableRedirection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirectionEx(
    _In_ PVOID Wow64FsEnableRedirection,
    _Out_ PVOID *OldFsRedirectionLevel
);

#endif

//
// Registry Functions
//
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
    _In_ ULONG RelativeTo,
    _In_ PWSTR Path
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
    _In_ ULONG RelativeTo,
    _In_ PWSTR Path
);

NTSYSAPI
NTSTATUS
NTAPI
RtlFormatCurrentUserKeyPath(
    _Out_ _At_(KeyPath->Buffer, __drv_allocatesMem(Mem) _Post_bytecap_(KeyPath->MaximumLength) _Post_bytecount_(KeyPath->Length))
        PUNICODE_STRING KeyPath
);

NTSYSAPI
NTSTATUS
NTAPI
RtlOpenCurrentUser(
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE KeyHandle
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValues(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _Inout_ _At_(*(*QueryTable).EntryContext, _Pre_unknown_)
        PRTL_QUERY_REGISTRY_TABLE QueryTable,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID Environment
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _In_z_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _In_reads_bytes_opt_(ValueLength) PVOID ValueData,
    _In_ ULONG ValueLength
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
RtlpNtCreateKey(
    _Out_ HANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG TitleIndex,
    _In_ PUNICODE_STRING Class,
    _Out_ PULONG Disposition
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpNtEnumerateSubKey(
    _In_ HANDLE KeyHandle,
    _Inout_ PUNICODE_STRING SubKeyName,
    _In_ ULONG Index,
    _In_ ULONG Unused
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpNtMakeTemporaryKey(
    _In_ HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpNtOpenKey(
    _Out_ HANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Unused
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpNtQueryValueKey(
    _In_ HANDLE KeyHandle,
    _Out_opt_ PULONG Type,
    _Out_opt_ PVOID Data,
    _Inout_opt_ PULONG DataLength,
    _In_ ULONG Unused
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpNtSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Type,
    _In_ PVOID Data,
    _In_ ULONG DataLength
);
#endif

//
// NLS Functions
//
NTSYSAPI
VOID
NTAPI
RtlGetDefaultCodePage(
    _Out_ PUSHORT AnsiCodePage,
    _Out_ PUSHORT OemCodePage
);

NTSYSAPI
VOID
NTAPI
RtlInitNlsTables(
    _In_ PUSHORT AnsiTableBase,
    _In_ PUSHORT OemTableBase,
    _In_ PUSHORT CaseTableBase,
    _Out_ PNLSTABLEINFO NlsTable
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
    _In_ PUSHORT TableBase,
    _Out_ PCPTABLEINFO CodePageTable
);

NTSYSAPI
VOID
NTAPI
RtlResetRtlTranslations(
    _In_ PNLSTABLEINFO NlsTable);

#if defined(NTOS_MODE_USER) && !defined(NO_RTL_INLINES)

//
// Misc conversion functions
//
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertLongToLargeInteger(
    _In_ LONG SignedInteger
)
{
    LARGE_INTEGER Result;

    Result.QuadPart = SignedInteger;
    return Result;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedIntegerMultiply(
    _In_ LONG Multiplicand,
    _In_ LONG Multiplier
)
{
    LARGE_INTEGER Product;

    Product.QuadPart = (LONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return Product;
}

static __inline
ULONG
NTAPI_INLINE
RtlEnlargedUnsignedDivide(
    _In_ ULARGE_INTEGER Dividend,
    _In_ ULONG Divisor,
    _In_opt_ PULONG Remainder
)
{
    ULONG Quotient;

    Quotient = (ULONG)(Dividend.QuadPart / Divisor);
    if (Remainder) {
        *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
    }

    return Quotient;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedUnsignedMultiply(
    _In_ ULONG Multiplicand,
    _In_ ULONG Multiplier
)
{
    LARGE_INTEGER Product;

    Product.QuadPart = (ULONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return Product;
}

#if defined(_AMD64_) || defined(_IA64_)
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedLargeIntegerDivide(
    _In_ LARGE_INTEGER Dividend,
    _In_ ULONG Divisor,
    _Out_opt_ PULONG Remainder)
{
  LARGE_INTEGER ret;
  ret.QuadPart = (ULONG64)Dividend.QuadPart / Divisor;
  if (Remainder)
    *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
  return ret;
}

#else
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedLargeIntegerDivide(
    _In_ LARGE_INTEGER Dividend,
    _In_ ULONG Divisor,
    _Out_opt_ PULONG Remainder
);

#endif /* defined(_AMD64_) || defined(_IA64_) */

#endif


NTSYSAPI
ULONG
NTAPI
RtlUniform(
    _In_ PULONG Seed
);

NTSYSAPI
ULONG
NTAPI
RtlRandom(
    _Inout_ PULONG Seed
);

NTSYSAPI
ULONG
NTAPI
RtlComputeCrc32(
    _In_ ULONG InitialCrc,
    _In_ const UCHAR *Buffer,
    _In_ ULONG Length
);

//
// Network Functions
//
NTSYSAPI
PSTR
NTAPI
RtlIpv4AddressToStringA(
    _In_ const struct in_addr *Addr,
    _Out_writes_(16) PCHAR S
);

NTSYSAPI
PWSTR
NTAPI
RtlIpv4AddressToStringW(
    _In_ const struct in_addr *Addr,
    _Out_writes_(16) PWCHAR S
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4AddressToStringExA(
    _In_ const struct in_addr *Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PCHAR AddressString,
    _Inout_ PULONG AddressStringLength
);

NTSTATUS
NTAPI
RtlIpv4AddressToStringExW(
    _In_ const struct in_addr *Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWCHAR AddressString,
    _Inout_ PULONG AddressStringLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressA(
    _In_ PCSTR String,
    _In_ BOOLEAN Strict,
    _Out_ PCSTR *Terminator,
    _Out_ struct in_addr *Addr
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(
    _In_ PCWSTR String,
    _In_ BOOLEAN Strict,
    _Out_ PCWSTR *Terminator,
    _Out_ struct in_addr *Addr
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressExA(
    _In_ PCSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ struct in_addr *Address,
    _Out_ PUSHORT Port
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressExW(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ struct in_addr *Address,
    _Out_ PUSHORT Port
);

NTSYSAPI
PSTR
NTAPI
RtlIpv6AddressToStringA(
    _In_ const struct in6_addr *Addr,
    _Out_writes_(46) PSTR S
);

NTSYSAPI
PWSTR
NTAPI
RtlIpv6AddressToStringW(
    _In_ const struct in6_addr *Addr,
    _Out_writes_(46) PWSTR S
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6AddressToStringExA(
    _In_ const struct in6_addr *Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PSTR AddressString,
    _Inout_ PULONG AddressStringLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6AddressToStringExW(
    _In_ const struct in6_addr *Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWCHAR AddressString,
    _Inout_ PULONG AddressStringLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressA(
    _In_ PCSTR String,
    _Out_ PCSTR *Terminator,
    _Out_ struct in6_addr *Addr
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(
    _In_ PCWSTR String,
    _Out_ PCWSTR *Terminator,
    _Out_ struct in6_addr *Addr
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressExA(
    _In_ PCSTR AddressString,
    _Out_ struct in6_addr *Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressExW(
    _In_ PCWSTR AddressString,
    _Out_ struct in6_addr *Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
);


//
// Time Functions
//
NTSYSAPI
BOOLEAN
NTAPI
RtlCutoverTimeToSystemTime(
    _In_ PTIME_FIELDS CutoverTimeFields,
    _Out_ PLARGE_INTEGER SystemTime,
    _In_ PLARGE_INTEGER CurrentTime,
    _In_ BOOLEAN ThisYearsCutoverOnly);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryTimeZoneInformation(
    _Out_ PRTL_TIME_ZONE_INFORMATION TimeZoneInformation);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
    _In_ ULONG SecondsSince1970,
    _Out_ PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetTimeZoneInformation(
    _In_ PRTL_TIME_ZONE_INFORMATION TimeZoneInformation);

_Success_(return != FALSE)
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
    _In_ PTIME_FIELDS TimeFields,
    _Out_ PLARGE_INTEGER Time
);

_Success_(return != FALSE)
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
);

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
    PLARGE_INTEGER Time,
    PTIME_FIELDS TimeFields
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSystemTimeToLocalTime(
    _In_ PLARGE_INTEGER SystemTime,
    _Out_ PLARGE_INTEGER LocalTime
);

//
// Version Functions
//
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
    _In_ PRTL_OSVERSIONINFOEXW VersionInfo,
    _In_ ULONG TypeMask,
    _In_ ULONGLONG ConditionMask
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
    _Out_
    _At_(lpVersionInformation->dwOSVersionInfoSize, _Pre_ _Valid_)
    _When_(lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW),
        _At_((PRTL_OSVERSIONINFOEXW)lpVersionInformation, _Out_))
        PRTL_OSVERSIONINFOW lpVersionInformation
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
BOOLEAN
NTAPI
RtlGetNtProductType(_Out_ PNT_PRODUCT_TYPE ProductType);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
//
// Synchronization functions
//
NTSYSAPI
VOID
NTAPI
RtlInitializeConditionVariable(
    _Out_ PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
VOID
NTAPI
RtlWakeConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
VOID
NTAPI
RtlWakeAllConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableCS(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_opt_ PLARGE_INTEGER TimeOut);

NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_opt_ PLARGE_INTEGER TimeOut,
    _In_ ULONG Flags);
#endif

//
// Synchronization functions
//
NTSYSAPI
VOID
NTAPI
RtlInitializeConditionVariable(OUT PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
VOID
NTAPI
RtlWakeConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
VOID
NTAPI
RtlWakeAllConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableCS(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                            IN OUT PRTL_CRITICAL_SECTION CriticalSection,
                            IN const PLARGE_INTEGER TimeOut OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                             IN OUT PRTL_SRWLOCK SRWLock,
                             IN PLARGE_INTEGER TimeOut OPTIONAL,
                             IN ULONG Flags);

VOID
NTAPI
RtlInitializeSRWLock(OUT PRTL_SRWLOCK SRWLock);

VOID
NTAPI
RtlAcquireSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

VOID
NTAPI
RtlReleaseSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

VOID
NTAPI
RtlAcquireSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

VOID
NTAPI
RtlReleaseSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

//
// Secure Memory Functions
//
#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterSecureMemoryCacheCallback(
    _In_ PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback);

NTSYSAPI
BOOLEAN
NTAPI
RtlFlushSecureMemoryCache(
    _In_ PVOID MemoryCache,
    _In_opt_ SIZE_T MemoryLength
);
#endif

//
// Boot Status Data Functions
//
#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateBootStatusDataFile(
    VOID
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSetBootStatusData(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN WriteMode,
    _In_ RTL_BSD_ITEM_TYPE DataClass,
    _In_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlLockBootStatusData(
    _Out_ PHANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockBootStatusData(
    _In_ HANDLE FileHandle
);
#endif

#ifdef NTOS_MODE_USER
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
    _In_ PUNICODE_STRING GuidString,
    _Out_ GUID *Guid);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
  _In_ REFGUID Guid,
  _Out_ _At_(GuidString->Buffer, __drv_allocatesMem(Mem))
    PUNICODE_STRING GuidString);

NTSYSAPI
NTSTATUS
NTAPI
RtlComputeImportTableHash(
    _In_ HANDLE hFile,
    _Out_ PCHAR Hash,
    _In_ ULONG ImportTableHashRevision
);
#endif

//
// MemoryStream functions
//
#ifdef NTOS_MODE_USER

NTSYSAPI
VOID
NTAPI
RtlInitMemoryStream(
    _Out_ PRTL_MEMORY_STREAM Stream
);

NTSYSAPI
VOID
NTAPI
RtlInitOutOfProcessMemoryStream(
    _Out_ PRTL_MEMORY_STREAM Stream
);

NTSYSAPI
VOID
NTAPI
RtlFinalReleaseOutOfProcessMemoryStream(
    _In_ PRTL_MEMORY_STREAM Stream
);

NTSYSAPI
HRESULT
NTAPI
RtlQueryInterfaceMemoryStream(
    _In_ struct IStream *This,
    _In_ REFIID RequestedIid,
    _Outptr_ PVOID *ResultObject
);

NTSYSAPI
ULONG
NTAPI
RtlAddRefMemoryStream(
    _In_ struct IStream *This
);

NTSYSAPI
ULONG
NTAPI
RtlReleaseMemoryStream(
    _In_ struct IStream *This
);

NTSYSAPI
HRESULT
NTAPI
RtlReadMemoryStream(
    _In_ struct IStream *This,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesRead
);

NTSYSAPI
HRESULT
NTAPI
RtlReadOutOfProcessMemoryStream(
    _In_ struct IStream *This,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesRead
);

NTSYSAPI
HRESULT
NTAPI
RtlSeekMemoryStream(
    _In_ struct IStream *This,
    _In_ LARGE_INTEGER RelativeOffset,
    _In_ ULONG Origin,
    _Out_opt_ PULARGE_INTEGER ResultOffset
);

NTSYSAPI
HRESULT
NTAPI
RtlCopyMemoryStreamTo(
    _In_ struct IStream *This,
    _In_ struct IStream *Target,
    _In_ ULARGE_INTEGER Length,
    _Out_opt_ PULARGE_INTEGER BytesRead,
    _Out_opt_ PULARGE_INTEGER BytesWritten
);

NTSYSAPI
HRESULT
NTAPI
RtlCopyOutOfProcessMemoryStreamTo(
    _In_ struct IStream *This,
    _In_ struct IStream *Target,
    _In_ ULARGE_INTEGER Length,
    _Out_opt_ PULARGE_INTEGER BytesRead,
    _Out_opt_ PULARGE_INTEGER BytesWritten
);

NTSYSAPI
HRESULT
NTAPI
RtlStatMemoryStream(
    _In_ struct IStream *This,
    _Out_ struct tagSTATSTG *Stats,
    _In_ ULONG Flags
);

// Dummy functions
NTSYSAPI
HRESULT
NTAPI
RtlWriteMemoryStream(
    _In_ struct IStream *This,
    _In_reads_bytes_(Length) CONST VOID *Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesWritten
);

NTSYSAPI
HRESULT
NTAPI
RtlSetMemoryStreamSize(
    _In_ struct IStream *This,
    _In_ ULARGE_INTEGER NewSize
);

NTSYSAPI
HRESULT
NTAPI
RtlCommitMemoryStream(
    _In_ struct IStream *This,
    _In_ ULONG CommitFlags
);

NTSYSAPI
HRESULT
NTAPI
RtlRevertMemoryStream(
    _In_ struct IStream *This
);

NTSYSAPI
HRESULT
NTAPI
RtlLockMemoryStreamRegion(
    _In_ struct IStream *This,
    _In_ ULARGE_INTEGER Offset,
    _In_ ULARGE_INTEGER Length,
    _In_ ULONG LockType
);

NTSYSAPI
HRESULT
NTAPI
RtlUnlockMemoryStreamRegion(
    _In_ struct IStream *This,
    _In_ ULARGE_INTEGER Offset,
    _In_ ULARGE_INTEGER Length,
    _In_ ULONG LockType
);

NTSYSAPI
HRESULT
NTAPI
RtlCloneMemoryStream(
    _In_ struct IStream *This,
    _Outptr_ struct IStream **ResultStream
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetNativeSystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)

NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
    _Out_ PRTL_RUN_ONCE RunOnce);

_Maybe_raises_SEH_exception_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ __inner_callback PRTL_RUN_ONCE_INIT_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID *Context);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID *Context);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context);

#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || (defined(__REACTOS__) && defined(_NTDLLBUILD_))
/* Put NTSYSAPI back when this will be really exported. Only statically linked for now */
// NTSYSAPI
VOID
NTAPI
RtlInitializeSRWLock(OUT PRTL_SRWLOCK SRWLock);

// NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

// NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

// NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

// NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

#endif /* Win vista or Reactos Ntdll build */

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7) || (defined(__REACTOS__) && defined(_NTDLLBUILD_))

// NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared(PRTL_SRWLOCK SRWLock);

// NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive(PRTL_SRWLOCK SRWLock);

#endif /* Win7 or Reactos Ntdll build */

#endif // NTOS_MODE_USER

NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionGuid(
    ULONG flags,
    const GUID *extguid,
    ULONG section_kind,
    const GUID *guid,
    void *ptr
);

#ifdef __cplusplus
}
#endif

#endif
