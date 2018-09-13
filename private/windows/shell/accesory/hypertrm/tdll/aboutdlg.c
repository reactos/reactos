/*	File: D:\WACKER\tdll\aboutdlg.c (Created: 04-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 4/28/99 11:58a $
 */

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>
#include <term\res.h>

#include "banner.h"
#include "globals.h"
#include "features.h"
#include "misc.h"
#include "upgrddlg.h"
#include "registry.h"

#if defined(INCL_PRIVATE_EDITION_BANNER)
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
LRESULT CALLBACK BannerAboutProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);

DWORD CALLBACK EditStreamCallback(DWORD dwCookie, LPBYTE pbBuff,
    LONG cb, LONG *pcb);
#endif

static const TCHAR g_achHyperTerminalRegKey[] =
    TEXT("SOFTWARE\\Hilgraeve Inc\\HyperTerminal PE\\3.0");
static const TCHAR g_achSerial[] = TEXT("Registered");

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	AboutDlg
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *	hwnd	- session window handle
 *
 * RETURNS:
 *	void
 *
 */
void AboutDlg(HWND hwndSession)
	{
    #if defined(INCL_PRIVATE_EDITION_BANNER)
    DialogBox(glblQueryDllHinst(), MAKEINTRESOURCE(IDD_ABOUT_DLG),
        hwndSession, AboutDlgProc);

    #else
	TCHAR	ach1[100];
	HWND	hwndAbout;

	LoadString(glblQueryDllHinst(), IDS_GNRL_APPNAME, ach1, sizeof(ach1) / sizeof(TCHAR));

	hwndAbout = CreateWindow(BANNER_DISPLAY_CLASS,
								ach1,
								BANNER_WINDOW_STYLE,
								0,
								0,
								100,
								100,
								hwndSession,
								NULL,
								glblQueryDllHinst(),
								NULL);

	UpdateWindow(hwndAbout);
    #endif
	return;
	}

// ----------------- Private edition about dialog routines ------------------
//
#if defined(INCL_PRIVATE_EDITION_BANNER)
#define BANNER_ABOUT_CLASS "Banner About Class"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	AboutDlgProc
 *
 * DESCRIPTION:
 *
 */
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
    {
    #define IDPB_UPGRADE 100

    HWND hwndAbout;

    switch (wMsg)
        {
    case WM_INITDIALOG:
	    hwndAbout = CreateWindow(BANNER_ABOUT_CLASS,
								NULL,
								WS_CHILD | WS_VISIBLE,
								0,
								0,
								100,
								100,
								hDlg,
								NULL,
								glblQueryDllHinst(),
								NULL);
        break;

    case WM_COMMAND:
        switch (wPar)
            {
        case IDOK:
        case IDCANCEL:
			EndDialog(hDlg, TRUE);
            break;

        case IDPB_UPGRADE:
            DoUpgradeDialog(hDlg);
            break;

        default:
            break;
            }
        break;

    default:
        return FALSE;
        }

    return TRUE;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	bannerRegisterClass
 *
 * DESCRIPTION:
 *	This function registers the window class for the banner window.
 *
 * ARGUEMENTS:
 *	The task instance handle.
 *
 * RETURNS:
 * The usual TRUE/FALSE from a registration function.
 *
 */
BOOL RegisterBannerAboutClass(HANDLE hInstance)
	{
	ATOM bRet = FALSE;
	WNDCLASS wnd;

	wnd.style			= CS_HREDRAW | CS_VREDRAW;
	wnd.lpfnWndProc 	= BannerAboutProc;
	wnd.cbClsExtra		= 0;
	wnd.cbWndExtra		= sizeof(HANDLE);
	wnd.hInstance		= hInstance;
	wnd.hIcon			= extLoadIcon(MAKEINTRESOURCE(IDI_PROG));
	wnd.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wnd.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wnd.lpszMenuName	= NULL;
	wnd.lpszClassName	= BANNER_ABOUT_CLASS;

	bRet = RegisterClass(&wnd);

	return bRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	AboutDlgProc
 *
 * DESCRIPTION:
 *  Pops up the about dialog.  In the private edition, this is an actual
 *  dialog of some complexity.
 *
 */
LRESULT CALLBACK BannerAboutProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
    {
	RECT	    rc;
	HBITMAP	    hBitmap = (HBITMAP)0;
	BITMAP	    bm;
	INT 	    x, y, cx, cy;
   	HDC			hDC;
   	PAINTSTRUCT ps;
//#if !defined(USE_PRIVATE_EDITION_3_BANNER)
   	LOGFONT 	lf;
   	HFONT		hFont;
    TCHAR atchSerialNumber[MAX_PATH * 2];
    DWORD dwSize = sizeof(atchSerialNumber);
//#endif

    switch (uMsg)
        {
    case WM_CREATE:
		//mpt:03-12-98 Changed the bitmap and avi to use system colors
		//hBitmap = LoadBitmap(glblQueryDllHinst(), MAKEINTRESOURCE(IDD_BM_BANNER));
		hBitmap = (HBITMAP)LoadImage(glblQueryDllHinst(),
			MAKEINTRESOURCE(IDD_BM_BANNER),
			IMAGE_BITMAP,
			0,
			0,
			LR_CREATEDIBSECTION | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);

		SetWindowLongPtr(hwnd, 0, (LONG_PTR)hBitmap);

    	GetObject(hBitmap, sizeof(BITMAP), (LPTSTR)&bm);

    	SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
    	AdjustWindowRect(&rc, WS_CHILD | WS_VISIBLE, FALSE);

    	cx = rc.right - rc.left;
    	cy = rc.bottom - rc.top;

        GetClientRect(GetParent(hwnd), &rc);

    	x = (rc.right - cx) / 2;
    	y = (rc.bottom - cy) / 3;

    	MoveWindow(hwnd, x, y, cx, cy, TRUE);

        #if defined(INCL_SPINNING_GLOBE)
        // Create an animation control and play spinning globe.
        //
            {
            HWND    hwndAnimate;
			//mpt:03-12-98 Changed the bitmap and avi to use system colors
            hwndAnimate = Animate_Create(hwnd, 100,
                WS_VISIBLE | WS_CHILD | ACS_TRANSPARENT,
                glblQueryDllHinst());

            MoveWindow(hwndAnimate, 177, 37, 118, 101, TRUE);
            Animate_Open(hwndAnimate, MAKEINTRESOURCE(IDR_GLOBE_AVI));
            Animate_Play(hwndAnimate, 0, -1, 1);
            }
        #endif
        break;

    case WM_PAINT:
    	hDC = BeginPaint(hwnd, &ps);
    	hBitmap = (HBITMAP)GetWindowLongPtr(hwnd, 0);

    	if (hBitmap)
    		utilDrawBitmap((HWND)0, hDC, hBitmap, 0, 0);

        // In the HTPE 3 banner, the version # and lot # are now in the
        // lower left corner of the bitmap. - cab:11/29/96
        //
//    #if !defined(USE_PRIVATE_EDITION_3_BANNER)
    	// Here's a mean trick.  The HwndFrame guy doesn't get set until
    	// long after the banner goes up.  Since we don't want the version
    	// number on the opening banner but do want it in the about portion
    	// this works. - mrw:3/17/95
    	//
    	if (glblQueryHwndFrame())
    		{
    		// Draw in the version number
    		//
            if ( regQueryValue(HKEY_CURRENT_USER,
                    g_achHyperTerminalRegKey,
                    g_achSerial,
                    atchSerialNumber,
                    &dwSize) == 0 )
                {

   		        memset(&lf, 0, sizeof(LOGFONT));

    		    lf.lfHeight = 14;
    		    lf.lfCharSet = ANSI_CHARSET;
    		    strcpy(lf.lfFaceName, "Arial");

    		    hFont = CreateFontIndirect(&lf);

    		    if (hFont)
    			    {
    			    hFont = SelectObject(hDC, hFont);
    			    SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
    			    TextOut(hDC, 15, 12, atchSerialNumber, strlen(atchSerialNumber));
    			    DeleteObject(SelectObject(hDC, hFont));
    			    }
                }
    		}
//    #endif

    	EndPaint(hwnd, &ps);
        break;

    case WM_LBUTTONDOWN:
        DoUpgradeDialog(hwnd);
        break;

    default:
        break;
        }

	return DefWindowProc(hwnd, uMsg, wPar, lPar);
    }

#endif //INCL_PRIVATE_EDITION_BANNER

