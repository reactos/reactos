////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CMenuBar implements menu bar for MFC. See MenuBar.h for how
// to use, and also the MBTest sample application.
//
#include "StdAfx.h"
#include "UIMenuBar.h"

const UINT MB_SET_MENU_NULL = WM_USER + 1100;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// if you want to see extra TRACE diagnostics, set CMenuBar::bTRACE = TRUE
BOOL CMenuBar::bTRACE = FALSE;

#ifdef _DEBUG
#define MBTRACEFN			\
	CTraceFn __fooble;	\
	if (CMenuBar::bTRACE)\
		TRACE
#define MBTRACE			\
	if (CMenuBar::bTRACE)\
		TRACE
#else
#define MBTRACEFN TRACE
#define MBTRACE   TRACE
#endif

IMPLEMENT_DYNAMIC(CMenuBar, CFlatToolBar)

BEGIN_MESSAGE_MAP(CMenuBar, CFlatToolBar)
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI_RANGE(0, 256, OnUpdateMenuButton)
	ON_MESSAGE(MB_SET_MENU_NULL, OnSetMenuNull)
END_MESSAGE_MAP()

CMenuBar::CMenuBar()
{
	if (iVerComCtl32 <= 470)
		AfxMessageBox(_T("Warning: This program requires comctl32.dll version 4.71 or greater."));

	m_iTrackingState = TRACK_NONE;		 // initial state: not tracking 
	m_iPopupTracking = m_iNewPopup = -1; // invalid
	m_hmenu = NULL;
	m_hMenuTracking = NULL;
	m_bAutoRemoveFrameMenu = TRUE;		 // set frame's menu to NULL
}

CMenuBar::~CMenuBar()
{
}

//////////////////
// Menu bar was created: install hook into owner window
//
int CMenuBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFlatToolBar::OnCreate(lpCreateStruct)==-1)
		return -1;
	UpdateFont();
	CWnd* pFrame = GetOwner();
	ASSERT_VALID(pFrame);
	m_frameHook.Install(this, *pFrame);
	return 0; // OK
}

//////////////////
// Set menu bar font from current system menu font
//
void CMenuBar::UpdateFont()
{
	static CFont font;
	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0);
	if ((HFONT)font)
		font.DeleteObject();
	VERIFY(font.CreateFontIndirect(&info.lfMenuFont));
	SetFont(&font);
}

//////////////////
// The reason for having this is so MFC won't automatically disable
// the menu buttons. Assumes < 256 top-level menu items. The ID of
// the ith menu button is i. IOW, the index and ID are the same.
//
void CMenuBar::OnUpdateMenuButton(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
	if (IsValidButton(pCmdUI->m_nID))
		pCmdUI->Enable(TRUE);
}

//////////////////
// Recompute layout of menu bar
//
void CMenuBar::RecomputeMenuLayout()
{
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE |
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

//////////////////
// Make frame recalculate control bar sizes after menu change
//
void CMenuBar::RecomputeToolbarSize()
{
	// Force toolbar to recompute size
	CFrameWnd* pFrame = (CFrameWnd*)GetOwner();
	ASSERT_VALID(pFrame);
	ASSERT(pFrame->IsFrameWnd());
	pFrame->RecalcLayout();

	// floating frame
	pFrame = GetParentFrame();
	if (pFrame->IsKindOf(RUNTIME_CLASS(CMiniFrameWnd)))
		pFrame->RecalcLayout();	
}

//////////////////
// Set tracking state: none, button, or popup
//
void CMenuBar::SetTrackingState(TRACKINGSTATE iState, int iButton)
{
	ASSERT_VALID(this);
	if (iState != m_iTrackingState) {
		if (iState == TRACK_NONE)
			iButton = -1;

#ifdef _DEBUG
		static LPCTSTR StateName[] = { _T("NONE"), _T("BUTTON"), _T("POPUP") };
		MBTRACE(_T("CMenuBar::SetTrackingState to %s, button=%d\n"),
			StateName[iState], iButton);
#endif

		SetHotItem(iButton);					 // could be none (-1)

		if (iState==TRACK_POPUP) {
			// set related state stuff
			m_bEscapeWasPressed = FALSE;	 // assume Esc key not pressed
			m_bProcessRightArrow =			 // assume left/right arrow..
				m_bProcessLeftArrow = TRUE; // ..will move to prev/next popup
			m_iPopupTracking = iButton;	 // which popup I'm tracking
		}
		m_iTrackingState = iState;
	}
}

//////////////////
// Toggle state from home state to button-tracking and back
//
void CMenuBar::ToggleTrackButtonMode()
{
	ASSERT_VALID(this);
	if (m_iTrackingState == TRACK_NONE || m_iTrackingState == TRACK_BUTTON) {
		SetTrackingState(m_iTrackingState == TRACK_NONE ?
			TRACK_BUTTON : TRACK_NONE, 0);
	}
}

//////////////////
// Get button index before/after a given button
//
int CMenuBar::GetNextOrPrevButton(int iButton, BOOL bPrev)
{
	ASSERT_VALID(this);
	if (bPrev) {
		iButton--;
		if (iButton <0)
			iButton = GetButtonCount() - 1;
	} else {
		iButton++;
		if (iButton >= GetButtonCount())
			iButton = 0;
	}
	return iButton;
}

/////////////////
// This is to correct a bug in the system toolbar control: TB_HITTEST only
// looks at the buttons, not the size of the window. So it returns a button
// hit even if that button is totally outside the size of the window!
//
int CMenuBar::HitTest(CPoint p) const
{
	int iHit = CFlatToolBar::HitTest(p);
	if (iHit>0) {
		CRect rc;
		GetClientRect(&rc);
		if (!rc.PtInRect(p)) // if point is outside window
			iHit = -1;			// can't be a hit!
	}
	return iHit;
}

//////////////////
// Load a different menu. The HMENU must not belong to any CMenu,
// and you must free it when you're done. Returns old menu.
//
HMENU CMenuBar::LoadMenu(HMENU hmenu)
{
	UINT iPrevID=(UINT)-1;
	ASSERT(::IsMenu(hmenu));
	ASSERT_VALID(this);

	if (m_bAutoRemoveFrameMenu) {
		CFrameWnd* pFrame = GetParentFrame();
		if (::GetMenu(*pFrame)!=NULL) {
			// I would like to set the frame's menu to NULL now, but if I do, MFC
			// gets all upset: it calls GetMenu and expects to have a real menu.
			// So Instead, I post a message to myself. Because the message is
			// posted, not sent, I won't process it until MFC is done with all its
			// initialization stuff. (MFC needs to set CFrameWnd::m_hMenuDefault
			// to the menu, which it gets by calling GetMenu.)
			//
			PostMessage(MB_SET_MENU_NULL, (WPARAM)pFrame->GetSafeHwnd());
		}
	}
	HMENU hOldMenu = m_hmenu;
	m_hmenu = hmenu;

	// delete existing buttons
	int nCount = GetButtonCount();
	while (nCount--) {
		VERIFY(DeleteButton(0));
	}

	SetImageList(NULL);
//	SetButtonSize(CSize(0,0)); // This barfs in VC 6.0

	DWORD dwStyle = GetStyle();
	BOOL bModifyStyle = ModifyStyle(0, TBSTYLE_FLAT|TBSTYLE_TRANSPARENT);

	// add text buttons
	UINT nMenuItems = hmenu ? ::GetMenuItemCount(hmenu) : 0;

	for (UINT i=0; i < nMenuItems; i++) {
		TCHAR name[64];
		memset(name, 0, sizeof(name)); // guarantees double-0 at end
		if (::GetMenuString(hmenu, i, name, countof(name)-1, MF_BYPOSITION)) {
			TBBUTTON tbb;
			memset(&tbb, 0, sizeof(tbb));
			tbb.idCommand = ::GetMenuItemID(hmenu, i);

			// Because the toolbar is too brain-damaged to know if it already has
			// a string, and is also too brain-dead to even let you delete strings,
			// I have to determine if each string has been added already. Otherwise
			// in a MDI app, as the menus are repeatedly switched between doc and
			// no-doc menus, I will keep adding strings until somebody runs out of
			// memory. Sheesh!
			// 
			int iString = -1;
			for (int j=0; j<m_arStrings.GetSize(); j++) {
				if (m_arStrings[j] == name) {
					iString = j; // found it
					break;
				}
			}
			if (iString <0) {
				// string not found: add it
				iString = AddStrings(name);
				m_arStrings.SetAtGrow(iString, name);
			}

			tbb.iString = iString;
			tbb.fsState = TBSTATE_ENABLED;
			tbb.fsStyle = TBSTYLE_AUTOSIZE;
			tbb.iBitmap = -1;
			tbb.idCommand = i;
			VERIFY(AddButtons(1, &tbb));
		}
	}

	if (bModifyStyle)
		SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);
	
	if (hmenu) {
		AutoSize();								 // size buttons
		RecomputeToolbarSize();				 // and menubar itself
	}
	return hOldMenu;
}

//////////////////
// Load menu from resource
//
HMENU CMenuBar::LoadMenu(LPCTSTR lpszMenuName)
{
	return LoadMenu(::LoadMenu(AfxFindResourceHandle(lpszMenuName,RT_MENU),lpszMenuName));
}

//////////////////
// Set the frame's menu to NULL. WPARAM is HWND of frame.
//
LRESULT CMenuBar::OnSetMenuNull(WPARAM wp, LPARAM lp)
{
	HWND hwnd = (HWND)wp;
	ASSERT(::IsWindow(hwnd));
	::SetMenu(hwnd, NULL);
	return 0;
}

//////////////////
// Handle mouse click: if clicked on button, press it
// and go into main menu loop.
//
void CMenuBar::OnLButtonDown(UINT nFlags, CPoint pt)
{
	ASSERT_VALID(this);
	int iButton = HitTest(pt);
	if (iButton >= 0 && iButton<GetButtonCount()) // if mouse is over a button:
		TrackPopup(iButton);								 //   track it
	else														 // otherwise:
		CFlatToolBar::OnLButtonDown(nFlags, pt);	 //   pass it on...
}

//////////////////
// Handle mouse movement
//
void CMenuBar::OnMouseMove(UINT nFlags, CPoint pt)
{
	ASSERT_VALID(this);

	if (m_iTrackingState==TRACK_BUTTON) {

		// In button-tracking state, ignore mouse-over to non-button area.
		// Normally, the toolbar would de-select the hot item in this case.
		// 
		// Only change the hot item if the mouse has actually moved.
		// This is necessary to avoid a bug where the user moves to a different
		// button from the one the mouse is over, and presses arrow-down to get
		// the menu, then Esc to cancel it. Without this code, the button will
		// jump to wherever the mouse is--not right.

		int iHot = HitTest(pt);
		if (IsValidButton(iHot) && pt != m_ptMouse)
			SetHotItem(iHot);
		return;			 // don't let toolbar get it
	}
	m_ptMouse = pt; // remember point
	CFlatToolBar::OnMouseMove(nFlags, pt);
}

//////////////////
// Window was resized: need to recompute layout
//
void CMenuBar::OnSize(UINT nType, int cx, int cy)
{
	CFlatToolBar::OnSize(nType, cx, cy);
	RecomputeMenuLayout();
}

//////////////////
// Bar style changed: eg, moved from left to right dock or floating
//
void CMenuBar::OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle)
{
	CFlatToolBar::OnBarStyleChange(dwOldStyle, dwNewStyle);
	RecomputeMenuLayout();
}

/////////////////
// When user selects a new menu item, note whether it has a submenu
// and/or parent menu, so I know whether right/left arrow should
// move to the next popup.
//
void CMenuBar::OnMenuSelect(HMENU hmenu, UINT iItem)
{
	if (m_iTrackingState > 0) {
		// process right-arrow iff item is NOT a submenu
		m_bProcessRightArrow = (::GetSubMenu(hmenu, iItem) == NULL);
		// process left-arrow iff curent menu is one I'm tracking
		m_bProcessLeftArrow = hmenu==m_hMenuTracking;
	}
}

// globals--yuk! But no other way using windows hooks.
//
static CMenuBar*	g_pMenuBar = NULL;
static HHOOK		g_hMsgHook = NULL;

////////////////
// Menu filter hook just passes to virtual CMenuBar function
//
LRESULT CALLBACK
CMenuBar::MenuInputFilter(int code, WPARAM wp, LPARAM lp)
{
	return (code==MSGF_MENU && g_pMenuBar &&
		g_pMenuBar->OnMenuInput(*((MSG*)lp))) ? TRUE
		: CallNextHookEx(g_hMsgHook, code, wp, lp);
}

//////////////////
// Handle menu input event: Look for left/right to change popup menu,
// mouse movement over over a different menu button for "hot" popup effect.
// Returns TRUE if message handled (to eat it).
//
BOOL CMenuBar::OnMenuInput(MSG& m)
{
	ASSERT_VALID(this);
	ASSERT(m_iTrackingState == TRACK_POPUP); // sanity check
	int msg = m.message;

	if (msg==WM_KEYDOWN) {
		// handle left/right-arow.
		TCHAR vkey = m.wParam;
		if (vkey == VK_LEFT)
			MBTRACE(_T("CMenuBar::OnMenuInput: handle VK_LEFT - m_bProcessLeftArrow=%d\n"),m_bProcessLeftArrow);
		else if (vkey == VK_RIGHT)
			MBTRACE(_T("CMenuBar::OnMenuInput: handle RIGHT - m_bProcessRightArrow=%d\n"),m_bProcessRightArrow);
		if ((vkey == VK_LEFT && m_bProcessLeftArrow) ||
			(vkey == VK_RIGHT && m_bProcessRightArrow)) {
			CancelMenuAndTrackNewOne(
				GetNextOrPrevButton(m_iPopupTracking, vkey==VK_LEFT));
			return TRUE; // eat it

		} else if (vkey == VK_ESCAPE) {
			m_bEscapeWasPressed = TRUE;	 // (menu will abort itself)
		}

	} else if (msg==WM_MOUSEMOVE || msg==WM_LBUTTONDOWN) {
		// handle mouse move or click
		CPoint pt = m.lParam;
		ScreenToClient(&pt);

		if (msg == WM_MOUSEMOVE) {
			if (pt != m_ptMouse) {
				int iButton = HitTest(pt);
				if (IsValidButton(iButton) && iButton != m_iPopupTracking) {
					// user moved mouse over a different button: track its popup
					CancelMenuAndTrackNewOne(iButton);
				}
				m_ptMouse = pt;
			}

		} else if (msg == WM_LBUTTONDOWN) {
			if (HitTest(pt) == m_iPopupTracking) {
				// user clicked on same button I am tracking: cancel menu
				MBTRACE(_T("CMenuBar:OnMenuInput: handle mouse click to exit popup\n"));
				CancelMenuAndTrackNewOne(-1);
				return TRUE; // eat it
			}
		}
	}
	return FALSE; // not handled
}

//////////////////
// Cancel the current popup menu by posting WM_CANCELMODE, and track a new
// menu. iNewPopup is which new popup to track (-1 to quit).
//
void CMenuBar::CancelMenuAndTrackNewOne(int iNewPopup)
{
	MBTRACE(_T("CMenuBar::CancelMenuAndTrackNewOne: %d\n"), iNewPopup);
	ASSERT_VALID(this);
	if (iNewPopup != m_iPopupTracking) {
		GetOwner()->PostMessage(WM_CANCELMODE); // quit menu loop
		m_iNewPopup = iNewPopup;					 // go to this popup (-1 = quit)
	}
}

//////////////////
// Track the popup submenu associated with the i'th button in the menu bar.
// This fn actually goes into a loop, tracking different menus until the user
// selects a command or exits the menu.
//
void CMenuBar::TrackPopup(int iButton)
{
	MBTRACE(_T("CMenuBar::TrackPopup %d\n"), iButton);
	ASSERT_VALID(this);
	ASSERT(m_hmenu);

	CMenu menu;
	menu.Attach(m_hmenu);
	int nMenuItems = menu.GetMenuItemCount();

	while (iButton >= 0) {					 // while user selects another menu

		m_iNewPopup = -1;						 // assume quit after this
		PressButton(iButton, TRUE);		 // press the button
		UpdateWindow();						 // and force repaint now

		// post a simulated arrow-down into the message stream
		// so TrackPopupMenu will read it and move to the first item
		GetOwner()->PostMessage(WM_KEYDOWN, VK_DOWN, 1);
		GetOwner()->PostMessage(WM_KEYUP, VK_DOWN, 1);

		SetTrackingState(TRACK_POPUP, iButton); // enter tracking state

		// Need to install a hook to trap menu input in order to make
		// left/right-arrow keys and "hot" mouse tracking work.
		//
		ASSERT(g_pMenuBar == NULL);
		g_pMenuBar = this;
		ASSERT(g_hMsgHook == NULL);
		g_hMsgHook = SetWindowsHookEx(WH_MSGFILTER,
			MenuInputFilter, NULL, ::GetCurrentThreadId());

		// get submenu and display it beneath button
		TPMPARAMS tpm;
		CRect rcButton;
		GetRect(iButton, rcButton);
		ClientToScreen(&rcButton);
		CPoint pt = ComputeMenuTrackPoint(rcButton, tpm);
		HMENU hMenuPopup = ::GetSubMenu(m_hmenu, iButton);
		ASSERT(hMenuPopup);
		m_hMenuTracking = hMenuPopup;
		BOOL bRet = TrackPopupMenuEx(hMenuPopup,
			TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,
			pt.x, pt.y, GetOwner()->GetSafeHwnd(), &tpm);

		// uninstall hook.
		::UnhookWindowsHookEx(g_hMsgHook);
		g_hMsgHook = NULL;
		g_pMenuBar = NULL;

		PressButton(iButton, FALSE);	 // un-press button
		UpdateWindow();					 // and force repaint now

		// If the user exited the menu loop by pressing Escape,
		// return to track-button state; otherwise normal non-tracking state.
		SetTrackingState(m_bEscapeWasPressed ?
			TRACK_BUTTON : TRACK_NONE, iButton);

		// If the user moved mouse to a new top-level popup (eg from File to
		// Edit button), I will have posted a WM_CANCELMODE to quit
		// the first popup, and set m_iNewPopup to the new menu to show.
		// Otherwise, m_iNewPopup will be -1 as set above.
		// So just set iButton to the next popup menu and keep looping...
		iButton = m_iNewPopup;
	}
	menu.Detach();
	m_hMenuTracking = m_hmenu;
}

//////////////////
// Given button rectangle, compute point and "exclude rect" for
// TrackPopupMenu, based on current docking style, so that the menu will
// appear always inside the window.
//
CPoint CMenuBar::ComputeMenuTrackPoint(const CRect& rcButn, TPMPARAMS& tpm)
{
	tpm.cbSize = sizeof(tpm);
	DWORD dwStyle = m_dwStyle;
	CPoint pt;
	CRect& rcExclude = (CRect&)tpm.rcExclude;
	rcExclude = rcButn;
	::GetWindowRect(::GetDesktopWindow(), &rcExclude);

	switch (dwStyle & CBRS_ALIGN_ANY) {
	case CBRS_ALIGN_BOTTOM:
		pt = CPoint(rcButn.left, rcButn.top);
		rcExclude.top = rcButn.top;
		break;

	case CBRS_ALIGN_LEFT:
		pt = CPoint(rcButn.right, rcButn.top);
		rcExclude.right = rcButn.right;
		break;

	case CBRS_ALIGN_RIGHT:
		pt = CPoint(rcButn.left, rcButn.top);
		rcExclude.left = rcButn.left;
		break;

	default: //	case CBRS_ALIGN_TOP:
		pt = CPoint(rcButn.left, rcButn.bottom);
		break;
	}
	return pt;
}

//////////////////
// This function translates special menu keys and mouse actions.
// You must call it from your frame's PreTranslateMessage.
//
BOOL CMenuBar::TranslateFrameMessage(MSG* pMsg)
{
	ASSERT_VALID(this);
	ASSERT(pMsg);
	UINT msg = pMsg->message;
	if (WM_LBUTTONDOWN <= msg && msg <= WM_MOUSELAST) {
		if (pMsg->hwnd != m_hWnd && m_iTrackingState > 0) {
			// user clicked outside menu bar: exit tracking mode
			MBTRACE(_T("CMenuBar::TranslateFrameMessage: user clicked outside menu bar: end tracking\n"));
			SetTrackingState(TRACK_NONE);
		}

	} else if (msg==WM_SYSKEYDOWN || msg==WM_SYSKEYUP || msg==WM_KEYDOWN) {

		BOOL bAlt = HIWORD(pMsg->lParam) & KF_ALTDOWN; // Alt key down
		TCHAR vkey = pMsg->wParam;							  // get virt key
		if (vkey==VK_MENU ||
			(vkey==VK_F10 && !((GetKeyState(VK_SHIFT) & 0x80000000) ||
			                   (GetKeyState(VK_CONTROL) & 0x80000000) || bAlt))) {

			// key is VK_MENU or F10 with no alt/ctrl/shift: toggle menu mode
			if (msg==WM_SYSKEYUP) {
				MBTRACE(_T("CMenuBar::TranslateFrameMessage: handle menu key\n"));
				ToggleTrackButtonMode();
			}
			return TRUE;

		} else if ((msg==WM_SYSKEYDOWN || msg==WM_KEYDOWN)) {
			if (m_iTrackingState == TRACK_BUTTON) {
				// I am tracking: handle left/right/up/down/space/Esc
				switch (vkey) {
				case VK_LEFT:
				case VK_RIGHT:
					// left or right-arrow: change hot button if tracking buttons
					MBTRACE(_T("CMenuBar::TranslateFrameMessage: VK_LEFT/RIGHT\n"));
					SetHotItem(GetNextOrPrevButton(GetHotItem(), vkey==VK_LEFT));
					return TRUE;

				case VK_SPACE:  // (personally, I like SPACE to enter menu too)
				case VK_UP:
				case VK_DOWN:
					// up or down-arrow: move into current menu, if any
					MBTRACE(_T("CMenuBar::TranslateFrameMessage: VK_UP/DOWN/SPACE\n"));
					TrackPopup(GetHotItem());
					return TRUE;

				case VK_ESCAPE:
					// escape key: exit tracking mode
					MBTRACE(_T("CMenuBar::TranslateFrameMessage: VK_ESCAPE\n"));
					SetTrackingState(TRACK_NONE);
					return TRUE;
				}
			}

			// Handle alphanumeric key: invoke menu. Note that Alt-X
			// chars come through as WM_SYSKEYDOWN, plain X as WM_KEYDOWN.
			if ((bAlt || m_iTrackingState == TRACK_BUTTON) && isalnum(vkey)) {
				// Alt-X, or else X while in tracking mode
				UINT nID;
				if (MapAccelerator(vkey, nID)) {
					MBTRACE(_T("CMenuBar::TranslateFrameMessage: map acclerator\n"));
					TrackPopup(nID);	 // found menu mnemonic: track it
					return TRUE;		 // handled
				} else if (m_iTrackingState==TRACK_BUTTON && !bAlt) {
					MessageBeep(0);
					return TRUE;
				}
			}

			// Default for any key not handled so far: return to no-menu state
			if (m_iTrackingState > 0) {
				MBTRACE(_T("CMenuBar::TranslateFrameMessage: unknown key, stop tracking\n"));
				SetTrackingState(TRACK_NONE);
			}
		}
	}
	return FALSE; // not handled, pass along
}

#ifdef _DEBUG
void CMenuBar::AssertValid() const
{
	CFlatToolBar::AssertValid();
	ASSERT(m_hmenu==NULL || ::IsMenu(m_hmenu));
	ASSERT(TRACK_NONE<=m_iTrackingState && m_iTrackingState<=TRACK_POPUP);
	m_frameHook.AssertValid();
}

void CMenuBar::Dump(CDumpContext& dc) const
{
	CFlatToolBar::Dump(dc);
}
#endif

//////////////////////////////////////////////////////////////////
// CMenuBarFrameHook is used to trap menu-related messages sent to the owning
// frame. The same class is also used to trap messages sent to the MDI client
// window in an MDI app. I should really use two classes for this,
// but it uses less code to chare the same class. Note however: there
// are two different INSTANCES of CMenuBarFrameHook in CMenuBar: one for
// the frame and one for the MDI client window.
//
CMenuBarFrameHook::CMenuBarFrameHook()
{
}

CMenuBarFrameHook::~CMenuBarFrameHook()
{
	HookWindow((HWND)NULL); // (unhook)
}

//////////////////
// Install hook to trap window messages sent to frame or MDI client.
// 
BOOL CMenuBarFrameHook::Install(CMenuBar* pMenuBar, HWND hWndToHook)
{
	ASSERT_VALID(pMenuBar);
	m_pMenuBar = pMenuBar;
	return HookWindow(hWndToHook);
}

//////////////////////////////////////////////////////////////////
// Trap frame/MDI client messages specific to menubar. 
//
LRESULT CMenuBarFrameHook::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	CMenuBar& mb = *m_pMenuBar;

	switch (msg) {
	// The following messages are trapped for the frame window
	case WM_SYSCOLORCHANGE:
		mb.UpdateFont();
		break;

	case WM_MENUSELECT:
		mb.OnMenuSelect((HMENU)lp, (UINT)LOWORD(wp));
		break;
	}
	return CSubclassWnd::WindowProc(msg, wp, lp);
}
