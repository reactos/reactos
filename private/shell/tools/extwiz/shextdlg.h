#if !defined(AFX_SHELLEXTENSIONS_H__AA4DBA54_5D3E_11D1_8CCE_00C04FD918D0__INCLUDED_)
#define AFX_SHELLEXTENSIONS_H__AA4DBA54_5D3E_11D1_8CCE_00C04FD918D0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ShellExtensions.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ShellExtensions dialog

class ShellExtensions : public CAppWizStepDlg
{
// Construction
public:
	ShellExtensions();   // standard constructor

// Dialog Data
	//{{AFX_DATA(ShellExtensions)
	enum { IDD = IDD_SHELLOPTIONS };
	CButton	m_btnDragAndDrop;
	CButton	m_btnContextMenu3;
	CButton	m_btnContextMenu2;
	BOOL	m_bContextMenu;
	BOOL	m_bContextMenu2;
	BOOL	m_bContextMenu3;
	BOOL	m_bCopyHook;
	BOOL	m_bDataObject;
	BOOL	m_bDragAndDrop;
	BOOL	m_bDropTarget;
	BOOL	m_bIcon;
	BOOL	m_bInfoTip;
	BOOL	m_bPropertySheet;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ShellExtensions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    BOOL OnDismiss();

	// Generated message map functions
	//{{AFX_MSG(ShellExtensions)
	afx_msg void OnContextmenu();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHELLEXTENSIONS_H__AA4DBA54_5D3E_11D1_8CCE_00C04FD918D0__INCLUDED_)
