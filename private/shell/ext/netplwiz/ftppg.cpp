#include "stdafx.h"

#include "ftppg.h"
#include "resource.h"



INT_PTR CNetPlacesWizardPage4::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    default:
        break;
    }

    return FALSE;
}

BOOL CNetPlacesWizardPage4::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   // Trace(TRACE_LEVEL_FLOW, TEXT("Entering WizardPasswordPage_Init\n"));
    
    // Default to true because we only get to this dialog if the user
    // didn't specify one.  Besides, that's how users typically get into
    // the server.

    // This looks like it should be localized, but no because this same
    // string is used for FTP across all languages.
    SetWindowText(GetDlgItem(hwnd, IDC_ANON_USERNAME), TEXT("Anonymous"));
    EnableWindow(GetDlgItem(hwnd, IDC_ANON_USERNAME), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_ANON_USERNAME_LABEL), FALSE);
    CheckDlgButton(hwnd, IDC_PASSWORD_ANONYMOUS, BST_CHECKED);
    _LoginChange(hwnd);

    return TRUE;
}


BOOL CNetPlacesWizardPage4::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_PASSWORD_ANONYMOUS:
        {
            _LoginChange(hwnd);
            return FALSE;
        }
    default:
        break;
    }

    return FALSE;
}


BOOL CNetPlacesWizardPage4::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code)
    {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hwnd), (PSWIZB_BACK | PSWIZB_NEXT));
            return TRUE;

        case PSN_WIZBACK:
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idWelcomePage);
            return -1;

        case PSN_WIZNEXT:
            _ChangeUrl(hwnd);
            // Figure out the appropriate next page and go to it
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idCompletionPage);
            return -1;
    }
    return FALSE;
}


void CNetPlacesWizardPage4::_LoginChange(HWND hDlg)
{
    BOOL fAnonymousLogin = IsDlgButtonChecked(hDlg, IDC_PASSWORD_ANONYMOUS);

    ShowWindow(GetDlgItem(hDlg, IDC_USER), (fAnonymousLogin ? SW_HIDE : SW_SHOW));
    ShowWindow(GetDlgItem(hDlg, IDC_USERNAME_LABEL), (fAnonymousLogin ? SW_HIDE : SW_SHOW));
    ShowWindow(GetDlgItem(hDlg, IDC_ANON_USERNAME), (fAnonymousLogin ? SW_SHOW : SW_HIDE));
    ShowWindow(GetDlgItem(hDlg, IDC_ANON_USERNAME_LABEL), (fAnonymousLogin ? SW_SHOW : SW_HIDE));

    // Hide the "You will be prompted for the password when you connect to the FTP server" text on anonymous
    EnableWindow(GetDlgItem(hDlg, IDC_PWD_PROMPT), !fAnonymousLogin);
    ShowWindow(GetDlgItem(hDlg, IDC_PWD_PROMPT), (fAnonymousLogin ? SW_HIDE : SW_SHOW));
}

void CNetPlacesWizardPage4::_ChangeUrl(HWND hDlg)
{
    BOOL fAnonymousLogin = IsDlgButtonChecked(hDlg, IDC_PASSWORD_ANONYMOUS);

    // Do they want to login with the user name?
    if (!fAnonymousLogin)
    {
        TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];

        // Yes, so get it from the editbox.
        FetchText(hDlg, IDC_USER, szUserName, ARRAYSIZE(szUserName));
        HRESULT hrSetUser = m_pdata->netplace.SetFtpUserPassword(szUserName, NULL);
    }
}
