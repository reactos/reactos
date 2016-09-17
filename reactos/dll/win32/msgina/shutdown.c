/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS msgina.dll
 * FILE:            lib/msgina/shutdown.c
 * PURPOSE:         Shutdown Dialog Box
 * PROGRAMMER:      Lee Schroeder (spaceseel at gmail dot com)
 */

#include "msgina.h"
#include <powrprof.h>
#include <strsafe.h>
#include <wingdi.h>
#include <winreg.h>
#include <stdlib.h>

DWORD g_shutdownCode = 0;
BOOL g_logoffHideState = FALSE;

static DWORD
LoadShutdownSelState(VOID)
{
    HKEY hKey;
    LONG lRet;
    DWORD dwValue, dwTemp, dwSize;

    /* Default to shutdown */
    dwValue = 1;

    lRet = RegOpenKeyExW(HKEY_CURRENT_USER,
                         L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                         0, KEY_QUERY_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS)
        return dwValue;

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
            case 0x01: /* Log off */
                dwValue = 0;
                break;

            case 0x02: /* Shut down */
                dwValue = 1;
                break;

            case 0x04: /* Reboot */
                dwValue = 2;
                break;

            case 0x10: /* Sleep */
                dwValue = 3;
                break;

            case 0x40: /* Hibernate */
                dwValue = 4;
                break;
        }
    }

    return dwValue;
}

static VOID
SaveShutdownSelState(DWORD ShutdownCode)
{
    HKEY hKey;
    DWORD dwValue = 0;

    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                        0, NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_SET_VALUE,
                        NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        return;
    }

    switch (ShutdownCode)
    {
        case 0: /* Log off */
            dwValue = 0x01;
            break;

        case 1: /* Shut down */
            dwValue = 0x02;
            break;

        case 2: /* Reboot */
            dwValue = 0x04;
            break;

        case 3: /* Sleep */
            dwValue = 0x10;
            break;

        case 4: /* Hibernate */
            dwValue = 0x40;
            break;
    }

    RegSetValueExW(hKey,
                   L"Shutdown Setting",
                   0, REG_DWORD,
                   (LPBYTE)&dwValue, sizeof(dwValue));
    RegCloseKey(hKey);
}

static VOID
UpdateShutdownShellDesc(HWND hWnd)
{
    UINT DescId = 0;
    DWORD ShutdownCode = 0;
    WCHAR tmpBuffer[256];

    ShutdownCode = SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_GETCURSEL, 0, 0);

    if (!g_logoffHideState)
    {
        switch (ShutdownCode)
        {
        case 0: /* Log off */
            DescId = IDS_SHUTDOWN_LOGOFF_DESC;
            break;
        case 1: /* Shut down */
            DescId = IDS_SHUTDOWN_SHUTDOWN_DESC;
            break;
        case 2: /* Restart */
            DescId = IDS_SHUTDOWN_RESTART_DESC;
            break;
        default:
            break;
        }

        if (IsPwrSuspendAllowed())
        {
            if (ShutdownCode == 3) /* Sleep */
            {
                DescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
            else if (ShutdownCode == 4) /* Hibernate */
            {
                DescId = IDS_SHUTDOWN_HIBERNATE_DESC;
            }
        }
        else
        {
            if (ShutdownCode == 3) /* Hibernate */
            {
                DescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
        }
    }
    else
    {
        switch (ShutdownCode)
        {
        case 0: /* Shut down */
            DescId = IDS_SHUTDOWN_SHUTDOWN_DESC;
            break;
        case 1: /* Restart */
            DescId = IDS_SHUTDOWN_RESTART_DESC;
            break;
        default:
            break;
        }

        if (IsPwrSuspendAllowed())
        {
            if (ShutdownCode == 2) /* Sleep */
            {
                DescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
            else if (ShutdownCode == 3) /* Hibernate */
            {
                DescId = IDS_SHUTDOWN_HIBERNATE_DESC;
            }
        }
        else
        {
            if (ShutdownCode == 2) /* Hibernate */
            {
                DescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
        }
    }

    LoadStringW(hDllInstance, DescId, tmpBuffer, _countof(tmpBuffer));
    SetDlgItemTextW(hWnd, IDC_SHUTDOWN_DESCRIPTION, tmpBuffer);
}

BOOL CALLBACK
ExitWindowsDialogShellProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            int defSelect = 0;
            int tmpSelect, lastState;
            WCHAR userBuffer[256];
            DWORD userBufferSize = _countof(userBuffer);
            WCHAR tmpBuffer[256];
            WCHAR tmpBuffer2[512];

            pgContext = (PGINA_CONTEXT)lParam;
            if (!pgContext)
            {
                WARN("pgContext is NULL, branding bitmaps will not be displayed.\n");
            }

            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pgContext);

            /* Clear the content before it's used */
            SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_RESETCONTENT, 0, 0);

            lastState = LoadShutdownSelState();

            if (!g_logoffHideState)
            {
                /* Log off */
                LoadStringW(hDllInstance, IDS_SHUTDOWN_LOGOFF, tmpBuffer, _countof(tmpBuffer));
                GetUserNameW(userBuffer, &userBufferSize);
                StringCchPrintfW(tmpBuffer2, _countof(tmpBuffer2), tmpBuffer, userBuffer);
                tmpSelect = SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer2);
                if (lastState == 0)
                {
                    defSelect = tmpSelect;
                }
            }

            /* Shut down - DEFAULT */
            LoadStringW(hDllInstance, IDS_SHUTDOWN_SHUTDOWN, tmpBuffer, _countof(tmpBuffer));
            tmpSelect = SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);
            if (lastState == 1)
            {
                defSelect = tmpSelect;
            }

            /* Restart */
            LoadStringW(hDllInstance, IDS_SHUTDOWN_RESTART, tmpBuffer, _countof(tmpBuffer));
            tmpSelect = SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);
            if (lastState == 2)
            {
                defSelect = tmpSelect;
            }

            /* Sleep */
            if (IsPwrSuspendAllowed())
            {
                LoadStringW(hDllInstance, IDS_SHUTDOWN_SLEEP, tmpBuffer, _countof(tmpBuffer));
                tmpSelect = SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);
                if (lastState == 3)
                {
                    defSelect = tmpSelect;
                }
            }

            /* Hibernate */
            if (IsPwrHibernateAllowed())
            {
                LoadStringW(hDllInstance, IDS_SHUTDOWN_HIBERNATE, tmpBuffer, _countof(tmpBuffer));
                tmpSelect = SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);
                if (lastState == 4)
                {
                    defSelect = tmpSelect;
                }
            }

            /* Sets the default shut down selection */
            SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_SETCURSEL, defSelect, 0);

            /* Updates the choice description based on the current selection */
            UpdateShutdownShellDesc(hWnd);

            /* Draw the logo bitmap */
            if (pgContext)
                pgContext->hBitmap = LoadImageW(hDllInstance, MAKEINTRESOURCEW(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

            return TRUE;
        }

        case WM_DESTROY:
        {
            if (pgContext)
                DeleteObject(pgContext->hBitmap);
            return TRUE;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            if (pgContext && pgContext->hBitmap)
            {
                BeginPaint(hWnd, &ps);
                DrawStateW(ps.hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, 0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hWnd, &ps);
                return TRUE;
            }
            return FALSE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    g_shutdownCode = SendDlgItemMessageW(hWnd, IDC_SHUTDOWN_LIST, CB_GETCURSEL, 0, 0);
                    SaveShutdownSelState(g_shutdownCode);
                    EndDialog(hWnd, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hWnd, IDCANCEL);
                    break;

                case IDHELP:
                    EndDialog(hWnd, IDHELP);
                    break;

                case IDC_SHUTDOWN_LIST:
                    UpdateShutdownShellDesc(hWnd);
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
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
    GINA_CONTEXT pgContext = { 0 };
    INT_PTR dlgValue = 0;

    UNREFERENCED_PARAMETER(lpUsername);

    g_logoffHideState = bHideLogoff;

    /* Load the shut down dialog box */
    dlgValue = DialogBoxParamW(hDllInstance,
                               MAKEINTRESOURCEW(IDD_SHUTDOWN_SHELL),
                               hParent,
                               ExitWindowsDialogShellProc,
                               (LPARAM)&pgContext);

    /* Determine what to do based on user selection */
    if (dlgValue == IDOK)
    {
        switch (g_shutdownCode)
        {
        case 0: /* Log off */
            return 0x01;
        case 1: /* Shut down */
            return 0x02;
        case 2: /* Reboot */
            return 0x04;
        case 3: /* Sleep */
            return 0x10;
        case 4: /* Hibernate */
            return 0x40;
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

    return 0x00;
}
