#ifndef PAGE2_H_INCLUDED
#define PAGE2_H_INCLUDED

#include "dialog.h"

class CNetPlacesWizardPage2 : public CPropertyPage
{
public:
    CNetPlacesWizardPage2(NETPLACESWIZARDDATA* pdata): m_pdata(pdata) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnDestroy(HWND hwnd);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);

    // Helper functions
    BOOL AddShareNamesToList(HWND hwndList);
    void GetFolderChoice(HWND hwndList, int iItem);

    // Data
    NETPLACESWIZARDDATA* m_pdata;
    BOOL m_fListValid;
};

#endif // !PAGE2_H_INCLUDED
