/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    cmfuncs.h

Abstract:

    Function definitions for the Configuration Manager.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _CMFUNCS_H
#define _CMFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <cmtypes.h>

//
// Native calls
//
NTSTATUS
NTAPI
NtCompactKeys(
    _In_ ULONG Count,
    _In_reads_(Count) PHANDLE KeyArray
);

NTSTATUS
NTAPI
NtCompressKey(
    _In_ HANDLE Key
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG TitleIndex,
    _In_opt_ PUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteKey(
    _In_ HANDLE KeyHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Index,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_bytecap_(Length) PVOID KeyInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
NtEnumerateValueKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Index,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushKey(
    _In_ HANDLE KeyHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetPlugPlayEvent(
    _In_ ULONG Reserved1,
    _In_ ULONG Reserved2,
    _Out_ PPLUGPLAY_EVENT_BLOCK Buffer,
    _In_ ULONG BufferSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeRegistry(
    _In_ USHORT Flag
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey(
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes,
    _In_ POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey2(
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes,
    _In_ POBJECT_ATTRIBUTES FileObjectAttributes,
    _In_ ULONG Flags
);

NTSTATUS
NTAPI
NtLoadKeyEx(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ POBJECT_ATTRIBUTES SourceFile,
    _In_ ULONG Flags,
    _In_ HANDLE TrustClassKey
);

NTSTATUS
NTAPI
NtLockProductActivationKeys(
    _In_ PULONG pPrivateVer,
    _In_ PULONG pSafeMode
);

NTSTATUS
NTAPI
NtLockRegistryKey(
    _In_ HANDLE KeyHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN Asynchroneous,
    _Out_bytecap_(Length) PVOID ChangeBuffer,
    _In_ ULONG Length,
    _In_ BOOLEAN WatchSubtree
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeMultipleKeys(
    _In_ HANDLE MasterKeyHandle,
    _In_opt_ ULONG Count,
    _In_reads_opt_(Count) OBJECT_ATTRIBUTES SubordinateObjects[],
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN WatchTree,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Asynchronous
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPlugPlayControl(
    _In_ PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    _Inout_ PVOID Buffer,
    _In_ ULONG BufferSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_bytecap_(Length) PVOID KeyInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    _In_ HANDLE KeyHandle,
    _Inout_updates_(EntryCount) PKEY_VALUE_ENTRY ValueEntries,
    _In_ ULONG EntryCount,
    _Out_writes_bytes_(*BufferLength) PVOID ValueBuffer,
    _Inout_ PULONG BufferLength,
    _Out_opt_ PULONG RequiredBufferLength
);

NTSTATUS
NTAPI
NtQueryOpenSubKeys(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _Out_ PULONG HandleCount
);

NTSTATUS
NTAPI
NtQueryOpenSubKeysEx(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG BufferLength,
    _In_ PVOID Buffer,
    _In_ PULONG RequiredSize
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
NtQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING NewName
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplaceKey(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE Key,
    _In_ POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRestoreKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG RestoreFlags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKeyEx(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
);


NTSTATUS
NTAPI
NtSaveMergedKeys(
    _In_ HANDLE HighPrecedenceKeyHandle,
    _In_ HANDLE LowPrecedenceKeyHandle,
    _In_ HANDLE FileHandle
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationKey(
    _In_ HANDLE KeyHandle,
    _In_ _Strict_type_match_
        KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    _In_reads_bytes_(KeySetInformationLength) PVOID KeySetInformation,
    _In_ ULONG KeySetInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_opt_ ULONG TitleIndex,
    _In_ ULONG Type,
    _In_ PVOID Data,
    _In_ ULONG DataSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey(
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSTATUS
NTAPI
NtUnloadKey2(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG Flags
);

NTSTATUS
NTAPI
NtUnloadKeyEx(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ HANDLE Event
);

#ifdef NTOS_MODE_USER
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG TitleIndex,
    _In_opt_ PUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
    _In_ HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Index,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_bytecap_(Length) PVOID KeyInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Index,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
    _In_ HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwGetPlugPlayEvent(
    _In_ ULONG Reserved1,
    _In_ ULONG Reserved2,
    _Out_bytecap_(BufferSize) PPLUGPLAY_EVENT_BLOCK Buffer,
    _In_ ULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey(
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes,
    _In_ POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey2(
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes,
    _In_ POBJECT_ATTRIBUTES FileObjectAttributes,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN Asynchroneous,
    _Out_bytecap_(Length) PVOID ChangeBuffer,
    _In_ ULONG Length,
    _In_ BOOLEAN WatchSubtree
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPlugPlayControl(
    _In_ PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    _Inout_bytecap_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_bytecap_(Length) PVOID KeyInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
    _In_ HANDLE KeyHandle,
    _Inout_ PKEY_VALUE_ENTRY ValueList,
    _In_ ULONG NumberOfValues,
    _Out_bytecap_(*Length) PVOID Buffer,
    _Inout_ PULONG Length,
    _Out_ PULONG ReturnLength
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(Length == 0, _Post_satisfies_(return < 0))
_When_(Length > 0, _Post_satisfies_(return <= 0))
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE Key,
    _In_ POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG RestoreFlags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKeyEx(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_SET_INFORMATION_CLASS KeyInformationClass,
    _In_bytecount_(KeyInformationLength) PVOID KeyInformation,
    _In_ ULONG KeyInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_opt_ ULONG TitleIndex,
    _In_ ULONG Type,
    _In_bytecount_(DataSize) PVOID Data,
    _In_ ULONG DataSize
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwInitializeRegistry(
    _In_ USHORT Flag
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey(
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes
);

#endif
