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

BOOLEAN KdDebuggerEnabled = FALSE;
BOOLEAN KdEnteredDebugger = FALSE;
BOOLEAN KdDebuggerNotPresent = TRUE;
BOOLEAN KiEnableTimerWatchdog = FALSE;
BOOLEAN KdBreakAfterSymbolLoad = FALSE;
ULONG KiBugCheckData;
BOOLEAN KdpBreakPending;
VOID STDCALL PspDumpThreads(BOOLEAN SystemThreads);

typedef struct
{
	ULONG ComponentId;
	ULONG Level;
} KD_COMPONENT_DATA;
#define MAX_KD_COMPONENT_TABLE_ENTRIES 128
KD_COMPONENT_DATA KdComponentTable[MAX_KD_COMPONENT_TABLE_ENTRIES];
ULONG KdComponentTableEntries = 0;

/* PRIVATE FUNCTIONS *********************************************************/

ULONG
STDCALL
KdpServiceDispatcher(ULONG Service,
                     PVOID Buffer1,
                     ULONG Buffer1Length)
{
    ULONG Result = 0;

    switch (Service)
    {
        case BREAKPOINT_PRINT: /* DbgPrint */
            Result = KdpPrintString(Buffer1, Buffer1Length);
            break;

#ifdef DBG
        case TAG('R', 'o', 's', ' '): /* ROS-INTERNAL */
        {
            switch ((ULONG)Buffer1)
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
#endif
        default:
            HalDisplayString ("Invalid debug service call!\n");
            break;
    }

    return Result;
}

BOOLEAN
NTAPI
KdpEnterDebuggerException(IN PKTRAP_FRAME TrapFrame,
                          IN PKEXCEPTION_FRAME ExceptionFrame,
                          IN PEXCEPTION_RECORD ExceptionRecord,
                          IN PCONTEXT Context,
                          IN KPROCESSOR_MODE PreviousMode,
                          IN BOOLEAN SecondChance)
{
    KD_CONTINUE_TYPE Return;

    /* HACK (just like all this routine */
    if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) Context->Eip++;

    /* Get out of here if the Debugger isn't connected */
    if (KdDebuggerNotPresent) return FALSE;

    /* Call KDBG if available */
    Return = KdbEnterDebuggerException(ExceptionRecord,
                                       PreviousMode,
                                       Context,
                                       TrapFrame,
                                       !SecondChance);

    /* Convert return to BOOLEAN */
    if (Return == kdContinue) return TRUE;
    return FALSE;
}

BOOLEAN
NTAPI
KdpCallGdb(IN PKTRAP_FRAME TrapFrame,
           IN PEXCEPTION_RECORD ExceptionRecord,
           IN PCONTEXT Context)
{
    KD_CONTINUE_TYPE Return = kdDoNotHandleException;

    /* Get out of here if the Debugger isn't connected */
    if (KdDebuggerNotPresent) return FALSE;

    /* FIXME:
     * Right now, the GDB wrapper seems to handle exceptions differntly
     * from KDGB and both are called at different times, while the GDB
     * one is only called once and that's it. I don't really have the knowledge
     * to fix the GDB stub, so until then, we'll be using this hack
     */
    if (WrapperInitRoutine)
    {
        Return = WrapperTable.KdpExceptionRoutine(ExceptionRecord,
                                                  Context,
                                                  TrapFrame);
    }

    /* Convert return to BOOLEAN */
    if (Return == kdContinue) return TRUE;
    return FALSE;
}

/* PUBLIC FUNCTIONS *********************************************************/

/*
 * @implemented
 */
NTSTATUS
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

    /* Return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
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

    /* Return success */
    return STATUS_SUCCESS;
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

/*
 * @unimplemented
 */
VOID
STDCALL
KdChangeOption(IN KD_OPTION Option,
               IN ULONG InBufferLength OPTIONAL,
               IN PVOID InBuffer,
               IN ULONG OutBufferLength OPTIONAL,
               OUT PVOID OutBuffer,
               OUT PULONG OutBufferRequiredLength OPTIONAL)
{
    UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryDebugFilterState(IN ULONG ComponentId,
                        IN ULONG Level)
{
	unsigned int i;

	/* convert Level to mask if it isn't already one */
	if ( Level < 32 )
		Level = 1 << Level;

	for ( i = 0; i < KdComponentTableEntries; i++ )
	{
		if ( ComponentId == KdComponentTable[i].ComponentId )
		{
			if ( Level & KdComponentTable[i].Level )
				return TRUE;
			break;
		}
	}
	return FALSE;
}

NTSTATUS
STDCALL
NtSetDebugFilterState(IN ULONG ComponentId,
                      IN ULONG Level,
                      IN BOOLEAN State)
{
	unsigned int i;
	for ( i = 0; i < KdComponentTableEntries; i++ )
	{
		if ( ComponentId == KdComponentTable[i].ComponentId )
			break;
	}
	if ( i == KdComponentTableEntries )
	{
		if ( i == MAX_KD_COMPONENT_TABLE_ENTRIES )
			return STATUS_INVALID_PARAMETER_1;
		++KdComponentTableEntries;
		KdComponentTable[i].ComponentId = ComponentId;
		KdComponentTable[i].Level = 0;
	}
	if ( State )
		KdComponentTable[i].Level |= Level;
	else
		KdComponentTable[i].Level &= ~Level;
	return STATUS_SUCCESS;
}

PKDEBUG_ROUTINE KiDebugRoutine = KdpEnterDebuggerException;

 /* EOF */
