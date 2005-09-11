/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/libsupp.c
 * PURPOSE:         RTL Support Routines
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern ULONG NtGlobalFlag;

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(VOID)
{
    /* This check is meaningless in kernel-mode */
    return TRUE;
}

KPROCESSOR_MODE
STDCALL
RtlpGetMode()
{
   return KernelMode;
}

PVOID
STDCALL
RtlpAllocateMemory(UINT Bytes,
                   ULONG Tag)
{
    return ExAllocatePoolWithTag(PagedPool,
                                 (SIZE_T)Bytes,
                                 Tag);
}


VOID
STDCALL
RtlpFreeMemory(PVOID Mem,
               ULONG Tag)
{
    ExFreePoolWithTag(Mem,
                      Tag);
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

NTSTATUS
STDCALL
LdrShutdownThread(VOID)
{
    return STATUS_SUCCESS;
}


PPEB
STDCALL
RtlpCurrentPeb(VOID)
{
   return ((PEPROCESS)(KeGetCurrentThread()->ApcState.Process))->Peb;
}

NTSTATUS
STDCALL
RtlDeleteHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlEnterHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    ExAcquireFastMutex((PFAST_MUTEX) CriticalSection);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlInitializeHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
   ExInitializeFastMutex((PFAST_MUTEX)CriticalSection );
   return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlLeaveHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    ExReleaseFastMutex((PFAST_MUTEX) CriticalSection );
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

VOID
NTAPI
RtlpCheckLogException(IN PEXCEPTION_RECORD ExceptionRecord,
                      IN PCONTEXT ContextRecord,
                      IN PVOID ContextData,
                      IN ULONG Size)
{
    /* Check the global flag */
    if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING)
    {
        /* FIXME: Log this exception */
    }
}

BOOLEAN
NTAPI
RtlpHandleDpcStackException(IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            IN ULONG_PTR RegistrationFrameEnd,
                            IN OUT PULONG_PTR StackLow,
                            IN OUT PULONG_PTR StackHigh)
{
    PKPRCB Prcb;
    ULONG_PTR DpcStack;

    /* Check if we are at DISPATCH or higher */
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        /* Get the PRCB and DPC Stack */
        Prcb = KeGetCurrentPrcb();
        DpcStack = (ULONG_PTR)Prcb->DpcStack;

        /* Check if we are in a DPC and the stack matches */
        if ((Prcb->DpcRoutineActive) &&
            (RegistrationFrameEnd <= DpcStack) &&
            ((ULONG_PTR)RegistrationFrame >= DpcStack - 4096))
        {
            /* Update the limits to the DPC Stack's */
            *StackHigh = DpcStack;
            *StackLow = DpcStack - 4096;
            return TRUE;
        }
    }

    /* Not in DPC stack */
    return FALSE;
}

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
                   (HANDLE)((ULONG_PTR)Entry->HandleIndex << 2));
}

BOOLEAN
RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   HANDLE_TABLE_ENTRY ExEntry;
   HANDLE Handle;
   USHORT HandleIndex;
   
   ExEntry.u1.Object = Entry;
   ExEntry.u2.GrantedAccess = 0x1; /* FIXME - valid handle */
   
   Handle = ExCreateHandle(AtomTable->ExHandleTable,
                                &ExEntry);
   if (Handle != NULL)
   {
      HandleIndex = (USHORT)((ULONG_PTR)Handle >> 2);
      /* FIXME - Handle Indexes >= 0xC000 ?! */
      if ((ULONG_PTR)HandleIndex >> 2 < 0xC000)
      {
         Entry->HandleIndex = HandleIndex;
         Entry->Atom = 0xC000 + HandleIndex;
         
         return TRUE;
      }
      else
         ExDestroyHandle(AtomTable->ExHandleTable,
                         Handle);
   }
   
   return FALSE;
}

PRTL_ATOM_TABLE_ENTRY
RtlpGetAtomEntry(PRTL_ATOM_TABLE AtomTable, ULONG Index)
{
   PHANDLE_TABLE_ENTRY ExEntry;
   PRTL_ATOM_TABLE_ENTRY Entry = NULL;
   
   /* NOTE: There's no need to explicitly enter a critical region because it's
            guaranteed that we're in a critical region right now (as we hold
            the atom table lock) */
   
   ExEntry = ExMapHandleToPointer(AtomTable->ExHandleTable,
                                  (HANDLE)((ULONG_PTR)Index << 2));
   if (ExEntry != NULL)
   {
      Entry = ExEntry->u1.Object;
      
      ExUnlockHandleTableEntry(AtomTable->ExHandleTable,
                               ExEntry);
   }
   
   return Entry;
}

/* FIXME - RtlpCreateUnicodeString is obsolete and should be removed ASAP! */
BOOLEAN FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType)
{
   ULONG Length;

   Length = (wcslen (Source) + 1) * sizeof(WCHAR);
   UniDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG('U', 'S', 'T', 'R'));
   if (UniDest->Buffer == NULL)
      return FALSE;

   RtlCopyMemory (UniDest->Buffer,
                  Source,
                  Length);

   UniDest->MaximumLength = Length;
   UniDest->Length = Length - sizeof (WCHAR);

   return TRUE;
}

/* EOF */
