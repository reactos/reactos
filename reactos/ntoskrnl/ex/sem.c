/* $Id: ntsem.c 12779 2005-01-04 04:45:00Z gdalsnes $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sem.c
 * PURPOSE:         Synchronization primitives
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ExSemaphoreObjectType;

static GENERIC_MAPPING ExSemaphoreMapping = {
	STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | SEMAPHORE_QUERY_STATE,
	SEMAPHORE_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
ExpCreateSemaphore(PVOID ObjectBody,
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

VOID INIT_FUNCTION
ExpInitializeSemaphoreImplementation(VOID)
{
   ExSemaphoreObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExSemaphoreObjectType->TypeName, L"Semaphore");
   
   ExSemaphoreObjectType->Tag = TAG('S', 'E', 'M', 'T');
   ExSemaphoreObjectType->PeakObjects = 0;
   ExSemaphoreObjectType->PeakHandles = 0;
   ExSemaphoreObjectType->TotalObjects = 0;
   ExSemaphoreObjectType->TotalHandles = 0;
   ExSemaphoreObjectType->PagedPoolCharge = 0;
   ExSemaphoreObjectType->NonpagedPoolCharge = sizeof(KSEMAPHORE);
   ExSemaphoreObjectType->Mapping = &ExSemaphoreMapping;
   ExSemaphoreObjectType->Dump = NULL;
   ExSemaphoreObjectType->Open = NULL;
   ExSemaphoreObjectType->Close = NULL;
   ExSemaphoreObjectType->Delete = NULL;
   ExSemaphoreObjectType->Parse = NULL;
   ExSemaphoreObjectType->Security = NULL;
   ExSemaphoreObjectType->QueryName = NULL;
   ExSemaphoreObjectType->OkayToClose = NULL;
   ExSemaphoreObjectType->Create = ExpCreateSemaphore;
   ExSemaphoreObjectType->DuplicationNotify = NULL;

   ObpCreateTypeObject(ExSemaphoreObjectType);
}

NTSTATUS STDCALL
NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
		  IN ACCESS_MASK DesiredAccess,
		  IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
		  IN LONG InitialCount,
		  IN LONG MaximumCount)
{
   PKSEMAPHORE Semaphore;
   NTSTATUS Status;

   Status = ObCreateObject(ExGetPreviousMode(),
			   ExSemaphoreObjectType,
			   ObjectAttributes,
			   ExGetPreviousMode(),
			   NULL,
			   sizeof(KSEMAPHORE),
			   0,
			   0,
			   (PVOID*)&Semaphore);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   KeInitializeSemaphore(Semaphore,
			 InitialCount,
			 MaximumCount);

   Status = ObInsertObject ((PVOID)Semaphore,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    SemaphoreHandle);

   ObDereferenceObject(Semaphore);

   return Status;
}


NTSTATUS STDCALL
NtOpenSemaphore(IN HANDLE SemaphoreHandle,
		IN ACCESS_MASK	DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   
   Status = ObOpenObjectByName(ObjectAttributes,
			       ExSemaphoreObjectType,
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
		 OUT PULONG ReturnLength  OPTIONAL)
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
				      ExSemaphoreObjectType,
				      UserMode,
				      (PVOID*)&Semaphore,
				      NULL);
   if (!NT_SUCCESS(Status))
     return Status;

   Info->CurrentCount = KeReadStateSemaphore(Semaphore);
   Info->MaximumCount = Semaphore->Limit;

   if (ReturnLength != NULL)
     *ReturnLength = sizeof(SEMAPHORE_BASIC_INFORMATION);

   ObDereferenceObject(Semaphore);

   return STATUS_SUCCESS;
}

NTSTATUS STDCALL
NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
		   IN LONG ReleaseCount,
		   OUT PLONG PreviousCount  OPTIONAL)
{
   PKSEMAPHORE Semaphore;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(SemaphoreHandle,
				      SEMAPHORE_MODIFY_STATE,
				      ExSemaphoreObjectType,
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
