/********************************************************
 pwpage.cpp

  User Manager password dialog/wizard page implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "pwpage.h"
#include "misc.h"

BOOL CPasswordPageBase::DoPasswordsMatch(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CPasswordBase::DoPasswordsMatch");

    BOOL fMatch;

    // Get the passwords
    TCHAR szConfirmPW[MAX_PASSWORD + 1];
    TCHAR szPassword[MAX_PASSWORD + 1];


    GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), szPassword,
        ARRAYSIZE(szPassword));

    GetWindowText(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), szConfirmPW,
        ARRAYSIZE(szConfirmPW));

    fMatch = (StrCmp(szPassword, szConfirmPW) == 0);

    if (!fMatch)
    {
        // Display a message saying the passwords don't match
        DisplayFormatMessage(hwnd, IDS_USR_NEWUSERWIZARD_CAPTION, 
            IDS_ERR_PWDNOMATCH, 
            MB_OK | MB_ICONERROR);
    }

    TraceLeaveValue(fMatch);
}

INT_PTR CPasswordWizardPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    }

    return FALSE;
}

BOOL CPasswordWizardPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CPasswordWizardPage::OnInitDialog");

    // Limit the password fields
    SendMessage(GetDlgItem(hwnd, IDC_PASSWORD), EM_SETLIMITTEXT,
        ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1, 0);

    SendMessage(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), EM_SETLIMITTEXT,
        ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1, 0);

    TraceLeaveValue(TRUE);
}

BOOL CPasswordWizardPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CPasswordWizardPage::OnCommand");

    switch (id)
    {
    case IDOK:
        // Verify that the passwords match
        if (DoPasswordsMatch(hwnd))
        {
            // Password is the same as confirm password - read password into user info
            GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), m_pUserInfo->m_szPasswordBuffer,
                ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer));

            // Hide the password
            m_pUserInfo->HidePassword();

            EndDialog(hwnd, IDOK);
        }
        else
        {
            m_pUserInfo->ZeroPassword();
        }
        break;

    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
        
    default:
        break;
    }

    TraceLeaveValue(TRUE);
}


BOOL CPasswordWizardPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_USR_CORE, "CPasswordWizardPage::OnNotify");

    switch (pnmh->code)
    {
    case PSN_SETACTIVE:
        PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT | PSWIZB_BACK);

        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
        TraceLeaveValue(TRUE);
    case PSN_WIZNEXT:
        // Save the data the user has entered
        if (DoPasswordsMatch(hwnd))
        {
            // Password is the same as confirm password - read password into user info
            GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), m_pUserInfo->m_szPasswordBuffer,
                ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer));
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);

            // Hide the password
            m_pUserInfo->HidePassword();
        }
        else
        {
            m_pUserInfo->ZeroPassword();

            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
        }

        TraceLeaveValue(TRUE);
    }

    TraceLeaveValue(FALSE);
}



INT_PTR CChangePasswordDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CChangePasswordDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CChangePasswordDlg::OnInitDialog");

    // Limit the password fields
    SendMessage(GetDlgItem(hwnd, IDC_PASSWORD), EM_SETLIMITTEXT,
        ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1, 0);

    SendMessage(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), EM_SETLIMITTEXT,
        ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1, 0);

    TraceLeaveValue(TRUE);
}

BOOL CChangePasswordDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CChangePasswordDlg::OnCommand");

    switch (id)
    {
    case IDOK:
        // Verify that the passwords match
        if (DoPasswordsMatch(hwnd))
        {
            // Password is the same as confirm password - read password into user info
            GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), m_pUserInfo->m_szPasswordBuffer,
                ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer));

            // Hide the password
            m_pUserInfo->HidePassword();

            // Update the password
            BOOL fBadPasswordFormat;
            if (SUCCEEDED(m_pUserInfo->UpdatePassword(&fBadPasswordFormat)))
            {
                EndDialog(hwnd, IDOK);
            }
            else
            {
                TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
                MakeDomainUserString(m_pUserInfo->m_szDomain, m_pUserInfo->m_szUsername,
                    szDomainUser, ARRAYSIZE(szDomainUser));


                if (fBadPasswordFormat)
                {
                    ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, 
                        IDS_USR_UPDATE_PASSWORD_TOOSHORT_ERROR, MB_ICONERROR | MB_OK,
                        szDomainUser);
                }
                else
                {
                    ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION,
                        IDS_USR_UPDATE_PASSWORD_ERROR, MB_ICONERROR | MB_OK,
                        szDomainUser);
                }
            }
        }
        break;

    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
        
    default:
        break;
    }

    TraceLeaveValue(TRUE);
}
