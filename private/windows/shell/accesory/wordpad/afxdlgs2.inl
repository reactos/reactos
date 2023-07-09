// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXDLGS2.H

#ifdef _AFXDLGS2_INLINE

_AFXDLGS2_INLINE CString CFontDialog2::GetFaceName() const
	{ return (LPCTSTR)m_cf.lpLogFont->lfFaceName; }
_AFXDLGS2_INLINE CString CFontDialog2::GetStyleName() const
	{ return m_cf.lpszStyle; }
_AFXDLGS2_INLINE int CFontDialog2::GetSize() const
	{ return m_cf.iPointSize; }
_AFXDLGS2_INLINE int CFontDialog2::GetWeight() const
	{ return (int)m_cf.lpLogFont->lfWeight; }
_AFXDLGS2_INLINE BOOL CFontDialog2::IsItalic() const
	{ return m_cf.lpLogFont->lfItalic ? TRUE : FALSE; }
_AFXDLGS2_INLINE BOOL CFontDialog2::IsStrikeOut() const
	{ return m_cf.lpLogFont->lfStrikeOut ? TRUE : FALSE; }
_AFXDLGS2_INLINE BOOL CFontDialog2::IsBold() const
	{ return m_cf.lpLogFont->lfWeight == FW_BOLD ? TRUE : FALSE; }
_AFXDLGS2_INLINE BOOL CFontDialog2::IsUnderline() const
	{ return m_cf.lpLogFont->lfUnderline ? TRUE : FALSE; }
_AFXDLGS2_INLINE COLORREF CFontDialog2::GetColor() const
	{ return m_cf.rgbColors; }

/////////////////////////////////////////////////////////////////////////////

#endif //_AFXDLGS2_INLINE
