#if !defined(AFX_SHELLTREEDLG_H__383BF0CC_353A_49C7_AF0D_DB8FC40F81A9__INCLUDED_)
#define AFX_SHELLTREEDLG_H__383BF0CC_353A_49C7_AF0D_DB8FC40F81A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ShellTreeDlg.h : header file
//
#include "IEShellTreeCtrl.h"
/////////////////////////////////////////////////////////////////////////////
// CShellTreeDlg dialog

class CShellTreeDlg : public CDialog
{
// Construction
public:
	CShellTreeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CShellTreeDlg)
	enum { IDD = IDD_SHELL_TREE };
	CIEShellTreeCtrl	m_ShellTree;
	CString	m_stHelp;
	CString	m_stPath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShellTreeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CShellTreeDlg)
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnAppUpdateAllViews(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnSetmessagestring(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHELLTREEDLG_H__383BF0CC_353A_49C7_AF0D_DB8FC40F81A9__INCLUDED_)
