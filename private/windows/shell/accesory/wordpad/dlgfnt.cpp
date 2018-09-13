// Copyright (C) 1992-1999 Microsoft Corporation
// All rights reserved.

#include "stdafx.h"
#include "stdafx2.h"

#ifdef AFX_AUX_SEG
#pragma code_seg(AFX_AUX_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// Choose Font dialog

CFontDialog2::CFontDialog2(LPLOGFONT lplfInitial, DWORD dwFlags, CDC* pdcPrinter,
	CWnd* pParentWnd) : CCommonDialog(pParentWnd)
{
	memset(&m_cf, 0, sizeof(m_cf));
	memset(&m_lf, 0, sizeof(m_lf));
	memset(&m_szStyleName, 0, sizeof(m_szStyleName));

	m_nIDHelp = AFX_IDD_FONT;

	m_cf.lStructSize = sizeof(m_cf);
	m_cf.lpszStyle = (LPTSTR)&m_szStyleName;
	m_cf.Flags = dwFlags | CF_ENABLEHOOK;
	if (!afxData.bWin4 && AfxHelpEnabled())
		m_cf.Flags |= CF_SHOWHELP;
	m_cf.lpfnHook = _AfxCommDlgProc;

	if (lplfInitial)
	{
		m_cf.lpLogFont = lplfInitial;
		m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
		memcpy(&m_lf, m_cf.lpLogFont, sizeof(m_lf));
	}
	else
	{
		m_cf.lpLogFont = &m_lf;
	}

	if (pdcPrinter)
	{
		ASSERT(pdcPrinter->m_hDC != NULL);
		m_cf.hDC = pdcPrinter->m_hDC;
		m_cf.Flags |= CF_PRINTERFONTS;
	}
}

CFontDialog2::CFontDialog2(const CHARFORMAT& charformat, DWORD dwFlags,
	CDC* pdcPrinter, CWnd* pParentWnd) : CCommonDialog(pParentWnd)
{
	memset(&m_cf, 0, sizeof(m_cf));
	memset(&m_lf, 0, sizeof(m_lf));
	memset(&m_szStyleName, 0, sizeof(m_szStyleName));

	m_nIDHelp = AFX_IDD_FONT;

	m_cf.lStructSize = sizeof(m_cf);
	m_cf.lpszStyle = (LPTSTR)&m_szStyleName;
	m_cf.Flags = dwFlags | CF_ENABLEHOOK | CF_INITTOLOGFONTSTRUCT;
	m_cf.Flags |= FillInLogFont(charformat);
	if (!afxData.bWin4 && AfxHelpEnabled())
		m_cf.Flags |= CF_SHOWHELP;
	m_cf.lpfnHook = _AfxCommDlgProc;

	m_cf.lpLogFont = &m_lf;

	if (pdcPrinter)
	{
		ASSERT(pdcPrinter->m_hDC != NULL);
		m_cf.hDC = pdcPrinter->m_hDC;
		m_cf.Flags |= CF_PRINTERFONTS;
	}
	if (charformat.dwMask & CFM_COLOR)
		m_cf.rgbColors = charformat.crTextColor;
}

INT_PTR CFontDialog2::DoModal()
{
	ASSERT_VALID(this);
	ASSERT(m_cf.Flags & CF_ENABLEHOOK);
	ASSERT(m_cf.lpfnHook != NULL); // can still be a user hook

	m_cf.hwndOwner = PreModal();
	int nResult = ::ChooseFont(&m_cf);
	PostModal();

	if (nResult == IDOK)
	{
		// copy logical font from user's initialization buffer (if needed)
		memcpy(&m_lf, m_cf.lpLogFont, sizeof(m_lf));
		return IDOK;
	}
	return nResult ? nResult : IDCANCEL;
}

void CFontDialog2::GetCurrentFont(LPLOGFONT lplf)
{
	ASSERT(lplf != NULL);

	if (m_hWnd != NULL)
		SendMessage(WM_CHOOSEFONT_GETLOGFONT, 0, (DWORD_PTR)lplf);
	else
		*lplf = m_lf;
}

////////////////////////////////////////////////////////////////////////////
// CFontDialog2 CHARFORMAT helpers

DWORD CFontDialog2::FillInLogFont(const CHARFORMAT& cf)
{
	USES_CONVERSION;
	DWORD dwFlags = 0;
	if (cf.dwMask & CFM_SIZE)
	{
		CDC dc;
		dc.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
		LONG yPerInch = dc.GetDeviceCaps(LOGPIXELSY);
		m_lf.lfHeight = -(int) ((cf.yHeight * yPerInch) / 1440);
	}
	else
		m_lf.lfHeight = 0;

	m_lf.lfWidth = 0;
	m_lf.lfEscapement = 0;
	m_lf.lfOrientation = 0;

	if ((cf.dwMask & (CFM_ITALIC|CFM_BOLD)) == (CFM_ITALIC|CFM_BOLD))
	{
		m_lf.lfWeight = (cf.dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
		m_lf.lfItalic = (BYTE)((cf.dwEffects & CFE_ITALIC) ? TRUE : FALSE);
	}
	else
	{
		dwFlags |= CF_NOSTYLESEL;
		m_lf.lfWeight = FW_DONTCARE;
		m_lf.lfItalic = FALSE;
	}

	if ((cf.dwMask & (CFM_UNDERLINE|CFM_STRIKEOUT|CFM_COLOR)) ==
		(CFM_UNDERLINE|CFM_STRIKEOUT|CFM_COLOR))
	{
		dwFlags |= CF_EFFECTS;
		m_lf.lfUnderline = (BYTE)((cf.dwEffects & CFE_UNDERLINE) ? TRUE : FALSE);
		m_lf.lfStrikeOut = (BYTE)((cf.dwEffects & CFE_STRIKEOUT) ? TRUE : FALSE);
	}
	else
	{
		m_lf.lfUnderline = (BYTE)FALSE;
		m_lf.lfStrikeOut = (BYTE)FALSE;
	}

	if (cf.dwMask & CFM_CHARSET)
		m_lf.lfCharSet = cf.bCharSet;
	else
		dwFlags |= CF_NOSCRIPTSEL;
	m_lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	m_lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_lf.lfQuality = DEFAULT_QUALITY;
	if (cf.dwMask & CFM_FACE)
	{
		m_lf.lfPitchAndFamily = cf.bPitchAndFamily;
		lstrcpy(m_lf.lfFaceName, cf.szFaceName);
	}
	else
	{
		m_lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
		m_lf.lfFaceName[0] = (TCHAR)0;
	}
	return dwFlags;
}

void CFontDialog2::GetCharFormat(CHARFORMAT& cf) const
{
	USES_CONVERSION;
	cf.dwEffects = 0;
	cf.dwMask = 0;
	if ((m_cf.Flags & CF_NOSTYLESEL) == 0)
	{
		cf.dwMask |= CFM_BOLD | CFM_ITALIC;
		cf.dwEffects |= (IsBold()) ? CFE_BOLD : 0;
		cf.dwEffects |= (IsItalic()) ? CFE_ITALIC : 0;
	}
	if ((m_cf.Flags & CF_NOSIZESEL) == 0)
	{
		cf.dwMask |= CFM_SIZE;
		//GetSize() returns in tenths of points so mulitply by 2 to get twips
		cf.yHeight = GetSize()*2;
	}

	if ((m_cf.Flags & CF_NOFACESEL) == 0)
	{
		cf.dwMask |= CFM_FACE;
		cf.bPitchAndFamily = m_cf.lpLogFont->lfPitchAndFamily;
		lstrcpy(cf.szFaceName, GetFaceName());
	}

	if (m_cf.Flags & CF_EFFECTS)
	{
		cf.dwMask |= CFM_UNDERLINE | CFM_STRIKEOUT | CFM_COLOR;
		cf.dwEffects |= (IsUnderline()) ? CFE_UNDERLINE : 0;
		cf.dwEffects |= (IsStrikeOut()) ? CFE_STRIKEOUT : 0;
		cf.crTextColor = GetColor();
	}
	if ((m_cf.Flags & CF_NOSCRIPTSEL) == 0)
	{
		cf.bCharSet = m_cf.lpLogFont->lfCharSet;
		cf.dwMask |= CFM_CHARSET;
	}
	cf.yOffset = 0;
}

////////////////////////////////////////////////////////////////////////////
// CFontDialog2 diagnostics

#ifdef _DEBUG
void CFontDialog2::Dump(CDumpContext& dc) const
{
	CDialog::Dump(dc);

	dc << "m_cf.hwndOwner = " << (UINT)m_cf.hwndOwner;
	dc << "\nm_cf.hDC = " << (UINT)m_cf.hDC;
	dc << "\nm_cf.iPointSize = " << m_cf.iPointSize;
	dc << "\nm_cf.Flags = " << (LPVOID)m_cf.Flags;
	dc << "\nm_cf.lpszStyle = " << m_cf.lpszStyle;
	dc << "\nm_cf.nSizeMin = " << m_cf.nSizeMin;
	dc << "\nm_cf.nSizeMax = " << m_cf.nSizeMax;
	dc << "\nm_cf.nFontType = " << m_cf.nFontType;
	dc << "\nm_cf.rgbColors = " << (LPVOID)m_cf.rgbColors;

	if (m_cf.lpfnHook == _AfxCommDlgProc)
		dc << "\nhook function set to standard MFC hook function";
	else
		dc << "\nhook function set to non-standard hook function";

	dc << "\n";
}
#endif //_DEBUG

#ifdef AFX_INIT_SEG
#pragma code_seg(AFX_INIT_SEG)
#endif

IMPLEMENT_DYNAMIC(CFontDialog2, CDialog)

////////////////////////////////////////////////////////////////////////////
