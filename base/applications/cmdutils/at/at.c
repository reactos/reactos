/*
 * PROJECT:     ReactOS AT utility
 * COPYRIGHT:   See COPYING in the top level directory
 * FILE:        base/applications/cmdutils/at/at.c
 * PURPOSE:     ReactOS AT utility
 * PROGRAMMERS: Eric Kohl <eric.kohl@reactos.org>
 */

#include <stdlib.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wincon.h>
#include <winnls.h>
#include <lm.h>

#include <conutils.h>

#include "resource.h"


PWSTR pszDaysOfWeekArray[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};


static
VOID
FreeDaysOfWeekArray(VOID)
{
    INT i;

    for (i = 0; i < 7; i++)
    {
        if (pszDaysOfWeekArray[i] != NULL)
            HeapFree(GetProcessHeap(), 0, pszDaysOfWeekArray[i]);
    }
}


static
BOOL
InitDaysOfWeekArray(VOID)
{
    INT i, nLength;

    for (i = 0; i < 7; i++)
    {
        nLength = GetLocaleInfo(LOCALE_USER_DEFAULT,
                                LOCALE_SABBREVDAYNAME1 + i,
                                NULL,
                                0);

        pszDaysOfWeekArray[i] = HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          nLength * sizeof(WCHAR));
        if (pszDaysOfWeekArray[i] == NULL)
        {
            FreeDaysOfWeekArray();
            return FALSE;
        }

        GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_SABBREVDAYNAME1 + i,
                      pszDaysOfWeekArray[i],
                      nLength);
    }

    return TRUE;
}


static
BOOL
ParseTime(
    PWSTR pszTime,
    PULONG pulJobHour,
    PULONG pulJobMinute)
{
    WCHAR szHour[3], szMinute[3], szAmPm[5];
    PWSTR startPtr, endPtr;
    ULONG ulHour = 0, ulMinute = 0;
    INT nLength;

    if (pszTime == NULL)
        return FALSE;

    startPtr = pszTime;

    /* Extract the hour string */
    nLength = 0;
    while (*startPtr != L'\0' && iswdigit(*startPtr))
    {
        if (nLength >= 2)
            return FALSE;

        szHour[nLength] = *startPtr;
        nLength++;

        startPtr++;
    }
    szHour[nLength] = L'\0';

    /* Check for a valid time separator */
    if (*startPtr != L':')
        return FALSE;

    /* Skip the time separator */
    startPtr++;

    /* Extract the minute string */
    nLength = 0;
    while (*startPtr != L'\0' && iswdigit(*startPtr))
    {
        if (nLength >= 2)
            return FALSE;

        szMinute[nLength] = *startPtr;
        nLength++;

        startPtr++;
    }
    szMinute[nLength] = L'\0';

    /* Extract the optional AM/PM indicator string */
    nLength = 0;
    while (*startPtr != L'\0')
    {
        if (nLength >= 4)
            return FALSE;

        if (!iswspace(*startPtr))
        {
            szAmPm[nLength] = *startPtr;
            nLength++;
        }

        startPtr++;
    }
    szAmPm[nLength] = L'\0';

    /* Convert the hour string */
    ulHour = wcstoul(szHour, &endPtr, 10);
    if (ulHour == 0 && *endPtr != UNICODE_NULL)
        return FALSE;

    /* Convert the minute string */
    ulMinute = wcstoul(szMinute, &endPtr, 10);
    if (ulMinute == 0 && *endPtr != UNICODE_NULL)
        return FALSE;

    /* Check for valid AM/PM indicator */
    if (wcslen(szAmPm) > 0 &&
        _wcsicmp(szAmPm, L"a") != 0 &&
        _wcsicmp(szAmPm, L"am") != 0 &&
        _wcsicmp(szAmPm, L"p") != 0 &&
        _wcsicmp(szAmPm, L"pm") != 0)
        return FALSE;

    /* Check for the valid minute range [0-59] */
    if (ulMinute > 59)
        return FALSE;

    if (wcslen(szAmPm) > 0)
    {
        /* 12 hour time format */

         /* Check for the valid hour range [1-12] */
        if (ulHour == 0 || ulHour > 12)
            return FALSE;

        /* Convert 12 hour format to 24 hour format */
        if (_wcsicmp(szAmPm, L"a") == 0 ||
            _wcsicmp(szAmPm, L"am") == 0)
        {
            if (ulHour == 12)
                ulHour = 0;
        }
        else
        {
            if (ulHour >= 1 && ulHour <= 11)
                ulHour += 12;
        }
    }
    else
    {
        /* 24 hour time format */

        /* Check for the valid hour range [0-23] */
        if (ulHour > 23)
            return FALSE;
    }

    if (pulJobHour != NULL)
        *pulJobHour = ulHour;

    if (pulJobMinute != NULL)
        *pulJobMinute = ulMinute;

    return TRUE;
}


static
BOOL
ParseId(
    PWSTR pszId,
    PULONG pulId)
{
    PWSTR startPtr, endPtr;
    ULONG ulId = 0;
    BOOL bResult = FALSE;

    startPtr = pszId;
    endPtr = NULL;
    ulId = wcstoul(startPtr, &endPtr, 10);
    if (endPtr != NULL && *endPtr == UNICODE_NULL)
    {
        bResult = TRUE;

        if (pulId != NULL)
            *pulId = ulId;
    }

    return bResult;
}


static
BOOL
ParseDaysOfMonth(
    PWSTR pszBuffer,
    PULONG pulDaysOfMonth)
{
    PWSTR startPtr, endPtr;
    ULONG ulValue;

    if (wcslen(pszBuffer) == 0)
        return FALSE;

    startPtr = pszBuffer;
    endPtr = NULL;
    for (;;)
    {
        ulValue = wcstoul(startPtr, &endPtr, 10);
        if (ulValue == 0)
            return FALSE;

        if (ulValue > 0 && ulValue <= 31)
            *pulDaysOfMonth |= (1 << (ulValue - 1));

        if (endPtr != NULL && *endPtr == UNICODE_NULL)
            return TRUE;

        startPtr = endPtr + 1;
        endPtr = NULL;
    }

    return FALSE;
}


static
BOOL
ParseDaysOfWeek(
    PWSTR pszBuffer,
    PUCHAR pucDaysOfWeek)
{
    PWSTR startPtr, endPtr;
    INT nLength, i;

    if (wcslen(pszBuffer) == 0)
        return FALSE;

    startPtr = pszBuffer;
    endPtr = NULL;
    for (;;)
    {
        endPtr = wcschr(startPtr, L',');
        if (endPtr == NULL)
            nLength = wcslen(startPtr);
        else
            nLength = (INT)((ULONG_PTR)endPtr - (ULONG_PTR)startPtr) / sizeof(WCHAR);

        for (i = 0; i < 7; i++)
        {
            if (nLength == wcslen(pszDaysOfWeekArray[i]) &&
                _wcsnicmp(startPtr, pszDaysOfWeekArray[i], nLength) == 0)
            {
                *pucDaysOfWeek |= (1 << i);
                break;
            }
        }

        if (endPtr == NULL)
            return TRUE;

        startPtr = endPtr + 1;
        endPtr = NULL;
    }

    return FALSE;
}


static
VOID
PrintErrorMessage(
    DWORD dwError)
{
    PWSTR pszBuffer = NULL;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   dwError,
                   0,
                   (PWSTR)&pszBuffer,
                   0,
                   NULL);

    ConPrintf(StdErr, L"%s\n", pszBuffer);
    LocalFree(pszBuffer);
}


static
VOID
PrintHorizontalLine(VOID)
{
    WCHAR szBuffer[80];
    INT i;

    for (i = 0; i < 79; i++)
        szBuffer[i] = L'-';
    szBuffer[79] = UNICODE_NULL;

    ConPrintf(StdOut, L"%s\n", szBuffer);
}


static
BOOL
Confirm(VOID)
{
    HINSTANCE hInstance;
    WCHAR szYesBuffer[8];
    WCHAR szNoBuffer[8];
    WCHAR szInput[80];
    DWORD dwOldMode;
    DWORD dwRead = 0;
    BOOL ret = FALSE;
    HANDLE hFile;

    hInstance = GetModuleHandleW(NULL);
    LoadStringW(hInstance, IDS_CONFIRM_YES, szYesBuffer, _countof(szYesBuffer));
    LoadStringW(hInstance, IDS_CONFIRM_NO, szNoBuffer, _countof(szNoBuffer));

    ZeroMemory(szInput, sizeof(szInput));

    hFile = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hFile, &dwOldMode);

    SetConsoleMode(hFile, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    for (;;)
    {
        ConResPrintf(StdOut, IDS_CONFIRM_QUESTION);

        ReadConsoleW(hFile, szInput, _countof(szInput), &dwRead, NULL);

        szInput[0] = towupper(szInput[0]);
        if (szInput[0] == szYesBuffer[0])
        {
            ret = TRUE;
            break;
        }
        else if (szInput[0] == 13 || szInput[0] == szNoBuffer[0])
        {
            ret = FALSE;
            break;
        }

        ConResPrintf(StdOut, IDS_CONFIRM_INVALID);
    }

    SetConsoleMode(hFile, dwOldMode);

    return ret;
}


static
DWORD_PTR
GetTimeAsJobTime(VOID)
{
    SYSTEMTIME Time;
    DWORD_PTR JobTime;

    GetLocalTime(&Time);

    JobTime = (DWORD_PTR)Time.wHour * 3600000 +
              (DWORD_PTR)Time.wMinute * 60000;

    return JobTime;
}


static
ULONG
GetCurrentDayOfMonth(VOID)
{
    SYSTEMTIME Time;

    GetLocalTime(&Time);

    return 1UL << (Time.wDay - 1);
}


static
VOID
JobTimeToTimeString(
    PWSTR pszBuffer,
    INT cchBuffer,
    WORD wHour,
    WORD wMinute)
{
    SYSTEMTIME Time = {0, 0, 0, 0, 0, 0, 0, 0};

    Time.wHour = wHour;
    Time.wMinute = wMinute;

    GetTimeFormat(LOCALE_USER_DEFAULT,
                  TIME_NOSECONDS,
                  &Time,
                  NULL,
                  pszBuffer,
                  cchBuffer);
}


static
INT
PrintJobDetails(
    PWSTR pszComputerName,
    ULONG ulJobId)
{
    PAT_INFO pBuffer = NULL;
    DWORD_PTR CurrentTime;
    WCHAR szStatusBuffer[16];
    WCHAR szScheduleBuffer[60];
    WCHAR szTimeBuffer[16];
    WCHAR szInteractiveBuffer[16];
    WCHAR szDateBuffer[8];
    INT i, nDateLength, nScheduleLength;
    HINSTANCE hInstance;
    NET_API_STATUS Status;

    Status = NetScheduleJobGetInfo(pszComputerName,
                                   ulJobId,
                                   (PBYTE *)&pBuffer);
    if (Status != NERR_Success)
    {
        PrintErrorMessage(Status);
        return 1;
    }

    hInstance = GetModuleHandle(NULL);

    if (pBuffer->Flags & JOB_EXEC_ERROR)
        LoadStringW(hInstance, IDS_ERROR, szStatusBuffer, _countof(szStatusBuffer));
    else
        LoadStringW(hInstance, IDS_OK, szStatusBuffer, _countof(szStatusBuffer));

    if (pBuffer->DaysOfMonth != 0)
    {
        if (pBuffer->Flags & JOB_RUN_PERIODICALLY)
            LoadStringW(hInstance, IDS_EVERY, szScheduleBuffer, _countof(szScheduleBuffer));
        else
            LoadStringW(hInstance, IDS_NEXT, szScheduleBuffer, _countof(szScheduleBuffer));

        nScheduleLength = wcslen(szScheduleBuffer);
        for (i = 0; i < 31; i++)
        {
            if (pBuffer->DaysOfMonth & (1 << i))
            {
                swprintf(szDateBuffer, L" %d", i + 1);
                nDateLength = wcslen(szDateBuffer);
                if (nScheduleLength + nDateLength <= 55)
                {
                    wcscat(szScheduleBuffer, szDateBuffer);
                    nScheduleLength += nDateLength;
                }
                else
                {
                    wcscat(szScheduleBuffer, L"...");
                    break;
                }
            }
        }
    }
    else if (pBuffer->DaysOfWeek != 0)
    {
        if (pBuffer->Flags & JOB_RUN_PERIODICALLY)
            LoadStringW(hInstance, IDS_EVERY, szScheduleBuffer, _countof(szScheduleBuffer));
        else
            LoadStringW(hInstance, IDS_NEXT, szScheduleBuffer, _countof(szScheduleBuffer));

        nScheduleLength = wcslen(szScheduleBuffer);
        for (i = 0; i < 7; i++)
        {
            if (pBuffer->DaysOfWeek & (1 << i))
            {
                swprintf(szDateBuffer, L" %s", pszDaysOfWeekArray[i]);
                nDateLength = wcslen(szDateBuffer);
                if (nScheduleLength + nDateLength <= 55)
                {
                    wcscat(szScheduleBuffer, szDateBuffer);
                    nScheduleLength += nDateLength;
                }
                else
                {
                    wcscat(szScheduleBuffer, L"...");
                    break;
                }
            }
        }
    }
    else
    {
        CurrentTime = GetTimeAsJobTime();
        if (CurrentTime > pBuffer->JobTime)
            LoadStringW(hInstance, IDS_TOMORROW, szScheduleBuffer, _countof(szScheduleBuffer));
        else
            LoadStringW(hInstance, IDS_TODAY, szScheduleBuffer, _countof(szScheduleBuffer));
    }

    JobTimeToTimeString(szTimeBuffer,
                        _countof(szTimeBuffer),
                        (WORD)(pBuffer->JobTime / 3600000),
                        (WORD)((pBuffer->JobTime % 3600000) / 60000));

    if (pBuffer->Flags & JOB_NONINTERACTIVE)
        LoadStringW(hInstance, IDS_NO, szInteractiveBuffer, _countof(szInteractiveBuffer));
    else
        LoadStringW(hInstance, IDS_YES, szInteractiveBuffer, _countof(szInteractiveBuffer));

    ConResPrintf(StdOut, IDS_TASKID, ulJobId);
    ConResPrintf(StdOut, IDS_STATUS, szStatusBuffer);
    ConResPrintf(StdOut, IDS_SCHEDULE, szScheduleBuffer);
    ConResPrintf(StdOut, IDS_TIME, szTimeBuffer);
    ConResPrintf(StdOut, IDS_INTERACTIVE, szInteractiveBuffer);
    ConResPrintf(StdOut, IDS_COMMAND, pBuffer->Command);

    NetApiBufferFree(pBuffer);

    return 0;
}


static
INT
PrintAllJobs(
    PWSTR pszComputerName)
{
    PAT_ENUM pBuffer = NULL;
    DWORD dwRead = 0, dwTotal = 0;
    DWORD dwResume = 0, i;
    DWORD_PTR CurrentTime;
    NET_API_STATUS Status;

    WCHAR szScheduleBuffer[32];
    WCHAR szTimeBuffer[16];
    WCHAR szDateBuffer[8];
    HINSTANCE hInstance;
    INT j, nDateLength, nScheduleLength;

    Status = NetScheduleJobEnum(pszComputerName,
                                (PBYTE *)&pBuffer,
                                MAX_PREFERRED_LENGTH,
                                &dwRead,
                                &dwTotal,
                                &dwResume);
    if (Status != NERR_Success)
    {
        PrintErrorMessage(Status);
        return 1;
    }

    if (dwTotal == 0)
    {
        ConResPrintf(StdOut, IDS_NO_ENTRIES);
        return 0;
    }

    ConResPrintf(StdOut, IDS_JOBS_LIST);
    PrintHorizontalLine();

    hInstance = GetModuleHandle(NULL);

    for (i = 0; i < dwRead; i++)
    {
        if (pBuffer[i].DaysOfMonth != 0)
        {
            if (pBuffer[i].Flags & JOB_RUN_PERIODICALLY)
                LoadStringW(hInstance, IDS_EVERY, szScheduleBuffer, _countof(szScheduleBuffer));
            else
                LoadStringW(hInstance, IDS_NEXT, szScheduleBuffer, _countof(szScheduleBuffer));

            nScheduleLength = wcslen(szScheduleBuffer);
            for (j = 0; j < 31; j++)
            {
                if (pBuffer[i].DaysOfMonth & (1 << j))
                {
                    swprintf(szDateBuffer, L" %d", j + 1);
                    nDateLength = wcslen(szDateBuffer);
                    if (nScheduleLength + nDateLength <= 19)
                    {
                        wcscat(szScheduleBuffer, szDateBuffer);
                        nScheduleLength += nDateLength;
                    }
                    else
                    {
                        wcscat(szScheduleBuffer, L"...");
                        break;
                    }
                }
            }
        }
        else if (pBuffer[i].DaysOfWeek != 0)
        {
            if (pBuffer[i].Flags & JOB_RUN_PERIODICALLY)
                LoadStringW(hInstance, IDS_EVERY, szScheduleBuffer, _countof(szScheduleBuffer));
            else
                LoadStringW(hInstance, IDS_NEXT, szScheduleBuffer, _countof(szScheduleBuffer));

            nScheduleLength = wcslen(szScheduleBuffer);
            for (j = 0; j < 7; j++)
            {
                if (pBuffer[i].DaysOfWeek & (1 << j))
                {
                    swprintf(szDateBuffer, L" %s", pszDaysOfWeekArray[j]);
                    nDateLength = wcslen(szDateBuffer);
                    if (nScheduleLength + nDateLength <= 55)
                    {
                        wcscat(szScheduleBuffer, szDateBuffer);
                        nScheduleLength += nDateLength;
                    }
                    else
                    {
                        wcscat(szScheduleBuffer, L"...");
                        break;
                    }
                }
            }
        }
        else
        {
            CurrentTime = GetTimeAsJobTime();
            if (CurrentTime > pBuffer[i].JobTime)
                LoadStringW(hInstance, IDS_TOMORROW, szScheduleBuffer, _countof(szScheduleBuffer));
            else
                LoadStringW(hInstance, IDS_TODAY, szScheduleBuffer, _countof(szScheduleBuffer));
        }

        JobTimeToTimeString(szTimeBuffer,
                            _countof(szTimeBuffer),
                            (WORD)(pBuffer[i].JobTime / 3600000),
                            (WORD)((pBuffer[i].JobTime % 3600000) / 60000));

        ConPrintf(StdOut,
                  L"   %6lu   %-21s   %-11s   %s\n",
                  pBuffer[i].JobId,
                  szScheduleBuffer,
                  szTimeBuffer,
                  pBuffer[i].Command);
    }

    NetApiBufferFree(pBuffer);

    return 0;
}


static
INT
AddJob(
    PWSTR pszComputerName,
    ULONG ulJobHour,
    ULONG ulJobMinute,
    ULONG ulDaysOfMonth,
    UCHAR ucDaysOfWeek,
    BOOL bInteractiveJob,
    BOOL bPeriodicJob,
    PWSTR pszCommand)
{
    AT_INFO InfoBuffer;
    ULONG ulJobId = 0;
    NET_API_STATUS Status;

    InfoBuffer.JobTime = (DWORD_PTR)ulJobHour * 3600000 +
                         (DWORD_PTR)ulJobMinute * 60000;
    InfoBuffer.DaysOfMonth = ulDaysOfMonth;
    InfoBuffer.DaysOfWeek = ucDaysOfWeek;
    InfoBuffer.Flags = (bInteractiveJob ? 0 : JOB_NONINTERACTIVE) |
                       (bPeriodicJob ? JOB_RUN_PERIODICALLY : 0);
    InfoBuffer.Command = pszCommand;

    Status = NetScheduleJobAdd(pszComputerName,
                               (PBYTE)&InfoBuffer,
                               &ulJobId);
    if (Status != NERR_Success)
    {
        PrintErrorMessage(Status);
        return 1;
    }

    ConResPrintf(StdOut, IDS_NEW_JOB, ulJobId);

    return 0;
}


static
INT
DeleteJob(
    PWSTR pszComputerName,
    ULONG ulJobId,
    BOOL bForceDelete)
{
    NET_API_STATUS Status;

    if (ulJobId == (ULONG)-1 && bForceDelete == FALSE)
    {
        ConResPrintf(StdOut, IDS_DELETE_ALL);
        if (!Confirm())
            return 0;
    }

    Status = NetScheduleJobDel(pszComputerName,
                               (ulJobId == (ULONG)-1) ? 0 : ulJobId,
                               (ulJobId == (ULONG)-1) ? -1 : ulJobId);
    if (Status != NERR_Success)
    {
        PrintErrorMessage(Status);
        return 1;
    }

    return 0;
}


int wmain(int argc, WCHAR **argv)
{
    PWSTR pszComputerName = NULL;
    PWSTR pszCommand = NULL;
    ULONG ulJobId = (ULONG)-1;
    ULONG ulJobHour = (ULONG)-1;
    ULONG ulJobMinute = (ULONG)-1;
    BOOL bDeleteJob = FALSE, bForceDelete = FALSE;
    BOOL bInteractiveJob = FALSE, bPeriodicJob = FALSE;
    BOOL bPrintUsage = FALSE;
    ULONG ulDaysOfMonth = 0;
    UCHAR ucDaysOfWeek = 0;
    INT nResult = 0;
    INT i, minIdx;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (!InitDaysOfWeekArray())
        return 1;

    /* Parse the computer name */
    i = 1;
    minIdx = 1;
    if (i < argc &&
        argv[i][0] == L'\\' &&
        argv[i][1] == L'\\')
    {
        pszComputerName = argv[i];
        i++;
        minIdx++;
    }

    /* Parse the time or job id */
    if (i < argc && argv[i][0] != L'/')
    {
        if (ParseTime(argv[i], &ulJobHour, &ulJobMinute))
        {
            i++;
            minIdx++;
        }
        else if (ParseId(argv[i], &ulJobId))
        {
            i++;
            minIdx++;
        }
    }

    /* Parse the options */
    for (; i < argc; i++)
    {
        if (argv[i][0] == L'/')
        {
            if (_wcsicmp(argv[i], L"/?") == 0)
            {
                bPrintUsage = TRUE;
                goto done;
            }
            else if (_wcsicmp(argv[i], L"/delete") == 0)
            {
                bDeleteJob = TRUE;
            }
            else if (_wcsicmp(argv[i], L"/yes") == 0)
            {
                bForceDelete = TRUE;
            }
            else if (_wcsicmp(argv[i], L"/interactive") == 0)
            {
                bInteractiveJob = TRUE;
            }
            else if (_wcsnicmp(argv[i], L"/every:", 7) == 0)
            {
                bPeriodicJob = TRUE;
                if (ParseDaysOfMonth(&(argv[i][7]), &ulDaysOfMonth) == FALSE)
                {
                    if (ParseDaysOfWeek(&(argv[i][7]), &ucDaysOfWeek) == FALSE)
                    {
                        ulDaysOfMonth = GetCurrentDayOfMonth();
                    }
                }
            }
            else if (_wcsnicmp(argv[i], L"/next:", 6) == 0)
            {
                bPeriodicJob = FALSE;
                if (ParseDaysOfMonth(&(argv[i][6]), &ulDaysOfMonth) == FALSE)
                {
                    if (ParseDaysOfWeek(&(argv[i][6]), &ucDaysOfWeek) == FALSE)
                    {
                        ulDaysOfMonth = GetCurrentDayOfMonth();
                    }
                }
            }
            else
            {
                bPrintUsage = TRUE;
                nResult = 1;
                goto done;
            }
        }
    }

    /* Parse the command */
    if (argc > minIdx && argv[argc - 1][0] != L'/')
    {
        pszCommand = argv[argc - 1];
    }

    if (bDeleteJob == TRUE)
    {
        /* Check for invalid options or arguments */
        if (bInteractiveJob == TRUE ||
            ulJobHour != (ULONG)-1 ||
            ulJobMinute != (ULONG)-1 ||
            ulDaysOfMonth != 0 ||
            ucDaysOfWeek != 0 ||
            pszCommand != NULL)
        {
            bPrintUsage = TRUE;
            nResult = 1;
            goto done;
        }

        nResult = DeleteJob(pszComputerName,
                            ulJobId,
                            bForceDelete);
    }
    else
    {
        if (ulJobHour != (ULONG)-1 && ulJobMinute != (ULONG)-1)
        {
            /* Check for invalid options or arguments */
            if (bForceDelete == TRUE ||
                pszCommand == NULL)
            {
                bPrintUsage = TRUE;
                nResult = 1;
                goto done;
            }

            nResult = AddJob(pszComputerName,
                             ulJobHour,
                             ulJobMinute,
                             ulDaysOfMonth,
                             ucDaysOfWeek,
                             bInteractiveJob,
                             bPeriodicJob,
                             pszCommand);
        }
        else
        {
            /* Check for invalid options or arguments */
            if (bForceDelete == TRUE ||
                bInteractiveJob == TRUE ||
                ulDaysOfMonth != 0 ||
                ucDaysOfWeek != 0 ||
                pszCommand != NULL)
            {
                bPrintUsage = TRUE;
                nResult = 1;
                goto done;
            }

            if (ulJobId == (ULONG)-1)
            {
                nResult = PrintAllJobs(pszComputerName);
            }
            else
            {
                nResult = PrintJobDetails(pszComputerName,
                                          ulJobId);
            }
        }
    }

done:
    FreeDaysOfWeekArray();

    if (bPrintUsage == TRUE)
        ConResPuts(StdOut, IDS_USAGE);

    return nResult;
}

/* EOF */
