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
#include <stdlib.h>

int g_shutdownCode = 0;
BOOL g_logoffHideState = FALSE;

VOID UpdateShutdownShellDesc(HWND hwnd)
{
    WCHAR tmpBuffer[256];
    UINT shutdownDescId = 0;
    HWND shutdownHwnd = GetDlgItem(hwnd, IDC_SHUTDOWN_DESCRIPTION);
    int shutdownCode = 0;
    
    shutdownCode = SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_GETCURSEL, 0, 0);

    if(!g_logoffHideState)
    {
        switch (shutdownCode)
        {
        case 0: /* Log off */
            shutdownDescId = IDS_SHUTDOWN_LOGOFF_DESC;
            break;
        case 1: /* Shut down */
            shutdownDescId = IDS_SHUTDOWN_SHUTDOWN_DESC;
            break;
        case 2: /* Restart */
            shutdownDescId = IDS_SHUTDOWN_RESTART_DESC;
            break;
        default:
            break;
        }
        
        if (IsPwrSuspendAllowed())
        {
            if (shutdownCode == 3) /* Sleep */
            {
                shutdownDescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
            else if (shutdownCode == 4) /* Hibernate */
            {
                shutdownDescId = IDS_SHUTDOWN_HIBERNATE_DESC;
            }
        }
        else
        {
            if (shutdownCode == 3) /* Hibernate */
            {
                shutdownDescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
        }
    }
    else
    {
        switch (shutdownCode)
        {
        case 0: /* Shut down */
            shutdownDescId = IDS_SHUTDOWN_SHUTDOWN_DESC;
            break;
        case 1: /* Restart */
            shutdownDescId = IDS_SHUTDOWN_RESTART_DESC;
            break;
        default:
            break;
        }

        if (IsPwrSuspendAllowed())
        {
            if (shutdownCode == 2) /* Sleep */
            {
                shutdownDescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
            else if (shutdownCode == 3) /* Hibernate */
            {
                shutdownDescId = IDS_SHUTDOWN_HIBERNATE_DESC;
            }
        }
        else
        {
            if (shutdownCode == 2) /* Hibernate */
            {
                shutdownDescId = IDS_SHUTDOWN_SLEEP_DESC;
            }
        }
    }

    LoadStringW(hDllInstance, shutdownDescId, tmpBuffer, sizeof(tmpBuffer));
    SetWindowTextW(shutdownHwnd, tmpBuffer);
}
 
BOOL CALLBACK ExitWindowsDialogShellProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwnd, GWL_USERDATA);

    switch (Message)
    {
        case WM_INITDIALOG:
        {
            int defSelect = 0;
            WCHAR userBuffer[256];
            DWORD userBufferSize = _countof(userBuffer);
            WCHAR tmpBuffer[256];
            WCHAR tmpBuffer2[512];

            pgContext = (PGINA_CONTEXT)lParam;
            if (!pgContext)
            {
                WARN("pgContext is NULL, branding bitmaps will not be displayed.\n");
            }
            
            SetWindowLongPtr(hwnd, GWL_USERDATA, (DWORD_PTR)pgContext);

            /* Clears the content before it's used */
            SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_RESETCONTENT, 0, 0);

            if(!g_logoffHideState)
            {
                /* Log off */
                LoadStringW(hDllInstance, IDS_SHUTDOWN_LOGOFF, tmpBuffer, sizeof(tmpBuffer)/sizeof(WCHAR));
                GetUserNameW(userBuffer, &userBufferSize);
                StringCchPrintfW(tmpBuffer2, 512, tmpBuffer, userBuffer);
                SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer2);
            }

            /* Shut down - DEFAULT */
            LoadStringW(hDllInstance, IDS_SHUTDOWN_SHUTDOWN, tmpBuffer, sizeof(tmpBuffer)/sizeof(WCHAR));
            defSelect = SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);

            /* Restart */
            LoadStringW(hDllInstance, IDS_SHUTDOWN_RESTART, tmpBuffer, sizeof(tmpBuffer)/sizeof(WCHAR));
            SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);

            /* Sleep */
            if (IsPwrSuspendAllowed())
            {
                LoadStringW(hDllInstance, IDS_SHUTDOWN_SLEEP, tmpBuffer, sizeof(tmpBuffer)/sizeof(WCHAR));
                SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);
            }

            /* Hibernate */
            if (IsPwrHibernateAllowed())
            {
                LoadStringW(hDllInstance, IDS_SHUTDOWN_HIBERNATE, tmpBuffer, sizeof(tmpBuffer)/sizeof(WCHAR));
                SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_ADDSTRING, 0, (LPARAM)tmpBuffer);
            }

            /* Sets the default shut down selection */
            SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_SETCURSEL, defSelect, 0);
            
            /* Updates the choice description based on the current selection */
            UpdateShutdownShellDesc(hwnd);
            
            /* Draw the logo graphic */
            if (pgContext)
                pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

            return TRUE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext && pgContext->hBitmap)
            {
                hdc = BeginPaint(hwnd, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwnd, &ps);
                return TRUE;
            }
            return FALSE;
        }
        case WM_DESTROY:
        {
            if (pgContext)
                DeleteObject(pgContext->hBitmap);
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    g_shutdownCode = SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
                case IDHELP:
                    EndDialog(hwnd, IDHELP);
                    break;
                case IDC_SHUTDOWN_LIST:
                    UpdateShutdownShellDesc(hwnd);
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
    int dlgValue = 0;
    
    g_logoffHideState = bHideLogoff;

    UNREFERENCED_PARAMETER(lpUsername);

    // Loads the shut down dialog box
    dlgValue = DialogBoxParam(hDllInstance,
                              MAKEINTRESOURCE(IDD_SHUTDOWN_SHELL),
                              hParent,
                              ExitWindowsDialogShellProc,
                              (LPARAM)&pgContext);

    // Determines what to do based on user selection
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
