/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               ntoskrnl/ob/handle.c
 * PURPOSE:            Managing handles
 * PROGRAMMER:         David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *                 17/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/string.h>

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
} HANDLE_BLOCK;

/* FUNCTIONS ***************************************************************/


NTSTATUS STDCALL NtDuplicateObject(IN HANDLE SourceProcessHandle,
				   IN PHANDLE SourceHandle,
				   IN HANDLE TargetProcessHandle,
				   OUT PHANDLE TargetHandle,
				   IN ACCESS_MASK DesiredAccess,
				   IN BOOLEAN InheritHandle,
				   ULONG Options)
{
   return(ZwDuplicateObject(SourceProcessHandle,
			    SourceHandle,
			    TargetProcessHandle,
			    TargetHandle,
			    DesiredAccess,
			    InheritHandle,
			    Options));
}

NTSTATUS STDCALL ZwDuplicateObject(IN HANDLE SourceProcessHandle,
				   IN PHANDLE SourceHandle,
				   IN HANDLE TargetProcessHandle,
				   OUT PHANDLE TargetHandle,
				   IN ACCESS_MASK DesiredAccess,
				   IN BOOLEAN InheritHandle,
				   ULONG Options)
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
   
   SourceHandleRep = ObTranslateHandle(&SourceProcess->Pcb,*SourceHandle);
   
   if (Options & DUPLICATE_SAME_ACCESS)
     {
	DesiredAccess = SourceHandleRep->GrantedAccess;
     }
   
   *TargetHandle = ObInsertHandle(&TargetProcess,SourceHandleRep->ObjectBody,
				  DesiredAccess,InheritHandle);
   
   if (Options & DUPLICATE_CLOSE_SOURCE)
     {
	ZwClose(*SourceHandle);
     }
   
   return(STATUS_SUCCESS);
}

VOID ObDestroyHandleTable(PKPROCESS Process)
/*
 * FUNCTION: Destroys the current process's handle table
 * NOTE: No references to handles in the table should be made during this
 * operation
 */
{
   PLIST_ENTRY current=NULL;
   unsigned int i;
   
   current = ExInterlockedRemoveHeadList(&(Process->HandleTable.ListHead),
					 &(Process->HandleTable.ListLock));
   
   while (current!=NULL)
     {
	HANDLE_BLOCK* blk = CONTAINING_RECORD(current,HANDLE_BLOCK,entry);
	
	/*
	 * Deference every handle in block
	 */
	for (i=0;i<HANDLE_BLOCK_ENTRIES;i++)
	  {
	     ObDereferenceObject(blk->handles[i].ObjectBody);
	  }
	
	/*
	 * Free the block
	 */
	ExFreePool(blk);
	
	current = ExInterlockedRemoveHeadList(&(Process->HandleTable.ListHead),
					     &(Process->HandleTable.ListLock));
     }
}

VOID ObInitializeHandleTable(PKPROCESS Parent, BOOLEAN Inherit,
			     PKPROCESS Process)
/*
 * FUNCTION: Initializes a handle table
 * ARGUMENTS:
 *        parent = Parent process (or NULL if this is the first process)
 *        Inherit = True if the process should inherit its parents objects
 *        Process = Process whose handle table is to be initialized
 */
{
   DPRINT("ObInitializeHandleTable(parent %x, Inherit %d, Process %x)\n",
	  Parent,Inherit,Process);
   
   InitializeListHead(&(Process->HandleTable.ListHead));
   KeInitializeSpinLock(&(Process->HandleTable.ListLock));
}

PHANDLE_REP ObTranslateHandle(PKPROCESS Process, HANDLE h)
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
   unsigned int handle = ((unsigned int)h) - 1;
   unsigned int count=handle/HANDLE_BLOCK_ENTRIES;
   HANDLE_BLOCK* blk = NULL;
   unsigned int i;
   
   DPRINT("ObTranslateHandle(Process %x, h %x)\n",Process,h);
   
   current = Process->HandleTable.ListHead.Flink;
   DPRINT("current %x\n",current);
   
   for (i=0;i<count;i++)
     {
	current = current->Flink;
	if (current==(&(Process->HandleTable.ListHead)))
	  {
	     return(NULL);
	  }
     }
   
   blk = CONTAINING_RECORD(current,HANDLE_BLOCK,entry);
   return(&(blk->handles[handle%HANDLE_BLOCK_ENTRIES]));
}

VOID ObDeleteHandle(HANDLE Handle)
{
   PHANDLE_REP Rep;
   
   DPRINT("ObDeleteHandle(Handle %x)\n",Handle);
   
   Rep = ObTranslateHandle(KeGetCurrentProcess(),Handle);
   Rep->ObjectBody=NULL;
   DPRINT("Finished ObDeleteHandle()\n");
}

HANDLE ObInsertHandle(PKPROCESS Process, PVOID ObjectBody,
		      ACCESS_MASK GrantedAccess, BOOLEAN Inherit)
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
   
   DPRINT("ObAddHandle(Process %x, obj %x)\n",Process,ObjectBody);
   
   current = Process->HandleTable.ListHead.Flink;
   
   /*
    * Scan through the currently allocated handle blocks looking for a free
    * slot
    */
   while (current!=(&(Process->HandleTable.ListHead)))
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
		  return((HANDLE)(handle+i));
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
   ExInterlockedInsertTailList(&(Process->HandleTable.ListHead),
			       &new_blk->entry,
			       &(Process->HandleTable.ListLock));
   new_blk->handles[0].ObjectBody = ObjectBody;
   new_blk->handles[0].GrantedAccess = GrantedAccess;
   new_blk->handles[0].Inherit = Inherit;
   return((HANDLE)handle);
}

