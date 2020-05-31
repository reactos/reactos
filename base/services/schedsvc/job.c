/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/schedsvc/job.c
 * PURPOSE:          Scheduling service
 * PROGRAMMER:       Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);


/* GLOBALS ******************************************************************/

typedef struct _SCHEDULE
{
    DWORD JobTime;
    DWORD DaysOfMonth;
    UCHAR DaysOfWeek;
    UCHAR Flags;
    WORD Reserved;
} SCHEDULE, PSCHEDULE;

DWORD dwNextJobId = 0;
DWORD dwJobCount = 0;
LIST_ENTRY JobListHead;
RTL_RESOURCE JobListLock;

LIST_ENTRY StartListHead;
RTL_RESOURCE StartListLock;

static WORD wDaysArray[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


/* FUNCTIONS *****************************************************************/

DWORD
GetNextJobTimeout(VOID)
{
    FILETIME FileTime;
    SYSTEMTIME SystemTime;
    ULARGE_INTEGER CurrentTime, Timeout;
    PJOB pNextJob;

    if (IsListEmpty(&StartListHead))
    {
        TRACE("No job in list! Wait until next update.\n");
        return INFINITE;
    }

    pNextJob = CONTAINING_RECORD((&StartListHead)->Flink, JOB, StartEntry);

    FileTime.dwLowDateTime = pNextJob->StartTime.u.LowPart;
    FileTime.dwHighDateTime = pNextJob->StartTime.u.HighPart;
    FileTimeToSystemTime(&FileTime, &SystemTime);

    TRACE("Start next job (%lu) at %02hu:%02hu %02hu.%02hu.%hu\n",
          pNextJob->JobId, SystemTime.wHour, SystemTime.wMinute,
          SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear);

    GetLocalTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &FileTime);

    CurrentTime.u.LowPart = FileTime.dwLowDateTime;
    CurrentTime.u.HighPart = FileTime.dwHighDateTime;

    if (CurrentTime.QuadPart >= pNextJob->StartTime.QuadPart)
    {
        TRACE("Next event has already gone by!\n");
        return 0;
    }

    Timeout.QuadPart = (pNextJob->StartTime.QuadPart - CurrentTime.QuadPart) / 10000;
    if (Timeout.u.HighPart != 0)
    {
        TRACE("Event happens too far in the future!\n");
        return INFINITE;
    }

    TRACE("Timeout: %lu\n", Timeout.u.LowPart);
    return Timeout.u.LowPart;
}


static
VOID
ReScheduleJob(
    PJOB pJob)
{
    /* Remove the job from the start list */
    RemoveEntryList(&pJob->StartEntry);

    /* Non-periodical job, remove it */
    if ((pJob->Flags & JOB_RUN_PERIODICALLY) == 0)
    {
        /* Remove the job from the registry */
        DeleteJob(pJob);

        /* Remove the job from the job list */
        RemoveEntryList(&pJob->JobEntry);
        dwJobCount--;

        /* Free the job object */
        HeapFree(GetProcessHeap(), 0, pJob);
        return;
    }

    /* Calculate the next start time */
    CalculateNextStartTime(pJob);

    /* Insert the job into the start list again */
    InsertJobIntoStartList(&StartListHead, pJob);
#if 0
    DumpStartList(&StartListHead);
#endif
}


VOID
RunNextJob(VOID)
{
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFOW StartupInfo;
    BOOL bRet;
    PJOB pNextJob;

    if (IsListEmpty(&StartListHead))
    {
        ERR("No job in list!\n");
        return;
    }

    pNextJob = CONTAINING_RECORD((&StartListHead)->Flink, JOB, StartEntry);

    TRACE("Run job %ld: %S\n", pNextJob->JobId, pNextJob->Command);

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpTitle = pNextJob->Command;
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_SHOWDEFAULT;

    if ((pNextJob->Flags & JOB_NONINTERACTIVE) == 0)
    {
        StartupInfo.dwFlags |= STARTF_INHERITDESKTOP;
        StartupInfo.lpDesktop = L"WinSta0\\Default";
    }

    bRet = CreateProcessW(NULL,
                          pNextJob->Command,
                          NULL,
                          NULL,
                          FALSE,
                          CREATE_NEW_CONSOLE,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInformation);
    if (bRet == FALSE)
    {
        ERR("CreateProcessW() failed (Error %lu)\n", GetLastError());
    }
    else
    {
        CloseHandle(ProcessInformation.hThread);
        CloseHandle(ProcessInformation.hProcess);
    }

    ReScheduleJob(pNextJob);
}


static
VOID
GetJobName(
    HKEY hJobsKey,
    PWSTR pszJobName)
{
    WCHAR szNameBuffer[JOB_NAME_LENGTH];
    FILETIME SystemTime;
    ULONG ulSeed, ulValue;
    HKEY hKey;
    LONG lError;

    GetSystemTimeAsFileTime(&SystemTime);
    ulSeed = SystemTime.dwLowDateTime;
    for (;;)
    {
        ulValue = RtlRandomEx(&ulSeed);
        swprintf(szNameBuffer, L"%08lx", ulValue);

        hKey = NULL;
        lError = RegOpenKeyEx(hJobsKey,
                              szNameBuffer,
                              0,
                              KEY_READ,
                              &hKey);
        if (lError != ERROR_SUCCESS)
        {
            wcscpy(pszJobName, szNameBuffer);
            return;
        }

        RegCloseKey(hKey);
    }
}


LONG
SaveJob(
    _In_ PJOB pJob)
{
    SCHEDULE Schedule;
    HKEY hJobsKey = NULL, hJobKey = NULL;
    LONG lError;

    TRACE("SaveJob()\n");

    lError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Services\\Schedule\\Jobs",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hJobsKey,
                             NULL);
    if (lError != ERROR_SUCCESS)
        goto done;

    GetJobName(hJobsKey, pJob->Name);

    lError = RegCreateKeyExW(hJobsKey,
                             pJob->Name,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hJobKey,
                             NULL);
    if (lError != ERROR_SUCCESS)
        goto done;

    Schedule.JobTime = pJob->JobTime;
    Schedule.DaysOfMonth = pJob->DaysOfMonth;
    Schedule.DaysOfWeek = pJob->DaysOfWeek;
    Schedule.Flags = pJob->Flags;

    lError = RegSetValueEx(hJobKey,
                           L"Schedule",
                           0,
                           REG_BINARY,
                           (PBYTE)&Schedule,
                           sizeof(Schedule));
    if (lError != ERROR_SUCCESS)
        goto done;

    lError = RegSetValueEx(hJobKey,
                           L"Command",
                           0,
                           REG_SZ,
                           (PBYTE)pJob->Command,
                           (wcslen(pJob->Command) + 1) * sizeof(WCHAR));
    if (lError != ERROR_SUCCESS)
        goto done;

done:
    if (hJobKey != NULL)
        RegCloseKey(hJobKey);

    if (hJobsKey != NULL)
        RegCloseKey(hJobsKey);

    return lError;
}


LONG
DeleteJob(
    _In_ PJOB pJob)
{
    HKEY hJobsKey = NULL;
    LONG lError;

    TRACE("DeleteJob()\n");

    lError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Services\\Schedule\\Jobs",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hJobsKey,
                             NULL);
    if (lError != ERROR_SUCCESS)
        goto done;

    lError = RegDeleteKey(hJobsKey,
                          pJob->Name);
    if (lError != ERROR_SUCCESS)
        goto done;

done:
    if (hJobsKey != NULL)
        RegCloseKey(hJobsKey);

    return lError;
}


LONG
LoadJobs(VOID)
{
    SCHEDULE Schedule;
    WCHAR szNameBuffer[JOB_NAME_LENGTH];
    DWORD dwNameLength, dwIndex, dwSize;
    HKEY hJobsKey = NULL, hJobKey = NULL;
    PJOB pJob = NULL;
    LONG lError;

    TRACE("LoadJobs()\n");

    lError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Services\\Schedule\\Jobs",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_READ,
                             NULL,
                             &hJobsKey,
                             NULL);
    if (lError != ERROR_SUCCESS)
        goto done;

    for (dwIndex = 0; dwIndex < 1000; dwIndex++)
    {
        dwNameLength = JOB_NAME_LENGTH;
        lError = RegEnumKeyEx(hJobsKey,
                              dwIndex,
                              szNameBuffer,
                              &dwNameLength,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
        if (lError != ERROR_SUCCESS)
        {
            lError = ERROR_SUCCESS;
            break;
        }

        TRACE("KeyName: %S\n", szNameBuffer);

        lError = RegOpenKeyEx(hJobsKey,
                              szNameBuffer,
                              0,
                              KEY_READ,
                              &hJobKey);
        if (lError != ERROR_SUCCESS)
            break;

        dwSize = sizeof(SCHEDULE);
        lError = RegQueryValueEx(hJobKey,
                                 L"Schedule",
                                 NULL,
                                 NULL,
                                 (PBYTE)&Schedule,
                                 &dwSize);
        if (lError == ERROR_SUCCESS)
        {
            dwSize = 0;
            RegQueryValueEx(hJobKey,
                            L"Command",
                            NULL,
                            NULL,
                            NULL,
                            &dwSize);
            if (dwSize != 0)
            {
                /* Allocate a new job object */
                pJob = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(JOB) + dwSize - sizeof(WCHAR));
                if (pJob == NULL)
                {
                    lError = ERROR_OUTOFMEMORY;
                    break;
                }

                lError = RegQueryValueEx(hJobKey,
                                         L"Command",
                                         NULL,
                                         NULL,
                                         (PBYTE)pJob->Command,
                                         &dwSize);
                if (lError != ERROR_SUCCESS)
                    break;

                wcscpy(pJob->Name, szNameBuffer);
                pJob->JobTime = Schedule.JobTime;
                pJob->DaysOfMonth = Schedule.DaysOfMonth;
                pJob->DaysOfWeek = Schedule.DaysOfWeek;
                pJob->Flags = Schedule.Flags;

                /* Acquire the job list lock exclusively */
                RtlAcquireResourceExclusive(&JobListLock, TRUE);

                /* Assign a new job ID */
                pJob->JobId = dwNextJobId++;
                dwJobCount++;

                /* Append the new job to the job list */
                InsertTailList(&JobListHead, &pJob->JobEntry);

                /* Calculate the next start time */
                CalculateNextStartTime(pJob);

                /* Insert the job into the start list */
                InsertJobIntoStartList(&StartListHead, pJob);
#if 0
                DumpStartList(&StartListHead);
#endif

                /* Release the job list lock */
                RtlReleaseResource(&JobListLock);

                pJob = NULL;
            }
        }

        RegCloseKey(hJobKey);
        hJobKey = NULL;
    }

done:
    if (pJob != NULL)
        HeapFree(GetProcessHeap(), 0, pJob);

    if (hJobKey != NULL)
        RegCloseKey(hJobKey);

    if (hJobsKey != NULL)
        RegCloseKey(hJobsKey);

    return lError;
}


static
WORD
DaysOfMonth(
    WORD wMonth,
    WORD wYear)
{
    if (wMonth == 2 && wYear % 4 == 0 && wYear % 400 != 0)
        return 29;

    return wDaysArray[wMonth];
}


VOID
CalculateNextStartTime(
    _In_ PJOB pJob)
{
    SYSTEMTIME CurrentSystemTime, StartSystemTime;
    FILETIME StartFileTime;
    WORD wDaysOffset, wTempOffset, i, wJobDayOfWeek, wJobDayOfMonth;
    DWORD_PTR CurrentTimeMs;
    BOOL bDaysOffsetValid;

    TRACE("CalculateNextStartTime(%p)\n", pJob);
    TRACE("JobTime: %lu\n", pJob->JobTime);
    TRACE("DaysOfWeek: 0x%x\n", pJob->DaysOfWeek);
    TRACE("DaysOfMonth: 0x%x\n", pJob->DaysOfMonth);

    GetLocalTime(&CurrentSystemTime);

    CurrentTimeMs = (DWORD_PTR)CurrentSystemTime.wHour * 3600000 +
                    (DWORD_PTR)CurrentSystemTime.wMinute * 60000;

    bDaysOffsetValid = FALSE;
    wDaysOffset = 0;
    if ((pJob->DaysOfWeek == 0) && (pJob->DaysOfMonth == 0))
    {
        if (CurrentTimeMs >= pJob->JobTime)
        {
            TRACE("Tomorrow!\n");
            wDaysOffset = 1;
        }

        bDaysOffsetValid = TRUE;
    }
    else
    {
        if (pJob->DaysOfWeek != 0)
        {
            TRACE("DaysOfWeek!\n");
            for (i = 0; i < 7; i++)
            {
                if (pJob->DaysOfWeek & (1 << i))
                {
                    /* Adjust the range */
                    wJobDayOfWeek = (i + 1) % 7;
                    TRACE("wJobDayOfWeek: %hu\n", wJobDayOfWeek);
                    TRACE("CurrentSystemTime.wDayOfWeek: %hu\n", CurrentSystemTime.wDayOfWeek);

                    /* Calculate the days offset */
                    if ((CurrentSystemTime.wDayOfWeek > wJobDayOfWeek ) ||
                        ((CurrentSystemTime.wDayOfWeek == wJobDayOfWeek) && (CurrentTimeMs >= pJob->JobTime)))
                    {
                        wTempOffset = 7 - CurrentSystemTime.wDayOfWeek + wJobDayOfWeek;
                        TRACE("wTempOffset: %hu\n", wTempOffset);
                    }
                    else
                    {
                        wTempOffset = wJobDayOfWeek - CurrentSystemTime.wDayOfWeek;
                        TRACE("wTempOffset: %hu\n", wTempOffset);
                    }

                    /* Use the smallest offset */
                    if (bDaysOffsetValid == FALSE)
                    {
                        wDaysOffset = wTempOffset;
                        bDaysOffsetValid = TRUE;
                    }
                    else
                    {
                        if (wTempOffset < wDaysOffset)
                            wDaysOffset = wTempOffset;
                    }
                }
            }
        }

        if (pJob->DaysOfMonth != 0)
        {
            FIXME("Support DaysOfMonth!\n");
            for (i = 0; i < 31; i++)
            {
                if (pJob->DaysOfMonth & (1 << i))
                {
                    /* Adjust the range */
                    wJobDayOfMonth = i + 1;
                    FIXME("wJobDayOfMonth: %hu\n", wJobDayOfMonth);
                    FIXME("CurrentSystemTime.wDay: %hu\n", CurrentSystemTime.wDay);

                    if ((CurrentSystemTime.wDay > wJobDayOfMonth) ||
                        ((CurrentSystemTime.wDay == wJobDayOfMonth) && (CurrentTimeMs >= pJob->JobTime)))
                    {
                        wTempOffset = DaysOfMonth(CurrentSystemTime.wMonth, CurrentSystemTime.wYear) -
                                      CurrentSystemTime.wDay + wJobDayOfMonth;
                        FIXME("wTempOffset: %hu\n", wTempOffset);
                    }
                    else
                    {
                        wTempOffset = wJobDayOfMonth - CurrentSystemTime.wDay;
                        FIXME("wTempOffset: %hu\n", wTempOffset);
                    }

                    /* Use the smallest offset */
                    if (bDaysOffsetValid == FALSE)
                    {
                        wDaysOffset = wTempOffset;
                        bDaysOffsetValid = TRUE;
                    }
                    else
                    {
                        if (wTempOffset < wDaysOffset)
                            wDaysOffset = wTempOffset;
                    }
                }
            }
        }
    }

    TRACE("wDaysOffset: %hu\n", wDaysOffset);

    CopyMemory(&StartSystemTime, &CurrentSystemTime, sizeof(SYSTEMTIME));

    StartSystemTime.wMilliseconds = 0;
    StartSystemTime.wSecond = 0;
    StartSystemTime.wHour = (WORD)(pJob->JobTime / 3600000);
    StartSystemTime.wMinute = (WORD)((pJob->JobTime % 3600000) / 60000);

    SystemTimeToFileTime(&StartSystemTime, &StartFileTime);

    pJob->StartTime.u.LowPart = StartFileTime.dwLowDateTime;
    pJob->StartTime.u.HighPart = StartFileTime.dwHighDateTime;
    if (bDaysOffsetValid && wDaysOffset != 0)
    {
        pJob->StartTime.QuadPart += ((ULONGLONG)wDaysOffset * 24 * 60 * 60 * 10000);
    }
}


VOID
InsertJobIntoStartList(
    _In_ PLIST_ENTRY StartListHead,
    _In_ PJOB pJob)
{
    PLIST_ENTRY CurrentEntry, PreviousEntry;
    PJOB CurrentJob;

    if (IsListEmpty(StartListHead))
    {
         InsertHeadList(StartListHead, &pJob->StartEntry);
         return;
    }

    CurrentEntry = StartListHead->Flink;
    while (CurrentEntry != StartListHead)
    {
        CurrentJob = CONTAINING_RECORD(CurrentEntry, JOB, StartEntry);

        if ((CurrentEntry == StartListHead->Flink) &&
            (pJob->StartTime.QuadPart < CurrentJob->StartTime.QuadPart))
        {
            /* Insert before the first entry */
            InsertHeadList(StartListHead, &pJob->StartEntry);
            return;
        }

        if (pJob->StartTime.QuadPart < CurrentJob->StartTime.QuadPart)
        {
            /* Insert between the previous and the current entry */
            PreviousEntry = CurrentEntry->Blink;
            pJob->StartEntry.Blink = PreviousEntry;
            pJob->StartEntry.Flink = CurrentEntry;
            PreviousEntry->Flink = &pJob->StartEntry;
            CurrentEntry->Blink = &pJob->StartEntry;
            return;
        }

        if ((CurrentEntry->Flink == StartListHead) &&
            (pJob->StartTime.QuadPart >= CurrentJob->StartTime.QuadPart))
        {
            /* Insert after the last entry */
            InsertTailList(StartListHead, &pJob->StartEntry);
            return;
        }

        CurrentEntry = CurrentEntry->Flink;
    }
}


VOID
DumpStartList(
    _In_ PLIST_ENTRY StartListHead)
{
    PLIST_ENTRY CurrentEntry;
    PJOB CurrentJob;

    CurrentEntry = StartListHead->Flink;
    while (CurrentEntry != StartListHead)
    {
        CurrentJob = CONTAINING_RECORD(CurrentEntry, JOB, StartEntry);

        TRACE("%3lu: %016I64x\n", CurrentJob->JobId, CurrentJob->StartTime.QuadPart);

        CurrentEntry = CurrentEntry->Flink;
    }
}

/* EOF */
