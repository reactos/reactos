// formatta.h : header file
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
// CFormatTabDlg dialog

class CFormatTabDlg : public CCSDialog
{
// Construction
public:
	CFormatTabDlg(PARAFORMAT& pf, CWnd* pParent = NULL);    // standard constructor
	~CFormatTabDlg();
	PARAFORMAT m_pf;
	LONG* m_tabarray;
	int m_nCount;

// Dialog Data
	//{{AFX_DATA(CFormatTabDlg)
	enum { IDD = IDD_FORMAT_TAB };
	CButton	m_buttonClearAll;
	CButton	m_buttonSet;
	CButton	m_buttonClear;
	CComboBox	m_comboBox;
	//}}AFX_DATA

// Implementation
protected:
	static const DWORD m_nHelpIDs[];
	virtual const DWORD* GetHelpIDs() {return m_nHelpIDs;}
	void UpdateButton(CButton& button, BOOL b);
	void UpdateButtons();
	BOOL Set();
	BOOL AddTabToArray(LONG lTab);
	BOOL RemoveTabFromArray(LONG lTab);
	void RemoveTabFromArrayByIndex(int nIndex);
	void UpdateListBox();
	void SetEditFocus();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	// Generated message map functions
	//{{AFX_MSG(CFormatTabDlg)
	afx_msg void OnClickedClear();
	afx_msg void OnClickedClearAll();
	afx_msg void OnClickedSet();
	afx_msg void OnEditChange();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchange();
	afx_msg LONG OnHelp(UINT wParam, LONG lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
