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

typedef NTSTATUS
(NTAPI *PRTL_QUERY_REGISTRY_ROUTINE)(
  IN PWSTR ValueName,
  IN ULONG ValueType,
  IN PVOID ValueData,
  IN ULONG ValueLength,
  IN PVOID Context,
  IN PVOID EntryContext);

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
  struct {
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
  struct {
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
  struct {
    SLIST_ENTRY Next;
    USHORT Depth;
    USHORT Sequence;
  } DUMMYSTRUCTNAME;
} SLIST_HEADER, *PSLIST_HEADER;

typedef SLIST_HEADER SLIST_HEADER32, *PSLIST_HEADER32;

#endif /* defined(_WIN64) */

#endif /* _SLIST_HEADER_ */

/* MS definition is broken! */
extern BOOLEAN NTSYSAPI NlsMbCodePageTag;
extern BOOLEAN NTSYSAPI NlsMbOemCodePageTag;
#define NLS_MB_CODE_PAGE_TAG NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG NlsMbOemCodePageTag

#define SHORT_LEAST_SIGNIFICANT_BIT       0
#define SHORT_MOST_SIGNIFICANT_BIT        1

#define LONG_LEAST_SIGNIFICANT_BIT        0
#define LONG_3RD_MOST_SIGNIFICANT_BIT     1
#define LONG_2ND_MOST_SIGNIFICANT_BIT     2
#define LONG_MOST_SIGNIFICANT_BIT         3

#define RTLVERLIB_DDI(x) Wdmlib##x

typedef BOOLEAN
(*PFN_RTL_IS_NTDDI_VERSION_AVAILABLE)(
  IN ULONG Version);

typedef BOOLEAN
(*PFN_RTL_IS_SERVICE_PACK_VERSION_INSTALLED)(
  IN ULONG Version);

$endif

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

typedef ULONG /* LOGICAL */
(NTAPI *PRTL_RUN_ONCE_INIT_FN) (
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN OUT PVOID Parameter OPTIONAL,
  IN OUT PVOID *Context OPTIONAL);

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

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_AVL_COMPARE_ROUTINE) (
  IN struct _RTL_AVL_TABLE *Table,
  IN PVOID FirstStruct,
  IN PVOID SecondStruct);

typedef PVOID
(NTAPI *PRTL_AVL_ALLOCATE_ROUTINE) (
  IN struct _RTL_AVL_TABLE *Table,
  IN CLONG ByteSize);

typedef VOID
(NTAPI *PRTL_AVL_FREE_ROUTINE) (
  IN struct _RTL_AVL_TABLE *Table,
  IN PVOID Buffer);

typedef NTSTATUS
(NTAPI *PRTL_AVL_MATCH_FUNCTION) (
  IN struct _RTL_AVL_TABLE *Table,
  IN PVOID UserData,
  IN PVOID MatchData);

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

#ifndef RTL_USE_AVL_TABLES

struct _RTL_GENERIC_TABLE;

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_GENERIC_COMPARE_ROUTINE) (
  IN struct _RTL_GENERIC_TABLE *Table,
  IN PVOID FirstStruct,
  IN PVOID SecondStruct);

typedef PVOID
(NTAPI *PRTL_GENERIC_ALLOCATE_ROUTINE) (
  IN struct _RTL_GENERIC_TABLE *Table,
  IN CLONG ByteSize);

typedef VOID
(NTAPI *PRTL_GENERIC_FREE_ROUTINE) (
  IN struct _RTL_GENERIC_TABLE *Table,
  IN PVOID Buffer);

typedef struct _RTL_SPLAY_LINKS {
  struct _RTL_SPLAY_LINKS *Parent;
  struct _RTL_SPLAY_LINKS *LeftChild;
  struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

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

#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef RTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef RTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define PRTL_GENERIC_COMPARE_ROUTINE PRTL_AVL_COMPARE_ROUTINE
#define RTL_GENERIC_COMPARE_ROUTINE RTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE PRTL_AVL_ALLOCATE_ROUTINE
#define RTL_GENERIC_ALLOCATE_ROUTINE RTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE PRTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_FREE_ROUTINE RTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_TABLE RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE PRTL_AVL_TABLE

#endif /* RTL_USE_AVL_TABLES */

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

#define HASH_ENTRY_KEY(x)    ((x)->Signature)

$endif
