/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
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

// Timer ID for the animated dialog bar.
#define IDT_BAR 1

typedef struct _DLG_DATA
{
    PGINA_CONTEXT pgContext;
    HBITMAP hLogoBitmap;
    HBITMAP hBarBitmap;
    HWND hWndBarCtrl;
    DWORD BarCounter;
    DWORD LogoWidth;
    DWORD LogoHeight;
    DWORD BarWidth;
    DWORD BarHeight;
} DLG_DATA, *PDLG_DATA;

static PDLG_DATA
DlgData_Create(HWND hwndDlg, PGINA_CONTEXT pgContext)
{
    PDLG_DATA pDlgData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pDlgData));
    if (pDlgData)
    {
        pDlgData->pgContext = pgContext;
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)pDlgData);
    }
    return pDlgData;
}

static VOID
DlgData_LoadBitmaps(_Inout_ PDLG_DATA pDlgData)
{
    BITMAP bm;

    if (!pDlgData)
    {
        return;
    }

    pDlgData->hLogoBitmap = LoadImageW(pDlgData->pgContext->hDllInstance,
                                       MAKEINTRESOURCEW(IDI_ROSLOGO), IMAGE_BITMAP,
                                       0, 0, LR_DEFAULTCOLOR);
    if (pDlgData->hLogoBitmap)
    {
        GetObject(pDlgData->hLogoBitmap, sizeof(bm), &bm);
        pDlgData->LogoWidth = bm.bmWidth;
        pDlgData->LogoHeight = bm.bmHeight;
    }

    pDlgData->hBarBitmap = LoadImageW(hDllInstance, MAKEINTRESOURCEW(IDI_BAR),
                                      IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    if (pDlgData->hBarBitmap)
    {
        GetObject(pDlgData->hBarBitmap, sizeof(bm), &bm);
        pDlgData->BarWidth = bm.bmWidth;
        pDlgData->BarHeight = bm.bmHeight;
    }
}

static VOID
DlgData_Destroy(_Inout_ HWND hwndDlg)
{
    PDLG_DATA pDlgData;

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    if (!pDlgData)
    {
        return;
    }

    SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)NULL);

    if (pDlgData->hBarBitmap)
    {
        DeleteObject(pDlgData->hBarBitmap);
    }

    if (pDlgData->hLogoBitmap)
    {
        DeleteObject(pDlgData->hLogoBitmap);
    }

    HeapFree(GetProcessHeap(), 0, pDlgData);
}

static BOOL
GUIInitialize(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUIInitialize(%p)\n", pgContext);
    return TRUE;
}

static
VOID
SetWelcomeText(HWND hWnd)
{
    PWCHAR pBuffer = NULL, p;
    HKEY hKey;
    DWORD BufSize, dwType, dwWelcomeSize, dwTitleLength;
    LONG rc;

    TRACE("SetWelcomeText(%p)\n", hWnd);

    /* Open the Winlogon key */
    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                       0,
                       KEY_QUERY_VALUE,
                       &hKey);
    if (rc != ERROR_SUCCESS)
    {
        WARN("RegOpenKeyExW() failed with error %lu\n", rc);
        return;
    }

    /* Get the size of the Welcome value */
    dwWelcomeSize = 0;
    rc = RegQueryValueExW(hKey,
                          L"Welcome",
                          NULL,
                          &dwType,
                          NULL,
                          &dwWelcomeSize);
    if (rc == ERROR_FILE_NOT_FOUND || dwWelcomeSize == 0 || dwType != REG_SZ)
        goto done;

    dwTitleLength = GetWindowTextLengthW(hWnd);
    BufSize = dwWelcomeSize + ((dwTitleLength + 1) * sizeof(WCHAR));

    pBuffer = HeapAlloc(GetProcessHeap(), 0, BufSize);
    if (pBuffer == NULL)
        goto done;

    GetWindowTextW(hWnd, pBuffer, BufSize / sizeof(WCHAR));
    wcscat(pBuffer, L" ");
    p = &pBuffer[dwTitleLength + 1];

    RegQueryValueExW(hKey,
                     L"Welcome",
                     NULL,
                     &dwType,
                     (PBYTE)p,
                     &dwWelcomeSize);

    SetWindowText(hWnd, pBuffer);

done:
    if (pBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, pBuffer);

    RegCloseKey(hKey);
}

static VOID
AdjustStatusMessageWindow(HWND hwndDlg, PDLG_DATA pDlgData)
{
    INT xOld, yOld, cxOld, cyOld;
    INT xNew, yNew, cxNew, cyNew;
    INT cxLabel, cyLabel, dyLabel;
    RECT rc, rcBar, rcLabel, rcWnd;
    BITMAP bmLogo, bmBar;
    DWORD style, exstyle;
    HWND hwndLogo = GetDlgItem(hwndDlg, IDC_ROSLOGO);
    HWND hwndBar = GetDlgItem(hwndDlg, IDC_BAR);
    HWND hwndLabel = GetDlgItem(hwndDlg, IDC_STATUS_MESSAGE);

    /* This adjustment is for CJK only */
    switch (PRIMARYLANGID(GetUserDefaultLangID()))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            break;

        default:
            return;
    }

    if (!GetObjectW(pDlgData->hLogoBitmap, sizeof(BITMAP), &bmLogo) ||
        !GetObjectW(pDlgData->hBarBitmap, sizeof(BITMAP), &bmBar))
    {
        return;
    }

    GetWindowRect(hwndBar, &rcBar);
    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rcBar, 2);
    dyLabel = bmLogo.bmHeight - rcBar.top;

    GetWindowRect(hwndLabel, &rcLabel);
    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rcLabel, 2);
    cxLabel = rcLabel.right - rcLabel.left;
    cyLabel = rcLabel.bottom - rcLabel.top;

    MoveWindow(hwndLogo, 0, 0, bmLogo.bmWidth, bmLogo.bmHeight, TRUE);
    MoveWindow(hwndBar, 0, bmLogo.bmHeight, bmLogo.bmWidth, bmBar.bmHeight, TRUE);
    MoveWindow(hwndLabel, rcLabel.left, rcLabel.top + dyLabel, cxLabel, cyLabel, TRUE);

    GetWindowRect(hwndDlg, &rcWnd);
    xOld = rcWnd.left;
    yOld = rcWnd.top;
    cxOld = rcWnd.right - rcWnd.left;
    cyOld = rcWnd.bottom - rcWnd.top;

    GetClientRect(hwndDlg, &rc);
    SetRect(&rc, 0, 0, bmLogo.bmWidth, rc.bottom - rc.top); /* new client size */

    style = (DWORD)GetWindowLongPtrW(hwndDlg, GWL_STYLE);
    exstyle = (DWORD)GetWindowLongPtrW(hwndDlg, GWL_EXSTYLE);
    AdjustWindowRectEx(&rc, style, FALSE, exstyle);

    cxNew = rc.right - rc.left;
    cyNew = (rc.bottom - rc.top) + dyLabel;
    xNew = xOld - (cxNew - cxOld) / 2;
    yNew = yOld - (cyNew - cyOld) / 2;
    MoveWindow(hwndDlg, xNew, yNew, cxNew, cyNew, TRUE);
}

static INT_PTR CALLBACK
StatusDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDLG_DATA pDlgData;
    UNREFERENCED_PARAMETER(wParam);

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

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
            SetDlgItemTextW(hwndDlg, IDC_STATUS_MESSAGE, msg->pMessage);
            SetEvent(msg->StartupEvent);

            pDlgData = DlgData_Create(hwndDlg, msg->Context);
            if (pDlgData == NULL)
                return FALSE;

            DlgData_LoadBitmaps(pDlgData);
            if (pDlgData->hBarBitmap)
            {
                if (SetTimer(hwndDlg, IDT_BAR, 20, NULL) == 0)
                {
                    ERR("SetTimer(IDT_BAR) failed: %d\n", GetLastError());
                }
                else
                {
                    /* Get the animation bar control */
                    pDlgData->hWndBarCtrl = GetDlgItem(hwndDlg, IDC_BAR);
                }
            }

            AdjustStatusMessageWindow(hwndDlg, pDlgData);
            return TRUE;
        }

        case WM_TIMER:
        {
            if (pDlgData && pDlgData->hBarBitmap)
            {
                /*
                 * Default rotation bar image width is 413 (same as logo)
                 * We can divide 413 by 7 without remainder
                 */
                pDlgData->BarCounter = (pDlgData->BarCounter + 7) % pDlgData->BarWidth;
                InvalidateRect(pDlgData->hWndBarCtrl, NULL, FALSE);
                UpdateWindow(pDlgData->hWndBarCtrl);
            }
            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;

            if (lpDis->CtlID != IDC_BAR)
            {
                return FALSE;
            }

            if (pDlgData && pDlgData->hBarBitmap)
            {
                HDC hdcMem;
                HGDIOBJ hOld;
                DWORD off = pDlgData->BarCounter;
                DWORD iw = pDlgData->BarWidth;
                DWORD ih = pDlgData->BarHeight;

                hdcMem = CreateCompatibleDC(lpDis->hDC);
                hOld = SelectObject(hdcMem, pDlgData->hBarBitmap);
                BitBlt(lpDis->hDC, off, 0, iw - off, ih, hdcMem, 0, 0, SRCCOPY);
                BitBlt(lpDis->hDC, 0, 0, off, ih, hdcMem, iw - off, 0, SRCCOPY);
                SelectObject(hdcMem, hOld);
                DeleteDC(hdcMem);

                return TRUE;
            }
            return FALSE;
        }

        case WM_DESTROY:
        {
            if (pDlgData && pDlgData->hBarBitmap)
            {
                KillTimer(hwndDlg, IDT_BAR);
            }
            DlgData_Destroy(hwndDlg);
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

    DialogBoxParamW(
        hDllInstance,
        MAKEINTRESOURCEW(IDD_STATUS),
        GetDesktopWindow(),
        StatusDialogProc,
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
                                           sizeof(*msg));
        if(!msg)
            return FALSE;

        msg->Context = pgContext;
        msg->dwOptions = dwOptions;
        msg->pTitle = pTitle;
        msg->pMessage = pMessage;
        msg->hDesktop = hDesktop;

        msg->StartupEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

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

    SetDlgItemTextW(pgContext->hStatusWindow, IDC_STATUS_MESSAGE, pMessage);

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
WelcomeDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDLG_DATA pDlgData;

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pDlgData = DlgData_Create(hwndDlg, (PGINA_CONTEXT)lParam);
            if (pDlgData == NULL)
                return FALSE;

            DlgData_LoadBitmaps(pDlgData);
            return TRUE;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            if (pDlgData && pDlgData->hLogoBitmap)
            {
                BeginPaint(hwndDlg, &ps);
                DrawStateW(ps.hdc, NULL, NULL, (LPARAM)pDlgData->hLogoBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            DlgData_Destroy(hwndDlg);
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
                                            MAKEINTRESOURCEW(IDD_WELCOME),
                                            GetDesktopWindow(),
                                            WelcomeDialogProc,
                                            (LPARAM)pgContext);
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

    LoadStringW(pgContext->hDllInstance, uCaption, szCaption, _countof(szCaption));
    LoadStringW(pgContext->hDllInstance, uText, szText, _countof(szText));

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

    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_USERNAME, UserName, _countof(UserName));
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_DOMAIN, Domain, _countof(Domain));
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_OLDPWD, OldPassword, _countof(OldPassword));
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD1, NewPassword1, _countof(NewPassword1));
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD2, NewPassword2, _countof(NewPassword2));

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
    RequestBuffer->DomainName.Length = (USHORT)wcslen(Domain) * sizeof(WCHAR);
    RequestBuffer->DomainName.MaximumLength = RequestBuffer->DomainName.Length + sizeof(WCHAR);
    RequestBuffer->DomainName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->DomainName.Buffer,
                  Domain,
                  RequestBuffer->DomainName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->DomainName.MaximumLength);

    /* Pack the user name */
    RequestBuffer->AccountName.Length = (USHORT)wcslen(UserName) * sizeof(WCHAR);
    RequestBuffer->AccountName.MaximumLength = RequestBuffer->AccountName.Length + sizeof(WCHAR);
    RequestBuffer->AccountName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->AccountName.Buffer,
                  UserName,
                  RequestBuffer->AccountName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->AccountName.MaximumLength);

    /* Pack the old password */
    RequestBuffer->OldPassword.Length = (USHORT)wcslen(OldPassword) * sizeof(WCHAR);
    RequestBuffer->OldPassword.MaximumLength = RequestBuffer->OldPassword.Length + sizeof(WCHAR);
    RequestBuffer->OldPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->OldPassword.Buffer,
                  OldPassword,
                  RequestBuffer->OldPassword.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->OldPassword.MaximumLength);

    /* Pack the new password */
    RequestBuffer->NewPassword.Length = (USHORT)wcslen(NewPassword1) * sizeof(WCHAR);
    RequestBuffer->NewPassword.MaximumLength = RequestBuffer->NewPassword.Length + sizeof(WCHAR);
    RequestBuffer->NewPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->NewPassword.Buffer,
                  NewPassword1,
                  RequestBuffer->NewPassword.MaximumLength);

    /* Connect to the LSA server */
    if (ConnectToLsa(pgContext) != ERROR_SUCCESS)
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
        (wcscmp(Domain, pgContext->DomainName) == 0) &&
        (wcscmp(OldPassword, pgContext->Password) == 0))
    {
        ZeroMemory(pgContext->Password, sizeof(pgContext->Password));
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

    pgContext = (PGINA_CONTEXT)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)pgContext);

            SetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_USERNAME, pgContext->UserName);
            SendDlgItemMessageW(hwndDlg, IDC_CHANGEPWD_DOMAIN, CB_ADDSTRING, 0, (LPARAM)pgContext->DomainName);
            SendDlgItemMessageW(hwndDlg, IDC_CHANGEPWD_DOMAIN, CB_SETCURSEL, 0, 0);
            SetFocus(GetDlgItem(hwndDlg, IDC_CHANGEPWD_OLDPWD));
            return TRUE;
        }

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

    LoadStringW(pgContext->hDllInstance, IDS_LOGONMSG, Buffer1, _countof(Buffer1));

    wsprintfW(Buffer2, L"%s\\%s", pgContext->DomainName, pgContext->UserName);
    wsprintfW(Buffer4, Buffer1, Buffer2);

    SetDlgItemTextW(hwnd, IDC_SECURITY_MESSAGE, Buffer4);

    LoadStringW(pgContext->hDllInstance, IDS_LOGONDATE, Buffer1, _countof(Buffer1));

    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                   (SYSTEMTIME*)&pgContext->LogonTime, NULL, Buffer2, _countof(Buffer2));

    GetTimeFormatW(LOCALE_USER_DEFAULT, 0,
                   (SYSTEMTIME*)&pgContext->LogonTime, NULL, Buffer3, _countof(Buffer3));

    wsprintfW(Buffer4, Buffer1, Buffer2, Buffer3);

    SetDlgItemTextW(hwnd, IDC_SECURITY_LOGONDATE, Buffer4);

    if (pgContext->bAutoAdminLogon)
        EnableWindow(GetDlgItem(hwnd, IDC_SECURITY_LOGOFF), FALSE);
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
        MAKEINTRESOURCEW(IDD_CHANGEPWD),
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
        MAKEINTRESOURCEW(IDD_LOGOFF),
        hwndDlg,
        LogOffDialogProc,
        (LPARAM)pgContext);
}


static
INT
OnShutDown(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext)
{
    INT ret;
    DWORD ShutdownOptions;

    TRACE("OnShutDown(%p %p)\n", hwndDlg, pgContext);

    pgContext->nShutdownAction = GetDefaultShutdownSelState();
    ShutdownOptions = GetDefaultShutdownOptions();

    if (pgContext->UserToken != NULL)
    {
        if (ImpersonateLoggedOnUser(pgContext->UserToken))
        {
            pgContext->nShutdownAction = LoadShutdownSelState();
            ShutdownOptions = GetAllowedShutdownOptions();
            RevertToSelf();
        }
        else
        {
            ERR("WL: ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        }
    }

    ret = ShutdownDialog(hwndDlg, ShutdownOptions, pgContext);

    if (ret == IDOK)
    {
        if (pgContext->UserToken != NULL)
        {
            if (ImpersonateLoggedOnUser(pgContext->UserToken))
            {
                SaveShutdownSelState(pgContext->nShutdownAction);
                RevertToSelf();
            }
            else
            {
                ERR("WL: ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
            }
        }
    }

    return ret;
}


static INT_PTR CALLBACK
SecurityDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)pgContext);

            SetWelcomeText(hwndDlg);

            OnInitSecurityDlg(hwndDlg, (PGINA_CONTEXT)lParam);
            SetFocus(GetDlgItem(hwndDlg, IDNO));
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_SECURITY_LOCK:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_LOCK_WKSTA);
                    return TRUE;
                case IDC_SECURITY_LOGOFF:
                    if (OnLogOff(hwndDlg, pgContext) == IDYES)
                        EndDialog(hwndDlg, WLX_SAS_ACTION_LOGOFF);
                    return TRUE;
                case IDC_SECURITY_SHUTDOWN:
                    if (OnShutDown(hwndDlg, pgContext) == IDOK)
                        EndDialog(hwndDlg, pgContext->nShutdownAction);
                    return TRUE;
                case IDC_SECURITY_CHANGEPWD:
                    if (OnChangePassword(hwndDlg, pgContext))
                        EndDialog(hwndDlg, WLX_SAS_ACTION_PWD_CHANGED);
                    return TRUE;
                case IDC_SECURITY_TASKMGR:
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
        MAKEINTRESOURCEW(IDD_SECURITY),
        GetDesktopWindow(),
        SecurityDialogProc,
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

    if (GetTextboxText(hwndDlg, IDC_LOGON_USERNAME, &UserName) && *UserName == '\0')
        goto done;

    if (GetTextboxText(hwndDlg, IDC_LOGON_DOMAIN, &Domain) && *Domain == '\0')
        goto done;

    if (!GetTextboxText(hwndDlg, IDC_LOGON_PASSWORD, &Password))
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
            ResourceMessageBox(pgContext,
                               hwndDlg,
                               MB_OK | MB_ICONERROR,
                               IDS_LOGONTITLE,
                               IDS_ACCOUNTLOCKED);
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
                                  pgContext->DomainName,
                                  pgContext->Password,
                                  &SubStatus);
            if (!NT_SUCCESS(Status))
            {
                TRACE("Login after password change failed! (Status 0x%08lx)\n", Status);

                goto done;
            }
        }
        else if (SubStatus == STATUS_ACCOUNT_EXPIRED)
        {
            ResourceMessageBox(pgContext,
                               hwndDlg,
                               MB_OK | MB_ICONEXCLAMATION,
                               IDS_LOGONTITLE,
                               IDS_ACCOUNTEXPIRED);
        }
        else if (SubStatus == STATUS_INVALID_LOGON_HOURS)
        {
            ResourceMessageBox(pgContext,
                               hwndDlg,
                               MB_OK | MB_ICONERROR,
                               IDS_LOGONTITLE,
                               IDS_INVALIDLOGONHOURS);
            goto done;
        }
        else if (SubStatus == STATUS_INVALID_WORKSTATION)
        {
            ResourceMessageBox(pgContext,
                               hwndDlg,
                               MB_OK | MB_ICONERROR,
                               IDS_LOGONTITLE,
                               IDS_INVALIDWORKSTATION);
            goto done;
        }
        else
        {
            TRACE("Other error!\n");
            ResourceMessageBox(pgContext,
                               hwndDlg,
                               MB_OK | MB_ICONERROR,
                               IDS_LOGONTITLE,
                               IDS_ACCOUNTRESTRICTION);
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

    ZeroMemory(pgContext->Password, sizeof(pgContext->Password));
    wcscpy(pgContext->Password, Password);

    result = TRUE;

done:
    pgContext->bAutoAdminLogon = FALSE;

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

    dwComputerNameLength = _countof(szComputerName);
    if (GetComputerNameW(szComputerName, &dwComputerNameLength))
    {
        lIndex = SendMessageW(hwndDomainComboBox, CB_ADDSTRING, 0, (LPARAM)szComputerName);
    }

    if (wcslen(pgContext->DomainName) != 0)
    {
        lFindIndex = SendMessageW(hwndDomainComboBox, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pgContext->DomainName);
        if (lFindIndex == CB_ERR)
        {
            lIndex = SendMessageW(hwndDomainComboBox, CB_ADDSTRING, 0, (LPARAM)pgContext->DomainName);
        }
        else
        {
            lIndex = lFindIndex;
        }
    }

    SendMessageW(hwndDomainComboBox, CB_SETCURSEL, lIndex, 0);
}


static INT_PTR CALLBACK
LogonDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDLG_DATA pDlgData;

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* FIXME: take care of NoDomainUI */
            pDlgData = DlgData_Create(hwndDlg, (PGINA_CONTEXT)lParam);
            if (pDlgData == NULL)
                return FALSE;

            DlgData_LoadBitmaps(pDlgData);

            SetWelcomeText(hwndDlg);

            if (pDlgData->pgContext->bAutoAdminLogon ||
                !pDlgData->pgContext->bDontDisplayLastUserName)
                SetDlgItemTextW(hwndDlg, IDC_LOGON_USERNAME, pDlgData->pgContext->UserName);

            if (pDlgData->pgContext->bAutoAdminLogon)
                SetDlgItemTextW(hwndDlg, IDC_LOGON_PASSWORD, pDlgData->pgContext->Password);

            SetDomainComboBox(GetDlgItem(hwndDlg, IDC_LOGON_DOMAIN), pDlgData->pgContext);

            if (pDlgData->pgContext->bDisableCAD)
                EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);

            if (!pDlgData->pgContext->bShutdownWithoutLogon)
                EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_SHUTDOWN), FALSE);

            SetFocus(GetDlgItem(hwndDlg, pDlgData->pgContext->bDontDisplayLastUserName ? IDC_LOGON_USERNAME : IDC_LOGON_PASSWORD));

            if (pDlgData->pgContext->bAutoAdminLogon)
                PostMessage(GetDlgItem(hwndDlg, IDOK), BM_CLICK, 0, 0);

            return TRUE;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            if (pDlgData && pDlgData->hLogoBitmap)
            {
                BeginPaint(hwndDlg, &ps);
                DrawStateW(ps.hdc, NULL, NULL, (LPARAM)pDlgData->hLogoBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }

        case WM_DESTROY:
            DlgData_Destroy(hwndDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (DoLogon(hwndDlg, pDlgData->pgContext))
                        EndDialog(hwndDlg, WLX_SAS_ACTION_LOGON);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;

                case IDC_LOGON_SHUTDOWN:
                    if (OnShutDown(hwndDlg, pDlgData->pgContext) == IDOK)
                        EndDialog(hwndDlg, pDlgData->pgContext->nShutdownAction);
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
                                                MAKEINTRESOURCEW(IDD_LEGALNOTICE),
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
        MAKEINTRESOURCEW(IDD_LOGON),
        GetDesktopWindow(),
        LogonDialogProc,
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

    LoadStringW(pgContext->hDllInstance, IDS_LOCKMSG, Buffer1, _countof(Buffer1));

    wsprintfW(Buffer2, L"%s\\%s", pgContext->DomainName, pgContext->UserName);
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

    if (GetTextboxText(hwndDlg, IDC_UNLOCK_USERNAME, &UserName) && *UserName == '\0')
    {
        HeapFree(GetProcessHeap(), 0, UserName);
        return FALSE;
    }

    if (GetTextboxText(hwndDlg, IDC_UNLOCK_PASSWORD, &Password))
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
            LoadStringW(pgContext->hDllInstance, IDS_LOCKEDWRONGPASSWORD, Buffer2, _countof(Buffer2));
            LoadStringW(pgContext->hDllInstance, IDS_COMPUTERLOCKED, Buffer1, _countof(Buffer1));
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
                LoadStringW(pgContext->hDllInstance, IDS_LOCKEDWRONGUSER, Buffer1, _countof(Buffer1));
                wsprintfW(Buffer2, Buffer1, pgContext->DomainName, pgContext->UserName);
                LoadStringW(pgContext->hDllInstance, IDS_COMPUTERLOCKED, Buffer1, _countof(Buffer1));
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
UnlockDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDLG_DATA pDlgData;
    INT result = WLX_SAS_ACTION_NONE;

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pDlgData = DlgData_Create(hwndDlg, (PGINA_CONTEXT)lParam);
            if (pDlgData == NULL)
                return FALSE;

            SetWelcomeText(hwndDlg);

            SetLockMessage(hwndDlg, IDC_UNLOCK_MESSAGE, pDlgData->pgContext);

            SetDlgItemTextW(hwndDlg, IDC_UNLOCK_USERNAME, pDlgData->pgContext->UserName);
            SetFocus(GetDlgItem(hwndDlg, IDC_UNLOCK_PASSWORD));

            if (pDlgData->pgContext->bDisableCAD)
                EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);

            DlgData_LoadBitmaps(pDlgData);
            return TRUE;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            if (pDlgData && pDlgData->hLogoBitmap)
            {
                BeginPaint(hwndDlg, &ps);
                DrawStateW(ps.hdc, NULL, NULL, (LPARAM)pDlgData->hLogoBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
            DlgData_Destroy(hwndDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (DoUnlock(hwndDlg, pDlgData->pgContext, &result))
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
        MAKEINTRESOURCEW(IDD_UNLOCK),
        GetDesktopWindow(),
        UnlockDialogProc,
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
LockedDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDLG_DATA pDlgData;

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pDlgData = DlgData_Create(hwndDlg, (PGINA_CONTEXT)lParam);
            if (pDlgData == NULL)
                return FALSE;

            DlgData_LoadBitmaps(pDlgData);

            SetWelcomeText(hwndDlg);

            SetLockMessage(hwndDlg, IDC_LOCKED_MESSAGE, pDlgData->pgContext);
            return TRUE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            if (pDlgData && pDlgData->hLogoBitmap)
            {
                BeginPaint(hwndDlg, &ps);
                DrawStateW(ps.hdc, NULL, NULL, (LPARAM)pDlgData->hLogoBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            DlgData_Destroy(hwndDlg);
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
        MAKEINTRESOURCEW(IDD_LOCKED),
        GetDesktopWindow(),
        LockedDialogProc,
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
