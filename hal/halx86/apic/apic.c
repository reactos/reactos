/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/apic/apic.c
 * PURPOSE:         HAL APIC Management and Control Code
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Vadim Galyant (vgal@rambler.ru)
 * REFERENCES:      http://www.joseflores.com/docs/ExploringIrql.html
 *                  http://www.codeproject.com/KB/system/soviet_kernel_hack.aspx
 *                  http://bbs.unixmap.net/thread-2022-1-1.html
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"
void __cdecl HackEoi(void);

#ifndef _M_AMD64
#include <ioapic.h>
#define APIC_LAZY_IRQL
#endif

/* GLOBALS ********************************************************************/

ULONG ApicVersion;
UCHAR HalpVectorToIndex[256];

#ifndef _M_AMD64
const UCHAR
HalpIRQLtoTPR[32] =
{
    0x00, /*  0 PASSIVE_LEVEL */
    0x3d, /*  1 APC_LEVEL */
    0x41, /*  2 DISPATCH_LEVEL */
    0x41, /*  3 \  */
    0x51, /*  4  \ */
    0x61, /*  5  | */
    0x71, /*  6  | */
    0x81, /*  7  | */
    0x91, /*  8  | */
    0xa1, /*  9  | */
    0xb1, /* 10  | */
    0xb1, /* 11  | */
    0xb1, /* 12  | */
    0xb1, /* 13  | */
    0xb1, /* 14  | */
    0xb1, /* 15 DEVICE IRQL */
    0xb1, /* 16  | */
    0xb1, /* 17  | */
    0xb1, /* 18  | */
    0xb1, /* 19  | */
    0xb1, /* 20  | */
    0xb1, /* 21  | */
    0xb1, /* 22  | */
    0xb1, /* 23  | */
    0xb1, /* 24  | */
    0xb1, /* 25  / */
    0xb1, /* 26 /  */
    0xc1, /* 27 PROFILE_LEVEL */
    0xd1, /* 28 CLOCK2_LEVEL */
    0xe1, /* 29 IPI_LEVEL */
    0xef, /* 30 POWER_LEVEL */
    0xff, /* 31 HIGH_LEVEL */
};

const KIRQL
HalVectorToIRQL[16] =
{
       0, /* 00 PASSIVE_LEVEL */
    0xff, /* 10 */
    0xff, /* 20 */
       1, /* 3D APC_LEVEL */
       2, /* 41 DISPATCH_LEVEL */
       4, /* 50 \ */
       5, /* 60  \ */
       6, /* 70  | */
       7, /* 80 DEVICE IRQL */
       8, /* 90  | */
       9, /* A0  / */
      10, /* B0 /  */
      27, /* C1 PROFILE_LEVEL */
      28, /* D1 CLOCK2_LEVEL */
      29, /* E1 IPI_LEVEL / EF POWER_LEVEL */
      31, /* FF HIGH_LEVEL */
};

/* For 0x50..0xBF vectors IRQLs values saves dynamically in HalpAllocateSystemInterruptVector() */
KIRQL HalpVectorToIRQL[16] =
{
    0x00, /* 00 PASSIVE_LEVEL */
    0xFF, /* 10 */
    0xFF, /* 20 */
    0x01, /* 3D APC_LEVEL */
    0x02, /* 41 DISPATCH_LEVEL */
    0xFF, /* 50 \ */
    0xFF, /* 60  \ */
    0xFF, /* 70  | */
    0xFF, /* 80 DEVICE IRQL */
    0xFF, /* 90  | */
    0xFF, /* A0  / */
    0xFF, /* B0 /  */
    0x1B, /* C1 PROFILE_LEVEL */
    0x1C, /* D1 CLOCK2_LEVEL */
    0x1D, /* E1 IPI_LEVEL / EF POWER_LEVEL */
    0x1F, /* FF HIGH_LEVEL */
};

#define HALP_DEVICE_INT_PRIORITY_LEVEL_BASE  5
#define HALP_DEVICE_INT_PRIORITY_LEVEL_COUNT 7
#define HALP_MAX_PRIORITY_LEVEL              15

#define HALP_DEV_INT_EDGE   0
#define HALP_DEV_INT_LEVEL  1

#define DL_EDGE_SENSITIVE    0
#define DL_LEVEL_SENSITIVE   1
//#define DL_INVERT_SENSITIVE  0x80

/* UCHAR HalpDevLevel[InterruptMode][TriggerMode] */
UCHAR HalpDevLevel[2][2] =
{
    /* Edge */           /* Level */
    {DL_EDGE_SENSITIVE,  DL_EDGE_SENSITIVE},  // Latched
    {DL_LEVEL_SENSITIVE, DL_LEVEL_SENSITIVE}  // LevelSensitive
}; 

#define DP_LOW_ACTIVE   0
#define DP_HIGH_ACTIVE  1

/* UCHAR HalpDevPolarity[Polarity][TriggerMode] */
UCHAR HalpDevPolarity[4][2] =
{
    /* Edge */       /* Level */
    {DP_HIGH_ACTIVE, DP_LOW_ACTIVE},  // POLARITY_CONFORMS 
    {DP_HIGH_ACTIVE, DP_HIGH_ACTIVE}, // POLARITY_ACTIVE_HIGH
    {DP_HIGH_ACTIVE, DP_LOW_ACTIVE},  // POLARITY_RESERVED
    {DP_LOW_ACTIVE,  DP_LOW_ACTIVE}   // POLARITY_ACTIVE_LOW
}; 

ULONGLONG HalpProc0TSCHz;
USHORT HalpVectorToINTI[MAX_CPUS * MAX_INT_VECTORS] = {0xFFFF};
APIC_INTI_INFO HalpIntiInfo[MAX_INTI];
ULONG HalpINTItoVector[MAX_INTI] = {0};
ULONG HalpDefaultApicDestinationModeMask = 0x800;
KSPIN_LOCK HalpAccountingLock;
BOOLEAN HalpForceApicPhysicalDestinationMode = FALSE;
BOOLEAN HalpForceClusteredApicMode = FALSE;
BOOLEAN HalpUse8254 = FALSE;

#define SUPPORTED_NODES      32
#define PRIORITY_LEVEL_BASE  5
#define PRIORITY_LEVEL_COUNT 7

UCHAR HalpNodeInterruptCount[SUPPORTED_NODES] = {0};
UCHAR HalpNodePriorityLevelUsage[SUPPORTED_NODES][PRIORITY_LEVEL_COUNT] = {{0}};

typedef VOID (*PINTERRUPT_ENTRY)(VOID);
extern PINTERRUPT_ENTRY HwInterruptTable[MAX_INT_VECTORS];

extern KAFFINITY HalpNodeProcessorAffinity[MAX_CPUS];
extern HALP_MP_INFO_TABLE HalpMpInfoTable;
extern USHORT HalpMaxApicInti[MAX_IOAPICS];
extern UCHAR HalpIntDestMap[MAX_CPUS];
extern UCHAR HalpMaxProcsPerCluster;
extern UCHAR HalpMaxNode;
#endif

/* PRIVATE FUNCTIONS **********************************************************/

FORCEINLINE
ULONG
IOApicRead(UCHAR Register)
{
    /* Select the register, then do the read */
    *(volatile UCHAR *)(IOAPIC_BASE + IOAPIC_IOREGSEL) = Register;
    return *(volatile ULONG *)(IOAPIC_BASE + IOAPIC_IOWIN);
}

FORCEINLINE
VOID
IOApicWrite(UCHAR Register, ULONG Value)
{
    /* Select the register, then do the write */
    *(volatile UCHAR *)(IOAPIC_BASE + IOAPIC_IOREGSEL) = Register;
    *(volatile ULONG *)(IOAPIC_BASE + IOAPIC_IOWIN) = Value;
}

FORCEINLINE
VOID
ApicWriteIORedirectionEntry(
    UCHAR Index,
    IOAPIC_REDIRECTION_REGISTER ReDirReg)
{
    IOApicWrite(IOAPIC_REDTBL + 2 * Index, ReDirReg.Long0);
    IOApicWrite(IOAPIC_REDTBL + 2 * Index + 1, ReDirReg.Long1);
}

FORCEINLINE
IOAPIC_REDIRECTION_REGISTER
ApicReadIORedirectionEntry(
    UCHAR Index)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;

    ReDirReg.Long0 = IOApicRead(IOAPIC_REDTBL + 2 * Index);
    ReDirReg.Long1 = IOApicRead(IOAPIC_REDTBL + 2 * Index + 1);

    return ReDirReg;
}

FORCEINLINE
VOID
ApicRequestInterrupt(IN UCHAR Vector, UCHAR TriggerMode)
{
    APIC_COMMAND_REGISTER CommandRegister;

    /* Setup the command register */
    CommandRegister.Long0 = 0;
    CommandRegister.Vector = Vector;
    CommandRegister.MessageType = APIC_MT_Fixed;
    CommandRegister.TriggerMode = TriggerMode;
    CommandRegister.DestinationShortHand = APIC_DSH_Self;

    /* Write the low dword to send the interrupt */
    ApicWrite(APIC_ICR0, CommandRegister.Long0);
}

FORCEINLINE
VOID
ApicSendEOI(void)
{
    //ApicWrite(APIC_EOI, 0);
    HackEoi();
}

FORCEINLINE
KIRQL
ApicGetProcessorIrql(VOID)
{
    /* Read the TPR and convert it to an IRQL */
    return TprToIrql(ApicRead(APIC_PPR));
}

FORCEINLINE
KIRQL
ApicGetCurrentIrql(VOID)
{
#ifdef _M_AMD64
    return (KIRQL)__readcr8();
#elif defined(APIC_LAZY_IRQL)
    // HACK: some magic to Sync VBox's APIC registers
    ApicRead(APIC_VER);

    /* Return the field in the PCR */
    return (KIRQL)__readfsbyte(FIELD_OFFSET(KPCR, Irql));
#else
    // HACK: some magic to Sync VBox's APIC registers
    ApicRead(APIC_VER);

    /* Read the TPR and convert it to an IRQL */
    return TprToIrql(ApicRead(APIC_TPR));
#endif
}

FORCEINLINE
VOID
ApicSetIrql(KIRQL Irql)
{
#ifdef _M_AMD64
    __writecr8(Irql);
#elif defined(APIC_LAZY_IRQL)
    __writefsbyte(FIELD_OFFSET(KPCR, Irql), Irql);
#else
    /* Convert IRQL and write the TPR */
    ApicWrite(APIC_TPR, IrqlToTpr(Irql));
#endif
}
#define ApicRaiseIrql ApicSetIrql

#if 0
#ifdef APIC_LAZY_IRQL
FORCEINLINE
VOID
ApicLowerIrql(KIRQL Irql)
{
    __writefsbyte(FIELD_OFFSET(KPCR, Irql), Irql);

    /* Is the new Irql lower than set in the TPR? */
    if (Irql < KeGetPcr()->IRR)
    {
        /* Save the new hard IRQL in the IRR field */
        KeGetPcr()->IRR = Irql;

        /* Need to lower it back */
        ApicWrite(APIC_TPR, IrqlToTpr(Irql));
    }
}
#else
#define ApicLowerIrql ApicSetIrql
#endif
#endif

UCHAR
FASTCALL
HalpIrqToVector(UCHAR Irq)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;

    /* Read low dword of the redirection entry */
    ReDirReg.Long0 = IOApicRead(IOAPIC_REDTBL + 2 * Irq);

    /* Return the vector */
    return (UCHAR)ReDirReg.Vector;
}

KIRQL
FASTCALL
HalpVectorToIrql(UCHAR Vector)
{
    return TprToIrql(Vector);
}

UCHAR
FASTCALL
HalpVectorToIrq(UCHAR Vector)
{
    return HalpVectorToIndex[Vector];
}

VOID
NTAPI
HalpSendEOI(VOID)
{
    ApicSendEOI();
}

#ifdef _M_AMD64
VOID
NTAPI
ApicInitializeLocalApic(ULONG Cpu)
{
    APIC_BASE_ADRESS_REGISTER BaseRegister;
    APIC_SPURIOUS_INERRUPT_REGISTER SpIntRegister;
    LVT_REGISTER LvtEntry;

    /* Enable the APIC if it wasn't yet */
    BaseRegister.Long = __readmsr(MSR_APIC_BASE);
    BaseRegister.Enable = 1;
    BaseRegister.BootStrapCPUCore = (Cpu == 0);
    __writemsr(MSR_APIC_BASE, BaseRegister.Long);

    /* Set spurious vector and SoftwareEnable to 1 */
    SpIntRegister.Long = ApicRead(APIC_SIVR);
    SpIntRegister.Vector = APIC_SPURIOUS_VECTOR;
    SpIntRegister.SoftwareEnable = 1;
    SpIntRegister.FocusCPUCoreChecking = 0;
    ApicWrite(APIC_SIVR, SpIntRegister.Long);

    /* Read the version and save it globally */
    if (Cpu == 0) ApicVersion = ApicRead(APIC_VER);

    /* Set the mode to flat (max 8 CPUs supported!) */
    ApicWrite(APIC_DFR, APIC_DF_Flat);

    /* Set logical apic ID */
    ApicWrite(APIC_LDR, ApicLogicalId(Cpu) << 24);

    /* Set the spurious ISR */
    KeRegisterInterruptHandler(APIC_SPURIOUS_VECTOR, ApicSpuriousService);

    /* Create a template LVT */
    LvtEntry.Long = 0;
    LvtEntry.Vector = 0xFF;
    LvtEntry.MessageType = APIC_MT_Fixed;
    LvtEntry.DeliveryStatus = 0;
    LvtEntry.RemoteIRR = 0;
    LvtEntry.TriggerMode = APIC_TGM_Edge;
    LvtEntry.Mask = 1;
    LvtEntry.TimerMode = 0;

    /* Initialize and mask LVTs */
    ApicWrite(APIC_TMRLVTR, LvtEntry.Long);
    ApicWrite(APIC_THRMLVTR, LvtEntry.Long);
    ApicWrite(APIC_PCLVTR, LvtEntry.Long);
    ApicWrite(APIC_EXT0LVTR, LvtEntry.Long);
    ApicWrite(APIC_EXT1LVTR, LvtEntry.Long);
    ApicWrite(APIC_EXT2LVTR, LvtEntry.Long);
    ApicWrite(APIC_EXT3LVTR, LvtEntry.Long);

    /* LINT0 */
    LvtEntry.Vector = APIC_SPURIOUS_VECTOR;
    LvtEntry.MessageType = APIC_MT_ExtInt;
    ApicWrite(APIC_LINT0, LvtEntry.Long);

    /* Enable LINT1 (NMI) */
    LvtEntry.Mask = 0;
    LvtEntry.Vector = APIC_NMI_VECTOR;
    LvtEntry.MessageType = APIC_MT_NMI;
    LvtEntry.TriggerMode = APIC_TGM_Level;
    ApicWrite(APIC_LINT1, LvtEntry.Long);

    /* Enable error LVTR */
    LvtEntry.Vector = APIC_ERROR_VECTOR;
    LvtEntry.MessageType = APIC_MT_Fixed;
    ApicWrite(APIC_ERRLVTR, LvtEntry.Long);

    /* Set the IRQL from the PCR */
    ApicSetIrql(KeGetPcr()->Irql);
#ifdef APIC_LAZY_IRQL
    /* Save the new hard IRQL in the IRR field */
    KeGetPcr()->IRR = KeGetPcr()->Irql;
#endif
}

UCHAR
NTAPI
HalpAllocateSystemInterrupt(
    IN UCHAR Irq,
    IN KIRQL Irql)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    IN UCHAR Vector;

    /* Start with lowest vector */
    Vector = IrqlToTpr(Irql) & 0xF0;

    /* Find an empty vector */
    while (HalpVectorToIndex[Vector] != 0xFF)
    {
        Vector++;

        /* Check if we went over the edge */
        if (TprToIrql(Vector) > Irql)
        {
            /* Nothing free, return failure */
            return 0;
        }
    }

    /* Save irq in the table */
    HalpVectorToIndex[Vector] = Irq;

    /* Setup a redirection entry */
    ReDirReg.Vector = Vector;
    ReDirReg.DeliveryMode = APIC_MT_LowestPriority;
    ReDirReg.DestinationMode = APIC_DM_Logical;
    ReDirReg.DeliveryStatus = 0;
    ReDirReg.Polarity = 0;
    ReDirReg.RemoteIRR = 0;
    ReDirReg.TriggerMode = APIC_TGM_Edge;
    ReDirReg.Mask = 1;
    ReDirReg.Reserved = 0;
    ReDirReg.Destination = 0;

    /* Initialize entry */
    IOApicWrite(IOAPIC_REDTBL + 2 * Irq, ReDirReg.Long0);
    IOApicWrite(IOAPIC_REDTBL + 2 * Irq + 1, ReDirReg.Long1);

    return Vector;
}

VOID
NTAPI
ApicInitializeIOApic(VOID)
{
    PHARDWARE_PTE Pte;
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    UCHAR Index;
    ULONG Vector;

    /* Map the I/O Apic page */
    Pte = HalAddressToPte(IOAPIC_BASE);
    Pte->PageFrameNumber = IOAPIC_PHYS_BASE / PAGE_SIZE;
    Pte->Valid = 1;
    Pte->Write = 1;
    Pte->Owner = 1;
    Pte->CacheDisable = 1;
    Pte->Global = 1;
    _ReadWriteBarrier();

    /* Setup a redirection entry */
    ReDirReg.Vector = 0xFF;
    ReDirReg.DeliveryMode = APIC_MT_Fixed;
    ReDirReg.DestinationMode = APIC_DM_Physical;
    ReDirReg.DeliveryStatus = 0;
    ReDirReg.Polarity = 0;
    ReDirReg.RemoteIRR = 0;
    ReDirReg.TriggerMode = APIC_TGM_Edge;
    ReDirReg.Mask = 1;
    ReDirReg.Reserved = 0;
    ReDirReg.Destination = 0;

    /* Loop all table entries */
    for (Index = 0; Index < 24; Index++)
    {
        /* Initialize entry */
        IOApicWrite(IOAPIC_REDTBL + 2 * Index, ReDirReg.Long0);
        IOApicWrite(IOAPIC_REDTBL + 2 * Index + 1, ReDirReg.Long1);
    }

    /* Init the vactor to index table */
    for (Vector = 0; Vector <= 255; Vector++)
    {
        HalpVectorToIndex[Vector] = 0xFF;
    }

    // HACK: Allocate all IRQs, should rather do that on demand
    for (Index = 0; Index <= 15; Index++)
    {
        /* Map the IRQs to IRQLs like with the PIC */
        HalpAllocateSystemInterrupt(Index, 27 - Index);
    }

    /* Enable the timer interrupt */
    ReDirReg.Vector = APIC_CLOCK_VECTOR;
    ReDirReg.DeliveryMode = APIC_MT_Fixed;
    ReDirReg.DestinationMode = APIC_DM_Physical;
    ReDirReg.TriggerMode = APIC_TGM_Edge;
    ReDirReg.Mask = 0;
    ReDirReg.Destination = ApicRead(APIC_ID);
    IOApicWrite(IOAPIC_REDTBL + 2 * APIC_CLOCK_INDEX, ReDirReg.Long0);
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitializePICs(IN BOOLEAN EnableInterrupts)
{
    ULONG_PTR EFlags;

    /* Save EFlags and disable interrupts */
    EFlags = __readeflags();
    _disable();

    /* Initialize and mask the PIC */
    HalpInitializeLegacyPICs(TRUE);

    /* Initialize the I/O APIC */
    ApicInitializeIOApic();

    /* Manually reserve some vectors */
    HalpVectorToIndex[APIC_CLOCK_VECTOR] = 8;
    HalpVectorToIndex[APC_VECTOR] = 99;
    HalpVectorToIndex[DISPATCH_VECTOR] = 99;

    /* Set interrupt handlers in the IDT */
    KeRegisterInterruptHandler(APIC_CLOCK_VECTOR, HalpClockInterrupt);
#ifndef _M_AMD64
    KeRegisterInterruptHandler(APC_VECTOR, HalpApcInterrupt);
    KeRegisterInterruptHandler(DISPATCH_VECTOR, HalpDispatchInterrupt);
#endif

    /* Register the vectors for APC and dispatch interrupts */
    HalpRegisterVector(IDT_INTERNAL, 0, APC_VECTOR, APC_LEVEL);
    HalpRegisterVector(IDT_INTERNAL, 0, DISPATCH_VECTOR, DISPATCH_LEVEL);

    /* Restore interrupt state */
    if (EnableInterrupts) EFlags |= EFLAGS_INTERRUPT_MASK;
    __writeeflags(EFlags);
}
#else
CODE_SEG("INIT")
VOID
NTAPI
HalpInitializePICs(_In_ BOOLEAN EnableInterrupts)
{
    ULONG_PTR EFlags;

    DPRINT("HalpInitializePICs: EnableInterrupts %X\n", EnableInterrupts);

    /* Save EFlags and disable interrupts */
    EFlags = __readeflags();
    _disable();

    /* Initialize and mask the PIC */
    HalpInitializeLegacyPICs(TRUE);

    DPRINT("HalpInitializePICs: FIXME HalpGlobal8259Mask\n");

    /* Restore interrupt state */
    if (EnableInterrupts) EFlags |= EFLAGS_INTERRUPT_MASK;
    __writeeflags(EFlags);
}
#endif

/* SOFTWARE INTERRUPT TRAPS ***************************************************/

#ifndef _M_AMD64
#if 0
VOID
DECLSPEC_NORETURN
FASTCALL
HalpApcInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    KPROCESSOR_MODE ProcessorMode;
    KIRQL OldIrql;
    ASSERT(ApicGetProcessorIrql() == APC_LEVEL);

   /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);

#ifdef APIC_LAZY_IRQL
    if (!HalBeginSystemInterrupt(APC_LEVEL, APC_VECTOR, &OldIrql))
    {
        /* "Spurious" interrupt, exit the interrupt */
        KiEoiHelper(TrapFrame);
    }
#else
    /* Save the old IRQL */
    OldIrql = ApicGetCurrentIrql();
    ASSERT(OldIrql < APC_LEVEL);
#endif

    /* Raise to APC_LEVEL */
    ApicRaiseIrql(APC_LEVEL);

    /* End the interrupt */
    ApicSendEOI();

    /* Kernel or user APC? */
    if (KiUserTrap(TrapFrame)) ProcessorMode = UserMode;
    else if (TrapFrame->EFlags & EFLAGS_V86_MASK) ProcessorMode = UserMode;
    else ProcessorMode = KernelMode;

    /* Enable interrupts and call the kernel's APC interrupt handler */
    _enable();
    KiDeliverApc(ProcessorMode, NULL, TrapFrame);

    /* Disable interrupts */
    _disable();

    /* Restore the old IRQL */
    ApicLowerIrql(OldIrql);

    /* Exit the interrupt */
    KiEoiHelper(TrapFrame);
}

VOID
DECLSPEC_NORETURN
FASTCALL
HalpDispatchInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    KIRQL OldIrql;
    ASSERT(ApicGetProcessorIrql() == DISPATCH_LEVEL);

   /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);

#ifdef APIC_LAZY_IRQL
    if (!HalBeginSystemInterrupt(DISPATCH_LEVEL, DISPATCH_VECTOR, &OldIrql))
    {
        /* "Spurious" interrupt, exit the interrupt */
        KiEoiHelper(TrapFrame);
    }
#else
    /* Get the current IRQL */
    OldIrql = ApicGetCurrentIrql();
    ASSERT(OldIrql < DISPATCH_LEVEL);
#endif

    /* Raise to DISPATCH_LEVEL */
    ApicRaiseIrql(DISPATCH_LEVEL);

    /* End the interrupt */
    ApicSendEOI();

    /* Enable interrupts and call the kernel's DPC interrupt handler */
    _enable();
    KiDispatchInterrupt();
    _disable();

    /* Restore the old IRQL */
    ApicLowerIrql(OldIrql);

    /* Exit the interrupt */
    KiEoiHelper(TrapFrame);
}
#endif

DECLSPEC_NORETURN
VOID
FASTCALL
HalpLocalApicErrorServiceHandler(_In_ PKTRAP_FRAME TrapFrame)
{
    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);

    // FIXME HalpApicErrorLog and HalpLocalApicErrorCount

    ApicWrite(APIC_EOI, 0);

    if (KeGetCurrentPrcb()->CpuType >= 6)
    {
        ApicWrite(APIC_ESR, 0);
    }

  #ifdef __REACTOS__
    KiEoiHelper(TrapFrame);
  #else
    #error FIXME Kei386EoiHelper()
  #endif
}
#endif

/* SOFTWARE INTERRUPTS ********************************************************/

#ifdef _M_AMD64
VOID
FASTCALL
HalRequestSoftwareInterrupt(IN KIRQL Irql)
{
    /* Convert irql to vector and request an interrupt */
    ApicRequestInterrupt(IrqlToSoftVector(Irql), APIC_TGM_Edge);
}

VOID
FASTCALL
HalClearSoftwareInterrupt(
    IN KIRQL Irql)
{
    /* Nothing to do */
}
#else
VOID
FASTCALL
HalRequestSoftwareInterrupt(_In_ KIRQL Irql)
{
    PHALP_PCR_HAL_RESERVED HalReserved;
    KIRQL CurrentIrql;

    HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;

    if (Irql == DISPATCH_LEVEL)
    {
        HalReserved->DpcRequested = TRUE;
    }
    else if (Irql == APC_LEVEL)
    {
        HalReserved->ApcRequested = TRUE;
    }
    else
    {
        DbgBreakPoint();
    }

    CurrentIrql = KeGetPcr()->Irql;

    if (CurrentIrql < Irql)
        KfLowerIrql(CurrentIrql);
}

VOID
FASTCALL
HalClearSoftwareInterrupt(_In_ KIRQL Irql)
{
    PHALP_PCR_HAL_RESERVED HalReserved;

    HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;

    if (Irql == DISPATCH_LEVEL)
    {
        HalReserved->DpcRequested = FALSE;
    }
    else if (Irql == APC_LEVEL)
    {
        HalReserved->ApcRequested = FALSE;
    }
    else
    {
        DbgBreakPoint();
    }
}
#endif

/* SYSTEM INTERRUPTS **********************************************************/

#ifdef _M_AMD64
BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    PKPRCB Prcb = KeGetCurrentPrcb();
    UCHAR Index;
    ASSERT(Irql <= HIGH_LEVEL);
    ASSERT((IrqlToTpr(Irql) & 0xF0) == (Vector & 0xF0));

    /* Get the irq for this vector */
    Index = HalpVectorToIndex[Vector];

    /* Check if its valid */
    if (Index == 0xff)
    {
        /* Interrupt is not in use */
        return FALSE;
    }

    /* Read the redirection entry */
    ReDirReg = ApicReadIORedirectionEntry(Index);

    /* Check if the interrupt was unused */
    if (ReDirReg.Vector != Vector)
    {
        ReDirReg.Vector = Vector;
        ReDirReg.DeliveryMode = APIC_MT_LowestPriority;
        ReDirReg.DestinationMode = APIC_DM_Logical;
        ReDirReg.Destination = 0;
    }

    /* Check if the destination is logical */
    if (ReDirReg.DestinationMode == APIC_DM_Logical)
    {
        /* Set the bit for this cpu */
        ReDirReg.Destination |= ApicLogicalId(Prcb->Number);
    }

    /* Set the trigger mode */
    ReDirReg.TriggerMode = 1 - InterruptMode;

    /* Now unmask it */
    ReDirReg.Mask = FALSE;

    /* Write back the entry */
    ApicWriteIORedirectionEntry(Index, ReDirReg);

    return TRUE;
}

VOID
NTAPI
HalDisableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    UCHAR Index;
    ASSERT(Irql <= HIGH_LEVEL);
    ASSERT(Vector < RTL_NUMBER_OF(HalpVectorToIndex));

    Index = HalpVectorToIndex[Vector];

    /* Read lower dword of redirection entry */
    ReDirReg.Long0 = IOApicRead(IOAPIC_REDTBL + 2 * Index);

    /* Mask it */
    ReDirReg.Mask = 1;

    /* Write back lower dword */
    IOApicWrite(IOAPIC_REDTBL + 2 * Irql, ReDirReg.Long0);
}
#else
UCHAR
NTAPI
HalpAddInterruptDest(_In_ ULONG InDestination,
                     _In_ UCHAR ProcessorNumber)
{
    UCHAR Destination;

    DPRINT("HalpAddInterruptDest: InDestination %X, Processor %X\n", InDestination, ProcessorNumber);

    if (HalpForceApicPhysicalDestinationMode)
    {
        DPRINT1("HalpAddInterruptDest: FIXME! DbgBreakPoint()\n");
        DbgBreakPoint();Destination = 0;
        return Destination;
    }

    Destination = HalpIntDestMap[ProcessorNumber];
    if (!Destination)
    {
        DPRINT("HalpAddInterruptDest: return %X\n", InDestination);
        return InDestination;
    }

    if (!HalpMaxProcsPerCluster)
    {
        Destination |= InDestination;
        DPRINT("HalpAddInterruptDest: return Destination %X\n", Destination);
        return Destination;
    }

    DPRINT1("HalpAddInterruptDest: FIXME! DbgBreakPoint()\n");
    DbgBreakPoint();

    DPRINT("HalpAddInterruptDest: return Destination %X\n", Destination);
    return Destination;
}

VOID
NTAPI
HalpSetRedirEntry(_In_ USHORT IntI,
                  _In_ PIOAPIC_REDIRECTION_REGISTER IoApicReg,
                  _In_ ULONG Destination)
{
    PIO_APIC_REGISTERS IoApicRegs;
    UCHAR IoUnit;

    for (IoUnit = 0; IoUnit < MAX_IOAPICS; IoUnit++)
    {
        if (IntI + 1 <= HalpMaxApicInti[IoUnit])
        {
            break;
        }

        ASSERT(IntI >= HalpMaxApicInti[IoUnit]);
        IntI -= HalpMaxApicInti[IoUnit];
    }

    ASSERT(IoUnit < MAX_IOAPICS);

    IoApicRegs = (PIO_APIC_REGISTERS)HalpMpInfoTable.IoApicVA[IoUnit];

    IoApicWrite(IoApicRegs, ((IOAPIC_REDTBL + 1) + IntI * 2), Destination); // RedirReg + 1
    IoApicWrite(IoApicRegs, (IOAPIC_REDTBL + IntI * 2), IoApicReg->Long0);  // RedirReg
}

VOID
NTAPI
HalpEnableRedirEntry(_In_ USHORT IntI,
                     _In_ PIOAPIC_REDIRECTION_REGISTER IoApicReg,
                     _In_ UCHAR ProcessorNumber)
{
    UCHAR Destination;

    HalpIntiInfo[IntI].Entry = IoApicReg->Long0;

    Destination = HalpAddInterruptDest(HalpIntiInfo[IntI].Destinations, ProcessorNumber);
    HalpIntiInfo[IntI].Destinations = Destination;

    HalpSetRedirEntry(IntI, IoApicReg, ((ULONG)Destination << 24));

    HalpIntiInfo[IntI].Enabled = 1;
}

BOOLEAN
NTAPI
HalEnableSystemInterrupt(_In_ ULONG SystemVector,
                         _In_ KIRQL Irql,
                         _In_ KINTERRUPT_MODE InterruptMode)
{
    IOAPIC_REDIRECTION_REGISTER IoApicReg;
    APIC_INTI_INFO IntiInfo;
    ULONG TriggerMode;
    ULONG Lock;
    USHORT IntI;
    UCHAR DevLevel;
    UCHAR CpuNumber;

    DPRINT1("HalEnableSystemInterrupt: Vector %X, Irql %X, Mode %X\n", SystemVector, Irql, InterruptMode);

    ASSERT(SystemVector < ((1 + SUPPORTED_NODES) * MAX_INT_VECTORS - 1));
    ASSERT(Irql <= HIGH_LEVEL);

    IntI = HalpVectorToINTI[SystemVector];
    if (IntI == 0xFFFF)
    {
        DPRINT1("HalEnableSystemInterrupt: return FALSE\n");
        return FALSE;
    }

    if (IntI >= MAX_INTI)
    {
        DPRINT1("EnableSystemInt: IntI %X, MAX_INTI %X\n", IntI, MAX_INTI);
        ASSERT(IntI < MAX_INTI);
    }

    IntiInfo = HalpIntiInfo[IntI];
    TriggerMode = IntiInfo.TriggerMode;

    if (InterruptMode == LevelSensitive)
    {
        DevLevel = HalpDevLevel[HALP_DEV_INT_LEVEL][TriggerMode];
    }
    else
    {
        DevLevel = HalpDevLevel[HALP_DEV_INT_EDGE][TriggerMode];
    }

    Lock = HalpAcquireHighLevelLock(&HalpAccountingLock);
    CpuNumber = KeGetPcr()->Prcb->Number;

    if (IntiInfo.Type != INTI_INFO_TYPE_INT &&
        IntiInfo.Type != INTI_INFO_TYPE_ExtINT)
    {
        DPRINT1("HalEnableSystemInterrupt: Unsupported IntiInfo.Type %X. DbgBreakPoint()\n", IntiInfo.Type);
        DbgBreakPoint();

        HalpReleaseHighLevelLock(&HalpAccountingLock, Lock);
        return TRUE;
    }

    if (IntiInfo.Type == INTI_INFO_TYPE_ExtINT)
    {
        DPRINT1("HalEnableSystemInterrupt: FIXME ExtINT. DbgBreakPoint()\n");
        DbgBreakPoint();

        HalpReleaseHighLevelLock(&HalpAccountingLock, Lock);
        return TRUE;
    }

    /* IntiInfo.Type == INTI_INFO_TYPE_INT (INTR Interrupt Source) */

    IoApicReg.LongLong = 0;

    if (SystemVector == APIC_CLOCK_VECTOR)
    {
        ASSERT(CpuNumber == 0);
        IoApicReg.Vector = APIC_CLOCK_VECTOR;
        IoApicReg.Long0 |= HalpDefaultApicDestinationModeMask;
    }
    else
    {
        if (SystemVector == 0xFF)
        {
            return FALSE;
        }

        IoApicReg.Vector = HalVectorToIDTEntry(SystemVector);

        if (!HalpForceApicPhysicalDestinationMode)
        {
            IoApicReg.DeliveryMode = 1;    // Lowest Priority
            IoApicReg.DestinationMode = 1; // Logical Mode
        }
    }

    if (DevLevel & DL_LEVEL_SENSITIVE)
    {
        IoApicReg.TriggerMode = 1; // Level sensitive
    }

    if (HalpDevPolarity[IntiInfo.Polarity][(DevLevel & DL_LEVEL_SENSITIVE)] == DP_LOW_ACTIVE)
    {
        IoApicReg.Polarity = 1; // Low active
    }

    HalpEnableRedirEntry(IntI, &IoApicReg, CpuNumber);

    DPRINT("HalEnableSystemInterrupt: HalpIntiInfo[IntI] %X\n", HalpIntiInfo[IntI].AsULONG);
    HalpReleaseHighLevelLock(&HalpAccountingLock, Lock);

    return TRUE;
}

VOID
NTAPI
HalDisableSystemInterrupt(_In_ ULONG Vector,
                          _In_ KIRQL Irql)
{
    DPRINT1("HalDisableSystemInterrupt: Vector %X, Irql %X\n", Vector, Irql);
    DbgBreakPoint();
}
#endif

#ifndef _M_AMD64
FORCEINLINE
VOID
KeSetCurrentIrql(_In_ KIRQL NewIrql)
{
    /* Set new current IRQL */
    KeGetPcr()->Irql = NewIrql;
}

VOID
FASTCALL
HalpGenerateInterrupt(_In_ UCHAR Vector)
{
    //DPRINT1("HalpGenerateInterrupt: Vector %X\n", Vector);
    ((PINTERRUPT_ENTRY)&HwInterruptTable[Vector])();
}

VOID
FASTCALL
HalpLowerIrqlHardwareInterrupts(_In_ KIRQL NewIrql)
{
    PUCHAR pPrcbVector;
    ULONG EFlags;
    UCHAR Vector;
    UCHAR Irql;
    UCHAR Idx;

    if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
        KeSetCurrentIrql(NewIrql);
        return;
    }

    pPrcbVector = (PUCHAR)KeGetCurrentPrcb()->HalReserved;
    if (pPrcbVector[0] == 0)
    {
        KeSetCurrentIrql(NewIrql);

        if (pPrcbVector[0] == 0)
            return;

        KeSetCurrentIrql(HIGH_LEVEL);
    }

    EFlags = __readeflags();
    _disable();

    while (pPrcbVector[0])
    {
        Idx = pPrcbVector[0];
        Vector = pPrcbVector[Idx];
        Irql = HalpVectorToIRQL[(UCHAR)Vector >> 4];

        if (Irql <= NewIrql)
            break;

        pPrcbVector[0] = Idx - 1;
        KeSetCurrentIrql(Irql - 1);

        HalpGenerateInterrupt(Vector);
        //HalpTotalReplayed++;
    }

    KeSetCurrentIrql(NewIrql);

    if (EFlags & EFLAGS_INTERRUPT_MASK)
        _enable();
}

VOID
NTAPI
HalpDispatchSoftwareInterrupt(_In_ KIRQL Irql,
                              _In_ PKTRAP_FRAME TrapFrame)
{
    PHALP_PCR_HAL_RESERVED HalReserved;
    ULONG EFlags;

    EFlags = __readeflags();
    KeGetPcr()->Irql = Irql;

    if (!(EFlags & EFLAGS_INTERRUPT_MASK))
        _enable();

    HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;
    if (Irql == APC_LEVEL)
    {
        HalReserved->ApcRequested = FALSE;
        KiDeliverApc(0, 0, (PKTRAP_FRAME)TrapFrame);
    }
    else if (Irql == DISPATCH_LEVEL)
    {
        HalReserved->DpcRequested = FALSE;
        KiDispatchInterrupt();
    }
    else
    {
        DbgBreakPoint();
    }

    if (!(EFlags & EFLAGS_INTERRUPT_MASK))
        _disable();
}

VOID
FASTCALL
HalpCheckForSoftwareInterrupt(_In_ KIRQL NewIrql,
                              _In_ PKTRAP_FRAME TrapFrame)
{
    PHALP_PCR_HAL_RESERVED HalReserved;
    BOOLEAN ApcRequested;
    BOOLEAN DpcRequested;

    HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;

    ApcRequested = HalReserved->ApcRequested;
    DpcRequested = HalReserved->DpcRequested;

    if (NewIrql)
    {
        if (NewIrql == APC_LEVEL && HalReserved->DpcRequested)
        {
            do
            {
                HalpDispatchSoftwareInterrupt(DISPATCH_LEVEL, TrapFrame);
                KeSetCurrentIrql(APC_LEVEL);
                HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;
            }
            while (HalReserved->DpcRequested);
        }

        return;
    }

    while (ApcRequested | DpcRequested)
    {
        if (DpcRequested)
        {
            HalpDispatchSoftwareInterrupt(DISPATCH_LEVEL, TrapFrame);
        }
        else
        {
            HalpDispatchSoftwareInterrupt(APC_LEVEL, TrapFrame);
        }

        KeSetCurrentIrql(PASSIVE_LEVEL);

        HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;
        ApcRequested = HalReserved->ApcRequested;
        DpcRequested = HalReserved->DpcRequested;
    }
}

BOOLEAN
NTAPI
HalBeginSystemInterrupt(_In_ KIRQL NewIrql,
                        _In_ ULONG SystemVector,
                        _Out_ PKIRQL OutOldIrql)
{
    PUCHAR pPrcbVector;
    UCHAR Idx;
    KIRQL OldIrql;

    OldIrql = KeGetCurrentIrql();

    if (OldIrql < HalpVectorToIRQL[(UCHAR)SystemVector >> 4])
    {
        *OutOldIrql = OldIrql;
        KeSetCurrentIrql(NewIrql);

        _enable();
        return TRUE;
    }

    pPrcbVector = (PUCHAR)KeGetCurrentPrcb()->HalReserved;

    Idx = pPrcbVector[0];
    pPrcbVector[Idx + 1] = SystemVector;
    pPrcbVector[0] = Idx + 1;

    _enable();
    return FALSE;
}

static
VOID
NTAPI
HalpEndSystemInterrupt(_In_ PHAL_INTERRUPT_CONTEXT IntContext,
                       _In_ KIRQL OldIrql)
{
    KIRQL Irql = HalpVectorToIRQL[IntContext->Vector >> 4];

    if (Irql < KeGetCurrentIrql())
    {
        HalpLowerIrqlHardwareInterrupts(Irql);
    }

    if (ApicRead(APIC_PPR) != (IntContext->Vector & 0xF0))
    {
        DPRINT1("HalpEndSystemInterrupt: SystemVector %X, APIC_PPR %X\n", IntContext->Vector, ApicRead(APIC_PPR));
        DbgBreakPoint();
    }

    ApicWrite(APIC_EOI, 0);

    KeSetCurrentIrql(IntContext->Irql);

    if (IntContext->Irql < DISPATCH_LEVEL)
    {
        PHALP_PCR_HAL_RESERVED HalReserved;
        HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;

        if (IntContext->Irql == PASSIVE_LEVEL &&
            HalReserved->ApcRequested &&
            ((UCHAR)(KeGetCurrentPrcb()->HalReserved[0]) == 0))
        {
            HalpCheckForSoftwareInterrupt(IntContext->Irql, IntContext->TrapFrame);
        }
    }

    //FIXME KiCheckForSListAddress(TrapFrame);
}

#ifdef __REACTOS__ // RosHalEndSystemInterrupt?
VOID
NTAPI
HalEndSystemInterrupt(_In_ KIRQL OldIrql,
                      _In_ PHAL_INTERRUPT_CONTEXT IntContext)
{
    //DPRINT1("HalEndSystemInterrupt: OldIrql %X, IntContext %X\n", OldIrql,  IntContext);
    HalpEndSystemInterrupt(IntContext, OldIrql);
}
#else
/* NT use non-standard parameters calling */
__declspec(naked)
VOID
NTAPI
HalEndSystemInterrupt(_In_ KIRQL OldIrql,
                      _In_ UCHAR Vector)
//                    _In_ PKTRAP_FRAME TrapFrame)
{
    HAL_INTERRUPT_CONTEXT IntContext

    DPRINT1("HalEndSystemInterrupt: FIXME !!! OldIrql %X,  Vector %X\n", OldIrql, Vector);
    DbgBreakPoint();

    /* NT really use stack for pointer TrapFrame (us third parameter),
       but ... HalEndSystemInterrupt() defined with two parameters.
    */

    IntContext.Irql = OldIrql;
    IntContext.Vector = Vector;
    IntContext.TrapFrame = 0;//TrapFrame; ? FIXME!

    HalpEndSystemInterrupt(&IntContext, OldIrql);
}
#endif

/* IRQL MANAGEMENT ************************************************************/

#if 0
KIRQL
NTAPI
KeGetCurrentIrql(VOID)
{
    /* Read the current TPR and convert it to an IRQL */
    return ApicGetCurrentIrql();
}

VOID
FASTCALL
KfLowerIrql(
    IN KIRQL OldIrql)
{
#if DBG
    /* Validate correct lower */
    if (OldIrql > ApicGetCurrentIrql())
    {
        /* Crash system */
        KeBugCheck(IRQL_NOT_LESS_OR_EQUAL);
    }
#endif
    /* Set the new IRQL */
    ApicLowerIrql(OldIrql);
}

KIRQL
FASTCALL
KfRaiseIrql(
    IN KIRQL NewIrql)
{
    KIRQL OldIrql;

    /* Read the current IRQL */
    OldIrql = ApicGetCurrentIrql();
#if DBG
    /* Validate correct raise */
    if (OldIrql > NewIrql)
    {
        /* Crash system */
        KeBugCheck(IRQL_NOT_GREATER_OR_EQUAL);
    }
#endif
    /* Convert the new IRQL to a TPR value and write the register */
    ApicRaiseIrql(NewIrql);

    /* Return old IRQL */
    return OldIrql;
}
#endif

KIRQL
NTAPI
KeGetCurrentIrql(VOID)
{
    /* Return the IRQL */
    return KeGetPcr()->Irql;
}

VOID
FASTCALL
KfLowerIrql(_In_ KIRQL NewIrql)
{
    HalpLowerIrqlHardwareInterrupts(NewIrql);
    HalpCheckForSoftwareInterrupt(NewIrql, 0);
}

KIRQL
FASTCALL
KfRaiseIrql(_In_ KIRQL NewIrql)
{
    PKPCR Pcr = KeGetPcr();
    KIRQL OldIrql;

    OldIrql = Pcr->Irql;
    Pcr->Irql = NewIrql;

    return OldIrql;
}

KIRQL
NTAPI
KeRaiseIrqlToDpcLevel(VOID)
{
    return KfRaiseIrql(DISPATCH_LEVEL);
}

KIRQL
NTAPI
KeRaiseIrqlToSynchLevel(VOID)
{
    return KfRaiseIrql(SYNCH_LEVEL);
}

VOID NTAPI Kii386SpinOnSpinLock(_In_ PKSPIN_LOCK SpinLock, _In_ ULONG Flags);

ULONG
FASTCALL
HalpAcquireHighLevelLock(_In_ volatile PKSPIN_LOCK SpinLock)
{
    ULONG EFlags;

    EFlags = __readeflags();

    while (TRUE)
    {
        _disable();

        if (InterlockedBitTestAndSet((volatile PLONG)SpinLock, 0) == 0)
        {
            break;
        }

      #if defined(_M_IX86) && DBG
        /* On x86 debug builds, we use a much slower but useful routine */
        Kii386SpinOnSpinLock(SpinLock, 5);
      #else
        /* It's locked... spin until it's unlocked */
        while (*(volatile PKSPIN_LOCK)SpinLock & 1)
        {
            /* Yield and keep looping */
            YieldProcessor();
        }
      #endif
    }

  #if DBG
    /* On debug builds, we OR in the KTHREAD */
    *SpinLock = ((KSPIN_LOCK)KeGetCurrentThread() | 1);
  #endif

    return EFlags;
}

VOID
FASTCALL
HalpReleaseHighLevelLock(_In_ volatile PKSPIN_LOCK SpinLock,
                         _In_ ULONG EFlags)
{
  #if DBG
    if (*SpinLock != ((KSPIN_LOCK)KeGetCurrentThread() | 1))
    {
        KeBugCheckEx(SPIN_LOCK_NOT_OWNED, (ULONG_PTR)SpinLock, 0, 0, 0);
    }
  #endif

    InterlockedAnd((volatile PLONG)SpinLock, 0);

    __writeeflags(EFlags);
}
ULONG
NTAPI 
HalpAllocateSystemInterruptVector(_In_ USHORT IntI)
{
    KAFFINITY Affinity;
    ULONG InterruptCount;
    ULONG MaxPriorityLevel;
    ULONG PriorityLevel;
    ULONG SystemVector;
    ULONG Vector;
    ULONG IrqlIdx;
    ULONG Node;   // (from 1 .. to 32)
    ULONG ix;

    DPRINT("HalpAllocateSystemInterruptVector: IntI %X\n", IntI);

    if (!HalpMaxNode)
    {
        DPRINT1("HalpGetSystemInterruptVector: HalpMaxNode == 0\n");
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x100, HalpDefaultInterruptAffinity, 0, 0);
    }

    InterruptCount = 0xFFFFFFFF;
    Node = 0;

    for (ix = HalpMaxNode; ix; ix--)
    {
       Affinity = HalpNodeProcessorAffinity[ix - 1];

       if ((HalpDefaultInterruptAffinity & Affinity) &&
           HalpNodeInterruptCount[ix - 1] < InterruptCount)
        {
            Node = ix;
            InterruptCount = HalpNodeInterruptCount[ix - 1];
        }
    }

    if (!Node)
    {
        DPRINT1("HalpGetSystemInterruptVector: Node == 0\n");
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x100, HalpDefaultInterruptAffinity, 0, 0);
    }

    /* 0x5n, 0x6n, 0x7n, 0x8n, 0x9n, 0xAn, 0xBn - bank of free vectors for HW devices.
       Vectors with n=0 not used.
    */
    MaxPriorityLevel = HalpNodePriorityLevelUsage[Node][HALP_DEVICE_INT_PRIORITY_LEVEL_COUNT-1];
    IrqlIdx = (HALP_DEVICE_INT_PRIORITY_LEVEL_COUNT - 1); // maximal index [(0xB - 0x5) - 1]

    for (ix = IrqlIdx; ix; ix--)
    {
        PriorityLevel = HalpNodePriorityLevelUsage[Node][ix - 1];
        if (PriorityLevel < MaxPriorityLevel)
        {
            IrqlIdx = ix - 1;
            MaxPriorityLevel = HalpNodePriorityLevelUsage[Node][ix - 1];
        }
    }

    if (PriorityLevel >= HALP_MAX_PRIORITY_LEVEL)
    {
        DPRINT1("HalpAllocateSystemInterruptVector: PriorityLevel %X\n", PriorityLevel);
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x101, HalpDefaultInterruptAffinity, 0, 0);
    }

    HalpNodeInterruptCount[Node - 1]++;
    HalpNodePriorityLevelUsage[Node][IrqlIdx] = (UCHAR)(PriorityLevel + 1); // (0x1-0xF)

    Vector = (UCHAR)(((HALP_DEVICE_INT_PRIORITY_LEVEL_BASE + IrqlIdx) << 4) + (UCHAR)(PriorityLevel + 1)); // (0x51-0x5F ... 0xB1-0xBF)
    SystemVector = (Node << 8) | Vector; // (0x0151-0x015F ... 0x20B1-0x20BF)
    ASSERT(SystemVector < (MAX_CPUS * MAX_INT_VECTORS));

    HalpVectorToIRQL[Vector >> 4] = (4 + IrqlIdx); // 0-6 (Irql: 4-10)
    HalpVectorToINTI[SystemVector] = IntI;
    HalpINTItoVector[IntI] = SystemVector;

    DPRINT("HalpAllocateSystemInterruptVector: SystemVector %X\n", SystemVector);

    return SystemVector;
}

ULONG
NTAPI
HalpGetSystemInterruptVector(_In_ PBUS_HANDLER BusHandler,
                             _In_ PBUS_HANDLER RootHandler,
                             _In_ ULONG BusInterruptLevel,
                             _In_ ULONG BusInterruptVector,
                             _Out_ PKIRQL OutIrql,
                             _Out_ PKAFFINITY OutAffinity)
{
    PVOID Handle;
    KAFFINITY Affinity;
    ULONG OLdLevel;
    ULONG Vector;
    ULONG SystemVector;
    ULONG AlocVector;
    USHORT IntI;

    DPRINT("HalpGetSystemInterruptVector: Level %X, Vector %X\n", BusInterruptLevel, BusInterruptVector);

    if (RootHandler != BusHandler)
    {
        DPRINT1("HalpGetSystemInterruptVector: DbgBreakPoint()\n");
        DbgBreakPoint(); // ASSERT(FALSE);
    }

    if (!HalpGetApicInterruptDesc(BusInterruptLevel, &IntI))
    {
        DPRINT1("HalpGetSystemInterruptVector: return 0\n");
        return 0;
    }

    DPRINT("HalpGetSystemInterruptVector: IntI %X\n", IntI);

    if (!HalpINTItoVector[IntI])
    {
        Handle = MmLockPagableDataSection(HalpGetSystemInterruptVector);
        OLdLevel = HalpAcquireHighLevelLock(&HalpAccountingLock);

        AlocVector = HalpAllocateSystemInterruptVector(IntI);
        if (!AlocVector)
        {
            HalpReleaseHighLevelLock(&HalpAccountingLock, OLdLevel);
            MmUnlockPagableImageSection(Handle);
            DPRINT1("HalpGetSystemInterruptVector: return 0\n");
            return 0;
        }

        if (!RootHandler->BusNumber &&
            BusInterruptLevel < HAL_PIC_VECTORS &&
            RootHandler->InterfaceType == Eisa)
        {
            DPRINT1("HalpGetSystemInterruptVector: DbgBreakPoint()\n");
            DbgBreakPoint(); // ASSERT(FALSE);
            //HalpPICINTToVector[BusInterruptLevel] = HalVectorToIDTEntry(AlocVector);
        }

        HalpReleaseHighLevelLock(&HalpAccountingLock, OLdLevel);
        MmUnlockPagableImageSection(Handle);
    }

    SystemVector = HalpINTItoVector[IntI];
    ASSERT(SystemVector < (MAX_CPUS * MAX_INT_VECTORS));
    ASSERT(HalpVectorToINTI[SystemVector] == IntI);

    Vector = HalVectorToIDTEntry(SystemVector);
    Affinity = HalpNodeProcessorAffinity[(SystemVector >> 8) - 1];

    *OutIrql = HalpVectorToIRQL[(ULONG)Vector >> 4];
    *OutAffinity = (Affinity & HalpDefaultInterruptAffinity);

    DPRINT("HalpGetSystemInterruptVector: *OutIrql %X, *OutAffinity %X\n", *OutIrql, *OutAffinity);

    if (*OutAffinity == 0)
    {
        DPRINT1("HalpGetSystemInterruptVector: *OutAffinity == 0\n");
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x102, HalpDefaultInterruptAffinity, SystemVector >> 8, SystemVector);
    }

    DPRINT("HalpGetSystemInterruptVector: SystemVector %X\n", SystemVector);
    return SystemVector;
}
#endif /* !_M_AMD64 */

/* EOF */
