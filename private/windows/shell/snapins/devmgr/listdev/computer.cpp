// computer.cpp : implementation file
//

#include "stdafx.h"
#include "ListDev.h"
#include "computer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// computer dialog


computer::computer(CWnd* pParent /*=NULL*/)
	: CDialog(computer::IDD, pParent)
{
	//{{AFX_DATA_INIT(computer)
	m_strComputerName = _T("");
	//}}AFX_DATA_INIT
}


void computer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(computer)
	DDX_Text(pDX, IDC_EDIT_COMPUTERNAME, m_strComputerName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(computer, CDialog)
	//{{AFX_MSG_MAP(computer)
	ON_BN_CLICKED(IDC_CANCEL, OnCancel)
	ON_BN_CLICKED(ID_OK, OnOk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// computer message handlers

void computer::OnCancel() 
{
	// TODO: Add your control notification handler code here
	
}

void computer::OnOk() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	EndDialog(1);
}
