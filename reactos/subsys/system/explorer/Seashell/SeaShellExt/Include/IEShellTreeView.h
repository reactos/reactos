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

#ifndef __IESHELLTREEVIEW_H__
#define __IESHELLTREEVIEW_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// IEShellTreeView.h : header file
//
#include "IEShellTreeCtrl.h"
#include "UITreeView.h"
/////////////////////////////////////////////////////////////////////////////
// CIEShellTreeView window

class CTRL_EXT_CLASS CIEShellTreeView : public CUITreeView
{
// Construction
protected:
	CIEShellTreeView();
	DECLARE_DYNCREATE(CIEShellTreeView)

// Attributes
public:
	CIEShellTreeCtrl &GetShellTreeCtrl();
	void SetPath(LPCTSTR pszPath);
	LPTVITEMDATA GetSelectedItemData();
// Operations
public:
// Overrides
	virtual bool PopulateTree(LPCTSTR pszPath);
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIEShellTreeView)
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void CreateTreeCtrl();
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL
// Implementation
public:
	virtual ~CIEShellTreeView();

// Generated message map functions
protected:
	//{{AFX_MSG(CIEShellTreeView)
			// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg LRESULT OnSetmessagestring(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	bool m_bPopulated;
	CString m_sPath;
};
/////////////////////////////////////////////////////////////////////////////
inline void CIEShellTreeView::SetPath(LPCTSTR pszPath)
{
	m_sPath = pszPath;
}

inline CIEShellTreeCtrl &CIEShellTreeView::GetShellTreeCtrl()
{
	return static_cast<CIEShellTreeCtrl&>(GetTreeCtrl());
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //__IESHELLTREEVIEW_H__
