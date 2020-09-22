/******************************************************************************
 *                           Runtime Library Types                            *
 ******************************************************************************/

$if (_WDMDDK_)
#define RTL_REGISTRY_ABSOLUTE             0
#define RTL_REGISTRY_SERVICES             1
#define RTL_REGISTRY_CONTROL              2
#define RTL_REGISTRY_WINDOWS_NT           3
#define RTL_REGISTRY_DEVICEMAP            4
#define RTL_REGISTRY_USER                 5
#define RTL_REGISTRY_MAXIMUM              6
#define RTL_REGISTRY_HANDLE               0x40000000
#define RTL_REGISTRY_OPTIONAL             0x80000000

/* RTL_QUERY_REGISTRY_TABLE.Flags */
#define RTL_QUERY_REGISTRY_SUBKEY         0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY         0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED       0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE        0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND       0x00000010
#define RTL_QUERY_REGISTRY_DIRECT         0x00000020
#define RTL_QUERY_REGISTRY_DELETE         0x00000040
#define RTL_QUERY_REGISTRY_TYPECHECK      0x00000100

#define RTL_QUERY_REGISTRY_TYPECHECK_SHIFT 24

#define HASH_STRING_ALGORITHM_DEFAULT     0
#define HASH_STRING_ALGORITHM_X65599      1
#define HASH_STRING_ALGORITHM_INVALID     0xffffffff

typedef struct _RTL_BITMAP {
  ULONG SizeOfBitMap;
  PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _RTL_BITMAP_RUN {
  ULONG StartingIndex;
  ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

_Function_class_(RTL_QUERY_REGISTRY_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
typedef NTSTATUS
(NTAPI RTL_QUERY_REGISTRY_ROUTINE)(
  _In_z_ PWSTR ValueName,
  _In_ ULONG ValueType,
  _In_reads_bytes_opt_(ValueLength) PVOID ValueData,
  _In_ ULONG ValueLength,
  _In_opt_ PVOID Context,
  _In_opt_ PVOID EntryContext);
typedef RTL_QUERY_REGISTRY_ROUTINE *PRTL_QUERY_REGISTRY_ROUTINE;

typedef struct _RTL_QUERY_REGISTRY_TABLE {
  PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
  ULONG Flags;
  PCWSTR Name;
  PVOID EntryContext;
  ULONG DefaultType;
  PVOID DefaultData;
  ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

typedef struct _TIME_FIELDS {
  CSHORT Year;
  CSHORT Month;
  CSHORT Day;
  CSHORT Hour;
  CSHORT Minute;
  CSHORT Second;
  CSHORT Milliseconds;
  CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

/* Slist Header */
#ifndef _SLIST_HEADER_
#define _SLIST_HEADER_

#if defined(_WIN64)

typedef struct DECLSPEC_ALIGN(16) _SLIST_ENTRY {
  struct _SLIST_ENTRY *Next;
} SLIST_ENTRY, *PSLIST_ENTRY;

typedef struct _SLIST_ENTRY32 {
  ULONG Next;
} SLIST_ENTRY32, *PSLIST_ENTRY32;

typedef union DECLSPEC_ALIGN(16) _SLIST_HEADER {
  _ANONYMOUS_STRUCT struct {
    ULONGLONG Alignment;
    ULONGLONG Region;
  } DUMMYSTRUCTNAME;
  struct {
    ULONGLONG Depth:16;
    ULONGLONG Sequence:9;
    ULONGLONG NextEntry:39;
    ULONGLONG HeaderType:1;
    ULONGLONG Init:1;
    ULONGLONG Reserved:59;
    ULONGLONG Region:3;
  } Header8;
  struct {
    ULONGLONG Depth:16;
    ULONGLONG Sequence:48;
    ULONGLONG HeaderType:1;
    ULONGLONG Init:1;
    ULONGLONG Reserved:2;
    ULONGLONG NextEntry:60;
  } Header16;
  struct {
    ULONGLONG Depth:16;
    ULONGLONG Sequence:48;
    ULONGLONG HeaderType:1;
    ULONGLONG Reserved:3;
    ULONGLONG NextEntry:60;
  } HeaderX64;
} SLIST_HEADER, *PSLIST_HEADER;

typedef union _SLIST_HEADER32 {
  ULONGLONG Alignment;
  _ANONYMOUS_STRUCT struct {
    SLIST_ENTRY32 Next;
    USHORT Depth;
    USHORT Sequence;
  } DUMMYSTRUCTNAME;
} SLIST_HEADER32, *PSLIST_HEADER32;

#else

#define SLIST_ENTRY SINGLE_LIST_ENTRY
#define _SLIST_ENTRY _SINGLE_LIST_ENTRY
#define PSLIST_ENTRY PSINGLE_LIST_ENTRY

typedef SLIST_ENTRY SLIST_ENTRY32, *PSLIST_ENTRY32;

typedef union _SLIST_HEADER {
  ULONGLONG Alignment;
  _ANONYMOUS_STRUCT struct {
    SLIST_ENTRY Next;
    USHORT Depth;
    USHORT Sequence;
  } DUMMYSTRUCTNAME;
} SLIST_HEADER, *PSLIST_HEADER;

typedef SLIST_HEADER SLIST_HEADER32, *PSLIST_HEADER32;

#endif /* defined(_WIN64) */

#endif /* _SLIST_HEADER_ */

/* Exception record flags */
#define EXCEPTION_NONCONTINUABLE  0x01
#define EXCEPTION_UNWINDING       0x02
#define EXCEPTION_EXIT_UNWIND     0x04
#define EXCEPTION_STACK_INVALID   0x08
#define EXCEPTION_NESTED_CALL     0x10
#define EXCEPTION_TARGET_UNWIND   0x20
#define EXCEPTION_COLLIDED_UNWIND 0x40
#define EXCEPTION_UNWIND (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | \
                          EXCEPTION_TARGET_UNWIND | EXCEPTION_COLLIDED_UNWIND)

#define IS_UNWINDING(Flag) ((Flag & EXCEPTION_UNWIND) != 0)
#define IS_DISPATCHING(Flag) ((Flag & EXCEPTION_UNWIND) == 0)
#define IS_TARGET_UNWIND(Flag) (Flag & EXCEPTION_TARGET_UNWIND)

#define EXCEPTION_MAXIMUM_PARAMETERS 15

/* Exception records */
typedef struct _EXCEPTION_RECORD {
  NTSTATUS ExceptionCode;
  ULONG ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID ExceptionAddress;
  ULONG NumberParameters;
  ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

typedef struct _EXCEPTION_RECORD32 {
  NTSTATUS ExceptionCode;
  ULONG ExceptionFlags;
  ULONG ExceptionRecord;
  ULONG ExceptionAddress;
  ULONG NumberParameters;
  ULONG ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD32, *PEXCEPTION_RECORD32;

typedef struct _EXCEPTION_RECORD64 {
  NTSTATUS ExceptionCode;
  ULONG ExceptionFlags;
  ULONG64 ExceptionRecord;
  ULONG64 ExceptionAddress;
  ULONG NumberParameters;
  ULONG __unusedAlignment;
  ULONG64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD64, *PEXCEPTION_RECORD64;

typedef struct _EXCEPTION_POINTERS {
  PEXCEPTION_RECORD ExceptionRecord;
  PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

#ifdef _NTSYSTEM_
extern BOOLEAN NlsMbCodePageTag;
#define NLS_MB_CODE_PAGE_TAG NlsMbCodePageTag
extern BOOLEAN NlsMbOemCodePageTag;
#define NLS_MB_OEM_CODE_PAGE_TAG NlsMbOemCodePageTag
#else
__CREATE_NTOS_DATA_IMPORT_ALIAS(NlsMbCodePageTag)
extern BOOLEAN *NlsMbCodePageTag;
#define NLS_MB_CODE_PAGE_TAG (*NlsMbCodePageTag)
__CREATE_NTOS_DATA_IMPORT_ALIAS(NlsMbOemCodePageTag)
extern BOOLEAN *NlsMbOemCodePageTag;
#define NLS_MB_OEM_CODE_PAGE_TAG (*NlsMbOemCodePageTag)
#endif

#define SHORT_LEAST_SIGNIFICANT_BIT       0
#define SHORT_MOST_SIGNIFICANT_BIT        1

#define LONG_LEAST_SIGNIFICANT_BIT        0
#define LONG_3RD_MOST_SIGNIFICANT_BIT     1
#define LONG_2ND_MOST_SIGNIFICANT_BIT     2
#define LONG_MOST_SIGNIFICANT_BIT         3

#define RTLVERLIB_DDI(x) Wdmlib##x

typedef BOOLEAN
(*PFN_RTL_IS_NTDDI_VERSION_AVAILABLE)(
  _In_ ULONG Version);

typedef BOOLEAN
(*PFN_RTL_IS_SERVICE_PACK_VERSION_INSTALLED)(
  _In_ ULONG Version);

typedef struct _OSVERSIONINFOA {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  CHAR szCSDVersion[128];
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;

typedef struct _OSVERSIONINFOW {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  WCHAR szCSDVersion[128];
} OSVERSIONINFOW, *POSVERSIONINFOW, *LPOSVERSIONINFOW, RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;

typedef struct _OSVERSIONINFOEXA {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  CHAR szCSDVersion[128];
  USHORT wServicePackMajor;
  USHORT wServicePackMinor;
  USHORT wSuiteMask;
  UCHAR wProductType;
  UCHAR wReserved;
} OSVERSIONINFOEXA, *POSVERSIONINFOEXA, *LPOSVERSIONINFOEXA;

typedef struct _OSVERSIONINFOEXW {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  WCHAR szCSDVersion[128];
  USHORT wServicePackMajor;
  USHORT wServicePackMinor;
  USHORT wSuiteMask;
  UCHAR wProductType;
  UCHAR wReserved;
} OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW, RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

#ifdef UNICODE
typedef OSVERSIONINFOEXW OSVERSIONINFOEX;
typedef POSVERSIONINFOEXW POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXW LPOSVERSIONINFOEX;
typedef OSVERSIONINFOW OSVERSIONINFO;
typedef POSVERSIONINFOW POSVERSIONINFO;
typedef LPOSVERSIONINFOW LPOSVERSIONINFO;
#else
typedef OSVERSIONINFOEXA OSVERSIONINFOEX;
typedef POSVERSIONINFOEXA POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXA LPOSVERSIONINFOEX;
typedef OSVERSIONINFOA OSVERSIONINFO;
typedef POSVERSIONINFOA POSVERSIONINFO;
typedef LPOSVERSIONINFOA LPOSVERSIONINFO;
#endif /* UNICODE */

$endif (_WDMDDK_)
$if (_NTDDK_)

#ifndef _RTL_RUN_ONCE_DEF
#define _RTL_RUN_ONCE_DEF

#define RTL_RUN_ONCE_INIT {0}

#define RTL_RUN_ONCE_CHECK_ONLY     0x00000001UL
#define RTL_RUN_ONCE_ASYNC          0x00000002UL
#define RTL_RUN_ONCE_INIT_FAILED    0x00000004UL

#define RTL_RUN_ONCE_CTX_RESERVED_BITS 2

#define RTL_HASH_ALLOCATED_HEADER            0x00000001

#define RTL_HASH_RESERVED_SIGNATURE 0

/* RtlVerifyVersionInfo() ComparisonType */

#define VER_EQUAL                       1
#define VER_GREATER                     2
#define VER_GREATER_EQUAL               3
#define VER_LESS                        4
#define VER_LESS_EQUAL                  5
#define VER_AND                         6
#define VER_OR                          7

#define VER_CONDITION_MASK              7
#define VER_NUM_BITS_PER_CONDITION_MASK 3

/* RtlVerifyVersionInfo() TypeMask */

#define VER_MINORVERSION                  0x0000001
#define VER_MAJORVERSION                  0x0000002
#define VER_BUILDNUMBER                   0x0000004
#define VER_PLATFORMID                    0x0000008
#define VER_SERVICEPACKMINOR              0x0000010
#define VER_SERVICEPACKMAJOR              0x0000020
#define VER_SUITENAME                     0x0000040
#define VER_PRODUCT_TYPE                  0x0000080

#define VER_NT_WORKSTATION              0x0000001
#define VER_NT_DOMAIN_CONTROLLER        0x0000002
#define VER_NT_SERVER                   0x0000003

#define VER_PLATFORM_WIN32s             0
#define VER_PLATFORM_WIN32_WINDOWS      1
#define VER_PLATFORM_WIN32_NT           2

typedef union _RTL_RUN_ONCE {
  PVOID Ptr;
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;

_Function_class_(RTL_RUN_ONCE_INIT_FN)
_IRQL_requires_same_
typedef ULONG /* LOGICAL */
(NTAPI *PRTL_RUN_ONCE_INIT_FN) (
  _Inout_ PRTL_RUN_ONCE RunOnce,
  _Inout_opt_ PVOID Parameter,
  _Inout_opt_ PVOID *Context);

#endif /* _RTL_RUN_ONCE_DEF */

typedef enum _TABLE_SEARCH_RESULT {
  TableEmptyTree,
  TableFoundNode,
  TableInsertAsLeft,
  TableInsertAsRight
} TABLE_SEARCH_RESULT;

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
  GenericLessThan,
  GenericGreaterThan,
  GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

// Forwarder
struct _RTL_AVL_TABLE;

_IRQL_requires_same_
_Function_class_(RTL_AVL_COMPARE_ROUTINE)
typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI RTL_AVL_COMPARE_ROUTINE) (
  _In_ struct _RTL_AVL_TABLE *Table,
  _In_ PVOID FirstStruct,
  _In_ PVOID SecondStruct);
typedef RTL_AVL_COMPARE_ROUTINE *PRTL_AVL_COMPARE_ROUTINE;

_IRQL_requires_same_
_Function_class_(RTL_AVL_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
typedef PVOID
(NTAPI RTL_AVL_ALLOCATE_ROUTINE) (
  _In_ struct _RTL_AVL_TABLE *Table,
  _In_ CLONG ByteSize);
typedef RTL_AVL_ALLOCATE_ROUTINE *PRTL_AVL_ALLOCATE_ROUTINE;

_IRQL_requires_same_
_Function_class_(RTL_AVL_FREE_ROUTINE)
typedef VOID
(NTAPI RTL_AVL_FREE_ROUTINE) (
  _In_ struct _RTL_AVL_TABLE *Table,
  _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer);
typedef RTL_AVL_FREE_ROUTINE *PRTL_AVL_FREE_ROUTINE;

_IRQL_requires_same_
_Function_class_(RTL_AVL_MATCH_FUNCTION)
typedef NTSTATUS
(NTAPI RTL_AVL_MATCH_FUNCTION) (
  _In_ struct _RTL_AVL_TABLE *Table,
  _In_ PVOID UserData,
  _In_ PVOID MatchData);
typedef RTL_AVL_MATCH_FUNCTION *PRTL_AVL_MATCH_FUNCTION;

typedef struct _RTL_BALANCED_LINKS {
  struct _RTL_BALANCED_LINKS *Parent;
  struct _RTL_BALANCED_LINKS *LeftChild;
  struct _RTL_BALANCED_LINKS *RightChild;
  CHAR Balance;
  UCHAR Reserved[3];
} RTL_BALANCED_LINKS, *PRTL_BALANCED_LINKS;

typedef struct _RTL_AVL_TABLE {
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

typedef struct _RTL_SPLAY_LINKS {
  struct _RTL_SPLAY_LINKS *Parent;
  struct _RTL_SPLAY_LINKS *LeftChild;
  struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

#ifndef RTL_USE_AVL_TABLES

struct _RTL_GENERIC_TABLE;

_IRQL_requires_same_
_Function_class_(RTL_GENERIC_COMPARE_ROUTINE)
typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI RTL_GENERIC_COMPARE_ROUTINE) (
  _In_ struct _RTL_GENERIC_TABLE *Table,
  _In_ PVOID FirstStruct,
  _In_ PVOID SecondStruct);
typedef RTL_GENERIC_COMPARE_ROUTINE *PRTL_GENERIC_COMPARE_ROUTINE;

_IRQL_requires_same_
_Function_class_(RTL_GENERIC_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
typedef PVOID
(NTAPI RTL_GENERIC_ALLOCATE_ROUTINE) (
  _In_ struct _RTL_GENERIC_TABLE *Table,
  _In_ CLONG ByteSize);
typedef RTL_GENERIC_ALLOCATE_ROUTINE *PRTL_GENERIC_ALLOCATE_ROUTINE;

_IRQL_requires_same_
_Function_class_(RTL_GENERIC_FREE_ROUTINE)
typedef VOID
(NTAPI RTL_GENERIC_FREE_ROUTINE) (
  _In_ struct _RTL_GENERIC_TABLE *Table,
  _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer);
typedef RTL_GENERIC_FREE_ROUTINE *PRTL_GENERIC_FREE_ROUTINE;

typedef struct _RTL_GENERIC_TABLE {
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

#ifdef RTL_USE_AVL_TABLES

#undef  RTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef  RTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef  RTL_GENERIC_FREE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE
#undef  RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define  RTL_GENERIC_COMPARE_ROUTINE     RTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_COMPARE_ROUTINE    PRTL_AVL_COMPARE_ROUTINE
#define  RTL_GENERIC_ALLOCATE_ROUTINE    RTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE   PRTL_AVL_ALLOCATE_ROUTINE
#define  RTL_GENERIC_FREE_ROUTINE        RTL_AVL_FREE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE       PRTL_AVL_FREE_ROUTINE
#define  RTL_GENERIC_TABLE               RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE              PRTL_AVL_TABLE

#endif /* RTL_USE_AVL_TABLES */

#define RTL_HASH_ALLOCATED_HEADER 0x00000001

typedef struct _RTL_DYNAMIC_HASH_TABLE_ENTRY {
  LIST_ENTRY Linkage;
  ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_ENTRY, *PRTL_DYNAMIC_HASH_TABLE_ENTRY;

typedef struct _RTL_DYNAMIC_HASH_TABLE_CONTEXT {
  PLIST_ENTRY ChainHead;
  PLIST_ENTRY PrevLinkage;
  ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_CONTEXT, *PRTL_DYNAMIC_HASH_TABLE_CONTEXT;

typedef struct _RTL_DYNAMIC_HASH_TABLE_ENUMERATOR {
  RTL_DYNAMIC_HASH_TABLE_ENTRY HashEntry;
  PLIST_ENTRY ChainHead;
  ULONG BucketIndex;
} RTL_DYNAMIC_HASH_TABLE_ENUMERATOR, *PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR;

typedef struct _RTL_DYNAMIC_HASH_TABLE {
  ULONG Flags;
  ULONG Shift;
  ULONG TableSize;
  ULONG Pivot;
  ULONG DivisorMask;
  ULONG NumEntries;
  ULONG NonEmptyBuckets;
  ULONG NumEnumerators;
  PVOID Directory;
} RTL_DYNAMIC_HASH_TABLE, *PRTL_DYNAMIC_HASH_TABLE;

#define HASH_ENTRY_KEY(x)    ((x)->Signature)

$endif (_NTDDK_)
$if (_NTIFS_)

#define RTL_SYSTEM_VOLUME_INFORMATION_FOLDER    L"System Volume Information"

_Function_class_(RTL_ALLOCATE_STRING_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_allocatesMem(Mem)
typedef PVOID
(NTAPI *PRTL_ALLOCATE_STRING_ROUTINE)(
  _In_ SIZE_T NumberOfBytes);

#if _WIN32_WINNT >= 0x0600
_Function_class_(RTL_REALLOCATE_STRING_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_allocatesMem(Mem)
typedef PVOID
(NTAPI *PRTL_REALLOCATE_STRING_ROUTINE)(
  _In_ SIZE_T NumberOfBytes,
  IN PVOID Buffer);
#endif

typedef VOID
(NTAPI *PRTL_FREE_STRING_ROUTINE)(
  _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer);

extern NTKERNELAPI const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine;
extern NTKERNELAPI const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine;

#if _WIN32_WINNT >= 0x0600
extern NTKERNELAPI const PRTL_REALLOCATE_STRING_ROUTINE RtlReallocateStringRoutine;
#endif

_Function_class_(RTL_HEAP_COMMIT_ROUTINE)
_IRQL_requires_same_
typedef NTSTATUS
(NTAPI *PRTL_HEAP_COMMIT_ROUTINE) (
  _In_ PVOID Base,
  _Inout_ PVOID *CommitAddress,
  _Inout_ PSIZE_T CommitSize);

typedef struct _RTL_HEAP_PARAMETERS {
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

#if (NTDDI_VERSION >= NTDDI_WIN2K)

typedef struct _GENERATE_NAME_CONTEXT {
  USHORT Checksum;
  BOOLEAN CheckSumInserted;
  _Field_range_(<=, 8) UCHAR NameLength;
  WCHAR NameBuffer[8];
  _Field_range_(<=, 4) ULONG ExtensionLength;
  WCHAR ExtensionBuffer[4];
  ULONG LastIndexValue;
} GENERATE_NAME_CONTEXT, *PGENERATE_NAME_CONTEXT;

typedef struct _PREFIX_TABLE_ENTRY {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  struct _PREFIX_TABLE_ENTRY *NextPrefixTree;
  RTL_SPLAY_LINKS Links;
  PSTRING Prefix;
} PREFIX_TABLE_ENTRY, *PPREFIX_TABLE_ENTRY;

typedef struct _PREFIX_TABLE {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  PPREFIX_TABLE_ENTRY NextPrefixTree;
} PREFIX_TABLE, *PPREFIX_TABLE;

typedef struct _UNICODE_PREFIX_TABLE_ENTRY {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  struct _UNICODE_PREFIX_TABLE_ENTRY *NextPrefixTree;
  struct _UNICODE_PREFIX_TABLE_ENTRY *CaseMatch;
  RTL_SPLAY_LINKS Links;
  PUNICODE_STRING Prefix;
} UNICODE_PREFIX_TABLE_ENTRY, *PUNICODE_PREFIX_TABLE_ENTRY;

typedef struct _UNICODE_PREFIX_TABLE {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  PUNICODE_PREFIX_TABLE_ENTRY NextPrefixTree;
  PUNICODE_PREFIX_TABLE_ENTRY LastNextEntry;
} UNICODE_PREFIX_TABLE, *PUNICODE_PREFIX_TABLE;

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)
typedef struct _COMPRESSED_DATA_INFO {
  USHORT CompressionFormatAndEngine;
  UCHAR CompressionUnitShift;
  UCHAR ChunkShift;
  UCHAR ClusterShift;
  UCHAR Reserved;
  USHORT NumberOfChunks;
  ULONG CompressedChunkSizes[ANYSIZE_ARRAY];
} COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;
#endif
$endif (_NTIFS_)
