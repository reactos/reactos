/* $Id: rtltypes.h,v 1.13 2004/02/10 16:22:56 navaraf Exp $
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

typedef struct _COMPRESSED_DATA_INFO
{
  USHORT CompressionFormatAndEngine;
  UCHAR CompressionUnitShift;
  UCHAR ChunkShift;
  UCHAR ClusterShift;
  UCHAR Reserved;
  USHORT NumberOfChunks;
  ULONG CompressedChunkSizes[1];
} COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;

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


typedef struct _USER_STACK
{
  PVOID FixedStackBase;
  PVOID FixedStackLimit;
  PVOID ExpandableStackBase;
  PVOID ExpandableStackLimit;
  PVOID ExpandableStackBottom;
} USER_STACK, *PUSER_STACK;

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


typedef struct _RTL_GENERIC_TABLE
{
  PVOID RootElement;
  ULONG Unknown2;
  ULONG Unknown3;
  ULONG Unknown4;
  ULONG Unknown5;
  ULONG ElementCount;
  PVOID CompareRoutine;
  PVOID AllocateRoutine;
  PVOID FreeRoutine;
  ULONG UserParameter;
} RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;


typedef struct _RTL_MESSAGE_RESOURCE_ENTRY
{
  USHORT Length;
  USHORT Flags;
  UCHAR Text[1];
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

#endif /* __DDK_RTLTYPES_H */
