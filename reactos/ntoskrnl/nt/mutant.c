/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/mutant.c
 * PURPOSE:         Synchronization primitives
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_TYPE ExMutantObjectType = NULL;

static GENERIC_MAPPING ExpMutantMapping = {
	STANDARD_RIGHTS_READ | SYNCHRONIZE | MUTANT_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | SYNCHRONIZE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | MUTANT_QUERY_STATE,
	MUTANT_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/


NTSTATUS STDCALL
NtpCreateMutant(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("NtpCreateMutant(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  return(STATUS_SUCCESS);
}


VOID STDCALL
NtpDeleteMutant(PVOID ObjectBody)
{
  DPRINT("NtpDeleteMutant(ObjectBody %x)\n", ObjectBody);

  KeReleaseMutant((PKMUTANT)ObjectBody,
		  MUTANT_INCREMENT,
		  TRUE,
		  FALSE);
}


VOID INIT_FUNCTION
NtInitializeMutantImplementation(VOID)
{
  ExMutantObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));

  RtlCreateUnicodeString(&ExMutantObjectType->TypeName, L"Mutant");

  ExMutantObjectType->Tag = TAG('M', 'T', 'N', 'T');
  ExMutantObjectType->PeakObjects = 0;
  ExMutantObjectType->PeakHandles = 0;
  ExMutantObjectType->TotalObjects = 0;
  ExMutantObjectType->TotalHandles = 0;
  ExMutantObjectType->PagedPoolCharge = 0;
  ExMutantObjectType->NonpagedPoolCharge = sizeof(KMUTANT);
  ExMutantObjectType->Mapping = &ExpMutantMapping;
  ExMutantObjectType->Dump = NULL;
  ExMutantObjectType->Open = NULL;
  ExMutantObjectType->Close = NULL;
  ExMutantObjectType->Delete = NtpDeleteMutant;
  ExMutantObjectType->Parse = NULL;
  ExMutantObjectType->Security = NULL;
  ExMutantObjectType->QueryName = NULL;
  ExMutantObjectType->OkayToClose = NULL;
  ExMutantObjectType->Create = NtpCreateMutant;
  ExMutantObjectType->DuplicationNotify = NULL;

  ObpCreateTypeObject(ExMutantObjectType);
}


NTSTATUS STDCALL
NtCreateMutant(OUT PHANDLE MutantHandle,
	       IN ACCESS_MASK DesiredAccess,
	       IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	       IN BOOLEAN InitialOwner)
{
  PKMUTEX Mutant;
  NTSTATUS Status;

  Status = ObCreateObject(ExGetPreviousMode(),
			  ExMutantObjectType,
			  ObjectAttributes,
			  ExGetPreviousMode(),
			  NULL,
			  sizeof(KMUTANT),
			  0,
			  0,
			  (PVOID*)&Mutant);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  KeInitializeMutant(Mutant,
		     InitialOwner);

  Status = ObInsertObject ((PVOID)Mutant,
			   NULL,
			   DesiredAccess,
			   0,
			   NULL,
			   MutantHandle);

  ObDereferenceObject(Mutant);

  return Status;
}


NTSTATUS STDCALL
NtOpenMutant(OUT PHANDLE MutantHandle,
	     IN ACCESS_MASK DesiredAccess,
	     IN POBJECT_ATTRIBUTES ObjectAttributes)
{
  return(ObOpenObjectByName(ObjectAttributes,
			    ExMutantObjectType,
			    NULL,
			    ExGetPreviousMode(),
			    DesiredAccess,
			    NULL,
			    MutantHandle));
}


NTSTATUS STDCALL
NtQueryMutant(IN HANDLE MutantHandle,
	      IN MUTANT_INFORMATION_CLASS MutantInformationClass,
	      OUT PVOID MutantInformation,
	      IN ULONG MutantInformationLength,
	      OUT PULONG ResultLength  OPTIONAL)
{
  MUTANT_BASIC_INFORMATION SafeMutantInformation;
  PKMUTANT Mutant;
  NTSTATUS Status;

  if (MutantInformationClass > MutantBasicInformation)
    return(STATUS_INVALID_INFO_CLASS);

  if (MutantInformationLength < sizeof(MUTANT_BASIC_INFORMATION))
    return(STATUS_INFO_LENGTH_MISMATCH);

  Status = ObReferenceObjectByHandle(MutantHandle,
				     MUTANT_QUERY_STATE,
				     ExMutantObjectType,
				     ExGetPreviousMode(),
				     (PVOID*)&Mutant,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  SafeMutantInformation.Count = KeReadStateMutant(Mutant);
  SafeMutantInformation.Owned = (Mutant->OwnerThread != NULL);
  SafeMutantInformation.Abandoned = Mutant->Abandoned;

  ObDereferenceObject(Mutant);
  
  Status = MmCopyToCaller(MutantInformation, &SafeMutantInformation, sizeof(MUTANT_BASIC_INFORMATION));
  if(NT_SUCCESS(Status))
  {
    if(ResultLength != NULL)
    {
      ULONG RetLen = sizeof(MUTANT_BASIC_INFORMATION);
      Status = MmCopyToCaller(ResultLength, &RetLen, sizeof(ULONG));
    }
    else
    {
      Status = STATUS_SUCCESS;
    }
  }

  return Status;
}


NTSTATUS STDCALL
NtReleaseMutant(IN HANDLE MutantHandle,
		IN PLONG PreviousCount  OPTIONAL)
{
  PKMUTANT Mutant;
  NTSTATUS Status;
  LONG Count;

  Status = ObReferenceObjectByHandle(MutantHandle,
				     MUTANT_ALL_ACCESS,
				     ExMutantObjectType,
				     ExGetPreviousMode(),
				     (PVOID*)&Mutant,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Count = KeReleaseMutant(Mutant,
			  MUTANT_INCREMENT,
			  0,
			  FALSE);
  ObDereferenceObject(Mutant);

  if (PreviousCount != NULL)
    {
      Status = MmCopyToCaller(PreviousCount, &Count, sizeof(LONG));
    }
  else
    {
      Status = STATUS_SUCCESS;
    }

  return Status;
}

/* EOF */
