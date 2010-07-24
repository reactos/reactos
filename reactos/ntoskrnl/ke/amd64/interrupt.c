/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/irq.c
 * PURPOSE:         Manages the Kernel's IRQ support for external drivers,
 *                  for the purpopses of connecting, disconnecting and setting
 *                  up ISRs for drivers. The backend behind the Io* Interrupt
 *                  routines.
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@web.de)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/



BOOLEAN
NTAPI
KeDisableInterrupts(VOID)
{
    ULONG64 Flags;

    /* Get the flags */
    Flags = __readeflags();

    /* Disable interrupts */
    _disable();

    return !!(Flags & EFLAGS_INTERRUPT_MASK);
}


BOOLEAN
NTAPI
KeDisconnectInterrupt(IN PKINTERRUPT Interrupt)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
KeInitializeInterrupt(IN PKINTERRUPT Interrupt,
                      IN PKSERVICE_ROUTINE ServiceRoutine,
                      IN PVOID ServiceContext,
                      IN PKSPIN_LOCK SpinLock,
                      IN ULONG Vector,
                      IN KIRQL Irql,
                      IN KIRQL SynchronizeIrql,
                      IN KINTERRUPT_MODE InterruptMode,
                      IN BOOLEAN ShareVector,
                      IN CHAR ProcessorNumber,
                      IN BOOLEAN FloatingSave)
{
    UNIMPLEMENTED;
}

