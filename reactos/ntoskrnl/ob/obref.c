/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/refderef.c
 * PURPOSE:         Manages the referencing and de-referencing of all Objects,
 *                  as well as the Object Fast Reference implementation.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FASTCALL
ObInitializeFastReference(IN PEX_FAST_REF FastRef,
                          PVOID Object)
{
    /* FIXME: Fast Referencing is Unimplemented */
    FastRef->Object = Object;
}

PVOID
FASTCALL
ObFastReferenceObject(IN PEX_FAST_REF FastRef)
{
    /* FIXME: Fast Referencing is Unimplemented */

    /* Do a normal Reference */
    ObReferenceObject(FastRef->Object);

    /* Return the Object */
    return FastRef->Object;
}

VOID
FASTCALL
ObFastDereferenceObject(IN PEX_FAST_REF FastRef,
                        PVOID Object)
{
    /* FIXME: Fast Referencing is Unimplemented */

    /* Do a normal Dereference */
    ObDereferenceObject(FastRef->Object);
}

PVOID
FASTCALL
ObFastReplaceObject(IN PEX_FAST_REF FastRef,
                    PVOID Object)
{
    PVOID OldObject = FastRef->Object;

    /* FIXME: Fast Referencing is Unimplemented */
    FastRef->Object = Object;

    /* Do a normal Dereference */
    ObDereferenceObject(OldObject);

    /* Return old Object*/
    return OldObject;
}

/* PUBLIC FUNCTIONS *********************************************************/

VOID
FASTCALL
ObfReferenceObject(IN PVOID Object)
{
    ASSERT(Object);

    /* Get the header and increment the reference count */
    InterlockedIncrement(&OBJECT_TO_OBJECT_HEADER(Object)->PointerCount);
}

VOID
FASTCALL
ObfDereferenceObject(IN PVOID Object)
{
    POBJECT_HEADER Header;

    /* Extract the object header */
    Header = OBJECT_TO_OBJECT_HEADER(Object);

    /* Check whether the object can now be deleted. */
    if (!(InterlockedDecrement(&Header->PointerCount)) &&
        !(Header->Flags & OB_FLAG_PERMANENT))
    {
        /* Sanity check */
        ASSERT(!Header->HandleCount);

        /* Check if we're at PASSIVE */
        if (KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            /* Remove the object */
            ObpDeleteObject(Object);
        }
        else
        {
            /* Add us to the list */
            do
            {
                Header->NextToFree = ObpReaperList;
            } while (InterlockedCompareExchangePointer(&ObpReaperList,
                                                       Header,
                                                       Header->NextToFree) !=
                     Header->NextToFree);

            /* Queue the work item */
            KeBugCheck(0);
            ExQueueWorkItem(&ObpReaperWorkItem, DelayedWorkQueue);
        }
    }
}

#ifdef ObDereferenceObject
#undef ObDereferenceObject
#endif

VOID
NTAPI
ObDereferenceObject(IN PVOID Object)
{
    /* Call the fastcall function */
    ObfDereferenceObject(Object);
}

NTSTATUS
NTAPI
ObReferenceObjectByPointer(IN PVOID Object,
                           IN ACCESS_MASK DesiredAccess,
                           IN POBJECT_TYPE ObjectType,
                           IN KPROCESSOR_MODE AccessMode)
{
    POBJECT_HEADER Header;

    /* Get the header */
    Header = OBJECT_TO_OBJECT_HEADER(Object);

    /*
     * Validate object type if the call is for UserMode.
     * NOTE: Unless it's a symbolic link (Caz Yokoyama [MSFT])
     */
    if ((Header->Type != ObjectType) && ((AccessMode != KernelMode) ||
        (ObjectType == ObSymbolicLinkType)))
    {
        /* Invalid type */
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Oncrement the reference count and return success */
    InterlockedIncrement(&Header->PointerCount);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
                        ULONG Attributes,
                        PACCESS_STATE PassedAccessState,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_TYPE ObjectType,
                        KPROCESSOR_MODE AccessMode,
                        PVOID ParseContext,
                        PVOID* ObjectPtr)
{
    PVOID Object = NULL;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    OBP_LOOKUP_CONTEXT Context;
    AUX_DATA AuxData;
    ACCESS_STATE AccessState;

    /* Capture the name */
    Status = ObpCaptureObjectName(&ObjectName, ObjectPath, AccessMode, TRUE);
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
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    /* Find the object */
    *ObjectPtr = NULL;
    Status = ObFindObject(NULL,
                          &ObjectName,
                          Attributes,
                          AccessMode,
                          &Object,
                          ObjectType,
                          &Context,
                          PassedAccessState,
                          NULL,
                          ParseContext,
                          NULL);
    if (NT_SUCCESS(Status))
    {
        /* Return the object */
        *ObjectPtr = Object;
    }

    /* Free the access state */
    if (PassedAccessState == &AccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

Quickie:
    /* Free the captured name if we had one, and return status */
    if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);
    return Status;
}

NTSTATUS
NTAPI
ObReferenceObjectByHandle(IN HANDLE Handle,
                          IN ACCESS_MASK DesiredAccess,
                          IN POBJECT_TYPE ObjectType,
                          IN KPROCESSOR_MODE AccessMode,
                          OUT PVOID* Object,
                          OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL)
{
    PHANDLE_TABLE_ENTRY HandleEntry;
    POBJECT_HEADER ObjectHeader;
    ACCESS_MASK GrantedAccess;
    ULONG Attributes;
    PEPROCESS CurrentProcess;
    PVOID HandleTable;
    PETHREAD CurrentThread;
    NTSTATUS Status;
    PAGED_CODE();

    /* Fail immediately if the handle is NULL */
    if (!Handle) return STATUS_INVALID_HANDLE;

    /* Check if the caller wants the current process */
    if ((Handle == NtCurrentProcess()) &&
        ((ObjectType == PsProcessType) || !(ObjectType)))
    {
        /* Get the current process */
        CurrentProcess = PsGetCurrentProcess();

        /* Reference ourselves */
        ObReferenceObject(CurrentProcess);

        /* Check if the caller wanted handle information */
        if (HandleInformation)
        {
            /* Return it */
            HandleInformation->HandleAttributes = 0;
            HandleInformation->GrantedAccess = PROCESS_ALL_ACCESS;
        }

        /* Return the pointer */
        *Object = CurrentProcess;
        return STATUS_SUCCESS;
    }
    else if (Handle == NtCurrentProcess())
    {
        /* The caller used this special handle value with a non-process type */
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Check if the caller wants the current thread */
    if ((Handle == NtCurrentThread()) &&
        ((ObjectType == PsThreadType) || !(ObjectType)))
    {
        /* Get the current thread */
        CurrentThread = PsGetCurrentThread();

        /* Reference ourselves */
        ObReferenceObject(CurrentThread);

        /* Check if the caller wanted handle information */
        if (HandleInformation)
        {
            /* Return it */
            HandleInformation->HandleAttributes = 0;
            HandleInformation->GrantedAccess = THREAD_ALL_ACCESS;
        }

        /* Return the pointer */
        *Object = CurrentThread;
        return STATUS_SUCCESS;
    }
    else if (Handle == NtCurrentThread())
    {
        /* The caller used this special handle value with a non-thread type */
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Check if this is a kernel handle */
    if (ObIsKernelHandle(Handle, AccessMode))
    {
        /* Use the kernel handle table and get the actual handle value */
        Handle = ObKernelHandleToHandle(Handle);
        HandleTable = ObpKernelHandleTable;
    }
    else
    {
        /* Otherwise use this process's handle table */
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }

    /* Enter a critical region while we touch the handle table */
    KeEnterCriticalRegion();

    /* Get the handle entry */
    HandleEntry = ExMapHandleToPointer(HandleTable, Handle);
    if (HandleEntry)
    {
        /* Get the object header and validate the type*/
        ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
        if (!(ObjectType) || (ObjectType == ObjectHeader->Type))
        {
            /* Get the granted access and validate it */
            GrantedAccess = HandleEntry->GrantedAccess;
            if ((AccessMode == KernelMode) ||
                !(~GrantedAccess & DesiredAccess))
            {
                /* Reference the object directly since we have its header */
                InterlockedIncrement(&ObjectHeader->PointerCount);

                /* Mask out the internal attributes */
                Attributes = HandleEntry->ObAttributes &
                             (EX_HANDLE_ENTRY_PROTECTFROMCLOSE |
                              EX_HANDLE_ENTRY_INHERITABLE |
                              EX_HANDLE_ENTRY_AUDITONCLOSE);

                /* Check if the caller wants handle information */
                if (HandleInformation)
                {
                    /* Fill out the information */
                    HandleInformation->HandleAttributes = Attributes;
                    HandleInformation->GrantedAccess = GrantedAccess;
                }

                /* Return the pointer */
                *Object = &ObjectHeader->Body;

                /* Unlock the handle */
                ExUnlockHandleTableEntry(HandleTable, HandleEntry);

                /* Return success */
                KeLeaveCriticalRegion();
                return STATUS_SUCCESS;
            }
            else
            {
                /* Requested access failed */
                Status = STATUS_ACCESS_DENIED;
            }
        }
        else
        {
            /* Invalid object type */
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }

        /* Unlock the entry */
        ExUnlockHandleTableEntry(HandleTable, HandleEntry);
    }
    else
    {
        /* Invalid handle */
        Status = STATUS_INVALID_HANDLE;
    }

    /* Return failure status */
    KeLeaveCriticalRegion();
    *Object = NULL;
    return Status;
}

/* EOF */
