/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdStatistics.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"

static
INT
DisplayServerStatistics(VOID)
{
    PSERVER_INFO_100 ServerInfo = NULL;
    PSTAT_SERVER_0 StatisticsInfo = NULL;
    LARGE_INTEGER LargeValue;
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

    ConResPrintf(StdOut, IDS_STATISTICS_SERVER_NAME, ServerInfo->sv100_name);

    printf("Statistik since %lu\n\n\n", StatisticsInfo->sts0_start);

    printf("Sessions accepted %lu\n", StatisticsInfo->sts0_sopens);
    printf("Sessions timed-out %lu\n", StatisticsInfo->sts0_stimedout);
    printf("Sessions errored-out %lu\n\n", StatisticsInfo->sts0_serrorout);

    LargeValue.u.LowPart = StatisticsInfo->sts0_bytessent_low;
    LargeValue.u.HighPart = StatisticsInfo->sts0_bytessent_high;
    printf("Kilobytes sent %I64u\n", LargeValue.QuadPart / 1024);

    LargeValue.u.LowPart = StatisticsInfo->sts0_bytesrcvd_low;
    LargeValue.u.HighPart = StatisticsInfo->sts0_bytesrcvd_high;
    printf("Kilobytes received %I64u\n\n", LargeValue.QuadPart / 1024);

    printf("Mean response time (msec) %lu\n\n", StatisticsInfo->sts0_avresponse);

    printf("System errors %lu\n", StatisticsInfo->sts0_syserrors);
    printf("Permission violations %lu\n", StatisticsInfo->sts0_permerrors);
    printf("Password violations %lu\n\n", StatisticsInfo->sts0_pwerrors);

    printf("Files accessed %lu\n", StatisticsInfo->sts0_fopens);
    printf("Communication devices accessed %lu\n", StatisticsInfo->sts0_devopens);
    printf("Print jobs spooled %lu\n\n", StatisticsInfo->sts0_jobsqueued);

    printf("Times buffers exhausted\n\n");
    printf("  Big buffers %lu\n", StatisticsInfo->sts0_bigbufneed);
    printf("  Request buffers %lu\n\n", StatisticsInfo->sts0_reqbufneed);

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

    ConResPrintf(StdOut, IDS_STATISTICS_WORKSTATION_NAME, WorkstationInfo->wki100_computername);

    printf("Statistik since %I64u\n\n\n", StatisticsInfo->StatisticsStartTime.QuadPart);

    printf("Bytes received %I64u\n", StatisticsInfo->BytesReceived.QuadPart);
    printf("Server Message Blocks (SMBs) received %I64u\n", StatisticsInfo->SmbsReceived.QuadPart);
    printf("Bytes transmitted %I64u\n", StatisticsInfo->BytesTransmitted.QuadPart);
    printf("Server Message Blocks (SMBs) transmitted %I64u\n", StatisticsInfo->SmbsTransmitted.QuadPart);
    printf("Read operations %lu\n", StatisticsInfo->ReadOperations);
    printf("Write operations %lu\n", StatisticsInfo->WriteOperations);
    printf("Raw reads denied %lu\n", StatisticsInfo->RawReadsDenied);
    printf("Raw writes denied %lu\n\n", StatisticsInfo->RawWritesDenied);

    printf("Network errors %lu\n", StatisticsInfo->NetworkErrors);
    printf("Connections made %lu\n", StatisticsInfo->CoreConnects +
                                     StatisticsInfo->Lanman20Connects +
                                     StatisticsInfo->Lanman21Connects +
                                     StatisticsInfo->LanmanNtConnects);
    printf("Reconnections made %lu\n", StatisticsInfo->Reconnects);
    printf("Server disconnects %lu\n\n", StatisticsInfo->ServerDisconnects);

    printf("Sessions started %lu\n", StatisticsInfo->Sessions);
    printf("Hung sessions %lu\n", StatisticsInfo->HungSessions);
    printf("Failed sessions %lu\n", StatisticsInfo->FailedSessions);
    printf("Failed operations %lu\n", StatisticsInfo->InitiallyFailedOperations +
                                      StatisticsInfo->FailedCompletionOperations);
    printf("Use count %lu\n", StatisticsInfo->UseCount);
    printf("Failed use count %lu\n\n", StatisticsInfo->FailedUseCount);

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