/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/pic.c
 * PURPOSE:         HAL PIC Management and Control Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#undef KeGetCurrentIrql

/* GLOBALS ********************************************************************/

ULONG HalpIrqlTable[HIGH_LEVEL + 1] =
{
    0xFFFFFFFF, // IRQL 0 PASSIVE_LEVEL
    0xFFFFFFFD, // IRQL 1 APC_LEVEL
    0xFFFFFFF9, // IRQL 2 DISPATCH_LEVEL
    0xFFFFFFD9, // IRQL 3
    0xFFFFFF99, // IRQL 4
    0xFFFFFF19, // IRQL 5
    0xFFFFFE19, // IRQL 6
    0xFFFFFC19, // IRQL 7
    0xFFFFF819, // IRQL 8
    0xFFFFF019, // IRQL 9
    0xFFFFE019, // IRQL 10
    0xFFFFC019, // IRQL 11
    0xFFFF8019, // IRQL 12
    0xFFFF0019, // IRQL 13
    0xFFFE0019, // IRQL 14
    0xFFFC0019, // IRQL 15
    0xFFF80019, // IRQL 16
    0xFFF00019, // IRQL 17
    0xFFE00019, // IRQL 18
    0xFFC00019, // IRQL 19
    0xFF800019, // IRQL 20
    0xFF000019, // IRQL 21
    0xFE000019, // IRQL 22
    0xFC000019, // IRQL 23
    0xF0000019, // IRQL 24
    0x80000019, // IRQL 25
    0x19,       // IRQL 26
    0x18,       // IRQL 27 PROFILE_LEVEL
    0x10,       // IRQL 28 CLOCK2_LEVEL
    0x00,       // IRQL 29 IPI_LEVEL
    0x00,       // IRQL 30 POWER_LEVEL
    0x00,       // IRQL 31 HIGH_LEVEL
};

UCHAR HalpMaskTable[HIGH_LEVEL + 1] =
{
    PROFILE_LEVEL, // INT 0 WATCHDOG
    APC_LEVEL,     // INT 1 SOFTWARE INTERRUPT
    DISPATCH_LEVEL,// INT 2 COMM RX
    IPI_LEVEL,     // INT 3 COMM TX
    CLOCK2_LEVEL,  // INT 4 TIMER 0
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    26,
    26
};

/* FUNCTIONS ******************************************************************/

VOID
HalpInitializeInterrupts(VOID)
{
    PKPCR Pcr = KeGetPcr();

    /* Fill out the IRQL mappings */
    RtlCopyMemory(Pcr->IrqlTable, HalpIrqlTable, sizeof(Pcr->IrqlTable));
    RtlCopyMemory(Pcr->IrqlMask, HalpMaskTable, sizeof(Pcr->IrqlMask));
}

/* IRQL MANAGEMENT ************************************************************/

/*
 * @implemented
 */
ULONG
HalGetInterruptSource(VOID)
{
    ULONG InterruptStatus;

    /* Get the interrupt status, and return the highest bit set */
    InterruptStatus = READ_REGISTER_ULONG(VIC_INT_STATUS);
    return 31 - _clz(InterruptStatus);
}

/*
 * @implemented
 */
KIRQL
NTAPI
KeGetCurrentIrql(VOID)
{
    /* Return the IRQL */
    return KeGetPcr()->Irql;
}

/*
 * @implemented
 */
KIRQL
NTAPI
KeRaiseIrqlToDpcLevel(VOID)
{
    PKPCR Pcr = KeGetPcr();
    KIRQL CurrentIrql;

    /* Save and update IRQL */
    CurrentIrql = Pcr->Irql;
    Pcr->Irql = DISPATCH_LEVEL;

#ifdef IRQL_DEBUG
    /* Validate correct raise */
    if (CurrentIrql > DISPATCH_LEVEL) KeBugCheck(IRQL_NOT_GREATER_OR_EQUAL);
#endif

    /* Return the previous value */
    return CurrentIrql;
}

/*
 * @implemented
 */
KIRQL
NTAPI
KeRaiseIrqlToSynchLevel(VOID)
{
    PKPCR Pcr = KeGetPcr();
    KIRQL CurrentIrql;

    /* Save and update IRQL */
    CurrentIrql = Pcr->Irql;
    Pcr->Irql = SYNCH_LEVEL;

#ifdef IRQL_DEBUG
    /* Validate correct raise */
    if (CurrentIrql > SYNCH_LEVEL)
    {
        /* Crash system */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     CurrentIrql,
                     SYNCH_LEVEL,
                     0,
                     1);
    }
#endif

    /* Return the previous value */
    return CurrentIrql;
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KfRaiseIrql(IN KIRQL NewIrql)
{
    ARM_STATUS_REGISTER Flags;
    PKPCR Pcr = KeGetPcr();
    KIRQL CurrentIrql;
    ULONG InterruptMask;

    /* Disable interrupts */
    Flags = KeArmStatusRegisterGet();
    _disable();

    /* Read current IRQL */
    CurrentIrql = Pcr->Irql;

#ifdef IRQL_DEBUG
    /* Validate correct raise */
    if (CurrentIrql > NewIrql)
    {
        /* Crash system */
        Pcr->Irql = PASSIVE_LEVEL;
        KeBugCheck(IRQL_NOT_GREATER_OR_EQUAL);
    }
#endif
    /* Clear interrupts associated to the old IRQL */
    WRITE_REGISTER_ULONG(VIC_INT_CLEAR, 0xFFFFFFFF);

    /* Set the new interrupt mask */
    InterruptMask = Pcr->IrqlTable[NewIrql];
    WRITE_REGISTER_ULONG(VIC_INT_ENABLE, InterruptMask);

    /* Set new IRQL */
    Pcr->Irql = NewIrql;

    /* Restore interrupt state */
    if (!Flags.IrqDisable) _enable();

    /* Return old IRQL */
    return CurrentIrql;
}

/*
 * @implemented
 */
VOID
FASTCALL
KfLowerIrql(IN KIRQL NewIrql)
{
    ARM_STATUS_REGISTER Flags;
    PKPCR Pcr = KeGetPcr();
    ULONG InterruptMask;

    /* Disableinterrupts */
    Flags = KeArmStatusRegisterGet();
    _disable();

#ifdef IRQL_DEBUG
    /* Validate correct lower */
    if (OldIrql > Pcr->Irql)
    {
        /* Crash system */
        Pcr->Irql = HIGH_LEVEL;
        KeBugCheck(IRQL_NOT_LESS_OR_EQUAL);
    }
#endif

    /* Clear interrupts associated to the old IRQL */
    WRITE_REGISTER_ULONG(VIC_INT_CLEAR, 0xFFFFFFFF);

    /* Set the new interrupt mask */
    InterruptMask = Pcr->IrqlTable[NewIrql];
    WRITE_REGISTER_ULONG(VIC_INT_ENABLE, InterruptMask);

    /* Save the new IRQL and restore interrupt state */
    Pcr->Irql = NewIrql;
    if (!Flags.IrqDisable) _enable();
}

/* SOFTWARE INTERRUPTS ********************************************************/

/*
 * @implemented
 */
VOID
FASTCALL
HalRequestSoftwareInterrupt(IN KIRQL Irql)
{
    /* Force a software interrupt */
    WRITE_REGISTER_ULONG(VIC_SOFT_INT, 1 << Irql);
}

/*
 * @implemented
 */
VOID
FASTCALL
HalClearSoftwareInterrupt(IN KIRQL Irql)
{
    /* Clear software interrupt */
    WRITE_REGISTER_ULONG(VIC_SOFT_INT_CLEAR, 1 << Irql);
}

/* SYSTEM INTERRUPTS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalEnableSystemInterrupt(IN ULONG Vector,
                         IN KIRQL Irql,
                         IN KINTERRUPT_MODE InterruptMode)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
HalDisableSystemInterrupt(IN ULONG Vector,
                          IN KIRQL Irql)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalBeginSystemInterrupt(IN KIRQL Irql,
                        IN ULONG Vector,
                        OUT PKIRQL OldIrql)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
HalEndSystemInterrupt(IN KIRQL OldIrql,
                      IN PKTRAP_FRAME TrapFrame)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
