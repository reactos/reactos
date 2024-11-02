/******************************************************************************
 *                         Runtime Library Functions                          *
 ******************************************************************************/

$if (_WDMDDK_ || _WINNT_)
#define FAST_FAIL_LEGACY_GS_VIOLATION           0
#define FAST_FAIL_VTGUARD_CHECK_FAILURE         1
#define FAST_FAIL_STACK_COOKIE_CHECK_FAILURE    2
#define FAST_FAIL_CORRUPT_LIST_ENTRY            3
#define FAST_FAIL_INCORRECT_STACK               4
#define FAST_FAIL_INVALID_ARG                   5
#define FAST_FAIL_GS_COOKIE_INIT                6
#define FAST_FAIL_FATAL_APP_EXIT                7
#define FAST_FAIL_RANGE_CHECK_FAILURE           8
#define FAST_FAIL_UNSAFE_REGISTRY_ACCESS        9
#define FAST_FAIL_GUARD_ICALL_CHECK_FAILURE     10
#define FAST_FAIL_GUARD_WRITE_CHECK_FAILURE     11
#define FAST_FAIL_INVALID_FIBER_SWITCH          12
#define FAST_FAIL_INVALID_SET_OF_CONTEXT        13
#define FAST_FAIL_INVALID_REFERENCE_COUNT       14
#define FAST_FAIL_INVALID_JUMP_BUFFER           18
#define FAST_FAIL_MRDATA_MODIFIED               19
#define FAST_FAIL_INVALID_FAST_FAIL_CODE        0xFFFFFFFF

DECLSPEC_NORETURN
FORCEINLINE
VOID
RtlFailFast(
  _In_ ULONG Code)
{
  __fastfail(Code);
}

$endif(_WDMDDK_ || _WINNT_)
$if (_WDMDDK_)

#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS) && (defined(_M_CEE_PURE) || defined(_M_CEE_SAFE))
#define NO_KERNEL_LIST_ENTRY_CHECKS
#endif

#if !defined(EXTRA_KERNEL_LIST_ENTRY_CHECKS) && defined(__REACTOS__)
#define EXTRA_KERNEL_LIST_ENTRY_CHECKS
#endif

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

#define RTL_STATIC_LIST_HEAD(x) LIST_ENTRY x = { &x, &x }

FORCEINLINE
VOID
InitializeListHead(
  _Out_ PLIST_ENTRY ListHead)
{
  ListHead->Flink = ListHead->Blink = ListHead;
}

_Must_inspect_result_
FORCEINLINE
BOOLEAN
IsListEmpty(
  _In_ const LIST_ENTRY * ListHead)
{
  return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
BOOLEAN
RemoveEntryListUnsafe(
  _In_ PLIST_ENTRY Entry)
{
  PLIST_ENTRY OldFlink;
  PLIST_ENTRY OldBlink;

  OldFlink = Entry->Flink;
  OldBlink = Entry->Blink;
  OldFlink->Blink = OldBlink;
  OldBlink->Flink = OldFlink;
  return (BOOLEAN)(OldFlink == OldBlink);
}

#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS)
FORCEINLINE
VOID
FatalListEntryError(
  _In_ PVOID P1,
  _In_ PVOID P2,
  _In_ PVOID P3)
{
  UNREFERENCED_PARAMETER(P1);
  UNREFERENCED_PARAMETER(P2);
  UNREFERENCED_PARAMETER(P3);

  RtlFailFast(FAST_FAIL_CORRUPT_LIST_ENTRY);
}

FORCEINLINE
VOID
RtlpCheckListEntry(
  _In_ PLIST_ENTRY Entry)
{
  if (Entry->Flink->Blink != Entry || Entry->Blink->Flink != Entry)
    FatalListEntryError(Entry->Blink, Entry, Entry->Flink);
}
#endif

FORCEINLINE
BOOLEAN
RemoveEntryList(
  _In_ PLIST_ENTRY Entry)
{
  PLIST_ENTRY OldFlink;
  PLIST_ENTRY OldBlink;

  OldFlink = Entry->Flink;
  OldBlink = Entry->Blink;
#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS)
#ifdef EXTRA_KERNEL_LIST_ENTRY_CHECKS
  if (OldFlink == Entry || OldBlink == Entry)
    FatalListEntryError(OldBlink, Entry, OldFlink);
#endif
  if (OldFlink->Blink != Entry || OldBlink->Flink != Entry)
    FatalListEntryError(OldBlink, Entry, OldFlink);
#endif
  OldFlink->Blink = OldBlink;
  OldBlink->Flink = OldFlink;
  return (BOOLEAN)(OldFlink == OldBlink);
}

FORCEINLINE
PLIST_ENTRY
RemoveHeadList(
  _Inout_ PLIST_ENTRY ListHead)
{
  PLIST_ENTRY Flink;
  PLIST_ENTRY Entry;

#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS) && DBG
  RtlpCheckListEntry(ListHead);
#ifdef EXTRA_KERNEL_LIST_ENTRY_CHECKS
  if (ListHead->Flink == ListHead || ListHead->Blink == ListHead)
    FatalListEntryError(ListHead->Blink, ListHead, ListHead->Flink);
#endif
#endif
  Entry = ListHead->Flink;
  Flink = Entry->Flink;
#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS)
  if (Entry->Blink != ListHead || Flink->Blink != Entry)
    FatalListEntryError(ListHead, Entry, Flink);
#endif
  ListHead->Flink = Flink;
  Flink->Blink = ListHead;
  return Entry;
}

FORCEINLINE
PLIST_ENTRY
RemoveTailList(
  _Inout_ PLIST_ENTRY ListHead)
{
  PLIST_ENTRY Blink;
  PLIST_ENTRY Entry;

#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS) && DBG
  RtlpCheckListEntry(ListHead);
#ifdef EXTRA_KERNEL_LIST_ENTRY_CHECKS
  if (ListHead->Flink == ListHead || ListHead->Blink == ListHead)
    FatalListEntryError(ListHead->Blink, ListHead, ListHead->Flink);
#endif
#endif
  Entry = ListHead->Blink;
  Blink = Entry->Blink;
#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS)
  if (Blink->Flink != Entry || Entry->Flink != ListHead)
    FatalListEntryError(Blink, Entry, ListHead);
#endif
  ListHead->Blink = Blink;
  Blink->Flink = ListHead;
  return Entry;
}

FORCEINLINE
VOID
InsertTailList(
  _Inout_ PLIST_ENTRY ListHead,
  _Inout_ __drv_aliasesMem PLIST_ENTRY Entry)
{
  PLIST_ENTRY OldBlink;
#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS) && DBG
  RtlpCheckListEntry(ListHead);
#endif
  OldBlink = ListHead->Blink;
  Entry->Flink = ListHead;
  Entry->Blink = OldBlink;
#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS)
  if (OldBlink->Flink != ListHead)
    FatalListEntryError(OldBlink->Blink, OldBlink, ListHead);
#endif
  OldBlink->Flink = Entry;
  ListHead->Blink = Entry;
}

FORCEINLINE
VOID
InsertHeadList(
  _Inout_ PLIST_ENTRY ListHead,
  _Inout_ __drv_aliasesMem PLIST_ENTRY Entry)
{
  PLIST_ENTRY OldFlink;
#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS) && DBG
  RtlpCheckListEntry(ListHead);
#endif
  OldFlink = ListHead->Flink;
  Entry->Flink = OldFlink;
  Entry->Blink = ListHead;
#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS)
  if (OldFlink->Blink != ListHead)
    FatalListEntryError(ListHead, OldFlink, OldFlink->Flink);
#endif
  OldFlink->Blink = Entry;
  ListHead->Flink = Entry;
}

FORCEINLINE
VOID
AppendTailList(
  _Inout_ PLIST_ENTRY ListHead,
  _Inout_ PLIST_ENTRY ListToAppend)
{
  PLIST_ENTRY ListEnd = ListHead->Blink;

#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS)
  RtlpCheckListEntry(ListHead);
  RtlpCheckListEntry(ListToAppend);
#endif
  ListHead->Blink->Flink = ListToAppend;
  ListHead->Blink = ListToAppend->Blink;
  ListToAppend->Blink->Flink = ListHead;
  ListToAppend->Blink = ListEnd;
}

FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryList(
  _Inout_ PSINGLE_LIST_ENTRY ListHead)
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
  _Inout_ PSINGLE_LIST_ENTRY ListHead,
  _Inout_ __drv_aliasesMem PSINGLE_LIST_ENTRY Entry)
{
  Entry->Next = ListHead->Next;
  ListHead->Next = Entry;
}

#endif /* !defined(MIDL_PASS) && !defined(SORTPP_PASS) */

__analysis_noreturn
NTSYSAPI
VOID
NTAPI
RtlAssert(
  _In_ PVOID FailedAssertion,
  _In_ PVOID FileName,
  _In_ ULONG LineNumber,
  _In_opt_z_ PSTR Message);

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
  _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
  _In_reads_bytes_(Length) const VOID UNALIGNED *Source,
  _In_ SIZE_T Length);
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

/* LOGICAL
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

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
  _Inout_ _At_(UnicodeString->Buffer, __drv_freesMem(Mem))
    PUNICODE_STRING UnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
  _In_ PUNICODE_STRING GuidString,
  _Out_ GUID *Guid);

_IRQL_requires_max_(DISPATCH_LEVEL)
_At_(DestinationString->Buffer, _Post_equal_to_(SourceString))
_When_(SourceString != NULL,
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(SourceString) * sizeof(WCHAR)))
_At_(DestinationString->MaximumLength, _Post_equal_to_(DestinationString->Length + sizeof(WCHAR))))
_When_(SourceString == NULL,
_At_(DestinationString->Length, _Post_equal_to_(0))
_At_(DestinationString->MaximumLength, _Post_equal_to_(0)))
NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCWSTR SourceString);

/* VOID
 * RtlMoveMemory(
 *    IN VOID UNALIGNED *Destination,
 *    IN CONST VOID UNALIGNED *Source,
 *    IN SIZE_T Length)
 */
#define RtlMoveMemory(Destination, Source, Length) \
    memmove(Destination, Source, Length)

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
  _In_ REFGUID Guid,
  _Out_ _At_(GuidString->Buffer, __drv_allocatesMem(Mem))
    PUNICODE_STRING GuidString);

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
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG StartingIndex,
  _In_ ULONG Length);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG StartingIndex,
  _In_ ULONG Length);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING DestinationString,
  _In_ PANSI_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
  _In_ PCANSI_STRING AnsiString);

#define RtlAnsiStringToUnicodeSize(String) (               \
  NLS_MB_CODE_PAGE_TAG ?                                   \
  RtlxAnsiStringToUnicodeSize(String) :                    \
  ((String)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)   \
)

_Success_(1)
_Unchanged_(Destination->MaximumLength)
_Unchanged_(Destination->Buffer)
_When_(_Old_(Destination->Length) + Source->Length <= Destination->MaximumLength,
  _At_(Destination->Length, _Post_equal_to_(_Old_(Destination->Length) + Source->Length))
  _At_(return, _Out_range_(==, 0)))
_When_(_Old_(Destination->Length) + Source->Length > Destination->MaximumLength,
  _Unchanged_(Destination->Length)
  _At_(return, _Out_range_(<, 0)))
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
  _Inout_ PUNICODE_STRING Destination,
  _In_ PCUNICODE_STRING Source);

_Success_(1)
_Unchanged_(Destination->MaximumLength)
_Unchanged_(Destination->Buffer)
/* _When_(_Old_(Destination->Length) + _String_length_(Source) * sizeof(WCHAR) <= Destination->MaximumLength,
  _At_(Destination->Length, _Post_equal_to_(_Old_(Destination->Length) + _String_length_(Source) * sizeof(WCHAR)))
  _At_(return, _Out_range_(==, 0)))
_When_(_Old_(Destination->Length) + _String_length_(Source) * sizeof(WCHAR) > Destination->MaximumLength,
  _Unchanged_(Destination->Length)
  _At_(return, _Out_range_(<, 0))) */
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
  _Inout_ PUNICODE_STRING Destination,
  _In_opt_z_ PCWSTR Source);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
  _In_ ULONG RelativeTo,
  _In_ PWSTR Path);

NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
  _In_ PRTL_BITMAP BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlClearBits(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
  _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear);

_Must_inspect_result_
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory(
  _In_ const VOID *Source1,
  _In_ const VOID *Source2,
  _In_ SIZE_T Length);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
  _In_ PCUNICODE_STRING String1,
  _In_ PCUNICODE_STRING String2,
  _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
  _In_reads_(String1Length) PCWCH String1,
  _In_ SIZE_T String1Length,
  _In_reads_(String2Length) PCWCH String2,
  _In_ SIZE_T String2Length,
  _In_ BOOLEAN CaseInSensitive);

_Unchanged_(DestinationString->Buffer)
_Unchanged_(DestinationString->MaximumLength)
_At_(DestinationString->Length,
  _When_(SourceString->Length > DestinationString->MaximumLength,
    _Post_equal_to_(DestinationString->MaximumLength))
  _When_(SourceString->Length <= DestinationString->MaximumLength,
    _Post_equal_to_(SourceString->Length)))
NTSYSAPI
VOID
NTAPI
RtlCopyUnicodeString(
  _Inout_ PUNICODE_STRING DestinationString,
  _In_opt_ PCUNICODE_STRING SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
  _In_ ULONG RelativeTo,
  _In_ PWSTR Path);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
  _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ ULONG Revision);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRegistryValue(
  _In_ ULONG RelativeTo,
  _In_ PCWSTR Path,
  _In_z_ PCWSTR ValueName);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
  _In_ CONST UNICODE_STRING *String1,
  _In_ CONST UNICODE_STRING *String2,
  _In_ BOOLEAN CaseInSensitive);

#if !defined(_AMD64_) && !defined(_IA64_)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedIntegerMultiply(
  _In_ LARGE_INTEGER Multiplicand,
  _In_ LONG Multiplier);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedLargeIntegerDivide(
  _In_ LARGE_INTEGER Dividend,
  _In_ ULONG Divisor,
  _Out_opt_ PULONG Remainder);
#endif

#if defined(_X86_) || defined(_IA64_)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedMagicDivide(
    _In_ LARGE_INTEGER Dividend,
    _In_ LARGE_INTEGER MagicDivisor,
    _In_ CCHAR  ShiftCount);
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
  _Inout_ _At_(AnsiString->Buffer, __drv_freesMem(Mem))
    PANSI_STRING AnsiString);

_Success_(return != -1)
_Must_inspect_result_
NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG NumberToFind,
  _In_ ULONG HintIndex);

_Success_(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG NumberToFind,
  _In_ ULONG HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
  _In_ PRTL_BITMAP BitMapHeader,
  _Out_ PULONG StartingIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns(
  _In_ PRTL_BITMAP BitMapHeader,
  _Out_writes_to_(SizeOfRunArray, return) PRTL_BITMAP_RUN RunArray,
  _In_range_(>, 0) ULONG SizeOfRunArray,
  _In_ BOOLEAN LocateLongestRuns);

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG FromIndex,
  _Out_ PULONG StartingRunIndex);

_Success_(return != -1)
_Must_inspect_result_
NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit(
  _In_ ULONGLONG Set);

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
  _In_ PRTL_BITMAP BitMapHeader,
  _Out_ PULONG StartingIndex);

_Success_(return != -1)
_Must_inspect_result_
NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit(
  _In_ ULONGLONG Set);

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG FromIndex,
  _Out_ PULONG StartingRunIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunSet(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG FromIndex,
  _Out_ PULONG StartingRunIndex);

_Success_(return != -1)
_Must_inspect_result_
NTSYSAPI
ULONG
NTAPI
RtlFindSetBits(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG NumberToFind,
  _In_ ULONG HintIndex);

_Success_(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_ ULONG NumberToFind,
  _In_ ULONG HintIndex);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
  _Out_ PANSI_STRING DestinationString,
  _In_opt_z_ __drv_aliasesMem PCSZ SourceString);

_At_(BitMapHeader->SizeOfBitMap, _Post_equal_to_(SizeOfBitMap))
_At_(BitMapHeader->Buffer, _Post_equal_to_(BitMapBuffer))
NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
  _Out_ PRTL_BITMAP BitMapHeader,
  _In_opt_ __drv_aliasesMem PULONG BitMapBuffer,
  _In_opt_ ULONG SizeOfBitMap);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitString(
  _Out_ PSTRING DestinationString,
  _In_opt_z_ __drv_aliasesMem PCSZ SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_At_(String->MaximumLength, _Const_)
NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
  _In_ ULONG Value,
  _In_opt_ ULONG Base,
  _Inout_ PUNICODE_STRING String);

_IRQL_requires_max_(PASSIVE_LEVEL)
_At_(String->MaximumLength, _Const_)
NTSYSAPI
NTSTATUS
NTAPI
RtlInt64ToUnicodeString(
  _In_ ULONGLONG Value,
  _In_opt_ ULONG Base,
  _Inout_ PUNICODE_STRING String);

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

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlLengthSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
  _In_ PRTL_BITMAP BitMapHeader);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
  _In_ PRTL_BITMAP BitMapHeader);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValues(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _Inout_ _At_(*(*QueryTable).EntryContext, _Pre_unknown_)
        PRTL_QUERY_REGISTRY_TABLE QueryTable,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID Environment);

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
  _In_ PRTL_BITMAP BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlSetBits(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
  _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
  _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ BOOLEAN DaclPresent,
  _In_opt_ PACL Dacl,
  _In_opt_ BOOLEAN DaclDefaulted);

//
// These functions are really bad and shouldn't be used.
// They have no type checking and can easily overwrite the target
// variable or only set half of it.
// Use Read/WriteUnalignedU16/U32/U64 from reactos/unaligned.h instead.
//
#if defined(_AMD64_) || defined(_M_AMD64) || defined(_X86) || defined(_M_IX86)

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
    *(USHORT*)(DestAddress) = *(USHORT UNALIGNED *)(SrcAddress)

/* VOID
 * RtlRetrieveUlong(
 *    PULONG DestinationAddress,
 *    PULONG SourceAddress);
 */
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    *(ULONG*)(DestAddress) = *(ULONG UNALIGNED *)(SrcAddress)

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

#endif /* defined(_AMD64_) || defined(_M_AMD64) || defined(_X86) || defined(_M_IX86) */

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

_Success_(return != FALSE)
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
    _In_ PTIME_FIELDS TimeFields,
    _Out_ PLARGE_INTEGER Time);

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
  _In_ PLARGE_INTEGER Time,
  _Out_ PTIME_FIELDS TimeFields);

NTSYSAPI
USHORT
FASTCALL
RtlUshortByteSwap(
  _In_ USHORT Source);

NTSYSAPI
ULONG
FASTCALL
RtlUlongByteSwap(
  _In_ ULONG Source);

NTSYSAPI
ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
  _In_ ULONGLONG Source);

_When_(AllocateDestinationString,
  _At_(DestinationString->MaximumLength,
    _Out_range_(<=, (SourceString->MaximumLength / sizeof(WCHAR)))))
_When_(!AllocateDestinationString,
  _At_(DestinationString->Buffer, _Const_)
  _At_(DestinationString->MaximumLength, _Const_))
_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PANSI_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToAnsiString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PANSI_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(
  _In_ PCUNICODE_STRING UnicodeString);

#define RtlUnicodeStringToAnsiSize(String) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(String) :                     \
    ((String)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
  _In_ PCUNICODE_STRING String,
  _In_opt_ ULONG Base,
  _Out_ PULONG Value);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(
  _In_ WCHAR SourceCharacter);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
  _In_reads_bytes_(SecurityDescriptorLength) PSECURITY_DESCRIPTOR SecurityDescriptorInput,
  _In_ ULONG SecurityDescriptorLength,
  _In_ SECURITY_INFORMATION RequiredInformation);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
    _Out_
    _At_(lpVersionInformation->dwOSVersionInfoSize, _Pre_ _Valid_)
    _When_(lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW),
        _At_((PRTL_OSVERSIONINFOEXW)lpVersionInformation, _Out_))
        PRTL_OSVERSIONINFOW lpVersionInformation);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
    _In_ PRTL_OSVERSIONINFOEXW VersionInfo,
    _In_ ULONG TypeMask,
    _In_ ULONGLONG ConditionMask);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
  _In_ ULONG RelativeTo,
  _In_ PCWSTR Path,
  _In_z_ PCWSTR ValueName,
  _In_ ULONG ValueType,
  _In_reads_bytes_opt_(ValueLength) PVOID ValueData,
  _In_ ULONG ValueLength);

$endif (_WDMDDK_)
$if (_NTDDK_)

#ifndef RTL_USE_AVL_TABLES

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable(
  _Out_ PRTL_GENERIC_TABLE Table,
  _In_ PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
  _In_ PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
  _In_ PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
  _In_opt_ PVOID TableContext);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable(
  _In_ PRTL_GENERIC_TABLE Table,
  _In_reads_bytes_(BufferSize) PVOID Buffer,
  _In_ CLONG BufferSize,
  _Out_opt_ PBOOLEAN NewElement);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull(
  _In_ PRTL_GENERIC_TABLE Table,
  _In_reads_bytes_(BufferSize) PVOID Buffer,
  _In_ CLONG BufferSize,
  _Out_opt_ PBOOLEAN NewElement,
  _In_ PVOID NodeOrParent,
  _In_ TABLE_SEARCH_RESULT SearchResult);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(
  _In_ PRTL_GENERIC_TABLE Table,
  _In_ PVOID Buffer);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable(
  _In_ PRTL_GENERIC_TABLE Table,
  _In_ PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull(
  _In_ PRTL_GENERIC_TABLE Table,
  _In_ PVOID Buffer,
  _Out_ PVOID *NodeOrParent,
  _Out_ TABLE_SEARCH_RESULT *SearchResult);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable(
  _In_ PRTL_GENERIC_TABLE Table,
  _In_ BOOLEAN Restart);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(
  _In_ PRTL_GENERIC_TABLE Table,
  _Inout_ PVOID *RestartKey);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
  _In_ PRTL_GENERIC_TABLE Table,
  _In_ ULONG I);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
  _In_ PRTL_GENERIC_TABLE Table);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(
  _In_ PRTL_GENERIC_TABLE Table);

#endif /* !RTL_USE_AVL_TABLES */

#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT     8

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(
  _Inout_ PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(
  _In_ PRTL_SPLAY_LINKS Links);

NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay(
  _In_ PRTL_SPLAY_LINKS Links,
  _Inout_ PRTL_SPLAY_LINKS *Root);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(
  _In_ PRTL_SPLAY_LINKS Links);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(
  _In_ PRTL_SPLAY_LINKS Links);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(
  _In_ PRTL_SPLAY_LINKS Links);

_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(
  _In_ PRTL_SPLAY_LINKS Links);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
  _In_ PCUNICODE_STRING String1,
  _In_ PCUNICODE_STRING String2,
  _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlUpperString(
  _Inout_ PSTRING DestinationString,
  _In_ const STRING *SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
  _Inout_ PACCESS_MASK AccessMask,
  _In_ PGENERIC_MAPPING GenericMapping);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlVolumeDeviceToDosName(
  _In_ PVOID VolumeDeviceObject,
  _Out_ PUNICODE_STRING DosName);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareString(
  _In_ const STRING *String1,
  _In_ const STRING *String2,
  _In_ BOOLEAN CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlCopyString(
  _Out_ PSTRING DestinationString,
  _In_opt_ const STRING *SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
  _In_ const STRING *String1,
  _In_ const STRING *String2,
  _In_ BOOLEAN CaseInSensitive);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
  _In_z_ PCSZ String,
  _In_opt_ ULONG Base,
  _Out_ PULONG Value);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
CHAR
NTAPI
RtlUpperChar(
  _In_ CHAR Character);

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
  _Out_writes_(Count - (Flags >> RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT))
    PVOID *Callers,
  _In_ ULONG Count,
  _In_ ULONG Flags);

$endif (_NTDDK_)
$if (_NTIFS_)

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
  _In_ HANDLE HeapHandle,
  _In_opt_ ULONG Flags,
  _In_ SIZE_T Size);

_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
  _In_ PVOID HeapHandle,
  _In_opt_ ULONG Flags,
  _In_ _Post_invalid_ PVOID BaseAddress);

NTSYSAPI
VOID
NTAPI
RtlCaptureContext(
  _Out_ PCONTEXT ContextRecord);

_Ret_range_(<, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandom(
  _Inout_ PULONG Seed);

_IRQL_requires_max_(APC_LEVEL)
_Success_(return != 0)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
  _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem))
    PUNICODE_STRING DestinationString,
  _In_z_ PCWSTR SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixString(
  _In_ const STRING *String1,
  _In_ const STRING *String2,
  _In_ BOOLEAN CaseInsensitive);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendStringToString(
  _Inout_ PSTRING Destination,
  _In_ const STRING *Source);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING DestinationString,
  _In_ PCOEM_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToCountedUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING DestinationString,
  _In_ PCOEM_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToCountedOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSYSAPI
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(UniDest->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING UniDest,
  _In_ PCUNICODE_STRING UniSource,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeOemString(
  _Inout_ _At_(OemString->Buffer, __drv_freesMem(Mem)) POEM_STRING OemString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToOemSize(
  _In_ PCUNICODE_STRING UnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlxOemStringToUnicodeSize(
  _In_ PCOEM_STRING OemString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
  _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG MaxBytesInUnicodeString,
  _Out_opt_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInMultiByteString) const CHAR *MultiByteString,
  _In_ ULONG BytesInMultiByteString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
  _Out_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInMultiByteString) const CHAR *MultiByteString,
  _In_ ULONG BytesInMultiByteString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
  _Out_ PULONG BytesInMultiByteString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
  _Out_writes_bytes_to_(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
  _In_ ULONG MaxBytesInMultiByteString,
  _Out_opt_ PULONG BytesInMultiByteString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToMultiByteN(
  _Out_writes_bytes_to_(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
  _In_ ULONG MaxBytesInMultiByteString,
  _Out_opt_ PULONG BytesInMultiByteString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlOemToUnicodeN(
  _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWSTR UnicodeString,
  _In_ ULONG MaxBytesInUnicodeString,
  _Out_opt_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInOemString) PCCH OemString,
  _In_ ULONG BytesInOemString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToOemN(
  _Out_writes_bytes_to_(MaxBytesInOemString, *BytesInOemString) PCHAR OemString,
  _In_ ULONG MaxBytesInOemString,
  _Out_opt_ PULONG BytesInOemString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToOemN(
  _Out_writes_bytes_to_(MaxBytesInOemString, *BytesInOemString) PCHAR OemString,
  _In_ ULONG MaxBytesInOemString,
  _Out_opt_ PULONG BytesInOemString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGenerate8dot3Name(
  _In_ PCUNICODE_STRING Name,
  _In_ BOOLEAN AllowExtendedCharacters,
  _Inout_ PGENERATE_NAME_CONTEXT Context,
  _Inout_ PUNICODE_STRING Name8dot3);
#else
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlGenerate8dot3Name(
  _In_ PCUNICODE_STRING Name,
  _In_ BOOLEAN AllowExtendedCharacters,
  _Inout_ PGENERATE_NAME_CONTEXT Context,
  _Inout_ PUNICODE_STRING Name8dot3);
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(
  _In_ PCUNICODE_STRING Name,
  _Inout_opt_ POEM_STRING OemName,
  _Out_opt_ PBOOLEAN NameContainsSpaces);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidOemCharacter(
  _Inout_ PWCHAR Char);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
PfxInitialize(
  _Out_ PPREFIX_TABLE PrefixTable);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
BOOLEAN
NTAPI
PfxInsertPrefix(
  _In_ PPREFIX_TABLE PrefixTable,
  _In_ __drv_aliasesMem PSTRING Prefix,
  _Out_ PPREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
PfxRemovePrefix(
  _In_ PPREFIX_TABLE PrefixTable,
  _In_ PPREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
PPREFIX_TABLE_ENTRY
NTAPI
PfxFindPrefix(
  _In_ PPREFIX_TABLE PrefixTable,
  _In_ PSTRING FullName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitializeUnicodePrefix(
  _Out_ PUNICODE_PREFIX_TABLE PrefixTable);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
BOOLEAN
NTAPI
RtlInsertUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ __drv_aliasesMem PUNICODE_STRING Prefix,
  _Out_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlRemoveUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ PUNICODE_STRING FullName,
  _In_ ULONG CaseInsensitiveIndex);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlNextUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ BOOLEAN Restart);

_Must_inspect_result_
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong(
  _In_reads_bytes_(Length) PVOID Source,
  _In_ SIZE_T Length,
  _In_ ULONG Pattern);

_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1980(
  _In_ PLARGE_INTEGER Time,
  _Out_ PULONG ElapsedSeconds);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1980ToTime(
  _In_ ULONG ElapsedSeconds,
  _Out_ PLARGE_INTEGER Time);

_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
  _In_ ULONG ElapsedSeconds,
  _Out_ PLARGE_INTEGER Time);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(
  _In_ PSID Sid);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid(
  _In_ PSID Sid1,
  _In_ PSID Sid2);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualPrefixSid(
  _In_ PSID Sid1,
  _In_ PSID Sid2);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(
  _In_ ULONG SubAuthorityCount);

NTSYSAPI
PVOID
NTAPI
RtlFreeSid(
  _In_ _Post_invalid_ PSID Sid);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateAndInitializeSid(
  _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
  _In_ UCHAR SubAuthorityCount,
  _In_ ULONG SubAuthority0,
  _In_ ULONG SubAuthority1,
  _In_ ULONG SubAuthority2,
  _In_ ULONG SubAuthority3,
  _In_ ULONG SubAuthority4,
  _In_ ULONG SubAuthority5,
  _In_ ULONG SubAuthority6,
  _In_ ULONG SubAuthority7,
  _Outptr_ PSID *Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
  _Out_ PSID Sid,
  _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
  _In_ UCHAR SubAuthorityCount);

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
  _In_ PSID Sid,
  _In_ ULONG SubAuthority);

_Post_satisfies_(return >= 8 && return <= SECURITY_MAX_SID_SIZE)
NTSYSAPI
ULONG
NTAPI
RtlLengthSid(
  _In_ PSID Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
  _In_ ULONG DestinationSidLength,
  _Out_writes_bytes_(DestinationSidLength) PSID DestinationSid,
  _In_ PSID SourceSid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
  _Inout_ PUNICODE_STRING UnicodeString,
  _In_ PSID Sid,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlCopyLuid(
  _Out_ PLUID DestinationLuid,
  _In_ PLUID SourceLuid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
  _Out_writes_bytes_(AclLength) PACL Acl,
  _In_ ULONG AclLength,
  _In_ ULONG AclRevision);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce(
  _Inout_ PACL Acl,
  _In_ ULONG AceRevision,
  _In_ ULONG StartingAceIndex,
  _In_reads_bytes_(AceListLength) PVOID AceList,
  _In_ ULONG AceListLength);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
  _Inout_ PACL Acl,
  _In_ ULONG AceIndex);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
  _In_ PACL Acl,
  _In_ ULONG AceIndex,
  _Outptr_ PVOID *Ace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
  _Inout_ PACL Acl,
  _In_ ULONG AceRevision,
  _In_ ACCESS_MASK AccessMask,
  _In_ PSID Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAceEx(
  _Inout_ PACL Acl,
  _In_ ULONG AceRevision,
  _In_ ULONG AceFlags,
  _In_ ACCESS_MASK AccessMask,
  _In_ PSID Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptorRelative(
  _Out_ PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
  _In_ ULONG Revision);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PBOOLEAN DaclPresent,
  _Out_ PACL *Dacl,
  _Out_ PBOOLEAN DaclDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(
  _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSID Owner,
  _In_opt_ BOOLEAN OwnerDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PSID *Owner,
  _Out_ PBOOLEAN OwnerDefaulted);

_IRQL_requires_max_(APC_LEVEL)
_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
  _In_ NTSTATUS Status);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCustomCPToUnicodeN(
  _In_ PCPTABLEINFO CustomCP,
  _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG MaxBytesInUnicodeString,
  _Out_opt_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInCustomCPString) PCH CustomCPString,
  _In_ ULONG BytesInCustomCPString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToCustomCPN(
  _In_ PCPTABLEINFO CustomCP,
  _Out_writes_bytes_to_(MaxBytesInCustomCPString, *BytesInCustomCPString) PCH CustomCPString,
  _In_ ULONG MaxBytesInCustomCPString,
  _Out_opt_ PULONG BytesInCustomCPString,
  _In_reads_bytes_(BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToCustomCPN(
  _In_ PCPTABLEINFO CustomCP,
  _Out_writes_bytes_to_(MaxBytesInCustomCPString, *BytesInCustomCPString) PCH CustomCPString,
  _In_ ULONG MaxBytesInCustomCPString,
  _Out_opt_ PULONG BytesInCustomCPString,
  _In_reads_bytes_(BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
  _In_ PUSHORT TableBase,
  _Out_ PCPTABLEINFO CodePageTable);

$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTSYSAPI
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(
  _In_ PVOID Source,
  _In_ SIZE_T Length);
#endif

$endif (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WINXP)

$if (_WDMDDK_)

NTSYSAPI
VOID
NTAPI
RtlClearBit(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
WCHAR
NTAPI
RtlDowncaseUnicodeChar(
  _In_ WCHAR SourceCharacter);

NTSYSAPI
VOID
NTAPI
RtlSetBit(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
  _In_ CONST UNICODE_STRING *String,
  _In_ BOOLEAN CaseInSensitive,
  _In_ ULONG HashAlgorithm,
  _Out_ PULONG HashValue);

$endif (_WDMDDK_)

$if (_NTDDK_)

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl(
  _Out_ PRTL_AVL_TABLE Table,
  _In_ PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
  _In_opt_ PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
  _In_opt_ PRTL_AVL_FREE_ROUTINE FreeRoutine,
  _In_opt_ PVOID TableContext);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_reads_bytes_(BufferSize) PVOID Buffer,
  _In_ CLONG BufferSize,
  _Out_opt_ PBOOLEAN NewElement);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_reads_bytes_(BufferSize) PVOID Buffer,
  _In_ CLONG BufferSize,
  _Out_opt_ PBOOLEAN NewElement,
  _In_ PVOID NodeOrParent,
  _In_ TABLE_SEARCH_RESULT SearchResult);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_ PVOID Buffer);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_ PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_ PVOID Buffer,
  _Out_ PVOID *NodeOrParent,
  _Out_ TABLE_SEARCH_RESULT *SearchResult);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_ BOOLEAN Restart);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(
  _In_ PRTL_AVL_TABLE Table,
  _Inout_ PVOID *RestartKey);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_ PVOID Buffer,
  _Out_ PVOID *RestartKey);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory(
  _In_ PRTL_AVL_TABLE Table,
  _In_opt_ PRTL_AVL_MATCH_FUNCTION MatchFunction,
  _In_opt_ PVOID MatchData,
  _In_ ULONG NextFlag,
  _Inout_ PVOID *RestartKey,
  _Inout_ PULONG DeleteCount,
  _In_ PVOID Buffer);

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl(
  _In_ PRTL_AVL_TABLE Table,
  _In_ ULONG I);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(
  _In_ PRTL_AVL_TABLE Table);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(
  _In_ PRTL_AVL_TABLE Table);

$endif (_NTDDK_)
$if (_NTIFS_)

_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
  _In_ ULONG Flags,
  _In_opt_ PVOID HeapBase,
  _In_opt_ SIZE_T ReserveSize,
  _In_opt_ SIZE_T CommitSize,
  _In_opt_ PVOID Lock,
  _In_opt_ PRTL_HEAP_PARAMETERS Parameters);

NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
  _In_ _Post_invalid_ PVOID HeapHandle);

NTSYSAPI
USHORT
NTAPI
RtlCaptureStackBackTrace(
  _In_ ULONG FramesToSkip,
  _In_ ULONG FramesToCapture,
  _Out_writes_to_(FramesToCapture, return) PVOID *BackTrace,
  _Out_opt_ PULONG BackTraceHash);

_Ret_range_(<, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
  _Inout_ PULONG Seed);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
  _Out_ PUNICODE_STRING DestinationString,
  _In_opt_z_ __drv_aliasesMem PCWSTR SourceString);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlValidateUnicodeString(
  _In_ ULONG Flags,
  _In_ PCUNICODE_STRING String);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
  _In_ ULONG Flags,
  _In_ PCUNICODE_STRING SourceString,
  _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING DestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
  _In_ USHORT CompressionFormatAndEngine,
  _Out_ PULONG CompressBufferWorkSpaceSize,
  _Out_ PULONG CompressFragmentWorkSpaceSize);

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressBuffer(
  _In_ USHORT CompressionFormatAndEngine,
  _In_reads_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _In_ ULONG UncompressedChunkSize,
  _Out_ PULONG FinalCompressedSize,
  _In_ PVOID WorkSpace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBuffer(
  _In_ USHORT CompressionFormat,
  _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _Out_ PULONG FinalUncompressedSize);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressFragment(
  _In_ USHORT CompressionFormat,
  _Out_writes_bytes_to_(UncompressedFragmentSize, *FinalUncompressedSize) PUCHAR UncompressedFragment,
  _In_ ULONG UncompressedFragmentSize,
  _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _In_range_(<, CompressedBufferSize) ULONG FragmentOffset,
  _Out_ PULONG FinalUncompressedSize,
  _In_ PVOID WorkSpace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk(
  _In_ USHORT CompressionFormat,
  _Inout_ PUCHAR *CompressedBuffer,
  _In_ PUCHAR EndOfCompressedBufferPlus1,
  _Out_ PUCHAR *ChunkBuffer,
  _Out_ PULONG ChunkSize);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlReserveChunk(
  _In_ USHORT CompressionFormat,
  _Inout_ PUCHAR *CompressedBuffer,
  _In_ PUCHAR EndOfCompressedBufferPlus1,
  _Out_ PUCHAR *ChunkBuffer,
  _In_ ULONG ChunkSize);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressChunks(
  _Out_writes_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _In_reads_bytes_(CompressedTailSize) PUCHAR CompressedTail,
  _In_ ULONG CompressedTailSize,
  _In_ PCOMPRESSED_DATA_INFO CompressedDataInfo);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCompressChunks(
  _In_reads_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _Out_writes_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_range_(>=, (UncompressedBufferSize - (UncompressedBufferSize / 16))) ULONG CompressedBufferSize,
  _Inout_updates_bytes_(CompressedDataInfoLength) PCOMPRESSED_DATA_INFO CompressedDataInfo,
  _In_range_(>, sizeof(COMPRESSED_DATA_INFO)) ULONG CompressedDataInfoLength,
  _In_ PVOID WorkSpace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(
  _In_ PSID Sid);

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
  _In_ PSID Sid);

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
  _In_ NTSTATUS Status);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSystemVolumeInformationFolder(
  _In_ PCUNICODE_STRING VolumeRootPath);

#if defined(_M_AMD64)

FORCEINLINE
VOID
RtlFillMemoryUlong(
  _Out_writes_bytes_all_(Length) PVOID Destination,
  _In_ SIZE_T Length,
  _In_ ULONG Pattern)
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
  _Out_writes_bytes_all_(Length) PVOID Destination,
  _In_ SIZE_T Length,
  _In_ ULONG Pattern);

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlonglong(
  _Out_writes_bytes_all_(Length) PVOID Destination,
  _In_ SIZE_T Length,
  _In_ ULONGLONG Pattern);

#endif /* defined(_M_AMD64) */
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

$if (_NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WS03)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
  _Out_ PANSI_STRING DestinationString,
  _In_opt_z_ __drv_aliasesMem PCSZ SourceString);
#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PBOOLEAN SaclPresent,
  _Out_ PACL *Sacl,
  _Out_ PBOOLEAN SaclDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
  _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSID Group,
  _In_opt_ BOOLEAN GroupDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PSID *Group,
  _Out_ PBOOLEAN GroupDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
  _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
  _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
  _Inout_ PULONG BufferLength);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
  _In_ PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
  _Out_writes_bytes_to_opt_(*AbsoluteSecurityDescriptorSize, *AbsoluteSecurityDescriptorSize) PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
  _Inout_ PULONG AbsoluteSecurityDescriptorSize,
  _Out_writes_bytes_to_opt_(*DaclSize, *DaclSize) PACL Dacl,
  _Inout_ PULONG DaclSize,
  _Out_writes_bytes_to_opt_(*SaclSize, *SaclSize) PACL Sacl,
  _Inout_ PULONG SaclSize,
  _Out_writes_bytes_to_opt_(*OwnerSize, *OwnerSize) PSID Owner,
  _Inout_ PULONG OwnerSize,
  _Out_writes_bytes_to_opt_(*PrimaryGroupSize, *PrimaryGroupSize) PSID PrimaryGroup,
  _Inout_ PULONG PrimaryGroupSize);

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */
$endif (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_VISTA)

$if (_WDMDDK_)
NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBitsUlongPtr(
  _In_ ULONG_PTR Target);

NTSYSAPI
ULONGLONG
NTAPI
RtlIoDecodeMemIoResource(
  _In_ struct _IO_RESOURCE_DESCRIPTOR *Descriptor,
  _Out_opt_ PULONGLONG Alignment,
  _Out_opt_ PULONGLONG MinimumAddress,
  _Out_opt_ PULONGLONG MaximumAddress);

NTSYSAPI
NTSTATUS
NTAPI
RtlIoEncodeMemIoResource(
  _In_ struct _IO_RESOURCE_DESCRIPTOR *Descriptor,
  _In_ UCHAR Type,
  _In_ ULONGLONG Length,
  _In_ ULONGLONG Alignment,
  _In_ ULONGLONG MinimumAddress,
  _In_ ULONGLONG MaximumAddress);

NTSYSAPI
ULONGLONG
NTAPI
RtlCmDecodeMemIoResource(
  _In_ struct _CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor,
  _Out_opt_ PULONGLONG Start);

NTSYSAPI
NTSTATUS
NTAPI
RtlFindClosestEncodableLength(
  _In_ ULONGLONG SourceLength,
  _Out_ PULONGLONG TargetLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlCmEncodeMemIoResource(
  _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
  _In_ UCHAR Type,
  _In_ ULONGLONG Length,
  _In_ ULONGLONG Start);

$endif (_WDMDDK_)
$if (_NTDDK_)

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
  _Out_ PRTL_RUN_ONCE RunOnce);

_IRQL_requires_max_(APC_LEVEL)
_Maybe_raises_SEH_exception_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
  _Inout_ PRTL_RUN_ONCE RunOnce,
  _In_ __inner_callback PRTL_RUN_ONCE_INIT_FN InitFn,
  _Inout_opt_ PVOID Parameter,
  _Outptr_opt_result_maybenull_ PVOID *Context);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
  _Inout_ PRTL_RUN_ONCE RunOnce,
  _In_ ULONG Flags,
  _Outptr_opt_result_maybenull_ PVOID *Context);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
  _Inout_ PRTL_RUN_ONCE RunOnce,
  _In_ ULONG Flags,
  _In_opt_ PVOID Context);

NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
  _In_ ULONG OSMajorVersion,
  _In_ ULONG OSMinorVersion,
  _In_ ULONG SpMajorVersion,
  _In_ ULONG SpMinorVersion,
  _Out_ PULONG ReturnedProductType);

$endif (_NTDDK_)
$if (_NTIFS_)
NTSYSAPI
NTSTATUS
NTAPI
RtlNormalizeString(
  _In_ ULONG NormForm,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
  _Inout_ PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlIsNormalizedString(
  _In_ ULONG NormForm,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_ PBOOLEAN Normalized);

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToAscii(
  _In_ ULONG Flags,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
  _Inout_ PLONG DestinationStringLength);

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
  _In_ ULONG Flags,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
  _Inout_ PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateServiceSid(
  _In_ PUNICODE_STRING ServiceName,
  _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
  _Inout_ PULONG ServiceSidLength);

NTSYSAPI
LONG
NTAPI
RtlCompareAltitudes(
  _In_ PCUNICODE_STRING Altitude1,
  _In_ PCUNICODE_STRING Altitude2);

$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

$if (_WDMDDK_)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToUTF8N(
  _Out_writes_bytes_to_(UTF8StringMaxByteCount, *UTF8StringActualByteCount)
    PCHAR UTF8StringDestination,
  _In_ ULONG UTF8StringMaxByteCount,
  _Out_ PULONG UTF8StringActualByteCount,
  _In_reads_bytes_(UnicodeStringByteCount) PCWCH UnicodeStringSource,
  _In_ ULONG UnicodeStringByteCount);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8ToUnicodeN(
  _Out_writes_bytes_to_(UnicodeStringMaxByteCount, *UnicodeStringActualByteCount)
    PWSTR UnicodeStringDestination,
  _In_ ULONG UnicodeStringMaxByteCount,
  _Out_ PULONG UnicodeStringActualByteCount,
  _In_reads_bytes_(UTF8StringByteCount) PCCH UTF8StringSource,
  _In_ ULONG UTF8StringByteCount);

NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedFeatures(
  IN ULONG64 FeatureMask);

$endif (_WDMDDK_)
$if (_NTDDK_)

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTable(
  _Inout_ _When_(NULL == *HashTable, __drv_allocatesMem(Mem))
    PRTL_DYNAMIC_HASH_TABLE *HashTable,
  _In_ ULONG Shift,
  _In_ _Reserved_ ULONG Flags);

NTSYSAPI
VOID
NTAPI
RtlDeleteHashTable(
  _In_ _When_((HashTable->Flags & RTL_HASH_ALLOCATED_HEADER), __drv_freesMem(Mem) _Post_invalid_)
    PRTL_DYNAMIC_HASH_TABLE HashTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlInsertEntryHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _In_ __drv_aliasesMem PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
  _In_ ULONG_PTR Signature,
  _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context);

NTSYSAPI
BOOLEAN
NTAPI
RtlRemoveEntryHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _In_ PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
  _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context);

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlLookupEntryHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _In_ ULONG_PTR Signature,
  _Out_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context);

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlGetNextEntryHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _In_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context);

NTSYSAPI
BOOLEAN
NTAPI
RtlInitEnumerationHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlEnumerateEntryHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
VOID
NTAPI
RtlEndEnumerationHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
BOOLEAN
NTAPI
RtlInitWeakEnumerationHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlWeaklyEnumerateEntryHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
VOID
NTAPI
RtlEndWeakEnumerationHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
  _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
BOOLEAN
NTAPI
RtlExpandHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlContractHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable);

$endif (_NTDDK_)
$if (_NTIFS_)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToUTF8N(
  _Out_writes_bytes_to_(UTF8StringMaxByteCount, *UTF8StringActualByteCount) PCHAR UTF8StringDestination,
  _In_ ULONG UTF8StringMaxByteCount,
  _Out_ PULONG UTF8StringActualByteCount,
  _In_reads_bytes_(UnicodeStringByteCount) PCWCH UnicodeStringSource,
  _In_ ULONG UnicodeStringByteCount);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8ToUnicodeN(
  _Out_writes_bytes_to_(UnicodeStringMaxByteCount, *UnicodeStringActualByteCount) PWSTR UnicodeStringDestination,
  _In_ ULONG UnicodeStringMaxByteCount,
  _Out_ PULONG UnicodeStringActualByteCount,
  _In_reads_bytes_(UTF8StringByteCount) PCCH UTF8StringSource,
  _In_ ULONG UTF8StringByteCount);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlReplaceSidInSd(
  _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PSID OldSid,
  _In_ PSID NewSid,
  _Out_ ULONG *NumChanges);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateVirtualAccountSid(
  _In_ PCUNICODE_STRING Name,
  _In_ ULONG BaseSubAuthority,
  _Out_writes_bytes_(*SidLength) PSID Sid,
  _Inout_ PULONG SidLength);
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
  _In_ LONG SignedInteger)
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
  _In_ ULONG UnsignedInteger)
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
  _In_ LARGE_INTEGER LargeInteger,
  _In_ CCHAR ShiftCount)
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
  _In_ LARGE_INTEGER LargeInteger,
  _In_ CCHAR ShiftCount)
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
  _In_ ULARGE_INTEGER Dividend,
  _In_ ULONG Divisor,
  _Out_opt_ PULONG Remainder)
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
  _In_ LARGE_INTEGER Subtrahend)
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
  _In_ LARGE_INTEGER Minuend,
  _In_ LARGE_INTEGER Subtrahend)
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
  _In_ ULONG Multiplicand,
  _In_ ULONG Multiplier)
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
  _In_ LONG Multiplicand,
  _In_ LONG Multiplier)
{
  LARGE_INTEGER ret;
  ret.QuadPart = (LONGLONG)Multiplicand * (ULONGLONG)Multiplier;
  return ret;
}

_At_(AnsiString->Buffer, _Post_equal_to_(Buffer))
_At_(AnsiString->Length, _Post_equal_to_(0))
_At_(AnsiString->MaximumLength, _Post_equal_to_(BufferSize))
FORCEINLINE
VOID
RtlInitEmptyAnsiString(
  _Out_ PANSI_STRING AnsiString,
  _Pre_maybenull_ _Pre_readable_size_(BufferSize) __drv_aliasesMem PCHAR Buffer,
  _In_ USHORT BufferSize)
{
  AnsiString->Length = 0;
  AnsiString->MaximumLength = BufferSize;
  AnsiString->Buffer = Buffer;
}

_At_(UnicodeString->Buffer, _Post_equal_to_(Buffer))
_At_(UnicodeString->Length, _Post_equal_to_(0))
_At_(UnicodeString->MaximumLength, _Post_equal_to_(BufferSize))
FORCEINLINE
VOID
RtlInitEmptyUnicodeString(
    _Out_ PUNICODE_STRING UnicodeString,
    _Writable_bytes_(BufferSize)
    _When_(BufferSize != 0, _Notnull_)
    __drv_aliasesMem PWSTR Buffer,
    _In_ USHORT BufferSize)
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
  _In_ LARGE_INTEGER Multiplicand,
  _In_ LONG Multiplier)
{
  LARGE_INTEGER ret;
  ret.QuadPart = Multiplicand.QuadPart * Multiplier;
  return ret;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedLargeIntegerDivide(
  _In_ LARGE_INTEGER Dividend,
  _In_ ULONG Divisor,
  _Out_opt_ PULONG Remainder)
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
  _In_ LARGE_INTEGER Dividend,
  _In_ LARGE_INTEGER Divisor,
  _Out_opt_ PLARGE_INTEGER Remainder)
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
  _In_ LARGE_INTEGER Dividend,
  _In_ LARGE_INTEGER Divisor,
  _Out_opt_ PLARGE_INTEGER Remainder);
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
  _In_ LARGE_INTEGER Dividend,
  _In_ LARGE_INTEGER MagicDivisor,
  _In_ CCHAR ShiftCount)
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
  _In_ LARGE_INTEGER Addend1,
  _In_ LARGE_INTEGER Addend2)
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
  _In_ LARGE_INTEGER LargeInteger,
  _In_ CCHAR ShiftCount)
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
  _Out_writes_bytes_all_(Size) PVOID Pointer,
  _In_ SIZE_T Size)
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
_Must_inspect_result_
FORCEINLINE
BOOLEAN
RtlCheckBit(
  _In_ PRTL_BITMAP BitMapHeader,
  _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitPosition)
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
#define RtlUshortByteSwap(_x) _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))

#if defined(_MSC_VER) && !defined(__clang__)
# define __assert_annotationA(msg) __annotation(L"Debug", L"AssertFail", L ## msg)
# define __assert_annotationW(msg) __annotation(L"Debug", L"AssertFail", msg)
#else
# define __assert_annotationA(msg) \
    DbgPrint("Assertion failed at %s(%d): %s\n", __FILE__, __LINE__, msg)
# define __assert_annotationW(msg) \
    DbgPrint("Assertion failed at %s(%d): %S\n", __FILE__, __LINE__, msg)
#endif

#ifdef _PREFAST_
# define NT_ANALYSIS_ASSUME(_exp) _Analysis_assume_(_exp)
#elif DBG
# define NT_ANALYSIS_ASSUME(_exp) ((void)0)
#else
# define NT_ANALYSIS_ASSUME(_exp) __noop(_exp)
#endif

#define NT_ASSERT_ACTION(exp) \
   (NT_ANALYSIS_ASSUME(exp), \
   ((!(exp)) ? \
      (__assert_annotationA(#exp), \
       DbgRaiseAssertionFailure(), FALSE) : TRUE))

#define NT_ASSERTMSG_ACTION(msg, exp) \
   (NT_ANALYSIS_ASSUME(exp), \
   ((!(exp)) ? \
      (__assert_annotationA(msg), \
       DbgRaiseAssertionFailure(), FALSE) : TRUE))

#define NT_ASSERTMSGW_ACTION(msg, exp) \
   (NT_ANALYSIS_ASSUME(exp), \
   ((!(exp)) ? \
      (__assert_annotationW(msg), \
       DbgRaiseAssertionFailure(), FALSE) : TRUE))

#if DBG

#define RTL_VERIFY(exp) \
  ((!(exp)) ? \
    RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

#define RTL_VERIFYMSG(msg, exp) \
  ((!(exp)) ? \
    RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, (PCHAR)msg ), FALSE : TRUE)

#define RTL_SOFT_VERIFY(exp) \
  ((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #exp), FALSE : TRUE)

#define RTL_SOFT_VERIFYMSG(msg, exp) \
  ((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #exp, (msg)), FALSE : TRUE)

/* The ASSERTs must be cast to void to avoid warnings about unused results. */
#define ASSERT                 (void)RTL_VERIFY
#define ASSERTMSG              (void)RTL_VERIFYMSG
#define RTL_SOFT_ASSERT        (void)RTL_SOFT_VERIFY
#define RTL_SOFT_ASSERTMSG     (void)RTL_SOFT_VERIFYMSG

#define NT_VERIFY              NT_ASSERT_ACTION
#define NT_VERIFYMSG           NT_ASSERTMSG_ACTION
#define NT_VERIFYMSGW          NT_ASSERTMSGW_ACTION

#define NT_ASSERT_ASSUME       (void)NT_ASSERT_ACTION
#define NT_ASSERTMSG_ASSUME    (void)NT_ASSERTMSG_ACTION
#define NT_ASSERTMSGW_ASSUME   (void)NT_ASSERTMSGW_ACTION

#define NT_ASSERT_NOASSUME     (void)NT_ASSERT_ACTION
#define NT_ASSERTMSG_NOASSUME  (void)NT_ASSERTMSG_ACTION
#define NT_ASSERTMSGW_NOASSUME (void)NT_ASSERTMSGW_ACTION

#else /* !DBG */

#define ASSERT(exp)                  ((void)0)
#define ASSERTMSG(msg, exp)          ((void)0)

#define RTL_SOFT_ASSERT(exp)         ((void)0)
#define RTL_SOFT_ASSERTMSG(msg, exp) ((void)0)

#define RTL_VERIFY(exp)              ((exp) ? TRUE : FALSE)
#define RTL_VERIFYMSG(msg, exp)      ((exp) ? TRUE : FALSE)

#define RTL_SOFT_VERIFY(exp)         ((exp) ? TRUE : FALSE)
#define RTL_SOFT_VERIFYMSG(msg, exp) ((exp) ? TRUE : FALSE)

#define NT_VERIFY(exp)          (NT_ANALYSIS_ASSUME(exp), ((exp) ? TRUE : FALSE))
#define NT_VERIFYMSG(msg, exp)  (NT_ANALYSIS_ASSUME(exp), ((exp) ? TRUE : FALSE))
#define NT_VERIFYMSGW(msg, exp) (NT_ANALYSIS_ASSUME(exp), ((exp) ? TRUE : FALSE))

#define NT_ASSERT_ASSUME(exp)          (NT_ANALYSIS_ASSUME(exp), (void)0)
#define NT_ASSERTMSG_ASSUME(msg, exp)  (NT_ANALYSIS_ASSUME(exp), (void)0)
#define NT_ASSERTMSGW_ASSUME(msg, exp) (NT_ANALYSIS_ASSUME(exp), (void)0)

#define NT_ASSERT_NOASSUME(exp)          ((void)0)
#define NT_ASSERTMSG_NOASSUME(msg, exp)  ((void)0)
#define NT_ASSERTMSGW_NOASSUME(msg, exp) ((void)0)

#endif /* DBG */

#define NT_FRE_ASSERT     (void)NT_ASSERT_ACTION
#define NT_FRE_ASSERTMSG  (void)NT_ASSERTMSG_ACTION
#define NT_FRE_ASSERTMSGW (void)NT_ASSERTMSGW_ACTION

#ifdef NT_ASSERT_ALWAYS_ASSUMES
# define NT_ASSERT NT_ASSERT_ASSUME
# define NT_ASSERTMSG NT_ASSERTMSG_ASSUME
# define NT_ASSERTMSGW NT_ASSERTMSGW_ASSUME
#else
# define NT_ASSERT NT_ASSERT_NOASSUME
# define NT_ASSERTMSG NT_ASSERTMSG_NOASSUME
# define NT_ASSERTMSGW NT_ASSERTMSGW_NOASSUME
#endif /* NT_ASSERT_ALWAYS_ASSUMES */

#if defined(_MSC_VER) && (defined(__REACTOS__) || defined(ASSERT_ALWAYS_NT_ASSERT)) && !defined(_BLDR_)
#undef ASSERT
#define ASSERT NT_ASSERT
#undef ASSERTMSG
#define ASSERTMSG NT_ASSERTMSG
#undef ASSERTMSGW
#define ASSERTMSGW NT_ASSERTMSGW
#undef RTL_VERIFY
#define RTL_VERIFY NT_VERIFY
#endif

#define InitializeListHead32(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))

#if !defined(_WINBASE_)

#if defined(_WIN64) && !defined(_NTSYSTEM_) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || !defined(_NTOSP_))

NTKERNELAPI
VOID
InitializeSListHead(
  _Out_ PSLIST_HEADER SListHead);

#else /* defined(_WIN64) &&  ... */

/* HACK */
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseStatus(
  _In_ NTSTATUS Status);

FORCEINLINE
VOID
InitializeSListHead(
  _Out_ PSLIST_HEADER SListHead)
{
#if defined(_WIN64)
    if (((ULONG_PTR)SListHead & 0xf) != 0) {
        ExRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }
#if defined(_IA64_)
    SListHead->Region = (ULONG_PTR)SListHead & VRN_MASK;
#else
    SListHead->Region = 0;
#endif /* _IA64_ */
#endif /* _WIN64 */
    SListHead->Alignment = 0;
}

#endif /* defined(_WIN64) &&  ... */

#ifdef _X86_

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(
  _Inout_ PSLIST_HEADER SListHead,
  _Inout_ __drv_aliasesMem PSLIST_ENTRY SListEntry);

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList(
  _Inout_ PSLIST_HEADER SListHead);

#define InterlockedFlushSList(SListHead) \
    ExInterlockedFlushSList(SListHead)

#else /* !_X86_ */

#define InterlockedPushEntrySList(SListHead, SListEntry) \
    ExpInterlockedPushEntrySList(SListHead, SListEntry)

#define InterlockedPopEntrySList(SListHead) \
    ExpInterlockedPopEntrySList(SListHead)

#define InterlockedFlushSList(SListHead) \
    ExpInterlockedFlushSList(SListHead)

#endif /* _X86_ */

#define QueryDepthSList(SListHead) \
    ExQueryDepthSList(SListHead)

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
  _In_ ULONG Version);

BOOLEAN
RTLVERLIB_DDI(RtlIsServicePackVersionInstalled)(
  _In_ ULONG Version);

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
  _In_ LONG Val)
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
  _In_ ULONG Val)
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
  _Out_ PVOID *CallersAddress,
  _Out_ PVOID *CallersCaller);
#endif
#endif

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

#if (NTDDI_VERSION >= NTDDI_WIN7)

FORCEINLINE
VOID
NTAPI
RtlInitHashTableContext(
  _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context)
{
  Context->ChainHead = NULL;
  Context->PrevLinkage = NULL;
}

FORCEINLINE
VOID
NTAPI
RtlInitHashTableContextFromEnumerator(
  _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context,
  _In_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator)
{
  Context->ChainHead = Enumerator->ChainHead;
  Context->PrevLinkage = Enumerator->HashEntry.Linkage.Blink;
}

FORCEINLINE
VOID
NTAPI
RtlReleaseHashTableContext(
  _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context)
{
  UNREFERENCED_PARAMETER(Context);
  return;
}

FORCEINLINE
ULONG
NTAPI
RtlTotalBucketsHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->TableSize;
}

FORCEINLINE
ULONG
NTAPI
RtlNonEmptyBucketsHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI
RtlEmptyBucketsHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->TableSize - HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI
RtlTotalEntriesHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NumEntries;
}

FORCEINLINE
ULONG
NTAPI
RtlActiveEnumeratorsHashTable(
  _In_ PRTL_DYNAMIC_HASH_TABLE HashTable)
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
