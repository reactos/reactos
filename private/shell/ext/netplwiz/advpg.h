#ifndef ADVPG_H_INCLUDED
#define ADVPG_H_INCLUDED

#include "data.h"

class CAdvancedPropertyPage: public CPropertyPage
{
public:
    CAdvancedPropertyPage(CUserManagerData* pdata): 
      m_pData(pdata),
      m_fRebootRequired(FALSE) {}

private:
    // Dialog message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnHelp(HWND hwnd, LPHELPINFO pHelpInfo);
    BOOL OnContextMenu(HWND hwnd);

private:
    // Functions

private:
    // Data
    CUserManagerData* m_pData;
    BOOL m_fRebootRequired;

};

#endif //!ADVPG_H_INCLUDED
