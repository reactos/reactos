////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CFlatToolBar implements dockable flat-style toolbars with "grippers"
//
#include "StdAfx.h"
#include "UIFlatBar.h"
#include "UIModulVer.h"

// if you want to see extra TRACE diagnostics, set below to TRUE
BOOL CFlatToolBar::bTRACE   = FALSE;

#ifdef _DEBUG
#define FBTRACEFN			\
	CTraceFn __fooble;	\
	if (bTRACE)				\
		TRACE
#define FBTRACE			\
	if (bTRACE)		      \
		TRACE
#else
#define FBTRACEFN TRACE
#define FBTRACE   TRACE
#endif

/////////////////
// One of these for each drop-down button
//
struct DROPDOWNBUTTON {
	DROPDOWNBUTTON* next;
	UINT idButton;								 // command ID of button
	UINT idMenu;								 // popup menu to display
};

// these define size of grippers
const GRIP_WIDTH   = 3;
const GRIP_MARGIN  = 5;

// flags stored in item data
#define	ITEMF_INITIALIZED	0x01 // item data initialized
#define	ITEMF_CHECKED		0x02 // item is checked

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////
// CFlatToolBar--does flat tool bar in MFC.
//
IMPLEMENT_DYNAMIC(CFlatToolBar, CFlatToolBarBase)

BEGIN_MESSAGE_MAP(CFlatToolBar, CFlatToolBarBase)
	ON_NOTIFY_REFLECT(TBN_DROPDOWN,  OnTbnDropDown)
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_NCCALCSIZE()
	ON_WM_ERASEBKGND()
	ON_WM_NCCREATE()
	ON_WM_PAINT()
	ON_WM_CREATE()
END_MESSAGE_MAP()

CFlatToolBar::CFlatToolBar()
{
	FBTRACE(_T("CFlatToolBar::CFlatToolBar, comctl32 version = %d\n"),
		iVerComCtl32);

	m_bDrawDisabledButtonsInColor = FALSE; // don't use color
	m_bInCoolBar = FALSE;						// assume not inside coolbar
	m_pDropDownButtons = NULL;					// list of drop-down buttons
	m_bShowDropdownArrowWhenVertical = TRUE;
	m_bNoEntry = FALSE;
}

CFlatToolBar::~CFlatToolBar()
{
	while (m_pDropDownButtons) {
		DROPDOWNBUTTON* pnext = m_pDropDownButtons->next;
		delete m_pDropDownButtons;
		m_pDropDownButtons = pnext;
	}
}

/////////////////
// Create handler: set flat style by default
//
int CFlatToolBar::OnCreate(LPCREATESTRUCT lpcs)
{
	if (CFlatToolBarBase::OnCreate(lpcs) == -1)
		return -1;
	ModifyStyle(0, TBSTYLE_FLAT);
	return 0;  // OK
}

////////////////
// Load function sets flat style after loading buttons.
//
BOOL CFlatToolBar::LoadToolBar(LPCTSTR lpszResourceName)
{
	// Set transparent/flat style before loading buttons to allow zero-height
	// border. This required because of bug in comctl32.dll that always adds
	// a border, unless flat/transparent.
	//
	DWORD dwStyle = GetStyle();
	ModifyStyle(0, TBSTYLE_FLAT|TBSTYLE_TRANSPARENT);
	BOOL bRet = CFlatToolBarBase::LoadToolBar(lpszResourceName);
	SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);
	return bRet;
}

//////////////////
// Calcluate size of client area. Adds room for grippers
//
void CFlatToolBar::OnNcCalcSize(BOOL bCalc, NCCALCSIZE_PARAMS*	pncp)
{
	if (m_bInCoolBar) { // if I am in a coolbar (rebar):
		Default();		  // ..bypass CToolBar/CControlBar

	} else {
		CRect& rc = (CRect&)pncp->rgrc[0]; // rect to return

		// copied from MFC below:
		CRect rcMargin(0,0,0,0);
		CControlBar::CalcInsideRect(rcMargin, m_dwStyle & CBRS_ORIENT_HORZ);

		// adjust non-client area for border space
		rc.left  += rcMargin.left;
		rc.top   += rcMargin.top; // MFC has -2 here, bug for newer comctl32
		rc.right += rcMargin.right;
		rc.bottom+= rcMargin.bottom;
	}
}

//////////////////
// MFC doesn't handle moving a TBSTYLE_FLAT toolbar correctly. The simplest
// way to fix it is to repaint whatever was underneath whenever the toolbar
// moves. This is done in this and the following function. All this stuff is
// only required because flat toolbars paint transparently (don't paint their
// backgrounds).
// 
void CFlatToolBar::OnWindowPosChanging(LPWINDOWPOS lpwp)
{
	if (m_bInCoolBar)
		// inside coolbars, don't do MFC thing
		Default();
	else {
		CFlatToolBarBase::OnWindowPosChanging(lpwp);
		if (!(lpwp->flags & SWP_NOMOVE))
			GetWindowRect(&m_rcOldPos);		 // remember old position
	}
}

//////////////////
// Now toolbar has really moved: repaint area beneath old position
//
void CFlatToolBar::OnWindowPosChanged(LPWINDOWPOS lpwp)
{
	if (m_bInCoolBar) {
		Default();
	} else {
		CFlatToolBarBase::OnWindowPosChanged(lpwp);
		if (!(lpwp->flags & SWP_NOMOVE)) {	 // if moved:
			InvalidateOldPos(m_rcOldPos);		 // invalidate area of old position

			// Now paint my non-client area at the new location.
			// Without this, you will still have a partial display bug (try it!)
			SendMessage(WM_NCPAINT);
		}
	}
}

//////////////////
// Invalidate toolbar rectangle. Because flat toolbars are transparent,
// this requires invalidating parent and all siblings that intersect the
// rectangle.
//
void CFlatToolBar::InvalidateOldPos(const CRect& rcInvalid)
{
	// make parent paint the area beneath rectangle
	CWnd* pParent = GetParent();		// parent (dock bar/frame) window
	ASSERT_VALID(pParent);				// check
	CRect rc = rcInvalid;				// copy rectangle
	pParent->ScreenToClient(&rc);		// convert to parent client coords
	pParent->InvalidateRect(&rc);		// invalidate

	// now do same for each sibling too
	for (CWnd* pSib = pParent->GetWindow(GW_CHILD);
		  pSib;
		  pSib=pSib->GetNextWindow(GW_HWNDNEXT)) {

		CRect rc;										// window rect of sibling
		pSib->GetWindowRect(&rc);					// ...
		if (rc.IntersectRect(rc, rcInvalid)) {	// if intersects invalid rect
			pSib->ScreenToClient(&rc);				// convert to sibling coords
			pSib->InvalidateRect(&rc);				// invalidate
			pSib->SendMessage(WM_NCPAINT);		// nonclient area too!
		}
	}
}

////////////////
// Override to avoid MFC in case I'm inside a coolbar
//
BOOL CFlatToolBar::OnEraseBkgnd(CDC* pDC)
{
	return m_bInCoolBar ? Default() : CFlatToolBarBase::OnEraseBkgnd(pDC);
}

//////////////////
// If toolbar is inside a coolbar, need to make the parent frame
// my owner so it will get notifications.
//
BOOL CFlatToolBar::OnNcCreate(LPCREATESTRUCT lpcs)
{
	CWnd* pParent = GetParent();
	ASSERT(pParent);
	TCHAR classname[64];
	GetClassName(pParent->m_hWnd, classname, countof(classname));
	if (_tcscmp(classname, REBARCLASSNAME)==0) {
		CFrameWnd* pFrame = GetParentFrame();
		ASSERT_VALID(pFrame);
		SetOwner(pFrame);
		m_bInCoolBar = TRUE;
	}
	return CFlatToolBarBase::OnNcCreate(lpcs);
}

//////////////////
// Avoid MFC if I'm inside a coolbar
//
void CFlatToolBar::OnPaint()
{
	if (m_bInCoolBar)
		Default();	// bypass CToolBar/CControlBar
	else
		CFlatToolBarBase::OnPaint();
}

////////////////////////////////////////////////////////////////
// Stuff for handling drop-down buttons in toobars
//

//////////////////
// Add dropdown buttons.
// The manager takes care of setting appropriate styles, etc.
//
// Args:
//		- array of LONGs: MAKELONG(commandID, menuID)
//		- number of buttons
//
BOOL CFlatToolBar::AddDropDownButton(UINT nIDButton, UINT nIDMenu, BOOL bArrow)
{
	ASSERT_VALID(this);

	DROPDOWNBUTTON* pb = FindDropDownButton(nIDButton);
	if (!pb) {
		pb = new DROPDOWNBUTTON;
		ASSERT(pb);
		pb->next = m_pDropDownButtons;
		m_pDropDownButtons = pb;
	}
	pb->idButton = nIDButton;
	pb->idMenu   = nIDMenu;

	int iButton = CommandToIndex(nIDButton);

	DWORD dwStyle = GetButtonStyle(iButton);
	dwStyle |= TBSTYLE_DROPDOWN;
	SetButtonStyle(iButton, dwStyle);

	if (bArrow)
		SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	return TRUE;
}

//////////////////
// Find buttons structure for given ID
//
DROPDOWNBUTTON* CFlatToolBar::FindDropDownButton(UINT nID)
{
	for (DROPDOWNBUTTON* pb = m_pDropDownButtons; pb; pb = pb->next) {
		if (pb->idButton == nID)
			return pb;
	}
	return NULL;
}

//////////////////
// Handle TBN_DROPDOWN
// Default is to display the specified menu at the right place.
// You can override to generate dynamic menus
//
// Args:
//		- NMTOOLBAR struct from TBN_DROPDOWN
//		- command id of button
//		- point to display menu at
//
void CFlatToolBar::OnTbnDropDown(NMHDR* pNMHDR, LRESULT* pRes)
{
	const NMTOOLBAR& nmtb = *(NMTOOLBAR*)pNMHDR;

	// get location of button
	CRect rc;
	GetRect(nmtb.iItem, rc);
	ClientToScreen(&rc);

	// call virtual function to display dropdown menu
	OnDropDownButton(nmtb, nmtb.iItem, rc);
}

/////////////////
// Virtual fn you can override to hand drop-down button
// events with more friendly args
//
void CFlatToolBar::OnDropDownButton(const NMTOOLBAR& nmtb, UINT nID, CRect rc)
{
	DROPDOWNBUTTON* pb = FindDropDownButton(nmtb.iItem);
	if (pb && pb->idMenu) {

		// load and display popup menu
		CMenu menu;
		VERIFY(menu.LoadMenu(pb->idMenu));
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,
			rc.left, rc.bottom, GetOwner(), &rc);
	}
}

