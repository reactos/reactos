/***********************************************************
 unpage.cpp

  User Manager username wizard and prop page implementations

 History:
  09/23/98: dsheldon created
***********************************************************/
#include "stdafx.h"
#include "resource.h"

#include "unpage.h"
#include "misc.h"

/*************************************
 CUsernamePageBase Implementation
*************************************/

BOOL CUsernamePageBase::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CUsernamePageBase::OnInitDialog");

    // Limit the text of the username, fullname and description fields
    HWND hwndUsername = GetDlgItem(hwnd, IDC_USER);
    SendMessage(hwndUsername, EM_SETLIMITTEXT, 
        ARRAYSIZE(m_pUserInfo->m_szUsername) - 1, 0);
    SetWindowText(hwndUsername, m_pUserInfo->m_szUsername);

    HWND hwndFullName = GetDlgItem(hwnd, IDC_FULLNAME);
    SendMessage(hwndFullName, EM_SETLIMITTEXT, 
        ARRAYSIZE(m_pUserInfo->m_szFullName) - 1, 0);
    SetWindowText(hwndFullName, m_pUserInfo->m_szFullName);

    HWND hwndDescription = GetDlgItem(hwnd, IDC_DESCRIPTION);
    SendMessage(hwndDescription, EM_SETLIMITTEXT, 
        ARRAYSIZE(m_pUserInfo->m_szComment) - 1, 0);
    SetWindowText(hwndDescription, m_pUserInfo->m_szComment);

    TraceLeaveValue(TRUE);
}

/*************************************
 CUsernameWizardPage Implementation
*************************************/

INT_PTR CUsernameWizardPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }
    return FALSE;
}

BOOL CUsernameWizardPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_USR_CORE, "CUsernameWizardPage::OnNotify");

    switch (pnmh->code)
    {
    case PSN_SETACTIVE:
        SetWizardButtons(hwnd, GetParent(hwnd));
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
        TraceLeaveValue(TRUE);
    case PSN_WIZNEXT:
        // Save the data the user has entered
        FetchText(hwnd, IDC_USER, m_pUserInfo->m_szUsername,
            ARRAYSIZE(m_pUserInfo->m_szUsername));

        FetchText(hwnd, IDC_FULLNAME, m_pUserInfo->m_szFullName,
            ARRAYSIZE(m_pUserInfo->m_szFullName));

        FetchText(hwnd, IDC_DESCRIPTION, m_pUserInfo->m_szComment,
            ARRAYSIZE(m_pUserInfo->m_szComment));

        if (S_OK != ValidateName(m_pUserInfo->m_szUsername))
        {
            // Username is invalid. warn now
            ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, IDS_BADUSERNAME, MB_ICONERROR | MB_OK);
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
        }
        else if (::UserAlreadyHasPermission(m_pUserInfo, hwnd))
        {
            // Don't let the user continue if the user they've selected already
            // has permission to use this machine
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
        }
        else
        {
            // We have a username (otherwise next would be disabled)
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
        }

        TraceLeaveValue(TRUE);
    }

    TraceLeaveValue(FALSE);
}

BOOL CUsernameWizardPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CUsernameWizardPage::OnCommand");

    switch (codeNotify)
    {
    case EN_CHANGE:
        SetWizardButtons(hwnd, GetParent(hwnd));
        TraceLeaveValue(TRUE);
    }

    TraceLeaveValue(FALSE);
}

void CUsernameWizardPage::SetWizardButtons(HWND hwnd, HWND hwndPropSheet)
{
    TraceEnter(TRACE_USR_CORE, "CUsernameWizardPage::SetWizardButtons");

    HWND hwndEdit = GetDlgItem(hwnd, IDC_USER);
    DWORD dwUNLength = GetWindowTextLength(hwndEdit);

    PropSheet_SetWizButtons(hwndPropSheet, (dwUNLength == 0) ? 0 : PSWIZB_NEXT);

    TraceLeaveVoid();
}

/*************************************
 CUsernamePropertyPage Implementation
*************************************/

INT_PTR CUsernamePropertyPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }
    return FALSE;
}

BOOL CUsernamePropertyPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CUsernamePropertyPage::OnCommand");

    switch (codeNotify)
    {
    case EN_CHANGE:
        PropSheet_Changed(GetParent(hwnd), hwnd);
        TraceLeaveValue(TRUE);
    }

    TraceLeaveValue(FALSE);
}

BOOL CUsernamePropertyPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_USR_CORE, "CUsernamePropertyPage::OnNotify");
    BOOL fReturn = FALSE;

    switch (pnmh->code)
    {
    case PSN_APPLY:
        {
            TCHAR szTemp[256];
            HRESULT hr;
            LONG lResult = PSNRET_NOERROR;

            // Try to update the username
            FetchText(hwnd, IDC_USER, szTemp,
                ARRAYSIZE(szTemp));

            TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
            ::MakeDomainUserString(m_pUserInfo->m_szDomain, m_pUserInfo->m_szUsername,
                szDomainUser, ARRAYSIZE(szDomainUser));

            if (StrCmp(szTemp, m_pUserInfo->m_szUsername) != 0)
            {
                hr = m_pUserInfo->UpdateUsername(szTemp);

                if (FAILED(hr))
                {
                    ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, 
                        IDS_USR_UPDATE_USERNAME_ERROR, MB_ICONERROR | MB_OK, szDomainUser);

                    lResult = PSNRET_INVALID_NOCHANGEPAGE;
                }
            }

            // Try to update the full name
            FetchText(hwnd, IDC_FULLNAME, szTemp,
                ARRAYSIZE(szTemp));

            if (StrCmp(szTemp, m_pUserInfo->m_szFullName) != 0)
            {
                hr = m_pUserInfo->UpdateFullName(szTemp);

                if (FAILED(hr))
                {
                    ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION,
                        IDS_USR_UPDATE_FULLNAME_ERROR, MB_ICONERROR | MB_OK, szDomainUser);

                    lResult = PSNRET_INVALID_NOCHANGEPAGE;
                }
            }

            // Try to update the description
            FetchText(hwnd, IDC_DESCRIPTION, szTemp,
                ARRAYSIZE(szTemp));

            if (StrCmp(szTemp, m_pUserInfo->m_szComment) != 0)
            {
                hr = m_pUserInfo->UpdateDescription(szTemp);

                if (FAILED(hr))
                {
                    ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION,
                        IDS_USR_UPDATE_DESCRIPTION_ERROR, MB_ICONERROR | MB_OK, szDomainUser);

                    lResult = PSNRET_INVALID_NOCHANGEPAGE;
                }
            }

            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
        }
        fReturn = TRUE;
        break;

    default:
        break;
    }

    TraceLeaveValue(fReturn);
}
