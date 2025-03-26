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

    PrintMessageStringV(4624, ServerInfo->sv100_name);
    ConPrintf(StdOut, L"\n\n");

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

    PrintMessageString(4600);
    ConPrintf(StdOut, L" %d/%d/%d %d:%02d %s\n\n\n",
              SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
              wHour, SystemTime.wMinute,
              (SystemTime.wHour >= 1 && SystemTime.wHour < 13) ? L"AM" : L"PM");

    PrintPaddedMessageString(4601, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_sopens);

    PrintPaddedMessageString(4602, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_stimedout);

    PrintPaddedMessageString(4603, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_serrorout);

    LargeValue.u.LowPart = StatisticsInfo->sts0_bytessent_low;
    LargeValue.u.HighPart = StatisticsInfo->sts0_bytessent_high;
    PrintPaddedMessageString(4604, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", LargeValue.QuadPart / 1024);

    LargeValue.u.LowPart = StatisticsInfo->sts0_bytesrcvd_low;
    LargeValue.u.HighPart = StatisticsInfo->sts0_bytesrcvd_high;
    PrintPaddedMessageString(4605, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", LargeValue.QuadPart / 1024);

    PrintPaddedMessageString(4606, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_avresponse);

    PrintPaddedMessageString(4610, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_syserrors);

    PrintPaddedMessageString(4612, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_permerrors);

    PrintPaddedMessageString(4611, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_pwerrors);

    PrintPaddedMessageString(4608, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_fopens);

    PrintPaddedMessageString(4613, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_devopens);

    PrintPaddedMessageString(4609, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->sts0_jobsqueued);

    PrintMessageString(4620);
    ConPrintf(StdOut, L"\n");

    ConPrintf(StdOut, L"  ");
    PrintPaddedMessageString(4621, nPaddedLength - 2);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->sts0_bigbufneed);

    ConPrintf(StdOut, L"  ");
    PrintPaddedMessageString(4622, nPaddedLength - 2);
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

    PrintMessageStringV(4623, WorkstationInfo->wki100_computername);
    ConPrintf(StdOut, L"\n\n");

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

    PrintMessageString(4600);
    ConPrintf(StdOut, L" %d/%d/%d %d:%02d %s\n\n\n",
              SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
              wHour, SystemTime.wMinute,
              (SystemTime.wHour >= 1 && SystemTime.wHour < 13) ? L"AM" : L"PM");

    PrintPaddedMessageString(4630, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->BytesReceived.QuadPart);

    PrintPaddedMessageString(4631, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->SmbsReceived.QuadPart);

    PrintPaddedMessageString(4632, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->BytesTransmitted.QuadPart);

    PrintPaddedMessageString(4633, nPaddedLength);
    ConPrintf(StdOut, L"%I64u\n", StatisticsInfo->SmbsTransmitted.QuadPart);

    PrintPaddedMessageString(4634, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->ReadOperations);

    PrintPaddedMessageString(4635, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->WriteOperations);

    PrintPaddedMessageString(4636, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->RawReadsDenied);

    PrintPaddedMessageString(4637, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->RawWritesDenied);

    PrintPaddedMessageString(4638, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->NetworkErrors);

    PrintPaddedMessageString(4639, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->CoreConnects +
                                StatisticsInfo->Lanman20Connects +
                                StatisticsInfo->Lanman21Connects +
                                StatisticsInfo->LanmanNtConnects);

    PrintPaddedMessageString(4640, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->Reconnects);

    PrintPaddedMessageString(4641, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n\n", StatisticsInfo->ServerDisconnects);

    PrintPaddedMessageString(4642, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->Sessions);

    PrintPaddedMessageString(4643, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->HungSessions);

    PrintPaddedMessageString(4644, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->FailedSessions);

    PrintPaddedMessageString(4645, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->InitiallyFailedOperations +
                                StatisticsInfo->FailedCompletionOperations);

    PrintPaddedMessageString(4646, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", StatisticsInfo->UseCount);

    PrintPaddedMessageString(4647, nPaddedLength);
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
            PrintMessageString(4381);
            ConPuts(StdOut, L"\n");
            PrintNetMessage(MSG_STATISTICS_SYNTAX);
            return 0;
        }

        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            PrintMessageString(4381);
            ConPuts(StdOut, L"\n");
            PrintNetMessage(MSG_STATISTICS_SYNTAX);
            PrintNetMessage(MSG_STATISTICS_HELP);
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
        PrintMessageString(4379);
        ConPuts(StdOut, L"\n");
        ConPuts(StdOut, L"   Server\n");
        ConPuts(StdOut, L"   Workstation\n");
        ConPuts(StdOut, L"\n");
    }

    if (result == 0)
        PrintErrorMessage(ERROR_SUCCESS);

    return result;
}
