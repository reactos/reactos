// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXCMN2_H__
#define __AFXCMN2_H__

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, on)
#endif

#ifndef IMAGE_BITMAP
#define IMAGE_BITMAP 0
#endif


/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

	#ifndef _RICHEDIT_
		#include <richedit.h>
	#endif
	#ifdef __AFXOLE_H__  // only include richole if OLE support is included
		#ifndef _RICHOLE_
			#include <richole.h>
			#define _RICHOLE_
		#endif
	#else
		struct IRichEditOle;
		struct IRichEditOleCallback;
	#endif

/////////////////////////////////////////////////////////////////////////////
// AFXCMN2 - RichEdit2 Control Class

// Classes declared in this file

//CObject
	//CCmdTarget;
		//CWnd
			class CRichEdit2Ctrl;

#undef AFX_DATA
#define AFX_DATA

/////////////////////////////////////////////////////////////////////////////
// CRichEdit2Ctrl

class CRichEdit2Ctrl : public CWnd
{
	DECLARE_DYNAMIC(CRichEdit2Ctrl)

// Constructors
public:
	CRichEdit2Ctrl();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
	BOOL CanUndo() const;
	int GetLineCount() const;
	BOOL GetModify() const;
	void SetModify(BOOL bModified = TRUE);
	void GetRect(LPRECT lpRect) const;
	CPoint GetCharPos(long lChar) const;
	void SetOptions(WORD wOp, DWORD dwFlags);

	// NOTE: first word in lpszBuffer must contain the size of the buffer!
	int GetLine(int nIndex, LPTSTR lpszBuffer) const;
	int GetLine(int nIndex, LPTSTR lpszBuffer, int nMaxLength) const;

	BOOL CanPaste(UINT nFormat = 0) const;
	void GetSel(long& nStartChar, long& nEndChar) const;
	void GetSel(CHARRANGE &cr) const;
	void LimitText(long nChars = 0);
	long LineFromChar(long nIndex) const;
	void SetSel(long nStartChar, long nEndChar);
	void SetSel(CHARRANGE &cr);
	DWORD GetDefaultCharFormat(CHARFORMAT &cf) const;
	DWORD GetSelectionCharFormat(CHARFORMAT &cf) const;
	long GetEventMask() const;
	long GetLimitText() const;
	DWORD GetParaFormat(PARAFORMAT &pf) const;
	// richedit EM_GETSELTEXT is ANSI
	long GetSelText(LPSTR lpBuf) const;
	CString GetSelText() const;
	WORD GetSelectionType() const;
	COLORREF SetBackgroundColor(BOOL bSysColor, COLORREF cr);
	BOOL SetDefaultCharFormat(CHARFORMAT &cf);
	BOOL SetSelectionCharFormat(CHARFORMAT &cf);
	BOOL SetWordCharFormat(CHARFORMAT &cf);
	DWORD SetEventMask(DWORD dwEventMask);
	BOOL SetParaFormat(PARAFORMAT &pf);
	BOOL SetTargetDevice(HDC hDC, long lLineWidth);
	BOOL SetTargetDevice(CDC &dc, long lLineWidth);
	long GetTextLength() const;
	BOOL SetReadOnly(BOOL bReadOnly = TRUE);
	int GetFirstVisibleLine() const;

// Operations
	void EmptyUndoBuffer();

	int LineIndex(int nLine = -1) const;
	int LineLength(int nLine = -1) const;
	void LineScroll(int nLines, int nChars = 0);
	void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
	void SetRect(LPCRECT lpRect);

	BOOL DisplayBand(LPRECT pDisplayRect);
	long FindText(DWORD dwFlags, FINDTEXTEX* pFindText) const;
	long FormatRange(FORMATRANGE* pfr, BOOL bDisplay = TRUE);
	void HideSelection(BOOL bHide, BOOL bPerm);
	void PasteSpecial(UINT nClipFormat, DWORD dvAspect = 0, HMETAFILE hMF = 0);
	void RequestResize();
	long StreamIn(int nFormat, EDITSTREAM &es);
	long StreamOut(int nFormat, EDITSTREAM &es);

	// Clipboard operations
	BOOL Undo();
	void Clear();
	void Copy();
	void Cut();
	void Paste();

// OLE support
	IRichEditOle* GetIRichEditOle() const;
	BOOL SetOLECallback(IRichEditOleCallback* pCallback);

// Implementation
public:
	virtual ~CRichEdit2Ctrl();
};
/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXCMN_INLINE inline
#include <afxcmn2.inl>
#undef _AFXCMN_INLINE
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, off)
#endif

#endif //__AFXCMN2_H__

/////////////////////////////////////////////////////////////////////////////
