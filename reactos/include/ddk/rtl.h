/* $Id: rtl.h,v 1.18 1999/11/20 21:44:09 ekohl Exp $
 * 
 */

#ifndef __DDK_RTL_H
#define __DDK_RTL_H

#include <stddef.h>

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

/*
 * PURPOSE: Flags for RtlQueryRegistryValues
 */
enum
{
   RTL_QUERY_REGISTRY_SUBKEY,
   RTL_QUERY_REGISTRY_TOPKEY,
   RTL_QUERY_REGISTRY_REQUIRED,
   RTL_QUERY_REGISTRY_NOVALUE,
   RTL_QUERY_REGISTRY_NOEXPAND,
   RTL_QUERY_REGISTRY_DIRECT,
   RTL_QUERY_REGISTRY_DELETE,
};

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
enum
{
   RTL_REGISTRY_ABSOLUTE,
   RTL_REGISTRY_SERVICES,
   RTL_REGISTRY_CONTROL,
   RTL_REGISTRY_WINDOWS_NT,
   RTL_REGISTRY_DEVICEMAP,
   RTL_REGISTRY_USER,
   RTL_REGISTRY_OPTIONAL,
   RTL_REGISTRY_VALUE,
};


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
VOID
InitializeObjectAttributes (
	POBJECT_ATTRIBUTES	InitializedAttributes,
	PUNICODE_STRING		ObjectName,
	ULONG			Attributes,
	HANDLE			RootDirectory,
	PSECURITY_DESCRIPTOR	SecurityDescriptor
	);

VOID
InitializeListHead (
	PLIST_ENTRY	ListHead
	);

VOID
InsertHeadList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	Entry
	);

VOID
InsertTailList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	Entry
	);

BOOLEAN
IsListEmpty (
	PLIST_ENTRY	ListHead
	);

PSINGLE_LIST_ENTRY
PopEntryList (
	PSINGLE_LIST_ENTRY	ListHead
	);

VOID
PushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	Entry
	);

VOID
RemoveEntryList (
	PLIST_ENTRY	Entry
	);

PLIST_ENTRY
RemoveHeadList (
	PLIST_ENTRY	ListHead
	);

PLIST_ENTRY
RemoveTailList (
	PLIST_ENTRY	ListHead
	);

PVOID
STDCALL
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
RtlAppendStringToString(
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
RtlConvertLongToLargeInteger (
	LONG	SignedInteger
	);

LARGE_INTEGER
RtlConvertUlongToLargeInteger (
	ULONG	UnsignedInteger
	);

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
RtlCreateRegistryKey (
	ULONG	RelativeTo,
	PWSTR	Path
	);

NTSTATUS
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
RtlDeleteRegistryValue (
	ULONG	RelativeTo,
	PWSTR	Path,
	PWSTR	ValueName
	);

NTSTATUS
STDCALL
RtlDowncaseUnicodeString (
	IN OUT PUNICODE_STRING	DestinationString,
	IN PUNICODE_STRING	SourceString,
	IN BOOLEAN		AllocateDestinationString
	);

LARGE_INTEGER
RtlEnlargedIntegerMultiply (
	LONG	Multiplicand,
	LONG	Multiplier
	);

ULONG
RtlEnlargedUnsignedDivide (
	ULARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	);

LARGE_INTEGER
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

/* RtlEraseUnicodeString is exported by ntdll.dll only! */
VOID
STDCALL
RtlEraseUnicodeString (
	IN	PUNICODE_STRING	String
	);

LARGE_INTEGER
RtlExtendedIntegerMultiply (
	LARGE_INTEGER	Multiplicand,
	LONG		Multiplier
	);

LARGE_INTEGER
RtlExtendedLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	);

LARGE_INTEGER
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

VOID
STDCALL
RtlFreeAnsiString (
	PANSI_STRING	AnsiString
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
STDCALL
RtlGetDefaultCodePage (
	PUSHORT AnsiCodePage,
	PUSHORT OemCodePage
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

LARGE_INTEGER
RtlLargeIntegerAdd (
	LARGE_INTEGER	Addend1,
	LARGE_INTEGER	Addend2
	);

VOID
RtlLargeIntegerAnd (
	PLARGE_INTEGER	Result,
	LARGE_INTEGER	Source,
	LARGE_INTEGER	Mask
	);

LARGE_INTEGER
RtlLargeIntegerArithmeticShift (
	LARGE_INTEGER	LargeInteger,
	CCHAR	ShiftCount
	);

LARGE_INTEGER
RtlLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	LARGE_INTEGER	Divisor,
	PLARGE_INTEGER	Remainder
	);

BOOLEAN
RtlLargeIntegerEqualTo (
	LARGE_INTEGER	Operand1,
	LARGE_INTEGER	Operand2
	);

BOOLEAN
RtlLargeIntegerEqualToZero (
	LARGE_INTEGER	Operand
	);

BOOLEAN
RtlLargeIntegerGreaterThan (
	LARGE_INTEGER	Operand1,
	LARGE_INTEGER	Operand2
	);

BOOLEAN
RtlLargeIntegerGreaterThanOrEqualTo (
	LARGE_INTEGER	Operand1,
	LARGE_INTEGER	Operand2
	);

BOOLEAN
RtlLargeIntegerGreaterThanOrEqualToZero (
	LARGE_INTEGER	Operand1
	);

BOOLEAN
RtlLargeIntegerGreaterThanZero (
	LARGE_INTEGER	Operand1
	);

BOOLEAN
RtlLargeIntegerLessThan (
	LARGE_INTEGER	Operand1,
	LARGE_INTEGER	Operand2
	);

BOOLEAN
RtlLargeIntegerLessThanOrEqualTo (
	LARGE_INTEGER	Operand1,
	LARGE_INTEGER	Operand2
	);

BOOLEAN
RtlLargeIntegerLessThanOrEqualToZero (
	LARGE_INTEGER	Operand
	);

BOOLEAN
RtlLargeIntegerLessThanZero (
	LARGE_INTEGER	Operand
	);

LARGE_INTEGER
RtlLargeIntegerNegate (
	LARGE_INTEGER	Subtrahend
	);

BOOLEAN
RtlLargeIntegerNotEqualTo (
	LARGE_INTEGER	Operand1,
	LARGE_INTEGER	Operand2
	);

BOOLEAN
RtlLargeIntegerNotEqualToZero (
	LARGE_INTEGER	Operand
	);

LARGE_INTEGER
RtlLargeIntegerShiftLeft (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	);

LARGE_INTEGER
RtlLargeIntegerShiftRight (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	);

LARGE_INTEGER
RtlLargeIntegerSubtract (
	LARGE_INTEGER	Minuend,
	LARGE_INTEGER	Subtrahend
	);

ULONG
RtlLengthSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor
	);

VOID
STDCALL
RtlMoveMemory (
	PVOID		Destination,
	CONST VOID	* Source,
	ULONG		Length
	);

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
	PWCHAR UnicodeString,
	ULONG  UnicodeSize,
	PULONG ResultSize,
	PCHAR  OemString,
	ULONG  OemSize
	);

NTSTATUS
STDCALL
RtlQueryRegistryValues (
	ULONG				RelativeTo,
	PWSTR				Path,
	PRTL_QUERY_REGISTRY_TABLE	QueryTable,
	PVOID				Context,
	PVOID				Environment
	);

VOID
RtlRetrieveUlong (
	PULONG	DestinationAddress,
	PULONG	SourceAddress
	);

VOID
RtlRetrieveUshort (
	PUSHORT	DestinationAddress,
	PUSHORT	SourceAddress
	);

NTSTATUS
RtlSetDaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	BOOLEAN			DaclPresent,
	PACL			Dacl,
	BOOLEAN			DaclDefaulted
	);

VOID
RtlStoreLong (
	PULONG	Address,
	ULONG	Value
	);

VOID
RtlStoreUshort (
	PUSHORT	Address,
	USHORT	Value
	);

BOOLEAN
RtlTimeFieldsToTime (
	PTIME_FIELDS	TimeFields,
	PLARGE_INTEGER	Time
	);

VOID
RtlTimeToTimeFields (
	PLARGE_INTEGER	Time,
	PTIME_FIELDS	TimeFields
	);

PWSTR
RtlStrtok (
	PUNICODE_STRING	_string,
	PWSTR		_sep,
	PWSTR		* temp
	);

VOID
RtlGetCallersAddress (
	PVOID	* CallersAddress
	);

VOID
STDCALL
RtlZeroMemory (
	PVOID	Destination,
	ULONG	Length
	);

typedef struct {
	ULONG		Length;
	ULONG		Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

// Heap creation routine

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


BOOLEAN
STDCALL
RtlFreeHeap (
	HANDLE	Heap,
	ULONG	Flags,
	PVOID	Address
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
RtlUnicodeStringToOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUnicodeToMultiByteN (
	PCHAR  MbString,
	ULONG  MbSize,
	PULONG ResultSize,
	PWCHAR UnicodeString,
	ULONG  UnicodeSize
	);

NTSTATUS
STDCALL
RtlUnicodeToMultiByteSize (
	PULONG MbSize,
	PWCHAR UnicodeString,
	ULONG UnicodeSize
	);

NTSTATUS
STDCALL
RtlUnicodeToOemN (
	PCHAR  OemString,
	ULONG  OemSize,
	PULONG ResultSize,
	PWCHAR UnicodeString,
	ULONG  UnicodeSize
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
RtlUpcaseUnicodeStringToOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
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

BOOLEAN
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

VOID
RtlStoreUlong (
	PULONG	Address,
	ULONG	Value
	);


DWORD
RtlNtStatusToDosError (
	NTSTATUS	StatusCode
	);



BOOL
WINAPI
RtlDestroyHeap (
	HANDLE	hheap
	);

LPVOID
STDCALL
RtlReAllocateHeap (
	HANDLE	hheap,
	DWORD	flags,
	LPVOID	ptr, 
	DWORD	size
	);

HANDLE
WINAPI
RtlGetProcessHeap (VOID);

BOOL
WINAPI
RtlLockHeap (
	HANDLE	hheap
	);

BOOL
WINAPI
RtlUnlockHeap (
	HANDLE	hheap
	);

UINT
STDCALL
RtlCompactHeap (
	HANDLE	hheap,
	DWORD	flags
	);

DWORD
WINAPI
RtlSizeHeap (
	HANDLE	hheap,
	DWORD	flags,
	PVOID	pmem
	);

BOOL
WINAPI
RtlValidateHeap (
	HANDLE	hheap,
	DWORD	flags,
	PVOID	pmem
	);

/* NtProcessStartup */

VOID
WINAPI
RtlDestroyProcessParameters(
	IN OUT	PSTARTUP_ARGUMENT	pArgument
	);
VOID
WINAPI
RtlDenormalizeProcessParams (
	IN OUT	PSTARTUP_ARGUMENT	pArgument
	);


NTSTATUS STDCALL
RtlInitializeContext(
        IN      HANDLE                  ProcessHandle,
        IN      PCONTEXT                Context,
        IN      PVOID                   Parameter,
        IN      PTHREAD_START_ROUTINE   StartAddress,
        IN OUT  PINITIAL_TEB            InitialTeb
        );


NTSTATUS STDCALL
RtlCreateUserThread(
        IN      HANDLE                  ProcessHandle,
        IN      PSECURITY_DESCRIPTOR    SecurityDescriptor,
        IN      BOOLEAN                 CreateSuspended,
        IN      LONG                    StackZeroBits,
        IN OUT  PULONG                  StackReserved,
        IN OUT  PULONG                  StackCommit,
        IN      PTHREAD_START_ROUTINE   StartAddress,
        IN      PVOID                   Parameter,
        IN OUT  PHANDLE                 ThreadHandle,
        IN OUT  PCLIENT_ID              ClientId
        );


NTSTATUS
STDCALL
RtlCreateUserProcess(PUNICODE_STRING ApplicationName,
                     PSECURITY_DESCRIPTOR ProcessSd,
                     PSECURITY_DESCRIPTOR ThreadSd,
                     WINBOOL bInheritHandles,
                     DWORD dwCreationFlags,
                     PCLIENT_ID ClientId,
                     PHANDLE ProcessHandle,
                     PHANDLE ThreadHandle);

/*  functions exported from NTOSKRNL.EXE which are considered RTL  */
#if 0
_stricmp
_strlwr
_strnicmp
_strnset
_strrev
_strset
_strupr
;_vsnprintf
#endif

int _wcsicmp (const wchar_t* cs, const wchar_t* ct);
wchar_t * _wcslwr (wchar_t *x);
int _wcsnicmp (const wchar_t * cs,const wchar_t * ct,size_t count);
wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill);
wchar_t * _wcsrev(wchar_t *s);
wchar_t *_wcsupr(wchar_t *x);

#if 0
;atoi
;atol
isdigit
islower
isprint
isspace
isupper
isxdigit
;mbstowcs
;mbtowc
memchr
memcpy
memmove
memset
;qsort
rand
sprintf
srand
strcat
strchr
strcmp
strcpy
strlen
strncat
strncmp
strcpy
strrchr
strspn
strstr
;strtok
;swprintf
tolower
toupper
towlower
towupper
vsprintf
#endif

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

#endif /* __DDK_RTL_H */
