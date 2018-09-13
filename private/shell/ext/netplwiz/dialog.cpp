#include "stdafx.h"

#include "dialog.h"

INT_PTR CPropertyPage::StaticProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPropertyPage* pthis = (CPropertyPage*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    PROPSHEETPAGE* ppage;
    INT_PTR fProcessed;

    if (uMsg == WM_INITDIALOG)
    {
        ppage = (PROPSHEETPAGE*) lParam;
        pthis = (CPropertyPage*) ppage->lParam;
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis); 
    }

    if (pthis != NULL)
    {
        fProcessed = pthis->DialogProc(hwndDlg, uMsg, wParam, lParam);
    }
    else
    {
        fProcessed = FALSE;
    }

    return fProcessed;
}

INT_PTR CDialog::StaticProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDialog* pthis = (CDialog*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    INT_PTR fProcessed;

    if (uMsg == WM_INITDIALOG)
    {
        pthis = (CDialog*) lParam;
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis); 
    }

    if (pthis != NULL)
    {
        fProcessed = pthis->DialogProc(hwndDlg, uMsg, wParam, lParam);
    }
    else
    {
        fProcessed = FALSE;
    }

    return fProcessed;

}
