/* $Id: ntsem.c,v 1.15 2002/02/19 00:09:24 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/ntsem.c
 * PURPOSE:         Synchronization primitives
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <ntos/synch.h>
#include <internal/pool.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ExSemaphoreType;

static GENERIC_MAPPING ExSemaphoreMapping = {
	STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | SEMAPHORE_QUERY_STATE,
	SEMAPHORE_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtpCreateSemaphore(PVOID ObjectBody,
		   PVOID Parent,
		   PWSTR RemainingPath,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("NtpCreateSemaphore(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  return(STATUS_SUCCESS);
}

VOID NtInitializeSemaphoreImplementation(VOID)
{
   ExSemaphoreType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExSemaphoreType->TypeName, L"Semaphore");
   
   ExSemaphoreType->Tag = TAG('S', 'E', 'M', 'T');
   ExSemaphoreType->MaxObjects = ULONG_MAX;
   ExSemaphoreType->MaxHandles = ULONG_MAX;
   ExSemaphoreType->TotalObjects = 0;
   ExSemaphoreType->TotalHandles = 0;
   ExSemaphoreType->PagedPoolCharge = 0;
   ExSemaphoreType->NonpagedPoolCharge = sizeof(KSEMAPHORE);
   ExSemaphoreType->Mapping = &ExSemaphoreMapping;
   ExSemaphoreType->Dump = NULL;
   ExSemaphoreType->Open = NULL;
   ExSemaphoreType->Close = NULL;
   ExSemaphoreType->Delete = NULL;
   ExSemaphoreType->Parse = NULL;
   ExSemaphoreType->Security = NULL;
   ExSemaphoreType->QueryName = NULL;
   ExSemaphoreType->OkayToClose = NULL;
   ExSemaphoreType->Create = NtpCreateSemaphore;
   ExSemaphoreType->DuplicationNotify = NULL;
}

NTSTATUS STDCALL
NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
		  IN ACCESS_MASK DesiredAccess,
		  IN POBJECT_ATTRIBUTES ObjectAttributes,
		  IN LONG InitialCount,
		  IN LONG MaximumCount)
{
   PKSEMAPHORE Semaphore;
   NTSTATUS Status;
   
   Status = ObCreateObject(SemaphoreHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   ExSemaphoreType,
			   (PVOID*)&Semaphore);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   KeInitializeSemaphore(Semaphore,
			 InitialCount,
			 MaximumCount);
   ObDereferenceObject(Semaphore);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtOpenSemaphore(IN HANDLE SemaphoreHandle,
		IN ACCESS_MASK	DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   
   Status = ObOpenObjectByName(ObjectAttributes,
			       ExSemaphoreType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       SemaphoreHandle);
   
   return Status;
}


NTSTATUS STDCALL
NtQuerySemaphore(IN HANDLE SemaphoreHandle,
		 IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
		 OUT PVOID SemaphoreInformation,
		 IN ULONG SemaphoreInformationLength,
		 OUT PULONG ReturnLength)
{
   PSEMAPHORE_BASIC_INFORMATION Info;
   PKSEMAPHORE Semaphore;
   NTSTATUS Status;

   Info = (PSEMAPHORE_BASIC_INFORMATION)SemaphoreInformation;

   if (SemaphoreInformationClass > SemaphoreBasicInformation)
     return STATUS_INVALID_INFO_CLASS;

   if (SemaphoreInformationLength < sizeof(SEMAPHORE_BASIC_INFORMATION))
     return STATUS_INFO_LENGTH_MISMATCH;

   Status = ObReferenceObjectByHandle(SemaphoreHandle,
				      SEMAPHORE_QUERY_STATE,
				      ExSemaphoreType,
				      UserMode,
				      (PVOID*)&Semaphore,
				      NULL);
   if (!NT_SUCCESS(Status))
     return Status;

   Info->CurrentCount = KeReadStateSemaphore(Semaphore);
   Info->MaximumCount = Semaphore->Limit;

   *ReturnLength = sizeof(SEMAPHORE_BASIC_INFORMATION);

   ObDereferenceObject(Semaphore);

   return STATUS_SUCCESS;
}

NTSTATUS STDCALL
NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
		   IN LONG ReleaseCount,
		   OUT PLONG PreviousCount)
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

/* EOF */
