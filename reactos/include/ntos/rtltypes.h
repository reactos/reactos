/* $Id$
 * 
 */

#ifndef __DDK_RTLTYPES_H
#define __DDK_RTLTYPES_H

#ifndef __USE_W32API

#define COMPRESSION_FORMAT_NONE		0x0000
#define COMPRESSION_FORMAT_DEFAULT	0x0001
#define COMPRESSION_FORMAT_LZNT1	0x0002

#define COMPRESSION_ENGINE_STANDARD	0x0000
#define COMPRESSION_ENGINE_MAXIMUM	0x0100
#define COMPRESSION_ENGINE_HIBER	0x0200
/*
#define VER_EQUAL						1
#define VER_GREATER						2
#define VER_GREATER_EQUAL				3
#define VER_LESS						4
#define VER_LESS_EQUAL					5
#define VER_AND							6
#define VER_OR							7

#define VER_CONDITION_MASK				7
#define VER_NUM_BITS_PER_CONDITION_MASK	3

#define VER_MINORVERSION				0x0000001
#define VER_MAJORVERSION				0x0000002
#define VER_BUILDNUMBER					0x0000004
#define VER_PLATFORMID					0x0000008
#define VER_SERVICEPACKMINOR			0x0000010
#define VER_SERVICEPACKMAJOR			0x0000020
#define VER_SUITENAME					0x0000040
#define VER_PRODUCT_TYPE				0x0000080

#define VER_NT_WORKSTATION				0x0000001
#define VER_NT_DOMAIN_CONTROLLER		0x0000002
#define VER_NT_SERVER					0x0000003
*/

typedef struct _CONTROLLER_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  PVOID ControllerExtension;
  KDEVICE_QUEUE DeviceWaitQueue;
  ULONG Spare1;
  LARGE_INTEGER Spare2;
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

typedef struct _STRING
{
  /*
   * Length in bytes of the string stored in buffer
   */
  USHORT Length;

  /*
   * Maximum length of the string 
   */
  USHORT MaximumLength;

  /*
   * String
   */
  PCHAR Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;

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

typedef struct _RTL_BITMAP
{
  ULONG SizeOfBitMap;
  PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _RTL_BITMAP_RUN
{
  ULONG StarttingIndex;
  ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

#ifndef STDCALL_FUNC
#define STDCALL_FUNC(a)  (STDCALL a)
#endif

typedef NTSTATUS STDCALL_FUNC
(*PRTL_QUERY_REGISTRY_ROUTINE) (PWSTR ValueName,
			       ULONG ValueType,
			       PVOID ValueData,
			       ULONG ValueLength,
			       PVOID Context,
			       PVOID EntryContext);

typedef struct _RTL_QUERY_REGISTRY_TABLE
{
  PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
  ULONG Flags;
  PWSTR Name;
  PVOID EntryContext;
  ULONG DefaultType;
  PVOID DefaultData;
  ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

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

typedef struct _RTL_SPLAY_LINKS
{
  struct _RTL_SPLAY_LINKS *Parent;
  struct _RTL_SPLAY_LINKS *LeftChild;
  struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;


typedef struct _RTL_RANGE_LIST
{
  LIST_ENTRY ListHead;
  ULONG Flags;  /* RTL_RANGE_LIST_... flags */
  ULONG Count;
  ULONG Stamp;
} RTL_RANGE_LIST, *PRTL_RANGE_LIST;

#define RTL_RANGE_LIST_ADD_IF_CONFLICT  0x00000001
#define RTL_RANGE_LIST_ADD_SHARED       0x00000002

typedef struct _RTL_RANGE
{
  ULONGLONG Start;
  ULONGLONG End;
  PVOID UserData;
  PVOID Owner;
  UCHAR Attributes;
  UCHAR Flags;  /* RTL_RANGE_... flags */
} RTL_RANGE, *PRTL_RANGE;

#define RTL_RANGE_SHARED      0x01
#define RTL_RANGE_CONFLICT    0x02

typedef BOOLEAN
(STDCALL *PRTL_CONFLICT_RANGE_CALLBACK) (PVOID Context,
					 PRTL_RANGE Range);


typedef struct _RANGE_LIST_ITERATOR
{
  PLIST_ENTRY RangeListHead;
  PLIST_ENTRY MergedHead;
  PVOID Current;
  ULONG Stamp;
} RTL_RANGE_LIST_ITERATOR, *PRTL_RANGE_LIST_ITERATOR;


typedef struct _INITIAL_TEB
{
  PVOID StackBase;
  PVOID StackLimit;
  PVOID StackCommit;
  PVOID StackCommitMax;
  PVOID StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

#define MAXIMUM_LEADBYTES 12

typedef struct _CPTABLEINFO
{
  USHORT  CodePage;
  USHORT  MaximumCharacterSize;  /* SBCS = 1, DBCS = 2 */
  USHORT  DefaultChar;
  USHORT  UniDefaultChar;
  USHORT  TransDefaultChar;
  USHORT  TransUniDefaultChar;
  USHORT  DBCSCodePage;
  UCHAR   LeadByte[MAXIMUM_LEADBYTES];
  PUSHORT MultiByteTable;
  PVOID   WideCharTable;
  PUSHORT DBCSRanges;
  PUSHORT DBCSOffsets;
} CPTABLEINFO, *PCPTABLEINFO;

typedef struct _NLSTABLEINFO
{
  CPTABLEINFO OemTableInfo;
  CPTABLEINFO AnsiTableInfo;
  PUSHORT UpperCaseTable;
  PUSHORT LowerCaseTable;
} NLSTABLEINFO, *PNLSTABLEINFO;


#else /* __USE_W32API */

#include <ddk/ntifs.h>

#endif /* __USE_W32API */

typedef struct _RTL_HEAP_DEFINITION
{
  ULONG Length;
  ULONG Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

typedef struct _RTL_ATOM_TABLE
{
  ULONG TableSize;
  ULONG NumberOfAtoms;
  PVOID Lock;		/* fast mutex (kernel mode)/ critical section (user mode) */
  PVOID HandleTable;
  LIST_ENTRY Slot[0];
} RTL_ATOM_TABLE, *PRTL_ATOM_TABLE;




#include <pshpack1.h>

typedef struct _NLS_FILE_HEADER
{
  USHORT  HeaderSize;
  USHORT  CodePage;
  USHORT  MaximumCharacterSize;  /* SBCS = 1, DBCS = 2 */
  USHORT  DefaultChar;
  USHORT  UniDefaultChar;
  USHORT  TransDefaultChar;
  USHORT  TransUniDefaultChar;
  USHORT  DBCSCodePage;
  UCHAR   LeadByte[MAXIMUM_LEADBYTES];
} NLS_FILE_HEADER, *PNLS_FILE_HEADER;

#include <poppack.h>
/*
typedef struct _OSVERSIONINFOEXA {
	ULONG dwOSVersionInfoSize;
	ULONG dwMajorVersion;
	ULONG dwMinorVersion;
	ULONG dwBuildNumber;
	ULONG dwPlatformId;
	CHAR szCSDVersion [128];
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

*/
typedef struct _RTL_MESSAGE_RESOURCE_ENTRY
{
  USHORT Length;
  USHORT Flags;
  CHAR Text[1];
} RTL_MESSAGE_RESOURCE_ENTRY, *PRTL_MESSAGE_RESOURCE_ENTRY;

typedef struct _RTL_MESSAGE_RESOURCE_BLOCK
{
  ULONG LowId;
  ULONG HighId;
  ULONG OffsetToEntries;
} RTL_MESSAGE_RESOURCE_BLOCK, *PRTL_MESSAGE_RESOURCE_BLOCK;

typedef struct _RTL_MESSAGE_RESOURCE_DATA
{
  ULONG NumberOfBlocks;
  RTL_MESSAGE_RESOURCE_BLOCK Blocks[1];
} RTL_MESSAGE_RESOURCE_DATA, *PRTL_MESSAGE_RESOURCE_DATA;

typedef VOID
(STDCALL *PRTL_BASE_PROCESS_START_ROUTINE)(PTHREAD_START_ROUTINE StartAddress,
  PVOID Parameter);


typedef struct _UNICODE_PREFIX_TABLE_ENTRY {
	USHORT NodeTypeCode;
	USHORT NameLength;
	struct _UNICODE_PREFIX_TABLE_ENTRY *NextPrefixTree;
	struct _UNICODE_PREFIX_TABLE_ENTRY *CaseMatch;
	RTL_SPLAY_LINKS Links;
	PUNICODE_STRING Prefix;
} UNICODE_PREFIX_TABLE_ENTRY;
typedef UNICODE_PREFIX_TABLE_ENTRY *PUNICODE_PREFIX_TABLE_ENTRY;

typedef struct _UNICODE_PREFIX_TABLE {
	USHORT NodeTypeCode;
	USHORT NameLength;
	PUNICODE_PREFIX_TABLE_ENTRY NextPrefixTree;
	PUNICODE_PREFIX_TABLE_ENTRY LastNextEntry;
} UNICODE_PREFIX_TABLE;
typedef UNICODE_PREFIX_TABLE *PUNICODE_PREFIX_TABLE;

typedef enum _TABLE_SEARCH_RESULT{
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

struct _RTL_AVL_TABLE;

typedef
RTL_GENERIC_COMPARE_RESULTS
(STDCALL *PRTL_AVL_COMPARE_ROUTINE) (
	struct _RTL_AVL_TABLE *Table,
	PVOID FirstStruct,
	PVOID SecondStruct
	);

typedef
PVOID
(STDCALL *PRTL_AVL_ALLOCATE_ROUTINE) (
	struct _RTL_AVL_TABLE *Table,
	ULONG ByteSize
	);

typedef
VOID
(STDCALL *PRTL_AVL_FREE_ROUTINE) (
	struct _RTL_AVL_TABLE *Table,
	PVOID Buffer
	);

typedef
NTSTATUS
(STDCALL *PRTL_AVL_MATCH_FUNCTION) (
	struct _RTL_AVL_TABLE *Table,
	PVOID UserData,
	PVOID MatchData
	);

typedef struct _RTL_BALANCED_LINKS {
	struct _RTL_BALANCED_LINKS *Parent;
	struct _RTL_BALANCED_LINKS *LeftChild;
	struct _RTL_BALANCED_LINKS *RightChild;
	CHAR Balance;
	UCHAR Reserved[3];
} RTL_BALANCED_LINKS;

typedef RTL_BALANCED_LINKS *PRTL_BALANCED_LINKS;

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
} RTL_AVL_TABLE;
typedef RTL_AVL_TABLE *PRTL_AVL_TABLE;

struct _RTL_GENERIC_TABLE;

typedef
RTL_GENERIC_COMPARE_RESULTS
(STDCALL *PRTL_GENERIC_COMPARE_ROUTINE) (
	struct _RTL_GENERIC_TABLE *Table,
	PVOID FirstStruct,
	PVOID SecondStruct
	);

typedef
PVOID
(STDCALL *PRTL_GENERIC_ALLOCATE_ROUTINE) (
	struct _RTL_GENERIC_TABLE *Table,
	ULONG ByteSize
	);

typedef
VOID
(STDCALL *PRTL_GENERIC_FREE_ROUTINE) (
	struct _RTL_GENERIC_TABLE *Table,
	PVOID Buffer
	);


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
} RTL_GENERIC_TABLE;
typedef RTL_GENERIC_TABLE *PRTL_GENERIC_TABLE;

typedef NTSTATUS
(*PHEAP_ENUMERATION_ROUTINE)(IN PVOID HeapHandle,
                             IN PVOID UserParam);

#endif /* __DDK_RTLTYPES_H */
