#include "stdafx.h"
#include "password.h"
#include <wininet.h>

#include "netfinpg.h"

INT_PTR CNetPlacesWizardPage3::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    default:
        break;
    }

    return FALSE;
}

BOOL CNetPlacesWizardPage3::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if ((EN_CHANGE == codeNotify) && (IDC_NETPLNAME_EDIT == id))
    {
        TCHAR szFriendlyName[MAX_PATH];

        // Edit contents have changed - disable Finish if necessary
        DWORD dwButtons = PSWIZB_BACK;

        FetchText(hwnd, id, szFriendlyName, ARRAYSIZE(szFriendlyName));

        if (*szFriendlyName)
        {
            dwButtons |= PSWIZB_FINISH;
        }
        else
        {
            dwButtons |= PSWIZB_DISABLEDFINISH;
        }

        PropSheet_SetWizButtons(GetParent(hwnd), dwButtons);

        return TRUE;
    }

    return FALSE;
}

BOOL CNetPlacesWizardPage3::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Bold the welcome title
    SendDlgItemMessage(hwnd, IDC_COMPLETE_TITLE, WM_SETFONT, (WPARAM) GetIntroFont(hwnd), 0);
 
    Edit_LimitText(GetDlgItem(hwnd, IDC_NETPLNAME_EDIT), NETPLACE_MAX_FRIENDLY_NAME);

    return TRUE;
}

BOOL CNetPlacesWizardPage3::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code)
    {
        case PSN_SETACTIVE:
            {
                // Load the completion message
                TCHAR szMessage[MAX_STATIC + MAX_PATH];

                if (FormatMessageString(IDS_COMPLETION_STATIC, szMessage, ARRAYSIZE(szMessage), m_pdata->netplace.GetResourceName()))
                {
                    SetDlgItemText(hwnd, IDC_COMPLETION_STATIC, szMessage);
                }

                // Create a default name
            
                LPCTSTR pszFriendlyName = m_pdata->netplace.GetFriendlyName();
                if (NULL != pszFriendlyName)
                {
                    SetDlgItemText(hwnd, IDC_NETPLNAME_EDIT, pszFriendlyName);
                }
                else
                {
                    SetDlgItemText(hwnd, IDC_NETPLNAME_EDIT, _T(""));
                }

                PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_FINISH);
            }
            return TRUE;

        case PSN_WIZBACK:
            // See if the user came here from the Welcome page or the 
            // Select Folders page and go back there
            //

            // Notify the network place data structure in case it needs to do cleanup
            m_pdata->netplace.Cancel();

            
            if (m_pdata->fShowFoldersPage)
            {
                // We need to remove the folder selected last time from the path
                // Find last whack '\'
                TCHAR* pchWhack = _tcsrchr(m_pdata->netplace.GetResourceName(), _T('\\'));
                if (pchWhack != NULL)
                    *pchWhack = _T('\0');

                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idFoldersPage);
            }
            else if (m_pdata->fShowFTPUserPage)
            {
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idPasswordPage);
            }
            else
            {
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idWelcomePage);
            }
            return -1;

        case PSN_WIZFINISH:
            {
                //
                // If we couldn't map the drive, return a non-zero
                // value to prevent the page from being destroyed
                //
                
                TCHAR szFriendlyName[MAX_PATH + 1];

                FetchText(hwnd, IDC_NETPLNAME_EDIT, szFriendlyName, ARRAYSIZE(szFriendlyName));

                HRESULT hrSetFriendlyName = m_pdata->netplace.SetFriendlyName(szFriendlyName);
                HRESULT hrCreatePlace;
                
                if (SUCCEEDED(hrSetFriendlyName))
                {
                    hrCreatePlace = m_pdata->netplace.CreatePlace(hwnd);
                }

                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) (SUCCEEDED(hrCreatePlace) ? 0 : GetDlgItem(hwnd, IDC_NETPLNAME_EDIT)));
               
                return TRUE;
            }
        case PSN_QUERYCANCEL:
            {
                // Notify the network place data structure in case it needs to do cleanup
                m_pdata->netplace.Cancel();

                return TRUE;
            }
    }
    return FALSE;
}