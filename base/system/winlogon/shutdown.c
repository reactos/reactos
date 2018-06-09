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
#define SECONDS_PER_DAY 86400
#define SECONDS_PER_DECADE 315360000


/* STRUCTS *******************************************************************/

typedef struct _SYS_SHUTDOWN_PARAMS
{
    PWSTR pszMessage;
    ULONG dwTimeout;
    BOOLEAN bRebootAfterShutdown;
    BOOLEAN bForceAppsClosed;
    DWORD dwReason;

    BOOLEAN bShuttingDown;
} SYS_SHUTDOWN_PARAMS, *PSYS_SHUTDOWN_PARAMS;


/* GLOBALS *******************************************************************/

SYS_SHUTDOWN_PARAMS g_ShutdownParams;


/* FUNCTIONS *****************************************************************/

static
BOOL
DoSystemShutdown(
    IN PSYS_SHUTDOWN_PARAMS pShutdownParams)
{
    BOOL Success;

    if (pShutdownParams->pszMessage)
    {
        HeapFree(GetProcessHeap(), 0, pShutdownParams->pszMessage);
        pShutdownParams->pszMessage = NULL;
    }

    /* If shutdown has been cancelled, bail out now */
    if (!pShutdownParams->bShuttingDown)
        return TRUE;

    Success = ExitWindowsEx((pShutdownParams->bRebootAfterShutdown ? EWX_REBOOT : EWX_SHUTDOWN) |
                            (pShutdownParams->bForceAppsClosed ? EWX_FORCE : 0),
                             pShutdownParams->dwReason);
    if (!Success)
    {
        /* Something went wrong, cancel shutdown */
        pShutdownParams->bShuttingDown = FALSE;
    }

    return Success;
}


static
VOID
OnTimer(
    HWND hwndDlg,
    PSYS_SHUTDOWN_PARAMS pShutdownParams)
{
    WCHAR szFormatBuffer[32];
    WCHAR szBuffer[32];
    INT iSeconds, iMinutes, iHours, iDays;

    if (!pShutdownParams->bShuttingDown)
    {
        /* Shutdown has been cancelled, close the dialog and bail out */
        EndDialog(hwndDlg, 0);
        return;
    }

    if (pShutdownParams->dwTimeout < SECONDS_PER_DAY)
    {
        iSeconds = (INT)pShutdownParams->dwTimeout;
        iHours = iSeconds / 3600;
        iSeconds -= iHours * 3600;
        iMinutes = iSeconds / 60;
        iSeconds -= iMinutes * 60;

        LoadStringW(hAppInstance, IDS_TIMEOUTSHORTFORMAT, szFormatBuffer, ARRAYSIZE(szFormatBuffer));
        swprintf(szBuffer, szFormatBuffer, iHours, iMinutes, iSeconds);
    }
    else
    {
        iDays = (INT)(pShutdownParams->dwTimeout / SECONDS_PER_DAY);

        LoadStringW(hAppInstance, IDS_TIMEOUTLONGFORMAT, szFormatBuffer, ARRAYSIZE(szFormatBuffer));
        swprintf(szBuffer, szFormatBuffer, iDays);
    }

    SetDlgItemTextW(hwndDlg, IDC_SYSSHUTDOWNTIMELEFT, szBuffer);

    if (pShutdownParams->dwTimeout == 0)
    {
        /* Close the dialog and perform the system shutdown */
        EndDialog(hwndDlg, 0);
        DoSystemShutdown(pShutdownParams);
        return;
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
        {
            pShutdownParams = (PSYS_SHUTDOWN_PARAMS)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pShutdownParams);

            if (pShutdownParams->pszMessage)
            {
                SetDlgItemTextW(hwndDlg,
                                IDC_SYSSHUTDOWNMESSAGE,
                                pShutdownParams->pszMessage);
            }

            DeleteMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
            SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

            PostMessage(hwndDlg, WM_TIMER, 0, 0);
            SetTimer(hwndDlg, SHUTDOWN_TIMER_ID, 1000, NULL);
            break;
        }

        /* NOTE: Do not handle WM_CLOSE */
        case WM_DESTROY:
            KillTimer(hwndDlg, SHUTDOWN_TIMER_ID);
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

    if (pShutdownParams->pszMessage)
    {
        HeapFree(GetProcessHeap(), 0, pShutdownParams->pszMessage);
        pShutdownParams->pszMessage = NULL;
    }

    if (status >= 0)
        return ERROR_SUCCESS;

    pShutdownParams->bShuttingDown = FALSE;
    return GetLastError();
}


DWORD
TerminateSystemShutdown(VOID)
{
    if (_InterlockedCompareExchange8((volatile char*)&g_ShutdownParams.bShuttingDown, FALSE, TRUE) == FALSE)
        return ERROR_NO_SHUTDOWN_IN_PROGRESS;

    return ERROR_SUCCESS;
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

    /* Fail if the timeout is 10 years or more */
    if (dwTimeout >= SECONDS_PER_DECADE)
        return ERROR_INVALID_PARAMETER;

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

    /* If dwTimeout is zero perform an immediate system shutdown, otherwise display the countdown shutdown dialog */
    if (g_ShutdownParams.dwTimeout == 0)
    {
        if (DoSystemShutdown(&g_ShutdownParams))
            return ERROR_SUCCESS;
    }
    else
    {
        hThread = CreateThread(NULL, 0, InitiateSystemShutdownThread, (PVOID)&g_ShutdownParams, 0, NULL);
        if (hThread)
        {
            CloseHandle(hThread);
            return ERROR_SUCCESS;
        }
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
