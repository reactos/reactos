// buttondi.cpp : implementation file
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
#include "resource.h"
#include "buttondi.h"
#include "strings.h"
#include "wordpad.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifndef DS_CONTEXTHELP
#define DS_CONTEXTHELP 0x2000L
#endif

static const int nFontSize = 10;

/////////////////////////////////////////////////////////////////////////////
// CButtonDialog dialog

int CButtonDialog::DisplayMessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption,
	LPCTSTR lpszButtons, WORD wStyle, int nDef, int nCancel,
	DWORD* pHelpIDs, CWnd* pParentWnd)
{
	CButtonDialog dlg(lpszText, lpszCaption, lpszButtons, wStyle, pHelpIDs,
		pParentWnd);
	dlg.SetDefault(nDef);
	dlg.SetCancel(nCancel);
	return dlg.DoModal();
}

CButtonDialog::CButtonDialog(LPCTSTR lpszText, LPCTSTR lpszCaption,
	LPCTSTR lpszButtons, WORD wStyle, DWORD* pHelpIDs ,
	CWnd* pParentWnd) : CCSDialog()
{

	ASSERT(lpszText != NULL);
	ASSERT(lpszCaption != NULL);
	if (HIWORD(lpszText) == NULL)
		VERIFY(m_strText.LoadString(LOWORD((DWORD)lpszText)));
	else
		m_strText = lpszText;
	if (HIWORD(lpszCaption) == NULL)
		VERIFY(m_strCaption.LoadString(LOWORD((DWORD)lpszCaption)));
	else
		m_strCaption = lpszCaption;
	if (lpszButtons != NULL)
		AddButtons(lpszButtons);

	m_pParentWnd = pParentWnd;
	m_nDefButton = 0;
	m_nCancel = -1;
	m_pButtons = NULL;
	m_wStyle = wStyle;
	m_nBaseID = nFontSize; // don't use IDOK, IDCANCEL, etc
	m_hDlgTmp = NULL;

	LOGFONT lf;
	memcpy(&lf, &theApp.m_lf, sizeof(LOGFONT));
	lf.lfWeight = FW_NORMAL;
    lf.lfWidth = 0; 
	VERIFY(m_font.CreateFontIndirect(&lf));

	m_pHelpIDs = pHelpIDs;
}

CButtonDialog::~CButtonDialog()
{
	delete [] m_pButtons;
	if (m_hDlgTmp != NULL)
		GlobalFree(m_hDlgTmp);
}

BEGIN_MESSAGE_MAP(CButtonDialog, CCSDialog)
	//{{AFX_MSG_MAP(CButtonDialog)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CButtonDialog message handlers

int CButtonDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (m_pHelpIDs != NULL)
	{
		for (int i=0;i<m_strArray.GetSize();i++)
			m_pHelpIDs[i*2] = i+m_nBaseID;
	}
	if (CCSDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	SetWindowText(m_strCaption);
	m_pButtons = new CButton[m_strArray.GetSize()];

	CRect rect(0, 0, 10, 10);
	if (!m_staticIcon.Create(NULL,
		SS_ICON | WS_GROUP | WS_CHILD | WS_VISIBLE, rect, this))
	{
		return -1;
	}
	m_staticIcon.SetIcon(::LoadIcon(NULL, GetIconID(m_wStyle)));

	if (!m_staticText.Create(m_strText, SS_LEFT | SS_NOPREFIX | WS_GROUP |
		WS_CHILD | WS_VISIBLE, rect, this))
	{
		return -1;
	}
	m_staticText.SetFont(&m_font);
	
	for (int i=0;i<m_strArray.GetSize();i++)
	{
		if (!m_pButtons[i].Create(m_strArray[i], WS_TABSTOP | WS_CHILD |
			WS_VISIBLE | ((i == 0) ? WS_GROUP : 0) |
		    ((i == m_nDefButton) ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
		    rect, this, i+m_nBaseID))
		{
			return -1;
		}
		m_pButtons[i].SetFont(&m_font);
	}
	PositionControls();
	return 0;
}

BOOL CButtonDialog::OnInitDialog()
{
	CCSDialog::OnInitDialog();
	if (m_pHelpIDs == NULL) // no context help
		ModifyStyleEx(WS_EX_CONTEXTHELP, 0); //remove

	m_pButtons[m_nDefButton].SetFocus();	
	return FALSE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CButtonDialog operations

void CButtonDialog::AddButtons(LPCTSTR lpszButton)
{
	CString str, strButtons;
	int i=0;
	if (HIWORD(lpszButton) == NULL)
		strButtons.LoadString(LOWORD((DWORD)lpszButton));
	else
		strButtons = lpszButton;
	while (AfxExtractSubString(str, strButtons, i++, '\n'))
		AddButton(str);
}

#ifndef DS_3DLOOK
#define DS_3DLOOK 0x4
#endif

void CButtonDialog::FillInHeader(LPDLGTEMPLATE lpDlgTmp)
{
	USES_CONVERSION;
	lpDlgTmp->style = DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_VISIBLE |
		WS_CAPTION | WS_SYSMENU;
	if (theApp.m_bWin4)
		lpDlgTmp->style |= DS_CONTEXTHELP;
	lpDlgTmp->dwExtendedStyle = 0;
	lpDlgTmp->cdit = 0;
	lpDlgTmp->x = 0;
	lpDlgTmp->y = 0;
	lpDlgTmp->cx = 100;
	lpDlgTmp->cy = 100;
	
	LPWSTR lpStr = (LPWSTR)(lpDlgTmp + 1); /* Move ptr to the variable fields */

	*lpStr++ = 0;  /* No Menu resource for Message Box */
	*lpStr++ = 0;  /* No Class name for MessageBox */

	int nLen = m_strCaption.GetLength();
	wcscpy(lpStr, T2CW(m_strCaption));

	lpStr += nLen+1;
	WORD* pWord = (WORD*)lpStr;
	*pWord = 10; // 10 pt font
	pWord++;
	lpStr = (LPWSTR) pWord;

	wcscpy(lpStr, T2W(theApp.m_lf.lfFaceName));
}

/////////////////////////////////////////////////////////////////////////////
// CButtonDialog overridables

BOOL CButtonDialog::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	if (wParam == IDCANCEL && m_nCancel != -1)
	{
		EndDialog(m_nCancel);
		return TRUE;
	}
	if (::GetDlgItem(m_hWnd, wParam)==NULL)
		return FALSE;
	EndDialog(wParam-m_nBaseID);
	return TRUE;
}

int CButtonDialog::DoModal()
{
	ASSERT(m_strArray.GetSize() != 0);
	if (m_strArray.GetSize() == 0)
		return (m_nCancel != -1) ? m_nCancel : 0;

	// compute size of header
	// Fixed portions of DLG template header = sizeof(DLGTEMPLATE);
    // One null byte for menu name and one for class name = 2
	// Caption text plus NULL = m_strCaption.GetLength()+1
	int nSize = sizeof(DLGTEMPLATE);
	nSize += (2 + m_strCaption.GetLength()+1+lstrlen(theApp.m_lf.lfFaceName)+1)*2 +sizeof(WORD);
	m_hDlgTmp = GlobalAlloc(GPTR, nSize);
	if (m_hDlgTmp == NULL)
		return IDCANCEL;
	LPDLGTEMPLATE lpDlgTmp = (LPDLGTEMPLATE)GlobalLock(m_hDlgTmp);
	FillInHeader(lpDlgTmp);
	GlobalUnlock(m_hDlgTmp);
	InitModalIndirect(m_hDlgTmp);	

	return CCSDialog::DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CButtonDialog implementation

void CButtonDialog::PositionControls()
{
	CSize sizeBase = GetBaseUnits();
	int nButtonHeight = (sizeBase.cy*14)/8;
	int nHeight = 0;
	int nSep,nLeftMargin,nRightMargin,nTopMargin,nBottomMargin;
    int nButtonAdj;
    int nWidth = 0;
    CRect rectText;

	// a) 5/8 screen Width
	// b) Caption
	// c) nLeftMargin ICON nSep TEXT nRightMargin
	// d) nLeftMargin Button1 nSep Button2 ... nRightMargin
	// client width is max(b,d, min(c,a))

	CSize sizeIcon(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
	nSep = sizeIcon.cx/2;
	nLeftMargin = nSep;
	nRightMargin = nSep;
	nTopMargin = nSep;
	nBottomMargin = nSep;
	
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_font);
	
	nButtonAdj = dc.GetTextExtent(_T("XXX"),3).cx; // padding on buttons
	
	int nScreenWidth58 = dc.GetDeviceCaps(HORZRES)*5/8;
	int nCaptionWidth = dc.GetTextExtent(m_strCaption, m_strCaption.
		GetLength()).cx;
	CSize sizeText = dc.GetTextExtent(m_strText,m_strText.GetLength());
	int nTextIconWidth = nLeftMargin+sizeIcon.cx+nSep+sizeText.cx+nRightMargin;
	int nButtons = m_strArray.GetSize();
	int nButtonsWidth = nLeftMargin+nRightMargin+(nButtons-1)*nSep;
	for (int i=0;i<nButtons;i++)
	{
		nButtonsWidth +=
			dc.GetTextExtent(m_strArray[i],m_strArray[i].GetLength()).cx+
			nButtonAdj*2;
	}
	
	nWidth = min(nTextIconWidth,nScreenWidth58);
	nWidth = max(nWidth, nCaptionWidth);
	nWidth = max(nWidth, nButtonsWidth);

	m_staticIcon.SetWindowPos(NULL, nLeftMargin, nTopMargin, sizeIcon.cx,
		sizeIcon.cy, SWP_NOZORDER);

	if (sizeText.cx > nWidth-nLeftMargin-nRightMargin-sizeIcon.cx-nSep)
	{
		sizeText.cx = nWidth-nLeftMargin-nRightMargin-sizeIcon.cx-nSep;
//		int nTextWidth = nWidth-nLeftMargin-nRightMargin-sizeIcon.cx-nSep;
//		rectText.SetRect(0, 0, nTextWidth, 32767);
		rectText.SetRect(0, 0, sizeText.cx, 32767);
		/* Ask DrawText for the right cy */
		sizeText.cy = dc.DrawText(m_strText, m_strText.GetLength(), &rectText,
			DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS | DT_NOPREFIX);
	}
	
	m_staticText.SetWindowPos(NULL, nSep+sizeIcon.cx+nSep, nTopMargin,
		sizeText.cx, sizeText.cy, SWP_NOZORDER);
	
	sizeText.cy = max(sizeText.cy, sizeIcon.cy); // at least icon height
	nHeight = nTopMargin + sizeText.cy + nSep + nButtonHeight + nBottomMargin;

	CRect rect;
	rect.left = (nWidth - (nButtonsWidth - nLeftMargin - nRightMargin))/2;
	rect.top = nTopMargin + sizeText.cy + nSep;
	rect.bottom = rect.top + nButtonHeight;

	for (i=0;i<m_strArray.GetSize();i++)
	{
		rect.right = rect.left + dc.GetTextExtent(m_strArray[i],m_strArray[i].GetLength()).cx +
			2*nButtonAdj;
		m_pButtons[i].MoveWindow(&rect);
		rect.left = rect.right + nSep;
	}

	rect.SetRect(0,0,nWidth,nHeight);
	CalcWindowRect(&rect);
	SetWindowPos(NULL, (dc.GetDeviceCaps(HORZRES)-rect.Width())/2,
		(dc.GetDeviceCaps(VERTRES)-rect.Height())/2, rect.Width(), rect.Height(),
		SWP_NOZORDER|SWP_NOACTIVATE);
	if(m_nCancel == -1) // no cancel button
	{
//		CMenu* pMenu = GetSystemMenu(FALSE);
//		if (pMenu != NULL)
//			pMenu->DeleteMenu(SC_CLOSE, MF_BYCOMMAND);
	}
	dc.SelectObject(pOldFont);
}

CSize CButtonDialog::GetBaseUnits()
{
	CDisplayIC dc;
	CFont* pFont = dc.SelectObject(&m_font);
	TEXTMETRIC tm;
	VERIFY(dc.GetTextMetrics(&tm));
	dc.SelectObject(pFont);
	return CSize(tm.tmAveCharWidth, tm.tmHeight);
}

LPCTSTR CButtonDialog::GetIconID(WORD wFlags)
{
	LPCTSTR lpszIcon = NULL;
	wFlags &= MB_ICONMASK;
	if (wFlags == MB_ICONHAND)
		lpszIcon = IDI_HAND;
	else if (wFlags == MB_ICONQUESTION)
		lpszIcon = IDI_QUESTION;
	else if (wFlags == MB_ICONEXCLAMATION)
		lpszIcon = IDI_EXCLAMATION;
	else if (wFlags == MB_ICONASTERISK)
		lpszIcon = IDI_ASTERISK;
	return lpszIcon;
}	
