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
#include <debug.h>

extern ULONG NtGlobalFlag;

typedef struct _RTL_RANGE_ENTRY
{
    LIST_ENTRY Entry;
    RTL_RANGE Range;
} RTL_RANGE_ENTRY, *PRTL_RANGE_ENTRY;

PAGED_LOOKASIDE_LIST RtlpRangeListEntryLookasideList;
SIZE_T RtlpAllocDeallocQueryBufferSize = 128;

/* FUNCTIONS *****************************************************************/

PVOID
NTAPI
RtlPcToFileHeader(
    IN  PVOID PcValue,
    OUT PVOID *BaseOfImage)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    BOOLEAN InSystem;
    KIRQL OldIrql;

    /* Get the base for this file */
    if ((ULONG_PTR)PcValue > (ULONG_PTR)MmHighestUserAddress)
    {
        /* Acquire the loaded module spinlock */
        KeAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);

        /* We are in kernel */
        *BaseOfImage = KiPcToFileHeader(PcValue, &LdrEntry, FALSE, &InSystem);

        /* Release lock */
        KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
    }
    else
    {
        /* User mode is not handled here! */
        *BaseOfImage = NULL;
    }

    return *BaseOfImage;
}

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
                                   'elRR',
                                   16);
}

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(VOID)
{
    /* This check is meaningless in kernel-mode */
    return FALSE;
}

BOOLEAN
NTAPI
RtlpSetInDbgPrint(VOID)
{
    /* Nothing to set in kernel mode */
    return FALSE;
}

VOID
NTAPI
RtlpClearInDbgPrint(VOID)
{
    /* Nothing to clear in kernel mode */
}

KPROCESSOR_MODE
NTAPI
RtlpGetMode(VOID)
{
   return KernelMode;
}

PVOID
NTAPI
RtlpAllocateMemory(ULONG Bytes,
                   ULONG Tag)
{
    return ExAllocatePoolWithTag(PagedPool,
                                 (SIZE_T)Bytes,
                                 Tag);
}

VOID
NTAPI
RtlpFreeMemory(PVOID Mem,
               ULONG Tag)
{
    if (Tag == TAG_ASTR || Tag == TAG_OSTR || Tag == TAG_USTR)
        ExFreePool(Mem);
    else
        ExFreePoolWithTag(Mem, Tag);
}

/*
 * @implemented
 */
VOID NTAPI
RtlAcquirePebLock(VOID)
{

}

/*
 * @implemented
 */
VOID NTAPI
RtlReleasePebLock(VOID)
{

}

NTSTATUS
NTAPI
LdrShutdownThread(VOID)
{
    return STATUS_SUCCESS;
}


PPEB
NTAPI
RtlGetCurrentPeb(VOID)
{
   return ((PEPROCESS)(KeGetCurrentThread()->ApcState.Process))->Peb;
}

NTSTATUS
NTAPI
RtlDeleteHeapLock(IN OUT PHEAP_LOCK Lock)
{
    ExDeleteResourceLite(&Lock->Resource);
    ExFreePoolWithTag(Lock, TAG_RTHL);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive)
{
    KeEnterCriticalRegion();

    if (Exclusive)
        ExAcquireResourceExclusiveLite(&Lock->Resource, TRUE);
    else
        ExAcquireResourceSharedLite(&Lock->Resource, TRUE);

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
RtlTryEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive)
{
    BOOLEAN Success;
    KeEnterCriticalRegion();

    if (Exclusive)
        Success = ExAcquireResourceExclusiveLite(&Lock->Resource, FALSE);
    else
        Success = ExAcquireResourceSharedLite(&Lock->Resource, FALSE);

    if (!Success)
        KeLeaveCriticalRegion();

    return Success;
}

NTSTATUS
NTAPI
RtlInitializeHeapLock(IN OUT PHEAP_LOCK *Lock)
{
    PHEAP_LOCK HeapLock = ExAllocatePoolWithTag(NonPagedPool,
                                                sizeof(HEAP_LOCK),
                                                TAG_RTHL);
    if (HeapLock == NULL)
        return STATUS_NO_MEMORY;

    ExInitializeResourceLite(&HeapLock->Resource);
    *Lock = HeapLock;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlLeaveHeapLock(IN OUT PHEAP_LOCK Lock)
{
    ExReleaseResourceLite(&Lock->Resource);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

struct _HEAP;

VOID
NTAPI
RtlpAddHeapToProcessList(struct _HEAP *Heap)
{
    UNREFERENCED_PARAMETER(Heap);
}

VOID
NTAPI
RtlpRemoveHeapFromProcessList(struct _HEAP *Heap)
{
    UNREFERENCED_PARAMETER(Heap);
}

VOID
RtlInitializeHeapManager(VOID)
{
}

#if DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  if(KeGetCurrentIrql() > APC_LEVEL)
  {
    DbgPrint("%s:%i: Pagable code called at IRQL > APC_LEVEL (%u)\n", file, line, KeGetCurrentIrql());
    ASSERT(FALSE);
  }
}
#endif

VOID
NTAPI
RtlpSetHeapParameters(IN PRTL_HEAP_PARAMETERS Parameters)
{
    /* Apply defaults for non-set parameters */
    if (!Parameters->SegmentCommit) Parameters->SegmentCommit = MmHeapSegmentCommit;
    if (!Parameters->SegmentReserve) Parameters->SegmentReserve = MmHeapSegmentReserve;
    if (!Parameters->DeCommitFreeBlockThreshold) Parameters->DeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;
    if (!Parameters->DeCommitTotalFreeThreshold) Parameters->DeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
}

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

#if !defined(_ARM_) && !defined(_AMD64_)

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
    ULONG_PTR Stack, NewStack, StackBegin, StackEnd = 0;
    ULONG Eip;
    BOOLEAN Result, StopSearch = FALSE;
    ULONG i = 0;
    PETHREAD Thread = PsGetCurrentThread();
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
#elif defined(_M_ARM)
    __asm__("mov sp, %0" : "=r"(Stack) : );
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
    _SEH2_TRY
    {
        /* Check if we want the user-mode stack frame */
        if (Flags == 1)
        {
            /* Get the trap frame and TEB */
            TrapFrame = KeGetTrapFrame(&Thread->Tcb);
            Teb = Thread->Tcb.Teb;

            /* Make sure we can trust the TEB and trap frame */
            if (!(Teb) ||
                (KeIsAttachedProcess()) ||
                (KeGetCurrentIrql() >= DISPATCH_LEVEL))
            {
                /* Invalid or unsafe attempt to get the stack */
                _SEH2_YIELD(return 0;)
            }

            /* Get the stack limits */
            StackBegin = (ULONG_PTR)Teb->NtTib.StackLimit;
            StackEnd = (ULONG_PTR)Teb->NtTib.StackBase;
#ifdef _M_IX86
            Stack = TrapFrame->Ebp;
#elif defined(_M_PPC)
            Stack = TrapFrame->Gpr1;
#else
#error Unknown architecture
#endif

            /* Validate them */
            if (StackEnd <= StackBegin) _SEH2_YIELD(return 0);
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
            if (!(Flags) && !(Eip & 0x80000000)) break; // FIXME: 3GB breakage

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
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* No index */
        i = 0;
    }
    _SEH2_END;

    /* Return frames parsed */
    return i;
}

#endif

#if defined(_M_AMD64) || defined(_M_ARM)
VOID
NTAPI
RtlpGetStackLimits(
    OUT PULONG_PTR LowLimit,
    OUT PULONG_PTR HighLimit)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    *HighLimit = (ULONG_PTR)CurrentThread->InitialStack;
    *LowLimit = (ULONG_PTR)CurrentThread->StackLimit;
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

BOOLEAN
NTAPI
RtlpCloseHandleCallback(
    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN HANDLE Handle,
    IN PVOID HandleTable)
{
    /* Destroy and unlock the handle entry */
    return ExDestroyHandle(HandleTable, Handle, HandleTableEntry);
}

VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->ExHandleTable)
   {
      ExSweepHandleTable(AtomTable->ExHandleTable,
                         RtlpCloseHandleCallback,
                         AtomTable->ExHandleTable);
      ExDestroyHandleTable(AtomTable->ExHandleTable, NULL);
      AtomTable->ExHandleTable = NULL;
   }
}

PRTL_ATOM_TABLE
RtlpAllocAtomTable(ULONG Size)
{
   PRTL_ATOM_TABLE Table = ExAllocatePoolWithTag(NonPagedPool,
                                                 Size,
                                                 TAG_ATMT);
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
   ExFreePoolWithTag(AtomTable, TAG_ATMT);
}

PRTL_ATOM_TABLE_ENTRY
RtlpAllocAtomTableEntry(ULONG Size)
{
    PRTL_ATOM_TABLE_ENTRY Entry;

    Entry = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_ATMT);
    if (Entry != NULL)
    {
        RtlZeroMemory(Entry, Size);
    }

    return Entry;
}

VOID
RtlpFreeAtomTableEntry(PRTL_ATOM_TABLE_ENTRY Entry)
{
    ExFreePoolWithTag(Entry, TAG_ATMT);
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

   /* Initialize ex handle table entry */
   ExEntry.Object = Entry;
   ExEntry.GrantedAccess = 0x1; /* FIXME - valid handle */

   /* Create ex handle */
   Handle = ExCreateHandle(AtomTable->ExHandleTable,
                           &ExEntry);
   if (!Handle) return FALSE;

   /* Calculate HandleIndex (by getting rid of the first two bits) */
   HandleIndex = (USHORT)((ULONG_PTR)Handle >> 2);

   /* Index must be less than 0xC000 */
   if (HandleIndex >= 0xC000)
   {
       /* Destroy ex handle */
       ExDestroyHandle(AtomTable->ExHandleTable,
                       Handle,
                       NULL);

       /* Return failure */
       return FALSE;
   }

   /* Initialize atom table entry */
   Entry->HandleIndex = HandleIndex;
   Entry->Atom = 0xC000 + HandleIndex;

   /* Return success */
   return TRUE;
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

/* Ldr SEH-Protected access to IMAGE_NT_HEADERS */

/* Rtl SEH-Free version of this */
NTSTATUS
NTAPI
RtlpImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders);

/*
 * @implemented
 * @note: This is here, so that we do not drag SEH into rosload, freeldr and bootmgfw
 */
NTSTATUS
NTAPI
RtlImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders)
{
    NTSTATUS Status;

    /* Assume failure. This is also done in RtlpImageNtHeaderEx, but this is guarded by SEH. */
    if (OutHeaders != NULL)
        *OutHeaders = NULL;

    _SEH2_TRY
    {
        Status = RtlpImageNtHeaderEx(Flags, Base, Size, OutHeaders);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail with the SEH error */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
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
    if (size < sizeof(*resdirptr)) return STATUS_RESOURCE_DATA_NOT_FOUND;
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

NTSTATUS
NTAPI
RtlpSafeCopyMemory(
   _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
   _In_reads_bytes_(Length) CONST VOID UNALIGNED *Source,
   _In_ SIZE_T Length)
{
    _SEH2_TRY
    {
        RtlCopyMemory(Destination, Source, Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
RtlCallVectoredExceptionHandlers(_In_ PEXCEPTION_RECORD ExceptionRecord,
                                 _In_ PCONTEXT Context)
{
    /* In the kernel we don't have vectored exception handlers */
    return FALSE;
}

VOID
NTAPI
RtlCallVectoredContinueHandlers(_In_ PEXCEPTION_RECORD ExceptionRecord,
                                _In_ PCONTEXT Context)
{
    /* No vectored continue handlers either in kernel mode */
    return;
}

/* EOF */
