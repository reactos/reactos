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
#include "stdafx2.h"
#include <dlgs.h>

#ifdef AFX_AUX_SEG
#pragma code_seg(AFX_AUX_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

static const UINT nMsgLBSELCHANGE = ::RegisterWindowMessage(LBSELCHSTRING);
static const UINT nMsgSHAREVI = ::RegisterWindowMessage(SHAREVISTRING);
static const UINT nMsgFILEOK = ::RegisterWindowMessage(FILEOKSTRING);
static const UINT nMsgCOLOROK = ::RegisterWindowMessage(COLOROKSTRING);
static const UINT nMsgHELP = ::RegisterWindowMessage(HELPMSGSTRING);

UINT_PTR CALLBACK
_AfxCommDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (hWnd == NULL)
		return 0;
/*
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
	if (pThreadState->m_pAlternateWndInit != NULL)
	{
		ASSERT_KINDOF(CFileDialog,pThreadState->m_pAlternateWndInit);
		pThreadState->m_pAlternateWndInit->SubclassWindow(hWnd);
		pThreadState->m_pAlternateWndInit = NULL;
	}
	ASSERT(pThreadState->m_pAlternateWndInit == NULL);
*/
	if (message == WM_INITDIALOG)
		return (UINT)AfxDlgProc(hWnd, message, wParam, lParam);

	if (message == nMsgHELP ||
	   (message == WM_COMMAND && LOWORD(wParam) == pshHelp))
	{
		// just translate the message into the AFX standard help command.
		SendMessage(hWnd, WM_COMMAND, ID_HELP, 0);
		return 1;
	}

	if (message < 0xC000)
	{
		// not a ::RegisterWindowMessage message
		return 0;
	}

	// assume it is already wired up to a permanent one
	CDialog* pDlg = (CDialog*)CWnd::FromHandlePermanent(hWnd);
	ASSERT(pDlg != NULL);
	ASSERT_KINDOF(CDialog, pDlg);

	if (pDlg->IsKindOf(RUNTIME_CLASS(CFileDialog)))
	{
		// If we're exploring then we are not interested in the Registered messages
		if (((CFileDialog*)pDlg)->m_ofn.Flags & OFN_EXPLORER)
			return 0;
	}

	// RegisterWindowMessage - does not copy to lastState buffer, so
	// CWnd::GetCurrentMessage and CWnd::Default will NOT work
	// while in these handlers

	// Dispatch special commdlg messages through our virtual callbacks
	if (message == nMsgSHAREVI)
	{
		ASSERT_KINDOF(CFileDialog, pDlg);
		return ((CFileDialog*)pDlg)->OnShareViolation((LPCTSTR)lParam);
	}
	else if (message == nMsgFILEOK)
	{
		ASSERT_KINDOF(CFileDialog, pDlg);

		if (afxData.bWin4)
			((CFileDialog*)pDlg)->m_pofnTemp = (OPENFILENAME*)lParam;

		BOOL bResult = ((CFileDialog*)pDlg)->OnFileNameOK();

		((CFileDialog*)pDlg)->m_pofnTemp = NULL;

		return bResult;
	}
	else if (message == nMsgLBSELCHANGE)
	{
		ASSERT_KINDOF(CFileDialog, pDlg);
		((CFileDialog*)pDlg)->OnLBSelChangedNotify((UINT)wParam, LOWORD(lParam),
				HIWORD(lParam));
		return 0;
	}
	else if (message == nMsgCOLOROK)
	{
		ASSERT_KINDOF(CColorDialog, pDlg);
		return ((CColorDialog*)pDlg)->OnColorOK();
	}
/*
//
// _afxNMsgSETRGB causes problems with the build.   Since it's not used
// for anything anyway, don't use it.
//

	else if (message == _afxNMsgSETRGB)
	{
		// nothing to do here, since this is a SendMessage
		return 0;
	}
*/
	return 0;
}
