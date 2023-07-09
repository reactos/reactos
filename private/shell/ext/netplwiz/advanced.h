#ifndef ADVANCED_H_INCLUDED
#define ADVANCED_H_INCLUDED

#include "dialog.h"

class CAdvancedDialog : public CDialog
{
public:
    CAdvancedDialog(NETPLACESDATA* pdata): m_pdata(pdata) {}

protected:
    // Message handlers
    virtual BOOL DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

    // Helper functions
    void FindUser(HWND hwndDlg, UINT uiTextLocation);

    // Data
    NETPLACESDATA* m_pdata;
};

#endif // !ADVANCED_H_INCLUDED