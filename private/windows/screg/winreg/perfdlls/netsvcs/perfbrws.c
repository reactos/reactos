/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfbrws.c

Abstract:

    This file implements a Performance Object that presents
    Browser Performance object data

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
#include <windows.h>
#include <assert.h>
#include <winperf.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <lmbrowsr.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfnet.h"
#include "netsvcmc.h"
#include "databrws.h"

//  BrowserStatFunction is used for collecting Browser Statistic Data
typedef NET_API_STATUS (*PBROWSERQUERYSTATISTIC) (
    IN  LPTSTR      servername OPTIONAL,
    OUT LPBROWSER_STATISTICS *statistics
    );

static  PBROWSERQUERYSTATISTIC BrowserStatFunction = NULL;
static  HANDLE dllHandle  = NULL;
static  BOOL    bInitOk = FALSE;

DWORD APIENTRY
OpenBrowserObject (
    IN  LPWSTR  lpValueName
)
/*++
    GetBrowserStatistic   -   Get the I_BrowserQueryStatistics entry point

--*/
{
    UINT    dwOldMode;
    LONG    status = ERROR_SUCCESS;

    dwOldMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    bInitOk = TRUE;
    //
    // Dynamically link to netapi32.dll.  If it's not there just return.
    //

    dllHandle = LoadLibraryW((LPCWSTR)L"NetApi32.Dll") ;
    if ( !dllHandle || dllHandle == INVALID_HANDLE_VALUE ) {
        status = GetLastError();
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFNET_UNABLE_OPEN_NETAPI32_DLL,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);
        BrowserStatFunction = NULL;
        bInitOk = FALSE;
    } else {
        //
        // Get the address of the service's main entry point.  This
        // entry point has a well-known name.
        //

        BrowserStatFunction = (PBROWSERQUERYSTATISTIC)GetProcAddress (
            dllHandle, "I_BrowserQueryStatistics") ;

        if (BrowserStatFunction == NULL) {
            status = GetLastError();
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFNET_UNABLE_LOCATE_BROWSER_PERF_FN,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);
            bInitOk = FALSE;
        }
    }

    SetErrorMode( dwOldMode );

    return status;
}

DWORD APIENTRY
CollectBrowserObjectData(
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
    DWORD                   TotalLen;  //  Length of the total return block
    NTSTATUS                Status = ERROR_SUCCESS;

    BROWSER_DATA_DEFINITION *pBrowserDataDefinition;
    BROWSER_COUNTER_DATA    *pBCD;

    BROWSER_STATISTICS      BrowserStatistics;
    LPBROWSER_STATISTICS    pBrowserStatistics = &BrowserStatistics;

    //
    //  Check for sufficient space for browser data
    //

    if (!bInitOk) {
        // function didn't initialize so bail out here
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    TotalLen = sizeof(BROWSER_DATA_DEFINITION) +
               sizeof(BROWSER_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        // not enough room in the buffer for 1 instance
        // so bail
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //

    pBrowserDataDefinition = (BROWSER_DATA_DEFINITION *) *lppData;

    memcpy (pBrowserDataDefinition,
            &BrowserDataDefinition,
            sizeof(BROWSER_DATA_DEFINITION));
    //
    //  Format and collect browser data
    //

    pBCD = (PBROWSER_COUNTER_DATA)&pBrowserDataDefinition[1];

    // test for quadword alignment of the structure
    assert  (((DWORD)(pBCD) & 0x00000007) == 0);
    

    memset (pBrowserStatistics, 0, sizeof (BrowserStatistics));

    if ( BrowserStatFunction != NULL ) {
        Status = (*BrowserStatFunction) (NULL,
                                         &pBrowserStatistics
                                        );
    } else {
        Status =  STATUS_INVALID_ADDRESS;
    }

    if (NT_SUCCESS(Status)) {
        pBCD->CounterBlock.ByteLength = sizeof(BROWSER_COUNTER_DATA);
        pBCD->TotalAnnounce  =
            pBCD->ServerAnnounce = BrowserStatistics.NumberOfServerAnnouncements.QuadPart;
        pBCD->TotalAnnounce  +=
            pBCD->DomainAnnounce = BrowserStatistics.NumberOfDomainAnnouncements.QuadPart;

        pBCD->ElectionPacket = BrowserStatistics.NumberOfElectionPackets;
        pBCD->MailslotWrite = BrowserStatistics.NumberOfMailslotWrites;
        pBCD->ServerList    = BrowserStatistics.NumberOfGetBrowserServerListRequests;
        pBCD->ServerEnum    = BrowserStatistics.NumberOfServerEnumerations;
        pBCD->DomainEnum    = BrowserStatistics.NumberOfDomainEnumerations;
        pBCD->OtherEnum     = BrowserStatistics.NumberOfOtherEnumerations;
        pBCD->TotalEnum     = BrowserStatistics.NumberOfServerEnumerations
                              + BrowserStatistics.NumberOfDomainEnumerations
                              + BrowserStatistics.NumberOfOtherEnumerations;
        pBCD->ServerAnnounceMiss    = BrowserStatistics.NumberOfMissedServerAnnouncements;
        pBCD->MailslotDatagramMiss  = BrowserStatistics.NumberOfMissedMailslotDatagrams;
        pBCD->ServerListMiss        = BrowserStatistics.NumberOfMissedGetBrowserServerListRequests;
        pBCD->ServerAnnounceAllocMiss = BrowserStatistics.NumberOfFailedServerAnnounceAllocations;
        pBCD->MailslotAllocFail     = BrowserStatistics.NumberOfFailedMailslotAllocations;
        pBCD->MailslotReceiveFail   = BrowserStatistics.NumberOfFailedMailslotReceives;
        pBCD->MailslotWriteFail     = BrowserStatistics.NumberOfFailedMailslotWrites;
        pBCD->MailslotOpenFail      = BrowserStatistics.NumberOfFailedMailslotOpens;
        pBCD->MasterAnnounceDup     = BrowserStatistics.NumberOfDuplicateMasterAnnouncements;
        pBCD->DatagramIllegal       = BrowserStatistics.NumberOfIllegalDatagrams.QuadPart;

    } else {
        if (BrowserStatFunction != NULL) {
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFNET_UNABLE_LOCATE_BROWSER_PERF_FN,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&Status);
        }

        //
        // Failure to access Browser: clear counters to 0
        //
        memset(pBCD, 0, sizeof(BROWSER_COUNTER_DATA));
        pBCD->CounterBlock.ByteLength = sizeof(BROWSER_COUNTER_DATA);

    }
    *lppData = (LPVOID)&pBCD[1];
    *lpcbTotalBytes =
        (DWORD)((LPBYTE)&pBCD[1] -
        (LPBYTE)pBrowserDataDefinition);
    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}

DWORD APIENTRY
CloseBrowserObject ()
{
    if (dllHandle != NULL) {
        FreeLibrary (dllHandle);
        dllHandle = NULL;
        BrowserStatFunction = NULL;
    }
    return ERROR_SUCCESS;
}
