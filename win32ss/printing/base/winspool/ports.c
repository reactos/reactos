/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Ports
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/ports.h>

typedef struct _MONITORUIDATA
{
    HMODULE hLibrary;
    HANDLE hActCtx;
    ULONG_PTR ulpCookie;
    PWSTR pModuleName;
    BOOL Activeated;
} MONITORUIDATA, *PMONITORUIDATA;

typedef DWORD (*PPfpFunction)(LPWSTR, ULONG_PTR, LPWSTR);

typedef struct _PORTTHREADINFO
{
    LPWSTR pName;
    ULONG_PTR hWnd;
    LPWSTR pPortName;
    PPfpFunction fpFunction;
    DWORD dwErrorCode;
    HANDLE hEvent;
} PORTTHREADINFO, *PPORTTHREADINFO;

VOID WINAPI
IntPortThread( PPORTTHREADINFO pPortThreadInfo )
{
    FIXME("IPT : %s\n",debugstr_w( pPortThreadInfo->pPortName ));
    // Do the RPC call
    RpcTryExcept
    {
        pPortThreadInfo->dwErrorCode = pPortThreadInfo->fpFunction( pPortThreadInfo->pName, pPortThreadInfo->hWnd, pPortThreadInfo->pPortName );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        pPortThreadInfo->dwErrorCode = RpcExceptionCode();
        ERR("IPT : _RpcXyzPort failed with exception code %lu!\n", pPortThreadInfo->dwErrorCode);
    }
    RpcEndExcept;

    SetEvent( pPortThreadInfo->hEvent );
}

//
// Start a thread to wait on a printer port.
//
BOOL WINAPI
StartPortThread( LPWSTR pName, HWND hWnd, LPWSTR pPortName, PPfpFunction fpFunction )
{
    PORTTHREADINFO PortThreadInfo;
    HANDLE htHandle;
    MSG Msg;
    DWORD tid;

    if ( hWnd ) EnableWindow( hWnd, FALSE );

    PortThreadInfo.pName = pName;
    PortThreadInfo.hWnd = (ULONG_PTR)hWnd;
    PortThreadInfo.pPortName = pPortName;
    PortThreadInfo.fpFunction = fpFunction;
    PortThreadInfo.dwErrorCode = ERROR_SUCCESS;
    PortThreadInfo.hEvent = CreateEventW( NULL, TRUE, FALSE, NULL );

    htHandle = CreateThread( NULL,
                             32*1024,
                            (LPTHREAD_START_ROUTINE)IntPortThread,
                            &PortThreadInfo,
                             0,
                            &tid );

    CloseHandle( htHandle );

    while ( MsgWaitForMultipleObjects( 1, &PortThreadInfo.hEvent, FALSE, INFINITE, QS_ALLINPUT ) == 1 )
    {
        while ( PeekMessageW( &Msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &Msg );
            DispatchMessageW( &Msg );
        }
    }

    CloseHandle( PortThreadInfo.hEvent );

    if ( hWnd )
    {
        EnableWindow(hWnd, TRUE);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
    }

    SetLastError(PortThreadInfo.dwErrorCode);
    return (PortThreadInfo.dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetMonitorUIFullName( PWSTR pDeviceName, PWSTR *pModuleName )
{
    STRSAFE_LPWSTR SysDir;
    UINT length;
    HRESULT hr;

   *pModuleName = NULL;

    SysDir = HeapAlloc(hProcessHeap, 0, MAX_PATH*sizeof(WCHAR));

    if ( SysDir )
    {
        memset( SysDir, 0, MAX_PATH*sizeof(WCHAR) );

        length = GetSystemDirectoryW( SysDir, MAX_PATH*sizeof(WCHAR) );

        if ( length > 0 )
        {
            StringCbCatW(SysDir, MAX_PATH*sizeof(WCHAR), L"\\");

            hr = StringCchCatW( SysDir, MAX_PATH*sizeof(WCHAR), pDeviceName );
            if ( !FAILED(hr) )
            {
                *pModuleName = SysDir;
                return TRUE;
            }
            SetLastError(HRESULT_CODE(hr));
        }

        HeapFree(hProcessHeap, 0, SysDir);
    }
    return FALSE;
}

BOOL WINAPI
GetMonitorUIActivationContext( PWSTR pDeviceName, PMONITORUIDATA pmuid )
{
   // ACTCTXW actctx;
   // HANDLE handle;
    BOOL Ret = FALSE;

    FIXME("GMUIAC : Module pDeviceName %S\n",pDeviceName);

    if ( !GetMonitorUIFullName( pDeviceName, &pmuid->pModuleName ) )
    {
        ERR("GetMonitorUIFullName Failed\n");
        return Ret;
    }
/*    OMG! SxS again?
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    actctx.lpResourceName = MAKEINTRESOURCEW(123); This might be the reason....
    actctx.lpSource = pmuid->pModuleName;

    handle = CreateActCtxW(&actctx);

    if ( handle != INVALID_HANDLE_VALUE )
    {
        pmuid->hActCtx = handle;
        if ( ActivateActCtx( handle, &pmuid->ulpCookie ) )
        {
            pmuid->Activeated = TRUE;
            Ret = TRUE;
        }
        else
        {
            pmuid->Activeated = FALSE;
        }
    }
    else
    {
        ERR("GetMonitorUIActivationContext Failed %S\n",pmuid->pModuleName);
    }*/
    pmuid->hActCtx = INVALID_HANDLE_VALUE;
    Ret = TRUE;
    return Ret;
}

VOID FASTCALL
FreeMonitorUI( PMONITORUIDATA pmuid )
{
    if ( pmuid )
    {
        if ( pmuid->hLibrary )
        {
            FreeLibrary( pmuid->hLibrary );
        }
        if ( pmuid->Activeated )
        {
            DeactivateActCtx( 0, pmuid->ulpCookie );
        }
        if ( pmuid->hActCtx != INVALID_HANDLE_VALUE )
        {
            ReleaseActCtx( pmuid->hActCtx );
        }
        if ( pmuid->pModuleName )
        {
            DllFreeSplMem( pmuid->pModuleName );
        }
        DllFreeSplMem( pmuid );
    }
}

BOOL FASTCALL
StrNCatBuff( PWSTR ptr, size_t Size, PWSTR args, ...)
{
    va_list Args;
    PWSTR pwstr;
    HRESULT hr;
    BOOL Ret = TRUE;

    va_start(Args, args );

   for ( pwstr = args ; pwstr ; pwstr = va_arg( Args, PWSTR ) )
   {
       hr = StringCchCatNW( ptr, Size, pwstr, wcslen(pwstr) );
       if ( FAILED(hr) )
       {
           SetLastError(HRESULT_CODE(hr));
           Ret = FALSE;
           break;
        }
    }

    va_end(Args);

    return Ret;
}

PWSTR WINAPI
ConstructXcvName( PWSTR pName, PWSTR pMonitorPortName, PWSTR pXcvName )
{
    BOOL Ret = FALSE;
    PWSTR pwstr = NULL;
    size_t sXcv, smpn = 0, Size = 0;

    if ( pName )
    {
        Size = wcslen( pName ) + 1;
    }

    sXcv = wcslen( pXcvName ) + Size;

    if ( pMonitorPortName )
    {
        smpn = wcslen( pMonitorPortName );
    }

    Size = sXcv + smpn + 3;

    pwstr = DllAllocSplMem( Size * sizeof(WCHAR) );

    memset( pwstr, 0, Size );

    if ( pwstr )
    {
        // The caller wants an Xcv handle and provided a string like:
        //    ", XcvMonitor Local Port"
        //    "\\COMPUTERNAME\, XcvMonitor Local Port"
        //    ", XcvPort LPT1:"
        //    "\\COMPUTERNAME\, XcvPort LPT1:"
        //
        //    This produces; !pName ",XcvMonitor " or pName "\\COMPUTERNAME\XcvMonitor "
        //
        Ret = StrNCatBuff( pwstr,
                           Size,
                           pName ? pName : L"",
                           pName ? L"\\" : L",",
                           pXcvName,
                           L" ",
                           pMonitorPortName ? pMonitorPortName : L"",
                           NULL );
    }

    if ( !Ret )
    {
        DllFreeSplMem( pwstr );
        pwstr = NULL;
    }

    return pwstr;
}

DWORD WINAPI
GetMonitorUI( PWSTR pName, PWSTR pMonitorPortName, PWSTR pXcvName, PMONITORUI *pmui, PMONITORUIDATA *ppmuid )
{
    DWORD dwErrorCode = ERROR_SUCCESS, cbOutputNeeded, dwStatus;
    HANDLE hPrinter = NULL;
    HMODULE hModule;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;
    PWSTR pDevice = NULL, pOutputString = NULL;
    PMONITORUIDATA pmuid = NULL;
    PRINTER_DEFAULTSW wDefault = { 0, 0, PRINTER_ATTRIBUTE_QUEUED };
    BYTE OutputData[1024], InputData[4];

    *pmui = NULL;
    *ppmuid = NULL;

    pDevice = ConstructXcvName( pName, pMonitorPortName, pXcvName );

    if ( !pDevice )
    {
        return GetLastError();
    }

    FIXME("GMUI : XcvName : %S\n",pDevice);

    if ( OpenPrinterW( (LPWSTR)pDevice, &hPrinter, &wDefault ) )
    {
        pHandle = (PSPOOLER_HANDLE)hPrinter;

        // Do the RPC call
        RpcTryExcept
        {
            dwErrorCode = _RpcXcvData( pHandle->hPrinter,
                                       L"MonitorUI",
                                       (PBYTE)&InputData,
                                       0,
                                       (PBYTE)&OutputData,
                                       1024,
                                       &cbOutputNeeded,
                                       &dwStatus );
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErrorCode = RpcExceptionCode();
            ERR("GMUI : _RpcXcvData failed with exception code %lu!\n", dwErrorCode);
        }
        RpcEndExcept;

        if ( dwErrorCode == ERROR_INSUFFICIENT_BUFFER )
        {
            pOutputString = DllAllocSplMem( cbOutputNeeded );

            // Do the RPC call
            RpcTryExcept
            {
                dwErrorCode = _RpcXcvData( pHandle->hPrinter,
                                           L"MonitorUI",
                                           (PBYTE)&InputData,
                                           0,
                                           (PBYTE)pOutputString,
                                           cbOutputNeeded,
                                           &cbOutputNeeded,
                                           &dwStatus );
            }
            RpcExcept(EXCEPTION_EXECUTE_HANDLER)
            {
                dwErrorCode = RpcExceptionCode();
                ERR("GMUI : _RpcXcvData failed with exception code %lu!\n", dwErrorCode);
            }
            RpcEndExcept;
        }

        if ( dwErrorCode != ERROR_SUCCESS || dwStatus != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        pmuid = DllAllocSplMem( sizeof(MONITORUIDATA) );
        if ( pmuid )
        {
            memset( pmuid, 0, sizeof(MONITORUIDATA) );
            pmuid->hActCtx = INVALID_HANDLE_VALUE;
        }
        else
        {
            ERR("GMUI : Memory error\n");
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        if ( GetMonitorUIActivationContext( pOutputString ? pOutputString : (PWSTR)&OutputData, pmuid ) )
        {
            FIXME("GMUI : MonitorUI Path : %S\n",pmuid->pModuleName);

            hModule = LoadLibraryW( pmuid->pModuleName );
            if ( hModule )
            {
                FARPROC fpInitializePrintMonitorUI = (PVOID) GetProcAddress( hModule, "InitializePrintMonitorUI" );
                if ( fpInitializePrintMonitorUI )
                {
                    pmuid->hLibrary = hModule;
                    *pmui = (PMONITORUI)(*fpInitializePrintMonitorUI)();
                    *ppmuid = pmuid;
                }
                else
                {
                   ERR("GMUI : Failed to get MUI %S\n",pmuid->pModuleName);
                   FreeMonitorUI( pmuid );
                }
            }
            else
            {
                ERR("GMUI : Failed to load library %S\n",pmuid->pModuleName);
            }
        }
    }
    else
    {
        ERR("GMUI : Failed to open printer handle\n");
    }

    dwErrorCode = GetLastError();

Cleanup:
    if ( hPrinter ) ClosePrinter( hPrinter );
    if ( pOutputString ) DllFreeSplMem( pOutputString );
    if ( pDevice ) DllFreeSplMem( pDevice );

    FIXME("GMUI : Error Code Exit %d\n",dwErrorCode);

    return dwErrorCode;
}

BOOL WINAPI
AddPortA(PSTR pName, HWND hWnd, PSTR pMonitorName)
{
    LPWSTR  nameW = NULL;
    LPWSTR  monitorW = NULL;
    DWORD   len;
    BOOL    res;

    TRACE("AddPortA(%s, %p, %s)\n",debugstr_a(pName), hWnd, debugstr_a(pMonitorName));

    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    if (pMonitorName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, NULL, 0);
        monitorW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, monitorW, len);
    }

    res = AddPortW(nameW, hWnd, monitorW);

    if (nameW) HeapFree(GetProcessHeap(), 0, nameW);
    if (monitorW) HeapFree(GetProcessHeap(), 0, monitorW);

    return res;
}

BOOL WINAPI
AddPortExW(PWSTR pName, DWORD Level, PBYTE lpBuffer, PWSTR lpMonitorName)
{
    DWORD dwErrorCode;
    WINSPOOL_PORT_CONTAINER PortInfoContainer;
    WINSPOOL_PORT_VAR_CONTAINER PortVarContainer;
    WINSPOOL_PORT_INFO_FF *pPortInfoFF;

    FIXME("AddPortExW(%S, %lu, %p, %S)\n", pName, Level, lpBuffer, lpMonitorName);

    switch (Level)
    {
        case 1:
           // FIXME!!!! Only Level 1 is supported? See note in wine winspool test info.c : line 575. It's just not supported here.
           PortInfoContainer.PortInfo.pPortInfo1 = (WINSPOOL_PORT_INFO_1*)lpBuffer;
           PortInfoContainer.Level = Level;
           PortVarContainer.cbMonitorData = 0;
           PortVarContainer.pMonitorData = NULL;
           break;

        case 0xFFFFFFFF:
           pPortInfoFF = (WINSPOOL_PORT_INFO_FF*)lpBuffer;
           PortInfoContainer.PortInfo.pPortInfoFF = pPortInfoFF;
           PortInfoContainer.Level = Level;
           PortVarContainer.cbMonitorData = pPortInfoFF->cbMonitorData;
           PortVarContainer.pMonitorData = pPortInfoFF->pMonitorData;
           break;

        default:
           ERR("Level = %d, unsupported!\n", Level);
           SetLastError(ERROR_INVALID_LEVEL);
           return FALSE;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcAddPortEx(pName, &PortInfoContainer, &PortVarContainer, lpMonitorName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
AddPortExA(PSTR pName, DWORD Level, PBYTE lpBuffer, PSTR lpMonitorName)
{
    PORT_INFO_1W   pi1W;
    PORT_INFO_1A * pi1A;
    LPWSTR  nameW = NULL;
    LPWSTR  monitorW = NULL;
    DWORD   len;
    BOOL    res = FALSE;
    WINSPOOL_PORT_INFO_FF *pPortInfoFF, PortInfoFF;

    pi1A = (PORT_INFO_1A *)lpBuffer;
    pPortInfoFF = (WINSPOOL_PORT_INFO_FF*)lpBuffer;

    FIXME("AddPortExA(%s, %d, %p, %s): %s\n", debugstr_a(pName), Level, lpBuffer, debugstr_a(lpMonitorName), debugstr_a(pi1A ? pi1A->pName : NULL));

    if ( !lpBuffer || !lpMonitorName )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    if (lpMonitorName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpMonitorName, -1, NULL, 0);
        monitorW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpMonitorName, -1, monitorW, len);
    }

    pi1W.pName = NULL;
    ZeroMemory( &PortInfoFF, sizeof(WINSPOOL_PORT_INFO_FF));

    switch ( Level )
    {
        case 1:
            if ( pi1A->pName )
            {
                len = MultiByteToWideChar(CP_ACP, 0, pi1A->pName, -1, NULL, 0);
                pi1W.pName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, pi1A->pName, -1, pi1W.pName, len);
            }
            break;

        case 0xFFFFFFFF:
            //
            // Remember the calling parameter is Ansi.
            //
            if ( !pPortInfoFF->pPortName || !(PCHAR)pPortInfoFF->pPortName )
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Cleanup;
            }

            len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pPortInfoFF->pPortName, -1, NULL, 0);
            PortInfoFF.pPortName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pPortInfoFF->pPortName, -1, (LPWSTR)PortInfoFF.pPortName, len);

            PortInfoFF.cbMonitorData = pPortInfoFF->cbMonitorData;
            PortInfoFF.pMonitorData  = pPortInfoFF->pMonitorData;
            break;

        default:
            ERR("Level = %d, unsupported!\n", Level);
            SetLastError(ERROR_INVALID_LEVEL);
            goto Cleanup;
    }

    res = AddPortExW( nameW, Level,  Level == 1 ? (PBYTE)&pi1W : (PBYTE)&PortInfoFF, monitorW );

Cleanup:
    if (nameW) HeapFree(GetProcessHeap(), 0, nameW);
    if (monitorW) HeapFree(GetProcessHeap(), 0, monitorW);
    if (pi1W.pName) HeapFree(GetProcessHeap(), 0, pi1W.pName);
    if (PortInfoFF.pPortName) HeapFree(GetProcessHeap(), 0, PortInfoFF.pPortName);

    return res;
}

BOOL WINAPI
AddPortW(PWSTR pName, HWND hWnd, PWSTR pMonitorName)
{
    DWORD SessionId, dwErrorCode = 0;
    PMONITORUIDATA pmuid;
    PMONITORUI pmui = NULL;
    BOOL Ret = FALSE;

    FIXME("AddPortW(%S, %p, %S)\n", pName, hWnd, pMonitorName);

    dwErrorCode = GetMonitorUI( pName, pMonitorName, L"XcvMonitor", &pmui, &pmuid );
    FIXME("AddPortW Error %d\n",dwErrorCode);
    if (dwErrorCode != ERROR_SUCCESS )
    {
        if ( dwErrorCode == ERROR_NOT_SUPPORTED          ||
             dwErrorCode == ERROR_MOD_NOT_FOUND          ||
             dwErrorCode == ERROR_INVALID_PRINT_MONITOR  ||
             dwErrorCode == ERROR_UNKNOWN_PORT           ||
             dwErrorCode == ERROR_INVALID_PRINTER_NAME )
        {
            if ( ProcessIdToSessionId( GetCurrentProcessId(), &SessionId ) && SessionId ) // Looking if this is remote.
            {
                dwErrorCode = ERROR_NOT_SUPPORTED;
            }
            else
            {
                Ret = StartPortThread( pName, hWnd, pMonitorName, (PPfpFunction)_RpcAddPort );
                FIXME("AddPortW return StartPortThread\n");
                dwErrorCode = GetLastError();
            }
        }
    }
    else
    {
        Ret = (*pmui->pfnAddPortUI)( pName, hWnd, pMonitorName, NULL );
    }

    SetLastError(dwErrorCode);
    FreeMonitorUI( pmuid );

    return Ret;
}

BOOL WINAPI
ConfigurePortA(PSTR pName, HWND hWnd, PSTR pPortName)
{
    LPWSTR  nameW = NULL;
    LPWSTR  portW = NULL;
    INT     len;
    DWORD   res;

    TRACE("ConfigurePortA(%s, %p, %s)\n", debugstr_a(pName), hWnd, debugstr_a(pPortName));

    /* convert servername to unicode */
    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* convert portname to unicode */
    if (pPortName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pPortName, -1, NULL, 0);
        portW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pPortName, -1, portW, len);
    }

    res = ConfigurePortW(nameW, hWnd, portW);

    if (nameW) HeapFree(GetProcessHeap(), 0, nameW);
    if (portW) HeapFree(GetProcessHeap(), 0, portW);

    return res;
}

BOOL WINAPI
ConfigurePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    DWORD SessionId, dwErrorCode = 0;
    PMONITORUIDATA pmuid;
    PMONITORUI pmui = NULL;
    BOOL Ret = FALSE;

    FIXME("ConfigurePortW(%S, %p, %S)\n", pName, hWnd, pPortName);

    dwErrorCode = GetMonitorUI( pName, pPortName, L"XcvPort", &pmui, &pmuid );

    if (dwErrorCode != ERROR_SUCCESS )
    {
        if ( dwErrorCode == ERROR_NOT_SUPPORTED          ||
             dwErrorCode == ERROR_MOD_NOT_FOUND          ||
             dwErrorCode == ERROR_INVALID_PRINT_MONITOR  ||
             dwErrorCode == ERROR_UNKNOWN_PORT           ||
             dwErrorCode == ERROR_INVALID_PRINTER_NAME )
        {
            if ( ProcessIdToSessionId( GetCurrentProcessId(), &SessionId ) && SessionId ) // Looking if this is remote.
            {
                dwErrorCode = ERROR_NOT_SUPPORTED;
            }
            else
            {
                Ret = StartPortThread(pName, hWnd, pPortName, (PPfpFunction)_RpcConfigurePort );
                dwErrorCode = GetLastError();
            }
        }
    }
    else
    {
        Ret = (*pmui->pfnConfigurePortUI)( pName, hWnd, pPortName );
    }

    SetLastError(dwErrorCode);
    FreeMonitorUI( pmuid );

    return Ret;
}

BOOL WINAPI
DeletePortA(PSTR pName, HWND hWnd, PSTR pPortName)
{
    LPWSTR  nameW = NULL;
    LPWSTR  portW = NULL;
    INT     len;
    DWORD   res;

    FIXME("DeletePortA(%s, %p, %s)\n", debugstr_a(pName), hWnd, debugstr_a(pPortName));

    /* convert servername to unicode */
    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* convert portname to unicode */
    if (pPortName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pPortName, -1, NULL, 0);
        portW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pPortName, -1, portW, len);
    }

    res = DeletePortW(nameW, hWnd, portW);

    if (nameW) HeapFree(GetProcessHeap(), 0, nameW);
    if (portW) HeapFree(GetProcessHeap(), 0, portW);

    return res;
}

BOOL WINAPI
DeletePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    DWORD dwErrorCode = 0;
    PMONITORUIDATA pmuid;
    PMONITORUI pmui = NULL;
    BOOL Ret = FALSE;

    FIXME("DeletePortW(%S, %p, %S)\n", pName, hWnd, pPortName);

    dwErrorCode = GetMonitorUI( pName, pPortName, L"XcvPort", &pmui, &pmuid );
    FIXME("DeletePortW Error %d\n",dwErrorCode);
    if (dwErrorCode != ERROR_SUCCESS )
    {
        if ( dwErrorCode == ERROR_NOT_SUPPORTED          ||
             dwErrorCode == ERROR_MOD_NOT_FOUND          ||
             dwErrorCode == ERROR_INVALID_PRINT_MONITOR  ||
             dwErrorCode == ERROR_UNKNOWN_PORT           ||
             dwErrorCode == ERROR_INVALID_PRINTER_NAME )
        {
            Ret = StartPortThread(pName, hWnd, pPortName, (PPfpFunction)_RpcDeletePort );
            dwErrorCode = GetLastError();
        }
    }
    else
    {
        Ret = (*pmui->pfnDeletePortUI)( pName, hWnd, pPortName );
    }

    SetLastError(dwErrorCode);
    FreeMonitorUI( pmuid );

    return Ret;
}

BOOL WINAPI
EnumPortsA(PSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL    res;
    LPBYTE  bufferW = NULL;
    LPWSTR  nameW = NULL;
    DWORD   needed = 0;
    DWORD   numentries = 0;
    INT     len;

    TRACE("EnumPortsA(%s, %d, %p, %d, %p, %p)\n", debugstr_a(pName), Level, pPorts, cbBuf, pcbNeeded, pcReturned);

    if ((Level < 1) || (Level > 2))
    {
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* convert servername to unicode */
    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the Ports */
    needed = cbBuf * sizeof(WCHAR);
    if (needed) bufferW = HeapAlloc(GetProcessHeap(), 0, needed);
    res = EnumPortsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? HeapReAlloc(GetProcessHeap(), 0, bufferW, needed) :
                              HeapAlloc(GetProcessHeap(), 0, needed);

        /* Try again with the large Buffer */
        res = EnumPortsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);
    }
    needed = pcbNeeded ? *pcbNeeded : 0;
    numentries = pcReturned ? *pcReturned : 0;

    /*
       W2k require the buffersize from EnumPortsW also for EnumPortsA.
       We use the smaller Ansi-Size to avoid conflicts with fixed Buffers of old Apps.
     */
    if (res)
    {
        /* EnumPortsW collected all Data. Parse them to calculate ANSI-Size */
        DWORD   entrysize = 0;
        DWORD   index;
        LPSTR   ptr;
        LPPORT_INFO_2W pi2w;
        LPPORT_INFO_2A pi2a;

        needed = 0;
        entrysize = (Level == 1) ? sizeof(PORT_INFO_1A) : sizeof(PORT_INFO_2A);

        /* First pass: calculate the size for all Entries */
        pi2w = (LPPORT_INFO_2W) bufferW;
        pi2a = (LPPORT_INFO_2A) pPorts;
        index = 0;
        while (index < numentries)
        {
            index++;
            needed += entrysize;    /* PORT_INFO_?A */
            TRACE("%p: parsing #%d (%s)\n", pi2w, index, debugstr_w(pi2w->pPortName));

            needed += WideCharToMultiByte(CP_ACP, 0, pi2w->pPortName, -1,
                                            NULL, 0, NULL, NULL);
            if (Level > 1)
            {
                needed += WideCharToMultiByte(CP_ACP, 0, pi2w->pMonitorName, -1,
                                                NULL, 0, NULL, NULL);
                needed += WideCharToMultiByte(CP_ACP, 0, pi2w->pDescription, -1,
                                                NULL, 0, NULL, NULL);
            }
            /* use LPBYTE with entrysize to avoid double code (PORT_INFO_1 + PORT_INFO_2) */
            pi2w = (LPPORT_INFO_2W) (((LPBYTE)pi2w) + entrysize);
            pi2a = (LPPORT_INFO_2A) (((LPBYTE)pi2a) + entrysize);
        }

        /* check for errors and quit on failure */
        if (cbBuf < needed)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            res = FALSE;
            goto cleanup;
        }
        len = entrysize * numentries;       /* room for all PORT_INFO_?A */
        ptr = (LPSTR) &pPorts[len];         /* room for strings */
        cbBuf -= len ;                      /* free Bytes in the user-Buffer */
        pi2w = (LPPORT_INFO_2W) bufferW;
        pi2a = (LPPORT_INFO_2A) pPorts;
        index = 0;
        /* Second Pass: Fill the User Buffer (if we have one) */
        while ((index < numentries) && pPorts)
        {
            index++;
            TRACE("%p: writing PORT_INFO_%dA #%d\n", pi2a, Level, index);
            pi2a->pPortName = ptr;
            len = WideCharToMultiByte(CP_ACP, 0, pi2w->pPortName, -1,
                                            ptr, cbBuf , NULL, NULL);
            ptr += len;
            cbBuf -= len;
            if (Level > 1)
            {
                pi2a->pMonitorName = ptr;
                len = WideCharToMultiByte(CP_ACP, 0, pi2w->pMonitorName, -1,
                                            ptr, cbBuf, NULL, NULL);
                ptr += len;
                cbBuf -= len;

                pi2a->pDescription = ptr;
                len = WideCharToMultiByte(CP_ACP, 0, pi2w->pDescription, -1,
                                            ptr, cbBuf, NULL, NULL);
                ptr += len;
                cbBuf -= len;

                pi2a->fPortType = pi2w->fPortType;
                pi2a->Reserved = 0; /* documented: "must be zero" */

            }
            /* use LPBYTE with entrysize to avoid double code (PORT_INFO_1 + PORT_INFO_2) */
            pi2w = (LPPORT_INFO_2W) (((LPBYTE)pi2w) + entrysize);
            pi2a = (LPPORT_INFO_2A) (((LPBYTE)pi2a) + entrysize);
        }
    }

cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = (res) ? numentries : 0;

    if (nameW) HeapFree(GetProcessHeap(), 0, nameW);
    if (bufferW) HeapFree(GetProcessHeap(), 0, bufferW);

    TRACE("returning %d with %d (%d byte for %d of %d entries)\n",
            (res), GetLastError(), needed, (res)? numentries : 0, numentries);

    return (res);
}

BOOL WINAPI
EnumPortsW(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

    TRACE("EnumPortsW(%S, %lu, %p, %lu, %p, %p)\n", pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);

    if ((Level < 1) || (Level > 2))
    {
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPorts(pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPorts failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallUpStructuresArray(cbBuf, pPorts, *pcReturned, pPortInfoMarshalling[Level]->pInfo, pPortInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SetPortA(PSTR pName, PSTR pPortName, DWORD dwLevel, PBYTE pPortInfo)
{
    LPWSTR NameW = NULL;
    LPWSTR PortNameW = NULL;
    PORT_INFO_3W  pi3W;
    PORT_INFO_3A *pi3A;
    DWORD len;
    BOOL res;

    pi3A = (PORT_INFO_3A*)pPortInfo;

    TRACE("SetPortA(%s, %s, %lu, %p)\n", pName, pPortName, dwLevel, pPortInfo);

    if ( dwLevel != 3 )
    {
        ERR("Level = %d, unsupported!\n", dwLevel);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        NameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, NameW, len);
    }

    if (pPortName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pPortName, -1, NULL, 0);
        PortNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pPortName, -1, PortNameW, len);
    }

    if (pi3A->pszStatus)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pi3A->pszStatus, -1, NULL, 0);
        pi3W.pszStatus = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pi3A->pszStatus, -1, pi3W.pszStatus, len);
    }

    pi3W.dwStatus   = pi3A->dwStatus;
    pi3W.dwSeverity = pi3A->dwSeverity;

    res = SetPortW( NameW, PortNameW, dwLevel, (PBYTE)&pi3W );

    if (NameW) HeapFree(GetProcessHeap(), 0, NameW);
    if (PortNameW) HeapFree(GetProcessHeap(), 0, PortNameW);
    if (pi3W.pszStatus) HeapFree(GetProcessHeap(), 0, pi3W.pszStatus);

    return res;
}

BOOL WINAPI
SetPortW(PWSTR pName, PWSTR pPortName, DWORD dwLevel, PBYTE pPortInfo)
{
    DWORD dwErrorCode;
    WINSPOOL_PORT_CONTAINER PortInfoContainer;

    TRACE("SetPortW(%S, %S, %lu, %p)\n", pName, pPortName, dwLevel, pPortInfo);

    if ( dwLevel != 3 )
    {
        ERR("Level = %d, unsupported!\n", dwLevel);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    PortInfoContainer.PortInfo.pPortInfo3 = (WINSPOOL_PORT_INFO_3*)pPortInfo;
    PortInfoContainer.Level = dwLevel;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcSetPort(pName, pPortName, &PortInfoContainer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
