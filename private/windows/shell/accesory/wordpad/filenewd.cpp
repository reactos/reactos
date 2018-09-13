// filenewd.cpp : implementation file
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
#include "filenewd.h"
#include "filedlg.h"
#include "helpids.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

DWORD const CFileNewDialog::m_nHelpIDs[] = 
{
	IDC_DATEDIALOG_LIST, IDH_WORDPAD_FILENEW_DOC,
    IDC_STATIC_HEADING, IDH_WORDPAD_FILENEW_DOC,
	0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CFileNewDialog dialog

CFileNewDialog::CFileNewDialog(CWnd* pParent /*=NULL*/)
	: CCSDialog(CFileNewDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFileNewDialog)
	m_nSel = -1;
	//}}AFX_DATA_INIT
}


void CFileNewDialog::DoDataExchange(CDataExchange* pDX)
{
	CCSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileNewDialog)
	DDX_Control(pDX, IDC_DATEDIALOG_LIST, m_listbox);
	DDX_LBIndex(pDX, IDC_DATEDIALOG_LIST, m_nSel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFileNewDialog, CCSDialog)
	//{{AFX_MSG_MAP(CFileNewDialog)
	ON_LBN_DBLCLK(IDC_DATEDIALOG_LIST, OnDblclkDatedialogList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFileNewDialog message handlers

BOOL CFileNewDialog::OnInitDialog() 
{
	CCSDialog::OnInitDialog();

    static const struct
    {
        int     rdType;
        int     idsType;
    }
    FileTypes[] = 
    {
        {RD_RICHTEXT,    IDS_RTF_DOCUMENT},
        {RD_WINWORD6,    IDS_WORD6_DOCUMENT},
        {RD_TEXT,        IDS_TEXT_DOCUMENT},
        {RD_UNICODETEXT, IDS_UNICODETEXT_DOCUMENT}
    };
 
	CString str;
    int     i;
    int     defType = CWordpadFileDialog::GetDefaultFileType();
    int     iSelected = 0;

    for (i = 0; i < sizeof(FileTypes)/sizeof(FileTypes[0]); i++)
    {
	    VERIFY(str.LoadString(FileTypes[i].idsType));
	    m_listbox.AddString(str);

        if (FileTypes[i].rdType == defType)
            iSelected = i; 
    }
    
	m_listbox.SetCurSel(iSelected);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFileNewDialog::OnDblclkDatedialogList() 
{
	OnOK();
}
