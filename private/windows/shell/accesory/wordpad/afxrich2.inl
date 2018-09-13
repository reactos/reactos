// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXRICH2.H

// CRichEdit2View
_AFXRICH2_INLINE CRichEdit2Ctrl& CRichEdit2View::GetRichEditCtrl() const
	{ return *(CRichEdit2Ctrl*)this; }
int _AFXRICH2_INLINE CRichEdit2View::GetPrintWidth() const
	{ return m_sizePaper.cx - m_rectMargin.left - m_rectMargin.right;}
CRect _AFXRICH2_INLINE CRichEdit2View::GetPrintRect() const
	{ return CRect(m_rectMargin.left, m_rectMargin.top, m_sizePaper.cx - m_rectMargin.right, m_sizePaper.cy - m_rectMargin.bottom); }
CRect _AFXRICH2_INLINE CRichEdit2View::GetPageRect() const
	{ return CRect(CPoint(0,0), m_sizePaper); }
void _AFXRICH2_INLINE CRichEdit2View::SetPaperSize(CSize sizePaper)
	{ m_sizePaper = sizePaper; }
CSize _AFXRICH2_INLINE CRichEdit2View::GetPaperSize() const
	{ return m_sizePaper; }
void _AFXRICH2_INLINE CRichEdit2View::SetMargins(const CRect& rectMargin)
	{ m_rectMargin = rectMargin; }
CRect _AFXRICH2_INLINE CRichEdit2View::GetMargins() const
	{ return m_rectMargin; }

_AFXRICH2_INLINE long CRichEdit2View::GetTextLength() const
	{ return GetRichEditCtrl().GetTextLength(); }
_AFXRICH2_INLINE CRichEdit2Doc* CRichEdit2View::GetDocument() const
{
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRichEdit2Doc)));
	return (CRichEdit2Doc*)m_pDocument;
}
_AFXRICH2_INLINE int CRichEdit2Doc::GetStreamFormat() const
	{ return m_bRTF ? SF_RTF : SF_TEXT; }
_AFXRICH2_INLINE BOOL CRichEdit2Doc::IsUnicode() const
    { return m_bUnicode; }
_AFXRICH2_INLINE void CRichEdit2Doc::InvalidateObjectCache()
	{ m_bUpdateObjectCache = TRUE; }

_AFXRICH2_INLINE void CRichEdit2CntrItem::Mark(BOOL b)
	{ m_bMark = b; }
_AFXRICH2_INLINE BOOL CRichEdit2CntrItem::IsMarked()
	{ return m_bMark||m_bLock; }
_AFXRICH2_INLINE CRichEdit2Doc* CRichEdit2CntrItem::GetDocument()
	{ return (CRichEdit2Doc*)COleClientItem::GetDocument(); }
_AFXRICH2_INLINE CRichEdit2View* CRichEdit2CntrItem::GetActiveView()
	{ return (CRichEdit2View*)COleClientItem::GetActiveView(); }

/////////////////////////////////////////////////////////////////////////////
