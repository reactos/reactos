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

#include "stdafx.h"
#include "UIExplorerFrameWnd.h"
#include "UIres.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CUIExplorerFrameWnd, CUIFrameWnd)

CUIExplorerFrameWnd::CUIExplorerFrameWnd() 
{
	m_pExplorerView = NULL;
}

CUIExplorerFrameWnd::~CUIExplorerFrameWnd()
{
}

BOOL CUIExplorerFrameWnd::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	// create a splitter with 1 row, 2 columns

	// if this asserts you forgot to call SetExplorerView
	ASSERT(m_pExplorerView);
	if (m_pExplorerView == NULL)
		return CUIFrameWnd::OnCreateClient(lpcs,pContext); 
USES_CONVERSION;
	m_wndSplitter.SetSection(A2CT(GetRuntimeClass()->m_lpszClassName));
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
	{
		TRACE0("Failed to CreateStatic Splitter\n");
		return FALSE;
	}
	CSize sizeSplitter(200,0);
	// add the second splitter pane - a tab view in column 1
	// add the first splitter pane - the default view in column 0
	if (!m_wndSplitter.CreateView(0, 0,	pContext->m_pNewViewClass, sizeSplitter, pContext))
	{
		TRACE0("Failed to create first pane\n");
		return FALSE;
		TRACE1("Created %s view1\n",m_pExplorerView->m_lpszClassName);
	}
	if (m_pExplorerView)
	{
		pContext->m_pNewViewClass = m_pExplorerView;
		if (!m_wndSplitter.CreateView(0, 1,	pContext->m_pNewViewClass, CSize(0, 0), pContext))
		{
			TRACE0("Failed to create second pane\n");
			return FALSE;
		}
		TRACE1("Created %s view2\n",m_pExplorerView->m_lpszClassName);
	}
	// activate the input view
	SetActiveView((CView*)m_wndSplitter.GetPane(0,0));
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CUIExplorerFrameWnd message handlers
BEGIN_MESSAGE_MAP(CUIExplorerFrameWnd, CUIFrameWnd)
	//{{AFX_MSG_MAP(CUIExplorerFrameWnd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


