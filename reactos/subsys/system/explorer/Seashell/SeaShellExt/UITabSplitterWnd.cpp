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
#include "UITabSplitterWnd.h"

/////////////////////////////////////////////////////////////////////////////
// CTabSplitterWnd message handlers

LPCTSTR CTabSplitterWnd::szSplitterSection = _T("Splitter");
LPCTSTR CTabSplitterWnd::szPaneWidthCurrent = _T("PaneWidthCurrent");
LPCTSTR CTabSplitterWnd::szPaneWidthMinimum = _T("PaneWidthMinimum");
LPCTSTR CTabSplitterWnd::szPaneHeightCurrent = _T("PaneHeightCurrent");
LPCTSTR CTabSplitterWnd::szPaneHeightMinimum = _T("PaneHeightMinimum");

IMPLEMENT_DYNAMIC(CTabSplitterWnd, CSplitterWnd)

BEGIN_MESSAGE_MAP(CTabSplitterWnd, CSplitterWnd)
	//{{AFX_MSG_MAP(CTabSplitterWnd)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CTabSplitterWnd::CTabSplitterWnd()
	 : m_strSection(szSplitterSection)
{	
	m_nCurCol = m_nCurRow = 0;
	m_cxCur = m_cyCur = 0;
	m_cxMin = m_cyMin = 0;
}

void CTabSplitterWnd::SetSection(LPCTSTR szSection)
{
	m_strSection = szSplitterSection;
	m_strSection += _T("\\");
	m_strSection += szSection;
}

CWnd *CTabSplitterWnd::GetActiveWnd()
{
	int row, col;
	return GetActivePane(row,col);
}

void CTabSplitterWnd::ActivateNext(BOOL bPrev)
{
	ASSERT_VALID(this);
	
	// find the coordinate of the current pane
	int row, col;
	if (GetActivePane(&row, &col) == NULL)
	{
		SetActivePane(0,0);
		return;
	}
	ASSERT(row >= 0 && row < m_nRows);
	ASSERT(col >= 0 && col < m_nCols);

	// determine next pane
	if (bPrev)
	{
		// prev
		if (--col < 0)
		{
			col = m_nCols - 1;
			if (--row < 0)
				row = m_nRows - 1;
		}
	}
	else
	{
		// next
		if (++col >= m_nCols)
		{
			col = 0;
			if (++row >= m_nRows)
				row = 0;
		}
	}

	// set newly active pane
	SetActivePane(row, col);
}

void CTabSplitterWnd::SaveSize()
{
#ifdef _DEBUG
	if (m_strSection == szSplitterSection)
		TRACE0("Warning: SetSection has not been called in IMSplitterWnd!\n");
#endif
	GetColumnInfo(0,m_cxCur,m_cxMin);
	if (m_cxCur)
		AfxGetApp()->WriteProfileInt(m_strSection,szPaneWidthCurrent,m_cxCur);
	if (m_cxMin)
		AfxGetApp()->WriteProfileInt(m_strSection,szPaneWidthMinimum,m_cxMin);
	GetRowInfo(0,m_cyCur,m_cyMin);
	if (m_cyCur)
		AfxGetApp()->WriteProfileInt(m_strSection,szPaneHeightCurrent,m_cyCur);
	if (m_cyMin)
		AfxGetApp()->WriteProfileInt(m_strSection,szPaneHeightMinimum,m_cyMin);
}

void CTabSplitterWnd::SetSize(int nCur,int nMin)
{
	if (m_nRows > 1) 
	{
		m_cyCur = nCur;
		m_cyMin = nMin;
	}
	if (m_nCols > 1) 
	{
		m_cxCur = nCur;
		m_cxMin = nMin;
	}
}

void CTabSplitterWnd::Apply()
{
	if (m_nRows > 1)
	{
		SetRowInfo(0,m_cyCur,m_cyMin);
		RecalcLayout();
	}
	else if (m_nCols > 1) 
	{
		SetColumnInfo(0,m_cxCur,m_cxMin);
		RecalcLayout();
	}
	else
		TRACE0("Applying splitter bar before creating it!\n");
}

BOOL CTabSplitterWnd::CreateView(int row,int col,CRuntimeClass* pViewClass,SIZE sizeInit,CCreateContext* pContext)
{
	if (m_nCols > 1) 
	{
		if (m_cxCur)
			sizeInit.cx = m_cxCur;
		else if (m_strSection != szSplitterSection)
			sizeInit.cx = AfxGetApp()->GetProfileInt(m_strSection,szPaneWidthCurrent,sizeInit.cx);
		m_cxCur = sizeInit.cx;
	}
	if (m_nRows > 1) 
	{
		if (m_cyCur)
			sizeInit.cy = m_cyCur;
		else if (m_strSection != szSplitterSection)
			sizeInit.cy = AfxGetApp()->GetProfileInt(m_strSection,szPaneHeightCurrent,sizeInit.cy);
		m_cyCur = sizeInit.cy;
	}
	return CSplitterWnd::CreateView(row,col,pViewClass,sizeInit,pContext);
}

void CTabSplitterWnd::StopTracking(BOOL bAccept)
{
	// save old active view
	CWnd* pOldActiveView = GetActivePane();
	CSplitterWnd::StopTracking(bAccept);
	if (bAccept) 
	{
		if (pOldActiveView == GetActivePane())
		{
			if (pOldActiveView == NULL)
			{
				if (m_nCols > 1)
					SetActivePane(0, 1); 
	//			pOldActiveView->SetFocus(); // make sure focus is restored
				if (m_nRows > 1)
					SetActivePane(0, 0); 
			}	
		}
		SaveSize();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTabSplitterWnd message handlers
void CTabSplitterWnd::OnDestroy()
{
	CSplitterWnd::OnDestroy();
	m_nCurRow = -1;
	m_nCurCol = -1;
}

void CTabSplitterWnd::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	SaveSize();
	CSplitterWnd::OnClose();
}

void CTabSplitterWnd::OnSetFocus(CWnd* pOldWnd) 
{
	CSplitterWnd::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	if (m_nCurRow >= 0 && m_nCurCol >= 0) 
	{
		SetActivePane(m_nCurRow,m_nCurCol);
		CWnd *pWnd = GetPane(m_nCurRow,m_nCurCol);
		pWnd->SetFocus();
	}
}

void CTabSplitterWnd::OnKillFocus(CWnd* pNewWnd) 
{
	CSplitterWnd::OnKillFocus(pNewWnd);
	
	// TODO: Add your message handler code here	
	GetActivePane(&m_nCurRow,&m_nCurCol);
}

// This currently only saves the first pane
void CTabSplitterWnd::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_cxCur;
		ar << m_cxMin;
		ar << m_cyCur;
		ar << m_cyMin;
	}
	else
	{
		ar >> m_cxCur;
		ar >> m_cxMin;
		ar >> m_cyCur;
		ar >> m_cyMin;
	}
}

// mouse wheel handled by the views
BOOL CTabSplitterWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// TODO: Add your message handler code here and/or call default
	TRACE(_T("SplitterWnd mouse wheel message\n"));
/*	if (m_nCurRow >= 0 && m_nCurCol >= 0) 
	{
		SetActivePane(m_nCurRow,m_nCurCol);
		CWnd *pWnd = GetPane(m_nCurRow,m_nCurCol);
	}*/
	return TRUE;
}

BOOL CTabSplitterWnd::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.lpszClass = AfxRegisterWndClass(
				  CS_DBLCLKS,                       
				  NULL,                             
				  NULL,                             
				  NULL); 
	ASSERT(cs.lpszClass);
	
	return CSplitterWnd::PreCreateWindow(cs);
}
