#if !defined(AFX_MFCEXPLORERDLG_H__CDBB7A42_DEE1_4AFC_A7A3_13FE102A30EE__INCLUDED_)
#define AFX_MFCEXPLORERDLG_H__CDBB7A42_DEE1_4AFC_A7A3_13FE102A30EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MFCExplorerDlg.h : header file
//
#include "IEShellTreeCtrl.h"
#include "IEShellListCtrl.h"
#include "IEShellComboBox.h"
/////////////////////////////////////////////////////////////////////////////
// CMFCExplorerDlg dialog

class CMFCExplorerDlg : public CDialog
{
// Construction
public:
	CMFCExplorerDlg(LPCTSTR pszPath=NULL,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMFCExplorerDlg)
	enum { IDD = IDD_MFC_EXPLORER };
	CIEShellTreeCtrl	m_tcShell;
	CIEShellListCtrl	m_lcShell;
	CIEShellComboBox	m_cbShell;
	//}}AFX_DATA

	void SetPath(LPCTSTR pszPath) { m_sPath = pszPath; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMFCExplorerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMFCExplorerDlg)
	afx_msg void OnMfcButtDetails();
	afx_msg void OnMfcButtLargeIcons();
	afx_msg void OnMfcButtReport();
	afx_msg void OnMfcButtSmallIcons();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString m_sPath;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCEXPLORERDLG_H__CDBB7A42_DEE1_4AFC_A7A3_13FE102A30EE__INCLUDED_)
