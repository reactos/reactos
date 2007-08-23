/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/ntfunc.c
 * PURPOSE:         Ntxxx function for registry access
 *
 * PROGRAMMERS:     Hartmut Birr
 *                  Casper Hornstrup
 *                  Alex Ionescu
 *                  Rex Jolliff
 *                  Eric Kohl
 *                  Filip Navara
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

/* GLOBALS ******************************************************************/

extern POBJECT_TYPE  CmpKeyObjectType;
extern LIST_ENTRY CmiKeyObjectListHead;

static BOOLEAN CmiRegistryInitialized = FALSE;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
CmpCreateHandle(PVOID ObjectBody,
                ACCESS_MASK GrantedAccess,
                ULONG HandleAttributes,
                PHANDLE HandleReturn)
                /*
                * FUNCTION: Add a handle referencing an object
                * ARGUMENTS:
                *         obj = Object body that the handle should refer to
                * RETURNS: The created handle
                * NOTE: The handle is valid only in the context of the current process
                */
{
    HANDLE_TABLE_ENTRY NewEntry;
    PEPROCESS CurrentProcess;
    PVOID HandleTable;
    POBJECT_HEADER ObjectHeader;
    HANDLE Handle;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE;

    PAGED_CODE();

    DPRINT("CmpCreateHandle(obj %p)\n",ObjectBody);

    ASSERT(ObjectBody);

    CurrentProcess = PsGetCurrentProcess();

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);

    /* check that this is a valid kernel pointer */
    //ASSERT((ULONG_PTR)ObjectHeader & EX_HANDLE_ENTRY_LOCKED);

    if (GrantedAccess & MAXIMUM_ALLOWED)
    {
        GrantedAccess &= ~MAXIMUM_ALLOWED;
        GrantedAccess |= GENERIC_ALL;
    }

    if (GrantedAccess & GENERIC_ACCESS)
    {
        RtlMapGenericMask(&GrantedAccess,
            &ObjectHeader->Type->TypeInfo.GenericMapping);
    }

    NewEntry.Object = ObjectHeader;
    if(HandleAttributes & OBJ_INHERIT)
        NewEntry.ObAttributes |= OBJ_INHERIT;
    else
        NewEntry.ObAttributes &= ~OBJ_INHERIT;
    NewEntry.GrantedAccess = GrantedAccess;

    if ((HandleAttributes & OBJ_KERNEL_HANDLE) &&
        ExGetPreviousMode() == KernelMode)
    {
        HandleTable = ObpKernelHandleTable;
        if (PsGetCurrentProcess() != PsInitialSystemProcess)
        {
            KeStackAttachProcess(&PsInitialSystemProcess->Pcb,
                &ApcState);
            AttachedToProcess = TRUE;
        }
    }
    else
    {
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }

    Handle = ExCreateHandle(HandleTable,
        &NewEntry);

    if (AttachedToProcess)
    {
        KeUnstackDetachProcess(&ApcState);
    }

    if(Handle != NULL)
    {
        if (HandleAttributes & OBJ_KERNEL_HANDLE)
        {
            /* mark the handle value */
            Handle = ObMarkHandleAsKernelHandle(Handle);
        }

        InterlockedIncrement(&ObjectHeader->HandleCount);
        ObReferenceObject(ObjectBody);

        *HandleReturn = Handle;

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

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
    UNICODE_STRING RemainingPath = {0};
    ULONG LocalDisposition;
    PKEY_OBJECT KeyObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID Object = NULL;
    PWSTR Start;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    unsigned int i;
    REG_PRE_CREATE_KEY_INFORMATION PreCreateKeyInfo;
    REG_POST_CREATE_KEY_INFORMATION PostCreateKeyInfo;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING CapturedClass = {0};
    HANDLE hKey;

    PAGED_CODE();

    DPRINT("NtCreateKey(TI 0x%x, DA 0x%x, Class '%wZ', OA 0x%p, OA->ON '%wZ'\n",
        TitleIndex, DesiredAccess, Class, ObjectAttributes,
        ObjectAttributes ? ObjectAttributes->ObjectName : NULL);

    PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeAndZeroHandle(KeyHandle);
            if (Disposition != NULL)
            {
                ProbeForWriteUlong(Disposition);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    if (Class != NULL)
    {
        Status = ProbeAndCaptureUnicodeString(&CapturedClass,
                                              PreviousMode,
                                              Class);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Capture all the info */
    DPRINT("Capturing Create Info\n");
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        PreviousMode,
                                        FALSE,
                                        &ObjectCreateInfo,
                                        &ObjectName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
        return Status;
    }

    PostCreateKeyInfo.CompleteName = &ObjectName;
    PreCreateKeyInfo.CompleteName = &ObjectName;
    Status = CmiCallRegisteredCallbacks(RegNtPreCreateKey, &PreCreateKeyInfo);
    if (!NT_SUCCESS(Status))
    {
        PostCreateKeyInfo.Object = NULL;
        PostCreateKeyInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);
        goto Cleanup;
    }

    Status = CmFindObject(&ObjectCreateInfo,
                          &ObjectName,
                          (PVOID*)&Object,
                          &RemainingPath,
                          CmpKeyObjectType,
                          NULL,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        PostCreateKeyInfo.Object = NULL;
        PostCreateKeyInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

        DPRINT1("CmpFindObject failed, Status: 0x%x\n", Status);
        goto Cleanup;
    }

    DPRINT("RemainingPath %wZ\n", &RemainingPath);

    if (RemainingPath.Length == 0)
    {
        /* Fail if the key has been deleted */
        if (((PKEY_OBJECT) Object)->Flags & KO_MARKED_FOR_DELETE)
        {
            PostCreateKeyInfo.Object = NULL;
            PostCreateKeyInfo.Status = STATUS_UNSUCCESSFUL;
            CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

            DPRINT1("Object marked for delete!\n");
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        Status = CmpCreateHandle(Object,
                                 DesiredAccess,
                                 ObjectCreateInfo.Attributes,
                                 &hKey);

        if (!NT_SUCCESS(Status))
            DPRINT1("CmpCreateHandle failed Status 0x%x\n", Status);

        PostCreateKeyInfo.Object = NULL;
        PostCreateKeyInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

        LocalDisposition = REG_OPENED_EXISTING_KEY;
        goto SuccessReturn;
    }

    /* If RemainingPath contains \ we must return error
       because NtCreateKey doesn't create trees */
    Start = RemainingPath.Buffer;
    if (*Start == L'\\')
    {
        Start++;
        //RemainingPath.Length -= sizeof(WCHAR);
        //RemainingPath.MaximumLength -= sizeof(WCHAR);
        //RemainingPath.Buffer++;
        //DPRINT1("String: %wZ\n", &RemainingPath);
    }

    if (RemainingPath.Buffer[(RemainingPath.Length / sizeof(WCHAR)) - 1] == '\\')
    {
        RemainingPath.Buffer[(RemainingPath.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
        RemainingPath.Length -= sizeof(WCHAR);
        RemainingPath.MaximumLength -= sizeof(WCHAR);
    }

    for (i = 1; i < RemainingPath.Length / sizeof(WCHAR); i++)
    {
        if (L'\\' == RemainingPath.Buffer[i])
        {
            DPRINT("NtCreateKey() doesn't create trees! (found \'\\\' in remaining path: \"%wZ\"!)\n", &RemainingPath);

            PostCreateKeyInfo.Object = NULL;
            PostCreateKeyInfo.Status = STATUS_OBJECT_NAME_NOT_FOUND;
            CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto Cleanup;
        }
    }

    DPRINT("RemainingPath %S  ParentObject 0x%p\n", RemainingPath.Buffer, Object);

    /* Acquire hive lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    /* Create the key */
    Status = CmpDoCreate(&((PKEY_OBJECT)Object)->RegistryHive->Hive,
                         ((PKEY_OBJECT)Object)->KeyCellOffset,
                         NULL,
                         &RemainingPath,
                         KernelMode,
                         Class,
                         CreateOptions,
                         (PKEY_OBJECT)Object,
                         NULL,
                         (PVOID*)&KeyObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiAddSubKey() failed (Status %lx)\n", Status);
        /* Release hive lock */
        ExReleaseResourceLite(&CmpRegistryLock);
        KeLeaveCriticalRegion();

        PostCreateKeyInfo.Object = NULL;
        PostCreateKeyInfo.Status = STATUS_UNSUCCESSFUL;
        CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    InsertTailList(&CmiKeyObjectListHead, &KeyObject->ListEntry);
    RtlCreateUnicodeString(&KeyObject->Name, Start);

    KeyObject->KeyCell->Parent = KeyObject->ParentKey->KeyCellOffset;
    KeyObject->KeyCell->SecurityKeyOffset = KeyObject->ParentKey->KeyCell->SecurityKeyOffset;
    KeyObject->ValueCache.ValueList = KeyObject->KeyCell->ValueList.List;
    KeyObject->ValueCache.Count = KeyObject->KeyCell->ValueList.Count;

    DPRINT("RemainingPath: %wZ\n", &RemainingPath);

    CmiAddKeyToList(KeyObject->ParentKey, KeyObject);

    VERIFY_KEY_OBJECT(KeyObject);

    Status = CmpCreateHandle(KeyObject,
                             DesiredAccess,
                             ObjectCreateInfo.Attributes,
                             &hKey);

    /* Free the create information */
    ObpFreeAndReleaseCapturedAttributes(OBJECT_TO_OBJECT_HEADER(KeyObject)->ObjectCreateInfo);
    OBJECT_TO_OBJECT_HEADER(KeyObject)->ObjectCreateInfo = NULL;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObInsertObject() failed!\n");

        PostCreateKeyInfo.Object = NULL;
        PostCreateKeyInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

        goto Cleanup;
    }

    /* Add the keep-alive reference */
    ObReferenceObject(KeyObject);
    /* Release hive lock */
    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();

    PostCreateKeyInfo.Object = KeyObject;
    PostCreateKeyInfo.Status = Status;
    CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

    CmiSyncHives();

    LocalDisposition = REG_CREATED_NEW_KEY;

SuccessReturn:
    _SEH_TRY
    {
        *KeyHandle = hKey;
        if (Disposition != NULL)
        {
            *Disposition = LocalDisposition;
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

Cleanup:
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);
    if (Class != NULL)
    {
        ReleaseCapturedUnicodeString(&CapturedClass,
                                     PreviousMode);
    }
    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    RtlFreeUnicodeString(&RemainingPath);
    if (Object != NULL) ObDereferenceObject(Object);

    return Status;
}

NTSTATUS
NTAPI
NtFlushKey(IN HANDLE KeyHandle)
{
    NTSTATUS Status;
    PKEY_OBJECT  KeyObject;
    PEREGISTRY_HIVE  RegistryHive;
    KPROCESSOR_MODE  PreviousMode;

    PAGED_CODE();

    DPRINT("NtFlushKey (KeyHandle %lx) called\n", KeyHandle);

    PreviousMode = ExGetPreviousMode();

    /* Verify that the handle is valid and is a registry key */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       0,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID *)&KeyObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    VERIFY_KEY_OBJECT(KeyObject);

    RegistryHive = KeyObject->RegistryHive;

    /* Acquire hive lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    if (IsNoFileHive(RegistryHive))
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Flush non-volatile hive */
        Status = CmiFlushRegistryHive(RegistryHive);
    }

    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();

    ObDereferenceObject(KeyObject);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
NtOpenKey(OUT PHANDLE KeyHandle,
          IN ACCESS_MASK DesiredAccess,
          IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    UNICODE_STRING RemainingPath;
    KPROCESSOR_MODE PreviousMode;
    PVOID Object = NULL;
    HANDLE hKey = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    REG_PRE_OPEN_KEY_INFORMATION PreOpenKeyInfo;
    REG_POST_OPEN_KEY_INFORMATION PostOpenKeyInfo;

    PAGED_CODE();

    DPRINT("NtOpenKey(KH 0x%p  DA %x  OA 0x%p  OA->ON '%wZ'\n",
        KeyHandle,
        DesiredAccess,
        ObjectAttributes,
        ObjectAttributes ? ObjectAttributes->ObjectName : NULL);

    /* Check place for result handle, if it's null - return immediately */
    if (KeyHandle == NULL)
        return(STATUS_INVALID_PARAMETER);

    PreviousMode = ExGetPreviousMode();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeAndZeroHandle(KeyHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
           return Status;
        }
    }

    /* WINE checks for the length also */
    /*if (ObjectAttributes->ObjectName->Length > MAX_NAME_LENGTH)
        return(STATUS_BUFFER_OVERFLOW);*/

    /* Capture all the info */
    DPRINT("Capturing Create Info\n");
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        PreviousMode,
                                        FALSE,
                                        &ObjectCreateInfo,
                                        &ObjectName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
        return Status;
    }

    if (ObjectName.Buffer && 
        ObjectName.Buffer[(ObjectName.Length / sizeof(WCHAR)) - 1] == '\\')
    {
        ObjectName.Buffer[(ObjectName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
        ObjectName.Length -= sizeof(WCHAR);
        ObjectName.MaximumLength -= sizeof(WCHAR);
    }

    PostOpenKeyInfo.CompleteName = &ObjectName;
    PreOpenKeyInfo.CompleteName = &ObjectName;
    Status = CmiCallRegisteredCallbacks(RegNtPreOpenKey, &PreOpenKeyInfo);
    if (!NT_SUCCESS(Status))
    {
        PostOpenKeyInfo.Object = NULL;
        PostOpenKeyInfo.Status = Status;
        CmiCallRegisteredCallbacks (RegNtPostOpenKey, &PostOpenKeyInfo);
        ObpReleaseCapturedAttributes(&ObjectCreateInfo);
        if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
        return Status;
    }

    RemainingPath.Buffer = NULL;

    Status = CmFindObject(&ObjectCreateInfo,
                          &ObjectName,
                          (PVOID*)&Object,
                          &RemainingPath,
                          CmpKeyObjectType,
                          NULL,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("CmpFindObject() returned 0x%08lx\n", Status);
        Status = STATUS_INVALID_HANDLE; /* Because ObFindObject returns STATUS_UNSUCCESSFUL */
        goto openkey_cleanup;
    }

    VERIFY_KEY_OBJECT((PKEY_OBJECT) Object);

    DPRINT("RemainingPath '%wZ'\n", &RemainingPath);

    if ((RemainingPath.Buffer != NULL) && (RemainingPath.Buffer[0] != 0))
    {
        RtlFreeUnicodeString(&RemainingPath);
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto openkey_cleanup;
    }

    RtlFreeUnicodeString(&RemainingPath);

    /* Fail if the key has been deleted */
    if (((PKEY_OBJECT)Object)->Flags & KO_MARKED_FOR_DELETE)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto openkey_cleanup;
    }

    Status = CmpCreateHandle(Object,
                             DesiredAccess,
                             ObjectCreateInfo.Attributes,
                             &hKey);

openkey_cleanup:

    ObpReleaseCapturedAttributes(&ObjectCreateInfo);
    PostOpenKeyInfo.Object = NT_SUCCESS(Status) ? (PVOID)Object : NULL;
    PostOpenKeyInfo.Status = Status;
    CmiCallRegisteredCallbacks (RegNtPostOpenKey, &PostOpenKeyInfo);
    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);

    if (Object)
    {
        ObDereferenceObject(Object);
    }

    if (NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            *KeyHandle = hKey;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    return Status;
}

/*
 * NOTE:
 * KeyObjectAttributes->RootDirectory specifies the handle to the parent key and
 * KeyObjectAttributes->Name specifies the name of the key to load.
 * Flags can be 0 or REG_NO_LAZY_FLUSH.
 */
NTSTATUS
NTAPI
NtLoadKey2 (IN POBJECT_ATTRIBUTES KeyObjectAttributes,
            IN POBJECT_ATTRIBUTES FileObjectAttributes,
            IN ULONG Flags)
{
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT ("NtLoadKey2() called\n");

#if 0
    if (!SeSinglePrivilegeCheck (SeRestorePrivilege, ExGetPreviousMode ()))
        return STATUS_PRIVILEGE_NOT_HELD;
#endif

    /* Acquire hive lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    Status = CmiLoadHive (KeyObjectAttributes,
                          FileObjectAttributes->ObjectName,
                          Flags);
    if (!NT_SUCCESS (Status))
    {
        DPRINT1 ("CmiLoadHive() failed (Status %lx)\n", Status);
    }

    /* Release hive lock */
    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();

    return Status;
}

NTSTATUS
NTAPI
NtInitializeRegistry (IN USHORT Flag)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (CmiRegistryInitialized == TRUE)
        return STATUS_ACCESS_DENIED;

    /* Save boot log file */
    IopSaveBootLogToFile();

    Status = CmiInitHives (Flag);

    CmiRegistryInitialized = TRUE;

    return Status;
}

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
        Status = CmDeleteKey(KeyObject);

        /* Remove the keep-alive reference */
        ObDereferenceObject(KeyObject);
        if (KeyObject->RegistryHive != KeyObject->ParentKey->RegistryHive)
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
        Status = CmEnumerateKey(KeyObject,
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
        Status = CmEnumerateValueKey(KeyObject,
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
        Status = CmQueryKey(KeyObject,
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
        Status = CmQueryValueKey(KeyObject,
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
        Status = CmSetValueKey(KeyObject,
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
    CmiSyncHives();
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
        Status = CmDeleteValueKey(KeyObject, *ValueName);

        /* Do the post callback */
        PostOperationInfo.Object = (PVOID)KeyObject;
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostDeleteValueKey,
                                   &PostOperationInfo);
    }

    /* Dereference the key body and synchronize the hives */
    ObDereferenceObject(KeyObject);
    CmiSyncHives();
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
