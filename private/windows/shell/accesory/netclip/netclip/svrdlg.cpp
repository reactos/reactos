// ServerInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "netclip.h"
#include "SvrDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerInfoDlg dialog


CServerInfoDlg::CServerInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServerInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServerInfoDlg)
	m_strMachine = _T("");
	//}}AFX_DATA_INIT
}


void CServerInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerInfoDlg)
	DDX_Text(pDX, IDC_MACHINE, m_strMachine);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CServerInfoDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerInfoDlg message handlers
