/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Initialize the x86 HAL
 * COPYRIGHT:   Copyright 2011 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"
#include "apicacpi.h"

VOID
NTAPI
ApicInitializeLocalApic(ULONG Cpu);

/* GLOBALS ******************************************************************/

#ifdef _M_IX86
PKPCR HalpProcessorPCR[MAX_CPUS];

ADDRESS_USAGE HalpDefaultIoSpace =
{
    NULL, CmResourceTypePort, IDT_INTERNAL,
    {
        {0x00,  0x20}, /* DMA 1 */
        {0xC0,  0x20}, /* DMA 2 */
        {0x80,  0x10}, /* DMA EPAR */
        {0x20,  0x2},  /* PIC 1 */
        {0xA0,  0x2},  /* PIC 2 */
        {0x40,  0x4},  /* PIT 1 */
        {0x48,  0x4},  /* PIT 2 */
        {0x92,  0x1},  /* System Control Port A */
        {0x70,  0x2},  /* CMOS  */
        {0xF0,  0x10}, /* x87 Coprocessor */
        {0xCF8, 0x8},  /* PCI 0 */
        {0,0},
    }
};
#endif

const USHORT HalpBuildType = HAL_BUILD_TYPE;

HALP_MP_INFO_TABLE HalpMpInfoTable;
PLOCAL_APIC HalpProcLocalApicTable = NULL;
KAFFINITY HalpNodeProcessorAffinity[MAX_CPUS] = {0};
ULONG HalpDefaultApicDestinationModeMask = 0x800;
ULONG HalpHybridApicPhysicalTargets = 0;
UCHAR HalpIntDestMap[MAX_CPUS] = {0};
UCHAR HalpMaxProcsPerCluster = 0;
UCHAR HalpInitLevel = 0xFF;
UCHAR HalpMaxNode = 0;
BOOLEAN HalpForceApicPhysicalDestinationMode = FALSE;

/* FUNCTIONS ****************************************************************/

#ifdef _M_IX86
VOID
NTAPI
HalInitApicInterruptHandlers()
{
    KDESCRIPTOR IdtDescriptor;
    PKIDTENTRY Idt;

    __sidt(&IdtDescriptor.Limit);
    Idt = (PKIDTENTRY)IdtDescriptor.Base;

    Idt[0x37].Offset = PtrToUlong(PicSpuriousService37);
    Idt[0x37].Selector = KGDT_R0_CODE;
    Idt[0x37].Access = 0x8E00;
    Idt[0x37].ExtendedOffset = (PtrToUlong(PicSpuriousService37) >> 16);

    Idt[0x1F].Offset = PtrToUlong(ApicSpuriousService);
    Idt[0x1F].Selector = KGDT_R0_CODE;
    Idt[0x1F].Access = 0x8E00;
    Idt[0x1F].ExtendedOffset = (PtrToUlong(ApicSpuriousService) >> 16);
}

UCHAR
NTAPI
HalpMapNtToHwProcessorId(UCHAR Number)
{
    ASSERT(HalpForceApicPhysicalDestinationMode == FALSE);

    if (!HalpMaxProcsPerCluster)
    {
        ASSERT(Number < 8);
        return (1 << Number);
    }

    // FIXME
    DbgBreakPoint();
    return 0;
}

UCHAR
NTAPI
HalpNodeNumber(PKPCR Pcr)
{
    UCHAR NodeNumber = 0;
    UCHAR DestMap;

    if (HalpForceApicPhysicalDestinationMode)
    {
        NodeNumber = Pcr->Prcb->Number;
        return (NodeNumber + 1);
    }

    if (!HalpMaxProcsPerCluster)
    {
        return (NodeNumber + 1);
    }

    DestMap = HalpIntDestMap[Pcr->Prcb->Number];

    if (DestMap)
    {
        NodeNumber = (DestMap >> 4);
        return (NodeNumber + 1);
    }

    return 0;
}

VOID
NTAPI
HalpInitializeApicAddressing()
{
    PKPCR Pcr = KeGetPcr();
    PKPRCB Prcb = (PKPRCB)Pcr->Prcb;
    UCHAR PrcNumber = Prcb->Number;
    UCHAR NodeNumber;
    UCHAR DestMap;

    if (HalpForceApicPhysicalDestinationMode)
    {
        ApicWrite(APIC_DFR, 0x0FFFFFFF);
        ApicWrite(APIC_LDR, 0);
    }
    else
    {
        if (HalpMaxProcsPerCluster)
        {
            ApicWrite(APIC_DFR, 0x0FFFFFFF);
        }
        else
        {
            ApicWrite(APIC_DFR, 0xFFFFFFFF);
        }

        DestMap = HalpMapNtToHwProcessorId(PrcNumber);
        HalpIntDestMap[PrcNumber] = DestMap;
        ApicWrite(APIC_LDR, ((ULONG)DestMap << 24));
    }

    NodeNumber = HalpNodeNumber(Pcr);

    if (HalpMaxNode < NodeNumber)
    {
        HalpMaxNode = NodeNumber;
    }

    ASSERT(HalpMaxNode);

    if (NodeNumber)
    {
        HalpNodeProcessorAffinity[NodeNumber - 1] |= (1 << PrcNumber);
    }
    else
    {
        HalpHybridApicPhysicalTargets |= (1 << PrcNumber);
    }
}

VOID
NTAPI
HalpMarkProcessorStarted(
    _In_ UCHAR Id,
    _In_ ULONG PrcNumber)
{
    ULONG ix;

    for (ix = 0; ix < HalpMpInfoTable.ProcessorCount; ix++)
    {
        if (HalpProcLocalApicTable[ix].Id == Id)
        {
            HalpProcLocalApicTable[ix].ProcessorStarted = TRUE;
            HalpProcLocalApicTable[ix].ProcessorNumber = PrcNumber;

            if (PrcNumber == 0)
            {
                HalpProcLocalApicTable[ix].FirstProcessor = TRUE;
            }

            break;
        }
    }
}

VOID
NTAPI
HalpBuildIpiDestinationMap(ULONG ProcessorNumber)
{
    if (HalpInitLevel == 0xFF)
    {
        return;
    }

    if (HalpForceApicPhysicalDestinationMode)
    {
        DPRINT1("HalpBuildIpiDestinationMap: [%X] FIXME\n", HalpInitLevel);
    }
    else if (HalpMaxProcsPerCluster)
    {
        DPRINT1("HalpBuildIpiDestinationMap: [%X] FIXME\n", HalpInitLevel);
    }
    else
    {
        DPRINT1("HalpBuildIpiDestinationMap: [%X] FIXME\n", HalpInitLevel);
    }
}

VOID
NTAPI
HalpInitializeLocalUnit()
{
    APIC_SPURIOUS_INERRUPT_REGISTER SpIntRegister;
    APIC_COMMAND_REGISTER CommandRegister;
    LVT_REGISTER LvtEntry;
    ULONG EFlags = __readeflags();
    PKPRCB Prcb;
    UCHAR Id;

    _disable();

    Prcb = KeGetPcr()->Prcb;

    if (Prcb->Number == 0)
    {
        /* MultiProcessor Specification, Table 4-1.
           MP Floating Pointer Structure Fields (MP FEATURE INFORMATION BYTE 2) Bit 7:IMCRP
           If TRUE - PIC Mode, if FALSE - Virtual Wire Mode
        */
        if (HalpMpInfoTable.ImcrPresent)
        {
            /* Enable PIC mode to Processor via APIC */
            WRITE_PORT_UCHAR(IMCR_ADDRESS_PORT, IMCR_SELECT);
            WRITE_PORT_UCHAR(IMCR_DATA_PORT, IMCR_PIC_VIA_APIC);
        }

        if ((UCHAR)HalpMaxProcsPerCluster > 4 ||
            (HalpMaxProcsPerCluster == 0 && HalpMpInfoTable.ProcessorCount > 8))
        {
            HalpMaxProcsPerCluster = 4;
        }

        if (HalpMpInfoTable.LocalApicversion == 0)
        {
            ASSERT(HalpMpInfoTable.ProcessorCount <= 8);
            HalpMaxProcsPerCluster = 0;
        }
    }

    ApicWrite(APIC_TPR, 0xFF);

    HalpInitializeApicAddressing();
    Id = (UCHAR)((ApicRead(APIC_ID)) >> 24);
    HalpMarkProcessorStarted(Id, Prcb->Number);

    KeRegisterInterruptHandler(APIC_SPURIOUS_VECTOR, ApicSpuriousService);

    SpIntRegister.Long = 0;
    SpIntRegister.Vector = APIC_SPURIOUS_VECTOR;
    SpIntRegister.SoftwareEnable = 1;
    ApicWrite(APIC_SIVR, SpIntRegister.Long);

    if (HalpMpInfoTable.LocalApicversion)
    {
        // FIXME UNIMPLIMENTED;
        ASSERT(FALSE);
    }

    LvtEntry.Long = 0;
    LvtEntry.Vector = APIC_PROFILE_VECTOR;
    LvtEntry.Mask = 1;
    LvtEntry.TimerMode = 1;
    ApicWrite(APIC_TMRLVTR, LvtEntry.Long);

    LvtEntry.Long = 0;
    LvtEntry.Vector = APIC_PERF_VECTOR;
    LvtEntry.Mask = 1;
    LvtEntry.TimerMode = 0;
    ApicWrite(APIC_PCLVTR, LvtEntry.Long);

    LvtEntry.Long = 0;
    LvtEntry.Vector = APIC_SPURIOUS_VECTOR;
    LvtEntry.Mask = 1;
    LvtEntry.TimerMode = 0;
    ApicWrite(APIC_LINT0, LvtEntry.Long);

    LvtEntry.Long = 0;
    LvtEntry.Vector = APIC_NMI_VECTOR;
    LvtEntry.Mask = 1;
    LvtEntry.TimerMode = 0;
    LvtEntry.MessageType = APIC_MT_NMI;
    LvtEntry.TriggerMode = APIC_TGM_Level;
    ApicWrite(APIC_LINT1, LvtEntry.Long);

    CommandRegister.Long0 = 0;
    CommandRegister.Vector = ZERO_VECTOR;
    CommandRegister.MessageType = APIC_MT_INIT;
    CommandRegister.TriggerMode = APIC_TGM_Level;
    CommandRegister.DestinationShortHand = APIC_DSH_AllIncludingSelf;
    ApicWrite(APIC_ICR0, CommandRegister.Long0);

    HalpBuildIpiDestinationMap(Prcb->Number);

    ApicWrite(APIC_TPR, 0x00);

    if (EFlags & EFLAGS_INTERRUPT_MASK)
    {
        _enable();
    }
}
#endif

#ifdef _M_IX86
VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PKPCR Pcr = KeGetPcr();

    /* Set default IDR */
    Pcr->IDR = 0xFFFFFFFF;

    *(PUCHAR)(Pcr->HalReserved) = ProcessorNumber; // FIXME
    HalpProcessorPCR[ProcessorNumber] = Pcr;

    // FIXME support '/INTAFFINITY' key in .ini

    /* By default, the HAL allows interrupt requests to be received by all processors */
    InterlockedBitTestAndSet((PLONG)&HalpDefaultInterruptAffinity, ProcessorNumber);

    if (ProcessorNumber == 0)
    {
        if (!DetectMP(KeLoaderBlock))
        {
            __halt();
        }

        /* Register routines for KDCOM */
        HalpRegisterKdSupportFunctions();

        // FIXME HalpGlobal8259Mask

        WRITE_PORT_UCHAR((PUCHAR)PIC1_DATA_PORT, 0xFF);
        WRITE_PORT_UCHAR((PUCHAR)PIC2_DATA_PORT, 0xFF);
    }

    HalInitApicInterruptHandlers();
    HalpInitializeLocalUnit();
}

NTSTATUS
NTAPI
HalpSetSystemInformation(IN HAL_SET_INFORMATION_CLASS InformationClass,
                         IN ULONG BufferSize,
                         IN OUT PVOID Buffer)
{
    DPRINT1("HalpSetSystemInformation()\n");
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Fill out HalDispatchTable */
    HalSetSystemInformation = HalpSetSystemInformation;

    /* Do some APIC HAL-specific initialization */
    HalpInitPhase0a(LoaderBlock);
}
#else
VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Initialize the local APIC for this cpu */
    ApicInitializeLocalApic(ProcessorNumber);

    /* Initialize profiling data (but don't start it) */
    HalInitializeProfiling();

    /* Initialize the timer */
    //ApicInitializeTimer(ProcessorNumber);

    /* Update the interrupt affinity */
    InterlockedBitTestAndSet((PLONG)&HalpDefaultInterruptAffinity,
                             ProcessorNumber);

    /* Register routines for KDCOM */
    HalpRegisterKdSupportFunctions();
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Initialize ACPI */
    HalpSetupAcpiPhase0(LoaderBlock);

    /* Initialize the PICs */
    HalpInitializePICs(TRUE);

    /* Initialize CMOS lock */
    KeInitializeSpinLock(&HalpSystemHardwareLock);

    /* Initialize CMOS */
    HalpInitializeCmos();

    /* Setup busy waiting */
    HalpCalibrateStallExecution();

    /* Initialize the clock */
    HalpInitializeClock();

    /*
     * We could be rebooting with a pending profile interrupt,
     * so clear it here before interrupts are enabled
     */
    HalStopProfileInterrupt(ProfileTime);

    /* Do some HAL-specific initialization */
    HalpInitPhase0(LoaderBlock);

    /* Enable clock interrupt handler */
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);
}
#endif

VOID
HalpInitPhase1(VOID)
{
    /* Initialize DMA. NT does this in Phase 0 */
    HalpInitDma();
}

/* EOF */
