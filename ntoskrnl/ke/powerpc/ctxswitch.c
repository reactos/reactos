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
STDCALL
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame)
{
    KeLowerIrql(APC_LEVEL);
    __asm__("mr 8,%0\n\t"
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
VOID
STDCALL
KiDecrementerTrap(PKTRAP_FRAME TrapFrame)
{
    DbgPrint("Decrementer Trap!\n");
    __asm__("mtdec %0" : : "r" (0x10000)); // Reset the trap
}
