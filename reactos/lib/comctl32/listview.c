/*
 * Listview control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999 Luc Tourangeau
 * Copyright 2000 Jason Mawdsley
 * Copyright 2001 CodeWeavers Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 *
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Oct. 21, 2002, by Dimitrie O. Paun.
 * 
 * Unless otherwise noted, we belive this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 * 
 * TODO:
 *
 * Features
 *   -- Hot item handling, mouse hovering
 *   -- Workareas support
 *   -- Tilemode support
 *   -- Groups support
 *
 * Bugs
 *   -- Expand large item in ICON mode when the cursor is flying over the icon or text.
 *   -- Support CustonDraw options for _WIN32_IE >= 0x560 (see NMLVCUSTOMDRAW docs.
 *   -- in LISTVIEW_AddGroupSelection, se whould send LVN_ODSTATECHANGED 
 *   -- LVA_SNAPTOGRID not implemented
 *   -- LISTVIEW_ApproximateViewRect partially implemented
 *   -- LISTVIEW_[GS]etColumnOrderArray stubs
 *   -- LISTVIEW_SetColumnWidth ignores header images & bitmap
 *   -- LISTVIEW_SetIconSpacing is incomplete
 *   -- LISTVIEW_SortItems is broken
 *   -- LISTVIEW_StyleChanged doesn't handle some changes too well
 *
 * Speedups
 *   -- LISTVIEW_GetNextItem needs to be rewritten. It is currently
 *      linear in the number of items in the list, and this is
 *      unacceptable for large lists.
 *   -- in sorted mode, LISTVIEW_InsertItemT sorts the array,
 *      instead of inserting in the right spot
 *   -- we should keep an ordered array of coordinates in iconic mode
 *      this would allow to frame items (iterator_frameditems),
 *      and find nearest item (LVFI_NEARESTXY) a lot more efficiently
 *
 * Flags
 *   -- LVIF_COLUMNS
 *   -- LVIF_GROUPID
 *   -- LVIF_NORECOMPUTE
 *
 * States
 *   -- LVIS_ACTIVATING (not currently supported by comctl32.dll version 6.0)
 *   -- LVIS_CUT
 *   -- LVIS_DROPHILITED
 *   -- LVIS_OVERLAYMASK
 *
 * Styles
 *   -- LVS_NOLABELWRAP
 *   -- LVS_NOSCROLL (see Q137520)
 *   -- LVS_SORTASCENDING, LVS_SORTDESCENDING
 *
 * Extended Styles
 *   -- LVS_EX_BORDERSELECT
 *   -- LVS_EX_FLATSB
 *   -- LVS_EX_GRIDLINES
 *   -- LVS_EX_HEADERDRAGDROP
 *   -- LVS_EX_INFOTIP
 *   -- LVS_EX_LABELTIP
 *   -- LVS_EX_MULTIWORKAREAS
 *   -- LVS_EX_ONECLICKACTIVATE
 *   -- LVS_EX_REGIONAL
 *   -- LVS_EX_SIMPLESELECT
 *   -- LVS_EX_TRACKSELECT
 *   -- LVS_EX_TWOCLICKACTIVATE
 *   -- LVS_EX_UNDERLINECOLD
 *   -- LVS_EX_UNDERLINEHOT
 *   
 * Notifications:
 *   -- LVN_BEGINSCROLL, LVN_ENDSCROLL
 *   -- LVN_GETINFOTIP
 *   -- LVN_HOTTRACK
 *   -- LVN_MARQUEEBEGIN
 *   -- LVN_ODFINDITEM
 *   -- LVN_ODSTATECHANGED
 *   -- LVN_SETDISPINFO
 *   -- NM_HOVER
 *
 * Messages:
 *   -- LVM_CANCELEDITLABEL
 *   -- LVM_ENABLEGROUPVIEW
 *   -- LVM_GETBKIMAGE, LVM_SETBKIMAGE
 *   -- LVM_GETGROUPINFO, LVM_SETGROUPINFO
 *   -- LVM_GETGROUPMETRICS, LVM_SETGROUPMETRICS
 *   -- LVM_GETINSERTMARK, LVM_SETINSERTMARK
 *   -- LVM_GETINSERTMARKCOLOR, LVM_SETINSERTMARKCOLOR
 *   -- LVM_GETINSERTMARKRECT
 *   -- LVM_GETNUMBEROFWORKAREAS
 *   -- LVM_GETOUTLINECOLOR, LVM_SETOUTLINECOLOR
 *   -- LVM_GETSELECTEDCOLUMN, LVM_SETSELECTEDCOLUMN
 *   -- LVM_GETISEARCHSTRINGW, LVM_GETISEARCHSTRINGA
 *   -- LVM_GETTILEINFO, LVM_SETTILEINFO
 *   -- LVM_GETTILEVIEWINFO, LVM_SETTILEVIEWINFO
 *   -- LVM_GETTOOLTIPS, LVM_SETTOOLTIPS
 *   -- LVM_GETUNICODEFORMAT, LVM_SETUNICODEFORMAT
 *   -- LVM_GETVIEW, LVM_SETVIEW
 *   -- LVM_GETWORKAREAS, LVM_SETWORKAREAS
 *   -- LVM_HASGROUP, LVM_INSERTGROUP, LVM_REMOVEGROUP, LVM_REMOVEALLGROUPS
 *   -- LVM_INSERTGROUPSORTED
 *   -- LVM_INSERTMARKHITTEST
 *   -- LVM_ISGROUPVIEWENABLED
 *   -- LVM_MAPIDTOINDEX, LVM_MAPINDEXTOID
 *   -- LVM_MOVEGROUP
 *   -- LVM_MOVEITEMTOGROUP
 *   -- LVM_SETINFOTIP
 *   -- LVM_SETTILEWIDTH
 *   -- LVM_SORTGROUPS
 *   -- LVM_SORTITEMSEX
 *
 * Known differences in message stream from native control (not known if
 * these differences cause problems):
 *   LVM_INSERTITEM issues LVM_SETITEMSTATE and LVM_SETITEM in certain cases.
 *   LVM_SETITEM does not always issue LVN_ITEMCHANGING/LVN_ITEMCHANGED.
 *   WM_CREATE does not issue WM_QUERYUISTATE and associated registry
 *     processing for "USEDOUBLECLICKTIME".
 */

#include "config.h"
#include "wine/port.h"

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

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(listview);

/* make sure you set this to 0 for production use! */
#define DEBUG_RANGES 1

typedef struct tagCOLUMN_INFO
{
  RECT rcHeader;	/* tracks the header's rectangle */
  int fmt;		/* same as LVCOLUMN.fmt */
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

typedef struct tagITEM_INFO
{
  ITEMHDR hdr;
  UINT state;
  LPARAM lParam;
  INT iIndent;
} ITEM_INFO;

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

typedef struct tagLISTVIEW_INFO
{
  HWND hwndSelf;
  HBRUSH hBkBrush;
  COLORREF clrBk;
  COLORREF clrText;
  COLORREF clrTextBk;
  COLORREF clrTextBkDefault;
  HIMAGELIST himlNormal;
  HIMAGELIST himlSmall;
  HIMAGELIST himlState;
  BOOL bLButtonDown;
  BOOL bRButtonDown;
  BOOL bNoItemMetrics;		/* flags if item metrics are not yet computed */
  INT nItemHeight;
  INT nItemWidth;
  RANGES selectionRanges;
  INT nSelectionMark;
  INT nHotItem;
  SHORT notifyFormat;
  HWND hwndNotify;
  RECT rcList;                 /* This rectangle is really the window
				* client rectangle possibly reduced by the 
				* horizontal scroll bar and/or header - see 
				* LISTVIEW_UpdateSize. This rectangle offset
				* by the LISTVIEW_GetOrigin value is in
				* client coordinates   */
  SIZE iconSize;
  SIZE iconSpacing;
  SIZE iconStateSize;
  UINT uCallbackMask;
  HWND hwndHeader;
  HCURSOR hHotCursor;
  HFONT hDefaultFont;
  HFONT hFont;
  INT ntmHeight;		/* Some cached metrics of the font used */
  INT ntmAveCharWidth;		/* by the listview to draw items */
  BOOL bRedraw;  		/* Turns on/off repaints & invalidations */
  BOOL bAutoarrange;		/* Autoarrange flag when NOT in LVS_AUTOARRANGE */
  BOOL bFocus;
  BOOL bDoChangeNotify;                /* send change notification messages? */
  INT nFocusedItem;
  RECT rcFocus;
  DWORD dwStyle;		/* the cached window GWL_STYLE */
  DWORD dwLvExStyle;		/* extended listview style */
  INT nItemCount;		/* the number of items in the list */
  HDPA hdpaItems;               /* array ITEM_INFO pointers */
  HDPA hdpaPosX;		/* maintains the (X, Y) coordinates of the */
  HDPA hdpaPosY;		/* items in LVS_ICON, and LVS_SMALLICON modes */
  HDPA hdpaColumns;		/* array of COLUMN_INFO pointers */
  POINT currIconPos;		/* this is the position next icon will be placed */
  PFNLVCOMPARE pfnCompare;
  LPARAM lParamSort;
  HWND hwndEdit;
  WNDPROC EditWndProc;
  INT nEditLabelItem;
  DWORD dwHoverTime;
  HWND hwndToolTip;

  DWORD cditemmode;             /* Keep the custom draw flags for an item/row */

  DWORD lastKeyPressTimestamp;
  WPARAM charCode;
  INT nSearchParamLength;
  WCHAR szSearchParam[ MAX_PATH ];
  BOOL bIsDrawing;
} LISTVIEW_INFO;

/*
 * constants
 */
/* How many we debug buffer to allocate */
#define DEBUG_BUFFERS 20
/* The size of a single debug bbuffer */
#define DEBUG_BUFFER_SIZE 256

/* Internal interface to LISTVIEW_HScroll and LISTVIEW_VScroll */
#define SB_INTERNAL      -1

/* maximum size of a label */
#define DISP_TEXT_SIZE 512

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

/* default column width for items in list display mode */
#define DEFAULT_COLUMN_WIDTH 128

/* Size of "line" scroll for V & H scrolls */
#define LISTVIEW_SCROLL_ICON_LINE_SIZE 37

/* Padding betwen image and label */
#define IMAGE_PADDING  2

/* Padding behind the label */
#define TRAILING_LABEL_PADDING  12
#define TRAILING_HEADER_PADDING  11

/* Border for the icon caption */
#define CAPTION_BORDER  2

/* Standard DrawText flags */
#define LV_ML_DT_FLAGS  (DT_TOP | DT_NOPREFIX | DT_EDITCONTROL | DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS)
#define LV_FL_DT_FLAGS  (DT_TOP | DT_NOPREFIX | DT_EDITCONTROL | DT_CENTER | DT_WORDBREAK | DT_NOCLIP)
#define LV_SL_DT_FLAGS  (DT_VCENTER | DT_EDITCONTROL | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS)

/* The time in milliseconds to reset the search in the list */
#define KEY_DELAY       450

/* Dump the LISTVIEW_INFO structure to the debug channel */
#define LISTVIEW_DUMP(iP) do { \
  TRACE("hwndSelf=%p, clrBk=0x%06lx, clrText=0x%06lx, clrTextBk=0x%06lx, ItemHeight=%d, ItemWidth=%d, Style=0x%08lx\n", \
        iP->hwndSelf, iP->clrBk, iP->clrText, iP->clrTextBk, \
        iP->nItemHeight, iP->nItemWidth, infoPtr->dwStyle); \
  TRACE("hwndSelf=%p, himlNor=%p, himlSml=%p, himlState=%p, Focused=%d, Hot=%d, exStyle=0x%08lx, Focus=%d\n", \
        iP->hwndSelf, iP->himlNormal, iP->himlSmall, iP->himlState, \
        iP->nFocusedItem, iP->nHotItem, iP->dwLvExStyle, iP->bFocus ); \
  TRACE("hwndSelf=%p, ntmH=%d, icSz.cx=%ld, icSz.cy=%ld, icSp.cx=%ld, icSp.cy=%ld, notifyFmt=%d\n", \
        iP->hwndSelf, iP->ntmHeight, iP->iconSize.cx, iP->iconSize.cy, \
        iP->iconSpacing.cx, iP->iconSpacing.cy, iP->notifyFormat); \
  TRACE("hwndSelf=%p, rcList=%s\n", iP->hwndSelf, debugrect(&iP->rcList)); \
} while(0)

/*
 * forward declarations
 */
static BOOL LISTVIEW_GetItemT(LISTVIEW_INFO *, LPLVITEMW, BOOL);
static void LISTVIEW_GetItemBox(LISTVIEW_INFO *, INT, LPRECT);
static void LISTVIEW_GetItemOrigin(LISTVIEW_INFO *, INT, LPPOINT);
static BOOL LISTVIEW_GetItemPosition(LISTVIEW_INFO *, INT, LPPOINT);
static BOOL LISTVIEW_GetItemRect(LISTVIEW_INFO *, INT, LPRECT);
static INT LISTVIEW_GetLabelWidth(LISTVIEW_INFO *, INT);
static void LISTVIEW_GetOrigin(LISTVIEW_INFO *, LPPOINT);
static BOOL LISTVIEW_GetViewRect(LISTVIEW_INFO *, LPRECT);
static void LISTVIEW_SetGroupSelection(LISTVIEW_INFO *, INT);
static BOOL LISTVIEW_SetItemT(LISTVIEW_INFO *, const LVITEMW *, BOOL);
static void LISTVIEW_UpdateScroll(LISTVIEW_INFO *);
static void LISTVIEW_SetSelection(LISTVIEW_INFO *, INT);
static void LISTVIEW_UpdateSize(LISTVIEW_INFO *);
static HWND LISTVIEW_EditLabelT(LISTVIEW_INFO *, INT, BOOL);
static LRESULT LISTVIEW_Command(LISTVIEW_INFO *, WPARAM, LPARAM);
static BOOL LISTVIEW_SortItems(LISTVIEW_INFO *, PFNLVCOMPARE, LPARAM);
static INT LISTVIEW_GetStringWidthT(LISTVIEW_INFO *, LPCWSTR, BOOL);
static BOOL LISTVIEW_KeySelection(LISTVIEW_INFO *, INT);
static UINT LISTVIEW_GetItemState(LISTVIEW_INFO *, INT, UINT);
static BOOL LISTVIEW_SetItemState(LISTVIEW_INFO *, INT, const LVITEMW *);
static LRESULT LISTVIEW_VScroll(LISTVIEW_INFO *, INT, INT, HWND);
static LRESULT LISTVIEW_HScroll(LISTVIEW_INFO *, INT, INT, HWND);
static INT LISTVIEW_GetTopIndex(LISTVIEW_INFO *);
static BOOL LISTVIEW_EnsureVisible(LISTVIEW_INFO *, INT, BOOL);
static HWND CreateEditLabelT(LISTVIEW_INFO *, LPCWSTR, DWORD, INT, INT, INT, INT, BOOL);
static HIMAGELIST LISTVIEW_SetImageList(LISTVIEW_INFO *, INT, HIMAGELIST);

/******** Text handling functions *************************************/

/* A text pointer is either NULL, LPSTR_TEXTCALLBACK, or points to a
 * text string. The string may be ANSI or Unicode, in which case
 * the boolean isW tells us the type of the string.
 *
 * The name of the function tell what type of strings it expects:
 *   W: Unicode, T: ANSI/Unicode - function of isW
 */

static inline BOOL is_textW(LPCWSTR text)
{
    return text != NULL && text != LPSTR_TEXTCALLBACKW;
}

static inline BOOL is_textT(LPCWSTR text, BOOL isW)
{
    /* we can ignore isW since LPSTR_TEXTCALLBACKW == LPSTR_TEXTCALLBACKA */
    return is_textW(text);
}

static inline int textlenT(LPCWSTR text, BOOL isW)
{
    return !is_textT(text, isW) ? 0 :
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

    if (!isW && is_textT(text, isW))
    {
	INT len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)text, -1, NULL, 0);
	wstr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	if (wstr) MultiByteToWideChar(CP_ACP, 0, (LPCSTR)text, -1, wstr, len);
    }
    TRACE("   wstr=%s\n", text == LPSTR_TEXTCALLBACKW ?  "(callback)" : debugstr_w(wstr));
    return wstr;
}

static inline void textfreeT(LPWSTR wstr, BOOL isW)
{
    if (!isW && is_textT(wstr, isW)) HeapFree(GetProcessHeap(), 0, wstr);
}

/*
 * dest is a pointer to a Unicode string
 * src is a pointer to a string (Unicode if isW, ANSI if !isW)
 */
static BOOL textsetptrT(LPWSTR *dest, LPWSTR src, BOOL isW)
{
    BOOL bResult = TRUE;
    
    if (src == LPSTR_TEXTCALLBACKW)
    {
	if (is_textW(*dest)) Free(*dest);
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
    if (!bt) return aw ? 1 : 0;
    if (aw == LPSTR_TEXTCALLBACKW)
	return bt == LPSTR_TEXTCALLBACKW ? 0 : -1;
    if (bt != LPSTR_TEXTCALLBACKW)
    {
	LPWSTR bw = textdupTtoW(bt, isW);
	int r = bw ? lstrcmpW(aw, bw) : 1;
	textfreeT(bw, isW);
	return r;
    }	    
	    
    return 1;
}
    
static inline int lstrncmpiW(LPCWSTR s1, LPCWSTR s2, int n)
{
    int res;

    n = min(min(n, strlenW(s1)), strlenW(s2));
    res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, s1, n, s2, n);
    return res ? res - sizeof(WCHAR) : res;
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

static char* debug_getbuf()
{
    static int index = 0;
    static char buffers[DEBUG_BUFFERS][DEBUG_BUFFER_SIZE];
    return buffers[index++ % DEBUG_BUFFERS];
}

static inline const char* debugrange(const RANGE *lprng)
{
    if (lprng) 
    {
	char* buf = debug_getbuf();
	snprintf(buf, DEBUG_BUFFER_SIZE, "[%d, %d)", lprng->lower, lprng->upper);
    	return buf;
    } else return "(null)";
}

static inline const char* debugpoint(const POINT *lppt)
{
    if (lppt) 
    {
	char* buf = debug_getbuf();
	snprintf(buf, DEBUG_BUFFER_SIZE, "(%ld, %ld)", lppt->x, lppt->y);
    	return buf;
    } else return "(null)";
}

static inline const char* debugrect(const RECT *rect)
{
    if (rect) 
    {
	char* buf = debug_getbuf();
	snprintf(buf, DEBUG_BUFFER_SIZE, "[(%ld, %ld);(%ld, %ld)]",
		 rect->left, rect->top, rect->right, rect->bottom);
    	return buf;
    } else return "(null)";
}

static const char * debugscrollinfo(const SCROLLINFO *pScrollInfo)
{
    char* buf = debug_getbuf(), *text = buf;
    int len, size = DEBUG_BUFFER_SIZE;
    
    if (pScrollInfo == NULL) return "(null)";
    len = snprintf(buf, size, "{cbSize=%d, ", pScrollInfo->cbSize);
    if (len == -1) goto end; buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_RANGE)
	len = snprintf(buf, size, "nMin=%d, nMax=%d, ", pScrollInfo->nMin, pScrollInfo->nMax);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_PAGE)
	len = snprintf(buf, size, "nPage=%u, ", pScrollInfo->nPage);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_POS)
	len = snprintf(buf, size, "nPos=%d, ", pScrollInfo->nPos);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (pScrollInfo->fMask & SIF_TRACKPOS)
	len = snprintf(buf, size, "nTrackPos=%d, ", pScrollInfo->nTrackPos);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    goto undo;
end:
    buf = text + strlen(text);
undo:
    if (buf - text > 2) { buf[-2] = '}'; buf[-1] = 0; }
    return text;
} 

static const char* debugnmlistview(const NMLISTVIEW *plvnm)
{
    if (plvnm)
    {
	char* buf = debug_getbuf();
        snprintf(buf, DEBUG_BUFFER_SIZE, "iItem=%d, iSubItem=%d, uNewState=0x%x,"
	         " uOldState=0x%x, uChanged=0x%x, ptAction=%s, lParam=%ld\n",
	         plvnm->iItem, plvnm->iSubItem, plvnm->uNewState, plvnm->uOldState,
		 plvnm->uChanged, debugpoint(&plvnm->ptAction), plvnm->lParam);
	return buf;
    } else return "(null)";
}

static const char* debuglvitem_t(const LVITEMW *lpLVItem, BOOL isW)
{
    char* buf = debug_getbuf(), *text = buf;
    int len, size = DEBUG_BUFFER_SIZE;
    
    if (lpLVItem == NULL) return "(null)";
    len = snprintf(buf, size, "{iItem=%d, iSubItem=%d, ", lpLVItem->iItem, lpLVItem->iSubItem);
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_STATE)
	len = snprintf(buf, size, "state=%x, stateMask=%x, ", lpLVItem->state, lpLVItem->stateMask);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_TEXT)
	len = snprintf(buf, size, "pszText=%s, cchTextMax=%d, ", debugtext_tn(lpLVItem->pszText, isW, 80), lpLVItem->cchTextMax);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_IMAGE)
	len = snprintf(buf, size, "iImage=%d, ", lpLVItem->iImage);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_PARAM)
	len = snprintf(buf, size, "lParam=%lx, ", lpLVItem->lParam);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_INDENT)
	len = snprintf(buf, size, "iIndent=%d, ", lpLVItem->iIndent);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
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
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_SUBITEM)
	len = snprintf(buf, size, "iSubItem=%d, ",  lpColumn->iSubItem);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_FMT)
	len = snprintf(buf, size, "fmt=%x, ", lpColumn->fmt);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_WIDTH)
	len = snprintf(buf, size, "cx=%d, ", lpColumn->cx);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_TEXT)
	len = snprintf(buf, size, "pszText=%s, cchTextMax=%d, ", debugtext_tn(lpColumn->pszText, isW, 80), lpColumn->cchTextMax);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_IMAGE)
	len = snprintf(buf, size, "iImage=%d, ", lpColumn->iImage);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_ORDER)
	len = snprintf(buf, size, "iOrder=%d, ", lpColumn->iOrder);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    goto undo;
end:
    buf = text + strlen(text);
undo:
    if (buf - text > 2) { buf[-2] = '}'; buf[-1] = 0; }
    return text;
}

static const char* debuglvhittestinfo(const LVHITTESTINFO *lpht)
{
    if (lpht) 
    {
	char* buf = debug_getbuf();
	snprintf(buf, DEBUG_BUFFER_SIZE, "{pt=%s, flags=0x%x, iItem=%d, iSubItem=%d}",
		 debugpoint(&lpht->pt), lpht->flags, lpht->iItem, lpht->iSubItem);
    	return buf;
    } else return "(null)";
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


/******** Notification functions i************************************/

static LRESULT notify_hdr(LISTVIEW_INFO *infoPtr, INT code, LPNMHDR pnmh)
{
    LRESULT result;
    
    TRACE("(code=%d)\n", code);

    pnmh->hwndFrom = infoPtr->hwndSelf;
    pnmh->idFrom = GetWindowLongW(infoPtr->hwndSelf, GWL_ID);
    pnmh->code = code;
    result = SendMessageW(infoPtr->hwndNotify, WM_NOTIFY,
			  (WPARAM)pnmh->idFrom, (LPARAM)pnmh);

    TRACE("  <= %ld\n", result);

    return result;
}

static inline LRESULT notify(LISTVIEW_INFO *infoPtr, INT code)
{
    NMHDR nmh;
    return notify_hdr(infoPtr, code, &nmh);
}

static inline void notify_itemactivate(LISTVIEW_INFO *infoPtr, LVHITTESTINFO *htInfo)
{
    NMITEMACTIVATE nmia;
    LVITEMW item;

    if (htInfo) {
      nmia.uNewState = 0;
      nmia.uOldState = 0;
      nmia.uChanged  = 0;
      nmia.uKeyFlags = 0;
      
      item.mask = LVIF_PARAM|LVIF_STATE;
      item.iItem = htInfo->iItem;
      item.iSubItem = 0;
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
    }
    notify_hdr(infoPtr, LVN_ITEMACTIVATE, (LPNMHDR)&nmia);
}

static inline LRESULT notify_listview(LISTVIEW_INFO *infoPtr, INT code, LPNMLISTVIEW plvnm)
{
    TRACE("(code=%d, plvnm=%s)\n", code, debugnmlistview(plvnm));
    return notify_hdr(infoPtr, code, (LPNMHDR)plvnm);
}

static LRESULT notify_click(LISTVIEW_INFO *infoPtr,  INT code, LVHITTESTINFO *lvht)
{
    NMLISTVIEW nmlv;
    LVITEMW item;
   
    TRACE("code=%d, lvht=%s\n", code, debuglvhittestinfo(lvht)); 
    ZeroMemory(&nmlv, sizeof(nmlv));
    nmlv.iItem = lvht->iItem;
    nmlv.iSubItem = lvht->iSubItem;
    nmlv.ptAction = lvht->pt;
    item.mask = LVIF_PARAM;
    item.iItem = lvht->iItem;
    item.iSubItem = 0;
    if (LISTVIEW_GetItemT(infoPtr, &item, TRUE)) nmlv.lParam = item.lParam;
    return notify_listview(infoPtr, code, &nmlv);
}

static void notify_deleteitem(LISTVIEW_INFO *infoPtr, INT nItem)
{
    NMLISTVIEW nmlv;
    LVITEMW item;
   
    ZeroMemory(&nmlv, sizeof (NMLISTVIEW));
    nmlv.iItem = nItem;
    item.mask = LVIF_PARAM;
    item.iItem = nItem;
    item.iSubItem = 0;
    if (LISTVIEW_GetItemT(infoPtr, &item, TRUE)) nmlv.lParam = item.lParam;
    notify_listview(infoPtr, LVN_DELETEITEM, &nmlv);
}

static int get_ansi_notification(INT unicodeNotificationCode)
{
    switch (unicodeNotificationCode)
    {
    case LVN_BEGINLABELEDITW: return LVN_BEGINLABELEDITA;
    case LVN_ENDLABELEDITW: return LVN_ENDLABELEDITA;
    case LVN_GETDISPINFOW: return LVN_GETDISPINFOA;
    case LVN_SETDISPINFOW: return LVN_SETDISPINFOA;
    case LVN_ODFINDITEMW: return LVN_ODFINDITEMA;
    case LVN_GETINFOTIPW: return LVN_GETINFOTIPA;
    }
    ERR("unknown notification %x\n", unicodeNotificationCode);
    assert(FALSE);
    return 0;
}

/*
  With testing on Windows 2000 it looks like the notify format
  has nothing to do with this message. It ALWAYS seems to be
  in ansi format.

  infoPtr : listview struct
  notificationCode : *Unicode* notification code
  pdi : dispinfo structure (can be unicode or ansi)
  isW : TRUE if dispinfo is Unicode
*/
static BOOL notify_dispinfoT(LISTVIEW_INFO *infoPtr, INT notificationCode, LPNMLVDISPINFOW pdi, BOOL isW)
{
    BOOL bResult = FALSE;
    BOOL convertToAnsi = FALSE;
    INT cchTempBufMax = 0, savCchTextMax = 0;
    LPWSTR pszTempBuf = NULL, savPszText = NULL;

    if ((pdi->item.mask & LVIF_TEXT) && is_textT(pdi->item.pszText, isW))
	    convertToAnsi = isW;

    if (convertToAnsi)
    {
	if (notificationCode != LVN_GETDISPINFOW)
	{
            cchTempBufMax = WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText,
                                                -1, NULL, 0, NULL, NULL);
        }
	else
	{
	    cchTempBufMax = pdi->item.cchTextMax;
	    *pdi->item.pszText = 0; /* make sure we don't process garbage */
	}

        pszTempBuf = HeapAlloc(GetProcessHeap(), 0, sizeof(CHAR) *
                               cchTempBufMax);
        if (!pszTempBuf) return FALSE;

        WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, (LPSTR)
                             pszTempBuf, cchTempBufMax, NULL, NULL);

        savCchTextMax = pdi->item.cchTextMax;
        savPszText = pdi->item.pszText;
        pdi->item.pszText = pszTempBuf;
        pdi->item.cchTextMax = cchTempBufMax;
    }

    TRACE(" pdi->item=%s\n", debuglvitem_t(&pdi->item, infoPtr->notifyFormat !=
           NFR_ANSI));

    bResult = notify_hdr(infoPtr, get_ansi_notification(notificationCode),
                        (LPNMHDR)pdi);

    if (convertToAnsi)
    {
        MultiByteToWideChar(CP_ACP, 0, (LPSTR) pdi->item.pszText, -1,
                            savPszText, savCchTextMax);
        pdi->item.pszText = savPszText; /* restores our buffer */
        pdi->item.cchTextMax = savCchTextMax;
        HeapFree(GetProcessHeap(), 0, pszTempBuf);
    }
    return bResult;
}

static void customdraw_fill(NMLVCUSTOMDRAW *lpnmlvcd, LISTVIEW_INFO *infoPtr, HDC hdc, 
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

static inline DWORD notify_customdraw (LISTVIEW_INFO *infoPtr, DWORD dwDrawStage, NMLVCUSTOMDRAW *lpnmlvcd)
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

static void prepaint_setup (LISTVIEW_INFO *infoPtr, HDC hdc, NMLVCUSTOMDRAW *lpnmlvcd)
{
    /* apprently, for selected items, we have to override the returned values */
    if (lpnmlvcd->nmcd.uItemState & CDIS_SELECTED)
    {
	if (infoPtr->bFocus)
	{
	    lpnmlvcd->clrTextBk = comctl32_color.clrHighlight;
	    lpnmlvcd->clrText   = comctl32_color.clrHighlightText;
        }
	else if (infoPtr->dwStyle & LVS_SHOWSELALWAYS)
	{
	    lpnmlvcd->clrTextBk = comctl32_color.clr3dFace;
	    lpnmlvcd->clrText   = comctl32_color.clrBtnText;
	}
    }

    /* Set the text attributes */
    if (lpnmlvcd->clrTextBk != CLR_NONE)
    {
	SetBkMode(hdc, OPAQUE);
	if (lpnmlvcd->clrTextBk == CLR_DEFAULT)
	    SetBkColor(hdc, infoPtr->clrTextBkDefault);
	else
	    SetBkColor(hdc,lpnmlvcd->clrTextBk);
    }
    else
	SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, lpnmlvcd->clrText);
}

static inline DWORD notify_postpaint (LISTVIEW_INFO *infoPtr, NMLVCUSTOMDRAW *lpnmlvcd)
{
    return notify_customdraw(infoPtr, CDDS_POSTPAINT, lpnmlvcd);
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
 * over items of iterest in the list. Typically, you create a
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
 * The iterator has a notion of a out-of-order, special item,
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

static RANGE iterator_range(ITERATOR* i)
{
    RANGE range;

    if (!i->ranges) return i->range;

    range.lower = (*(RANGE*)DPA_GetPtr(i->ranges->hdpa, 0)).lower;
    range.upper = (*(RANGE*)DPA_GetPtr(i->ranges->hdpa, DPA_GetPtrCount(i->ranges->hdpa) - 1)).upper;
    return range;
}

/***
 * Releases resources associated with this ierator.
 */
static inline void iterator_destroy(ITERATOR* i)
{
    ranges_destroy(i->ranges);
}

/***
 * Create an empty iterator.
 */
static inline BOOL iterator_empty(ITERATOR* i)
{
    ZeroMemory(i, sizeof(*i));
    i->nItem = i->nSpecial = i->range.lower = i->range.upper = -1;
    return TRUE;
}

/***
 * Create an iterator over a range.
 */
static inline BOOL iterator_rangeitems(ITERATOR* i, RANGE range)
{
    iterator_empty(i);
    i->range = range;
    return TRUE;
}

/***
 * Create an iterator over a bunch of ranges.
 * Please note that the iterator will take ownership of the ranges,
 * and will free them upon destruction.
 */
static inline BOOL iterator_rangesitems(ITERATOR* i, RANGES ranges)
{
    iterator_empty(i);
    i->ranges = ranges;
    return TRUE;
}

/***
 * Creates an iterator over the items which intersect lprc.
 */
static BOOL iterator_frameditems(ITERATOR* i, LISTVIEW_INFO* infoPtr, const RECT *lprc)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    RECT frame = *lprc, rcItem, rcTemp;
    POINT Origin;
    
    /* in case we fail, we want to return an empty iterator */
    if (!iterator_empty(i)) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);

    TRACE("(lprc=%s)\n", debugrect(lprc));
    OffsetRect(&frame, -Origin.x, -Origin.y);

    if (uView == LVS_ICON || uView == LVS_SMALLICON)
    {
	INT nItem;
	
	if (uView == LVS_ICON && infoPtr->nFocusedItem != -1)
	{
	    LISTVIEW_GetItemBox(infoPtr, infoPtr->nFocusedItem, &rcItem);
	    if (IntersectRect(&rcTemp, &rcItem, lprc))
		i->nSpecial = infoPtr->nFocusedItem;
	}
	if (!(iterator_rangesitems(i, ranges_create(50)))) return FALSE;
	/* to do better here, we need to have PosX, and PosY sorted */
	TRACE("building icon ranges:\n");
	for (nItem = 0; nItem < infoPtr->nItemCount; nItem++)
	{
            rcItem.left = (LONG)DPA_GetPtr(infoPtr->hdpaPosX, nItem);
	    rcItem.top = (LONG)DPA_GetPtr(infoPtr->hdpaPosY, nItem);
	    rcItem.right = rcItem.left + infoPtr->nItemWidth;
	    rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
	    if (IntersectRect(&rcTemp, &rcItem, &frame))
		ranges_additem(i->ranges, nItem);
	}
	return TRUE;
    }
    else if (uView == LVS_REPORT)
    {
	RANGE range;
	
	if (frame.left >= infoPtr->nItemWidth) return TRUE;
	if (frame.top >= infoPtr->nItemHeight * infoPtr->nItemCount) return TRUE;
	
	range.lower = max(frame.top / infoPtr->nItemHeight, 0);
	range.upper = min((frame.bottom - 1) / infoPtr->nItemHeight, infoPtr->nItemCount - 1) + 1;
	if (range.upper <= range.lower) return TRUE;
	if (!iterator_rangeitems(i, range)) return FALSE;
	TRACE("    report=%s\n", debugrange(&i->range));
    }
    else
    {
	INT nPerCol = max((infoPtr->rcList.bottom - infoPtr->rcList.top) / infoPtr->nItemHeight, 1);
	INT nFirstRow = max(frame.top / infoPtr->nItemHeight, 0);
	INT nLastRow = min((frame.bottom - 1) / infoPtr->nItemHeight, nPerCol - 1);
	INT nFirstCol = max(frame.left / infoPtr->nItemWidth, 0);
	INT nLastCol = min((frame.right - 1) / infoPtr->nItemWidth, (infoPtr->nItemCount + nPerCol - 1) / nPerCol);
	INT lower = nFirstCol * nPerCol + nFirstRow;
	RANGE item_range;
	INT nCol;

	TRACE("nPerCol=%d, nFirstRow=%d, nLastRow=%d, nFirstCol=%d, nLastCol=%d, lower=%d\n",
	      nPerCol, nFirstRow, nLastRow, nFirstCol, nLastCol, lower);
	
	if (nLastCol < nFirstCol || nLastRow < nFirstRow) return TRUE;

	if (!(iterator_rangesitems(i, ranges_create(nLastCol - nFirstCol + 1)))) return FALSE;
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
 * Creates an iterator over the items which intersect the visible region of hdc.
 */
static BOOL iterator_visibleitems(ITERATOR *i, LISTVIEW_INFO *infoPtr, HDC  hdc)
{
    POINT Origin, Position;
    RECT rcItem, rcClip;
    INT rgntype;
    
    rgntype = GetClipBox(hdc, &rcClip);
    if (rgntype == NULLREGION) return iterator_empty(i);
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
	rcItem.left = Position.x + Origin.x;
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

/******** Misc helper functions ************************************/

static inline LRESULT CallWindowProcT(WNDPROC proc, HWND hwnd, UINT uMsg,
		                      WPARAM wParam, LPARAM lParam, BOOL isW)
{
    if (isW) return CallWindowProcW(proc, hwnd, uMsg, wParam, lParam);
    else return CallWindowProcA(proc, hwnd, uMsg, wParam, lParam);
}

static inline BOOL is_autoarrange(LISTVIEW_INFO *infoPtr)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    
    return ((infoPtr->dwStyle & LVS_AUTOARRANGE) || infoPtr->bAutoarrange) &&
	   (uView == LVS_ICON || uView == LVS_SMALLICON);
}

/******** Internal API functions ************************************/

static inline COLUMN_INFO * LISTVIEW_GetColumnInfo(LISTVIEW_INFO *infoPtr, INT nSubItem)
{
    static COLUMN_INFO mainItem;

    if (nSubItem == 0 && DPA_GetPtrCount(infoPtr->hdpaColumns) == 0) return &mainItem;
    assert (nSubItem >= 0 && nSubItem < DPA_GetPtrCount(infoPtr->hdpaColumns));
    return (COLUMN_INFO *)DPA_GetPtr(infoPtr->hdpaColumns, nSubItem);
}
	
static inline void LISTVIEW_GetHeaderRect(LISTVIEW_INFO *infoPtr, INT nSubItem, RECT *lprc)
{
    *lprc = LISTVIEW_GetColumnInfo(infoPtr, nSubItem)->rcHeader;
}
	
static inline BOOL LISTVIEW_GetItemW(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem)
{
    return LISTVIEW_GetItemT(infoPtr, lpLVItem, TRUE);
}

/* Listview invalidation functions: use _only_ these functions to invalidate */

static inline BOOL is_redrawing(LISTVIEW_INFO *infoPtr)
{
    return infoPtr->bRedraw;
}

static inline void LISTVIEW_InvalidateRect(LISTVIEW_INFO *infoPtr, const RECT* rect)
{
    if(!is_redrawing(infoPtr)) return; 
    TRACE(" invalidating rect=%s\n", debugrect(rect));
    InvalidateRect(infoPtr->hwndSelf, rect, TRUE);
}

static inline void LISTVIEW_InvalidateItem(LISTVIEW_INFO *infoPtr, INT nItem)
{
    RECT rcBox;

    if(!is_redrawing(infoPtr)) return; 
    LISTVIEW_GetItemBox(infoPtr, nItem, &rcBox);
    LISTVIEW_InvalidateRect(infoPtr, &rcBox);
}

static inline void LISTVIEW_InvalidateSubItem(LISTVIEW_INFO *infoPtr, INT nItem, INT nSubItem)
{
    POINT Origin, Position;
    RECT rcBox;
    
    if(!is_redrawing(infoPtr)) return; 
    assert ((infoPtr->dwStyle & LVS_TYPEMASK) == LVS_REPORT);
    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);
    LISTVIEW_GetHeaderRect(infoPtr, nSubItem, &rcBox);
    rcBox.top = 0;
    rcBox.bottom = infoPtr->nItemHeight;
    OffsetRect(&rcBox, Origin.x + Position.x, Origin.y + Position.y);
    LISTVIEW_InvalidateRect(infoPtr, &rcBox);
}

static inline void LISTVIEW_InvalidateList(LISTVIEW_INFO *infoPtr)
{
    LISTVIEW_InvalidateRect(infoPtr, NULL);
}

static inline void LISTVIEW_InvalidateColumn(LISTVIEW_INFO *infoPtr, INT nColumn)
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
static inline INT LISTVIEW_GetCountPerRow(LISTVIEW_INFO *infoPtr)
{
    INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;

    return max(nListWidth/infoPtr->nItemWidth, 1);
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
static inline INT LISTVIEW_GetCountPerColumn(LISTVIEW_INFO *infoPtr)
{
    INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;

    return max(nListHeight / infoPtr->nItemHeight, 1);
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
 *    accept and it ignores averything else. In particular it will
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
    INT nItem;
    INT endidx,idx;
    LVITEMW item;
    WCHAR buffer[MAX_PATH];
    DWORD lastKeyPressTimestamp = infoPtr->lastKeyPressTimestamp;

    /* simple parameter checking */
    if (!charCode || !keyData) return 0;

    /* only allow the valid WM_CHARs through */
    if (!isalnum(charCode) &&
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

    /* if there's one item or less, there is no where to go */
    if (infoPtr->nItemCount <= 1) return 0;

    /* update the search parameters */
    infoPtr->lastKeyPressTimestamp = GetTickCount();
    if (infoPtr->lastKeyPressTimestamp - lastKeyPressTimestamp < KEY_DELAY) {
        if (infoPtr->nSearchParamLength < MAX_PATH)
            infoPtr->szSearchParam[infoPtr->nSearchParamLength++]=charCode;
        if (infoPtr->charCode != charCode)
            infoPtr->charCode = charCode = 0;
    } else {
        infoPtr->charCode=charCode;
        infoPtr->szSearchParam[0]=charCode;
        infoPtr->nSearchParamLength=1;
        /* Redundant with the 1 char string */
        charCode=0;
    }

    /* and search from the current position */
    nItem=-1;
    if (infoPtr->nFocusedItem >= 0) {
        endidx=infoPtr->nFocusedItem;
        idx=endidx;
        /* if looking for single character match,
         * then we must always move forward
         */
        if (infoPtr->nSearchParamLength == 1)
            idx++;
    } else {
        endidx=infoPtr->nItemCount;
        idx=0;
    }
    do {
        if (idx == infoPtr->nItemCount) {
            if (endidx == infoPtr->nItemCount || endidx == 0)
                break;
            idx=0;
        }

        /* get item */
        item.mask = LVIF_TEXT;
        item.iItem = idx;
        item.iSubItem = 0;
        item.pszText = buffer;
        item.cchTextMax = MAX_PATH;
        if (!LISTVIEW_GetItemW(infoPtr, &item)) return 0;

        /* check for a match */
        if (lstrncmpiW(item.pszText,infoPtr->szSearchParam,infoPtr->nSearchParamLength) == 0) {
            nItem=idx;
            break;
        } else if ( (charCode != 0) && (nItem == -1) && (nItem != infoPtr->nFocusedItem) &&
                    (lstrncmpiW(item.pszText,infoPtr->szSearchParam,1) == 0) ) {
            /* This would work but we must keep looking for a longer match */
            nItem=idx;
        }
        idx++;
    } while (idx != endidx);

    if (nItem != -1)
        LISTVIEW_KeySelection(infoPtr, nItem);

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
static void LISTVIEW_UpdateHeaderSize(LISTVIEW_INFO *infoPtr, INT nNewScrollPos)
{
    RECT winRect;
    POINT point[2];

    TRACE("nNewScrollPos=%d\n", nNewScrollPos);

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
        SWP_NOZORDER | SWP_NOACTIVATE);
}

/***
 * DESCRIPTION:
 * Update the scrollbars. This functions should be called whenever
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    SCROLLINFO horzInfo, vertInfo;

    if ((infoPtr->dwStyle & LVS_NOSCROLL) || !is_redrawing(infoPtr)) return;

    ZeroMemory(&horzInfo, sizeof(SCROLLINFO));
    horzInfo.cbSize = sizeof(SCROLLINFO);
    horzInfo.nPage = infoPtr->rcList.right - infoPtr->rcList.left;

    /* for now, we'll set info.nMax to the _count_, and adjust it later */
    if (uView == LVS_LIST)
    {
	INT nPerCol = LISTVIEW_GetCountPerColumn(infoPtr);
	horzInfo.nMax = (infoPtr->nItemCount + nPerCol - 1) / nPerCol;

	/* scroll by at least one column per page */
	if(horzInfo.nPage < infoPtr->nItemWidth)
		horzInfo.nPage = infoPtr->nItemWidth;

	horzInfo.nPage /= infoPtr->nItemWidth;
    }
    else if (uView == LVS_REPORT)
    {
	horzInfo.nMax = infoPtr->nItemWidth;
    }
    else /* LVS_ICON, or LVS_SMALLICON */
    {
	RECT rcView;

	if (LISTVIEW_GetViewRect(infoPtr, &rcView)) horzInfo.nMax = rcView.right - rcView.left;
    }
  
    horzInfo.fMask = SIF_RANGE | SIF_PAGE;
    horzInfo.nMax = max(horzInfo.nMax - 1, 0);
    SetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &horzInfo, TRUE);
    TRACE("horzInfo=%s\n", debugscrollinfo(&horzInfo));

    /* Setting the horizontal scroll can change the listview size
     * (and potentially everything else) so we need to recompute
     * everything again for the vertical scroll
     */

    ZeroMemory(&vertInfo, sizeof(SCROLLINFO));
    vertInfo.cbSize = sizeof(SCROLLINFO);
    vertInfo.nPage = infoPtr->rcList.bottom - infoPtr->rcList.top;

    if (uView == LVS_REPORT)
    {
	vertInfo.nMax = infoPtr->nItemCount;
	
	/* scroll by at least one page */
	if(vertInfo.nPage < infoPtr->nItemHeight)
	  vertInfo.nPage = infoPtr->nItemHeight;

	vertInfo.nPage /= infoPtr->nItemHeight;
    }
    else if (uView != LVS_LIST) /* LVS_ICON, or LVS_SMALLICON */
    {
	RECT rcView;

	if (LISTVIEW_GetViewRect(infoPtr, &rcView)) vertInfo.nMax = rcView.bottom - rcView.top;
    }

    vertInfo.fMask = SIF_RANGE | SIF_PAGE;
    vertInfo.nMax = max(vertInfo.nMax - 1, 0);
    SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &vertInfo, TRUE);
    TRACE("vertInfo=%s\n", debugscrollinfo(&vertInfo));

    /* Update the Header Control */
    if (uView == LVS_REPORT)
    {
	horzInfo.fMask = SIF_POS;
	GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &horzInfo);
	LISTVIEW_UpdateHeaderSize(infoPtr, horzInfo.nPos);
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
static void LISTVIEW_ShowFocusRect(LISTVIEW_INFO *infoPtr, BOOL fShow)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    HDC hdc;

    TRACE("fShow=%d, nItem=%d\n", fShow, infoPtr->nFocusedItem);

    if (infoPtr->nFocusedItem < 0) return;

    /* we need some gymnastics in ICON mode to handle large items */
    if ( (infoPtr->dwStyle & LVS_TYPEMASK) == LVS_ICON )
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
    if ((infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && (uView == LVS_REPORT))
    {
	DRAWITEMSTRUCT dis;
	LVITEMW item;

        item.iItem = infoPtr->nFocusedItem;
	item.iSubItem = 0;
        item.mask = LVIF_PARAM;
	if (!LISTVIEW_GetItemW(infoPtr, &item)) goto done;
	   
	ZeroMemory(&dis, sizeof(dis)); 
	dis.CtlType = ODT_LISTVIEW;
	dis.CtlID = GetWindowLongW(infoPtr->hwndSelf, GWL_ID);
	dis.itemID = item.iItem;
	dis.itemAction = ODA_FOCUS;
	if (fShow) dis.itemState |= ODS_FOCUS;
	dis.hwndItem = infoPtr->hwndSelf;
	dis.hDC = hdc;
	LISTVIEW_GetItemBox(infoPtr, dis.itemID, &dis.rcItem);
	dis.itemData = item.lParam;

	SendMessageW(infoPtr->hwndNotify, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
    }
    else
    {
	DrawFocusRect(hdc, &infoPtr->rcFocus);
    }
done:
    ReleaseDC(infoPtr->hwndSelf, hdc);
}

/***
 * Invalidates all visible selected items.
 */
static void LISTVIEW_InvalidateSelectedItems(LISTVIEW_INFO *infoPtr)
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
 * over again, when this function is call in a loop. Instead,
 * one ca factor the computation of the Origin before the loop,
 * and offset the value retured by this function, on every iteration.
 * 
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem  : item number
 * [O] lpptOrig : item top, left corner
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_GetItemOrigin(LISTVIEW_INFO *infoPtr, INT nItem, LPPOINT lpptPosition)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;

    assert(nItem >= 0 && nItem < infoPtr->nItemCount);

    if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
    {
	lpptPosition->x = (LONG)DPA_GetPtr(infoPtr->hdpaPosX, nItem);
	lpptPosition->y = (LONG)DPA_GetPtr(infoPtr->hdpaPosY, nItem);
    }
    else if (uView == LVS_LIST)
    {
        INT nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
	lpptPosition->x = nItem / nCountPerColumn * infoPtr->nItemWidth;
	lpptPosition->y = nItem % nCountPerColumn * infoPtr->nItemHeight;
    }
    else /* LVS_REPORT */
    {
	lpptPosition->x = 0;
	lpptPosition->y = nItem * infoPtr->nItemHeight;
    }
}
    
/***
 * DESCRIPTION:            [INTERNAL]
 * Compute the rectangles of an item.  This is to localize all
 * the computations in one place. If you are not interested in some
 * of these values, simply pass in a NULL -- the fucntion is smart
 * enough to compute only what's necessary. The function computes
 * the standard rectangles (BOUNDS, ICON, LABEL) plus a non-standard
 * one, the BOX rectangle. This rectangle is very cheap to compute,
 * and is guaranteed to contain all the other rectangles. Computing
 * the ICON rect is also cheap, but all the others are potentaily
 * expensive. This gives an easy and effective optimization when
 * searching (like point inclusion, or rectangle intersection):
 * first test against the BOX, and if TRUE, test agains the desired
 * rectangle.
 * If the function does not have all the necessary information
 * to computed the requested rectangles, will crash with a
 * failed assertion. This is done so we catch all programming
 * errors, given that the function is called only from our code.
 *
 * We have the following 'special' meanings for a few fields:
 *   * If LVIS_FOCUSED is set, we assume the item has the focus
 *     This is important in ICON mode, where it might get a larger
 *     then usual rectange
 *
 * Please note that subitem support works only in REPORT mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : item to compute the measures for
 * [O] lprcBox : ptr to Box rectangle
 *                The internal LVIR_BOX rectangle
 * [0] lprcState : ptr to State icon rectangle
 *  		  The internal LVIR_STATE rectangle
 * [O] lprcIcon : ptr to Icon rectangle
 *                Same as LVM_GETITEMRECT with LVIR_ICON
 * [O] lprcLabel : ptr to Label rectangle
 *                Same as LVM_GETITEMRECT with LVIR_LABEL
 *
 * RETURN:
 *   None.
 */
static void LISTVIEW_GetItemMetrics(LISTVIEW_INFO *infoPtr, const LVITEMW *lpLVItem,
				    LPRECT lprcBox, LPRECT lprcState, 
				    LPRECT lprcIcon, LPRECT lprcLabel)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    BOOL doState = FALSE, doIcon = FALSE, doLabel = FALSE, oversizedBox = FALSE;
    RECT Box, State, Icon, Label;
    COLUMN_INFO *lpColumnInfo = NULL;

    TRACE("(lpLVItem=%s)\n", debuglvitem_t(lpLVItem, TRUE));
	
    /* Be smart and try to figure out the minimum we have to do */
    if (lpLVItem->iSubItem) assert(uView == LVS_REPORT);
    if (uView == LVS_ICON && (lprcBox || lprcLabel))
    {
	assert((lpLVItem->mask & LVIF_STATE) && (lpLVItem->stateMask & LVIS_FOCUSED));
	if (lpLVItem->state & LVIS_FOCUSED) oversizedBox = doLabel = TRUE;
    }
    if (lprcLabel) doLabel = TRUE;
    if (doLabel || lprcIcon) doIcon = TRUE;
    if (doIcon || lprcState) doState = TRUE;
    
    /************************************************************/
    /* compute the box rectangle (it should be cheap to do)     */
    /************************************************************/
    if (lpLVItem->iSubItem || uView == LVS_REPORT)
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
		
    /************************************************************/
    /* compute STATEICON bounding box                           */
    /************************************************************/
    if (doState)
    {
	if (uView == LVS_ICON)
	{
     	    State.left = Box.left - infoPtr->iconStateSize.cx - 2;
	    if (infoPtr->himlNormal) 
		State.left += (infoPtr->nItemWidth - infoPtr->iconSize.cx) / 2;
	    State.top  = Box.top + infoPtr->iconSize.cy - infoPtr->iconStateSize.cy + 4;
	}
	else
	{
	    /* we need the ident in report mode, if we don't have it, we fail */
	    State.left = Box.left;
	    if (uView == LVS_REPORT) 
	    {
		if (lpLVItem->iSubItem == 0)
		{
		    State.left += REPORT_MARGINX;
		    assert(lpLVItem->mask & LVIF_INDENT);
		    State.left += infoPtr->iconSize.cx * lpLVItem->iIndent;
		}
	    }
	    State.top  = Box.top;
	}	
	State.right    = State.left;
	State.bottom   = State.top;
	if (infoPtr->himlState && lpLVItem->iSubItem == 0)
	{
	    State.right  += infoPtr->iconStateSize.cx;
	    State.bottom += infoPtr->iconStateSize.cy;
	}
	if (lprcState) *lprcState = State;
	TRACE("    - state=%s\n", debugrect(&State));
    }

    /************************************************************/
    /* compute ICON bounding box (ala LVM_GETITEMRECT)          */
    /************************************************************/
    if (doIcon)
    {
	if (uView == LVS_ICON)
	{
	    Icon.left   = Box.left;
	    if (infoPtr->himlNormal) 
		Icon.left += (infoPtr->nItemWidth - infoPtr->iconSize.cx) / 2;
	    Icon.top    = Box.top + ICON_TOP_PADDING;
	    Icon.right  = Icon.left;
	    Icon.bottom = Icon.top;
	    if (infoPtr->himlNormal)
	    {
		Icon.right  += infoPtr->iconSize.cx;
		Icon.bottom += infoPtr->iconSize.cy;
	    }
	}
	else /* LVS_SMALLICON, LVS_LIST or LVS_REPORT */
	{
	    Icon.left = State.right;
	    Icon.top    = Box.top;
	    Icon.right  = Icon.left;
	    if (infoPtr->himlSmall &&
                (!lpColumnInfo || lpLVItem->iSubItem == 0 || (lpColumnInfo->fmt & LVCFMT_IMAGE) ||
                 ((infoPtr->dwLvExStyle & LVS_EX_SUBITEMIMAGES) && lpLVItem->iImage != I_IMAGECALLBACK)))
		Icon.right += infoPtr->iconSize.cx;
	    Icon.bottom = Icon.top + infoPtr->nItemHeight;
	}
	if(lprcIcon) *lprcIcon = Icon;
	TRACE("    - icon=%s\n", debugrect(&Icon));
     }

    /************************************************************/
    /* compute LABEL bounding box (ala LVM_GETITEMRECT)         */
    /************************************************************/
    if (doLabel)
    {
	SIZE labelSize = { 0, 0 };

	/* calculate how far to the right can the label strech */
	Label.right = Box.right;
	if (uView == LVS_REPORT)
	{
	    if (lpLVItem->iSubItem == 0) Label = lpColumnInfo->rcHeader;
	}

	if (lpLVItem->iSubItem || ((infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && uView == LVS_REPORT))
	{
	   labelSize.cx = infoPtr->nItemWidth;
	   labelSize.cy = infoPtr->nItemHeight;
	   goto calc_label;
	}
	
	/* we need the text in non owner draw mode */
	assert(lpLVItem->mask & LVIF_TEXT);
	if (is_textT(lpLVItem->pszText, TRUE))
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
	    if (uView == LVS_ICON) 
		rcText.bottom -= ICON_TOP_PADDING + infoPtr->iconSize.cy + ICON_BOTTOM_PADDING;

	    /* now figure out the flags */
	    if (uView == LVS_ICON)
		uFormat = oversizedBox ? LV_FL_DT_FLAGS : LV_ML_DT_FLAGS;
	    else
		uFormat = LV_SL_DT_FLAGS;
	    
    	    DrawTextW (hdc, lpLVItem->pszText, -1, &rcText, uFormat | DT_CALCRECT);

	    labelSize.cx = min(rcText.right - rcText.left + TRAILING_LABEL_PADDING, infoPtr->nItemWidth);
	    labelSize.cy = rcText.bottom - rcText.top;

    	    SelectObject(hdc, hOldFont);
    	    ReleaseDC(infoPtr->hwndSelf, hdc);
	}

calc_label:
	if (uView == LVS_ICON)
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
	else /* LVS_SMALLICON, LVS_LIST or LVS_REPORT */
	{
	    Label.left = Icon.right;
	    Label.top = Box.top;
	    Label.right = min(Label.left + labelSize.cx, Label.right);
	    Label.bottom = Label.top + infoPtr->nItemHeight;
	}
  
	if (lprcLabel) *lprcLabel = Label;
	TRACE("    - label=%s\n", debugrect(&Label));
    }

    /* Fix the Box if necessary */
    if (lprcBox)
    {
	if (oversizedBox) UnionRect(lprcBox, &Box, &Label);
	else *lprcBox = Box;
    }
    TRACE("    - box=%s\n", debugrect(&Box));
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
static void LISTVIEW_GetItemBox(LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lprcBox)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    POINT Position, Origin;
    LVITEMW lvItem;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);

    /* Be smart and try to figure out the minimum we have to do */
    lvItem.mask = 0;
    if (uView == LVS_ICON && infoPtr->bFocus && LISTVIEW_GetItemState(infoPtr, nItem, LVIS_FOCUSED))
	lvItem.mask |= LVIF_TEXT;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.pszText = szDispText;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    if (lvItem.mask) LISTVIEW_GetItemW(infoPtr, &lvItem);
    if (uView == LVS_ICON)
    {
	lvItem.mask |= LVIF_STATE;
	lvItem.stateMask = LVIS_FOCUSED;
	lvItem.state = (lvItem.mask & LVIF_TEXT ? LVIS_FOCUSED : 0);
    }
    LISTVIEW_GetItemMetrics(infoPtr, &lvItem, lprcBox, 0, 0, 0);

    OffsetRect(lprcBox, Position.x + Origin.x, Position.y + Origin.y);
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
static BOOL LISTVIEW_MoveIconTo(LISTVIEW_INFO *infoPtr, INT nItem, const POINT *lppt, BOOL isNew)
{
    POINT old;
    
    if (!isNew)
    { 
        old.x = (LONG)DPA_GetPtr(infoPtr->hdpaPosX, nItem);
        old.y = (LONG)DPA_GetPtr(infoPtr->hdpaPosY, nItem);
    
        if (lppt->x == old.x && lppt->y == old.y) return TRUE;
	LISTVIEW_InvalidateItem(infoPtr, nItem);
    }

    /* Allocating a POINTER for every item is too resource intensive,
     * so we'll keep the (x,y) in different arrays */
    if (!DPA_SetPtr(infoPtr->hdpaPosX, nItem, (void *)lppt->x)) return FALSE;
    if (!DPA_SetPtr(infoPtr->hdpaPosY, nItem, (void *)lppt->y)) return FALSE;

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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    void (*next_pos)(LISTVIEW_INFO *, LPPOINT);
    POINT pos;
    INT i;

    if (uView != LVS_ICON && uView != LVS_SMALLICON) return FALSE;
  
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
    
    infoPtr->bAutoarrange = TRUE;
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
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lprcView : bounding rectangle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static void LISTVIEW_GetAreaRect(LISTVIEW_INFO *infoPtr, LPRECT lprcView)
{
    INT i, x, y;

    SetRectEmpty(lprcView);

    switch (infoPtr->dwStyle & LVS_TYPEMASK)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
	for (i = 0; i < infoPtr->nItemCount; i++)
	{
	    x = (LONG)DPA_GetPtr(infoPtr->hdpaPosX, i);
           y = (LONG)DPA_GetPtr(infoPtr->hdpaPosY, i);
	    lprcView->right = max(lprcView->right, x);
	    lprcView->bottom = max(lprcView->bottom, y);
	}
	if (infoPtr->nItemCount > 0)
	{
	    lprcView->right += infoPtr->nItemWidth;
	    lprcView->bottom += infoPtr->nItemHeight;
	}
	break;

    case LVS_LIST:
	y = LISTVIEW_GetCountPerColumn(infoPtr);
	x = infoPtr->nItemCount / y;
	if (infoPtr->nItemCount % y) x++;
	lprcView->right = x * infoPtr->nItemWidth;
	lprcView->bottom = y * infoPtr->nItemHeight;
	break;
	
    case LVS_REPORT:	    
    	lprcView->right = infoPtr->nItemWidth;
	lprcView->bottom = infoPtr->nItemCount * infoPtr->nItemHeight;
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
static BOOL LISTVIEW_GetViewRect(LISTVIEW_INFO *infoPtr, LPRECT lprcView)
{
    POINT ptOrigin;

    TRACE("(lprcView=%p)\n", lprcView);

    if (!lprcView) return FALSE;
 
    LISTVIEW_GetOrigin(infoPtr, &ptOrigin);
    LISTVIEW_GetAreaRect(infoPtr, lprcView); 
    OffsetRect(lprcView, ptOrigin.x, ptOrigin.y); 

    TRACE("lprcView=%s\n", debugrect(lprcView));

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
	lpSubItem = (SUBITEM_INFO *)DPA_GetPtr(hdpaSubItems, i);
	if (lpSubItem->iSubItem == nSubItem)
	    return lpSubItem;
    }

    return NULL;
}


/***
 * DESCRIPTION:
 * Caclulates the desired item width.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *  The desired item width.
 */
static INT LISTVIEW_CalculateItemWidth(LISTVIEW_INFO *infoPtr)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT nItemWidth = 0;

    TRACE("uView=%d\n", uView);

    if (uView == LVS_ICON)
	nItemWidth = infoPtr->iconSpacing.cx;
    else if (uView == LVS_REPORT)
    {
	RECT rcHeader;

	if (DPA_GetPtrCount(infoPtr->hdpaColumns) > 0)
	{
	    LISTVIEW_GetHeaderRect(infoPtr, DPA_GetPtrCount(infoPtr->hdpaColumns) - 1, &rcHeader);
            nItemWidth = rcHeader.right;
	}
    }
    else /* LVS_SMALLICON, or LVS_LIST */
    {
	INT i;
	
	for (i = 0; i < infoPtr->nItemCount; i++)
	    nItemWidth = max(LISTVIEW_GetLabelWidth(infoPtr, i), nItemWidth);

        if (infoPtr->himlSmall) nItemWidth += infoPtr->iconSize.cx; 
        if (infoPtr->himlState) nItemWidth += infoPtr->iconStateSize.cx;

	nItemWidth = max(DEFAULT_COLUMN_WIDTH, nItemWidth + WIDTH_PADDING);
    }

    return max(nItemWidth, 1);
}

/***
 * DESCRIPTION:
 * Caclulates the desired item height.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *  The desired item height.
 */
static INT LISTVIEW_CalculateItemHeight(LISTVIEW_INFO *infoPtr)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT nItemHeight;

    TRACE("uView=%d\n", uView);

    if (uView == LVS_ICON)
	nItemHeight = infoPtr->iconSpacing.cy;
    else
    {
	nItemHeight = infoPtr->ntmHeight; 
	if (infoPtr->himlState)
	    nItemHeight = max(nItemHeight, infoPtr->iconStateSize.cy);
	if (infoPtr->himlSmall)
	    nItemHeight = max(nItemHeight, infoPtr->iconSize.cy);
	if (infoPtr->himlState || infoPtr->himlSmall)
	    nItemHeight += HEIGHT_PADDING;
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

    if (GetTextMetricsW(hdc, &tm))
    {
	infoPtr->ntmHeight = tm.tmHeight;
	infoPtr->ntmAveCharWidth = tm.tmAveCharWidth;
    }
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
    
    TRACE("range1=%s, range2=%s, cmp=%d\n", debugrange((RANGE*)range1), debugrange((RANGE*)range2), cmp);

    return cmp;
}

#if DEBUG_RANGES
#define ranges_check(ranges, desc) ranges_assert(ranges, desc, __FUNCTION__, __LINE__)
#else
#define ranges_check(ranges, desc) do { } while(0)
#endif

static void ranges_assert(RANGES ranges, LPCSTR desc, const char *func, int line)
{
    INT i;
    RANGE *prev, *curr;
    
    TRACE("*** Checking %s:%d:%s ***\n", func, line, desc);
    assert (ranges);
    assert (DPA_GetPtrCount(ranges->hdpa) >= 0);
    ranges_dump(ranges);
    prev = (RANGE *)DPA_GetPtr(ranges->hdpa, 0);
    if (DPA_GetPtrCount(ranges->hdpa) > 0)
	assert (prev->lower >= 0 && prev->lower < prev->upper);
    for (i = 1; i < DPA_GetPtrCount(ranges->hdpa); i++)
    {
	curr = (RANGE *)DPA_GetPtr(ranges->hdpa, i);
	assert (prev->upper <= curr->lower);
	assert (curr->lower < curr->upper);
	prev = curr;
    }
    TRACE("--- Done checking---\n");
}

static RANGES ranges_create(int count)
{
    RANGES ranges = (RANGES)Alloc(sizeof(struct tagRANGES));
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
        RANGE *newrng = (RANGE *)Alloc(sizeof(RANGE));
	if (!newrng) goto fail;
	*newrng = *((RANGE*)DPA_GetPtr(ranges->hdpa, i));
	DPA_SetPtr(clone->hdpa, i, newrng);
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
        newrgn = (RANGE *)Alloc(sizeof(RANGE));
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

        /* merge now common anges */
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
	    break;
	}
	/* case 2: engulf */
	else if ( (chkrgn->upper <= range.upper) &&
		  (chkrgn->lower >= range.lower) ) 
	{
	    DPA_DeletePtr(ranges->hdpa, index);
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
	    RANGE tmprgn = *chkrgn, *newrgn;

	    if (!(newrgn = (RANGE *)Alloc(sizeof(RANGE)))) goto fail;
	    newrgn->lower = chkrgn->lower;
	    newrgn->upper = range.lower;
	    chkrgn->lower = range.upper;
	    if (DPA_InsertPtr(ranges->hdpa, index, newrgn) == -1)
	    {
		Free(newrgn);
		goto fail;
	    }
	    chkrgn = &tmprgn;
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
*   FAILURE : TRUE
*/
static BOOL LISTVIEW_DeselectAllSkipItems(LISTVIEW_INFO *infoPtr, RANGES toSkip)
{
    LVITEMW lvItem;
    ITERATOR i;
    RANGES clone;

    TRACE("()\n");

    lvItem.state = 0;
    lvItem.stateMask = LVIS_SELECTED;
    
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
static INT LISTVIEW_GetSelectedCount(LISTVIEW_INFO *infoPtr)
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

/* Helper function for LISTVIEW_ShiftIndices *only* */
static INT shift_item(LISTVIEW_INFO *infoPtr, INT nShiftItem, INT nItem, INT direction)
{
    if (nShiftItem < nItem) return nShiftItem;

    if (nShiftItem > nItem) return nShiftItem + direction;

    if (direction > 0) return nShiftItem + direction;

    return min(nShiftItem, infoPtr->nItemCount - 1);
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
    INT nNewFocus;
    BOOL bOldChange;

    /* temporarily disable change notification while shifting items */
    bOldChange = infoPtr->bDoChangeNotify;
    infoPtr->bDoChangeNotify = FALSE;

    TRACE("Shifting %iu, %i steps\n", nItem, direction);

    ranges_shift(infoPtr->selectionRanges, nItem, direction, infoPtr->nItemCount);

    assert(abs(direction) == 1);

    infoPtr->nSelectionMark = shift_item(infoPtr, infoPtr->nSelectionMark, nItem, direction);

    nNewFocus = shift_item(infoPtr, infoPtr->nFocusedItem, nItem, direction);
    if (nNewFocus != infoPtr->nFocusedItem)
        LISTVIEW_SetItemFocus(infoPtr, nNewFocus);
    
    /* But we are not supposed to modify nHotItem! */

    infoPtr->bDoChangeNotify = bOldChange;
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
 * None
 */
static void LISTVIEW_AddGroupSelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
    INT nFirst = min(infoPtr->nSelectionMark, nItem);
    INT nLast = max(infoPtr->nSelectionMark, nItem);
    INT i;
    LVITEMW item;

    if (nFirst == -1) nFirst = nItem;

    item.state = LVIS_SELECTED;
    item.stateMask = LVIS_SELECTED;

    /* FIXME: this is not correct LVS_OWNERDATA
     * setting the item states individually will generate
     * a LVN_ITEMCHANGED notification for each one. Instead,
     * we have to send a LVN_ODSTATECHANGED notification.
     * See MSDN documentation for LVN_ITEMCHANGED.
     */
    for (i = nFirst; i <= nLast; i++)
	LISTVIEW_SetItemState(infoPtr,i,&item);
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    RANGES selection;
    LVITEMW item;
    ITERATOR i;

    if (!(selection = ranges_create(100))) return;

    item.state = LVIS_SELECTED; 
    item.stateMask = LVIS_SELECTED;

    if ((uView == LVS_LIST) || (uView == LVS_REPORT))
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
	if (!LISTVIEW_GetItemRect(infoPtr, nItem, &rcItem)) return;
	rcSelMark.left = LVIR_BOUNDS;
	if (!LISTVIEW_GetItemRect(infoPtr, infoPtr->nSelectionMark, &rcSelMark)) return;
	UnionRect(&rcSel, &rcItem, &rcSelMark);
	iterator_frameditems(&i, infoPtr, &rcSel);
	while(iterator_next(&i))
	{
	    LISTVIEW_GetItemPosition(infoPtr, i.nItem, &ptItem);
	    if (PtInRect(&rcSel, ptItem)) ranges_additem(selection, i.nItem);
	}
	iterator_destroy(&i);
    }

    LISTVIEW_DeselectAllSkipItems(infoPtr, selection);
    iterator_rangesitems(&i, selection);
    while(iterator_next(&i))
	LISTVIEW_SetItemState(infoPtr, i.nItem, &item);
    /* this will also destroy the selection */
    iterator_destroy(&i);
    
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
 *
 * RETURN:
 *   SUCCESS : TRUE (needs to be repainted)
 *   FAILURE : FALSE (nothing has changed)
 */
static BOOL LISTVIEW_KeySelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
  /* FIXME: pass in the state */
  WORD wShift = HIWORD(GetKeyState(VK_SHIFT));
  WORD wCtrl = HIWORD(GetKeyState(VK_CONTROL));
  BOOL bResult = FALSE;

  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    if (infoPtr->dwStyle & LVS_SINGLESEL)
    {
      bResult = TRUE;
      LISTVIEW_SetSelection(infoPtr, nItem);
    }
    else
    {
      if (wShift)
      {
        bResult = TRUE;
        LISTVIEW_SetGroupSelection(infoPtr, nItem);
      }
      else if (wCtrl)
      {
        bResult = LISTVIEW_SetItemFocus(infoPtr, nItem);
      }
      else
      {
        bResult = TRUE;
        LISTVIEW_SetSelection(infoPtr, nItem);
      }
    }
    LISTVIEW_EnsureVisible(infoPtr, nItem, FALSE);
  }

  UpdateWindow(infoPtr->hwndSelf); /* update client area */
  return bResult;
}


/***
 * DESCRIPTION:
 * Called when the mouse is being actively tracked and has hovered for a specified
 * amount of time
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fwKeys : key indicator
 * [I] pts : mouse position
 *
 * RETURN:
 *   0 if the message was processed, non-zero if there was an error
 *
 * INFO:
 * LVS_EX_TRACKSELECT: An item is automatically selected when the cursor remains
 * over the item for a certain period of time.
 *
 */
static LRESULT LISTVIEW_MouseHover(LISTVIEW_INFO *infoPtr, WORD fwKyes, POINTS pts)
{
    if(infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT)
	/* FIXME: select the item!!! */
	/*LISTVIEW_GetItemAtPt(infoPtr, pt)*/;

    return 0;
}

/***
 * DESCRIPTION:
 * Called whenever WM_MOUSEMOVE is received.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fwKeys : key indicator
 * [I] pts : mouse position
 *
 * RETURN:
 *   0 if the message is processed, non-zero if there was an error
 */
static LRESULT LISTVIEW_MouseMove(LISTVIEW_INFO *infoPtr, WORD fwKeys, POINTS pts)
{
  TRACKMOUSEEVENT trackinfo;

  /* see if we are supposed to be tracking mouse hovering */
  if(infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT) {
     /* fill in the trackinfo struct */
     trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
     trackinfo.dwFlags = TME_QUERY;
     trackinfo.hwndTrack = infoPtr->hwndSelf;
     trackinfo.dwHoverTime = infoPtr->dwHoverTime;

     /* see if we are already tracking this hwnd */
     _TrackMouseEvent(&trackinfo);

     if(!(trackinfo.dwFlags & TME_HOVER)) {
       trackinfo.dwFlags = TME_HOVER;

       /* call TRACKMOUSEEVENT so we receive WM_MOUSEHOVER messages */
       _TrackMouseEvent(&trackinfo);
    }
  }

  return 0;
}


/***
 * Tests wheather the item is assignable to a list with style lStyle 
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
 * Helper for LISTVIEW_SetItemT *only*: sets item attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : valid pointer to new item atttributes
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    ITEM_INFO *lpItem;
    NMLISTVIEW nmlv;
    UINT uChanged = 0;
    LVITEMW item;

    TRACE("()\n");

    assert(lpLVItem->iItem >= 0 && lpLVItem->iItem < infoPtr->nItemCount);
    
    if (lpLVItem->mask == 0) return TRUE;   

    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	/* a virtual listview we stores only selection and focus */
	if (lpLVItem->mask & ~LVIF_STATE)
	    return FALSE;
	lpItem = NULL;
    }
    else
    {
	HDPA hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
	lpItem = (ITEM_INFO *)DPA_GetPtr(hdpaSubItems, 0);
	assert (lpItem);
    }

    /* we need to get the lParam and state of the item */
    item.iItem = lpLVItem->iItem;
    item.iSubItem = lpLVItem->iSubItem;
    item.mask = LVIF_STATE | LVIF_PARAM;
    item.stateMask = ~0;
    item.state = 0;
    item.lParam = 0;
    if (!isNew && !LISTVIEW_GetItemW(infoPtr, &item)) return FALSE;

    TRACE("oldState=%x, newState=%x\n", item.state, lpLVItem->state);
    /* determine what fields will change */    
    if ((lpLVItem->mask & LVIF_STATE) && ((item.state ^ lpLVItem->state) & lpLVItem->stateMask & ~infoPtr->uCallbackMask))
	uChanged |= LVIF_STATE;

    if ((lpLVItem->mask & LVIF_IMAGE) && (lpItem->hdr.iImage != lpLVItem->iImage))
	uChanged |= LVIF_IMAGE;

    if ((lpLVItem->mask & LVIF_PARAM) && (lpItem->lParam != lpLVItem->lParam))
	uChanged |= LVIF_PARAM;

    if ((lpLVItem->mask & LVIF_INDENT) && (lpItem->iIndent != lpLVItem->iIndent))
	uChanged |= LVIF_INDENT;

    if ((lpLVItem->mask & LVIF_TEXT) && textcmpWT(lpItem->hdr.pszText, lpLVItem->pszText, isW))
	uChanged |= LVIF_TEXT;
   
    TRACE("uChanged=0x%x\n", uChanged); 
    if (!uChanged) return TRUE;
    *bChanged = TRUE;
    
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    nmlv.iItem = lpLVItem->iItem;
    nmlv.uNewState = (item.state & ~lpLVItem->stateMask) | (lpLVItem->state & lpLVItem->stateMask);
    nmlv.uOldState = item.state;
    nmlv.uChanged = uChanged;
    nmlv.lParam = item.lParam;
    
    /* send LVN_ITEMCHANGING notification, if the item is not being inserted */
    /* and we are _NOT_ virtual (LVS_OWERNDATA), and change notifications */
    /* are enabled */
    if(lpItem && !isNew && infoPtr->bDoChangeNotify &&
       notify_listview(infoPtr, LVN_ITEMCHANGING, &nmlv))
	return FALSE;

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
	if (lpItem && (lpLVItem->stateMask & ~infoPtr->uCallbackMask & ~(LVIS_FOCUSED | LVIS_SELECTED)))
	{
	    lpItem->state &= ~lpLVItem->stateMask;
	    lpItem->state |= (lpLVItem->state & lpLVItem->stateMask);
	}
	if (lpLVItem->state & lpLVItem->stateMask & ~infoPtr->uCallbackMask & LVIS_SELECTED)
	{
	    if (infoPtr->dwStyle & LVS_SINGLESEL) LISTVIEW_DeselectAllSkipItem(infoPtr, lpLVItem->iItem);
	    ranges_additem(infoPtr->selectionRanges, lpLVItem->iItem);
	}
	else if (lpLVItem->stateMask & LVIS_SELECTED)
	    ranges_delitem(infoPtr->selectionRanges, lpLVItem->iItem);
	
	/* if we are asked to change focus, and we manage it, do it */
	if (lpLVItem->state & lpLVItem->stateMask & ~infoPtr->uCallbackMask & LVIS_FOCUSED)
	{
	    if (lpLVItem->state & LVIS_FOCUSED)
	    {
		LISTVIEW_SetItemFocus(infoPtr, -1);
		infoPtr->nFocusedItem = lpLVItem->iItem;
    	        LISTVIEW_EnsureVisible(infoPtr, lpLVItem->iItem, uView == LVS_LIST);
	    }
	    else if (infoPtr->nFocusedItem == lpLVItem->iItem)
		infoPtr->nFocusedItem = -1;
	}
    }

    /* if we're inserting the item, we're done */
    if (isNew) return TRUE;
    
    /* send LVN_ITEMCHANGED notification */
    if (lpLVItem->mask & LVIF_PARAM) nmlv.lParam = lpLVItem->lParam;
    if (infoPtr->bDoChangeNotify) notify_listview(infoPtr, LVN_ITEMCHANGED, &nmlv);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Helper for LISTVIEW_{Set,Insert}ItemT *only*: sets subitem attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : valid pointer to new subitem atttributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 * [O] bChanged : will be set to TRUE if the item really changed
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL set_sub_item(LISTVIEW_INFO *infoPtr, const LVITEMW *lpLVItem, BOOL isW, BOOL *bChanged)
{
    HDPA hdpaSubItems;
    SUBITEM_INFO *lpSubItem;

    /* we do not support subitems for virtual listviews */
    if (infoPtr->dwStyle & LVS_OWNERDATA) return FALSE;
    
    /* set subitem only if column is present */
    if (lpLVItem->iSubItem >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return FALSE;
   
    /* First do some sanity checks */
    if (lpLVItem->mask & ~(LVIF_TEXT | LVIF_IMAGE)) return FALSE;
    if (!(lpLVItem->mask & (LVIF_TEXT | LVIF_IMAGE))) return TRUE;
   
    /* get the subitem structure, and create it if not there */
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    assert (hdpaSubItems);
    
    lpSubItem = LISTVIEW_GetSubItemPtr(hdpaSubItems, lpLVItem->iSubItem);
    if (!lpSubItem)
    {
	SUBITEM_INFO *tmpSubItem;
	INT i;

	lpSubItem = (SUBITEM_INFO *)Alloc(sizeof(SUBITEM_INFO));
	if (!lpSubItem) return FALSE;
	/* we could binary search here, if need be...*/
  	for (i = 1; i < DPA_GetPtrCount(hdpaSubItems); i++)
  	{
	    tmpSubItem = (SUBITEM_INFO *)DPA_GetPtr(hdpaSubItems, i);
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
    
    if (lpLVItem->mask & LVIF_IMAGE)
	if (lpSubItem->hdr.iImage != lpLVItem->iImage)
	{
	    lpSubItem->hdr.iImage = lpLVItem->iImage;
	    *bChanged = TRUE;
	}

    if (lpLVItem->mask & LVIF_TEXT)
	if (lpSubItem->hdr.pszText != lpLVItem->pszText)
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
 * [I] lpLVItem : new item atttributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemT(LISTVIEW_INFO *infoPtr, const LVITEMW *lpLVItem, BOOL isW)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    LPWSTR pszText = NULL;
    BOOL bResult, bChanged = FALSE;
    
    TRACE("(lpLVItem=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (!lpLVItem || lpLVItem->iItem < 0 || lpLVItem->iItem >= infoPtr->nItemCount)
	return FALSE;

    /* For efficiency, we transform the lpLVItem->pszText to Unicode here */
    if ((lpLVItem->mask & LVIF_TEXT) && is_textW(lpLVItem->pszText))
    {
	pszText = lpLVItem->pszText;
	((LVITEMW *)lpLVItem)->pszText = textdupTtoW(lpLVItem->pszText, isW);
    }
    
    /* actually set the fields */
    if (!is_assignable_item(lpLVItem, infoPtr->dwStyle)) return FALSE;
    
    if (lpLVItem->iSubItem)
	bResult = set_sub_item(infoPtr, lpLVItem, TRUE, &bChanged);
    else
	bResult = set_main_item(infoPtr, lpLVItem, FALSE, TRUE, &bChanged);

    /* redraw item, if necessary */
    if (bChanged && !infoPtr->bIsDrawing)
    {
	/* this little optimization eliminates some nasty flicker */
	if ( uView == LVS_REPORT && !(infoPtr->dwStyle & LVS_OWNERDRAWFIXED) &&
	     (!(infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) || lpLVItem->iSubItem) )
	    LISTVIEW_InvalidateSubItem(infoPtr, lpLVItem->iItem, lpLVItem->iSubItem);
	else
	    LISTVIEW_InvalidateItem(infoPtr, lpLVItem->iItem);
    }
    /* restore text */
    if (pszText)
    {
	textfreeT(lpLVItem->pszText, isW);
	((LVITEMW *)lpLVItem)->pszText = pszText;
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
static INT LISTVIEW_GetTopIndex(LISTVIEW_INFO *infoPtr)
{
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT nItem = 0;
    SCROLLINFO scrollInfo;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;

    if (uView == LVS_LIST)
    {
	if ((lStyle & WS_HSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
	    nItem = scrollInfo.nPos * LISTVIEW_GetCountPerColumn(infoPtr);
    }
    else if (uView == LVS_REPORT)
    {
	if ((lStyle & WS_VSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	    nItem = scrollInfo.nPos;
    } 
    else
    {
	if ((lStyle & WS_VSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	    nItem = LISTVIEW_GetCountPerRow(infoPtr) * (scrollInfo.nPos / infoPtr->nItemHeight);
    }

    TRACE("nItem=%d\n", nItem);
    
    return nItem;
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
static inline BOOL LISTVIEW_FillBkgnd(LISTVIEW_INFO *infoPtr, HDC hdc, const RECT *lprcBox)
{
    if (!infoPtr->hBkBrush) return FALSE;

    TRACE("(hdc=%p, lprcBox=%s, hBkBrush=%p)\n", hdc, debugrect(lprcBox), infoPtr->hBkBrush);

    return FillRect(hdc, lprcBox, infoPtr->hBkBrush);
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
static BOOL LISTVIEW_DrawItem(LISTVIEW_INFO *infoPtr, HDC hdc, INT nItem, INT nSubItem, POINT pos, DWORD cdmode)
{
    UINT uFormat, uView = infoPtr->dwStyle & LVS_TYPEMASK;
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    WCHAR szCallback[] = { '(', 'c', 'a', 'l', 'l', 'b', 'a', 'c', 'k', ')', 0 };
    DWORD cdsubitemmode = CDRF_DODEFAULT;
    RECT* lprcFocus, rcSelect, rcBox, rcState, rcIcon, rcLabel;
    NMLVCUSTOMDRAW nmlvcd;
    HIMAGELIST himl;
    LVITEMW lvItem;

    TRACE("(hdc=%p, nItem=%d, nSubItem=%d, pos=%s)\n", hdc, nItem, nSubItem, debugpoint(&pos));

    /* get information needed for drawing the item */
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
    if (nSubItem == 0) lvItem.mask |= LVIF_STATE | LVIF_PARAM;
    if (uView == LVS_REPORT) lvItem.mask |= LVIF_INDENT;
    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK;
    lvItem.iItem = nItem;
    lvItem.iSubItem = nSubItem;
    lvItem.state = 0;
    lvItem.lParam = 0;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    lvItem.pszText = szDispText;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) return FALSE;
    if (nSubItem > 0 && (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT)) 
	lvItem.state = LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED);
    if (lvItem.pszText == LPSTR_TEXTCALLBACKW) lvItem.pszText = szCallback;
    TRACE("   lvItem=%s\n", debuglvitem_t(&lvItem, TRUE));

    /* now check if we need to update the focus rectangle */
    lprcFocus = infoPtr->bFocus && (lvItem.state & LVIS_FOCUSED) ? &infoPtr->rcFocus : 0;
    
    if (!lprcFocus) lvItem.state &= ~LVIS_FOCUSED;
    LISTVIEW_GetItemMetrics(infoPtr, &lvItem, &rcBox, &rcState, &rcIcon, &rcLabel);
    OffsetRect(&rcBox, pos.x, pos.y);
    OffsetRect(&rcState, pos.x, pos.y);
    OffsetRect(&rcIcon, pos.x, pos.y);
    OffsetRect(&rcLabel, pos.x, pos.y);
    TRACE("    rcBox=%s, rcState=%s, rcIcon=%s. rcLabel=%s\n", 
	debugrect(&rcBox), debugrect(&rcState), debugrect(&rcIcon), debugrect(&rcLabel));

    /* fill in the custom draw structure */
    customdraw_fill(&nmlvcd, infoPtr, hdc, &rcBox, &lvItem);

    if (nSubItem > 0) cdmode = infoPtr->cditemmode;
    if (cdmode & CDRF_NOTIFYITEMDRAW)
        cdsubitemmode = notify_customdraw(infoPtr, CDDS_PREPAINT, &nmlvcd);
    if (nSubItem == 0) infoPtr->cditemmode = cdsubitemmode;
    if (cdsubitemmode & CDRF_SKIPDEFAULT) goto postpaint;
    /* we have to send a CDDS_SUBITEM customdraw explicitly for subitem 0 */
    if (nSubItem == 0 && cdsubitemmode == CDRF_NOTIFYITEMDRAW)
    {
        cdsubitemmode = notify_customdraw(infoPtr, CDDS_SUBITEM | CDDS_ITEMPREPAINT, &nmlvcd);
        if (cdsubitemmode & CDRF_SKIPDEFAULT) goto postpaint;
    }
    if (nSubItem == 0 || (cdmode & CDRF_NOTIFYITEMDRAW))
        prepaint_setup(infoPtr, hdc, &nmlvcd);

    /* in full row select, subitems, will just use main item's colors */
    if (nSubItem && uView == LVS_REPORT && (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT))
	nmlvcd.clrTextBk = CLR_NONE;
    
    /* state icons */
    if (infoPtr->himlState && !IsRectEmpty(&rcState))
    {
        UINT uStateImage = (lvItem.state & LVIS_STATEIMAGEMASK) >> 12;
        if (uStateImage)
	{
	     TRACE("uStateImage=%d\n", uStateImage);
	     ImageList_Draw(infoPtr->himlState, uStateImage - 1, hdc, rcState.left, rcState.top, ILD_NORMAL);
	}
    }

    /* small icons */
    himl = (uView == LVS_ICON ? infoPtr->himlNormal : infoPtr->himlSmall);
    if (himl && lvItem.iImage >= 0 && !IsRectEmpty(&rcIcon))
    {
	TRACE("iImage=%d\n", lvItem.iImage);
	ImageList_Draw(himl, lvItem.iImage, hdc, rcIcon.left, rcIcon.top,
			(lvItem.state & LVIS_SELECTED) && (infoPtr->bFocus) ? ILD_SELECTED : ILD_NORMAL);
    }

    /* Don't bother painting item being edited */
    if (infoPtr->hwndEdit && nItem == infoPtr->nEditLabelItem && nSubItem == 0) goto postpaint;

    /* draw the selection background, if we're drawing the main item */
    if (nSubItem == 0)
    {
	rcSelect = rcLabel;
    	if (uView == LVS_REPORT && (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT))
	    rcSelect.right = rcBox.right;
   
    	if (nmlvcd.clrTextBk != CLR_NONE) 
            ExtTextOutW(hdc, rcSelect.left, rcSelect.top, ETO_OPAQUE, &rcSelect, 0, 0, 0);
    	if(lprcFocus) *lprcFocus = rcSelect;
    }
   
    /* figure out the text drawing flags */
    uFormat = (uView == LVS_ICON ? (lprcFocus ? LV_FL_DT_FLAGS : LV_ML_DT_FLAGS) : LV_SL_DT_FLAGS);
    if (uView == LVS_ICON)
	uFormat = (lprcFocus ? LV_FL_DT_FLAGS : LV_ML_DT_FLAGS);
    else if (nSubItem)
    {
	switch (LISTVIEW_GetColumnInfo(infoPtr, nSubItem)->fmt & LVCFMT_JUSTIFYMASK)
	{
	case LVCFMT_RIGHT:  uFormat |= DT_RIGHT;  break;
	case LVCFMT_CENTER: uFormat |= DT_CENTER; break;
	default:            uFormat |= DT_LEFT;
	}
    }
    if (!(uFormat & (DT_RIGHT | DT_CENTER)))
    {
        if (himl && lvItem.iImage >= 0 && !IsRectEmpty(&rcIcon)) rcLabel.left += IMAGE_PADDING;
        else rcLabel.left += LABEL_HOR_PADDING;
    }
    else if (uFormat & DT_RIGHT) rcLabel.right -= LABEL_HOR_PADDING;
    DrawTextW(hdc, lvItem.pszText, -1, &rcLabel, uFormat);

postpaint:
    if (cdsubitemmode & CDRF_NOTIFYPOSTPAINT)
        notify_postpaint(infoPtr, &nmlvcd);
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
static void LISTVIEW_RefreshOwnerDraw(LISTVIEW_INFO *infoPtr, ITERATOR *i, HDC hdc, DWORD cdmode)
{
    UINT uID = GetWindowLongW(infoPtr->hwndSelf, GWL_ID);
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

	TRACE("item=%s, rcItem=%s\n", debuglvitem_t(&item, TRUE), debugrect(&dis.rcItem));

    /*
     * Even if we do not send the CDRF_NOTIFYITEMDRAW we need to fill the nmlvcd
     * structure for the rest. of the paint cycle
     */
	customdraw_fill(&nmlvcd, infoPtr, hdc, &dis.rcItem, &item);
	if (cdmode & CDRF_NOTIFYITEMDRAW)
            cditemmode = notify_customdraw(infoPtr, CDDS_PREPAINT, &nmlvcd);
    
	if (!(cditemmode & CDRF_SKIPDEFAULT))
	{
            prepaint_setup (infoPtr, hdc, &nmlvcd);
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
    POINT Origin, Position;
    RANGE colRange;
    ITERATOR j;

    TRACE("()\n");

    /* figure out what to draw */
    rgntype = GetClipBox(hdc, &rcClip);
    if (rgntype == NULLREGION) return;
    
    /* Get scroll info once before loop */
    LISTVIEW_GetOrigin(infoPtr, &Origin);
    
    /* narrow down the columns we need to paint */
    for(colRange.lower = 0; colRange.lower < DPA_GetPtrCount(infoPtr->hdpaColumns); colRange.lower++)
    {
	LISTVIEW_GetHeaderRect(infoPtr, colRange.lower, &rcItem);
	if (rcItem.right + Origin.x >= rcClip.left) break;
    }
    for(colRange.upper = DPA_GetPtrCount(infoPtr->hdpaColumns); colRange.upper > 0; colRange.upper--)
    {
	LISTVIEW_GetHeaderRect(infoPtr, colRange.upper - 1, &rcItem);
	if (rcItem.left + Origin.x < rcClip.right) break;
    }
    iterator_rangeitems(&j, colRange);

    /* in full row select, we _have_ to draw the main item */
    if (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT)
	j.nSpecial = 0;

    /* iterate through the invalidated rows */
    while(iterator_next(i))
    {
	/* iterate through the invalidated columns */
	while(iterator_next(&j))
	{
	    LISTVIEW_GetItemOrigin(infoPtr, i->nItem, &Position);
	    Position.x += Origin.x;
	    Position.y += Origin.y;

	    if (rgntype == COMPLEXREGION && !((infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) && j.nItem == 0))
	    {
		LISTVIEW_GetHeaderRect(infoPtr, j.nItem, &rcItem);
		rcItem.top = 0;
	        rcItem.bottom = infoPtr->nItemHeight;
		OffsetRect(&rcItem, Position.x, Position.y);
		if (!RectVisible(hdc, &rcItem)) continue;
	    }

	    LISTVIEW_DrawItem(infoPtr, hdc, i->nItem, j.nItem, Position, cdmode);
	}
    }
    iterator_destroy(&j);
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
	LISTVIEW_GetItemOrigin(infoPtr, i->nItem, &Position);
	Position.x += Origin.x;
	Position.y += Origin.y;

        LISTVIEW_DrawItem(infoPtr, hdc, i->nItem, 0, Position, cdmode);
    }
}


/***
 * DESCRIPTION:
 * Draws listview items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 *
 * RETURN:
 * NoneX
 */
static void LISTVIEW_Refresh(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    COLORREF oldTextColor, oldClrTextBk, oldClrText;
    NMLVCUSTOMDRAW nmlvcd;
    HFONT hOldFont;
    DWORD cdmode;
    INT oldBkMode;
    RECT rcClient;
    ITERATOR i;

    LISTVIEW_DUMP(infoPtr);
  
    infoPtr->bIsDrawing = TRUE;

    /* save dc values we're gonna trash while drawing */
    hOldFont = SelectObject(hdc, infoPtr->hFont);
    oldBkMode = GetBkMode(hdc);
    infoPtr->clrTextBkDefault = GetBkColor(hdc);
    oldTextColor = GetTextColor(hdc);

    oldClrTextBk = infoPtr->clrTextBk;
    oldClrText   = infoPtr->clrText;
   
    infoPtr->cditemmode = CDRF_DODEFAULT;

    GetClientRect(infoPtr->hwndSelf, &rcClient);
    customdraw_fill(&nmlvcd, infoPtr, hdc, &rcClient, 0);
    cdmode = notify_customdraw(infoPtr, CDDS_PREPAINT, &nmlvcd);
    if (cdmode & CDRF_SKIPDEFAULT) goto enddraw;
    prepaint_setup(infoPtr, hdc, &nmlvcd);

    /* Use these colors to draw the items */
    infoPtr->clrTextBk = nmlvcd.clrTextBk;
    infoPtr->clrText = nmlvcd.clrText;

    /* nothing to draw */
    if(infoPtr->nItemCount == 0) goto enddraw;

    /* figure out what we need to draw */
    iterator_visibleitems(&i, infoPtr, hdc);
    
    /* send cache hint notification */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
    	RANGE range = iterator_range(&i);
	NMLVCACHEHINT nmlv;
	
    	ZeroMemory(&nmlv, sizeof(NMLVCACHEHINT));
    	nmlv.iFrom = range.lower;
    	nmlv.iTo   = range.upper - 1;
    	notify_hdr(infoPtr, LVN_ODCACHEHINT, &nmlv.hdr);
    }

    if ((infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && (uView == LVS_REPORT))
	LISTVIEW_RefreshOwnerDraw(infoPtr, &i, hdc, cdmode);
    else
    {
    	if (uView == LVS_REPORT)
            LISTVIEW_RefreshReport(infoPtr, &i, hdc, cdmode);
	else /* LVS_LIST, LVS_ICON or LVS_SMALLICON */
	    LISTVIEW_RefreshList(infoPtr, &i, hdc, cdmode);

	/* if we have a focus rect, draw it */
	if (infoPtr->bFocus)
	    DrawFocusRect(hdc, &infoPtr->rcFocus);
    }
    iterator_destroy(&i);
    
enddraw:
    if (cdmode & CDRF_NOTIFYPOSTPAINT)
	notify_postpaint(infoPtr, &nmlvcd);

    infoPtr->clrTextBk = oldClrTextBk;
    infoPtr->clrText = oldClrText;

    SelectObject(hdc, hOldFont);
    SetBkMode(hdc, oldBkMode);
    SetBkColor(hdc, infoPtr->clrTextBkDefault);
    SetTextColor(hdc, oldTextColor);
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
static DWORD LISTVIEW_ApproximateViewRect(LISTVIEW_INFO *infoPtr, INT nItemCount,
                                            WORD wWidth, WORD wHeight)
{
  UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
  INT nItemCountPerColumn = 1;
  INT nColumnCount = 0;
  DWORD dwViewRect = 0;

  if (nItemCount == -1)
    nItemCount = infoPtr->nItemCount;

  if (uView == LVS_LIST)
  {
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
  else if (uView == LVS_REPORT)
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
  else if (uView == LVS_SMALLICON)
    FIXME("uView == LVS_SMALLICON: not implemented\n");
  else if (uView == LVS_ICON)
    FIXME("uView == LVS_ICON: not implemented\n");

  return dwViewRect;
}


/***
 * DESCRIPTION:
 * Create a drag image list for the specified item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] iItem   : index of item
 * [O] lppt    : Upperr-left corner of the image
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
    HIMAGELIST dragList = 0;
    TRACE("iItem=%d Count=%d \n", iItem, infoPtr->nItemCount);

    if (iItem < 0 || iItem >= infoPtr->nItemCount)
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

    rcItem.left = rcItem.top = 0;
    rcItem.right = size.cx;
    rcItem.bottom = size.cy;
    FillRect(hdc, &rcItem, infoPtr->hBkBrush);
    
    pos.x = pos.y = 0;
    if (LISTVIEW_DrawItem(infoPtr, hdc, iItem, 0, pos, infoPtr->cditemmode))
    {
        dragList = ImageList_Create(size.cx, size.cy, ILC_COLOR, 10, 10);
        SelectObject(hdc, hOldbmp);
        ImageList_Add(dragList, hbmp, 0);
    }
    else
        SelectObject(hdc, hOldbmp);

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
static BOOL LISTVIEW_DeleteAllItems(LISTVIEW_INFO *infoPtr)
{
    NMLISTVIEW nmlv;
    HDPA hdpaSubItems = NULL;
    BOOL bSuppress;
    ITEMHDR *hdrItem;
    INT i, j;

    TRACE("()\n");

    /* we do it directly, to avoid notifications */
    ranges_clear(infoPtr->selectionRanges);
    infoPtr->nSelectionMark = -1;
    infoPtr->nFocusedItem = -1;
    SetRectEmpty(&infoPtr->rcFocus);
    /* But we are supposed to leave nHotItem as is! */


    /* send LVN_DELETEALLITEMS notification */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    nmlv.iItem = -1;
    bSuppress = notify_listview(infoPtr, LVN_DELETEALLITEMS, &nmlv);

    for (i = infoPtr->nItemCount - 1; i >= 0; i--)
    {
        /* send LVN_DELETEITEM notification, if not supressed */
	if (!bSuppress) notify_deleteitem(infoPtr, i);
	if (!(infoPtr->dwStyle & LVS_OWNERDATA))
	{
	    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, i);
	    for (j = 0; j < DPA_GetPtrCount(hdpaSubItems); j++)
	    {
		hdrItem = (ITEMHDR *)DPA_GetPtr(hdpaSubItems, j);
		if (is_textW(hdrItem->pszText)) Free(hdrItem->pszText);
		Free(hdrItem);
	    }
	    DPA_Destroy(hdpaSubItems);
	    DPA_DeletePtr(infoPtr->hdpaItems, i);
	}
	DPA_DeletePtr(infoPtr->hdpaPosX, i);
	DPA_DeletePtr(infoPtr->hdpaPosY, i);
	infoPtr->nItemCount --;
    }
    
    LISTVIEW_UpdateScroll(infoPtr);

    LISTVIEW_InvalidateList(infoPtr);
    
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
    INT nCol;
   
    if (nColumn < 0 || DPA_GetPtrCount(infoPtr->hdpaColumns) < 1) return;
    lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, min(nColumn, DPA_GetPtrCount(infoPtr->hdpaColumns) - 1));
    rcCol = lpColumnInfo->rcHeader;
    if (nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns))
	rcCol.left = rcCol.right;
    
    /* ajust the other columns */
    for (nCol = nColumn; nCol < DPA_GetPtrCount(infoPtr->hdpaColumns); nCol++)
    {
	lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, nCol);
        lpColumnInfo->rcHeader.left += dx;
        lpColumnInfo->rcHeader.right += dx;
    }

    /* do not update screen if not in report mode */
    if (!is_redrawing(infoPtr) || (infoPtr->dwStyle & LVS_TYPEMASK) != LVS_REPORT) return;
    
    /* if we have a focus, must first erase the focus rect */
    if (infoPtr->bFocus) LISTVIEW_ShowFocusRect(infoPtr, FALSE);
    
    /* Need to reset the item width when inserting a new column */
    infoPtr->nItemWidth += dx;

    LISTVIEW_UpdateScroll(infoPtr);

    /* scroll to cover the deleted column, and invalidate for redraw */
    rcOld = infoPtr->rcList;
    rcOld.left = rcCol.left;
    ScrollWindowEx(infoPtr->hwndSelf, dx, 0, &rcOld, &rcOld, 0, 0, SW_ERASE | SW_INVALIDATE);
    
    /* we can restore focus now */
    if (infoPtr->bFocus) LISTVIEW_ShowFocusRect(infoPtr, TRUE);
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

    if (nColumn < 0 || DPA_GetPtrCount(infoPtr->hdpaColumns) == 0
           || nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return FALSE;

    /* While the MSDN specifically says that column zero should not be deleted,
       it does in fact work on WinNT, and at least one app depends on it. On
       WinNT, deleting column zero deletes the last column of items but the
       first header. Since no app will ever depend on that bizarre behavior, 
       we just delete the last column including the header.
     */
    if (nColumn == 0)
        nColumn = DPA_GetPtrCount(infoPtr->hdpaColumns) - 1;

    LISTVIEW_GetHeaderRect(infoPtr, nColumn, &rcCol);
    
    if (!Header_DeleteItem(infoPtr->hwndHeader, nColumn))
	return FALSE;

    Free(DPA_GetPtr(infoPtr->hdpaColumns, nColumn));
    DPA_DeletePtr(infoPtr->hdpaColumns, nColumn);
  
    if (!(infoPtr->dwStyle & LVS_OWNERDATA))
    {
	SUBITEM_INFO *lpSubItem, *lpDelItem;
	HDPA hdpaSubItems;
	INT nItem, nSubItem, i;
	
	if (nColumn == 0)
	    return LISTVIEW_DeleteAllItems(infoPtr);
	    
	for (nItem = 0; nItem < infoPtr->nItemCount; nItem++)
	{
	    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
	    nSubItem = 0;
	    lpDelItem = 0;
	    for (i = 1; i < DPA_GetPtrCount(hdpaSubItems); i++)
	    {
		lpSubItem = (SUBITEM_INFO *)DPA_GetPtr(hdpaSubItems, i);
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

	    /* if we found our subitem, zapp it */	
	    if (nSubItem > 0)
	    {
		/* free string */
		if (is_textW(lpDelItem->hdr.pszText))
		    Free(lpDelItem->hdr.pszText);

		/* free item */
		Free(lpDelItem);

		/* free dpa memory */
		DPA_DeletePtr(hdpaSubItems, nSubItem);
    	    }
	}
    }

    /* update the other column info */
    LISTVIEW_ScrollColumns(infoPtr, nColumn, -(rcCol.right - rcCol.left));

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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
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
    if (uView == LVS_REPORT)
	nPerCol = infoPtr->nItemCount + 1;
    else if (uView == LVS_LIST)
	nPerCol = LISTVIEW_GetCountPerColumn(infoPtr);
    else /* LVS_ICON, or LVS_SMALLICON */
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
    TRACE("rcScroll=%s, dx=%d\n", debugrect(&rcScroll), dir * infoPtr->nItemHeight);
    if (IntersectRect(&rcScroll, &rcScroll, &infoPtr->rcList))
    {
	TRACE("Scrolling rcScroll=%s, rcList=%s\n", debugrect(&rcScroll), debugrect(&infoPtr->rcList));
	ScrollWindowEx(infoPtr->hwndSelf, 0, dir * infoPtr->nItemHeight, 
		       &rcScroll, &rcScroll, 0, 0, SW_ERASE | SW_INVALIDATE);
    }

    /* report has only that column, so we're done */
    if (uView == LVS_REPORT) return;

    /* now for LISTs, we have to deal with the columns to the right */
    rcScroll.left = (nItemCol + 1) * infoPtr->nItemWidth;
    rcScroll.top = 0;
    rcScroll.right = (infoPtr->nItemCount / nPerCol + 1) * infoPtr->nItemWidth;
    rcScroll.bottom = nPerCol * infoPtr->nItemHeight;
    OffsetRect(&rcScroll, Origin.x, Origin.y);
    if (IntersectRect(&rcScroll, &rcScroll, &infoPtr->rcList))
	ScrollWindowEx(infoPtr->hwndSelf, 0, dir * infoPtr->nItemHeight,
		       &rcScroll, &rcScroll, 0, 0, SW_ERASE | SW_INVALIDATE);
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    LVITEMW item;

    TRACE("(nItem=%d)\n", nItem);

    if (nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;
    
    /* remove selection, and focus */
    item.state = 0;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    LISTVIEW_SetItemState(infoPtr, nItem, &item);
	    
    /* send LVN_DELETEITEM notification. */
    notify_deleteitem(infoPtr, nItem);

    /* we need to do this here, because we'll be deleting stuff */  
    if (uView == LVS_SMALLICON || uView == LVS_ICON)
	LISTVIEW_InvalidateItem(infoPtr, nItem);
    
    if (!(infoPtr->dwStyle & LVS_OWNERDATA))
    {
        HDPA hdpaSubItems;
	ITEMHDR *hdrItem;
	INT i;

	hdpaSubItems = (HDPA)DPA_DeletePtr(infoPtr->hdpaItems, nItem);	
	for (i = 0; i < DPA_GetPtrCount(hdpaSubItems); i++)
    	{
            hdrItem = (ITEMHDR *)DPA_GetPtr(hdpaSubItems, i);
	    if (is_textW(hdrItem->pszText)) Free(hdrItem->pszText);
            Free(hdrItem);
        }
        DPA_Destroy(hdpaSubItems);
    }

    if (uView == LVS_SMALLICON || uView == LVS_ICON)
    {
	DPA_DeletePtr(infoPtr->hdpaPosX, nItem);
	DPA_DeletePtr(infoPtr->hdpaPosY, nItem);
    }

    infoPtr->nItemCount--;
    LISTVIEW_ShiftIndices(infoPtr, nItem, -1);

    /* now is the invalidation fun */
    LISTVIEW_ScrollOnInsert(infoPtr, nItem, -1);
    return TRUE;
}


/***
 * DESCRIPTION:
 * Callback implementation for editlabel control
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] pszText : modified text
 * [I] isW : TRUE if psxText is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_EndEditLabelT(LISTVIEW_INFO *infoPtr, LPWSTR pszText, BOOL isW)
{
    NMLVDISPINFOW dispInfo;

    TRACE("(pszText=%s, isW=%d)\n", debugtext_t(pszText, isW), isW);

    ZeroMemory(&dispInfo, sizeof(dispInfo));
    dispInfo.item.mask = LVIF_PARAM | LVIF_STATE;
    dispInfo.item.iItem = infoPtr->nEditLabelItem;
    dispInfo.item.iSubItem = 0;
    dispInfo.item.stateMask = ~0;
    if (!LISTVIEW_GetItemW(infoPtr, &dispInfo.item)) return FALSE;
    /* add the text from the edit in */
    dispInfo.item.mask |= LVIF_TEXT;
    dispInfo.item.pszText = pszText;
    dispInfo.item.cchTextMax = textlenT(pszText, isW);

    /* Do we need to update the Item Text */
    if (!notify_dispinfoT(infoPtr, LVN_ENDLABELEDITW, &dispInfo, isW)) return FALSE;
    if (!pszText) return TRUE;

    ZeroMemory(&dispInfo, sizeof(dispInfo));
    dispInfo.item.mask = LVIF_TEXT;
    dispInfo.item.iItem = infoPtr->nEditLabelItem;
    dispInfo.item.iSubItem = 0;
    dispInfo.item.pszText = pszText;
    dispInfo.item.cchTextMax = textlenT(pszText, isW);
    return LISTVIEW_SetItemT(infoPtr, &dispInfo.item, isW);
}

/***
 * DESCRIPTION:
 * Begin in place editing of specified list view item
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] isW : TRUE if it's a Unicode req, FALSE if ASCII
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static HWND LISTVIEW_EditLabelT(LISTVIEW_INFO *infoPtr, INT nItem, BOOL isW)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { 0 };
    NMLVDISPINFOW dispInfo;
    RECT rect;

    TRACE("(nItem=%d, isW=%d)\n", nItem, isW);

    if (~infoPtr->dwStyle & LVS_EDITLABELS) return 0;
    if (nItem < 0 || nItem >= infoPtr->nItemCount) return 0;

    infoPtr->nEditLabelItem = nItem;

    /* Is the EditBox still there, if so remove it */
    if(infoPtr->hwndEdit != 0)
    {
        SetFocus(infoPtr->hwndSelf);
        infoPtr->hwndEdit = 0;
    }

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
    dispInfo.item.pszText = szDispText;
    dispInfo.item.cchTextMax = DISP_TEXT_SIZE;
    if (!LISTVIEW_GetItemT(infoPtr, &dispInfo.item, isW)) return 0;

    infoPtr->hwndEdit = CreateEditLabelT(infoPtr, dispInfo.item.pszText, WS_VISIBLE,
		    rect.left-2, rect.top-1, 0, rect.bottom - rect.top+2, isW);
    if (!infoPtr->hwndEdit) return 0;
    
    if (notify_dispinfoT(infoPtr, LVN_BEGINLABELEDITW, &dispInfo, isW))
    {
	SendMessageW(infoPtr->hwndEdit, WM_CLOSE, 0, 0);
	infoPtr->hwndEdit = 0;
	return 0;
    }

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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
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
        /* scroll left/right, but in LVS_REPORT mode */
        if (uView == LVS_LIST)
            nScrollPosWidth = infoPtr->nItemWidth;
        else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
            nScrollPosWidth = 1;

	if (rcItem.left < infoPtr->rcList.left)
	{
	    nHorzAdjust = -1;
	    if (uView != LVS_REPORT) nHorzDiff = rcItem.left - infoPtr->rcList.left;
	}
	else
	{
	    nHorzAdjust = 1;
	    if (uView != LVS_REPORT) nHorzDiff = rcItem.right - infoPtr->rcList.right;
	}
    }

    if (rcItem.top < infoPtr->rcList.top || rcItem.bottom > infoPtr->rcList.bottom)
    {
	/* scroll up/down, but not in LVS_LIST mode */
        if (uView == LVS_REPORT)
            nScrollPosHeight = infoPtr->nItemHeight;
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
            nScrollPosHeight = 1;

	if (rcItem.top < infoPtr->rcList.top)
	{
	    nVertAdjust = -1;
	    if (uView != LVS_LIST) nVertDiff = rcItem.top - infoPtr->rcList.top;
	}
	else
	{
	    nVertAdjust = 1;
	    if (uView != LVS_LIST) nVertDiff = rcItem.bottom - infoPtr->rcList.bottom;
	}
    }

    if (!nScrollPosWidth && !nScrollPosHeight) return TRUE;

    if (nScrollPosWidth)
    {
	INT diff = nHorzDiff / nScrollPosWidth;
	if (nHorzDiff % nScrollPosWidth) diff += nHorzAdjust;
	LISTVIEW_HScroll(infoPtr, SB_INTERNAL, diff, 0);
    }

    if (nScrollPosHeight)
    {
	INT diff = nVertDiff / nScrollPosHeight;
	if (nVertDiff % nScrollPosHeight) diff += nVertAdjust;
	LISTVIEW_VScroll(infoPtr, SB_INTERNAL, diff, 0);
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
static INT LISTVIEW_FindItemW(LISTVIEW_INFO *infoPtr, INT nStart,
                              const LVFINDINFOW *lpFindInfo)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    BOOL bWrap = FALSE, bNearest = FALSE;
    INT nItem = nStart + 1, nLast = infoPtr->nItemCount, nNearestItem = -1;
    ULONG xdist, ydist, dist, mindist = 0x7fffffff;
    POINT Position, Destination;
    LVITEMW lvItem;

    if (!lpFindInfo || nItem < 0) return -1;
    
    lvItem.mask = 0;
    if (lpFindInfo->flags & (LVFI_STRING | LVFI_PARTIAL))
    {
        lvItem.mask |= LVIF_TEXT;
        lvItem.pszText = szDispText;
        lvItem.cchTextMax = DISP_TEXT_SIZE;
    }

    if (lpFindInfo->flags & LVFI_WRAP)
        bWrap = TRUE;

    if ((lpFindInfo->flags & LVFI_NEARESTXY) && 
	(uView == LVS_ICON || uView ==LVS_SMALLICON))
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

    /* if LVFI_PARAM is specified, all other flags are ignored */
    if (lpFindInfo->flags & LVFI_PARAM)
    {
        lvItem.mask |= LVIF_PARAM;
	bNearest = FALSE;
	lvItem.mask &= ~LVIF_TEXT;
    }

again:
    for (; nItem < nLast; nItem++)
    {
        lvItem.iItem = nItem;
        lvItem.iSubItem = 0;
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
            if (lpFindInfo->flags & LVFI_PARTIAL)
            {
            	if (strstrW(lvItem.pszText, lpFindInfo->psz) == NULL) continue;
            }
            else
            {
            	if (lstrcmpW(lvItem.pszText, lpFindInfo->psz) != 0) continue;
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
static INT LISTVIEW_FindItemA(LISTVIEW_INFO *infoPtr, INT nStart,
                              const LVFINDINFOA *lpFindInfo)
{
    BOOL hasText = lpFindInfo->flags & (LVFI_STRING | LVFI_PARTIAL);
    LVFINDINFOW fiw;
    INT res;

    memcpy(&fiw, lpFindInfo, sizeof(fiw));
    if (hasText) fiw.psz = textdupTtoW((LPCWSTR)lpFindInfo->psz, FALSE);
    res = LISTVIEW_FindItemW(infoPtr, nStart, &fiw);
    if (hasText) textfreeT((LPWSTR)fiw.psz, FALSE);
    return res;
}

/***
 * DESCRIPTION:
 * Retrieves the background image of the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lpBkImage : background image attributes
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
/* static BOOL LISTVIEW_GetBkImage(LISTVIEW_INFO *infoPtr, LPLVBKIMAGE lpBkImage)   */
/* {   */
/*   FIXME (listview, "empty stub!\n"); */
/*   return FALSE;   */
/* }   */

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
static BOOL LISTVIEW_GetColumnT(LISTVIEW_INFO *infoPtr, INT nColumn, LPLVCOLUMNW lpColumn, BOOL isW)
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

    if (!SendMessageW(infoPtr->hwndHeader, isW ? HDM_GETITEMW : HDM_GETITEMA, nColumn, (LPARAM)&hdi)) return FALSE;

    if (lpColumn->mask & LVCF_FMT)
	lpColumn->fmt = lpColumnInfo->fmt;

    if (lpColumn->mask & LVCF_WIDTH)
        lpColumn->cx = lpColumnInfo->rcHeader.right - lpColumnInfo->rcHeader.left;

    if (lpColumn->mask & LVCF_IMAGE)
	lpColumn->iImage = hdi.iImage;

    if (lpColumn->mask & LVCF_ORDER)
	lpColumn->iOrder = hdi.iOrder;

    return TRUE;
}


static BOOL LISTVIEW_GetColumnOrderArray(LISTVIEW_INFO *infoPtr, INT iCount, LPINT lpiArray)
{
    INT i;

    if (!lpiArray)
	return FALSE;

    /* FIXME: little hack */
    for (i = 0; i < iCount; i++)
	lpiArray[i] = i;

    return TRUE;
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
static INT LISTVIEW_GetColumnWidth(LISTVIEW_INFO *infoPtr, INT nColumn)
{
    INT nColumnWidth = 0;
    RECT rcHeader;

    TRACE("nColumn=%d\n", nColumn);

    /* we have a 'column' in LIST and REPORT mode only */
    switch(infoPtr->dwStyle & LVS_TYPEMASK)
    {
    case LVS_LIST:
	nColumnWidth = infoPtr->nItemWidth;
	break;
    case LVS_REPORT:
	if (nColumn < 0 || nColumn >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return 0;
	LISTVIEW_GetHeaderRect(infoPtr, nColumn, &rcHeader);
	nColumnWidth = rcHeader.right - rcHeader.left;
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
static INT LISTVIEW_GetCountPerPage(LISTVIEW_INFO *infoPtr)
{
    switch (infoPtr->dwStyle & LVS_TYPEMASK)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
	return infoPtr->nItemCount;
    case LVS_REPORT:
	return LISTVIEW_GetCountPerColumn(infoPtr);
    case LVS_LIST:
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
static HIMAGELIST LISTVIEW_GetImageList(LISTVIEW_INFO *infoPtr, INT nImageList)
{
    switch (nImageList)
    {
    case LVSIL_NORMAL: return infoPtr->himlNormal;
    case LVSIL_SMALL: return infoPtr->himlSmall;
    case LVSIL_STATE: return infoPtr->himlState;
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
 *           if FALSE, the lpLVItem is a LPLVITEMA.
 *
 * NOTE:
 *   This is the internal 'GetItem' interface -- it tries to
 *   be smart, and avoids text copies, if possible, by modifing
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
static BOOL LISTVIEW_GetItemT(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    ITEMHDR callbackHdr = { LPSTR_TEXTCALLBACKW, I_IMAGECALLBACK };
    NMLVDISPINFOW dispInfo;
    ITEM_INFO *lpItem;
    ITEMHDR* pItemHdr;
    HDPA hdpaSubItems;
    INT isubitem;

    TRACE("(lpLVItem=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (!lpLVItem || lpLVItem->iItem < 0 || lpLVItem->iItem >= infoPtr->nItemCount)
	return FALSE;

    if (lpLVItem->mask == 0) return TRUE;

    /* make a local copy */
    isubitem = lpLVItem->iSubItem;

    /* a quick optimization if all we're asked is the focus state
     * these queries are worth optimising since they are common,
     * and can be answered in constant time, without the heavy accesses */
    if ( (lpLVItem->mask == LVIF_STATE) && (lpLVItem->stateMask == LVIS_FOCUSED) &&
	 !(infoPtr->uCallbackMask & LVIS_FOCUSED) )
    {
	lpLVItem->state = 0;
	if (infoPtr->nFocusedItem == lpLVItem->iItem)
	    lpLVItem->state |= LVIS_FOCUSED;
	return TRUE;
    }

    ZeroMemory(&dispInfo, sizeof(dispInfo));

    /* if the app stores all the data, handle it separately */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	dispInfo.item.state = 0;

	/* apprently, we should not callback for lParam in LVS_OWNERDATA */
	if ((lpLVItem->mask & ~(LVIF_STATE | LVIF_PARAM)) || infoPtr->uCallbackMask)
	{
	    /* NOTE: copy only fields which we _know_ are initialized, some apps
	     *       depend on the uninitialized fields being 0 */
	    dispInfo.item.mask = lpLVItem->mask & ~LVIF_PARAM;
	    dispInfo.item.iItem = lpLVItem->iItem;
	    dispInfo.item.iSubItem = isubitem;
	    if (lpLVItem->mask & LVIF_TEXT)
	    {
		dispInfo.item.pszText = lpLVItem->pszText;
		dispInfo.item.cchTextMax = lpLVItem->cchTextMax;		
	    }
	    if (lpLVItem->mask & LVIF_STATE)
	        dispInfo.item.stateMask = lpLVItem->stateMask & infoPtr->uCallbackMask;
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
	    TRACE("   getdispinfo(1):lpLVItem=%s\n", debuglvitem_t(lpLVItem, isW));
	}
	
	/* make sure lParam is zeroed out */
	if (lpLVItem->mask & LVIF_PARAM) lpLVItem->lParam = 0;
	
	/* we store only a little state, so if we're not asked, we're done */
	if (!(lpLVItem->mask & LVIF_STATE) || isubitem) return TRUE;

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
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    lpItem = (ITEM_INFO *)DPA_GetPtr(hdpaSubItems, 0);
    assert (lpItem);

    if (isubitem)
    {
	SUBITEM_INFO *lpSubItem = LISTVIEW_GetSubItemPtr(hdpaSubItems, isubitem);
        pItemHdr = lpSubItem ? &lpSubItem->hdr : &callbackHdr;
        if (!lpSubItem)
        {
            WARN(" iSubItem invalid (%08x), ignored.\n", isubitem);
            isubitem = 0;
        }
    }
    else
	pItemHdr = &lpItem->hdr;

    /* Do we need to query the state from the app? */
    if ((lpLVItem->mask & LVIF_STATE) && infoPtr->uCallbackMask && isubitem == 0)
    {
	dispInfo.item.mask |= LVIF_STATE;
	dispInfo.item.stateMask = infoPtr->uCallbackMask;
    }
  
    /* Do we need to enquire about the image? */
    if ((lpLVItem->mask & LVIF_IMAGE) && pItemHdr->iImage == I_IMAGECALLBACK &&
        (isubitem == 0 || (infoPtr->dwLvExStyle & LVS_EX_SUBITEMIMAGES)))
    {
	dispInfo.item.mask |= LVIF_IMAGE;
        dispInfo.item.iImage = I_IMAGECALLBACK;
    }

    /* Apps depend on calling back for text if it is NULL or LPSTR_TEXTCALLBACKW */
    if ((lpLVItem->mask & LVIF_TEXT) && !is_textW(pItemHdr->pszText))
    {
	dispInfo.item.mask |= LVIF_TEXT;
	dispInfo.item.pszText = lpLVItem->pszText;
	dispInfo.item.cchTextMax = lpLVItem->cchTextMax;
	if (dispInfo.item.pszText && dispInfo.item.cchTextMax > 0)
	    *dispInfo.item.pszText = '\0';
    }

    /* If we don't have all the requested info, query the application */
    if (dispInfo.item.mask != 0)
    {
	dispInfo.item.iItem = lpLVItem->iItem;
	dispInfo.item.iSubItem = lpLVItem->iSubItem; /* yes: the original subitem */
	dispInfo.item.lParam = lpItem->lParam;
	notify_dispinfoT(infoPtr, LVN_GETDISPINFOW, &dispInfo, isW);
	TRACE("   getdispinfo(2):item=%s\n", debuglvitem_t(&dispInfo.item, isW));
    }

    /* we should not store values for subitems */
    if (isubitem) dispInfo.item.mask &= ~LVIF_DI_SETITEM;

    /* Now, handle the iImage field */
    if (dispInfo.item.mask & LVIF_IMAGE)
    {
	lpLVItem->iImage = dispInfo.item.iImage;
	if ((dispInfo.item.mask & LVIF_DI_SETITEM) && pItemHdr->iImage == I_IMAGECALLBACK)
	    pItemHdr->iImage = dispInfo.item.iImage;
    }
    else if (lpLVItem->mask & LVIF_IMAGE)
    {
        if(isubitem == 0 || (infoPtr->dwLvExStyle & LVS_EX_SUBITEMIMAGES))
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
	if (isW) lpLVItem->pszText = pItemHdr->pszText;
	else textcpynT(lpLVItem->pszText, isW, pItemHdr->pszText, TRUE, lpLVItem->cchTextMax);
    }

    /* if this is a subitem, we're done */
    if (isubitem) return TRUE;
  
    /* Next is the lParam field */
    if (dispInfo.item.mask & LVIF_PARAM)
    {
	lpLVItem->lParam = dispInfo.item.lParam;
	if ((dispInfo.item.mask & LVIF_DI_SETITEM))
	    lpItem->lParam = dispInfo.item.lParam;
    }
    else if (lpLVItem->mask & LVIF_PARAM)
	lpLVItem->lParam = lpItem->lParam;

    /* ... the state field (this one is different due to uCallbackmask) */
    if (lpLVItem->mask & LVIF_STATE) 
    {
	lpLVItem->state = lpItem->state;
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
    if (lpLVItem->mask & LVIF_INDENT)
	lpLVItem->iIndent = lpItem->iIndent;

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
 *           if FALSE, the lpLVItem is a LPLVITEMA.
 *
 * NOTE:
 *   This is the external 'GetItem' interface -- it properly copies
 *   the text in the provided buffer.
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetItemExtT(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    LPWSTR pszText;
    BOOL bResult;

    if (!lpLVItem || lpLVItem->iItem < 0 || lpLVItem->iItem >= infoPtr->nItemCount)
	return FALSE;

    pszText = lpLVItem->pszText;
    bResult = LISTVIEW_GetItemT(infoPtr, lpLVItem, isW);
    if (bResult && lpLVItem->pszText != pszText)
	textcpynT(pszText, isW, lpLVItem->pszText, isW, lpLVItem->cchTextMax);
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
static BOOL LISTVIEW_GetItemPosition(LISTVIEW_INFO *infoPtr, INT nItem, LPPOINT lpptPosition)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    POINT Origin;

    TRACE("(nItem=%d, lpptPosition=%p)\n", nItem, lpptPosition);

    if (!lpptPosition || nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, lpptPosition);

    if (uView == LVS_ICON)
    {
        lpptPosition->x += (infoPtr->nItemWidth - infoPtr->iconSize.cx) / 2;
        lpptPosition->y += ICON_TOP_PADDING;
    }
    lpptPosition->x += Origin.x;
    lpptPosition->y += Origin.y;
    
    TRACE ("  lpptPosition=%s\n", debugpoint(lpptPosition));
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
static BOOL LISTVIEW_GetItemRect(LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lprc)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    BOOL doLabel = TRUE, oversizedBox = FALSE;
    POINT Position, Origin;
    LVITEMW lvItem;
    RECT label_rect;

    TRACE("(hwnd=%p, nItem=%d, lprc=%p)\n", infoPtr->hwndSelf, nItem, lprc);

    if (!lprc || nItem < 0 || nItem >= infoPtr->nItemCount) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
    LISTVIEW_GetItemOrigin(infoPtr, nItem, &Position);

    /* Be smart and try to figure out the minimum we have to do */
    if (lprc->left == LVIR_ICON) doLabel = FALSE;
    if (uView == LVS_REPORT && lprc->left == LVIR_BOUNDS) doLabel = FALSE;
    if (uView == LVS_ICON && lprc->left != LVIR_ICON &&
	infoPtr->bFocus && LISTVIEW_GetItemState(infoPtr, nItem, LVIS_FOCUSED))
	oversizedBox = TRUE;

    /* get what we need from the item before hand, so we make
     * only one request. This can speed up things, if data
     * is stored on the app side */
    lvItem.mask = 0;
    if (uView == LVS_REPORT) lvItem.mask |= LVIF_INDENT;
    if (doLabel) lvItem.mask |= LVIF_TEXT;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.pszText = szDispText;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    if (lvItem.mask && !LISTVIEW_GetItemW(infoPtr, &lvItem)) return FALSE;
    /* we got the state already up, simulate it here, to avoid a reget */
    if (uView == LVS_ICON && (lprc->left != LVIR_ICON))
    {
	lvItem.mask |= LVIF_STATE;
	lvItem.stateMask = LVIS_FOCUSED;
	lvItem.state = (oversizedBox ? LVIS_FOCUSED : 0);
    }

    if (uView == LVS_REPORT && (infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) && lprc->left == LVIR_SELECTBOUNDS)
	lprc->left = LVIR_BOUNDS;
    switch(lprc->left)
    {
    case LVIR_ICON:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, NULL, NULL, lprc, NULL);
        break;

    case LVIR_LABEL:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, NULL, NULL, NULL, lprc);
        break;

    case LVIR_BOUNDS:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, lprc, NULL, NULL, NULL);
        break;

    case LVIR_SELECTBOUNDS:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, NULL, NULL, lprc, &label_rect);
	UnionRect(lprc, lprc, &label_rect);
        break;

    default:
	WARN("Unknown value: %ld\n", lprc->left);
	return FALSE;
    }

    OffsetRect(lprc, Position.x + Origin.x, Position.y + Origin.y);

    TRACE(" rect=%s\n", debugrect(lprc));

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
 *       Fortunately, LISTVIEW_GetItemMetrics does the right thing.
 * 
 * RETURN:
 *     TRUE: success
 *     FALSE: failure
 */
static BOOL LISTVIEW_GetSubItemRect(LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lprc)
{
    POINT Position;
    LVITEMW lvItem;
    
    if (!lprc) return FALSE;

    TRACE("(nItem=%d, nSubItem=%ld)\n", nItem, lprc->top);
    /* On WinNT, a subitem of '0' calls LISTVIEW_GetItemRect */
    if (lprc->top == 0)
        return LISTVIEW_GetItemRect(infoPtr, nItem, lprc);

    if ((infoPtr->dwStyle & LVS_TYPEMASK) != LVS_REPORT) return FALSE;

    if (!LISTVIEW_GetItemPosition(infoPtr, nItem, &Position)) return FALSE;

    lvItem.mask = 0;
    lvItem.iItem = nItem;
    lvItem.iSubItem = lprc->top;
    
    if (lvItem.mask && !LISTVIEW_GetItemW(infoPtr, &lvItem)) return FALSE;
    switch(lprc->left)
    {
    case LVIR_ICON:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, NULL, NULL, lprc, NULL);
        break;

    case LVIR_LABEL:
    case LVIR_BOUNDS:
	LISTVIEW_GetItemMetrics(infoPtr, &lvItem, lprc, NULL, NULL, NULL);
        break;

    default:
	ERR("Unknown bounds=%ld\n", lprc->left);
	return FALSE;
    }

    OffsetRect(lprc, Position.x, Position.y);
    return TRUE;
}


/***
 * DESCRIPTION:
 * Retrieves the width of a label.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *   SUCCESS : string width (in pixels)
 *   FAILURE : zero
 */
static INT LISTVIEW_GetLabelWidth(LISTVIEW_INFO *infoPtr, INT nItem)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    LVITEMW lvItem;

    TRACE("(nItem=%d)\n", nItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.pszText = szDispText;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) return 0;
  
    return LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE);
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
static LONG LISTVIEW_GetItemSpacing(LISTVIEW_INFO *infoPtr, BOOL bSmall)
{
  LONG lResult;

  if (!bSmall)
  {
    lResult = MAKELONG(infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy);
  }
  else
  {
    if ((infoPtr->dwStyle & LVS_TYPEMASK) == LVS_ICON)
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
static UINT LISTVIEW_GetItemState(LISTVIEW_INFO *infoPtr, INT nItem, UINT uMask)
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
static INT LISTVIEW_GetItemTextT(LISTVIEW_INFO *infoPtr, INT nItem, LPLVITEMW lpLVItem, BOOL isW)
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
static INT LISTVIEW_GetNextItem(LISTVIEW_INFO *infoPtr, INT nItem, UINT uFlags)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
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
	if (!(LISTVIEW_GetItemState(infoPtr, infoPtr->nFocusedItem, uMask) & uMask) == uMask) return -1;
	return (infoPtr->nFocusedItem == nItem) ? -1 : infoPtr->nFocusedItem;
    }
    
    if (uFlags & LVNI_ABOVE)
    {
      if ((uView == LVS_LIST) || (uView == LVS_REPORT))
      {
        while (nItem >= 0)
        {
          nItem--;
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
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
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_BELOW)
    {
      if ((uView == LVS_LIST) || (uView == LVS_REPORT))
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
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
        {
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TOLEFT)
    {
      if (uView == LVS_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
        while (nItem - nCountPerColumn >= 0)
        {
          nItem -= nCountPerColumn;
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
      {
        /* Special case for autoarrange - move 'ti the beginning of a row */
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
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TORIGHT)
    {
      if (uView == LVS_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
        while (nItem + nCountPerColumn < infoPtr->nItemCount)
        {
          nItem += nCountPerColumn;
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
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
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
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
static void LISTVIEW_GetOrigin(LISTVIEW_INFO *infoPtr, LPPOINT lpptOrigin)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT nHorzPos = 0, nVertPos = 0;
    SCROLLINFO scrollInfo;

    scrollInfo.cbSize = sizeof(SCROLLINFO);    
    scrollInfo.fMask = SIF_POS;
    
    if ((infoPtr->dwStyle & WS_HSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
	nHorzPos = scrollInfo.nPos;
    if ((infoPtr->dwStyle & WS_VSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	nVertPos = scrollInfo.nPos;

    TRACE("nHorzPos=%d, nVertPos=%d\n", nHorzPos, nVertPos);

    lpptOrigin->x = infoPtr->rcList.left;
    lpptOrigin->y = infoPtr->rcList.top;
    if (uView == LVS_LIST)
	nHorzPos *= infoPtr->nItemWidth;
    else if (uView == LVS_REPORT)
	nVertPos *= infoPtr->nItemHeight;
    
    lpptOrigin->x -= nHorzPos;
    lpptOrigin->y -= nVertPos;

    TRACE(" origin=%s\n", debugpoint(lpptOrigin));
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
static INT LISTVIEW_GetStringWidthT(LISTVIEW_INFO *infoPtr, LPCWSTR lpszText, BOOL isW)
{
    SIZE stringSize;
    
    stringSize.cx = 0;    
    if (is_textT(lpszText, isW))
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
static INT LISTVIEW_HitTest(LISTVIEW_INFO *infoPtr, LPLVHITTESTINFO lpht, BOOL subitem, BOOL select)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    RECT rcBox, rcBounds, rcState, rcIcon, rcLabel, rcSearch;
    POINT Origin, Position, opt;
    LVITEMW lvItem;
    ITERATOR i;
    INT iItem;
    
    TRACE("(pt=%s, subitem=%d, select=%d)\n", debugpoint(&lpht->pt), subitem, select);
    
    lpht->flags = 0;
    lpht->iItem = -1;
    if (subitem) lpht->iSubItem = 0;

    if (infoPtr->rcList.left > lpht->pt.x)
	lpht->flags |= LVHT_TOLEFT;
    else if (infoPtr->rcList.right < lpht->pt.x)
	lpht->flags |= LVHT_TORIGHT;
    
    if (infoPtr->rcList.top > lpht->pt.y)
	lpht->flags |= LVHT_ABOVE;
    else if (infoPtr->rcList.bottom < lpht->pt.y)
	lpht->flags |= LVHT_BELOW;

    TRACE("lpht->flags=0x%x\n", lpht->flags);
    if (lpht->flags) return -1;

    lpht->flags |= LVHT_NOWHERE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);
   
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
    if (uView == LVS_REPORT) lvItem.mask |= LVIF_INDENT;
    lvItem.stateMask = LVIS_STATEIMAGEMASK;
    if (uView == LVS_ICON) lvItem.stateMask |= LVIS_FOCUSED;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 0;
    lvItem.pszText = szDispText;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) return -1;
    if (!infoPtr->bFocus) lvItem.state &= ~LVIS_FOCUSED; 
    
    LISTVIEW_GetItemMetrics(infoPtr, &lvItem, &rcBox, &rcState, &rcIcon, &rcLabel);
    LISTVIEW_GetItemOrigin(infoPtr, iItem, &Position);
    opt.x = lpht->pt.x - Position.x - Origin.x;
    opt.y = lpht->pt.y - Position.y - Origin.y;
    
    if (uView == LVS_REPORT)
	rcBounds = rcBox;
    else
	UnionRect(&rcBounds, &rcIcon, &rcLabel);
    TRACE("rcBounds=%s\n", debugrect(&rcBounds));
    if (!PtInRect(&rcBounds, opt)) return -1;

    if (PtInRect(&rcIcon, opt))
	lpht->flags |= LVHT_ONITEMICON;
    else if (PtInRect(&rcLabel, opt))
	lpht->flags |= LVHT_ONITEMLABEL;
    else if (infoPtr->himlState && ((lvItem.state & LVIS_STATEIMAGEMASK) >> 12) && PtInRect(&rcState, opt))
	lpht->flags |= LVHT_ONITEMSTATEICON;
    if (lpht->flags & LVHT_ONITEM)
	lpht->flags &= ~LVHT_NOWHERE;
   
    TRACE("lpht->flags=0x%x\n", lpht->flags); 
    if (uView == LVS_REPORT && subitem)
    {
  	INT j;

        rcBounds.right = rcBounds.left;
        for (j = 0; j < DPA_GetPtrCount(infoPtr->hdpaColumns); j++)
        {
	    rcBounds.left = rcBounds.right;
	    rcBounds.right += LISTVIEW_GetColumnWidth(infoPtr, j);
	    if (PtInRect(&rcBounds, opt))
	    {
		lpht->iSubItem = j;
		break;
	    }
	}
    }

    if (select && !(uView == LVS_REPORT &&
                    ((infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT) ||
                     (infoPtr->dwStyle & LVS_OWNERDRAWFIXED))))
    {
        if (uView == LVS_REPORT)
        {
            UnionRect(&rcBounds, &rcIcon, &rcLabel);
            UnionRect(&rcBounds, &rcBounds, &rcState);
        }
        if (!PtInRect(&rcBounds, opt)) iItem = -1;
    }
    return lpht->iItem = iItem;
}


/* LISTVIEW_InsertCompare:  callback routine for comparing pszText members of the LV_ITEMS
   in a LISTVIEW on insert.  Passed to DPA_Sort in LISTVIEW_InsertItem.
   This function should only be used for inserting items into a sorted list (LVM_INSERTITEM)
   and not during the processing of a LVM_SORTITEMS message. Applications should provide
   their own sort proc. when sending LVM_SORTITEMS.
*/
/* Platform SDK:
    (remarks on LVITEM: LVM_INSERTITEM will insert the new item in the proper sort postion...
        if:
          LVS_SORTXXX must be specified,
          LVS_OWNERDRAW is not set,
          <item>.pszText is not LPSTR_TEXTCALLBACK.

    (LVS_SORT* flags): "For the LVS_SORTASCENDING... styles, item indices
    are sorted based on item text..."
*/
static INT WINAPI LISTVIEW_InsertCompare(  LPVOID first, LPVOID second,  LPARAM lParam)
{
    ITEM_INFO* lv_first = (ITEM_INFO*) DPA_GetPtr( (HDPA)first, 0 );
    ITEM_INFO* lv_second = (ITEM_INFO*) DPA_GetPtr( (HDPA)second, 0 );
    INT cmpv = textcmpWT(lv_first->hdr.pszText, lv_second->hdr.pszText, TRUE); 

    /* if we're sorting descending, negate the return value */
    return (((LISTVIEW_INFO *)lParam)->dwStyle & LVS_SORTDESCENDING) ? -cmpv : cmpv;
}

/***
 * nESCRIPTION:
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT nItem;
    HDPA hdpaSubItems;
    NMLISTVIEW nmlv;
    ITEM_INFO *lpItem;
    BOOL is_sorted, has_changed;
    LVITEMW item;

    TRACE("(lpLVItem=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (infoPtr->dwStyle & LVS_OWNERDATA) return infoPtr->nItemCount++;

    /* make sure it's an item, and not a subitem; cannot insert a subitem */
    if (!lpLVItem || lpLVItem->iItem < 0 || lpLVItem->iSubItem) return -1;

    if (!is_assignable_item(lpLVItem, infoPtr->dwStyle)) return -1;

    if ( !(lpItem = (ITEM_INFO *)Alloc(sizeof(ITEM_INFO))) )
	return -1;
    
    /* insert item in listview control data structure */
    if ( !(hdpaSubItems = DPA_Create(8)) ) goto fail;
    if ( !DPA_SetPtr(hdpaSubItems, 0, lpItem) ) assert (FALSE);

    is_sorted = (infoPtr->dwStyle & (LVS_SORTASCENDING | LVS_SORTDESCENDING)) &&
	        !(infoPtr->dwStyle & LVS_OWNERDRAWFIXED) && (LPSTR_TEXTCALLBACKW != lpLVItem->pszText);

    nItem = is_sorted ? infoPtr->nItemCount : min(lpLVItem->iItem, infoPtr->nItemCount);
    TRACE(" inserting at %d, sorted=%d, count=%d, iItem=%d\n", nItem, is_sorted, infoPtr->nItemCount, lpLVItem->iItem);
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
    item.state &= ~LVIS_STATEIMAGEMASK;
    if (!set_main_item(infoPtr, &item, TRUE, isW, &has_changed)) goto undo;

    /* if we're sorted, sort the list, and update the index */
    if (is_sorted)
    {
	DPA_Sort( infoPtr->hdpaItems, LISTVIEW_InsertCompare, (LPARAM)infoPtr );
	nItem = DPA_GetPtrIndex( infoPtr->hdpaItems, hdpaSubItems );
	assert(nItem != -1);
    }

    /* make room for the position, if we are in the right mode */
    if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
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
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    nmlv.iItem = nItem;
    nmlv.lParam = lpItem->lParam;
    notify_listview(infoPtr, LVN_INSERTITEM, &nmlv);

    /* align items (set position of each item) */
    if ((uView == LVS_SMALLICON || uView == LVS_ICON))
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
static BOOL LISTVIEW_RedrawItems(LISTVIEW_INFO *infoPtr, INT nFirst, INT nLast)
{
    INT i;
 
    if (nLast < nFirst || min(nFirst, nLast) < 0 || 
	max(nFirst, nLast) >= infoPtr->nItemCount)
	return FALSE;
    
    for (i = nFirst; i <= nLast; i++)
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
 *  If the control is in report mode (LVS_REPORT) the control can
 *  be scrolled only in line increments. "dy" will be rounded to the
 *  nearest number of pixels that are a whole line. Ex: if line height
 *  is 16 and an 8 is passed, the list will be scrolled by 16. If a 7
 *  is passed the the scroll will be 0.  (per MSDN 7/2002)
 *
 *  For:  (per experimentaion with native control and CSpy ListView)
 *     LVS_ICON       dy=1 = 1 pixel  (vertical only)
 *                    dx ignored
 *     LVS_SMALLICON  dy=1 = 1 pixel  (vertical only)
 *                    dx ignored
 *     LVS_LIST       dx=1 = 1 column (horizontal only)
 *                           but will only scroll 1 column per message
 *                           no matter what the value.
 *                    dy must be 0 or FALSE returned.
 *     LVS_REPORT     dx=1 = 1 pixel
 *                    dy=  see above
 *
 */
static BOOL LISTVIEW_Scroll(LISTVIEW_INFO *infoPtr, INT dx, INT dy)
{
    switch(infoPtr->dwStyle & LVS_TYPEMASK) {
    case LVS_REPORT:
	dy += (dy < 0 ? -1 : 1) * infoPtr->nItemHeight/2;
        dy /= infoPtr->nItemHeight;
	break;
    case LVS_LIST:
    	if (dy != 0) return FALSE;
	break;
    default: /* icon */
	dx = 0;
	break;
    }	

    if (dx != 0) LISTVIEW_HScroll(infoPtr, SB_INTERNAL, dx, 0);
    if (dy != 0) LISTVIEW_VScroll(infoPtr, SB_INTERNAL, dy, 0);
  
    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the background color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] clrBk : background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetBkColor(LISTVIEW_INFO *infoPtr, COLORREF clrBk)
{
    TRACE("(clrBk=%lx)\n", clrBk);

    if(infoPtr->clrBk != clrBk) {
	if (infoPtr->clrBk != CLR_NONE) DeleteObject(infoPtr->hBkBrush);
	infoPtr->clrBk = clrBk;
	if (clrBk == CLR_NONE)
	    infoPtr->hBkBrush = (HBRUSH)GetClassLongW(infoPtr->hwndSelf, GCL_HBRBACKGROUND);
	else
	    infoPtr->hBkBrush = CreateSolidBrush(clrBk);
	LISTVIEW_InvalidateList(infoPtr);
    }

   return TRUE;
}

/* LISTVIEW_SetBkImage */

/*** Helper for {Insert,Set}ColumnT *only* */
static void column_fill_hditem(LISTVIEW_INFO *infoPtr, HDITEMW *lphdi, INT nColumn, const LVCOLUMNW *lpColumn, BOOL isW)
{
    if (lpColumn->mask & LVCF_FMT)
    {
	/* format member is valid */
	lphdi->mask |= HDI_FORMAT;

	/* set text alignment (leftmost column must be left-aligned) */
        if (nColumn == 0 || lpColumn->fmt & LVCFMT_LEFT)
            lphdi->fmt |= HDF_LEFT;
        else if (lpColumn->fmt & LVCFMT_RIGHT)
            lphdi->fmt |= HDF_RIGHT;
        else if (lpColumn->fmt & LVCFMT_CENTER)
            lphdi->fmt |= HDF_CENTER;

        if (lpColumn->fmt & LVCFMT_BITMAP_ON_RIGHT)
            lphdi->fmt |= HDF_BITMAP_ON_RIGHT;

        if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
        {
            lphdi->fmt |= HDF_IMAGE;
            lphdi->iImage = I_IMAGECALLBACK;
        }
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
            TRACE("start cxy=%d rcHeader=%s\n", lphdi->cxy, debugrect(&rcHeader));

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

    /* insert item in header control */
    nNewColumn = SendMessageW(infoPtr->hwndHeader, 
		              isW ? HDM_INSERTITEMW : HDM_INSERTITEMA,
                              (WPARAM)nColumn, (LPARAM)&hdi);
    if (nNewColumn == -1) return -1;
    if (nNewColumn != nColumn) ERR("nColumn=%d, nNewColumn=%d\n", nColumn, nNewColumn);
   
    /* create our own column info */ 
    if (!(lpColumnInfo = Alloc(sizeof(COLUMN_INFO)))) goto fail;
    if (DPA_InsertPtr(infoPtr->hdpaColumns, nNewColumn, lpColumnInfo) == -1) goto fail;

    if (lpColumn->mask & LVCF_FMT) lpColumnInfo->fmt = lpColumn->fmt;
    if (!Header_GetItemRect(infoPtr->hwndHeader, nNewColumn, &lpColumnInfo->rcHeader)) goto fail;

    /* now we have to actually adjust the data */
    if (!(infoPtr->dwStyle & LVS_OWNERDATA) && infoPtr->nItemCount > 0)
    {
	SUBITEM_INFO *lpSubItem;
	HDPA hdpaSubItems;
	INT nItem, i;
	
	for (nItem = 0; nItem < infoPtr->nItemCount; nItem++)
	{
	    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
	    for (i = 1; i < DPA_GetPtrCount(hdpaSubItems); i++)
	    {
		lpSubItem = (SUBITEM_INFO *)DPA_GetPtr(hdpaSubItems, i);
		if (lpSubItem->iSubItem >= nNewColumn)
		    lpSubItem->iSubItem++;
	    }
	}
    }

    /* make space for the new column */
    LISTVIEW_ScrollColumns(infoPtr, nNewColumn + 1, lpColumnInfo->rcHeader.right - lpColumnInfo->rcHeader.left);
    
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
 * [I] isW: if TRUE, the lpColumn is a LPLVCOLUMNW, else it is a LPLVCOLUMNA
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetColumnT(LISTVIEW_INFO *infoPtr, INT nColumn,
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
        if (Header_GetItemW(infoPtr->hwndHeader, nColumn, &hdiget))
	    hdi.fmt = hdiget.fmt & HDF_STRING;
    }
    column_fill_hditem(infoPtr, &hdi, nColumn, lpColumn, isW);

    /* set header item attributes */
    bResult = SendMessageW(infoPtr->hwndHeader, isW ? HDM_SETITEMW : HDM_SETITEMA, (WPARAM)nColumn, (LPARAM)&hdi);
    if (!bResult) return FALSE;

    if (lpColumn->mask & LVCF_FMT)
    {
	COLUMN_INFO *lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, nColumn);
	int oldFmt = lpColumnInfo->fmt;
	
	lpColumnInfo->fmt = lpColumn->fmt;
	if ((oldFmt ^ lpColumn->fmt) & (LVCFMT_JUSTIFYMASK | LVCFMT_IMAGE))
	{
	    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
	    if (uView == LVS_REPORT) LISTVIEW_InvalidateColumn(infoPtr, nColumn);
	}
    }

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
  FIXME("iCount %d lpiArray %p\n", iCount, lpiArray);

  if (!lpiArray)
    return FALSE;

  return TRUE;
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    WCHAR szDispText[DISP_TEXT_SIZE] = { 0 };
    INT max_cx = 0;
    HDITEMW hdi;

    TRACE("(nColumn=%d, cx=%d\n", nColumn, cx);

    /* set column width only if in report or list mode */
    if (uView != LVS_REPORT && uView != LVS_LIST) return FALSE;

    /* take care of invalid cx values */
    if(uView == LVS_REPORT && cx < -2) cx = LVSCW_AUTOSIZE;
    else if (uView == LVS_LIST && cx < 1) return FALSE;

    /* resize all columns if in LVS_LIST mode */
    if(uView == LVS_LIST) 
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
	lvItem.pszText = szDispText;
	lvItem.cchTextMax = DISP_TEXT_SIZE;
	for (; lvItem.iItem < infoPtr->nItemCount; lvItem.iItem++)
	{
	    if (!LISTVIEW_GetItemW(infoPtr, &lvItem)) continue;
	    nLabelWidth = LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE);
	    if (max_cx < nLabelWidth) max_cx = nLabelWidth;
	}
	if (infoPtr->himlSmall && (nColumn == 0 || (LISTVIEW_GetColumnInfo(infoPtr, nColumn)->fmt & LVCFMT_IMAGE)))
	    max_cx += infoPtr->iconSize.cx;
	max_cx += TRAILING_LABEL_PADDING;
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
	    hdi.mask = HDI_TEXT;
	    hdi.cchTextMax = DISP_TEXT_SIZE;
	    hdi.pszText = szDispText;
	    if (Header_GetItemW(infoPtr->hwndHeader, nColumn, (LPARAM)&hdi))
	    {
		HDC hdc = GetDC(infoPtr->hwndSelf);
		HFONT old_font = SelectObject(hdc, (HFONT)SendMessageW(infoPtr->hwndHeader, WM_GETFONT, 0, 0));
		SIZE size;

		if (GetTextExtentPoint32W(hdc, hdi.pszText, lstrlenW(hdi.pszText), &size))
		    cx = size.cx + TRAILING_HEADER_PADDING;
		/* FIXME: Take into account the header image, if one is present */
		SelectObject(hdc, old_font);
		ReleaseDC(infoPtr->hwndSelf, hdc);
	    }
	    cx = max (cx, max_cx);
	}
    }

    if (cx < 0) return FALSE;

    /* call header to update the column change */
    hdi.mask = HDI_WIDTH;
    hdi.cxy = cx;
    TRACE("hdi.cxy=%d\n", hdi.cxy);
    return Header_SetItemW(infoPtr->hwndHeader, nColumn, (LPARAM)&hdi);
}

/***
 * Creates the checkbox imagelist.  Helper for LISTVIEW_SetExtendedListViewStyle
 *
 */
static HIMAGELIST LISTVIEW_CreateCheckBoxIL(LISTVIEW_INFO *infoPtr)
{
    HDC hdc_wnd, hdc;
    HBITMAP hbm_im, hbm_mask, hbm_orig;
    RECT rc;
    HBRUSH hbr_white = GetStockObject(WHITE_BRUSH);
    HBRUSH hbr_black = GetStockObject(BLACK_BRUSH);
    HIMAGELIST himl;

    himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                            ILC_COLOR | ILC_MASK, 2, 2);
    hdc_wnd = GetDC(infoPtr->hwndSelf);
    hdc = CreateCompatibleDC(hdc_wnd);
    hbm_im = CreateCompatibleBitmap(hdc_wnd, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    hbm_mask = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 1, NULL);
    ReleaseDC(infoPtr->hwndSelf, hdc_wnd);

    rc.left = rc.top = 0;
    rc.right = GetSystemMetrics(SM_CXSMICON);
    rc.bottom = GetSystemMetrics(SM_CYSMICON);

    hbm_orig = SelectObject(hdc, hbm_mask);
    FillRect(hdc, &rc, hbr_white);
    InflateRect(&rc, -3, -3);
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
static DWORD LISTVIEW_SetExtendedListViewStyle(LISTVIEW_INFO *infoPtr, DWORD dwMask, DWORD dwStyle)
{
    DWORD dwOldStyle = infoPtr->dwLvExStyle;

    /* set new style */
    if (dwMask)
	infoPtr->dwLvExStyle = (dwOldStyle & ~dwMask) | (dwStyle & dwMask);
    else
	infoPtr->dwLvExStyle = dwStyle;

    if((infoPtr->dwLvExStyle ^ dwOldStyle) & LVS_EX_CHECKBOXES)
    {
        HIMAGELIST himl = 0;
        if(infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES)
            himl = LISTVIEW_CreateCheckBoxIL(infoPtr);
        LISTVIEW_SetImageList(infoPtr, LVSIL_STATE, himl);
    }

    return dwOldStyle;
}

/***
 * DESCRIPTION:
 * Sets the new hot cursor used during hot tracking and hover selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I} hCurosr : the new hot cursor handle
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
    DWORD oldspacing = MAKELONG(infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy);
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;

    TRACE("requested=(%d,%d)\n", cx, cy);
    
    /* this is supported only for LVS_ICON style */
    if (uView != LVS_ICON) return oldspacing;
  
    /* set to defaults, if instructed to */
    if (cx == -1) cx = GetSystemMetrics(SM_CXICONSPACING);
    if (cy == -1) cy = GetSystemMetrics(SM_CYICONSPACING);

    /* if 0 then compute width
     * FIXME: Should scan each item and determine max width of
     *        icon or label, then make that the width */
    if (cx == 0)
	cx = infoPtr->iconSpacing.cx;

    /* if 0 then compute height */
    if (cy == 0) 
	cy = infoPtr->iconSize.cy + 2 * infoPtr->ntmHeight +
	     ICON_BOTTOM_PADDING + ICON_TOP_PADDING + LABEL_VERT_PADDING;
    

    infoPtr->iconSpacing.cx = cx;
    infoPtr->iconSpacing.cy = cy;

    TRACE("old=(%d,%d), new=(%d,%d), iconSize=(%ld,%ld), ntmH=%d\n",
	  LOWORD(oldspacing), HIWORD(oldspacing), cx, cy, 
	  infoPtr->iconSize.cx, infoPtr->iconSize.cy,
	  infoPtr->ntmHeight);

    /* these depend on the iconSpacing */
    LISTVIEW_UpdateItemSize(infoPtr);

    return oldspacing;
}

inline void set_icon_size(SIZE *size, HIMAGELIST himl, BOOL small)
{
    INT cx, cy;
    
    if (himl && ImageList_GetIconSize(himl, &cx, &cy))
    {
	size->cx = cx;
	size->cy = cy;
    }
    else
    {
	size->cx = GetSystemMetrics(small ? SM_CXSMICON : SM_CXICON);
	size->cy = GetSystemMetrics(small ? SM_CYSMICON : SM_CYICON);
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT oldHeight = infoPtr->nItemHeight;
    HIMAGELIST himlOld = 0;

    TRACE("(nType=%d, himl=%p\n", nType, himl);

    switch (nType)
    {
    case LVSIL_NORMAL:
        himlOld = infoPtr->himlNormal;
        infoPtr->himlNormal = himl;
        if (uView == LVS_ICON) set_icon_size(&infoPtr->iconSize, himl, FALSE);
        LISTVIEW_SetIconSpacing(infoPtr, 0, 0);
    break;

    case LVSIL_SMALL:
        himlOld = infoPtr->himlSmall;
        infoPtr->himlSmall = himl;
         if (uView != LVS_ICON) set_icon_size(&infoPtr->iconSize, himl, TRUE);
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
    TRACE("(nItems=%d, dwFlags=%lx)\n", nItems, dwFlags);

    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
	INT nOldCount = infoPtr->nItemCount;

	if (nItems < nOldCount)
	{
	    RANGE range = { nItems, nOldCount };
	    ranges_del(infoPtr->selectionRanges, range);
	    if (infoPtr->nFocusedItem >= nItems)
	    {
		infoPtr->nFocusedItem = -1;
		SetRectEmpty(&infoPtr->rcFocus);
	    }
	}

	infoPtr->nItemCount = nItems;
	LISTVIEW_UpdateScroll(infoPtr);

	/* the flags are valid only in ownerdata report and list modes */
	if (uView == LVS_ICON || uView == LVS_SMALLICON) dwFlags = 0;

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
    
	    if (uView == LVS_REPORT)
	    {
		rcErase.left = 0;
		rcErase.top = nFrom * infoPtr->nItemHeight;
		rcErase.right = infoPtr->nItemWidth;
		rcErase.bottom = nTo * infoPtr->nItemHeight;
		OffsetRect(&rcErase, Origin.x, Origin.y);
		if (IntersectRect(&rcErase, &rcErase, &infoPtr->rcList))
		    LISTVIEW_InvalidateRect(infoPtr, &rcErase);
	    }
	    else /* LVS_LIST */
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
static BOOL LISTVIEW_SetItemPosition(LISTVIEW_INFO *infoPtr, INT nItem, POINT pt)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    POINT Origin;

    TRACE("(nItem=%d, &pt=%s\n", nItem, debugpoint(&pt));

    if (nItem < 0 || nItem >= infoPtr->nItemCount ||
	!(uView == LVS_ICON || uView == LVS_SMALLICON)) return FALSE;

    LISTVIEW_GetOrigin(infoPtr, &Origin);

    /* This point value seems to be an undocumented feature.
     * The best guess is that it means either at the origin, 
     * or at true beginning of the list. I will assume the origin. */
    if ((pt.x == -1) && (pt.y == -1))
	pt = Origin;
    
    if (uView == LVS_ICON)
    {
	pt.x -= (infoPtr->nItemWidth - infoPtr->iconSize.cx) / 2;
	pt.y -= ICON_TOP_PADDING;
    }
    pt.x -= Origin.x;
    pt.y -= Origin.y;

    infoPtr->bAutoarrange = FALSE;

    return LISTVIEW_MoveIconTo(infoPtr, nItem, &pt, FALSE);
}

/***
 * DESCRIPTION:
 * Sets the state of one or many items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] lpLVItem : item or subitem info
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemState(LISTVIEW_INFO *infoPtr, INT nItem, const LVITEMW *lpLVItem)
{
    BOOL bResult = TRUE;
    LVITEMW lvItem;

    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_STATE;
    lvItem.state = lpLVItem->state;
    lvItem.stateMask = lpLVItem->stateMask;
    TRACE("lvItem=%s\n", debuglvitem_t(&lvItem, TRUE));

    if (nItem == -1)
    {
    	/* apply to all items */
    	for (lvItem.iItem = 0; lvItem.iItem < infoPtr->nItemCount; lvItem.iItem++)
	    if (!LISTVIEW_SetItemT(infoPtr, &lvItem, TRUE)) bResult = FALSE;
    }
    else
	bResult = LISTVIEW_SetItemT(infoPtr, &lvItem, TRUE);

    /*
     *update selection mark
     *
     * Investigation on windows 2k showed that selection mark was updated
     * whenever a new selection was made, but if the selected item was
     * unselected it was not updated.
     *
     * we are probably still not 100% accurate, but this at least sets the
     * proper selection mark when it is needed
     */

    if (bResult && (lvItem.state & lvItem.stateMask & LVIS_SELECTED) &&
        ((infoPtr->nSelectionMark == -1) || (lvItem.iItem <= infoPtr->nSelectionMark)))
    {
        int i;
        infoPtr->nSelectionMark = -1;
        for (i = 0; i < infoPtr->nItemCount; i++)
        {
            if (infoPtr->uCallbackMask & LVIS_SELECTED)
            {
                if (LISTVIEW_GetItemState(infoPtr, i, LVIS_SELECTED))
                {
                    infoPtr->nSelectionMark = i;
                    break;
                }
            }
            else if (ranges_contain(infoPtr->selectionRanges, i))
            {
                infoPtr->nSelectionMark = i;
                break;
            }
        }
    }

    return bResult;
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

    if (nItem < 0 && nItem >= infoPtr->nItemCount) return FALSE;
    
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

  infoPtr->nSelectionMark = nIndex;

  return nOldIndex;
}

/***
 * DESCRIPTION:
 * Sets the text background color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] clrTextBk : text background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetTextBkColor(LISTVIEW_INFO *infoPtr, COLORREF clrTextBk)
{
    TRACE("(clrTextBk=%lx)\n", clrTextBk);

    if (infoPtr->clrTextBk != clrTextBk)
    {
	infoPtr->clrTextBk = clrTextBk;
	LISTVIEW_InvalidateList(infoPtr);
    }
    
  return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the text foreground color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] clrText : text color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetTextColor (LISTVIEW_INFO *infoPtr, COLORREF clrText)
{
    TRACE("(clrText=%lx)\n", clrText);

    if (infoPtr->clrText != clrText)
    {
	infoPtr->clrText = clrText;
	LISTVIEW_InvalidateList(infoPtr);
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Determines which listview item is located at the specified position.
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

/* LISTVIEW_SetUnicodeFormat */
/* LISTVIEW_SetWorkAreas */

/***
 * DESCRIPTION:
 * Callback internally used by LISTVIEW_SortItems()
 *
 * PARAMETER(S):
 * [I] first : pointer to first ITEM_INFO to compare
 * [I] second : pointer to second ITEM_INFO to compare
 * [I] lParam : HWND of control
 *
 * RETURN:
 *   if first comes before second : negative
 *   if first comes after second : positive
 *   if first and second are equivalent : zero
 */
static INT WINAPI LISTVIEW_CallBackCompare(LPVOID first, LPVOID second, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)lParam;
  ITEM_INFO* lv_first = (ITEM_INFO*) DPA_GetPtr( (HDPA)first, 0 );
  ITEM_INFO* lv_second = (ITEM_INFO*) DPA_GetPtr( (HDPA)second, 0 );

  /* Forward the call to the client defined callback */
  return (infoPtr->pfnCompare)( lv_first->lParam , lv_second->lParam, infoPtr->lParamSort );
}

/***
 * DESCRIPTION:
 * Sorts the listview items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] pfnCompare : application-defined value
 * [I] lParamSort : pointer to comparision callback
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SortItems(LISTVIEW_INFO *infoPtr, PFNLVCOMPARE pfnCompare, LPARAM lParamSort)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    HDPA hdpaSubItems;
    ITEM_INFO *lpItem;
    LPVOID selectionMarkItem;
    LVITEMW item;
    int i;

    TRACE("(pfnCompare=%p, lParamSort=%lx)\n", pfnCompare, lParamSort);

    if (infoPtr->dwStyle & LVS_OWNERDATA) return FALSE;

    if (!infoPtr->hdpaItems) return FALSE;

    /* if there are 0 or 1 items, there is no need to sort */
    if (infoPtr->nItemCount < 2) return TRUE;

    if (infoPtr->nFocusedItem >= 0)
    {
	hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, infoPtr->nFocusedItem);
	lpItem = (ITEM_INFO *)DPA_GetPtr(hdpaSubItems, 0);
	if (lpItem) lpItem->state |= LVIS_FOCUSED;
    }
    /* FIXME: go thorugh selected items and mark them so in lpItem->state */
    /*        clear the lpItem->state for non-selected ones */
    /*        remove the selection ranges */
    
    infoPtr->pfnCompare = pfnCompare;
    infoPtr->lParamSort = lParamSort;
    DPA_Sort(infoPtr->hdpaItems, LISTVIEW_CallBackCompare, (LPARAM)infoPtr);

    /* Adjust selections and indices so that they are the way they should
     * be after the sort (otherwise, the list items move around, but
     * whatever is at the item's previous original position will be
     * selected instead)
     */
    selectionMarkItem=(infoPtr->nSelectionMark>=0)?DPA_GetPtr(infoPtr->hdpaItems, infoPtr->nSelectionMark):NULL;
    for (i=0; i < infoPtr->nItemCount; i++)
    {
	hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, i);
	lpItem = (ITEM_INFO *)DPA_GetPtr(hdpaSubItems, 0);

	if (lpItem->state & LVIS_SELECTED)
	{
	    item.state = LVIS_SELECTED;
	    item.stateMask = LVIS_SELECTED;
	    LISTVIEW_SetItemState(infoPtr, i, &item);
	}
	if (lpItem->state & LVIS_FOCUSED)
	{
            infoPtr->nFocusedItem = i;
	    lpItem->state &= ~LVIS_FOCUSED;
	}
    }
    if (selectionMarkItem != NULL)
	infoPtr->nSelectionMark = DPA_GetPtrIndex(infoPtr->hdpaItems, selectionMarkItem);
    /* I believe nHotItem should be left alone, see LISTVIEW_ShiftIndices */

    /* refresh the display */
    if (uView != LVS_ICON && uView != LVS_SMALLICON)
	LISTVIEW_InvalidateList(infoPtr);

    return TRUE;
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
 * Creates the listview control.
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
  LISTVIEW_INFO *infoPtr;
  UINT uView = lpcs->style & LVS_TYPEMASK;
  LOGFONTW logFont;

  TRACE("(lpcs=%p)\n", lpcs);

  /* initialize info pointer */
  infoPtr = (LISTVIEW_INFO *)Alloc(sizeof(LISTVIEW_INFO));
  if (!infoPtr) return -1;

  SetWindowLongW(hwnd, 0, (LONG)infoPtr);

  infoPtr->hwndSelf = hwnd;
  infoPtr->dwStyle = lpcs->style;
  /* determine the type of structures to use */
  infoPtr->hwndNotify = lpcs->hwndParent;
  infoPtr->notifyFormat = SendMessageW(infoPtr->hwndNotify, WM_NOTIFYFORMAT,
                                       (WPARAM)infoPtr->hwndSelf, (LPARAM)NF_QUERY);

  /* initialize color information  */
  infoPtr->clrBk = CLR_NONE;
  infoPtr->clrText = comctl32_color.clrWindowText;
  infoPtr->clrTextBk = CLR_DEFAULT;
  LISTVIEW_SetBkColor(infoPtr, comctl32_color.clrWindow);

  /* set default values */
  infoPtr->nFocusedItem = -1;
  infoPtr->nSelectionMark = -1;
  infoPtr->nHotItem = -1;
  infoPtr->bRedraw = TRUE;
  infoPtr->bNoItemMetrics = TRUE;
  infoPtr->bDoChangeNotify = TRUE;
  infoPtr->iconSpacing.cx = GetSystemMetrics(SM_CXICONSPACING);
  infoPtr->iconSpacing.cy = GetSystemMetrics(SM_CYICONSPACING);
  infoPtr->nEditLabelItem = -1;
  infoPtr->dwHoverTime = -1; /* default system hover time */

  /* get default font (icon title) */
  SystemParametersInfoW(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
  infoPtr->hDefaultFont = CreateFontIndirectW(&logFont);
  infoPtr->hFont = infoPtr->hDefaultFont;
  LISTVIEW_SaveTextMetrics(infoPtr);

  /* create header */
  infoPtr->hwndHeader =	CreateWindowW(WC_HEADERW, NULL,
    WS_CHILD | HDS_HORZ | (DWORD)((LVS_NOSORTHEADER & lpcs->style)?0:HDS_BUTTONS),
    0, 0, 0, 0, hwnd, NULL,
    lpcs->hInstance, NULL);
  if (!infoPtr->hwndHeader) goto fail;

  /* set header unicode format */
  SendMessageW(infoPtr->hwndHeader, HDM_SETUNICODEFORMAT, (WPARAM)TRUE, (LPARAM)NULL);

  /* set header font */
  SendMessageW(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)infoPtr->hFont, (LPARAM)TRUE);

  /* allocate memory for the data structure */
  if (!(infoPtr->selectionRanges = ranges_create(10))) goto fail;
  if (!(infoPtr->hdpaItems = DPA_Create(10))) goto fail;
  if (!(infoPtr->hdpaPosX  = DPA_Create(10))) goto fail;
  if (!(infoPtr->hdpaPosY  = DPA_Create(10))) goto fail;
  if (!(infoPtr->hdpaColumns = DPA_Create(10))) goto fail;

  /* initialize the icon sizes */
  set_icon_size(&infoPtr->iconSize, infoPtr->himlNormal, uView != LVS_ICON);
  set_icon_size(&infoPtr->iconStateSize, infoPtr->himlState, TRUE);

  /* init item size to avoid division by 0 */
  LISTVIEW_UpdateItemSize (infoPtr);
  
  if (uView == LVS_REPORT)
  {
    if (!(LVS_NOCOLUMNHEADER & lpcs->style))
    {
      ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
    }
    else
    {
      /* set HDS_HIDDEN flag to hide the header bar */
      SetWindowLongW(infoPtr->hwndHeader, GWL_STYLE,
                    GetWindowLongW(infoPtr->hwndHeader, GWL_STYLE) | HDS_HIDDEN);
    }
  }

  return 0;

fail:
    DestroyWindow(infoPtr->hwndHeader);
    ranges_destroy(infoPtr->selectionRanges);
    DPA_Destroy(infoPtr->hdpaItems);
    DPA_Destroy(infoPtr->hdpaPosX);
    DPA_Destroy(infoPtr->hdpaPosY);
    DPA_Destroy(infoPtr->hdpaColumns);
    Free(infoPtr);
    return -1;
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
static inline BOOL LISTVIEW_EraseBkgnd(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    RECT rc;

    TRACE("(hdc=%p)\n", hdc);

    if (!GetClipBox(hdc, &rc)) return FALSE;

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
				INT nScrollDiff, HWND hScrollWnd)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT nOldScrollPos, nNewScrollPos;
    SCROLLINFO scrollInfo;
    BOOL is_an_icon;

    TRACE("(nScrollCode=%d(%s), nScrollDiff=%d)\n", nScrollCode, 
	debugscrollcode(nScrollCode), nScrollDiff);

    if (infoPtr->hwndEdit) SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;

    is_an_icon = ((uView == LVS_ICON) || (uView == LVS_SMALLICON));

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
    if (uView == LVS_REPORT) nScrollDiff *= infoPtr->nItemHeight;
   
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
                                INT nScrollDiff, HWND hScrollWnd)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT nOldScrollPos, nNewScrollPos;
    SCROLLINFO scrollInfo;

    TRACE("(nScrollCode=%d(%s), nScrollDiff=%d)\n", nScrollCode, 
	debugscrollcode(nScrollCode), nScrollDiff);

    if (infoPtr->hwndEdit) SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;

    if (!GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo)) return 1;

    nOldScrollPos = scrollInfo.nPos;

    switch (nScrollCode)
    {
    case SB_INTERNAL:
        break;

    case SB_LINELEFT:
	nScrollDiff = -1;
        break;

    case SB_LINERIGHT:
	nScrollDiff = 1;
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
    
    if(uView == LVS_REPORT)
        LISTVIEW_UpdateHeaderSize(infoPtr, nNewScrollPos);
      
    /* now adjust to client coordinates */
    nScrollDiff = nOldScrollPos - nNewScrollPos;
    if (uView == LVS_LIST) nScrollDiff *= infoPtr->nItemWidth;
   
    /* and scroll the window */
    scroll_list(infoPtr, nScrollDiff, 0);

  return 0;
}

static LRESULT LISTVIEW_MouseWheel(LISTVIEW_INFO *infoPtr, INT wheelDelta)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    INT gcWheelDelta = 0;
    UINT pulScrollLines = 3;
    SCROLLINFO scrollInfo;

    TRACE("(wheelDelta=%d)\n", wheelDelta);

    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);
    gcWheelDelta -= wheelDelta;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;

    switch(uView)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
       /*
        *  listview should be scrolled by a multiple of 37 dependently on its dimension or its visible item number
        *  should be fixed in the future.
        */
        LISTVIEW_VScroll(infoPtr, SB_INTERNAL, (gcWheelDelta < 0) ?
                -LISTVIEW_SCROLL_ICON_LINE_SIZE : LISTVIEW_SCROLL_ICON_LINE_SIZE, 0);
        break;

    case LVS_REPORT:
        if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
        {
            int cLineScroll = min(LISTVIEW_GetCountPerColumn(infoPtr), pulScrollLines);
            cLineScroll *= (gcWheelDelta / WHEEL_DELTA);
            LISTVIEW_VScroll(infoPtr, SB_INTERNAL, cLineScroll, 0);
        }
        break;

    case LVS_LIST:
        LISTVIEW_HScroll(infoPtr, (gcWheelDelta < 0) ? SB_LINELEFT : SB_LINERIGHT, 0, 0);
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
  UINT uView =  infoPtr->dwStyle & LVS_TYPEMASK;
  INT nItem = -1;
  NMLVKEYDOWN nmKeyDown;

  TRACE("(nVirtualKey=%d, lKeyData=%ld)\n", nVirtualKey, lKeyData);

  /* send LVN_KEYDOWN notification */
  nmKeyDown.wVKey = nVirtualKey;
  nmKeyDown.flags = 0;
  notify_hdr(infoPtr, LVN_KEYDOWN, &nmKeyDown.hdr);

  switch (nVirtualKey)
  {
  case VK_RETURN:
    if ((infoPtr->nItemCount > 0) && (infoPtr->nFocusedItem != -1))
    {
      notify(infoPtr, NM_RETURN);
      notify(infoPtr, LVN_ITEMACTIVATE);
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
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_TOLEFT);
    break;

  case VK_UP:
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_ABOVE);
    break;

  case VK_RIGHT:
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_TORIGHT);
    break;

  case VK_DOWN:
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_BELOW);
    break;

  case VK_PRIOR:
    if (uView == LVS_REPORT)
      nItem = infoPtr->nFocusedItem - LISTVIEW_GetCountPerColumn(infoPtr);
    else
      nItem = infoPtr->nFocusedItem - LISTVIEW_GetCountPerColumn(infoPtr)
                                    * LISTVIEW_GetCountPerRow(infoPtr);
    if(nItem < 0) nItem = 0;
    break;

  case VK_NEXT:
    if (uView == LVS_REPORT)
      nItem = infoPtr->nFocusedItem + LISTVIEW_GetCountPerColumn(infoPtr);
    else
      nItem = infoPtr->nFocusedItem + LISTVIEW_GetCountPerColumn(infoPtr)
                                    * LISTVIEW_GetCountPerRow(infoPtr);
    if(nItem >= infoPtr->nItemCount) nItem = infoPtr->nItemCount - 1;
    break;
  }

  if ((nItem != -1) && (nItem != infoPtr->nFocusedItem))
      LISTVIEW_KeySelection(infoPtr, nItem);

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

    /* if we did not have the focus, there's nothing to do */
    if (!infoPtr->bFocus) return 0;
   
    /* send NM_KILLFOCUS notification */
    notify(infoPtr, NM_KILLFOCUS);

    /* if we have a focus rectagle, get rid of it */
    LISTVIEW_ShowFocusRect(infoPtr, FALSE);
    
    /* set window focus flag */
    infoPtr->bFocus = FALSE;

    /* invalidate the selected items before reseting focus flag */
    LISTVIEW_InvalidateSelectedItems(infoPtr);
    
    return 0;
}


/***
 * DESCRIPTION:
 * Track mouse/dragging
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] pt : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_TrackMouse(LISTVIEW_INFO *infoPtr, POINT pt)
{
    INT cxDrag = GetSystemMetrics(SM_CXDRAG);
    INT cyDrag = GetSystemMetrics(SM_CYDRAG);
    RECT r;
    MSG msg;

    TRACE("\n");

    r.top = pt.y - cyDrag;
    r.left = pt.x - cxDrag;
    r.bottom = pt.y + cyDrag;
    r.right = pt.x + cxDrag;

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
 * Processes double click messages (left mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonDblClk(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    LVHITTESTINFO htInfo;

    TRACE("(key=%hu, X=%hu, Y=%hu)\n", wKey, pts.x, pts.y);

    /* send NM_RELEASEDCAPTURE notification */
    notify(infoPtr, NM_RELEASEDCAPTURE);

    htInfo.pt.x = pts.x;
    htInfo.pt.y = pts.y;

    /* send NM_DBLCLK notification */
    LISTVIEW_HitTest(infoPtr, &htInfo, TRUE, FALSE);
    notify_click(infoPtr, NM_DBLCLK, &htInfo);

    /* To send the LVN_ITEMACTIVATE, it must be on an Item */
    if(htInfo.iItem != -1) notify_itemactivate(infoPtr,&htInfo);

    return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse down messages (left mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonDown(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
  LVHITTESTINFO lvHitTestInfo;
  static BOOL bGroupSelect = TRUE;
  POINT pt = { pts.x, pts.y };
  INT nItem;

  TRACE("(key=%hu, X=%hu, Y=%hu)\n", wKey, pts.x, pts.y);

  /* send NM_RELEASEDCAPTURE notification */
  notify(infoPtr, NM_RELEASEDCAPTURE);

  if (!infoPtr->bFocus) SetFocus(infoPtr->hwndSelf);

  /* set left button down flag */
  infoPtr->bLButtonDown = TRUE;

  lvHitTestInfo.pt.x = pts.x;
  lvHitTestInfo.pt.y = pts.y;

  nItem = LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, TRUE);
  TRACE("at %s, nItem=%d\n", debugpoint(&pt), nItem);
  infoPtr->nEditLabelItem = -1;
  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    if ((infoPtr->dwLvExStyle & LVS_EX_CHECKBOXES) && (lvHitTestInfo.flags & LVHT_ONITEMSTATEICON))
    {
        DWORD state = LISTVIEW_GetItemState(infoPtr, nItem, LVIS_STATEIMAGEMASK) >> 12;
        if(state == 1 || state == 2)
        {
            LVITEMW lvitem;
            state ^= 3;
            lvitem.state = state << 12;
            lvitem.stateMask = LVIS_STATEIMAGEMASK;
            LISTVIEW_SetItemState(infoPtr, nItem, &lvitem);
        }
        return 0;
    }
    if (LISTVIEW_TrackMouse(infoPtr, lvHitTestInfo.pt))
    {
        NMLISTVIEW nmlv;

	ZeroMemory(&nmlv, sizeof(nmlv));
        nmlv.iItem = nItem;
        nmlv.ptAction.x = lvHitTestInfo.pt.x;
        nmlv.ptAction.y = lvHitTestInfo.pt.y;

        notify_listview(infoPtr, LVN_BEGINDRAG, &nmlv);

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
          LISTVIEW_AddGroupSelection(infoPtr, nItem);
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
	  infoPtr->nEditLabelItem = nItem;

	/* set selection (clears other pre-existing selections) */
        LISTVIEW_SetSelection(infoPtr, nItem);
      }
    }
  }
  else
  {
    /* remove all selections */
    LISTVIEW_DeselectAll(infoPtr);
    ReleaseCapture();
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse up messages (left mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonUp(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    LVHITTESTINFO lvHitTestInfo;
    
    TRACE("(key=%hu, X=%hu, Y=%hu)\n", wKey, pts.x, pts.y);

    if (!infoPtr->bLButtonDown) return 0;

    lvHitTestInfo.pt.x = pts.x;
    lvHitTestInfo.pt.y = pts.y;

    /* send NM_CLICK notification */
    LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, FALSE);
    notify_click(infoPtr, NM_CLICK, &lvHitTestInfo);

    /* set left button flag */
    infoPtr->bLButtonDown = FALSE;

    /* if we clicked on a selected item, edit the label */
    if(lvHitTestInfo.iItem == infoPtr->nEditLabelItem && (lvHitTestInfo.flags & LVHT_ONITEMLABEL))
        LISTVIEW_EditLabelT(infoPtr, lvHitTestInfo.iItem, TRUE);

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
  TRACE("()\n");

  /* delete all items */
  LISTVIEW_DeleteAllItems(infoPtr);

  /* destroy data structure */
  DPA_Destroy(infoPtr->hdpaItems);
  DPA_Destroy(infoPtr->hdpaPosX);
  DPA_Destroy(infoPtr->hdpaPosY);
  DPA_Destroy(infoPtr->hdpaColumns);
  ranges_destroy(infoPtr->selectionRanges);

  /* destroy image lists */
  if (!(infoPtr->dwStyle & LVS_SHAREIMAGELISTS))
  {
      if (infoPtr->himlNormal)
	  ImageList_Destroy(infoPtr->himlNormal);
      if (infoPtr->himlSmall)
	  ImageList_Destroy(infoPtr->himlSmall);
      if (infoPtr->himlState)
	  ImageList_Destroy(infoPtr->himlState);
  }

  /* destroy font, bkgnd brush */
  infoPtr->hFont = 0;
  if (infoPtr->hDefaultFont) DeleteObject(infoPtr->hDefaultFont);
  if (infoPtr->clrBk != CLR_NONE) DeleteObject(infoPtr->hBkBrush);

  SetWindowLongW(infoPtr->hwndSelf, 0, 0);

  /* free listview info pointer*/
  Free(infoPtr);

  return 0;
}

/***
 * DESCRIPTION:
 * Handles notifications from header.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nCtrlId : control identifier
 * [I] lpnmh : notification information
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_HeaderNotification(LISTVIEW_INFO *infoPtr, const NMHEADERW *lpnmh)
{
    UINT uView =  infoPtr->dwStyle & LVS_TYPEMASK;
    
    TRACE("(lpnmh=%p)\n", lpnmh);

    if (!lpnmh || lpnmh->iItem < 0 || lpnmh->iItem >= DPA_GetPtrCount(infoPtr->hdpaColumns)) return 0;
    
    switch (lpnmh->hdr.code)
    {    
	case HDN_TRACKW:
	case HDN_TRACKA:
	case HDN_ITEMCHANGEDW:
	case HDN_ITEMCHANGEDA:
	{
	    COLUMN_INFO *lpColumnInfo;
	    INT dx, cxy;

	    if (!lpnmh->pitem || !(lpnmh->pitem->mask & HDI_WIDTH))
	    {
    		HDITEMW hdi;
    
		hdi.mask = HDI_WIDTH;
    		if (!Header_GetItemW(infoPtr->hwndHeader, lpnmh->iItem, (LPARAM)&hdi)) return 0;
		cxy = hdi.cxy;
	    }
	    else
		cxy = lpnmh->pitem->cxy;
	    
	    /* determine how much we change since the last know position */
	    lpColumnInfo = LISTVIEW_GetColumnInfo(infoPtr, lpnmh->iItem);
	    dx = cxy - (lpColumnInfo->rcHeader.right - lpColumnInfo->rcHeader.left);
	    if (dx != 0)
	    {
		RECT rcCol = lpColumnInfo->rcHeader;

		lpColumnInfo->rcHeader.right += dx;
		LISTVIEW_ScrollColumns(infoPtr, lpnmh->iItem + 1, dx);
		if (uView == LVS_REPORT && is_redrawing(infoPtr))
		{
		    /* this trick works for left aligned columns only */
		    if ((lpColumnInfo->fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		    {
			rcCol.right = min (rcCol.right, lpColumnInfo->rcHeader.right);
			rcCol.left = max (rcCol.left, rcCol.right - 3 * infoPtr->ntmAveCharWidth);
		    }
		    rcCol.top = infoPtr->rcList.top;
		    rcCol.bottom = infoPtr->rcList.bottom;
		    LISTVIEW_InvalidateRect(infoPtr, &rcCol);
		}
	    }
	}
	break;

	case HDN_ITEMCLICKW:
	case HDN_ITEMCLICKA:
	{
            /* Handle sorting by Header Column */
            NMLISTVIEW nmlv;

            ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
            nmlv.iItem = -1;
            nmlv.iSubItem = lpnmh->iItem;
            notify_listview(infoPtr, LVN_COLUMNCLICK, &nmlv);
        }
	break;
    }

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

    if (nCommand != NF_REQUERY) return 0;
    
    infoPtr->notifyFormat = SendMessageW(hwndFrom, WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);
    
    return 0;
}

/***
 * DESCRIPTION:
 * Paints/Repaints the listview control.
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
	UINT uView =  infoPtr->dwStyle & LVS_TYPEMASK;
	
	infoPtr->bNoItemMetrics = FALSE;
	LISTVIEW_UpdateItemSize(infoPtr);
	if (uView == LVS_ICON || uView == LVS_SMALLICON)
	    LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);
	LISTVIEW_UpdateScroll(infoPtr);
    }
    if (hdc) 
	LISTVIEW_Refresh(infoPtr, hdc);
    else
    {
	PAINTSTRUCT ps;

	hdc = BeginPaint(infoPtr->hwndSelf, &ps);
	if (!hdc) return 1;
	if (ps.fErase) LISTVIEW_FillBkgnd(infoPtr, hdc, &ps.rcPaint);
	LISTVIEW_Refresh(infoPtr, hdc);
	EndPaint(infoPtr->hwndSelf, &ps);
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Processes double click messages (right mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDblClk(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    LVHITTESTINFO lvHitTestInfo;
    
    TRACE("(key=%hu,X=%hu,Y=%hu)\n", wKey, pts.x, pts.y);

    /* send NM_RELEASEDCAPTURE notification */
    notify(infoPtr, NM_RELEASEDCAPTURE);

    /* send NM_RDBLCLK notification */
    lvHitTestInfo.pt.x = pts.x;
    lvHitTestInfo.pt.y = pts.y;
    LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, FALSE);
    notify_click(infoPtr, NM_RDBLCLK, &lvHitTestInfo);

    return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse down messages (right mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDown(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    LVHITTESTINFO lvHitTestInfo;
    INT nItem;

    TRACE("(key=%hu,X=%hu,Y=%hu)\n", wKey, pts.x, pts.y);

    /* send NM_RELEASEDCAPTURE notification */
    notify(infoPtr, NM_RELEASEDCAPTURE);

    /* make sure the listview control window has the focus */
    if (!infoPtr->bFocus) SetFocus(infoPtr->hwndSelf);

    /* set right button down flag */
    infoPtr->bRButtonDown = TRUE;

    /* determine the index of the selected item */
    lvHitTestInfo.pt.x = pts.x;
    lvHitTestInfo.pt.y = pts.y;
    nItem = LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, TRUE);
  
    if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
    {
	LISTVIEW_SetItemFocus(infoPtr, nItem);
	if (!((wKey & MK_SHIFT) || (wKey & MK_CONTROL)) &&
            !LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED))
	    LISTVIEW_SetSelection(infoPtr, nItem);
    }
    else
    {
	LISTVIEW_DeselectAll(infoPtr);
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse up messages (right mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonUp(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    LVHITTESTINFO lvHitTestInfo;
    POINT pt;

    TRACE("(key=%hu,X=%hu,Y=%hu)\n", wKey, pts.x, pts.y);

    if (!infoPtr->bRButtonDown) return 0;
 
    /* set button flag */
    infoPtr->bRButtonDown = FALSE;

    /* Send NM_RClICK notification */
    lvHitTestInfo.pt.x = pts.x;
    lvHitTestInfo.pt.y = pts.y;
    LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE, FALSE);
    notify_click(infoPtr, NM_RCLICK, &lvHitTestInfo);

    /* Change to screen coordinate for WM_CONTEXTMENU */
    pt = lvHitTestInfo.pt;
    ClientToScreen(infoPtr->hwndSelf, &pt);

    /* Send a WM_CONTEXTMENU message in response to the RBUTTONUP */
    SendMessageW(infoPtr->hwndSelf, WM_CONTEXTMENU,
		 (WPARAM)infoPtr->hwndSelf, MAKELPARAM(pt.x, pt.y));

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
static BOOL LISTVIEW_SetCursor(LISTVIEW_INFO *infoPtr, HWND hwnd, UINT nHittest, UINT wMouseMsg)
{
    LVHITTESTINFO lvHitTestInfo;

    if(!(infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT)) return FALSE;

    if(!infoPtr->hHotCursor)  return FALSE;

    GetCursorPos(&lvHitTestInfo.pt);
    if (LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, FALSE, FALSE) < 0) return FALSE;

    SetCursor(infoPtr->hHotCursor);

    return TRUE;
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
    notify(infoPtr, NM_SETFOCUS);

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

    TRACE("(hfont=%p,redraw=%hu)\n", hFont, fRedraw);

    infoPtr->hFont = hFont ? hFont : infoPtr->hDefaultFont;
    if (infoPtr->hFont == oldFont) return 0;
    
    LISTVIEW_SaveTextMetrics(infoPtr);

    if ((infoPtr->dwStyle & LVS_TYPEMASK) == LVS_REPORT)
	SendMessageW(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(fRedraw, 0));

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
 * [I] bRedraw: state of redraw flag
 *
 * RETURN:
 * DefWinProc return value
 */
static LRESULT LISTVIEW_SetRedraw(LISTVIEW_INFO *infoPtr, BOOL bRedraw)
{
    TRACE("infoPtr->bRedraw=%d, bRedraw=%d\n", infoPtr->bRedraw, bRedraw);

    /* we can not use straight equality here because _any_ non-zero value is TRUE */
    if ((infoPtr->bRedraw && bRedraw) || (!infoPtr->bRedraw && !bRedraw)) return 0;

    infoPtr->bRedraw = bRedraw;

    if(!bRedraw) return 0;
    
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
    if ((infoPtr->dwStyle & LVS_TYPEMASK) == LVS_LIST && 
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
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;

    TRACE("uView=%d, rcList(old)=%s\n", uView, debugrect(&infoPtr->rcList));
    
    GetClientRect(infoPtr->hwndSelf, &infoPtr->rcList);

    if (uView == LVS_LIST)
    {
	/* Apparently the "LIST" style is supposed to have the same
	 * number of items in a column even if there is no scroll bar.
	 * Since if a scroll bar already exists then the bottom is already
	 * reduced, only reduce if the scroll bar does not currently exist.
	 * The "2" is there to mimic the native control. I think it may be
	 * related to either padding or edges.  (GLA 7/2002)
	 */
    	if (!(infoPtr->dwStyle & WS_HSCROLL))
	    infoPtr->rcList.bottom -= GetSystemMetrics(SM_CYHSCROLL);
        infoPtr->rcList.bottom = max (infoPtr->rcList.bottom - 2, 0);
    }
    else if (uView == LVS_REPORT && !(infoPtr->dwStyle & LVS_NOCOLUMNHEADER))
    {
	HDLAYOUT hl;
	WINDOWPOS wp;

	hl.prc = &infoPtr->rcList;
	hl.pwpos = &wp;
	Header_Layout(infoPtr->hwndHeader, &hl);

	SetWindowPos(wp.hwnd, wp.hwndInsertAfter, wp.x, wp.y, wp.cx, wp.cy, wp.flags);

	infoPtr->rcList.top = max(wp.cy, 0);
    }

    TRACE("  rcList=%s\n", debugrect(&infoPtr->rcList));
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
    UINT uNewView = lpss->styleNew & LVS_TYPEMASK;
    UINT uOldView = lpss->styleOld & LVS_TYPEMASK;

    TRACE("(styletype=%x, styleOld=0x%08lx, styleNew=0x%08lx)\n",
          wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE) return 0;
  
    /* FIXME: if LVS_NOSORTHEADER changed, update header */
    /*        what if LVS_OWNERDATA changed? */
    /*        or LVS_SINGLESEL */
    /*        or LVS_SORT{AS,DES}CENDING */

    infoPtr->dwStyle = lpss->styleNew;

    if (((lpss->styleOld & WS_HSCROLL) != 0)&&
        ((lpss->styleNew & WS_HSCROLL) == 0))
       ShowScrollBar(infoPtr->hwndSelf, SB_HORZ, FALSE);

    if (((lpss->styleOld & WS_VSCROLL) != 0)&&
        ((lpss->styleNew & WS_VSCROLL) == 0))
       ShowScrollBar(infoPtr->hwndSelf, SB_VERT, FALSE);

    if (uNewView != uOldView)
    {
    	SIZE oldIconSize = infoPtr->iconSize;
    	HIMAGELIST himl;
    
        SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);
    	ShowWindow(infoPtr->hwndHeader, SW_HIDE);

        ShowScrollBar(infoPtr->hwndSelf, SB_BOTH, FALSE);
        SetRectEmpty(&infoPtr->rcFocus);

        himl = (uNewView == LVS_ICON ? infoPtr->himlNormal : infoPtr->himlSmall);
        set_icon_size(&infoPtr->iconSize, himl, uNewView != LVS_ICON);
    
        if (uNewView == LVS_ICON)
        {
            if ((infoPtr->iconSize.cx != oldIconSize.cx) || (infoPtr->iconSize.cy != oldIconSize.cy))
            {
	  	TRACE("icon old size=(%ld,%ld), new size=(%ld,%ld)\n",
		      oldIconSize.cx, oldIconSize.cy, infoPtr->iconSize.cx, infoPtr->iconSize.cy);
	        LISTVIEW_SetIconSpacing(infoPtr, 0, 0);
            }
        }
        else if (uNewView == LVS_REPORT)
        {
            HDLAYOUT hl;
            WINDOWPOS wp;

            hl.prc = &infoPtr->rcList;
            hl.pwpos = &wp;
            Header_Layout(infoPtr->hwndHeader, &hl);
            SetWindowPos(infoPtr->hwndHeader, infoPtr->hwndSelf, wp.x, wp.y, wp.cx, wp.cy, wp.flags);
        }

	LISTVIEW_UpdateItemSize(infoPtr);
    }

    if (uNewView == LVS_REPORT)
	ShowWindow(infoPtr->hwndHeader, (lpss->styleNew & LVS_NOCOLUMNHEADER) ? SW_HIDE : SW_SHOWNORMAL);
     
    if ( (uNewView == LVS_ICON || uNewView == LVS_SMALLICON) &&
	 (uNewView != uOldView || ((lpss->styleNew ^ lpss->styleOld) & LVS_ALIGNMASK)) )
	 LISTVIEW_Arrange(infoPtr, LVA_DEFAULT);

    /* update the size of the client area */
    LISTVIEW_UpdateSize(infoPtr);

    /* add scrollbars if needed */
    LISTVIEW_UpdateScroll(infoPtr);

    /* invalidate client area + erase background */
    LISTVIEW_InvalidateList(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Window procedure of the listview control.
 *
 */
static LRESULT WINAPI
LISTVIEW_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongW(hwnd, 0);

  TRACE("(uMsg=%x wParam=%x lParam=%lx)\n", uMsg, wParam, lParam);

  if (!infoPtr && (uMsg != WM_CREATE))
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);

  if (infoPtr)
  {
    infoPtr->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
  }

  switch (uMsg)
  {
  case LVM_APPROXIMATEVIEWRECT:
    return LISTVIEW_ApproximateViewRect(infoPtr, (INT)wParam,
                                        LOWORD(lParam), HIWORD(lParam));
  case LVM_ARRANGE:
    return LISTVIEW_Arrange(infoPtr, (INT)wParam);

/* case LVM_CANCELEDITLABEL: */

  case LVM_CREATEDRAGIMAGE:
    return (LRESULT)LISTVIEW_CreateDragImage(infoPtr, (INT)wParam, (LPPOINT)lParam);

  case LVM_DELETEALLITEMS:
    return LISTVIEW_DeleteAllItems(infoPtr);

  case LVM_DELETECOLUMN:
    return LISTVIEW_DeleteColumn(infoPtr, (INT)wParam);

  case LVM_DELETEITEM:
    return LISTVIEW_DeleteItem(infoPtr, (INT)wParam);

  case LVM_EDITLABELW:
    return (LRESULT)LISTVIEW_EditLabelT(infoPtr, (INT)wParam, TRUE);

  case LVM_EDITLABELA:
    return (LRESULT)LISTVIEW_EditLabelT(infoPtr, (INT)wParam, FALSE);

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
    return LISTVIEW_GetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, FALSE);

  case LVM_GETCOLUMNW:
    return LISTVIEW_GetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, TRUE);

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
    return LISTVIEW_GetItemExtT(infoPtr, (LPLVITEMW)lParam, FALSE);

  case LVM_GETITEMW:
    return LISTVIEW_GetItemExtT(infoPtr, (LPLVITEMW)lParam, TRUE);

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
    return LISTVIEW_GetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, FALSE);

  case LVM_GETITEMTEXTW:
    return LISTVIEW_GetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, TRUE);

  case LVM_GETNEXTITEM:
    return LISTVIEW_GetNextItem(infoPtr, (INT)wParam, LOWORD(lParam));

  case LVM_GETNUMBEROFWORKAREAS:
    FIXME("LVM_GETNUMBEROFWORKAREAS: unimplemented\n");
    return 1;

  case LVM_GETORIGIN:
    if (!lParam) return FALSE;
    LISTVIEW_GetOrigin(infoPtr, (LPPOINT)lParam);
    return TRUE;

  /* case LVM_GETOUTLINECOLOR: */

  /* case LVM_GETSELECTEDCOLUMN: */

  case LVM_GETSELECTEDCOUNT:
    return LISTVIEW_GetSelectedCount(infoPtr);

  case LVM_GETSELECTIONMARK:
    return infoPtr->nSelectionMark;

  case LVM_GETSTRINGWIDTHA:
    return LISTVIEW_GetStringWidthT(infoPtr, (LPCWSTR)lParam, FALSE);

  case LVM_GETSTRINGWIDTHW:
    return LISTVIEW_GetStringWidthT(infoPtr, (LPCWSTR)lParam, TRUE);

  case LVM_GETSUBITEMRECT:
    return LISTVIEW_GetSubItemRect(infoPtr, (UINT)wParam, (LPRECT)lParam);

  case LVM_GETTEXTBKCOLOR:
    return infoPtr->clrTextBk;

  case LVM_GETTEXTCOLOR:
    return infoPtr->clrText;

  /* case LVM_GETTILEINFO: */

  /* case LVM_GETTILEVIEWINFO: */

  case LVM_GETTOOLTIPS:
    return (LRESULT)infoPtr->hwndToolTip;

  case LVM_GETTOPINDEX:
    return LISTVIEW_GetTopIndex(infoPtr);

  /*case LVM_GETUNICODEFORMAT:
    FIXME("LVM_GETUNICODEFORMAT: unimplemented\n");
    return FALSE;*/

  /* case LVM_GETVIEW: */

  case LVM_GETVIEWRECT:
    return LISTVIEW_GetViewRect(infoPtr, (LPRECT)lParam);

  case LVM_GETWORKAREAS:
    FIXME("LVM_GETWORKAREAS: unimplemented\n");
    return FALSE;

  /* case LVM_HASGROUP: */

  case LVM_HITTEST:
    return LISTVIEW_HitTest(infoPtr, (LPLVHITTESTINFO)lParam, FALSE, FALSE);

  case LVM_INSERTCOLUMNA:
    return LISTVIEW_InsertColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, FALSE);

  case LVM_INSERTCOLUMNW:
    return LISTVIEW_InsertColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, TRUE);

  /* case LVM_INSERTGROUP: */

  /* case LVM_INSERTGROUPSORTED: */

  case LVM_INSERTITEMA:
    return LISTVIEW_InsertItemT(infoPtr, (LPLVITEMW)lParam, FALSE);

  case LVM_INSERTITEMW:
    return LISTVIEW_InsertItemT(infoPtr, (LPLVITEMW)lParam, TRUE);

  /* case LVM_INSERTMARKHITTEST: */

  /* case LVM_ISGROUPVIEWENABLED: */

  /* case LVM_MAPIDTOINDEX: */

  /* case LVM_MAPINDEXTOID: */

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

  /* case LVM_SETBKIMAGE: */

  case LVM_SETCALLBACKMASK:
    infoPtr->uCallbackMask = (UINT)wParam;
    return TRUE;

  case LVM_SETCOLUMNA:
    return LISTVIEW_SetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, FALSE);

  case LVM_SETCOLUMNW:
    return LISTVIEW_SetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, TRUE);

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
    return LISTVIEW_SetHoverTime(infoPtr, (DWORD)wParam);

  case LVM_SETICONSPACING:
    return LISTVIEW_SetIconSpacing(infoPtr, (short)LOWORD(lParam), (short)HIWORD(lParam));

  case LVM_SETIMAGELIST:
    return (LRESULT)LISTVIEW_SetImageList(infoPtr, (INT)wParam, (HIMAGELIST)lParam);

  /* case LVM_SETINFOTIP: */

  /* case LVM_SETINSERTMARK: */

  /* case LVM_SETINSERTMARKCOLOR: */

  case LVM_SETITEMA:
    return LISTVIEW_SetItemT(infoPtr, (LPLVITEMW)lParam, FALSE);

  case LVM_SETITEMW:
    return LISTVIEW_SetItemT(infoPtr, (LPLVITEMW)lParam, TRUE);

  case LVM_SETITEMCOUNT:
    return LISTVIEW_SetItemCount(infoPtr, (INT)wParam, (DWORD)lParam);

  case LVM_SETITEMPOSITION:
    {
	POINT pt;
        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        return LISTVIEW_SetItemPosition(infoPtr, (INT)wParam, pt);
    }

  case LVM_SETITEMPOSITION32:
    if (lParam == 0) return FALSE;
    return LISTVIEW_SetItemPosition(infoPtr, (INT)wParam, *((POINT*)lParam));

  case LVM_SETITEMSTATE:
    return LISTVIEW_SetItemState(infoPtr, (INT)wParam, (LPLVITEMW)lParam);

  case LVM_SETITEMTEXTA:
    return LISTVIEW_SetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, FALSE);

  case LVM_SETITEMTEXTW:
    return LISTVIEW_SetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, TRUE);

  /* case LVM_SETOUTLINECOLOR: */

  /* case LVM_SETSELECTEDCOLUMN: */

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

  /* case LVM_SETUNICODEFORMAT: */

  /* case LVM_SETVIEW: */

  /* case LVM_SETWORKAREAS: */

  /* case LVM_SORTGROUPS: */

  case LVM_SORTITEMS:
    return LISTVIEW_SortItems(infoPtr, (PFNLVCOMPARE)lParam, (LPARAM)wParam);

  /* LVM_SORTITEMSEX: */

  case LVM_SUBITEMHITTEST:
    return LISTVIEW_HitTest(infoPtr, (LPLVHITTESTINFO)lParam, TRUE, FALSE);

  case LVM_UPDATE:
    return LISTVIEW_Update(infoPtr, (INT)wParam);

  case WM_CHAR:
    return LISTVIEW_ProcessLetterKeys( infoPtr, wParam, lParam );

  case WM_COMMAND:
    return LISTVIEW_Command(infoPtr, wParam, lParam);

  case WM_CREATE:
    return LISTVIEW_Create(hwnd, (LPCREATESTRUCTW)lParam);

  case WM_ERASEBKGND:
    return LISTVIEW_EraseBkgnd(infoPtr, (HDC)wParam);

  case WM_GETDLGCODE:
    return DLGC_WANTCHARS | DLGC_WANTARROWS;

  case WM_GETFONT:
    return (LRESULT)infoPtr->hFont;

  case WM_HSCROLL:
    return LISTVIEW_HScroll(infoPtr, (INT)LOWORD(wParam), 0, (HWND)lParam);

  case WM_KEYDOWN:
    return LISTVIEW_KeyDown(infoPtr, (INT)wParam, (LONG)lParam);

  case WM_KILLFOCUS:
    return LISTVIEW_KillFocus(infoPtr);

  case WM_LBUTTONDBLCLK:
    return LISTVIEW_LButtonDblClk(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_LBUTTONDOWN:
    return LISTVIEW_LButtonDown(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_LBUTTONUP:
    return LISTVIEW_LButtonUp(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_MOUSEMOVE:
    return LISTVIEW_MouseMove (infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_MOUSEHOVER:
    return LISTVIEW_MouseHover(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_NCDESTROY:
    return LISTVIEW_NCDestroy(infoPtr);

  case WM_NOTIFY:
    if (lParam && ((LPNMHDR)lParam)->hwndFrom == infoPtr->hwndHeader)
        return LISTVIEW_HeaderNotification(infoPtr, (LPNMHEADERW)lParam);
    else return 0;

  case WM_NOTIFYFORMAT:
    return LISTVIEW_NotifyFormat(infoPtr, (HWND)wParam, (INT)lParam);

  case WM_PAINT:
    return LISTVIEW_Paint(infoPtr, (HDC)wParam);

  case WM_RBUTTONDBLCLK:
    return LISTVIEW_RButtonDblClk(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_RBUTTONDOWN:
    return LISTVIEW_RButtonDown(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_RBUTTONUP:
    return LISTVIEW_RButtonUp(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_SETCURSOR:
    if(LISTVIEW_SetCursor(infoPtr, (HWND)wParam, LOWORD(lParam), HIWORD(lParam)))
      return TRUE;
    goto fwd_msg;

  case WM_SETFOCUS:
    return LISTVIEW_SetFocus(infoPtr, (HWND)wParam);

  case WM_SETFONT:
    return LISTVIEW_SetFont(infoPtr, (HFONT)wParam, (WORD)lParam);

  case WM_SETREDRAW:
    return LISTVIEW_SetRedraw(infoPtr, (BOOL)wParam);

  case WM_SIZE:
    return LISTVIEW_Size(infoPtr, (short)LOWORD(lParam), (short)HIWORD(lParam));

  case WM_STYLECHANGED:
    return LISTVIEW_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

  case WM_SYSCOLORCHANGE:
    COMCTL32_RefreshSysColors();
    return 0;

/*	case WM_TIMER: */

  case WM_VSCROLL:
    return LISTVIEW_VScroll(infoPtr, (INT)LOWORD(wParam), 0, (HWND)lParam);

  case WM_MOUSEWHEEL:
      if (wParam & (MK_SHIFT | MK_CONTROL))
          return DefWindowProcW(hwnd, uMsg, wParam, lParam);
      return LISTVIEW_MouseWheel(infoPtr, (short int)HIWORD(wParam));

  case WM_WINDOWPOSCHANGED:
      if (!(((WINDOWPOS *)lParam)->flags & SWP_NOSIZE)) 
      {
	  SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE |
		       SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
	  LISTVIEW_UpdateSize(infoPtr);
	  LISTVIEW_UpdateScroll(infoPtr);
      }
      return DefWindowProcW(hwnd, uMsg, wParam, lParam);

/*	case WM_WININICHANGE: */

  default:
    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
      ERR("unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam);

  fwd_msg:
    /* call default window procedure */
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }

  return 0;
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
    wndClass.lpfnWndProc = (WNDPROC)LISTVIEW_WindowProc;
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
	    int		  len;

	    if (!infoPtr->hwndEdit || !hdc) return 0;
	    len = GetWindowTextW(infoPtr->hwndEdit, buffer, sizeof(buffer)/sizeof(buffer[0]));
	    GetWindowRect(infoPtr->hwndEdit, &rect);

            /* Select font to get the right dimension of the string */
            hFont = (HFONT)SendMessageW(infoPtr->hwndEdit, WM_GETFONT, 0, 0);
            if(hFont != 0)
            {
                hOldFont = SelectObject(hdc, hFont);
            }

	    if (GetTextExtentPoint32W(hdc, buffer, lstrlenW(buffer), &sz))
	    {
                TEXTMETRICW textMetric;

                /* Add Extra spacing for the next character */
                GetTextMetricsW(hdc, &textMetric);
                sz.cx += (textMetric.tmMaxCharWidth * 2);

		SetWindowPos (
		    infoPtr->hwndEdit,
		    HWND_TOP,
		    0,
		    0,
		    sz.cx,
		    rect.bottom - rect.top,
		    SWP_DRAWFRAME|SWP_NOMOVE);
	    }
            if(hFont != 0)
                SelectObject(hdc, hOldFont);

	    ReleaseDC(infoPtr->hwndSelf, hdc);

	    break;
	}

	default:
	  return SendMessageW (infoPtr->hwndNotify, WM_COMMAND, wParam, lParam);
    }

    return 0;
}


/***
 * DESCRIPTION:
 * Subclassed edit control windproc function
 *
 * PARAMETER(S):
 * [I] hwnd : the edit window handle
 * [I] uMsg : the message that is to be processed
 * [I] wParam : first message parameter
 * [I] lParam : second message parameter
 * [I] isW : TRUE if input is Unicode
 *
 * RETURN:
 *   Zero.
 */
static LRESULT EditLblWndProcT(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL isW)
{
    LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongW(GetParent(hwnd), 0);
    BOOL cancel = FALSE;

    TRACE("(hwnd=%p, uMsg=%x, wParam=%x, lParam=%lx, isW=%d)\n",
	  hwnd, uMsg, wParam, lParam, isW);

    switch (uMsg)
    {
	case WM_GETDLGCODE:
	  return DLGC_WANTARROWS | DLGC_WANTALLKEYS;

	case WM_KILLFOCUS:
	    break;

	case WM_DESTROY:
	{
	    WNDPROC editProc = infoPtr->EditWndProc;
	    infoPtr->EditWndProc = 0;
	    SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)editProc);
	    return CallWindowProcT(editProc, hwnd, uMsg, wParam, lParam, isW);
	}

	case WM_KEYDOWN:
	    if (VK_ESCAPE == (INT)wParam)
	    {
		cancel = TRUE;
                break;
	    }
	    else if (VK_RETURN == (INT)wParam)
		break;

	default:
	    return CallWindowProcT(infoPtr->EditWndProc, hwnd, uMsg, wParam, lParam, isW);
    }

    /* kill the edit */
    if (infoPtr->hwndEdit)
    {
	LPWSTR buffer = NULL;

        infoPtr->hwndEdit = 0;
	if (!cancel)
	{
	    DWORD len = isW ? GetWindowTextLengthW(hwnd) : GetWindowTextLengthA(hwnd);

	    if (len)
	    {
		if ( (buffer = Alloc((len+1) * (isW ? sizeof(WCHAR) : sizeof(CHAR)))) )
		{
		    if (isW) GetWindowTextW(hwnd, buffer, len+1);
		    else GetWindowTextA(hwnd, (CHAR*)buffer, len+1);
		}
	    }
	}
	LISTVIEW_EndEditLabelT(infoPtr, buffer, isW);

	if (buffer) Free(buffer);

    }

    SendMessageW(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

/***
 * DESCRIPTION:
 * Subclassed edit control Unicode windproc function
 *
 * PARAMETER(S):
 * [I] hwnd : the edit window handle
 * [I] uMsg : the message that is to be processed
 * [I] wParam : first message parameter
 * [I] lParam : second message parameter
 *
 * RETURN:
 */
LRESULT CALLBACK EditLblWndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return EditLblWndProcT(hwnd, uMsg, wParam, lParam, TRUE);
}

/***
 * DESCRIPTION:
 * Subclassed edit control ANSI windproc function
 *
 * PARAMETER(S):
 * [I] hwnd : the edit window handle
 * [I] uMsg : the message that is to be processed
 * [I] wParam : first message parameter
 * [I] lParam : second message parameter
 *
 * RETURN:
 */
LRESULT CALLBACK EditLblWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return EditLblWndProcT(hwnd, uMsg, wParam, lParam, FALSE);
}

/***
 * DESCRIPTION:
 * Creates a subclassed edit cotrol
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] text : initial text for the edit
 * [I] style : the window style
 * [I] isW : TRUE if input is Unicode
 *
 * RETURN:
 */
static HWND CreateEditLabelT(LISTVIEW_INFO *infoPtr, LPCWSTR text, DWORD style,
	INT x, INT y, INT width, INT height, BOOL isW)
{
    WCHAR editName[5] = { 'E', 'd', 'i', 't', '\0' };
    HWND hedit;
    SIZE sz;
    HDC hdc;
    HDC hOldFont=0;
    TEXTMETRICW textMetric;
    HINSTANCE hinst = (HINSTANCE)GetWindowLongW(infoPtr->hwndSelf, GWL_HINSTANCE);

    TRACE("(text=%s, ..., isW=%d)\n", debugtext_t(text, isW), isW);

    style |= WS_CHILDWINDOW|WS_CLIPSIBLINGS|ES_LEFT|WS_BORDER;
    hdc = GetDC(infoPtr->hwndSelf);

    /* Select the font to get appropriate metric dimensions */
    if(infoPtr->hFont != 0)
        hOldFont = SelectObject(hdc, infoPtr->hFont);

    /*Get String Length in pixels */
    GetTextExtentPoint32W(hdc, text, lstrlenW(text), &sz);

    /*Add Extra spacing for the next character */
    GetTextMetricsW(hdc, &textMetric);
    sz.cx += (textMetric.tmMaxCharWidth * 2);

    if(infoPtr->hFont != 0)
        SelectObject(hdc, hOldFont);

    ReleaseDC(infoPtr->hwndSelf, hdc);
    if (isW)
	hedit = CreateWindowW(editName, text, style, x, y, sz.cx, height, infoPtr->hwndSelf, 0, hinst, 0);
    else
	hedit = CreateWindowA("Edit", (LPCSTR)text, style, x, y, sz.cx, height, infoPtr->hwndSelf, 0, hinst, 0);

    if (!hedit) return 0;

    infoPtr->EditWndProc = (WNDPROC)
	(isW ? SetWindowLongW(hedit, GWL_WNDPROC, (LONG)EditLblWndProcW) :
               SetWindowLongA(hedit, GWL_WNDPROC, (LONG)EditLblWndProcA) );

    SendMessageW(hedit, WM_SETFONT, (WPARAM)infoPtr->hFont, FALSE);

    return hedit;
}
