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

// IEShellTreeView.cpp : implementation file
#include "stdafx.h"
#include "UIRes.h"
#include "UIMessages.h"
#include "IEShellTreeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIEShellTreeView

IMPLEMENT_DYNCREATE(CIEShellTreeView,CUITreeView)

CIEShellTreeView::CIEShellTreeView()
 : CUITreeView(IDC_TREE_SHELL)
{
	m_bPopulated = false;
}


CIEShellTreeView::~CIEShellTreeView()
{
}

LPTVITEMDATA CIEShellTreeView::GetSelectedItemData()
{
	HTREEITEM hItem = GetShellTreeCtrl().GetSelectedItem();
	LPTVITEMDATA ptvid=NULL;
	if (hItem)
		ptvid = (LPTVITEMDATA)GetShellTreeCtrl().GetItemData(hItem);
	return ptvid;
}

void CIEShellTreeView::OnInitialUpdate() 
{
	CUITreeView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	if (!m_bPopulated)
	{
		GetShellTreeCtrl().SetComboBoxWnd(GetParentFrame()->GetSafeHwnd());
		PopulateTree(NULL);
	}
}

bool CIEShellTreeView::PopulateTree(LPCTSTR pszPath)
{
	CIEShellTreeCtrl *pCtrl = (CIEShellTreeCtrl*)m_pTreeCtrl;
	m_bPopulated = pCtrl->LoadFolderItems(pszPath);	
	return m_bPopulated;
}

void CIEShellTreeView::CreateTreeCtrl()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pTreeCtrl = new CIEShellTreeCtrl;	
}

BEGIN_MESSAGE_MAP(CIEShellTreeView, CUITreeView)
	//{{AFX_MSG_MAP(CIEShellTreeView)
	ON_MESSAGE(WM_SETMESSAGESTRING,OnSetmessagestring)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIEShellTreeView message handlers

void CIEShellTreeView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if (bActivate && pDeactiveView != NULL)
	{
//		theApp.SwitchTab(CViewTabView::TAB_LIST_FOLDER);
		GetTreeCtrl().SetFocus();
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


LRESULT CIEShellTreeView::OnSetmessagestring(WPARAM wParam, LPARAM lParam)
{
	CFrameWnd *pFrame = GetParentFrame();
	if (pFrame)
		return pFrame->SendMessage(WM_SETMESSAGESTRING,wParam,lParam);
	return 0;
}
