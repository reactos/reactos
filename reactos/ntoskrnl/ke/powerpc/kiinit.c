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

/* GLOBALS *******************************************************************/

/* Ku bit should be set, so that we get the best options for page protection */
#define PPC_SEG_Ku 0x40000000
#define PPC_SEG_Ks 0x20000000

extern LOADER_MODULE KeLoaderModules[64];
extern ULONG KeLoaderModuleCount;
extern ULONG_PTR MmFreeLdrLastKernelAddress;
KPRCB PrcbData[MAXIMUM_PROCESSORS];

/* FUNCTIONS *****************************************************************/

extern void SetPhysByte(ULONG_PTR address, char value);

VOID
DrawDigit(struct _boot_infos_t *BootInfo, ULONG Digit, int x, int y)
{
    int i,j,k;

    for( i = 0; i < 7; i++ ) {
	for( j = 0; j < 8; j++ ) {
	    for( k = 0; k < BootInfo->dispDeviceDepth/8; k++ ) {
		SetPhysByte(((ULONG_PTR)BootInfo->dispDeviceBase)+
			    k +
			    (((j+x) * (BootInfo->dispDeviceDepth/8)) +
			     ((i+y) * (BootInfo->dispDeviceRowBytes))),
			    BootInfo->dispFont[Digit][i*8+j] == 'X' ? 255 : 0);
	    }
	}
    }
}

VOID
DrawNumber(struct _boot_infos_t *BootInfo, ULONG Number, int x, int y)
{
    int i;

    for( i = 0; i < 8; i++, Number<<=4 ) {
	DrawDigit(BootInfo,(Number>>28)&0xf,x+(i*8),y);
    }
}

VOID
DrawString(struct _boot_infos_t *BootInfo, const char *str, int x, int y)
{
    int i, xx;
    
    for( i = 0; str[i]; i++ ) {
	xx = x + (i * 8);
	if( str[i] >= '0' && str[i] <= '9' ) 
	    DrawDigit(BootInfo, str[i] - '0', xx, y);
	else if( str[i] >= 'A' && str[i] <= 'Z' )
	    DrawDigit(BootInfo, str[i] - 'A' + 10, xx, y);
	else if( str[i] >= 'a' && str[i] <= 'z' )
	    DrawDigit(BootInfo, str[i] - 'a' + 10, xx, y);
	else
	    DrawDigit(BootInfo, 37, xx, y);
    }
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
    Pcr->Prcb = PrcbData;
    Pcr->Prcb->MajorVersion = 1;
    Pcr->Prcb->MinorVersion = 1;
    Pcr->Prcb->Number = 0; /* UP for now */
    Pcr->Prcb->SetMember = 1;
    Pcr->Prcb->BuildType = 0;
    Pcr->Prcb->DpcStack = DpcStack;
    KiProcessorBlock[ProcessorNumber] = Pcr->Prcb;
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

#if 0
    /* Setup the Idle Thread */
    KeInitializeThread(InitProcess,
                       InitThread,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       IdleStack);
#endif
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
        DpcStack = MmCreateKernelStack(FALSE);
        if (!DpcStack) KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
        Prcb->DpcStack = DpcStack;
    }

    /* Free Initial Memory */
    MiFreeInitMemory();

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

/* translate address */
int PpcVirt2phys( int virt, int inst );
/* Add a new page table entry for the indicated mapping */
BOOLEAN InsertPageEntry( int virt, int phys, int slot, int _sdr1 );
void SetBat( int bat, int inst, int hi, int lo );
void SetSR( int n, int val );

/* Use this for early boot additions to the page table */
VOID
NTAPI
KiSystemStartup(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Cpu, PhysicalPage, i;
    PKIPCR Pcr = (PKIPCR)KPCR_BASE;
    PKPRCB Prcb;

    /* Zero bats for now ... We may use these for something later */
    for( i = 0; i < 4; i++ ) {
	SetBat( i, 0, 0, 0 );
	SetBat( i, 1, 0, 0 );
    }

    /* Set up segs for normal paged address space. */
    for( i = 0; i < 16; i++ ) {
	SetSR(i, (i < 8 ? PPC_SEG_Ku : PPC_SEG_Ks) | i);
    }

    /* Save the loader block and get the current CPU */
    //KeLoaderBlock = LoaderBlock;
    Cpu = KeNumberProcessors;
    if (!Cpu)
    {
	/* Skippable initialization for secondary processor */
	/* We'll allocate a page from the end of the kernel area for KPCR.  This code will probably
	 * change when we get SMP support.
	 */
	PhysicalPage = PpcVirt2phys(MmFreeLdrLastKernelAddress, FALSE);
	MmFreeLdrLastKernelAddress += (1<<PAGE_SHIFT);
	InsertPageEntry((ULONG)Pcr, PhysicalPage, 0, 0);
	
	PhysicalPage = PpcVirt2phys(MmFreeLdrLastKernelAddress, FALSE);
	MmFreeLdrLastKernelAddress += (1<<PAGE_SHIFT);
	InsertPageEntry((ULONG)KI_USER_SHARED_DATA, PhysicalPage, 0, 0);
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
    Prcb = Pcr->Prcb;
    Pcr->Number = Cpu;
    Pcr->SetMember = 1 << Cpu;
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

