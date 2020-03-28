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
#include <debug.h>

/* Make bochs debug output in the very early boot phase available */
//#define AUTO_ENABLE_BOCHS

/* VARIABLES ***************************************************************/

ULONG  PortNumber = DEFAULT_DEBUG_PORT;
CPPORT PortInfo   = {0, DEFAULT_DEBUG_BAUD_RATE, 0};
ULONG KdpPortIrq;
#ifdef AUTO_ENABLE_BOCHS
KDP_DEBUG_MODE KdpDebugMode = {{{.Bochs=TRUE}}};
#else
KDP_DEBUG_MODE KdpDebugMode;
#endif
PKDP_INIT_ROUTINE WrapperInitRoutine;
KD_DISPATCH_TABLE WrapperTable;
LIST_ENTRY KdProviders = {&KdProviders, &KdProviders};
KD_DISPATCH_TABLE DispatchTable[KdMax];

PKDP_INIT_ROUTINE InitRoutines[KdMax] = {KdpScreenInit,
                                         KdpSerialInit,
                                         KdpDebugLogInit,
                                         KdpBochsInit,
                                         KdpKdbgInit};

extern ANSI_STRING KdpLogFileName;

/* PRIVATE FUNCTIONS *********************************************************/

CODE_SEG("INIT")
PCHAR
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

CODE_SEG("INIT")
VOID
NTAPI
KdpCallInitRoutine(ULONG BootPhase)
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
        CurrentTable->KdpInitRoutine(CurrentTable, BootPhase);

        /* Next Table */
        CurrentEntry = CurrentEntry->Flink;
    }

    /* Call the Wrapper Init Routine */
    if (WrapperInitRoutine)
        WrapperTable.KdpInitRoutine(&WrapperTable, BootPhase);
}

BOOLEAN
NTAPI
KdInitSystem(ULONG BootPhase,
             PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Value;
    ULONG i;
    PCHAR CommandLine, Port = NULL, BaudRate = NULL, Irq = NULL;

    /* Set Default Port Options */
    if (BootPhase == 0)
    {
        /* Check if we have a loader block */
        if (LoaderBlock)
        {
            /* Check if we have a command line */
            CommandLine = LoaderBlock->LoadOptions;
            if (CommandLine)
            {
                /* Upcase it */
                _strupr(CommandLine);

                /* XXX Check for settings that we support */
                if (strstr(CommandLine, "NODEBUG")) KdDebuggerEnabled = FALSE;
                else if (strstr(CommandLine, "CRASHDEBUG")) KdDebuggerEnabled = FALSE;
                else if (strstr(CommandLine, "DEBUG"))
                {
                    /* Enable the kernel debugger */
                    KdDebuggerNotPresent = FALSE;
                    KdDebuggerEnabled = TRUE;
#ifdef KDBG
                    /* Get the KDBG Settings */
                    KdbpGetCommandLineSettings(LoaderBlock->LoadOptions);
#endif
                }

                /* Get the port and baud rate */
                Port = strstr(CommandLine, "DEBUGPORT");
                BaudRate = strstr(CommandLine, "BAUDRATE");
                Irq = strstr(CommandLine, "IRQ");
            }
            else
            {
                /* No command line options? Disable debugger by default */
                KdDebuggerEnabled = FALSE;
            }
        }
        else
        {
            /* Called from a bugcheck or a re-enable. Unconditionally enable KD. */
            KdDebuggerEnabled = TRUE;
        }

        /* Let user-mode know our state */
        SharedUserData->KdDebuggerEnabled = KdDebuggerEnabled;

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
        if (KdDebuggerEnabled && KdpDebugMode.Value == 0)
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

        /* Call Wrapper at Phase 0 */
        if (WrapperInitRoutine) WrapperInitRoutine(&WrapperTable, 0);
        return TRUE;
    }
    else /* BootPhase > 0 */
    {
    }

    /* Call the Initialization Routines of the Registered Providers */
    KdpCallInitRoutine(BootPhase);

    /* Return success */
    return TRUE;
}

/* EOF */
