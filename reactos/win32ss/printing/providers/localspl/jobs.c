/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

LIST_ENTRY LocalJobQueue;

void
InitializeJobQueue()
{
    const WCHAR wszPath[] = L"\\PRINTERS\\?????.SHD";
    const DWORD cchPath = _countof(wszPath) - 1;
    const DWORD cchFolders = sizeof("\\PRINTERS\\") - 1;
    const DWORD cchPattern = sizeof("?????") - 1;

    DWORD dwJobID;
    HANDLE hFind;
    PLOCAL_JOB pJob;
    PWSTR p;
    WCHAR wszFullPath[MAX_PATH];
    WIN32_FIND_DATAW FindData;

    // Construct the full path search pattern.
    CopyMemory(wszFullPath, wszSpoolDirectory, cchSpoolDirectory * sizeof(WCHAR));
    CopyMemory(&wszFullPath[cchSpoolDirectory], wszPath, (cchPath + 1) * sizeof(WCHAR));

    // Use the search pattern to look for unfinished jobs serialized in shadow files (.SHD)
    hFind = FindFirstFileW(wszFullPath, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // No unfinished jobs found.
        return;
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

        // Add it to the job queue of the respective printer.
        InsertTailList(&pJob->Printer->JobQueue, &pJob->Entry);
    }
    while (FindNextFileW(hFind, &FindData));

    FindClose(hFind);
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
    PSHD_HEADER pShadowFile = NULL;
    PWSTR pwszPrinterName;

    // Try to open the file.
    hFile = CreateFileW(pwszFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Get its file size (small enough for a single DWORD) and allocate memory for all of it.
    cbFileSize = GetFileSize(hFile, NULL);
    pShadowFile = DllAllocSplMem(cbFileSize);
    if (!pShadowFile)
    {
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Read the entire file.
    if (!ReadFile(hFile, pShadowFile, cbFileSize, &cbRead, NULL))
    {
        ERR("ReadFile failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Check signature and header size.
    if (pShadowFile->dwSignature != SHD_WIN2003_SIGNATURE || pShadowFile->cbHeader != sizeof(SHD_HEADER))
    {
        ERR("Signature or Header Size mismatch!\n");
        goto Cleanup;
    }

    // Retrieve the associated printer from the table.
    pwszPrinterName = (PWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offPrinterName);
    pPrinter = RtlLookupElementGenericTable(&PrinterTable, pwszPrinterName);
    if (!pPrinter)
    {
        ERR("This shadow file references a non-existing printer!\n");
        goto Cleanup;
    }

    // Create a new job structure and copy over the relevant fields.
    pJob = DllAllocSplMem(sizeof(LOCAL_JOB));
    if (!pJob)
    {
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    pJob->dwJobID = pShadowFile->dwJobID;
    pJob->Printer = pPrinter;
    pJob->pwszDatatype = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offDatatype));
    pJob->pwszDocumentName = AllocSplStr((PCWSTR)((ULONG_PTR)pShadowFile + pShadowFile->offDocumentName));
    pJob->pwszOutputFile = NULL;
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
WriteJobShadowFile(PCWSTR pwszFilePath, const PLOCAL_JOB pJob)
{
    BOOL bReturnValue = FALSE;
    DWORD cbDatatype;
    DWORD cbDocumentName;
    DWORD cbFileSize;
    DWORD cbPrinterName;
    DWORD cbWritten;
    DWORD dwCurrentOffset;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PSHD_HEADER pShadowFile = NULL;

    // Try to open the file.
    hFile = CreateFileW(pwszFilePath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Compute the total size of the shadow file.
    cbDatatype = (wcslen(pJob->pwszDatatype) + 1) * sizeof(WCHAR);
    cbDocumentName = (wcslen(pJob->pwszDocumentName) + 1) * sizeof(WCHAR);
    cbPrinterName = (wcslen(pJob->Printer->pwszPrinterName) + 1) * sizeof(WCHAR);
    cbFileSize = sizeof(SHD_HEADER) + cbDatatype + cbDocumentName + cbPrinterName;

    // Allocate memory for it.
    pShadowFile = DllAllocSplMem(cbFileSize);
    if (!pShadowFile)
    {
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Fill out the shadow file header information.
    pShadowFile->dwSignature = SHD_WIN2003_SIGNATURE;
    pShadowFile->cbHeader = sizeof(SHD_HEADER);
    pShadowFile->dwJobID = pJob->dwJobID;

    // Add the extra values that are stored as offsets in the shadow file.
    // The first value begins right after the shadow file header.
    dwCurrentOffset = sizeof(SHD_HEADER);

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszDatatype, cbDatatype);
    pShadowFile->offDatatype = dwCurrentOffset;
    dwCurrentOffset += cbDatatype;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->pwszDocumentName, cbDocumentName);
    pShadowFile->offDocumentName = dwCurrentOffset;
    dwCurrentOffset += cbDocumentName;

    CopyMemory((PBYTE)pShadowFile + dwCurrentOffset, pJob->Printer->pwszPrinterName, cbPrinterName);
    pShadowFile->offPrinterName = dwCurrentOffset;
    dwCurrentOffset += cbPrinterName;

    // Write the file.
    if (!WriteFile(hFile, pShadowFile, cbFileSize, &cbWritten, NULL))
    {
        ERR("WriteFile failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    bReturnValue = TRUE;

Cleanup:
    if (pShadowFile)
        DllFreeSplMem(pShadowFile);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

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
