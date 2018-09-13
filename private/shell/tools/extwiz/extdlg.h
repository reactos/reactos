#if !defined(AFX_EXTENSIONCHOICE_H__AA4DBA53_5D3E_11D1_8CCE_00C04FD918D0__INCLUDED_)
#define AFX_EXTENSIONCHOICE_H__AA4DBA53_5D3E_11D1_8CCE_00C04FD918D0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ExtensionChoice.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ExtensionChoice dialog

class ExtensionChoice : public CAppWizStepDlg
{
// Construction
public:
	ExtensionChoice();   // standard constructor

// Dialog Data
	//{{AFX_DATA(ExtensionChoice)
	enum { IDD = IDD_EXTENSION };
	CEdit	m_edtExt;
	CString	m_strClassDescription;
	CString	m_strClassType;
	CString	m_strFileExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ExtensionChoice)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    BOOL OnDismiss();

	// Generated message map functions
	//{{AFX_MSG(ExtensionChoice)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXTENSIONCHOICE_H__AA4DBA53_5D3E_11D1_8CCE_00C04FD918D0__INCLUDED_)
