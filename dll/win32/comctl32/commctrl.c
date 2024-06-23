/*
 * Common controls functions
 *
 * Copyright 1997 Dimitrie O. Paun
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

static const WCHAR strCC32SubclassInfo[] = {
    'C','C','3','2','S','u','b','c','l','a','s','s','I','n','f','o',0
};

#ifdef __REACTOS__

#include <strsafe.h>

#define NAME       L"microsoft.windows.common-controls"
#define VERSION_V5 L"5.82.2600.2982"
#define VERSION    L"6.0.2600.2982"
#define PUBLIC_KEY L"6595b64144ccf1df"

#ifdef __i386__
#define ARCH L"x86"
#elif defined __x86_64__
#define ARCH L"amd64"
#else
#define ARCH L"none"
#endif

static const WCHAR manifest_filename[] = ARCH L"_" NAME L"_" PUBLIC_KEY L"_" VERSION L"_none_deadbeef.manifest";
static const WCHAR manifest_filename_v5[] = ARCH L"_" NAME L"_" PUBLIC_KEY L"_" VERSION_V5 L"_none_deadbeef.manifest";

static WCHAR* GetManifestPath(BOOL create, BOOL bV6)
{
    WCHAR *pwszBuf;
    HRESULT hres;

    pwszBuf = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (!pwszBuf)
        return NULL;

    GetWindowsDirectoryW(pwszBuf, MAX_PATH);
    hres = StringCchCatW(pwszBuf, MAX_PATH, L"\\winsxs");
    if (FAILED(hres))
        return NULL;
    if (create)
        CreateDirectoryW(pwszBuf, NULL);
    hres = StringCchCatW(pwszBuf, MAX_PATH, L"\\manifests\\");
    if (FAILED(hres))
        return NULL;
    if (create)
        CreateDirectoryW(pwszBuf, NULL);

    hres = StringCchCatW(pwszBuf, MAX_PATH, bV6 ? manifest_filename : manifest_filename_v5);
    if (FAILED(hres))
        return NULL;

    return pwszBuf;
}

static HANDLE CreateComctl32ActCtx(BOOL bV6)
{
    HANDLE ret;
    WCHAR* pwstrSource;
    ACTCTXW ActCtx = {sizeof(ACTCTX)};

    pwstrSource = GetManifestPath(FALSE, bV6);
    if (!pwstrSource)
    {
        ERR("GetManifestPath failed! bV6=%d\n", bV6);
        return INVALID_HANDLE_VALUE;
    }
    ActCtx.lpSource = pwstrSource;
    ret = CreateActCtxW(&ActCtx);
    HeapFree(GetProcessHeap(), 0, pwstrSource);
    if (ret == INVALID_HANDLE_VALUE)
        ERR("CreateActCtxW failed! bV6=%d\n", bV6);
    return ret;
}

static void RegisterControls(BOOL bV6)
{
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
    TOOLTIPS_Register ();
    TRACKBAR_Register ();
    TREEVIEW_Register ();
    UPDOWN_Register ();

    if (!bV6)
    {
        TOOLBAR_Register ();
    }
    else
    {
        BUTTON_Register ();
        COMBO_Register ();
        COMBOLBOX_Register ();
        EDIT_Register ();
        LISTBOX_Register ();
        STATIC_Register ();

        TOOLBARv6_Register();
    }
}

static void UnregisterControls(BOOL bV6)
{
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
    TOOLTIPS_Unregister ();
    TRACKBAR_Unregister ();
    TREEVIEW_Unregister ();
    UPDOWN_Unregister ();

    if (!bV6)
    {
        TOOLBAR_Unregister ();
    }
    else
    {
        BUTTON_Unregister();
        COMBO_Unregister ();
        COMBOLBOX_Unregister ();
        EDIT_Unregister ();
        LISTBOX_Unregister ();
        STATIC_Unregister ();

        TOOLBARv6_Unregister ();
    }

}

static void InitializeClasses()
{
    HANDLE hActCtx5, hActCtx6;
    BOOL activated;
    ULONG_PTR ulCookie;

    /* like comctl32 5.82+ register all the common control classes */
    /* Register the classes once no matter what */
    hActCtx5 = CreateComctl32ActCtx(FALSE);
    activated = (hActCtx5 != INVALID_HANDLE_VALUE ? ActivateActCtx(hActCtx5, &ulCookie) : FALSE);
    RegisterControls(FALSE);      /* Register the classes pretending to be v5 */
    if (activated) DeactivateActCtx(0, ulCookie);

    hActCtx6 = CreateComctl32ActCtx(TRUE);
    if (hActCtx6 != INVALID_HANDLE_VALUE)
    {
        activated = ActivateActCtx(hActCtx6, &ulCookie);
        RegisterControls(TRUE);      /* Register the classes pretending to be v6 */
        if (activated) DeactivateActCtx(0, ulCookie);

        /* Initialize the themed controls only when the v6 manifest is present */
        THEMING_Initialize (hActCtx5, hActCtx6);
    }
}

static void UninitializeClasses()
{
    HANDLE hActCtx5, hActCtx6;
    BOOL activated;
    ULONG_PTR ulCookie;

    hActCtx5 = CreateComctl32ActCtx(FALSE);
    activated = (hActCtx5 != INVALID_HANDLE_VALUE ? ActivateActCtx(hActCtx5, &ulCookie) : FALSE);
    UnregisterControls(FALSE);
    if (activated) DeactivateActCtx(0, ulCookie);

    hActCtx6 = CreateComctl32ActCtx(TRUE);
    if (hActCtx6 != INVALID_HANDLE_VALUE)
    {
        activated = ActivateActCtx(hActCtx6, &ulCookie);
        THEMING_Uninitialize();
        UnregisterControls(TRUE);
        if (activated) DeactivateActCtx(0, ulCookie);
    }
}

/***********************************************************************
 * RegisterClassNameW [COMCTL32.@]
 *
 * Register window class again while using as SxS module.
 */
BOOLEAN WINAPI RegisterClassNameW(LPCWSTR className)
{
    InitializeClasses();
    return TRUE;
}

#endif /* __REACTOS__ */

#ifndef __REACTOS__
static void unregister_versioned_classes(void)
{
#define VERSION "6.0.2600.2982!"
    static const char *classes[] =
    {
        VERSION WC_BUTTONA,
        VERSION WC_COMBOBOXA,
        VERSION "ComboLBox",
        VERSION WC_EDITA,
        VERSION WC_LISTBOXA,
        VERSION WC_STATICA,
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(classes); i++)
        UnregisterClassA(classes[i], NULL);

#undef VERSION
}
#endif

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
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

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

#ifndef __REACTOS__
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

            BUTTON_Register ();
            COMBO_Register ();
            COMBOLBOX_Register ();
            EDIT_Register ();
            LISTBOX_Register ();
            STATIC_Register ();

            /* subclass user32 controls */
            THEMING_Initialize ();
#else
            InitializeClasses();
#endif

            break;

	case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
#ifndef __REACTOS__
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

            unregister_versioned_classes ();

#else
            UninitializeClasses();
#endif
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
    COLORREF oldbkcolor;

    if (style & SBT_POPOUT)
        border = BDR_RAISEDOUTER;
    else if (style & SBT_NOBORDERS)
        border = 0;

    oldbkcolor = SetBkColor (hdc, comctl32_color.clrBtnFace);
    DrawEdge (hdc, &r, border, BF_MIDDLE|BF_RECT|BF_ADJUST);

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

    SetBkColor (hdc, oldbkcolor);
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
    TRACE("(%u, %s): stub\n", bInstall, debugstr_w(cmdline));
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

   TRACE ("(%p, %p, %lx, %lx)\n", hWnd, pfnSubclass, uIDSubclass, dwRef);

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

   TRACE ("(%p, %p, %lx, %p)\n", hWnd, pfnSubclass, uID, pdwRef);

   /* See if we have been called for this window */
   stack = GetPropW (hWnd, COMCTL32_wSubclass);
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

   TRACE ("(%p, %p, %lx)\n", hWnd, pfnSubclass, uID);

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
static LRESULT WINAPI COMCTL32_SubclassProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LPSUBCLASS_INFO stack;
   LPSUBCLASSPROCS proc;
   LRESULT ret;
    
   TRACE ("(%p, 0x%08x, 0x%08lx, 0x%08lx)\n", hWnd, uMsg, wParam, lParam);

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
   
   TRACE ("(%p, 0x%08x, 0x%08lx, 0x%08lx)\n", hWnd, uMsg, wParam, lParam);

   /* retrieve our little stack from the Properties */
   stack = GetPropW (hWnd, COMCTL32_wSubclass);
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

/***********************************************************************
 * DrawShadowText [COMCTL32.@]
 *
 * Draw text with shadow.
 */
int WINAPI DrawShadowText(HDC hdc, LPCWSTR pszText, UINT cch, RECT *prc, DWORD dwFlags,
                          COLORREF crText, COLORREF crShadow, int ixOffset, int iyOffset)
{
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
    bi.bmiHeader.biWidth = prc->right - prc->left + 4;
    bi.bmiHeader.biHeight = prc->bottom - prc->top + 5; // bottom-up DIB
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
    SetRect(&rcText, 0, 0, prc->right - prc->left, prc->bottom - prc->top);
    DrawTextW(hdcMem, pszText, cch, &rcText, dwFlags);
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
    ixOffset -= 3;
    iyOffset -= 3;

    /* Alpha blend helper image to destination DC */
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    GdiAlphaBlend(hdc, prc->left + ixOffset, prc->top + iyOffset, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, hdcMem, 0, 0, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, bf);

    /* Delete the helper bitmap */
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbm);
    DeleteDC(hdcMem);

    /* Finally draw the text over shadow */
    crOldText = SetTextColor(hdc, crText);
    SetBkMode(hdc, TRANSPARENT);
    iRet = DrawTextW(hdc, pszText, cch, prc, dwFlags);
    SetTextColor(hdc, crOldText);

    return iRet;
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
