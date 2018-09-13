// buttondi.h : header file
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
// CButtonDialog dialog

class CButtonDialog : public CCSDialog
{
// Construction
public:
	CButtonDialog(LPCTSTR lpszText, LPCTSTR lpszCaption, LPCTSTR lpszButtons, 
		WORD wStyle, DWORD* pHelpIDs = NULL, CWnd* pParentWnd = NULL);
	~CButtonDialog();

// Attributes
	CFont m_font;
// Operations
	static int DisplayMessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption, 
		LPCTSTR lpszButtons, WORD wStyle, int nDef = 0, int nCancel = -1, 
		DWORD* pHelpIDs = NULL, CWnd* pParentWnd = NULL);

	void AddButton(CString& strButton) { m_strArray.Add(strButton);}
	void AddButtons(LPCTSTR lpszButton);
	void SetCancel(int nCancel)
		{ ASSERT(nCancel < m_strArray.GetSize()); m_nCancel = nCancel;}
	void SetDefault(int nDef)
		{ ASSERT(nDef < m_strArray.GetSize()); m_nDefButton = nDef;}
	void FillInHeader(LPDLGTEMPLATE lpDlgTmp);

// Overridables
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual int DoModal();

// Implementation
protected:
	virtual const DWORD* GetHelpIDs() {return m_pHelpIDs;}
	DWORD* m_pHelpIDs;
	int m_nDefButton;
	int m_nCancel;
	HGLOBAL m_hDlgTmp;
	UINT m_nBaseID;
	WORD m_wStyle;
	CButton* m_pButtons;
	CStatic m_staticIcon;
	CStatic m_staticText;
	CString m_strCaption;
	CString m_strText;
	CStringArray m_strArray;
	CSize GetBaseUnits();
	LPCTSTR GetIconID(WORD wFlags);
	void PositionControls();

	// Generated message map functions
	//{{AFX_MSG(CButtonDialog)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
