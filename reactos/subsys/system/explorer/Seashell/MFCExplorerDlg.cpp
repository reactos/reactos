// MFCExplorerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SeaShell.h"
#include "MFCExplorerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMFCExplorerDlg dialog


CMFCExplorerDlg::CMFCExplorerDlg(LPCTSTR pszPath,CWnd* pParent /*=NULL*/)
	: CDialog(CMFCExplorerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMFCExplorerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	if (pszPath)
		m_sPath = pszPath;
}


void CMFCExplorerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMFCExplorerDlg)
	DDX_Control(pDX, IDC_TREE_SHELL, m_tcShell);
	DDX_Control(pDX, IDC_LIST_SHELL, m_lcShell);
	DDX_Control(pDX, IDC_COMBO_SHELL, m_cbShell);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMFCExplorerDlg, CDialog)
	//{{AFX_MSG_MAP(CMFCExplorerDlg)
	ON_BN_CLICKED(IDC_MFC_BUTT_DETAILS, OnMfcButtDetails)
	ON_BN_CLICKED(IDC_MFC_BUTT_LARGE_ICONS, OnMfcButtLargeIcons)
	ON_BN_CLICKED(IDC_MFC_BUTT_REPORT, OnMfcButtReport)
	ON_BN_CLICKED(IDC_MFC_BUTT_SMALL_ICONS, OnMfcButtSmallIcons)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFCExplorerDlg message handlers

void CMFCExplorerDlg::OnMfcButtDetails() 
{
	// TODO: Add your control notification handler code here
	m_lcShell.SetViewType(LVS_LIST);	
}

void CMFCExplorerDlg::OnMfcButtLargeIcons() 
{
	// TODO: Add your control notification handler code here
	m_lcShell.SetViewType(LVS_ICON);		
}

void CMFCExplorerDlg::OnMfcButtReport() 
{
	// TODO: Add your control notification handler code here
	m_lcShell.SetViewType(LVS_REPORT);	
}

void CMFCExplorerDlg::OnMfcButtSmallIcons() 
{
	// TODO: Add your control notification handler code here
	m_lcShell.SetViewType(LVS_SMALLICON);
}

BOOL CMFCExplorerDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_cbShell.SetTreeCtrlWnd(m_tcShell.GetSafeHwnd());
	m_tcShell.SetListCtrlWnd(m_lcShell.GetSafeHwnd());
	m_tcShell.SetComboBoxWnd(m_cbShell.GetSafeHwnd());
	m_tcShell.LoadFolderItems(m_sPath);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
