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
#include "UIListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIListView

IMPLEMENT_DYNAMIC(CUIListView, CView)

CUIListView::CUIListView(UINT nID) : m_nID(nID), m_pListCtrl(NULL)
{
	m_Style = WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS;
	m_bDragDrop = true;
}

CUIListView::~CUIListView()
{
	delete m_pListCtrl;
}

CUIODListCtrl &CUIListView::GetListCtrl()
{
	ASSERT(m_pListCtrl);
	return *m_pListCtrl;
}

void CUIListView::CreateListCtrl()
{
	m_pListCtrl = new CUIODListCtrl;
	GetListCtrl().SetDragDrop(m_bDragDrop);
}

BEGIN_MESSAGE_MAP(CUIListView, CView)
	//{{AFX_MSG_MAP(CUIListView)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_UPDATE_ALL_VIEWS, OnAppUpdateAllViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIListView drawing

void CUIListView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CUIListView diagnostics

#ifdef _DEBUG
void CUIListView::AssertValid() const
{
	CView::AssertValid();
}

void CUIListView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUIListView message handlers

int CUIListView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateListCtrl();

	// TODO: Add your specialized creation code here
USES_CONVERSION;
	GetListCtrl().SetSection(A2CT(GetRuntimeClass()->m_lpszClassName));
	GetListCtrl().ChangeStyle(m_Style);
	if (GetListCtrl().Create(m_Style,CRect(0,0,0,0),this,m_nID) == -1)
		return -1;

 	GetListCtrl().Init();

	return 0;
}

void CUIListView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	GetListCtrl().SetFocus();	
}

void CUIListView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (GetListCtrl().GetSafeHwnd())
		GetListCtrl().MoveWindow(0,0,cx,cy);	
}

LRESULT CUIListView::OnAppUpdateAllViews( WPARAM wParam, LPARAM lParam )
{
	ASSERT(wParam);
	GetDocument()->UpdateAllViews(NULL,wParam,(CObject*)lParam);
	return 1L;
}

BOOL CUIListView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.lpszClass = AfxRegisterWndClass(
				  CS_DBLCLKS,                       
				  NULL,                             
				  NULL,                             
				  NULL); 
	ASSERT(cs.lpszClass);
	
	return CView::PreCreateWindow(cs);
}
