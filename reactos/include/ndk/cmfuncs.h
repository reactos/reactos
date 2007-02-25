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

//
// Native calls
//
NTSTATUS
NTAPI
NtCompactKeys(
    IN ULONG Count,
    IN PHANDLE KeyArray
);

NTSTATUS
NTAPI
NtCompressKey(
    IN HANDLE Key
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    IN PULONG Disposition OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteKey(
    IN HANDLE KeyHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushKey(
    IN HANDLE KeyHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetPlugPlayEvent(
    IN ULONG Reserved1,
    IN ULONG Reserved2,
    OUT PPLUGPLAY_EVENT_BLOCK Buffer,
    IN ULONG BufferSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

NTSTATUS
NTAPI
NtLoadKeyEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags,
    IN HANDLE TrustClassKey,
    IN HANDLE Event,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE RootHandle
);

NTSTATUS
NTAPI
NtLockProductActivationKeys(
    IN PULONG pPrivateVer,
    IN PULONG pSafeMode
);

NTSTATUS
NTAPI
NtLockRegistryKey(
    IN HANDLE KeyHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN Asynchroneous,
    OUT PVOID ChangeBuffer,
    IN ULONG Length,
    IN BOOLEAN WatchSubtree
);

NTSTATUS
NTAPI
NtNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,
    IN ULONG Count,
    IN POBJECT_ATTRIBUTES SlaveObjects,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN Asynchronous
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPlugPlayControl(
    IN PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN OUT PKEY_VALUE_ENTRY ValueList,
    IN ULONG NumberOfValues,
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN ULONG HandleCount
);

NTSTATUS
NTAPI
NtQueryOpenSubKeysEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN ULONG BufferLength,
    IN PVOID Buffer,
    IN PULONG RequiredSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG RestoreFlags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);


NTSTATUS
NTAPI
NtSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSTATUS
NTAPI
NtUnloadKey2(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN ULONG Flags
);

NTSTATUS
NTAPI
NtUnloadKeyEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN HANDLE Event
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    IN PULONG Disposition OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
    IN HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
    IN HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwGetPlugPlayEvent(
    IN ULONG Reserved1,
    IN ULONG Reserved2,
    OUT PPLUGPLAY_EVENT_BLOCK Buffer,
    IN ULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN Asynchroneous,
    OUT PVOID ChangeBuffer,
    IN ULONG Length,
    IN BOOLEAN WatchSubtree
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPlugPlayControl(
    IN PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN OUT PKEY_VALUE_ENTRY ValueList,
    IN ULONG NumberOfValues,
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG RestoreFlags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

#endif
