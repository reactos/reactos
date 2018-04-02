/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/shutdown.c
 * PURPOSE:         System shutdown dialog
 * PROGRAMMERS:     alpha5056 <alpha5056@users.noreply.github.com>
 */

/* INCLUDES ******************************************************************/

#include "winlogon.h"

#include <rpc.h>
#include <winreg_s.h>

/* DEFINES *******************************************************************/

#define SHUTDOWN_TIMER_ID 2000


/* STRUCTS *******************************************************************/

typedef struct _SYS_SHUTDOWN_PARAMS
{
    PWSTR pszMessage;
    ULONG dwTimeout;
    BOOLEAN bRebootAfterShutdown;
    BOOLEAN bForceAppsClosed;
    DWORD dwReason;

    HWND hShutdownDialog;
    BOOLEAN bShuttingDown;
} SYS_SHUTDOWN_PARAMS, *PSYS_SHUTDOWN_PARAMS;


/* GLOBALS *******************************************************************/

SYS_SHUTDOWN_PARAMS g_ShutdownParams;


/* FUNCTIONS *****************************************************************/

static
VOID
OnTimer(
    HWND hwndDlg,
    PSYS_SHUTDOWN_PARAMS pShutdownParams)
{
    WCHAR szBuffer[10];
    INT iSeconds, iMinutes, iHours;

    iSeconds = (INT)pShutdownParams->dwTimeout;
    iHours = iSeconds / 3600;
    iSeconds -= iHours * 3600;
    iMinutes = iSeconds / 60;
    iSeconds -= iMinutes * 60;

    swprintf(szBuffer, L"%02d:%02d:%02d", iHours, iMinutes, iSeconds);
    SetDlgItemTextW(hwndDlg, IDC_SHUTDOWNTIMELEFT, szBuffer);

    if (pShutdownParams->dwTimeout == 0)
    {
        PostMessage(hwndDlg, WM_CLOSE, 0, 0);
        ExitWindowsEx((pShutdownParams->bRebootAfterShutdown ? EWX_REBOOT : EWX_SHUTDOWN) |
                      (pShutdownParams->bForceAppsClosed ? EWX_FORCE : 0),
                      pShutdownParams->dwReason);
    }

    pShutdownParams->dwTimeout--;
}


static
INT_PTR
CALLBACK
ShutdownDialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSYS_SHUTDOWN_PARAMS pShutdownParams;

    pShutdownParams = (PSYS_SHUTDOWN_PARAMS)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pShutdownParams = (PSYS_SHUTDOWN_PARAMS)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pShutdownParams);

            pShutdownParams->hShutdownDialog = hwndDlg;

            if (pShutdownParams->pszMessage)
            {
                SetDlgItemTextW(hwndDlg,
                                IDC_SHUTDOWNCOMMENT,
                                pShutdownParams->pszMessage);
            }

            RemoveMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
            SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

            PostMessage(hwndDlg, WM_TIMER, 0, 0);
            SetTimer(hwndDlg, SHUTDOWN_TIMER_ID, 1000, NULL);
            break;

        case WM_CLOSE:
            pShutdownParams->hShutdownDialog = NULL;
            pShutdownParams->bShuttingDown = FALSE;

            KillTimer(hwndDlg, SHUTDOWN_TIMER_ID);

            if (pShutdownParams->pszMessage)
            {
                HeapFree(GetProcessHeap(), 0, pShutdownParams->pszMessage);
                pShutdownParams->pszMessage = NULL;
            }

            EndDialog(hwndDlg, 0);
            break;

        case WM_TIMER:
            OnTimer(hwndDlg, pShutdownParams);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


static
DWORD
WINAPI
InitiateSystemShutdownThread(
    LPVOID lpParameter)
{
    PSYS_SHUTDOWN_PARAMS pShutdownParams;
    INT_PTR status;

    pShutdownParams = (PSYS_SHUTDOWN_PARAMS)lpParameter;

    status = DialogBoxParamW(hAppInstance,
                             MAKEINTRESOURCEW(IDD_SYSSHUTDOWN),
                             NULL,
                             ShutdownDialogProc,
                             (LPARAM)pShutdownParams);
    if (status >= 0)
    {
        return ERROR_SUCCESS;
    }

    if (pShutdownParams->pszMessage)
    {
        HeapFree(GetProcessHeap(), 0, pShutdownParams->pszMessage);
        pShutdownParams->pszMessage = NULL;
    }

    pShutdownParams->bShuttingDown = FALSE;

    return GetLastError();
}


DWORD
TerminateSystemShutdown(VOID)
{
    if (g_ShutdownParams.bShuttingDown == FALSE)
        return ERROR_NO_SHUTDOWN_IN_PROGRESS;

    return PostMessage(g_ShutdownParams.hShutdownDialog, WM_CLOSE, 0, 0) ? ERROR_SUCCESS : GetLastError();
}


DWORD
StartSystemShutdown(
    PUNICODE_STRING lpMessage,
    ULONG dwTimeout,
    BOOLEAN bForceAppsClosed,
    BOOLEAN bRebootAfterShutdown,
    ULONG dwReason)
{
    HANDLE hThread;

    if (_InterlockedCompareExchange8((volatile char*)&g_ShutdownParams.bShuttingDown, TRUE, FALSE) == TRUE)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    if (lpMessage && lpMessage->Length && lpMessage->Buffer)
    {
        g_ShutdownParams.pszMessage = HeapAlloc(GetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                lpMessage->Length + sizeof(UNICODE_NULL));
        if (g_ShutdownParams.pszMessage == NULL)
        {
            g_ShutdownParams.bShuttingDown = FALSE;
            return GetLastError();
        }

        wcsncpy(g_ShutdownParams.pszMessage,
                lpMessage->Buffer,
                lpMessage->Length / sizeof(WCHAR));
    }
    else
    {
        g_ShutdownParams.pszMessage = NULL;
    }

    g_ShutdownParams.dwTimeout = dwTimeout;
    g_ShutdownParams.bForceAppsClosed = bForceAppsClosed;
    g_ShutdownParams.bRebootAfterShutdown = bRebootAfterShutdown;
    g_ShutdownParams.dwReason = dwReason;

    hThread = CreateThread(NULL, 0, InitiateSystemShutdownThread, (PVOID)&g_ShutdownParams, 0, NULL);
    if (hThread)
    {
        CloseHandle(hThread);
        return ERROR_SUCCESS;
    }

    if (g_ShutdownParams.pszMessage)
    {
        HeapFree(GetProcessHeap(), 0, g_ShutdownParams.pszMessage);
        g_ShutdownParams.pszMessage = NULL;
    }

    g_ShutdownParams.bShuttingDown = FALSE;

    return GetLastError();
}

/* EOF */
