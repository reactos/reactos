//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      htmlpage.cpp
//
//  Contents:  CHtmlPropPage implimentation
//
//  History:   15-Jan-97 EricB Created
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "htmlpage.h"
#pragma hdrstop

const int MINURLLEN = 2; // BUGBUG: is there a better constant for this?

#define WM_USER_DOINIT (WM_USER + 1000)

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::CHtmlPropPage
//
//  Sysnopsis:  Constructor.
//
//-----------------------------------------------------------------------------
CHtmlPropPage::CHtmlPropPage(void) :
    m_hPage(NULL),
    m_hInst(NULL),
    m_pPropView(NULL),
    m_pszUrl(NULL),
    m_fDirty(FALSE),
    m_fInInit(FALSE)
{
#ifdef _DEBUG
    strcpy(szClass, "CHtmlPropPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::~CHtmlPropPage
//
//  Sysnopsis:  Destructor.
//
//-----------------------------------------------------------------------------
CHtmlPropPage::~CHtmlPropPage(void)
{
    if (m_pszUrl != NULL)
    {
        delete m_pszUrl;
    }
    if (m_pPropView != NULL)
    {
        delete m_pPropView;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::CreatePage
//
//  Sysnopsis:  Creates a new page instance using the given params.
//
//-----------------------------------------------------------------------------
HRESULT
CHtmlPropPage::CreatePage(LPCTSTR pszUrl, LPCTSTR pszTabTitle,
                          HINSTANCE hInstance, HPROPSHEETPAGE * phPage)
{
    // TODO: wrap this in a try/catch block
    int cch = lstrlen(pszUrl);

    if (IsBadStringPtr(pszUrl, cch) || cch < MINURLLEN ||
        IsBadWritePtr(phPage, sizeof(HPROPSHEETPAGE*)))
    {
        return E_INVALIDARG;
    }

    if (pszTabTitle != NULL && IsBadStringPtr(pszTabTitle, lstrlen(pszTabTitle)))
    {
        //
        // BUGBUG: are we going to allow null titles?
        //
        return E_INVALIDARG;
    }

    CHtmlPropPage * pPageObj = new CHtmlPropPage;
    if (pPageObj == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pPageObj->m_hInst = hInstance;

    pPageObj->m_pszUrl = new TCHAR[cch + 1];
    if (pPageObj->m_pszUrl == NULL)
    {
        delete pPageObj;
        return E_OUTOFMEMORY;
    }
    lstrcpy(pPageObj->m_pszUrl, pszUrl);

    PROPSHEETPAGE   psp;

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_USECALLBACK | PSP_USETITLE;
    psp.pszTemplate = (LPCTSTR)IDD_PROPPAGE;
    psp.pfnDlgProc  = StaticDlgProc;
    psp.pfnCallback = PageRelease;
    psp.pcRefParent = NULL; // do not set PSP_USEREFPARENT
    psp.lParam      = (LPARAM) pPageObj;
    psp.hInstance   = hInstance;
    psp.pszTitle    = pszTabTitle;

    *phPage = CreatePropertySheetPage(&psp);

    if (*phPage == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnDoInit
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnDoInit()
{
    //
    // Enable the apply button.
    // BUGBUG: if we can get control changed notification from the web
    // browser, then move this code to the change notify proc.
    //
    PropSheet_Changed(GetParent(m_hPage), m_hPage);

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnInitDialog
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnInitDialog(HWND hPage, LPARAM lParam)
{
    HRESULT hr;
    HWND hwndStatic = GetDlgItem(hPage, IDC_HTMLSITE);

    m_hPage = hPage;

    //
    // Ensure that the parent control is sized to its max
    //
    RECT rc;
    GetClientRect(GetParent(hwndStatic), &rc);
    SetWindowPos(hwndStatic, NULL, 0,0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER|SWP_NOACTIVATE);

    hr = CPropView::Create(hwndStatic, m_hInst, m_pszUrl, &m_pPropView);

    if (FAILED(hr) || m_pPropView == NULL)
    {
        return FALSE;
    }

    //
    // Post a message to complete the initialization.
    //
    PostMessage(hPage, WM_USER_DOINIT, 0, 0);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::StaticDlgProc
//
//  Sysnopsis:  static dialog proc
//
//-----------------------------------------------------------------------------
BOOL CALLBACK
CHtmlPropPage::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHtmlPropPage *pThis = (CHtmlPropPage *)GetWindowLong(hDlg, DWL_USER);

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

        pThis = (CHtmlPropPage *) ppsp->lParam;
        pThis->m_hPage = hDlg;

        SetWindowLong(hDlg, DWL_USER, (LONG)pThis);
    }

    if (pThis != NULL)
    {
        return pThis->DlgProc(hDlg, uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hDlg, uMsg, wParam, lParam);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::DlgProc
//
//  Sysnopsis:  per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        m_fInInit = TRUE;
        hr = OnInitDialog(hDlg, lParam);
        m_fInInit = FALSE;
        return hr;

    case WM_USER_DOINIT:
        return OnDoInit();

    case WM_NOTIFY:
        return OnNotify(uMsg, wParam, lParam);

//    case WM_MOVE:
//    case WM_WINDOWPOSCHANGED:
    case WM_SHOWWINDOW:
        return OnShowWindow();

    case WM_SETFOCUS:
        return OnSetFocus((HWND)wParam);

    case WM_HELP:
        return OnHelp(lParam);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_DESTROY:
        return OnDestroy();

    default:
        return(FALSE);
    }

    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnNotify
//
//  Sysnopsis:  Handles notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnNotify(UINT uMessage, UINT uParam, LPARAM lParam)
{
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
        return OnApply();

    case PSN_RESET:
        OnCancel();
        return FALSE; // allow the property sheet to be destroyed.

    case PSN_SETACTIVE:
        return OnPSNSetActive(lParam);

    case PSN_KILLACTIVE:
        return OnPSNKillActive(lParam);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnCommand
//
//  Sysnopsis:  Handles the WM_COMMAND message
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnApply
//
//  Sysnopsis:  Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnApply(void)
{
    m_pPropView->OnApply();
    return PSNRET_NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnCancel
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnCancel(void)
{
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnSetFocus
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnSetFocus(HWND hwndLoseFocus)
{
    // An application should return zero if it processes this message.
    return 1;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnPSNSetActive
//
//  Sysnopsis:  Page activation event.
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnPSNSetActive(LPARAM lParam)
{
    SetWindowLong(m_hPage, DWL_MSGRESULT, 0);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnPSNKillActive
//
//  Sysnopsis:  Page deactivation event (when other pages cover this one).
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnPSNKillActive(LPARAM lParam)
{
    SetWindowLong(m_hPage, DWL_MSGRESULT, 0);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnDestroy
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnDestroy(void)
{
    // If an application processes this message, it should return zero.
    return 1;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::OnShowWindow
//
//  Sysnopsis:  On dialog window show operations, resizes the view window.
//
//-----------------------------------------------------------------------------
LRESULT
CHtmlPropPage::OnShowWindow(void)
{
    if (m_pPropView == NULL)
    {
        return 0;
    }

    TRACE(TEXT("CHtmlPropPage::OnShowWindow: moving view window\n"));

    RECT rc;
    GetWindowRect(GetDlgItem(m_hPage, IDC_HTMLSITE), &rc);

    POINT ptTop, ptBot;
    ptTop.x = rc.left;
    ptTop.y = rc.top;
    ptBot.x = rc.right;
    ptBot.y = rc.bottom;
    ScreenToClient(m_hPage, &ptTop);
    ScreenToClient(m_hPage, &ptBot);
    // BUGBUG: does bRepaint need to be TRUE?
    MoveWindow(m_pPropView->m_hWnd, ptTop.x, ptTop.y,
               ptBot.x - ptTop.x, ptBot.y - ptTop.y, TRUE);
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmlPropPage::PageRelease
//
//  Sysnopsis:  Callback used to free the CHtmlPropPage object when the
//              property sheet has been destroyed.
//
//-----------------------------------------------------------------------------
UINT CALLBACK
CHtmlPropPage::PageRelease(HWND hDlg, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    if (uMsg == PSPCB_RELEASE)
    {
        //
        // Determine instance that invoked this static function
        //

        CHtmlPropPage *pThis = (CHtmlPropPage *) ppsp->lParam;

        delete pThis;

        SetWindowLong(hDlg, DWL_USER, (LONG)NULL);
    }

    return TRUE;
}

