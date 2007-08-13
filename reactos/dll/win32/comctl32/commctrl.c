/*
 * Common controls functions
 *
 * Copyright 1997 Dimitrie O. Paun
 * Copyright 1998,2000 Eric Kohl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 * 
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Oct. 21, 2002, by Christian Neumair.
 *
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 *
 * TODO
 *   -- implement GetMUILanguage + InitMUILanguage
 *   -- finish NOTES for MenuHelp, GetEffectiveClientRect and GetStatusTextW
 *   -- FIXMEs + BUGS (search for them)
 *
 * Control Classes
 *   -- ICC_ANIMATE_CLASS
 *   -- ICC_BAR_CLASSES
 *   -- ICC_COOL_CLASSES
 *   -- ICC_DATE_CLASSES
 *   -- ICC_HOTKEY_CLASS
 *   -- ICC_INTERNET_CLASSES
 *   -- ICC_LINK_CLASS
 *   -- ICC_LISTVIEW_CLASSES
 *   -- ICC_NATIVEFNTCTL_CLASS
 *   -- ICC_PAGESCROLLER_CLASS
 *   -- ICC_PROGRESS_CLASS
 *   -- ICC_STANDARD_CLASSES (not yet implemented)
 *   -- ICC_TAB_CLASSES
 *   -- ICC_TREEVIEW_CLASSES
 *   -- ICC_UPDOWN_CLASS
 *   -- ICC_USEREX_CLASSES
 *   -- ICC_WIN95_CLASSES
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "winerror.h"
#include "winreg.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

LRESULT WINAPI COMCTL32_SubclassProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LPWSTR  COMCTL32_wSubclass = NULL;
HMODULE COMCTL32_hModule = 0;
LANGID  COMCTL32_uiLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
HBRUSH  COMCTL32_hPattern55AABrush = NULL;
COMCTL32_SysColor  comctl32_color;

static HBITMAP COMCTL32_hPattern55AABitmap = NULL;

static const WORD wPattern55AA[] =
{
    0x5555, 0xaaaa, 0x5555, 0xaaaa,
    0x5555, 0xaaaa, 0x5555, 0xaaaa
};

static const WCHAR strCC32SubclassInfo[] = {
    'C','C','3','2','S','u','b','c','l','a','s','s','I','n','f','o',0
};

/***********************************************************************
 * DllMain [Internal]
 *
 * Initializes the internal 'COMCTL32.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the 'dlls' instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserverd, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);

            COMCTL32_hModule = (HMODULE)hinstDLL;

            /* add global subclassing atom (used by 'tooltip' and 'updown') */
            COMCTL32_wSubclass = (LPWSTR)(DWORD_PTR)GlobalAddAtomW (strCC32SubclassInfo);
            TRACE("Subclassing atom added: %p\n", COMCTL32_wSubclass);

            /* create local pattern brush */
            COMCTL32_hPattern55AABitmap = CreateBitmap (8, 8, 1, 1, wPattern55AA);
            COMCTL32_hPattern55AABrush = CreatePatternBrush (COMCTL32_hPattern55AABitmap);

	    /* Get all the colors at DLL load */
	    COMCTL32_RefreshSysColors();

            /* like comctl32 5.82+ register all the common control classes */
            ANIMATE_Register ();
            COMBOEX_Register ();
            DATETIME_Register ();
            FLATSB_Register ();
            HEADER_Register ();
            HOTKEY_Register ();
            IPADDRESS_Register ();
            LISTVIEW_Register ();
            MONTHCAL_Register ();
            NATIVEFONT_Register ();
            PAGER_Register ();
            PROGRESS_Register ();
            REBAR_Register ();
            STATUS_Register ();
            SYSLINK_Register ();
            TAB_Register ();
            TOOLBAR_Register ();
            TOOLTIPS_Register ();
            TRACKBAR_Register ();
            TREEVIEW_Register ();
            UPDOWN_Register ();

            /* subclass user32 controls */
            THEMING_Initialize ();
            break;

	case DLL_PROCESS_DETACH:
            /* clean up subclassing */ 
            THEMING_Uninitialize();

            /* unregister all common control classes */
            ANIMATE_Unregister ();
            COMBOEX_Unregister ();
            DATETIME_Unregister ();
            FLATSB_Unregister ();
            HEADER_Unregister ();
            HOTKEY_Unregister ();
            IPADDRESS_Unregister ();
            LISTVIEW_Unregister ();
            MONTHCAL_Unregister ();
            NATIVEFONT_Unregister ();
            PAGER_Unregister ();
            PROGRESS_Unregister ();
            REBAR_Unregister ();
            STATUS_Unregister ();
            SYSLINK_Unregister ();
            TAB_Unregister ();
            TOOLBAR_Unregister ();
            TOOLTIPS_Unregister ();
            TRACKBAR_Unregister ();
            TREEVIEW_Unregister ();
            UPDOWN_Unregister ();

            /* delete local pattern brush */
            DeleteObject (COMCTL32_hPattern55AABrush);
            COMCTL32_hPattern55AABrush = NULL;
            DeleteObject (COMCTL32_hPattern55AABitmap);
            COMCTL32_hPattern55AABitmap = NULL;

            /* delete global subclassing atom */
            GlobalDeleteAtom (LOWORD(COMCTL32_wSubclass));
            TRACE("Subclassing atom deleted: %p\n", COMCTL32_wSubclass);
            COMCTL32_wSubclass = NULL;
            break;
    }

    return TRUE;
}


/***********************************************************************
 * MenuHelp [COMCTL32.2]
 *
 * Handles the setting of status bar help messages when the user
 * selects menu items.
 *
 * PARAMS
 *     uMsg       [I] message (WM_MENUSELECT) (see NOTES)
 *     wParam     [I] wParam of the message uMsg
 *     lParam     [I] lParam of the message uMsg
 *     hMainMenu  [I] handle to the application's main menu
 *     hInst      [I] handle to the module that contains string resources
 *     hwndStatus [I] handle to the status bar window
 *     lpwIDs     [I] pointer to an array of integers (see NOTES)
 *
 * RETURNS
 *     No return value
 *
 * NOTES
 *     The official documentation is incomplete!
 *     This is the correct documentation:
 *
 *     uMsg:
 *     MenuHelp() does NOT handle WM_COMMAND messages! It only handles
 *     WM_MENUSELECT messages.
 *
 *     lpwIDs:
 *     (will be written ...)
 */

VOID WINAPI
MenuHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, HMENU hMainMenu,
	  HINSTANCE hInst, HWND hwndStatus, UINT* lpwIDs)
{
    UINT uMenuID = 0;

    if (!IsWindow (hwndStatus))
	return;

    switch (uMsg) {
	case WM_MENUSELECT:
	    TRACE("WM_MENUSELECT wParam=0x%lX lParam=0x%lX\n",
		   wParam, lParam);

            if ((HIWORD(wParam) == 0xFFFF) && (lParam == 0)) {
                /* menu was closed */
		TRACE("menu was closed!\n");
                SendMessageW (hwndStatus, SB_SIMPLE, FALSE, 0);
            }
	    else {
		/* menu item was selected */
		if (HIWORD(wParam) & MF_POPUP)
		    uMenuID = (UINT)*(lpwIDs+1);
		else
		    uMenuID = (UINT)LOWORD(wParam);
		TRACE("uMenuID = %u\n", uMenuID);

		if (uMenuID) {
		    WCHAR szText[256];

		    if (!LoadStringW (hInst, uMenuID, szText, sizeof(szText)/sizeof(szText[0])))
			szText[0] = '\0';

		    SendMessageW (hwndStatus, SB_SETTEXTW,
				    255 | SBT_NOBORDERS, (LPARAM)szText);
		    SendMessageW (hwndStatus, SB_SIMPLE, TRUE, 0);
		}
	    }
	    break;

        case WM_COMMAND :
	    TRACE("WM_COMMAND wParam=0x%lX lParam=0x%lX\n",
		   wParam, lParam);
	    /* WM_COMMAND is not invalid since it is documented
	     * in the windows api reference. So don't output
             * any FIXME for WM_COMMAND
             */
	    WARN("We don't care about the WM_COMMAND\n");
	    break;

	default:
	    FIXME("Invalid Message 0x%x!\n", uMsg);
	    break;
    }
}


/***********************************************************************
 * ShowHideMenuCtl [COMCTL32.3]
 *
 * Shows or hides controls and updates the corresponding menu item.
 *
 * PARAMS
 *     hwnd   [I] handle to the client window.
 *     uFlags [I] menu command id.
 *     lpInfo [I] pointer to an array of integers. (See NOTES.)
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     The official documentation is incomplete!
 *     This is the correct documentation:
 *
 *     hwnd
 *     Handle to the window that contains the menu and controls.
 *
 *     uFlags
 *     Identifier of the menu item to receive or lose a check mark.
 *
 *     lpInfo
 *     The array of integers contains pairs of values. BOTH values of
 *     the first pair must be the handles to the application's main menu.
 *     Each subsequent pair consists of a menu id and control id.
 */

BOOL WINAPI
ShowHideMenuCtl (HWND hwnd, UINT_PTR uFlags, LPINT lpInfo)
{
    LPINT lpMenuId;

    TRACE("%p, %lx, %p\n", hwnd, uFlags, lpInfo);

    if (lpInfo == NULL)
	return FALSE;

    if (!(lpInfo[0]) || !(lpInfo[1]))
	return FALSE;

    /* search for control */
    lpMenuId = &lpInfo[2];
    while (*lpMenuId != uFlags)
	lpMenuId += 2;

    if (GetMenuState ((HMENU)(DWORD_PTR)lpInfo[1], uFlags, MF_BYCOMMAND) & MFS_CHECKED) {
	/* uncheck menu item */
	CheckMenuItem ((HMENU)(DWORD_PTR)lpInfo[0], *lpMenuId, MF_BYCOMMAND | MF_UNCHECKED);

	/* hide control */
	lpMenuId++;
	SetWindowPos (GetDlgItem (hwnd, *lpMenuId), 0, 0, 0, 0, 0,
			SWP_HIDEWINDOW);
    }
    else {
	/* check menu item */
	CheckMenuItem ((HMENU)(DWORD_PTR)lpInfo[0], *lpMenuId, MF_BYCOMMAND | MF_CHECKED);

	/* show control */
	lpMenuId++;
	SetWindowPos (GetDlgItem (hwnd, *lpMenuId), 0, 0, 0, 0, 0,
			SWP_SHOWWINDOW);
    }

    return TRUE;
}


/***********************************************************************
 * GetEffectiveClientRect [COMCTL32.4]
 *
 * Calculates the coordinates of a rectangle in the client area.
 *
 * PARAMS
 *     hwnd   [I] handle to the client window.
 *     lpRect [O] pointer to the rectangle of the client window
 *     lpInfo [I] pointer to an array of integers (see NOTES)
 *
 * RETURNS
 *     No return value.
 *
 * NOTES
 *     The official documentation is incomplete!
 *     This is the correct documentation:
 *
 *     lpInfo
 *     (will be written ...)
 */

VOID WINAPI
GetEffectiveClientRect (HWND hwnd, LPRECT lpRect, const INT *lpInfo)
{
    RECT rcCtrl;
    const INT *lpRun;
    HWND hwndCtrl;

    TRACE("(%p %p %p)\n",
	   hwnd, lpRect, lpInfo);

    GetClientRect (hwnd, lpRect);
    lpRun = lpInfo;

    do {
	lpRun += 2;
	if (*lpRun == 0)
	    return;
	lpRun++;
	hwndCtrl = GetDlgItem (hwnd, *lpRun);
	if (GetWindowLongW (hwndCtrl, GWL_STYLE) & WS_VISIBLE) {
	    TRACE("control id 0x%x\n", *lpRun);
	    GetWindowRect (hwndCtrl, &rcCtrl);
	    MapWindowPoints (NULL, hwnd, (LPPOINT)&rcCtrl, 2);
	    SubtractRect (lpRect, lpRect, &rcCtrl);
	}
	lpRun++;
    } while (*lpRun);
}


/***********************************************************************
 * DrawStatusTextW [COMCTL32.@]
 *
 * Draws text with borders, like in a status bar.
 *
 * PARAMS
 *     hdc   [I] handle to the window's display context
 *     lprc  [I] pointer to a rectangle
 *     text  [I] pointer to the text
 *     style [I] drawing style
 *
 * RETURNS
 *     No return value.
 *
 * NOTES
 *     The style variable can have one of the following values:
 *     (will be written ...)
 */

void WINAPI DrawStatusTextW (HDC hdc, LPCRECT lprc, LPCWSTR text, UINT style)
{
    RECT r = *lprc;
    UINT border = BDR_SUNKENOUTER;

    if (style & SBT_POPOUT)
        border = BDR_RAISEDOUTER;
    else if (style & SBT_NOBORDERS)
        border = 0;

    DrawEdge (hdc, &r, border, BF_RECT|BF_ADJUST);

    /* now draw text */
    if (text) {
        int oldbkmode = SetBkMode (hdc, TRANSPARENT);
        UINT align = DT_LEFT;
        if (*text == '\t') {
	    text++;
	    align = DT_CENTER;
            if (*text == '\t') {
	        text++;
	        align = DT_RIGHT;
	    }
        }
        r.left += 3;
        if (style & SBT_RTLREADING)
	    FIXME("Unsupported RTL style!\n");
        DrawTextW (hdc, text, -1, &r, align|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
	SetBkMode(hdc, oldbkmode);
    }
}


/***********************************************************************
 * DrawStatusText  [COMCTL32.@]
 * DrawStatusTextA [COMCTL32.5]
 *
 * Draws text with borders, like in a status bar.
 *
 * PARAMS
 *     hdc   [I] handle to the window's display context
 *     lprc  [I] pointer to a rectangle
 *     text  [I] pointer to the text
 *     style [I] drawing style
 *
 * RETURNS
 *     No return value.
 */

void WINAPI DrawStatusTextA (HDC hdc, LPCRECT lprc, LPCSTR text, UINT style)
{
    INT len;
    LPWSTR textW = NULL;

    if ( text ) {
	if ( (len = MultiByteToWideChar( CP_ACP, 0, text, -1, NULL, 0 )) ) {
	    if ( (textW = Alloc( len * sizeof(WCHAR) )) )
		MultiByteToWideChar( CP_ACP, 0, text, -1, textW, len );
	}
    }
    DrawStatusTextW( hdc, lprc, textW, style );
    Free( textW );
}


/***********************************************************************
 * CreateStatusWindow  [COMCTL32.@]
 * CreateStatusWindowA [COMCTL32.6]
 *
 * Creates a status bar
 *
 * PARAMS
 *     style  [I] window style
 *     text   [I] pointer to the window text
 *     parent [I] handle to the parent window
 *     wid    [I] control id of the status bar
 *
 * RETURNS
 *     Success: handle to the status window
 *     Failure: 0
 */

HWND WINAPI
CreateStatusWindowA (LONG style, LPCSTR text, HWND parent, UINT wid)
{
    return CreateWindowA(STATUSCLASSNAMEA, text, style,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   parent, (HMENU)(DWORD_PTR)wid, 0, 0);
}


/***********************************************************************
 * CreateStatusWindowW [COMCTL32.@]
 *
 * Creates a status bar control
 *
 * PARAMS
 *     style  [I] window style
 *     text   [I] pointer to the window text
 *     parent [I] handle to the parent window
 *     wid    [I] control id of the status bar
 *
 * RETURNS
 *     Success: handle to the status window
 *     Failure: 0
 */

HWND WINAPI
CreateStatusWindowW (LONG style, LPCWSTR text, HWND parent, UINT wid)
{
    return CreateWindowW(STATUSCLASSNAMEW, text, style,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   parent, (HMENU)(DWORD_PTR)wid, 0, 0);
}


/***********************************************************************
 * CreateUpDownControl [COMCTL32.16]
 *
 * Creates an up-down control
 *
 * PARAMS
 *     style  [I] window styles
 *     x      [I] horizontal position of the control
 *     y      [I] vertical position of the control
 *     cx     [I] with of the control
 *     cy     [I] height of the control
 *     parent [I] handle to the parent window
 *     id     [I] the control's identifier
 *     inst   [I] handle to the application's module instance
 *     buddy  [I] handle to the buddy window, can be NULL
 *     maxVal [I] upper limit of the control
 *     minVal [I] lower limit of the control
 *     curVal [I] current value of the control
 *
 * RETURNS
 *     Success: handle to the updown control
 *     Failure: 0
 */

HWND WINAPI
CreateUpDownControl (DWORD style, INT x, INT y, INT cx, INT cy,
		     HWND parent, INT id, HINSTANCE inst,
		     HWND buddy, INT maxVal, INT minVal, INT curVal)
{
    HWND hUD =
	CreateWindowW (UPDOWN_CLASSW, 0, style, x, y, cx, cy,
			 parent, (HMENU)(DWORD_PTR)id, inst, 0);
    if (hUD) {
	SendMessageW (hUD, UDM_SETBUDDY, (WPARAM)buddy, 0);
	SendMessageW (hUD, UDM_SETRANGE, 0, MAKELONG(maxVal, minVal));
	SendMessageW (hUD, UDM_SETPOS, 0, MAKELONG(curVal, 0));
    }

    return hUD;
}


/***********************************************************************
 * InitCommonControls [COMCTL32.17]
 *
 * Registers the common controls.
 *
 * PARAMS
 *     No parameters.
 *
 * RETURNS
 *     No return values.
 *
 * NOTES
 *     This function is just a dummy - all the controls are registered at
 *     the DLL's initialization. See InitCommonContolsEx for details.
 */

VOID WINAPI
InitCommonControls (void)
{
}


/***********************************************************************
 * InitCommonControlsEx [COMCTL32.@]
 *
 * Registers the common controls.
 *
 * PARAMS
 *     lpInitCtrls [I] pointer to an INITCOMMONCONTROLS structure.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Probaly all versions of comctl32 initializes the Win95 controls in DllMain
 *     during DLL initializaiton. Starting from comctl32 v5.82 all the controls
 *     are initialized there. We follow this behaviour and this function is just
 *     a dummy.
 *
 *     Note: when writing programs under Windows, if you don't call any function
 *     from comctl32 the linker may not link this DLL. If InitCommonControlsEx
 *     was the only comctl32 function you were calling and you remove it you may
 *     have a false impression that InitCommonControlsEx actually did something.
 */

BOOL WINAPI
InitCommonControlsEx (const INITCOMMONCONTROLSEX *lpInitCtrls)
{
    if (!lpInitCtrls || lpInitCtrls->dwSize != sizeof(INITCOMMONCONTROLSEX))
        return FALSE;

    TRACE("(0x%08x)\n", lpInitCtrls->dwICC);
    return TRUE;
}


/***********************************************************************
 * CreateToolbarEx [COMCTL32.@]
 *
 * Creates a toolbar window.
 *
 * PARAMS
 *     hwnd
 *     style
 *     wID
 *     nBitmaps
 *     hBMInst
 *     wBMID
 *     lpButtons
 *     iNumButtons
 *     dxButton
 *     dyButton
 *     dxBitmap
 *     dyBitmap
 *     uStructSize
 *
 * RETURNS
 *     Success: handle to the tool bar control
 *     Failure: 0
 */

HWND WINAPI
CreateToolbarEx (HWND hwnd, DWORD style, UINT wID, INT nBitmaps,
                 HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
                 INT iNumButtons, INT dxButton, INT dyButton,
                 INT dxBitmap, INT dyBitmap, UINT uStructSize)
{
    HWND hwndTB;

    hwndTB =
        CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, style|WS_CHILD, 0,0,100,30,
                        hwnd, (HMENU)(DWORD_PTR)wID, COMCTL32_hModule, NULL);
    if(hwndTB) {
	TBADDBITMAP tbab;

        SendMessageW (hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM)uStructSize, 0);

       /* set bitmap and button size */
       /*If CreateToolbarEx receives 0, windows sets default values*/
       if (dxBitmap < 0)
           dxBitmap = 16;
       if (dyBitmap < 0)
           dyBitmap = 16;
       if (dxBitmap == 0 || dyBitmap == 0)
           dxBitmap = dyBitmap = 16;
       SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(dxBitmap, dyBitmap));

       if (dxButton < 0)
           dxButton = dxBitmap;
       if (dyButton < 0)
           dyButton = dyBitmap;
       /* TB_SETBUTTONSIZE -> TB_SETBITMAPSIZE bug introduced for Windows compatibility */
       if (dxButton != 0 && dyButton != 0)
            SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(dxButton, dyButton));


	/* add bitmaps */
	if (nBitmaps > 0 || hBMInst == HINST_COMMCTRL)
	{
	    tbab.hInst = hBMInst;
	    tbab.nID   = wBMID;

	    SendMessageW (hwndTB, TB_ADDBITMAP, (WPARAM)nBitmaps, (LPARAM)&tbab);
	}
	/* add buttons */
	if(iNumButtons > 0)
	SendMessageW (hwndTB, TB_ADDBUTTONSW,
			(WPARAM)iNumButtons, (LPARAM)lpButtons);
    }

    return hwndTB;
}


/***********************************************************************
 * CreateMappedBitmap [COMCTL32.8]
 *
 * Loads a bitmap resource using a colour map.
 *
 * PARAMS
 *     hInstance  [I] Handle to the module containing the bitmap.
 *     idBitmap   [I] The bitmap resource ID.
 *     wFlags     [I] CMB_MASKED for using bitmap as a mask or 0 for normal.
 *     lpColorMap [I] Colour information needed for the bitmap or NULL (uses system colours).
 *     iNumMaps   [I] Number of COLORMAP's pointed to by lpColorMap.
 *
 * RETURNS
 *     Success: handle to the new bitmap
 *     Failure: 0
 */

HBITMAP WINAPI
CreateMappedBitmap (HINSTANCE hInstance, INT_PTR idBitmap, UINT wFlags,
		    LPCOLORMAP lpColorMap, INT iNumMaps)
{
    HGLOBAL hglb;
    HRSRC hRsrc;
    const BITMAPINFOHEADER *lpBitmap;
    LPBITMAPINFOHEADER lpBitmapInfo;
    UINT nSize, nColorTableSize, iColor;
    RGBQUAD *pColorTable;
    INT i, iMaps, nWidth, nHeight;
    HDC hdcScreen;
    HBITMAP hbm;
    LPCOLORMAP sysColorMap;
    COLORREF cRef;
    COLORMAP internalColorMap[4] =
	{{0x000000, 0}, {0x808080, 0}, {0xC0C0C0, 0}, {0xFFFFFF, 0}};

    /* initialize pointer to colortable and default color table */
    if (lpColorMap) {
	iMaps = iNumMaps;
	sysColorMap = lpColorMap;
    }
    else {
	internalColorMap[0].to = GetSysColor (COLOR_BTNTEXT);
	internalColorMap[1].to = GetSysColor (COLOR_BTNSHADOW);
	internalColorMap[2].to = GetSysColor (COLOR_BTNFACE);
	internalColorMap[3].to = GetSysColor (COLOR_BTNHIGHLIGHT);
	iMaps = 4;
	sysColorMap = (LPCOLORMAP)internalColorMap;
    }

    hRsrc = FindResourceW (hInstance, (LPWSTR)idBitmap, (LPWSTR)RT_BITMAP);
    if (hRsrc == 0)
	return 0;
    hglb = LoadResource (hInstance, hRsrc);
    if (hglb == 0)
	return 0;
    lpBitmap = (LPBITMAPINFOHEADER)LockResource (hglb);
    if (lpBitmap == NULL)
	return 0;

    if (lpBitmap->biSize >= sizeof(BITMAPINFOHEADER) && lpBitmap->biClrUsed)
        nColorTableSize = lpBitmap->biClrUsed;
    else if (lpBitmap->biBitCount <= 8)	
        nColorTableSize = (1 << lpBitmap->biBitCount);
    else
        nColorTableSize = 0;
    nSize = lpBitmap->biSize + nColorTableSize * sizeof(RGBQUAD);
    lpBitmapInfo = (LPBITMAPINFOHEADER)GlobalAlloc (GMEM_FIXED, nSize);
    if (lpBitmapInfo == NULL)
	return 0;
    RtlMoveMemory (lpBitmapInfo, lpBitmap, nSize);

    pColorTable = (RGBQUAD*)(((LPBYTE)lpBitmapInfo)+(UINT)lpBitmapInfo->biSize);

    for (iColor = 0; iColor < nColorTableSize; iColor++) {
	for (i = 0; i < iMaps; i++) {
            cRef = RGB(pColorTable[iColor].rgbRed,
                       pColorTable[iColor].rgbGreen,
                       pColorTable[iColor].rgbBlue);
	    if ( cRef  == sysColorMap[i].from) {
#if 0
		if (wFlags & CBS_MASKED) {
		    if (sysColorMap[i].to != COLOR_BTNTEXT)
			pColorTable[iColor] = RGB(255, 255, 255);
		}
		else
#endif
                    pColorTable[iColor].rgbBlue  = GetBValue(sysColorMap[i].to);
                    pColorTable[iColor].rgbGreen = GetGValue(sysColorMap[i].to);
                    pColorTable[iColor].rgbRed   = GetRValue(sysColorMap[i].to);
		break;
	    }
	}
    }
    nWidth  = (INT)lpBitmapInfo->biWidth;
    nHeight = (INT)lpBitmapInfo->biHeight;
    hdcScreen = GetDC (NULL);
    hbm = CreateCompatibleBitmap (hdcScreen, nWidth, nHeight);
    if (hbm) {
	HDC hdcDst = CreateCompatibleDC (hdcScreen);
	HBITMAP hbmOld = SelectObject (hdcDst, hbm);
	const BYTE *lpBits = (const BYTE *)(lpBitmap + 1);
	lpBits += nColorTableSize * sizeof(RGBQUAD);
	StretchDIBits (hdcDst, 0, 0, nWidth, nHeight, 0, 0, nWidth, nHeight,
		         lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS,
		         SRCCOPY);
	SelectObject (hdcDst, hbmOld);
	DeleteDC (hdcDst);
    }
    ReleaseDC (NULL, hdcScreen);
    GlobalFree ((HGLOBAL)lpBitmapInfo);
    FreeResource (hglb);

    return hbm;
}


/***********************************************************************
 * CreateToolbar [COMCTL32.7]
 *
 * Creates a toolbar control.
 *
 * PARAMS
 *     hwnd
 *     style
 *     wID
 *     nBitmaps
 *     hBMInst
 *     wBMID
 *     lpButtons
 *     iNumButtons
 *
 * RETURNS
 *     Success: handle to the tool bar control
 *     Failure: 0
 *
 * NOTES
 *     Do not use this functions anymore. Use CreateToolbarEx instead.
 */

HWND WINAPI
CreateToolbar (HWND hwnd, DWORD style, UINT wID, INT nBitmaps,
	       HINSTANCE hBMInst, UINT wBMID,
	       LPCTBBUTTON lpButtons,INT iNumButtons)
{
    return CreateToolbarEx (hwnd, style | CCS_NODIVIDER, wID, nBitmaps,
			    hBMInst, wBMID, lpButtons,
			    iNumButtons, 0, 0, 0, 0, CCSIZEOF_STRUCT(TBBUTTON, dwData));
}


/***********************************************************************
 * DllGetVersion [COMCTL32.@]
 *
 * Retrieves version information of the 'COMCTL32.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a comctl32.dll from IE4.01 SP1.
 */

HRESULT WINAPI DllGetVersion (DLLVERSIONINFO *pdvi)
{
    if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) {
        WARN("wrong DLLVERSIONINFO size from app\n");
	return E_INVALIDARG;
    }

    pdvi->dwMajorVersion = COMCTL32_VERSION;
    pdvi->dwMinorVersion = COMCTL32_VERSION_MINOR;
    pdvi->dwBuildNumber = 2919;
    pdvi->dwPlatformID = 6304;

    TRACE("%u.%u.%u.%u\n",
	   pdvi->dwMajorVersion, pdvi->dwMinorVersion,
	   pdvi->dwBuildNumber, pdvi->dwPlatformID);

    return S_OK;
}

/***********************************************************************
 *		DllInstall (COMCTL32.@)
 *
 * Installs the ComCtl32 DLL.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: A HRESULT error
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
  FIXME("(%s, %s): stub\n", bInstall?"TRUE":"FALSE",
	debugstr_w(cmdline));

  return S_OK;
}

/***********************************************************************
 * _TrackMouseEvent [COMCTL32.@]
 *
 * Requests notification of mouse events
 *
 * During mouse tracking WM_MOUSEHOVER or WM_MOUSELEAVE events are posted
 * to the hwnd specified in the ptme structure.  After the event message
 * is posted to the hwnd, the entry in the queue is removed.
 *
 * If the current hwnd isn't ptme->hwndTrack the TME_HOVER flag is completely
 * ignored. The TME_LEAVE flag results in a WM_MOUSELEAVE message being posted
 * immediately and the TME_LEAVE flag being ignored.
 *
 * PARAMS
 *     ptme [I,O] pointer to TRACKMOUSEEVENT information structure.
 *
 * RETURNS
 *     Success: non-zero
 *     Failure: zero
 *
 * IMPLEMENTATION moved to USER32.TrackMouseEvent
 *
 */

BOOL WINAPI
_TrackMouseEvent (TRACKMOUSEEVENT *ptme)
{
    return TrackMouseEvent (ptme);
}

/*************************************************************************
 * GetMUILanguage [COMCTL32.@]
 *
 * Returns the user interface language in use by the current process.
 *
 * RETURNS
 *      Language ID in use by the current process.
 */
LANGID WINAPI GetMUILanguage (VOID)
{
    return COMCTL32_uiLang;
}


/*************************************************************************
 * InitMUILanguage [COMCTL32.@]
 *
 * Sets the user interface language to be used by the current process.
 *
 * RETURNS
 *      Nothing.
 */
VOID WINAPI InitMUILanguage (LANGID uiLang)
{
   COMCTL32_uiLang = uiLang;
}


/***********************************************************************
 * SetWindowSubclass [COMCTL32.410]
 *
 * Starts a window subclass
 *
 * PARAMS
 *     hWnd [in] handle to window subclass.
 *     pfnSubclass [in] Pointer to new window procedure.
 *     uIDSubclass [in] Unique identifier of sublass together with pfnSubclass.
 *     dwRef [in] Reference data to pass to window procedure.
 *
 * RETURNS
 *     Success: non-zero
 *     Failure: zero
 *
 * BUGS
 *     If an application manually subclasses a window after subclassing it with
 *     this API and then with this API again, then none of the previous 
 *     subclasses get called or the origional window procedure.
 */

BOOL WINAPI SetWindowSubclass (HWND hWnd, SUBCLASSPROC pfnSubclass,
                        UINT_PTR uIDSubclass, DWORD_PTR dwRef)
{
   LPSUBCLASS_INFO stack;
   LPSUBCLASSPROCS proc;

   TRACE ("(%p, %p, %lx, %lx)\n", hWnd, pfnSubclass, uIDSubclass, dwRef);

   /* Since the window procedure that we set here has two additional arguments,
    * we can't simply set it as the new window procedure of the window. So we
    * set our own window procedure and then calculate the other two arguments
    * from there. */

   /* See if we have been called for this window */
   stack = (LPSUBCLASS_INFO)GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack) {
      /* allocate stack */
      stack = Alloc (sizeof(SUBCLASS_INFO));
      if (!stack) {
         ERR ("Failed to allocate our Subclassing stack\n");
         return FALSE;
      }
      SetPropW (hWnd, COMCTL32_wSubclass, (HANDLE)stack);

      /* set window procedure to our own and save the current one */
      if (IsWindowUnicode (hWnd))
         stack->origproc = (WNDPROC)SetWindowLongPtrW (hWnd, GWLP_WNDPROC,
                                                   (DWORD_PTR)COMCTL32_SubclassProc);
      else
         stack->origproc = (WNDPROC)SetWindowLongPtrA (hWnd, GWLP_WNDPROC,
                                                   (DWORD_PTR)COMCTL32_SubclassProc);
   }
   else {
      /* Check to see if we have called this function with the same uIDSubClass
       * and pfnSubclass */
      proc = stack->SubclassProcs;
      while (proc) {
         if ((proc->id == uIDSubclass) &&
            (proc->subproc == pfnSubclass)) {
            proc->ref = dwRef;
            return TRUE;
         }
         proc = proc->next;
      }
   }
   
   proc = Alloc(sizeof(SUBCLASSPROCS));
   if (!proc) {
      ERR ("Failed to allocate subclass entry in stack\n");
      if (IsWindowUnicode (hWnd))
         SetWindowLongPtrW (hWnd, GWLP_WNDPROC, (DWORD_PTR)stack->origproc);
      else
         SetWindowLongPtrA (hWnd, GWLP_WNDPROC, (DWORD_PTR)stack->origproc);
      Free (stack);
      RemovePropW( hWnd, COMCTL32_wSubclass );
      return FALSE;
   }
   
   proc->subproc = pfnSubclass;
   proc->ref = dwRef;
   proc->id = uIDSubclass;
   proc->next = stack->SubclassProcs;
   stack->SubclassProcs = proc;

   return TRUE;
}


/***********************************************************************
 * GetWindowSubclass [COMCTL32.411]
 *
 * Gets the Reference data from a subclass.
 *
 * PARAMS
 *     hWnd [in] Handle to window which were subclassing
 *     pfnSubclass [in] Pointer to the subclass procedure
 *     uID [in] Unique indentifier of the subclassing procedure
 *     pdwRef [out] Pointer to the reference data
 *
 * RETURNS
 *     Success: Non-zero
 *     Failure: 0
 */

BOOL WINAPI GetWindowSubclass (HWND hWnd, SUBCLASSPROC pfnSubclass,
                              UINT_PTR uID, DWORD_PTR *pdwRef)
{
   const SUBCLASS_INFO *stack;
   const SUBCLASSPROCS *proc;

   TRACE ("(%p, %p, %lx, %p)\n", hWnd, pfnSubclass, uID, pdwRef);

   /* See if we have been called for this window */
   stack = (LPSUBCLASS_INFO)GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack)
      return FALSE;

   proc = stack->SubclassProcs;
   while (proc) {
      if ((proc->id == uID) &&
         (proc->subproc == pfnSubclass)) {
         *pdwRef = proc->ref;
         return TRUE;
      }
      proc = proc->next;
   }

   return FALSE;
}


/***********************************************************************
 * RemoveWindowSubclass [COMCTL32.412]
 *
 * Removes a window subclass.
 *
 * PARAMS
 *     hWnd [in] Handle to the window were subclassing
 *     pfnSubclass [in] Pointer to the subclass procedure
 *     uID [in] Unique identifier of this subclass
 *
 * RETURNS
 *     Success: non-zero
 *     Failure: zero
 */

BOOL WINAPI RemoveWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uID)
{
   LPSUBCLASS_INFO stack;
   LPSUBCLASSPROCS prevproc = NULL;
   LPSUBCLASSPROCS proc;
   BOOL ret = FALSE;

   TRACE ("(%p, %p, %lx)\n", hWnd, pfnSubclass, uID);

   /* Find the Subclass to remove */
   stack = (LPSUBCLASS_INFO)GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack)
      return FALSE;

   proc = stack->SubclassProcs;
   while (proc) {
      if ((proc->id == uID) &&
         (proc->subproc == pfnSubclass)) {
         
         if (!prevproc)
            stack->SubclassProcs = proc->next;
         else
            prevproc->next = proc->next;
          
         if (stack->stackpos == proc)
            stack->stackpos = stack->stackpos->next;
            
         Free (proc);
         ret = TRUE;
         break;
      }
      prevproc = proc;
      proc = proc->next;
   }
   
   if (!stack->SubclassProcs && !stack->running) {
      TRACE("Last Subclass removed, cleaning up\n");
      /* clean up our heap and reset the origional window procedure */
      if (IsWindowUnicode (hWnd))
         SetWindowLongPtrW (hWnd, GWLP_WNDPROC, (DWORD_PTR)stack->origproc);
      else
         SetWindowLongPtrA (hWnd, GWLP_WNDPROC, (DWORD_PTR)stack->origproc);
      Free (stack);
      RemovePropW( hWnd, COMCTL32_wSubclass );
   }
   
   return ret;
}

/***********************************************************************
 * COMCTL32_SubclassProc (internal)
 *
 * Window procedure for all subclassed windows. 
 * Saves the current subclassing stack position to support nested messages
 */
LRESULT WINAPI COMCTL32_SubclassProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LPSUBCLASS_INFO stack;
   LPSUBCLASSPROCS proc;
   LRESULT ret;
    
   TRACE ("(%p, 0x%08x, 0x%08lx, 0x%08lx)\n", hWnd, uMsg, wParam, lParam);

   stack = (LPSUBCLASS_INFO)GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack) {
      ERR ("Our sub classing stack got erased for %p!! Nothing we can do\n", hWnd);
      return 0;
   }
    
   /* Save our old stackpos to properly handle nested messages */
   proc = stack->stackpos;
   stack->stackpos = stack->SubclassProcs;
   stack->running++;
   ret = DefSubclassProc(hWnd, uMsg, wParam, lParam);
   stack->running--;
   stack->stackpos = proc;
    
   if (!stack->SubclassProcs && !stack->running) {
      TRACE("Last Subclass removed, cleaning up\n");
      /* clean up our heap and reset the origional window procedure */
      if (IsWindowUnicode (hWnd))
         SetWindowLongPtrW (hWnd, GWLP_WNDPROC, (DWORD_PTR)stack->origproc);
      else
         SetWindowLongPtrA (hWnd, GWLP_WNDPROC, (DWORD_PTR)stack->origproc);
      Free (stack);
      RemovePropW( hWnd, COMCTL32_wSubclass );
   }
   return ret;
}

/***********************************************************************
 * DefSubclassProc [COMCTL32.413]
 *
 * Calls the next window procedure (ie. the one before this subclass)
 *
 * PARAMS
 *     hWnd [in] The window that we're subclassing
 *     uMsg [in] Message
 *     wParam [in] WPARAM
 *     lParam [in] LPARAM
 *
 * RETURNS
 *     Success: non-zero
 *     Failure: zero
 */

LRESULT WINAPI DefSubclassProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LPSUBCLASS_INFO stack;
   LRESULT ret;
   
   TRACE ("(%p, 0x%08x, 0x%08lx, 0x%08lx)\n", hWnd, uMsg, wParam, lParam);

   /* retrieve our little stack from the Properties */
   stack = (LPSUBCLASS_INFO)GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack) {
      ERR ("Our sub classing stack got erased for %p!! Nothing we can do\n", hWnd);
      return 0;
   }

   /* If we are at the end of stack then we have to call the original
    * window procedure */
   if (!stack->stackpos) {
      if (IsWindowUnicode (hWnd))
         ret = CallWindowProcW (stack->origproc, hWnd, uMsg, wParam, lParam);
      else
         ret = CallWindowProcA (stack->origproc, hWnd, uMsg, wParam, lParam);
   } else {
      const SUBCLASSPROCS *proc = stack->stackpos;
      stack->stackpos = stack->stackpos->next; 
      /* call the Subclass procedure from the stack */
      ret = proc->subproc (hWnd, uMsg, wParam, lParam,
            proc->id, proc->ref);
   }

   return ret;
}


/***********************************************************************
 * COMCTL32_CreateToolTip [NOT AN API]
 *
 * Creates a tooltip for the control specified in hwnd and does all
 * necessary setup and notifications.
 *
 * PARAMS
 *     hwndOwner [I] Handle to the window that will own the tool tip.
 *
 * RETURNS
 *     Success: Handle of tool tip window.
 *     Failure: NULL
 */

HWND
COMCTL32_CreateToolTip(HWND hwndOwner)
{
    HWND hwndToolTip;

    hwndToolTip = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, WS_POPUP,
				  CW_USEDEFAULT, CW_USEDEFAULT,
				  CW_USEDEFAULT, CW_USEDEFAULT, hwndOwner,
				  0, 0, 0);

    /* Send NM_TOOLTIPSCREATED notification */
    if (hwndToolTip)
    {
	NMTOOLTIPSCREATED nmttc;
        /* true owner can be different if hwndOwner is a child window */
        HWND hwndTrueOwner = GetWindow(hwndToolTip, GW_OWNER);
        nmttc.hdr.hwndFrom = hwndTrueOwner;
        nmttc.hdr.idFrom = GetWindowLongPtrW(hwndTrueOwner, GWLP_ID);
	nmttc.hdr.code = NM_TOOLTIPSCREATED;
	nmttc.hwndToolTips = hwndToolTip;

       SendMessageW(GetParent(hwndTrueOwner), WM_NOTIFY,
                    (WPARAM)GetWindowLongPtrW(hwndTrueOwner, GWLP_ID),
		    (LPARAM)&nmttc);
    }

    return hwndToolTip;
}


/***********************************************************************
 * COMCTL32_RefreshSysColors [NOT AN API]
 *
 * Invoked on any control recognizing a WM_SYSCOLORCHANGE message to
 * refresh the color values in the color structure
 *
 * PARAMS
 *     none
 *
 * RETURNS
 *     none
 */

VOID
COMCTL32_RefreshSysColors(void)
{
    comctl32_color.clrBtnHighlight = GetSysColor (COLOR_BTNHIGHLIGHT);
    comctl32_color.clrBtnShadow = GetSysColor (COLOR_BTNSHADOW);
    comctl32_color.clrBtnText = GetSysColor (COLOR_BTNTEXT);
    comctl32_color.clrBtnFace = GetSysColor (COLOR_BTNFACE);
    comctl32_color.clrHighlight = GetSysColor (COLOR_HIGHLIGHT);
    comctl32_color.clrHighlightText = GetSysColor (COLOR_HIGHLIGHTTEXT);
    comctl32_color.clr3dHilight = GetSysColor (COLOR_3DHILIGHT);
    comctl32_color.clr3dShadow = GetSysColor (COLOR_3DSHADOW);
    comctl32_color.clr3dDkShadow = GetSysColor (COLOR_3DDKSHADOW);
    comctl32_color.clr3dFace = GetSysColor (COLOR_3DFACE);
    comctl32_color.clrWindow = GetSysColor (COLOR_WINDOW);
    comctl32_color.clrWindowText = GetSysColor (COLOR_WINDOWTEXT);
    comctl32_color.clrGrayText = GetSysColor (COLOR_GRAYTEXT);
    comctl32_color.clrActiveCaption = GetSysColor (COLOR_ACTIVECAPTION);
    comctl32_color.clrInfoBk = GetSysColor (COLOR_INFOBK);
    comctl32_color.clrInfoText = GetSysColor (COLOR_INFOTEXT);
}

/***********************************************************************
 * COMCTL32_DrawInsertMark [NOT AN API]
 *
 * Draws an insertion mark (which looks similar to an 'I').
 *
 * PARAMS
 *     hDC           [I] Device context to draw onto.
 *     lpRect        [I] Co-ordinates of insertion mark.
 *     clrInsertMark [I] Colour of the insertion mark.
 *     bHorizontal   [I] True if insert mark should be drawn horizontally,
 *                       vertical otherwise.
 *
 * RETURNS
 *     none
 *
 * NOTES
 *     Draws up to but not including the bottom co-ordinate when drawing
 *     vertically or the right co-ordinate when horizontal.
 */
void COMCTL32_DrawInsertMark(HDC hDC, const RECT *lpRect, COLORREF clrInsertMark, BOOL bHorizontal)
{
    HPEN hPen = CreatePen(PS_SOLID, 1, clrInsertMark);
    HPEN hOldPen;
    static const DWORD adwPolyPoints[] = {4,4,4};
    LONG lCentre = (bHorizontal ? 
        lpRect->top + (lpRect->bottom - lpRect->top)/2 : 
        lpRect->left + (lpRect->right - lpRect->left)/2);
    LONG l1 = (bHorizontal ? lpRect->left : lpRect->top);
    LONG l2 = (bHorizontal ? lpRect->right : lpRect->bottom);
    const POINT aptInsertMark[] =
    {
        /* top (V) or left (H) arrow */
        {lCentre    , l1 + 2},
        {lCentre - 2, l1    },
        {lCentre + 3, l1    },
        {lCentre + 1, l1 + 2},
        /* middle line */
        {lCentre    , l2 - 2},
        {lCentre    , l1 - 1},
        {lCentre + 1, l1 - 1},
        {lCentre + 1, l2 - 2},
        /* bottom (V) or right (H) arrow */
        {lCentre    , l2 - 3},
        {lCentre - 2, l2 - 1},
        {lCentre + 3, l2 - 1},
        {lCentre + 1, l2 - 3},
    };
    hOldPen = SelectObject(hDC, hPen);
    PolyPolyline(hDC, aptInsertMark, adwPolyPoints, sizeof(adwPolyPoints)/sizeof(adwPolyPoints[0]));
    SelectObject(hDC, hOldPen);
    DeleteObject(hPen);
}

/***********************************************************************
 * COMCTL32_EnsureBitmapSize [internal]
 *
 * If needed, enlarge the bitmap so that the width is at least cxMinWidth and
 * the height is at least cyMinHeight. If the bitmap already has these
 * dimensions nothing changes.
 *
 * PARAMS
 *     hBitmap       [I/O] Bitmap to modify. The handle may change
 *     cxMinWidth    [I]   If the width of the bitmap is smaller, then it will
 *                         be enlarged to this value
 *     cyMinHeight   [I]   If the height of the bitmap is smaller, then it will
 *                         be enlarged to this value
 *     cyBackground  [I]   The color with which the new area will be filled
 *
 * RETURNS
 *     none
 */
void COMCTL32_EnsureBitmapSize(HBITMAP *pBitmap, int cxMinWidth, int cyMinHeight, COLORREF crBackground)
{
    int cxNew, cyNew;
    BITMAP bmp;
    HBITMAP hNewBitmap;
    HBITMAP hNewDCBitmap, hOldDCBitmap;
    HBRUSH hNewDCBrush;
    HDC hdcNew, hdcOld;

    if (!GetObjectW(*pBitmap, sizeof(BITMAP), &bmp))
        return;
    cxNew = (cxMinWidth > bmp.bmWidth ? cxMinWidth : bmp.bmWidth);
    cyNew = (cyMinHeight > bmp.bmHeight ? cyMinHeight : bmp.bmHeight);
    if (cxNew == bmp.bmWidth && cyNew == bmp.bmHeight)
        return;

    hdcNew = CreateCompatibleDC(NULL);
    hNewBitmap = CreateBitmap(cxNew, cyNew, bmp.bmPlanes, bmp.bmBitsPixel, NULL);
    hNewDCBitmap = SelectObject(hdcNew, hNewBitmap);
    hNewDCBrush = SelectObject(hdcNew, CreateSolidBrush(crBackground));

    hdcOld = CreateCompatibleDC(NULL);
    hOldDCBitmap = SelectObject(hdcOld, *pBitmap);

    BitBlt(hdcNew, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcOld, 0, 0, SRCCOPY);
    if (bmp.bmWidth < cxMinWidth)
        PatBlt(hdcNew, bmp.bmWidth, 0, cxNew, bmp.bmHeight, PATCOPY);
    if (bmp.bmHeight < cyMinHeight)
        PatBlt(hdcNew, 0, bmp.bmHeight, bmp.bmWidth, cyNew, PATCOPY);
    if (bmp.bmWidth < cxMinWidth && bmp.bmHeight < cyMinHeight)
        PatBlt(hdcNew, bmp.bmWidth, bmp.bmHeight, cxNew, cyNew, PATCOPY);

    SelectObject(hdcNew, hNewDCBitmap);
    DeleteObject(SelectObject(hdcNew, hNewDCBrush));
    DeleteDC(hdcNew);
    SelectObject(hdcOld, hOldDCBitmap);
    DeleteDC(hdcOld);

    DeleteObject(*pBitmap);    
    *pBitmap = hNewBitmap;
    return;
}

/***********************************************************************
 * MirrorIcon [COMCTL32.414]
 *
 * Mirrors an icon so that it will appear correctly on a mirrored DC.
 *
 * PARAMS
 *     phicon1 [I/O] Icon.
 *     phicon2 [I/O] Icon.
 *
 * RETURNS
 *     Success: TRUE.
 *     Failure: FALSE.
 */
BOOL WINAPI MirrorIcon(HICON *phicon1, HICON *phicon2)
{
    FIXME("(%p, %p): stub\n", phicon1, phicon2);
    return FALSE;
}

static inline int IsDelimiter(WCHAR c)
{
    switch(c)
    {
	case '/':
	case '\\':
	case '.':
	case ' ':
	    return TRUE;
    }
    return FALSE;
}

static int CALLBACK PathWordBreakProc(LPCWSTR lpch, int ichCurrent, int cch, int code)
{
    if (code == WB_ISDELIMITER)
        return IsDelimiter(lpch[ichCurrent]);
    else
    {
        int dir = (code == WB_LEFT) ? -1 : 1;
        for(; 0 <= ichCurrent && ichCurrent < cch; ichCurrent += dir)
            if (IsDelimiter(lpch[ichCurrent])) return ichCurrent;
    }
    return ichCurrent;
}

/***********************************************************************
 * SetPathWordBreakProc [COMCTL32.384]
 *
 * Sets the word break procedure for an edit control to one that understands
 * paths so that the user can jump over directories.
 *
 * PARAMS
 *     hwnd [I] Handle to edit control.
 *     bSet [I] If this is TRUE then the word break proc is set, otherwise it is removed.
 *
 * RETURNS
 *     Result from EM_SETWORDBREAKPROC message.
 */
LRESULT WINAPI SetPathWordBreakProc(HWND hwnd, BOOL bSet)
{
    return SendMessageW(hwnd, EM_SETWORDBREAKPROC, 0,
        (LPARAM)(bSet ? PathWordBreakProc : NULL));
}

/***********************************************************************
 * DrawShadowText [COMCTL32.@]
 *
 * Draw text with shadow.
 */
int WINAPI DrawShadowText(HDC hdc, LPCWSTR pszText, UINT cch, const RECT *pRect, DWORD dwFlags,
                          COLORREF crText, COLORREF crShadow, int ixOffset, int iyOffset)
{
    RECT rect = *pRect;

    FIXME("(%p, %s, %d, %p, %d, 0x%08x, 0x%08x, %d, %d): stub\n", hdc, debugstr_w(pszText), cch, pRect, dwFlags,
                                                                  crText, crShadow, ixOffset, iyOffset);
    return DrawTextW(hdc, pszText, cch, &rect, DT_LEFT);
}
