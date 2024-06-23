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

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "windowsx.h"

#ifdef __REACTOS__
// This is really ComCtl32 v5.82, the last one not supporting SxS
#undef  COMCTL32_VERSION // Undefines what the PSDK gave to us
#define COMCTL32_VERSION        5
#define COMCTL32_VERSION_MINOR 82
#endif

extern HMODULE COMCTL32_hModule DECLSPEC_HIDDEN;
extern HBRUSH  COMCTL32_hPattern55AABrush DECLSPEC_HIDDEN;

/* Property sheet / Wizard */
#define IDD_PROPSHEET 1006
#define IDD_WIZARD    1020

#define IDC_TABCONTROL   12320
#define IDC_APPLY_BUTTON 12321
#define IDC_BACK_BUTTON  12323
#define IDC_NEXT_BUTTON  12324
#define IDC_FINISH_BUTTON 12325
#define IDC_SUNKEN_LINE   12326
#define IDC_SUNKEN_LINEHEADER 12327

#define IDS_CLOSE	  4160

/* Toolbar customization dialog */
#define IDD_TBCUSTOMIZE     200

#define IDC_AVAILBTN_LBOX   201
#define IDC_RESET_BTN       202
#define IDC_TOOLBARBTN_LBOX 203
#define IDC_REMOVE_BTN      204
#define IDC_HELP_BTN        205
#define IDC_MOVEUP_BTN      206
#define IDC_MOVEDN_BTN      207

#define IDS_SEPARATOR      1024

/* Toolbar imagelist bitmaps */
#define IDB_STD_SMALL       120
#define IDB_STD_LARGE       121
#define IDB_VIEW_SMALL      124
#define IDB_VIEW_LARGE      125
#define IDB_HIST_SMALL      130
#define IDB_HIST_LARGE      131

#define IDM_TODAY                      4163
#define IDM_GOTODAY                    4164

/* Treeview Checkboxes */

#define IDT_CHECK        401

/* Command Link arrow */
#define IDB_CMDLINK      402


/* Cursors */
#define IDC_MOVEBUTTON                  102
#define IDC_COPY                        104
#define IDC_DIVIDER                     106
#define IDC_DIVIDEROPEN                 107


/* DragList resources */
#define IDI_DRAGARROW                   501

/* HOTKEY internal strings */
#define HKY_NONE                        2048

/* Tooltip icons */
#define IDI_TT_INFO_SM                   22
#define IDI_TT_WARN_SM                   25
#define IDI_TT_ERROR_SM                  28

/* Taskdialog strings */
#define IDS_BUTTON_YES    3000
#define IDS_BUTTON_NO     3001
#define IDS_BUTTON_RETRY  3002
#define IDS_BUTTON_OK     3003
#define IDS_BUTTON_CANCEL 3004
#define IDS_BUTTON_CLOSE  3005

#define IDS_TD_EXPANDED   3020
#define IDS_TD_COLLAPSED  3021

#ifndef __REACTOS__
#define WM_SYSTIMER     0x0118
#endif

enum combobox_state_flags
{
    CBF_DROPPED      = 0x0001,
    CBF_BUTTONDOWN   = 0x0002,
    CBF_NOROLLUP     = 0x0004,
    CBF_MEASUREITEM  = 0x0008,
    CBF_FOCUSED      = 0x0010,
    CBF_CAPTURE      = 0x0020,
    CBF_EDIT         = 0x0040,
    CBF_NORESIZE     = 0x0080,
    CBF_NOTIFY       = 0x0100,
    CBF_NOREDRAW     = 0x0200,
    CBF_SELCHANGE    = 0x0400,
    CBF_HOT          = 0x0800,
    CBF_NOEDITNOTIFY = 0x1000,
    CBF_NOLBSELECT   = 0x2000, /* do not change current selection */
    CBF_BEENFOCUSED  = 0x4000, /* has it ever had focus           */
    CBF_EUI          = 0x8000,
};

typedef struct
{
   HWND           self;
   HWND           owner;
   UINT           dwStyle;
   HWND           hWndEdit;
   HWND           hWndLBox;
   UINT           wState;
   HFONT          hFont;
   RECT           textRect;
   RECT           buttonRect;
   RECT           droppedRect;
   INT            droppedIndex;
   INT            fixedOwnerDrawHeight;
   INT            droppedWidth;   /* not used unless set explicitly */
   INT            item_height;
   INT            visibleItems;
} HEADCOMBO, *LPHEADCOMBO;

extern BOOL COMBO_FlipListbox(HEADCOMBO *lphc, BOOL ok, BOOL bRedrawButton) DECLSPEC_HIDDEN;

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

#ifndef __REACTOS__
#define COMCTL32_VERSION_MINOR 81
#endif

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

HRGN set_control_clipping(HDC hdc, const RECT *rect) DECLSPEC_HIDDEN;

extern void ANIMATE_Register(void) DECLSPEC_HIDDEN;
extern void ANIMATE_Unregister(void) DECLSPEC_HIDDEN;
extern void BUTTON_Register(void) DECLSPEC_HIDDEN;
extern void COMBO_Register(void) DECLSPEC_HIDDEN;
extern void COMBOEX_Register(void) DECLSPEC_HIDDEN;
extern void COMBOEX_Unregister(void) DECLSPEC_HIDDEN;
extern void COMBOLBOX_Register(void) DECLSPEC_HIDDEN;
extern void DATETIME_Register(void) DECLSPEC_HIDDEN;
extern void DATETIME_Unregister(void) DECLSPEC_HIDDEN;
extern void EDIT_Register(void) DECLSPEC_HIDDEN;
extern void FLATSB_Register(void) DECLSPEC_HIDDEN;
extern void FLATSB_Unregister(void) DECLSPEC_HIDDEN;
extern void HEADER_Register(void) DECLSPEC_HIDDEN;
extern void HEADER_Unregister(void) DECLSPEC_HIDDEN;
extern void HOTKEY_Register(void) DECLSPEC_HIDDEN;
extern void HOTKEY_Unregister(void) DECLSPEC_HIDDEN;
extern void IPADDRESS_Register(void) DECLSPEC_HIDDEN;
extern void IPADDRESS_Unregister(void) DECLSPEC_HIDDEN;
extern void LISTBOX_Register(void) DECLSPEC_HIDDEN;
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
extern void STATIC_Register(void) DECLSPEC_HIDDEN;
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
#ifdef __REACTOS__
extern void BUTTON_Unregister(void) DECLSPEC_HIDDEN;
extern void COMBO_Unregister(void) DECLSPEC_HIDDEN;
extern void COMBOLBOX_Unregister(void) DECLSPEC_HIDDEN;
extern void EDIT_Unregister(void) DECLSPEC_HIDDEN;
extern void LISTBOX_Unregister(void) DECLSPEC_HIDDEN;
extern void STATIC_Unregister(void) DECLSPEC_HIDDEN;
extern void TOOLBARv6_Register(void) DECLSPEC_HIDDEN;
extern void TOOLBARv6_Unregister(void) DECLSPEC_HIDDEN;
#endif /* __REACTOS__ */

int MONTHCAL_MonthLength(int month, int year) DECLSPEC_HIDDEN;
int MONTHCAL_CalculateDayOfWeek(SYSTEMTIME *date, BOOL inplace) DECLSPEC_HIDDEN;
LONG MONTHCAL_CompareSystemTime(const SYSTEMTIME *first, const SYSTEMTIME *second) DECLSPEC_HIDDEN;
#ifdef __REACTOS__
extern void THEMING_Initialize(HANDLE hActCtx5, HANDLE hActCtx6) DECLSPEC_HIDDEN;
#else
extern void THEMING_Initialize(void) DECLSPEC_HIDDEN;
#endif
extern void THEMING_Uninitialize(void) DECLSPEC_HIDDEN;
extern LRESULT THEMING_CallOriginalClass(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;

#ifdef __REACTOS__
#define IDI_SHIELD  32518
#define wcsnicmp _wcsnicmp
#define GetDpiForWindow(PVOID) 96
#endif

#endif  /* __WINE_COMCTL32_H */
