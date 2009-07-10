/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmapi.c
 * PURPOSE:         Configuration Manager - Internal Registry APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

BOOLEAN CmBootAcceptFirstTime = TRUE;
BOOLEAN CmFirstTime = TRUE;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
NtCreateKey(OUT PHANDLE KeyHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes,
            IN ULONG TitleIndex,
            IN PUNICODE_STRING Class OPTIONAL,
            IN ULONG CreateOptions,
            OUT PULONG Disposition OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    CM_PARSE_CONTEXT ParseContext = {0};
    HANDLE Handle;
    PAGED_CODE();
    DPRINT("NtCreateKey(OB name %wZ)\n", ObjectAttributes->ObjectName);

    /* Check for user-mode caller */
    if (PreviousMode != KernelMode)
    {
        /* Prepare to probe parameters */
        _SEH2_TRY
        {
            /* Check if we have a class */
            if (Class)
            {
                /* Probe it */
                ParseContext.Class = ProbeForReadUnicodeString(Class);
                ProbeForRead(ParseContext.Class.Buffer,
                             ParseContext.Class.Length,
                             sizeof(WCHAR));
            }

            /* Probe the key handle */
            ProbeForWriteHandle(KeyHandle);
            *KeyHandle = NULL;

            /* Probe object attributes */
            ProbeForRead(ObjectAttributes,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));

            if (Disposition) ProbeForWriteUlong(Disposition);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Save the class directly */
        if (Class) ParseContext.Class = *Class;
    }

    /* Setup the parse context */
    ParseContext.CreateOperation = TRUE;
    ParseContext.CreateOptions = CreateOptions;

    /* Do the create */
    Status = ObOpenObjectByName(ObjectAttributes,
                                CmpKeyObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                &ParseContext,
                                &Handle);

    _SEH2_TRY
    {
        /* Return data to user */
        if (NT_SUCCESS(Status)) *KeyHandle = Handle;
        if (Disposition) *Disposition = ParseContext.Disposition;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the status */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtOpenKey(OUT PHANDLE KeyHandle,
          IN ACCESS_MASK DesiredAccess,
          IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    CM_PARSE_CONTEXT ParseContext = {0};
    HANDLE Handle;
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();
    DPRINT("NtOpenKey(OB 0x%wZ)\n", ObjectAttributes->ObjectName);

    /* Check for user-mode caller */
    if (PreviousMode != KernelMode)
    {
        /* Prepare to probe parameters */
        _SEH2_TRY
        {
            /* Probe the key handle */
            ProbeForWriteHandle(KeyHandle);
            *KeyHandle = NULL;

            /* Probe object attributes */
            ProbeForRead(ObjectAttributes,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the status */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Just let the object manager handle this */
    Status = ObOpenObjectByName(ObjectAttributes,
                                CmpKeyObjectType,
                                ExGetPreviousMode(),
                                NULL,
                                DesiredAccess,
                                &ParseContext,
                                &Handle);

    /* Only do this if we succeeded */
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            /* Return the handle to caller */
            *KeyHandle = Handle;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the status */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status */
    return Status;
}


NTSTATUS
NTAPI
NtDeleteKey(IN HANDLE KeyHandle)
{
    PCM_KEY_BODY KeyObject;
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
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Setup the callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    DeleteKeyInfo.Object = (PVOID)KeyObject;
    Status = CmiCallRegisteredCallbacks(RegNtPreDeleteKey, &DeleteKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Check if we are read-only */
        if ((KeyObject->KeyControlBlock->ExtFlags & CM_KCB_READ_ONLY_KEY) ||
            (KeyObject->KeyControlBlock->ParentKcb->ExtFlags & CM_KCB_READ_ONLY_KEY))
        {
            /* Fail */
            Status = STATUS_ACCESS_DENIED;
        }
        else
        {
            /* Call the internal API */
            Status = CmDeleteKey(KeyObject);
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
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PCM_KEY_BODY KeyObject;
    REG_ENUMERATE_KEY_INFORMATION EnumerateKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();
    DPRINT("NtEnumerateKey() KH 0x%x, Index 0x%x, KIC %d, Length %d\n",
           KeyHandle, Index, KeyInformationClass, Length);

    /* Reject classes we don't know about */
    if ((KeyInformationClass != KeyBasicInformation) &&
        (KeyInformationClass != KeyNodeInformation)  &&
        (KeyInformationClass != KeyFullInformation))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_ENUMERATE_SUB_KEYS,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(ResultLength);
            ProbeForWrite(KeyInformation,
                          Length,
                          sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            return Status;
        }
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
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PCM_KEY_BODY KeyObject;
    REG_ENUMERATE_VALUE_KEY_INFORMATION EnumerateValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    PAGED_CODE();
    DPRINT("NtEnumerateValueKey() KH 0x%x, Index 0x%x, KVIC %d, Length %d\n",
           KeyHandle, Index, KeyValueInformationClass, Length);

    /* Reject classes we don't know about */
    if ((KeyValueInformationClass != KeyValueBasicInformation) &&
        (KeyValueInformationClass != KeyValueFullInformation)  &&
        (KeyValueInformationClass != KeyValuePartialInformation))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_QUERY_VALUE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(ResultLength);
            ProbeForWrite(KeyValueInformation,
                          Length,
                          sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            return Status;
        }
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
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PCM_KEY_BODY KeyObject;
    REG_QUERY_KEY_INFORMATION QueryKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    PAGED_CODE();
    DPRINT("NtQueryKey() KH 0x%x, KIC %d, Length %d\n",
           KeyHandle, KeyInformationClass, Length);

    /* Reject invalid classes */
    if ((KeyInformationClass != KeyBasicInformation) &&
        (KeyInformationClass != KeyNodeInformation)  &&
        (KeyInformationClass != KeyFullInformation)  &&
        (KeyInformationClass != KeyNameInformation) &&
        (KeyInformationClass != KeyCachedInformation) &&
        (KeyInformationClass != KeyFlagsInformation))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if just the name is required */
    if (KeyInformationClass == KeyNameInformation)
    {
        /* Ignore access level */
        Status = ObReferenceObjectByHandle(KeyHandle,
                                           0,
                                           CmpKeyObjectType,
                                           ExGetPreviousMode(),
                                           (PVOID*)&KeyObject,
                                           &HandleInfo);
        if (NT_SUCCESS(Status))
        {
            /* At least a single bit of access is required */
            if (!HandleInfo.GrantedAccess)
            {
                /* No such luck */
                ObDereferenceObject(KeyObject);
                Status = STATUS_ACCESS_DENIED;
            }
        }
    }
    else
    {
        /* Get a reference */
        Status = ObReferenceObjectByHandle(KeyHandle,
                                           KEY_QUERY_VALUE,
                                           CmpKeyObjectType,
                                           ExGetPreviousMode(),
                                           (PVOID*)&KeyObject,
                                           NULL);
    }

    /* Quit on failure */
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(ResultLength);
            ProbeForWrite(KeyInformation,
                          Length,
                          sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            return Status;
        }
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
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PCM_KEY_BODY KeyObject;
    REG_QUERY_VALUE_KEY_INFORMATION QueryValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    UNICODE_STRING ValueNameCopy = *ValueName;
    PAGED_CODE();
    DPRINT("NtQueryValueKey() KH 0x%x, VN '%wZ', KVIC %d, Length %d\n",
        KeyHandle, ValueName, KeyValueInformationClass, Length);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_QUERY_VALUE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(ResultLength);
            ProbeForWrite(KeyValueInformation,
                          Length,
                          sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            return Status;
        }
    }

    /* Make sure the name is aligned properly */
    if ((ValueNameCopy.Length & (sizeof(WCHAR) - 1)))
    {
        /* It isn't, so we'll fail */
        ObDereferenceObject(KeyObject);
        return STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Ignore any null characters at the end */
        while ((ValueNameCopy.Length) &&
               !(ValueNameCopy.Buffer[ValueNameCopy.Length / sizeof(WCHAR) - 1]))
        {
            /* Skip it */
            ValueNameCopy.Length -= sizeof(WCHAR);
        }
    }

    /* Setup the callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    QueryValueKeyInfo.Object = (PVOID)KeyObject;
    QueryValueKeyInfo.ValueName = &ValueNameCopy;
    QueryValueKeyInfo.KeyValueInformationClass = KeyValueInformationClass;
    QueryValueKeyInfo.Length = Length;
    QueryValueKeyInfo.ResultLength = ResultLength;

    /* Do the callback */
    Status = CmiCallRegisteredCallbacks(RegNtPreQueryValueKey, &QueryValueKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmQueryValueKey(KeyObject->KeyControlBlock,
                                 ValueNameCopy,
                                 KeyValueInformationClass,
                                 KeyValueInformation,
                                 Length,
                                 ResultLength);

        /* Do the post callback */
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostQueryValueKey, &PostOperationInfo);
    }

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
    PCM_KEY_BODY KeyObject;
    REG_SET_VALUE_KEY_INFORMATION SetValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    UNICODE_STRING ValueNameCopy = *ValueName;
    PAGED_CODE();
    DPRINT("NtSetValueKey() KH 0x%x, VN '%wZ', TI %x, T %d, DS %d\n",
        KeyHandle, ValueName, TitleIndex, Type, DataSize);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_SET_VALUE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Make sure the name is aligned, not too long, and the data under 4GB */
    if ( (ValueNameCopy.Length > 32767) ||
         ((ValueNameCopy.Length & (sizeof(WCHAR) - 1))) ||
         (DataSize > 0x80000000))
    {
        /* Fail */
        ObDereferenceObject(KeyObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Ignore any null characters at the end */
    while ((ValueNameCopy.Length) &&
           !(ValueNameCopy.Buffer[ValueNameCopy.Length / sizeof(WCHAR) - 1]))
    {
        /* Skip it */
        ValueNameCopy.Length -= sizeof(WCHAR);
    }

    /* Don't touch read-only keys */
    if (KeyObject->KeyControlBlock->ExtFlags & CM_KCB_READ_ONLY_KEY)
    {
        /* Fail */
        ObDereferenceObject(KeyObject);
        return STATUS_ACCESS_DENIED;
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
                               &ValueNameCopy,
                               Type,
                               Data,
                               DataSize);
    }

    /* Do the post-callback */
    PostOperationInfo.Status = Status;
    CmiCallRegisteredCallbacks(RegNtPostSetValueKey, &PostOperationInfo);

    /* Dereference and return status */
    ObDereferenceObject(KeyObject);
    return Status;
}

NTSTATUS
NTAPI
NtDeleteValueKey(IN HANDLE KeyHandle,
                 IN PUNICODE_STRING ValueName)
{
    PCM_KEY_BODY KeyObject;
    NTSTATUS Status;
    REG_DELETE_VALUE_KEY_INFORMATION DeleteValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    UNICODE_STRING ValueNameCopy = *ValueName;
    PAGED_CODE();

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_SET_VALUE,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Don't touch read-only keys */
    if (KeyObject->KeyControlBlock->ExtFlags & CM_KCB_READ_ONLY_KEY)
    {
        /* Fail */
        ObDereferenceObject(KeyObject);
        return STATUS_ACCESS_DENIED;
    }

    /* Make sure the name is aligned properly */
    if ((ValueNameCopy.Length & (sizeof(WCHAR) - 1)))
    {
        /* It isn't, so we'll fail */
        ObDereferenceObject(KeyObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Do the callback */
    DeleteValueKeyInfo.Object = (PVOID)KeyObject;
    DeleteValueKeyInfo.ValueName = ValueName;
    Status = CmiCallRegisteredCallbacks(RegNtPreDeleteValueKey,
                                        &DeleteValueKeyInfo);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal API */
        Status = CmDeleteValueKey(KeyObject->KeyControlBlock, ValueNameCopy);

        /* Do the post callback */
        PostOperationInfo.Object = (PVOID)KeyObject;
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostDeleteValueKey,
                                   &PostOperationInfo);
    }

    /* Dereference the key body */
    ObDereferenceObject(KeyObject);
    return Status;
}

NTSTATUS
NTAPI
NtFlushKey(IN HANDLE KeyHandle)
{
    NTSTATUS Status;
    PCM_KEY_BODY KeyObject;
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
    CmpLockRegistry();

    /* Lock the KCB */
    CmpAcquireKcbLockShared(KeyObject->KeyControlBlock);

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

    /* Release the locks */
    CmpReleaseKcbLock(KeyObject->KeyControlBlock);
    CmpUnlockRegistry();

    /* Dereference the object and return status */
    ObDereferenceObject(KeyObject);
    return Status;
}

NTSTATUS
NTAPI
NtLoadKey(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
          IN POBJECT_ATTRIBUTES FileObjectAttributes)
{
    /* Call the newer API */
    return NtLoadKeyEx(KeyObjectAttributes, FileObjectAttributes, 0, NULL);
}

NTSTATUS
NTAPI
NtLoadKey2(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
           IN POBJECT_ATTRIBUTES FileObjectAttributes,
           IN ULONG Flags)
{
    /* Call the newer API */
    return NtLoadKeyEx(KeyObjectAttributes, FileObjectAttributes, Flags, NULL);
}

NTSTATUS
NTAPI
NtLoadKeyEx(IN POBJECT_ATTRIBUTES TargetKey,
            IN POBJECT_ATTRIBUTES SourceFile,
            IN ULONG Flags,
            IN HANDLE TrustClassKey)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PCM_KEY_BODY KeyBody = NULL;
    PAGED_CODE();

    /* Validate flags */
    if (Flags & ~REG_NO_LAZY_FLUSH) return STATUS_INVALID_PARAMETER;

    /* Validate privilege */
    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, PreviousMode))
    {
        /* Fail */
        DPRINT1("Restore Privilege missing!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
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
    /* Call the newer API */
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
NtInitializeRegistry(IN USHORT Flag)
{
    BOOLEAN SetupBoot;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Always do this as kernel mode */
    if (KeGetPreviousMode() == UserMode) return ZwInitializeRegistry(Flag);

    /* Validate flag */
    if (Flag > CM_BOOT_FLAG_MAX) return STATUS_INVALID_PARAMETER;

    /* Check if boot was accepted */
    if ((Flag >= CM_BOOT_FLAG_ACCEPTED) && (Flag <= CM_BOOT_FLAG_MAX))
    {
        /* Only allow once */
        if (!CmBootAcceptFirstTime) return STATUS_ACCESS_DENIED;
        CmBootAcceptFirstTime = FALSE;

        /* Get the control set accepted */
        Flag -= CM_BOOT_FLAG_ACCEPTED;
        if (Flag)
        {
            /* FIXME: Save the last known good boot */
            //Status = CmpSaveBootControlSet(Flag);

            /* Notify HAL */
            HalEndOfBoot();

            /* Enable lazy flush */
            CmpHoldLazyFlush = FALSE;
            CmpLazyFlush();
            return Status;
        }

        /* Otherwise, invalid boot */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if this was a setup boot */
    SetupBoot = (Flag == CM_BOOT_FLAG_SETUP ? TRUE : FALSE);

    /* Make sure we're only called once */
    if (!CmFirstTime) return STATUS_ACCESS_DENIED;
    CmFirstTime = FALSE;

    /* Acquire registry lock */
    //CmpLockRegistryExclusive();

    /* Initialize the hives and lazy flusher */
    CmpCmdInit(SetupBoot);

    /* FIXME: Save version data */
    //CmpSetVersionData();

    /* Release the registry lock */
    //CmpUnlockRegistry();
    return STATUS_SUCCESS;
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
                   OUT PULONG HandleCount)
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
NtRenameKey(IN HANDLE KeyHandle,
            IN PUNICODE_STRING ReplacementName)
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
