// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXDLGS2_H__
#define __AFXDLGS2_H__

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif

#ifndef _INC_COMMDLG
	#include <commdlg.h>    // common dialog APIs
#endif

	#ifndef _RICHEDIT_
		#include <richedit.h>
	#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, on)
#endif

#ifndef _AFX_NOFORCE_LIBS
#ifndef _MAC

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#else //!_MAC

/////////////////////////////////////////////////////////////////////////////
// Mac libraries

#endif //_MAC
#endif //!_AFX_NOFORCE_LIBS

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXDLGS - MFC Standard dialogs

// Classes declared in this file

	// CDialog
		//class CCommonDialog;  // implementation base class

			class CFontDialog2;    // Font chooser dialog

/////////////////////////////////////////////////////////////////////////////

#undef AFX_DATA
#define AFX_DATA

/////////////////////////////////////////////////////////////////////////////
// CFontDialog2 - used to select a font

class CFontDialog2 : public CCommonDialog
{
	DECLARE_DYNAMIC(CFontDialog2)

public:
// Attributes
	// font choosing parameter block
	CHOOSEFONT m_cf;

// Constructors
	CFontDialog2(LPLOGFONT lplfInitial = NULL,
		DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
		CDC* pdcPrinter = NULL,
		CWnd* pParentWnd = NULL);
	CFontDialog2(const CHARFORMAT& charformat,
		DWORD dwFlags = CF_SCREENFONTS,
		CDC* pdcPrinter = NULL,
		CWnd* pParentWnd = NULL);
// Operations
	virtual int DoModal();

	// Get the selected font (works during DoModal displayed or after)
	void GetCurrentFont(LPLOGFONT lplf);

	// Helpers for parsing information after successful return
	CString GetFaceName() const;  // return the face name of the font
	CString GetStyleName() const; // return the style name of the font
	int GetSize() const;          // return the pt size of the font
	COLORREF GetColor() const;    // return the color of the font
	int GetWeight() const;        // return the chosen font weight
	BOOL IsStrikeOut() const;     // return TRUE if strikeout
	BOOL IsUnderline() const;     // return TRUE if underline
	BOOL IsBold() const;          // return TRUE if bold font
	BOOL IsItalic() const;        // return TRUE if italic font
	void GetCharFormat(CHARFORMAT& cf) const;

// Implementation
	LOGFONT m_lf; // default LOGFONT to store the info
	DWORD FillInLogFont(const CHARFORMAT& cf);

#ifdef _DEBUG
public:
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	TCHAR m_szStyleName[64]; // contains style name after return
};


#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXDLGS_INLINE inline
#include <afxdlgs2.inl>
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, off)
#endif

#endif //__AFXDLGS2_H__

/////////////////////////////////////////////////////////////////////////////
