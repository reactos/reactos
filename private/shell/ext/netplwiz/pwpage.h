#ifndef PWPAGE_H
#define PWPAGE_H

#include "data.h"

// Base class containing common stuff for the password prop page and set password dialog
class CPasswordPageBase
{
public:
    CPasswordPageBase(CUserInfo* pUserInfo): m_pUserInfo(pUserInfo) {}

protected:
    // Helpers
    BOOL DoPasswordsMatch(HWND hwnd);

protected:
    // Data
    CUserInfo* m_pUserInfo;
};

class CPasswordWizardPage: public CPropertyPage, public CPasswordPageBase
{
public:
    CPasswordWizardPage(CUserInfo* pUserInfo): CPasswordPageBase(pUserInfo) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
};

class CChangePasswordDlg: public CDialog, public CPasswordPageBase
{
public:
    CChangePasswordDlg(CUserInfo* pUserInfo): CPasswordPageBase(pUserInfo) {}

    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
};

#endif //!PWPAGE_H
