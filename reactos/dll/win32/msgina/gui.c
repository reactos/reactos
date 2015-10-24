/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

#include <wingdi.h>
#include <winnls.h>
#include <winreg.h>

typedef struct _DISPLAYSTATUSMSG
{
    PGINA_CONTEXT Context;
    HDESK hDesktop;
    DWORD dwOptions;
    PWSTR pTitle;
    PWSTR pMessage;
    HANDLE StartupEvent;
} DISPLAYSTATUSMSG, *PDISPLAYSTATUSMSG;

typedef struct _LEGALNOTICEDATA
{
    LPWSTR pszCaption;
    LPWSTR pszText;
} LEGALNOTICEDATA, *PLEGALNOTICEDATA;


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
        ERR("Duplicating handle failed!\n");
        HeapFree(GetProcessHeap(), 0, lpParam);
        return FALSE;
    }

    if(!SetThreadDesktop(hDesk))
    {
        ERR("Setting thread desktop failed!\n");
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
        /*
         * If everything goes correctly, 'msg' is freed
         * by the 'StartupWindowThread' thread.
         */
        msg = (PDISPLAYSTATUSMSG)HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(DISPLAYSTATUSMSG));
        if(!msg)
            return FALSE;

        msg->Context = pgContext;
        msg->dwOptions = dwOptions;
        msg->pTitle = pTitle;
        msg->pMessage = pMessage;
        msg->hDesktop = hDesktop;

        msg->StartupEvent = CreateEventW(NULL,
                                         TRUE,
                                         FALSE,
                                         NULL);

        if (!msg->StartupEvent)
        {
            HeapFree(GetProcessHeap(), 0, msg);
            return FALSE;
        }

        Thread = CreateThread(NULL,
                              0,
                              StartupWindowThread,
                              (PVOID)msg,
                              0,
                              &ThreadId);
        if (Thread)
        {
            /* 'msg' will be freed by 'StartupWindowThread' */

            CloseHandle(Thread);
            WaitForSingleObject(msg->StartupEvent, INFINITE);
            CloseHandle(msg->StartupEvent);
            return TRUE;
        }
        else
        {
            /*
             * The 'StartupWindowThread' thread couldn't be created,
             * so we need to free the allocated 'msg'.
             */
            HeapFree(GetProcessHeap(), 0, msg);
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
    PGINA_CONTEXT pgContext;
    
    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
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
GUIDisplaySASNotice(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUIDisplaySASNotice()\n");

    /* Display the notice window */
    pgContext->pWlxFuncs->WlxDialogBoxParam(pgContext->hWlx,
                                            pgContext->hDllInstance,
                                            MAKEINTRESOURCEW(IDD_NOTICE_DLG),
                                            GetDesktopWindow(),
                                            EmptyWindowProc,
                                            (LPARAM)NULL);
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


static
INT
ResourceMessageBox(
    IN PGINA_CONTEXT pgContext,
    IN HWND hwnd,
    IN UINT uType,
    IN UINT uCaption,
    IN UINT uText)
{
    WCHAR szCaption[256];
    WCHAR szText[256];

    LoadStringW(pgContext->hDllInstance, uCaption, szCaption, 256);
    LoadStringW(pgContext->hDllInstance, uText, szText, 256);

    return pgContext->pWlxFuncs->WlxMessageBox(pgContext->hWlx,
                                               hwnd,
                                               szText,
                                               szCaption,
                                               uType);
}


static
BOOL
DoChangePassword(
    IN PGINA_CONTEXT pgContext,
    IN HWND hwndDlg)
{
    WCHAR UserName[256];
    WCHAR Domain[256];
    WCHAR OldPassword[256];
    WCHAR NewPassword1[256];
    WCHAR NewPassword2[256];
    PMSV1_0_CHANGEPASSWORD_REQUEST RequestBuffer = NULL;
    PMSV1_0_CHANGEPASSWORD_RESPONSE ResponseBuffer = NULL;
    ULONG RequestBufferSize;
    ULONG ResponseBufferSize = 0;
    LPWSTR Ptr;
    BOOL res = FALSE;
    NTSTATUS ProtocolStatus;
    NTSTATUS Status;

    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_USERNAME, UserName, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_DOMAIN, Domain, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_OLDPWD, OldPassword, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD1, NewPassword1, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD2, NewPassword2, 256);

    /* Compare the two passwords and fail if they do not match */
    if (wcscmp(NewPassword1, NewPassword2) != 0)
    {
        ResourceMessageBox(pgContext,
                           hwndDlg,
                           MB_OK | MB_ICONEXCLAMATION,
                           IDS_CHANGEPWDTITLE,
                           IDS_NONMATCHINGPASSWORDS);
        return FALSE;
    }

    /* Calculate the request buffer size */
    RequestBufferSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
                        ((wcslen(Domain) + 1) * sizeof(WCHAR)) +
                        ((wcslen(UserName) + 1) * sizeof(WCHAR)) +
                        ((wcslen(OldPassword) + 1) * sizeof(WCHAR)) +
                        ((wcslen(NewPassword1) + 1) * sizeof(WCHAR));

    /* Allocate the request buffer */
    RequestBuffer = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              RequestBufferSize);
    if (RequestBuffer == NULL)
    {
        ERR("HeapAlloc failed\n");
        return FALSE;
    }

    /* Initialize the request buffer */
    RequestBuffer->MessageType = MsV1_0ChangePassword;
    RequestBuffer->Impersonating = TRUE;

    Ptr = (LPWSTR)((ULONG_PTR)RequestBuffer + sizeof(MSV1_0_CHANGEPASSWORD_REQUEST));

    /* Pack the domain name */
    RequestBuffer->DomainName.Length = wcslen(Domain) * sizeof(WCHAR);
    RequestBuffer->DomainName.MaximumLength = RequestBuffer->DomainName.Length + sizeof(WCHAR);
    RequestBuffer->DomainName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->DomainName.Buffer,
                  Domain,
                  RequestBuffer->DomainName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->DomainName.MaximumLength);

    /* Pack the user name */
    RequestBuffer->AccountName.Length = wcslen(UserName) * sizeof(WCHAR);
    RequestBuffer->AccountName.MaximumLength = RequestBuffer->AccountName.Length + sizeof(WCHAR);
    RequestBuffer->AccountName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->AccountName.Buffer,
                  UserName,
                  RequestBuffer->AccountName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->AccountName.MaximumLength);

    /* Pack the old password */
    RequestBuffer->OldPassword.Length = wcslen(OldPassword) * sizeof(WCHAR);
    RequestBuffer->OldPassword.MaximumLength = RequestBuffer->OldPassword.Length + sizeof(WCHAR);
    RequestBuffer->OldPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->OldPassword.Buffer,
                  OldPassword,
                  RequestBuffer->OldPassword.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->OldPassword.MaximumLength);

    /* Pack the new password */
    RequestBuffer->NewPassword.Length = wcslen(NewPassword1) * sizeof(WCHAR);
    RequestBuffer->NewPassword.MaximumLength = RequestBuffer->NewPassword.Length + sizeof(WCHAR);
    RequestBuffer->NewPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->NewPassword.Buffer,
                  NewPassword1,
                  RequestBuffer->NewPassword.MaximumLength);

    /* Connect to the LSA server */
    if (!ConnectToLsa(pgContext))
    {
        ERR("ConnectToLsa() failed\n");
        goto done;
    }

    /* Call the authentication package */
    Status = LsaCallAuthenticationPackage(pgContext->LsaHandle,
                                          pgContext->AuthenticationPackage,
                                          RequestBuffer,
                                          RequestBufferSize,
                                          (PVOID*)&ResponseBuffer,
                                          &ResponseBufferSize,
                                          &ProtocolStatus);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaCallAuthenticationPackage failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (!NT_SUCCESS(ProtocolStatus))
    {
        TRACE("LsaCallAuthenticationPackage failed (ProtocolStatus 0x%08lx)\n", ProtocolStatus);
        goto done;
    }

    res = TRUE;

    ResourceMessageBox(pgContext,
                       hwndDlg,
                       MB_OK | MB_ICONINFORMATION,
                       IDS_CHANGEPWDTITLE,
                       IDS_PASSWORDCHANGED);

    if ((wcscmp(UserName, pgContext->UserName) == 0) &&
        (wcscmp(Domain, pgContext->Domain) == 0) &&
        (wcscmp(OldPassword, pgContext->Password) == 0))
    {
        ZeroMemory(pgContext->Password, 256 * sizeof(WCHAR));
        wcscpy(pgContext->Password, NewPassword1);
    }

done:
    if (RequestBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, RequestBuffer);

    if (ResponseBuffer != NULL)
        LsaFreeReturnBuffer(ResponseBuffer);

    return res;
}


static INT_PTR CALLBACK
ChangePasswordDialogProc(
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
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            SetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_USERNAME, pgContext->UserName);
            SendDlgItemMessageW(hwndDlg, IDC_CHANGEPWD_DOMAIN, CB_ADDSTRING, 0, (LPARAM)pgContext->Domain);
            SendDlgItemMessageW(hwndDlg, IDC_CHANGEPWD_DOMAIN, CB_SETCURSEL, 0, 0);
            SetFocus(GetDlgItem(hwndDlg, IDC_CHANGEPWD_OLDPWD));
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (DoChangePassword(pgContext, hwndDlg))
                    {
                        EndDialog(hwndDlg, TRUE);
                    }
                    else
                    {
                        SetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD1, NULL);
                        SetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD2, NULL);
                        SetFocus(GetDlgItem(hwndDlg, IDC_CHANGEPWD_OLDPWD));
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, FALSE);
            return TRUE;
    }

    return FALSE;
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

    if (pgContext->bAutoAdminLogon == TRUE)
        EnableWindow(GetDlgItem(hwnd, IDC_LOGOFF), FALSE);
}


static BOOL
OnChangePassword(
    IN HWND hwnd,
    IN PGINA_CONTEXT pgContext)
{
    INT res;

    TRACE("OnChangePassword()\n");

    res = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_CHANGE_PASSWORD),
        hwnd,
        ChangePasswordDialogProc,
        (LPARAM)pgContext);

    TRACE("Result: %x\n", res);

    return FALSE;
}


static INT_PTR CALLBACK
LogOffDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDYES:
                    EndDialog(hwndDlg, IDYES);
                    return TRUE;

                case IDNO:
                    EndDialog(hwndDlg, IDNO);
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, IDNO);
            return TRUE;
    }

    return FALSE;
}


static
INT
OnLogOff(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext)
{
    return pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOGOFF_DLG),
        hwndDlg,
        LogOffDialogProc,
        (LPARAM)pgContext);
}


static
VOID
UpdateShutdownDesc(
    IN HWND hwnd)
{
    WCHAR szBuffer[256];
    UINT shutdownDescId = 0;
    int shutdownCode = 0;

    shutdownCode = SendDlgItemMessageW(hwnd, IDC_SHUTDOWN_LIST, CB_GETCURSEL, 0, 0);

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

        case 3: /* Sleep */
            shutdownDescId = IDS_SHUTDOWN_SLEEP_DESC;
            break;

        case 4: /* Hibernate */
            shutdownDescId = IDS_SHUTDOWN_HIBERNATE_DESC;
            break;

        default:
            break;
    }

    LoadStringW(hDllInstance, shutdownDescId, szBuffer, sizeof(szBuffer));
    SetDlgItemTextW(hwnd, IDC_SHUTDOWN_DESCRIPTION, szBuffer);
}


static
VOID
ShutDownOnInit(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext)
{
    WCHAR szBuffer[256];
    WCHAR szBuffer2[256];
    HWND hwndList;
    INT idx, count, i;

    hwndList = GetDlgItem(hwndDlg, IDC_SHUTDOWN_LIST);

    /* Clears the content before it's used */
    SendMessageW(hwndList, CB_RESETCONTENT, 0, 0);

    /* Log off */
    LoadStringW(hDllInstance, IDS_SHUTDOWN_LOGOFF, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
    wsprintfW(szBuffer2, szBuffer, pgContext->UserName);
    idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer2);
    if (idx != CB_ERR)
        SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_LOGOFF);

    /* Shut down */
    LoadStringW(hDllInstance, IDS_SHUTDOWN_SHUTDOWN, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
    idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    if (idx != CB_ERR)
        SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);

    /* Restart */
    LoadStringW(hDllInstance, IDS_SHUTDOWN_RESTART, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
    idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    if (idx != CB_ERR)
        SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_REBOOT);

    /* Sleep */
#if 0
    LoadStringW(hDllInstance, IDS_SHUTDOWN_SLEEP, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
    idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    if (idx != CB_ERR)
        SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_SLEEP);
#endif

    /* Hibernate */
#if 0
    LoadStringW(hDllInstance, IDS_SHUTDOWN_HIBERNATE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
    idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    if (idx != CB_ERR)
        SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_HIBERNATE);
#endif

    /* Sets the default shut down selection */
    count = SendMessageW(hwndList, CB_GETCOUNT, 0, 0);
    for (i = 0; i < count; i++)
    {
        if (pgContext->nShutdownAction == SendMessageW(hwndList, CB_GETITEMDATA, i, 0))
        {
            SendMessageW(hwndList, CB_SETCURSEL, i, 0);
            break;
        }
    }

    /* Updates the choice description based on the current selection */
    UpdateShutdownDesc(hwndDlg);
}


static
VOID
ShutDownOnOk(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext)
{
    INT idx;

    idx = SendDlgItemMessageW(hwndDlg,
                              IDC_SHUTDOWN_LIST,
                              CB_GETCURSEL,
                              0,
                              0);
    if (idx != CB_ERR)
    {
        pgContext->nShutdownAction = SendDlgItemMessageW(hwndDlg,
                                                         IDC_SHUTDOWN_LIST,
                                                         CB_GETITEMDATA,
                                                         idx,
                                                         0);
    }
}


BOOL
CALLBACK
ShutDownDialogProc(
    HWND hwnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwnd, GWL_USERDATA);

    switch (Message)
    {
        case WM_INITDIALOG:
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwnd, GWL_USERDATA, (INT_PTR)pgContext);

            ShutDownOnInit(hwnd, pgContext);

            /* Draw the logo graphic */
            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            return TRUE;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext->hBitmap)
            {
                hdc = BeginPaint(hwnd, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwnd, &ps);
            }
            return TRUE;
        }

        case WM_DESTROY:
            DeleteObject(pgContext->hBitmap);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    ShutDownOnOk(hwnd, pgContext);
                    EndDialog(hwnd, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;

                case IDC_SHUTDOWN_LIST:
                    UpdateShutdownDesc(hwnd);
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


static
INT
OnShutDown(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext)
{
    return pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_SHUTDOWN_DLG),
        hwndDlg,
        ShutDownDialogProc,
        (LPARAM)pgContext);
}


static INT_PTR CALLBACK
LoggedOnWindowProc(
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
                    if (OnLogOff(hwndDlg, pgContext) == IDYES)
                        EndDialog(hwndDlg, WLX_SAS_ACTION_LOGOFF);
                    return TRUE;
                case IDC_SHUTDOWN:
                    if (OnShutDown(hwndDlg, pgContext) == IDOK)
                        EndDialog(hwndDlg, pgContext->nShutdownAction);
                    return TRUE;
                case IDC_CHANGEPWD:
                    if (OnChangePassword(hwndDlg, pgContext))
                        EndDialog(hwndDlg, WLX_SAS_ACTION_PWD_CHANGED);
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

    pgContext->pWlxFuncs->WlxSwitchDesktopToWinlogon(
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
        pgContext->pWlxFuncs->WlxSwitchDesktopToUser(
            pgContext->hWlx);
    }

    return result;
}


static
BOOL
DoLogon(
    IN HWND hwndDlg,
    IN OUT PGINA_CONTEXT pgContext)
{
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;
    LPWSTR Domain = NULL;
    BOOL result = FALSE;
    NTSTATUS Status, SubStatus = STATUS_SUCCESS;

    if (GetTextboxText(hwndDlg, IDC_USERNAME, &UserName) && *UserName == '\0')
        goto done;

    if (GetTextboxText(hwndDlg, IDC_LOGON_TO, &Domain) && *Domain == '\0')
        goto done;

    if (!GetTextboxText(hwndDlg, IDC_PASSWORD, &Password))
        goto done;

    Status = DoLoginTasks(pgContext, UserName, Domain, Password, &SubStatus);
    if (Status == STATUS_LOGON_FAILURE)
    {
        ResourceMessageBox(pgContext,
                           hwndDlg,
                           MB_OK | MB_ICONEXCLAMATION,
                           IDS_LOGONTITLE,
                           IDS_LOGONWRONGUSERORPWD);
        goto done;
    }
    else if (Status == STATUS_ACCOUNT_RESTRICTION)
    {
        TRACE("DoLoginTasks failed! Status 0x%08lx  SubStatus 0x%08lx\n", Status, SubStatus);

        if (SubStatus == STATUS_ACCOUNT_DISABLED)
        {
            ResourceMessageBox(pgContext,
                               hwndDlg,
                               MB_OK | MB_ICONEXCLAMATION,
                               IDS_LOGONTITLE,
                               IDS_LOGONUSERDISABLED);
            goto done;
        }
        else if (SubStatus == STATUS_ACCOUNT_LOCKED_OUT)
        {
TRACE("Account locked!\n");
            pgContext->pWlxFuncs->WlxMessageBox(pgContext->hWlx,
                                                hwndDlg,
                                                L"Account locked!",
                                                L"Logon error",
                                                MB_OK | MB_ICONERROR);
            goto done;
        }
        else if ((SubStatus == STATUS_PASSWORD_MUST_CHANGE) ||
                 (SubStatus == STATUS_PASSWORD_EXPIRED))
        {
            if (SubStatus == STATUS_PASSWORD_MUST_CHANGE)
                ResourceMessageBox(pgContext,
                                   hwndDlg,
                                   MB_OK | MB_ICONSTOP,
                                   IDS_LOGONTITLE,
                                   IDS_PASSWORDMUSTCHANGE);
            else
                ResourceMessageBox(pgContext,
                                   hwndDlg,
                                   MB_OK | MB_ICONSTOP,
                                   IDS_LOGONTITLE,
                                   IDS_PASSWORDEXPIRED);

            if (!OnChangePassword(hwndDlg,
                                  pgContext))
                goto done;

            Status = DoLoginTasks(pgContext,
                                  pgContext->UserName,
                                  pgContext->Domain,
                                  pgContext->Password,
                                  &SubStatus);
            if (!NT_SUCCESS(Status))
            {
                TRACE("Login after password change failed! (Status 0x%08lx)\n", Status);

                goto done;
            }
        }
        else
        {
TRACE("Other error!\n");
            pgContext->pWlxFuncs->WlxMessageBox(pgContext->hWlx,
                                                hwndDlg,
                                                L"Other error!",
                                                L"Logon error",
                                                MB_OK | MB_ICONERROR);
            goto done;
        }
    }
    else if (!NT_SUCCESS(Status))
    {
TRACE("DoLoginTasks failed! Status 0x%08lx\n", Status);

        goto done;
    }


    if (!CreateProfile(pgContext, UserName, Domain, Password))
    {
        ERR("Failed to create the profile!\n");
        goto done;
    }

    ZeroMemory(pgContext->Password, 256 * sizeof(WCHAR));
    wcscpy(pgContext->Password, Password);

    result = TRUE;

done:
    if (UserName != NULL)
        HeapFree(GetProcessHeap(), 0, UserName);

    if (Password != NULL)
        HeapFree(GetProcessHeap(), 0, Password);

    if (Domain != NULL)
        HeapFree(GetProcessHeap(), 0, Domain);

    return result;
}


static
VOID
SetDomainComboBox(
    HWND hwndDomainComboBox,
    PGINA_CONTEXT pgContext)
{
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwComputerNameLength;
    LONG lIndex = 0;
    LONG lFindIndex;

    SendMessageW(hwndDomainComboBox, CB_RESETCONTENT, 0, 0);

    dwComputerNameLength = sizeof(szComputerName) / sizeof(WCHAR);
    if (GetComputerNameW(szComputerName, &dwComputerNameLength))
    {
        lIndex = SendMessageW(hwndDomainComboBox, CB_ADDSTRING, 0, (LPARAM)szComputerName);
    }

    if (wcslen(pgContext->Domain) != 0)
    {
        lFindIndex = SendMessageW(hwndDomainComboBox, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pgContext->Domain);
        if (lFindIndex == CB_ERR)
        {
            lIndex = SendMessageW(hwndDomainComboBox, CB_ADDSTRING, 0, (LPARAM)pgContext->Domain);
        }
        else
        {
            lIndex = lFindIndex;
        }
    }

    SendMessageW(hwndDomainComboBox, CB_SETCURSEL, lIndex, 0);
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
            /* FIXME: take care of NoDomainUI */
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            if (pgContext->bDontDisplayLastUserName == FALSE)
                SetDlgItemTextW(hwndDlg, IDC_USERNAME, pgContext->UserName);

            if (pgContext->bDisableCAD == TRUE)
                EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);

            if (pgContext->bShutdownWithoutLogon == FALSE)
                EnableWindow(GetDlgItem(hwndDlg, IDC_SHUTDOWN), FALSE);

            SetDomainComboBox(GetDlgItem(hwndDlg, IDC_LOGON_TO), pgContext);

            SetFocus(GetDlgItem(hwndDlg, pgContext->bDontDisplayLastUserName ? IDC_USERNAME : IDC_PASSWORD));

            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            return TRUE;

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
            DeleteObject(pgContext->hBitmap);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (DoLogon(hwndDlg, pgContext))
                        EndDialog(hwndDlg, WLX_SAS_ACTION_LOGON);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;

                case IDC_SHUTDOWN:
                    if (OnShutDown(hwndDlg, pgContext) == IDOK)
                        EndDialog(hwndDlg, pgContext->nShutdownAction);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static
INT_PTR
CALLBACK
LegalNoticeDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PLEGALNOTICEDATA pLegalNotice;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pLegalNotice = (PLEGALNOTICEDATA)lParam;
            SetWindowTextW(hwndDlg, pLegalNotice->pszCaption);
            SetDlgItemTextW(hwndDlg, IDC_LEGALNOTICE_TEXT, pLegalNotice->pszText);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static INT
GUILoggedOutSAS(
    IN OUT PGINA_CONTEXT pgContext)
{
    LEGALNOTICEDATA LegalNotice = {NULL, NULL};
    HKEY hKey = NULL;
    LONG rc;
    int result;

    TRACE("GUILoggedOutSAS()\n");

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                       0,
                       KEY_QUERY_VALUE,
                       &hKey);
    if (rc == ERROR_SUCCESS)
    {
        ReadRegSzValue(hKey,
                       L"LegalNoticeCaption",
                       &LegalNotice.pszCaption);

        ReadRegSzValue(hKey,
                       L"LegalNoticeText",
                       &LegalNotice.pszText);

        RegCloseKey(hKey);
    }

    if (LegalNotice.pszCaption != NULL && wcslen(LegalNotice.pszCaption) != 0 &&
        LegalNotice.pszText != NULL && wcslen(LegalNotice.pszText) != 0)
    {
        pgContext->pWlxFuncs->WlxDialogBoxParam(pgContext->hWlx,
                                                pgContext->hDllInstance,
                                                MAKEINTRESOURCEW(IDD_LEGALNOTICE_DLG),
                                                GetDesktopWindow(),
                                                LegalNoticeDialogProc,
                                                (LPARAM)&LegalNotice);
    }

    if (LegalNotice.pszCaption != NULL)
        HeapFree(GetProcessHeap(), 0, LegalNotice.pszCaption);

    if (LegalNotice.pszText != NULL)
        HeapFree(GetProcessHeap(), 0, LegalNotice.pszText);

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


static VOID
SetLockMessage(HWND hwnd,
               INT nDlgItem,
               PGINA_CONTEXT pgContext)
{
    WCHAR Buffer1[256];
    WCHAR Buffer2[256];
    WCHAR Buffer3[512];

    LoadStringW(pgContext->hDllInstance, IDS_LOCKMSG, Buffer1, 256);

    wsprintfW(Buffer2, L"%s\\%s", pgContext->Domain, pgContext->UserName);
    wsprintfW(Buffer3, Buffer1, Buffer2);

    SetDlgItemTextW(hwnd, nDlgItem, Buffer3);
}


static
BOOL
DoUnlock(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext,
    OUT LPINT Action)
{
    WCHAR Buffer1[256];
    WCHAR Buffer2[256];
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;
    BOOL res = FALSE;

    if (GetTextboxText(hwndDlg, IDC_USERNAME, &UserName) && *UserName == '\0')
    {
        HeapFree(GetProcessHeap(), 0, UserName);
        return FALSE;
    }

    if (GetTextboxText(hwndDlg, IDC_PASSWORD, &Password))
    {
        if (UserName != NULL && Password != NULL &&
            wcscmp(UserName, pgContext->UserName) == 0 &&
            wcscmp(Password, pgContext->Password) == 0)
        {
            *Action = WLX_SAS_ACTION_UNLOCK_WKSTA;
            res = TRUE;
        }
        else if (wcscmp(UserName, pgContext->UserName) == 0 &&
                 wcscmp(Password, pgContext->Password) != 0)
        {
            /* Wrong Password */
            LoadStringW(pgContext->hDllInstance, IDS_LOCKEDWRONGPASSWORD, Buffer2, 256);
            LoadStringW(pgContext->hDllInstance, IDS_COMPUTERLOCKED, Buffer1, 256);
            MessageBoxW(hwndDlg, Buffer2, Buffer1, MB_OK | MB_ICONERROR);
        }
        else
        {
            /* Wrong user name */
            if (DoAdminUnlock(pgContext, UserName, NULL, Password))
            {
                *Action = WLX_SAS_ACTION_UNLOCK_WKSTA;
                res = TRUE;
            }
            else
            {
                LoadStringW(pgContext->hDllInstance, IDS_LOCKEDWRONGUSER, Buffer1, 256);
                wsprintfW(Buffer2, Buffer1, pgContext->Domain, pgContext->UserName);
                LoadStringW(pgContext->hDllInstance, IDS_COMPUTERLOCKED, Buffer1, 256);
                MessageBoxW(hwndDlg, Buffer2, Buffer1, MB_OK | MB_ICONERROR);
            }
        }
    }

    if (UserName != NULL)
        HeapFree(GetProcessHeap(), 0, UserName);

    if (Password != NULL)
        HeapFree(GetProcessHeap(), 0, Password);

    return res;
}


static
INT_PTR
CALLBACK
UnlockWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;
    INT result = WLX_SAS_ACTION_NONE;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            SetLockMessage(hwndDlg, IDC_LOCKMSG, pgContext);

            SetDlgItemTextW(hwndDlg, IDC_USERNAME, pgContext->UserName);
            SetFocus(GetDlgItem(hwndDlg, IDC_PASSWORD));

            if (pgContext->bDisableCAD == TRUE)
                EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);

            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            return TRUE;

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
            DeleteObject(pgContext->hBitmap);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (DoUnlock(hwndDlg, pgContext, &result))
                        EndDialog(hwndDlg, result);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static INT
GUILockedSAS(
    IN OUT PGINA_CONTEXT pgContext)
{
    int result;

    TRACE("GUILockedSAS()\n");

    result = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_UNLOCK_DLG),
        GetDesktopWindow(),
        UnlockWindowProc,
        (LPARAM)pgContext);
    if (result >= WLX_SAS_ACTION_LOGON &&
        result <= WLX_SAS_ACTION_SWITCH_CONSOLE)
    {
        WARN("GUILockedSAS() returns 0x%x\n", result);
        return result;
    }

    WARN("GUILockedSAS() failed (0x%x)\n", result);
    return WLX_SAS_ACTION_NONE;
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
            SetLockMessage(hwndDlg, IDC_LOCKMSG, pgContext);
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
