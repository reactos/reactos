/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/freeze.c
 * PURPOSE:         Routines for freezing and unfreezing processors for
 *                  kernel debugger synchronization.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* Freeze data */
KIRQL KiOldIrql;
ULONG KiFreezeFlag;
static KIRQL KiFreezeOldIrql[MAXIMUM_PROCESSORS];
static BOOLEAN KiFreezeInterruptEnable[MAXIMUM_PROCESSORS];
static BOOLEAN KiFreezeArchitectureOwner[MAXIMUM_PROCESSORS];
static volatile LONG KiFreezeDepth[MAXIMUM_PROCESSORS];

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
KeFreezeExecution(IN PKTRAP_FRAME TrapFrame,
                  IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Enable;
    BOOLEAN ArchitectureOwner;
    ULONG Processor;
    KIRQL OldIrql;

#ifndef CONFIG_SMP
    UNREFERENCED_PARAMETER(TrapFrame);
    UNREFERENCED_PARAMETER(ExceptionFrame);
#endif

    /* Disable interrupts, get previous state and set the freeze flag */
    Enable = KeDisableInterrupts();
    Processor = KeGetCurrentProcessorNumber();
    ASSERT(Processor < MAXIMUM_PROCESSORS);

    if (KiFreezeDepth[Processor] != 0)
    {
        ++KiFreezeDepth[Processor];
        return KiFreezeInterruptEnable[Processor];
    }

#ifndef CONFIG_SMP
    KiFreezeFlag = 4;

    /* Raise IRQL if we have to */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < DISPATCH_LEVEL)
        OldIrql = KeRaiseIrqlToDpcLevel();
    ArchitectureOwner = TRUE;
#else
    /* Raise IRQL to HIGH_LEVEL */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    KiFreezeOldIrql[Processor] = OldIrql;
    KiFreezeInterruptEnable[Processor] = Enable;
    KiFreezeArchitectureOwner[Processor] = FALSE;
    KiFreezeDepth[Processor] = 1;

    ArchitectureOwner = KxFreezeExecution();
    if (ArchitectureOwner)
        KiFreezeFlag |= 4;
    else
        KiFreezeFlag |= 2;
#endif

    /* Save the old IRQL to be restored on unfreeze */
    KiOldIrql = OldIrql;
    KiFreezeOldIrql[Processor] = OldIrql;
    KiFreezeInterruptEnable[Processor] = Enable;
    KiFreezeArchitectureOwner[Processor] = ArchitectureOwner;
    KiFreezeDepth[Processor] = 1;

    /* Return whether interrupts were enabled */
    return Enable;
}

VOID
NTAPI
KeThawExecution(IN BOOLEAN Enable)
{
    BOOLEAN ArchitectureOwner;
    BOOLEAN SavedEnable;
    ULONG Processor;
    KIRQL OldIrql;

    Processor = KeGetCurrentProcessorNumber();
    ASSERT(Processor < MAXIMUM_PROCESSORS);

    if (KiFreezeDepth[Processor] == 0)
        return;

    if (--KiFreezeDepth[Processor] != 0)
        return;

    OldIrql = KiFreezeOldIrql[Processor];
    SavedEnable = KiFreezeInterruptEnable[Processor];
    ArchitectureOwner = KiFreezeArchitectureOwner[Processor];
    KiFreezeArchitectureOwner[Processor] = FALSE;
    UNREFERENCED_PARAMETER(Enable);

    if (ArchitectureOwner)
    {
#ifdef CONFIG_SMP
        /* Architecture specific thaw code */
        KxThawExecution();
#endif

        /* Clear the freeze flag */
        KiFreezeFlag = 0;
    }

    /* Cleanup CPU caches */
    KeFlushCurrentTb();

    /* Restore the old IRQL */
#ifndef CONFIG_SMP
    if (OldIrql < DISPATCH_LEVEL)
#endif
    KeLowerIrql(OldIrql);

    /* Re-enable interrupts */
    KeRestoreInterrupts(SavedEnable);
}
