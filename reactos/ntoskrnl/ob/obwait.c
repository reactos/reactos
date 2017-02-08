/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/obwait.c
 * PURPOSE:         Handles Waiting on Objects
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*++
* @name NtWaitForMultipleObjects
* @implemented NT4
*
*     The NtWaitForMultipleObjects routine <FILLMEIN>
*
* @param ObjectCount
*        <FILLMEIN>
*
* @param HandleArray
*        <FILLMEIN>
*
* @param WaitType
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtWaitForMultipleObjects(IN ULONG ObjectCount,
                         IN PHANDLE HandleArray,
                         IN WAIT_TYPE WaitType,
                         IN BOOLEAN Alertable,
                         IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    PKWAIT_BLOCK WaitBlockArray;
    HANDLE Handles[MAXIMUM_WAIT_OBJECTS], KernelHandle;
    PVOID Objects[MAXIMUM_WAIT_OBJECTS];
    PVOID WaitObjects[MAXIMUM_WAIT_OBJECTS];
    ULONG i, ReferencedObjects, j;
    KPROCESSOR_MODE PreviousMode;
    LARGE_INTEGER SafeTimeOut;
    BOOLEAN LockInUse;
    PHANDLE_TABLE_ENTRY HandleEntry;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE HandleTable;
    ACCESS_MASK GrantedAccess;
    PVOID DefaultObject;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check for valid Object Count */
    if ((ObjectCount > MAXIMUM_WAIT_OBJECTS) || !(ObjectCount))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Check for valid Wait Type */
    if ((WaitType != WaitAll) && (WaitType != WaitAny))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Enter SEH */
    PreviousMode = ExGetPreviousMode();
    _SEH2_TRY
    {
        /* Probe for user mode */
        if (PreviousMode != KernelMode)
        {
            /* Check if we have a timeout */
            if (TimeOut)
            {
                /* Make a local copy of the timeout on the stack */
                SafeTimeOut = ProbeForReadLargeInteger(TimeOut);
                TimeOut = &SafeTimeOut;
            }

             /* Probe all the handles */
            ProbeForRead(HandleArray,
                         ObjectCount * sizeof(HANDLE),
                         sizeof(HANDLE));
        }

        /*
         * Make a copy so we don't have to guard with SEH later and keep
         * track of what objects we referenced if dereferencing pointers
         * suddenly fails
         */
        RtlCopyMemory(Handles,
                      HandleArray,
                      ObjectCount * sizeof(HANDLE));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) //ExSystemExceptionFilter()
    {
        /* Cover up for kernel mode */
        if (PreviousMode == KernelMode)
        {
            /* But don't fail silently */
            DbgPrint("Mon dieu! Covering up for BAD driver passing invalid pointer (0x%p)! Hon hon hon!\n", HandleArray);
        }

        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check if we can use the internal Wait Array */
    if (ObjectCount > THREAD_WAIT_OBJECTS)
    {
        /* Allocate from Pool */
        WaitBlockArray = ExAllocatePoolWithTag(NonPagedPool,
                                               ObjectCount *
                                               sizeof(KWAIT_BLOCK),
                                               TAG_WAIT);
        if (!WaitBlockArray)
        {
            /* Fail */
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        /* No need for the array  */
        WaitBlockArray = NULL;   
    }

    /* Enter a critical region since we'll play with handles */
    LockInUse = TRUE;
    KeEnterCriticalRegion();

    /* Start the loop */
    i = 0;
    ReferencedObjects = 0;
    do
    {
        /* Use the right Executive Handle */
        if (ObpIsKernelHandle(Handles[i], PreviousMode))
        {
            /* Use the System Handle Table and decode */
            HandleTable = ObpKernelHandleTable;
            KernelHandle = ObKernelHandleToHandle(Handles[i]);

            /* Get a pointer to it */
            HandleEntry = ExMapHandleToPointer(HandleTable, KernelHandle);
        }
        else
        {
            /* Use the Process' Handle table and get the Ex Handle */
            HandleTable = PsGetCurrentProcess()->ObjectTable;

            /* Get a pointer to it */
            HandleEntry = ExMapHandleToPointer(HandleTable, Handles[i]);
        }

        /* Check if we have an entry */
        if (!HandleEntry)
        {
            /* Fail, handle is invalid */
            Status = STATUS_INVALID_HANDLE;
            DPRINT1("Invalid handle %p passed to NtWaitForMultipleObjects\n", Handles[i]);
            goto Quickie;
        }

        /* Check for synchronize access */
        GrantedAccess = HandleEntry->GrantedAccess;
        if ((PreviousMode != KernelMode) && (!(GrantedAccess & SYNCHRONIZE)))
        {
            /* Unlock the entry and fail */
            ExUnlockHandleTableEntry(HandleTable, HandleEntry);
            DPRINT1("Handle does not have SYNCHRONIZE access\n");
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* Get the Object Header */
        ObjectHeader = ObpGetHandleObject(HandleEntry);

        /* Get default Object */
        DefaultObject = ObjectHeader->Type->DefaultObject;

        /* Check if it's the internal offset */
        if (IsPointerOffset(DefaultObject))
        {
            /* Increase reference count */
            InterlockedIncrement(&ObjectHeader->PointerCount);
            ReferencedObjects++;

            /* Save the Object and Wait Object, this is a relative offset */
            Objects[i] = &ObjectHeader->Body;
            WaitObjects[i] = (PVOID)((ULONG_PTR)&ObjectHeader->Body +
                                     (ULONG_PTR)DefaultObject);
        }
        else
        {
            /* This is our internal Object */
            ReferencedObjects++;
            Objects[i] = NULL;
            WaitObjects[i] = DefaultObject;
        }

        /* Unlock the Handle Table Entry */
        ExUnlockHandleTableEntry(HandleTable, HandleEntry);

        /* Keep looping */
        i++;
    } while (i < ObjectCount);

    /* For a Waitall, we can't have the same object more then once */
    if (WaitType == WaitAll)
    {
        /* Clear the main loop variable */
        i = 0;

        /* Start the loop */
        do
        {
            /* Check the current and forward object */
            for (j = i + 1; j < ObjectCount; j++)
            {
                /* Make sure they don't match */
                if (WaitObjects[i] == WaitObjects[j])
                {
                    /* Fail */
                    Status = STATUS_INVALID_PARAMETER_MIX;
                    DPRINT1("Passed a duplicate object to NtWaitForMultipleObjects\n");
                    goto Quickie;
                }
            }

            /* Keep looping */
            i++;
        } while (i < ObjectCount);
    }

    /* Now we can finally wait. Always use SEH since it can raise an exception */
    _SEH2_TRY
    {
        /* We're done playing with handles */
        LockInUse = FALSE;
        KeLeaveCriticalRegion();

        /* Do the kernel wait */
        Status = KeWaitForMultipleObjects(ObjectCount,
                                          WaitObjects,
                                          WaitType,
                                          UserRequest,
                                          PreviousMode,
                                          Alertable,
                                          TimeOut,
                                          WaitBlockArray);
    }
    _SEH2_EXCEPT((_SEH2_GetExceptionCode() == STATUS_MUTANT_LIMIT_EXCEEDED) ?
                 EXCEPTION_EXECUTE_HANDLER :
                 EXCEPTION_CONTINUE_SEARCH)
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

Quickie:
    /* First derefence */
    while (ReferencedObjects)
    {
        /* Decrease the number of objects */
        ReferencedObjects--;

        /* Check if we had a valid object in this position */
        if (Objects[ReferencedObjects])
        {
            /* Dereference it */
            ObDereferenceObject(Objects[ReferencedObjects]);
        }
    }

    /* Free wait block array */
    if (WaitBlockArray) ExFreePoolWithTag(WaitBlockArray, TAG_WAIT);

    /* Re-enable APCs if needed */
    if (LockInUse) KeLeaveCriticalRegion();

    /* Return status */
    return Status;
}

/*++
* @name NtWaitForMultipleObjects32
* @implemented NT5.1
*
*     The NtWaitForMultipleObjects32 routine <FILLMEIN>
*
* @param ObjectCount
*        <FILLMEIN>
*
* @param HandleArray
*        <FILLMEIN>
*
* @param WaitType
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtWaitForMultipleObjects32(IN ULONG ObjectCount,
                           IN PLONG Handles,
                           IN WAIT_TYPE WaitType,
                           IN BOOLEAN Alertable,
                           IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    /* FIXME WOW64 */
    return NtWaitForMultipleObjects(ObjectCount,
                                    (PHANDLE)Handles,
                                    WaitType,
                                    Alertable,
                                    TimeOut);
}

/*++
* @name NtWaitForSingleObject
* @implemented NT4
*
*     The NtWaitForSingleObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtWaitForSingleObject(IN HANDLE ObjectHandle,
                      IN BOOLEAN Alertable,
                      IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
    PVOID Object, WaitableObject;
    KPROCESSOR_MODE PreviousMode;
    LARGE_INTEGER SafeTimeOut;
    NTSTATUS Status;

    /* Check if we came with a timeout from user mode */
    PreviousMode = ExGetPreviousMode();
    if ((TimeOut) && (PreviousMode != KernelMode))
    {
        /* Enter SEH for proving */
        _SEH2_TRY
        {
            /* Make a copy on the stack */
            SafeTimeOut = ProbeForReadLargeInteger(TimeOut);
            TimeOut = &SafeTimeOut;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Get the Object */
    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       SYNCHRONIZE,
                                       NULL,
                                       PreviousMode,
                                       &Object,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Get the Waitable Object */
        WaitableObject = OBJECT_TO_OBJECT_HEADER(Object)->Type->DefaultObject;

        /* Is it an offset for internal objects? */
        if (IsPointerOffset(WaitableObject))
        {
            /* Turn it into a pointer */
            WaitableObject = (PVOID)((ULONG_PTR)Object +
                                     (ULONG_PTR)WaitableObject);
        }

        /* SEH this since it can also raise an exception */
        _SEH2_TRY
        {
            /* Ask the kernel to do the wait */
            Status = KeWaitForSingleObject(WaitableObject,
                                           UserRequest,
                                           PreviousMode,
                                           Alertable,
                                           TimeOut);
        }
        _SEH2_EXCEPT((_SEH2_GetExceptionCode() == STATUS_MUTANT_LIMIT_EXCEEDED) ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Dereference the Object */
        ObDereferenceObject(Object);
    }
    else
    {
        DPRINT1("Failed to reference the handle with status 0x%x\n", Status);
    }

    /* Return the status */
    return Status;
}

/*++
* @name NtSignalAndWaitForSingleObject
* @implemented NT4
*
*     The NtSignalAndWaitForSingleObject routine <FILLMEIN>
*
* @param ObjectHandleToSignal
*        <FILLMEIN>
*
* @param WaitableObjectHandle
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtSignalAndWaitForSingleObject(IN HANDLE ObjectHandleToSignal,
                               IN HANDLE WaitableObjectHandle,
                               IN BOOLEAN Alertable,
                               IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode;
    POBJECT_TYPE Type;
    PVOID SignalObj, WaitObj, WaitableObject;
    LARGE_INTEGER SafeTimeOut;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    NTSTATUS Status;

    /* Check if we came with a timeout from user mode */
    PreviousMode = ExGetPreviousMode();
    if ((TimeOut) && (PreviousMode != KernelMode))
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Make a copy on the stack */
            SafeTimeOut = ProbeForReadLargeInteger(TimeOut);
            TimeOut = &SafeTimeOut;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Start by getting the signal object*/
    Status = ObReferenceObjectByHandle(ObjectHandleToSignal,
                                       0,
                                       NULL,
                                       PreviousMode,
                                       &SignalObj,
                                       &HandleInfo);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now get the wait object */
    Status = ObReferenceObjectByHandle(WaitableObjectHandle,
                                       SYNCHRONIZE,
                                       NULL,
                                       PreviousMode,
                                       &WaitObj,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to reference the wait object */
        ObDereferenceObject(SignalObj);
        return Status;
    }

    /* Get the real waitable object */
    WaitableObject = OBJECT_TO_OBJECT_HEADER(WaitObj)->Type->DefaultObject;

    /* Handle internal offset */
    if (IsPointerOffset(WaitableObject))
    {
        /* Get real pointer */
        WaitableObject = (PVOID)((ULONG_PTR)WaitObj +
                                 (ULONG_PTR)WaitableObject);
    }

    /* Check Signal Object Type */
    Type = OBJECT_TO_OBJECT_HEADER(SignalObj)->Type;
    if (Type == ExEventObjectType)
    {
        /* Check if we came from user-mode without the right access */
        if ((PreviousMode != KernelMode) &&
            !(HandleInfo.GrantedAccess & EVENT_MODIFY_STATE))
        {
            /* Fail: lack of rights */
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* Set the Event */
        KeSetEvent(SignalObj, EVENT_INCREMENT, TRUE);
    }
    else if (Type == ExMutantObjectType)
    {
        /* This can raise an exception */
        _SEH2_TRY
        {
            /* Release the mutant */
            KeReleaseMutant(SignalObj, MUTANT_INCREMENT, FALSE, TRUE);
        }
        _SEH2_EXCEPT(((_SEH2_GetExceptionCode() == STATUS_ABANDONED) ||
                      (_SEH2_GetExceptionCode() == STATUS_MUTANT_NOT_OWNED)) ?
                      EXCEPTION_EXECUTE_HANDLER :
                      EXCEPTION_CONTINUE_SEARCH)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else if (Type == ExSemaphoreObjectType)
    {
        /* Check if we came from user-mode without the right access */
        if ((PreviousMode != KernelMode) &&
            !(HandleInfo.GrantedAccess & SEMAPHORE_MODIFY_STATE))
        {
            /* Fail: lack of rights */
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* This can raise an exception*/
        _SEH2_TRY
        {
            /* Release the semaphore */
            KeReleaseSemaphore(SignalObj, SEMAPHORE_INCREMENT, 1, TRUE);
        }
        _SEH2_EXCEPT((_SEH2_GetExceptionCode() == STATUS_SEMAPHORE_LIMIT_EXCEEDED) ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        /* This isn't a valid object to be waiting on */
        Status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Make sure we didn't fail */
    if (NT_SUCCESS(Status))
    {
        /* SEH this since it can also raise an exception */
        _SEH2_TRY
        {
            /* Perform the wait now */
            Status = KeWaitForSingleObject(WaitableObject,
                                           UserRequest,
                                           PreviousMode,
                                           Alertable,
                                           TimeOut);
        }
        _SEH2_EXCEPT((_SEH2_GetExceptionCode() == STATUS_MUTANT_LIMIT_EXCEEDED) ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* We're done here, dereference both objects */
Quickie:
    ObDereferenceObject(SignalObj);
    ObDereferenceObject(WaitObj);
    return Status;
}

/* EOF */
