/*
 * 
 */

#ifndef __DDK_RTL_H
#define __DDK_RTL_H

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

typedef struct _ANSI_STRING
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
} ANSI_STRING, *PANSI_STRING;

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
VOID InitializeObjectAttributes(POBJECT_ATTRIBUTES InitializedAttributes,
				PUNICODE_STRING ObjectName,
				ULONG Attributes,
				HANDLE RootDirectory,
                                PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID InitializeListHead(PLIST_ENTRY ListHead);
VOID InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
VOID InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
BOOLEAN IsListEmpty(PLIST_ENTRY ListHead);
PSINGLE_LIST_ENTRY PopEntryList(PSINGLE_LIST_ENTRY ListHead);
VOID PushEntryList(PSINGLE_LIST_ENTRY ListHead, PSINGLE_LIST_ENTRY Entry);
VOID RemoveEntryList(PLIST_ENTRY Entry);
PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead);
PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead);
ULONG RtlAnsiStringToUnicodeSize(PANSI_STRING AnsiString);
NTSTATUS RtlAnsiStringToUnicodeString(PUNICODE_STRING DestinationString,
				      PANSI_STRING SourceString,
				      BOOLEAN AllocateDestinationString);
NTSTATUS RtlAppendUnicodeStringToString(PUNICODE_STRING Destination,
					PUNICODE_STRING Source);
NTSTATUS RtlAppendUnicodeToString(PUNICODE_STRING Destination,
				  PWSTR Source);
NTSTATUS RtlCharToInteger(PCSZ String, ULONG Base, PULONG Value);
NTSTATUS RtlCheckRegistryKey(ULONG RelativeTo, PWSTR Path);
ULONG RtlCompareMemory(PVOID Source1, PVOID Source2, ULONG Length);
LONG RtlCompareString(PSTRING String1, PSTRING String2, 
		      BOOLEAN CaseInsensitive);
LONG RtlCompareUnicodeString(PUNICODE_STRING String1,
			     PUNICODE_STRING String2,
			     BOOLEAN BaseInsensitive);
VOID RtlCopyBytes(PVOID Destination, CONST VOID* Source, ULONG Length);
VOID RtlCopyMemory(VOID* Destination, VOID* Source, ULONG Length);
VOID RtlCopyString(PSTRING DestinationString, PSTRING SourceString);
VOID RtlCopyUnicodeString(PUNICODE_STRING DestinationString,
			  PUNICODE_STRING SourceString);
NTSTATUS RtlCreateRegistryKey(ULONG RelativeTo,
			      PWSTR Path);
NTSTATUS RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				     ULONG Revision);
NTSTATUS RtlDeleteRegistryValue(ULONG RelativeTo,
				PWSTR Path,
				PWSTR ValueName);
BOOLEAN RtlEqualString(PSTRING String1,
		       PSTRING String2,
		       BOOLEAN CaseInSensitive);
BOOLEAN RtlEqualUnicodeString(PUNICODE_STRING String1,
			      PUNICODE_STRING String2,
			      BOOLEAN CaseInSensitive);
VOID RtlFillMemory(PVOID Destination, ULONG Length, UCHAR Fill);
VOID RtlFreeAnsiString(PANSI_STRING AnsiString);
VOID RtlFreeUnicodeString(PUNICODE_STRING UnicodeString);
VOID RtlInitAnsiString(PANSI_STRING DestinationString,
		       PCSZ SourceString);
VOID RtlInitString(PSTRING DestinationString, PCSZ SourceString);
VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString,
			  PCWSTR SourceString);
NTSTATUS RtlIntegerToUnicodeString(ULONG Value,
				   ULONG Base,
				   PUNICODE_STRING String);

/**  LARGE_INTEGER Functions  *******************************************/
LARGE_INTEGER RtlConvertLongToLargeInteger(LONG SignedInteger);
LARGE_INTEGER RtlConvertUlongToLargeInteger(ULONG UnsignedInteger);
LARGE_INTEGER RtlEnlargedIntegerMultiply(LONG Multiplicand,
                                         LONG Multiplier);
ULONG RtlEnlargedUnsignedDivide(ULARGE_INTEGER Dividend,
				ULONG Divisor,
				PULONG Remainder);
LARGE_INTEGER RtlEnlargedUnsignedMultiply(ULONG Multiplicand,
					  ULONG Multipler);
LARGE_INTEGER RtlExtendedIntegerMultiply(LARGE_INTEGER Multiplicand,
                                         LONG Multiplier);
LARGE_INTEGER RtlExtendedLargeIntegerDivide(LARGE_INTEGER Dividend,
					    ULONG Divisor,
					    PULONG Remainder);
LARGE_INTEGER RtlExtendedMagicDivide(LARGE_INTEGER Dividend,
				     LARGE_INTEGER MagicDivisor,
				     CCHAR ShiftCount);
LARGE_INTEGER ExInterlockedAddLargeInteger(PLARGE_INTEGER Addend,
					   LARGE_INTEGER Increment,
					   PKSPIN_LOCK Lock);
LARGE_INTEGER RtlLargeIntegerAdd(LARGE_INTEGER Addend1,
                                 LARGE_INTEGER Addend2);
VOID RtlLargeIntegerAnd(PLARGE_INTEGER Result,
			LARGE_INTEGER Source,
			LARGE_INTEGER Mask);
LARGE_INTEGER RtlLargeIntegerArithmeticShift(LARGE_INTEGER LargeInteger,
					     CCHAR ShiftCount);
LARGE_INTEGER RtlLargeIntegerDivide(LARGE_INTEGER Dividend,
				    LARGE_INTEGER Divisor,
				    PLARGE_INTEGER Remainder);
BOOLEAN RtlLargeIntegerEqualTo(LARGE_INTEGER Operand1,
                               LARGE_INTEGER Operand2);
BOOLEAN RtlLargeIntegerEqualToZero(LARGE_INTEGER Operand);
BOOLEAN RtlLargeIntegerGreaterThan(LARGE_INTEGER Operand1,
                                   LARGE_INTEGER Operand2);
BOOLEAN RtlLargeIntegerGreaterThanOrEqualTo(LARGE_INTEGER Operand1,
                                            LARGE_INTEGER Operand2);
BOOLEAN RtlLargeIntegerGreaterThanOrEqualToZero(LARGE_INTEGER Operand1);
BOOLEAN RtlLargeIntegerGreaterThanZero(LARGE_INTEGER Operand1);
BOOLEAN RtlLargeIntegerLessThan(LARGE_INTEGER Operand1,
                                LARGE_INTEGER Operand2);
BOOLEAN RtlLargeIntegerLessThanOrEqualTo(LARGE_INTEGER Operand1,
                                         LARGE_INTEGER Operand2);
BOOLEAN RtlLargeIntegerLessThanOrEqualToZero(LARGE_INTEGER Operand);
BOOLEAN RtlLargeIntegerLessThanZero(LARGE_INTEGER Operand);
LARGE_INTEGER RtlLargeIntegerNegate(LARGE_INTEGER Subtrahend);
BOOLEAN RtlLargeIntegerNotEqualTo(LARGE_INTEGER Operand1,
                                  LARGE_INTEGER Operand2);
BOOLEAN RtlLargeIntegerNotEqualToZero(LARGE_INTEGER Operand);
LARGE_INTEGER RtlLargeIntegerShiftLeft(LARGE_INTEGER LargeInteger,
				       CCHAR ShiftCount);
LARGE_INTEGER RtlLargeIntegerShiftRight(LARGE_INTEGER LargeInteger,
					CCHAR ShiftCount);
LARGE_INTEGER RtlLargeIntegerSubtract(LARGE_INTEGER Minuend,
				      LARGE_INTEGER Subtrahend);

/* MISSING FUNCTIONS GO HERE */

ULONG RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor);
VOID RtlMoveMemory(PVOID Destination, CONST VOID* Source, ULONG Length);
NTSTATUS RtlQueryRegistryValues(ULONG RelativeTo,
				PWSTR Path, 
				PRTL_QUERY_REGISTRY_TABLE QueryTable,
				PVOID Context, PVOID Environment);
VOID RtlRetrieveUlong(PULONG DestinationAddress,
		      PULONG SourceAddress);
VOID RtlRetrieveUshort(PUSHORT DestinationAddress,
		       PUSHORT SourceAddress);
NTSTATUS RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				      BOOLEAN DaclPresent,
				      PACL Dacl,
				      BOOLEAN DaclDefaulted);
VOID RtlStoreLong(PULONG Address, ULONG Value);
VOID RtlStoreUshort(PUSHORT Address, USHORT Value);
BOOLEAN RtlTimeFieldsToTime(PTIME_FIELDS TimeFields, PLARGE_INTEGER Time);
VOID RtlTimeToTimeFields(PLARGE_INTEGER Time, PTIME_FIELDS TimeFields);
PWSTR RtlStrtok(PUNICODE_STRING _string, PWSTR _sep, PWSTR* temp);
VOID RtlGetCallersAddress(PVOID* CallersAddress);
VOID RtlZeroMemory(PVOID Destination, ULONG Length);

typedef struct {
	ULONG    	Length;
	ULONG    	Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

// Heap creation routine

HANDLE 
STDCALL
RtlCreateHeap(
	ULONG Flags, 
	PVOID BaseAddress, 
	ULONG SizeToReserve, 
	ULONG SizeToCommit, 
	PVOID Unknown,
	PRTL_HEAP_DEFINITION Definition
	);

PVOID 
STDCALL 
RtlAllocateHeap(
	HANDLE Heap, 
	ULONG Flags, 
	ULONG Size 
	);


BOOLEAN 
STDCALL 
RtlFreeHeap(
	HANDLE Heap, 
	ULONG Flags, 
	PVOID Address 
	);

NTSTATUS RtlUnicodeStringToAnsiString(IN OUT PANSI_STRING DestinationString,
                                      IN PUNICODE_STRING SourceString,
                                      IN BOOLEAN AllocateDestinationString);
NTSTATUS RtlUnicodeStringToInteger(IN PUNICODE_STRING String, IN ULONG Base,
                                   OUT PULONG Value);
NTSTATUS RtlUpcaseUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                                IN PUNICODE_STRING SourceString,
                                IN BOOLEAN AllocateDestinationString);
VOID RtlUpperString(PSTRING DestinationString, PSTRING SourceString);
BOOLEAN RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor);
NTSTATUS RtlWriteRegistryValue(ULONG RelativeTo,
			       PWSTR Path,
			       PWSTR ValueName,
			       ULONG ValueType,
			       PVOID ValueData,
			       ULONG ValueLength);





VOID RtlStoreUlong(PULONG Address,
		   ULONG Value);



#endif /* __DDK_RTL_H */
