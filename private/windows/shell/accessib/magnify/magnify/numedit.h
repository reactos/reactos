#if !defined(AFX_NUMEDIT_H__CD9E16EA_1CDC_11D1_B6B9_0060083316C7__INCLUDED_)
#define AFX_NUMEDIT_H__CD9E16EA_1CDC_11D1_B6B9_0060083316C7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NumEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNumberEdit window

class CNumberEdit : public CEdit
{
// Construction
public:
	CNumberEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNumberEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNumberEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CNumberEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NUMEDIT_H__CD9E16EA_1CDC_11D1_B6B9_0060083316C7__INCLUDED_)
