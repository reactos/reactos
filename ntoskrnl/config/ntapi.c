/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/ntapi.c
 * PURPOSE:         Configuration Manager - Internal Registry APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

BOOLEAN CmBootAcceptFirstTime = TRUE;
BOOLEAN CmFirstTime = TRUE;
extern ULONG InitSafeBootMode;

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
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    CM_PARSE_CONTEXT ParseContext = {0};
    HANDLE Handle;
    PAGED_CODE();

    DPRINT("NtCreateKey(Path: %wZ, Root %x, Access: %x, CreateOptions %x)\n",
            ObjectAttributes->ObjectName, ObjectAttributes->RootDirectory,
            DesiredAccess, CreateOptions);

    /* Ignore the WOW64 flag, it's not valid in the kernel */
    DesiredAccess &= ~KEY_WOW64_RES;

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

            if (Disposition)
                ProbeForWriteUlong(Disposition);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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

    DPRINT("Returning handle %x, Status %x.\n", Handle, Status);

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
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();
    DPRINT("NtOpenKey(Path: %wZ, Root %x, Access: %x)\n",
            ObjectAttributes->ObjectName, ObjectAttributes->RootDirectory, DesiredAccess);

    /* Ignore the WOW64 flag, it's not valid in the kernel */
    DesiredAccess &= ~KEY_WOW64_RES;

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
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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

    DPRINT("Returning handle %x, Status %x.\n", Handle, Status);

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
    DPRINT("NtEnumerateKey() KH 0x%p, Index 0x%x, KIC %d, Length %lu\n",
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
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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
    DPRINT("Returning status %x.\n", Status);
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
    DPRINT("NtEnumerateValueKey() KH 0x%p, Index 0x%x, KVIC %d, Length %lu\n",
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
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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
    DPRINT("NtQueryKey() KH 0x%p, KIC %d, Length %lu\n",
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
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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
    DPRINT("NtQueryValueKey() KH 0x%p, VN '%wZ', KVIC %d, Length %lu\n",
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
            /* Dereference and return status */
            ObDereferenceObject(KeyObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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
    NTSTATUS Status = STATUS_SUCCESS;
    PCM_KEY_BODY KeyObject = NULL;
    REG_SET_VALUE_KEY_INFORMATION SetValueKeyInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    UNICODE_STRING ValueNameCopy;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if (!DataSize)
        Data = NULL;

    /* Probe and copy the data */
    if ((PreviousMode != KernelMode) && (DataSize != 0))
    {
        PVOID DataCopy = ExAllocatePoolWithTag(PagedPool, DataSize, TAG_CM);
        if (!DataCopy)
            return STATUS_INSUFFICIENT_RESOURCES;
        _SEH2_TRY
        {
            ProbeForRead(Data, DataSize, 1);
            RtlCopyMemory(DataCopy, Data, DataSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(DataCopy, TAG_CM);
            return Status;
        }
        Data = DataCopy;
    }

    /* Capture the string */
    Status = ProbeAndCaptureUnicodeString(&ValueNameCopy, PreviousMode, ValueName);
    if (!NT_SUCCESS(Status))
        goto end;

    DPRINT("NtSetValueKey() KH 0x%p, VN '%wZ', TI %x, T %lu, DS %lu\n",
        KeyHandle, &ValueNameCopy, TitleIndex, Type, DataSize);

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_SET_VALUE,
                                       CmpKeyObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
        goto end;

    /* Make sure the name is aligned, not too long, and the data under 4GB */
    if ( (ValueNameCopy.Length > 32767) ||
         ((ValueNameCopy.Length & (sizeof(WCHAR) - 1))) ||
         (DataSize > 0x80000000))
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER;
        goto end;
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
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    /* Setup callback */
    PostOperationInfo.Object = (PVOID)KeyObject;
    SetValueKeyInfo.Object = (PVOID)KeyObject;
    SetValueKeyInfo.ValueName = &ValueNameCopy;
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

end:
    /* Dereference and return status */
    if (KeyObject)
        ObDereferenceObject(KeyObject);
    ReleaseCapturedUnicodeString(&ValueNameCopy, PreviousMode);
    if ((PreviousMode != KernelMode) && Data)
        ExFreePoolWithTag(Data, TAG_CM);
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
    if (KeGetPreviousMode() == UserMode)
        return ZwInitializeRegistry(Flag);

    /* Enough of the system has booted by now */
    Ki386PerfEnd();

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

    /* Lock the registry exclusively */
    CmpLockRegistryExclusive();

    /* Initialize the hives and lazy flusher */
    CmpCmdInit(SetupBoot);

    /* Save version data */
    CmpSetVersionData();

    /* Release the registry lock */
    CmpUnlockRegistry();
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

// FIXME: different for different windows versions!
#define PRODUCT_ACTIVATION_VERSION 7749

NTSTATUS
NTAPI
NtLockProductActivationKeys(IN PULONG pPrivateVer,
                            IN PULONG pSafeMode)
{
    KPROCESSOR_MODE PreviousMode;

    PreviousMode = ExGetPreviousMode();
    _SEH2_TRY
    {
        /* Check if the caller asked for the version */
        if (pPrivateVer != NULL)
        {
            /* For user mode, probe it */
            if (PreviousMode != KernelMode)
            {
                ProbeForRead(pPrivateVer, sizeof(ULONG), sizeof(ULONG));
            }

            /* Return the expected version */
            *pPrivateVer = PRODUCT_ACTIVATION_VERSION;
        }

        /* Check if the caller asked for safe mode mode state */
        if (pSafeMode != NULL)
        {
            /* For user mode, probe it */
            if (PreviousMode != KernelMode)
            {
                ProbeForRead(pSafeMode, sizeof(ULONG), sizeof(ULONG));
            }

            /* Return the safe boot mode state */
            *pSafeMode = InitSafeBootMode;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
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
    KPROCESSOR_MODE PreviousMode;
    PCM_KEY_BODY KeyBody = NULL;
    HANDLE KeyHandle;
    NTSTATUS Status;

    DPRINT("NtQueryOpenSubKeys()\n");

    PAGED_CODE();

    /* Get the processor mode */
    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        /* Prepare to probe parameters */
        _SEH2_TRY
        {
            /* Probe target key */
            ProbeForRead(TargetKey,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));

            /* Probe handle count */
            ProbeForWriteUlong(HandleCount);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open a handle to the key */
    Status = ObOpenObjectByName(TargetKey,
                                CmpKeyObjectType,
                                PreviousMode,
                                NULL,
                                KEY_READ,
                                NULL,
                                &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        /* Reference the key object */
        Status = ObReferenceObjectByHandle(KeyHandle,
                                           KEY_READ,
                                           CmpKeyObjectType,
                                           PreviousMode,
                                           (PVOID *)&KeyBody,
                                           NULL);

        /* Close the handle */
        NtClose(KeyHandle);
    }

    /* Fail, if the key object could not be referenced */
    if (!NT_SUCCESS(Status))
        return Status;

    /* Lock the registry exclusively */
    CmpLockRegistryExclusive();

    /* Fail, if we did not open a hive root key */
    if (KeyBody->KeyControlBlock->KeyCell !=
        KeyBody->KeyControlBlock->KeyHive->BaseBlock->RootCell)
    {
        DPRINT("Error: Key is not a hive root key!\n");
        CmpUnlockRegistry();
        ObDereferenceObject(KeyBody);
        return STATUS_INVALID_PARAMETER;
    }

    /* Call the internal API */
    *HandleCount = CmCountOpenSubKeys(KeyBody->KeyControlBlock,
                                      FALSE);

    /* Unlock the registry */
    CmpUnlockRegistry();

    /* Dereference the key object */
    ObDereferenceObject(KeyBody);

    DPRINT("Done.\n");

    return Status;
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
    /* Call the extended API */
    return NtSaveKeyEx(KeyHandle, FileHandle, REG_STANDARD_FORMAT);
}

NTSTATUS
NTAPI
NtSaveKeyEx(IN HANDLE KeyHandle,
            IN HANDLE FileHandle,
            IN ULONG Flags)
{
    NTSTATUS Status;
    PCM_KEY_BODY KeyObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    PAGED_CODE();

    DPRINT("NtSaveKeyEx(0x%p, 0x%p, %lu)\n", KeyHandle, FileHandle, Flags);

    /* Verify the flags */
    if ((Flags != REG_STANDARD_FORMAT)
        && (Flags != REG_LATEST_FORMAT)
        && (Flags != REG_NO_COMPRESSION))
    {
        /* Only one of these values can be specified */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for the SeBackupPrivilege */
    if (!SeSinglePrivilegeCheck(SeBackupPrivilege, PreviousMode))
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_READ,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID*)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Call the internal API */
    Status = CmSaveKey(KeyObject->KeyControlBlock, FileHandle, Flags);

    ObDereferenceObject(KeyObject);
    return Status;
}

NTSTATUS
NTAPI
NtSaveMergedKeys(IN HANDLE HighPrecedenceKeyHandle,
                 IN HANDLE LowPrecedenceKeyHandle,
                 IN HANDLE FileHandle)
{
    KPROCESSOR_MODE PreviousMode;
    PCM_KEY_BODY HighPrecedenceKeyObject = NULL;
    PCM_KEY_BODY LowPrecedenceKeyObject = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("NtSaveMergedKeys(0x%p, 0x%p, 0x%p)\n",
           HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);

    PreviousMode = ExGetPreviousMode();

    /* Check for the SeBackupPrivilege */
    if (!SeSinglePrivilegeCheck(SeBackupPrivilege, PreviousMode))
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Verify that the handles are valid and are registry keys */
    Status = ObReferenceObjectByHandle(HighPrecedenceKeyHandle,
                                       KEY_READ,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID*)&HighPrecedenceKeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = ObReferenceObjectByHandle(LowPrecedenceKeyHandle,
                                       KEY_READ,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID*)&LowPrecedenceKeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Call the internal API */
    Status = CmSaveMergedKeys(HighPrecedenceKeyObject->KeyControlBlock,
                              LowPrecedenceKeyObject->KeyControlBlock,
                              FileHandle);

done:
    if (LowPrecedenceKeyObject)
        ObDereferenceObject(LowPrecedenceKeyObject);

    if (HighPrecedenceKeyObject)
        ObDereferenceObject(HighPrecedenceKeyObject);

    return Status;
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
    return NtUnloadKey2(KeyObjectAttributes, 0);
}

NTSTATUS
NTAPI
NtUnloadKey2(IN POBJECT_ATTRIBUTES TargetKey,
             IN ULONG Flags)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;
    CM_PARSE_CONTEXT ParseContext = {0};
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PCM_KEY_BODY KeyBody = NULL;
    ULONG ParentConv = 0, ChildConv = 0;
    HANDLE Handle;

    PAGED_CODE();

    /* Validate privilege */
    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, PreviousMode))
    {
        /* Fail */
        DPRINT1("Restore Privilege missing!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Check for user-mode caller */
    if (PreviousMode != KernelMode)
    {
        /* Prepare to probe parameters */
        _SEH2_TRY
        {
            /* Probe object attributes */
            ProbeForRead(TargetKey,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));

            ObjectAttributes = *TargetKey;

            /* Probe the string */
            ProbeForReadUnicodeString(&TargetKey->ObjectName);

            ObjectName = *TargetKey->ObjectName;

            ProbeForRead(ObjectName.Buffer,
                         ObjectName.Length,
                         sizeof(WCHAR));

            ObjectAttributes.ObjectName = &ObjectName;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Save the target attributes directly */
        ObjectAttributes = *TargetKey;
    }

    /* Setup the parse context */
    ParseContext.CreateOperation = TRUE;
    ParseContext.CreateOptions = REG_OPTION_BACKUP_RESTORE;

    /* Do the create */
    Status = ObOpenObjectByName(&ObjectAttributes,
                                CmpKeyObjectType,
                                KernelMode,
                                NULL,
                                KEY_WRITE,
                                &ParseContext,
                                &Handle);

    /* Return if failure encountered */
    if (!NT_SUCCESS(Status)) return Status;

    /* Reference it */
    Status = ObReferenceObjectByHandle(Handle,
                                       KEY_WRITE,
                                       CmpKeyObjectType,
                                       KernelMode,
                                       (PVOID *)&KeyBody,
                                       NULL);

    /* Close the handle */
    ZwClose(Handle);

    /* Return if failure encountered */
    if (!NT_SUCCESS(Status)) return Status;

    /* Acquire the lock depending on flags */
    if (Flags == REG_FORCE_UNLOAD)
    {
        /* Lock registry exclusively */
        CmpLockRegistryExclusive();
    }
    else
    {
        /* Lock registry */
        CmpLockRegistry();

        /* Acquire the hive loading lock */
        ExAcquirePushLockExclusive(&CmpLoadHiveLock);

        /* Lock parent and child */
        if (KeyBody->KeyControlBlock->ParentKcb)
            ParentConv = KeyBody->KeyControlBlock->ParentKcb->ConvKey;
        else
            ParentConv = KeyBody->KeyControlBlock->ConvKey;

        ChildConv = KeyBody->KeyControlBlock->ConvKey;

        CmpAcquireTwoKcbLocksExclusiveByKey(ChildConv, ParentConv);
    }

    /* Check if it's being deleted already */
    if (KeyBody->KeyControlBlock->Delete)
    {
        /* Return appropriate status */
        Status = STATUS_KEY_DELETED;
        goto Quickie;
    }

    /* Check if it's a readonly key */
    if (KeyBody->KeyControlBlock->ExtFlags & CM_KCB_READ_ONLY_KEY)
    {
        /* Return appropriate status */
        Status = STATUS_ACCESS_DENIED;
        goto Quickie;
    }

    /* Call the internal API */
    Status = CmUnloadKey(KeyBody->KeyControlBlock,
                         Flags);

    /* Check if we failed, but really need to succeed */
    if ((Status == STATUS_CANNOT_DELETE) && (Flags == REG_FORCE_UNLOAD))
    {
        /* TODO: We should perform another attempt here */
        ASSERT(FALSE);
    }

    /* If CmUnloadKey failed we need to unlock registry ourselves */
    if (!NT_SUCCESS(Status))
    {
        if (Flags != REG_FORCE_UNLOAD)
        {
            /* Release the KCB locks */
            CmpReleaseTwoKcbLockByKey(ChildConv, ParentConv);

            /* Release the hive loading lock */
            ExReleasePushLockExclusive(&CmpLoadHiveLock);
        }

        /* Unlock the registry */
        CmpUnlockRegistry();
    }

Quickie:
    /* Dereference the key */
    ObDereferenceObject(KeyBody);

    /* Return status */
    return Status;
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
