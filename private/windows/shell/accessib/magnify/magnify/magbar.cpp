/******************************************************************************
Module name: MagBar.cpp
Copyright (c) 1997-1999 Microsoft Corporation
Written by:  Jeffrey Richter
Purpose:     Shell Run CAppBar-derived class implementation file.
******************************************************************************/

#include "stdafx.h"
#include <malloc.h>
#include <WinReg.h>
#include "..\Mag_Hook\Mag_Hook.h"
#include "Magnify.h"
#include "resource.h"
#include "MagDlg.h"
#include "MagBar.h"
#include "Registry.h"
// The way to include "MultiMon.h" is to include it twice and define 
// COMPILE_MULTIMON_STUBS. a-anilk
#include "MultiMon.h"

#define COMPILE_MULTIMON_STUBS
#include "MultiMon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#ifdef USEONESTRETCHBLT
#include "FastDib.h"
#endif

template<class TYPE> 
TYPE ForceInRange(TYPE tVal, TYPE tMin, TYPE tMax) {
	TYPE t = tVal;
	if (tVal < tMin) t = tMin;
	else if (tVal > tMax) t = tMax;
	return(t);
}

// If we are using VC5 there is no CURSORINFO structure in winuser.h
// We have to define it here.
#ifdef VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT2
typedef struct tagCURSORINFO
{
    DWORD   cbSize;
    DWORD   flags;
    HCURSOR hCursor;
    POINT   ptScreenPos;
} CURSORINFO, *PCURSORINFO, *LPCURSORINFO;
#endif

#define CURSOR_SHOWING     0x00000001

typedef BOOL (WINAPI *_tagGetCursorInfo)(PCURSORINFO pci);
_tagGetCursorInfo g_pGetCursorInfo = NULL;


/////////////////////////////////////////////////////////////////////////////
// Constant global variables


const TCHAR CMagBar::m_szRegSubkey[] = __TEXT("Software\\Microsoft\\Magnify");


/////////////////////////////////////////////////////////////////////////////
// Public member functions

CMagBar::CMagBar(CMagnifyDlg* pwndSettings) : CAppBar(CMagBar::IDD, NULL)
{
	m_bForceHideCursor = FALSE;
	// Construct and save the physical palette
	PLOGPALETTE ppal = (PLOGPALETTE)_alloca(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * 256);   // 256 palette entries.
	ppal->palVersion = 0x300;
	ppal->palNumEntries = 256;
	for (int i = 0; i < 256; i++)
	{
		ppal->palPalEntry[i].peFlags = (BYTE)PC_EXPLICIT;
		ppal->palPalEntry[i].peRed   = (BYTE)i;
		ppal->palPalEntry[i].peGreen = (BYTE)0;
		ppal->palPalEntry[i].peBlue  = (BYTE)0;
	}
	m_hpalPhysical.CreatePalette(ppal);
	m_pwndSettings = pwndSettings;
	m_szMinTracking = CSize(0, 0);
	m_fShowCrossHair = /*FALSE JMC*/TRUE;
	m_fShowZoomRect = FALSE;
	//{{AFX_DATA_INIT(CMagBar)
	//}}AFX_DATA_INIT

	m_bSizingOrMoving = FALSE;
	m_str.LoadString(IDS_MOVEFLOAT);
	m_strD.LoadString(IDS_MOVESTRING);

	// JMC: NOTE: There is no unicode version of GetProcAddress
	g_pGetCursorInfo = (_tagGetCursorInfo)GetProcAddress(GetModuleHandle(__TEXT("user32.dll")), "GetCursorInfo");

#ifdef USEONESTRETCHBLT
	m_hbmOffScreen = NULL;
#endif

	m_dwLastMouseMoveTrack = 0;
	
}


/////////////////////////////////////////////////////////////////////////////
// Internal implementation functions


void CMagBar::HideFloatAdornments(BOOL fHide)
{
	if (fHide)
		ModifyStyle(WS_CAPTION, 0, SWP_DRAWFRAME);
	else
		ModifyStyle(0, WS_CAPTION | WS_SYSMENU, SWP_DRAWFRAME);
}


/////////////////////////////////////////////////////////////////////////////
// Overridable functions


// Tell our derived class that there is a proposed state change
void CMagBar::OnAppBarStateChange (BOOL fProposed, UINT uStateProposed)
{
	// Hide the window adorments when docked.
	HideFloatAdornments((uStateProposed == ABE_FLOAT) ? FALSE : TRUE);
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::DoDataExchange(CDataExchange* pDX)
{
	CAppBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMagBar)
	//}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////


BEGIN_MESSAGE_MAP(CMagBar, CAppBar)
	//{{AFX_MSG_MAP(CMagBar)
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_KEYDOWN()
	ON_WM_DESTROY()
	ON_WM_SETTINGCHANGE()
	ON_WM_SYSCOLORCHANGE()
	ON_COMMAND(ID_APPBAR_COPYTOCLIPBOARD, OnAppbarCopyToClipboard)
	ON_WM_CLOSE()
	ON_WM_SETCURSOR()
	ON_COMMAND(ID_APPBAR_EXIT, OnAppbarExit)
	ON_COMMAND(ID_APPBAR_OPTIONS, OnAppbarOptions)
	ON_COMMAND(ID_APPBAR_HIDE, OnAppbarHide)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	ON_MESSAGE(WM_EVENT_CARETMOVE, OnEventCaretMove)
	ON_MESSAGE(WM_EVENT_FOCUSMOVE, OnEventFocusMove)
	ON_MESSAGE(WM_EVENT_MOUSEMOVE, OnEventMouseMove)
	ON_MESSAGE(WM_EVENT_FORCEMOVE, OnEventForceMove)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMagBar message handlers


LRESULT CMagBar::OnEventXMove(WPARAM wParam, LPARAM lParam, BOOL fShowCrossHair)
{
	// If the location is over the Magnify window, ignore this event.
// JMC: HACK	if (WindowFromPoint(CPoint(wParam)) != this)
	{
//		DrawZoomRect();	// erase old one
		m_ptZoom = CPoint((DWORD)wParam);
		m_fShowCrossHair = fShowCrossHair;
		DoTheZoomIn(NULL, m_fShowCrossHair);
//		DrawZoomRect();	// draw new one
	}
	return(0);
}


LRESULT CMagBar::OnEventCaretMove(WPARAM wParam, LPARAM lParam)
{
	// Don't track if it has been less than .25 seconds since a mouse track
	if (TrackText() && (GetTickCount() - m_dwLastMouseMoveTrack)> 250) OnEventXMove(wParam, lParam, FALSE);
	return(0);
}


LRESULT CMagBar::OnEventFocusMove(WPARAM wParam, LPARAM lParam)
{
	if (TrackFocus() && (GetTickCount() - m_dwLastMouseMoveTrack)> 250) OnEventXMove(wParam, lParam, FALSE);
	return(0);
}


LRESULT CMagBar::OnEventMouseMove(WPARAM wParam, LPARAM lParam)
{
	if (TrackCursor())
	{
		OnEventXMove(wParam, lParam, TRUE);
		m_dwLastMouseMoveTrack = GetTickCount();
	}
	return(0);
}



LRESULT CMagBar::OnEventForceMove(WPARAM wParam, LPARAM lParam)
{
    // Unconditional move...
    // (Third param actually ignored - should remove globally...)
	OnEventXMove(wParam, lParam, FALSE);
	return(0);
}


/////////////////////////////////////////////////////////////////////////////


UINT CMagBar::MagLevel() { return(m_pwndSettings->m_nStationaryMagLevel); }
BOOL CMagBar::TrackText() { return(m_pwndSettings->m_fStationaryTrackText); }
BOOL CMagBar::TrackSecondaryFocus() { return(m_pwndSettings->m_fStationaryTrackSecondaryFocus); }
BOOL CMagBar::TrackCursor() { return(m_pwndSettings->m_fStationaryTrackCursor); }
BOOL CMagBar::TrackFocus() { return(m_pwndSettings->m_fStationaryTrackFocus); }
BOOL CMagBar::InvertColors() { return(m_pwndSettings->m_fStationaryInvertColors); }


/////////////////////////////////////////////////////////////////////////////


BOOL CMagBar::OnInitDialog()
{
	CAppBar::OnInitDialog();

	// TODO: Add extra initialization here
	// Remove our owner window so that we stay visible when the owner is minimized
	::SetWindowLongPtr(m_hWnd, GWLP_HWNDPARENT, NULL);

	// Set the CAppBar class's behavior flags
	m_fdwFlags = ABF_ALLOWANYWHERE;

	// Width has no limits, height sizes in client-area-height increments
	CRect rc;
	GetClientRect(&rc);
	m_szSizeInc.cx = m_szSizeInc.cy = 1;

	// The appbar has a minimum client-area size that is determin ed by the 
	// client area set in the dialog box template.
	m_szMinTracking.cx = rc.Width();
	m_szMinTracking.cy = rc.Height();

	// Setup default state data for the AppBar
	APPBARSTATE abs;

	abs.m_cbSize = sizeof(abs);
	abs.m_uState = ABE_TOP;
	abs.m_fAutohide = FALSE;
	abs.m_fAlwaysOnTop = TRUE;

	// Set the default floating location of the appbar
	GetWindowRect(&abs.m_rcFloat);

	// Make the default width 80% of the workarea width
	CRect rcWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
	abs.m_rcFloat.right = abs.m_rcFloat.left + (rcWorkArea.Width() * 8 / 10);

	// Temporarily turn off window adornments to determine the dimensions
	// of the appbar when docked.
	HideFloatAdornments(TRUE);
	AdjustWindowRectEx(&rc, GetStyle(), FALSE, GetExStyle());
	HideFloatAdornments(FALSE);
	abs.m_auDimsDock[ABE_LEFT]   = rc.Width();
	abs.m_auDimsDock[ABE_TOP]    = rc.Height();
	abs.m_auDimsDock[ABE_RIGHT]  = rc.Width();
	abs.m_auDimsDock[ABE_BOTTOM] = rc.Height();

	// Check the registry to see if we have been used before and if so,
	// reload our persistent settings.
	CRegSettings reg;
	if (reg.OpenSubkey(TRUE, HKEY_CURRENT_USER, m_szRegSubkey) == ERROR_SUCCESS)
	{
		DWORD cbData = sizeof(abs);
		reg.GetBinary(__TEXT("AppBar"), (PBYTE) &abs, &cbData);
		
		// In case these values are out of bound for multimon case. 
		// Reset to old values. 
		if ( MonitorFromRect( &abs.m_rcFloat, MONITOR_DEFAULTTONULL) == NULL )
		{
			abs.m_uState = ABE_TOP;

			// Set the default floating location of the appbar
			GetWindowRect(&abs.m_rcFloat);

			// Make the default width 80% of the workarea width
			CRect rcWorkArea;
			::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
			abs.m_rcFloat.right = abs.m_rcFloat.left + (rcWorkArea.Width() * 8 / 10);

			// Temporarily turn off window adornments to determine the dimensions
			// of the appbar when docked.
			HideFloatAdornments(TRUE);
			AdjustWindowRectEx(&rc, GetStyle(), FALSE, GetExStyle());
			HideFloatAdornments(FALSE);
			abs.m_auDimsDock[ABE_LEFT]   = rc.Width();
			abs.m_auDimsDock[ABE_TOP]    = rc.Height();
			abs.m_auDimsDock[ABE_RIGHT]  = rc.Width();
			abs.m_auDimsDock[ABE_BOTTOM] = rc.Height();
		}

		if (cbData != sizeof(abs))
		{
			// The saved persistent data is a different size than what we 
			// expect. The user probably saved the data using an older version
			// of the AppBar class.  Do any version differencing here...
		}
	}

	// Set the initial state of the appbar.
	SetState(abs);

	// Set the system screen reader flag on so that apps
	// like Word 97 will expose the caret position. Send INI change: a-anilk
	::SystemParametersInfo(SPI_SETSCREENREADER, TRUE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);

	// Set the initial zommed area to be where the mouse cursor is
	GetCursorPos(&m_ptZoom);

	// A system menu gives keyboard functionality for sizing/moving...
	CMenu *hSysMenu = GetSystemMenu(FALSE);
	if ( hSysMenu != NULL )
	{
		hSysMenu->EnableMenuItem( 0, MF_BYPOSITION | MF_GRAYED);
		hSysMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_GRAYED);
		hSysMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_GRAYED);
	}

	
	// Create the memory DC's used for blt'ing the cursor to the screen
	m_dcMem.CreateCompatibleDC(NULL); // Used for blt'ing the cursor
#ifdef USEONESTRETCHBLT
	// Setup offscreen DC
	m_dcOffScreen.CreateCompatibleDC(NULL);
	SetupOffScreenDC();
#else
	m_dcIconBuffer.CreateCompatibleDC(NULL); // Used for blt'ing the cursor
	// Load the dummy 32x32 icon buffer into one of the memory DC's
	m_bmIcon.LoadBitmap(IDB_BITMAP1);
	m_dcIconBuffer.SelectObject(&m_bmIcon);
#endif
	
	
	// Set the refresh timer
	SetTimer(eRefreshTimerId, eRefreshTimerInterval, NULL);

	// Install the ActiveX Accessibility WinEvent Hook. 
	// See the Mag_Hook project for the hook filter function.   
/*JMC: INVESTIGATE*/	InstallEventHook(m_hWnd);  // JMR: Error check this?

//JMC: HACK	m_wndZoomRect.SetLocation(50, 50, 200, 200);
//JMC: HACK	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	return TRUE;  // return TRUE unless you set the focus to a control
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnDestroy()
{
	// TODO: Add your message handler code here
	// Save the current state of the appbar in the registry so that we'll
	// come up in the same state the next time the user runs us.
	CRegSettings reg;
	if (reg.OpenSubkey(FALSE, HKEY_CURRENT_USER, m_szRegSubkey) == ERROR_SUCCESS)
	{
		APPBARSTATE abs;
		abs.m_cbSize = sizeof(abs);
		GetState(&abs);
		// Save the AppBar's state variables to the registry.
		reg.PutBinary(__TEXT("AppBar"), (PBYTE) &abs, sizeof(abs));
	}
	InstallEventHook(NULL);	// remove the event hook
	KillTimer(eRefreshTimerId);

	// Turn off the system-wide screen reader flag.
	// JMR: Remove?:    gfAppExiting=TRUE; No need to send message: a-anilk
	::SystemParametersInfo(SPI_SETSCREENREADER, FALSE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);

#ifdef USEONESTRETCHBLT
	// Destroy the off screen DC before deleting the last bitmap that was in it
	m_dcOffScreen.DeleteDC();
	if(m_hbmOffScreen)
		DeleteObject(m_hbmOffScreen);
#endif


	CAppBar::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// Get the minimum size of the window assuming it has no client area.
	// This is the width/height of the window that must always be present
	CRect rcBorder(0, 0, 0, 0);
	AdjustWindowRectEx(&rcBorder, GetStyle(), FALSE, GetExStyle());
	
	if (GetState() == ABE_FLOAT)
	{
		lpMMI->ptMinTrackSize.x = m_szMinTracking.cx + rcBorder.Width();
		lpMMI->ptMinTrackSize.y = m_szMinTracking.cy + rcBorder.Height();
	}

	// Get the width & height of the screen
	lpMMI->ptMaxTrackSize.x = GetSystemMetrics(SM_CXSCREEN);
	lpMMI->ptMaxTrackSize.y = GetSystemMetrics(SM_CYSCREEN);
	if (GetState() == ABE_FLOAT)
	{
		// When the window is floating...
		// Width can be 100% of screen
		// JMC: Let the height be full screen
//		lpMMI->ptMaxTrackSize.y /= 2; // Height can be half the screen height
	}
	else
	{
		// The appbar can't be more than half the width or height
		// of the screen when docked
		if (!IsEdgeTopOrBottom(GetState()))
			lpMMI->ptMaxTrackSize.x /= 2;
		
		if (!IsEdgeLeftOrRight(GetState()))
			lpMMI->ptMaxTrackSize.y /= 2;
	}

	CAppBar::OnGetMinMaxInfo(lpMMI);
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnSize(UINT nType, int cx, int cy)
{
	CAppBar::OnSize(nType, cx, cy);
	CalcZoomedSize();
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnTimer(UINT nIDEvent)
{
	if(eRefreshTimerId == nIDEvent)
	{
#if 0
		// JMC: HACK:
		m_ptZoom = CPoint(GetMessagePos());
#endif

		// TODO: Add your message handler code here and/or call default
		DrawZoomRect();
		DoTheZoomIn(NULL, m_fShowCrossHair = /*FALSE JMC*/TRUE); 
		m_fShowCrossHair = FALSE;
		DrawZoomRect();
		CAppBar::OnTimer(nIDEvent);
	}
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// This allows the user to task switch to ScreenX and then pan
	// the view around. The shift key makes it go faster, and the
	// control key makes it go all the way to the edge.
	// This also has the keyboard interface for magnification changes.
	// Use Page Up, Page Down, Home, and End
	switch (nChar)
	{
	case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT:
		MoveView(nChar, GetKeyState(VK_SHIFT) & 0x8000, GetKeyState(VK_CONTROL) & 0x8000);
		break;
		
	case VK_PRIOR: case VK_NEXT: case VK_HOME: case VK_END:
		ZoomView(nChar); break;
	}

	CAppBar::OnKeyDown(nChar, nRepCnt, nFlags);
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnSettingChange(UINT uFlags, LPCTSTR lpszSection) {
	CAppBar::OnSettingChange(uFlags, lpszSection);
	
	// TODO: Add your message handler code here
	// Note: WM_SETTINGCHANGE is defined as WM_WININICHAGE in WinUser.h
	//if (gfAppExiting) break;

	// If someone else turns off the system-wide screen reader flag, turn it back on.
	// if ((uFlags == SPI_SETSCREENREADER) && (lpszSection != NULL))
	//	::SystemParametersInfo(SPI_SETSCREENREADER, TRUE, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// We cannot use MFC's Command Update Handlers for the menu items because 
	// they only work when you have a CFrameWnd-derived class.
	CMenu menu;
	menu.LoadMenu(MAKEINTRESOURCE(IDR_STATIONARY));
	menu.GetSubMenu(0)->TrackPopupMenu(0, 
		point.x, point.y, this, NULL);	
}

 
/////////////////////////////////////////////////////////////////////////////
// CAppBar command handlers


void CMagBar::OnSysCommand(UINT nID, LPARAM lParam)
{
	// JMC: This code is defective
#if 0
	if (nID == SC_CLOSE)
	{
		// We have to manually add this so that the dialog box closes when 
		// when the user clicks the close button (which appears in the top, right)
		// corner of the dialog box when it is floating).
		OnCancel();
	}
#endif
	
	CAppBar::OnSysCommand(nID, lParam);
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnCancel()
{
	// JMC: We don't want to do anything for 'OnCancel'.  We'll only let the user hit ALT-F4,
	// or use a close button
//   CAppBar::OnCancel();
}


/////////////////////////////////////////////////////////////////////////////


void CMagBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	DrawZoomRect();
	if(m_bSizingOrMoving)
	{
		CRect rcClient;
		LOGFONT lf;
		HFONT hFont, hOld;
		LOGFONT lfSystem;

		memset( &lf, 0, sizeof(LOGFONT));

		GetClientRect(&rcClient);
		CBrush brush(RGB(255, 255, 128));
		dc.FillRect(&rcClient, &brush);
		dc.SetBkMode(TRANSPARENT);
		
		// Choose the correct font. 
		// lf.lfWeight = FW_BOLD;
		// lstrcpy( lf.lfFaceName , TEXT("MS Shell Dlg"));
		lf.lfHeight = -MulDiv( MagLevel()*2 + 14 , GetDeviceCaps(dc, LOGPIXELSY), 72);
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(lfSystem), (LPVOID) &lfSystem);
		lf.lfCharSet = lfSystem.lfCharSet;

		hFont = ::CreateFontIndirect(&lf);
		hOld = (HFONT) dc.SelectObject(hFont);

		if ( GetState() == ABE_FLOAT )
			dc.ExtTextOut ( 10, 10, 0, &rcClient, m_strD, NULL );
		else
			dc.ExtTextOut ( 10, 10, 0, &rcClient, m_str, NULL );

		dc.SelectObject(hOld);
		DeleteObject(&lf);
	}
	else
		DoTheZoomIn(&dc, m_fShowCrossHair);
	DrawZoomRect();
	// Do not call CAppBar::OnPaint() for painting messages
}


//////////////////////////////// End of File //////////////////////////////////

/************************************************************************
* CalcZoomedSize
* Calculates some globals.  This routine needs to be called any
* time that the size of the app or the zoom factor changes.
************************************************************************/
VOID CMagBar::CalcZoomedSize()
{
	CRect rc;
	GetClientRect(&rc);
	m_cxZoomed = (rc.right  - 1)/ MagLevel() + 1;
	m_cyZoomed = (rc.bottom - 1)/ MagLevel() + 1;
	SIZE sz;
	sz.cx = m_cxZoomed;
	sz.cy = m_cyZoomed;
	// Best to set this same in the MagHook. 
	SetZoomRect(sz);
}

/************************************************************************
* DoTheZoomIn
* Does the actual paint of the zoomed image and the crosshair showing
* the location of the cursor (optional).
* Arguments:
*   HDC hdc - If not NULL, this hdc will be used to paint with.
*             If NULL, a dc for the apps window will be obtained.
*   BOOL fShowLocation - if TRUE, a crosshair will be drawn showing 
*                        the location of the point to zoom in on.               
************************************************************************/
#ifdef USEONESTRETCHBLT
VOID CMagBar::DoTheZoomIn(CDC* pdc, BOOL fShowCrossHair)
{
	if (!pdc)
	{
		// JMC: THIS IS A HACK SO WE DON'T CUE UP TOO MAY ZOOMS
		Invalidate(FALSE);
		return;
	}
	BOOL bCreatedOurOwnClientDC = FALSE;
	if (!pdc)
	{
		// If no DC is specifed, use a DC for our client area
		pdc = new CClientDC(this);
		bCreatedOurOwnClientDC = TRUE;
	}

	// We don't have the correct state information while we are sizing or zooming
	ASSERT(!m_bSizingOrMoving);


	HPALETTE hpalOld = NULL;
	CClientDC dcScreen(NULL);

	BOOL bShowCursor = g_pGetCursorInfo && !(GetAsyncKeyState(VK_SHIFT) & 0x8000) && !m_bForceHideCursor;


	// Calculate the location of the 'zoom' point.  m_ptZoom only contains the point
	// that we would like to zoom.  We must make sure that our zoom rect does not
	// go outside the screen.
	// We know that the width and hight of the zoom rect are going to be m_cxZoomed
	// and m_cyZoomed, so we force ptZoomAdjusted to be >= m_cxZoomed/2 from the left
	// and 'screen width' - m_cxZoomed + m_cxZoomed/2 from the right
	// We must do the same calculation for the 'y'
	// Since m_cxZoomed may be odd, we have to be careful when we look at m_xcZoomed / 2.
	CPoint ptZoomAdjusted;
	int cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	// If CXVIRTUALSCREEN isn't there (95), use CXSCREEN instead...
	if( cxScreen == 0 )
	{
		ptZoomAdjusted.x = ForceInRange((int) m_ptZoom.x, m_cxZoomed / 2, GetSystemMetrics(SM_CXSCREEN) - m_cxZoomed + (m_cxZoomed / 2));
		ptZoomAdjusted.y = ForceInRange((int) m_ptZoom.y, m_cyZoomed / 2, GetSystemMetrics(SM_CYSCREEN) - m_cyZoomed + (m_cyZoomed / 2));
	}
	else
	{
		// Adjust for Multimonitor
		ptZoomAdjusted.x = ForceInRange((int) m_ptZoom.x, m_cxZoomed / 2 + GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_CXVIRTUALSCREEN) + GetSystemMetrics(SM_XVIRTUALSCREEN) - m_cxZoomed + (m_cxZoomed / 2));
		ptZoomAdjusted.y = ForceInRange((int) m_ptZoom.y, m_cyZoomed / 2 + GetSystemMetrics(SM_YVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN) + GetSystemMetrics(SM_YVIRTUALSCREEN) - m_cyZoomed + (m_cyZoomed / 2));
	}	


	CPoint ptZoomRectTopLeft(ptZoomAdjusted.x - m_cxZoomed/2, ptZoomAdjusted.y - m_cyZoomed/2);
	CSize sizeZoomRect(m_cxZoomed, m_cyZoomed);
	// Always move the zoom rect
//JMC: HACK	m_wndZoomRect.SetLocation(ptZoomRectTopLeft.x, ptZoomRectTopLeft.y, ptZoomRectTopLeft.x + sizeZoomRect.cx, ptZoomRectTopLeft.y + sizeZoomRect.cy);


	// Get the cursor bitmaps
	ICONINFO ii;
	ZeroMemory(&ii, sizeof(ii));
	CURSORINFO ci;
	ZeroMemory(&ci, sizeof(ci));
	if(bShowCursor)
	{
		ci.cbSize = sizeof(ci);

		// JMC: HACK - WE NEED TO MAKE SURE THIS IS THE WRITE VERSION OF GetCursorInfo
		// or we have to call GetCursorHack();
		g_pGetCursorInfo(&ci);
//		ci.hCursor = (HCURSOR)GetCursorHack();
//		ci.ptScreenPos = GetCursorPos(); // JMC: Verify GetCursorPos returns right coordinates for what we want

		if(ci.hCursor)
			GetIconInfo(ci.hCursor, &ii);

		if(!ii.hbmMask)
			bShowCursor = FALSE; // We do not have the cursor info
	}

	// Update thd DIB's palette
//	UpdateDIBColorTable(m_dcOffScreen, pdc->GetSafeHdc(), (HPALETTE)GetCurrentObject(pdc->GetSafeHdc(), OBJ_PAL));
//	UpdateDIBColorTable(m_dcOffScreen, pdc->GetSafeHdc(), NULL);

	// Clear background - this prevents 'holes' where a bitblt includes
	// an area of the virtual screen which is not actually on any
	// monitor (can happen when using a staggered multimon setup).
	COLORREF clrrefOld = m_dcOffScreen.SetBkColor( RGB( 0, 0, 255 ) );
	m_dcOffScreen.ExtTextOut( 0, 0, ETO_OPAQUE, 
		CRect(0, 0, sizeZoomRect.cx, sizeZoomRect.cy ), NULL, 0, NULL );
	m_dcOffScreen.SetBkColor( clrrefOld );

	// Make a check here to find if the zoomed 
	// Copy the screen to the offscreen bitmap
 	m_dcOffScreen.BitBlt(0, 0, sizeZoomRect.cx, sizeZoomRect.cy,
 		&dcScreen, ptZoomRectTopLeft.x, ptZoomRectTopLeft.y, CAPTUREBLT | 
		                (InvertColors() ? NOTSRCCOPY : SRCCOPY));

    // Don't let the cursor zoom our own window.
	// Instead, make our own window gray on the off screen bitmap
 	CRect rcZoom(ptZoomRectTopLeft.x, ptZoomRectTopLeft.y, ptZoomRectTopLeft.x + sizeZoomRect.cx, ptZoomRectTopLeft.y + sizeZoomRect.cy);
    CRect rcOverlapped;
	CRect rcWindow;
	GetClientRect(&rcWindow);
	ClientToScreen(&rcWindow);
 	rcOverlapped.IntersectRect(&rcWindow, &rcZoom);
	if(!rcOverlapped.IsRectEmpty())
	{
		COLORREF clrrefMagWindowDefault = RGB(128, 128, 128);
		CRect rcTemp;
		rcTemp.left = (rcOverlapped.left - ptZoomRectTopLeft.x);
		rcTemp.top = (rcOverlapped.top - ptZoomRectTopLeft.y);
		rcTemp.right = (rcOverlapped.right - ptZoomRectTopLeft.x);
		rcTemp.bottom = (rcOverlapped.bottom - ptZoomRectTopLeft.y);

		// Fill in the rest of the area with our gray color
		COLORREF clrrefOld = m_dcOffScreen.SetBkColor(clrrefMagWindowDefault);
		m_dcOffScreen.ExtTextOut(0, 0, ETO_OPAQUE, &rcTemp, NULL, 0, NULL);
		m_dcOffScreen.SetBkColor(clrrefOld);
	}
	
	// Draw the icon on the off screen bitmap
	CRect rcCursor(ci.ptScreenPos.x, ci.ptScreenPos.y, ci.ptScreenPos.x + 32, ci.ptScreenPos.y + 32);
	rcOverlapped.IntersectRect(&rcCursor, &rcZoom);

	if(bShowCursor && !rcOverlapped.IsRectEmpty())
	{
		CPoint ptIconDest;
		ptIconDest.x = ci.ptScreenPos.x - ptZoomRectTopLeft.x - ii.xHotspot;
		ptIconDest.y = ci.ptScreenPos.y - ptZoomRectTopLeft.y - ii.yHotspot;
		// blt the upper half of the mask to the screen
		CBitmap *pbmOrig = m_dcMem.SelectObject(CBitmap::FromHandle(ii.hbmMask));
		m_dcOffScreen.BitBlt(ptIconDest.x, ptIconDest.y, 32, 32, &m_dcMem, 0, 0, SRCAND);
		
		if(ii.hbmColor)
		{
			// If there is a 'color' bitmap, that makes up the second layer
			// of the cursor
			m_dcMem.SelectObject(CBitmap::FromHandle(ii.hbmColor));
			m_dcOffScreen.BitBlt(ptIconDest.x, ptIconDest.y, 32, 32, &m_dcMem, 0, 0, SRCINVERT);
		}
		else
		{
			// If there is not a 'color' bitmap, the rest of the cursor is
			// stored in the bottom half of the 'mask' bitmap
			m_dcOffScreen.BitBlt(ptIconDest.x, ptIconDest.y, 32, 32, &m_dcMem, 0, 32, SRCINVERT);
		}
		m_dcMem.SelectObject(pbmOrig);
	}

	// JMC: Make sure we free the resources we got with GetIconInfo()
	if(ii.hbmMask)
		DeleteObject(ii.hbmMask);
	if(ii.hbmColor)
		DeleteObject(ii.hbmColor);


	// Copy the off screen bitmap to the screen.
	CPalette* ppalOld = pdc->SelectPalette(&m_hpalPhysical, FALSE);
	pdc->RealizePalette();
	pdc->SetStretchBltMode(COLORONCOLOR);

	// Stretch the offscreen bitmap to the screen
	pdc->StretchBlt(0, 0, MagLevel() * sizeZoomRect.cx, MagLevel() * sizeZoomRect.cy,
		&m_dcOffScreen, 0, 0, sizeZoomRect.cx, sizeZoomRect.cy,
		SRCCOPY);

	pdc->SelectPalette(ppalOld, FALSE);

	if(bCreatedOurOwnClientDC) // we need to delete this DC
		delete pdc;
}
#else // Old 5 blt way
VOID CMagBar::DoTheZoomIn(CDC* pdc, BOOL fShowCrossHair)
{
	if (!pdc)
	{
		// JMC: THIS IS A HACK SO WE DON'T CUE UP TOO MAY ZOOMS
		Invalidate(FALSE);
		return;
	}

	// We don't have the correct state information while we are sizing or zooming
	ASSERT(!m_bSizingOrMoving);


	HPALETTE hpalOld = NULL;
	BOOL bCreatedOurOwnClientDC = FALSE;
	CClientDC dcScreen(NULL);

	BOOL bShowCursor = g_pGetCursorInfo && !(GetAsyncKeyState(VK_SHIFT) & 0x8000);


	// Calculate the location of the 'zoom' point.  m_ptZoom only contains the point
	// that we would like to zoom.  We must make sure that our zoom rect does not
	// go outside the screen.
	// We know that the width and hight of the zoom rect are going to be m_cxZoomed
	// and m_cyZoomed, so we force ptZoomAdjusted to be >= m_cxZoomed/2 from the left
	// and 'screen width' - m_cxZoomed + m_cxZoomed/2 from the right
	// We must do the same calculation for the 'y'
	// Since m_cxZoomed may be odd, we have to be careful when we look at m_xcZoomed / 2.
	CPoint ptZoomAdjusted;
	ptZoomAdjusted.x = ForceInRange((int) m_ptZoom.x, m_cxZoomed / 2, GetSystemMetrics(SM_CXSCREEN) - m_cxZoomed + (m_cxZoomed / 2));
	ptZoomAdjusted.y = ForceInRange((int) m_ptZoom.y, m_cyZoomed / 2, GetSystemMetrics(SM_CYSCREEN) - m_cyZoomed + (m_cyZoomed / 2));


	CPoint ptZoomRectTopLeft(ptZoomAdjusted.x - m_cxZoomed/2, ptZoomAdjusted.y - m_cyZoomed/2);
	CSize sizeZoomRect(m_cxZoomed, m_cyZoomed);

	// Always move the zoom rect
//JMC: HACK	m_wndZoomRect.SetLocation(ptZoomRectTopLeft.x, ptZoomRectTopLeft.y, ptZoomRectTopLeft.x + sizeZoomRect.cx, ptZoomRectTopLeft.y + sizeZoomRect.cy);


	// Calculate where the cursor is going to go
	CRect rcIconDest;
	CRect rcIconSrc;
	ICONINFO ii;
	ZeroMemory(&ii, sizeof(ii));
	if(bShowCursor)
	{
		CURSORINFO ci;
		ZeroMemory(&ci, sizeof(ci));
		ci.cbSize = sizeof(ci);

//		g_pGetCursorInfo(&ci);
		ci.hCursor = (HCURSOR)GetCursorHack();

		if(ci.hCursor)
			GetIconInfo(ci.hCursor, &ii);

		if(ii.hbmMask)
		{
			rcIconSrc.left   = m_ptZoom.x - ii.xHotspot;
			rcIconSrc.top    = m_ptZoom.y - ii.yHotspot;
			rcIconSrc.right  = rcIconSrc.left + 32;
			rcIconSrc.bottom = rcIconSrc.top  + 32;
			rcIconDest.left   = (m_ptZoom.x - ptZoomRectTopLeft.x - ii.xHotspot) * MagLevel();
			rcIconDest.top    = (m_ptZoom.y - ptZoomRectTopLeft.y - ii.yHotspot) * MagLevel();
			rcIconDest.right  = rcIconDest.left + 32*MagLevel();
			rcIconDest.bottom = rcIconDest.top  + 32*MagLevel();
		}
		else
			bShowCursor = FALSE; // We do not have the cursor info
	}

	if (!pdc)
	{
		// If no DC is specifed, use a DC for our client area
		pdc = new CClientDC(this);
		bCreatedOurOwnClientDC = TRUE;
	}


	CPalette* ppalOld = pdc->SelectPalette(&m_hpalPhysical, FALSE);
	pdc->RealizePalette();
	pdc->SetStretchBltMode(COLORONCOLOR);

	
	// Don't let the cursor zoom our own window.
	// Instead, make our own window blue
	CRect rcZoom(ptZoomRectTopLeft.x, ptZoomRectTopLeft.y, ptZoomRectTopLeft.x + sizeZoomRect.cx, ptZoomRectTopLeft.y + sizeZoomRect.cy);
	CRect rcOverlapped;
	CRect rcWindow;
	GetClientRect(&rcWindow);
	ClientToScreen(&rcWindow);
	rcOverlapped.IntersectRect(&rcWindow, &rcZoom);
	if(!rcOverlapped.IsRectEmpty())
	{
		COLORREF clrrefMagWindowDefault = RGB(128, 128, 128);
		CRect rcTemp;
		rcTemp.left = (rcOverlapped.left - ptZoomRectTopLeft.x)*MagLevel();
		rcTemp.top = (rcOverlapped.top - ptZoomRectTopLeft.y)*MagLevel();
		rcTemp.right = (rcOverlapped.right - ptZoomRectTopLeft.x)*MagLevel();
		rcTemp.bottom = (rcOverlapped.bottom - ptZoomRectTopLeft.y)*MagLevel();

		// Draw the Cursor only in gray area
		CRgn rgn;
		rgn.CreateRectRgn(rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom);
		pdc->SelectClipRgn(&rgn);
		
		// blt the upper half of the mask to the screen
		// Color the background gray
		COLORREF clrrefOldIcon = m_dcIconBuffer.SetBkColor(clrrefMagWindowDefault);
		m_dcIconBuffer.ExtTextOut(0, 0, ETO_OPAQUE, &CRect(0, 0, 32, 32), NULL, 0, NULL);
		m_dcIconBuffer.SetBkColor(clrrefOldIcon);

		CBitmap *pbmOrig = m_dcMem.SelectObject(CBitmap::FromHandle(ii.hbmMask));
		m_dcIconBuffer.BitBlt(0, 0, 32, 32, &m_dcMem, 0, 0, SRCAND);
		if(ii.hbmColor)
		{
			// If there is a 'color' bitmap, that makes up the second layer
			// of the cursor
			m_dcMem.SelectObject(CBitmap::FromHandle(ii.hbmColor));
			m_dcIconBuffer.BitBlt(0, 0, 32, 32, &m_dcMem, 0, 0, SRCINVERT);
		}
		else
		{
			// If there is not a 'color' bitmap, the rest of the cursor is
			// stored in the bottom half of the 'mask' bitmap
			m_dcIconBuffer.BitBlt(0, 0, 32, 32, &m_dcMem, 0, 32, SRCINVERT);
		}
		m_dcMem.SelectObject(pbmOrig);

		
		pdc->StretchBlt(rcIconDest.left, rcIconDest.top, rcIconDest.Width(), rcIconDest.Height(), &m_dcIconBuffer, 0, 0, 32, 32, SRCCOPY);

		// Fill in the rest of the area with our gray color
		COLORREF clrrefOld = pdc->SetBkColor(clrrefMagWindowDefault);
		pdc->ExcludeClipRect(&rcIconDest); // Exclude the icon we just drew
		pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rcTemp, NULL, 0, NULL);
		pdc->SetBkColor(clrrefOld);

		pdc->SelectClipRgn(NULL);
		pdc->ExcludeClipRect(&rcTemp);
	}
	
	
	if(bShowCursor)
	{
		// blt the upper half of the mask to the screen
		CBitmap *pbmOrig = m_dcMem.SelectObject(CBitmap::FromHandle(ii.hbmMask));
		m_dcIconBuffer.BitBlt(0, 0, 32, 32, &dcScreen, rcIconSrc.left, rcIconSrc.top, CAPTUREBLT | (InvertColors() ? NOTSRCCOPY : SRCCOPY));
		m_dcIconBuffer.BitBlt(0, 0, 32, 32, &m_dcMem, 0, 0, SRCAND);
		
		if(ii.hbmColor)
		{
			// If there is a 'color' bitmap, that makes up the second layer
			// of the cursor
			m_dcMem.SelectObject(CBitmap::FromHandle(ii.hbmColor));
			m_dcIconBuffer.BitBlt(0, 0, 32, 32, &m_dcMem, 0, 0, SRCINVERT);
		}
		else
		{
			// If there is not a 'color' bitmap, the rest of the cursor is
			// stored in the bottom half of the 'mask' bitmap
			m_dcIconBuffer.BitBlt(0, 0, 32, 32, &m_dcMem, 0, 32, SRCINVERT);
		}
		m_dcMem.SelectObject(pbmOrig);

		pdc->StretchBlt(0, 0, MagLevel() * sizeZoomRect.cx, rcIconDest.top,
			&dcScreen, ptZoomRectTopLeft.x, ptZoomRectTopLeft.y, sizeZoomRect.cx, rcIconSrc.top - ptZoomRectTopLeft.y, 
			CAPTUREBLT | (InvertColors() ? NOTSRCCOPY : SRCCOPY));
		
		pdc->StretchBlt(0, rcIconDest.top, rcIconDest.left, rcIconDest.Height(),
			&dcScreen, ptZoomRectTopLeft.x, rcIconSrc.top, rcIconSrc.left - ptZoomRectTopLeft.x, rcIconSrc.Height(), 
			CAPTUREBLT | (InvertColors() ? NOTSRCCOPY : SRCCOPY));
		
		pdc->StretchBlt(rcIconDest.left, rcIconDest.top, rcIconDest.Width(), rcIconDest.Height(), &m_dcIconBuffer, 0, 0, 32, 32, SRCCOPY);

		pdc->StretchBlt(rcIconDest.right, rcIconDest.top, MagLevel() * sizeZoomRect.cx - rcIconDest.right, rcIconDest.Height(),
			&dcScreen, rcIconSrc.right, rcIconSrc.top, ptZoomRectTopLeft.x + sizeZoomRect.cx - rcIconSrc.right, rcIconSrc.Height(), 
			CAPTUREBLT | (InvertColors() ? NOTSRCCOPY : SRCCOPY));

		pdc->StretchBlt(0, rcIconDest.bottom, MagLevel() * sizeZoomRect.cx, MagLevel() * sizeZoomRect.cy - rcIconDest.bottom,
			&dcScreen, ptZoomRectTopLeft.x, rcIconSrc.bottom, sizeZoomRect.cx, ptZoomRectTopLeft.y + sizeZoomRect.cy - rcIconSrc.bottom, 
			CAPTUREBLT | (InvertColors() ? NOTSRCCOPY : SRCCOPY));

//		pdc->ExcludeClipRect(&rcIconDest);

	}
	else
	{
		pdc->StretchBlt(0, 0, MagLevel() * sizeZoomRect.cx, MagLevel() * sizeZoomRect.cy,
			&dcScreen, ptZoomRectTopLeft.x, ptZoomRectTopLeft.y, sizeZoomRect.cx, sizeZoomRect.cy, 
			CAPTUREBLT | (InvertColors() ? NOTSRCCOPY : SRCCOPY));
	}

	// JMC: Make sure we free the resources we got with GetIconInfo()
	if(ii.hbmMask)
		DeleteObject(ii.hbmMask);
	if(ii.hbmColor)
		DeleteObject(ii.hbmColor);

	

#if 0 // JMC: OLD WAY TO DO THIS
	if (fShowCrossHair)
	{
		int CenterX, CenterY;
		int StartX, StartY; // starting position for drawing crosshair
		UINT ModX, ModY, CorrectionX, CorrectionY;
		CRect rc;
		
		GetClientRect(&rc);
		CenterX = (rc.right  / 2) + MagLevel();
		CenterY = (rc.bottom / 2) + MagLevel();
		
		// Move the center of the cross hair if near the edge of the screen
		if (m_ptZoom.x != x) CenterX += ((m_ptZoom.x - x) * MagLevel());
		if (m_ptZoom.y != y) CenterY += ((m_ptZoom.y - y) * MagLevel());
		
		// Find the start position for the horz. line of the crosshair
		StartX = CenterX - (5 * MagLevel());
		StartY = CenterY - MagLevel();
		
		// These weird calculations are to make sure that the crosshair
		// is drawn in alignment with the zoomed in pixels. Don't ask
		// how it works - I just looked at different combinations until
		// I found something that worked. This could all break if we do
		// something that changes how we calculate the zoomed rect or
		// something like that.
		ModX = StartX % MagLevel();
		ModY = StartY % MagLevel();
		CorrectionX = CorrectionY = 0;
		if (ModX)
		{
			if ((MagLevel() - ModX) < ModX)
				CorrectionX = (ModX - MagLevel());
			else
				CorrectionX = ModX;
		}
		if (ModY)
		{
			if ((MagLevel() - ModY) < ModY)
				CorrectionY = (ModY - MagLevel());
			else
				CorrectionY = ModY;
		}
		
		// Mow that we know the correction, apply to start coords
		StartX -= CorrectionX;
		StartY -= CorrectionY;
		pdc->PatBlt(StartX, StartY, 9 * MagLevel(), MagLevel(), DSTINVERT);
		
		// Draw the cursor crosshair vertical line
		StartX = CenterX - MagLevel();
		StartY = CenterY - (5 * MagLevel());
		
		// Correction is same for vertical line
		StartX -= CorrectionX;
		StartY -= CorrectionY;
		pdc->PatBlt(StartX, StartY, MagLevel(), 9 * MagLevel(), DSTINVERT);
	}
#endif

	pdc->SelectPalette(ppalOld, FALSE);

	if(bCreatedOurOwnClientDC) // we need to delete this DC
		delete pdc;
}
#endif

/************************************************************************
* MoveView
* This function moves the current view around.
* Arguments:
*   INT nDirectionCode - Direction to move.  Must be VK_UP, VK_DOWN,
*                        VK_LEFT or VK_RIGHT.
*   BOOL fFast         - TRUE if the move should jump a larger increment.
*                        If FALSE, the move is just one pixel.
*   BOOL fPeg          - If TRUE, the view will be pegged to the screen
*                        boundary in the specified direction.  This overides
*                        the fFast parameter.
************************************************************************/
VOID CMagBar::MoveView(INT nDirectionCode,BOOL fFast,BOOL fPeg)
{
	INT delta;
	
	if (fFast)
		delta = 8;   // Move by 8 pixels at a time instead of 1
	else
		delta = 1;
	int cxScreen = GetSystemMetrics(SM_CXSCREEN);
	int cyScreen = GetSystemMetrics(SM_CYSCREEN);

	switch (nDirectionCode)
	{
	case VK_UP:
		if (fPeg)
			m_ptZoom.y = m_cyZoomed / 2;
		else
			m_ptZoom.y -= delta;
		m_ptZoom.y = ForceInRange((int) m_ptZoom.y, 0, cyScreen);
		break;
		
	case VK_DOWN:
		if (fPeg)
			m_ptZoom.y = cyScreen - (m_cyZoomed / 2);
		else
			m_ptZoom.y += delta;
		m_ptZoom.y = ForceInRange((int) m_ptZoom.y, 0, cyScreen);
		break;
		
	case VK_LEFT:
		if (fPeg)
			m_ptZoom.x = m_cxZoomed / 2;
		else
			m_ptZoom.x -= delta;
		m_ptZoom.x = ForceInRange((int) m_ptZoom.x, 0, cxScreen);
		break;
		
	case VK_RIGHT:
		if (fPeg)
			m_ptZoom.x = cxScreen - (m_cxZoomed / 2);
		else
			m_ptZoom.x += delta;
		m_ptZoom.x = ForceInRange((int) m_ptZoom.x, 0, cxScreen);
		break;
	}
	DrawZoomRect();
	DoTheZoomIn(NULL, /*FALSE JMC*/TRUE);
	DrawZoomRect();
}

/************************************************************************
* DrawZoomRect
* This function draws the tracking rectangle.  The size and shape of
* the rectangle will be proportional to the size and shape of the
* app's client, and will be affected by the zoom factor as well.
************************************************************************/
VOID CMagBar::DrawZoomRect()
{
	CRect rc;
	int x, y;
	
	if (!m_fShowZoomRect) return;
	
	int cxScreen = GetSystemMetrics(SM_CXSCREEN);
	int cyScreen = GetSystemMetrics(SM_CYSCREEN);
	x = ForceInRange((int) m_ptZoom.x, m_cxZoomed / 2, cxScreen - (m_cxZoomed / 2));
	y = ForceInRange((INT) m_ptZoom.y, m_cyZoomed / 2, cyScreen - (m_cyZoomed / 2));
	
	rc.SetRect(x - m_cxZoomed / 2, y - m_cyZoomed / 2, m_cxZoomed, m_cyZoomed);
	rc.InflateRect(1, 1);
	CClientDC dcScreen(NULL);
	dcScreen.PatBlt(rc.left,      rc.top,        rc.right - rc.left,    1,                     DSTINVERT);
	dcScreen.PatBlt(rc.left,      rc.bottom,     1,                     -(rc.bottom - rc.top), DSTINVERT);
	dcScreen.PatBlt(rc.right - 1, rc.top,        1,                     rc.bottom - rc.top,    DSTINVERT);
	dcScreen.PatBlt(rc.right,     rc.bottom - 1, -(rc.right - rc.left), 1,                     DSTINVERT);
}


/************************************************************************
* ZoomView
* This just redraws at the new zoom level, taking a key as input
* Arguments:
*	WPARAM	ZoomChangeCode - how to change the zoom. must be VK_NEXT,
*							 VK_PRIOR,VK_HOME, or VK_END.
************************************************************************/
VOID CMagBar::ZoomView (WPARAM ZoomChangeCode)
{
#if 0
	switch (ZoomChangeCode)
	{
	case VK_NEXT:  m_nZoom++; break;
	case VK_PRIOR: m_nZoom--; break;
	case VK_HOME:  m_nZoom = CMagnifyDlg::nMinZoom; break;
	case VK_END:   m_nZoom = CMagnifyDlg::nMaxZoom; break;
	}

	m_nZoom = ForceInRange(m_nZoom, CMagnifyDlg::nMinZoom, CMagnifyDlg::nMaxZoom);
#endif
	DrawZoomRect();
	CalcZoomedSize();
	DoTheZoomIn(NULL, m_fShowCrossHair);
	DrawZoomRect();
}



VOID CMagBar::CopyToClipboard()
{
	CClientDC dcSrc(this);
	
	if (OpenClipboard())
	{
		CRect rc;
		GetClientRect(&rc);
		// JMC: Don't use a CBitmap because we'll destroy it when we loose scope
		HBITMAP hbm = CreateCompatibleBitmap(dcSrc.GetSafeHdc(), rc.Width(), rc.Height());
		CDC dcDst;
		dcDst.CreateCompatibleDC(&dcSrc);

		// Calculate the dimensions of the bitmap and convert them to 
		// tenths of a millimeter for setting the size with the SetBitmapDimensionEx
		// call.  This allows programs like WinWord to retrieve the bitmap and know 
		// what size to display it as.
		const DWORD dwMM10PERINCH = 254; // Tenths of a millimeter per inch.
		::SetBitmapDimensionEx(hbm,
			(rc.Width()  * dwMM10PERINCH) / (DWORD) dcSrc.GetDeviceCaps(LOGPIXELSX), 
			(rc.Height() * dwMM10PERINCH) / (DWORD) dcSrc.GetDeviceCaps(LOGPIXELSY), 
			NULL);

		CBitmap *pbmOld = dcDst.SelectObject(CBitmap::FromHandle(hbm));
		dcDst.BitBlt(0, 0, rc.Width(), rc.Height(), &dcSrc, rc.left, rc.top, 
			/*InvertColors() ? NOTSRCCOPY : SRCCOPY*/ SRCCOPY); // JMC: Always copy it as it is on screen

		EmptyClipboard();
		SetClipboardData(CF_BITMAP, hbm);
		CloseClipboard();
		dcDst.SelectObject(pbmOld);
	} else ASSERT(FALSE);
}

void CMagBar::OnAppbarCopyToClipboard()
{
	// TODO: Add your command handler code here
	CopyToClipboard();	
}

void CMagBar::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	m_pwndSettings->PostMessage(WM_CLOSE);
	// JMC: Let the options dialog close us.
//	DestroyWindow();	
//	CAppBar::OnClose();
}

// JMC: HACK - Remove this when it is defined correctly in Winuser.h
#define IDC_HAND_TEMPxxx       MAKEINTRESOURCE(32649)

BOOL CMagBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	BOOL fPrimaryMouseBtnDown = 
		(GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) 
		? VK_RBUTTON : VK_LBUTTON) & 0x8000) != 0;
	
	// If they are over the client, or the mouse button is down over the caption
	// (ie: when they are dragging the window by its client area), show the 'hand'
	if(HTCLIENT == nHitTest || (fPrimaryMouseBtnDown && HTCAPTION))
	{
		// JMC: TODO: For NT5/Memphis, we should be able to use USER's new IDC_HAND
        // Try NT5/98 cursor first...
        HCURSOR hCur = LoadCursor(NULL, IDC_HAND_TEMPxxx);
        // If that fails, load our own hand cursor...
        if( ! hCur )
            hCur = AfxGetApp()->LoadCursor(IDC_CUSTOM_HAND);
        SetCursor( hCur );

		return TRUE;
	}
	return CAppBar::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CMagBar::PreTranslateMessage(MSG* pMsg) 
{
	// JMC: HACK TO GET THIS TO WORK WITH MouseHook
	switch(pMsg->message)
	{
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		FakeCursorMove(pMsg->pt);
		break;
	default:
		break;
	}
	return CAppBar::PreTranslateMessage(pMsg);
}


LRESULT CMagBar::OnEnterSizeMove(WPARAM wParam, LPARAM lParam)
{
	Invalidate();
	m_bSizingOrMoving = TRUE;
	return CAppBar::OnEnterSizeMove(wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////


LRESULT CMagBar::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
	m_bSizingOrMoving = FALSE;
#ifdef USEONESTRETCHBLT
	SetupOffScreenDC();
#endif
	return CAppBar::OnExitSizeMove(wParam, lParam);
}

void CMagBar::SetupOffScreenDC()
{
#ifdef USEONESTRETCHBLT
	CalcZoomedSize();
	HDC hdc = ::GetDC(m_hWnd);
	PVOID pBits = NULL;;
//	m_hbmOffScreen = CreateCompatibleDIB(hdc, NULL, m_cxZoomed, m_cyZoomed, &pBits);

	// We don't need to 'Delete' the old bitmap because it is deleted right after
	// our call to SelectObject
	m_hbmOffScreen = CreateCompatibleBitmap(hdc, m_cxZoomed, m_cyZoomed);
	::ReleaseDC(m_hWnd, hdc);
	CBitmap *pbmOld = m_dcOffScreen.SelectObject(CBitmap::FromHandle(m_hbmOffScreen));
	if(pbmOld)
		pbmOld->DeleteObject();
#endif
}

void CMagBar::OnAppbarExit() 
{
	// JMC: This will close us
	m_pwndSettings->PostMessage(WM_CLOSE);
}

void CMagBar::OnAppbarOptions() 
{
	m_pwndSettings->ShowWindow(SW_RESTORE);
	m_pwndSettings->SetForegroundWindow();
	
}

void CMagBar::OnAppbarHide() 
{
	m_pwndSettings->SetStationaryHidden();
}

void CMagBar::OnSysColorChange()
{
	SetupOffScreenDC();
}