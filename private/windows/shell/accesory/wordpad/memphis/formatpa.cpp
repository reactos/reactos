// formatpa.cpp : implementation file
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
#include "formatpa.h"
#include "ddxm.h"
#include "helpids.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const DWORD CFormatParaDlg::m_nHelpIDs[] = 
{
	IDC_EDIT_LEFT, IDH_WORDPAD_INDENT_LEFT,
	IDC_EDIT_RIGHT, IDH_WORDPAD_INDENT_RIGHT,
	IDC_EDIT_FIRST_LINE, IDH_WORDPAD_INDENT_FIRST,
	IDC_BOX, IDH_COMM_GROUPBOX,
	IDC_COMBO_ALIGNMENT, IDH_WORDPAD_ALIGN,
	IDC_TEXT_ALIGNMENT, IDH_WORDPAD_ALIGN,
	0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CFormatParaDlg dialog

CFormatParaDlg::CFormatParaDlg(PARAFORMAT& pf, CWnd* pParent /*=NULL*/)
	: CCSDialog(CFormatParaDlg::IDD, pParent)
{
	m_pf = pf;
	if (m_pf.dwMask & PFM_ALIGNMENT)
	{
		if (m_pf.wAlignment & PFA_LEFT && m_pf.wAlignment & PFA_RIGHT)
			m_nAlignment = 2;
		else
			m_nAlignment = (m_pf.wAlignment & PFA_LEFT) ? 0 : 1;
	}
	else
		m_nAlignment = -1;
	//{{AFX_DATA_INIT(CFormatParaDlg)
	m_nFirst = 0;
	m_nLeft = 0;
	m_nRight = 0;
	//}}AFX_DATA_INIT
}

void CFormatParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CCSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFormatParaDlg)
	DDX_CBIndex(pDX, IDC_COMBO_ALIGNMENT, m_nAlignment);
	DDX_Twips(pDX, IDC_EDIT_FIRST_LINE, m_nFirst);
	DDV_MinMaxTwips(pDX, m_nFirst, -31680, 31680);
	DDX_Twips(pDX, IDC_EDIT_LEFT, m_nLeft);
	DDV_MinMaxTwips(pDX, m_nLeft, -31680, 31680);
	DDX_Twips(pDX, IDC_EDIT_RIGHT, m_nRight);
	DDV_MinMaxTwips(pDX, m_nRight, -31680, 31680);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFormatParaDlg, CCSDialog)
	//{{AFX_MSG_MAP(CFormatParaDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFormatParaDlg message handlers

void CFormatParaDlg::OnOK()
{
	CCSDialog::OnOK();
	m_pf.dwMask = 0;
	if (m_nAlignment >= 0)
	{
		ASSERT(m_nAlignment < 3);
		m_pf.dwMask |= PFM_ALIGNMENT;
		m_pf.wAlignment = (WORD)((m_nAlignment == 0) ? PFA_LEFT : 
			(m_nAlignment == 1) ? PFA_RIGHT : PFA_CENTER);
	}
	if (m_nRight != DDXM_BLANK)
		m_pf.dwMask |= PFM_RIGHTINDENT;
	if (m_nLeft != DDXM_BLANK && m_nFirst != DDXM_BLANK)
		m_pf.dwMask |= PFM_STARTINDENT;
	if (m_nFirst != DDXM_BLANK)
		m_pf.dwMask |= PFM_OFFSET;

	m_pf.dxRightIndent = m_nRight;
	m_pf.dxOffset = -m_nFirst;
	m_pf.dxStartIndent = m_nLeft + m_nFirst;
}

BOOL CFormatParaDlg::OnInitDialog() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_COMBO_ALIGNMENT);
	CString str;
	str.LoadString(IDS_LEFT);
	pBox->AddString(str);
	str.LoadString(IDS_RIGHT);
	pBox->AddString(str);
	str.LoadString(IDS_CENTER);
	pBox->AddString(str);

	if (m_nWordWrap == 0)
	{
		GetDlgItem(IDC_COMBO_ALIGNMENT)->EnableWindow(FALSE);
		GetDlgItem(IDC_TEXT_ALIGNMENT)->EnableWindow(FALSE);
	}

	m_nRight = (m_pf.dwMask & PFM_RIGHTINDENT) ? m_pf.dxRightIndent : DDXM_BLANK;
	if (m_pf.dwMask & PFM_OFFSET)
	{
		m_nFirst = -m_pf.dxOffset;
		m_nLeft = (m_pf.dwMask & PFM_STARTINDENT) ? 
			m_pf.dxStartIndent + m_pf.dxOffset : DDXM_BLANK;
	}
	else
		m_nLeft = m_nFirst = DDXM_BLANK;
	
	CCSDialog::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
