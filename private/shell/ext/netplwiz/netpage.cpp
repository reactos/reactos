/********************************************************
 netpage.cpp

  User Manager New Network User Wizard Page Implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "netpage.h"
#include "misc.h"

CNetworkUserWizardPage::CNetworkUserWizardPage(CUserInfo* pUserInfo)
{
    TraceEnter(TRACE_USR_CORE, "CNetworkUserWizardPage::CNetworkUserWizardPage");

    m_pUserInfo = pUserInfo;

    TraceLeaveVoid();
}

INT_PTR CNetworkUserWizardPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    }

    return FALSE;
}

BOOL CNetworkUserWizardPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CNetworkUserWizardPage::OnInitDialog");

    SendMessage(GetDlgItem(hwnd, IDC_USER), EM_SETLIMITTEXT,
        MAX_USER, 0);

    SendMessage(GetDlgItem(hwnd, IDC_DOMAIN), EM_SETLIMITTEXT,
        MAX_DOMAIN, 0);

    TraceLeaveResult(TRUE);
}

BOOL CNetworkUserWizardPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_USR_CORE, "CNetworkUserWizardPage::OnNotify");
           
    switch (pnmh->code)
    {
    case PSN_SETACTIVE:
        if (m_pUserInfo->m_psid != NULL)
        {
            LocalFree(m_pUserInfo->m_psid);
            m_pUserInfo->m_psid = NULL;
        }

        SetWizardButtons(hwnd, GetParent(hwnd));
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
        TraceLeaveValue(TRUE);
    case PSN_WIZNEXT:
        // Read in the network user name and domain name
        if (FAILED(GetUserAndDomain(hwnd)))
        {
            // We don't have both!
            DisplayFormatMessage(hwnd, IDS_USR_NEWUSERWIZARD_CAPTION, IDS_USR_NETUSERNAME_ERROR,
                MB_OK | MB_ICONERROR);

            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
        }
        else
        {
            if (::UserAlreadyHasPermission(m_pUserInfo, hwnd))
            {
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
            }
            else
            {
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
            }
        }

        TraceLeaveValue(TRUE);
    }

    TraceLeaveValue(FALSE);
}

BOOL CNetworkUserWizardPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CNetworkUserWizardPage::OnCommand");

    switch (id)
    {
    case IDC_BROWSE_BUTTON:
        {
            // Launch object picker to find a network account to give permissions to
            TCHAR szUser[MAX_USER + 1];
            TCHAR szDomain[MAX_DOMAIN + 1];
            
            if (S_OK == ::BrowseForUser(hwnd, szUser, ARRAYSIZE(szUser),
                szDomain, ARRAYSIZE(szDomain)))
            {
                // Ok clicked and buffers valid
                SetDlgItemText(hwnd, IDC_USER, szUser);
                SetDlgItemText(hwnd, IDC_DOMAIN, szDomain);
            }

            TraceLeaveResult(TRUE);
        }

    case IDC_USER:
        if (codeNotify == EN_CHANGE)
        {
            SetWizardButtons(hwnd, GetParent(hwnd));
        }
        break;
    }

    TraceLeaveValue(FALSE);
}

void CNetworkUserWizardPage::SetWizardButtons(HWND hwnd, HWND hwndPropSheet)
{
    TraceEnter(TRACE_USR_CORE, "CNetworkUserWizardPage::SetWizardButtons");

    HWND hwndUsername = GetDlgItem(hwnd, IDC_USER);
    DWORD dwUNLength = GetWindowTextLength(hwndUsername);

    PropSheet_SetWizButtons(hwndPropSheet, (dwUNLength == 0) ? 0 : PSWIZB_NEXT);

    TraceLeaveVoid();
}

HRESULT CNetworkUserWizardPage::GetUserAndDomain(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CNetworkUserWizardPage::GetUserAndDomain");
    HRESULT hr = S_OK;

    // This code checks to ensure the user isn't trying
    // to add a well-known group like Everyone! This is bad
    // If the SID isn't read here, it is read in in CUserInfo::ChangeLocalGroup

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];

    FetchText(hwnd, IDC_USER, m_pUserInfo->m_szUsername, ARRAYSIZE(m_pUserInfo->m_szUsername));
    FetchText(hwnd, IDC_DOMAIN, m_pUserInfo->m_szDomain, ARRAYSIZE(m_pUserInfo->m_szDomain));

    // If the username doesn't already contain a domain and the domain specified in blank
    if ((NULL == StrChr(m_pUserInfo->m_szUsername, TEXT('\\'))) &&
        (0 == m_pUserInfo->m_szDomain[0]))
    {
        // Assume local machine for the domain
        DWORD cchName = ARRAYSIZE(m_pUserInfo->m_szDomain);
        
        if (!GetComputerName(m_pUserInfo->m_szDomain, &cchName))
        {
            *(m_pUserInfo->m_szDomain) = 0;
        }
    }

    ::MakeDomainUserString(m_pUserInfo->m_szDomain, m_pUserInfo->m_szUsername, 
        szDomainUser, ARRAYSIZE(szDomainUser));

#ifdef _0

    // Try to find the SID for this user
    DWORD cchDomain = ARRAYSIZE(m_pUserInfo->m_szDomain);
    hr = AttemptLookupAccountName(szDomainUser, &m_pUserInfo->m_psid, m_pUserInfo->m_szDomain, 
        &cchDomain, &m_pUserInfo->m_sUse);

    if (SUCCEEDED(hr))
    {
        // Make sure this isn't a well-known group like 'Everyone'
        if (m_pUserInfo->m_sUse == SidTypeWellKnownGroup)
        {
            TraceMsg("Tried to add a well-known group! Bad... Failing.");
            hr = E_FAIL;
        }
    }
    else
    {
        // Failed to get the user's SID, just use the names provided
        hr = S_OK;
        
        // We'll get their SID once we add them
        m_pUserInfo->m_psid = NULL;
    }

#endif //0

    // Failed to get the user's SID, just use the names provided
    hr = S_OK;
    
    // We'll get their SID once we add them
    m_pUserInfo->m_psid = NULL;

    if (FAILED(hr))
    {
        LocalFree(m_pUserInfo->m_psid);
        m_pUserInfo->m_psid = NULL;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    TraceLeaveResult(hr);
}
