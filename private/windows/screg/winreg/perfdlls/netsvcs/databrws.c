/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    databrws.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Browser Performance data objects

Created:

    Bob Watson  22-Oct-1996

Revision History:

    None.

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "databrws.h"

// dummy variable for field sizing.
static BROWSER_COUNTER_DATA   bcd;

BROWSER_DATA_DEFINITION BrowserDataDefinition =
{
    {   sizeof (BROWSER_DATA_DEFINITION) + sizeof (BROWSER_COUNTER_DATA),
        sizeof (BROWSER_DATA_DEFINITION),
        sizeof (PERF_OBJECT_TYPE),
        BROWSER_OBJECT_TITLE_INDEX,
        0,
        53,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(BROWSER_DATA_DEFINITION) - sizeof(PERF_OBJECT_TYPE)) /
            sizeof (PERF_COUNTER_DEFINITION),
        0,
        -1,
        UNICODE_CODE_PAGE,
        {0L, 0L},
        {0L, 0L}
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        54,
        0,
        55,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_BULK_COUNT,
        sizeof (bcd.ServerAnnounce),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->ServerAnnounce
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        78,
        0,
        79,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_BULK_COUNT,
        sizeof (bcd.DomainAnnounce),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->DomainAnnounce
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        188,
        0,
        813,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_BULK_COUNT,
        sizeof (bcd.TotalAnnounce),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->TotalAnnounce
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        80,
        0,
        81,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.ElectionPacket),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->ElectionPacket
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        82,
        0,
        83,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.MailslotWrite),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->MailslotWrite
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        84,
        0,
        85,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.ServerList),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->ServerList
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        156,
        0,
        161,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.ServerEnum),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->ServerEnum
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        158,
        0,
        163,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.DomainEnum),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->DomainEnum
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        160,
        0,
        165,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.OtherEnum),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->OtherEnum
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        190,
        0,
        815,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.TotalEnum),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->TotalEnum
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        162,
        0,
        167,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (bcd.ServerAnnounceMiss),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->ServerAnnounceMiss
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        164,
        0,
        169,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (bcd.MailslotDatagramMiss),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->MailslotDatagramMiss
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        166,
        0,
        171,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (bcd.ServerListMiss),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->ServerListMiss
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        168,
        0,
        381,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.ServerAnnounceAllocMiss),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->ServerAnnounceAllocMiss
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        170,
        0,
        383,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (bcd.MailslotAllocFail),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->MailslotAllocFail
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        806,
        0,
        385,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (bcd.MailslotReceiveFail),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->MailslotReceiveFail
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        808,
        0,
        387,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (bcd.MailslotWriteFail),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->MailslotWriteFail
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        810,
        0,
        807,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (bcd.MailslotOpenFail),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->MailslotOpenFail
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        812,
        0,
        809,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (bcd.MasterAnnounceDup),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->MasterAnnounceDup
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        814,
        0,
        811,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_BULK_COUNT,
        sizeof (bcd.DatagramIllegal),
        (DWORD)(ULONG_PTR)&((BROWSER_COUNTER_DATA *)0)->DatagramIllegal
    }
};

