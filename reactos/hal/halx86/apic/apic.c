/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/apic.c
 * PURPOSE:         HAL APIC Management and Control Code
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
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

VOID
NTAPI
HalpInitializeLegacyPIC(VOID)
{
    I8259_ICW1 Icw1;
    I8259_ICW2 Icw2;
    I8259_ICW3 Icw3;
    I8259_ICW4 Icw4;

    /* Initialize ICW1 for master, interval 8, edge-triggered mode with ICW4 */
    Icw1.NeedIcw4 = TRUE;
    Icw1.OperatingMode = Cascade;
    Icw1.Interval = Interval8;
    Icw1.InterruptMode = EdgeTriggered;
    Icw1.Init = TRUE;
    Icw1.InterruptVectorAddress = 0;
    __outbyte(PIC1_CONTROL_PORT, Icw1.Bits);

    /* ICW2 - interrupt vector offset */
    Icw2.Bits = PRIMARY_VECTOR_BASE;
    __outbyte(PIC1_DATA_PORT, Icw2.Bits);

    /* Connect slave to IRQ 2 */
    Icw3.Bits = 0;
    Icw3.SlaveIrq2 = TRUE;
    __outbyte(PIC1_DATA_PORT, Icw3.Bits);

    /* Enable 8086 mode, non-automatic EOI, non-buffered mode, non special fully nested mode */
    Icw4.SystemMode = New8086Mode;
    Icw4.EoiMode = NormalEoi;
    Icw4.BufferedMode = NonBuffered;
    Icw4.SpecialFullyNestedMode = FALSE;
    Icw4.Reserved = 0;
    __outbyte(PIC1_DATA_PORT, Icw4.Bits);

    /* Mask all interrupts */
    __outbyte(PIC1_DATA_PORT, 0xFF);

    /* Initialize ICW1 for slave, interval 8, edge-triggered mode with ICW4 */
    Icw1.NeedIcw4 = TRUE;
    Icw1.InterruptMode = EdgeTriggered;
    Icw1.OperatingMode = Cascade;
    Icw1.Interval = Interval8;
    Icw1.Init = TRUE;
    Icw1.InterruptVectorAddress = 0; /* This is only used in MCS80/85 mode */
    __outbyte(PIC2_CONTROL_PORT, Icw1.Bits);

    /* Set interrupt vector base */
    Icw2.Bits = PRIMARY_VECTOR_BASE + 8;
    __outbyte(PIC2_DATA_PORT, Icw2.Bits);

    /* Slave ID */
    Icw3.Bits = 0;
    Icw3.SlaveId = 2;
    __outbyte(PIC2_DATA_PORT, Icw3.Bits);

    /* Enable 8086 mode, non-automatic EOI, non-buffered mode, non special fully nested mode */
    Icw4.SystemMode = New8086Mode;
    Icw4.EoiMode = NormalEoi;
    Icw4.BufferedMode = NonBuffered;
    Icw4.SpecialFullyNestedMode = FALSE;
    Icw4.Reserved = 0;
    __outbyte(PIC2_DATA_PORT, Icw4.Bits);

    /* Mask all interrupts */
    __outbyte(PIC2_DATA_PORT, 0xFF);
}

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

    ApicSendEOI();
}

VOID
NTAPI
HalpInitializePICs(IN BOOLEAN EnableInterrupts)
{
    ULONG_PTR EFlags;

    /* Save EFlags and disable interrupts */
    EFlags = __readeflags();
    _disable();

    /* Initialize and mask the PIC */
    HalpInitializeLegacyPIC();

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


/* SOFTWARE INTERRUPT TRAPS ***************************************************/

#ifndef _M_AMD64
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


/* SOFTWARE INTERRUPTS ********************************************************/


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


/* SYSTEM INTERRUPTS **********************************************************/

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

#ifndef _M_AMD64
BOOLEAN
NTAPI
HalBeginSystemInterrupt(
    IN KIRQL Irql,
    IN ULONG Vector,
    OUT PKIRQL OldIrql)
{
    KIRQL CurrentIrql;

    /* Get the current IRQL */
    CurrentIrql = ApicGetCurrentIrql();

#ifdef APIC_LAZY_IRQL
    /* Check if this interrupt is allowed */
    if (CurrentIrql >= Irql)
    {
        IOAPIC_REDIRECTION_REGISTER RedirReg;
        UCHAR Index;

        /* It is not, set the real Irql in the TPR! */
        ApicWrite(APIC_TPR, IrqlToTpr(CurrentIrql));

        /* Save the new hard IRQL in the IRR field */
        KeGetPcr()->IRR = CurrentIrql;

        /* End this interrupt */
        ApicSendEOI();

        /* Get the irq for this vector */
        Index = HalpVectorToIndex[Vector];

        /* Check if its valid */
        if (Index != 0xff)
        {
            /* Read the I/O redirection entry */
            RedirReg = ApicReadIORedirectionEntry(Index);

            /* Re-request the interrupt to be handled later */
            ApicRequestInterrupt(Vector, (UCHAR)RedirReg.TriggerMode);
       }
       else
       {
            /* Re-request the interrupt to be handled later */
            ApicRequestInterrupt(Vector, APIC_TGM_Edge);
       }

        /* Pretend it was a spurious interrupt */
        return FALSE;
    }
#endif
    /* Save the current IRQL */
    *OldIrql = CurrentIrql;

    /* Set the new IRQL */
    ApicRaiseIrql(Irql);

    /* Turn on interrupts */
    _enable();

    /* Success */
    return TRUE;
}

VOID
NTAPI
HalEndSystemInterrupt(
    IN KIRQL OldIrql,
    IN PKTRAP_FRAME TrapFrame)
{
    /* Send an EOI */
    ApicSendEOI();

    /* Restore the old IRQL */
    ApicLowerIrql(OldIrql);
}


/* IRQL MANAGEMENT ************************************************************/

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

#endif /* !_M_AMD64 */

