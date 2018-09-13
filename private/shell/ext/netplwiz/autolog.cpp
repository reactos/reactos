/********************************************************
 autolog.cpp

  User Manager autologon dialog implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "data.h"
#include "autolog.h"

INT_PTR CAutologonUserDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CAutologonUserDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CAutologonUserDlg::OnInitDialog");

    // Limit the sizes of the edit boxes
    HWND hwndUsername = GetDlgItem(hwnd, IDC_USER);
    SendMessage(hwndUsername, EM_SETLIMITTEXT, MAX_USER, 0);

    HWND hwndPassword = GetDlgItem(hwnd, IDC_PASSWORD);
    SendMessage(hwndPassword, EM_SETLIMITTEXT, MAX_PASSWORD, 0);

    HWND hwndConfirm = GetDlgItem(hwnd, IDC_CONFIRMPASSWORD);
    SendMessage(hwndConfirm, EM_SETLIMITTEXT, MAX_PASSWORD, 0);

    // Populate the username field and set focus to password
    SetWindowText(hwndUsername, m_pszUsername);
    SetFocus(hwndPassword);
    BOOL fSetDefaultFocus = FALSE;

    TraceLeaveValue(fSetDefaultFocus);
}

BOOL CAutologonUserDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CAutologonUserDlg::OnCommand");

    switch (id)
    {
    case IDOK:
        {
            TCHAR szUsername[MAX_USER + 1];
            TCHAR szPassword[MAX_PASSWORD + 1];
            TCHAR szConfirm[MAX_PASSWORD + 1];

            FetchText(hwnd, IDC_USER,
                szUsername, ARRAYSIZE(szUsername));

            GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD),
                szPassword, ARRAYSIZE(szPassword));

            GetWindowText(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD),
                szConfirm, ARRAYSIZE(szConfirm));

            if (StrCmp(szConfirm, szPassword) != 0)
            {
                // Display a message saying the passwords don't match
                DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, 
                    IDS_ERR_PWDNOMATCH, MB_OK | MB_ICONERROR);
                break;
            }
            else
            {
                // Actually apply the autologon
                SetAutoLogon(szUsername, szPassword);
                ZeroMemory(szPassword, ARRAYSIZE(szPassword));
            }
        }
        
        // Fall through
    case IDCANCEL:
        EndDialog(hwnd, id);
    }

    TraceLeaveValue(TRUE);
}
