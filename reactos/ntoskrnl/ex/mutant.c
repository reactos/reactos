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
 * FILE:            ntoskrnl/ex/mutant.c
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

static const INFORMATION_CLASS_INFO ExMutantInfoClass[] =
{
  ICI_SQ_SAME( sizeof(MUTANT_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* MutantBasicInformation */
};

/* FUNCTIONS *****************************************************************/


NTSTATUS STDCALL
ExpCreateMutant(PVOID ObjectBody,
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
ExpDeleteMutant(PVOID ObjectBody)
{
  DPRINT("NtpDeleteMutant(ObjectBody %x)\n", ObjectBody);

  KeReleaseMutant((PKMUTANT)ObjectBody,
		  MUTANT_INCREMENT,
		  TRUE,
		  FALSE);
}


VOID INIT_FUNCTION
ExpInitializeMutantImplementation(VOID)
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
  ExMutantObjectType->Delete = ExpDeleteMutant;
  ExMutantObjectType->Parse = NULL;
  ExMutantObjectType->Security = NULL;
  ExMutantObjectType->QueryName = NULL;
  ExMutantObjectType->OkayToClose = NULL;
  ExMutantObjectType->Create = ExpCreateMutant;
  ExMutantObjectType->DuplicationNotify = NULL;

  ObpCreateTypeObject(ExMutantObjectType);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateMutant(OUT PHANDLE MutantHandle,
	       IN ACCESS_MASK DesiredAccess,
	       IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	       IN BOOLEAN InitialOwner)
{
  KPROCESSOR_MODE PreviousMode;
  HANDLE hMutant;
  PKMUTEX Mutant;
  NTSTATUS Status = STATUS_SUCCESS;
  
   PreviousMode = ExGetPreviousMode();

   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(MutantHandle,
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
			  ExMutantObjectType,
			  ObjectAttributes,
			  PreviousMode,
			  NULL,
			  sizeof(KMUTANT),
			  0,
			  0,
			  (PVOID*)&Mutant);
  if(NT_SUCCESS(Status))
  {
    KeInitializeMutant(Mutant,
		       InitialOwner);

    Status = ObInsertObject((PVOID)Mutant,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    &hMutant);
    ObDereferenceObject(Mutant);
    
    if(NT_SUCCESS(Status))
    {
      _SEH_TRY
      {
        *MutantHandle = hMutant;
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
NtOpenMutant(OUT PHANDLE MutantHandle,
	     IN ACCESS_MASK DesiredAccess,
	     IN POBJECT_ATTRIBUTES ObjectAttributes)
{
  HANDLE hMutant;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("NtOpenMutant(0x%x, 0x%x, 0x%x)\n", MutantHandle, DesiredAccess, ObjectAttributes);

  PreviousMode = ExGetPreviousMode();

  if(PreviousMode == UserMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(MutantHandle,
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
			      ExMutantObjectType,
			      NULL,
			      PreviousMode,
			      DesiredAccess,
			      NULL,
			      &hMutant);

  if(NT_SUCCESS(Status))
  {
    _SEH_TRY
    {
      *MutantHandle = hMutant;
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
NtQueryMutant(IN HANDLE MutantHandle,
	      IN MUTANT_INFORMATION_CLASS MutantInformationClass,
	      OUT PVOID MutantInformation,
	      IN ULONG MutantInformationLength,
	      OUT PULONG ResultLength  OPTIONAL)
{
   PKMUTANT Mutant;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   DefaultQueryInfoBufferCheck(MutantInformationClass,
                               ExMutantInfoClass,
                               MutantInformation,
                               MutantInformationLength,
                               ResultLength,
                               PreviousMode,
                               &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQueryMutant() failed, Status: 0x%x\n", Status);
     return Status;
   }

   Status = ObReferenceObjectByHandle(MutantHandle,
				      MUTANT_QUERY_STATE,
				      ExMutantObjectType,
				      PreviousMode,
				      (PVOID*)&Mutant,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     switch(MutantInformationClass)
     {
       case MutantBasicInformation:
       {
         PMUTANT_BASIC_INFORMATION BasicInfo = (PMUTANT_BASIC_INFORMATION)MutantInformation;

         _SEH_TRY
         {
           BasicInfo->Count = KeReadStateMutant(Mutant);
           BasicInfo->Owned = (Mutant->OwnerThread != NULL);
           BasicInfo->Abandoned = Mutant->Abandoned;

           if(ResultLength != NULL)
           {
             *ResultLength = sizeof(MUTANT_BASIC_INFORMATION);
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

     ObDereferenceObject(Mutant);
   }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtReleaseMutant(IN HANDLE MutantHandle,
		IN PLONG PreviousCount  OPTIONAL)
{
   PKMUTANT Mutant;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtReleaseMutant(MutantHandle 0%x PreviousCount 0%x)\n",
	  MutantHandle, PreviousCount);

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

   Status = ObReferenceObjectByHandle(MutantHandle,
				      MUTANT_QUERY_STATE,
				      ExMutantObjectType,
				      PreviousMode,
				      (PVOID*)&Mutant,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     LONG Prev = KeReleaseMutant(Mutant, MUTANT_INCREMENT, 0, FALSE);
     ObDereferenceObject(Mutant);

     if(PreviousCount != NULL)
     {
       _SEH_TRY
       {
         *PreviousCount = Prev;
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
