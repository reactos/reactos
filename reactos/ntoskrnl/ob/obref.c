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
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
FASTCALL
ObReferenceObjectSafe(IN PVOID Object)
{
    POBJECT_HEADER ObjectHeader;
    LONG OldValue, NewValue;

    /* Get the object header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    /* Get the current reference count and fail if it's zero */
    OldValue = ObjectHeader->PointerCount;
    if (!OldValue) return FALSE;

    /* Start reference loop */
    do
    {
        /* Increase the reference count */
        NewValue = InterlockedCompareExchange(&ObjectHeader->PointerCount,
                                              OldValue + 1,
                                              OldValue);
        if (OldValue == NewValue) return TRUE;

        /* Keep looping */
        OldValue = NewValue;
    } while (OldValue);

    /* If we got here, then the reference count is now 0 */
    return FALSE;
}

VOID
NTAPI
ObpDeferObjectDeletion(IN POBJECT_HEADER Header)
{
    PVOID Entry;

    /* Loop while trying to update the list */
    do
    {
        /* Get the current entry */
        Entry = ObpReaperList;

        /* Link our object to the list */
        Header->NextToFree = Entry;

        /* Update the list */
    } while (InterlockedCompareExchangePointer(&ObpReaperList,
                                               Header,
                                               Entry) != Entry);

    /* Queue the work item if needed */
    if (!Entry) ExQueueWorkItem(&ObpReaperWorkItem, CriticalWorkQueue);
}

LONG
FASTCALL
ObReferenceObjectEx(IN PVOID Object,
                    IN LONG Count)
{
    /* Increment the reference count and return the count now */
    return InterlockedExchangeAdd(&OBJECT_TO_OBJECT_HEADER(Object)->
                                  PointerCount,
                                  Count);
}

LONG
FASTCALL
ObDereferenceObjectEx(IN PVOID Object,
                      IN LONG Count)
{
    POBJECT_HEADER Header;
    LONG NewCount;

    /* Extract the object header */
    Header = OBJECT_TO_OBJECT_HEADER(Object);

    /* Check whether the object can now be deleted. */
    NewCount = InterlockedExchangeAdd(&Header->PointerCount, -Count);
    if (!Count) ObpDeferObjectDeletion(Header);

    /* Return the current count */
    return NewCount;
}

VOID
FASTCALL
ObInitializeFastReference(IN PEX_FAST_REF FastRef,
                          IN PVOID Object OPTIONAL)
{
    /* Check if we were given an object and reference it 7 times */
    if (Object) ObReferenceObjectEx(Object, MAX_FAST_REFS);

    /* Sanity check */
    ASSERT(!(((ULONG_PTR)Object) & MAX_FAST_REFS));

    /* Check if the caller gave us an object */
    if (Object)
    {
        /* He did, so write the biased pointer */
        FastRef->Object = (PVOID)((ULONG_PTR)Object | MAX_FAST_REFS);
    }
    else
    {
        /* Otherwise, clear the current object */
        FastRef->Object = NULL;
    }
}

PVOID
FASTCALL
ObFastReferenceObjectLocked(IN PEX_FAST_REF FastRef)
{
    PVOID Object;

    /* Get the object and reference it slowly */
    Object = (PVOID)((ULONG_PTR)FastRef->Object & MAX_FAST_REFS);
    if (Object) ObReferenceObject(Object);
    return Object;
}

PVOID
FASTCALL
ObFastReferenceObject(IN PEX_FAST_REF FastRef)
{
    ULONG_PTR Value, NewValue;
    ULONG_PTR Count;
    PVOID Object;

    /* Start reference loop */
    for (;;)
    {
        /* Get the current count */
        Value = FastRef->Value;
        if (!(Value & MAX_FAST_REFS)) break;

        /* Increase the reference count */
        NewValue = Value - 1;
        if (ExpChangeRundown(FastRef, NewValue, Value) == Value) break;
    }

    /* Get the object and count */
    Object = (PVOID)(Value &~ MAX_FAST_REFS);
    Count = Value & MAX_FAST_REFS;

    /* Check if the reference count is over 1 */
    if (Count > 1) return Object;

    /* Check if the reference count has reached 0 */
    if (!Count) return NULL;

    /* Otherwise, reference the object 7 times */
    ObReferenceObjectEx(Object, MAX_FAST_REFS);
    ASSERT(!(((ULONG_PTR)Object) & MAX_FAST_REFS));

    for (;;)
    {
        /* Check if the current count is too high */
        Value = FastRef->Value;
        if (((FastRef->RefCnt + MAX_FAST_REFS) > MAX_FAST_REFS) ||
            ((PVOID)((ULONG_PTR)FastRef->Object &~ MAX_FAST_REFS) != Object))
        {
            /* Completely dereference the object */
            ObDereferenceObjectEx(Object, MAX_FAST_REFS);
            break;
        }
        else
        {
            /* Increase the reference count */
            NewValue = Value + MAX_FAST_REFS;
            if (ExpChangeRundown(FastRef, NewValue, Value) == Value) break;
        }
    }

    /* Return the Object */
    return Object;
}

VOID
FASTCALL
ObFastDereferenceObject(IN PEX_FAST_REF FastRef,
                        IN PVOID Object)
{
    ULONG_PTR Value, NewValue;

    /* Sanity checks */
    ASSERT(Object);
    ASSERT(!(((ULONG_PTR)Object) & MAX_FAST_REFS));

    /* Start dereference loop */
    for (;;)
    {
        /* Get the current count */
        Value = FastRef->Value;
        if ((Value ^ (ULONG_PTR)Object) < MAX_FAST_REFS)
        {
            /* Decrease the reference count */
            NewValue = Value + 1;
            if (ExpChangeRundown(FastRef, NewValue, Value) == Value) return;
        }
        else
        {
            /* Do a normal Dereference */
            ObDereferenceObject(Object);
            return;
        }
    }
}

PVOID
FASTCALL
ObFastReplaceObject(IN PEX_FAST_REF FastRef,
                    PVOID Object)
{
    ULONG_PTR NewValue;
    EX_FAST_REF OldRef;
    PVOID OldObject;

    /* Check if we were given an object and reference it 7 times */
    if (Object) ObReferenceObjectEx(Object, MAX_FAST_REFS);

    /* Sanity check */
    ASSERT(!(((ULONG_PTR)Object) & MAX_FAST_REFS));

    /* Check if the caller gave us an object */
    if (Object)
    {
        /* He did, so bias the pointer */
        NewValue = (ULONG_PTR)Object | MAX_FAST_REFS;
    }
    else
    {
        /* No object, we're clearing */
        NewValue = 0;
    }

    /* Switch objects */
    OldRef.Value = InterlockedExchange((PLONG)&FastRef->Value, NewValue);
    OldObject = (PVOID)((ULONG_PTR)OldRef.Object &~ MAX_FAST_REFS);
    if ((OldObject) && (OldRef.RefCnt))
    {
        /* Dereference the old object */
        ObDereferenceObjectEx(OldObject, OldRef.RefCnt);
    }

    /* Return the old object */
    return OldObject;
}

/* PUBLIC FUNCTIONS *********************************************************/

LONG_PTR
FASTCALL
ObfReferenceObject(IN PVOID Object)
{
    ASSERT(Object);

    /* Get the header and increment the reference count */
    return InterlockedIncrement(&OBJECT_TO_OBJECT_HEADER(Object)->PointerCount);
}

LONG_PTR
FASTCALL
ObfDereferenceObject(IN PVOID Object)
{
    POBJECT_HEADER Header;
    LONG_PTR OldCount;

    /* Extract the object header */
    Header = OBJECT_TO_OBJECT_HEADER(Object);

    if (Header->PointerCount < Header->HandleCount)
    {
        DPRINT1("Misbehaving object: %wZ\n", &Header->Type->Name);
        return Header->PointerCount;
    }

    /* Check whether the object can now be deleted. */
    OldCount = InterlockedDecrement(&Header->PointerCount);
    if (!OldCount)
    {
        /* Sanity check */
        if (Header->HandleCount)
        {
            DPRINT1("Misbehaving object: %wZ\n", &Header->Type->Name);
            return Header->PointerCount;
        }

        /* Check if APCs are still active */
        if (!KeAreAllApcsDisabled())
        {
            /* Remove the object */
            ObpDeleteObject(Object, FALSE);
        }
        else
        {
            /* Add us to the deferred deletion list */
            ObpDeferObjectDeletion(Header);
        }
    }

    /* Return the old count */
    return OldCount;
}

VOID
NTAPI
ObDereferenceObjectDeferDelete(IN PVOID Object)
{
    POBJECT_HEADER Header = OBJECT_TO_OBJECT_HEADER(Object);

    /* Check whether the object can now be deleted. */
    if (!InterlockedDecrement(&Header->PointerCount))
    {
        /* Add us to the deferred deletion list */
        ObpDeferObjectDeletion(Header);
    }
}

#undef ObDereferenceObject
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

    /* Increment the reference count and return success */
    InterlockedIncrement(&Header->PointerCount);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObReferenceObjectByName(IN PUNICODE_STRING ObjectPath,
                        IN ULONG Attributes,
                        IN PACCESS_STATE PassedAccessState,
                        IN ACCESS_MASK DesiredAccess,
                        IN POBJECT_TYPE ObjectType,
                        IN KPROCESSOR_MODE AccessMode,
                        IN OUT PVOID ParseContext,
                        OUT PVOID* ObjectPtr)
{
    PVOID Object = NULL;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    OBP_LOOKUP_CONTEXT Context;
    AUX_DATA AuxData;
    ACCESS_STATE AccessState;
    PAGED_CODE();

    /* Fail quickly */
    if (!ObjectPath) return STATUS_OBJECT_NAME_INVALID;

    /* Capture the name */
    Status = ObpCaptureObjectName(&ObjectName, ObjectPath, AccessMode, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* We also need a valid name after capture */
    if (!ObjectName.Length) return STATUS_OBJECT_NAME_INVALID;

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
    Status = ObpLookupObjectName(NULL,
                                 &ObjectName,
                                 Attributes,
                                 ObjectType,
                                 AccessMode,
                                 ParseContext,
                                 NULL,
                                 NULL,
                                 PassedAccessState,
                                 &Context,
                                 &Object);

    /* Cleanup after lookup */
    ObpCleanupDirectoryLookup(&Context);

    /* Check if the lookup succeeded */
    if (NT_SUCCESS(Status))
    {
        /* Check if access is allowed */
        if (ObpCheckObjectReference(Object,
                                    PassedAccessState,
                                    FALSE,
                                    AccessMode,
                                    &Status))
        {
            /* Return the object */
            *ObjectPtr = Object;
        }
    }

    /* Free the access state */
    if (PassedAccessState == &AccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

Quickie:
    /* Free the captured name if we had one, and return status */
    ObpFreeObjectNameBuffer(&ObjectName);
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

    /* Assume failure */
    *Object = NULL;

    /* Check if this is a special handle */
    if (HandleToLong(Handle) < 0)
    {
        /* Check if this is the current process */
        if (Handle == NtCurrentProcess())
        {
            /* Check if this is the right object type */
            if ((ObjectType == PsProcessType) || !(ObjectType))
            {
                /* Get the current process and granted access */
                CurrentProcess = PsGetCurrentProcess();
                GrantedAccess = CurrentProcess->GrantedAccess;
                
                /* Validate access */
                if ((AccessMode == KernelMode) ||
                    !(~GrantedAccess & DesiredAccess))
                {                   
                    /* Check if the caller wanted handle information */
                    if (HandleInformation)
                    {
                        /* Return it */
                        HandleInformation->HandleAttributes = 0;
                        HandleInformation->GrantedAccess = GrantedAccess;
                    }
                    
                    /* Reference ourselves */
                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(CurrentProcess);
                    InterlockedExchangeAdd(&ObjectHeader->PointerCount, 1);
                    
                    /* Return the pointer */
                    *Object = CurrentProcess;
                    ASSERT(*Object != NULL);
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    /* Access denied */
                    Status = STATUS_ACCESS_DENIED;
                }
            }
            else
            {
                /* The caller used this special handle value with a non-process type */
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            
            /* Return the status */
            return Status;
        }
        else if (Handle == NtCurrentThread())
        {
            /* Check if this is the right object type */
            if ((ObjectType == PsThreadType) || !(ObjectType))
            {
                /* Get the current process and granted access */
                CurrentThread = PsGetCurrentThread();
                GrantedAccess = CurrentThread->GrantedAccess;
                
                /* Validate access */
                if ((AccessMode == KernelMode) ||
                    !(~GrantedAccess & DesiredAccess))
                {                   
                    /* Check if the caller wanted handle information */
                    if (HandleInformation)
                    {
                        /* Return it */
                        HandleInformation->HandleAttributes = 0;
                        HandleInformation->GrantedAccess = GrantedAccess;
                    }
                    
                    /* Reference ourselves */
                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(CurrentThread);
                    InterlockedExchangeAdd(&ObjectHeader->PointerCount, 1);
                    
                    /* Return the pointer */
                    *Object = CurrentThread;
                    ASSERT(*Object != NULL);
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    /* Access denied */
                    Status = STATUS_ACCESS_DENIED;
                }
            }
            else
            {
                /* The caller used this special handle value with a non-process type */
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            
            /* Return the status */
            return Status;
        }
        else if (AccessMode == KernelMode)
        {
            /* Use the kernel handle table and get the actual handle value */
            Handle = ObKernelHandleToHandle(Handle);
            HandleTable = ObpKernelHandleTable;
        }
        else
        {
            /* Invalid access, fail */
            return STATUS_INVALID_HANDLE;
        }
    }
    else
    {
        /* Otherwise use this process's handle table */
        HandleTable = PsGetCurrentProcess()->ObjectTable;
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
        if (!(ObjectType) || (ObjectType == ObjectHeader->Type))
        {
            /* Get the granted access and validate it */
            GrantedAccess = HandleEntry->GrantedAccess;
            
            /* Validate access */
            if ((AccessMode == KernelMode) ||
                !(~GrantedAccess & DesiredAccess))
            {
                /* Reference the object directly since we have its header */
                InterlockedIncrement(&ObjectHeader->PointerCount);

                /* Mask out the internal attributes */
                Attributes = HandleEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES;

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
                KeLeaveCriticalRegion();

                /* Return success */
                ASSERT(*Object != NULL);
                return STATUS_SUCCESS;
            }
            else
            {
                /* Requested access failed */
                DPRINT("Rights not granted: %x\n", ~GrantedAccess & DesiredAccess);
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
