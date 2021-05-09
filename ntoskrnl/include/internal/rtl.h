#pragma once

#ifdef _WIN64
VOID
NTAPI
RtlInitializeSListHead(IN PSLIST_HEADER ListHead);
#define InitializeSListHead RtlInitializeSListHead
#endif

NTSTATUS
NTAPI
RtlQueryAtomListInAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN ULONG MaxAtomCount,
    OUT ULONG *AtomCount,
    OUT RTL_ATOM *AtomList
);

VOID
NTAPI
RtlInitializeRangeListPackage(
    VOID
);

#define RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END        1
#define RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET 2
#define RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE    4
NTSTATUS
NTAPI
RtlFindCharInUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING SearchString,
    _In_ PCUNICODE_STRING MatchString,
    _Out_ PUSHORT Position
);

_IRQL_requires_max_(APC_LEVEL)
ULONG
NTAPI
RtlRosGetAppcompatVersion(VOID);

/* EOF */
