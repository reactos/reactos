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

extern HMODULE COMCTL32_hModule;
extern HBRUSH  COMCTL32_hPattern55AABrush;

/* has a value of: 0, CCS_TOP, CCS_NOMOVEY, CCS_BOTTOM */
#define CCS_LAYOUT_MASK 0x3

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


/* Month calendar month menu popup */
#define IDD_MCMONTHMENU     300

#define IDM_JAN				301
#define IDM_FEB				302
#define IDM_MAR				303
#define IDM_APR				304
#define IDM_MAY				305
#define IDM_JUN				306
#define IDM_JUL				307
#define IDM_AUG				308
#define IDM_SEP				309
#define IDM_OCT				310
#define IDM_NOV				311
#define IDM_DEC				312

#define IDM_TODAY                      4163
#define IDM_GOTODAY                    4164

/* Treeview Checkboxes */

#define IDT_CHECK        401


/* Header cursors */
#define IDC_DIVIDER                     106
#define IDC_DIVIDEROPEN                 107


/* DragList resources */
#define IDI_DRAGARROW                   501
#define IDC_COPY                        502

#define IDC_MOVEBUTTON                    1

/* HOTKEY internal strings */
#define HKY_NONE                        2048

/* Tooltip icons */
#define IDI_TT_INFO_SM                   22
#define IDI_TT_WARN_SM                   25
#define IDI_TT_ERROR_SM                  28

typedef struct
{
    COLORREF clrBtnHighlight;       /* COLOR_BTNHIGHLIGHT                  */
    COLORREF clrBtnShadow;          /* COLOR_BTNSHADOW                     */
    COLORREF clrBtnText;            /* COLOR_BTNTEXT                       */
    COLORREF clrBtnFace;            /* COLOR_BTNFACE                       */
    COLORREF clrHighlight;          /* COLOR_HIGHLIGHT                     */
    COLORREF clrHighlightText;      /* COLOR_HIGHLIGHTTEXT                 */
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

extern COMCTL32_SysColor  comctl32_color;

/* Internal function */
HWND COMCTL32_CreateToolTip (HWND);
VOID COMCTL32_RefreshSysColors(void);
void COMCTL32_DrawInsertMark(HDC hDC, const RECT *lpRect, COLORREF clrInsertMark, BOOL bHorizontal);
void COMCTL32_EnsureBitmapSize(HBITMAP *pBitmap, int cxMinWidth, int cyMinHeight, COLORREF crBackground);
INT  Str_GetPtrWtoA (LPCWSTR lpSrc, LPSTR lpDest, INT nMaxLen);
INT  Str_GetPtrAtoW (LPCSTR lpSrc, LPWSTR lpDest, INT nMaxLen);
BOOL Str_SetPtrAtoW (LPWSTR *lppDest, LPCSTR lpSrc);
BOOL Str_SetPtrWtoA (LPSTR *lppDest, LPCWSTR lpSrc);

#define COMCTL32_VERSION_MINOR 81
#define WINE_FILEVERSION 5, COMCTL32_VERSION_MINOR, 4704, 1100
#define WINE_FILEVERSIONSTR "5.81"

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

LPVOID WINAPI Alloc (DWORD);
LPVOID WINAPI ReAlloc (LPVOID, DWORD);
BOOL   WINAPI Free (LPVOID);
DWORD  WINAPI GetSize (LPVOID);

INT  WINAPI Str_GetPtrA (LPCSTR, LPSTR, INT);
INT  WINAPI Str_GetPtrW (LPCWSTR, LPWSTR, INT);

INT  WINAPI DPA_GetPtrIndex (const HDPA, LPVOID);
BOOL WINAPI DPA_Grow (const HDPA, INT);

#define DPAM_NOSORT             0x0001
#define DPAM_INSERT             0x0004
#define DPAM_DELETE             0x0008

typedef PVOID (CALLBACK *PFNDPAMERGE)(DWORD,PVOID,PVOID,LPARAM);
BOOL WINAPI DPA_Merge (const HDPA, const HDPA, DWORD, PFNDPACOMPARE, PFNDPAMERGE, LPARAM);

#define DPA_GetPtrCount(hdpa)  (*(INT*)(hdpa))

LRESULT WINAPI SetPathWordBreakProc(HWND hwnd, BOOL bSet);
BOOL WINAPI MirrorIcon(HICON *phicon1, HICON *phicon2);

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
extern void SYSLINK_Register(void);
extern void SYSLINK_Unregister(void);
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


int MONTHCAL_MonthLength(int month, int year);

static inline void MONTHCAL_CopyTime(const SYSTEMTIME *from, SYSTEMTIME *to)
{
  to->wYear = from->wYear;
  to->wMonth = from->wMonth;
  to->wDayOfWeek = from->wDayOfWeek;
  to->wDay = from->wDay;
  to->wHour = from->wHour;
  to->wMinute = from->wMinute;
  to->wSecond = from->wSecond;
  to->wMilliseconds = from->wMilliseconds;
}

extern void THEMING_Initialize(void);
extern void THEMING_Uninitialize(void);
extern LRESULT THEMING_CallOriginalClass(HWND, UINT, WPARAM, LPARAM);
extern void THEMING_SetSubclassData(HWND, ULONG_PTR);

#endif  /* __WINE_COMCTL32_H */
