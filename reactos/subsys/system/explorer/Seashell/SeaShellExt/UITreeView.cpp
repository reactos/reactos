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

// TabClientView.cpp : implementation file
//

#include "stdafx.h"
#include "UITreeView.h"
#include "UIMessages.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUITreeView

IMPLEMENT_DYNAMIC(CUITreeView, CView)

CUITreeView::CUITreeView(UINT nID) : m_nID(nID), m_pTreeCtrl(NULL)
{
	m_Style = WS_VISIBLE | /*WS_BORDER |*/ WS_CHILD | LVS_REPORT;
}

CUITreeView::~CUITreeView()
{
	delete m_pTreeCtrl;
}

CUITreeCtrl &CUITreeView::GetTreeCtrl()
{
	ASSERT(m_pTreeCtrl);
	return *m_pTreeCtrl;
}

void CUITreeView::CreateTreeCtrl()
{
	m_pTreeCtrl = new CUITreeCtrl;
}

void CUITreeView::SelectionChanged(const CRefresh &Refresh)
{
}

BEGIN_MESSAGE_MAP(CUITreeView, CView)
	//{{AFX_MSG_MAP(CUITreeView)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_UPDATE_ALL_VIEWS, OnAppUpdateAllViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUITreeView drawing

void CUITreeView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CUITreeView diagnostics

#ifdef _DEBUG
void CUITreeView::AssertValid() const
{
	CView::AssertValid();
}

void CUITreeView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUITreeView message handlers

int CUITreeView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateTreeCtrl();

	if (GetTreeCtrl().Create(m_Style,CRect(0,0,0,0),this,m_nID) == -1)
		return -1;

	return 0;
}

BOOL CUITreeView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.lpszClass = AfxRegisterWndClass(
				  CS_DBLCLKS,                       
				  NULL,                             
				  NULL,                             
				  NULL); 
	ASSERT(cs.lpszClass);
	BOOL ret = CView::PreCreateWindow(cs);
	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	return ret;
}

void CUITreeView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	GetTreeCtrl().SetFocus();	
}

void CUITreeView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (GetTreeCtrl().GetSafeHwnd())
		GetTreeCtrl().MoveWindow(0,0,cx,cy);	
}

LRESULT CUITreeView::OnAppUpdateAllViews( WPARAM wParam, LPARAM lParam )
{
	ASSERT(wParam);
	GetDocument()->UpdateAllViews(NULL,wParam,(CObject*)lParam);
	return 1L;
}

void CUITreeView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (lHint == HINT_TREE_SEL_CHANGED)
	{
		ASSERT(pHint);
		ASSERT_KINDOF(CRefresh,pHint);
		CRefresh *pRefresh = (CRefresh*)pHint;
		SelectionChanged(*pRefresh);
	}	
}

