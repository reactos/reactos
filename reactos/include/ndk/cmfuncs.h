/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    cmfuncs.h

Abstract:

    Function definitions for the Configuration Manager.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

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
NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    IN PULONG Disposition OPTIONAL
);

NTSTATUS
NTAPI
NtDeleteKey(
    IN HANDLE KeyHandle
);

NTSTATUS
NTAPI
NtDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

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

NTSTATUS
NTAPI
NtFlushKey(
    IN HANDLE KeyHandle
);

NTSTATUS
NTAPI
NtGetPlugPlayEvent(
    IN ULONG Reserved1,
    IN ULONG Reserved2,
    OUT PPLUGPLAY_EVENT_BLOCK Buffer,
    IN ULONG BufferSize
);

NTSTATUS
NTAPI
NtInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSTATUS
NTAPI
NtLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSTATUS
NTAPI
NtLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

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
NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtPlugPlayControl(
    IN PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize
);

NTSTATUS
NTAPI
NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

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
NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSTATUS
NTAPI
NtRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG RestoreFlags
);

NTSTATUS
NTAPI
NtSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSTATUS
NTAPI
NtSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);

NTSTATUS
NTAPI
NtSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

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

NTSTATUS
NTAPI
NtUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

#ifdef NTOS_MODE_USER
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


NTSTATUS
NTAPI
ZwDeleteKey(
    IN HANDLE KeyHandle
);

NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

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

NTSTATUS
NTAPI
ZwFlushKey(
    IN HANDLE KeyHandle
);

NTSTATUS
NTAPI
ZwGetPlugPlayEvent(
    IN ULONG Reserved1,
    IN ULONG Reserved2,
    OUT PPLUGPLAY_EVENT_BLOCK Buffer,
    IN ULONG BufferSize
);

NTSTATUS
NTAPI
ZwInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSTATUS
NTAPI
ZwLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSTATUS
NTAPI
ZwLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

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

NTSTATUS
NTAPI
ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
ZwPlugPlayControl(
    IN PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize
);

NTSTATUS
NTAPI
ZwQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

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

NTSTATUS
NTAPI
ZwReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSTATUS
NTAPI
ZwRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG RestoreFlags
);

NTSTATUS
NTAPI
ZwSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSTATUS
NTAPI
ZwSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);

NTSTATUS
NTAPI
ZwSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

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

NTSTATUS
NTAPI
ZwUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

#endif
