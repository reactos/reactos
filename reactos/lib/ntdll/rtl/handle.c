/* $Id: handle.c,v 1.4 2002/09/08 10:23:05 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Handle table
 * FILE:              lib/ntdll/rtl/handle.c
 * PROGRAMER:         Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
RtlInitializeHandleTable(ULONG TableSize,
			 ULONG HandleSize,
			 PRTL_HANDLE_TABLE HandleTable)
{
   /* initialize handle table */
   memset(HandleTable,
	  0,
	  sizeof(RTL_HANDLE_TABLE));
   HandleTable->TableSize = TableSize;
   HandleTable->HandleSize = HandleSize;
}


VOID STDCALL
RtlDestroyHandleTable(PRTL_HANDLE_TABLE HandleTable)
{
   PVOID ArrayPointer;
   ULONG ArraySize;

   /* free handle array */
   ArrayPointer = (PVOID)HandleTable->Handles;
   ArraySize = (ULONG)HandleTable->Limit - (ULONG)HandleTable->Handles;
   NtFreeVirtualMemory(NtCurrentProcess(),
		       &ArrayPointer,
		       &ArraySize,
		       MEM_RELEASE);
}


PRTL_HANDLE STDCALL
RtlAllocateHandle(PRTL_HANDLE_TABLE HandleTable,
		  PULONG Index)
{
   RTL_HANDLE **pp_new,**pph,*ph;
   NTSTATUS Status;
   PRTL_HANDLE retval;
   PVOID ArrayPointer;
   ULONG ArraySize;

   pp_new = &HandleTable->FirstFree;

   if (HandleTable->FirstFree == NULL)
     {
	/* no free handle available */
	if (HandleTable->LastUsed == NULL)
	  {
	     /* allocate handle array */
	     ArraySize = HandleTable->HandleSize * HandleTable->TableSize;
	     ArrayPointer = NULL;
	     Status = NtAllocateVirtualMemory(NtCurrentProcess(),
					      (PVOID*)&ArrayPointer,
					      0,
					      &ArraySize,
					      MEM_RESERVE | MEM_COMMIT,
					      PAGE_READWRITE);
	     if (!NT_SUCCESS(Status))
	       return NULL;

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
	     ph = (PRTL_HANDLE)((ULONG)ph + HandleTable->HandleSize);
	  }
	*pph = 0;
     }

   /* remove handle from free list */
   retval = *pp_new;
   *pp_new = retval->Next;
   retval->Next = NULL;

   if (Index)
     *Index = ((ULONG)retval - (ULONG)HandleTable->Handles) / HandleTable->HandleSize;

   return retval;
}


BOOLEAN STDCALL
RtlFreeHandle(PRTL_HANDLE_TABLE HandleTable,
	      PRTL_HANDLE Handle)
{
   /* check if handle is valid */
   if (RtlIsValidHandle(HandleTable, Handle))
     return FALSE;
   
   /* clear handle */
   memset(Handle, 0, HandleTable->HandleSize);
   
   /* add handle to free list */
   Handle->Next = HandleTable->FirstFree;
   HandleTable->FirstFree = Handle;
   
   return TRUE;
}


BOOLEAN STDCALL
RtlIsValidHandle(PRTL_HANDLE_TABLE HandleTable,
		 PRTL_HANDLE Handle)
{
   if ((HandleTable != NULL)
       && (Handle != NULL)
       && (Handle >= HandleTable->Handles)
       && (Handle < HandleTable->Limit))
     return TRUE;
   return FALSE;
}


BOOLEAN STDCALL
RtlIsValidIndexHandle(PRTL_HANDLE_TABLE HandleTable,
		      PRTL_HANDLE *Handle,
		      ULONG Index)
{
   PRTL_HANDLE InternalHandle;

   DPRINT("RtlIsValidIndexHandle(HandleTable %p Handle %p Index %x)\n", HandleTable, Handle, Index);

   if (HandleTable == NULL)
     return FALSE;

   DPRINT("Handles %p HandleSize %x\n",
	  HandleTable->Handles, HandleTable->HandleSize);

   InternalHandle = (PRTL_HANDLE)((ULONG)HandleTable->Handles + (HandleTable->HandleSize * Index));
   if (RtlIsValidHandle(HandleTable, InternalHandle) == FALSE)
     return FALSE;

   DPRINT("InternalHandle %p\n", InternalHandle);

   if (Handle != NULL)
     *Handle = InternalHandle;

  return TRUE;
}

/* EOF */
