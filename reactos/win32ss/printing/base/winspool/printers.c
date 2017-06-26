/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Local Constants

/** And the award for the most confusingly named setting goes to "Device", for storing the default printer of the current user.
    Ok, I admit that this has historical reasons. It's still not straightforward in any way though! */
static const WCHAR wszWindowsKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
static const WCHAR wszDeviceValue[] = L"Device";

static void
_MarshallUpPrinterInfo(PBYTE* ppPrinterInfo, DWORD Level)
{
    // Replace relative offset addresses in the output by absolute pointers and advance to the next structure.
    if (Level == 0)
    {
        PPRINTER_INFO_STRESS pPrinterInfo0 = (PPRINTER_INFO_STRESS)(*ppPrinterInfo);

        pPrinterInfo0->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo0->pPrinterName + (ULONG_PTR)pPrinterInfo0);

        if (pPrinterInfo0->pServerName)
            pPrinterInfo0->pServerName = (PWSTR)((ULONG_PTR)pPrinterInfo0->pServerName + (ULONG_PTR)pPrinterInfo0);

        *ppPrinterInfo += sizeof(PRINTER_INFO_STRESS);
    }
    else if (Level == 1)
    {
        PPRINTER_INFO_1W pPrinterInfo1 = (PPRINTER_INFO_1W)(*ppPrinterInfo);

        pPrinterInfo1->pName = (PWSTR)((ULONG_PTR)pPrinterInfo1->pName + (ULONG_PTR)pPrinterInfo1);
        pPrinterInfo1->pDescription = (PWSTR)((ULONG_PTR)pPrinterInfo1->pDescription + (ULONG_PTR)pPrinterInfo1);
        pPrinterInfo1->pComment = (PWSTR)((ULONG_PTR)pPrinterInfo1->pComment + (ULONG_PTR)pPrinterInfo1);

        *ppPrinterInfo += sizeof(PRINTER_INFO_1W);
    }
    else if (Level == 2)
    {
        PPRINTER_INFO_2W pPrinterInfo2 = (PPRINTER_INFO_2W)(*ppPrinterInfo);

        pPrinterInfo2->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pPrinterName + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pShareName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pShareName + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pPortName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pPortName + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pDriverName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pDriverName + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pComment = (PWSTR)((ULONG_PTR)pPrinterInfo2->pComment + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pLocation = (PWSTR)((ULONG_PTR)pPrinterInfo2->pLocation + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pDevMode = (PDEVMODEW)((ULONG_PTR)pPrinterInfo2->pDevMode + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pSepFile = (PWSTR)((ULONG_PTR)pPrinterInfo2->pSepFile + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pPrintProcessor = (PWSTR)((ULONG_PTR)pPrinterInfo2->pPrintProcessor + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pDatatype = (PWSTR)((ULONG_PTR)pPrinterInfo2->pDatatype + (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pParameters = (PWSTR)((ULONG_PTR)pPrinterInfo2->pParameters + (ULONG_PTR)pPrinterInfo2);

        if (pPrinterInfo2->pServerName)
            pPrinterInfo2->pServerName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pServerName + (ULONG_PTR)pPrinterInfo2);

        if (pPrinterInfo2->pSecurityDescriptor)
            pPrinterInfo2->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)((ULONG_PTR)pPrinterInfo2->pSecurityDescriptor + (ULONG_PTR)pPrinterInfo2);

        *ppPrinterInfo += sizeof(PRINTER_INFO_2W);
    }
    else if (Level == 3)
    {
        PPRINTER_INFO_3 pPrinterInfo3 = (PPRINTER_INFO_3)(*ppPrinterInfo);

        pPrinterInfo3->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)((ULONG_PTR)pPrinterInfo3->pSecurityDescriptor + (ULONG_PTR)pPrinterInfo3);

        *ppPrinterInfo += sizeof(PRINTER_INFO_3);
    }
    else if (Level == 4)
    {
        PPRINTER_INFO_4W pPrinterInfo4 = (PPRINTER_INFO_4W)(*ppPrinterInfo);

        pPrinterInfo4->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo4->pPrinterName + (ULONG_PTR)pPrinterInfo4);

        if (pPrinterInfo4->pServerName)
            pPrinterInfo4->pServerName = (PWSTR)((ULONG_PTR)pPrinterInfo4->pServerName + (ULONG_PTR)pPrinterInfo4);

        *ppPrinterInfo += sizeof(PRINTER_INFO_4W);
    }
    else if (Level == 5)
    {
        PPRINTER_INFO_5W pPrinterInfo5 = (PPRINTER_INFO_5W)(*ppPrinterInfo);

        pPrinterInfo5->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo5->pPrinterName + (ULONG_PTR)pPrinterInfo5);
        pPrinterInfo5->pPortName = (PWSTR)((ULONG_PTR)pPrinterInfo5->pPortName + (ULONG_PTR)pPrinterInfo5);

        *ppPrinterInfo += sizeof(PRINTER_INFO_5W);
    }
    else if (Level == 6)
    {
        *ppPrinterInfo += sizeof(PRINTER_INFO_6);
    }
    else if (Level == 7)
    {
        PPRINTER_INFO_7W pPrinterInfo7 = (PPRINTER_INFO_7W)(*ppPrinterInfo);

        if (pPrinterInfo7->pszObjectGUID)
            pPrinterInfo7->pszObjectGUID = (PWSTR)((ULONG_PTR)pPrinterInfo7->pszObjectGUID + (ULONG_PTR)pPrinterInfo7);

        *ppPrinterInfo += sizeof(PRINTER_INFO_7W);
    }
    else if (Level == 8)
    {
        PPRINTER_INFO_8W pPrinterInfo8 = (PPRINTER_INFO_8W)(*ppPrinterInfo);

        pPrinterInfo8->pDevMode = (PDEVMODEW)((ULONG_PTR)pPrinterInfo8->pDevMode + (ULONG_PTR)pPrinterInfo8);

        *ppPrinterInfo += sizeof(PRINTER_INFO_8W);
    }
    else if (Level == 9)
    {
        PPRINTER_INFO_9W pPrinterInfo9 = (PPRINTER_INFO_9W)(*ppPrinterInfo);

        pPrinterInfo9->pDevMode = (PDEVMODEW)((ULONG_PTR)pPrinterInfo9->pDevMode + (ULONG_PTR)pPrinterInfo9);

        *ppPrinterInfo += sizeof(PRINTER_INFO_9W);
    }
}

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

HANDLE WINAPI
AddPrinterW(PWSTR pName, DWORD Level, PBYTE pPrinter)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL WINAPI
ClosePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

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

DWORD WINAPI
DeviceCapabilitiesA(LPCSTR pDevice, LPCSTR pPort, WORD fwCapability, LPSTR pOutput, const DEVMODEA* pDevMode)
{
    return 0;
}

DWORD WINAPI
DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort, WORD fwCapability, LPWSTR pOutput, const DEVMODEW* pDevMode)
{
    return 0;
}

LONG WINAPI
DocumentPropertiesA(HWND hWnd, HANDLE hPrinter, LPSTR pDeviceName, PDEVMODEA pDevModeOutput, PDEVMODEA pDevModeInput, DWORD fMode)
{
    return 0;
}

LONG WINAPI
DocumentPropertiesW(HWND hWnd, HANDLE hPrinter, LPWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput, DWORD fMode)
{
    return 0;
}

BOOL WINAPI
EndDocPrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

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
    return FALSE;
}

BOOL WINAPI
EnumPrintersW(DWORD Flags, PWSTR Name, DWORD Level, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

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
        DWORD i;
        PBYTE p = pPrinterEnum;

        for (i = 0; i < *pcReturned; i++)
            _MarshallUpPrinterInfo(&p, Level);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetDefaultPrinterA(LPSTR pszBuffer, LPDWORD pcchBuffer)
{
    DWORD dwErrorCode;
    PWSTR pwszBuffer = NULL;

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
    return FALSE;
}

BOOL WINAPI
GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return FALSE;
}

BOOL WINAPI
GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return FALSE;
}

BOOL WINAPI
GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;

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
        dwErrorCode = _RpcGetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcGetPrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        PBYTE p = pPrinter;
        _MarshallUpPrinterInfo(&p, Level);
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
ResetPrinterW(HANDLE hPrinter, PPRINTER_DEFAULTSW pDefault)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetDefaultPrinterA(LPCSTR pszPrinter)
{
    BOOL bReturnValue = FALSE;
    DWORD cch;
    PWSTR pwszPrinter = NULL;

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
SetPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pPrinter, DWORD Command)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD WINAPI
StartDocPrinterA(HANDLE hPrinter, DWORD Level, PBYTE pDocInfo)
{
    DOC_INFO_1W wDocInfo1 = { 0 };
    DWORD cch;
    DWORD dwErrorCode;
    DWORD dwReturnValue = 0;
    PDOC_INFO_1A pDocInfo1 = (PDOC_INFO_1A)pDocInfo;

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
    return FALSE;
}
