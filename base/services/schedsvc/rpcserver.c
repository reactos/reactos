/*
 *  ReactOS Services
 *  Copyright (C) 2015 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/schedsvc/rpcserver.c
 * PURPOSE:          Scheduler service
 * PROGRAMMER:       Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "lmerr.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);


/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\atsvc", NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(atsvc_v1_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


/* Function 0 */
NET_API_STATUS
WINAPI
NetrJobAdd(
    ATSVC_HANDLE ServerName,
    LPAT_INFO pAtInfo,
    LPDWORD pJobId)
{
    PJOB pJob;

    TRACE("NetrJobAdd(%S %p %p)\n",
          ServerName, pAtInfo, pJobId);

    /* Allocate a new job object */
    pJob = HeapAlloc(GetProcessHeap(),
                     HEAP_ZERO_MEMORY,
                     sizeof(JOB) + wcslen(pAtInfo->Command) * sizeof(WCHAR));
    if (pJob == NULL)
        return ERROR_OUTOFMEMORY;

    /* Initialize the job object */
    pJob->JobTime = pAtInfo->JobTime;
    pJob->DaysOfMonth = pAtInfo->DaysOfMonth;
    pJob->DaysOfWeek = pAtInfo->DaysOfWeek;
    pJob->Flags = pAtInfo->Flags;
    wcscpy(pJob->Command, pAtInfo->Command);

    /* Acquire the job list lock exclusively */
    RtlAcquireResourceExclusive(&JobListLock, TRUE);

    /* Assign a new job ID */
    pJob->JobId = dwNextJobId++;
    dwJobCount++;

    /* Append the new job to the job list */
    InsertTailList(&JobListHead, &pJob->JobEntry);

    /* Save the job in the registry */
    SaveJob(pJob);

    /* Calculate the next start time */
    CalculateNextStartTime(pJob);

#if 0
    DumpStartList(&StartListHead);
#endif

    /* Release the job list lock */
    RtlReleaseResource(&JobListLock);

    /* Set the update event */
    if (Events[1] != NULL)
        SetEvent(Events[1]);

    /* Return the new job ID */
    *pJobId = pJob->JobId;

    return ERROR_SUCCESS;
}


/* Function 1 */
NET_API_STATUS
WINAPI
NetrJobDel(
    ATSVC_HANDLE ServerName,
    DWORD MinJobId,
    DWORD MaxJobId)
{
    PLIST_ENTRY JobEntry, NextEntry;
    PJOB CurrentJob;

    TRACE("NetrJobDel(%S %lu %lu)\n",
          ServerName, MinJobId, MaxJobId);

    /* Check the job IDs */
    if (MinJobId > MaxJobId)
        return ERROR_INVALID_PARAMETER;

    /* Acquire the job list lock exclusively */
    RtlAcquireResourceExclusive(&JobListLock, TRUE);

    JobEntry = JobListHead.Flink;
    while (JobEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(JobEntry, JOB, JobEntry);

        if ((CurrentJob->JobId >= MinJobId) && (CurrentJob->JobId <= MaxJobId))
        {
#if 0
            DumpStartList(&StartListHead);
#endif

            /* Remove the job from the registry */
            DeleteJob(CurrentJob);

            NextEntry = JobEntry->Flink;
            if (RemoveEntryList(JobEntry))
            {
                dwJobCount--;
                HeapFree(GetProcessHeap(), 0, CurrentJob);
                JobEntry = NextEntry;
                continue;
            }
        }

        JobEntry = JobEntry->Flink;
    }

    /* Release the job list lock */
    RtlReleaseResource(&JobListLock);

    /* Set the update event */
    if (Events[1] != NULL)
        SetEvent(Events[1]);

    return ERROR_SUCCESS;
}


/* Function 2 */
NET_API_STATUS
__stdcall
NetrJobEnum(
    ATSVC_HANDLE ServerName,
    LPAT_ENUM_CONTAINER pEnumContainer,
    DWORD PreferedMaximumLength,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle)
{
    PLIST_ENTRY JobEntry;
    PJOB CurrentJob;
    PAT_ENUM pEnum;
    DWORD dwStartIndex, dwIndex;
    DWORD dwEntriesToRead, dwEntriesRead;
    DWORD dwRequiredSize, dwEntrySize;
    PWSTR pString;
    DWORD dwError = ERROR_SUCCESS;

    TRACE("NetrJobEnum(%S %p %lu %p %p)\n",
          ServerName, pEnumContainer, PreferedMaximumLength, pTotalEntries, pResumeHandle);

    if (pEnumContainer == NULL)
    {
        *pTotalEntries = 0;
        return ERROR_INVALID_PARAMETER;
    }

    if (*pResumeHandle >= dwJobCount)
    {
        *pTotalEntries = 0;
        return ERROR_SUCCESS;
    }

    dwStartIndex = *pResumeHandle;
    TRACE("dwStartIndex: %lu\n", dwStartIndex);

    /* Acquire the job list lock exclusively */
    RtlAcquireResourceShared(&JobListLock, TRUE);

    dwEntriesToRead = 0;
    dwRequiredSize = 0;
    dwIndex = 0;
    JobEntry = JobListHead.Flink;
    while (JobEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(JobEntry, JOB, JobEntry);

        if (dwIndex >= dwStartIndex)
        {
            TRACE("dwIndex: %lu\n", dwIndex);
            dwEntrySize = sizeof(AT_ENUM) +
                          (wcslen(CurrentJob->Command) + 1) * sizeof(WCHAR);
            TRACE("dwEntrySize: %lu\n", dwEntrySize);

            if ((PreferedMaximumLength != ULONG_MAX) &&
                (dwRequiredSize + dwEntrySize > PreferedMaximumLength))
                break;

            dwRequiredSize += dwEntrySize;
            dwEntriesToRead++;
        }

        JobEntry = JobEntry->Flink;
        dwIndex++;
    }
    TRACE("dwEntriesToRead: %lu\n", dwEntriesToRead);
    TRACE("dwRequiredSize: %lu\n", dwRequiredSize);

    if (PreferedMaximumLength != ULONG_MAX)
        dwRequiredSize = PreferedMaximumLength;

    TRACE("Allocating dwRequiredSize: %lu\n", dwRequiredSize);
    pEnum = midl_user_allocate(dwRequiredSize);
    if (pEnum == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    pString = (PWSTR)((ULONG_PTR)pEnum + dwEntriesToRead * sizeof(AT_ENUM));

    dwEntriesRead = 0;
    dwIndex = 0;
    JobEntry = JobListHead.Flink;
    while (JobEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(JobEntry, JOB, JobEntry);

        if (dwIndex >= dwStartIndex)
        {
            pEnum[dwIndex].JobId = CurrentJob->JobId;
            pEnum[dwIndex].JobTime = CurrentJob->JobTime;
            pEnum[dwIndex].DaysOfMonth = CurrentJob->DaysOfMonth;
            pEnum[dwIndex].DaysOfWeek = CurrentJob->DaysOfWeek;
            pEnum[dwIndex].Flags = CurrentJob->Flags;
            pEnum[dwIndex].Command = pString;
            wcscpy(pString, CurrentJob->Command);

            pString = (PWSTR)((ULONG_PTR)pString + (wcslen(CurrentJob->Command) + 1) * sizeof(WCHAR));

            dwEntriesRead++;
        }

        if (dwEntriesRead == dwEntriesToRead)
            break;

        /* Next job */
        JobEntry = JobEntry->Flink;
        dwIndex++;
    }

    pEnumContainer->EntriesRead = dwEntriesRead;
    pEnumContainer->Buffer = pEnum;

    *pTotalEntries = dwJobCount;
    *pResumeHandle = dwIndex;

    if (dwEntriesRead + dwStartIndex < dwJobCount)
        dwError = ERROR_MORE_DATA;
    else
        dwError = ERROR_SUCCESS;

done:
    /* Release the job list lock */
    RtlReleaseResource(&JobListLock);

    return dwError;
}


/* Function 3 */
NET_API_STATUS
WINAPI
NetrJobGetInfo(
    ATSVC_HANDLE ServerName,
    DWORD JobId,
    LPAT_INFO *ppAtInfo)
{
    PLIST_ENTRY JobEntry;
    PJOB CurrentJob;
    PAT_INFO pInfo;
    DWORD dwError = ERROR_FILE_NOT_FOUND;

    TRACE("NetrJobGetInfo(%S %lu %p)\n",
          ServerName, JobId, ppAtInfo);

    /* Acquire the job list lock exclusively */
    RtlAcquireResourceShared(&JobListLock, TRUE);

    /* Traverse the job list */
    JobEntry = JobListHead.Flink;
    while (JobEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(JobEntry, JOB, JobEntry);

        /* Do we have the right job? */
        if (CurrentJob->JobId == JobId)
        {
            pInfo = midl_user_allocate(sizeof(AT_INFO));
            if (pInfo == NULL)
            {
                dwError = ERROR_OUTOFMEMORY;
                goto done;
            }

            pInfo->Command = midl_user_allocate((wcslen(CurrentJob->Command) + 1) * sizeof(WCHAR));
            if (pInfo->Command == NULL)
            {
                midl_user_free(pInfo);
                dwError = ERROR_OUTOFMEMORY;
                goto done;
            }

            pInfo->JobTime = CurrentJob->JobTime;
            pInfo->DaysOfMonth = CurrentJob->DaysOfMonth;
            pInfo->DaysOfWeek = CurrentJob->DaysOfWeek;
            pInfo->Flags = CurrentJob->Flags;
            wcscpy(pInfo->Command, CurrentJob->Command);

            *ppAtInfo = pInfo;

            dwError = ERROR_SUCCESS;
            goto done;
        }

        /* Next job */
        JobEntry = JobEntry->Flink;
    }

done:
    /* Release the job list lock */
    RtlReleaseResource(&JobListLock);

    return dwError;
}

/* EOF */
