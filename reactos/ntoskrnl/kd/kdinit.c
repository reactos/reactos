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

/* Make bochs debug output in the very early boot phase available */
//#define AUTO_ENABLE_BOCHS

/* VARIABLES ***************************************************************/

KD_PORT_INFORMATION PortInfo = {DEFAULT_DEBUG_PORT, DEFAULT_DEBUG_BAUD_RATE, 0};
ULONG KdpPortIrq;
#ifdef AUTO_ENABLE_BOCHS
KDP_DEBUG_MODE KdpDebugMode = {{{.Bochs=TRUE}}};;
PKDP_INIT_ROUTINE WrapperInitRoutine = KdpBochsInit;
KD_DISPATCH_TABLE WrapperTable = {.KdpInitRoutine = KdpBochsInit, .KdpPrintRoutine = KdpBochsDebugPrint};
#else
KDP_DEBUG_MODE KdpDebugMode;
PKDP_INIT_ROUTINE WrapperInitRoutine;
KD_DISPATCH_TABLE WrapperTable;
#endif
LIST_ENTRY KdProviders = {&KdProviders, &KdProviders};
KD_DISPATCH_TABLE DispatchTable[KdMax];

PKDP_INIT_ROUTINE InitRoutines[KdMax] = {KdpScreenInit,
                                         KdpSerialInit,
                                         KdpInitDebugLog};

/* PRIVATE FUNCTIONS *********************************************************/

PCHAR
STDCALL
KdpGetWrapperDebugMode(PCHAR Currentp2,
                       PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR p2 = Currentp2;

#ifdef DBG
    /* Check for BOCHS Debugging */
    if (!_strnicmp(p2, "BOCHS", 5))
    {
        /* Enable It */
        p2 += 5;
        KdpDebugMode.Bochs = TRUE;
        WrapperInitRoutine = KdpBochsInit;
    }

    /* Check for GDB Debugging */
    if (!_strnicmp(p2, "GDB", 3))
    {
        /* Enable it */
        p2 += 3;
        KdpDebugMode.Gdb = TRUE;

        /* Enable Debugging */
        KdDebuggerEnabled = TRUE;
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
    }
#endif

#ifdef KDBG
    /* Get the KDBG Settings and enable it */
    KdDebuggerEnabled = TRUE;
    KdpDebugMode.Gdb = TRUE;
    KdbpGetCommandLineSettings((PCHAR)LoaderBlock->CommandLine);
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

VOID
INIT_FUNCTION
KdInitSystem(ULONG BootPhase,
             PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Value;
    ULONG i;
    PCHAR p1, p2;

    /* Set Default Port Options */
    if (BootPhase == 0)
    {

        /* Parse the Command Line */
        p1 = (PCHAR)LoaderBlock->CommandLine;
        while (p1 && (p2 = strchr(p1, '/')))
        {
            /* Move past the slash */
            p2++;

            /* Identify the Debug Type being Used */
            if (!_strnicmp(p2, "DEBUGPORT=", 10))
            {
                p2 += 10;
                p2 = KdpGetDebugMode(p2);
                p2 = KdpGetWrapperDebugMode(p2, LoaderBlock);
            }
            /* Check for Kernel Debugging Enable */
            else if (!_strnicmp(p2, "DEBUG", 5))
            {
                /* Enable it on the Serial Port */
                p2 += 5;
                KdDebuggerEnabled = TRUE;
                KdpDebugMode.Serial = TRUE;
            }
            /* Check for Kernel Debugging Bypass */
            else if (!_strnicmp(p2, "NODEBUG", 7))
            {
                /* Disable Debugging */
                p2 += 7;
                KdDebuggerEnabled = FALSE;
            }
            /* Check for Kernel Debugging Bypass unless STOP Error */
            else if (!_strnicmp(p2, "CRASHDEBUG", 10))
            {
                /* Disable Debugging */
                p2 += 10;
                KdDebuggerEnabled = FALSE;
            }
            /* Check Serial Port Settings [Baud Rate] */
            else if (!_strnicmp(p2, "BAUDRATE=", 9))
            {
                /* Get the Baud Rate */
                p2 += 9;
                Value = (ULONG)atol(p2);

                /* Check if it's valid and Set it */
                if (0 < Value) PortInfo.BaudRate = SerialPortInfo.BaudRate = Value;
            }
            /* Check Serial Port Settings [IRQ] */
            else if (!_strnicmp(p2, "IRQ=", 4))
            {
                /* Get the IRQ */
                p2 += 3;
                Value = (ULONG)atol(p2);

                /* Check if it's valid and set it */
                if (0 < Value) KdpPortIrq = Value;
            }

            /* Move to next */
            p1 = p2;
        }

        /* Call Providers at Phase 0 */
        for (i = 0; i < KdMax; i++)
        {
            InitRoutines[i](&DispatchTable[i], 0);
        }

        /* Call Wrapper at Phase 0 */
        if (WrapperInitRoutine) WrapperInitRoutine(&WrapperTable, 0);

        return;
    }

    /* Call the Initialization Routines of the Registered Providers */
    KdpCallInitRoutine(BootPhase);
}

 /* EOF */
