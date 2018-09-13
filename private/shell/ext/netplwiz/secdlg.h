#ifndef SECDLG_H
#define SECDLG_H

// The DoModal call for this dialog will return IDOK if the applet should be
// shown or IDCANCEL if the users.cpl should exit without displaying the applet.

class CSecurityCheckDlg: public CDialog
{
public:

public:
    CSecurityCheckDlg(LPCTSTR pszDomainUser):
        m_pszDomainUser(pszDomainUser)
        {}

private:
    // Dialog message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
    // Helpers
    HRESULT RelaunchAsUser(HWND hwnd);

    LPCTSTR m_pszDomainUser;
};

#endif // !SECDLG_H
