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
 * @name _EqualStrings
 *
 * Returns whether two strings are equal.
 * Unlike wcscmp, this function also works with NULL strings.
 *
 * @param pwszA
 * First string to compare.
 *
 * @param pwszB
 * Second string to compare.
 *
 * @return
 * TRUE if the strings are equal, FALSE if they differ.
 */
static __inline BOOL
_EqualStrings(PCWSTR pwszA, PCWSTR pwszB)
{
    if (!pwszA && !pwszB)
        return TRUE;

    if (pwszA && !pwszB)
        return FALSE;

    if (!pwszA && pwszB)
        return FALSE;

    return (wcscmp(pwszA, pwszB) == 0);
}

static BOOL
_GetNextJobID(PDWORD dwJobID)
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

DWORD
GetJobFilePath(PCWSTR pwszExtension, DWORD dwJobID, PWSTR pwszOutput)
{
    const WCHAR wszPrintersPath[] = L"\\PRINTERS\\";
    const DWORD cchPrintersPath = _countof(wszPrintersPath) - 1;
    const DWORD cchSpoolerFile = sizeof("?????.") - 1;
    const DWORD cchExtension = sizeof("SPL") - 1;                   // pwszExtension may be L"SPL" or L"SHD", same length for both!

    if (pwszOutput)
    {
        CopyMemory(pwszOutput, wszSpoolDirectory, cchSpoolDirectory * sizeof(WCHAR));
        CopyMemory(&pwszOutput[cchSpoolDirectory], wszPrintersPath, cchPrintersPath * sizeof(WCHAR));
        swprintf(&pwszOutput[cchSpoolDirectory + cchPrintersPath], L"%05lu.", dwJobID);
        CopyMemory(&pwszOutput[cchSpoolDirectory + cchPrintersPath + cchSpoolerFile], pwszExtension, (cchExtension + 1) * sizeof(WCHAR));
    }

    return (cchSpoolDirectory + cchPrintersPath + cchSpoolerFile + cchExtension + 1) * sizeof(WCHAR);
}

BOOL
InitializeGlobalJobList()
{
    const WCHAR wszPath[] = L"\\PRINTERS\\?????.SHD";
    const DWORD cchPath = _countof(wszPath) - 1;

    DWORD dwErrorCode;
    DWORD dwJobID;
    HANDLE hFind;
    PLOCAL_JOB pJob = NULL;
    PWSTR p;
    WCHAR wszFullPath[MAX_PATH];
    WIN32_FIND_DATAW FindData;

    // This one is incremented in _GetNextJobID.
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
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    do
    {
        // Skip possible subdirectories.
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        // Extract the Job ID and verify the file name format at the same time.
        // This includes all valid names (like "00005.SHD") and excludes invalid ones (like "10ABC.SHD").
        dwJobID = wcstoul(FindData.cFileName, &p, 10);
        if (!IS_VALID_JOB_ID(dwJobID))
            continue;

        if (wcsicmp(p, L".SHD") != 0)
            continue;

        // This shadow file has a valid name. Construct the full path and try to load it.
        GetJobFilePath(L"SHD", dwJobID, wszFullPath);
        pJob = ReadJobShadowFile(wszFullPath);
        if (!pJob)
            continue;

        // Add it to the Global Job List.
        if (!InsertElementSkiplist(&GlobalJobList, pJob))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("InsertElementSkiplist failed for job %lu for the GlobalJobList!\n", pJob->dwJobID);
            goto Cleanup;
        }

        // Add it to the Printer's Job List.
        if (!InsertElementSkiplist(&pJob->pPrinter->JobList, pJob))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("InsertElementSkiplist failed for job %lu for the Printer's Job List!\n", pJob->dwJobID);
            goto Cleanup;
        }
    }
    while (FindNextFileW(hFind, &FindData));

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    // Outside the loop
    if (hFind)
        FindClose(hFind);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

void
InitializePrinterJobList(PLOCAL_PRINTER pPrinter)
{
    // Initialize an empty list for this printer's jobs.
    // This one is only for sorting the jobs. If you need to lookup a job, search the GlobalJobList by Job ID.
    InitializeSkiplist(&pPrinter->JobList, DllAllocSplMem, _PrinterJobListCompareRoutine, (PSKIPLIST_FREE_ROUTINE)DllFreeSplMem);
}

DWORD WINAPI
CreateJob(PLOCAL_PRINTER_HANDLE pPrinterHandle)
{
    const WCHAR wszDoubleBackslash[] = L"\\";
    const DWORD cchDoubleBackslash = _countof(wszDoubleBackslash) - 1;

    DWORD cchMachineName;
    DWORD cchUserName;
    DWORD dwErrorCode;
    PLOCAL_JOB pJob;
    RPC_BINDING_HANDLE hServerBinding = NULL;
    RPC_WSTR pwszBinding = NULL;
    RPC_WSTR pwszMachineName = NULL;

    // Create a new job.
    pJob = DllAllocSplMem(sizeof(LOCAL_JOB));
    if (!pJob)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Reserve an ID for this job.
    if (!_GetNextJobID(&pJob->dwJobID))
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
    pJob->pDevMode = DuplicateDevMode(pPrinterHandle->pDevMode);
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
    CopyMemory(&pJob->pwszMachineName[cchDoubleBackslash], pwszMachineName, (cchMachineName + 1) * sizeof(WCHAR));

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

    // We were successful!
    pPrinterHandle->bStartedDoc = TRUE;
    pPrinterHandle->pJob = pJob;
    dwErrorCode = ERROR_SUCCESS;

    // Don't let the cleanup routine free this.
    pJob = NULL;

Cleanup:
    if (pJob)
        DllFreeSplMem(pJob);

    if (pwszMachineName)
        RpcStringFreeW(&pwszMachineName);

    if (pwszBinding)
        RpcStringFreeW(&pwszBinding);

    if (hServerBinding)
        RpcBindingFree(&hServerBinding);

    return dwErrorCode;
}

BOOL WINAPI
LocalAddJob(HANDLE hPrinter, DWORD Level, PBYTE pData, DWORD cbBuf, PDWORD pcbNeeded)
{
    ADDJOB_INFO_1W AddJobInfo1;
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Check if this is a printer handle.
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // This handle must not have started a job yet!
    if (pPrinterHandle->pJob)
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
    *pcbNeeded = sizeof(ADDJOB_INFO_1W) + GetJobFilePath(L"SPL", 0, NULL);
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // All requirements are met - create a new job.
    dwErrorCode = CreateJob(pPrinterHandle);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Mark that this job was started with AddJob (so that it can be scheduled for printing with ScheduleJob).
    pPrinterHandle->pJob->bAddedJob = TRUE;

    // Return a proper ADDJOB_INFO_1W structure.
    AddJobInfo1.JobId = pPrinterHandle->pJob->dwJobID;
    AddJobInfo1.Path = (PWSTR)(pData + sizeof(ADDJOB_INFO_1W));

    CopyMemory(pData, &AddJobInfo1, sizeof(ADDJOB_INFO_1W));
    GetJobFilePath(L"SPL", AddJobInfo1.JobId, AddJobInfo1.Path);

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}


static DWORD
_LocalGetJobLevel1(PLOCAL_PRINTER_HANDLE pPrinterHandle, PLOCAL_JOB pJob, PBYTE* ppStart, PBYTE* ppEnd, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD cbDatatype = (wcslen(pJob->pwszDatatype) + 1) * sizeof(WCHAR);
    DWORD cbDocumentName = 0;  
    DWORD cbMachineName = (wcslen(pJob->pwszMachineName) + 1) * sizeof(WCHAR);
    DWORD cbPrinterName = (wcslen(pJob->pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
    DWORD cbStatus = 0;
    DWORD cbUserName = 0;
    DWORD dwErrorCode;
    JOB_INFO_1W JobInfo1 = { 0 };

    // Calculate the lengths of the optional values.
    if (pJob->pwszDocumentName)
        cbDocumentName = (wcslen(pJob->pwszDocumentName) + 1) * sizeof(WCHAR);

    if (pJob->pwszStatus)
        cbStatus = (wcslen(pJob->pwszStatus) + 1) * sizeof(WCHAR);

    if (pJob->pwszUserName)
        cbUserName = (wcslen(pJob->pwszUserName) + 1) * sizeof(WCHAR);

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

    *ppEnd -= cbMachineName;
    JobInfo1.pMachineName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszMachineName, cbMachineName);

    *ppEnd -= cbPrinterName;
    JobInfo1.pPrinterName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrinter->pwszPrinterName, cbPrinterName);

    // Copy the optional values.
    if (cbDocumentName)
    {
        *ppEnd -= cbDocumentName;
        JobInfo1.pDocument = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszDocumentName, cbDocumentName);
    }

    if (cbStatus)
    {
        *ppEnd -= cbStatus;
        JobInfo1.pStatus = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszStatus, cbStatus);
    }

    if (cbUserName)
    {
        *ppEnd -= cbUserName;
        JobInfo1.pUserName = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszUserName, cbUserName);
    }

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
    DWORD cbDevMode = pJob->pDevMode->dmSize + pJob->pDevMode->dmDriverExtra;
    DWORD cbDocumentName = 0;
    DWORD cbDriverName = (wcslen(pJob->pPrinter->pwszPrinterDriver) + 1) * sizeof(WCHAR);
    DWORD cbMachineName = (wcslen(pJob->pwszMachineName) + 1) * sizeof(WCHAR);
    DWORD cbNotifyName = 0;
    DWORD cbPrinterName = (wcslen(pJob->pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
    DWORD cbPrintProcessor = (wcslen(pJob->pPrintProcessor->pwszName) + 1) * sizeof(WCHAR);
    DWORD cbPrintProcessorParameters = 0;
    DWORD cbStatus = 0;
    DWORD cbUserName = 0;
    DWORD dwErrorCode;
    FILETIME ftNow;
    FILETIME ftSubmitted;
    JOB_INFO_2W JobInfo2 = { 0 };
    ULARGE_INTEGER uliNow;
    ULARGE_INTEGER uliSubmitted;

    // Calculate the lengths of the optional values.
    if (pJob->pwszDocumentName)
        cbDocumentName = (wcslen(pJob->pwszDocumentName) + 1) * sizeof(WCHAR);

    if (pJob->pwszNotifyName)
        cbNotifyName = (wcslen(pJob->pwszNotifyName) + 1) * sizeof(WCHAR);

    if (pJob->pwszPrintProcessorParameters)
        cbPrintProcessorParameters = (wcslen(pJob->pwszPrintProcessorParameters) + 1) * sizeof(WCHAR);

    if (pJob->pwszStatus)
        cbStatus = (wcslen(pJob->pwszStatus) + 1) * sizeof(WCHAR);

    if (pJob->pwszUserName)
        cbUserName = (wcslen(pJob->pwszUserName) + 1) * sizeof(WCHAR);

    // Check if the supplied buffer is large enough.
    *pcbNeeded += sizeof(JOB_INFO_2W) + cbDatatype + cbDevMode + cbDocumentName + cbDriverName + cbMachineName + cbNotifyName + cbPrinterName + cbPrintProcessor + cbPrintProcessorParameters + cbStatus + cbUserName;
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Put the strings at the end of the buffer.
    *ppEnd -= cbDatatype;
    JobInfo2.pDatatype = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszDatatype, cbDatatype);

    *ppEnd -= cbDevMode;
    JobInfo2.pDevMode = (PDEVMODEW)*ppEnd;
    CopyMemory(*ppEnd, pJob->pDevMode, cbDevMode);

    *ppEnd -= cbDriverName;
    JobInfo2.pDriverName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrinter->pwszPrinterDriver, cbDriverName);

    *ppEnd -= cbMachineName;
    JobInfo2.pMachineName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pwszMachineName, cbMachineName);

    *ppEnd -= cbPrinterName;
    JobInfo2.pPrinterName = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrinter->pwszPrinterName, cbPrinterName);

    *ppEnd -= cbPrintProcessor;
    JobInfo2.pPrintProcessor = (PWSTR)*ppEnd;
    CopyMemory(*ppEnd, pJob->pPrintProcessor->pwszName, cbPrintProcessor);

    // Copy the optional values.
    if (cbDocumentName)
    {
        *ppEnd -= cbDocumentName;
        JobInfo2.pDocument = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszDocumentName, cbDocumentName);
    }

    if (cbNotifyName)
    {
        *ppEnd -= cbNotifyName;
        JobInfo2.pNotifyName = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszNotifyName, cbNotifyName);
    }

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

    if (cbUserName)
    {
        *ppEnd -= cbUserName;
        JobInfo2.pUserName = (PWSTR)*ppEnd;
        CopyMemory(*ppEnd, pJob->pwszUserName, cbUserName);
    }

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
    if (pHandle->HandleType != HandleType_Printer)
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
    if (!_EqualStrings(pJob->pwszDatatype, pJobInfo->pDatatype))
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDatatype, pJobInfo->pDatatype))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the document name has changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszDocumentName, pJobInfo->pDocument))
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDocumentName, pJobInfo->pDocument))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the status message has changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszStatus, pJobInfo->pStatus))
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszStatus, pJobInfo->pStatus))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the user name has changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszUserName, pJobInfo->pUserName) != 0)
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
    if (!_EqualStrings(pJob->pwszDatatype, pJobInfo->pDatatype))
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDatatype, pJobInfo->pDatatype))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the document name has changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszDocumentName, pJobInfo->pDocument))
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszDocumentName, pJobInfo->pDocument))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the notify name has changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszNotifyName, pJobInfo->pNotifyName))
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

    // Check if the Print Processor Parameters have changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszPrintProcessorParameters, pJobInfo->pParameters))
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszPrintProcessorParameters, pJobInfo->pParameters))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the Status Message has changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszStatus, pJobInfo->pStatus))
    {
        // Use the new value.
        if (!ReallocSplStr(&pJob->pwszStatus, pJobInfo->pStatus))
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    // Check if the user name has changed. An empty string is permitted here!
    if (!_EqualStrings(pJob->pwszUserName, pJobInfo->pUserName))
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
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;
    WCHAR wszFullPath[MAX_PATH];

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
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

    // If we do spooled printing, the job information is written down into a shadow file.
    if (!(pPrinterHandle->pPrinter->dwAttributes & PRINTER_ATTRIBUTE_DIRECT))
    {
        // Write the job data into the shadow file.
        GetJobFilePath(L"SHD", JobId, wszFullPath);
        WriteJobShadowFile(wszFullPath, pJob);
    }

    // Perform an additional command if desired.
    if (Command)
    {
        if (Command == JOB_CONTROL_SENT_TO_PRINTER)
        {
            // This indicates the end of the Print Job.

            // Cancel the Job at the Print Processor.
            if (pJob->hPrintProcessor)
                pJob->pPrintProcessor->pfnControlPrintProcessor(pJob->hPrintProcessor, JOB_CONTROL_CANCEL);

            FreeJob(pJob);

            // TODO: All open handles associated with the job need to be invalidated.
            // This certainly needs handle tracking...
        }
        else
        {
            ERR("Unimplemented SetJob Command: %lu!\n", Command);
        }
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
    DWORD i;
    PBYTE pEnd = &pStart[cbBuf];
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PSKIPLIST_NODE pFirstJobNode;
    PSKIPLIST_NODE pNode;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
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
    i = 0;
    pNode = pFirstJobNode;

    while (i < NoJobs && pNode)
    {
        pJob = (PLOCAL_JOB)pNode->Element;

        // The function behaves differently for each level.
        if (Level == 1)
            _LocalGetJobLevel1(pPrinterHandle, pJob, NULL, NULL, 0, pcbNeeded);
        else if (Level == 2)
            _LocalGetJobLevel2(pPrinterHandle, pJob, NULL, NULL, 0, pcbNeeded);

        // We stop either when there are no more jobs in the list or when the caller didn't request more, whatever comes first.
        i++;
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
    ZeroMemory(pStart, cbBuf);

    // Now call the same functions again to copy the actual data for each job into the buffer.
    i = 0;
    pNode = pFirstJobNode;

    while (i < NoJobs && pNode)
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
        i++;
        pNode = pNode->Next[0];
    }

    *pcReturned = i;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalScheduleJob(HANDLE hPrinter, DWORD dwJobID)
{
    DWORD dwAttributes;
    DWORD dwErrorCode;
    HANDLE hThread;
    PLOCAL_JOB pJob;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;
    WCHAR wszFullPath[MAX_PATH];

    // Check if this is a printer handle.
    if (pHandle->HandleType != HandleType_Printer)
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

    // Check if this Job was started with AddJob.
    if (!pJob->bAddedJob)
    {
        dwErrorCode = ERROR_SPL_NO_ADDJOB;
        goto Cleanup;
    }

    // Construct the full path to the spool file.
    GetJobFilePath(L"SPL", dwJobID, wszFullPath);

    // Check if it exists.
    dwAttributes = GetFileAttributesW(wszFullPath);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES || dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        dwErrorCode = ERROR_SPOOL_FILE_NOT_FOUND;
        goto Cleanup;
    }

    // Spooling is finished at this point.
    pJob->dwStatus &= ~JOB_STATUS_SPOOLING;

    // Write the job data into the shadow file.
    wcscpy(wcsrchr(wszFullPath, L'.'), L".SHD");
    WriteJobShadowFile(wszFullPath, pJob);

    // Create the thread for performing the printing process.
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PrintingThreadProc, pJob, 0, NULL);
    if (!hThread)
    {
        dwErrorCode = GetLastError();
        ERR("CreateThread failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We don't need the thread handle. Keeping it open blocks the thread from terminating.
    CloseHandle(hThread);

    // ScheduleJob has done its job. The rest happens inside the thread.
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
    pJob->pDevMode = DuplicateDevMode((PDEVMODEW)((ULONG_PTR)pShadowFile + pShadowFile->offDevMode));
    pJob->pwszDatatype = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offDatatype));
    pJob->pwszMachineName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offMachineName));
    CopyMemory(&pJob->stSubmitted, &pShadowFile->stSubmitted, sizeof(SYSTEMTIME));

    // Copy the optional values.
    if (pShadowFile->offDocumentName)
        pJob->pwszDocumentName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offDocumentName));

    if (pShadowFile->offNotifyName)
        pJob->pwszNotifyName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offNotifyName));

    if (pShadowFile->offPrintProcessorParameters)
        pJob->pwszPrintProcessorParameters = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offPrintProcessorParameters));

    if (pShadowFile->offUserName)
        pJob->pwszUserName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offUserName));

    // Jobs read from shadow files were always added using AddJob.
    pJob->bAddedJob = TRUE;

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
    DWORD cbDatatype = (wcslen(pJob->pwszDatatype) + 1) * sizeof(WCHAR);
    DWORD cbDevMode = pJob->pDevMode->dmSize + pJob->pDevMode->dmDriverExtra;
    DWORD cbDocumentName = 0;
    DWORD cbFileSize;
    DWORD cbMachineName = (wcslen(pJob->pwszMachineName) + 1) * sizeof(WCHAR);
    DWORD cbNotifyName = 0;
    DWORD cbPrinterDriver = (wcslen(pJob->pPrinter->pwszPrinterDriver) + 1) * sizeof(WCHAR);
    DWORD cbPrinterName = (wcslen(pJob->pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
    DWORD cbPrintProcessor = (wcslen(pJob->pPrintProcessor->pwszName) + 1) * sizeof(WCHAR);
    DWORD cbPrintProcessorParameters = 0;
    DWORD cbUserName = 0;
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

    // Calculate the lengths of the optional values and the total size of the shadow file.
    if (pJob->pwszDocumentName)
        cbDocumentName = (wcslen(pJob->pwszDocumentName) + 1) * sizeof(WCHAR);

    if (pJob->pwszNotifyName)
        cbNotifyName = (wcslen(pJob->pwszNotifyName) + 1) * sizeof(WCHAR);

    if (pJob->pwszPrintProcessorParameters)
        cbPrintProcessorParameters = (wcslen(pJob->pwszPrintProcessorParameters) + 1) * sizeof(WCHAR);

    if (pJob->pwszUserName)
        cbUserName = (wcslen(pJob->pwszUserName) + 1) * sizeof(WCHAR);

    cbFileSize = sizeof(SHD_HEADER) + cbDatatype + cbDocumentName + cbDevMode + cbMachineName + cbNotifyName + cbPrinterDriver + cbPrinterName + cbPrintProcessor + cbPrintProcessorParameters + cbUserName;

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

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pDevMode, cbDevMode);
    pShadowFile->offDevMode = dwCurrentOffset;
    dwCurrentOffset += cbDevMode;

    // offDriverName is only written, but automatically determined through offPrinterName when reading.
    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pPrinter->pwszPrinterDriver, cbPrinterDriver);
    pShadowFile->offDriverName = dwCurrentOffset;
    dwCurrentOffset += cbPrinterDriver;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszMachineName, cbMachineName);
    pShadowFile->offMachineName = dwCurrentOffset;
    dwCurrentOffset += cbMachineName;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pPrinter->pwszPrinterName, cbPrinterName);
    pShadowFile->offPrinterName = dwCurrentOffset;
    dwCurrentOffset += cbPrinterName;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pPrintProcessor->pwszName, cbPrintProcessor);
    pShadowFile->offPrintProcessor = dwCurrentOffset;
    dwCurrentOffset += cbPrintProcessor;

    // Copy the optional values.
    if (cbDocumentName)
    {
        CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszDocumentName, cbDocumentName);
        pShadowFile->offDocumentName = dwCurrentOffset;
        dwCurrentOffset += cbDocumentName;
    }

    if (cbNotifyName)
    {
        CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszNotifyName, cbNotifyName);
        pShadowFile->offNotifyName = dwCurrentOffset;
        dwCurrentOffset += cbNotifyName;
    }

    if (cbPrintProcessorParameters)
    {
        CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszPrintProcessorParameters, cbPrintProcessorParameters);
        pShadowFile->offPrintProcessorParameters = dwCurrentOffset;
        dwCurrentOffset += cbPrintProcessorParameters;
    }

    if (cbUserName)
    {
        CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszUserName, cbUserName);
        pShadowFile->offUserName = dwCurrentOffset;
        dwCurrentOffset += cbUserName;
    }

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

void
FreeJob(PLOCAL_JOB pJob)
{
    PWSTR pwszSHDFile;
    
    // Remove the Job from both Job Lists.
    DeleteElementSkiplist(&pJob->pPrinter->JobList, pJob);
    DeleteElementSkiplist(&GlobalJobList, pJob);

    // Try to delete the corresponding .SHD file.
    pwszSHDFile = DllAllocSplMem(GetJobFilePath(L"SHD", 0, NULL));
    if (pwszSHDFile && GetJobFilePath(L"SHD", pJob->dwJobID, pwszSHDFile))
        DeleteFileW(pwszSHDFile);

    // Free memory for the mandatory fields.
    DllFreeSplMem(pJob->pDevMode);
    DllFreeSplStr(pJob->pwszDatatype);
    DllFreeSplStr(pJob->pwszDocumentName);
    DllFreeSplStr(pJob->pwszMachineName);
    DllFreeSplStr(pJob->pwszNotifyName);
    DllFreeSplStr(pJob->pwszUserName);

    // Free memory for the optional fields if they are present.
    if (pJob->pwszOutputFile)
        DllFreeSplStr(pJob->pwszOutputFile);
    
    if (pJob->pwszPrintProcessorParameters)
        DllFreeSplStr(pJob->pwszPrintProcessorParameters);

    if (pJob->pwszStatus)
        DllFreeSplStr(pJob->pwszStatus);

    // Finally free the job structure itself.
    DllFreeSplMem(pJob);
}
