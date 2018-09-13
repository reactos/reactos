// datedial.h : header file
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

/////////////////////////////////////////////////////////////////////////////
// CDateDialog dialog

class CDateDialog : public CCSDialog
{
// Construction
public:
	CDateDialog(CWnd* pParent = NULL);   // standard constructor

// Attributes
	static SYSTEMTIME m_time;
	static LCID m_id;
	static CListBox* m_pListBox;
	static BOOL CALLBACK DateFmtEnumProc(LPTSTR lpszFormatString);
	static BOOL CALLBACK TimeFmtEnumProc(LPTSTR lpszFormatString);

// Dialog Data
	//{{AFX_DATA(CDateDialog)
	enum { IDD = IDD_DATEDIALOG };
	CListBox	m_listBox;
	CString	m_strSel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDateDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static const DWORD m_nHelpIDs[];
	virtual const DWORD* GetHelpIDs() {return m_nHelpIDs;}

	// Generated message map functions
	//{{AFX_MSG(CDateDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkDatedialogList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
