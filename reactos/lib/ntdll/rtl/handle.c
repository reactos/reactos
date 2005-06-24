/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Handle table
 * FILE:              lib/ntdll/rtl/handle.c
 * PROGRAMER:         Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

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
   HandleTable->MaximumNumberOfHandles = TableSize;
   HandleTable->SizeOfHandleTableEntry = HandleSize;
}


/*
 * @implemented
 */
VOID STDCALL
RtlDestroyHandleTable(PRTL_HANDLE_TABLE HandleTable)
{
   PVOID ArrayPointer;
   ULONG ArraySize;

   /* free handle array */
   if (HandleTable->CommittedHandles)
     {
        ArrayPointer = (PVOID)HandleTable->CommittedHandles;
        ArraySize = HandleTable->SizeOfHandleTableEntry * HandleTable->MaximumNumberOfHandles;
        NtFreeVirtualMemory(NtCurrentProcess(),
		            &ArrayPointer,
		            &ArraySize,
		            MEM_RELEASE);
     }
}


/*
 * @implemented
 */
PRTL_HANDLE_TABLE_ENTRY STDCALL
RtlAllocateHandle(PRTL_HANDLE_TABLE HandleTable,
		  PULONG Index)
{
   PRTL_HANDLE_TABLE_ENTRY *pp_new, *pph, ph;
   NTSTATUS Status;
   PRTL_HANDLE_TABLE_ENTRY retval;
   PVOID ArrayPointer;
   ULONG ArraySize;

   pp_new = &HandleTable->FreeHandles;

   if (HandleTable->FreeHandles == NULL)
     {
	/* no free handle available */
	if (HandleTable->UnCommittedHandles == NULL)
	  {
	     /* allocate handle array */
	     ArraySize = HandleTable->SizeOfHandleTableEntry * HandleTable->MaximumNumberOfHandles;
	     ArrayPointer = NULL;
	     
	     /* FIXME - only reserve handles here! */
	     Status = NtAllocateVirtualMemory(NtCurrentProcess(),
					      (PVOID*)&ArrayPointer,
					      0,
					      &ArraySize,
					      MEM_RESERVE | MEM_COMMIT,
					      PAGE_READWRITE);
	     if (!NT_SUCCESS(Status))
	       return NULL;

	     /* update handle array pointers */
	     HandleTable->FreeHandles = (PRTL_HANDLE_TABLE_ENTRY)ArrayPointer;
	     HandleTable->MaxReservedHandles = (PRTL_HANDLE_TABLE_ENTRY)((ULONG_PTR)ArrayPointer + ArraySize);
	     HandleTable->CommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)ArrayPointer;
	     HandleTable->UnCommittedHandles = (PRTL_HANDLE_TABLE_ENTRY)ArrayPointer;
	  }

        /* FIXME - should check if handles need to be committed */

	/* build free list in handle array */
	ph = HandleTable->FreeHandles;
	pph = pp_new;
	while (ph < HandleTable->MaxReservedHandles)
	  {
	     *pph = ph;
	     pph = &ph->NextFree;
	     ph = (PRTL_HANDLE_TABLE_ENTRY)((ULONG_PTR)ph + HandleTable->SizeOfHandleTableEntry);
	  }
	*pph = 0;
     }

   /* remove handle from free list */
   retval = *pp_new;
   *pp_new = retval->NextFree;
   retval->NextFree = NULL;

   if (Index)
     *Index = ((ULONG)((ULONG_PTR)retval - (ULONG_PTR)HandleTable->CommittedHandles) /
               HandleTable->SizeOfHandleTableEntry);

   return retval;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlFreeHandle(PRTL_HANDLE_TABLE HandleTable,
	      PRTL_HANDLE_TABLE_ENTRY Handle)
{
   /* check if handle is valid */
   if (RtlIsValidHandle(HandleTable, Handle))
     return FALSE;

   /* clear handle */
   memset(Handle, 0, HandleTable->SizeOfHandleTableEntry);

   /* add handle to free list */
   Handle->NextFree = HandleTable->FreeHandles;
   HandleTable->FreeHandles = Handle;

   return TRUE;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlIsValidHandle(PRTL_HANDLE_TABLE HandleTable,
		 PRTL_HANDLE_TABLE_ENTRY Handle)
{
   if ((HandleTable != NULL)
       && (Handle >= HandleTable->CommittedHandles)
       && (Handle < HandleTable->MaxReservedHandles)
       && (Handle->Flags & RTL_HANDLE_VALID))
     return TRUE;
   return FALSE;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlIsValidIndexHandle(PRTL_HANDLE_TABLE HandleTable,
		      PRTL_HANDLE_TABLE_ENTRY *Handle,
		      ULONG Index)
{
   PRTL_HANDLE_TABLE_ENTRY InternalHandle;

   DPRINT("RtlIsValidIndexHandle(HandleTable %p Handle %p Index %x)\n", HandleTable, Handle, Index);

   if (HandleTable == NULL)
     return FALSE;

   DPRINT("Handles %p HandleSize %x\n",
	  HandleTable->CommittedHandles, HandleTable->SizeOfHandleTableEntry);

   InternalHandle = (PRTL_HANDLE_TABLE_ENTRY)((ULONG_PTR)HandleTable->CommittedHandles +
                                              (HandleTable->SizeOfHandleTableEntry * Index));
   if (!RtlIsValidHandle(HandleTable, InternalHandle))
     return FALSE;

   DPRINT("InternalHandle %p\n", InternalHandle);

   if (Handle != NULL)
     *Handle = InternalHandle;

  return TRUE;
}

/* EOF */
