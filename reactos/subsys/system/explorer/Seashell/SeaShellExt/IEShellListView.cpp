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

// IEShellListView.cpp : implementation file
#include "stdafx.h"
#include "UIRes.h"
#include "IEShellListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIEShellListView

IMPLEMENT_DYNCREATE(CIEShellListView,CUIListView)

CIEShellListView::CIEShellListView()
 : CUIListView(IDC_LIST_SHELL)
{
	m_pActiveWnd = NULL;
}


CIEShellListView::~CIEShellListView()
{
}

void CIEShellListView::SetActiveWindow(CWnd *pWnd)
{
	ASSERT_VALID(pWnd);
	if (pWnd->GetSafeHwnd() == NULL)
		return;
	if (m_pActiveWnd->GetSafeHwnd() == pWnd->GetSafeHwnd())
		return;
	CRect rect;	
	m_pActiveWnd->GetWindowRect(rect);
	ScreenToClient(&rect);
	m_pActiveWnd->ShowWindow(SW_HIDE);

	m_pActiveWnd = pWnd;
	m_pActiveWnd->ShowWindow(SW_SHOW);
	m_pActiveWnd->MoveWindow(&rect);
}

void CIEShellListView::LoadShellFolderItems(const CRefreshShellFolder &rFolder)
{
	LPTVITEMDATA lptvid = reinterpret_cast<LPTVITEMDATA>(rFolder.GetItemData());
	if (lptvid == NULL)
		return;
	GetShellListCtrl().Populate(lptvid);
}

BEGIN_MESSAGE_MAP(CIEShellListView, CUIListView)
	//{{AFX_MSG_MAP(CIEShellListView)
	ON_MESSAGE(WM_SETMESSAGESTRING,OnSetmessagestring)
	ON_MESSAGE(WM_APP_UPDATE_ALL_VIEWS,OnAppUpdateAllViews)
	ON_WM_SIZE()
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_CB_IE_SEL_CHANGE,OnCBIESelChange)
	ON_MESSAGE(WM_APP_CB_IE_HIT_ENTER,OnCBIEHitEnter)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIEShellListView message handlers

void CIEShellListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (lHint == HINT_TREE_SEL_CHANGED)
	{
		GetListCtrl().SendMessage(WM_APP_UPDATE_ALL_VIEWS,(WPARAM)lHint,(LPARAM)pHint);
	}	
}

void CIEShellListView::CreateListCtrl()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pListCtrl = new CIEShellListCtrl;
	m_pActiveWnd = m_pListCtrl;
}

CIEShellListCtrl &CIEShellListView::GetShellListCtrl()
{
	// TODO: Add your specialized code here and/or call the base class

	return static_cast<CIEShellListCtrl&>(GetListCtrl());
}

LRESULT CIEShellListView::OnAppUpdateAllViews(WPARAM wParam, LPARAM lParam)
{
	if (wParam == HINT_TREE_INTERNET_FOLDER_SELECTED)
	{
		if (lParam)
		{
			if (m_htmlCtrl.GetSafeHwnd() == NULL)
			{
				if (m_htmlCtrl.Create(NULL,					// class name
					NULL,									// title
					(WS_CHILD | WS_VISIBLE),				// style
					CRect(),								// rectangle
					this,									// parent
					2000,									// control ID
					NULL))									// frame/doc context not use
				{
					m_htmlCtrl.SetNotifyWnd(GetParentFrame()->GetSafeHwnd());
					m_htmlCtrl.GoHome();
				}
			}
			else
				GetParentFrame()->SendMessage(WM_APP_CB_IE_SET_EDIT_TEXT,(WPARAM)(LPCTSTR)m_htmlCtrl.GetLocationURL(),0);	
			SetActiveWindow(&m_htmlCtrl);
		}
		else
			SetActiveWindow(m_pListCtrl);
	}
	return 1;
}

void CIEShellListView::OnSize(UINT nType, int cx, int cy) 
{
	// TODO: Add your message handler code here
	if (m_pActiveWnd && m_pActiveWnd->GetSafeHwnd())
		m_pActiveWnd->MoveWindow(0,0,cx,cy);	
	else
		CView::OnSize(nType, cx, cy);
}

LRESULT CIEShellListView::OnCBIESelChange(WPARAM wParam,LPARAM lParam)
{
	if (m_pActiveWnd->GetSafeHwnd() == m_htmlCtrl.GetSafeHwnd())
	{
		ASSERT(lParam);
		if (lParam)
			m_htmlCtrl.Navigate((LPCTSTR)lParam);
	}
	return 1L;
}

LRESULT CIEShellListView::OnCBIEHitEnter(WPARAM wParam,LPARAM lParam)
{
	if (m_pActiveWnd->GetSafeHwnd() == m_htmlCtrl.GetSafeHwnd())
	{
		ASSERT(lParam);
		if (lParam)
			m_htmlCtrl.Navigate((LPCTSTR)lParam);
	}
	return 1L;
}

LRESULT CIEShellListView::OnSetmessagestring(WPARAM wParam, LPARAM lParam)
{
	CFrameWnd *pFrame = GetParentFrame();
	if (pFrame)
		return pFrame->SendMessage(WM_SETMESSAGESTRING,wParam,lParam);
	return 0;
}
