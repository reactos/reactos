#include "stdafx.h"

#include "password.h"

INT_PTR CPasswordDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    default:
        break;
    }

    return FALSE;
}

BOOL CPasswordDialog::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szMessage[MAX_PATH + MAX_DOMAIN + MAX_USER + 256 + 2]; szMessage[0] = 0;
    //
    // Store the passed-in information
    //

    //
    // Limit the size of the edit controls
    //
    Edit_LimitText(GetDlgItem(hwnd, IDC_USER),
                   m_cchDomainUser - 1);

    Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD),
                   m_cchPassword - 1);

    //
    // Set the username and share
    //
    SetDlgItemText(hwnd, IDC_USER, m_pszDomainUser);
    
    // We may need to generate a user name here to use if no user name was
    // passed in
    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    LPTSTR pszUserNameToUse;

    if (*m_pszDomainUser)
    {
        pszUserNameToUse = m_pszDomainUser;
    }
    else
    {
        szDomainUser[0] = 0;

        TCHAR szUser[MAX_USER + 1];
        DWORD cchUser = ARRAYSIZE(szUser);
        TCHAR szDomain[MAX_DOMAIN + 1];
        DWORD cchDomain = ARRAYSIZE(szDomain);

        GetCurrentUserAndDomainName(szUser, &cchUser, szDomain, &cchDomain);
        
        MakeDomainUserString(szDomain, szUser, szDomainUser, ARRAYSIZE(szDomainUser));
        pszUserNameToUse = szDomainUser;
    }


    FormatMessageString(IDS_PWD_STATIC, szMessage, ARRAYSIZE(szMessage), m_pszResourceName, pszUserNameToUse);

    SetDlgItemText(hwnd, IDC_MESSAGE, szMessage);

    // Now set the error message description
    TCHAR szError[512];

    DWORD dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_dwError, 0, szError, ARRAYSIZE(szError), NULL);

    if (0 == dwFormatResult)
    {
        LoadString(g_hInstance, IDS_ERR_UNEXPECTED, szError, ARRAYSIZE(szError));
    }

    SetDlgItemText(hwnd, IDC_ERROR, szError);

    return TRUE;
}

BOOL CPasswordDialog::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id) 
    {
    case IDOK:
        {
            // Read the username and password from the dialog.  Note
            // that we don't call FetchText for the password since it
            // strips leading and trailing whitespace and, for all we
            // know, that could be an important part of the password
            FetchText(hwnd, IDC_USER, m_pszDomainUser, m_cchDomainUser);

            GetDlgItemText(hwnd, IDC_PASSWORD, m_pszPassword, m_cchPassword);
        }
        // Fall through
    case IDCANCEL:
        EndDialog(hwnd, id);
        return TRUE;
    }

    return FALSE;
}
