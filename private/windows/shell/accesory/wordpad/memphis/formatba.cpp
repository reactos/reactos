// formatba.cpp : implementation file
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

#include "wordpad.h"
#include "wordpdoc.h"
#include "wordpvw.h"
#include "formatba.h"
#include "strings.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// reserve lobyte for charset
#define PRINTER_FONT 0x0100
#define TT_FONT	0x0200
#define DEVICE_FONT 0x0400

#define BMW 16
#define BMH 15

static int nFontSizes[] =
	{8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
int CLocalComboBox::m_nFontHeight = 0;

class CFontDesc
{
public:
	CFontDesc(LPCTSTR lpszName, LPCTSTR lpszScript, BYTE nCharSet,
		BYTE nPitchAndFamily, DWORD dwFlags);
	CString m_strName;
	CString m_strScript;
	BYTE m_nCharSet;
	BYTE m_nPitchAndFamily;
	DWORD m_dwFlags;
};

CFontDesc::CFontDesc(LPCTSTR lpszName, LPCTSTR lpszScript, BYTE nCharSet,
	BYTE nPitchAndFamily, DWORD dwFlags)
{
	m_strName = lpszName;
	m_strScript = lpszScript;
	m_nCharSet = nCharSet;
	m_nPitchAndFamily = nPitchAndFamily;
	m_dwFlags = dwFlags;
}

BEGIN_MESSAGE_MAP(CFormatBar, CToolBar)
	//{{AFX_MSG_MAP(CFormatBar)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_CBN_DROPDOWN(IDC_FONTSIZE, OnFontSizeDropDown)
	ON_CBN_KILLFOCUS(IDC_FONTNAME, OnFontNameKillFocus)
	ON_CBN_KILLFOCUS(IDC_FONTSIZE, OnFontSizeKillFocus)
	ON_CBN_SETFOCUS(IDC_FONTNAME, OnComboSetFocus)
	ON_CBN_SETFOCUS(IDC_FONTSIZE, OnComboSetFocus)
	ON_CBN_CLOSEUP(IDC_FONTNAME, OnComboCloseUp)
	ON_CBN_CLOSEUP(IDC_FONTSIZE, OnComboCloseUp)
	ON_REGISTERED_MESSAGE(CWordPadApp::m_nPrinterChangedMsg, OnPrinterChanged)
	// Global help commands
END_MESSAGE_MAP()

static CSize GetBaseUnits(CFont* pFont)
{
	ASSERT(pFont != NULL);
	ASSERT(pFont->GetSafeHandle() != NULL);
	pFont = theApp.m_dcScreen.SelectObject(pFont);
	TEXTMETRIC tm;
	VERIFY(theApp.m_dcScreen.GetTextMetrics(&tm));

	theApp.m_dcScreen.SelectObject(pFont);
//	return CSize(tm.tmAveCharWidth, tm.tmHeight+tm.tmDescent);
	return CSize(tm.tmAveCharWidth, tm.tmHeight);
}

CFormatBar::CFormatBar()
{
	CFont fnt;
  	fnt.Attach(GetStockObject(theApp.m_nDefFont));
	m_szBaseUnits = GetBaseUnits(&fnt);
	CLocalComboBox::m_nFontHeight = m_szBaseUnits.cy;
}

void CFormatBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	CToolBar::OnUpdateCmdUI(pTarget, bDisableIfNoHndler);
	// don't update combo boxes if either one has the focus
	if (!m_comboFontName.HasFocus() && !m_comboFontSize.HasFocus())
		SyncToView();
}

void CFormatBar::SyncToView()
{
	USES_CONVERSION;
	// get the current font from the view and update
	CHARHDR fh;
	CHARFORMAT& cf = fh.cf;
	fh.hwndFrom = m_hWnd;
	fh.idFrom = GetDlgCtrlID();
	fh.code = FN_GETFORMAT;
	VERIFY(GetOwner()->SendMessage(WM_NOTIFY, fh.idFrom, (LPARAM)&fh));

	// the selection must be same font and charset to display correctly
	if ((cf.dwMask & (CFM_FACE|CFM_CHARSET)) == (CFM_FACE|CFM_CHARSET))
		m_comboFontName.MatchFont(cf.szFaceName, cf.bCharSet);
	else
		m_comboFontName.SetTheText(_T(""));

	// SetTwipSize only updates if different
	// -1 means selection is not a single point size
	m_comboFontSize.SetTwipSize( (cf.dwMask & CFM_SIZE) ? cf.yHeight : -1);
}

void CFormatBar::OnFontSizeDropDown()
{
	CString str;
	m_comboFontName.GetTheText(str);
	LPCTSTR lpszName = NULL;
	BOOL bPrinterFont;
	int nIndex = m_comboFontName.FindStringExact(-1, str);
	if (nIndex != CB_ERR)
	{
		CFontDesc* pDesc = (CFontDesc*)m_comboFontName.GetItemData(nIndex);
		ASSERT(pDesc != NULL);
		bPrinterFont = pDesc->m_dwFlags & PRINTER_FONT;
		lpszName = pDesc->m_strName;
	}

	int nSize = m_comboFontSize.GetTwipSize();
	if (nSize == -2) // error
	{
		AfxMessageBox(IDS_INVALID_NUMBER, MB_OK|MB_ICONINFORMATION);
		nSize = m_comboFontSize.m_nTwipsLast;
	}
	else if ((nSize >= 0 && nSize < 20) || nSize > 32760)
	{
		AfxMessageBox(IDS_INVALID_FONTSIZE, MB_OK|MB_ICONINFORMATION);
		nSize = m_comboFontSize.m_nTwipsLast;
	}

	if (bPrinterFont)
		m_comboFontSize.EnumFontSizes(m_dcPrinter, lpszName);
	else
		m_comboFontSize.EnumFontSizes(theApp.m_dcScreen, lpszName);
	m_comboFontSize.SetTwipSize(nSize);
}

void CFormatBar::OnComboCloseUp()
{
	NotifyOwner(NM_RETURN);
}

void CFormatBar::OnComboSetFocus()
{
	NotifyOwner(NM_SETFOCUS);
}

void CFormatBar::OnFontNameKillFocus()
{
	USES_CONVERSION;
	// get the current font from the view and update
	NotifyOwner(NM_KILLFOCUS);

	CCharFormat cf;
	cf.szFaceName[0] = NULL;

	// this will retrieve the font entered in the edit control
	// it tries to match the font to something already present in the combo box
	// this effectively ignores case of a font the user enters
	// if a user enters arial, this will cause it to become Arial
	CString str;
	m_comboFontName.GetTheText(str);	// returns "arial"
	m_comboFontName.SetTheText(str);					// selects "Arial"
	m_comboFontName.GetTheText(str);	// returns "Arial"

	// if font name box is not empty
	if (str[0] != NULL)
	{
		cf.dwMask = CFM_FACE | CFM_CHARSET;
		int nIndex = m_comboFontName.FindStringExact(-1, str);
		if (nIndex != CB_ERR)
		{
			CFontDesc* pDesc = (CFontDesc*)m_comboFontName.GetItemData(nIndex);
			ASSERT(pDesc != NULL);
			ASSERT(pDesc->m_strName.GetLength() < LF_FACESIZE);
			lstrcpyn(cf.szFaceName, pDesc->m_strName, LF_FACESIZE);
			cf.bCharSet = pDesc->m_nCharSet;
			cf.bPitchAndFamily = pDesc->m_nPitchAndFamily;
		}
		else // unknown font
		{
			ASSERT(str.GetLength() < LF_FACESIZE);
			lstrcpyn(cf.szFaceName, str, LF_FACESIZE);
			cf.bCharSet = DEFAULT_CHARSET;
			cf.bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		}
		SetCharFormat(cf);
	}
}

void CFormatBar::OnFontSizeKillFocus()
{
	NotifyOwner(NM_KILLFOCUS);
	int nSize = m_comboFontSize.GetTwipSize();
	if (nSize == -2)
	{
		AfxMessageBox(IDS_INVALID_NUMBER, MB_OK|MB_ICONINFORMATION);
		nSize = m_comboFontSize.m_nTwipsLast;
	}
	else if ((nSize >= 0 && nSize < 20) || nSize > 32760)
	{
		AfxMessageBox(IDS_INVALID_FONTSIZE, MB_OK|MB_ICONINFORMATION);
		nSize = m_comboFontSize.m_nTwipsLast;
	}
	else if (nSize > 0)
	{
		CCharFormat cf;
		cf.dwMask = CFM_SIZE;
		cf.yHeight = nSize;
		SetCharFormat(cf);
	}
}

LONG CFormatBar::OnPrinterChanged(UINT, LONG)
{
	theApp.CreatePrinterDC(m_dcPrinter);
	m_comboFontName.EnumFontFamiliesEx(m_dcPrinter);
	return 0;
}

int CFormatBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CToolBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	theApp.m_listPrinterNotify.AddTail(m_hWnd);
	
	CRect rect(0,0, (3*LF_FACESIZE*m_szBaseUnits.cx)/2, 200);
	if (!m_comboFontName.Create(WS_TABSTOP|WS_VISIBLE|WS_TABSTOP|
		WS_VSCROLL|CBS_DROPDOWN|CBS_SORT|CBS_AUTOHSCROLL|CBS_HASSTRINGS|
		CBS_OWNERDRAWFIXED, rect, this, IDC_FONTNAME))
	{
		TRACE0("Failed to create fontname combo-box\n");
		return -1;
	}
	m_comboFontName.LimitText(LF_FACESIZE);

	rect.SetRect(0, 0, 10*m_szBaseUnits.cx, 200);
	if (!m_comboFontSize.Create(WS_TABSTOP|WS_VISIBLE|WS_TABSTOP|
		WS_VSCROLL|CBS_DROPDOWN, rect, this, IDC_FONTSIZE))
	{
		TRACE0("Failed to create fontsize combo-box\n");
		return -1;
	}

	m_comboFontSize.LimitText(4);
	m_comboFontName.EnumFontFamiliesEx(m_dcPrinter);
	
	return 0;
}

void CFormatBar::OnDestroy()
{
	CToolBar::OnDestroy();
	POSITION pos = theApp.m_listPrinterNotify.Find(m_hWnd);
	ASSERT(pos != NULL);
	theApp.m_listPrinterNotify.RemoveAt(pos);
}

void CFormatBar::PositionCombos()
{
	CRect rect;
	// make font name box same size as font size box
	// this is necessary since font name box is owner draw
	m_comboFontName.SetItemHeight(-1, m_comboFontSize.GetItemHeight(-1));

	m_comboFontName.GetWindowRect(&rect);
	int nHeight = rect.Height();

	m_comboFontName.GetWindowRect(&rect);
	SetButtonInfo(0, IDC_FONTNAME, TBBS_SEPARATOR, rect.Width());
	GetItemRect(0, &rect); // FontName ComboBox
	m_comboFontName.SetWindowPos(NULL, rect.left,
		((rect.Height() - nHeight) / 2) + rect.top, 0, 0,
		SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

	m_comboFontSize.GetWindowRect(&rect);
	SetButtonInfo(2, IDC_FONTSIZE, TBBS_SEPARATOR, rect.Width());
	GetItemRect(2, &rect); // FontSize ComboBox
	m_comboFontSize.SetWindowPos(NULL, rect.left,
		((rect.Height() - nHeight) / 2) + rect.top, 0, 0,
		SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
}

/////////////////////////////////////////////////////////////////////////////
// CFontComboBox

BEGIN_MESSAGE_MAP(CFontComboBox, CLocalComboBox)
	//{{AFX_MSG_MAP(CFontComboBox)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	// Global help commands
END_MESSAGE_MAP()

CFontComboBox::CFontComboBox()
{
	VERIFY(m_bmFontType.LoadBitmap(IDB_FONTTYPE));
}

void CFontComboBox::OnDestroy()
{
	// destroy all the CFontDesc's
	EmptyContents();
	CLocalComboBox::OnDestroy();
}

void CFontComboBox::EmptyContents()
{
	// destroy all the CFontDesc's
	int nCount = GetCount();
	for (int i=0;i<nCount;i++)
		delete (CFontDesc*)GetItemData(i);
}

void CFontComboBox::EnumFontFamiliesEx(CDC& dc, BYTE nCharSet)
{
	CMapStringToPtr map;
	CString str;
	GetTheText(str);

	EmptyContents();
	ResetContent();
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfCharSet = nCharSet;

	if (dc.m_hDC != NULL)
	{
		if (theApp.m_bWin4)
		{
			::EnumFontFamiliesEx(dc.m_hDC, &lf,
				(FONTENUMPROC) EnumFamPrinterCallBackEx, (LPARAM) this, NULL);
		}
		else
		{
			::EnumFontFamilies(dc.m_hDC, NULL,
				(FONTENUMPROC) EnumFamPrinterCallBack, (LPARAM) this);
		}
	}
	else
	{
		HDC hDC = theApp.m_dcScreen.m_hDC;
		ASSERT(hDC != NULL);
		if (theApp.m_bWin4)
		{
			::EnumFontFamiliesEx(hDC, &lf,
				(FONTENUMPROC) EnumFamScreenCallBackEx, (LPARAM) this, NULL);
		}
		else
		{
			::EnumFontFamilies(hDC, NULL,
				(FONTENUMPROC) EnumFamScreenCallBack, (LPARAM) this);
		}
	}
	// now walk through the fonts and remove (charset) from fonts with only one

	int nCount = m_arrayFontDesc.GetSize();
	// walk through fonts adding names to string map
	// first time add value 0, after that add value 1
	for (int i = 0; i<nCount;i++)
	{
		CFontDesc* pDesc = (CFontDesc*)m_arrayFontDesc[i];
		void* pv = NULL;
		if (map.Lookup(pDesc->m_strName, pv)) // found it
		{
			if (pv == NULL) // only one entry so far
			{
				map.RemoveKey(pDesc->m_strName);
				map.SetAt(pDesc->m_strName, (void*)1);
			}
		}
		else // not found
			map.SetAt(pDesc->m_strName, (void*)0);
	}

	for (i = 0; i<nCount;i++)
	{
		CFontDesc* pDesc = (CFontDesc*)m_arrayFontDesc[i];
		CString str = pDesc->m_strName;
		void* pv = NULL;
		VERIFY(map.Lookup(str, pv));
		if (pv != NULL && !pDesc->m_strScript.IsEmpty())
		{
			str += " (";
			str += pDesc->m_strScript;
			str += ")";
		}

		int nIndex = AddString(str);
		ASSERT(nIndex >=0);
		if (nIndex >=0) //no error
			SetItemData(nIndex, (DWORD)pDesc);
	}

	SetTheText(str);
	m_arrayFontDesc.RemoveAll();
}

void CFontComboBox::AddFont(ENUMLOGFONT* pelf, DWORD dwType, LPCTSTR lpszScript)
{
	LOGFONT& lf = pelf->elfLogFont;
	if (lf.lfCharSet == MAC_CHARSET) // don't put in MAC fonts, commdlg doesn't either
		return;
	// Don't display vertical font for FE platform
	if ((GetSystemMetrics(SM_DBCSENABLED)) && (lf.lfFaceName[0] == '@'))
		return;
	// don't put in non-printer raster fonts
	CFontDesc* pDesc = new CFontDesc(lf.lfFaceName, lpszScript,
		lf.lfCharSet, lf.lfPitchAndFamily, dwType);
	m_arrayFontDesc.Add(pDesc);
}

BOOL CALLBACK AFX_EXPORT CFontComboBox::EnumFamScreenCallBack(ENUMLOGFONT* pelf,
	NEWTEXTMETRICEX* /*lpntm*/, int FontType, LPVOID pThis)
{
	// don't put in non-printer raster fonts
	if (FontType & RASTER_FONTTYPE)
		return 1;
	DWORD dwData = (FontType & TRUETYPE_FONTTYPE) ? TT_FONT : 0;
	((CFontComboBox *)pThis)->AddFont(pelf, dwData);
	return 1;
}

BOOL CALLBACK AFX_EXPORT CFontComboBox::EnumFamPrinterCallBack(ENUMLOGFONT* pelf,
	NEWTEXTMETRICEX* /*lpntm*/, int FontType, LPVOID pThis)
{
	DWORD dwData = PRINTER_FONT;
	if (FontType & TRUETYPE_FONTTYPE)
		dwData |= TT_FONT;
	else if (FontType & DEVICE_FONTTYPE)
		dwData |= DEVICE_FONT;
	((CFontComboBox *)pThis)->AddFont(pelf, dwData);
	return 1;
}

BOOL CALLBACK AFX_EXPORT CFontComboBox::EnumFamScreenCallBackEx(ENUMLOGFONTEX* pelf,
	NEWTEXTMETRICEX* /*lpntm*/, int FontType, LPVOID pThis)
{
	// don't put in non-printer raster fonts
	if (FontType & RASTER_FONTTYPE)
		return 1;
	DWORD dwData = (FontType & TRUETYPE_FONTTYPE) ? TT_FONT : 0;
	((CFontComboBox *)pThis)->AddFont((ENUMLOGFONT*)pelf, dwData, CString(pelf->elfScript));
	return 1;
}

BOOL CALLBACK AFX_EXPORT CFontComboBox::EnumFamPrinterCallBackEx(ENUMLOGFONTEX* pelf,
	NEWTEXTMETRICEX* /*lpntm*/, int FontType, LPVOID pThis)
{
	DWORD dwData = PRINTER_FONT;
	if (FontType & TRUETYPE_FONTTYPE)
		dwData |= TT_FONT;
	else if (FontType & DEVICE_FONTTYPE)
		dwData |= DEVICE_FONT;
	((CFontComboBox *)pThis)->AddFont((ENUMLOGFONT*)pelf, dwData, CString(pelf->elfScript));
	return 1;
}

void CFontComboBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(lpDIS->CtlType == ODT_COMBOBOX);
	int id = (int)(WORD)lpDIS->itemID;

	CDC *pDC = CDC::FromHandle(lpDIS->hDC);
	CRect rc(lpDIS->rcItem);
	if (lpDIS->itemState & ODS_FOCUS)
		pDC->DrawFocusRect(rc);
	int nIndexDC = pDC->SaveDC();

	CBrush brushFill;
	if (lpDIS->itemState & ODS_SELECTED)
	{
		brushFill.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else
		brushFill.CreateSolidBrush(pDC->GetBkColor());
	pDC->SetBkMode(TRANSPARENT);
	pDC->FillRect(rc, &brushFill);

	CFontDesc* pDesc= (CFontDesc*)lpDIS->itemData;
	ASSERT(pDesc != NULL);
	DWORD dwData = pDesc->m_dwFlags;
	if (dwData & (TT_FONT|DEVICE_FONT)) // truetype or device flag set by SetItemData
	{
		CDC dc;
		dc.CreateCompatibleDC(pDC);
		CBitmap* pBitmap = dc.SelectObject(&m_bmFontType);
		if (dwData & TT_FONT)
			pDC->BitBlt(rc.left, rc.top, BMW, BMH, &dc, BMW, 0, SRCAND);
		else // DEVICE_FONT
			pDC->BitBlt(rc.left, rc.top, BMW, BMH, &dc, 0, 0, SRCAND);
		dc.SelectObject(pBitmap);
	}
	
	rc.left += BMW + 6;
	CString strText;
	GetLBText(id, strText);
	pDC->TextOut(rc.left,rc.top,strText,strText.GetLength());

	pDC->RestoreDC(nIndexDC);
}

void CFontComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	ASSERT(lpMIS->CtlType == ODT_COMBOBOX);
	ASSERT(m_nFontHeight > 0);
	CRect rc;
	
	GetWindowRect(&rc);
	lpMIS->itemWidth = rc.Width();
	lpMIS->itemHeight = max(BMH, m_nFontHeight);
}

int CFontComboBox::CompareItem(LPCOMPAREITEMSTRUCT lpCIS)
{
	ASSERT(lpCIS->CtlType == ODT_COMBOBOX);
	int id1 = (int)(WORD)lpCIS->itemID1;
	int id2 = (int)(WORD)lpCIS->itemID2;
	CString str1,str2;
	if (id1 == -1)
		return -1;
	if (id2 == -1)
		return 1;
	GetLBText(id1, str1);
	GetLBText(id2, str2);
	return str1.Collate(str2);
}

// find a font with the face name and charset
void CFontComboBox::MatchFont(LPCTSTR lpszName, BYTE nCharSet)
{
	int nFirstIndex = FindString(-1, lpszName);
	if (nFirstIndex != CB_ERR)
	{
		int nIndex = nFirstIndex;
		do
		{
			CFontDesc* pDesc = (CFontDesc*)GetItemData(nIndex);
			ASSERT(pDesc != NULL);
			// check the actual font name to avoid matching Courier western
			// to Courier New western
			if ((nCharSet == DEFAULT_CHARSET || pDesc->m_nCharSet == nCharSet) &&
				lstrcmp(lpszName, pDesc->m_strName)==0)
			{
				//got a match
				if (GetCurSel() != nIndex)
					SetCurSel(nIndex);
				return;
			}
			nIndex = FindString(nIndex, lpszName);
		} while (nIndex != nFirstIndex);
		// loop until found or back to first item again
	}
	//enter font name
	SetTheText(lpszName, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CSizeComboBox

CSizeComboBox::CSizeComboBox()
{
	m_nTwipsLast = 0;
}

void CSizeComboBox::EnumFontSizes(CDC& dc, LPCTSTR pFontName)
{
	ResetContent();
	if (pFontName == NULL)
		return;
	if (pFontName[0] == NULL)
		return;
	
	ASSERT(dc.m_hDC != NULL);
	m_nLogVert = dc.GetDeviceCaps(LOGPIXELSY);

	::EnumFontFamilies(dc.m_hDC, pFontName,
		(FONTENUMPROC) EnumSizeCallBack, (LPARAM) this);
}

void CSizeComboBox::TwipsToPointString(LPTSTR lpszBuf, int nTwips)
{
	ASSERT(lpszBuf != NULL);
	lpszBuf[0] = NULL;
	if (nTwips >= 0)
	{
		// round to nearest half point
		nTwips = (nTwips+5)/10;
		if ((nTwips%2) == 0)
			_stprintf(lpszBuf, _T("%ld"), nTwips/2);
		else
			_stprintf(lpszBuf, _T("%.1f"), (float)nTwips/2.F);
	}
}

void CSizeComboBox::SetTwipSize(int nTwips)
{
	if (nTwips != GetTwipSize())
	{
		TCHAR buf[10];
		TwipsToPointString(buf, nTwips);
		SetTheText(buf, TRUE);
	}
	m_nTwipsLast = nTwips;
}

int CSizeComboBox::GetTwipSize()
{
	// return values
	// -2 -- error
	// -1 -- edit box empty
	// >=0 -- font size in twips
	CString str;
	GetTheText(str);
	LPCTSTR lpszText = str;

	while (*lpszText == ' ' || *lpszText == '\t')
		lpszText++;

	if (lpszText[0] == NULL)
		return -1; // no text in control

	double d = _tcstod(lpszText, (LPTSTR*)&lpszText);
	while (*lpszText == ' ' || *lpszText == '\t')
		lpszText++;

	if (*lpszText != NULL)
		return -2;   // not terminated properly

	return (d<0.) ? 0 : (int)(d*20.);
}

BOOL CALLBACK AFX_EXPORT CSizeComboBox::EnumSizeCallBack(LOGFONT FAR* /*lplf*/,
		LPNEWTEXTMETRIC lpntm, int FontType, LPVOID lpv)
{
	CSizeComboBox* pThis = (CSizeComboBox*)lpv;
	ASSERT(pThis != NULL);
	TCHAR buf[10];
	if (
		(FontType & TRUETYPE_FONTTYPE) ||
		!( (FontType & TRUETYPE_FONTTYPE) || (FontType & RASTER_FONTTYPE) )
		) // if truetype or vector font
	{
		// this occurs when there is a truetype and nontruetype version of a font
		if (pThis->GetCount() != 0)
			pThis->ResetContent();
					
		for (int i = 0; i < 16; i++)
		{
			wsprintf(buf, _T("%d"), nFontSizes[i]);
			pThis->AddString(buf);
		}
		return FALSE; // don't call me again
	}
	// calc character height in pixels
	pThis->InsertSize(MulDiv(lpntm->tmHeight-lpntm->tmInternalLeading,
		1440, pThis->m_nLogVert));
	return TRUE; // call me again
}

void CSizeComboBox::InsertSize(int nSize)
{
	ASSERT(nSize > 0);
	DWORD dwSize = (DWORD)nSize;
	TCHAR buf[10];
	TwipsToPointString(buf, nSize);
	if (FindStringExact(-1, buf) == CB_ERR)
	{
		int nIndex = -1;
		int nPos = 0;
		DWORD dw;
		while ((dw = GetItemData(nPos)) != CB_ERR)
		{
			if (dw > dwSize)
			{
				nIndex = nPos;
				break;
			}
			nPos++;
		}
		nIndex = InsertString(nIndex, buf);
		ASSERT(nIndex != CB_ERR);
		if (nIndex != CB_ERR)
			SetItemData(nIndex, dwSize);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CLocalComboBox

BEGIN_MESSAGE_MAP(CLocalComboBox, CComboBox)
	//{{AFX_MSG_MAP(CLocalComboBox)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Global help commands
END_MESSAGE_MAP()

void CLocalComboBox::GetTheText(CString& str)
{
	int nIndex = GetCurSel();
	if (nIndex == CB_ERR)
		GetWindowText(str);
	else
		GetLBText(nIndex, str);
}

void CLocalComboBox::SetTheText(LPCTSTR lpszText,BOOL bMatchExact)
{
	int idx = (bMatchExact) ? FindStringExact(-1,lpszText) :
		FindString(-1, lpszText);
	SetCurSel( (idx==CB_ERR) ? -1 : idx);
	if (idx == CB_ERR)
		SetWindowText(lpszText);
}

BOOL CLocalComboBox::LimitText(int nMaxChars)
{
	BOOL b = CComboBox::LimitText(nMaxChars);
	if (b)
		m_nLimitText = nMaxChars;
	return b;
}

int CLocalComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;
  	SendMessage(WM_SETFONT, (WPARAM)GetStockObject(theApp.m_nDefFont));
	return 0;
}

BOOL CLocalComboBox::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		CFormatBar* pBar = (CFormatBar*)GetParent();
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
			pBar->SyncToView();
			pBar->NotifyOwner(NM_RETURN);
			return TRUE;
		case VK_RETURN:
			pBar->NotifyOwner(NM_RETURN);
			return TRUE;
		case VK_TAB:
			pBar->GetNextDlgTabItem(this)->SetFocus();
			return TRUE;
		case VK_UP:
		case VK_DOWN:
			if ((GetKeyState(VK_MENU) >= 0) && (GetKeyState(VK_CONTROL) >=0) &&
				!GetDroppedState())
			{
				ShowDropDown();
				return TRUE;
			}
		}
	}
	return CComboBox::PreTranslateMessage(pMsg);
}

void CFormatBar::NotifyOwner(UINT nCode)
{
	NMHDR nm;
	nm.hwndFrom = m_hWnd;
	nm.idFrom = GetDlgCtrlID();
	nm.code = nCode;
	GetOwner()->SendMessage(WM_NOTIFY, nm.idFrom, (LPARAM)&nm);
}

void CFormatBar::SetCharFormat(CCharFormat& cf)
{
	CHARHDR fnm;
	fnm.hwndFrom = m_hWnd;
	fnm.idFrom = GetDlgCtrlID();
	fnm.code = FN_SETFORMAT;
	fnm.cf = cf;
	VERIFY(GetOwner()->SendMessage(WM_NOTIFY, fnm.idFrom, (LPARAM)&fnm));
}
