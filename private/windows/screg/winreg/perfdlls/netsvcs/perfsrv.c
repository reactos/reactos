/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfsrv.c

Abstract:

    This file implements a Performance Object that presents
    Server Performance object data

Created:

    Bob Watson  22-Oct-1996

Revision History


--*/
//
//  Include Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddnfs.h>
#include <windows.h>
#include <assert.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <srvfsctl.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <assert.h>
#include <perfutil.h>
#include "perfnet.h"
#include "netsvcmc.h"
#include "datasrv.h"
#include "datasrvq.h"

#define MAX_SRVQ_NAME_LENGTH    16

static  HANDLE  hSrv = NULL;

static  SRV_QUEUE_STATISTICS *pSrvQueueStatistics = NULL;
static  DWORD  dwDataBufferLength = 0L;
static  SYSTEM_BASIC_INFORMATION BasicInfo;

static  BOOL bSrvQOk = TRUE;

DWORD APIENTRY
OpenServerObject (
    IN  LPWSTR  lpValueName
)
{
    STRING              DeviceName;
    UNICODE_STRING      DeviceNameU;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    NTSTATUS            status;

    // open the handle to the server for data collection
    //
    // Get access to the Server for it's data
    //

    RtlInitString(&DeviceName, SERVER_DEVICE_NAME);
    RtlAnsiStringToUnicodeString(&DeviceNameU, &DeviceName, TRUE);
    InitializeObjectAttributes(&ObjectAttributes,
                                &DeviceNameU,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

    status = NtOpenFile(&hSrv,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT
                        );

    if (!NT_SUCCESS(status)) {
        hSrv = NULL;
        bSrvQOk = FALSE;
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFNET_UNABLE_OPEN_SERVER,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);
    }

    RtlFreeUnicodeString(&DeviceNameU);

    return (DWORD)RtlNtStatusToDosError(status);

}

DWORD APIENTRY
OpenServerQueueObject (
    IN  LPWSTR  szValueName
)
{
    NTSTATUS    status;
    //
    //  collect basic and static processor data
    //

    status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &BasicInfo,
                    sizeof(SYSTEM_BASIC_INFORMATION),
                    NULL
                    );

    assert (NT_SUCCESS(status));
    if (!NT_SUCCESS(status)) {
        // all we really want is the number of processors so
        // if we can't get that from the system, then we'll
        // substitute 32 for the number
        BasicInfo.NumberOfProcessors = 32;
        status = ERROR_SUCCESS;
    }
    // compute the various buffer sizes required

    dwDataBufferLength = sizeof(SRV_QUEUE_STATISTICS) *
        (BasicInfo.NumberOfProcessors + 1);

    pSrvQueueStatistics = (SRV_QUEUE_STATISTICS *)ALLOCMEM (
        hLibHeap, HEAP_ZERO_MEMORY, dwDataBufferLength);

    // if memory allocation failed, then no server queue stats will
    // be returned.

    assert (pSrvQueueStatistics != NULL);

    if (pSrvQueueStatistics == NULL) {
        bSrvQOk = FALSE;
    }

    return ERROR_SUCCESS;

}

DWORD APIENTRY
CollectServerObjectData(
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the Physical Disk object

Arguments:

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD  TotalLen;            //  Length of the total return block
    NTSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    SRV_DATA_DEFINITION *pSrvDataDefinition;
    SRV_COUNTER_DATA    *pSCD;

    SRV_STATISTICS SrvStatistics;

    ULONG      Remainder;

    if (hSrv == NULL) {
        // bail out if the server didn't get opened.
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    //
    //  Check for sufficient space for server data
    //

    TotalLen = sizeof(SRV_DATA_DEFINITION) +
               sizeof(SRV_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        // bail out if the data won't fit in the caller's buffer
        // or the server didn't get opened.
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //

    pSrvDataDefinition = (SRV_DATA_DEFINITION *) *lppData;

    memcpy (pSrvDataDefinition,
           &SrvDataDefinition,
           sizeof(SRV_DATA_DEFINITION));

    //
    //  Format and collect server data
    //

    pSCD = (PSRV_COUNTER_DATA)&pSrvDataDefinition[1];

    // test for quadword alignment of the structure
    assert  (((DWORD)(pSCD) & 0x00000007) == 0);

    pSCD->CounterBlock.ByteLength = sizeof(SRV_COUNTER_DATA);

    Status = NtFsControlFile(hSrv,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_SRV_GET_STATISTICS,
                                NULL,
                                0,
                                &SrvStatistics,
                                sizeof(SrvStatistics)
                                );

    if ( NT_SUCCESS(Status) ) {
        pSCD->TotalBytes            = SrvStatistics.TotalBytesSent.QuadPart +
                                        SrvStatistics.TotalBytesReceived.QuadPart;

        pSCD->TotalBytesReceived    = SrvStatistics.TotalBytesReceived.QuadPart;
        pSCD->TotalBytesSent        = SrvStatistics.TotalBytesSent.QuadPart;
        pSCD->SessionsTimedOut      = SrvStatistics.SessionsTimedOut;
        pSCD->SessionsErroredOut    = SrvStatistics.SessionsErroredOut;
        pSCD->SessionsLoggedOff     = SrvStatistics.SessionsLoggedOff;
        pSCD->SessionsForcedLogOff  = SrvStatistics.SessionsForcedLogOff;
        pSCD->LogonErrors           = SrvStatistics.LogonErrors;
        pSCD->AccessPermissionErrors = SrvStatistics.AccessPermissionErrors;
        pSCD->GrantedAccessErrors   = SrvStatistics.GrantedAccessErrors;
        pSCD->SystemErrors          = SrvStatistics.SystemErrors;
        pSCD->BlockingSmbsRejected  = SrvStatistics.BlockingSmbsRejected;
        pSCD->WorkItemShortages     = SrvStatistics.WorkItemShortages;
        pSCD->TotalFilesOpened      = SrvStatistics.TotalFilesOpened;
        pSCD->CurrentOpenFiles      = SrvStatistics.CurrentNumberOfOpenFiles;
        pSCD->CurrentSessions       = SrvStatistics.CurrentNumberOfSessions;
        pSCD->CurrentOpenSearches   = SrvStatistics.CurrentNumberOfOpenSearches;
        pSCD->CurrentNonPagedPoolUsage = SrvStatistics.CurrentNonPagedPoolUsage;
        pSCD->NonPagedPoolFailures  = SrvStatistics.NonPagedPoolFailures;
        pSCD->PeakNonPagedPoolUsage = SrvStatistics.PeakNonPagedPoolUsage;
        pSCD->CurrentPagedPoolUsage = SrvStatistics.CurrentPagedPoolUsage;
        pSCD->PagedPoolFailures     = SrvStatistics.PagedPoolFailures;
        pSCD->PeakPagedPoolUsage    = SrvStatistics.PeakPagedPoolUsage;
        pSCD->ContextBlockQueueRate = SrvStatistics.TotalWorkContextBlocksQueued.Count;
        pSCD->NetLogon =
            pSCD->NetLogonTotal     = SrvStatistics.SessionLogonAttempts;

    } else {

        // log an event describing the error
        DWORD   dwData[4];
        DWORD   dwDataIndex = 0;

        dwData[dwDataIndex++] = Status;
        dwData[dwDataIndex++] = IoStatusBlock.Status;
        dwData[dwDataIndex++] = (DWORD)IoStatusBlock.Information;

        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,        // error type
            0,                          // category (not used)
            PERFNET_UNABLE_READ_SERVER, // error code
            NULL,                       // SID (not used),
            0,                          // number of strings
            dwDataIndex * sizeof(DWORD),  // sizeof raw data
            NULL,                       // message text array
            (LPVOID)&dwData[0]);        // raw data
        //
        // Failure to access Server: clear counters to 0
        //

        memset(pSCD, 0, sizeof(SRV_COUNTER_DATA));
        pSCD->CounterBlock.ByteLength = sizeof(SRV_COUNTER_DATA);
    }

    *lppData = (LPVOID)&pSCD[1];
    *lpcbTotalBytes = (DWORD)((LPBYTE)&pSCD[1] - (LPBYTE)pSrvDataDefinition);
    *lpNumObjectTypes = 1;
    return ERROR_SUCCESS;
}

DWORD APIENTRY
CollectServerQueueObjectData(
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the Physical Disk object

Arguments:

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD  dwPerfDataLength;
    LONG  nQueue;

    DWORD                       *pdwCounter;
    LARGE_INTEGER UNALIGNED     *pliCounter;

    NTSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    SRVQ_DATA_DEFINITION        *pSrvQDataDefinition;
    PERF_INSTANCE_DEFINITION    *pPerfInstanceDefinition;
    SRVQ_COUNTER_DATA           *pSQCD;


    SRV_QUEUE_STATISTICS *pThisQueueStatistics;

    UNICODE_STRING      QueueName;
    WCHAR               QueueNameBuffer[MAX_SRVQ_NAME_LENGTH];

    ULONG      Remainder;
    NET_API_STATUS   NetStatus;

    if (!bSrvQOk) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    //
    //  Check for sufficient space for server data
    //

    TotalLen = sizeof(SRVQ_DATA_DEFINITION) +
               sizeof(PERF_INSTANCE_DEFINITION) +
               sizeof(SRVQ_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    // assign local pointer to current position in buffer
    pSrvQDataDefinition = (SRVQ_DATA_DEFINITION *) *lppData;

    //
    //  Define perf object data block
    //

    memcpy (pSrvQDataDefinition,
            &SrvQDataDefinition,
            sizeof(SRVQ_DATA_DEFINITION));

    //
    //  Format and collect server Queue data
    //

    QueueName.Length = 0;
    QueueName.MaximumLength = sizeof(QueueNameBuffer);
    QueueName.Buffer = QueueNameBuffer;

    Status = NtFsControlFile(hSrv,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_SRV_GET_QUEUE_STATISTICS,
                                NULL,
                                0,
                                pSrvQueueStatistics,
                                dwDataBufferLength
                                );

    if (NT_SUCCESS(Status)) {
        // server data was collected successfully so...
        // process each processor queue instance.

        nQueue = 0;
        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                            &pSrvQDataDefinition[1];

        TotalLen = sizeof(SRVQ_DATA_DEFINITION);

        for (nQueue = 0; nQueue < BasicInfo.NumberOfProcessors; nQueue++) {
            // see if this instance will fit
            TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                        8 +     // size of 3 (unicode) digit queuelength name
                        sizeof(SRVQ_COUNTER_DATA);

            if ( *lpcbTotalBytes < TotalLen ) {
                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                return ERROR_MORE_DATA;
            }

            RtlIntegerToUnicodeString(nQueue,
                                      10,
                                      &QueueName);

            // there should be enough room for this instance so initialize it

            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                (PVOID *) &pSQCD,
                0,
                0,
                (DWORD)-1,
                QueueName.Buffer);

            pSQCD->CounterBlock.ByteLength = sizeof (SRVQ_COUNTER_DATA);

            // initialize pointers for this instance
            pThisQueueStatistics = &pSrvQueueStatistics[nQueue];

            pSQCD->QueueLength = pThisQueueStatistics->QueueLength;
            pSQCD->ActiveThreads = pThisQueueStatistics->ActiveThreads;
            pSQCD->AvailableThreads = pThisQueueStatistics->AvailableThreads;
            pSQCD->AvailableWorkItems = pThisQueueStatistics->FreeWorkItems;
            pSQCD->BorrowedWorkItems = pThisQueueStatistics->StolenWorkItems;
            pSQCD->WorkItemShortages = pThisQueueStatistics->NeedWorkItem;
            pSQCD->CurrentClients = pThisQueueStatistics->CurrentClients;
            pSQCD->TotalBytesTransfered =
                pSQCD->BytesReceived = pThisQueueStatistics->BytesReceived.QuadPart;
            pSQCD->TotalBytesTransfered +=
                pSQCD->BytesSent = pThisQueueStatistics->BytesSent.QuadPart;
            pSQCD->TotalOperations =
                pSQCD->ReadOperations = pThisQueueStatistics->ReadOperations.QuadPart;
            pSQCD->TotalBytes =
                pSQCD->BytesRead = pThisQueueStatistics->BytesRead.QuadPart;
            pSQCD->TotalOperations +=
                pSQCD->WriteOperations = pThisQueueStatistics->WriteOperations.QuadPart;
            pSQCD->TotalBytes +=
                pSQCD->BytesWritten = pThisQueueStatistics->BytesWritten.QuadPart;
            pSQCD->TotalContextBlocksQueued = pThisQueueStatistics->TotalWorkContextBlocksQueued.Count;

            // update the current pointer
            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pSQCD[1];
        }

        RtlInitUnicodeString (&QueueName, (LPCWSTR)L"Blocking Queue");

        // now load the "blocking" queue data
        // see if this instance will fit
        TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                QWORD_MULTIPLE(QueueName.Length + sizeof(WCHAR)) +
                sizeof (SRVQ_COUNTER_DATA);

        if ( *lpcbTotalBytes < TotalLen ) {
            // this instance won't fit so bail out
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        // there should be enough room for this instance so initialize it

        MonBuildInstanceDefinition(pPerfInstanceDefinition,
            (PVOID *) &pSQCD,
            0,
            0,
            (DWORD)-1,
            QueueName.Buffer);

        pSQCD->CounterBlock.ByteLength = sizeof(SRVQ_COUNTER_DATA);

        // initialize pointers for this instance
        pThisQueueStatistics = &pSrvQueueStatistics[nQueue];

        pSQCD->QueueLength = pThisQueueStatistics->QueueLength;
        pSQCD->ActiveThreads = pThisQueueStatistics->ActiveThreads;
        pSQCD->AvailableThreads = pThisQueueStatistics->AvailableThreads;
        pSQCD->AvailableWorkItems = 0;
        pSQCD->BorrowedWorkItems = 0;
        pSQCD->WorkItemShortages = 0;
        pSQCD->CurrentClients = 0;
        pSQCD->TotalBytesTransfered =
            pSQCD->BytesReceived = pThisQueueStatistics->BytesReceived.QuadPart;
        pSQCD->TotalBytesTransfered +=
            pSQCD->BytesSent = pThisQueueStatistics->BytesSent.QuadPart;
        pSQCD->ReadOperations = 0;
        pSQCD->TotalBytes =
            pSQCD->BytesRead = pThisQueueStatistics->BytesRead.QuadPart;
        pSQCD->WriteOperations = 0;
        pSQCD->TotalBytes +=
            pSQCD->BytesWritten = pThisQueueStatistics->BytesWritten.QuadPart;
        pSQCD->TotalOperations = 0;
        pSQCD->TotalContextBlocksQueued = pThisQueueStatistics->TotalWorkContextBlocksQueued.Count;

        nQueue++; // to include the Blocking Queue statistics entry

        // update the current pointer
        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pSQCD[1];

        // update queue (instance) count in object data block
        pSrvQDataDefinition->SrvQueueObjectType.NumInstances = nQueue;

        // update available length
        *lpcbTotalBytes =
            pSrvQDataDefinition->SrvQueueObjectType.TotalByteLength =
                (DWORD)((PCHAR) pPerfInstanceDefinition -
                (PCHAR) pSrvQDataDefinition);

#if DBG
        if (*lpcbTotalBytes > TotalLen ) {
            DbgPrint ("\nPERFNET: Server Queue Perf Ctr. Instance Size Underestimated:");
            DbgPrint ("\nPERFNET:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
        }
#endif

        *lppData = (LPVOID)pPerfInstanceDefinition;

        *lpNumObjectTypes = 1;
        return ERROR_SUCCESS;
    } else {
        // unable to read server queue data for some reason so don't return this
        // object

        // log an event describing the error
        DWORD   dwData[4];
        DWORD   dwDataIndex = 0;

        dwData[dwDataIndex++] = Status;
        dwData[dwDataIndex++] = IoStatusBlock.Status;
        dwData[dwDataIndex++] = (DWORD)IoStatusBlock.Information;

        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,        // error type
            0,                          // category (not used)
            PERFNET_UNABLE_READ_SERVER_QUEUE, // error code
            NULL,                       // SID (not used),
            0,                          // number of strings
            dwDataIndex * sizeof(DWORD),  // sizeof raw data
            NULL,                       // message text array
            (LPVOID)&dwData[0]);        // raw data

        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }
}

DWORD APIENTRY
CloseServerObject ()
{
    if (hSrv != NULL) {
        NtClose(hSrv);
        hSrv = NULL;
    }

    return ERROR_SUCCESS;
}
DWORD APIENTRY
CloseServerQueueObject ()
{
    if (hLibHeap != NULL) {
        if (pSrvQueueStatistics != NULL) {
            FREEMEM (hLibHeap, 0, pSrvQueueStatistics);
            pSrvQueueStatistics = NULL;
        }
    }
    return ERROR_SUCCESS;
}

