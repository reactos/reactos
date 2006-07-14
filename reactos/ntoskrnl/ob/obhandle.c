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

NTSTATUS
NTAPI
ObpChargeQuotaForObject(IN POBJECT_HEADER ObjectHeader,
                        IN POBJECT_TYPE ObjectType)
{
    POBJECT_HEADER_QUOTA_INFO ObjectQuota;
    ULONG PagedPoolCharge, NonPagedPoolCharge;
    PEPROCESS Process;

    /* Get quota information */
    ObjectQuota = OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader);

    /* Check if this is a new object */
    if (ObjectHeader->Flags & OB_FLAG_CREATE_INFO)
    {
        /* Remove the flag */
        ObjectHeader->Flags &= ~ OB_FLAG_CREATE_INFO;
        if (ObjectQuota)
        {
            /* We have a quota, get the charges */
            PagedPoolCharge = ObjectQuota->PagedPoolCharge;
            NonPagedPoolCharge = ObjectQuota->NonPagedPoolCharge;
        }
        else
        {
            /* Get it from the object type */
            PagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
            NonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
        }

        /*
         * Charge the quota
         * FIXME: This is a *COMPLETE* guess and probably defintely not the way to do this.
         */
        Process = PsGetCurrentProcess();
        Process->QuotaBlock->QuotaEntry[PagedPool].Usage += PagedPoolCharge;
        Process->QuotaBlock->QuotaEntry[NonPagedPool].Usage += NonPagedPoolCharge;
        ObjectHeader->QuotaBlockCharged = Process->QuotaBlock;
    }

    /* Return success */
    return STATUS_SUCCESS;
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
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Decrementing count for: %p. HC LC %lx %lx\n",
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
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Decremented count for: %p. HC LC %lx %lx\n",
            __FUNCTION__,
            ObjectBody,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
}

/*++
* @name ObpCloseHandleTableEntry
*
*     The ObpCloseHandleTableEntry routine <FILLMEIN>
*
* @param HandleTable
*        <FILLMEIN>.
*
* @param HandleEntry
*        <FILLMEIN>.
*
* @param Handle
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @param IgnoreHandleProtection
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObpCloseHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                         IN PHANDLE_TABLE_ENTRY HandleEntry,
                         IN HANDLE Handle,
                         IN KPROCESSOR_MODE AccessMode,
                         IN BOOLEAN IgnoreHandleProtection)
{
    PVOID Body;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER ObjectHeader;
    ACCESS_MASK GrantedAccess;
    PAGED_CODE();

    /* Get the object data */
    ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
    ObjectType = ObjectHeader->Type;
    Body = &ObjectHeader->Body;
    GrantedAccess = HandleEntry->GrantedAccess;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closing handle: %lx for %p. HC LC %lx %lx\n",
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
                                                       Handle,
                                                       AccessMode))
        {
            /* Fail */
            ExUnlockHandleTableEntry(HandleTable, HandleEntry);
            return STATUS_HANDLE_NOT_CLOSABLE;
        }
    }

    /* The callback allowed us to close it, but does the handle itself? */
    if ((HandleEntry->ObAttributes & EX_HANDLE_ENTRY_PROTECTFROMCLOSE) &&
        !(IgnoreHandleProtection))
    {
        /* It doesn't, are we from user mode? */
        if (AccessMode != KernelMode)
        {
            /* We are! Unlock the entry */
            ExUnlockHandleTableEntry(HandleTable, HandleEntry);

            /* Make sure we have an exception port */
            if (PsGetCurrentProcess()->ExceptionPort)
            {
                /* Raise an exception */
                return KeRaiseUserException(STATUS_HANDLE_NOT_CLOSABLE);
            }
            else
            {
                /* Return the error isntead */
                return STATUS_HANDLE_NOT_CLOSABLE;
            }
        }

        /* Otherwise, we are kernel mode, so unlock the entry and return */
        ExUnlockHandleTableEntry(HandleTable, HandleEntry);
        return STATUS_HANDLE_NOT_CLOSABLE;
    }

    /* Destroy and unlock the handle entry */
    ExDestroyHandleByEntry(HandleTable, HandleEntry, Handle);

    /* Now decrement the handle count */
    ObpDecrementHandleCount(Body, PsGetCurrentProcess(), GrantedAccess);

    /* Dereference the object as well */
    ASSERT(ObjectHeader->Type);
    ASSERT(ObjectHeader->PointerCount != 0xCCCCCCCC);
    if (!wcscmp(ObjectHeader->Type->Name.Buffer, L"Key"))
    {
        //
        // WE DONT CLOSE REGISTRY HANDLES BECAUSE CM IS BRAINDEAD
        //
        DPRINT("NOT CLOSING THE KEY\n");
    }
    else
    {
        ObDereferenceObject(Body);
    }

    /* Return to caller */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closed handle: %lx for %p. HC LC %lx %lx\n",
            __FUNCTION__,
            Handle,
            Body,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
    return STATUS_SUCCESS;
}

/*++
* @name ObpIncrementHandleCount
*
*     The ObpIncrementHandleCount routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>.
*
* @param AccessState
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @param HandleAttributes
*        <FILLMEIN>.
*
* @param Process
*        <FILLMEIN>.
*
* @param OpenReason
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
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
    NTSTATUS Status;

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incrementing count for: %p. Reason: %lx. HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Charge quota and remove the creator info flag */
    Status = ObpChargeQuotaForObject(ObjectHeader, ObjectType);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we're opening an existing handle */
    if (OpenReason == ObOpenHandle)
    {
        /* Validate the caller's access to this object */
        if (!ObCheckObjectAccess(Object,
                                 AccessState,
                                 TRUE,
                                 AccessMode,
                                 &Status))
        {
            /* Access was denied, so fail */
            return Status;
        }
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
    InterlockedIncrement(&ObjectHeader->HandleCount);

    /* FIXME: Use the Handle Database */
    ProcessHandleCount = 0;

    /* Check if we have an open procedure */
    if (ObjectType->TypeInfo.OpenProcedure)
    {
        /* Call it */
        ObjectType->TypeInfo.OpenProcedure(OpenReason,
                                           Process,
                                           Object,
                                           AccessState->PreviouslyGrantedAccess,
                                           ProcessHandleCount);
    }

    /* Increase total number of handles */
    ObjectType->TotalNumberOfHandles++;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incremented count for: %p. Reason: %lx HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
    return STATUS_SUCCESS;
}

/*++
* @name ObpIncrementUnnamedHandleCount
*
*     The ObpIncrementUnnamedHandleCount routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>.
*
* @param AccessState
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @param HandleAttributes
*        <FILLMEIN>.
*
* @param Process
*        <FILLMEIN>.
*
* @param OpenReason
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObpIncrementUnnamedHandleCount(IN PVOID Object,
                               IN PACCESS_MASK DesiredAccess,
                               IN KPROCESSOR_MODE AccessMode,
                               IN ULONG HandleAttributes,
                               IN PEPROCESS Process)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    ULONG ProcessHandleCount;
    NTSTATUS Status;

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incrementing count for: %p. UNNAMED. HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Charge quota and remove the creator info flag */
    Status = ObpChargeQuotaForObject(ObjectHeader, ObjectType);
    if (!NT_SUCCESS(Status)) return Status;

    /* Convert MAXIMUM_ALLOWED to GENERIC_ALL */
    if (*DesiredAccess & MAXIMUM_ALLOWED)
    {
        /* Mask out MAXIMUM_ALLOWED and stick GENERIC_ALL instead */
        *DesiredAccess &= ~MAXIMUM_ALLOWED;
        *DesiredAccess |= GENERIC_ALL;
    }

    /* Check if we have to map the GENERIC mask */
    if (*DesiredAccess & GENERIC_ACCESS)
    {
        /* Map it to the correct access masks */
        RtlMapGenericMask(DesiredAccess,
                          &ObjectType->TypeInfo.GenericMapping);
    }

    /* Increase the handle count */
    InterlockedIncrement(&ObjectHeader->HandleCount);

    /* FIXME: Use the Handle Database */
    ProcessHandleCount = 0;

    /* Check if we have an open procedure */
    if (ObjectType->TypeInfo.OpenProcedure)
    {
        /* Call it */
        ObjectType->TypeInfo.OpenProcedure(ObCreateHandle,
                                           Process,
                                           Object,
                                           *DesiredAccess,
                                           ProcessHandleCount);
    }

    /* Increase total number of handles */
    ObjectType->TotalNumberOfHandles++;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incremented count for: %p. UNNAMED HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
    return STATUS_SUCCESS;
}

/*++
* @name ObpCreateUnnamedHandle
*
*     The ObpCreateUnnamedHandle routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>.
*
* @param DesiredAccess
*        <FILLMEIN>.
*
* @param AdditionalReferences
*        <FILLMEIN>.
*
* @param HandleAttributes
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @param ReturnedObject
*        <FILLMEIN>.
*
* @param ReturnedHandle
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObpCreateUnnamedHandle(IN PVOID Object,
                       IN ACCESS_MASK DesiredAccess,
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
    PVOID HandleTable;
    NTSTATUS Status;
    ULONG i;
    PAGED_CODE();

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Creating handle for: %p. UNNAMED. HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

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
    Status = ObpIncrementUnnamedHandleCount(Object,
                                            &DesiredAccess,
                                            AccessMode,
                                            HandleAttributes,
                                            PsGetCurrentProcess());
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
    NewEntry.GrantedAccess = DesiredAccess;

    /* Handle extra references */
    if (AdditionalReferences)
    {
        /* Make a copy in case we fail later below */
        i = AdditionalReferences;
        while (i--)
        {
            /* Increment the count */
            InterlockedIncrement(&ObjectHeader->PointerCount);
        }
    }

    /*
     * Create the actual handle. We'll need to do this *after* calling
     * ObpIncrementHandleCount to make sure that Object Security is valid
     * (specified in Gl00my documentation on Ob)
     */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Handle Properties: [%p-%lx-%lx]\n",
            __FUNCTION__,
            NewEntry.Object, NewEntry.ObAttributes & 3, NewEntry.GrantedAccess);
    Handle = ExCreateHandle(HandleTable, &NewEntry);

     /* Detach if needed */
    if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);

    /* Make sure we got a handle */
    if (Handle)
    {
        /* Check if this was a kernel handle */
        if (HandleAttributes & OBJ_KERNEL_HANDLE)
        {
            /* Set the kernel handle bit */
            Handle = ObMarkHandleAsKernelHandle(Handle);
        }

        /* Return handle and object */
        *ReturnedHandle = Handle;
        if (ReturnedObject) *ReturnedObject = Object;
        OBTRACE(OB_HANDLE_DEBUG,
                "%s - Returning Handle: %lx HC LC %lx %lx\n",
                __FUNCTION__,
                Handle,
                ObjectHeader->HandleCount,
                ObjectHeader->PointerCount);
        return STATUS_SUCCESS;
    }

    /* Handle extra references */
    while (AdditionalReferences--)
    {
        /* Decrement the count */
        InterlockedDecrement(&ObjectHeader->PointerCount);
    }

    /* Decrement the handle count and detach */
    ObpDecrementHandleCount(&ObjectHeader->Body,
                            PsGetCurrentProcess(),
                            NewEntry.GrantedAccess);
    return STATUS_INSUFFICIENT_RESOURCES;
}

/*++
* @name ObpCreateHandle
*
*     The ObpCreateHandle routine <FILLMEIN>
*
* @param OpenReason
*        <FILLMEIN>.
*
* @param Object
*        <FILLMEIN>.
*
* @param Type
*        <FILLMEIN>.
*
* @param AccessState
*        <FILLMEIN>.
*
* @param AdditionalReferences
*        <FILLMEIN>.
*
* @param HandleAttributes
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @param ReturnedObject
*        <FILLMEIN>.
*
* @param ReturnedHandle
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks Gloomy says OpenReason is "enables Security" if == 1.
*          since this function *has* to call ObpIncrementHandleCount,
*          which needs to somehow know the OpenReason, and since
*          ObOpenHandle == 1, I'm guessing this is actually the
*          OpenReason. Also makes sense since this function is shared
*          by Duplication, Creation and Opening..
*
*--*/
NTSTATUS
NTAPI
ObpCreateHandle(IN OB_OPEN_REASON OpenReason,
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
    ULONG i;
    PAGED_CODE();

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Creating handle for: %p. Reason: %lx. HC LC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Check if the types match */
    if ((Type) && (ObjectType != Type)) return STATUS_OBJECT_TYPE_MISMATCH;

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

    /* Handle extra references */
    if (AdditionalReferences)
    {
        /* Make a copy in case we fail later below */
        i = AdditionalReferences;
        while (i--)
        {
            /* Increment the count */
            InterlockedIncrement(&ObjectHeader->PointerCount);
        }
    }

    /*
     * Create the actual handle. We'll need to do this *after* calling
     * ObpIncrementHandleCount to make sure that Object Security is valid
     * (specified in Gl00my documentation on Ob)
     */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Handle Properties: [%p-%lx-%lx]\n",
            __FUNCTION__,
            NewEntry.Object, NewEntry.ObAttributes & 3, NewEntry.GrantedAccess);
    Handle = ExCreateHandle(HandleTable, &NewEntry);

     /* Detach if needed */
    if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);

    /* Make sure we got a handle */
    if (Handle)
    {
        /* Check if this was a kernel handle */
        if (HandleAttributes & OBJ_KERNEL_HANDLE)
        {
            /* Set the kernel handle bit */
            Handle = ObMarkHandleAsKernelHandle(Handle);
        }

        /* Return handle and object */
        *ReturnedHandle = Handle;
        if (ReturnedObject) *ReturnedObject = Object;
        OBTRACE(OB_HANDLE_DEBUG,
                "%s - Returning Handle: %lx HC LC %lx %lx\n",
                __FUNCTION__,
                Handle,
                ObjectHeader->HandleCount,
                ObjectHeader->PointerCount);
        return STATUS_SUCCESS;
    }

    /* Handle extra references */
    while (AdditionalReferences--)
    {
        /* Increment the count */
        InterlockedDecrement(&ObjectHeader->PointerCount);
    }

    /* Decrement the handle count and detach */
    ObpDecrementHandleCount(&ObjectHeader->Body,
                            PsGetCurrentProcess(),
                            NewEntry.GrantedAccess);
    return STATUS_INSUFFICIENT_RESOURCES;
}

/*++
* @name ObpCloseHandle
*
*     The ObpCloseHandle routine <FILLMEIN>
*
* @param Handle
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObpCloseHandle(IN HANDLE Handle,
               IN KPROCESSOR_MODE AccessMode)
{
    PVOID HandleTable;
    BOOLEAN AttachedToProcess = FALSE;
    KAPC_STATE ApcState;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    NTSTATUS Status;
    PAGED_CODE();
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closing handle: %lx\n", __FUNCTION__, Handle);

    /* Check if we're dealing with a kernel handle */
    if (ObIsKernelHandle(Handle, AccessMode))
    {
        /* Use the kernel table and convert the handle */
        HandleTable = ObpKernelHandleTable;
        Handle = ObKernelHandleToHandle(Handle);

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
        /* Use the process's handle table */
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }

    /* Enter a critical region to protect handle access */
    KeEnterCriticalRegion();

    /* Get the handle entry */
    HandleTableEntry = ExMapHandleToPointer(HandleTable, Handle);
    if (HandleTableEntry)
    {
        /* Now close the entry */
        Status = ObpCloseHandleTableEntry(HandleTable,
                                          HandleTableEntry,
                                          Handle,
                                          AccessMode,
                                          FALSE);

        /* We can quit the critical region now */
        KeLeaveCriticalRegion();

        /* Detach and return success */
        if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);
        return STATUS_SUCCESS;
    }
    else
    {
        /* We failed, quit the critical region */
        KeLeaveCriticalRegion();

        /* Detach */
        if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);

        /* Check if this was a user-mode caller with a valid exception port */
        if ((AccessMode != KernelMode) &&
            (PsGetCurrentProcess()->ExceptionPort))
        {
            /* Raise an exception */
            Status = KeRaiseUserException(STATUS_INVALID_HANDLE);
        }
        else
        {
            /* Just return the status */
            Status = STATUS_INVALID_HANDLE;
        }
    }

    /* Return status */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closed handle: %lx S: %lx\n",
            __FUNCTION__, Handle, Status);
    return Status;
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
        /* Check if inheriting is not supported for this object */
        if (ObjectHeader->Type->TypeInfo.InvalidAttributes & OBJ_INHERIT)
        {
            /* Fail without changing anything */
            return FALSE;
        }

        /* Set the flag */
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

/*++
* @name ObpCloseHandleCallback
*
*     The ObpCloseHandleCallback routine <FILLMEIN>
*
* @param HandleTable
*        <FILLMEIN>.
*
* @param Object
*        <FILLMEIN>.
*
* @param GrantedAccess
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
VOID
NTAPI
ObpCloseHandleCallback(IN PHANDLE_TABLE_ENTRY HandleTableEntry,
                       IN HANDLE Handle,
                       IN PVOID Context)
{
    POBP_CLOSE_HANDLE_CONTEXT CloseContext = (POBP_CLOSE_HANDLE_CONTEXT)Context;

    /* Simply decrement the handle count */
    ObpCloseHandleTableEntry(CloseContext->HandleTable,
                             HandleTableEntry,
                             Handle,
                             CloseContext->AccessMode,
                             TRUE);
}

/*++
* @name ObpDuplicateHandleCallback
*
*     The ObpDuplicateHandleCallback routine <FILLMEIN>
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
    if (Ret)
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

VOID
NTAPI
ObClearProcessHandleTable(IN PEPROCESS Process)
{
    /* FIXME */
}

/*++
* @name ObpCreateHandleTable
*
*     The ObpCreateHandleTable routine <FILLMEIN>
*
* @param Parent
*        <FILLMEIN>.
*
* @param Process
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
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

/*++
* @name ObKillProcess
*
*     The ObKillProcess routine <FILLMEIN>
*
* @param Process
*        <FILLMEIN>.
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
NTAPI
ObKillProcess(IN PEPROCESS Process)
{
    PHANDLE_TABLE HandleTable = Process->ObjectTable;
    OBP_CLOSE_HANDLE_CONTEXT Context;
    PAGED_CODE();

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Fill out the context */
    Context.AccessMode = KernelMode;
    Context.HandleTable = HandleTable;

    /* Sweep the handle table to close all handles */
    ExSweepHandleTable(HandleTable,
                       ObpCloseHandleCallback,
                       &Context);

    /* Destroy the table and leave the critical region */
    ExDestroyHandleTable(HandleTable);
    KeLeaveCriticalRegion();

    /* Clear the object table */
    Process->ObjectTable = NULL;
}

NTSTATUS
NTAPI
ObDuplicateObject(IN PEPROCESS SourceProcess,
                  IN HANDLE SourceHandle,
                  IN PEPROCESS TargetProcess OPTIONAL,
                  IN PHANDLE TargetHandle OPTIONAL,
                  IN ACCESS_MASK DesiredAccess,
                  IN ULONG HandleAttributes,
                  IN ULONG Options,
                  IN KPROCESSOR_MODE PreviousMode)
{
    HANDLE_TABLE_ENTRY NewHandleEntry;
    BOOLEAN AttachedToProcess = FALSE;
    PVOID SourceObject;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    HANDLE NewHandle;
    KAPC_STATE ApcState;
    NTSTATUS Status = STATUS_SUCCESS;
    ACCESS_MASK TargetAccess, SourceAccess;
    ACCESS_STATE AccessState;
    PACCESS_STATE PassedAccessState = NULL;
    AUX_DATA AuxData;
    PHANDLE_TABLE HandleTable = NULL;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PAGED_CODE();
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicating handle: %lx for %p into %p\n",
            __FUNCTION__,
            SourceHandle,
            SourceProcess,
            TargetProcess);

    /* Check if we're not in the source process */
    if (SourceProcess != PsGetCurrentProcess())
    {
        /* Attach to it */
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);
        AttachedToProcess = TRUE;
    }

    /* Now reference the source handle */
    Status = ObReferenceObjectByHandle(SourceHandle,
                                       0,
                                       NULL,
                                       PreviousMode,
                                       (PVOID*)&SourceObject,
                                       &HandleInformation);

    /* Check if we were attached */
    if (AttachedToProcess)
    {
        /* We can safely detach now */
        KeUnstackDetachProcess(&ApcState);
        AttachedToProcess = FALSE;
    }

    /* Fail if we couldn't reference it */
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the source access */
    SourceAccess = HandleInformation.GrantedAccess;

    /* Check if we're not in the target process */
    if (TargetProcess != PsGetCurrentProcess())
    {
        /* Attach to it */
        KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);
        AttachedToProcess = TRUE;
    }

    /* Check if we're duplicating the attributes */
    if (Options & DUPLICATE_SAME_ATTRIBUTES)
    {
        /* Duplicate them */
        HandleAttributes = HandleInformation.HandleAttributes;
    }

    /* Check if we're duplicating the access */
    if (Options & DUPLICATE_SAME_ACCESS) DesiredAccess = SourceAccess;

    /* Get object data */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(SourceObject);
    ObjectType = ObjectHeader->Type;

    /* Fill out the entry */
    NewHandleEntry.Object = ObjectHeader;
    NewHandleEntry.ObAttributes |= HandleAttributes &
                                   (EX_HANDLE_ENTRY_PROTECTFROMCLOSE |
                                    EX_HANDLE_ENTRY_INHERITABLE |
                                    EX_HANDLE_ENTRY_AUDITONCLOSE);

    /* Check if we're using a generic mask */
    if (DesiredAccess & GENERIC_ACCESS)
    {
        /* Map it */
        RtlMapGenericMask(&DesiredAccess, &ObjectType->TypeInfo.GenericMapping);
    }

    /* Set the target access */
    TargetAccess = DesiredAccess;
    NewHandleEntry.GrantedAccess = TargetAccess;

    /* Check if we're asking for new access */
    if (TargetAccess & ~SourceAccess)
    {
        /* We are. We need the security procedure to validate this */
        if (ObjectType->TypeInfo.SecurityProcedure == SeDefaultObjectMethod)
        {
            /* Use our built-in access state */
            PassedAccessState = &AccessState;
            Status = SeCreateAccessState(&AccessState,
                                         &AuxData,
                                         TargetAccess,
                                         &ObjectType->TypeInfo.GenericMapping);
        }
        else
        {
            /* Otherwise we can't allow this privilege elevation */
            Status = STATUS_ACCESS_DENIED;
        }
    }
    else
    {
        /* We don't need an access state */
        Status = STATUS_SUCCESS;
    }

    /* Make sure the access state was created OK */
    if (NT_SUCCESS(Status))
    {
        /* Add a new handle */
        Status = ObpIncrementHandleCount(SourceObject,
                                         PassedAccessState,
                                         PreviousMode,
                                         HandleAttributes,
                                         PsGetCurrentProcess(),
                                         ObDuplicateHandle);

        /* Set the handle table, now that we know this handle was added */
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }

    /* Check if we were attached */
    if (AttachedToProcess)
    {
        /* We can safely detach now */
        KeUnstackDetachProcess(&ApcState);
        AttachedToProcess = FALSE;
    }

    /* Check if we have to close the source handle */
    if (Options & DUPLICATE_CLOSE_SOURCE)
    {
        /* Attach and close */
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);
        NtClose(SourceHandle);
        KeUnstackDetachProcess(&ApcState);
    }

    /* Check if we had an access state */
    if (PassedAccessState) SeDeleteAccessState(PassedAccessState);

    /* Now check if incrementing actually failed */
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the source object */
        ObDereferenceObject(SourceObject);
        return Status;
    }

    /* Now create the handle */
    NewHandle = ExCreateHandle(HandleTable, &NewHandleEntry);
    if (!NewHandle)
    {
        /* Undo the increment */
        ObpDecrementHandleCount(SourceObject,
                                TargetProcess,
                                TargetAccess);

        /* Deference the object and set failure status */
        ObDereferenceObject(SourceObject);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return the handle */
    if (TargetHandle) *TargetHandle = NewHandle;

    /* Return status */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicated handle: %lx for %p into %p. Source: %p HC PC %lx %lx\n",
            __FUNCTION__,
            NewHandle,
            SourceProcess,
            TargetProcess,
            SourceObject,
            ObjectHeader->PointerCount,
            ObjectHeader->HandleCount);
    return Status;
}

/* PUBLIC FUNCTIONS *********************************************************/

/*++
* @name ObOpenObjectByName
* @implemented NT4
*
*     The ObOpenObjectByName routine <FILLMEIN>
*
* @param ObjectAttributes
*        <FILLMEIN>.
*
* @param ObjectType
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @param PassedAccessState
*        <FILLMEIN>.
*
* @param DesiredAccess
*        <FILLMEIN>.
*
* @param ParseContext
*        <FILLMEIN>.
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

    /* Check if we didn't get any Object Attributes */
    if (!ObjectAttributes)
    {
        /* Fail with special status code */
        *Handle = NULL;
        return STATUS_INVALID_PARAMETER;
    }

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

        /* Check if we still have create info */
        if (ObjectHeader->ObjectCreateInfo)
        {
            /* Free it */
            //ObpFreeAndReleaseCapturedAttributes(&ObjectCreateInfo);
            //ObjectHeader->ObjectCreateInfo = NULL;
        }
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
    if (!NT_SUCCESS(Status)) ObDereferenceObject(Object);

Cleanup:
    /* Delete the access state */
    if (PassedAccessState == &AccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

    /* Release the object attributes and return status */
Quickie:
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);
    if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - returning Object %p with PC S: %lx %lx\n",
            __FUNCTION__,
            Object,
            Object ? OBJECT_TO_OBJECT_HEADER(Object)->PointerCount : -1,
            Status);
    return Status;
}

/*++
* @name ObOpenObjectByPointer
* @implemented NT4
*
*     The ObOpenObjectByPointer routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>.
*
* @param HandleAttributes
*        <FILLMEIN>.
*
* @param PassedAccessState
*        <FILLMEIN>.
*
* @param DesiredAccess
*        <FILLMEIN>.
*
* @param ObjectType
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
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
ObOpenObjectByPointer(IN PVOID Object,
                      IN ULONG HandleAttributes,
                      IN PACCESS_STATE PassedAccessState,
                      IN ACCESS_MASK DesiredAccess,
                      IN POBJECT_TYPE ObjectType,
                      IN KPROCESSOR_MODE AccessMode,
                      OUT PHANDLE Handle)
{
    POBJECT_HEADER Header;
    NTSTATUS Status;
    ACCESS_STATE AccessState;
    AUX_DATA AuxData;
    PAGED_CODE();

    /* Get the Header Info */
    Header = OBJECT_TO_OBJECT_HEADER(Object);

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
                                     &Header->Type->TypeInfo.GenericMapping);
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
    if (!NT_SUCCESS(Status)) ObDereferenceObject(Object);

    /* Delete the access state */
    if (PassedAccessState == &AccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

    /* Return */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - returning Object with PC S: %lx %lx\n",
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
* @name ObInsertObject
* @implemented NT4
*
*     The ObInsertObject routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>.
*
* @param PassedAccessState
*        <FILLMEIN>.
*
* @param DesiredAccess
*        <FILLMEIN>.
*
* @param AdditionalReferences
*        <FILLMEIN>.
*
* @param ReferencedObject
*        <FILLMEIN>.
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
ObInsertObject(IN PVOID Object,
               IN PACCESS_STATE PassedAccessState OPTIONAL,
               IN ACCESS_MASK DesiredAccess,
               IN ULONG AdditionalReferences,
               OUT PVOID *ReferencedObject OPTIONAL,
               OUT PHANDLE Handle)
{
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    POBJECT_HEADER Header;
    POBJECT_TYPE ObjectType;
    PVOID FoundObject = NULL;
    POBJECT_HEADER FoundHeader = NULL;
    NTSTATUS Status = STATUS_SUCCESS, RealStatus;
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

    /* Check if this is an named object */
    if ((ObjectNameInfo) && (ObjectNameInfo->Name.Buffer)) IsNamed = TRUE;

    /* Check if the object is unnamed and also doesn't have security */
    if ((!ObjectType->TypeInfo.SecurityRequired) && !(IsNamed))
    {
        /* ReactOS HACK */
        if (Handle)
        {
            /* Assume failure */
            *Handle = NULL;

            /* Create the handle */
            Status = ObpCreateUnnamedHandle(Object,
                                            DesiredAccess,
                                            AdditionalReferences + 1,
                                            ObjectCreateInfo->Attributes,
                                            ExGetPreviousMode(),
                                            ReferencedObject,
                                            Handle);
        }

        /* Free the create information */
        ObpFreeAndReleaseCapturedAttributes(ObjectCreateInfo);
        Header->ObjectCreateInfo = NULL;

        /* Remove the extra keep-alive reference */
        if (Handle) ObDereferenceObject(Object);

        /* Return */
        OBTRACE(OB_HANDLE_DEBUG,
                "%s - returning Object with PC S: %lx %lx\n",
                __FUNCTION__,
                OBJECT_TO_OBJECT_HEADER(Object)->PointerCount,
                Status);
        return Status;
    }

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

    /* Save the security descriptor */
    PassedAccessState->SecurityDescriptor =
        ObjectCreateInfo->SecurityDescriptor;

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
        /* Check if we found an object that doesn't match the one requested */
        if ((NT_SUCCESS(Status)) && (FoundObject) && (Object != FoundObject))
        {
            /* This means we're opening an object, not creating a new one */
            FoundHeader = OBJECT_TO_OBJECT_HEADER(FoundObject);
            OpenReason = ObOpenHandle;

            /* Make sure the caller said it's OK to do this */
            if (ObjectCreateInfo->Attributes & OBJ_OPENIF)
            {
                /* He did, but did he want this type? */
                if (ObjectType != FoundHeader->Type)
                {
                    /* Wrong type, so fail */
                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                }
                else
                {
                    /* Right type, so warn */
                    Status = STATUS_OBJECT_NAME_EXISTS;
                }
            }
            else
            {
                /* Caller wanted to create a new object, fail */
                Status = STATUS_OBJECT_NAME_COLLISION;
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
    }

    /* Now check if this object is being created */
    if (FoundObject == Object)
    {
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
            KEBUGCHECK(0);
            ObDereferenceObject(Object);
            if (PassedAccessState == &AccessState)
            {
                /* We used a local one; delete it */
                SeDeleteAccessState(PassedAccessState);
            }

            /* Return failure code */
            return Status;
        }
    }

    /* Save the actual status until here */
    RealStatus = Status;

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
                                 FoundObject,
                                 NULL,
                                 PassedAccessState,
                                 AdditionalReferences + 1,
                                 ObjectCreateInfo->Attributes,
                                 ExGetPreviousMode(),
                                 ReferencedObject,
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
    if (Handle) ObDereferenceObject(Object);

    /* Check our final status */
    if (!NT_SUCCESS(Status))
    {
        /* Return the status of the failure */
        *Handle = NULL;
        RealStatus = Status;
    }

    /* Check if we created our own access state */
    if (PassedAccessState == &AccessState)
    {
        /* We used a local one; delete it */
        SeDeleteAccessState(PassedAccessState);
    }

    /* Return status code */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - returning Object with PC S/RS: %lx %lx %lx\n",
            __FUNCTION__,
            OBJECT_TO_OBJECT_HEADER(Object)->PointerCount,
            RealStatus, Status);
    return RealStatus;
}

/*++
* @name ObCloseHandle
* @implemented NT5.1
*
*     The ObCloseHandle routine <FILLMEIN>
*
* @param Handle
*        <FILLMEIN>.
*
* @param AccessMode
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObCloseHandle(IN HANDLE Handle,
              IN KPROCESSOR_MODE AccessMode)
{
    //
    // Call the internal API
    //
    return ObpCloseHandle(Handle, AccessMode);
}

/*++
* @name NtClose
* @implemented NT4
*
*     The NtClose routine <FILLMEIN>
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
NtClose(IN HANDLE Handle)
{
    //
    // Call the internal API
    //
    return ObpCloseHandle(Handle, ExGetPreviousMode());
}

NTSTATUS
NTAPI
NtDuplicateObject(IN HANDLE SourceProcessHandle,
                  IN HANDLE SourceHandle,
                  IN HANDLE TargetProcessHandle OPTIONAL,
                  OUT PHANDLE TargetHandle OPTIONAL,
                  IN ACCESS_MASK DesiredAccess,
                  IN ULONG HandleAttributes,
                  IN ULONG Options)
{
    PEPROCESS SourceProcess, TargetProcess, Target;
    HANDLE hTarget;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicating handle: %lx for %lx into %lx.\n",
            __FUNCTION__,
            SourceHandle,
            SourceProcessHandle,
            TargetProcessHandle);

    if((TargetHandle) && (PreviousMode != KernelMode))
    {
        /* Enter SEH */
        _SEH_TRY
        {
            /* Probe the handle */
            ProbeForWriteHandle(TargetHandle);
        }
        _SEH_HANDLE
        {
            /* Get the exception status */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail if the pointer was invalid */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Now reference the input handle */
    Status = ObReferenceObjectByHandle(SourceProcessHandle,
                                       PROCESS_DUP_HANDLE,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&SourceProcess,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Check if got a target handle */
    if (TargetProcessHandle)
    {
        /* Now reference the output handle */
        Status = ObReferenceObjectByHandle(TargetProcessHandle,
                                           PROCESS_DUP_HANDLE,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&TargetProcess,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            /* Use this target process */
            Target = TargetProcess;
        }
        else
        {
            /* No target process */
            Target = NULL;
        }
    }
    else
    {
        /* No target process */
        Status = STATUS_SUCCESS;
        Target = NULL;
    }

    /* Call the internal routine */
    Status = ObDuplicateObject(SourceProcess,
                               SourceHandle,
                               Target,
                               &hTarget,
                               DesiredAccess,
                               HandleAttributes,
                               Options,
                               PreviousMode);

    /* Check if the caller wanted the return handle */
    if (TargetHandle)
    {
        /* Protect the write to user mode */
        _SEH_TRY
        {
            /* Write the new handle */
            *TargetHandle = hTarget;
        }
        _SEH_HANDLE
        {
            /* Otherwise, get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Dereference the processes */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicated handle: %lx into %lx S %lx\n",
            __FUNCTION__,
            hTarget,
            TargetProcessHandle,
            Status);
    ObDereferenceObject(TargetProcess);
    ObDereferenceObject(SourceProcess);
    return Status;
}

#undef ObIsKernelHandle
BOOLEAN
NTAPI
ObIsKernelHandle(IN HANDLE Handle)
{
    /* We know we're kernel mode, so just check for the kernel handle flag */
    return (BOOLEAN)((ULONG_PTR)Handle & KERNEL_HANDLE_FLAG);
}

/* EOF */
