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
    UNICODE_STRING RemainingPath = {0}, ReturnedPath = {0};
    ULONG LocalDisposition;
    PKEY_OBJECT KeyObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID Object = NULL;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    unsigned int i;
    REG_PRE_CREATE_KEY_INFORMATION PreCreateKeyInfo;
    REG_POST_CREATE_KEY_INFORMATION PostCreateKeyInfo;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING CapturedClass = {0};
    HANDLE hKey;
    PCM_KEY_NODE Node, ParentNode;

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
                          &ReturnedPath,
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
    
    RemainingPath = ReturnedPath;
    DPRINT("RemainingPath (preparse) %wZ\n", &RemainingPath);

    if (RemainingPath.Length == 0)
    {
        /* Fail if the key has been deleted */
        if (((PKEY_OBJECT) Object)->KeyControlBlock->Delete)
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
    while (RemainingPath.Length && *RemainingPath.Buffer == L'\\')
    {
        RemainingPath.Length -= sizeof(WCHAR);
        RemainingPath.MaximumLength -= sizeof(WCHAR);
        RemainingPath.Buffer++;
    }

    while (RemainingPath.Length && 
           RemainingPath.Buffer[(RemainingPath.Length / sizeof(WCHAR)) - 1] == L'\\')
    {
        RemainingPath.Length -= sizeof(WCHAR);
        RemainingPath.MaximumLength -= sizeof(WCHAR);
    }

    for (i = 0; i < RemainingPath.Length / sizeof(WCHAR); i++)
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

    DPRINT("RemainingPath %wZ ParentObject 0x%p\n", &RemainingPath, Object);

    // 
    if (RemainingPath.Length == 0 || RemainingPath.Buffer[0] == UNICODE_NULL)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }
        

    /* Acquire hive lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    /* Create the key */
    Status = CmpDoCreate(((PKEY_OBJECT)Object)->KeyControlBlock->KeyHive,
                         ((PKEY_OBJECT)Object)->KeyControlBlock->KeyCell,
                         NULL,
                         &RemainingPath,
                         KernelMode,
                         Class,
                         CreateOptions,
                         ((PKEY_OBJECT)Object)->KeyControlBlock,
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

    RtlDuplicateUnicodeString
        (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, 
         &RemainingPath, &KeyObject->Name);
    DPRINT("Key Name: %wZ\n", &KeyObject->Name);

    ParentNode = (PCM_KEY_NODE)HvGetCell(KeyObject->KeyControlBlock->ParentKcb->KeyHive,
                                         KeyObject->KeyControlBlock->ParentKcb->KeyCell);

    Node = (PCM_KEY_NODE)HvGetCell(KeyObject->KeyControlBlock->KeyHive,
                                   KeyObject->KeyControlBlock->KeyCell);
    
    Node->Parent = KeyObject->KeyControlBlock->ParentKcb->KeyCell;
    Node->Security = ParentNode->Security;
    
    KeyObject->KeyControlBlock->ValueCache.ValueList = Node->ValueList.List;
    KeyObject->KeyControlBlock->ValueCache.Count = Node->ValueList.Count;

    DPRINT("RemainingPath: %wZ\n", &RemainingPath);

    CmiAddKeyToList(((PKEY_OBJECT)Object), KeyObject);

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
    RtlFreeUnicodeString(&ReturnedPath);
    if (Object != NULL) ObDereferenceObject(Object);

    return Status;
}

NTSTATUS
NTAPI
NtFlushKey(IN HANDLE KeyHandle)
{
    NTSTATUS Status;
    PKEY_OBJECT  KeyObject;
    PCMHIVE  RegistryHive;
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
    
    RegistryHive = (PCMHIVE)KeyObject->KeyControlBlock->KeyHive;
    
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
    if (((PKEY_OBJECT)Object)->KeyControlBlock->Delete)
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

/* EOF */
