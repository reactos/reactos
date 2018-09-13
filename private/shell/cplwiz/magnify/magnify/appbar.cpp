/******************************************************************************
Module name: AppBar.cpp
Written by:  Jeffrey Richter
Purpose:     AppBar base class implementation file.
******************************************************************************/


#include "stdafx.h"
#include <WinReg.h>
#include "AppBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Static, AppBar-specific helper functions


void CAppBar::ResetSystemKnowledge (void) {

#ifdef _DEBUG
   // Only do this for debug builds.
   APPBARDATA abd;
   abd.cbSize = sizeof(abd);
   abd.hWnd = NULL;
   ::SHAppBarMessage(ABM_REMOVE, &abd);
#endif
}


/////////////////////////////////////////////////////////////////////////////


UINT CAppBar::GetEdgeFromPoint (DWORD fdwFlags, CPoint pt) {

   // At least one edge or floating must be allowed.
   ASSERT((fdwFlags & ABF_ALLOWANYWHERE) != 0);

   UINT uState = ABE_FLOAT;   // Assume that the AppBar is floating

   // Let's get floating out of the way first
   if ((fdwFlags & ABF_ALLOWFLOAT) != 0) {

      // Get the rectangle that bounds the size of the screen
      // minus any docked (but not-autohidden) AppBars.
      CRect rc;
      ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

      // Leave a 1/2 width/height-of-a-scrollbar gutter around the workarea
      rc.InflateRect(-GetSystemMetrics(SM_CXVSCROLL), -GetSystemMetrics(SM_CYHSCROLL));

      if (rc.PtInRect(pt) || ((fdwFlags & ABF_ALLOWANYEDGE) == 0)) {
         // If the point is in the adjusted workarea
         // OR no edges are allowed.
         return(uState);      // The AppBar should float
      }
   }

   // If we get here, the AppBar should be docked; determine the proper edge

   // Get the dimensions of the screen
   int cxScreen = GetSystemMetrics(SM_CXSCREEN);
   int cyScreen = GetSystemMetrics(SM_CYSCREEN);

   // Find the center of the screen
   CPoint ptCenter(cxScreen / 2, cyScreen / 2);

   // Find the distance from the point to the center
   CPoint ptOffset = pt - ptCenter;

   // Determine if the point is farther from the left/right or top/bottom
   BOOL fIsLeftOrRight = (AbsoluteValue(ptOffset.y) * cxScreen) <=
      (AbsoluteValue(ptOffset.x) * cyScreen);

   // If (it should be left/right, AND we allow left/right) 
   // OR we don't allow top/bottom
   if ((fIsLeftOrRight && ((fdwFlags & ABF_ALLOWLEFTRIGHT) != 0)) ||
       ((fdwFlags & ABF_ALLOWTOPBOTTOM) == 0)) {

      uState = (0 <= ptOffset.x) ? ABE_RIGHT : ABE_LEFT;
   } else {

      uState = (0 <= ptOffset.y) ? ABE_BOTTOM : ABE_TOP;
   }
   
   return(uState);   // Return calculated edge
}


/////////////////////////////////////////////////////////////////////////////
// Public member functions


CAppBar::CAppBar (UINT nIDTemplate, CWnd* pParent /*=NULL*/)
   : CDialog(nIDTemplate, pParent) {
   //{{AFX_DATA_INIT(CAppBar)
      // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
      // Setup the default width/height for the AppBar

   // Force the shell to update its list of AppBars and the workarea.
   // This is a precaution and is very useful when debugging.  If you create
   // an AppBar and then just terminate the application, the shell still 
   // thinks that the AppBar exists and the user's workarea is smaller than
   // it should be.  When a new AppBar is created, calling this function 
   // fixes the user's workarea.
   ResetSystemKnowledge();

   // Set default state of AppBar to docked on bottom with no width & height
   ZeroMemory(&m_abs, sizeof(m_abs));
   m_abs.m_cbSize                 = sizeof(m_abs);
   m_abs.m_uState                 = ABE_BOTTOM;
   m_abs.m_fAutohide              = FALSE;
   m_abs.m_fAlwaysOnTop           = FALSE;
   m_abs.m_auDimsDock[ABE_LEFT]   = 0;
   m_abs.m_auDimsDock[ABE_TOP]    = 0;
   m_abs.m_auDimsDock[ABE_RIGHT]  = 0;
   m_abs.m_auDimsDock[ABE_BOTTOM] = 0;
   m_abs.m_rcFloat.SetRectEmpty();

   m_fdwTaskBarState             = SHAppBarMessage(ABM_GETSTATE);
   m_fdwFlags                    = 0;
   m_szSizeInc                   = CSize(0, 0); // Don't allow re-sizing
   m_uStateProposedPrev          = ABE_UNKNOWN;
   m_fFullScreenAppOpen          = FALSE;
   m_fAutoHideIsVisible          = FALSE;
}


/////////////////////////////////////////////////////////////////////////////


UINT CAppBar::GetAutohideEdge (void) {

   for (UINT uEdge = ABE_LEFT; uEdge <= ABE_BOTTOM; uEdge++) {
      if (m_hWnd == (HWND) SHAppBarMessage(ABM_GETAUTOHIDEBAR, uEdge)) {
         // We are an auto-hide AppBar and we found our edge.
         return(uEdge);
      }
   }

   // NOTE: If AppBar is docked but not auto-hidden, we return ABE_UNKNOWN
   return(ABE_UNKNOWN);   
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::MimicState (DWORD fdwStateChangedMask, DWORD fdwState) {

   BOOL fAnyChange = FALSE;   // Assume that nothing changes

   // If the autohide state changed AND our style allows 
   // us to mimic the Autohide state
   if (((fdwStateChangedMask & ABS_AUTOHIDE) != 0) && 
       ((m_fdwFlags & ABF_MIMICTASKBARAUTOHIDE) != 0)) {

      BOOL fIsAutohide = (ABS_AUTOHIDE & fdwState) != 0;

      // If our state doesn't match, change our state
      if (IsBarAutohide() != fIsAutohide) {
         m_abs.m_fAutohide = fIsAutohide;
         fAnyChange = TRUE;
      }
   }

   // If the AlwaysOnTop state changed AND our style allows 
   // us to mimic the AlwaysOnTop state/
   if (((fdwStateChangedMask & ABS_ALWAYSONTOP) != 0) && 
       ((m_fdwFlags & ABF_MIMICTASKBARALWAYSONTOP) != 0)) {

      // If our state doesn't match, change our state
      m_abs.m_fAlwaysOnTop = (ABS_ALWAYSONTOP & fdwState) != 0;
      fAnyChange = TRUE;
   }
   if (fAnyChange) {
      SetState();
      ShowHiddenAppBar(FALSE);
   }
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::SetState (UINT uState) {

   // If the AppBar is registered as auto-hide, unregister it.
   UINT uEdge = GetAutohideEdge();
   if (uEdge != ABE_UNKNOWN) {
      // Our AppBar is auto-hidden, unregister it.
      SHAppBarMessage(ABM_SETAUTOHIDEBAR, uEdge, FALSE);
   }

   // Save the new requested state
   m_abs.m_uState = uState;

   switch (uState) {
      case ABE_UNKNOWN:
         // We are being completely unregisterred.
         // Probably, the AppBar window is being destroyed.
         // If the AppBar is registered as NOT auto-hide, unregister it.
         SHAppBarMessage(ABM_REMOVE);
         break;

      case ABE_FLOAT:
         // We are floating and therefore are just a regular window.
         // Tell the shell that the docked AppBar should be of 0x0 dimensions
         // so that the workspace is not affected by the AppBar.
         SHAppBarMessage(ABM_SETPOS, uState, FALSE, &CRect(0, 0, 0, 0));
			SetWindowPos(NULL, m_abs.m_rcFloat.left, m_abs.m_rcFloat.top, 
				m_abs.m_rcFloat.Width(), m_abs.m_rcFloat.Height(), 
				SWP_NOZORDER | SWP_NOACTIVATE);
         break;

      default:
         if (IsBarAutohide() && !SHAppBarMessage(ABM_SETAUTOHIDEBAR, GetState(), TRUE)) {
            // We couldn't set the AppBar on a new edge, let's dock it instead.
            m_abs.m_fAutohide = FALSE;
            // Call a virtual function to let derived classes know that the AppBar
            // changed from auto-hide to docked.
            OnAppBarForcedToDocked();
         }
         CRect rc;
         GetRect(GetState(), &rc);
         if (IsBarAutohide()) {
            SHAppBarMessage(ABM_SETPOS, ABE_LEFT, FALSE, &CRect(0, 0, 0, 0));
         } else {
            // Tell the shell where the AppBar is.
            SHAppBarMessage(ABM_SETPOS, uState, FALSE, &rc);
         }
         AdjustLocationForAutohide(m_fAutoHideIsVisible, &rc);
         // Slide window in from or out to the edge
         SlideWindow(rc);
         break;
   }

	// Set the AppBar's z-order appropriately
	const CWnd* pwnd = &wndNoTopMost;		// Assume normal Z-Order
	if (m_abs.m_fAlwaysOnTop) {	
      // If we are supposed to be always-on-top, put us there.
      pwnd = &wndTopMost;

		if (m_fFullScreenAppOpen) {
         // But, if a full-screen window is opened, put ourself at the bottom
         // of the z-order so that we don't cover the full-screen window
         pwnd = &wndBottom;
      }
	}
	SetWindowPos(pwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

   // Make sure that any auto-hide appbars stay on top of us after we move
   // even though our activation state has not changed.
   SHAppBarMessage(ABM_ACTIVATE);

   // Tell our derived class that there is a state change
   OnAppBarStateChange(FALSE, uState);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::SetState (APPBARSTATE& abs) {

   // The AppBar window doesn't exist, just update the state variables
   CopyMemory(&m_abs, &abs, min(abs.m_cbSize, sizeof(m_abs)));
	m_abs.m_cbSize = sizeof(m_abs);		// In case caller used an old version
   if (IsWindow(m_hWnd)) SetState();
}


/////////////////////////////////////////////////////////////////////////////
// Internal implementation functions


UINT CAppBar::SHAppBarMessage (DWORD dwMessage, UINT uEdge /*= ABE_FLOAT*/, 
   LPARAM lParam /*= 0*/, CRect *rc /*= NULL*/) {

   // Initialize an APPBARDATA structure.
   APPBARDATA abd;
   abd.cbSize           = sizeof(abd);
   abd.hWnd             = m_hWnd;
   abd.uCallbackMessage = s_uAppBarNotifyMsg;
   abd.uEdge            = uEdge;
   abd.rc               = (rc == NULL) ? CRect(0, 0, 0, 0) : *rc;
   abd.lParam           = lParam;
   UINT uRetVal         = ::SHAppBarMessage(dwMessage, &abd);

   // If the caller passed a rectangle, return the updated rectangle.
   if (rc != NULL) *rc = abd.rc;
   return(uRetVal);
}


/////////////////////////////////////////////////////////////////////////////


UINT CAppBar::CalcProposedState (const CPoint& pt) {

   // Force the AppBar to float if the user is holding down the Ctrl key
   // and the AppBar's style allows floating.
   BOOL fForceFloat = ((GetKeyState(VK_CONTROL) & 0x8000) != 0) 
      && ((m_fdwFlags & ABF_ALLOWFLOAT) != 0);
   return(fForceFloat ? ABE_FLOAT : GetEdgeFromPoint(m_fdwFlags, pt));
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::GetRect (UINT uStateProposed, CRect* prcProposed) {

   // This function finds the x, y, cx, cy of the AppBar window
   if (ABE_FLOAT == uStateProposed) {

      // The AppBar is floating, the proposed rectangle is correct
   } else {

      // The AppBar is docked or auto-hide

      // Set dimensions to full screen.
      *prcProposed = CRect(0, 0, 
         GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

      // Subtract off what we want from the full screen dimensions
      if (!IsBarAutohide()) {
         // Ask the shell where we can dock.
         SHAppBarMessage(ABM_QUERYPOS, uStateProposed, FALSE, prcProposed);
      }

      switch (uStateProposed) {
         case ABE_LEFT:    
            prcProposed->right = 
               prcProposed->left + m_abs.m_auDimsDock[uStateProposed];
            break;

         case ABE_TOP:
            prcProposed->bottom = 
               prcProposed->top + m_abs.m_auDimsDock[uStateProposed]; 
            break;

         case ABE_RIGHT:   
            prcProposed->left = 
               prcProposed->right - m_abs.m_auDimsDock[uStateProposed]; 
            break;

         case ABE_BOTTOM:  
            prcProposed->top = 
               prcProposed->bottom - m_abs.m_auDimsDock[uStateProposed]; 
            break;
      }
   }
}


/////////////////////////////////////////////////////////////////////////////


BOOL CAppBar::AdjustLocationForAutohide (BOOL fShow, CRect* prc) {

   if ((GetState() == ABE_UNKNOWN) || (GetState() == ABE_FLOAT) || 
      !IsBarAutohide()) {

      // If we are not docked on an edge OR we are not auto-hidden, there is
      // nothing for us to do; just return.
      return(FALSE);
   }

   // Showing/hiding doesn't change our size; only our position.
   int x = 0, y = 0;    // Assume a position of (0, 0)

   if (fShow) {

      // If we are on the right or bottom, calculate our visible position
      switch (GetState()) {
         case ABE_RIGHT:
            x = GetSystemMetrics(SM_CXSCREEN) - prc->Width();
            break;

         case ABE_BOTTOM:
            y = GetSystemMetrics(SM_CYSCREEN) - prc->Height();
            break;
      }
   } else {

      // Keep a part of the AppBar visible at all times
      const int cxVisibleBorder = 2 * GetSystemMetrics(SM_CXBORDER);
      const int cyVisibleBorder = 2 * GetSystemMetrics(SM_CYBORDER);

      // Calculate our x or y coordinate so that only the border is visible
      switch (GetState()) {
         case ABE_LEFT:   
            x = -(prc->Width() - cxVisibleBorder); 
            break;

         case ABE_RIGHT:  
            x = GetSystemMetrics(SM_CXSCREEN) - cxVisibleBorder; 
            break;

         case ABE_TOP:
            y = -(prc->Height() - cyVisibleBorder); 
            break;

         case ABE_BOTTOM:
            y = GetSystemMetrics(SM_CYSCREEN) - cyVisibleBorder; 
            break;
      }
   }
   *prc = CRect(x, y, x + prc->Width(), y + prc->Height());
   return(TRUE);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::ShowHiddenAppBar (BOOL fShow /*= TRUE*/) {

   if (m_fAutoHideIsVisible != fShow) {
      // We are chaning our visibility
      // Get our window location in screen coordinates.
      CRect rc;
      GetWindowRect(&rc);

      if (AdjustLocationForAutohide(fShow, &rc)) {
         // the rectangle was adjusted, we are an autohide bar
         // Rememebr whether we are visible or not.
         m_fAutoHideIsVisible = fShow;

         // Slide window in from or out to the edge
         SlideWindow(rc);
      }
   }
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::SlideWindow (const CRect& rcEnd) {

   BOOL fFullDragOn;

   // Only slide the window if the user has FullDrag turned on
   ::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &fFullDragOn, 0);

   // Get the current window position
   CRect rcStart;
   GetWindowRect(&rcStart);   

   if (fFullDragOn && (rcStart != rcEnd)) {

      // Get our starting and ending time.
      DWORD dwTimeStart = GetTickCount();
      DWORD dwTimeEnd = dwTimeStart + AUTOHIDETIMERINTERVAL;
      DWORD dwTime;

      while ((dwTime = ::GetTickCount()) < dwTimeEnd) {

         // While we are still sliding, calculate our new position
         int x = rcStart.left - (rcStart.left - rcEnd.left) 
            * (int) (dwTime - dwTimeStart) / AUTOHIDETIMERINTERVAL;

         int y = rcStart.top  - (rcStart.top  - rcEnd.top)  
            * (int) (dwTime - dwTimeStart) / AUTOHIDETIMERINTERVAL;

         int nWidth  = rcStart.Width()  - (rcStart.Width()  - rcEnd.Width())  
            * (int) (dwTime - dwTimeStart) / AUTOHIDETIMERINTERVAL;

         int nHeight = rcStart.Height() - (rcStart.Height() - rcEnd.Height()) 
            * (int) (dwTime - dwTimeStart) / AUTOHIDETIMERINTERVAL;

         // Show the window at its changed position
         SetWindowPos(NULL, x, y, nWidth, nHeight, 
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);
         UpdateWindow();
      }
   }

   // Make sure that the window is at its final position
   SetWindowPos(NULL, rcEnd.left, rcEnd.top, rcEnd.Width(), rcEnd.Height(),
      SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);
	UpdateWindow();
}


/////////////////////////////////////////////////////////////////////////////
// Overridable functions


void CAppBar::OnAppBarStateChange (BOOL fProposed, UINT uStateProposed) {

   // This function intentionally left blank.
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnAppBarForcedToDocked (void) {

   // Display the ppBar's caption text as the message box caption text.
   CString sz;
   GetWindowText(sz);

   ::MessageBox(NULL, 
      __TEXT("There is already an auto hidden window on this edge.\n")
      __TEXT("Only one auto hidden window is allowed on each edge."),
      sz, MB_OK | MB_ICONSTOP);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnABNFullScreenApp (BOOL fOpen) {

   // This function is called when a FullScreen window is openning or 
   // closing. A FullScreen window is a top-level window that has its caption 
   // above the top of the screen allowing the entire screen to be occupied 
   // by the window's client area.  
   
   // If the AppBar is a topmost window when a FullScreen windiw is activated, 
   // we need to change our window to a non-topmost window so that the AppBar 
   // doesn't cover the FullScreen window's client area.

   // If the FullScreen window is closing, we need to set the AppBar's 
   // Z-Order back to when the user wants it to be.
	m_fFullScreenAppOpen = fOpen;
	SetState();
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnABNPosChanged (void) {

   // The TaskBar or another AppBar has changed its size or position. 
   if ((GetState() != ABE_FLOAT) && !IsBarAutohide()) {

      // If we're not floating and we're not auto-hidden, we have to 
      // reposition our window.
      SetState();
   }
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnABNStateChange (DWORD fdwStateChangedMask, DWORD fdwState) {

   // Make our state mimic the taskbar's state.
   MimicState(fdwStateChangedMask, fdwState);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnABNWindowArrange (BOOL fBeginning) {

   // This function intentionally left blank.
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::DoDataExchange(CDataExchange* pDX) {
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppBar)
      // NOTE: the ClassWizard will add DDX and DDV calls here
   //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////


// Register a window message for the AppBar's callback notifications
UINT CAppBar::s_uAppBarNotifyMsg = 
   RegisterWindowMessage(__TEXT("AppBarNotify"));


/////////////////////////////////////////////////////////////////////////////


// Called when the AppBar recieves a g_uAppBarNotifyMsg window message
LRESULT CAppBar::OnAppBarCallbackMsg (WPARAM uNotifyMsg, LPARAM lParam) {

   switch (uNotifyMsg) {

      case ABN_FULLSCREENAPP: 
         OnABNFullScreenApp((BOOL) lParam); 
         break;

      case ABN_POSCHANGED:
         OnABNPosChanged(); 
         break;

      case ABN_WINDOWARRANGE:
         OnABNWindowArrange((BOOL) lParam); 
         break;

      case ABN_STATECHANGE:
         // The shell sends ABN_STATECHANGE notifications at inappropriate 
         // times.  So, we remember the TaskBar's current state and set
         // a mask indicating which states have actually changed. This mask
         // and the state information is passed to the derived class.

         // Get the state of the Taskbar
         DWORD fdwTaskBarState = SHAppBarMessage(ABM_GETSTATE);

         // Prepare a mask indicating which states have changed. The code in
         // the derived class should only act on the states that have changed.
         DWORD fdwStateChangedMask = 0;
         if ((fdwTaskBarState & ABS_ALWAYSONTOP) != 
             (m_fdwTaskBarState & ABS_ALWAYSONTOP)) {
            fdwStateChangedMask |= ABS_ALWAYSONTOP;
         }
         if ((fdwTaskBarState & ABS_AUTOHIDE) != 
             (m_fdwTaskBarState & ABS_AUTOHIDE)) {
            fdwStateChangedMask |= ABS_AUTOHIDE;
         }

         // Call the derived class
         OnABNStateChange(fdwStateChangedMask, fdwTaskBarState);

         // Save the TaskBar's state so that we can see exactly which states
         // change the next time be get an ABN_STATECHANGE notification.
         m_fdwTaskBarState = fdwTaskBarState;
         break;
   }

   return(0);
}


/////////////////////////////////////////////////////////////////////////////


BEGIN_MESSAGE_MAP(CAppBar, CDialog)
   ON_REGISTERED_MESSAGE(s_uAppBarNotifyMsg, OnAppBarCallbackMsg)
   ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
   ON_MESSAGE(WM_SIZING, OnSizing)
   ON_MESSAGE(WM_MOVING, OnMoving)
   ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
   //{{AFX_MSG_MAP(CAppBar)
   ON_WM_CREATE()
   ON_WM_DESTROY()
   ON_WM_WINDOWPOSCHANGED()
   ON_WM_ACTIVATE()
   ON_WM_NCMOUSEMOVE()
   ON_WM_NCHITTEST()
   ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAppBar message handlers


int CAppBar::OnCreate(LPCREATESTRUCT lpCreateStruct) {

   if (CDialog::OnCreate(lpCreateStruct) == -1)
      return(-1);

   // Associate a timer with the AppBar.  The timer is used to determine
   // when a visible, inactive, auto-hide AppBar should be re-hidden.
   SetTimer(AUTOHIDETIMERID, AUTOHIDETIMERINTERVAL, NULL);

   // Register our AppBar window with the Shell
   SHAppBarMessage(ABM_NEW);

   // Force the AppBar to mimic the state of the TaskBar
   // Assume that all states have changed
   MimicState(ABS_ALWAYSONTOP | ABS_AUTOHIDE, m_fdwTaskBarState);

   return(0);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnDestroy() {

   // Kill the Autohide timer 
   KillTimer(AUTOHIDETIMERID);

   // Unregister our AppBar window with the Shell
   SetState(ABE_UNKNOWN);

   CDialog::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos) {

   CDialog::OnWindowPosChanged(lpwndpos);

   // When our window changes position, tell the Shell so that any 
   // auto-hidden AppBar on our edge stays on top of our window making it 
   // always accessible to the user.
   SHAppBarMessage(ABM_WINDOWPOSCHANGED);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) {

   CDialog::OnActivate(nState, pWndOther, bMinimized);

   if (nState == WA_INACTIVE) {
      // Hide the AppBar if we are docked and auto-hidden
      ShowHiddenAppBar(FALSE);
   }

   // When our window changes position, tell the Shell so that any 
   // auto-hidden AppBar on our edge stays on top of our window making it 
   // always accessible to the user.
   SHAppBarMessage(ABM_ACTIVATE);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnTimer(UINT nIDEvent) {

   if (GetActiveWindow() != this) {

      // Possibly hide the AppBar if we are not the active window 
      
      // Get the position of the mouse and the AppBar's position
      // Everything must be in screen coordinates.
      CPoint pt(::GetMessagePos());
      CRect rc;
      GetWindowRect(&rc);

      // Add a little margin around the AppBar
      rc.InflateRect(2 * GetSystemMetrics(SM_CXDOUBLECLK), 
         2 * GetSystemMetrics(SM_CYDOUBLECLK));

      if (!rc.PtInRect(pt)) {
         // If the mouse is NOT over the AppBar, hide the AppBar
         ShowHiddenAppBar(FALSE);
      }
   }

   CDialog::OnTimer(nIDEvent);
}


/////////////////////////////////////////////////////////////////////////////


void CAppBar::OnNcMouseMove(UINT nHitTest, CPoint point) {

   // If we are a docked, auto-hidden AppBar, shown us
   // when the user moves over our non-client area
   ShowHiddenAppBar(TRUE);

   CDialog::OnNcMouseMove(nHitTest, point);
}


/////////////////////////////////////////////////////////////////////////////


UINT CAppBar::OnNcHitTest(CPoint point) {

   // Find out what the system thinks is the hit test code
   UINT u = CDialog::OnNcHitTest(point);

   // NOTE: If the user presses the secondary mouse button, pretend that the
   // user clicked on the client area so that we get WM_CONTEXTMENU messages
   BOOL fPrimaryMouseBtnDown = 
      (GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) 
         ? VK_RBUTTON : VK_LBUTTON) & 0x8000) != 0;

   if ((u == HTCLIENT) && fPrimaryMouseBtnDown) {

      // User clicked in client area, allow AppBar to move.  We get this 
      // behavior by pretending that the user clicked on the caption area.
      u = HTCAPTION;
   }

   // When the AppBar is docked, the user can resize only one edge.
   // This next section determines which edge the user can resize.
   // To allow resizing, the AppBar window must have the WS_THICKFRAME style.

   // If the AppBar is docked and the hittest code is a resize code...
   if ((GetState() != ABE_FLOAT) && (GetState() != ABE_UNKNOWN) && 
       (HTSIZEFIRST <= u) && (u <= HTSIZELAST)) {

      if (0 == (IsEdgeLeftOrRight(GetState()) ? 
         m_szSizeInc.cx : m_szSizeInc.cy)) {

         // If the width/height size increment is zero, then resizing is NOT 
         // allowed for the edge that the AppBar is docked on.
         u = HTBORDER;  // Pretend that the mouse is not on a resize border
      } else {

         // Resizing IS allowed for the edge that the AppBar is docked on.
         // Get the location of the appbar's client area in screen coordinates.
         CRect rcClient;
         GetClientRect(&rcClient);
         ClientToScreen(rcClient);
         u = HTBORDER;  // Assume that we can't resize

         switch (GetState()) {
            case ABE_LEFT:
               if (point.x > rcClient.right) u = HTRIGHT;
               break;

            case ABE_TOP:
               if (point.y > rcClient.bottom) u = HTBOTTOM;
               break;

            case ABE_RIGHT:
               if (point.x < rcClient.left) u = HTLEFT;
               break;

            case ABE_BOTTOM:
               if (point.y < rcClient.top) u = HTTOP;
               break;
         }
      }
   }

   return(u);  // Return the hittest code
}


/////////////////////////////////////////////////////////////////////////////


LRESULT CAppBar::OnEnterSizeMove(WPARAM wParam, LPARAM lParam) {

   // The user started moving/resizing the AppBar, save its current state.
   m_uStateProposedPrev = GetState();
   return(0);
}


/////////////////////////////////////////////////////////////////////////////


LRESULT CAppBar::OnExitSizeMove(WPARAM wParam, LPARAM lParam) {

   // The user stopped moving/resizing the AppBar, set the new state.

   // Save the new proposed state of the AppBar.
   UINT uStateProposedPrev = m_uStateProposedPrev;

   // Set the proposed state back to unknown.  This causes GetState
   // to return the current state rather than the proposed state.
   m_uStateProposedPrev = ABE_UNKNOWN;

   // Get the location of the window in screen coordinates
   CRect rc;
   GetWindowRect(&rc);

   // If the AppBar's state has changed...
   if (GetState() == uStateProposedPrev) {

      switch (GetState()) {
         case ABE_UNKNOWN:
            break;

         case ABE_LEFT: 
         case ABE_RIGHT:
            // Save the new width of the docked AppBar
            m_abs.m_auDimsDock[m_abs.m_uState] = rc.Width(); 
            break;

         case ABE_TOP: 
         case ABE_BOTTOM:
            // Save the new height of the docked AppBar
            m_abs.m_auDimsDock[m_abs.m_uState] = rc.Height(); 
            break;
      }
   }

   // Always save the new position of the floating AppBar
   if (uStateProposedPrev == ABE_FLOAT)
      m_abs.m_rcFloat = rc; 

   // After setting the dimensions, set the AppBar to the proposed state
   SetState(uStateProposedPrev);
   return(0);
}


/////////////////////////////////////////////////////////////////////////////


LRESULT CAppBar::OnMoving(WPARAM wParam, LPARAM lParam) {

   // We control the moving of the AppBar.  For example, if the mouse moves 
   // close to an edge, we want to dock the AppBar.

   // The lParam contains the window's position proposed by the system
   CRect* prc = (CRect *) lParam;

   // Get the location of the mouse cursor
   CPoint pt(::GetMessagePos());

   // Where should the AppBar be based on the mouse position?
   UINT uStateProposed = CalcProposedState(pt);

   if ((m_uStateProposedPrev != ABE_FLOAT) && (uStateProposed == ABE_FLOAT)) {
      // While moving, the user took us from a docked/autohidden state to 
      // the float state.  We have to calculate a rectangle location so that
      // the mouse cursor stays inside the window.
      GetFloatRect(prc);
      *prc = CRect(pt.x - prc->Width() / 2, pt.y, 
         (pt.x - prc->Width() / 2) + prc->Width(), pt.y + prc->Height());
   }

   // Remember the most-recently proposed state
   m_uStateProposedPrev = uStateProposed;

   // Tell the system where to move the window based on the proposed state
   GetRect(uStateProposed, prc);

   // Tell our derived class that there is a proposed state change
   OnAppBarStateChange(TRUE, uStateProposed);

   return(0);
}


/////////////////////////////////////////////////////////////////////////////


LRESULT CAppBar::OnSizing(WPARAM wParam, LPARAM lParam) {

   // We control the sizing of the AppBar.  For example, if the user re-sizes 
   // an edge, we want to change the size in descrete increments.

   // The lParam contains the window's position proposed by the system
   CRect* prc = (CRect *) lParam;

   // Get the minimum size of the window assuming it has no client area.
   // This is the width/height of the window that must always be present
   CRect rcBorder(0, 0, 0, 0);
   AdjustWindowRectEx(&rcBorder, GetStyle(), FALSE, GetExStyle());

   // We force the window to resize in discrete units set by the m_szSizeInc 
   // member.  From the new, proposed window dimensions passed to us, round 
   // the width/height to the nearest discrete unit.
   int nWidthNew  = ((prc->Width()  - rcBorder.Width())  + m_szSizeInc.cx / 2) / 
      m_szSizeInc.cx * m_szSizeInc.cx + rcBorder.Width();
   int nHeightNew = ((prc->Height() - rcBorder.Height()) + m_szSizeInc.cy / 2) / 
      m_szSizeInc.cy * m_szSizeInc.cy + rcBorder.Height();

   // Adjust the rectangle's dimensions
   switch (wParam) {
      case WMSZ_LEFT:    
         prc->left   = prc->right  - nWidthNew;  
         break;

      case WMSZ_TOP:     
         prc->top    = prc->bottom - nHeightNew; 
         break;

      case WMSZ_RIGHT:   
         prc->right  = prc->left   + nWidthNew;  
         break;

      case WMSZ_BOTTOM:  
         prc->bottom = prc->top    + nHeightNew; 
         break;

      case WMSZ_BOTTOMLEFT:
         prc->bottom = prc->top    + nHeightNew; 
         prc->left   = prc->right  - nWidthNew;  
         break;

      case WMSZ_BOTTOMRIGHT:
         prc->bottom = prc->top    + nHeightNew; 
         prc->right  = prc->left   + nWidthNew;  
         break;

      case WMSZ_TOPLEFT:
         prc->left   = prc->right  - nWidthNew;
         prc->top    = prc->bottom - nHeightNew;
         break;

      case WMSZ_TOPRIGHT:
         prc->top    = prc->bottom - nHeightNew;
         prc->right  = prc->left   + nWidthNew;  
         break;
   }
   return(0);
}


//////////////////////////////// End of File //////////////////////////////////
