/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/libsup.c
 * PURPOSE:         Rtl library support routines
 * PROGRAMMER:      Gunnar Dalsnes
 * UPDATE HISTORY:
 *
 */

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

KPROCESSOR_MODE
RtlpGetMode()
{
   return UserMode;
}


PVOID
RtlpAllocateMemory(UINT Bytes,
                   ULONG Tag)
{
    UNREFERENCED_PARAMETER(Tag);
    
    return RtlAllocateHeap(RtlGetProcessHeap(),
                           0,
                           Bytes);
}


VOID
RtlpFreeMemory(PVOID Mem,
               ULONG Tag)
{
    UNREFERENCED_PARAMETER(Tag);
    
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                Mem);
}


#ifdef DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  /* meaningless in user mode */
}
#endif

/* RTL Atom Tables ************************************************************/

typedef struct _RTL_ATOM_HANDLE
{
   RTL_HANDLE_TABLE_ENTRY Handle;
   PRTL_ATOM_TABLE_ENTRY AtomEntry;
} RTL_ATOM_HANDLE, *PRTL_ATOM_HANDLE;

NTSTATUS
RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   RtlInitializeCriticalSection(&AtomTable->CriticalSection);
   return STATUS_SUCCESS;
}


VOID
RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   RtlDeleteCriticalSection(&AtomTable->CriticalSection);
}


BOOLEAN
RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlEnterCriticalSection(&AtomTable->CriticalSection);
   return TRUE;
}


VOID
RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlLeaveCriticalSection(&AtomTable->CriticalSection);
}


/* handle functions */

BOOLEAN
RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlInitializeHandleTable(0xCFFF,
			    sizeof(RTL_ATOM_HANDLE),
			    &AtomTable->RtlHandleTable);

   return TRUE;
}

VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlDestroyHandleTable(&AtomTable->RtlHandleTable);
}

PRTL_ATOM_TABLE
RtlpAllocAtomTable(ULONG Size)
{
   return (PRTL_ATOM_TABLE)RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           Size);
}

VOID
RtlpFreeAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               AtomTable);
}

PRTL_ATOM_TABLE_ENTRY
RtlpAllocAtomTableEntry(ULONG Size)
{
   return (PRTL_ATOM_TABLE_ENTRY)RtlAllocateHeap(RtlGetProcessHeap(),
                                                 HEAP_ZERO_MEMORY,
                                                 Size);
}

VOID
RtlpFreeAtomTableEntry(PRTL_ATOM_TABLE_ENTRY Entry)
{
   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               Entry);
}

VOID
RtlpFreeAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   PRTL_HANDLE_TABLE_ENTRY RtlHandleEntry;
   
   if (RtlIsValidIndexHandle(&AtomTable->RtlHandleTable,
                             &RtlHandleEntry,
                             (ULONG)Entry->HandleIndex))
   {
      RtlFreeHandle(&AtomTable->RtlHandleTable,
                    RtlHandleEntry);
   }
}

BOOLEAN
RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   ULONG HandleIndex;
   PRTL_HANDLE_TABLE_ENTRY RtlHandle;
   
   RtlHandle = RtlAllocateHandle(&AtomTable->RtlHandleTable,
                                 &HandleIndex);
   if (RtlHandle != NULL)
   {
      PRTL_ATOM_HANDLE AtomHandle = (PRTL_ATOM_HANDLE)RtlHandle;

      /* FIXME - Handle Indexes >= 0xC000 ?! */
      if (HandleIndex < 0xC000)
      {
         Entry->HandleIndex = (USHORT)HandleIndex;
         Entry->Atom = 0xC000 + (USHORT)HandleIndex;

         AtomHandle->AtomEntry = Entry;
         AtomHandle->Handle.Flags = RTL_HANDLE_VALID;

         return TRUE;
      }
      else
      {
         /* set the valid flag, otherwise RtlFreeHandle will fail! */
         AtomHandle->Handle.Flags = RTL_HANDLE_VALID;
         
         RtlFreeHandle(&AtomTable->RtlHandleTable,
                       RtlHandle);
      }
   }

   return FALSE;
}

PRTL_ATOM_TABLE_ENTRY
RtlpGetAtomEntry(PRTL_ATOM_TABLE AtomTable, ULONG Index)
{
   PRTL_HANDLE_TABLE_ENTRY RtlHandle;
   
   if (RtlIsValidIndexHandle(&AtomTable->RtlHandleTable,
                             &RtlHandle,
                             Index))
   {
      PRTL_ATOM_HANDLE AtomHandle = (PRTL_ATOM_HANDLE)RtlHandle;

      return AtomHandle->AtomEntry;
   }
   
   return NULL;
}

