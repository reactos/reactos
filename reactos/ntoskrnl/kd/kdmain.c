/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/kd/kdmain.c
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
BOOLEAN KdBreakAfterSymbolLoad = FALSE;
BOOLEAN KdpBreakPending = FALSE;
BOOLEAN KdPitchDebugger = TRUE;
BOOLEAN KdIgnoreUmExceptions = FALSE;
KD_CONTEXT KdpContext;
ULONG Kd_WIN2000_Mask;
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

#if DBG
        case ' soR': /* ROS-INTERNAL */
        {
            switch ((ULONG_PTR)Buffer1)
            {
                case DumpAllThreads:
                    PspDumpThreads(TRUE);
                    break;

                case DumpUserThreads:
                    PspDumpThreads(FALSE);
                    break;

                case KdSpare3:
                    MmDumpArmPfnDatabase(FALSE);
                    break;

                default:
                    break;
            }
            break;
        }

#if defined(_M_IX86) && !defined(_WINKD_) // See ke/i386/traphdlr.c
        /* Register a debug callback */
        case 'CsoR':
        {
            switch (Buffer1Length)
            {
                case ID_Win32PreServiceHook:
                    KeWin32PreServiceHook = Buffer1;
                    break;

                case ID_Win32PostServiceHook:
                    KeWin32PostServiceHook = Buffer1;
                    break;

            }
            break;
        }
#endif

        /* Special  case for stack frame dumps */
        case 'DsoR':
        {
            KeRosDumpStackFrames((PULONG)Buffer1, Buffer1Length);
            break;
        }

#if defined(KDBG)
        /* Register KDBG CLI callback */
        case 'RbdK':
        {
            Result = KdbRegisterCliCallback(Buffer1, Buffer1Length);
            break;
        }
#endif /* KDBG */
#endif /* DBG */
        default:
            DPRINT1("Invalid debug service call!\n");
            HalDisplayString("Invalid debug service call!\r\n");
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
    KD_CONTINUE_TYPE Return = kdHandleException;
    ULONG ExceptionCommand = ExceptionRecord->ExceptionInformation[0];

    /* Check if this was a breakpoint due to DbgPrint or Load/UnloadSymbols */
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionCommand == BREAKPOINT_LOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_UNLOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_COMMAND_STRING) ||
         (ExceptionCommand == BREAKPOINT_PRINT) ||
         (ExceptionCommand == BREAKPOINT_PROMPT)))
    {
        /* Check if this is a debug print */
        if (ExceptionCommand == BREAKPOINT_PRINT)
        {
            /* Print the string */
            KdpServiceDispatcher(BREAKPOINT_PRINT,
                                 (PVOID)ExceptionRecord->ExceptionInformation[1],
                                 ExceptionRecord->ExceptionInformation[2]);

            /* Return success */
            KeSetContextReturnRegister(Context, STATUS_SUCCESS);
        }
#ifdef KDBG
        else if (ExceptionCommand == BREAKPOINT_LOAD_SYMBOLS)
        {
            PLDR_DATA_TABLE_ENTRY LdrEntry;

            /* Load symbols. Currently implemented only for KDBG! */
            if(KdbpSymFindModule(((PKD_SYMBOLS_INFO)ExceptionRecord->ExceptionInformation[2])->BaseOfDll, NULL, -1, &LdrEntry))
                KdbSymProcessSymbols(LdrEntry);
        }
        else if (ExceptionCommand == BREAKPOINT_PROMPT)
        {
            ULONG ReturnValue;
            LPSTR OutString;
            USHORT OutStringLength;

            /* Get the response string  and length */
            OutString = (LPSTR)Context->Ebx;
            OutStringLength = (USHORT)Context->Edi;

            /* Call KDBG */
            ReturnValue = KdpPrompt((LPSTR)ExceptionRecord->
                                    ExceptionInformation[1],
                                    (USHORT)ExceptionRecord->
                                    ExceptionInformation[2],
                                    OutString,
                                    OutStringLength);

            /* Return the number of characters that we received */
            Context->Eax = ReturnValue;
        }
#endif

        /* This we can handle: simply bump the Program Counter */
        KeSetContextPc(Context, KeGetContextPc(Context) + KD_BREAKPOINT_SIZE);
        return TRUE;
    }

#ifdef KDBG
    /* Check if this is an assertion failure */
    if (ExceptionRecord->ExceptionCode == STATUS_ASSERTION_FAILURE)
    {
        /* Bump EIP to the instruction following the int 2C */
        Context->Eip += 2;
    }
#endif

    /* Get out of here if the Debugger isn't connected */
    if (KdDebuggerNotPresent) return FALSE;

#ifdef KDBG
    /* Call KDBG if available */
    Return = KdbEnterDebuggerException(ExceptionRecord,
                                       PreviousMode,
                                       Context,
                                       TrapFrame,
                                       !SecondChance);
#else /* not KDBG */
    if (WrapperInitRoutine)
    {
        /* Call GDB */
        Return = WrapperTable.KdpExceptionRoutine(ExceptionRecord,
                                                  Context,
                                                  TrapFrame);
    }
#endif /* not KDBG */

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

BOOLEAN
NTAPI
KdIsThisAKdTrap(IN PEXCEPTION_RECORD ExceptionRecord,
                IN PCONTEXT Context,
                IN KPROCESSOR_MODE PreviousMode)
{
    /* KDBG has its own mechanism for ignoring user mode exceptions */
    return FALSE;
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
    if (ComponentId != MAXULONG)
    {
        /* No, search for an existing entry in the table */
        for (i = 0; i < KdComponentTableEntries; i++)
        {
            /* Check if it is the right component */
            if (ComponentId == KdComponentTable[i].ComponentId)
            {
                /* Check if mask are matching */
                return (Level & KdComponentTable[i].Level) ? TRUE : FALSE;
            }
        }
    }

    /* Entry not found in the table, use default mask */
    return (Level & Kd_DEFAULT_MASK) ? TRUE : FALSE;
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
    if (ComponentId == MAXULONG)
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
