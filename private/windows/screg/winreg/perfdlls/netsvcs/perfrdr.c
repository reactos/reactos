/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfrdr.c

Abstract:

    This file implements a Performance Object that presents
    Redirector Performance object data

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
#include <ntioapi.h>
#include <windows.h>
#include <assert.h>
#include <srvfsctl.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfnet.h"
#include "netsvcmc.h"
#include "datardr.h"

static HANDLE  hRdr = NULL;

DWORD APIENTRY
OpenRedirObject (
    IN  LPWSTR  lpValueName
)
{
    UNICODE_STRING      DeviceNameU;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    NTSTATUS            status;
    RTL_RELATIVE_NAME   RelativeName;

    // open the handle to the server for data collection
    //
    //  Now get access to the Redirector for its data
    //

    RtlInitUnicodeString(&DeviceNameU, (LPCWSTR)DD_NFS_DEVICE_NAME_U);
    RelativeName.ContainingDirectory = NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                                &DeviceNameU,
                                OBJ_CASE_INSENSITIVE,
                                RelativeName.ContainingDirectory,
                                NULL
                                );

    status = NtCreateFile(&hRdr,
                            SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_OPEN_IF,
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0
                            );

    if (!NT_SUCCESS(status)) {
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFNET_UNABLE_OPEN_REDIR,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);
    }

    return (DWORD)RtlNtStatusToDosError(status);

}

DWORD APIENTRY
CollectRedirObjectData(
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

    DWORD           TotalLen;          //  Length of the total return block
    NTSTATUS        Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    RDR_DATA_DEFINITION *pRdrDataDefinition;
    RDR_COUNTER_DATA    *pRCD;

    REDIR_STATISTICS RdrStatistics;

    if ( hRdr == NULL ) {
        // redir didn't get opened and it has already been logged
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }
    //
    //  Check for sufficient space for redirector data
    //

    TotalLen = sizeof(RDR_DATA_DEFINITION) +
               sizeof(RDR_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //

    pRdrDataDefinition = (RDR_DATA_DEFINITION *) *lppData;

    memcpy (pRdrDataDefinition,
            &RdrDataDefinition,
            sizeof(RDR_DATA_DEFINITION));

    //
    //  Format and collect redirector data
    //

    pRCD = (PRDR_COUNTER_DATA)&pRdrDataDefinition[1];

    // test for quadword alignment of the structure
    assert  (((DWORD)(pRCD) & 0x00000007) == 0);

    pRCD->CounterBlock.ByteLength = sizeof (RDR_COUNTER_DATA);

    Status = NtFsControlFile(hRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_LMR_GET_STATISTICS,
                                NULL,
                                0,
                                &RdrStatistics,
                                sizeof(RdrStatistics)
                                );
    if (NT_SUCCESS(Status)) {
        // transfer Redir data
        pRCD->Bytes             = RdrStatistics.BytesReceived.QuadPart +
                                  RdrStatistics.BytesTransmitted.QuadPart;
        pRCD->IoOperations      = RdrStatistics.ReadOperations +
                                  RdrStatistics.WriteOperations;
        pRCD->Smbs              = RdrStatistics.SmbsReceived.QuadPart +
                                  RdrStatistics.SmbsTransmitted.QuadPart;
        pRCD->BytesReceived     = RdrStatistics.BytesReceived.QuadPart;
        pRCD->SmbsReceived      = RdrStatistics.SmbsReceived.QuadPart;
        pRCD->PagingReadBytesRequested  = RdrStatistics.PagingReadBytesRequested.QuadPart;
        pRCD->NonPagingReadBytesRequested   = RdrStatistics.NonPagingReadBytesRequested.QuadPart;
        pRCD->CacheReadBytesRequested   = RdrStatistics.CacheReadBytesRequested.QuadPart;
        pRCD->NetworkReadBytesRequested = RdrStatistics.NetworkReadBytesRequested.QuadPart;
        pRCD->BytesTransmitted  = RdrStatistics.BytesTransmitted.QuadPart;
        pRCD->SmbsTransmitted   = RdrStatistics.SmbsTransmitted.QuadPart;
        pRCD->PagingWriteBytesRequested = RdrStatistics.PagingWriteBytesRequested.QuadPart;
        pRCD->NonPagingWriteBytesRequested  = RdrStatistics.NonPagingWriteBytesRequested.QuadPart;
        pRCD->CacheWriteBytesRequested  = RdrStatistics.CacheWriteBytesRequested.QuadPart;
        pRCD->NetworkWriteBytesRequested    = RdrStatistics.NetworkWriteBytesRequested.QuadPart;
        pRCD->ReadOperations    = RdrStatistics.ReadOperations;
        pRCD->RandomReadOperations  = RdrStatistics.RandomReadOperations;
        pRCD->ReadSmbs          = RdrStatistics.ReadSmbs;
        pRCD->LargeReadSmbs     = RdrStatistics.LargeReadSmbs;
        pRCD->SmallReadSmbs     = RdrStatistics.SmallReadSmbs;
        pRCD->WriteOperations   = RdrStatistics.WriteOperations;
        pRCD->RandomWriteOperations = RdrStatistics.RandomWriteOperations;
        pRCD->WriteSmbs         = RdrStatistics.WriteSmbs;
        pRCD->LargeWriteSmbs    = RdrStatistics.LargeWriteSmbs;
        pRCD->SmallWriteSmbs    = RdrStatistics.SmallWriteSmbs;
        pRCD->RawReadsDenied    = RdrStatistics.RawReadsDenied;
        pRCD->RawWritesDenied   = RdrStatistics.RawWritesDenied;
        pRCD->NetworkErrors     = RdrStatistics.NetworkErrors;
        pRCD->Sessions          = RdrStatistics.Sessions;
        pRCD->Reconnects        = RdrStatistics.Reconnects;
        pRCD->CoreConnects      = RdrStatistics.CoreConnects;
        pRCD->Lanman20Connects  = RdrStatistics.Lanman20Connects;
        pRCD->Lanman21Connects  = RdrStatistics.Lanman21Connects;
        pRCD->LanmanNtConnects  = RdrStatistics.LanmanNtConnects;
        pRCD->ServerDisconnects = RdrStatistics.ServerDisconnects;
        pRCD->HungSessions      = RdrStatistics.HungSessions;
        pRCD->CurrentCommands   = RdrStatistics.CurrentCommands;

    } else {

        //
        // Failure to access Redirector: clear counters to 0
        //

        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFNET_UNABLE_READ_REDIR,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&Status);

        memset(pRCD, 0, sizeof(RDR_COUNTER_DATA));
        pRCD->CounterBlock.ByteLength = sizeof (RDR_COUNTER_DATA);

    }

    *lppData = (LPVOID)&pRCD[1];

    *lpcbTotalBytes = (DWORD)((LPBYTE)&pRCD[1] - (LPBYTE)pRdrDataDefinition);
    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}

DWORD APIENTRY
CloseRedirObject ()
{
    if (hRdr != NULL) {
        NtClose(hRdr);
        hRdr = NULL;
    }

    return ERROR_SUCCESS;
}
