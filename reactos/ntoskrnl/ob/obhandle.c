/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/obhandle.c
 * PURPOSE:         Manages all functions related to the Object Manager handle
 *                  implementation, including creating and destroying handles
 *                  and/or handle tables, duplicating objects, and setting the
 *                  permanent or temporary flags.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

PHANDLE_TABLE ObpKernelHandleTable = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
ObpDeleteNameCheck(IN PVOID Object)
{
    POBJECT_HEADER ObjectHeader;
    OBP_LOOKUP_CONTEXT Context;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    POBJECT_TYPE ObjectType;

    /* Get object structures */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
    ObjectType = ObjectHeader->Type;

    /*
     * Check if the handle count is 0, if the object is named,
     * and if the object isn't a permanent object.
     */
    if (!(ObjectHeader->HandleCount) &&
         (ObjectNameInfo) &&
         (ObjectNameInfo->Name.Length) &&
         !(ObjectHeader->Flags & OB_FLAG_PERMANENT))
    {
        /* Make sure it's still inserted */
        Context.Directory = ObjectNameInfo->Directory;
        Context.DirectoryLocked = TRUE;
        Object = ObpLookupEntryDirectory(ObjectNameInfo->Directory,
                                         &ObjectNameInfo->Name,
                                         0,
                                         FALSE,
                                         &Context);
        if (Object)
        {
            /* First delete it from the directory */
            ObpDeleteEntryDirectory(&Context);

            /* Now check if we have a security callback */
            if (ObjectType->TypeInfo.SecurityRequired)
            {
                /* Call it */
                ObjectType->TypeInfo.SecurityProcedure(Object,
                                                       DeleteSecurityDescriptor,
                                                       0,
                                                       NULL,
                                                       NULL,
                                                       &ObjectHeader->
                                                       SecurityDescriptor,
                                                       ObjectType->
                                                       TypeInfo.PoolType,
                                                       NULL);
            }

            /* Free the name */
            ExFreePool(ObjectNameInfo->Name.Buffer);
            RtlInitEmptyUnicodeString(&ObjectNameInfo->Name, NULL, 0);

            /* Clear the current directory and de-reference it */
            ObDereferenceObject(ObjectNameInfo->Directory);
            ObDereferenceObject(Object);
            ObjectNameInfo->Directory = NULL;
        }
    }
}

/*++
* @name ObpSetPermanentObject
*
*     The ObpSetPermanentObject routine <FILLMEIN>
*
* @param ObjectBody
*        <FILLMEIN>
*
* @param Permanent
*        <FILLMEIN>
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
FASTCALL
ObpSetPermanentObject(IN PVOID ObjectBody,
                      IN BOOLEAN Permanent)
{
    POBJECT_HEADER ObjectHeader;

    /* Get the header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);
    if (Permanent)
    {
        /* Set it to permanent */
        ObjectHeader->Flags |= OB_FLAG_PERMANENT;
    }
    else
    {
        /* Remove the flag */
        ObjectHeader->Flags &= ~OB_FLAG_PERMANENT;

        /* Check if we should delete the object now */
        ObpDeleteNameCheck(ObjectBody);
    }
}

ULONG
NTAPI
ObpGetHandleCountByHandleTable(PHANDLE_TABLE HandleTable)
{
    return HandleTable->HandleCount;
}

VOID
ObpGetNextHandleByProcessCount(PSYSTEM_HANDLE_TABLE_ENTRY_INFO pshi,
                               PEPROCESS Process,
                               int Count)
{
    ULONG P;
    //      KIRQL oldIrql;

    //      pshi->HandleValue;

    /*
    This will never work with ROS! M$, I guess uses 0 -> 65535.
    Ros uses 0 -> 4294967295!
    */

    P = (ULONG) Process->UniqueProcessId;
    pshi->UniqueProcessId = (USHORT) P;

    //      KeAcquireSpinLock( &Process->HandleTable.ListLock, &oldIrql );

    //      pshi->GrantedAccess;
    //      pshi->Object;
    //      pshi->TypeIndex;
    //      pshi->HandleAttributes;

    //      KeReleaseSpinLock( &Process->HandleTable.ListLock, oldIrql );

    return;
}
static VOID
ObpDecrementHandleCount(PVOID ObjectBody)
{
    POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);
    LONG NewHandleCount = InterlockedDecrement(&ObjectHeader->HandleCount);
    DPRINT("Header: %x\n", ObjectHeader);
    DPRINT("NewHandleCount: %x\n", NewHandleCount);
    DPRINT("OBJECT_HEADER_TO_NAME_INFO: %x\n", OBJECT_HEADER_TO_NAME_INFO(ObjectHeader));

    if ((ObjectHeader->Type != NULL) &&
        (ObjectHeader->Type->TypeInfo.CloseProcedure != NULL))
    {
        /* the handle count should be decremented but we pass the previous value
        to the callback */
        ObjectHeader->Type->TypeInfo.CloseProcedure(NULL, ObjectBody, 0, NewHandleCount + 1, NewHandleCount + 1);
    }

    /* Check if we should delete the object */
    ObpDeleteNameCheck(ObjectBody);
}

BOOLEAN
NTAPI
ObpSetHandleAttributes(IN PHANDLE_TABLE HandleTable,
                       IN OUT PHANDLE_TABLE_ENTRY HandleTableEntry,
                       IN PVOID Context)
{
    POBP_SET_HANDLE_ATTRIBUTES_CONTEXT SetHandleInfo =
        (POBP_SET_HANDLE_ATTRIBUTES_CONTEXT)Context;
    POBJECT_HEADER ObjectHeader = EX_HTE_TO_HDR(HandleTableEntry);
    PAGED_CODE();

    /* Don't allow operations on kernel objects */
    if ((ObjectHeader->Flags & OB_FLAG_KERNEL_MODE) &&
        (SetHandleInfo->PreviousMode != KernelMode))
    {
        /* Fail */
        return FALSE;
    }

    /* Check if making the handle inheritable */
    if (SetHandleInfo->Information.Inherit)
    {
        /* Set the flag. FIXME: Need to check if this is allowed */
        HandleTableEntry->ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
    }
    else
    {
        /* Otherwise this implies we're removing the flag */
        HandleTableEntry->ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
    }

    /* Check if making the handle protected */
    if (SetHandleInfo->Information.ProtectFromClose)
    {
        /* Set the flag */
        HandleTableEntry->ObAttributes |= EX_HANDLE_ENTRY_PROTECTFROMCLOSE;
    }
    else
    {
        /* Otherwise, remove it */
        HandleTableEntry->ObAttributes &= ~EX_HANDLE_ENTRY_PROTECTFROMCLOSE;
    }

    /* Return success */
    return TRUE;
}

static NTSTATUS
ObpDeleteHandle(HANDLE Handle)
{
    PHANDLE_TABLE_ENTRY HandleEntry;
    PVOID Body;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE ObjectTable;

    PAGED_CODE();

    DPRINT("ObpDeleteHandle(Handle %p)\n",Handle);

    ObjectTable = PsGetCurrentProcess()->ObjectTable;

    KeEnterCriticalRegion();

    HandleEntry = ExMapHandleToPointer(ObjectTable,
        Handle);
    if(HandleEntry != NULL)
    {
        if(HandleEntry->ObAttributes & EX_HANDLE_ENTRY_PROTECTFROMCLOSE)
        {
            ExUnlockHandleTableEntry(ObjectTable,
                HandleEntry);

            KeLeaveCriticalRegion();

            return STATUS_HANDLE_NOT_CLOSABLE;
        }

        ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
        Body = &ObjectHeader->Body;

        /* destroy and unlock the handle entry */
        ExDestroyHandleByEntry(ObjectTable,
            HandleEntry,
            Handle);

        ObpDecrementHandleCount(Body);

        KeLeaveCriticalRegion();

        return STATUS_SUCCESS;
    }
    KeLeaveCriticalRegion();
    return STATUS_INVALID_HANDLE;
}

NTSTATUS
NTAPI
ObDuplicateObject(PEPROCESS SourceProcess,
                  PEPROCESS TargetProcess,
                  HANDLE SourceHandle,
                  PHANDLE TargetHandle,
                  ACCESS_MASK DesiredAccess,
                  ULONG HandleAttributes,
                  ULONG Options)
{
    PHANDLE_TABLE_ENTRY SourceHandleEntry;
    HANDLE_TABLE_ENTRY NewHandleEntry;
    BOOLEAN AttachedToProcess = FALSE;
    PVOID ObjectBody;
    POBJECT_HEADER ObjectHeader;
    ULONG NewHandleCount;
    HANDLE NewTargetHandle;
    PEPROCESS CurrentProcess;
    KAPC_STATE ApcState;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if(SourceProcess == NULL ||
        ObIsKernelHandle(SourceHandle, ExGetPreviousMode()))
    {
        SourceProcess = PsInitialSystemProcess;
        SourceHandle = ObKernelHandleToHandle(SourceHandle);
    }

    CurrentProcess = PsGetCurrentProcess();

    KeEnterCriticalRegion();

    if (SourceProcess != CurrentProcess)
    {
        KeStackAttachProcess(&SourceProcess->Pcb,
            &ApcState);
        AttachedToProcess = TRUE;
    }
    SourceHandleEntry = ExMapHandleToPointer(SourceProcess->ObjectTable,
        SourceHandle);
    if (SourceHandleEntry == NULL)
    {
        if (AttachedToProcess)
        {
            KeUnstackDetachProcess(&ApcState);
        }

        KeLeaveCriticalRegion();
        return STATUS_INVALID_HANDLE;
    }

    ObjectHeader = EX_HTE_TO_HDR(SourceHandleEntry);
    ObjectBody = &ObjectHeader->Body;

    NewHandleEntry.Object = SourceHandleEntry->Object;
    if(HandleAttributes & OBJ_INHERIT)
        NewHandleEntry.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
    else
        NewHandleEntry.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
    NewHandleEntry.GrantedAccess = ((Options & DUPLICATE_SAME_ACCESS) ?
        SourceHandleEntry->GrantedAccess :
    DesiredAccess);
    if (Options & DUPLICATE_SAME_ACCESS)
    {
        NewHandleEntry.GrantedAccess = SourceHandleEntry->GrantedAccess;
    }
    else
    {
        if (DesiredAccess & GENERIC_ACCESS)
        {
            RtlMapGenericMask(&DesiredAccess,
                &ObjectHeader->Type->TypeInfo.GenericMapping);
        }
        NewHandleEntry.GrantedAccess = DesiredAccess;
    }

    /* reference the object so it doesn't get deleted after releasing the lock
    and before creating a new handle for it */
    ObReferenceObject(ObjectBody);

    /* increment the handle count of the object, it should always be >= 2 because
    we're holding a handle lock to this object! if the new handle count was
    1 here, we're in big trouble... it would've been safe to increment and
    check the handle count without using interlocked functions because the
    entry is locked, which means the handle count can't change. */
    NewHandleCount = InterlockedIncrement(&ObjectHeader->HandleCount);
    ASSERT(NewHandleCount >= 2);

    ExUnlockHandleTableEntry(SourceProcess->ObjectTable,
        SourceHandleEntry);

    if (AttachedToProcess)
    {
        KeUnstackDetachProcess(&ApcState);
        AttachedToProcess = FALSE;
    }

    if (TargetProcess != CurrentProcess)
    {
        KeStackAttachProcess(&TargetProcess->Pcb,
            &ApcState);
        AttachedToProcess = TRUE;
    }

    /* attempt to create the new handle */
    NewTargetHandle = ExCreateHandle(TargetProcess->ObjectTable,
        &NewHandleEntry);
    if (AttachedToProcess)
    {
        KeUnstackDetachProcess(&ApcState);
        AttachedToProcess = FALSE;
    }

    if (NewTargetHandle != NULL)
    {
        if (Options & DUPLICATE_CLOSE_SOURCE)
        {
            if (SourceProcess != CurrentProcess)
            {
                KeStackAttachProcess(&SourceProcess->Pcb,
                    &ApcState);
                AttachedToProcess = TRUE;
            }

            /* delete the source handle */
            ObpDeleteHandle(SourceHandle);

            if (AttachedToProcess)
            {
                KeUnstackDetachProcess(&ApcState);
            }
        }

        ObDereferenceObject(ObjectBody);

        *TargetHandle = NewTargetHandle;
    }
    else
    {
        /* decrement the handle count we previously incremented, but don't call the
        closing procedure because we're not closing a handle! */
        if(InterlockedDecrement(&ObjectHeader->HandleCount) == 0)
        {
            ObDereferenceObject(ObjectBody);
        }

        ObDereferenceObject(ObjectBody);
        Status = STATUS_UNSUCCESSFUL;
    }

    KeLeaveCriticalRegion();

    return Status;
}

static VOID STDCALL
SweepHandleCallback(PHANDLE_TABLE HandleTable,
                    PVOID Object,
                    ULONG GrantedAccess,
                    PVOID Context)
{
    POBJECT_HEADER ObjectHeader;
    PVOID ObjectBody;

    PAGED_CODE();

    ObjectHeader = EX_OBJ_TO_HDR(Object);
    ObjectBody = &ObjectHeader->Body;

    ObpDecrementHandleCount(ObjectBody);
}

static BOOLEAN STDCALL
DuplicateHandleCallback(PHANDLE_TABLE HandleTable,
                        PHANDLE_TABLE_ENTRY HandleTableEntry,
                        PVOID Context)
{
    POBJECT_HEADER ObjectHeader;
    BOOLEAN Ret = FALSE;

    PAGED_CODE();

    Ret = (HandleTableEntry->ObAttributes & EX_HANDLE_ENTRY_INHERITABLE) != 0;
    if(Ret)
    {
        ObjectHeader = EX_HTE_TO_HDR(HandleTableEntry);
        if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
        {
            ObReferenceObject(&ObjectHeader->Body);
        }
    }

    return Ret;
}

VOID
NTAPI
ObCreateHandleTable(PEPROCESS Parent,
                    BOOLEAN Inherit,
                    PEPROCESS Process)
                    /*
                    * FUNCTION: Creates a handle table for a process
                    * ARGUMENTS:
                    *       Parent = Parent process (or NULL if this is the first process)
                    *       Inherit = True if the process should inherit its parent's handles
                    *       Process = Process whose handle table is to be created
                    */
{
    PAGED_CODE();

    DPRINT("ObCreateHandleTable(Parent %x, Inherit %d, Process %x)\n",
        Parent,Inherit,Process);
    if(Parent != NULL)
    {
        Process->ObjectTable = ExDupHandleTable(Process,
            DuplicateHandleCallback,
            NULL,
            Parent->ObjectTable);
    }
    else
    {
        Process->ObjectTable = ExCreateHandleTable(Process);
    }
}


VOID
STDCALL
ObKillProcess(PEPROCESS Process)
{
    PAGED_CODE();

    /* FIXME - Temporary hack: sweep and destroy here, needs to be fixed!!! */
    ExSweepHandleTable(Process->ObjectTable,
        SweepHandleCallback,
        Process);
    ExDestroyHandleTable(Process->ObjectTable);
    Process->ObjectTable = NULL;
}


NTSTATUS
NTAPI
ObpCreateHandle(PVOID ObjectBody,
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
    PEPROCESS Process, CurrentProcess;
    POBJECT_HEADER ObjectHeader;
    HANDLE Handle;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE;

    PAGED_CODE();

    DPRINT("ObpCreateHandle(obj %p)\n",ObjectBody);

    ASSERT(ObjectBody);

    CurrentProcess = PsGetCurrentProcess();

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);

    /* check that this is a valid kernel pointer */
    ASSERT((ULONG_PTR)ObjectHeader & EX_HANDLE_ENTRY_LOCKED);

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
        NewEntry.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
    else
        NewEntry.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
    NewEntry.GrantedAccess = GrantedAccess;

    if ((HandleAttributes & OBJ_KERNEL_HANDLE) &&
        ExGetPreviousMode == KernelMode)
    {
        Process = PsInitialSystemProcess;
        if (Process != CurrentProcess)
        {
            KeStackAttachProcess(&Process->Pcb,
                &ApcState);
            AttachedToProcess = TRUE;
        }
    }
    else
    {
        Process = CurrentProcess;
        /* mask out the OBJ_KERNEL_HANDLE attribute */
        HandleAttributes &= ~OBJ_KERNEL_HANDLE;
    }

    Handle = ExCreateHandle(Process->ObjectTable,
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

        if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
        {
            ObReferenceObject(ObjectBody);
        }

        *HandleReturn = Handle;

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
                   IN POBJECT_TYPE ObjectType,
                   IN KPROCESSOR_MODE AccessMode,
                   IN PACCESS_STATE PassedAccessState,
                   IN ACCESS_MASK DesiredAccess,
                   IN OUT PVOID ParseContext,
                   OUT PHANDLE Handle)
{
    PVOID Object = NULL;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    NTSTATUS Status;
    OBP_LOOKUP_CONTEXT Context;
    AUX_DATA AuxData;
    PGENERIC_MAPPING GenericMapping = NULL;
    ACCESS_STATE AccessState;
    PAGED_CODE();

    /* Capture all the info */
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        AccessMode,
                                        TRUE,
                                        &ObjectCreateInfo,
                                        &ObjectName);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we didn't get an access state */
    if (!PassedAccessState)
    {
        /* Try to get the generic mapping if we can */
        if (ObjectType) GenericMapping = &ObjectType->TypeInfo.GenericMapping;

        /* Use our built-in access state */
        PassedAccessState = &AccessState;
        Status = SeCreateAccessState(&AccessState,
                                     &AuxData,
                                     DesiredAccess,
                                     GenericMapping);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    /* Get the security descriptor */
    if (ObjectCreateInfo.SecurityDescriptor)
    {
        /* Save it in the access state */
        PassedAccessState->SecurityDescriptor =
            ObjectCreateInfo.SecurityDescriptor;
    }

    /* Now do the lookup */
    Status = ObFindObject(ObjectCreateInfo.RootDirectory,
                          &ObjectName,
                          ObjectCreateInfo.Attributes,
                          AccessMode,
                          &Object,
                          ObjectType,
                          &Context,
                          PassedAccessState,
                          ObjectCreateInfo.SecurityQos,
                          ParseContext,
                          NULL);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Create the actual handle now */
    Status = ObpCreateHandle(Object,
                             DesiredAccess,
                             ObjectCreateInfo.Attributes,
                             Handle);

Cleanup:
    /* Dereference the object */
    if (Object) ObDereferenceObject(Object);

    /* Delete the access state */
    if (PassedAccessState == &AccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

    /* Release the object attributes and return status */
Quickie:
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);
    if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);
    return Status;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
ObOpenObjectByPointer(IN PVOID Object,
                      IN ULONG HandleAttributes,
                      IN PACCESS_STATE PassedAccessState,
                      IN ACCESS_MASK DesiredAccess,
                      IN POBJECT_TYPE ObjectType,
                      IN KPROCESSOR_MODE AccessMode,
                      OUT PHANDLE Handle)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the object */
    Status = ObReferenceObjectByPointer(Object,
                                        0,
                                        ObjectType,
                                        AccessMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the handle */
    Status = ObpCreateHandle(Object,
                             DesiredAccess,
                             HandleAttributes,
                             Handle);

    /* ROS Hack: Dereference the object and return */
    ObDereferenceObject(Object);
    return Status;
}

NTSTATUS STDCALL
ObFindHandleForObject(IN PEPROCESS Process,
                      IN PVOID Object,
                      IN POBJECT_TYPE ObjectType,
                      IN POBJECT_HANDLE_INFORMATION HandleInformation,
                      OUT PHANDLE HandleReturn)
{
    DPRINT("ObFindHandleForObject is unimplemented!\n");
    return STATUS_UNSUCCESSFUL;
}

/*++
* @name ObMakeTemporaryObject
* @implemented NT4
*
*     The ObMakeTemporaryObject routine <FILLMEIN>
*
* @param ObjectBody
*        <FILLMEIN>
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
NTAPI
ObMakeTemporaryObject(IN PVOID ObjectBody)
{
    ObpSetPermanentObject (ObjectBody, FALSE);
}

/*
* @implemented
*/
NTSTATUS 
STDCALL
ObInsertObject(IN PVOID Object,
               IN PACCESS_STATE PassedAccessState OPTIONAL,
               IN ACCESS_MASK DesiredAccess,
               IN ULONG AdditionalReferences,
               OUT PVOID* ReferencedObject OPTIONAL,
               OUT PHANDLE Handle)
{
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    POBJECT_HEADER Header;
    PVOID FoundObject = NULL;
    POBJECT_HEADER FoundHeader = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR NewSecurityDescriptor = NULL;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    OBP_LOOKUP_CONTEXT Context;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    PAGED_CODE();

    /* Get the Header and Create Info */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectCreateInfo = Header->ObjectCreateInfo;
    ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(Header);

    /* First try to find the Object */
    if (ObjectNameInfo && ObjectNameInfo->Name.Buffer)
    {
        Status = ObFindObject(ObjectCreateInfo->RootDirectory,
            &ObjectNameInfo->Name,
            ObjectCreateInfo->Attributes,
            KernelMode,
            &FoundObject,
            Header->Type,
            &Context,
            NULL,
            ObjectCreateInfo->SecurityQos,
            NULL,
            Object);
        if (!NT_SUCCESS(Status)) return Status;

        if (FoundObject)
        {
            DPRINT("Getting header: %x\n", FoundObject);
            FoundHeader = OBJECT_TO_OBJECT_HEADER(FoundObject);
        }
    }
    else
    {
        /*
         * OK, if we got here then that means we don't have a name,
         * so RemainingPath.Buffer/RemainingPath would've been NULL
         * under the old implemetantation, so just use NULL.
         * If remaining path wouldn't have been NULL, then we would've
         * called ObFindObject which already has this code.
         * We basically kill 3-4 hacks and add 2 new ones.
         */
        if ((Header->Type == IoFileObjectType) ||
            (Header->Type->TypeInfo.OpenProcedure != NULL))
        {
            DPRINT("About to call Open Routine\n");
            if (Header->Type == IoFileObjectType)
            {
                /* TEMPORARY HACK. DO NOT TOUCH -- Alex */
                DPRINT("Calling IopCreateFile: %x\n", FoundObject);
                Status = IopCreateFile(&Header->Body,
                                       FoundObject,
                                       NULL,
                                       NULL);
                DPRINT("Called IopCreateFile: %x\n", Status);
            }
            else if (Header->Type->TypeInfo.OpenProcedure)
            {
                DPRINT("Calling %x\n", Header->Type->TypeInfo.OpenProcedure);
                Status = Header->Type->TypeInfo.OpenProcedure(ObCreateHandle,
                                                              NULL,
                                                              &Header->Body,
                                                              0,
                                                              0);
            }

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Create Failed\n");
                if (FoundObject) ObDereferenceObject(FoundObject);
                return Status;
            }
        }
    }

    DPRINT("Security Assignment in progress\n");
    SeCaptureSubjectContext(&SubjectContext);

    /* Build the new security descriptor */
    Status = SeAssignSecurity((FoundHeader != NULL) ? FoundHeader->SecurityDescriptor : NULL,
        (ObjectCreateInfo != NULL) ? ObjectCreateInfo->SecurityDescriptor : NULL,
        &NewSecurityDescriptor,
        (Header->Type == ObDirectoryType),
        &SubjectContext,
        &Header->Type->TypeInfo.GenericMapping,
        PagedPool);

    if (NT_SUCCESS(Status))
    {
        DPRINT("NewSecurityDescriptor %p\n", NewSecurityDescriptor);

        if (Header->Type->TypeInfo.SecurityProcedure != NULL)
        {
            /* Call the security method */
            Status = Header->Type->TypeInfo.SecurityProcedure(&Header->Body,
                AssignSecurityDescriptor,
                0,
                NewSecurityDescriptor,
                NULL,
                NULL,
                NonPagedPool,
                NULL);
        }
        else
        {
            /* Assign the security descriptor to the object header */
            Status = ObpAddSecurityDescriptor(NewSecurityDescriptor,
                &Header->SecurityDescriptor);
            DPRINT("Object security descriptor %p\n", Header->SecurityDescriptor);
        }

        /* Release the new security descriptor */
        SeDeassignSecurity(&NewSecurityDescriptor);
    }

    DPRINT("Security Complete\n");
    SeReleaseSubjectContext(&SubjectContext);

    /* Create the Handle */
    /* HACKHACK: Because of ROS's incorrect startup, this can be called
    * without a valid Process until I finalize the startup patch,
    * so don't create a handle if this is the case. We also don't create
    * a handle if Handle is NULL when the Registry Code calls it, because
    * the registry code totally bastardizes the Ob and needs to be fixed
    */
    DPRINT("Creating handle\n");
    if (Handle != NULL)
    {
        Status = ObpCreateHandle(&Header->Body,
            DesiredAccess,
            ObjectCreateInfo->Attributes,
            Handle);
        DPRINT("handle Created: %d. refcount. handlecount %d %d\n",
            *Handle, Header->PointerCount, Header->HandleCount);
    }

    /* We can delete the Create Info now */
    Header->ObjectCreateInfo = NULL;
    ObpFreeAndReleaseCapturedAttributes(ObjectCreateInfo);

    DPRINT("Status %x\n", Status);
    return Status;
}

/*
* @implemented
*/
NTSTATUS STDCALL
NtDuplicateObject (IN	HANDLE		SourceProcessHandle,
                   IN	HANDLE		SourceHandle,
                   IN	HANDLE		TargetProcessHandle,
                   OUT	PHANDLE		TargetHandle  OPTIONAL,
                   IN	ACCESS_MASK	DesiredAccess,
                   IN	ULONG		HandleAttributes,
                   IN   ULONG		Options)
{
    PEPROCESS SourceProcess;
    PEPROCESS TargetProcess;
    PEPROCESS CurrentProcess;
    HANDLE hTarget;
    BOOLEAN AttachedToProcess = FALSE;
    KPROCESSOR_MODE PreviousMode;
    KAPC_STATE ApcState;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if(TargetHandle != NULL && PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(TargetHandle);
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

    Status = ObReferenceObjectByHandle(SourceProcessHandle,
        PROCESS_DUP_HANDLE,
        NULL,
        PreviousMode,
        (PVOID*)&SourceProcess,
        NULL);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Status = ObReferenceObjectByHandle(TargetProcessHandle,
        PROCESS_DUP_HANDLE,
        NULL,
        PreviousMode,
        (PVOID*)&TargetProcess,
        NULL);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(SourceProcess);
        return(Status);
    }

    CurrentProcess = PsGetCurrentProcess();

    /* Check for magic handle first */
    if (SourceHandle == NtCurrentThread() ||
        SourceHandle == NtCurrentProcess())
    {
        PVOID ObjectBody;
        POBJECT_TYPE ObjectType;

        ObjectType = (SourceHandle == NtCurrentThread()) ? PsThreadType : PsProcessType;

        Status = ObReferenceObjectByHandle(SourceHandle,
            0,
            ObjectType,
            PreviousMode,
            &ObjectBody,
            NULL);
        if(NT_SUCCESS(Status))
        {
            if (Options & DUPLICATE_SAME_ACCESS)
            {
                /* grant all access rights */
                DesiredAccess = ((ObjectType == PsThreadType) ? THREAD_ALL_ACCESS : PROCESS_ALL_ACCESS);
            }
            else
            {
                if (DesiredAccess & GENERIC_ACCESS)
                {
                    RtlMapGenericMask(&DesiredAccess,
                        &ObjectType->TypeInfo.GenericMapping);
                }
            }

            if (TargetProcess != CurrentProcess)
            {
                KeStackAttachProcess(&TargetProcess->Pcb,
                    &ApcState);
                AttachedToProcess = TRUE;
            }

            Status = ObpCreateHandle(ObjectBody,
                DesiredAccess,
                HandleAttributes,
                &hTarget);

            if (AttachedToProcess)
            {
                KeUnstackDetachProcess(&ApcState);
                AttachedToProcess = FALSE;
            }

            ObDereferenceObject(ObjectBody);

            if (Options & DUPLICATE_CLOSE_SOURCE)
            {
                if (SourceProcess != CurrentProcess)
                {
                    KeStackAttachProcess(&SourceProcess->Pcb,
                        &ApcState);
                    AttachedToProcess = TRUE;
                }

                ObpDeleteHandle(SourceHandle);

                if (AttachedToProcess)
                {
                    KeUnstackDetachProcess(&ApcState);
                }
            }
        }
    }
    else
    {
        Status = ObDuplicateObject(SourceProcess,
            TargetProcess,
            SourceHandle,
            &hTarget,
            DesiredAccess,
            HandleAttributes,
            Options);
    }

    ObDereferenceObject(TargetProcess);
    ObDereferenceObject(SourceProcess);

    if(NT_SUCCESS(Status) && TargetHandle != NULL)
    {
        _SEH_TRY
        {
            *TargetHandle = hTarget;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    return Status;
}

NTSTATUS STDCALL
NtClose(IN HANDLE Handle)
{
    PEPROCESS Process, CurrentProcess;
    BOOLEAN AttachedToProcess = FALSE;
    KAPC_STATE ApcState;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();
    CurrentProcess = PsGetCurrentProcess();

    if(ObIsKernelHandle(Handle, PreviousMode))
    {
        Process = PsInitialSystemProcess;
        Handle = ObKernelHandleToHandle(Handle);

        if (Process != CurrentProcess)
        {
            KeStackAttachProcess(&Process->Pcb,
                &ApcState);
            AttachedToProcess = TRUE;
        }
    }
    else
        Process = CurrentProcess;

    Status = ObpDeleteHandle(Handle);

    if (AttachedToProcess)
    {
        KeUnstackDetachProcess(&ApcState);
    }

    if (!NT_SUCCESS(Status))
    {
        if((PreviousMode != KernelMode) &&
            (CurrentProcess->ExceptionPort))
        {
            KeRaiseUserException(Status);
        }
        return Status;
    }

    return(STATUS_SUCCESS);
}

/*++
* @name NtMakeTemporaryObject
* @implemented NT4
*
*     The NtMakeTemporaryObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtMakeTemporaryObject(IN HANDLE ObjectHandle)
{
    PVOID ObjectBody;
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the object for DELETE access */
    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       DELETE,
                                       NULL,
                                       KeGetPreviousMode(),
                                       &ObjectBody,
                                       NULL);
    if (Status != STATUS_SUCCESS) return Status;

    /* Set it as temporary and dereference it */
    ObpSetPermanentObject(ObjectBody, FALSE);
    ObDereferenceObject(ObjectBody);
    return STATUS_SUCCESS;
}

/*++
* @name NtMakePermanentObject
* @implemented NT4
*
*     The NtMakePermanentObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtMakePermanentObject(IN HANDLE ObjectHandle)
{
    PVOID ObjectBody;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Make sure that the caller has SeCreatePermanentPrivilege */
    Status = SeSinglePrivilegeCheck(SeCreatePermanentPrivilege,
                                    PreviousMode);
    if (!NT_SUCCESS(Status)) return STATUS_PRIVILEGE_NOT_HELD;

    /* Reference the object */
    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       0,
                                       NULL,
                                       PreviousMode,
                                       &ObjectBody,
                                       NULL);
    if (Status != STATUS_SUCCESS) return Status;

    /* Set it as permanent and dereference it */
    ObpSetPermanentObject(ObjectBody, TRUE);
    ObDereferenceObject(ObjectBody);
    return STATUS_SUCCESS;
}
/* EOF */
