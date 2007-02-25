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

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, KdInitSystem)
#endif


/* Make bochs debug output in the very early boot phase available */
//#define AUTO_ENABLE_BOCHS

/* VARIABLES ***************************************************************/

KD_PORT_INFORMATION PortInfo = {DEFAULT_DEBUG_PORT, DEFAULT_DEBUG_BAUD_RATE, 0};
ULONG KdpPortIrq;
#ifdef AUTO_ENABLE_BOCHS
KDP_DEBUG_MODE KdpDebugMode = {{{.Bochs=TRUE}}};;
#else
KDP_DEBUG_MODE KdpDebugMode;
#endif
PKDP_INIT_ROUTINE WrapperInitRoutine;
KD_DISPATCH_TABLE WrapperTable;
BOOLEAN KdpEarlyBreak = FALSE;
LIST_ENTRY KdProviders = {&KdProviders, &KdProviders};
KD_DISPATCH_TABLE DispatchTable[KdMax];

PKDP_INIT_ROUTINE InitRoutines[KdMax] = {KdpScreenInit,
                                         KdpSerialInit,
                                         KdpInitDebugLog,
                                         KdpBochsInit};

/* PRIVATE FUNCTIONS *********************************************************/

PCHAR
STDCALL
KdpGetWrapperDebugMode(PCHAR Currentp2,
                       PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR p2 = Currentp2;

    /* Check for GDB Debugging */
    if (!_strnicmp(p2, "GDB", 3))
    {
        /* Enable it */
        p2 += 3;
        KdpDebugMode.Gdb = TRUE;

        /* Enable Debugging */
        KdDebuggerEnabled = TRUE;
        KdDebuggerNotPresent = FALSE;
        WrapperInitRoutine = KdpGdbStubInit;
    }

    /* Check for PICE Debugging */
    else if (!_strnicmp(p2, "PICE", 4))
    {
        /* Enable it */
        p2 += 4;
        KdpDebugMode.Pice = TRUE;

        /* Enable Debugging */
        KdDebuggerEnabled = TRUE;
        KdDebuggerNotPresent = FALSE;
    }

#ifdef KDBG
    /* Get the KDBG Settings and enable it */
    KdDebuggerEnabled = TRUE;
    KdDebuggerNotPresent = FALSE;
    KdbpGetCommandLineSettings(LoaderBlock->LoadOptions);
#endif
    return p2;
}

PCHAR
STDCALL
KdpGetDebugMode(PCHAR Currentp2)
{
    PCHAR p2 = Currentp2;
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
        Value = (ULONG)atol(p2);
        if (Value > 0 && Value < 5)
        {
            /* Valid port found, enable Serial Debugging */
            KdpDebugMode.Serial = TRUE;

            /* Set the port to use */
            SerialPortInfo.ComPort = Value;
            KdpPort = Value;
        }
    }
    /* Check for Debug Log Debugging */
    else if (!_strnicmp(p2, "FILE", 4))
    {
        /* Enable It */
        p2 += 4;
        KdpDebugMode.File = TRUE;
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

VOID
STDCALL
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
INIT_FUNCTION
NTAPI
KdInitSystem(ULONG BootPhase,
             PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Value;
    ULONG i;
    PCHAR CommandLine, Port, BaudRate, Irq;

    /* Set Default Port Options */
    if (BootPhase == 0)
    {
        /* Get the Command Line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Upcase it */
        _strupr(CommandLine);

        /* Check for settings that we support */
        if (strstr(CommandLine, "BREAK")) KdpEarlyBreak = TRUE;
        if (strstr(CommandLine, "NODEBUG")) KdDebuggerEnabled = FALSE;
        if (strstr(CommandLine, "CRASHDEBUG")) KdDebuggerEnabled = FALSE;
        if (strstr(CommandLine, "DEBUG"))
        {
            /* Enable on the serial port */
            KdDebuggerEnabled = TRUE;
            KdpDebugMode.Serial = TRUE;
        }

        /* Get the port and baud rate */
        Port = strstr(CommandLine, "DEBUGPORT");
        BaudRate = strstr(CommandLine, "BAUDRATE");
        Irq = strstr(CommandLine, "IRQ");

        /* Check if we got the /DEBUGPORT parameter */
        if (Port)
        {
            /* Move past the actual string, to reach the port*/
            Port += strlen("DEBUGPORT");

            /* Now get past any spaces and skip the equal sign */
            while (*Port == ' ') Port++;
            Port++;

            /* Get the debug mode and wrapper */
            Port = KdpGetDebugMode(Port);
            Port = KdpGetWrapperDebugMode(Port, LoaderBlock);
            KdDebuggerEnabled = TRUE;
        }

        /* Check if we got a baud rate */
        if (BaudRate)
        {
            /* Move past the actual string, to reach the rate */
            BaudRate += strlen("BAUDRATE");

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
            Irq += strlen("IRQ");

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

    /* Call the Initialization Routines of the Registered Providers */
    KdpCallInitRoutine(BootPhase);

    /* Return success */
    return TRUE;
}

 /* EOF */
