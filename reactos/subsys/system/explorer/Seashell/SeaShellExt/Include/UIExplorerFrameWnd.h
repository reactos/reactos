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

#if !defined(AFX_EXPLORERFRAMEWND_H__13CAB7C3_D316_11D1_8693_000000000000__INCLUDED_)
#define AFX_EXPLORERFRAMEWND_H__13CAB7C3_D316_11D1_8693_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EXPLORERFRAMEWND.h : header file
//
#include "UIFrameWnd.h"
#include "UITabSplitterWnd.h"

class CTRL_EXT_CLASS CUIExplorerFrameWnd : public CUIFrameWnd
{
	DECLARE_DYNAMIC(CUIExplorerFrameWnd)
protected:
	CUIExplorerFrameWnd();           // protected constructor used by dynamic creation

// Attributes
public:
	void SetExplorerView(CRuntimeClass *pClass);
	CTabSplitterWnd &GetSplitterWnd();
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIExplorerFrameWnd)
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL
		
// Implementation
protected:
	virtual ~CUIExplorerFrameWnd();

	// Generated message map functions
	//{{AFX_MSG(CUIExplorerFrameWnd)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
private:
	CTabSplitterWnd m_wndSplitter; 
	CRuntimeClass *m_pExplorerView;
};

inline void CUIExplorerFrameWnd::SetExplorerView(CRuntimeClass *pClass)
{
	m_pExplorerView = pClass;
}

inline CTabSplitterWnd &CUIExplorerFrameWnd::GetSplitterWnd()
{
	return m_wndSplitter;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPLORERFRAMEWND_H__13CAB7C3_D316_11D1_8693_000000000000__INCLUDED_)
