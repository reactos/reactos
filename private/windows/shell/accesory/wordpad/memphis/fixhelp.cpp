// fixhelp.cpp : implementation file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "fixhelp.h"

BOOL g_fDisableStandardHelp = FALSE ;

HHOOK g_HelpFixHook = (HHOOK) 0 ;

LRESULT CALLBACK HelpFixControlProc(
    HWND  hwnd,
    UINT  uMsg,
    WPARAM wParam,
    LPARAM  lParam);

LRESULT CALLBACK HelpFixDialogProc(
    HWND  hwnd,
    UINT  uMsg,
    WPARAM wParam,
    LPARAM  lParam);

LRESULT CALLBACK HelpFixHook(
    int code,
    WPARAM wParam,
    LPARAM lParam) ;

class CWordPadCWnd : public CWnd
{
public:

	LRESULT CallDWP(UINT nMsg, WPARAM wParam, LPARAM lParam)
   {
	    return DefWindowProc(nMsg, wParam, lParam) ;
   }
} ;

void FixHelp(CWnd* pWnd, BOOL fFixWndProc)
{
    //
    // Subclass the main window proc if we are supposed to
	// and if MFC has alread subclassed it
    //

    if (fFixWndProc)
    {
        if (GetWindowLong(pWnd->m_hWnd, GWL_WNDPROC) == (LONG)AfxWndProc)
	     {
              SetWindowLong(pWnd->m_hWnd, GWL_WNDPROC,
                               (LONG)HelpFixDialogProc);
	     }
    }

	//
    // Search all child windows.  If their window proc
    // is AfxWndProc, then subclass with our window proc
	//

    CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
    while(pWndChild != NULL)
    {
        if (GetWindowLong(pWndChild->GetSafeHwnd(), GWL_WNDPROC) == (LONG)AfxWndProc)
        {
            SetWindowLong(pWndChild->GetSafeHwnd(), GWL_WNDPROC,
                              (LONG)HelpFixControlProc);
        }
        pWndChild = pWndChild->GetWindow(GW_HWNDNEXT);
    }
}

LRESULT CALLBACK HelpFixControlProc(
    HWND  hwnd,
	UINT  uMsg,
	WPARAM wParam,
    LPARAM  lParam)
{
    if (uMsg == WM_HELP)
    {
		//
        // bypass MFC's handler, message will be sent to
		// parent of the control
		//

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return AfxWndProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK HelpFixDialogProc(
    HWND  hwnd,
	UINT  uMsg,
	WPARAM wParam,
    LPARAM  lParam)
{
    if (uMsg == WM_HELP)
    {
		CWordPadCWnd* pWnd = (CWordPadCWnd *) CWnd::FromHandlePermanent(hwnd) ;

		//
        // bypass MFC's handler, message will be sent to window proc for
		// the dialog box
		//

		if (NULL != pWnd)
		{
            return pWnd->CallDWP(uMsg, wParam, lParam) ;
		}
    }
    return AfxWndProc(hwnd,uMsg,wParam,lParam);
}


void SetHelpFixHook(void)
{
   g_HelpFixHook = ::SetWindowsHookEx(
                        WH_CALLWNDPROC,
                        (HOOKPROC) HelpFixHook,
                        NULL,
                        ::GetCurrentThreadId());
}

void RemoveHelpFixHook(void)
{
    ::UnhookWindowsHookEx(g_HelpFixHook) ;

    g_HelpFixHook = (HHOOK) 0 ;
}

LRESULT CALLBACK HelpFixHook(
    int code,
    WPARAM wParam,
    LPARAM lParam)
{
    if (code < 0)
    {
        return ::CallNextHookEx(
                   g_HelpFixHook,
                   code,
                   wParam,
                   lParam) ;
    }

    CWPSTRUCT *pcwps = (CWPSTRUCT *) lParam ;

    if (pcwps->message == WM_INITDIALOG)
    {
        CWnd *pWnd = CWnd::FromHandlePermanent(pcwps->hwnd) ;

        if (pWnd != NULL)
        {
            FixHelp(pWnd, TRUE) ;
        }
    }

    return ::CallNextHookEx(
                g_HelpFixHook,
                code,
                wParam,
                lParam) ;
}
