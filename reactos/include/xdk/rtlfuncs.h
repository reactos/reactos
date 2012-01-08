/******************************************************************************
 *                         Runtime Library Functions                          *
 ******************************************************************************/

$if (_WDMDDK_)

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

#define RTL_STATIC_LIST_HEAD(x) LIST_ENTRY x = { &x, &x }

FORCEINLINE
VOID
InitializeListHead(
  OUT PLIST_ENTRY ListHead)
{
  ListHead->Flink = ListHead->Blink = ListHead;
}

FORCEINLINE
BOOLEAN
IsListEmpty(
  IN CONST LIST_ENTRY * ListHead)
{
  return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
BOOLEAN
RemoveEntryList(
  IN PLIST_ENTRY Entry)
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
  IN OUT PLIST_ENTRY ListHead)
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
  IN OUT PLIST_ENTRY ListHead)
{
  PLIST_ENTRY Blink;
  PLIST_ENTRY Entry;

  Entry = ListHead->Blink;
  Blink = Entry->Blink;
  ListHead->Blink = Blink;
  Blink->Flink = ListHead;
  return Entry;
}

FORCEINLINE
VOID
InsertTailList(
  IN OUT PLIST_ENTRY ListHead,
  IN OUT PLIST_ENTRY Entry)
{
  PLIST_ENTRY OldBlink;
  OldBlink = ListHead->Blink;
  Entry->Flink = ListHead;
  Entry->Blink = OldBlink;
  OldBlink->Flink = Entry;
  ListHead->Blink = Entry;
}

FORCEINLINE
VOID
InsertHeadList(
  IN OUT PLIST_ENTRY ListHead,
  IN OUT PLIST_ENTRY Entry)
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
AppendTailList(
  IN OUT PLIST_ENTRY ListHead,
  IN OUT PLIST_ENTRY ListToAppend)
{
  PLIST_ENTRY ListEnd = ListHead->Blink;

  ListHead->Blink->Flink = ListToAppend;
  ListHead->Blink = ListToAppend->Blink;
  ListToAppend->Blink->Flink = ListHead;
  ListToAppend->Blink = ListEnd;
}

FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryList(
  IN OUT PSINGLE_LIST_ENTRY ListHead)
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
  IN OUT PSINGLE_LIST_ENTRY ListHead,
  IN OUT PSINGLE_LIST_ENTRY Entry)
{
  Entry->Next = ListHead->Next;
  ListHead->Next = Entry;
}

#endif /* !defined(MIDL_PASS) && !defined(SORTPP_PASS) */

NTSYSAPI
VOID
NTAPI
RtlAssert(
  IN PVOID FailedAssertion,
  IN PVOID FileName,
  IN ULONG LineNumber,
  IN PSTR Message);

/* VOID
 * RtlCopyMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN CONST VOID UNALIGNED *Source,
 *     IN SIZE_T Length)
 */
#define RtlCopyMemory(Destination, Source, Length) \
    memcpy(Destination, Source, Length)

#define RtlCopyBytes RtlCopyMemory

#if defined(_M_AMD64)
NTSYSAPI
VOID
NTAPI
RtlCopyMemoryNonTemporal(
  VOID UNALIGNED *Destination,
  CONST VOID UNALIGNED *Source,
  SIZE_T Length);
#else
#define RtlCopyMemoryNonTemporal RtlCopyMemory
#endif

/* BOOLEAN
 * RtlEqualLuid(
 *     IN PLUID Luid1,
 *     IN PLUID Luid2)
 */
#define RtlEqualLuid(Luid1, Luid2) \
    (((Luid1)->LowPart == (Luid2)->LowPart) && ((Luid1)->HighPart == (Luid2)->HighPart))

/* ULONG
 * RtlEqualMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN CONST VOID UNALIGNED *Source,
 *     IN SIZE_T Length)
 */
#define RtlEqualMemory(Destination, Source, Length) \
    (!memcmp(Destination, Source, Length))

/* VOID
 * RtlFillMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN SIZE_T Length,
 *     IN UCHAR Fill)
 */
#define RtlFillMemory(Destination, Length, Fill) \
    memset(Destination, Fill, Length)

#define RtlFillBytes RtlFillMemory

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
  IN OUT PUNICODE_STRING UnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
  IN PUNICODE_STRING GuidString,
  OUT GUID *Guid);

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString OPTIONAL);

/* VOID
 * RtlMoveMemory(
 *    IN VOID UNALIGNED *Destination,
 *    IN CONST VOID UNALIGNED *Source,
 *    IN SIZE_T Length)
 */
#define RtlMoveMemory(Destination, Source, Length) \
    memmove(Destination, Source, Length)

NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
  IN REFGUID Guid,
  OUT PUNICODE_STRING GuidString);

/* VOID
 * RtlZeroMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN SIZE_T Length)
 */
#define RtlZeroMemory(Destination, Length) \
    memset(Destination, 0, Length)

#define RtlZeroBytes RtlZeroMemory
$endif (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)

$if (_WDMDDK_)
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG StartingIndex,
  IN ULONG Length);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG StartingIndex,
  IN ULONG Length);

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PANSI_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
  IN PCANSI_STRING AnsiString);

#define RtlAnsiStringToUnicodeSize(String) (               \
  NLS_MB_CODE_PAGE_TAG ?                                   \
  RtlxAnsiStringToUnicodeSize(String) :                    \
  ((String)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)   \
)

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
  IN OUT PUNICODE_STRING Destination,
  IN PCUNICODE_STRING Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
  IN OUT PUNICODE_STRING Destination,
  IN PCWSTR Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
  IN ULONG RelativeTo,
  IN PWSTR Path);

NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
  IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlClearBits(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG StartingIndex,
  IN ULONG NumberToClear);

NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory(
  IN CONST VOID *Source1,
  IN CONST VOID *Source2,
  IN SIZE_T Length);

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
  IN PCUNICODE_STRING String1,
  IN PCUNICODE_STRING String2,
  IN BOOLEAN CaseInSensitive);

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
  IN PCWCH String1,
  IN SIZE_T String1Length,
  IN PCWCH String2,
  IN SIZE_T String2Length,
  IN BOOLEAN CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlCopyUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCUNICODE_STRING SourceString OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
  IN ULONG RelativeTo,
  IN PWSTR Path);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN ULONG Revision);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRegistryValue(
  IN ULONG RelativeTo,
  IN PCWSTR Path,
  IN PCWSTR ValueName);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
  IN CONST UNICODE_STRING *String1,
  IN CONST UNICODE_STRING *String2,
  IN BOOLEAN CaseInSensitive);

#if !defined(_AMD64_) && !defined(_IA64_)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedIntegerMultiply(
  IN LARGE_INTEGER Multiplicand,
  IN LONG Multiplier);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN ULONG Divisor,
  OUT PULONG Remainder OPTIONAL);
#endif

#if defined(_X86_) || defined(_IA64_)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedMagicDivide(
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER MagicDivisor,
    IN CCHAR  ShiftCount);
#endif

NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
  IN PANSI_STRING AnsiString);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG NumberToFind,
  IN ULONG HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG NumberToFind,
  IN ULONG HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
  IN PRTL_BITMAP BitMapHeader,
  OUT PULONG StartingIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns(
  IN PRTL_BITMAP BitMapHeader,
  OUT PRTL_BITMAP_RUN RunArray,
  IN ULONG SizeOfRunArray,
  IN BOOLEAN LocateLongestRuns);

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG FromIndex,
  OUT PULONG StartingRunIndex);

NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit(
  IN ULONGLONG Set);

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
  IN PRTL_BITMAP BitMapHeader,
  OUT PULONG StartingIndex);

NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit(
  IN ULONGLONG Set);

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG FromIndex,
  OUT PULONG StartingRunIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBits(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG NumberToFind,
  IN ULONG HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG NumberToFind,
  IN ULONG HintIndex);

NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
  IN OUT PANSI_STRING DestinationString,
  IN PCSZ SourceString);

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
  IN PRTL_BITMAP BitMapHeader,
  IN PULONG BitMapBuffer,
  IN ULONG SizeOfBitMap);

NTSYSAPI
VOID
NTAPI
RtlInitString(
  IN OUT PSTRING DestinationString,
  IN PCSZ SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
  IN ULONG Value,
  IN ULONG Base OPTIONAL,
  IN OUT PUNICODE_STRING String);

NTSYSAPI
NTSTATUS
NTAPI
RtlInt64ToUnicodeString(
  IN ULONGLONG Value,
  IN ULONG Base OPTIONAL,
  IN OUT PUNICODE_STRING String);

#ifdef _WIN64
#define RtlIntPtrToUnicodeString(Value, Base, String) \
    RtlInt64ToUnicodeString(Value, Base, String)
#else
#define RtlIntPtrToUnicodeString(Value, Base, String) \
    RtlIntegerToUnicodeString(Value, Base, String)
#endif

/* BOOLEAN
 * RtlIsZeroLuid(
 *     IN PLUID L1);
 */
#define RtlIsZeroLuid(_L1) \
    ((BOOLEAN) ((!(_L1)->LowPart) && (!(_L1)->HighPart)))

NTSYSAPI
ULONG
NTAPI
RtlLengthSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
  IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
  IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValues(
  IN ULONG RelativeTo,
  IN PCWSTR Path,
  IN OUT PRTL_QUERY_REGISTRY_TABLE QueryTable,
  IN PVOID Context OPTIONAL,
  IN PVOID Environment OPTIONAL);

#define SHORT_SIZE  (sizeof(USHORT))
#define SHORT_MASK  (SHORT_SIZE - 1)
#define LONG_SIZE (sizeof(LONG))
#define LONGLONG_SIZE   (sizeof(LONGLONG))
#define LONG_MASK (LONG_SIZE - 1)
#define LONGLONG_MASK   (LONGLONG_SIZE - 1)
#define LOWBYTE_MASK 0x00FF

#define FIRSTBYTE(VALUE)  ((VALUE) & LOWBYTE_MASK)
#define SECONDBYTE(VALUE) (((VALUE) >> 8) & LOWBYTE_MASK)
#define THIRDBYTE(VALUE)  (((VALUE) >> 16) & LOWBYTE_MASK)
#define FOURTHBYTE(VALUE) (((VALUE) >> 24) & LOWBYTE_MASK)

NTSYSAPI
VOID
NTAPI
RtlSetAllBits(
  IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlSetBits(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG StartingIndex,
  IN ULONG NumberToSet);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN BOOLEAN DaclPresent,
  IN PACL Dacl OPTIONAL,
  IN BOOLEAN DaclDefaulted OPTIONAL);

#if defined(_AMD64_)

/* VOID
 * RtlStoreUlong(
 *     IN PULONG Address,
 *     IN ULONG Value);
 */
#define RtlStoreUlong(Address,Value) \
    *(ULONG UNALIGNED *)(Address) = (Value)

/* VOID
 * RtlStoreUlonglong(
 *     IN OUT PULONGLONG Address,
 *     ULONGLONG Value);
 */
#define RtlStoreUlonglong(Address,Value) \
    *(ULONGLONG UNALIGNED *)(Address) = (Value)

/* VOID
 * RtlStoreUshort(
 *     IN PUSHORT Address,
 *     IN USHORT Value);
 */
#define RtlStoreUshort(Address,Value) \
    *(USHORT UNALIGNED *)(Address) = (Value)

/* VOID
 * RtlRetrieveUshort(
 *     PUSHORT DestinationAddress,
 *    PUSHORT SourceAddress);
 */
#define RtlRetrieveUshort(DestAddress,SrcAddress) \
    *(USHORT UNALIGNED *)(DestAddress) = *(USHORT)(SrcAddress)

/* VOID
 * RtlRetrieveUlong(
 *    PULONG DestinationAddress,
 *    PULONG SourceAddress);
 */
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    *(ULONG UNALIGNED *)(DestAddress) = *(PULONG)(SrcAddress)

#else

#define RtlStoreUlong(Address,Value)                      \
    if ((ULONG_PTR)(Address) & LONG_MASK) { \
        ((PUCHAR) (Address))[LONG_LEAST_SIGNIFICANT_BIT]    = (UCHAR)(FIRSTBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_3RD_MOST_SIGNIFICANT_BIT] = (UCHAR)(SECONDBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_2ND_MOST_SIGNIFICANT_BIT] = (UCHAR)(THIRDBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_MOST_SIGNIFICANT_BIT]     = (UCHAR)(FOURTHBYTE(Value)); \
    } \
    else { \
        *((PULONG)(Address)) = (ULONG) (Value); \
    }

#define RtlStoreUlonglong(Address,Value) \
    if ((ULONG_PTR)(Address) & LONGLONG_MASK) { \
        RtlStoreUlong((ULONG_PTR)(Address), \
                      (ULONGLONG)(Value) & 0xFFFFFFFF); \
        RtlStoreUlong((ULONG_PTR)(Address)+sizeof(ULONG), \
                      (ULONGLONG)(Value) >> 32); \
    } else { \
        *((PULONGLONG)(Address)) = (ULONGLONG)(Value); \
    }

#define RtlStoreUshort(Address,Value) \
    if ((ULONG_PTR)(Address) & SHORT_MASK) { \
        ((PUCHAR) (Address))[SHORT_LEAST_SIGNIFICANT_BIT] = (UCHAR)(FIRSTBYTE(Value)); \
        ((PUCHAR) (Address))[SHORT_MOST_SIGNIFICANT_BIT ] = (UCHAR)(SECONDBYTE(Value)); \
    } \
    else { \
        *((PUSHORT) (Address)) = (USHORT)Value; \
    }

#define RtlRetrieveUshort(DestAddress,SrcAddress) \
    if ((ULONG_PTR)(SrcAddress) & LONG_MASK) \
    { \
        ((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
        ((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
    } \
    else \
    { \
        *((PUSHORT)(DestAddress))=*((PUSHORT)(SrcAddress)); \
    }

#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    if ((ULONG_PTR)(SrcAddress) & LONG_MASK) \
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

#endif /* defined(_AMD64_) */

#ifdef _WIN64
/* VOID
 * RtlStoreUlongPtr(
 *     IN OUT PULONG_PTR Address,
 *     IN ULONG_PTR Value);
 */
#define RtlStoreUlongPtr(Address,Value) RtlStoreUlonglong(Address,Value)
#else
#define RtlStoreUlongPtr(Address,Value) RtlStoreUlong(Address,Value)
#endif /* _WIN64 */

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
  IN PTIME_FIELDS TimeFields,
  IN PLARGE_INTEGER Time);

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
  IN PLARGE_INTEGER Time,
  IN PTIME_FIELDS TimeFields);

NTSYSAPI
ULONG
FASTCALL
RtlUlongByteSwap(
  IN ULONG Source);

NTSYSAPI
ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
  IN ULONGLONG Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
  IN OUT PANSI_STRING DestinationString,
  IN PCUNICODE_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(
  IN PCUNICODE_STRING UnicodeString);

#define RtlUnicodeStringToAnsiSize(String) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(String) :                     \
    ((String)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
  IN PCUNICODE_STRING String,
  IN ULONG Base OPTIONAL,
  OUT PULONG Value);

NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(
  IN WCHAR SourceCharacter);

NTSYSAPI
USHORT
FASTCALL
RtlUshortByteSwap(
  IN USHORT Source);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
  IN ULONG SecurityDescriptorLength,
  IN SECURITY_INFORMATION RequiredInformation);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
  IN ULONG RelativeTo,
  IN PCWSTR Path,
  IN PCWSTR ValueName,
  IN ULONG ValueType,
  IN PVOID ValueData,
  IN ULONG ValueLength);

$endif (_WDMDDK_)
$if (_NTDDK_)

#ifndef RTL_USE_AVL_TABLES

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable(
  OUT PRTL_GENERIC_TABLE Table,
  IN PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
  IN PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
  IN PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
  IN PVOID TableContext OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL,
  IN PVOID NodeOrParent,
  IN TABLE_SEARCH_RESULT SearchResult);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer,
  OUT PVOID *NodeOrParent,
  OUT TABLE_SEARCH_RESULT *SearchResult);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN BOOLEAN Restart);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(
  IN PRTL_GENERIC_TABLE Table,
  IN OUT PVOID *RestartKey);

NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN ULONG I);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
  IN PRTL_GENERIC_TABLE Table);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(
  IN PRTL_GENERIC_TABLE Table);

#endif /* !RTL_USE_AVL_TABLES */

#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT     8

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(
  IN OUT PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay(
  IN PRTL_SPLAY_LINKS Links,
  IN OUT PRTL_SPLAY_LINKS *Root);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
  IN PCUNICODE_STRING  String1,
  IN PCUNICODE_STRING  String2,
  IN BOOLEAN  CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlUpperString(
  IN OUT PSTRING  DestinationString,
  IN const PSTRING  SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCUNICODE_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
  IN OUT PACCESS_MASK AccessMask,
  IN PGENERIC_MAPPING GenericMapping);

NTSYSAPI
NTSTATUS
NTAPI
RtlVolumeDeviceToDosName(
  IN PVOID VolumeDeviceObject,
  OUT PUNICODE_STRING DosName);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
  IN OUT PRTL_OSVERSIONINFOW lpVersionInformation);

NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
  IN PRTL_OSVERSIONINFOEXW VersionInfo,
  IN ULONG TypeMask,
  IN ULONGLONG ConditionMask);

NTSYSAPI
LONG
NTAPI
RtlCompareString(
  IN const PSTRING String1,
  IN const PSTRING String2,
  IN BOOLEAN CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlCopyString(
  OUT PSTRING DestinationString,
  IN const PSTRING SourceString OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
  IN const PSTRING String1,
  IN const PSTRING String2,
  IN BOOLEAN CaseInSensitive);

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
  IN PCSZ String,
  IN ULONG Base OPTIONAL,
  OUT PULONG Value);

NTSYSAPI
CHAR
NTAPI
RtlUpperChar(
  IN CHAR Character);

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
  OUT PVOID *Callers,
  IN ULONG Count,
  IN ULONG Flags);

$endif (_NTDDK_)
$if (_NTIFS_)

NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
  IN HANDLE HeapHandle,
  IN ULONG Flags OPTIONAL,
  IN SIZE_T Size);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
  IN PVOID HeapHandle,
  IN ULONG Flags OPTIONAL,
  IN PVOID BaseAddress);

NTSYSAPI
VOID
NTAPI
RtlCaptureContext(
  OUT PCONTEXT ContextRecord);

NTSYSAPI
ULONG
NTAPI
RtlRandom(
  IN OUT PULONG Seed);

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
  OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendStringToString(
  IN OUT PSTRING Destination,
  IN const STRING *Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCOEM_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
  IN OUT POEM_STRING DestinationString,
  IN PCUNICODE_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString(
  IN OUT POEM_STRING DestinationString,
  IN PCUNICODE_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToCountedUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCOEM_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToCountedOemString(
  IN OUT POEM_STRING DestinationString,
  IN PCUNICODE_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
  IN OUT POEM_STRING DestinationString,
  IN PCUNICODE_STRING SourceString,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
  IN OUT PUNICODE_STRING UniDest,
  IN PCUNICODE_STRING UniSource,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
VOID
NTAPI
RtlFreeOemString (
  IN OUT POEM_STRING OemString);

NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToOemSize(
  IN PCUNICODE_STRING UnicodeString);

NTSYSAPI
ULONG
NTAPI
RtlxOemStringToUnicodeSize(
  IN PCOEM_STRING OemString);

NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
  OUT PWCH UnicodeString,
  IN ULONG MaxBytesInUnicodeString,
  OUT PULONG BytesInUnicodeString OPTIONAL,
  IN const CHAR *MultiByteString,
  IN ULONG BytesInMultiByteString);

NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
  OUT PULONG BytesInUnicodeString,
  IN const CHAR *MultiByteString,
  IN ULONG BytesInMultiByteString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
  OUT PULONG BytesInMultiByteString,
  IN PCWCH UnicodeString,
  IN ULONG BytesInUnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
  OUT PCHAR MultiByteString,
  IN ULONG MaxBytesInMultiByteString,
  OUT PULONG BytesInMultiByteString OPTIONAL,
  IN PWCH UnicodeString,
  IN ULONG BytesInUnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToMultiByteN(
  OUT PCHAR MultiByteString,
  IN ULONG MaxBytesInMultiByteString,
  OUT PULONG BytesInMultiByteString OPTIONAL,
  IN PCWCH UnicodeString,
  IN ULONG BytesInUnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlOemToUnicodeN(
  OUT PWSTR UnicodeString,
  IN ULONG MaxBytesInUnicodeString,
  OUT PULONG BytesInUnicodeString OPTIONAL,
  IN PCCH OemString,
  IN ULONG BytesInOemString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToOemN(
  OUT PCHAR OemString,
  IN ULONG MaxBytesInOemString,
  OUT PULONG BytesInOemString OPTIONAL,
  IN PCWCH UnicodeString,
  IN ULONG BytesInUnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToOemN(
  OUT PCHAR OemString,
  IN ULONG MaxBytesInOemString,
  OUT PULONG BytesInOemString OPTIONAL,
  IN PCWCH UnicodeString,
  IN ULONG BytesInUnicodeString);

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTSYSAPI
NTSTATUS
NTAPI
RtlGenerate8dot3Name(
  IN PCUNICODE_STRING Name,
  IN BOOLEAN AllowExtendedCharacters,
  IN OUT PGENERATE_NAME_CONTEXT Context,
  IN OUT PUNICODE_STRING Name8dot3);
#else
NTSYSAPI
VOID
NTAPI
RtlGenerate8dot3Name(
  IN PCUNICODE_STRING Name,
  IN BOOLEAN AllowExtendedCharacters,
  IN OUT PGENERATE_NAME_CONTEXT Context,
  IN OUT PUNICODE_STRING Name8dot3);
#endif

NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(
  IN PCUNICODE_STRING Name,
  IN OUT POEM_STRING OemName OPTIONAL,
  IN OUT PBOOLEAN NameContainsSpaces OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidOemCharacter(
  IN OUT PWCHAR Char);

NTSYSAPI
VOID
NTAPI
PfxInitialize(
  OUT PPREFIX_TABLE PrefixTable);

NTSYSAPI
BOOLEAN
NTAPI
PfxInsertPrefix(
  IN PPREFIX_TABLE PrefixTable,
  IN PSTRING Prefix,
  OUT PPREFIX_TABLE_ENTRY PrefixTableEntry);

NTSYSAPI
VOID
NTAPI
PfxRemovePrefix(
  IN PPREFIX_TABLE PrefixTable,
  IN PPREFIX_TABLE_ENTRY PrefixTableEntry);

NTSYSAPI
PPREFIX_TABLE_ENTRY
NTAPI
PfxFindPrefix(
  IN PPREFIX_TABLE PrefixTable,
  IN PSTRING FullName);

NTSYSAPI
VOID
NTAPI
RtlInitializeUnicodePrefix(
  OUT PUNICODE_PREFIX_TABLE PrefixTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlInsertUnicodePrefix(
  IN PUNICODE_PREFIX_TABLE PrefixTable,
  IN PUNICODE_STRING Prefix,
  OUT PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry);

NTSYSAPI
VOID
NTAPI
RtlRemoveUnicodePrefix(
  IN PUNICODE_PREFIX_TABLE PrefixTable,
  IN PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry);

NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix(
  IN PUNICODE_PREFIX_TABLE PrefixTable,
  IN PUNICODE_STRING FullName,
  IN ULONG CaseInsensitiveIndex);

NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlNextUnicodePrefix(
  IN PUNICODE_PREFIX_TABLE PrefixTable,
  IN BOOLEAN Restart);

NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong(
  IN PVOID Source,
  IN SIZE_T Length,
  IN ULONG Pattern);

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1980(
  IN PLARGE_INTEGER Time,
  OUT PULONG ElapsedSeconds);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1980ToTime(
  IN ULONG ElapsedSeconds,
  OUT PLARGE_INTEGER Time);

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
  IN PLARGE_INTEGER Time,
  OUT PULONG ElapsedSeconds);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
  IN ULONG ElapsedSeconds,
  OUT PLARGE_INTEGER Time);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(
  IN PSID Sid);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid(
  IN PSID Sid1,
  IN PSID Sid2);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualPrefixSid(
  IN PSID Sid1,
  IN PSID Sid2);

NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(
  IN ULONG SubAuthorityCount);

NTSYSAPI
PVOID
NTAPI
RtlFreeSid(
  IN PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateAndInitializeSid(
  IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
  IN UCHAR SubAuthorityCount,
  IN ULONG SubAuthority0,
  IN ULONG SubAuthority1,
  IN ULONG SubAuthority2,
  IN ULONG SubAuthority3,
  IN ULONG SubAuthority4,
  IN ULONG SubAuthority5,
  IN ULONG SubAuthority6,
  IN ULONG SubAuthority7,
  OUT PSID *Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
  OUT PSID Sid,
  IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
  IN UCHAR SubAuthorityCount);

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
  IN PSID Sid,
  IN ULONG SubAuthority);

NTSYSAPI
ULONG
NTAPI
RtlLengthSid(
  IN PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
  IN ULONG Length,
  IN PSID Destination,
  IN PSID Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
  IN OUT PUNICODE_STRING UnicodeString,
  IN PSID Sid,
  IN BOOLEAN AllocateDestinationString);

NTSYSAPI
VOID
NTAPI
RtlCopyLuid(
  OUT PLUID DestinationLuid,
  IN PLUID SourceLuid);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
  OUT PACL Acl,
  IN ULONG AclLength,
  IN ULONG AclRevision);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce(
  IN OUT PACL Acl,
  IN ULONG AceRevision,
  IN ULONG StartingAceIndex,
  IN PVOID AceList,
  IN ULONG AceListLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
  IN OUT PACL Acl,
  IN ULONG AceIndex);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
  IN PACL Acl,
  IN ULONG AceIndex,
  OUT PVOID *Ace);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
  IN OUT PACL Acl,
  IN ULONG AceRevision,
  IN ACCESS_MASK AccessMask,
  IN PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAceEx(
  IN OUT PACL Acl,
  IN ULONG AceRevision,
  IN ULONG AceFlags,
  IN ACCESS_MASK AccessMask,
  IN PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptorRelative(
  OUT PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
  IN ULONG Revision);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  OUT PBOOLEAN DaclPresent,
  OUT PACL *Dacl,
  OUT PBOOLEAN DaclDefaulted);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSID Owner OPTIONAL,
  IN BOOLEAN OwnerDefaulted);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  OUT PSID *Owner,
  OUT PBOOLEAN OwnerDefaulted);

NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
  IN NTSTATUS Status);

NTSYSAPI
NTSTATUS
NTAPI
RtlCustomCPToUnicodeN(
  IN PCPTABLEINFO CustomCP,
  OUT PWCH UnicodeString,
  IN ULONG MaxBytesInUnicodeString,
  OUT PULONG BytesInUnicodeString OPTIONAL,
  IN PCH CustomCPString,
  IN ULONG BytesInCustomCPString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToCustomCPN(
  IN PCPTABLEINFO CustomCP,
  OUT PCH CustomCPString,
  IN ULONG MaxBytesInCustomCPString,
  OUT PULONG BytesInCustomCPString OPTIONAL,
  IN PWCH UnicodeString,
  IN ULONG BytesInUnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToCustomCPN(
  IN PCPTABLEINFO CustomCP,
  OUT PCH CustomCPString,
  IN ULONG MaxBytesInCustomCPString,
  OUT PULONG BytesInCustomCPString OPTIONAL,
  IN PWCH UnicodeString,
  IN ULONG BytesInUnicodeString);

NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
  IN PUSHORT TableBase,
  IN OUT PCPTABLEINFO CodePageTable);

$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTSYSAPI
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(
  IN PVOID Source,
  IN SIZE_T Length);
#endif

$endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

$if (_WDMDDK_)

NTSYSAPI
VOID
NTAPI
RtlClearBit(
  PRTL_BITMAP BitMapHeader,
  ULONG BitNumber);

NTSYSAPI
WCHAR
NTAPI
RtlDowncaseUnicodeChar(
  IN WCHAR SourceCharacter);

NTSYSAPI
VOID
NTAPI
RtlSetBit(
  PRTL_BITMAP BitMapHeader,
  ULONG BitNumber);

NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG BitNumber);

NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
  IN CONST UNICODE_STRING *String,
  IN BOOLEAN CaseInSensitive,
  IN ULONG HashAlgorithm,
  OUT PULONG HashValue);

$endif (_WDMDDK_)

$if (_NTDDK_)

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl(
  OUT PRTL_AVL_TABLE Table,
  IN PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
  IN PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
  IN PRTL_AVL_FREE_ROUTINE FreeRoutine,
  IN PVOID TableContext OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL,
  IN PVOID NodeOrParent,
  IN TABLE_SEARCH_RESULT SearchResult);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  OUT PVOID *NodeOrParent,
  OUT TABLE_SEARCH_RESULT *SearchResult);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN BOOLEAN Restart);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(
  IN PRTL_AVL_TABLE Table,
  IN OUT PVOID *RestartKey);

NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  OUT PVOID *RestartKey);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory(
  IN PRTL_AVL_TABLE Table,
  IN PRTL_AVL_MATCH_FUNCTION MatchFunction OPTIONAL,
  IN PVOID MatchData OPTIONAL,
  IN ULONG NextFlag,
  IN OUT PVOID *RestartKey,
  IN OUT PULONG DeleteCount,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN ULONG I);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(
  IN PRTL_AVL_TABLE Table);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(
  IN PRTL_AVL_TABLE Table);

$endif (_NTDDK_)
$if (_NTIFS_)

NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
  IN ULONG Flags,
  IN PVOID HeapBase OPTIONAL,
  IN SIZE_T ReserveSize OPTIONAL,
  IN SIZE_T CommitSize OPTIONAL,
  IN PVOID Lock OPTIONAL,
  IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
  IN PVOID HeapHandle);

NTSYSAPI
USHORT
NTAPI
RtlCaptureStackBackTrace(
  IN ULONG FramesToSkip,
  IN ULONG FramesToCapture,
  OUT PVOID *BackTrace,
  OUT PULONG BackTraceHash OPTIONAL);

NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
  IN OUT PULONG Seed);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
  OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlValidateUnicodeString(
  IN ULONG Flags,
  IN PCUNICODE_STRING String);

NTSYSAPI
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
  IN ULONG Flags,
  IN PCUNICODE_STRING SourceString,
  OUT PUNICODE_STRING DestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
  IN USHORT CompressionFormatAndEngine,
  OUT PULONG CompressBufferWorkSpaceSize,
  OUT PULONG CompressFragmentWorkSpaceSize);

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressBuffer(
  IN USHORT CompressionFormatAndEngine,
  IN PUCHAR UncompressedBuffer,
  IN ULONG UncompressedBufferSize,
  OUT PUCHAR CompressedBuffer,
  IN ULONG CompressedBufferSize,
  IN ULONG UncompressedChunkSize,
  OUT PULONG FinalCompressedSize,
  IN PVOID WorkSpace);

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBuffer(
  IN USHORT CompressionFormat,
  OUT PUCHAR UncompressedBuffer,
  IN ULONG UncompressedBufferSize,
  IN PUCHAR CompressedBuffer,
  IN ULONG CompressedBufferSize,
  OUT PULONG FinalUncompressedSize);

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressFragment(
  IN USHORT CompressionFormat,
  OUT PUCHAR UncompressedFragment,
  IN ULONG UncompressedFragmentSize,
  IN PUCHAR CompressedBuffer,
  IN ULONG CompressedBufferSize,
  IN ULONG FragmentOffset,
  OUT PULONG FinalUncompressedSize,
  IN PVOID WorkSpace);

NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk(
  IN USHORT CompressionFormat,
  IN OUT PUCHAR *CompressedBuffer,
  IN PUCHAR EndOfCompressedBufferPlus1,
  OUT PUCHAR *ChunkBuffer,
  OUT PULONG ChunkSize);

NTSYSAPI
NTSTATUS
NTAPI
RtlReserveChunk(
  IN USHORT CompressionFormat,
  IN OUT PUCHAR *CompressedBuffer,
  IN PUCHAR EndOfCompressedBufferPlus1,
  OUT PUCHAR *ChunkBuffer,
  IN ULONG ChunkSize);

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressChunks(
  OUT PUCHAR UncompressedBuffer,
  IN ULONG UncompressedBufferSize,
  IN PUCHAR CompressedBuffer,
  IN ULONG CompressedBufferSize,
  IN PUCHAR CompressedTail,
  IN ULONG CompressedTailSize,
  IN PCOMPRESSED_DATA_INFO CompressedDataInfo);

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressChunks(
  IN PUCHAR UncompressedBuffer,
  IN ULONG UncompressedBufferSize,
  OUT PUCHAR CompressedBuffer,
  IN ULONG CompressedBufferSize,
  IN OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
  IN ULONG CompressedDataInfoLength,
  IN PVOID WorkSpace);

NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(
  IN PSID Sid);

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
  IN PSID Sid);

NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
  IN NTSTATUS Status);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSystemVolumeInformationFolder(
  IN PCUNICODE_STRING VolumeRootPath);

#if defined(_M_AMD64)

FORCEINLINE
VOID
RtlFillMemoryUlong (
  OUT PVOID Destination,
  IN SIZE_T Length,
  IN ULONG Pattern)
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
  OUT PVOID Destination,
  IN SIZE_T Length,
  IN ULONG Pattern);

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlonglong(
  OUT PVOID Destination,
  IN SIZE_T Length,
  IN ULONGLONG Pattern);

#endif /* defined(_M_AMD64) */
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

$if (_NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WS03)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
  OUT PANSI_STRING DestinationString,
  IN PCSZ SourceString OPTIONAL);
#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  OUT PBOOLEAN SaclPresent,
  OUT PACL *Sacl,
  OUT PBOOLEAN SaclDefaulted);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSID Group OPTIONAL,
  IN BOOLEAN GroupDefaulted OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  OUT PSID *Group,
  OUT PBOOLEAN GroupDefaulted);

NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
  IN PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
  OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor OPTIONAL,
  IN OUT PULONG BufferLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
  IN PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
  OUT PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor OPTIONAL,
  IN OUT PULONG AbsoluteSecurityDescriptorSize,
  OUT PACL Dacl OPTIONAL,
  IN OUT PULONG DaclSize,
  OUT PACL Sacl OPTIONAL,
  IN OUT PULONG SaclSize,
  OUT PSID Owner OPTIONAL,
  IN OUT PULONG OwnerSize,
  OUT PSID PrimaryGroup OPTIONAL,
  IN OUT PULONG PrimaryGroupSize);

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */
$endif (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_VISTA)

$if (_WDMDDK_)
NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBitsUlongPtr(
  IN ULONG_PTR Target);

NTSYSAPI
ULONGLONG
NTAPI
RtlIoDecodeMemIoResource(
  IN struct _IO_RESOURCE_DESCRIPTOR *Descriptor,
  OUT PULONGLONG Alignment OPTIONAL,
  OUT PULONGLONG MinimumAddress OPTIONAL,
  OUT PULONGLONG MaximumAddress OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlIoEncodeMemIoResource(
  IN struct _IO_RESOURCE_DESCRIPTOR *Descriptor,
  IN UCHAR Type,
  IN ULONGLONG Length,
  IN ULONGLONG Alignment,
  IN ULONGLONG MinimumAddress,
  IN ULONGLONG MaximumAddress);

NTSYSAPI
ULONGLONG
NTAPI
RtlCmDecodeMemIoResource(
  IN struct _CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor,
  OUT PULONGLONG Start OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlFindClosestEncodableLength(
  IN ULONGLONG SourceLength,
  OUT PULONGLONG TargetLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlCmEncodeMemIoResource(
  IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
  IN UCHAR Type,
  IN ULONGLONG Length,
  IN ULONGLONG Start);

$endif (_WDMDDK_)
$if (_NTDDK_)

NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
  OUT PRTL_RUN_ONCE RunOnce);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN PRTL_RUN_ONCE_INIT_FN InitFn,
  IN OUT PVOID Parameter OPTIONAL,
  OUT PVOID *Context OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN ULONG Flags,
  OUT PVOID *Context OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN ULONG Flags,
  IN PVOID Context OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
  IN ULONG OSMajorVersion,
  IN ULONG OSMinorVersion,
  IN ULONG SpMajorVersion,
  IN ULONG SpMinorVersion,
  OUT PULONG ReturnedProductType);

$endif (_NTDDK_)
$if (_NTIFS_)
NTSYSAPI
NTSTATUS
NTAPI
RtlNormalizeString(
  IN ULONG NormForm,
  IN PCWSTR SourceString,
  IN LONG SourceStringLength,
  OUT PWSTR DestinationString,
  IN OUT PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlIsNormalizedString(
  IN ULONG NormForm,
  IN PCWSTR SourceString,
  IN LONG SourceStringLength,
  OUT PBOOLEAN Normalized);

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToAscii(
  IN ULONG Flags,
  IN PCWSTR SourceString,
  IN LONG SourceStringLength,
  OUT PWSTR DestinationString,
  IN OUT PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToUnicode(
  IN ULONG Flags,
  IN PCWSTR SourceString,
  IN LONG SourceStringLength,
  OUT PWSTR DestinationString,
  IN OUT PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToNameprepUnicode(
  IN ULONG Flags,
  IN PCWSTR SourceString,
  IN LONG SourceStringLength,
  OUT PWSTR DestinationString,
  IN OUT PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateServiceSid(
  IN PUNICODE_STRING ServiceName,
  OUT PSID ServiceSid,
  IN OUT PULONG ServiceSidLength);

NTSYSAPI
LONG
NTAPI
RtlCompareAltitudes(
  IN PCUNICODE_STRING Altitude1,
  IN PCUNICODE_STRING Altitude2);

$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

$if (_WDMDDK_)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToUTF8N(
  OUT PCHAR UTF8StringDestination,
  IN ULONG UTF8StringMaxByteCount,
  OUT PULONG UTF8StringActualByteCount,
  IN PCWCH UnicodeStringSource,
  IN ULONG UnicodeStringByteCount);

NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8ToUnicodeN(
  OUT PWSTR UnicodeStringDestination,
  IN ULONG UnicodeStringMaxByteCount,
  OUT PULONG UnicodeStringActualByteCount,
  IN PCCH UTF8StringSource,
  IN ULONG UTF8StringByteCount);

NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedFeatures(
  IN ULONG64 FeatureMask);

$endif (_WDMDDK_)
$if (_NTDDK_)

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTable(
  IN OUT PRTL_DYNAMIC_HASH_TABLE *HashTable OPTIONAL,
  IN ULONG Shift,
  IN ULONG Flags);

NTSYSAPI
VOID
NTAPI
RtlDeleteHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlInsertEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
  IN ULONG_PTR Signature,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlRemoveEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context OPTIONAL);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlLookupEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN ULONG_PTR Signature,
  OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context OPTIONAL);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlGetNextEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context);

NTSYSAPI
BOOLEAN
NTAPI
RtlInitEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlEnumerateEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
VOID
NTAPI
RtlEndEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
BOOLEAN
NTAPI
RtlInitWeakEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlWeaklyEnumerateEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
VOID
NTAPI
RtlEndWeakEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
BOOLEAN
NTAPI
RtlExpandHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlContractHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable);

$endif (_NTDDK_)
$if (_NTIFS_)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToUTF8N(
  OUT PCHAR UTF8StringDestination,
  IN ULONG UTF8StringMaxByteCount,
  OUT PULONG UTF8StringActualByteCount,
  IN PCWCH UnicodeStringSource,
  IN ULONG UnicodeStringByteCount);

NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8ToUnicodeN(
  OUT PWSTR UnicodeStringDestination,
  IN ULONG UnicodeStringMaxByteCount,
  OUT PULONG UnicodeStringActualByteCount,
  IN PCCH UTF8StringSource,
  IN ULONG UTF8StringByteCount);

NTSYSAPI
NTSTATUS
NTAPI
RtlReplaceSidInSd(
  IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSID OldSid,
  IN PSID NewSid,
  OUT ULONG *NumChanges);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateVirtualAccountSid(
  IN PCUNICODE_STRING Name,
  IN ULONG BaseSubAuthority,
  OUT PSID Sid,
  IN OUT PULONG SidLength);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

$if (_WDMDDK_)

#if !defined(MIDL_PASS)
/* inline funftions */
//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertLongToLargeInteger(
  IN LONG SignedInteger)
{
  LARGE_INTEGER ret;
  ret.QuadPart = SignedInteger;
  return ret;
}

//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertUlongToLargeInteger(
  IN ULONG UnsignedInteger)
{
  LARGE_INTEGER ret;
  ret.QuadPart = UnsignedInteger;
  return ret;
}

//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerShiftLeft(
  IN LARGE_INTEGER LargeInteger,
  IN CCHAR ShiftCount)
{
  LARGE_INTEGER Result;

  Result.QuadPart = LargeInteger.QuadPart << ShiftCount;
  return Result;
}

//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerShiftRight(
  IN LARGE_INTEGER LargeInteger,
  IN CCHAR ShiftCount)
{
  LARGE_INTEGER Result;

  Result.QuadPart = (ULONG64)LargeInteger.QuadPart >> ShiftCount;
  return Result;
}

//DECLSPEC_DEPRECATED_DDK
static __inline
ULONG
NTAPI_INLINE
RtlEnlargedUnsignedDivide(
  IN ULARGE_INTEGER Dividend,
  IN ULONG Divisor,
  IN OUT PULONG Remainder)
{
  if (Remainder)
    *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
  return (ULONG)(Dividend.QuadPart / Divisor);
}

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerNegate(
  IN LARGE_INTEGER Subtrahend)
{
  LARGE_INTEGER Difference;

  Difference.QuadPart = -Subtrahend.QuadPart;
  return Difference;
}

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerSubtract(
  IN LARGE_INTEGER Minuend,
  IN LARGE_INTEGER Subtrahend)
{
  LARGE_INTEGER Difference;

  Difference.QuadPart = Minuend.QuadPart - Subtrahend.QuadPart;
  return Difference;
}

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedUnsignedMultiply(
  IN ULONG Multiplicand,
  IN ULONG Multiplier)
{
  LARGE_INTEGER ret;
  ret.QuadPart = (ULONGLONG)Multiplicand * (ULONGLONG)Multiplier;
  return ret;
}

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedIntegerMultiply(
  IN LONG Multiplicand,
  IN LONG Multiplier)
{
  LARGE_INTEGER ret;
  ret.QuadPart = (LONGLONG)Multiplicand * (ULONGLONG)Multiplier;
  return ret;
}

FORCEINLINE
VOID
RtlInitEmptyAnsiString(
  OUT PANSI_STRING AnsiString,
  IN PCHAR Buffer,
  IN USHORT BufferSize)
{
  AnsiString->Length = 0;
  AnsiString->MaximumLength = BufferSize;
  AnsiString->Buffer = Buffer;
}

FORCEINLINE
VOID
RtlInitEmptyUnicodeString(
  OUT PUNICODE_STRING UnicodeString,
  IN PWSTR Buffer,
  IN USHORT BufferSize)
{
  UnicodeString->Length = 0;
  UnicodeString->MaximumLength = BufferSize;
  UnicodeString->Buffer = Buffer;
}
$endif (_WDMDDK_)

#if defined(_AMD64_) || defined(_IA64_)

$if (_WDMDDK_)

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedIntegerMultiply(
  IN LARGE_INTEGER Multiplicand,
  IN LONG Multiplier)
{
  LARGE_INTEGER ret;
  ret.QuadPart = Multiplicand.QuadPart * Multiplier;
  return ret;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN ULONG Divisor,
  OUT PULONG Remainder OPTIONAL)
{
  LARGE_INTEGER ret;
  ret.QuadPart = (ULONG64)Dividend.QuadPart / Divisor;
  if (Remainder)
    *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
  return ret;
}

$endif (_WDMDDK_)

$if (_NTDDK_)

//DECLSPEC_DEPRECATED_DDK_WINXP
FORCEINLINE
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN LARGE_INTEGER Divisor,
  OUT PLARGE_INTEGER Remainder OPTIONAL)
{
  LARGE_INTEGER ret;
  ret.QuadPart = Dividend.QuadPart / Divisor.QuadPart;
  if (Remainder)
    Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;
  return ret;
}

#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN LARGE_INTEGER Divisor,
  OUT PLARGE_INTEGER Remainder OPTIONAL);
#endif

$endif (_NTDDK_)

#endif /* defined(_AMD64_) || defined(_IA64_) */

$if (_WDMDDK_)

#if defined(_AMD64_)

#define MultiplyHigh __mulh
#define UnsignedMultiplyHigh __umulh

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedMagicDivide(
  IN LARGE_INTEGER Dividend,
  IN LARGE_INTEGER MagicDivisor,
  IN CCHAR ShiftCount)
{
  LARGE_INTEGER ret;
  ULONG64 ret64;
  BOOLEAN Pos;
  Pos = (Dividend.QuadPart >= 0);
  ret64 = UnsignedMultiplyHigh(Pos ? Dividend.QuadPart : -Dividend.QuadPart,
                               MagicDivisor.QuadPart);
  ret64 >>= ShiftCount;
  ret.QuadPart = Pos ? ret64 : -(LONG64)ret64;
  return ret;
}
#endif

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerAdd(
  IN LARGE_INTEGER Addend1,
  IN LARGE_INTEGER Addend2)
{
  LARGE_INTEGER ret;
  ret.QuadPart = Addend1.QuadPart + Addend2.QuadPart;
  return ret;
}

/* VOID
 * RtlLargeIntegerAnd(
 *     IN OUT LARGE_INTEGER Result,
 *     IN LARGE_INTEGER Source,
 *     IN LARGE_INTEGER Mask);
 */
#define RtlLargeIntegerAnd(Result, Source, Mask) \
    Result.QuadPart = Source.QuadPart & Mask.QuadPart

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerArithmeticShift(
  IN LARGE_INTEGER LargeInteger,
  IN CCHAR ShiftCount)
{
  LARGE_INTEGER ret;
  ret.QuadPart = LargeInteger.QuadPart >> ShiftCount;
  return ret;
}

/* BOOLEAN
 * RtlLargeIntegerEqualTo(
 *     IN LARGE_INTEGER  Operand1,
 *     IN LARGE_INTEGER  Operand2);
 */
#define RtlLargeIntegerEqualTo(X,Y) \
    (!(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))

FORCEINLINE
PVOID
RtlSecureZeroMemory(
  OUT PVOID Pointer,
  IN SIZE_T Size)
{
  volatile char* vptr = (volatile char*)Pointer;
#if defined(_M_AMD64)
  __stosb((PUCHAR)vptr, 0, Size);
#else
  char * endptr = (char *)vptr + Size;
  while (vptr < endptr) {
    *vptr = 0; vptr++;
  }
#endif
   return Pointer;
}

#if defined(_M_AMD64)
FORCEINLINE
BOOLEAN
RtlCheckBit(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG BitPosition)
{
  return BitTest64((LONG64 CONST*)BitMapHeader->Buffer, (LONG64)BitPosition);
}
#else
#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP)/32]) >> ((BP)%32)) & 0x1)
#endif /* defined(_M_AMD64) */

#define RtlLargeIntegerGreaterThan(X,Y) (                              \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart > (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                      \
)

#define RtlLargeIntegerGreaterThanOrEqualTo(X,Y) (                      \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart >= (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                       \
)

#define RtlLargeIntegerNotEqualTo(X,Y) (                          \
    (((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)) \
)

#define RtlLargeIntegerLessThan(X,Y) (                                 \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart < (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                      \
)

#define RtlLargeIntegerLessThanOrEqualTo(X,Y) (                         \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart <= (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                       \
)

#define RtlLargeIntegerGreaterThanZero(X) (       \
    (((X).HighPart == 0) && ((X).LowPart > 0)) || \
    ((X).HighPart > 0 )                           \
)

#define RtlLargeIntegerGreaterOrEqualToZero(X) ( (X).HighPart >= 0 )

#define RtlLargeIntegerEqualToZero(X) ( !((X).LowPart | (X).HighPart) )

#define RtlLargeIntegerNotEqualToZero(X) ( ((X).LowPart | (X).HighPart) )

#define RtlLargeIntegerLessThanZero(X) ( ((X).HighPart < 0) )

#define RtlLargeIntegerLessOrEqualToZero(X) ( ((X).HighPart < 0) || !((X).LowPart | (X).HighPart) )

#endif /* !defined(MIDL_PASS) */

/* Byte Swap Functions */
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037 || defined(__GNUC__))) || \
    ((defined(_M_AMD64) || defined(_M_IA64)) \
        && (_MSC_FULL_VER > 13009175 || defined(__GNUC__)))

#define RtlUshortByteSwap(_x) _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))

#endif

#if DBG

#define ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

#define ASSERTMSG(msg, exp) \
  (VOID)((!(exp)) ? \
    RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, (PCHAR)msg ), FALSE : TRUE)

#define RTL_SOFT_ASSERT(exp) \
  (VOID)((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #exp), FALSE : TRUE)

#define RTL_SOFT_ASSERTMSG(msg, exp) \
  (VOID)((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #exp, (msg)), FALSE : TRUE)

#define RTL_VERIFY(exp) ASSERT(exp)
#define RTL_VERIFYMSG(msg, exp) ASSERTMSG(msg, exp)

#define RTL_SOFT_VERIFY(exp) RTL_SOFT_ASSERT(exp)
#define RTL_SOFT_VERIFYMSG(msg, exp) RTL_SOFT_ASSERTMSG(msg, exp)

#if defined(_MSC_VER)

#define NT_ASSERT(exp) \
   ((!(exp)) ? \
      (__annotation(L"Debug", L"AssertFail", L#exp), \
       DbgRaiseAssertionFailure(), FALSE) : TRUE)

#define NT_ASSERTMSG(msg, exp) \
   ((!(exp)) ? \
      (__annotation(L"Debug", L"AssertFail", L##msg), \
      DbgRaiseAssertionFailure(), FALSE) : TRUE)

#define NT_ASSERTMSGW(msg, exp) \
    ((!(exp)) ? \
        (__annotation(L"Debug", L"AssertFail", msg), \
         DbgRaiseAssertionFailure(), FALSE) : TRUE)

#define NT_VERIFY     NT_ASSERT
#define NT_VERIFYMSG  NT_ASSERTMSG
#define NT_VERIFYMSGW NT_ASSERTMSGW

#else

/* GCC doesn't support __annotation (nor PDB) */
#define NT_ASSERT(exp) \
   (VOID)((!(exp)) ? (DbgRaiseAssertionFailure(), FALSE) : TRUE)

#define NT_ASSERTMSG NT_ASSERT
#define NT_ASSERTMSGW NT_ASSERT

#endif

#else /* !DBG */

#define ASSERT(exp) ((VOID) 0)
#define ASSERTMSG(msg, exp) ((VOID) 0)

#define RTL_SOFT_ASSERT(exp) ((VOID) 0)
#define RTL_SOFT_ASSERTMSG(msg, exp) ((VOID) 0)

#define RTL_VERIFY(exp) ((exp) ? TRUE : FALSE)
#define RTL_VERIFYMSG(msg, exp) ((exp) ? TRUE : FALSE)

#define RTL_SOFT_VERIFY(exp) ((exp) ? TRUE : FALSE)
#define RTL_SOFT_VERIFYMSG(msg, exp) ((exp) ? TRUE : FALSE)

#define NT_ASSERT(exp)          ((VOID)0)
#define NT_ASSERTMSG(msg, exp)  ((VOID)0)
#define NT_ASSERTMSGW(msg, exp) ((VOID)0)

#define NT_VERIFY(_exp)           ((_exp) ? TRUE : FALSE)
#define NT_VERIFYMSG(_msg, _exp ) ((_exp) ? TRUE : FALSE)
#define NT_VERIFYMSGW(_msg, _exp) ((_exp) ? TRUE : FALSE)

#endif /* DBG */

#define InitializeListHead32(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))

#if !defined(_WINBASE_)

#if defined(_WIN64) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_))

NTKERNELAPI
VOID
InitializeSListHead(
  OUT PSLIST_HEADER SListHead);

#else

FORCEINLINE
VOID
InitializeSListHead(
  OUT PSLIST_HEADER SListHead)
{
#if defined(_IA64_)
  ULONG64 FeatureBits;
#endif

#if defined(_WIN64)
  if (((ULONG_PTR)SListHead & 0xf) != 0) {
    RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
  }
#endif
  RtlZeroMemory(SListHead, sizeof(SLIST_HEADER));
#if defined(_IA64_)
  FeatureBits = __getReg(CV_IA64_CPUID4);
  if ((FeatureBits & KF_16BYTE_INSTR) != 0) {
    SListHead->Header16.HeaderType = 1;
    SListHead->Header16.Init = 1;
  }
#endif
}

#endif

#if defined(_WIN64)

#define InterlockedPopEntrySList(Head) \
    ExpInterlockedPopEntrySList(Head)

#define InterlockedPushEntrySList(Head, Entry) \
    ExpInterlockedPushEntrySList(Head, Entry)

#define InterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#else /* !defined(_WIN64) */

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList(
  IN PSLIST_HEADER ListHead);

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(
  IN PSLIST_HEADER ListHead,
  IN PSLIST_ENTRY ListEntry);

#define InterlockedFlushSList(ListHead) \
    ExInterlockedFlushSList(ListHead)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#endif /* !defined(_WIN64) */

#endif /* !defined(_WINBASE_) */

#define RTL_CONTEXT_EX_OFFSET(ContextEx, Chunk) ((ContextEx)->Chunk.Offset)
#define RTL_CONTEXT_EX_LENGTH(ContextEx, Chunk) ((ContextEx)->Chunk.Length)
#define RTL_CONTEXT_EX_CHUNK(Base, Layout, Chunk)       \
    ((PVOID)((PCHAR)(Base) + RTL_CONTEXT_EX_OFFSET(Layout, Chunk)))
#define RTL_CONTEXT_OFFSET(Context, Chunk)              \
    RTL_CONTEXT_EX_OFFSET((PCONTEXT_EX)(Context + 1), Chunk)
#define RTL_CONTEXT_LENGTH(Context, Chunk)              \
    RTL_CONTEXT_EX_LENGTH((PCONTEXT_EX)(Context + 1), Chunk)
#define RTL_CONTEXT_CHUNK(Context, Chunk)               \
    RTL_CONTEXT_EX_CHUNK((PCONTEXT_EX)(Context + 1),    \
                         (PCONTEXT_EX)(Context + 1),    \
                         Chunk)

BOOLEAN
RTLVERLIB_DDI(RtlIsNtDdiVersionAvailable)(
  IN ULONG Version);

BOOLEAN
RTLVERLIB_DDI(RtlIsServicePackVersionInstalled)(
  IN ULONG Version);

#ifndef RtlIsNtDdiVersionAvailable
#define RtlIsNtDdiVersionAvailable WdmlibRtlIsNtDdiVersionAvailable
#endif

#ifndef RtlIsServicePackVersionInstalled
#define RtlIsServicePackVersionInstalled WdmlibRtlIsServicePackVersionInstalled
#endif

#define RtlInterlockedSetBits(Flags, Flag) \
    InterlockedOr((PLONG)(Flags), Flag)

#define RtlInterlockedAndBits(Flags, Flag) \
    InterlockedAnd((PLONG)(Flags), Flag)

#define RtlInterlockedClearBits(Flags, Flag) \
    RtlInterlockedAndBits(Flags, ~(Flag))

#define RtlInterlockedXorBits(Flags, Flag) \
    InterlockedXor(Flags, Flag)

#define RtlInterlockedSetBitsDiscardReturn(Flags, Flag) \
    (VOID) RtlInterlockedSetBits(Flags, Flag)

#define RtlInterlockedAndBitsDiscardReturn(Flags, Flag) \
    (VOID) RtlInterlockedAndBits(Flags, Flag)

#define RtlInterlockedClearBitsDiscardReturn(Flags, Flag) \
    RtlInterlockedAndBitsDiscardReturn(Flags, ~(Flag))

$endif (_WDMDDK_)

$if (_NTDDK_)

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

#define RtlInitializeSplayLinks(Links) {    \
  PRTL_SPLAY_LINKS _SplayLinks;            \
  _SplayLinks = (PRTL_SPLAY_LINKS)(Links); \
  _SplayLinks->Parent = _SplayLinks;   \
  _SplayLinks->LeftChild = NULL;       \
  _SplayLinks->RightChild = NULL;      \
}

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

#if !defined(MIDL_PASS)

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertLongToLuid(
  IN LONG Val)
{
  LUID Luid;
  LARGE_INTEGER Temp;

  Temp.QuadPart = Val;
  Luid.LowPart = Temp.u.LowPart;
  Luid.HighPart = Temp.u.HighPart;
  return Luid;
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(
  IN ULONG Val)
{
  LUID Luid;

  Luid.LowPart = Val;
  Luid.HighPart = 0;
  return Luid;
}

#endif /* !defined(MIDL_PASS) */

#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)
#define RtlGetCallersAddress(CallersAddress, CallersCaller) \
    *CallersAddress = (PVOID)_ReturnAddress(); \
    *CallersCaller = NULL;
#else
#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
  OUT PVOID *CallersAddress,
  OUT PVOID *CallersCaller);
#endif
#endif

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

#if (NTDDI_VERSION >= NTDDI_WIN7)

FORCEINLINE
VOID
NTAPI
RtlInitHashTableContext(
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context)
{
  Context->ChainHead = NULL;
  Context->PrevLinkage = NULL;
}

FORCEINLINE
VOID
NTAPI
RtlInitHashTableContextFromEnumerator(
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context,
  IN PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator)
{
  Context->ChainHead = Enumerator->ChainHead;
  Context->PrevLinkage = Enumerator->HashEntry.Linkage.Blink;
}

FORCEINLINE
VOID
NTAPI
RtlReleaseHashTableContext(
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context)
{
  UNREFERENCED_PARAMETER(Context);
  return;
}

FORCEINLINE
ULONG
NTAPI
RtlTotalBucketsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->TableSize;
}

FORCEINLINE
ULONG
NTAPI
RtlNonEmptyBucketsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI
RtlEmptyBucketsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->TableSize - HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI
RtlTotalEntriesHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NumEntries;
}

FORCEINLINE
ULONG
NTAPI
RtlActiveEnumeratorsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NumEnumerators;
}

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#endif /* !defined(MIDL_PASS) && !defined(SORTPP_PASS) */

$endif (_NTDDK_)
$if (_NTIFS_)

#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE 1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING 2

#define RtlUnicodeStringToOemSize(STRING) (NLS_MB_OEM_CODE_PAGE_TAG ?                                \
                                           RtlxUnicodeStringToOemSize(STRING) :                      \
                                           ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

#define RtlOemStringToUnicodeSize(STRING) (                 \
    NLS_MB_OEM_CODE_PAGE_TAG ?                              \
    RtlxOemStringToUnicodeSize(STRING) :                    \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)  \
)

#define RtlOemStringToCountedUnicodeSize(STRING) (                    \
    (ULONG)(RtlOemStringToUnicodeSize(STRING) - sizeof(UNICODE_NULL)) \
)

#define RtlOffsetToPointer(B,O) ((PCHAR)(((PCHAR)(B)) + ((ULONG_PTR)(O))))
#define RtlPointerToOffset(B,P) ((ULONG)(((PCHAR)(P)) - ((PCHAR)(B))))
$endif (_NTIFS_)
