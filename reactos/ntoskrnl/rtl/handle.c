/* $Id: handle.c,v 1.3 2002/09/08 10:23:41 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Handle table
 * FILE:              lib/ntdll/rtl/handle.c
 * PROGRAMER:         Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/handle.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>


#define TAG_HDTB  TAG('H', 'D', 'T', 'B')


static BOOLEAN
RtlpIsValidHandle(PRTL_HANDLE_TABLE HandleTable, PRTL_HANDLE Handle);
static BOOLEAN
RtlpIsValidIndexHandle(PRTL_HANDLE_TABLE HandleTable, PRTL_HANDLE *Handle, ULONG Index);


/* FUNCTIONS *****************************************************************/

VOID
RtlpInitializeHandleTable(ULONG TableSize,
			  PRTL_HANDLE_TABLE HandleTable)
{
   /* initialize handle table */
   memset(HandleTable,
	  0,
	  sizeof(RTL_HANDLE_TABLE));
   HandleTable->TableSize = TableSize;
}


VOID
RtlpDestroyHandleTable(PRTL_HANDLE_TABLE HandleTable)
{
   ExFreePool((PVOID)HandleTable->Handles);
}


BOOLEAN
RtlpAllocateHandle(PRTL_HANDLE_TABLE HandleTable,
		   PVOID Object,
		   PULONG Index)
{
   RTL_HANDLE **pp_new,**pph,*ph;
   PRTL_HANDLE retval;
   PVOID ArrayPointer;
   ULONG ArraySize;

   if (Index == NULL)
     return FALSE;

   pp_new = &HandleTable->FirstFree;

   if (HandleTable->FirstFree == NULL)
     {
	/* no free handle available */
	if (HandleTable->LastUsed == NULL)
	  {
	     /* allocate handle array */
	     ArraySize = sizeof(RTL_HANDLE) * HandleTable->TableSize;
	     ArrayPointer = ExAllocatePoolWithTag(NonPagedPool,
						  ArraySize,
						  TAG_HDTB);
	     if (ArrayPointer == NULL)
	       return FALSE;

	     /* update handle array pointers */
	     HandleTable->Handles = (PRTL_HANDLE)ArrayPointer;
	     HandleTable->Limit = (PRTL_HANDLE)(ArrayPointer + ArraySize);
	     HandleTable->LastUsed = (PRTL_HANDLE)ArrayPointer;
	  }

	/* build free list in handle array */
	ph = HandleTable->LastUsed;
	pph = pp_new;
	while (ph < HandleTable->Limit)
	  {
	     *pph = ph;
	     pph = &ph->Next;
	     ph = (PRTL_HANDLE)((ULONG)ph + sizeof(RTL_HANDLE));
	  }
	*pph = 0;
     }

   /* remove handle from free list */
   retval = *pp_new;
   *pp_new = retval->Next;
   retval->Next = NULL;
   retval->Object = Object;

   *Index = ((ULONG)retval - (ULONG)HandleTable->Handles) / sizeof(RTL_HANDLE);

   return TRUE;
}


BOOLEAN
RtlpFreeHandle(PRTL_HANDLE_TABLE HandleTable,
	       ULONG Index)
{
   PRTL_HANDLE Handle;

   /* check if handle is valid */
   if (RtlpIsValidIndexHandle(HandleTable,
			 &Handle,
			 Index))
     return FALSE;
   
   /* clear handle */
   memset(Handle, 0, sizeof(RTL_HANDLE));
   
   /* add handle to free list */
   Handle->Next = HandleTable->FirstFree;
   HandleTable->FirstFree = Handle;
   
   return TRUE;
}


static BOOLEAN
RtlpIsValidHandle(PRTL_HANDLE_TABLE HandleTable,
		  PRTL_HANDLE Handle)
{
   if ((HandleTable != NULL)
       && (Handle != NULL)
       && (Handle >= HandleTable->Handles)
       && (Handle < HandleTable->Limit))
     return TRUE;
   return FALSE;
}


static BOOLEAN
RtlpIsValidIndexHandle(PRTL_HANDLE_TABLE HandleTable,
		       PRTL_HANDLE *Handle,
		       ULONG Index)
{
   PRTL_HANDLE InternalHandle;

   DPRINT("RtlpIsValidIndexHandle(HandleTable %p Handle %p Index %x)\n", HandleTable, Handle, Index);

   if (HandleTable == NULL)
     return FALSE;

   DPRINT("Handles %p\n", HandleTable->Handles);

   InternalHandle = (PRTL_HANDLE)((ULONG)HandleTable->Handles + sizeof(RTL_HANDLE) * Index);
   if (RtlpIsValidHandle(HandleTable, InternalHandle) == FALSE)
     return FALSE;

   DPRINT("InternalHandle %p\n", InternalHandle);

   if (Handle != NULL)
     *Handle = InternalHandle;

  return TRUE;
}

PVOID
RtlpMapHandleToPointer(PRTL_HANDLE_TABLE HandleTable,
		       ULONG Index)
{
   PRTL_HANDLE Handle;

   if (!RtlpIsValidIndexHandle(HandleTable,
			       &Handle,
			       Index))
     {
	return NULL;
     }

   return Handle->Object;
}

/* EOF */
