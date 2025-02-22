/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/jobs.h>

DWORD
_RpcAddJob(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Level, BYTE* pAddJob, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pAddJobAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pAddJobAligned = AlignRpcPtr(pAddJob, &cbBuf);

    if (AddJobW(hPrinter, Level, pAddJobAligned, cbBuf, pcbNeeded))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        MarshallDownStructure(pAddJobAligned, AddJobInfo1Marshalling.pInfo, AddJobInfo1Marshalling.cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pAddJob, pAddJobAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcEnumJobs(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, BYTE* pJob, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pJobAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pJobAligned = AlignRpcPtr(pJob, &cbBuf);

    if (EnumJobsW(hPrinter, FirstJob, NoJobs, Level, pJobAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets for JOB_INFO_1W and JOB_INFO_2W.
        if (Level <= 2)
        {
            ASSERT(Level >= 1);
            MarshallDownStructuresArray(pJobAligned, *pcReturned, pJobInfoMarshalling[Level]->pInfo, pJobInfoMarshalling[Level]->cbStructureSize, TRUE);
        }
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pJob, pJobAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcGetJob(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD JobId, DWORD Level, BYTE* pJob, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pJobAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pJobAligned = AlignRpcPtr(pJob, &cbBuf);

    if (GetJobW(hPrinter, JobId, Level, pJobAligned, cbBuf, pcbNeeded))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallDownStructure(pJobAligned, pJobInfoMarshalling[Level]->pInfo, pJobInfoMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pJob, pJobAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcScheduleJob(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD JobId)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!ScheduleJob(hPrinter, JobId))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcSetJob(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD JobId, WINSPOOL_JOB_CONTAINER* pJobContainer, DWORD Command)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    // pJobContainer->JobInfo is a union of pointers, so we can just convert any element to our BYTE pointer.
    if (!SetJobW(hPrinter, JobId, pJobContainer->Level, (PBYTE)pJobContainer->JobInfo.Level1, Command))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
