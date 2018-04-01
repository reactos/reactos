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
} SYS_SHUTDOWN_PARAMS, *PSYS_SHUTDOWN_PARAMS;

/* GLOBALS *******************************************************************/

HWND g_hShutdownDialog = NULL;
BOOLEAN g_bShuttingDown = FALSE;
SYS_SHUTDOWN_PARAMS g_ShutdownParams;

/* FUNCTIONS *****************************************************************/

static
INT_PTR
CALLBACK
ShutdownDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            g_hShutdownDialog = hwndDlg;
            if (g_ShutdownParams.usMessage.Length)
            {
                SetDlgItemTextW(hwndDlg,
                                IDC_SHUTDOWNCOMMENT,
                                g_ShutdownParams.usMessage.Buffer);
            }
            RemoveMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
            SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
            PostMessage(hwndDlg, WM_TIMER, 0, 0);
            SetTimer(hwndDlg, IDT_SYSSHUTDOWN, 1000, NULL);
        break;
        }
        case WM_CLOSE:
        {
            g_hShutdownDialog = NULL;
            g_bShuttingDown = FALSE;
            KillTimer(hwndDlg, IDT_SYSSHUTDOWN);
            if (g_ShutdownParams.usMessage.Buffer)
            {
                HeapFree(GetProcessHeap(), 0, g_ShutdownParams.usMessage.Buffer);
                RtlInitEmptyUnicodeString(&g_ShutdownParams.usMessage, NULL, 0);
            }
            EndDialog(hwndDlg, 0);
            DestroyWindow(hwndDlg);
        break;
        }
        case WM_TIMER:
        {
            WCHAR strbuf[34];
            int seconds, minutes, hours;
            seconds = (int)(g_ShutdownParams.dwTimeout);
            hours = seconds/3600;
            seconds -= hours*3600;
            minutes = seconds/60;
            seconds -= minutes*60;
            ZeroMemory(strbuf, sizeof(strbuf));
            //FIXME: Show time remaining according to the locale's format
            RtlStringCbPrintfW(strbuf, sizeof(strbuf), L"%d:%d:%d", hours, minutes, seconds);
            SetDlgItemTextW(hwndDlg, IDC_SHUTDOWNTIMELEFT, strbuf);
            if (g_ShutdownParams.dwTimeout == 0)
            {
                PostMessage(hwndDlg, WM_CLOSE, 0, 0);
                ExitWindowsEx((g_ShutdownParams.bRebootAfterShutdown ? EWX_REBOOT : EWX_SHUTDOWN) |
                              (g_ShutdownParams.bForceAppsClosed ? EWX_FORCE : 0),
                              g_ShutdownParams.dwReason);
            }
            g_ShutdownParams.dwTimeout--;
        break;
        }
        default:
            return FALSE;
    }
    return TRUE;
}


static
DWORD
WINAPI
InitiateSystemShutdownThread(LPVOID lpParameter)
{
    INT_PTR status;
    status = DialogBoxW(hAppInstance, MAKEINTRESOURCEW(IDD_SYSSHUTDOWN),
                        NULL, ShutdownDialogProc);
    if (status >= 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        if (g_ShutdownParams.usMessage.Buffer)
        {
            HeapFree(GetProcessHeap(), 0, g_ShutdownParams.usMessage.Buffer);
            RtlInitEmptyUnicodeString(&g_ShutdownParams.usMessage, NULL, 0);
        }
        g_bShuttingDown = FALSE;
        return GetLastError();
    }
}


DWORD
TerminateSystemShutdown(VOID)
{
    if (g_bShuttingDown == FALSE)
        return ERROR_NO_SHUTDOWN_IN_PROGRESS;
    return PostMessage(g_hShutdownDialog, WM_CLOSE, 0, 0) ? ERROR_SUCCESS : GetLastError();
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

    if (_InterlockedCompareExchange8((volatile char*)&g_bShuttingDown, TRUE, FALSE) == TRUE)
        return ERROR_SHUTDOWN_IN_PROGRESS;
    if (lpMessage && lpMessage->Length && lpMessage->Buffer)
    {
        g_ShutdownParams.usMessage.Buffer = HeapAlloc(GetProcessHeap(), 0, lpMessage->Length+sizeof(UNICODE_NULL));
        if (g_ShutdownParams.usMessage.Buffer == NULL)
        {
            g_bShuttingDown = FALSE;
            return GetLastError();
        }
        RtlInitEmptyUnicodeString(&g_ShutdownParams.usMessage, g_ShutdownParams.usMessage.Buffer, lpMessage->Length+sizeof(UNICODE_NULL));
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
    hThread = CreateThread(NULL, 0, InitiateSystemShutdownThread, NULL, 0, NULL);
    if (hThread)
    {
        CloseHandle(hThread);
        return ERROR_SUCCESS;
    }
    else
    {
        if (g_ShutdownParams.usMessage.Buffer)
        {
            HeapFree(GetProcessHeap(), 0, g_ShutdownParams.usMessage.Buffer);
            RtlInitEmptyUnicodeString(&g_ShutdownParams.usMessage, NULL, 0);
        }
        g_bShuttingDown = FALSE;
        return GetLastError();
    }
}

/* EOF */
