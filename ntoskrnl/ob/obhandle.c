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
#include <debug.h>

PHANDLE_TABLE ObpKernelHandleTable = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

PHANDLE_TABLE
NTAPI
ObReferenceProcessHandleTable(IN PEPROCESS Process)
{
    PHANDLE_TABLE HandleTable = NULL;

    /* Lock the process */
    if (ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Get the handle table */
        HandleTable = Process->ObjectTable;
        if (!HandleTable)
        {
            /* No table, release the lock */
            ExReleaseRundownProtection(&Process->RundownProtect);
        }
    }

    /* Return the handle table */
    return HandleTable;
}

VOID
NTAPI
ObDereferenceProcessHandleTable(IN PEPROCESS Process)
{
    /* Release the process lock */
    ExReleaseRundownProtection(&Process->RundownProtect);
}

ULONG
NTAPI
ObGetProcessHandleCount(IN PEPROCESS Process)
{
    ULONG HandleCount;
    PHANDLE_TABLE HandleTable;

    ASSERT(Process);

    /* Ensure the handle table doesn't go away while we use it */
    HandleTable = ObReferenceProcessHandleTable(Process);

    if (HandleTable != NULL)
    {
        /* Count the number of handles the process has */
        HandleCount = HandleTable->HandleCount;

        /* Let the handle table go */
        ObDereferenceProcessHandleTable(Process);
    }
    else
    {
        /* No handle table, no handles */
        HandleCount = 0;
    }

    return HandleCount;
}

NTSTATUS
NTAPI
ObpReferenceProcessObjectByHandle(IN HANDLE Handle,
                                  IN PEPROCESS Process,
                                  IN PHANDLE_TABLE HandleTable,
                                  IN KPROCESSOR_MODE AccessMode,
                                  OUT PVOID *Object,
                                  OUT POBJECT_HANDLE_INFORMATION HandleInformation,
                                  OUT PACCESS_MASK AuditMask)
{
    PHANDLE_TABLE_ENTRY HandleEntry;
    POBJECT_HEADER ObjectHeader;
    ACCESS_MASK GrantedAccess;
    ULONG Attributes;
    PETHREAD Thread = PsGetCurrentThread();
    NTSTATUS Status;

    /* Assume failure */
    *Object = NULL;

    /* Check if this is a special handle */
    if (HandleToLong(Handle) < 0)
    {
        /* Check if the caller wants the current process */
        if (Handle == NtCurrentProcess())
        {
            /* Return handle info */
            HandleInformation->HandleAttributes = 0;
            HandleInformation->GrantedAccess = Process->GrantedAccess;

            /* No audit mask */
            *AuditMask = 0;

            /* Reference ourselves */
            ObjectHeader = OBJECT_TO_OBJECT_HEADER(Process);
            InterlockedIncrementSizeT(&ObjectHeader->PointerCount);

            /* Return the pointer */
            *Object = Process;
            ASSERT(*Object != NULL);
            return STATUS_SUCCESS;
        }

        /* Check if the caller wants the current thread */
        if (Handle == NtCurrentThread())
        {
            /* Return handle information */
            HandleInformation->HandleAttributes = 0;
            HandleInformation->GrantedAccess = Thread->GrantedAccess;

            /* Reference ourselves */
            ObjectHeader = OBJECT_TO_OBJECT_HEADER(Thread);
            InterlockedIncrementSizeT(&ObjectHeader->PointerCount);

            /* No audit mask */
            *AuditMask = 0;

            /* Return the pointer */
            *Object = Thread;
            ASSERT(*Object != NULL);
            return STATUS_SUCCESS;
        }

        /* This is a kernel handle... do we have access? */
        if (AccessMode == KernelMode)
        {
            /* Use the kernel handle table and get the actual handle value */
            Handle = ObKernelHandleToHandle(Handle);
            HandleTable = ObpKernelHandleTable;
        }
        else
        {
            /* This is an illegal attempt to access a kernel handle */
            return STATUS_INVALID_HANDLE;
        }
    }

    /* Enter a critical region while we touch the handle table */
    ASSERT(HandleTable != NULL);
    KeEnterCriticalRegion();

    /* Get the handle entry */
    HandleEntry = ExMapHandleToPointer(HandleTable, Handle);
    if (HandleEntry)
    {
        /* Get the object header and validate the type*/
        ObjectHeader = ObpGetHandleObject(HandleEntry);

        /* Get the granted access and validate it */
        GrantedAccess = HandleEntry->GrantedAccess;

        /* Mask out the internal attributes */
        Attributes = HandleEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES;

        /* Fill out the information */
        HandleInformation->HandleAttributes = Attributes;
        HandleInformation->GrantedAccess = GrantedAccess;

        /* No audit mask (FIXME!) */
        *AuditMask = 0;

        /* Return the pointer */
        *Object = &ObjectHeader->Body;

        /* Add a reference */
        InterlockedIncrementSizeT(&ObjectHeader->PointerCount);

        /* Unlock the handle */
        ExUnlockHandleTableEntry(HandleTable, HandleEntry);
        KeLeaveCriticalRegion();

        /* Return success */
        ASSERT(*Object != NULL);
        return STATUS_SUCCESS;
    }
    else
    {
        /* Invalid handle */
        Status = STATUS_INVALID_HANDLE;
    }

    /* Return failure status */
    KeLeaveCriticalRegion();
    return Status;
}

BOOLEAN
NTAPI
ObpEnumFindHandleProcedure(IN PHANDLE_TABLE_ENTRY HandleEntry,
                           IN HANDLE Handle,
                           IN PVOID Context)
{
    POBJECT_HEADER ObjectHeader;
    ACCESS_MASK GrantedAccess;
    ULONG HandleAttributes;
    POBP_FIND_HANDLE_DATA FindData = Context;

    /* Get the object header */
    ObjectHeader = ObpGetHandleObject(HandleEntry);

    /* Make sure it's valid and matching */
    if ((FindData->ObjectHeader) && (FindData->ObjectHeader != ObjectHeader))
    {
        /* No match, fail */
        return FALSE;
    }

    /* Now attempt to match the object type */
    if ((FindData->ObjectType) && (FindData->ObjectType != ObjectHeader->Type))
    {
        /* No match, fail */
        return FALSE;
    }

    /* Check if we have extra information */
    if (FindData->HandleInformation)
    {
        /* Get the granted access and attributes */
        GrantedAccess = HandleEntry->GrantedAccess;
        HandleAttributes = HandleEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES;

        /* Attempt to match them */
        if ((FindData->HandleInformation->HandleAttributes != HandleAttributes) ||
            (FindData->HandleInformation->GrantedAccess != GrantedAccess))
        {
            /* No match, fail */
            return FALSE;
        }
    }

    /* We have a match */
    return TRUE;
}

POBJECT_HANDLE_COUNT_ENTRY
NTAPI
ObpInsertHandleCount(IN POBJECT_HEADER ObjectHeader)
{
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_ENTRY FreeEntry;
    POBJECT_HANDLE_COUNT_DATABASE HandleDatabase, OldHandleDatabase;
    ULONG i;
    ULONG Size, OldSize;
    OBJECT_HANDLE_COUNT_DATABASE SingleDatabase;
    PAGED_CODE();

    /* Get the handle info */
    HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO(ObjectHeader);
    if (!HandleInfo) return NULL;

    /* Check if we only have one entry */
    if (ObjectHeader->Flags & OB_FLAG_SINGLE_PROCESS)
    {
        /* Fill out the single entry */
        SingleDatabase.CountEntries = 1;
        SingleDatabase.HandleCountEntries[0] = HandleInfo->SingleEntry;

        /* Use this as the old size */
        OldHandleDatabase = &SingleDatabase;
        OldSize = sizeof(SingleDatabase);

        /* Now we'll have two entries, and an entire DB */
        i = 2;
        Size = sizeof(OBJECT_HANDLE_COUNT_DATABASE) +
               ((i - 1) * sizeof(OBJECT_HANDLE_COUNT_ENTRY));
    }
    else
    {
        /* We already have a DB, get the information from it */
        OldHandleDatabase = HandleInfo->HandleCountDatabase;
        i = OldHandleDatabase->CountEntries;
        OldSize = sizeof(OBJECT_HANDLE_COUNT_DATABASE) +
                  ((i - 1) * sizeof(OBJECT_HANDLE_COUNT_ENTRY));

        /* Add 4 more entries */
        i += 4;
        Size = OldSize + (4 * sizeof(OBJECT_HANDLE_COUNT_ENTRY));
    }

    /* Allocate the DB */
    HandleDatabase = ExAllocatePoolWithTag(PagedPool, Size, TAG_OB_HANDLE);
    if (!HandleDatabase) return NULL;

    /* Copy the old database */
    RtlCopyMemory(HandleDatabase, OldHandleDatabase, OldSize);

    /* Check if we he had a single entry before */
    if (ObjectHeader->Flags & OB_FLAG_SINGLE_PROCESS)
    {
        /* Now we have more */
        ObjectHeader->Flags &= ~OB_FLAG_SINGLE_PROCESS;
    }
    else
    {
        /* Otherwise we had a DB, free it */
        ExFreePoolWithTag(OldHandleDatabase, TAG_OB_HANDLE);
    }

    /* Find the end of the copy and zero out the new data */
    FreeEntry = (PVOID)((ULONG_PTR)HandleDatabase + OldSize);
    RtlZeroMemory(FreeEntry, Size - OldSize);

    /* Set the new information and return the free entry */
    HandleDatabase->CountEntries = i;
    HandleInfo->HandleCountDatabase = HandleDatabase;
    return FreeEntry;
}

NTSTATUS
NTAPI
ObpIncrementHandleDataBase(IN POBJECT_HEADER ObjectHeader,
                           IN PEPROCESS Process,
                           IN OUT PULONG NewProcessHandleCount)
{
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_ENTRY HandleEntry, FreeEntry = NULL;
    POBJECT_HANDLE_COUNT_DATABASE HandleDatabase;
    ULONG i;
    PAGED_CODE();

    /* Get the handle info and check if we only have one entry */
    HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO(ObjectHeader);
    if (ObjectHeader->Flags & OB_FLAG_SINGLE_PROCESS)
    {
        /* Check if the entry is free */
        if (!HandleInfo->SingleEntry.HandleCount)
        {
            /* Add ours */
            HandleInfo->SingleEntry.HandleCount = 1;
            HandleInfo->SingleEntry.Process = Process;

            /* Return success and 1 handle */
            *NewProcessHandleCount = 1;
            return STATUS_SUCCESS;
        }
        else if (HandleInfo->SingleEntry.Process == Process)
        {
            /* Busy entry, but same process */
            *NewProcessHandleCount = ++HandleInfo->SingleEntry.HandleCount;
            return STATUS_SUCCESS;
        }
        else
        {
            /* Insert a new entry */
            FreeEntry = ObpInsertHandleCount(ObjectHeader);
            if (!FreeEntry) return STATUS_INSUFFICIENT_RESOURCES;
            ASSERT(!FreeEntry->Process);
            ASSERT(!FreeEntry->HandleCount);

            /* Fill it out */
            FreeEntry->Process = Process;
            FreeEntry->HandleCount = 1;

            /* Return success and 1 handle */
            *NewProcessHandleCount = 1;
            return STATUS_SUCCESS;
        }
    }

    /* We have a database instead */
    HandleDatabase = HandleInfo->HandleCountDatabase;
    if (HandleDatabase)
    {
        /* Get the entries and loop them */
        i = HandleDatabase->CountEntries;
        HandleEntry = &HandleDatabase->HandleCountEntries[0];
        while (i)
        {
            /* Check if this is a match */
            if (HandleEntry->Process == Process)
            {
                /* Found it, get the process handle count */
                *NewProcessHandleCount = ++HandleEntry->HandleCount;
                return STATUS_SUCCESS;
            }
            else if (!HandleEntry->HandleCount)
            {
                /* Found a free entry */
                FreeEntry = HandleEntry;
            }

            /* Keep looping */
            HandleEntry++;
            i--;
        }

        /* Check if we couldn't find a free entry */
        if (!FreeEntry)
        {
            /* Allocate one */
            FreeEntry = ObpInsertHandleCount(ObjectHeader);
            if (!FreeEntry) return STATUS_INSUFFICIENT_RESOURCES;
            ASSERT(!FreeEntry->Process);
            ASSERT(!FreeEntry->HandleCount);
        }

        /* Fill out the entry */
        FreeEntry->Process = Process;
        FreeEntry->HandleCount = 1;
        *NewProcessHandleCount = 1;
    }

    /* Return success if we got here */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObpChargeQuotaForObject(IN POBJECT_HEADER ObjectHeader,
                        IN POBJECT_TYPE ObjectType,
                        OUT PBOOLEAN NewObject)
{
    POBJECT_HEADER_QUOTA_INFO ObjectQuota;
    ULONG PagedPoolCharge, NonPagedPoolCharge;

    /* Get quota information */
    ObjectQuota = OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader);
    *NewObject = FALSE;

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

        /* Charge the quota */
        ObjectHeader->QuotaBlockCharged = (PVOID)1;
        DPRINT("FIXME: Should charge: %lx %lx\n", PagedPoolCharge, NonPagedPoolCharge);
#if 0
            PsChargeSharedPoolQuota(PsGetCurrentProcess(),
                                    PagedPoolCharge,
                                    NonPagedPoolCharge);
#endif

        /* Check if we don't have a quota block */
        if (!ObjectHeader->QuotaBlockCharged) return STATUS_QUOTA_EXCEEDED;

        /* Now set the flag */
        *NewObject = TRUE;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObpValidateAccessMask(IN PACCESS_STATE AccessState)
{
    PISECURITY_DESCRIPTOR SecurityDescriptor;

    /* We're only interested if the object for this access state has an SD */
    SecurityDescriptor = AccessState->SecurityDescriptor;
    if (SecurityDescriptor)
    {
        /* Check if the SD has a system ACL but hasn't been granted access to get/set it */
        if ((SecurityDescriptor->Control & SE_SACL_PRESENT) &&
            !(AccessState->PreviouslyGrantedAccess & ACCESS_SYSTEM_SECURITY))
        {
            /* We're gonna need access */
            AccessState->RemainingDesiredAccess |= ACCESS_SYSTEM_SECURITY;
        }
    }

    /* This can't fail */
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
                        IN ACCESS_MASK GrantedAccess,
                        IN POBJECT_TYPE ObjectType)
{
    POBJECT_HEADER ObjectHeader;
    LONG SystemHandleCount, ProcessHandleCount;
    LONG NewCount;
    KIRQL CalloutIrql;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_ENTRY HandleEntry;
    POBJECT_HANDLE_COUNT_DATABASE HandleDatabase;
    ULONG i;
    PAGED_CODE();

    /* Get the object type and header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Decrementing count for: %p. HC PC %lx %lx\n",
            __FUNCTION__,
            ObjectBody,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Lock the object */
    ObpAcquireObjectLock(ObjectHeader);

    /* Set default counts */
    SystemHandleCount = ObjectHeader->HandleCount;
    ProcessHandleCount = 0;

    /* Decrement the handle count */
    NewCount = InterlockedDecrementSizeT(&ObjectHeader->HandleCount);

    /* Check if we're out of handles and this was an exclusive object */
    if (!(NewCount) && (ObjectHeader->Flags & OB_FLAG_EXCLUSIVE))
    {
        /* Clear the exclusive flag */
        OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader)->ExclusiveProcess = NULL;
    }

    /* Is the object type keeping track of handles? */
    if (ObjectType->TypeInfo.MaintainHandleCount)
    {
        /* Get handle information */
        HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO(ObjectHeader);

        /* Check if there's only a single entry */
        if (ObjectHeader->Flags & OB_FLAG_SINGLE_PROCESS)
        {
            /* It should be us */
            ASSERT(HandleInfo->SingleEntry.Process == Process);
            ASSERT(HandleInfo->SingleEntry.HandleCount > 0);

            /* Get the handle counts */
            ProcessHandleCount = HandleInfo->SingleEntry.HandleCount--;
            HandleEntry = &HandleInfo->SingleEntry;
        }
        else
        {
            /* Otherwise, get the database */
            HandleDatabase = HandleInfo->HandleCountDatabase;
            if (HandleDatabase)
            {
                /* Get the entries and loop them */
                i = HandleDatabase->CountEntries;
                HandleEntry = &HandleDatabase->HandleCountEntries[0];
                while (i)
                {
                    /* Check if this is a match */
                    if ((HandleEntry->HandleCount) &&
                        (HandleEntry->Process == Process))
                    {
                        /* Found it, get the process handle count */
                        ProcessHandleCount = HandleEntry->HandleCount--;
                        break;
                    }

                    /* Keep looping */
                    HandleEntry++;
                    i--;
                }
            }
            else
            {
                /* No database, so no entry */
                HandleEntry = NULL;
            }
        }

        /* Check if this is the last handle */
        if (ProcessHandleCount == 1)
        {
            /* Then clear the entry */
            HandleEntry->Process = NULL;
            HandleEntry->HandleCount = 0;
        }
    }

    /* Release the lock */
    ObpReleaseObjectLock(ObjectHeader);

    /* Check if we have a close procedure */
    if (ObjectType->TypeInfo.CloseProcedure)
    {
        /* Call it */
        ObpCalloutStart(&CalloutIrql);
        ObjectType->TypeInfo.CloseProcedure(Process,
                                            ObjectBody,
                                            GrantedAccess,
                                            ProcessHandleCount,
                                            SystemHandleCount);
        ObpCalloutEnd(CalloutIrql, "Close", ObjectType, ObjectBody);
    }

    /* Check if we should delete the object */
    ObpDeleteNameCheck(ObjectBody);

    /* Decrease the total number of handles for this type */
    InterlockedDecrement((PLONG)&ObjectType->TotalNumberOfHandles);
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Decremented count for: %p. HC PC %lx %lx\n",
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
    KIRQL CalloutIrql;
    PAGED_CODE();

    /* Get the object data */
    ObjectHeader = ObpGetHandleObject(HandleEntry);
    ObjectType = ObjectHeader->Type;
    Body = &ObjectHeader->Body;
    GrantedAccess = HandleEntry->GrantedAccess;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closing handle: %p for %p. HC PC %lx %lx\n",
            __FUNCTION__,
            Handle,
            Body,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Check if the object has an Okay To Close procedure */
    if (ObjectType->TypeInfo.OkayToCloseProcedure)
    {
        /* Call it and check if it's not letting us close it */
        ObpCalloutStart(&CalloutIrql);
        if (!ObjectType->TypeInfo.OkayToCloseProcedure(PsGetCurrentProcess(),
                                                       Body,
                                                       Handle,
                                                       AccessMode))
        {
            /* Fail */
            ObpCalloutEnd(CalloutIrql, "NtClose", ObjectType, Body);
            ExUnlockHandleTableEntry(HandleTable, HandleEntry);
            return STATUS_HANDLE_NOT_CLOSABLE;
        }

        /* Success, validate callout retrn */
        ObpCalloutEnd(CalloutIrql, "NtClose", ObjectType, Body);
    }

    /* The callback allowed us to close it, but does the handle itself? */
    if ((HandleEntry->GrantedAccess & ObpAccessProtectCloseBit) &&
        !(IgnoreHandleProtection))
    {
        /* It doesn't, are we from user mode? */
        if (AccessMode != KernelMode)
        {
            /* We are! Unlock the entry */
            ExUnlockHandleTableEntry(HandleTable, HandleEntry);

            /* Make sure we have a debug port */
            if (PsGetCurrentProcess()->DebugPort)
            {
                /* Raise an exception */
                return KeRaiseUserException(STATUS_HANDLE_NOT_CLOSABLE);
            }
            else
            {
                /* Return the error instead */
                return STATUS_HANDLE_NOT_CLOSABLE;
            }
        }
        else
        {
            /* Otherwise, bugcheck the OS */
            KeBugCheckEx(INVALID_KERNEL_HANDLE, (ULONG_PTR)Handle, 0, 0, 0);
        }
    }

    /* Destroy and unlock the handle entry */
    ExDestroyHandle(HandleTable, Handle, HandleEntry);

    /* Now decrement the handle count */
    ObpDecrementHandleCount(Body,
                            PsGetCurrentProcess(),
                            GrantedAccess,
                            ObjectType);

    /* Dereference the object as well */
    ObDereferenceObject(Body);

    /* Return to caller */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closed handle: %p for %p.\n",
            __FUNCTION__,
            Handle,
            Body);
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
                        IN PACCESS_STATE AccessState OPTIONAL,
                        IN KPROCESSOR_MODE AccessMode,
                        IN ULONG HandleAttributes,
                        IN PEPROCESS Process,
                        IN OB_OPEN_REASON OpenReason)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    ULONG ProcessHandleCount;
    NTSTATUS Status;
    PEPROCESS ExclusiveProcess;
    BOOLEAN Exclusive = FALSE, NewObject;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    KIRQL CalloutIrql;
    KPROCESSOR_MODE ProbeMode;
    ULONG Total;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PAGED_CODE();

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incrementing count for: %p. Reason: %lx. HC PC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Check if caller is forcing user mode */
    if (HandleAttributes & OBJ_FORCE_ACCESS_CHECK)
    {
        /* Force it */
        ProbeMode = UserMode;
    }
    else
    {
        /* Keep original setting */
        ProbeMode = AccessMode;
    }

    /* Lock the object */
    ObpAcquireObjectLock(ObjectHeader);

    /* Charge quota and remove the creator info flag */
    Status = ObpChargeQuotaForObject(ObjectHeader, ObjectType, &NewObject);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if the open is exclusive */
    if (HandleAttributes & OBJ_EXCLUSIVE)
    {
        /* Check if the object allows this, or if the inherit flag was given */
        if ((HandleAttributes & OBJ_INHERIT) ||
            !(ObjectHeader->Flags & OB_FLAG_EXCLUSIVE))
        {
            /* Incorrect attempt */
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        /* Check if we have access to it */
        ExclusiveProcess = OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader);
        if ((!(ExclusiveProcess) && (ObjectHeader->HandleCount)) ||
            ((ExclusiveProcess) && (ExclusiveProcess != PsGetCurrentProcess())))
        {
            /* This isn't the right process */
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* Now you got exclusive access */
        Exclusive = TRUE;
    }
    else if ((ObjectHeader->Flags & OB_FLAG_EXCLUSIVE) &&
             (OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader)))
    {
        /* Caller didn't want exclusive access, but the object is exclusive */
        Status = STATUS_ACCESS_DENIED;
        goto Quickie;
    }

    /* Check for exclusive kernel object */
    NameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
    if ((NameInfo) && (NameInfo->QueryReferences & OB_FLAG_KERNEL_EXCLUSIVE) &&
        (ProbeMode != KernelMode))
    {
        /* Caller is not kernel, but the object is kernel exclusive */
        Status = STATUS_ACCESS_DENIED;
        goto Quickie;
    }

    /*
     * Check if this is an object that went from 0 handles back to existence,
     * but doesn't have an open procedure, only a close procedure. This means
     * that it will never realize that the object is back alive, so we must
     * fail the request.
     */
    if (!(ObjectHeader->HandleCount) &&
        !(NewObject) &&
        (ObjectType->TypeInfo.MaintainHandleCount) &&
        !(ObjectType->TypeInfo.OpenProcedure) &&
        (ObjectType->TypeInfo.CloseProcedure))
    {
        /* Fail */
        Status = STATUS_UNSUCCESSFUL;
        goto Quickie;
    }

    /* Check if we're opening an existing handle */
    if ((OpenReason == ObOpenHandle) ||
        ((OpenReason == ObDuplicateHandle) && (AccessState)))
    {
        /* Validate the caller's access to this object */
        if (!ObCheckObjectAccess(Object,
                                 AccessState,
                                 TRUE,
                                 ProbeMode,
                                 &Status))
        {
            /* Access was denied, so fail */
            goto Quickie;
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

        /* Check if the caller is trying to access system security */
        if (AccessState->RemainingDesiredAccess & ACCESS_SYSTEM_SECURITY)
        {
            /* FIXME: TODO */
            DPRINT1("ACCESS_SYSTEM_SECURITY not validated!\n");
        }
    }

    /* Check if this is an exclusive handle */
    if (Exclusive)
    {
        /* Save the owner process */
        OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader)->ExclusiveProcess = Process;
    }

    /* Increase the handle count */
    InterlockedIncrementSizeT(&ObjectHeader->HandleCount);
    ProcessHandleCount = 0;

    /* Check if we have a handle database */
    if (ObjectType->TypeInfo.MaintainHandleCount)
    {
        /* Increment the handle database */
        Status = ObpIncrementHandleDataBase(ObjectHeader,
                                            Process,
                                            &ProcessHandleCount);
        if (!NT_SUCCESS(Status))
        {
            /* FIXME: This should never happen for now */
            DPRINT1("Unhandled case\n");
            ASSERT(FALSE);
            goto Quickie;
        }
    }

    /* Release the lock */
    ObpReleaseObjectLock(ObjectHeader);

    /* Check if we have an open procedure */
    Status = STATUS_SUCCESS;
    if (ObjectType->TypeInfo.OpenProcedure)
    {
        /* Call it */
        ObpCalloutStart(&CalloutIrql);
        Status = ObjectType->TypeInfo.OpenProcedure(OpenReason,
                                                    Process,
                                                    Object,
                                                    AccessState ?
                                                    AccessState->
                                                    PreviouslyGrantedAccess :
                                                    0,
                                                    ProcessHandleCount);
        ObpCalloutEnd(CalloutIrql, "Open", ObjectType, Object);

        /* Check if the open procedure failed */
        if (!NT_SUCCESS(Status))
        {
            /* FIXME: This should never happen for now */
            DPRINT1("Unhandled case\n");
            ASSERT(FALSE);
            return Status;
        }
    }

    /* Check if this is a create operation */
    if (OpenReason == ObCreateHandle)
    {
        /* Check if we have creator info */
        CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO(ObjectHeader);
        if (CreatorInfo)
        {
            /* We do, acquire the lock */
            ObpEnterObjectTypeMutex(ObjectType);

            /* Insert us on the list */
            InsertTailList(&ObjectType->TypeList, &CreatorInfo->TypeList);

            /* Release the lock */
            ObpLeaveObjectTypeMutex(ObjectType);
        }
    }

    /* Increase total number of handles */
    Total = InterlockedIncrement((PLONG)&ObjectType->TotalNumberOfHandles);
    if (Total > ObjectType->HighWaterNumberOfHandles)
    {
        /* Fixup count */
        ObjectType->HighWaterNumberOfHandles = Total;
    }

    /* Trace call and return */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incremented count for: %p. Reason: %lx HC PC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
    return Status;

Quickie:
    /* Release lock and return */
    ObpReleaseObjectLock(ObjectHeader);
    return Status;
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
    PEPROCESS ExclusiveProcess;
    BOOLEAN Exclusive = FALSE, NewObject;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    KIRQL CalloutIrql;
    ULONG Total;

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incrementing count for: %p. UNNAMED. HC PC %lx %lx\n",
            __FUNCTION__,
            Object,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Lock the object */
    ObpAcquireObjectLock(ObjectHeader);

    /* Charge quota and remove the creator info flag */
    Status = ObpChargeQuotaForObject(ObjectHeader, ObjectType, &NewObject);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if the open is exclusive */
    if (HandleAttributes & OBJ_EXCLUSIVE)
    {
        /* Check if the object allows this, or if the inherit flag was given */
        if ((HandleAttributes & OBJ_INHERIT) ||
            !(ObjectHeader->Flags & OB_FLAG_EXCLUSIVE))
        {
            /* Incorrect attempt */
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        /* Check if we have access to it */
        ExclusiveProcess = OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader);
        if ((!(ExclusiveProcess) && (ObjectHeader->HandleCount)) ||
            ((ExclusiveProcess) && (ExclusiveProcess != PsGetCurrentProcess())))
        {
            /* This isn't the right process */
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* Now you got exclusive access */
        Exclusive = TRUE;
    }
    else if ((ObjectHeader->Flags & OB_FLAG_EXCLUSIVE) &&
             (OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader)))
    {
        /* Caller didn't want exclusive access, but the object is exclusive */
        Status = STATUS_ACCESS_DENIED;
        goto Quickie;
    }

    /*
     * Check if this is an object that went from 0 handles back to existence,
     * but doesn't have an open procedure, only a close procedure. This means
     * that it will never realize that the object is back alive, so we must
     * fail the request.
     */
    if (!(ObjectHeader->HandleCount) &&
        !(NewObject) &&
        (ObjectType->TypeInfo.MaintainHandleCount) &&
        !(ObjectType->TypeInfo.OpenProcedure) &&
        (ObjectType->TypeInfo.CloseProcedure))
    {
        /* Fail */
        Status = STATUS_UNSUCCESSFUL;
        goto Quickie;
    }

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

    /* Check if this is an exclusive handle */
    if (Exclusive)
    {
        /* Save the owner process */
        OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader)->ExclusiveProcess = Process;
    }

    /* Increase the handle count */
    InterlockedIncrementSizeT(&ObjectHeader->HandleCount);
    ProcessHandleCount = 0;

    /* Check if we have a handle database */
    if (ObjectType->TypeInfo.MaintainHandleCount)
    {
        /* Increment the handle database */
        Status = ObpIncrementHandleDataBase(ObjectHeader,
                                            Process,
                                            &ProcessHandleCount);
        if (!NT_SUCCESS(Status))
        {
            /* FIXME: This should never happen for now */
            DPRINT1("Unhandled case\n");
            ASSERT(FALSE);
            goto Quickie;
        }
    }

    /* Release the lock */
    ObpReleaseObjectLock(ObjectHeader);

    /* Check if we have an open procedure */
    Status = STATUS_SUCCESS;
    if (ObjectType->TypeInfo.OpenProcedure)
    {
        /* Call it */
        ObpCalloutStart(&CalloutIrql);
        Status = ObjectType->TypeInfo.OpenProcedure(ObCreateHandle,
                                                    Process,
                                                    Object,
                                                    *DesiredAccess,
                                                    ProcessHandleCount);
        ObpCalloutEnd(CalloutIrql, "Open", ObjectType, Object);

        /* Check if the open procedure failed */
        if (!NT_SUCCESS(Status))
        {
            /* FIXME: This should never happen for now */
            DPRINT1("Unhandled case\n");
            ASSERT(FALSE);
            return Status;
        }
    }

    /* Check if we have creator info */
    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO(ObjectHeader);
    if (CreatorInfo)
    {
        /* We do, acquire the lock */
        ObpEnterObjectTypeMutex(ObjectType);

        /* Insert us on the list */
        InsertTailList(&ObjectType->TypeList, &CreatorInfo->TypeList);

        /* Release the lock */
        ObpLeaveObjectTypeMutex(ObjectType);
    }

    /* Increase total number of handles */
    Total = InterlockedIncrement((PLONG)&ObjectType->TotalNumberOfHandles);
    if (Total > ObjectType->HighWaterNumberOfHandles)
    {
        /* Fixup count */
        ObjectType->HighWaterNumberOfHandles = Total;
    }

    /* Trace call and return */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Incremented count for: %p. UNNAMED HC PC %lx %lx\n",
            __FUNCTION__,
            Object,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);
    return Status;

Quickie:
    /* Release lock and return */
    ObpReleaseObjectLock(ObjectHeader);
    return Status;
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
    BOOLEAN AttachedToProcess = FALSE, KernelHandle = FALSE;
    PVOID HandleTable;
    NTSTATUS Status;
    ACCESS_MASK GrantedAccess;
    POBJECT_TYPE ObjectType;
    PAGED_CODE();

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Creating handle for: %p. UNNAMED. HC PC %lx %lx\n",
            __FUNCTION__,
            Object,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Save the object header */
    NewEntry.Object = ObjectHeader;

    /* Mask out the internal attributes */
    NewEntry.ObAttributes |= HandleAttributes & OBJ_HANDLE_ATTRIBUTES;

    /* Check if this is a kernel handle */
    if (HandleAttributes & OBJ_KERNEL_HANDLE)
    {
        /* Set the handle table */
        HandleTable = ObpKernelHandleTable;
        KernelHandle = TRUE;

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

    /* Remove what's not in the valid access mask */
    GrantedAccess = DesiredAccess & (ObjectType->TypeInfo.ValidAccessMask |
                                     ACCESS_SYSTEM_SECURITY);

    /* Handle extra references */
    if (AdditionalReferences)
    {
        /* Add them to the header */
        InterlockedExchangeAddSizeT(&ObjectHeader->PointerCount,
                                    AdditionalReferences);
    }

    /* Save the access mask */
    NewEntry.GrantedAccess = GrantedAccess;

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

    /* Make sure we got a handle */
    if (Handle)
    {
        /* Check if this was a kernel handle */
        if (KernelHandle) Handle = ObMarkHandleAsKernelHandle(Handle);

        /* Return handle and object */
        *ReturnedHandle = Handle;

        /* Return the new object only if caller wanted it biased */
        if ((AdditionalReferences) && (ReturnedObject))
        {
            /* Return it */
            *ReturnedObject = Object;
        }

        /* Detach if needed */
        if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);

        /* Trace and return */
        OBTRACE(OB_HANDLE_DEBUG,
                "%s - Returning Handle: %p HC PC %lx %lx\n",
                __FUNCTION__,
                Handle,
                ObjectHeader->HandleCount,
                ObjectHeader->PointerCount);
        return STATUS_SUCCESS;
    }

    /* Handle extra references */
    if (AdditionalReferences)
    {
        /* Dereference it as many times as required */
        InterlockedExchangeAddSizeT(&ObjectHeader->PointerCount,
                                    -(LONG)AdditionalReferences);
    }

    /* Decrement the handle count and detach */
    ObpDecrementHandleCount(&ObjectHeader->Body,
                            PsGetCurrentProcess(),
                            GrantedAccess,
                            ObjectType);

    /* Detach and fail */
    if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);
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
* @remarks Cleans up the Lookup Context on return.
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
                IN POBP_LOOKUP_CONTEXT Context,
                IN KPROCESSOR_MODE AccessMode,
                OUT PVOID *ReturnedObject,
                OUT PHANDLE ReturnedHandle)
{
    HANDLE_TABLE_ENTRY NewEntry;
    POBJECT_HEADER ObjectHeader;
    HANDLE Handle;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE, KernelHandle = FALSE;
    POBJECT_TYPE ObjectType;
    PVOID HandleTable;
    NTSTATUS Status;
    ACCESS_MASK DesiredAccess, GrantedAccess;
    PAUX_ACCESS_DATA AuxData;
    PAGED_CODE();

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Creating handle for: %p. Reason: %lx. HC PC %lx %lx\n",
            __FUNCTION__,
            Object,
            OpenReason,
            ObjectHeader->HandleCount,
            ObjectHeader->PointerCount);

    /* Check if the types match */
    if ((Type) && (ObjectType != Type))
    {
        /* They don't, cleanup */
        if (Context) ObpReleaseLookupContext(Context);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Save the object header */
    NewEntry.Object = ObjectHeader;

    /* Check if this is a kernel handle */
    if (HandleAttributes & OBJ_KERNEL_HANDLE)
    {
        /* Set the handle table */
        HandleTable = ObpKernelHandleTable;
        KernelHandle = TRUE;

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
        if (Context) ObpReleaseLookupContext(Context);
        if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);
        return Status;
    }

    /* Check if we are doing audits on close */
    if (AccessState->GenerateOnClose)
    {
        /* Force the attribute on */
        HandleAttributes |= OBJ_AUDIT_OBJECT_CLOSE;
    }

    /* Mask out the internal attributes */
    NewEntry.ObAttributes |= (HandleAttributes & OBJ_HANDLE_ATTRIBUTES);

    /* Get the original desired access */
    DesiredAccess = AccessState->RemainingDesiredAccess |
                    AccessState->PreviouslyGrantedAccess;

    /* Remove what's not in the valid access mask */
    GrantedAccess = DesiredAccess & (ObjectType->TypeInfo.ValidAccessMask |
                                     ACCESS_SYSTEM_SECURITY);

    /* Update the value in the access state */
    AccessState->PreviouslyGrantedAccess = GrantedAccess;

    /* Get the auxiliary data */
    AuxData = AccessState->AuxData;

    /* Handle extra references */
    if (AdditionalReferences)
    {
        /* Add them to the header */
        InterlockedExchangeAddSizeT(&ObjectHeader->PointerCount,
                                    AdditionalReferences);
    }

    /* Now we can release the object */
    if (Context) ObpReleaseLookupContext(Context);

    /* Save the access mask */
    NewEntry.GrantedAccess = GrantedAccess;

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

    /* Make sure we got a handle */
    if (Handle)
    {
        /* Check if this was a kernel handle */
        if (KernelHandle) Handle = ObMarkHandleAsKernelHandle(Handle);

        /* Return it */
        *ReturnedHandle = Handle;

        /* Check if we need to generate on audit */
        if (AccessState->GenerateAudit)
        {
            /* Audit the handle creation */
            //SeAuditHandleCreation(AccessState, Handle);
        }

        /* Check if this was a create */
        if (OpenReason == ObCreateHandle)
        {
            /* Check if we need to audit the privileges */
            if ((AuxData->PrivilegeSet) &&
                (AuxData->PrivilegeSet->PrivilegeCount))
            {
                /* Do the audit */
#if 0
                SePrivilegeObjectAuditAlarm(Handle,
                                            &AccessState->
                                            SubjectSecurityContext,
                                            GrantedAccess,
                                            AuxData->PrivilegeSet,
                                            TRUE,
                                            ExGetPreviousMode());
#endif
            }
        }

        /* Return the new object only if caller wanted it biased */
        if ((AdditionalReferences) && (ReturnedObject))
        {
            /* Return it */
            *ReturnedObject = Object;
        }

        /* Detach if needed */
        if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);

        /* Trace and return */
        OBTRACE(OB_HANDLE_DEBUG,
                "%s - Returning Handle: %p HC PC %lx %lx\n",
                __FUNCTION__,
                Handle,
                ObjectHeader->HandleCount,
                ObjectHeader->PointerCount);
        return STATUS_SUCCESS;
    }

    /* Decrement the handle count and detach */
    ObpDecrementHandleCount(&ObjectHeader->Body,
                            PsGetCurrentProcess(),
                            GrantedAccess,
                            ObjectType);

    /* Handle extra references */
    if (AdditionalReferences)
    {
        /* Check how many extra references were added */
        if (AdditionalReferences > 1)
        {
            /* Dereference it many times */
            InterlockedExchangeAddSizeT(&ObjectHeader->PointerCount,
                                        -(LONG)(AdditionalReferences - 1));
        }

        /* Dereference the object one last time */
        ObDereferenceObject(Object);
    }

    /* Detach if necessary and fail */
    if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);
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
    PEPROCESS Process = PsGetCurrentProcess();
    PAGED_CODE();
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closing handle: %p\n", __FUNCTION__, Handle);

    if (AccessMode == KernelMode && Handle == (HANDLE)-1)
        return STATUS_INVALID_HANDLE;

    /* Check if we're dealing with a kernel handle */
    if (ObpIsKernelHandle(Handle, AccessMode))
    {
        /* Use the kernel table and convert the handle */
        HandleTable = ObpKernelHandleTable;
        Handle = ObKernelHandleToHandle(Handle);

        /* Check if we're not in the system process */
        if (Process != PsInitialSystemProcess)
        {
            /* Attach to the system process */
            KeStackAttachProcess(&PsInitialSystemProcess->Pcb, &ApcState);
            AttachedToProcess = TRUE;
        }
    }
    else
    {
        /* Use the process's handle table */
        HandleTable = Process->ObjectTable;
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
    }
    else
    {
        /* We failed, quit the critical region */
        KeLeaveCriticalRegion();

        /* Detach */
        if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);

        /* Check if we have a valid handle that's not the process or thread */
        if ((Handle) &&
            (Handle != NtCurrentProcess()) &&
            (Handle != NtCurrentThread()))
        {
            /* Check if we came from user mode */
            if (AccessMode != KernelMode)
            {
                /* Check if we have no debug port */
                if (Process->DebugPort)
                {
                    /* Make sure we're not attached */
                    if (!KeIsAttachedProcess())
                    {
                        /* Raise an exception */
                        return KeRaiseUserException(STATUS_INVALID_HANDLE);
                    }
                }
            }
            else
            {
                /* This is kernel mode. Check if we're exiting */
                if (!(PsIsThreadTerminating(PsGetCurrentThread())) &&
                    (Process->Peb))
                {
                    /* Check if the debugger is enabled */
                    if (KdDebuggerEnabled)
                    {
                        /* Bugcheck */
                        KeBugCheckEx(INVALID_KERNEL_HANDLE, (ULONG_PTR)Handle, 1, 0, 0);
                    }
                }
            }
        }

        /* Set invalid status */
        Status = STATUS_INVALID_HANDLE;
    }

    /* Return status */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Closed handle: %p S: %lx\n",
            __FUNCTION__, Handle, Status);
    return Status;
}

/*++
* @name ObpSetHandleAttributes
*
*     The ObpSetHandleAttributes routine <FILLMEIN>
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
ObpSetHandleAttributes(IN OUT PHANDLE_TABLE_ENTRY HandleTableEntry,
                       IN ULONG_PTR Context)
{
    POBP_SET_HANDLE_ATTRIBUTES_CONTEXT SetHandleInfo = (PVOID)Context;
    POBJECT_HEADER ObjectHeader = ObpGetHandleObject(HandleTableEntry);

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
        HandleTableEntry->ObAttributes |= OBJ_INHERIT;
    }
    else
    {
        /* Otherwise this implies we're removing the flag */
        HandleTableEntry->ObAttributes &= ~OBJ_INHERIT;
    }

    /* Check if making the handle protected */
    if (SetHandleInfo->Information.ProtectFromClose)
    {
        /* Set the flag */
        HandleTableEntry->GrantedAccess |= ObpAccessProtectCloseBit;
    }
    else
    {
        /* Otherwise, remove it */
        HandleTableEntry->GrantedAccess &= ~ObpAccessProtectCloseBit;
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
BOOLEAN
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
    return TRUE;
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
ObpDuplicateHandleCallback(IN PEPROCESS Process,
                           IN PHANDLE_TABLE HandleTable,
                           IN PHANDLE_TABLE_ENTRY OldEntry,
                           IN PHANDLE_TABLE_ENTRY HandleTableEntry)
{
    POBJECT_HEADER ObjectHeader;
    BOOLEAN Ret = FALSE;
    ACCESS_STATE AccessState;
    NTSTATUS Status;
    PAGED_CODE();

    /* Make sure that the handle is inheritable */
    Ret = (HandleTableEntry->ObAttributes & OBJ_INHERIT) != 0;
    if (Ret)
    {
        /* Get the object header */
        ObjectHeader = ObpGetHandleObject(HandleTableEntry);

        /* Increment the pointer count */
        InterlockedIncrementSizeT(&ObjectHeader->PointerCount);

        /* Release the handle lock */
        ExUnlockHandleTableEntry(HandleTable, OldEntry);

        /* Setup the access state */
        AccessState.PreviouslyGrantedAccess = HandleTableEntry->GrantedAccess;

        /* Call the shared routine for incrementing handles */
        Status = ObpIncrementHandleCount(&ObjectHeader->Body,
                                         &AccessState,
                                         KernelMode,
                                         HandleTableEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES,
                                         Process,
                                         ObInheritHandle);
        if (!NT_SUCCESS(Status))
        {
            /* Return failure */
            ObDereferenceObject(&ObjectHeader->Body);
            Ret = FALSE;
        }
    }
    else
    {
        /* Release the handle lock */
        ExUnlockHandleTableEntry(HandleTable, OldEntry);
    }

    /* Return duplication result */
    return Ret;
}

/*++
* @name ObClearProcessHandleTable
*
*     The ObClearProcessHandleTable routine clears the handle table
*     of the given process.
*
* @param Process
*        The process of which the handle table should be cleared.
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
NTAPI
ObClearProcessHandleTable(IN PEPROCESS Process)
{
    PHANDLE_TABLE HandleTable;
    OBP_CLOSE_HANDLE_CONTEXT Context;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE;

    ASSERT(Process);

    /* Ensure the handle table doesn't go away while we use it */
    HandleTable = ObReferenceProcessHandleTable(Process);
    if (!HandleTable) return;

    /* Attach to the current process if needed */
    if (PsGetCurrentProcess() != Process)
    {
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        AttachedToProcess = TRUE;
    }

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Fill out the context */
    Context.AccessMode = UserMode;
    Context.HandleTable = HandleTable;

    /* Sweep the handle table to close all handles */
    ExSweepHandleTable(HandleTable,
                       ObpCloseHandleCallback,
                       &Context);

    /* Leave the critical region */
    KeLeaveCriticalRegion();

    /* Detach if needed */
    if (AttachedToProcess)
        KeUnstackDetachProcess(&ApcState);

    /* Let the handle table go */
    ObDereferenceProcessHandleTable(Process);
}

/*++
* @name ObInitProcess
*
*     The ObInitProcess routine initializes the handle table for the process
*     to be initialized, by either creating a new one or duplicating it from
*     the parent process.
*
* @param Parent
*        A parent process (optional).
*
* @param Process
*        The process to initialize.
*
* @return Success or failure.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObInitProcess(IN PEPROCESS Parent OPTIONAL,
              IN PEPROCESS Process)
{
    PHANDLE_TABLE ParentTable, ObjectTable;

    /* Check for a parent */
    if (Parent)
    {
        /* Reference the parent's table */
        ParentTable = ObReferenceProcessHandleTable(Parent);
        if (!ParentTable) return STATUS_PROCESS_IS_TERMINATING;

        /* Duplicate it */
        ObjectTable = ExDupHandleTable(Process,
                                       ParentTable,
                                       ObpDuplicateHandleCallback,
                                       OBJ_INHERIT);
    }
    else
    {
        /* Otherwise just create a new table */
        ParentTable = NULL;
        ObjectTable = ExCreateHandleTable(Process);
    }

    /* Make sure we have a table */
    if (ObjectTable)
    {
        /* Associate it */
        Process->ObjectTable = ObjectTable;

        /* Check for auditing */
        if (SeDetailedAuditingWithToken(NULL))
        {
            /* FIXME: TODO */
            DPRINT1("Need auditing!\n");
        }

        /* Get rid of the old table now */
        if (ParentTable) ObDereferenceProcessHandleTable(Parent);

        /* We are done */
        return STATUS_SUCCESS;
    }
    else
    {
        /* Fail */
        Process->ObjectTable = NULL;
        if (ParentTable) ObDereferenceProcessHandleTable(Parent);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

/*++
* @name ObKillProcess
*
*     The ObKillProcess routine performs rundown operations on the process,
*     then clears and destroys its handle table.
*
* @param Process
*        The process to be killed.
*
* @return None.
*
* @remarks Called by the Object Manager cleanup code (kernel)
*          when a process is to be destroyed.
*
*--*/
VOID
NTAPI
ObKillProcess(IN PEPROCESS Process)
{
    PHANDLE_TABLE HandleTable;
    OBP_CLOSE_HANDLE_CONTEXT Context;
    BOOLEAN HardErrors;
    PAGED_CODE();

    /* Wait for process rundown and then complete it */
    ExWaitForRundownProtectionRelease(&Process->RundownProtect);
    ExRundownCompleted(&Process->RundownProtect);

    /* Get the object table */
    HandleTable = Process->ObjectTable;
    if (!HandleTable) return;

    /* Disable hard errors while we close handles */
    HardErrors = IoSetThreadHardErrorMode(FALSE);

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Fill out the context */
    Context.AccessMode = KernelMode;
    Context.HandleTable = HandleTable;

    /* Sweep the handle table to close all handles */
    ExSweepHandleTable(HandleTable,
                       ObpCloseHandleCallback,
                       &Context);
    ASSERT(HandleTable->HandleCount == 0);

    /* Leave the critical region */
    KeLeaveCriticalRegion();

    /* Re-enable hard errors */
    IoSetThreadHardErrorMode(HardErrors);

    /* Destroy the object table */
    Process->ObjectTable = NULL;
    ExDestroyHandleTable(HandleTable, NULL);
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
    NTSTATUS Status;
    ACCESS_MASK TargetAccess, SourceAccess;
    ACCESS_STATE AccessState;
    PACCESS_STATE PassedAccessState = NULL;
    AUX_ACCESS_DATA AuxData;
    PHANDLE_TABLE HandleTable;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    ULONG AuditMask;
    BOOLEAN KernelHandle = FALSE;

    PAGED_CODE();
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicating handle: %p for %p into %p\n",
            __FUNCTION__,
            SourceHandle,
            SourceProcess,
            TargetProcess);

    /* Assume failure */
    if (TargetHandle) *TargetHandle = NULL;

    /* Check if we're not duplicating the same access */
    if (!(Options & DUPLICATE_SAME_ACCESS))
    {
        /* Validate the desired access */
        Status = STATUS_SUCCESS; //ObpValidateDesiredAccess(DesiredAccess);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Reference the object table */
    HandleTable = ObReferenceProcessHandleTable(SourceProcess);
    if (!HandleTable) return STATUS_PROCESS_IS_TERMINATING;

    /* Reference the process object */
    Status = ObpReferenceProcessObjectByHandle(SourceHandle,
                                               SourceProcess,
                                               HandleTable,
                                               PreviousMode,
                                               &SourceObject,
                                               &HandleInformation,
                                               &AuditMask);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObDereferenceProcessHandleTable(SourceProcess);
        return Status;
    }
    else
    {
        /* Check if we have to don't have to audit object close */
        if (!(HandleInformation.HandleAttributes & OBJ_AUDIT_OBJECT_CLOSE))
        {
            /* Then there is no audit mask */
            AuditMask = 0;
        }
    }

    /* Check if there's no target process */
    if (!TargetProcess)
    {
        /* Check if the caller wanted actual duplication */
        if (!(Options & DUPLICATE_CLOSE_SOURCE))
        {
            /* Invalid request */
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            /* Otherwise, do the attach */
            KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

            /* Close the handle and detach */
            NtClose(SourceHandle);
            KeUnstackDetachProcess(&ApcState);
        }

        /* Return */
        ObDereferenceProcessHandleTable(SourceProcess);
        ObDereferenceObject(SourceObject);
        return Status;
    }

    /* Create a kernel handle if asked, but only in the system process */
    if (PreviousMode == KernelMode &&
        HandleAttributes & OBJ_KERNEL_HANDLE &&
        TargetProcess == PsInitialSystemProcess)
    {
        KernelHandle = TRUE;
    }

    /* Get the target handle table */
    HandleTable = ObReferenceProcessHandleTable(TargetProcess);
    if (!HandleTable)
    {
        /* Check if the caller wanted us to close the handle */
        if (Options & DUPLICATE_CLOSE_SOURCE)
        {
            /* Do the attach */
            KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

            /* Close the handle and detach */
            NtClose(SourceHandle);
            KeUnstackDetachProcess(&ApcState);
        }

        /* Return */
        ObDereferenceProcessHandleTable(SourceProcess);
        ObDereferenceObject(SourceObject);
        return STATUS_PROCESS_IS_TERMINATING;
    }

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
    else
    {
        /* Don't allow caller to bypass auditing */
        HandleAttributes |= HandleInformation.HandleAttributes &
                            OBJ_AUDIT_OBJECT_CLOSE;
    }

    /* Check if we're duplicating the access */
    if (Options & DUPLICATE_SAME_ACCESS) DesiredAccess = SourceAccess;

    /* Get object data */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(SourceObject);
    ObjectType = ObjectHeader->Type;

    /* Fill out the entry */
    RtlZeroMemory(&NewHandleEntry, sizeof(HANDLE_TABLE_ENTRY));
    NewHandleEntry.Object = ObjectHeader;
    NewHandleEntry.ObAttributes |= (HandleAttributes & OBJ_HANDLE_ATTRIBUTES);

    /* Check if we're using a generic mask */
    if (DesiredAccess & GENERIC_ACCESS)
    {
        /* Map it */
        RtlMapGenericMask(&DesiredAccess,
                          &ObjectType->TypeInfo.GenericMapping);
    }

    /* Set the target access, always propagate ACCESS_SYSTEM_SECURITY */
    TargetAccess = DesiredAccess & (ObjectType->TypeInfo.ValidAccessMask |
                                    ACCESS_SYSTEM_SECURITY);
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
        /* Dereference handle tables */
        ObDereferenceProcessHandleTable(SourceProcess);
        ObDereferenceProcessHandleTable(TargetProcess);

        /* Dereference the source object */
        ObDereferenceObject(SourceObject);
        return Status;
    }

    if (NewHandleEntry.ObAttributes & OBJ_PROTECT_CLOSE)
    {
        NewHandleEntry.ObAttributes &= ~OBJ_PROTECT_CLOSE;
        NewHandleEntry.GrantedAccess |= ObpAccessProtectCloseBit;
    }

    /* Now create the handle */
    NewHandle = ExCreateHandle(HandleTable, &NewHandleEntry);
    if (!NewHandle)
    {
        /* Undo the increment */
        ObpDecrementHandleCount(SourceObject,
                                TargetProcess,
                                TargetAccess,
                                ObjectType);

        /* Deference the object and set failure status */
        ObDereferenceObject(SourceObject);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Mark it as a kernel handle if requested */
    if (KernelHandle)
    {
        NewHandle = ObMarkHandleAsKernelHandle(NewHandle);
    }

    /* Return the handle */
    if (TargetHandle) *TargetHandle = NewHandle;

    /* Dereference handle tables */
    ObDereferenceProcessHandleTable(SourceProcess);
    ObDereferenceProcessHandleTable(TargetProcess);

    /* Return status */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicated handle: %p for %p into %p. Source: %p HC PC %lx %lx\n",
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
    NTSTATUS Status, Status2;
    POBJECT_HEADER ObjectHeader;
    PGENERIC_MAPPING GenericMapping = NULL;
    OB_OPEN_REASON OpenReason;
    POB_TEMP_BUFFER TempBuffer;
    PAGED_CODE();

    /* Assume failure */
    *Handle = NULL;

    /* Check if we didn't get any Object Attributes */
    if (!ObjectAttributes)
    {
        /* Fail with special status code */
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate the temporary buffer */
    TempBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(OB_TEMP_BUFFER),
                                       TAG_OB_TEMP_STORAGE);
    if (!TempBuffer) return STATUS_INSUFFICIENT_RESOURCES;

    /* Capture all the info */
    Status = ObpCaptureObjectCreateInformation(ObjectAttributes,
                                               AccessMode,
                                               AccessMode,
                                               TRUE,
                                               &TempBuffer->ObjectCreateInfo,
                                               &ObjectName);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ExFreePoolWithTag(TempBuffer, TAG_OB_TEMP_STORAGE);
        return Status;
    }

    /* Check if we didn't get an access state */
    if (!PassedAccessState)
    {
        /* Try to get the generic mapping if we can */
        if (ObjectType) GenericMapping = &ObjectType->TypeInfo.GenericMapping;

        /* Use our built-in access state */
        PassedAccessState = &TempBuffer->LocalAccessState;
        Status = SeCreateAccessState(&TempBuffer->LocalAccessState,
                                     &TempBuffer->AuxData,
                                     DesiredAccess,
                                     GenericMapping);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    /* Get the security descriptor */
    if (TempBuffer->ObjectCreateInfo.SecurityDescriptor)
    {
        /* Save it in the access state */
        PassedAccessState->SecurityDescriptor =
            TempBuffer->ObjectCreateInfo.SecurityDescriptor;
    }

    /* Validate the access mask */
    Status = ObpValidateAccessMask(PassedAccessState);
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup after lookup */
        ObpReleaseLookupContext(&TempBuffer->LookupContext);
        goto Cleanup;
    }

    /* Now do the lookup */
    Status = ObpLookupObjectName(TempBuffer->ObjectCreateInfo.RootDirectory,
                                 &ObjectName,
                                 TempBuffer->ObjectCreateInfo.Attributes,
                                 ObjectType,
                                 AccessMode,
                                 ParseContext,
                                 TempBuffer->ObjectCreateInfo.SecurityQos,
                                 NULL,
                                 PassedAccessState,
                                 &TempBuffer->LookupContext,
                                 &Object);
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup after lookup */
        ObpReleaseLookupContext(&TempBuffer->LookupContext);
        goto Cleanup;
    }

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
            ObpFreeObjectCreateInformation(ObjectHeader->
                                                ObjectCreateInfo);
            ObjectHeader->ObjectCreateInfo = NULL;
        }
    }
    else
    {
        /* Otherwise, we are merely opening it */
        OpenReason = ObOpenHandle;
    }

    /* Check if we have invalid object attributes */
    if (ObjectHeader->Type->TypeInfo.InvalidAttributes &
        TempBuffer->ObjectCreateInfo.Attributes)
    {
        /* Set failure code */
        Status = STATUS_INVALID_PARAMETER;

        /* Cleanup after lookup */
        ObpReleaseLookupContext(&TempBuffer->LookupContext);

        /* Dereference the object */
        ObDereferenceObject(Object);
    }
    else
    {
        /* Create the actual handle now */
        Status2 = ObpCreateHandle(OpenReason,
                                  Object,
                                  ObjectType,
                                  PassedAccessState,
                                  0,
                                  TempBuffer->ObjectCreateInfo.Attributes,
                                  &TempBuffer->LookupContext,
                                  AccessMode,
                                  NULL,
                                  Handle);
        if (!NT_SUCCESS(Status2))
        {
            ObDereferenceObject(Object);
            Status = Status2;
        }
    }

Cleanup:
    /* Delete the access state */
    if (PassedAccessState == &TempBuffer->LocalAccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

Quickie:
    /* Release the object attributes and temporary buffer */
    ObpReleaseObjectCreateInformation(&TempBuffer->ObjectCreateInfo);
    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    ExFreePoolWithTag(TempBuffer, TAG_OB_TEMP_STORAGE);

    /* Return status */
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
    AUX_ACCESS_DATA AuxData;
    PAGED_CODE();

    /* Assume failure */
    *Handle = NULL;

    /* Reference the object */
    Status = ObReferenceObjectByPointer(Object,
                                        0,
                                        ObjectType,
                                        AccessMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Header Info */
    Header = OBJECT_TO_OBJECT_HEADER(Object);

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

    /* Check if we have invalid object attributes */
    if (Header->Type->TypeInfo.InvalidAttributes & HandleAttributes)
    {
        /* Delete the access state */
        if (PassedAccessState == &AccessState)
        {
            SeDeleteAccessState(PassedAccessState);
        }

        /* Dereference the object */
        ObDereferenceObject(Object);
        return STATUS_INVALID_PARAMETER;
    }

    /* Create the handle */
    Status = ObpCreateHandle(ObOpenHandle,
                             Object,
                             ObjectType,
                             PassedAccessState,
                             0,
                             HandleAttributes,
                             NULL,
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

/*++
* @name ObFindHandleForObject
* @implemented NT4
*
*     The ObFindHandleForObject routine <FILLMEIN>
*
* @param Process
*        <FILLMEIN>.
*
* @param Object
*        <FILLMEIN>.
*
* @param ObjectType
*        <FILLMEIN>.
*
* @param HandleInformation
*        <FILLMEIN>.
*
* @param HandleReturn
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
BOOLEAN
NTAPI
ObFindHandleForObject(IN PEPROCESS Process,
                      IN PVOID Object,
                      IN POBJECT_TYPE ObjectType,
                      IN POBJECT_HANDLE_INFORMATION HandleInformation,
                      OUT PHANDLE Handle)
{
    OBP_FIND_HANDLE_DATA FindData;
    BOOLEAN Result = FALSE;
    PVOID ObjectTable;

    /* Make sure we have an object table */
    ObjectTable = ObReferenceProcessHandleTable(Process);
    if (ObjectTable)
    {
        /* Check if we have an object */
        if (Object)
        {
            /* Set its header */
            FindData.ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
        }
        else
        {
            /* Otherwise, no object to match*/
            FindData.ObjectHeader = NULL;
        }

        /* Set other information */
        FindData.ObjectType = ObjectType;
        FindData.HandleInformation = HandleInformation;

        /* Enumerate the handle table */
        if (ExEnumHandleTable(Process->ObjectTable,
                              ObpEnumFindHandleProcedure,
                              &FindData,
                              Handle))
        {
            /* Set success */
            Result = TRUE;
        }

        /* Let go of the table */
        ObDereferenceProcessHandleTable(Process);
    }

    /* Return the result */
    return Result;
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
               IN PACCESS_STATE AccessState OPTIONAL,
               IN ACCESS_MASK DesiredAccess,
               IN ULONG ObjectPointerBias,
               OUT PVOID *NewObject OPTIONAL,
               OUT PHANDLE Handle)
{
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PUNICODE_STRING ObjectName;
    PVOID InsertObject;
    PSECURITY_DESCRIPTOR ParentDescriptor = NULL;
    BOOLEAN SdAllocated = FALSE;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    OBP_LOOKUP_CONTEXT Context;
    ACCESS_STATE LocalAccessState;
    AUX_ACCESS_DATA AuxData;
    OB_OPEN_REASON OpenReason;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS, RealStatus;
    BOOLEAN IsNewObject;
    PAGED_CODE();

    /* Get the Header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    /* Detect invalid insert */
    if (!(ObjectHeader->Flags & OB_FLAG_CREATE_INFO))
    {
        /* Display warning and break into debugger */
        DPRINT1("OB: Attempting to insert existing object %p\n", Object);
        DbgBreakPoint();

        /* Allow debugger to continue */
        ObDereferenceObject(Object);
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the create and name info, as well as the object type */
    ObjectCreateInfo = ObjectHeader->ObjectCreateInfo;
    ObjectNameInfo = ObpReferenceNameInfo(ObjectHeader);
    ObjectType = ObjectHeader->Type;
    ObjectName = NULL;

    /* Check if this is an named object */
    if ((ObjectNameInfo) && (ObjectNameInfo->Name.Buffer))
    {
        /* Get the object name */
        ObjectName = &ObjectNameInfo->Name;
    }

    /* Sanity check */
    ASSERT((Handle) ||
           ((ObjectPointerBias == 0) &&
            (ObjectName == NULL) &&
            (ObjectType->TypeInfo.SecurityRequired) &&
            (NewObject == NULL)));

    /* Check if the object is unnamed and also doesn't have security */
    PreviousMode = KeGetPreviousMode();
    if (!(ObjectType->TypeInfo.SecurityRequired) && !(ObjectName))
    {
        /* Assume failure */
        *Handle = NULL;
        ObjectHeader->ObjectCreateInfo = NULL;

        /* Create the handle */
        Status = ObpCreateUnnamedHandle(Object,
                                        DesiredAccess,
                                        ObjectPointerBias + 1,
                                        ObjectCreateInfo->Attributes,
                                        PreviousMode,
                                        NewObject,
                                        Handle);

        /* Free the create information */
        ObpFreeObjectCreateInformation(ObjectCreateInfo);

        /* Release the object name information */
        ObpDereferenceNameInfo(ObjectNameInfo);

        /* Remove the extra keep-alive reference */
        ObDereferenceObject(Object);

        /* Return */
        OBTRACE(OB_HANDLE_DEBUG,
                "%s - returning Object with PC S: %lx %lx\n",
                __FUNCTION__,
                ObjectHeader->PointerCount,
                Status);
        return Status;
    }

    /* Check if we didn't get an access state */
    if (!AccessState)
    {
        /* Use our built-in access state */
        AccessState = &LocalAccessState;
        Status = SeCreateAccessState(&LocalAccessState,
                                     &AuxData,
                                     DesiredAccess,
                                     &ObjectType->TypeInfo.GenericMapping);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObpDereferenceNameInfo(ObjectNameInfo);
            ObDereferenceObject(Object);
            return Status;
        }
    }

    /* Save the security descriptor */
    AccessState->SecurityDescriptor = ObjectCreateInfo->SecurityDescriptor;

    /* Validate the access mask */
    Status = ObpValidateAccessMask(AccessState);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObpDereferenceNameInfo(ObjectNameInfo);
        ObDereferenceObject(Object);
        return Status;
    }

    /* Setup a lookup context */
    ObpInitializeLookupContext(&Context);
    InsertObject = Object;
    OpenReason = ObCreateHandle;

    /* Check if the object is named */
    if (ObjectName)
    {
        /* Look it up */
        Status = ObpLookupObjectName(ObjectCreateInfo->RootDirectory,
                                     ObjectName,
                                     ObjectCreateInfo->Attributes,
                                     ObjectType,
                                     (ObjectHeader->Flags & OB_FLAG_KERNEL_MODE) ?
                                     KernelMode : UserMode,
                                     ObjectCreateInfo->ParseContext,
                                     ObjectCreateInfo->SecurityQos,
                                     Object,
                                     AccessState,
                                     &Context,
                                     &InsertObject);

        /* Check if we found an object that doesn't match the one requested */
        if ((NT_SUCCESS(Status)) && (InsertObject) && (Object != InsertObject))
        {
            /* This means we're opening an object, not creating a new one */
            OpenReason = ObOpenHandle;

            /* Make sure the caller said it's OK to do this */
            if (ObjectCreateInfo->Attributes & OBJ_OPENIF)
            {
                /* He did, but did he want this type? */
                if (ObjectType != OBJECT_TO_OBJECT_HEADER(InsertObject)->Type)
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
                /* Check if this was a symbolic link */
                if (OBJECT_TO_OBJECT_HEADER(InsertObject)->Type ==
                    ObpSymbolicLinkObjectType)
                {
                    /* Dereference it */
                    ObDereferenceObject(InsertObject);
                }

                /* Caller wanted to create a new object, fail */
                Status = STATUS_OBJECT_NAME_COLLISION;
            }
        }

        /* Check if anything until now failed */
        if (!NT_SUCCESS(Status))
        {
            /* Cleanup after lookup */
            ObpReleaseLookupContext(&Context);

            /* Remove query reference that we added */
            ObpDereferenceNameInfo(ObjectNameInfo);

            /* Dereference the object and delete the access state */
            ObDereferenceObject(Object);
            if (AccessState == &LocalAccessState)
            {
                /* We used a local one; delete it */
                SeDeleteAccessState(AccessState);
            }

            /* Return failure code */
            return Status;
        }
        else
        {
            /* Check if this is a symbolic link */
            if (ObjectType == ObpSymbolicLinkObjectType)
            {
                /* Create the internal name */
                ObpCreateSymbolicLinkName(Object);
            }
        }
    }

    /* Now check if this object is being created */
    if (InsertObject == Object)
    {
        /* Check if it's named or forces security */
        if ((ObjectName) || (ObjectType->TypeInfo.SecurityRequired))
        {
            /* Make sure it's inserted into an object directory */
            if ((ObjectNameInfo) && (ObjectNameInfo->Directory))
            {
                /* Get the current descriptor */
                ObGetObjectSecurity(ObjectNameInfo->Directory,
                                    &ParentDescriptor,
                                    &SdAllocated);
            }

            /* Now assign it */
            Status = ObAssignSecurity(AccessState,
                                      ParentDescriptor,
                                      Object,
                                      ObjectType);

            /* Check if we captured one */
            if (ParentDescriptor)
            {
                /* We did, release it */
                ObReleaseObjectSecurity(ParentDescriptor, SdAllocated);
            }
            else if (NT_SUCCESS(Status))
            {
                /* Other we didn't, but we were able to use the current SD */
                SeReleaseSecurityDescriptor(ObjectCreateInfo->SecurityDescriptor,
                                            ObjectCreateInfo->ProbeMode,
                                            TRUE);

                /* Clear the current one */
                AccessState->SecurityDescriptor =
                ObjectCreateInfo->SecurityDescriptor = NULL;
            }
        }

        /* Check if anything until now failed */
        if (!NT_SUCCESS(Status))
        {
            /* Check if the directory was added */
            if (Context.DirectoryLocked)
            {
                /* Weird case where we need to do a manual delete */
                DPRINT1("Unhandled path\n");
                ASSERT(FALSE);
            }

            /* Cleanup the lookup */
            ObpReleaseLookupContext(&Context);

            /* Remove query reference that we added */
            ObpDereferenceNameInfo(ObjectNameInfo);

            /* Dereference the object and delete the access state */
            ObDereferenceObject(Object);
            if (AccessState == &LocalAccessState)
            {
                /* We used a local one; delete it */
                SeDeleteAccessState(AccessState);
            }

            /* Return failure code */
            ASSERT(FALSE);
            return Status;
        }
    }

    /* Save the actual status until here */
    RealStatus = Status;

    /* Check if caller wants us to create a handle */
    ObjectHeader->ObjectCreateInfo = NULL;
    if (Handle)
    {
        /* Create the handle */
        Status = ObpCreateHandle(OpenReason,
                                 InsertObject,
                                 NULL,
                                 AccessState,
                                 ObjectPointerBias + 1,
                                 ObjectCreateInfo->Attributes,
                                 &Context,
                                 PreviousMode,
                                 NewObject,
                                 Handle);
        if (!NT_SUCCESS(Status))
        {
            /* If the object had a name, backout everything */
            if (ObjectName) ObpDeleteNameCheck(Object);

            /* Return the status of the failure */
            *Handle = NULL;
            RealStatus = Status;
        }

        /* Remove a query reference */
        ObpDereferenceNameInfo(ObjectNameInfo);

        /* Remove the extra keep-alive reference */
        ObDereferenceObject(Object);
    }
    else
    {
        /* Otherwise, lock the object */
        ObpAcquireObjectLock(ObjectHeader);

        /* And charge quota for the process to make it appear as used */
        RealStatus = ObpChargeQuotaForObject(ObjectHeader,
                                             ObjectType,
                                             &IsNewObject);

        /* Release the lock */
        ObpReleaseObjectLock(ObjectHeader);

        /* Check if we failed and dereference the object if so */
        if (!NT_SUCCESS(RealStatus)) ObDereferenceObject(Object);
    }

    /* We can delete the Create Info now */
    ObpFreeObjectCreateInformation(ObjectCreateInfo);

    /* Check if we created our own access state and delete it if so */
    if (AccessState == &LocalAccessState) SeDeleteAccessState(AccessState);

    /* Return status code */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - returning Object with PC RS/S: %lx %lx %lx\n",
            __FUNCTION__,
            OBJECT_TO_OBJECT_HEADER(Object)->PointerCount,
            RealStatus, Status);
    return RealStatus;
}

/*++
* @name ObSetHandleAttributes
* @implemented NT5.1
*
*     The ObSetHandleAttributes routine <FILLMEIN>
*
* @param Handle
*        <FILLMEIN>.
*
* @param HandleFlags
*        <FILLMEIN>.
*
* @param PreviousMode
*        <FILLMEIN>.
*
* @return <FILLMEIN>.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObSetHandleAttributes(IN HANDLE Handle,
                      IN POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleFlags,
                      IN KPROCESSOR_MODE PreviousMode)
{
    OBP_SET_HANDLE_ATTRIBUTES_CONTEXT SetHandleAttributesContext;
    BOOLEAN Result, AttachedToSystemProcess = FALSE;
    PHANDLE_TABLE HandleTable;
    KAPC_STATE ApcState;
    PAGED_CODE();

    /* Check if this is a kernel handle */
    if (ObpIsKernelHandle(Handle, PreviousMode))
    {
        /* Use the kernel table and convert the handle */
        HandleTable = ObpKernelHandleTable;
        Handle = ObKernelHandleToHandle(Handle);

        /* Check if we're not in the system process */
        if (PsGetCurrentProcess() != PsInitialSystemProcess)
        {
            /* Attach to the system process */
            KeStackAttachProcess(&PsInitialSystemProcess->Pcb, &ApcState);
            AttachedToSystemProcess = TRUE;
        }
    }
    else
    {
        /* Get the current process' handle table */
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }

    /* Initialize the handle attribute context */
    SetHandleAttributesContext.PreviousMode = PreviousMode;
    SetHandleAttributesContext.Information = *HandleFlags;

    /* Invoke the ObpSetHandleAttributes callback */
    Result = ExChangeHandle(HandleTable,
                            Handle,
                            ObpSetHandleAttributes,
                            (ULONG_PTR)&SetHandleAttributesContext);

    /* Did we attach to the system process? */
    if (AttachedToSystemProcess)
    {
        /* Detach from it */
        KeUnstackDetachProcess(&ApcState);
    }

    /* Return the result as an NTSTATUS value */
    return Result ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
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
    /* Call the internal API */
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
    /* Call the internal API */
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
    NTSTATUS Status;
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicating handle: %p for %p into %p.\n",
            __FUNCTION__,
            SourceHandle,
            SourceProcessHandle,
            TargetProcessHandle);

    /* Check if we have a target handle */
    if ((TargetHandle) && (PreviousMode != KernelMode))
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Probe the handle and assume failure */
            ProbeForWriteHandle(TargetHandle);
            *TargetHandle = NULL;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Now reference the input handle */
    Status = ObReferenceObjectByHandle(SourceProcessHandle,
                                       PROCESS_DUP_HANDLE,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&SourceProcess,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

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
        _SEH2_TRY
        {
            /* Write the new handle */
            *TargetHandle = hTarget;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Otherwise, get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Dereference the processes */
    OBTRACE(OB_HANDLE_DEBUG,
            "%s - Duplicated handle: %p into %p S %lx\n",
            __FUNCTION__,
            hTarget,
            TargetProcessHandle,
            Status);
    if (Target) ObDereferenceObject(Target);
    ObDereferenceObject(SourceProcess);
    return Status;
}

BOOLEAN
NTAPI
ObIsKernelHandle(IN HANDLE Handle)
{
    /* Use the inlined version. We know we are in kernel mode. */
    return ObpIsKernelHandle(Handle, KernelMode);
}

/* EOF */
