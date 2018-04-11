/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdStatistics.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"
#include <rtltypes.h>
#include <rtlfuncs.h>

static
INT
DisplayServerStatistics(VOID)
{
    PSERVER_INFO_100 ServerInfo = NULL;
    PSTAT_SERVER_0 StatisticsInfo = NULL;
    LARGE_INTEGER LargeValue;
    FILETIME FileTime, LocalFileTime;
    SYSTEMTIME SystemTime;
    WORD wHour;
    INT nPaddedLength = 35;
    NET_API_STATUS Status;

    Status = NetServerGetInfo(NULL, 100, (PBYTE*)&ServerInfo);
    if (Status != NERR_Success)
        goto done;

    Status = NetStatisticsGet(NULL,
                              SERVICE_SERVER,
                              0,
                              0,
                              (LPBYTE*)&StatisticsInfo);
    if (Status != NERR_Success)
        goto done;

    ConResPrintf(StdOut, IDS_STATISTICS_SRV_NAME, ServerInfo->sv100_name);

    RtlSecondsSince1970ToTime(StatisticsInfo->sts0_start,
                              &LargeValue);
    FileTime.dwLowDateTime = LargeValue.u.LowPart;
    FileTime.dwHighDateTime = LargeValue.u.HighPart;
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SystemTime);

    wHour = SystemTime.wHour;
    if (wHour == 0)
    {
        wHour = 12;
    }
    else if (wHour > 12)
    {
        wHour = wHour - 12;
    }

    ConResPrintf(StdOut, IDS_STATISTICS_SINCE,
                 SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
                 wHour, SystemTime.wMinute,
                 (SystemTime.wHour >= 1 && SystemTime.wHour < 13) ? L"AM" : L"PM");

    PrintPaddedResourceString(IDS_STATISTICS_SRV_SESACCEPT, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_sopens);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_SESSTIME, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_stimedout);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_SESSERROR, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_serrorout);

    LargeValue.u.LowPart = StatisticsInfo->sts0_bytessent_low;
    LargeValue.u.HighPart = StatisticsInfo->sts0_bytessent_high;
    PrintPaddedResourceString(IDS_STATISTICS_SRV_KBSENT, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", LargeValue.QuadPart / 1024);

    LargeValue.u.LowPart = StatisticsInfo->sts0_bytesrcvd_low;
    LargeValue.u.HighPart = StatisticsInfo->sts0_bytesrcvd_high;
    PrintPaddedResourceString(IDS_STATISTICS_SRV_KBRCVD, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", LargeValue.QuadPart / 1024);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_MRESPTIME, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_avresponse);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_SYSERRORS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_syserrors);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_PMERRORS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_permerrors);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_PWERRORS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_pwerrors);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_FILES, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_fopens);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_DEVICES, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_devopens);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_JOBS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_jobsqueued);

    ConResPuts(StdOut, IDS_STATISTICS_SRV_BUFFERS);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_BIGBUFFERS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_bigbufneed);

    PrintPaddedResourceString(IDS_STATISTICS_SRV_REQBUFFERS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_reqbufneed);

done:
    if (StatisticsInfo != NULL)
        NetApiBufferFree(StatisticsInfo);

    if (ServerInfo != NULL)
        NetApiBufferFree(ServerInfo);

    return 0;
}


static
INT
DisplayWorkstationStatistics(VOID)
{
    PWKSTA_INFO_100 WorkstationInfo = NULL;
    PSTAT_WORKSTATION_0 StatisticsInfo = NULL;
    LARGE_INTEGER LargeValue;
    FILETIME FileTime, LocalFileTime;
    SYSTEMTIME SystemTime;
    WORD wHour;
    INT nPaddedLength = 47;
    NET_API_STATUS Status;

    Status = NetWkstaGetInfo(NULL,
                             100,
                             (PBYTE*)&WorkstationInfo);
    if (Status != NERR_Success)
        goto done;

    Status = NetStatisticsGet(NULL,
                              SERVICE_SERVER,
                              0,
                              0,
                              (LPBYTE*)&StatisticsInfo);
    if (Status != NERR_Success)
        goto done;

    ConResPrintf(StdOut, IDS_STATISTICS_WKS_NAME, WorkstationInfo->wki100_computername);

    RtlSecondsSince1970ToTime(StatisticsInfo->StatisticsStartTime.u.LowPart,
                              &LargeValue);
    FileTime.dwLowDateTime = LargeValue.u.LowPart;
    FileTime.dwHighDateTime = LargeValue.u.HighPart;
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SystemTime);

    wHour = SystemTime.wHour;
    if (wHour == 0)
    {
        wHour = 12;
    }
    else if (wHour > 12)
    {
        wHour = wHour - 12;
    }

    ConResPrintf(StdOut, IDS_STATISTICS_SINCE,
                 SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
                 wHour, SystemTime.wMinute, (SystemTime.wHour >= 1 && SystemTime.wHour < 13) ? L"AM" : L"PM");

    PrintPaddedResourceString(IDS_STATISTICS_WKS_BYTESRCVD, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->BytesReceived.QuadPart);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_SMBSRCVD, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->SmbsReceived.QuadPart);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_BYTESTRANS, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->BytesTransmitted.QuadPart);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_SMBSTRANS, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->SmbsTransmitted.QuadPart);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_READOPS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->ReadOperations);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_WRITEOPS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->WriteOperations);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_READDENIED, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->RawReadsDenied);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_WRITEDENIED, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->RawWritesDenied);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_NETWORKERROR, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->NetworkErrors);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_CONNECTS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->CoreConnects +
                                StatisticsInfo->Lanman20Connects +
                                StatisticsInfo->Lanman21Connects +
                                StatisticsInfo->LanmanNtConnects);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_RECONNECTS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->Reconnects);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_DISCONNECTS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->ServerDisconnects);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_SESSIONS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->Sessions);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_HUNGSESSIONS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->HungSessions);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_FAILSESSIONS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->FailedSessions);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_FAILEDOPS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->InitiallyFailedOperations +
                                StatisticsInfo->FailedCompletionOperations);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_USECOUNT, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->UseCount);

    PrintPaddedResourceString(IDS_STATISTICS_WKS_FAILUSECOUNT, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->FailedUseCount);

done:
    if (StatisticsInfo != NULL)
        NetApiBufferFree(StatisticsInfo);

    if (WorkstationInfo != NULL)
        NetApiBufferFree(WorkstationInfo);

    return 0;
}


INT
cmdStatistics(
    INT argc,
    WCHAR **argv)
{
    INT i, result = 0;
    BOOL bServer = FALSE;
    BOOL bWorkstation = FALSE;

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"server") == 0)
        {
            if (bWorkstation == FALSE)
                bServer = TRUE;
            continue;
        }

        if (_wcsicmp(argv[i], L"workstation") == 0)
        {
            if (bServer == FALSE)
                bWorkstation = TRUE;
            continue;
        }

        if (_wcsicmp(argv[i], L"help") == 0)
        {
            /* Print short syntax help */
            ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
            ConResPuts(StdOut, IDS_STATISTICS_SYNTAX);
            return 0;
        }

        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
            ConResPuts(StdOut, IDS_STATISTICS_SYNTAX);
            ConResPuts(StdOut, IDS_STATISTICS_HELP_1);
            ConResPuts(StdOut, IDS_STATISTICS_HELP_2);
            return 0;
        }
    }

    if (bServer)
    {
        result = DisplayServerStatistics();
    }
    else if (bWorkstation)
    {
        result = DisplayWorkstationStatistics();
    }
    else
    {
        ConResPuts(StdOut, IDS_STATISTICS_TEXT);
    }

    if (result == 0)
        ConResPuts(StdOut, IDS_ERROR_NO_ERROR);

    return result;
}