/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/printers.h>
//#include <marshalling/printerdrivers.h>
#include <strsafe.h>

extern HINSTANCE hinstWinSpool;
//
// See winddiui.h, ReactOS version is limited.
// Loading from XyzUI.dll part of XyzDRV.dll set. example TTYUI.DLL or UniDrvUI.DLL.
//
typedef DWORD (WINAPI *DEVICECAPABILITIES) (HANDLE,PWSTR,WORD,PVOID,PDEVMODEW);
static DEVICECAPABILITIES fpDeviceCapabilities;

typedef LONG (WINAPI *DEVICEPROPERTYSHEETS) (PPROPSHEETUI_INFO,LPARAM);
static DEVICEPROPERTYSHEETS fpDevicePropertySheets;
typedef LONG (WINAPI *DOCUMENTPROPERTYSHEETS) (PPROPSHEETUI_INFO,LPARAM);
static DOCUMENTPROPERTYSHEETS fpDocumentPropertySheets;

typedef LONG (WINAPI *COMMONPROPERTYSHEETUIW) (HWND,PFNPROPSHEETUI,LPARAM,LPDWORD);
static COMMONPROPERTYSHEETUIW fpCommonPropertySheetUIW;

typedef LONG (WINAPI *QUERYCOLORPROFILE) (HANDLE,PDEVMODEW,ULONG,PVOID,ULONG*,FLONG*);
static  QUERYCOLORPROFILE fpQueryColorProfile;

typedef BOOL (WINAPI *SPOOLERPRINTEREVENT) (LPWSTR,int,DWORD,LPARAM);
static SPOOLERPRINTEREVENT fpPrinterEvent;

typedef BOOL (WINAPI *DEVQUERYPRINT) (HANDLE,LPDEVMODEW,DWORD*);
static DEVQUERYPRINT fpDevQueryPrint;

typedef BOOL (WINAPI *DEVQUERYPRINTEX) (PDEVQUERYPRINT_INFO);
static DEVQUERYPRINTEX fpDevQueryPrintEx;

//
//  PrintUI.dll
//
LONG WINAPI ConstructPrinterFriendlyName( PWSTR, PVOID, LPDWORD Size );
typedef LONG (WINAPI *CONSTRUCTPRINTERFRIENDLYNAME) (PWSTR,PVOID,LPDWORD);
static CONSTRUCTPRINTERFRIENDLYNAME fpConstructPrinterFriendlyName;

//
//  CompstUI User Data
//
typedef struct _COMPUI_USERDATA
{
  HMODULE hModule;
  LPWSTR pszPrinterName;
} COMPUI_USERDATA, *PCOMPUI_USERDATA;

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
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("AbortPrinter(%p)\n", hPrinter);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pHandle->bTrayIcon = pHandle->bStartedDoc = FALSE;

    if ( pHandle->hSPLFile != INVALID_HANDLE_VALUE && pHandle->bJob )
    {
        // Close any open file handle.
        CloseHandle( pHandle->hSPLFile );
        pHandle->hSPLFile = INVALID_HANDLE_VALUE;

        SetJobW( hPrinter, pHandle->dwJobID, 0, NULL, JOB_CONTROL_DELETE );

        return ScheduleJob( hPrinter, pHandle->dwJobID );
    }

    // Do the RPC call.
    RpcTryExcept
    {
        dwErrorCode = _RpcAbortPrinter(&pHandle->hPrinter);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcAbortPrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

HANDLE WINAPI
AddPrinterA(PSTR pName, DWORD Level, PBYTE pPrinter)
{
    UNICODE_STRING pNameW, usBuffer;
    PWSTR pwstrNameW;
    PRINTER_INFO_2W *ppi2w = (PRINTER_INFO_2W*)pPrinter;
    PRINTER_INFO_2A *ppi2a = (PRINTER_INFO_2A*)pPrinter;
    HANDLE ret = NULL;
    PWSTR pwszPrinterName = NULL;
    PWSTR pwszServerName = NULL;
    PWSTR pwszShareName = NULL;
    PWSTR pwszPortName = NULL;
    PWSTR pwszDriverName = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszLocation = NULL;
    PWSTR pwszSepFile = NULL;
    PWSTR pwszPrintProcessor = NULL;
    PWSTR pwszDatatype = NULL;
    PWSTR pwszParameters = NULL;
    PDEVMODEW pdmw = NULL;

    TRACE("AddPrinterA(%s, %d, %p)\n", debugstr_a(pName), Level, pPrinter);

    if(Level != 2)
    {
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return NULL;
    }

    pwstrNameW = AsciiToUnicode(&pNameW,pName);

    if (ppi2a->pShareName)
    {
        pwszShareName = AsciiToUnicode(&usBuffer, ppi2a->pShareName);
        if (!(ppi2w->pShareName = pwszShareName)) goto Cleanup;
    }
    if (ppi2a->pPortName)
    {
        pwszPortName = AsciiToUnicode(&usBuffer, ppi2a->pPortName);
        if (!(ppi2w->pPortName = pwszPortName)) goto Cleanup;
    }
    if (ppi2a->pDriverName)
    {
        pwszDriverName = AsciiToUnicode(&usBuffer, ppi2a->pDriverName);
        if (!(ppi2w->pDriverName = pwszDriverName)) goto Cleanup;
    }
    if (ppi2a->pComment)
    {
        pwszComment = AsciiToUnicode(&usBuffer, ppi2a->pComment);
        if (!(ppi2w->pComment = pwszComment)) goto Cleanup;
    }
    if (ppi2a->pLocation)
    {
        pwszLocation = AsciiToUnicode(&usBuffer, ppi2a->pLocation);
        if (!(ppi2w->pLocation = pwszLocation)) goto Cleanup;
    }
    if (ppi2a->pSepFile)
    {
        pwszSepFile = AsciiToUnicode(&usBuffer, ppi2a->pSepFile);
        if (!(ppi2w->pSepFile = pwszSepFile)) goto Cleanup;
    }
    if (ppi2a->pServerName)
    {
        pwszPrintProcessor = AsciiToUnicode(&usBuffer, ppi2a->pPrintProcessor);
        if (!(ppi2w->pPrintProcessor = pwszPrintProcessor)) goto Cleanup;
    }
    if (ppi2a->pDatatype)
    {
        pwszDatatype = AsciiToUnicode(&usBuffer, ppi2a->pDatatype);
        if (!(ppi2w->pDatatype = pwszDatatype)) goto Cleanup;
    }
    if (ppi2a->pParameters)
    {
        pwszParameters = AsciiToUnicode(&usBuffer, ppi2a->pParameters);
        if (!(ppi2w->pParameters = pwszParameters)) goto Cleanup;
    }
    if ( ppi2a->pDevMode )
    {
        RosConvertAnsiDevModeToUnicodeDevmode( ppi2a->pDevMode, &pdmw );
        ppi2w->pDevMode = pdmw;
    }
    if (ppi2a->pServerName)
    {
        pwszServerName = AsciiToUnicode(&usBuffer, ppi2a->pServerName);
        if (!(ppi2w->pPrinterName = pwszServerName)) goto Cleanup;
    }
    if (ppi2a->pPrinterName)
    {
        pwszPrinterName = AsciiToUnicode(&usBuffer, ppi2a->pPrinterName);
        if (!(ppi2w->pPrinterName = pwszPrinterName)) goto Cleanup;
    }

    ret = AddPrinterW(pwstrNameW, Level, (LPBYTE)ppi2w);

Cleanup:
    if (pdmw) HeapFree(hProcessHeap, 0, pdmw);
    if (pwszPrinterName) HeapFree(hProcessHeap, 0, pwszPrinterName);
    if (pwszServerName) HeapFree(hProcessHeap, 0, pwszServerName);
    if (pwszShareName) HeapFree(hProcessHeap, 0, pwszShareName);
    if (pwszPortName) HeapFree(hProcessHeap, 0, pwszPortName);
    if (pwszDriverName) HeapFree(hProcessHeap, 0, pwszDriverName);
    if (pwszComment) HeapFree(hProcessHeap, 0, pwszComment);
    if (pwszLocation) HeapFree(hProcessHeap, 0, pwszLocation);
    if (pwszSepFile) HeapFree(hProcessHeap, 0, pwszSepFile);
    if (pwszPrintProcessor) HeapFree(hProcessHeap, 0, pwszPrintProcessor);
    if (pwszDatatype) HeapFree(hProcessHeap, 0, pwszDatatype);
    if (pwszParameters) HeapFree(hProcessHeap, 0, pwszParameters);

    RtlFreeUnicodeString(&pNameW);
    return ret;
}

HANDLE WINAPI
AddPrinterW(PWSTR pName, DWORD Level, PBYTE pPrinter)
{
    DWORD dwErrorCode;
    WINSPOOL_PRINTER_CONTAINER PrinterContainer;
    WINSPOOL_DEVMODE_CONTAINER DevModeContainer;
    WINSPOOL_SECURITY_CONTAINER SecurityContainer;
    SECURITY_DESCRIPTOR *sd = NULL;
    DWORD size;
    HANDLE hPrinter = NULL, hHandle = NULL;
    PSPOOLER_HANDLE pHandle = NULL;

    TRACE("AddPrinterW(%S, %lu, %p)\n", pName, Level, pPrinter);

    DevModeContainer.cbBuf = 0;
    DevModeContainer.pDevMode = NULL;

    SecurityContainer.cbBuf = 0;
    SecurityContainer.pSecurity = NULL;

    if ( Level != 2 )
    {
        FIXME( "Unsupported level %d\n", Level );
        SetLastError( ERROR_INVALID_LEVEL );
        return hHandle;
    }
    else
    {
        PPRINTER_INFO_2W pi2w = (PPRINTER_INFO_2W)pPrinter;
        if ( pi2w )
        {
            if ( pi2w->pDevMode )
            {
                if ( IsValidDevmodeNoSizeW( pi2w->pDevMode ) )
                {
                    DevModeContainer.cbBuf = pi2w->pDevMode->dmSize + pi2w->pDevMode->dmDriverExtra;
                    DevModeContainer.pDevMode = (PBYTE)pi2w->pDevMode;
                }
            }

            if ( pi2w->pSecurityDescriptor )
            {
                sd = get_sd( pi2w->pSecurityDescriptor, &size );
                if ( sd )
                {
                    SecurityContainer.cbBuf = size;
                    SecurityContainer.pSecurity = (PBYTE)sd;
                }
            }
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return hHandle;
        }
    }

    PrinterContainer.PrinterInfo.pPrinterInfo1 = (WINSPOOL_PRINTER_INFO_1*)pPrinter;
    PrinterContainer.Level = Level;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcAddPrinter( pName, &PrinterContainer, &DevModeContainer, &SecurityContainer, &hPrinter );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    if (hPrinter)
    {
        // Create a new SPOOLER_HANDLE structure.
        pHandle = HeapAlloc(hProcessHeap, HEAP_ZERO_MEMORY, sizeof(SPOOLER_HANDLE));
        if (!pHandle)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            _RpcDeletePrinter(hPrinter);
            _RpcClosePrinter(hPrinter);
            goto Cleanup;
        }

        pHandle->Sig = SPOOLER_HANDLE_SIG;
        pHandle->hPrinter = hPrinter;
        pHandle->hSPLFile = INVALID_HANDLE_VALUE;
        pHandle->hSpoolFileHandle = INVALID_HANDLE_VALUE;
        hHandle = (HANDLE)pHandle;
    }

Cleanup:
    if ( sd ) HeapFree( GetProcessHeap(), 0, sd );

    SetLastError(dwErrorCode);
    return hHandle;
}

BOOL WINAPI
ClosePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("ClosePrinter(%p)\n", hPrinter);

    // Sanity checks.
    if ( IntProtectHandle( hPrinter, TRUE ) )
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

    pHandle->Sig = -1;

    // Free the memory for the handle.
    HeapFree(hProcessHeap, 0, pHandle);

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DeletePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("DeletePrinter(%p)\n", hPrinter);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call.
    RpcTryExcept
    {
        dwErrorCode = _RpcDeletePrinter(&pHandle->hPrinter);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcDeletePrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

//
// Based on GDI32:printdrv.c:IntGetPrinterDriver.
//
HMODULE
WINAPI
LoadPrinterDriver( HANDLE hspool )
{
    INT iTries = 0;
    DWORD Size = (sizeof(WCHAR) * MAX_PATH) * 2; // DRIVER_INFO_5W + plus strings.
    PDRIVER_INFO_5W pdi = NULL;
    HMODULE hLibrary = NULL;

    do
    {
        ++iTries;

        pdi = RtlAllocateHeap( GetProcessHeap(), 0, Size);

        if ( !pdi )
            break;

        if ( GetPrinterDriverW(hspool, NULL, 5, (LPBYTE)pdi, Size, &Size) )
        {
            TRACE("Level 5 Size %d\n",Size);

            // Name and load configure library (for example, C:\DRIVERS\Pscrptui.dll). Not printui.dll!

            hLibrary = LoadLibrary(pdi->pConfigFile);

            FIXME("IGPD : Get Printer Driver Config File : %S\n",pdi->pConfigFile);

            RtlFreeHeap( GetProcessHeap(), 0, pdi);
            return hLibrary;
        }

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            ++iTries;

        RtlFreeHeap( GetProcessHeap(), 0, pdi);
     }
     while ( iTries < 2 );
     ERR("No Printer Driver Error %d\n",GetLastError());
     return NULL;
}

DWORD WINAPI
DeviceCapabilitiesA(LPCSTR pDevice, LPCSTR pPort, WORD fwCapability, LPSTR pOutput, const DEVMODEA* pDevMode)
{
    PWSTR pwszDeviceName = NULL;
    PDEVMODEW pdmwInput = NULL;
    BOOL bReturnValue = GDI_ERROR;
    DWORD cch;

    FIXME("DeviceCapabilitiesA(%s, %s, %hu, %p, %p)\n", pDevice, pPort, fwCapability, pOutput, pDevMode);

    if (pDevice)
    {
        // Convert pName to a Unicode string pwszDeviceName.
        cch = strlen(pDevice);

        pwszDeviceName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszDeviceName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pDevice, -1, pwszDeviceName, cch + 1);
    }

    if (pDevMode)
    {
        RosConvertAnsiDevModeToUnicodeDevmode((PDEVMODEA)pDevMode, &pdmwInput);
    }

    // pPort is ignored so no need to pass it.
    bReturnValue = DeviceCapabilitiesW( pwszDeviceName, NULL, fwCapability, (LPWSTR)pOutput, (const DEVMODEW*) pdmwInput );

Cleanup:
    if(pwszDeviceName)
        HeapFree(hProcessHeap, 0, pwszDeviceName);

    if (pdmwInput)
        HeapFree(hProcessHeap, 0, pdmwInput);

    return bReturnValue;
}

DWORD WINAPI
DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort, WORD fwCapability, LPWSTR pOutput, const DEVMODEW* pDevMode)
{
    HANDLE hPrinter;
    HMODULE hLibrary;
    DWORD iDevCap = GDI_ERROR;

    FIXME("DeviceCapabilitiesW(%S, %S, %hu, %p, %p)\n", pDevice, pPort, fwCapability, pOutput, pDevMode);

    if ( pDevMode )
    {
        if (!IsValidDevmodeNoSizeW( (PDEVMODEW)pDevMode ) )
        {
            ERR("DeviceCapabilitiesW : Devode Invalid\n");
            return -1;
        }
    }

    if ( OpenPrinterW( (LPWSTR)pDevice, &hPrinter, NULL ) )
    {
        hLibrary = LoadPrinterDriver( hPrinter );

        if ( hLibrary )
        {
            fpDeviceCapabilities = (PVOID)GetProcAddress( hLibrary, "DrvDeviceCapabilities" );

            if ( fpDeviceCapabilities )
            {
                iDevCap = fpDeviceCapabilities( hPrinter, (PWSTR)pDevice, fwCapability, pOutput, (PDEVMODE)pDevMode );
            }

            FreeLibrary(hLibrary);
        }

        ClosePrinter( hPrinter );
    }

    return iDevCap;
}

BOOL
WINAPI
DevQueryPrint( HANDLE hPrinter, LPDEVMODEW pDevMode, DWORD *pResID)
{
    HMODULE hLibrary;
    BOOL Ret = FALSE;

    hLibrary = LoadPrinterDriver( hPrinter );

    if ( hLibrary )
    {
        fpDevQueryPrint = (PVOID)GetProcAddress( hLibrary, "DevQueryPrint" );

        if ( fpDevQueryPrint )
        {
            Ret = fpDevQueryPrint( hPrinter, pDevMode, pResID );
        }

        FreeLibrary(hLibrary);
    }
    return Ret;
}

BOOL WINAPI
DevQueryPrintEx( PDEVQUERYPRINT_INFO pDQPInfo )
{
    HMODULE hLibrary;
    BOOL Ret = FALSE;

    hLibrary = LoadPrinterDriver( pDQPInfo->hPrinter );

    if ( hLibrary )
    {
        fpDevQueryPrintEx = (PVOID)GetProcAddress( hLibrary, "DevQueryPrintEx" );

        if ( fpDevQueryPrintEx )
        {
            Ret = fpDevQueryPrintEx( pDQPInfo );
        }

        FreeLibrary(hLibrary);
    }
    return Ret;
}

INT WINAPI
DocumentEvent( HANDLE hPrinter, HDC hdc, int iEsc, ULONG cbIn, PVOID pvIn, ULONG cbOut, PVOID pvOut)
{
    FIXME("DocumentEvent(%p, %p, %lu, %lu, %p, %lu, %p)\n", hPrinter, hdc, iEsc, cbIn, pvIn, cbOut, pvOut);
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

    FIXME("DocumentPropertiesA(%p, %p, %s, %p, %p, %lu)\n", hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode);

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
        RosConvertAnsiDevModeToUnicodeDevmode(pDevModeInput, &pdmwInput);
    }

    if (pDevModeOutput)
    {
        // Create working buffer for output from DocumentPropertiesW.

        // Do it RIGHT! Get the F...ing Size!
        LONG Size = DocumentPropertiesW( hWnd, hPrinter, pwszDeviceName, NULL, NULL, 0 );

        if ( Size < 0 )
        {
            goto Cleanup;
        }

        pdmwOutput = HeapAlloc(hProcessHeap, 0, Size);
        if (!pdmwOutput)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }
    }

    lReturnValue = DocumentPropertiesW(hWnd, hPrinter, pwszDeviceName, pdmwOutput, pdmwInput, fMode);
    FIXME("lReturnValue from DocumentPropertiesW is '%ld'.\n", lReturnValue);

    if (lReturnValue < 0)
    {
        FIXME("DocumentPropertiesW failed!\n");
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

PRINTER_INFO_9W * get_devmodeW(HANDLE hprn)
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

BOOL
FASTCALL
CreateUIUserData( ULONG_PTR *puserdata, HANDLE hPrinter )
{
    PCOMPUI_USERDATA pcui_ud = DllAllocSplMem( sizeof(COMPUI_USERDATA) );

    *puserdata = (ULONG_PTR)pcui_ud;
    FIXME("CreateUIUserData\n");
    if ( pcui_ud )
    {
        pcui_ud->hModule = LoadPrinterDriver( hPrinter );

        if ( !pcui_ud->hModule )
        {
            DllFreeSplMem( pcui_ud );
            *puserdata = 0;
        }
    }
    return *puserdata != 0;
}

VOID
FASTCALL
DestroyUIUserData( ULONG_PTR *puserdata )
{
    PCOMPUI_USERDATA pcui_ud = (PCOMPUI_USERDATA)*puserdata;
    FIXME("DestroyUIUserData\n");
    if ( pcui_ud )
    {
        if ( pcui_ud->hModule )
        {
            FreeLibrary( pcui_ud->hModule );
            pcui_ud->hModule = NULL;
        }

        if ( pcui_ud->pszPrinterName )
        {
            DllFreeSplMem( pcui_ud->pszPrinterName );
            pcui_ud->pszPrinterName = NULL;
        }

        DllFreeSplMem( pcui_ud );
        *puserdata = 0;
    }
}

BOOL
FASTCALL
IntFixUpDevModeNames( PDOCUMENTPROPERTYHEADER pdphdr )
{
    PRINTER_INFO_2W *pi2 = NULL;
    DWORD needed = 0;
    BOOL res;

    if (!(pdphdr->fMode & DM_OUT_BUFFER) ||
         pdphdr->fMode & DM_NOPERMISSION || // Do not allow the user to modify properties on the displayed property sheet pages.
        !pdphdr->pdmOut )
    {
        return FALSE;
    }

    res = GetPrinterW( pdphdr->hPrinter, 2, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        pi2 = HeapAlloc(hProcessHeap, 0, needed);
        res = GetPrinterW( pdphdr->hPrinter, 2, (LPBYTE)pi2, needed, &needed);
    }

    if (res)
    {
        FIXME("IFUDMN : Get Printer Name %S\n",pi2->pPrinterName);
        StringCchCopyW( pdphdr->pdmOut->dmDeviceName, CCHDEVICENAME-1, pi2->pPrinterName );
        pdphdr->pdmOut->dmDeviceName[CCHDEVICENAME-1] = 0;
    }
    else
    {
        ERR("IFUDMN : GetPrinterW failed with %u\n", GetLastError());
    }
    HeapFree(hProcessHeap, 0, pi2);
    return res;
}

LONG
WINAPI
CreatePrinterFriendlyName( PCOMPUI_USERDATA pcui_ud, LPWSTR pszPrinterName )
{
    LONG Result = 0;
    DWORD Size = 0;
    HMODULE hLibrary = NULL;

    hLibrary = LoadLibraryA( "printui.dll" );

    if ( hLibrary )
    {
        fpConstructPrinterFriendlyName = (PVOID)GetProcAddress( hLibrary, "ConstructPrinterFriendlyName" );

        if ( fpConstructPrinterFriendlyName )
        {
             if ( !fpConstructPrinterFriendlyName( pszPrinterName, NULL, &Size ) )
             {
                 if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                 {
                     PWSTR pwstr = DllAllocSplMem( (Size + 1) * sizeof(WCHAR) );

                     pcui_ud->pszPrinterName = pwstr;

                     if ( pwstr )
                         Result = fpConstructPrinterFriendlyName( pszPrinterName, pwstr, &Size );
                 }
             }
        }
        FreeLibrary( hLibrary );
    }

    if ( !Result )
    {
        DllFreeSplMem( pcui_ud->pszPrinterName );
        pcui_ud->pszPrinterName = AllocSplStr( pszPrinterName );
    }

    return Result;
}

//
// Tested with XP CompstUI as a callback and works. Fails perfectly.
//
LONG
WINAPI
DocumentPropertySheets( PPROPSHEETUI_INFO pCPSUIInfo, LPARAM lparam )
{
    LONG Result = -1;
    PDOCUMENTPROPERTYHEADER pdphdr;

    FIXME("DocumentPropertySheets(%p, 0x%lx)\n", pCPSUIInfo, lparam);

    // If pPSUIInfo is NULL, and if either lParam -> fMode is zero or lParam -> pdmOut is NULL,
    // this function should return the size, in bytes, of the printer's DEVMODEW structure.
    if ( !pCPSUIInfo && lparam )
    {
        pdphdr = (PDOCUMENTPROPERTYHEADER)lparam;

        if ( pdphdr->cbSize >= sizeof(PDOCUMENTPROPERTYHEADER) &&
            !(pdphdr->fMode & DM_PROMPT) )
        {
            HMODULE hLibrary = LoadPrinterDriver( pdphdr->hPrinter );

            if ( hLibrary )
            {
                fpDocumentPropertySheets = (PVOID)GetProcAddress( hLibrary, "DrvDocumentPropertySheets" );

                if ( fpDocumentPropertySheets )
                {
                    FIXME("DPS : fpDocumentPropertySheets(%p, 0x%lx) pdmOut %p\n", pCPSUIInfo, lparam, pdphdr->pdmOut);
                    Result = fpDocumentPropertySheets( pCPSUIInfo, lparam );
                    FIXME("DPS : fpDocumentPropertySheets result %d cbOut %d\n",Result, pdphdr->cbOut);
                }
                else
                {
                    //
                    // ReactOS backup!!! Currently no supporting UI driver.
                    //
                    PRINTER_INFO_9W * pi9 = get_devmodeW( pdphdr->hPrinter );
                    if ( pi9 )
                    {
                        Result = pi9->pDevMode->dmSize + pi9->pDevMode->dmDriverExtra;
                        FIXME("IDPS : Using ReactOS backup!!! DevMode Size %d\n",Result);
                        HeapFree(hProcessHeap, 0, pi9);
                    }
                }

                FreeLibrary(hLibrary);

                if ( Result > 0 )
                {
                    IntFixUpDevModeNames( pdphdr );
                }

                return Result;
            }
            else
            {
                SetLastError(ERROR_INVALID_HANDLE);
            }
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
        return Result;
    }

    Result = 0;

    if ( pCPSUIInfo )
    {
        PSETRESULT_INFO psri;
        PPROPSHEETUI_INFO_HEADER ppsuiihdr;
        PCOMPUI_USERDATA pcui_ud;
        pdphdr = (PDOCUMENTPROPERTYHEADER)pCPSUIInfo->lParamInit;

        if ( pdphdr->cbSize < sizeof(PDOCUMENTPROPERTYHEADER) )
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return Result;
        }

        switch ( pCPSUIInfo->Reason )
        {
            case PROPSHEETUI_REASON_INIT:
            {
                FIXME("DocPS : PROPSHEETUI_REASON_INIT\n");
                if ( CreateUIUserData( &pCPSUIInfo->UserData, pdphdr->hPrinter ) )
                {
                    pcui_ud = (PCOMPUI_USERDATA)pCPSUIInfo->UserData;

                    fpDocumentPropertySheets = (PVOID)GetProcAddress( pcui_ud->hModule, "DrvDocumentPropertySheets" );

                    if ( fpDocumentPropertySheets )
                    {
                        pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                     CPSFUNC_SET_FUSION_CONTEXT,
                                                     -3,  // What type of handle is this?
                                                     0 ); // Not used, must be zero.

                        Result = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                              CPSFUNC_ADD_PFNPROPSHEETUIW,
                                                             (LPARAM)fpDocumentPropertySheets,
                                                              pCPSUIInfo->lParamInit );
                        break;
                    }
                    FIXME("DocPS : PROPSHEETUI_REASON_INIT Fail\n");
                    DestroyUIUserData( &pCPSUIInfo->UserData );
                }
            }
                break;

            case PROPSHEETUI_REASON_GET_INFO_HEADER:
                FIXME("DocPS : PROPSHEETUI_REASON_GET_INFO_HEADER\n");

                ppsuiihdr = (PPROPSHEETUI_INFO_HEADER)lparam;

                pcui_ud = (PCOMPUI_USERDATA)pCPSUIInfo->UserData;

                CreatePrinterFriendlyName( pcui_ud, pdphdr->pszPrinterName );

                ppsuiihdr->Flags  = PSUIHDRF_NOAPPLYNOW|PSUIHDRF_PROPTITLE;
                ppsuiihdr->pTitle = pcui_ud->pszPrinterName;
                ppsuiihdr->hInst  = hinstWinSpool;
                ppsuiihdr->IconID = IDI_CPSUI_DOCUMENT;

                Result = CPSUI_OK;
                break;

            case PROPSHEETUI_REASON_DESTROY:
                FIXME("DocPS : PROPSHEETUI_REASON_DESTROY\n");
                DestroyUIUserData( &pCPSUIInfo->UserData );
                Result = CPSUI_OK;
                break;

            case PROPSHEETUI_REASON_SET_RESULT:
                FIXME("DocPS : PROPSHEETUI_REASON_SET_RESULT\n");

                psri = (PSETRESULT_INFO)lparam;

                pCPSUIInfo->Result = psri->Result;
                if ( pCPSUIInfo->Result > 0 )
                {
                    IntFixUpDevModeNames( pdphdr );
                }
                Result = CPSUI_OK;
                break;
        }
    }
    return Result;
}

LONG
WINAPI
DevicePropertySheets( PPROPSHEETUI_INFO pCPSUIInfo, LPARAM lparam )
{
    LONG Result = 0;
    PDEVICEPROPERTYHEADER pdphdr;

    FIXME("DevicePropertySheets(%p, 0x%lx)\n", pCPSUIInfo, lparam);

    if ( pCPSUIInfo )
    {
        PSETRESULT_INFO psri;
        PPROPSHEETUI_INFO_HEADER ppsuiihdr;
        PCOMPUI_USERDATA pcui_ud;
        pdphdr = (PDEVICEPROPERTYHEADER)pCPSUIInfo->lParamInit;

        if ( pdphdr->cbSize < sizeof(DEVICEPROPERTYHEADER) )
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return Result;
        }

        switch ( pCPSUIInfo->Reason )
        {
            case PROPSHEETUI_REASON_INIT:
            {
                FIXME("DevPS : PROPSHEETUI_REASON_INIT\n");
                if ( CreateUIUserData( &pCPSUIInfo->UserData, pdphdr->hPrinter ) )
                {
                    pcui_ud = (PCOMPUI_USERDATA)pCPSUIInfo->UserData;

                    fpDevicePropertySheets = (PVOID)GetProcAddress( pcui_ud->hModule, "DrvDevicePropertySheets" );

                    if ( fpDevicePropertySheets )
                    {
                        pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                     CPSFUNC_SET_FUSION_CONTEXT,
                                                     -3,  // What type of handle is this?
                                                     0 ); // Not used, must be zero.

                        Result = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                              CPSFUNC_ADD_PFNPROPSHEETUIW,
                                                             (LPARAM)fpDevicePropertySheets,
                                                              pCPSUIInfo->lParamInit );
                        break;
                    }
                    FIXME("DevPS : PROPSHEETUI_REASON_INIT Fail\n");
                    DestroyUIUserData( &pCPSUIInfo->UserData );
                }
            }
                break;

            case PROPSHEETUI_REASON_GET_INFO_HEADER:
                FIXME("DevPS : PROPSHEETUI_REASON_GET_INFO_HEADER\n");

                ppsuiihdr = (PPROPSHEETUI_INFO_HEADER)lparam;

                pcui_ud = (PCOMPUI_USERDATA)pCPSUIInfo->UserData;

                CreatePrinterFriendlyName( pcui_ud, pdphdr->pszPrinterName );

                ppsuiihdr->Flags  = PSUIHDRF_NOAPPLYNOW|PSUIHDRF_PROPTITLE;
                ppsuiihdr->pTitle = pcui_ud->pszPrinterName;
                ppsuiihdr->hInst  = hinstWinSpool;
                ppsuiihdr->IconID = IDI_CPSUI_DOCUMENT;

                Result = CPSUI_OK;
                break;

            case PROPSHEETUI_REASON_DESTROY:
                FIXME("DevPS : PROPSHEETUI_REASON_DESTROY\n");
                DestroyUIUserData( &pCPSUIInfo->UserData );
                Result = CPSUI_OK;
                break;

            case PROPSHEETUI_REASON_SET_RESULT:
                FIXME("DevPS : PROPSHEETUI_REASON_SET_RESULT\n");
                psri = (PSETRESULT_INFO)lparam;
                pCPSUIInfo->Result = psri->Result;
                Result = CPSUI_OK;
                break;
        }
    }
    return Result;
}

LONG
WINAPI
CallCommonPropertySheetUI(HWND hWnd, PFNPROPSHEETUI pfnPropSheetUI, LPARAM lparam, LPDWORD pResult)
{
    HMODULE hLibrary = NULL;
    LONG Ret = ERR_CPSUI_GETLASTERROR;

    FIXME("CallCommonPropertySheetUI(%p, %p, 0x%lx, %p)\n", hWnd, pfnPropSheetUI, lparam, pResult);

    if ( ( hLibrary = LoadLibraryA( "compstui.dll" ) ) )
    {
        fpCommonPropertySheetUIW = (PVOID) GetProcAddress(hLibrary, "CommonPropertySheetUIW");

        if ( fpCommonPropertySheetUIW )
        {
            Ret = fpCommonPropertySheetUIW( hWnd, pfnPropSheetUI, lparam, pResult );
        }

        FreeLibrary(hLibrary);
    }
    return Ret;
}

LONG WINAPI
DocumentPropertiesW(HWND hWnd, HANDLE hPrinter, LPWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput, DWORD fMode)
{
    HANDLE hUseHandle = NULL;
    DOCUMENTPROPERTYHEADER docprophdr;
    LONG Result = IDOK;

    FIXME("DocumentPropertiesW(%p, %p, %S, %p, %p, %lu)\n", hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode);

    if (hPrinter)
    {
        hUseHandle = hPrinter;
    }
    else if (!OpenPrinterW(pDeviceName, &hUseHandle, NULL))
    {
        ERR("No handle, and no usable printer name passed in\n");
        return -1;
    }

    if ( !(fMode & DM_IN_BUFFER ) ||
         ( ( pDevModeInput && !IsValidDevmodeNoSizeW( (PDEVMODEW)pDevModeInput ) ) ) )
    {
        pDevModeInput = NULL;
        fMode &= ~DM_IN_BUFFER;
    }

    docprophdr.cbSize         = sizeof(DOCUMENTPROPERTYHEADER);
    docprophdr.Reserved       = 0;
    docprophdr.hPrinter       = hUseHandle;
    docprophdr.pszPrinterName = pDeviceName;
    docprophdr.cbOut          = 0;

    if ( pDevModeOutput )
    {
        docprophdr.pdmIn  = NULL;
        docprophdr.pdmOut = NULL;
        docprophdr.fMode  = 0;
        FIXME("DPW : Call DocumentPropertySheets with pDevModeOutput %p\n",pDevModeOutput);
        docprophdr.cbOut  = DocumentPropertySheets( NULL, (LPARAM)&docprophdr );
    }

    docprophdr.pdmIn  = pDevModeInput;
    docprophdr.pdmOut = pDevModeOutput;
    docprophdr.fMode  = fMode;

    if ( fMode & DM_IN_PROMPT )
    {
        Result = CPSUI_CANCEL;

        //
        // Now call the Property Sheet for Print > Properties.
        //
        if ( CallCommonPropertySheetUI( hWnd, (PFNPROPSHEETUI)DocumentPropertySheets, (LPARAM)&docprophdr, (LPDWORD)&Result ) < 0 )
        {
            FIXME("CallCommonPropertySheetUI return error\n");
            Result = ERR_CPSUI_GETLASTERROR;
        }
        else
            Result = (Result == CPSUI_OK) ? IDOK : IDCANCEL;
        FIXME("CallCommonPropertySheetUI returned\n");
    }
    else
    {
        FIXME("DPW : CallDocumentPropertySheets\n");
        Result = DocumentPropertySheets( NULL, (LPARAM)&docprophdr );
    }

    if ( Result != ERR_CPSUI_GETLASTERROR || Result != ERR_CPSUI_ALLOCMEM_FAILED )
    {
        if ( pDevModeOutput )
        {
            if ( !IsValidDevmodeNoSizeW( pDevModeOutput ) )
            {
                ERR("DPW : Improper pDevModeOutput size.\n");
                Result = -1;
            }
        }
        else
        {
            ERR("No pDevModeOutput\n");
        }
    }

    if (hUseHandle && !hPrinter)
        ClosePrinter(hUseHandle);
    return Result;
}

BOOL
WINAPI
PrinterProperties( HWND hWnd, HANDLE hPrinter )
{
    PRINTER_INFO_2W *pi2 = NULL;
    DWORD needed = 0;
    LONG Ret, Result = 0;
    BOOL res;
    DEVICEPROPERTYHEADER devprophdr;

    FIXME("PrinterProperties(%p, %p)\n", hWnd, hPrinter);

    devprophdr.cbSize         = sizeof(DEVICEPROPERTYHEADER);
    devprophdr.Flags          = DPS_NOPERMISSION;
    devprophdr.hPrinter       = hPrinter;
    devprophdr.pszPrinterName = NULL;

    res = GetPrinterW( hPrinter, 2, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        pi2 = HeapAlloc(hProcessHeap, 0, needed);
        res = GetPrinterW(hPrinter, 2, (LPBYTE)pi2, needed, &needed);
    }

    //
    // Above can fail, still process w/o printer name.
    //
    if ( res ) devprophdr.pszPrinterName = pi2->pPrinterName;

    needed = 1;

    if ( ( SetPrinterDataW( hPrinter, L"PrinterPropertiesPermission", REG_DWORD, (LPBYTE)&needed, sizeof(DWORD) ) == ERROR_SUCCESS ) )
    {
        devprophdr.Flags &= ~DPS_NOPERMISSION;
    }

    Ret = CallCommonPropertySheetUI( hWnd, (PFNPROPSHEETUI)DevicePropertySheets, (LPARAM)&devprophdr, (LPDWORD)&Result );

    res = (Ret >= 0);

    if (!res)
    {
        FIXME("PrinterProperties fail ICPSUI\n");
    }

    if (pi2) HeapFree(hProcessHeap, 0, pi2);

    return res;
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
    pHandle->bTrayIcon = pHandle->bJob = pHandle->bStartedDoc = FALSE;

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
    DWORD dwErrorCode;
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
        dwErrorCode = ERROR_INVALID_LEVEL;
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
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, Name, -1, pwszName, cch + 1);
    }

    /* Ref: https://stackoverflow.com/questions/41147180/why-enumprintersa-and-enumprintersw-request-the-same-amount-of-memory */
    if (!EnumPrintersW(Flags, pwszName, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned))
    {
        dwErrorCode = GetLastError();
        goto Cleanup;
    }

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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        ERR("HeapAlloc failed!\n");
                        goto Cleanup;
                    }

                    WideCharToMultiByte(CP_ACP, 0, ppi2w[i].pParameters, -1, pszParameters, cch + 1, NULL, NULL);
                    StringCchCopyA(ppi2a[i].pParameters, cch + 1, pszParameters);

                    HeapFree(hProcessHeap, 0, pszParameters);
                }
                if ( ppi2w[i].pDevMode )
                {
                    RosConvertUnicodeDevModeToAnsiDevmode( ppi2w[i].pDevMode, ppi2a[i].pDevMode );
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pwszName)
    {
        HeapFree(hProcessHeap, 0, pwszName);
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
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
    if ( !pszBuffer || cchInputBuffer < *pcchBuffer)
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
    DWORD dwErrorCode;
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
    PPRINTER_INFO_9A ppi9a = (PPRINTER_INFO_9A)pPrinter;
    PPRINTER_INFO_9W ppi9w = (PPRINTER_INFO_9W)pPrinter;
    DWORD cch;

    TRACE("GetPrinterA(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pPrinter, cbBuf, pcbNeeded);

    // Check for invalid levels here for early error return. Should be 1-9.
    if (Level <  1 || Level > 9)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        ERR("Invalid Level!\n");
        goto Cleanup;
    }

    if (!GetPrinterW(hPrinter, Level, pPrinter, cbBuf, pcbNeeded))
    {
        dwErrorCode = GetLastError();
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi2w->pParameters, -1, pszParameters, cch + 1, NULL, NULL);
                StringCchCopyA(ppi2a->pParameters, cch + 1, pszParameters);

                HeapFree(hProcessHeap, 0, pszParameters);
            }
            if ( ppi2w->pDevMode )
            {
                RosConvertUnicodeDevModeToAnsiDevmode( ppi2w->pDevMode, ppi2a->pDevMode );
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
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
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                    ERR("HeapAlloc failed!\n");
                    goto Cleanup;
                }

                WideCharToMultiByte(CP_ACP, 0, ppi7w->pszObjectGUID, -1, pszaObjectGUID, cch + 1, NULL, NULL);
                StringCchCopyA(ppi7a->pszObjectGUID, cch + 1, pszaObjectGUID);

                HeapFree(hProcessHeap, 0, pszaObjectGUID);
            }
        }
            break;
        case 8:
        case 9:
            RosConvertUnicodeDevModeToAnsiDevmode(ppi9w->pDevMode, ppi9a->pDevMode);
            break;
    }       // switch

    dwErrorCode = ERROR_SUCCESS;

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

    if ( bReturnValue )
    {
        PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)*phPrinter;
        pHandle->bAnsi = TRUE;
    }

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

        pHandle->Sig = SPOOLER_HANDLE_SIG;
        pHandle->hPrinter = hPrinter;
        pHandle->hSPLFile = INVALID_HANDLE_VALUE;
        pHandle->hSpoolFileHandle = INVALID_HANDLE_VALUE;

        // Return it as phPrinter.
        *phPrinter = (HANDLE)pHandle;
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

//
// Dead API.
//
DWORD WINAPI
PrinterMessageBoxA(HANDLE hPrinter, DWORD Error, HWND hWnd, LPSTR pText, LPSTR pCaption, DWORD dwType)
{
    return 50;
}

DWORD WINAPI
PrinterMessageBoxW(HANDLE hPrinter, DWORD Error, HWND hWnd, LPWSTR pText, LPWSTR pCaption, DWORD dwType)
{
    return 50;
}

BOOL WINAPI
QueryColorProfile(
  HANDLE    hPrinter,
  PDEVMODEW pdevmode,
  ULONG     ulQueryMode,
  VOID      *pvProfileData,
  ULONG     *pcbProfileData,
  FLONG     *pflProfileData )
{
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;
    BOOL Ret = FALSE;
    HMODULE hLibrary;

    FIXME("QueryColorProfile(%p, %p, %l, %p, %p, %p)\n", hPrinter, pdevmode, ulQueryMode, pvProfileData, pcbProfileData, pflProfileData);

    if ( pHandle->bNoColorProfile )
    {
        Ret = (BOOL)SP_ERROR;
    }
    else
    {

        if ( pdevmode )
        {
            if (!IsValidDevmodeNoSizeW( pdevmode ) )
            {
                ERR("DeviceCapabilitiesW : Devode Invalid");
                return FALSE;
            }
        }

        hLibrary = LoadPrinterDriver( hPrinter );

        if ( hLibrary )
        {
            fpQueryColorProfile = (PVOID)GetProcAddress( hLibrary, "DrvQueryColorProfile" );

            if ( fpQueryColorProfile )
            {
                Ret = fpQueryColorProfile( hPrinter, pdevmode, ulQueryMode, pvProfileData, pcbProfileData, pflProfileData );
            }
            else
            {
                pHandle->bNoColorProfile = TRUE;
                Ret = (BOOL)SP_ERROR;
            }

            FreeLibrary(hLibrary);
        }
    }
    return Ret;
}

// Note from GDI32:printdrv.c
//
//  QuerySpoolMode :
//    BOOL return TRUE if successful.
//    dlFont 0x0001 for Downloading fonts. 0x0002 unknown XPS_PASS?.
//    dwVersion is version of EMFSPOOL. Must be 0x00010000. See [MS-EMFSPOOL] page 18.
//

#define QSM_DOWNLOADINGFONTS 0x0001

/*
   Note from MSDN : "V4 print drivers using RAW mode to send PCL/Postscript have 0 byte spool file"

   Use XPS_PASS instead of RAW to pass information directly to the print filter pipeline in
   v4 and v3 XPSDrv drivers. Here's how to proceed with Windows 8:

    Call GetPrinterDriver to retrieve the DRIVER_INFO_8 structure.
    Check DRIVER_INFO_8::dwPrinterDriverAttributes for the PRINTER_DRIVER_XPS flag.
    Choose your datatype based on the presence or absence of the flag:
        If the flag is set, use XPS_PASS.
        If the flag isn't set, use RAW.
 */

#define QSM_XPS_PASS         0x0002 // Guessing. PRINTER_DRIVER_XPS?

BOOL WINAPI
QuerySpoolMode( HANDLE hPrinter, PDWORD downloadFontsFlags, PDWORD dwVersion )
{
    PRINTER_INFO_2W *pi2 = NULL;
    DWORD needed = 0;
    BOOL res;

    FIXME("QuerySpoolMode(%p, %p, %p)\n", hPrinter, downloadFontsFlags, dwVersion);

    res = GetPrinterW( hPrinter, 2, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        pi2 = HeapAlloc(hProcessHeap, 0, needed);
        res = GetPrinterW(hPrinter, 2, (LPBYTE)pi2, needed, &needed);
    }

    if ( res )
    {
        *dwVersion = 0x10000;
        *downloadFontsFlags = 0;

        if ( pi2->pServerName )
        {
            *downloadFontsFlags |= QSM_DOWNLOADINGFONTS;
        }
    }
//
//  Guessing,,,
//  To do : Add GetPrinterDriver for DRIVER_INFO_8, test PRINTER_DRIVER_XPS flag,
//          to set *downloadFontsFlags |= QSM_XPS_PASS;
//
//  Vista+ looks for QSM_XPS_PASS to be set in GDI32.
//
    HeapFree(hProcessHeap, 0, pi2);
    return res;
}

//
// This requires IC support.
//
DWORD WINAPI
QueryRemoteFonts( HANDLE hPrinter, PUNIVERSAL_FONT_ID pufi, ULONG NumberOfUFIs )
{
    HANDLE hIC;
    DWORD Result = -1, cOut, cIn = 0;
    PBYTE pOut;

    FIXME("QueryRemoteFonts(%p, %p, %lu)\n", hPrinter, pufi, NumberOfUFIs);

    hIC = CreatePrinterIC( hPrinter, NULL );
    if ( hIC )
    {
        cOut = (NumberOfUFIs * sizeof(UNIVERSAL_FONT_ID)) + sizeof(DWORD); // Include "DWORD" first part to return size.

        pOut = HeapAlloc( hProcessHeap, 0, cOut );
        if ( pOut )
        {
            if ( PlayGdiScriptOnPrinterIC( hIC, (LPBYTE)&cIn, sizeof(DWORD), pOut, cOut, 0 ) )
            {
                cIn = *((PDWORD)pOut); // Fisrt part is the size of the UFID object.

                Result = cIn; // Return the required size.

                if( NumberOfUFIs < cIn )
                {
                    cIn = NumberOfUFIs;
                }
                //     Copy whole object back to GDI32, exclude first DWORD part.
                memcpy( pufi, pOut + sizeof(DWORD), cIn * sizeof(UNIVERSAL_FONT_ID) );
            }
            HeapFree( hProcessHeap, 0, pOut );
        }
        DeletePrinterIC( hIC );
    }
    return Result;
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
    BOOL ret;
    UNICODE_STRING pNameW;
    PDEVMODEW pdmw = NULL;
    PPRINTER_DEFAULTSW pdw = (PPRINTER_DEFAULTSW)pDefault;

    TRACE("ResetPrinterA(%p, %p)\n", hPrinter, pDefault);

    if ( pDefault->pDatatype == (LPSTR)-1 )
    {
        pdw->pDatatype = (LPWSTR)-1;
    }
    else
    {
        pdw->pDatatype = AsciiToUnicode( &pNameW, pDefault->pDatatype );
    }
    if ( pDefault->pDevMode == (LPDEVMODEA)-1)
    {
        pdw->pDevMode = (LPDEVMODEW)-1;
    }
    else
    {
        if ( pDefault->pDevMode )//&& IsValidDevmodeNoSizeW( pDefault->pDevMode ) )
        {
            RosConvertAnsiDevModeToUnicodeDevmode( pDefault->pDevMode, &pdmw );
            pdw->pDevMode = pdmw;
        }
    }

    ret = ResetPrinterW( hPrinter, pdw );

    if (pdmw) HeapFree(hProcessHeap, 0, pdmw);

    RtlFreeUnicodeString( &pNameW );

    return ret;
}

BOOL WINAPI
ResetPrinterW(HANDLE hPrinter, PPRINTER_DEFAULTSW pDefault)
{
    TRACE("ResetPrinterW(%p, %p)\n", hPrinter, pDefault);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SeekPrinter( HANDLE hPrinter, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER pliNewPointer, DWORD dwMoveMethod, BOOL bWrite )
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    FIXME("SeekPrinter(%p, %I64u, %p, %lu, %d)\n", hPrinter, liDistanceToMove.QuadPart, pliNewPointer, dwMoveMethod, bWrite);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcSeekPrinter(pHandle->hPrinter, liDistanceToMove, pliNewPointer, dwMoveMethod, bWrite);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcSeekPrinter failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
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
    BOOL Ret = FALSE;
    UNICODE_STRING usBuffer;
    PPRINTER_INFO_STRESS ppisa = (PPRINTER_INFO_STRESS)pPrinter;
    PPRINTER_INFO_STRESS ppisw = (PPRINTER_INFO_STRESS)pPrinter;
    PPRINTER_INFO_2A ppi2a = (PPRINTER_INFO_2A)pPrinter;
    PPRINTER_INFO_2W ppi2w = (PPRINTER_INFO_2W)pPrinter;
    PPRINTER_INFO_7A ppi7a = (PPRINTER_INFO_7A)pPrinter;
    PPRINTER_INFO_7W ppi7w = (PPRINTER_INFO_7W)pPrinter;
    PPRINTER_INFO_9A ppi9a = (PPRINTER_INFO_9A)pPrinter;
    PPRINTER_INFO_9W ppi9w = (PPRINTER_INFO_9W)pPrinter;
    PWSTR pwszPrinterName = NULL;
    PWSTR pwszServerName = NULL;
    PWSTR pwszShareName = NULL;
    PWSTR pwszPortName = NULL;
    PWSTR pwszDriverName = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszLocation = NULL;
    PWSTR pwszSepFile = NULL;
    PWSTR pwszPrintProcessor = NULL;
    PWSTR pwszDatatype = NULL;
    PWSTR pwszParameters = NULL;
    PDEVMODEW pdmw = NULL;

    FIXME("SetPrinterA(%p, %lu, %p, %lu)\n", hPrinter, Level, pPrinter, Command);

    switch ( Level )
    {
        case 0:
            if ( Command == 0 )
            {
                if (ppisa->pPrinterName)
                {
                    pwszPrinterName = AsciiToUnicode(&usBuffer, (LPCSTR)ppisa->pPrinterName);
                    if (!(ppisw->pPrinterName = pwszPrinterName)) goto Cleanup;
                }
                if (ppisa->pServerName)
                {
                    pwszServerName = AsciiToUnicode(&usBuffer, (LPCSTR)ppisa->pServerName);
                    if (!(ppisw->pPrinterName = pwszServerName)) goto Cleanup;
                }
            }
            if ( Command == PRINTER_CONTROL_SET_STATUS )
            {
                // Set the pPrinter parameter to a pointer to a DWORD value that specifies the new printer status.
                PRINTER_INFO_6 pi6;
                pi6.dwStatus = (DWORD_PTR)pPrinter;
                pPrinter = (LPBYTE)&pi6;
                Level = 6;
                Command = 0;
            }
            break;
        case 2:
            {
                if (ppi2a->pShareName)
                {
                    pwszShareName = AsciiToUnicode(&usBuffer, ppi2a->pShareName);
                    if (!(ppi2w->pShareName = pwszShareName)) goto Cleanup;
                }
                if (ppi2a->pPortName)
                {
                    pwszPortName = AsciiToUnicode(&usBuffer, ppi2a->pPortName);
                    if (!(ppi2w->pPortName = pwszPortName)) goto Cleanup;
                }
                if (ppi2a->pDriverName)
                {
                    pwszDriverName = AsciiToUnicode(&usBuffer, ppi2a->pDriverName);
                    if (!(ppi2w->pDriverName = pwszDriverName)) goto Cleanup;
                }
                if (ppi2a->pComment)
                {
                    pwszComment = AsciiToUnicode(&usBuffer, ppi2a->pComment);
                    if (!(ppi2w->pComment = pwszComment)) goto Cleanup;
                }
                if (ppi2a->pLocation)
                {
                    pwszLocation = AsciiToUnicode(&usBuffer, ppi2a->pLocation);
                    if (!(ppi2w->pLocation = pwszLocation)) goto Cleanup;
                }
                if (ppi2a->pSepFile)
                {
                    pwszSepFile = AsciiToUnicode(&usBuffer, ppi2a->pSepFile);
                    if (!(ppi2w->pSepFile = pwszSepFile)) goto Cleanup;
                }
                if (ppi2a->pServerName)
                {
                    pwszPrintProcessor = AsciiToUnicode(&usBuffer, ppi2a->pPrintProcessor);
                    if (!(ppi2w->pPrintProcessor = pwszPrintProcessor)) goto Cleanup;
                }
                if (ppi2a->pDatatype)
                {
                    pwszDatatype = AsciiToUnicode(&usBuffer, ppi2a->pDatatype);
                    if (!(ppi2w->pDatatype = pwszDatatype)) goto Cleanup;
                }
                if (ppi2a->pParameters)
                {
                    pwszParameters = AsciiToUnicode(&usBuffer, ppi2a->pParameters);
                    if (!(ppi2w->pParameters = pwszParameters)) goto Cleanup;
                }

                if ( ppi2a->pDevMode )
                {
                    RosConvertAnsiDevModeToUnicodeDevmode( ppi2a->pDevMode, &pdmw );
                    ppi2w->pDevMode = pdmw;
                }
            }
        //
        //  These two strings are relitive and common to these three Levels.
        //  Fall through...
        //
        case 4:
        case 5:
            {
                if (ppi2a->pServerName) // 4 & 5 : pPrinterName.
                {
                    pwszServerName = AsciiToUnicode(&usBuffer, ppi2a->pServerName);
                    if (!(ppi2w->pPrinterName = pwszServerName)) goto Cleanup;
                }
                if (ppi2a->pPrinterName) // 4 : pServerName, 5 : pPortName.
                {
                    pwszPrinterName = AsciiToUnicode(&usBuffer, ppi2a->pPrinterName);
                    if (!(ppi2w->pPrinterName = pwszPrinterName)) goto Cleanup;
                }
            }
            break;
        case 3:
        case 6:
            break;
        case 7:
            {
                if (ppi7a->pszObjectGUID)
                {
                    pwszPrinterName = AsciiToUnicode(&usBuffer, ppi7a->pszObjectGUID);
                    if (!(ppi7w->pszObjectGUID = pwszPrinterName)) goto Cleanup;
                }
            }
            break;

        case 8:
        /* 8 is the global default printer info and 9 already sets it instead of the per-user one */
        /* still, PRINTER_INFO_8W is the same as PRINTER_INFO_9W */
        /* fall through */
        case 9:
            {
                RosConvertAnsiDevModeToUnicodeDevmode( ppi9a->pDevMode, &pdmw );
                ppi9w->pDevMode = pdmw;
            }
            break;

        default:
            FIXME( "Unsupported level %d\n", Level);
            SetLastError( ERROR_INVALID_LEVEL );
    }

    Ret = SetPrinterW( hPrinter, Level, pPrinter, Command );

Cleanup:
    if (pdmw) HeapFree(hProcessHeap, 0, pdmw);
    if (pwszPrinterName) HeapFree(hProcessHeap, 0, pwszPrinterName);
    if (pwszServerName) HeapFree(hProcessHeap, 0, pwszServerName);
    if (pwszShareName) HeapFree(hProcessHeap, 0, pwszShareName);
    if (pwszPortName) HeapFree(hProcessHeap, 0, pwszPortName);
    if (pwszDriverName) HeapFree(hProcessHeap, 0, pwszDriverName);
    if (pwszComment) HeapFree(hProcessHeap, 0, pwszComment);
    if (pwszLocation) HeapFree(hProcessHeap, 0, pwszLocation);
    if (pwszSepFile) HeapFree(hProcessHeap, 0, pwszSepFile);
    if (pwszPrintProcessor) HeapFree(hProcessHeap, 0, pwszPrintProcessor);
    if (pwszDatatype) HeapFree(hProcessHeap, 0, pwszDatatype);
    if (pwszParameters) HeapFree(hProcessHeap, 0, pwszParameters);
    return Ret;
}

BOOL WINAPI
SetPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pPrinter, DWORD Command)
{
    DWORD dwErrorCode;
    WINSPOOL_PRINTER_CONTAINER PrinterContainer;
    WINSPOOL_DEVMODE_CONTAINER DevModeContainer;
    WINSPOOL_SECURITY_CONTAINER SecurityContainer;
    SECURITY_DESCRIPTOR *sd = NULL;
    DWORD size;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    FIXME("SetPrinterW(%p, %lu, %p, %lu)\n", hPrinter, Level, pPrinter, Command);

    // Sanity checks
    if (!pHandle)
        return ERROR_INVALID_HANDLE;

    DevModeContainer.cbBuf = 0;
    DevModeContainer.pDevMode = NULL;

    SecurityContainer.cbBuf = 0;
    SecurityContainer.pSecurity = NULL;

    switch ( Level )
    {
        case 0:
            if ( Command == PRINTER_CONTROL_SET_STATUS )
            {
                // Set the pPrinter parameter to a pointer to a DWORD value that specifies the new printer status.
                PRINTER_INFO_6 pi6;
                pi6.dwStatus = (DWORD_PTR)pPrinter;
                pPrinter = (LPBYTE)&pi6;
                Level = 6;
                Command = 0;
            }
            break;
        case 2:
            {
                PPRINTER_INFO_2W pi2w = (PPRINTER_INFO_2W)pPrinter;
                if ( pi2w )
                {
                    if ( pi2w->pDevMode )
                    {
                         if ( IsValidDevmodeNoSizeW( pi2w->pDevMode ) )
                         {
                             DevModeContainer.cbBuf = pi2w->pDevMode->dmSize + pi2w->pDevMode->dmDriverExtra;
                             DevModeContainer.pDevMode = (PBYTE)pi2w->pDevMode;
                         }
                    }

                    if ( pi2w->pSecurityDescriptor )
                    {
                        sd = get_sd( pi2w->pSecurityDescriptor, &size );
                        if ( sd )
                        {
                            SecurityContainer.cbBuf = size;
                            SecurityContainer.pSecurity = (PBYTE)sd;
                        }
                    }
                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }
            }
            break;
        case 3:
            {
                PPRINTER_INFO_3 pi3 = (PPRINTER_INFO_3)pPrinter;
                if ( pi3 )
                {
                    if ( pi3->pSecurityDescriptor )
                    {
                        sd = get_sd( pi3->pSecurityDescriptor, &size );
                        if ( sd )
                        {
                            SecurityContainer.cbBuf = size;
                            SecurityContainer.pSecurity = (PBYTE)sd;
                        }
                    }
                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }
            }
            break;

        case 4:
        case 5:
        case 6:
        case 7:
            if ( pPrinter == NULL )
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            break;

        case 8:
        /* 8 is the global default printer info and 9 already sets it instead of the per-user one */
        /* still, PRINTER_INFO_8W is the same as PRINTER_INFO_9W */
        /* fall through */
        case 9:
            {
                PPRINTER_INFO_9W pi9w = (PPRINTER_INFO_9W)pPrinter;
                if ( pi9w )
                {
                    if ( pi9w->pDevMode )
                    {
                         if ( IsValidDevmodeNoSizeW( pi9w->pDevMode ) )
                         {
                             DevModeContainer.cbBuf = pi9w->pDevMode->dmSize + pi9w->pDevMode->dmDriverExtra;
                             DevModeContainer.pDevMode = (LPBYTE)pi9w->pDevMode;
                         }
                    }
                }
            }
            break;

        default:
            FIXME( "Unsupported level %d\n", Level );
            SetLastError( ERROR_INVALID_LEVEL );
            return FALSE;
    }

    PrinterContainer.PrinterInfo.pPrinterInfo1 = (WINSPOOL_PRINTER_INFO_1*)pPrinter;
    PrinterContainer.Level = Level;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcSetPrinter(pHandle->hPrinter, &PrinterContainer, &DevModeContainer, &SecurityContainer, Command);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    if ( sd ) HeapFree( GetProcessHeap(), 0, sd );

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SplDriverUnloadComplete(LPWSTR pDriverFile)
{
    TRACE("DriverUnloadComplete(%S)\n", pDriverFile);
    UNIMPLEMENTED;
    return TRUE; // return true for now.
}
BOOL WINAPI

SpoolerPrinterEvent( LPWSTR pPrinterName, INT DriverEvent, DWORD Flags, LPARAM lParam )
{
    HMODULE hLibrary;
    HANDLE hPrinter;
    BOOL Ret = FALSE;

    if ( OpenPrinterW( pPrinterName, &hPrinter, NULL ) )
    {
        hLibrary = LoadPrinterDriver( hPrinter );

        if ( hLibrary )
        {
            fpPrinterEvent = (PVOID)GetProcAddress( hLibrary, "DrvPrinterEvent" );

            if ( fpPrinterEvent )
            {
                Ret = fpPrinterEvent( pPrinterName, DriverEvent, Flags, lParam );
            }

            FreeLibrary(hLibrary);
        }

        ClosePrinter( hPrinter );
    }

    return Ret;
}

INT_PTR CALLBACK file_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LPWSTR filename;

    switch(msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtrW(hwnd, DWLP_USER, lparam);
        return TRUE;

    case WM_COMMAND:
        if(HIWORD(wparam) == BN_CLICKED)
        {
            if(LOWORD(wparam) == IDOK)
            {
                HANDLE hf;
                DWORD len = SendDlgItemMessageW(hwnd, EDITBOX, WM_GETTEXTLENGTH, 0, 0);
                LPWSTR *output;

                filename = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
                GetDlgItemTextW(hwnd, EDITBOX, filename, len + 1);

                if(GetFileAttributesW(filename) != INVALID_FILE_ATTRIBUTES)
                {
                    WCHAR caption[200], message[200];
                    int mb_ret;

                    LoadStringW(hinstWinSpool, IDS_CAPTION, caption, ARRAYSIZE(caption));
                    LoadStringW(hinstWinSpool, IDS_FILE_EXISTS, message, ARRAYSIZE(message));
                    mb_ret = MessageBoxW(hwnd, message, caption, MB_OKCANCEL | MB_ICONEXCLAMATION);
                    if(mb_ret == IDCANCEL)
                    {
                        HeapFree(GetProcessHeap(), 0, filename);
                        return TRUE;
                    }
                }
                hf = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if(hf == INVALID_HANDLE_VALUE)
                {
                    WCHAR caption[200], message[200];

                    LoadStringW(hinstWinSpool, IDS_CAPTION, caption, ARRAYSIZE(caption));
                    LoadStringW(hinstWinSpool, IDS_CANNOT_OPEN, message, ARRAYSIZE(message));
                    MessageBoxW(hwnd, message, caption, MB_OK | MB_ICONEXCLAMATION);
                    HeapFree(GetProcessHeap(), 0, filename);
                    return TRUE;
                }
                CloseHandle(hf);
                DeleteFileW(filename);
                output = (LPWSTR *)GetWindowLongPtrW(hwnd, DWLP_USER);
                *output = filename;
                EndDialog(hwnd, IDOK);
                return TRUE;
            }
            if(LOWORD(wparam) == IDCANCEL)
            {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
        }
        return FALSE;
    }
    return FALSE;
}

static const WCHAR FILE_Port[] = {'F','I','L','E',':',0};

LPWSTR WINAPI
StartDocDlgW( HANDLE hPrinter, DOCINFOW *doc )
{
    LPWSTR ret = NULL;
    DWORD len, attr, retDlg;

    FIXME("StartDocDlgW(%p, %p)\n", hPrinter, doc);

    if (doc->lpszOutput == NULL) /* Check whether default port is FILE: */
    {
        PRINTER_INFO_5W *pi5;
        GetPrinterW(hPrinter, 5, NULL, 0, &len);
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
        pi5 = HeapAlloc(GetProcessHeap(), 0, len);
        GetPrinterW(hPrinter, 5, (LPBYTE)pi5, len, &len);
        if (!pi5->pPortName || wcsicmp(pi5->pPortName, FILE_Port))
        {
            HeapFree(GetProcessHeap(), 0, pi5);
            return NULL;
        }
        HeapFree(GetProcessHeap(), 0, pi5);
    }

    if (doc->lpszOutput == NULL || !wcsicmp(doc->lpszOutput, FILE_Port))
    {
        LPWSTR name;

        retDlg = DialogBoxParamW( hinstWinSpool,
                                  MAKEINTRESOURCEW(FILENAME_DIALOG),
                                  GetForegroundWindow(),
                                  file_dlg_proc,
                                 (LPARAM)&name );

        if ( retDlg == IDOK )
        {
            if (!(len = GetFullPathNameW(name, 0, NULL, NULL)))
            {
                HeapFree(GetProcessHeap(), 0, name);
                return NULL;
            }
            ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            GetFullPathNameW(name, len, ret, NULL);
            HeapFree(GetProcessHeap(), 0, name);
        }
        else if ( retDlg == 0 ) // FALSE, some type of error occurred.
        {
            ret = (LPWSTR)SP_ERROR;
        }
        else if ( retDlg == IDCANCEL )
        {
            SetLastError( ERROR_CANCELLED );
            ret = (LPWSTR)SP_APPABORT;
        }
        return ret;
    }

    if (!(len = GetFullPathNameW(doc->lpszOutput, 0, NULL, NULL)))
        return NULL;

    ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    GetFullPathNameW(doc->lpszOutput, len, ret, NULL);

    attr = GetFileAttributesW(ret);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        HeapFree(GetProcessHeap(), 0, ret);
        ret = NULL;
    }
    return ret;
}

LPSTR WINAPI
StartDocDlgA( HANDLE hPrinter, DOCINFOA *doc )
{
    UNICODE_STRING usBuffer;
    DOCINFOW docW = { 0 };
    LPWSTR retW;
    LPWSTR docnameW = NULL, outputW = NULL, datatypeW = NULL;
    LPSTR ret = NULL;

    docW.cbSize = sizeof(docW);
    if (doc->lpszDocName)
    {
        docnameW = AsciiToUnicode(&usBuffer, doc->lpszDocName);
        if (!(docW.lpszDocName = docnameW)) goto failed;
    }
    if (doc->lpszOutput)
    {
        outputW = AsciiToUnicode(&usBuffer, doc->lpszOutput);
        if (!(docW.lpszOutput = outputW)) goto failed;
    }
    if (doc->lpszDatatype)
    {
        datatypeW = AsciiToUnicode(&usBuffer, doc->lpszDatatype);
        if (!(docW.lpszDatatype = datatypeW)) goto failed;
    }
    docW.fwType = doc->fwType;

    retW = StartDocDlgW(hPrinter, &docW);

    if (retW)
    {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, retW, -1, NULL, 0, NULL, NULL);
        ret = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_ACP, 0, retW, -1, ret, len, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, retW);
    }

failed:
    if (datatypeW) HeapFree(GetProcessHeap(), 0, datatypeW);
    if (outputW) HeapFree(GetProcessHeap(), 0, outputW);
    if (docnameW) HeapFree(GetProcessHeap(), 0, docnameW);

    return ret;
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
        ERR("Level = %d, unsupported!\n", Level);
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
        ERR("Level = %d, unsupported!\n", Level);
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
        if ( !pHandle->bTrayIcon )
        {
            UpdateTrayIcon( hPrinter, pHandle->dwJobID );
        }
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
    DWORD dwErrorCode, Bogus = 0;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hXcv;

    TRACE("XcvDataW(%p, %S, %p, %lu, %p, %lu, %p, %p)\n", hXcv, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded, pdwStatus);

    if ( pcbOutputNeeded == NULL )
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Sanity checks.
    if (!pHandle) // ( IntProtectHandle( hXcv, FALSE ) )
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    //
    // Do fixups.
    //
    if ( pInputData == NULL )
    {
        if ( !cbInputData )
        {
             pInputData = (PBYTE)&Bogus;
        }
    }

    if ( pOutputData == NULL )
    {
        if ( !cbOutputData )
        {
            pOutputData = (PBYTE)&Bogus;
        }
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcXcvData( pHandle->hPrinter,
                                   pszDataName,
                                   pInputData,
                                   cbInputData,
                                   pOutputData,
                                   cbOutputData,
                                   pcbOutputNeeded,
                                   pdwStatus );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcXcvData failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    //IntUnprotectHandle( hXcv );

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
