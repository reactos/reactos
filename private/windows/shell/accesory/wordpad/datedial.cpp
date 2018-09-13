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
PARAFORMAT CDateDialog::m_pf;

/////////////////////////////////////////////////////////////////////////////
// CDateDialog dialog

const DWORD CDateDialog::m_nHelpIDs[] =
{
	IDC_DATEDIALOG_LIST, IDH_WORDPAD_TIMEDATE,
	IDC_STATIC_HEADING, IDH_WORDPAD_TIMEDATE,
	0, 0
};

CDateDialog::CDateDialog(CWnd* pParent , PARAFORMAT& pf)
	: CCSDialog(CDateDialog::IDD, pParent)
{
	m_pf = pf;
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

	// if we have Arabic/Hebrew locale
	if ((PRIMARYLANGID(LANGIDFROMLCID(m_id))== LANG_ARABIC) || 
		(PRIMARYLANGID(LANGIDFROMLCID(m_id))== LANG_HEBREW))
	{
		if(
		  (m_pf.wEffects & PFE_RTLPARA) &&
		  !(GetWindowLongPtr(m_pListBox->m_hWnd,GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
		  )
		{
			::SetWindowLongPtr (m_pListBox->m_hWnd , GWL_EXSTYLE , 
			   ::GetWindowLongPtr (m_pListBox->m_hWnd , GWL_EXSTYLE)|
			     WS_EX_RTLREADING | WS_EX_RIGHT | WS_EX_LEFTSCROLLBAR);
		}	
	}	
	
	EnumDateFormats(DateFmtEnumProc, m_id, DATE_SHORTDATE);
	EnumDateFormats(DateFmtEnumProc, m_id, DATE_LONGDATE);
	EnumTimeFormats(TimeFmtEnumProc, m_id, 0);

	m_pListBox = NULL;
	m_listBox.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


// The following masks are defined in WinNls.h under #ifdef WINVER>=0x0500. so 
// even we included the header file, still we don't see it. I defined it as
// follows.

#ifndef DATE_LTRREADING 
#define DATE_LTRREADING 0x00000010
#endif //!DATE_LTRREADING 

#ifndef DATE_RTLREADING 
#define DATE_RTLREADING 0x00000020
#endif //!DATE_RTLREADING 

BOOL CALLBACK CDateDialog::DateFmtEnumProc(LPTSTR lpszFormatString)
{
	ASSERT(m_pListBox != NULL);

	TCHAR buffer[256];
    TCHAR *buf = buffer;
	DWORD dwFlags = 0;

	// if we have Arabic/Hebrew locale
	if ((PRIMARYLANGID(LANGIDFROMLCID(m_id))== LANG_ARABIC) || 
		(PRIMARYLANGID(LANGIDFROMLCID(m_id))== LANG_HEBREW))
	{
		if (m_pf.wEffects & PFE_RTLPARA)
			dwFlags |= DATE_RTLREADING;
		else
			dwFlags |= DATE_LTRREADING;
	}	

	VERIFY(GetDateFormat(m_id, dwFlags, &m_time, lpszFormatString, buf, 256));

    // Strip leading blanks

    while (_istspace(*buf))
        ++buf;

#ifndef _CHICAGO_
	if ((PRIMARYLANGID(LANGIDFROMLCID(m_id))== LANG_ARABIC) || 
		(PRIMARYLANGID(LANGIDFROMLCID(m_id))== LANG_HEBREW))
	{
		if (m_pf.wEffects & PFE_RTLPARA)
        	lstrcat(buf, TEXT("\x200F")); // Unicode RLM
		else
        	lstrcat(buf, TEXT("\x200E")); // Unicode LRM
	}	
#endif // !_CHICAGO_
	
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
