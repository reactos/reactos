#ifndef PAGE1_H_INCLUDED
#define PAGE1_H_INCLUDED

#include "dialog.h"

class CNetPlacesWizardPage1 : public CPropertyPage
{
public:
    CNetPlacesWizardPage1(NETPLACESWIZARDDATA* pdata): m_pdata(pdata), m_hwndExampleTT(NULL) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
    BOOL OnDestroy(HWND hwnd);

    // Helpers
    DWORD GetNextPage(HWND hwndDlg);
    void SetPageState(HWND hwnd);
    void InitExampleTooltip(HWND hwnd);
    void TrackExampleTooltip(HWND hwnd);
    void ShowExampleTooltip(HWND hwnd);

    // Data
    NETPLACESWIZARDDATA* m_pdata;
    HWND m_hwndExampleTT;

    // Parent subclass stuff
    HWND m_hwnd;
    WNDPROC m_pfnOldPropSheetParentProc;
    static LRESULT CALLBACK StaticParentSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK ParentSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // !PAGE1_H_INCLUDED
