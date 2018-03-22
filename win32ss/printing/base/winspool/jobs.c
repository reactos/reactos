/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/jobs.h>

BOOL WINAPI
AddJobA(HANDLE hPrinter, DWORD Level, PBYTE pData, DWORD cbBuf, PDWORD pcbNeeded)
{
    TRACE("AddJobA(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pData, cbBuf, pcbNeeded);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddJobW(HANDLE hPrinter, DWORD Level, PBYTE pData, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("AddJobW(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pData, cbBuf, pcbNeeded);

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcAddJob(pHandle->hPrinter, Level, pData, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcAddJob failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        MarshallUpStructure(cbBuf, pData, AddJobInfo1Marshalling.pInfo, AddJobInfo1Marshalling.cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumJobsA(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumJobsA(%p, %lu, %lu, %lu, %p, %lu, %p, %p)\n", hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumJobsW(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("EnumJobsW(%p, %lu, %lu, %lu, %p, %lu, %p, %p)\n", hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned);

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumJobs(pHandle->hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumJobs failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers for JOB_INFO_1W and JOB_INFO_2W.
        if (Level <= 2)
        {
            ASSERT(Level >= 1);
            MarshallUpStructuresArray(cbBuf, pJob, *pcReturned, pJobInfoMarshalling[Level]->pInfo, pJobInfoMarshalling[Level]->cbStructureSize, TRUE);
        }
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded)
{
    TRACE("GetJobA(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("GetJobW(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetJob(pHandle->hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcGetJob failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallUpStructure(cbBuf, pJob, pJobInfoMarshalling[Level]->pInfo, pJobInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
ScheduleJob(HANDLE hPrinter, DWORD dwJobID)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("ScheduleJob(%p, %lu)\n", hPrinter, dwJobID);

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcScheduleJob(pHandle->hPrinter, dwJobID);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcScheduleJob failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command)
{
    TRACE("SetJobA(%p, %lu, %lu, %p, %lu)\n", hPrinter, JobId, Level, pJobInfo, Command);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;
    WINSPOOL_JOB_CONTAINER JobContainer;

    TRACE("SetJobW(%p, %lu, %lu, %p, %lu)\n", hPrinter, JobId, Level, pJobInfo, Command);

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // pJobContainer->JobInfo is a union of pointers, so we can just set any element to our BYTE pointer.
    JobContainer.Level = Level;
    JobContainer.JobInfo.Level1 = (WINSPOOL_JOB_INFO_1*)pJobInfo;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcSetJob(pHandle->hPrinter, JobId, &JobContainer, Command);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcSetJob failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
