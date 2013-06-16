/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

WINE_DEFAULT_DEBUG_CHANNEL(msgina);

typedef struct _DISPLAYSTATUSMSG
{
    PGINA_CONTEXT Context;
    HDESK hDesktop;
    DWORD dwOptions;
    PWSTR pTitle;
    PWSTR pMessage;
    HANDLE StartupEvent;
} DISPLAYSTATUSMSG, *PDISPLAYSTATUSMSG;

static BOOL
GUIInitialize(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUIInitialize(%p)\n", pgContext);
    return TRUE;
}

static INT_PTR CALLBACK
StatusMessageWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lParam;
            if (!msg)
                return FALSE;

            msg->Context->hStatusWindow = hwndDlg;

            if (msg->pTitle)
                SetWindowTextW(hwndDlg, msg->pTitle);
            SetDlgItemTextW(hwndDlg, IDC_STATUSLABEL, msg->pMessage);
            SetEvent(msg->StartupEvent);
            return TRUE;
        }
    }
    return FALSE;
}

static DWORD WINAPI
StartupWindowThread(LPVOID lpParam)
{
    HDESK hDesk;
    PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lpParam;

    /* When SetThreadDesktop is called the system closes the desktop handle when needed
       so we have to create a new handle because this handle may still be in use by winlogon  */
    if (!DuplicateHandle (  GetCurrentProcess(), 
                            msg->hDesktop, 
                            GetCurrentProcess(), 
                            (HANDLE*)&hDesk, 
                            0, 
                            FALSE, 
                            DUPLICATE_SAME_ACCESS))
    {
        HeapFree(GetProcessHeap(), 0, lpParam);
        return FALSE;
    }

    if(!SetThreadDesktop(hDesk))
    {
        HeapFree(GetProcessHeap(), 0, lpParam);
        return FALSE;
    }

    DialogBoxParam(
        hDllInstance,
        MAKEINTRESOURCE(IDD_STATUSWINDOW_DLG),
        GetDesktopWindow(),
        StatusMessageWindowProc,
        (LPARAM)lpParam);

    HeapFree(GetProcessHeap(), 0, lpParam);
    return TRUE;
}

static BOOL
GUIDisplayStatusMessage(
    IN PGINA_CONTEXT pgContext,
    IN HDESK hDesktop,
    IN DWORD dwOptions,
    IN PWSTR pTitle,
    IN PWSTR pMessage)
{
    PDISPLAYSTATUSMSG msg;
    HANDLE Thread;
    DWORD ThreadId;

    TRACE("GUIDisplayStatusMessage(%ws)\n", pMessage);

    if (!pgContext->hStatusWindow)
    {
        msg = (PDISPLAYSTATUSMSG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DISPLAYSTATUSMSG));
        if(!msg)
            return FALSE;

        msg->Context = pgContext;
        msg->dwOptions = dwOptions;
        msg->pTitle = pTitle;
        msg->pMessage = pMessage;
        msg->hDesktop = hDesktop;

        msg->StartupEvent = CreateEventW(
            NULL,
            TRUE,
            FALSE,
            NULL);

        if (!msg->StartupEvent)
            return FALSE;

        Thread = CreateThread(
            NULL,
            0,
            StartupWindowThread,
            (PVOID)msg,
            0,
            &ThreadId);
        if (Thread)
        {
            CloseHandle(Thread);
            WaitForSingleObject(msg->StartupEvent, INFINITE);
            CloseHandle(msg->StartupEvent);
            return TRUE;
        }

        return FALSE;
    }

    if (pTitle)
        SetWindowTextW(pgContext->hStatusWindow, pTitle);

    SetDlgItemTextW(pgContext->hStatusWindow, IDC_STATUSLABEL, pMessage);

    return TRUE;
}

static BOOL
GUIRemoveStatusMessage(
    IN PGINA_CONTEXT pgContext)
{
    if (pgContext->hStatusWindow)
    {
        EndDialog(pgContext->hStatusWindow, 0);
        pgContext->hStatusWindow = NULL;
    }

    return TRUE;
}

static INT_PTR CALLBACK
EmptyWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hwndDlg);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return FALSE;
}

static VOID
GUIDisplaySASNotice(
    IN OUT PGINA_CONTEXT pgContext)
{
    INT result;

    TRACE("GUIDisplaySASNotice()\n");

    /* Display the notice window */
    result = DialogBoxParam(
        pgContext->hDllInstance,
        MAKEINTRESOURCE(IDD_NOTICE_DLG),
        GetDesktopWindow(),
        EmptyWindowProc,
        (LPARAM)NULL);
    if (result == -1)
    {
        /* Failed to display the window. Do as if the user
         * already has pressed CTRL+ALT+DELETE */
        pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
    }
}

/* Get the text contained in a textbox. Allocates memory in pText
 * to contain the text. Returns TRUE in case of success */
static BOOL
GetTextboxText(
    IN HWND hwndDlg,
    IN INT TextboxId,
    OUT LPWSTR *pText)
{
    LPWSTR Text;
    int Count;

    Count = GetWindowTextLength(GetDlgItem(hwndDlg, TextboxId));
    Text = HeapAlloc(GetProcessHeap(), 0, (Count + 1) * sizeof(WCHAR));
    if (!Text)
        return FALSE;
    if (Count != GetWindowTextW(GetDlgItem(hwndDlg, TextboxId), Text, Count + 1))
    {
        HeapFree(GetProcessHeap(), 0, Text);
        return FALSE;
    }
    *pText = Text;
    return TRUE;
}

static VOID
OnInitSecurityDlg(HWND hwnd,
                  PGINA_CONTEXT pgContext)
{
    WCHAR Buffer1[256];
    WCHAR Buffer2[256];
    WCHAR Buffer3[256];
    WCHAR Buffer4[512];

    LoadStringW(pgContext->hDllInstance, IDS_LOGONMSG, Buffer1, 256);

    wsprintfW(Buffer2, L"%s\\%s", pgContext->Domain, pgContext->UserName);
    wsprintfW(Buffer4, Buffer1, Buffer2);

    SetDlgItemTextW(hwnd, IDC_LOGONMSG, Buffer4);

    LoadStringW(pgContext->hDllInstance, IDS_LOGONDATE, Buffer1, 256);

    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                   (SYSTEMTIME*)&pgContext->LogonTime, NULL, Buffer2, 256);

    GetTimeFormatW(LOCALE_USER_DEFAULT, 0,
                   (SYSTEMTIME*)&pgContext->LogonTime, NULL, Buffer3, 256);

    wsprintfW(Buffer4, Buffer1, Buffer2, Buffer3);

    SetDlgItemTextW(hwnd, IDC_LOGONDATE, Buffer4);
}

static INT_PTR CALLBACK
LoggedOnWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            OnInitSecurityDlg(hwndDlg, (PGINA_CONTEXT)lParam);
            SetFocus(GetDlgItem(hwndDlg, IDNO));
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_LOCK:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_LOCK_WKSTA);
                    return TRUE;
                case IDC_LOGOFF:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_LOGOFF);
                    return TRUE;
                case IDC_SHUTDOWN:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
                    return TRUE;
                case IDC_TASKMGR:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_TASKLIST);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;
            }
            break;
        }
        case WM_CLOSE:
        {
            EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
            return TRUE;
        }
    }

    return FALSE;
}

static INT
GUILoggedOnSAS(
    IN OUT PGINA_CONTEXT pgContext,
    IN DWORD dwSasType)
{
    INT result;

    TRACE("GUILoggedOnSAS()\n");

    if (dwSasType != WLX_SAS_TYPE_CTRL_ALT_DEL)
    {
        /* Nothing to do for WLX_SAS_TYPE_TIMEOUT ; the dialog will
         * close itself thanks to the use of WlxDialogBoxParam */
        return WLX_SAS_ACTION_NONE;
    }

    result = pgContext->pWlxFuncs->WlxSwitchDesktopToWinlogon(
        pgContext->hWlx);

    result = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOGGEDON_DLG),
        GetDesktopWindow(),
        LoggedOnWindowProc,
        (LPARAM)pgContext);

    if (result < WLX_SAS_ACTION_LOGON ||
        result > WLX_SAS_ACTION_SWITCH_CONSOLE)
    {
        result = WLX_SAS_ACTION_NONE;
    }

    if (result == WLX_SAS_ACTION_NONE)
    {
        result = pgContext->pWlxFuncs->WlxSwitchDesktopToUser(
            pgContext->hWlx);
    }

    return result;
}

static INT_PTR CALLBACK
LoggedOutWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* FIXME: take care of DontDisplayLastUserName, NoDomainUI, ShutdownWithoutLogon */
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);
            SetFocus(GetDlgItem(hwndDlg, IDC_USERNAME));

            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            return TRUE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext->hBitmap)
            {
                hdc = BeginPaint(hwndDlg, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            DeleteObject(pgContext->hBitmap);
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    LPWSTR UserName = NULL, Password = NULL;
                    INT result = WLX_SAS_ACTION_NONE;

                    if (GetTextboxText(hwndDlg, IDC_USERNAME, &UserName) && *UserName == '\0')
                        break;
                    if (GetTextboxText(hwndDlg, IDC_PASSWORD, &Password) &&
                        DoLoginTasks(pgContext, UserName, NULL, Password))
                    {
                        result = WLX_SAS_ACTION_LOGON;
                    }
                    HeapFree(GetProcessHeap(), 0, UserName);
                    HeapFree(GetProcessHeap(), 0, Password);
                    EndDialog(hwndDlg, result);
                    return TRUE;
                }
                case IDCANCEL:
                {
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;
                }
                case IDC_SHUTDOWN:
                {
                    EndDialog(hwndDlg, WLX_SAS_ACTION_SHUTDOWN);
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

static INT
GUILoggedOutSAS(
    IN OUT PGINA_CONTEXT pgContext)
{
    int result;

    TRACE("GUILoggedOutSAS()\n");

    result = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOGGEDOUT_DLG),
        GetDesktopWindow(),
        LoggedOutWindowProc,
        (LPARAM)pgContext);
    if (result >= WLX_SAS_ACTION_LOGON &&
        result <= WLX_SAS_ACTION_SWITCH_CONSOLE)
    {
        WARN("WlxLoggedOutSAS() returns 0x%x\n", result);
        return result;
    }

    WARN("WlxDialogBoxParam() failed (0x%x)\n", result);
    return WLX_SAS_ACTION_NONE;
}

static INT
GUILockedSAS(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUILockedSAS()\n");

    UNREFERENCED_PARAMETER(pgContext);

    UNIMPLEMENTED;
    return WLX_SAS_ACTION_UNLOCK_WKSTA;
}


static VOID
OnInitLockedDlg(HWND hwnd,
                PGINA_CONTEXT pgContext)
{
    WCHAR Buffer1[256];
    WCHAR Buffer2[256];
    WCHAR Buffer3[512];

    LoadStringW(pgContext->hDllInstance, IDS_LOCKMSG, Buffer1, 256);

    wsprintfW(Buffer2, L"%s\\%s", pgContext->Domain, pgContext->UserName);
    wsprintfW(Buffer3, Buffer1, Buffer2);

    SetDlgItemTextW(hwnd, IDC_LOCKMSG, Buffer3);
}


static INT_PTR CALLBACK
LockedWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            OnInitLockedDlg(hwndDlg, pgContext);
            return TRUE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext->hBitmap)
            {
                hdc = BeginPaint(hwndDlg, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            DeleteObject(pgContext->hBitmap);
            return TRUE;
        }
    }

    return FALSE;
}


static VOID
GUIDisplayLockedNotice(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUIdisplayLockedNotice()\n");

    pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOCKED_DLG),
        GetDesktopWindow(),
        LockedWindowProc,
        (LPARAM)pgContext);
}

GINA_UI GinaGraphicalUI = {
    GUIInitialize,
    GUIDisplayStatusMessage,
    GUIRemoveStatusMessage,
    GUIDisplaySASNotice,
    GUILoggedOnSAS,
    GUILoggedOutSAS,
    GUILockedSAS,
    GUIDisplayLockedNotice,
};
