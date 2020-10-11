/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/shutdown.c
 * PURPOSE:         System shutdown dialog
 * PROGRAMMERS:     alpha5056 <alpha5056@users.noreply.github.com>
 *                  Hermes Belusca-Maito
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

    HDESK hShutdownDesk;
    WCHAR DesktopName[512];
    WINDOWPLACEMENT wpPos;

    BOOLEAN bShuttingDown;
    BOOLEAN bRebootAfterShutdown;
    BOOLEAN bForceAppsClosed;
    DWORD dwReason;
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
    HDESK hInputDesktop;
    BOOL bSuccess;
    DWORD dwSize;
    INT iSeconds, iMinutes, iHours, iDays;
    WCHAR szFormatBuffer[32];
    WCHAR szBuffer[32];
    WCHAR DesktopName[512];

    if (!pShutdownParams->bShuttingDown)
    {
        /* Shutdown has been cancelled, close the dialog and bail out */
        EndDialog(hwndDlg, IDABORT);
        return;
    }

    /*
     * Check whether the input desktop has changed. If so, close the dialog,
     * and let the shutdown thread recreate it on the new desktop.
     */

    // TODO: Investigate: It would be great if we could also compare with
    // our internally maintained desktop handles, before calling that heavy
    // comparison.
    // (Note that we cannot compare handles with arbitrary input desktop,
    // since OpenInputDesktop() creates new handle instances everytime.)

    hInputDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (!hInputDesktop)
    {
        /* No input desktop but we have a dialog: kill it */
        ERR("OpenInputDesktop() failed, error 0x%lx\n", GetLastError());
        EndDialog(hwndDlg, 0);
        return;
    }
    bSuccess = GetUserObjectInformationW(hInputDesktop,
                                         UOI_NAME,
                                         DesktopName,
                                         sizeof(DesktopName),
                                         &dwSize);
    if (!bSuccess)
    {
        ERR("GetUserObjectInformationW(0x%p) failed, error 0x%lx\n",
            hInputDesktop, GetLastError());
    }
    CloseDesktop(hInputDesktop);

    if (bSuccess && (wcscmp(DesktopName, pShutdownParams->DesktopName) != 0))
    {
        TRACE("Input desktop has changed: '%S' --> '%S'\n",
              pShutdownParams->DesktopName, DesktopName);

        /* Save the original dialog position to be restored later */
        pShutdownParams->wpPos.length = sizeof(pShutdownParams->wpPos);
        GetWindowPlacement(hwndDlg, &pShutdownParams->wpPos);

        /* Close the dialog */
        EndDialog(hwndDlg, IDCANCEL);
        return;
    }

    /* Update the shutdown timeout */
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
        /* Close the dialog and let the shutdown thread perform the system shutdown */
        EndDialog(hwndDlg, 0);
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

    pShutdownParams = (PSYS_SHUTDOWN_PARAMS)GetWindowLongPtrW(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pShutdownParams = (PSYS_SHUTDOWN_PARAMS)lParam;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)pShutdownParams);

            /* Display the shutdown message */
            if (pShutdownParams->pszMessage)
            {
                SetDlgItemTextW(hwndDlg,
                                IDC_SYSSHUTDOWNMESSAGE,
                                pShutdownParams->pszMessage);
            }

            /* Remove the Close menu item */
            DeleteMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);

            /* Position the window (initial position, or restore from old) */
            if (pShutdownParams->wpPos.length == sizeof(pShutdownParams->wpPos))
                SetWindowPlacement(hwndDlg, &pShutdownParams->wpPos);

            SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);

            /* Initialize the timer */
            PostMessageW(hwndDlg, WM_TIMER, 0, 0);
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
    HDESK hInputDesktop;
    DWORD dwSize;
    INT_PTR res;

    pShutdownParams = (PSYS_SHUTDOWN_PARAMS)lpParameter;

    /* Default to initial dialog position */
    pShutdownParams->wpPos.length = 0;

    /* Continuously display the shutdown dialog on the current input desktop */
    while (TRUE)
    {
        /* Retrieve the current input desktop */
        hInputDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
        if (!hInputDesktop)
        {
            /* No input desktop on the current WinSta0, just shut down */
            ERR("OpenInputDesktop() failed, error 0x%lx\n", GetLastError());
            break;
        }

        /* Remember it for checking desktop changes later */
        pShutdownParams->hShutdownDesk = hInputDesktop;
        if (!GetUserObjectInformationW(pShutdownParams->hShutdownDesk,
                                       UOI_NAME,
                                       pShutdownParams->DesktopName,
                                       sizeof(pShutdownParams->DesktopName),
                                       &dwSize))
        {
            ERR("GetUserObjectInformationW(0x%p) failed, error 0x%lx\n",
                pShutdownParams->hShutdownDesk, GetLastError());
        }

        /* Assign the desktop to the current thread */
        SetThreadDesktop(hInputDesktop);

        /* Display the shutdown dialog on the current input desktop */
        res = DialogBoxParamW(hAppInstance,
                              MAKEINTRESOURCEW(IDD_SYSSHUTDOWN),
                              NULL,
                              ShutdownDialogProc,
                              (LPARAM)pShutdownParams);

        /* Close the desktop */
        CloseDesktop(hInputDesktop);

        /*
         * Check why the dialog has been closed.
         *
         * - If it failed to be created (returned -1), don't care about
         *   re-creating it, and proceed directly to shutdown.
         *
         * - If it closed unexpectedly (returned != 1), check whether a
         *   shutdown is in progress. If the shutdown has been cancelled,
         *   just bail out; if a shutdown is in progress and the timeout
         *   is 0, bail out and proceed to shutdown.
         *
         * - If the dialog has closed because the input desktop changed,
         *   loop again and recreate it on the new desktop.
         */
        if ((res == -1) || (res != IDCANCEL) ||
            !(pShutdownParams->bShuttingDown && (pShutdownParams->dwTimeout > 0)))
        {
            break;
        }
    }

    /* Reset dialog information */
    pShutdownParams->hShutdownDesk = NULL;
    ZeroMemory(&pShutdownParams->DesktopName, sizeof(pShutdownParams->DesktopName));
    ZeroMemory(&pShutdownParams->wpPos, sizeof(pShutdownParams->wpPos));

    if (pShutdownParams->pszMessage)
    {
        HeapFree(GetProcessHeap(), 0, pShutdownParams->pszMessage);
        pShutdownParams->pszMessage = NULL;
    }

    if (pShutdownParams->bShuttingDown)
    {
        /* Perform the system shutdown */
        if (DoSystemShutdown(pShutdownParams))
            return ERROR_SUCCESS;
        else
            return GetLastError();
    }

    pShutdownParams->bShuttingDown = FALSE;
    return ERROR_SUCCESS;
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

    if ((dwTimeout != 0) && lpMessage && lpMessage->Length && lpMessage->Buffer)
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
