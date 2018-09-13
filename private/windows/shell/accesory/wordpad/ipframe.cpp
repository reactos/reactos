// ipframe.cpp : implementation of the CInPlaceFrame class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "wordpad.h"
#include "formatba.h"
#include "ruler.h"
#include "ipframe.h"
#include "wordpdoc.h"
#include "wordpvw.h"
#include "colorlis.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInPlaceFrame

IMPLEMENT_DYNCREATE(CInPlaceFrame, COleIPFrameWnd)

BEGIN_MESSAGE_MAP(CInPlaceFrame, COleIPFrameWnd)
	//{{AFX_MSG_MAP(CInPlaceFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_HELP, OnHelpFinder)
	ON_COMMAND(ID_CHAR_COLOR, OnCharColor)
	ON_COMMAND(ID_HELP_INDEX, OnHelpFinder)
	ON_COMMAND(ID_PEN_TOGGLE, OnPenToggle)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(ID_VIEW_TOOLBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FORMATBAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(ID_VIEW_FORMATBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RULER, OnUpdateControlBarMenu)
	ON_COMMAND_EX(ID_VIEW_RULER, OnBarCheck)
	ON_MESSAGE(WM_SIZECHILD, OnResizeChild)
	ON_MESSAGE(WPM_BARSTATE, OnBarState)
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
//	ON_COMMAND(ID_CONTEXT_HELP, COleIPFrameWnd::OnContextHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars

static UINT BASED_CODE toolButtons[] =
{
	// same order as in the bitmap 'itoolbar.bmp'
	ID_EDIT_CUT,
	ID_EDIT_COPY,
	ID_EDIT_PASTE,

		ID_SEPARATOR,
	ID_PEN_TOGGLE,
	ID_PEN_PERIOD,
	ID_PEN_SPACE,
	ID_PEN_BACKSPACE,
	ID_PEN_NEWLINE,
	ID_PEN_LENS
};

#define NUM_PEN_ITEMS 7
#define NUM_PEN_TOGGLE 5

static UINT BASED_CODE format[] =
{
	// same order as in the bitmap 'format.bmp'
		ID_SEPARATOR, // font name combo box
		ID_SEPARATOR,
		ID_SEPARATOR, // font size combo box
		ID_SEPARATOR,
        ID_SEPARATOR, // font script combo box
        ID_SEPARATOR,
	ID_CHAR_BOLD,
	ID_CHAR_ITALIC,
	ID_CHAR_UNDERLINE,
	ID_CHAR_COLOR,
		ID_SEPARATOR,
	ID_PARA_LEFT,
	ID_PARA_CENTER,
	ID_PARA_RIGHT,
		ID_SEPARATOR,
	ID_INSERT_BULLET,
};

/////////////////////////////////////////////////////////////////////////////
// CInPlaceFrame construction/destruction

int CInPlaceFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleIPFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// CResizeBar implements in-place resizing.
	if (!m_wndResizeBar.Create(this))
	{
		TRACE0("Failed to create resize bar\n");
		return -1;      // fail to create
	}

	if (!CreateRulerBar(this))
		return FALSE;

	// By default, it is a good idea to register a drop-target that does
	//  nothing with your frame window.  This prevents drops from
	//  "falling through" to a container that supports drag-drop.
	m_dropTarget.Register(this);

	return 0;
}

// OnCreateControlBars is called by the framework to create control bars on the
//  container application's windows.  pWndFrame is the top level frame window of
//  the container and is always non-NULL.  pWndDoc is the doc level frame window
//  and will be NULL when the container is an SDI application.  A server
//  application can place MFC control bars on either window.
BOOL CInPlaceFrame::OnCreateControlBars(CFrameWnd* pWndFrame, CFrameWnd* /*pWndDoc*/)
{
	if (!CreateToolBar(pWndFrame))
		return FALSE;

	if (!CreateFormatBar(pWndFrame))
		return FALSE;

	// set owner to this window, so messages are delivered to correct app
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndFormatBar.EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
	pWndFrame->EnableDocking(CBRS_ALIGN_ANY);
	pWndFrame->DockControlBar(&m_wndToolBar);
	pWndFrame->DockControlBar(&m_wndFormatBar);

	m_wndToolBar.SetOwner(this);
	m_wndFormatBar.SetOwner(this);
	m_wndRulerBar.SetOwner(this);
	OnBarState(1, RD_EMBEDDED); //load bar state
	return TRUE;
}

BOOL CInPlaceFrame::CreateToolBar(CWnd* pWndFrame)
{
	// Create toolbar on client's frame window
	ASSERT(m_wndToolBar.m_hWnd == NULL);
	int nPen = GetSystemMetrics(SM_PENWINDOWS) ? NUM_PEN_TOGGLE : 
		NUM_PEN_ITEMS;
	UINT nID = theApp.m_bLargeIcons ? 
		IDR_SRVR_INPLACE_BIG : IDR_SRVR_INPLACE;
	if (!m_wndToolBar.Create(pWndFrame, WS_CHILD|WS_VISIBLE|CBRS_TOP|
			CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC)||
		!m_wndToolBar.LoadBitmap(nID) ||
		!m_wndToolBar.SetButtons(toolButtons, 
			sizeof(toolButtons)/sizeof(UINT) - nPen))
	{
		TRACE0("Failed to create toolbar\n");
		return FALSE;      // fail to create
	}
	if (theApp.m_bLargeIcons)
		m_wndToolBar.SetSizes(CSize(31,30), CSize(24,24));
	else
		m_wndToolBar.SetSizes(CSize(23,22), CSize(16,16));
	CString str;
	str.LoadString(IDS_TITLE_TOOLBAR);
	m_wndToolBar.SetWindowText(str);
	return TRUE;
}

BOOL CInPlaceFrame::CreateFormatBar(CWnd* pWndFrame)
{
	ASSERT(m_wndFormatBar.m_hWnd == NULL);
	m_wndFormatBar.m_hWndOwner = m_hWnd;
	UINT nID = theApp.m_bLargeIcons ? IDB_FORMATBAR_BIG : IDB_FORMATBAR;
	if (!m_wndFormatBar.Create(pWndFrame, WS_CHILD|WS_VISIBLE|CBRS_TOP|
		CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_HIDE_INPLACE|CBRS_SIZE_DYNAMIC, ID_VIEW_FORMATBAR) ||
		!m_wndFormatBar.LoadBitmap(nID) ||
		!m_wndFormatBar.SetButtons(format, 
			sizeof(format)/sizeof(UINT)))
	{
		TRACE0("Failed to create FormatBar\n");
		return FALSE;      // fail to create
	}

	if (theApp.m_bLargeIcons)
		m_wndFormatBar.SetSizes(CSize(31,30), CSize(24,24));
	else
		m_wndFormatBar.SetSizes(CSize(23,22), CSize(16,16));
	CString str;
	str.LoadString(IDS_TITLE_FORMATBAR);
	m_wndFormatBar.SetWindowText(str);
	m_wndFormatBar.PositionCombos();
	return TRUE;
}

CInPlaceFrame::CreateRulerBar(CWnd* pWndFrame)
{
	if (!m_wndRulerBar.Create(pWndFrame, 
		WS_CHILD|WS_VISIBLE|CBRS_ALIGN_TOP|CBRS_HIDE_INPLACE, ID_VIEW_RULER))
	{
		TRACE0("Failed to create ruler\n");
		return FALSE;      // fail to create
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CInPlaceFrame Operations

/////////////////////////////////////////////////////////////////////////////
// CInPlaceFrame diagnostics

#ifdef _DEBUG
void CInPlaceFrame::AssertValid() const
{
	COleIPFrameWnd::AssertValid();
}

void CInPlaceFrame::Dump(CDumpContext& dc) const
{
	COleIPFrameWnd::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CInPlaceFrame commands

void CInPlaceFrame::OnDestroy()
{
	m_wndToolBar.DestroyWindow();
	m_wndFormatBar.DestroyWindow();
	COleIPFrameWnd::OnDestroy();
}

void CInPlaceFrame::RepositionFrame(LPCRECT lpPosRect, LPCRECT lpClipRect)
{
	CRect rectNew = lpPosRect;
	rectNew.left -= HORZ_TEXTOFFSET;
	rectNew.top -= VERT_TEXTOFFSET;
	m_wndResizeBar.BringWindowToTop();
	COleIPFrameWnd::RepositionFrame(&rectNew, lpClipRect);
	CWnd* pWnd = GetActiveView();
	if (pWnd != NULL)
		pWnd->BringWindowToTop();
	m_wndRulerBar.BringWindowToTop();
}

void CInPlaceFrame::RecalcLayout(BOOL bNotify)
{
	if (m_wndResizeBar.m_hWnd != NULL)
		m_wndResizeBar.BringWindowToTop();
	COleIPFrameWnd::RecalcLayout(bNotify);
	CWnd* pWnd = GetActiveView();
	if (pWnd != NULL)
		pWnd->BringWindowToTop();
	if (m_wndRulerBar.m_hWnd != NULL)
		m_wndRulerBar.BringWindowToTop();

	// at least 12 pt region plus ruler if it exists
	CDisplayIC dc;
	CSize size;
	size.cy = MulDiv(12, dc.GetDeviceCaps(LOGPIXELSY), 72)+1;
	size.cx = dc.GetDeviceCaps(LOGPIXELSX)/4; // 1/4"
	size.cx += HORZ_TEXTOFFSET; //adjust for offset
	size.cy += VERT_TEXTOFFSET;
	if (m_wndRulerBar.m_hWnd != NULL && m_wndRulerBar.IsVisible())
	{
		CRect rect;
		m_wndRulerBar.GetWindowRect(&rect);
		size.cy += rect.Height();
	}
	m_wndResizeBar.SetMinSize(size);
}

void CInPlaceFrame::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	COleIPFrameWnd::CalcWindowRect(lpClientRect, nAdjustType);
}

LRESULT CInPlaceFrame::OnResizeChild(WPARAM /*wParam*/, LPARAM lParam)
{
	// notify the container that the rectangle has changed!
	CWordPadDoc* pDoc = (CWordPadDoc*)GetActiveDocument();
	if (pDoc == NULL)
		return 0;

	ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CWordPadDoc)));

	// get new rect and parent
	CRect rectNew;
	rectNew.CopyRect((LPCRECT)lParam);
	CWnd* pParentWnd = GetParent();
	ASSERT_VALID(pParentWnd);

	// convert rectNew relative to pParentWnd
	ClientToScreen(&rectNew);
	pParentWnd->ScreenToClient(&rectNew);

	if (m_wndRulerBar.GetStyle()&WS_VISIBLE)
	{
		CRect rect;
		m_wndRulerBar.GetWindowRect(&rect);
		rectNew.top += rect.Height();
	}
	rectNew.left += HORZ_TEXTOFFSET;
	rectNew.top += VERT_TEXTOFFSET;

	// adjust the new rectangle for the current control bars
	CWnd* pLeftOver = GetDlgItem(AFX_IDW_PANE_FIRST);
	ASSERT(pLeftOver != NULL);
	CRect rectCur = m_rectPos;
	pLeftOver->CalcWindowRect(&rectCur, CWnd::adjustOutside);
	rectNew.left += m_rectPos.left - rectCur.left;
	rectNew.top += m_rectPos.top - rectCur.top;
	rectNew.right -= rectCur.right - m_rectPos.right;
	rectNew.bottom -= rectCur.bottom - m_rectPos.bottom;
	OnRequestPositionChange(rectNew);

	return 0;
}

LONG CInPlaceFrame::OnBarState(UINT wParam, LONG lParam)
{
	if (lParam == -1)
		return 0L;
	if (wParam == 0)
	{
		GetDockState(theApp.GetDockState(RD_EMBEDDED));
		ASSERT(m_pMainFrame != NULL);
		m_pMainFrame->GetDockState(theApp.GetDockState(RD_EMBEDDED, FALSE));
	}
	else
	{
		SetDockState(theApp.GetDockState(RD_EMBEDDED));
		m_pMainFrame->SetDockState(theApp.GetDockState(RD_EMBEDDED, FALSE));
	}
	return 0L;
}

void CInPlaceFrame::OnHelpFinder() 
{
    ::HtmlHelpA( ::GetDesktopWindow(), "wordpad.chm", HH_DISPLAY_TOPIC, 0L );
}

void CInPlaceFrame::OnCharColor() 
{
	CColorMenu colorMenu;
	CRect rc;
	int index = m_wndFormatBar.CommandToIndex(ID_CHAR_COLOR);
	m_wndFormatBar.GetItemRect(index, &rc);
	m_wndFormatBar.ClientToScreen(rc);
	colorMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON,rc.left,rc.bottom, this);
}

void CInPlaceFrame::OnPenToggle() 
{
	static int nPen = 0;
	m_wndToolBar.SetButtons(toolButtons, sizeof(toolButtons)/sizeof(UINT) - nPen);
	nPen = (nPen == 0) ? NUM_PEN_TOGGLE : 0;
	m_wndToolBar.Invalidate();
	m_wndToolBar.GetParentFrame()->RecalcLayout();
}
