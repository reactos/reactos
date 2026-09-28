/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/apic/apic.c
 * PURPOSE:         HAL APIC Management and Control Code
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 * REFERENCES:      https://web.archive.org/web/20190407074221/http://www.joseflores.com/docs/ExploringIrql.html
 *                  https://www.codeproject.com/KB/system/soviet_kernel_hack.aspx
 *                  http://bbs.unixmap.net/thread-2022-1-1.html (DEAD_LINK)
 *                  https://codemachine.com/articles/interrupt_dispatching.html
 *                  https://www.osronline.com/article.cfm%5Earticle=211.htm
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "apicp.h"
#include <smp.h>
#define NDEBUG
#include <debug.h>

#ifndef _M_AMD64
#define APIC_LAZY_IRQL
#endif

/* GLOBALS ********************************************************************/

ULONG ApicVersion;

/* Always updated together. */
UCHAR HalpVectorToIndex[256];
UCHAR HalpGsivToVector[256];

extern HALP_APIC_INFO_TABLE HalpApicInfoTable;

typedef struct _HALP_IOAPIC_UNIT
{
    ULONG_PTR Base;
    ULONG InputBase;
    ULONG InputCount;
} HALP_IOAPIC_UNIT, *PHALP_IOAPIC_UNIT;

/* I/O APICs in use, the first one is mapped at IOAPIC_BASE */
HALP_IOAPIC_UNIT HalpIoApics[HALP_MAX_IOAPICS];
ULONG HalpIoApicCount;

/* One past the highest input served by an I/O APIC */
ULONG HalpMaxGsi;

/*
 * Last value written to each redirection entry
 * This is done for a few reasons but the biggest benefit
 * is preperation for ACPI Sleep. Which we need to cache the last
 * known state to restore. IOAPIC data lost during ACPI Sleep!
 */
IOAPIC_REDIRECTION_REGISTER HalpIoApicShadow[HALP_MAX_INPUTS];

/* Guards the vector tables, the shadow table and the IOREGSEL/IOWIN window */
static KSPIN_LOCK HalpVectorLock;

/* Vectors handed to the ACPI driver, kept out of the HAL's own allocations */
static BOOLEAN HalpAcpiGrantedVector[256];

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
IOApicRead(
    _In_ ULONG_PTR Base,
    _In_ UCHAR Register)
{
    /* Select the register, then do the read */
    WRITE_REGISTER_ULONG((PULONG)(Base + IOAPIC_IOREGSEL), Register);
    return READ_REGISTER_ULONG((PULONG)(Base + IOAPIC_IOWIN));
}

FORCEINLINE
VOID
IOApicWrite(
    _In_ ULONG_PTR Base,
    _In_ UCHAR Register,
    _In_ ULONG Value)
{
    /* Select the register, then do the write */
    WRITE_REGISTER_ULONG((PULONG)(Base + IOAPIC_IOREGSEL), Register);
    WRITE_REGISTER_ULONG((PULONG)(Base + IOAPIC_IOWIN), Value);
}

/**
 * @brief
 * Finds the I/O APIC that serves an input.
 *
 * @param[in] Input
 * The global system interrupt to look up.
 *
 * @param[out] Unit
 * Receives the I/O APIC serving the input.
 *
 * @return
 * TRUE if an I/O APIC serves the input, FALSE otherwise.
 */
FORCEINLINE
BOOLEAN
HalpFindIoApicInput(
    _In_ ULONG Input,
    _Out_ PHALP_IOAPIC_UNIT *Unit)
{
    ULONG Index;

    for (Index = 0; Index < HalpIoApicCount; Index++)
    {
        if ((Input >= HalpIoApics[Index].InputBase) &&
            (Input - HalpIoApics[Index].InputBase < HalpIoApics[Index].InputCount))
        {
            *Unit = &HalpIoApics[Index];
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief
 * Programs a redirection entry and records it in the shadow table.
 *
 * @remarks
 * Callers serialize through the dispatcher lock or run before other
 * processors are started, since IOREGSEL and IOWIN are shared.
 */
FORCEINLINE
VOID
ApicWriteIORedirectionEntry(
    _In_ ULONG Input,
    _In_ IOAPIC_REDIRECTION_REGISTER ReDirReg)
{
    PHALP_IOAPIC_UNIT Unit;
    UCHAR Register;

    if (!HalpFindIoApicInput(Input, &Unit))
    {
        ASSERT(FALSE);
        return;
    }

    HalpIoApicShadow[Input] = ReDirReg;

    /* The destination has to be in place before the low half unmasks the
       entry, so that an asserted input cannot reach a stale processor */
    Register = (UCHAR)(IOAPIC_REDTBL + 2 * (Input - Unit->InputBase));
    IOApicWrite(Unit->Base, Register + 1, ReDirReg.Long1);
    IOApicWrite(Unit->Base, Register, ReDirReg.Long0);
}

/**
 * @brief
 * Returns a redirection entry from the shadow table, without accessing
 * the I/O APIC.
 */
FORCEINLINE
IOAPIC_REDIRECTION_REGISTER
ApicReadIORedirectionEntry(
    _In_ ULONG Input)
{
    ASSERT(Input < HalpMaxGsi);
    return HalpIoApicShadow[Input];
}

/**
 * @brief
 * Makes a vector pending in the local APIC of the current processor.
 *
 * @param[in] Vector
 * The vector to request.
 *
 * @remarks
 * A fixed command has to assert. Some implementations drop a de-asserting one,
 * which would leave the IRR wait below spinning.
 */
FORCEINLINE
VOID
ApicRequestSelfInterrupt(
    _In_ UCHAR Vector)
{
    ULONG Flags;
    APIC_INTERRUPT_COMMAND_REGISTER Icr;
    APIC_INTERRUPT_COMMAND_REGISTER IcrStatus;

    /*
     * The IRR registers are spaced 16 bytes apart and hold 32 status bits each.
     * Pre-compute the register and bit that match our vector.
     */
    ULONG VectorHigh = Vector / 32;
    ULONG VectorLow = Vector % 32;
    ULONG Irr = APIC_IRR + 0x10 * VectorHigh;
    ULONG IrrBit = 1UL << VectorLow;

    /* Setup the command register */
    Icr.LongLong = 0;
    Icr.Vector = Vector;
    Icr.MessageType = APIC_MT_Fixed;
    Icr.Level = APIC_LEVEL_Assert;
    Icr.TriggerMode = APIC_TGM_Edge;
    Icr.DestinationShortHand = APIC_DSH_Self;

    /* Disable interrupts so that we can change IRR without being interrupted */
    Flags = __readeflags();
    _disable();

    /* Wait for the APIC to be idle */
    do
    {
        IcrStatus.Long0 = ApicRead(APIC_ICR0);
    } while (IcrStatus.DeliveryStatus);

    /* Write high dword first, then low dword to send the interrupt */
    ApicWrite(APIC_ICR1, Icr.Long1);
    ApicWrite(APIC_ICR0, Icr.Long0);

    /* Wait until we see the interrupt request.
     * It will stay in requested state until we re-enable interrupts.
     */
    while (!(ApicRead(Irr) & IrrBit))
    {
        YieldProcessor();
    }

    /* Finally, restore the original interrupt state */
    if (Flags & EFLAGS_INTERRUPT_MASK)
    {
        _enable();
    }
}

FORCEINLINE
VOID
ApicSendEOI(void)
{
    ApicWrite(APIC_EOI, 0);
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
    /* Return the field in the PCR */
    return (KIRQL)__readfsbyte(FIELD_OFFSET(KPCR, Irql));
#else
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
HalpIrqToVector(
    _In_ UCHAR Irq)
{
    /* Firmware can leave a vector in an entry, so use the allocation table */
    return HalpGsivToVector[Irq];
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
ApicInitializeLocalApic(
    _In_ ULONG Cpu)
{
    APIC_BASE_ADDRESS_REGISTER BaseRegister;
    APIC_SPURIOUS_INERRUPT_REGISTER SpIntRegister;
    APIC_VERSION_REGISTER VersionRegister;
    APIC_EXTENDED_FEATURE_REGISTER FeatureRegister;
    LVT_REGISTER LvtEntry;
    ULONG Index;

    /* Enable the APIC if it wasn't yet */
    BaseRegister.LongLong = __readmsr(MSR_APIC_BASE);
    BaseRegister.Enable = 1;
    BaseRegister.BootStrapCPUCore = (Cpu == 0);
    __writemsr(MSR_APIC_BASE, BaseRegister.LongLong);

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
    LvtEntry.Vector = APIC_FREE_VECTOR;
    LvtEntry.MessageType = APIC_MT_Fixed;
    LvtEntry.DeliveryStatus = 0;
    LvtEntry.RemoteIRR = 0;
    LvtEntry.TriggerMode = APIC_TGM_Edge;
    LvtEntry.Mask = 1;
    LvtEntry.TimerMode = 0;

    /* Initialize and mask LVTs. Accessing one this APIC lacks is an illegal register error */
    VersionRegister.Long = ApicRead(APIC_VER);
    ApicWrite(APIC_TMRLVTR, LvtEntry.Long);

    /* The performance counter LVT came with 5 LVT entries, the thermal LVT with 6 */
    if (VersionRegister.MaxLVT >= 4)
    {
        ApicWrite(APIC_PCLVTR, LvtEntry.Long);
    }
    if (VersionRegister.MaxLVT >= 5)
    {
        ApicWrite(APIC_THRMLVTR, LvtEntry.Long);
    }

    /* AMD extended LVTs reset unmasked */
    if (VersionRegister.ExtRegSpacePresent)
    {
        FeatureRegister.Long = ApicRead(APIC_EAFR);
        for (Index = 0; Index < min(FeatureRegister.ExtLvtCount, 4); Index++)
        {
            ApicWrite((APIC_REGISTER)(APIC_EXT0LVTR + Index * 0x10), LvtEntry.Long);
        }
    }

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

    /* Clear any error from the writes above */
    ApicWrite(APIC_ESR, 0);

    /* Enable error LVTR */
    KeRegisterInterruptHandler(APIC_ERROR_VECTOR, ApicErrorService);
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

/* Raises to HIGH_LEVEL, since HalEnableSystemInterrupt runs above DISPATCH_LEVEL */
KIRQL
NTAPI
HalpAcquireVectorLock(VOID)
{
    KIRQL OldIrql;

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&HalpVectorLock);
    return OldIrql;
}

VOID
NTAPI
HalpReleaseVectorLock(
    _In_ KIRQL OldIrql)
{
    KeReleaseSpinLockFromDpcLevel(&HalpVectorLock);
    KeLowerIrql(OldIrql);
}

/**
 * @brief
 * Tells whether a vector can go to the ACPI driver. It can when the HAL has not
 * handed it out, or when the ACPI driver already holds it from an earlier query.
 */
BOOLEAN
NTAPI
HalpIsVectorGrantable(
    _In_ ULONG Vector)
{
    if (Vector > 0xFF)
        return FALSE;

    return HalpAcpiGrantedVector[Vector] || (HalpVectorToIndex[Vector] == APIC_FREE_VECTOR);
}

VOID
NTAPI
HalpGrantVector(
    _In_ ULONG Vector)
{
    if (Vector <= 0xFF)
        HalpAcpiGrantedVector[Vector] = TRUE;
}

BOOLEAN
NTAPI
HalpIsVectorGranted(
    _In_ ULONG Vector)
{
    return (Vector <= 0xFF) && HalpAcpiGrantedVector[Vector];
}

UCHAR
NTAPI
HalpAllocateSystemInterrupt(
    _In_ UCHAR Irq,
    _In_ UCHAR Vector)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;

    ASSERT(Irq < HalpMaxGsi);
    ASSERT(HalpVectorToIndex[Vector] == APIC_FREE_VECTOR);

    /* Setup a redirection entry */
    ReDirReg.Vector = Vector;
    ReDirReg.MessageType = APIC_MT_LowestPriority;
    ReDirReg.DestinationMode = APIC_DM_Logical;
    ReDirReg.DeliveryStatus = 0;
    ReDirReg.Polarity = 0;
    ReDirReg.RemoteIRR = 0;
    ReDirReg.TriggerMode = APIC_TGM_Edge;
    ReDirReg.Mask = 1;
    ReDirReg.Reserved = 0;
    ReDirReg.Destination = ApicRead(APIC_ID) >> 24;

    /* Initialize entry */
    ApicWriteIORedirectionEntry(Irq, ReDirReg);

    /* Record the allocation in both tables */
    HalpVectorToIndex[Vector] = Irq;
    HalpGsivToVector[Irq] = Vector;

    return Vector;
}

ULONG
NTAPI
HalpGetRootInterruptVector(
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL OutIrql,
    _Out_ PKAFFINITY OutAffinity)
{
    PHALP_IOAPIC_UNIT Unit;
    UCHAR Vector;
    KIRQL Irql, OldIrql;

    /* Resources that already hold a system vector are translated here too, so stay quiet */
    if (!HalpFindIoApicInput(BusInterruptLevel, &Unit))
    {
        DPRINT("No I/O APIC serves input %lu\n", BusInterruptLevel);
        *OutAffinity = 0;
        *OutIrql = 0;
        return 0;
    }

    OldIrql = HalpAcquireVectorLock();

    /* Get the vector currently registered */
    Vector = HalpIrqToVector(BusInterruptLevel);

    /* Check if it's used */
    if (Vector != APIC_FREE_VECTOR)
    {
        /* Calculate IRQL */
        NT_ASSERT(HalpVectorToIndex[Vector] == BusInterruptLevel);
        *OutIrql = HalpVectorToIrql(Vector);
    }
    else
    {
        ULONG Offset;

        /* Outer loop to find alternative slots, when all IRQLs are in use */
        for (Offset = 0; Offset < 15; Offset++)
        {
            /* Loop allowed IRQL range */
            for (Irql = CLOCK_LEVEL - 1; Irql >= CMCI_LEVEL; Irql--)
            {
                /* Profile level is not a device level, and on x86 several
                   device IRQLs share one priority. Each priority is tried once */
                if ((IrqlToTpr(Irql) >= IrqlToTpr(PROFILE_LEVEL)) ||
                    (IrqlToTpr(Irql) == IrqlToTpr(Irql + 1)))
                {
                    continue;
                }

                /* Calculate the vactor */
                Vector = IrqlToTpr(Irql) + Offset;

                /* Check if the vector is free, granted ones are the ACPI driver's to hand out */
                if ((HalpVectorToIrq(Vector) == APIC_FREE_VECTOR) &&
                    !HalpAcpiGrantedVector[Vector])
                {
                    /* Found one, allocate the interrupt. The IRQL is the one
                       the vector's priority maps to, as later lookups report */
                    Vector = HalpAllocateSystemInterrupt(BusInterruptLevel, Vector);
                    *OutIrql = HalpVectorToIrql(Vector);
                    goto Exit;
                }
            }
        }

        HalpReleaseVectorLock(OldIrql);
        DPRINT1("Failed to get an interrupt vector for IRQ %lu\n", BusInterruptLevel);
        *OutAffinity = 0;
        *OutIrql = 0;
        return 0;
    }

Exit:
    HalpReleaseVectorLock(OldIrql);

    *OutAffinity = HalpDefaultInterruptAffinity;
    ASSERT(HalpDefaultInterruptAffinity);

    return Vector;
}

/* Returns the IRQL a device vector runs at */
KIRQL
NTAPI
HalConvertDeviceIdtToIrql(
    _In_ ULONG IdtEntry)
{
    if (IdtEntry > 0xFF)
        return PASSIVE_LEVEL;

    return HalpVectorToIrql((UCHAR)IdtEntry);
}

/**
 * @brief
 * Maps an I/O APIC and adds it to the units in use.
 *
 * @param[in] PhysicalBase
 * Physical address of the I/O APIC registers.
 *
 * @param[in] InputBase
 * Global system interrupt of the first redirection entry.
 */
static
VOID
NTAPI
HalpMapIoApic(
    _In_ ULONG PhysicalBase,
    _In_ ULONG InputBase)
{
    PHALP_IOAPIC_UNIT Unit;
    PHARDWARE_PTE Pte;
    ULONG Version;

    if ((HalpIoApicCount == HALP_MAX_IOAPICS) || (InputBase >= HALP_MAX_INPUTS))
    {
        DPRINT1("I/O APIC at 0x%lx with input base %lu ignored\n", PhysicalBase, InputBase);
        return;
    }

    /* Each I/O APIC takes the next page from IOAPIC_BASE */
    Unit = &HalpIoApics[HalpIoApicCount];
    Unit->Base = (ULONG_PTR)IOAPIC_BASE + HalpIoApicCount * PAGE_SIZE;
    Pte = HalAddressToPte(Unit->Base);
    Pte->PageFrameNumber = PhysicalBase / PAGE_SIZE;
    Pte->Valid = 1;
    Pte->Write = 1;
    Pte->Owner = 1;
    Pte->CacheDisable = 1;
    Pte->Global = 1;
    _ReadWriteBarrier();
    Unit->Base += BYTE_OFFSET(PhysicalBase);

    /* Nothing decodes the address when all bits read back set */
    Version = IOApicRead(Unit->Base, IOAPIC_VER);
    if (Version == 0xFFFFFFFF)
    {
        DPRINT1("No I/O APIC responds at 0x%lx\n", PhysicalBase);
        return;
    }

    /* Bits 23:16 hold the index of the last redirection entry, the register index caps the count */
    Unit->InputBase = InputBase;
    Unit->InputCount = min(((Version >> 16) & 0xFF) + 1, HALP_IOAPIC_MAX_ENTRIES);
    Unit->InputCount = min(Unit->InputCount, HALP_MAX_INPUTS - InputBase);
    HalpMaxGsi = max(HalpMaxGsi, InputBase + Unit->InputCount);
    HalpIoApicCount++;
}

VOID
NTAPI
ApicInitializeIOApic(VOID)
{
    PHALP_IOAPIC_UNIT Unit;
    IOAPIC_REDIRECTION_REGISTER ReDirReg, Current;
    ULONG Index, Vector, Input;
    UCHAR Register;

    KeInitializeSpinLock(&HalpVectorLock);

    /* Use the I/O APICs from the MADT, or the default one without it */
    for (Index = 0; Index < HALP_APIC_INFO_TABLE_IOAPIC_NUMBER; Index++)
    {
        if (HalpApicInfoTable.IoApicPA[Index] != 0)
        {
            HalpMapIoApic(HalpApicInfoTable.IoApicPA[Index],
                          HalpApicInfoTable.IoApicIrqBase[Index]);
        }
    }
    if (HalpIoApicCount == 0)
    {
        HalpMapIoApic(IOAPIC_PHYS_BASE, 0);
    }

    /* Setup a redirection entry */
    ReDirReg.LongLong = 0;
    ReDirReg.Vector = APIC_FREE_VECTOR;
    ReDirReg.MessageType = APIC_MT_Fixed;
    ReDirReg.DestinationMode = APIC_DM_Physical;
    ReDirReg.TriggerMode = APIC_TGM_Edge;
    ReDirReg.Mask = 1;
    ReDirReg.Destination = ApicRead(APIC_ID) >> 24;

    /* Initialize all entries of all I/O APICs */
    for (Unit = HalpIoApics; Unit < HalpIoApics + HalpIoApicCount; Unit++)
    {
        for (Input = Unit->InputBase; Input < Unit->InputBase + Unit->InputCount; Input++)
        {
            /* Entries the firmware routes to SMI stay as they are */
            Register = (UCHAR)(IOAPIC_REDTBL + 2 * (Input - Unit->InputBase));
            Current.Long0 = IOApicRead(Unit->Base, Register);
            if (Current.MessageType == APIC_MT_SMI)
            {
                Current.Long1 = IOApicRead(Unit->Base, Register + 1);
                HalpIoApicShadow[Input] = Current;
                continue;
            }

            ApicWriteIORedirectionEntry(Input, ReDirReg);
        }
    }

    /* Init the vector and input tables */
    for (Vector = 0; Vector <= 255; Vector++)
    {
        HalpVectorToIndex[Vector] = APIC_FREE_VECTOR;
        HalpGsivToVector[Vector] = APIC_FREE_VECTOR;
    }

    /* Enable the timer interrupt (but keep it masked) */
    ReDirReg.Vector = APIC_CLOCK_VECTOR;
    ReDirReg.MessageType = APIC_MT_Fixed;
    ReDirReg.DestinationMode = APIC_DM_Physical;
    ReDirReg.TriggerMode = APIC_TGM_Level;
    ReDirReg.Mask = 1;
    ReDirReg.Destination = ApicRead(APIC_ID) >> 24;
    ApicWriteIORedirectionEntry(APIC_CLOCK_INDEX, ReDirReg);
}

VOID
NTAPI
HalpInitializePICs(
    _In_ BOOLEAN EnableInterrupts)
{
    ULONG_PTR EFlags;

    /* Save EFlags and disable interrupts */
    EFlags = __readeflags();
    _disable();

    /* Initialize and mask the PIC */
    HalpInitializeLegacyPICs();

    /* Initialize the I/O APIC */
    ApicInitializeIOApic();

    /* Manually reserve some vectors */
    HalpVectorToIndex[APC_VECTOR] = APIC_RESERVED_VECTOR;
    HalpVectorToIndex[DISPATCH_VECTOR] = APIC_RESERVED_VECTOR;
    HalpVectorToIndex[APIC_CLOCK_VECTOR] = APIC_CLOCK_INDEX;
    HalpGsivToVector[APIC_CLOCK_INDEX] = APIC_CLOCK_VECTOR;
    HalpVectorToIndex[CLOCK_IPI_VECTOR] = APIC_RESERVED_VECTOR;
    HalpVectorToIndex[APIC_SPURIOUS_VECTOR] = APIC_RESERVED_VECTOR;
#ifndef _M_AMD64
    HalpVectorToIndex[APIC_IPI_VECTOR] = APIC_RESERVED_VECTOR;
#endif

    /* Local APIC sources, all above the device vector range */
    HalpVectorToIndex[APIC_PROFILE_VECTOR] = APIC_RESERVED_VECTOR;
    HalpVectorToIndex[APIC_ERROR_VECTOR] = APIC_RESERVED_VECTOR;
    HalpVectorToIndex[APIC_IPI_VECTOR] = APIC_RESERVED_VECTOR;
    HalpVectorToIndex[APIC_NMI_VECTOR] = APIC_RESERVED_VECTOR;

    /* Set interrupt handlers in the IDT */
    KeRegisterInterruptHandler(APIC_CLOCK_VECTOR, HalpClockInterrupt);
    KeRegisterInterruptHandler(CLOCK_IPI_VECTOR, HalpClockIpi);
#ifndef _M_AMD64
    KeRegisterInterruptHandler(APC_VECTOR, HalpApcInterrupt);
    KeRegisterInterruptHandler(DISPATCH_VECTOR, HalpDispatchInterrupt);
    KeRegisterInterruptHandler(APIC_IPI_VECTOR, HalpIpiInterrupt);
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

VOID
DECLSPEC_NORETURN
FASTCALL
HalpIpiInterruptHandler(
    _In_ PKTRAP_FRAME TrapFrame)
{
    KIRQL OldIrql;

    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);

    /* Get the current IRQL */
    OldIrql = ApicGetCurrentIrql();

    /* Raise to IPI_LEVEL */
    ApicRaiseIrql(IPI_LEVEL);

    /* EOI first, a freeze request may not return for a long time */
    ApicSendEOI();

    /* Service the requests with interrupts still disabled */
    KiIpiServiceRoutine(TrapFrame, NULL);

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
    ApicRequestSelfInterrupt(IrqlToSoftVector(Irql));
}

VOID
FASTCALL
HalClearSoftwareInterrupt(
    IN KIRQL Irql)
{
    /* Nothing to do */
}


/* SYSTEM INTERRUPTS **********************************************************/

/**
 * @brief
 * Asks the ACPI driver for the input of a vector it allocated.
 *
 * @param[in] Vector
 * The interrupt vector to look up.
 *
 * @param[out] Input
 * Receives the global system interrupt of the vector.
 *
 * @param[out] Polarity
 * Receives the polarity reported for the input.
 *
 * @return
 * STATUS_SUCCESS when an I/O APIC serves the input, STATUS_INVALID_PARAMETER
 * for a message-signaled vector, or another error status otherwise.
 */
static
NTSTATUS
NTAPI
HalpQueryVectorInput(
    _In_ ULONG Vector,
    _Out_ PULONG Input,
    _Out_ PKINTERRUPT_POLARITY Polarity)
{
    PHALP_IOAPIC_UNIT Unit;
    NTSTATUS Status;

    *Input = 0;
    *Polarity = InterruptPolarityUnknown;

    if (HalGetVectorInputOverride == NULL)
        return STATUS_NOT_FOUND;

    Status = HalGetVectorInputOverride(Vector, HalpDefaultInterruptAffinity, Input, Polarity);
    if (NT_SUCCESS(Status) && !HalpFindIoApicInput(*Input, &Unit))
    {
        DPRINT1("No I/O APIC serves input %lu of vector 0x%lx\n", *Input, Vector);
        return STATUS_NOT_FOUND;
    }

    return Status;
}

/**
 * @brief
 * Picks the polarity to program for an input.
 *
 * @param[in] Input
 * The global system interrupt being enabled.
 *
 * @param[in] Mode
 * The trigger mode of the input.
 *
 * @param[in] Reported
 * The polarity reported by the ACPI driver, or InterruptPolarityUnknown.
 *
 * @return
 * The reported polarity, else the one of an MADT interrupt source override
 * targeting the input, else the default of the trigger mode.
 */
static
KINTERRUPT_POLARITY
NTAPI
HalpGetInputPolarity(
    _In_ ULONG Input,
    _In_ KINTERRUPT_MODE Mode,
    _In_ KINTERRUPT_POLARITY Reported)
{
    ULONG Irq;

    if (Reported != InterruptPolarityUnknown)
        return Reported;

    for (Irq = 0; Irq < HALP_ISA_IRQ_COUNT; Irq++)
    {
        if ((HalpApicInfoTable.IsaIrqPolarity[Irq] != InterruptPolarityUnknown) &&
            (HalpApicInfoTable.IsaIrqGsi[Irq] == Input))
        {
            return HalpApicInfoTable.IsaIrqPolarity[Irq];
        }
    }

    /* PCI lines are level and active low, ISA lines are edge and active high */
    return (Mode == LevelSensitive) ? InterruptActiveLow : InterruptActiveHigh;
}

BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    _In_ ULONG Vector,
    _In_ KIRQL Irql,
    _In_ KINTERRUPT_MODE InterruptMode)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    KINTERRUPT_POLARITY Polarity;
    NTSTATUS Status;
    ULONG Input;
    UCHAR Index;
    UCHAR Previous;
    KIRQL OldIrql;
    ASSERT(Irql <= HIGH_LEVEL);
    ASSERT((IrqlToTpr(Irql) & 0xF0) == (Vector & 0xF0));

    /* Get the irq for this vector */
    Index = HalpVectorToIndex[Vector];

    /* Message-signaled interrupts have no input to unmask */
    if (Index == APIC_MSI_VECTOR)
    {
        return TRUE;
    }

    /* The ACPI driver allocates vectors itself when it arbitrates interrupts.
       Ask it before taking the lock, since this calls out of the HAL */
    Polarity = InterruptPolarityUnknown;
    Status = STATUS_NOT_FOUND;
    Input = 0;
    if (Index == APIC_FREE_VECTOR)
    {
        Status = HalpQueryVectorInput(Vector, &Input, &Polarity);
    }

    OldIrql = HalpAcquireVectorLock();

    /* Another processor may have claimed the vector while the lock was not held */
    Index = HalpVectorToIndex[Vector];
    if (Index == APIC_FREE_VECTOR)
    {
        if (Status == STATUS_INVALID_PARAMETER)
        {
            HalpVectorToIndex[Vector] = APIC_MSI_VECTOR;
            HalpReleaseVectorLock(OldIrql);
            return TRUE;
        }

        /* The ACPI driver owns the input, so a vector the HAL gave it and never enabled is dropped */
        if (NT_SUCCESS(Status))
        {
            Previous = HalpGsivToVector[Input];
            if ((Previous != APIC_FREE_VECTOR) && ApicReadIORedirectionEntry(Input).Mask)
            {
                HalpVectorToIndex[Previous] = APIC_FREE_VECTOR;
                HalpGsivToVector[Input] = APIC_FREE_VECTOR;
            }
        }

        /* Don't take over an input another vector is using */
        if (!NT_SUCCESS(Status) || (HalpGsivToVector[Input] != APIC_FREE_VECTOR))
        {
            HalpReleaseVectorLock(OldIrql);
            DPRINT1("No free input for vector 0x%lx, status 0x%lx\n", Vector, Status);
            return FALSE;
        }

        HalpAllocateSystemInterrupt((UCHAR)Input, (UCHAR)Vector);
        Index = (UCHAR)Input;
    }

    /* Check if its valid */
    if (Index >= HalpMaxGsi)
    {
        /* Message-signaled vectors have nothing to unmask, others are not in use */
        HalpReleaseVectorLock(OldIrql);
        return (Index == APIC_MSI_VECTOR);
    }

    /* Read the redirection entry */
    ReDirReg = ApicReadIORedirectionEntry(Index);

    /* Check if the interrupt is already enabled */
    if (ReDirReg.Mask == FALSE)
    {
        /* If the vector matches, there is nothing more to do,
           otherwise something is wrong. */
        HalpReleaseVectorLock(OldIrql);
        return (ReDirReg.Vector == Vector);
    }

    /* Set up the redirection entry */
    ReDirReg.Vector = Vector;
    ReDirReg.MessageType = APIC_MT_Fixed;
    ReDirReg.DestinationMode = APIC_DM_Physical;
    ReDirReg.Destination = ApicRead(APIC_ID) >> 24;
    ReDirReg.TriggerMode = (InterruptMode == LevelSensitive) ?
        APIC_TGM_Level : APIC_TGM_Edge;
    ReDirReg.Polarity =
        (HalpGetInputPolarity(Index, InterruptMode, Polarity) == InterruptActiveLow);
    ReDirReg.Mask = FALSE;

    /* Write back the entry */
    ApicWriteIORedirectionEntry(Index, ReDirReg);

    HalpReleaseVectorLock(OldIrql);
    return TRUE;
}

VOID
NTAPI
HalDisableSystemInterrupt(
    _In_ ULONG Vector,
    _In_ KIRQL Irql)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    UCHAR Index;
    KIRQL OldIrql;
    ASSERT(Irql <= HIGH_LEVEL);
    ASSERT(Vector < RTL_NUMBER_OF(HalpVectorToIndex));

    OldIrql = HalpAcquireVectorLock();

    /* Message-signaled and reserved vectors have no entry to mask */
    Index = HalpVectorToIndex[Vector];
    if (Index < HalpMaxGsi)
    {
        ReDirReg = ApicReadIORedirectionEntry(Index);
        ReDirReg.Mask = 1;
        ApicWriteIORedirectionEntry(Index, ReDirReg);
    }
    else
    {
        HalpReleaseMessageMark(Vector);
    }

    HalpReleaseVectorLock(OldIrql);
}

BOOLEAN
NTAPI
HalBeginSystemInterrupt(
    _In_ KIRQL Irql,
    _In_ ULONG Vector,
    _Out_ PKIRQL OldIrql)
{
    KIRQL CurrentIrql;

    /* Get the current IRQL */
    CurrentIrql = ApicGetCurrentIrql();

#ifdef APIC_LAZY_IRQL
    /* Check if this interrupt is allowed */
    if (CurrentIrql >= Irql)
    {
        /* It is not, set the real Irql in the TPR! */
        ApicWrite(APIC_TPR, IrqlToTpr(CurrentIrql));

        /* Save the new hard IRQL in the IRR field */
        KeGetPcr()->IRR = CurrentIrql;

        /* End this interrupt */
        ApicSendEOI();

        /* This is an input of a controller, a reserved source or a message */
        ASSERT((HalpVectorToIndex[Vector] < HalpMaxGsi) ||
               (HalpVectorToIndex[Vector] == APIC_RESERVED_VECTOR) ||
               (HalpVectorToIndex[Vector] == APIC_MSI_VECTOR));

        /* The EOI above already released the input, so re-request the vector
           here to be handled once the Irql allows it */
        ApicRequestSelfInterrupt(Vector);

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


/* INTERRUPT CONNECTION *******************************************************/

/* Physical mode names one processor, so a line settles for the lowest of the set */
static
NTSTATUS
NTAPI
HalpBuildLineDestination(
    _In_ KAFFINITY TargetProcessors,
    _Out_ PBOOLEAN Logical,
    _Out_ PUCHAR Destination)
{
    NTSTATUS Status;

    Status = HalpBuildInterruptDestination(TargetProcessors, Logical, Destination);
    if (!NT_SUCCESS(Status) && (TargetProcessors & (TargetProcessors - 1)))
    {
        TargetProcessors &= (0 - TargetProcessors);
        Status = HalpBuildInterruptDestination(TargetProcessors, Logical, Destination);
    }

    return Status;
}

/**
 * @brief
 * Points an I/O APIC input at a vector and unmasks it. The caller holds the
 * vector lock.
 *
 * @param[in] Input
 * Global system interrupt to program.
 *
 * @param[in] Vector
 * Vector the input delivers.
 *
 * @param[in] Mode
 * Level or edge triggered.
 *
 * @param[in] Polarity
 * Polarity of the input. InterruptPolarityUnknown takes active low for a level
 * input and active high for an edge one.
 *
 * @param[in] Logical
 * TRUE when Destination holds logical IDs.
 *
 * @param[in] Destination
 * Destination field of the redirection entry.
 */
static
VOID
NTAPI
HalpProgramInterruptInput(
    _In_ ULONG Input,
    _In_ ULONG Vector,
    _In_ KINTERRUPT_MODE Mode,
    _In_ KINTERRUPT_POLARITY Polarity,
    _In_ BOOLEAN Logical,
    _In_ UCHAR Destination)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;

    Polarity = HalpGetInputPolarity(Input, Mode, Polarity);

    ReDirReg = ApicReadIORedirectionEntry(Input);
    ReDirReg.Vector = Vector;
    ReDirReg.Destination = Destination;
    if (Logical)
    {
        ReDirReg.MessageType = APIC_MT_LowestPriority;
        ReDirReg.DestinationMode = APIC_DM_Logical;
    }
    else
    {
        ReDirReg.MessageType = APIC_MT_Fixed;
        ReDirReg.DestinationMode = APIC_DM_Physical;
    }
    ReDirReg.TriggerMode = (Mode == LevelSensitive) ? APIC_TGM_Level : APIC_TGM_Edge;
    ReDirReg.Polarity = (Polarity == InterruptActiveLow);
    ReDirReg.Mask = 0;
    ApicWriteIORedirectionEntry(Input, ReDirReg);
}

/**
 * @brief
 * Enables the interrupt a connection data block describes. Only blocks holding
 * a single vector are accepted.
 */
NTSTATUS
NTAPI
HalEnableInterrupt(
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData)
{
    PINTERRUPT_VECTOR_DATA VectorData;
    PHALP_IOAPIC_UNIT Unit;
    ULONG Vector, Input;
    UCHAR Index, Previous, Destination;
    KAFFINITY Targets;
    BOOLEAN Logical;
    NTSTATUS Status;
    KIRQL OldIrql;

    if ((ConnectionData == NULL) || (ConnectionData->Count != 1))
    {
        return STATUS_INVALID_PARAMETER;
    }

    VectorData = &ConnectionData->Vectors[0];
    Vector = VectorData->Vector;
    if (Vector > 0xFF)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* The vector alone decides the IRQL, so the caller cannot ask for another */
    if (VectorData->Irql != HalpVectorToIrql((UCHAR)Vector))
    {
        DPRINT1("Vector 0x%lx cannot run at IRQL %u\n", Vector, VectorData->Irql);
        return STATUS_INVALID_PARAMETER;
    }

    switch (VectorData->Type)
    {
        case InterruptTypeXapicMessage:
        case InterruptTypeHypertransport:
        case InterruptTypeMessageRequest:
        {
            /* The vector has to be free or already carrying messages */
            Status = STATUS_SUCCESS;
            OldIrql = HalpAcquireVectorLock();
            Index = HalpVectorToIndex[Vector];
            if (Index == APIC_FREE_VECTOR)
                HalpVectorToIndex[Vector] = APIC_MSI_VECTOR;
            else if (Index != APIC_MSI_VECTOR)
                Status = STATUS_INVALID_PARAMETER;
            HalpReleaseVectorLock(OldIrql);
            return Status;
        }

        case InterruptTypeControllerInput:
        {
            Input = VectorData->ControllerInput.Gsiv;
            if (!HalpFindIoApicInput(Input, &Unit))
                return STATUS_INVALID_PARAMETER;

#if (NTDDI_VERSION >= NTDDI_WIN7)
            if (VectorData->TargetProcessors.Group != 0)
                return STATUS_INVALID_PARAMETER;

            Targets = VectorData->TargetProcessors.Mask;
#else
            Targets = VectorData->TargetProcessors;
#endif

            /* Settle the destination before anything is committed */
            Status = HalpBuildLineDestination(Targets, &Logical, &Destination);
            if (!NT_SUCCESS(Status))
                return Status;

            OldIrql = HalpAcquireVectorLock();

            Index = HalpVectorToIndex[Vector];
            if (Index == APIC_FREE_VECTOR)
            {
                /* Give up the vector this input was handed earlier */
                Previous = HalpIrqToVector((UCHAR)Input);
                if (Previous != APIC_FREE_VECTOR)
                    HalpVectorToIndex[Previous] = APIC_FREE_VECTOR;

                HalpVectorToIndex[Vector] = (UCHAR)Input;
                HalpGsivToVector[Input] = (UCHAR)Vector;
            }
            else if (Index != Input)
            {
                HalpReleaseVectorLock(OldIrql);
                return STATUS_INVALID_PARAMETER;
            }

            HalpProgramInterruptInput(Input,
                                      Vector,
                                      VectorData->Mode,
                                      VectorData->Polarity,
                                      Logical,
                                      Destination);

            HalpReleaseVectorLock(OldIrql);
            return STATUS_SUCCESS;
        }

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

/**
 * @brief
 * Turns off an interrupt enabled through HalEnableInterrupt. A message vector
 * from a HAL message block stays allocated until its owner gives it back.
 */
NTSTATUS
NTAPI
HalDisableInterrupt(
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData)
{
    PINTERRUPT_VECTOR_DATA VectorData;
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    PHALP_IOAPIC_UNIT Unit;
    ULONG Vector, Input;
    NTSTATUS Status;
    KIRQL OldIrql;

    if ((ConnectionData == NULL) || (ConnectionData->Count != 1))
    {
        return STATUS_INVALID_PARAMETER;
    }

    VectorData = &ConnectionData->Vectors[0];
    Vector = VectorData->Vector;
    if (Vector > 0xFF)
    {
        return STATUS_INVALID_PARAMETER;
    }

    switch (VectorData->Type)
    {
        case InterruptTypeXapicMessage:
        case InterruptTypeHypertransport:
        case InterruptTypeMessageRequest:
            OldIrql = HalpAcquireVectorLock();
            HalpReleaseMessageMark(Vector);
            HalpReleaseVectorLock(OldIrql);
            return STATUS_SUCCESS;

        case InterruptTypeControllerInput:
        {
            Input = VectorData->ControllerInput.Gsiv;
            if (!HalpFindIoApicInput(Input, &Unit))
            {
                return STATUS_INVALID_PARAMETER;
            }

            Status = STATUS_INVALID_PARAMETER;
            OldIrql = HalpAcquireVectorLock();
            if (HalpVectorToIndex[Vector] == Input)
            {
                ReDirReg = ApicReadIORedirectionEntry(Input);
                ReDirReg.Mask = 1;
                ApicWriteIORedirectionEntry(Input, ReDirReg);
                Status = STATUS_SUCCESS;
            }
            HalpReleaseVectorLock(OldIrql);
            return Status;
        }

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

/**
 * @brief
 * Reports the input a vector arrives on and the polarity that input carries.
 * The private dispatch override answers instead when one is installed.
 */
NTSTATUS
NTAPI
HalGetVectorInput(
    _In_ ULONG Vector,
    _In_ KAFFINITY Affinity,
    _Out_ PULONG Input,
    _Out_ PKINTERRUPT_POLARITY Polarity)
{
    IOAPIC_REDIRECTION_REGISTER ReDirReg;
    UCHAR Index;
    KIRQL OldIrql;

    /* The override calls out of the HAL, so it runs without the lock */
    if (HalGetVectorInputOverride != NULL)
    {
        return HalGetVectorInputOverride(Vector, Affinity, Input, Polarity);
    }

    *Input = 0;
    *Polarity = InterruptPolarityUnknown;

    if (Vector > 0xFF)
    {
        return STATUS_INVALID_PARAMETER;
    }

    OldIrql = HalpAcquireVectorLock();
    Index = HalpVectorToIndex[Vector];
    if (Index >= HalpMaxGsi)
    {
        HalpReleaseVectorLock(OldIrql);
        return STATUS_NOT_FOUND;
    }

    ReDirReg = ApicReadIORedirectionEntry(Index);
    HalpReleaseVectorLock(OldIrql);

    *Input = Index;
    *Polarity = ReDirReg.Polarity ? InterruptActiveLow : InterruptActiveHigh;
    return STATUS_SUCCESS;
}

/* ACPI POWER MANAGEMENT ******************************************************/

/**
 * @brief
 * Returns the version register of the I/O APIC whose first input is
 * InterruptBase, or zero when no I/O APIC starts there. The reserved high byte
 * carries the number of inputs instead.
 */
ULONG
NTAPI
HalpGetInterruptControllerVersion(
    _In_ ULONG InterruptBase)
{
    PHALP_IOAPIC_UNIT Unit;
    ULONG Version;
    KIRQL OldIrql;

    for (Unit = HalpIoApics; Unit < HalpIoApics + HalpIoApicCount; Unit++)
    {
        if (Unit->InputBase != InterruptBase)
        {
            continue;
        }

        OldIrql = HalpAcquireVectorLock();
        Version = IOApicRead(Unit->Base, IOAPIC_VER);
        HalpReleaseVectorLock(OldIrql);

        /* Report the inputs in use, which may be fewer than the unit has */
        Version &= 0xFF;
        Version |= (Unit->InputCount - 1) << 16;
        Version |= Unit->InputCount << 24;
        return Version;
    }

    return 0;
}

BOOLEAN
NTAPI
HalpIsInterruptInputValid(
    _In_ ULONG Input)
{
    PHALP_IOAPIC_UNIT Unit;

    return HalpFindIoApicInput(Input, &Unit);
}

/* Writes every redirection entry back from the shadow table */
VOID
NTAPI
HalpRestoreInterruptController(VOID)
{
    PHALP_IOAPIC_UNIT Unit;
    ULONG Input;
    KIRQL OldIrql;

    OldIrql = HalpAcquireVectorLock();

    for (Unit = HalpIoApics; Unit < HalpIoApics + HalpIoApicCount; Unit++)
    {
        for (Input = Unit->InputBase; Input < Unit->InputBase + Unit->InputCount; Input++)
        {
            ApicWriteIORedirectionEntry(Input, HalpIoApicShadow[Input]);
        }
    }

    HalpReleaseVectorLock(OldIrql);
}


/* IRQL MANAGEMENT ************************************************************/

#ifndef _M_AMD64
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

