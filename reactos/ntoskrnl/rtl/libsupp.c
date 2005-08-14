/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/libsupp.c
 * PURPOSE:         Rtl library support routines
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


KPROCESSOR_MODE
RtlpGetMode()
{
   return KernelMode;
}

/*
 * @implemented
 */
VOID STDCALL
RtlAcquirePebLock(VOID)
{

}

/*
 * @implemented
 */
VOID STDCALL
RtlReleasePebLock(VOID)
{

}


PPEB
STDCALL
RtlpCurrentPeb(VOID)
{
   return ((PEPROCESS)(KeGetCurrentThread()->ApcState.Process))->Peb;
}

NTSTATUS
STDCALL
RtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return STATUS_SUCCESS;
}

DWORD
STDCALL
RtlSetCriticalSectionSpinCount(
   PRTL_CRITICAL_SECTION CriticalSection,
   DWORD SpinCount
   )
{
   return 0;
}

NTSTATUS
STDCALL
RtlEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    ExAcquireFastMutex((PFAST_MUTEX) CriticalSection);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
   ExInitializeFastMutex((PFAST_MUTEX)CriticalSection );
   return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlLeaveCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    ExReleaseFastMutex((PFAST_MUTEX) CriticalSection );
    return STATUS_SUCCESS;
}

BOOLEAN
STDCALL
RtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return ExTryToAcquireFastMutex((PFAST_MUTEX) CriticalSection );
}


NTSTATUS
STDCALL
RtlInitializeCriticalSectionAndSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount)
{
    ExInitializeFastMutex((PFAST_MUTEX)CriticalSection );
    return STATUS_SUCCESS;
}


#ifdef DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  if(KeGetCurrentIrql() > APC_LEVEL)
  {
    DbgPrint("%s:%i: Pagable code called at IRQL > APC_LEVEL (%d)\n", file, line, KeGetCurrentIrql());
    KEBUGCHECK(0);
  }
}
#endif

/* RTL Atom Tables ************************************************************/

NTSTATUS
RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   ExInitializeFastMutex(&AtomTable->FastMutex);

   return STATUS_SUCCESS;
}


VOID
RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
}


BOOLEAN
RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   ExAcquireFastMutex(&AtomTable->FastMutex);
   return TRUE;
}

VOID
RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   ExReleaseFastMutex(&AtomTable->FastMutex);
}

BOOLEAN
RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   AtomTable->ExHandleTable = ExCreateHandleTable(NULL);
   return (AtomTable->ExHandleTable != NULL);
}

static VOID STDCALL
AtomDeleteHandleCallback(PHANDLE_TABLE HandleTable,
                         PVOID Object,
                         ULONG GrantedAccess,
                         PVOID Context)
{
   return;
}

VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->ExHandleTable)
   {
      ExDestroyHandleTable(AtomTable->ExHandleTable,
                           AtomDeleteHandleCallback,
                           AtomTable);
      AtomTable->ExHandleTable = NULL;
   }
}

PRTL_ATOM_TABLE
RtlpAllocAtomTable(ULONG Size)
{
   PRTL_ATOM_TABLE Table = ExAllocatePool(NonPagedPool,
                                          Size);
   if (Table != NULL)
   {
      RtlZeroMemory(Table,
                    Size);
   }
   
   return Table;
}

VOID
RtlpFreeAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   ExFreePool(AtomTable);
}

PRTL_ATOM_TABLE_ENTRY
RtlpAllocAtomTableEntry(ULONG Size)
{
   PRTL_ATOM_TABLE_ENTRY Entry = ExAllocatePool(NonPagedPool,
                                                Size);
   if (Entry != NULL)
   {
      RtlZeroMemory(Entry,
                    Size);
   }

   return Entry;
}

VOID
RtlpFreeAtomTableEntry(PRTL_ATOM_TABLE_ENTRY Entry)
{
   ExFreePool(Entry);
}

VOID
RtlpFreeAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   ExDestroyHandle(AtomTable->ExHandleTable,
                   (LONG)Entry->HandleIndex);
}

BOOLEAN
RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   HANDLE_TABLE_ENTRY ExEntry;
   LONG HandleIndex;
   
   ExEntry.u1.Object = Entry;
   ExEntry.u2.GrantedAccess = 0x1; /* FIXME - valid handle */
   
   HandleIndex = ExCreateHandle(AtomTable->ExHandleTable,
                                &ExEntry);
   if (HandleIndex != 0)
   {
      /* FIXME - Handle Indexes >= 0xC000 ?! */
      if (HandleIndex < 0xC000)
      {
         Entry->HandleIndex = (USHORT)HandleIndex;
         Entry->Atom = 0xC000 + (USHORT)HandleIndex;
         
         return TRUE;
      }
      else
         ExDestroyHandle(AtomTable->ExHandleTable,
                         HandleIndex);
   }
   
   return FALSE;
}

PRTL_ATOM_TABLE_ENTRY
RtlpGetAtomEntry(PRTL_ATOM_TABLE AtomTable, ULONG Index)
{
   PHANDLE_TABLE_ENTRY ExEntry;
   
   ExEntry = ExMapHandleToPointer(AtomTable->ExHandleTable,
                                  (LONG)Index);
   if (ExEntry != NULL)
   {
      PRTL_ATOM_TABLE_ENTRY Entry;
      
      Entry = ExEntry->u1.Object;
      
      ExUnlockHandleTableEntry(AtomTable->ExHandleTable,
                               ExEntry);
      return Entry;
   }
   
   return NULL;
}

/* EOF */
