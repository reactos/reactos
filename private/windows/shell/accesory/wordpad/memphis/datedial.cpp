// datedial.cpp : implementation file
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
#include "datedial.h"
#include "helpids.h"
#include <winnls.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

SYSTEMTIME CDateDialog::m_time;
LCID CDateDialog::m_id;
CListBox* CDateDialog::m_pListBox = NULL;

/////////////////////////////////////////////////////////////////////////////
// CDateDialog dialog

const DWORD CDateDialog::m_nHelpIDs[] =
{
	IDC_DATEDIALOG_LIST, IDH_WORDPAD_TIMEDATE,
	IDC_STATIC_HEADING, IDH_WORDPAD_TIMEDATE,
	0, 0
};

CDateDialog::CDateDialog(CWnd* pParent /*=NULL*/)
	: CCSDialog(CDateDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDateDialog)
	m_strSel = _T("");
	//}}AFX_DATA_INIT
}


void CDateDialog::DoDataExchange(CDataExchange* pDX)
{
	CCSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDateDialog)
	DDX_Control(pDX, IDC_DATEDIALOG_LIST, m_listBox);
	DDX_LBString(pDX, IDC_DATEDIALOG_LIST, m_strSel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDateDialog, CCSDialog)
	//{{AFX_MSG_MAP(CDateDialog)
	ON_LBN_DBLCLK(IDC_DATEDIALOG_LIST, OnDblclkDatedialogList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDateDialog message handlers

BOOL CDateDialog::OnInitDialog()
{
	CCSDialog::OnInitDialog();

	m_pListBox = &m_listBox; // set static member
	GetLocalTime(&m_time);
	m_id = GetUserDefaultLCID();
	
	EnumDateFormats(DateFmtEnumProc, m_id, DATE_SHORTDATE);
	EnumDateFormats(DateFmtEnumProc, m_id, DATE_LONGDATE);
	EnumTimeFormats(TimeFmtEnumProc, m_id, 0);

	m_pListBox = NULL;
	m_listBox.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CALLBACK CDateDialog::DateFmtEnumProc(LPTSTR lpszFormatString)
{
	ASSERT(m_pListBox != NULL);

	TCHAR buffer[256];
    TCHAR *buf = buffer;

	VERIFY(GetDateFormat(m_id, 0, &m_time, lpszFormatString, buf, 256));

    // Strip leading blanks

    while (_istspace(*buf))
        ++buf;

	// we can end up with same format because a format with leading
	// zeroes may be the same as one without when a number is big enough
	// e.g. 09/10/94 9/10/94 are different but 10/10/94 and 10/10/94 are
	// the same
	if (m_pListBox->FindStringExact(-1,buf) == CB_ERR)
		m_pListBox->AddString(buf);
	return TRUE;
}

BOOL CALLBACK CDateDialog::TimeFmtEnumProc(LPTSTR lpszFormatString)
{
	ASSERT(m_pListBox != NULL);

	TCHAR buffer[256];
    TCHAR *buf = buffer;

	VERIFY(GetTimeFormat(m_id, 0, &m_time, lpszFormatString, buf, 256));

    // Strip leading blanks

    while (_istspace(*buf))
        ++buf;

	// we can end up with same format because a format with leading
	// zeroes may be the same as one without when a number is big enough
	// e.g. 09/10/94 9/10/94 are different but 10/10/94 and 10/10/94 are
	// the same
	if (m_pListBox->FindStringExact(-1,buf) == CB_ERR)
		m_pListBox->AddString(buf);
	return TRUE;
}

void CDateDialog::OnDblclkDatedialogList()
{
	OnOK();
}
