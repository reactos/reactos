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

#if !defined(AFX_SPAWNFRAMEWND_H__13CAB7C3_D316_11D1_8693_000000000000__INCLUDED_)
#define AFX_SPAWNFRAMEWND_H__13CAB7C3_D316_11D1_8693_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SpawnFrameWnd.h : header file
//
#include "UIStatusBar.h"
#include "UIFlatBar.h"
#include "UICoolMenu.h"
#include "UICoolBar.h"
#include "UIMenuBar.h"
#include "IEShellComboBox.h"

/////////////////////////////////////////////////////////////////////////////
// CUIFrameWnd frame
class CTRL_EXT_CLASS CToolBarComboBox : public CIEShellComboBox 
{
public:
protected:
	DECLARE_MESSAGE_MAP()
private:
};

class CTRL_EXT_CLASS CReallyCoolBar : public CCoolBar 
{
protected:
	DECLARE_DYNAMIC(CReallyCoolBar)
	CFlatToolBar	m_wndToolBar;			 // toolbar
	CMenuBar		m_wndMenuBar;			 // menu bar
	virtual BOOL   OnCreateBands();		 // fn to create the bands
	void SetToolBarID(UINT IDToolbar);
protected:
	UINT m_IDToolbar;
};

class CTRL_EXT_CLASS CWebBrowserCoolBar : public CReallyCoolBar 
{
protected:
	DECLARE_DYNAMIC(CWebBrowserCoolBar)
	CFlatToolBar	m_wndBrowserBar;			 // toolbar
	virtual BOOL   OnCreateBands();		 // fn to create the bands
	CToolBarComboBox &GetComboBox() { return m_wndCombo; }
protected:
	CToolBarComboBox m_wndCombo;
	CString m_sCBValue;
};

class CTRL_EXT_CLASS CUIFrameWnd : public CFrameWnd
{
	DECLARE_DYNAMIC(CUIFrameWnd)
protected:
	CUIFrameWnd();           // protected constructor used by dynamic creation

// Attributes
public:
// COOLMENU SUPPORT
// END COOLMENU SUPPORT
	CDialogBar m_wndFieldChooser;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIFrameWnd)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void GetMessageString( UINT nID, CString& rMessage ) const;	
	//}}AFX_VIRTUAL
	virtual CUIStatusBar &GetStatusBar();
	virtual CReallyCoolBar &GetCoolBar();
	virtual CCoolMenuManager &GetMenuManager();
	virtual void RestoreWindow(UINT nCmdShow);
		
// Implementation
protected:
	virtual ~CUIFrameWnd();
	virtual void CreateCoolBar();

	void SetReportCtrl(bool bSet);
	// Generated message map functions
	//{{AFX_MSG(CUIFrameWnd)
	afx_msg void OnClose();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	afx_msg LRESULT OnRegisteredMouseWheel(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCBIESelChange(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCBIEPopulate(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCBIEHitEnter(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCBIESetEditText(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()
protected:
	UINT				m_IDToolbar;
	CReallyCoolBar		*m_pwndCoolBar;
private:
	CUIStatusBar		m_wndStatusBar;
	CCoolMenuManager	m_menuManager;	 // cool (bitmap button) menus
	bool				m_bReportCtrl;
	static LPCTSTR szMainFrame;
};

inline void CUIFrameWnd::SetReportCtrl(bool bSet)
{
	m_bReportCtrl = bSet;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPAWNFRAMEWND_H__13CAB7C3_D316_11D1_8693_000000000000__INCLUDED_)
