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
KdpPrintToLog(PCH String)
{
    ULONG StringLength = strlen(String);

    /* Don't overflow */
    if ((CurrentPosition + StringLength) > BufferSize) return;

    /* Add the string to the buffer */
    RtlMoveMemory(&DebugBuffer[CurrentPosition], String, StringLength);

    /* Update the Current Position */
    CurrentPosition += StringLength;

    /* Make sure we are initialized and can queue */
    if (!KdpLogInitialized || (ItemQueued)) return;

    /* Queue the work item */
    ExQueueWorkItem(&KdpDebugLogQueue, HyperCriticalWorkQueue);
    ItemQueued = TRUE;
}

VOID
STDCALL
KdpInitDebugLog(PKD_DISPATCH_TABLE DispatchTable,
                ULONG BootPhase)
{
    if (!KdpDebugMode.File) return;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK Iosb;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpInitDebugLog;
        DispatchTable->KdpPrintRoutine = KdpPrintToLog;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
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
KdpSerialDebugPrint(LPSTR Message)
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
        KdPortInitializeEx(&SerialPortInfo, 0, 0);

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\n   Serial debugging enabled\n\n");
    }
}

/* SCREEN FUNCTIONS **********************************************************/

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
        DispatchTable->KdpPrintRoutine = HalDisplayString;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\n   Screen debugging enabled\n\n");
    }
}

/* GENERAL FUNCTIONS *********************************************************/

BOOLEAN
STDCALL
KdpDetectConflicts(PCM_RESOURCE_LIST DriverList)
{
    ULONG ComPortBase = 0;
    ULONG i;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;

    /* Select the COM Port Base */
    switch (KdpPort)
    {
        case 1: ComPortBase = 0x3f8; break;
        case 2: ComPortBase = 0x2f8; break;
        case 3: ComPortBase = 0x3e8; break;
        case 4: ComPortBase = 0x2e8; break;
    }

    /* search for this port address in DriverList */
    for (i = 0; i < DriverList->List[0].PartialResourceList.Count; i++)
    {
        ResourceDescriptor = &DriverList->List[0].PartialResourceList.PartialDescriptors[i];
        if (ResourceDescriptor->Type == CmResourceTypePort)
        {
            if ((ResourceDescriptor->u.Port.Start.u.LowPart <= ComPortBase) &&
                (ResourceDescriptor->u.Port.Start.u.LowPart +
                 ResourceDescriptor->u.Port.Length > ComPortBase))
            {
                /* Conflict found */
                return TRUE;
            }
        }
    }

    /* No Conflicts */
    return FALSE;
}

ULONG
STDCALL
KdpPrintString(PANSI_STRING String)
{
    if (!KdpDebugMode.Value) return 0;
    PCH pch = String->Buffer;
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
        CurrentTable->KdpPrintRoutine(pch);

        /* Next Table */
        CurrentEntry = CurrentEntry->Flink;
    }

    /* Call the Wrapper Routine */
    if (WrapperInitRoutine) WrapperTable.KdpPrintRoutine(pch);

    /* Return the Length */
    return((ULONG)String->Length);
}

/* EOF */

