/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/generic/pic.c
 * PURPOSE:         HAL PIC Management and Control Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

USHORT HalpEisaELCR;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
HalpInitializePICs(IN BOOLEAN EnableInterrupts)
{
    ULONG EFlags;
    I8259_ICW1 Icw1;
    I8259_ICW2 Icw2;
    I8259_ICW3 Icw3;
    I8259_ICW4 Icw4;
    EISA_ELCR Elcr;
    ULONG i, j;
    
    /* Save EFlags and disable interrupts */
    EFlags = __readeflags();
    _disable();
    
    /* Initialize ICW1 for master, interval 8, edge-triggered mode with ICW4 */
    Icw1.NeedIcw4 = TRUE;
    Icw1.InterruptMode = EdgeTriggered;
    Icw1.OperatingMode = Cascade;
    Icw1.Interval = Interval8;
    Icw1.Init = TRUE;
    Icw1.InterruptVectorAddress = 0; /* This is only used in MCS80/85 mode */
    __outbyte(PIC1_CONTROL_PORT, Icw1.Bits);
    
    /* Set interrupt vector base */
    Icw2.Bits = PRIMARY_VECTOR_BASE;
    __outbyte(PIC1_DATA_PORT, Icw2.Bits);
    
    /* Connect slave to IRQ 2 */
    Icw3.Bits = 0;
    Icw3.SlaveIrq2 = TRUE;
    __outbyte(PIC1_DATA_PORT, Icw3.Bits);
    
    /* Enable 8086 mode, non-automatic EOI, non-buffered mode, non special fully nested mode */
    Icw4.Reserved = 0;
    Icw4.SystemMode = New8086Mode;
    Icw4.EoiMode = NormalEoi;
    Icw4.BufferedMode = NonBuffered;
    Icw4.SpecialFullyNestedMode = FALSE;
    __outbyte(PIC1_DATA_PORT, Icw4.Bits);
    
    /* Mask all interrupts */
    __outbyte(PIC1_DATA_PORT, 0xFF);
    
    /* Initialize ICW1 for master, interval 8, edge-triggered mode with ICW4 */
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
    Icw4.Reserved = 0;
    Icw4.SystemMode = New8086Mode;
    Icw4.EoiMode = NormalEoi;
    Icw4.BufferedMode = NonBuffered;
    Icw4.SpecialFullyNestedMode = FALSE;
    __outbyte(PIC2_DATA_PORT, Icw4.Bits);
    
    /* Mask all interrupts */
    __outbyte(PIC2_DATA_PORT, 0xFF);
    
    /* Read EISA Edge/Level Register for master and slave */
    Elcr.Bits = (__inbyte(EISA_ELCR_SLAVE) << 8) | __inbyte(EISA_ELCR_MASTER);
    
    /* IRQs 0, 1, 2, 8, and 13 are system-reserved and must be edge */
    if (!(Elcr.Master.Irq0Level) && !(Elcr.Master.Irq1Level) && !(Elcr.Master.Irq2Level) &&
        !(Elcr.Slave.Irq8Level) && !(Elcr.Slave.Irq13Level))
    {
        /* ELCR is as it's supposed to be, save it */
        HalpEisaELCR = Elcr.Bits;
        DPRINT1("HAL Detected EISA Interrupt Controller (ELCR: %lx)\n", HalpEisaELCR);
        
        /* Scan for level interrupts */
        for (i = 1, j = 0; j < 16; i <<= 1, j++)
        {
            /* Warn the user ReactOS does not (and has never) supported this */
            if (HalpEisaELCR & i) DPRINT1("WARNING: IRQ %d is SHARED and LEVEL-SENSITIVE. This is unsupported!\n", j);
        }
    }
    
    /* Restore interrupt state */
    if (EnableInterrupts) EFlags |= EFLAGS_INTERRUPT_MASK;
    __writeeflags(EFlags);
}

/* IRQL MANAGEMENT ************************************************************/

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
    if (CurrentIrql > DISPATCH_LEVEL)
    {
        /* Crash system */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     CurrentIrql,
                     DISPATCH_LEVEL,
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


/* SOFTWARE INTERRUPTS ********************************************************/

/*
 * @implemented
 */
VOID
FASTCALL
HalClearSoftwareInterrupt(IN KIRQL Irql)
{
    /* Mask out the requested bit */
    KeGetPcr()->IRR &= ~(1 << Irql);
}
