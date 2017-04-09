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

                /* Append the new job to the job list */
                InsertTailList(&JobListHead, &pJob->JobEntry);

                /* Release the job list lock */
                RtlReleaseResource(&JobListLock);

                // Calculate start time

                // Insert job into the start list

                // Update the start timer

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


#if 0
VOID
CalculateNextStartTime(PJOB pJob)
{
    SYSTEMTIME Time;
    DWORD_PTR JobTime;
    WORD wDay;
    BOOL bToday = FALSE;

    GetLocalTime(&Time);

    Now = (DWORD_PTR)Time.wHour * 3600000 + 
          (DWORD_PTR)Time.wMinute * 60000;
    if (pJob->JobTime > Now)
        bToday = TRUE;

    if (pJob->DaysOfMonth != 0)
    {
        wDay = 0;
        for (i = Time.wDay - 1; i < 32; i++)
        {
            if (pJob->DaysOfMonth && (1 << i))
            {
                wDay = i;
                break;
            }
        }
        ERR("Next day this month: %hu\n", wDay);
    }
    else if (pJob->DaysOfWeek != 0)
    {

    }
}
#endif
