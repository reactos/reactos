// listdlg.cpp : Defines the class behaviors for the application.
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
#include "listdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CListDlg, CDialog)
	//{{AFX_MSG_MAP(CListDlg)
	ON_LBN_DBLCLK(IDC_LISTDIALOG_LIST, OnOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CListDlg::CListDlg(UINT idStrDlgTitle, UINT idStrListTitle, 
	const CStringList& listItems, int nDefSel) : CDialog(CListDlg::IDD),
	m_listItems(listItems)
{
	VERIFY(m_strDlgTitle.LoadString(idStrDlgTitle));
	VERIFY(m_strListTitle.LoadString(idStrListTitle));
	m_nSelection = nDefSel;
}

BOOL CListDlg::OnInitDialog()
{
	SetWindowText(m_strDlgTitle);
// fix this
	SetDlgItemText(IDC_STATIC_HEADING, m_strListTitle);
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LISTDIALOG_LIST);
	ASSERT(pListBox != NULL);
	// fill with document templates in list

	POSITION pos = m_listItems.GetHeadPosition();
	while (pos != NULL)
	{
		 if ( pListBox->AddString(m_listItems.GetNext(pos)) == -1)
		 	return FALSE;
	}
	pListBox->SetCurSel(m_nSelection);

	return CDialog::OnInitDialog();
}

void CListDlg::OnOK()
{
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LISTDIALOG_LIST);
	ASSERT(pListBox != NULL);
	m_nSelection = pListBox->GetCurSel();
	CDialog::OnOK();
}
