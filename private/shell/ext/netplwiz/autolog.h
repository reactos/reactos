#ifndef AUTOLOG_H
#define AUTOLOG_H

#include "data.h"

class CAutologonUserDlg: public CDialog
{
public:
    CAutologonUserDlg(LPTSTR szInitialUser)
    {m_pszUsername = szInitialUser;}

private:
    // Dialog message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
    // Data
    LPTSTR m_pszUsername;
};

#endif AUTOLOG_H
