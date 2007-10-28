/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/ntapi.c
 * PURPOSE:         Native API Routines for Registry Access.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
NtCreateKey(OUT PHANDLE KeyHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes,
            IN ULONG TitleIndex,
            IN PUNICODE_STRING Class,
            IN ULONG CreateOptions,
            OUT PULONG Disposition)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED; 
}

NTSTATUS
NTAPI
NtOpenKey(OUT PHANDLE KeyHandle,
          IN ACCESS_MASK DesiredAccess,
          IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;  
}

NTSTATUS
NTAPI
NtFlushKey(IN HANDLE KeyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtLoadKey2 (IN POBJECT_ATTRIBUTES KeyObjectAttributes,
            IN POBJECT_ATTRIBUTES FileObjectAttributes,
            IN ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtInitializeRegistry(IN USHORT Flag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
NtDeleteKey(IN HANDLE KeyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtEnumerateKey(IN HANDLE KeyHandle,
               IN ULONG Index,
               IN KEY_INFORMATION_CLASS KeyInformationClass,
               OUT PVOID KeyInformation,
               IN ULONG Length,
               OUT PULONG ResultLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtEnumerateValueKey(IN HANDLE KeyHandle,
                    IN ULONG Index,
                    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                    OUT PVOID KeyValueInformation,
                    IN ULONG Length,
                    OUT PULONG ResultLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED; 
}

NTSTATUS
NTAPI
NtQueryKey(IN HANDLE KeyHandle,
           IN KEY_INFORMATION_CLASS KeyInformationClass,
           OUT PVOID KeyInformation,
           IN ULONG Length,
           OUT PULONG ResultLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQueryValueKey(IN HANDLE KeyHandle,
                IN PUNICODE_STRING ValueName,
                IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                OUT PVOID KeyValueInformation,
                IN ULONG Length,
                OUT PULONG ResultLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetValueKey(IN HANDLE KeyHandle,
              IN PUNICODE_STRING ValueName,
              IN ULONG TitleIndex,
              IN ULONG Type,
              IN PVOID Data,
              IN ULONG DataSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED; 
}

NTSTATUS
NTAPI
NtDeleteValueKey(IN HANDLE KeyHandle,
                 IN PUNICODE_STRING ValueName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtCompactKeys(IN ULONG Count,
              IN PHANDLE KeyArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtCompressKey(IN HANDLE Key)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtLoadKey(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
          IN POBJECT_ATTRIBUTES FileObjectAttributes)
{
    return NtLoadKey2(KeyObjectAttributes, FileObjectAttributes, 0);
}

NTSTATUS
NTAPI
NtLoadKeyEx(IN POBJECT_ATTRIBUTES TargetKey,
            IN POBJECT_ATTRIBUTES SourceFile,
            IN ULONG Flags,
            IN HANDLE TrustClassKey,
            IN HANDLE Event,
            IN ACCESS_MASK DesiredAccess,
            OUT PHANDLE RootHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtLockProductActivationKeys(IN PULONG pPrivateVer,
                            IN PULONG pSafeMode)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtLockRegistryKey(IN HANDLE KeyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtNotifyChangeMultipleKeys(IN HANDLE MasterKeyHandle,
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
                           IN BOOLEAN Asynchronous)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtNotifyChangeKey(IN HANDLE KeyHandle,
                  IN HANDLE Event,
                  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                  IN PVOID ApcContext OPTIONAL,
                  OUT PIO_STATUS_BLOCK IoStatusBlock,
                  IN ULONG CompletionFilter,
                  IN BOOLEAN WatchTree,
                  OUT PVOID Buffer,
                  IN ULONG Length,
                  IN BOOLEAN Asynchronous)
{
     return NtNotifyChangeMultipleKeys(KeyHandle,
                                       0,
                                       NULL,
                                       Event,
                                       ApcRoutine,
                                       ApcContext,
                                       IoStatusBlock,
                                       CompletionFilter,
                                       WatchTree,
                                       Buffer,
                                       Length,
                                       Asynchronous);
}

NTSTATUS
NTAPI
NtQueryMultipleValueKey(IN HANDLE KeyHandle,
                        IN OUT PKEY_VALUE_ENTRY ValueList,
                        IN ULONG NumberOfValues,
                        OUT PVOID Buffer,
                        IN OUT PULONG Length,
                        OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQueryOpenSubKeys(IN POBJECT_ATTRIBUTES TargetKey,
                   IN ULONG HandleCount)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQueryOpenSubKeysEx(IN POBJECT_ATTRIBUTES TargetKey,
                     IN ULONG BufferLength,
                     IN PVOID Buffer,
                     IN PULONG RequiredSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtReplaceKey(IN POBJECT_ATTRIBUTES ObjectAttributes,
             IN HANDLE Key,
             IN POBJECT_ATTRIBUTES ReplacedObjectAttributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtRestoreKey(IN HANDLE KeyHandle,
             IN HANDLE FileHandle,
             IN ULONG RestoreFlags)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSaveKey(IN HANDLE KeyHandle,
          IN HANDLE FileHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSaveKeyEx(IN HANDLE KeyHandle,
            IN HANDLE FileHandle,
            IN ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSaveMergedKeys(IN HANDLE HighPrecedenceKeyHandle,
                 IN HANDLE LowPrecedenceKeyHandle,
                 IN HANDLE FileHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetInformationKey(IN HANDLE KeyHandle,
                    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
                    IN PVOID KeyInformation,
                    IN ULONG KeyInformationLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtUnloadKey(IN POBJECT_ATTRIBUTES KeyObjectAttributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtUnloadKey2(IN POBJECT_ATTRIBUTES TargetKey,
             IN ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtUnloadKeyEx(IN POBJECT_ATTRIBUTES TargetKey,
              IN HANDLE Event)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
