#pragma once

#ifndef RTL_CONSTANT_STRING
#define RTL_CONSTANT_STRING(s)  { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#endif
#define STATUS_NOT_SUPPORTED                    ((NTSTATUS)0xC00000BB)

extern _IRQL_requires_max_(PASSIVE_LEVEL)
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

extern NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping
);
