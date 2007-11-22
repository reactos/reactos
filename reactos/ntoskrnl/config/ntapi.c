/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmapi.c
 * PURPOSE:         Configuration Manager - Internal Registry APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
NtDeleteKey(IN HANDLE KeyHandle)
{
    PKEY_OBJECT KeyObject;
    NTSTATUS Status;
    REG_DELETE_KEY_INFORMATION DeleteKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();

    DPRINT("NtDeleteKey(KH 0x%p)\n", KeyHandle);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       DELETE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed with Status = 0x%08X\n");
        return Status;
    }

    /* Setup the callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    DeleteKeyInfo.Object = (PVOID)KeyObject;
    Status = CmiCallRegisteredCallbacks(RegNtPreDeleteKey, &DeleteKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmDeleteKey(KeyObject->KeyControlBlock);

        /* Remove the keep-alive reference */
        ObDereferenceObject(KeyObject);
        if (KeyObject->KeyControlBlock->KeyHive !=
            KeyObject->KeyControlBlock->ParentKcb->KeyHive)
        {
            /* Dereference again */
            ObDereferenceObject(KeyObject);
        }

        /* Do post callback */
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostDeleteKey, &PostOperationInfo);
    }

    /* Dereference the object */
    ObDereferenceObject(KeyObject);
    return Status;
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
    NTSTATUS Status;
    PKEY_OBJECT KeyObject;
    REG_ENUMERATE_KEY_INFORMATION EnumerateKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();

    DPRINT("NtEnumerateKey() KH 0x%x, Index 0x%x, KIC %d, Length %d\n",
        KeyHandle, Index, KeyInformationClass, Length);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_ENUMERATE_SUB_KEYS,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed with Status = 0x%08X\n");
        return Status;
    }

    /* Setup the callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    EnumerateKeyInfo.Object = (PVOID)KeyObject;
    EnumerateKeyInfo.Index = Index;
    EnumerateKeyInfo.KeyInformationClass = KeyInformationClass;
    EnumerateKeyInfo.Length = Length;
    EnumerateKeyInfo.ResultLength = ResultLength;

    /* Do the callback */
    Status = CmiCallRegisteredCallbacks(RegNtPreEnumerateKey, &EnumerateKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmEnumerateKey(KeyObject->KeyControlBlock,
                                Index,
                                KeyInformationClass,
                                KeyInformation,
                                Length,
                                ResultLength);

        /* Do the post callback */
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostEnumerateKey, &PostOperationInfo);
    }

    /* Dereference and return status */
    ObDereferenceObject(KeyObject);
    return Status;
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
    NTSTATUS Status;
    PKEY_OBJECT KeyObject;
    REG_ENUMERATE_VALUE_KEY_INFORMATION EnumerateValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();

    DPRINT("NtEnumerateValueKey() KH 0x%x, Index 0x%x, KVIC %d, Length %d\n",
        KeyHandle, Index, KeyValueInformationClass, Length);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_QUERY_VALUE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed with Status = 0x%08X\n");
        return Status;
    }

    /* Setup the callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    EnumerateValueKeyInfo.Object = (PVOID)KeyObject;
    EnumerateValueKeyInfo.Index = Index;
    EnumerateValueKeyInfo.KeyValueInformationClass = KeyValueInformationClass;
    EnumerateValueKeyInfo.KeyValueInformation = KeyValueInformation;
    EnumerateValueKeyInfo.Length = Length;
    EnumerateValueKeyInfo.ResultLength = ResultLength;

    /* Do the callback */
    Status = CmiCallRegisteredCallbacks(RegNtPreEnumerateValueKey,
                                        &EnumerateValueKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmEnumerateValueKey(KeyObject->KeyControlBlock,
                                     Index,
                                     KeyValueInformationClass,
                                     KeyValueInformation,
                                     Length,
                                     ResultLength);

        /* Do the post callback */
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostEnumerateValueKey, &PostOperationInfo);
    }

    ObDereferenceObject(KeyObject);
    return Status;
}

NTSTATUS
NTAPI
NtQueryKey(IN HANDLE KeyHandle,
           IN KEY_INFORMATION_CLASS KeyInformationClass,
           OUT PVOID KeyInformation,
           IN ULONG Length,
           OUT PULONG ResultLength)
{
    NTSTATUS Status;
    PKEY_OBJECT KeyObject;
    REG_QUERY_KEY_INFORMATION QueryKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();

    DPRINT("NtQueryKey() KH 0x%x, KIC %d, Length %d\n",
        KeyHandle, KeyInformationClass, Length);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       (KeyInformationClass !=
                                        KeyNameInformation) ?
                                       KEY_QUERY_VALUE : 0,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed with Status = 0x%08X\n");
        return Status;
    }

    /* Setup the callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    QueryKeyInfo.Object = (PVOID)KeyObject;
    QueryKeyInfo.KeyInformationClass = KeyInformationClass;
    QueryKeyInfo.KeyInformation = KeyInformation;
    QueryKeyInfo.Length = Length;
    QueryKeyInfo.ResultLength = ResultLength;

    /* Do the callback */
    Status = CmiCallRegisteredCallbacks(RegNtPreQueryKey, &QueryKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmQueryKey(KeyObject->KeyControlBlock,
                            KeyInformationClass,
                            KeyInformation,
                            Length,
                            ResultLength);

        /* Do the post callback */
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostQueryKey, &PostOperationInfo);
    }

    /* Dereference and return status */
    ObDereferenceObject(KeyObject);
    return Status;
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
    NTSTATUS Status;
    PKEY_OBJECT KeyObject;
    REG_QUERY_VALUE_KEY_INFORMATION QueryValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();

    DPRINT("NtQueryValueKey() KH 0x%x, VN '%wZ', KVIC %d, Length %d\n",
        KeyHandle, ValueName, KeyValueInformationClass, Length);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_QUERY_VALUE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed with Status = 0x%08X\n");
        return Status;
    }

    /* Setup the callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    QueryValueKeyInfo.Object = (PVOID)KeyObject;
    QueryValueKeyInfo.ValueName = ValueName;
    QueryValueKeyInfo.KeyValueInformationClass = KeyValueInformationClass;
    QueryValueKeyInfo.Length = Length;
    QueryValueKeyInfo.ResultLength = ResultLength;

    /* Do the callback */
    Status = CmiCallRegisteredCallbacks(RegNtPreQueryValueKey, &QueryValueKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmQueryValueKey(KeyObject->KeyControlBlock,
                                 *ValueName,
                                 KeyValueInformationClass,
                                 KeyValueInformation,
                                 Length,
                                 ResultLength);

        /* Do the post callback */
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostQueryValueKey, &PostOperationInfo);
    }

    DPRINT("NtQueryValueKey() returning 0x%08X\n", Status);

    /* Dereference and return status */
    ObDereferenceObject(KeyObject);
    return Status;
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
    NTSTATUS Status;
    PKEY_OBJECT KeyObject;
    REG_SET_VALUE_KEY_INFORMATION SetValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();

    DPRINT("NtSetValueKey() KH 0x%x, VN '%wZ', TI %x, T %d, DS %d\n",
        KeyHandle, ValueName, TitleIndex, Type, DataSize);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_SET_VALUE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObReferenceObjectByHandle() failed with Status = 0x%08X\n", Status);
        return Status;
    }

    /* Setup callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    SetValueKeyInfo.Object = (PVOID)KeyObject;
    SetValueKeyInfo.ValueName = ValueName;
    SetValueKeyInfo.TitleIndex = TitleIndex;
    SetValueKeyInfo.Type = Type;
    SetValueKeyInfo.Data = Data;
    SetValueKeyInfo.DataSize = DataSize;

    /* Do the callback */
    Status = CmiCallRegisteredCallbacks(RegNtPreSetValueKey, &SetValueKeyInfo);
    if (NT_SUCCESS(Status))
    {        
        /* Call the internal API */
        Status = CmSetValueKey(KeyObject->KeyControlBlock,
                               ValueName,
                               Type,
                               Data,
                               DataSize);
    }

    /* Do the post-callback and de-reference the key object */
    PostOperationInfo.Status = Status;
    CmiCallRegisteredCallbacks(RegNtPostSetValueKey, &PostOperationInfo);
    ObDereferenceObject(KeyObject);

    /* Synchronize the hives and return */
    CmpLazyFlush();
    return Status;
}

NTSTATUS
NTAPI
NtDeleteValueKey(IN HANDLE KeyHandle,
                 IN PUNICODE_STRING ValueName)
{
    PKEY_OBJECT KeyObject;
    NTSTATUS Status;
    REG_DELETE_VALUE_KEY_INFORMATION DeleteValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_SET_VALUE,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed with Status = 0x%08X\n");
        return Status;
    }

    /* Do the callback */
    DeleteValueKeyInfo.Object = (PVOID)KeyObject;
    DeleteValueKeyInfo.ValueName = ValueName;
    Status = CmiCallRegisteredCallbacks(RegNtPreDeleteValueKey,
                                        &DeleteValueKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmDeleteValueKey(KeyObject->KeyControlBlock, *ValueName);

        /* Do the post callback */
        PostOperationInfo.Object = (PVOID)KeyObject;
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostDeleteValueKey,
                                   &PostOperationInfo);
    }

    /* Dereference the key body and synchronize the hives */
    ObDereferenceObject(KeyObject);
    CmpLazyFlush();
    return Status;
}

NTSTATUS
NTAPI
NtFlushKey(IN HANDLE KeyHandle)
{
    NTSTATUS Status;
    PKEY_OBJECT KeyObject;
    PAGED_CODE();
    
    /* Get the key object */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       0,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;
    
    /* Lock the registry */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);
    
    /* Make sure KCB isn't deleted */
    if (KeyObject->KeyControlBlock->Delete)
    {
        /* Fail */
        Status = STATUS_KEY_DELETED;
    }
    else
    {
        /* Call the internal API */
        Status = CmFlushKey(KeyObject->KeyControlBlock, FALSE);
    }
    
    /* Release the lock */
    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();
    
    /* Dereference the object and return status */
    ObDereferenceObject(KeyObject);
    return Status;
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
NtLoadKey2(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
           IN POBJECT_ATTRIBUTES FileObjectAttributes,
           IN ULONG Flags)
{
    return NtLoadKeyEx(KeyObjectAttributes, FileObjectAttributes, Flags, NULL);
}

NTSTATUS
NTAPI
CmLoadKey(IN POBJECT_ATTRIBUTES TargetKey,
          IN POBJECT_ATTRIBUTES SourceFile,
          IN ULONG Flags,
          IN PKEY_OBJECT KeyBody);

NTSTATUS
NTAPI
NtLoadKeyEx(IN POBJECT_ATTRIBUTES TargetKey,
            IN POBJECT_ATTRIBUTES SourceFile,
            IN ULONG Flags,
            IN HANDLE TrustClassKey)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKEY_OBJECT KeyBody = NULL;
    PAGED_CODE();

    /* Validate flags */
    if (Flags & ~REG_NO_LAZY_FLUSH) return STATUS_INVALID_PARAMETER;
    
    /* Validate privilege */
    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, PreviousMode))
    {
        /* Fail */
        DPRINT1("Restore Privilege missing!\n");
        //return STATUS_PRIVILEGE_NOT_HELD;
    }
    
    /* Block APCs */
    KeEnterCriticalRegion();
    
    /* Check if we have a trust class */
    if (TrustClassKey)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(TrustClassKey,
                                           0,
                                           CmpKeyObjectType,
                                           PreviousMode,
                                           (PVOID *)&KeyBody,
                                           NULL);
    }
    
    /* Call the internal API */
    Status = CmLoadKey(TargetKey, SourceFile, Flags, KeyBody);
    
    /* Dereference the trust key, if any */
    if (KeyBody) ObDereferenceObject(KeyBody);
    
    /* Bring back APCs */
    KeLeaveCriticalRegion();
    
    /* Return status */
    return Status;
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
