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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 * 
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Oct. 21, 2002, by Christian Neumair.
 *
 * Unless otherwise noted, we belive this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 *
 * TODO
 *   -- implement GetMUILanguage + InitMUILanguage
 *   -- LibMain => DLLMain ("DLLMain takes over the functionality of both the
 *                           LibMain and the WEP function.", MSDN)
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
 *   -- ICC_LINK_CLASS (not yet implemented)
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

extern void ANIMATE_Register(void);
extern void ANIMATE_Unregister(void);
extern void COMBOEX_Register(void);
extern void COMBOEX_Unregister(void);
extern void DATETIME_Register(void);
extern void DATETIME_Unregister(void);
extern void FLATSB_Register(void);
extern void FLATSB_Unregister(void);
extern void HEADER_Register(void);
extern void HEADER_Unregister(void);
extern void HOTKEY_Register(void);
extern void HOTKEY_Unregister(void);
extern void IPADDRESS_Register(void);
extern void IPADDRESS_Unregister(void);
extern void LISTVIEW_Register(void);
extern void LISTVIEW_Unregister(void);
extern void MONTHCAL_Register(void);
extern void MONTHCAL_Unregister(void);
extern void NATIVEFONT_Register(void);
extern void NATIVEFONT_Unregister(void);
extern void PAGER_Register(void);
extern void PAGER_Unregister(void);
extern void PROGRESS_Register(void);
extern void PROGRESS_Unregister(void);
extern void REBAR_Register(void);
extern void REBAR_Unregister(void);
extern void STATUS_Register(void);
extern void STATUS_Unregister(void);
extern void TAB_Register(void);
extern void TAB_Unregister(void);
extern void TOOLBAR_Register(void);
extern void TOOLBAR_Unregister(void);
extern void TOOLTIPS_Register(void);
extern void TOOLTIPS_Unregister(void);
extern void TRACKBAR_Register(void);
extern void TRACKBAR_Unregister(void);
extern void TREEVIEW_Register(void);
extern void TREEVIEW_Unregister(void);
extern void UPDOWN_Register(void);
extern void UPDOWN_Unregister(void);


LPSTR    COMCTL32_aSubclass = NULL;
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


/***********************************************************************
 * DllMain [Internal] Initializes the internal 'COMCTL32.DLL'.
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
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);

            COMCTL32_hModule = (HMODULE)hinstDLL;

            /* add global subclassing atom (used by 'tooltip' and 'updown') */
            COMCTL32_aSubclass = (LPSTR)(DWORD)GlobalAddAtomA ("CC32SubclassInfo");
            TRACE("Subclassing atom added: %p\n", COMCTL32_aSubclass);

            /* create local pattern brush */
            COMCTL32_hPattern55AABitmap = CreateBitmap (8, 8, 1, 1, wPattern55AA);
            COMCTL32_hPattern55AABrush = CreatePatternBrush (COMCTL32_hPattern55AABitmap);

	    /* Get all the colors at DLL load */
	    COMCTL32_RefreshSysColors();

            /* register all Win95 common control classes */
            ANIMATE_Register ();
            FLATSB_Register ();
            HEADER_Register ();
            HOTKEY_Register ();
            LISTVIEW_Register ();
            PROGRESS_Register ();
            STATUS_Register ();
            TAB_Register ();
            TOOLBAR_Register ();
            TOOLTIPS_Register ();
            TRACKBAR_Register ();
            TREEVIEW_Register ();
            UPDOWN_Register ();
            break;

	case DLL_PROCESS_DETACH:
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
            GlobalDeleteAtom (LOWORD(COMCTL32_aSubclass));
            TRACE("Subclassing atom deleted: %p\n", COMCTL32_aSubclass);
            COMCTL32_aSubclass = NULL;
            break;
    }

    return TRUE;
}


/***********************************************************************
 * MenuHelp [COMCTL32.2]
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
	    TRACE("WM_MENUSELECT wParam=0x%X lParam=0x%lX\n",
		   wParam, lParam);

            if ((HIWORD(wParam) == 0xFFFF) && (lParam == 0)) {
                /* menu was closed */
		TRACE("menu was closed!\n");
                SendMessageA (hwndStatus, SB_SIMPLE, FALSE, 0);
            }
	    else {
		/* menu item was selected */
		if (HIWORD(wParam) & MF_POPUP)
		    uMenuID = (UINT)*(lpwIDs+1);
		else
		    uMenuID = (UINT)LOWORD(wParam);
		TRACE("uMenuID = %u\n", uMenuID);

		if (uMenuID) {
		    CHAR szText[256];

		    if (!LoadStringA (hInst, uMenuID, szText, 256))
			szText[0] = '\0';

		    SendMessageA (hwndStatus, SB_SETTEXTA,
				    255 | SBT_NOBORDERS, (LPARAM)szText);
		    SendMessageA (hwndStatus, SB_SIMPLE, TRUE, 0);
		}
	    }
	    break;

        case WM_COMMAND :
	    TRACE("WM_COMMAND wParam=0x%X lParam=0x%lX\n",
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
 *     Identifier of the menu item to receive or loose a check mark.
 *
 *     lpInfo
 *     The array of integers contains pairs of values. BOTH values of
 *     the first pair must be the handles to the application's main menu.
 *     Each subsequent pair consists of a menu id and control id.
 */

BOOL WINAPI
ShowHideMenuCtl (HWND hwnd, UINT uFlags, LPINT lpInfo)
{
    LPINT lpMenuId;

    TRACE("%p, %x, %p\n", hwnd, uFlags, lpInfo);

    if (lpInfo == NULL)
	return FALSE;

    if (!(lpInfo[0]) || !(lpInfo[1]))
	return FALSE;

    /* search for control */
    lpMenuId = &lpInfo[2];
    while (*lpMenuId != uFlags)
	lpMenuId += 2;

    if (GetMenuState ((HMENU)lpInfo[1], uFlags, MF_BYCOMMAND) & MFS_CHECKED) {
	/* uncheck menu item */
	CheckMenuItem ((HMENU)lpInfo[0], *lpMenuId, MF_BYCOMMAND | MF_UNCHECKED);

	/* hide control */
	lpMenuId++;
	SetWindowPos (GetDlgItem (hwnd, *lpMenuId), 0, 0, 0, 0, 0,
			SWP_HIDEWINDOW);
    }
    else {
	/* check menu item */
	CheckMenuItem ((HMENU)lpInfo[0], *lpMenuId, MF_BYCOMMAND | MF_CHECKED);

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
GetEffectiveClientRect (HWND hwnd, LPRECT lpRect, LPINT lpInfo)
{
    RECT rcCtrl;
    INT  *lpRun;
    HWND hwndCtrl;

    TRACE("(0x%08lx 0x%08lx 0x%08lx)\n",
	   (DWORD)hwnd, (DWORD)lpRect, (DWORD)lpInfo);

    GetClientRect (hwnd, lpRect);
    lpRun = lpInfo;

    do {
	lpRun += 2;
	if (*lpRun == 0)
	    return;
	lpRun++;
	hwndCtrl = GetDlgItem (hwnd, *lpRun);
	if (GetWindowLongA (hwndCtrl, GWL_STYLE) & WS_VISIBLE) {
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

void WINAPI DrawStatusTextW (HDC hdc, LPRECT lprc, LPCWSTR text, UINT style)
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
        if (*text == L'\t') {
	    text++;
	    align = DT_CENTER;
	    if (*text == L'\t') {
	        text++;
	        align = DT_RIGHT;
	    }
        }
        r.left += 3;
        if (style & SBT_RTLREADING)
	    FIXME("Unsupported RTL style!\n");
        DrawTextW (hdc, text, -1, &r, align|DT_VCENTER|DT_SINGLELINE);
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

void WINAPI DrawStatusTextA (HDC hdc, LPRECT lprc, LPCSTR text, UINT style)
{
    INT len;
    LPWSTR textW = NULL;

    if ( text ) {
	if ( (len = MultiByteToWideChar( CP_ACP, 0, text, -1, NULL, 0 )) ) {
	    if ( (textW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )) )
		MultiByteToWideChar( CP_ACP, 0, text, -1, textW, len );
	}
    }
    DrawStatusTextW( hdc, lprc, textW, style );
    HeapFree( GetProcessHeap(), 0, textW );
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
			   parent, (HMENU)wid, 0, 0);
}


/***********************************************************************
 * CreateStatusWindowW [COMCTL32.@] Creates a status bar control
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
			   parent, (HMENU)wid, 0, 0);
}


/***********************************************************************
 * CreateUpDownControl [COMCTL32.16] Creates an up-down control
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
	CreateWindowA (UPDOWN_CLASSA, 0, style, x, y, cx, cy,
			 parent, (HMENU)id, inst, 0);
    if (hUD) {
	SendMessageA (hUD, UDM_SETBUDDY, (WPARAM)buddy, 0);
	SendMessageA (hUD, UDM_SETRANGE, 0, MAKELONG(maxVal, minVal));
	SendMessageA (hUD, UDM_SETPOS, 0, MAKELONG(curVal, 0));
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
 *     This function is just a dummy.
 *     The Win95 controls are registered at the DLL's initialization.
 *     To register other controls InitCommonControlsEx() must be used.
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
 *     Only the additional common controls are registered by this function.
 *     The Win95 controls are registered at the DLL's initialization.
 *
 * FIXME
 *     implement the following control classes:
 *       ICC_LINK_CLASS
 *       ICC_STANDARD_CLASSES
 */

BOOL WINAPI
InitCommonControlsEx (LPINITCOMMONCONTROLSEX lpInitCtrls)
{
    INT cCount;
    DWORD dwMask;

    if (!lpInitCtrls)
	return FALSE;
    if (lpInitCtrls->dwSize != sizeof(INITCOMMONCONTROLSEX))
	return FALSE;

    TRACE("(0x%08lx)\n", lpInitCtrls->dwICC);

    for (cCount = 0; cCount < 32; cCount++) {
	dwMask = 1 << cCount;
	if (!(lpInitCtrls->dwICC & dwMask))
	    continue;

	switch (lpInitCtrls->dwICC & dwMask) {
	    /* dummy initialization */
	    case ICC_ANIMATE_CLASS:
	    case ICC_BAR_CLASSES:
	    case ICC_LISTVIEW_CLASSES:
	    case ICC_TREEVIEW_CLASSES:
	    case ICC_TAB_CLASSES:
	    case ICC_UPDOWN_CLASS:
	    case ICC_PROGRESS_CLASS:
	    case ICC_HOTKEY_CLASS:
		break;

	    /* advanced classes - not included in Win95 */
	    case ICC_DATE_CLASSES:
		MONTHCAL_Register ();
		DATETIME_Register ();
		break;

	    case ICC_USEREX_CLASSES:
		COMBOEX_Register ();
		break;

	    case ICC_COOL_CLASSES:
		REBAR_Register ();
		break;

	    case ICC_INTERNET_CLASSES:
		IPADDRESS_Register ();
		break;

	    case ICC_PAGESCROLLER_CLASS:
		PAGER_Register ();
		break;

	    case ICC_NATIVEFNTCTL_CLASS:
		NATIVEFONT_Register ();
		break;

	    default:
		FIXME("Unknown class! dwICC=0x%lX\n", dwMask);
		break;
	}
    }

    return TRUE;
}


/***********************************************************************
 * CreateToolbarEx [COMCTL32.@] Creates a tool bar window
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

    /* If not position is specified then put it at the top */
    if ((style & CCS_BOTTOM) == 0) {
      style|=CCS_TOP;
    }

    hwndTB =
        CreateWindowExA (0, TOOLBARCLASSNAMEA, "", style|WS_CHILD, 0, 0, 0, 0,
			   hwnd, (HMENU)wID, 0, NULL);
    if(hwndTB) {
	TBADDBITMAP tbab;

        SendMessageA (hwndTB, TB_BUTTONSTRUCTSIZE,
			(WPARAM)uStructSize, 0);

       /* set bitmap and button size */
       /*If CreateToolbarEx receives 0, windows sets default values*/
       if (dxBitmap <= 0)
           dxBitmap = 16;
       if (dyBitmap <= 0)
           dyBitmap = 15;
       SendMessageA (hwndTB, TB_SETBITMAPSIZE, 0,
                       MAKELPARAM((WORD)dxBitmap, (WORD)dyBitmap));

       if (dxButton <= 0)
           dxButton = 24;
       if (dyButton <= 0)
           dyButton = 22;
       SendMessageA (hwndTB, TB_SETBUTTONSIZE, 0,
                       MAKELPARAM((WORD)dxButton, (WORD)dyButton));


	/* add bitmaps */
	if (nBitmaps > 0)
	{
	tbab.hInst = hBMInst;
	tbab.nID   = wBMID;

	SendMessageA (hwndTB, TB_ADDBITMAP,
			(WPARAM)nBitmaps, (LPARAM)&tbab);
	}
	/* add buttons */
	if(iNumButtons > 0)
	SendMessageA (hwndTB, TB_ADDBUTTONSA,
			(WPARAM)iNumButtons, (LPARAM)lpButtons);
    }

    return hwndTB;
}


/***********************************************************************
 * CreateMappedBitmap [COMCTL32.8]
 *
 * PARAMS
 *     hInstance  [I]
 *     idBitmap   [I]
 *     wFlags     [I]
 *     lpColorMap [I]
 *     iNumMaps   [I]
 *
 * RETURNS
 *     Success: handle to the new bitmap
 *     Failure: 0
 */

HBITMAP WINAPI
CreateMappedBitmap (HINSTANCE hInstance, INT idBitmap, UINT wFlags,
		    LPCOLORMAP lpColorMap, INT iNumMaps)
{
    HGLOBAL hglb;
    HRSRC hRsrc;
    LPBITMAPINFOHEADER lpBitmap, lpBitmapInfo;
    UINT nSize, nColorTableSize;
    RGBQUAD *pColorTable;
    INT iColor, i, iMaps, nWidth, nHeight;
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

    hRsrc = FindResourceA (hInstance, (LPSTR)idBitmap, (LPSTR)RT_BITMAP);
    if (hRsrc == 0)
	return 0;
    hglb = LoadResource (hInstance, hRsrc);
    if (hglb == 0)
	return 0;
    lpBitmap = (LPBITMAPINFOHEADER)LockResource (hglb);
    if (lpBitmap == NULL)
	return 0;

    nColorTableSize = (1 << lpBitmap->biBitCount);
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
	LPBYTE lpBits = (LPBYTE)(lpBitmap + 1);
	lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);
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
 * CreateToolbar [COMCTL32.7] Creates a tool bar control
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

HRESULT WINAPI
COMCTL32_DllGetVersion (DLLVERSIONINFO *pdvi)
{
    if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) {
        WARN("wrong DLLVERSIONINFO size from app\n");
	return E_INVALIDARG;
    }

    pdvi->dwMajorVersion = COMCTL32_VERSION;
    pdvi->dwMinorVersion = COMCTL32_VERSION_MINOR;
    pdvi->dwBuildNumber = 2919;
    pdvi->dwPlatformID = 6304;

    TRACE("%lu.%lu.%lu.%lu\n",
	   pdvi->dwMajorVersion, pdvi->dwMinorVersion,
	   pdvi->dwBuildNumber, pdvi->dwPlatformID);

    return S_OK;
}

/***********************************************************************
 *		DllInstall (COMCTL32.@)
 */
HRESULT WINAPI COMCTL32_DllInstall(BOOL bInstall, LPCWSTR cmdline)
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
 * FIXME: "Returns the language currently in use by the common controls
 * for a particular process." (MSDN)
 *
 */
LANGID WINAPI GetMUILanguage (VOID)
{
    return COMCTL32_uiLang;
}


/*************************************************************************
 * InitMUILanguage [COMCTL32.@]
 *
 * FIXME: "Enables an application to specify a language to be used with
 * the common controls that is different than the system language." (MSDN)
 *
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
   int newnum, n;

   TRACE ("(%p, %p, %x, %lx)\n", hWnd, pfnSubclass, uIDSubclass, dwRef);

   /* Since the window procedure that we set here has two additional arguments,
    * we can't simply set it as the new window procedure of the window. So we
    * set our own window procedure and then calculate the other two arguments
    * from there. */

   /* See if we have been called for this window */
   stack = (LPSUBCLASS_INFO)GetPropA (hWnd, COMCTL32_aSubclass);
   if (!stack) {
      /* allocate stack */
      stack = (LPSUBCLASS_INFO)HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY,
                                         sizeof(SUBCLASS_INFO));
      if (!stack) {
         ERR ("Failed to allocate our Subclassing stack\n");
         return FALSE;
      }
      SetPropA (hWnd, COMCTL32_aSubclass, (HANDLE)stack);

      /* set window procedure to our own and save the current one */
      if (IsWindowUnicode (hWnd))
         stack->origproc = (WNDPROC)SetWindowLongW (hWnd, GWL_WNDPROC,
                                                   (LONG)DefSubclassProc);
      else
         stack->origproc = (WNDPROC)SetWindowLongA (hWnd, GWL_WNDPROC,
                                                   (LONG)DefSubclassProc);
   } else {
      WNDPROC current;
      if (IsWindowUnicode (hWnd))
         current = (WNDPROC)GetWindowLongW (hWnd, GWL_WNDPROC);
      else
         current = (WNDPROC)GetWindowLongA (hWnd, GWL_WNDPROC);

      if (current != DefSubclassProc) {
         ERR ("Application has subclassed with our procedure, then manually, then with us again.  The current implementation can't handle this.\n");
         return FALSE;
      }
   }

   /* Check to see if we have called this function with the same uIDSubClass
    * and pfnSubclass */
   for (n = 0; n <= stack->stacknum + stack->stacknew - 1; n++)
      if ((stack->SubclassProcs[n].id == uIDSubclass) && 
         (stack->SubclassProcs[n].subproc == pfnSubclass)) {
         stack->SubclassProcs[n].ref = dwRef;
         return TRUE;
      }

   if ((stack->stacknum + stack->stacknew) >= 32) {
      ERR ("We have a Subclass stack overflow, please increment size\n");
      return FALSE;
   }

   /* we can't simply increment both stackpos and stacknum because there might
    * be a window procedure running lower in the stack, we can only get them
    * up to date once the last window procedure has run */
   if (stack->stacknum == stack->stackpos) {
      stack->stacknum++;
      stack->stackpos++;
   } else
      stack->stacknew++;

   newnum = stack->stacknew + stack->stacknum - 1;

   stack->SubclassProcs[newnum].subproc = pfnSubclass;
   stack->SubclassProcs[newnum].ref = dwRef;
   stack->SubclassProcs[newnum].id = uIDSubclass;
   
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
   LPSUBCLASS_INFO stack;
   int n;

   TRACE ("(%p, %p, %x, %p)\n", hWnd, pfnSubclass, uID, pdwRef);

   /* See if we have been called for this window */
   stack = (LPSUBCLASS_INFO)GetPropA (hWnd, COMCTL32_aSubclass);
   if (!stack)
      return FALSE;

   for (n = 0; n <= stack->stacknum + stack->stacknew - 1; n++)
      if ((stack->SubclassProcs[n].id == uID) &&
         (stack->SubclassProcs[n].subproc == pfnSubclass)) {
         *pdwRef = stack->SubclassProcs[n].ref;
         return TRUE;
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
   int n;

   TRACE ("(%p, %p, %x)\n", hWnd, pfnSubclass, uID);

   /* Find the Subclass to remove */
   stack = (LPSUBCLASS_INFO)GetPropA (hWnd, COMCTL32_aSubclass);
   if (!stack)
      return FALSE;

   if ((stack->stacknum == 1) && (stack->stackpos == 1) && !stack->stacknew) {
      TRACE("Last Subclass removed, cleaning up\n");
      /* clean up our heap and reset the origional window procedure */
      if (IsWindowUnicode (hWnd))
         SetWindowLongW (hWnd, GWL_WNDPROC, (LONG)stack->origproc);
      else
         SetWindowLongA (hWnd, GWL_WNDPROC, (LONG)stack->origproc);
      HeapFree (GetProcessHeap (), 0, stack);
      RemovePropA( hWnd, COMCTL32_aSubclass );
      return TRUE;
   }
 
   for (n = stack->stacknum + stack->stacknew - 1; n >= 0; n--)
      if ((stack->SubclassProcs[n].id == uID) &&
         (stack->SubclassProcs[n].subproc == pfnSubclass)) {
         if (n != (stack->stacknum + stack->stacknew))
            /* Fill the hole in the stack */
            memmove (&stack->SubclassProcs[n], &stack->SubclassProcs[n + 1],
                    sizeof(stack->SubclassProcs[0]) * (stack->stacknew + stack->stacknum - n));
         stack->SubclassProcs[n].subproc = NULL;
         stack->SubclassProcs[n].ref = 0;
         stack->SubclassProcs[n].id = 0;

         /* If we are currently running a window procedure we have to manipulate
          * the stack position pointers so that we don't corrupt the stack */
         if ((n < stack->stackpos) || (stack->stackpos == stack->stacknum)) {
            stack->stacknum--;
            stack->stackpos--;
         } else if (n >= stack->stackpos)
            stack->stacknew--;
         return TRUE;
      }

   return FALSE;
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
   int stackpos;
   LRESULT ret;

   /* retrieve our little stack from the Properties */
   stack = (LPSUBCLASS_INFO)GetPropA (hWnd, COMCTL32_aSubclass);
   if (!stack) {
      ERR ("Our sub classing stack got erased for %p!! Nothing we can do\n", hWnd);
      return 0;
   }

   /* If we are at pos 0 then we have to call the origional window procedure */
   if (stack->stackpos == 0) {
      if (IsWindowUnicode (hWnd))
         return CallWindowProcW (stack->origproc, hWnd, uMsg, wParam, lParam);
      else
         return CallWindowProcA (stack->origproc, hWnd, uMsg, wParam, lParam);
   }

   stackpos = --stack->stackpos;
   /* call the Subclass procedure from the stack */
   ret = stack->SubclassProcs[stackpos].subproc (hWnd, uMsg, wParam, lParam,
         stack->SubclassProcs[stackpos].id, stack->SubclassProcs[stackpos].ref);
   stack->stackpos++;

   if ((stack->stackpos == stack->stacknum) && stack->stacknew) {
      stack->stacknum += stack->stacknew;
      stack->stackpos += stack->stacknew;
      stack->stacknew = 0;
   }

   /* If we removed the last entry in our stack while a window procedure was
    * running then we have to clean up */
   if ((stack->stackpos == 0) && (stack->stacknum == 0)) {
      TRACE("Last Subclass removed, cleaning up\n");
      /* clean up our heap and reset the origional window procedure */
      if (IsWindowUnicode (hWnd))
         SetWindowLongW (hWnd, GWL_WNDPROC, (LONG)stack->origproc);
      else
         SetWindowLongA (hWnd, GWL_WNDPROC, (LONG)stack->origproc);
      HeapFree (GetProcessHeap (), 0, stack);
      RemovePropA( hWnd, COMCTL32_aSubclass );
      return TRUE;
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

    hwndToolTip = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0,
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
        nmttc.hdr.idFrom = GetWindowLongA(hwndTrueOwner, GWL_ID);
	nmttc.hdr.code = NM_TOOLTIPSCREATED;
	nmttc.hwndToolTips = hwndToolTip;

       SendMessageA(GetParent(hwndTrueOwner), WM_NOTIFY,
                    (WPARAM)GetWindowLongA(hwndTrueOwner, GWL_ID),
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
