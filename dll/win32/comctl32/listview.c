/*
 * Listview control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999 Luc Tourangeau
 * Copyright 2000 Jason Mawdsley
 * Copyright 2001 CodeWeavers Inc.
 * Copyright 2002 Dimitrie O. Paun
 * Copyright 2009-2015 Nikolay Sivov
 * Copyright 2009 Owen Rudge for CodeWeavers
 * Copyright 2012-2013 Daniel Jelinski
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
 * TODO:
 *
 * Default Message Processing
 *   -- WM_CREATE: create the icon and small icon image lists at this point only if
 *      the LVS_SHAREIMAGELISTS style is not specified.
 *   -- WM_WINDOWPOSCHANGED: arrange the list items if the current view is icon
 *      or small icon and the LVS_AUTOARRANGE style is specified.
 *   -- WM_TIMER
 *   -- WM_WININICHANGE
 *
 * Features
 *   -- Hot item handling, mouse hovering
 *   -- Workareas support
 *   -- Tilemode support
 *   -- Groups support
 *
 * Bugs
 *   -- Expand large item in ICON mode when the cursor is flying over the icon or text.
 *   -- Support CustomDraw options for _WIN32_IE >= 0x560 (see NMLVCUSTOMDRAW docs).
 *   -- LVA_SNAPTOGRID not implemented
 *   -- LISTVIEW_ApproximateViewRect partially implemented
 *   -- LISTVIEW_StyleChanged doesn't handle some changes too well
 *
 * Speedups
 *   -- LISTVIEW_GetNextItem needs to be rewritten. It is currently
 *      linear in the number of items in the list, and this is
 *      unacceptable for large lists.
 *   -- if list is sorted by item text LISTVIEW_InsertItemT could use
 *      binary search to calculate item index (e.g. DPA_Search()).
 *      This requires sorted state to be reliably tracked in item modifiers.
 *   -- we should keep an ordered array of coordinates in iconic mode.
 *      This would allow framing items (iterator_frameditems),
 *      and finding the nearest item (LVFI_NEARESTXY) a lot more efficiently.
 *
 * Flags
 *   -- LVIF_COLUMNS
 *   -- LVIF_GROUPID
 *
 * States
 *   -- LVIS_ACTIVATING (not currently supported by comctl32.dll version 6.0)
 *   -- LVIS_DROPHILITED
 *
 * Styles
 *   -- LVS_NOLABELWRAP
 *   -- LVS_NOSCROLL (see Q137520)
 *   -- LVS_ALIGNTOP
 *
 * Extended Styles
 *   -- LVS_EX_BORDERSELECT
 *   -- LVS_EX_FLATSB
 *   -- LVS_EX_INFOTIP
 *   -- LVS_EX_LABELTIP
 *   -- LVS_EX_MULTIWORKAREAS
 *   -- LVS_EX_REGIONAL
 *   -- LVS_EX_SIMPLESELECT
 *   -- LVS_EX_TWOCLICKACTIVATE
 *   -- LVS_EX_UNDERLINECOLD
 *   -- LVS_EX_UNDERLINEHOT
 *   
 * Notifications:
 *   -- LVN_BEGINSCROLL, LVN_ENDSCROLL
 *   -- LVN_GETINFOTIP
 *   -- LVN_HOTTRACK
 *   -- LVN_SETDISPINFO
 *
 * Messages:
 *   -- LVM_ENABLEGROUPVIEW
 *   -- LVM_GETBKIMAGE
 *   -- LVM_GETGROUPINFO, LVM_SETGROUPINFO
 *   -- LVM_GETGROUPMETRICS, LVM_SETGROUPMETRICS
 *   -- LVM_GETINSERTMARK, LVM_SETINSERTMARK
 *   -- LVM_GETINSERTMARKCOLOR, LVM_SETINSERTMARKCOLOR
 *   -- LVM_GETINSERTMARKRECT
 *   -- LVM_GETNUMBEROFWORKAREAS
 *   -- LVM_GETOUTLINECOLOR, LVM_SETOUTLINECOLOR
 *   -- LVM_GETISEARCHSTRINGW, LVM_GETISEARCHSTRINGA
 *   -- LVM_GETTILEINFO, LVM_SETTILEINFO
 *   -- LVM_GETTILEVIEWINFO, LVM_SETTILEVIEWINFO
 *   -- LVM_GETWORKAREAS, LVM_SETWORKAREAS
 *   -- LVM_HASGROUP, LVM_INSERTGROUP, LVM_REMOVEGROUP, LVM_REMOVEALLGROUPS
 *   -- LVM_INSERTGROUPSORTED
 *   -- LVM_INSERTMARKHITTEST
 *   -- LVM_ISGROUPVIEWENABLED
 *   -- LVM_MOVEGROUP
 *   -- LVM_MOVEITEMTOGROUP
 *   -- LVM_SETINFOTIP
 *   -- LVM_SETTILEWIDTH
 *   -- LVM_SORTGROUPS
 *
 * Macros:
 *   -- ListView_GetHoverTime, ListView_SetHoverTime
 *   -- ListView_GetISearchString
 *   -- ListView_GetNumberOfWorkAreas
 *   -- ListView_GetWorkAreas, ListView_SetWorkAreas
 *
 * Functions:
 *   -- LVGroupComparE
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vsstyle.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(listview);

typedef struct tagCOLUMN_INFO
{
  RECT rcHeader;	/* tracks the header's rectangle */
  INT fmt;		/* same as LVCOLUMN.fmt */
  INT cxMin;
} COLUMN_INFO;

typedef struct tagITEMHDR
{
  LPWSTR pszText;
  INT iImage;
} ITEMHDR, *LPITEMHDR;

typedef struct tagSUBITEM_INFO
{
  ITEMHDR hdr;
  INT iSubItem;
} SUBITEM_INFO;

typedef struct tagITEM_ID ITEM_ID;

typedef struct tagITEM_INFO
{
  ITEMHDR hdr;
  UINT state;
  LPARAM lParam;
  INT iIndent;
  ITEM_ID *id;
} ITEM_INFO;

struct tagITEM_ID
{
  UINT id;   /* item id */
  HDPA item; /* link to item data */
};

typedef struct tagRANGE
{
  INT lower;
  INT upper;
} RANGE;

typedef struct tagRANGES
{
  HDPA hdpa;
} *RANGES;

typedef struct tagITERATOR
{
  INT nItem;
  INT nSpecial;
  RANGE range;
  RANGES ranges;
  INT index;
} ITERATOR;

typedef struct tagDELAYED_ITEM_EDIT
{
  BOOL fEnabled;
  INT iItem;
} DELAYED_ITEM_EDIT;

enum notification_mask
{
  NOTIFY_MASK_ITEM_CHANGE = 0x1,
  NOTIFY_MASK_END_LABEL_EDIT = 0x2,
  NOTIFY_MASK_UNMASK_ALL = 0xffffffff
};

typedef struct tagLISTVIEW_INFO
{
  /* control window */
  HWND hwndSelf;
  RECT rcList;                 /* This rectangle is really the window
				* client rectangle possibly reduced by the 
				* horizontal scroll bar and/or header - see 
				* LISTVIEW_UpdateSize. This rectangle offset
				* by the LISTVIEW_GetOrigin value is in
				* client coordinates   */

  /* notification window */
  SHORT notifyFormat;
  HWND hwndNotify;
  DWORD notify_mask;
  UINT uCallbackMask;

  /* tooltips */
  HWND hwndToolTip;

  /* items */
  INT nItemCount;		/* the number of items in the list */
  HDPA hdpaItems;               /* array ITEM_INFO pointers */
  HDPA hdpaItemIds;             /* array of ITEM_ID pointers */
  HDPA hdpaPosX;		/* maintains the (X, Y) coordinates of the */
  HDPA hdpaPosY;		/* items in LVS_ICON, and LVS_SMALLICON modes */
  RANGES selectionRanges;
  INT nSelectionMark;           /* item to start next multiselection from */
  INT nHotItem;

  /* columns */
  HDPA hdpaColumns;		/* array of COLUMN_INFO pointers */
  BOOL colRectsDirty;		/* trigger column rectangles requery from header */
  INT selected_column;          /* index for LVM_SETSELECTEDCOLUMN/LVM_GETSELECTEDCOLUMN */

  /* item metrics */
  BOOL bNoItemMetrics;		/* flags if item metrics are not yet computed */
  INT nItemHeight;
  INT nItemWidth;

  /* style */
  DWORD dwStyle;		/* the cached window GWL_STYLE */
  DWORD dwLvExStyle;		/* extended listview style */
  DWORD uView;			/* current view available through LVM_[G,S]ETVIEW */

  /* edit item */
  HWND hwndEdit;
  WNDPROC EditWndProc;
  INT nEditLabelItem;
  DELAYED_ITEM_EDIT itemEdit;   /* Pointer to this structure will be the timer ID */

  /* icons */
  HIMAGELIST himlNormal;
  HIMAGELIST himlSmall;
  HIMAGELIST himlState;
  SIZE iconSize;
  BOOL autoSpacing;
  SIZE iconSpacing;
  SIZE iconStateSize;
  POINT currIconPos;        /* this is the position next icon will be placed */

  /* header */
  HWND hwndHeader;
  INT xTrackLine;           /* The x coefficient of the track line or -1 if none */

  /* marquee selection */
  BOOL bMarqueeSelect;      /* marquee selection/highlight underway */
  BOOL bScrolling;
  RECT marqueeRect;         /* absolute coordinates of marquee selection */
  RECT marqueeDrawRect;     /* relative coordinates for drawing marquee */
  POINT marqueeOrigin;      /* absolute coordinates of marquee click origin */

  /* focus drawing */
  BOOL bFocus;              /* control has focus */
  INT nFocusedItem;
  RECT rcFocus;             /* focus bounds */

  /* colors */
  HBRUSH hBkBrush;
  COLORREF clrBk;
  COLORREF clrText;
  COLORREF clrTextBk;
  HBITMAP hBkBitmap;

  /* font */
  HFONT hDefaultFont;
  HFONT hFont;
  INT ntmHeight;            /* Some cached metrics of the font used */
  INT ntmMaxCharWidth;      /* by the listview to draw items */
  INT nEllipsisWidth;

  /* mouse operation */
  BOOL bLButtonDown;
  BOOL bDragging;
  POINT ptClickPos;         /* point where the user clicked */
  INT nLButtonDownItem;     /* tracks item to reset multiselection on WM_LBUTTONUP */
  DWORD dwHoverTime;
  HCURSOR hHotCursor;
  INT cWheelRemainder;

  /* keyboard operation */
  DWORD lastKeyPressTimestamp;
  WPARAM charCode;
  INT nSearchParamLength;
  WCHAR szSearchParam[ MAX_PATH ];

  /* painting */
  BOOL bIsDrawing;         /* Drawing in progress */
  INT nMeasureItemHeight;  /* WM_MEASUREITEM result */
  BOOL redraw;             /* WM_SETREDRAW switch */

  /* misc */
  DWORD iVersion;          /* CCM_[G,S]ETVERSION */
} LISTVIEW_INFO;

/*
 * constants
 */
/* How many we debug buffer to allocate */
#define DEBUG_BUFFERS 20
/* The size of a single debug buffer */
#define DEBUG_BUFFER_SIZE 256

/* Internal interface to LISTVIEW_HScroll and LISTVIEW_VScroll */
#define SB_INTERNAL      -1

/* maximum size of a label */
#define DISP_TEXT_SIZE 260

/* padding for items in list and small icon display modes */
#define WIDTH_PADDING 12

/* padding for items in list, report and small icon display modes */
#define HEIGHT_PADDING 1

/* offset of items in report display mode */
#define REPORT_MARGINX 2

/* padding for icon in large icon display mode
 *   ICON_TOP_PADDING_NOTHITABLE - space between top of box and area
 *                                 that HITTEST will see.
 *   ICON_TOP_PADDING_HITABLE - spacing between above and icon.
 *   ICON_TOP_PADDING - sum of the two above.
 *   ICON_BOTTOM_PADDING - between bottom of icon and top of text
 *   LABEL_HOR_PADDING - between text and sides of box
 *   LABEL_VERT_PADDING - between bottom of text and end of box
 *
 *   ICON_LR_PADDING - additional width above icon size.
 *   ICON_LR_HALF - half of the above value
 */
#define ICON_TOP_PADDING_NOTHITABLE  2
#define ICON_TOP_PADDING_HITABLE     2
#define ICON_TOP_PADDING (ICON_TOP_PADDING_NOTHITABLE + ICON_TOP_PADDING_HITABLE)
#define ICON_BOTTOM_PADDING          4
#define LABEL_HOR_PADDING            5
#define LABEL_VERT_PADDING           7
#define ICON_LR_PADDING              16
#define ICON_LR_HALF                 (ICON_LR_PADDING/2)

/* default label width for items in list and small icon display modes */
#define DEFAULT_LABEL_WIDTH 40
/* maximum select rectangle width for empty text item in LV_VIEW_DETAILS */
#define MAX_EMPTYTEXT_SELECT_WIDTH 80

/* default column width for items in list display mode */
#define DEFAULT_COLUMN_WIDTH 128

/* Size of "line" scroll for V & H scrolls */
#define LISTVIEW_SCROLL_ICON_LINE_SIZE 37

/* Padding between image and label */
#define IMAGE_PADDING  2

/* Padding behind the label */
#define TRAILING_LABEL_PADDING  12
#define TRAILING_HEADER_PADDING  11

/* Border for the icon caption */
#define CAPTION_BORDER  2

/* Standard DrawText flags */
#define LV_ML_DT_FLAGS  (DT_TOP | DT_NOPREFIX | DT_EDITCONTROL | DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS)
#define LV_FL_DT_FLAGS  (DT_TOP | DT_NOPREFIX | DT_EDITCONTROL | DT_CENTER | DT_WORDBREAK | DT_NOCLIP)
#define LV_SL_DT_FLAGS  (DT_VCENTER | DT_NOPREFIX | DT_EDITCONTROL | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS)

/* Image index from state */
#define STATEIMAGEINDEX(x) (((x) & LVIS_STATEIMAGEMASK) >> 12)

/* The time in milliseconds to reset the search in the list */
#define KEY_DELAY       450

/* Dump the LISTVIEW_INFO structure to the debug channel */
#define LISTVIEW_DUMP(iP) do { \
  TRACE("hwndSelf=%p, clrBk=%#lx, clrText=%#lx, clrTextBk=%#lx, ItemHeight=%d, ItemWidth=%d, Style=%#lx\n", \
        iP->hwndSelf, iP->clrBk, iP->clrText, iP->clrTextBk, \
        iP->nItemHeight, iP->nItemWidth, iP->dwStyle); \
  TRACE("hwndSelf=%p, himlNor=%p, himlSml=%p, himlState=%p, Focused=%d, Hot=%d, exStyle=%#lx, Focus=%d\n", \
        iP->hwndSelf, iP->himlNormal, iP->himlSmall, iP->himlState, \
        iP->nFocusedItem, iP->nHotItem, iP->dwLvExStyle, iP->bFocus ); \
  TRACE("hwndSelf=%p, ntmH=%d, icSz.cx=%ld, icSz.cy=%ld, icSp.cx=%ld, icSp.cy=%ld, notifyFmt=%d\n", \
        iP->hwndSelf, iP->ntmHeight, iP->iconSize.cx, iP->iconSize.cy, \
        iP->iconSpacing.cx, iP->iconSpacing.cy, iP->notifyFormat); \
  TRACE("hwndSelf=%p, rcList=%s\n", iP->hwndSelf, wine_dbgstr_rect(&iP->rcList)); \
} while(0)

static const WCHAR themeClass[] = L"ListView";

/*
 * forward declarations
 */
static BOOL LISTVIEW_GetItemT(const LISTVIEW_INFO *, LPLVITEMW, BOOL);
static void LISTVIEW_GetItemBox(const LISTVIEW_INFO *, INT, LPRECT);
static void LISTVIEW_GetItemOrigin(const LISTVIEW_INFO *, INT, LPPOINT);
static BOOL LISTVIEW_GetItemPosition(const LISTVIEW_INFO *, INT, LPPOINT);
static BOOL LISTVIEW_GetItemRect(const LISTVIEW_INFO *, INT, LPRECT);
static void LISTVIEW_GetOrigin(const LISTVIEW_INFO *, LPPOINT);
static BOOL LISTVIEW_GetViewRect(const LISTVIEW_INFO *, LPRECT);
static void LISTVIEW_UpdateSize(LISTVIEW_INFO *);
static LRESULT LISTVIEW_Command(LISTVIEW_INFO *, WPARAM, LPARAM);
static INT LISTVIEW_GetStringWidthT(const LISTVIEW_INFO *, LPCWSTR, BOOL);
static BOOL LISTVIEW_KeySelection(LISTVIEW_INFO *, INT, BOOL);
static UINT LISTVIEW_GetItemState(const LISTVIEW_INFO *, INT, UINT);
static BOOL LISTVIEW_SetItemState(LISTVIEW_INFO *, INT, const LVITEMW *);
static VOID LISTVIEW_SetOwnerDataState(LISTVIEW_INFO *, INT, INT, const LVITEMW *);
static LRESULT LISTVIEW_VScroll(LISTVIEW_INFO *, INT, INT);
static LRESULT LISTVIEW_HScroll(LISTVIEW_INFO *, INT, INT);
static BOOL LISTVIEW_EnsureVisible(LISTVIEW_INFO *, INT, BOOL);
static HIMAGELIST LISTVIEW_SetImageList(LISTVIEW_INFO *, INT, HIMAGELIST);
static INT LISTVIEW_HitTest(const LISTVIEW_INFO *, LPLVHITTESTINFO, BOOL, BOOL);
static BOOL LISTVIEW_EndEditLabelT(LISTVIEW_INFO *, BOOL, BOOL);
static BOOL LISTVIEW_Scroll(LISTVIEW_INFO *, INT, INT);

/******** Text handling functions *************************************/

/* A text pointer is either NULL, LPSTR_TEXTCALLBACK, or points to a
 * text string. The string may be ANSI or Unicode, in which case
 * the boolean isW tells us the type of the string.
 *
 * The name of the function tell what type of strings it expects:
 *   W: Unicode, T: ANSI/Unicode - function of isW
 */

static inline BOOL is_text(LPCWSTR text)
{
    return text != NULL && text != LPSTR_TEXTCALLBACKW;
}

static inline int textlenT(LPCWSTR text, BOOL isW)
{
    return !is_text(text) ? 0 :
	   isW ? lstrlenW(text) : lstrlenA((LPCSTR)text);
}

static inline void textcpynT(LPWSTR dest, BOOL isDestW, LPCWSTR src, BOOL isSrcW, INT max)
{
    if (isDestW)
	if (isSrcW) lstrcpynW(dest, src, max);
	else MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, dest, max);
    else
	if (isSrcW) WideCharToMultiByte(CP_ACP, 0, src, -1, (LPSTR)dest, max, NULL, NULL);
	else lstrcpynA((LPSTR)dest, (LPCSTR)src, max);
}

static inline LPWSTR textdupTtoW(LPCWSTR text, BOOL isW)
{
    LPWSTR wstr = (LPWSTR)text;

    if (!isW && is_text(text))
    {
	INT len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)text, -1, NULL, 0);
	wstr = Alloc(len * sizeof(WCHAR));
	if (wstr) MultiByteToWideChar(CP_ACP, 0, (LPCSTR)text, -1, wstr, len);
    }
    TRACE("   wstr=%s\n", text == LPSTR_TEXTCALLBACKW ?  "(callback)" : debugstr_w(wstr));
    return wstr;
}

static inline void textfreeT(LPWSTR wstr, BOOL isW)
{
    if (!isW && is_text(wstr)) Free (wstr);
}

/*
 * dest is a pointer to a Unicode string
 * src is a pointer to a string (Unicode if isW, ANSI if !isW)
 */
static BOOL textsetptrT(LPWSTR *dest, LPCWSTR src, BOOL isW)
{
    BOOL bResult = TRUE;
    
    if (src == LPSTR_TEXTCALLBACKW)
    {
	if (is_text(*dest)) Free(*dest);
	*dest = LPSTR_TEXTCALLBACKW;
    }
    else
    {
	LPWSTR pszText = textdupTtoW(src, isW);
	if (*dest == LPSTR_TEXTCALLBACKW) *dest = NULL;
	bResult = Str_SetPtrW(dest, pszText);
	textfreeT(pszText, isW);
    }
    return bResult;
}

/*
 * compares a Unicode to a Unicode/ANSI text string
 */
static inline int textcmpWT(LPCWSTR aw, LPCWSTR bt, BOOL isW)
{
    if (!aw) return bt ? -1 : 0;
    if (!bt) return 1;
    if (aw == LPSTR_TEXTCALLBACKW)
	return bt == LPSTR_TEXTCALLBACKW ? 1 : -1;
    if (bt != LPSTR_TEXTCALLBACKW)
    {
	LPWSTR bw = textdupTtoW(bt, isW);
	int r = bw ? lstrcmpW(aw, bw) : 1;
	textfreeT(bw, isW);
	return r;
    }	    
	    
    return 1;
}

/******** Debugging functions *****************************************/

static inline LPCSTR debugtext_t(LPCWSTR text, BOOL isW)
{
    if (text == LPSTR_TEXTCALLBACKW) return "(callback)";
    return isW ? debugstr_w(text) : debugstr_a((LPCSTR)text);
}

static inline LPCSTR debugtext_tn(LPCWSTR text, BOOL isW, INT n)
{
    if (text == LPSTR_TEXTCALLBACKW) return "(callback)";
    n = min(textlenT(text, isW), n);
    return isW ? debugstr_wn(text, n) : debugstr_an((LPCSTR)text, n);
}

static char* debug_getbuf(void)
{
    static int index = 0;
    static char buffers[DEBUG_BUFFERS][DEBUG_BUFFER_SIZE];
    return buffers[index++ % DEBUG_BUFFERS];
}

static inline const char* debugrange(const RANGE *lprng)
{
    if (!lprng) return "(null)";
    return wine_dbg_sprintf("[%d, %d]", lprng->lower, lprng->upper);
}

static const char* debugscrollinfo(const SCROLLINFO *pScrollInfo)
{
    char* buf = debug_getbuf(), *text = buf;
    int len, size = DEBUG_BUFFER_SIZE;

    if (pScrollInfo == NULL) return "(null)";
    len = snprintf(buf, size, "{cbSize=%u, ", pScrollInfo->cbSize);
    if (len == -1) goto end;
    buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_RANGE)
	len = snprintf(buf, size, "nMin=%d, nMax=%d, ", pScrollInfo->nMin, pScrollInfo->nMax);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_PAGE)
	len = snprintf(buf, size, "nPage=%u, ", pScrollInfo->nPage);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_POS)
	len = snprintf(buf, size, "nPos=%d, ", pScrollInfo->nPos);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_TRACKPOS)
	len = snprintf(buf, size, "nTrackPos=%d, ", pScrollInfo->nTrackPos);
    else len = 0;
    if (len == -1) goto end;
    buf += len;
    goto undo;
end:
    buf = text + strlen(text);
undo:
    if (buf - text > 2) { buf[-2] = '}'; buf[-1] = 0; }
    return text;
} 

static const char* debugnmlistview(const NMLISTVIEW *plvnm)
{
    if (!plvnm) return "(null)";
    return wine_dbg_sprintf("iItem=%d, iSubItem=%d, uNewState=0x%x,"
	         " uOldState=0x%x, uChanged=0x%x, ptAction=%s, lParam=%Id",
	         plvnm->iItem, plvnm->iSubItem, plvnm->uNewState, plvnm->uOldState,
		 plvnm->uChanged, wine_dbgstr_point(&plvnm->ptAction), plvnm->lParam);
}

static const char* debuglvitem_t(const LVITEMW *lpLVItem, BOOL isW)
{
    char* buf = debug_getbuf(), *text = buf;
    int len, size = DEBUG_BUFFER_SIZE;
    
    if (lpLVItem == NULL) return "(null)";
    len = snprintf(buf, size, "{iItem=%d, iSubItem=%d, ", lpLVItem->iItem, lpLVItem->iSubItem);
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpLVItem->mask & LVIF_STATE)
	len = snprintf(buf, size, "state=%x, stateMask=%x, ", lpLVItem->state, lpLVItem->stateMask);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpLVItem->mask & LVIF_TEXT)
	len = snprintf(buf, size, "pszText=%s, cchTextMax=%d, ", debugtext_tn(lpLVItem->pszText, isW, 80), lpLVItem->cchTextMax);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpLVItem->mask & LVIF_IMAGE)
	len = snprintf(buf, size, "iImage=%d, ", lpLVItem->iImage);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpLVItem->mask & LVIF_PARAM)
	len = snprintf(buf, size, "lParam=%Ix, ", lpLVItem->lParam);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpLVItem->mask & LVIF_INDENT)
	len = snprintf(buf, size, "iIndent=%d, ", lpLVItem->iIndent);
    else len = 0;
    if (len == -1) goto end;
    buf += len;
    goto undo;
end:
    buf = text + strlen(text);
undo:
    if (buf - text > 2) { buf[-2] = '}'; buf[-1] = 0; }
    return text;
}

static const char* debuglvcolumn_t(const LVCOLUMNW *lpColumn, BOOL isW)
{
    char* buf = debug_getbuf(), *text = buf;
    int len, size = DEBUG_BUFFER_SIZE;
    
    if (lpColumn == NULL) return "(null)";
    len = snprintf(buf, size, "{");
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpColumn->mask & LVCF_SUBITEM)
	len = snprintf(buf, size, "iSubItem=%d, ",  lpColumn->iSubItem);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpColumn->mask & LVCF_FMT)
	len = snprintf(buf, size, "fmt=%x, ", lpColumn->fmt);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpColumn->mask & LVCF_WIDTH)
	len = snprintf(buf, size, "cx=%d, ", lpColumn->cx);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpColumn->mask & LVCF_TEXT)
	len = snprintf(buf, size, "pszText=%s, cchTextMax=%d, ", debugtext_tn(lpColumn->pszText, isW, 80), lpColumn->cchTextMax);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpColumn->mask & LVCF_IMAGE)
	len = snprintf(buf, size, "iImage=%d, ", lpColumn->iImage);
    else len = 0;
    if (len == -1) goto end;
    buf += len; size -= len;
    if (lpColumn->mask & LVCF_ORDER)
	len = snprintf(buf, size, "iOrder=%d, ", lpColumn->iOrder);
    else len = 0;
    if (len == -1) goto end;
    buf += len;
    goto undo;
end:
    buf = text + strlen(text);
undo:
    if (buf - text > 2) { buf[-2] = '}'; buf[-1] = 0; }
    return text;
}

static const char* debuglvhittestinfo(const LVHITTESTINFO *lpht)
{
    if (!lpht) return "(null)";

    return wine_dbg_sprintf("{pt=%s, flags=0x%x, iItem=%d, iSubItem=%d}",
		 wine_dbgstr_point(&lpht->pt), lpht->flags, lpht->iItem, lpht->iSubItem);
}

/* Return the corresponding text for a given scroll value */
static inline LPCSTR debugscrollcode(int nScrollCode)
{
  switch(nScrollCode)
  {
  case SB_LINELEFT: return "SB_LINELEFT";
  case SB_LINERIGHT: return "SB_LINERIGHT";
  case SB_PAGELEFT: return "SB_PAGELEFT";
  case SB_PAGERIGHT: return "SB_PAGERIGHT";
  case SB_THUMBPOSITION: return "SB_THUMBPOSITION";
  case SB_THUMBTRACK: return "SB_THUMBTRACK";
  case SB_ENDSCROLL: return "SB_ENDSCROLL";
  case SB_INTERNAL: return "SB_INTERNAL";
  default: return "unknown";
  }
}


/******** Notification functions ************************************/

static int get_ansi_notification(UINT unicodeNotificationCode)
{
    switch (unicodeNotificationCode)
    {
    case LVN_BEGINLABELEDITA:
    case LVN_BEGINLABELEDITW: return LVN_BEGINLABELEDITA;
    case LVN_ENDLABELEDITA:
    case LVN_ENDLABELEDITW: return LVN_ENDLABELEDITA;
    case LVN_GETDISPINFOA:
    case LVN_GETDISPINFOW: return LVN_GETDISPINFOA;
    case LVN_SETDISPINFOA:
    case LVN_SETDISPINFOW: return LVN_SETDISPINFOA;
    case LVN_ODFINDITEMA:
    case LVN_ODFINDITEMW: return LVN_ODFINDITEMA;
    case LVN_GETINFOTIPA:
    case LVN_GETINFOTIPW: return LVN_GETINFOTIPA;
    /* header forwards */
    case HDN_TRACKA:
    case HDN_TRACKW: return HDN_TRACKA;
    case HDN_ENDTRACKA:
    case HDN_ENDTRACKW: return HDN_ENDTRACKA;
    case HDN_BEGINDRAG: return HDN_BEGINDRAG;
    case HDN_ENDDRAG: return HDN_ENDDRAG;
    case HDN_ITEMCHANGINGA:
    case HDN_ITEMCHANGINGW: return HDN_ITEMCHANGINGA;
    case HDN_ITEMCHANGEDA:
    case HDN_ITEMCHANGEDW: return HDN_ITEMCHANGEDA;
    case HDN_ITEMCLICKA:
    case HDN_ITEMCLICKW: return HDN_ITEMCLICKA;
    case HDN_DIVIDERDBLCLICKA:
    case HDN_DIVIDERDBLCLICKW: return HDN_DIVIDERDBLCLICKA;
    default: break;
    }
    FIXME("unknown notification %x\n", unicodeNotificationCode);
    return unicodeNotificationCode;
}

/* forwards header notifications to listview parent */
static LRESULT notify_forward_header(const LISTVIEW_INFO *infoPtr, NMHEADERW *lpnmhW)
{
    LPCWSTR text = NULL, filter = NULL;
    LRESULT ret;
    NMHEADERA *lpnmh = (NMHEADERA*) lpnmhW;

    /* on unicode format exit earlier */
    if (infoPtr->notifyFormat == NFR_UNICODE)
        return SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, lpnmh->hdr.idFrom,
                            (LPARAM)lpnmh);

    /* header always supplies unicode notifications,
       all we have to do is to convert strings to ANSI */
    if (lpnmh->pitem)
    {
        /* convert item text */
        if (lpnmh->pitem->mask & HDI_TEXT)
        {
            text = (LPCWSTR)lpnmh->pitem->pszText;
            lpnmh->pitem->pszText = NULL;
            Str_SetPtrWtoA(&lpnmh->pitem->pszText, text);
        }
        /* convert filter text */
        if ((lpnmh->pitem->mask & HDI_FILTER) && (lpnmh->pitem->type == HDFT_ISSTRING) &&
             lpnmh->pitem->pvFilter)
        {
            filter = (LPCWSTR)((HD_TEXTFILTERA*)lpnmh->pitem->pvFilter)->pszText;
            ((HD_TEXTFILTERA*)lpnmh->pitem->pvFilter)->pszText = NULL;
            Str_SetPtrWtoA(&((HD_TEXTFILTERA*)lpnmh->pitem->pvFilter)->pszText, filter);
        }
    }
    lpnmh->hdr.code = get_ansi_notification(lpnmh->hdr.code);

    ret = SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, lpnmh->hdr.idFrom,
                       (LPARAM)lpnmh);

    /* cleanup */
    if(text)
    {
        Free(lpnmh->pitem->pszText);
        lpnmh->pitem->pszText = (LPSTR)text;
    }
    if(filter)
    {
        Free(((HD_TEXTFILTERA*)lpnmh->pitem->pvFilter)->pszText);
        ((HD_TEXTFILTERA*)lpnmh->pitem->pvFilter)->pszText = (LPSTR)filter;
    }

    return ret;
}

static LRESULT notify_hdr(const LISTVIEW_INFO *infoPtr, INT code, LPNMHDR pnmh)
{
    LRESULT result;
    
    TRACE("(code=%d)\n", code);

    pnmh->hwndFrom = infoPtr->hwndSelf;
    pnmh->idFrom = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    pnmh->code = code;
    result = SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, pnmh->idFrom, (LPARAM)pnmh);

    TRACE("  <= %Id\n", result);

    return result;
}

static inline BOOL notify(const LISTVIEW_INFO *infoPtr, INT code)
{
    NMHDR nmh;
    HWND hwnd = infoPtr->hwndSelf;
    notify_hdr(infoPtr, code, &nmh);
    return IsWindow(hwnd);
}

static inline void notify_itemactivate(const LISTVIEW_INFO *infoPtr, const LVHITTESTINFO *htInfo)
{
    NMITEMACTIVATE nmia;
    LVITEMW item;

    nmia.uNewState = 0;
    nmia.uOldState = 0;
    nmia.uChanged  = 0;
    nmia.uKeyFlags = 0;

    item.mask = LVIF_PARAM|LVIF_STATE;
    item.iItem = htInfo->iItem;
    item.iSubItem = 0;
    item.stateMask = (UINT)-1;
    if (LISTVIEW_GetItemT(infoPtr, &item, TRUE)) {
        nmia.lParam = item.lParam;
        nmia.uOldState = item.state;
        nmia.uNewState = item.state | LVIS_ACTIVATING;
        nmia.uChanged  = LVIF_STATE;
    }

    nmia.iItem = htInfo->iItem;
    nmia.iSubItem = htInfo->iSubItem;
    nmia.ptAction = htInfo->pt;

    if (GetKeyState(VK_SHIFT) & 0x8000) nmia.uKeyFlags |= LVKF_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000) nmia.uKeyFlags |= LVKF_CONTROL;
    if (GetKeyState(VK_MENU) & 0x8000) nmia.uKeyFlags |= LVKF_ALT;

    notify_hdr(infoPtr, LVN_ITEMACTIVATE, (LPNMHDR)&nmia);
}

static inline LRESULT notify_listview(const LISTVIEW_INFO *infoPtr, INT code, LPNMLISTVIEW plvnm)
{
    TRACE("(code=%d, plvnm=%s)\n", code, debugnmlistview(plvnm));
    return notify_hdr(infoPtr, code, (LPNMHDR)plvnm);
}

/* Handles NM_DBLCLK, NM_CLICK, NM_RDBLCLK, NM_RCLICK. Only NM_RCLICK return value is used. */
static BOOL notify_click(const LISTVIEW_INFO *infoPtr, INT code, const LVHITTESTINFO *lvht)
{
    NMITEMACTIVATE nmia;
    LVITEMW item;
    HWND hwnd = infoPtr->hwndSelf;
    LRESULT ret;

    TRACE("code=%d, lvht=%s\n", code, debuglvhittestinfo(lvht)); 
    ZeroMemory(&nmia, sizeof(nmia));
    nmia.iItem = lvht->iItem;
    nmia.iSubItem = lvht->iSubItem;
    nmia.ptAction = lvht->pt;
    item.mask = LVIF_PARAM;
    item.iItem = lvht->iItem;
    item.iSubItem = 0;
    if (LISTVIEW_GetItemT(infoPtr, &item, TRUE)) nmia.lParam = item.lParam;
    ret = notify_hdr(infoPtr, code, (NMHDR*)&nmia);
    return IsWindow(hwnd) && (code == NM_RCLICK ? !ret : TRUE);
}

static BOOL notify_deleteitem(const LISTVIEW_INFO *infoPtr, INT nItem)
{
    NMLISTVIEW nmlv;
    LVITEMW item;
    HWND hwnd = infoPtr->hwndSelf;

    ZeroMemory(&nmlv, sizeof (NMLISTVIEW));
    nmlv.iItem = nItem;
    item.mask = LVIF_PARAM;
    item.iItem = nItem;
    item.iSubItem = 0;
    if (LISTVIEW_GetItemT(infoPtr, &item, TRUE)) nmlv.lParam = item.lParam;
    notify_listview(infoPtr, LVN_DELETEITEM, &nmlv);
    return IsWindow(hwnd);
}

/*
  Send notification. depends on dispinfoW having same
  structure as dispinfoA.
  infoPtr : listview struct
  code : *Unicode* notification code
  pdi : dispinfo structure (can be unicode or ansi)
  isW : TRUE if dispinfo is Unicode
*/
static BOOL notify_dispinfoT(const LISTVIEW_INFO *infoPtr, UINT code, LPNMLVDISPINFOW pdi, BOOL isW)
{
    INT length = 0, ret_length;
    LPWSTR buffer = NULL, ret_text;
    BOOL return_ansi = FALSE;
    BOOL return_unicode = FALSE;
    BOOL ret;

    if ((pdi->item.mask & LVIF_TEXT) && is_text(pdi->item.pszText))
    {
	return_unicode = ( isW && infoPtr->notifyFormat == NFR_ANSI);
	return_ansi    = (!isW && infoPtr->notifyFormat == NFR_UNICODE);
    }

    ret_length = pdi->item.cchTextMax;
    ret_text = pdi->item.pszText;

    if (return_unicode || return_ansi)
    {
        if (code != LVN_GETDISPINFOW)
        {
            length = return_ansi ?
       		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pdi->item.pszText, -1, NULL, 0):
       		WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, NULL, 0, NULL, NULL);
        }
        else
        {
            length = pdi->item.cchTextMax;
            *pdi->item.pszText = 0; /* make sure we don't process garbage */
        }

        buffer = Alloc( length * (return_ansi ? sizeof(WCHAR) : sizeof(CHAR)) );
        if (!buffer) return FALSE;

        if (return_ansi)
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pdi->item.pszText, -1,
	                        buffer, length);
        else
            WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, (LPSTR) buffer,
	                        length, NULL, NULL);

        pdi->item.pszText = buffer;
        pdi->item.cchTextMax = length;
    }

    if (infoPtr->notifyFormat == NFR_ANSI)
        code = get_ansi_notification(code);

    TRACE(" pdi->item=%s\n", debuglvitem_t(&pdi->item, infoPtr->notifyFormat != NFR_ANSI));
    ret = notify_hdr(infoPtr, code, &pdi->hdr);
    TRACE(" resulting code=%d\n", pdi->hdr.code);

    if (return_ansi || return_unicode)
    {
        if (return_ansi && (pdi->hdr.code == LVN_GETDISPINFOA))
        {
            strcpy((char*)ret_text, (char*)pdi->item.pszText);
        }
        else if (return_unicode && (pdi->hdr.code == LVN_GETDISPINFOW))
        {
            lstrcpyW(ret_text, pdi->item.pszText);
        }
        else if (return_ansi) /* note : pointer can be changed by app ! */
        {
	    WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, (LPSTR) ret_text,
                ret_length, NULL, NULL);
        }
        else
            MultiByteToWideChar(CP_ACP, 0, (LPSTR) pdi->item.pszText, -1,
                ret_text, ret_length);

        pdi->item.pszText = ret_text; /* restores our buffer */
        pdi->item.cchTextMax = ret_length;

        Free(buffer);
        return ret;
    }

    /* if dispinfo holder changed notification code then convert */
    if (!isW && (pdi->hdr.code == LVN_GETDISPINFOW) && (pdi->item.mask & LVIF_TEXT))
    {
        length = WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, NULL, 0, NULL, NULL);

        buffer = Alloc(length * sizeof(CHAR));
        if (!buffer) return FALSE;

        WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, (LPSTR) buffer,
                ret_length, NULL, NULL);

        strcpy((LPSTR)pdi->item.pszText, (LPSTR)buffer);
        Free(buffer);
    }

    return ret;
}

static void customdraw_fill(NMLVCUSTOMDRAW *lpnmlvcd, const LISTVIEW_INFO *infoPtr, HDC hdc,
			    const RECT *rcBounds, const LVITEMW *lplvItem)
{
    ZeroMemory(lpnmlvcd, sizeof(NMLVCUSTOMDRAW));
    lpnmlvcd->nmcd.hdc = hdc;
    lpnmlvcd->nmcd.rc = *rcBounds;
    lpnmlvcd->clrTextBk = infoPtr->clrTextBk;
    lpnmlvcd->clrText   = infoPtr->clrText;
    if (!lplvItem) return;
    lpnmlvcd->nmcd.dwItemSpec = lplvItem->iItem + 1;
    lpnmlvcd->iSubItem = lplvItem->iSubItem;
    if (lplvItem->state & LVIS_SELECTED) lpnmlvcd->nmcd.uItemState |= CDIS_SELECTED;
    if (lplvItem->state & LVIS_FOCUSED) lpnmlvcd->nmcd.uItemState |= CDIS_FOCUS;
    if (lplvItem->iItem == infoPtr->nHotItem) lpnmlvcd->nmcd.uItemState |= CDIS_HOT;
    lpnmlvcd->nmcd.lItemlParam = lplvItem->lParam;
}

static inline DWORD notify_customdraw (const LISTVIEW_INFO *infoPtr, DWORD dwDrawStage, NMLVCUSTOMDRAW *lpnmlvcd)
{
    BOOL isForItem = (lpnmlvcd->nmcd.dwItemSpec != 0);
    DWORD result;

    lpnmlvcd->nmcd.dwDrawStage = dwDrawStage;
    if (isForItem) lpnmlvcd->nmcd.dwDrawStage |= CDDS_ITEM; 
    if (lpnmlvcd->iSubItem) lpnmlvcd->nmcd.dwDrawStage |= CDDS_SUBITEM;
    if (isForItem) lpnmlvcd->nmcd.dwItemSpec--;
    result = notify_hdr(infoPtr, NM_CUSTOMDRAW, &lpnmlvcd->nmcd.hdr);
    if (isForItem) lpnmlvcd->nmcd.dwItemSpec++;
    return result;
}

static void prepaint_setup (const LISTVIEW_INFO *infoPtr, HDC hdc, const NMLVCUSTOMDRAW *cd, BOOL SubItem)
{
    COLORREF backcolor, textcolor;

    backcolor = cd->clrTextBk;
    textcolor = cd->clrText;

    /* apparently, for selected items, we have to override the returned values */
    if (!SubItem)
    {
        if (cd->nmcd.uItemState & CDIS_SELECTED)
        {
            if (infoPtr->bFocus)
            {
                backcolor = comctl32_color.clrHighlight;
                textcolor = comctl32_color.clrHighlightText;
            }
            else if (infoPtr->dwStyle & LVS_SHOWSELALWAYS)
            {
                backcolor = comctl32_color.clr3dFace;
                textcolor = comctl32_color.clrBtnText;
            }
        }
    }

    if (backcolor == CLR_DEFAULT)
        backcolor = comctl32_color.clrWindow;
    if (textcolor == CLR_DEFAULT)
        textcolor = comctl32_color.clrWindowText;

    /* Set the text attributes */
    if (backcolor != CLR_NONE)
    {
	SetBkMode(hdc, OPAQUE);
	SetBkColor(hdc, backcolor);
    }
    else
	SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textcolor);
}

static inline DWORD notify_postpaint (const LISTVIEW_INFO *infoPtr, NMLVCUSTOMDRAW *lpnmlvcd)
{
    return notify_customdraw(infoPtr, CDDS_POSTPAINT, lpnmlvcd);
}

/* returns TRUE when repaint needed, FALSE otherwise */
static BOOL notify_measureitem(LISTVIEW_INFO *infoPtr)
{
    MEASUREITEMSTRUCT mis;
    mis.CtlType = ODT_LISTVIEW;
    mis.CtlID = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    mis.itemID = -1;
    mis.itemWidth = 0;
    mis.itemData = 0;
    mis.itemHeight= infoPtr->nItemHeight;
    SendMessageW(infoPtr->hwndNotify, WM_MEASUREITEM, mis.CtlID, (LPARAM)&mis);
    if (infoPtr->nItemHeight != max(mis.itemHeight, 1))
    {
        infoPtr->nMeasureItemHeight = infoPtr->nItemHeight = max(mis.itemHeight, 1);
        return TRUE;
    }
    return FALSE;
}

/******** Item iterator functions **********************************/

static RANGES ranges_create(int count);
static void ranges_destroy(RANGES ranges);
static BOOL ranges_add(RANGES ranges, RANGE range);
static BOOL ranges_del(RANGES ranges, RANGE range);
static void ranges_dump(RANGES ranges);

static inline BOOL ranges_additem(RANGES ranges, INT nItem)
{
    RANGE range = { nItem, nItem + 1 };

    return ranges_add(ranges, range);
}

static inline BOOL ranges_delitem(RANGES ranges, INT nItem)
{
    RANGE range = { nItem, nItem + 1 };

    return ranges_del(ranges, range);
}

/***
 * ITERATOR DOCUMENTATION
 *
 * The iterator functions allow for easy, and convenient iteration
 * over items of interest in the list. Typically, you create an
 * iterator, use it, and destroy it, as such:
 *   ITERATOR i;
 *
 *   iterator_xxxitems(&i, ...);
 *   while (iterator_{prev,next}(&i)
 *   {
 *       //code which uses i.nItem
 *   }
 *   iterator_destroy(&i);
 *
 *   where xxx is either: framed, or visible.
 * Note that it is important that the code destroys the iterator
 * after it's done with it, as the creation of the iterator may
 * allocate memory, which thus needs to be freed.
 * 
 * You can iterate both forwards, and backwards through the list,
 * by using iterator_next or iterator_prev respectively.
 * 
 * Lower numbered items are draw on top of higher number items in
 * LVS_ICON, and LVS_SMALLICON (which are the only modes where
 * items may overlap). So, to test items, you should use
 *    iterator_next
 * which lists the items top to bottom (in Z-order).
 * For drawing items, you should use
 *    iterator_prev
 * which lists the items bottom to top (in Z-order).
 * If you keep iterating over the items after the end-of-items
 * marker (-1) is returned, the iterator will start from the
 * beginning. Typically, you don't need to test for -1,
 * because iterator_{next,prev} will return TRUE if more items
 * are to be iterated over, or FALSE otherwise.
 *
 * Note: the iterator is defined to be bidirectional. That is,
 *       any number of prev followed by any number of next, or
 *       five versa, should leave the iterator at the same item:
 *           prev * n, next * n = next * n, prev * n
 *
 * The iterator has a notion of an out-of-order, special item,
 * which sits at the start of the list. This is used in
 * LVS_ICON, and LVS_SMALLICON mode to handle the focused item,
 * which needs to be first, as it may overlap other items.
 *           
 * The code is a bit messy because we have:
 *   - a special item to deal with
 *   - simple range, or composite range
 *   - empty range.
 * If you find bugs, or want to add features, please make sure you
 * always check/modify *both* iterator_prev, and iterator_next.
 */

/****
 * This function iterates through the items in increasing order,
 * but prefixed by the special item, then -1. That is:
 *    special, 1, 2, 3, ..., n, -1.
 * Each item is listed only once.
 */
static inline BOOL iterator_next(ITERATOR* i)
{
    if (i->nItem == -1)
    {
	i->nItem = i->nSpecial;
	if (i->nItem != -1) return TRUE;
    }
    if (i->nItem == i->nSpecial)
    {
	if (i->ranges) i->index = 0;
	goto pickarange;
    }

    i->nItem++;
testitem:
    if (i->nItem == i->nSpecial) i->nItem++;
    if (i->nItem < i->range.upper) return TRUE;

pickarange:
    if (i->ranges)
    {
	if (i->index < DPA_GetPtrCount(i->ranges->hdpa))
	    i->range = *(RANGE*)DPA_GetPtr(i->ranges->hdpa, i->index++);
	else goto end;
    }
    else if (i->nItem >= i->range.upper) goto end;

    i->nItem = i->range.lower;
    if (i->nItem >= 0) goto testitem;
end:
    i->nItem = -1;
    return FALSE;
}

/****
 * This function iterates through the items in decreasing order,
 * followed by the special item, then -1. That is:
 *    n, n-1, ..., 3, 2, 1, special, -1.
 * Each item is listed only once.
 */
static inline BOOL iterator_prev(ITERATOR* i)
{
    BOOL start = FALSE;

    if (i->nItem == -1)
    {
	start = TRUE;
	if (i->ranges) i->index = DPA_GetPtrCount(i->ranges->hdpa);
	goto pickarange;
    }
    if (i->nItem == i->nSpecial)
    {
	i->nItem = -1;
	return FALSE;
    }

testitem:
    i->nItem--;
    if (i->nItem == i->nSpecial) i->nItem--;
    if (i->nItem >= i->range.lower) return TRUE;

pickarange:
    if (i->ranges)
    {
	if (i->index > 0)
	    i->range = *(RANGE*)DPA_GetPtr(i->ranges->hdpa, --i->index);
	else goto end;
    }
    else if (!start && i->nItem < i->range.lower) goto end;

    i->nItem = i->range.upper;
    if (i->nItem > 0) goto testitem;
end:
    return (i->nItem = i->nSpecial) != -1;
}

static RANGE iterator_range(const ITERATOR *i)
{
    RANGE range;

    if (!i->ranges) return i->range;

    if (DPA_GetPtrCount(i->ranges->hdpa) > 0)
    {
        range.lower = (*(RANGE*)DPA_GetPtr(i->ranges->hdpa, 0)).lower;
        range.upper = (*(RANGE*)DPA_GetPtr(i->ranges->hdpa, DPA_GetPtrCount(i->ranges->hdpa) - 1)).upper;
    }
    else range.lower = range.upper = 0;

    return range;
}

/***
 * Releases resources associated with this iterator.
 */
static inline void iterator_destroy(const ITERATOR *i)
{
    ranges_destroy(i->ranges);
}

/***
 * Create an empty iterator.
 */
static inline void iterator_empty(ITERATOR* i)
{
    ZeroMemory(i, sizeof(*i));
    i->nItem = i->nSpecial = i->range.lower = i->range.upper = -1;
}

/***
 * Create an iterator over a range.
 */
static inline void iterator_rangeitems(ITERATOR* i, RANGE range)
{
    iterator_empty(i);
    i->range = range;
}

/***
 * Create an iterator over a bunch of ranges.
 * Please note that the iterator will take ownership of the ranges,
 * and will free them upon destruction.
 */
static inline void iterator_rangesitems(ITERATOR* i, RANGES ranges)
{
    iterator_empty(i);
    i->ranges = ranges;
}

/***
 * Creates an iterator over the items which intersect frame.
 * Uses absolute coordinates rather than compensating for the current offset.
 */
static BOOL iterator_frameditems_absolute(ITERATOR* i, const LISTVIEW_INFO* infoPtr, const RECT *frame)
{
    RECT rcItem, rcTemp;
    RANGES ranges;

    TRACE("(frame=%s)\n", wine_dbgstr_rect(frame));

    /* in case we fail, we want to return an empty iterator */
    iterator_empty(i);

    if (infoPtr->nItemCount == 0)
        return TRUE;

    if (infoPtr->uView == LV_VIEW_ICON || infoPtr->uView == LV_VIEW_SMALLICON)
    {
	INT nItem;
	
	if (infoPtr->uView == LV_VIEW_ICON && infoPtr->nFocusedItem != -1)
	{
	    LISTVIEW_GetItemBox(infoPtr, infoPtr->nFocusedItem, &rcItem);
	    if (IntersectRect(&rcTemp, &rcItem, frame))
		i->nSpecial = infoPtr->nFocusedItem;
	}
	if (!(ranges = ranges_create(50))) return FALSE;
	iterator_rangesitems(i, ranges);
	/* to do better here, we need to have PosX, and PosY sorted */
	TRACE("building icon ranges:\n");
	for (nItem = 0; nItem < infoPtr->nItemCount; nItem++)
	{
            rcItem.left = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosX, nItem);
	    rcItem.top = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosY, nItem);
	    rcItem.right = rcItem.left + infoPtr->nItemWidth;
	    rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
	    if (IntersectRect(&rcTemp, &rcItem, frame))
		ranges_additem(i->ranges, nItem);
	}
	return TRUE;
    }
    else if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	RANGE range;
	
	if (frame->left >= infoPtr->nItemWidth) return TRUE;
	if (frame->top >= infoPtr->nItemHeight * infoPtr->nItemCount) return TRUE;
	
	range.lower = max(frame->top / infoPtr->nItemHeight, 0);
	range.upper = min((frame->bottom - 1) / infoPtr->nItemHeight, infoPtr->nItemCount - 1) + 1;
	if (range.upper <= range.lower) return TRUE;
	iterator_rangeitems(i, range);
	TRACE("    report=%s\n", debugrange(&i->range));
    }
    else
    {
	INT nPerCol = max((infoPtr->rcList.bottom - infoPtr->rcList.top) / infoPtr->nItemHeight, 1);
	INT nFirstRow = max(frame->top / infoPtr->nItemHeight, 0);
	INT nLastRow = min((frame->bottom - 1) / infoPtr->nItemHeight, nPerCol - 1);
	INT nFirstCol;
	INT nLastCol;
	INT lower;
	RANGE item_range;
	INT nCol;

	if (infoPtr->nItemWidth)
	{
	    nFirstCol = max(frame->left / infoPtr->nItemWidth, 0);
            nLastCol  = min((frame->right - 1) / infoPtr->nItemWidth, (infoPtr->nItemCount + nPerCol - 1) / nPerCol);
	}
	else
	{
	    nFirstCol = max(frame->left, 0);
            nLastCol  = min(frame->right - 1, (infoPtr->nItemCount + nPerCol - 1) / nPerCol);
	}

	lower = nFirstCol * nPerCol + nFirstRow;

	TRACE("nPerCol=%d, nFirstRow=%d, nLastRow=%d, nFirstCol=%d, nLastCol=%d, lower=%d\n",
	      nPerCol, nFirstRow, nLastRow, nFirstCol, nLastCol, lower);
	
	if (nLastCol < nFirstCol || nLastRow < nFirstRow) return TRUE;

	if (!(ranges = ranges_create(nLastCol - nFirstCol + 1))) return FALSE;
	iterator_rangesitems(i, ranges);
	TRACE("building list ranges:\n");
	for (nCol = nFirstCol; nCol <= nLastCol; nCol++)
	{
	    item_range.lower = nCol * nPerCol + nFirstRow;
	    if(item_range.lower >= infoPtr->nItemCount) break;
	    item_range.upper = min(nCol * nPerCol + nLastRow + 1, infoPtr->nItemCount);
	    TRACE("   list=%s\n", debugrange(&item_range));
	    ranges_add(i->ranges, item_range);
	}
    }

    return TRUE;
}

/***
 * Creates an iterator over the items which intersect lprc.
 */
static BOOL iterator_frameditems(ITERATOR* i, const LISTVIEW_INFO* infoPtr, const RECT *lprc)
{
    RECT frame = *lprc;
    POINT Origin;

    TRACE("(lprc=%s)\n", wine_dbgstr_rect(lprc));

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    OffsetRect(&frame, -Origin.x, -Origin.y);

    return iterator_frameditems_absolute(i, infoPtr, &frame);
}

/***
 * Creates an iterator over the items which intersect the visible region of hdc.
 */
static BOOL iterator_visibleitems(ITERATOR *i, const LISTVIEW_INFO *infoPtr, HDC  hdc)
{
    POINT Origin, Position;
    RECT rcItem, rcClip;
    INT rgntype;
    
    rgntype = GetClipBox(hdc, &rcClip);
    if (rgntype == NULLREGION)
    {
        iterator_empty(i);
        return TRUE;
    }
    if (!iterator_frameditems(i, infoPtr, &rcClip)) return FALSE;
    if (rgntype == SIMPLEREGION) return TRUE;

    /* first deal with the special item */
    if (i->nSpecial != -1)
    {
	LISTVIEW_GetItemBox(infoPtr, i->nSpecial, &rcItem);
	if (!RectVisible(hdc, &rcItem)) i->nSpecial = -1;
    }
    
    /* if we can't deal with the region, we'll just go with the simple range */
    LISTVIEW_GetOrigin(infoPtr, &Origin);
    TRACE("building visible range:\n");
    if (!i->ranges && i->range.lower < i->range.upper)
    {
	if (!(i->ranges = ranges_create(50))) return TRUE;
	if (!ranges_add(i->ranges, i->range))
        {
	    ranges_destroy(i->ranges);
	    i->ranges = 0;
	    return TRUE;
        }
    }

    /* now delete the invisible items from the list */
    while(iterator_next(i))
    {
	LISTVIEW_GetItemOrigin(infoPtr, i->nItem, &Position);
	rcItem.left = (infoPtr->uView == LV_VIEW_DETAILS) ? Origin.x : Position.x + Origin.x;
	rcItem.top = Position.y + Origin.y;
	rcItem.right = rcItem.left + infoPtr->nItemWidth;
	rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
	if (!RectVisible(hdc, &rcItem))
	    ranges_delitem(i->ranges, i->nItem);
    }
    /* the iterator should restart on the next iterator_next */
    TRACE("done\n");
    
    return TRUE;
}

/* Remove common elements from two iterators */
/* Passed iterators have to point on the first elements */
static BOOL iterator_remove_common_items(ITERATOR *iter1, ITERATOR *iter2)
{
    if(!iter1->ranges || !iter2->ranges) {
        int lower, upper;

        if(iter1->ranges || iter2->ranges ||
                (iter1->range.lower<iter2->range.lower && iter1->range.upper>iter2->range.upper) ||
                (iter1->range.lower>iter2->range.lower && iter1->range.upper<iter2->range.upper)) {
            ERR("result is not a one range iterator\n");
            return FALSE;
        }

        if(iter1->range.lower==-1 || iter2->range.lower==-1)
            return TRUE;

        lower = iter1->range.lower;
        upper = iter1->range.upper;

        if(lower < iter2->range.lower)
            iter1->range.upper = iter2->range.lower;
        else if(upper > iter2->range.upper)
            iter1->range.lower = iter2->range.upper;
        else
            iter1->range.lower = iter1->range.upper = -1;

        if(iter2->range.lower < lower)
            iter2->range.upper = lower;
        else if(iter2->range.upper > upper)
            iter2->range.lower = upper;
        else
            iter2->range.lower = iter2->range.upper = -1;

        return TRUE;
    }

    iterator_next(iter1);
    iterator_next(iter2);

    while(1) {
        if(iter1->nItem==-1 || iter2->nItem==-1)
            break;

        if(iter1->nItem == iter2->nItem) {
            int delete = iter1->nItem;

            iterator_prev(iter1);
            iterator_prev(iter2);
            ranges_delitem(iter1->ranges, delete);
            ranges_delitem(iter2->ranges, delete);
            iterator_next(iter1);
            iterator_next(iter2);
        } else if(iter1->nItem > iter2->nItem)
            iterator_next(iter2);
        else
            iterator_next(iter1);
    }

    iter1->nItem = iter1->range.lower = iter1->range.upper = -1;
    iter2->nItem = iter2->range.lower = iter2->range.upper = -1;
    return TRUE;
}

/******** Misc helper functions ************************************/

static inline LRESULT CallWindowProcT(WNDPROC proc, HWND hwnd, UINT uMsg,
		                      WPARAM wParam, LPARAM lParam, BOOL isW)
{
    if (isW) return CallWindowProcW(proc, hwnd, uMsg, wParam, lParam);
    else return CallWindowProcA(proc, hwnd, uMsg, wParam, lParam);
}

static inline BOOL is_autoarrange(const LISTVIEW_INFO *infoPtr)
{
    return (infoPtr->dwStyle & LVS_AUTOARRANGE) &&
        (infoPtr->uView == LV_VIEW_ICON || infoPtr->uView == LV_VIEW_SMALLICON);
}

static void toggle_checkbox_state(LISTVIEW_INFO *infoPtr, INT nItem)
{
    DWORD state = STATEIMAGEINDEX(LISTVIEW_GetItemState(infoPtr, nItem, LVIS_STATEIMAGEMASK));
    if(state == 1 || state == 2)
    {
        LVITEMW lvitem;
        state ^= 3;
        lvitem.state = INDEXTOSTATEIMAGEMASK(state);
        lvitem.stateMask = LVIS_STATEIMAGEMASK;
        LISTVIEW_SetItemState(infoPtr, nItem, &lvitem);
    }
}

/* this should be called after window style got updated,
   it used to reset view state to match current window style */
static inline void map_style_view(LISTVIEW_INFO *infoPtr)
{
    switch (infoPtr->dwStyle & LVS_TYPEMASK)
    {
    case LVS_ICON:
        infoPtr->uView = LV_VIEW_ICON;
        break;
    case LVS_REPORT:
        infoPtr->uView = LV_VIEW_DETAILS;
        break;
    case LVS_SMALLICON:
        infoPtr->uView = LV_VIEW_SMALLICON;
        break;
    case LVS_LIST:
        infoPtr->uView = LV_VIEW_LIST;
    }
}

/* computes next item id value */
static DWORD get_next_itemid(const LISTVIEW_INFO *infoPtr)
{
    INT count = DPA_GetPtrCount(infoPtr->hdpaItemIds);

    if (count > 0)
    {
        ITEM_ID *lpID = DPA_GetPtr(infoPtr->hdpaItemIds, count - 1);
        return lpID->id + 1;
    }
    return 0;
}

/******** Internal API functions ************************************/

static inline COLUMN_INFO * LISTVIEW_GetColumnInfo(const LISTVIEW_INFO *infoPtr, INT nSubItem)
{
    static COLUMN_INFO mainItem;

    if (nSubItem == 0 && DPA_GetPtrCount(infoPtr->hdpaColumns) == 0) return &mainItem;
    assert (nSubItem >= 0 && nSubItem < DPA_GetPtrCount(infoPtr->hdpaColumns));

    /* update cached column rectangles */
    if (infoPtr->colRectsDirty)
    {
        COLUMN_INFO *info;
        LISTVIEW_INFO *Ptr = (LISTVIEW_INFO*)infoPtr;
        INT i;

        for (i = 0; i < DPA_GetPtrCount(infoPtr->hdpaColumns); i++) {
            info = DPA_GetPtr(infoPtr->hdpaColumns, i);
            SendMessageW(infoPtr->hwndHeader, HDM_GETITEMRECT, i, (LPARAM)&info->rcHeader);
        }
        Ptr->colRectsDirty = FALSE;
    }

    return DPA_GetPtr(infoPtr->hdpaColumns, nSubItem);
}

static INT LISTVIEW_CreateHeader(LISTVIEW_INFO *infoPtr)
{
    DWORD dFlags = WS_CHILD | HDS_HORZ | HDS_FULLDRAG | HDS_DRAGDROP;
    HINSTANCE hInst;

    if (infoPtr->hwndHeader) return 0;

    TRACE("Creating header for list %p\n", infoPtr->hwndSelf);

    /* setup creation flags */
    dFlags |= (LVS_NOSORTHEADER & infoPtr->dwStyle) ? 0 : HDS_BUTTONS;
    dFlags |= (LVS_NOCOLUMNHEADER & infoPtr->dwStyle) ? HDS_HIDDEN : 0;

    hInst = (HINSTANCE)GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_HINSTANCE);

    /* create header */
    infoPtr->hwndHeader = CreateWindowW(WC_HEADERW, NULL, dFlags,
      0, 0, 0, 0, infoPtr->hwndSelf, NULL, hInst, NULL);
    if (!infoPtr->hwndHeader) return -1;

    /* set header unicode format */
    SendMessageW(infoPtr->hwndHeader, HDM_SETUNICODEFORMAT, TRUE, 0);

    /* set header font */
    SendMessageW(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)infoPtr->hFont, TRUE);

    /* set header image list */
    if (infoPtr->himlSmall)
        SendMessageW(infoPtr->hwndHeader, HDM_SETIMAGELIST, 0, (LPARAM)infoPtr->himlSmall);

    LISTVIEW_UpdateSize(infoPtr);

    return 0;
}

static inline void LISTVIEW_GetHeaderRect(const LISTVIEW_INFO *infoPtr, INT nSubItem, LPRECT lprc)
{
    *lprc = LISTVIEW_GetColumnInfo(infoPtr, nSubItem)->rcHeader;
}

static inline BOOL LISTVIEW_IsHeaderEnabled(const LISTVIEW_INFO *infoPtr)
{
    return (infoPtr->uView == LV_VIEW_DETAILS ||
            infoPtr->dwLvExStyle & LVS_EX_HEADERINALLVIEWS) &&
          !(infoPtr->dwStyle & LVS_NOCOLUMNHEADER);
}
	
static inline BOOL LISTVIEW_GetItemW(const LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem)
{
    return LISTVIEW_GetItemT(infoPtr, lpLVItem, TRUE);
}

/* used to handle collapse main item column case */
static inline BOOL LISTVIEW_DrawFocusRect(const LISTVIEW_INFO *infoPtr, HDC hdc)
{
    return (infoPtr->rcFocus.left < infoPtr->rcFocus.right) ?
            DrawFocusRect(hdc, &infoPtr->rcFocus) : FALSE;
}

/* Listview invalidation functions: use _only_ these functions to invalidate */

static inline BOOL is_redrawing(const LISTVIEW_INFO *infoPtr)
{
    return infoPtr->redraw;
}

static inline void LISTVIEW_InvalidateRect(const LISTVIEW_INFO *infoPtr, const RECT* rect)
{
    if(!is_redrawing(infoPtr)) return; 
    TRACE(" invalidating rect=%s\n", wine_dbgstr_rect(rect));
    InvalidateRect(infoPtr->hwndSelf, rect, TRUE);
}

static inline void LISTVIEW_InvalidateItem(const LISTVIEW_INFO *infoPtr, INT nItem)
{
    RECT rcBox;

    if (!is_redrawing(infoPtr) || nItem < 0 || nItem >= infoPtr->nItemCount)
        return;

    LISTVIEW_GetItemBox(infoPtr, nItem, &rcBox);
    LISTVIEW_InvalidateRect(infoPtr, &rcBox);
}

static inline void LISTVIEW_InvalidateSubItem(const LISTVIEW_INFO *infoPtr, INT nItem, INT nSubItem)
{
    POINT Origin, Position;
    RECT rcBox;
    
    if(!is_redrawing(infoPtr)) return; 
    assert (infoPtr->uView == LV_VIEW_DETAILS);
    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);
    LISTVIEW_GetHeaderRect(infoPtr, nSubItem, &rcBox);
    rcBox.top = 0;
    rcBox.bottom = infoPtr->nItemHeight;
    OffsetRect(&rcBox, Origin.x, Origin.y + Position.y);
    LISTVIEW_InvalidateRect(infoPtr, &rcBox);
}

static inline void LISTVIEW_InvalidateList(const LISTVIEW_INFO *infoPtr)
{
    LISTVIEW_InvalidateRect(infoPtr, NULL);
}

static inline void LISTVIEW_InvalidateColumn(const LISTVIEW_INFO *infoPtr, INT nColumn)
{
    RECT rcCol;
    
    if(!is_redrawing(infoPtr)) return; 
    LISTVIEW_GetHeaderRect(infoPtr, nColumn, &rcCol);
    rcCol.top = infoPtr->rcList.top;
    rcCol.bottom = infoPtr->rcList.bottom;
    LISTVIEW_InvalidateRect(infoPtr, &rcCol);
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that can fit vertically in the client area.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of items per row.
 */
static inline INT LISTVIEW_GetCountPerRow(const LISTVIEW_INFO *infoPtr)
{
    INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;

    return max(nListWidth/(infoPtr->nItemWidth ? infoPtr->nItemWidth : 1), 1);
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that can fit horizontally in the client
 * area.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of items per column.
 */
static inline INT LISTVIEW_GetCountPerColumn(const LISTVIEW_INFO *infoPtr)
{
    INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;

    return infoPtr->nItemHeight ? max(nListHeight / infoPtr->nItemHeight, 1) : 0;
}


/*************************************************************************
 *		LISTVIEW_ProcessLetterKeys
 *
 *  Processes keyboard messages generated by pressing the letter keys
 *  on the keyboard.
 *  What this does is perform a case insensitive search from the
 *  current position with the following quirks:
 *  - If two chars or more are pressed in quick succession we search
 *    for the corresponding string (e.g. 'abc').
 *  - If there is a delay we wipe away the current search string and
 *    restart with just that char.
 *  - If the user keeps pressing the same character, whether slowly or
 *    fast, so that the search string is entirely composed of this
 *    character ('aaaaa' for instance), then we search for first item
 *    that starting with that character.
 *  - If the user types the above character in quick succession, then
 *    we must also search for the corresponding string ('aaaaa'), and
 *    go to that string if there is a match.
 *
 * PARAMETERS
 *   [I] hwnd : handle to the window
 *   [I] charCode : the character code, the actual character
 *   [I] keyData : key data
 *
 * RETURNS
 *
 *  Zero.
 *
 * BUGS
 *
 *  - The current implementation has a list of characters it will
 *    accept and it ignores everything else. In particular it will
 *    ignore accentuated characters which seems to match what
 *    Windows does. But I'm not sure it makes sense to follow
 *    Windows there.
 *  - We don't sound a beep when the search fails.
 *
 * SEE ALSO
 *
 *  TREEVIEW_ProcessLetterKeys
 */
static INT LISTVIEW_ProcessLetterKeys(LISTVIEW_INFO *infoPtr, WPARAM charCode, LPARAM keyData)
{
    WCHAR buffer[MAX_PATH];
    DWORD prevTime;
    LVITEMW item;
    int startidx;
    INT nItem;
    INT diff;

    /* simple parameter checking */
    if (!charCode || !keyData || infoPtr->nItemCount == 0) return 0;

    /* only allow the valid WM_CHARs through */
    if (!iswalnum(charCode) &&
        charCode != '.' && charCode != '`' && charCode != '!' &&
        charCode != '@' && charCode != '#' && charCode != '$' &&
        charCode != '%' && charCode != '^' && charCode != '&' &&
        charCode != '*' && charCode != '(' && charCode != ')' &&
        charCode != '-' && charCode != '_' && charCode != '+' &&
        charCode != '=' && charCode != '\\'&& charCode != ']' &&
        charCode != '}' && charCode != '[' && charCode != '{' &&
        charCode != '/' && charCode != '?' && charCode != '>' &&
        charCode != '<' && charCode != ',' && charCode != '~')
        return 0;

    /* update the search parameters */
    prevTime = infoPtr->lastKeyPressTimestamp;
    infoPtr->lastKeyPressTimestamp = GetTickCount();
    diff = infoPtr->lastKeyPressTimestamp - prevTime;

    if (diff >= 0 && diff < KEY_DELAY)
    {
        if (infoPtr->nSearchParamLength < MAX_PATH - 1)
            infoPtr->szSearchParam[infoPtr->nSearchParamLength++] = charCode;

        if (infoPtr->charCode != charCode)
            infoPtr->charCode = charCode = 0;
    }
    else
    {
        infoPtr->charCode = charCode;
        infoPtr->szSearchParam[0] = charCode;
        infoPtr->nSearchParamLength = 1;
    }

    /* should start from next after focused item, so next item that matches
       will be selected, if there isn't any and focused matches it will be selected
       on second search stage from beginning of the list */
    if (infoPtr->nFocusedItem >= 0 && infoPtr->nItemCount > 1)
    {
        /* with some accumulated search data available start with current focus, otherwise
           it's excluded from search */
        startidx = infoPtr->nSearchParamLength > 1 ? infoPtr->nFocusedItem : infoPtr->nFocusedItem + 1;
        if (startidx == infoPtr->nItemCount) startidx = 0;
    }
    else
        startidx = 0;

    /* let application handle this for virtual listview */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
        NMLVFINDITEMW nmlv;

        memset(&nmlv.lvfi, 0, sizeof(nmlv.lvfi));
        nmlv.lvfi.flags = (LVFI_WRAP | LVFI_PARTIAL);
        nmlv.lvfi.psz = infoPtr->szSearchParam;
        nmlv.iStart = startidx;

        infoPtr->szSearchParam[infoPtr->nSearchParamLength] = 0;

        nItem = notify_hdr(infoPtr, LVN_ODFINDITEMW, (LPNMHDR)&nmlv.hdr);
    }
    else
    {
        int i = startidx, endidx;

        /* and search from the current position */
        nItem = -1;
        endidx = infoPtr->nItemCount;

        /* first search in [startidx, endidx), on failure continue in [0, startidx) */
        while (1)
        {
            /* start from first item if not found with >= startidx */
            if (i == infoPtr->nItemCount && startidx > 0)
            {
                endidx = startidx;
                startidx = 0;
            }

            for (i = startidx; i < endidx; i++)
            {
                /* retrieve text */
                item.mask = LVIF_TEXT;
                item.iItem = i;
                item.iSubItem = 0;
                item.pszText = buffer;
                item.cchTextMax = MAX_PATH;
                if (!LISTVIEW_GetItemW(infoPtr, &item)) return 0;

                if (!wcsnicmp(item.pszText, infoPtr->szSearchParam, infoPtr->nSearchParamLength))
                {
                    nItem = i;
                    break;
                }
                /* this is used to find first char match when search string is not available yet,
                   otherwise every WM_CHAR will search to next item by first char, ignoring that we're
                   already waiting for user to complete a string */
                else if (nItem == -1 && infoPtr->nSearchParamLength == 1 && !wcsnicmp(item.pszText, infoPtr->szSearchParam, 1))
                {
                    /* this would work but we must keep looking for a longer match */
                    nItem = i;
                }
            }

            if ( nItem != -1 || /* found something */
                 endidx != infoPtr->nItemCount || /* second search done */
                (startidx == 0 && endidx == infoPtr->nItemCount) /* full range for first search */ )
                break;
        };
    }

    if (nItem != -1)
        LISTVIEW_KeySelection(infoPtr, nItem, FALSE);

    return 0;
}

/*************************************************************************
 * LISTVIEW_UpdateHeaderSize [Internal]
 *
 * Function to resize the header control
 *
 * PARAMS
 * [I]  hwnd : handle to a window
 * [I]  nNewScrollPos : scroll pos to set
 *
 * RETURNS
 * None.
 */
static void LISTVIEW_UpdateHeaderSize(const LISTVIEW_INFO *infoPtr, INT nNewScrollPos)
{
    RECT winRect;
    POINT point[2];

    TRACE("nNewScrollPos=%d\n", nNewScrollPos);

    if (!infoPtr->hwndHeader)  return;

    GetWindowRect(infoPtr->hwndHeader, &winRect);
    point[0].x = winRect.left;
    point[0].y = winRect.top;
    point[1].x = winRect.right;
    point[1].y = winRect.bottom;

    MapWindowPoints(HWND_DESKTOP, infoPtr->hwndSelf, point, 2);
    point[0].x = -nNewScrollPos;
    point[1].x += nNewScrollPos;

    SetWindowPos(infoPtr->hwndHeader,0,
        point[0].x,point[0].y,point[1].x,point[1].y,
        (infoPtr->dwStyle & LVS_NOCOLUMNHEADER) ? SWP_HIDEWINDOW : SWP_SHOWWINDOW |
        SWP_NOZORDER | SWP_NOACTIVATE);
}

static INT LISTVIEW_UpdateHScroll(LISTVIEW_INFO *infoPtr)
{
    SCROLLINFO horzInfo;
    INT dx;

    ZeroMemory(&horzInfo, sizeof(SCROLLINFO));
    horzInfo.cbSize = sizeof(SCROLLINFO);
    horzInfo.nPage = infoPtr->rcList.right - infoPtr->rcList.left;

    /* for now, we'll set info.nMax to the _count_, and adjust it later */
    if (infoPtr->uView == LV_VIEW_LIST)
    {
	INT nPerCol = LISTVIEW_GetCountPerColumn(infoPtr);
	horzInfo.nMax = (infoPtr->nItemCount + nPerCol - 1) / nPerCol;

	/* scroll by at least one column per page */
	if(horzInfo.nPage < infoPtr->nItemWidth)
		horzInfo.nPage = infoPtr->nItemWidth;

	if (infoPtr->nItemWidth)
	    horzInfo.nPage /= infoPtr->nItemWidth;
    }
    else if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	horzInfo.nMax = infoPtr->nItemWidth;
    }
    else /* LV_VIEW_ICON, or LV_VIEW_SMALLICON */
    {
	RECT rcView;

	if (LISTVIEW_GetViewRect(infoPtr, &rcView)) horzInfo.nMax = rcView.right - rcView.left;
    }

    if (LISTVIEW_IsHeaderEnabled(infoPtr))
    {
	if (DPA_GetPtrCount(infoPtr->hdpaColumns))
	{
	    RECT rcHeader;
	    INT index;

	    index = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX,
                                 DPA_GetPtrCount(infoPtr->hdpaColumns) - 1, 0);

	    LISTVIEW_GetHeaderRect(infoPtr, index, &rcHeader);
	    horzInfo.nMax = rcHeader.right;
	    TRACE("horzInfo.nMax=%d\n", horzInfo.nMax);
	}
    }

    horzInfo.fMask = SIF_RANGE | SIF_PAGE;
    horzInfo.nMax = max(horzInfo.nMax - 1, 0);
    dx = GetScrollPos(infoPtr->hwndSelf, SB_HORZ);
    dx -= SetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &horzInfo, TRUE);
    TRACE("horzInfo=%s\n", debugscrollinfo(&horzInfo));

    /* Update the Header Control */
    if (infoPtr->hwndHeader)
    {
	horzInfo.fMask = SIF_POS;
	GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &horzInfo);
	LISTVIEW_UpdateHeaderSize(infoPtr, horzInfo.nPos);
    }

    LISTVIEW_UpdateSize(infoPtr);
    return dx;
}

static INT LISTVIEW_UpdateVScroll(LISTVIEW_INFO *infoPtr)
{
    SCROLLINFO vertInfo;
    INT dy;

    ZeroMemory(&vertInfo, sizeof(SCROLLINFO));
    vertInfo.cbSize = sizeof(SCROLLINFO);
    vertInfo.nPage = infoPtr->rcList.bottom - infoPtr->rcList.top;

    if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	vertInfo.nMax = infoPtr->nItemCount;
	
	/* scroll by at least one page */
	if(vertInfo.nPage < infoPtr->nItemHeight)
	  vertInfo.nPage = infoPtr->nItemHeight;

        if (infoPtr->nItemHeight > 0)
            vertInfo.nPage /= infoPtr->nItemHeight;
    }
    else if (infoPtr->uView != LV_VIEW_LIST) /* LV_VIEW_ICON, or LV_VIEW_SMALLICON */
    {
	RECT rcView;

	if (LISTVIEW_GetViewRect(infoPtr, &rcView)) vertInfo.nMax = rcView.bottom - rcView.top;
    }

    vertInfo.fMask = SIF_RANGE | SIF_PAGE;
    vertInfo.nMax = max(vertInfo.nMax - 1, 0);
    dy = GetScrollPos(infoPtr->hwndSelf, SB_VERT);
    dy -= SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &vertInfo, TRUE);
    TRACE("vertInfo=%s\n", debugscrollinfo(&vertInfo));

    LISTVIEW_UpdateSize(infoPtr);
    return dy;
}

/***
 * DESCRIPTION:
 * Update the scrollbars. This function should be called whenever
 * the content, size or view changes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * None
 */
static void LISTVIEW_UpdateScroll(LISTVIEW_INFO *infoPtr)
{
    INT dx, dy, pass;

    if ((infoPtr->dwStyle & LVS_NOSCROLL) || !is_redrawing(infoPtr)) return;

    /* Setting the horizontal scroll can change the listview size
     * (and potentially everything else) so we need to recompute
     * everything again for the vertical scroll and vice-versa
     */
    for (dx = 0, dy = 0, pass = 0; pass <= 1; pass++)
    {
        dx += LISTVIEW_UpdateHScroll(infoPtr);
        dy += LISTVIEW_UpdateVScroll(infoPtr);
    }

    /* Change of the range may have changed the scroll pos. If so move the content */
    if (dx != 0 || dy != 0)
    {
        RECT listRect;
        listRect = infoPtr->rcList;
        ScrollWindowEx(infoPtr->hwndSelf, dx, dy, &listRect, &listRect, 0, 0,
            SW_ERASE | SW_INVALIDATE);
    }
}


/***
 * DESCRIPTION:
 * Shows/hides the focus rectangle. 
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fShow : TRUE to show the focus, FALSE to hide it.
 *
 * RETURN:
 * None
 */
static void LISTVIEW_ShowFocusRect(const LISTVIEW_INFO *infoPtr, BOOL fShow)
{
    HDC hdc;

    TRACE("fShow=%d, nItem=%d\n", fShow, infoPtr->nFocusedItem);

    if (infoPtr->nFocusedItem < 0) return;

    /* we need some gymnastics in ICON mode to handle large items */
    if (infoPtr->uView == LV_VIEW_ICON)
    {
	RECT rcBox;

	LISTVIEW_GetItemBox(infoPtr, infoPtr->nFocusedItem, &rcBox); 
	if ((rcBox.bottom - rcBox.top) > infoPtr->nItemHeight)
	{
	    LISTVIEW_InvalidateRect(infoPtr, &rcBox);
	    return;
	}
    }

    if (!(hdc = GetDC(infoPtr->hwndSelf))) return;

    /* for some reason, owner draw should work only in report mode */
    if ((infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && (infoPtr->uView == LV_VIEW_DETAILS))
    {
	DRAWITEMSTRUCT dis;
	LVITEMW item;

	HFONT hFont = infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont;
	HFONT hOldFont = SelectObject(hdc, hFont);

        item.iItem = infoPtr->nFocusedItem;
	item.iSubItem = 0;
        item.mask = LVIF_PARAM;
	if (!LISTVIEW_GetItemW(infoPtr, &item)) goto done;
	   
	ZeroMemory(&dis, sizeof(dis)); 
	dis.CtlType = ODT_LISTVIEW;
	dis.CtlID = (UINT)GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
	dis.itemID = item.iItem;
	dis.itemAction = ODA_FOCUS;
	if (fShow) dis.itemState |= ODS_FOCUS;
	dis.hwndItem = infoPtr->hwndSelf;
	dis.hDC = hdc;
	LISTVIEW_GetItemBox(infoPtr, dis.itemID, &dis.rcItem);
	dis.itemData = item.lParam;

	SendMessageW(infoPtr->hwndNotify, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);

	SelectObject(hdc, hOldFont);
    }
    else
        LISTVIEW_InvalidateItem(infoPtr, infoPtr->nFocusedItem);

done:
    ReleaseDC(infoPtr->hwndSelf, hdc);
}

/***
 * Invalidates all visible selected items.
 */
static void LISTVIEW_InvalidateSelectedItems(const LISTVIEW_INFO *infoPtr)
{
    ITERATOR i; 
   
    iterator_frameditems(&i, infoPtr, &infoPtr->rcList); 
    while(iterator_next(&i))
    {
	if (LISTVIEW_GetItemState(infoPtr, i.nItem, LVIS_SELECTED))
	    LISTVIEW_InvalidateItem(infoPtr, i.nItem);
    }
    iterator_destroy(&i);
}

	    
/***
 * DESCRIPTION:            [INTERNAL]
 * Computes an item's (left,top) corner, relative to rcView.
 * That is, the position has NOT been made relative to the Origin.
 * This is deliberate, to avoid computing the Origin over, and
 * over again, when this function is called in a loop. Instead,
 * one can factor the computation of the Origin before the loop,
 * and offset the value returned by this function, on every iteration.
 * 
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem  : item number
 * [O] lpptOrig : item top, left corner
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_GetItemOrigin(const LISTVIEW_INFO *infoPtr, INT nItem, LPPOINT lpptPosition)
{
    assert(nItem >= 0 && nItem < infoPtr->nItemCount);

    if ((infoPtr->uView == LV_VIEW_SMALLICON) || (infoPtr->uView == LV_VIEW_ICON))
    {
	lpptPosition->x = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosX, nItem);
	lpptPosition->y = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosY, nItem);
    }
    else if (infoPtr->uView == LV_VIEW_LIST)
    {
        INT nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
	lpptPosition->x = nItem / nCountPerColumn * infoPtr->nItemWidth;
	lpptPosition->y = nItem % nCountPerColumn * infoPtr->nItemHeight;
    }
    else /* LV_VIEW_DETAILS */
    {
	lpptPosition->x = REPORT_MARGINX;
	/* item is always at zero indexed column */
	if (DPA_GetPtrCount(infoPtr->hdpaColumns) > 0)
	    lpptPosition->x += LISTVIEW_GetColumnInfo(infoPtr, 0)->rcHeader.left;
	lpptPosition->y = nItem * infoPtr->nItemHeight;
    }
}
    
/***
 * DESCRIPTION:            [INTERNAL]
 * Compute the rectangles of an item.  This is to localize all
 * the computations in one place. If you are not interested in some
 * of these values, simply pass in a NULL -- the function is smart
 * enough to compute only what's necessary. The function computes
 * the standard rectangles (BOUNDS, ICON, LABEL) plus a non-standard
 * one, the BOX rectangle. This rectangle is very cheap to compute,
 * and is guaranteed to contain all the other rectangles. Computing
 * the ICON rect is also cheap, but all the others are potentially
 * expensive. This gives an easy and effective optimization when
 * searching (like point inclusion, or rectangle intersection):
 * first test against the BOX, and if TRUE, test against the desired
 * rectangle.
 * If the function does not have all the necessary information
 * to computed the requested rectangles, will crash with a
 * failed assertion. This is done so we catch all programming
 * errors, given that the function is called only from our code.
 *
 * We have the following 'special' meanings for a few fields:
 *   * If LVIS_FOCUSED is set, we assume the item has the focus
 *     This is important in ICON mode, where it might get a larger
 *     then usual rectangle
 *
 * Please note that subitem support works only in REPORT mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : item to compute the measures for
 * [O] lprcBox : ptr to Box rectangle
 *                Same as LVM_GETITEMRECT with LVIR_BOUNDS
 * [0] lprcSelectBox : ptr to select box rectangle
 *  		  Same as LVM_GETITEMRECT with LVIR_SELECTEDBOUNDS
 * [O] lprcIcon : ptr to Icon rectangle
 *                Same as LVM_GETITEMRECT with LVIR_ICON
 * [O] lprcStateIcon: ptr to State Icon rectangle
 * [O] lprcLabel : ptr to Label rectangle
 *                Same as LVM_GETITEMRECT with LVIR_LABEL
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_GetItemMetrics(const LISTVIEW_INFO *infoPtr, const LVITEMW *lpLVItem,
				    LPRECT lprcBox, LPRECT lprcSelectBox,
				    LPRECT lprcIcon, LPRECT lprcStateIcon, LPRECT lprcLabel)
{
    BOOL doSelectBox = FALSE, doIcon = FALSE, doLabel = FALSE, oversizedBox = FALSE;
    RECT Box, SelectBox, Icon, Label;
    COLUMN_INFO *lpColumnInfo = NULL;
    SIZE labelSize = { 0, 0 };

    TRACE("(lpLVItem=%s)\n", debuglvitem_t(lpLVItem, TRUE));

    /* Be smart and try to figure out the minimum we have to do */
    if (lpLVItem->iSubItem) assert(infoPtr->uView == LV_VIEW_DETAILS);
    if (infoPtr->uView == LV_VIEW_ICON && (lprcBox || lprcLabel))
    {
	assert((lpLVItem->mask & LVIF_STATE) && (lpLVItem->stateMask & LVIS_FOCUSED));
	if (lpLVItem->state & LVIS_FOCUSED) oversizedBox = doLabel = TRUE;
    }
    if (lprcSelectBox) doSelectBox = TRUE;
    if (lprcLabel) doLabel = TRUE;
    if (doLabel || lprcIcon || lprcStateIcon) doIcon = TRUE;
    if (doSelectBox)
    {
        doIcon = TRUE;
        doLabel = TRUE;
    }

    /************************************************************/
    /* compute the box rectangle (it should be cheap to do)     */
    /************************************************************/
    if (lpLVItem->iSubItem || infoPtr->uView == LV_VIEW_DETAILS)
	lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, lpLVItem->iSubItem);

    if (lpLVItem->iSubItem)    
    {
	Box = lpColumnInfo->rcHeader;
    }
    else
    {
	Box.left = 0;
	Box.right = infoPtr->nItemWidth;
    }
    Box.top = 0;
    Box.bottom = infoPtr->nItemHeight;

    /******************************************************************/
    /* compute ICON bounding box (ala LVM_GETITEMRECT) and STATEICON  */
    /******************************************************************/
    if (doIcon)
    {
	LONG state_width = 0;

	if (infoPtr->himlState && lpLVItem->iSubItem == 0)
	    state_width = infoPtr->iconStateSize.cx;

	if (infoPtr->uView == LV_VIEW_ICON)
	{
	    Icon.left   = Box.left + state_width;
	    if (infoPtr->himlNormal)
		Icon.left += (infoPtr->nItemWidth - infoPtr->iconSize.cx - state_width) / 2;
	    Icon.top    = Box.top + ICON_TOP_PADDING;
	    Icon.right  = Icon.left;
	    Icon.bottom = Icon.top;
	    if (infoPtr->himlNormal)
	    {
		Icon.right  += infoPtr->iconSize.cx;
		Icon.bottom += infoPtr->iconSize.cy;
	    }
	}
	else /* LV_VIEW_SMALLICON, LV_VIEW_LIST or LV_VIEW_DETAILS */
	{
	    Icon.left   = Box.left + state_width;

	    if (infoPtr->uView == LV_VIEW_DETAILS && lpLVItem->iSubItem == 0)
	    {
		/* we need the indent in report mode */
		assert(lpLVItem->mask & LVIF_INDENT);
		Icon.left += infoPtr->iconSize.cx * lpLVItem->iIndent + REPORT_MARGINX;
	    }

	    Icon.top    = Box.top;
	    Icon.right  = Icon.left;
	    if (infoPtr->himlSmall &&
                (!lpColumnInfo || lpLVItem->iSubItem == 0 ||
                 ((infoPtr->dwLvExStyle & LVS_EX_SUBITEMIMAGES) && lpLVItem->iImage != I_IMAGECALLBACK)))
		Icon.right += infoPtr->iconSize.cx;
	    Icon.bottom = Icon.top + infoPtr->iconSize.cy;
	}
	if(lprcIcon) *lprcIcon = Icon;
	TRACE("    - icon=%s\n", wine_dbgstr_rect(&Icon));

        /* TODO: is this correct? */
        if (lprcStateIcon)
        {
            lprcStateIcon->left = Icon.left - state_width;
            lprcStateIcon->right = Icon.left;
            lprcStateIcon->top = Icon.top;
            lprcStateIcon->bottom = lprcStateIcon->top + infoPtr->iconSize.cy;
            TRACE("    - state icon=%s\n", wine_dbgstr_rect(lprcStateIcon));
        }
     }
     else Icon.right = 0;

    /************************************************************/
    /* compute LABEL bounding box (ala LVM_GETITEMRECT)         */
    /************************************************************/
    if (doLabel)
    {
	/* calculate how far to the right can the label stretch */
	Label.right = Box.right;
	if (infoPtr->uView == LV_VIEW_DETAILS)
	{
	    if (lpLVItem->iSubItem == 0)
	    {
		/* we need a zero based rect here */
		Label = lpColumnInfo->rcHeader;
		OffsetRect(&Label, -Label.left, 0);
	    }
	}

	if (lpLVItem->iSubItem || ((infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && infoPtr->uView == LV_VIEW_DETAILS))
	{
	   labelSize.cx = infoPtr->nItemWidth;
	   labelSize.cy = infoPtr->nItemHeight;
	   goto calc_label;
	}
	
	/* we need the text in non owner draw mode */
	assert(lpLVItem->mask & LVIF_TEXT);
	if (is_text(lpLVItem->pszText))
        {
    	    HFONT hFont = infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont;
    	    HDC hdc = GetDC(infoPtr->hwndSelf);
    	    HFONT hOldFont = SelectObject(hdc, hFont);
	    UINT uFormat;
	    RECT rcText;

	    /* compute rough rectangle where the label will go */
	    SetRectEmpty(&rcText);
	    rcText.right = infoPtr->nItemWidth - TRAILING_LABEL_PADDING;
	    rcText.bottom = infoPtr->nItemHeight;
	    if (infoPtr->uView == LV_VIEW_ICON)
		rcText.bottom -= ICON_TOP_PADDING + infoPtr->iconSize.cy + ICON_BOTTOM_PADDING;

	    /* now figure out the flags */
	    if (infoPtr->uView == LV_VIEW_ICON)
		uFormat = oversizedBox ? LV_FL_DT_FLAGS : LV_ML_DT_FLAGS;
	    else
		uFormat = LV_SL_DT_FLAGS;
	    
    	    DrawTextW (hdc, lpLVItem->pszText, -1, &rcText, uFormat | DT_CALCRECT);

	    if (rcText.right != rcText.left)
	        labelSize.cx = min(rcText.right - rcText.left + TRAILING_LABEL_PADDING, infoPtr->nItemWidth);

	    labelSize.cy = rcText.bottom - rcText.top;

    	    SelectObject(hdc, hOldFont);
    	    ReleaseDC(infoPtr->hwndSelf, hdc);
	}

calc_label:
	if (infoPtr->uView == LV_VIEW_ICON)
	{
	    Label.left = Box.left + (infoPtr->nItemWidth - labelSize.cx) / 2;
	    Label.top  = Box.top + ICON_TOP_PADDING_HITABLE +
		         infoPtr->iconSize.cy + ICON_BOTTOM_PADDING;
	    Label.right = Label.left + labelSize.cx;
	    Label.bottom = Label.top + infoPtr->nItemHeight;
	    if (!oversizedBox && labelSize.cy > infoPtr->ntmHeight)
	    {
		labelSize.cy = min(Box.bottom - Label.top, labelSize.cy);
		labelSize.cy /= infoPtr->ntmHeight;
		labelSize.cy = max(labelSize.cy, 1);
		labelSize.cy *= infoPtr->ntmHeight;
	     }
	     Label.bottom = Label.top + labelSize.cy + HEIGHT_PADDING;
	}
	else if (infoPtr->uView == LV_VIEW_DETAILS)
	{
	    Label.left = Icon.right;
	    Label.top = Box.top;
	    Label.right = lpLVItem->iSubItem ? lpColumnInfo->rcHeader.right :
			  lpColumnInfo->rcHeader.right - lpColumnInfo->rcHeader.left;
	    Label.bottom = Label.top + infoPtr->nItemHeight;
	}
	else /* LV_VIEW_SMALLICON or LV_VIEW_LIST */
	{
	    Label.left = Icon.right;
	    Label.top = Box.top;
	    Label.right = min(Label.left + labelSize.cx, Label.right);
	    Label.bottom = Label.top + infoPtr->nItemHeight;
	}
  
	if (lprcLabel) *lprcLabel = Label;
	TRACE("    - label=%s\n", wine_dbgstr_rect(&Label));
    }

    /************************************************************/
    /* compute SELECT bounding box                              */
    /************************************************************/
    if (doSelectBox)
    {
	if (infoPtr->uView == LV_VIEW_DETAILS)
	{
	    SelectBox.left = Icon.left;
	    SelectBox.top = Box.top;
	    SelectBox.bottom = Box.bottom;

	    if (labelSize.cx)
	        SelectBox.right = min(Label.left + labelSize.cx, Label.right);
	    else
	        SelectBox.right = min(Label.left + MAX_EMPTYTEXT_SELECT_WIDTH, Label.right);
	}
	else
	{
	    UnionRect(&SelectBox, &Icon, &Label);
	}
	if (lprcSelectBox) *lprcSelectBox = SelectBox;
	TRACE("    - select box=%s\n", wine_dbgstr_rect(&SelectBox));
    }

    /* Fix the Box if necessary */
    if (lprcBox)
    {
	if (oversizedBox) UnionRect(lprcBox, &Box, &Label);
	else *lprcBox = Box;
    }
    TRACE("    - box=%s\n", wine_dbgstr_rect(&Box));
}

/***
 * DESCRIPTION:            [INTERNAL]
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item number
 * [O] lprcBox : ptr to Box rectangle
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_GetItemBox(const LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lprcBox)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    POINT Position, Origin;
    LVITEMW lvItem;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);

    /* Be smart and try to figure out the minimum we have to do */
    lvItem.mask = 0;
    if (infoPtr->uView == LV_VIEW_ICON && infoPtr->bFocus && LISTVIEW_GetItemState(infoPtr, nItem, LVIS_FOCUSED))
	lvItem.mask |= LVIF_TEXT;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.pszText = szDispText;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    if (lvItem.mask) LISTVIEW_GetItemW(infoPtr, &lvItem);
    if (infoPtr->uView == LV_VIEW_ICON)
    {
	lvItem.mask |= LVIF_STATE;
	lvItem.stateMask = LVIS_FOCUSED;
	lvItem.state = (lvItem.mask & LVIF_TEXT ? LVIS_FOCUSED : 0);
    }
    LISTVIEW_GetItemMetrics(infoPtr, &lvItem, lprcBox, 0, 0, 0, 0);

    if (infoPtr->uView == LV_VIEW_DETAILS && infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT &&
        SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX, 0, 0))
    {
        OffsetRect(lprcBox, Origin.x, Position.y + Origin.y);
    }
    else
        OffsetRect(lprcBox, Position.x + Origin.x, Position.y + Origin.y);
}

/* LISTVIEW_MapIdToIndex helper */
static INT CALLBACK MapIdSearchCompare(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    ITEM_ID *id1 = (ITEM_ID*)p1;
    ITEM_ID *id2 = (ITEM_ID*)p2;

    if (id1->id == id2->id) return 0;

    return (id1->id < id2->id) ? -1 : 1;
}

/***
 * DESCRIPTION:
 * Returns the item index for id specified.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] iID : item id to get index for
 *
 * RETURN:
 * Item index, or -1 on failure.
 */
static INT LISTVIEW_MapIdToIndex(const LISTVIEW_INFO *infoPtr, UINT iID)
{
    ITEM_ID ID;
    INT index;

    TRACE("iID=%d\n", iID);

    if (infoPtr->dwStyle & LVS_OWNERDATA) return -1;
    if (infoPtr->nItemCount == 0) return -1;

    ID.id = iID;
    index = DPA_Search(infoPtr->hdpaItemIds, &ID, -1, MapIdSearchCompare, 0, DPAS_SORTED);

    if (index != -1)
    {
        ITEM_ID *lpID = DPA_GetPtr(infoPtr->hdpaItemIds, index);
        return DPA_GetPtrIndex(infoPtr->hdpaItems, lpID->item);
    }

    return -1;
}

/***
 * DESCRIPTION:
 * Returns the item id for index given.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] iItem : item index to get id for
 *
 * RETURN:
 * Item id.
 */
static DWORD LISTVIEW_MapIndexToId(const LISTVIEW_INFO *infoPtr, INT iItem)
{
    ITEM_INFO *lpItem;
    HDPA hdpaSubItems;

    TRACE("iItem=%d\n", iItem);

    if (infoPtr->dwStyle & LVS_OWNERDATA) return -1;
    if (iItem < 0 || iItem >= infoPtr->nItemCount) return -1;

    hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, iItem);
    lpItem = DPA_GetPtr(hdpaSubItems, 0);

    return lpItem->id->id;
}

/***
 * DESCRIPTION:
 * Returns the current icon position, and advances it along the top.
 * The returned position is not offset by Origin.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lpPos : will get the current icon position
 *
 * RETURN:
 * None
 */
static void LISTVIEW_NextIconPosTop(LISTVIEW_INFO *infoPtr, LPPOINT lpPos)
{
    INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
    
    *lpPos = infoPtr->currIconPos;
    
    infoPtr->currIconPos.x += infoPtr->nItemWidth;
    if (infoPtr->currIconPos.x + infoPtr->nItemWidth <= nListWidth) return;

    infoPtr->currIconPos.x  = 0;
    infoPtr->currIconPos.y += infoPtr->nItemHeight;
}

    
/***
 * DESCRIPTION:
 * Returns the current icon position, and advances it down the left edge.
 * The returned position is not offset by Origin.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lpPos : will get the current icon position
 *
 * RETURN:
 * None
 */
static void LISTVIEW_NextIconPosLeft(LISTVIEW_INFO *infoPtr, LPPOINT lpPos)
{
    INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
    
    *lpPos = infoPtr->currIconPos;
    
    infoPtr->currIconPos.y += infoPtr->nItemHeight;
    if (infoPtr->currIconPos.y + infoPtr->nItemHeight <= nListHeight) return;

    infoPtr->currIconPos.x += infoPtr->nItemWidth;
    infoPtr->currIconPos.y  = 0;
}

    
/***
 * DESCRIPTION:
 * Moves an icon to the specified position.
 * It takes care of invalidating the item, etc.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : the item to move
 * [I] lpPos : the new icon position
 * [I] isNew : flags the item as being new
 *
 * RETURN:
 *   Success: TRUE
 *   Failure: FALSE
 */
static BOOL LISTVIEW_MoveIconTo(const LISTVIEW_INFO *infoPtr, INT nItem, const POINT *lppt, BOOL isNew)
{
    POINT old;
    
    if (!isNew)
    { 
        old.x = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosX, nItem);
        old.y = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosY, nItem);
    
        if (lppt->x == old.x && lppt->y == old.y) return TRUE;
	LISTVIEW_InvalidateItem(infoPtr, nItem);
    }

    /* Allocating a POINTER for every item is too resource intensive,
     * so we'll keep the (x,y) in different arrays */
    if (!DPA_SetPtr(infoPtr->hdpaPosX, nItem, (void *)(LONG_PTR)lppt->x)) return FALSE;
    if (!DPA_SetPtr(infoPtr->hdpaPosY, nItem, (void *)(LONG_PTR)lppt->y)) return FALSE;

    LISTVIEW_InvalidateItem(infoPtr, nItem);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Arranges listview items in icon display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nAlignCode : alignment code
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_Arrange(LISTVIEW_INFO *infoPtr, INT nAlignCode)
{
    void (*next_pos)(LISTVIEW_INFO *, LPPOINT);
    POINT pos;
    INT i;

    if (infoPtr->uView != LV_VIEW_ICON && infoPtr->uView != LV_VIEW_SMALLICON) return FALSE;
  
    TRACE("nAlignCode=%d\n", nAlignCode);

    if (nAlignCode == LVA_DEFAULT)
    {
	if (infoPtr->dwStyle & LVS_ALIGNLEFT) nAlignCode = LVA_ALIGNLEFT;
        else nAlignCode = LVA_ALIGNTOP;
    }
   
    switch (nAlignCode)
    {
    case LVA_ALIGNLEFT:  next_pos = LISTVIEW_NextIconPosLeft; break;
    case LVA_ALIGNTOP:   next_pos = LISTVIEW_NextIconPosTop;  break;
    case LVA_SNAPTOGRID: next_pos = LISTVIEW_NextIconPosTop;  break; /* FIXME */
    default: return FALSE;
    }
    
    infoPtr->currIconPos.x = infoPtr->currIconPos.y = 0;
    for (i = 0; i < infoPtr->nItemCount; i++)
    {
	next_pos(infoPtr, &pos);
	LISTVIEW_MoveIconTo(infoPtr, i, &pos, FALSE);
    }

    return TRUE;
}
  
/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle of all the items, not offset by Origin.
 * For LVS_REPORT always returns empty rectangle.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lprcView : bounding rectangle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static void LISTVIEW_GetAreaRect(const LISTVIEW_INFO *infoPtr, LPRECT lprcView)
{
    INT i, x, y;

    SetRectEmpty(lprcView);

    switch (infoPtr->uView)
    {
    case LV_VIEW_ICON:
    case LV_VIEW_SMALLICON:
	for (i = 0; i < infoPtr->nItemCount; i++)
	{
	    x = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosX, i);
            y = (LONG_PTR)DPA_GetPtr(infoPtr->hdpaPosY, i);
	    lprcView->right = max(lprcView->right, x);
	    lprcView->bottom = max(lprcView->bottom, y);
	}
	if (infoPtr->nItemCount > 0)
	{
	    lprcView->right += infoPtr->nItemWidth;
	    lprcView->bottom += infoPtr->nItemHeight;
	}
	break;

    case LV_VIEW_LIST:
	y = LISTVIEW_GetCountPerColumn(infoPtr);
	x = infoPtr->nItemCount / y;
	if (infoPtr->nItemCount % y) x++;
	lprcView->right = x * infoPtr->nItemWidth;
	lprcView->bottom = y * infoPtr->nItemHeight;
	break;
    }
}

/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle of all the items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lprcView : bounding rectangle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetViewRect(const LISTVIEW_INFO *infoPtr, LPRECT lprcView)
{
    POINT ptOrigin;

    TRACE("(lprcView=%p)\n", lprcView);

    if (!lprcView) return FALSE;

    LISTVIEW_GetAreaRect(infoPtr, lprcView);

    if (infoPtr->uView != LV_VIEW_DETAILS)
    {
        LISTVIEW_GetOrigin(infoPtr, &ptOrigin);
        OffsetRect(lprcView, ptOrigin.x, ptOrigin.y);
    }

    TRACE("lprcView=%s\n", wine_dbgstr_rect(lprcView));

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the subitem pointer associated with the subitem index.
 *
 * PARAMETER(S):
 * [I] hdpaSubItems : DPA handle for a specific item
 * [I] nSubItem : index of subitem
 *
 * RETURN:
 *   SUCCESS : subitem pointer
 *   FAILURE : NULL
 */
static SUBITEM_INFO* LISTVIEW_GetSubItemPtr(HDPA hdpaSubItems, INT nSubItem)
{
    SUBITEM_INFO *lpSubItem;
    INT i;

    /* we should binary search here if need be */
    for (i = 1; i < DPA_GetPtrCount(hdpaSubItems); i++)
    {
        lpSubItem = DPA_GetPtr(hdpaSubItems, i);
	if (lpSubItem->iSubItem == nSubItem)
	    return lpSubItem;
    }

    return NULL;
}


/***
 * DESCRIPTION:
 * Calculates the desired item width.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *  The desired item width.
 */
static INT LISTVIEW_CalculateItemWidth(const LISTVIEW_INFO *infoPtr)
{
    INT nItemWidth = 0;

    TRACE("view %ld\n", infoPtr->uView);

    if (infoPtr->uView == LV_VIEW_ICON)
	nItemWidth = infoPtr->iconSpacing.cx;
    else if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	if (DPA_GetPtrCount(infoPtr->hdpaColumns) > 0)
	{
	    RECT rcHeader;
	    INT index;

	    index = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX,
                                 DPA_GetPtrCount(infoPtr->hdpaColumns) - 1, 0);

	    LISTVIEW_GetHeaderRect(infoPtr, index, &rcHeader);
            nItemWidth = rcHeader.right;
	}
    }
    else /* LV_VIEW_SMALLICON, or LV_VIEW_LIST */
    {
	WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
	LVITEMW lvItem;
	INT i;

	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 0;

	for (i = 0; i < infoPtr->nItemCount; i++)
	{
	    lvItem.iItem = i;
	    lvItem.pszText = szDispText;
	    lvItem.cchTextMax = DISP_TEXT_SIZE;
	    if (LISTVIEW_GetItemW(infoPtr, &lvItem))
		nItemWidth = max(LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE),
				 nItemWidth);
	}

        if (infoPtr->himlSmall) nItemWidth += infoPtr->iconSize.cx; 
        if (infoPtr->himlState) nItemWidth += infoPtr->iconStateSize.cx;

        nItemWidth = max(DEFAULT_COLUMN_WIDTH, nItemWidth + WIDTH_PADDING);
    }

    return nItemWidth;
}

/***
 * DESCRIPTION:
 * Calculates the desired item height.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *  The desired item height.
 */
static INT LISTVIEW_CalculateItemHeight(const LISTVIEW_INFO *infoPtr)
{
    INT nItemHeight;

    TRACE("view %ld\n", infoPtr->uView);

    if (infoPtr->uView == LV_VIEW_ICON)
	nItemHeight = infoPtr->iconSpacing.cy;
    else
    {
	nItemHeight = infoPtr->ntmHeight;
	if (infoPtr->himlState)
	    nItemHeight = max(nItemHeight, infoPtr->iconStateSize.cy);
	if (infoPtr->himlSmall)
	    nItemHeight = max(nItemHeight, infoPtr->iconSize.cy);
	nItemHeight += HEIGHT_PADDING;
    if (infoPtr->nMeasureItemHeight > 0)
        nItemHeight = infoPtr->nMeasureItemHeight;
    }

    return max(nItemHeight, 1);
}

/***
 * DESCRIPTION:
 * Updates the width, and height of an item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *  None.
 */
static inline void LISTVIEW_UpdateItemSize(LISTVIEW_INFO *infoPtr)
{
    infoPtr->nItemWidth = LISTVIEW_CalculateItemWidth(infoPtr);
    infoPtr->nItemHeight = LISTVIEW_CalculateItemHeight(infoPtr);
}


/***
 * DESCRIPTION:
 * Retrieves and saves important text metrics info for the current
 * Listview font.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 */
static void LISTVIEW_SaveTextMetrics(LISTVIEW_INFO *infoPtr)
{
    HDC hdc = GetDC(infoPtr->hwndSelf);
    HFONT hFont = infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont;
    HFONT hOldFont = SelectObject(hdc, hFont);
    TEXTMETRICW tm;
    SIZE sz;

    if (GetTextMetricsW(hdc, &tm))
    {
	infoPtr->ntmHeight = tm.tmHeight;
	infoPtr->ntmMaxCharWidth = tm.tmMaxCharWidth;
    }

    if (GetTextExtentPoint32A(hdc, "...", 3, &sz))
	infoPtr->nEllipsisWidth = sz.cx;
	
    SelectObject(hdc, hOldFont);
    ReleaseDC(infoPtr->hwndSelf, hdc);
    
    TRACE("tmHeight=%d\n", infoPtr->ntmHeight);
}

/***
 * DESCRIPTION:
 * A compare function for ranges
 *
 * PARAMETER(S)
 * [I] range1 : pointer to range 1;
 * [I] range2 : pointer to range 2;
 * [I] flags : flags
 *
 * RETURNS:
 * > 0 : if range 1 > range 2
 * < 0 : if range 2 > range 1
 * = 0 : if range intersects range 2
 */
static INT CALLBACK ranges_cmp(LPVOID range1, LPVOID range2, LPARAM flags)
{
    INT cmp;
    
    if (((RANGE*)range1)->upper <= ((RANGE*)range2)->lower) 
	cmp = -1;
    else if (((RANGE*)range2)->upper <= ((RANGE*)range1)->lower) 
	cmp = 1;
    else 
	cmp = 0;

    TRACE("range1=%s, range2=%s, cmp=%d\n", debugrange(range1), debugrange(range2), cmp);

    return cmp;
}

#define ranges_check(ranges, desc) if (TRACE_ON(listview)) ranges_assert(ranges, desc, __FILE__, __LINE__)

static void ranges_assert(RANGES ranges, LPCSTR desc, const char *file, int line)
{
    INT i;
    RANGE *prev, *curr;
    
    TRACE("*** Checking %s:%d:%s ***\n", file, line, desc);
    assert (ranges);
    assert (DPA_GetPtrCount(ranges->hdpa) >= 0);
    ranges_dump(ranges);
    if (DPA_GetPtrCount(ranges->hdpa) > 0)
    {
	prev = DPA_GetPtr(ranges->hdpa, 0);
	assert (prev->lower >= 0 && prev->lower < prev->upper);
	for (i = 1; i < DPA_GetPtrCount(ranges->hdpa); i++)
	{
	    curr = DPA_GetPtr(ranges->hdpa, i);
	    assert (prev->upper <= curr->lower);
	    assert (curr->lower < curr->upper);
	    prev = curr;
	}
    }
    TRACE("--- Done checking---\n");
}

static RANGES ranges_create(int count)
{
    RANGES ranges = Alloc(sizeof(*ranges));
    if (!ranges) return NULL;
    ranges->hdpa = DPA_Create(count);
    if (ranges->hdpa) return ranges;
    Free(ranges);
    return NULL;
}

static void ranges_clear(RANGES ranges)
{
    INT i;
	
    for(i = 0; i < DPA_GetPtrCount(ranges->hdpa); i++)
        Free(DPA_GetPtr(ranges->hdpa, i));
    DPA_DeleteAllPtrs(ranges->hdpa);
}


static void ranges_destroy(RANGES ranges)
{
    if (!ranges) return;
    ranges_clear(ranges);
    DPA_Destroy(ranges->hdpa);
    Free(ranges);
}

static RANGES ranges_clone(RANGES ranges)
{
    RANGES clone;
    INT i;
	   
    if (!(clone = ranges_create(DPA_GetPtrCount(ranges->hdpa)))) goto fail;

    for (i = 0; i < DPA_GetPtrCount(ranges->hdpa); i++)
    {
        RANGE *newrng = Alloc(sizeof(*newrng));
	if (!newrng) goto fail;
	*newrng = *((RANGE*)DPA_GetPtr(ranges->hdpa, i));
        if (!DPA_SetPtr(clone->hdpa, i, newrng))
        {
            Free(newrng);
            goto fail;
        }
    }
    return clone;
    
fail:
    TRACE ("clone failed\n");
    ranges_destroy(clone);
    return NULL;
}

static RANGES ranges_diff(RANGES ranges, RANGES sub)
{
    INT i;

    for (i = 0; i < DPA_GetPtrCount(sub->hdpa); i++)
	ranges_del(ranges, *((RANGE *)DPA_GetPtr(sub->hdpa, i)));

    return ranges;
}

static void ranges_dump(RANGES ranges)
{
    INT i;

    for (i = 0; i < DPA_GetPtrCount(ranges->hdpa); i++)
    	TRACE("   %s\n", debugrange(DPA_GetPtr(ranges->hdpa, i)));
}

static inline BOOL ranges_contain(RANGES ranges, INT nItem)
{
    RANGE srchrng = { nItem, nItem + 1 };

    TRACE("(nItem=%d)\n", nItem);
    ranges_check(ranges, "before contain");
    return DPA_Search(ranges->hdpa, &srchrng, 0, ranges_cmp, 0, DPAS_SORTED) != -1;
}

static INT ranges_itemcount(RANGES ranges)
{
    INT i, count = 0;
    
    for (i = 0; i < DPA_GetPtrCount(ranges->hdpa); i++)
    {
	RANGE *sel = DPA_GetPtr(ranges->hdpa, i);
	count += sel->upper - sel->lower;
    }

    return count;
}

static BOOL ranges_shift(RANGES ranges, INT nItem, INT delta, INT nUpper)
{
    RANGE srchrng = { nItem, nItem + 1 }, *chkrng;
    INT index;

    index = DPA_Search(ranges->hdpa, &srchrng, 0, ranges_cmp, 0, DPAS_SORTED | DPAS_INSERTAFTER);
    if (index == -1) return TRUE;

    for (; index < DPA_GetPtrCount(ranges->hdpa); index++)
    {
	chkrng = DPA_GetPtr(ranges->hdpa, index);
    	if (chkrng->lower >= nItem)
	    chkrng->lower = max(min(chkrng->lower + delta, nUpper - 1), 0);
        if (chkrng->upper > nItem)
	    chkrng->upper = max(min(chkrng->upper + delta, nUpper), 0);
    }
    return TRUE;
}

static BOOL ranges_add(RANGES ranges, RANGE range)
{
    RANGE srchrgn;
    INT index;

    TRACE("(%s)\n", debugrange(&range));
    ranges_check(ranges, "before add");

    /* try find overlapping regions first */
    srchrgn.lower = range.lower - 1;
    srchrgn.upper = range.upper + 1;
    index = DPA_Search(ranges->hdpa, &srchrgn, 0, ranges_cmp, 0, DPAS_SORTED);
   
    if (index == -1)
    {
	RANGE *newrgn;

	TRACE("Adding new range\n");

	/* create the brand new range to insert */	
        newrgn = Alloc(sizeof(*newrgn));
	if(!newrgn) goto fail;
	*newrgn = range;
	
	/* figure out where to insert it */
	index = DPA_Search(ranges->hdpa, newrgn, 0, ranges_cmp, 0, DPAS_SORTED | DPAS_INSERTAFTER);
	TRACE("index=%d\n", index);
	if (index == -1) index = 0;
	
	/* and get it over with */
	if (DPA_InsertPtr(ranges->hdpa, index, newrgn) == -1)
	{
	    Free(newrgn);
	    goto fail;
	}
    }
    else
    {
	RANGE *chkrgn, *mrgrgn;
	INT fromindex, mergeindex;

	chkrgn = DPA_GetPtr(ranges->hdpa, index);
	TRACE("Merge with %s @%d\n", debugrange(chkrgn), index);

	chkrgn->lower = min(range.lower, chkrgn->lower);
	chkrgn->upper = max(range.upper, chkrgn->upper);
	
	TRACE("New range %s @%d\n", debugrange(chkrgn), index);

        /* merge now common ranges */
	fromindex = 0;
	srchrgn.lower = chkrgn->lower - 1;
	srchrgn.upper = chkrgn->upper + 1;
	    
	do
	{
	    mergeindex = DPA_Search(ranges->hdpa, &srchrgn, fromindex, ranges_cmp, 0, 0);
	    if (mergeindex == -1) break;
	    if (mergeindex == index) 
	    {
		fromindex = index + 1;
		continue;
	    }
	  
	    TRACE("Merge with index %i\n", mergeindex);
	    
	    mrgrgn = DPA_GetPtr(ranges->hdpa, mergeindex);
	    chkrgn->lower = min(chkrgn->lower, mrgrgn->lower);
	    chkrgn->upper = max(chkrgn->upper, mrgrgn->upper);
	    Free(mrgrgn);
	    DPA_DeletePtr(ranges->hdpa, mergeindex);
	    if (mergeindex < index) index --;
	} while(1);
    }

    ranges_check(ranges, "after add");
    return TRUE;
    
fail:
    ranges_check(ranges, "failed add");
    return FALSE;
}

static BOOL ranges_del(RANGES ranges, RANGE range)
{
    RANGE *chkrgn;
    INT index;

    TRACE("(%s)\n", debugrange(&range));
    ranges_check(ranges, "before del");

    /* we don't use DPAS_SORTED here, since we need *
     * to find the first overlapping range          */
    index = DPA_Search(ranges->hdpa, &range, 0, ranges_cmp, 0, 0);
    while(index != -1)
    {
	chkrgn = DPA_GetPtr(ranges->hdpa, index);

	TRACE("Matches range %s @%d\n", debugrange(chkrgn), index);

	/* case 1: Same range */
	if ( (chkrgn->upper == range.upper) &&
	     (chkrgn->lower == range.lower) )
	{
	    DPA_DeletePtr(ranges->hdpa, index);
	    Free(chkrgn);
	    break;
	}
	/* case 2: engulf */
	else if ( (chkrgn->upper <= range.upper) &&
		  (chkrgn->lower >= range.lower) )
	{
	    DPA_DeletePtr(ranges->hdpa, index);
	    Free(chkrgn);
	}
	/* case 3: overlap upper */
	else if ( (chkrgn->upper <= range.upper) &&
		  (chkrgn->lower < range.lower) )
	{
	    chkrgn->upper = range.lower;
	}
	/* case 4: overlap lower */
	else if ( (chkrgn->upper > range.upper) &&
		  (chkrgn->lower >= range.lower) )
	{
	    chkrgn->lower = range.upper;
	    break;
	}
	/* case 5: fully internal */
	else
	{
	    RANGE *newrgn;

	    if (!(newrgn = Alloc(sizeof(*newrgn)))) goto fail;
	    newrgn->lower = chkrgn->lower;
	    newrgn->upper = range.lower;
	    chkrgn->lower = range.upper;
	    if (DPA_InsertPtr(ranges->hdpa, index, newrgn) == -1)
	    {
		Free(newrgn);
		goto fail;
	    }
	    break;
	}

	index = DPA_Search(ranges->hdpa, &range, index, ranges_cmp, 0, 0);
    }

    ranges_check(ranges, "after del");
    return TRUE;

fail:
    ranges_check(ranges, "failed del");
    return FALSE;
}

/***
* DESCRIPTION:
* Removes all selection ranges
*
* Parameters(s):
* [I] infoPtr : valid pointer to the listview structure
* [I] toSkip : item range to skip removing the selection
*
* RETURNS:
*   SUCCESS : TRUE
*   FAILURE : FALSE
*/
static BOOL LISTVIEW_DeselectAllSkipItems(LISTVIEW_INFO *infoPtr, RANGES toSkip)
{
    LVITEMW lvItem;
    ITERATOR i;
    RANGES clone;

    TRACE("()\n");

    lvItem.state = 0;
    lvItem.stateMask = LVIS_SELECTED;

    /* Only send one deselect all (-1) notification for LVS_OWNERDATA style */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
        LISTVIEW_SetItemState(infoPtr, -1, &lvItem);
        return TRUE;
    }

    /* need to clone the DPA because callbacks can change it */
    if (!(clone = ranges_clone(infoPtr->selectionRanges))) return FALSE;
    iterator_rangesitems(&i, ranges_diff(clone, toSkip));
    while(iterator_next(&i))
	LISTVIEW_SetItemState(infoPtr, i.nItem, &lvItem);
    /* note that the iterator destructor will free the cloned range */
    iterator_destroy(&i);

    return TRUE;
}

static inline BOOL LISTVIEW_DeselectAllSkipItem(LISTVIEW_INFO *infoPtr, INT nItem)
{
    RANGES toSkip;
   
    if (!(toSkip = ranges_create(1))) return FALSE;
    if (nItem != -1) ranges_additem(toSkip, nItem);
    LISTVIEW_DeselectAllSkipItems(infoPtr, toSkip);
    ranges_destroy(toSkip);
    return TRUE;
}

static inline BOOL LISTVIEW_DeselectAll(LISTVIEW_INFO *infoPtr)
{
    return LISTVIEW_DeselectAllSkipItem(infoPtr, -1);
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that are marked as selected.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of items selected.
 */
static INT LISTVIEW_GetSelectedCount(const LISTVIEW_INFO *infoPtr)
{
    INT nSelectedCount = 0;

    if (infoPtr->uCallbackMask & LVIS_SELECTED)
    {
        INT i;
	for (i = 0; i < infoPtr->nItemCount; i++)
  	{
	    if (LISTVIEW_GetItemState(infoPtr, i, LVIS_SELECTED))
		nSelectedCount++;
	}
    }
    else
	nSelectedCount = ranges_itemcount(infoPtr->selectionRanges);

    TRACE("nSelectedCount=%d\n", nSelectedCount);
    return nSelectedCount;
}

/***
 * DESCRIPTION:
 * Manages the item focus.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 *
 * RETURN:
 *   TRUE : focused item changed
 *   FALSE : focused item has NOT changed
 */
static inline BOOL LISTVIEW_SetItemFocus(LISTVIEW_INFO *infoPtr, INT nItem)
{
    INT oldFocus = infoPtr->nFocusedItem;
    LVITEMW lvItem;

    if (nItem == infoPtr->nFocusedItem) return FALSE;
    
    lvItem.state =  nItem == -1 ? 0 : LVIS_FOCUSED;
    lvItem.stateMask = LVIS_FOCUSED;
    LISTVIEW_SetItemState(infoPtr, nItem == -1 ? infoPtr->nFocusedItem : nItem, &lvItem);

    return oldFocus != infoPtr->nFocusedItem;
}

static INT shift_item(const LISTVIEW_INFO *infoPtr, INT nShiftItem, INT nItem, INT direction)
{
    if (nShiftItem < nItem) return nShiftItem;

    if (nShiftItem > nItem) return nShiftItem + direction;

    if (direction > 0) return nShiftItem + direction;

    return min(nShiftItem, infoPtr->nItemCount - 1);
}

/* This function updates focus index.

Parameters:
   focus : current focus index
   item : index of item to be added/removed
   direction : add/remove flag
*/
static void LISTVIEW_ShiftFocus(LISTVIEW_INFO *infoPtr, INT focus, INT item, INT direction)
{
    DWORD old_mask = infoPtr->notify_mask & NOTIFY_MASK_ITEM_CHANGE;

    infoPtr->notify_mask &= ~NOTIFY_MASK_ITEM_CHANGE;
    focus = shift_item(infoPtr, focus, item, direction);
    if (focus != infoPtr->nFocusedItem)
        LISTVIEW_SetItemFocus(infoPtr, focus);
    infoPtr->notify_mask |= old_mask;
}

/**
* DESCRIPTION:
* Updates the various indices after an item has been inserted or deleted.
*
* PARAMETER(S):
* [I] infoPtr : valid pointer to the listview structure
* [I] nItem : item index
* [I] direction : Direction of shift, +1 or -1.
*
* RETURN:
* None
*/
static void LISTVIEW_ShiftIndices(LISTVIEW_INFO *infoPtr, INT nItem, INT direction)
{
    TRACE("Shifting %i, %i steps\n", nItem, direction);

    ranges_shift(infoPtr->selectionRanges, nItem, direction, infoPtr->nItemCount);
    assert(abs(direction) == 1);
    infoPtr->nSelectionMark = shift_item(infoPtr, infoPtr->nSelectionMark, nItem, direction);

    /* But we are not supposed to modify nHotItem! */
}

/**
 * DESCRIPTION:
 * Adds a block of selections.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 *
 * RETURN:
 * Whether the window is still valid.
 */
static BOOL LISTVIEW_AddGroupSelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
    INT nFirst = min(infoPtr->nSelectionMark, nItem);
    INT nLast = max(infoPtr->nSelectionMark, nItem);
    HWND hwndSelf = infoPtr->hwndSelf;
    DWORD old_mask;
    LVITEMW item;
    INT i;

    /* Temporarily disable change notification
     * If the control is LVS_OWNERDATA, we need to send
     * only one LVN_ODSTATECHANGED notification.
     * See MSDN documentation for LVN_ITEMCHANGED.
     */
    old_mask = infoPtr->notify_mask & NOTIFY_MASK_ITEM_CHANGE;
    if (infoPtr->dwStyle & LVS_OWNERDATA)
        infoPtr->notify_mask &= ~NOTIFY_MASK_ITEM_CHANGE;

    if (nFirst == -1) nFirst = nItem;

    item.state = LVIS_SELECTED;
    item.stateMask = LVIS_SELECTED;

    for (i = nFirst; i <= nLast; i++)
	LISTVIEW_SetItemState(infoPtr,i,&item);

    if (infoPtr->dwStyle & LVS_OWNERDATA)
        LISTVIEW_SetOwnerDataState(infoPtr, nFirst, nLast, &item);

    if (!IsWindow(hwndSelf))
        return FALSE;
    infoPtr->notify_mask |= old_mask;
    return TRUE;
}


/***
 * DESCRIPTION:
 * Sets a single group selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 *
 * RETURN:
 * None
 */
static void LISTVIEW_SetGroupSelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
    INT nFirst = -1, nLast = -1;
    RANGES selection;
    DWORD old_mask;
    LVITEMW item;
    ITERATOR i;

    if (!(selection = ranges_create(100))) return;

    item.state = LVIS_SELECTED; 
    item.stateMask = LVIS_SELECTED;

    if ((infoPtr->uView == LV_VIEW_LIST) || (infoPtr->uView == LV_VIEW_DETAILS))
    {
	if (infoPtr->nSelectionMark == -1)
	{
	    infoPtr->nSelectionMark = nItem;
	    ranges_additem(selection, nItem);
	}
	else
	{
	    RANGE sel;
	    
	    sel.lower = min(infoPtr->nSelectionMark, nItem);
	    sel.upper = max(infoPtr->nSelectionMark, nItem) + 1;
	    ranges_add(selection, sel);
	}
    }
    else
    {
	RECT rcItem, rcSel, rcSelMark;
	POINT ptItem;
	
	rcItem.left = LVIR_BOUNDS;
	if (!LISTVIEW_GetItemRect(infoPtr, nItem, &rcItem)) {
	     ranges_destroy (selection);
	     return;
	}
	rcSelMark.left = LVIR_BOUNDS;
	if (!LISTVIEW_GetItemRect(infoPtr, infoPtr->nSelectionMark, &rcSelMark)) {
	     ranges_destroy (selection);
	     return;
	}
	UnionRect(&rcSel, &rcItem, &rcSelMark);
	iterator_frameditems(&i, infoPtr, &rcSel);
	while(iterator_next(&i))
	{
	    LISTVIEW_GetItemPosition(infoPtr, i.nItem, &ptItem);
	    if (PtInRect(&rcSel, ptItem)) ranges_additem(selection, i.nItem);
	}
	iterator_destroy(&i);
    }

    /* Disable per item notifications on LVS_OWNERDATA style */
    old_mask = infoPtr->notify_mask & NOTIFY_MASK_ITEM_CHANGE;
    if (infoPtr->dwStyle & LVS_OWNERDATA)
        infoPtr->notify_mask &= ~NOTIFY_MASK_ITEM_CHANGE;

    LISTVIEW_DeselectAllSkipItems(infoPtr, selection);

    iterator_rangesitems(&i, selection);
    while(iterator_next(&i))
    {
        /* Find the range for LVN_ODSTATECHANGED */
        if (nFirst == -1)
            nFirst = i.nItem;
        nLast = i.nItem;
        LISTVIEW_SetItemState(infoPtr, i.nItem, &item);
    }
    /* this will also destroy the selection */
    iterator_destroy(&i);

    if (infoPtr->dwStyle & LVS_OWNERDATA)
        LISTVIEW_SetOwnerDataState(infoPtr, nFirst, nLast, &item);

    infoPtr->notify_mask |= old_mask;
    LISTVIEW_SetItemFocus(infoPtr, nItem);
}

/***
 * DESCRIPTION:
 * Sets a single selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 *
 * RETURN:
 * None
 */
static void LISTVIEW_SetSelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
    LVITEMW lvItem;

    TRACE("nItem=%d\n", nItem);
    
    LISTVIEW_DeselectAllSkipItem(infoPtr, nItem);

    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    LISTVIEW_SetItemState(infoPtr, nItem, &lvItem);

    infoPtr->nSelectionMark = nItem;
}

/***
 * DESCRIPTION:
 * Set selection(s) with keyboard.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] space : VK_SPACE code sent
 *
 * RETURN:
 *   SUCCESS : TRUE (needs to be repainted)
 *   FAILURE : FALSE (nothing has changed)
 */
static BOOL LISTVIEW_KeySelection(LISTVIEW_INFO *infoPtr, INT nItem, BOOL space)
{
  /* FIXME: pass in the state */
  WORD wShift = GetKeyState(VK_SHIFT) & 0x8000;
  WORD wCtrl = GetKeyState(VK_CONTROL) & 0x8000;
  BOOL bResult = FALSE;

  TRACE("nItem=%d, wShift=%d, wCtrl=%d\n", nItem, wShift, wCtrl);
  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    bResult = TRUE;

    if (infoPtr->dwStyle & LVS_SINGLESEL || (wShift == 0 && wCtrl == 0))
      LISTVIEW_SetSelection(infoPtr, nItem);
    else
    {
      if (wShift)
        LISTVIEW_SetGroupSelection(infoPtr, nItem);
      else if (wCtrl)
      {
        LVITEMW lvItem;
        lvItem.state = ~LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED);
        lvItem.stateMask = LVIS_SELECTED;
        if (space)
        {
            LISTVIEW_SetItemState(infoPtr, nItem, &lvItem);
            if (lvItem.state & LVIS_SELECTED)
                infoPtr->nSelectionMark = nItem;
        }
        bResult = LISTVIEW_SetItemFocus(infoPtr, nItem);
      }
    }
    LISTVIEW_EnsureVisible(infoPtr, nItem, FALSE);
  }

  UpdateWindow(infoPtr->hwndSelf); /* update client area */
  return bResult;
}

static BOOL LISTVIEW_GetItemAtPt(const LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, POINT pt)
{
    LVHITTESTINFO lvHitTestInfo;

    ZeroMemory(&lvHitTestInfo, sizeof(lvHitTestInfo));
    lvHitTestInfo.pt.x = pt.x;
    lvHitTestInfo.pt.y = pt.y;

    LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, FALSE);

    lpLVItem->mask = LVIF_PARAM;
    lpLVItem->iItem = lvHitTestInfo.iItem;
    lpLVItem->iSubItem = 0;

    return LISTVIEW_GetItemT(infoPtr, lpLVItem, TRUE);
}

static inline BOOL LISTVIEW_IsHotTracking(const LISTVIEW_INFO *infoPtr)
{
    return ((infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT) ||
            (infoPtr->dwLvExStyle & LVS_EX_ONECLICKACTIVATE) ||
            (infoPtr->dwLvExStyle & LVS_EX_TWOCLICKACTIVATE));
}

/***
 * DESCRIPTION:
 * Called when the mouse is being actively tracked and has hovered for a specified
 * amount of time
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fwKeys : key indicator
 * [I] x,y : mouse position
 *
 * RETURN:
 *   0 if the message was processed, non-zero if there was an error
 *
 * INFO:
 * LVS_EX_TRACKSELECT: An item is automatically selected when the cursor remains
 * over the item for a certain period of time.
 *
 */
static LRESULT LISTVIEW_MouseHover(LISTVIEW_INFO *infoPtr, INT x, INT y)
{
    NMHDR hdr;

    if (notify_hdr(infoPtr, NM_HOVER, &hdr)) return 0;

    if (LISTVIEW_IsHotTracking(infoPtr))
    {
        LVITEMW item;
        POINT pt;

        pt.x = x;
        pt.y = y;

        if (LISTVIEW_GetItemAtPt(infoPtr, &item, pt))
            LISTVIEW_SetSelection(infoPtr, item.iItem);

        SetFocus(infoPtr->hwndSelf);
    }

    return 0;
}

#define SCROLL_LEFT   0x1
#define SCROLL_RIGHT  0x2
#define SCROLL_UP     0x4
#define SCROLL_DOWN   0x8

/***
 * DESCRIPTION:
 * Utility routine to draw and highlight items within a marquee selection rectangle.
 *
 * PARAMETER(S):
 * [I] infoPtr     : valid pointer to the listview structure
 * [I] coords_orig : original co-ordinates of the cursor
 * [I] coords_offs : offsetted coordinates of the cursor
 * [I] offset      : offset amount
 * [I] scroll      : Bitmask of which directions we should scroll, if at all
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_MarqueeHighlight(LISTVIEW_INFO *infoPtr, const POINT *coords_orig,
                                      INT scroll)
{
    BOOL controlDown = FALSE;
    LVITEMW item;
    ITERATOR old_elems, new_elems;
    RECT rect;
    POINT coords_offs, offset;

    /* Ensure coordinates are within client bounds */
    coords_offs.x = max(min(coords_orig->x, infoPtr->rcList.right), 0);
    coords_offs.y = max(min(coords_orig->y, infoPtr->rcList.bottom), 0);

    /* Get offset */
    LISTVIEW_GetOrigin(infoPtr, &offset);

    /* Offset coordinates by the appropriate amount */
    coords_offs.x -= offset.x;
    coords_offs.y -= offset.y;

    if (coords_offs.x > infoPtr->marqueeOrigin.x)
    {
        rect.left = infoPtr->marqueeOrigin.x;
        rect.right = coords_offs.x;
    }
    else
    {
        rect.left = coords_offs.x;
        rect.right = infoPtr->marqueeOrigin.x;
    }

    if (coords_offs.y > infoPtr->marqueeOrigin.y)
    {
        rect.top = infoPtr->marqueeOrigin.y;
        rect.bottom = coords_offs.y;
    }
    else
    {
        rect.top = coords_offs.y;
        rect.bottom = infoPtr->marqueeOrigin.y;
    }

    /* Cancel out the old marquee rectangle and draw the new one */
    LISTVIEW_InvalidateRect(infoPtr, &infoPtr->marqueeDrawRect);

    /* Scroll by the appropriate distance if applicable - speed up scrolling as
       the cursor is further away */

    if ((scroll & SCROLL_LEFT) && (coords_orig->x <= 0))
        LISTVIEW_Scroll(infoPtr, coords_orig->x, 0);

    if ((scroll & SCROLL_RIGHT) && (coords_orig->x >= infoPtr->rcList.right))
        LISTVIEW_Scroll(infoPtr, (coords_orig->x - infoPtr->rcList.right), 0);

    if ((scroll & SCROLL_UP) && (coords_orig->y <= 0))
        LISTVIEW_Scroll(infoPtr, 0, coords_orig->y);

    if ((scroll & SCROLL_DOWN) && (coords_orig->y >= infoPtr->rcList.bottom))
        LISTVIEW_Scroll(infoPtr, 0, (coords_orig->y - infoPtr->rcList.bottom));

    iterator_frameditems_absolute(&old_elems, infoPtr, &infoPtr->marqueeRect);

    infoPtr->marqueeRect = rect;
    infoPtr->marqueeDrawRect = rect;
    OffsetRect(&infoPtr->marqueeDrawRect, offset.x, offset.y);

    iterator_frameditems_absolute(&new_elems, infoPtr, &infoPtr->marqueeRect);
    iterator_remove_common_items(&old_elems, &new_elems);

    /* Iterate over no longer selected items */
    while (iterator_next(&old_elems))
    {
        if (old_elems.nItem > -1)
        {
            if (LISTVIEW_GetItemState(infoPtr, old_elems.nItem, LVIS_SELECTED) == LVIS_SELECTED)
                item.state = 0;
            else
                item.state = LVIS_SELECTED;

            item.stateMask = LVIS_SELECTED;

            LISTVIEW_SetItemState(infoPtr, old_elems.nItem, &item);
        }
    }
    iterator_destroy(&old_elems);


    /* Iterate over newly selected items */
    if (GetKeyState(VK_CONTROL) & 0x8000)
        controlDown = TRUE;

    while (iterator_next(&new_elems))
    {
        if (new_elems.nItem > -1)
        {
            /* If CTRL is pressed, invert. If not, always select the item. */
            if ((controlDown) && (LISTVIEW_GetItemState(infoPtr, new_elems.nItem, LVIS_SELECTED)))
                item.state = 0;
            else
                item.state = LVIS_SELECTED;

            item.stateMask = LVIS_SELECTED;

            LISTVIEW_SetItemState(infoPtr, new_elems.nItem, &item);
        }
    }
    iterator_destroy(&new_elems);

    LISTVIEW_InvalidateRect(infoPtr, &infoPtr->marqueeDrawRect);
}

/***
 * DESCRIPTION:
 * Called when we are in a marquee selection that involves scrolling the listview (ie,
 * the cursor is outside the bounds of the client area). This is a TIMERPROC.
 *
 * PARAMETER(S):
 * [I] hwnd : Handle to the listview
 * [I] uMsg : WM_TIMER (ignored)
 * [I] idEvent : The timer ID interpreted as a pointer to a LISTVIEW_INFO struct
 * [I] dwTimer : The elapsed time (ignored)
 *
 * RETURN:
 *   None.
 */
static VOID CALLBACK LISTVIEW_ScrollTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    LISTVIEW_INFO *infoPtr;
    SCROLLINFO scrollInfo;
    POINT coords;
    INT scroll = 0;

    infoPtr = (LISTVIEW_INFO *) idEvent;

    if (!infoPtr)
        return;

    /* Get the current cursor position and convert to client coordinates */
    GetCursorPos(&coords);
    ScreenToClient(hWnd, &coords);

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;

    /* Work out in which directions we can scroll */
    if (GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
    {
        if (scrollInfo.nPos != scrollInfo.nMin)
            scroll |= SCROLL_UP;

        if (((scrollInfo.nPage + scrollInfo.nPos) - 1) != scrollInfo.nMax)
            scroll |= SCROLL_DOWN;
    }

    if (GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
    {
        if (scrollInfo.nPos != scrollInfo.nMin)
            scroll |= SCROLL_LEFT;

        if (((scrollInfo.nPage + scrollInfo.nPos) - 1) != scrollInfo.nMax)
            scroll |= SCROLL_RIGHT;
    }

    if (((coords.x <= 0) && (scroll & SCROLL_LEFT)) ||
        ((coords.y <= 0) && (scroll & SCROLL_UP))   ||
        ((coords.x >= infoPtr->rcList.right) && (scroll & SCROLL_RIGHT)) ||
        ((coords.y >= infoPtr->rcList.bottom) && (scroll & SCROLL_DOWN)))
    {
        LISTVIEW_MarqueeHighlight(infoPtr, &coords, scroll);
    }
}

/***
 * DESCRIPTION:
 * Called whenever WM_MOUSEMOVE is received.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fwKeys : key indicator
 * [I] x,y : mouse position
 *
 * RETURN:
 *   0 if the message is processed, non-zero if there was an error
 */
static LRESULT LISTVIEW_MouseMove(LISTVIEW_INFO *infoPtr, WORD fwKeys, INT x, INT y)
{
    LVHITTESTINFO ht;
    RECT rect;
    POINT pt;

    pt.x = x;
    pt.y = y;

    if (!(fwKeys & MK_LBUTTON))
        infoPtr->bLButtonDown = FALSE;

    if (infoPtr->bLButtonDown)
    {
        rect.left = rect.right = infoPtr->ptClickPos.x;
        rect.top = rect.bottom = infoPtr->ptClickPos.y;

        InflateRect(&rect, GetSystemMetrics(SM_CXDRAG), GetSystemMetrics(SM_CYDRAG));

        if (infoPtr->bMarqueeSelect)
        {
            /* Enable the timer if we're going outside our bounds, in case the user doesn't
               move the mouse again */

            if ((x <= 0) || (y <= 0) || (x >= infoPtr->rcList.right) ||
                (y >= infoPtr->rcList.bottom))
            {
                if (!infoPtr->bScrolling)
                {
                    infoPtr->bScrolling = TRUE;
                    SetTimer(infoPtr->hwndSelf, (UINT_PTR) infoPtr, 1, LISTVIEW_ScrollTimer);
                }
            }
            else
            {
                infoPtr->bScrolling = FALSE;
                KillTimer(infoPtr->hwndSelf, (UINT_PTR) infoPtr);
            }

            LISTVIEW_MarqueeHighlight(infoPtr, &pt, 0);
            return 0;
        }

        ht.pt = pt;
        LISTVIEW_HitTest(infoPtr, &ht, TRUE, TRUE);

        /* reset item marker */
        if (infoPtr->nLButtonDownItem != ht.iItem)
            infoPtr->nLButtonDownItem = -1;

        if (!PtInRect(&rect, pt))
        {
            /* this path covers the following:
               1. WM_LBUTTONDOWN over selected item (sets focus on it)
               2. change focus with keys
               3. move mouse over item from step 1 selects it and moves focus on it */
            if (infoPtr->nLButtonDownItem != -1 &&
               !LISTVIEW_GetItemState(infoPtr, infoPtr->nLButtonDownItem, LVIS_SELECTED))
            {
                LVITEMW lvItem;

                lvItem.state =  LVIS_FOCUSED | LVIS_SELECTED;
                lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;

                LISTVIEW_SetItemState(infoPtr, infoPtr->nLButtonDownItem, &lvItem);
                infoPtr->nLButtonDownItem = -1;
            }

            if (!infoPtr->bDragging)
            {
                ht.pt = infoPtr->ptClickPos;
                LISTVIEW_HitTest(infoPtr, &ht, TRUE, TRUE);

                /* If the click is outside the range of an item, begin a
                   highlight. If not, begin an item drag. */
                if (ht.iItem == -1)
                {
                    NMHDR hdr;

                    /* If we're allowing multiple selections, send notification.
                       If return value is non-zero, cancel. */
                    if (!(infoPtr->dwStyle & LVS_SINGLESEL) && (notify_hdr(infoPtr, LVN_MARQUEEBEGIN, &hdr) == 0))
                    {
                        /* Store the absolute coordinates of the click */
                        POINT offset;
                        LISTVIEW_GetOrigin(infoPtr, &offset);

                        infoPtr->marqueeOrigin.x = infoPtr->ptClickPos.x - offset.x;
                        infoPtr->marqueeOrigin.y = infoPtr->ptClickPos.y - offset.y;

                        /* Begin selection and capture mouse */
                        infoPtr->bMarqueeSelect = TRUE;
                        infoPtr->marqueeRect = rect;
                        SetCapture(infoPtr->hwndSelf);
                    }
                }
                else
                {
                    NMLISTVIEW nmlv;

                    ZeroMemory(&nmlv, sizeof(nmlv));
                    nmlv.iItem = ht.iItem;
                    nmlv.ptAction = infoPtr->ptClickPos;

                    notify_listview(infoPtr, LVN_BEGINDRAG, &nmlv);
                    infoPtr->bDragging = TRUE;
                }
            }

            return 0;
        }
    }

    /* see if we are supposed to be tracking mouse hovering */
    if (LISTVIEW_IsHotTracking(infoPtr)) {
        TRACKMOUSEEVENT trackinfo;
        DWORD flags;

        trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
        trackinfo.dwFlags = TME_QUERY;

        /* see if we are already tracking this hwnd */
        _TrackMouseEvent(&trackinfo);

        flags = TME_LEAVE;
        if(infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT)
            flags |= TME_HOVER;

        if((trackinfo.dwFlags & flags) != flags || trackinfo.hwndTrack != infoPtr->hwndSelf) {
            trackinfo.dwFlags     = flags;
            trackinfo.dwHoverTime = infoPtr->dwHoverTime;
            trackinfo.hwndTrack   = infoPtr->hwndSelf;

            /* call TRACKMOUSEEVENT so we receive WM_MOUSEHOVER messages */
            _TrackMouseEvent(&trackinfo);
        }
    }

    return 0;
}


/***
 * Tests whether the item is assignable to a list with style lStyle
 */
static inline BOOL is_assignable_item(const LVITEMW *lpLVItem, LONG lStyle)
{
    if ( (lpLVItem->mask & LVIF_TEXT) && 
	(lpLVItem->pszText == LPSTR_TEXTCALLBACKW) &&
	(lStyle & (LVS_SORTASCENDING | LVS_SORTDESCENDING)) ) return FALSE;
    
    return TRUE;
}


/***
 * DESCRIPTION:
 * Helper for LISTVIEW_SetItemT and LISTVIEW_InsertItemT: sets item attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : valid pointer to new item attributes
 * [I] isNew : the item being set is being inserted
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 * [O] bChanged : will be set to TRUE if the item really changed
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL set_main_item(LISTVIEW_INFO *infoPtr, const LVITEMW *lpLVItem, BOOL isNew, BOOL isW, BOOL *bChanged)
{
    ITEM_INFO *lpItem;
    NMLISTVIEW nmlv;
    UINT uChanged = 0;
    LVITEMW item;
    /* stateMask is ignored for LVM_INSERTITEM */
    UINT stateMask = isNew ? ~0 : lpLVItem->stateMask;
    BOOL send_change_notification;

    TRACE("()\n");

    assert(lpLVItem->iItem >= 0 && lpLVItem->iItem < infoPtr->nItemCount);
    
    if (lpLVItem->mask == 0) return TRUE;   

    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	/* a virtual listview only stores selection and focus */
	if (lpLVItem->mask & ~LVIF_STATE)
	    return FALSE;
	lpItem = NULL;
    }
    else
    {
        HDPA hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
        lpItem = DPA_GetPtr(hdpaSubItems, 0);
	assert (lpItem);
    }

    /* we need to get the lParam and state of the item */
    item.iItem = lpLVItem->iItem;
    item.iSubItem = lpLVItem->iSubItem;
    item.mask = LVIF_STATE | LVIF_PARAM;
    item.stateMask = (infoPtr->dwStyle & LVS_OWNERDATA) ? LVIS_FOCUSED | LVIS_SELECTED : ~0;

    item.state = 0;
    item.lParam = 0;
    if (!isNew && !LISTVIEW_GetItemW(infoPtr, &item)) return FALSE;

    TRACE("oldState=%x, newState=%x\n", item.state, lpLVItem->state);
    /* determine what fields will change */    
    if ((lpLVItem->mask & LVIF_STATE) && ((item.state ^ lpLVItem->state) & stateMask & ~infoPtr->uCallbackMask))
	uChanged |= LVIF_STATE;

    if ((lpLVItem->mask & LVIF_IMAGE) && (lpItem->hdr.iImage != lpLVItem->iImage))
	uChanged |= LVIF_IMAGE;

    if ((lpLVItem->mask & LVIF_PARAM) && (lpItem->lParam != lpLVItem->lParam))
	uChanged |= LVIF_PARAM;

    if ((lpLVItem->mask & LVIF_INDENT) && (lpItem->iIndent != lpLVItem->iIndent))
	uChanged |= LVIF_INDENT;

    if ((lpLVItem->mask & LVIF_TEXT) && textcmpWT(lpItem->hdr.pszText, lpLVItem->pszText, isW))
	uChanged |= LVIF_TEXT;
   
    TRACE("change mask=0x%x\n", uChanged);
    
    memset(&nmlv, 0, sizeof(NMLISTVIEW));
    nmlv.iItem = lpLVItem->iItem;
    if (lpLVItem->mask & LVIF_STATE)
    {
        nmlv.uNewState = (item.state & ~stateMask) | (lpLVItem->state & stateMask);
        nmlv.uOldState = item.state;
    }
    nmlv.uChanged = isNew ? LVIF_STATE : (uChanged ? uChanged : lpLVItem->mask);
    nmlv.lParam = item.lParam;

    /* Send change notification if the item is not being inserted, or inserted (selected|focused),
       and we are _NOT_ virtual (LVS_OWNERDATA), and change notifications
       are enabled. Even if nothing really changed we still need to send this,
       in this case uChanged mask is just set to passed item mask. */

    send_change_notification = !isNew;
    send_change_notification |= (uChanged & LVIF_STATE) && (lpLVItem->state & (LVIS_FOCUSED | LVIS_SELECTED));
    send_change_notification &= !!(infoPtr->notify_mask & NOTIFY_MASK_ITEM_CHANGE);

    if (lpItem && send_change_notification)
    {
      HWND hwndSelf = infoPtr->hwndSelf;

      if (notify_listview(infoPtr, LVN_ITEMCHANGING, &nmlv))
	return FALSE;
      if (!IsWindow(hwndSelf))
	return FALSE;
    }

    /* When item is inserted we need to shift existing focus index if new item has lower index. */
    if (isNew && (stateMask & ~infoPtr->uCallbackMask & LVIS_FOCUSED) &&
        /* this means we won't hit a focus change path later */
        ((uChanged & LVIF_STATE) == 0 || (!(lpLVItem->state & LVIS_FOCUSED) && (infoPtr->nFocusedItem != lpLVItem->iItem))))
    {
        if (infoPtr->nFocusedItem != -1 && (lpLVItem->iItem <= infoPtr->nFocusedItem))
            infoPtr->nFocusedItem++;
    }

    if (!uChanged) return TRUE;
    *bChanged = TRUE;

    /* copy information */
    if (lpLVItem->mask & LVIF_TEXT)
        textsetptrT(&lpItem->hdr.pszText, lpLVItem->pszText, isW);

    if (lpLVItem->mask & LVIF_IMAGE)
	lpItem->hdr.iImage = lpLVItem->iImage;

    if (lpLVItem->mask & LVIF_PARAM)
	lpItem->lParam = lpLVItem->lParam;

    if (lpLVItem->mask & LVIF_INDENT)
	lpItem->iIndent = lpLVItem->iIndent;

    if (uChanged & LVIF_STATE)
    {
	if (lpItem && (stateMask & ~infoPtr->uCallbackMask))
	{
	    lpItem->state &= ~stateMask;
	    lpItem->state |= (lpLVItem->state & stateMask);
	}
	if (lpLVItem->state & stateMask & ~infoPtr->uCallbackMask & LVIS_SELECTED)
	{
	    if (infoPtr->dwStyle & LVS_SINGLESEL) LISTVIEW_DeselectAllSkipItem(infoPtr, lpLVItem->iItem);
	    ranges_additem(infoPtr->selectionRanges, lpLVItem->iItem);
	}
	else if (stateMask & LVIS_SELECTED)
	{
	    ranges_delitem(infoPtr->selectionRanges, lpLVItem->iItem);
	}
	/* If we are asked to change focus, and we manage it, do it.
           It's important to have all new item data stored at this point,
           because changing existing focus could result in a redrawing operation,
           which in turn could ask for disp data, application should see all data
           for inserted item when processing LVN_GETDISPINFO.

           The way this works application will see nested item change notifications -
           changed item notifications interrupted by ones from item losing focus. */
	if (stateMask & ~infoPtr->uCallbackMask & LVIS_FOCUSED)
	{
	    if (lpLVItem->state & LVIS_FOCUSED)
	    {
		/* update selection mark */
		if (infoPtr->nFocusedItem == -1 && infoPtr->nSelectionMark == -1)
		    infoPtr->nSelectionMark = lpLVItem->iItem;

		if (infoPtr->nFocusedItem != -1)
		{
		    /* remove current focus */
		    item.mask  = LVIF_STATE;
		    item.state = 0;
		    item.stateMask = LVIS_FOCUSED;

		    /* recurse with redrawing an item */
		    LISTVIEW_SetItemState(infoPtr, infoPtr->nFocusedItem, &item);
		}

		infoPtr->nFocusedItem = lpLVItem->iItem;
	        LISTVIEW_EnsureVisible(infoPtr, lpLVItem->iItem, infoPtr->uView == LV_VIEW_LIST);
	    }
	    else if (infoPtr->nFocusedItem == lpLVItem->iItem)
	    {
	        infoPtr->nFocusedItem = -1;
	    }
	}
    }

    if (send_change_notification)
    {
        if (lpLVItem->mask & LVIF_PARAM) nmlv.lParam = lpLVItem->lParam;
        notify_listview(infoPtr, LVN_ITEMCHANGED, &nmlv);
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Helper for LISTVIEW_{Set,Insert}ItemT *only*: sets subitem attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : valid pointer to new subitem attributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 * [O] bChanged : will be set to TRUE if the item really changed
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL set_sub_item(const LISTVIEW_INFO *infoPtr, const LVITEMW *lpLVItem, BOOL isW, BOOL *bChanged)
{
    HDPA hdpaSubItems;
    SUBITEM_INFO *lpSubItem;

    /* we do not support subitems for virtual listviews */
    if (infoPtr->dwStyle & LVS_OWNERDATA) return FALSE;
    
    /* set subitem only if column is present */
    if (lpLVItem->iSubItem >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return FALSE;
   
    /* First do some sanity checks */
    /* The LVIF_STATE flag is valid for subitems, but does not appear to be
       particularly useful. We currently do not actually do anything with
       the flag on subitems.
    */
    if (lpLVItem->mask & ~(LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_DI_SETITEM)) return FALSE;
    if (!(lpLVItem->mask & (LVIF_TEXT | LVIF_IMAGE | LVIF_STATE))) return TRUE;

    /* get the subitem structure, and create it if not there */
    hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    assert (hdpaSubItems);
    
    lpSubItem = LISTVIEW_GetSubItemPtr(hdpaSubItems, lpLVItem->iSubItem);
    if (!lpSubItem)
    {
	SUBITEM_INFO *tmpSubItem;
	INT i;

	lpSubItem = Alloc(sizeof(*lpSubItem));
	if (!lpSubItem) return FALSE;
	/* we could binary search here, if need be...*/
  	for (i = 1; i < DPA_GetPtrCount(hdpaSubItems); i++)
  	{
            tmpSubItem = DPA_GetPtr(hdpaSubItems, i);
	    if (tmpSubItem->iSubItem > lpLVItem->iSubItem) break;
  	}
	if (DPA_InsertPtr(hdpaSubItems, i, lpSubItem) == -1)
	{
	    Free(lpSubItem);
	    return FALSE;
	}
        lpSubItem->iSubItem = lpLVItem->iSubItem;
        lpSubItem->hdr.iImage = I_IMAGECALLBACK;
	*bChanged = TRUE;
    }
    
    if ((lpLVItem->mask & LVIF_IMAGE) && (lpSubItem->hdr.iImage != lpLVItem->iImage))
    {
        lpSubItem->hdr.iImage = lpLVItem->iImage;
        *bChanged = TRUE;
    }

    if ((lpLVItem->mask & LVIF_TEXT) && textcmpWT(lpSubItem->hdr.pszText, lpLVItem->pszText, isW))
    {
        textsetptrT(&lpSubItem->hdr.pszText, lpLVItem->pszText, isW);
        *bChanged = TRUE;
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets item attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : new item attributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemT(LISTVIEW_INFO *infoPtr, LVITEMW *lpLVItem, BOOL isW)
{
    HWND hwndSelf = infoPtr->hwndSelf;
    LPWSTR pszText = NULL;
    BOOL bResult, bChanged = FALSE;
    RECT oldItemArea;

    TRACE("(lpLVItem=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (!lpLVItem || lpLVItem->iItem < 0 || lpLVItem->iItem >= infoPtr->nItemCount)
	return FALSE;

    /* Store old item area */
    LISTVIEW_GetItemBox(infoPtr, lpLVItem->iItem, &oldItemArea);

    /* For efficiency, we transform the lpLVItem->pszText to Unicode here */
    if ((lpLVItem->mask & LVIF_TEXT) && is_text(lpLVItem->pszText))
    {
	pszText = lpLVItem->pszText;
	lpLVItem->pszText = textdupTtoW(lpLVItem->pszText, isW);
    }

    /* actually set the fields */
    if (!is_assignable_item(lpLVItem, infoPtr->dwStyle)) return FALSE;

    if (lpLVItem->iSubItem)
	bResult = set_sub_item(infoPtr, lpLVItem, TRUE, &bChanged);
    else
	bResult = set_main_item(infoPtr, lpLVItem, FALSE, TRUE, &bChanged);
    if (!IsWindow(hwndSelf))
	return FALSE;

    /* redraw item, if necessary */
    if (bChanged && !infoPtr->bIsDrawing)
    {
	/* this little optimization eliminates some nasty flicker */
	if ( infoPtr->uView == LV_VIEW_DETAILS && !(infoPtr->dwStyle & LVS_OWNERDRAWFIXED) &&
	     !(infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) &&
             lpLVItem->iSubItem > 0 && lpLVItem->iSubItem <= DPA_GetPtrCount(infoPtr->hdpaColumns) )
	    LISTVIEW_InvalidateSubItem(infoPtr, lpLVItem->iItem, lpLVItem->iSubItem);
	else
        {
            LISTVIEW_InvalidateRect(infoPtr, &oldItemArea);
	    LISTVIEW_InvalidateItem(infoPtr, lpLVItem->iItem);
        }
    }
    /* restore text */
    if (pszText)
    {
	textfreeT(lpLVItem->pszText, isW);
	lpLVItem->pszText = pszText;
    }

    return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the index of the item at coordinate (0, 0) of the client area.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * item index
 */
static INT LISTVIEW_GetTopIndex(const LISTVIEW_INFO *infoPtr)
{
    INT nItem = 0;
    SCROLLINFO scrollInfo;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;

    if (infoPtr->uView == LV_VIEW_LIST)
    {
	if (GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
	    nItem = scrollInfo.nPos * LISTVIEW_GetCountPerColumn(infoPtr);
    }
    else if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	if (GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	    nItem = scrollInfo.nPos;
    } 
    else
    {
	if (GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	    nItem = LISTVIEW_GetCountPerRow(infoPtr) * (scrollInfo.nPos / infoPtr->nItemHeight);
    }

    TRACE("nItem=%d\n", nItem);
    
    return nItem;
}

static void LISTVIEW_DrawBackgroundBitmap(const LISTVIEW_INFO *infoPtr, HDC hdc, const RECT *lprcBox)
{
    HDC mem_hdc;

    if (!infoPtr->hBkBitmap)
        return;

    TRACE("(hdc=%p, lprcBox=%s, hBkBitmap=%p)\n", hdc, wine_dbgstr_rect(lprcBox), infoPtr->hBkBitmap);

    mem_hdc = CreateCompatibleDC(hdc);
    SelectObject(mem_hdc, infoPtr->hBkBitmap);
    BitBlt(hdc, lprcBox->left, lprcBox->top, lprcBox->right - lprcBox->left,
           lprcBox->bottom - lprcBox->top, mem_hdc, lprcBox->left, lprcBox->top, SRCCOPY);
    DeleteDC(mem_hdc);
}

/***
 * DESCRIPTION:
 * Erases the background of the given rectangle
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] lprcBox : clipping rectangle
 *
 * RETURN:
 *   Success: TRUE
 *   Failure: FALSE
 */
static BOOL LISTVIEW_FillBkgnd(const LISTVIEW_INFO *infoPtr, HDC hdc, const RECT *lprcBox)
{
    if (infoPtr->hBkBrush)
    {
        TRACE("(hdc=%p, lprcBox=%s, hBkBrush=%p)\n", hdc, wine_dbgstr_rect(lprcBox), infoPtr->hBkBrush);

        FillRect(hdc, lprcBox, infoPtr->hBkBrush);
    }

    LISTVIEW_DrawBackgroundBitmap(infoPtr, hdc, lprcBox);
    return TRUE;
}

/* Draw main item or subitem */
static void LISTVIEW_DrawItemPart(LISTVIEW_INFO *infoPtr, LVITEMW *item, const NMLVCUSTOMDRAW *nmlvcd, const POINT *pos)
{
    RECT rcSelect, rcLabel, rcBox, rcStateIcon, rcIcon;
    const RECT *background;
    HIMAGELIST himl;
    UINT format;
    RECT *focus;

    /* now check if we need to update the focus rectangle */
    focus = infoPtr->bFocus && (item->state & LVIS_FOCUSED) ? &infoPtr->rcFocus : 0;
    if (!focus) item->state &= ~LVIS_FOCUSED;

    LISTVIEW_GetItemMetrics(infoPtr, item, &rcBox, &rcSelect, &rcIcon, &rcStateIcon, &rcLabel);
    OffsetRect(&rcBox, pos->x, pos->y);
    OffsetRect(&rcSelect, pos->x, pos->y);
    OffsetRect(&rcIcon, pos->x, pos->y);
    OffsetRect(&rcStateIcon, pos->x, pos->y);
    OffsetRect(&rcLabel, pos->x, pos->y);
    TRACE("%d: box=%s, select=%s, icon=%s. label=%s\n", item->iSubItem,
        wine_dbgstr_rect(&rcBox), wine_dbgstr_rect(&rcSelect),
        wine_dbgstr_rect(&rcIcon), wine_dbgstr_rect(&rcLabel));

    /* FIXME: temporary hack */
    rcSelect.left = rcLabel.left;

    if (infoPtr->uView == LV_VIEW_DETAILS && item->iSubItem == 0)
    {
        if (!(infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT))
            OffsetRect(&rcSelect, LISTVIEW_GetColumnInfo(infoPtr, 0)->rcHeader.left, 0);
        OffsetRect(&rcIcon, LISTVIEW_GetColumnInfo(infoPtr, 0)->rcHeader.left, 0);
        OffsetRect(&rcStateIcon, LISTVIEW_GetColumnInfo(infoPtr, 0)->rcHeader.left, 0);
        OffsetRect(&rcLabel, LISTVIEW_GetColumnInfo(infoPtr, 0)->rcHeader.left, 0);
    }

    /* in icon mode, the label rect is really what we want to draw the
     * background for */
    /* in detail mode, we want to paint background for label rect when
     * item is not selected or listview has full row select; otherwise paint
     * background for text only */
    if ( infoPtr->uView == LV_VIEW_ICON ||
        (infoPtr->uView == LV_VIEW_DETAILS && (!(item->state & LVIS_SELECTED) ||
        (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT))))
        background = &rcLabel;
    else
        background = &rcSelect;

    if (nmlvcd->clrTextBk != CLR_NONE)
        ExtTextOutW(nmlvcd->nmcd.hdc, background->left, background->top, ETO_OPAQUE, background, NULL, 0, NULL);

    if (item->state & LVIS_FOCUSED)
    {
        if (infoPtr->uView == LV_VIEW_DETAILS)
        {
            if (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT)
            {
                /* we have to update left focus bound too if item isn't in leftmost column
	           and reduce right box bound */
                if (DPA_GetPtrCount(infoPtr->hdpaColumns) > 0)
                {
                    INT leftmost;

                    if ((leftmost = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX, 0, 0)))
                    {
                        INT Originx = pos->x - LISTVIEW_GetColumnInfo(infoPtr, leftmost)->rcHeader.left;
                        INT rightmost = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX,
                            DPA_GetPtrCount(infoPtr->hdpaColumns) - 1, 0);

                        rcBox.right   = LISTVIEW_GetColumnInfo(infoPtr, rightmost)->rcHeader.right + Originx;
                        rcSelect.left = LISTVIEW_GetColumnInfo(infoPtr, leftmost)->rcHeader.left + Originx;
                    }
                }
                rcSelect.right = rcBox.right;
            }
            infoPtr->rcFocus = rcSelect;
        }
        else
            infoPtr->rcFocus = rcLabel;
    }

    /* state icons */
    if (infoPtr->himlState && STATEIMAGEINDEX(item->state) && (item->iSubItem == 0))
    {
        UINT stateimage = STATEIMAGEINDEX(item->state);
        if (stateimage)
	{
	     TRACE("stateimage=%d\n", stateimage);
	     ImageList_Draw(infoPtr->himlState, stateimage-1, nmlvcd->nmcd.hdc, rcStateIcon.left, rcStateIcon.top, ILD_NORMAL);
	}
    }

    /* item icons */
    himl = (infoPtr->uView == LV_VIEW_ICON ? infoPtr->himlNormal : infoPtr->himlSmall);
    if (himl && item->iImage >= 0 && !IsRectEmpty(&rcIcon))
    {
        UINT style;

        TRACE("iImage=%d\n", item->iImage);

        if (item->state & (LVIS_SELECTED | LVIS_CUT) && infoPtr->bFocus)
            style = ILD_SELECTED;
        else
            style = ILD_NORMAL;

        ImageList_DrawEx(himl, item->iImage, nmlvcd->nmcd.hdc, rcIcon.left, rcIcon.top,
                         rcIcon.right - rcIcon.left, rcIcon.bottom - rcIcon.top, infoPtr->clrBk,
                         item->state & LVIS_CUT ? RGB(255, 255, 255) : CLR_DEFAULT,
                         style | (item->state & LVIS_OVERLAYMASK));
    }

    /* Don't bother painting item being edited */
    if (infoPtr->hwndEdit && item->iItem == infoPtr->nEditLabelItem && item->iSubItem == 0) return;

    /* figure out the text drawing flags */
    format = (infoPtr->uView == LV_VIEW_ICON ? (focus ? LV_FL_DT_FLAGS : LV_ML_DT_FLAGS) : LV_SL_DT_FLAGS);
    if (infoPtr->uView == LV_VIEW_ICON)
	format = (focus ? LV_FL_DT_FLAGS : LV_ML_DT_FLAGS);
    else if (item->iSubItem)
    {
	switch (LISTVIEW_GetColumnInfo(infoPtr, item->iSubItem)->fmt & LVCFMT_JUSTIFYMASK)
	{
	case LVCFMT_RIGHT:  format |= DT_RIGHT;  break;
	case LVCFMT_CENTER: format |= DT_CENTER; break;
	default:            format |= DT_LEFT;
	}
    }
    if (!(format & (DT_RIGHT | DT_CENTER)))
    {
        if (himl && item->iImage >= 0 && !IsRectEmpty(&rcIcon)) rcLabel.left += IMAGE_PADDING;
        else rcLabel.left += LABEL_HOR_PADDING;
    }
    else if (format & DT_RIGHT) rcLabel.right -= LABEL_HOR_PADDING;

    /* for GRIDLINES reduce the bottom so the text formats correctly */
    if (infoPtr->uView == LV_VIEW_DETAILS && infoPtr->dwLvExStyle & LVS_EX_GRIDLINES)
        rcLabel.bottom--;

    DrawTextW(nmlvcd->nmcd.hdc, item->pszText, -1, &rcLabel, format);
}

/***
 * DESCRIPTION:
 * Draws an item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] nItem : item index
 * [I] nSubItem : subitem index
 * [I] pos : item position in client coordinates
 * [I] cdmode : custom draw mode
 *
 * RETURN:
 *   Success: TRUE
 *   Failure: FALSE
 */
static BOOL LISTVIEW_DrawItem(LISTVIEW_INFO *infoPtr, HDC hdc, INT nItem, ITERATOR *subitems, POINT pos, DWORD cdmode)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    static WCHAR callbackW[] = L"(callback)";
    DWORD cdsubitemmode = CDRF_DODEFAULT;
    RECT *focus, rcBox;
    NMLVCUSTOMDRAW nmlvcd;
    LVITEMW lvItem;

    TRACE("(hdc=%p, nItem=%d, subitems=%p, pos=%s)\n", hdc, nItem, subitems, wine_dbgstr_point(&pos));

    /* get information needed for drawing the item */
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    if (infoPtr->uView == LV_VIEW_DETAILS) lvItem.mask |= LVIF_INDENT;
    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK | LVIS_CUT | LVIS_OVERLAYMASK;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.state = 0;
    lvItem.lParam = 0;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    lvItem.pszText = szDispText;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) return FALSE;
    if (lvItem.pszText == LPSTR_TEXTCALLBACKW) lvItem.pszText = callbackW;
    TRACE("   lvItem=%s\n", debuglvitem_t(&lvItem, TRUE));

    /* now check if we need to update the focus rectangle */
    focus = infoPtr->bFocus && (lvItem.state & LVIS_FOCUSED) ? &infoPtr->rcFocus : 0;
    if (!focus) lvItem.state &= ~LVIS_FOCUSED;

    LISTVIEW_GetItemMetrics(infoPtr, &lvItem, &rcBox, NULL, NULL, NULL, NULL);
    OffsetRect(&rcBox, pos.x, pos.y);

    /* Full custom draw stage sequence looks like this:

       LV_VIEW_DETAILS:

       - CDDS_ITEMPREPAINT
       - CDDS_ITEMPREPAINT|CDDS_SUBITEM   | => sent n times, where n is number of subitems,
         CDDS_ITEMPOSTPAINT|CDDS_SUBITEM  |    including item itself
       - CDDS_ITEMPOSTPAINT

       other styles:

       - CDDS_ITEMPREPAINT
       - CDDS_ITEMPOSTPAINT
    */

    /* fill in the custom draw structure */
    customdraw_fill(&nmlvcd, infoPtr, hdc, &rcBox, &lvItem);
    if (cdmode & CDRF_NOTIFYITEMDRAW)
        cdsubitemmode = notify_customdraw(infoPtr, CDDS_ITEMPREPAINT, &nmlvcd);
    if (cdsubitemmode & CDRF_SKIPDEFAULT) goto postpaint;

    if (subitems)
    {
        while (iterator_next(subitems))
        {
            DWORD subitemstage = CDRF_DODEFAULT;

            /* We need to query for each subitem, item's data (subitem == 0) is already here at this point */
            if (subitems->nItem)
            {
                lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT;
                lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK | LVIS_CUT | LVIS_OVERLAYMASK;
                lvItem.iItem = nItem;
                lvItem.iSubItem = subitems->nItem;
                lvItem.state = 0;
                lvItem.lParam = 0;
                lvItem.cchTextMax = DISP_TEXT_SIZE;
                lvItem.pszText = szDispText;
                szDispText[0] = 0;
                if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) return FALSE;
                if (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT)
	            lvItem.state = LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED);
                if (lvItem.pszText == LPSTR_TEXTCALLBACKW) lvItem.pszText = callbackW;
                TRACE("   lvItem=%s\n", debuglvitem_t(&lvItem, TRUE));

                /* update custom draw data */
                LISTVIEW_GetItemMetrics(infoPtr, &lvItem, &nmlvcd.nmcd.rc, NULL, NULL, NULL, NULL);
                OffsetRect(&nmlvcd.nmcd.rc, pos.x, pos.y);
                nmlvcd.iSubItem = subitems->nItem;
            }

            if (cdsubitemmode & CDRF_NOTIFYSUBITEMDRAW)
                subitemstage = notify_customdraw(infoPtr, CDDS_SUBITEM | CDDS_ITEMPREPAINT, &nmlvcd);

            if (subitems->nItem == 0 || (cdmode & CDRF_NOTIFYITEMDRAW))
                prepaint_setup(infoPtr, hdc, &nmlvcd, FALSE);
            else if (!(infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT))
                prepaint_setup(infoPtr, hdc, &nmlvcd, TRUE);

            if (!(subitemstage & CDRF_SKIPDEFAULT))
                LISTVIEW_DrawItemPart(infoPtr, &lvItem, &nmlvcd, &pos);

            if (subitemstage & CDRF_NOTIFYPOSTPAINT)
                subitemstage = notify_customdraw(infoPtr, CDDS_SUBITEM | CDDS_ITEMPOSTPAINT, &nmlvcd);
        }
    }
    else
    {
        prepaint_setup(infoPtr, hdc, &nmlvcd, FALSE);
        LISTVIEW_DrawItemPart(infoPtr, &lvItem, &nmlvcd, &pos);
    }

postpaint:
    if (cdsubitemmode & CDRF_NOTIFYPOSTPAINT)
    {
        nmlvcd.iSubItem = 0;
        notify_customdraw(infoPtr, CDDS_ITEMPOSTPAINT, &nmlvcd);
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Draws listview items when in owner draw mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 *
 * RETURN:
 * None
 */
static void LISTVIEW_RefreshOwnerDraw(const LISTVIEW_INFO *infoPtr, ITERATOR *i, HDC hdc, DWORD cdmode)
{
    UINT uID = (UINT)GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    DWORD cditemmode = CDRF_DODEFAULT;
    NMLVCUSTOMDRAW nmlvcd;
    POINT Origin, Position;
    DRAWITEMSTRUCT dis;
    LVITEMW item;
    
    TRACE("()\n");

    ZeroMemory(&dis, sizeof(dis));
    
    /* Get scroll info once before loop */
    LISTVIEW_GetOrigin(infoPtr, &Origin);
    
    /* iterate through the invalidated rows */
    while(iterator_next(i))
    {
	item.iItem = i->nItem;
	item.iSubItem = 0;
	item.mask = LVIF_PARAM | LVIF_STATE;
	item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	if (!LISTVIEW_GetItemW(infoPtr, &item)) continue;
	   
	dis.CtlType = ODT_LISTVIEW;
	dis.CtlID = uID;
	dis.itemID = item.iItem;
	dis.itemAction = ODA_DRAWENTIRE;
	dis.itemState = 0;
	if (item.state & LVIS_SELECTED) dis.itemState |= ODS_SELECTED;
	if (infoPtr->bFocus && (item.state & LVIS_FOCUSED)) dis.itemState |= ODS_FOCUS;
	dis.hwndItem = infoPtr->hwndSelf;
	dis.hDC = hdc;
	LISTVIEW_GetItemOrigin(infoPtr, dis.itemID, &Position);
	dis.rcItem.left = Position.x + Origin.x;
	dis.rcItem.right = dis.rcItem.left + infoPtr->nItemWidth;
	dis.rcItem.top = Position.y + Origin.y;
	dis.rcItem.bottom = dis.rcItem.top + infoPtr->nItemHeight;
	dis.itemData = item.lParam;

	TRACE("item=%s, rcItem=%s\n", debuglvitem_t(&item, TRUE), wine_dbgstr_rect(&dis.rcItem));

    /*
     * Even if we do not send the CDRF_NOTIFYITEMDRAW we need to fill the nmlvcd
     * structure for the rest. of the paint cycle
     */
	customdraw_fill(&nmlvcd, infoPtr, hdc, &dis.rcItem, &item);
	if (cdmode & CDRF_NOTIFYITEMDRAW)
            cditemmode = notify_customdraw(infoPtr, CDDS_PREPAINT, &nmlvcd);
    
	if (!(cditemmode & CDRF_SKIPDEFAULT))
	{
            prepaint_setup (infoPtr, hdc, &nmlvcd, FALSE);
	    SendMessageW(infoPtr->hwndNotify, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
	}

    	if (cditemmode & CDRF_NOTIFYPOSTPAINT)
            notify_postpaint(infoPtr, &nmlvcd);
    }
}

/***
 * DESCRIPTION:
 * Draws listview items when in report display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] cdmode : custom draw mode
 *
 * RETURN:
 * None
 */
static void LISTVIEW_RefreshReport(LISTVIEW_INFO *infoPtr, ITERATOR *i, HDC hdc, DWORD cdmode)
{
    INT rgntype;
    RECT rcClip, rcItem;
    POINT Origin;
    RANGES colRanges;
    INT col;
    ITERATOR j;

    TRACE("()\n");

    /* figure out what to draw */
    rgntype = GetClipBox(hdc, &rcClip);
    if (rgntype == NULLREGION) return;
    
    /* Get scroll info once before loop */
    LISTVIEW_GetOrigin(infoPtr, &Origin);

    colRanges = ranges_create(DPA_GetPtrCount(infoPtr->hdpaColumns));

    /* narrow down the columns we need to paint */
    for(col = 0; col < DPA_GetPtrCount(infoPtr->hdpaColumns); col++)
    {
	INT index = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX, col, 0);

	LISTVIEW_GetHeaderRect(infoPtr, index, &rcItem);
	if ((rcItem.right + Origin.x >= rcClip.left) && (rcItem.left + Origin.x < rcClip.right))
	    ranges_additem(colRanges, index);
    }
    iterator_rangesitems(&j, colRanges);

    /* in full row select, we _have_ to draw the main item */
    if (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT)
	j.nSpecial = 0;

    /* iterate through the invalidated rows */
    while(iterator_next(i))
    {
        RANGES subitems;
        POINT Position;
        ITERATOR k;

        SelectObject(hdc, infoPtr->hFont);
	LISTVIEW_GetItemOrigin(infoPtr, i->nItem, &Position);
        Position.x = Origin.x;
	Position.y += Origin.y;

        subitems = ranges_create(DPA_GetPtrCount(infoPtr->hdpaColumns));

	/* iterate through the invalidated columns */
	while(iterator_next(&j))
	{
	    LISTVIEW_GetHeaderRect(infoPtr, j.nItem, &rcItem);

	    if (rgntype == COMPLEXREGION && !((infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) && j.nItem == 0))
	    {
		rcItem.top = 0;
	        rcItem.bottom = infoPtr->nItemHeight;
		OffsetRect(&rcItem, Origin.x, Position.y);
		if (!RectVisible(hdc, &rcItem)) continue;
	    }

            ranges_additem(subitems, j.nItem);
	}

        iterator_rangesitems(&k, subitems);
        LISTVIEW_DrawItem(infoPtr, hdc, i->nItem, &k, Position, cdmode);
        iterator_destroy(&k);
    }
    iterator_destroy(&j);
}

/***
 * DESCRIPTION:
 * Draws the gridlines if necessary when in report display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 *
 * RETURN:
 * None
 */
static void LISTVIEW_RefreshReportGrid(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    INT rgntype;
    INT y, itemheight;
    INT col, index;
    HPEN hPen, hOldPen;
    RECT rcClip, rcItem = {0};
    POINT Origin;
    RANGES colRanges;
    ITERATOR j;
    BOOL rmost = FALSE;

    TRACE("()\n");

    /* figure out what to draw */
    rgntype = GetClipBox(hdc, &rcClip);
    if (rgntype == NULLREGION) return;

    /* Get scroll info once before loop */
    LISTVIEW_GetOrigin(infoPtr, &Origin);

    colRanges = ranges_create(DPA_GetPtrCount(infoPtr->hdpaColumns));

    /* narrow down the columns we need to paint */
    for(col = 0; col < DPA_GetPtrCount(infoPtr->hdpaColumns); col++)
    {
        index = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX, col, 0);

        LISTVIEW_GetHeaderRect(infoPtr, index, &rcItem);
        if ((rcItem.right + Origin.x >= rcClip.left) && (rcItem.left + Origin.x < rcClip.right))
            ranges_additem(colRanges, index);
    }

    /* is right most vertical line visible? */
    if (DPA_GetPtrCount(infoPtr->hdpaColumns) > 0)
    {
        index = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX, DPA_GetPtrCount(infoPtr->hdpaColumns) - 1, 0);
        LISTVIEW_GetHeaderRect(infoPtr, index, &rcItem);
        rmost = (rcItem.right + Origin.x < rcClip.right);
    }

    if ((hPen = CreatePen( PS_SOLID, 1, comctl32_color.clr3dFace )))
    {
        hOldPen = SelectObject ( hdc, hPen );

        /* draw the vertical lines for the columns */
        iterator_rangesitems(&j, colRanges);
        while(iterator_next(&j))
        {
            LISTVIEW_GetHeaderRect(infoPtr, j.nItem, &rcItem);
            if (rcItem.left == 0) continue; /* skip leftmost column */
            rcItem.left += Origin.x;
            rcItem.right += Origin.x;
            rcItem.top = infoPtr->rcList.top;
            rcItem.bottom = infoPtr->rcList.bottom;
            TRACE("vert col=%d, rcItem=%s\n", j.nItem, wine_dbgstr_rect(&rcItem));
            MoveToEx (hdc, rcItem.left, rcItem.top, NULL);
            LineTo (hdc, rcItem.left, rcItem.bottom);
        }
        iterator_destroy(&j);
        /* draw rightmost grid line if visible */
        if (rmost)
        {
            index = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX,
                                 DPA_GetPtrCount(infoPtr->hdpaColumns) - 1, 0);
            LISTVIEW_GetHeaderRect(infoPtr, index, &rcItem);

            rcItem.right += Origin.x;

            MoveToEx (hdc, rcItem.right, infoPtr->rcList.top, NULL);
            LineTo (hdc, rcItem.right, infoPtr->rcList.bottom);
        }

        /* draw the horizontal lines for the rows */
        itemheight =  LISTVIEW_CalculateItemHeight(infoPtr);
        rcItem.left   = infoPtr->rcList.left;
        rcItem.right  = infoPtr->rcList.right;
        for(y = Origin.y > 1 ? Origin.y - 1 : itemheight - 1 + Origin.y % itemheight; y<=infoPtr->rcList.bottom; y+=itemheight)
        {
            rcItem.bottom = rcItem.top = y;
            TRACE("horz rcItem=%s\n", wine_dbgstr_rect(&rcItem));
            MoveToEx (hdc, rcItem.left, rcItem.top, NULL);
            LineTo (hdc, rcItem.right, rcItem.top);
        }

        SelectObject( hdc, hOldPen );
        DeleteObject( hPen );
    }
    else
        ranges_destroy(colRanges);
}

/***
 * DESCRIPTION:
 * Draws listview items when in list display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] cdmode : custom draw mode
 *
 * RETURN:
 * None
 */
static void LISTVIEW_RefreshList(LISTVIEW_INFO *infoPtr, ITERATOR *i, HDC hdc, DWORD cdmode)
{
    POINT Origin, Position;

    /* Get scroll info once before loop */
    LISTVIEW_GetOrigin(infoPtr, &Origin);
    
    while(iterator_prev(i))
    {
        SelectObject(hdc, infoPtr->hFont);
	LISTVIEW_GetItemOrigin(infoPtr, i->nItem, &Position);
	Position.x += Origin.x;
	Position.y += Origin.y;

        LISTVIEW_DrawItem(infoPtr, hdc, i->nItem, NULL, Position, cdmode);
    }
}


/***
 * DESCRIPTION:
 * Draws listview items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] prcErase : rect to be erased before refresh (may be NULL)
 *
 * RETURN:
 * NoneX
 */
static void LISTVIEW_Refresh(LISTVIEW_INFO *infoPtr, HDC hdc, const RECT *prcErase)
{
    COLORREF oldTextColor = 0, oldBkColor = 0;
    NMLVCUSTOMDRAW nmlvcd;
    HFONT hOldFont = 0;
    DWORD cdmode;
    INT oldBkMode = 0;
    RECT rcClient;
    ITERATOR i;
    HDC hdcOrig = hdc;
    HBITMAP hbmp = NULL;
    RANGE range;

    LISTVIEW_DUMP(infoPtr);

    if (infoPtr->dwLvExStyle & LVS_EX_DOUBLEBUFFER) {
        TRACE("double buffering\n");

        hdc = CreateCompatibleDC(hdcOrig);
        if (!hdc) {
            ERR("Failed to create DC for backbuffer\n");
            return;
        }
        hbmp = CreateCompatibleBitmap(hdcOrig, infoPtr->rcList.right,
                                      infoPtr->rcList.bottom);
        if (!hbmp) {
            ERR("Failed to create bitmap for backbuffer\n");
            DeleteDC(hdc);
            return;
        }

        SelectObject(hdc, hbmp);
        SelectObject(hdc, infoPtr->hFont);

        if(GetClipBox(hdcOrig, &rcClient))
            IntersectClipRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
    } else {
        /* Save dc values we're gonna trash while drawing
         * FIXME: Should be done in LISTVIEW_DrawItem() */
        hOldFont = SelectObject(hdc, infoPtr->hFont);
        oldBkMode = GetBkMode(hdc);
        oldBkColor = GetBkColor(hdc);
        oldTextColor = GetTextColor(hdc);
    }

    infoPtr->bIsDrawing = TRUE;

    if (prcErase) {
        LISTVIEW_FillBkgnd(infoPtr, hdc, prcErase);
    } else if (infoPtr->dwLvExStyle & LVS_EX_DOUBLEBUFFER) {
        /* If no erasing was done (usually because RedrawWindow was called
         * with RDW_INVALIDATE only) we need to copy the old contents into
         * the backbuffer before continuing. */
        BitBlt(hdc, infoPtr->rcList.left, infoPtr->rcList.top,
               infoPtr->rcList.right - infoPtr->rcList.left,
               infoPtr->rcList.bottom - infoPtr->rcList.top,
               hdcOrig, infoPtr->rcList.left, infoPtr->rcList.top, SRCCOPY);
    }

    GetClientRect(infoPtr->hwndSelf, &rcClient);
    customdraw_fill(&nmlvcd, infoPtr, hdc, &rcClient, 0);
    cdmode = notify_customdraw(infoPtr, CDDS_PREPAINT, &nmlvcd);
    if (cdmode & CDRF_SKIPDEFAULT) goto enddraw;

    /* nothing to draw */
    if(infoPtr->nItemCount == 0) goto enddraw;

    /* figure out what we need to draw */
    iterator_visibleitems(&i, infoPtr, hdc);
    range = iterator_range(&i);

    /* send cache hint notification */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	NMLVCACHEHINT nmlv;
	
    	ZeroMemory(&nmlv, sizeof(NMLVCACHEHINT));
    	nmlv.iFrom = range.lower;
    	nmlv.iTo   = range.upper - 1;
    	notify_hdr(infoPtr, LVN_ODCACHEHINT, &nmlv.hdr);
    }

    if ((infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && (infoPtr->uView == LV_VIEW_DETAILS))
	LISTVIEW_RefreshOwnerDraw(infoPtr, &i, hdc, cdmode);
    else
    {
	if (infoPtr->uView == LV_VIEW_DETAILS)
            LISTVIEW_RefreshReport(infoPtr, &i, hdc, cdmode);
	else /* LV_VIEW_LIST, LV_VIEW_ICON or LV_VIEW_SMALLICON */
	    LISTVIEW_RefreshList(infoPtr, &i, hdc, cdmode);

	/* if we have a focus rect and it's visible, draw it */
	if (infoPtr->bFocus && range.lower <= infoPtr->nFocusedItem &&
                        (range.upper - 1) >= infoPtr->nFocusedItem)
	    LISTVIEW_DrawFocusRect(infoPtr, hdc);
    }
    iterator_destroy(&i);
    
enddraw:
    /* For LVS_EX_GRIDLINES go and draw lines */
    /*  This includes the case where there were *no* items */
    if ((infoPtr->uView == LV_VIEW_DETAILS) && infoPtr->dwLvExStyle & LVS_EX_GRIDLINES)
        LISTVIEW_RefreshReportGrid(infoPtr, hdc);

    /* Draw marquee rectangle if appropriate */
    if (infoPtr->bMarqueeSelect)
        DrawFocusRect(hdc, &infoPtr->marqueeDrawRect);

    if (cdmode & CDRF_NOTIFYPOSTPAINT)
	notify_postpaint(infoPtr, &nmlvcd);

    if(hbmp) {
        BitBlt(hdcOrig, infoPtr->rcList.left, infoPtr->rcList.top,
               infoPtr->rcList.right - infoPtr->rcList.left,
               infoPtr->rcList.bottom - infoPtr->rcList.top,
               hdc, infoPtr->rcList.left, infoPtr->rcList.top, SRCCOPY);

        DeleteObject(hbmp);
        DeleteDC(hdc);
    } else {
        SelectObject(hdc, hOldFont);
        SetBkMode(hdc, oldBkMode);
        SetBkColor(hdc, oldBkColor);
        SetTextColor(hdc, oldTextColor);
    }

    infoPtr->bIsDrawing = FALSE;
}


/***
 * DESCRIPTION:
 * Calculates the approximate width and height of a given number of items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItemCount : number of items
 * [I] wWidth : width
 * [I] wHeight : height
 *
 * RETURN:
 * Returns a DWORD. The width in the low word and the height in high word.
 */
static DWORD LISTVIEW_ApproximateViewRect(const LISTVIEW_INFO *infoPtr, INT nItemCount,
                                            WORD wWidth, WORD wHeight)
{
  DWORD dwViewRect = 0;

  if (nItemCount == -1)
    nItemCount = infoPtr->nItemCount;

  if (infoPtr->uView == LV_VIEW_LIST)
  {
    INT nItemCountPerColumn = 1;
    INT nColumnCount = 0;

    if (wHeight == 0xFFFF)
    {
      /* use current height */
      wHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
    }

    if (wHeight < infoPtr->nItemHeight)
      wHeight = infoPtr->nItemHeight;

    if (nItemCount > 0)
    {
      if (infoPtr->nItemHeight > 0)
      {
        nItemCountPerColumn = wHeight / infoPtr->nItemHeight;
        if (nItemCountPerColumn == 0)
          nItemCountPerColumn = 1;

        if (nItemCount % nItemCountPerColumn != 0)
          nColumnCount = nItemCount / nItemCountPerColumn;
        else
          nColumnCount = nItemCount / nItemCountPerColumn + 1;
      }
    }

    /* Microsoft padding magic */
    wHeight = nItemCountPerColumn * infoPtr->nItemHeight + 2;
    wWidth = nColumnCount * infoPtr->nItemWidth + 2;

    dwViewRect = MAKELONG(wWidth, wHeight);
  }
  else if (infoPtr->uView == LV_VIEW_DETAILS)
  {
    RECT rcBox;

    if (infoPtr->nItemCount > 0)
    {
      LISTVIEW_GetItemBox(infoPtr, 0, &rcBox);
      wWidth = rcBox.right - rcBox.left;
      wHeight = (rcBox.bottom - rcBox.top) * nItemCount;
    }
    else
    {
      /* use current height and width */
      if (wHeight == 0xffff)
          wHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
      if (wWidth == 0xffff)
          wWidth = infoPtr->rcList.right - infoPtr->rcList.left;
    }

    dwViewRect = MAKELONG(wWidth, wHeight);
  }
  else if (infoPtr->uView == LV_VIEW_ICON)
  {
    UINT rows,cols;
    UINT nItemWidth;
    UINT nItemHeight;

    nItemWidth = infoPtr->iconSpacing.cx;
    nItemHeight = infoPtr->iconSpacing.cy;

    if (wWidth == 0xffff)
      wWidth = infoPtr->rcList.right - infoPtr->rcList.left;

    if (wWidth < nItemWidth)
      wWidth = nItemWidth;

    cols = wWidth / nItemWidth;
    if (cols > nItemCount)
      cols = nItemCount;
    if (cols < 1)
        cols = 1;

    if (nItemCount)
    {
      rows = nItemCount / cols;
      if (nItemCount % cols)
        rows++;
    }
    else
      rows = 0;

    wHeight = (nItemHeight * rows)+2;
    wWidth = (nItemWidth * cols)+2;

    dwViewRect = MAKELONG(wWidth, wHeight);
  }
  else if (infoPtr->uView == LV_VIEW_SMALLICON)
    FIXME("uView == LV_VIEW_SMALLICON: not implemented\n");

  return dwViewRect;
}

/***
 * DESCRIPTION:
 * Cancel edit label with saving item text.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Always returns TRUE.
 */
static LRESULT LISTVIEW_CancelEditLabel(LISTVIEW_INFO *infoPtr)
{
    if (infoPtr->hwndEdit)
    {
        /* handle value will be lost after LISTVIEW_EndEditLabelT */
        HWND edit = infoPtr->hwndEdit;

        LISTVIEW_EndEditLabelT(infoPtr, TRUE, IsWindowUnicode(infoPtr->hwndEdit));
        SendMessageW(edit, WM_CLOSE, 0, 0);
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Create a drag image list for the specified item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] iItem   : index of item
 * [O] lppt    : Upper-left corner of the image
 *
 * RETURN:
 * Returns a handle to the image list if successful, NULL otherwise.
 */
static HIMAGELIST LISTVIEW_CreateDragImage(LISTVIEW_INFO *infoPtr, INT iItem, LPPOINT lppt)
{
    RECT rcItem;
    SIZE size;
    POINT pos;
    HDC hdc, hdcOrig;
    HBITMAP hbmp, hOldbmp;
    HFONT hOldFont;
    HIMAGELIST dragList = 0;
    TRACE("iItem=%d Count=%d\n", iItem, infoPtr->nItemCount);

    if (iItem < 0 || iItem >= infoPtr->nItemCount || !lppt)
        return 0;

    rcItem.left = LVIR_BOUNDS;
    if (!LISTVIEW_GetItemRect(infoPtr, iItem, &rcItem))
        return 0;

    lppt->x = rcItem.left;
    lppt->y = rcItem.top;

    size.cx = rcItem.right - rcItem.left;
    size.cy = rcItem.bottom - rcItem.top;

    hdcOrig = GetDC(infoPtr->hwndSelf);
    hdc = CreateCompatibleDC(hdcOrig);
    hbmp = CreateCompatibleBitmap(hdcOrig, size.cx, size.cy);
    hOldbmp = SelectObject(hdc, hbmp);
    hOldFont = SelectObject(hdc, infoPtr->hFont);

    SetRect(&rcItem, 0, 0, size.cx, size.cy);
    FillRect(hdc, &rcItem, infoPtr->hBkBrush);
    
    pos.x = pos.y = 0;
    if (LISTVIEW_DrawItem(infoPtr, hdc, iItem, NULL, pos, CDRF_DODEFAULT))
    {
        dragList = ImageList_Create(size.cx, size.cy, ILC_COLOR, 10, 10);
        SelectObject(hdc, hOldbmp);
        ImageList_Add(dragList, hbmp, 0);
    }
    else
        SelectObject(hdc, hOldbmp);

    SelectObject(hdc, hOldFont);
    DeleteObject(hbmp);
    DeleteDC(hdc);
    ReleaseDC(infoPtr->hwndSelf, hdcOrig);

    TRACE("ret=%p\n", dragList);

    return dragList;
}


/***
 * DESCRIPTION:
 * Removes all listview items and subitems.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_DeleteAllItems(LISTVIEW_INFO *infoPtr, BOOL destroy)
{
    HDPA hdpaSubItems = NULL;
    BOOL suppress = FALSE;
    ITEMHDR *hdrItem;
    ITEM_INFO *lpItem;
    ITEM_ID *lpID;
    INT i, j;

    TRACE("()\n");

    /* we do it directly, to avoid notifications */
    ranges_clear(infoPtr->selectionRanges);
    infoPtr->nSelectionMark = -1;
    infoPtr->nFocusedItem = -1;
    SetRectEmpty(&infoPtr->rcFocus);
    /* But we are supposed to leave nHotItem as is! */

    /* send LVN_DELETEALLITEMS notification */
    if (!(infoPtr->dwStyle & LVS_OWNERDATA) || !destroy)
    {
        NMLISTVIEW nmlv;

        memset(&nmlv, 0, sizeof(NMLISTVIEW));
        nmlv.iItem = -1;
        suppress = notify_listview(infoPtr, LVN_DELETEALLITEMS, &nmlv);
    }

    for (i = infoPtr->nItemCount - 1; i >= 0; i--)
    {
	if (!(infoPtr->dwStyle & LVS_OWNERDATA))
	{
	    /* send LVN_DELETEITEM notification, if not suppressed
	       and if it is not a virtual listview */
	    if (!suppress) notify_deleteitem(infoPtr, i);
	    hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, i);
	    lpItem = DPA_GetPtr(hdpaSubItems, 0);
	    /* free id struct */
	    j = DPA_GetPtrIndex(infoPtr->hdpaItemIds, lpItem->id);
	    lpID = DPA_GetPtr(infoPtr->hdpaItemIds, j);
	    DPA_DeletePtr(infoPtr->hdpaItemIds, j);
	    Free(lpID);
	    /* both item and subitem start with ITEMHDR header */
	    for (j = 0; j < DPA_GetPtrCount(hdpaSubItems); j++)
	    {
	        hdrItem = DPA_GetPtr(hdpaSubItems, j);
		if (is_text(hdrItem->pszText)) Free(hdrItem->pszText);
		Free(hdrItem);
	    }
	    DPA_Destroy(hdpaSubItems);
	    DPA_DeletePtr(infoPtr->hdpaItems, i);
	}
	DPA_DeletePtr(infoPtr->hdpaPosX, i);
	DPA_DeletePtr(infoPtr->hdpaPosY, i);
	infoPtr->nItemCount --;
    }
    
    if (!destroy)
    {
        LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);
        LISTVIEW_UpdateScroll(infoPtr);
    }
    LISTVIEW_InvalidateList(infoPtr);
    infoPtr->bNoItemMetrics = TRUE;
    
    return TRUE;
}

/***
 * DESCRIPTION:
 * Scrolls, and updates the columns, when a column is changing width.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nColumn : column to scroll
 * [I] dx : amount of scroll, in pixels
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_ScrollColumns(LISTVIEW_INFO *infoPtr, INT nColumn, INT dx)
{
    COLUMN_INFO *lpColumnInfo;
    RECT rcOld, rcCol;
    POINT ptOrigin;
    INT nCol;
    HDITEMW hdi;

    if (nColumn < 0 || DPA_GetPtrCount(infoPtr->hdpaColumns) < 1) return;
    lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, min(nColumn, DPA_GetPtrCount(infoPtr->hdpaColumns) - 1));
    rcCol = lpColumnInfo->rcHeader;
    if (nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns))
	rcCol.left = rcCol.right;

    /* adjust the other columns */
    hdi.mask = HDI_ORDER;
    if (SendMessageW(infoPtr->hwndHeader, HDM_GETITEMW, nColumn, (LPARAM)&hdi))
    {
	INT nOrder = hdi.iOrder;
	for (nCol = 0; nCol < DPA_GetPtrCount(infoPtr->hdpaColumns); nCol++)
	{
	    hdi.mask = HDI_ORDER;
	    SendMessageW(infoPtr->hwndHeader, HDM_GETITEMW, nCol, (LPARAM)&hdi);
	    if (hdi.iOrder >= nOrder) {
		lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, nCol);
		lpColumnInfo->rcHeader.left  += dx;
		lpColumnInfo->rcHeader.right += dx;
	    }
	}
    }

    /* do not update screen if not in report mode */
    if (!is_redrawing(infoPtr) || infoPtr->uView != LV_VIEW_DETAILS) return;
    
    /* Need to reset the item width when inserting a new column */
    infoPtr->nItemWidth += dx;

    LISTVIEW_UpdateScroll(infoPtr);
    LISTVIEW_GetOrigin(infoPtr, &ptOrigin);

    /* scroll to cover the deleted column, and invalidate for redraw */
    rcOld = infoPtr->rcList;
    rcOld.left = ptOrigin.x + rcCol.left + dx;
    ScrollWindowEx(infoPtr->hwndSelf, dx, 0, &rcOld, &rcOld, 0, 0, SW_ERASE | SW_INVALIDATE);
}

/***
 * DESCRIPTION:
 * Removes a column from the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nColumn : column index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_DeleteColumn(LISTVIEW_INFO *infoPtr, INT nColumn)
{
    RECT rcCol;
    
    TRACE("nColumn=%d\n", nColumn);

    if (nColumn < 0 || nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns))
        return FALSE;

    /* While the MSDN specifically says that column zero should not be deleted,
       what actually happens is that the column itself is deleted but no items or subitems
       are removed.
     */

    LISTVIEW_GetHeaderRect(infoPtr, nColumn, &rcCol);
    
    if (!SendMessageW(infoPtr->hwndHeader, HDM_DELETEITEM, nColumn, 0))
	return FALSE;

    Free(DPA_GetPtr(infoPtr->hdpaColumns, nColumn));
    DPA_DeletePtr(infoPtr->hdpaColumns, nColumn);
  
    if (!(infoPtr->dwStyle & LVS_OWNERDATA) && nColumn)
    {
	SUBITEM_INFO *lpSubItem, *lpDelItem;
	HDPA hdpaSubItems;
	INT nItem, nSubItem, i;
	
	for (nItem = 0; nItem < infoPtr->nItemCount; nItem++)
	{
            hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, nItem);
	    nSubItem = 0;
	    lpDelItem = 0;
	    for (i = 1; i < DPA_GetPtrCount(hdpaSubItems); i++)
	    {
                lpSubItem = DPA_GetPtr(hdpaSubItems, i);
		if (lpSubItem->iSubItem == nColumn)
		{
		    nSubItem = i;
		    lpDelItem = lpSubItem;
		}
		else if (lpSubItem->iSubItem > nColumn) 
		{
		    lpSubItem->iSubItem--;
		}
	    }

	    /* if we found our subitem, zap it */
	    if (nSubItem > 0)
	    {
		/* free string */
		if (is_text(lpDelItem->hdr.pszText))
		    Free(lpDelItem->hdr.pszText);

		/* free item */
		Free(lpDelItem);

		/* free dpa memory */
		DPA_DeletePtr(hdpaSubItems, nSubItem);
    	    }
	}
    }

    /* update the other column info */
    if(DPA_GetPtrCount(infoPtr->hdpaColumns) == 0)
        LISTVIEW_InvalidateList(infoPtr);
    else
        LISTVIEW_ScrollColumns(infoPtr, nColumn, -(rcCol.right - rcCol.left));
    LISTVIEW_UpdateItemSize(infoPtr);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Invalidates the listview after an item's insertion or deletion.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] dir : -1 if deleting, 1 if inserting
 *
 * RETURN:
 *   None
 */
static void LISTVIEW_ScrollOnInsert(LISTVIEW_INFO *infoPtr, INT nItem, INT dir)
{
    INT nPerCol, nItemCol, nItemRow;
    RECT rcScroll;
    POINT Origin;

    /* if we don't refresh, what's the point of scrolling? */
    if (!is_redrawing(infoPtr)) return;
    
    assert (abs(dir) == 1);

    /* arrange icons if autoarrange is on */
    if (is_autoarrange(infoPtr))
    {
	BOOL arrange = TRUE;
	if (dir < 0 && nItem >= infoPtr->nItemCount) arrange = FALSE;
	if (dir > 0 && nItem == infoPtr->nItemCount - 1) arrange = FALSE;
	if (arrange) LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);
    }

    /* scrollbars need updating */
    LISTVIEW_UpdateScroll(infoPtr);

    /* figure out the item's position */ 
    if (infoPtr->uView == LV_VIEW_DETAILS)
	nPerCol = infoPtr->nItemCount + 1;
    else if (infoPtr->uView == LV_VIEW_LIST)
	nPerCol = LISTVIEW_GetCountPerColumn(infoPtr);
    else /* LV_VIEW_ICON, or LV_VIEW_SMALLICON */
	return;
    
    nItemCol = nItem / nPerCol;
    nItemRow = nItem % nPerCol;
    LISTVIEW_GetOrigin(infoPtr, &Origin);

    /* move the items below up a slot */
    rcScroll.left = nItemCol * infoPtr->nItemWidth;
    rcScroll.top = nItemRow * infoPtr->nItemHeight;
    rcScroll.right = rcScroll.left + infoPtr->nItemWidth;
    rcScroll.bottom = nPerCol * infoPtr->nItemHeight;
    OffsetRect(&rcScroll, Origin.x, Origin.y);
    TRACE("rcScroll=%s, dx=%d\n", wine_dbgstr_rect(&rcScroll), dir * infoPtr->nItemHeight);
    if (IntersectRect(&rcScroll, &rcScroll, &infoPtr->rcList))
    {
	TRACE("Invalidating rcScroll=%s, rcList=%s\n", wine_dbgstr_rect(&rcScroll), wine_dbgstr_rect(&infoPtr->rcList));
	InvalidateRect(infoPtr->hwndSelf, &rcScroll, TRUE);
    }

    /* report has only that column, so we're done */
    if (infoPtr->uView == LV_VIEW_DETAILS) return;

    /* now for LISTs, we have to deal with the columns to the right */
    SetRect(&rcScroll, (nItemCol + 1) * infoPtr->nItemWidth, 0,
            (infoPtr->nItemCount / nPerCol + 1) * infoPtr->nItemWidth,
            nPerCol * infoPtr->nItemHeight);
    OffsetRect(&rcScroll, Origin.x, Origin.y);
    if (IntersectRect(&rcScroll, &rcScroll, &infoPtr->rcList))
	InvalidateRect(infoPtr->hwndSelf, &rcScroll, TRUE);
}

/***
 * DESCRIPTION:
 * Removes an item from the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_DeleteItem(LISTVIEW_INFO *infoPtr, INT nItem)
{
    LVITEMW item;
    const BOOL is_icon = (infoPtr->uView == LV_VIEW_SMALLICON || infoPtr->uView == LV_VIEW_ICON);
    INT focus = infoPtr->nFocusedItem;

    TRACE("(nItem=%d)\n", nItem);

    if (nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;
    
    /* remove selection, and focus */
    item.state = 0;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    LISTVIEW_SetItemState(infoPtr, nItem, &item);

    /* send LVN_DELETEITEM notification. */
    if (!notify_deleteitem(infoPtr, nItem)) return FALSE;

    /* we need to do this here, because we'll be deleting stuff */  
    if (is_icon)
	LISTVIEW_InvalidateItem(infoPtr, nItem);
    
    if (!(infoPtr->dwStyle & LVS_OWNERDATA))
    {
        HDPA hdpaSubItems;
	ITEMHDR *hdrItem;
	ITEM_INFO *lpItem;
	ITEM_ID *lpID;
	INT i;

	hdpaSubItems = DPA_DeletePtr(infoPtr->hdpaItems, nItem);
	lpItem = DPA_GetPtr(hdpaSubItems, 0);

	/* free id struct */
	i = DPA_GetPtrIndex(infoPtr->hdpaItemIds, lpItem->id);
	lpID = DPA_GetPtr(infoPtr->hdpaItemIds, i);
	DPA_DeletePtr(infoPtr->hdpaItemIds, i);
	Free(lpID);
	for (i = 0; i < DPA_GetPtrCount(hdpaSubItems); i++)
    	{
            hdrItem = DPA_GetPtr(hdpaSubItems, i);
	    if (is_text(hdrItem->pszText)) Free(hdrItem->pszText);
            Free(hdrItem);
        }
        DPA_Destroy(hdpaSubItems);
    }

    if (is_icon)
    {
	DPA_DeletePtr(infoPtr->hdpaPosX, nItem);
	DPA_DeletePtr(infoPtr->hdpaPosY, nItem);
    }

    infoPtr->nItemCount--;
    LISTVIEW_ShiftIndices(infoPtr, nItem, -1);
    LISTVIEW_ShiftFocus(infoPtr, focus, nItem, -1);

    /* now is the invalidation fun */
    if (!is_icon)
        LISTVIEW_ScrollOnInsert(infoPtr, nItem, -1);
    return TRUE;
}


/***
 * DESCRIPTION:
 * Callback implementation for editlabel control
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] storeText : store edit box text as item text
 * [I] isW : TRUE if psxText is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_EndEditLabelT(LISTVIEW_INFO *infoPtr, BOOL storeText, BOOL isW)
{
    HWND hwndSelf = infoPtr->hwndSelf;
    WCHAR szDispText[DISP_TEXT_SIZE] = { 0 };
    NMLVDISPINFOW dispInfo;
    INT editedItem = infoPtr->nEditLabelItem;
    BOOL same;
    WCHAR *pszText = NULL;
    BOOL res;

    if (storeText)
    {
        DWORD len = isW ? GetWindowTextLengthW(infoPtr->hwndEdit) : GetWindowTextLengthA(infoPtr->hwndEdit);

        if (len++)
        {
            if (!(pszText = Alloc(len * (isW ? sizeof(WCHAR) : sizeof(CHAR)))))
                return FALSE;

            if (isW)
                GetWindowTextW(infoPtr->hwndEdit, pszText, len);
            else
                GetWindowTextA(infoPtr->hwndEdit, (CHAR*)pszText, len);
        }
    }

    TRACE("(pszText=%s, isW=%d)\n", debugtext_t(pszText, isW), isW);

    ZeroMemory(&dispInfo, sizeof(dispInfo));
    dispInfo.item.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
    dispInfo.item.iItem = editedItem;
    dispInfo.item.iSubItem = 0;
    dispInfo.item.stateMask = ~0;
    dispInfo.item.pszText = szDispText;
    dispInfo.item.cchTextMax = DISP_TEXT_SIZE;
    if (!LISTVIEW_GetItemT(infoPtr, &dispInfo.item, isW))
    {
       res = FALSE;
       goto cleanup;
    }

    if (isW)
        same = (lstrcmpW(dispInfo.item.pszText, pszText) == 0);
    else
    {
        LPWSTR tmp = textdupTtoW(pszText, FALSE);
        same = (lstrcmpW(dispInfo.item.pszText, tmp) == 0);
        textfreeT(tmp, FALSE);
    }

    /* add the text from the edit in */
    dispInfo.item.mask |= LVIF_TEXT;
    dispInfo.item.pszText = same ? NULL : pszText;
    dispInfo.item.cchTextMax = textlenT(dispInfo.item.pszText, isW);

    infoPtr->notify_mask &= ~NOTIFY_MASK_END_LABEL_EDIT;

    /* Do we need to update the Item Text */
    res = notify_dispinfoT(infoPtr, LVN_ENDLABELEDITW, &dispInfo, isW);

    infoPtr->notify_mask |= NOTIFY_MASK_END_LABEL_EDIT;

    infoPtr->nEditLabelItem = -1;
    infoPtr->hwndEdit = 0;

    if (!res) goto cleanup;

    if (!IsWindow(hwndSelf))
    {
	res = FALSE;
	goto cleanup;
    }
    if (!pszText) return TRUE;
    if (same)
    {
        res = TRUE;
        goto cleanup;
    }

    if (!(infoPtr->dwStyle & LVS_OWNERDATA))
    {
        HDPA hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, editedItem);
        ITEM_INFO* lpItem = DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem && lpItem->hdr.pszText == LPSTR_TEXTCALLBACKW)
        {
            LISTVIEW_InvalidateItem(infoPtr, editedItem);
            res = TRUE;
            goto cleanup;
        }
    }

    ZeroMemory(&dispInfo, sizeof(dispInfo));
    dispInfo.item.mask = LVIF_TEXT;
    dispInfo.item.iItem = editedItem;
    dispInfo.item.iSubItem = 0;
    dispInfo.item.pszText = pszText;
    dispInfo.item.cchTextMax = textlenT(pszText, isW);
    res = LISTVIEW_SetItemT(infoPtr, &dispInfo.item, isW);

cleanup:
    Free(pszText);

    return res;
}

static LRESULT EditLblWndProcT(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL isW)
{
    LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongPtrW(GetParent(hwnd), 0);
    BOOL save = TRUE;

    TRACE("hwnd %p, uMsg %x, wParam %Ix, lParam %Ix, isW %d\n", hwnd, uMsg, wParam, lParam, isW);

    switch (uMsg)
    {
	case WM_GETDLGCODE:
	  return DLGC_WANTARROWS | DLGC_WANTALLKEYS;

	case WM_DESTROY:
	{
	    WNDPROC editProc = infoPtr->EditWndProc;
	    infoPtr->EditWndProc = 0;
	    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (DWORD_PTR)editProc);
	    return CallWindowProcT(editProc, hwnd, uMsg, wParam, lParam, isW);
	}

	case WM_KEYDOWN:
	    if (VK_ESCAPE == (INT)wParam)
	    {
		save = FALSE;
                break;
	    }
	    else if (VK_RETURN == (INT)wParam)
		break;

	default:
	    return CallWindowProcT(infoPtr->EditWndProc, hwnd, uMsg, wParam, lParam, isW);
    }

    /* kill the edit */
    if (infoPtr->hwndEdit)
	LISTVIEW_EndEditLabelT(infoPtr, save, isW);

    SendMessageW(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

static LRESULT CALLBACK EditLblWndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return EditLblWndProcT(hwnd, uMsg, wParam, lParam, TRUE);
}

static LRESULT CALLBACK EditLblWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return EditLblWndProcT(hwnd, uMsg, wParam, lParam, FALSE);
}

static HWND CreateEditLabelT(LISTVIEW_INFO *infoPtr, LPCWSTR text, BOOL isW)
{
    static const DWORD style = WS_CHILDWINDOW|WS_CLIPSIBLINGS|ES_LEFT|ES_AUTOHSCROLL|WS_BORDER|WS_VISIBLE;
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_HINSTANCE);
    HWND hedit;

    TRACE("(%p, text=%s, isW=%d)\n", infoPtr, debugtext_t(text, isW), isW);

    /* window will be resized and positioned after LVN_BEGINLABELEDIT */
    if (isW)
	hedit = CreateWindowW(WC_EDITW, text, style, 0, 0, 0, 0, infoPtr->hwndSelf, 0, hinst, 0);
    else
	hedit = CreateWindowA(WC_EDITA, (LPCSTR)text, style, 0, 0, 0, 0, infoPtr->hwndSelf, 0, hinst, 0);

    if (!hedit) return 0;

    infoPtr->EditWndProc = (WNDPROC)
	(isW ? SetWindowLongPtrW(hedit, GWLP_WNDPROC, (DWORD_PTR)EditLblWndProcW) :
               SetWindowLongPtrA(hedit, GWLP_WNDPROC, (DWORD_PTR)EditLblWndProcA) );

    SendMessageW(hedit, WM_SETFONT, (WPARAM)infoPtr->hFont, FALSE);
    SendMessageW(hedit, EM_SETLIMITTEXT, DISP_TEXT_SIZE-1, 0);

    return hedit;
}

static HWND LISTVIEW_EditLabelT(LISTVIEW_INFO *infoPtr, INT nItem, BOOL isW)
{
    WCHAR disptextW[DISP_TEXT_SIZE] = { 0 };
    HWND hwndSelf = infoPtr->hwndSelf;
    NMLVDISPINFOW dispInfo;
    HFONT hOldFont = NULL;
    TEXTMETRICW tm;
    RECT rect;
    SIZE sz;
    HDC hdc;

    TRACE("(nItem=%d, isW=%d)\n", nItem, isW);

    if (~infoPtr->dwStyle & LVS_EDITLABELS) return 0;

    /* remove existing edit box */
    if (infoPtr->hwndEdit)
    {
        SetFocus(infoPtr->hwndSelf);
        infoPtr->hwndEdit = 0;
    }

    if (nItem < 0 || nItem >= infoPtr->nItemCount) return 0;

    infoPtr->nEditLabelItem = nItem;

    LISTVIEW_SetSelection(infoPtr, nItem);
    LISTVIEW_SetItemFocus(infoPtr, nItem);
    LISTVIEW_InvalidateItem(infoPtr, nItem);

    rect.left = LVIR_LABEL;
    if (!LISTVIEW_GetItemRect(infoPtr, nItem, &rect)) return 0;
    
    ZeroMemory(&dispInfo, sizeof(dispInfo));
    dispInfo.item.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
    dispInfo.item.iItem = nItem;
    dispInfo.item.iSubItem = 0;
    dispInfo.item.stateMask = ~0;
    dispInfo.item.pszText = disptextW;
    dispInfo.item.cchTextMax = DISP_TEXT_SIZE;
    if (!LISTVIEW_GetItemT(infoPtr, &dispInfo.item, isW)) return 0;

    infoPtr->hwndEdit = CreateEditLabelT(infoPtr, dispInfo.item.pszText, isW);
    if (!infoPtr->hwndEdit) return 0;
    
    if (notify_dispinfoT(infoPtr, LVN_BEGINLABELEDITW, &dispInfo, isW))
    {
	if (!IsWindow(hwndSelf))
	    return 0;
	SendMessageW(infoPtr->hwndEdit, WM_CLOSE, 0, 0);
	infoPtr->hwndEdit = 0;
	return 0;
    }

    TRACE("disp text=%s\n", debugtext_t(dispInfo.item.pszText, isW));

    /* position and display edit box */
    hdc = GetDC(infoPtr->hwndSelf);

    /* select the font to get appropriate metric dimensions */
    if (infoPtr->hFont)
        hOldFont = SelectObject(hdc, infoPtr->hFont);

    /* use real edit box content, it could be altered during LVN_BEGINLABELEDIT notification */
    GetWindowTextW(infoPtr->hwndEdit, disptextW, DISP_TEXT_SIZE);
    TRACE("edit box text=%s\n", debugstr_w(disptextW));

    /* get string length in pixels */
    GetTextExtentPoint32W(hdc, disptextW, lstrlenW(disptextW), &sz);

    /* add extra spacing for the next character */
    GetTextMetricsW(hdc, &tm);
    sz.cx += tm.tmMaxCharWidth * 2;

    if (infoPtr->hFont)
        SelectObject(hdc, hOldFont);

    ReleaseDC(infoPtr->hwndSelf, hdc);

    sz.cy = rect.bottom - rect.top + 2;
    rect.left -= 2;
    rect.top  -= 1;
    TRACE("moving edit (%ld,%ld)-(%ld,%ld)\n", rect.left, rect.top, sz.cx, sz.cy);
    MoveWindow(infoPtr->hwndEdit, rect.left, rect.top, sz.cx, sz.cy, FALSE);
    ShowWindow(infoPtr->hwndEdit, SW_NORMAL);
    SetFocus(infoPtr->hwndEdit);
    SendMessageW(infoPtr->hwndEdit, EM_SETSEL, 0, -1);
    return infoPtr->hwndEdit;
}


/***
 * DESCRIPTION:
 * Ensures the specified item is visible, scrolling into view if necessary.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] bPartial : partially or entirely visible
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_EnsureVisible(LISTVIEW_INFO *infoPtr, INT nItem, BOOL bPartial)
{
    INT nScrollPosHeight = 0;
    INT nScrollPosWidth = 0;
    INT nHorzAdjust = 0;
    INT nVertAdjust = 0;
    INT nHorzDiff = 0;
    INT nVertDiff = 0;
    RECT rcItem, rcTemp;

    rcItem.left = LVIR_BOUNDS;
    if (!LISTVIEW_GetItemRect(infoPtr, nItem, &rcItem)) return FALSE;

    if (bPartial && IntersectRect(&rcTemp, &infoPtr->rcList, &rcItem)) return TRUE;
    
    if (rcItem.left < infoPtr->rcList.left || rcItem.right > infoPtr->rcList.right)
    {
        /* scroll left/right, but in LV_VIEW_DETAILS mode */
        if (infoPtr->uView == LV_VIEW_LIST)
            nScrollPosWidth = infoPtr->nItemWidth;
        else if ((infoPtr->uView == LV_VIEW_SMALLICON) || (infoPtr->uView == LV_VIEW_ICON))
            nScrollPosWidth = 1;

	if (rcItem.left < infoPtr->rcList.left)
	{
	    nHorzAdjust = -1;
	    if (infoPtr->uView != LV_VIEW_DETAILS) nHorzDiff = rcItem.left - infoPtr->rcList.left;
	}
	else
	{
	    nHorzAdjust = 1;
	    if (infoPtr->uView != LV_VIEW_DETAILS) nHorzDiff = rcItem.right - infoPtr->rcList.right;
	}
    }

    if (rcItem.top < infoPtr->rcList.top || rcItem.bottom > infoPtr->rcList.bottom)
    {
	/* scroll up/down, but not in LVS_LIST mode */
        if (infoPtr->uView == LV_VIEW_DETAILS)
            nScrollPosHeight = infoPtr->nItemHeight;
        else if ((infoPtr->uView == LV_VIEW_ICON) || (infoPtr->uView == LV_VIEW_SMALLICON))
            nScrollPosHeight = 1;

	if (rcItem.top < infoPtr->rcList.top)
	{
	    nVertAdjust = -1;
	    if (infoPtr->uView != LV_VIEW_LIST) nVertDiff = rcItem.top - infoPtr->rcList.top;
	}
	else
	{
	    nVertAdjust = 1;
	    if (infoPtr->uView != LV_VIEW_LIST) nVertDiff = rcItem.bottom - infoPtr->rcList.bottom;
	}
    }

    if (!nScrollPosWidth && !nScrollPosHeight) return TRUE;

    if (nScrollPosWidth)
    {
	INT diff = nHorzDiff / nScrollPosWidth;
	if (nHorzDiff % nScrollPosWidth) diff += nHorzAdjust;
	LISTVIEW_HScroll(infoPtr, SB_INTERNAL, diff);
    }

    if (nScrollPosHeight)
    {
	INT diff = nVertDiff / nScrollPosHeight;
	if (nVertDiff % nScrollPosHeight) diff += nVertAdjust;
	LISTVIEW_VScroll(infoPtr, SB_INTERNAL, diff);
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Searches for an item with specific characteristics.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nStart : base item index
 * [I] lpFindInfo : item information to look for
 *
 * RETURN:
 *   SUCCESS : index of item
 *   FAILURE : -1
 */
static INT LISTVIEW_FindItemW(const LISTVIEW_INFO *infoPtr, INT nStart,
                              const LVFINDINFOW *lpFindInfo)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    BOOL bWrap = FALSE, bNearest = FALSE;
    INT nItem = nStart + 1, nLast = infoPtr->nItemCount, nNearestItem = -1;
    ULONG xdist, ydist, dist, mindist = 0x7fffffff;
    POINT Position, Destination;
    int search_len = 0;
    LVITEMW lvItem;

    /* Search in virtual listviews should be done by application, not by
       listview control, so we just send LVN_ODFINDITEMW and return the result */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
        NMLVFINDITEMW nmlv;

        nmlv.iStart = nStart;
        nmlv.lvfi = *lpFindInfo;
        return notify_hdr(infoPtr, LVN_ODFINDITEMW, (LPNMHDR)&nmlv.hdr);
    }

    if (!lpFindInfo || nItem < 0) return -1;

    lvItem.mask = 0;
    if (lpFindInfo->flags & (LVFI_STRING | LVFI_PARTIAL | LVFI_SUBSTRING))
    {
        lvItem.mask |= LVIF_TEXT;
        lvItem.pszText = szDispText;
        lvItem.cchTextMax = DISP_TEXT_SIZE;
    }

    if (lpFindInfo->flags & LVFI_WRAP)
        bWrap = TRUE;

    if ((lpFindInfo->flags & LVFI_NEARESTXY) && 
	(infoPtr->uView == LV_VIEW_ICON || infoPtr->uView == LV_VIEW_SMALLICON))
    {
	POINT Origin;
	RECT rcArea;
	
        LISTVIEW_GetOrigin(infoPtr, &Origin);
	Destination.x = lpFindInfo->pt.x - Origin.x;
	Destination.y = lpFindInfo->pt.y - Origin.y;
	switch(lpFindInfo->vkDirection)
	{
	case VK_DOWN:  Destination.y += infoPtr->nItemHeight; break;
	case VK_UP:    Destination.y -= infoPtr->nItemHeight; break;
	case VK_RIGHT: Destination.x += infoPtr->nItemWidth; break;
	case VK_LEFT:  Destination.x -= infoPtr->nItemWidth; break;
	case VK_HOME:  Destination.x = Destination.y = 0; break;
	case VK_NEXT:  Destination.y += infoPtr->rcList.bottom - infoPtr->rcList.top; break;
	case VK_PRIOR: Destination.y -= infoPtr->rcList.bottom - infoPtr->rcList.top; break;
	case VK_END:
	    LISTVIEW_GetAreaRect(infoPtr, &rcArea);
	    Destination.x = rcArea.right; 
	    Destination.y = rcArea.bottom; 
	    break;
	default: ERR("Unknown vkDirection=%d\n", lpFindInfo->vkDirection);
	}
	bNearest = TRUE;
    }
    else Destination.x = Destination.y = 0;

    /* if LVFI_PARAM is specified, all other flags are ignored */
    if (lpFindInfo->flags & LVFI_PARAM)
    {
        lvItem.mask |= LVIF_PARAM;
	bNearest = FALSE;
	lvItem.mask &= ~LVIF_TEXT;
    }

    nItem = bNearest ? -1 : nStart + 1;

    if (lpFindInfo->flags & (LVFI_PARTIAL | LVFI_SUBSTRING))
        search_len = lstrlenW(lpFindInfo->psz);

again:
    for (; nItem < nLast; nItem++)
    {
        lvItem.iItem = nItem;
        lvItem.iSubItem = 0;
        lvItem.pszText = szDispText;
        if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) continue;

	if (lvItem.mask & LVIF_PARAM)
        {
            if (lpFindInfo->lParam == lvItem.lParam)
                return nItem;
            else
                continue;
        }

        if (lvItem.mask & LVIF_TEXT)
	{
            if (lpFindInfo->flags & (LVFI_PARTIAL | LVFI_SUBSTRING))
            {
                if (StrCmpNIW(lvItem.pszText, lpFindInfo->psz, search_len)) continue;
            }
            else
            {
                if (StrCmpIW(lvItem.pszText, lpFindInfo->psz)) continue;
            }
	}

        if (!bNearest) return nItem;

	/* This is very inefficient. To do a good job here,
	 * we need a sorted array of (x,y) item positions */
	LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);

	/* compute the distance^2 to the destination */
	xdist = Destination.x - Position.x;
	ydist = Destination.y - Position.y;
	dist = xdist * xdist + ydist * ydist;

	/* remember the distance, and item if it's closer */
	if (dist < mindist)
	{
	    mindist = dist;
	    nNearestItem = nItem;
	}
    }

    if (bWrap)
    {
        nItem = 0;
        nLast = min(nStart + 1, infoPtr->nItemCount);
        bWrap = FALSE;
	goto again;
    }

    return nNearestItem;
}

/***
 * DESCRIPTION:
 * Searches for an item with specific characteristics.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nStart : base item index
 * [I] lpFindInfo : item information to look for
 *
 * RETURN:
 *   SUCCESS : index of item
 *   FAILURE : -1
 */
static INT LISTVIEW_FindItemA(const LISTVIEW_INFO *infoPtr, INT nStart,
                              const LVFINDINFOA *lpFindInfo)
{
    LVFINDINFOW fiw;
    INT res;
    LPWSTR strW = NULL;

    memcpy(&fiw, lpFindInfo, sizeof(fiw));
    if (lpFindInfo->flags & (LVFI_STRING | LVFI_PARTIAL | LVFI_SUBSTRING))
        fiw.psz = strW = textdupTtoW((LPCWSTR)lpFindInfo->psz, FALSE);
    res = LISTVIEW_FindItemW(infoPtr, nStart, &fiw);
    textfreeT(strW, FALSE);
    return res;
}

/***
 * DESCRIPTION:
 * Retrieves column attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nColumn :  column index
 * [IO] lpColumn : column information
 * [I] isW : if TRUE, then lpColumn is a LPLVCOLUMNW
 *           otherwise it is in fact a LPLVCOLUMNA
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetColumnT(const LISTVIEW_INFO *infoPtr, INT nColumn, LPLVCOLUMNW lpColumn, BOOL isW)
{
    COLUMN_INFO *lpColumnInfo;
    HDITEMW hdi;

    if (!lpColumn || nColumn < 0 || nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return FALSE;
    lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, nColumn);

    /* initialize memory */
    ZeroMemory(&hdi, sizeof(hdi));

    if (lpColumn->mask & LVCF_TEXT)
    {
        hdi.mask |= HDI_TEXT;
        hdi.pszText = lpColumn->pszText;
        hdi.cchTextMax = lpColumn->cchTextMax;
    }

    if (lpColumn->mask & LVCF_IMAGE)
        hdi.mask |= HDI_IMAGE;

    if (lpColumn->mask & LVCF_ORDER)
        hdi.mask |= HDI_ORDER;

    if (lpColumn->mask & LVCF_SUBITEM)
        hdi.mask |= HDI_LPARAM;

    if (!SendMessageW(infoPtr->hwndHeader, isW ? HDM_GETITEMW : HDM_GETITEMA, nColumn, (LPARAM)&hdi)) return FALSE;

    if (lpColumn->mask & LVCF_FMT)
	lpColumn->fmt = lpColumnInfo->fmt;

    if (lpColumn->mask & LVCF_WIDTH)
        lpColumn->cx = lpColumnInfo->rcHeader.right - lpColumnInfo->rcHeader.left;

    if (lpColumn->mask & LVCF_IMAGE)
	lpColumn->iImage = hdi.iImage;

    if (lpColumn->mask & LVCF_ORDER)
	lpColumn->iOrder = hdi.iOrder;

    if (lpColumn->mask & LVCF_SUBITEM)
	lpColumn->iSubItem = hdi.lParam;

    if (lpColumn->mask & LVCF_MINWIDTH)
	lpColumn->cxMin = lpColumnInfo->cxMin;

    return TRUE;
}

static inline BOOL LISTVIEW_GetColumnOrderArray(const LISTVIEW_INFO *infoPtr, INT iCount, LPINT lpiArray)
{
    if (!infoPtr->hwndHeader) return FALSE;
    return SendMessageW(infoPtr->hwndHeader, HDM_GETORDERARRAY, iCount, (LPARAM)lpiArray);
}

/***
 * DESCRIPTION:
 * Retrieves the column width.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] int : column index
 *
 * RETURN:
 *   SUCCESS : column width
 *   FAILURE : zero
 */
static INT LISTVIEW_GetColumnWidth(const LISTVIEW_INFO *infoPtr, INT nColumn)
{
    INT nColumnWidth = 0;
    HDITEMW hdItem;

    TRACE("nColumn=%d\n", nColumn);

    /* we have a 'column' in LIST and REPORT mode only */
    switch(infoPtr->uView)
    {
    case LV_VIEW_LIST:
	nColumnWidth = infoPtr->nItemWidth;
	break;
    case LV_VIEW_DETAILS:
	/* We are not using LISTVIEW_GetHeaderRect as this data is updated only after a HDN_ITEMCHANGED.
	 * There is an application that subclasses the listview, calls LVM_GETCOLUMNWIDTH in the
	 * HDN_ITEMCHANGED handler and goes into infinite recursion if it receives old data.
	 */
	hdItem.mask = HDI_WIDTH;
	if (!SendMessageW(infoPtr->hwndHeader, HDM_GETITEMW, nColumn, (LPARAM)&hdItem))
	{
	    WARN("(%p): HDM_GETITEMW failed for item %d\n", infoPtr->hwndSelf, nColumn);
	    return 0;
	}
	nColumnWidth = hdItem.cxy;
	break;
    }

    TRACE("nColumnWidth=%d\n", nColumnWidth);
    return nColumnWidth;
}

/***
 * DESCRIPTION:
 * In list or report display mode, retrieves the number of items that can fit
 * vertically in the visible area. In icon or small icon display mode,
 * retrieves the total number of visible items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of fully visible items.
 */
static INT LISTVIEW_GetCountPerPage(const LISTVIEW_INFO *infoPtr)
{
    switch (infoPtr->uView)
    {
    case LV_VIEW_ICON:
    case LV_VIEW_SMALLICON:
	return infoPtr->nItemCount;
    case LV_VIEW_DETAILS:
	return LISTVIEW_GetCountPerColumn(infoPtr);
    case LV_VIEW_LIST:
	return LISTVIEW_GetCountPerRow(infoPtr) * LISTVIEW_GetCountPerColumn(infoPtr);
    }
    assert(FALSE);
    return 0;
}

/***
 * DESCRIPTION:
 * Retrieves an image list handle.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nImageList : image list identifier
 *
 * RETURN:
 *   SUCCESS : image list handle
 *   FAILURE : NULL
 */
static HIMAGELIST LISTVIEW_GetImageList(const LISTVIEW_INFO *infoPtr, INT nImageList)
{
    switch (nImageList)
    {
    case LVSIL_NORMAL: return infoPtr->himlNormal;
    case LVSIL_SMALL:  return infoPtr->himlSmall;
    case LVSIL_STATE:  return infoPtr->himlState;
    case LVSIL_GROUPHEADER:
        FIXME("LVSIL_GROUPHEADER not supported\n");
        break;
    default:
        WARN("got unknown imagelist index - %d\n", nImageList);
    }
    return NULL;
}

/* LISTVIEW_GetISearchString */

/***
 * DESCRIPTION:
 * Retrieves item attributes.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [IO] lpLVItem : item info
 * [I] isW : if TRUE, then lpLVItem is a LPLVITEMW,
 *           if FALSE, then lpLVItem is a LPLVITEMA.
 *
 * NOTE:
 *   This is the internal 'GetItem' interface -- it tries to
 *   be smart and avoid text copies, if possible, by modifying
 *   lpLVItem->pszText to point to the text string. Please note
 *   that this is not always possible (e.g. OWNERDATA), so on
 *   entry you *must* supply valid values for pszText, and cchTextMax.
 *   The only difference to the documented interface is that upon
 *   return, you should use *only* the lpLVItem->pszText, rather than
 *   the buffer pointer you provided on input. Most code already does
 *   that, so it's not a problem.
 *   For the two cases when the text must be copied (that is,
 *   for LVM_GETITEM, and LVM_GETITEMTEXT), use LISTVIEW_GetItemExtT.
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetItemT(const LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    ITEMHDR callbackHdr = { LPSTR_TEXTCALLBACKW, I_IMAGECALLBACK };
    BOOL is_subitem_invalid = FALSE;
    NMLVDISPINFOW dispInfo;
    ITEM_INFO *lpItem;
    ITEMHDR* pItemHdr;
    HDPA hdpaSubItems;

    TRACE("(item=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (!lpLVItem || lpLVItem->iItem < 0 || lpLVItem->iItem >= infoPtr->nItemCount)
	return FALSE;

    if (lpLVItem->mask == 0) return TRUE;
    TRACE("mask=%x\n", lpLVItem->mask);

    if (lpLVItem->iSubItem && (lpLVItem->mask & LVIF_STATE))
        lpLVItem->state = 0;

    /* a quick optimization if all we're asked is the focus state
     * these queries are worth optimising since they are common,
     * and can be answered in constant time, without the heavy accesses */
    if ( (lpLVItem->mask == LVIF_STATE) && (lpLVItem->stateMask == LVIS_FOCUSED) &&
	 !(infoPtr->uCallbackMask & LVIS_FOCUSED) )
    {
        lpLVItem->state = 0;
        if (infoPtr->nFocusedItem == lpLVItem->iItem && !lpLVItem->iSubItem)
            lpLVItem->state |= LVIS_FOCUSED;
        return TRUE;
    }

    ZeroMemory(&dispInfo, sizeof(dispInfo));

    /* if the app stores all the data, handle it separately */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	dispInfo.item.state = 0;

	/* apparently, we should not callback for lParam in LVS_OWNERDATA */
	if ((lpLVItem->mask & ~(LVIF_STATE | LVIF_PARAM)) ||
	   ((lpLVItem->mask & LVIF_STATE) && (infoPtr->uCallbackMask & lpLVItem->stateMask)))
	{
	    UINT mask = lpLVItem->mask;

	    /* NOTE: copy only fields which we _know_ are initialized, some apps
	     *       depend on the uninitialized fields being 0 */
	    dispInfo.item.mask = lpLVItem->mask & ~LVIF_PARAM;
	    dispInfo.item.iItem = lpLVItem->iItem;
	    dispInfo.item.iSubItem = lpLVItem->iSubItem;
	    if (lpLVItem->mask & LVIF_TEXT)
	    {
		if (lpLVItem->mask & LVIF_NORECOMPUTE)
		    /* reset mask */
		    dispInfo.item.mask &= ~(LVIF_TEXT | LVIF_NORECOMPUTE);
		else
		{
		    dispInfo.item.pszText = lpLVItem->pszText;
		    dispInfo.item.cchTextMax = lpLVItem->cchTextMax;
		}
	    }
	    if (lpLVItem->mask & LVIF_STATE)
	        dispInfo.item.stateMask = lpLVItem->stateMask & infoPtr->uCallbackMask;
	    /* could be zeroed on LVIF_NORECOMPUTE case */
	    if (dispInfo.item.mask)
	    {
	        notify_dispinfoT(infoPtr, LVN_GETDISPINFOW, &dispInfo, isW);
	        dispInfo.item.stateMask = lpLVItem->stateMask;
	        if (lpLVItem->mask & (LVIF_GROUPID|LVIF_COLUMNS))
	        {
	            /* full size structure expected - _WIN32IE >= 0x560 */
	            *lpLVItem = dispInfo.item;
	        }
	        else if (lpLVItem->mask & LVIF_INDENT)
	        {
	            /* indent member expected - _WIN32IE >= 0x300 */
	            memcpy(lpLVItem, &dispInfo.item, offsetof( LVITEMW, iGroupId ));
	        }
	        else
	        {
	            /* minimal structure expected */
	            memcpy(lpLVItem, &dispInfo.item, offsetof( LVITEMW, iIndent ));
	        }
	        lpLVItem->mask = mask;
	        TRACE("   getdispinfo(1):lpLVItem=%s\n", debuglvitem_t(lpLVItem, isW));
	    }
	}
	
	/* make sure lParam is zeroed out */
	if (lpLVItem->mask & LVIF_PARAM) lpLVItem->lParam = 0;

	/* callback marked pointer required here */
	if ((lpLVItem->mask & LVIF_TEXT) && (lpLVItem->mask & LVIF_NORECOMPUTE))
	    lpLVItem->pszText = LPSTR_TEXTCALLBACKW;

	/* we store only a little state, so if we're not asked, we're done */
	if (!(lpLVItem->mask & LVIF_STATE) || lpLVItem->iSubItem) return TRUE;

	/* if focus is handled by us, report it */
	if ( lpLVItem->stateMask & ~infoPtr->uCallbackMask & LVIS_FOCUSED ) 
	{
	    lpLVItem->state &= ~LVIS_FOCUSED;
	    if (infoPtr->nFocusedItem == lpLVItem->iItem)
	        lpLVItem->state |= LVIS_FOCUSED;
        }

	/* and do the same for selection, if we handle it */
	if ( lpLVItem->stateMask & ~infoPtr->uCallbackMask & LVIS_SELECTED ) 
	{
	    lpLVItem->state &= ~LVIS_SELECTED;
	    if (ranges_contain(infoPtr->selectionRanges, lpLVItem->iItem))
		lpLVItem->state |= LVIS_SELECTED;
	}
	
	return TRUE;
    }

    /* find the item and subitem structures before we proceed */
    hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    lpItem = DPA_GetPtr(hdpaSubItems, 0);
    assert (lpItem);

    if (lpLVItem->iSubItem)
    {
        SUBITEM_INFO *lpSubItem = LISTVIEW_GetSubItemPtr(hdpaSubItems, lpLVItem->iSubItem);
        if (lpSubItem)
            pItemHdr = &lpSubItem->hdr;
        else
        {
            pItemHdr = &callbackHdr;
            is_subitem_invalid = TRUE;
        }
    }
    else
	pItemHdr = &lpItem->hdr;

    /* Do we need to query the state from the app? */
    if ((lpLVItem->mask & LVIF_STATE) && infoPtr->uCallbackMask && (!lpLVItem->iSubItem || is_subitem_invalid))
    {
	dispInfo.item.mask |= LVIF_STATE;
	dispInfo.item.stateMask = infoPtr->uCallbackMask;
    }
  
    /* Do we need to enquire about the image? */
    if ((lpLVItem->mask & LVIF_IMAGE) && pItemHdr->iImage == I_IMAGECALLBACK &&
        (!lpLVItem->iSubItem || (infoPtr->dwLvExStyle & LVS_EX_SUBITEMIMAGES)))
    {
	dispInfo.item.mask |= LVIF_IMAGE;
        dispInfo.item.iImage = I_IMAGECALLBACK;
    }

    /* Only items support indentation */
    if ((lpLVItem->mask & LVIF_INDENT) && lpItem->iIndent == I_INDENTCALLBACK && !lpLVItem->iSubItem)
    {
        dispInfo.item.mask |= LVIF_INDENT;
        dispInfo.item.iIndent = I_INDENTCALLBACK;
    }

    /* Apps depend on calling back for text if it is NULL or LPSTR_TEXTCALLBACKW */
    if ((lpLVItem->mask & LVIF_TEXT) && !(lpLVItem->mask & LVIF_NORECOMPUTE) &&
        !is_text(pItemHdr->pszText))
    {
	dispInfo.item.mask |= LVIF_TEXT;
	dispInfo.item.pszText = lpLVItem->pszText;
	dispInfo.item.cchTextMax = lpLVItem->cchTextMax;
	if (dispInfo.item.pszText && dispInfo.item.cchTextMax > 0)
	    *dispInfo.item.pszText = '\0';
    }

    /* If we don't have all the requested info, query the application */
    if (dispInfo.item.mask)
    {
	dispInfo.item.iItem = lpLVItem->iItem;
	dispInfo.item.iSubItem = lpLVItem->iSubItem;
	dispInfo.item.lParam = lpItem->lParam;
	notify_dispinfoT(infoPtr, LVN_GETDISPINFOW, &dispInfo, isW);
	TRACE("   getdispinfo(2):item=%s\n", debuglvitem_t(&dispInfo.item, isW));
    }

    /* we should not store values for subitems */
    if (lpLVItem->iSubItem) dispInfo.item.mask &= ~LVIF_DI_SETITEM;

    /* Now, handle the iImage field */
    if (dispInfo.item.mask & LVIF_IMAGE)
    {
	lpLVItem->iImage = dispInfo.item.iImage;
	if ((dispInfo.item.mask & LVIF_DI_SETITEM) && pItemHdr->iImage == I_IMAGECALLBACK)
	    pItemHdr->iImage = dispInfo.item.iImage;
    }
    else if (lpLVItem->mask & LVIF_IMAGE)
    {
        if (!lpLVItem->iSubItem || (infoPtr->dwLvExStyle & LVS_EX_SUBITEMIMAGES))
            lpLVItem->iImage = pItemHdr->iImage;
        else
            lpLVItem->iImage = 0;
    }

    /* The pszText field */
    if (dispInfo.item.mask & LVIF_TEXT)
    {
	if ((dispInfo.item.mask & LVIF_DI_SETITEM) && pItemHdr->pszText)
	    textsetptrT(&pItemHdr->pszText, dispInfo.item.pszText, isW);

	lpLVItem->pszText = dispInfo.item.pszText;
    }
    else if (lpLVItem->mask & LVIF_TEXT)
    {
	/* if LVN_GETDISPINFO's disabled with LVIF_NORECOMPUTE return callback placeholder */
	if (isW || !is_text(pItemHdr->pszText)) lpLVItem->pszText = pItemHdr->pszText;
	else textcpynT(lpLVItem->pszText, isW, pItemHdr->pszText, TRUE, lpLVItem->cchTextMax);
    }

    /* Next is the lParam field */
    if (dispInfo.item.mask & LVIF_PARAM)
    {
	lpLVItem->lParam = dispInfo.item.lParam;
	if ((dispInfo.item.mask & LVIF_DI_SETITEM))
	    lpItem->lParam = dispInfo.item.lParam;
    }
    else if (lpLVItem->mask & LVIF_PARAM)
	lpLVItem->lParam = lpItem->lParam;

    /* if this is a subitem, we're done */
    if (lpLVItem->iSubItem) return TRUE;

    /* ... the state field (this one is different due to uCallbackmask) */
    if (lpLVItem->mask & LVIF_STATE)
    {
	lpLVItem->state = lpItem->state & lpLVItem->stateMask;
	if (dispInfo.item.mask & LVIF_STATE)
	{
	    lpLVItem->state &= ~dispInfo.item.stateMask;
	    lpLVItem->state |= (dispInfo.item.state & dispInfo.item.stateMask);
	}
	if ( lpLVItem->stateMask & ~infoPtr->uCallbackMask & LVIS_FOCUSED ) 
	{
	    lpLVItem->state &= ~LVIS_FOCUSED;
	    if (infoPtr->nFocusedItem == lpLVItem->iItem)
	        lpLVItem->state |= LVIS_FOCUSED;
        }
	if ( lpLVItem->stateMask & ~infoPtr->uCallbackMask & LVIS_SELECTED ) 
	{
	    lpLVItem->state &= ~LVIS_SELECTED;
	    if (ranges_contain(infoPtr->selectionRanges, lpLVItem->iItem))
		lpLVItem->state |= LVIS_SELECTED;
	}	    
    }

    /* and last, but not least, the indent field */
    if (dispInfo.item.mask & LVIF_INDENT)
    {
	lpLVItem->iIndent = dispInfo.item.iIndent;
	if ((dispInfo.item.mask & LVIF_DI_SETITEM) && lpItem->iIndent == I_INDENTCALLBACK)
	    lpItem->iIndent = dispInfo.item.iIndent;
    }
    else if (lpLVItem->mask & LVIF_INDENT)
    {
        lpLVItem->iIndent = lpItem->iIndent;
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves item attributes.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [IO] lpLVItem : item info
 * [I] isW : if TRUE, then lpLVItem is a LPLVITEMW,
 *           if FALSE, then lpLVItem is a LPLVITEMA.
 *
 * NOTE:
 *   This is the external 'GetItem' interface -- it properly copies
 *   the text in the provided buffer.
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetItemExtT(const LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    LPWSTR pszText;
    BOOL bResult;

    if (!lpLVItem || lpLVItem->iItem < 0 || lpLVItem->iItem >= infoPtr->nItemCount)
	return FALSE;

    pszText = lpLVItem->pszText;
    bResult = LISTVIEW_GetItemT(infoPtr, lpLVItem, isW);
    if (bResult && (lpLVItem->mask & LVIF_TEXT) && lpLVItem->pszText != pszText)
    {
	if (lpLVItem->pszText != LPSTR_TEXTCALLBACKW)
	    textcpynT(pszText, isW, lpLVItem->pszText, isW, lpLVItem->cchTextMax);
	else
	    pszText = LPSTR_TEXTCALLBACKW;
    }
    lpLVItem->pszText = pszText;

    return bResult;
}


/***
 * DESCRIPTION:
 * Retrieves the position (upper-left) of the listview control item.
 * Note that for LVS_ICON style, the upper-left is that of the icon
 * and not the bounding box.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [O] lpptPosition : coordinate information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetItemPosition(const LISTVIEW_INFO *infoPtr, INT nItem, LPPOINT lpptPosition)
{
    POINT Origin;

    TRACE("(nItem=%d, lpptPosition=%p)\n", nItem, lpptPosition);

    if (!lpptPosition || nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, lpptPosition);

    if (infoPtr->uView == LV_VIEW_ICON)
    {
        lpptPosition->x += (infoPtr->nItemWidth - infoPtr->iconSize.cx) / 2;
        lpptPosition->y += ICON_TOP_PADDING;
    }
    lpptPosition->x += Origin.x;
    lpptPosition->y += Origin.y;
    
    TRACE ("  lpptPosition=%s\n", wine_dbgstr_point(lpptPosition));
    return TRUE;
}


/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle for a listview control item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [IO] lprc : bounding rectangle coordinates
 *     lprc->left specifies the portion of the item for which the bounding
 *     rectangle will be retrieved.
 *
 *     LVIR_BOUNDS Returns the bounding rectangle of the entire item,
 *        including the icon and label.
 *         *
 *         * For LVS_ICON
 *         * Experiment shows that native control returns:
 *         *  width = min (48, length of text line)
 *         *    .left = position.x - (width - iconsize.cx)/2
 *         *    .right = .left + width
 *         *  height = #lines of text * ntmHeight + icon height + 8
 *         *    .top = position.y - 2
 *         *    .bottom = .top + height
 *         *  separation between items .y = itemSpacing.cy - height
 *         *                           .x = itemSpacing.cx - width
 *     LVIR_ICON Returns the bounding rectangle of the icon or small icon.
 *         *
 *         * For LVS_ICON
 *         * Experiment shows that native control returns:
 *         *  width = iconSize.cx + 16
 *         *    .left = position.x - (width - iconsize.cx)/2
 *         *    .right = .left + width
 *         *  height = iconSize.cy + 4
 *         *    .top = position.y - 2
 *         *    .bottom = .top + height
 *         *  separation between items .y = itemSpacing.cy - height
 *         *                           .x = itemSpacing.cx - width
 *     LVIR_LABEL Returns the bounding rectangle of the item text.
 *         *
 *         * For LVS_ICON
 *         * Experiment shows that native control returns:
 *         *  width = text length
 *         *    .left = position.x - width/2
 *         *    .right = .left + width
 *         *  height = ntmH * linecount + 2
 *         *    .top = position.y + iconSize.cy + 6
 *         *    .bottom = .top + height
 *         *  separation between items .y = itemSpacing.cy - height
 *         *                           .x = itemSpacing.cx - width
 *     LVIR_SELECTBOUNDS Returns the union of the LVIR_ICON and LVIR_LABEL
 *	rectangles, but excludes columns in report view.
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 *
 * NOTES
 *   Note that the bounding rectangle of the label in the LVS_ICON view depends
 *   upon whether the window has the focus currently and on whether the item
 *   is the one with the focus.  Ensure that the control's record of which
 *   item has the focus agrees with the items' records.
 */
static BOOL LISTVIEW_GetItemRect(const LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lprc)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    BOOL doLabel = TRUE, oversizedBox = FALSE;
    POINT Position, Origin;
    LVITEMW lvItem;
    LONG mode;

    TRACE("(hwnd=%p, nItem=%d, lprc=%p)\n", infoPtr->hwndSelf, nItem, lprc);

    if (!lprc || nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);

    /* Be smart and try to figure out the minimum we have to do */
    if (lprc->left == LVIR_ICON) doLabel = FALSE;
    if (infoPtr->uView == LV_VIEW_DETAILS && lprc->left == LVIR_BOUNDS) doLabel = FALSE;
    if (infoPtr->uView == LV_VIEW_ICON && lprc->left != LVIR_ICON &&
	infoPtr->bFocus && LISTVIEW_GetItemState(infoPtr, nItem, LVIS_FOCUSED))
	oversizedBox = TRUE;

    /* get what we need from the item before hand, so we make
     * only one request. This can speed up things, if data
     * is stored on the app side */
    lvItem.mask = 0;
    if (infoPtr->uView == LV_VIEW_DETAILS) lvItem.mask |= LVIF_INDENT;
    if (doLabel) lvItem.mask |= LVIF_TEXT;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.pszText = szDispText;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    if (lvItem.mask && !LISTVIEW_GetItemW(infoPtr, &lvItem)) return FALSE;
    /* we got the state already up, simulate it here, to avoid a reget */
    if (infoPtr->uView == LV_VIEW_ICON && (lprc->left != LVIR_ICON))
    {
	lvItem.mask |= LVIF_STATE;
	lvItem.stateMask = LVIS_FOCUSED;
	lvItem.state = (oversizedBox ? LVIS_FOCUSED : 0);
    }

    if (infoPtr->uView == LV_VIEW_DETAILS && (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) && lprc->left == LVIR_SELECTBOUNDS)
	lprc->left = LVIR_BOUNDS;

    mode = lprc->left;
    switch(lprc->left)
    {
    case LVIR_ICON:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, NULL, NULL, lprc, NULL, NULL);
        break;

    case LVIR_LABEL:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, NULL, NULL, NULL, NULL, lprc);
        break;

    case LVIR_BOUNDS:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, lprc, NULL, NULL, NULL, NULL);
        break;

    case LVIR_SELECTBOUNDS:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, NULL, lprc, NULL, NULL, NULL);
        break;

    default:
        WARN("Unknown value: %ld\n", lprc->left);
        return FALSE;
    }

    if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	if (mode != LVIR_BOUNDS)
	    OffsetRect(lprc, Origin.x + LISTVIEW_GetColumnInfo(infoPtr, 0)->rcHeader.left,
	                     Position.y + Origin.y);
	else
	    OffsetRect(lprc, Origin.x, Position.y + Origin.y);
    }
    else
        OffsetRect(lprc, Position.x + Origin.x, Position.y + Origin.y);

    TRACE(" rect=%s\n", wine_dbgstr_rect(lprc));

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the spacing between listview control items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [IO] lprc : rectangle to receive the output
 *             on input, lprc->top = nSubItem
 *                       lprc->left = LVIR_ICON | LVIR_BOUNDS | LVIR_LABEL
 * 
 * NOTE: for subItem = 0, we should return the bounds of the _entire_ item,
 *       not only those of the first column.
 * 
 * RETURN:
 *     TRUE: success
 *     FALSE: failure
 */
static BOOL LISTVIEW_GetSubItemRect(const LISTVIEW_INFO *infoPtr, INT item, LPRECT lprc)
{
    RECT rect = { 0, 0, 0, 0 };
    POINT origin;
    INT y;
    
    if (!lprc) return FALSE;

    TRACE("item %d, subitem %ld, type %ld\n", item, lprc->top, lprc->left);

    /* Subitem of '0' means item itself, and this works for all control view modes */
    if (lprc->top == 0)
        return LISTVIEW_GetItemRect(infoPtr, item, lprc);

    if (infoPtr->uView != LV_VIEW_DETAILS) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &origin);
    /* this works for any item index, no matter if it exists or not */
    y = item * infoPtr->nItemHeight + origin.y;

    if (infoPtr->hwndHeader && SendMessageW(infoPtr->hwndHeader, HDM_GETITEMRECT, lprc->top, (LPARAM)&rect))
    {
        rect.top = 0;
        rect.bottom = infoPtr->nItemHeight;
    }
    else
    {
        /* Native implementation is broken for this case and garbage is left for left and right fields,
           we zero them to get predictable output */
        lprc->left = lprc->right = lprc->top = 0;
        lprc->bottom = infoPtr->nItemHeight;
        OffsetRect(lprc, origin.x, y);
        TRACE("return rect %s\n", wine_dbgstr_rect(lprc));
        return TRUE;
    }

    switch (lprc->left)
    {
    case LVIR_ICON:
    {
        /* it doesn't matter if main item actually has an icon, if imagelist is set icon width is returned */
        if (infoPtr->himlSmall)
            rect.right = rect.left + infoPtr->iconSize.cx;
        else
            rect.right = rect.left;

        rect.bottom = rect.top + infoPtr->iconSize.cy;
        break;
    }
    case LVIR_LABEL:
    case LVIR_BOUNDS:
        break;

    default:
        ERR("Unknown bounds %ld\n", lprc->left);
        return FALSE;
    }

    OffsetRect(&rect, origin.x, y);
    *lprc = rect;
    TRACE("return rect %s\n", wine_dbgstr_rect(lprc));

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the spacing between listview control items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] bSmall : flag for small or large icon
 *
 * RETURN:
 * Horizontal + vertical spacing
 */
static LONG LISTVIEW_GetItemSpacing(const LISTVIEW_INFO *infoPtr, BOOL bSmall)
{
  LONG lResult;

  if (!bSmall)
  {
    lResult = MAKELONG(infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy);
  }
  else
  {
    if (infoPtr->uView == LV_VIEW_ICON)
      lResult = MAKELONG(DEFAULT_COLUMN_WIDTH, GetSystemMetrics(SM_CXSMICON)+HEIGHT_PADDING);
    else
      lResult = MAKELONG(infoPtr->nItemWidth, infoPtr->nItemHeight);
  }
  return lResult;
}

/***
 * DESCRIPTION:
 * Retrieves the state of a listview control item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] uMask : state mask
 *
 * RETURN:
 * State specified by the mask.
 */
static UINT LISTVIEW_GetItemState(const LISTVIEW_INFO *infoPtr, INT nItem, UINT uMask)
{
    LVITEMW lvItem;

    if (nItem < 0 || nItem >= infoPtr->nItemCount) return 0;

    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = uMask;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) return 0;

    return lvItem.state & uMask;
}

/***
 * DESCRIPTION:
 * Retrieves the text of a listview control item or subitem.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nItem : item index
 * [IO] lpLVItem : item information
 * [I] isW :  TRUE if lpLVItem is Unicode
 *
 * RETURN:
 *   SUCCESS : string length
 *   FAILURE : 0
 */
static INT LISTVIEW_GetItemTextT(const LISTVIEW_INFO *infoPtr, INT nItem, LPLVITEMW lpLVItem, BOOL isW)
{
    if (!lpLVItem || nItem < 0 || nItem >= infoPtr->nItemCount) return 0;

    lpLVItem->mask = LVIF_TEXT;
    lpLVItem->iItem = nItem;
    if (!LISTVIEW_GetItemExtT(infoPtr, lpLVItem, isW)) return 0;

    return textlenT(lpLVItem->pszText, isW);
}

/***
 * DESCRIPTION:
 * Searches for an item based on properties + relationships.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] uFlags : relationship flag
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static INT LISTVIEW_GetNextItem(const LISTVIEW_INFO *infoPtr, INT nItem, UINT uFlags)
{
    UINT uMask = 0;
    LVFINDINFOW lvFindInfo;
    INT nCountPerColumn;
    INT nCountPerRow;
    INT i;

    TRACE("nItem=%d, uFlags=%x, nItemCount=%d\n", nItem, uFlags, infoPtr->nItemCount);
    if (nItem < -1 || nItem >= infoPtr->nItemCount) return -1;

    ZeroMemory(&lvFindInfo, sizeof(lvFindInfo));

    if (uFlags & LVNI_CUT)
      uMask |= LVIS_CUT;

    if (uFlags & LVNI_DROPHILITED)
      uMask |= LVIS_DROPHILITED;

    if (uFlags & LVNI_FOCUSED)
      uMask |= LVIS_FOCUSED;

    if (uFlags & LVNI_SELECTED)
      uMask |= LVIS_SELECTED;

    /* if we're asked for the focused item, that's only one, 
     * so it's worth optimizing */
    if (uFlags & LVNI_FOCUSED)
    {
	if ((LISTVIEW_GetItemState(infoPtr, infoPtr->nFocusedItem, uMask) & uMask) != uMask) return -1;
	return (infoPtr->nFocusedItem == nItem) ? -1 : infoPtr->nFocusedItem;
    }
    
    if (uFlags & LVNI_ABOVE)
    {
      if ((infoPtr->uView == LV_VIEW_LIST) || (infoPtr->uView == LV_VIEW_DETAILS))
      {
        while (nItem >= 0)
        {
          nItem--;
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else
      {
        /* Special case for autoarrange - move 'til the top of a list */
        if (is_autoarrange(infoPtr))
        {
          nCountPerRow = LISTVIEW_GetCountPerRow(infoPtr);
          while (nItem - nCountPerRow >= 0)
          {
            nItem -= nCountPerRow;
            if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
              return nItem;
          }
          return -1;
        }
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_UP;
        LISTVIEW_GetItemPosition(infoPtr, nItem, &lvFindInfo.pt);
        while ((nItem = LISTVIEW_FindItemW(infoPtr, nItem, &lvFindInfo)) != -1)
        {
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_BELOW)
    {
      if ((infoPtr->uView == LV_VIEW_LIST) || (infoPtr->uView == LV_VIEW_DETAILS))
      {
        while (nItem < infoPtr->nItemCount)
        {
          nItem++;
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else
      {
        /* Special case for autoarrange - move 'til the bottom of a list */
        if (is_autoarrange(infoPtr))
        {
          nCountPerRow = LISTVIEW_GetCountPerRow(infoPtr);
          while (nItem + nCountPerRow < infoPtr->nItemCount )
          {
            nItem += nCountPerRow;
            if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
              return nItem;
          }
          return -1;
        }
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_DOWN;
        LISTVIEW_GetItemPosition(infoPtr, nItem, &lvFindInfo.pt);
        while ((nItem = LISTVIEW_FindItemW(infoPtr, nItem, &lvFindInfo)) != -1)
        {
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TOLEFT)
    {
      if (infoPtr->uView == LV_VIEW_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
        while (nItem - nCountPerColumn >= 0)
        {
          nItem -= nCountPerColumn;
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((infoPtr->uView == LV_VIEW_SMALLICON) || (infoPtr->uView == LV_VIEW_ICON))
      {
        /* Special case for autoarrange - move 'til the beginning of a row */
        if (is_autoarrange(infoPtr))
        {
          nCountPerRow = LISTVIEW_GetCountPerRow(infoPtr);
          while (nItem % nCountPerRow > 0)
          {
            nItem --;
            if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
              return nItem;
          }
          return -1;
        }
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_LEFT;
        LISTVIEW_GetItemPosition(infoPtr, nItem, &lvFindInfo.pt);
        while ((nItem = LISTVIEW_FindItemW(infoPtr, nItem, &lvFindInfo)) != -1)
        {
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TORIGHT)
    {
      if (infoPtr->uView == LV_VIEW_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
        while (nItem + nCountPerColumn < infoPtr->nItemCount)
        {
          nItem += nCountPerColumn;
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((infoPtr->uView == LV_VIEW_SMALLICON) || (infoPtr->uView == LV_VIEW_ICON))
      {
        /* Special case for autoarrange - move 'til the end of a row */
        if (is_autoarrange(infoPtr))
        {
          nCountPerRow = LISTVIEW_GetCountPerRow(infoPtr);
          while (nItem % nCountPerRow < nCountPerRow - 1 )
          {
            nItem ++;
            if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
              return nItem;
          }
          return -1;
        }
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_RIGHT;
        LISTVIEW_GetItemPosition(infoPtr, nItem, &lvFindInfo.pt);
        while ((nItem = LISTVIEW_FindItemW(infoPtr, nItem, &lvFindInfo)) != -1)
        {
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else
    {
      nItem++;

      /* search by index */
      for (i = nItem; i < infoPtr->nItemCount; i++)
      {
        if ((LISTVIEW_GetItemState(infoPtr, i, uMask) & uMask) == uMask)
          return i;
      }
    }

    return -1;
}

static BOOL LISTVIEW_GetNextItemIndex(const LISTVIEW_INFO *infoPtr, LVITEMINDEX *index, UINT flags)
{
    /* FIXME: specified item group is ignored */

    if (!index)
        return FALSE;

    index->iItem = LISTVIEW_GetNextItem(infoPtr, index->iItem, flags);
    return index->iItem != -1;
}

/* LISTVIEW_GetNumberOfWorkAreas */

/***
 * DESCRIPTION:
 * Retrieves the origin coordinates when in icon or small icon display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lpptOrigin : coordinate information
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_GetOrigin(const LISTVIEW_INFO *infoPtr, LPPOINT lpptOrigin)
{
    INT nHorzPos = 0, nVertPos = 0;
    SCROLLINFO scrollInfo;

    scrollInfo.cbSize = sizeof(SCROLLINFO);    
    scrollInfo.fMask = SIF_POS;
    
    if (GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
	nHorzPos = scrollInfo.nPos;
    if (GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	nVertPos = scrollInfo.nPos;

    TRACE("nHorzPos=%d, nVertPos=%d\n", nHorzPos, nVertPos);

    lpptOrigin->x = infoPtr->rcList.left;
    lpptOrigin->y = infoPtr->rcList.top;
    if (infoPtr->uView == LV_VIEW_LIST)
	nHorzPos *= infoPtr->nItemWidth;
    else if (infoPtr->uView == LV_VIEW_DETAILS)
	nVertPos *= infoPtr->nItemHeight;
    
    lpptOrigin->x -= nHorzPos;
    lpptOrigin->y -= nVertPos;

    TRACE(" origin=%s\n", wine_dbgstr_point(lpptOrigin));
}

/***
 * DESCRIPTION:
 * Retrieves the width of a string.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] lpszText : text string to process
 * [I] isW : TRUE if lpszText is Unicode, FALSE otherwise
 *
 * RETURN:
 *   SUCCESS : string width (in pixels)
 *   FAILURE : zero
 */
static INT LISTVIEW_GetStringWidthT(const LISTVIEW_INFO *infoPtr, LPCWSTR lpszText, BOOL isW)
{
    SIZE stringSize;
    
    stringSize.cx = 0;    
    if (is_text(lpszText))
    {
    	HFONT hFont = infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont;
    	HDC hdc = GetDC(infoPtr->hwndSelf);
    	HFONT hOldFont = SelectObject(hdc, hFont);

    	if (isW)
  	    GetTextExtentPointW(hdc, lpszText, lstrlenW(lpszText), &stringSize);
    	else
  	    GetTextExtentPointA(hdc, (LPCSTR)lpszText, lstrlenA((LPCSTR)lpszText), &stringSize);
    	SelectObject(hdc, hOldFont);
    	ReleaseDC(infoPtr->hwndSelf, hdc);
    }
    return stringSize.cx;
}

/***
 * DESCRIPTION:
 * Determines which listview item is located at the specified position.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [IO] lpht : hit test information
 * [I] subitem : fill out iSubItem.
 * [I] select : return the index only if the hit selects the item
 *
 * NOTE:
 * (mm 20001022): We must not allow iSubItem to be touched, for
 * an app might pass only a structure with space up to iItem!
 * (MS Office 97 does that for instance in the file open dialog)
 * 
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static INT LISTVIEW_HitTest(const LISTVIEW_INFO *infoPtr, LPLVHITTESTINFO lpht, BOOL subitem, BOOL select)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    RECT rcBox, rcBounds, rcState, rcIcon, rcLabel, rcSearch;
    POINT Origin, Position, opt;
    BOOL is_fullrow;
    LVITEMW lvItem;
    ITERATOR i;
    INT iItem;
    
    TRACE("(pt=%s, subitem=%d, select=%d)\n", wine_dbgstr_point(&lpht->pt), subitem, select);
    
    lpht->flags = 0;
    lpht->iItem = -1;
    if (subitem) lpht->iSubItem = 0;

    LISTVIEW_GetOrigin(infoPtr, &Origin);

    /* set whole list relation flags */
    if (subitem && infoPtr->uView == LV_VIEW_DETAILS)
    {
        /* LVM_SUBITEMHITTEST checks left bound of possible client area */
        if (infoPtr->rcList.left > lpht->pt.x && Origin.x < lpht->pt.x)
	    lpht->flags |= LVHT_TOLEFT;

	if (lpht->pt.y < infoPtr->rcList.top && lpht->pt.y >= 0)
	    opt.y = lpht->pt.y + infoPtr->rcList.top;
	else
	    opt.y = lpht->pt.y;

	if (infoPtr->rcList.bottom < opt.y)
	    lpht->flags |= LVHT_BELOW;
    }
    else
    {
	if (infoPtr->rcList.left > lpht->pt.x)
	    lpht->flags |= LVHT_TOLEFT;
	else if (infoPtr->rcList.right < lpht->pt.x)
	    lpht->flags |= LVHT_TORIGHT;

	if (infoPtr->rcList.top > lpht->pt.y)
	    lpht->flags |= LVHT_ABOVE;
	else if (infoPtr->rcList.bottom < lpht->pt.y)
	    lpht->flags |= LVHT_BELOW;
    }

    /* even if item is invalid try to find subitem */
    if (infoPtr->uView == LV_VIEW_DETAILS && subitem)
    {
	RECT *pRect;
	INT j;

	opt.x = lpht->pt.x - Origin.x;

	lpht->iSubItem = -1;
	for (j = 0; j < DPA_GetPtrCount(infoPtr->hdpaColumns); j++)
	{
	    pRect = &LISTVIEW_GetColumnInfo(infoPtr, j)->rcHeader;

	    if ((opt.x >= pRect->left) && (opt.x < pRect->right))
	    {
		lpht->iSubItem = j;
		break;
	    }
	}
	TRACE("lpht->iSubItem=%d\n", lpht->iSubItem);

	/* if we're outside horizontal columns bounds there's nothing to test further */
	if (lpht->iSubItem == -1)
	{
	    lpht->iItem = -1;
	    lpht->flags = LVHT_NOWHERE;
	    return -1;
	}
    }

    TRACE("lpht->flags=0x%x\n", lpht->flags);
    if (lpht->flags) return -1;

    lpht->flags |= LVHT_NOWHERE;

    /* first deal with the large items */
    rcSearch.left = lpht->pt.x;
    rcSearch.top = lpht->pt.y;
    rcSearch.right = rcSearch.left + 1;
    rcSearch.bottom = rcSearch.top + 1;

    iterator_frameditems(&i, infoPtr, &rcSearch);
    iterator_next(&i); /* go to first item in the sequence */
    iItem = i.nItem;
    iterator_destroy(&i);

    TRACE("lpht->iItem=%d\n", iItem);
    if (iItem == -1) return -1;

    lvItem.mask = LVIF_STATE | LVIF_TEXT;
    if (infoPtr->uView == LV_VIEW_DETAILS) lvItem.mask |= LVIF_INDENT;
    lvItem.stateMask = LVIS_STATEIMAGEMASK;
    if (infoPtr->uView == LV_VIEW_ICON) lvItem.stateMask |= LVIS_FOCUSED;
    lvItem.iItem = iItem;
    lvItem.iSubItem = subitem ? lpht->iSubItem : 0;
    lvItem.pszText = szDispText;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) return -1;
    if (!infoPtr->bFocus) lvItem.state &= ~LVIS_FOCUSED;

    LISTVIEW_GetItemMetrics(infoPtr, &lvItem, &rcBox, NULL, &rcIcon, &rcState, &rcLabel);
    LISTVIEW_GetItemOrigin(infoPtr, iItem, &Position);
    opt.x = lpht->pt.x - Position.x - Origin.x;

    if (lpht->pt.y < infoPtr->rcList.top && lpht->pt.y >= 0)
	opt.y = lpht->pt.y - Position.y - Origin.y + infoPtr->rcList.top;
    else
	opt.y = lpht->pt.y - Position.y - Origin.y;

    if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	rcBounds = rcBox;
	if (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT)
	    opt.x = lpht->pt.x - Origin.x;
    }
    else
    {
        UnionRect(&rcBounds, &rcIcon, &rcLabel);
        UnionRect(&rcBounds, &rcBounds, &rcState);
    }
    TRACE("rcBounds=%s\n", wine_dbgstr_rect(&rcBounds));
    if (!PtInRect(&rcBounds, opt)) return -1;

    /* That's a special case - row rectangle is used as item rectangle and
       returned flags contain all item parts. */
    is_fullrow = (infoPtr->uView == LV_VIEW_DETAILS) && ((infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) || (infoPtr->dwStyle & LVS_OWNERDRAWFIXED));

    if (PtInRect(&rcIcon, opt))
	lpht->flags |= LVHT_ONITEMICON;
    else if (PtInRect(&rcLabel, opt))
	lpht->flags |= LVHT_ONITEMLABEL;
    else if (infoPtr->himlState && PtInRect(&rcState, opt))
	lpht->flags |= LVHT_ONITEMSTATEICON;
    if (is_fullrow && !(lpht->flags & LVHT_ONITEM))
    {
	lpht->flags = LVHT_ONITEM | LVHT_ABOVE;
    }
    if (lpht->flags & LVHT_ONITEM)
	lpht->flags &= ~LVHT_NOWHERE;
    TRACE("lpht->flags=0x%x\n", lpht->flags); 

    if (select && !is_fullrow)
    {
        if (infoPtr->uView == LV_VIEW_DETAILS)
        {
            /* get main item bounds */
            lvItem.iSubItem = 0;
            LISTVIEW_GetItemMetrics(infoPtr, &lvItem, &rcBox, NULL, &rcIcon, &rcState, &rcLabel);
            UnionRect(&rcBounds, &rcIcon, &rcLabel);
            UnionRect(&rcBounds, &rcBounds, &rcState);
        }
        if (!PtInRect(&rcBounds, opt)) iItem = -1;
    }
    return lpht->iItem = iItem;
}

/***
 * DESCRIPTION:
 * Inserts a new item in the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : item information
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : new item index
 *   FAILURE : -1
 */
static INT LISTVIEW_InsertItemT(LISTVIEW_INFO *infoPtr, const LVITEMW *lpLVItem, BOOL isW)
{
    INT nItem;
    HDPA hdpaSubItems;
    NMLISTVIEW nmlv;
    ITEM_INFO *lpItem;
    ITEM_ID *lpID;
    BOOL is_sorted, has_changed;
    LVITEMW item;
    HWND hwndSelf = infoPtr->hwndSelf;

    TRACE("(item=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (infoPtr->dwStyle & LVS_OWNERDATA) return infoPtr->nItemCount++;

    /* make sure it's an item, and not a subitem; cannot insert a subitem */
    if (!lpLVItem || lpLVItem->iSubItem) return -1;

    if (!is_assignable_item(lpLVItem, infoPtr->dwStyle)) return -1;

    if (!(lpItem = Alloc(sizeof(*lpItem)))) return -1;
    
    /* insert item in listview control data structure */
    if ( !(hdpaSubItems = DPA_Create(8)) ) goto fail;
    if ( !DPA_SetPtr(hdpaSubItems, 0, lpItem) ) assert (FALSE);

    /* link with id struct */
    if (!(lpID = Alloc(sizeof(*lpID)))) goto fail;
    lpItem->id = lpID;
    lpID->item = hdpaSubItems;
    lpID->id = get_next_itemid(infoPtr);
    if ( DPA_InsertPtr(infoPtr->hdpaItemIds, infoPtr->nItemCount, lpID) == -1) goto fail;

    is_sorted = (infoPtr->dwStyle & (LVS_SORTASCENDING | LVS_SORTDESCENDING)) &&
	        !(infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && (LPSTR_TEXTCALLBACKW != lpLVItem->pszText);

    if (lpLVItem->iItem < 0 && !is_sorted) return -1;

    /* calculate new item index */
    if (is_sorted)
    {
        HDPA hItem;
        ITEM_INFO *item_s;
        INT i = 0, cmpv;
        WCHAR *textW;

        textW = textdupTtoW(lpLVItem->pszText, isW);

        while (i < infoPtr->nItemCount)
        {
            hItem  = DPA_GetPtr( infoPtr->hdpaItems, i);
            item_s = DPA_GetPtr(hItem, 0);

            cmpv = textcmpWT(item_s->hdr.pszText, textW, TRUE);
            if (infoPtr->dwStyle & LVS_SORTDESCENDING) cmpv *= -1;

            if (cmpv >= 0) break;
            i++;
        }

        textfreeT(textW, isW);

        nItem = i;
    }
    else
        nItem = min(lpLVItem->iItem, infoPtr->nItemCount);

    TRACE("inserting at %d, sorted=%d, count=%d, iItem=%d\n", nItem, is_sorted, infoPtr->nItemCount, lpLVItem->iItem);
    nItem = DPA_InsertPtr( infoPtr->hdpaItems, nItem, hdpaSubItems );
    if (nItem == -1) goto fail;
    infoPtr->nItemCount++;

    /* shift indices first so they don't get tangled */
    LISTVIEW_ShiftIndices(infoPtr, nItem, 1);

    /* set the item attributes */
    if (lpLVItem->mask & (LVIF_GROUPID|LVIF_COLUMNS))
    {
        /* full size structure expected - _WIN32IE >= 0x560 */
        item = *lpLVItem;
    }
    else if (lpLVItem->mask & LVIF_INDENT)
    {
        /* indent member expected - _WIN32IE >= 0x300 */
        memcpy(&item, lpLVItem, offsetof( LVITEMW, iGroupId ));
    }
    else
    {
        /* minimal structure expected */
        memcpy(&item, lpLVItem, offsetof( LVITEMW, iIndent ));
    }
    item.iItem = nItem;
    if (infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES)
    {
        if (item.mask & LVIF_STATE)
        {
            item.stateMask |= LVIS_STATEIMAGEMASK;
            item.state &= ~LVIS_STATEIMAGEMASK;
            item.state |= INDEXTOSTATEIMAGEMASK(1);
        }
        else
        {
            item.mask |= LVIF_STATE;
            item.stateMask = LVIS_STATEIMAGEMASK;
            item.state = INDEXTOSTATEIMAGEMASK(1);
        }
    }

    if (!set_main_item(infoPtr, &item, TRUE, isW, &has_changed)) goto undo;

    /* make room for the position, if we are in the right mode */
    if ((infoPtr->uView == LV_VIEW_SMALLICON) || (infoPtr->uView == LV_VIEW_ICON))
    {
        if (DPA_InsertPtr(infoPtr->hdpaPosX, nItem, 0) == -1)
	    goto undo;
        if (DPA_InsertPtr(infoPtr->hdpaPosY, nItem, 0) == -1)
	{
	    DPA_DeletePtr(infoPtr->hdpaPosX, nItem);
	    goto undo;
	}
    }

    /* send LVN_INSERTITEM notification */
    memset(&nmlv, 0, sizeof(NMLISTVIEW));
    nmlv.iItem = nItem;
    nmlv.lParam = lpItem->lParam;
    notify_listview(infoPtr, LVN_INSERTITEM, &nmlv);
    if (!IsWindow(hwndSelf))
	return -1;

    /* align items (set position of each item) */
    if (infoPtr->uView == LV_VIEW_SMALLICON || infoPtr->uView == LV_VIEW_ICON)
    {
	POINT pt;

	if (infoPtr->dwStyle & LVS_ALIGNLEFT)
	    LISTVIEW_NextIconPosLeft(infoPtr, &pt);
        else
	    LISTVIEW_NextIconPosTop(infoPtr, &pt);

	LISTVIEW_MoveIconTo(infoPtr, nItem, &pt, TRUE);
    }

    /* now is the invalidation fun */
    LISTVIEW_ScrollOnInsert(infoPtr, nItem, 1);
    return nItem;

undo:
    LISTVIEW_ShiftIndices(infoPtr, nItem, -1);
    LISTVIEW_ShiftFocus(infoPtr, infoPtr->nFocusedItem, nItem, -1);
    DPA_DeletePtr(infoPtr->hdpaItems, nItem);
    infoPtr->nItemCount--;
fail:
    DPA_DeletePtr(hdpaSubItems, 0);
    DPA_Destroy (hdpaSubItems);
    Free (lpItem);
    return -1;
}

/***
 * DESCRIPTION:
 * Checks item visibility.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nFirst : item index to check for
 *
 * RETURN:
 *   Item visible : TRUE
 *   Item invisible or failure : FALSE
 */
static BOOL LISTVIEW_IsItemVisible(const LISTVIEW_INFO *infoPtr, INT nItem)
{
    POINT Origin, Position;
    RECT rcItem;
    HDC hdc;
    BOOL ret;

    TRACE("nItem=%d\n", nItem);

    if (nItem < 0 || nItem >= DPA_GetPtrCount(infoPtr->hdpaItems)) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);
    rcItem.left = Position.x + Origin.x;
    rcItem.top  = Position.y + Origin.y;
    rcItem.right  = rcItem.left + infoPtr->nItemWidth;
    rcItem.bottom = rcItem.top + infoPtr->nItemHeight;

    hdc = GetDC(infoPtr->hwndSelf);
    if (!hdc) return FALSE;
    ret = RectVisible(hdc, &rcItem);
    ReleaseDC(infoPtr->hwndSelf, hdc);

    return ret;
}

/***
 * DESCRIPTION:
 * Redraws a range of items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nFirst : first item
 * [I] nLast : last item
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_RedrawItems(const LISTVIEW_INFO *infoPtr, INT nFirst, INT nLast)
{
    INT i;

    for (i = max(nFirst, 0); i <= min(nLast, infoPtr->nItemCount - 1); i++)
	LISTVIEW_InvalidateItem(infoPtr, i);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Scroll the content of a listview.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] dx : horizontal scroll amount in pixels
 * [I] dy : vertical scroll amount in pixels
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 *
 * COMMENTS:
 *  If the control is in report view (LV_VIEW_DETAILS) the control can
 *  be scrolled only in line increments. "dy" will be rounded to the
 *  nearest number of pixels that are a whole line. Ex: if line height
 *  is 16 and an 8 is passed, the list will be scrolled by 16. If a 7
 *  is passed, then the scroll will be 0.  (per MSDN 7/2002)
 */
static BOOL LISTVIEW_Scroll(LISTVIEW_INFO *infoPtr, INT dx, INT dy)
{
    switch(infoPtr->uView) {
    case LV_VIEW_DETAILS:
	dy += (dy < 0 ? -1 : 1) * infoPtr->nItemHeight/2;
        dy /= infoPtr->nItemHeight;
	break;
    case LV_VIEW_LIST:
    	if (dy != 0) return FALSE;
	break;
    default: /* icon */
	break;
    }	

    if (dx != 0) LISTVIEW_HScroll(infoPtr, SB_INTERNAL, dx);
    if (dy != 0) LISTVIEW_VScroll(infoPtr, SB_INTERNAL, dy);
  
    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the background color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] color   : background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetBkColor(LISTVIEW_INFO *infoPtr, COLORREF color)
{
    TRACE("color %lx\n", color);

    if(infoPtr->clrBk != color) {
	if (infoPtr->clrBk != CLR_NONE) DeleteObject(infoPtr->hBkBrush);
	infoPtr->clrBk = color;
	if (color == CLR_NONE)
	    infoPtr->hBkBrush = (HBRUSH)GetClassLongPtrW(infoPtr->hwndSelf, GCLP_HBRBACKGROUND);
	else
	{
	    infoPtr->hBkBrush = CreateSolidBrush(color);
	    infoPtr->dwLvExStyle &= ~LVS_EX_TRANSPARENTBKGND;
	}
    }

    return TRUE;
}

static BOOL LISTVIEW_SetBkImage(LISTVIEW_INFO *infoPtr, const LVBKIMAGEW *image, BOOL isW)
{
    TRACE("%08lx, %p, %p, %u, %d, %d\n", image->ulFlags, image->hbm, image->pszImage,
          image->cchImageMax, image->xOffsetPercent, image->yOffsetPercent);

    if (image->ulFlags & ~LVBKIF_SOURCE_MASK)
        FIXME("unsupported flags %08lx\n", image->ulFlags & ~LVBKIF_SOURCE_MASK);

    if (image->xOffsetPercent || image->yOffsetPercent)
        FIXME("unsupported offset %d,%d\n", image->xOffsetPercent, image->yOffsetPercent);

    switch (image->ulFlags & LVBKIF_SOURCE_MASK)
    {
    case LVBKIF_SOURCE_NONE:
        if (infoPtr->hBkBitmap)
        {
            DeleteObject(infoPtr->hBkBitmap);
            infoPtr->hBkBitmap = NULL;
        }
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
        break;

    case LVBKIF_SOURCE_HBITMAP:
    {
        BITMAP bm;

        if (infoPtr->hBkBitmap)
        {
            DeleteObject(infoPtr->hBkBitmap);
            infoPtr->hBkBitmap = NULL;
        }
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
        if (GetObjectW(image->hbm, sizeof(bm), &bm) == sizeof(bm))
        {
            infoPtr->hBkBitmap = image->hbm;
            return TRUE;
        }
        break;
    }

    case LVBKIF_SOURCE_URL:
        FIXME("LVBKIF_SOURCE_URL: %s\n", isW ? debugstr_w(image->pszImage) : debugstr_a((LPCSTR)image->pszImage));
        break;
    }

    return FALSE;
}

/*** Helper for {Insert,Set}ColumnT *only* */
static void column_fill_hditem(const LISTVIEW_INFO *infoPtr, HDITEMW *lphdi, INT nColumn,
                               const LVCOLUMNW *lpColumn, BOOL isW)
{
    if (lpColumn->mask & LVCF_FMT)
    {
	/* format member is valid */
	lphdi->mask |= HDI_FORMAT;

	/* set text alignment (leftmost column must be left-aligned) */
        if (nColumn == 0 || (lpColumn->fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
            lphdi->fmt |= HDF_LEFT;
        else if ((lpColumn->fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
            lphdi->fmt |= HDF_RIGHT;
        else if ((lpColumn->fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_CENTER)
            lphdi->fmt |= HDF_CENTER;

        if (lpColumn->fmt & LVCFMT_BITMAP_ON_RIGHT)
            lphdi->fmt |= HDF_BITMAP_ON_RIGHT;

        if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
        {
            lphdi->fmt |= HDF_IMAGE;
            lphdi->iImage = I_IMAGECALLBACK;
        }

        if (lpColumn->fmt & LVCFMT_FIXED_WIDTH)
            lphdi->fmt |= HDF_FIXEDWIDTH;
    }

    if (lpColumn->mask & LVCF_WIDTH)
    {
        lphdi->mask |= HDI_WIDTH;
        if(lpColumn->cx == LVSCW_AUTOSIZE_USEHEADER)
        {
            /* make it fill the remainder of the controls width */
            RECT rcHeader;
            INT item_index;

            for(item_index = 0; item_index < (nColumn - 1); item_index++)
	    {
            	LISTVIEW_GetHeaderRect(infoPtr, item_index, &rcHeader);
		lphdi->cxy += rcHeader.right - rcHeader.left;
	    }

            /* retrieve the layout of the header */
            GetClientRect(infoPtr->hwndSelf, &rcHeader);
            TRACE("start cxy=%d rcHeader=%s\n", lphdi->cxy, wine_dbgstr_rect(&rcHeader));

            lphdi->cxy = (rcHeader.right - rcHeader.left) - lphdi->cxy;
        }
        else
            lphdi->cxy = lpColumn->cx;
    }

    if (lpColumn->mask & LVCF_TEXT)
    {
        lphdi->mask |= HDI_TEXT | HDI_FORMAT;
        lphdi->fmt |= HDF_STRING;
        lphdi->pszText = lpColumn->pszText;
        lphdi->cchTextMax = textlenT(lpColumn->pszText, isW);
    }

    if (lpColumn->mask & LVCF_IMAGE)
    {
        lphdi->mask |= HDI_IMAGE;
        lphdi->iImage = lpColumn->iImage;
    }

    if (lpColumn->mask & LVCF_ORDER)
    {
	lphdi->mask |= HDI_ORDER;
	lphdi->iOrder = lpColumn->iOrder;
    }
}


/***
 * DESCRIPTION:
 * Inserts a new column.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nColumn : column index
 * [I] lpColumn : column information
 * [I] isW : TRUE if lpColumn is Unicode, FALSE otherwise
 *
 * RETURN:
 *   SUCCESS : new column index
 *   FAILURE : -1
 */
static INT LISTVIEW_InsertColumnT(LISTVIEW_INFO *infoPtr, INT nColumn,
                                  const LVCOLUMNW *lpColumn, BOOL isW)
{
    COLUMN_INFO *lpColumnInfo;
    INT nNewColumn;
    HDITEMW hdi;

    TRACE("(nColumn=%d, lpColumn=%s, isW=%d)\n", nColumn, debuglvcolumn_t(lpColumn, isW), isW);

    if (!lpColumn || nColumn < 0) return -1;
    nColumn = min(nColumn, DPA_GetPtrCount(infoPtr->hdpaColumns));
    
    ZeroMemory(&hdi, sizeof(HDITEMW));
    column_fill_hditem(infoPtr, &hdi, nColumn, lpColumn, isW);

    /*
     * A mask not including LVCF_WIDTH turns into a mask of width, width 10
     * (can be seen in SPY) otherwise column never gets added.
     */
    if (!(lpColumn->mask & LVCF_WIDTH)) {
        hdi.mask |= HDI_WIDTH;
        hdi.cxy = 10;
    }

    /*
     * when the iSubItem is available Windows copies it to the header lParam. It seems
     * to happen only in LVM_INSERTCOLUMN - not in LVM_SETCOLUMN
     */
    if (lpColumn->mask & LVCF_SUBITEM)
    {
        hdi.mask |= HDI_LPARAM;
        hdi.lParam = lpColumn->iSubItem;
    }

    /* create header if not present */
    LISTVIEW_CreateHeader(infoPtr);
    if (!(LVS_NOCOLUMNHEADER & infoPtr->dwStyle) &&
         (infoPtr->uView == LV_VIEW_DETAILS) && (WS_VISIBLE & infoPtr->dwStyle))
    {
        ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
    }

    /* insert item in header control */
    nNewColumn = SendMessageW(infoPtr->hwndHeader, 
		              isW ? HDM_INSERTITEMW : HDM_INSERTITEMA,
                              nColumn, (LPARAM)&hdi);
    if (nNewColumn == -1) return -1;
    if (nNewColumn != nColumn) ERR("nColumn=%d, nNewColumn=%d\n", nColumn, nNewColumn);
   
    /* create our own column info */ 
    if (!(lpColumnInfo = Alloc(sizeof(*lpColumnInfo)))) goto fail;
    if (DPA_InsertPtr(infoPtr->hdpaColumns, nNewColumn, lpColumnInfo) == -1) goto fail;

    if (lpColumn->mask & LVCF_FMT) lpColumnInfo->fmt = lpColumn->fmt;
    if (lpColumn->mask & LVCF_MINWIDTH) lpColumnInfo->cxMin = lpColumn->cxMin;
    if (!SendMessageW(infoPtr->hwndHeader, HDM_GETITEMRECT, nNewColumn, (LPARAM)&lpColumnInfo->rcHeader))
        goto fail;

    /* now we have to actually adjust the data */
    if (!(infoPtr->dwStyle & LVS_OWNERDATA) && infoPtr->nItemCount > 0)
    {
	SUBITEM_INFO *lpSubItem;
	HDPA hdpaSubItems;
	INT nItem, i;
	LVITEMW item;
	BOOL changed;

	item.iSubItem = nNewColumn;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iImage = I_IMAGECALLBACK;
	item.pszText = LPSTR_TEXTCALLBACKW;

	for (nItem = 0; nItem < infoPtr->nItemCount; nItem++)
	{
            hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, nItem);
	    for (i = 1; i < DPA_GetPtrCount(hdpaSubItems); i++)
	    {
                lpSubItem = DPA_GetPtr(hdpaSubItems, i);
		if (lpSubItem->iSubItem >= nNewColumn)
		    lpSubItem->iSubItem++;
	    }

	    /* add new subitem for each item */
	    item.iItem = nItem;
	    set_sub_item(infoPtr, &item, isW, &changed);
	}
    }

    /* make space for the new column */
    LISTVIEW_ScrollColumns(infoPtr, nNewColumn + 1, lpColumnInfo->rcHeader.right - lpColumnInfo->rcHeader.left);
    LISTVIEW_UpdateItemSize(infoPtr);
    
    return nNewColumn;

fail:
    if (nNewColumn != -1) SendMessageW(infoPtr->hwndHeader, HDM_DELETEITEM, nNewColumn, 0);
    if (lpColumnInfo)
    {
        DPA_DeletePtr(infoPtr->hdpaColumns, nNewColumn);
        Free(lpColumnInfo);
    }
    return -1;
}

/***
 * DESCRIPTION:
 * Sets the attributes of a header item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nColumn : column index
 * [I] lpColumn : column attributes
 * [I] isW: if TRUE, then lpColumn is a LPLVCOLUMNW, else it is a LPLVCOLUMNA
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetColumnT(const LISTVIEW_INFO *infoPtr, INT nColumn,
                                const LVCOLUMNW *lpColumn, BOOL isW)
{
    HDITEMW hdi, hdiget;
    BOOL bResult;

    TRACE("(nColumn=%d, lpColumn=%s, isW=%d)\n", nColumn, debuglvcolumn_t(lpColumn, isW), isW);
    
    if (!lpColumn || nColumn < 0 || nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return FALSE;

    ZeroMemory(&hdi, sizeof(HDITEMW));
    if (lpColumn->mask & LVCF_FMT)
    {
        hdi.mask |= HDI_FORMAT;
        hdiget.mask = HDI_FORMAT;
        if (SendMessageW(infoPtr->hwndHeader, HDM_GETITEMW, nColumn, (LPARAM)&hdiget))
	    hdi.fmt = hdiget.fmt & HDF_STRING;
    }
    column_fill_hditem(infoPtr, &hdi, nColumn, lpColumn, isW);

    /* set header item attributes */
    bResult = SendMessageW(infoPtr->hwndHeader, isW ? HDM_SETITEMW : HDM_SETITEMA, nColumn, (LPARAM)&hdi);
    if (!bResult) return FALSE;

    if (lpColumn->mask & LVCF_FMT)
    {
	COLUMN_INFO *lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, nColumn);
	INT oldFmt = lpColumnInfo->fmt;
	
	lpColumnInfo->fmt = lpColumn->fmt;
	if ((oldFmt ^ lpColumn->fmt) & (LVCFMT_JUSTIFYMASK | LVCFMT_IMAGE))
	{
	    if (infoPtr->uView == LV_VIEW_DETAILS) LISTVIEW_InvalidateColumn(infoPtr, nColumn);
	}
    }

    if (lpColumn->mask & LVCF_MINWIDTH)
	LISTVIEW_GetColumnInfo(infoPtr, nColumn)->cxMin = lpColumn->cxMin;

    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the column order array
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] iCount : number of elements in column order array
 * [I] lpiArray : pointer to column order array
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetColumnOrderArray(LISTVIEW_INFO *infoPtr, INT iCount, const INT *lpiArray)
{
    if (!infoPtr->hwndHeader) return FALSE;
    infoPtr->colRectsDirty = TRUE;
    return SendMessageW(infoPtr->hwndHeader, HDM_SETORDERARRAY, iCount, (LPARAM)lpiArray);
}

/***
 * DESCRIPTION:
 * Sets the width of a column
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nColumn : column index
 * [I] cx : column width
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetColumnWidth(LISTVIEW_INFO *infoPtr, INT nColumn, INT cx)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { 0 };
    INT max_cx = 0;
    HDITEMW hdi;

    TRACE("(nColumn=%d, cx=%d)\n", nColumn, cx);

    /* set column width only if in report or list mode */
    if (infoPtr->uView != LV_VIEW_DETAILS && infoPtr->uView != LV_VIEW_LIST) return FALSE;

    /* take care of invalid cx values - LVSCW_AUTOSIZE_* values are negative,
       with _USEHEADER being the lowest */
    if (infoPtr->uView == LV_VIEW_DETAILS && cx < LVSCW_AUTOSIZE_USEHEADER) cx = LVSCW_AUTOSIZE;
    else if (infoPtr->uView == LV_VIEW_LIST && cx <= 0) return FALSE;

    /* resize all columns if in LV_VIEW_LIST mode */
    if(infoPtr->uView == LV_VIEW_LIST)
    {
	infoPtr->nItemWidth = cx;
	LISTVIEW_InvalidateList(infoPtr);
	return TRUE;
    }

    if (nColumn < 0 || nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return FALSE;

    if (cx == LVSCW_AUTOSIZE || (cx == LVSCW_AUTOSIZE_USEHEADER && nColumn < DPA_GetPtrCount(infoPtr->hdpaColumns) -1))
    {
	INT nLabelWidth;
	LVITEMW lvItem;

	lvItem.mask = LVIF_TEXT;	
	lvItem.iItem = 0;
	lvItem.iSubItem = nColumn;
	lvItem.cchTextMax = DISP_TEXT_SIZE;
	for (; lvItem.iItem < infoPtr->nItemCount; lvItem.iItem++)
	{
            lvItem.pszText = szDispText;
	    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) continue;
	    nLabelWidth = LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE);
	    if (max_cx < nLabelWidth) max_cx = nLabelWidth;
	}
	if (infoPtr->himlSmall && (nColumn == 0 || (LISTVIEW_GetColumnInfo(infoPtr, nColumn)->fmt & LVCFMT_IMAGE)))
	    max_cx += infoPtr->iconSize.cx;
	max_cx += TRAILING_LABEL_PADDING;
        if (nColumn == 0 && (infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES))
            max_cx += GetSystemMetrics(SM_CXSMICON);
    }

    /* autosize based on listview items width */
    if(cx == LVSCW_AUTOSIZE)
	cx = max_cx;
    else if(cx == LVSCW_AUTOSIZE_USEHEADER)
    {
	/* if iCol is the last column make it fill the remainder of the controls width */
        if(nColumn == DPA_GetPtrCount(infoPtr->hdpaColumns) - 1) 
	{
	    RECT rcHeader;
	    POINT Origin;

	    LISTVIEW_GetOrigin(infoPtr, &Origin);
	    LISTVIEW_GetHeaderRect(infoPtr, nColumn, &rcHeader);

	    cx = infoPtr->rcList.right - Origin.x - rcHeader.left;
	}
	else
	{
            /* Despite what the MS docs say, if this is not the last
               column, then MS resizes the column to the width of the
               largest text string in the column, including headers
               and items. This is different from LVSCW_AUTOSIZE in that
	       LVSCW_AUTOSIZE ignores the header string length. */
	    cx = 0;

	    /* retrieve header text */
	    hdi.mask = HDI_TEXT|HDI_FORMAT|HDI_IMAGE|HDI_BITMAP;
	    hdi.cchTextMax = DISP_TEXT_SIZE;
	    hdi.pszText = szDispText;
	    if (SendMessageW(infoPtr->hwndHeader, HDM_GETITEMW, nColumn, (LPARAM)&hdi))
	    {
		HDC hdc = GetDC(infoPtr->hwndSelf);
		HFONT old_font = SelectObject(hdc, (HFONT)SendMessageW(infoPtr->hwndHeader, WM_GETFONT, 0, 0));
		HIMAGELIST himl = (HIMAGELIST)SendMessageW(infoPtr->hwndHeader, HDM_GETIMAGELIST, 0, 0);
		INT bitmap_margin = 0;
		SIZE size;

		if (GetTextExtentPoint32W(hdc, hdi.pszText, lstrlenW(hdi.pszText), &size))
		    cx = size.cx + TRAILING_HEADER_PADDING;

		if (hdi.fmt & (HDF_IMAGE|HDF_BITMAP))
		    bitmap_margin = SendMessageW(infoPtr->hwndHeader, HDM_GETBITMAPMARGIN, 0, 0);

		if ((hdi.fmt & HDF_IMAGE) && himl)
		{
		    INT icon_cx, icon_cy;

		    if (!ImageList_GetIconSize(himl, &icon_cx, &icon_cy))
		        cx += icon_cx + 2*bitmap_margin;
		}
		else if (hdi.fmt & HDF_BITMAP)
		{
		    BITMAP bmp;

		    GetObjectW(hdi.hbm, sizeof(BITMAP), &bmp);
		    cx += bmp.bmWidth + 2*bitmap_margin;
		}

		SelectObject(hdc, old_font);
		ReleaseDC(infoPtr->hwndSelf, hdc);
	    }
	    cx = max (cx, max_cx);
	}
    }

    if (cx < 0) return FALSE;

    /* call header to update the column change */
    hdi.mask = HDI_WIDTH;
    hdi.cxy = max(cx, LISTVIEW_GetColumnInfo(infoPtr, nColumn)->cxMin);
    TRACE("hdi.cxy=%d\n", hdi.cxy);
    return SendMessageW(infoPtr->hwndHeader, HDM_SETITEMW, nColumn, (LPARAM)&hdi);
}

/***
 * Creates the checkbox imagelist.  Helper for LISTVIEW_SetExtendedListViewStyle
 *
 */
static HIMAGELIST LISTVIEW_CreateThemedCheckBoxImageList(const LISTVIEW_INFO *info)
{
    HBITMAP bitmap, old_bitmap;
    HIMAGELIST image_list;
    HDC hdc, mem_hdc;
    HTHEME theme;
    RECT rect;
    SIZE size;

    if (!GetWindowTheme(info->hwndSelf))
        return NULL;

    theme = OpenThemeDataForDpi(NULL, L"Button", GetDpiForWindow(info->hwndSelf));
    if (!theme)
        return NULL;

    hdc = GetDC(info->hwndSelf);
    GetThemePartSize(theme, hdc, BP_CHECKBOX, 0, NULL, TS_DRAW, &size);
    SetRect(&rect, 0, 0, size.cx, size.cy);
    image_list = ImageList_Create(size.cx, size.cy, ILC_COLOR32, 2, 2);
    mem_hdc = CreateCompatibleDC(hdc);
    bitmap = CreateCompatibleBitmap(hdc, size.cx, size.cy);
    old_bitmap = SelectObject(mem_hdc, bitmap);
    ReleaseDC(info->hwndSelf, hdc);

    if (IsThemeBackgroundPartiallyTransparent(theme, BP_CHECKBOX, CBS_UNCHECKEDNORMAL))
        FillRect(mem_hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
    DrawThemeBackground(theme, mem_hdc, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, &rect, NULL);
    ImageList_Add(image_list, bitmap, NULL);

    if (IsThemeBackgroundPartiallyTransparent(theme, BP_CHECKBOX, CBS_CHECKEDNORMAL))
        FillRect(mem_hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
    DrawThemeBackground(theme, mem_hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, &rect, NULL);
    ImageList_Add(image_list, bitmap, NULL);

    SelectObject(mem_hdc, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(mem_hdc);
    CloseThemeData(theme);
    return image_list;
}

static HIMAGELIST LISTVIEW_CreateCheckBoxIL(const LISTVIEW_INFO *infoPtr)
{
    HDC hdc_wnd, hdc;
    HBITMAP hbm_im, hbm_mask, hbm_orig;
    RECT rc;
    HBRUSH hbr_white, hbr_black;
    HIMAGELIST himl;

    himl = LISTVIEW_CreateThemedCheckBoxImageList(infoPtr);
    if (himl)
        return himl;

    hbr_white = GetStockObject(WHITE_BRUSH);
    hbr_black = GetStockObject(BLACK_BRUSH);
    himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                            ILC_COLOR | ILC_MASK, 2, 2);
    hdc_wnd = GetDC(infoPtr->hwndSelf);
    hdc = CreateCompatibleDC(hdc_wnd);
    hbm_im = CreateCompatibleBitmap(hdc_wnd, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    hbm_mask = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 1, NULL);
    ReleaseDC(infoPtr->hwndSelf, hdc_wnd);

    SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    hbm_orig = SelectObject(hdc, hbm_mask);
    FillRect(hdc, &rc, hbr_white);
    InflateRect(&rc, -2, -2);
    FillRect(hdc, &rc, hbr_black);

    SelectObject(hdc, hbm_im);
    DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_MONO);
    SelectObject(hdc, hbm_orig);
    ImageList_Add(himl, hbm_im, hbm_mask); 

    SelectObject(hdc, hbm_im);
    DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_MONO | DFCS_CHECKED);
    SelectObject(hdc, hbm_orig);
    ImageList_Add(himl, hbm_im, hbm_mask);

    DeleteObject(hbm_mask);
    DeleteObject(hbm_im);
    DeleteDC(hdc);

    return himl;
}

/***
 * DESCRIPTION:
 * Sets the extended listview style.
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] dwMask : mask
 * [I] dwStyle : style
 *
 * RETURN:
 *   SUCCESS : previous style
 *   FAILURE : 0
 */
static DWORD LISTVIEW_SetExtendedListViewStyle(LISTVIEW_INFO *infoPtr, DWORD mask, DWORD ex_style)
{
    DWORD old_ex_style = infoPtr->dwLvExStyle;

    TRACE("mask %#lx, ex_style %#lx\n", mask, ex_style);

    /* set new style */
    if (mask)
	infoPtr->dwLvExStyle = (old_ex_style & ~mask) | (ex_style & mask);
    else
	infoPtr->dwLvExStyle = ex_style;

    if((infoPtr->dwLvExStyle ^ old_ex_style) & LVS_EX_CHECKBOXES)
    {
        HIMAGELIST himl = 0;
        if(infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES)
        {
            LVITEMW item;
            item.mask = LVIF_STATE;
            item.stateMask = LVIS_STATEIMAGEMASK;
            item.state = INDEXTOSTATEIMAGEMASK(1);
            LISTVIEW_SetItemState(infoPtr, -1, &item);

            himl = LISTVIEW_CreateCheckBoxIL(infoPtr);
            if(!(infoPtr->dwStyle & LVS_SHAREIMAGELISTS))
                ImageList_Destroy(infoPtr->himlState);
        }
        himl = LISTVIEW_SetImageList(infoPtr, LVSIL_STATE, himl);
        /*   checkbox list replaces previous custom list or... */
        if(((infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES) &&
           !(infoPtr->dwStyle & LVS_SHAREIMAGELISTS)) ||
            /* ...previous was checkbox list */
            (old_ex_style & LVS_EX_CHECKBOXES))
            ImageList_Destroy(himl);
    }

    if((infoPtr->dwLvExStyle ^ old_ex_style) & LVS_EX_HEADERDRAGDROP)
    {
        DWORD style;

        /* if not already created */
        LISTVIEW_CreateHeader(infoPtr);

        style = GetWindowLongW(infoPtr->hwndHeader, GWL_STYLE);
        if (infoPtr->dwLvExStyle & LVS_EX_HEADERDRAGDROP)
            style |= HDS_DRAGDROP;
        else
            style &= ~HDS_DRAGDROP;
        SetWindowLongW(infoPtr->hwndHeader, GWL_STYLE, style);
    }

    /* GRIDLINES adds decoration at top so changes sizes */
    if((infoPtr->dwLvExStyle ^ old_ex_style) & LVS_EX_GRIDLINES)
    {
        LISTVIEW_CreateHeader(infoPtr);
        LISTVIEW_UpdateSize(infoPtr);
    }

    if((infoPtr->dwLvExStyle ^ old_ex_style) & LVS_EX_FULLROWSELECT)
    {
        LISTVIEW_CreateHeader(infoPtr);
    }

    if((infoPtr->dwLvExStyle ^ old_ex_style) & LVS_EX_TRANSPARENTBKGND)
    {
        if (infoPtr->dwLvExStyle & LVS_EX_TRANSPARENTBKGND)
            LISTVIEW_SetBkColor(infoPtr, CLR_NONE);
    }

    if((infoPtr->dwLvExStyle ^ old_ex_style) & LVS_EX_HEADERINALLVIEWS)
    {
        if (infoPtr->dwLvExStyle & LVS_EX_HEADERINALLVIEWS)
            LISTVIEW_CreateHeader(infoPtr);
        else
            ShowWindow(infoPtr->hwndHeader, SW_HIDE);
        LISTVIEW_UpdateSize(infoPtr);
        LISTVIEW_UpdateScroll(infoPtr);
    }

    LISTVIEW_InvalidateList(infoPtr);
    return old_ex_style;
}

/***
 * DESCRIPTION:
 * Sets the new hot cursor used during hot tracking and hover selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hCursor : the new hot cursor handle
 *
 * RETURN:
 * Returns the previous hot cursor
 */
static HCURSOR LISTVIEW_SetHotCursor(LISTVIEW_INFO *infoPtr, HCURSOR hCursor)
{
    HCURSOR oldCursor = infoPtr->hHotCursor;
    
    infoPtr->hHotCursor = hCursor;

    return oldCursor;
}


/***
 * DESCRIPTION:
 * Sets the hot item index.
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] iIndex : index
 *
 * RETURN:
 *   SUCCESS : previous hot item index
 *   FAILURE : -1 (no hot item)
 */
static INT LISTVIEW_SetHotItem(LISTVIEW_INFO *infoPtr, INT iIndex)
{
    INT iOldIndex = infoPtr->nHotItem;
    
    infoPtr->nHotItem = iIndex;
    
    return iOldIndex;
}


/***
 * DESCRIPTION:
 * Sets the amount of time the cursor must hover over an item before it is selected.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] dwHoverTime : hover time, if -1 the hover time is set to the default
 *
 * RETURN:
 * Returns the previous hover time
 */
static DWORD LISTVIEW_SetHoverTime(LISTVIEW_INFO *infoPtr, DWORD dwHoverTime)
{
    DWORD oldHoverTime = infoPtr->dwHoverTime;
    
    infoPtr->dwHoverTime = dwHoverTime;
    
    return oldHoverTime;
}

/***
 * DESCRIPTION:
 * Sets spacing for icons of LVS_ICON style.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] cx : horizontal spacing (-1 = system spacing, 0 = autosize)
 * [I] cy : vertical spacing (-1 = system spacing, 0 = autosize)
 *
 * RETURN:
 *   MAKELONG(oldcx, oldcy)
 */
static DWORD LISTVIEW_SetIconSpacing(LISTVIEW_INFO *infoPtr, INT cx, INT cy)
{
    INT iconWidth = 0, iconHeight = 0;
    DWORD oldspacing = MAKELONG(infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy);

    TRACE("requested=(%d,%d)\n", cx, cy);

    /* set to defaults, if instructed to */
    if (cx == -1 && cy == -1)
    {
        infoPtr->autoSpacing = TRUE;
        if (infoPtr->himlNormal)
            ImageList_GetIconSize(infoPtr->himlNormal, &iconWidth, &iconHeight);
        cx = GetSystemMetrics(SM_CXICONSPACING) - GetSystemMetrics(SM_CXICON) + iconWidth;
        cy = GetSystemMetrics(SM_CYICONSPACING) - GetSystemMetrics(SM_CYICON) + iconHeight;
    }
    else
        infoPtr->autoSpacing = FALSE;

    /* if 0 then keep width */
    if (cx != 0)
        infoPtr->iconSpacing.cx = cx;

    /* if 0 then keep height */
    if (cy != 0)
        infoPtr->iconSpacing.cy = cy;

    TRACE("old=(%d,%d), new=(%ld,%ld), iconSize=(%ld,%ld), ntmH=%d\n",
          LOWORD(oldspacing), HIWORD(oldspacing), infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy,
	  infoPtr->iconSize.cx, infoPtr->iconSize.cy,
	  infoPtr->ntmHeight);

    /* these depend on the iconSpacing */
    LISTVIEW_UpdateItemSize(infoPtr);

    return oldspacing;
}

static inline void set_icon_size(SIZE *size, HIMAGELIST himl, BOOL is_small)
{
    INT cx, cy;
    
    if (himl && ImageList_GetIconSize(himl, &cx, &cy))
    {
	size->cx = cx;
	size->cy = cy;
    }
    else
    {
        size->cx = GetSystemMetrics(is_small ? SM_CXSMICON : SM_CXICON);
        size->cy = GetSystemMetrics(is_small ? SM_CYSMICON : SM_CYICON);
    }
}

/***
 * DESCRIPTION:
 * Sets image lists.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nType : image list type
 * [I] himl : image list handle
 *
 * RETURN:
 *   SUCCESS : old image list
 *   FAILURE : NULL
 */
static HIMAGELIST LISTVIEW_SetImageList(LISTVIEW_INFO *infoPtr, INT nType, HIMAGELIST himl)
{
    INT oldHeight = infoPtr->nItemHeight;
    HIMAGELIST himlOld = 0;

    TRACE("(nType=%d, himl=%p)\n", nType, himl);

    switch (nType)
    {
    case LVSIL_NORMAL:
        himlOld = infoPtr->himlNormal;
        infoPtr->himlNormal = himl;
        if (infoPtr->uView == LV_VIEW_ICON) set_icon_size(&infoPtr->iconSize, himl, FALSE);
        if (infoPtr->autoSpacing)
            LISTVIEW_SetIconSpacing(infoPtr, -1, -1);
    break;

    case LVSIL_SMALL:
        himlOld = infoPtr->himlSmall;
        infoPtr->himlSmall = himl;
        if (infoPtr->uView != LV_VIEW_ICON) set_icon_size(&infoPtr->iconSize, himl, TRUE);
        if (infoPtr->hwndHeader)
            SendMessageW(infoPtr->hwndHeader, HDM_SETIMAGELIST, 0, (LPARAM)himl);
    break;

    case LVSIL_STATE:
        himlOld = infoPtr->himlState;
        infoPtr->himlState = himl;
        set_icon_size(&infoPtr->iconStateSize, himl, TRUE);
        ImageList_SetBkColor(infoPtr->himlState, CLR_NONE);
    break;

    default:
        ERR("Unknown icon type=%d\n", nType);
	return NULL;
    }

    infoPtr->nItemHeight = LISTVIEW_CalculateItemHeight(infoPtr);
    if (infoPtr->nItemHeight != oldHeight)
        LISTVIEW_UpdateScroll(infoPtr);

    return himlOld;
}

/***
 * DESCRIPTION:
 * Preallocates memory (does *not* set the actual count of items !)
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItems : item count (projected number of items to allocate)
 * [I] dwFlags : update flags
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemCount(LISTVIEW_INFO *infoPtr, INT nItems, DWORD dwFlags)
{
    TRACE("nItems %d, flags %#lx\n", nItems, dwFlags);

    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	INT nOldCount = infoPtr->nItemCount;
	infoPtr->nItemCount = nItems;

	if (nItems < nOldCount)
	{
	    RANGE range = { nItems, nOldCount };
	    ranges_del(infoPtr->selectionRanges, range);
	    if (infoPtr->nFocusedItem >= nItems)
	    {
		LISTVIEW_SetItemFocus(infoPtr, -1);
                infoPtr->nFocusedItem = -1;
		SetRectEmpty(&infoPtr->rcFocus);
	    }
	}

	LISTVIEW_UpdateScroll(infoPtr);

	/* the flags are valid only in ownerdata report and list modes */
	if (infoPtr->uView == LV_VIEW_ICON || infoPtr->uView == LV_VIEW_SMALLICON) dwFlags = 0;

	if (!(dwFlags & LVSICF_NOSCROLL) && infoPtr->nFocusedItem != -1)
	    LISTVIEW_EnsureVisible(infoPtr, infoPtr->nFocusedItem, FALSE);

	if (!(dwFlags & LVSICF_NOINVALIDATEALL))
	    LISTVIEW_InvalidateList(infoPtr);
	else
	{
	    INT nFrom, nTo;
	    POINT Origin;
	    RECT rcErase;
	    
	    LISTVIEW_GetOrigin(infoPtr, &Origin);
    	    nFrom = min(nOldCount, nItems);
	    nTo = max(nOldCount, nItems);
    
	    if (infoPtr->uView == LV_VIEW_DETAILS)
	    {
                SetRect(&rcErase, 0, nFrom * infoPtr->nItemHeight, infoPtr->nItemWidth,
                        nTo * infoPtr->nItemHeight);
		OffsetRect(&rcErase, Origin.x, Origin.y);
		if (IntersectRect(&rcErase, &rcErase, &infoPtr->rcList))
		    LISTVIEW_InvalidateRect(infoPtr, &rcErase);
	    }
	    else /* LV_VIEW_LIST */
	    {
		INT nPerCol = LISTVIEW_GetCountPerColumn(infoPtr);

		rcErase.left = (nFrom / nPerCol) * infoPtr->nItemWidth;
		rcErase.top = (nFrom % nPerCol) * infoPtr->nItemHeight;
		rcErase.right = rcErase.left + infoPtr->nItemWidth;
		rcErase.bottom = nPerCol * infoPtr->nItemHeight;
		OffsetRect(&rcErase, Origin.x, Origin.y);
		if (IntersectRect(&rcErase, &rcErase, &infoPtr->rcList))
		    LISTVIEW_InvalidateRect(infoPtr, &rcErase);

		rcErase.left = (nFrom / nPerCol + 1) * infoPtr->nItemWidth;
		rcErase.top = 0;
		rcErase.right = (nTo / nPerCol + 1) * infoPtr->nItemWidth;
		rcErase.bottom = nPerCol * infoPtr->nItemHeight;
		OffsetRect(&rcErase, Origin.x, Origin.y);
		if (IntersectRect(&rcErase, &rcErase, &infoPtr->rcList))
		    LISTVIEW_InvalidateRect(infoPtr, &rcErase);
	    }
	}
    }
    else
    {
	/* According to MSDN for non-LVS_OWNERDATA this is just
	 * a performance issue. The control allocates its internal
	 * data structures for the number of items specified. It
	 * cuts down on the number of memory allocations. Therefore
	 * we will just issue a WARN here
	 */
	WARN("for non-ownerdata performance option not implemented.\n");
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the position of an item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] pt : coordinate
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemPosition(LISTVIEW_INFO *infoPtr, INT nItem, const POINT *pt)
{
    POINT Origin, Pt;

    TRACE("(nItem=%d, pt=%s)\n", nItem, wine_dbgstr_point(pt));

    if (!pt || nItem < 0 || nItem >= infoPtr->nItemCount ||
	!(infoPtr->uView == LV_VIEW_ICON || infoPtr->uView == LV_VIEW_SMALLICON)) return FALSE;

    Pt = *pt;
    LISTVIEW_GetOrigin(infoPtr, &Origin);

    /* This point value seems to be an undocumented feature.
     * The best guess is that it means either at the origin, 
     * or at true beginning of the list. I will assume the origin. */
    if ((Pt.x == -1) && (Pt.y == -1))
	Pt = Origin;
    
    if (infoPtr->uView == LV_VIEW_ICON)
    {
	Pt.x -= (infoPtr->nItemWidth - infoPtr->iconSize.cx) / 2;
	Pt.y -= ICON_TOP_PADDING;
    }
    Pt.x -= Origin.x;
    Pt.y -= Origin.y;

    return LISTVIEW_MoveIconTo(infoPtr, nItem, &Pt, FALSE);
}

/* Make sure to also disable per item notifications via the notification mask. */
static VOID LISTVIEW_SetOwnerDataState(LISTVIEW_INFO *infoPtr, INT nFirst, INT nLast, const LVITEMW *item)
{
    NMLVODSTATECHANGE nmlv;

    if (nFirst == nLast) return;
    if (!item) return;

    ZeroMemory(&nmlv, sizeof(nmlv));
    nmlv.iFrom = nFirst;
    nmlv.iTo = nLast;
    nmlv.uOldState = 0;
    nmlv.uNewState = item->state;

    notify_hdr(infoPtr, LVN_ODSTATECHANGED, (LPNMHDR)&nmlv);
}

/***
 * DESCRIPTION:
 * Sets the state of one or many items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] item  : item or subitem info
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemState(LISTVIEW_INFO *infoPtr, INT nItem, const LVITEMW *item)
{
    BOOL ret = TRUE;
    LVITEMW lvItem;

    if (!item) return FALSE;

    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_STATE;
    lvItem.state = item->state;
    lvItem.stateMask = item->stateMask;
    TRACE("item=%s\n", debuglvitem_t(&lvItem, TRUE));

    if (nItem == -1)
    {
        UINT oldstate = 0;
        DWORD old_mask;

        /* special case optimization for recurring attempt to deselect all */
        if (lvItem.state == 0 && lvItem.stateMask == LVIS_SELECTED && !LISTVIEW_GetSelectedCount(infoPtr))
            return TRUE;

	/* select all isn't allowed in LVS_SINGLESEL */
	if ((lvItem.state & lvItem.stateMask & LVIS_SELECTED) && (infoPtr->dwStyle & LVS_SINGLESEL))
	    return FALSE;

	/* focus all isn't allowed */
	if (lvItem.state & lvItem.stateMask & LVIS_FOCUSED) return FALSE;

        old_mask = infoPtr->notify_mask & NOTIFY_MASK_ITEM_CHANGE;
        if (infoPtr->dwStyle & LVS_OWNERDATA)
        {
            infoPtr->notify_mask &= ~NOTIFY_MASK_ITEM_CHANGE;
            if (!(lvItem.state & LVIS_SELECTED) && LISTVIEW_GetSelectedCount(infoPtr))
                oldstate |= LVIS_SELECTED;
            if (infoPtr->nFocusedItem != -1) oldstate |= LVIS_FOCUSED;
        }

    	/* apply to all items */
    	for (lvItem.iItem = 0; lvItem.iItem < infoPtr->nItemCount; lvItem.iItem++)
	    if (!LISTVIEW_SetItemT(infoPtr, &lvItem, TRUE)) ret = FALSE;

        if (infoPtr->dwStyle & LVS_OWNERDATA)
        {
            NMLISTVIEW nmlv;

            infoPtr->notify_mask |= old_mask;

            nmlv.iItem = -1;
            nmlv.iSubItem = 0;
            nmlv.uNewState = lvItem.state & lvItem.stateMask;
            nmlv.uOldState = oldstate & lvItem.stateMask;
            nmlv.uChanged = LVIF_STATE;
            nmlv.ptAction.x = nmlv.ptAction.y = 0;
            nmlv.lParam = 0;

            notify_listview(infoPtr, LVN_ITEMCHANGED, &nmlv);
        }
    }
    else
	ret = LISTVIEW_SetItemT(infoPtr, &lvItem, TRUE);

    return ret;
}

/***
 * DESCRIPTION:
 * Sets the text of an item or subitem.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nItem : item index
 * [I] lpLVItem : item or subitem info
 * [I] isW : TRUE if input is Unicode
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemTextT(LISTVIEW_INFO *infoPtr, INT nItem, const LVITEMW *lpLVItem, BOOL isW)
{
    LVITEMW lvItem;

    if (!lpLVItem || nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;
    if (infoPtr->dwStyle & LVS_OWNERDATA) return FALSE;

    lvItem.iItem = nItem;
    lvItem.iSubItem = lpLVItem->iSubItem;
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = lpLVItem->pszText;
    lvItem.cchTextMax = lpLVItem->cchTextMax;
    
    TRACE("(nItem=%d, lpLVItem=%s, isW=%d)\n", nItem, debuglvitem_t(&lvItem, isW), isW);

    return LISTVIEW_SetItemT(infoPtr, &lvItem, isW); 
}

/***
 * DESCRIPTION:
 * Set item index that marks the start of a multiple selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nIndex : index
 *
 * RETURN:
 * Index number or -1 if there is no selection mark.
 */
static INT LISTVIEW_SetSelectionMark(LISTVIEW_INFO *infoPtr, INT nIndex)
{
  INT nOldIndex = infoPtr->nSelectionMark;

  TRACE("(nIndex=%d)\n", nIndex);

  if (nIndex >= -1 && nIndex < infoPtr->nItemCount)
    infoPtr->nSelectionMark = nIndex;

  return nOldIndex;
}

/***
 * DESCRIPTION:
 * Sets the text background color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] color   : text background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetTextBkColor(LISTVIEW_INFO *infoPtr, COLORREF color)
{
    TRACE("color %#lx\n", color);

    infoPtr->clrTextBk = color;
    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the text foreground color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] color   : text color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetTextColor (LISTVIEW_INFO *infoPtr, COLORREF color)
{
    TRACE("color %#lx\n", color);

    infoPtr->clrText = color;
    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets new ToolTip window to ListView control.
 *
 * PARAMETER(S):
 * [I] infoPtr        : valid pointer to the listview structure
 * [I] hwndNewToolTip : handle to new ToolTip
 *
 * RETURN:
 *   old tool tip
 */
static HWND LISTVIEW_SetToolTips( LISTVIEW_INFO *infoPtr, HWND hwndNewToolTip)
{
  HWND hwndOldToolTip = infoPtr->hwndToolTip;
  infoPtr->hwndToolTip = hwndNewToolTip;
  return hwndOldToolTip;
}

/*
 * DESCRIPTION:
 *   sets the Unicode character format flag for the control
 * PARAMETER(S):
 *    [I] infoPtr         :valid pointer to the listview structure
 *    [I] fUnicode        :true to switch to UNICODE false to switch to ANSI
 *
 * RETURN:
 *    Old Unicode Format
 */
static BOOL LISTVIEW_SetUnicodeFormat( LISTVIEW_INFO *infoPtr, BOOL unicode)
{
  SHORT rc = infoPtr->notifyFormat;
  infoPtr->notifyFormat = (unicode) ? NFR_UNICODE : NFR_ANSI;
  return rc == NFR_UNICODE;
}

/*
 * DESCRIPTION:
 *   sets the control view mode
 * PARAMETER(S):
 *    [I] infoPtr         :valid pointer to the listview structure
 *    [I] nView           :new view mode value
 *
 * RETURN:
 *    SUCCESS:  1
 *    FAILURE: -1
 */
static INT LISTVIEW_SetView(LISTVIEW_INFO *infoPtr, DWORD nView)
{
  HIMAGELIST himl;

  if (infoPtr->uView == nView) return 1;

  if ((INT)nView < 0 || nView > LV_VIEW_MAX) return -1;
  if (nView == LV_VIEW_TILE)
  {
      FIXME("View LV_VIEW_TILE unimplemented\n");
      return -1;
  }

  infoPtr->uView = nView;

  SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);
  ShowWindow(infoPtr->hwndHeader, SW_HIDE);

  ShowScrollBar(infoPtr->hwndSelf, SB_BOTH, FALSE);
  SetRectEmpty(&infoPtr->rcFocus);

  himl = (nView == LV_VIEW_ICON ? infoPtr->himlNormal : infoPtr->himlSmall);
  set_icon_size(&infoPtr->iconSize, himl, nView != LV_VIEW_ICON);

  switch (nView)
  {
  case LV_VIEW_ICON:
  case LV_VIEW_SMALLICON:
      LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);
      break;
  case LV_VIEW_DETAILS:
  {
      HDLAYOUT hl;
      WINDOWPOS wp;

      LISTVIEW_CreateHeader( infoPtr );

      hl.prc = &infoPtr->rcList;
      hl.pwpos = &wp;
      SendMessageW(infoPtr->hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hl);
      SetWindowPos(infoPtr->hwndHeader, infoPtr->hwndSelf, wp.x, wp.y, wp.cx, wp.cy,
                   wp.flags | ((infoPtr->dwStyle & LVS_NOCOLUMNHEADER) ? SWP_HIDEWINDOW : SWP_SHOWWINDOW));
      break;
  }
  case LV_VIEW_LIST:
      break;
  }

  LISTVIEW_UpdateItemSize(infoPtr);
  LISTVIEW_UpdateSize(infoPtr);
  LISTVIEW_UpdateScroll(infoPtr);
  LISTVIEW_InvalidateList(infoPtr);

  TRACE("nView %ld\n", nView);

  return 1;
}

/* LISTVIEW_SetWorkAreas */

struct sorting_context
{
    HDPA items;
    PFNLVCOMPARE compare_func;
    LPARAM lParam;
};

/* DPA_Sort() callback used for LVM_SORTITEMS */
static INT WINAPI LISTVIEW_CallBackCompare(LPVOID first, LPVOID second, LPARAM lParam)
{
    struct sorting_context *context = (struct sorting_context *)lParam;
    ITEM_INFO* lv_first = DPA_GetPtr( first, 0 );
    ITEM_INFO* lv_second = DPA_GetPtr( second, 0 );

    return context->compare_func(lv_first->lParam, lv_second->lParam, context->lParam);
}

/* DPA_Sort() callback used for LVM_SORTITEMSEX */
static INT WINAPI LISTVIEW_CallBackCompareEx(LPVOID first, LPVOID second, LPARAM lParam)
{
    struct sorting_context *context = (struct sorting_context *)lParam;
    INT first_idx  = DPA_GetPtrIndex( context->items, first  );
    INT second_idx = DPA_GetPtrIndex( context->items, second );

    return context->compare_func(first_idx, second_idx, context->lParam);
}

/***
 * DESCRIPTION:
 * Sorts the listview items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] pfnCompare : application-defined value
 * [I] lParamSort : pointer to comparison callback
 * [I] IsEx : TRUE when LVM_SORTITEMSEX used
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SortItems(LISTVIEW_INFO *infoPtr, PFNLVCOMPARE pfnCompare,
                               LPARAM lParamSort, BOOL IsEx)
{
    HDPA hdpaSubItems, hdpaItems;
    ITEM_INFO *lpItem;
    LPVOID selectionMarkItem = NULL;
    LPVOID focusedItem = NULL;
    struct sorting_context context;
    int i;

    TRACE("pfnCompare %p, lParamSort %Ix\n", pfnCompare, lParamSort);

    if (infoPtr->dwStyle & LVS_OWNERDATA) return FALSE;

    if (!pfnCompare) return FALSE;
    if (!infoPtr->hdpaItems) return FALSE;

    /* if there are 0 or 1 items, there is no need to sort */
    if (infoPtr->nItemCount < 2) return TRUE;
    if (!(hdpaItems = DPA_Clone(infoPtr->hdpaItems, NULL))) return FALSE;

    /* clear selection */
    ranges_clear(infoPtr->selectionRanges);

    /* save selection mark and focused item */
    if (infoPtr->nSelectionMark >= 0)
        selectionMarkItem = DPA_GetPtr(infoPtr->hdpaItems, infoPtr->nSelectionMark);
    if (infoPtr->nFocusedItem >= 0)
        focusedItem = DPA_GetPtr(infoPtr->hdpaItems, infoPtr->nFocusedItem);

    context.items = hdpaItems;
    context.compare_func = pfnCompare;
    context.lParam = lParamSort;
    if (IsEx)
        DPA_Sort(hdpaItems, LISTVIEW_CallBackCompareEx, (LPARAM)&context);
    else
        DPA_Sort(hdpaItems, LISTVIEW_CallBackCompare, (LPARAM)&context);
    DPA_Destroy(infoPtr->hdpaItems);
    infoPtr->hdpaItems = hdpaItems;

    /* restore selection ranges */
    for (i=0; i < infoPtr->nItemCount; i++)
    {
        hdpaSubItems = DPA_GetPtr(infoPtr->hdpaItems, i);
        lpItem = DPA_GetPtr(hdpaSubItems, 0);

	if (lpItem->state & LVIS_SELECTED)
	    ranges_additem(infoPtr->selectionRanges, i);
    }
    /* restore selection mark and focused item */
    infoPtr->nSelectionMark = DPA_GetPtrIndex(infoPtr->hdpaItems, selectionMarkItem);
    infoPtr->nFocusedItem   = DPA_GetPtrIndex(infoPtr->hdpaItems, focusedItem);

    /* I believe nHotItem should be left alone, see LISTVIEW_ShiftIndices */

    /* refresh the display */
    LISTVIEW_InvalidateList(infoPtr);
    return TRUE;
}

/***
 * DESCRIPTION:
 * Update theme handle after a theme change.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *   SUCCESS : 0
 *   FAILURE : something else
 */
static LRESULT LISTVIEW_ThemeChanged(const LISTVIEW_INFO *infoPtr)
{
    HTHEME theme = GetWindowTheme(infoPtr->hwndSelf);
    CloseThemeData(theme);
    OpenThemeData(infoPtr->hwndSelf, themeClass);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 0;
}

/***
 * DESCRIPTION:
 * Updates an items or rearranges the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_Update(LISTVIEW_INFO *infoPtr, INT nItem)
{
    TRACE("(nItem=%d)\n", nItem);

    if (nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;

    /* rearrange with default alignment style */
    if (is_autoarrange(infoPtr))
	LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);
    else
	LISTVIEW_InvalidateItem(infoPtr, nItem);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Draw the track line at the place defined in the infoPtr structure.
 * The line is drawn with a XOR pen so drawing the line for the second time
 * in the same place erases the line.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_DrawTrackLine(const LISTVIEW_INFO *infoPtr)
{
    HDC hdc;

    if (infoPtr->xTrackLine == -1)
        return FALSE;

    if (!(hdc = GetDC(infoPtr->hwndSelf)))
        return FALSE;
    PatBlt( hdc, infoPtr->xTrackLine, infoPtr->rcList.top,
            1, infoPtr->rcList.bottom - infoPtr->rcList.top, DSTINVERT );
    ReleaseDC(infoPtr->hwndSelf, hdc);
    return TRUE;
}

/***
 * DESCRIPTION:
 * Called when an edit control should be displayed. This function is called after
 * we are sure that there was a single click - not a double click (this is a TIMERPROC).
 *
 * PARAMETER(S):
 * [I] hwnd : Handle to the listview
 * [I] uMsg : WM_TIMER (ignored)
 * [I] idEvent : The timer ID interpreted as a pointer to a DELAYED_EDIT_ITEM struct
 * [I] dwTimer : The elapsed time (ignored)
 *
 * RETURN:
 *   None.
 */
static VOID CALLBACK LISTVIEW_DelayedEditItem(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    DELAYED_ITEM_EDIT *editItem = (DELAYED_ITEM_EDIT *)idEvent;
    LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongPtrW(hwnd, 0);

    KillTimer(hwnd, idEvent);
    editItem->fEnabled = FALSE;
    /* check if the item is still selected */
    if (infoPtr->bFocus && LISTVIEW_GetItemState(infoPtr, editItem->iItem, LVIS_SELECTED))
        LISTVIEW_EditLabelT(infoPtr, editItem->iItem, TRUE);
}

/***
 * DESCRIPTION:
 * Creates the listview control - the WM_NCCREATE phase.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] lpcs : the create parameters
 *
 * RETURN:
 *   Success: TRUE
 *   Failure: FALSE
 */
static LRESULT LISTVIEW_NCCreate(HWND hwnd, WPARAM wParam, const CREATESTRUCTW *lpcs)
{
  LISTVIEW_INFO *infoPtr;
  LOGFONTW logFont;

  TRACE("(lpcs=%p)\n", lpcs);

  /* initialize info pointer */
  infoPtr = Alloc(sizeof(*infoPtr));
  if (!infoPtr) return FALSE;

  SetWindowLongPtrW(hwnd, 0, (DWORD_PTR)infoPtr);

  infoPtr->hwndSelf = hwnd;
  infoPtr->dwStyle = lpcs->style;    /* Note: may be changed in WM_CREATE */
  map_style_view(infoPtr);
  /* determine the type of structures to use */
  infoPtr->hwndNotify = lpcs->hwndParent;
  /* infoPtr->notifyFormat will be filled in WM_CREATE */

  /* initialize color information  */
  infoPtr->clrBk = CLR_NONE;
  infoPtr->clrText = CLR_DEFAULT;
  infoPtr->clrTextBk = CLR_DEFAULT;
  LISTVIEW_SetBkColor(infoPtr, comctl32_color.clrWindow);

  /* set default values */
  infoPtr->nFocusedItem = -1;
  infoPtr->nSelectionMark = -1;
  infoPtr->nHotItem = -1;
  infoPtr->redraw = TRUE;
  infoPtr->bNoItemMetrics = TRUE;
  infoPtr->notify_mask = NOTIFY_MASK_UNMASK_ALL;
  infoPtr->autoSpacing = TRUE;
  infoPtr->iconSpacing.cx = GetSystemMetrics(SM_CXICONSPACING) - GetSystemMetrics(SM_CXICON);
  infoPtr->iconSpacing.cy = GetSystemMetrics(SM_CYICONSPACING) - GetSystemMetrics(SM_CYICON);
  infoPtr->nEditLabelItem = -1;
  infoPtr->nLButtonDownItem = -1;
  infoPtr->dwHoverTime = HOVER_DEFAULT; /* default system hover time */
  infoPtr->cWheelRemainder = 0;
  infoPtr->nMeasureItemHeight = 0;
  infoPtr->xTrackLine = -1;  /* no track line */
  infoPtr->itemEdit.fEnabled = FALSE;
  infoPtr->iVersion = COMCTL32_VERSION;
  infoPtr->colRectsDirty = FALSE;
  infoPtr->selected_column = -1;

  /* get default font (icon title) */
  SystemParametersInfoW(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
  infoPtr->hDefaultFont = CreateFontIndirectW(&logFont);
  infoPtr->hFont = infoPtr->hDefaultFont;
  LISTVIEW_SaveTextMetrics(infoPtr);

  /* allocate memory for the data structure */
  if (!(infoPtr->selectionRanges = ranges_create(10))) goto fail;
  if (!(infoPtr->hdpaItems = DPA_Create(10))) goto fail;
  if (!(infoPtr->hdpaItemIds = DPA_Create(10))) goto fail;
  if (!(infoPtr->hdpaPosX  = DPA_Create(10))) goto fail;
  if (!(infoPtr->hdpaPosY  = DPA_Create(10))) goto fail;
  if (!(infoPtr->hdpaColumns = DPA_Create(10))) goto fail;

  return DefWindowProcW(hwnd, WM_NCCREATE, wParam, (LPARAM)lpcs);

fail:
    DestroyWindow(infoPtr->hwndHeader);
    ranges_destroy(infoPtr->selectionRanges);
    DPA_Destroy(infoPtr->hdpaItems);
    DPA_Destroy(infoPtr->hdpaItemIds);
    DPA_Destroy(infoPtr->hdpaPosX);
    DPA_Destroy(infoPtr->hdpaPosY);
    DPA_Destroy(infoPtr->hdpaColumns);
    Free(infoPtr);
    return FALSE;
}

/***
 * DESCRIPTION:
 * Creates the listview control - the WM_CREATE phase. Most of the data is
 * already set up in LISTVIEW_NCCreate
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] lpcs : the create parameters
 *
 * RETURN:
 *   Success: 0
 *   Failure: -1
 */
static LRESULT LISTVIEW_Create(HWND hwnd, const CREATESTRUCTW *lpcs)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongPtrW(hwnd, 0);

  TRACE("lpcs %p, style %#lx\n", lpcs, lpcs->style);

  infoPtr->dwStyle = lpcs->style;
  map_style_view(infoPtr);

  infoPtr->notifyFormat = SendMessageW(infoPtr->hwndNotify, WM_NOTIFYFORMAT,
                                       (WPARAM)infoPtr->hwndSelf, NF_QUERY);
  /* on error defaulting to ANSI notifications */
  if (infoPtr->notifyFormat == 0) infoPtr->notifyFormat = NFR_ANSI;
  TRACE("notify format=%d\n", infoPtr->notifyFormat);

  if ((infoPtr->uView == LV_VIEW_DETAILS) && (lpcs->style & WS_VISIBLE))
  {
    if (LISTVIEW_CreateHeader(infoPtr) < 0)  return -1;
  }
  else
    infoPtr->hwndHeader = 0;

  /* init item size to avoid division by 0 */
  LISTVIEW_UpdateItemSize (infoPtr);
  LISTVIEW_UpdateSize (infoPtr);

  if (infoPtr->uView == LV_VIEW_DETAILS)
  {
    if (!(LVS_NOCOLUMNHEADER & lpcs->style) && (WS_VISIBLE & lpcs->style))
    {
      ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
    }
    LISTVIEW_UpdateScroll(infoPtr);
    /* send WM_MEASUREITEM notification */
    if (infoPtr->dwStyle & LVS_OWNERDRAWFIXED) notify_measureitem(infoPtr);
  }

  OpenThemeData(hwnd, themeClass);

  /* initialize the icon sizes */
  set_icon_size(&infoPtr->iconSize, infoPtr->himlNormal, infoPtr->uView != LV_VIEW_ICON);
  set_icon_size(&infoPtr->iconStateSize, infoPtr->himlState, TRUE);
  return 0;
}

/***
 * DESCRIPTION:
 * Destroys the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *   Success: 0
 *   Failure: -1
 */
static LRESULT LISTVIEW_Destroy(LISTVIEW_INFO *infoPtr)
{
    HTHEME theme = GetWindowTheme(infoPtr->hwndSelf);
    CloseThemeData(theme);

    /* delete all items */
    LISTVIEW_DeleteAllItems(infoPtr, TRUE);

    return 0;
}

/***
 * DESCRIPTION:
 * Enables the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] bEnable : specifies whether to enable or disable the window
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_Enable(const LISTVIEW_INFO *infoPtr)
{
    if (infoPtr->dwStyle & LVS_OWNERDRAWFIXED)
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return TRUE;
}

/***
 * DESCRIPTION:
 * Erases the background of the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static inline BOOL LISTVIEW_EraseBkgnd(const LISTVIEW_INFO *infoPtr, HDC hdc)
{
    RECT rc;

    TRACE("(hdc=%p)\n", hdc);

    if (!GetClipBox(hdc, &rc)) return FALSE;

    if (infoPtr->clrBk == CLR_NONE)
    {
        if (infoPtr->dwLvExStyle & LVS_EX_TRANSPARENTBKGND)
            SendMessageW(infoPtr->hwndNotify, WM_PRINTCLIENT, (WPARAM)hdc, PRF_ERASEBKGND);
        else
            SendMessageW(infoPtr->hwndNotify, WM_ERASEBKGND, (WPARAM)hdc, 0);
        LISTVIEW_DrawBackgroundBitmap(infoPtr, hdc, &rc);
        return TRUE;
    }

    /* for double buffered controls we need to do this during refresh */
    if (infoPtr->dwLvExStyle & LVS_EX_DOUBLEBUFFER) return FALSE;

    return LISTVIEW_FillBkgnd(infoPtr, hdc, &rc);
}
	

/***
 * DESCRIPTION:
 * Helper function for LISTVIEW_[HV]Scroll *only*.
 * Performs vertical/horizontal scrolling by a give amount.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] dx : amount of horizontal scroll
 * [I] dy : amount of vertical scroll
 */
static void scroll_list(LISTVIEW_INFO *infoPtr, INT dx, INT dy)
{
    /* now we can scroll the list */
    ScrollWindowEx(infoPtr->hwndSelf, dx, dy, &infoPtr->rcList, 
		   &infoPtr->rcList, 0, 0, SW_ERASE | SW_INVALIDATE);
    /* if we have focus, adjust rect */
    OffsetRect(&infoPtr->rcFocus, dx, dy);
    UpdateWindow(infoPtr->hwndSelf);
}

/***
 * DESCRIPTION:
 * Performs vertical scrolling.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nScrollCode : scroll code
 * [I] nScrollDiff : units to scroll in SB_INTERNAL mode, 0 otherwise
 * [I] hScrollWnd  : scrollbar control window handle
 *
 * RETURN:
 * Zero
 *
 * NOTES:
 *   SB_LINEUP/SB_LINEDOWN:
 *        for LVS_ICON, LVS_SMALLICON is 37 by experiment
 *        for LVS_REPORT is 1 line
 *        for LVS_LIST cannot occur
 *
 */
static LRESULT LISTVIEW_VScroll(LISTVIEW_INFO *infoPtr, INT nScrollCode, 
				INT nScrollDiff)
{
    INT nOldScrollPos, nNewScrollPos;
    SCROLLINFO scrollInfo;
    BOOL is_an_icon;

    TRACE("(nScrollCode=%d(%s), nScrollDiff=%d)\n", nScrollCode, 
	debugscrollcode(nScrollCode), nScrollDiff);

    if (infoPtr->hwndEdit) SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;

    is_an_icon = ((infoPtr->uView == LV_VIEW_ICON) || (infoPtr->uView == LV_VIEW_SMALLICON));

    if (!GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo)) return 1;

    nOldScrollPos = scrollInfo.nPos;
    switch (nScrollCode)
    {
    case SB_INTERNAL:
        break;

    case SB_LINEUP:
	nScrollDiff = (is_an_icon) ? -LISTVIEW_SCROLL_ICON_LINE_SIZE : -1;
        break;

    case SB_LINEDOWN:
	nScrollDiff = (is_an_icon) ? LISTVIEW_SCROLL_ICON_LINE_SIZE : 1;
        break;

    case SB_PAGEUP:
	nScrollDiff = -scrollInfo.nPage;
        break;

    case SB_PAGEDOWN:
	nScrollDiff = scrollInfo.nPage;
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
	nScrollDiff = scrollInfo.nTrackPos - scrollInfo.nPos;
        break;

    default:
	nScrollDiff = 0;
    }

    /* quit right away if pos isn't changing */
    if (nScrollDiff == 0) return 0;
    
    /* calculate new position, and handle overflows */
    nNewScrollPos = scrollInfo.nPos + nScrollDiff;
    if (nScrollDiff > 0) {
	if (nNewScrollPos < nOldScrollPos ||
	    nNewScrollPos > scrollInfo.nMax)
	    nNewScrollPos = scrollInfo.nMax;
    } else {
	if (nNewScrollPos > nOldScrollPos ||
	    nNewScrollPos < scrollInfo.nMin)
	    nNewScrollPos = scrollInfo.nMin;
    }

    /* set the new position, and reread in case it changed */
    scrollInfo.fMask = SIF_POS;
    scrollInfo.nPos = nNewScrollPos;
    nNewScrollPos = SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo, TRUE);
    
    /* carry on only if it really changed */
    if (nNewScrollPos == nOldScrollPos) return 0;
    
    /* now adjust to client coordinates */
    nScrollDiff = nOldScrollPos - nNewScrollPos;
    if (infoPtr->uView == LV_VIEW_DETAILS) nScrollDiff *= infoPtr->nItemHeight;
   
    /* and scroll the window */ 
    scroll_list(infoPtr, 0, nScrollDiff);

    return 0;
}

/***
 * DESCRIPTION:
 * Performs horizontal scrolling.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nScrollCode : scroll code
 * [I] nScrollDiff : units to scroll in SB_INTERNAL mode, 0 otherwise
 * [I] hScrollWnd  : scrollbar control window handle
 *
 * RETURN:
 * Zero
 *
 * NOTES:
 *   SB_LINELEFT/SB_LINERIGHT:
 *        for LVS_ICON, LVS_SMALLICON  1 pixel
 *        for LVS_REPORT is 1 pixel
 *        for LVS_LIST  is 1 column --> which is a 1 because the
 *                                      scroll is based on columns not pixels
 *
 */
static LRESULT LISTVIEW_HScroll(LISTVIEW_INFO *infoPtr, INT nScrollCode,
                                INT nScrollDiff)
{
    INT nOldScrollPos, nNewScrollPos;
    SCROLLINFO scrollInfo;
    BOOL is_an_icon;

    TRACE("(nScrollCode=%d(%s), nScrollDiff=%d)\n", nScrollCode, 
	debugscrollcode(nScrollCode), nScrollDiff);

    if (infoPtr->hwndEdit) SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;

    is_an_icon = ((infoPtr->uView == LV_VIEW_ICON) || (infoPtr->uView == LV_VIEW_SMALLICON));

    if (!GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo)) return 1;

    nOldScrollPos = scrollInfo.nPos;

    switch (nScrollCode)
    {
    case SB_INTERNAL:
        break;

    case SB_LINELEFT:
	nScrollDiff = (is_an_icon) ? -LISTVIEW_SCROLL_ICON_LINE_SIZE : -1;
        break;

    case SB_LINERIGHT:
	nScrollDiff = (is_an_icon) ? LISTVIEW_SCROLL_ICON_LINE_SIZE : 1;
        break;

    case SB_PAGELEFT:
	nScrollDiff = -scrollInfo.nPage;
        break;

    case SB_PAGERIGHT:
	nScrollDiff = scrollInfo.nPage;
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
	nScrollDiff = scrollInfo.nTrackPos - scrollInfo.nPos;
	break;

    default:
	nScrollDiff = 0;
    }

    /* quit right away if pos isn't changing */
    if (nScrollDiff == 0) return 0;
    
    /* calculate new position, and handle overflows */
    nNewScrollPos = scrollInfo.nPos + nScrollDiff;
    if (nScrollDiff > 0) {
	if (nNewScrollPos < nOldScrollPos ||
	    nNewScrollPos > scrollInfo.nMax)
	    nNewScrollPos = scrollInfo.nMax;
    } else {
	if (nNewScrollPos > nOldScrollPos ||
	    nNewScrollPos < scrollInfo.nMin)
	    nNewScrollPos = scrollInfo.nMin;
    }

    /* set the new position, and reread in case it changed */
    scrollInfo.fMask = SIF_POS;
    scrollInfo.nPos = nNewScrollPos;
    nNewScrollPos = SetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo, TRUE);

    /* carry on only if it really changed */
    if (nNewScrollPos == nOldScrollPos) return 0;

    LISTVIEW_UpdateHeaderSize(infoPtr, nNewScrollPos);

    /* now adjust to client coordinates */
    nScrollDiff = nOldScrollPos - nNewScrollPos;
    if (infoPtr->uView == LV_VIEW_LIST) nScrollDiff *= infoPtr->nItemWidth;

    /* and scroll the window */
    scroll_list(infoPtr, nScrollDiff, 0);

    return 0;
}

static LRESULT LISTVIEW_MouseWheel(LISTVIEW_INFO *infoPtr, INT wheelDelta)
{
    INT pulScrollLines = 3;

    TRACE("(wheelDelta=%d)\n", wheelDelta);

    switch(infoPtr->uView)
    {
    case LV_VIEW_ICON:
    case LV_VIEW_SMALLICON:
       /*
        *  listview should be scrolled by a multiple of 37 dependently on its dimension or its visible item number
        *  should be fixed in the future.
        */
        LISTVIEW_VScroll(infoPtr, SB_INTERNAL, (wheelDelta > 0) ?
                -LISTVIEW_SCROLL_ICON_LINE_SIZE : LISTVIEW_SCROLL_ICON_LINE_SIZE);
        break;

    case LV_VIEW_DETAILS:
        SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);

        /* if scrolling changes direction, ignore left overs */
        if ((wheelDelta < 0 && infoPtr->cWheelRemainder < 0) ||
            (wheelDelta > 0 && infoPtr->cWheelRemainder > 0))
            infoPtr->cWheelRemainder += wheelDelta;
        else
            infoPtr->cWheelRemainder = wheelDelta;
        if (infoPtr->cWheelRemainder && pulScrollLines)
        {
            int cLineScroll;
            pulScrollLines = min((UINT)LISTVIEW_GetCountPerColumn(infoPtr), pulScrollLines);
            cLineScroll = pulScrollLines * infoPtr->cWheelRemainder / WHEEL_DELTA;
            infoPtr->cWheelRemainder -= WHEEL_DELTA * cLineScroll / pulScrollLines;
            LISTVIEW_VScroll(infoPtr, SB_INTERNAL, -cLineScroll);
        }
        break;

    case LV_VIEW_LIST:
        LISTVIEW_HScroll(infoPtr, (wheelDelta > 0) ? SB_LINELEFT : SB_LINERIGHT, 0);
        break;
    }
    return 0;
}

/***
 * DESCRIPTION:
 * ???
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nVirtualKey : virtual key
 * [I] lKeyData : key data
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KeyDown(LISTVIEW_INFO *infoPtr, INT nVirtualKey, LONG lKeyData)
{
  HWND hwndSelf = infoPtr->hwndSelf;
  INT nItem = -1;
  NMLVKEYDOWN nmKeyDown;

  TRACE("(nVirtualKey=%d, lKeyData=%ld)\n", nVirtualKey, lKeyData);

  /* send LVN_KEYDOWN notification */
  nmKeyDown.wVKey = nVirtualKey;
  nmKeyDown.flags = 0;
  notify_hdr(infoPtr, LVN_KEYDOWN, &nmKeyDown.hdr);
  if (!IsWindow(hwndSelf))
    return 0;

  switch (nVirtualKey)
  {
  case VK_SPACE:
    nItem = infoPtr->nFocusedItem;
    if (infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES)
        toggle_checkbox_state(infoPtr, infoPtr->nFocusedItem);
    break;

  case VK_RETURN:
    if ((infoPtr->nItemCount > 0) && (infoPtr->nFocusedItem != -1))
    {
        if (!notify(infoPtr, NM_RETURN)) return 0;
        if (!notify(infoPtr, LVN_ITEMACTIVATE)) return 0;
    }
    break;

  case VK_HOME:
    if (infoPtr->nItemCount > 0)
      nItem = 0;
    break;

  case VK_END:
    if (infoPtr->nItemCount > 0)
      nItem = infoPtr->nItemCount - 1;
    break;

  case VK_LEFT:
    nItem = LISTVIEW_GetNextItem(infoPtr, infoPtr->nFocusedItem, LVNI_TOLEFT);
    break;

  case VK_UP:
    nItem = LISTVIEW_GetNextItem(infoPtr, infoPtr->nFocusedItem, LVNI_ABOVE);
    break;

  case VK_RIGHT:
    nItem = LISTVIEW_GetNextItem(infoPtr, infoPtr->nFocusedItem, LVNI_TORIGHT);
    break;

  case VK_DOWN:
    nItem = LISTVIEW_GetNextItem(infoPtr, infoPtr->nFocusedItem, LVNI_BELOW);
    break;

  case VK_PRIOR:
    if (infoPtr->uView == LV_VIEW_DETAILS)
    {
      INT topidx = LISTVIEW_GetTopIndex(infoPtr);
      if (infoPtr->nFocusedItem == topidx)
        nItem = topidx - LISTVIEW_GetCountPerColumn(infoPtr) + 1;
      else
        nItem = topidx;
    }
    else
      nItem = infoPtr->nFocusedItem - LISTVIEW_GetCountPerColumn(infoPtr)
                                    * LISTVIEW_GetCountPerRow(infoPtr);
    if(nItem < 0) nItem = 0;
    break;

  case VK_NEXT:
    if (infoPtr->uView == LV_VIEW_DETAILS)
    {
      INT topidx = LISTVIEW_GetTopIndex(infoPtr);
      INT cnt = LISTVIEW_GetCountPerColumn(infoPtr);
      if (infoPtr->nFocusedItem == topidx + cnt - 1)
        nItem = infoPtr->nFocusedItem + cnt - 1;
      else
        nItem = topidx + cnt - 1;
    }
    else
      nItem = infoPtr->nFocusedItem + LISTVIEW_GetCountPerColumn(infoPtr)
                                    * LISTVIEW_GetCountPerRow(infoPtr);
    if(nItem >= infoPtr->nItemCount) nItem = infoPtr->nItemCount - 1;
    break;
  }

  if ((nItem != -1) && (nItem != infoPtr->nFocusedItem || nVirtualKey == VK_SPACE))
      LISTVIEW_KeySelection(infoPtr, nItem, nVirtualKey == VK_SPACE);

  return 0;
}

/***
 * DESCRIPTION:
 * Kills the focus.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KillFocus(LISTVIEW_INFO *infoPtr)
{
    TRACE("()\n");

    /* drop any left over scroll amount */
    infoPtr->cWheelRemainder = 0;

    /* if we did not have the focus, there's nothing more to do */
    if (!infoPtr->bFocus) return 0;
   
    /* send NM_KILLFOCUS notification */
    if (!notify(infoPtr, NM_KILLFOCUS)) return 0;

    /* if we have a focus rectangle, get rid of it */
    LISTVIEW_ShowFocusRect(infoPtr, FALSE);

    /* if have a marquee selection, stop it */
    if (infoPtr->bMarqueeSelect)
    {
        /* Remove the marquee rectangle and release our mouse capture */
        LISTVIEW_InvalidateRect(infoPtr, &infoPtr->marqueeRect);
        ReleaseCapture();

        SetRectEmpty(&infoPtr->marqueeRect);

        infoPtr->bMarqueeSelect = FALSE;
        infoPtr->bScrolling = FALSE;
        KillTimer(infoPtr->hwndSelf, (UINT_PTR) infoPtr);
    }

    /* set window focus flag */
    infoPtr->bFocus = FALSE;

    /* invalidate the selected items before resetting focus flag */
    LISTVIEW_InvalidateSelectedItems(infoPtr);
    
    return 0;
}

/***
 * DESCRIPTION:
 * Processes double click messages (left mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] x,y : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonDblClk(LISTVIEW_INFO *infoPtr, WORD wKey, INT x, INT y)
{
    LVHITTESTINFO htInfo;

    TRACE("(key=%hu, X=%u, Y=%u)\n", wKey, x, y);
    
    /* Cancel the item edition if any */
    if (infoPtr->itemEdit.fEnabled)
    {
      KillTimer(infoPtr->hwndSelf, (UINT_PTR)&infoPtr->itemEdit);
      infoPtr->itemEdit.fEnabled = FALSE;
    }

    /* send NM_RELEASEDCAPTURE notification */
    if (!notify(infoPtr, NM_RELEASEDCAPTURE)) return 0;

    htInfo.pt.x = x;
    htInfo.pt.y = y;

    /* send NM_DBLCLK notification */
    LISTVIEW_HitTest(infoPtr, &htInfo, TRUE, FALSE);
    if (!notify_click(infoPtr, NM_DBLCLK, &htInfo)) return 0;

    /* To send the LVN_ITEMACTIVATE, it must be on an Item */
    if(htInfo.iItem != -1) notify_itemactivate(infoPtr,&htInfo);

    return 0;
}

static LRESULT LISTVIEW_TrackMouse(const LISTVIEW_INFO *infoPtr, POINT pt)
{
    MSG msg;
    RECT r;

    r.top = r.bottom = pt.y;
    r.left = r.right = pt.x;

    InflateRect(&r, GetSystemMetrics(SM_CXDRAG), GetSystemMetrics(SM_CYDRAG));

    SetCapture(infoPtr->hwndSelf);

    while (1)
    {
	if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD))
	{
	    if (msg.message == WM_MOUSEMOVE)
	    {
		pt.x = (short)LOWORD(msg.lParam);
		pt.y = (short)HIWORD(msg.lParam);
		if (PtInRect(&r, pt))
		    continue;
		else
		{
		    ReleaseCapture();
		    return 1;
		}
	    }
	    else if (msg.message >= WM_LBUTTONDOWN &&
		     msg.message <= WM_RBUTTONDBLCLK)
	    {
		break;
	    }

	    DispatchMessageW(&msg);
	}

	if (GetCapture() != infoPtr->hwndSelf)
	    return 0;
    }

    ReleaseCapture();
    return 0;
}


/***
 * DESCRIPTION:
 * Processes mouse down messages (left mouse button).
 *
 * PARAMETERS:
 *   infoPtr  [I ] valid pointer to the listview structure
 *   wKey     [I ] key flag
 *   x,y      [I ] mouse coordinate
 *
 * RETURN:
 *   Zero
 */
static LRESULT LISTVIEW_LButtonDown(LISTVIEW_INFO *infoPtr, WORD wKey, INT x, INT y)
{
  LVHITTESTINFO lvHitTestInfo;
  static BOOL bGroupSelect = TRUE;
  POINT pt = { x, y };
  INT nItem;

  TRACE("(key=%hu, X=%u, Y=%u)\n", wKey, x, y);

  /* send NM_RELEASEDCAPTURE notification */
  if (!notify(infoPtr, NM_RELEASEDCAPTURE)) return 0;

  /* set left button down flag and record the click position */
  infoPtr->bLButtonDown = TRUE;
  infoPtr->ptClickPos = pt;
  infoPtr->bDragging = FALSE;
  infoPtr->bMarqueeSelect = FALSE;
  infoPtr->bScrolling = FALSE;

  lvHitTestInfo.pt.x = x;
  lvHitTestInfo.pt.y = y;

  nItem = LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, TRUE);
  TRACE("at %s, nItem=%d\n", wine_dbgstr_point(&pt), nItem);
  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    if ((infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES) && (lvHitTestInfo.flags & LVHT_ONITEMSTATEICON))
    {
        notify_click(infoPtr, NM_CLICK, &lvHitTestInfo);
        toggle_checkbox_state(infoPtr, nItem);
        infoPtr->bLButtonDown = FALSE;
        return 0;
    }

    if (infoPtr->dwStyle & LVS_SINGLESEL)
    {
      if (LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED))
        infoPtr->nEditLabelItem = nItem;
      else
        LISTVIEW_SetSelection(infoPtr, nItem);
    }
    else
    {
      if ((wKey & MK_CONTROL) && (wKey & MK_SHIFT))
      {
        if (bGroupSelect)
	{
          if (!LISTVIEW_AddGroupSelection(infoPtr, nItem)) return 0;
    	  LISTVIEW_SetItemFocus(infoPtr, nItem);
          infoPtr->nSelectionMark = nItem;
	}
        else
	{
          LVITEMW item;

	  item.state = LVIS_SELECTED | LVIS_FOCUSED;
	  item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

	  LISTVIEW_SetItemState(infoPtr,nItem,&item);
	  infoPtr->nSelectionMark = nItem;
	}
      }
      else if (wKey & MK_CONTROL)
      {
        LVITEMW item;

	bGroupSelect = (LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED) == 0);
	
	item.state = (bGroupSelect ? LVIS_SELECTED : 0) | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	LISTVIEW_SetItemState(infoPtr, nItem, &item);
        infoPtr->nSelectionMark = nItem;
      }
      else  if (wKey & MK_SHIFT)
      {
        LISTVIEW_SetGroupSelection(infoPtr, nItem);
      }
      else
      {
	if (LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED))
	{
	  infoPtr->nEditLabelItem = nItem;
	  infoPtr->nLButtonDownItem = nItem;

          LISTVIEW_SetItemFocus(infoPtr, nItem);
	}
	else
	  /* set selection (clears other pre-existing selections) */
	  LISTVIEW_SetSelection(infoPtr, nItem);
      }
    }

    if (!infoPtr->bFocus)
        SetFocus(infoPtr->hwndSelf);

    if (infoPtr->dwLvExStyle & LVS_EX_ONECLICKACTIVATE)
        if(lvHitTestInfo.iItem != -1) notify_itemactivate(infoPtr,&lvHitTestInfo);
  }
  else
  {
    if (!infoPtr->bFocus)
        SetFocus(infoPtr->hwndSelf);

    /* remove all selections */
    if (!(wKey & MK_CONTROL) && !(wKey & MK_SHIFT))
        LISTVIEW_DeselectAll(infoPtr);
    ReleaseCapture();
  }
  
  return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse up messages (left mouse button).
 *
 * PARAMETERS:
 *   infoPtr [I ] valid pointer to the listview structure
 *   wKey    [I ] key flag
 *   x,y     [I ] mouse coordinate
 *
 * RETURN:
 *   Zero
 */
static LRESULT LISTVIEW_LButtonUp(LISTVIEW_INFO *infoPtr, WORD wKey, INT x, INT y)
{
    LVHITTESTINFO lvHitTestInfo;
    
    TRACE("(key=%hu, X=%u, Y=%u)\n", wKey, x, y);

    if (!infoPtr->bLButtonDown) return 0;

    lvHitTestInfo.pt.x = x;
    lvHitTestInfo.pt.y = y;

    /* send NM_CLICK notification */
    LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, FALSE);
    if (!notify_click(infoPtr, NM_CLICK, &lvHitTestInfo)) return 0;

    /* set left button flag */
    infoPtr->bLButtonDown = FALSE;

    /* set a single selection, reset others */
    if(lvHitTestInfo.iItem == infoPtr->nLButtonDownItem && lvHitTestInfo.iItem != -1)
        LISTVIEW_SetSelection(infoPtr, infoPtr->nLButtonDownItem);
    infoPtr->nLButtonDownItem = -1;

    if (infoPtr->bDragging || infoPtr->bMarqueeSelect)
    {
        /* Remove the marquee rectangle and release our mouse capture */
        if (infoPtr->bMarqueeSelect)
        {
            LISTVIEW_InvalidateRect(infoPtr, &infoPtr->marqueeDrawRect);
            ReleaseCapture();
        }

        SetRectEmpty(&infoPtr->marqueeRect);
        SetRectEmpty(&infoPtr->marqueeDrawRect);

        infoPtr->bDragging = FALSE;
        infoPtr->bMarqueeSelect = FALSE;
        infoPtr->bScrolling = FALSE;

        KillTimer(infoPtr->hwndSelf, (UINT_PTR) infoPtr);
        return 0;
    }

    /* if we clicked on a selected item, edit the label */
    if(lvHitTestInfo.iItem == infoPtr->nEditLabelItem && (lvHitTestInfo.flags & LVHT_ONITEMLABEL))
    {
        /* we want to make sure the user doesn't want to do a double click. So we will
         * delay the edit. WM_LBUTTONDBLCLICK will cancel the timer
         */
        infoPtr->itemEdit.fEnabled = TRUE;
        infoPtr->itemEdit.iItem = lvHitTestInfo.iItem;
        SetTimer(infoPtr->hwndSelf,
            (UINT_PTR)&infoPtr->itemEdit,
            GetDoubleClickTime(),
            LISTVIEW_DelayedEditItem);
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Destroys the listview control (called after WM_DESTROY).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NCDestroy(LISTVIEW_INFO *infoPtr)
{
  INT i;

  TRACE("()\n");

  /* destroy data structure */
  DPA_Destroy(infoPtr->hdpaItems);
  DPA_Destroy(infoPtr->hdpaItemIds);
  DPA_Destroy(infoPtr->hdpaPosX);
  DPA_Destroy(infoPtr->hdpaPosY);
  /* columns */
  for (i = 0; i < DPA_GetPtrCount(infoPtr->hdpaColumns); i++)
      Free(DPA_GetPtr(infoPtr->hdpaColumns, i));
  DPA_Destroy(infoPtr->hdpaColumns);
  ranges_destroy(infoPtr->selectionRanges);

  /* destroy image lists */
  if (!(infoPtr->dwStyle & LVS_SHAREIMAGELISTS))
  {
      ImageList_Destroy(infoPtr->himlNormal);
      ImageList_Destroy(infoPtr->himlSmall);
      ImageList_Destroy(infoPtr->himlState);
  }

  /* destroy font, bkgnd brush */
  infoPtr->hFont = 0;
  if (infoPtr->hDefaultFont) DeleteObject(infoPtr->hDefaultFont);
  if (infoPtr->clrBk != CLR_NONE) DeleteObject(infoPtr->hBkBrush);
  if (infoPtr->hBkBitmap) DeleteObject(infoPtr->hBkBitmap);

  SetWindowLongPtrW(infoPtr->hwndSelf, 0, 0);

  Free(infoPtr);

  return 0;
}

/***
 * DESCRIPTION:
 * Handles notifications.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpnmhdr : notification information
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Notify(LISTVIEW_INFO *infoPtr, NMHDR *lpnmhdr)
{
    NMHEADERW *lpnmh;
    
    TRACE("(lpnmhdr=%p)\n", lpnmhdr);

    if (!lpnmhdr || lpnmhdr->hwndFrom != infoPtr->hwndHeader) return 0;

    /* remember: HDN_LAST < HDN_FIRST */
    if (lpnmhdr->code > HDN_FIRST || lpnmhdr->code < HDN_LAST) return 0;
    lpnmh = (NMHEADERW *)lpnmhdr;

    if (lpnmh->iItem < 0 || lpnmh->iItem >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return 0;

    switch (lpnmhdr->code)
    {
	case HDN_TRACKW:
	case HDN_TRACKA:
	{
	    COLUMN_INFO *lpColumnInfo;
	    POINT ptOrigin;
	    INT x;
	    
	    if (!lpnmh->pitem || !(lpnmh->pitem->mask & HDI_WIDTH))
		break;

            /* remove the old line (if any) */
            LISTVIEW_DrawTrackLine(infoPtr);
            
            /* compute & draw the new line */
            lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, lpnmh->iItem);
            x = lpColumnInfo->rcHeader.left + lpnmh->pitem->cxy;
            LISTVIEW_GetOrigin(infoPtr, &ptOrigin);
            infoPtr->xTrackLine = x + ptOrigin.x;
            LISTVIEW_DrawTrackLine(infoPtr);
            return notify_forward_header(infoPtr, lpnmh);
	}

	case HDN_ENDTRACKA:
	case HDN_ENDTRACKW:
	    /* remove the track line (if any) */
	    LISTVIEW_DrawTrackLine(infoPtr);
	    infoPtr->xTrackLine = -1;
            return notify_forward_header(infoPtr, lpnmh);

        case HDN_BEGINDRAG:
            if ((infoPtr->dwLvExStyle & LVS_EX_HEADERDRAGDROP) == 0) return 1;
            return notify_forward_header(infoPtr, lpnmh);

        case HDN_ENDDRAG:
            infoPtr->colRectsDirty = TRUE;
            LISTVIEW_InvalidateList(infoPtr);
            return notify_forward_header(infoPtr, lpnmh);

	case HDN_ITEMCHANGEDW:
	case HDN_ITEMCHANGEDA:
	{
	    COLUMN_INFO *lpColumnInfo;
	    HDITEMW hdi;
	    INT dx, cxy;

	    if (!lpnmh->pitem || !(lpnmh->pitem->mask & HDI_WIDTH))
	    {
		hdi.mask = HDI_WIDTH;
		if (!SendMessageW(infoPtr->hwndHeader, HDM_GETITEMW, lpnmh->iItem, (LPARAM)&hdi)) return 0;
		cxy = hdi.cxy;
	    }
	    else
		cxy = lpnmh->pitem->cxy;
	    
	    /* determine how much we change since the last know position */
	    lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, lpnmh->iItem);
	    dx = cxy - (lpColumnInfo->rcHeader.right - lpColumnInfo->rcHeader.left);
	    if (dx != 0)
	    {
		lpColumnInfo->rcHeader.right += dx;

		hdi.mask = HDI_ORDER;
		SendMessageW(infoPtr->hwndHeader, HDM_GETITEMW, lpnmh->iItem, (LPARAM)&hdi);

		/* not the rightmost one */
		if (hdi.iOrder + 1 < DPA_GetPtrCount(infoPtr->hdpaColumns))
		{
		    INT nIndex = SendMessageW(infoPtr->hwndHeader, HDM_ORDERTOINDEX,
					      hdi.iOrder + 1, 0);
		    LISTVIEW_ScrollColumns(infoPtr, nIndex, dx);
		}
		else
		{
		    /* only needs to update the scrolls */
		    infoPtr->nItemWidth += dx;
		    LISTVIEW_UpdateScroll(infoPtr);
		}
		LISTVIEW_UpdateItemSize(infoPtr);
		if (infoPtr->uView == LV_VIEW_DETAILS && is_redrawing(infoPtr))
		{
		    POINT ptOrigin;
		    RECT rcCol = lpColumnInfo->rcHeader;
		    
		    LISTVIEW_GetOrigin(infoPtr, &ptOrigin);
		    OffsetRect(&rcCol, ptOrigin.x, 0);
		    
		    rcCol.top = infoPtr->rcList.top;
		    rcCol.bottom = infoPtr->rcList.bottom;

		    /* resizing left-aligned columns leaves most of the left side untouched */
		    if ((lpColumnInfo->fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		    {
			INT nMaxDirty = infoPtr->nEllipsisWidth + infoPtr->ntmMaxCharWidth;
			if (dx > 0)
			    nMaxDirty += dx;
			rcCol.left = max (rcCol.left, rcCol.right - nMaxDirty);
		    }

		    /* when shrinking the last column clear the now unused field */
		    if (hdi.iOrder == DPA_GetPtrCount(infoPtr->hdpaColumns) - 1)
		    {
		        RECT right;

		        rcCol.right -= dx;

		        /* deal with right from rightmost column area */
		        right.left = rcCol.right;
		        right.top  = rcCol.top;
		        right.bottom = rcCol.bottom;
		        right.right = infoPtr->rcList.right;

		        LISTVIEW_InvalidateRect(infoPtr, &right);
		    }

		    LISTVIEW_InvalidateRect(infoPtr, &rcCol);
		}
	    }
	    break;
        }

	case HDN_ITEMCLICKW:
	case HDN_ITEMCLICKA:
	{
            /* Handle sorting by Header Column */
            NMLISTVIEW nmlv;

            ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
            nmlv.iItem = -1;
            nmlv.iSubItem = lpnmh->iItem;
            notify_listview(infoPtr, LVN_COLUMNCLICK, &nmlv);
            return notify_forward_header(infoPtr, lpnmh);
        }

	case HDN_DIVIDERDBLCLICKW:
	case HDN_DIVIDERDBLCLICKA:
            /* FIXME: for LVS_EX_HEADERINALLVIEWS and not LV_VIEW_DETAILS
                      we should use LVSCW_AUTOSIZE_USEHEADER, helper rework or
                      split needed for that */
            LISTVIEW_SetColumnWidth(infoPtr, lpnmh->iItem, LVSCW_AUTOSIZE);
            return notify_forward_header(infoPtr, lpnmh);
    }
    return 0;
}

/***
 * DESCRIPTION:
 * Paint non-client area of control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structureof the sender
 * [I] region : update region
 *
 * RETURN:
 *  0  - frame was painted
 */
static LRESULT LISTVIEW_NCPaint(const LISTVIEW_INFO *infoPtr, HRGN region)
{
    LONG exstyle = GetWindowLongW (infoPtr->hwndSelf, GWL_EXSTYLE);
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    RECT r, window_rect;
    HDC dc;
    HRGN cliprgn;
    int cxEdge = GetSystemMetrics (SM_CXEDGE),
        cyEdge = GetSystemMetrics (SM_CYEDGE);

    if (!theme || !(exstyle & WS_EX_CLIENTEDGE))
       return DefWindowProcW (infoPtr->hwndSelf, WM_NCPAINT, (WPARAM)region, 0);

    GetWindowRect(infoPtr->hwndSelf, &r);

    cliprgn = CreateRectRgn (r.left + cxEdge, r.top + cyEdge,
        r.right - cxEdge, r.bottom - cyEdge);
    if (region != (HRGN)1)
        CombineRgn (cliprgn, cliprgn, region, RGN_AND);

    dc = GetDCEx(infoPtr->hwndSelf, region, DCX_WINDOW | DCX_INTERSECTRGN);
    if (infoPtr->hwndHeader && LISTVIEW_IsHeaderEnabled(infoPtr))
    {
        GetWindowRect(infoPtr->hwndHeader, &window_rect);
        OffsetRect(&window_rect, -r.left, -r.top);
        ExcludeClipRect(dc, window_rect.left, window_rect.top, window_rect.right, window_rect.bottom);
    }

    OffsetRect(&r, -r.left, -r.top);
    if (IsThemeBackgroundPartiallyTransparent (theme, 0, 0))
        DrawThemeParentBackground(infoPtr->hwndSelf, dc, &r);
    DrawThemeBackground (theme, dc, 0, 0, &r, 0);
    ReleaseDC(infoPtr->hwndSelf, dc);

    /* Call default proc to get the scrollbars etc. painted */
    DefWindowProcW (infoPtr->hwndSelf, WM_NCPAINT, (WPARAM)cliprgn, 0);
    DeleteObject(cliprgn);

    return 0;
}

/***
 * DESCRIPTION:
 * Determines the type of structure to use.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structureof the sender
 * [I] hwndFrom : listview window handle
 * [I] nCommand : command specifying the nature of the WM_NOTIFYFORMAT
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NotifyFormat(LISTVIEW_INFO *infoPtr, HWND hwndFrom, INT nCommand)
{
    TRACE("(hwndFrom=%p, nCommand=%d)\n", hwndFrom, nCommand);

    if (nCommand == NF_REQUERY)
        infoPtr->notifyFormat = SendMessageW(infoPtr->hwndNotify, WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);

    return infoPtr->notifyFormat;
}

/***
 * DESCRIPTION:
 * Paints/Repaints the listview control. Internal use.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Paint(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    TRACE("(hdc=%p)\n", hdc);

    if (infoPtr->bNoItemMetrics && infoPtr->nItemCount)
    {
	infoPtr->bNoItemMetrics = FALSE;
	LISTVIEW_UpdateItemSize(infoPtr);
	if (infoPtr->uView == LV_VIEW_ICON || infoPtr->uView == LV_VIEW_SMALLICON)
	    LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);
	LISTVIEW_UpdateScroll(infoPtr);
    }

    if (infoPtr->hwndHeader)  UpdateWindow(infoPtr->hwndHeader);

    if (hdc) 
        LISTVIEW_Refresh(infoPtr, hdc, NULL);
    else
    {
	PAINTSTRUCT ps;

	hdc = BeginPaint(infoPtr->hwndSelf, &ps);
	if (!hdc) return 1;
	LISTVIEW_Refresh(infoPtr, hdc, ps.fErase ? &ps.rcPaint : NULL);
	EndPaint(infoPtr->hwndSelf, &ps);
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Paints/Repaints the listview control, WM_PAINT handler.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 *
 * RETURN:
 * Zero
 */
static inline LRESULT LISTVIEW_WMPaint(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    TRACE("(hdc=%p)\n", hdc);

    if (!is_redrawing(infoPtr))
        return DefWindowProcW (infoPtr->hwndSelf, WM_PAINT, (WPARAM)hdc, 0);

    return LISTVIEW_Paint(infoPtr, hdc);
}

/***
 * DESCRIPTION:
 * Paints/Repaints the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] options : drawing options
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_PrintClient(LISTVIEW_INFO *infoPtr, HDC hdc, DWORD options)
{
    FIXME("(hdc=%p options=%#lx) partial stub\n", hdc, options);

    LISTVIEW_Paint(infoPtr, hdc);

    return 0;
}


/***
 * DESCRIPTION:
 * Processes double click messages (right mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] x,y : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDblClk(const LISTVIEW_INFO *infoPtr, WORD wKey, INT x, INT y)
{
    LVHITTESTINFO lvHitTestInfo;
    
    TRACE("(key=%hu,X=%u,Y=%u)\n", wKey, x, y);

    /* send NM_RELEASEDCAPTURE notification */
    if (!notify(infoPtr, NM_RELEASEDCAPTURE)) return 0;

    /* send NM_RDBLCLK notification */
    lvHitTestInfo.pt.x = x;
    lvHitTestInfo.pt.y = y;
    LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, FALSE);
    notify_click(infoPtr, NM_RDBLCLK, &lvHitTestInfo);

    return 0;
}

/***
 * DESCRIPTION:
 * Processes WM_RBUTTONDOWN message and corresponding drag operation.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] x, y : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDown(LISTVIEW_INFO *infoPtr, WORD wKey, INT x, INT y)
{
    LVHITTESTINFO ht;
    INT item;

    TRACE("(key=%hu, x=%d, y=%d)\n", wKey, x, y);

    /* send NM_RELEASEDCAPTURE notification */
    if (!notify(infoPtr, NM_RELEASEDCAPTURE)) return 0;

    /* determine the index of the selected item */
    ht.pt.x = x;
    ht.pt.y = y;
    item = LISTVIEW_HitTest(infoPtr, &ht, TRUE, TRUE);

    /* make sure the listview control window has the focus */
    if (!infoPtr->bFocus) SetFocus(infoPtr->hwndSelf);

    if ((item >= 0) && (item < infoPtr->nItemCount))
    {
	LISTVIEW_SetItemFocus(infoPtr, item);
	if (!((wKey & MK_SHIFT) || (wKey & MK_CONTROL)) &&
            !LISTVIEW_GetItemState(infoPtr, item, LVIS_SELECTED))
	    LISTVIEW_SetSelection(infoPtr, item);
    }
    else
	LISTVIEW_DeselectAll(infoPtr);

    if (LISTVIEW_TrackMouse(infoPtr, ht.pt))
    {
	if (ht.iItem != -1)
	{
            NMLISTVIEW nmlv;

            memset(&nmlv, 0, sizeof(nmlv));
            nmlv.iItem = ht.iItem;
            nmlv.ptAction = ht.pt;

            notify_listview(infoPtr, LVN_BEGINRDRAG, &nmlv);
	}
    }
    else
    {
	SetFocus(infoPtr->hwndSelf);

        ht.pt.x = x;
        ht.pt.y = y;
        LISTVIEW_HitTest(infoPtr, &ht, TRUE, FALSE);

	if (notify_click(infoPtr, NM_RCLICK, &ht))
	{
	    /* Send a WM_CONTEXTMENU message in response to the WM_RBUTTONUP */
	    SendMessageW(infoPtr->hwndSelf, WM_CONTEXTMENU,
		(WPARAM)infoPtr->hwndSelf, (LPARAM)GetMessagePos());
	}
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Sets the cursor.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hwnd : window handle of window containing the cursor
 * [I] nHittest : hit-test code
 * [I] wMouseMsg : ideintifier of the mouse message
 *
 * RETURN:
 * TRUE if cursor is set
 * FALSE otherwise
 */
static BOOL LISTVIEW_SetCursor(const LISTVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LVHITTESTINFO lvHitTestInfo;

    if (!LISTVIEW_IsHotTracking(infoPtr)) goto forward;

    if (!infoPtr->hHotCursor) goto forward;

    GetCursorPos(&lvHitTestInfo.pt);
    if (LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, FALSE, FALSE) < 0) goto forward;

    SetCursor(infoPtr->hHotCursor);

    return TRUE;

forward:

    return DefWindowProcW(infoPtr->hwndSelf, WM_SETCURSOR, wParam, lParam);
}

/***
 * DESCRIPTION:
 * Sets the focus.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hwndLoseFocus : handle of previously focused window
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_SetFocus(LISTVIEW_INFO *infoPtr, HWND hwndLoseFocus)
{
    TRACE("(hwndLoseFocus=%p)\n", hwndLoseFocus);

    /* if we have the focus already, there's nothing to do */
    if (infoPtr->bFocus) return 0;
   
    /* send NM_SETFOCUS notification */
    if (!notify(infoPtr, NM_SETFOCUS)) return 0;

    /* set window focus flag */
    infoPtr->bFocus = TRUE;

    /* put the focus rect back on */
    LISTVIEW_ShowFocusRect(infoPtr, TRUE);

    /* redraw all visible selected items */
    LISTVIEW_InvalidateSelectedItems(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Sets the font.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fRedraw : font handle
 * [I] fRedraw : redraw flag
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_SetFont(LISTVIEW_INFO *infoPtr, HFONT hFont, WORD fRedraw)
{
    HFONT oldFont = infoPtr->hFont;
    INT oldHeight = infoPtr->nItemHeight;

    TRACE("(hfont=%p,redraw=%hu)\n", hFont, fRedraw);

    infoPtr->hFont = hFont ? hFont : infoPtr->hDefaultFont;
    if (infoPtr->hFont == oldFont) return 0;
    
    LISTVIEW_SaveTextMetrics(infoPtr);

    infoPtr->nItemHeight = LISTVIEW_CalculateItemHeight(infoPtr);

    if (infoPtr->uView == LV_VIEW_DETAILS)
    {
	SendMessageW(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(fRedraw, 0));
        LISTVIEW_UpdateSize(infoPtr);
        LISTVIEW_UpdateScroll(infoPtr);
    }
    else if (infoPtr->nItemHeight != oldHeight)
        LISTVIEW_UpdateScroll(infoPtr);

    if (fRedraw) LISTVIEW_InvalidateList(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Message handling for WM_SETREDRAW.
 * For the Listview, it invalidates the entire window (the doc specifies otherwise)
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] redraw: state of redraw flag
 *
 * RETURN:
 *     Zero.
 */
static LRESULT LISTVIEW_SetRedraw(LISTVIEW_INFO *infoPtr, BOOL redraw)
{
    TRACE("old=%d, new=%d\n", infoPtr->redraw, redraw);

    if (infoPtr->redraw == !!redraw)
        return 0;

    if (!(infoPtr->redraw = !!redraw))
        return 0;

    if (is_autoarrange(infoPtr))
	LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);
    LISTVIEW_UpdateScroll(infoPtr);

    /* despite what the WM_SETREDRAW docs says, apps expect us
     * to invalidate the listview here... stupid! */
    LISTVIEW_InvalidateList(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Resizes the listview control. This function processes WM_SIZE
 * messages.  At this time, the width and height are not used.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] Width : new width
 * [I] Height : new height
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Size(LISTVIEW_INFO *infoPtr, int Width, int Height)
{
    RECT rcOld = infoPtr->rcList;

    TRACE("(width=%d, height=%d)\n", Width, Height);

    LISTVIEW_UpdateSize(infoPtr);
    if (EqualRect(&rcOld, &infoPtr->rcList)) return 0;
  
    /* do not bother with display related stuff if we're not redrawing */ 
    if (!is_redrawing(infoPtr)) return 0;
    
    if (is_autoarrange(infoPtr)) 
	LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);

    LISTVIEW_UpdateScroll(infoPtr);

    /* refresh all only for lists whose height changed significantly */
    if ((infoPtr->uView == LV_VIEW_LIST) &&
	(rcOld.bottom - rcOld.top) / infoPtr->nItemHeight !=
	(infoPtr->rcList.bottom - infoPtr->rcList.top) / infoPtr->nItemHeight)
	LISTVIEW_InvalidateList(infoPtr);

  return 0;
}

/***
 * DESCRIPTION:
 * Sets the size information.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *  None
 */
static void LISTVIEW_UpdateSize(LISTVIEW_INFO *infoPtr)
{
    TRACE("uView %ld, rcList(old)=%s\n", infoPtr->uView, wine_dbgstr_rect(&infoPtr->rcList));

    GetClientRect(infoPtr->hwndSelf, &infoPtr->rcList);

    if (infoPtr->uView == LV_VIEW_LIST)
    {
	/* Apparently the "LIST" style is supposed to have the same
	 * number of items in a column even if there is no scroll bar.
	 * Since if a scroll bar already exists then the bottom is already
	 * reduced, only reduce if the scroll bar does not currently exist.
	 * The "2" is there to mimic the native control. I think it may be
	 * related to either padding or edges.  (GLA 7/2002)
	 */
	if (!(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & WS_HSCROLL))
	    infoPtr->rcList.bottom -= GetSystemMetrics(SM_CYHSCROLL);
        infoPtr->rcList.bottom = max (infoPtr->rcList.bottom - 2, 0);
    }

    /* When ListView control is created invisible, header isn't created right away. */
    if (infoPtr->hwndHeader)
    {
        POINT origin;
        WINDOWPOS wp;
        HDLAYOUT hl;
        RECT rect;

        LISTVIEW_GetOrigin(infoPtr, &origin);

        rect = infoPtr->rcList;
        rect.left += origin.x;

        hl.prc = &rect;
	hl.pwpos = &wp;
	SendMessageW( infoPtr->hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hl );
	TRACE("  wp.flags=0x%08x, wp=%d,%d (%dx%d)\n", wp.flags, wp.x, wp.y, wp.cx, wp.cy);

	if (LISTVIEW_IsHeaderEnabled(infoPtr))
	    wp.flags |= SWP_SHOWWINDOW;
	else
	{
	    wp.flags |= SWP_HIDEWINDOW;
	    wp.cy = 0;
	}

	SetWindowPos(infoPtr->hwndHeader, wp.hwndInsertAfter, wp.x, wp.y, wp.cx, wp.cy, wp.flags);
	TRACE("  after SWP wp=%d,%d (%dx%d)\n", wp.x, wp.y, wp.cx, wp.cy);

	infoPtr->rcList.top = max(wp.cy, 0);
    }
    /* extra padding for grid */
    if (infoPtr->uView == LV_VIEW_DETAILS && infoPtr->dwLvExStyle & LVS_EX_GRIDLINES)
	infoPtr->rcList.top += 2;

    TRACE("  rcList=%s\n", wine_dbgstr_rect(&infoPtr->rcList));
}

/***
 * DESCRIPTION:
 * Processes WM_STYLECHANGED messages.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wStyleType : window style type (normal or extended)
 * [I] lpss : window style information
 *
 * RETURN:
 * Zero
 */
static INT LISTVIEW_StyleChanged(LISTVIEW_INFO *infoPtr, WPARAM wStyleType,
                                 const STYLESTRUCT *lpss)
{
    UINT uNewView, uOldView;
    BOOL repaint = FALSE;
    UINT style;

    TRACE("styletype %Ix, styleOld %#lx, styleNew %#lx\n",
          wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE || lpss->styleNew == infoPtr->dwStyle) return 0;

    infoPtr->dwStyle = lpss->styleNew;

    if (((lpss->styleOld & WS_HSCROLL) != 0)&&
        ((lpss->styleNew & WS_HSCROLL) == 0))
       ShowScrollBar(infoPtr->hwndSelf, SB_HORZ, FALSE);

    if (((lpss->styleOld & WS_VSCROLL) != 0)&&
        ((lpss->styleNew & WS_VSCROLL) == 0))
       ShowScrollBar(infoPtr->hwndSelf, SB_VERT, FALSE);

    uNewView = lpss->styleNew & LVS_TYPEMASK;
    uOldView = lpss->styleOld & LVS_TYPEMASK;

    if (uNewView != uOldView)
    {
    	HIMAGELIST himl;

        repaint = TRUE;

        /* LVM_SETVIEW doesn't change window style bits within LVS_TYPEMASK,
           changing style updates current view only when view bits change. */
        map_style_view(infoPtr);
        SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);
    	ShowWindow(infoPtr->hwndHeader, SW_HIDE);

        ShowScrollBar(infoPtr->hwndSelf, SB_BOTH, FALSE);
        SetRectEmpty(&infoPtr->rcFocus);

        himl = (uNewView == LVS_ICON ? infoPtr->himlNormal : infoPtr->himlSmall);
        set_icon_size(&infoPtr->iconSize, himl, uNewView != LVS_ICON);

        if (uNewView == LVS_REPORT)
        {
            HDLAYOUT hl;
            WINDOWPOS wp;

            LISTVIEW_CreateHeader( infoPtr );

            hl.prc = &infoPtr->rcList;
            hl.pwpos = &wp;
            SendMessageW( infoPtr->hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hl );
            SetWindowPos(infoPtr->hwndHeader, infoPtr->hwndSelf, wp.x, wp.y, wp.cx, wp.cy,
                    wp.flags | ((infoPtr->dwStyle & LVS_NOCOLUMNHEADER)
                        ? SWP_HIDEWINDOW : SWP_SHOWWINDOW));
        }

	LISTVIEW_UpdateItemSize(infoPtr);
    }

    if (uNewView == LVS_REPORT || infoPtr->dwLvExStyle & LVS_EX_HEADERINALLVIEWS)
    {
        if ((lpss->styleOld ^ lpss->styleNew) & LVS_NOCOLUMNHEADER)
        {
            if (lpss->styleNew & LVS_NOCOLUMNHEADER)
            {
                /* Turn off the header control */
                style = GetWindowLongW(infoPtr->hwndHeader, GWL_STYLE);
                TRACE("Hide header control, was 0x%08x\n", style);
                SetWindowLongW(infoPtr->hwndHeader, GWL_STYLE, style | HDS_HIDDEN);
            } else {
                /* Turn on the header control */
                if ((style = GetWindowLongW(infoPtr->hwndHeader, GWL_STYLE)) & HDS_HIDDEN)
                {
                    TRACE("Show header control, was 0x%08x\n", style);
                    SetWindowLongW(infoPtr->hwndHeader, GWL_STYLE, (style & ~HDS_HIDDEN) | WS_VISIBLE);
                }
            }
        }
    }

    if ( (uNewView == LVS_ICON || uNewView == LVS_SMALLICON) &&
	 (uNewView != uOldView || ((lpss->styleNew ^ lpss->styleOld) & LVS_ALIGNMASK)) )
	 LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);

    /* update the size of the client area */
    LISTVIEW_UpdateSize(infoPtr);

    /* add scrollbars if needed */
    LISTVIEW_UpdateScroll(infoPtr);

    if (repaint)
        LISTVIEW_InvalidateList(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Processes WM_STYLECHANGING messages.
 *
 * PARAMETER(S):
 * [I] wStyleType : window style type (normal or extended)
 * [I0] lpss : window style information
 *
 * RETURN:
 * Zero
 */
static INT LISTVIEW_StyleChanging(WPARAM wStyleType,
                                  STYLESTRUCT *lpss)
{
    TRACE("styletype %Ix, styleOld %#lx, styleNew %#lx\n",
          wStyleType, lpss->styleOld, lpss->styleNew);

    /* don't forward LVS_OWNERDATA only if not already set to */
    if ((lpss->styleNew ^ lpss->styleOld) & LVS_OWNERDATA)
    {
        if (lpss->styleOld & LVS_OWNERDATA)
            lpss->styleNew |= LVS_OWNERDATA;
        else
            lpss->styleNew &= ~LVS_OWNERDATA;
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Processes WM_SHOWWINDOW messages.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] bShown  : window is being shown (FALSE when hidden)
 * [I] iStatus : window show status
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_ShowWindow(LISTVIEW_INFO *infoPtr, WPARAM bShown, LPARAM iStatus)
{
  /* header delayed creation */
  if ((infoPtr->uView == LV_VIEW_DETAILS) && bShown)
  {
    LISTVIEW_CreateHeader(infoPtr);

    if (!(LVS_NOCOLUMNHEADER & infoPtr->dwStyle))
      ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
  }

  return DefWindowProcW(infoPtr->hwndSelf, WM_SHOWWINDOW, bShown, iStatus);
}

/***
 * DESCRIPTION:
 * Processes CCM_GETVERSION messages.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Current version
 */
static inline LRESULT LISTVIEW_GetVersion(const LISTVIEW_INFO *infoPtr)
{
  return infoPtr->iVersion;
}

/***
 * DESCRIPTION:
 * Processes CCM_SETVERSION messages.
 *
 * PARAMETER(S):
 * [I] infoPtr  : valid pointer to the listview structure
 * [I] iVersion : version to be set
 *
 * RETURN:
 * -1 when requested version is greater than DLL version;
 * previous version otherwise
 */
static LRESULT LISTVIEW_SetVersion(LISTVIEW_INFO *infoPtr, DWORD iVersion)
{
  INT iOldVersion = infoPtr->iVersion;

  if (iVersion > COMCTL32_VERSION)
    return -1;

  infoPtr->iVersion = iVersion;

  TRACE("new version %ld\n", iVersion);

  return iOldVersion;
}

/***
 * DESCRIPTION:
 * Window procedure of the listview control.
 *
 */
static LRESULT WINAPI
LISTVIEW_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongPtrW(hwnd, 0);

  TRACE("hwnd %p, uMsg %x, wParam %Ix, lParam %Ix\n", hwnd, uMsg, wParam, lParam);

  if (!infoPtr && (uMsg != WM_NCCREATE))
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);

  switch (uMsg)
  {
  case LVM_APPROXIMATEVIEWRECT:
    return LISTVIEW_ApproximateViewRect(infoPtr, (INT)wParam,
                                        LOWORD(lParam), HIWORD(lParam));
  case LVM_ARRANGE:
    return LISTVIEW_Arrange(infoPtr, (INT)wParam);

  case LVM_CANCELEDITLABEL:
    return LISTVIEW_CancelEditLabel(infoPtr);

  case LVM_CREATEDRAGIMAGE:
    return (LRESULT)LISTVIEW_CreateDragImage(infoPtr, (INT)wParam, (LPPOINT)lParam);

  case LVM_DELETEALLITEMS:
    return LISTVIEW_DeleteAllItems(infoPtr, FALSE);

  case LVM_DELETECOLUMN:
    return LISTVIEW_DeleteColumn(infoPtr, (INT)wParam);

  case LVM_DELETEITEM:
    return LISTVIEW_DeleteItem(infoPtr, (INT)wParam);

  case LVM_EDITLABELA:
  case LVM_EDITLABELW:
    return (LRESULT)LISTVIEW_EditLabelT(infoPtr, (INT)wParam,
                                        uMsg == LVM_EDITLABELW);
  /* case LVM_ENABLEGROUPVIEW: */

  case LVM_ENSUREVISIBLE:
    return LISTVIEW_EnsureVisible(infoPtr, (INT)wParam, (BOOL)lParam);

  case LVM_FINDITEMW:
    return LISTVIEW_FindItemW(infoPtr, (INT)wParam, (LPLVFINDINFOW)lParam);

  case LVM_FINDITEMA:
    return LISTVIEW_FindItemA(infoPtr, (INT)wParam, (LPLVFINDINFOA)lParam);

  case LVM_GETBKCOLOR:
    return infoPtr->clrBk;

  /* case LVM_GETBKIMAGE: */

  case LVM_GETCALLBACKMASK:
    return infoPtr->uCallbackMask;

  case LVM_GETCOLUMNA:
  case LVM_GETCOLUMNW:
    return LISTVIEW_GetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam,
                               uMsg == LVM_GETCOLUMNW);

  case LVM_GETCOLUMNORDERARRAY:
    return LISTVIEW_GetColumnOrderArray(infoPtr, (INT)wParam, (LPINT)lParam);

  case LVM_GETCOLUMNWIDTH:
    return LISTVIEW_GetColumnWidth(infoPtr, (INT)wParam);

  case LVM_GETCOUNTPERPAGE:
    return LISTVIEW_GetCountPerPage(infoPtr);

  case LVM_GETEDITCONTROL:
    return (LRESULT)infoPtr->hwndEdit;

  case LVM_GETEXTENDEDLISTVIEWSTYLE:
    return infoPtr->dwLvExStyle;

  /* case LVM_GETGROUPINFO: */

  /* case LVM_GETGROUPMETRICS: */

  case LVM_GETHEADER:
    return (LRESULT)infoPtr->hwndHeader;

  case LVM_GETHOTCURSOR:
    return (LRESULT)infoPtr->hHotCursor;

  case LVM_GETHOTITEM:
    return infoPtr->nHotItem;

  case LVM_GETHOVERTIME:
    return infoPtr->dwHoverTime;

  case LVM_GETIMAGELIST:
    return (LRESULT)LISTVIEW_GetImageList(infoPtr, (INT)wParam);

  /* case LVM_GETINSERTMARK: */

  /* case LVM_GETINSERTMARKCOLOR: */

  /* case LVM_GETINSERTMARKRECT: */

  case LVM_GETISEARCHSTRINGA:
  case LVM_GETISEARCHSTRINGW:
    FIXME("LVM_GETISEARCHSTRING: unimplemented\n");
    return FALSE;

  case LVM_GETITEMA:
  case LVM_GETITEMW:
    return LISTVIEW_GetItemExtT(infoPtr, (LPLVITEMW)lParam, uMsg == LVM_GETITEMW);

  case LVM_GETITEMCOUNT:
    return infoPtr->nItemCount;

  case LVM_GETITEMPOSITION:
    return LISTVIEW_GetItemPosition(infoPtr, (INT)wParam, (LPPOINT)lParam);

  case LVM_GETITEMRECT:
    return LISTVIEW_GetItemRect(infoPtr, (INT)wParam, (LPRECT)lParam);

  case LVM_GETITEMSPACING:
    return LISTVIEW_GetItemSpacing(infoPtr, (BOOL)wParam);

  case LVM_GETITEMSTATE:
    return LISTVIEW_GetItemState(infoPtr, (INT)wParam, (UINT)lParam);

  case LVM_GETITEMTEXTA:
  case LVM_GETITEMTEXTW:
    return LISTVIEW_GetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam,
                                 uMsg == LVM_GETITEMTEXTW);

  case LVM_GETNEXTITEM:
    return LISTVIEW_GetNextItem(infoPtr, (INT)wParam, LOWORD(lParam));

  case LVM_GETNEXTITEMINDEX:
    return LISTVIEW_GetNextItemIndex(infoPtr, (LVITEMINDEX *)wParam, lParam);

  case LVM_GETNUMBEROFWORKAREAS:
    FIXME("LVM_GETNUMBEROFWORKAREAS: unimplemented\n");
    return 1;

  case LVM_GETORIGIN:
    if (!lParam) return FALSE;
    if (infoPtr->uView == LV_VIEW_DETAILS ||
        infoPtr->uView == LV_VIEW_LIST) return FALSE;
    LISTVIEW_GetOrigin(infoPtr, (LPPOINT)lParam);
    return TRUE;

  /* case LVM_GETOUTLINECOLOR: */

  case LVM_GETSELECTEDCOLUMN:
    return infoPtr->selected_column;

  case LVM_GETSELECTEDCOUNT:
    return LISTVIEW_GetSelectedCount(infoPtr);

  case LVM_GETSELECTIONMARK:
    return infoPtr->nSelectionMark;

  case LVM_GETSTRINGWIDTHA:
  case LVM_GETSTRINGWIDTHW:
    return LISTVIEW_GetStringWidthT(infoPtr, (LPCWSTR)lParam,
                                    uMsg == LVM_GETSTRINGWIDTHW);

  case LVM_GETSUBITEMRECT:
    return LISTVIEW_GetSubItemRect(infoPtr, (UINT)wParam, (LPRECT)lParam);

  case LVM_GETTEXTBKCOLOR:
    return infoPtr->clrTextBk;

  case LVM_GETTEXTCOLOR:
    return infoPtr->clrText;

  /* case LVM_GETTILEINFO: */

  /* case LVM_GETTILEVIEWINFO: */

  case LVM_GETTOOLTIPS:
    if( !infoPtr->hwndToolTip )
        infoPtr->hwndToolTip = COMCTL32_CreateToolTip( hwnd );
    return (LRESULT)infoPtr->hwndToolTip;

  case LVM_GETTOPINDEX:
    return LISTVIEW_GetTopIndex(infoPtr);

  case LVM_GETUNICODEFORMAT:
    return (infoPtr->notifyFormat == NFR_UNICODE);

  case LVM_GETVIEW:
    return infoPtr->uView;

  case LVM_GETVIEWRECT:
    return LISTVIEW_GetViewRect(infoPtr, (LPRECT)lParam);

  case LVM_GETWORKAREAS:
    FIXME("LVM_GETWORKAREAS: unimplemented\n");
    return FALSE;

  /* case LVM_HASGROUP: */

  case LVM_HITTEST:
    return LISTVIEW_HitTest(infoPtr, (LPLVHITTESTINFO)lParam, FALSE, TRUE);

  case LVM_INSERTCOLUMNA:
  case LVM_INSERTCOLUMNW:
    return LISTVIEW_InsertColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam,
                                  uMsg == LVM_INSERTCOLUMNW);

  /* case LVM_INSERTGROUP: */

  /* case LVM_INSERTGROUPSORTED: */

  case LVM_INSERTITEMA:
  case LVM_INSERTITEMW:
    return LISTVIEW_InsertItemT(infoPtr, (LPLVITEMW)lParam, uMsg == LVM_INSERTITEMW);

  /* case LVM_INSERTMARKHITTEST: */

  /* case LVM_ISGROUPVIEWENABLED: */

  case LVM_ISITEMVISIBLE:
    return LISTVIEW_IsItemVisible(infoPtr, (INT)wParam);

  case LVM_MAPIDTOINDEX:
    return LISTVIEW_MapIdToIndex(infoPtr, (UINT)wParam);

  case LVM_MAPINDEXTOID:
    return LISTVIEW_MapIndexToId(infoPtr, (INT)wParam);

  /* case LVM_MOVEGROUP: */

  /* case LVM_MOVEITEMTOGROUP: */

  case LVM_REDRAWITEMS:
    return LISTVIEW_RedrawItems(infoPtr, (INT)wParam, (INT)lParam);

  /* case LVM_REMOVEALLGROUPS: */

  /* case LVM_REMOVEGROUP: */

  case LVM_SCROLL:
    return LISTVIEW_Scroll(infoPtr, (INT)wParam, (INT)lParam);

  case LVM_SETBKCOLOR:
    return LISTVIEW_SetBkColor(infoPtr, (COLORREF)lParam);

  case LVM_SETBKIMAGEA:
  case LVM_SETBKIMAGEW:
    return LISTVIEW_SetBkImage(infoPtr, (LVBKIMAGEW *)lParam, uMsg == LVM_SETBKIMAGEW);

  case LVM_SETCALLBACKMASK:
    infoPtr->uCallbackMask = (UINT)wParam;
    return TRUE;

  case LVM_SETCOLUMNA:
  case LVM_SETCOLUMNW:
    return LISTVIEW_SetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam,
                               uMsg == LVM_SETCOLUMNW);

  case LVM_SETCOLUMNORDERARRAY:
    return LISTVIEW_SetColumnOrderArray(infoPtr, (INT)wParam, (LPINT)lParam);

  case LVM_SETCOLUMNWIDTH:
    return LISTVIEW_SetColumnWidth(infoPtr, (INT)wParam, (short)LOWORD(lParam));

  case LVM_SETEXTENDEDLISTVIEWSTYLE:
    return LISTVIEW_SetExtendedListViewStyle(infoPtr, (DWORD)wParam, (DWORD)lParam);

  /* case LVM_SETGROUPINFO: */

  /* case LVM_SETGROUPMETRICS: */

  case LVM_SETHOTCURSOR:
    return (LRESULT)LISTVIEW_SetHotCursor(infoPtr, (HCURSOR)lParam);

  case LVM_SETHOTITEM:
    return LISTVIEW_SetHotItem(infoPtr, (INT)wParam);

  case LVM_SETHOVERTIME:
    return LISTVIEW_SetHoverTime(infoPtr, (DWORD)lParam);

  case LVM_SETICONSPACING:
    if(lParam == -1)
        return LISTVIEW_SetIconSpacing(infoPtr, -1, -1);
    return LISTVIEW_SetIconSpacing(infoPtr, LOWORD(lParam), HIWORD(lParam));

  case LVM_SETIMAGELIST:
    return (LRESULT)LISTVIEW_SetImageList(infoPtr, (INT)wParam, (HIMAGELIST)lParam);

  /* case LVM_SETINFOTIP: */

  /* case LVM_SETINSERTMARK: */

  /* case LVM_SETINSERTMARKCOLOR: */

  case LVM_SETITEMA:
  case LVM_SETITEMW:
    {
	if (infoPtr->dwStyle & LVS_OWNERDATA) return FALSE;
	return LISTVIEW_SetItemT(infoPtr, (LPLVITEMW)lParam, (uMsg == LVM_SETITEMW));
    }

  case LVM_SETITEMCOUNT:
    return LISTVIEW_SetItemCount(infoPtr, (INT)wParam, (DWORD)lParam);

  case LVM_SETITEMPOSITION:
    {
	POINT pt;
        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        return LISTVIEW_SetItemPosition(infoPtr, (INT)wParam, &pt);
    }

  case LVM_SETITEMPOSITION32:
    return LISTVIEW_SetItemPosition(infoPtr, (INT)wParam, (POINT*)lParam);

  case LVM_SETITEMSTATE:
    return LISTVIEW_SetItemState(infoPtr, (INT)wParam, (LPLVITEMW)lParam);

  case LVM_SETITEMTEXTA:
  case LVM_SETITEMTEXTW:
    return LISTVIEW_SetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam,
                                 uMsg == LVM_SETITEMTEXTW);

  /* case LVM_SETOUTLINECOLOR: */

  case LVM_SETSELECTEDCOLUMN:
    infoPtr->selected_column = (INT)wParam;
    return TRUE;

  case LVM_SETSELECTIONMARK:
    return LISTVIEW_SetSelectionMark(infoPtr, (INT)lParam);

  case LVM_SETTEXTBKCOLOR:
    return LISTVIEW_SetTextBkColor(infoPtr, (COLORREF)lParam);

  case LVM_SETTEXTCOLOR:
    return LISTVIEW_SetTextColor(infoPtr, (COLORREF)lParam);

  /* case LVM_SETTILEINFO: */

  /* case LVM_SETTILEVIEWINFO: */

  /* case LVM_SETTILEWIDTH: */

  case LVM_SETTOOLTIPS:
    return (LRESULT)LISTVIEW_SetToolTips(infoPtr, (HWND)lParam);

  case LVM_SETUNICODEFORMAT:
    return LISTVIEW_SetUnicodeFormat(infoPtr, wParam);

  case LVM_SETVIEW:
    return LISTVIEW_SetView(infoPtr, wParam);

  /* case LVM_SETWORKAREAS: */

  /* case LVM_SORTGROUPS: */

  case LVM_SORTITEMS:
  case LVM_SORTITEMSEX:
    return LISTVIEW_SortItems(infoPtr, (PFNLVCOMPARE)lParam, wParam,
                              uMsg == LVM_SORTITEMSEX);
  case LVM_SUBITEMHITTEST:
    return LISTVIEW_HitTest(infoPtr, (LPLVHITTESTINFO)lParam, TRUE, FALSE);

  case LVM_UPDATE:
    return LISTVIEW_Update(infoPtr, (INT)wParam);

  case CCM_GETVERSION:
    return LISTVIEW_GetVersion(infoPtr);

  case CCM_SETVERSION:
    return LISTVIEW_SetVersion(infoPtr, wParam);

  case WM_CHAR:
    return LISTVIEW_ProcessLetterKeys( infoPtr, wParam, lParam );

  case WM_COMMAND:
    return LISTVIEW_Command(infoPtr, wParam, lParam);

  case WM_NCCREATE:
    return LISTVIEW_NCCreate(hwnd, wParam, (LPCREATESTRUCTW)lParam);

  case WM_CREATE:
    return LISTVIEW_Create(hwnd, (LPCREATESTRUCTW)lParam);

  case WM_DESTROY:
    return LISTVIEW_Destroy(infoPtr);

  case WM_ENABLE:
    return LISTVIEW_Enable(infoPtr);

  case WM_ERASEBKGND:
    return LISTVIEW_EraseBkgnd(infoPtr, (HDC)wParam);

  case WM_GETDLGCODE:
    return DLGC_WANTCHARS | DLGC_WANTARROWS;

  case WM_GETFONT:
    return (LRESULT)infoPtr->hFont;

  case WM_HSCROLL:
    return LISTVIEW_HScroll(infoPtr, (INT)LOWORD(wParam), 0);

  case WM_KEYDOWN:
    return LISTVIEW_KeyDown(infoPtr, (INT)wParam, (LONG)lParam);

  case WM_KILLFOCUS:
    return LISTVIEW_KillFocus(infoPtr);

  case WM_LBUTTONDBLCLK:
    return LISTVIEW_LButtonDblClk(infoPtr, (WORD)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_LBUTTONDOWN:
    return LISTVIEW_LButtonDown(infoPtr, (WORD)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_LBUTTONUP:
    return LISTVIEW_LButtonUp(infoPtr, (WORD)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_MOUSEMOVE:
    return LISTVIEW_MouseMove (infoPtr, (WORD)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_MOUSEHOVER:
    return LISTVIEW_MouseHover(infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_NCDESTROY:
    return LISTVIEW_NCDestroy(infoPtr);

  case WM_NCPAINT:
    return LISTVIEW_NCPaint(infoPtr, (HRGN)wParam);

  case WM_NOTIFY:
    return LISTVIEW_Notify(infoPtr, (LPNMHDR)lParam);

  case WM_NOTIFYFORMAT:
    return LISTVIEW_NotifyFormat(infoPtr, (HWND)wParam, (INT)lParam);

  case WM_PRINTCLIENT:
    return LISTVIEW_PrintClient(infoPtr, (HDC)wParam, (DWORD)lParam);

  case WM_PAINT:
    return LISTVIEW_WMPaint(infoPtr, (HDC)wParam);

  case WM_RBUTTONDBLCLK:
    return LISTVIEW_RButtonDblClk(infoPtr, (WORD)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_RBUTTONDOWN:
    return LISTVIEW_RButtonDown(infoPtr, (WORD)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_SETCURSOR:
    return LISTVIEW_SetCursor(infoPtr, wParam, lParam);

  case WM_SETFOCUS:
    return LISTVIEW_SetFocus(infoPtr, (HWND)wParam);

  case WM_SETFONT:
    return LISTVIEW_SetFont(infoPtr, (HFONT)wParam, (WORD)lParam);

  case WM_SETREDRAW:
    return LISTVIEW_SetRedraw(infoPtr, (BOOL)wParam);

  case WM_SHOWWINDOW:
    return LISTVIEW_ShowWindow(infoPtr, wParam, lParam);

  case WM_STYLECHANGED:
    return LISTVIEW_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

  case WM_STYLECHANGING:
    return LISTVIEW_StyleChanging(wParam, (LPSTYLESTRUCT)lParam);

  case WM_SYSCOLORCHANGE:
    COMCTL32_RefreshSysColors();
    return 0;

/*	case WM_TIMER: */
  case WM_THEMECHANGED:
    return LISTVIEW_ThemeChanged(infoPtr);

  case WM_VSCROLL:
    return LISTVIEW_VScroll(infoPtr, (INT)LOWORD(wParam), 0);

  case WM_MOUSEWHEEL:
      if (wParam & (MK_SHIFT | MK_CONTROL))
          return DefWindowProcW(hwnd, uMsg, wParam, lParam);
      return LISTVIEW_MouseWheel(infoPtr, (short int)HIWORD(wParam));

  case WM_WINDOWPOSCHANGED:
      if (!(((WINDOWPOS *)lParam)->flags & SWP_NOSIZE)) 
      {
          SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE |
                       SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);

          if ((infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && (infoPtr->uView == LV_VIEW_DETAILS))
          {
              if (notify_measureitem(infoPtr)) LISTVIEW_InvalidateList(infoPtr);
          }
          LISTVIEW_Size(infoPtr, ((WINDOWPOS *)lParam)->cx, ((WINDOWPOS *)lParam)->cy);
      }
      return DefWindowProcW(hwnd, uMsg, wParam, lParam);

/*	case WM_WININICHANGE: */

  default:
    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
      ERR("unknown msg %04x, wp %Ix, lp %Ix\n", uMsg, wParam, lParam);

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }

}

/***
 * DESCRIPTION:
 * Registers the window class.
 *
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
void LISTVIEW_Register(void)
{
    WNDCLASSW wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc = LISTVIEW_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(LISTVIEW_INFO *);
    wndClass.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_LISTVIEWW;
    RegisterClassW(&wndClass);
}

/***
 * DESCRIPTION:
 * Unregisters the window class.
 *
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
void LISTVIEW_Unregister(void)
{
    UnregisterClassW(WC_LISTVIEWW, NULL);
}

/***
 * DESCRIPTION:
 * Handle any WM_COMMAND messages
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wParam : the first message parameter
 * [I] lParam : the second message parameter
 *
 * RETURN:
 *   Zero.
 */
static LRESULT LISTVIEW_Command(LISTVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{

    TRACE("%p, %x, %x, %Ix\n", infoPtr, HIWORD(wParam), LOWORD(wParam), lParam);

    if (!infoPtr->hwndEdit) return 0;

    switch (HIWORD(wParam))
    {
	case EN_UPDATE:
	{
	    /*
	     * Adjust the edit window size
	     */
	    WCHAR buffer[1024];
	    HDC           hdc = GetDC(infoPtr->hwndEdit);
            HFONT         hFont, hOldFont = 0;
	    RECT	  rect;
	    SIZE	  sz;

	    if (!infoPtr->hwndEdit || !hdc) return 0;
	    GetWindowTextW(infoPtr->hwndEdit, buffer, ARRAY_SIZE(buffer));
	    GetWindowRect(infoPtr->hwndEdit, &rect);

            /* Select font to get the right dimension of the string */
            hFont = (HFONT)SendMessageW(infoPtr->hwndEdit, WM_GETFONT, 0, 0);
            if (hFont)
            {
                hOldFont = SelectObject(hdc, hFont);
            }

	    if (GetTextExtentPoint32W(hdc, buffer, lstrlenW(buffer), &sz))
	    {
                TEXTMETRICW textMetric;

                /* Add Extra spacing for the next character */
                GetTextMetricsW(hdc, &textMetric);
                sz.cx += (textMetric.tmMaxCharWidth * 2);

		SetWindowPos(infoPtr->hwndEdit, NULL, 0, 0, sz.cx,
		    rect.bottom - rect.top, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOZORDER);
	    }
            if (hFont)
                SelectObject(hdc, hOldFont);

	    ReleaseDC(infoPtr->hwndEdit, hdc);

	    break;
	}
	case EN_KILLFOCUS:
	{
            if (infoPtr->notify_mask & NOTIFY_MASK_END_LABEL_EDIT)
                LISTVIEW_CancelEditLabel(infoPtr);
            break;
	}

	default:
	  return SendMessageW (infoPtr->hwndNotify, WM_COMMAND, wParam, lParam);
    }

    return 0;
}
