/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PIC initialization
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* PRIVATE FUNCTIONS *********************************************************/

static VOID
HalpIoWait(VOID)
{
    UCHAR i;

    /*
     * Give the old PICs enough time to react to commands.
     * (KeStallExecutionProcessor is not available at this stage)
     */
    for (i = 0; i < 6; i++)
        __outbyte(CPU_IO_o_ARTIC_DELAY, 0);
}

VOID
NTAPI
HalpInitializeLegacyPICs(VOID)
{
    I8259_ICW1 Icw1;
    I8259_ICW2 Icw2;
    I8259_ICW3 Icw3;
    I8259_ICW4 Icw4;

    ASSERT(!(__readeflags() & EFLAGS_INTERRUPT_MASK));

    /* Initialize ICW1 for master, interval 8, edge-triggered mode with ICW4 */
    Icw1.NeedIcw4 = TRUE;
    Icw1.OperatingMode = Cascade;
    Icw1.Interval = Interval8;
    Icw1.InterruptMode = EdgeTriggered;
    Icw1.Init = TRUE;
    Icw1.InterruptVectorAddress = 0;
    __outbyte(PIC1_CONTROL_PORT, Icw1.Bits);
    HalpIoWait();

    /* ICW2 - interrupt vector offset */
    Icw2.Bits = PRIMARY_VECTOR_BASE;
    __outbyte(PIC1_DATA_PORT, Icw2.Bits);
    HalpIoWait();

    /* Connect slave to cascade IRQ */
    Icw3.Bits = 0;
    Icw3.SlaveIrq7 = TRUE;
    __outbyte(PIC1_DATA_PORT, Icw3.Bits);
    HalpIoWait();

    /* Enable 8086 mode, non-automatic EOI, buffered mode, special fully nested mode */
    Icw4.SystemMode = New8086Mode;
    Icw4.EoiMode = NormalEoi;
    Icw4.BufferedMode = BufferedMaster;
    Icw4.SpecialFullyNestedMode = TRUE;
    Icw4.Reserved = 0;
    __outbyte(PIC1_DATA_PORT, Icw4.Bits);
    HalpIoWait();

    /* Mask all interrupts */
    __outbyte(PIC1_DATA_PORT, 0xFF);
    HalpIoWait();

    /* Initialize ICW1 for slave, interval 8, edge-triggered mode with ICW4 */
    Icw1.NeedIcw4 = TRUE;
    Icw1.InterruptMode = EdgeTriggered;
    Icw1.OperatingMode = Cascade;
    Icw1.Interval = Interval8;
    Icw1.Init = TRUE;
    Icw1.InterruptVectorAddress = 0; /* This is only used in MCS80/85 mode */
    __outbyte(PIC2_CONTROL_PORT, Icw1.Bits);
    HalpIoWait();

    /* Set interrupt vector base */
    Icw2.Bits = PRIMARY_VECTOR_BASE + 8;
    __outbyte(PIC2_DATA_PORT, Icw2.Bits);
    HalpIoWait();

    /* Slave ID */
    Icw3.Bits = 0;
    Icw3.SlaveId = PIC_CASCADE_IRQ;
    __outbyte(PIC2_DATA_PORT, Icw3.Bits);
    HalpIoWait();

    /* Enable 8086 mode, non-automatic EOI, buffered mode, non special fully nested mode */
    Icw4.SystemMode = New8086Mode;
    Icw4.EoiMode = NormalEoi;
    Icw4.BufferedMode = BufferedSlave;
    Icw4.SpecialFullyNestedMode = FALSE;
    Icw4.Reserved = 0;
    __outbyte(PIC2_DATA_PORT, Icw4.Bits);
    HalpIoWait();

    /* Mask all interrupts */
    __outbyte(PIC2_DATA_PORT, 0xFF);
    HalpIoWait();
}
