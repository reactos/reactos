// ShellTreeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SeaShell.h"
#include "ShellTreeDlg.h"
#include "UIMessages.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShellTreeDlg dialog


CShellTreeDlg::CShellTreeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CShellTreeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CShellTreeDlg)
	m_stHelp = _T("");
	m_stPath = _T("");
	//}}AFX_DATA_INIT
}


void CShellTreeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShellTreeDlg)
	DDX_Control(pDX, IDC_SHELL_TREE, m_ShellTree);
	DDX_Text(pDX, IDC_ST_HELP, m_stHelp);
	DDX_Text(pDX, IDC_ST_PATH, m_stPath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CShellTreeDlg, CDialog)
	//{{AFX_MSG_MAP(CShellTreeDlg)
	ON_MESSAGE(WM_SETMESSAGESTRING,OnSetmessagestring)
	ON_MESSAGE(WM_APP_UPDATE_ALL_VIEWS,OnAppUpdateAllViews)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShellTreeDlg message handlers

BOOL CShellTreeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_ShellTree.LoadFolderItems();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CShellTreeDlg::OnAppUpdateAllViews(WPARAM wParam, LPARAM lParam)
{
	if (wParam == HINT_TREE_SEL_CHANGED)
	{
		const CRefreshShellFolder *pRefresh = reinterpret_cast<const CRefreshShellFolder*>(lParam);
		m_stPath = m_ShellTree.GetPathName(pRefresh->GetItem());		
		UpdateData(FALSE);
	}
	return 1;
}

LRESULT CShellTreeDlg::OnSetmessagestring(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		m_stHelp = (LPCTSTR)lParam;		
		UpdateData(FALSE);
	}
	return 1;
}
