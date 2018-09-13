// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXCMN2.H

#ifdef _AFXCMN2_INLINE

_AFXCMN2_INLINE CRichEdit2Ctrl::CRichEdit2Ctrl()
	{ }
_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::CanUndo() const
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_CANUNDO, 0, 0); }
_AFXCMN2_INLINE int CRichEdit2Ctrl::GetLineCount() const
	{ ASSERT(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, EM_GETLINECOUNT, 0, 0); }
_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::GetModify() const
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_GETMODIFY, 0, 0); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::SetModify(BOOL bModified /* = TRUE */)
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETMODIFY, bModified, 0);}
_AFXCMN2_INLINE void CRichEdit2Ctrl::GetRect(LPRECT lpRect) const
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_GETRECT, 0, (LPARAM)lpRect); }
_AFXCMN2_INLINE CPoint CRichEdit2Ctrl::GetCharPos(long lChar) const
	{ ASSERT(::IsWindow(m_hWnd)); CPoint pt; ::SendMessage(m_hWnd, EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)lChar); return pt;}
_AFXCMN2_INLINE void CRichEdit2Ctrl::SetOptions(WORD wOp, DWORD dwFlags)
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETOPTIONS, (WPARAM)wOp, (LPARAM)dwFlags); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::EmptyUndoBuffer()
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EMPTYUNDOBUFFER, 0, 0); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo)
	{ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_REPLACESEL, (WPARAM) bCanUndo, (LPARAM)lpszNewText); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::SetRect(LPCRECT lpRect)
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETRECT, 0, (LPARAM)lpRect); }
_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::Undo()
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_UNDO, 0, 0); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::Clear()
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CLEAR, 0, 0); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::Copy()
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_COPY, 0, 0); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::Cut()
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CUT, 0, 0); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::Paste()
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_PASTE, 0, 0); }
_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::SetReadOnly(BOOL bReadOnly /* = TRUE */ )
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETREADONLY, bReadOnly, 0L); }
_AFXCMN2_INLINE int CRichEdit2Ctrl::GetFirstVisibleLine() const
	{ ASSERT(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0L); }
_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::DisplayBand(LPRECT pDisplayRect)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_DISPLAYBAND, 0, (LPARAM)pDisplayRect); }
_AFXCMN2_INLINE void CRichEdit2Ctrl::GetSel(CHARRANGE &cr) const
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr); }

_AFXCMN2_INLINE void CRichEdit2Ctrl::LimitText(long nChars)
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXLIMITTEXT, 0, nChars); }

_AFXCMN2_INLINE long CRichEdit2Ctrl::LineFromChar(long nIndex) const
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_EXLINEFROMCHAR, 0, nIndex); }

_AFXCMN2_INLINE void CRichEdit2Ctrl::SetSel(CHARRANGE &cr)
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr); }

#ifndef _CHICAGO_
_AFXCMN2_INLINE long CRichEdit2Ctrl::FindText(DWORD dwFlags, FINDTEXTEX* pFindText) const
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_FINDTEXTEX, dwFlags, (LPARAM)pFindText); }
#endif // !_CHICAGO_

_AFXCMN2_INLINE long CRichEdit2Ctrl::FormatRange(FORMATRANGE* pfr, BOOL bDisplay)
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_FORMATRANGE, (WPARAM)bDisplay, (LPARAM)pfr); }

_AFXCMN2_INLINE long CRichEdit2Ctrl::GetEventMask() const
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETEVENTMASK, 0, 0L); }

_AFXCMN2_INLINE long CRichEdit2Ctrl::GetLimitText() const
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETLIMITTEXT, 0, 0L); }

_AFXCMN2_INLINE long CRichEdit2Ctrl::GetSelText(LPSTR lpBuf) const
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpBuf); }

_AFXCMN2_INLINE void CRichEdit2Ctrl::HideSelection(BOOL bHide, BOOL bPerm)
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_HIDESELECTION, bHide, bPerm); }

_AFXCMN2_INLINE void CRichEdit2Ctrl::RequestResize()
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_REQUESTRESIZE, 0, 0L); }

_AFXCMN2_INLINE WORD CRichEdit2Ctrl::GetSelectionType() const
	{ ASSERT(::IsWindow(m_hWnd)); return (WORD)::SendMessage(m_hWnd, EM_SELECTIONTYPE, 0, 0L); }

_AFXCMN2_INLINE COLORREF CRichEdit2Ctrl::SetBackgroundColor(BOOL bSysColor, COLORREF cr)
	{ ASSERT(::IsWindow(m_hWnd)); return (COLORREF)::SendMessage(m_hWnd, EM_SETBKGNDCOLOR, bSysColor, cr); }

_AFXCMN2_INLINE DWORD CRichEdit2Ctrl::SetEventMask(DWORD dwEventMask)
	{ ASSERT(::IsWindow(m_hWnd)); return (DWORD)::SendMessage(m_hWnd, EM_SETEVENTMASK, 0, dwEventMask); }

_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::SetOLECallback(IRichEditOleCallback* pCallback)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETOLECALLBACK, 0, (LPARAM)pCallback); }

_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::SetTargetDevice(HDC hDC, long lLineWidth)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETTARGETDEVICE, (WPARAM)hDC, lLineWidth); }

_AFXCMN2_INLINE BOOL CRichEdit2Ctrl::SetTargetDevice(CDC &dc, long lLineWidth)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETTARGETDEVICE, (WPARAM)dc.m_hDC, lLineWidth); }

_AFXCMN2_INLINE long CRichEdit2Ctrl::StreamIn(int nFormat, EDITSTREAM &es)
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_STREAMIN, nFormat, (LPARAM)&es); }

_AFXCMN2_INLINE long CRichEdit2Ctrl::StreamOut(int nFormat, EDITSTREAM &es)
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_STREAMOUT, nFormat, (LPARAM)&es); }

_AFXCMN2_INLINE long CRichEdit2Ctrl::GetTextLength() const
	{ ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, WM_GETTEXTLENGTH, NULL, NULL); }


#endif //_AFXCMN2_INLINE

/////////////////////////////////////////////////////////////////////////////
