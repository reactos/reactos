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

#ifdef AFX_CMNCTL_SEG
#pragma code_seg(AFX_CMNCTL_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// _AFX_RICHEDIT2_STATE

_AFX_RICHEDIT2_STATE::~_AFX_RICHEDIT2_STATE()
{
	if (m_hInstRichEdit != NULL)
#ifndef _MAC
		::FreeLibrary(m_hInstRichEdit);
#else
		REFreeLibrary(m_hInstRichEdit);
#endif
}

_AFX_RICHEDIT2_STATE* AFX_CDECL AfxGetRichEdit2State()
{
	return _afxRichEdit2State.GetData();
}

/////////////////////////////////////////////////////////////////////////////
// CRichEdit2

BOOL CRichEdit2Ctrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	_AFX_RICHEDIT2_STATE* pState = _afxRichEdit2State;
	if (pState->m_hInstRichEdit == NULL)
	{
#ifndef _MAC
		pState->m_hInstRichEdit = LoadLibraryA("RICHED20.DLL");
#else
#ifndef _AFXDLL
		// Ordinarily the __gForceREInit reference would be produced by richedit.h, but
		// MFC builds with _WLM_NOFORCE_LIBS, which prevents richedit.h from generating
		// the reference.
		pState->m_hInstRichEdit = ((HINSTANCE)__gForceREInit), RELoadLibrary();
#else
		pState->m_hInstRichEdit = RELoadLibrary();
#endif
#endif
		if (pState->m_hInstRichEdit == NULL)
			return FALSE;
	}

	CWnd* pWnd = this;

	return pWnd->Create(_T("RICHEDIT"), NULL, dwStyle, rect, pParentWnd, nID);
}

int CRichEdit2Ctrl::GetLine(int nIndex, LPTSTR lpszBuffer) const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex,
		(LPARAM)lpszBuffer);
}

int CRichEdit2Ctrl::LineIndex(int nLine /* = -1 */) const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, EM_LINEINDEX, nLine, 0);
}

int CRichEdit2Ctrl::LineLength(int nLine /* = -1 */) const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, EM_LINELENGTH, nLine, 0);
}

void CRichEdit2Ctrl::LineScroll(int nLines, int nChars /* = 0 */)
{
	ASSERT(::IsWindow(m_hWnd));
	::SendMessage(m_hWnd, EM_LINESCROLL, nChars, nLines);
}

void CRichEdit2Ctrl::SetSel(long nStartChar, long nEndChar)
{
	ASSERT(::IsWindow(m_hWnd));
	CHARRANGE cr;
	cr.cpMin = nStartChar;
	cr.cpMax = nEndChar;
	::SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
}

BOOL CRichEdit2Ctrl::CanPaste(UINT nFormat) const
{
	ASSERT(::IsWindow(m_hWnd));
	COleMessageFilter* pFilter = AfxOleGetMessageFilter();
	if (pFilter != NULL)
		pFilter->BeginBusyState();
	BOOL b = (BOOL)::SendMessage(m_hWnd, EM_CANPASTE, nFormat, 0L);
	if (pFilter != NULL)
		pFilter->EndBusyState();
	return b;
}

void CRichEdit2Ctrl::PasteSpecial(UINT nClipFormat, DWORD dvAspect, HMETAFILE hMF)
{
	ASSERT(::IsWindow(m_hWnd));
	REPASTESPECIAL reps;
	reps.dwAspect = dvAspect;
	reps.dwParam = (DWORD_PTR)hMF;
	::SendMessage(m_hWnd, EM_PASTESPECIAL, nClipFormat, (LPARAM)&reps);
}

int CRichEdit2Ctrl::GetLine(int nIndex, LPTSTR lpszBuffer, int nMaxLength) const
{
	ASSERT(::IsWindow(m_hWnd));
	*(LPINT)lpszBuffer = nMaxLength;
	return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex, (LPARAM)lpszBuffer);
}

void CRichEdit2Ctrl::GetSel(long& nStartChar, long& nEndChar) const
{
	ASSERT(::IsWindow(m_hWnd));
	CHARRANGE cr;
	::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
	nStartChar = cr.cpMin;
	nEndChar = cr.cpMax;
}

CString CRichEdit2Ctrl::GetSelText() const
{
	ASSERT(::IsWindow(m_hWnd));

	CHARRANGE cr;
    LPTSTR    lpsz;

	cr.cpMin = cr.cpMax = 0;
	::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);

    try
    {
        // Need to use sizeof(WCHAR) to include DBCS characters
	    lpsz = (TCHAR*)_alloca((cr.cpMax - cr.cpMin + 1)*sizeof(WCHAR));

        // lpsz[0] = NULL;

        //
        // BUGBUG: The Ansi richedit2 control does not zero terminate the
        //         returned string if the current selection contains nothing
        //         but DBCS characters.
        //

        ZeroMemory(lpsz, (cr.cpMax - cr.cpMin + 1)*sizeof(WCHAR));

	    ::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpsz);
    }
    catch(...)
    {
        //
        // If _alloca fails it will throw an exception.  It's ok to return a
        // constant string here because it will immediately get copied into
        // a CString.
        //

        ASSERT(!"Out of stack space");

        lpsz = TEXT("");
    }

	return lpsz;
}

IRichEditOle* CRichEdit2Ctrl::GetIRichEditOle() const
{
	ASSERT(::IsWindow(m_hWnd));
	IRichEditOle *pRichItem = NULL;
	::SendMessage(m_hWnd, EM_GETOLEINTERFACE, 0, (LPARAM)&pRichItem);
	return pRichItem;
}

BOOL CRichEdit2Ctrl::SetDefaultCharFormat(CHARFORMAT &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
}

BOOL CRichEdit2Ctrl::SetSelectionCharFormat(CHARFORMAT &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

BOOL CRichEdit2Ctrl::SetWordCharFormat(CHARFORMAT &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION|SCF_WORD, (LPARAM)&cf);
}

DWORD CRichEdit2Ctrl::GetDefaultCharFormat(CHARFORMAT &cf) const
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 0, (LPARAM)&cf);
}

DWORD CRichEdit2Ctrl::GetSelectionCharFormat(CHARFORMAT &cf) const
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 1, (LPARAM)&cf);
}

DWORD CRichEdit2Ctrl::GetParaFormat(PARAFORMAT &pf) const
{
	ASSERT(::IsWindow(m_hWnd));
	pf.cbSize = sizeof(PARAFORMAT);
	return (DWORD)::SendMessage(m_hWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
}

BOOL CRichEdit2Ctrl::SetParaFormat(PARAFORMAT &pf)
{
	ASSERT(::IsWindow(m_hWnd));
	pf.cbSize = sizeof(PARAFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}

/////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4074)
#pragma init_seg(lib)

PROCESS_LOCAL(_AFX_RICHEDIT2_STATE, _afxRichEdit2State)

/////////////////////////////////////////////////////////////////////////////
