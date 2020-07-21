/*
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         IRQL mapping
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

/* GLOBALS ********************************************************************/

/* This table contains the static x86 PIC mapping between IRQLs and IRQs */
ULONG KiI8259MaskTable[32] =
{
    /*
     * It Device IRQLs only start at 4 or higher, so these are just software
     * IRQLs that don't really change anything on the hardware
     */
    0b00000000000000000000000000000000, /* IRQL 0 */
    0b00000000000000000000000000000000, /* IRQL 1 */
    0b00000000000000000000000000000000, /* IRQL 2 */
    0b00000000000000000000000000000000, /* IRQL 3 */

    /*
     * These next IRQLs are actually useless from the PIC perspective, because
     * with only 2 PICs, the mask you can send them is only 8 bits each, for 16
     * bits total, so these IRQLs are masking off a phantom PIC.
     */
    0b11111111100000000000000000000000, /* IRQL 4 */
    0b11111111110000000000000000000000, /* IRQL 5 */
    0b11111111111000000000000000000000, /* IRQL 6 */
    0b11111111111100000000000000000000, /* IRQL 7 */
    0b11111111111110000000000000000000, /* IRQL 8 */
    0b11111111111111000000000000000000, /* IRQL 9 */
    0b11111111111111100000000000000000, /* IRQL 10 */
    0b11111111111111110000000000000000, /* IRQL 11 */

    /*
     * Okay, now we're finally starting to mask off IRQs on the slave PIC, from
     * IRQ15 to IRQ8. This means the higher-level IRQs get less priority in the
     * IRQL sense.
     */
    0b11111111111111111000000000000000, /* IRQL 12 */
    0b11111111111111111100000000000000, /* IRQL 13 */
    0b11111111111111111110000000000000, /* IRQL 14 */
    0b11111111111111111111000000000000, /* IRQL 15 */
    0b11111111111111111111100000000000, /* IRQL 16 */
    0b11111111111111111111110000000000, /* IRQL 17 */
    0b11111111111111111111111000000000, /* IRQL 18 */
    0b11111111111111111111111000000000, /* IRQL 19 */

    /*
     * Now we mask off the IRQs on the master. Notice the 0 "droplet"? You might
     * have also seen that IRQL 18 and 19 are essentially equal as far as the
     * PIC is concerned. That bit is actually IRQ8, which happens to be the RTC.
     * The RTC will keep firing as long as we don't reach PROFILE_LEVEL which
     * actually kills it. The RTC clock (unlike the system clock) is used by the
     * profiling APIs in the HAL, so that explains the logic.
     */
    0b11111111111111111111111010000000, /* IRQL 20 */
    0b11111111111111111111111011000000, /* IRQL 21 */
    0b11111111111111111111111011100000, /* IRQL 22 */
    0b11111111111111111111111011110000, /* IRQL 23 */
    0b11111111111111111111111011111000, /* IRQL 24 */
    0b11111111111111111111111011111000, /* IRQL 25 */
    0b11111111111111111111111011111010, /* IRQL 26 */
    0b11111111111111111111111111111010, /* IRQL 27 */

    /*
     * IRQL 24 and 25 are actually identical, so IRQL 28 is actually the last
     * IRQL to modify a bit on the master PIC. It happens to modify the very
     * last of the IRQs, IRQ0, which corresponds to the system clock interval
     * timer that keeps track of time (the Windows heartbeat). We only want to
     * turn this off at a high-enough IRQL, which is why IRQLs 24 and 25 are the
     * same to give this guy a chance to come up higher. Note that IRQL 28 is
     * called CLOCK2_LEVEL, which explains the usage we just explained.
     */
    0b11111111111111111111111111111011, /* IRQL 28 */

    /*
     * We have finished off with the PIC so there's nothing left to mask at the
     * level of these IRQLs, making them only logical IRQLs on x86 machines.
     * Note that we have another 0 "droplet" you might've caught since IRQL 26.
     * In this case, it's the 2nd bit that never gets turned off, which is IRQ2,
     * the cascade IRQ that we use to bridge the slave PIC with the master PIC.
     * We never want to turn it off, so no matter the IRQL, it will be set to 0.
     */
    0b11111111111111111111111111111011, /* IRQL 29 */
    0b11111111111111111111111111111011, /* IRQL 30 */
    0b11111111111111111111111111111011  /* IRQL 31 */
};

/* This table indicates which IRQs, if pending, can preempt a given IRQL level */
ULONG FindHigherIrqlMask[32] =
{
    /*
     * Software IRQLs, at these levels all hardware interrupts can preempt.
     * Each higher IRQL simply enables which software IRQL can preempt the
     * current level.
     */
    0b11111111111111111111111111111110, /* IRQL 0 */
    0b11111111111111111111111111111100, /* IRQL 1 */
    0b11111111111111111111111111111000, /* IRQL 2 */

    /*
     * IRQL3 means only hardware IRQLs can now preempt. These last 4 zeros will
     * then continue throughout the rest of the list, trickling down.
     */
    0b11111111111111111111111111110000, /* IRQL 3 */

    /*
     * Just like in the previous list, these masks don't really mean anything
     * since we've only got two PICs with 16 possible IRQs total
     */
    0b00000111111111111111111111110000, /* IRQL 4 */
    0b00000011111111111111111111110000, /* IRQL 5 */
    0b00000001111111111111111111110000, /* IRQL 6 */
    0b00000000111111111111111111110000, /* IRQL 7 */
    0b00000000011111111111111111110000, /* IRQL 8 */
    0b00000000001111111111111111110000, /* IRQL 9 */
    0b00000000000111111111111111110000, /* IRQL 10 */

    /*
     * Now we start progressivly limiting which slave PIC interrupts have the
     * right to preempt us at each level.
     */
    0b00000000000011111111111111110000, /* IRQL 11 */
    0b00000000000001111111111111110000, /* IRQL 12 */
    0b00000000000000111111111111110000, /* IRQL 13 */
    0b00000000000000011111111111110000, /* IRQL 14 */
    0b00000000000000001111111111110000, /* IRQL 15 */
    0b00000000000000000111111111110000, /* IRQL 16 */
    0b00000000000000000011111111110000, /* IRQL 17 */
    0b00000000000000000001111111110000, /* IRQL 18 */
    0b00000000000000000001111111110000, /* IRQL 19 */

    /*
     * Also recall from the earlier table that IRQL 18/19 are treated the same
     * in order to spread the masks better thoughout the 32 IRQLs and to reflect
     * the fact that some bits will always stay on until much higher IRQLs since
     * they are system-critical. One such example is the 1 bit that you start to
     * see trickling down here. This is IRQ8, the RTC timer used for profiling,
     * so it will always preempt until we reach PROFILE_LEVEL.
     */
    0b00000000000000000001011111110000, /* IRQL 20 */
    0b00000000000000000001001111110000, /* IRQL 21 */
    0b00000000000000000001000111110000, /* IRQL 22 */
    0b00000000000000000001000011110000, /* IRQL 23 */
    0b00000000000000000001000001110000, /* IRQL 24 */
    0b00000000000000000001000000110000, /* IRQL 25 */
    0b00000000000000000001000000010000, /* IRQL 26 */

    /* At this point, only the clock (IRQ0) can still preempt... */
    0b00000000000000000000000000010000, /* IRQL 27 */

    /* And any higher than that there's no relation with hardware PICs anymore */
    0b00000000000000000000000000000000, /* IRQL 28 */
    0b00000000000000000000000000000000, /* IRQL 29 */
    0b00000000000000000000000000000000, /* IRQL 30 */
    0b00000000000000000000000000000000  /* IRQL 31 */
};
