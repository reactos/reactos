/* $Id: ntsem.c 12779 2005-01-04 04:45:00Z gdalsnes $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sem.c
 * PURPOSE:         Synchronization primitives
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
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

static const INFORMATION_CLASS_INFO ExSemaphoreInfoClass[] =
{
  ICI_SQ_SAME( sizeof(SEMAPHORE_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SemaphoreBasicInformation */
};

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

/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
		  IN ACCESS_MASK DesiredAccess,
		  IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
		  IN LONG InitialCount,
		  IN LONG MaximumCount)
{
   PKSEMAPHORE Semaphore;
   HANDLE hSemaphore;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(SemaphoreHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
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

   Status = ObCreateObject(PreviousMode,
			   ExSemaphoreObjectType,
			   ObjectAttributes,
			   PreviousMode,
			   NULL,
			   sizeof(KSEMAPHORE),
			   0,
			   0,
			   (PVOID*)&Semaphore);
   if (!NT_SUCCESS(Status))
   {
     KeInitializeSemaphore(Semaphore,
			   InitialCount,
			   MaximumCount);

     Status = ObInsertObject ((PVOID)Semaphore,
			      NULL,
			      DesiredAccess,
			      0,
			      NULL,
			      &hSemaphore);

     ObDereferenceObject(Semaphore);
   
     if(NT_SUCCESS(Status))
     {
       _SEH_TRY
       {
         *SemaphoreHandle = hSemaphore;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtOpenSemaphore(OUT PHANDLE SemaphoreHandle,
		IN ACCESS_MASK	DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   HANDLE hSemaphore;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(SemaphoreHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
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
   
   Status = ObOpenObjectByName(ObjectAttributes,
			       ExSemaphoreObjectType,
			       NULL,
			       PreviousMode,
			       DesiredAccess,
			       NULL,
			       &hSemaphore);
   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
       *SemaphoreHandle = hSemaphore;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }
   
   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtQuerySemaphore(IN HANDLE SemaphoreHandle,
		 IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
		 OUT PVOID SemaphoreInformation,
		 IN ULONG SemaphoreInformationLength,
		 OUT PULONG ReturnLength  OPTIONAL)
{
   PKSEMAPHORE Semaphore;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   DefaultQueryInfoBufferCheck(SemaphoreInformationClass,
                               ExSemaphoreInfoClass,
                               SemaphoreInformation,
                               SemaphoreInformationLength,
                               ReturnLength,
                               PreviousMode,
                               &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQuerySemaphore() failed, Status: 0x%x\n", Status);
     return Status;
   }

   Status = ObReferenceObjectByHandle(SemaphoreHandle,
				      SEMAPHORE_QUERY_STATE,
				      ExSemaphoreObjectType,
				      PreviousMode,
				      (PVOID*)&Semaphore,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     switch(SemaphoreInformationClass)
     {
       case SemaphoreBasicInformation:
       {
         PSEMAPHORE_BASIC_INFORMATION BasicInfo = (PSEMAPHORE_BASIC_INFORMATION)SemaphoreInformation;

         _SEH_TRY
         {
           BasicInfo->CurrentCount = KeReadStateSemaphore(Semaphore);
           BasicInfo->MaximumCount = Semaphore->Limit;

           if(ReturnLength != NULL)
           {
             *ReturnLength = sizeof(SEMAPHORE_BASIC_INFORMATION);
           }
         }
         _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
         _SEH_END;
         break;
       }

       default:
         Status = STATUS_NOT_IMPLEMENTED;
         break;
     }

     ObDereferenceObject(Semaphore);
   }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
		   IN LONG ReleaseCount,
		   OUT PLONG PreviousCount  OPTIONAL)
{
   KPROCESSOR_MODE PreviousMode;
   PKSEMAPHORE Semaphore;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousCount != NULL && PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(PreviousCount,
                     sizeof(LONG),
                     sizeof(ULONG));
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
   
   Status = ObReferenceObjectByHandle(SemaphoreHandle,
				      SEMAPHORE_MODIFY_STATE,
				      ExSemaphoreObjectType,
				      PreviousMode,
				      (PVOID*)&Semaphore,
				      NULL);
   if (NT_SUCCESS(Status))
   {
     LONG PrevCount = KeReleaseSemaphore(Semaphore,
                                         IO_NO_INCREMENT,
                                         ReleaseCount,
                                         FALSE);
     ObDereferenceObject(Semaphore);
     
     if(PreviousCount != NULL)
     {
       _SEH_TRY
       {
         *PreviousCount = PrevCount;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   }

   return Status;
}

/* EOF */
