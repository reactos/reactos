/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/kernel.c
 * PURPOSE:         Initializes the kernel
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

KNODE KiNode0;
PKNODE KeNodeBlock[1];
UCHAR KeNumberNodes = 1;
UCHAR KeProcessNodeSeed;
ULONG KiPcrInitDone = 0;
extern ULONG Ke386GlobalPagesEnabled;

/* System-defined Spinlocks */
KSPIN_LOCK KiDispatcherLock;
KSPIN_LOCK MmPfnLock;
KSPIN_LOCK MmSystemSpaceLock;
KSPIN_LOCK CcBcbSpinLock;
KSPIN_LOCK CcMasterSpinLock;
KSPIN_LOCK CcVacbSpinLock;
KSPIN_LOCK CcWorkQueueSpinLock;
KSPIN_LOCK NonPagedPoolLock;
KSPIN_LOCK MmNonPagedPoolLock;
KSPIN_LOCK IopCancelSpinLock;
KSPIN_LOCK IopVpbSpinLock;
KSPIN_LOCK IopDatabaseLock;
KSPIN_LOCK IopCompletionLock;
KSPIN_LOCK NtfsStructLock;
KSPIN_LOCK AfdWorkQueueSpinLock;
KSPIN_LOCK KiTimerTableLock[16];
KSPIN_LOCK KiReverseStallIpiLock;

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, KeInit1)
#pragma alloc_text(INIT, KeInit2)
#endif

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitSpinLocks(IN PKPRCB Prcb,
                IN CCHAR Number)
{
    ULONG i;

    /* Initialize Dispatcher Fields */
    Prcb->QueueIndex = 1;
    Prcb->ReadySummary = 0;
    Prcb->DeferredReadyListHead.Next = NULL;
    for (i = 0; i < 32; i++)
    {
        /* Initialize the ready list */
        InitializeListHead(&Prcb->DispatcherReadyListHead[i]);
    }

    /* Initialize DPC Fields */
    InitializeListHead(&Prcb->DpcData[0].DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcData[0].DpcLock);
    Prcb->DpcData[0].DpcQueueDepth = 0;
    Prcb->DpcData[0].DpcCount = 0;
    Prcb->DpcRoutineActive = FALSE;
    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    KeInitializeDpc(&Prcb->CallDpc, NULL, NULL);
    //KeSetTargetProcessorDpc(&Prcb->CallDpc, Number);
    KeSetImportanceDpc(&Prcb->CallDpc, HighImportance);

    /* Initialize the Wait List Head */
    InitializeListHead(&Prcb->WaitListHead);

    /* Initialize Queued Spinlocks */
    Prcb->LockQueue[LockQueueDispatcherLock].Next = NULL;
    Prcb->LockQueue[LockQueueDispatcherLock].Lock = &KiDispatcherLock;
    Prcb->LockQueue[LockQueueExpansionLock].Next = NULL;
    Prcb->LockQueue[LockQueueExpansionLock].Lock = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Next = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Lock = &MmPfnLock;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Next = NULL;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Lock = &MmSystemSpaceLock;
    Prcb->LockQueue[LockQueueBcbLock].Next = NULL;
    Prcb->LockQueue[LockQueueBcbLock].Lock = &CcBcbSpinLock;
    Prcb->LockQueue[LockQueueMasterLock].Next = NULL;
    Prcb->LockQueue[LockQueueMasterLock].Lock = &CcMasterSpinLock;
    Prcb->LockQueue[LockQueueVacbLock].Next = NULL;
    Prcb->LockQueue[LockQueueVacbLock].Lock = &CcVacbSpinLock;
    Prcb->LockQueue[LockQueueWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueWorkQueueLock].Lock = &CcWorkQueueSpinLock;
    Prcb->LockQueue[LockQueueNonPagedPoolLock].Next = NULL;
    Prcb->LockQueue[LockQueueNonPagedPoolLock].Lock = &NonPagedPoolLock;
    Prcb->LockQueue[LockQueueMmNonPagedPoolLock].Next = NULL;
    Prcb->LockQueue[LockQueueMmNonPagedPoolLock].Lock = &MmNonPagedPoolLock;
    Prcb->LockQueue[LockQueueIoCancelLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCancelLock].Lock = &IopCancelSpinLock;
    Prcb->LockQueue[LockQueueIoVpbLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoVpbLock].Lock = &IopVpbSpinLock;
    Prcb->LockQueue[LockQueueIoDatabaseLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoDatabaseLock].Lock = &IopDatabaseLock;
    Prcb->LockQueue[LockQueueIoCompletionLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCompletionLock].Lock = &IopCompletionLock;
    Prcb->LockQueue[LockQueueNtfsStructLock].Next = NULL;
    Prcb->LockQueue[LockQueueNtfsStructLock].Lock = &NtfsStructLock;
    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Lock = &AfdWorkQueueSpinLock;
    Prcb->LockQueue[LockQueueUnusedSpare16].Next = NULL;
    Prcb->LockQueue[LockQueueUnusedSpare16].Lock = NULL;
    for (i = LockQueueTimerTableLock; i < LockQueueMaximumLock; i++)
    {
        KeInitializeSpinLock(&KiTimerTableLock[i - 16]);
        Prcb->LockQueue[i].Next = NULL;
        Prcb->LockQueue[i].Lock = &KiTimerTableLock[i - 16];
    }

    /* Check if this is the boot CPU */
    if (!Number)
    {
        /* Initialize the lock themselves */
        KeInitializeSpinLock(&KiDispatcherLock);
        KeInitializeSpinLock(&KiReverseStallIpiLock);
        KeInitializeSpinLock(&MmPfnLock);
        KeInitializeSpinLock(&MmSystemSpaceLock);
        KeInitializeSpinLock(&CcBcbSpinLock);
        KeInitializeSpinLock(&CcMasterSpinLock);
        KeInitializeSpinLock(&CcVacbSpinLock);
        KeInitializeSpinLock(&CcWorkQueueSpinLock);
        KeInitializeSpinLock(&IopCancelSpinLock);
        KeInitializeSpinLock(&IopCompletionLock);
        KeInitializeSpinLock(&IopDatabaseLock);
        KeInitializeSpinLock(&IopVpbSpinLock);
        KeInitializeSpinLock(&NonPagedPoolLock);
        KeInitializeSpinLock(&MmNonPagedPoolLock);
        KeInitializeSpinLock(&NtfsStructLock);
        KeInitializeSpinLock(&AfdWorkQueueSpinLock);
    }
}

VOID
NTAPI
KeInit1(VOID)
{
    PKIPCR KPCR;
    PKPRCB Prcb;
    BOOLEAN NpxPresent;
    ULONG FeatureBits;
    extern USHORT KiBootGdt[];
    extern KTSS KiBootTss;

    /* Initialize the PCR */
    KPCR = (PKIPCR)KPCR_BASE;
    Prcb = &KPCR->PrcbData;
    memset(KPCR, 0, PAGE_SIZE);
    KPCR->Self = (PKPCR)KPCR;
    KPCR->Prcb = &KPCR->PrcbData;
    KPCR->Irql = SYNCH_LEVEL;
    KPCR->NtTib.Self = &KPCR->NtTib;
    KPCR->NtTib.ExceptionList = (PVOID)-1;
    KPCR->GDT = KiBootGdt;
    KPCR->IDT = KiIdt;
    KPCR->TSS = &KiBootTss;
    KPCR->Number = 0;
    KPCR->SetMember = 1 << 0;
    KeActiveProcessors = 1 << 0;
    KPCR->PrcbData.SetMember = 1 << 0;
    KiPcrInitDone = 1;

    /*
     * Low-level GDT, TSS and LDT Setup, most of which Freeldr should have done
     * instead, and we should only add some extra information. This would be
     * required for future NTLDR compatibility.
     */
    KiInitializeGdt(NULL);
    Ki386BootInitializeTSS();
    Ki386InitializeLdt();
    KeInitExceptions();
    KeInitInterrupts();

    /* Detect and set the CPU Type */
    KiSetProcessorType();

    /* Set CR0 features based on detected CPU */
    KiSetCR0Bits();

    /* Check if an FPU is present */
    NpxPresent = KiIsNpxPresent();

    /* Initialize the Power Management Support for this PRCB */
    PoInitializePrcb(Prcb);

    /* Bugcheck if this is a 386 CPU */
    if (Prcb->CpuType == 3) KeBugCheckEx(0x5D, 0x386, 0, 0, 0);

    /* Get the processor features for the CPU */
    FeatureBits = KiGetFeatureBits();

    /* Save feature bits */
    Prcb->FeatureBits = FeatureBits;

    /* Get cache line information for this CPU */
    KiGetCacheInformation();

    /* Initialize spinlocks and DPC data */
    KiInitSpinLocks(Prcb, 0);

    /* Set Node Data */
    KeNodeBlock[0] = &KiNode0;
    Prcb->ParentNode = KeNodeBlock[0];
    KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

    /* Set boot-level flags */
    KeI386NpxPresent = NpxPresent;
    KeI386CpuType = Prcb->CpuType;
    KeI386CpuStep = Prcb->CpuStep;
    KeProcessorArchitecture = 0;
    KeProcessorLevel = (USHORT)Prcb->CpuType;
    if (Prcb->CpuID) KeProcessorRevision = Prcb->CpuStep;
    KeFeatureBits = FeatureBits;
    KeI386FxsrPresent = (KeFeatureBits & KF_FXSR) ? TRUE : FALSE;
    KeI386XMMIPresent = (KeFeatureBits & KF_XMMI) ? TRUE : FALSE;

    /* Check if Fxsr was found */
    if (KeI386FxsrPresent)
    {
        /* Enable it. FIXME: Send an IPI */
        Ke386SetCr4(Ke386GetCr4() | X86_CR4_OSFXSR);

        /* Check if XMM was found too */
        if (KeI386XMMIPresent)
        {
            /* Enable it: FIXME: Send an IPI. */
            Ke386SetCr4(Ke386GetCr4() | X86_CR4_OSXMMEXCPT);

            /* FIXME: Implement and enable XMM Page Zeroing for Mm */
        }
    }

   if (KeFeatureBits & KF_GLOBAL_PAGE)
   {
      ULONG Flags;
      /* Enable global pages */
      Ke386GlobalPagesEnabled = TRUE;
      Ke386SaveFlags(Flags);
      Ke386DisableInterrupts();
      Ke386SetCr4(Ke386GetCr4() | X86_CR4_PGE);
      Ke386RestoreFlags(Flags);
   }

   if (KeFeatureBits & KF_FAST_SYSCALL)
   {
      extern void KiFastCallEntry(void);

      /* CS Selector of the target segment. */
      Ke386Wrmsr(0x174, KGDT_R0_CODE, 0);
      /* Target ESP. */
      Ke386Wrmsr(0x175, 0, 0);
      /* Target EIP. */
      Ke386Wrmsr(0x176, (ULONG_PTR)KiFastCallEntry, 0);
   }

   /* Does the CPU Support 'prefetchnta' (SSE)  */
   if(KeFeatureBits & KF_XMMI)
   {
       ULONG Protect;

       Protect = MmGetPageProtect(NULL, (PVOID)RtlPrefetchMemoryNonTemporal);
       MmSetPageProtect(NULL, (PVOID)RtlPrefetchMemoryNonTemporal, Protect | PAGE_IS_WRITABLE);
       /* Replace the ret by a nop */
       *(PCHAR)RtlPrefetchMemoryNonTemporal = 0x90;
       MmSetPageProtect(NULL, (PVOID)RtlPrefetchMemoryNonTemporal, Protect);
   }
}

VOID
INIT_FUNCTION
NTAPI
KeInit2(VOID)
{
   ULONG Protect;
   PKIPCR Pcr = (PKIPCR)KeGetPcr();
   PKPRCB Prcb = Pcr->Prcb;

   KiInitializeBugCheck();
   KeInitializeDispatcher();
   KiInitializeSystemClock();

   DPRINT1("CPU Detection Complete.\n"
           "CPUID: %lx\n"
           "Step : %lx\n"
           "Type : %lx\n"
           "ID   : %s\n"
           "FPU  : %lx\n"
           "XMMI : %lx\n"
           "Fxsr : %lx\n"
           "Feat : %lx\n"
           "Ftrs : %lx\n"
           "Cache: %lx\n"
           "CR0  : %lx\n"
           "CR4  : %lx\n",
           Prcb->CpuID,
           Prcb->CpuStep,
           Prcb->CpuType,
           Prcb->VendorString,
           KeI386NpxPresent,
           KeI386XMMIPresent,
           KeI386FxsrPresent,
           Prcb->FeatureBits,
           KeFeatureBits,
           Pcr->SecondLevelCacheSize,
           Ke386GetCr0(),
           Ke386GetCr4());

   /* Set IDT to writable */
   Protect = MmGetPageProtect(NULL, (PVOID)KiIdt);
   MmSetPageProtect(NULL, (PVOID)KiIdt, Protect | PAGE_IS_WRITABLE);
}
