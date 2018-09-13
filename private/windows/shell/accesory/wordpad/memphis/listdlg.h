// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

class CListDlg : public CDialog
{
public:
	//{{AFX_DATA(CListDlg)
	enum { IDD = IDD_LISTDIALOG };
	//}}AFX_DATA
	CListDlg::CListDlg(UINT idStrDlgTitle, UINT idStrListTitle, 
		const CStringList& listItems, int nDefSel=0);
	CString m_strDlgTitle,m_strListTitle;
	const CStringList& m_listItems;
	int m_nSelection;

protected:
	BOOL OnInitDialog();
	//{{AFX_MSG(CListDlg)
	afx_msg void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
