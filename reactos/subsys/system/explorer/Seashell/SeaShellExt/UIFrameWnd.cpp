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
#include "UIFrameWnd.h"
#include "UIApp.h"
#include "UIres.h"
#include "UIMessages.h"
#include "WindowPlacement.h"
#include "IEShellTreeView.h"
#include "IEShellListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static UINT nMsgMouseWheel =
   (((::GetVersion() & 0x80000000) && LOBYTE(LOWORD(::GetVersion()) == 4)) ||
	 (!(::GetVersion() & 0x80000000) && LOBYTE(LOWORD(::GetVersion()) == 3)))
	 ? ::RegisterWindowMessage(MSH_MOUSEWHEEL) : 0;

LPCTSTR CUIFrameWnd::szMainFrame = _T("MainFrame");

IMPLEMENT_DYNAMIC(CUIFrameWnd, CFrameWnd)

CUIFrameWnd::CUIFrameWnd() : m_IDToolbar(0),m_pwndCoolBar(NULL)
{
	m_bReportCtrl = false;
}

CUIFrameWnd::~CUIFrameWnd()
{
	delete m_pwndCoolBar;
}

void CUIFrameWnd::RestoreWindow(UINT nCmdShow) 
{
// Load window placement from profile
	CWindowPlacement wp;
	if (!wp.Restore(CUIFrameWnd::szMainFrame, this))
	{
		ShowWindow(nCmdShow);
	}
}

CUIStatusBar &CUIFrameWnd::GetStatusBar()
{
	return m_wndStatusBar;
}

CReallyCoolBar &CUIFrameWnd::GetCoolBar()
{
	ASSERT(m_pwndCoolBar);
	return *m_pwndCoolBar;
}

void CUIFrameWnd::CreateCoolBar()
{
	if (m_pwndCoolBar == NULL)
		m_pwndCoolBar = new CReallyCoolBar;
}

CCoolMenuManager &CUIFrameWnd::GetMenuManager()
{
	return m_menuManager;
}

BEGIN_MESSAGE_MAP(CUIFrameWnd, CFrameWnd)
	//{{AFX_MSG_MAP(CUIFrameWnd)
	ON_WM_INITMENUPOPUP()
	ON_WM_MENUSELECT()
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(nMsgMouseWheel, OnRegisteredMouseWheel)
	ON_MESSAGE(WM_APP_CB_IE_SET_EDIT_TEXT,OnCBIESetEditText)
	ON_MESSAGE(WM_APP_CB_IE_POPULATE,OnCBIEPopulate)
	ON_MESSAGE(WM_APP_CB_IE_SEL_CHANGE,OnCBIESelChange)
	ON_MESSAGE(WM_APP_CB_IE_HIT_ENTER,OnCBIEHitEnter)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIFrameWnd message handlers

BOOL CUIFrameWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	CWinApp *pApp = AfxGetApp();
	if (pApp->IsKindOf(RUNTIME_CLASS(CUIApp)))
	{
		if (((CUIApp*)pApp)->IsMyClassRegistered())
		{
			cs.lpszClass = ((CUIApp*)pApp)->GetMyClass();
			TRACE(_T("Setting to window class %s in CUIFrameWnd\n"),cs.lpszClass);
		}
	}
	if (cs.lpszClass == NULL)
	{
		cs.lpszClass = AfxRegisterWndClass(
					  CS_DBLCLKS,                       
					  NULL,                             
					  NULL,                             
					  NULL); 
		ASSERT(cs.lpszClass);
	}
	CWindowPlacement wp;
	if (wp.GetProfileWP(szMainFrame)) 
	{
		CRect rc(wp.rcNormalPosition);
		cs.x = rc.left;
		cs.y = rc.top;
		cs.cx = rc.Width();
		cs.cy = rc.Height();
	}
	return CFrameWnd::PreCreateWindow(cs);
}

void CUIFrameWnd::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	CWindowPlacement wp;	
	wp.Save(szMainFrame,this);
	
	CFrameWnd::OnClose();
}

void CUIFrameWnd::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
	// store width of first pane and its style
	if (GetStatusBar().m_nStatusPane1Width == -1 && GetStatusBar().m_bMenuSelect)
	{
		UINT nStatusPane1Style;
		GetStatusBar().GetPaneInfo(0, GetStatusBar().m_nStatusPane1ID, 
			nStatusPane1Style, GetStatusBar().m_nStatusPane1Width);
		GetStatusBar().SetPaneInfo(0, GetStatusBar().m_nStatusPane1ID, 
			SBPS_NOBORDERS|SBPS_STRETCH, 16384);
	}
}


int CUIFrameWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	// if not tool bar set no point in continuing
	ASSERT(m_IDToolbar > 0);
	if (m_IDToolbar == 0)
		return 0;
	
	if (!GetStatusBar().Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	GetStatusBar().AddPane(ID_SEPARATOR,TRUE);
	GetStatusBar().SetPanes(FALSE);

	CreateCoolBar();
	ASSERT(m_pwndCoolBar);
	if (m_pwndCoolBar == NULL)
		return -1;
	CReallyCoolBar& cb = GetCoolBar();
	ASSERT(m_IDToolbar > 0);
	cb.SetToolBarID(m_IDToolbar);
	VERIFY(cb.Create(this,
		WS_CHILD|WS_VISIBLE|WS_BORDER|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
			RBS_TOOLTIPS|RBS_BANDBORDERS|RBS_VARHEIGHT));

	cb.SetColors(GetSysColor(COLOR_BTNTEXT),GetSysColor(COLOR_3DFACE));
	
	// install/load cool menus
	GetMenuManager().Install(this);
	GetMenuManager().LoadToolbar(m_IDToolbar);
	if (cb.IsKindOf(RUNTIME_CLASS(CWebBrowserCoolBar)))
		GetMenuManager().LoadToolbar(IDB_HOTTOOLBAR);

	if (m_bReportCtrl)
	{
		if (!m_wndFieldChooser.Create(this, IDD_FIELDCHOOSER,
			CBRS_LEFT|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_HIDE_INPLACE, ID_VIEW_FIELDCHOOSER))
			return -1;		// fail to create

		EnableDocking(CBRS_ALIGN_ANY);

		m_wndFieldChooser.EnableDocking(0);
		m_wndFieldChooser.SetWindowText(_T("Field Chooser"));

		FloatControlBar(&m_wndFieldChooser, CPoint(100, GetSystemMetrics(SM_CYSCREEN) / 3));
		ShowControlBar(&m_wndFieldChooser, FALSE, FALSE);
	}	
	return 0;
}

void CUIFrameWnd::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
  	CFrameWnd::OnMenuSelect(nItemID, nFlags, hSysMenu);

	// Restore first pane of the statusbar?
	if (nFlags == 0xFFFF && hSysMenu == 0 && GetStatusBar().m_nStatusPane1Width != -1)
	{
		GetStatusBar().m_bMenuSelect = FALSE;
		GetStatusBar().SetPaneInfo(0, GetStatusBar().m_nStatusPane1ID, GetStatusBar().m_nStatusPane1Style, GetStatusBar().m_nStatusPane1Width);
		GetStatusBar().m_nStatusPane1Width = -1;   // Set it to illegal value
	}
	else 
	{
		GetStatusBar().m_bMenuSelect = TRUE;
	}
}

void CUIFrameWnd::GetMessageString( UINT nID, CString& rMessage ) const
{
	CFrameWnd::GetMessageString(nID,rMessage);
	CUIFrameWnd *pThis = const_cast<CUIFrameWnd*>(this);
	if (pThis->GetStatusBar().IsPanes())
		pThis->GetStatusBar().SetPaneText(0,rMessage);
}

BOOL CUIFrameWnd::PreTranslateMessage(MSG* pMsg)
{
	return m_pwndCoolBar && m_pwndCoolBar->m_wndMenuBar.TranslateFrameMessage(pMsg) ?
		TRUE : CFrameWnd::PreTranslateMessage(pMsg);
}

////////////////////////////////////////////////////////////////
// CReallyCoolBar
//
IMPLEMENT_DYNAMIC(CReallyCoolBar, CCoolBar)
IMPLEMENT_DYNAMIC(CWebBrowserCoolBar, CCoolBar)

////////////////
// This is the virtual function you have to override to add bands
//
BOOL CReallyCoolBar::OnCreateBands()
{
	//////////////////
	// Create menu bar
	CMenuBar& mb = m_wndMenuBar;

	VERIFY(mb.Create(this, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
		CBRS_TOOLTIPS|CBRS_SIZE_DYNAMIC|CBRS_FLYBY|CBRS_ORIENT_HORZ));
	mb.ModifyStyle(0, TBSTYLE_TRANSPARENT);

	// Load doc or main menu. Only required since this app flips
	// between coolbar/toolbars.
	CFrameWnd *pParent = GetParentFrame();
	CFrameWnd* pFrame = pParent->GetActiveFrame();
	ASSERT(m_IDToolbar > 0);
	mb.LoadMenu(m_IDToolbar);

	CRect rc;
	mb.GetItemRect(0, &rc);
	CSize szMenu = mb.CalcDynamicLayout(-1, LM_HORZ); // get min horz size

	// create menu bar band.
	CSize szMin;
	szMin = CSize( szMenu.cx, rc.Height());
	if (!InsertBand(&mb, szMin, 0x7ff))
		return FALSE;

	//////////////////
	// Create tool bar
	CFlatToolBar& tb = m_wndToolBar;
	VERIFY(tb.Create(this,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
			CBRS_TOOLTIPS|CBRS_SIZE_DYNAMIC|CBRS_FLYBY|CBRS_ORIENT_HORZ));
	VERIFY(tb.LoadToolBar(m_IDToolbar)); 
	// use transparent so coolbar bitmap will show through
	tb.ModifyStyle(0, TBSTYLE_TRANSPARENT);

	// Get minimum size of toolbar, which is basically size of one button
	CSize szHorz = tb.CalcDynamicLayout(-1, LM_HORZ); // get min horz size
	tb.GetItemRect(0, &rc);
	szMin = rc.Size(); // CSize( szHorz.cx, rc.Height()); 

	// create toolbar band. Use largest size possible
	if (!InsertBand(&tb, szMin, 0x7fff, NULL, -1, TRUE))
		return FALSE;

	return TRUE; // OK
}

void CReallyCoolBar::SetToolBarID(UINT IDToolbar)
{
	m_IDToolbar = IDToolbar;
}

BOOL CWebBrowserCoolBar::OnCreateBands()
{
	CReallyCoolBar::OnCreateBands();

	CFlatToolBar& bb = m_wndBrowserBar;
	VERIFY(bb.Create(this,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
			CBRS_TOOLTIPS|CBRS_SIZE_DYNAMIC|CBRS_FLYBY|CBRS_ORIENT_HORZ));
	CImageList img;
	CString str;
//	bb.GetToolBarCtrl().SetButtonWidth(50, 150);
	bb.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
	img.Create(IDB_HOTTOOLBAR, 22, 0, RGB(255, 0, 255));
	bb.GetToolBarCtrl().SetHotImageList(&img);
	img.Detach();
	img.Create(IDB_COLDTOOLBAR, 22, 0, RGB(255, 0, 255));
	bb.GetToolBarCtrl().SetImageList(&img);
	img.Detach();
	bb.ModifyStyle(0, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT);
	bb.SetButtons(NULL, 7);
	// use transparent so coolbar bitmap will show through
	bb.ModifyStyle(0, TBSTYLE_TRANSPARENT);

	// set up each toolbar button
/*	str.LoadString(IDS_BROWSER_GO_BACK);
	bb.SetButtonText(0, str);
	str.LoadString(IDS_BROWSER_GO_FORWARD);
	bb.SetButtonText(1, str);
	str.LoadString(IDS_BROWSER_STOP);
	bb.SetButtonText(2, str);
	str.LoadString(IDS_BROWSER_REFRESH);
	bb.SetButtonText(3, str);
	str.LoadString(IDS_BROWSER_GO_HOME);
	bb.SetButtonText(4, str);
	str.LoadString(IDS_BROWSER_SEARCH);
	bb.SetButtonText(5, str);
	str.LoadString(IDS_BROWSER_FAVORITES);
	bb.SetButtonText(6, str);
*/
	bb.SetButtonInfo(0, ID_BROWSER_GO_BACK, TBSTYLE_BUTTON, 0);
	bb.SetButtonInfo(1, ID_BROWSER_GO_FORWARD, TBSTYLE_BUTTON, 1);
	bb.SetButtonInfo(2, ID_BROWSER_STOP, TBSTYLE_BUTTON, 2);
	bb.SetButtonInfo(3, ID_BROWSER_REFRESH, TBSTYLE_BUTTON, 3);
	bb.SetButtonInfo(4, ID_BROWSER_GO_HOME, TBSTYLE_BUTTON, 4);
	bb.SetButtonInfo(5, ID_BROWSER_SEARCH, TBSTYLE_BUTTON, 5);
	bb.SetButtonInfo(6, ID_BROWSER_FAVORITES, TBSTYLE_BUTTON, 6);


	CRect rectToolBar;
	CRect rc;
	// set up toolbar button sizes
	bb.GetItemRect(0, &rectToolBar);
//	bb.SetSizes(rectToolBar.Size(), CSize(30,20));

	// Get minimum size of toolbar, which is basically size of one button
	CSize szHorz = bb.CalcDynamicLayout(-1, LM_HORZ); // get min horz size
	bb.GetItemRect(0, &rc);
	CSize szMin = rc.Size(); // CSize( szHorz.cx, rc.Height()); 

	// create toolbar band. Use largest size possible
	if (!InsertBand(&bb, szMin, 0x7fff, NULL, -1, TRUE))
		return FALSE;

	//////////////////
	// Create combo box and fill with data

	CRect rcCombo(0,0,600,200);
	m_wndCombo.Create(WS_VISIBLE|WS_CHILD|WS_VSCROLL|CBS_DROPDOWN|
		WS_CLIPCHILDREN|WS_CLIPSIBLINGS, rcCombo, this, IDC_CB_IE_ADDRESS);
	m_wndCombo.SetFont(GetFont());

	CSize szMinCombo = m_wndBrowserBar.CalcDynamicLayout(-1, LM_HORZDOCK);	      // get min horz size
	if (!InsertBand(&m_wndCombo, szMinCombo, 0x7fff, _T("Address")))
		return FALSE;

	return TRUE;
}

// changed from MFC to use WindowFromPoint instead of GetFocus
LRESULT CUIFrameWnd::OnRegisteredMouseWheel(WPARAM wParam, LPARAM lParam)
{
	// convert from MSH_MOUSEWHEEL to WM_MOUSEWHEEL
	TRACE(_T("CUFrameWnd registered mouse wheel message\n"));
//	return CFrameWnd::OnRegisteredMouseWheel(wParam,lParam);

	WORD keyState = 0;
	keyState |= (::GetKeyState(VK_CONTROL) < 0) ? MK_CONTROL : 0;
	keyState |= (::GetKeyState(VK_SHIFT) < 0) ? MK_SHIFT : 0;

	LRESULT lResult;
	CPoint point;
	::GetCursorPos(&point);
	HWND hwFocus = ::WindowFromPoint(point);
//	HWND hwFocus = ::GetFocus();
	const HWND hwDesktop = ::GetDesktopWindow();

	if (hwFocus == NULL)
		lResult = SendMessage(WM_MOUSEWHEEL, (wParam << 16) | keyState, lParam);
	else
	{
		do {
			lParam = ::SendMessage(hwFocus, WM_MOUSEWHEEL,
				(wParam << 16) | keyState, lParam);
			hwFocus = ::GetParent(hwFocus);
		}
		while (lParam == 0 && hwFocus != NULL && hwFocus != hwDesktop);
	}
	return lResult;

}

BOOL CUIFrameWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	CFrameWnd::OnMouseWheel(nFlags,zDelta,pt); 
	// TODO: Add your message handler code here and/or call default
	TRACE(_T("CUIFrameWnd mouse wheel message\n"));
	return TRUE;
}

LRESULT CUIFrameWnd::OnCBIEPopulate(WPARAM wParam,LPARAM lParam)
{
	TRACE(_T("Received ComboBox populate message in CUIFrameWnd\n"));
	if (m_pwndCoolBar && m_pwndCoolBar->IsKindOf(RUNTIME_CLASS(CWebBrowserCoolBar)))
	{
		TRACE(_T("Sending to ShellComboBox...\n"));
		((CWebBrowserCoolBar*)m_pwndCoolBar)->GetComboBox().SendMessage(WM_APP_CB_IE_POPULATE,wParam,lParam);
	}
	return 1L;
}

LRESULT CUIFrameWnd::OnCBIEHitEnter(WPARAM wParam,LPARAM lParam)
{
	if (lParam == NULL)
		return 0L;
	if (m_pwndCoolBar && m_pwndCoolBar->IsKindOf(RUNTIME_CLASS(CWebBrowserCoolBar)))
	{
		CWinApp *pApp = AfxGetApp();
		CUIApp *pUIApp=NULL;
		if (pApp->IsKindOf(RUNTIME_CLASS(CUIApp)))
			pUIApp = (CUIApp*)pApp;
		if (pUIApp)
		{
			CView *pView = NULL;
			pView = pUIApp->GetView(RUNTIME_CLASS(CIEShellTreeView));
			if (pView)
			{
				LRESULT lResult = 0;
				if (wParam == 0)
					lResult = ((CIEShellTreeView*)pView)->GetShellTreeCtrl().SendMessage(WM_APP_CB_IE_HIT_ENTER,wParam,lParam);
				if (lResult == 0)
				{
					pView = pUIApp->GetView(RUNTIME_CLASS(CIEShellListView));
					if (pView)
						pView->SendMessage(WM_APP_CB_IE_HIT_ENTER,wParam,lParam);
				}
			}
		}
	}
	return 1L;	
}

LRESULT CUIFrameWnd::OnCBIESelChange(WPARAM wParam,LPARAM lParam)
{
	if (m_pwndCoolBar && m_pwndCoolBar->IsKindOf(RUNTIME_CLASS(CWebBrowserCoolBar)))
	{
		CWinApp *pApp = AfxGetApp();
		CUIApp *pUIApp=NULL;
		if (pApp->IsKindOf(RUNTIME_CLASS(CUIApp)))
			pUIApp = (CUIApp*)pApp;
		if (pUIApp)
		{
			CView *pView = NULL;
			if (wParam)
			{
				pView = pUIApp->GetView(RUNTIME_CLASS(CIEShellTreeView));
				if (pView)
					((CIEShellTreeView*)pView)->GetShellTreeCtrl().SendMessage(WM_APP_CB_IE_SEL_CHANGE,wParam,lParam);
			}
			else
			{
				pView = pUIApp->GetView(RUNTIME_CLASS(CIEShellListView));
				if (pView)
					pView->SendMessage(WM_APP_CB_IE_SEL_CHANGE,wParam,lParam);
			}
		}
	}
	return 1L;
}

LRESULT CUIFrameWnd::OnCBIESetEditText(WPARAM wParam,LPARAM lParam)
{
	if (m_pwndCoolBar && m_pwndCoolBar->IsKindOf(RUNTIME_CLASS(CWebBrowserCoolBar)))
	{
		if (wParam)
			((CWebBrowserCoolBar*)m_pwndCoolBar)->GetComboBox().GetEditCtrl()->SetWindowText((LPCTSTR)wParam);
	}
	return 1L;
}


////////////////////////////////////////////////////////////////
// CToolBarComboBox
//

BEGIN_MESSAGE_MAP(CToolBarComboBox, CIEShellComboBox)
END_MESSAGE_MAP()

//////////////////
