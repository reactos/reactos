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
		         &BODY_TO_HEADER(ObjectPtrArray[i])->ObjectType->TypeName);
	         Status = STATUS_HANDLE_NOT_WAITABLE;
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
	       &BODY_TO_HEADER(ObjectPtr)->ObjectType->TypeName);
       Status = STATUS_HANDLE_NOT_WAITABLE;
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


NTSTATUS STDCALL
NtSignalAndWaitForSingleObject(IN HANDLE ObjectHandleToSignal,
			       IN HANDLE WaitableObjectHandle,
			       IN BOOLEAN Alertable,
			       IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
   KPROCESSOR_MODE PreviousMode;
   DISPATCHER_HEADER* hdr;
   PVOID SignalObj;
   PVOID WaitObj;
   LARGE_INTEGER SafeTimeOut;
   NTSTATUS Status = STATUS_SUCCESS;

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
   
   Status = ObReferenceObjectByHandle(ObjectHandleToSignal,
				      0,
				      NULL,
				      PreviousMode,
				      &SignalObj,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

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

   hdr = (DISPATCHER_HEADER *)SignalObj;
   switch (hdr->Type)
     {
      case EventNotificationObject:
      case EventSynchronizationObject:
	KeSetEvent(SignalObj,
		   EVENT_INCREMENT,
		   TRUE);
	break;

      case MutantObject:
	KeReleaseMutex(SignalObj,
		       TRUE);
	break;

      case SemaphoreObject:
	KeReleaseSemaphore(SignalObj,
			   SEMAPHORE_INCREMENT,
			   1,
			   TRUE);
	break;

      default:
	ObDereferenceObject(SignalObj);
	ObDereferenceObject(WaitObj);
	return STATUS_OBJECT_TYPE_MISMATCH;
     }

   Status = KeWaitForSingleObject(WaitObj,
				  UserRequest,
				  PreviousMode,
				  Alertable,
				  TimeOut);

   ObDereferenceObject(SignalObj);
   ObDereferenceObject(WaitObj);

   return Status;
}

/* EOF */
