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
/* $Id: handle.c,v 1.56 2004/05/04 20:18:52 jimtabor Exp $
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

#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <internal/safe.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/*
 * PURPOSE: Defines a handle
 */
typedef struct
{
   PVOID ObjectBody;
   ACCESS_MASK GrantedAccess;
} HANDLE_REP, *PHANDLE_REP;

#define HANDLE_BLOCK_ENTRIES ((PAGE_SIZE-sizeof(LIST_ENTRY))/sizeof(HANDLE_REP))

#define OB_HANDLE_FLAG_MASK    0x00000007
#define OB_HANDLE_FLAG_AUDIT   0x00000004
#define OB_HANDLE_FLAG_PROTECT 0x00000002
#define OB_HANDLE_FLAG_INHERIT 0x00000001

#define OB_POINTER_TO_ENTRY(Pointer) \
  (PVOID)((ULONG_PTR)(Pointer) & ~OB_HANDLE_FLAG_MASK)

#define OB_ENTRY_TO_POINTER(Entry) \
  (PVOID)((ULONG_PTR)(Entry) & ~OB_HANDLE_FLAG_MASK)

/*
 * PURPOSE: Defines a page's worth of handles
 */
typedef struct
{
   LIST_ENTRY entry;
   HANDLE_REP handles[HANDLE_BLOCK_ENTRIES];
} HANDLE_BLOCK, *PHANDLE_BLOCK;


/* GLOBALS *******************************************************************/

#define TAG_HANDLE_TABLE    TAG('H', 'T', 'B', 'L')

/* FUNCTIONS ***************************************************************/


static PHANDLE_REP ObpGetObjectByHandle(PHANDLE_TABLE HandleTable, HANDLE h)
/*
 * FUNCTION: Get the data structure for a handle
 * ARGUMENTS:
 *         Process = Process to get the handle for
 *         h = Handle
 * ARGUMENTS: A pointer to the information about the handle on success,
 *            NULL on failure
 */
{
   PLIST_ENTRY current;
   unsigned int handle = (((unsigned int)h) >> 2) - 1;
   unsigned int count=handle/HANDLE_BLOCK_ENTRIES;
   HANDLE_BLOCK* blk = NULL;
   unsigned int i;
   
   DPRINT("ObpGetObjectByHandle(HandleTable %x, h %x)\n",HandleTable,h);
   
   current = HandleTable->ListHead.Flink;
   DPRINT("current %x\n",current);
   
   for (i=0;i<count;i++)
     {
	current = current->Flink;
	if (current == (&(HandleTable->ListHead)))
	  {
	     return(NULL);
	  }
     }
   
   blk = CONTAINING_RECORD(current,HANDLE_BLOCK,entry);
   DPRINT("object: %p\n",&(blk->handles[handle%HANDLE_BLOCK_ENTRIES]));
   return(&(blk->handles[handle%HANDLE_BLOCK_ENTRIES]));
}


NTSTATUS
ObpQueryHandleAttributes(HANDLE Handle,
			 POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo)
{
  PEPROCESS Process;
  KIRQL oldIrql;
  PHANDLE_REP HandleRep;

  DPRINT("ObpQueryHandleAttributes(Handle %x)\n", Handle);

  Process = PsGetCurrentProcess();

  KeAcquireSpinLock(&Process->HandleTable.ListLock, &oldIrql);
  HandleRep = ObpGetObjectByHandle(&Process->HandleTable,
				   Handle);
  if (HandleRep == NULL)
    {
      KeReleaseSpinLock(&Process->HandleTable.ListLock, oldIrql);
      return STATUS_INVALID_HANDLE;
    }

  HandleInfo->Inherit =
    ((ULONG_PTR)HandleRep->ObjectBody & OB_HANDLE_FLAG_INHERIT);
  HandleInfo->ProtectFromClose =
    ((ULONG_PTR)HandleRep->ObjectBody & OB_HANDLE_FLAG_PROTECT);

  KeReleaseSpinLock(&Process->HandleTable.ListLock, oldIrql);

  return STATUS_SUCCESS;
}


NTSTATUS
ObpSetHandleAttributes(HANDLE Handle,
		       POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo)
{
  PEPROCESS Process;
  KIRQL oldIrql;
  PHANDLE_REP HandleRep;

  DPRINT("ObpQueryHandleAttributes(Handle %x)\n", Handle);

  Process = PsGetCurrentProcess();

  KeAcquireSpinLock(&Process->HandleTable.ListLock, &oldIrql);
  HandleRep = ObpGetObjectByHandle(&Process->HandleTable,
				   Handle);
  if (HandleRep == NULL)
    {
      KeReleaseSpinLock(&Process->HandleTable.ListLock, oldIrql);
      return STATUS_INVALID_HANDLE;
    }

  if (HandleInfo->Inherit)
    HandleRep->ObjectBody = (PVOID)((ULONG_PTR)HandleRep->ObjectBody | OB_HANDLE_FLAG_INHERIT);
  else
    HandleRep->ObjectBody = (PVOID)((ULONG_PTR)HandleRep->ObjectBody & ~OB_HANDLE_FLAG_INHERIT);

  if (HandleInfo->ProtectFromClose)
    HandleRep->ObjectBody = (PVOID)((ULONG_PTR)HandleRep->ObjectBody | OB_HANDLE_FLAG_PROTECT);
  else
    HandleRep->ObjectBody = (PVOID)((ULONG_PTR)HandleRep->ObjectBody & ~OB_HANDLE_FLAG_PROTECT);

  /* FIXME: Do we need to set anything in the object header??? */

  KeReleaseSpinLock(&Process->HandleTable.ListLock, oldIrql);

  return STATUS_SUCCESS;
}


NTSTATUS
ObDuplicateObject(PEPROCESS SourceProcess,
		  PEPROCESS TargetProcess,
		  HANDLE SourceHandle,
		  PHANDLE TargetHandle,
		  ACCESS_MASK DesiredAccess,
		  BOOLEAN InheritHandle,
		  ULONG	Options)
{
  KIRQL oldIrql;
  PHANDLE_REP SourceHandleRep;
  PVOID ObjectBody;

  KeAcquireSpinLock(&SourceProcess->HandleTable.ListLock, &oldIrql);
  SourceHandleRep = ObpGetObjectByHandle(&SourceProcess->HandleTable,
					 SourceHandle);
  if (SourceHandleRep == NULL)
    {
      KeReleaseSpinLock(&SourceProcess->HandleTable.ListLock, oldIrql);
      return(STATUS_INVALID_HANDLE);
    }
  ObjectBody = OB_ENTRY_TO_POINTER(SourceHandleRep->ObjectBody);
  ObReferenceObjectByPointer(ObjectBody,
			     0,
			     NULL,
			     UserMode);
  
  if (Options & DUPLICATE_SAME_ACCESS)
    {
      DesiredAccess = SourceHandleRep->GrantedAccess;
    }
  
  KeReleaseSpinLock(&SourceProcess->HandleTable.ListLock, oldIrql);
  ObCreateHandle(TargetProcess,
		 ObjectBody,
		 DesiredAccess,
		 InheritHandle,
		 TargetHandle);
  
  if (Options & DUPLICATE_CLOSE_SOURCE)
    {
      ZwClose(SourceHandle);
    }
  
  ObDereferenceObject(ObjectBody);
  return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS STDCALL 
NtDuplicateObject (IN	HANDLE		SourceProcessHandle,
		   IN	HANDLE		SourceHandle,
		   IN	HANDLE		TargetProcessHandle,
		   OUT	PHANDLE		UnsafeTargetHandle,
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
   PHANDLE_REP SourceHandleRep;
   KIRQL oldIrql;
   PVOID ObjectBody;
   HANDLE TargetHandle;
   NTSTATUS Status;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   Status = ObReferenceObjectByHandle(SourceProcessHandle,
				      PROCESS_DUP_HANDLE,
				      NULL,
				      UserMode,
				      (PVOID*)&SourceProcess,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
       return(Status);
     }
   Status = ObReferenceObjectByHandle(TargetProcessHandle,
				      PROCESS_DUP_HANDLE,
				      NULL,
				      UserMode,
				      (PVOID*)&TargetProcess,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
       ObDereferenceObject(SourceProcess);
       return(Status);
     }

   /* Check for magic handle first */
   if (SourceHandle == NtCurrentThread())
     {
       ObReferenceObjectByHandle(SourceHandle,
                                 PROCESS_DUP_HANDLE,
                                 NULL,
                                 UserMode,
                                 &ObjectBody,
                                 NULL);

       ObCreateHandle(TargetProcess,
                      ObjectBody,
                      THREAD_ALL_ACCESS,
                      InheritHandle,
                      &TargetHandle);
     }
   else
     {
       KeAcquireSpinLock(&SourceProcess->HandleTable.ListLock, &oldIrql);
       SourceHandleRep = ObpGetObjectByHandle(&SourceProcess->HandleTable,
					      SourceHandle);
       if (SourceHandleRep == NULL)
	 {
	   KeReleaseSpinLock(&SourceProcess->HandleTable.ListLock, oldIrql);
	   ObDereferenceObject(SourceProcess);
	   ObDereferenceObject(TargetProcess);
	   return(STATUS_INVALID_HANDLE);
	 }
       ObjectBody = OB_ENTRY_TO_POINTER(SourceHandleRep->ObjectBody);
       ObReferenceObjectByPointer(ObjectBody,
			          0,
			          NULL,
			          UserMode);
   
       if (Options & DUPLICATE_SAME_ACCESS)
	 {
	   DesiredAccess = SourceHandleRep->GrantedAccess;
	 }
   
       KeReleaseSpinLock(&SourceProcess->HandleTable.ListLock, oldIrql);
       if (!((ULONG_PTR)SourceHandleRep->ObjectBody & OB_HANDLE_FLAG_INHERIT))
         {
	   ObDereferenceObject(TargetProcess);
	   ObDereferenceObject(SourceProcess);
	   ObDereferenceObject(ObjectBody);
	   return STATUS_INVALID_HANDLE;
	 }
       ObCreateHandle(TargetProcess,
		      ObjectBody,
		      DesiredAccess,
		      InheritHandle,
		      &TargetHandle);
     }
   
   if (Options & DUPLICATE_CLOSE_SOURCE)
     {
	ZwClose(SourceHandle);
     }
   
   ObDereferenceObject(TargetProcess);
   ObDereferenceObject(SourceProcess);
   ObDereferenceObject(ObjectBody);
   
   Status = MmCopyToCaller(UnsafeTargetHandle, &TargetHandle, sizeof(HANDLE));
   if (!NT_SUCCESS(Status))
     {
       return(Status);
     }

   return(STATUS_SUCCESS);
}

VOID ObCloseAllHandles(PEPROCESS Process)
{
   KIRQL oldIrql;
   PHANDLE_TABLE HandleTable;
   PLIST_ENTRY current_entry;
   PHANDLE_BLOCK current;
   ULONG i;
   PVOID ObjectBody;
   
   DPRINT("ObCloseAllHandles(Process %x)\n", Process);
   
   HandleTable = &Process->HandleTable;
   
   KeAcquireSpinLock(&HandleTable->ListLock, &oldIrql);
   
   current_entry = HandleTable->ListHead.Flink;
   
   while (current_entry != &HandleTable->ListHead)
     {
	current = CONTAINING_RECORD(current_entry, HANDLE_BLOCK, entry);
	
	for (i = 0; i < HANDLE_BLOCK_ENTRIES; i++)
	  {
	     ObjectBody = OB_ENTRY_TO_POINTER(current->handles[i].ObjectBody);
	     
	     if (ObjectBody != NULL)
	       {
		  POBJECT_HEADER Header = BODY_TO_HEADER(ObjectBody);
		  
		  if (Header->ObjectType == PsProcessType ||
		      Header->ObjectType == PsThreadType)
		    {
		       DPRINT("Deleting handle to %x\n", ObjectBody);
		    }
		  
		  ObReferenceObjectByPointer(ObjectBody,
					     0,
					     NULL,
					     UserMode);
		  InterlockedDecrement(&Header->HandleCount);
		  current->handles[i].ObjectBody = NULL;
		  
		  KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);
		  if ((Header->ObjectType != NULL) &&
		      (Header->ObjectType->Close != NULL))
		    {
		       Header->ObjectType->Close(ObjectBody,
						 Header->HandleCount);
		    }
		  
		  ObDereferenceObject(ObjectBody);
		  KeAcquireSpinLock(&HandleTable->ListLock, &oldIrql);
		  current_entry = &HandleTable->ListHead;
		  break;
	       }
	  }
	
	current_entry = current_entry->Flink;
     }
   KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);
   DPRINT("ObCloseAllHandles() finished\n");
   DPRINT("Type %x\n", BODY_TO_HEADER(Process)->ObjectType);
}

VOID ObDeleteHandleTable(PEPROCESS Process)
/*
 * FUNCTION: Deletes the handle table associated with a process
 */
{
   PLIST_ENTRY current = NULL;
   PHANDLE_TABLE HandleTable = NULL;
   
   ObCloseAllHandles(Process);
   
   HandleTable = &Process->HandleTable;
   current = RemoveHeadList(&HandleTable->ListHead);
   
   while (current != &HandleTable->ListHead)
     {
	HANDLE_BLOCK* HandleBlock = CONTAINING_RECORD(current,
						      HANDLE_BLOCK,
						      entry);
	DPRINT("Freeing %x\n", HandleBlock);
	ExFreePool(HandleBlock);
	
	current = RemoveHeadList(&HandleTable->ListHead);
     }
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
   PHANDLE_TABLE ParentHandleTable, HandleTable;
   KIRQL oldIrql;
   PLIST_ENTRY parent_current;
   ULONG i;
   PHANDLE_BLOCK current_block, new_block;   

   DPRINT("ObCreateHandleTable(Parent %x, Inherit %d, Process %x)\n",
	  Parent,Inherit,Process);
   
   InitializeListHead(&(Process->HandleTable.ListHead));
   KeInitializeSpinLock(&(Process->HandleTable.ListLock));
   
   if (Parent != NULL)
     {
	ParentHandleTable = &Parent->HandleTable;
	HandleTable = &Process->HandleTable;

	KeAcquireSpinLock(&Parent->HandleTable.ListLock, &oldIrql);
	KeAcquireSpinLockAtDpcLevel(&Process->HandleTable.ListLock);
	
	parent_current = ParentHandleTable->ListHead.Flink;
	
	while (parent_current != &ParentHandleTable->ListHead)
	  {
	     current_block = CONTAINING_RECORD(parent_current,
					       HANDLE_BLOCK,
					       entry);
	     new_block = ExAllocatePoolWithTag(NonPagedPool,
		                               sizeof(HANDLE_BLOCK),
					       TAG_HANDLE_TABLE);
	     if (new_block == NULL)
	     {
		break;
	     }
	     RtlZeroMemory(new_block, sizeof(HANDLE_BLOCK));

	     for (i=0; i<HANDLE_BLOCK_ENTRIES; i++)
	     {
		if (current_block->handles[i].ObjectBody)
		{
		   if ((ULONG_PTR)current_block->handles[i].ObjectBody & OB_HANDLE_FLAG_INHERIT)
		   {
		      new_block->handles[i].ObjectBody = 
			current_block->handles[i].ObjectBody;
		      new_block->handles[i].GrantedAccess = 
			current_block->handles[i].GrantedAccess;
		      InterlockedIncrement(&(BODY_TO_HEADER(OB_ENTRY_TO_POINTER(current_block->handles[i].ObjectBody))->HandleCount));
		   }
		}
	     }
	     InsertTailList(&Process->HandleTable.ListHead, &new_block->entry);
	     parent_current = parent_current->Flink;
	  }
	KeReleaseSpinLockFromDpcLevel(&Process->HandleTable.ListLock);
	KeReleaseSpinLock(&Parent->HandleTable.ListLock, oldIrql);
     }
}


NTSTATUS
ObDeleteHandle(PEPROCESS Process,
	       HANDLE Handle,
	       PVOID *ObjectBody)
{
   PHANDLE_REP Rep;
   PVOID Body;
   KIRQL oldIrql;
   PHANDLE_TABLE HandleTable;
   POBJECT_HEADER Header;

   DPRINT("ObDeleteHandle(Handle %x)\n",Handle);

   HandleTable = &Process->HandleTable;

   KeAcquireSpinLock(&HandleTable->ListLock, &oldIrql);

   Rep = ObpGetObjectByHandle(HandleTable, Handle);
   if (Rep == NULL)
     {
	KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);
	*ObjectBody = NULL;
	return STATUS_INVALID_HANDLE;
     }

   if ((ULONG_PTR)Rep->ObjectBody & OB_HANDLE_FLAG_PROTECT)
     {
	KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);
	*ObjectBody = NULL;
	return STATUS_HANDLE_NOT_CLOSABLE;
     }

   Body = OB_ENTRY_TO_POINTER(Rep->ObjectBody);
   DPRINT("ObjectBody %x\n", Body);
   if (Body == NULL)
     {
	KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);
	*ObjectBody = NULL;
	return STATUS_UNSUCCESSFUL;
     }

   Header = BODY_TO_HEADER(Body);
   ObReferenceObjectByPointer(Body,
			      0,
			      NULL,
			      UserMode);
   InterlockedDecrement(&Header->HandleCount);
   Rep->ObjectBody = NULL;

   KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);

   if ((Header->ObjectType != NULL) &&
       (Header->ObjectType->Close != NULL))
     {
	Header->ObjectType->Close(Body, Header->HandleCount);
     }

   *ObjectBody = Body;

   DPRINT("Finished ObDeleteHandle()\n");

   return STATUS_SUCCESS;
}


NTSTATUS ObCreateHandle(PEPROCESS Process,
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
   LIST_ENTRY* current;
   unsigned int handle=1;
   unsigned int i;
   HANDLE_BLOCK* new_blk = NULL;
   PHANDLE_TABLE HandleTable;
   KIRQL oldlvl;

   DPRINT("ObCreateHandle(Process %x, obj %x)\n",Process,ObjectBody);

   assert(Process);

   if (ObjectBody != NULL)
     {
	InterlockedIncrement(&(BODY_TO_HEADER(ObjectBody)->HandleCount));
     }
   HandleTable = &Process->HandleTable;
   KeAcquireSpinLock(&HandleTable->ListLock, &oldlvl);
   current = HandleTable->ListHead.Flink;
   /*
    * Scan through the currently allocated handle blocks looking for a free
    * slot
    */
   while (current != (&HandleTable->ListHead))
     {
	HANDLE_BLOCK* blk = CONTAINING_RECORD(current,HANDLE_BLOCK,entry);

	DPRINT("Current %x\n",current);

	for (i=0;i<HANDLE_BLOCK_ENTRIES;i++)
	  {
	     DPRINT("Considering slot %d containing %x\n",i,blk->handles[i]);
	     if (blk->handles[i].ObjectBody == NULL)
	       {
		  blk->handles[i].ObjectBody = OB_POINTER_TO_ENTRY(ObjectBody);
		  if (Inherit)
		    blk->handles[i].ObjectBody = (PVOID)((ULONG_PTR)blk->handles[i].ObjectBody | OB_HANDLE_FLAG_INHERIT);
		  blk->handles[i].GrantedAccess = GrantedAccess;
		  KeReleaseSpinLock(&HandleTable->ListLock, oldlvl);
		  *HandleReturn = (HANDLE)((handle + i) << 2);
		  return(STATUS_SUCCESS);
	       }
	  }
	
	handle = handle + HANDLE_BLOCK_ENTRIES;
	current = current->Flink;
     }

   /*
    * Add a new handle block to the end of the list
    */
   new_blk = 
     (HANDLE_BLOCK *)ExAllocatePoolWithTag(NonPagedPool,sizeof(HANDLE_BLOCK),
					   TAG_HANDLE_TABLE);
   if (!new_blk)
    {
      *HandleReturn = (PHANDLE)NULL;
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
   RtlZeroMemory(new_blk,sizeof(HANDLE_BLOCK));
   InsertTailList(&(Process->HandleTable.ListHead),
		  &new_blk->entry);
   new_blk->handles[0].ObjectBody = OB_POINTER_TO_ENTRY(ObjectBody);
   if (Inherit)
     new_blk->handles[0].ObjectBody = (PVOID)((ULONG_PTR)new_blk->handles[0].ObjectBody | OB_HANDLE_FLAG_INHERIT);
   new_blk->handles[0].GrantedAccess = GrantedAccess;
   KeReleaseSpinLock(&HandleTable->ListLock, oldlvl);
   *HandleReturn = (HANDLE)(handle << 2);
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ObQueryObjectAuditingByHandle(IN HANDLE Handle,
			      OUT PBOOLEAN GenerateOnClose)
{
  PEPROCESS Process;
  KIRQL oldIrql;
  PHANDLE_REP HandleRep;

  DPRINT("ObQueryObjectAuditingByHandle(Handle %x)\n", Handle);

  Process = PsGetCurrentProcess();

  KeAcquireSpinLock(&Process->HandleTable.ListLock, &oldIrql);
  HandleRep = ObpGetObjectByHandle(&Process->HandleTable,
				   Handle);
  if (HandleRep == NULL)
    {
      KeReleaseSpinLock(&Process->HandleTable.ListLock, oldIrql);
      return STATUS_INVALID_HANDLE;
    }

  *GenerateOnClose = (BOOLEAN)((ULONG_PTR)HandleRep->ObjectBody | OB_HANDLE_FLAG_AUDIT);

  KeReleaseSpinLock(&Process->HandleTable.ListLock, oldIrql);

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ObReferenceObjectByHandle(HANDLE Handle,
			  ACCESS_MASK DesiredAccess,
			  POBJECT_TYPE ObjectType,
			  KPROCESSOR_MODE AccessMode,
			  PVOID* Object,
			  POBJECT_HANDLE_INFORMATION HandleInformation)
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
 */
{
   PHANDLE_REP HandleRep;
   POBJECT_HEADER ObjectHeader;
   KIRQL oldIrql;
   PVOID ObjectBody;
   ACCESS_MASK GrantedAccess;
   ULONG Attributes;
   NTSTATUS Status;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   DPRINT("ObReferenceObjectByHandle(Handle %x, DesiredAccess %x, "
	   "ObjectType %x, AccessMode %d, Object %x)\n",Handle,DesiredAccess,
	   ObjectType,AccessMode,Object);

   /*
    * Handle special handle names
    */
   if (Handle == NtCurrentProcess() && 
       (ObjectType == PsProcessType || ObjectType == NULL))
     {
	DPRINT("Reference from %x\n", ((PULONG)&Handle)[-1]);
	
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
	    HandleInformation->HandleAttributes = 0; /* FIXME? */
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
	    HandleInformation->HandleAttributes = 0; /* FIXME? */
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
   
   KeAcquireSpinLock(&PsGetCurrentProcess()->HandleTable.ListLock,
		     &oldIrql);
   HandleRep = ObpGetObjectByHandle(&PsGetCurrentProcess()->HandleTable,
				    Handle);
   if (HandleRep == NULL || HandleRep->ObjectBody == 0)
     {
	KeReleaseSpinLock(&PsGetCurrentProcess()->HandleTable.ListLock,
			  oldIrql);
	return(STATUS_INVALID_HANDLE);
     }
   ObjectBody = OB_ENTRY_TO_POINTER(HandleRep->ObjectBody);
   DPRINT("ObjectBody %p\n",ObjectBody);
   ObjectHeader = BODY_TO_HEADER(ObjectBody);
   DPRINT("ObjectHeader->RefCount %lu\n",ObjectHeader->RefCount);
   ObReferenceObjectByPointer(ObjectBody,
			      0,
			      NULL,
			      UserMode);
   Attributes = (ULONG_PTR)HandleRep->ObjectBody & OB_HANDLE_FLAG_MASK;
   GrantedAccess = HandleRep->GrantedAccess;
   KeReleaseSpinLock(&PsGetCurrentProcess()->HandleTable.ListLock,
		     oldIrql);
   
   ObjectHeader = BODY_TO_HEADER(ObjectBody);
   DPRINT("ObjectHeader->RefCount %lu\n",ObjectHeader->RefCount);

   if (ObjectType != NULL && ObjectType != ObjectHeader->ObjectType)
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }
   
   if (ObjectHeader->ObjectType == PsProcessType)
     {
	DPRINT("Reference from %x\n", ((PULONG)&Handle)[-1]);
     }
   
   if (DesiredAccess && AccessMode == UserMode)
     {
	RtlMapGenericMask(&DesiredAccess, ObjectHeader->ObjectType->Mapping);

	if (!(GrantedAccess & DesiredAccess) &&
	    !((~GrantedAccess) & DesiredAccess))
	  {
	     CHECKPOINT;
	     return(STATUS_ACCESS_DENIED);
	  }
     }

   if (HandleInformation != NULL)
     {
	HandleInformation->HandleAttributes = Attributes;
	HandleInformation->GrantedAccess = GrantedAccess;
     }

   *Object = ObjectBody;
   
   CHECKPOINT;
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
   PVOID		ObjectBody;
   POBJECT_HEADER	Header;
   NTSTATUS Status;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   DPRINT("NtClose(Handle %x)\n",Handle);
   
   Status = ObDeleteHandle(PsGetCurrentProcess(),
			   Handle,
			   &ObjectBody);
   if (!NT_SUCCESS(Status))
     {
        if(((PEPROCESS)(KeGetCurrentThread()->ApcState.Process))->ExceptionPort)
           KeRaiseUserException(Status);
	return Status;
     }
   
   Header = BODY_TO_HEADER(ObjectBody);
   
   DPRINT("Dereferencing %x\n", ObjectBody);
   ObDereferenceObject(ObjectBody);
   
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

  Access = DesiredAccess;
  ObjectHeader = BODY_TO_HEADER(Object);

  RtlMapGenericMask(&Access,
		    ObjectHeader->ObjectType->Mapping);

  return(ObCreateHandle(PsGetCurrentProcess(),
			Object,
			Access,
			ObjectHeader->Inherit,
			Handle));
}


ULONG 
ObpGetHandleCountbyHandleTable(PHANDLE_TABLE HandleTable)
{
unsigned int i, Count=0;
PHANDLE_BLOCK blk;
POBJECT_HEADER Header;
PVOID ObjectBody;
KIRQL oldIrql;

PLIST_ENTRY current = HandleTable->ListHead.Flink;

  KeAcquireSpinLock(&HandleTable->ListLock, &oldIrql);

  while (current != &HandleTable->ListHead)
       {
	  blk = CONTAINING_RECORD(current, HANDLE_BLOCK, entry);

	  for ( i=0; i<HANDLE_BLOCK_ENTRIES; i++)
	     {
	         ObjectBody = OB_ENTRY_TO_POINTER(blk->handles[i].ObjectBody);
	     
	         if (ObjectBody != NULL)
	           {
		     Header = BODY_TO_HEADER(ObjectBody);
		     /* Make sure this is real. */
		     if (Header->ObjectType != NULL)
		         Count++;
		   }
	     }
          current = current->Flink;
       }
  KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);
  return (Count);	
}

/* EOF */
