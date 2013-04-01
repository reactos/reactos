/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 * 
 * PROGRAMMERS:     arty
                    (i386 implementation by Alex Ionescu)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <ppcmmu/mmu.h>

/*++
 * KiThreadStartup
 *
 *     The KiThreadStartup routine is the beginning of any thread.
 *
 * Params:
 *     SystemRoutine - Pointer to the System Startup Routine. Either 
 *                     PspUserThreadStartup or PspSystemThreadStartup
 *
 *     StartRoutine - For Kernel Threads only, specifies the starting execution
 *                    point of the new thread.
 *
 *     StartContext - For Kernel Threads only, specifies a pointer to variable
 *                    context data to be sent to the StartRoutine above.
 *
 *     UserThread - Indicates whether or not this is a user thread. This tells
 *                  us if the thread has a context or not.
 *
 *     TrapFrame - Pointer to the KTHREAD to which the caller wishes to
 *           switch from.
 *
 * Returns:
 *     Should never return for a system thread. Returns through the System Call
 *     Exit Dispatcher for a user thread.
 *
 * Remarks:
 *     If a return from a system thread is detected, a bug check will occur.
 *
 *--*/

VOID
NTAPI
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame)
{
    KeLowerIrql(APC_LEVEL);
    __asm__("mr 0,%0\n\t"
            "mr 3,%1\n\t"
            "mr 4,%2\n\t"
            "mr 5,%3\n\t"
            "mr 6,%4\n\t"
            "sc" : : 
            "r" (0xf0000), /* Thread start function */
            "r" (SystemRoutine),
            "r" (StartRoutine),
            "r" (StartContext),
            "r" (UserThread));
    PspTerminateThreadByPointer(PsGetCurrentThread(), STATUS_THREAD_IS_TERMINATING, TRUE);
}

/* Take a decrementer trap, and prepare the given trap frame, swapping 
 * process and thread context as appropriate. */
VOID KiDecrementerTrapFinish(PKTRAP_FRAME TrapFrame);

VOID
FASTCALL
KiQueueReadyThread(IN PKTHREAD Thread,
                   IN PKPRCB Prcb);

PKTHREAD KiLastThread = NULL;
PKTRAP_FRAME KiLastThreadTrapFrame = NULL;

VOID
NTAPI
KiDecrementerTrap(PKTRAP_FRAME TrapFrame)
{
    KIRQL Irql;
    PKPRCB Prcb = KeGetPcr()->Prcb;
    if (!KiLastThread)
        KiLastThread = KeGetCurrentThread();

    if (KiLastThread->State == Running)
        KiQueueReadyThread(KiLastThread, Prcb);

    if (!KiLastThreadTrapFrame)
        KiLastThreadTrapFrame = Prcb->IdleThread->TrapFrame;

    TrapFrame->OldIrql = KeGetCurrentIrql();
    *KiLastThreadTrapFrame = *TrapFrame;

    if (Prcb->NextThread)
    {
        Prcb->CurrentThread = Prcb->NextThread;
        Prcb->NextThread = NULL;
    }
    else
        Prcb->CurrentThread = Prcb->IdleThread;

    Prcb->CurrentThread->State = Running;

    KiLastThreadTrapFrame = Prcb->CurrentThread->TrapFrame;
    KiLastThread = Prcb->CurrentThread;

    *TrapFrame = *KiLastThreadTrapFrame;
    Irql = KeGetCurrentIrql();

    if (Irql > TrapFrame->OldIrql)
        KfRaiseIrql(Irql);
    else if (Irql < TrapFrame->OldIrql)
        KfLowerIrql(Irql);

    /* When we return, we'll go through rfi and be in new thread land */
    __asm__("mtdec %0" : : "r" (0x1000000)); // Reset the trap
}
