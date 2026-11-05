/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     GICv2 and GICv3 interrupt controllers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* Distributor registers */
#define GICD_CTLR           0x0000
#define GICD_TYPER          0x0004
#define GICD_IGROUPR        0x0080
#define GICD_ISENABLER      0x0100
#define GICD_ICENABLER      0x0180
#define GICD_ICPENDR        0x0280
#define GICD_IPRIORITYR     0x0400
#define GICD_ICFGR          0x0C00
#define GICD_IROUTER        0x6000
#define GICD_PIDR2          0xFFE8

/* GICv2 only distributor registers */
#define GICD_ITARGETSR      0x0800
#define GICD_SGIR           0x0F00

#define GICD_SGIR_SELF      (2 << 24)

#define GICD_CTLR_RWP       0x80000000
#define GICD_CTLR_ENABLE    0x13

/* GICv2 enables both groups, with the security extensions bit 0 is group 1 */
#define GICV2_CTLR_ENABLE   0x03

/* Without the security extensions GICC_IAR only takes group 0, the
 * non-secure side of a GIC that has them cannot write the groups at all */
#define GICV2_GROUP         0

/* Redistributor registers, the SGI frame follows the control frame */
#define GICR_TYPER          0x0008
#define GICR_WAKER          0x0014
#define GICR_SGI_OFFSET     0x10000
#define GICR_IGROUPR0       0x0080
#define GICR_ISENABLER0     0x0100
#define GICR_ICENABLER0     0x0180
#define GICR_IPRIORITYR     0x0400
#define GICR_ICFGR1         0x0C04

#define GICR_TYPER_VLPIS    0x02
#define GICR_TYPER_LAST     0x10
#define GICR_WAKER_SLEEP    0x02
#define GICR_WAKER_ASLEEP   0x04

/* GICv2 CPU interface registers */
#define GICC_CTLR           0x0000
#define GICC_PMR            0x0004
#define GICC_BPR            0x0008
#define GICC_IAR            0x000C
#define GICC_EOIR           0x0010

/* GICv2 IAR keeps the sending processor of an SGI above the INTID */
#define GICC_IAR_INTID      0x3FF

/* GLOBALS *******************************************************************/

static PUCHAR HalpGicd;
static PUCHAR HalpGicrControl[MAXIMUM_PROCESSORS];
static PUCHAR HalpGicrSgi[MAXIMUM_PROCESSORS];
static ULONG HalpGicLineCount;
static KSPIN_LOCK HalpGicLock;

/* Nothing touches the CPU interface before the version is known */
static BOOLEAN HalpGicReady;

/* GICv2 state, the CPU interface is one banked frame */
static BOOLEAN HalpGicV2;
static PUCHAR HalpGicc;
static UCHAR HalpGicCpuMask[MAXIMUM_PROCESSORS];
static ULONG HalpGicLastIar[MAXIMUM_PROCESSORS];

/* FUNCTIONS *****************************************************************/

FORCEINLINE
ULONG
HalpGicRead(
    _In_ PUCHAR Base,
    _In_ ULONG Offset)
{
    return READ_REGISTER_ULONG((PULONG)(Base + Offset));
}

FORCEINLINE
VOID
HalpGicWrite(
    _In_ PUCHAR Base,
    _In_ ULONG Offset,
    _In_ ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)(Base + Offset), Value);
}

static
VOID
HalpGicWaitForDistributor(VOID)
{
    while (HalpGicRead(HalpGicd, GICD_CTLR) & GICD_CTLR_RWP)
        YieldProcessor();
}

/* GICR_TYPER keeps the affinity as Aff3.Aff2.Aff1.Aff0 in its upper half */
static
ULONG
HalpMpidrToAffinity(
    _In_ ULONG64 Mpidr)
{
    return (ULONG)(((Mpidr >> 32) & 0xFF) << 24) | (ULONG)(Mpidr & 0xFFFFFF);
}

static
BOOLEAN
HalpGicFindRedistributor(
    _In_ ULONG ProcessorNumber,
    _In_ ULONG64 Mpidr)
{
    PHYSICAL_ADDRESS Frame;
    PUCHAR Control;
    ULONG64 Typer;
    ULONG Stride, Affinity;

    Affinity = HalpMpidrToAffinity(Mpidr);
    Frame = HalpPlatform.GicrBase;

    for (;;)
    {
        Control = HalpMapPhysicalMemory(Frame, 1, TRUE);
        if (!Control)
            return FALSE;

        Typer = READ_REGISTER_ULONG64((PULONG64)(Control + GICR_TYPER));
        if ((ULONG)(Typer >> 32) == Affinity)
        {
            HalpGicrControl[ProcessorNumber] = Control;
            Frame.QuadPart += GICR_SGI_OFFSET;
            HalpGicrSgi[ProcessorNumber] = HalpMapPhysicalMemory(Frame, 1, TRUE);
            return (HalpGicrSgi[ProcessorNumber] != NULL);
        }

        HalpUnmapVirtualAddress(Control, 1);

        /* GICv4 adds two frames for virtual LPIs */
        Stride = (Typer & GICR_TYPER_VLPIS) ? 0x40000 : 0x20000;
        if ((Typer & GICR_TYPER_LAST) ||
            (HalpPlatform.GicrLength &&
             (Frame.QuadPart + Stride >= HalpPlatform.GicrBase.QuadPart + HalpPlatform.GicrLength)))
        {
            return FALSE;
        }

        Frame.QuadPart += Stride;
    }
}

VOID
NTAPI
HalpGicInitializeDistributor(VOID)
{
    ULONG i;

    KeInitializeSpinLock(&HalpGicLock);

    if (!HalpPlatform.GicdBase.QuadPart)
    {
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x6C1C, 0, 0, 0);
    }

    HalpGicd = HalpMapPhysicalMemory(HalpPlatform.GicdBase, HALP_GICD_SIZE / PAGE_SIZE, TRUE);
    if (!HalpGicd)
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x6C1C, 1, 0, 0);

    /* The MADT may leave the version at 0, ArchRev in PIDR2 has it then */
    if (!HalpPlatform.GicVersion)
        HalpPlatform.GicVersion = (UCHAR)((HalpGicRead(HalpGicd, GICD_PIDR2) >> 4) & 0xF);
    HalpGicV2 = (HalpPlatform.GicVersion <= 2);

    if (HalpGicV2)
    {
        if (!HalpPlatform.GiccBase.QuadPart)
            KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x6C1C, 3, 0, 0);

        HalpGicc = HalpMapPhysicalMemory(HalpPlatform.GiccBase, HALP_GICC_SIZE / PAGE_SIZE, TRUE);
        if (!HalpGicc)
            KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x6C1C, 4, 0, 0);
    }

    HalpGicWrite(HalpGicd, GICD_CTLR, 0);
    HalpGicWaitForDistributor();

    /* ITLinesNumber gives the SPI count in blocks of 32 */
    HalpGicLineCount = ((HalpGicRead(HalpGicd, GICD_TYPER) & 0x1F) + 1) * 32;
    if (HalpGicLineCount > GIC_MAX_INTID + 1)
        HalpGicLineCount = GIC_MAX_INTID + 1;

    /* Every SPI starts disabled, in group 1, level sensitive, at the lowest priority */
    for (i = GIC_SPI_FIRST; i < HalpGicLineCount; i += 32)
    {
        HalpGicWrite(HalpGicd, GICD_ICENABLER + i / 8, 0xFFFFFFFF);
        HalpGicWrite(HalpGicd, GICD_ICPENDR + i / 8, 0xFFFFFFFF);
        HalpGicWrite(HalpGicd, GICD_IGROUPR + i / 8, HalpGicV2 ? GICV2_GROUP : 0xFFFFFFFF);
    }

    for (i = GIC_SPI_FIRST; i < HalpGicLineCount; i += 16)
        HalpGicWrite(HalpGicd, GICD_ICFGR + i / 4, 0);

    for (i = GIC_SPI_FIRST; i < HalpGicLineCount; i += 4)
        HalpGicWrite(HalpGicd, GICD_IPRIORITYR + i, 0xF0F0F0F0);

    HalpGicWaitForDistributor();
    HalpGicWrite(HalpGicd, GICD_CTLR, HalpGicV2 ? GICV2_CTLR_ENABLE : GICD_CTLR_ENABLE);
    HalpGicWaitForDistributor();
    HalpGicReady = TRUE;
}

VOID
NTAPI
HalpGicInitializeCpu(
    _In_ ULONG ProcessorNumber)
{
    PUCHAR Control, Sgi;
    ULONG64 Mpidr;
    ULONG i;

    Mpidr = KeArm64ReadRegister(ARM64_MPIDR_EL1) & 0xFF00FFFFFFULL;
    HalpProcessorMpidr[ProcessorNumber] = Mpidr;

    if (HalpGicV2)
    {
        /* The banked first ITARGETSR names this processor, it reads 0 on a uniprocessor GIC */
        HalpGicCpuMask[ProcessorNumber] = READ_REGISTER_UCHAR(HalpGicd + GICD_ITARGETSR);

        /* SGIs and PPIs live in the banked distributor registers, PPIs level sensitive */
        HalpGicWrite(HalpGicd, GICD_ICENABLER, 0xFFFFFFFF);
        HalpGicWrite(HalpGicd, GICD_IGROUPR, GICV2_GROUP);
        HalpGicWrite(HalpGicd, GICD_ICFGR + 4, 0);

        for (i = 0; i < GIC_SPI_FIRST; i++)
            WRITE_REGISTER_UCHAR(HalpGicd + GICD_IPRIORITYR + i, HalpIrqlToPriority(i < GIC_SGI_COUNT ? (i & 0xF) : PASSIVE_LEVEL));

        /* No priority grouping, group 0 signalled as IRQ since FIQEn stays clear */
        HalpGicWrite(HalpGicc, GICC_PMR, HalpIrqlToPriority(HIGH_LEVEL));
        HalpGicWrite(HalpGicc, GICC_BPR, 0);
        HalpGicWrite(HalpGicc, GICC_CTLR, GICV2_CTLR_ENABLE);

        HalpGicWrite(HalpGicd, GICD_ISENABLER, (1 << APC_LEVEL) | (1 << DISPATCH_LEVEL) | (1 << IPI_LEVEL));
        return;
    }

    if (!HalpGicFindRedistributor(ProcessorNumber, Mpidr))
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x6C1C, 2, ProcessorNumber, (ULONG_PTR)Mpidr);

    Control = HalpGicrControl[ProcessorNumber];
    Sgi = HalpGicrSgi[ProcessorNumber];

    /* Wake the redistributor up */
    HalpGicWrite(Control, GICR_WAKER, HalpGicRead(Control, GICR_WAKER) & ~GICR_WAKER_SLEEP);
    while (HalpGicRead(Control, GICR_WAKER) & GICR_WAKER_ASLEEP)
        YieldProcessor();

    /* SGIs and PPIs start disabled in group 1, PPIs level sensitive */
    HalpGicWrite(Sgi, GICR_ICENABLER0, 0xFFFFFFFF);
    HalpGicWrite(Sgi, GICR_IGROUPR0, 0xFFFFFFFF);
    HalpGicWrite(Sgi, GICR_ICFGR1, 0);

    /* Software interrupts use their IRQL as the SGI number */
    for (i = 0; i < GIC_SPI_FIRST; i++)
        WRITE_REGISTER_UCHAR(Sgi + GICR_IPRIORITYR + i, HalpIrqlToPriority(i < GIC_SGI_COUNT ? (i & 0xF) : PASSIVE_LEVEL));

    /* System register access, no priority grouping, group 1 on */
    KeArm64WriteRegister(ARM64_ICC_SRE_EL1, KeArm64ReadRegister(ARM64_ICC_SRE_EL1) | 1);
    __isb(_ARM64_BARRIER_SY);
    KeArm64WriteRegister(ARM64_ICC_PMR_EL1, HalpIrqlToPriority(HIGH_LEVEL));
    KeArm64WriteRegister(ARM64_ICC_BPR1_EL1, 0);
    KeArm64WriteRegister(ARM64_ICC_CTLR_EL1, 0);
    KeArm64WriteRegister(ARM64_ICC_IGRPEN1_EL1, 1);
    __isb(_ARM64_BARRIER_SY);

    /* The software interrupts are always live, the priority mask holds them */
    HalpGicWrite(Sgi, GICR_ISENABLER0, (1 << APC_LEVEL) | (1 << DISPATCH_LEVEL) | (1 << IPI_LEVEL));
}

VOID
NTAPI
HalpGicSetPriority(
    _In_ ULONG IntId,
    _In_ UCHAR Priority)
{
    ULONG Cpu;

    /* GICv2 banks the private lines in the distributor, the write only reaches this processor */
    if ((IntId < GIC_SPI_FIRST) && !HalpGicV2)
    {
        for (Cpu = 0; Cpu < MAXIMUM_PROCESSORS; Cpu++)
        {
            if (HalpGicrSgi[Cpu])
                WRITE_REGISTER_UCHAR(HalpGicrSgi[Cpu] + GICR_IPRIORITYR + IntId, Priority);
        }
    }
    else
    {
        WRITE_REGISTER_UCHAR(HalpGicd + GICD_IPRIORITYR + IntId, Priority);
    }
}

BOOLEAN
NTAPI
HalpGicEnableInterrupt(
    _In_ ULONG IntId,
    _In_ KINTERRUPT_MODE Mode)
{
    ULONG Config, Shift, Cpu;
    KIRQL OldIrql;

    if (IntId >= HalpGicLineCount)
        return FALSE;

    if ((IntId < GIC_SPI_FIRST) && HalpGicV2)
    {
        HalpGicWrite(HalpGicd, GICD_ISENABLER, 1 << IntId);
        return TRUE;
    }

    if (IntId < GIC_SPI_FIRST)
    {
        for (Cpu = 0; Cpu < MAXIMUM_PROCESSORS; Cpu++)
        {
            if (HalpGicrSgi[Cpu])
                HalpGicWrite(HalpGicrSgi[Cpu], GICR_ISENABLER0, 1 << IntId);
        }
        return TRUE;
    }

    KeAcquireSpinLock(&HalpGicLock, &OldIrql);

    /* Two bits per line, bit 1 selects edge triggering */
    Shift = (IntId % 16) * 2 + 1;
    Config = HalpGicRead(HalpGicd, GICD_ICFGR + (IntId / 16) * 4);
    if (Mode == Latched)
        Config |= (1 << Shift);
    else
        Config &= ~(1 << Shift);
    HalpGicWrite(HalpGicd, GICD_ICFGR + (IntId / 16) * 4, Config);

    /* Route to the boot processor */
    if (HalpGicV2)
        WRITE_REGISTER_UCHAR(HalpGicd + GICD_ITARGETSR + IntId, HalpGicCpuMask[0] ? HalpGicCpuMask[0] : 1);
    else
        WRITE_REGISTER_ULONG64((PULONG64)(HalpGicd + GICD_IROUTER + IntId * 8), HalpProcessorMpidr[0]);

    HalpGicWrite(HalpGicd, GICD_ISENABLER + (IntId / 32) * 4, 1 << (IntId % 32));

    KeReleaseSpinLock(&HalpGicLock, OldIrql);
    return TRUE;
}

VOID
NTAPI
HalpGicDisableInterrupt(
    _In_ ULONG IntId)
{
    ULONG Cpu;

    if (IntId >= HalpGicLineCount)
        return;

    if ((IntId < GIC_SPI_FIRST) && HalpGicV2)
    {
        HalpGicWrite(HalpGicd, GICD_ICENABLER, 1 << IntId);
        return;
    }

    if (IntId < GIC_SPI_FIRST)
    {
        for (Cpu = 0; Cpu < MAXIMUM_PROCESSORS; Cpu++)
        {
            if (HalpGicrSgi[Cpu])
                HalpGicWrite(HalpGicrSgi[Cpu], GICR_ICENABLER0, 1 << IntId);
        }
        return;
    }

    HalpGicWrite(HalpGicd, GICD_ICENABLER + (IntId / 32) * 4, 1 << (IntId % 32));
}

/**
 * @brief Sends an SGI to every processor in the target set.
 *
 * ICC_SGI1R_EL1 takes the Aff3, Aff2 and Aff1 of the target cluster and a
 * bit mask of Aff0 values inside it, so one write is needed per target.
 */
VOID
NTAPI
HalpGicSendSgi(
    _In_ ULONG IntId,
    _In_ KAFFINITY TargetSet)
{
    ULONG64 Mpidr, Value;
    ULONG Cpu, Targets;

    if (HalpGicV2)
    {
        /* Sending to ourselves needs no CPU interface number */
        if (TargetSet == AFFINITY_MASK(KeGetCurrentProcessorNumber()))
        {
            HalpGicWrite(HalpGicd, GICD_SGIR, GICD_SGIR_SELF | (IntId & 0xF));
            return;
        }

        for (Targets = 0, Cpu = 0; TargetSet; Cpu++, TargetSet >>= 1)
        {
            if (TargetSet & 1)
                Targets |= HalpGicCpuMask[Cpu];
        }

        if (Targets)
            HalpGicWrite(HalpGicd, GICD_SGIR, (Targets << 16) | (IntId & 0xF));
        return;
    }

    for (Cpu = 0; TargetSet; Cpu++, TargetSet >>= 1)
    {
        if (!(TargetSet & 1))
            continue;

        Mpidr = HalpProcessorMpidr[Cpu];
        Value = ((Mpidr >> 32) & 0xFF) << 48;
        Value |= ((Mpidr >> 16) & 0xFF) << 32;
        Value |= ((ULONG64)IntId & 0xF) << 24;
        Value |= ((Mpidr >> 8) & 0xFF) << 16;
        Value |= 1ULL << (Mpidr & 0xF);
        KeArm64WriteRegister(ARM64_ICC_SGI1R_EL1, Value);
    }

    __isb(_ARM64_BARRIER_SY);
}

/**
 * @brief Acknowledges the highest priority pending interrupt.
 *
 * GICv2 wants the whole IAR value back at end of interrupt, the kernel ends
 * the interrupt before it enables them again so one value per processor does.
 */
ULONG
NTAPI
HalpGicAcknowledge(VOID)
{
    ULONG Iar;

    if (HalpGicV2)
    {
        Iar = HalpGicRead(HalpGicc, GICC_IAR);
        HalpGicLastIar[KeGetCurrentProcessorNumber()] = Iar;
        return Iar & GICC_IAR_INTID;
    }

    return (ULONG)KeArm64ReadRegister(ARM64_ICC_IAR1_EL1) & 0xFFFFFF;
}

VOID
NTAPI
HalpGicEndOfInterrupt(
    _In_ ULONG IntId)
{
    ULONG Iar;

    if (HalpGicV2)
    {
        Iar = HalpGicLastIar[KeGetCurrentProcessorNumber()];
        if ((Iar & GICC_IAR_INTID) != IntId)
            Iar = IntId;
        HalpGicWrite(HalpGicc, GICC_EOIR, Iar);
        return;
    }

    KeArm64WriteRegister(ARM64_ICC_EOIR1_EL1, IntId);
    __isb(_ARM64_BARRIER_SY);
}

VOID
NTAPI
HalpGicSetPriorityMask(
    _In_ UCHAR Priority)
{
    /* The GICv3 registers fault on a GICv2, so wait for the distributor to pick */
    if (!HalpGicReady)
        return;

    if (HalpGicV2)
        HalpGicWrite(HalpGicc, GICC_PMR, Priority);
    else
        KeArm64WriteRegister(ARM64_ICC_PMR_EL1, Priority);

    __dsb(_ARM64_BARRIER_SY);
    __isb(_ARM64_BARRIER_SY);
}
