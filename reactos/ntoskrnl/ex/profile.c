/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/profile.c
 * PURPOSE:         Support for profiling
 * 
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* TYPES ********************************************************************/

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExProfileObjectType = NULL;

static GENERIC_MAPPING ExpProfileMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	STANDARD_RIGHTS_ALL};

/*
 * Size of the profile hash table.
 */
#define PROFILE_HASH_TABLE_SIZE      (32)

/*
 * Table of lists of per-process profiling data structures hashed by PID.
 */
LIST_ENTRY ProcessProfileListHashTable[PROFILE_HASH_TABLE_SIZE];

/*
 * Head of the list of profile data structures for the kernel.
 */
LIST_ENTRY SystemProfileList;

/*
 * Lock that protects the profiling data structures.
 */
KSPIN_LOCK ProfileListLock;

/*
 * Timer interrupts happen before we have initialized the profiling
 * data structures so just ignore them before that.
 */
BOOLEAN ProfileInitDone = FALSE;

VOID INIT_FUNCTION
ExpInitializeProfileImplementation(VOID)
{
  ULONG i;

  InitializeListHead(&SystemProfileList);
  
  for (i = 0; i < PROFILE_HASH_TABLE_SIZE; i++)
    {
      InitializeListHead(&ProcessProfileListHashTable[i]);
    }

  KeInitializeSpinLock(&ProfileListLock);
  ProfileInitDone = TRUE;

  ExProfileObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
  
  RtlCreateUnicodeString(&ExProfileObjectType->TypeName, L"Profile");
  
  ExProfileObjectType->Tag = TAG('P', 'R', 'O', 'F');
  ExProfileObjectType->PeakObjects = 0;
  ExProfileObjectType->PeakHandles = 0;
  ExProfileObjectType->TotalObjects = 0;
  ExProfileObjectType->TotalHandles = 0;
  ExProfileObjectType->PagedPoolCharge = 0;
  ExProfileObjectType->NonpagedPoolCharge = sizeof(KPROFILE);
  ExProfileObjectType->Mapping = &ExpProfileMapping;
  ExProfileObjectType->Dump = NULL;
  ExProfileObjectType->Open = NULL;
  ExProfileObjectType->Close = NULL;
  ExProfileObjectType->Delete = KiDeleteProfile;
  ExProfileObjectType->Parse = NULL;
  ExProfileObjectType->Security = NULL;
  ExProfileObjectType->QueryName = NULL;
  ExProfileObjectType->OkayToClose = NULL;
  ExProfileObjectType->Create = NULL;

  ObpCreateTypeObject(ExProfileObjectType);
}

NTSTATUS STDCALL 
NtCreateProfile(OUT PHANDLE ProfileHandle,
		IN HANDLE Process  OPTIONAL,
		IN PVOID ImageBase, 
		IN ULONG ImageSize, 
		IN ULONG BucketSize,
		IN PVOID Buffer,
		IN ULONG BufferSize,
		IN KPROFILE_SOURCE ProfileSource,
		IN KAFFINITY Affinity)
{
  HANDLE hProfile;
  PKPROFILE Profile;
  PEPROCESS pProcess;
  KPROCESSOR_MODE PreviousMode;
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PreviousMode = ExGetPreviousMode();
  
  if(BufferSize == 0)
  {
    return STATUS_INVALID_PARAMETER_7;
  }
  
  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(ProfileHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
      ProbeForWrite(Buffer,
                    BufferSize,
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

  /*
   * Reference the associated process
   */
  if (Process != NULL)
    {
      Status = ObReferenceObjectByHandle(Process,
					 PROCESS_QUERY_INFORMATION,
					 PsProcessType,
					 PreviousMode,
					 (PVOID*)&pProcess,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  else
    {
      pProcess = NULL;
      if(!SeSinglePrivilegeCheck(SeSystemProfilePrivilege,
                                 PreviousMode))
      {
        DPRINT1("NtCreateProfile: Caller requires the SeSystemProfilePrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
      }
    }

  /*
   * Check the parameters
   */
  if ((pProcess == NULL && ImageBase < (PVOID)KERNEL_BASE) ||
      (pProcess != NULL && ImageBase >= (PVOID)KERNEL_BASE))
    {
      return(STATUS_INVALID_PARAMETER_3);
    }
  if (((ImageSize >> BucketSize) * 4) >= BufferSize)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }
  if (ProfileSource != ProfileTime)
    {
      return(STATUS_INVALID_PARAMETER_9);
    }
  if (Affinity != 0)
    {
      return(STATUS_INVALID_PARAMETER_10);
    }

  /*
   * Create the object
   */
  InitializeObjectAttributes(&ObjectAttributes,
                             NULL,
                             0,
                             NULL,
                             NULL);

  Status = ObCreateObject(KernelMode,
			  ExProfileObjectType,
			  &ObjectAttributes,
			  PreviousMode,
			  NULL,
			  sizeof(KPROFILE),
			  0,
			  0,
			  (PVOID*)&Profile);
  if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

  /*
   * Initialize it
   */
  Profile->Base = ImageBase;
  Profile->Size = ImageSize;
  Profile->BucketShift = BucketSize;
  Profile->BufferMdl = MmCreateMdl(NULL, Buffer, BufferSize);
  if(Profile->BufferMdl == NULL) {
	DPRINT("MmCreateMdl: Out of memory!");
	ObDereferenceObject (Profile);
	return(STATUS_NO_MEMORY);
  }  
  MmProbeAndLockPages(Profile->BufferMdl, UserMode, IoWriteAccess);
  Profile->Buffer = MmGetSystemAddressForMdl(Profile->BufferMdl);
  Profile->BufferSize = BufferSize;
  Profile->ProcessorMask = Affinity;
  Profile->Started = FALSE;
  Profile->Process = pProcess;

  /*
   * Insert the profile into the profile list data structures
   */
  KiInsertProfile(Profile);

  Status = ObInsertObject ((PVOID)Profile,
			   NULL,
			   STANDARD_RIGHTS_ALL,
			   0,
			   NULL,
			   &hProfile);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Profile);
      return Status;
    }

  /*
   * Copy the created handle back to the caller
   */
  _SEH_TRY
  {
    *ProfileHandle = hProfile;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  ObDereferenceObject(Profile);

  return Status;
}

NTSTATUS STDCALL 
NtQueryIntervalProfile(IN  KPROFILE_SOURCE ProfileSource,
		       OUT PULONG Interval)
{
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PreviousMode = ExGetPreviousMode();
  
  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(Interval,
                    sizeof(ULONG),
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

  if (ProfileSource == ProfileTime)
    {
      ULONG ReturnInterval;

      /* FIXME: What units does this use, for now nanoseconds */
      ReturnInterval = 100;

      _SEH_TRY
      {
        *Interval = ReturnInterval;
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
      
      return Status;
    }
  return STATUS_INVALID_PARAMETER_2;
}

NTSTATUS STDCALL 
NtSetIntervalProfile(IN ULONG Interval,
		     IN KPROFILE_SOURCE Source)
{
  return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL 
NtStartProfile(IN HANDLE ProfileHandle)
{
  PKPROFILE Profile;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;
  
  PreviousMode = ExGetPreviousMode();

  Status = ObReferenceObjectByHandle(ProfileHandle,
				     STANDARD_RIGHTS_ALL,
				     ExProfileObjectType,
				     PreviousMode,
				     (PVOID*)&Profile,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  Profile->Started = TRUE;
  ObDereferenceObject(Profile);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtStopProfile(IN HANDLE ProfileHandle)
{
  PKPROFILE Profile;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;
  
  PreviousMode = ExGetPreviousMode();

  Status = ObReferenceObjectByHandle(ProfileHandle,
				     STANDARD_RIGHTS_ALL,
				     ExProfileObjectType,
				     PreviousMode,
				     (PVOID*)&Profile,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  Profile->Started = FALSE;
  ObDereferenceObject(Profile);
  return(STATUS_SUCCESS);
}


