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

#ifdef _OBDEBUG_
#define OBTRACE DPRINT1
#else
#define OBTRACE DPRINT
#endif

/* UGLY FUNCTIONS ************************************************************/

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

/* PRIVATE FUNCTIONS *********************************************************/

/*++
* @name ObpDeleteNameCheck
*
*     The ObpDeleteNameCheck routine checks if a named object should be
*     removed from the object directory namespace.
*
* @param Object
*        Pointer to the object to check for possible removal.
*
* @return None.
*
* @remarks An object is removed if the following 4 criteria are met:
*          1) The object has 0 handles open
*          2) The object is in the directory namespace and has a name
*          3) The object is not permanent
*
*--*/
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
*     The ObpSetPermanentObject routine makes an sets or clears the permanent
*     flag of an object, thus making it either permanent or temporary.
*
* @param ObjectBody
*        Pointer to the object to make permanent or temporary.
*
* @param Permanent
*        Flag specifying which operation to perform.
*
* @return None.
*
* @remarks If the object is being made temporary, then it will be checked
*          as a candidate for immediate removal from the namespace.
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

/*++
* @name ObpDecrementHandleCount
*
*     The ObpDecrementHandleCount routine <FILLMEIN>
*
* @param ObjectBody
*        <FILLMEIN>.
*
* @param Process
*        <FILLMEIN>.
*
* @param GrantedAccess
*        <FILLMEIN>.
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
NTAPI
ObpDecrementHandleCount(IN PVOID ObjectBody,
                        IN PEPROCESS Process,
                        IN ACCESS_MASK GrantedAccess)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    LONG SystemHandleCount, ProcessHandleCount;

    /* Get the object type and header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);
    ObjectType = ObjectHeader->Type;
    OBTRACE("OBTRACE - %s - Decrementing count for: %p. HC LC %lx %lx\n",
            __FUNCTION__,
            ObjectBody,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* FIXME: The process handle count should be in the Handle DB. Investigate */
    SystemHandleCount = ObjectHeader->HandleCount;
    ProcessHandleCount = 0;

    /* Decrement the handle count */
    InterlockedDecrement(&ObjectHeader->HandleCount);

    /* Check if we have a close procedure */
    if (ObjectType->TypeInfo.CloseProcedure)
    {
        /* Call it */
        ObjectType->TypeInfo.CloseProcedure(Process,
                                            ObjectBody,
                                            GrantedAccess,
                                            ProcessHandleCount,
                                            SystemHandleCount);
    }

    /* Check if we should delete the object */
    ObpDeleteNameCheck(ObjectBody);

    /* Decrease the total number of handles for this type */
    ObjectType->TotalNumberOfHandles--;
    OBTRACE("OBTRACE - %s - Decremented count for: %p. HC LC %lx %lx\n",
            __FUNCTION__,
            ObjectBody,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
}

/*++
* @name ObpDeleteHandle
*
*     The ObpDeleteHandle routine <FILLMEIN>
*
* @param Handle
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObpDeleteHandle(HANDLE Handle)
{
    PHANDLE_TABLE_ENTRY HandleEntry;
    PVOID Body;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE ObjectTable;
    ACCESS_MASK GrantedAccess;
    NTSTATUS Status = STATUS_INVALID_HANDLE;
    PAGED_CODE();

    /*
     * Get the object table of the current process/
     * NOTE: We might actually be attached to the system process
     */
    ObjectTable = PsGetCurrentProcess()->ObjectTable;

    /* Enter a critical region while touching the handle locks */
    KeEnterCriticalRegion();

    /* Get the entry for this handle */
    HandleEntry = ExMapHandleToPointer(ObjectTable, Handle);
    if(HandleEntry)
    {
        /* Get the object data */
        ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
        ObjectType = ObjectHeader->Type;
        Body = &ObjectHeader->Body;
        GrantedAccess = HandleEntry->GrantedAccess;
        OBTRACE("OBTRACE - %s - Deleting handle: %lx for %p. HC LC %lx %lx\n",
                __FUNCTION__,
                Handle,
                Body,
                ObjectHeader->HandleCount,
                ObjectHeader->PointerCount);

        /* Check if the object has an Okay To Close procedure */
        if (ObjectType->TypeInfo.OkayToCloseProcedure)
        {
            /* Call it and check if it's not letting us close it */
            if (!ObjectType->TypeInfo.OkayToCloseProcedure(PsGetCurrentProcess(),
                                                           Body,
                                                           Handle))
            {
                /* Fail */
                ExUnlockHandleTableEntry(ObjectTable, HandleEntry);
                KeLeaveCriticalRegion();
                return STATUS_HANDLE_NOT_CLOSABLE;
            }
        }

        /* The callback allowed us to close it, but does the handle itself? */
        if(HandleEntry->ObAttributes & EX_HANDLE_ENTRY_PROTECTFROMCLOSE)
        {
            /* Fail */
            ExUnlockHandleTableEntry(ObjectTable, HandleEntry);
            KeLeaveCriticalRegion();
            return STATUS_HANDLE_NOT_CLOSABLE;
        }

        /* Destroy and unlock the handle entry */
        ExDestroyHandleByEntry(ObjectTable, HandleEntry, Handle);

        /* Now decrement the handle count */
        ObpDecrementHandleCount(Body, PsGetCurrentProcess(), GrantedAccess);
        Status = STATUS_SUCCESS;
        OBTRACE("OBTRACE - %s - Deleted handle: %lx for %p. HC LC %lx %lx\n",
                __FUNCTION__,
                Handle,
                Body,
                ObjectHeader->HandleCount,
                ObjectHeader->PointerCount);
    }

    /* Leave the critical region and return the status */
    KeLeaveCriticalRegion();
    return Status;
}

NTSTATUS
NTAPI
ObpIncrementHandleCount(IN PVOID Object,
                        IN PACCESS_STATE AccessState,
                        IN KPROCESSOR_MODE AccessMode,
                        IN ULONG HandleAttributes,
                        IN PEPROCESS Process,
                        IN OB_OPEN_REASON OpenReason)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    ULONG ProcessHandleCount;

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE("OBTRACE - %s - Incrementing count for: %p. Reason: %lx. HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Check if we're opening an existing handle */
    if (OpenReason == ObOpenHandle)
    {
        /*
         * FIXME: Do validation as described in Chapter 8
         * of Windows Internals 4th.
         */
#if 0
        if (!ObCheckObjectAccess(Object,
                                 AccessState,
                                 TRUE,
                                 AccessMode,
                                 &Status))
        {
            return Status;
        }
#endif
    }
    else if (OpenReason == ObCreateHandle)
    {
        /* Convert MAXIMUM_ALLOWED to GENERIC_ALL */
        if (AccessState->RemainingDesiredAccess & MAXIMUM_ALLOWED)
        {
            /* Mask out MAXIMUM_ALLOWED and stick GENERIC_ALL instead */
            AccessState->RemainingDesiredAccess &= ~MAXIMUM_ALLOWED;
            AccessState->RemainingDesiredAccess |= GENERIC_ALL;
        }

        /* Check if we have to map the GENERIC mask */
        if (AccessState->RemainingDesiredAccess & GENERIC_ACCESS)
        {
            /* Map it to the correct access masks */
            RtlMapGenericMask(&AccessState->RemainingDesiredAccess,
                              &ObjectType->TypeInfo.GenericMapping);
        }
    }

    /* Increase the handle count */
    if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
    {
        /*
         * FIXME: Is really needed? Perhaps we should instead take
         * advantage of the AddtionalReferences parameter to add the
         * bias when required. This might be the source of the mysterious
         * ReactOS bug where ObInsertObject *requires* an immediate dereference
         * even in a success case.
         * Will have to think more about this when doing the Increment/Create
         * split later.
         */
        ObReferenceObject(Object);
    }

    /* FIXME: Use the Handle Database */
    ProcessHandleCount = 0;

    /* Check if we have an open procedure */
    if (ObjectType->TypeInfo.OpenProcedure)
    {
#if 0
        /* Call it */
        ObjectType->TypeInfo.OpenProcedure(OpenReason,
                                           Process,
                                           Object,
                                           AccessState->PreviouslyGrantedAccess,
                                           ProcessHandleCount);
#endif
    }

    /* Increase total number of handles */
    ObjectType->TotalNumberOfHandles++;
    OBTRACE("OBTRACE - %s - Incremented count for: %p. Reason: %lx HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObpCreateHandle(IN OB_OPEN_REASON OpenReason, // Gloomy says this is "enables Security" if == 1.
                                              // since this function *has* to call ObpIncrementHandleCount,
                                              // which needs to somehow know the OpenReason, and since
                                              // ObOpenHandle == 1, I'm guessing this is actually the
                                              // OpenReason. Also makes sense since this function is shared
                                              // by Duplication, Creation and Opening.
                IN PVOID Object,
                IN POBJECT_TYPE Type OPTIONAL,
                IN PACCESS_STATE AccessState,
                IN ULONG AdditionalReferences,
                IN ULONG HandleAttributes,
                IN KPROCESSOR_MODE AccessMode,
                OUT PVOID *ReturnedObject,
                OUT PHANDLE ReturnedHandle)
{
    HANDLE_TABLE_ENTRY NewEntry;
    POBJECT_HEADER ObjectHeader;
    HANDLE Handle;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE;
    POBJECT_TYPE ObjectType;
    PVOID HandleTable;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE("OBTRACE - %s - Creating handle for: %p. Reason: %lx. HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Check if the types match */
    if ((Type) && (ObjectType != Type))
    {
        /* They don't; fail */
        DPRINT1("Type mismatch: %wZ, %wZ\n", &ObjectType->Name, &Type->Name);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Check if this is a kernel handle */
    if ((HandleAttributes & OBJ_KERNEL_HANDLE) && (AccessMode == KernelMode))
    {
        /* Set the handle table */
        HandleTable = ObpKernelHandleTable;

        /* Check if we're not in the system process */
        if (PsGetCurrentProcess() != PsInitialSystemProcess)
        {
            /* Attach to the system process */
            KeStackAttachProcess(&PsInitialSystemProcess->Pcb, &ApcState);
            AttachedToProcess = TRUE;
        }
    }
    else
    {
        /* Get the current handle table */
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }

    /* Increment the handle count */
    Status = ObpIncrementHandleCount(Object,
                                     AccessState,
                                     AccessMode,
                                     HandleAttributes,
                                     PsGetCurrentProcess(),
                                     OpenReason);
    if (!NT_SUCCESS(Status))
    {
        /*
         * We failed (meaning security failure, according to NT Internals)
         * detach and return
         */
        if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);
        return Status;
    }

    /* Save the object header (assert its validity too) */
    ASSERT((ULONG_PTR)ObjectHeader & EX_HANDLE_ENTRY_LOCKED);
    NewEntry.Object = ObjectHeader;

    /* Mask out the internal attributes */
    NewEntry.ObAttributes |= HandleAttributes &
                             (EX_HANDLE_ENTRY_PROTECTFROMCLOSE |
                              EX_HANDLE_ENTRY_INHERITABLE |
                              EX_HANDLE_ENTRY_AUDITONCLOSE);

    /* Save the access mask */
    NewEntry.GrantedAccess = AccessState->RemainingDesiredAccess |
                             AccessState->PreviouslyGrantedAccess;

    /*
     * Create the actual handle. We'll need to do this *after* calling
     * ObpIncrementHandleCount to make sure that Object Security is valid
     * (specified in Gl00my documentation on Ob)
     */
    OBTRACE("OBTRACE - %s - Handle Properties: [%p-%lx-%lx]\n",
            __FUNCTION__,
            NewEntry.Object, NewEntry.ObAttributes & 3, NewEntry.GrantedAccess);
    Handle = ExCreateHandle(HandleTable, &NewEntry);

     /* Detach if needed */
    if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);

    /* Make sure we got a handle */
    if (Handle)
    {
        /* Handle extra references */
        while (AdditionalReferences--)
        {
            /* Increment the count */
            InterlockedIncrement(&ObjectHeader->PointerCount);
        }

        /* Check if this was a kernel handle */
        if (HandleAttributes & OBJ_KERNEL_HANDLE)
        {
            /* Set the kernel handle bit */
            Handle = ObMarkHandleAsKernelHandle(Handle);
        }

        /* Return handle and object */
        *ReturnedHandle = Handle;
        if (ReturnedObject) *ReturnedObject = Object;
        OBTRACE("OBTRACE - %s - Returning Handle: %lx HC LC %lx %lx\n",
                __FUNCTION__,
                Handle,
                ObjectHeader->HandleCount,
                ObjectHeader->PointerCount);
        return STATUS_SUCCESS;
    }

    /* Decrement the handle count and detach */
    ObpDecrementHandleCount(&ObjectHeader->Body,
                            PsGetCurrentProcess(),
                            NewEntry.GrantedAccess);
    return STATUS_INSUFFICIENT_RESOURCES;
}

/*++
* @name ObpSetHandleAttributes
*
*     The ObpSetHandleAttributes routine <FILLMEIN>
*
* @param HandleTable
*        <FILLMEIN>.
*
* @param HandleTableEntry
*        <FILLMEIN>.
*
* @param Context
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
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

VOID
NTAPI
ObpCloseHandleCallback(IN PHANDLE_TABLE HandleTable,
                       IN PVOID Object,
                       IN ULONG GrantedAccess,
                       IN PVOID Context)
{
    PAGED_CODE();

    /* Simply decrement the handle count */
    ObpDecrementHandleCount(&EX_OBJ_TO_HDR(Object)->Body,
                            PsGetCurrentProcess(),
                            GrantedAccess);
}

BOOLEAN
NTAPI
ObpDuplicateHandleCallback(IN PHANDLE_TABLE HandleTable,
                           IN PHANDLE_TABLE_ENTRY HandleTableEntry,
                           IN PVOID Context)
{
    POBJECT_HEADER ObjectHeader;
    BOOLEAN Ret = FALSE;
    ACCESS_STATE AccessState;
    NTSTATUS Status;
    PAGED_CODE();

    /* Make sure that the handle is inheritable */
    Ret = (HandleTableEntry->ObAttributes & EX_HANDLE_ENTRY_INHERITABLE) != 0;
    if(Ret)
    {
        /* Get the object header */
        ObjectHeader = EX_HTE_TO_HDR(HandleTableEntry);

        /* Setup the access state */
        AccessState.PreviouslyGrantedAccess = HandleTableEntry->GrantedAccess;

        /* Call the shared routine for incrementing handles */
        Status = ObpIncrementHandleCount(&ObjectHeader->Body,
                                         &AccessState,
                                         KernelMode,
                                         HandleTableEntry->ObAttributes,
                                         PsGetCurrentProcess(),
                                         ObInheritHandle);
        if (!NT_SUCCESS(Status))
        {
            /* Return failure */
            Ret = FALSE;
        }
        else
        {
            /* Otherwise increment the pointer count */
            InterlockedIncrement(&ObjectHeader->PointerCount);
        }
    }

    /* Return duplication result */
    return Ret;
}

NTSTATUS
NTAPI
ObpCreateHandleTable(IN PEPROCESS Parent,
                     IN PEPROCESS Process)
{
    PHANDLE_TABLE HandleTable;
    PAGED_CODE();

    /* Check if we have a parent */
    if (Parent)
    {
        /* Duplicate the parent's */
        HandleTable = ExDupHandleTable(Process,
                                       ObpDuplicateHandleCallback,
                                       NULL,
                                       Parent->ObjectTable);
    }
    else
    {
        /* Create a new one */
        HandleTable = ExCreateHandleTable(Process);
    }

    /* Now write it and make sure we got one */
    Process->ObjectTable = HandleTable;
    if (!HandleTable) return STATUS_INSUFFICIENT_RESOURCES;

    /* If we got here then the table was created OK */
    return STATUS_SUCCESS;
}

VOID
NTAPI
ObKillProcess(IN PEPROCESS Process)
{
    PAGED_CODE();

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Sweep the handle table to close all handles */
    ExSweepHandleTable(Process->ObjectTable,
                       ObpCloseHandleCallback,
                       Process);

    /* Destroy the table and leave the critical region */
    ExDestroyHandleTable(Process->ObjectTable);
    KeLeaveCriticalRegion();

    /* Clear the object table */
    Process->ObjectTable = NULL;
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
    POBJECT_HEADER ObjectHeader;
    AUX_DATA AuxData;
    PGENERIC_MAPPING GenericMapping = NULL;
    ACCESS_STATE AccessState;
    OB_OPEN_REASON OpenReason;
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

    /* Check if this object has create information */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    if (ObjectHeader->Flags & OB_FLAG_CREATE_INFO)
    {
        /* Then we are creating a new handle */
        OpenReason = ObCreateHandle;
    }
    else
    {
        /* Otherwise, we are merely opening it */
        OpenReason = ObOpenHandle;
    }

    /* Create the actual handle now */
    Status = ObpCreateHandle(OpenReason,
                             Object,
                             ObjectType,
                             PassedAccessState,
                             0,
                             ObjectCreateInfo.Attributes,
                             AccessMode,
                             NULL,
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
    OBTRACE("OBTRACE: %s returning Object with PC S: %lx %lx\n",
            __FUNCTION__,
            OBJECT_TO_OBJECT_HEADER(Object)->PointerCount,
            Status);
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
    ACCESS_STATE AccessState;
    AUX_DATA AuxData;
    PAGED_CODE();

    /* Reference the object */
    Status = ObReferenceObjectByPointer(Object,
                                        0,
                                        ObjectType,
                                        AccessMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we didn't get an access state */
    if (!PassedAccessState)
    {
        /* Use our built-in access state */
        PassedAccessState = &AccessState;
        Status = SeCreateAccessState(&AccessState,
                                     &AuxData,
                                     DesiredAccess,
                                     &ObjectType->TypeInfo.GenericMapping);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(Object);
            return Status;
        }
    }

    /* Create the handle */
    Status = ObpCreateHandle(ObOpenHandle,
                             Object,
                             ObjectType,
                             PassedAccessState,
                             0,
                             HandleAttributes,
                             AccessMode,
                             NULL,
                             Handle);

    /* Delete the access state */
    if (PassedAccessState == &AccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

    /* ROS Hack: Dereference the object and return */
    ObDereferenceObject(Object);

    /* Return */
    OBTRACE("OBTRACE: %s returning Object with PC S: %lx %lx\n",
            __FUNCTION__,
            OBJECT_TO_OBJECT_HEADER(Object)->PointerCount,
            Status);
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
NTAPI
ObInsertObject(IN PVOID Object,
               IN PACCESS_STATE PassedAccessState OPTIONAL,
               IN ACCESS_MASK DesiredAccess,
               IN ULONG AdditionalReferences,
               OUT PVOID* ReferencedObject OPTIONAL,
               OUT PHANDLE Handle)
{
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    POBJECT_HEADER Header;
    POBJECT_TYPE ObjectType;
    PVOID FoundObject = NULL;
    POBJECT_HEADER FoundHeader = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR DirectorySd = NULL;
    BOOLEAN SdAllocated;
    OBP_LOOKUP_CONTEXT Context;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    ACCESS_STATE AccessState;
    AUX_DATA AuxData;
    BOOLEAN IsNamed = FALSE;
    OB_OPEN_REASON OpenReason = ObCreateHandle;
    PAGED_CODE();

    /* Get the Header and Create Info */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectCreateInfo = Header->ObjectCreateInfo;
    ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(Header);
    ObjectType = Header->Type;

    /* Check if we didn't get an access state */
    if (!PassedAccessState)
    {
        /* Use our built-in access state */
        PassedAccessState = &AccessState;
        Status = SeCreateAccessState(&AccessState,
                                     &AuxData,
                                     DesiredAccess,
                                     &ObjectType->TypeInfo.GenericMapping);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(Object);
            return Status;
        }
    }

    /* Check if this is an named object */
    if ((ObjectNameInfo) && (ObjectNameInfo->Name.Buffer)) IsNamed = TRUE;

    /* Check if the object is named */
    if (IsNamed)
    {
        /* Look it up */
        Status = ObFindObject(ObjectCreateInfo->RootDirectory,
                              &ObjectNameInfo->Name,
                              ObjectCreateInfo->Attributes,
                              KernelMode,
                              &FoundObject,
                              ObjectType,
                              &Context,
                              PassedAccessState,
                              ObjectCreateInfo->SecurityQos,
                              ObjectCreateInfo->ParseContext,
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
        if (Header->Type == IoFileObjectType)
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
        }
    }

    /* Check if it's named or forces security */
    if ((IsNamed) || (ObjectType->TypeInfo.SecurityRequired))
    {
        /* Make sure it's inserted into an object directory */
        if ((ObjectNameInfo) && (ObjectNameInfo->Directory))
        {
            /* Get the current descriptor */
            ObGetObjectSecurity(ObjectNameInfo->Directory,
                                &DirectorySd,
                                &SdAllocated);
        }

        /* Now assign it */
        Status = ObAssignSecurity(PassedAccessState,
                                  DirectorySd,
                                  Object,
                                  ObjectType);

        /* Check if we captured one */
        if (DirectorySd)
        {
            /* We did, release it */
            DPRINT1("Here\n");
            ObReleaseObjectSecurity(DirectorySd, SdAllocated);
        }
        else if (NT_SUCCESS(Status))
        {
            /* Other we didn't, but we were able to use the current SD */
            SeReleaseSecurityDescriptor(ObjectCreateInfo->SecurityDescriptor,
                                        ObjectCreateInfo->ProbeMode,
                                        TRUE);

            /* Clear the current one */
            PassedAccessState->SecurityDescriptor =
                ObjectCreateInfo->SecurityDescriptor = NULL;
        }
    }

    /* Check if anything until now failed */
    if (!NT_SUCCESS(Status))
    {
        /* We failed, dereference the object and delete the access state */
        ObDereferenceObject(Object);
        if (PassedAccessState == &AccessState)
        {
            /* We used a local one; delete it */
            SeDeleteAccessState(PassedAccessState);
        }

        /* Return failure code */
        return Status;
    }

    /* HACKHACK: Because of ROS's incorrect startup, this can be called
    * without a valid Process until I finalize the startup patch,
    * so don't create a handle if this is the case. We also don't create
    * a handle if Handle is NULL when the Registry Code calls it, because
    * the registry code totally bastardizes the Ob and needs to be fixed
    */
    if (Handle)
    {
        /* Create the handle */
        Status = ObpCreateHandle(OpenReason,
                                 &Header->Body,
                                 NULL,
                                 PassedAccessState,
                                 AdditionalReferences + 1,
                                 ObjectCreateInfo->Attributes,
                                 ExGetPreviousMode(),
                                 NULL,
                                 Handle);
    }

    /* We can delete the Create Info now */
    Header->ObjectCreateInfo = NULL;
    ObpFreeAndReleaseCapturedAttributes(ObjectCreateInfo);

    /* Check if creating the handle failed */
    if (!NT_SUCCESS(Status))
    {
        /* If the object had a name, backout everything */
        if (IsNamed) ObpDeleteNameCheck(Object);
    }

    /* Remove the extra keep-alive reference */
    //ObDereferenceObject(Object); FIXME: Will require massive changes

    /* Check if we created our own access state */
    if (PassedAccessState == &AccessState)
    {
        /* We used a local one; delete it */
        SeDeleteAccessState(PassedAccessState);
    }

    /* Return failure code */
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
    ACCESS_STATE AccessState;
    AUX_DATA AuxData;
    PACCESS_STATE PassedAccessState = NULL;

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

            /* Use our built-in access state */
            PassedAccessState = &AccessState;
            Status = SeCreateAccessState(&AccessState,
                                         &AuxData,
                                         DesiredAccess,
                                         &ObjectType->TypeInfo.GenericMapping);

            /* Add a new handle */
            Status = ObpIncrementHandleCount(ObjectBody,
                                             PassedAccessState,
                                             PreviousMode,
                                             HandleAttributes,
                                             PsGetCurrentProcess(),
                                             ObDuplicateHandle);

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
