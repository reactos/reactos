/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               ntoskrnl/ob/wait.c
 * PURPOSE:            Handles Waiting on Objects
 *
 * PROGRAMMERS:        Alex Ionescu (alex@relsoft.net) - Created file
 *                     David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

BOOL inline FASTCALL KiIsObjectWaitable(PVOID Object);

NTSTATUS STDCALL
NtWaitForMultipleObjects(IN ULONG ObjectCount,
			 IN PHANDLE ObjectsArray,
			 IN WAIT_TYPE WaitType,
			 IN BOOLEAN Alertable,
			 IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
   KWAIT_BLOCK WaitBlockArray[MAXIMUM_WAIT_OBJECTS];
   HANDLE SafeObjectsArray[MAXIMUM_WAIT_OBJECTS];
   PVOID ObjectPtrArray[MAXIMUM_WAIT_OBJECTS];
   ULONG i, j;
   KPROCESSOR_MODE PreviousMode;
   LARGE_INTEGER SafeTimeOut;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtWaitForMultipleObjects(ObjectCount %lu ObjectsArray[] %x, Alertable %d, "
	  "TimeOut %x)\n", ObjectCount,ObjectsArray,Alertable,TimeOut);

   PreviousMode = ExGetPreviousMode();

   if (ObjectCount > MAXIMUM_WAIT_OBJECTS)
     return STATUS_UNSUCCESSFUL;
   if (0 == ObjectCount)
     return STATUS_INVALID_PARAMETER;

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(ObjectsArray,
                    ObjectCount * sizeof(ObjectsArray[0]),
                    sizeof(ULONG));
       /* make a copy so we don't have to guard with SEH later and keep track of
          what objects we referenced in case dereferencing pointers suddenly fails */
       RtlCopyMemory(SafeObjectsArray, ObjectsArray, ObjectCount * sizeof(ObjectsArray[0]));
       ObjectsArray = SafeObjectsArray;

       if(TimeOut != NULL)
       {
         ProbeForRead(TimeOut,
                      sizeof(LARGE_INTEGER),
                      sizeof(ULONG));
         /* make a local copy of the timeout on the stack */
         SafeTimeOut = *TimeOut;
         TimeOut = &SafeTimeOut;
       }
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

   /* reference all objects */
   for (i = 0; i < ObjectCount; i++)
     {
        Status = ObReferenceObjectByHandle(ObjectsArray[i],
                                           SYNCHRONIZE,
                                           NULL,
                                           PreviousMode,
                                           &ObjectPtrArray[i],
                                           NULL);
        if (!NT_SUCCESS(Status) || !KiIsObjectWaitable(ObjectPtrArray[i]))
          {
             if (NT_SUCCESS(Status))
	       {
	         DPRINT1("Waiting for object type '%wZ' is not supported\n",
		         &BODY_TO_HEADER(ObjectPtrArray[i])->Type->Name);
	         Status = STATUS_INVALID_HANDLE;
		 i++;
	       }
             /* dereference all referenced objects */
             for (j = 0; j < i; j++)
               {
                  ObDereferenceObject(ObjectPtrArray[j]);
               }

             return(Status);
          }
     }

   Status = KeWaitForMultipleObjects(ObjectCount,
                                     ObjectPtrArray,
                                     WaitType,
                                     UserRequest,
                                     PreviousMode,
                                     Alertable,
                                     TimeOut,
                                     WaitBlockArray);

   /* dereference all objects */
   for (i = 0; i < ObjectCount; i++)
     {
        ObDereferenceObject(ObjectPtrArray[i]);
     }

   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtWaitForSingleObject(IN HANDLE ObjectHandle,
		      IN BOOLEAN Alertable,
		      IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
   PVOID ObjectPtr;
   KPROCESSOR_MODE PreviousMode;
   LARGE_INTEGER SafeTimeOut;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtWaitForSingleObject(ObjectHandle %x, Alertable %d, TimeOut %x)\n",
	  ObjectHandle,Alertable,TimeOut);

   PreviousMode = ExGetPreviousMode();

   if(TimeOut != NULL && PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(TimeOut,
                    sizeof(LARGE_INTEGER),
                    sizeof(ULONG));
       /* make a copy on the stack */
       SafeTimeOut = *TimeOut;
       TimeOut = &SafeTimeOut;
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

   Status = ObReferenceObjectByHandle(ObjectHandle,
				      SYNCHRONIZE,
				      NULL,
				      PreviousMode,
				      &ObjectPtr,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (!KiIsObjectWaitable(ObjectPtr))
     {
       DPRINT1("Waiting for object type '%wZ' is not supported\n",
	       &BODY_TO_HEADER(ObjectPtr)->Type->Name);
       Status = STATUS_INVALID_HANDLE;
     }
   else
     {
       Status = KeWaitForSingleObject(ObjectPtr,
				      UserRequest,
				      PreviousMode,
				      Alertable,
				      TimeOut);
     }

   ObDereferenceObject(ObjectPtr);

   return(Status);
}


NTSTATUS
STDCALL
NtSignalAndWaitForSingleObject(IN HANDLE ObjectHandleToSignal,
                               IN HANDLE WaitableObjectHandle,
                               IN BOOLEAN Alertable,
                               IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDISPATCHER_HEADER Header;
    PVOID SignalObj;
    PVOID WaitObj;
    LARGE_INTEGER SafeTimeOut;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Capture timeout */
    if(!TimeOut && PreviousMode != KernelMode)
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

    /* FIXME: Use DefaultObject from ObjectHeader */
    Header = (PDISPATCHER_HEADER)SignalObj;
    
    /* Check dispatcher type */
    /* FIXME: Check Object Type instead! */
    switch (Header->Type)
    {
        case EventNotificationObject:
        case EventSynchronizationObject:
            /* Set the Event */
            /* FIXME: Check permissions */
            KeSetEvent(SignalObj, EVENT_INCREMENT, TRUE);
            break;

        case MutantObject:
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
            break;

        case SemaphoreObject:
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
            break;

        default:
            Status = STATUS_OBJECT_TYPE_MISMATCH;
            goto Quickie;
    }

    /* Now wait. Also SEH this since it can also raise an exception */
    _SEH_TRY
    {
        Status = KeWaitForSingleObject(WaitObj,
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
