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
    BOOL ret;

    FIXME("AddJobA(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pData, cbBuf, pcbNeeded);

    if (Level != 1)
    {
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ret = AddJobW(hPrinter, Level, pData, cbBuf, pcbNeeded);

    if (ret)
    {
        DWORD dwErrorCode;
        ADDJOB_INFO_1W *addjobW = (ADDJOB_INFO_1W*)pData;
        dwErrorCode = UnicodeToAnsiInPlace(addjobW->Path);
        if (dwErrorCode != ERROR_SUCCESS)
        {
           ret = FALSE;
        }
    }
    return ret;
}

BOOL WINAPI
AddJobW(HANDLE hPrinter, DWORD Level, PBYTE pData, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    FIXME("AddJobW(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pData, cbBuf, pcbNeeded);

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
        JOB_INFO_1W* pji1w = (JOB_INFO_1W*)pData;

        // Replace relative offset addresses in the output by absolute pointers.
        MarshallUpStructure(cbBuf, pData, AddJobInfo1Marshalling.pInfo, AddJobInfo1Marshalling.cbStructureSize, TRUE);
        pHandle->bJob = TRUE;
        UpdateTrayIcon( hPrinter, pji1w->JobId );
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumJobsA(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode, i;
    JOB_INFO_1W* pji1w = (JOB_INFO_1W*)pJob;
    JOB_INFO_2A* pji2a = (JOB_INFO_2A*)pJob;
    JOB_INFO_2W* pji2w = (JOB_INFO_2W*)pJob;

    TRACE("EnumJobsA(%p, %lu, %lu, %lu, %p, %lu, %p, %p)\n", hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned);

    if ( Level == 3 )
        return EnumJobsW( hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned );

    if ( EnumJobsW( hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned ) )
    {
        switch ( Level )
        {
            case 1:
            {
                for ( i = 0; i < *pcReturned; i++ )
                {
                    dwErrorCode = UnicodeToAnsiInPlace(pji1w[i].pPrinterName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji1w[i].pMachineName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji1w[i].pUserName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji1w[i].pDocument);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji1w[i].pDatatype);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji1w[i].pStatus);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                }
            }
                break;

            case 2:
            {
                for ( i = 0; i < *pcReturned; i++ )
                {
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pPrinterName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pMachineName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pUserName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pDocument);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pNotifyName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pDatatype);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pPrintProcessor);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pParameters);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pji2w[i].pStatus);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    if ( pji2w[i].pDevMode )
                    {
                        RosConvertUnicodeDevModeToAnsiDevmode( pji2w[i].pDevMode, pji2a[i].pDevMode );
                    }
                }
            }
                break;
        }
        return TRUE;
    }
Cleanup:
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
    DWORD dwErrorCode;
    JOB_INFO_1W* pji1w = (JOB_INFO_1W*)pJob;
    JOB_INFO_2A* pji2a = (JOB_INFO_2A*)pJob;
    JOB_INFO_2W* pji2w = (JOB_INFO_2W*)pJob;

    FIXME("GetJobA(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);

    if ( Level == 3 )
        return GetJobW( hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded );

    if ( GetJobW( hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded ) )
    {
        switch ( Level )
        {
            case 1:
                dwErrorCode = UnicodeToAnsiInPlace(pji1w->pPrinterName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji1w->pMachineName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji1w->pUserName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji1w->pDocument);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji1w->pDatatype);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji1w->pStatus);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                break;

            case 2:
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pPrinterName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pMachineName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pUserName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pDocument);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pNotifyName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pDatatype);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pPrintProcessor);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pParameters);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pji2w->pStatus);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                if ( pji2w->pDevMode )
                {
                    RosConvertUnicodeDevModeToAnsiDevmode( pji2w->pDevMode, pji2a->pDevMode );
                }
                break;
        }
        return TRUE;
    }
Cleanup:
    return FALSE;
}

BOOL WINAPI
GetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    FIXME("GetJobW(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);

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

    if ( dwErrorCode == ERROR_SUCCESS )
        pHandle->bJob = FALSE;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command)
{
    BOOL ret;
    LPBYTE JobW;
    UNICODE_STRING usBuffer;

    TRACE("SetJobA(%p, %lu, %lu, %p, %lu)\n", hPrinter, JobId, Level, pJobInfo, Command);

    /* JobId, pPrinterName, pMachineName, pDriverName, Size, Submitted, Time and TotalPages
       are all ignored by SetJob, so we don't bother copying them */
    switch(Level)
    {
    case 0:
        JobW = NULL;
        break;
    case 1:
      {
        JOB_INFO_1A *info1A = (JOB_INFO_1A*)pJobInfo;
        JOB_INFO_1W *info1W = HeapAlloc(GetProcessHeap(), 0, sizeof(*info1W));
        ZeroMemory( info1W, sizeof(JOB_INFO_1W) );

        JobW = (LPBYTE)info1W;
        info1W->pUserName = AsciiToUnicode(&usBuffer, info1A->pUserName);
        info1W->pDocument = AsciiToUnicode(&usBuffer, info1A->pDocument);
        info1W->pDatatype = AsciiToUnicode(&usBuffer, info1A->pDatatype);
        info1W->pStatus = AsciiToUnicode(&usBuffer, info1A->pStatus);
        info1W->Status = info1A->Status;
        info1W->Priority = info1A->Priority;
        info1W->Position = info1A->Position;
        info1W->PagesPrinted = info1A->PagesPrinted;
        break;
      }
    case 2:
      {
        JOB_INFO_2A *info2A = (JOB_INFO_2A*)pJobInfo;
        JOB_INFO_2W *info2W = HeapAlloc(GetProcessHeap(), 0, sizeof(*info2W));
        ZeroMemory( info2W, sizeof(JOB_INFO_2W) );

        JobW = (LPBYTE)info2W;
        info2W->pUserName = AsciiToUnicode(&usBuffer, info2A->pUserName);
        info2W->pDocument = AsciiToUnicode(&usBuffer, info2A->pDocument);
        info2W->pNotifyName = AsciiToUnicode(&usBuffer, info2A->pNotifyName);
        info2W->pDatatype = AsciiToUnicode(&usBuffer, info2A->pDatatype);
        info2W->pPrintProcessor = AsciiToUnicode(&usBuffer, info2A->pPrintProcessor);
        info2W->pParameters = AsciiToUnicode(&usBuffer, info2A->pParameters);
        info2W->pDevMode = info2A->pDevMode ? GdiConvertToDevmodeW(info2A->pDevMode) : NULL;
        info2W->pStatus = AsciiToUnicode(&usBuffer, info2A->pStatus);
        info2W->pSecurityDescriptor = info2A->pSecurityDescriptor;
        info2W->Status = info2A->Status;
        info2W->Priority = info2A->Priority;
        info2W->Position = info2A->Position;
        info2W->StartTime = info2A->StartTime;
        info2W->UntilTime = info2A->UntilTime;
        info2W->PagesPrinted = info2A->PagesPrinted;
        break;
      }
    case 3:
        JobW = HeapAlloc(GetProcessHeap(), 0, sizeof(JOB_INFO_3));
        memcpy(JobW, pJobInfo, sizeof(JOB_INFO_3));
        break;
    default:
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ret = SetJobW(hPrinter, JobId, Level, JobW, Command);

    switch(Level)
    {
    case 1:
      {
        JOB_INFO_1W *info1W = (JOB_INFO_1W*)JobW;
        if (info1W->pUserName) HeapFree(GetProcessHeap(), 0, info1W->pUserName);
        if (info1W->pDocument) HeapFree(GetProcessHeap(), 0, info1W->pDocument);
        if (info1W->pDatatype) HeapFree(GetProcessHeap(), 0, info1W->pDatatype);
        if (info1W->pStatus) HeapFree(GetProcessHeap(), 0, info1W->pStatus);
        break;
      }
    case 2:
      {
        JOB_INFO_2W *info2W = (JOB_INFO_2W*)JobW;
        if (info2W->pUserName) HeapFree(GetProcessHeap(), 0, info2W->pUserName);
        if (info2W->pDocument) HeapFree(GetProcessHeap(), 0, info2W->pDocument);
        if (info2W->pNotifyName) HeapFree(GetProcessHeap(), 0, info2W->pNotifyName);
        if (info2W->pDatatype) HeapFree(GetProcessHeap(), 0, info2W->pDatatype);
        if (info2W->pPrintProcessor) HeapFree(GetProcessHeap(), 0, info2W->pPrintProcessor);
        if (info2W->pParameters) HeapFree(GetProcessHeap(), 0, info2W->pParameters);
        if (info2W->pDevMode) HeapFree(GetProcessHeap(), 0, info2W->pDevMode);
        if (info2W->pStatus) HeapFree(GetProcessHeap(), 0, info2W->pStatus);
        break;
      }
    }
    HeapFree(GetProcessHeap(), 0, JobW);

    return ret;
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
