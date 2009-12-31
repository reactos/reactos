/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             hal/pic.c
 * PURPOSE:          Hardware Abstraction Layer DLL
 * PROGRAMMER:       Samuel Serapión
 */
#include <hal.h>

//#define NDEBUG
#include <debug.h>
#include <asm.h>

/*
 *  8259 interrupt controllers
 */
enum
{
     Int0ctl=        0x20,           /* control port (ICW1, OCW2, OCW3) */
     Int0aux=        0x21,           /* everything else (ICW2, ICW3, ICW4, OCW1) */
     Int1ctl=        0xA0,           /* control port */
     Int1aux=        0xA1,           /* everything else (ICW2, ICW3, ICW4, OCW1) */
 
     Icw1=           0x10,           /* select bit in ctl register */
     Ocw2=           0x00,
     Ocw3=           0x08,

     EOI=            0x20,           /* non-specific end of interrupt */
   
     Elcr1=          0x4D0,          /* Edge/Level Triggered Register */
     Elcr2=          0x4D1,
};



INT i8259mask = 0xFFFF;          /* disabled interrupts */
INT i8259elcr; 


VOID
NTAPI
HalpInitPICs(VOID)
{
    ULONG OldEflags;
    INT x;

    OldEflags = __readeflags();
    _disable();

    /*
     *  Set up the first 8259 interrupt processor.
     *  Make 8259 interrupts start at CPU vector VectorPIC.
     *  Set the 8259 as master with edge triggered
     *  input with fully nested interrupts.
     */
    __outbyte(Int0ctl, 0x20);                    /* ICW1 - master, edge triggered */
    __outbyte(Int0aux, 0x11);                    /* Edge, cascade, CAI 8, ICW4 */
    __outbyte(Int0aux, PRIMARY_VECTOR_BASE);     /* ICW2 - interrupt vector offset */
    __outbyte(Int0aux, 0x04);                    /* ICW3 - have slave on level 2 */
    __outbyte(Int0aux, 0x01);                    /* ICW4 - 8086 mode, not buffered */
    __outbyte(Int0aux, 0xFF);                    /* Mask Interrupts */
    /*
     *  Set up the second 8259 interrupt processor.
     *  Make 8259 interrupts start at CPU vector VectorPIC+8.
     *  Set the 8259 as slave with edge triggered
     *  input with fully nested interrupts.
     */
    __outbyte(Int1ctl, 0xA0);                    /* ICW1 - master, edge triggered, */
    __outbyte(Int1aux, 0x11);                    /* Edge, cascade, CAI 8, ICW4 */
    __outbyte(Int1aux, PRIMARY_VECTOR_BASE+8);   /* ICW2 - interrupt vector offset */
    __outbyte(Int1aux, 0x02);                    /* ICW3 - I am a slave on level 2 */
    __outbyte(Int1aux, 0x01);                    /* ICW4 - 8086 mode, not buffered */
    __outbyte(Int1aux, 0xFF);                    /* Mask Interrupts */


    /*
     *  pass #2 8259 interrupts to #1
     */
    i8259mask &= ~0x04;
    __outbyte(Int0aux, i8259mask & 0xFF);

    /*
     * Set Ocw3 to return the ISR when ctl read.
     * After initialisation status read is set to IRR.
     * Read IRR first to possibly deassert an outstanding
     * interrupt.
     */
    __inbyte (Int0ctl);
    __outbyte(Int0ctl, Ocw3|0x03);
    __inbyte (Int1ctl);
    __outbyte(Int1ctl, Ocw3|0x03);

    /*
     * Check for Edge/Level register.
     * This check may not work for all chipsets.
     * First try a non-intrusive test - the bits for
     * IRQs 13, 8, 2, 1 and 0 must be edge (0). If
     * that's OK try a R/W test.
     */
    x = (__inbyte(Elcr2) << 8) | __inbyte(Elcr1);

    if (!(x & 0x2107))
    {
        __outbyte(Elcr1, 0);

        if (__inbyte (Elcr1) == 0)
        {
            __outbyte(Elcr1, 0x20);

            if (__inbyte (Elcr1) == 0x20)
            {
                i8259elcr = x;
            }
            __outbyte(Elcr1, x & 0xFF);
            DPRINT("ELCR: %4.4uX\n", i8259elcr);
        }
    }

    __writeeflags(OldEflags);
}

VOID
NTAPI
HalDisableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql,
    KINTERRUPT_MODE InterruptMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

