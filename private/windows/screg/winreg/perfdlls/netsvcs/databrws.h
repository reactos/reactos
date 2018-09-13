/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATABRWS.h

Abstract:

    Header file for the Windows NT Browser Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATABRWS_H_
#define _DATABRWS_H_

//
//    This is the Browser counter structure presently returned by NT.
//

typedef struct _BROWSER_DATA_DEFINITION {
    PERF_OBJECT_TYPE            BrowserObjectType;
    PERF_COUNTER_DEFINITION     ServerAnnounce;
    PERF_COUNTER_DEFINITION     DomainAnnounce;
    PERF_COUNTER_DEFINITION     TotalAnnounce;
    PERF_COUNTER_DEFINITION     ElectionPacket;
    PERF_COUNTER_DEFINITION     MailslotWrite;
    PERF_COUNTER_DEFINITION     ServerList;
    PERF_COUNTER_DEFINITION     ServerEnum;
    PERF_COUNTER_DEFINITION     DomainEnum;
    PERF_COUNTER_DEFINITION     OtherEnum;
    PERF_COUNTER_DEFINITION     TotalEnum;
    PERF_COUNTER_DEFINITION     ServerAnnounceMiss;
    PERF_COUNTER_DEFINITION     MailslotDatagramMiss;
    PERF_COUNTER_DEFINITION     ServerListMiss;
    PERF_COUNTER_DEFINITION     ServerAnnounceAllocMiss;
    PERF_COUNTER_DEFINITION     MailslotAllocFail;
    PERF_COUNTER_DEFINITION     MailslotReceiveFail;
    PERF_COUNTER_DEFINITION     MailslotWriteFail;
    PERF_COUNTER_DEFINITION     MailslotOpenFail;
    PERF_COUNTER_DEFINITION     MasterAnnounceDup;
    PERF_COUNTER_DEFINITION     DatagramIllegal;
}  BROWSER_DATA_DEFINITION, *PBROWSER_DATA_DEFINITION;

typedef struct _BROWSER_COUNTER_DATA{
    PERF_COUNTER_BLOCK      CounterBlock;
    LONGLONG                ServerAnnounce;
    LONGLONG                DomainAnnounce;
    LONGLONG                TotalAnnounce;
    DWORD                   ElectionPacket;
    DWORD                   MailslotWrite;
    DWORD                   ServerList;
    DWORD                   ServerEnum;
    DWORD                   DomainEnum;
    DWORD                   OtherEnum;
    DWORD                   TotalEnum;
    DWORD                   ServerAnnounceMiss;
    DWORD                   MailslotDatagramMiss;
    DWORD                   ServerListMiss;
    DWORD                   ServerAnnounceAllocMiss;
    DWORD                   MailslotAllocFail;
    DWORD                   MailslotReceiveFail;
    DWORD                   MailslotWriteFail;
    DWORD                   MailslotOpenFail;
    DWORD                   MasterAnnounceDup;
    LONGLONG                DatagramIllegal;
} BROWSER_COUNTER_DATA, * PBROWSER_COUNTER_DATA;

extern BROWSER_DATA_DEFINITION BrowserDataDefinition;

#endif // _DATABRWS_H_
