/*
 * Common controls functions
 *
 * Copyright 1997 Dimitrie O. Paun
 *           1998 Juergen Schmied <j.schmied@metronet.de>
 * Copyright 1998,2000 Eric Kohl
 * Copyright 2014-2015 Michael Müller
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

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "winerror.h"
#include "winreg.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);


static LRESULT WINAPI COMCTL32_SubclassProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static LPWSTR COMCTL32_wSubclass = NULL;
HMODULE COMCTL32_hModule = 0;
static LANGID COMCTL32_uiLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
HBRUSH  COMCTL32_hPattern55AABrush = NULL;
COMCTL32_SysColor  comctl32_color;

static HBITMAP COMCTL32_hPattern55AABitmap = NULL;

static const WORD wPattern55AA[] =
{
    0x5555, 0xaaaa, 0x5555, 0xaaaa,
    0x5555, 0xaaaa, 0x5555, 0xaaaa
};

static const WCHAR strCC32SubclassInfo[] = L"CC32SubclassInfo";

static const struct
{
    const WCHAR *nameW;
    void (*fn_register)(void);
}
classes[] =
{
#if __WINE_COMCTL32_VERSION == 6
    {L"Button", BUTTON_Register},
    {L"ComboBox", COMBO_Register},
    {L"ComboBoxEx32", COMBOEX_Register},
    {L"ComboLBox", COMBOLBOX_Register},
    {L"Edit", EDIT_Register},
    {L"ListBox", LISTBOX_Register},
    {L"msctls_hotkey32", HOTKEY_Register},
    {L"msctls_progress32", PROGRESS_Register},
    {L"msctls_statusbar32", STATUS_Register},
    {L"msctls_trackbar32", TRACKBAR_Register},
    {L"msctls_updown32", UPDOWN_Register},
    {L"NativeFontCtl", NATIVEFONT_Register},
    {L"ReBarWindow32", REBAR_Register},
    {L"Static", STATIC_Register},
    {L"SysAnimate32", ANIMATE_Register},
    {L"SysDateTimePick32", DATETIME_Register},
    {L"SysHeader32", HEADER_Register},
    {L"SysIPAddress32", IPADDRESS_Register},
    {L"SysLink", SYSLINK_Register},
    {L"SysListView32", LISTVIEW_Register},
    {L"SysMonthCal32", MONTHCAL_Register},
    {L"SysPager", PAGER_Register},
    {L"SysTabControl32", TAB_Register},
    {L"SysTreeView32", TREEVIEW_Register},
    {L"ToolbarWindow32", TOOLBAR_Register},
    {L"tooltips_class32", TOOLTIPS_Register},
#else
    {L"ComboBoxEx32", COMBOEX_Register},
    {L"msctls_hotkey32", HOTKEY_Register},
    {L"msctls_progress32", PROGRESS_Register},
    {L"msctls_statusbar32", STATUS_Register},
    {L"msctls_trackbar32", TRACKBAR_Register},
    {L"msctls_updown32", UPDOWN_Register},
    {L"NativeFontCtl", NATIVEFONT_Register},
    {L"ReBarWindow32", REBAR_Register},
    {L"SysAnimate32", ANIMATE_Register},
    {L"SysDateTimePick32", DATETIME_Register},
    {L"SysHeader32", HEADER_Register},
    {L"SysIPAddress32", IPADDRESS_Register},
    {L"SysListView32", LISTVIEW_Register},
    {L"SysMonthCal32", MONTHCAL_Register},
    {L"SysPager", PAGER_Register},
    {L"SysTabControl32", TAB_Register},
    {L"SysTreeView32", TREEVIEW_Register},
    {L"ToolbarWindow32", TOOLBAR_Register},
    {L"tooltips_class32", TOOLTIPS_Register},
#endif /* __WINE_COMCTL32_VERSION == 6 */
};

static void unregister_classes(void)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(classes); i++)
    {
#if __WINE_COMCTL32_VERSION == 6
        WCHAR versioned_class[40];

        wcscpy(versioned_class, L"6.0.2600.2982!");
        wcscat(versioned_class, classes[i].nameW);
        UnregisterClassW(versioned_class, NULL);
#else
        UnregisterClassW(classes[i].nameW, NULL);
#endif
    }
}

BOOLEAN WINAPI RegisterClassNameW(const WCHAR *class)
{
    int min = 0, max = ARRAY_SIZE(classes) - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        if (!(res = wcsicmp(class, classes[pos].nameW)))
        {
            classes[pos].fn_register();
            return TRUE;
        }
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }

    return FALSE;
}

/***********************************************************************
 * DllMain [Internal]
 *
 * Initializes the internal 'COMCTL32.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the 'dlls' instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p, %#lx, %p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);

            COMCTL32_hModule = hinstDLL;

            /* add global subclassing atom (used by 'tooltip' and 'updown') */
            COMCTL32_wSubclass = (LPWSTR)(DWORD_PTR)GlobalAddAtomW (strCC32SubclassInfo);
            TRACE("Subclassing atom added: %p\n", COMCTL32_wSubclass);

            /* create local pattern brush */
            COMCTL32_hPattern55AABitmap = CreateBitmap (8, 8, 1, 1, wPattern55AA);
            COMCTL32_hPattern55AABrush = CreatePatternBrush (COMCTL32_hPattern55AABitmap);

	    /* Get all the colors at DLL load */
	    COMCTL32_RefreshSysColors();
            break;

	case DLL_PROCESS_DETACH:
            if (lpvReserved) break;

            /* unregister all common control classes */
            unregister_classes ();

            /* delete local pattern brush */
            DeleteObject (COMCTL32_hPattern55AABrush);
            DeleteObject (COMCTL32_hPattern55AABitmap);

            /* delete global subclassing atom */
            GlobalDeleteAtom (LOWORD(COMCTL32_wSubclass));
            TRACE("Subclassing atom deleted: %p\n", COMCTL32_wSubclass);
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
	    TRACE("WM_MENUSELECT wParam %#Ix, lParam %#Ix\n", wParam, lParam);

            if ((HIWORD(wParam) == 0xFFFF) && (lParam == 0)) {
                /* menu was closed */
		TRACE("menu was closed!\n");
                SendMessageW (hwndStatus, SB_SIMPLE, FALSE, 0);
            }
	    else {
		/* menu item was selected */
		if (HIWORD(wParam) & MF_POPUP)
		    uMenuID = *(lpwIDs+1);
		else
		    uMenuID = (UINT)LOWORD(wParam);
		TRACE("uMenuID = %u\n", uMenuID);

		if (uMenuID) {
		    WCHAR szText[256];

		    if (!LoadStringW (hInst, uMenuID, szText, ARRAY_SIZE(szText)))
			szText[0] = '\0';

		    SendMessageW (hwndStatus, SB_SETTEXTW,
				    255 | SBT_NOBORDERS, (LPARAM)szText);
		    SendMessageW (hwndStatus, SB_SIMPLE, TRUE, 0);
		}
	    }
	    break;

        case WM_COMMAND :
	    TRACE("WM_COMMAND wParam %#Ix, lParam %#Ix\n", wParam, lParam);
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

    TRACE("%p, %Ix, %p\n", hwnd, uFlags, lpInfo);

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

void COMCTL32_DrawStatusText(HDC hdc, LPCRECT lprc, LPCWSTR text, UINT style, BOOL draw_background)
{
    RECT r = *lprc;
    UINT border;
    COLORREF oldbkcolor;

    if (draw_background)
    {
        if (style & SBT_POPOUT)
            border = BDR_RAISEDOUTER;
        else if (style & SBT_NOBORDERS)
            border = 0;
        else
            border = BDR_SUNKENOUTER;

        oldbkcolor = SetBkColor (hdc, comctl32_color.clrBtnFace);
        DrawEdge (hdc, &r, border, BF_MIDDLE|BF_RECT|BF_ADJUST);
        SetBkColor (hdc, oldbkcolor);
    }

    /* now draw text */
    if (text) {
        int oldbkmode = SetBkMode (hdc, TRANSPARENT);
        COLORREF oldtextcolor;
        UINT align = DT_LEFT;
        int strCnt = 0;

        oldtextcolor = SetTextColor (hdc, comctl32_color.clrBtnText);
        if (style & SBT_RTLREADING)
            FIXME("Unsupported RTL style!\n");
        r.left += 3;
        do {
            if (*text == '\t') {
                if (strCnt) {
                    DrawTextW (hdc, text - strCnt, strCnt, &r, align|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
                    strCnt = 0;
                }
                if (align==DT_RIGHT) {
                    break;
                }
                align = (align==DT_LEFT ? DT_CENTER : DT_RIGHT);
            } else {
                strCnt++;
            }
        } while(*text++);

        if (strCnt) DrawTextW (hdc, text - strCnt, -1, &r, align|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        SetBkMode (hdc, oldbkmode);
        SetTextColor (hdc, oldtextcolor);
    }
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

void WINAPI DrawStatusTextW(HDC hdc, LPCRECT lprc, LPCWSTR text, UINT style)
{
    COMCTL32_DrawStatusText(hdc, lprc, text, style, TRUE);
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
 *     the DLL initialization time. See InitCommonControlsEx for details.
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
 *     Probably all versions of comctl32 initializes the Win95 controls in DllMain
 *     during DLL initialization. Starting from comctl32 v5.82 all the controls
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

    TRACE("%#lx\n", lpInitCtrls->dwICC);
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
                 HINSTANCE hBMInst, UINT_PTR wBMID, LPCTBBUTTON lpButtons,
                 INT iNumButtons, INT dxButton, INT dyButton,
                 INT dxBitmap, INT dyBitmap, UINT uStructSize)
{
    HWND hwndTB;

    hwndTB =
        CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, style|WS_CHILD, 0,0,100,30,
                        hwnd, (HMENU)(DWORD_PTR)wID, COMCTL32_hModule, NULL);
    if(hwndTB) {
	TBADDBITMAP tbab;

        SendMessageW (hwndTB, TB_BUTTONSTRUCTSIZE, uStructSize, 0);

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

            SendMessageW (hwndTB, TB_ADDBITMAP, nBitmaps, (LPARAM)&tbab);
	}
	/* add buttons */
	if(iNumButtons > 0)
        SendMessageW (hwndTB, TB_ADDBUTTONSW, iNumButtons, (LPARAM)lpButtons);
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
	sysColorMap = internalColorMap;
    }

    hRsrc = FindResourceW (hInstance, (LPWSTR)idBitmap, (LPWSTR)RT_BITMAP);
    if (hRsrc == 0)
	return 0;
    hglb = LoadResource (hInstance, hRsrc);
    if (hglb == 0)
	return 0;
    lpBitmap = LockResource (hglb);
    if (lpBitmap == NULL)
	return 0;

    if (lpBitmap->biSize >= sizeof(BITMAPINFOHEADER) && lpBitmap->biClrUsed)
        nColorTableSize = lpBitmap->biClrUsed;
    else if (lpBitmap->biBitCount <= 8)	
        nColorTableSize = (1 << lpBitmap->biBitCount);
    else
        nColorTableSize = 0;
    nSize = lpBitmap->biSize;
    if (nSize == sizeof(BITMAPINFOHEADER) && lpBitmap->biCompression == BI_BITFIELDS)
        nSize += 3 * sizeof(DWORD);
    nSize += nColorTableSize * sizeof(RGBQUAD);
    lpBitmapInfo = GlobalAlloc (GMEM_FIXED, nSize);
    if (lpBitmapInfo == NULL)
	return 0;
    RtlMoveMemory (lpBitmapInfo, lpBitmap, nSize);

    pColorTable = (RGBQUAD*)(((LPBYTE)lpBitmapInfo) + lpBitmapInfo->biSize);

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
    nWidth  = lpBitmapInfo->biWidth;
    nHeight = lpBitmapInfo->biHeight;
    hdcScreen = GetDC (NULL);
    hbm = CreateCompatibleBitmap (hdcScreen, nWidth, nHeight);
    if (hbm) {
	HDC hdcDst = CreateCompatibleDC (hdcScreen);
	HBITMAP hbmOld = SelectObject (hdcDst, hbm);
	const BYTE *lpBits = (const BYTE *)lpBitmap + nSize;
	StretchDIBits (hdcDst, 0, 0, nWidth, nHeight, 0, 0, nWidth, nHeight,
		         lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS,
		         SRCCOPY);
	SelectObject (hdcDst, hbmOld);
	DeleteDC (hdcDst);
    }
    ReleaseDC (NULL, hdcScreen);
    GlobalFree (lpBitmapInfo);
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
 *     Do not use this function anymore. Use CreateToolbarEx instead.
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


#if __WINE_COMCTL32_VERSION == 6
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
    TRACE("(%u, %s): stub\n", bInstall, debugstr_w(cmdline));
    return S_OK;
}
#endif /* __WINE_COMCTL32_VERSION == 6 */

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
 *     uIDSubclass [in] Unique identifier of subclass together with pfnSubclass.
 *     dwRef [in] Reference data to pass to window procedure.
 *
 * RETURNS
 *     Success: non-zero
 *     Failure: zero
 *
 * BUGS
 *     If an application manually subclasses a window after subclassing it with
 *     this API and then with this API again, then none of the previous 
 *     subclasses get called or the original window procedure.
 */

BOOL WINAPI SetWindowSubclass (HWND hWnd, SUBCLASSPROC pfnSubclass,
                        UINT_PTR uIDSubclass, DWORD_PTR dwRef)
{
   LPSUBCLASS_INFO stack;
   LPSUBCLASSPROCS proc;

   TRACE("%p, %p, %Ix, %Ix\n", hWnd, pfnSubclass, uIDSubclass, dwRef);

   if (!hWnd || !pfnSubclass)
       return FALSE;

   /* Since the window procedure that we set here has two additional arguments,
    * we can't simply set it as the new window procedure of the window. So we
    * set our own window procedure and then calculate the other two arguments
    * from there. */

   /* See if we have been called for this window */
   stack = GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack) {
      /* allocate stack */
      stack = Alloc (sizeof(SUBCLASS_INFO));
      if (!stack) {
         ERR ("Failed to allocate our Subclassing stack\n");
         return FALSE;
      }
      SetPropW (hWnd, COMCTL32_wSubclass, stack);

      /* set window procedure to our own and save the current one */
      stack->is_unicode = IsWindowUnicode (hWnd);
      stack->origproc = (WNDPROC)SetWindowLongPtrW (hWnd, GWLP_WNDPROC,
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
      if (stack->is_unicode)
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
 *     hWnd [in] Handle to the window which we are subclassing
 *     pfnSubclass [in] Pointer to the subclass procedure
 *     uID [in] Unique identifier of the subclassing procedure
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

   TRACE("%p, %p, %Ix, %p\n", hWnd, pfnSubclass, uID, pdwRef);

   /* See if we have been called for this window */
   stack = GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack)
      goto done;

   proc = stack->SubclassProcs;
   while (proc) {
      if ((proc->id == uID) &&
         (proc->subproc == pfnSubclass)) {
         if (pdwRef)
            *pdwRef = proc->ref;
         return TRUE;
      }
      proc = proc->next;
   }

done:
   if (pdwRef)
      *pdwRef = 0;
   return FALSE;
}


/***********************************************************************
 * RemoveWindowSubclass [COMCTL32.412]
 *
 * Removes a window subclass.
 *
 * PARAMS
 *     hWnd [in] Handle to the window which we are subclassing
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

   TRACE("%p, %p, %Ix.\n", hWnd, pfnSubclass, uID);

   /* Find the Subclass to remove */
   stack = GetPropW (hWnd, COMCTL32_wSubclass);
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
      /* clean up our heap and reset the original window procedure */
      if ((WNDPROC)GetWindowLongPtrW (hWnd, GWLP_WNDPROC) != COMCTL32_SubclassProc)
         WARN("Window procedure has been modified, skipping restore\n");
      else if (stack->is_unicode)
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
static LRESULT WINAPI COMCTL32_SubclassProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LPSUBCLASS_INFO stack;
   LPSUBCLASSPROCS proc;
   LRESULT ret;

   TRACE("%p, %#x, %#Ix, %#Ix\n", hWnd, uMsg, wParam, lParam);

   stack = GetPropW (hWnd, COMCTL32_wSubclass);
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
      /* clean up our heap and reset the original window procedure */
      if (stack->is_unicode)
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
 * Calls the next window procedure (i.e. the one before this subclass)
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

   TRACE("%p, %#x, %#Ix, %#Ix\n", hWnd, uMsg, wParam, lParam);

   /* retrieve our little stack from the Properties */
   stack = GetPropW (hWnd, COMCTL32_wSubclass);
   if (!stack) {
      ERR ("Our sub classing stack got erased for %p!! Nothing we can do\n", hWnd);
      return 0;
   }

   /* If we are at the end of stack then we have to call the original
    * window procedure */
   if (!stack->stackpos) {
      ret = CallWindowProcW (stack->origproc, hWnd, uMsg, wParam, lParam);
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
                     GetWindowLongPtrW(hwndTrueOwner, GWLP_ID), (LPARAM)&nmttc);
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
    comctl32_color.clrHotTrackingColor = GetSysColor (COLOR_HOTLIGHT);
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
    PolyPolyline(hDC, aptInsertMark, adwPolyPoints, ARRAY_SIZE(adwPolyPoints));
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

void COMCTL32_GetFontMetrics(HFONT hFont, TEXTMETRICW *ptm)
{
    HDC hdc = GetDC(NULL);
    HFONT hOldFont;

    hOldFont = SelectObject(hdc, hFont);
    GetTextMetricsW(hdc, ptm);
    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
}

#ifndef OCM__BASE      /* avoid including olectl.h */
#define OCM__BASE (WM_USER+0x1c00)
#endif

/***********************************************************************
 * COMCTL32_IsReflectedMessage [internal]
 *
 * Some parents reflect notify messages - for some messages sent by the child,
 * they send it back with the message code increased by OCM__BASE (0x2000).
 * This allows better subclassing of controls. We don't need to handle such
 * messages but we don't want to print ERRs for them, so this helper function
 * identifies them.
 *
 * Some of the codes are in the CCM_FIRST..CCM_LAST range, but there is no
 * collision with defined CCM_ codes.
 */
BOOL COMCTL32_IsReflectedMessage(UINT uMsg)
{
    switch (uMsg)
    {
        case OCM__BASE + WM_COMMAND:
        case OCM__BASE + WM_CTLCOLORBTN:
        case OCM__BASE + WM_CTLCOLOREDIT:
        case OCM__BASE + WM_CTLCOLORDLG:
        case OCM__BASE + WM_CTLCOLORLISTBOX:
        case OCM__BASE + WM_CTLCOLORMSGBOX:
        case OCM__BASE + WM_CTLCOLORSCROLLBAR:
        case OCM__BASE + WM_CTLCOLORSTATIC:
        case OCM__BASE + WM_DRAWITEM:
        case OCM__BASE + WM_MEASUREITEM:
        case OCM__BASE + WM_DELETEITEM:
        case OCM__BASE + WM_VKEYTOITEM:
        case OCM__BASE + WM_CHARTOITEM:
        case OCM__BASE + WM_COMPAREITEM:
        case OCM__BASE + WM_HSCROLL:
        case OCM__BASE + WM_VSCROLL:
        case OCM__BASE + WM_PARENTNOTIFY:
        case OCM__BASE + WM_NOTIFY:
            return TRUE;
        default:
            return FALSE;
    }
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

static inline BOOL IsDelimiter(WCHAR c)
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

#if __WINE_COMCTL32_VERSION == 6 || defined(__REACTOS__)
/***********************************************************************
 * DrawShadowText [COMCTL32.@]
 *
 * Draw text with shadow.
 */
int WINAPI DrawShadowText(HDC hdc, LPCWSTR text, UINT length, RECT *rect, DWORD flags,
                          COLORREF crText, COLORREF crShadow, int offset_x, int offset_y)
{
#ifdef __REACTOS__
    COLORREF crOldText;
    RECT rcText;
    INT iRet, x, y, x2, y2;
    BYTE *pBits;
    HBITMAP hbm, hbmOld;
    BITMAPINFO bi;
    HDC hdcMem;
    HFONT hOldFont;
    BLENDFUNCTION bf;

    /* Create 32 bit DIB section for the shadow */
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = rect->right - rect->left + 4;
    bi.bmiHeader.biHeight = rect->bottom - rect->top + 5; // bottom-up DIB
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (PVOID*)&pBits, NULL, 0);
    if(!hbm)
    {
        ERR("CreateDIBSection failed\n");
        return 0;
    }

    /* Create memory device context for new DIB section and select it */
    hdcMem = CreateCompatibleDC(hdc);
    if(!hdcMem)
    {
        ERR("CreateCompatibleDC failed\n");
        DeleteObject(hbm);
        return 0;
    }

    hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);

    /* Draw text on our helper bitmap */
    hOldFont = (HFONT)SelectObject(hdcMem, GetCurrentObject(hdc, OBJ_FONT));
    SetTextColor(hdcMem, RGB(16, 16, 16));
    SetBkColor(hdcMem, RGB(0, 0, 0));
    SetBkMode(hdcMem, TRANSPARENT);
    SetRect(&rcText, 0, 0, rect->right - rect->left, rect->bottom - rect->top);
    DrawTextW(hdcMem, text, length, &rcText, flags);
    SelectObject(hdcMem, hOldFont);

    /* Flush GDI so data pointed by pBits is valid */
    GdiFlush();

    /* Set alpha of pixels (forget about colors for now. They will be changed in next loop).
       We copy text image 4*5 times and each time alpha is added */
    for (x = 0; x < bi.bmiHeader.biWidth; ++x)
        for (y = 0; y < bi.bmiHeader.biHeight; ++y)
        {
            BYTE *pDest = &pBits[(y * bi.bmiHeader.biWidth + x) * 4];
            UINT Alpha = 0;

            for (x2 = x - 4 + 1; x2 <= x; ++x2)
                for (y2 = y; y2 < y + 5; ++y2)
                {
                    if (x2 >= 0 && x2 < bi.bmiHeader.biWidth && y2 >= 0 && y2 < bi.bmiHeader.biHeight)
                    {
                        BYTE *pSrc = &pBits[(y2 * bi.bmiHeader.biWidth + x2) * 4];
                        Alpha += pSrc[0];
                    }
                }

            if (Alpha > 255)
                Alpha = 255;
            pDest[3] = Alpha;
        }

    /* Now set the color of each pixel to shadow color * alpha (see GdiAlphaBlend) */
    for (x = 0; x < bi.bmiHeader.biWidth; ++x)
        for (y = 0; y < bi.bmiHeader.biHeight; ++y)
        {
            BYTE *pDest = &pBits[(y * bi.bmiHeader.biWidth + x) * 4];
            pDest[0] = GetBValue(crShadow) * pDest[3] / 255;
            pDest[1] = GetGValue(crShadow) * pDest[3] / 255;
            pDest[2] = GetRValue(crShadow) * pDest[3] / 255;
        }

    /* Fix ixOffset of the shadow (tested on Win) */
    offset_x -= 3;
    offset_y -= 3;

    /* Alpha blend helper image to destination DC */
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    GdiAlphaBlend(hdc, rect->left + offset_x, rect->top + offset_y, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, hdcMem, 0, 0, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, bf);

    /* Delete the helper bitmap */
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbm);
    DeleteDC(hdcMem);

    /* Finally draw the text over shadow */
    crOldText = SetTextColor(hdc, crText);
    SetBkMode(hdc, TRANSPARENT);
    iRet = DrawTextW(hdc, text, length, rect, flags);
    SetTextColor(hdc, crOldText);

    return iRet;
#else
    int bkmode, ret;
    COLORREF clr;
    RECT r;

    FIXME("%p, %s, %d, %p, %#lx, %#lx, %#lx, %d, %d: semi-stub\n", hdc, debugstr_w(text),
        length, rect, flags, crText, crShadow, offset_x, offset_y);

    bkmode = SetBkMode(hdc, TRANSPARENT);
    clr = SetTextColor(hdc, crShadow);

    /* FIXME: for shadow we need to render normally, blur it, and blend with current background. */
    r = *rect;
    OffsetRect(&r, 1, 1);
    DrawTextW(hdc, text, length, &r, flags);

    SetTextColor(hdc, crText);

    /* with text color on top of a shadow */
    ret = DrawTextW(hdc, text, length, rect, flags);

    SetTextColor(hdc, clr);
    SetBkMode(hdc, bkmode);

    return ret;
#endif
}

/***********************************************************************
 * LoadIconWithScaleDown [COMCTL32.@]
 */
HRESULT WINAPI LoadIconWithScaleDown(HINSTANCE hinst, const WCHAR *name, int cx, int cy, HICON *icon)
{
    TRACE("(%p, %s, %d, %d, %p)\n", hinst, debugstr_w(name), cx, cy, icon);

    *icon = NULL;

    if (!name)
        return E_INVALIDARG;

    *icon = LoadImageW(hinst, name, IMAGE_ICON, cx, cy,
                       (hinst || IS_INTRESOURCE(name)) ? 0 : LR_LOADFROMFILE);
    if (!*icon)
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

/***********************************************************************
 * LoadIconMetric [COMCTL32.@]
 */
HRESULT WINAPI LoadIconMetric(HINSTANCE hinst, const WCHAR *name, int size, HICON *icon)
{
    int cx, cy;

    TRACE("(%p, %s, %d, %p)\n", hinst, debugstr_w(name), size, icon);

    if (size == LIM_SMALL)
    {
        cx = GetSystemMetrics(SM_CXSMICON);
        cy = GetSystemMetrics(SM_CYSMICON);
    }
    else if (size == LIM_LARGE)
    {
        cx = GetSystemMetrics(SM_CXICON);
        cy = GetSystemMetrics(SM_CYICON);
    }
    else
    {
        *icon = NULL;
        return E_INVALIDARG;
    }

    return LoadIconWithScaleDown(hinst, name, cx, cy, icon);
}
#endif /* __WINE_COMCTL32_VERSION == 6 */

static const WCHAR strMRUList[] = L"MRUList";

/**************************************************************************
 * Alloc [COMCTL32.71]
 *
 * Allocates memory block from the dll's private heap
 */
void * WINAPI Alloc(DWORD size)
{
    return LocalAlloc(LMEM_ZEROINIT, size);
}

/**************************************************************************
 * ReAlloc [COMCTL32.72]
 *
 * Changes the size of an allocated memory block or allocates a memory
 * block using the dll's private heap.
 *
 */
void * WINAPI ReAlloc(void *src, DWORD size)
{
    if (src)
        return LocalReAlloc(src, size, LMEM_ZEROINIT | LMEM_MOVEABLE);
    else
        return LocalAlloc(LMEM_ZEROINIT, size);
}

/**************************************************************************
 * Free [COMCTL32.73]
 *
 * Frees an allocated memory block from the dll's private heap.
 */
BOOL WINAPI Free(void *mem)
{
    return !LocalFree(mem);
}

/**************************************************************************
 * GetSize [COMCTL32.74]
 */
DWORD WINAPI GetSize(void *mem)
{
    return LocalSize(mem);
}

/**************************************************************************
 * MRU-Functions  {COMCTL32}
 *
 * NOTES
 * The MRU-API is a set of functions to manipulate lists of M.R.U. (Most Recently
 * Used) items. It is an undocumented API that is used (at least) by the shell
 * and explorer to implement their recent documents feature.
 *
 * Since these functions are undocumented, they are unsupported by MS and
 * may change at any time.
 *
 * Internally, the list is implemented as a last in, last out list of items
 * persisted into the system registry under a caller chosen key. Each list
 * item is given a one character identifier in the Ascii range from 'a' to
 * '}'. A list of the identifiers in order from newest to oldest is stored
 * under the same key in a value named "MRUList".
 *
 * Items are re-ordered by changing the order of the values in the MRUList
 * value. When a new item is added, it becomes the new value of the oldest
 * identifier, and that identifier is moved to the front of the MRUList value.
 *
 * Wine stores MRU-lists in the same registry format as Windows, so when
 * switching between the builtin and native comctl32.dll no problems or
 * incompatibilities should occur.
 *
 * The following undocumented structure is used to create an MRU-list:
 *|typedef INT (CALLBACK *MRUStringCmpFn)(LPCTSTR lhs, LPCTSTR rhs);
 *|typedef INT (CALLBACK *MRUBinaryCmpFn)(LPCVOID lhs, LPCVOID rhs, DWORD length);
 *|
 *|typedef struct tagMRUINFO
 *|{
 *|    DWORD   cbSize;
 *|    UINT    uMax;
 *|    UINT    fFlags;
 *|    HKEY    hKey;
 *|    LPTSTR  lpszSubKey;
 *|    PROC    lpfnCompare;
 *|} MRUINFO, *LPMRUINFO;
 *
 * MEMBERS
 *  cbSize      [I] The size of the MRUINFO structure. This must be set
 *                  to sizeof(MRUINFO) by the caller.
 *  uMax        [I] The maximum number of items allowed in the list. Because
 *                  of the limited number of identifiers, this should be set to
 *                  a value from 1 to 30 by the caller.
 *  fFlags      [I] If bit 0 is set, the list will be used to store binary
 *                  data, otherwise it is assumed to store strings. If bit 1
 *                  is set, every change made to the list will be reflected in
 *                  the registry immediately, otherwise changes will only be
 *                  written when the list is closed.
 *  hKey        [I] The registry key that the list should be written under.
 *                  This must be supplied by the caller.
 *  lpszSubKey  [I] A caller supplied name of a subkey under hKey to write
 *                  the list to. This may not be blank.
 *  lpfnCompare [I] A caller supplied comparison function, which may be either
 *                  an MRUStringCmpFn if dwFlags does not have bit 0 set, or a
 *                  MRUBinaryCmpFn otherwise.
 *
 * FUNCTIONS
 *  - Create an MRU-list with CreateMRUList() or CreateMRUListLazy().
 *  - Add items to an MRU-list with AddMRUString() or AddMRUData().
 *  - Remove items from an MRU-list with DelMRUString().
 *  - Find data in an MRU-list with FindMRUString() or FindMRUData().
 *  - Iterate through an MRU-list with EnumMRUList().
 *  - Free an MRU-list with FreeMRUList().
 */

typedef INT (CALLBACK *MRUStringCmpFnA)(LPCSTR lhs, LPCSTR rhs);
typedef INT (CALLBACK *MRUStringCmpFnW)(LPCWSTR lhs, LPCWSTR rhs);
typedef INT (CALLBACK *MRUBinaryCmpFn)(LPCVOID lhs, LPCVOID rhs, DWORD length);

struct MRUINFOA
{
    DWORD  cbSize;
    UINT   uMax;
    UINT   fFlags;
    HKEY   hKey;
    LPSTR  lpszSubKey;
    union
    {
        MRUStringCmpFnA string_cmpfn;
        MRUBinaryCmpFn  binary_cmpfn;
    } u;
};

struct MRUINFOW
{
    DWORD   cbSize;
    UINT    uMax;
    UINT    fFlags;
    HKEY    hKey;
    LPWSTR  lpszSubKey;
    union
    {
        MRUStringCmpFnW string_cmpfn;
        MRUBinaryCmpFn  binary_cmpfn;
    } u;
};

/* MRUINFO.fFlags */
#define MRU_STRING     0 /* list will contain strings */
#define MRU_BINARY     1 /* list will contain binary data */
#define MRU_CACHEWRITE 2 /* only save list order to reg. is FreeMRUList */

/* If list is a string list lpfnCompare has the following prototype
 * int CALLBACK MRUCompareString(LPCSTR s1, LPCSTR s2)
 * for binary lists the prototype is
 * int CALLBACK MRUCompareBinary(LPCVOID data1, LPCVOID data2, DWORD cbData)
 * where cbData is the no. of bytes to compare.
 * Need to check what return value means identical - 0?
 */

typedef struct tagWINEMRUITEM
{
    DWORD          size;        /* size of data stored               */
    DWORD          itemFlag;    /* flags                             */
    BYTE           datastart;
} WINEMRUITEM, *LPWINEMRUITEM;

/* itemFlag */
#define WMRUIF_CHANGED   0x0001 /* this dataitem changed             */

typedef struct tagWINEMRULIST
{
    struct MRUINFOW extview;    /* original create information       */
    BOOL           isUnicode;   /* is compare fn Unicode */
    DWORD          wineFlags;   /* internal flags                    */
    DWORD          cursize;     /* current size of realMRU           */
    LPWSTR         realMRU;     /* pointer to string of index names  */
    LPWINEMRUITEM  *array;      /* array of pointers to data         */
                                /* in 'a' to 'z' order               */
} WINEMRULIST, *LPWINEMRULIST;

/* wineFlags */
#define WMRUF_CHANGED  0x0001   /* MRU list has changed              */

/**************************************************************************
 *              MRU_SaveChanged (internal)
 *
 * Local MRU saving code
 */
static void MRU_SaveChanged(WINEMRULIST *mp)
{
    UINT i, err;
    HKEY newkey;
    WCHAR realname[2];
    WINEMRUITEM *witem;

    /* or should we do the following instead of RegOpenKeyEx:
     */

    /* open the sub key */
    if ((err = RegOpenKeyExW(mp->extview.hKey, mp->extview.lpszSubKey, 0, KEY_WRITE, &newkey)))
    {
        /* not present - what to do ??? */
        ERR("Could not open key, error=%d, attempting to create\n", err);
        if ((err = RegCreateKeyExW( mp->extview.hKey, mp->extview.lpszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_READ | KEY_WRITE, 0, &newkey, 0)))
        {
            ERR("failed to create key /%s/, err=%d\n", debugstr_w(mp->extview.lpszSubKey), err);
            return;
        }
    }

    if (mp->wineFlags & WMRUF_CHANGED)
    {
        mp->wineFlags &= ~WMRUF_CHANGED;
        if ((err = RegSetValueExW(newkey, strMRUList, 0, REG_SZ, (BYTE *)mp->realMRU,
                (lstrlenW(mp->realMRU) + 1)*sizeof(WCHAR))))
        {
            ERR("error saving MRUList, err=%d\n", err);
        }
        TRACE("saving MRUList=/%s/\n", debugstr_w(mp->realMRU));
    }

    realname[1] = 0;
    for (i = 0; i < mp->cursize; ++i)
    {
        witem = mp->array[i];
        if (witem->itemFlag & WMRUIF_CHANGED)
        {
            witem->itemFlag &= ~WMRUIF_CHANGED;
            realname[0] = 'a' + i;
            if ((err = RegSetValueExW(newkey, realname, 0, (mp->extview.fFlags & MRU_BINARY) ?
                    REG_BINARY : REG_SZ, &witem->datastart, witem->size)))
            {
                ERR("error saving /%s/, err=%d\n", debugstr_w(realname), err);
            }
            TRACE("saving value for name /%s/ size %ld\n", debugstr_w(realname), witem->size);
        }
    }
    RegCloseKey(newkey);
}

/**************************************************************************
 *              FreeMRUList [COMCTL32.152]
 *
 * Frees a most-recently-used items list.
 */
void WINAPI FreeMRUList(HANDLE hMRUList)
{
    WINEMRULIST *mp = hMRUList;
    unsigned int i;

    TRACE("%p.\n", hMRUList);

    if (!hMRUList)
        return;

    if (mp->wineFlags & WMRUF_CHANGED)
    {
        /* need to open key and then save the info */
        MRU_SaveChanged(mp);
    }

    for (i = 0; i < mp->extview.uMax; ++i)
        Free(mp->array[i]);

    Free(mp->realMRU);
    Free(mp->array);
    Free(mp->extview.lpszSubKey);
    Free(mp);
}

/**************************************************************************
 *                  FindMRUData [COMCTL32.169]
 *
 * Searches binary list for item that matches data of given length.
 * Returns position in list order 0 -> MRU and value corresponding to item's reg.
 * name will be stored in it ('a' -> 0).
 *
 */
INT WINAPI FindMRUData(HANDLE hList, const void *data, DWORD cbData, int *pos)
{
    const WINEMRULIST *mp = hList;
    INT ret;
    UINT i;
    LPSTR dataA = NULL;

    if (!mp || !mp->extview.u.string_cmpfn)
        return -1;

    if (!(mp->extview.fFlags & MRU_BINARY) && !mp->isUnicode)
    {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, data, -1, NULL, 0, NULL, NULL);
        dataA = Alloc(len);
        WideCharToMultiByte(CP_ACP, 0, data, -1, dataA, len, NULL, NULL);
    }

    for (i = 0; i < mp->cursize; ++i)
    {
        if (mp->extview.fFlags & MRU_BINARY)
        {
            if (!mp->extview.u.binary_cmpfn(data, &mp->array[i]->datastart, cbData))
                break;
        }
        else
        {
            if (mp->isUnicode)
            {
                if (!mp->extview.u.string_cmpfn(data, (LPWSTR)&mp->array[i]->datastart))
                    break;
            }
            else
            {
                DWORD len = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&mp->array[i]->datastart, -1,
                        NULL, 0, NULL, NULL);
                LPSTR itemA = Alloc(len);
                INT cmp;
                WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&mp->array[i]->datastart, -1, itemA, len, NULL, NULL);

                cmp = mp->extview.u.string_cmpfn((LPWSTR)dataA, (LPWSTR)itemA);
                Free(itemA);
                if (!cmp)
                    break;
            }
        }
    }

    Free(dataA);
    if (i < mp->cursize)
        ret = i;
    else
        ret = -1;
    if (pos && (ret != -1))
        *pos = 'a' + i;

    TRACE("%p, %p, %ld, %p, returning %d.\n", hList, data, cbData, pos, ret);

    return ret;
}

/**************************************************************************
 *              AddMRUData [COMCTL32.167]
 *
 * Add item to MRU binary list.  If item already exists in list then it is
 * simply moved up to the top of the list and not added again.  If list is
 * full then the least recently used item is removed to make room.
 *
 */
INT WINAPI AddMRUData(HANDLE hList, const void *data, DWORD cbData)
{
    WINEMRULIST *mp = hList;
    WINEMRUITEM *witem;
    INT i, replace;

    if ((replace = FindMRUData(hList, data, cbData, NULL)) >= 0)
    {
        /* Item exists, just move it to the front */
        LPWSTR pos = wcschr(mp->realMRU, replace + 'a');
        while (pos > mp->realMRU)
        {
            pos[0] = pos[-1];
            pos--;
        }
    }
    else
    {
        /* either add a new entry or replace oldest */
        if (mp->cursize < mp->extview.uMax)
        {
            /* Add in a new item */
            replace = mp->cursize;
            mp->cursize++;
        }
        else
        {
            /* get the oldest entry and replace data */
            replace = mp->realMRU[mp->cursize - 1] - 'a';
            Free(mp->array[replace]);
        }

        /* Allocate space for new item and move in the data */
        mp->array[replace] = witem = Alloc(cbData + sizeof(WINEMRUITEM));
        witem->itemFlag |= WMRUIF_CHANGED;
        witem->size = cbData;
        memcpy( &witem->datastart, data, cbData);

        /* now rotate MRU list */
        for (i = mp->cursize - 1; i >= 1; --i)
            mp->realMRU[i] = mp->realMRU[i-1];
    }

    /* The new item gets the front spot */
    mp->wineFlags |= WMRUF_CHANGED;
    mp->realMRU[0] = replace + 'a';

    TRACE("%p, %p, %ld adding data, /%c/ now most current\n", hList, data, cbData, replace+'a');

    if (!(mp->extview.fFlags & MRU_CACHEWRITE))
    {
        /* save changed stuff right now */
        MRU_SaveChanged(mp);
    }

    return replace;
}

/**************************************************************************
 *              AddMRUStringW [COMCTL32.401]
 *
 * Add an item to an MRU string list.
 *
 */
INT WINAPI AddMRUStringW(HANDLE hList, const WCHAR *str)
{
    TRACE("%p, %s.\n", hList, debugstr_w(str));

    if (!hList)
        return -1;

    if (!str || IsBadStringPtrW(str, -1))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    return AddMRUData(hList, str, (lstrlenW(str) + 1) * sizeof(WCHAR));
}

/**************************************************************************
 *              AddMRUStringA [COMCTL32.153]
 */
INT WINAPI AddMRUStringA(HANDLE hList, const char *str)
{
    WCHAR *strW;
    DWORD len;
    INT ret;

    TRACE("%p, %s.\n", hList, debugstr_a(str));

    if (!hList)
        return -1;

    if (IsBadStringPtrA(str, -1))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0) * sizeof(WCHAR);
    strW = Alloc(len);
    if (!strW)
        return -1;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len/sizeof(WCHAR));
    ret = AddMRUData(hList, strW, len);
    Free(strW);
    return ret;
}

/**************************************************************************
 *              DelMRUString [COMCTL32.156]
 *
 * Removes item from either string or binary list (despite its name)
 *
 * PARAMS
 *    hList [I] list handle
 *    nItemPos [I] item position to remove 0 -> MRU
 *
 * RETURNS
 *    TRUE if successful, FALSE if nItemPos is out of range.
 */
BOOL WINAPI DelMRUString(HANDLE hList, INT nItemPos)
{
    FIXME("(%p, %d): stub\n", hList, nItemPos);
    return TRUE;
}

/**************************************************************************
 *                  FindMRUStringW [COMCTL32.402]
 */
INT WINAPI FindMRUStringW(HANDLE hList, const WCHAR *str, int *pos)
{
    return FindMRUData(hList, str, (lstrlenW(str) + 1) * sizeof(WCHAR), pos);
}

/**************************************************************************
 *                  FindMRUStringA [COMCTL32.155]
 *
 * Searches string list for item that matches given string.
 *
 * RETURNS
 *    Position in list 0 -> MRU.  -1 if item not found.
 */
INT WINAPI FindMRUStringA(HANDLE hList, const char *str, int *pos)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    WCHAR *strW = Alloc(len * sizeof(*strW));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len);
    ret = FindMRUData(hList, strW, len * sizeof(WCHAR), pos);
    Free(strW);
    return ret;
}

/*************************************************************************
 *                 create_mru_list (internal)
 */
static HANDLE create_mru_list(WINEMRULIST *mp)
{
    UINT i, err;
    HKEY newkey;
    DWORD datasize, dwdisp;
    WCHAR realname[2];
    WINEMRUITEM *witem;
    DWORD type;

    /* get space to save indices that will turn into names
     * but in order of most to least recently used
     */
    mp->realMRU = Alloc((mp->extview.uMax + 2) * sizeof(WCHAR));

    /* get space to save pointers to actual data in order of
     * 'a' to 'z' (0 to n).
     */
    mp->array = Alloc(mp->extview.uMax * sizeof(LPVOID));

    /* open the sub key */
    if ((err = RegCreateKeyExW(mp->extview.hKey, mp->extview.lpszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_READ | KEY_WRITE, 0, &newkey, &dwdisp)))
    {
        /* error - what to do ??? */
        ERR("%lu, %u, %x, %p, %s, %p: Could not open key, error=%d\n", mp->extview.cbSize, mp->extview.uMax, mp->extview.fFlags,
                mp->extview.hKey, debugstr_w(mp->extview.lpszSubKey), mp->extview.u.string_cmpfn, err);
        return 0;
    }

    /* get values from key 'MRUList' */
    if (newkey)
    {
        datasize = (mp->extview.uMax + 1) * sizeof(WCHAR);
        if (RegQueryValueExW( newkey, strMRUList, 0, &type, (BYTE *)mp->realMRU, &datasize))
        {
            /* not present - set size to 1 (will become 0 later) */
            datasize = 1;
            *mp->realMRU = 0;
        }
        else
            datasize /= sizeof(WCHAR);

        TRACE("MRU list = %s, datasize = %ld\n", debugstr_w(mp->realMRU), datasize);

        mp->cursize = datasize - 1;
        /* datasize now has number of items in the MRUList */

        /* get actual values for each entry */
        realname[1] = 0;
        for (i = 0; i < mp->cursize; ++i)
        {
            realname[0] = 'a' + i;
            if (RegQueryValueExW(newkey, realname, 0, &type, 0, &datasize))
            {
                /* not present - what to do ??? */
                ERR("Key %s not found 1\n", debugstr_w(realname));
            }
            mp->array[i] = witem = Alloc(datasize + sizeof(WINEMRUITEM));
            witem->size = datasize;
            if (RegQueryValueExW(newkey, realname, 0, &type, &witem->datastart, &datasize))
            {
                /* not present - what to do ??? */
                ERR("Key %s not found 2\n", debugstr_w(realname));
            }
        }
        RegCloseKey( newkey );
    }
    else
        mp->cursize = 0;

    TRACE("%lu, %u, %x, %p, %s, %p: Current Size = %ld\n", mp->extview.cbSize, mp->extview.uMax, mp->extview.fFlags,
            mp->extview.hKey, debugstr_w(mp->extview.lpszSubKey), mp->extview.u.string_cmpfn, mp->cursize);
    return mp;
}

/**************************************************************************
 *                  CreateMRUListLazyW [COMCTL32.404]
 */
HANDLE WINAPI CreateMRUListLazyW(const struct MRUINFOW *info, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4)
{
    WINEMRULIST *mp;

    /* Native does not check for a NULL. */
    if (!info->hKey || IsBadStringPtrW(info->lpszSubKey, -1))
        return NULL;

    mp = Alloc(sizeof(*mp));
    memcpy(&mp->extview, info, sizeof(*info));
    mp->extview.lpszSubKey = Alloc((lstrlenW(info->lpszSubKey) + 1) * sizeof(WCHAR));
    lstrcpyW(mp->extview.lpszSubKey, info->lpszSubKey);
    mp->isUnicode = TRUE;

    return create_mru_list(mp);
}

/**************************************************************************
 *                  CreateMRUListLazyA [COMCTL32.157]
 *
 * Creates a most-recently-used list.
 */
HANDLE WINAPI CreateMRUListLazyA(const struct MRUINFOA *info, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4)
{
    WINEMRULIST *mp;
    DWORD len;

    /* Native does not check for a NULL lpcml */

    if (!info->hKey || IsBadStringPtrA(info->lpszSubKey, -1))
        return 0;

    mp = Alloc(sizeof(*mp));
    memcpy(&mp->extview, info, sizeof(*info));
    len = MultiByteToWideChar(CP_ACP, 0, info->lpszSubKey, -1, NULL, 0);
    mp->extview.lpszSubKey = Alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, info->lpszSubKey, -1, mp->extview.lpszSubKey, len);
    mp->isUnicode = FALSE;
    return create_mru_list(mp);
}

/**************************************************************************
 *              CreateMRUListW [COMCTL32.400]
 */
HANDLE WINAPI CreateMRUListW(const struct MRUINFOW *info)
{
    return CreateMRUListLazyW(info, 0, 0, 0);
}

/**************************************************************************
 *              CreateMRUListA [COMCTL32.151]
 */
HANDLE WINAPI CreateMRUListA(const struct MRUINFOA *info)
{
     return CreateMRUListLazyA(info, 0, 0, 0);
}

/**************************************************************************
 *                EnumMRUListW [COMCTL32.403]
 *
 * Enumerate item in a most-recently-used list
 *
 * PARAMS
 *    hList [I] list handle
 *    nItemPos [I] item position to enumerate
 *    lpBuffer [O] buffer to receive item
 *    nBufferSize [I] size of buffer
 *
 * RETURNS
 *    For binary lists specifies how many bytes were copied to buffer, for
 *    string lists specifies full length of string.  Enumerating past the end
 *    of list returns -1.
 *    If lpBuffer == NULL or nItemPos is -ve return value is no. of items in
 *    the list.
 */
INT WINAPI EnumMRUListW(HANDLE hList, INT nItemPos, void *buffer, DWORD nBufferSize)
{
    const WINEMRULIST *mp = hList;
    const WINEMRUITEM *witem;
    INT desired, datasize;

    if (!mp) return -1;
    if ((nItemPos < 0) || !buffer) return mp->cursize;
    if (nItemPos >= mp->cursize) return -1;
    desired = mp->realMRU[nItemPos];
    desired -= 'a';
    TRACE("nItemPos=%d, desired=%d\n", nItemPos, desired);
    witem = mp->array[desired];
    datasize = min(witem->size, nBufferSize);
    memcpy(buffer, &witem->datastart, datasize);
    TRACE("(%p, %d, %p, %ld): returning len %d\n", hList, nItemPos, buffer, nBufferSize, datasize);
    return datasize;
}

/**************************************************************************
 *                EnumMRUListA [COMCTL32.154]
 */
INT WINAPI EnumMRUListA(HANDLE hList, INT nItemPos, void *buffer, DWORD nBufferSize)
{
    const WINEMRULIST *mp = hList;
    WINEMRUITEM *witem;
    INT desired, datasize;
    DWORD lenA;

    if (!mp) return -1;
    if ((nItemPos < 0) || !buffer) return mp->cursize;
    if (nItemPos >= mp->cursize) return -1;
    desired = mp->realMRU[nItemPos];
    desired -= 'a';
    TRACE("nItemPos=%d, desired=%d\n", nItemPos, desired);
    witem = mp->array[desired];
    if (mp->extview.fFlags & MRU_BINARY)
    {
        datasize = min(witem->size, nBufferSize);
        memcpy(buffer, &witem->datastart, datasize);
    }
    else
    {
        lenA = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&witem->datastart, -1, NULL, 0, NULL, NULL);
        datasize = min(lenA, nBufferSize);
        WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&witem->datastart, -1, buffer, datasize, NULL, NULL);
        ((char *)buffer)[ datasize - 1 ] = '\0';
        datasize = lenA - 1;
    }
    TRACE("(%p, %d, %p, %ld): returning len=%d\n", hList, nItemPos, buffer, nBufferSize, datasize);
    return datasize;
}

/**************************************************************************
 * Str_GetPtrWtoA [internal]
 *
 * Converts a unicode string into a multi byte string
 *
 */

INT Str_GetPtrWtoA(const WCHAR *src, char *dst, INT nMaxLen)
{
    INT len;

    TRACE("%s, %p, %d.\n", debugstr_w(src), dst, nMaxLen);

    if (!dst && src)
        return WideCharToMultiByte(CP_ACP, 0, src, -1, 0, 0, NULL, NULL);

    if (!nMaxLen)
        return 0;

    if (!src)
    {
        dst[0] = 0;
        return 0;
    }

    len = WideCharToMultiByte(CP_ACP, 0, src, -1, 0, 0, NULL, NULL);
    if (len >= nMaxLen)
        len = nMaxLen - 1;

    WideCharToMultiByte(CP_ACP, 0, src, -1, dst, len, NULL, NULL);
    dst[len] = '\0';

    return len;
}

/**************************************************************************
 * Str_GetPtrAtoW [internal]
 *
 * Converts a multibyte string into a unicode string
 */

INT Str_GetPtrAtoW(const char *src, WCHAR *dst, INT nMaxLen)
{
    INT len;

    TRACE("%s, %p, %d.\n", debugstr_a(src), dst, nMaxLen);

    if (!dst && src)
        return MultiByteToWideChar(CP_ACP, 0, src, -1, 0, 0);

    if (!nMaxLen)
        return 0;

    if (!src)
    {
        *dst = 0;
        return 0;
    }

    len = MultiByteToWideChar(CP_ACP, 0, src, -1, 0, 0);
    if (len >= nMaxLen)
        len = nMaxLen - 1;

    MultiByteToWideChar(CP_ACP, 0, src, -1, dst, len);
    dst[len] = 0;

    return len;
}

/**************************************************************************
 * Str_SetPtrAtoW [internal]
 *
 * Converts a multi byte string to a unicode string.
 * If the pointer to the destination buffer is NULL a buffer is allocated.
 * If the destination buffer is too small to keep the converted multi byte
 * string the destination buffer is reallocated. If the source pointer is
 */
BOOL Str_SetPtrAtoW(WCHAR **dst, const char *src)
{
    TRACE("%p, %s.\n", dst, debugstr_a(src));

    if (src)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
        LPWSTR ptr = ReAlloc(*dst, len * sizeof(**dst));

        if (!ptr)
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, src, -1, ptr, len);
        *dst = ptr;
    }
    else
    {
        Free(*dst);
        *dst = NULL;
    }

    return TRUE;
}

/**************************************************************************
 * Str_SetPtrWtoA [internal]
 *
 * Converts a unicode string to a multi byte string.
 * If the pointer to the destination buffer is NULL a buffer is allocated.
 * If the destination buffer is too small to keep the converted wide
 * string the destination buffer is reallocated. If the source pointer is
 * NULL, the destination buffer is freed.
 */
BOOL Str_SetPtrWtoA(char **dst, const WCHAR *src)
{
    TRACE("%p, %s.\n", dst, debugstr_w(src));

    if (src)
    {
        INT len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, FALSE);
        LPSTR ptr = ReAlloc(*dst, len * sizeof(**dst));

        if (!ptr)
            return FALSE;
        WideCharToMultiByte(CP_ACP, 0, src, -1, ptr, len, NULL, FALSE);
        *dst = ptr;
    }
    else
    {
        Free(*dst);
        *dst = NULL;
    }

    return TRUE;
}

/**************************************************************************
 * Notification functions
 */

struct NOTIFYDATA
{
    HWND hwndFrom;
    HWND hwndTo;
    DWORD  dwParam3;
    DWORD  dwParam4;
    DWORD  dwParam5;
    DWORD  dwParam6;
};

/**************************************************************************
 * DoNotify [Internal]
 */

static LRESULT DoNotify(const struct NOTIFYDATA *notify, UINT code, NMHDR *hdr)
{
    NMHDR nmhdr;
    NMHDR *lpNmh = NULL;
    UINT idFrom = 0;

    TRACE("%p, %p, %d, %p, %#lx.\n", notify->hwndFrom, notify->hwndTo, code, hdr, notify->dwParam5);

    if (!notify->hwndTo)
        return 0;

    if (notify->hwndFrom == (HWND)-1)
    {
        lpNmh = hdr;
        idFrom = hdr->idFrom;
    }
    else
    {
        if (notify->hwndFrom)
            idFrom = GetDlgCtrlID(notify->hwndFrom);

        lpNmh = hdr ? hdr : &nmhdr;
        lpNmh->hwndFrom = notify->hwndFrom;
        lpNmh->idFrom = idFrom;
        lpNmh->code = code;
    }

    return SendMessageW(notify->hwndTo, WM_NOTIFY, idFrom, (LPARAM)lpNmh);
}

/**************************************************************************
 * SendNotify [COMCTL32.341]
 *
 * Sends a WM_NOTIFY message to the specified window.
 *
 */
LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, UINT code, NMHDR *hdr)
{
    struct NOTIFYDATA notify;

    TRACE("%p, %p, %d, %p.\n", hwndTo, hwndFrom, code, hdr);

    notify.hwndFrom = hwndFrom;
    notify.hwndTo   = hwndTo;
    notify.dwParam5 = 0;
    notify.dwParam6 = 0;

    return DoNotify(&notify, code, hdr);
}

/**************************************************************************
 * SendNotifyEx [COMCTL32.342]
 *
 * Sends a WM_NOTIFY message to the specified window.
 *
 * PARAMS
 *     hwndFrom [I] Window to receive the message
 *     hwndTo   [I] Window that the message is from
 *     code     [I] Notification code
 *     hdr      [I] The NMHDR and any additional information to send or NULL
 *     dwParam5 [I] Unknown
 *
 * RETURNS
 *     Success: return value from notification
 *     Failure: 0
 *
 * NOTES
 *     If hwndFrom is -1 then the identifier of the control sending the
 *     message is taken from the NMHDR structure.
 *     If hwndFrom is not -1 then lpHdr can be NULL.
 */
LRESULT WINAPI SendNotifyEx(HWND hwndTo, HWND hwndFrom, UINT code, NMHDR *hdr, DWORD dwParam5)
{
    struct NOTIFYDATA notify;
    HWND hwndNotify;

    TRACE("%p, %p, %d, %p, %#lx\n", hwndFrom, hwndTo, code, hdr, dwParam5);

    hwndNotify = hwndTo;
    if (!hwndTo)
    {
        if (IsWindow(hwndFrom))
        {
            hwndNotify = GetParent(hwndFrom);
            if (!hwndNotify)
                return 0;
        }
    }

    notify.hwndFrom = hwndFrom;
    notify.hwndTo   = hwndNotify;
    notify.dwParam5 = dwParam5;
    notify.dwParam6 = 0;

    return DoNotify(&notify, code, hdr);
}

/* reallocate *array if needed so it can hold at least count items */
/* *size measured in bytes, count measured in items */
/* return FALSE means failed (*array still valid but too small) */
/* return TRUE means successful */
static BOOL COMCTL32_array_reserve(void **array, DWORD *size, DWORD count, DWORD item_size)
{
    void *new_array;
    DWORD needed_size;

    if (count > (DWORD)-1 / item_size)
        return FALSE;
    needed_size = count * item_size;
    if (*size >= needed_size)
        return TRUE;
    new_array = *array ? ReAlloc(*array, needed_size) : Alloc(needed_size);
    if (!new_array)
        return FALSE;
    *array = new_array;
    *size = needed_size;
    return TRUE;
}

/* Text field conversion behavior flags for COMCTL32_SendConvertedNotify() */
enum conversion_flags
{
    /* Convert Unicode text to ANSI for parent before sending. If not set, do nothing */
    CONVERT_SEND = 0x01,
    /* Convert ANSI text from parent back to Unicode for children */
    CONVERT_RECEIVE = 0x02,
    /* Send empty text to parent if text is NULL. Original text pointer still remains NULL */
    SEND_EMPTY_IF_NULL = 0x04,
    /* Set text to null after parent received the notification if the required mask is not set before sending notification */
    SET_NULL_IF_NO_MASK = 0x08,
    /* Zero out the text buffer before sending it to parent */
    ZERO_SEND = 0x10
};

static UINT COMCTL32_GetAnsiNtfCode(UINT code)
{
    switch (code)
    {
    /* ComboxBoxEx */
    case CBEN_DRAGBEGINW: return CBEN_DRAGBEGINA;
    case CBEN_ENDEDITW: return CBEN_ENDEDITA;
    case CBEN_GETDISPINFOW: return CBEN_GETDISPINFOA;
    /* Date and Time Picker */
    case DTN_FORMATW: return DTN_FORMATA;
    case DTN_FORMATQUERYW: return DTN_FORMATQUERYA;
    case DTN_USERSTRINGW: return DTN_USERSTRINGA;
    case DTN_WMKEYDOWNW: return DTN_WMKEYDOWNA;
    /* Header */
    case HDN_BEGINTRACKW: return HDN_BEGINTRACKA;
    case HDN_DIVIDERDBLCLICKW: return HDN_DIVIDERDBLCLICKA;
    case HDN_ENDTRACKW: return HDN_ENDTRACKA;
    case HDN_GETDISPINFOW: return HDN_GETDISPINFOA;
    case HDN_ITEMCHANGEDW: return HDN_ITEMCHANGEDA;
    case HDN_ITEMCHANGINGW: return HDN_ITEMCHANGINGA;
    case HDN_ITEMCLICKW: return HDN_ITEMCLICKA;
    case HDN_ITEMDBLCLICKW: return HDN_ITEMDBLCLICKA;
    case HDN_TRACKW: return HDN_TRACKA;
    /* List View */
    case LVN_BEGINLABELEDITW: return LVN_BEGINLABELEDITA;
    case LVN_ENDLABELEDITW: return LVN_ENDLABELEDITA;
    case LVN_GETDISPINFOW: return LVN_GETDISPINFOA;
    case LVN_GETINFOTIPW: return LVN_GETINFOTIPA;
    case LVN_INCREMENTALSEARCHW: return LVN_INCREMENTALSEARCHA;
    case LVN_ODFINDITEMW: return LVN_ODFINDITEMA;
    case LVN_SETDISPINFOW: return LVN_SETDISPINFOA;
    /* Toolbar */
    case TBN_GETBUTTONINFOW: return TBN_GETBUTTONINFOA;
    case TBN_GETINFOTIPW: return TBN_GETINFOTIPA;
    /* Tooltip */
    case TTN_GETDISPINFOW: return TTN_GETDISPINFOA;
    /* Tree View */
    case TVN_BEGINDRAGW: return TVN_BEGINDRAGA;
    case TVN_BEGINLABELEDITW: return TVN_BEGINLABELEDITA;
    case TVN_BEGINRDRAGW: return TVN_BEGINRDRAGA;
    case TVN_DELETEITEMW: return TVN_DELETEITEMA;
    case TVN_ENDLABELEDITW: return TVN_ENDLABELEDITA;
    case TVN_GETDISPINFOW: return TVN_GETDISPINFOA;
    case TVN_GETINFOTIPW: return TVN_GETINFOTIPA;
    case TVN_ITEMEXPANDEDW: return TVN_ITEMEXPANDEDA;
    case TVN_ITEMEXPANDINGW: return TVN_ITEMEXPANDINGA;
    case TVN_SELCHANGEDW: return TVN_SELCHANGEDA;
    case TVN_SELCHANGINGW: return TVN_SELCHANGINGA;
    case TVN_SETDISPINFOW: return TVN_SETDISPINFOA;
    }
    return code;
}

/* Convert text to Unicode and return the original text address */
static WCHAR *COMCTL32_ConvertText(WCHAR **text)
{
    WCHAR *oldText = *text;
    *text = NULL;
    Str_SetPtrWtoA((CHAR **)text, oldText);
    return oldText;
}

static void COMCTL32_RestoreText(WCHAR **text, WCHAR *oldText)
{
    if (!oldText) return;

    Free(*text);
    *text = oldText;
}

static LRESULT COMCTL32_SendConvertedNotify(HWND hwndNotify, NMHDR *hdr, UINT *mask, UINT requiredMask, WCHAR **text,
                                            INT *textMax, DWORD flags)
{
    CHAR *sendBuffer = NULL;
    CHAR *receiveBuffer;
    INT bufferSize;
    WCHAR *oldText;
    INT oldTextMax;
    LRESULT ret = NO_ERROR;

    oldText = *text;
    oldTextMax = textMax ? *textMax : 0;

    hdr->code = COMCTL32_GetAnsiNtfCode(hdr->code);

    if (mask && !(*mask & requiredMask))
    {
        ret = SendMessageW(hwndNotify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
        if (flags & SET_NULL_IF_NO_MASK) oldText = NULL;
        goto done;
    }

    if (oldTextMax < 0) goto done;

    if ((*text && flags & (CONVERT_SEND | ZERO_SEND)) || (!*text && flags & SEND_EMPTY_IF_NULL))
    {
        bufferSize = textMax ? *textMax : lstrlenW(*text) + 1;
        sendBuffer = Alloc(bufferSize);
        if (!sendBuffer) goto done;
        if (!(flags & ZERO_SEND)) WideCharToMultiByte(CP_ACP, 0, *text, -1, sendBuffer, bufferSize, NULL, FALSE);
        *text = (WCHAR *)sendBuffer;
    }

    ret = SendMessageW(hwndNotify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);

    if (*text && oldText && (flags & CONVERT_RECEIVE))
    {
        /* MultiByteToWideChar requires that source and destination are not the same buffer */
        if (*text == oldText)
        {
            bufferSize = lstrlenA((CHAR *)*text)  + 1;
            receiveBuffer = Alloc(bufferSize);
            if (!receiveBuffer) goto done;
            memcpy(receiveBuffer, *text, bufferSize);
            MultiByteToWideChar(CP_ACP, 0, receiveBuffer, bufferSize, oldText, oldTextMax);
            Free(receiveBuffer);
        }
        else
            MultiByteToWideChar(CP_ACP, 0, (CHAR *)*text, -1, oldText, oldTextMax);
    }

done:
    Free(sendBuffer);
    *text = oldText;
    return ret;
}

LRESULT COMCTL32_forward_notify_to_ansi_window(HWND hwnd_notify, NMHDR *hdr, WCHAR **unicode_buffer, DWORD *unicode_buffer_size)
{
    LRESULT ret;

    switch (hdr->code)
    {
    /* ComboBoxEx */
    case CBEN_GETDISPINFOW:
    {
        NMCOMBOBOXEXW *nmcbe = (NMCOMBOBOXEXW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmcbe->ceItem.mask, CBEIF_TEXT, &nmcbe->ceItem.pszText,
                                            &nmcbe->ceItem.cchTextMax, ZERO_SEND | SET_NULL_IF_NO_MASK | CONVERT_RECEIVE);
    }
    case CBEN_DRAGBEGINW:
    {
        NMCBEDRAGBEGINW *nmdbW = (NMCBEDRAGBEGINW *)hdr;
        NMCBEDRAGBEGINA nmdbA = {{0}};
        nmdbA.hdr.code = COMCTL32_GetAnsiNtfCode(nmdbW->hdr.code);
        nmdbA.hdr.hwndFrom = nmdbW->hdr.hwndFrom;
        nmdbA.hdr.idFrom = nmdbW->hdr.idFrom;
        nmdbA.iItemid = nmdbW->iItemid;
        WideCharToMultiByte(CP_ACP, 0, nmdbW->szText, ARRAY_SIZE(nmdbW->szText), nmdbA.szText, ARRAY_SIZE(nmdbA.szText),
                            NULL, FALSE);
        return SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)&nmdbA);
    }
    case CBEN_ENDEDITW:
    {
        NMCBEENDEDITW *nmedW = (NMCBEENDEDITW *)hdr;
        NMCBEENDEDITA nmedA = {{0}};
        nmedA.hdr.code = COMCTL32_GetAnsiNtfCode(nmedW->hdr.code);
        nmedA.hdr.hwndFrom = nmedW->hdr.hwndFrom;
        nmedA.hdr.idFrom = nmedW->hdr.idFrom;
        nmedA.fChanged = nmedW->fChanged;
        nmedA.iNewSelection = nmedW->iNewSelection;
        nmedA.iWhy = nmedW->iWhy;
        WideCharToMultiByte(CP_ACP, 0, nmedW->szText, ARRAY_SIZE(nmedW->szText), nmedA.szText, ARRAY_SIZE(nmedA.szText),
                            NULL, FALSE);
        return SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)&nmedA);
    }
    /* Date and Time Picker */
    case DTN_FORMATW:
    {
        NMDATETIMEFORMATW *nmdtf = (NMDATETIMEFORMATW *)hdr;
        WCHAR *oldFormat;
        DWORD textLength;

        hdr->code = COMCTL32_GetAnsiNtfCode(hdr->code);
        oldFormat = COMCTL32_ConvertText((WCHAR **)&nmdtf->pszFormat);
        ret = SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)nmdtf);
        COMCTL32_RestoreText((WCHAR **)&nmdtf->pszFormat, oldFormat);

        if (nmdtf->pszDisplay)
        {
            textLength = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)nmdtf->pszDisplay, -1, 0, 0);
            if (!COMCTL32_array_reserve((void **)unicode_buffer, unicode_buffer_size, textLength, sizeof(WCHAR))) return ret;
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)nmdtf->pszDisplay, -1, *unicode_buffer, textLength);
            if (nmdtf->pszDisplay != nmdtf->szDisplay)
                nmdtf->pszDisplay = *unicode_buffer;
            else
            {
                textLength = min(textLength, ARRAY_SIZE(nmdtf->szDisplay));
                memcpy(nmdtf->szDisplay, *unicode_buffer, textLength * sizeof(WCHAR));
            }
        }

        return ret;
    }
    case DTN_FORMATQUERYW:
    {
        NMDATETIMEFORMATQUERYW *nmdtfq = (NMDATETIMEFORMATQUERYW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, NULL, 0, (WCHAR **)&nmdtfq->pszFormat, NULL, CONVERT_SEND);
    }
    case DTN_WMKEYDOWNW:
    {
        NMDATETIMEWMKEYDOWNW *nmdtkd = (NMDATETIMEWMKEYDOWNW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, NULL, 0, (WCHAR **)&nmdtkd->pszFormat, NULL, CONVERT_SEND);
    }
    case DTN_USERSTRINGW:
    {
        NMDATETIMESTRINGW *nmdts = (NMDATETIMESTRINGW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, NULL, 0, (WCHAR **)&nmdts->pszUserString, NULL, CONVERT_SEND);
    }
    /* Header */
    case HDN_BEGINTRACKW:
    case HDN_DIVIDERDBLCLICKW:
    case HDN_ENDTRACKW:
    case HDN_ITEMCHANGEDW:
    case HDN_ITEMCHANGINGW:
    case HDN_ITEMCLICKW:
    case HDN_ITEMDBLCLICKW:
    case HDN_TRACKW:
    {
        NMHEADERW *nmh = (NMHEADERW *)hdr;
        WCHAR *oldText = NULL, *oldFilterText = NULL;
        HD_TEXTFILTERW *tf = NULL;

        hdr->code = COMCTL32_GetAnsiNtfCode(hdr->code);

        if (!nmh->pitem) return SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
        if (nmh->pitem->mask & HDI_TEXT) oldText = COMCTL32_ConvertText(&nmh->pitem->pszText);
        if ((nmh->pitem->mask & HDI_FILTER) && (nmh->pitem->type == HDFT_ISSTRING) && nmh->pitem->pvFilter)
        {
            tf = (HD_TEXTFILTERW *)nmh->pitem->pvFilter;
            oldFilterText = COMCTL32_ConvertText(&tf->pszText);
        }
        ret = SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
        COMCTL32_RestoreText(&nmh->pitem->pszText, oldText);
        if (tf) COMCTL32_RestoreText(&tf->pszText, oldFilterText);
        return ret;
    }
    case HDN_GETDISPINFOW:
    {
        NMHDDISPINFOW *nmhddi = (NMHDDISPINFOW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmhddi->mask, HDI_TEXT, &nmhddi->pszText, &nmhddi->cchTextMax,
                                            SEND_EMPTY_IF_NULL | CONVERT_SEND | CONVERT_RECEIVE);
    }
    /* List View */
    case LVN_BEGINLABELEDITW:
    case LVN_ENDLABELEDITW:
    case LVN_SETDISPINFOW:
    {
        NMLVDISPINFOW *nmlvdi = (NMLVDISPINFOW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmlvdi->item.mask, LVIF_TEXT, &nmlvdi->item.pszText,
                                            &nmlvdi->item.cchTextMax, SET_NULL_IF_NO_MASK | CONVERT_SEND | CONVERT_RECEIVE);
    }
    case LVN_GETDISPINFOW:
    {
        NMLVDISPINFOW *nmlvdi = (NMLVDISPINFOW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmlvdi->item.mask, LVIF_TEXT, &nmlvdi->item.pszText,
                                            &nmlvdi->item.cchTextMax, CONVERT_RECEIVE);
    }
    case LVN_GETINFOTIPW:
    {
        NMLVGETINFOTIPW *nmlvgit = (NMLVGETINFOTIPW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, NULL, 0, &nmlvgit->pszText, &nmlvgit->cchTextMax,
                                            CONVERT_SEND | CONVERT_RECEIVE);
    }
    case LVN_INCREMENTALSEARCHW:
    case LVN_ODFINDITEMW:
    {
        NMLVFINDITEMW *nmlvfi = (NMLVFINDITEMW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmlvfi->lvfi.flags, LVFI_STRING | LVFI_SUBSTRING,
                                            (WCHAR **)&nmlvfi->lvfi.psz, NULL, CONVERT_SEND);
    }
    /* Toolbar */
    case TBN_GETBUTTONINFOW:
    {
        NMTOOLBARW *nmtb = (NMTOOLBARW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, NULL, 0, &nmtb->pszText, &nmtb->cchText,
                                            SEND_EMPTY_IF_NULL | CONVERT_SEND | CONVERT_RECEIVE);
    }
    case TBN_GETINFOTIPW:
    {
        NMTBGETINFOTIPW *nmtbgit = (NMTBGETINFOTIPW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, NULL, 0, &nmtbgit->pszText, &nmtbgit->cchTextMax, CONVERT_RECEIVE);
    }
    /* Tooltip */
    case TTN_GETDISPINFOW:
    {
        NMTTDISPINFOW *nmttdiW = (NMTTDISPINFOW *)hdr;
        NMTTDISPINFOA nmttdiA = {{0}};
        DWORD size;

        nmttdiA.hdr.code = COMCTL32_GetAnsiNtfCode(nmttdiW->hdr.code);
        nmttdiA.hdr.hwndFrom = nmttdiW->hdr.hwndFrom;
        nmttdiA.hdr.idFrom = nmttdiW->hdr.idFrom;
        nmttdiA.hinst = nmttdiW->hinst;
        nmttdiA.uFlags = nmttdiW->uFlags;
        nmttdiA.lParam = nmttdiW->lParam;
        nmttdiA.lpszText = nmttdiA.szText;
        WideCharToMultiByte(CP_ACP, 0, nmttdiW->szText, ARRAY_SIZE(nmttdiW->szText), nmttdiA.szText,
                            ARRAY_SIZE(nmttdiA.szText), NULL, FALSE);

        ret = SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)&nmttdiA);

        nmttdiW->hinst = nmttdiA.hinst;
        nmttdiW->uFlags = nmttdiA.uFlags;
        nmttdiW->lParam = nmttdiA.lParam;

        MultiByteToWideChar(CP_ACP, 0, nmttdiA.szText, ARRAY_SIZE(nmttdiA.szText), nmttdiW->szText,
                            ARRAY_SIZE(nmttdiW->szText));
        if (!nmttdiA.lpszText)
            nmttdiW->lpszText = nmttdiW->szText;
        else if (!IS_INTRESOURCE(nmttdiA.lpszText))
        {
            size = MultiByteToWideChar(CP_ACP, 0, nmttdiA.lpszText, -1, 0, 0);
            if (size > ARRAY_SIZE(nmttdiW->szText))
            {
                if (!COMCTL32_array_reserve((void **)unicode_buffer, unicode_buffer_size, size, sizeof(WCHAR))) return ret;
                MultiByteToWideChar(CP_ACP, 0, nmttdiA.lpszText, -1, *unicode_buffer, size);
                nmttdiW->lpszText = *unicode_buffer;
                /* Override content in szText */
                memcpy(nmttdiW->szText, nmttdiW->lpszText, min(sizeof(nmttdiW->szText), size * sizeof(WCHAR)));
            }
            else
            {
                MultiByteToWideChar(CP_ACP, 0, nmttdiA.lpszText, -1, nmttdiW->szText, ARRAY_SIZE(nmttdiW->szText));
                nmttdiW->lpszText = nmttdiW->szText;
            }
        }
        else
        {
            nmttdiW->szText[0] = 0;
            nmttdiW->lpszText = (WCHAR *)nmttdiA.lpszText;
        }

        return ret;
    }
    /* Tree View */
    case TVN_BEGINDRAGW:
    case TVN_BEGINRDRAGW:
    case TVN_ITEMEXPANDEDW:
    case TVN_ITEMEXPANDINGW:
    {
        NMTREEVIEWW *nmtv = (NMTREEVIEWW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmtv->itemNew.mask, TVIF_TEXT, &nmtv->itemNew.pszText, NULL,
                                            CONVERT_SEND);
    }
    case TVN_DELETEITEMW:
    {
        NMTREEVIEWW *nmtv = (NMTREEVIEWW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmtv->itemOld.mask, TVIF_TEXT, &nmtv->itemOld.pszText, NULL,
                                            CONVERT_SEND);
    }
    case TVN_BEGINLABELEDITW:
    case TVN_ENDLABELEDITW:
    {
        NMTVDISPINFOW *nmtvdi = (NMTVDISPINFOW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmtvdi->item.mask, TVIF_TEXT, &nmtvdi->item.pszText,
                                            &nmtvdi->item.cchTextMax, SET_NULL_IF_NO_MASK | CONVERT_SEND | CONVERT_RECEIVE);
    }
    case TVN_SELCHANGINGW:
    case TVN_SELCHANGEDW:
    {
        NMTREEVIEWW *nmtv = (NMTREEVIEWW *)hdr;
        WCHAR *oldItemOldText = NULL;
        WCHAR *oldItemNewText = NULL;

        hdr->code = COMCTL32_GetAnsiNtfCode(hdr->code);

        if (!((nmtv->itemNew.mask | nmtv->itemOld.mask) & TVIF_TEXT))
            return SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);

        if (nmtv->itemOld.mask & TVIF_TEXT) oldItemOldText = COMCTL32_ConvertText(&nmtv->itemOld.pszText);
        if (nmtv->itemNew.mask & TVIF_TEXT) oldItemNewText = COMCTL32_ConvertText(&nmtv->itemNew.pszText);

        ret = SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
        COMCTL32_RestoreText(&nmtv->itemOld.pszText, oldItemOldText);
        COMCTL32_RestoreText(&nmtv->itemNew.pszText, oldItemNewText);
        return ret;
    }
    case TVN_GETDISPINFOW:
    {
        NMTVDISPINFOW *nmtvdi = (NMTVDISPINFOW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmtvdi->item.mask, TVIF_TEXT, &nmtvdi->item.pszText,
                                            &nmtvdi->item.cchTextMax, ZERO_SEND | CONVERT_RECEIVE);
    }
    case TVN_SETDISPINFOW:
    {
        NMTVDISPINFOW *nmtvdi = (NMTVDISPINFOW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, &nmtvdi->item.mask, TVIF_TEXT, &nmtvdi->item.pszText,
                                            &nmtvdi->item.cchTextMax, SET_NULL_IF_NO_MASK | CONVERT_SEND | CONVERT_RECEIVE);
    }
    case TVN_GETINFOTIPW:
    {
        NMTVGETINFOTIPW *nmtvgit = (NMTVGETINFOTIPW *)hdr;
        return COMCTL32_SendConvertedNotify(hwnd_notify, hdr, NULL, 0, &nmtvgit->pszText, &nmtvgit->cchTextMax, CONVERT_RECEIVE);
    }
    }
    /* Other notifications, no need to convert */
    return SendMessageW(hwnd_notify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
}

void COMCTL32_OpenThemeForWindow(HWND hwnd, const WCHAR *theme_class)
{
#if __WINE_COMCTL32_VERSION == 6
    OpenThemeData(hwnd, theme_class);
#endif
}

void COMCTL32_CloseThemeForWindow(HWND hwnd)
{
#if __WINE_COMCTL32_VERSION == 6
    CloseThemeData(GetWindowTheme(hwnd));
#endif
}

/* A helper to handle CCM_SETVERSION messages */
LRESULT COMCTL32_SetVersion(INT *current_version, INT new_version)
{
#if __WINE_COMCTL32_VERSION == 6
    return *current_version;
#else
    INT old_version;

    if (new_version > 5)
        return -1;

    old_version = *current_version;
    *current_version = new_version;
    return old_version;
#endif
}

/* A helper to handle WM_THEMECHANGED messages */
LRESULT COMCTL32_ThemeChanged(HWND hwnd, const WCHAR *theme_class, BOOL invalidate, BOOL erase)
{
#if __WINE_COMCTL32_VERSION == 6
    if (theme_class)
    {
        COMCTL32_CloseThemeForWindow(hwnd);
        COMCTL32_OpenThemeForWindow(hwnd, theme_class);
    }

    if (invalidate)
        InvalidateRect(hwnd, NULL, erase);
    return 0;
#else
    return DefWindowProcW(hwnd, WM_THEMECHANGED, 0, 0);
#endif
}

/* A helper to handle WM_NCPAINT messages
 *
 * If theme_class is specified, open the specified theme class. Otherwise, get the theme class from
 * the window.
 */
LRESULT COMCTL32_NCPaint(HWND hwnd, WPARAM wp, LPARAM lp, const WCHAR *theme_class)
{
#if __WINE_COMCTL32_VERSION == 6
    HRGN region = (HRGN)wp, clipRgn;
    INT cxEdge, cyEdge;
    HTHEME theme;
    LONG exStyle;
    HDC dc;
    RECT r;

    exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (!(exStyle & WS_EX_CLIENTEDGE))
        return DefWindowProcW(hwnd, WM_NCPAINT, wp, lp);

    if (theme_class)
        theme = OpenThemeDataForDpi(NULL, theme_class, GetDpiForWindow(hwnd));
    else
        theme = GetWindowTheme(hwnd);
    if (!theme)
        return DefWindowProcW(hwnd, WM_NCPAINT, wp, lp);

    cxEdge = GetSystemMetrics(SM_CXEDGE);
    cyEdge = GetSystemMetrics(SM_CYEDGE);
    GetWindowRect(hwnd, &r);

    /* New clipping region passed to default proc to exclude border */
    clipRgn = CreateRectRgn(r.left + cxEdge, r.top + cyEdge, r.right - cxEdge, r.bottom - cyEdge);
    if (region != (HRGN)1)
        CombineRgn(clipRgn, clipRgn, region, RGN_AND);
    OffsetRect(&r, -r.left, -r.top);

#ifdef __REACTOS__ /* r73789 */
    dc = GetWindowDC(hwnd);
    /* Exclude client part */
    ExcludeClipRect(dc, r.left + cxEdge, r.top + cyEdge,
        r.right - cxEdge, r.bottom -cyEdge);
#else
    dc = GetDCEx(hwnd, region, DCX_WINDOW | DCX_INTERSECTRGN);
#endif
    if (IsThemeBackgroundPartiallyTransparent(theme, 0, 0))
        DrawThemeParentBackground(hwnd, dc, &r);
    DrawThemeBackground(theme, dc, 0, 0, &r, 0);
    ReleaseDC(hwnd, dc);
    if (theme_class)
        CloseThemeData(theme);

    /* Call default proc to get the scrollbars etc. also painted */
    DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)clipRgn, 0);
    DeleteObject(clipRgn);
    return 0;
#else
    return DefWindowProcW(hwnd, WM_NCPAINT, wp, lp);
#endif
}

BOOL COMCTL32_IsThemed(HWND hwnd)
{
#if __WINE_COMCTL32_VERSION == 6
    return !!GetWindowTheme(hwnd);
#else
    return FALSE;
#endif
}

HRGN set_control_clipping( HDC hdc, const RECT *rect )
{
    RECT rc = *rect;
    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );

    if (GetClipRgn( hdc, hrgn ) != 1)
    {
        DeleteObject( hrgn );
        hrgn = 0;
    }
    DPtoLP( hdc, (POINT *)&rc, 2 );
    if (GetLayout( hdc ) & LAYOUT_RTL)  /* compensate for the shifting done by IntersectClipRect */
    {
        rc.left++;
        rc.right++;
    }
    IntersectClipRect( hdc, rc.left, rc.top, rc.right, rc.bottom );
    return hrgn;
}
