/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/shutdown.c
 * PURPOSE:         System shutdown dialog
 * PROGRAMMERS:     alpha5056 <alpha5056@users.noreply.github.com>
 */

/* INCLUDES ******************************************************************/

#include "winlogon.h"

#include <ntstrsafe.h>
#include <rpc.h>
#include <winreg_s.h>

/* DEFINES *******************************************************************/

#define IDT_SYSSHUTDOWN 2000

/* STRUCTS *******************************************************************/

typedef struct _SYS_SHUTDOWN_PARAMS
{
    UNICODE_STRING usMessage;
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
    WCHAR strbuf[34];
    INT seconds, minutes, hours;

    seconds = (INT)pShutdownParams->dwTimeout;
    hours = seconds / 3600;
    seconds -= hours * 3600;
    minutes = seconds / 60;
    seconds -= minutes * 60;

    RtlStringCbPrintfW(strbuf, sizeof(strbuf), L"%d:%d:%d", hours, minutes, seconds);
    SetDlgItemTextW(hwndDlg, IDC_SHUTDOWNTIMELEFT, strbuf);

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

            if (pShutdownParams->usMessage.Length)
            {
                SetDlgItemTextW(hwndDlg,
                                IDC_SHUTDOWNCOMMENT,
                                pShutdownParams->usMessage.Buffer);
            }
            RemoveMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
            SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
            PostMessage(hwndDlg, WM_TIMER, 0, 0);
            SetTimer(hwndDlg, IDT_SYSSHUTDOWN, 1000, NULL);
            break;

        case WM_CLOSE:
            pShutdownParams->hShutdownDialog = NULL;
            pShutdownParams->bShuttingDown = FALSE;

            KillTimer(hwndDlg, IDT_SYSSHUTDOWN);

            if (pShutdownParams->usMessage.Buffer)
            {
                HeapFree(GetProcessHeap(), 0, pShutdownParams->usMessage.Buffer);
                RtlInitEmptyUnicodeString(&pShutdownParams->usMessage, NULL, 0);
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

    if (pShutdownParams->usMessage.Buffer)
    {
        HeapFree(GetProcessHeap(), 0, pShutdownParams->usMessage.Buffer);
        RtlInitEmptyUnicodeString(&pShutdownParams->usMessage, NULL, 0);
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
        g_ShutdownParams.usMessage.Buffer = HeapAlloc(GetProcessHeap(), 0, lpMessage->Length + sizeof(UNICODE_NULL));
        if (g_ShutdownParams.usMessage.Buffer == NULL)
        {
            g_ShutdownParams.bShuttingDown = FALSE;
            return GetLastError();
        }

        RtlInitEmptyUnicodeString(&g_ShutdownParams.usMessage, g_ShutdownParams.usMessage.Buffer, lpMessage->Length + sizeof(UNICODE_NULL));
        RtlCopyUnicodeString(&(g_ShutdownParams.usMessage), (PUNICODE_STRING)lpMessage);
    }
    else
    {
        RtlInitEmptyUnicodeString(&g_ShutdownParams.usMessage, NULL, 0);
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

    if (g_ShutdownParams.usMessage.Buffer)
    {
        HeapFree(GetProcessHeap(), 0, g_ShutdownParams.usMessage.Buffer);
        RtlInitEmptyUnicodeString(&g_ShutdownParams.usMessage, NULL, 0);
    }

    g_ShutdownParams.bShuttingDown = FALSE;

    return GetLastError();
}

/* EOF */
