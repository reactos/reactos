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

VOID NTAPI PspDumpThreads(BOOLEAN SystemThreads);

extern CPPORT PortInfo;
extern ANSI_STRING KdpLogFileName;

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
KdpTrap(IN PKTRAP_FRAME TrapFrame,
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

    /* We'll manually dump the stack for the user... */
    KeRosDumpStackFrames(NULL, 0);
#endif /* not KDBG */

    /* Debugger didn't handle it, please handle! */
    if (Return == kdHandleException) return FALSE;

    /* Debugger handled it */
    return TRUE;
}

BOOLEAN
NTAPI
KdpStub(IN PKTRAP_FRAME TrapFrame,
        IN PKEXCEPTION_FRAME ExceptionFrame,
        IN PEXCEPTION_RECORD ExceptionRecord,
        IN PCONTEXT ContextRecord,
        IN KPROCESSOR_MODE PreviousMode,
        IN BOOLEAN SecondChanceException)
{
    return KdpTrap(TrapFrame,
                   ExceptionFrame,
                   ExceptionRecord,
                   ContextRecord,
                   PreviousMode,
                   SecondChanceException);
}

/* PUBLIC FUNCTIONS *********************************************************/

static PCHAR
NTAPI
KdpGetDebugMode(PCHAR Currentp2)
{
    PCHAR p1, p2 = Currentp2;
    ULONG Value;

    /* Check for Screen Debugging */
    if (!_strnicmp(p2, "SCREEN", 6))
    {
        /* Enable It */
        p2 += 6;
        KdpDebugMode.Screen = TRUE;
    }
    /* Check for Serial Debugging */
    else if (!_strnicmp(p2, "COM", 3))
    {
        /* Gheck for a valid Serial Port */
        p2 += 3;
        if (*p2 != ':')
        {
            Value = (ULONG)atol(p2);
            if (Value > 0 && Value < 5)
            {
                /* Valid port found, enable Serial Debugging */
                KdpDebugMode.Serial = TRUE;

                /* Set the port to use */
                SerialPortNumber = Value;
                KdpPort = Value;
            }
        }
        else
        {
            Value = strtoul(p2 + 1, NULL, 0);
            if (Value)
            {
                KdpDebugMode.Serial = TRUE;
                SerialPortInfo.Address = UlongToPtr(Value);
                SerialPortNumber = 0;
                KdpPort = 0;
            }
        }
    }
    /* Check for Debug Log Debugging */
    else if (!_strnicmp(p2, "FILE", 4))
    {
        /* Enable It */
        p2 += 4;
        KdpDebugMode.File = TRUE;
        if (*p2 == ':')
        {
            p2++;
            p1 = p2;
            while (*p2 != '\0' && *p2 != ' ') p2++;
            KdpLogFileName.MaximumLength = KdpLogFileName.Length = p2 - p1;
            KdpLogFileName.Buffer = p1;
        }
    }
    /* Check for BOCHS Debugging */
    else if (!_strnicmp(p2, "BOCHS", 5))
    {
        /* Enable It */
        p2 += 5;
        KdpDebugMode.Bochs = TRUE;
    }

    return p2;
}

NTSTATUS
NTAPI
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    ULONG Value;
    ULONG i;
    PCHAR CommandLine, Port = NULL, BaudRate = NULL, Irq = NULL;

    if (LoaderBlock)
    {
        /* Check if we have a command line */
        CommandLine = LoaderBlock->LoadOptions;
        if (CommandLine)
        {
            /* Upcase it */
            _strupr(CommandLine);

            /* Get the port and baud rate */
            Port = strstr(CommandLine, "DEBUGPORT");
            BaudRate = strstr(CommandLine, "BAUDRATE");
            Irq = strstr(CommandLine, "IRQ");
        }
    }

    /* Check if we got the /DEBUGPORT parameter(s) */
    while (Port)
    {
        /* Move past the actual string, to reach the port*/
        Port += sizeof("DEBUGPORT") - 1;

        /* Now get past any spaces and skip the equal sign */
        while (*Port == ' ') Port++;
        Port++;

        /* Get the debug mode and wrapper */
        Port = KdpGetDebugMode(Port);
        Port = strstr(Port, "DEBUGPORT");
    }

    /* Use serial port then */
    if (KdpDebugMode.Value == 0)
        KdpDebugMode.Serial = TRUE;

    /* Check if we got a baud rate */
    if (BaudRate)
    {
        /* Move past the actual string, to reach the rate */
        BaudRate += sizeof("BAUDRATE") - 1;

        /* Now get past any spaces */
        while (*BaudRate == ' ') BaudRate++;

        /* And make sure we have a rate */
        if (*BaudRate)
        {
            /* Read and set it */
            Value = atol(BaudRate + 1);
            if (Value) PortInfo.BaudRate = SerialPortInfo.BaudRate = Value;
        }
    }

    /* Check Serial Port Settings [IRQ] */
    if (Irq)
    {
        /* Move past the actual string, to reach the rate */
        Irq += sizeof("IRQ") - 1;

        /* Now get past any spaces */
        while (*Irq == ' ') Irq++;

        /* And make sure we have an IRQ */
        if (*Irq)
        {
            /* Read and set it */
            Value = atol(Irq + 1);
            if (Value) KdpPortIrq = Value;
        }
    }

    /* Call Providers at Phase 0 */
    for (i = 0; i < KdMax; i++)
    {
        InitRoutines[i](&DispatchTable[i], 0);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    PLIST_ENTRY CurrentEntry;
    PKD_DISPATCH_TABLE CurrentTable;

    /* Call the registered handlers */
    CurrentEntry = KdProviders.Flink;
    while (CurrentEntry != &KdProviders)
    {
        /* Get the current table */
        CurrentTable = CONTAINING_RECORD(CurrentEntry,
                                         KD_DISPATCH_TABLE,
                                         KdProvidersList);

        /* Call it */
        CurrentTable->KdpInitRoutine(CurrentTable, 1);

        /* Next Table */
        CurrentEntry = CurrentEntry->Flink;
    }

    /* Call the Wrapper Init Routine */
    if (WrapperInitRoutine)
        WrapperTable.KdpInitRoutine(&WrapperTable, 1);
    return STATUS_SUCCESS;
}

 /* EOF */
