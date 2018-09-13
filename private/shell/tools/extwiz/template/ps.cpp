// C$$ClassType$$PS.cpp : Implementation of C$$ClassType$$PS
#include "stdafx.h"
#include "$$root$$.h"
#include "$$ClassType$$PS.h"

BOOL CALLBACK C$$ClassType$$PS_DlgProc(HWND hwnd, UINT uMsg, 
    WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = FALSE;
    static C$$ClassType$$PS* ppse = NULL;
    
    switch(uMsg)
    {
    case WM_INITDIALOG:
        ppse = (CPS*)lParam;
        bRet = TRUE;
        break;

    case WM_NOTIFY:
        switch(((NMHDR*)lParam)->code)
        {
        case PSN_APPLY:
            // TODO: Add code to apply changes in the dialog
            SetWindowLong(hwnd, DWL_MSGRESULT, TRUE);
            bRet = TRUE;
            break;
        case PSN_KILLACTIVE:
            // page is being deactivated either by another page being 
            // activated, or user chose OK.
            SetWindowLong(hwnd, DWL_MSGRESULT, FALSE);
            bRet = TRUE;
            break;
        case PSN_RESET:
            // TODO: Add any cleanup code 
            SetWindowLong(hwnd, DWL_MSGRESULT, FALSE);
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// C$$ClassType$$PS

STDMETHODIMP C$$ClassType$$PS::Initialize (LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, 
                     HKEY hkeyProgID)
{

    return NOERROR;
}

STDMETHODIMP C$$ClassType$$PS::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam)
{
    PROPSHEETPAGE psp;     
    HPROPSHEETPAGE hpage;  
    
    psp.dwSize      = sizeof(psp);   // no extra data 
    psp.dwFlags     = PSP_USEREFPARENT; 
    psp.hInstance   = _Module.GetModuleInstance(); 
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE); 
    psp.pfnDlgProc  = C$$ClassType$$PS_DlgProc;     
    psp.pcRefParent = (UINT*)&_Module.m_nLockCnt; 
    psp.lParam      = (LPARAM)this;  
    hpage = CreatePropertySheetPage(&psp);     
    if (hpage) 
    { 
        if (!lpfnAddPage(hpage, lParam)) 
            DestroyPropertySheetPage(hpage);     
    }
    return NOERROR;
}

STDMETHODIMP C$$ClassType$$PS::ReplacePage(UINT uPageID, 
    LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    // TODO: Implement this method if you want to replace a
    // Property page in a control panel 
    return S_FALSE;
}
