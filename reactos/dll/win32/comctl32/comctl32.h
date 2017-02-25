/******************************************************************************
 *
 * Common definitions (resource ids and global variables)
 *
 * Copyright 1999 Thuy Nguyen
 * Copyright 1999 Eric Kohl
 * Copyright 2002 Dimitrie O. Paun
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
 */

#ifndef __WINE_COMCTL32_H
#define __WINE_COMCTL32_H

#include <wine/config.h>

#include <stdarg.h>
#include <assert.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winreg.h>
#include <objbase.h>
#include <uxtheme.h>
#include <vssym32.h>

#include <wine/unicode.h>
#include <wine/debug.h>

#include "resource.h"

extern HMODULE COMCTL32_hModule DECLSPEC_HIDDEN;
extern HBRUSH  COMCTL32_hPattern55AABrush DECLSPEC_HIDDEN;

typedef struct
{
    COLORREF clrBtnHighlight;       /* COLOR_BTNHIGHLIGHT                  */
    COLORREF clrBtnShadow;          /* COLOR_BTNSHADOW                     */
    COLORREF clrBtnText;            /* COLOR_BTNTEXT                       */
    COLORREF clrBtnFace;            /* COLOR_BTNFACE                       */
    COLORREF clrHighlight;          /* COLOR_HIGHLIGHT                     */
    COLORREF clrHighlightText;      /* COLOR_HIGHLIGHTTEXT                 */
    COLORREF clrHotTrackingColor;   /* COLOR_HOTLIGHT                      */
    COLORREF clr3dHilight;          /* COLOR_3DHILIGHT                     */
    COLORREF clr3dShadow;           /* COLOR_3DSHADOW                      */
    COLORREF clr3dDkShadow;         /* COLOR_3DDKSHADOW                    */
    COLORREF clr3dFace;             /* COLOR_3DFACE                        */
    COLORREF clrWindow;             /* COLOR_WINDOW                        */
    COLORREF clrWindowText;         /* COLOR_WINDOWTEXT                    */
    COLORREF clrGrayText;           /* COLOR_GREYTEXT                      */
    COLORREF clrActiveCaption;      /* COLOR_ACTIVECAPTION                 */
    COLORREF clrInfoBk;             /* COLOR_INFOBK                        */
    COLORREF clrInfoText;           /* COLOR_INFOTEXT                      */
} COMCTL32_SysColor;

extern COMCTL32_SysColor  comctl32_color DECLSPEC_HIDDEN;

typedef struct _BUTTON_DATA {
    LONG state;
    HFONT font;
    LONG_PTR image;
    DWORD ui_state;

    RECT rcTextMargin;
    BUTTON_IMAGELIST imlData;
} BUTTON_DATA, *PBUTTON_DATA;

static inline PBUTTON_DATA _GetButtonData(HWND hwnd)
{
    return (PBUTTON_DATA)GetWindowLongPtrW( hwnd, 0 );
}

/* Internal function */
HWND COMCTL32_CreateToolTip (HWND) DECLSPEC_HIDDEN;
VOID COMCTL32_RefreshSysColors(void) DECLSPEC_HIDDEN;
void COMCTL32_DrawInsertMark(HDC hDC, const RECT *lpRect, COLORREF clrInsertMark, BOOL bHorizontal) DECLSPEC_HIDDEN;
void COMCTL32_EnsureBitmapSize(HBITMAP *pBitmap, int cxMinWidth, int cyMinHeight, COLORREF crBackground) DECLSPEC_HIDDEN;
void COMCTL32_GetFontMetrics(HFONT hFont, TEXTMETRICW *ptm) DECLSPEC_HIDDEN;
BOOL COMCTL32_IsReflectedMessage(UINT uMsg) DECLSPEC_HIDDEN;
INT  Str_GetPtrWtoA (LPCWSTR lpSrc, LPSTR lpDest, INT nMaxLen) DECLSPEC_HIDDEN;
INT  Str_GetPtrAtoW (LPCSTR lpSrc, LPWSTR lpDest, INT nMaxLen) DECLSPEC_HIDDEN;
BOOL Str_SetPtrAtoW (LPWSTR *lppDest, LPCSTR lpSrc) DECLSPEC_HIDDEN;
BOOL Str_SetPtrWtoA (LPSTR *lppDest, LPCWSTR lpSrc) DECLSPEC_HIDDEN;

/* Our internal stack structure of the window procedures to subclass */
typedef struct _SUBCLASSPROCS {
    SUBCLASSPROC subproc;
    UINT_PTR id;
    DWORD_PTR ref;
    struct _SUBCLASSPROCS *next;
} SUBCLASSPROCS, *LPSUBCLASSPROCS;

typedef struct
{
   SUBCLASSPROCS *SubclassProcs;
   SUBCLASSPROCS *stackpos;
   WNDPROC origproc;
   int running;
} SUBCLASS_INFO, *LPSUBCLASS_INFO;

/* undocumented functions */

LPVOID WINAPI Alloc (DWORD) __WINE_ALLOC_SIZE(1);
LPVOID WINAPI ReAlloc (LPVOID, DWORD) __WINE_ALLOC_SIZE(2);
BOOL   WINAPI Free (LPVOID);
DWORD  WINAPI GetSize (LPVOID);

INT  WINAPI Str_GetPtrA (LPCSTR, LPSTR, INT);
INT  WINAPI Str_GetPtrW (LPCWSTR, LPWSTR, INT);

LRESULT WINAPI SetPathWordBreakProc(HWND hwnd, BOOL bSet);
BOOL WINAPI MirrorIcon(HICON *phicon1, HICON *phicon2);

extern void ANIMATE_Register(void) DECLSPEC_HIDDEN;
extern void ANIMATE_Unregister(void) DECLSPEC_HIDDEN;
extern void COMBOEX_Register(void) DECLSPEC_HIDDEN;
extern void COMBOEX_Unregister(void) DECLSPEC_HIDDEN;
extern void DATETIME_Register(void) DECLSPEC_HIDDEN;
extern void DATETIME_Unregister(void) DECLSPEC_HIDDEN;
extern void FLATSB_Register(void) DECLSPEC_HIDDEN;
extern void FLATSB_Unregister(void) DECLSPEC_HIDDEN;
extern void HEADER_Register(void) DECLSPEC_HIDDEN;
extern void HEADER_Unregister(void) DECLSPEC_HIDDEN;
extern void HOTKEY_Register(void) DECLSPEC_HIDDEN;
extern void HOTKEY_Unregister(void) DECLSPEC_HIDDEN;
extern void IPADDRESS_Register(void) DECLSPEC_HIDDEN;
extern void IPADDRESS_Unregister(void) DECLSPEC_HIDDEN;
extern void LISTVIEW_Register(void) DECLSPEC_HIDDEN;
extern void LISTVIEW_Unregister(void) DECLSPEC_HIDDEN;
extern void MONTHCAL_Register(void) DECLSPEC_HIDDEN;
extern void MONTHCAL_Unregister(void) DECLSPEC_HIDDEN;
extern void NATIVEFONT_Register(void) DECLSPEC_HIDDEN;
extern void NATIVEFONT_Unregister(void) DECLSPEC_HIDDEN;
extern void PAGER_Register(void) DECLSPEC_HIDDEN;
extern void PAGER_Unregister(void) DECLSPEC_HIDDEN;
extern void PROGRESS_Register(void) DECLSPEC_HIDDEN;
extern void PROGRESS_Unregister(void) DECLSPEC_HIDDEN;
extern void REBAR_Register(void) DECLSPEC_HIDDEN;
extern void REBAR_Unregister(void) DECLSPEC_HIDDEN;
extern void STATUS_Register(void) DECLSPEC_HIDDEN;
extern void STATUS_Unregister(void) DECLSPEC_HIDDEN;
extern void SYSLINK_Register(void) DECLSPEC_HIDDEN;
extern void SYSLINK_Unregister(void) DECLSPEC_HIDDEN;
extern void TAB_Register(void) DECLSPEC_HIDDEN;
extern void TAB_Unregister(void) DECLSPEC_HIDDEN;
extern void TOOLBAR_Register(void) DECLSPEC_HIDDEN;
extern void TOOLBAR_Unregister(void) DECLSPEC_HIDDEN;
extern void TOOLTIPS_Register(void) DECLSPEC_HIDDEN;
extern void TOOLTIPS_Unregister(void) DECLSPEC_HIDDEN;
extern void TRACKBAR_Register(void) DECLSPEC_HIDDEN;
extern void TRACKBAR_Unregister(void) DECLSPEC_HIDDEN;
extern void TREEVIEW_Register(void) DECLSPEC_HIDDEN;
extern void TREEVIEW_Unregister(void) DECLSPEC_HIDDEN;
extern void UPDOWN_Register(void) DECLSPEC_HIDDEN;
extern void UPDOWN_Unregister(void) DECLSPEC_HIDDEN;
extern void BUTTON_Register();
extern void BUTTON_Unregister();

int MONTHCAL_MonthLength(int month, int year) DECLSPEC_HIDDEN;
int MONTHCAL_CalculateDayOfWeek(SYSTEMTIME *date, BOOL inplace) DECLSPEC_HIDDEN;
LONG MONTHCAL_CompareSystemTime(const SYSTEMTIME *first, const SYSTEMTIME *second) DECLSPEC_HIDDEN;

extern void THEMING_Initialize(HANDLE hActCtx5, HANDLE hActCtx6) DECLSPEC_HIDDEN;
extern void THEMING_Uninitialize(void) DECLSPEC_HIDDEN;
extern LRESULT THEMING_CallOriginalClass(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
extern void THEMING_SetSubclassData(HWND, ULONG_PTR) DECLSPEC_HIDDEN;

#endif  /* __WINE_COMCTL32_H */
