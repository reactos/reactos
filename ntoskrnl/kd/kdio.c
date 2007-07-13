/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/kdio.c
 * PURPOSE:         NT Kernel Debugger Input/Output Functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define BufferSize 32*1024

HANDLE KdbLogFileHandle;
BOOLEAN KdpLogInitialized;
CHAR DebugBuffer[BufferSize];
ULONG CurrentPosition;
WORK_QUEUE_ITEM KdpDebugLogQueue;
BOOLEAN ItemQueued;
KD_PORT_INFORMATION SerialPortInfo = {DEFAULT_DEBUG_PORT, DEFAULT_DEBUG_BAUD_RATE, 0};

/* Current Port in use. FIXME: Do we support more then one? */
ULONG KdpPort;

/* DEBUG LOG FUNCTIONS *******************************************************/

VOID
STDCALL
KdpPrintToLogInternal(PVOID Context)
{
    IO_STATUS_BLOCK Iosb;

    /* Write to the Debug Log */
    NtWriteFile(KdbLogFileHandle,
                NULL,
                NULL,
                NULL,
                &Iosb,
                DebugBuffer,
                CurrentPosition,
                NULL,
                NULL);

    /* Clear the Current Position */
    CurrentPosition = 0;

    /* A new item can be queued now */
    ItemQueued = FALSE;
}

VOID
STDCALL
KdpPrintToLog(PCH String,
              ULONG StringLength)
{
    /* Don't overflow */
    if ((CurrentPosition + StringLength) > BufferSize) return;

    /* Add the string to the buffer */
    RtlCopyMemory(&DebugBuffer[CurrentPosition], String, StringLength);

    /* Update the Current Position */
    CurrentPosition += StringLength;

    /* Make sure we are initialized and can queue */
    if (!KdpLogInitialized || (ItemQueued)) return;

    /* 
     * Queue the work item 
     * Note that we don't want to queue if we are > DISPATCH_LEVEL...
     * The message is in the buffer and will simply be taken care of at
     * the next time we are at <= DISPATCH, so it won't be lost.
     */
    if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
        ExQueueWorkItem(&KdpDebugLogQueue, HyperCriticalWorkQueue);
        ItemQueued = TRUE;
    }
}

VOID
STDCALL
KdpInitDebugLog(PKD_DISPATCH_TABLE DispatchTable,
                ULONG BootPhase)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK Iosb;

    if (!KdpDebugMode.File) return;

    if (BootPhase == 0)
    {
        *KdComPortInUse = NULL;

        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpInitDebugLog;
        DispatchTable->KdpPrintRoutine = KdpPrintToLog;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);

        /* Display separator + ReactOS version at start of the debug log */
        DPRINT1("---------------------------------------------------------------\n");
        DPRINT1("ReactOS "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\n");
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\n   File log debugging enabled\n\n");
    }
    else if (BootPhase == 3)
    {
        /* Setup the Log Name */
        RtlInitUnicodeString(&FileName, L"\\SystemRoot\\debug.log");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &FileName,
                                   0,
                                   NULL,
                                   NULL);

        /* Create the Log File */
        Status = NtCreateFile(&KdbLogFileHandle,
                              FILE_ALL_ACCESS,
                              &ObjectAttributes,
                              &Iosb,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ,
                              FILE_SUPERSEDE,
                              FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0);

        /* Allow it to be used */
        ExInitializeWorkItem(&KdpDebugLogQueue, &KdpPrintToLogInternal, NULL);
        KdpLogInitialized = TRUE;
    }
}

/* SERIAL FUNCTIONS **********************************************************/

VOID
STDCALL
KdpSerialDebugPrint(LPSTR Message,
                    ULONG Length)
{
    PCHAR pch = (PCHAR) Message;

    while (*pch != 0)
    {
        if (*pch == '\n')
        {
            KdPortPutByteEx(&SerialPortInfo, '\r');
        }
        KdPortPutByteEx(&SerialPortInfo, *pch);
        pch++;
    }
}

VOID
STDCALL
KdpSerialInit(PKD_DISPATCH_TABLE DispatchTable,
              ULONG BootPhase)
{
    if (!KdpDebugMode.Serial) return;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpSerialInit;
        DispatchTable->KdpPrintRoutine = KdpSerialDebugPrint;

        /* Initialize the Port */
        if (!KdPortInitializeEx(&SerialPortInfo, 0, 0))
        {
            KdpDebugMode.Serial = FALSE;
            return;
        }
        *KdComPortInUse = (PUCHAR)(ULONG_PTR)SerialPortInfo.BaseAddress;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);

        /* Display separator + ReactOS version at start of the debug log */
        DPRINT1("-----------------------------------------------------\n");
        DPRINT1("ReactOS "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\n");
        DPRINT1("Command Line: %s\n", KeLoaderBlock->LoadOptions);
        DPRINT1("ARC Paths: %s %s %s %s\n", KeLoaderBlock->ArcBootDeviceName,
                                            KeLoaderBlock->NtHalPathName,
                                            KeLoaderBlock->ArcHalDeviceName,
                                            KeLoaderBlock->NtBootPathName);
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\n   Serial debugging enabled\n\n");
    }
}

/* SCREEN FUNCTIONS **********************************************************/

VOID
STDCALL
KdpScreenPrint(LPSTR Message,
               ULONG Length)
{
    /* Call HAL */
    HalDisplayString(Message);
}

VOID
STDCALL
KdpScreenInit(PKD_DISPATCH_TABLE DispatchTable,
              ULONG BootPhase)
{
    if (!KdpDebugMode.Screen) return;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpScreenInit;
        DispatchTable->KdpPrintRoutine = KdpScreenPrint;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\n   Screen debugging enabled\n\n");
    }
}

/* GENERAL FUNCTIONS *********************************************************/

ULONG
STDCALL
KdpPrintString(LPSTR String,
               ULONG Length)
{
    PLIST_ENTRY CurrentEntry;
    PKD_DISPATCH_TABLE CurrentTable;

    if (!KdpDebugMode.Value) return 0;

    /* Call the registered handlers */
    CurrentEntry = KdProviders.Flink;
    while (CurrentEntry != &KdProviders)
    {
        /* Get the current table */
        CurrentTable = CONTAINING_RECORD(CurrentEntry,
                                         KD_DISPATCH_TABLE,
                                         KdProvidersList);

        /* Call it */
        CurrentTable->KdpPrintRoutine(String, Length);

        /* Next Table */
        CurrentEntry = CurrentEntry->Flink;
    }

    /* Call the Wrapper Routine */
    if (WrapperInitRoutine) WrapperTable.KdpPrintRoutine(String, Length);

    /* Return the Length */
    return Length;
}

/* EOF */

