/*	File: D:\WACKER\tdll\sidebar.c (Created: 10-Mar-1995)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#include "stdtyp.h"
#include "globals.h"
#include "assert.h"
#include "session.h"
#include <term\res.h>
#include <emu\emu.h>

#define INDENT 3
#define SIDEBAR_CLASS	"sidebar class"
#define LOSHORT(x)	((short)LOWORD(x))
#define HISHORT(x)	((short)HIWORD(x))

static void SB_WM_SIZE(const HWND hwnd, const int cx, const int cy);
LRESULT CALLBACK SidebarButtonProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);

// I know, a static.  Bad news but not really.	Since this is just for
// the minitel I didn't want to screw around with local atoms.  If we
// ever decide that this stuff will be used for more general purposes,
// we can put in the atoms
//
static WNDPROC fpSidebarButtonProc;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateSidebar
 *
 * DESCRIPTION:
 *	Creates a sidebar.	What's a sidebar you say?  Its a bar with buttons
 *	on it running down the left side of the session window.  It is used
 *	only for the minitel emulator and displays buttons specific to that
 *	emulator.
 *
 * ARGUMENTS:
 *	hwndSession - session window handle.
 *
 * RETURNS:
 *	sidebar window handle.
 *
 * AUTHOR: Mike Ward, 10-Mar-1995
 */
HWND CreateSidebar(const HWND hwndSession, const HSESSION hSession)
	{
	UINT i;
	TCHAR ach[100];
	HWND hwnd;
	HWND hwndSideBar;
	SIZE sz;
	LONG cx = 0;
	LONG cy = 0;
	HDC  hdc;
	HGDIOBJ hFont;

	// Figure out the longest string to size things by
	//
	hdc = GetDC(hwndSession);
	hFont = GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(hdc, hFont);

	for (i = 0 ; i < 9 ; ++i)
		{
		LoadString(glblQueryDllHinst(), IDS_SIDEBAR_INDEX+i, ach,
			sizeof(ach));

		GetTextExtentPoint32(hdc, ach, lstrlen(ach), &sz);
		cx = max(sz.cx, cx);
		cy = max(sz.cy, cy);
		}

	ReleaseDC(hwndSession, hdc);

	// Good button size is 1.5 times text height.  Also add padding
	// for horizontal diretion.
	//
	cx += GetSystemMetrics(SM_CXBORDER) * 10;
	cy = (LONG)(cy * 1.5);

	// Create sidebar window with proper x dimension
	//
	hwndSideBar = CreateWindowEx(WS_EX_CLIENTEDGE, SIDEBAR_CLASS, 0,
		WS_CHILD, 0, 0, cx+2+(2*INDENT), 100, hwndSession,
		(HMENU)IDC_SIDEBAR_WIN, glblQueryDllHinst(), hSession);

	// Important: Set fpSidebarButtonProc to zero here.  It may have
	// been initialized by an earlier instance.  We could set it to
	// zero in the WM_DESTROY but then I'ld have to keep a list of
	// button window handles which I don't want to do here.
	//
	fpSidebarButtonProc = 0;

	// Create buttons for sidebar and postion with text
	//
	for (i = 0 ; i < 9 ; ++i)
		{
		LoadString(glblQueryDllHinst(), IDS_SIDEBAR_INDEX+i, ach, sizeof(ach));

		hwnd = CreateWindowEx(0, "BUTTON", ach,
			WS_CHILD | WS_VISIBLE | BS_LEFT | BS_PUSHBUTTON,
			0, 0, cx, cy, hwndSideBar,
			(HMENU)(IDM_MINITEL_INDEX+i), glblQueryDllHinst(), 0);

		SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0);
		MoveWindow(hwnd, INDENT, ((int)i*(cy+INDENT))+INDENT, cx, cy, FALSE);

		// Need to subclass the buttons so that they don't get the focus
		// rectangle left on them.	This will require an atom to so that
		// we can get the original button proc.
		//
		if (fpSidebarButtonProc == 0)
			fpSidebarButtonProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);

		if (fpSidebarButtonProc)
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)SidebarButtonProc);
		}

	return hwndSideBar;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SidebarProc
 *
 * DESCRIPTION:
 *	Sidebar window proc.
 *
 * AUTHOR: Mike Ward, 10-Mar-1995
 */
LRESULT CALLBACK SidebarProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	HSESSION hSession;

	switch (uMsg)
		{
	case WM_CREATE:
		// Save session handle for later
		//
		SetWindowLongPtr(hwnd, GWLP_USERDATA,
			(LONG_PTR)((LPCREATESTRUCT)lPar)->lpCreateParams);

		return 0;

	case WM_COMMAND:
		switch (LOWORD(wPar))
			{
			case IDM_MINITEL_INDEX:
			case IDM_MINITEL_CANCEL:
			case IDM_MINITEL_PREVIOUS:
			case IDM_MINITEL_REPEAT:
			case IDM_MINITEL_GUIDE:
			case IDM_MINITEL_CORRECT:
			case IDM_MINITEL_NEXT:
			case IDM_MINITEL_SEND:
			case IDM_MINITEL_CONFIN:
				if (HIWORD(wPar) == BN_CLICKED)
					{
					hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

					emuMinitelSendKey(sessQueryEmuHdl(hSession),
						LOWORD(wPar));

					SetFocus(sessQueryHwnd(hSession));
					}

				break;

			default:
				break;
			}
		break;

	case WM_SIZE:
		SB_WM_SIZE(hwnd, LOSHORT(lPar), HISHORT(lPar));
		return 0;

	default:
		break;
		}

	return DefWindowProc(hwnd, uMsg, wPar, lPar);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SB_WM_SIZE
 *
 * DESCRIPTION:
 *	Sizing logic for sidebar.  This routine sizes the sidebar vertically
 *	to the session window.
 *
 * ARGUMENTS:
 *	hwnd		- sidebar window handle
 *	cx			- x size of window
 *	cy			- y size of window
 *
 * RETURNS:
 *	void
 *
 * AUTHOR: Mike Ward, 10-Mar-1995
 */
static void SB_WM_SIZE(const HWND hwnd, const int cx, const int cy)
	{
	RECT rc;
	RECT rcSB;
	RECT rcTmp;
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(GetParent(hwnd),
							  GWLP_USERDATA);
	const HWND hwndToolbar = sessQueryHwndToolbar(hSession);
	const HWND hwndStatusbar = sessQueryHwndStatusbar(hSession);

	if (cx != 0 || cy != 0)
		return;

	GetWindowRect(hwnd, &rcSB);
	GetClientRect(GetParent(hwnd), &rc);

	if (IsWindow(hwndToolbar) && IsWindowVisible(hwndToolbar))
		{
		GetWindowRect(hwndToolbar, &rcTmp);
		rc.top += (rcTmp.bottom - rcTmp.top);
		}

	if (IsWindow(hwndStatusbar) && IsWindowVisible(hwndStatusbar))
		{
		GetWindowRect(hwndStatusbar, &rcTmp);
		rc.bottom -= (rcTmp.bottom - rcTmp.top);
		rc.bottom += 2 * GetSystemMetrics(SM_CYBORDER);
		}

	MoveWindow(hwnd, rc.left, rc.top, rcSB.right-rcSB.left, rc.bottom-rc.top,
		TRUE);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	RegisterSidebarClass
 *
 * DESCRIPTION:
 *	Registers the sidebar window class used for Minitel.
 *
 * ARGUMENTS:
 *	hInstance	- instance handle of program.
 *
 * RETURNS:
 *	TRUE/FALSE
 *
 * AUTHOR: Mike Ward, 10-Mar-1995
 */
int RegisterSidebarClass(const HINSTANCE hInstance)
	{
	WNDCLASS  wc;

	wc.style		 = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	 = (WNDPROC)SidebarProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = sizeof(long);
	wc.hInstance	 = hInstance;
	wc.hIcon		 = 0;
	wc.hCursor		 = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = SIDEBAR_CLASS;

	if (RegisterClass(&wc) == FALSE)
		{
		assert(FALSE);
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SidebarButtonProc
 *
 * DESCRIPTION:
 *	Don't you just love windows.  I don't want the focus to ever remain
 *	on the Sidebar but to do this with standard buttons I have to subclass
 *	them.
 *
 * AUTHOR: Mike Ward, 13-Mar-1995
 */
LRESULT CALLBACK SidebarButtonProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	HSESSION hSession;
	POINT pt;
	RECT  rc;

	switch (uMsg)
		{
	case WM_LBUTTONUP:
		hSession = (HSESSION)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);

		// Well, its never simple.  If the user clicks on the button, we don't
		// reset focus else our sidebar never receives the notification.  If
		// the user holds the button down however and drags off the button
		// and then lets up (button up not on the button) then we need to
		// set focus. - mrw.
		//
		pt.x = LOWORD(lPar);
		pt.y = HIWORD(lPar);
		GetClientRect(hwnd, &rc);

		if (!PtInRect(&rc, pt))
			SetFocus(sessQueryHwnd(hSession));

		break;

	default:
		break;
		}

	return CallWindowProc(fpSidebarButtonProc, hwnd, uMsg, wPar, lPar);
	}
