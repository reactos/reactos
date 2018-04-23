/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS msgina.dll
 * FILE:            lib/msgina/shutdown.c
 * PURPOSE:         Shutdown Dialog Box (GUI only)
 * PROGRAMMERS:     Lee Schroeder (spaceseel at gmail dot com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "msgina.h"
#include <powrprof.h>
#include <wingdi.h>

/* Shutdown state flags */
#define WLX_SHUTDOWN_STATE_LOGOFF       0x01
#define WLX_SHUTDOWN_STATE_POWER_OFF    0x02
#define WLX_SHUTDOWN_STATE_REBOOT       0x04
// 0x08
#define WLX_SHUTDOWN_STATE_SLEEP        0x10
// 0x20
#define WLX_SHUTDOWN_STATE_HIBERNATE    0x40
// 0x80

typedef struct _SHUTDOWN_DLG_CONTEXT
{
    PGINA_CONTEXT pgContext;
    HBITMAP hBitmap;
    DWORD ShutdownOptions;
    BOOL bCloseDlg;
    BOOL bReasonUI;
} SHUTDOWN_DLG_CONTEXT, *PSHUTDOWN_DLG_CONTEXT;


static
BOOL
GetShutdownReasonUI(VOID)
{
    OSVERSIONINFOEX VersionInfo;
    DWORD dwValue, dwSize;
    HKEY hKey;
    LONG lRet;

    /* Query the policy value */
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"Software\\Policies\\Microsoft\\Windows NT\\Reliability",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwValue = 0;
        dwSize = sizeof(dwValue);
        RegQueryValueExW(hKey,
                         L"ShutdownReasonUI",
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize);
        RegCloseKey(hKey);

        return (dwValue != 0) ? TRUE : FALSE;
    }

    /* Query the machine value */
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"Software\\Microsoft\\Windows\\CurrentVersion\\Reliability",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwValue = 0;
        dwSize = sizeof(dwValue);
        RegQueryValueExW(hKey,
                         L"ShutdownReasonUI",
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize);
        RegCloseKey(hKey);

        return (dwValue != 0) ? TRUE : FALSE;
    }

    /* Return the default value */
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    if (!GetVersionEx((POSVERSIONINFO)&VersionInfo))
        return FALSE;

    return FALSE;
//    return (VersionInfo.wProductType == VER_NT_WORKSTATION) ? FALSE : TRUE;
}

DWORD
GetDefaultShutdownSelState(VOID)
{
    return WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;
}

DWORD
LoadShutdownSelState(VOID)
{
    LONG lRet;
    HKEY hKeyCurrentUser, hKey;
    DWORD dwValue, dwTemp, dwSize;

    /* Default to shutdown */
    dwValue = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;

    /* Open the current user HKCU key */
    lRet = RegOpenCurrentUser(MAXIMUM_ALLOWED, &hKeyCurrentUser);
    if (lRet == ERROR_SUCCESS)
    {
        /* Open the subkey */
        lRet = RegOpenKeyExW(hKeyCurrentUser,
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                             0, KEY_QUERY_VALUE, &hKey);
        RegCloseKey(hKeyCurrentUser);
    }
    if (lRet != ERROR_SUCCESS)
        return dwValue;

    /* Read the value */
    dwSize = sizeof(dwTemp);
    lRet = RegQueryValueExW(hKey,
                            L"Shutdown Setting",
                            NULL, NULL,
                            (LPBYTE)&dwTemp, &dwSize);
    RegCloseKey(hKey);

    if (lRet == ERROR_SUCCESS)
    {
        switch (dwTemp)
        {
            case WLX_SHUTDOWN_STATE_LOGOFF:
                dwValue = WLX_SAS_ACTION_LOGOFF;
                break;

            case WLX_SHUTDOWN_STATE_POWER_OFF:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;
                break;

            case WLX_SHUTDOWN_STATE_REBOOT:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_REBOOT;
                break;

            // 0x08

            case WLX_SHUTDOWN_STATE_SLEEP:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_SLEEP;
                break;

            // 0x20

            case WLX_SHUTDOWN_STATE_HIBERNATE:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_HIBERNATE;
                break;

            // 0x80
        }
    }

    return dwValue;
}

VOID
SaveShutdownSelState(
    IN DWORD ShutdownCode)
{
    LONG lRet;
    HKEY hKeyCurrentUser, hKey;
    DWORD dwValue = 0;

    /* Open the current user HKCU key */
    lRet = RegOpenCurrentUser(MAXIMUM_ALLOWED, &hKeyCurrentUser);
    if (lRet == ERROR_SUCCESS)
    {
        /* Create the subkey */
        lRet = RegCreateKeyExW(hKeyCurrentUser,
                               L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                               0, NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_SET_VALUE,
                               NULL, &hKey, NULL);
        RegCloseKey(hKeyCurrentUser);
    }
    if (lRet != ERROR_SUCCESS)
        return;

    switch (ShutdownCode)
    {
        case WLX_SAS_ACTION_LOGOFF:
            dwValue = WLX_SHUTDOWN_STATE_LOGOFF;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
            dwValue = WLX_SHUTDOWN_STATE_POWER_OFF;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
            dwValue = WLX_SHUTDOWN_STATE_REBOOT;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_SLEEP:
            dwValue = WLX_SHUTDOWN_STATE_SLEEP;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_HIBERNATE:
            dwValue = WLX_SHUTDOWN_STATE_HIBERNATE;
            break;
    }

    RegSetValueExW(hKey,
                   L"Shutdown Setting",
                   0, REG_DWORD,
                   (LPBYTE)&dwValue, sizeof(dwValue));
    RegCloseKey(hKey);
}

DWORD
GetDefaultShutdownOptions(VOID)
{
    return WLX_SHUTDOWN_STATE_POWER_OFF | WLX_SHUTDOWN_STATE_REBOOT;
}

DWORD
GetAllowedShutdownOptions(VOID)
{
    DWORD Options = 0;

    // FIXME: Compute those options accordings to current user's rights!
    Options |= WLX_SHUTDOWN_STATE_LOGOFF | WLX_SHUTDOWN_STATE_POWER_OFF | WLX_SHUTDOWN_STATE_REBOOT;

    if (IsPwrSuspendAllowed())
        Options |= WLX_SHUTDOWN_STATE_SLEEP;

    if (IsPwrHibernateAllowed())
        Options |= WLX_SHUTDOWN_STATE_HIBERNATE;

    return Options;
}

static VOID
UpdateShutdownDesc(
    IN HWND hDlg,
    IN PSHUTDOWN_DLG_CONTEXT pContext) // HINSTANCE hInstance
{
    UINT DescId = 0;
    DWORD ShutdownCode;
    WCHAR szBuffer[256];

    ShutdownCode = SendDlgItemMessageW(hDlg, IDC_SHUTDOWN_ACTION, CB_GETCURSEL, 0, 0);
    if (ShutdownCode == CB_ERR) // Invalid selection
        return;

    ShutdownCode = SendDlgItemMessageW(hDlg, IDC_SHUTDOWN_ACTION, CB_GETITEMDATA, ShutdownCode, 0);

    switch (ShutdownCode)
    {
        case WLX_SAS_ACTION_LOGOFF:
            DescId = IDS_SHUTDOWN_LOGOFF_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
            DescId = IDS_SHUTDOWN_SHUTDOWN_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
            DescId = IDS_SHUTDOWN_RESTART_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_SLEEP:
            DescId = IDS_SHUTDOWN_SLEEP_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_HIBERNATE:
            DescId = IDS_SHUTDOWN_HIBERNATE_DESC;
            break;

        default:
            break;
    }

    LoadStringW(pContext->pgContext->hDllInstance, DescId, szBuffer, _countof(szBuffer));
    SetDlgItemTextW(hDlg, IDC_SHUTDOWN_DESCRIPTION, szBuffer);

    if (pContext->bReasonUI)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_REASON_PLANNED), (ShutdownCode != WLX_SAS_ACTION_LOGOFF));
        EnableWindow(GetDlgItem(hDlg, IDC_REASON_LIST), (ShutdownCode != WLX_SAS_ACTION_LOGOFF));
        EnableWindow(GetDlgItem(hDlg, IDC_REASON_COMMENT), (ShutdownCode != WLX_SAS_ACTION_LOGOFF));
    }
}

static VOID
ShutdownOnInit(
    IN HWND hDlg,
    IN PSHUTDOWN_DLG_CONTEXT pContext)
{
    PGINA_CONTEXT pgContext = pContext->pgContext;
    HWND hwndList;
    INT idx, count, i;
    WCHAR szBuffer[256];
    WCHAR szBuffer2[256];

    hwndList = GetDlgItem(hDlg, IDC_SHUTDOWN_ACTION);

    /* Clear the content before it's used */
    SendMessageW(hwndList, CB_RESETCONTENT, 0, 0);

    /* Log off */
    if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_LOGOFF)
    {
        LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_LOGOFF, szBuffer, _countof(szBuffer));
        StringCchPrintfW(szBuffer2, _countof(szBuffer2), szBuffer, pgContext->UserName);
        idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer2);
        if (idx != CB_ERR)
            SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_LOGOFF);
    }

    /* Shut down - DEFAULT */
    if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_POWER_OFF)
    {
        LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_SHUTDOWN, szBuffer, _countof(szBuffer));
        idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
        if (idx != CB_ERR)
            SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
    }

    /* Restart */
    if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_REBOOT)
    {
        LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_RESTART, szBuffer, _countof(szBuffer));
        idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
        if (idx != CB_ERR)
            SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_REBOOT);
    }

    // if (pContext->ShutdownOptions & 0x08) {}

    /* Sleep */
    if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_SLEEP)
    {
        LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_SLEEP, szBuffer, _countof(szBuffer));
        idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
        if (idx != CB_ERR)
            SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_SLEEP);
    }

    // if (pContext->ShutdownOptions & 0x20) {}

    /* Hibernate */
    if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_HIBERNATE)
    {
        LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_HIBERNATE, szBuffer, _countof(szBuffer));
        idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
        if (idx != CB_ERR)
            SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_HIBERNATE);
    }

    // if (pContext->ShutdownOptions & 0x80) {}

    /* Set the default shut down selection */
    count = SendMessageW(hwndList, CB_GETCOUNT, 0, 0);
    for (i = 0; i < count; i++)
    {
        if (SendMessageW(hwndList, CB_GETITEMDATA, i, 0) == pgContext->nShutdownAction)
        {
            SendMessageW(hwndList, CB_SETCURSEL, i, 0);
            break;
        }
    }

    /* Update the choice description based on the current selection */
    UpdateShutdownDesc(hDlg, pContext);
}

static VOID
ShutdownOnOk(
    IN HWND hDlg,
    IN PGINA_CONTEXT pgContext)
{
    INT idx;

    idx = SendDlgItemMessageW(hDlg,
                              IDC_SHUTDOWN_ACTION,
                              CB_GETCURSEL,
                              0,
                              0);
    if (idx != CB_ERR)
    {
        pgContext->nShutdownAction =
            SendDlgItemMessageW(hDlg,
                                IDC_SHUTDOWN_ACTION,
                                CB_GETITEMDATA,
                                idx,
                                0);
    }
}

static INT_PTR
CALLBACK
ShutdownDialogProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSHUTDOWN_DLG_CONTEXT pContext;

    pContext = (PSHUTDOWN_DLG_CONTEXT)GetWindowLongPtrW(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pContext = (PSHUTDOWN_DLG_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)pContext);

            ShutdownOnInit(hDlg, pContext);

            /* Draw the logo bitmap */
            pContext->hBitmap =
                LoadImageW(pContext->pgContext->hDllInstance, MAKEINTRESOURCEW(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            return TRUE;
        }

        case WM_DESTROY:
            DeleteObject(pContext->hBitmap);
            return TRUE;

        case WM_ACTIVATE:
        {
            /*
             * If the user deactivates the shutdown dialog (it loses its focus
             * while the dialog is not being closed), then destroy the dialog
             * and cancel shutdown.
             */
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                if (!pContext->bCloseDlg)
                {
                    pContext->bCloseDlg = TRUE;
                    EndDialog(hDlg, 0);
                }
            }
            return FALSE;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            if (pContext->hBitmap)
            {
                BeginPaint(hDlg, &ps);
                DrawStateW(ps.hdc, NULL, NULL, (LPARAM)pContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hDlg, &ps);
            }
            return TRUE;
        }

        case WM_CLOSE:
            pContext->bCloseDlg = TRUE;
            EndDialog(hDlg, IDCANCEL);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    ShutdownOnOk(hDlg, pContext->pgContext);

                /* Fall back */
                case IDCANCEL:
                case IDHELP:
                    pContext->bCloseDlg = TRUE;
                    EndDialog(hDlg, LOWORD(wParam));
                    break;

                case IDC_SHUTDOWN_ACTION:
                    UpdateShutdownDesc(hDlg, pContext);
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

INT_PTR
ShutdownDialog(
    IN HWND hwndDlg,
    IN DWORD ShutdownOptions,
    IN PGINA_CONTEXT pgContext)
{
    INT_PTR ret;
    SHUTDOWN_DLG_CONTEXT Context;

#if 0
    DWORD ShutdownOptions;

    // FIXME: User impersonation!!
    pgContext->nShutdownAction = LoadShutdownSelState();
    ShutdownOptions = GetAllowedShutdownOptions();
#endif

    Context.pgContext = pgContext;
    Context.ShutdownOptions = ShutdownOptions;
    Context.bCloseDlg = FALSE;
    Context.bReasonUI = GetShutdownReasonUI();

    if (pgContext->hWlx && pgContext->pWlxFuncs)
    {
        ret = pgContext->pWlxFuncs->WlxDialogBoxParam(pgContext->hWlx,
                                                      pgContext->hDllInstance,
                                                      MAKEINTRESOURCEW(Context.bReasonUI ? IDD_SHUTDOWN_REASON : IDD_SHUTDOWN),
                                                      hwndDlg,
                                                      ShutdownDialogProc,
                                                      (LPARAM)&Context);
    }
    else
    {
        ret = DialogBoxParamW(pgContext->hDllInstance,
                              MAKEINTRESOURCEW(Context.bReasonUI ? IDD_SHUTDOWN_REASON : IDD_SHUTDOWN),
                              hwndDlg,
                              ShutdownDialogProc,
                              (LPARAM)&Context);
    }

#if 0
    // FIXME: User impersonation!!
    if (ret == IDOK)
        SaveShutdownSelState(pgContext->nShutdownAction);
#endif

    return ret;
}


/*
 * NOTES:
 * - Based upon observations on the ShellShutdownDialog() function, the function doesn't actually
 *   do anything except show a dialog box and returning a value based upon the value chosen. That
 *   means that any code that calls the function has to execute the chosen action (shut down,
 *   restart, etc.).
 * - When this function is called in Windows XP, it shows the classic dialog box regardless if
 *   SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\LogonType is enabled or not.
 * - When the Help button is pushed, it sends the same return value as IDCANCEL (0x00), but
 *   at the same time, it calls the help file directly from the dialog box.
 * - When the dialog is created, it doesn't disable all other input from the other windows.
 *   This is done elsewhere. When running the function ShellShutdownDialog() from XP/2K3, if the user clicks
 *   out of the window, it automatically closes itself.
 * - The parameter, lpUsername never seems to be used when calling the function from Windows XP. Either
 *   it was a parameter that was never used in the final version before release, or it has a use that
 *   is currently not known.
 */
DWORD WINAPI
ShellShutdownDialog(
    HWND   hParent,
    LPWSTR lpUsername,
    BOOL   bHideLogoff)
{
    INT_PTR dlgValue;
    DWORD ShutdownOptions;

    /*
     * As we are called by the shell itself, don't use
     * the cached GINA context but use a local copy here.
     */
    GINA_CONTEXT gContext = { 0 };
    DWORD BufferSize;

    UNREFERENCED_PARAMETER(lpUsername);

    ShutdownOptions = GetAllowedShutdownOptions();
    if (bHideLogoff)
        ShutdownOptions &= ~WLX_SHUTDOWN_STATE_LOGOFF;

    /* Initialize our local GINA context */
    gContext.hDllInstance = hDllInstance;
    BufferSize = _countof(gContext.UserName);
    // NOTE: Only when this function is called, Win checks inside
    // HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
    // value "Logon User Name", and determines whether it will display
    // the user name.
    GetUserNameW(gContext.UserName, &BufferSize);
    gContext.nShutdownAction = LoadShutdownSelState();

    /* Load the shutdown dialog box */
    dlgValue = ShutdownDialog(hParent, ShutdownOptions, &gContext);

    /* Determine what to do based on user selection */
    if (dlgValue == IDOK)
    {
        SaveShutdownSelState(gContext.nShutdownAction);

        switch (gContext.nShutdownAction)
        {
            case WLX_SAS_ACTION_LOGOFF:
                return WLX_SHUTDOWN_STATE_LOGOFF;

            case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
                return WLX_SHUTDOWN_STATE_POWER_OFF;

            case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
                return WLX_SHUTDOWN_STATE_REBOOT;

            // 0x08

            case WLX_SAS_ACTION_SHUTDOWN_SLEEP:
                return WLX_SHUTDOWN_STATE_SLEEP;

            // 0x20

            case WLX_SAS_ACTION_SHUTDOWN_HIBERNATE:
                return WLX_SHUTDOWN_STATE_HIBERNATE;

            // 0x80
        }
    }
    /* Help file is called directly here */
    else if (dlgValue == IDHELP)
    {
        FIXME("Help is not implemented yet.");
        MessageBoxW(hParent, L"Help is not implemented yet.", L"Message", MB_OK | MB_ICONEXCLAMATION);
    }
    else if (dlgValue == -1)
    {
        ERR("Failed to create dialog\n");
    }

    return 0;
}
