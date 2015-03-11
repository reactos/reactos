/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    rtltypes.h

Abstract:

    Type definitions for the Run-Time Library

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _RTLTYPES_H
#define _RTLTYPES_H

//
// Dependencies
//
#include <umtypes.h>
#include <mmtypes.h>
#include <ldrtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Maximum Atom Length
//
#define RTL_MAXIMUM_ATOM_LENGTH                             255

//
// Process Parameters Flags
//
#define RTL_USER_PROCESS_PARAMETERS_NORMALIZED              0x01
#define RTL_USER_PROCESS_PARAMETERS_PROFILE_USER            0x02
#define RTL_USER_PROCESS_PARAMETERS_PROFILE_KERNEL          0x04
#define RTL_USER_PROCESS_PARAMETERS_PROFILE_SERVER          0x08
#define RTL_USER_PROCESS_PARAMETERS_UNKNOWN                 0x10
#define RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB             0x20
#define RTL_USER_PROCESS_PARAMETERS_RESERVE_16MB            0x40
#define RTL_USER_PROCESS_PARAMETERS_CASE_SENSITIVE          0x80
#define RTL_USER_PROCESS_PARAMETERS_DISABLE_HEAP_CHECKS     0x100
#define RTL_USER_PROCESS_PARAMETERS_PROCESS_OR_1            0x200
#define RTL_USER_PROCESS_PARAMETERS_PROCESS_OR_2            0x400
#define RTL_USER_PROCESS_PARAMETERS_PRIVATE_DLL_PATH        0x1000
#define RTL_USER_PROCESS_PARAMETERS_LOCAL_DLL_PATH          0x2000
#define RTL_USER_PROCESS_PARAMETERS_IMAGE_KEY_MISSING       0x4000
#define RTL_USER_PROCESS_PARAMETERS_NX                      0x20000

#define RTL_MAX_DRIVE_LETTERS 32
#define RTL_DRIVE_LETTER_VALID (USHORT)0x0001

//
// End of Exception List
//
#define EXCEPTION_CHAIN_END                                 ((PEXCEPTION_REGISTRATION_RECORD)-1)

//
// Range and Range List Flags
//
#define RTL_RANGE_LIST_ADD_IF_CONFLICT                      0x00000001
#define RTL_RANGE_LIST_ADD_SHARED                           0x00000002

#define RTL_RANGE_SHARED                                    0x01
#define RTL_RANGE_CONFLICT                                  0x02

//
// Flags in RTL_ACTIVATION_CONTEXT_STACK_FRAME (from Checked NTDLL)
//
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_RELEASE_ON_DEACTIVATION         0x01
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NO_DEACTIVATE                   0x02
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST                    0x04
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED                  0x08
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NOT_REALLY_ACTIVATED            0x10
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED                       0x20
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_DEACTIVATED                     0x40

//
// Activation Context Frame Flags (from Checked NTDLL)
//
#define RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER     0x01

//
// RtlActivateActivationContextEx Flags (from Checked NTDLL)
//
#define RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION   0x01

//
// RtlDeactivateActivationContext Flags (based on Win32 flag and name of above)
//
#define RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION         0x01

//
// RtlQueryActivationContext Flags (based on Win32 flag and name of above)
//
#define RTL_QUERY_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT         0x01
#define RTL_QUERY_ACTIVATION_CONTEXT_FLAG_IS_HMODULE                            0x02
#define RTL_QUERY_ACTIVATION_CONTEXT_FLAG_IS_ADDRESS                            0x04
#define RTL_QUERY_ACTIVATION_CONTEXT_FLAG_NO_ADDREF                             0x80000000

//
// Public Heap Flags
//
#if !defined(NTOS_MODE_USER) && !defined(_NTIFS_)
#define HEAP_NO_SERIALIZE                                   0x00000001
#define HEAP_GROWABLE                                       0x00000002
#define HEAP_GENERATE_EXCEPTIONS                            0x00000004
#define HEAP_ZERO_MEMORY                                    0x00000008
#define HEAP_REALLOC_IN_PLACE_ONLY                          0x00000010
#define HEAP_TAIL_CHECKING_ENABLED                          0x00000020
#define HEAP_FREE_CHECKING_ENABLED                          0x00000040
#define HEAP_DISABLE_COALESCE_ON_FREE                       0x00000080
#define HEAP_CREATE_ALIGN_16                                0x00010000
#define HEAP_CREATE_ENABLE_TRACING                          0x00020000
#define HEAP_CREATE_ENABLE_EXECUTE                          0x00040000
#endif

//
// User-Defined Heap Flags and Classes
//
#define HEAP_SETTABLE_USER_VALUE                            0x00000100
#define HEAP_SETTABLE_USER_FLAG1                            0x00000200
#define HEAP_SETTABLE_USER_FLAG2                            0x00000400
#define HEAP_SETTABLE_USER_FLAG3                            0x00000800
#define HEAP_SETTABLE_USER_FLAGS                            0x00000E00
#define HEAP_CLASS_0                                        0x00000000
#define HEAP_CLASS_1                                        0x00001000
#define HEAP_CLASS_2                                        0x00002000
#define HEAP_CLASS_3                                        0x00003000
#define HEAP_CLASS_4                                        0x00004000
#define HEAP_CLASS_5                                        0x00005000
#define HEAP_CLASS_6                                        0x00006000
#define HEAP_CLASS_7                                        0x00007000
#define HEAP_CLASS_8                                        0x00008000
#define HEAP_CLASS_MASK                                     0x0000F000

//
// Internal HEAP Structure Flags
//
#define HEAP_FLAG_PAGE_ALLOCS                               0x01000000
#define HEAP_PROTECTION_ENABLED                             0x02000000
#define HEAP_BREAK_WHEN_OUT_OF_VM                           0x04000000
#define HEAP_NO_ALIGNMENT                                   0x08000000
#define HEAP_CAPTURE_STACK_BACKTRACES                       0x08000000
#define HEAP_SKIP_VALIDATION_CHECKS                         0x10000000
#define HEAP_VALIDATE_ALL_ENABLED                           0x20000000
#define HEAP_VALIDATE_PARAMETERS_ENABLED                    0x40000000
#define HEAP_LOCK_USER_ALLOCATED                            0x80000000

//
// Heap Validation Flags
//
#define HEAP_CREATE_VALID_MASK                              \
    (HEAP_NO_SERIALIZE              |                       \
     HEAP_GROWABLE                  |                       \
     HEAP_GENERATE_EXCEPTIONS       |                       \
     HEAP_ZERO_MEMORY               |                       \
     HEAP_REALLOC_IN_PLACE_ONLY     |                       \
     HEAP_TAIL_CHECKING_ENABLED     |                       \
     HEAP_FREE_CHECKING_ENABLED     |                       \
     HEAP_DISABLE_COALESCE_ON_FREE  |                       \
     HEAP_CLASS_MASK                |                       \
     HEAP_CREATE_ALIGN_16           |                       \
     HEAP_CREATE_ENABLE_TRACING     |                       \
     HEAP_CREATE_ENABLE_EXECUTE)
#ifdef C_ASSERT
C_ASSERT(HEAP_CREATE_VALID_MASK == 0x0007F0FF);
#endif

//
// Native image architecture
//
#if defined(_M_IX86)
#define IMAGE_FILE_MACHINE_NATIVE IMAGE_FILE_MACHINE_I386
#elif defined(_M_ARM)
#define IMAGE_FILE_MACHINE_NATIVE IMAGE_FILE_MACHINE_ARM
#elif defined(_M_AMD64)
#define IMAGE_FILE_MACHINE_NATIVE IMAGE_FILE_MACHINE_AMD64
#else
#error Define these please!
#endif

//
// Registry Keys
//
#define RTL_REGISTRY_ABSOLUTE                               0
#define RTL_REGISTRY_SERVICES                               1
#define RTL_REGISTRY_CONTROL                                2
#define RTL_REGISTRY_WINDOWS_NT                             3
#define RTL_REGISTRY_DEVICEMAP                              4
#define RTL_REGISTRY_USER                                   5
#define RTL_REGISTRY_MAXIMUM                                6
#define RTL_REGISTRY_HANDLE                                 0x40000000
#define RTL_REGISTRY_OPTIONAL                               0x80000000
#define RTL_QUERY_REGISTRY_SUBKEY                           0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY                           0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED                         0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE                          0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND                         0x00000010
#define RTL_QUERY_REGISTRY_DIRECT                           0x00000020
#define RTL_QUERY_REGISTRY_DELETE                           0x00000040

//
// Versioning
//
#define VER_MINORVERSION                                    0x0000001
#define VER_MAJORVERSION                                    0x0000002
#define VER_BUILDNUMBER                                     0x0000004
#define VER_PLATFORMID                                      0x0000008
#define VER_SERVICEPACKMINOR                                0x0000010
#define VER_SERVICEPACKMAJOR                                0x0000020
#define VER_SUITENAME                                       0x0000040
#define VER_PRODUCT_TYPE                                    0x0000080
#define VER_PLATFORM_WIN32s                                 0
#define VER_PLATFORM_WIN32_WINDOWS                          1
#define VER_PLATFORM_WIN32_NT                               2
#define VER_EQUAL                                           1
#define VER_GREATER                                         2
#define VER_GREATER_EQUAL                                   3
#define VER_LESS                                            4
#define VER_LESS_EQUAL                                      5
#define VER_AND                                             6
#define VER_OR                                              7
#define VER_CONDITION_MASK                                  7
#define VER_NUM_BITS_PER_CONDITION_MASK                     3

//
// Timezone IDs
//
#define TIME_ZONE_ID_UNKNOWN                                0
#define TIME_ZONE_ID_STANDARD                               1
#define TIME_ZONE_ID_DAYLIGHT                               2

//
// Maximum Path Length
//
#define MAX_PATH                                            260

//
// RTL Lock Type (Critical Section or Resource)
//
#define RTL_CRITSECT_TYPE                                   0
#define RTL_RESOURCE_TYPE                                   1

//
// RtlAcquirePrivileges Flags
//
#define RTL_ACQUIRE_PRIVILEGE_IMPERSONATE                   1
#define RTL_ACQUIRE_PRIVILEGE_PROCESS                       2

#ifdef NTOS_MODE_USER

//
// String Hash Algorithms
//
#define HASH_STRING_ALGORITHM_DEFAULT                       0
#define HASH_STRING_ALGORITHM_X65599                        1
#define HASH_STRING_ALGORITHM_INVALID                       0xffffffff

//
// RtlDuplicateString Flags
//
#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE         1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING   2

//
// RtlFindCharInUnicodeString Flags
//
#define RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END        1
#define RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET 2
#define RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE    4

//
// RtlImageNtHeaderEx Flags
//
#define RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK          0x00000001

//
// RtlDosApplyFileIsolationRedirection_Ustr Flags
//
#define RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL  0x01

//
// Codepages
//
#define NLS_MB_CODE_PAGE_TAG                                NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG                            NlsMbOemCodePageTag
#define NLS_OEM_LEAD_BYTE_INFO                              NlsOemLeadByteInfo

//
// Activation Contexts
//
#define INVALID_ACTIVATION_CONTEXT                          (PVOID)0xFFFFFFFF

//
// C++ CONST casting
//
#if defined(__cplusplus)
#define RTL_CONST_CAST(type)                    const_cast<type>
#else
#define RTL_CONST_CAST(type)                    (type)
#endif

//
// Constant String Macro
//
#define RTL_CONSTANT_STRING(__SOURCE_STRING__)                  \
{                                                               \
    sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
    sizeof(__SOURCE_STRING__),                                  \
    (__SOURCE_STRING__)                                         \
}

//
// Constant Object Attributes Macro
//
#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a)                    \
{                                                               \
    sizeof(OBJECT_ATTRIBUTES),                                  \
    NULL,                                                       \
    RTL_CONST_CAST(PUNICODE_STRING)(n),                         \
    a,                                                          \
    NULL,                                                       \
    NULL                                                        \
}

#define RTL_INIT_OBJECT_ATTRIBUTES(n, a)                        \
    RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a)

#else /* NTOS_MODE_USER */
//
// Message Resource Flag
//
#define MESSAGE_RESOURCE_UNICODE                            0x0001

#endif /* !NTOS_MODE_USER */
#define MAXIMUM_LEADBYTES                                   12

//
// RTL Debug Queries
//
#define RTL_DEBUG_QUERY_MODULES                             0x01
#define RTL_DEBUG_QUERY_BACKTRACES                          0x02
#define RTL_DEBUG_QUERY_HEAPS                               0x04
#define RTL_DEBUG_QUERY_HEAP_TAGS                           0x08
#define RTL_DEBUG_QUERY_HEAP_BLOCKS                         0x10
#define RTL_DEBUG_QUERY_LOCKS                               0x20

//
// RTL Handle Flags
//
#define RTL_HANDLE_VALID                                    0x1

//
// RTL Atom Flags
//
#define RTL_ATOM_IS_PINNED                                  0x1

//
// Critical section lock bits
//
#define CS_LOCK_BIT                                         0x1
#define CS_LOCK_BIT_V                                       0x0
#define CS_LOCK_WAITER_WOKEN                                0x2
#define CS_LOCK_WAITER_INC                                  0x4

//
// Codepage Tags
//
#ifdef NTOS_MODE_USER
extern BOOLEAN NTSYSAPI NLS_MB_CODE_PAGE_TAG;
extern BOOLEAN NTSYSAPI NLS_MB_OEM_CODE_PAGE_TAG;

//
// Constant String Macro
//
#define RTL_CONSTANT_STRING(__SOURCE_STRING__)                  \
{                                                               \
    sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
    sizeof(__SOURCE_STRING__),                                  \
    (__SOURCE_STRING__)                                         \
}

#endif /* NTOS_MODE_USER */

#ifdef NTOS_MODE_USER

//
// Boot Status Data Field Types
//
typedef enum _RTL_BSD_ITEM_TYPE
{
    RtlBsdItemVersionNumber,
    RtlBsdItemProductType,
    RtlBsdItemAabEnabled,
    RtlBsdItemAabTimeout,
    RtlBsdItemBootGood,
    RtlBsdItemBootShutdown,
    RtlBsdItemMax
} RTL_BSD_ITEM_TYPE, *PRTL_BSD_ITEM_TYPE;

//
// Table and Compare result types
//
typedef enum _TABLE_SEARCH_RESULT
{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

typedef enum _RTL_GENERIC_COMPARE_RESULTS
{
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

#endif /* NTOS_MODE_USER */

//
// RTL Path Types
//
typedef enum _RTL_PATH_TYPE
{
    RtlPathTypeUnknown,
    RtlPathTypeUncAbsolute,
    RtlPathTypeDriveAbsolute,
    RtlPathTypeDriveRelative,
    RtlPathTypeRooted,
    RtlPathTypeRelative,
    RtlPathTypeLocalDevice,
    RtlPathTypeRootLocalDevice,
} RTL_PATH_TYPE;

#ifndef NTOS_MODE_USER

//
// Heap Information Class
//
typedef enum _HEAP_INFORMATION_CLASS
{
    HeapCompatibilityInformation,
    HeapEnableTerminationOnCorruption
} HEAP_INFORMATION_CLASS;

//
// Callback function for RTL Timers or Registered Waits
//
typedef VOID
(NTAPI *WAITORTIMERCALLBACKFUNC)(
    PVOID pvContext,
    BOOLEAN fTimerOrWaitFired
);

//
// Handler during Vectored RTL Exceptions
//
typedef LONG
(NTAPI *PVECTORED_EXCEPTION_HANDLER)(
    PEXCEPTION_POINTERS ExceptionPointers
);

//
// Worker Thread Callback for Rtl
//
typedef VOID
(NTAPI *WORKERCALLBACKFUNC)(
    _In_ PVOID Context
);

#else /* !NTOS_MODE_USER */

//
// RTL Library Allocation/Free Routines
//
typedef PVOID
(NTAPI *PRTL_ALLOCATE_STRING_ROUTINE)(
    SIZE_T NumberOfBytes
);

typedef PVOID
(NTAPI *PRTL_REALLOCATE_STRING_ROUTINE)(
    SIZE_T NumberOfBytes,
    PVOID Buffer
);

typedef
VOID
(NTAPI *PRTL_FREE_STRING_ROUTINE)(
    PVOID Buffer
);

extern const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine;
extern const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine;
extern const PRTL_REALLOCATE_STRING_ROUTINE RtlReallocateStringRoutine;

#endif /* NTOS_MODE_USER */

//
// Unhandled Exception Filter
//
typedef ULONG
(NTAPI *RTLP_UNHANDLED_EXCEPTION_FILTER)(
    _In_ struct _EXCEPTION_POINTERS *ExceptionInfo
);
typedef RTLP_UNHANDLED_EXCEPTION_FILTER *PRTLP_UNHANDLED_EXCEPTION_FILTER;

//
// Callback for RTL Heap Enumeration
//
typedef NTSTATUS
(NTAPI *PHEAP_ENUMERATION_ROUTINE)(
    _In_ PVOID HeapHandle,
    _In_ PVOID UserParam
);

//
// Thread and Process Start Routines for RtlCreateUserThread/Process
//
typedef ULONG (NTAPI *PTHREAD_START_ROUTINE)(
    PVOID Parameter
);

typedef VOID
(NTAPI *PRTL_BASE_PROCESS_START_ROUTINE)(
    PTHREAD_START_ROUTINE StartAddress,
    PVOID Parameter
);

//
// Worker Start/Exit Function
//
typedef NTSTATUS
(NTAPI *PRTL_START_POOL_THREAD)(
    _In_ PTHREAD_START_ROUTINE Function,
    _In_ PVOID Parameter,
    _Out_ PHANDLE ThreadHandle
);

typedef NTSTATUS
(NTAPI *PRTL_EXIT_POOL_THREAD)(
    _In_ NTSTATUS ExitStatus
);

//
// Declare empty structure definitions so that they may be referenced by
// routines before they are defined
//
struct _RTL_AVL_TABLE;
struct _RTL_GENERIC_TABLE;
struct _RTL_RANGE;

//
// Routines and callbacks for the RTL AVL/Generic Table package
//
#ifdef NTOS_MODE_USER
typedef NTSTATUS
(NTAPI RTL_AVL_MATCH_FUNCTION)(
    struct _RTL_AVL_TABLE *Table,
    PVOID UserData,
    PVOID MatchData
);
typedef RTL_AVL_MATCH_FUNCTION *PRTL_AVL_MATCH_FUNCTION;

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI RTL_AVL_COMPARE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
);
typedef RTL_AVL_COMPARE_ROUTINE *PRTL_AVL_COMPARE_ROUTINE;

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI RTL_GENERIC_COMPARE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
);
typedef RTL_GENERIC_COMPARE_ROUTINE *PRTL_GENERIC_COMPARE_ROUTINE;

typedef PVOID
(NTAPI RTL_GENERIC_ALLOCATE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    CLONG ByteSize
);
typedef RTL_GENERIC_ALLOCATE_ROUTINE *PRTL_GENERIC_ALLOCATE_ROUTINE;

typedef PVOID
(NTAPI RTL_AVL_ALLOCATE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    CLONG ByteSize
);
typedef RTL_AVL_ALLOCATE_ROUTINE *PRTL_AVL_ALLOCATE_ROUTINE;

typedef VOID
(NTAPI RTL_GENERIC_FREE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    PVOID Buffer
);
typedef RTL_GENERIC_FREE_ROUTINE *PRTL_GENERIC_FREE_ROUTINE;

typedef VOID
(NTAPI RTL_AVL_FREE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    PVOID Buffer
);
typedef RTL_AVL_FREE_ROUTINE *PRTL_AVL_FREE_ROUTINE;

#ifdef RTL_USE_AVL_TABLES
#undef  RTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef  RTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef  RTL_GENERIC_FREE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE

#define  RTL_GENERIC_COMPARE_ROUTINE     RTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_COMPARE_ROUTINE    PRTL_AVL_COMPARE_ROUTINE
#define  RTL_GENERIC_ALLOCATE_ROUTINE    RTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE   PRTL_AVL_ALLOCATE_ROUTINE
#define  RTL_GENERIC_FREE_ROUTINE        RTL_AVL_FREE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE       PRTL_AVL_FREE_ROUTINE
#endif /* RTL_USE_AVL_TABLES */

#endif /* NTOS_MODE_USER */

//
// RTL Query Registry callback
//
#ifdef NTOS_MODE_USER
typedef NTSTATUS
(NTAPI *PRTL_QUERY_REGISTRY_ROUTINE)(
    _In_ PWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueLength,
    _In_ PVOID Context,
    _In_ PVOID EntryContext
);
#endif

//
// RTL Secure Memory callbacks
//
#ifdef NTOS_MODE_USER
typedef NTSTATUS
(NTAPI *PRTL_SECURE_MEMORY_CACHE_CALLBACK)(
    _In_ PVOID Address,
    _In_ SIZE_T Length
);
#endif

//
// RTL Range List callbacks
//
#ifdef NTOS_MODE_USER
typedef BOOLEAN
(NTAPI *PRTL_CONFLICT_RANGE_CALLBACK)(
    PVOID Context,
    struct _RTL_RANGE *Range
);

//
// Custom Heap Commit Routine for RtlCreateHeap
//
typedef NTSTATUS
(NTAPI * PRTL_HEAP_COMMIT_ROUTINE)(
    _In_ PVOID Base,
    _Inout_ PVOID *CommitAddress,
    _Inout_ PSIZE_T CommitSize
);

//
// Parameters for RtlCreateHeap
//
typedef struct _RTL_HEAP_PARAMETERS
{
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;

//
// RTL Bitmap structures
//
typedef struct _RTL_BITMAP
{
    ULONG SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _RTL_BITMAP_RUN
{
    ULONG StartingIndex;
    ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

//
// RtlGenerateXxxName context
//
typedef struct _GENERATE_NAME_CONTEXT
{
    USHORT Checksum;
    BOOLEAN CheckSumInserted;
    UCHAR NameLength;
    WCHAR NameBuffer[8];
    ULONG ExtensionLength;
    WCHAR ExtensionBuffer[4];
    ULONG LastIndexValue;
} GENERATE_NAME_CONTEXT, *PGENERATE_NAME_CONTEXT;

//
// RTL Splay and Balanced Links structures
//
typedef struct _RTL_SPLAY_LINKS
{
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

typedef struct _RTL_BALANCED_LINKS
{
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS, *PRTL_BALANCED_LINKS;

//
// RTL Avl/Generic Tables
//
#ifndef RTL_USE_AVL_TABLES
typedef struct _RTL_GENERIC_TABLE
{
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;
#endif /* !RTL_USE_AVL_TABLES */

typedef struct _RTL_AVL_TABLE
{
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE, *PRTL_AVL_TABLE;

#ifdef RTL_USE_AVL_TABLES
#undef  RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define  RTL_GENERIC_TABLE  RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE PRTL_AVL_TABLE
#endif /* RTL_USE_AVL_TABLES */

//
// RTL Compression Buffer
//
typedef struct _COMPRESSED_DATA_INFO {
    USHORT CompressionFormatAndEngine;
    UCHAR CompressionUnitShift;
    UCHAR ChunkShift;
    UCHAR ClusterShift;
    UCHAR Reserved;
    USHORT NumberOfChunks;
    ULONG CompressedChunkSizes[ANYSIZE_ARRAY];
} COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;

//
// RtlQueryRegistry Data
//
typedef struct _RTL_QUERY_REGISTRY_TABLE
{
    PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
    ULONG Flags;
    PCWSTR Name;
    PVOID EntryContext;
    ULONG DefaultType;
    PVOID DefaultData;
    ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

//
// RTL Unicode Table Structures
//
typedef struct _UNICODE_PREFIX_TABLE_ENTRY
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    struct _UNICODE_PREFIX_TABLE_ENTRY *NextPrefixTree;
    struct _UNICODE_PREFIX_TABLE_ENTRY *CaseMatch;
    RTL_SPLAY_LINKS Links;
    PUNICODE_STRING Prefix;
} UNICODE_PREFIX_TABLE_ENTRY, *PUNICODE_PREFIX_TABLE_ENTRY;

typedef struct _UNICODE_PREFIX_TABLE
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    PUNICODE_PREFIX_TABLE_ENTRY NextPrefixTree;
    PUNICODE_PREFIX_TABLE_ENTRY LastNextEntry;
} UNICODE_PREFIX_TABLE, *PUNICODE_PREFIX_TABLE;

//
// Time Structure for RTL Time calls
//
typedef struct _TIME_FIELDS
{
    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

//
// Activation Context Frame
//
typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME
{
    struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *Previous;
    PACTIVATION_CONTEXT ActivationContext;
    ULONG Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_BASIC
{
    SIZE_T Size;
    ULONG Format;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;
} RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_BASIC, *PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_BASIC;

typedef struct _RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED
{
    SIZE_T Size;
    ULONG Format;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;
    PVOID Extra1;
    PVOID Extra2;
    PVOID Extra3;
    PVOID Extra4;
} RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED, *PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED;

typedef RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME;
typedef PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;
    ULONG_PTR Cookie;
    PVOID ActivationStackBackTrace[8];
} RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME;

#if (NTDDI_VERSION >= NTDDI_WS03)
typedef struct _ACTIVATION_CONTEXT_STACK
{
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME ActiveFrame;
    LIST_ENTRY FrameListCache;
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    ULONG StackId;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;
#else
typedef struct _ACTIVATION_CONTEXT_STACK
{
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    PVOID ActiveFrame;
    LIST_ENTRY FrameListCache;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;
#endif

typedef struct _ACTIVATION_CONTEXT_DATA
{
    ULONG Magic;
    ULONG HeaderSize;
    ULONG FormatVersion;
    ULONG TotalSize;
    ULONG DefaultTocOffset;
    ULONG ExtendedTocOffset;
    ULONG AssemblyRosterOffset;
    ULONG Flags;
} ACTIVATION_CONTEXT_DATA, *PACTIVATION_CONTEXT_DATA;

typedef struct _ACTIVATION_CONTEXT_STACK_FRAMELIST
{
    ULONG Magic;
    ULONG FramesInUse;
    LIST_ENTRY Links;
    ULONG Flags;
    ULONG NotFramesInUse;
    RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frames[32];
} ACTIVATION_CONTEXT_STACK_FRAMELIST, *PACTIVATION_CONTEXT_STACK_FRAMELIST;

#endif /* NTOS_MODE_USER */

//
// ACE Structure
//
typedef struct _ACE
{
    ACE_HEADER Header;
    ACCESS_MASK AccessMask;
} ACE, *PACE;

//
// Information Structures for RTL Debug Functions
//
typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    ULONG Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    CHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX
{
    ULONG NextOffset;
    RTL_PROCESS_MODULE_INFORMATION BaseInfo;
    ULONG ImageCheckSum;
    ULONG TimeDateStamp;
    PVOID DefaultBase;
} RTL_PROCESS_MODULE_INFORMATION_EX, *PRTL_PROCESS_MODULE_INFORMATION_EX;

typedef struct _RTL_HEAP_TAG_INFO
{
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
} RTL_HEAP_TAG_INFO, *PRTL_HEAP_TAG_INFO;

typedef struct _RTL_HEAP_USAGE_ENTRY
{
    struct _RTL_HEAP_USAGE_ENTRY *Next;
    PVOID Address;
    SIZE_T Size;
    USHORT AllocatorBackTraceIndex;
    USHORT TagIndex;
} RTL_HEAP_USAGE_ENTRY, *PRTL_HEAP_USAGE_ENTRY;

typedef struct _RTL_HEAP_USAGE
{
    ULONG Length;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
    SIZE_T BytesReserved;
    SIZE_T BytesReservedMaximum;
    PRTL_HEAP_USAGE_ENTRY Entries;
    PRTL_HEAP_USAGE_ENTRY AddedEntries;
    PRTL_HEAP_USAGE_ENTRY RemovedEntries;
    ULONG_PTR Reserved[8];
} RTL_HEAP_USAGE, *PRTL_HEAP_USAGE;

typedef struct _RTL_HEAP_WALK_ENTRY
{
    PVOID DataAddress;
    SIZE_T DataSize;
    UCHAR OverheadBytes;
    UCHAR SegmentIndex;
    USHORT Flags;
    union
    {
        struct
        {
            SIZE_T Settable;
            USHORT TagIndex;
            USHORT AllocatorBackTraceIndex;
            ULONG Reserved[2];
        } Block;
        struct
        {
            ULONG_PTR CommittedSize;
            ULONG_PTR UnCommittedSize;
            PVOID FirstEntry;
            PVOID LastEntry;
        } Segment;
    };
} RTL_HEAP_WALK_ENTRY, *PRTL_HEAP_WALK_ENTRY;

typedef struct _RTL_HEAP_ENTRY
{
    SIZE_T Size;
    USHORT Flags;
    USHORT AllocatorBackTraceIndex;
    union
    {
        struct
        {
            SIZE_T Settable;
            ULONG Tag;
        } s1;
        struct
        {
            SIZE_T CommittedSize;
            PVOID FirstBlock;
        } s2;
    } u;
} RTL_HEAP_ENTRY, *PRTL_HEAP_ENTRY;

typedef struct _RTL_HEAP_TAG
{
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
    USHORT TagIndex;
    USHORT CreatorBackTraceIndex;
    WCHAR TagName[24];
} RTL_HEAP_TAG, *PRTL_HEAP_TAG;

typedef struct _RTL_HEAP_INFORMATION
{
    PVOID BaseAddress;
    ULONG Flags;
    USHORT EntryOverhead;
    USHORT CreatorBackTraceIndex;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
    ULONG NumberOfTags;
    ULONG NumberOfEntries;
    ULONG NumberOfPseudoTags;
    ULONG PseudoTagGranularity;
    ULONG Reserved[4];
    PRTL_HEAP_TAG Tags;
    PRTL_HEAP_ENTRY Entries;
} RTL_HEAP_INFORMATION, *PRTL_HEAP_INFORMATION;

typedef struct _RTL_PROCESS_HEAPS
{
    ULONG NumberOfHeaps;
    RTL_HEAP_INFORMATION Heaps[1];
} RTL_PROCESS_HEAPS, *PRTL_PROCESS_HEAPS;

typedef struct _RTL_PROCESS_LOCK_INFORMATION
{
    PVOID Address;
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    ULONG OwnerThreadId;
    ULONG ActiveCount;
    ULONG ContentionCount;
    ULONG EntryCount;
    ULONG RecursionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
} RTL_PROCESS_LOCK_INFORMATION, *PRTL_PROCESS_LOCK_INFORMATION;

typedef struct _RTL_PROCESS_LOCKS
{
    ULONG NumberOfLocks;
    RTL_PROCESS_LOCK_INFORMATION Locks[1];
} RTL_PROCESS_LOCKS, *PRTL_PROCESS_LOCKS;

typedef struct _RTL_PROCESS_BACKTRACE_INFORMATION
{
    PVOID SymbolicBackTrace;
    ULONG TraceCount;
    USHORT Index;
    USHORT Depth;
    PVOID BackTrace[16];
} RTL_PROCESS_BACKTRACE_INFORMATION, *PRTL_PROCESS_BACKTRACE_INFORMATION;

typedef struct _RTL_PROCESS_BACKTRACES
{
    ULONG CommittedMemory;
    ULONG ReservedMemory;
    ULONG NumberOfBackTraceLookups;
    ULONG NumberOfBackTraces;
    RTL_PROCESS_BACKTRACE_INFORMATION BackTraces[1];
} RTL_PROCESS_BACKTRACES, *PRTL_PROCESS_BACKTRACES;

typedef struct _RTL_PROCESS_VERIFIER_OPTIONS
{
    ULONG SizeStruct;
    ULONG Option;
    UCHAR OptionData[1];
    //
    // Option array continues below
    //
} RTL_PROCESS_VERIFIER_OPTIONS, *PRTL_PROCESS_VERIFIER_OPTIONS;

typedef struct _RTL_DEBUG_INFORMATION
{
    HANDLE SectionHandleClient;
    PVOID ViewBaseClient;
    PVOID ViewBaseTarget;
    ULONG ViewBaseDelta;
    HANDLE EventPairClient;
    PVOID EventPairTarget;
    HANDLE TargetProcessId;
    HANDLE TargetThreadHandle;
    ULONG Flags;
    ULONG OffsetFree;
    ULONG CommitSize;
    ULONG ViewSize;
    union
    {
        PRTL_PROCESS_MODULES Modules;
        PRTL_PROCESS_MODULE_INFORMATION_EX ModulesEx;
    };
    PRTL_PROCESS_BACKTRACES BackTraces;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_PROCESS_LOCKS Locks;
    HANDLE SpecificHeap;
    HANDLE TargetProcessHandle;
    RTL_PROCESS_VERIFIER_OPTIONS VerifierOptions;
    HANDLE ProcessHeap;
    HANDLE CriticalSectionHandle;
    HANDLE CriticalSectionOwnerThread;
    PVOID Reserved[4];
} RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION;

//
// Unload Event Trace Structure for RtlGetUnloadEventTrace
//
typedef struct _RTL_UNLOAD_EVENT_TRACE
{
    PVOID BaseAddress;
    ULONG SizeOfImage;
    ULONG Sequence;
    ULONG TimeDateStamp;
    ULONG CheckSum;
    WCHAR ImageName[32];
} RTL_UNLOAD_EVENT_TRACE, *PRTL_UNLOAD_EVENT_TRACE;

//
// RTL Handle Structures
//
typedef struct _RTL_HANDLE_TABLE_ENTRY
{
    union
    {
        ULONG Flags;
        struct _RTL_HANDLE_TABLE_ENTRY *NextFree;
    };
} RTL_HANDLE_TABLE_ENTRY, *PRTL_HANDLE_TABLE_ENTRY;

typedef struct _RTL_HANDLE_TABLE
{
    ULONG MaximumNumberOfHandles;
    ULONG SizeOfHandleTableEntry;
    ULONG Reserved[2];
    PRTL_HANDLE_TABLE_ENTRY FreeHandles;
    PRTL_HANDLE_TABLE_ENTRY CommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY UnCommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY MaxReservedHandles;
} RTL_HANDLE_TABLE, *PRTL_HANDLE_TABLE;

#ifdef NTOS_MODE_USER
//
// Exception Record
//
typedef struct _EXCEPTION_REGISTRATION_RECORD
{
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;
#endif /* NTOS_MODE_USER */

//
// Current Directory Structures
//
typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _RTLP_CURDIR_REF
{
    LONG RefCount;
    HANDLE Handle;
} RTLP_CURDIR_REF, *PRTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U
{
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_PERTHREAD_CURDIR
{
    PRTL_DRIVE_LETTER_CURDIR CurrentDirectories;
    PUNICODE_STRING ImageName;
    PVOID Environment;
} RTL_PERTHREAD_CURDIR, *PRTL_PERTHREAD_CURDIR;

//
// Private State structure for RtlAcquirePrivilege/RtlReleasePrivilege
//
typedef struct _RTL_ACQUIRE_STATE
{
    HANDLE Token;
    HANDLE OldImpersonationToken;
    PTOKEN_PRIVILEGES OldPrivileges;
    PTOKEN_PRIVILEGES NewPrivileges;
    ULONG Flags;
    UCHAR OldPrivBuffer[1024];
} RTL_ACQUIRE_STATE, *PRTL_ACQUIRE_STATE;

#ifndef NTOS_MODE_USER

//
// RTL Critical Section Structures
//
typedef struct _RTL_CRITICAL_SECTION_DEBUG
{
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[2];
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;

typedef struct _RTL_CRITICAL_SECTION
{
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;

#endif /* !NTOS_MODE_USER */

//
// RTL Private Heap Structures
//
typedef struct _HEAP_LOCK
{
    union
    {
        RTL_CRITICAL_SECTION CriticalSection;
#ifndef NTOS_MODE_USER
        ERESOURCE Resource;
#endif
        UCHAR Padding[0x68]; /* Max ERESOURCE size for x64 build. Needed because RTL is built only once */
    };
} HEAP_LOCK, *PHEAP_LOCK;

//
// RTL Range List Structures
//
typedef struct _RTL_RANGE_LIST
{
    LIST_ENTRY ListHead;
    ULONG Flags;
    ULONG Count;
    ULONG Stamp;
} RTL_RANGE_LIST, *PRTL_RANGE_LIST;

typedef struct _RTL_RANGE
{
    ULONGLONG Start;
    ULONGLONG End;
    PVOID UserData;
    PVOID Owner;
    UCHAR Attributes;
    UCHAR Flags;
} RTL_RANGE, *PRTL_RANGE;

typedef struct _RANGE_LIST_ITERATOR
{
    PLIST_ENTRY RangeListHead;
    PLIST_ENTRY MergedHead;
    PVOID Current;
    ULONG Stamp;
} RTL_RANGE_LIST_ITERATOR, *PRTL_RANGE_LIST_ITERATOR;

//
// RTL Resource
//
#define RTL_RESOURCE_FLAG_LONG_TERM ((ULONG)0x00000001)

typedef struct _RTL_RESOURCE
{
    RTL_CRITICAL_SECTION Lock;
    HANDLE SharedSemaphore;
    ULONG SharedWaiters;
    HANDLE ExclusiveSemaphore;
    ULONG ExclusiveWaiters;
    LONG NumberActive;
    HANDLE OwningThread;
    ULONG TimeoutBoost;
    PVOID DebugInfo;
} RTL_RESOURCE, *PRTL_RESOURCE;

//
// Structures for RtlCreateUserProcess
//
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;
    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    SIZE_T EnvironmentSize;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
    SIZE_T EnvironmentVersion;
#endif
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _RTL_USER_PROCESS_INFORMATION
{
    ULONG Size;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

//
// RTL Atom Table Structures
//
typedef struct _RTL_ATOM_TABLE_ENTRY
{
    struct _RTL_ATOM_TABLE_ENTRY *HashLink;
    USHORT HandleIndex;
    USHORT Atom;
    USHORT ReferenceCount;
    UCHAR Flags;
    UCHAR NameLength;
    WCHAR Name[1];
} RTL_ATOM_TABLE_ENTRY, *PRTL_ATOM_TABLE_ENTRY;

typedef struct _RTL_ATOM_TABLE
{
    ULONG Signature;
    union
    {
#ifdef NTOS_MODE_USER
        RTL_CRITICAL_SECTION CriticalSection;
#else
        FAST_MUTEX FastMutex;
#endif
    };
    union
    {
#ifdef NTOS_MODE_USER
        RTL_HANDLE_TABLE RtlHandleTable;
#else
        PHANDLE_TABLE ExHandleTable;
#endif
    };
    ULONG NumberOfBuckets;
    PRTL_ATOM_TABLE_ENTRY Buckets[1];
} RTL_ATOM_TABLE, *PRTL_ATOM_TABLE;

#ifndef _WINBASE_
//
// System Time and Timezone Structures
//
typedef struct _SYSTEMTIME
{
    USHORT wYear;
    USHORT wMonth;
    USHORT wDayOfWeek;
    USHORT wDay;
    USHORT wHour;
    USHORT wMinute;
    USHORT wSecond;
    USHORT wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _TIME_ZONE_INFORMATION
{
    LONG Bias;
    WCHAR StandardName[32];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;
#endif /* !_WINBASE_ */

//
// Native version of Timezone Structure
//
typedef LPTIME_ZONE_INFORMATION PRTL_TIME_ZONE_INFORMATION;

//
// Hotpatch Header
//
typedef struct _RTL_PATCH_HEADER
{
    LIST_ENTRY PatchList;
    PVOID PatchImageBase;
    struct _RTL_PATCH_HEADER *NextPath;
    ULONG PatchFlags;
    LONG PatchRefCount;
    struct _HOTPATCH_HEADER *HotpatchHeader;
    UNICODE_STRING TargetDllName;
    PVOID TargetDllBase;
    PLDR_DATA_TABLE_ENTRY TargetLdrDataTableEntry;
    PLDR_DATA_TABLE_ENTRY PatchLdrDataTableEntry;
    struct _SYSTEM_HOTPATCH_CODE_INFORMATION *CodeInfo;
} RTL_PATCH_HEADER, *PRTL_PATCH_HEADER;

//
// Header for NLS Files
//
typedef struct _NLS_FILE_HEADER
{
    USHORT HeaderSize;
    USHORT CodePage;
    USHORT MaximumCharacterSize;
    USHORT DefaultChar;
    USHORT UniDefaultChar;
    USHORT TransDefaultChar;
    USHORT TransUniDefaultChar;
    UCHAR LeadByte[MAXIMUM_LEADBYTES];
} NLS_FILE_HEADER, *PNLS_FILE_HEADER;

//
// Stack Traces
//
typedef struct _RTL_STACK_TRACE_ENTRY
{
    struct _RTL_STACK_TRACE_ENTRY *HashChain;
    ULONG TraceCount;
    USHORT Index;
    USHORT Depth;
    PVOID BackTrace[32];
} RTL_STACK_TRACE_ENTRY, *PRTL_STACK_TRACE_ENTRY;

typedef struct _STACK_TRACE_DATABASE
{
    RTL_CRITICAL_SECTION CriticalSection;
} STACK_TRACE_DATABASE, *PSTACK_TRACE_DATABASE;

typedef struct _RTL_TRACE_BLOCK
{
    ULONG Magic;
    ULONG Count;
    ULONG Size;
    ULONG UserCount;
    ULONG UserSize;
    PVOID UserContext;
    struct _RTL_TRACE_BLOCK *Next;
    PVOID *Trace;
} RTL_TRACE_BLOCK, *PRTL_TRACE_BLOCK;

//
// Auto-Managed Rtl* String Buffer
//
typedef struct _RTL_BUFFER
{
    PUCHAR Buffer;
    PUCHAR StaticBuffer;
    SIZE_T Size;
    SIZE_T StaticSize;
    SIZE_T ReservedForAllocatedSize;
    PVOID ReservedForIMalloc;
} RTL_BUFFER, *PRTL_BUFFER;

typedef struct _RTL_UNICODE_STRING_BUFFER
{
    UNICODE_STRING String;
    RTL_BUFFER ByteBuffer;
    WCHAR MinimumStaticBufferForTerminalNul;
} RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

#ifndef NTOS_MODE_USER

//
// Message Resource Entry, Block and Data
//
typedef struct _MESSAGE_RESOURCE_ENTRY
{
    USHORT Length;
    USHORT Flags;
    UCHAR Text[ANYSIZE_ARRAY];
} MESSAGE_RESOURCE_ENTRY, *PMESSAGE_RESOURCE_ENTRY;

typedef struct _MESSAGE_RESOURCE_BLOCK
{
    ULONG LowId;
    ULONG HighId;
    ULONG OffsetToEntries;
} MESSAGE_RESOURCE_BLOCK, *PMESSAGE_RESOURCE_BLOCK;

typedef struct _MESSAGE_RESOURCE_DATA
{
    ULONG NumberOfBlocks;
    MESSAGE_RESOURCE_BLOCK Blocks[ANYSIZE_ARRAY];
} MESSAGE_RESOURCE_DATA, *PMESSAGE_RESOURCE_DATA;

#endif /* !NTOS_MODE_USER */

#ifdef NTOS_MODE_USER

//
// Memory Stream
//
#ifndef CONST_VTBL
#ifdef CONST_VTABLE
#define CONST_VTBL const
#else
#define CONST_VTBL
#endif
#endif

struct IStreamVtbl;
struct IStream;
struct tagSTATSTG;

typedef struct _RTL_MEMORY_STREAM RTL_MEMORY_STREAM, *PRTL_MEMORY_STREAM;

typedef VOID 
(NTAPI *PRTL_MEMORY_STREAM_FINAL_RELEASE_ROUTINE)(
    _In_ PRTL_MEMORY_STREAM Stream
);

struct _RTL_MEMORY_STREAM
{
    CONST_VTBL struct IStreamVtbl *Vtbl;
    LONG RefCount;
    ULONG Unk1;
    PVOID Current;
    PVOID Start;
    PVOID End;
    PRTL_MEMORY_STREAM_FINAL_RELEASE_ROUTINE FinalRelease;
    HANDLE ProcessHandle;
};

#endif /* NTOS_MODE_USER */

#ifdef __cplusplus
}
#endif

#endif /* !_RTLTYPES_H */
