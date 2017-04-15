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


/* FUNCTIONS *****************************************************************/

static
VOID
GetJobName(
    HKEY hJobsKey,
    PWSTR pszJobName)
{
    WCHAR szNameBuffer[9];
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
    WCHAR szNameBuffer[32];
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
        dwNameLength = 32;
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

                // Cancel the start timer

                /* Append the new job to the job list */
                InsertTailList(&JobListHead, &pJob->JobEntry);

                /* Calculate the next start time */
                CalculateNextStartTime(pJob);

                /* Insert the job into the start list */
                InsertJobIntoStartList(&StartListHead, pJob);
#if 0
                DumpStartList(&StartListHead);
#endif

                // Update the start timer

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
    WORD wDaysArray[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (wMonth == 2 && wYear % 4 == 0 && wYear % 400 != 0)
        return 29;

    return wDaysArray[wMonth];
}


VOID
CalculateNextStartTime(PJOB pJob)
{
    SYSTEMTIME StartTime;
    FILETIME FileTime;
    DWORD_PTR Now;

    TRACE("CalculateNextStartTime(%p)\n", pJob);

    GetLocalTime(&StartTime);

    Now = (DWORD_PTR)StartTime.wHour * 3600000 +
          (DWORD_PTR)StartTime.wMinute * 60000;

    StartTime.wMilliseconds = 0;
    StartTime.wSecond = 0;
    StartTime.wHour = (WORD)(pJob->JobTime / 3600000);
    StartTime.wMinute = (WORD)((pJob->JobTime % 3600000) / 60000);

    /* Start the job tomorrow */
    if (Now > pJob->JobTime)
    {
        if (StartTime.wDay + 1 > DaysOfMonth(StartTime.wMonth, StartTime.wYear))
        {
            if (StartTime.wMonth == 12)
            {
                StartTime.wDay = 1;
                StartTime.wMonth = 1;
                StartTime.wYear++;
            }
            else
            {
                StartTime.wDay = 1;
                StartTime.wMonth++;
            }
        }
        else
        {
            StartTime.wDay++;
        }
    }

    TRACE("Next start: %02hu:%02hu %02hu.%02hu.%hu\n", StartTime.wHour,
          StartTime.wMinute, StartTime.wDay, StartTime.wMonth, StartTime.wYear);

    SystemTimeToFileTime(&StartTime, &FileTime);
    pJob->StartTime.u.LowPart = FileTime.dwLowDateTime;
    pJob->StartTime.u.HighPart = FileTime.dwHighDateTime;
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
