/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownAddJobInfo(PADDJOB_INFO_1W pAddJobInfo1)
{
    // Replace absolute pointer addresses in the output by relative offsets.
    pAddJobInfo1->Path = (PWSTR)((ULONG_PTR)pAddJobInfo1->Path - (ULONG_PTR)pAddJobInfo1);
}

static void
_MarshallDownJobInfo(PBYTE pJobInfo, DWORD Level)
{
    PJOB_INFO_1W pJobInfo1;
    PJOB_INFO_2W pJobInfo2;

    // Replace absolute pointer addresses in the output by relative offsets.
    if (Level == 1)
    {
        pJobInfo1 = (PJOB_INFO_1W)pJobInfo;
        pJobInfo1->pDatatype = (PWSTR)((ULONG_PTR)pJobInfo1->pDatatype - (ULONG_PTR)pJobInfo1);
        pJobInfo1->pDocument = (PWSTR)((ULONG_PTR)pJobInfo1->pDocument - (ULONG_PTR)pJobInfo1);
        pJobInfo1->pMachineName = (PWSTR)((ULONG_PTR)pJobInfo1->pMachineName - (ULONG_PTR)pJobInfo1);
        pJobInfo1->pPrinterName = (PWSTR)((ULONG_PTR)pJobInfo1->pPrinterName - (ULONG_PTR)pJobInfo1);

        if (pJobInfo1->pStatus)
            pJobInfo1->pStatus = (PWSTR)((ULONG_PTR)pJobInfo1->pStatus - (ULONG_PTR)pJobInfo1);

        pJobInfo1->pUserName = (PWSTR)((ULONG_PTR)pJobInfo1->pUserName - (ULONG_PTR)pJobInfo1);
    }
    else if (Level == 2)
    {
        pJobInfo2 = (PJOB_INFO_2W)pJobInfo;
        pJobInfo2->pDatatype = (PWSTR)((ULONG_PTR)pJobInfo2->pDatatype - (ULONG_PTR)pJobInfo2);
        pJobInfo2->pDevMode = (PDEVMODEW)((ULONG_PTR)pJobInfo2->pDevMode - (ULONG_PTR)pJobInfo2);
        pJobInfo2->pDocument = (PWSTR)((ULONG_PTR)pJobInfo2->pDocument - (ULONG_PTR)pJobInfo2);
        pJobInfo2->pDriverName = (PWSTR)((ULONG_PTR)pJobInfo2->pDriverName - (ULONG_PTR)pJobInfo2);
        pJobInfo2->pMachineName = (PWSTR)((ULONG_PTR)pJobInfo2->pMachineName - (ULONG_PTR)pJobInfo2);
        pJobInfo2->pNotifyName = (PWSTR)((ULONG_PTR)pJobInfo2->pNotifyName - (ULONG_PTR)pJobInfo2);
        pJobInfo2->pPrinterName = (PWSTR)((ULONG_PTR)pJobInfo2->pPrinterName - (ULONG_PTR)pJobInfo2);
        pJobInfo2->pPrintProcessor = (PWSTR)((ULONG_PTR)pJobInfo2->pPrintProcessor - (ULONG_PTR)pJobInfo2);

        if (pJobInfo2->pParameters)
            pJobInfo2->pParameters = (PWSTR)((ULONG_PTR)pJobInfo2->pParameters - (ULONG_PTR)pJobInfo2);

        if (pJobInfo2->pStatus)
            pJobInfo2->pStatus = (PWSTR)((ULONG_PTR)pJobInfo2->pStatus - (ULONG_PTR)pJobInfo2);

        pJobInfo2->pUserName = (PWSTR)((ULONG_PTR)pJobInfo2->pUserName - (ULONG_PTR)pJobInfo2);
    }
}

DWORD
_RpcAddJob(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Level, BYTE* pAddJob, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    AddJobW(hPrinter, Level, pAddJob, cbBuf, pcbNeeded);
    dwErrorCode = GetLastError();

    if (dwErrorCode == ERROR_SUCCESS)
        _MarshallDownAddJobInfo((PADDJOB_INFO_1W)pAddJob);

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcEnumJobs(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, BYTE* pJob, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pJob;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    EnumJobsW(hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallDownJobInfo(p, Level);

            if (Level == 1)
                p += sizeof(JOB_INFO_1W);
            else if (Level == 2)
                p += sizeof(JOB_INFO_2W);
        }
    }

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcGetJob(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD JobId, DWORD Level, BYTE* pJob, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    GetJobW(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
    dwErrorCode = GetLastError();

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        _MarshallDownJobInfo(pJob, Level);
    }

    RpcRevertToSelf();
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

    ScheduleJob(hPrinter, JobId);
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
    SetJobW(hPrinter, JobId, pJobContainer->Level, (PBYTE)pJobContainer->JobInfo.Level1, Command);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
