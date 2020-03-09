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

VOID NTAPI PspDumpThreads(BOOLEAN SystemThreads);

extern CPPORT PortInfo;
extern ANSI_STRING KdpLogFileName;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KdpReportCommandStringStateChange(IN PSTRING NameString,
                                  IN PSTRING CommandString,
                                  IN OUT PCONTEXT Context)
{
}

BOOLEAN
NTAPI
KdpReportExceptionStateChange(IN PEXCEPTION_RECORD ExceptionRecord,
                              IN OUT PCONTEXT ContextRecord,
                              IN PKTRAP_FRAME TrapFrame,
                              IN KPROCESSOR_MODE PreviousMode,
                              IN BOOLEAN SecondChanceException)
{
    KD_CONTINUE_TYPE Return = kdHandleException;
#ifdef KDBG
    /* Check if this is an assertion failure */
    if (ExceptionRecord->ExceptionCode == STATUS_ASSERTION_FAILURE)
    {
        /* Bump EIP to the instruction following the int 2C */
        ContextRecord->Eip += 2;
    }
#endif

    /* Get out of here if the Debugger isn't connected */
    if (KdDebuggerNotPresent) return FALSE;

#ifdef KDBG
    /* Call KDBG if available */
    Return = KdbEnterDebuggerException(ExceptionRecord,
                                       PreviousMode,
                                       ContextRecord,
                                       TrapFrame,
                                       !SecondChanceException);
#else /* not KDBG */
    if (WrapperInitRoutine)
    {
        /* Call GDB */
        Return = WrapperTable.KdpExceptionRoutine(ExceptionRecord,
                                                  ContextRecord,
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

#ifdef KDBG
            /* Get the KDBG Settings */
            KdbpGetCommandLineSettings(CommandLine);
#endif

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

    NtGlobalFlag |= FLG_STOP_ON_EXCEPTION;

    return STATUS_SUCCESS;
}

 /* EOF */
