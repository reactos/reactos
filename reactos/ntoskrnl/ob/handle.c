/* $Id: handle.c,v 1.22 2000/05/27 12:48:59 dwelch Exp $
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

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

#define HANDLE_BLOCK_ENTRIES ((PAGESIZE-sizeof(LIST_ENTRY))/sizeof(HANDLE_REP))

/*
 * PURPOSE: Defines a page's worth of handles
 */
typedef struct
{
   LIST_ENTRY entry;
   HANDLE_REP handles[HANDLE_BLOCK_ENTRIES];
} HANDLE_BLOCK, *PHANDLE_BLOCK;

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
   unsigned int handle = (((unsigned int)h) - 1) >> 2;
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
   return(&(blk->handles[handle%HANDLE_BLOCK_ENTRIES]));
}


NTSTATUS STDCALL NtDuplicateObject (IN	HANDLE		SourceProcessHandle,
				    IN	PHANDLE		SourceHandle,
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
   PHANDLE_REP SourceHandleRep;
   KIRQL oldIrql;
   PVOID ObjectBody;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   ObReferenceObjectByHandle(SourceProcessHandle,
			     PROCESS_DUP_HANDLE,
			     NULL,
			     UserMode,
			     (PVOID*)&SourceProcess,
			     NULL);
   ObReferenceObjectByHandle(TargetProcessHandle,
			     PROCESS_DUP_HANDLE,
			     NULL,
			     UserMode,
			     (PVOID*)&TargetProcess,
			     NULL);
   
   KeAcquireSpinLock(&SourceProcess->Pcb.HandleTable.ListLock, &oldIrql);
   SourceHandleRep = ObpGetObjectByHandle(&SourceProcess->Pcb.HandleTable,
					  *SourceHandle);
   if (SourceHandleRep == NULL)
     {
	KeReleaseSpinLock(&SourceProcess->Pcb.HandleTable.ListLock, oldIrql);
	ObDereferenceObject(SourceProcess);
	ObDereferenceObject(TargetProcess);
	return(STATUS_INVALID_HANDLE);
     }
   ObjectBody = SourceHandleRep->ObjectBody;
   ObReferenceObjectByPointer(ObjectBody,
			      GENERIC_ALL,
			      NULL,
			      UserMode);
   
   if (Options & DUPLICATE_SAME_ACCESS)
     {
	DesiredAccess = SourceHandleRep->GrantedAccess;
     }
   
   KeReleaseSpinLock(&SourceProcess->Pcb.HandleTable.ListLock, oldIrql);
   
   ObCreateHandle(TargetProcess,
		  ObjectBody,
		  DesiredAccess,
		  InheritHandle,
		  TargetHandle);
   
   if (Options & DUPLICATE_CLOSE_SOURCE)
     {
	ZwClose(*SourceHandle);
     }
   
   ObDereferenceObject(TargetProcess);
   ObDereferenceObject(SourceProcess);
   ObDereferenceObject(ObjectBody);
   
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
   
   HandleTable = &Process->Pcb.HandleTable;
   
   KeAcquireSpinLock(&HandleTable->ListLock, &oldIrql);
   
   current_entry = HandleTable->ListHead.Flink;
   
   while (current_entry != &HandleTable->ListHead)
     {
	current = CONTAINING_RECORD(current_entry, HANDLE_BLOCK, entry);
	
	for (i = 0; i < HANDLE_BLOCK_ENTRIES; i++)
	  {
	     ObjectBody = current->handles[i].ObjectBody;	   	     
	     
	     if (ObjectBody != NULL)
	       {
		  POBJECT_HEADER Header = BODY_TO_HEADER(ObjectBody);
		  
		  if (Header->ObjectType == PsProcessType ||
		      Header->ObjectType == PsThreadType)
		    {
		       DPRINT("Deleting handle to %x\n", ObjectBody);
		    }
		  
		  ObReferenceObjectByPointer(ObjectBody,
					     GENERIC_ALL,
					     NULL,
					     UserMode);
		  Header->HandleCount--;
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
   
   HandleTable = &Process->Pcb.HandleTable;
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
   PHANDLE_TABLE ParentHandleTable;
   KIRQL oldIrql;
   PLIST_ENTRY parent_current;
   ULONG i;
   
   DPRINT("ObCreateHandleTable(Parent %x, Inherit %d, Process %x)\n",
	  Parent,Inherit,Process);
   
   InitializeListHead(&(Process->Pcb.HandleTable.ListHead));
   KeInitializeSpinLock(&(Process->Pcb.HandleTable.ListLock));
   
   if (Parent != NULL)
     {
	ParentHandleTable = &Parent->Pcb.HandleTable;
	
	KeAcquireSpinLock(&Parent->Pcb.HandleTable.ListLock, &oldIrql);
	
	parent_current = ParentHandleTable->ListHead.Flink;
	
	while (parent_current != &ParentHandleTable->ListHead)
	  {
	     HANDLE_BLOCK* current_block = CONTAINING_RECORD(parent_current,
							     HANDLE_BLOCK,
							     entry);
	     HANDLE NewHandle;
	     
	     for (i=0; i<HANDLE_BLOCK_ENTRIES; i++)
	       {
		  if (Inherit || current_block->handles[i].Inherit)
		    {
		       ObCreateHandle(Process,
				      current_block->handles[i].ObjectBody,
				      current_block->handles[i].GrantedAccess,
				      current_block->handles[i].Inherit,
				      &NewHandle);
		    }
		  else
		    {
		       ObCreateHandle(Process,
				      NULL,
				      0,
				      current_block->handles[i].Inherit,
				      &NewHandle);
		    }
	       }
	     
	     parent_current = parent_current->Flink;
	  }
	
	KeReleaseSpinLock(&Parent->Pcb.HandleTable.ListLock, oldIrql);
     }
}


PVOID ObDeleteHandle(PEPROCESS Process, HANDLE Handle)
{
   PHANDLE_REP Rep;
   PVOID ObjectBody;
   KIRQL oldIrql;
   PHANDLE_TABLE HandleTable;
   POBJECT_HEADER Header;
   
   DPRINT("ObDeleteHandle(Handle %x)\n",Handle);
   
   HandleTable = &Process->Pcb.HandleTable;
   
   KeAcquireSpinLock(&HandleTable->ListLock, &oldIrql);
   
   Rep = ObpGetObjectByHandle(HandleTable, Handle);
   if (Rep == NULL)
     {
	KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);	
	return(NULL);
     }
   
   ObjectBody = Rep->ObjectBody;
   DPRINT("ObjectBody %x\n", ObjectBody);
   if (ObjectBody != NULL)
     {     
	Header = BODY_TO_HEADER(ObjectBody);
	BODY_TO_HEADER(ObjectBody)->HandleCount--;
	ObReferenceObjectByPointer(ObjectBody,
				   GENERIC_ALL,
				   NULL,
				   UserMode);
	Rep->ObjectBody = NULL;
     
	KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);
   
	if ((Header->ObjectType != NULL) &&
	    (Header->ObjectType->Close != NULL))
	  {
	     Header->ObjectType->Close(ObjectBody, Header->HandleCount);
	  }
     }
   else
     {
	KeReleaseSpinLock(&HandleTable->ListLock, oldIrql);	
     }
   
   DPRINT("Finished ObDeleteHandle()\n");
   return(ObjectBody);
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
 * NOTE: THe handle is valid only in the context of the current process
 */
{
   LIST_ENTRY* current;
   unsigned int handle=1;
   unsigned int i;
   HANDLE_BLOCK* new_blk = NULL;
   PHANDLE_TABLE HandleTable;
   KIRQL oldlvl;
   
   DPRINT("ObCreateHandle(Process %x, obj %x)\n",Process,ObjectBody);
   
   if (ObjectBody != NULL)
     {
	BODY_TO_HEADER(ObjectBody)->HandleCount++;
     }
   
   HandleTable = &Process->Pcb.HandleTable;
   
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
	     if (blk->handles[i].ObjectBody==NULL)
	       {
		  blk->handles[i].ObjectBody = ObjectBody;
		  blk->handles[i].GrantedAccess = GrantedAccess;
		  blk->handles[i].Inherit = Inherit;
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
   new_blk = (HANDLE_BLOCK *)ExAllocatePool(NonPagedPool,sizeof(HANDLE_BLOCK));
   memset(new_blk,0,sizeof(HANDLE_BLOCK));
   InsertTailList(&(Process->Pcb.HandleTable.ListHead),
		  &new_blk->entry);
   KeReleaseSpinLock(&HandleTable->ListLock, oldlvl);
   new_blk->handles[0].ObjectBody = ObjectBody;
   new_blk->handles[0].GrantedAccess = GrantedAccess;
   new_blk->handles[0].Inherit = Inherit;
   *HandleReturn = (HANDLE)(handle << 2);
   return(STATUS_SUCCESS);   
}


NTSTATUS STDCALL ObReferenceObjectByHandle(HANDLE Handle,
					   ACCESS_MASK DesiredAccess,
					   POBJECT_TYPE ObjectType,
					   KPROCESSOR_MODE AccessMode,
					   PVOID* Object,
					   POBJECT_HANDLE_INFORMATION 
					           HandleInformationPtr)
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
	
	ObReferenceObjectByPointer(PsGetCurrentProcess(),
				   PROCESS_ALL_ACCESS,
				   PsProcessType,
				   UserMode);
	*Object = PsGetCurrentProcess();
	DPRINT("Referencing current process %x\n", PsGetCurrentProcess());
	return(STATUS_SUCCESS);
     }
   else if (Handle == NtCurrentProcess())
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }
   if (Handle == NtCurrentThread() && 
       (ObjectType == PsThreadType || ObjectType == NULL))
     {
	ObReferenceObjectByPointer(PsGetCurrentThread(),
				   THREAD_ALL_ACCESS,
				   PsThreadType,
				   UserMode);
	*Object = PsGetCurrentThread();
	CHECKPOINT;
	return(STATUS_SUCCESS);
     }
   else if (Handle == NtCurrentThread())
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }
   
   KeAcquireSpinLock(&PsGetCurrentProcess()->Pcb.HandleTable.ListLock,
		     &oldIrql);
   HandleRep = ObpGetObjectByHandle(&PsGetCurrentProcess()->Pcb.HandleTable,
				    Handle);
   if (HandleRep == NULL || HandleRep->ObjectBody == NULL)
     {
	KeReleaseSpinLock(&PsGetCurrentProcess()->Pcb.HandleTable.ListLock,
			  oldIrql);
	return(STATUS_INVALID_HANDLE);
     }
   ObjectBody = HandleRep->ObjectBody;
   ObReferenceObjectByPointer(ObjectBody,
			      GENERIC_ALL,
			      NULL,
			      UserMode);
   GrantedAccess = HandleRep->GrantedAccess;
   KeReleaseSpinLock(&PsGetCurrentProcess()->Pcb.HandleTable.ListLock,
		     oldIrql);
   
   ObjectHeader = BODY_TO_HEADER(ObjectBody);

   if (ObjectType != NULL && ObjectType != ObjectHeader->ObjectType)
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }   
   
   if (ObjectHeader->ObjectType == PsProcessType)
     {
	DPRINT("Reference from %x\n", ((PULONG)&Handle)[-1]);
     }
   
   if (!(GrantedAccess & DesiredAccess) &&
       !((~GrantedAccess) & DesiredAccess))
     {
	CHECKPOINT;
	return(STATUS_ACCESS_DENIED);
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
 */
NTSTATUS STDCALL NtClose(HANDLE Handle)
{
   PVOID		ObjectBody;
   POBJECT_HEADER	Header;
   
   assert_irql(PASSIVE_LEVEL);
   
   DPRINT("NtClose(Handle %x)\n",Handle);
   
   ObjectBody = ObDeleteHandle(PsGetCurrentProcess(), Handle);
   if (ObjectBody == NULL)
     {
	return(STATUS_HANDLES_CLOSED);
     }
   
   Header = BODY_TO_HEADER(ObjectBody);
   
   DPRINT("Dereferencing %x\n", ObjectBody);
   ObDereferenceObject(ObjectBody);
   
   return(STATUS_SUCCESS);
}


/* EOF */
