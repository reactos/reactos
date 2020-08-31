/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/printers.h>
#include <marshalling/printerdrivers.h>
#include <strsafe.h>

// Local Constants

/** And the award for the most confusingly named setting goes to "Device", for storing the default printer of the current user.
    Ok, I admit that this has historical reasons. It's still not straightforward in any way though! */
static const WCHAR wszWindowsKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
static const WCHAR wszDeviceValue[] = L"Device";

static DWORD
_StartDocPrinterSpooled(PSPOOLER_HANDLE pHandle, PDOC_INFO_1W pDocInfo1, PADDJOB_INFO_1W pAddJobInfo1)
{
    DWORD cbNeeded;
    DWORD dwErrorCode;
    PJOB_INFO_1W pJobInfo1 = NULL;

    // Create the spool file.
    pHandle->hSPLFile = CreateFileW(pAddJobInfo1->Path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    if (pHandle->hSPLFile == INVALID_HANDLE_VALUE)
    {
        dwErrorCode = GetLastError();
        ERR("CreateFileW failed for \"%S\" with error %lu!\n", pAddJobInfo1->Path, dwErrorCode);
        goto Cleanup;
    }

    // Get the size of the job information.
    GetJobW((HANDLE)pHandle, pAddJobInfo1->JobId, 1, NULL, 0, &cbNeeded);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        dwErrorCode = GetLastError();
        ERR("GetJobW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Allocate enough memory for the returned job information.
    pJobInfo1 = HeapAlloc(hProcessHeap, 0, cbNeeded);
    if (!pJobInfo1)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("HeapAlloc failed!\n");
        goto Cleanup;
    }

    // Get the job information.
    if (!GetJobW((HANDLE)pHandle, pAddJobInfo1->JobId, 1, (PBYTE)pJobInfo1, cbNeeded, &cbNeeded))
    {
        dwErrorCode = GetLastError();
        ERR("GetJobW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Add our document information.
    if (pDocInfo1->pDatatype)
        pJobInfo1->pDatatype = pDocInfo1->pDatatype;

    pJobInfo1->pDocument = pDocInfo1->pDocName;

    // Set the new job information.
    if (!SetJobW((HANDLE)pHandle, pAddJobInfo1->JobId, 1, (PBYTE)pJobInfo1, 0))
    {
        dwErrorCode = GetLastError();
        ERR("SetJobW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We were successful!
    pHandle->dwJobID = pAddJobInfo1->JobId;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pJobInfo1)
        HeapFree(hProcessHeap, 0, pJobInfo1);

    return dwErrorCode;
}

static DWORD
_StartDocPrinterWithRPC(PSPOOLER_HANDLE pHandle, PDOC_INFO_1W pDocInfo1)
{
    DWORD dwErrorCode;
    WINSPOOL_DOC_INFO_CONTAINER DocInfoContainer;

    DocInfoContainer.Level = 1;
    DocInfoContainer.DocInfo.pDocInfo1 = (WINSPOOL_DOC_INFO_1*)pDocInfo1;

    RpcTryExcept
    {
        dwErrorCode = _RpcStartDocPrinter(pHandle->hPrinter, &DocInfoContainer, &pHandle->dwJobID);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcStartDocPrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    return dwErrorCode;
}

BOOL WINAPI
AbortPrinter(HANDLE hPrinter)
{
    TRACE("AbortPrinter(%p)\n", hPrinter);
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE WINAPI
AddPrinterA(PSTR pName, DWORD Level, PBYTE pPrinter)
{
    TRACE("AddPrinterA(%s, %lu, %p)\n", pName, Level, pPrinter);
    UNIMPLEMENTED;
    return NULL;
}

HANDLE WINAPI
AddPrinterW(PWSTR pName, DWORD Level, PBYTE pPrinter)
{
    TRACE("AddPrinterW(%S, %lu, %p)\n", pName, Level, pPrinter);
    UNIMPLEMENTED;
    return NULL;
}

BOOL WINAPI
ClosePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("ClosePrinter(%p)\n", hPrinter);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call.
    RpcTryExcept
    {
        dwErrorCode = _RpcClosePrinter(&pHandle->hPrinter);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcClosePrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    // Close any open file handle.
    if (pHandle->hSPLFile != INVALID_HANDLE_VALUE)
        CloseHandle(pHandle->hSPLFile);

    // Free the memory for the handle.
    HeapFree(hProcessHeap, 0, pHandle);

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DeletePrinter(HANDLE hPrinter)
{
    TRACE("DeletePrinter(%p)\n", hPrinter);
    UNIMPLEMENTED;
    return FALSE;
}

DWORD WINAPI
DeviceCapabilitiesA(LPCSTR pDevice, LPCSTR pPort, WORD fwCapability, LPSTR pOutput, const DEVMODEA* pDevMode)
{
    TRACE("DeviceCapabilitiesA(%s, %s, %hu, %p, %p)\n", pDevice, pPort, fwCapability, pOutput, pDevMode);
    UNIMPLEMENTED;
    return 0;
}

DWORD WINAPI
DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort, WORD fwCapability, LPWSTR pOutput, const DEVMODEW* pDevMode)
{
    TRACE("DeviceCapabilitiesW(%S, %S, %hu, %p, %p)\n", pDevice, pPort, fwCapability, pOutput, pDevMode);
    UNIMPLEMENTED;
    return 0;
}

INT WINAPI
DocumentEvent( HANDLE hPrinter, HDC hdc, int iEsc, ULONG cbIn, PVOID pvIn, ULONG cbOut, PVOID pvOut)
{
    TRACE("DocumentEvent(%p, %p, %lu, %lu, %p, %lu, %p)\n", hPrinter, hdc, iEsc, cbIn, pvIn, cbOut, pvOut);
    UNIMPLEMENTED;
    return DOCUMENTEVENT_UNSUPPORTED;
}

LONG WINAPI
DocumentPropertiesA(HWND hWnd, HANDLE hPrinter, LPSTR pDeviceName, PDEVMODEA pDevModeOutput, PDEVMODEA pDevModeInput, DWORD fMode)
{
    PWSTR pwszDeviceName = NULL;
    PDEVMODEW pdmwInput = NULL;
    PDEVMODEW pdmwOutput = NULL;
    LONG lReturnValue = -1;
    DWORD cch;

    TRACE("DocumentPropertiesA(%p, %p, %s, %p, %p, %lu)\n", hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode);

    if (pDeviceName)
    {
        // Convert pName to a Unicode string pwszDeviceName.
        cch = strlen(pDeviceName);

        pwszDeviceName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszDeviceName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pDeviceName, -1, pwszDeviceName, cch + 1);
    }

    if (pDevModeInput)
    {
        // Create working buffer for input to DocumentPropertiesW.
        pdmwInput = HeapAlloc(hProcessHeap, 0, sizeof(DEVMODEW));
        if (!pdmwInput)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }
        RosConvertAnsiDevModeToUnicodeDevmode(pDevModeInput, pdmwInput);
    }

    if (pDevModeOutput)
    {
        // Create working buffer for output from DocumentPropertiesW.
        pdmwOutput = HeapAlloc(hProcessHeap, 0, sizeof(DEVMODEW));
        if (!pdmwOutput)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }
    }

    lReturnValue = DocumentPropertiesW(hWnd, hPrinter, pwszDeviceName, pdmwOutput, pdmwInput, fMode);
    TRACE("lReturnValue from DocumentPropertiesW is '%ld'.\n", lReturnValue);

    if (lReturnValue < 0)
    {
        TRACE("DocumentPropertiesW failed!\n");
        goto Cleanup;
    }

    if (pdmwOutput)
    {
        RosConvertUnicodeDevModeToAnsiDevmode(pdmwOutput, pDevModeOutput);
    }

Cleanup:
    if(pwszDeviceName)
        HeapFree(hProcessHeap, 0, pwszDeviceName);

    if (pdmwInput)
        HeapFree(hProcessHeap, 0, pdmwInput);

    if (pdmwOutput)
        HeapFree(hProcessHeap, 0, pdmwOutput);

    return lReturnValue;
}

static PRINTER_INFO_9W * get_devmodeW(HANDLE hprn)
{
    PRINTER_INFO_9W *pi9 = NULL;
    DWORD needed = 0;
    BOOL res;

    res = GetPrinterW(hprn, 9, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        pi9 = HeapAlloc(hProcessHeap, 0, needed);
        res = GetPrinterW(hprn, 9, (LPBYTE)pi9, needed, &needed);
    }

    if (res)
        return pi9;

    ERR("GetPrinterW failed with %u\n", GetLastError());
    HeapFree(hProcessHeap, 0, pi9);
    return NULL;
}

LONG WINAPI
DocumentPropertiesW(HWND hWnd, HANDLE hPrinter, LPWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput, DWORD fMode)
{
    HANDLE hUseHandle = NULL;
    PRINTER_INFO_9W *pi9 = NULL;
    LONG Result = -1, Length;

    TRACE("DocumentPropertiesW(%p, %p, %S, %p, %p, %lu)\n", hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode);
    if (hPrinter)
    {
        hUseHandle = hPrinter;
    }
    else if (!OpenPrinterW(pDeviceName, &hUseHandle, NULL))
    {
        ERR("No handle, and no usable printer name passed in\n");
        return -1;
    }

    pi9 = get_devmodeW(hUseHandle);

    if (pi9)
    {
        Length = pi9->pDevMode->dmSize + pi9->pDevMode->dmDriverExtra;
        // See wineps.drv PSDRV_ExtDeviceMode
        if (fMode)
        {
            Result = 1; /* IDOK */

            if (fMode & DM_IN_BUFFER)
            {
                FIXME("Merge pDevModeInput with pi9, write back to driver!\n");
                // See wineps.drv PSDRV_MergeDevmodes
            }

            if (fMode & DM_IN_PROMPT)
            {
                FIXME("Show property sheet!\n");
                Result = 2; /* IDCANCEL */
            }

            if (fMode & (DM_OUT_BUFFER | DM_OUT_DEFAULT))
            {
                if (pDevModeOutput)
                {
                    memcpy(pDevModeOutput, pi9->pDevMode, pi9->pDevMode->dmSize + pi9->pDevMode->dmDriverExtra);
                }
                else
                {
                    ERR("No pDevModeOutput\n");
                    Result = -1;
                }
            }
        }
        else
        {
            Result = Length;
        }

        HeapFree(hProcessHeap, 0, pi9);
    }

    if (hUseHandle && !hPrinter)
        ClosePrinter(hUseHandle);
    return Result;
}

BOOL WINAPI
EndDocPrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("EndDocPrinter(%p)\n", hPrinter);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    if (pHandle->hSPLFile != INVALID_HANDLE_VALUE)
    {
        // For spooled jobs, the document is finished by calling _RpcScheduleJob.
        RpcTryExcept
        {
            dwErrorCode = _RpcScheduleJob(pHandle->hPrinter, pHandle->dwJobID);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErrorCode = RpcExceptionCode();
            ERR("_RpcScheduleJob failed with exception code %lu!\n", dwErrorCode);
        }
        RpcEndExcept;

        // Close the spool file handle.
        CloseHandle(pHandle->hSPLFile);
    }
    else
    {
        // In all other cases, just call _RpcEndDocPrinter.
        RpcTryExcept
        {
            dwErrorCode = _RpcEndDocPrinter(pHandle->hPrinter);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErrorCode = RpcExceptionCode();
            ERR("_RpcEndDocPrinter failed with exception code %lu!\n", dwErrorCode);
        }
        RpcEndExcept;
    }

    // A new document can now be started again.
    pHandle->bStartedDoc = FALSE;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EndPagePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("EndPagePrinter(%p)\n", hPrinter);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    if (pHandle->hSPLFile != INVALID_HANDLE_VALUE)
    {
        // For spooled jobs, we don't need to do anything.
        dwErrorCode = ERROR_SUCCESS;
    }
    else
    {
        // In all other cases, just call _RpcEndPagePrinter.
        RpcTryExcept
        {
            dwErrorCode = _RpcEndPagePrinter(pHandle->hPrinter);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErrorCode = RpcExceptionCode();
            ERR("_RpcEndPagePrinter failed with exception code %lu!\n", dwErrorCode);
        }
        RpcEndExcept;
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumPrintersA(DWORD Flags, PSTR Name, DWORD Level, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL bReturnValue = FALSE;
    DWORD cch;
    PWSTR pwszName = NULL;
    PSTR pszPrinterName = NULL;
    PSTR pszServerName = NULL;
    PSTR pszDescription = NULL;
    PSTR pszName = NULL;
    PSTR pszComment = NULL;
    PSTR pszShareName = NULL;
    PSTR pszPortName = NULL;
    PSTR pszDriverName = NULL;
    PSTR pszLocation = NULL;
    PSTR pszSepFile = NULL;
    PSTR pszPrintProcessor = NULL;
    PSTR pszDatatype = NULL;
    PSTR pszParameters = NULL;
    DWORD i;
    PPRINTER_INFO_1W ppi1w = NULL;
    PPRINTER_INFO_1A ppi1a = NULL;
    PPRINTER_INFO_2W ppi2w = NULL;
    PPRINTER_INFO_2A ppi2a = NULL;
    PPRINTER_INFO_4W ppi4w = NULL;
    PPRINTER_INFO_4A ppi4a = NULL;
    PPRINTER_INFO_5W ppi5w = NULL;
    PPRINTER_INFO_5A ppi5a = NULL;

    TRACE("EnumPrintersA(%lu, %s, %lu, %p, %lu, %p, %p)\n", Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);

    // Check for invalid levels here for early error return. MSDN says that only 1, 2, 4, and 5 are allowable.
    if (Level !=  1 && Level != 2 && Level != 4 && Level != 5)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        ERR("Invalid Level!\n");
        goto Cleanup;
    }

    if (Name)
    {
        // Convert pName to a Unicode string pwszName.
        cch = strlen(Name);

        pwszName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, Name, -1, pwszName, cch + 1);
    }
 
    /* Ref: https://stackoverflow.com/questions/41147180/why-enumprintersa-and-enumprintersw-request-the-same-amount-of-memory */
    bReturnValue = EnumPrintersW(Flags, pwszName, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
    HeapFree(hProcessHeap, 0, pwszName);

    TRACE("*pcReturned is '%d' and bReturnValue is '%d' and GetLastError is '%ld'.\n", *pcReturned, bReturnValue, GetLastError());

    /* We are mapping multiple different pointers to the same pPrinterEnum pointer here so that */
    /* we can do in-place conversion. We read the Unicode response from the EnumPrintersW and */
    /* then we write back the ANSI conversion into the same buffer for our EnumPrintersA output */

    /* mapping to pPrinterEnum for Unicode (w) characters for Levels 1, 2, 4, and 5 */
    ppi1w = (PPRINTER_INFO_1W)pPrinterEnum;
    ppi2w = (PPRINTER_INFO_2W)pPrinterEnum;
    ppi4w = (PPRINTER_INFO_4W)pPrinterEnum;
    ppi5w = (PPRINTER_INFO_5W)pPrinterEnum;
    /* mapping to pPrinterEnum for ANSI (a) characters for Levels 1, 2, 4, and 5 */
    ppi1a = (PPRINTER_INFO_1A)pPrinterEnum;
    ppi2a = (PPRINTER_INFO_2A)pPrinterEnum;
    ppi4a = (PPRINTER_INFO_4A)pPrinterEnum;
    ppi5a = (PPRINTER_INFO_5A)pPrinterEnum;

    for (i = 0; i < *pcReturned; i++)
    {
        switch (Level)
        {
            case 1:
            {
                if (ppi1w[i].pDescription)
                {
                    // Convert Unicode pDescription to a ANSI string pszDescription.
                    cch = wcslen(ppi1w[i].pDescription);

                    pszDescription = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszDescription)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi1w[i].pDescription, -1, pszDescription, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi1a[i].pDescription, cch + 1, pszDescription);

                    HeapFree(hProcessHeap, 0, pszDescription);
                }

                if (ppi1w[i].pName)
                {
                    // Convert Unicode pName to a ANSI string pszName.
                    cch = wcslen(ppi1w[i].pName);

                    pszName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi1w[i].pName, -1, pszName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi1a[i].pName, cch + 1, pszName);

                    HeapFree(hProcessHeap, 0, pszName);
                }

                if (ppi1w[i].pComment)
                {
                    // Convert Unicode pComment to a ANSI string pszComment.
                    cch = wcslen(ppi1w[i].pComment);

                    pszComment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszComment)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi1w[i].pComment, -1, pszComment, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi1a[i].pComment, cch + 1, pszComment);

                    HeapFree(hProcessHeap, 0, pszComment);
                }
                break;
            }


            case 2:
            {
                if (ppi2w[i].pServerName)
                {
                    // Convert Unicode pServerName to a ANSI string pszServerName.
                    cch = wcslen(ppi2w[i].pServerName);

                    pszServerName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszServerName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pServerName, -1, pszServerName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pServerName, cch + 1, pszServerName);

                    HeapFree(hProcessHeap, 0, pszServerName);
                }

                if (ppi2w[i].pPrinterName)
                {
                    // Convert Unicode pPrinterName to a ANSI string pszPrinterName.
                    cch = wcslen(ppi2w[i].pPrinterName);

                    pszPrinterName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszPrinterName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pPrinterName, -1, pszPrinterName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pPrinterName, cch + 1, pszPrinterName);

                    HeapFree(hProcessHeap, 0, pszPrinterName);
                }

                if (ppi2w[i].pShareName)
                {
                    // Convert Unicode pShareName to a ANSI string pszShareName.
                    cch = wcslen(ppi2w[i].pShareName);

                    pszShareName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszShareName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pShareName, -1, pszShareName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pShareName, cch + 1, pszShareName);

                    HeapFree(hProcessHeap, 0, pszShareName);
                }

                if (ppi2w[i].pPortName)
                {
                    // Convert Unicode pPortName to a ANSI string pszPortName.
                    cch = wcslen(ppi2w[i].pPortName);

                    pszPortName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszPortName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pPortName, -1, pszPortName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pPortName, cch + 1, pszPortName);

                    HeapFree(hProcessHeap, 0, pszPortName);
                }

                if (ppi2w[i].pDriverName)
                {
                    // Convert Unicode pDriverName to a ANSI string pszDriverName.
                    cch = wcslen(ppi2w[i].pDriverName);

                    pszDriverName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszDriverName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pDriverName, -1, pszDriverName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pDriverName, cch + 1, pszDriverName);

                    HeapFree(hProcessHeap, 0, pszDriverName);
                }

                if (ppi2w[i].pComment)
                {
                    // Convert Unicode pComment to a ANSI string pszComment.
                    cch = wcslen(ppi2w[i].pComment);

                    pszComment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszComment)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pComment, -1, pszComment, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pComment, cch + 1, pszComment);

                    HeapFree(hProcessHeap, 0, pszComment);
                }

                if (ppi2w[i].pLocation)
                {
                    // Convert Unicode pLocation to a ANSI string pszLocation.
                    cch = wcslen(ppi2w[i].pLocation);

                    pszLocation = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszLocation)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pLocation, -1, pszLocation, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pLocation, cch + 1, pszLocation);

                    HeapFree(hProcessHeap, 0, pszLocation);
                }


                if (ppi2w[i].pSepFile)
                {
                    // Convert Unicode pSepFile to a ANSI string pszSepFile.
                    cch = wcslen(ppi2w[i].pSepFile);

                    pszSepFile = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszSepFile)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pSepFile, -1, pszSepFile, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pSepFile, cch + 1, pszSepFile);

                    HeapFree(hProcessHeap, 0, pszSepFile);
                }

                if (ppi2w[i].pPrintProcessor)
                {
                    // Convert Unicode pPrintProcessor to a ANSI string pszPrintProcessor.
                    cch = wcslen(ppi2w[i].pPrintProcessor);

                    pszPrintProcessor = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszPrintProcessor)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pPrintProcessor, -1, pszPrintProcessor, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pPrintProcessor, cch + 1, pszPrintProcessor);

                    HeapFree(hProcessHeap, 0, pszPrintProcessor);
                }


                if (ppi2w[i].pDatatype)
                {
                    // Convert Unicode pDatatype to a ANSI string pszDatatype.
                    cch = wcslen(ppi2w[i].pDatatype);

                    pszDatatype = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszDatatype)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pDatatype, -1, pszDatatype, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pDatatype, cch + 1, pszDatatype);

                    HeapFree(hProcessHeap, 0, pszDatatype);
                }

                if (ppi2w[i].pParameters)
                {
                    // Convert Unicode pParameters to a ANSI string pszParameters.
                    cch = wcslen(ppi2w[i].pParameters);

                    pszParameters = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszParameters)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pParameters, -1, pszParameters, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pParameters, cch + 1, pszParameters);

                    HeapFree(hProcessHeap, 0, pszParameters);
                }
                break;

            }

            case 4:
            {
                if (ppi4w[i].pPrinterName)
                {
                    // Convert Unicode pPrinterName to a ANSI string pszPrinterName.
                    cch = wcslen(ppi4w[i].pPrinterName);

                    pszPrinterName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszPrinterName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi4w[i].pPrinterName, -1, pszPrinterName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi4a[i].pPrinterName, cch + 1, pszPrinterName);

                    HeapFree(hProcessHeap, 0, pszPrinterName);
                }

                if (ppi4w[i].pServerName)
                {
                    // Convert Unicode pServerName to a ANSI string pszServerName.
                    cch = wcslen(ppi4w[i].pServerName);

                    pszServerName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszServerName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi4w[i].pServerName, -1, pszServerName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi4a[i].pServerName, cch + 1, pszServerName);

                    HeapFree(hProcessHeap, 0, pszServerName);
                }
                break;
            }

            case 5:
            {
                if (ppi5w[i].pPrinterName)
                {
                    // Convert Unicode pPrinterName to a ANSI string pszPrinterName.
                    cch = wcslen(ppi5w[i].pPrinterName);

                    pszPrinterName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszPrinterName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi5w[i].pPrinterName, -1, pszPrinterName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi5a[i].pPrinterName, cch + 1, pszPrinterName);

                    HeapFree(hProcessHeap, 0, pszPrinterName);
                }

                if (ppi5w[i].pPortName)
                {
                    // Convert Unicode pPortName to a ANSI string pszPortName.
                    cch = wcslen(ppi5w[i].pPortName);

                    pszPortName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                    if (!pszPortName)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi5w[i].pPortName, -1, pszPortName, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi5a[i].pPortName, cch + 1, pszPortName);

                    HeapFree(hProcessHeap, 0, pszPortName);
                }
                break;
            }

        }   // switch
    }       // for

Cleanup:

    return bReturnValue;
}

BOOL WINAPI
EnumPrintersW(DWORD Flags, PWSTR Name, DWORD Level, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

    TRACE("EnumPrintersW(%lu, %S, %lu, %p, %lu, %p, %p)\n", Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);

    // Dismiss invalid levels already at this point.
    if (Level == 3 || Level > 5)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (cbBuf && pPrinterEnum)
        ZeroMemory(pPrinterEnum, cbBuf);

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrinters(Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPrinters failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 9);
        MarshallUpStructuresArray(cbBuf, pPrinterEnum, *pcReturned, pPrinterInfoMarshalling[Level]->pInfo, pPrinterInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
FlushPrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pcWritten, DWORD cSleep)
{
    TRACE("FlushPrinter(%p, %p, %lu, %p, %lu)\n", hPrinter, pBuf, cbBuf, pcWritten, cSleep);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetDefaultPrinterA(LPSTR pszBuffer, LPDWORD pcchBuffer)
{
    DWORD dwErrorCode;
    PWSTR pwszBuffer = NULL;

    TRACE("GetDefaultPrinterA(%p, %p)\n", pszBuffer, pcchBuffer);

    // Sanity check.
    if (!pcchBuffer)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Check if an ANSI buffer was given and if so, allocate a Unicode buffer of the same size.
    if (pszBuffer && *pcchBuffer)
    {
        pwszBuffer = HeapAlloc(hProcessHeap, 0, *pcchBuffer * sizeof(WCHAR));
        if (!pwszBuffer)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }
    }

    if (!GetDefaultPrinterW(pwszBuffer, pcchBuffer))
    {
        dwErrorCode = GetLastError();
        goto Cleanup;
    }

    // We successfully got a string in pwszBuffer, so convert the Unicode string to ANSI.
    WideCharToMultiByte(CP_ACP, 0, pwszBuffer, -1, pszBuffer, *pcchBuffer, NULL, NULL);

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pwszBuffer)
        HeapFree(hProcessHeap, 0, pwszBuffer);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetDefaultPrinterW(LPWSTR pszBuffer, LPDWORD pcchBuffer)
{
    DWORD cbNeeded;
    DWORD cchInputBuffer;
    DWORD dwErrorCode;
    HKEY hWindowsKey = NULL;
    PWSTR pwszDevice = NULL;
    PWSTR pwszComma;

    TRACE("GetDefaultPrinterW(%p, %p)\n", pszBuffer, pcchBuffer);

    // Sanity check.
    if (!pcchBuffer)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    cchInputBuffer = *pcchBuffer;

    // Open the registry key where the default printer for the current user is stored.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_CURRENT_USER, wszWindowsKey, 0, KEY_READ, &hWindowsKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Determine the size of the required buffer.
    dwErrorCode = (DWORD)RegQueryValueExW(hWindowsKey, wszDeviceValue, NULL, NULL, NULL, &cbNeeded);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Allocate it.
    pwszDevice = HeapAlloc(hProcessHeap, 0, cbNeeded);
    if (!pwszDevice)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("HeapAlloc failed!\n");
        goto Cleanup;
    }

    // Now get the actual value.
    dwErrorCode = RegQueryValueExW(hWindowsKey, wszDeviceValue, NULL, NULL, (PBYTE)pwszDevice, &cbNeeded);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We get a string "<Printer Name>,winspool,<Port>:".
    // Extract the printer name from it.
    pwszComma = wcschr(pwszDevice, L',');
    if (!pwszComma)
    {
        ERR("Found no or invalid default printer: %S!\n", pwszDevice);
        dwErrorCode = ERROR_INVALID_NAME;
        goto Cleanup;
    }

    // Store the length of the Printer Name (including the terminating NUL character!) in *pcchBuffer.
    *pcchBuffer = pwszComma - pwszDevice + 1;

    // Check if the supplied buffer is large enough.
    if (cchInputBuffer < *pcchBuffer)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy the default printer.
    *pwszComma = 0;
    CopyMemory(pszBuffer, pwszDevice, *pcchBuffer * sizeof(WCHAR));

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (hWindowsKey)
        RegCloseKey(hWindowsKey);

    if (pwszDevice)
        HeapFree(hProcessHeap, 0, pwszDevice);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded)
{
    PPRINTER_INFO_1A ppi1a = (PPRINTER_INFO_1A)pPrinter;
    PPRINTER_INFO_1W ppi1w = (PPRINTER_INFO_1W)pPrinter;
    PPRINTER_INFO_2A ppi2a = (PPRINTER_INFO_2A)pPrinter;
    PPRINTER_INFO_2W ppi2w = (PPRINTER_INFO_2W)pPrinter;
    PPRINTER_INFO_4A ppi4a = (PPRINTER_INFO_4A)pPrinter;
    PPRINTER_INFO_4W ppi4w = (PPRINTER_INFO_4W)pPrinter;
    PPRINTER_INFO_5A ppi5a = (PPRINTER_INFO_5A)pPrinter;
    PPRINTER_INFO_5W ppi5w = (PPRINTER_INFO_5W)pPrinter;
    PPRINTER_INFO_7A ppi7a = (PPRINTER_INFO_7A)pPrinter;
    PPRINTER_INFO_7W ppi7w = (PPRINTER_INFO_7W)pPrinter;
    DWORD cch;
    BOOL bReturnValue = FALSE;

    TRACE("GetPrinterA(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pPrinter, cbBuf, pcbNeeded);

    // Check for invalid levels here for early error return. Should be 1-9.
    if (Level <  1 || Level > 9)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        ERR("Invalid Level!\n");
        goto Cleanup;
    }

    bReturnValue = GetPrinterW(hPrinter, Level, pPrinter, cbBuf, pcbNeeded);

    if (!bReturnValue)
    {
        TRACE("GetPrinterW failed!\n");
        goto Cleanup;
    }

    switch (Level)
    {
        case 1:
        {
            if (ppi1w->pDescription)
            {
                PSTR pszDescription;

                // Convert Unicode pDescription to a ANSI string pszDescription.
                cch = wcslen(ppi1w->pDescription);

                pszDescription = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszDescription)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi1w->pDescription, -1, pszDescription, cch + 1, NULL, NULL);
                StringCchCopyA(ppi1a->pDescription, cch + 1, pszDescription);

                HeapFree(hProcessHeap, 0, pszDescription);
            }

            if (ppi1w->pName)
            {
                PSTR pszName;

                // Convert Unicode pName to a ANSI string pszName.
                cch = wcslen(ppi1w->pName);

                pszName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi1w->pName, -1, pszName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi1a->pName, cch + 1, pszName);

                HeapFree(hProcessHeap, 0, pszName);
            }

            if (ppi1w->pComment)
            {
                PSTR pszComment;

                // Convert Unicode pComment to a ANSI string pszComment.
                cch = wcslen(ppi1w->pComment);

                pszComment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszComment)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi1w->pComment, -1, pszComment, cch + 1, NULL, NULL);
                StringCchCopyA(ppi1a->pComment, cch + 1, pszComment);

                HeapFree(hProcessHeap, 0, pszComment);
            }
            break;
        }

        case 2:
        {
            if (ppi2w->pServerName)
            {
                PSTR pszServerName;

                // Convert Unicode pServerName to a ANSI string pszServerName.
                cch = wcslen(ppi2w->pServerName);

                pszServerName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszServerName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pServerName, -1, pszServerName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pServerName, cch + 1, pszServerName);

                HeapFree(hProcessHeap, 0, pszServerName);
            }

            if (ppi2w->pPrinterName)
            {
                PSTR pszPrinterName;

                // Convert Unicode pPrinterName to a ANSI string pszPrinterName.
                cch = wcslen(ppi2w->pPrinterName);

                pszPrinterName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszPrinterName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pPrinterName, -1, pszPrinterName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pPrinterName, cch + 1, pszPrinterName);

                HeapFree(hProcessHeap, 0, pszPrinterName);
            }

            if (ppi2w->pShareName)
            {
                PSTR pszShareName;

                // Convert Unicode pShareName to a ANSI string pszShareName.
                cch = wcslen(ppi2w->pShareName);

                pszShareName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszShareName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pShareName, -1, pszShareName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pShareName, cch + 1, pszShareName);

                HeapFree(hProcessHeap, 0, pszShareName);
            }

            if (ppi2w->pPortName)
            {
                PSTR pszPortName;

                // Convert Unicode pPortName to a ANSI string pszPortName.
                cch = wcslen(ppi2w->pPortName);

                pszPortName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszPortName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pPortName, -1, pszPortName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pPortName, cch + 1, pszPortName);

                HeapFree(hProcessHeap, 0, pszPortName);
            }

            if (ppi2w->pDriverName)
            {
                PSTR pszDriverName;

                // Convert Unicode pDriverName to a ANSI string pszDriverName.
                cch = wcslen(ppi2w->pDriverName);

                pszDriverName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszDriverName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pDriverName, -1, pszDriverName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pDriverName, cch + 1, pszDriverName);

                HeapFree(hProcessHeap, 0, pszDriverName);
            }

            if (ppi2w->pComment)
            {
                PSTR pszComment;

                // Convert Unicode pComment to a ANSI string pszComment.
                cch = wcslen(ppi2w->pComment);

                pszComment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszComment)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pComment, -1, pszComment, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pComment, cch + 1, pszComment);

                HeapFree(hProcessHeap, 0, pszComment);
            }

            if (ppi2w->pLocation)
            {
                PSTR pszLocation;

                // Convert Unicode pLocation to a ANSI string pszLocation.
                cch = wcslen(ppi2w->pLocation);

                pszLocation = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszLocation)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pLocation, -1, pszLocation, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pLocation, cch + 1, pszLocation);

                HeapFree(hProcessHeap, 0, pszLocation);
            }

            if (ppi2w->pSepFile)
            {
                PSTR pszSepFile;

                // Convert Unicode pSepFile to a ANSI string pszSepFile.
                cch = wcslen(ppi2w->pSepFile);

                pszSepFile = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszSepFile)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pSepFile, -1, pszSepFile, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pSepFile, cch + 1, pszSepFile);

                HeapFree(hProcessHeap, 0, pszSepFile);
            }

            if (ppi2w->pPrintProcessor)
            {
                PSTR pszPrintProcessor;

                // Convert Unicode pPrintProcessor to a ANSI string pszPrintProcessor.
                cch = wcslen(ppi2w->pPrintProcessor);

                pszPrintProcessor = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszPrintProcessor)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pPrintProcessor, -1, pszPrintProcessor, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pPrintProcessor, cch + 1, pszPrintProcessor);

                HeapFree(hProcessHeap, 0, pszPrintProcessor);
            }

            if (ppi2w->pDatatype)
            {
                PSTR pszDatatype;

                // Convert Unicode pDatatype to a ANSI string pszDatatype.
                cch = wcslen(ppi2w->pDatatype);

                pszDatatype = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszDatatype)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pDatatype, -1, pszDatatype, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pDatatype, cch + 1, pszDatatype);

                HeapFree(hProcessHeap, 0, pszDatatype);
            }

            if (ppi2w->pParameters)
            {
                PSTR pszParameters;

                // Convert Unicode pParameters to a ANSI string pszParameters.
                cch = wcslen(ppi2w->pParameters);

                pszParameters = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszParameters)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pParameters, -1, pszParameters, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pParameters, cch + 1, pszParameters);

                HeapFree(hProcessHeap, 0, pszParameters);
            }
            break;
        }

        case 4:
        {
            if (ppi4w->pPrinterName)
            {
                PSTR pszPrinterName;

                // Convert Unicode pPrinterName to a ANSI string pszPrinterName.
                cch = wcslen(ppi4w->pPrinterName);

                pszPrinterName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszPrinterName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi4w->pPrinterName, -1, pszPrinterName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi4a->pPrinterName, cch + 1, pszPrinterName);

                HeapFree(hProcessHeap, 0, pszPrinterName);
            }

            if (ppi4w->pServerName)
            {
                PSTR pszServerName;

                // Convert Unicode pServerName to a ANSI string pszServerName.
                cch = wcslen(ppi4w->pServerName);

                pszServerName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszServerName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi4w->pServerName, -1, pszServerName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi4a->pServerName, cch + 1, pszServerName);

                HeapFree(hProcessHeap, 0, pszServerName);
            }
            break;
        }

        case 5:
        {
            if (ppi5w->pPrinterName)
            {
                PSTR pszPrinterName;

                // Convert Unicode pPrinterName to a ANSI string pszPrinterName.
                cch = wcslen(ppi5w->pPrinterName);

                pszPrinterName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszPrinterName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi5w->pPrinterName, -1, pszPrinterName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi5a->pPrinterName, cch + 1, pszPrinterName);

                HeapFree(hProcessHeap, 0, pszPrinterName);
            }

            if (ppi5w->pPortName)
            {
                PSTR pszPortName;

                // Convert Unicode pPortName to a ANSI string pszPortName.
                cch = wcslen(ppi5w->pPortName);

                pszPortName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszPortName)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi5w->pPortName, -1, pszPortName, cch + 1, NULL, NULL);
                StringCchCopyA(ppi5a->pPortName, cch + 1, pszPortName);

                HeapFree(hProcessHeap, 0, pszPortName);
            }
            break;
        }

        case 7:
        {
            if (ppi7w->pszObjectGUID)
            {
                PSTR pszaObjectGUID;

                // Convert Unicode pszObjectGUID to a ANSI string pszaObjectGUID.
                cch = wcslen(ppi7w->pszObjectGUID);

                pszaObjectGUID = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
                if (!pszaObjectGUID)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi7w->pszObjectGUID, -1, pszaObjectGUID, cch + 1, NULL, NULL);
                StringCchCopyA(ppi7a->pszObjectGUID, cch + 1, pszaObjectGUID);

                HeapFree(hProcessHeap, 0, pszaObjectGUID);
            }
            break;
        }
    }       // switch

Cleanup:
    return bReturnValue;
}

BOOL WINAPI
GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{   
    /*
     * We are mapping multiple different pointers to the same pDriverInfo pointer here so that
     * we can use the same incoming pointer for different Levels
     */
    PDRIVER_INFO_1W pdi1w = (PDRIVER_INFO_1W)pDriverInfo;
    PDRIVER_INFO_2W pdi2w = (PDRIVER_INFO_2W)pDriverInfo;
    PDRIVER_INFO_3W pdi3w = (PDRIVER_INFO_3W)pDriverInfo;
    PDRIVER_INFO_4W pdi4w = (PDRIVER_INFO_4W)pDriverInfo;
    PDRIVER_INFO_5W pdi5w = (PDRIVER_INFO_5W)pDriverInfo;
    PDRIVER_INFO_6W pdi6w = (PDRIVER_INFO_6W)pDriverInfo;

    BOOL bReturnValue = FALSE;
    DWORD cch;
    PWSTR pwszEnvironment = NULL;

    TRACE("GetPrinterDriverA(%p, %s, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);

    // Check for invalid levels here for early error return. Should be 1-6.
    if (Level <  1 || Level > 6)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        ERR("Invalid Level!\n");
        goto Exit;
    }

    if (pEnvironment)
    {
        // Convert pEnvironment to a Unicode string pwszEnvironment.
        cch = strlen(pEnvironment);

        pwszEnvironment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszEnvironment)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Exit;
        }

        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, pwszEnvironment, cch + 1);
    }

    bReturnValue = GetPrinterDriverW(hPrinter, pwszEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);
    TRACE("*pcbNeeded is '%d' and bReturnValue is '%d' and GetLastError is '%ld'.\n", *pcbNeeded, bReturnValue, GetLastError());

    if (pwszEnvironment)
    {
        HeapFree(hProcessHeap, 0, pwszEnvironment);
    }

    if (!bReturnValue)
    {
        TRACE("GetPrinterDriverW failed!\n");
        goto Exit;
    }

    // Do Unicode to ANSI conversions for strings based on Level
    switch (Level)
    {
        case 1:
        {
            if (!UnicodeToAnsiInPlace(pdi1w->pName))
                goto Exit;

            break;
        }

        case 2:
        {
            if (!UnicodeToAnsiInPlace(pdi2w->pName))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi2w->pEnvironment))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi2w->pDriverPath))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi2w->pDataFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi2w->pConfigFile))
                goto Exit;

            break;
        }

        case 3:
        {
            if (!UnicodeToAnsiInPlace(pdi3w->pName))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi3w->pEnvironment))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi3w->pDriverPath))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi3w->pDataFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi3w->pConfigFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi3w->pHelpFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi3w->pDependentFiles))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi3w->pMonitorName))
                goto Exit;
 
            if (!UnicodeToAnsiInPlace(pdi3w->pDefaultDataType))
                goto Exit;

            break;
        }

        case 4:
        {
            if (!UnicodeToAnsiInPlace(pdi4w->pName))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pEnvironment))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pDriverPath))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pDataFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pConfigFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pHelpFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pDependentFiles))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pMonitorName))
                goto Exit;
 
            if (!UnicodeToAnsiInPlace(pdi4w->pDefaultDataType))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi4w->pszzPreviousNames))
                goto Exit;

            break;
        }

        case 5:
        {
            if (!UnicodeToAnsiInPlace(pdi5w->pName))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi5w->pEnvironment))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi5w->pDriverPath))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi5w->pDataFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi5w->pConfigFile))
                goto Exit;

            break;
        }

        case 6:
        {
            if (!UnicodeToAnsiInPlace(pdi6w->pName))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pEnvironment))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pDriverPath))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pDataFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pConfigFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pHelpFile))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pDependentFiles))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pMonitorName))
                goto Exit;
 
            if (!UnicodeToAnsiInPlace(pdi6w->pDefaultDataType))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pszzPreviousNames))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pszMfgName))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pszOEMUrl))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pszHardwareID))
                goto Exit;

            if (!UnicodeToAnsiInPlace(pdi6w->pszProvider))
                goto Exit;
        }
    }

    bReturnValue = TRUE;

Exit:

    return bReturnValue;
}

BOOL WINAPI
GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("GetPrinterDriverW(%p, %S, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Dismiss invalid levels already at this point.
    if (Level > 8 || Level < 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (cbBuf && pDriverInfo)
        ZeroMemory(pDriverInfo, cbBuf);

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrinterDriver(pHandle->hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcGetPrinterDriver failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 5);
        MarshallUpStructure(cbBuf, pDriverInfo, pPrinterDriverMarshalling[Level]->pInfo, pPrinterDriverMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("GetPrinterW(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pPrinter, cbBuf, pcbNeeded);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Dismiss invalid levels already at this point.
    if (Level > 9)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (cbBuf && pPrinter)
        ZeroMemory(pPrinter, cbBuf);

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrinter(pHandle->hPrinter, Level, pPrinter, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcGetPrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 9);
        MarshallUpStructure(cbBuf, pPrinter, pPrinterInfoMarshalling[Level]->pInfo, pPrinterInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
OpenPrinterA(LPSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    BOOL bReturnValue = FALSE;
    DWORD cch;
    PWSTR pwszPrinterName = NULL;
    PRINTER_DEFAULTSW wDefault = { 0 };

    TRACE("OpenPrinterA(%s, %p, %p)\n", pPrinterName, phPrinter, pDefault);

    if (pPrinterName)
    {
        // Convert pPrinterName to a Unicode string pwszPrinterName
        cch = strlen(pPrinterName);

        pwszPrinterName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszPrinterName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pPrinterName, -1, pwszPrinterName, cch + 1);
    }

    if (pDefault)
    {
        wDefault.DesiredAccess = pDefault->DesiredAccess;

        if (pDefault->pDatatype)
        {
            // Convert pDefault->pDatatype to a Unicode string wDefault.pDatatype
            cch = strlen(pDefault->pDatatype);

            wDefault.pDatatype = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
            if (!wDefault.pDatatype)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ERR("HeapAlloc failed!\n");
                goto Cleanup;
            }

            MultiByteToWideChar(CP_ACP, 0, pDefault->pDatatype, -1, wDefault.pDatatype, cch + 1);
        }

        if (pDefault->pDevMode)
            wDefault.pDevMode = GdiConvertToDevmodeW(pDefault->pDevMode);
    }

    bReturnValue = OpenPrinterW(pwszPrinterName, phPrinter, &wDefault);

Cleanup:
    if (wDefault.pDatatype)
        HeapFree(hProcessHeap, 0, wDefault.pDatatype);

    if (wDefault.pDevMode)
        HeapFree(hProcessHeap, 0, wDefault.pDevMode);

    if (pwszPrinterName)
        HeapFree(hProcessHeap, 0, pwszPrinterName);

    return bReturnValue;
}

BOOL WINAPI
OpenPrinterW(LPWSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    DWORD dwErrorCode;
    HANDLE hPrinter;
    PSPOOLER_HANDLE pHandle;
    PWSTR pDatatype = NULL;
    WINSPOOL_DEVMODE_CONTAINER DevModeContainer = { 0 };
    ACCESS_MASK AccessRequired = 0;

    TRACE("OpenPrinterW(%S, %p, %p)\n", pPrinterName, phPrinter, pDefault);

    // Sanity check
    if (!phPrinter)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Prepare the additional parameters in the format required by _RpcOpenPrinter
    if (pDefault)
    {
        pDatatype = pDefault->pDatatype;
        DevModeContainer.cbBuf = sizeof(DEVMODEW);
        DevModeContainer.pDevMode = (BYTE*)pDefault->pDevMode;
        AccessRequired = pDefault->DesiredAccess;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcOpenPrinter(pPrinterName, &hPrinter, pDatatype, &DevModeContainer, AccessRequired);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcOpenPrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Create a new SPOOLER_HANDLE structure.
        pHandle = HeapAlloc(hProcessHeap, HEAP_ZERO_MEMORY, sizeof(SPOOLER_HANDLE));
        if (!pHandle)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        pHandle->hPrinter = hPrinter;
        pHandle->hSPLFile = INVALID_HANDLE_VALUE;

        // Return it as phPrinter.
        *phPrinter = (HANDLE)pHandle;
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
ReadPrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pNoBytesRead)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("ReadPrinter(%p, %p, %lu, %p)\n", hPrinter, pBuf, cbBuf, pNoBytesRead);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcReadPrinter(pHandle->hPrinter, pBuf, cbBuf, pNoBytesRead);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcReadPrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
ResetPrinterA(HANDLE hPrinter, PPRINTER_DEFAULTSA pDefault)
{
    TRACE("ResetPrinterA(%p, %p)\n", hPrinter, pDefault);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
ResetPrinterW(HANDLE hPrinter, PPRINTER_DEFAULTSW pDefault)
{
    TRACE("ResetPrinterW(%p, %p)\n", hPrinter, pDefault);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetDefaultPrinterA(LPCSTR pszPrinter)
{
    BOOL bReturnValue = FALSE;
    DWORD cch;
    PWSTR pwszPrinter = NULL;

    TRACE("SetDefaultPrinterA(%s)\n", pszPrinter);

    if (pszPrinter)
    {
        // Convert pszPrinter to a Unicode string pwszPrinter
        cch = strlen(pszPrinter);

        pwszPrinter = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszPrinter)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pszPrinter, -1, pwszPrinter, cch + 1);
    }

    bReturnValue = SetDefaultPrinterW(pwszPrinter);

Cleanup:
    if (pwszPrinter)
        HeapFree(hProcessHeap, 0, pwszPrinter);

    return bReturnValue;
}

BOOL WINAPI
SetDefaultPrinterW(LPCWSTR pszPrinter)
{
    const WCHAR wszDevicesKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices";

    DWORD cbDeviceValueData;
    DWORD cbPrinterValueData = 0;
    DWORD cchPrinter;
    DWORD dwErrorCode;
    HKEY hDevicesKey = NULL;
    HKEY hWindowsKey = NULL;
    PWSTR pwszDeviceValueData = NULL;
    WCHAR wszPrinter[MAX_PRINTER_NAME + 1];

    TRACE("SetDefaultPrinterW(%S)\n", pszPrinter);

    // Open the Devices registry key.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_CURRENT_USER, wszDevicesKey, 0, KEY_READ, &hDevicesKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Did the caller give us a printer to set as default?
    if (pszPrinter && *pszPrinter)
    {
        // Check if the given printer exists and query the value data size.
        dwErrorCode = (DWORD)RegQueryValueExW(hDevicesKey, pszPrinter, NULL, NULL, NULL, &cbPrinterValueData);
        if (dwErrorCode == ERROR_FILE_NOT_FOUND)
        {
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }
        else if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegQueryValueExW failed with status %lu!\n", dwErrorCode);
            goto Cleanup;
        }

        cchPrinter = wcslen(pszPrinter);
    }
    else
    {
        // If there is already a default printer, we're done!
        cchPrinter = _countof(wszPrinter);
        if (GetDefaultPrinterW(wszPrinter, &cchPrinter))
        {
            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }

        // Otherwise, get us the first printer from the "Devices" key to later set it as default and query the value data size.
        cchPrinter = _countof(wszPrinter);
        dwErrorCode = (DWORD)RegEnumValueW(hDevicesKey, 0, wszPrinter, &cchPrinter, NULL, NULL, NULL, &cbPrinterValueData);
        if (dwErrorCode != ERROR_MORE_DATA)
            goto Cleanup;

        pszPrinter = wszPrinter;
    }

    // We now need to query the value data, which has the format "winspool,<Port>:"
    // and make "<Printer Name>,winspool,<Port>:" out of it.
    // Allocate a buffer large enough for the final data.
    cbDeviceValueData = (cchPrinter + 1) * sizeof(WCHAR) + cbPrinterValueData;
    pwszDeviceValueData = HeapAlloc(hProcessHeap, 0, cbDeviceValueData);
    if (!pwszDeviceValueData)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("HeapAlloc failed!\n");
        goto Cleanup;
    }

    // Copy the Printer Name and a comma into it.
    CopyMemory(pwszDeviceValueData, pszPrinter, cchPrinter * sizeof(WCHAR));
    pwszDeviceValueData[cchPrinter] = L',';

    // Append the value data, which has the format "winspool,<Port>:"
    dwErrorCode = (DWORD)RegQueryValueExW(hDevicesKey, pszPrinter, NULL, NULL, (PBYTE)&pwszDeviceValueData[cchPrinter + 1], &cbPrinterValueData);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Open the Windows registry key.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_CURRENT_USER, wszWindowsKey, 0, KEY_SET_VALUE, &hWindowsKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Store our new default printer.
    dwErrorCode = (DWORD)RegSetValueExW(hWindowsKey, wszDeviceValue, 0, REG_SZ, (PBYTE)pwszDeviceValueData, cbDeviceValueData);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

Cleanup:
    if (hDevicesKey)
        RegCloseKey(hDevicesKey);

    if (hWindowsKey)
        RegCloseKey(hWindowsKey);

    if (pwszDeviceValueData)
        HeapFree(hProcessHeap, 0, pwszDeviceValueData);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SetPrinterA(HANDLE hPrinter, DWORD Level, PBYTE pPrinter, DWORD Command)
{
    TRACE("SetPrinterA(%p, %lu, %p, %lu)\n", hPrinter, Level, pPrinter, Command);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pPrinter, DWORD Command)
{
    TRACE("SetPrinterW(%p, %lu, %p, %lu)\n", hPrinter, Level, pPrinter, Command);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SplDriverUnloadComplete(LPWSTR pDriverFile)
{
    TRACE("DriverUnloadComplete(%S)\n", pDriverFile);
    UNIMPLEMENTED;
    return TRUE; // return true for now.
}

DWORD WINAPI
StartDocPrinterA(HANDLE hPrinter, DWORD Level, PBYTE pDocInfo)
{
    DOC_INFO_1W wDocInfo1 = { 0 };
    DWORD cch;
    DWORD dwErrorCode;
    DWORD dwReturnValue = 0;
    PDOC_INFO_1A pDocInfo1 = (PDOC_INFO_1A)pDocInfo;

    TRACE("StartDocPrinterA(%p, %lu, %p)\n", hPrinter, Level, pDocInfo);

    // Only check the minimum required for accessing pDocInfo.
    // Additional sanity checks are done in StartDocPrinterW.
    if (!pDocInfo1)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (pDocInfo1->pDatatype)
    {
        // Convert pDocInfo1->pDatatype to a Unicode string wDocInfo1.pDatatype
        cch = strlen(pDocInfo1->pDatatype);

        wDocInfo1.pDatatype = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!wDocInfo1.pDatatype)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pDocInfo1->pDatatype, -1, wDocInfo1.pDatatype, cch + 1);
    }

    if (pDocInfo1->pDocName)
    {
        // Convert pDocInfo1->pDocName to a Unicode string wDocInfo1.pDocName
        cch = strlen(pDocInfo1->pDocName);

        wDocInfo1.pDocName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!wDocInfo1.pDocName)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pDocInfo1->pDocName, -1, wDocInfo1.pDocName, cch + 1);
    }

    if (pDocInfo1->pOutputFile)
    {
        // Convert pDocInfo1->pOutputFile to a Unicode string wDocInfo1.pOutputFile
        cch = strlen(pDocInfo1->pOutputFile);

        wDocInfo1.pOutputFile = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!wDocInfo1.pOutputFile)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pDocInfo1->pOutputFile, -1, wDocInfo1.pOutputFile, cch + 1);
    }

    dwReturnValue = StartDocPrinterW(hPrinter, Level, (PBYTE)&wDocInfo1);
    dwErrorCode = GetLastError();

Cleanup:
    if (wDocInfo1.pDatatype)
        HeapFree(hProcessHeap, 0, wDocInfo1.pDatatype);

    if (wDocInfo1.pDocName)
        HeapFree(hProcessHeap, 0, wDocInfo1.pDocName);

    if (wDocInfo1.pOutputFile)
        HeapFree(hProcessHeap, 0, wDocInfo1.pOutputFile);

    SetLastError(dwErrorCode);
    return dwReturnValue;
}

DWORD WINAPI
StartDocPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pDocInfo)
{
    DWORD cbAddJobInfo1;
    DWORD cbNeeded;
    DWORD dwErrorCode;
    DWORD dwReturnValue = 0;
    PADDJOB_INFO_1W pAddJobInfo1 = NULL;
    PDOC_INFO_1W pDocInfo1 = (PDOC_INFO_1W)pDocInfo;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("StartDocPrinterW(%p, %lu, %p)\n", hPrinter, Level, pDocInfo);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    if (!pDocInfo1)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (pHandle->bStartedDoc)
    {
        dwErrorCode = ERROR_INVALID_PRINTER_STATE;
        goto Cleanup;
    }

    // Check if we want to redirect output into a file.
    if (pDocInfo1->pOutputFile)
    {
        // Do a StartDocPrinter RPC call in this case.
        dwErrorCode = _StartDocPrinterWithRPC(pHandle, pDocInfo1);
    }
    else
    {
        // Allocate memory for the ADDJOB_INFO_1W structure and a path.
        cbAddJobInfo1 = sizeof(ADDJOB_INFO_1W) + MAX_PATH * sizeof(WCHAR);
        pAddJobInfo1 = HeapAlloc(hProcessHeap, 0, cbAddJobInfo1);
        if (!pAddJobInfo1)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        // Try to add a new job.
        // This only succeeds if the printer is set to do spooled printing.
        if (AddJobW((HANDLE)pHandle, 1, (PBYTE)pAddJobInfo1, cbAddJobInfo1, &cbNeeded))
        {
            // Do spooled printing.
            dwErrorCode = _StartDocPrinterSpooled(pHandle, pDocInfo1, pAddJobInfo1);
        }
        else if (GetLastError() == ERROR_INVALID_ACCESS)
        {
            // ERROR_INVALID_ACCESS is returned when the printer is set to do direct printing.
            // In this case, we do a StartDocPrinter RPC call.
            dwErrorCode = _StartDocPrinterWithRPC(pHandle, pDocInfo1);
        }
        else
        {
            dwErrorCode = GetLastError();
            ERR("AddJobW failed with error %lu!\n", dwErrorCode);
            goto Cleanup;
        }
    }

    if (dwErrorCode == ERROR_SUCCESS)
    {
        pHandle->bStartedDoc = TRUE;
        dwReturnValue = pHandle->dwJobID;
    }

Cleanup:
    if (pAddJobInfo1)
        HeapFree(hProcessHeap, 0, pAddJobInfo1);

    SetLastError(dwErrorCode);
    return dwReturnValue;
}

BOOL WINAPI
StartPagePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("StartPagePrinter(%p)\n", hPrinter);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcStartPagePrinter(pHandle->hPrinter);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcStartPagePrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
WritePrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pcWritten)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("WritePrinter(%p, %p, %lu, %p)\n", hPrinter, pBuf, cbBuf, pcWritten);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    if (!pHandle->bStartedDoc)
    {
        dwErrorCode = ERROR_SPL_NO_STARTDOC;
        goto Cleanup;
    }

    if (pHandle->hSPLFile != INVALID_HANDLE_VALUE)
    {
        // Write to the spool file. This doesn't need an RPC request.
        if (!WriteFile(pHandle->hSPLFile, pBuf, cbBuf, pcWritten, NULL))
        {
            dwErrorCode = GetLastError();
            ERR("WriteFile failed with error %lu!\n", dwErrorCode);
            goto Cleanup;
        }

        dwErrorCode = ERROR_SUCCESS;
    }
    else
    {
        // TODO: This case (for direct printing or remote printing) has bad performance if multiple small-sized WritePrinter calls are performed.
        // We may increase performance by writing into a buffer and only doing a single RPC call when the buffer is full.

        // Do the RPC call
        RpcTryExcept
        {
            dwErrorCode = _RpcWritePrinter(pHandle->hPrinter, pBuf, cbBuf, pcWritten);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErrorCode = RpcExceptionCode();
            ERR("_RpcWritePrinter failed with exception code %lu!\n", dwErrorCode);
        }
        RpcEndExcept;
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
XcvDataW(HANDLE hXcv, PCWSTR pszDataName, PBYTE pInputData, DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded, PDWORD pdwStatus)
{
    TRACE("XcvDataW(%p, %S, %p, %lu, %p, %lu, %p, %p)\n", hXcv, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded, pdwStatus);
    return FALSE;
}
