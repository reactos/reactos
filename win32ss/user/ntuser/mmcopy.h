#pragma once


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
_MmCopyFromCaller(
    _Out_writes_bytes_all_(Bytes) PVOID Target,
    _In_reads_bytes_(Bytes) PVOID Source,
    _In_ UINT Bytes);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
_MmCopyToCaller(
    _Out_writes_bytes_all_(Bytes) PVOID Target,
    _In_reads_bytes_(Bytes) PVOID Source,
    _In_ UINT Bytes);

#define MmCopyFromCaller(x,y,z) _MmCopyFromCaller((PCHAR)(x),(PCHAR)(y),(UINT)(z))
#define MmCopyToCaller(x,y,z) _MmCopyToCaller((PCHAR)(x),(PCHAR)(y),(UINT)(z))
