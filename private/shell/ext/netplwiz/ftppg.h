#ifndef PAGE4_H_INCLUDED
#define PAGE4_H_INCLUDED

#include "dialog.h"

class CNetPlacesWizardPage4 : public CPropertyPage
{
public:
    CNetPlacesWizardPage4(NETPLACESWIZARDDATA* pdata): m_pdata(pdata) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);

private:
    // Helpers
    void _LoginChange(HWND hDlg);
    void _ChangeUrl(HWND hDlg);

    // Data
    NETPLACESWIZARDDATA* m_pdata;
};

#endif // !PAGE4_H_INCLUDED
