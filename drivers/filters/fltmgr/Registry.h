#pragma once


NTSTATUS
FltpOpenFilterServicesKey(
    _In_ PFLT_FILTER Filter,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SubKey,
    _Out_ PHANDLE Handle
);

NTSTATUS
FltpReadRegistryValue(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_opt_ ULONG Type,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG BytesRequired
);
