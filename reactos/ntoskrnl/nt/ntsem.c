/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/ntsem.c
 * PURPOSE:         Synchronization primitives
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ExSemaphoreType;

/* FUNCTIONS *****************************************************************/

NTSTATUS NtpCreateSemaphore(PVOID ObjectBody,
			    PVOID Parent,
			    PWSTR RemainingPath,
			    POBJECT_ATTRIBUTES ObjectAttributes)
{
   
   DPRINT("NtpCreateSemaphore(ObjectBody %x, Parent %x, RemainingPath %w)\n",
	  ObjectBody, Parent, RemainingPath);
   
   if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   if (Parent != NULL && RemainingPath != NULL)
     {
	ObAddEntryDirectory(Parent, ObjectBody, RemainingPath+1);
     }
   return(STATUS_SUCCESS);
}

VOID NtInitializeSemaphoreImplementation(VOID)
{
   UNICODE_STRING TypeName;
   
   ExSemaphoreType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeString(&TypeName, L"Event");
   
   ExSemaphoreType->TypeName = TypeName;
   
   ExSemaphoreType->MaxObjects = ULONG_MAX;
   ExSemaphoreType->MaxHandles = ULONG_MAX;
   ExSemaphoreType->TotalObjects = 0;
   ExSemaphoreType->TotalHandles = 0;
   ExSemaphoreType->PagedPoolCharge = 0;
   ExSemaphoreType->NonpagedPoolCharge = sizeof(KSEMAPHORE);
   ExSemaphoreType->Dump = NULL;
   ExSemaphoreType->Open = NULL;
   ExSemaphoreType->Close = NULL;
   ExSemaphoreType->Delete = NULL;
   ExSemaphoreType->Parse = NULL;
   ExSemaphoreType->Security = NULL;
   ExSemaphoreType->QueryName = NULL;
   ExSemaphoreType->OkayToClose = NULL;
   ExSemaphoreType->Create = NtpCreateSemaphore;
}

NTSTATUS STDCALL NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
				   IN ACCESS_MASK DesiredAccess,
				   IN POBJECT_ATTRIBUTES ObjectAttributes,
				   IN ULONG InitialCount,
				   IN ULONG MaximumCount)
{
   PKSEMAPHORE Semaphore;
   
   DPRINT("NtCreateSemaphore()\n");
   Semaphore = ObCreateObject(SemaphoreHandle,
			      DesiredAccess,
			      ObjectAttributes,
			      ExSemaphoreType);
   KeInitializeSemaphore(Semaphore,
			 InitialCount,
			 MaximumCount);
   ObDereferenceObject(Semaphore);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtOpenSemaphore(IN HANDLE SemaphoreHandle,
				 IN ACCESS_MASK	DesiredAccess,
				 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   PKSEMAPHORE Semaphore;   

   
   Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
				    ObjectAttributes->Attributes,
				    NULL,
				    DesiredAccess,
				    ExSemaphoreType,
				    UserMode,
				    NULL,
				    (PVOID*)&Semaphore);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Semaphore,
			   DesiredAccess,
			   FALSE,
			   SemaphoreHandle);
   ObDereferenceObject(Semaphore);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtQuerySemaphore(HANDLE	SemaphoreHandle,
				  CINT	SemaphoreInformationClass,
				  OUT	PVOID	SemaphoreInformation,
				  ULONG	Length,
				  PULONG	ReturnLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtReleaseSemaphore(IN	HANDLE	SemaphoreHandle,
				    IN	ULONG	ReleaseCount,
				    IN	PULONG	PreviousCount)
{
   PKSEMAPHORE Semaphore;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(SemaphoreHandle,
				      SEMAPHORE_MODIFY_STATE,
				      ExSemaphoreType,
				      UserMode,
				      (PVOID*)&Semaphore,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   KeReleaseSemaphore(Semaphore,
		      IO_NO_INCREMENT,
		      ReleaseCount,
		      FALSE);
   ObDereferenceObject(Semaphore);
   return(STATUS_SUCCESS);
}
