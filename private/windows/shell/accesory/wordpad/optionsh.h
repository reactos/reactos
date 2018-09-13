// optionsh.h : header file
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
// COptionSheet

class COptionSheet : public CCSPropertySheet
{
// Construction
public:
	COptionSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	CUnitsPage units;
	CDocOptPage pageText;
	CDocOptPage pageRTF;
	CDocOptPage pageWord;
	CDocOptPage pageWrite;
	CEmbeddedOptPage pageEmbedded;

// Operations
public:
	INT_PTR DoModal();
	void SetPageButtons(CDocOptPage& page, CDocOptions& options, BOOL bPrimary = TRUE);
	void SetState(CDocOptPage& page, CDocOptions& optiosn, BOOL bPrimary = TRUE);

// Overrides
    virtual LONG OnHelp(WPARAM, LPARAM lParam);
    virtual LONG OnHelpContextMenu(WPARAM, LPARAM lParam);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionSheet)
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
protected:
	//{{AFX_MSG(COptionSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
