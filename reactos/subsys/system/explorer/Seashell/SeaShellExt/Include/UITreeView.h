//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#if !defined(AFX_UITREEVIEW__6016F537_2DF1_11D2_A412_E0317E000000__INCLUDED_)
#define AFX_UITREEVIEW__6016F537_2DF1_11D2_A412_E0317E000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CUITreeView view
#include "UITreeCtrl.h"

class CTRL_EXT_CLASS CUITreeView : public CView
{
protected:
	CUITreeView(UINT nID);           // protected constructor used by dynamic creation
	DECLARE_DYNAMIC(CUITreeView)

// Attributes
public:
	virtual CUITreeCtrl &GetTreeCtrl();
	virtual void CreateTreeCtrl();
protected:
	virtual void SelectionChanged(const CRefresh &Refresh);
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUITreeView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CUITreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CUITreeView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LRESULT OnAppUpdateAllViews( WPARAM wParam, LPARAM lParam );
	DECLARE_MESSAGE_MAP()
protected:
	CUITreeCtrl *m_pTreeCtrl;
private:
	UINT m_Style;
	UINT m_nID;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UITREEVIEW__6016F537_2DF1_11D2_A412_E0317E000000__INCLUDED_)
