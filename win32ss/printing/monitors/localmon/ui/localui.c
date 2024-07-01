/*
 * Implementation of the Local Printmonitor User Interface
 *
 * Copyright 2007 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#ifdef __REACTOS__
#include <wchar.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"

#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/debug.h"
#include "localui.h"

WINE_DEFAULT_DEBUG_CHANNEL(localui);

/*****************************************************/

static HINSTANCE LOCALUI_hInstance;

static const WCHAR cmd_AddPortW[] = {'A','d','d','P','o','r','t',0};
static const WCHAR cmd_ConfigureLPTPortCommandOKW[] = {'C','o','n','f','i','g','u','r','e',
                                    'L','P','T','P','o','r','t',
                                    'C','o','m','m','a','n','d','O','K',0};
static const WCHAR cmd_DeletePortW[] = {'D','e','l','e','t','e','P','o','r','t',0};
static const WCHAR cmd_GetDefaultCommConfigW[] = {'G','e','t',
                                    'D','e','f','a','u','l','t',
                                    'C','o','m','m','C','o','n','f','i','g',0};
static const WCHAR cmd_GetTransmissionRetryTimeoutW[] = {'G','e','t',
                                    'T','r','a','n','s','m','i','s','s','i','o','n',
                                    'R','e','t','r','y','T','i','m','e','o','u','t',0};
static const WCHAR cmd_PortIsValidW[] = {'P','o','r','t','I','s','V','a','l','i','d',0};
static const WCHAR cmd_SetDefaultCommConfigW[] = {'S','e','t',
                                    'D','e','f','a','u','l','t',
                                    'C','o','m','m','C','o','n','f','i','g',0};

static const WCHAR fmt_uW[]  = {'%','u',0};
static const WCHAR portname_LPT[]  = {'L','P','T',0};
static const WCHAR portname_COM[]  = {'C','O','M',0};
static const WCHAR portname_FILE[] = {'F','I','L','E',':',0};
static const WCHAR portname_CUPS[] = {'C','U','P','S',':',0};
static const WCHAR portname_LPR[]  = {'L','P','R',':',0};

static const WCHAR XcvMonitorW[] = {',','X','c','v','M','o','n','i','t','o','r',' ',0};
static const WCHAR XcvPortW[] = {',','X','c','v','P','o','r','t',' ',0};

/*****************************************************/

typedef struct tag_addportui_t {
    LPWSTR  portname;
    HANDLE  hXcv;
} addportui_t;

typedef struct tag_lptconfig_t {
    HANDLE  hXcv;
    DWORD   value;
} lptconfig_t;


static INT_PTR CALLBACK dlgproc_lptconfig(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/*****************************************************
 *   strdupWW [internal]
 */

static LPWSTR strdupWW(LPCWSTR pPrefix, LPCWSTR pSuffix)
{
    LPWSTR  ptr;
    DWORD   len;

    len = lstrlenW(pPrefix) + (pSuffix ? lstrlenW(pSuffix) : 0) + 1;
    ptr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (ptr) {
        lstrcpyW(ptr, pPrefix);
        if (pSuffix) lstrcatW(ptr, pSuffix);
    }
    return ptr;
}

/*****************************************************
 *   dlg_configure_com [internal]
 *
 */

static BOOL dlg_configure_com(HANDLE hXcv, HWND hWnd, PCWSTR pPortName)
{
    COMMCONFIG cfg;
    LPWSTR shortname;
    DWORD status;
    DWORD dummy;
    DWORD len;
    BOOL  res;

    /* strip the colon (pPortName is never empty here) */
    len = lstrlenW(pPortName);
    shortname = HeapAlloc(GetProcessHeap(), 0, len  * sizeof(WCHAR));
    if (shortname) {
        memcpy(shortname, pPortName, (len -1) * sizeof(WCHAR));
        shortname[len-1] = '\0';

        /* get current settings */
        len = FIELD_OFFSET(COMMCONFIG, wcProviderData[1]);
        status = ERROR_SUCCESS;
        res = XcvDataW( hXcv, cmd_GetDefaultCommConfigW,
                        (PBYTE) shortname,
                        (lstrlenW(shortname) +1) * sizeof(WCHAR),
                        (PBYTE) &cfg, len, &len, &status);

        if (res && (status == ERROR_SUCCESS)) {
            /* display the Dialog */
            res = CommConfigDialogW(pPortName, hWnd, &cfg);
            if (res) {
                status = ERROR_SUCCESS;
                /* set new settings */
                res = XcvDataW(hXcv, cmd_SetDefaultCommConfigW,
                               (PBYTE) &cfg, len,
                               (PBYTE) &dummy, 0, &len, &status);
            }
        }
        HeapFree(GetProcessHeap(), 0, shortname);
        return res;
    }
    return FALSE;
}


/*****************************************************
 *   dlg_configure_lpt [internal]
 *
 */

static BOOL dlg_configure_lpt(HANDLE hXcv, HWND hWnd)
{
    lptconfig_t data;
    BOOL  res;


    data.hXcv = hXcv;

    res = DialogBoxParamW(LOCALUI_hInstance, MAKEINTRESOURCEW(LPTCONFIG_DIALOG), hWnd,
                               dlgproc_lptconfig, (LPARAM) &data);

    TRACE("got %u with %u\n", res, GetLastError());

    if (!res) SetLastError(ERROR_CANCELLED);
    return res;
}

/******************************************************************
 *  dlg_port_already_exists [internal]
 */

static void dlg_port_already_exists(HWND hWnd, LPCWSTR portname)
{
    WCHAR res_PortW[IDS_LOCALPORT_MAXLEN];
    WCHAR res_PortExistsW[IDS_PORTEXISTS_MAXLEN];
    LPWSTR  message;
    DWORD   len;

    res_PortW[0] = '\0';
    res_PortExistsW[0] = '\0';
    LoadStringW(LOCALUI_hInstance, IDS_LOCALPORT, res_PortW, IDS_LOCALPORT_MAXLEN);
    LoadStringW(LOCALUI_hInstance, IDS_PORTEXISTS, res_PortExistsW, IDS_PORTEXISTS_MAXLEN);

    len = lstrlenW(portname) + IDS_PORTEXISTS_MAXLEN + 1;
    message = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (message) {
        message[0] = '\0';
        swprintf(message, res_PortExistsW, portname);
        MessageBoxW(hWnd, message, res_PortW, MB_OK | MB_ICONERROR);
        HeapFree(GetProcessHeap(), 0, message);
    }
}

/******************************************************************
 *  dlg_invalid_portname [internal]
 */

static void dlg_invalid_portname(HWND hWnd, LPCWSTR portname)
{
    WCHAR res_PortW[IDS_LOCALPORT_MAXLEN];
    WCHAR res_InvalidNameW[IDS_INVALIDNAME_MAXLEN];
    LPWSTR  message;
    DWORD   len;

    res_PortW[0] = '\0';
    res_InvalidNameW[0] = '\0';
    LoadStringW(LOCALUI_hInstance, IDS_LOCALPORT, res_PortW, IDS_LOCALPORT_MAXLEN);
    LoadStringW(LOCALUI_hInstance, IDS_INVALIDNAME, res_InvalidNameW, IDS_INVALIDNAME_MAXLEN);

    len = lstrlenW(portname) + IDS_INVALIDNAME_MAXLEN;
    message = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (message) {
        message[0] = '\0';
        swprintf(message, res_InvalidNameW, portname);
        MessageBoxW(hWnd, message, res_PortW, MB_OK | MB_ICONERROR);
        HeapFree(GetProcessHeap(), 0, message);
    }
}

/******************************************************************
 * display the Dialog "Nothing to configure"
 *
 */

static void dlg_nothingtoconfig(HWND hWnd)
{
    WCHAR res_PortW[IDS_LOCALPORT_MAXLEN];
    WCHAR res_nothingW[IDS_NOTHINGTOCONFIG_MAXLEN];

    res_PortW[0] = '\0';
    res_nothingW[0] = '\0';
    LoadStringW(LOCALUI_hInstance, IDS_LOCALPORT, res_PortW, IDS_LOCALPORT_MAXLEN);
    LoadStringW(LOCALUI_hInstance, IDS_NOTHINGTOCONFIG, res_nothingW, IDS_NOTHINGTOCONFIG_MAXLEN);

    MessageBoxW(hWnd, res_nothingW, res_PortW, MB_OK | MB_ICONINFORMATION);
}

/******************************************************************
 *  dlg_win32error [internal]
 */

static void dlg_win32error(HWND hWnd, DWORD lasterror)
{
    WCHAR res_PortW[IDS_LOCALPORT_MAXLEN];
    LPWSTR  message = NULL;
    DWORD   res;

    res_PortW[0] = '\0';
    LoadStringW(LOCALUI_hInstance, IDS_LOCALPORT, res_PortW, IDS_LOCALPORT_MAXLEN);


    res = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, lasterror, 0, (LPWSTR) &message, 0, NULL);

    if (res > 0) {
        MessageBoxW(hWnd, message, res_PortW, MB_OK | MB_ICONERROR);
        LocalFree(message);
    }
}

/*****************************************************************************
 *
 */

static INT_PTR CALLBACK dlgproc_addport(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    addportui_t * data;
    DWORD   status;
    DWORD   dummy;
    DWORD   len;
    DWORD   res;

    switch(msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtrW(hwnd, DWLP_USER, lparam);
        return TRUE;

    case WM_COMMAND:
        if (wparam == MAKEWPARAM(IDOK, BN_CLICKED))
        {
            data = (addportui_t *) GetWindowLongPtrW(hwnd, DWLP_USER);
            /* length in WCHAR, without the '\0' */
            len = SendDlgItemMessageW(hwnd, ADDPORT_EDIT, WM_GETTEXTLENGTH, 0, 0);
            data->portname = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));

            if (!data->portname) {
                EndDialog(hwnd, FALSE);
                return TRUE;
            }
            /* length is in WCHAR, including the '\0' */
            GetDlgItemTextW(hwnd, ADDPORT_EDIT, data->portname, len + 1);
            status = ERROR_SUCCESS;
            res = XcvDataW( data->hXcv, cmd_PortIsValidW, (PBYTE) data->portname,
                            (lstrlenW(data->portname) + 1) * sizeof(WCHAR),
                            (PBYTE) &dummy, 0, &len, &status);

            TRACE("got %u with status %u\n", res, status);
            if (res && (status == ERROR_SUCCESS)) {
                /* The caller must free data->portname */
                EndDialog(hwnd, TRUE);
                return TRUE;
            }

            if (res && (status == ERROR_INVALID_NAME)) {
                dlg_invalid_portname(hwnd, data->portname);
                HeapFree(GetProcessHeap(), 0, data->portname);
                data->portname = NULL;
                return TRUE;
            }

            dlg_win32error(hwnd, status);
            HeapFree(GetProcessHeap(), 0, data->portname);
            data->portname = NULL;
            return TRUE;
        }

        if (wparam == MAKEWPARAM(IDCANCEL, BN_CLICKED))
        {
            EndDialog(hwnd, FALSE);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/*****************************************************************************
 *   dlgproc_lptconfig  [internal]
 *
 * Our message-proc is simple, as the range-check is done only during the
 * command "OK" and the dialog is set to the start-value at "out of range".
 *
 * Native localui.dll does the check during keyboard-input and set the dialog
 * to the previous value.
 *
 */

static INT_PTR CALLBACK dlgproc_lptconfig(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    lptconfig_t * data;
    WCHAR   bufferW[16];
    DWORD   status;
    DWORD   dummy;
    DWORD   len;
    DWORD   res;


    switch(msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtrW(hwnd, DWLP_USER, lparam);
        data = (lptconfig_t *) lparam;

        /* Get current setting */
        data->value = 45;
        status = ERROR_SUCCESS;
        res = XcvDataW( data->hXcv, cmd_GetTransmissionRetryTimeoutW,
                        (PBYTE) &dummy, 0,
                        (PBYTE) &data->value, sizeof(data->value), &len, &status);

        TRACE("got %u with status %u\n", res, status);

        /* Set current setting as the initial value in the Dialog */
        SetDlgItemInt(hwnd, LPTCONFIG_EDIT, data->value, FALSE);
        return TRUE;

    case WM_COMMAND:
        if (wparam == MAKEWPARAM(IDOK, BN_CLICKED))
        {
            data = (lptconfig_t *) GetWindowLongPtrW(hwnd, DWLP_USER);

            status = FALSE;
            res = GetDlgItemInt(hwnd, LPTCONFIG_EDIT, (BOOL *) &status, FALSE);
            /* length is in WCHAR, including the '\0' */
            GetDlgItemTextW(hwnd, LPTCONFIG_EDIT, bufferW, ARRAY_SIZE(bufferW));
            TRACE("got %s and %u (translated: %u)\n", debugstr_w(bufferW), res, status);

            /* native localui.dll use the same limits */
            if ((res > 0) && (res < 1000000) && status) {
                swprintf(bufferW, fmt_uW, res);
                res = XcvDataW( data->hXcv, cmd_ConfigureLPTPortCommandOKW,
                        (PBYTE) bufferW,
                        (lstrlenW(bufferW) +1) * sizeof(WCHAR),
                        (PBYTE) &dummy, 0, &len, &status);

                TRACE("got %u with status %u\n", res, status);
                EndDialog(hwnd, TRUE);
                return TRUE;
            }

            /* Set initial value and rerun the Dialog */
            SetDlgItemInt(hwnd, LPTCONFIG_EDIT, data->value, FALSE);
            return TRUE;
        }

        if (wparam == MAKEWPARAM(IDCANCEL, BN_CLICKED))
        {
            EndDialog(hwnd, FALSE);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}


/*****************************************************
 * get_type_from_name (internal)
 *
 */

static DWORD get_type_from_name(LPCWSTR name)
{
    HANDLE  hfile;

    if (!_wcsnicmp(name, portname_LPT, ARRAY_SIZE(portname_LPT) -1))
        return PORT_IS_LPT;

    if (!_wcsnicmp(name, portname_COM, ARRAY_SIZE(portname_COM) -1))
        return PORT_IS_COM;

    if (!_wcsicmp(name, portname_FILE))
        return PORT_IS_FILE;

    if (name[0] == '/')
        return PORT_IS_UNIXNAME;

    if (name[0] == '|')
        return PORT_IS_PIPE;

    if (!wcsncmp(name, portname_CUPS, ARRAY_SIZE(portname_CUPS) -1))
        return PORT_IS_CUPS;

    if (!wcsncmp(name, portname_LPR, ARRAY_SIZE(portname_LPR) -1))
        return PORT_IS_LPR;

    /* Must be a file or a directory. Does the file exist ? */
    hfile = CreateFileW(name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    TRACE("%p for OPEN_EXISTING on %s\n", hfile, debugstr_w(name));
    if (hfile == INVALID_HANDLE_VALUE) {
        /* Can we create the file? */
        hfile = CreateFileW(name, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
        TRACE("%p for OPEN_ALWAYS\n", hfile);
    }
    if (hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(hfile);
        return PORT_IS_FILENAME;
    }
    /* We can't use the name. use GetLastError() for the reason */
    return PORT_IS_UNKNOWN;
}

/*****************************************************
 *   open_monitor_by_name [internal]
 *
 */
static BOOL open_monitor_by_name(LPCWSTR pPrefix, LPCWSTR pPort, HANDLE * phandle)
{
    PRINTER_DEFAULTSW pd;
    LPWSTR  fullname;
    BOOL    res;

    * phandle = 0;
    TRACE("(%s,%s)\n", debugstr_w(pPrefix),debugstr_w(pPort) );

    fullname = strdupWW(pPrefix, pPort);
    pd.pDatatype = NULL;
    pd.pDevMode  = NULL;
    pd.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    res = OpenPrinterW(fullname, phandle, &pd);
    HeapFree(GetProcessHeap(), 0, fullname);
    return res;
}

/*****************************************************
 *   localui_AddPortUI [exported through MONITORUI]
 *
 * Display a Dialog to add a local Port
 *
 * PARAMS
 *  pName       [I] Servername or NULL (local Computer)
 *  hWnd        [I] Handle to parent Window for the Dialog-Box or NULL
 *  pMonitorName[I] Name of the Monitor, that should be used to add a Port or NULL
 *  ppPortName  [O] PTR to PTR of a buffer, that receive the Name of the new Port or NULL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 * The caller must free the buffer (returned in ppPortName) with GlobalFree().
 * Native localui.dll failed with ERROR_INVALID_PARAMETER, when the user tried
 * to add a Port, that start with "COM" or "LPT".
 *
 */
static BOOL WINAPI localui_AddPortUI(PCWSTR pName, HWND hWnd, PCWSTR pMonitorName, PWSTR *ppPortName)
{
    addportui_t data;
    HANDLE  hXcv;
    DWORD   needed;
    DWORD   dummy;
    DWORD   status;
    DWORD   res = FALSE;

    TRACE(  "(%s, %p, %s, %p) (*ppPortName: %p)\n", debugstr_w(pName), hWnd,
            debugstr_w(pMonitorName), ppPortName, ppPortName ? *ppPortName : NULL);

    if (open_monitor_by_name(XcvMonitorW, pMonitorName, &hXcv)) {

        ZeroMemory(&data, sizeof(addportui_t));
        data.hXcv = hXcv;
        res = DialogBoxParamW(LOCALUI_hInstance, MAKEINTRESOURCEW(ADDPORT_DIALOG), hWnd,
                               dlgproc_addport, (LPARAM) &data);

        TRACE("got %u with %u for %s\n", res, GetLastError(), debugstr_w(data.portname));

        if (ppPortName) *ppPortName = NULL;

        if (res) {
            res = XcvDataW(hXcv, cmd_AddPortW, (PBYTE) data.portname,
                            (lstrlenW(data.portname)+1) * sizeof(WCHAR),
                            (PBYTE) &dummy, 0, &needed, &status);

            TRACE("got %u with status %u\n", res, status);
            if (res && (status == ERROR_SUCCESS) && ppPortName) {
                /* Native localui uses GlobalAlloc also.
                   The caller must GlobalFree the buffer */
                *ppPortName = GlobalAlloc(GPTR, (lstrlenW(data.portname)+1) * sizeof(WCHAR));
                if (*ppPortName) lstrcpyW(*ppPortName, data.portname);
            }

            if (res && (status == ERROR_ALREADY_EXISTS)) {
                dlg_port_already_exists(hWnd, data.portname);
                /* Native localui also return "TRUE" from AddPortUI in this case */
            }

            HeapFree(GetProcessHeap(), 0, data.portname);
        }
        else
        {
            SetLastError(ERROR_CANCELLED);
        }
        ClosePrinter(hXcv);
    }

    TRACE("=> %u with %u\n", res, GetLastError());
    return res;
}


/*****************************************************
 *   localui_ConfigurePortUI [exported through MONITORUI]
 *
 * Display the Configuration-Dialog for a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box or NULL
 *  pPortName [I] Name of the Port, that should be configured
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localui_ConfigurePortUI(PCWSTR pName, HWND hWnd, PCWSTR pPortName)
{
    HANDLE  hXcv;
    DWORD   res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));
    if (open_monitor_by_name(XcvPortW, pPortName, &hXcv)) {

        res = get_type_from_name(pPortName);
        switch(res)
        {

        case PORT_IS_COM:
            res = dlg_configure_com(hXcv, hWnd, pPortName);
            break;

        case PORT_IS_LPT:
            res = dlg_configure_lpt(hXcv, hWnd);
            break;

        default:
            dlg_nothingtoconfig(hWnd);
            SetLastError(ERROR_CANCELLED);
            res = FALSE;
        }

        ClosePrinter(hXcv);
        return res;
    }
    return FALSE;

}

/*****************************************************
 *   localui_DeletePortUI [exported through MONITORUI]
 *
 * Delete a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window
 *  pPortName [I] Name of the Port, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Native localui does not allow deleting a COM/LPT port (ERROR_NOT_SUPPORTED)
 *
 */
static BOOL WINAPI localui_DeletePortUI(PCWSTR pName, HWND hWnd, PCWSTR pPortName)
{
    HANDLE  hXcv;
    DWORD   dummy;
    DWORD   needed;
    DWORD   status;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    if ((!pPortName) || (!pPortName[0])) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (open_monitor_by_name(XcvPortW, pPortName, &hXcv)) {
        /* native localui tests here for LPT / COM - Ports and failed with
           ERROR_NOT_SUPPORTED. */
        if (XcvDataW(hXcv, cmd_DeletePortW, (LPBYTE) pPortName,
            (lstrlenW(pPortName)+1) * sizeof(WCHAR), (LPBYTE) &dummy, 0, &needed, &status)) {

            ClosePrinter(hXcv);
            if (status != ERROR_SUCCESS) SetLastError(status);
            return (status == ERROR_SUCCESS);
        }
        ClosePrinter(hXcv);
        return FALSE;
    }
    SetLastError(ERROR_UNKNOWN_PORT);
    return FALSE;
}

/*****************************************************
 *      InitializePrintMonitorUI  (LOCALUI.@)
 *
 * Initialize the User-Interface for the Local Ports
 *
 * RETURNS
 *  Success: Pointer to a MONITORUI Structure
 *  Failure: NULL
 *
 */

PMONITORUI WINAPI InitializePrintMonitorUI(void)
{
    static MONITORUI mymonitorui =
    {
        sizeof(MONITORUI),
        localui_AddPortUI,
        localui_ConfigurePortUI,
        localui_DeletePortUI
    };

    TRACE("=> %p\n", &mymonitorui);
    return &mymonitorui;
}

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinstDLL );
            LOCALUI_hInstance = hinstDLL;
            break;
    }
    return TRUE;
}
