/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               ntoskrnl/ob/wait.c
 * PURPOSE:            Handles Waiting on Objects
 *
 * PROGRAMMERS:        Alex Ionescu (alex@relsoft.net)
 *                     David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define TAG_WAIT TAG('W', 'a', 'i', 't')

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtWaitForMultipleObjects(IN ULONG ObjectCount,
                         IN PHANDLE HandleArray,
                         IN WAIT_TYPE WaitType,
                         IN BOOLEAN Alertable,
                         IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
    PKWAIT_BLOCK WaitBlockArray = NULL;
    HANDLE Handles[MAXIMUM_WAIT_OBJECTS];
    PVOID Objects[MAXIMUM_WAIT_OBJECTS];
    PVOID WaitObjects[MAXIMUM_WAIT_OBJECTS];
    ULONG i = 0, ReferencedObjects = 0, j;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeTimeOut;
    BOOLEAN LockInUse;
    PHANDLE_TABLE_ENTRY HandleEntry;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE HandleTable;
    ACCESS_MASK GrantedAccess;
    PVOID DefaultObject;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("NtWaitForMultipleObjects(ObjectCount %lu HandleArray[] %x, Alertable %d, "
            "TimeOut %x)\n", ObjectCount, HandleArray, Alertable, TimeOut);

    /* Enter a critical region since we'll play with handles */
    LockInUse = TRUE;
    KeEnterCriticalRegion();

    /* Check for valid Object Count */
    if ((ObjectCount > MAXIMUM_WAIT_OBJECTS) || !ObjectCount)
    {
        Status = STATUS_INVALID_PARAMETER_1;
        DPRINT1("No object count, or too many objects\n");
        goto Quickie;
    }

    /* Check for valid Wait Type */
    if ((WaitType != WaitAll) && (WaitType != WaitAny))
    {
        Status = STATUS_INVALID_PARAMETER_3;
        DPRINT1("Invalid wait type\n");
        goto Quickie;
    }

    /* Capture arguments */
    _SEH_TRY
    {
        if(PreviousMode != KernelMode)
        {
            ProbeForRead(HandleArray,
                         ObjectCount * sizeof(HANDLE),
                         sizeof(HANDLE));
            
            if(TimeOut)
            {
                ProbeForRead(TimeOut,
                             sizeof(LARGE_INTEGER),
                             sizeof(ULONG));

                /* Make a local copy of the timeout on the stack */
                SafeTimeOut = *TimeOut;
                TimeOut = &SafeTimeOut;
            }
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
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status)) goto Quickie;

    /* Check if we can use the internal Wait Array */
    if (ObjectCount > THREAD_WAIT_OBJECTS)
    {
        /* Allocate from Pool */
        WaitBlockArray = ExAllocatePoolWithTag(NonPagedPool,
                                               ObjectCount * sizeof(KWAIT_BLOCK),
                                               TAG_WAIT);
    }

    /* Start the loop */
    do
    {
        /* Use the right Executive Handle */
        if(ObIsKernelHandle(Handles[i], PreviousMode))
        {
            /* Use the System Handle Table and decode */
            HandleTable = ObpKernelHandleTable;
            Handles[i] = ObKernelHandleToHandle(Handles[i]);
        }
        else
        {
            /* Use the Process' Handle table and get the Ex Handle */
            HandleTable = PsGetCurrentProcess()->ObjectTable;
        }

        /* Get a pointer to it */
        if (!(HandleEntry = ExMapHandleToPointer(HandleTable, Handles[i])))
        {
            DPRINT1("Invalid handle\n");
            Status = STATUS_INVALID_HANDLE;
            goto Quickie;
        }

        /* Check for synchronize access */
        GrantedAccess = HandleEntry->u2.GrantedAccess;
        if ((PreviousMode != KernelMode) && (!(GrantedAccess & SYNCHRONIZE)))
        {
            /* Unlock the entry and fail */
            ExUnlockHandleTableEntry(HandleTable, HandleEntry);
            Status = STATUS_ACCESS_DENIED;
            DPRINT1("Handle doesn't have SYNCH access\n");
            goto Quickie;
        }

        /* Get the Object Header */
        ObjectHeader = EX_HTE_TO_HDR(HandleEntry);

        /* Get default Object */
        DefaultObject = ObjectHeader->Type->DefaultObject;

        /* Check if it's the internal offset */
        if ((LONG_PTR)DefaultObject >= 0)
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
                    DPRINT1("Objects duplicated with WaitAll\n");
                    goto Quickie;
                }
            }

            /* Keep looping */
            i++;
        } while (i < ObjectCount);
    }

    /* Now we can finally wait. Use SEH since it can raise an exception */
    _SEH_TRY
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
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

Quickie:
    /* First derefence */
    while (ReferencedObjects)
    {
        ReferencedObjects--;
        if (Objects[ReferencedObjects])
        {   
            ObDereferenceObject(Objects[ReferencedObjects]);
        }
    }

    /* Free wait block array */
    if (WaitBlockArray) ExFreePool(WaitBlockArray);

    /* Re-enable APCs if needed */
    if (LockInUse) KeLeaveCriticalRegion();

    /* Return status */
    DPRINT1("Returning: %x\n", Status);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtWaitForSingleObject(IN HANDLE ObjectHandle,
                      IN BOOLEAN Alertable,
                      IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
    PVOID Object, WaitableObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeTimeOut;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("NtWaitForSingleObject(ObjectHandle %x, Alertable %d, TimeOut %x)\n",
            ObjectHandle,Alertable,TimeOut);

    /* Capture timeout */
    if(TimeOut && PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForRead(TimeOut,
                         sizeof(LARGE_INTEGER),
                         sizeof(ULONG));
            /* Make a copy on the stack */
            SafeTimeOut = *TimeOut;
            TimeOut = &SafeTimeOut;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
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
        WaitableObject = BODY_TO_HEADER(Object)->Type->DefaultObject;

        /* Is it an offset for internal objects? */
        if ((LONG_PTR)WaitableObject >= 0)
        {
            /* Turn it into a pointer */
            WaitableObject = (PVOID)((ULONG_PTR)Object +
                                     (ULONG_PTR)WaitableObject);
        }

        /* Now wait. Also SEH this since it can also raise an exception */
        _SEH_TRY
        {
            Status = KeWaitForSingleObject(WaitableObject,
                                           UserRequest,
                                           PreviousMode,
                                           Alertable,
                                           TimeOut);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Dereference the Object */
        ObDereferenceObject(Object);
    }

    /* Return the status */
    return Status;
}

NTSTATUS
STDCALL
NtSignalAndWaitForSingleObject(IN HANDLE ObjectHandleToSignal,
                               IN HANDLE WaitableObjectHandle,
                               IN BOOLEAN Alertable,
                               IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    POBJECT_TYPE Type;
    PVOID SignalObj;
    PVOID WaitObj;
    PVOID WaitableObject;
    LARGE_INTEGER SafeTimeOut;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Capture timeout */
    DPRINT("NtSignalAndWaitForSingleObject\n");
    if(TimeOut && PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForRead(TimeOut,
                         sizeof(LARGE_INTEGER),
                         sizeof(ULONG));
            /* Make a copy on the stack */
            SafeTimeOut = *TimeOut;
            TimeOut = &SafeTimeOut;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Start by getting the signal object*/
    Status = ObReferenceObjectByHandle(ObjectHandleToSignal,
                                       0,
                                       NULL,
                                       PreviousMode,
                                       &SignalObj,
                                       &HandleInfo);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Now get the wait object */
    Status = ObReferenceObjectByHandle(WaitableObjectHandle,
                                       SYNCHRONIZE,
                                       NULL,
                                       PreviousMode,
                                       &WaitObj,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(SignalObj);
        return Status;
    }

    /* Get the real waitable object */
    WaitableObject = BODY_TO_HEADER(WaitObj)->Type->DefaultObject;

    /* Handle internal offset */
    if ((LONG_PTR)WaitableObject >= 0)
    {
        /* Get real pointer */
        WaitableObject = (PVOID)((ULONG_PTR)WaitObj +
                                 (ULONG_PTR)WaitableObject);
    }
    
    /* Check Signal Object Type */
    Type = BODY_TO_HEADER(WaitObj)->Type;
    if (Type == ExEventObjectType)
    {
        /* Set the Event */
        /* FIXME: Check permissions */
        KeSetEvent(SignalObj, EVENT_INCREMENT, TRUE);
    }
    else if (Type == ExMutantObjectType)
    {
        /* Release the Mutant. This can raise an exception*/
        _SEH_TRY
        {
            KeReleaseMutant(SignalObj, MUTANT_INCREMENT, FALSE, TRUE);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
            goto Quickie;
        }
        _SEH_END;
    }
    else if (Type == ExSemaphoreObjectType)
    {
        /* Release the Semaphore. This can raise an exception*/
        /* FIXME: Check permissions */
        _SEH_TRY
        {
            KeReleaseSemaphore(SignalObj, SEMAPHORE_INCREMENT, 1, TRUE);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
            goto Quickie;
        }
        _SEH_END;
    }
    else
    {
        Status = STATUS_OBJECT_TYPE_MISMATCH;
        DPRINT1("Waiting on invalid object type\n");
        goto Quickie;
    }

    /* Now wait. Also SEH this since it can also raise an exception */
    _SEH_TRY
    {
        Status = KeWaitForSingleObject(WaitableObject,
                                       UserRequest,
                                       PreviousMode,
                                       Alertable,
                                       TimeOut);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* We're done here */
Quickie:
    ObDereferenceObject(SignalObj);
    ObDereferenceObject(WaitObj);
    return Status;
}

/* EOF */
