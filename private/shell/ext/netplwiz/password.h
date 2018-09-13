#ifndef PASSWORD_H_INCLUDED
#define PASSWORD_H_INCLUDED

#include "dialog.h"

class CPasswordDialog: public CDialog
{
public:
    CPasswordDialog(TCHAR* pszResourceName, TCHAR* pszDomainUser, DWORD cchDomainUser, 
        TCHAR* pszPassword, DWORD cchPassword, DWORD dwError): 
        m_pszResourceName(pszResourceName),
        m_pszDomainUser(pszDomainUser),
        m_cchDomainUser(cchDomainUser),
        m_pszPassword(pszPassword),
        m_cchPassword(cchPassword),
        m_dwError(dwError)
        {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

    // Data
    TCHAR* m_pszResourceName;

    TCHAR* m_pszDomainUser;
    DWORD m_cchDomainUser;

    TCHAR* m_pszPassword;
    DWORD m_cchPassword;

    DWORD m_dwError;
};

#endif //!PASSWORD_H_INCLUDED
