/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/kd/kdinit.c
 * PURPOSE:         Kernel Debugger Initializtion
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* VARIABLES ***************************************************************/

BOOLEAN EXPORTED KdDebuggerEnabled = FALSE;
BOOLEAN EXPORTED KdEnteredDebugger = FALSE;
BOOLEAN EXPORTED KdDebuggerNotPresent = TRUE;
BOOLEAN EXPORTED KiEnableTimerWatchdog = FALSE;
ULONG EXPORTED KiBugCheckData;
BOOLEAN KdpBreakPending;
VOID STDCALL PspDumpThreads(BOOLEAN SystemThreads);

/* PRIVATE FUNCTIONS *********************************************************/

ULONG
STDCALL
KdpServiceDispatcher(ULONG Service,
                     PVOID Context1,
                     PVOID Context2)
{
    ULONG Result = 0;

    switch (Service)
    {
        case 1: /* DbgPrint */
            Result = KdpPrintString ((PANSI_STRING)Context1);
            break;

        case TAG('R', 'o', 's', ' '): /* ROS-INTERNAL */
        {
            switch ((ULONG)Context1)
            {
                case DumpNonPagedPool:
                    MiDebugDumpNonPagedPool(FALSE);
                    break;

                case ManualBugCheck:
                    KEBUGCHECK(MANUALLY_INITIATED_CRASH);
                    break;

                case DumpNonPagedPoolStats:
                    MiDebugDumpNonPagedPoolStats(FALSE);
                    break;

                case DumpNewNonPagedPool:
                    MiDebugDumpNonPagedPool(TRUE);
                    break;

                case DumpNewNonPagedPoolStats:
                    MiDebugDumpNonPagedPoolStats(TRUE);
                    break;

                case DumpAllThreads:
                    PspDumpThreads(TRUE);
                    break;

                case DumpUserThreads:
                    PspDumpThreads(FALSE);
                    break;

                case EnterDebugger:
                    DbgBreakPoint();
                    break;

                default:
                    break;
            }
        }

        default:
            HalDisplayString ("Invalid debug service call!\n");
            break;
    }

    return Result;
}

KD_CONTINUE_TYPE
STDCALL
KdpEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
                          KPROCESSOR_MODE PreviousMode,
                          PCONTEXT Context,
                          PKTRAP_FRAME TrapFrame,
                          BOOLEAN FirstChance,
                          BOOLEAN Gdb)
{
    /* Get out of here if the Debugger isn't enabled */
    if (!KdDebuggerEnabled) return kdHandleException;

    /* FIXME:
     * Right now, the GDB wrapper seems to handle exceptions differntly
     * from KDGB and both are called at different times, while the GDB
     * one is only called once and that's it. I don't really have the knowledge
     * to fix the GDB stub, so until then, we'll be using this hack
     */
    if (Gdb)
    {
        /* Call the registered wrapper */
        if (WrapperInitRoutine) return WrapperTable.
                                       KdpExceptionRoutine(ExceptionRecord,
                                                           Context,
                                                           TrapFrame);
    }

    /* Call KDBG if available */
    return KdbEnterDebuggerException(ExceptionRecord,
                                     PreviousMode,
                                     Context,
                                     TrapFrame,
                                     FirstChance);
}

/* PUBLIC FUNCTIONS *********************************************************/

/*
 * @implemented
 */
VOID
STDCALL
KdDisableDebugger(VOID)
{
    KIRQL OldIrql;

    /* Raise IRQL */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* TODO: Disable any breakpoints */

    /* Disable the Debugger */
    KdDebuggerEnabled = FALSE;

    /* Lower the IRQL */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
STDCALL
KdEnableDebugger(VOID)
{
    KIRQL OldIrql;

    /* Raise IRQL */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* TODO: Re-enable any breakpoints */

    /* Enable the Debugger */
    KdDebuggerEnabled = TRUE;

    /* Lower the IRQL */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KdPollBreakIn(VOID)
{
    return KdpBreakPending;
}

/*
 * @implemented
 */
VOID
STDCALL
KeEnterKernelDebugger(VOID)
{
    HalDisplayString("\n\n *** Entered kernel debugger ***\n");

    /* Set the Variable */
    KdEnteredDebugger = TRUE;

    /* Halt the CPU */
    for (;;) Ke386HaltProcessor();
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
KdPowerTransition(ULONG PowerState)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

 /* EOF */
