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

#define EX_OBJ_TO_HDR(eob) ((POBJECT_HEADER)((ULONG_PTR)(eob) &                \
  ~(EX_HANDLE_ENTRY_PROTECTFROMCLOSE | EX_HANDLE_ENTRY_INHERITABLE |           \
  EX_HANDLE_ENTRY_AUDITONCLOSE)))
#define EX_HTE_TO_HDR(hte) ((POBJECT_HEADER)((ULONG_PTR)((hte)->u1.Object) &   \
  ~(EX_HANDLE_ENTRY_PROTECTFROMCLOSE | EX_HANDLE_ENTRY_INHERITABLE |           \
  EX_HANDLE_ENTRY_AUDITONCLOSE)))

#define GENERIC_ANY (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

/* FUNCTIONS ***************************************************************/

VOID
STDCALL
ObKillProcess(PEPROCESS Process)
{
    ObDeleteHandleTable(Process);
}

VOID
ObpDecrementHandleCount(PVOID ObjectBody)
{
  POBJECT_HEADER ObjectHeader = BODY_TO_HEADER(ObjectBody);
  LONG NewHandleCount = InterlockedDecrement(&ObjectHeader->HandleCount);

  if ((ObjectHeader->ObjectType != NULL) &&
      (ObjectHeader->ObjectType->Close != NULL))
  {
    /* the handle count should be decremented but we pass the previous value
       to the callback */
    ObjectHeader->ObjectType->Close(ObjectBody, NewHandleCount + 1);
  }

  if(NewHandleCount == 0)
  {
    ObDereferenceObject(ObjectBody);
  }
}


NTSTATUS
ObpQueryHandleAttributes(HANDLE Handle,
			 POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo)
{
  PEPROCESS Process;
  PHANDLE_TABLE_ENTRY HandleTableEntry;
  LONG ExHandle = HANDLE_TO_EX_HANDLE(Handle);
  
  PAGED_CODE();

  DPRINT("ObpQueryHandleAttributes(Handle %x)\n", Handle);

  KeEnterCriticalRegion();

  Process = PsGetCurrentProcess();

  HandleTableEntry = ExMapHandleToPointer(Process->ObjectTable,
                                          ExHandle);
  if (HandleTableEntry == NULL)
    {
      KeLeaveCriticalRegion();
      return STATUS_INVALID_HANDLE;
    }

  HandleInfo->Inherit = (HandleTableEntry->u1.ObAttributes & EX_HANDLE_ENTRY_INHERITABLE) != 0;
  HandleInfo->ProtectFromClose = (HandleTableEntry->u1.ObAttributes & EX_HANDLE_ENTRY_PROTECTFROMCLOSE) != 0;

  ExUnlockHandleTableEntry(Process->ObjectTable,
                           HandleTableEntry);

  KeLeaveCriticalRegion();

  return STATUS_SUCCESS;
}


NTSTATUS
ObpSetHandleAttributes(HANDLE Handle,
		       POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo)
{
  PEPROCESS Process;
  PHANDLE_TABLE_ENTRY HandleTableEntry;
  LONG ExHandle = HANDLE_TO_EX_HANDLE(Handle);
  
  PAGED_CODE();

  DPRINT("ObpSetHandleAttributes(Handle %x)\n", Handle);

  Process = PsGetCurrentProcess();
  
  KeEnterCriticalRegion();

  HandleTableEntry = ExMapHandleToPointer(Process->ObjectTable,
                                          ExHandle);
  if (HandleTableEntry == NULL)
    {
      KeLeaveCriticalRegion();
      return STATUS_INVALID_HANDLE;
    }

  if (HandleInfo->Inherit)
    HandleTableEntry->u1.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
  else
    HandleTableEntry->u1.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;

  if (HandleInfo->ProtectFromClose)
    HandleTableEntry->u1.ObAttributes |= EX_HANDLE_ENTRY_PROTECTFROMCLOSE;
  else
    HandleTableEntry->u1.ObAttributes &= ~EX_HANDLE_ENTRY_PROTECTFROMCLOSE;

  /* FIXME: Do we need to set anything in the object header??? */

  ExUnlockHandleTableEntry(Process->ObjectTable,
                           HandleTableEntry);

  KeLeaveCriticalRegion();

  return STATUS_SUCCESS;
}


NTSTATUS
ObDuplicateObject(PEPROCESS SourceProcess,
		  PEPROCESS TargetProcess,
		  HANDLE SourceHandle,
		  PHANDLE TargetHandle,
		  ACCESS_MASK DesiredAccess,
		  BOOLEAN InheritHandle,
		  ULONG Options)
{
  PHANDLE_TABLE_ENTRY SourceHandleEntry;
  HANDLE_TABLE_ENTRY NewHandleEntry;
  PVOID ObjectBody;
  POBJECT_HEADER ObjectHeader;
  LONG ExTargetHandle;
  LONG ExSourceHandle = HANDLE_TO_EX_HANDLE(SourceHandle);
  ULONG NewHandleCount;
  
  PAGED_CODE();
  
  KeEnterCriticalRegion();
  
  SourceHandleEntry = ExMapHandleToPointer(SourceProcess->ObjectTable,
                                           ExSourceHandle);
  if (SourceHandleEntry == NULL)
    {
      KeLeaveCriticalRegion();
      return STATUS_INVALID_HANDLE;
    }

  ObjectHeader = EX_HTE_TO_HDR(SourceHandleEntry);
  ObjectBody = HEADER_TO_BODY(ObjectHeader);

  NewHandleEntry.u1.Object = SourceHandleEntry->u1.Object;
  if(InheritHandle)
    NewHandleEntry.u1.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
  else
    NewHandleEntry.u1.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
  NewHandleEntry.u2.GrantedAccess = ((Options & DUPLICATE_SAME_ACCESS) ?
                                     SourceHandleEntry->u2.GrantedAccess :
                                     DesiredAccess);
  
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

  KeLeaveCriticalRegion();

  /* attempt to create the new handle */
  ExTargetHandle = ExCreateHandle(TargetProcess->ObjectTable,
                                  &NewHandleEntry);
  if (ExTargetHandle != EX_INVALID_HANDLE)
  {
    if (Options & DUPLICATE_CLOSE_SOURCE)
    {
      ObDeleteHandle(SourceProcess,
                     SourceHandle);
    }
    
    ObDereferenceObject(ObjectBody);

    *TargetHandle = EX_HANDLE_TO_HANDLE(ExTargetHandle);
    
    return STATUS_SUCCESS;
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
    return STATUS_UNSUCCESSFUL;
  }
}

/*
 * @implemented
 */
NTSTATUS STDCALL 
NtDuplicateObject (IN	HANDLE		SourceProcessHandle,
		   IN	HANDLE		SourceHandle,
		   IN	HANDLE		TargetProcessHandle,
		   OUT	PHANDLE		TargetHandle,
		   IN	ACCESS_MASK	DesiredAccess,
		   IN	BOOLEAN		InheritHandle,
		   ULONG		Options)
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
 *	   InheritHandle = Indicates wheter the new handle will be inheritable
 *                         or not.
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
   HANDLE hTarget;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PAGED_CODE();
   
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(TargetHandle,
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

   /* Check for magic handle first */
   if (SourceHandle == NtCurrentThread() ||
       SourceHandle == NtCurrentProcess())
     {
       PVOID ObjectBody;
       
       Status = ObReferenceObjectByHandle(SourceHandle,
                                          PROCESS_DUP_HANDLE,
                                          NULL,
                                          PreviousMode,
                                          &ObjectBody,
                                          NULL);
       if(NT_SUCCESS(Status))
       {
         Status = ObCreateHandle(TargetProcess,
                                 ObjectBody,
                                 THREAD_ALL_ACCESS,
                                 InheritHandle,
                                 &hTarget);

         ObDereferenceObject(ObjectBody);
         
         if (Options & DUPLICATE_CLOSE_SOURCE)
         {
           ObDeleteHandle(SourceProcess,
                          SourceHandle);
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
                                  InheritHandle,
                                  Options);
     }

   ObDereferenceObject(TargetProcess);
   ObDereferenceObject(SourceProcess);

   if(NT_SUCCESS(Status))
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
DeleteHandleCallback(PHANDLE_TABLE HandleTable,
                     PVOID Object,
                     ULONG GrantedAccess,
                     PVOID Context)
{
  POBJECT_HEADER ObjectHeader;
  PVOID ObjectBody;
  
  PAGED_CODE();

  ObjectHeader = EX_OBJ_TO_HDR(Object);
  ObjectBody = HEADER_TO_BODY(ObjectHeader);
  
  ObpDecrementHandleCount(ObjectBody);
}

VOID ObDeleteHandleTable(PEPROCESS Process)
/*
 * FUNCTION: Deletes the handle table associated with a process
 */
{
   PAGED_CODE();

   ExDestroyHandleTable(Process->ObjectTable,
                        DeleteHandleCallback,
                        Process);
}

static BOOLEAN STDCALL
DuplicateHandleCallback(PHANDLE_TABLE HandleTable,
                        PHANDLE_TABLE_ENTRY HandleTableEntry,
                        PVOID Context)
{
  POBJECT_HEADER ObjectHeader;
  BOOLEAN Ret = FALSE;
  
  PAGED_CODE();
  
  Ret = (HandleTableEntry->u1.ObAttributes & EX_HANDLE_ENTRY_INHERITABLE) != 0;
  if(Ret)
  {
    ObjectHeader = EX_HTE_TO_HDR(HandleTableEntry);
    if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
    {
      ObReferenceObjectByPointer(HEADER_TO_BODY(ObjectHeader),
			         0,
			         NULL,
			         UserMode);
    }
  }
  
  return Ret;
}

VOID ObCreateHandleTable(PEPROCESS Parent,
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


NTSTATUS
ObDeleteHandle(PEPROCESS Process,
	       HANDLE Handle)
{
   PHANDLE_TABLE_ENTRY HandleEntry;
   PVOID Body;
   POBJECT_HEADER ObjectHeader;
   LONG ExHandle = HANDLE_TO_EX_HANDLE(Handle);
   
   PAGED_CODE();

   DPRINT("ObDeleteHandle(Handle %x)\n",Handle);
   
   KeEnterCriticalRegion();
   
   HandleEntry = ExMapHandleToPointer(Process->ObjectTable,
                                      ExHandle);
   if(HandleEntry != NULL)
   {
     if(HandleEntry->u1.ObAttributes & EX_HANDLE_ENTRY_PROTECTFROMCLOSE)
     {
       ExUnlockHandleTableEntry(Process->ObjectTable,
                                HandleEntry);

       KeLeaveCriticalRegion();

       return STATUS_HANDLE_NOT_CLOSABLE;
     }

     ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
     Body = HEADER_TO_BODY(ObjectHeader);
     
     ObpDecrementHandleCount(Body);
     
     /* destroy and unlock the handle entry */
     ExDestroyHandleByEntry(Process->ObjectTable,
                            HandleEntry,
                            ExHandle);
     
     KeLeaveCriticalRegion();
     
     return STATUS_SUCCESS;
   }
   KeLeaveCriticalRegion();
   return STATUS_INVALID_HANDLE;
}


NTSTATUS
ObCreateHandle(PEPROCESS Process,
	       PVOID ObjectBody,
	       ACCESS_MASK GrantedAccess,
	       BOOLEAN Inherit,
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
   POBJECT_HEADER ObjectHeader;
   LONG ExHandle;
   
   PAGED_CODE();

   DPRINT("ObCreateHandle(Process %x, obj %x)\n",Process,ObjectBody);

   ASSERT(Process);
   ASSERT(ObjectBody);

   ObjectHeader = BODY_TO_HEADER(ObjectBody);

   ASSERT((ULONG_PTR)ObjectHeader & EX_HANDLE_ENTRY_LOCKED);
   
   if (GrantedAccess & MAXIMUM_ALLOWED)
     {
        GrantedAccess &= ~MAXIMUM_ALLOWED;
        GrantedAccess |= GENERIC_ALL;
     }

   if (GrantedAccess & GENERIC_ANY)
     {
       RtlMapGenericMask(&GrantedAccess,
		         ObjectHeader->ObjectType->Mapping);
     }

   NewEntry.u1.Object = ObjectHeader;
   if(Inherit)
     NewEntry.u1.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
   else
     NewEntry.u1.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
   NewEntry.u2.GrantedAccess = GrantedAccess;
   
   ExHandle = ExCreateHandle(Process->ObjectTable,
                             &NewEntry);
   DPRINT("ObCreateHandle(0x%x)==0x%x [HT:0x%x]\n", ObjectHeader, *HandleReturn, Process->ObjectTable);
   if(ExHandle != EX_INVALID_HANDLE)
   {
     if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
     {
      ObReferenceObjectByPointer(ObjectBody,
			         0,
			         NULL,
			         UserMode);
     }
     
     *HandleReturn = EX_HANDLE_TO_HANDLE(ExHandle);

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
  PEPROCESS Process;
  LONG ExHandle = HANDLE_TO_EX_HANDLE(Handle);
  
  PAGED_CODE();

  DPRINT("ObQueryObjectAuditingByHandle(Handle %x)\n", Handle);

  Process = PsGetCurrentProcess();
  
  KeEnterCriticalRegion();

  HandleEntry = ExMapHandleToPointer(Process->ObjectTable,
                                     ExHandle);
  if(HandleEntry != NULL)
  {
    *GenerateOnClose = (HandleEntry->u1.ObAttributes & EX_HANDLE_ENTRY_AUDITONCLOSE) != 0;
    
    ExUnlockHandleTableEntry(Process->ObjectTable,
                             HandleEntry);

    KeLeaveCriticalRegion();
    
    return STATUS_SUCCESS;
  }
  
  KeLeaveCriticalRegion();
  
  return STATUS_INVALID_HANDLE;
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
   POBJECT_HEADER ObjectHeader;
   PVOID ObjectBody;
   ACCESS_MASK GrantedAccess;
   ULONG Attributes;
   NTSTATUS Status;
   LONG ExHandle = HANDLE_TO_EX_HANDLE(Handle);
   
   PAGED_CODE();
   
   DPRINT("ObReferenceObjectByHandle(Handle %x, DesiredAccess %x, "
	   "ObjectType %x, AccessMode %d, Object %x)\n",Handle,DesiredAccess,
	   ObjectType,AccessMode,Object);

   /*
    * Handle special handle names
    */
   if (Handle == NtCurrentProcess() && 
       (ObjectType == PsProcessType || ObjectType == NULL))
     {
	Status = ObReferenceObjectByPointer(PsGetCurrentProcess(),
	                                    PROCESS_ALL_ACCESS,
	                                    PsProcessType,
	                                    UserMode);
	if (! NT_SUCCESS(Status))
	  {
	    return Status;
	  }

	if (HandleInformation != NULL)
	  {
	    HandleInformation->HandleAttributes = 0;
	    HandleInformation->GrantedAccess = PROCESS_ALL_ACCESS;
	  }

	*Object = PsGetCurrentProcess();
	DPRINT("Referencing current process %x\n", PsGetCurrentProcess());
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
	Status = ObReferenceObjectByPointer(PsGetCurrentThread(),
	                                    THREAD_ALL_ACCESS,
	                                    PsThreadType,
	                                    UserMode);
	if (! NT_SUCCESS(Status))
	  {
	    return Status;
	  }

	if (HandleInformation != NULL)
	  {
	    HandleInformation->HandleAttributes = 0;
	    HandleInformation->GrantedAccess = THREAD_ALL_ACCESS;
	  }

	*Object = PsGetCurrentThread();
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
   
   KeEnterCriticalRegion();
   
   HandleEntry = ExMapHandleToPointer(PsGetCurrentProcess()->ObjectTable,
				      ExHandle);
   if (HandleEntry == NULL)
     {
        KeLeaveCriticalRegion();
        DPRINT("ExMapHandleToPointer() failed for handle 0x%x\n", Handle);
        return(STATUS_INVALID_HANDLE);
     }

   ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
   ObjectBody = HEADER_TO_BODY(ObjectHeader);
   
   DPRINT("locked1: ObjectHeader: 0x%x [HT:0x%x]\n", ObjectHeader, PsGetCurrentProcess()->ObjectTable);
   
   if (ObjectType != NULL && ObjectType != ObjectHeader->ObjectType)
     {
        DPRINT("ObjectType mismatch: %wZ vs %wZ (handle 0x%x)\n", &ObjectType->TypeName, ObjectHeader->ObjectType ? &ObjectHeader->ObjectType->TypeName : NULL, Handle);

        ExUnlockHandleTableEntry(PsGetCurrentProcess()->ObjectTable,
                                 HandleEntry);

        KeLeaveCriticalRegion();

        return(STATUS_OBJECT_TYPE_MISMATCH);
     }

   /* map the generic access masks if the caller asks for generic access */
   if (DesiredAccess & GENERIC_ANY)
     {
        RtlMapGenericMask(&DesiredAccess,
                          BODY_TO_HEADER(ObjectBody)->ObjectType->Mapping);
     }
   
   GrantedAccess = HandleEntry->u2.GrantedAccess;
   
   /* Unless running as KernelMode, deny access if caller desires more access
      rights than the handle can grant */
   if(AccessMode != KernelMode && (~GrantedAccess & DesiredAccess))
     {
        ExUnlockHandleTableEntry(PsGetCurrentProcess()->ObjectTable,
                                 HandleEntry);

        KeLeaveCriticalRegion();

        return(STATUS_ACCESS_DENIED);
     }

   ObReferenceObjectByPointer(ObjectBody,
			      0,
			      NULL,
			      UserMode);
   Attributes = HandleEntry->u1.ObAttributes & (EX_HANDLE_ENTRY_PROTECTFROMCLOSE |
                                                EX_HANDLE_ENTRY_INHERITABLE |
                                                EX_HANDLE_ENTRY_AUDITONCLOSE);

   ExUnlockHandleTableEntry(PsGetCurrentProcess()->ObjectTable,
                            HandleEntry);
   
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
   NTSTATUS Status;
   
   PAGED_CODE();
   
   Status = ObDeleteHandle(PsGetCurrentProcess(),
			   Handle);
   if (!NT_SUCCESS(Status))
     {
        if(((PEPROCESS)(KeGetCurrentThread()->ApcState.Process))->ExceptionPort)
           KeRaiseUserException(Status);
	return Status;
     }
   
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ObInsertObject(IN PVOID Object,
	       IN PACCESS_STATE PassedAccessState OPTIONAL,
	       IN ACCESS_MASK DesiredAccess,
	       IN ULONG AdditionalReferences,
	       OUT PVOID* ReferencedObject OPTIONAL,
	       OUT PHANDLE Handle)
{
  POBJECT_HEADER ObjectHeader;
  ACCESS_MASK Access;
  
  PAGED_CODE();

  Access = DesiredAccess;
  ObjectHeader = BODY_TO_HEADER(Object);

  return(ObCreateHandle(PsGetCurrentProcess(),
			Object,
			Access,
			ObjectHeader->Inherit,
			Handle));
}


ULONG
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
  UNIMPLEMENTED;
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
//      pshi->ObjectTypeIndex;
//      pshi->HandleAttributes;

//      KeReleaseSpinLock( &Process->HandleTable.ListLock, oldIrql );

      return;
}



/* EOF */
