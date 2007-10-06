/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/powerpc/kiinit.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Art Yerkes (ayerkes@speakeasy.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <reactos/ppcboot.h>

#define NDEBUG
#include <debug.h>
#include <ppcdebug.h>
#include "ppcmmu/mmu.h"

/* GLOBALS *******************************************************************/

/* Ku bit should be set, so that we get the best options for page protection */
#define PPC_SEG_Ku 0x40000000
#define PPC_SEG_Ks 0x20000000

extern LOADER_MODULE KeLoaderModules[64];
extern ULONG KeLoaderModuleCount;
extern ULONG_PTR MmFreeLdrLastKernelAddress;
KPRCB PrcbData[MAXIMUM_PROCESSORS];

/* FUNCTIONS *****************************************************************/

/*
 * Trap frame:
 * r0 .. r32
 * lr, ctr, srr0, srr1, dsisr
 */
__asm__(".text\n\t"
	".globl syscall_start\n\t"
	".globl syscall_end\n\t"
	".globl KiSystemService\n\t"
	"syscall_start:\n\t"
	"mr 2,1\n\t"
	"lis 1,KiSystemService1@ha\n\t"
	"addi 1,1,KiSystemService1@l\n\t"
	"mfsrr0 0\n\t"
	"mtsrr0 1\n\t"
	"lis 1,_kernel_trap_stack@ha\n\t"
	"addi 1,1,_kernel_trap_stack@l\n\t"
	"subi 1,1,0x100\n\t"
	"rfi\n\t"
	"syscall_end:\n\t"
	".space 4");

extern int syscall_start[], syscall_end;

VOID
NTAPI
KiSetupSyscallHandler()
{
    paddr_t handler_target;
    int *source;
    for(source = syscall_start, handler_target = 0xc00; 
	source < &syscall_end; 
	source++, handler_target += sizeof(int))
	SetPhys(handler_target, *source);
}

VOID
NTAPI
KiInitializePcr(IN ULONG ProcessorNumber,
                IN PKIPCR Pcr,
                IN PKTHREAD IdleThread,
                IN PVOID DpcStack)
{
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;
    Pcr->CurrentIrql = PASSIVE_LEVEL;
    Pcr->PrcbData = &PrcbData[ProcessorNumber];
    Pcr->PrcbData->MajorVersion = PRCB_MAJOR_VERSION;
    Pcr->PrcbData->MinorVersion = 0;
    Pcr->PrcbData->Number = 0; /* UP for now */
    Pcr->PrcbData->SetMember = 1;
#if DBG
    Pcr->PrcbData->BuildType = PRCB_BUILD_DEBUG;
#else
    Pcr->PrcbData->BuildType = 0;
#endif
    Pcr->PrcbData->DpcStack = DpcStack;
    KiProcessorBlock[ProcessorNumber] = Pcr->PrcbData;
}

extern ULONG KiGetFeatureBits();
extern VOID KiSetProcessorType();
extern VOID KiGetCacheInformation();

VOID
NTAPI
KiInitializeKernel(IN PKPROCESS InitProcess,
                   IN PKTHREAD InitThread,
                   IN PVOID IdleStack,
                   IN PKPRCB Prcb,
                   IN CCHAR Number,
                   IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG FeatureBits;
    LARGE_INTEGER PageDirectory;
    PVOID DpcStack;

    /* Detect and set the CPU Type */
    KiSetProcessorType();

    /* Initialize the Power Management Support for this PRCB */
    PoInitializePrcb(Prcb);

    /* Get the processor features for the CPU */
    FeatureBits = KiGetFeatureBits();

    /* Save feature bits */
    Prcb->FeatureBits = FeatureBits;

    /* Get cache line information for this CPU */
    KiGetCacheInformation();

    /* Initialize spinlocks and DPC data */
    KiInitSpinLocks(Prcb, Number);

    /* Check if this is the Boot CPU */
    if (!Number)
    {
        /* Set Node Data */
        KeNodeBlock[0] = &KiNode0;
        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        /* Set boot-level flags */
        KeProcessorArchitecture = 0;
        KeProcessorLevel = (USHORT)Prcb->CpuType;
        KeFeatureBits = FeatureBits;

        /* Set the current MP Master KPRCB to the Boot PRCB */
        Prcb->MultiThreadSetMaster = Prcb;

        /* Initialize portable parts of the OS */
        KiInitSystem();

        /* Initialize the Idle Process and the Process Listhead */
        InitializeListHead(&KiProcessListHead);
        PageDirectory.QuadPart = 0;
        KeInitializeProcess(InitProcess,
                            0,
                            0xFFFFFFFF,
                            &PageDirectory,
			    TRUE);
        InitProcess->QuantumReset = MAXCHAR;
    }
    else
    {
        /* FIXME */
        DPRINT1("SMP Boot support not yet present\n");
    }

    /* Setup the Idle Thread */
    KeInitializeThread(InitProcess,
                       InitThread,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       IdleStack);
    InitThread->NextProcessor = Number;
    InitThread->Priority = HIGH_PRIORITY;
    InitThread->State = Running;
    InitThread->Affinity = 1 << Number;
    InitThread->WaitIrql = DISPATCH_LEVEL;
    InitProcess->ActiveProcessors = 1 << Number;

    /* Set up the thread-related fields in the PRCB */
    //Prcb->CurrentThread = InitThread;
    Prcb->NextThread = NULL;
    //Prcb->IdleThread = InitThread;

    /* Initialize the Kernel Executive */
    ExpInitializeExecutive(0, (PLOADER_PARAMETER_BLOCK)LoaderBlock);

    /* Only do this on the boot CPU */
    if (!Number)
    {
        /* Calculate the time reciprocal */
        KiTimeIncrementReciprocal =
            KiComputeReciprocal(KeMaximumIncrement,
                                &KiTimeIncrementShiftCount);

        /* Update DPC Values in case they got updated by the executive */
        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

        /* Allocate the DPC Stack */
        DpcStack = MmCreateKernelStack(FALSE, 0);
        if (!DpcStack) KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
        Prcb->DpcStack = DpcStack;
    }

    /* Free Initial Memory */
    // MiFreeInitMemory();

    while (1)
    {
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = 0x7fffffffffffffffLL;
        KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
    }

    /* Bug Check and loop forever if anything failed */
    KEBUGCHECK(0);
    for(;;);
}

extern int KiPageFaultHandler(int inst, ppc_trap_frame_t *frame);

/* Use this for early boot additions to the page table */
VOID
NTAPI
KiSystemStartup(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Cpu;
    ppc_map_info_t info[3];
    PKIPCR Pcr = (PKIPCR)KPCR_BASE;
    PKPRCB Prcb;

    __asm__("mr 13,%0" : : "r" (KPCR_BASE));

    /* Set the page fault handler to the kernel */
    MmuSetPageCallback(KiPageFaultHandler);

    // Make 0xf... special
    MmuAllocVsid(2, 0x8000);
    MmuSetVsid(15,16,2);

    /* Get the current CPU */
    Cpu = KeNumberProcessors;
    if (!Cpu)
    {
	/* We'll allocate a page from the end of the kernel area for KPCR.  This code will probably
	 * change when we get SMP support.
	 */
	info[0].phys = 0;
	info[0].proc = 2;
	info[0].addr = (vaddr_t)Pcr;
	info[0].flags = MMU_KRW_UR;
	info[1].phys = 0;
	info[1].proc = 2;
	info[1].addr = ((vaddr_t)Pcr) + (1 << PAGE_SHIFT);
	info[1].flags = MMU_KRW_UR;
	info[2].phys = 0;
	info[2].proc = 2;
	info[2].addr = (vaddr_t)KI_USER_SHARED_DATA;
	info[2].flags = MMU_KRW_UR;
	MmuMapPage(info, 3);
    }

    /* Skip initial setup if this isn't the Boot CPU */
    if (Cpu) goto AppCpuInit;

    /* Initialize the PCR */
    RtlZeroMemory(Pcr, PAGE_SIZE);
    KiInitializePcr(Cpu,
                    Pcr,
                    &KiInitialThread.Tcb,
                    KiDoubleFaultStack);

    /* Set us as the current process */
    KiInitialThread.Tcb.ApcState.Process = &KiInitialProcess.Pcb;

    /* Setup CPU-related fields */
AppCpuInit:
    Pcr->Number = Cpu;
    Pcr->SetMember = 1 << Cpu;
    Prcb = KeGetCurrentPrcb();
    Prcb->SetMember = 1 << Cpu;

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(Cpu, LoaderBlock);

    /* Set active processors */
    KeActiveProcessors |= Pcr->SetMember;
    KeNumberProcessors++;

    /* Initialize the Debugger for the Boot CPU */
    if (!Cpu) KdInitSystem (0, LoaderBlock);

    /* Check for break-in */
    if (KdPollBreakIn()) 
    {
	DbgBreakPointWithStatus(1);
    }

    /* Raise to HIGH_LEVEL */
    KfRaiseIrql(HIGH_LEVEL);

    /* Call main kernel intialization */
    KiInitializeKernel(&KiInitialProcess.Pcb,
                       &KiInitialThread.Tcb,
                       P0BootStack,
                       Prcb,
                       Cpu,
                       (PVOID)LoaderBlock);
}

VOID
NTAPI
KiInitMachineDependent(VOID)
{
}

void abort()
{
    KeBugCheck(0);
    while(1);
}
