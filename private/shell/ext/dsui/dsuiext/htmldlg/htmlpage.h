//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      htmlpage.h
//
//  Contents:  CHtmlPropPage definition
//
//  History:   14-Jan-97 EricB Created
//
//-----------------------------------------------------------------------------

#ifndef _HTMLPAGE_H_
#define _HTMLPAGE_H_

#include "view.h"

class CHtmlPropPage // : public IHtmlPropPage
{
public:

#ifdef _DEBUG
    char szClass[16];
#endif

    CHtmlPropPage(void);

    //
    //  Static WndProc to be passed to PROPSHEETPAGE::pfnDlgProc
    //

    static BOOL CALLBACK StaticDlgProc(HWND hDlg,     UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);

    //
    //  Instance specific wind proc
    //

    LRESULT DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    //
    // Page/object creation member.
    //

    static HRESULT CreatePage(LPCTSTR pszUrl, LPCTSTR pszTabTitle,
                              HINSTANCE hInstance, HPROPSHEETPAGE *phPage);

private:
    ~CHtmlPropPage(void);

    //
    //  Member functions, called by WndProc
    //

    HRESULT OnInitDialog(HWND hPage, LONG lParam);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(UINT uMessage, UINT uParam, LPARAM lParam);
    LRESULT OnApply(void);
    LRESULT OnCancel(void);
    LRESULT OnSetFocus(HWND hwndLoseFocus);
    LRESULT OnShowWindow(void);
    LRESULT OnDestroy(void);
    LRESULT OnPSMQuerySibling(WPARAM wParam, LPARAM lParam);
    LRESULT OnPSNSetActive(LPARAM lParam);
    LRESULT OnPSNKillActive(LPARAM lParam);
    LRESULT OnHelp(LPARAM lParam) { return TRUE; }
    LRESULT OnDoInit();

    static  UINT CALLBACK PageRelease(HWND hwnd, UINT uMsg,
                                      LPPROPSHEETPAGE ppsp);
    //
    //  Data members
    //

    HWND            m_hPage;
    HINSTANCE       m_hInst;
    LPTSTR          m_pszUrl;
    CPropView     * m_pPropView;
    BOOL            m_fDirty;
    BOOL            m_fInInit;
};

#endif // _HTMLPAGE_H_
