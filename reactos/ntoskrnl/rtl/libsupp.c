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

typedef struct _RTL_RANGE_ENTRY
{
    LIST_ENTRY Entry;
    RTL_RANGE Range;
} RTL_RANGE_ENTRY, *PRTL_RANGE_ENTRY;

PAGED_LOOKASIDE_LIST RtlpRangeListEntryLookasideList;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
RtlInitializeRangeListPackage(VOID)
{
    /* Setup the lookaside list for allocations (not used yet) */
    ExInitializePagedLookasideList(&RtlpRangeListEntryLookasideList,
                                   NULL,
                                   NULL,
                                   POOL_COLD_ALLOCATION,
                                   sizeof(RTL_RANGE_ENTRY),
                                   TAG('R', 'R', 'l', 'e'),
                                   16);
}

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(BOOLEAN Type)
{
    /* This check is meaningless in kernel-mode */
    return Type;
}

BOOLEAN
NTAPI
RtlpSetInDbgPrint(IN BOOLEAN NewValue)
{
    /* This check is meaningless in kernel-mode */
    return FALSE;
}

KPROCESSOR_MODE
STDCALL
RtlpGetMode()
{
   return KernelMode;
}

PVOID
STDCALL
RtlpAllocateMemory(ULONG Bytes,
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
    KEBUGCHECK(0);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlEnterHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    KEBUGCHECK(0);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlInitializeHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
   KEBUGCHECK(0);
   return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlLeaveHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    KEBUGCHECK(0);
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
            ((ULONG_PTR)RegistrationFrame >= DpcStack - KERNEL_STACK_SIZE))
        {
            /* Update the limits to the DPC Stack's */
            *StackHigh = DpcStack;
            *StackLow = DpcStack - KERNEL_STACK_SIZE;
            return TRUE;
        }
    }

    /* Not in DPC stack */
    return FALSE;
}

BOOLEAN
NTAPI
RtlpCaptureStackLimits(IN ULONG_PTR Ebp,
                       IN ULONG_PTR *StackBegin,
                       IN ULONG_PTR *StackEnd)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Don't even try at ISR level or later */
    if (KeGetCurrentIrql() > DISPATCH_LEVEL) return FALSE;
    
    /* Start with defaults */
    *StackBegin = Thread->StackLimit;
    *StackEnd = (ULONG_PTR)Thread->StackBase;
    
    /* Check if EBP is inside the stack */
    if ((*StackBegin <= Ebp) && (Ebp <= *StackEnd))
    {
        /* Then make the stack start at EBP */
        *StackBegin = Ebp;
    }
    else
    {
        /* Now we're going to assume we're on the DPC stack */
        *StackEnd = (ULONG_PTR)(KeGetPcr()->Prcb->DpcStack);
        *StackBegin = *StackEnd - KERNEL_STACK_SIZE;
        
        /* Check if we seem to be on the DPC stack */
        if ((*StackEnd) && (*StackBegin < Ebp) && (Ebp <= *StackEnd))
        {
            /* We're on the DPC stack */
            *StackBegin = Ebp;
        }
        else
        {
            /* We're somewhere else entirely... use EBP for safety */
            *StackBegin = Ebp;
            *StackEnd = (ULONG_PTR)PAGE_ALIGN(*StackBegin);
        }
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    ULONG_PTR Stack, NewStack, StackBegin, StackEnd;
    ULONG Eip;
    BOOLEAN Result, StopSearch = FALSE;
    ULONG i = 0;
    PKTHREAD Thread = KeGetCurrentThread();
    PTEB Teb;
    PKTRAP_FRAME TrapFrame;
    
    /* Get current EBP */
#if defined(_M_IX86)
#if defined __GNUC__
    __asm__("mov %%ebp, %0" : "=r" (Stack) : );
#elif defined(_MSC_VER)
    __asm mov Stack, ebp
#endif
#elif defined(_M_MIPS)
        __asm__("move $sp, %0" : "=r" (Stack) : );
#elif defined(_M_PPC)
    __asm__("mr %0,1" : "=r" (Stack) : );
#else
#error Unknown architecture
#endif
    
    /* Set it as the stack begin limit as well */
    StackBegin = (ULONG_PTR)Stack;
    
    /* Check if we're called for non-logging mode */
    if (!Flags)
    {
        /* Get the actual safe limits */
        Result = RtlpCaptureStackLimits((ULONG_PTR)Stack,
                                        &StackBegin,
                                        &StackEnd);
        if (!Result) return 0;
    }
    
    /* Use a SEH block for maximum protection */
    _SEH_TRY
    {
        /* Check if we want the user-mode stack frame */
        if (Flags == 1)
        {
            /* Get the trap frame and TEB */          
            TrapFrame = Thread->TrapFrame;
            Teb = Thread->Teb;
            
            /* Make sure we can trust the TEB and trap frame */
            if (!(Teb) || 
                !((PVOID)((ULONG_PTR)TrapFrame & 0x80000000)) || 
                ((PVOID)TrapFrame <= (PVOID)Thread->StackLimit) ||
                ((PVOID)TrapFrame >= (PVOID)Thread->StackBase) ||
                (KeIsAttachedProcess()) || 
                (KeGetCurrentIrql() >= DISPATCH_LEVEL))
            {
                /* Invalid or unsafe attempt to get the stack */
                return 0;
            }

            /* Get the stack limits */
            StackBegin = (ULONG_PTR)Teb->Tib.StackLimit;
            StackEnd = (ULONG_PTR)Teb->Tib.StackBase;
            Stack = TrapFrame->Ebp;

            /* Validate them */
            if (StackEnd <= StackBegin) return 0;
            ProbeForRead((PVOID)StackBegin,
                         StackEnd - StackBegin,
                         sizeof(CHAR));
        }
        
        /* Loop the frames */
        for (i = 0; i < Count; i++)
        {
            /*
             * Leave if we're past the stack,
             * if we're before the stack,
             * or if we've reached ourselves.
             */
            if ((Stack >= StackEnd) ||
                (!i ? (Stack < StackBegin) : (Stack <= StackBegin)) ||
                ((StackEnd - Stack) < (2 * sizeof(ULONG_PTR))))
            {
                /* We're done or hit a bad address */
                break;
            }
            
            /* Get new stack and EIP */
            NewStack = *(PULONG_PTR)Stack;
            Eip = *(PULONG_PTR)(Stack + sizeof(ULONG_PTR));
            
            /* Check if the new pointer is above the oldone and past the end */
            if (!((Stack < NewStack) && (NewStack < StackEnd)))
            {
                /* Stop searching after this entry */
                StopSearch = TRUE;
            }
            
            /* Also make sure that the EIP isn't a stack address */
            if ((StackBegin < Eip) && (Eip < StackEnd)) break;
            
            /* Check if we reached a user-mode address */
            if (!(Flags) && !(Eip & 0x80000000)) break;
            
            /* Save this frame */
            Callers[i] = (PVOID)Eip;
            
            /* Check if we should continue */
            if (StopSearch)
            {
                /* Return the next index */
                i++;
                break;
            }
            
            /* Move to the next stack */
            Stack = NewStack;
        }
    }
    _SEH_HANDLE
    {
        /* No index */
        i = 0;
    }
    _SEH_END;
    
    /* Return frames parsed */
    return i;    
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

VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->ExHandleTable)
   {
      ExSweepHandleTable(AtomTable->ExHandleTable,
                         NULL,
                         NULL);
      ExDestroyHandleTable(AtomTable->ExHandleTable, NULL);
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
                   (HANDLE)((ULONG_PTR)Entry->HandleIndex << 2),
                   NULL);
}

BOOLEAN
RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   HANDLE_TABLE_ENTRY ExEntry;
   HANDLE Handle;
   USHORT HandleIndex;
   
   ExEntry.Object = Entry;
   ExEntry.GrantedAccess = 0x1; /* FIXME - valid handle */
   
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
                         Handle,
                         NULL);
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
      Entry = ExEntry->Object;
      
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

   UniDest->MaximumLength = (USHORT)Length;
   UniDest->Length = (USHORT)Length - sizeof (WCHAR);

   return TRUE;
}

/*
 * Ldr Resource support code
 */

IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( IMAGE_RESOURCE_DIRECTORY *dir,
                                              LPCWSTR name, void *root,
                                              int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( IMAGE_RESOURCE_DIRECTORY *dir,
                                            USHORT id, void *root, int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_first_entry( IMAGE_RESOURCE_DIRECTORY *dir,
                                            void *root, int want_dir );

/**********************************************************************
 *  find_entry
 *
 * Find a resource entry
 */
NTSTATUS find_entry( PVOID BaseAddress, LDR_RESOURCE_INFO *info,
                     ULONG level, void **ret, int want_dir )
{
    ULONG size;
    void *root;
    IMAGE_RESOURCE_DIRECTORY *resdirptr;

    root = RtlImageDirectoryEntryToData( BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size );
    if (!root) return STATUS_RESOURCE_DATA_NOT_FOUND;
    resdirptr = root;

    if (!level--) goto done;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Type, root, want_dir || level )))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;

    resdirptr = *ret;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Name, root, want_dir || level )))
        return STATUS_RESOURCE_NAME_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;
    if (level) return STATUS_INVALID_PARAMETER;  /* level > 3 */

    resdirptr = *ret;

    if ((*ret = find_first_entry( resdirptr, root, want_dir ))) return STATUS_SUCCESS;

    return STATUS_RESOURCE_DATA_NOT_FOUND;

done:
    *ret = resdirptr;
    return STATUS_SUCCESS;
}


/* EOF */
