#ifndef NETPAGE_H
#define NETPAGE_H

#include "data.h"

class CNetworkUserWizardPage: public CPropertyPage
{
public:
    CNetworkUserWizardPage(CUserInfo* pUserInfo);

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);


private:
    // Data
    CUserInfo* m_pUserInfo;

private:
    // Functions
    void SetWizardButtons(HWND hwnd, HWND hwndPropSheet);
    HRESULT GetUserAndDomain(HWND hwnd);
};

#endif //!NETPAGE_H
