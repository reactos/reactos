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

//
// Retrieves the ComponentId and Level for BREAKPOINT_PRINT
// and OutputString and OutputStringLength for BREAKPOINT_PROMPT.
//
#if defined(_X86_)

//
// EBX/EDI on x86
//
#define KdpGetParameterThree(Context)  ((Context)->Ebx)
#define KdpGetParameterFour(Context)   ((Context)->Edi)

#elif defined(_AMD64_)

//
// R8/R9 on AMD64
//
#define KdpGetParameterThree(Context)  ((Context)->R8)
#define KdpGetParameterFour(Context)   ((Context)->R9)

#elif defined(_ARM_)

//
// R3/R4 on ARM
//
#define KdpGetParameterThree(Context)  ((Context)->R3)
#define KdpGetParameterFour(Context)   ((Context)->R4)

#else
#error Unsupported Architecture
#endif

/* VARIABLES ***************************************************************/

BOOLEAN KdDebuggerEnabled = FALSE;
BOOLEAN KdEnteredDebugger = FALSE;
BOOLEAN KdDebuggerNotPresent = TRUE;
BOOLEAN KdBreakAfterSymbolLoad = FALSE;
BOOLEAN KdPitchDebugger = TRUE;
BOOLEAN KdIgnoreUmExceptions = FALSE;

VOID NTAPI PspDumpThreads(BOOLEAN SystemThreads);

#if 0
ULONG Kd_DEFAULT_MASK = 1 << DPFLTR_ERROR_LEVEL;
#endif

/* PRIVATE FUNCTIONS *********************************************************/

ULONG
NTAPI
KdpServiceDispatcher(ULONG Service,
                     PVOID Buffer1,
                     ULONG Buffer1Length,
                     KPROCESSOR_MODE PreviousMode)
{
    ULONG Result = 0;

    switch (Service)
    {
        case BREAKPOINT_PRINT: /* DbgPrint */
        {
            /* Call KDBG */
            BOOLEAN Handled;
            Result = KdpPrint(MAXULONG,
                              DPFLTR_INFO_LEVEL,
                              (PCHAR)Buffer1,
                              (USHORT)Buffer1Length,
                              PreviousMode,
                              NULL, // TrapFrame,
                              NULL, // ExceptionFrame,
                              &Handled);
            break;
        }

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
            KeRosDumpStackFrames((PULONG_PTR)Buffer1, Buffer1Length);
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
            /* Call KDBG */
            NTSTATUS ReturnStatus;
            BOOLEAN Handled;
            ReturnStatus = KdpPrint((ULONG)KdpGetParameterThree(Context),
                                    (ULONG)KdpGetParameterFour(Context),
                                    (PCHAR)ExceptionRecord->ExceptionInformation[1],
                                    (USHORT)ExceptionRecord->ExceptionInformation[2],
                                    PreviousMode,
                                    TrapFrame,
                                    ExceptionFrame,
                                    &Handled);

            /* Update the return value for the caller */
            KeSetContextReturnRegister(Context, ReturnStatus);
        }
#ifdef KDBG
        else if (ExceptionCommand == BREAKPOINT_LOAD_SYMBOLS)
        {
            PKD_SYMBOLS_INFO SymbolsInfo;
            KD_SYMBOLS_INFO CapturedSymbolsInfo;
            PLDR_DATA_TABLE_ENTRY LdrEntry;

            SymbolsInfo = (PKD_SYMBOLS_INFO)ExceptionRecord->ExceptionInformation[2];
            if (PreviousMode != KernelMode)
            {
                _SEH2_TRY
                {
                    ProbeForRead(SymbolsInfo,
                                 sizeof(*SymbolsInfo),
                                 1);
                    KdpMoveMemory(&CapturedSymbolsInfo,
                                  SymbolsInfo,
                                  sizeof(*SymbolsInfo));
                    SymbolsInfo = &CapturedSymbolsInfo;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    SymbolsInfo = NULL;
                }
                _SEH2_END;
            }

            if (SymbolsInfo != NULL)
            {
                /* Load symbols. Currently implemented only for KDBG! */
                if (KdbpSymFindModule(SymbolsInfo->BaseOfDll, NULL, -1, &LdrEntry))
                {
                    KdbSymProcessSymbols(LdrEntry);
                }
            }
        }
        else if (ExceptionCommand == BREAKPOINT_PROMPT)
        {
            /* Call KDBG */
            ULONG ReturnLength;
            ReturnLength = KdpPrompt((PCHAR)ExceptionRecord->ExceptionInformation[1],
                                     (USHORT)ExceptionRecord->ExceptionInformation[2],
                                     (PCHAR)KdpGetParameterThree(Context),
                                     (USHORT)KdpGetParameterFour(Context),
                                     PreviousMode,
                                     TrapFrame,
                                     ExceptionFrame);

            /* Update the return value for the caller */
            KeSetContextReturnRegister(Context, ReturnLength);
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
#ifdef _M_AMD64
        Context->Rip += 2;
#else
        Context->Eip += 2;
#endif
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
KdIsThisAKdTrap(IN PEXCEPTION_RECORD ExceptionRecord,
                IN PCONTEXT Context,
                IN KPROCESSOR_MODE PreviousMode)
{
    /* KDBG has its own mechanism for ignoring user mode exceptions */
    return FALSE;
}

/* PUBLIC FUNCTIONS *********************************************************/

VOID
NTAPI
KdUpdateDataBlock(VOID)
{
}

BOOLEAN
NTAPI
KdEnterDebugger(IN PKTRAP_FRAME TrapFrame,
                IN PKEXCEPTION_FRAME ExceptionFrame)
{
    return FALSE;
}

VOID
NTAPI
KdExitDebugger(IN BOOLEAN Enable)
{
}

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
    SharedUserData->KdDebuggerEnabled = FALSE;

    /* Lower the IRQL */
    KeLowerIrql(OldIrql);

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdEnableDebuggerWithLock(IN BOOLEAN NeedLock)
{
    return STATUS_ACCESS_DENIED;
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
    SharedUserData->KdDebuggerEnabled = TRUE;

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
    return FALSE;
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
    return KdpServiceDispatcher(Command,
                                InputBuffer,
                                InputBufferLength,
                                PreviousMode);
}

PKDEBUG_ROUTINE KiDebugRoutine = KdpEnterDebuggerException;

 /* EOF */
