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

#ifndef __IESHELLLISTVIEW_H__
#define __IESHELLLISTVIEW_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// IEShellListView.h : header file
//
#include "IEShellListCtrl.h"
#include "UIListView.h"
#include "HtmlCtrl.h"
/////////////////////////////////////////////////////////////////////////////
// CIEShellListView window

class CTRL_EXT_CLASS CIEShellListView : public CUIListView
{
// Construction
protected:
	CIEShellListView();
	DECLARE_DYNCREATE(CIEShellListView)

// Attributes
public:
	CIEShellListCtrl &GetShellListCtrl();

// Operations
public:
// Overrides

protected:
	virtual void LoadShellFolderItems(const CRefreshShellFolder &rFolder);
	virtual void SetActiveWindow(CWnd *pWnd);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIEShellListView)
	public:
	virtual void CreateListCtrl();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL
// Implementation
public:
	virtual ~CIEShellListView();

// Generated message map functions
protected:
	//{{AFX_MSG(CIEShellListView)
			// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg LRESULT OnAppUpdateAllViews(WPARAM wParam,LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnSetmessagestring(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	afx_msg LRESULT OnCBIESelChange(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCBIEHitEnter(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	CWnd *m_pActiveWnd;
	CHtmlCtrl m_htmlCtrl;
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //__IESHELLLISTVIEW_H__
