// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__476445A6_F5F7_4383_B534_EBAB2D5728B1__INCLUDED_)
#define AFX_MAINFRM_H__476445A6_F5F7_4383_B534_EBAB2D5728B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSeaShellView;

#include "UIExplorerFrameWnd.h"

class CMainFrame : public CUIExplorerFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
protected:
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void CreateCoolBar();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
	CSeaShellView* GetRightPane();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnSettingChange(WPARAM wParam,LPARAM lParam);
	afx_msg void OnViewExplorerdialog();
	afx_msg void OnViewFilefilter();
	afx_msg void OnViewTreedialog();
	//}}AFX_MSG
	afx_msg void OnUpdateViewStyles(CCmdUI* pCmdUI);
	afx_msg void OnViewStyle(UINT nCommandID);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__476445A6_F5F7_4383_B534_EBAB2D5728B1__INCLUDED_)
