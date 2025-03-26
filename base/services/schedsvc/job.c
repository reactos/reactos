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
FILETIME NextJobStartTime;
BOOL bValidNextJobStartTime = FALSE;


static WORD wDaysArray[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


/* FUNCTIONS *****************************************************************/

VOID
GetNextJobTimeout(HANDLE hTimer)
{
    PLIST_ENTRY CurrentEntry;
    FILETIME DueTime;
    PJOB CurrentJob;

    bValidNextJobStartTime = FALSE;
    CurrentEntry = JobListHead.Flink;
    while (CurrentEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(CurrentEntry, JOB, JobEntry);

        if (bValidNextJobStartTime == FALSE)
        {
            CopyMemory(&NextJobStartTime, &CurrentJob->StartTime, sizeof(FILETIME));
            bValidNextJobStartTime = TRUE;
        }
        else
        {
            if (CompareFileTime(&NextJobStartTime, &CurrentJob->StartTime) > 0)
                CopyMemory(&NextJobStartTime, &CurrentJob->StartTime, sizeof(FILETIME));
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    if (bValidNextJobStartTime == FALSE)
    {
        TRACE("No valid job!\n");
        return;
    }

    LocalFileTimeToFileTime(&DueTime, &NextJobStartTime);

    SetWaitableTimer(hTimer,
                     (PLARGE_INTEGER)&DueTime,
                     0,
                     NULL,
                     NULL,
                     TRUE);
}

#if 0
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
#endif

VOID
RunCurrentJobs(VOID)
{
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFOW StartupInfo;
    PLIST_ENTRY CurrentEntry;
    PJOB CurrentJob;
    BOOL bRet;

    CurrentEntry = JobListHead.Flink;
    while (CurrentEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(CurrentEntry, JOB, JobEntry);

        if (CompareFileTime(&NextJobStartTime, &CurrentJob->StartTime) == 0)
        {
            TRACE("Run job %ld: %S\n", CurrentJob->JobId, CurrentJob->Command);

            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof(StartupInfo);
            StartupInfo.lpTitle = CurrentJob->Command;
            StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
            StartupInfo.wShowWindow = SW_SHOWDEFAULT;

            if ((CurrentJob->Flags & JOB_NONINTERACTIVE) == 0)
            {
                StartupInfo.dwFlags |= STARTF_INHERITDESKTOP;
                StartupInfo.lpDesktop = L"WinSta0\\Default";
            }

            bRet = CreateProcessW(NULL,
                                  CurrentJob->Command,
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
        }

        CurrentEntry = CurrentEntry->Flink;
    }
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
    ULARGE_INTEGER LocalStartTime;

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

    LocalStartTime.u.LowPart = StartFileTime.dwLowDateTime;
    LocalStartTime.u.HighPart = StartFileTime.dwHighDateTime;
    if (bDaysOffsetValid && wDaysOffset != 0)
    {
        LocalStartTime.QuadPart += ((ULONGLONG)wDaysOffset * 24 * 60 * 60 * 10000);
    }

    pJob->StartTime.dwLowDateTime = LocalStartTime.u.LowPart;
    pJob->StartTime.dwHighDateTime = LocalStartTime.u.HighPart;
}

#if 0
VOID
DumpStartList(
    _In_ PLIST_ENTRY StartListHead)
{
    PLIST_ENTRY CurrentEntry;
    PJOB CurrentJob;

    CurrentEntry = JobListHead->Flink;
    while (CurrentEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(CurrentEntry, JOB, StartEntry);

        TRACE("%3lu: %016I64x\n", CurrentJob->JobId, CurrentJob->StartTime.QuadPart);

        CurrentEntry = CurrentEntry->Flink;
    }
}
#endif
/* EOF */
