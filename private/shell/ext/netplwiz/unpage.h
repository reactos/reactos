#ifndef UNPAGE_H
#define UNPAGE_H

#include "data.h"

class CUsernamePageBase
{
protected:
    CUsernamePageBase(CUserInfo* pUserInfo): m_pUserInfo(pUserInfo) {}

protected:
    // Message handlers
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

protected:
    // Data
    CUserInfo* m_pUserInfo;
};

class CUsernameWizardPage: public CPropertyPage, public CUsernamePageBase
{
public:
    CUsernameWizardPage(CUserInfo* pUserInfo): CUsernamePageBase(pUserInfo) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
    // Functions
    void SetWizardButtons(HWND hwnd, HWND hwndPropSheet);
};

class CUsernamePropertyPage: public CPropertyPage, public CUsernamePageBase
{
public:
    CUsernamePropertyPage(CUserInfo* pUserInfo): CUsernamePageBase(pUserInfo) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
};

#endif //!UNPAGE_H
