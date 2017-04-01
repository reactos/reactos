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
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "lmerr.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

typedef struct _JOB
{
    LIST_ENTRY Entry;
    DWORD JobId;

    DWORD_PTR JobTime;
    DWORD DaysOfMonth;
    UCHAR DaysOfWeek;
    UCHAR Flags;
    WCHAR Command[1];
} JOB, *PJOB;

DWORD dwNextJobId = 0;
DWORD dwJobCount = 0;
LIST_ENTRY JobListHead;
RTL_RESOURCE JobListLock;


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
    InsertTailList(&JobListHead, &pJob->Entry);

    /* Release the job list lock */
    RtlReleaseResource(&JobListLock);

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

    /* Acquire the job list lock exclusively */
    RtlAcquireResourceExclusive(&JobListLock, TRUE);

    JobEntry = JobListHead.Flink;
    while (JobEntry != &JobListHead)
    {
        CurrentJob = CONTAINING_RECORD(JobEntry, JOB, Entry);

        if ((CurrentJob->JobId >= MinJobId) && (CurrentJob->JobId <= MaxJobId))
        {
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
    TRACE("NetrJobEnum(%S %p %lu %p %p)\n",
          ServerName, pEnumContainer, PreferedMaximumLength, pTotalEntries, pResumeHandle);
    return ERROR_SUCCESS;
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
        CurrentJob = CONTAINING_RECORD(JobEntry, JOB, Entry);

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
