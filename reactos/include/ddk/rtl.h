/* $Id: rtl.h,v 1.55 2001/07/04 20:40:18 chorns Exp $
 * 
 */

#ifndef __DDK_RTL_H
#define __DDK_RTL_H

#if defined(__NTOSKRNL__) || defined(__NTDRIVER__) || defined(__NTHAL__) || defined(__NTDLL__) || defined (__NTAPP__)

#include <stddef.h>
#include <stdarg.h>

#endif /* __NTOSKRNL__ || __NTDRIVER__ || __NTHAL__ || __NTDLL__ || __NTAPP__ */

#include <pe.h>

typedef struct _INITIAL_TEB
{
	PVOID StackBase;
	PVOID StackLimit;
	PVOID StackCommit;
	PVOID StackCommitMax;
	PVOID StackReserve;
} INITIAL_TEB, *PINITIAL_TEB;

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
   ULONG  SizeOfBitMap;
   PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct {
	ULONG		Length;
	ULONG		Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

typedef struct _RTL_ATOM_TABLE
{
	ULONG TableSize;
	ULONG NumberOfAtoms;
	PVOID Lock;		/* fast mutex (kernel mode)/ critical section (user mode) */
	PVOID HandleTable;
	LIST_ENTRY Slot[0];
} RTL_ATOM_TABLE, *PRTL_ATOM_TABLE;

struct _LB_RANGE
{
	ULONG dummy;
};

typedef struct _RTL_NLS_DATA
{
	USHORT			CodePage;
	USHORT			MaxCharacterSize;  // SBCS = 1, DBCS = 2
	WCHAR			DefaultCharacter;
	WCHAR			char1;
	WCHAR			char2;
	WCHAR			char3;
	USHORT			DbcsFlag;
	struct _LB_RANGE	LeadByteRange[6];
	USHORT			reserved;
	PWCHAR			MultiByteToUnicode;
	PCHAR			UnicodeToMultiByte;
	ULONG			unknown;
	PCHAR			DbcsTags;
} RTL_NLS_DATA, *PRTL_NLS_DATA;


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


/*
 * PURPOSE: Flags for RtlQueryRegistryValues
 */
#define RTL_QUERY_REGISTRY_SUBKEY	(0x00000001)
#define RTL_QUERY_REGISTRY_TOPKEY	(0x00000002)
#define RTL_QUERY_REGISTRY_REQUIRED	(0x00000004)
#define RTL_QUERY_REGISTRY_NOVALUE	(0x00000008)
#define RTL_QUERY_REGISTRY_NOEXPAND	(0x00000010)
#define RTL_QUERY_REGISTRY_DIRECT	(0x00000020)
#define RTL_QUERY_REGISTRY_DELETE	(0x00000040)


typedef NTSTATUS (*PRTL_QUERY_REGISTRY_ROUTINE)(PWSTR ValueName,
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

/*
 * PURPOSE: Used with RtlCheckRegistryKey, RtlCreateRegistryKey, 
 * RtlDeleteRegistryKey
 */
#define RTL_REGISTRY_ABSOLUTE   0
#define RTL_REGISTRY_SERVICES   1
#define RTL_REGISTRY_CONTROL    2
#define RTL_REGISTRY_WINDOWS_NT 3
#define RTL_REGISTRY_DEVICEMAP  4
#define RTL_REGISTRY_USER       5
#define RTL_REGISTRY_MAXIMUM    6

#define RTL_REGISTRY_HANDLE     0x40000000
#define RTL_REGISTRY_OPTIONAL   0x80000000


#define SHORT_SIZE	(sizeof(USHORT))
#define SHORT_MASK	(SHORT_SIZE-1)
#define LONG_SIZE	(sizeof(ULONG))
#define LONG_MASK	(LONG_SIZE-1)
#define LOWBYTE_MASK	0x00FF

#define FIRSTBYTE(Value)	((Value) & LOWBYTE_MASK)
#define SECONDBYTE(Value)	(((Value) >> 8) & LOWBYTE_MASK)
#define THIRDBYTE(Value)	(((Value) >> 16) & LOWBYTE_MASK)
#define FOURTHBYTE(Value)	(((Value) >> 24) & LOWBYTE_MASK)

/* FIXME: reverse byte-order on big-endian machines (e.g. MIPS) */
#define SHORT_LEAST_SIGNIFICANT_BIT	0
#define SHORT_MOST_SIGNIFICANT_BIT	1

#define LONG_LEAST_SIGNIFICANT_BIT	0
#define LONG_3RD_MOST_SIGNIFICANT_BIT	1
#define LONG_2RD_MOST_SIGNIFICANT_BIT	2
#define LONG_MOST_SIGNIFICANT_BIT	3



#if defined(__NTOSKRNL__) || defined(__NTDLL__)
#define NLS_MB_CODE_PAGE_TAG     NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG NlsMbOemCodePageTag
#else
#define NLS_MB_CODE_PAGE_TAG     (*NlsMbCodePageTag)
#define NLS_MB_OEM_CODE_PAGE_TAG (*NlsMbOemCodePageTag)
#endif /* __NTOSKRNL__ || __NTDLL__ */

extern BOOLEAN NLS_MB_CODE_PAGE_TAG;
extern BOOLEAN NLS_MB_OEM_CODE_PAGE_TAG;


/*
 * VOID
 * InitializeObjectAttributes (
 *	POBJECT_ATTRIBUTES	InitializedAttributes,
 *	PUNICODE_STRING		ObjectName,
 *	ULONG			Attributes,
 *	HANDLE			RootDirectory,
 *	PSECURITY_DESCRIPTOR	SecurityDescriptor
 *	);
 *
 * FUNCTION: Sets up a parameter of type OBJECT_ATTRIBUTES for a 
 * subsequent call to ZwCreateXXX or ZwOpenXXX
 * ARGUMENTS:
 *        InitializedAttributes (OUT) = Caller supplied storage for the
 *                                      object attributes
 *        ObjectName = Full path name for object
 *        Attributes = Attributes for the object
 *        RootDirectory = Where the object should be placed or NULL
 *        SecurityDescriptor = Ignored
 */
#define InitializeObjectAttributes(p,n,a,r,s) \
{ \
	(p)->Length = sizeof(OBJECT_ATTRIBUTES); \
	(p)->ObjectName = n; \
	(p)->Attributes = a; \
	(p)->RootDirectory = r; \
	(p)->SecurityDescriptor = s; \
	(p)->SecurityQualityOfService = NULL; \
}


/*
 * VOID
 * InitializeListHead (
 *		PLIST_ENTRY	ListHead
 *		);
 *
 * FUNCTION: Initializes a double linked list
 * ARGUMENTS:
 *         ListHead = Caller supplied storage for the head of the list
 */
#define InitializeListHead(ListHead) \
{ \
	(ListHead)->Flink = (ListHead); \
	(ListHead)->Blink = (ListHead); \
}


/*
 * VOID
 * InsertHeadList (
 *		PLIST_ENTRY	ListHead,
 *		PLIST_ENTRY	Entry
 *		);
 *
 * FUNCTION: Inserts an entry in a double linked list
 * ARGUMENTS:
 *        ListHead = Head of the list
 *        Entry = Entry to insert
 */
#define InsertHeadList(ListHead, ListEntry) \
{ \
	PLIST_ENTRY OldFlink; \
	OldFlink = (ListHead)->Flink; \
	(ListEntry)->Flink = OldFlink; \
	(ListEntry)->Blink = (ListHead); \
	OldFlink->Blink = (ListEntry); \
	(ListHead)->Flink = (ListEntry); \
	assert((ListEntry) != NULL); \
	assert((ListEntry)->Blink!=NULL); \
	assert((ListEntry)->Blink->Flink == (ListEntry)); \
	assert((ListEntry)->Flink != NULL); \
	assert((ListEntry)->Flink->Blink == (ListEntry)); \
}


/*
 * VOID
 * InsertTailList (
 *		PLIST_ENTRY	ListHead,
 *		PLIST_ENTRY	Entry
 *		);
 *
 * FUNCTION:
 *	Inserts an entry in a double linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *	Entry = Entry to insert
 */
#define InsertTailList(ListHead, ListEntry) \
{ \
	PLIST_ENTRY OldBlink; \
	OldBlink = (ListHead)->Blink; \
	(ListEntry)->Flink = (ListHead); \
	(ListEntry)->Blink = OldBlink; \
	OldBlink->Flink = (ListEntry); \
	(ListHead)->Blink = (ListEntry); \
	assert((ListEntry) != NULL); \
	assert((ListEntry)->Blink != NULL); \
	assert((ListEntry)->Blink->Flink == (ListEntry)); \
	assert((ListEntry)->Flink != NULL); \
	assert((ListEntry)->Flink->Blink == (ListEntry)); \
}

/*
 * BOOLEAN
 * IsListEmpty (
 *	PLIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Checks if a double linked list is empty
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
*/
#define IsListEmpty(ListHead) \
	((ListHead)->Flink == (ListHead))


/*
 * PSINGLE_LIST_ENTRY
 * PopEntryList (
 *	PSINGLE_LIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Removes an entry from the head of a single linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *
 * RETURNS:
 *	The removed entry
 */
/*
#define PopEntryList(ListHead) \
	(ListHead)->Next; \
	{ \
		PSINGLE_LIST_ENTRY FirstEntry; \
		FirstEntry = (ListHead)->Next; \
		if (FirstEntry != NULL) \
		{ \
			(ListHead)->Next = FirstEntry->Next; \
		} \
	}
*/
static
inline
PSINGLE_LIST_ENTRY
 PopEntryList(
	PSINGLE_LIST_ENTRY	ListHead
	)
{
	PSINGLE_LIST_ENTRY ListEntry;

	ListEntry = ListHead->Next;
	if (ListEntry!=NULL)
	{
		ListHead->Next = ListEntry->Next;
	}
	return ListEntry;
}

/*
VOID
PushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	Entry
	);
*/
/*
#define PushEntryList(ListHead,Entry) \
	(Entry)->Next = (ListHead)->Next; \
	(ListHead)->Next = (Entry)
*/
static
inline
VOID
PushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	Entry
	)
{
	Entry->Next = ListHead->Next;
	ListHead->Next = Entry;
}


/*
 * An ReactOS extension
 */
static
inline
PSINGLE_LIST_ENTRY
 PopEntrySList(
	PSLIST_HEADER	ListHead
	)
{
	PSINGLE_LIST_ENTRY ListEntry;

	ListEntry = ListHead->s.Next.Next;
	if (ListEntry!=NULL)
	{
		ListHead->s.Next.Next = ListEntry->Next;
    ListHead->s.Depth++;
    ListHead->s.Sequence++;
  }
	return ListEntry;
}


/*
 * An ReactOS extension
 */
static
inline
VOID
PushEntrySList (
	PSLIST_HEADER	ListHead,
	PSINGLE_LIST_ENTRY	Entry
	)
{
	Entry->Next = ListHead->s.Next.Next;
	ListHead->s.Next.Next = Entry;
  ListHead->s.Depth++;
  ListHead->s.Sequence++;
}


/*
 *VOID
 *RemoveEntryList (
 *	PLIST_ENTRY	Entry
 *	);
 *
 * FUNCTION:
 *	Removes an entry from a double linked list
 *
 * ARGUMENTS:
 *	ListEntry = Entry to remove
 */
#define RemoveEntryList(ListEntry) \
{ \
	PLIST_ENTRY OldFlink; \
	PLIST_ENTRY OldBlink; \
	assert((ListEntry) != NULL); \
	assert((ListEntry)->Blink!=NULL); \
	assert((ListEntry)->Blink->Flink == (ListEntry)); \
	assert((ListEntry)->Flink != NULL); \
	assert((ListEntry)->Flink->Blink == (ListEntry)); \
	OldFlink = (ListEntry)->Flink; \
	OldBlink = (ListEntry)->Blink; \
	OldFlink->Blink = OldBlink; \
	OldBlink->Flink = OldFlink; \
        (ListEntry)->Flink = NULL; \
        (ListEntry)->Blink = NULL; \
}


/*
 * PLIST_ENTRY
 * RemoveHeadList (
 *	PLIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Removes the head entry from a double linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *
 * RETURNS:
 *	The removed entry
 */
/*
#define RemoveHeadList(ListHead) \
	(ListHead)->Flink; \
	{RemoveEntryList((ListHead)->Flink)}
*/
/*
PLIST_ENTRY
RemoveHeadList (
	PLIST_ENTRY	ListHead
	);
*/

static
inline
PLIST_ENTRY
RemoveHeadList (
	PLIST_ENTRY	ListHead
	)
{
	PLIST_ENTRY Old;
	PLIST_ENTRY OldFlink;
	PLIST_ENTRY OldBlink;

	Old = ListHead->Flink;

	OldFlink = ListHead->Flink->Flink;
	OldBlink = ListHead->Flink->Blink;
	OldFlink->Blink = OldBlink;
	OldBlink->Flink = OldFlink;
        if (Old != ListHead)
     {
        Old->Flink = NULL;
        Old->Blink = NULL;
     }
   
	return(Old);
}


/*
 * PLIST_ENTRY
 * RemoveTailList (
 *	PLIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Removes the tail entry from a double linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *
 * RETURNS:
 *	The removed entry
 */
/*
#define RemoveTailList(ListHead) \
	(ListHead)->Blink; \
	{RemoveEntryList((ListHead)->Blink)}
*/
/*
PLIST_ENTRY
RemoveTailList (
	PLIST_ENTRY	ListHead
	);
*/

static
inline
PLIST_ENTRY
RemoveTailList (
	PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY Old;
	PLIST_ENTRY OldFlink;
	PLIST_ENTRY OldBlink;

	Old = ListHead->Blink;

	OldFlink = ListHead->Blink->Flink;
	OldBlink = ListHead->Blink->Blink;
	OldFlink->Blink = OldBlink;
	OldBlink->Flink = OldFlink;
   if (Old != ListHead)
     {
        Old->Flink = NULL;
        Old->Blink = NULL;
     }
   
	return(Old);
}


NTSTATUS
STDCALL
RtlAddAtomToAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	PWSTR		AtomName,
	OUT	PRTL_ATOM	Atom
	);

PVOID STDCALL
RtlAllocateHeap (
	HANDLE	Heap,
	ULONG	Flags,
	ULONG	Size
	);

WCHAR
STDCALL
RtlAnsiCharToUnicodeChar (
	CHAR	AnsiChar
	);

ULONG
STDCALL
RtlAnsiStringToUnicodeSize (
	PANSI_STRING	AnsiString
	);

NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString (
	PUNICODE_STRING	DestinationString,
	PANSI_STRING	SourceString,
	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlAppendAsciizToString(
	PSTRING	Destination,
	PCSZ	Source
	);

NTSTATUS
STDCALL
RtlAppendStringToString (
	PSTRING	Destination,
	PSTRING	Source
	);

NTSTATUS
STDCALL
RtlAppendUnicodeStringToString (
	PUNICODE_STRING	Destination,
	PUNICODE_STRING	Source
	);

NTSTATUS
STDCALL
RtlAppendUnicodeToString (
	PUNICODE_STRING	Destination,
	PWSTR		Source
	);

BOOLEAN
STDCALL
RtlAreBitsClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	);

BOOLEAN
STDCALL
RtlAreBitsSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	);

VOID
STDCALL
RtlAssert (
	PVOID FailedAssertion,
	PVOID FileName,
	ULONG LineNumber,
	PCHAR Message
	);

NTSTATUS
STDCALL
RtlCharToInteger (
	PCSZ	String,
	ULONG	Base,
	PULONG	Value
	);

NTSTATUS
STDCALL
RtlCheckRegistryKey (
	ULONG	RelativeTo,
	PWSTR	Path
	);

VOID
STDCALL
RtlClearAllBits (
	IN	PRTL_BITMAP	BitMapHeader
	);

VOID
STDCALL
RtlClearBits (
	IN	PRTL_BITMAP	BitMapHeader,
	IN	ULONG		StartingIndex,
	IN	ULONG		NumberToClear
	);

DWORD
STDCALL
RtlCompactHeap (
	HANDLE	hheap,
	DWORD	flags
	);

ULONG
STDCALL
RtlCompareMemory (
	PVOID	Source1,
	PVOID	Source2,
	ULONG	Length
	);

LONG
STDCALL
RtlCompareString (
	PSTRING	String1,
	PSTRING	String2,
	BOOLEAN	CaseInsensitive
	);

LONG
STDCALL
RtlCompareUnicodeString (
	PUNICODE_STRING	String1,
	PUNICODE_STRING	String2,
	BOOLEAN		BaseInsensitive
	);

LARGE_INTEGER
STDCALL
RtlConvertLongToLargeInteger (
	IN	LONG	SignedInteger
	);

NTSTATUS
STDCALL
RtlConvertSidToUnicodeString (
	IN OUT	PUNICODE_STRING	String,
	IN	PSID		Sid,
	IN	BOOLEAN		AllocateString
	);

LARGE_INTEGER
STDCALL
RtlConvertUlongToLargeInteger (
	IN	ULONG	UnsignedInteger
	);

#if 0
VOID
RtlCopyBytes (
	PVOID		Destination,
	CONST VOID	* Source,
	ULONG		Length
	);

VOID
RtlCopyMemory (
	VOID		* Destination,
	CONST VOID	* Source,
	ULONG		Length
	);
#endif

#define RtlCopyMemory(Destination,Source,Length) \
	memcpy((Destination),(Source),(Length))

#define RtlCopyBytes RtlCopyMemory

VOID
STDCALL
RtlCopyString (
	PSTRING	DestinationString,
	PSTRING	SourceString
	);

VOID
STDCALL
RtlCopyUnicodeString (
	PUNICODE_STRING	DestinationString,
	PUNICODE_STRING	SourceString
	);

NTSTATUS
STDCALL
RtlCreateAtomTable (
	IN	ULONG TableSize,
	IN OUT	PRTL_ATOM_TABLE *AtomTable
	);

HANDLE
STDCALL
RtlCreateHeap (
	ULONG			Flags,
	PVOID			BaseAddress,
	ULONG			SizeToReserve,
	ULONG			SizeToCommit,
	PVOID			Unknown,
	PRTL_HEAP_DEFINITION	Definition
	);

NTSTATUS
STDCALL
RtlCreateRegistryKey (
	ULONG	RelativeTo,
	PWSTR	Path
	);

NTSTATUS
STDCALL
RtlCreateSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	ULONG			Revision
	);

BOOLEAN
STDCALL
RtlCreateUnicodeString (
	OUT	PUNICODE_STRING	Destination,
	IN	PWSTR		Source
	);

BOOLEAN
STDCALL
RtlCreateUnicodeStringFromAsciiz (
	OUT	PUNICODE_STRING	Destination,
	IN	PCSZ		Source
	);

NTSTATUS
STDCALL
RtlCustomCPToUnicodeN (
	PRTL_NLS_DATA	NlsData,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize,
	PULONG		ResultSize,
	PCHAR		CustomString,
	ULONG		CustomSize
	);

NTSTATUS
STDCALL
RtlDeleteAtomFromAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	RTL_ATOM	Atom
	);

NTSTATUS
STDCALL
RtlDeleteRegistryValue (
	ULONG	RelativeTo,
	PWSTR	Path,
	PWSTR	ValueName
	);

NTSTATUS
STDCALL
RtlDestroyAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable
	);

BOOL
STDCALL
RtlDestroyHeap (
	HANDLE	hheap
	);

NTSTATUS
STDCALL
RtlDowncaseUnicodeString (
	IN OUT PUNICODE_STRING	DestinationString,
	IN PUNICODE_STRING	SourceString,
	IN BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlEmptyAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	BOOLEAN		DeletePinned
	);

LARGE_INTEGER
STDCALL
RtlEnlargedIntegerMultiply (
	LONG	Multiplicand,
	LONG	Multiplier
	);

ULONG
STDCALL
RtlEnlargedUnsignedDivide (
	ULARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	);

LARGE_INTEGER
STDCALL
RtlEnlargedUnsignedMultiply (
	ULONG	Multiplicand,
	ULONG	Multiplier
	);

BOOLEAN
STDCALL
RtlEqualString (
	PSTRING	String1,
	PSTRING	String2,
	BOOLEAN	CaseInSensitive
	);

BOOLEAN
STDCALL
RtlEqualUnicodeString (
	PUNICODE_STRING	String1,
	PUNICODE_STRING	String2,
	BOOLEAN		CaseInSensitive
	);

LARGE_INTEGER
STDCALL
RtlExtendedIntegerMultiply (
	LARGE_INTEGER	Multiplicand,
	LONG		Multiplier
	);

LARGE_INTEGER
STDCALL
RtlExtendedLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	);

LARGE_INTEGER
STDCALL
RtlExtendedMagicDivide (
	LARGE_INTEGER	Dividend,
	LARGE_INTEGER	MagicDivisor,
	CCHAR		ShiftCount
	);

VOID
STDCALL
RtlFillMemory (
	PVOID	Destination,
	ULONG	Length,
	UCHAR	Fill
	);

VOID
STDCALL
RtlFillMemoryUlong (
	PVOID	Destination,
	ULONG	Length,
	ULONG	Fill
	);

ULONG
STDCALL
RtlFindClearBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

ULONG
STDCALL
RtlFindClearBitsAndSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

ULONG
STDCALL
RtlFindFirstRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

ULONG
STDCALL
RtlFindFirstRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

ULONG
STDCALL
RtlFindLongestRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

ULONG
STDCALL
RtlFindLongestRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

NTSTATUS
STDCALL
RtlFindMessage (
	IN	PVOID				BaseAddress,
	IN	ULONG				Type,
	IN	ULONG				Language,
	IN	ULONG				MessageId,
	OUT	PRTL_MESSAGE_RESOURCE_ENTRY	*MessageResourceEntry
	);

ULONG
STDCALL
RtlFindSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

ULONG
STDCALL
RtlFindSetBitsAndClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

NTSTATUS
STDCALL
RtlFormatCurrentUserKeyPath (
	IN OUT	PUNICODE_STRING	KeyPath
	);

VOID
STDCALL
RtlFreeAnsiString (
	PANSI_STRING	AnsiString
	);

BOOLEAN
STDCALL
RtlFreeHeap (
	HANDLE	Heap,
	ULONG	Flags,
	PVOID	Address
	);

VOID
STDCALL
RtlFreeOemString (
	POEM_STRING	OemString
	);

VOID
STDCALL
RtlFreeUnicodeString (
	PUNICODE_STRING	UnicodeString
	);

VOID
RtlGetCallersAddress (
	PVOID	* CallersAddress
	);

VOID
STDCALL
RtlGetDefaultCodePage (
	PUSHORT AnsiCodePage,
	PUSHORT OemCodePage
	);

HANDLE
STDCALL
RtlGetProcessHeap (
	VOID
	);

PVOID
STDCALL
RtlImageDirectoryEntryToData (
	PVOID	BaseAddress,
	BOOLEAN	bFlag,
	ULONG	Directory,
	PULONG	Size
	);

PIMAGE_NT_HEADERS
STDCALL
RtlImageNtHeader (
	PVOID	BaseAddress
	);

PIMAGE_SECTION_HEADER
STDCALL
RtlImageRvaToSection (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva
	);

ULONG
STDCALL
RtlImageRvaToVa (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva,
	PIMAGE_SECTION_HEADER	*SectionHeader
	);

VOID
STDCALL
RtlInitAnsiString (
	PANSI_STRING	DestinationString,
	PCSZ		SourceString
	);

VOID
STDCALL
RtlInitString (
	PSTRING	DestinationString,
	PCSZ	SourceString
	);

VOID
STDCALL
RtlInitUnicodeString (
	PUNICODE_STRING	DestinationString,
	PCWSTR		SourceString
	);

VOID
STDCALL
RtlInitializeBitMap (
	IN OUT	PRTL_BITMAP	BitMapHeader,
	IN	PULONG		BitMapBuffer,
	IN	ULONG		SizeOfBitMap
	);

NTSTATUS
STDCALL
RtlInitializeContext (
	IN	HANDLE			ProcessHandle,
	IN	PCONTEXT		Context,
	IN	PVOID			Parameter,
	IN	PTHREAD_START_ROUTINE	StartAddress,
	IN OUT	PINITIAL_TEB		InitialTeb
	);

VOID
STDCALL
RtlInitializeGenericTable (
	IN OUT	PRTL_GENERIC_TABLE	Table,
	IN	PVOID			CompareRoutine,
	IN	PVOID			AllocateRoutine,
	IN	PVOID			FreeRoutine,
	IN	ULONG			UserParameter
	);

PVOID
STDCALL
RtlInsertElementGenericTable (
	IN OUT	PRTL_GENERIC_TABLE	Table,
	IN	PVOID			Element,
	IN	ULONG			ElementSize,
	IN	ULONG			Unknown4
	);

NTSTATUS
STDCALL
RtlIntegerToChar (
	IN	ULONG	Value,
	IN	ULONG	Base,
	IN	ULONG	Length,
	IN OUT	PCHAR	String
	);

NTSTATUS
STDCALL
RtlIntegerToUnicodeString (
	IN	ULONG		Value,
	IN	ULONG		Base,
	IN OUT	PUNICODE_STRING	String
	);

BOOLEAN
STDCALL
RtlIsGenericTableEmpty (
	IN	PRTL_GENERIC_TABLE	Table
	);

BOOLEAN
STDCALL
RtlIsNameLegalDOS8Dot3 (
	IN	PUNICODE_STRING	UnicodeName,
	IN OUT	PANSI_STRING	AnsiName,
	IN OUT	PBOOLEAN	SpacesFound
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerAdd (
	LARGE_INTEGER	Addend1,
	LARGE_INTEGER	Addend2
	);

/*
 * VOID
 * RtlLargeIntegerAnd (
 *	PLARGE_INTEGER	Result,
 *	LARGE_INTEGER	Source,
 *	LARGE_INTEGER	Mask
 *	);
 */
#define RtlLargeIntegerAnd(Result, Source, Mask) \
{ \
	Result.HighPart = Source.HighPart & Mask.HighPart; \
	Result.LowPart = Source.LowPart & Mask.LowPart; \
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerArithmeticShift (
	LARGE_INTEGER	LargeInteger,
	CCHAR	ShiftCount
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	LARGE_INTEGER	Divisor,
	PLARGE_INTEGER	Remainder
	);

/*
 * BOOLEAN
 * RtlLargeIntegerEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerEqualTo(X,Y) \
	(!(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))

/*
 * BOOLEAN
 * RtlLargeIntegerEqualToZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerEqualToZero(X) \
	(!((X).LowPart | (X).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThan (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerGreaterThan(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart > (Y).LowPart)) || \
	  ((X).HighPart > (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThanOrEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerGreaterThanOrEqualTo(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart >= (Y).LowPart)) || \
	  ((X).HighPart > (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThanOrEqualToZero (
 *	LARGE_INTEGER	Operand1
 *	);
 */
#define RtlLargeIntegerGreaterOrEqualToZero(X) \
	((X).HighPart >= 0)

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThanZero (
 *	LARGE_INTEGER	Operand1
 *	);
 */
#define RtlLargeIntegerGreaterThanZero(X) \
	((((X).HighPart == 0) && ((X).LowPart > 0)) || \
	  ((X).HighPart > 0 ))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThan (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerLessThan(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart < (Y).LowPart)) || \
	  ((X).HighPart < (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThanOrEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerLessThanOrEqualTo(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart <= (Y).LowPart)) || \
	  ((X).HighPart < (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThanOrEqualToZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerLessOrEqualToZero(X) \
	(((X).HighPart < 0) || !((X).LowPart | (X).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThanZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerLessThanZero(X) \
	(((X).HighPart < 0))

LARGE_INTEGER
STDCALL
RtlLargeIntegerNegate (
	LARGE_INTEGER	Subtrahend
	);

/*
 * BOOLEAN
 * RtlLargeIntegerNotEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerNotEqualTo(X,Y) \
	((((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))

/*
 * BOOLEAN
 * RtlLargeIntegerNotEqualToZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerNotEqualToZero(X) \
	(((X).LowPart | (X).HighPart))

LARGE_INTEGER
STDCALL
RtlLargeIntegerShiftLeft (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerShiftRight (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerSubtract (
	LARGE_INTEGER	Minuend,
	LARGE_INTEGER	Subtrahend
	);

ULONG
STDCALL
RtlLengthSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor
	);

BOOL
STDCALL
RtlLockHeap (
	HANDLE	hheap
	);

NTSTATUS
STDCALL
RtlLookupAtomInAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	PWSTR		AtomName,
	OUT	PRTL_ATOM	Atom
	);

VOID STDCALL
RtlMoveMemory (PVOID Destination, CONST VOID* Source, ULONG Length);

NTSTATUS
STDCALL
RtlMultiByteToUnicodeN (
	PWCHAR UnicodeString,
	ULONG  UnicodeSize,
	PULONG ResultSize,
	PCHAR  MbString,
	ULONG  MbSize
	);

NTSTATUS
STDCALL
RtlMultiByteToUnicodeSize (
	PULONG UnicodeSize,
	PCHAR  MbString,
	ULONG  MbSize
	);

DWORD
STDCALL
RtlNtStatusToDosError (
	NTSTATUS	StatusCode
	);

DWORD
STDCALL
RtlNtStatusToDosErrorNoTeb (
	NTSTATUS	StatusCode
	);

int
STDCALL
RtlNtStatusToPsxErrno (
	NTSTATUS	StatusCode
	);

ULONG
STDCALL
RtlNumberGenericTableElements (
	IN	PRTL_GENERIC_TABLE	Table
	);

ULONG
STDCALL
RtlNumberOfClearBits (
	PRTL_BITMAP	BitMapHeader
	);

ULONG
STDCALL
RtlNumberOfSetBits (
	PRTL_BITMAP	BitMapHeader
	);

ULONG
STDCALL
RtlOemStringToUnicodeSize (
	POEM_STRING	AnsiString
	);

NTSTATUS
STDCALL
RtlOemStringToUnicodeString (
	PUNICODE_STRING	DestinationString,
	POEM_STRING	SourceString,
	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlOemToUnicodeN (
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize,
	PULONG	ResultSize,
	PCHAR	OemString,
	ULONG	OemSize
	);

NTSTATUS
STDCALL
RtlOpenCurrentUser (
	IN	ACCESS_MASK	DesiredAccess,
	OUT	PHANDLE		KeyHandle
	);

NTSTATUS STDCALL
RtlPinAtomInAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	RTL_ATOM	Atom
	);

BOOLEAN
STDCALL
RtlPrefixString (
	PANSI_STRING	String1,
	PANSI_STRING	String2,
	BOOLEAN		CaseInsensitive
	);

BOOLEAN
STDCALL
RtlPrefixUnicodeString (
	PUNICODE_STRING	String1,
	PUNICODE_STRING	String2,
	BOOLEAN		CaseInsensitive
	);

NTSTATUS
STDCALL
RtlQueryAtomInAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	RTL_ATOM	Atom,
	IN OUT	PULONG		RefCount OPTIONAL,
	IN OUT	PULONG		PinCount OPTIONAL,
	IN OUT	PWSTR		AtomName OPTIONAL,
	IN OUT	PULONG		NameLength OPTIONAL
	);

NTSTATUS
STDCALL
RtlQueryRegistryValues (
	IN	ULONG				RelativeTo,
	IN	PWSTR				Path,
	IN	PRTL_QUERY_REGISTRY_TABLE	QueryTable,
	IN	PVOID				Context,
	IN	PVOID				Environment
	);

NTSTATUS
STDCALL
RtlQueryTimeZoneInformation (
	IN OUT	PTIME_ZONE_INFORMATION	TimeZoneInformation
	);

VOID
STDCALL
RtlRaiseException (
	IN	PEXCEPTION_RECORD	ExceptionRecord
	);

LPVOID
STDCALL
RtlReAllocateHeap (
	HANDLE	hheap,
	DWORD	flags,
	LPVOID	ptr,
	DWORD	size
	);

/*
 * VOID
 * RtlRetrieveUlong (
 *	PULONG	DestinationAddress,
 *	PULONG	SourceAddress
 *	);
 */
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
	if ((ULONG)(SrcAddress) & LONG_MASK) \
	{ \
		((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
		((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
		((PUCHAR)(DestAddress))[2]=((PUCHAR)(SrcAddress))[2]; \
		((PUCHAR)(DestAddress))[3]=((PUCHAR)(SrcAddress))[3]; \
	} \
	else \
	{ \
		*((PULONG)(DestAddress))=*((PULONG)(SrcAddress)); \
	}

/*
 * VOID
 * RtlRetrieveUshort (
 *	PUSHORT	DestinationAddress,
 *	PUSHORT	SourceAddress
 *	);
 */
#define RtlRetrieveUshort(DestAddress,SrcAddress) \
	if ((ULONG)(SrcAddress) & SHORT_MASK) \
	{ \
		((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
		((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
	} \
	else \
	{ \
		*((PUSHORT)(DestAddress))=*((PUSHORT)(SrcAddress)); \
	}

VOID
STDCALL
RtlSecondsSince1970ToTime (
	ULONG SecondsSince1970,
	PLARGE_INTEGER Time
	);

VOID
STDCALL
RtlSecondsSince1980ToTime (
	ULONG SecondsSince1980,
	PLARGE_INTEGER Time
	);

VOID
STDCALL
RtlSetAllBits (
	IN	PRTL_BITMAP	BitMapHeader
	);

VOID
STDCALL
RtlSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		NumberToSet
	);

NTSTATUS
STDCALL
RtlSetDaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	BOOLEAN			DaclPresent,
	PACL			Dacl,
	BOOLEAN			DaclDefaulted
	);

NTSTATUS
STDCALL
RtlSetTimeZoneInformation (
	IN OUT	PTIME_ZONE_INFORMATION	TimeZoneInformation
	);

DWORD
STDCALL
RtlSizeHeap (
	HANDLE	hheap,
	DWORD	flags,
	PVOID	pmem
	);

/*
 * VOID
 * RtlStoreUlong (
 *	PULONG	Address,
 *	ULONG	Value
 *	);
 */
#define RtlStoreUlong(Address,Value) \
	if ((ULONG)(Address) & LONG_MASK) \
	{ \
		((PUCHAR)(Address))[LONG_LEAST_SIGNIFICANT_BIT]=(UCHAR)(FIRSTBYTE(Value)); \
		((PUCHAR)(Address))[LONG_3RD_MOST_SIGNIFICANT_BIT]=(UCHAR)(FIRSTBYTE(Value)); \
		((PUCHAR)(Address))[LONG_2ND_MOST_SIGNIFICANT_BIT]=(UCHAR)(THIRDBYTE(Value)); \
		((PUCHAR)(Address))[LONG_MOST_SIGNIFICANT_BIT]=(UCHAR)(FOURTHBYTE(Value)); \
	} \
	else \
	{ \
		*((PULONG)(Address))=(ULONG)(Value); \
	}

/*
 * VOID
 * RtlStoreUshort (
 *	PUSHORT	Address,
 *	USHORT	Value
 *	);
 */
#define RtlStoreUshort(Address,Value) \
	if ((ULONG)(Address) & SHORT_MASK) \
	{ \
		((PUCHAR)(Address))[SHORT_LEAST_SIGNIFICANT_BIT]=(UCHAR)(FIRSTBYTE(Value)); \
		((PUCHAR)(Address))[SHORT_MOST_SIGNIFICANT_BIT]=(UCHAR)(SECONDBYTE(Value)); \
	} \
	else \
	{ \
		*((PUSHORT)(Address))=(USHORT)(Value); \
	}

BOOLEAN
STDCALL
RtlTimeFieldsToTime (
	PTIME_FIELDS	TimeFields,
	PLARGE_INTEGER	Time
	);

BOOLEAN
STDCALL
RtlTimeToSecondsSince1970 (
	PLARGE_INTEGER Time,
	PULONG SecondsSince1970
	);

BOOLEAN
STDCALL
RtlTimeToSecondsSince1980 (
	PLARGE_INTEGER Time,
	PULONG SecondsSince1980
	);

VOID
STDCALL
RtlTimeToTimeFields (
	PLARGE_INTEGER	Time,
	PTIME_FIELDS	TimeFields
	);

ULONG
STDCALL
RtlUnicodeStringToAnsiSize (
	IN	PUNICODE_STRING	UnicodeString
	);

NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString (
	IN OUT	PANSI_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUnicodeStringToInteger (
	IN	PUNICODE_STRING	String,
	IN	ULONG		Base,
	OUT	PULONG		Value
	);

ULONG
STDCALL
RtlUnicodeStringToOemSize (
	IN	PUNICODE_STRING	UnicodeString
	);

NTSTATUS
STDCALL
RtlUnicodeStringToCountedOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUnicodeStringToOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUnicodeToCustomCPN (
	PRTL_NLS_DATA	NlsData,
	PCHAR		MbString,
	ULONG		MbSize,
	PULONG		ResultSize,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize
	);

NTSTATUS
STDCALL
RtlUnicodeToMultiByteN (
	PCHAR	MbString,
	ULONG	MbSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

NTSTATUS
STDCALL
RtlUnicodeToMultiByteSize (
	PULONG	MbSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

NTSTATUS
STDCALL
RtlUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

BOOL
STDCALL
RtlUnlockHeap (
	HANDLE	hheap
	);

VOID
STDCALL
RtlUnwind (
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3,
	ULONG	Unknown4
	);

WCHAR
STDCALL
RtlUpcaseUnicodeChar (
	WCHAR Source
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeString (
	IN OUT	PUNICODE_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToAnsiString (
	IN OUT	PANSI_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToCountedOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeToCustomCPN (
	PRTL_NLS_DATA	NlsData,
	PCHAR		MbString,
	ULONG		MbSize,
	PULONG		ResultSize,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeToMultiByteN (
	PCHAR	MbString,
	ULONG	MbSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

CHAR
STDCALL
RtlUpperChar (
	CHAR	Source
	);

VOID
STDCALL
RtlUpperString (
	PSTRING	DestinationString,
	PSTRING	SourceString
	);

BOOL
STDCALL
RtlValidateHeap (
	HANDLE	hheap,
	DWORD	flags,
	PVOID	pmem
	);

BOOLEAN
STDCALL
RtlValidSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor
	);

NTSTATUS
STDCALL
RtlWriteRegistryValue (
	ULONG	RelativeTo,
	PWSTR	Path,
	PWSTR	ValueName,
	ULONG	ValueType,
	PVOID	ValueData,
	ULONG	ValueLength
	);

VOID STDCALL
RtlZeroMemory (PVOID Destination, ULONG Length);

ULONG
STDCALL
RtlxAnsiStringToUnicodeSize (
	IN	PANSI_STRING 	AnsiString
	);

ULONG
STDCALL
RtlxOemStringToUnicodeSize (
	IN	POEM_STRING	OemString
	);

ULONG
STDCALL
RtlxUnicodeStringToAnsiSize (
	IN	PUNICODE_STRING	UnicodeString
	);

ULONG
STDCALL
RtlxUnicodeStringToOemSize (
	IN	PUNICODE_STRING	UnicodeString
	);


/* Register io functions */

UCHAR
STDCALL
READ_REGISTER_UCHAR (
	PUCHAR	Register
	);

USHORT
STDCALL
READ_REGISTER_USHORT (
	PUSHORT	Register
	);

ULONG
STDCALL
READ_REGISTER_ULONG (
	PULONG	Register
	);

VOID
STDCALL
READ_REGISTER_BUFFER_UCHAR (
	PUCHAR	Register,
	PUCHAR	Buffer,
	ULONG	Count
	);

VOID
STDCALL
READ_REGISTER_BUFFER_USHORT (
	PUSHORT	Register,
	PUSHORT	Buffer,
	ULONG	Count
	);

VOID
STDCALL
READ_REGISTER_BUFFER_ULONG (
	PULONG	Register,
	PULONG	Buffer,
	ULONG	Count
	);

VOID
STDCALL
WRITE_REGISTER_UCHAR (
	PUCHAR	Register,
	UCHAR	Value
	);

VOID
STDCALL
WRITE_REGISTER_USHORT (
	PUSHORT	Register,
	USHORT	Value
	);

VOID
STDCALL
WRITE_REGISTER_ULONG (
	PULONG	Register,
	ULONG	Value
	);

VOID
STDCALL
WRITE_REGISTER_BUFFER_UCHAR (
	PUCHAR	Register,
	PUCHAR	Buffer,
	ULONG	Count
	);

VOID
STDCALL
WRITE_REGISTER_BUFFER_USHORT (
	PUSHORT	Register,
	PUSHORT	Buffer,
	ULONG	Count
	);

VOID
STDCALL
WRITE_REGISTER_BUFFER_ULONG (
	PULONG	Register,
	PULONG	Buffer,
	ULONG	Count
	);


NTSTATUS STDCALL RtlCreateAcl(PACL Acl, ULONG AclSize, ULONG AclRevision);
NTSTATUS STDCALL RtlQueryInformationAcl (PACL Acl, PVOID Information, ULONG InformationLength, ACL_INFORMATION_CLASS InformationClass);
NTSTATUS STDCALL RtlSetInformationAcl (PACL Acl, PVOID Information, ULONG InformationLength, ACL_INFORMATION_CLASS InformationClass);
BOOLEAN STDCALL RtlValidAcl (PACL Acl);

NTSTATUS STDCALL RtlAddAccessAllowedAce(PACL Acl, ULONG Revision, ACCESS_MASK AccessMask, PSID Sid);
NTSTATUS STDCALL RtlAddAccessDeniedAce(PACL Acl, ULONG Revision, ACCESS_MASK AccessMask, PSID Sid);
NTSTATUS STDCALL RtlAddAce(PACL Acl, ULONG Revision, ULONG StartingIndex, PACE AceList, ULONG AceListLength);
NTSTATUS STDCALL RtlAddAuditAccessAce (PACL Acl, ULONG Revision, ACCESS_MASK AccessMask, PSID Sid, BOOLEAN Success, BOOLEAN Failure);
NTSTATUS STDCALL RtlDeleteAce(PACL Acl, ULONG AceIndex);
BOOLEAN STDCALL RtlFirstFreeAce(PACL Acl, PACE* Ace);
NTSTATUS STDCALL RtlGetAce(PACL Acl, ULONG AceIndex, PACE *Ace);

NTSTATUS STDCALL RtlAbsoluteToSelfRelativeSD (PSECURITY_DESCRIPTOR AbsSD, PSECURITY_DESCRIPTOR RelSD, PULONG BufferLength);
NTSTATUS STDCALL RtlMakeSelfRelativeSD (PSECURITY_DESCRIPTOR AbsSD, PSECURITY_DESCRIPTOR RelSD, PULONG BufferLength);
NTSTATUS STDCALL RtlCreateSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG Revision);
BOOLEAN STDCALL RtlValidSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor);
ULONG STDCALL RtlLengthSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor);
NTSTATUS STDCALL RtlSetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, BOOLEAN DaclPresent, PACL Dacl, BOOLEAN DaclDefaulted);
NTSTATUS STDCALL RtlGetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PBOOLEAN DaclPresent, PACL* Dacl, PBOOLEAN DaclDefauted);
NTSTATUS STDCALL RtlSetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID Owner, BOOLEAN OwnerDefaulted);
NTSTATUS STDCALL RtlGetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID* Owner, PBOOLEAN OwnerDefaulted);
NTSTATUS STDCALL RtlSetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID Group, BOOLEAN GroupDefaulted);
NTSTATUS STDCALL RtlGetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID* Group, PBOOLEAN GroupDefaulted);
NTSTATUS STDCALL RtlGetControlSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSECURITY_DESCRIPTOR_CONTROL Control, PULONG Revision);
NTSTATUS STDCALL RtlSetSaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, BOOLEAN SaclPresent, PACL Sacl, BOOLEAN SaclDefaulted);
NTSTATUS STDCALL RtlGetSaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PBOOLEAN SaclPresent, PACL* Sacl, PBOOLEAN SaclDefauted);
NTSTATUS STDCALL RtlSelfRelativeToAbsoluteSD (PSECURITY_DESCRIPTOR RelSD,
					      PSECURITY_DESCRIPTOR AbsSD,
					      PDWORD AbsSDSize,
					      PACL Dacl,
					      PDWORD DaclSize,
					      PACL Sacl,
					      PDWORD SaclSize,
					      PSID Owner,
					      PDWORD OwnerSize,
					      PSID Group,
					      PDWORD GroupSize);

NTSTATUS STDCALL RtlAllocateAndInitializeSid (PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
					      UCHAR SubAuthorityCount,
					      ULONG SubAuthority0,
					      ULONG SubAuthority1,
					      ULONG SubAuthority2,
					      ULONG SubAuthority3,
					      ULONG SubAuthority4,
					      ULONG SubAuthority5,
					      ULONG SubAuthority6,
					      ULONG SubAuthority7,
					      PSID *Sid);
ULONG STDCALL RtlLengthRequiredSid (UCHAR SubAuthorityCount);
PSID_IDENTIFIER_AUTHORITY STDCALL RtlIdentifierAuthoritySid (PSID Sid);
NTSTATUS STDCALL RtlInitializeSid (PSID Sid, PSID_IDENTIFIER_AUTHORITY IdentifierAuthority, UCHAR SubAuthorityCount);
PULONG STDCALL RtlSubAuthoritySid (PSID Sid, ULONG SubAuthority);
NTSTATUS STDCALL RtlCopySid (ULONG BufferLength, PSID Dest, PSID Src);
BOOLEAN STDCALL RtlEqualPrefixSid (PSID Sid1, PSID Sid2);
BOOLEAN STDCALL RtlEqualSid(PSID Sid1, PSID Sid2);
PSID STDCALL RtlFreeSid (PSID Sid);
ULONG STDCALL RtlLengthSid (PSID Sid);
PULONG STDCALL RtlSubAuthoritySid (PSID Sid, ULONG SubAuthority);
PUCHAR STDCALL RtlSubAuthorityCountSid (PSID Sid);
BOOLEAN STDCALL RtlValidSid (PSID Sid);
NTSTATUS STDCALL RtlConvertSidToUnicodeString (PUNICODE_STRING String, PSID Sid, BOOLEAN AllocateBuffer);

BOOLEAN STDCALL RtlAreAllAccessesGranted (ACCESS_MASK GrantedAccess, ACCESS_MASK DesiredAccess);
BOOLEAN STDCALL RtlAreAnyAccessesGranted (ACCESS_MASK GrantedAccess, ACCESS_MASK DesiredAccess);
VOID STDCALL RtlMapGenericMask (PACCESS_MASK AccessMask, PGENERIC_MAPPING GenericMapping);


/*  functions exported from NTOSKRNL.EXE which are considered RTL  */

#if defined(__NTOSKRNL__) || defined(__NTDRIVER__) || defined(__NTHAL__) || defined(__NTDLL__) || defined(__NTAPP__)

char *_itoa (int value, char *string, int radix);
int _snprintf(char * buf, size_t cnt, const char *fmt, ...);
int _snwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, ...);
int _stricmp(const char *s1, const char *s2);
char * _strlwr(char *x);
int _strnicmp(const char *s1, const char *s2, size_t n);
char * _strnset(char* szToFill, int szFill, size_t sizeMaxFill);
char * _strrev(char *s);
char * _strset(char* szToFill, int szFill);
char * _strupr(char *x);
int _vsnprintf(char *buf, size_t cnt, const char *fmt, va_list args);
int _wcsicmp (const wchar_t* cs, const wchar_t* ct);
wchar_t * _wcslwr (wchar_t *x);
int _wcsnicmp (const wchar_t * cs,const wchar_t * ct,size_t count);
wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill);
wchar_t * _wcsrev(wchar_t *s);
wchar_t *_wcsupr(wchar_t *x);

int atoi(const char *str);
long atol(const char *str);
int isdigit(int c);
int islower(int c);
int isprint(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
size_t mbstowcs (wchar_t *wcstr, const char *mbstr, size_t count);
int mbtowc (wchar_t *wchar, const char *mbchar, size_t count);
void * memchr(const void *s, int c, size_t n);
void * memcpy(void *to, const void *from, size_t count);
void * memmove(void *dest,const void *src, size_t count);
void * memset(void *src, int val, size_t count);

#if 0
qsort
#endif

int rand(void);
int sprintf(char * buf, const char *fmt, ...);
void srand(unsigned seed);
char * strcat(char *s, const char *append);
char * strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
char * strcpy(char *to, const char *from);
size_t strlen(const char *str);
char * strncat(char *dst, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
char *strrchr(const char *s, int c);
size_t strspn(const char *s1, const char *s2);
char *strstr(const char *s, const char *find);
int swprintf(wchar_t *buf, const wchar_t *fmt, ...);
int tolower(int c);
int toupper(int c);
wchar_t towlower(wchar_t c);
wchar_t towupper(wchar_t c);
int vsprintf(char *buf, const char *fmt, va_list args);
wchar_t * wcscat(wchar_t *dest, const wchar_t *src);
wchar_t * wcschr(const wchar_t *str, wchar_t ch);
int wcscmp(const wchar_t *cs, const wchar_t *ct);
wchar_t* wcscpy(wchar_t* str1, const wchar_t* str2);
size_t wcscspn(const wchar_t *str,const wchar_t *reject);
size_t wcslen(const wchar_t *s);
wchar_t * wcsncat(wchar_t *dest, const wchar_t *src, size_t count);
int wcsncmp(const wchar_t *cs, const wchar_t *ct, size_t count);
wchar_t * wcsncpy(wchar_t *dest, const wchar_t *src, size_t count);
wchar_t * wcsrchr(const wchar_t *str, wchar_t ch);
size_t wcsspn(const wchar_t *str,const wchar_t *accept);
wchar_t *wcsstr(const wchar_t *s,const wchar_t *b);
size_t wcstombs (char *mbstr, const wchar_t *wcstr, size_t count);
int wctomb (char *mbchar, wchar_t wchar);

#endif /* __NTOSKRNL__ || __NTDRIVER__ || __NTHAL__ || __NTDLL__ || __NTAPP__ */

#endif /* __DDK_RTL_H */
