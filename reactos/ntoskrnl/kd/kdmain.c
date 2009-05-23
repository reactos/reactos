/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/kd/kdinit.c
 * PURPOSE:         Kernel Debugger Initialization
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* VARIABLES ***************************************************************/

BOOLEAN KdDebuggerEnabled = FALSE;
BOOLEAN KdEnteredDebugger = FALSE;
BOOLEAN KdDebuggerNotPresent = TRUE;
BOOLEAN KiEnableTimerWatchdog = FALSE;
BOOLEAN KdBreakAfterSymbolLoad = FALSE;
BOOLEAN KdpBreakPending;
BOOLEAN KdPitchDebugger = TRUE;
VOID NTAPI PspDumpThreads(BOOLEAN SystemThreads);

typedef struct
{
	ULONG ComponentId;
	ULONG Level;
} KD_COMPONENT_DATA;
#define MAX_KD_COMPONENT_TABLE_ENTRIES 128
KD_COMPONENT_DATA KdComponentTable[MAX_KD_COMPONENT_TABLE_ENTRIES];
ULONG KdComponentTableEntries = 0;

ULONG Kd_DEFAULT_MASK = 1 << DPFLTR_ERROR_LEVEL;

/* PRIVATE FUNCTIONS *********************************************************/

ULONG
NTAPI
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
                    KeBugCheck(MANUALLY_INITIATED_CRASH);
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
            break;
        }

        /* Special  case for stack frame dumps */
        case TAG('R', 'o', 's', 'D'):
        {
            KeRosDumpStackFrames((PULONG)Buffer1, Buffer1Length);
            break;
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
    ULONG ExceptionCommand = ExceptionRecord->ExceptionInformation[0];
#ifdef _M_IX86
    ULONG EipOld;
#endif

    /* Check if this was a breakpoint due to DbgPrint or Load/UnloadSymbols */
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionCommand == BREAKPOINT_LOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_UNLOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_COMMAND_STRING) ||
         (ExceptionCommand == BREAKPOINT_PRINT)))
    {
        /* Check if this is a debug print */
        if (ExceptionCommand == BREAKPOINT_PRINT)
        {
            /* Print the string */
            KdpServiceDispatcher(BREAKPOINT_PRINT,
                                 (PVOID)ExceptionRecord->ExceptionInformation[1],
                                 ExceptionRecord->ExceptionInformation[2]);
            Context->Eax = STATUS_SUCCESS;
        }
        else if (ExceptionCommand == BREAKPOINT_LOAD_SYMBOLS)
        {
            /* Load symbols. Currently implemented only for KDBG! */
            KDB_SYMBOLFILE_HOOK((PANSI_STRING)ExceptionRecord->ExceptionInformation[1],
                (PKD_SYMBOLS_INFO)ExceptionRecord->ExceptionInformation[2]);
        }

        /* This we can handle: simply bump EIP */
#ifdef _M_IX86
        Context->Eip++;
#elif _M_ARM
        Context->Pc += sizeof(ULONG);
#endif
        return TRUE;
    }

    /* Get out of here if the Debugger isn't connected */
    if (KdDebuggerNotPresent) return FALSE;

    /* Save old EIP value */
#ifdef _M_IX86
    EipOld = Context->Eip;
#endif

    /* Call KDBG if available */
    Return = KdbEnterDebuggerException(ExceptionRecord,
                                       PreviousMode,
                                       Context,
                                       TrapFrame,
                                       !SecondChance);

    /* Bump EIP over int 3 if debugger did not already change it */
    if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT)
    {
        //DPRINT1("Address: %p. Return: %d\n", EipOld, Return);
    }

    /* Debugger didn't handle it, please handle! */
    if (Return == kdHandleException) return FALSE;

    /* Debugger handled it */
    return TRUE;
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

    /* Debugger didn't handle it, please handle! */
    if (Return == kdHandleException) return FALSE;

    /* Debugger handled it */
    return TRUE;
}

/* PUBLIC FUNCTIONS *********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
KdRefreshDebuggerNotPresent(VOID)
{
    UNIMPLEMENTED;

    /* Just return whatever was set previously -- FIXME! */
    return KdDebuggerNotPresent;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
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
NTAPI
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
NTAPI
KdPollBreakIn(VOID)
{
    return KdpBreakPending;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdPowerTransition(ULONG PowerState)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdChangeOption(IN KD_OPTION Option,
               IN ULONG InBufferLength OPTIONAL,
               IN PVOID InBuffer,
               IN ULONG OutBufferLength OPTIONAL,
               OUT PVOID OutBuffer,
               OUT PULONG OutBufferRequiredLength OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
NtQueryDebugFilterState(IN ULONG ComponentId,
                        IN ULONG Level)
{
    ULONG i;

    /* Convert Level to mask if it isn't already one */
    if (Level < 32)
        Level = 1 << Level;

    /* Check if it is not the default component */
    if (ComponentId != DPFLTR_DEFAULT_ID)
    {
        /* No, search for an existing entry in the table */
        for (i = 0; i < KdComponentTableEntries; i++)
        {
            /* Check if it is the right component */
            if (ComponentId == KdComponentTable[i].ComponentId)
            {
                /* Check if mask are matching */
                return (Level & KdComponentTable[i].Level) != 0;
            }
        }
    }

    /* Entry not found in the table, use default mask */
    return (Level & Kd_DEFAULT_MASK) != 0;
}

NTSTATUS
NTAPI
NtSetDebugFilterState(IN ULONG ComponentId,
                      IN ULONG Level,
                      IN BOOLEAN State)
{
    ULONG i;

    /* Convert Level to mask if it isn't already one */
    if (Level < 32)
        Level = 1 << Level;
    Level &= ~DPFLTR_MASK;

    /* Check if it is the default component */
    if (ComponentId == DPFLTR_DEFAULT_ID)
    {
        /* Yes, modify the default mask */
        if (State)
            Kd_DEFAULT_MASK |= Level;
        else
            Kd_DEFAULT_MASK &= ~Level;

        return STATUS_SUCCESS;
    }

    /* Search for an existing entry */
    for (i = 0; i < KdComponentTableEntries; i++ )
    {
        if (ComponentId == KdComponentTable[i].ComponentId)
            break;
    }

    /* Check if we have found an existing entry */
    if (i == KdComponentTableEntries)
    {
        /* Check if we have enough space in the table */
        if (i == MAX_KD_COMPONENT_TABLE_ENTRIES)
            return STATUS_INVALID_PARAMETER_1;

        /* Add a new entry */
        ++KdComponentTableEntries;
        KdComponentTable[i].ComponentId = ComponentId;
        KdComponentTable[i].Level = Kd_DEFAULT_MASK;
    }

    /* Update entry table */
    if (State)
        KdComponentTable[i].Level |= Level;
    else
        KdComponentTable[i].Level &= ~Level;

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdSystemDebugControl(IN SYSDBG_COMMAND Command,
                     IN PVOID InputBuffer,
                     IN ULONG InputBufferLength,
                     OUT PVOID OutputBuffer,
                     IN ULONG OutputBufferLength,
                     IN OUT PULONG ReturnLength,
                     IN KPROCESSOR_MODE PreviousMode)
{
    /* HACK */
    return KdpServiceDispatcher(Command, InputBuffer, InputBufferLength);
}

PKDEBUG_ROUTINE KiDebugRoutine = KdpEnterDebuggerException;

 /* EOF */
