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

/*
 * PURPOSE: Defines a handle
 */
typedef struct
{
   PVOID obj;
} HANDLE_REP, *PHANDLE_REP;

#define HANDLE_BLOCK_ENTRIES ((PAGESIZE-sizeof(LIST_ENTRY))/sizeof(HANDLE_REP))

/*
 * PURPOSE: Defines a page's worth of handles
 */
typedef struct
{
   LIST_ENTRY entry;
   HANDLE_REP handles[HANDLE_BLOCK_ENTRIES];
} HANDLE_BLOCK;

/* GLOBALS *****************************************************************/

/*
 * PURPOSE: Head of the list of handle blocks
 */
LIST_ENTRY handle_list_head = {NULL,NULL};
KSPIN_LOCK handle_list_lock = {0};

/* FUNCTIONS ***************************************************************/

VOID ObjDestroyHandleTable(VOID)
/*
 * FUNCTION: Destroys the current process's handle table
 * NOTE: No references to handles in the table should be made during this
 * operation
 */
{
   PLIST_ENTRY current=ExInterlockedRemoveHeadList(&handle_list_head,
						   &handle_list_lock);
   unsigned int i;
   
   while (current!=NULL)
     {
	HANDLE_BLOCK* blk = (HANDLE_BLOCK *)current;
	
	/*
	 * Deference every handle in block
	 */
	for (i=0;i<HANDLE_BLOCK_ENTRIES;i++)
	  {
	     ObDereferenceObject(blk->handles[i].obj);
	  }
	
	/*
	 * Free the block
	 */
	ExFreePool(blk);
	
	current = ExInterlockedRemoveHeadList(&handle_list_head,
					      &handle_list_lock);
     }
}

VOID ObjInitializeHandleTable(HANDLE parent)
/*
 * FUNCTION: Initializes a handle table for the current process
 * ARGUMENTS:
 *        parent = Parent process (or NULL if this is the first process)
 */
{
   DPRINT("ObjInitializeHandleTable(parent %x)\n",parent);
   
   InitializeListHead(&handle_list_head);
   KeInitializeSpinLock(&handle_list_lock);
}

static PHANDLE_REP ObTranslateHandle(HANDLE* h)
{
   PLIST_ENTRY current = handle_list_head.Flink;
   unsigned int handle = ((unsigned int)h) - 1;
   unsigned int count=handle/HANDLE_BLOCK_ENTRIES;
   HANDLE_BLOCK* blk = NULL;
   unsigned int i;
   
   for (i=0;i<count;i++)
     {
	current = current->Flink;
	if (current==(&handle_list_head))
	  {
	     return(NULL);
	  }
     }
   
   blk = (HANDLE_BLOCK *)current;
   return(&(blk->handles[handle%HANDLE_BLOCK_ENTRIES]));
}

PVOID ObGetObjectByHandle(HANDLE h)
/*
 * FUNCTION: Translate a handle to the corresponding object
 * ARGUMENTS:
 *         h = Handle to translate
 * RETURNS: The object
 */
{
   DPRINT("ObGetObjectByHandle(h %x)\n",h);
   
   if (h==NULL)
     {
	return(NULL);
     }
   
   return(ObTranslateHandle(h)->obj);
}

VOID ObDeleteHandle(HANDLE Handle)
{
   PHANDLE_REP Rep = ObTranslateHandle(Handle);
   Rep->obj=NULL;
}

HANDLE ObAddHandle(PVOID obj)
/*
 * FUNCTION: Add a handle referencing an object
 * ARGUMENTS:
 *         obj = Object body that the handle should refer to
 * RETURNS: The created handle
 * NOTE: THe handle is valid only in the context of the current process
 */
{
   LIST_ENTRY* current = handle_list_head.Flink;
   unsigned int handle=1;
   unsigned int i;
   HANDLE_BLOCK* new_blk = NULL;
   
   DPRINT("ObAddHandle(obj %x)\n",obj);
   
   /*
    * Scan through the currently allocated handle blocks looking for a free
    * slot
    */
   while (current!=(&handle_list_head))
     {
	HANDLE_BLOCK* blk = (HANDLE_BLOCK *)current;

        DPRINT("Current %x\n",current);

	for (i=0;i<HANDLE_BLOCK_ENTRIES;i++)
	  {
             DPRINT("Considering slot %d containing %x\n",i,blk->handles[i]);
	     if (blk->handles[i].obj==NULL)
	       {
		  blk->handles[i].obj=obj;
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
   ExInterlockedInsertTailList(&handle_list_head,&new_blk->entry,
			       &handle_list_lock);
   new_blk->handles[0].obj=obj;
   return((HANDLE)handle);
}

