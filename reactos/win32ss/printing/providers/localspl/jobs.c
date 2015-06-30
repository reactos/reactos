/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
SKIPLIST GlobalJobList;

// Local Variables
static DWORD _dwLastJobID;


/**
 * @name _GlobalJobListCompareRoutine
 *
 * SKIPLIST_COMPARE_ROUTINE for the Global Job List.
 * We need the Global Job List to check whether a Job ID is already in use. Consequently, this list is sorted by ID.
 */
static int WINAPI
_GlobalJobListCompareRoutine(PVOID FirstStruct, PVOID SecondStruct)
{
    PLOCAL_JOB A = (PLOCAL_JOB)FirstStruct;
    PLOCAL_JOB B = (PLOCAL_JOB)SecondStruct;

    return A->dwJobID - B->dwJobID;
}

/**
 * @name _PrinterJobListCompareRoutine
 *
 * SKIPLIST_COMPARE_ROUTINE for the each Printer's Job List.
 * Jobs in this list are sorted in the desired order of processing.
 */
static int WINAPI
_PrinterJobListCompareRoutine(PVOID FirstStruct, PVOID SecondStruct)
{
    PLOCAL_JOB A = (PLOCAL_JOB)FirstStruct;
    PLOCAL_JOB B = (PLOCAL_JOB)SecondStruct;
    int iComparison;
    FILETIME ftSubmittedA;
    FILETIME ftSubmittedB;
    ULARGE_INTEGER uliSubmittedA;
    ULARGE_INTEGER uliSubmittedB;
    ULONGLONG ullResult;

    // First compare the priorities to determine the order.
    // The job with a higher priority shall come first.
    iComparison = A->dwPriority - B->dwPriority;
    if (iComparison != 0)
        return iComparison;

    // Both have the same priority, so go by creation time.
    // Comparison is done using the MSDN-recommended way for comparing SYSTEMTIMEs.
    if (!SystemTimeToFileTime(&A->stSubmitted, &ftSubmittedA))
    {
        ERR("SystemTimeToFileTime failed for A with error %lu!\n", GetLastError());
        return 0;
    }

    if (!SystemTimeToFileTime(&B->stSubmitted, &ftSubmittedB))
    {
        ERR("SystemTimeToFileTime failed for B with error %lu!\n", GetLastError());
        return 0;
    }

    uliSubmittedA.LowPart = ftSubmittedA.dwLowDateTime;
    uliSubmittedA.HighPart = ftSubmittedA.dwHighDateTime;
    uliSubmittedB.LowPart = ftSubmittedB.dwLowDateTime;
    uliSubmittedB.HighPart = ftSubmittedB.dwHighDateTime;
    ullResult = uliSubmittedA.QuadPart - uliSubmittedB.QuadPart;

    if (ullResult < 0)
        return -1;
    else if (ullResult > 0)
        return 1;

    return 0;
}

BOOL
GetNextJobID(PDWORD dwJobID)
{
    ++_dwLastJobID;

    while (LookupElementSkiplist(&GlobalJobList, &_dwLastJobID, NULL))
    {
        // This ID is already taken. Try the next one.
        ++_dwLastJobID;
    }

    if (!IS_VALID_JOB_ID(_dwLastJobID))
    {
        ERR("Job ID %lu isn't valid!\n", _dwLastJobID);
        return FALSE;
    }

    *dwJobID = _dwLastJobID;
    return TRUE;
}

void
InitializeGlobalJobList()
{
    const WCHAR wszPath[] = L"\\PRINTERS\\?????.SHD";
    const DWORD cchPath = _countof(wszPath) - 1;
    const DWORD cchFolders = sizeof("\\PRINTERS\\") - 1;
    const DWORD cchPattern = sizeof("?????") - 1;

    DWORD dwJobID;
    HANDLE hFind;
    PLOCAL_JOB pJob = NULL;
    PWSTR p;
    WCHAR wszFullPath[MAX_PATH];
    WIN32_FIND_DATAW FindData;

    // This one is incremented in GetNextJobID.
    _dwLastJobID = 0;

    // Initialize an empty list for all jobs of all local printers.
    // We will search it by Job ID (supply a pointer to a DWORD in LookupElementSkiplist).
    InitializeSkiplist(&GlobalJobList, DllAllocSplMem, _GlobalJobListCompareRoutine, (PSKIPLIST_FREE_ROUTINE)DllFreeSplMem);

    // Construct the full path search pattern.
    CopyMemory(wszFullPath, wszSpoolDirectory, cchSpoolDirectory * sizeof(WCHAR));
    CopyMemory(&wszFullPath[cchSpoolDirectory], wszPath, (cchPath + 1) * sizeof(WCHAR));

    // Use the search pattern to look for unfinished jobs serialized in shadow files (.SHD)
    hFind = FindFirstFileW(wszFullPath, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // No unfinished jobs found.
        goto Cleanup;
    }

    do
    {
        // Skip possible subdirectories.
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        // Extract the Job ID and verify the file name format at the same time.
        dwJobID = wcstoul(FindData.cFileName, &p, 10);
        if (!IS_VALID_JOB_ID(dwJobID))
            continue;

        if (wcsicmp(p, L".SHD") != 0)
            continue;

        // This shadow file has a valid name. Construct the full path and try to load it.
        CopyMemory(&wszFullPath[cchSpoolDirectory + cchFolders], FindData.cFileName, cchPattern);
        pJob = ReadJobShadowFile(wszFullPath);
        if (!pJob)
            continue;

        // Add it to the Global Job List.
        if (!InsertElementSkiplist(&GlobalJobList, pJob))
        {
            ERR("InsertElementSkiplist failed for job %lu for the GlobalJobList!\n", pJob->dwJobID);
            goto Cleanup;
        }

        // Add it to the Printer's Job List.
        if (!InsertElementSkiplist(&pJob->pPrinter->JobList, pJob))
        {
            ERR("InsertElementSkiplist failed for job %lu for the Printer's Job List!\n", pJob->dwJobID);
            goto Cleanup;
        }
    }
    while (FindNextFileW(hFind, &FindData));

Cleanup:
    // Outside the loop
    if (hFind)
        FindClose(hFind);
}

void
InitializePrinterJobList(PLOCAL_PRINTER pPrinter)
{
    // Initialize an empty list for this printer's jobs.
    // This one is only for sorting the jobs. If you need to lookup a job, search the GlobalJobList by Job ID.
    InitializeSkiplist(&pPrinter->JobList, DllAllocSplMem, _PrinterJobListCompareRoutine, (PSKIPLIST_FREE_ROUTINE)DllFreeSplMem);
}

BOOL WINAPI
LocalAddJob(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded)
{
    const WCHAR wszDoubleBackslash[] = L"\\";
    const DWORD cchDoubleBackslash = _countof(wszDoubleBackslash) - 1;
    const WCHAR wszPrintersPath[] = L"\\PRINTERS\\";
    const DWORD cchPrintersPath = _countof(wszPrintersPath) - 1;
    const DWORD cchSpl = _countof("?????.SPL") - 1;

    ADDJOB_INFO_1W AddJobInfo1;
    DWORD cchMachineName;
    DWORD cchUserName;
    DWORD dwErrorCode;
    PBYTE p;
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;
    RPC_BINDING_HANDLE hServerBinding = NULL;
    RPC_WSTR pwszBinding = NULL;
    RPC_WSTR pwszMachineName = NULL;

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // This handle must not have started a job yet!
    if (pPrinterHandle->pStartedJob)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Check if this is the right structure level.
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Check if the printer is set to do direct printing.
    // The Job List isn't used in this case.
    if (pPrinterHandle->pPrinter->dwAttributes & PRINTER_ATTRIBUTE_DIRECT)
    {
        dwErrorCode = ERROR_INVALID_ACCESS;
        goto Cleanup;
    }

    // Check if the supplied buffer is large enough.
    *pcbNeeded = sizeof(ADDJOB_INFO_1W) + (cchSpoolDirectory + cchPrintersPath + cchSpl + 1) * sizeof(WCHAR);
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Create a new job.
    pJob = DllAllocSplMem(sizeof(LOCAL_JOB));
    if (!pJob)
    {
        dwErrorCode = GetLastError();
        ERR("DllAllocSplMem failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Reserve an ID for this job.
    if (!GetNextJobID(&pJob->dwJobID))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Copy over defaults to the LOCAL_JOB structure.
    pJob->pPrinter = pPrinterHandle->pPrinter;
    pJob->pPrintProcessor = pPrinterHandle->pPrinter->pPrintProcessor;
    pJob->dwPriority = DEF_PRIORITY;
    pJob->dwStatus = JOB_STATUS_SPOOLING;
    pJob->pwszDatatype = AllocSplStr(pPrinterHandle->pwszDatatype);
    pJob->pwszDocumentName = AllocSplStr(wszDefaultDocumentName);
    CopyMemory(&pJob->DevMode, &pPrinterHandle->DevMode, sizeof(DEVMODEW));
    GetSystemTime(&pJob->stSubmitted);

    // Get the user name for the Job.
    cchUserName = UNLEN + 1;
    pJob->pwszUserName = DllAllocSplMem(cchUserName * sizeof(WCHAR));
    if (!GetUserNameW(pJob->pwszUserName, &cchUserName))
    {
        dwErrorCode = GetLastError();
        ERR("GetUserNameW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // FIXME: For now, pwszNotifyName equals pwszUserName.
    pJob->pwszNotifyName = AllocSplStr(pJob->pwszUserName);

    // Get the name of the machine that submitted the Job over RPC.
    dwErrorCode = RpcBindingServerFromClient(NULL, &hServerBinding);
    if (dwErrorCode != RPC_S_OK)
    {
        ERR("RpcBindingServerFromClient failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    dwErrorCode = RpcBindingToStringBindingW(hServerBinding, &pwszBinding);
    if (dwErrorCode != RPC_S_OK)
    {
        ERR("RpcBindingToStringBindingW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    dwErrorCode = RpcStringBindingParseW(pwszBinding, NULL, NULL, &pwszMachineName, NULL, NULL);
    if (dwErrorCode != RPC_S_OK)
    {
        ERR("RpcStringBindingParseW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    cchMachineName = wcslen(pwszMachineName);
    pJob->pwszMachineName = DllAllocSplMem((cchMachineName + cchDoubleBackslash + 1) * sizeof(WCHAR));
    CopyMemory(pJob->pwszMachineName, wszDoubleBackslash, cchDoubleBackslash * sizeof(WCHAR));
    CopyMemory(pJob->pwszMachineName + cchDoubleBackslash, pwszMachineName, (cchMachineName + 1) * sizeof(WCHAR));

    // Add the job to the Global Job List.
    if (!InsertElementSkiplist(&GlobalJobList, pJob))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("InsertElementSkiplist failed for job %lu for the GlobalJobList!\n", pJob->dwJobID);
        goto Cleanup;
    }

    // Add the job at the end of the Printer's Job List.
    // As all new jobs are created with default priority, we can be sure that it would always be inserted at the end.
    if (!InsertTailElementSkiplist(&pJob->pPrinter->JobList, pJob))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("InsertTailElementSkiplist failed for job %lu for the Printer's Job List!\n", pJob->dwJobID);
        goto Cleanup;
    }

    // Return a proper ADDJOB_INFO_1W structure.
    AddJobInfo1.JobId = pJob->dwJobID;
    AddJobInfo1.Path = (PWSTR)(pData + sizeof(ADDJOB_INFO_1W));
    p = pData;
    CopyMemory(p, &AddJobInfo1, sizeof(ADDJOB_INFO_1W));
    p += sizeof(ADDJOB_INFO_1W);
    CopyMemory(p, wszSpoolDirectory, cchSpoolDirectory);
    p += cchSpoolDirectory;
    CopyMemory(p, wszPrintersPath, cchPrintersPath);
    p += cchPrintersPath;
    swprintf((PWSTR)p, L"%05lu.SPL", pJob->dwJobID);

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pwszMachineName)
        RpcStringFreeW(&pwszMachineName);

    if (pwszBinding)
        RpcStringFreeW(&pwszBinding);

    if (hServerBinding)
        RpcBindingFree(&hServerBinding);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}


static DWORD
_LocalGetJobLevel1(PLOCAL_PRINTER_HANDLE pPrinterHandle, PLOCAL_JOB pJob, PBYTE* ppStart, PBYTE* ppEnd, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD cbDatatype = (wcslen(pJob->pwszDatatype) + 1) * sizeof(WCHAR);
    DWORD cbDocumentName = (wcslen(pJob->pwszDocumentName) + 1) * sizeof(WCHAR);
    DWORD cbMachineName = (wcslen(pJob->pwszMachineName) + 1) * sizeof(WCHAR);
    DWORD cbPrinterName = (wcslen(pJob->pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
    DWORD cbStatus = 0;
    DWORD cbUserName = (wcslen(pJob->pwszUserName) + 1) * sizeof(WCHAR);
    DWORD dwErrorCode;
    JOB_INFO_1W JobInfo1 = { 0 };

    // A Status Message is optional.
    if (pJob->pwszStatus)
        cbStatus = (wcslen(pJob->pwszStatus) + 1) * sizeof(WCHAR);

    // Check if the supplied buffer is large enough.
    *pcbNeeded += sizeof(JOB_INFO_1W) + cbDatatype + cbDocumentName + cbMachineName + cbPrinterName + cbStatus + cbUserName;
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Put the strings at the end of the buffer.
    *ppEnd -= cbDatatype;
    JobInfo1.pDatatype = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszDatatype, cbDatatype);

    *ppEnd -= cbDocumentName;
    JobInfo1.pDocument = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszDocumentName, cbDocumentName);

    *ppEnd -= cbMachineName;
    JobInfo1.pMachineName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszMachineName, cbMachineName);

    *ppEnd -= cbPrinterName;
    JobInfo1.pPrinterName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrinter->pwszPrinterName, cbPrinterName);

    if (cbStatus)
    {
        *ppEnd -= cbStatus;
        JobInfo1.pStatus = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszStatus, cbStatus);
    }

    *ppEnd -= cbUserName;
    JobInfo1.pUserName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszUserName, cbUserName);

    // Fill the rest of the structure.
    JobInfo1.JobId = pJob->dwJobID;
    JobInfo1.Priority = pJob->dwPriority;
    JobInfo1.Status = pJob->dwStatus;
    JobInfo1.TotalPages = pJob->dwTotalPages;
    CopyMemory(&JobInfo1.Submitted, &pJob->stSubmitted, sizeof(SYSTEMTIME));

    // Finally copy the structure to the output pointer.
    CopyMemory(*ppStart, &JobInfo1, sizeof(JOB_INFO_1W));
    *ppStart += sizeof(JOB_INFO_1W);
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

static DWORD
_LocalGetJobLevel2(PLOCAL_PRINTER_HANDLE pPrinterHandle, PLOCAL_JOB pJob, PBYTE* ppStart, PBYTE* ppEnd, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD cbDatatype = (wcslen(pJob->pwszDatatype) + 1) * sizeof(WCHAR);
    DWORD cbDocumentName = (wcslen(pJob->pwszDocumentName) + 1) * sizeof(WCHAR);
    DWORD cbDriverName = (wcslen(pJob->pPrinter->pwszPrinterDriver) + 1) * sizeof(WCHAR);
    DWORD cbMachineName = (wcslen(pJob->pwszMachineName) + 1) * sizeof(WCHAR);
    DWORD cbNotifyName = (wcslen(pJob->pwszNotifyName) + 1) * sizeof(WCHAR);
    DWORD cbPrinterName = (wcslen(pJob->pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
    DWORD cbPrintProcessor = (wcslen(pJob->pPrintProcessor->pwszName) + 1) * sizeof(WCHAR);
    DWORD cbPrintProcessorParameters = 0;
    DWORD cbStatus = 0;
    DWORD cbUserName = (wcslen(pJob->pwszUserName) + 1) * sizeof(WCHAR);
    DWORD dwErrorCode;
    FILETIME ftNow;
    FILETIME ftSubmitted;
    JOB_INFO_2W JobInfo2 = { 0 };
    ULARGE_INTEGER uliNow;
    ULARGE_INTEGER uliSubmitted;

    // Print Processor Parameters and Status Message are optional.
    if (pJob->pwszPrintProcessorParameters)
        cbPrintProcessorParameters = (wcslen(pJob->pwszPrintProcessorParameters) + 1) * sizeof(WCHAR);

    if (pJob->pwszStatus)
        cbStatus = (wcslen(pJob->pwszStatus) + 1) * sizeof(WCHAR);

    // Check if the supplied buffer is large enough.
    *pcbNeeded += sizeof(JOB_INFO_2W) + cbDatatype + sizeof(DEVMODEW) + cbDocumentName + cbDriverName + cbMachineName + cbNotifyName + cbPrinterName + cbPrintProcessor + cbPrintProcessorParameters + cbStatus + cbUserName;
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Put the strings at the end of the buffer.
    *ppEnd -= cbDatatype;
    JobInfo2.pDatatype = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszDatatype, cbDatatype);

    *ppEnd -= sizeof(DEVMODEW);
    JobInfo2.pDevMode = (PDEVMODEW)*ppEnd;
    CopyMemory(*ppEnd, &pJob->DevMode, sizeof(DEVMODEW));

    *ppEnd -= cbDocumentName;
    JobInfo2.pDocument = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszDocumentName, cbDocumentName);

    *ppEnd -= cbDriverName;
    JobInfo2.pDriverName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrinter->pwszPrinterDriver, cbDriverName);

    *ppEnd -= cbMachineName;
    JobInfo2.pMachineName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszMachineName, cbMachineName);

    *ppEnd -= cbNotifyName;
    JobInfo2.pNotifyName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszNotifyName, cbNotifyName);

    *ppEnd -= cbPrinterName;
    JobInfo2.pPrinterName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrinter->pwszPrinterName, cbPrinterName);

    *ppEnd -= cbPrintProcessor;
    JobInfo2.pPrintProcessor = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrintProcessor->pwszName, cbPrintProcessor);

    if (cbPrintProcessorParameters)
    {
        *ppEnd -= cbPrintProcessorParameters;
        JobInfo2.pParameters = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszPrintProcessorParameters, cbPrintProcessorParameters);
    }

    if (cbStatus)
    {
        *ppEnd -= cbStatus;
        JobInfo2.pStatus = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszStatus, cbStatus);
    }

    *ppEnd -= cbUserName;
    JobInfo2.pUserName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszUserName, cbUserName);

    // Time in JOB_INFO_2W is the number of milliseconds elapsed since the job was submitted. Calculate this time.
    if (!SystemTimeToFileTime(&pJob->stSubmitted, &ftSubmitted))
    {
        ERR("SystemTimeToFileTime failed with error %lu!\n", GetLastError());
        return FALSE;
    }

    GetSystemTimeAsFileTime(&ftNow);
    uliSubmitted.LowPart = ftSubmitted.dwLowDateTime;
    uliSubmitted.HighPart = ftSubmitted.dwHighDateTime;
    uliNow.LowPart = ftNow.dwLowDateTime;
    uliNow.HighPart = ftNow.dwHighDateTime;
    JobInfo2.Time = (DWORD)((uliNow.QuadPart - uliSubmitted.QuadPart) / 10000);

    // Position in JOB_INFO_2W is the 1-based index of the job in the processing queue.
    // Retrieve this through the element index of the job in the Printer's Job List.
    if (!LookupElementSkiplist(&pJob->pPrinter->JobList, pJob, &JobInfo2.Position))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("pJob could not be located in the Printer's Job List!\n");
        goto Cleanup;
    }

    // Make the index 1-based.
    ++JobInfo2.Position;

    // Fill the rest of the structure.
    JobInfo2.JobId = pJob->dwJobID;
    JobInfo2.PagesPrinted = pJob->dwPagesPrinted;
    JobInfo2.Priority = pJob->dwPriority;
    JobInfo2.StartTime = pJob->dwStartTime;
    JobInfo2.Status = pJob->dwStatus;
    JobInfo2.TotalPages = pJob->dwTotalPages;
    JobInfo2.UntilTime = pJob->dwUntilTime;
    CopyMemory(&JobInfo2.Submitted, &pJob->stSubmitted, sizeof(SYSTEMTIME));

    // Finally copy the structure to the output pointer.
    CopyMemory(*ppStart, &JobInfo2, sizeof(JOB_INFO_2W));
    *ppStart += sizeof(JOB_INFO_2W);
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

BOOL WINAPI
LocalGetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pStart, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pEnd = &pStart[cbBuf];
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // Get the desired job.
    pJob = LookupElementSkiplist(&GlobalJobList, &JobId, NULL);
    if (!pJob || pJob->pPrinter != pPrinterHandle->pPrinter)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Begin counting.
    *pcbNeeded = 0;

    // The function behaves differently for each level.
    if (Level == 1)
        dwErrorCode = _LocalGetJobLevel1(pPrinterHandle, pJob, &pStart, &pEnd, cbBuf, pcbNeeded);
    else if (Level == 2)
        dwErrorCode = _LocalGetJobLevel2(pPrinterHandle, pJob, &pStart, &pEnd, cbBuf, pcbNeeded);
    else
        dwErrorCode = ERROR_INVALID_LEVEL;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

static DWORD
_LocalSetJobLevel1(PLOCAL_PRINTER_HANDLE pPrinterHandle, PLOCAL_JOB pJob, PJOB_INFO_1W pJobInfo)
{
    DWORD dwErrorCode;

    // First check the validity of the input before changing anything.
    if (!FindDatatype(pJob->pPrintProcessor, pJobInfo->pDatatype))
    {
        dwErrorCode = ERROR_INVALID_DATATYPE;
        goto Cleanup;
    }

    // Check if the datatype has changed.
    if (wcscmp(pJob->pwszDatatype, pJobInfo->pDatatype) != 0)
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDatatype, pJobInfo->pDatatype))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the document name has changed.
    if (wcscmp(pJob->pwszDocumentName, pJobInfo->pDocument) != 0)
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDocumentName, pJobInfo->pDocument))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the status message has changed.
    if ((!pJob->pwszStatus && pJobInfo->pStatus) || wcscmp(pJob->pwszStatus, pJobInfo->pStatus) != 0)
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszStatus, pJobInfo->pStatus))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the user name has changed.
    if (wcscmp(pJob->pwszUserName, pJobInfo->pUserName) != 0)
    {
        // The new user name doesn't need to exist, so no additional verification is required.

        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszUserName, pJobInfo->pUserName))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the priority has changed.
    if (pJob->dwPriority != pJobInfo->Priority && IS_VALID_PRIORITY(pJobInfo->Priority))
    {
        // Set the new priority.
        pJob->dwPriority = pJobInfo->Priority;

        // Remove and reinsert the job in the Printer's Job List.
        // The Compare function will be used to find the right position now considering the new priority.
        DeleteElementSkiplist(&pJob->pPrinter->JobList, pJob);
        InsertElementSkiplist(&pJob->pPrinter->JobList, pJob);
    }

    // Check if the status flags have changed.
    if (pJob->dwStatus != pJobInfo->Status)
    {
        // Only add status flags that make sense.
        if (pJobInfo->Status & JOB_STATUS_PAUSED)
            pJob->dwStatus |= JOB_STATUS_PAUSED;

        if (pJobInfo->Status & JOB_STATUS_ERROR)
            pJob->dwStatus |= JOB_STATUS_ERROR;

        if (pJobInfo->Status & JOB_STATUS_OFFLINE)
            pJob->dwStatus |= JOB_STATUS_OFFLINE;

        if (pJobInfo->Status & JOB_STATUS_PAPEROUT)
            pJob->dwStatus |= JOB_STATUS_PAPEROUT;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

static DWORD
_LocalSetJobLevel2(PLOCAL_PRINTER_HANDLE pPrinterHandle, PLOCAL_JOB pJob, PJOB_INFO_2W pJobInfo)
{
    DWORD dwErrorCode;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;

    // First check the validity of the input before changing anything.
    pPrintProcessor = FindPrintProcessor(pJobInfo->pPrintProcessor);
    if (!pPrintProcessor)
    {
        dwErrorCode = ERROR_UNKNOWN_PRINTPROCESSOR;
        goto Cleanup;
    }

    if (!FindDatatype(pPrintProcessor, pJobInfo->pDatatype))
    {
        dwErrorCode = ERROR_INVALID_DATATYPE;
        goto Cleanup;
    }

    // Check if the datatype has changed.
    if (wcscmp(pJob->pwszDatatype, pJobInfo->pDatatype) != 0)
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDatatype, pJobInfo->pDatatype))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the document name has changed.
    if (wcscmp(pJob->pwszDocumentName, pJobInfo->pDocument) != 0)
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDocumentName, pJobInfo->pDocument))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the notify name has changed.
    if (wcscmp(pJob->pwszNotifyName, pJobInfo->pNotifyName) != 0)
    {
        // The new notify name doesn't need to exist, so no additional verification is required.

        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszNotifyName, pJobInfo->pNotifyName))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the (optional) Print Processor Parameters have changed.
    if ((!pJob->pwszPrintProcessorParameters && pJobInfo->pParameters) || wcscmp(pJob->pwszPrintProcessorParameters, pJobInfo->pParameters) != 0)
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszPrintProcessorParameters, pJobInfo->pParameters))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the (optional) Status Message has changed.
    if ((!pJob->pwszStatus && pJobInfo->pStatus) || wcscmp(pJob->pwszStatus, pJobInfo->pStatus) != 0)
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszStatus, pJobInfo->pStatus))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the user name has changed.
    if (wcscmp(pJob->pwszUserName, pJobInfo->pUserName) != 0)
    {
        // The new user name doesn't need to exist, so no additional verification is required.

        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszUserName, pJobInfo->pUserName))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the priority has changed.
    if (pJob->dwPriority != pJobInfo->Priority && IS_VALID_PRIORITY(pJobInfo->Priority))
    {
        // Set the new priority.
        pJob->dwPriority = pJobInfo->Priority;

        // Remove and reinsert the job in the Printer's Job List.
        // The Compare function will be used to find the right position now considering the new priority.
        DeleteElementSkiplist(&pJob->pPrinter->JobList, pJob);
        InsertElementSkiplist(&pJob->pPrinter->JobList, pJob);
    }

    // Check if the status flags have changed.
    if (pJob->dwStatus != pJobInfo->Status)
    {
        // Only add status flags that make sense.
        if (pJobInfo->Status & JOB_STATUS_PAUSED)
            pJob->dwStatus |= JOB_STATUS_PAUSED;

        if (pJobInfo->Status & JOB_STATUS_ERROR)
            pJob->dwStatus |= JOB_STATUS_ERROR;

        if (pJobInfo->Status & JOB_STATUS_OFFLINE)
            pJob->dwStatus |= JOB_STATUS_OFFLINE;

        if (pJobInfo->Status & JOB_STATUS_PAPEROUT)
            pJob->dwStatus |= JOB_STATUS_PAPEROUT;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

BOOL WINAPI
LocalSetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command)
{
    const WCHAR wszFolder[] = L"\\PRINTERS\\";
    const DWORD cchFolder = _countof(wszFolder) - 1;

    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;
    WCHAR wszFullPath[MAX_PATH];

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // Get the desired job.
    pJob = LookupElementSkiplist(&GlobalJobList, &JobId, NULL);
    if (!pJob || pJob->pPrinter != pPrinterHandle->pPrinter)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Setting job information is handled differently for each level.
    if (Level)
    {
        if (Level == 1)
            dwErrorCode = _LocalSetJobLevel1(pPrinterHandle, pJob, (PJOB_INFO_1W)pJobInfo);
        else if (Level == 2)
            dwErrorCode = _LocalSetJobLevel2(pPrinterHandle, pJob, (PJOB_INFO_2W)pJobInfo);
        else
            dwErrorCode = ERROR_INVALID_LEVEL;
    }

    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Construct the full path to the shadow file.
    CopyMemory(wszFullPath, wszSpoolDirectory, cchSpoolDirectory * sizeof(WCHAR));
    CopyMemory(&wszFullPath[cchSpoolDirectory], wszFolder, cchFolder * sizeof(WCHAR));
    swprintf(&wszFullPath[cchSpoolDirectory + cchFolder], L"%05lu.SHD", JobId);

    // Write the job data into the shadow file.
    WriteJobShadowFile(wszFullPath, pJob);

    // Perform an additional command if desired.
    if (Command)
    {
        // TODO
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalEnumJobs(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pStart, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pEnd = &pStart[cbBuf];
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PSKIPLIST_NODE pFirstJobNode;
    PSKIPLIST_NODE pNode;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // Check the level.
    if (Level > 2)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // Lookup the node of the first job requested by the caller in the Printer's Job List.
    pFirstJobNode = LookupNodeByIndexSkiplist(&pPrinterHandle->pPrinter->JobList, FirstJob);

    // Count the required buffer size and the number of jobs.
    pNode = pFirstJobNode;

    while (*pcReturned < NoJobs && pNode)
    {
        pJob = (PLOCAL_JOB)pNode->Element;

        // The function behaves differently for each level.
        if (Level == 1)
            _LocalGetJobLevel1(pPrinterHandle, pJob, NULL, NULL, 0, pcbNeeded);
        else if (Level == 2)
            _LocalGetJobLevel2(pPrinterHandle, pJob, NULL, NULL, 0, pcbNeeded);

        // We stop either when there are no more jobs in the list or when the caller didn't request more, whatever comes first.
        (*pcReturned)++;
        pNode = pNode->Next[0];
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Begin counting again and also empty the given buffer.
    *pcbNeeded = 0;
    *pcReturned = 0;
    ZeroMemory(pStart, cbBuf);

    // Now call the same functions again to copy the actual data for each job into the buffer.
    pNode = pFirstJobNode;

    while (*pcReturned < NoJobs && pNode)
    {
        pJob = (PLOCAL_JOB)pNode->Element;

        // The function behaves differently for each level.
        if (Level == 1)
            dwErrorCode = _LocalGetJobLevel1(pPrinterHandle, pJob, &pStart, &pEnd, cbBuf, pcbNeeded);
        else if (Level == 2)
            dwErrorCode = _LocalGetJobLevel2(pPrinterHandle, pJob, &pStart, &pEnd, cbBuf, pcbNeeded);

        if (dwErrorCode != ERROR_SUCCESS)
            goto Cleanup;

        // We stop either when there are no more jobs in the list or when the caller didn't request more, whatever comes first.
        (*pcReturned)++;
        pNode = pNode->Next[0];
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalScheduleJob(HANDLE hPrinter, DWORD dwJobID)
{
    const WCHAR wszFolder[] = L"\\PRINTERS\\";
    const DWORD cchFolder = _countof(wszFolder) - 1;

    DWORD dwAttributes;
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;
    WCHAR wszFullPath[MAX_PATH];

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // Check if the Job ID is valid.
    pJob = LookupElementSkiplist(&GlobalJobList, &dwJobID, NULL);
    if (!pJob || pJob->pPrinter != pPrinterHandle->pPrinter)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Construct the full path to the spool file.
    CopyMemory(wszFullPath, wszSpoolDirectory, cchSpoolDirectory * sizeof(WCHAR));
    CopyMemory(&wszFullPath[cchSpoolDirectory], wszFolder, cchFolder * sizeof(WCHAR));
    swprintf(&wszFullPath[cchSpoolDirectory + cchFolder], L"%05lu.SPL", dwJobID);

    // Check if it exists.
    dwAttributes = GetFileAttributesW(wszFullPath);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES || dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        dwErrorCode = ERROR_SPOOL_FILE_NOT_FOUND;
        goto Cleanup;
    }

    // Switch from spooling to printing.
    pJob->dwStatus &= ~JOB_STATUS_SPOOLING;
    pJob->dwStatus |= JOB_STATUS_PRINTING;

    // Write the job data into the shadow file.
    wcscpy(wcsrchr(wszFullPath, L'.'), L".SHD");
    WriteJobShadowFile(wszFullPath, pJob);

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

PLOCAL_JOB
ReadJobShadowFile(PCWSTR pwszFilePath)
{
    DWORD cbFileSize;
    DWORD cbRead;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PLOCAL_JOB pJob;
    PLOCAL_JOB pReturnValue = NULL;
    PLOCAL_PRINTER pPrinter;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    PSHD_HEADER pShadowFile = NULL;
    PWSTR pwszPrinterName;
    PWSTR pwszPrintProcessor;

    // Try to open the file.
    hFile = CreateFileW(pwszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW failed with error %lu for file \"%S\"!\n", GetLastError(), pwszFilePath);
        goto Cleanup;
    }

    // Get its file size (small enough for a single DWORD) and allocate memory for all of it.
    cbFileSize = GetFileSize(hFile, NULL);
    pShadowFile = DllAllocSplMem(cbFileSize);
    if (!pShadowFile)
    {
        ERR("DllAllocSplMem failed with error %lu for file \"%S\"!\n", GetLastError(), pwszFilePath);
        goto Cleanup;
    }

    // Read the entire file.
    if (!ReadFile(hFile, pShadowFile, cbFileSize, &cbRead, NULL))
    {
        ERR("ReadFile failed with error %lu for file \"%S\"!\n", GetLastError(), pwszFilePath);
        goto Cleanup;
    }

    // Check signature and header size.
    if (pShadowFile->dwSignature != SHD_WIN2003_SIGNATURE || pShadowFile->cbHeader != sizeof(SHD_HEADER))
    {
        ERR("Signature or Header Size mismatch for file \"%S\"!\n", pwszFilePath);
        goto Cleanup;
    }

    // Retrieve the associated printer from the list.
    pwszPrinterName = (PWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offPrinterName);
    pPrinter = LookupElementSkiplist(&PrinterList, &pwszPrinterName, NULL);
    if (!pPrinter)
    {
        ERR("Shadow file \"%S\" references a non-existing printer \"%S\"!\n", pwszFilePath, pwszPrinterName);
        goto Cleanup;
    }

    // Retrieve the associated Print Processor from the list.
    pwszPrintProcessor = (PWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offPrintProcessor);
    pPrintProcessor = FindPrintProcessor(pwszPrintProcessor);
    if (!pPrintProcessor)
    {
        ERR("Shadow file \"%S\" references a non-existing Print Processor \"%S\"!\n", pwszFilePath, pwszPrintProcessor);
        goto Cleanup;
    }

    // Create a new job structure and copy over the relevant fields.
    pJob = DllAllocSplMem(sizeof(LOCAL_JOB));
    if (!pJob)
    {
        ERR("DllAllocSplMem failed with error %lu for file \"%S\"!\n", GetLastError(), pwszFilePath);
        goto Cleanup;
    }

    pJob->dwJobID = pShadowFile->dwJobID;
    pJob->dwPriority = pShadowFile->dwPriority;
    pJob->dwStartTime = pShadowFile->dwStartTime;
    pJob->dwTotalPages = pShadowFile->dwTotalPages;
    pJob->dwUntilTime = pShadowFile->dwUntilTime;    
    pJob->pPrinter = pPrinter;
    pJob->pPrintProcessor = pPrintProcessor;
    pJob->pwszDatatype = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offDatatype));
    pJob->pwszDocumentName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offDocumentName));
    pJob->pwszMachineName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offMachineName));
    pJob->pwszNotifyName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offNotifyName));

    if (pShadowFile->offPrintProcessorParameters)
        pJob->pwszPrintProcessorParameters = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offPrintProcessorParameters));

    pJob->pwszUserName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offUserName));

    CopyMemory(&pJob->stSubmitted, &pShadowFile->stSubmitted, sizeof(SYSTEMTIME));
    CopyMemory(&pJob->DevMode, (PDEVMODEW)((ULONG_PTR)pShadowFile + pShadowFile->offDevMode), sizeof(DEVMODEW));

    pReturnValue = pJob;

Cleanup:
    if (pShadowFile)
        DllFreeSplMem(pShadowFile);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return pReturnValue;
}

BOOL
WriteJobShadowFile(PWSTR pwszFilePath, const PLOCAL_JOB pJob)
{
    BOOL bReturnValue = FALSE;
    DWORD cbDatatype;
    DWORD cbDocumentName;
    DWORD cbFileSize;
    DWORD cbMachineName;
    DWORD cbNotifyName;
    DWORD cbPrinterDriver;
    DWORD cbPrinterName;
    DWORD cbPrintProcessor;
    DWORD cbPrintProcessorParameters = 0;
    DWORD cbUserName;
    DWORD cbWritten;
    DWORD dwCurrentOffset;
    HANDLE hSHDFile = INVALID_HANDLE_VALUE;
    HANDLE hSPLFile = INVALID_HANDLE_VALUE;
    PSHD_HEADER pShadowFile = NULL;

    // Try to open the SHD file.
    hSHDFile = CreateFileW(pwszFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (hSHDFile == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW failed with error %lu for file \"%S\"!\n", GetLastError(), pwszFilePath);
        goto Cleanup;
    }

    // Compute the total size of the shadow file.
    cbDatatype = (wcslen(pJob->pwszDatatype) + 1) * sizeof(WCHAR);
    cbDocumentName = (wcslen(pJob->pwszDocumentName) + 1) * sizeof(WCHAR);
    cbMachineName = (wcslen(pJob->pwszMachineName) + 1) * sizeof(WCHAR);
    cbNotifyName = (wcslen(pJob->pwszNotifyName) + 1) * sizeof(WCHAR);
    cbPrinterDriver = (wcslen(pJob->pPrinter->pwszPrinterDriver) + 1) * sizeof(WCHAR);
    cbPrinterName = (wcslen(pJob->pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
    cbPrintProcessor = (wcslen(pJob->pPrintProcessor->pwszName) + 1) * sizeof(WCHAR);
    cbUserName = (wcslen(pJob->pwszUserName) + 1) * sizeof(WCHAR);

    // Print Processor Parameters are optional.
    if (pJob->pwszPrintProcessorParameters)
        cbPrintProcessorParameters = (wcslen(pJob->pwszPrintProcessorParameters) + 1) * sizeof(WCHAR);

    cbFileSize = sizeof(SHD_HEADER) + cbDatatype + cbDocumentName + sizeof(DEVMODEW) + cbMachineName + cbNotifyName + cbPrinterDriver + cbPrinterName + cbPrintProcessor + cbPrintProcessorParameters + cbUserName;

    // Allocate memory for it.
    pShadowFile = DllAllocSplMem(cbFileSize);
    if (!pShadowFile)
    {
        ERR("DllAllocSplMem failed with error %lu for file \"%S\"!\n", GetLastError(), pwszFilePath);
        goto Cleanup;
    }

    // Fill out the shadow file header information.
    pShadowFile->dwSignature = SHD_WIN2003_SIGNATURE;
    pShadowFile->cbHeader = sizeof(SHD_HEADER);

    // Copy the values.
    pShadowFile->dwJobID = pJob->dwJobID;
    pShadowFile->dwPriority = pJob->dwPriority;
    pShadowFile->dwStartTime = pJob->dwStartTime;
    pShadowFile->dwTotalPages = pJob->dwTotalPages;
    pShadowFile->dwUntilTime = pJob->dwUntilTime;
    CopyMemory(&pShadowFile->stSubmitted, &pJob->stSubmitted, sizeof(SYSTEMTIME));

    // Determine the file size of the .SPL file
    wcscpy(wcsrchr(pwszFilePath, L'.'), L".SPL");
    hSPLFile = CreateFileW(pwszFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hSPLFile != INVALID_HANDLE_VALUE)
        pShadowFile->dwSPLSize = GetFileSize(hSPLFile, NULL);

    // Add the extra values that are stored as offsets in the shadow file.
    // The first value begins right after the shadow file header.
    dwCurrentOffset = sizeof(SHD_HEADER);

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszDatatype, cbDatatype);
    pShadowFile->offDatatype = dwCurrentOffset;
    dwCurrentOffset += cbDatatype;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszDocumentName, cbDocumentName);
    pShadowFile->offDocumentName = dwCurrentOffset;
    dwCurrentOffset += cbDocumentName;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, &pJob->DevMode, sizeof(DEVMODEW));
    pShadowFile->offDevMode = dwCurrentOffset;
    dwCurrentOffset += sizeof(DEVMODEW);

    // offDriverName is only written, but automatically determined through offPrinterName when reading.
    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pPrinter->pwszPrinterDriver, cbPrinterDriver);
    pShadowFile->offDriverName = dwCurrentOffset;
    dwCurrentOffset += cbPrinterDriver;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszMachineName, cbMachineName);
    pShadowFile->offMachineName = dwCurrentOffset;
    dwCurrentOffset += cbMachineName;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszNotifyName, cbNotifyName);
    pShadowFile->offNotifyName = dwCurrentOffset;
    dwCurrentOffset += cbNotifyName;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pPrinter->pwszPrinterName, cbPrinterName);
    pShadowFile->offPrinterName = dwCurrentOffset;
    dwCurrentOffset += cbPrinterName;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pPrintProcessor->pwszName, cbPrintProcessor);
    pShadowFile->offPrintProcessor = dwCurrentOffset;
    dwCurrentOffset += cbPrintProcessor;

    if (cbPrintProcessorParameters)
    {
        CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszPrintProcessorParameters, cbPrintProcessorParameters);
        pShadowFile->offPrintProcessorParameters = dwCurrentOffset;
        dwCurrentOffset += cbPrintProcessorParameters;
    }

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszUserName, cbUserName);
    pShadowFile->offUserName = dwCurrentOffset;
    dwCurrentOffset += cbUserName;

    // Write the file.
    if (!WriteFile(hSHDFile, pShadowFile, cbFileSize, &cbWritten, NULL))
    {
        ERR("WriteFile failed with error %lu for file \"%S\"!\n", GetLastError(), pwszFilePath);
        goto Cleanup;
    }

    bReturnValue = TRUE;

Cleanup:
    if (pShadowFile)
        DllFreeSplMem(pShadowFile);

    if (hSHDFile != INVALID_HANDLE_VALUE)
        CloseHandle(hSHDFile);

    if (hSPLFile != INVALID_HANDLE_VALUE)
        CloseHandle(hSPLFile);

    return bReturnValue;
}

BOOL
FreeJob(PLOCAL_JOB pJob)
{
    ////////// TODO /////////
    /// Add some checks
    DllFreeSplStr(pJob->pwszDatatype);
    DllFreeSplStr(pJob->pwszDocumentName);
    DllFreeSplStr(pJob->pwszOutputFile);
    DllFreeSplMem(pJob);

    return TRUE;
}
