#ifndef PAGE3_H_INCLUDED
#define PAGE3_H_INCLUDED

#include "dialog.h"

class CNetPlacesWizardPage3 : public CPropertyPage
{
public:
    CNetPlacesWizardPage3(NETPLACESWIZARDDATA* pdata): m_pdata(pdata) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);

    // Helpers
    BOOL IsReconnectSet();
    
    // Data
    NETPLACESWIZARDDATA* m_pdata;
};

#endif // !PAGE3_H_INCLUDED
