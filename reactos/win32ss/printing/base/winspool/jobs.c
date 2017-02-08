/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallUpAddJobInfo(PADDJOB_INFO_1W pAddJobInfo1)
{
    // Replace relative offset addresses in the output by absolute pointers.
    pAddJobInfo1->Path = (PWSTR)((ULONG_PTR)pAddJobInfo1->Path + (ULONG_PTR)pAddJobInfo1);
}

static void
_MarshallUpJobInfo(PBYTE pJobInfo, DWORD Level)
{
    PJOB_INFO_1W pJobInfo1;
    PJOB_INFO_2W pJobInfo2;

    // Replace relative offset addresses in the output by absolute pointers.
    if (Level == 1)
    {
        pJobInfo1 = (PJOB_INFO_1W)pJobInfo;
        pJobInfo1->pDatatype = (PWSTR)((ULONG_PTR)pJobInfo1->pDatatype + (ULONG_PTR)pJobInfo1);
        pJobInfo1->pDocument = (PWSTR)((ULONG_PTR)pJobInfo1->pDocument + (ULONG_PTR)pJobInfo1);
        pJobInfo1->pMachineName = (PWSTR)((ULONG_PTR)pJobInfo1->pMachineName + (ULONG_PTR)pJobInfo1);
        pJobInfo1->pPrinterName = (PWSTR)((ULONG_PTR)pJobInfo1->pPrinterName + (ULONG_PTR)pJobInfo1);

        if (pJobInfo1->pStatus)
            pJobInfo1->pStatus = (PWSTR)((ULONG_PTR)pJobInfo1->pStatus + (ULONG_PTR)pJobInfo1);

        pJobInfo1->pUserName = (PWSTR)((ULONG_PTR)pJobInfo1->pUserName + (ULONG_PTR)pJobInfo1);
    }
    else if (Level == 2)
    {
        pJobInfo2 = (PJOB_INFO_2W)pJobInfo;
        pJobInfo2->pDatatype = (PWSTR)((ULONG_PTR)pJobInfo2->pDatatype + (ULONG_PTR)pJobInfo2);
        pJobInfo2->pDevMode = (PDEVMODEW)((ULONG_PTR)pJobInfo2->pDevMode + (ULONG_PTR)pJobInfo2);
        pJobInfo2->pDocument = (PWSTR)((ULONG_PTR)pJobInfo2->pDocument + (ULONG_PTR)pJobInfo2);
        pJobInfo2->pDriverName = (PWSTR)((ULONG_PTR)pJobInfo2->pDriverName + (ULONG_PTR)pJobInfo2);
        pJobInfo2->pMachineName = (PWSTR)((ULONG_PTR)pJobInfo2->pMachineName + (ULONG_PTR)pJobInfo2);
        pJobInfo2->pNotifyName = (PWSTR)((ULONG_PTR)pJobInfo2->pNotifyName + (ULONG_PTR)pJobInfo2);
        pJobInfo2->pPrinterName = (PWSTR)((ULONG_PTR)pJobInfo2->pPrinterName + (ULONG_PTR)pJobInfo2);
        pJobInfo2->pPrintProcessor = (PWSTR)((ULONG_PTR)pJobInfo2->pPrintProcessor + (ULONG_PTR)pJobInfo2);

        if (pJobInfo2->pParameters)
            pJobInfo2->pParameters = (PWSTR)((ULONG_PTR)pJobInfo2->pParameters + (ULONG_PTR)pJobInfo2);

        if (pJobInfo2->pStatus)
            pJobInfo2->pStatus = (PWSTR)((ULONG_PTR)pJobInfo2->pStatus + (ULONG_PTR)pJobInfo2);

        pJobInfo2->pUserName = (PWSTR)((ULONG_PTR)pJobInfo2->pUserName + (ULONG_PTR)pJobInfo2);
    }
}

BOOL WINAPI
AddJobA(HANDLE hPrinter, DWORD Level, PBYTE pData, DWORD cbBuf, PDWORD pcbNeeded)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddJobW(HANDLE hPrinter, DWORD Level, PBYTE pData, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

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
        _MarshallUpAddJobInfo((PADDJOB_INFO_1W)pData);

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumJobsA(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumJobsW(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pJob;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

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
        // Replace relative offset addresses in the output by absolute pointers.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallUpJobInfo(p, Level);

            if (Level == 1)
                p += sizeof(JOB_INFO_1W);
            else if (Level == 2)
                p += sizeof(JOB_INFO_2W);
        }
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

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
        _MarshallUpJobInfo(pJob, Level);
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
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;
    WINSPOOL_JOB_CONTAINER JobContainer;

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
