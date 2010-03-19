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

BOOLEAN
FORCEINLINE
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

$endif
#if (NTDDI_VERSION >= NTDDI_WIN2K)

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

$endif
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
  IN PCUNICODE_STRING SourceString);

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
  IN OUT PULONG Remainder);
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
NTSTATUS
NTAPI
RtlHashUnicodeString(
  IN CONST UNICODE_STRING *String,
  IN BOOLEAN CaseInSensitive,
  IN ULONG HashAlgorithm,
  OUT PULONG HashValue);

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

#define LONG_SIZE (sizeof(LONG))
#define LONG_MASK (LONG_SIZE - 1)

/* VOID
 * RtlRetrieveUlong(
 *    PULONG DestinationAddress,
 *    PULONG SourceAddress);
 */
#if defined(_AMD64_)
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    *(ULONG UNALIGNED *)(DestAddress) = *(PULONG)(SrcAddress)
#else
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
#endif

/* VOID
 * RtlRetrieveUshort(
 *     PUSHORT DestinationAddress,
 *    PUSHORT SourceAddress);
 */
#if defined(_AMD64_)
#define RtlRetrieveUshort(DestAddress,SrcAddress) \
    *(USHORT UNALIGNED *)(DestAddress) = *(USHORT)(SrcAddress)
#else
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
#endif

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

/* VOID
 * RtlStoreUlong(
 *     IN PULONG Address,
 *     IN ULONG Value);
 */
#if defined(_AMD64_)
#define RtlStoreUlong(Address,Value) \
    *(ULONG UNALIGNED *)(Address) = (Value)
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
#endif

/* VOID
 * RtlStoreUlonglong(
 *     IN OUT PULONGLONG Address,
 *     ULONGLONG Value);
 */
#if defined(_AMD64_)
#define RtlStoreUlonglong(Address,Value) \
    *(ULONGLONG UNALIGNED *)(Address) = (Value)
#else
#define RtlStoreUlonglong(Address,Value) \
    if ((ULONG_PTR)(Address) & LONGLONG_MASK) { \
        RtlStoreUlong((ULONG_PTR)(Address), \
                      (ULONGLONG)(Value) & 0xFFFFFFFF); \
        RtlStoreUlong((ULONG_PTR)(Address)+sizeof(ULONG), \
                      (ULONGLONG)(Value) >> 32); \
    } else { \
        *((PULONGLONG)(Address)) = (ULONGLONG)(Value); \
    }
#endif

/* VOID
 * RtlStoreUlongPtr(
 *     IN OUT PULONG_PTR Address,
 *     IN ULONG_PTR Value);
 */
#ifdef _WIN64
#define RtlStoreUlongPtr(Address,Value)                         \
    RtlStoreUlonglong(Address,Value)
#else
#define RtlStoreUlongPtr(Address,Value)                         \
    RtlStoreUlong(Address,Value)
#endif

/* VOID
 * RtlStoreUshort(
 *     IN PUSHORT Address,
 *     IN USHORT Value);
 */
#if defined(_AMD64_)
#define RtlStoreUshort(Address,Value) \
    *(USHORT UNALIGNED *)(Address) = (Value)
#else
#define RtlStoreUshort(Address,Value) \
    if ((ULONG_PTR)(Address) & SHORT_MASK) { \
        ((PUCHAR) (Address))[SHORT_LEAST_SIGNIFICANT_BIT] = (UCHAR)(FIRSTBYTE(Value)); \
        ((PUCHAR) (Address))[SHORT_MOST_SIGNIFICANT_BIT ] = (UCHAR)(SECONDBYTE(Value)); \
    } \
    else { \
        *((PUSHORT) (Address)) = (USHORT)Value; \
    }
#endif

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

#endif // (NTDDI_VERSION >= NTDDI_WIN2K)

#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTSYSAPI
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(
  IN PVOID Source,
  IN SIZE_T Length);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

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

#endif // (NTDDI_VERSION >= NTDDI_WINXP)

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBitsUlongPtr(
  IN ULONG_PTR Target);

NTSYSAPI
ULONGLONG
NTAPI
RtlIoDecodeMemIoResource (
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

#endif

#if !defined(MIDL_PASS)
/* inline funftions */
//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertLongToLargeInteger(LONG SignedInteger)
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
  ULONG UnsignedInteger)
{
  LARGE_INTEGER ret;
  ret.QuadPart = UnsignedInteger;
  return ret;
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

#if defined(_AMD64_) || defined(_IA64_)
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedIntegerMultiply(
  LARGE_INTEGER Multiplicand,
  LONG Multiplier)
{
  LARGE_INTEGER ret;
  ret.QuadPart = Multiplicand.QuadPart * Multiplier;
  return ret;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedLargeIntegerDivide(
  LARGE_INTEGER Dividend,
  ULONG Divisor,
  PULONG Remainder)
{
  LARGE_INTEGER ret;
  ret.QuadPart = (ULONG64)Dividend.QuadPart / Divisor;
  if (Remainder)
    *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
  return ret;
}
#endif

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
  ret.QuadPart = Pos ? ret64 : -ret64;
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
ULONG
RtlCheckBit(
  IN PRTL_BITMAP BitMapHeader,
  IN ULONG BitPosition)
{
  return BitTest((LONG CONST*)BitMapHeader->Buffer, BitPosition);
}
#else
#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP)/32]) >> ((BP)%32)) & 0x1)
#endif /* defined(_M_AMD64) */

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
    RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, msg ), FALSE : TRUE)

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

VOID
FORCEINLINE
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

