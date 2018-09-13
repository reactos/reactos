// formatta.cpp : implementation file
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
#include "formatta.h"
#include "ddxm.h"
#include "helpids.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const DWORD CFormatTabDlg::m_nHelpIDs[] =
{
	IDC_BUTTON_SET, IDH_WORDPAD_TABSET,
	IDC_BUTTON_CLEAR, IDH_WORDPAD_TABCLEAR,
	IDC_BUTTON_CLEARALL, IDH_WORDPAD_TAB_CLEARALL,
	IDC_COMBO1, IDH_WORDPAD_TABSTOPS,
	IDC_BOX, IDH_COMM_GROUPBOX,
	0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CFormatTabDlg dialog

CFormatTabDlg::CFormatTabDlg(PARAFORMAT& pf, CWnd* pParent /*=NULL*/)
	: CCSDialog(CFormatTabDlg::IDD, pParent)
{
	m_pf = pf;
	m_tabarray = new LONG[MAX_TAB_STOPS];
	m_nCount = 0;
	if (m_pf.dwMask & PFM_TABSTOPS)
	{
		m_nCount = m_pf.cTabCount;
		ASSERT(m_pf.cTabCount <= MAX_TAB_STOPS);
		for (int i=0;i<m_pf.cTabCount;i++)
			m_tabarray[i] = m_pf.rgxTabs[i];
	}
	
	//{{AFX_DATA_INIT(CFormatTabDlg)
	//}}AFX_DATA_INIT
}

CFormatTabDlg::~CFormatTabDlg()
{
	delete [] m_tabarray;
}

void CFormatTabDlg::DoDataExchange(CDataExchange* pDX)
{
	CCSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFormatTabDlg)
	DDX_Control(pDX, IDC_BUTTON_CLEARALL, m_buttonClearAll);
	DDX_Control(pDX, IDC_BUTTON_SET, m_buttonSet);
	DDX_Control(pDX, IDC_BUTTON_CLEAR, m_buttonClear);
	DDX_Control(pDX, IDC_COMBO1, m_comboBox);
	//}}AFX_DATA_MAP
	if (!pDX->m_bSaveAndValidate)
		UpdateListBox();
}

BEGIN_MESSAGE_MAP(CFormatTabDlg, CCSDialog)
	//{{AFX_MSG_MAP(CFormatTabDlg)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnClickedClear)
	ON_BN_CLICKED(IDC_BUTTON_CLEARALL, OnClickedClearAll)
	ON_BN_CLICKED(IDC_BUTTON_SET, OnClickedSet)
	ON_CBN_EDITCHANGE(IDC_COMBO1, OnEditChange)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchange)
	ON_MESSAGE(WM_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFormatTabDlg message handlers

void CFormatTabDlg::OnClickedClear()
{
	int nTab;
	int nSel = m_comboBox.GetCurSel();
	if (nSel == CB_ERR)
	{
		CDataExchange dx(this, TRUE);
		DDX_Twips(&dx, IDC_COMBO1, nTab);
		DDV_MinMaxTwips(&dx, nTab, 0, 31680);
		if (nTab != DDXM_BLANK)
		{
				if (RemoveTabFromArray(nTab))
					UpdateListBox();
		}
	}
	else
	{
		ASSERT(nSel < m_nCount);
		RemoveTabFromArrayByIndex(nSel);
		UpdateListBox();
	}
	UpdateButtons();
	SetEditFocus();
}

void CFormatTabDlg::OnClickedClearAll()
{
	m_nCount = 0;
	m_comboBox.ResetContent();
	UpdateButtons();
	SetEditFocus();
}

void CFormatTabDlg::OnClickedSet()
{
	Set();
	UpdateButtons();
	SetEditFocus();
}

BOOL CFormatTabDlg::Set()
{
	int nTab;
	CDataExchange dx(this, TRUE);
	DDX_Twips(&dx, IDC_COMBO1, nTab);
	DDV_MinMaxTwips(&dx, nTab, 0, 31680);
	if (nTab != DDXM_BLANK)
	{
		if (m_nCount == MAX_TAB_STOPS)
		{
			AfxMessageBox(IDS_NOMORETABS);
			m_comboBox.Clear();
			return FALSE;
		}
		if (AddTabToArray(nTab))
			UpdateListBox();
		return TRUE;
	}
	return FALSE;
}

void CFormatTabDlg::SetEditFocus()
{
	m_comboBox.SetFocus();
	m_comboBox.SetEditSel(0,-1);
}

BOOL CFormatTabDlg::RemoveTabFromArray(LONG lTab)
{
	int i;
	for (i=0;i<m_nCount;i++)
	{
		if (m_tabarray[i] == lTab)
		{
			RemoveTabFromArrayByIndex(i);
			return TRUE;
		}
	}
	return FALSE;
}

void CFormatTabDlg::RemoveTabFromArrayByIndex(int nIndex)
{
	memmove(&m_tabarray[nIndex], &m_tabarray[nIndex+1],
		(m_nCount-nIndex-1)*sizeof(LONG));
	m_nCount--;
}

BOOL CFormatTabDlg::AddTabToArray(LONG lTab)
{
	int i;
	BOOL bInsert = FALSE;
	LONG lTemp;
	for (i=0;i<m_nCount;i++)
	{
		if (!bInsert && lTab < m_tabarray[i])
			bInsert = TRUE;
		else if (lTab == m_tabarray[i]) // we don't want repeats
			return FALSE;
		if (bInsert)
		{
			lTemp = m_tabarray[i];
			m_tabarray[i] = lTab;
			lTab = lTemp;
		}
	}
	m_tabarray[m_nCount++] = lTab;
	return TRUE;
}

void CFormatTabDlg::UpdateListBox()
{
	int i;
	TCHAR szT[64];
	ASSERT(m_nCount >= 0);
	m_comboBox.ResetContent();
	for (i=0;i<m_nCount;i++)
	{
		theApp.PrintTwips(szT, m_tabarray[i], 2);
		m_comboBox.AddString(szT);
	}
}

void CFormatTabDlg::OnOK()
{
	if (m_buttonSet.IsWindowEnabled())
	{
		if (!Set())
			return;
	}
	CCSDialog::OnOK();
	m_pf.cTabCount = (SHORT) m_nCount;
	for (int i=0;i<m_nCount;i++)
		m_pf.rgxTabs[i] = m_tabarray[i];
	m_pf.dwMask = PFM_TABSTOPS;
}

void CFormatTabDlg::OnEditChange()
{
	UpdateButtons();
}

void CFormatTabDlg::UpdateButton(CButton& button, BOOL b)
{
	if (b != button.IsWindowEnabled())
		button.EnableWindow(b);
}

void CFormatTabDlg::UpdateButtons()
{
	UpdateButton(m_buttonClearAll, m_nCount > 0);
	BOOL bHasText = (m_comboBox.GetWindowTextLength() > 0);
	UpdateButton(m_buttonSet, bHasText);
	UpdateButton(m_buttonClear, bHasText);
	WORD wID = LOWORD(GetDefID());
	if (bHasText && wID != IDC_BUTTON_SET)
		SetDefID(IDC_BUTTON_SET);
	else if (!bHasText && wID != IDOK)
		SetDefID(IDOK);
}

BOOL CFormatTabDlg::OnInitDialog()
{
	CCSDialog::OnInitDialog();
	UpdateButtons();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFormatTabDlg::OnSelchange()
{
	UpdateButton(m_buttonClearAll, m_nCount > 0);
	// force these since if the edit control is empty and
	// an item in the box is clicked on, the edit control will
	// not be filled in first
	UpdateButton(m_buttonSet, TRUE);
	UpdateButton(m_buttonClear, TRUE);
	WORD wID = LOWORD(GetDefID());
	if (wID != IDC_BUTTON_SET)
		SetDefID(IDC_BUTTON_SET);
}


LONG CFormatTabDlg::OnHelp(UINT, LONG lParam)
{
	LPHELPINFO phi = (LPHELPINFO) lParam ;
	HWND hWndCombo = ::GetDlgItem(m_hWnd, IDC_COMBO1) ;

	HWND hWndItem = (HWND) phi->hItemHandle ;

	if (::GetParent(hWndItem) == hWndCombo)
    {
		hWndItem = hWndCombo ;
    }

	::WinHelp(hWndItem, AfxGetApp()->m_pszHelpFilePath,
		HELP_WM_HELP, (DWORD)(LPVOID)GetHelpIDs());

	return 0;
}

