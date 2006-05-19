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
/* $Id$
 *
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               ntoskrnl/ob/handle.c
 * PURPOSE:            Managing handles
 * PROGRAMMER:         David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *                 17/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define GENERIC_ANY (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

/* GLOBALS *****************************************************************/

PHANDLE_TABLE ObpKernelHandleTable = NULL;

/* TEMPORARY HACK. DO NOT REMOVE -- Alex */
NTSTATUS
STDCALL
ExpDesktopCreate(PVOID ObjectBody,
                 PVOID Parent,
                 PWSTR RemainingPath,
                 POBJECT_CREATE_INFORMATION ObjectCreateInformation);

/* FUNCTIONS ***************************************************************/

static VOID
ObpDecrementHandleCount(PVOID ObjectBody)
{
  PROS_OBJECT_HEADER ObjectHeader = BODY_TO_HEADER(ObjectBody);
  LONG NewHandleCount = InterlockedDecrement(&ObjectHeader->HandleCount);
  DPRINT("Header: %x\n", ObjectHeader);
  DPRINT("NewHandleCount: %x\n", NewHandleCount);
  DPRINT("HEADER_TO_OBJECT_NAME: %x\n", HEADER_TO_OBJECT_NAME(ObjectHeader));

  if ((ObjectHeader->Type != NULL) &&
      (ObjectHeader->Type->TypeInfo.CloseProcedure != NULL))
  {
    /* the handle count should be decremented but we pass the previous value
       to the callback */
    ObjectHeader->Type->TypeInfo.CloseProcedure(NULL, ObjectBody, 0, NewHandleCount + 1, NewHandleCount + 1);
  }

  if(NewHandleCount == 0)
  {
    if(HEADER_TO_OBJECT_NAME(ObjectHeader) && 
       HEADER_TO_OBJECT_NAME(ObjectHeader)->Directory != NULL &&
       !(ObjectHeader->Flags & OB_FLAG_PERMANENT))
    {
      /* delete the object from the namespace when the last handle got closed.
         Only do this if it's actually been inserted into the namespace and
         if it's not a permanent object. */
      ObpRemoveEntryDirectory((PROS_OBJECT_HEADER)ObjectHeader);
    }

    /* remove the keep-alive reference */
    ObDereferenceObject(ObjectBody);
  }
}


NTSTATUS
NTAPI
ObpQueryHandleAttributes(HANDLE Handle,
			 POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo)
{
  PHANDLE_TABLE_ENTRY HandleTableEntry;
  PEPROCESS Process, CurrentProcess;
  KAPC_STATE ApcState;
  BOOLEAN AttachedToProcess = FALSE;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  DPRINT("ObpQueryHandleAttributes(Handle %p)\n", Handle);
  CurrentProcess = PsGetCurrentProcess();

  KeEnterCriticalRegion();

  if(ObIsKernelHandle(Handle, ExGetPreviousMode()))
  {
    Process = PsInitialSystemProcess;
    Handle = ObKernelHandleToHandle(Handle);

    if (Process != CurrentProcess)
    {
      KeStackAttachProcess(&Process->Pcb,
                           &ApcState);
      AttachedToProcess = TRUE;
    }
  }
  else
  {
    Process = CurrentProcess;
  }

  HandleTableEntry = ExMapHandleToPointer(Process->ObjectTable,
                                          Handle);
  if (HandleTableEntry != NULL)
  {
    HandleInfo->Inherit = (HandleTableEntry->ObAttributes & EX_HANDLE_ENTRY_INHERITABLE) != 0;
    HandleInfo->ProtectFromClose = (HandleTableEntry->ObAttributes & EX_HANDLE_ENTRY_PROTECTFROMCLOSE) != 0;

    ExUnlockHandleTableEntry(Process->ObjectTable,
                             HandleTableEntry);
  }
  else
    Status = STATUS_INVALID_HANDLE;

  if (AttachedToProcess)
  {
    KeUnstackDetachProcess(&ApcState);
  }

  KeLeaveCriticalRegion();

  return Status;
}


NTSTATUS
NTAPI
ObpSetHandleAttributes(HANDLE Handle,
		       POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo)
{
  PHANDLE_TABLE_ENTRY HandleTableEntry;
  PEPROCESS Process, CurrentProcess;
  KAPC_STATE ApcState;
  BOOLEAN AttachedToProcess = FALSE;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  DPRINT("ObpSetHandleAttributes(Handle %p)\n", Handle);
  CurrentProcess = PsGetCurrentProcess();

  KeEnterCriticalRegion();

  if(ObIsKernelHandle(Handle, ExGetPreviousMode()))
  {
    Process = PsInitialSystemProcess;
    Handle = ObKernelHandleToHandle(Handle);

    if (Process != CurrentProcess)
    {
      KeStackAttachProcess(&Process->Pcb,
                           &ApcState);
      AttachedToProcess = TRUE;
    }
  }
  else
  {
    Process = CurrentProcess;
  }

  HandleTableEntry = ExMapHandleToPointer(Process->ObjectTable,
                                          Handle);
  if (HandleTableEntry != NULL)
  {
    if (HandleInfo->Inherit)
      HandleTableEntry->ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
    else
      HandleTableEntry->ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;

    if (HandleInfo->ProtectFromClose)
      HandleTableEntry->ObAttributes |= EX_HANDLE_ENTRY_PROTECTFROMCLOSE;
    else
      HandleTableEntry->ObAttributes &= ~EX_HANDLE_ENTRY_PROTECTFROMCLOSE;

    /* FIXME: Do we need to set anything in the object header??? */

    ExUnlockHandleTableEntry(Process->ObjectTable,
                             HandleTableEntry);
  }
  else
    Status = STATUS_INVALID_HANDLE;

  if (AttachedToProcess)
  {
    KeUnstackDetachProcess(&ApcState);
  }

  KeLeaveCriticalRegion();

  return Status;
}


static NTSTATUS
ObpDeleteHandle(HANDLE Handle)
{
   PHANDLE_TABLE_ENTRY HandleEntry;
   PVOID Body;
   PROS_OBJECT_HEADER ObjectHeader;
   PHANDLE_TABLE ObjectTable;

   PAGED_CODE();

   DPRINT("ObpDeleteHandle(Handle %p)\n",Handle);

   ObjectTable = PsGetCurrentProcess()->ObjectTable;

   KeEnterCriticalRegion();

   HandleEntry = ExMapHandleToPointer(ObjectTable,
                                      Handle);
   if(HandleEntry != NULL)
   {
     if(HandleEntry->ObAttributes & EX_HANDLE_ENTRY_PROTECTFROMCLOSE)
     {
       ExUnlockHandleTableEntry(ObjectTable,
                                HandleEntry);

       KeLeaveCriticalRegion();

       return STATUS_HANDLE_NOT_CLOSABLE;
     }

     ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
     Body = &ObjectHeader->Body;

    /* destroy and unlock the handle entry */
     ExDestroyHandleByEntry(ObjectTable,
                            HandleEntry,
                            Handle);

     ObpDecrementHandleCount(Body);

     KeLeaveCriticalRegion();

     return STATUS_SUCCESS;
   }
   KeLeaveCriticalRegion();
   return STATUS_INVALID_HANDLE;
}


NTSTATUS
NTAPI
ObDuplicateObject(PEPROCESS SourceProcess,
		  PEPROCESS TargetProcess,
		  HANDLE SourceHandle,
		  PHANDLE TargetHandle,
		  ACCESS_MASK DesiredAccess,
		  ULONG HandleAttributes,
		  ULONG Options)
{
  PHANDLE_TABLE_ENTRY SourceHandleEntry;
  HANDLE_TABLE_ENTRY NewHandleEntry;
  BOOLEAN AttachedToProcess = FALSE;
  PVOID ObjectBody;
  PROS_OBJECT_HEADER ObjectHeader;
  ULONG NewHandleCount;
  HANDLE NewTargetHandle;
  PEPROCESS CurrentProcess;
  KAPC_STATE ApcState;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  if(SourceProcess == NULL ||
     ObIsKernelHandle(SourceHandle, ExGetPreviousMode()))
  {
    SourceProcess = PsInitialSystemProcess;
    SourceHandle = ObKernelHandleToHandle(SourceHandle);
  }

  CurrentProcess = PsGetCurrentProcess();

  KeEnterCriticalRegion();

  if (SourceProcess != CurrentProcess)
  {
    KeStackAttachProcess(&SourceProcess->Pcb,
                         &ApcState);
    AttachedToProcess = TRUE;
  }
  SourceHandleEntry = ExMapHandleToPointer(SourceProcess->ObjectTable,
                                           SourceHandle);
  if (SourceHandleEntry == NULL)
    {
      if (AttachedToProcess)
      {
        KeUnstackDetachProcess(&ApcState);
      }

      KeLeaveCriticalRegion();
      return STATUS_INVALID_HANDLE;
    }

  ObjectHeader = EX_HTE_TO_HDR(SourceHandleEntry);
  ObjectBody = &ObjectHeader->Body;

  NewHandleEntry.Object = SourceHandleEntry->Object;
  if(HandleAttributes & OBJ_INHERIT)
    NewHandleEntry.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
  else
    NewHandleEntry.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
  NewHandleEntry.GrantedAccess = ((Options & DUPLICATE_SAME_ACCESS) ?
                                     SourceHandleEntry->GrantedAccess :
                                     DesiredAccess);
  if (Options & DUPLICATE_SAME_ACCESS)
  {
    NewHandleEntry.GrantedAccess = SourceHandleEntry->GrantedAccess;
  }
  else
  {
    if (DesiredAccess & GENERIC_ANY)
    {
      RtlMapGenericMask(&DesiredAccess,
                        &ObjectHeader->Type->TypeInfo.GenericMapping);
    }
    NewHandleEntry.GrantedAccess = DesiredAccess;
  }

  /* reference the object so it doesn't get deleted after releasing the lock
     and before creating a new handle for it */
  ObReferenceObject(ObjectBody);

  /* increment the handle count of the object, it should always be >= 2 because
     we're holding a handle lock to this object! if the new handle count was
     1 here, we're in big trouble... it would've been safe to increment and
     check the handle count without using interlocked functions because the
     entry is locked, which means the handle count can't change. */
  NewHandleCount = InterlockedIncrement(&ObjectHeader->HandleCount);
  ASSERT(NewHandleCount >= 2);

  ExUnlockHandleTableEntry(SourceProcess->ObjectTable,
                           SourceHandleEntry);

  if (AttachedToProcess)
  {
    KeUnstackDetachProcess(&ApcState);
    AttachedToProcess = FALSE;
  }

  if (TargetProcess != CurrentProcess)
  {
    KeStackAttachProcess(&TargetProcess->Pcb,
                         &ApcState);
    AttachedToProcess = TRUE;
  }

  /* attempt to create the new handle */
  NewTargetHandle = ExCreateHandle(TargetProcess->ObjectTable,
                                   &NewHandleEntry);
  if (AttachedToProcess)
  {
    KeUnstackDetachProcess(&ApcState);
    AttachedToProcess = FALSE;
  }

  if (NewTargetHandle != NULL)
  {
    if (Options & DUPLICATE_CLOSE_SOURCE)
    {
      if (SourceProcess != CurrentProcess)
      {
        KeStackAttachProcess(&SourceProcess->Pcb,
                             &ApcState);
        AttachedToProcess = TRUE;
      }

      /* delete the source handle */
      ObpDeleteHandle(SourceHandle);

      if (AttachedToProcess)
      {
        KeUnstackDetachProcess(&ApcState);
      }
    }

    ObDereferenceObject(ObjectBody);

    *TargetHandle = NewTargetHandle;
  }
  else
  {
    /* decrement the handle count we previously incremented, but don't call the
       closing procedure because we're not closing a handle! */
    if(InterlockedDecrement(&ObjectHeader->HandleCount) == 0)
    {
      ObDereferenceObject(ObjectBody);
    }

    ObDereferenceObject(ObjectBody);
    Status = STATUS_UNSUCCESSFUL;
  }

  KeLeaveCriticalRegion();

  return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtDuplicateObject (IN	HANDLE		SourceProcessHandle,
		   IN	HANDLE		SourceHandle,
		   IN	HANDLE		TargetProcessHandle,
		   OUT	PHANDLE		TargetHandle  OPTIONAL,
		   IN	ACCESS_MASK	DesiredAccess,
		   IN	ULONG		HandleAttributes,
		   IN   ULONG		Options)
/*
 * FUNCTION: Copies a handle from one process space to another
 * ARGUMENTS:
 *         SourceProcessHandle = The source process owning the handle. The
 *                               source process should have opened
 *			         the SourceHandle with PROCESS_DUP_HANDLE
 *                               access.
 *	   SourceHandle = The handle to the object.
 *	   TargetProcessHandle = The destination process owning the handle
 *	   TargetHandle (OUT) = Caller should supply storage for the
 *                              duplicated handle.
 *	   DesiredAccess = The desired access to the handle.
 *	   HandleAttributes = The desired handle attributes.
 *	   Options = Specifies special actions upon duplicating the handle.
 *                   Can be one of the values DUPLICATE_CLOSE_SOURCE |
 *                   DUPLICATE_SAME_ACCESS. DUPLICATE_CLOSE_SOURCE specifies
 *                   that the source handle should be closed after duplicating.
 *                   DUPLICATE_SAME_ACCESS specifies to ignore the
 *                   DesiredAccess paramter and just grant the same access to
 *                   the new handle.
 * RETURNS: Status
 * REMARKS: This function maps to the win32 DuplicateHandle.
 */
{
   PEPROCESS SourceProcess;
   PEPROCESS TargetProcess;
   PEPROCESS CurrentProcess;
   HANDLE hTarget;
   BOOLEAN AttachedToProcess = FALSE;
   KPROCESSOR_MODE PreviousMode;
   KAPC_STATE ApcState;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();

   PreviousMode = ExGetPreviousMode();

   if(TargetHandle != NULL && PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWriteHandle(TargetHandle);
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

   Status = ObReferenceObjectByHandle(SourceProcessHandle,
				      PROCESS_DUP_HANDLE,
				      NULL,
				      PreviousMode,
				      (PVOID*)&SourceProcess,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
       return(Status);
     }

   Status = ObReferenceObjectByHandle(TargetProcessHandle,
				      PROCESS_DUP_HANDLE,
				      NULL,
				      PreviousMode,
				      (PVOID*)&TargetProcess,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
       ObDereferenceObject(SourceProcess);
       return(Status);
     }

   CurrentProcess = PsGetCurrentProcess();

   /* Check for magic handle first */
   if (SourceHandle == NtCurrentThread() ||
       SourceHandle == NtCurrentProcess())
     {
       PVOID ObjectBody;
       POBJECT_TYPE ObjectType;

       ObjectType = (SourceHandle == NtCurrentThread()) ? PsThreadType : PsProcessType;

       Status = ObReferenceObjectByHandle(SourceHandle,
                                          0,
                                          ObjectType,
                                          PreviousMode,
                                          &ObjectBody,
                                          NULL);
       if(NT_SUCCESS(Status))
       {
         if (Options & DUPLICATE_SAME_ACCESS)
         {
           /* grant all access rights */
           DesiredAccess = ((ObjectType == PsThreadType) ? THREAD_ALL_ACCESS : PROCESS_ALL_ACCESS);
         }
         else
         {
           if (DesiredAccess & GENERIC_ANY)
           {
             RtlMapGenericMask(&DesiredAccess,
                               &ObjectType->TypeInfo.GenericMapping);
           }
         }

         if (TargetProcess != CurrentProcess)
         {
           KeStackAttachProcess(&TargetProcess->Pcb,
                                &ApcState);
           AttachedToProcess = TRUE;
         }

         Status = ObpCreateHandle(ObjectBody,
                                  DesiredAccess,
                                  HandleAttributes,
                                  &hTarget);

         if (AttachedToProcess)
         {
           KeUnstackDetachProcess(&ApcState);
           AttachedToProcess = FALSE;
         }

         ObDereferenceObject(ObjectBody);

         if (Options & DUPLICATE_CLOSE_SOURCE)
         {
           if (SourceProcess != CurrentProcess)
           {
             KeStackAttachProcess(&SourceProcess->Pcb,
                                  &ApcState);
             AttachedToProcess = TRUE;
           }

           ObpDeleteHandle(SourceHandle);

           if (AttachedToProcess)
           {
             KeUnstackDetachProcess(&ApcState);
           }
         }
       }
     }
   else
     {
       Status = ObDuplicateObject(SourceProcess,
                                  TargetProcess,
                                  SourceHandle,
                                  &hTarget,
                                  DesiredAccess,
                                  HandleAttributes,
                                  Options);
     }

   ObDereferenceObject(TargetProcess);
   ObDereferenceObject(SourceProcess);

   if(NT_SUCCESS(Status) && TargetHandle != NULL)
   {
     _SEH_TRY
     {
       *TargetHandle = hTarget;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   return Status;
}

static VOID STDCALL
SweepHandleCallback(PHANDLE_TABLE HandleTable,
                    PVOID Object,
                    ULONG GrantedAccess,
                    PVOID Context)
{
  PROS_OBJECT_HEADER ObjectHeader;
  PVOID ObjectBody;

  PAGED_CODE();

  ObjectHeader = EX_OBJ_TO_HDR(Object);
  ObjectBody = &ObjectHeader->Body;

  ObpDecrementHandleCount(ObjectBody);
}

static BOOLEAN STDCALL
DuplicateHandleCallback(PHANDLE_TABLE HandleTable,
                        PHANDLE_TABLE_ENTRY HandleTableEntry,
                        PVOID Context)
{
  PROS_OBJECT_HEADER ObjectHeader;
  BOOLEAN Ret = FALSE;

  PAGED_CODE();

  Ret = (HandleTableEntry->ObAttributes & EX_HANDLE_ENTRY_INHERITABLE) != 0;
  if(Ret)
  {
    ObjectHeader = EX_HTE_TO_HDR(HandleTableEntry);
    if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
    {
      ObReferenceObject(&ObjectHeader->Body);
    }
  }

  return Ret;
}

VOID
NTAPI
ObCreateHandleTable(PEPROCESS Parent,
			 BOOLEAN Inherit,
			 PEPROCESS Process)
/*
 * FUNCTION: Creates a handle table for a process
 * ARGUMENTS:
 *       Parent = Parent process (or NULL if this is the first process)
 *       Inherit = True if the process should inherit its parent's handles
 *       Process = Process whose handle table is to be created
 */
{
   PAGED_CODE();

   DPRINT("ObCreateHandleTable(Parent %x, Inherit %d, Process %x)\n",
	  Parent,Inherit,Process);
   if(Parent != NULL)
   {
     Process->ObjectTable = ExDupHandleTable(Process,
                                             DuplicateHandleCallback,
                                             NULL,
                                             Parent->ObjectTable);
   }
   else
   {
     Process->ObjectTable = ExCreateHandleTable(Process);
   }
}


VOID
STDCALL
ObKillProcess(PEPROCESS Process)
{
   PAGED_CODE();

   /* FIXME - Temporary hack: sweep and destroy here, needs to be fixed!!! */
   ExSweepHandleTable(Process->ObjectTable,
                      SweepHandleCallback,
                      Process);
   ExDestroyHandleTable(Process->ObjectTable);
   Process->ObjectTable = NULL;
}


NTSTATUS
NTAPI
ObpCreateHandle(PVOID ObjectBody,
	        ACCESS_MASK GrantedAccess,
	        ULONG HandleAttributes,
	        PHANDLE HandleReturn)
/*
 * FUNCTION: Add a handle referencing an object
 * ARGUMENTS:
 *         obj = Object body that the handle should refer to
 * RETURNS: The created handle
 * NOTE: The handle is valid only in the context of the current process
 */
{
   HANDLE_TABLE_ENTRY NewEntry;
   PEPROCESS Process, CurrentProcess;
   PROS_OBJECT_HEADER ObjectHeader;
   HANDLE Handle;
   KAPC_STATE ApcState;
   BOOLEAN AttachedToProcess = FALSE;

   PAGED_CODE();

   DPRINT("ObpCreateHandle(obj %p)\n",ObjectBody);

   ASSERT(ObjectBody);

   CurrentProcess = PsGetCurrentProcess();

   ObjectHeader = BODY_TO_HEADER(ObjectBody);

   /* check that this is a valid kernel pointer */
   ASSERT((ULONG_PTR)ObjectHeader & EX_HANDLE_ENTRY_LOCKED);

   if (GrantedAccess & MAXIMUM_ALLOWED)
     {
        GrantedAccess &= ~MAXIMUM_ALLOWED;
        GrantedAccess |= GENERIC_ALL;
     }

   if (GrantedAccess & GENERIC_ANY)
     {
       RtlMapGenericMask(&GrantedAccess,
		         &ObjectHeader->Type->TypeInfo.GenericMapping);
     }

   NewEntry.Object = ObjectHeader;
   if(HandleAttributes & OBJ_INHERIT)
     NewEntry.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
   else
     NewEntry.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
   NewEntry.GrantedAccess = GrantedAccess;

   if ((HandleAttributes & OBJ_KERNEL_HANDLE) &&
       ExGetPreviousMode == KernelMode)
   {
       Process = PsInitialSystemProcess;
       if (Process != CurrentProcess)
       {
           KeStackAttachProcess(&Process->Pcb,
                                &ApcState);
           AttachedToProcess = TRUE;
       }
   }
   else
   {
       Process = CurrentProcess;
       /* mask out the OBJ_KERNEL_HANDLE attribute */
       HandleAttributes &= ~OBJ_KERNEL_HANDLE;
   }

   Handle = ExCreateHandle(Process->ObjectTable,
                           &NewEntry);

   if (AttachedToProcess)
   {
       KeUnstackDetachProcess(&ApcState);
   }

   if(Handle != NULL)
   {
     if (HandleAttributes & OBJ_KERNEL_HANDLE)
     {
       /* mark the handle value */
       Handle = ObMarkHandleAsKernelHandle(Handle);
     }

     if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
     {
       ObReferenceObject(ObjectBody);
     }

     *HandleReturn = Handle;

     return STATUS_SUCCESS;
   }

   return STATUS_UNSUCCESSFUL;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ObQueryObjectAuditingByHandle(IN HANDLE Handle,
			      OUT PBOOLEAN GenerateOnClose)
{
  PHANDLE_TABLE_ENTRY HandleEntry;
  PEPROCESS Process, CurrentProcess;
  KAPC_STATE ApcState;
  BOOLEAN AttachedToProcess = FALSE;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  DPRINT("ObQueryObjectAuditingByHandle(Handle %p)\n", Handle);

  CurrentProcess = PsGetCurrentProcess();

  KeEnterCriticalRegion();

  if(ObIsKernelHandle(Handle, ExGetPreviousMode()))
  {
    Process = PsInitialSystemProcess;
    Handle = ObKernelHandleToHandle(Handle);

    if (Process != CurrentProcess)
    {
      KeStackAttachProcess(&Process->Pcb,
                           &ApcState);
      AttachedToProcess = TRUE;
    }
  }
  else
     Process = CurrentProcess;

  HandleEntry = ExMapHandleToPointer(Process->ObjectTable,
                                     Handle);
  if(HandleEntry != NULL)
  {
    *GenerateOnClose = (HandleEntry->ObAttributes & EX_HANDLE_ENTRY_AUDITONCLOSE) != 0;

    ExUnlockHandleTableEntry(Process->ObjectTable,
                             HandleEntry);
  }
  else
    Status = STATUS_INVALID_HANDLE;

  if (AttachedToProcess)
  {
    KeUnstackDetachProcess(&ApcState);
  }

  KeLeaveCriticalRegion();

  return Status;
}


/*
 * FUNCTION: Increments the reference count for an object and returns a
 * pointer to its body
 * ARGUMENTS:
 *         Handle = Handle for the object
 *         DesiredAccess = Desired access to the object
 *         ObjectType
 *         AccessMode
 *         Object (OUT) = Points to the object body on return
 *         HandleInformation (OUT) = Contains information about the handle
 *                                   on return
 * RETURNS: Status
 *
 * @implemented
 */
NTSTATUS STDCALL
ObReferenceObjectByHandle(HANDLE Handle,
			  ACCESS_MASK DesiredAccess,
			  POBJECT_TYPE ObjectType,
			  KPROCESSOR_MODE AccessMode,
			  PVOID* Object,
			  POBJECT_HANDLE_INFORMATION HandleInformation)
{
   PHANDLE_TABLE_ENTRY HandleEntry;
   PROS_OBJECT_HEADER ObjectHeader;
   PVOID ObjectBody;
   ACCESS_MASK GrantedAccess;
   ULONG Attributes;
   PEPROCESS CurrentProcess, Process;
   BOOLEAN AttachedToProcess = FALSE;
   KAPC_STATE ApcState;

   PAGED_CODE();

   DPRINT("ObReferenceObjectByHandle(Handle %p, DesiredAccess %x, "
	   "ObjectType %p, AccessMode %d, Object %p)\n",Handle,DesiredAccess,
	   ObjectType,AccessMode,Object);

   if (Handle == NULL)
     {
       return STATUS_INVALID_HANDLE;
     }

   CurrentProcess = PsGetCurrentProcess();

   /*
    * Handle special handle names
    */
   if (Handle == NtCurrentProcess() &&
       (ObjectType == PsProcessType || ObjectType == NULL))
     {
        ObReferenceObject(CurrentProcess);

	if (HandleInformation != NULL)
	  {
	    HandleInformation->HandleAttributes = 0;
	    HandleInformation->GrantedAccess = PROCESS_ALL_ACCESS;
	  }

	*Object = CurrentProcess;
	DPRINT("Referencing current process %p\n", CurrentProcess);
	return STATUS_SUCCESS;
     }
   else if (Handle == NtCurrentProcess())
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }

   if (Handle == NtCurrentThread() &&
       (ObjectType == PsThreadType || ObjectType == NULL))
     {
        PETHREAD CurrentThread = PsGetCurrentThread();

        ObReferenceObject(CurrentThread);

	if (HandleInformation != NULL)
	  {
	    HandleInformation->HandleAttributes = 0;
	    HandleInformation->GrantedAccess = THREAD_ALL_ACCESS;
	  }

	*Object = CurrentThread;
	CHECKPOINT;
	return STATUS_SUCCESS;
     }
   else if (Handle == NtCurrentThread())
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }

   /* desire as much access rights as possible */
   if (DesiredAccess & MAXIMUM_ALLOWED)
     {
        DesiredAccess &= ~MAXIMUM_ALLOWED;
        DesiredAccess |= GENERIC_ALL;
     }

   if(ObIsKernelHandle(Handle, AccessMode))
   {
      Process = PsInitialSystemProcess;
      Handle = ObKernelHandleToHandle(Handle);
   }
   else
   {
      Process = CurrentProcess;
   }

   KeEnterCriticalRegion();

   if (Process != CurrentProcess)
   {
     KeStackAttachProcess(&Process->Pcb,
                          &ApcState);
     AttachedToProcess = TRUE;
   }

   HandleEntry = ExMapHandleToPointer(Process->ObjectTable,
				      Handle);
   if (HandleEntry == NULL)
     {
        if (AttachedToProcess)
        {
            KeUnstackDetachProcess(&ApcState);
        }
        KeLeaveCriticalRegion();
        DPRINT("ExMapHandleToPointer() failed for handle 0x%p\n", Handle);
        return(STATUS_INVALID_HANDLE);
     }

   ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
   ObjectBody = &ObjectHeader->Body;

   DPRINT("locked1: ObjectHeader: 0x%p [HT:0x%p]\n", ObjectHeader, Process->ObjectTable);

   if (ObjectType != NULL && ObjectType != ObjectHeader->Type)
     {
        DPRINT("ObjectType mismatch: %wZ vs %wZ (handle 0x%p)\n", &ObjectType->Name, ObjectHeader->Type ? &ObjectHeader->Type->Name : NULL, Handle);

        ExUnlockHandleTableEntry(Process->ObjectTable,
                                 HandleEntry);

        if (AttachedToProcess)
        {
            KeUnstackDetachProcess(&ApcState);
        }

        KeLeaveCriticalRegion();

        return(STATUS_OBJECT_TYPE_MISMATCH);
     }

   /* map the generic access masks if the caller asks for generic access */
   if (DesiredAccess & GENERIC_ANY)
     {
        RtlMapGenericMask(&DesiredAccess,
                          &BODY_TO_HEADER(ObjectBody)->Type->TypeInfo.GenericMapping);
     }

   GrantedAccess = HandleEntry->GrantedAccess;

   /* Unless running as KernelMode, deny access if caller desires more access
      rights than the handle can grant */
   if(AccessMode != KernelMode && (~GrantedAccess & DesiredAccess))
     {
        ExUnlockHandleTableEntry(Process->ObjectTable,
                                 HandleEntry);

        if (AttachedToProcess)
        {
            KeUnstackDetachProcess(&ApcState);
        }

        KeLeaveCriticalRegion();

        DPRINT1("GrantedAccess: 0x%x, ~GrantedAccess: 0x%x, DesiredAccess: 0x%x, denied: 0x%x\n", GrantedAccess, ~GrantedAccess, DesiredAccess, ~GrantedAccess & DesiredAccess);

        return(STATUS_ACCESS_DENIED);
     }

   ObReferenceObject(ObjectBody);

   Attributes = HandleEntry->ObAttributes & (EX_HANDLE_ENTRY_PROTECTFROMCLOSE |
                                                EX_HANDLE_ENTRY_INHERITABLE |
                                                EX_HANDLE_ENTRY_AUDITONCLOSE);

   ExUnlockHandleTableEntry(Process->ObjectTable,
                            HandleEntry);

   if (AttachedToProcess)
   {
       KeUnstackDetachProcess(&ApcState);
   }

   KeLeaveCriticalRegion();

   if (HandleInformation != NULL)
     {
	HandleInformation->HandleAttributes = Attributes;
	HandleInformation->GrantedAccess = GrantedAccess;
     }

   *Object = ObjectBody;

   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtClose
 *
 * DESCRIPTION
 *	Closes a handle reference to an object.
 *
 * ARGUMENTS
 *	Handle
 *		Handle to close.
 *
 * RETURN VALUE
 * 	Status.
 *
 * @implemented
 */
NTSTATUS STDCALL
NtClose(IN HANDLE Handle)
{
   PEPROCESS Process, CurrentProcess;
   BOOLEAN AttachedToProcess = FALSE;
   KAPC_STATE ApcState;
   NTSTATUS Status;
   KPROCESSOR_MODE PreviousMode;

   PAGED_CODE();

   PreviousMode = ExGetPreviousMode();
   CurrentProcess = PsGetCurrentProcess();

   if(ObIsKernelHandle(Handle, PreviousMode))
   {
      Process = PsInitialSystemProcess;
      Handle = ObKernelHandleToHandle(Handle);

      if (Process != CurrentProcess)
      {
        KeStackAttachProcess(&Process->Pcb,
                             &ApcState);
        AttachedToProcess = TRUE;
      }
   }
   else
      Process = CurrentProcess;

   Status = ObpDeleteHandle(Handle);

   if (AttachedToProcess)
   {
     KeUnstackDetachProcess(&ApcState);
   }

   if (!NT_SUCCESS(Status))
     {
        if((PreviousMode != KernelMode) &&
           (CurrentProcess->ExceptionPort))
        {
           KeRaiseUserException(Status);
        }
	return Status;
     }

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS 
STDCALL
ObInsertObject(IN PVOID Object,
               IN PACCESS_STATE PassedAccessState OPTIONAL,
               IN ACCESS_MASK DesiredAccess,
               IN ULONG AdditionalReferences,
               OUT PVOID* ReferencedObject OPTIONAL,
               OUT PHANDLE Handle)
{
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    PROS_OBJECT_HEADER Header;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    PVOID FoundObject = NULL;
    PROS_OBJECT_HEADER FoundHeader = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING RemainingPath;
    BOOLEAN ObjectAttached = FALSE; 
    PSECURITY_DESCRIPTOR NewSecurityDescriptor = NULL;
    SECURITY_SUBJECT_CONTEXT SubjectContext;

    PAGED_CODE();
    
    /* Get the Header and Create Info */
    DPRINT("ObInsertObject: %x\n", Object);
    Header = BODY_TO_HEADER(Object);
    ObjectCreateInfo = Header->ObjectCreateInfo;
    ObjectNameInfo = HEADER_TO_OBJECT_NAME(Header);
    
    /* First try to find the Object */
    if (ObjectNameInfo && ObjectNameInfo->Name.Buffer)
    {
        DPRINT("Object has a name. Trying to find it: %wZ.\n", &ObjectNameInfo->Name);
        Status = ObFindObject(ObjectCreateInfo,
                              &ObjectNameInfo->Name,
                              &FoundObject,
                              &RemainingPath,
                              NULL);
        DPRINT("FoundObject: %x, Path: %wZ\n", FoundObject, &RemainingPath);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ObFindObject() failed! (Status 0x%x)\n", Status);
            return Status;
        }
        
        if (FoundObject)
        {
            DPRINT("Getting header: %x\n", FoundObject);
            FoundHeader = BODY_TO_HEADER(FoundObject);
        }
        
        if (FoundHeader && RemainingPath.Buffer == NULL)
        {
            DPRINT("Object exists\n");
            ObDereferenceObject(FoundObject);
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }
    else
    {
        DPRINT("No name, empty remaining path\n");
        RtlInitUnicodeString(&RemainingPath, NULL);
    }

    if (FoundHeader && FoundHeader->Type == ObDirectoryType &&
        RemainingPath.Buffer)
    {
        /* The name was changed so let's update it */
        /* FIXME: TEMPORARY HACK This will go in ObFindObject in the next commit */
        PVOID NewName;
        PWSTR BufferPos = RemainingPath.Buffer;
        ULONG Delta = 0;
        
        ObpAddEntryDirectory(FoundObject, (PROS_OBJECT_HEADER)Header, NULL);
        ObjectAttached = TRUE;
        
        ObjectNameInfo = HEADER_TO_OBJECT_NAME(Header);
        
        if (BufferPos[0] == L'\\')
        {
            BufferPos++;
            Delta = sizeof(WCHAR);
        }
        NewName = ExAllocatePool(NonPagedPool, RemainingPath.MaximumLength - Delta);
        RtlMoveMemory(NewName, BufferPos, RemainingPath.MaximumLength - Delta);
        if (ObjectNameInfo->Name.Buffer) ExFreePool(ObjectNameInfo->Name.Buffer);
        ObjectNameInfo->Name.Buffer = NewName;
        ObjectNameInfo->Name.Length = RemainingPath.Length - Delta;
        ObjectNameInfo->Name.MaximumLength = RemainingPath.MaximumLength - Delta;
        DPRINT("Name: %S\n", ObjectNameInfo->Name.Buffer);
    }

    if ((Header->Type == IoFileObjectType) ||
        (Header->Type == ExDesktopObjectType) ||
        (Header->Type->TypeInfo.OpenProcedure != NULL))
    {    
        DPRINT("About to call Open Routine\n");
        if (Header->Type == IoFileObjectType)
        {
            /* TEMPORARY HACK. DO NOT TOUCH -- Alex */
            DPRINT("Calling IopCreateFile: %x\n", FoundObject);
            Status = IopCreateFile(&Header->Body,
                                   FoundObject,
                                   RemainingPath.Buffer,            
                                   ObjectCreateInfo);
            DPRINT("Called IopCreateFile: %x\n", Status);
                                   
        }
        else if (Header->Type == ExDesktopObjectType)
        {
            /* TEMPORARY HACK. DO NOT TOUCH -- Alex */
            DPRINT("Calling ExpDesktopCreate\n");
            Status = ExpDesktopCreate(&Header->Body,
                                      FoundObject,
                                      RemainingPath.Buffer,            
                                      ObjectCreateInfo);
        }
        else if (Header->Type->TypeInfo.OpenProcedure != NULL)
        {
            DPRINT("Calling %x\n", Header->Type->TypeInfo.OpenProcedure);
            Status = Header->Type->TypeInfo.OpenProcedure(ObCreateHandle,
                                                                NULL,
                                                                &Header->Body,
                                                                0,
                                                                0);
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT("Create Failed\n");
            if (ObjectAttached == TRUE)
            {
                ObpRemoveEntryDirectory((PROS_OBJECT_HEADER)Header);
            }
            if (FoundObject)
            {
                ObDereferenceObject(FoundObject);
            }
            RtlFreeUnicodeString(&RemainingPath);
            return Status;
        }
    }

    RtlFreeUnicodeString(&RemainingPath);
  
    DPRINT("Security Assignment in progress\n");  
    SeCaptureSubjectContext(&SubjectContext);

    /* Build the new security descriptor */
    Status = SeAssignSecurity((FoundHeader != NULL) ? FoundHeader->SecurityDescriptor : NULL,
			    (ObjectCreateInfo != NULL) ? ObjectCreateInfo->SecurityDescriptor : NULL,
			    &NewSecurityDescriptor,
			    (Header->Type == ObDirectoryType),
			    &SubjectContext,
			    &Header->Type->TypeInfo.GenericMapping,
			    PagedPool);

    if (NT_SUCCESS(Status))
    {
        DPRINT("NewSecurityDescriptor %p\n", NewSecurityDescriptor);

        if (Header->Type->TypeInfo.SecurityProcedure != NULL)
        {
            /* Call the security method */
            Status = Header->Type->TypeInfo.SecurityProcedure(&Header->Body,
                                                                    AssignSecurityDescriptor,
                                                                    0,
                                                                    NewSecurityDescriptor,
                                                                    NULL,
                                                                    NULL,
                                                                    NonPagedPool,
                                                                    NULL);
        }
        else
        {
            /* Assign the security descriptor to the object header */
            Status = ObpAddSecurityDescriptor(NewSecurityDescriptor,
                                              &Header->SecurityDescriptor);
            DPRINT("Object security descriptor %p\n", Header->SecurityDescriptor);
        }

        /* Release the new security descriptor */
        SeDeassignSecurity(&NewSecurityDescriptor);
    }

    DPRINT("Security Complete\n");
    SeReleaseSubjectContext(&SubjectContext);
        
    /* Create the Handle */
    /* HACKHACK: Because of ROS's incorrect startup, this can be called
     * without a valid Process until I finalize the startup patch,
     * so don't create a handle if this is the case. We also don't create
     * a handle if Handle is NULL when the Registry Code calls it, because
     * the registry code totally bastardizes the Ob and needs to be fixed
     */
    DPRINT("Creating handle\n");
    if (Handle != NULL)
    {
        Status = ObpCreateHandle(&Header->Body,
                                 DesiredAccess,
                                 ObjectCreateInfo->Attributes,
                                 Handle);
        DPRINT("handle Created: %d. refcount. handlecount %d %d\n",
                 *Handle, Header->PointerCount, Header->HandleCount);
    }
    
    /* We can delete the Create Info now */
    Header->ObjectCreateInfo = NULL;
    ObpReleaseCapturedAttributes(ObjectCreateInfo);
    ExFreePool(ObjectCreateInfo);
    
    DPRINT("Status %x\n", Status);
    return Status;
}


ULONG
NTAPI
ObpGetHandleCountByHandleTable(PHANDLE_TABLE HandleTable)
{
  return HandleTable->HandleCount;
}

/*
 * FUNCTION: Searches the handle table of a specified process whether it contains a
 *           valid handle to the Object we're looking for. If not, it'll create one.
 *
 * NOTES:
 * The parameters of this function is basically a mixture of some of the parameters
 * of ObReferenceObjectByHandle() and ObReferenceObjectByPointer(). A little thinking
 * about what this function does (by it's name) makes clear what parameters it requires.
 * For example the AccessMode parameter of ObReferenceObjectByHandle/Pointer() is not
 * required at all as it only has influence on the object security. This function doesn't
 * want to get access to an object, it just looks for a valid handle and if it can't find
 * one, it'll just create one. It wouldn't make sense to check for security again as the
 * caller already has a pointer to the object.
 *
 * A test on an XP machine shows that this prototype appears to be correct.
 *
 * ARGUMENTS:
 * Process = This parameter simply describes in which handle table we're looking
 *           for a handle to the object.
 * Object = The object pointer that we're looking for
 * ObjectType = Just a sanity check as ObReferenceObjectByHandle() and
 *              ObReferenceObjectByPointer() provides.
 * HandleInformation = This one has to be the opposite meaning of the usage in
 *                     ObReferenceObjectByHandle(). If we actually found a valid
 *                     handle in the table, we need to check against the information
 *                     provided so we make sure this handle has all access rights
 *                     (and attributes?!) we need. If they don't match, we can't
 *                     use this handle and keep looking because the caller is likely
 *                     to depend on these access rights.
 * HandleReturn = The last parameter is the same as in ObCreateHandle(). If we could
 *                find a suitable handle in the handle table, return this handle, if
 *                not, we'll just create one using ObCreateHandle() with all access
 *                rights the caller needs.
 *
 * RETURNS: Status
 *
 * @unimplemented
 */
NTSTATUS STDCALL
ObFindHandleForObject(IN PEPROCESS Process,
                      IN PVOID Object,
                      IN POBJECT_TYPE ObjectType,
                      IN POBJECT_HANDLE_INFORMATION HandleInformation,
                      OUT PHANDLE HandleReturn)
{
  DPRINT("ObFindHandleForObject is unimplemented!\n");
  return STATUS_UNSUCCESSFUL;
}

VOID
ObpGetNextHandleByProcessCount(PSYSTEM_HANDLE_TABLE_ENTRY_INFO pshi,
                               PEPROCESS Process,
                               int Count)
{
      ULONG P;
//      KIRQL oldIrql;

//      pshi->HandleValue;

/*
   This will never work with ROS! M$, I guess uses 0 -> 65535.
   Ros uses 0 -> 4294967295!
 */

      P = (ULONG) Process->UniqueProcessId;
      pshi->UniqueProcessId = (USHORT) P;

//      KeAcquireSpinLock( &Process->HandleTable.ListLock, &oldIrql );

//      pshi->GrantedAccess;
//      pshi->Object;
//      pshi->TypeIndex;
//      pshi->HandleAttributes;

//      KeReleaseSpinLock( &Process->HandleTable.ListLock, oldIrql );

      return;
}



/* EOF */
