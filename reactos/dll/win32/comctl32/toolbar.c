/*
 * Toolbar control
 *
 * Copyright 1998,1999 Eric Kohl
 * Copyright 2000 Eric Kohl for CodeWeavers
 * Copyright 2004 Robert Shearman
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
 * of Comctl32.dll version 6.0 on Mar. 14, 2004, by Robert Shearman.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features or bugs please note them below.
 * 
 * TODO:
 *   - Styles:
 *     - TBSTYLE_REGISTERDROP
 *     - TBSTYLE_EX_DOUBLEBUFFER
 *   - Messages:
 *     - TB_GETMETRICS
 *     - TB_GETOBJECT
 *     - TB_INSERTMARKHITTEST
 *     - TB_SAVERESTORE
 *     - TB_SETMETRICS
 *     - WM_WININICHANGE
 *   - Notifications:
 *     - NM_CHAR
 *     - TBN_GETOBJECT
 *     - TBN_SAVE
 *   - Button wrapping (under construction).
 *   - Fix TB_SETROWS and Separators.
 *   - iListGap custom draw support.
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom contains executables addstr.exe, btncount.exe,
 *     btnstate.exe, butstrsz.exe, chkbtn.exe, chngbmp.exe, customiz.exe,
 *     enablebtn.exe, getbmp.exe, getbtn.exe, getflags.exe, hidebtn.exe,
 *     indetbtn.exe, insbtn.exe, pressbtn.exe, setbtnsz.exe, setcmdid.exe,
 *     setparnt.exe, setrows.exe, toolwnd.exe.
 *   - Microsoft's controlspy examples.
 *   - Charles Petzold's 'Programming Windows': gadgets.exe
 *
 *  Differences between MSDN and actual native control operation:
 *   1. MSDN says: "TBSTYLE_LIST: Creates a flat toolbar with button text
 *                  to the right of the bitmap. Otherwise, this style is
 *                  identical to TBSTYLE_FLAT."
 *      As implemented by both v4.71 and v5.80 of the native COMCTL32.DLL
 *      you can create a TBSTYLE_LIST without TBSTYLE_FLAT and the result
 *      is non-flat non-transparent buttons. Therefore TBSTYLE_LIST does
 *      *not* imply TBSTYLE_FLAT as documented.  (GA 8/2001)
 *
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "tmschema.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolbar);

static HCURSOR hCursorDrag = NULL;

typedef struct
{
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    BYTE  bHot;
    BYTE  bDropDownPressed;
    DWORD_PTR dwData;
    INT_PTR iString;
    INT nRow;
    RECT rect;
    INT cx; /* manually set size */
} TBUTTON_INFO;

typedef struct
{
    UINT nButtons;
    HINSTANCE hInst;
    UINT nID;
} TBITMAP_INFO;

typedef struct
{
    HIMAGELIST himl;
    INT id;
} IMLENTRY, *PIMLENTRY;

typedef struct
{
    DWORD    dwStructSize;    /* size of TBBUTTON struct */
    INT      nWidth;          /* width of the toolbar */
    RECT     client_rect;
    RECT     rcBound;         /* bounding rectangle */
    INT      nButtonHeight;
    INT      nButtonWidth;
    INT      nBitmapHeight;
    INT      nBitmapWidth;
    INT      nIndent;
    INT      nRows;           /* number of button rows */
    INT      nMaxTextRows;    /* maximum number of text rows */
    INT      cxMin;           /* minimum button width */
    INT      cxMax;           /* maximum button width */
    INT      nNumButtons;     /* number of buttons */
    INT      nNumBitmaps;     /* number of bitmaps */
    INT      nNumStrings;     /* number of strings */
    INT      nNumBitmapInfos;
    INT      nButtonDown;     /* toolbar button being pressed or -1 if none */
    INT      nButtonDrag;     /* toolbar button being dragged or -1 if none */
    INT      nOldHit;
    INT      nHotItem;        /* index of the "hot" item */
    SIZE     szPadding;       /* padding values around button */
    INT      iTopMargin;      /* the top margin */
    INT      iListGap;        /* default gap between text and image for toolbar with list style */
    HFONT    hDefaultFont;
    HFONT    hFont;           /* text font */
    HIMAGELIST himlInt;       /* image list created internally */
    PIMLENTRY *himlDef;       /* default image list array */
    INT       cimlDef;        /* default image list array count */
    PIMLENTRY *himlHot;       /* hot image list array */
    INT       cimlHot;        /* hot image list array count */
    PIMLENTRY *himlDis;       /* disabled image list array */
    INT       cimlDis;        /* disabled image list array count */
    HWND     hwndToolTip;     /* handle to tool tip control */
    HWND     hwndNotify;      /* handle to the window that gets notifications */
    HWND     hwndSelf;        /* my own handle */
    BOOL     bAnchor;         /* anchor highlight enabled */
    BOOL     bDoRedraw;       /* Redraw status */
    BOOL     bDragOutSent;    /* has TBN_DRAGOUT notification been sent for this drag? */
    BOOL     bUnicode;        /* Notifications are ASCII (FALSE) or Unicode (TRUE)? */
    BOOL     bCaptured;       /* mouse captured? */
    DWORD      dwStyle;       /* regular toolbar style */
    DWORD      dwExStyle;     /* extended toolbar style */
    DWORD      dwDTFlags;     /* DrawText flags */

    COLORREF   clrInsertMark;   /* insert mark color */
    COLORREF   clrBtnHighlight; /* color for Flat Separator */
    COLORREF   clrBtnShadow;    /* color for Flag Separator */
    INT      iVersion;
    LPWSTR   pszTooltipText;    /* temporary store for a string > 80 characters
                                 * for TTN_GETDISPINFOW notification */
    TBINSERTMARK  tbim;         /* info on insertion mark */
    TBUTTON_INFO *buttons;      /* pointer to button array */
    LPWSTR       *strings;      /* pointer to string array */
    TBITMAP_INFO *bitmaps;
} TOOLBAR_INFO, *PTOOLBAR_INFO;


/* used by customization dialog */
typedef struct
{
    PTOOLBAR_INFO tbInfo;
    HWND          tbHwnd;
} CUSTDLG_INFO, *PCUSTDLG_INFO;

typedef struct
{
    TBBUTTON btn;
    BOOL     bVirtual;
    BOOL     bRemovable;
    WCHAR    text[64];
} CUSTOMBUTTON, *PCUSTOMBUTTON;

typedef enum
{
    IMAGE_LIST_DEFAULT,
    IMAGE_LIST_HOT,
    IMAGE_LIST_DISABLED
} IMAGE_LIST_TYPE;

#define SEPARATOR_WIDTH    8
#define TOP_BORDER         2
#define BOTTOM_BORDER      2
#define DDARROW_WIDTH      11
#define ARROW_HEIGHT       3
#define INSERTMARK_WIDTH   2

#define DEFPAD_CX 7
#define DEFPAD_CY 6
#define DEFLISTGAP 4

/* vertical padding used in list mode when image is present */
#define LISTPAD_CY 9

/* how wide to treat the bitmap if it isn't present */
#define NONLIST_NOTEXT_OFFSET 2

#define TOOLBAR_NOWHERE (-1)

#define TOOLBAR_GetInfoPtr(hwnd) ((TOOLBAR_INFO *)GetWindowLongPtrW(hwnd,0))
#define TOOLBAR_HasText(x, y) (TOOLBAR_GetText(x, y) ? TRUE : FALSE)
#define TOOLBAR_HasDropDownArrows(exStyle) ((exStyle & TBSTYLE_EX_DRAWDDARROWS) ? TRUE : FALSE)

/* Used to find undocumented extended styles */
#define TBSTYLE_EX_ALL (TBSTYLE_EX_DRAWDDARROWS | \
                        TBSTYLE_EX_UNDOC1 | \
                        TBSTYLE_EX_MIXEDBUTTONS | \
                        TBSTYLE_EX_DOUBLEBUFFER | \
                        TBSTYLE_EX_HIDECLIPPEDBUTTONS)

/* all of the CCS_ styles */
#define COMMON_STYLES (CCS_TOP|CCS_NOMOVEY|CCS_BOTTOM|CCS_NORESIZE| \
                       CCS_NOPARENTALIGN|CCS_ADJUSTABLE|CCS_NODIVIDER|CCS_VERT)

#define GETIBITMAP(infoPtr, i) (infoPtr->iVersion >= 5 ? LOWORD(i) : i)
#define GETHIMLID(infoPtr, i) (infoPtr->iVersion >= 5 ? HIWORD(i) : 0)
#define GETDEFIMAGELIST(infoPtr, id) TOOLBAR_GetImageList(infoPtr->himlDef, infoPtr->cimlDef, id)
#define GETHOTIMAGELIST(infoPtr, id) TOOLBAR_GetImageList(infoPtr->himlHot, infoPtr->cimlHot, id)
#define GETDISIMAGELIST(infoPtr, id) TOOLBAR_GetImageList(infoPtr->himlDis, infoPtr->cimlDis, id)

static const WCHAR themeClass[] = { 'T','o','o','l','b','a','r',0 };

static BOOL TOOLBAR_GetButtonInfo(const TOOLBAR_INFO *infoPtr, NMTOOLBARW *nmtb);
static BOOL TOOLBAR_IsButtonRemovable(const TOOLBAR_INFO *infoPtr, int iItem, PCUSTOMBUTTON btnInfo);
static HIMAGELIST TOOLBAR_GetImageList(const PIMLENTRY *pies, INT cies, INT id);
static PIMLENTRY TOOLBAR_GetImageListEntry(const PIMLENTRY *pies, INT cies, INT id);
static VOID TOOLBAR_DeleteImageList(PIMLENTRY **pies, INT *cies);
static HIMAGELIST TOOLBAR_InsertImageList(PIMLENTRY **pies, INT *cies, HIMAGELIST himl, INT id);
static LRESULT TOOLBAR_LButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam);
static void TOOLBAR_SetHotItemEx (TOOLBAR_INFO *infoPtr, INT nHit, DWORD dwReason);
static void TOOLBAR_LayoutToolbar(HWND hwnd);
static LRESULT TOOLBAR_AutoSize(HWND hwnd);
static void TOOLBAR_CheckImageListIconSize(TOOLBAR_INFO *infoPtr);
static void TOOLBAR_TooltipAddTool(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *button);
static void TOOLBAR_TooltipSetRect(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *button);

static LRESULT
TOOLBAR_NotifyFormat(const TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam);

static inline int default_top_margin(const TOOLBAR_INFO *infoPtr)
{
    return (infoPtr->dwStyle & TBSTYLE_FLAT ? 0 : TOP_BORDER);
}

static LPWSTR
TOOLBAR_GetText(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *btnPtr)
{
    LPWSTR lpText = NULL;

    /* NOTE: iString == -1 is undocumented */
    if ((HIWORD(btnPtr->iString) != 0) && (btnPtr->iString != -1))
        lpText = (LPWSTR)btnPtr->iString;
    else if ((btnPtr->iString >= 0) && (btnPtr->iString < infoPtr->nNumStrings))
        lpText = infoPtr->strings[btnPtr->iString];

    return lpText;
}

static void
TOOLBAR_DumpTBButton(const TBBUTTON *tbb, BOOL fUnicode)
{
    TRACE("TBBUTTON: id %d, bitmap=%d, state=%02x, style=%02x, data=%08lx, stringid=0x%08lx (%s)\n",
          tbb->idCommand,tbb->iBitmap, tbb->fsState, tbb->fsStyle, tbb->dwData, tbb->iString,
          (fUnicode ? wine_dbgstr_w((LPWSTR)tbb->iString) : wine_dbgstr_a((LPSTR)tbb->iString)));
}

static void
TOOLBAR_DumpButton(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *bP, INT btn_num)
{
    if (TRACE_ON(toolbar)){
        TRACE("button %d id %d, bitmap=%d, state=%02x, style=%02x, data=%08lx, stringid=0x%08lx\n",
              btn_num, bP->idCommand, GETIBITMAP(infoPtr, bP->iBitmap), 
              bP->fsState, bP->fsStyle, bP->dwData, bP->iString);
	TRACE("string %s\n", debugstr_w(TOOLBAR_GetText(infoPtr,bP)));
        TRACE("button %d id %d, hot=%s, row=%d, rect=(%s)\n",
              btn_num, bP->idCommand, (bP->bHot) ? "TRUE":"FALSE", bP->nRow,
              wine_dbgstr_rect(&bP->rect));
    }
}


static void
TOOLBAR_DumpToolbar(const TOOLBAR_INFO *iP, INT line)
{
    if (TRACE_ON(toolbar)) {
	INT i;

	TRACE("toolbar %p at line %d, exStyle=%08x, buttons=%d, bitmaps=%d, strings=%d, style=%08x\n",
	      iP->hwndSelf, line,
	      iP->dwExStyle, iP->nNumButtons, iP->nNumBitmaps,
	      iP->nNumStrings, iP->dwStyle);
	TRACE("toolbar %p at line %d, himlInt=%p, himlDef=%p, himlHot=%p, himlDis=%p, redrawable=%s\n",
	      iP->hwndSelf, line,
	      iP->himlInt, iP->himlDef, iP->himlHot, iP->himlDis,
	      (iP->bDoRedraw) ? "TRUE" : "FALSE");
 	for(i=0; i<iP->nNumButtons; i++) {
            TOOLBAR_DumpButton(iP, &iP->buttons[i], i);
	}
    }
}


/***********************************************************************
* 		TOOLBAR_CheckStyle
*
* This function validates that the styles set are implemented and
* issues FIXME's warning of possible problems. In a perfect world this
* function should be null.
*/
static void
TOOLBAR_CheckStyle (HWND hwnd, DWORD dwStyle)
{
    if (dwStyle & TBSTYLE_REGISTERDROP)
	FIXME("[%p] TBSTYLE_REGISTERDROP not implemented\n", hwnd);
}


static INT
TOOLBAR_SendNotify (NMHDR *nmhdr, const TOOLBAR_INFO *infoPtr, UINT code)
{
	if(!IsWindow(infoPtr->hwndSelf))
	    return 0;   /* we have just been destroyed */

    nmhdr->idFrom = GetDlgCtrlID (infoPtr->hwndSelf);
    nmhdr->hwndFrom = infoPtr->hwndSelf;
    nmhdr->code = code;

    TRACE("to window %p, code=%08x, %s\n", infoPtr->hwndNotify, code,
	  (infoPtr->bUnicode) ? "via Unicode" : "via ANSI");

    return SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, nmhdr->idFrom, (LPARAM)nmhdr);
}

/***********************************************************************
* 		TOOLBAR_GetBitmapIndex
*
* This function returns the bitmap index associated with a button.
* If the button specifies I_IMAGECALLBACK, then the TBN_GETDISPINFO
* is issued to retrieve the index.
*/
static INT
TOOLBAR_GetBitmapIndex(const TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr)
{
    INT ret = btnPtr->iBitmap;

    if (ret == I_IMAGECALLBACK)
    {
        /* issue TBN_GETDISPINFO */
        NMTBDISPINFOW nmgd;

        memset(&nmgd, 0, sizeof(nmgd));
        nmgd.idCommand = btnPtr->idCommand;
        nmgd.lParam = btnPtr->dwData;
        nmgd.dwMask = TBNF_IMAGE;
        nmgd.iImage = -1;
        /* Windows also send TBN_GETDISPINFOW even if the control is ANSI */
        TOOLBAR_SendNotify(&nmgd.hdr, infoPtr, TBN_GETDISPINFOW);
        if (nmgd.dwMask & TBNF_DI_SETITEM)
            btnPtr->iBitmap = nmgd.iImage;
        ret = nmgd.iImage;
        TRACE("TBN_GETDISPINFO returned bitmap id %d, mask=%08x, nNumBitmaps=%d\n",
            ret, nmgd.dwMask, infoPtr->nNumBitmaps);
    }

    if (ret != I_IMAGENONE)
        ret = GETIBITMAP(infoPtr, ret);

    return ret;
}


static BOOL
TOOLBAR_IsValidBitmapIndex(const TOOLBAR_INFO *infoPtr, INT index)
{
    HIMAGELIST himl;
    INT id = GETHIMLID(infoPtr, index);
    INT iBitmap = GETIBITMAP(infoPtr, index);

    if (((himl = GETDEFIMAGELIST(infoPtr, id)) &&
        iBitmap >= 0 && iBitmap < ImageList_GetImageCount(himl)) ||
        (index == I_IMAGECALLBACK))
      return TRUE;
    else
      return FALSE;
}


static inline BOOL
TOOLBAR_IsValidImageList(const TOOLBAR_INFO *infoPtr, INT index)
{
    HIMAGELIST himl = GETDEFIMAGELIST(infoPtr, GETHIMLID(infoPtr, index));
    return (himl != NULL) && (ImageList_GetImageCount(himl) > 0);
}


/***********************************************************************
* 		TOOLBAR_GetImageListForDrawing
*
* This function validates the bitmap index (including I_IMAGECALLBACK
* functionality) and returns the corresponding image list.
*/
static HIMAGELIST
TOOLBAR_GetImageListForDrawing (const TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr,
                                IMAGE_LIST_TYPE imagelist, INT * index)
{
    HIMAGELIST himl;

    if (!TOOLBAR_IsValidBitmapIndex(infoPtr,btnPtr->iBitmap)) {
	if (btnPtr->iBitmap == I_IMAGENONE) return NULL;
	ERR("bitmap for ID %d, index %d is not valid, number of bitmaps in imagelist: %d\n",
	    HIWORD(btnPtr->iBitmap), LOWORD(btnPtr->iBitmap), infoPtr->nNumBitmaps);
	return NULL;
    }

    if ((*index = TOOLBAR_GetBitmapIndex(infoPtr, btnPtr)) < 0) {
	if ((*index == I_IMAGECALLBACK) ||
	    (*index == I_IMAGENONE)) return NULL;
	ERR("TBN_GETDISPINFO returned invalid index %d\n",
	    *index);
	return NULL;
    }

    switch(imagelist)
    {
    case IMAGE_LIST_DEFAULT:
        himl = GETDEFIMAGELIST(infoPtr, GETHIMLID(infoPtr, btnPtr->iBitmap));
        break;
    case IMAGE_LIST_HOT:
        himl = GETHOTIMAGELIST(infoPtr, GETHIMLID(infoPtr, btnPtr->iBitmap));
        break;
    case IMAGE_LIST_DISABLED:
        himl = GETDISIMAGELIST(infoPtr, GETHIMLID(infoPtr, btnPtr->iBitmap));
        break;
    default:
        himl = NULL;
        FIXME("Shouldn't reach here\n");
    }

    if (!himl)
       TRACE("no image list\n");

    return himl;
}


static void
TOOLBAR_DrawFlatSeparator (const RECT *lpRect, HDC hdc, const TOOLBAR_INFO *infoPtr)
{
    RECT myrect;
    COLORREF oldcolor, newcolor;

    myrect.left = (lpRect->left + lpRect->right) / 2 - 1;
    myrect.right = myrect.left + 1;
    myrect.top = lpRect->top + 2;
    myrect.bottom = lpRect->bottom - 2;

    newcolor = (infoPtr->clrBtnShadow == CLR_DEFAULT) ?
	        comctl32_color.clrBtnShadow : infoPtr->clrBtnShadow;
    oldcolor = SetBkColor (hdc, newcolor);
    ExtTextOutW (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    myrect.left = myrect.right;
    myrect.right = myrect.left + 1;

    newcolor = (infoPtr->clrBtnHighlight == CLR_DEFAULT) ?
	        comctl32_color.clrBtnHighlight : infoPtr->clrBtnHighlight;
    SetBkColor (hdc, newcolor);
    ExtTextOutW (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    SetBkColor (hdc, oldcolor);
}


/***********************************************************************
* 		TOOLBAR_DrawFlatHorizontalSeparator
*
* This function draws horizontal separator for toolbars having CCS_VERT style.
* In this case, the separator is a pixel high line of COLOR_BTNSHADOW,
* followed by a pixel high line of COLOR_BTNHIGHLIGHT. These separators
* are horizontal as opposed to the vertical separators for not dropdown
* type.
*
* FIXME: It is possible that the height of each line is really SM_CYBORDER.
*/
static void
TOOLBAR_DrawFlatHorizontalSeparator (const RECT *lpRect, HDC hdc,
                             const TOOLBAR_INFO *infoPtr)
{
    RECT myrect;
    COLORREF oldcolor, newcolor;

    myrect.left = lpRect->left;
    myrect.right = lpRect->right;
    myrect.top = lpRect->top + (lpRect->bottom - lpRect->top - 2)/2;
    myrect.bottom = myrect.top + 1;

    InflateRect (&myrect, -2, 0);

    TRACE("rect=(%s)\n", wine_dbgstr_rect(&myrect));

    newcolor = (infoPtr->clrBtnShadow == CLR_DEFAULT) ?
	        comctl32_color.clrBtnShadow : infoPtr->clrBtnShadow;
    oldcolor = SetBkColor (hdc, newcolor);
    ExtTextOutW (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    myrect.top = myrect.bottom;
    myrect.bottom = myrect.top + 1;

    newcolor = (infoPtr->clrBtnHighlight == CLR_DEFAULT) ?
	        comctl32_color.clrBtnHighlight : infoPtr->clrBtnHighlight;
    SetBkColor (hdc, newcolor);
    ExtTextOutW (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    SetBkColor (hdc, oldcolor);
}


static void
TOOLBAR_DrawArrow (HDC hdc, INT left, INT top, COLORREF clr)
{
    INT x, y;
    HPEN hPen, hOldPen;

    if (!(hPen = CreatePen( PS_SOLID, 1, clr))) return;
    hOldPen = SelectObject ( hdc, hPen );
    x = left + 2;
    y = top;
    MoveToEx (hdc, x, y, NULL);
    LineTo (hdc, x+5, y++); x++;
    MoveToEx (hdc, x, y, NULL);
    LineTo (hdc, x+3, y++); x++;
    MoveToEx (hdc, x, y, NULL);
    LineTo (hdc, x+1, y);
    SelectObject( hdc, hOldPen );
    DeleteObject( hPen );
}

/*
 * Draw the text string for this button.
 * note: infoPtr->himlDis *SHOULD* be non-zero when infoPtr->himlDef
 * 	is non-zero, so we can simply check himlDef to see if we have
 *      an image list
 */
static void
TOOLBAR_DrawString (const TOOLBAR_INFO *infoPtr, RECT *rcText, LPCWSTR lpText,
                    const NMTBCUSTOMDRAW *tbcd, DWORD dwItemCDFlag)
{
    HDC hdc = tbcd->nmcd.hdc;
    HFONT  hOldFont = 0;
    COLORREF clrOld = 0;
    COLORREF clrOldBk = 0;
    int oldBkMode = 0;
    UINT state = tbcd->nmcd.uItemState;

    /* draw text */
    if (lpText) {
        TRACE("string=%s rect=(%s)\n", debugstr_w(lpText),
              wine_dbgstr_rect(rcText));

	hOldFont = SelectObject (hdc, infoPtr->hFont);
	if ((state & CDIS_HOT) && (dwItemCDFlag & TBCDRF_HILITEHOTTRACK )) {
	    clrOld = SetTextColor (hdc, tbcd->clrTextHighlight);
	}
	else if (state & CDIS_DISABLED) {
	    clrOld = SetTextColor (hdc, tbcd->clrBtnHighlight);
	    OffsetRect (rcText, 1, 1);
	    DrawTextW (hdc, lpText, -1, rcText, infoPtr->dwDTFlags);
	    SetTextColor (hdc, comctl32_color.clr3dShadow);
	    OffsetRect (rcText, -1, -1);
	}
	else if (state & CDIS_INDETERMINATE) {
	    clrOld = SetTextColor (hdc, comctl32_color.clr3dShadow);
	}
	else if ((state & CDIS_MARKED) && !(dwItemCDFlag & TBCDRF_NOMARK)) {
	    clrOld = SetTextColor (hdc, tbcd->clrTextHighlight);
	    clrOldBk = SetBkColor (hdc, tbcd->clrMark);
	    oldBkMode = SetBkMode (hdc, tbcd->nHLStringBkMode);
	}
	else {
	    clrOld = SetTextColor (hdc, tbcd->clrText);
	}

	DrawTextW (hdc, lpText, -1, rcText, infoPtr->dwDTFlags);
	SetTextColor (hdc, clrOld);
	if ((state & CDIS_MARKED) && !(dwItemCDFlag & TBCDRF_NOMARK))
	{
	    SetBkColor (hdc, clrOldBk);
	    SetBkMode (hdc, oldBkMode);
	}
	SelectObject (hdc, hOldFont);
    }
}


static void
TOOLBAR_DrawPattern (const RECT *lpRect, const NMTBCUSTOMDRAW *tbcd)
{
    HDC hdc = tbcd->nmcd.hdc;
    HBRUSH hbr = SelectObject (hdc, tbcd->hbrMonoDither);
    COLORREF clrTextOld;
    COLORREF clrBkOld;
    INT cx = lpRect->right - lpRect->left;
    INT cy = lpRect->bottom - lpRect->top;
    INT cxEdge = GetSystemMetrics(SM_CXEDGE);
    INT cyEdge = GetSystemMetrics(SM_CYEDGE);
    clrTextOld = SetTextColor(hdc, tbcd->clrBtnHighlight);
    clrBkOld = SetBkColor(hdc, tbcd->clrBtnFace);
    PatBlt (hdc, lpRect->left + cxEdge, lpRect->top + cyEdge,
            cx - (2 * cxEdge), cy - (2 * cyEdge), PATCOPY);
    SetBkColor(hdc, clrBkOld);
    SetTextColor(hdc, clrTextOld);
    SelectObject (hdc, hbr);
}


static void TOOLBAR_DrawMasked(HIMAGELIST himl, int index, HDC hdc, INT x, INT y, UINT draw_flags)
{
    INT cx, cy;
    HBITMAP hbmMask, hbmImage;
    HDC hdcMask, hdcImage;

    ImageList_GetIconSize(himl, &cx, &cy);

    /* Create src image */
    hdcImage = CreateCompatibleDC(hdc);
    hbmImage = CreateCompatibleBitmap(hdc, cx, cy);
    SelectObject(hdcImage, hbmImage);
    ImageList_DrawEx(himl, index, hdcImage, 0, 0, cx, cy,
                     RGB(0xff, 0xff, 0xff), RGB(0,0,0), draw_flags);

    /* Create Mask */
    hdcMask = CreateCompatibleDC(0);
    hbmMask = CreateBitmap(cx, cy, 1, 1, NULL);
    SelectObject(hdcMask, hbmMask);

    /* Remove the background and all white pixels */
    ImageList_DrawEx(himl, index, hdcMask, 0, 0, cx, cy,
                     RGB(0xff, 0xff, 0xff), RGB(0,0,0), ILD_MASK);
    SetBkColor(hdcImage, RGB(0xff, 0xff, 0xff));
    BitBlt(hdcMask, 0, 0, cx, cy, hdcImage, 0, 0, NOTSRCERASE);

    /* draw the new mask 'etched' to hdc */
    SetBkColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, GetSysColorBrush(COLOR_3DHILIGHT));
    /* E20746 op code is (Dst ^ (Src & (Pat ^ Dst))) */
    BitBlt(hdc, x + 1, y + 1, cx, cy, hdcMask, 0, 0, 0xE20746);
    SelectObject(hdc, GetSysColorBrush(COLOR_3DSHADOW));
    BitBlt(hdc, x, y, cx, cy, hdcMask, 0, 0, 0xE20746);

    /* Cleanup */
    DeleteObject(hbmImage);
    DeleteDC(hdcImage);
    DeleteObject (hbmMask);
    DeleteDC(hdcMask);
}


static UINT
TOOLBAR_TranslateState(const TBUTTON_INFO *btnPtr)
{
    UINT retstate = 0;

    retstate |= (btnPtr->fsState & TBSTATE_CHECKED) ? CDIS_CHECKED  : 0;
    retstate |= (btnPtr->fsState & TBSTATE_PRESSED) ? CDIS_SELECTED : 0;
    retstate |= (btnPtr->fsState & TBSTATE_ENABLED) ? 0 : CDIS_DISABLED;
    retstate |= (btnPtr->fsState & TBSTATE_MARKED ) ? CDIS_MARKED   : 0;
    retstate |= (btnPtr->bHot                     ) ? CDIS_HOT      : 0;
    retstate |= ((btnPtr->fsState & (TBSTATE_ENABLED|TBSTATE_INDETERMINATE)) == (TBSTATE_ENABLED|TBSTATE_INDETERMINATE)) ? CDIS_INDETERMINATE : 0;
    /* NOTE: we don't set CDIS_GRAYED, CDIS_FOCUS, CDIS_DEFAULT */
    return retstate;
}

/* draws the image on a toolbar button */
static void
TOOLBAR_DrawImage(const TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr, INT left, INT top,
                  const NMTBCUSTOMDRAW *tbcd, DWORD dwItemCDFlag)
{
    HIMAGELIST himl = NULL;
    BOOL draw_masked = FALSE;
    INT index;
    INT offset = 0;
    UINT draw_flags = ILD_TRANSPARENT;

    if (tbcd->nmcd.uItemState & (CDIS_DISABLED | CDIS_INDETERMINATE))
    {
        himl = TOOLBAR_GetImageListForDrawing(infoPtr, btnPtr, IMAGE_LIST_DISABLED, &index);
        if (!himl)
        {
            himl = TOOLBAR_GetImageListForDrawing(infoPtr, btnPtr, IMAGE_LIST_DEFAULT, &index);
            draw_masked = TRUE;
        }
    }
    else if (tbcd->nmcd.uItemState & CDIS_CHECKED ||
      ((tbcd->nmcd.uItemState & CDIS_HOT) 
      && ((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf))))
    {
        /* if hot, attempt to draw with hot image list, if fails, 
           use default image list */
        himl = TOOLBAR_GetImageListForDrawing(infoPtr, btnPtr, IMAGE_LIST_HOT, &index);
        if (!himl)
            himl = TOOLBAR_GetImageListForDrawing(infoPtr, btnPtr, IMAGE_LIST_DEFAULT, &index);
	}
    else
        himl = TOOLBAR_GetImageListForDrawing(infoPtr, btnPtr, IMAGE_LIST_DEFAULT, &index);

    if (!himl)
        return;

    if (!(dwItemCDFlag & TBCDRF_NOOFFSET) && 
        (tbcd->nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED)))
        offset = 1;

    if (!(dwItemCDFlag & TBCDRF_NOMARK) &&
        (tbcd->nmcd.uItemState & CDIS_MARKED))
        draw_flags |= ILD_BLEND50;

    TRACE("drawing index=%d, himl=%p, left=%d, top=%d, offset=%d\n",
      index, himl, left, top, offset);

    if (draw_masked)
        TOOLBAR_DrawMasked (himl, index, tbcd->nmcd.hdc, left + offset, top + offset, draw_flags);
    else
        ImageList_Draw (himl, index, tbcd->nmcd.hdc, left + offset, top + offset, draw_flags);
}

/* draws a blank frame for a toolbar button */
static void
TOOLBAR_DrawFrame(const TOOLBAR_INFO *infoPtr, const NMTBCUSTOMDRAW *tbcd, DWORD dwItemCDFlag)
{
    HDC hdc = tbcd->nmcd.hdc;
    RECT rc = tbcd->nmcd.rc;
    /* if the state is disabled or indeterminate then the button
     * cannot have an interactive look like pressed or hot */
    BOOL non_interactive_state = (tbcd->nmcd.uItemState & CDIS_DISABLED) ||
                                 (tbcd->nmcd.uItemState & CDIS_INDETERMINATE);
    BOOL pressed_look = !non_interactive_state &&
                        ((tbcd->nmcd.uItemState & CDIS_SELECTED) || 
                         (tbcd->nmcd.uItemState & CDIS_CHECKED));

    /* app don't want us to draw any edges */
    if (dwItemCDFlag & TBCDRF_NOEDGES)
        return;

    if (infoPtr->dwStyle & TBSTYLE_FLAT)
    {
        if (pressed_look)
            DrawEdge (hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
        else if ((tbcd->nmcd.uItemState & CDIS_HOT) && !non_interactive_state)
            DrawEdge (hdc, &rc, BDR_RAISEDINNER, BF_RECT);
    }
    else
    {
        if (pressed_look)
            DrawEdge (hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_MIDDLE);
        else
            DrawEdge (hdc, &rc, EDGE_RAISED,
              BF_SOFT | BF_RECT | BF_MIDDLE);
    }
}

static void
TOOLBAR_DrawSepDDArrow(const TOOLBAR_INFO *infoPtr, const NMTBCUSTOMDRAW *tbcd, RECT *rcArrow, BOOL bDropDownPressed, DWORD dwItemCDFlag)
{
    HDC hdc = tbcd->nmcd.hdc;
    int offset = 0;
    BOOL pressed = bDropDownPressed ||
        (tbcd->nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED));

    if (infoPtr->dwStyle & TBSTYLE_FLAT)
    {
        if (pressed)
            DrawEdge (hdc, rcArrow, BDR_SUNKENOUTER, BF_RECT);
        else if ( (tbcd->nmcd.uItemState & CDIS_HOT) &&
                 !(tbcd->nmcd.uItemState & CDIS_DISABLED) &&
                 !(tbcd->nmcd.uItemState & CDIS_INDETERMINATE))
            DrawEdge (hdc, rcArrow, BDR_RAISEDINNER, BF_RECT);
    }
    else
    {
        if (pressed)
            DrawEdge (hdc, rcArrow, EDGE_SUNKEN, BF_RECT | BF_MIDDLE);
        else
            DrawEdge (hdc, rcArrow, EDGE_RAISED,
              BF_SOFT | BF_RECT | BF_MIDDLE);
    }

    if (pressed)
        offset = (dwItemCDFlag & TBCDRF_NOOFFSET) ? 0 : 1;

    if (tbcd->nmcd.uItemState & (CDIS_DISABLED | CDIS_INDETERMINATE))
    {
        TOOLBAR_DrawArrow(hdc, rcArrow->left+1, rcArrow->top+1 + (rcArrow->bottom - rcArrow->top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnHighlight);
        TOOLBAR_DrawArrow(hdc, rcArrow->left, rcArrow->top + (rcArrow->bottom - rcArrow->top - ARROW_HEIGHT) / 2, comctl32_color.clr3dShadow);
    }
    else
        TOOLBAR_DrawArrow(hdc, rcArrow->left + offset, rcArrow->top + offset + (rcArrow->bottom - rcArrow->top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnText);
}

/* draws a complete toolbar button */
static void
TOOLBAR_DrawButton (HWND hwnd, TBUTTON_INFO *btnPtr, HDC hdc, DWORD dwBaseCustDraw)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = infoPtr->dwStyle;
    BOOL hasDropDownArrow = (TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle) &&
                            (btnPtr->fsStyle & BTNS_DROPDOWN)) ||
                            (btnPtr->fsStyle & BTNS_WHOLEDROPDOWN);
    BOOL drawSepDropDownArrow = hasDropDownArrow && 
                                (~btnPtr->fsStyle & BTNS_WHOLEDROPDOWN);
    RECT rc, rcArrow, rcBitmap, rcText;
    LPWSTR lpText = NULL;
    NMTBCUSTOMDRAW tbcd;
    DWORD ntfret;
    INT offset;
    INT oldBkMode;
    DWORD dwItemCustDraw;
    DWORD dwItemCDFlag;
    HTHEME theme = GetWindowTheme (hwnd);

    rc = btnPtr->rect;
    CopyRect (&rcArrow, &rc);

    /* separator - doesn't send NM_CUSTOMDRAW */
    if (btnPtr->fsStyle & BTNS_SEP) {
        if (theme)
        {
            DrawThemeBackground (theme, hdc, 
                (dwStyle & CCS_VERT) ? TP_SEPARATORVERT : TP_SEPARATOR, 0, 
                &rc, NULL);
        }
        else
        /* with the FLAT style, iBitmap is the width and has already */
        /* been taken into consideration in calculating the width    */
        /* so now we need to draw the vertical separator             */
        /* empirical tests show that iBitmap can/will be non-zero    */
        /* when drawing the vertical bar...      */
        if ((dwStyle & TBSTYLE_FLAT) /* && (btnPtr->iBitmap == 0) */) {
            if (dwStyle & CCS_VERT)
               TOOLBAR_DrawFlatHorizontalSeparator (&rc, hdc, infoPtr);
	    else
		TOOLBAR_DrawFlatSeparator (&rc, hdc, infoPtr);
	}
	else if (btnPtr->fsStyle != BTNS_SEP) {
	    FIXME("Draw some kind of separator: fsStyle=%x\n",
		  btnPtr->fsStyle);
	}
	return;
    }

    /* get a pointer to the text */
    lpText = TOOLBAR_GetText(infoPtr, btnPtr);

    if (hasDropDownArrow)
    {
        int right;

        if (dwStyle & TBSTYLE_FLAT)
            right = max(rc.left, rc.right - DDARROW_WIDTH);
        else
            right = max(rc.left, rc.right - DDARROW_WIDTH - 2);

        if (drawSepDropDownArrow)
           rc.right = right;

        rcArrow.left = right;
    }

    /* copy text & bitmap rects after adjusting for drop-down arrow
     * so that text & bitmap is centered in the rectangle not containing
     * the arrow */
    CopyRect(&rcText, &rc);
    CopyRect(&rcBitmap, &rc);

    /* Center the bitmap horizontally and vertically */
    if (dwStyle & TBSTYLE_LIST)
    {
        if (lpText &&
            infoPtr->nMaxTextRows > 0 &&
            (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) ||
            (btnPtr->fsStyle & BTNS_SHOWTEXT)) )
            rcBitmap.left += GetSystemMetrics(SM_CXEDGE) + infoPtr->szPadding.cx / 2;
        else
            rcBitmap.left += GetSystemMetrics(SM_CXEDGE) + infoPtr->iListGap / 2;
    }
    else
        rcBitmap.left += ((rc.right - rc.left) - infoPtr->nBitmapWidth) / 2;

    rcBitmap.top += infoPtr->szPadding.cy / 2;

    TRACE("iBitmap=%d, start=(%d,%d) w=%d, h=%d\n",
      btnPtr->iBitmap, rcBitmap.left, rcBitmap.top,
      infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);
    TRACE("Text=%s\n", debugstr_w(lpText));
    TRACE("iListGap=%d, padding = { %d, %d }\n", infoPtr->iListGap, infoPtr->szPadding.cx, infoPtr->szPadding.cy);

    /* calculate text position */
    if (lpText)
    {
        rcText.left += GetSystemMetrics(SM_CXEDGE);
        rcText.right -= GetSystemMetrics(SM_CXEDGE);
        if (dwStyle & TBSTYLE_LIST)
        {
            rcText.left += infoPtr->nBitmapWidth + infoPtr->iListGap + 2;
        }
        else
        {
            if (ImageList_GetImageCount(GETDEFIMAGELIST(infoPtr, 0)) > 0)
                rcText.top += infoPtr->szPadding.cy/2 + infoPtr->nBitmapHeight + 1;
            else
                rcText.top += infoPtr->szPadding.cy/2 + 2;
        }
    }

    /* Initialize fields in all cases, because we use these later
     * NOTE: applications can and do alter these to customize their
     * toolbars */
    ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
    tbcd.clrText = comctl32_color.clrBtnText;
    tbcd.clrTextHighlight = comctl32_color.clrHighlightText;
    tbcd.clrBtnFace = comctl32_color.clrBtnFace;
    tbcd.clrBtnHighlight = comctl32_color.clrBtnHighlight;
    tbcd.clrMark = comctl32_color.clrHighlight;
    tbcd.clrHighlightHotTrack = 0;
    tbcd.nStringBkMode = TRANSPARENT;
    tbcd.nHLStringBkMode = OPAQUE;
    /* MSDN says that this is the text rectangle.
     * But (why always a but) tracing of v5.7 of native shows
     * that this is really a *relative* rectangle based on the
     * the nmcd.rc. Also the left and top are always 0 ignoring
     * any bitmap that might be present. */
    tbcd.rcText.left = 0;
    tbcd.rcText.top = 0;
    tbcd.rcText.right = rcText.right - rc.left;
    tbcd.rcText.bottom = rcText.bottom - rc.top;
    tbcd.nmcd.uItemState = TOOLBAR_TranslateState(btnPtr);
    tbcd.nmcd.hdc = hdc;
    tbcd.nmcd.rc = rc;
    tbcd.hbrMonoDither = COMCTL32_hPattern55AABrush;

    /* FIXME: what are these used for? */
    tbcd.hbrLines = 0;
    tbcd.hpenLines = 0;

    /* Issue Item Prepaint notify */
    dwItemCustDraw = 0;
    dwItemCDFlag = 0;
    if (dwBaseCustDraw & CDRF_NOTIFYITEMDRAW)
    {
	tbcd.nmcd.dwDrawStage = CDDS_ITEMPREPAINT;
	tbcd.nmcd.dwItemSpec = btnPtr->idCommand;
	tbcd.nmcd.lItemlParam = btnPtr->dwData;
	ntfret = TOOLBAR_SendNotify(&tbcd.nmcd.hdr, infoPtr, NM_CUSTOMDRAW);
        /* reset these fields so the user can't alter the behaviour like native */
        tbcd.nmcd.hdc = hdc;
        tbcd.nmcd.rc = rc;

	dwItemCustDraw = ntfret & 0xffff;
	dwItemCDFlag = ntfret & 0xffff0000;
	if (dwItemCustDraw & CDRF_SKIPDEFAULT)
	    return;
	/* save the only part of the rect that the user can change */
	rcText.right = tbcd.rcText.right + rc.left;
	rcText.bottom = tbcd.rcText.bottom + rc.top;
    }

    if (!(dwItemCDFlag & TBCDRF_NOOFFSET) &&
        (btnPtr->fsState & (TBSTATE_PRESSED | TBSTATE_CHECKED)))
        OffsetRect(&rcText, 1, 1);

    if (!(tbcd.nmcd.uItemState & CDIS_HOT) && 
        ((tbcd.nmcd.uItemState & CDIS_CHECKED) || (tbcd.nmcd.uItemState & CDIS_INDETERMINATE)))
        TOOLBAR_DrawPattern (&rc, &tbcd);

    if (((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)) 
        && (tbcd.nmcd.uItemState & CDIS_HOT))
    {
        if ( dwItemCDFlag & TBCDRF_HILITEHOTTRACK )
        {
            COLORREF oldclr;

            oldclr = SetBkColor(hdc, tbcd.clrHighlightHotTrack);
            ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, 0);
            if (hasDropDownArrow)
                ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rcArrow, NULL, 0, 0);
            SetBkColor(hdc, oldclr);
        }
    }

    if (theme)
    {
        int partId = drawSepDropDownArrow ? TP_SPLITBUTTON : TP_BUTTON;
        int stateId = TS_NORMAL;
        
        if (tbcd.nmcd.uItemState & CDIS_DISABLED)
            stateId = TS_DISABLED;
        else if (tbcd.nmcd.uItemState & CDIS_SELECTED)
            stateId = TS_PRESSED;
        else if (tbcd.nmcd.uItemState & CDIS_CHECKED)
            stateId = (tbcd.nmcd.uItemState & CDIS_HOT) ? TS_HOTCHECKED : TS_HOT;
        else if ((tbcd.nmcd.uItemState & CDIS_HOT)
            || (drawSepDropDownArrow && btnPtr->bDropDownPressed))
            stateId = TS_HOT;
            
        DrawThemeBackground (theme, hdc, partId, stateId, &tbcd.nmcd.rc, NULL);
    }
    else
        TOOLBAR_DrawFrame(infoPtr, &tbcd, dwItemCDFlag);

    if (drawSepDropDownArrow)
    {
        if (theme)
        {
            int stateId = TS_NORMAL;
            
            if (tbcd.nmcd.uItemState & CDIS_DISABLED)
                stateId = TS_DISABLED;
            else if (btnPtr->bDropDownPressed || (tbcd.nmcd.uItemState & CDIS_SELECTED))
                stateId = TS_PRESSED;
            else if (tbcd.nmcd.uItemState & CDIS_CHECKED)
                stateId = (tbcd.nmcd.uItemState & CDIS_HOT) ? TS_HOTCHECKED : TS_HOT;
            else if (tbcd.nmcd.uItemState & CDIS_HOT)
                stateId = TS_HOT;
                
            DrawThemeBackground (theme, hdc, TP_DROPDOWNBUTTON, stateId, &rcArrow, NULL);
            DrawThemeBackground (theme, hdc, TP_SPLITBUTTONDROPDOWN, stateId, &rcArrow, NULL);
        }
        else
            TOOLBAR_DrawSepDDArrow(infoPtr, &tbcd, &rcArrow, btnPtr->bDropDownPressed, dwItemCDFlag);
    }

    oldBkMode = SetBkMode (hdc, tbcd.nStringBkMode);
    if (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || (btnPtr->fsStyle & BTNS_SHOWTEXT))
        TOOLBAR_DrawString (infoPtr, &rcText, lpText, &tbcd, dwItemCDFlag);
    SetBkMode (hdc, oldBkMode);

    TOOLBAR_DrawImage(infoPtr, btnPtr, rcBitmap.left, rcBitmap.top, &tbcd, dwItemCDFlag);

    if (hasDropDownArrow && !drawSepDropDownArrow)
    {
        if (tbcd.nmcd.uItemState & (CDIS_DISABLED | CDIS_INDETERMINATE))
        {
            TOOLBAR_DrawArrow(hdc, rcArrow.left+1, rcArrow.top+1 + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnHighlight);
            TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clr3dShadow);
        }
        else if (tbcd.nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED))
        {
            offset = (dwItemCDFlag & TBCDRF_NOOFFSET) ? 0 : 1;
            TOOLBAR_DrawArrow(hdc, rcArrow.left + offset, rcArrow.top + offset + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnText);
        }
        else
            TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnText);
    }

    if (dwItemCustDraw & CDRF_NOTIFYPOSTPAINT)
    {
        tbcd.nmcd.dwDrawStage = CDDS_ITEMPOSTPAINT;
        TOOLBAR_SendNotify(&tbcd.nmcd.hdr, infoPtr, NM_CUSTOMDRAW);
    }

}


static void
TOOLBAR_Refresh (HWND hwnd, HDC hdc, const PAINTSTRUCT *ps)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT i;
    RECT rcTemp, rcClient;
    NMTBCUSTOMDRAW tbcd;
    DWORD ntfret;
    DWORD dwBaseCustDraw;

    /* the app has told us not to redraw the toolbar */
    if (!infoPtr->bDoRedraw)
        return;

    /* if imagelist belongs to the app, it can be changed
       by the app after setting it */
    if (GETDEFIMAGELIST(infoPtr, 0) != infoPtr->himlInt)
    {
        infoPtr->nNumBitmaps = 0;
        for (i = 0; i < infoPtr->cimlDef; i++)
            infoPtr->nNumBitmaps += ImageList_GetImageCount(infoPtr->himlDef[i]->himl);
    }

    TOOLBAR_DumpToolbar (infoPtr, __LINE__);

    /* change the imagelist icon size if we manage the list and it is necessary */
    TOOLBAR_CheckImageListIconSize(infoPtr);

    /* Send initial notify */
    ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
    tbcd.nmcd.dwDrawStage = CDDS_PREPAINT;
    tbcd.nmcd.hdc = hdc;
    tbcd.nmcd.rc = ps->rcPaint;
    ntfret = TOOLBAR_SendNotify(&tbcd.nmcd.hdr, infoPtr, NM_CUSTOMDRAW);
    dwBaseCustDraw = ntfret & 0xffff;

    GetClientRect(hwnd, &rcClient);

    /* redraw necessary buttons */
    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++)
    {
        BOOL bDraw;
        if (!RectVisible(hdc, &btnPtr->rect))
            continue;
        if (infoPtr->dwExStyle & TBSTYLE_EX_HIDECLIPPEDBUTTONS)
        {
            IntersectRect(&rcTemp, &rcClient, &btnPtr->rect);
            bDraw = EqualRect(&rcTemp, &btnPtr->rect);
        }
        else
            bDraw = TRUE;
        bDraw &= IntersectRect(&rcTemp, &(ps->rcPaint), &(btnPtr->rect));
        bDraw = (btnPtr->fsState & TBSTATE_HIDDEN) ? FALSE : bDraw;
        if (bDraw)
            TOOLBAR_DrawButton(hwnd, btnPtr, hdc, dwBaseCustDraw);
    }

    /* draw insert mark if required */
    if (infoPtr->tbim.iButton != -1)
    {
        RECT rcButton = infoPtr->buttons[infoPtr->tbim.iButton].rect;
        RECT rcInsertMark;
        rcInsertMark.top = rcButton.top;
        rcInsertMark.bottom = rcButton.bottom;
        if (infoPtr->tbim.dwFlags & TBIMHT_AFTER)
            rcInsertMark.left = rcInsertMark.right = rcButton.right;
        else
            rcInsertMark.left = rcInsertMark.right = rcButton.left - INSERTMARK_WIDTH;
        COMCTL32_DrawInsertMark(hdc, &rcInsertMark, infoPtr->clrInsertMark, FALSE);
    }

    if (dwBaseCustDraw & CDRF_NOTIFYPOSTPAINT)
    {
	ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
	tbcd.nmcd.dwDrawStage = CDDS_POSTPAINT;
	tbcd.nmcd.hdc = hdc;
	tbcd.nmcd.rc = ps->rcPaint;
	TOOLBAR_SendNotify(&tbcd.nmcd.hdr, infoPtr, NM_CUSTOMDRAW);
    }
}

/***********************************************************************
* 		TOOLBAR_MeasureString
*
* This function gets the width and height of a string in pixels. This
* is done first by using GetTextExtentPoint to get the basic width
* and height. The DrawText is called with DT_CALCRECT to get the exact
* width. The reason is because the text may have more than one "&" (or
* prefix characters as M$ likes to call them). The prefix character
* indicates where the underline goes, except for the string "&&" which
* is reduced to a single "&". GetTextExtentPoint does not process these
* only DrawText does. Note that the BTNS_NOPREFIX is handled here.
*/
static void
TOOLBAR_MeasureString(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *btnPtr,
		      HDC hdc, LPSIZE lpSize)
{
    RECT myrect;

    lpSize->cx = 0;
    lpSize->cy = 0;

    if (infoPtr->nMaxTextRows > 0 &&
        !(btnPtr->fsState & TBSTATE_HIDDEN) &&
        (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) ||
        (btnPtr->fsStyle & BTNS_SHOWTEXT)) )
    {
        LPWSTR lpText = TOOLBAR_GetText(infoPtr, btnPtr);

	if(lpText != NULL) {
	    /* first get size of all the text */
	    GetTextExtentPoint32W (hdc, lpText, strlenW (lpText), lpSize);

	    /* feed above size into the rectangle for DrawText */
	    myrect.left = myrect.top = 0;
	    myrect.right = lpSize->cx;
	    myrect.bottom = lpSize->cy;

	    /* Use DrawText to get true size as drawn (less pesky "&") */
	    DrawTextW (hdc, lpText, -1, &myrect, DT_VCENTER | DT_SINGLELINE |
	    	   DT_CALCRECT | ((btnPtr->fsStyle & BTNS_NOPREFIX) ?
				  DT_NOPREFIX : 0));

	    /* feed back to caller  */
	    lpSize->cx = myrect.right;
	    lpSize->cy = myrect.bottom;
	}
    }

    TRACE("string size %d x %d!\n", lpSize->cx, lpSize->cy);
}

/***********************************************************************
* 		TOOLBAR_CalcStrings
*
* This function walks through each string and measures it and returns
* the largest height and width to caller.
*/
static void
TOOLBAR_CalcStrings (HWND hwnd, LPSIZE lpSize)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT i;
    SIZE sz;
    HDC hdc;
    HFONT hOldFont;

    lpSize->cx = 0;
    lpSize->cy = 0;

    if (infoPtr->nMaxTextRows == 0)
        return;

    hdc = GetDC (hwnd);
    hOldFont = SelectObject (hdc, infoPtr->hFont);

    if (infoPtr->nNumButtons == 0 && infoPtr->nNumStrings > 0)
    {
        TEXTMETRICW tm;

        GetTextMetricsW(hdc, &tm);
        lpSize->cy = tm.tmHeight;
    }

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
        if(TOOLBAR_HasText(infoPtr, btnPtr))
        {
            TOOLBAR_MeasureString(infoPtr, btnPtr, hdc, &sz);
            if (sz.cx > lpSize->cx)
                lpSize->cx = sz.cx;
            if (sz.cy > lpSize->cy)
                lpSize->cy = sz.cy;
        }
    }

    SelectObject (hdc, hOldFont);
    ReleaseDC (hwnd, hdc);

    TRACE("max string size %d x %d!\n", lpSize->cx, lpSize->cy);
}

/***********************************************************************
* 		TOOLBAR_WrapToolbar
*
* This function walks through the buttons and separators in the
* toolbar, and sets the TBSTATE_WRAP flag only on those items where
* wrapping should occur based on the width of the toolbar window.
* It does *not* calculate button placement itself.  That task
* takes place in TOOLBAR_CalcToolbar. If the program wants to manage
* the toolbar wrapping on its own, it can use the TBSTYLE_WRAPABLE
* flag, and set the TBSTATE_WRAP flags manually on the appropriate items.
*
* Note: TBSTYLE_WRAPABLE or TBSTYLE_EX_UNDOC1 can be used also to allow
* vertical toolbar lists.
*/

static void
TOOLBAR_WrapToolbar( HWND hwnd, DWORD dwStyle )
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT x, cx, i, j;
    RECT rc;
    BOOL bButtonWrap;

    /* 	When the toolbar window style is not TBSTYLE_WRAPABLE,	*/
    /*	no layout is necessary. Applications may use this style */
    /*	to perform their own layout on the toolbar. 		*/
    if( !(dwStyle & TBSTYLE_WRAPABLE) &&
	!(infoPtr->dwExStyle & TBSTYLE_EX_UNDOC1) )  return;

    btnPtr = infoPtr->buttons;
    x  = infoPtr->nIndent;

    if (GetParent(hwnd))
    {
        /* this can get the parents width, to know how far we can extend
         * this toolbar.  We cannot use its height, as there may be multiple
         * toolbars in a rebar control
         */
        GetClientRect( GetParent(hwnd), &rc );
        infoPtr->nWidth = rc.right - rc.left;
    }
    else
    {
        GetWindowRect( hwnd, &rc );
        infoPtr->nWidth = rc.right - rc.left;
    }

    bButtonWrap = FALSE;

    TRACE("start ButtonWidth=%d, BitmapWidth=%d, nWidth=%d, nIndent=%d\n",
	  infoPtr->nButtonWidth, infoPtr->nBitmapWidth, infoPtr->nWidth,
	  infoPtr->nIndent);

    for (i = 0; i < infoPtr->nNumButtons; i++ )
    {
	btnPtr[i].fsState &= ~TBSTATE_WRAP;

	if (btnPtr[i].fsState & TBSTATE_HIDDEN)
	    continue;

        if (btnPtr[i].cx > 0)
            cx = btnPtr[i].cx;
        /* horizontal separators are treated as buttons for width    */
	else if ((btnPtr[i].fsStyle & BTNS_SEP) &&
            !(infoPtr->dwStyle & CCS_VERT))
            cx = (btnPtr[i].iBitmap > 0) ? btnPtr[i].iBitmap : SEPARATOR_WIDTH;
	else
	    cx = infoPtr->nButtonWidth;

	/* Two or more adjacent separators form a separator group.   */
	/* The first separator in a group should be wrapped to the   */
	/* next row if the previous wrapping is on a button.	     */
	if( bButtonWrap &&
		(btnPtr[i].fsStyle & BTNS_SEP) &&
		(i + 1 < infoPtr->nNumButtons ) &&
		(btnPtr[i + 1].fsStyle & BTNS_SEP) )
	{
	    TRACE("wrap point 1 btn %d style %02x\n", i, btnPtr[i].fsStyle);
	    btnPtr[i].fsState |= TBSTATE_WRAP;
	    x = infoPtr->nIndent;
	    i++;
	    bButtonWrap = FALSE;
	    continue;
	}

	/* The layout makes sure the bitmap is visible, but not the button. */
	/* Test added to also wrap after a button that starts a row but     */
	/* is bigger than the area.  - GA  8/01                             */
	if (( x + cx - (infoPtr->nButtonWidth - infoPtr->nBitmapWidth) / 2
	   > infoPtr->nWidth ) ||
	    ((x == infoPtr->nIndent) && (cx > infoPtr->nWidth)))
	{
	    BOOL bFound = FALSE;

	    /* 	If the current button is a separator and not hidden,  */
	    /*	go to the next until it reaches a non separator.      */
	    /*	Wrap the last separator if it is before a button.     */
	    while( ( ((btnPtr[i].fsStyle & BTNS_SEP) &&
		      !(btnPtr[i].fsStyle & BTNS_DROPDOWN)) ||
		     (btnPtr[i].fsState & TBSTATE_HIDDEN) ) &&
			i < infoPtr->nNumButtons )
	    {
		i++;
		bFound = TRUE;
	    }

	    if( bFound && i < infoPtr->nNumButtons )
	    {
		i--;
		TRACE("wrap point 2 btn %d style %02x, x=%d, cx=%d\n",
		      i, btnPtr[i].fsStyle, x, cx);
		btnPtr[i].fsState |= TBSTATE_WRAP;
		x = infoPtr->nIndent;
		bButtonWrap = FALSE;
		continue;
	    }
	    else if ( i >= infoPtr->nNumButtons)
		break;

	    /* 	If the current button is not a separator, find the last  */
	    /*	separator and wrap it.   				 */
	    for ( j = i - 1; j >= 0  &&  !(btnPtr[j].fsState & TBSTATE_WRAP); j--)
	    {
		if ((btnPtr[j].fsStyle & BTNS_SEP) &&
			!(btnPtr[j].fsState & TBSTATE_HIDDEN))
		{
		    bFound = TRUE;
		    i = j;
		    TRACE("wrap point 3 btn %d style %02x, x=%d, cx=%d\n",
			  i, btnPtr[i].fsStyle, x, cx);
		    x = infoPtr->nIndent;
		    btnPtr[j].fsState |= TBSTATE_WRAP;
		    bButtonWrap = FALSE;
		    break;
		}
	    }

	    /* 	If no separator available for wrapping, wrap one of 	*/
	    /*  non-hidden previous button.  			     	*/
	    if (!bFound)
	    {
		for ( j = i - 1;
			j >= 0 && !(btnPtr[j].fsState & TBSTATE_WRAP); j--)
		{
		    if (btnPtr[j].fsState & TBSTATE_HIDDEN)
			continue;

		    bFound = TRUE;
		    i = j;
		    TRACE("wrap point 4 btn %d style %02x, x=%d, cx=%d\n",
			  i, btnPtr[i].fsStyle, x, cx);
		    x = infoPtr->nIndent;
		    btnPtr[j].fsState |= TBSTATE_WRAP;
		    bButtonWrap = TRUE;
		    break;
		}
	    }

	    /* If all above failed, wrap the current button. */
	    if (!bFound)
	    {
		TRACE("wrap point 5 btn %d style %02x, x=%d, cx=%d\n",
		      i, btnPtr[i].fsStyle, x, cx);
		btnPtr[i].fsState |= TBSTATE_WRAP;
		x = infoPtr->nIndent;
		if (btnPtr[i].fsStyle & BTNS_SEP )
		    bButtonWrap = FALSE;
		else
		    bButtonWrap = TRUE;
	    }
	}
	else {
	    TRACE("wrap point 6 btn %d style %02x, x=%d, cx=%d\n",
		  i, btnPtr[i].fsStyle, x, cx);
	    x += cx;
	}
    }
}


/***********************************************************************
* 		TOOLBAR_MeasureButton
*
* Calculates the width and height required for a button. Used in
* TOOLBAR_CalcToolbar to set the all-button width and height and also for
* the width of buttons that are autosized.
*
* Note that it would have been rather elegant to use one piece of code for
* both the laying out of the toolbar and for controlling where button parts
* are drawn, but the native control has inconsistencies between the two that
* prevent this from being effectively. These inconsistencies can be seen as
* artefacts where parts of the button appear outside of the bounding button
* rectangle.
*
* There are several cases for the calculation of the button dimensions and
* button part positioning:
*
* List
* ====
*
* With Bitmap:
*
* +--------------------------------------------------------+ ^
* |                    ^                     ^             | |
* |                    | pad.cy / 2          | centred     | |
* | pad.cx/2 + cxedge +--------------+     +------------+  | | DEFPAD_CY +
* |<----------------->| nBitmapWidth |     | Text       |  | | max(nBitmapHeight, szText.cy)
* |                   |<------------>|     |            |  | |
* |                   +--------------+     +------------+  | |
* |<-------------------------------------->|               | |
* |  cxedge + iListGap + nBitmapWidth + 2  |<----------->  | |
* |                                           szText.cx    | |
* +--------------------------------------------------------+ -
* <-------------------------------------------------------->
*  2*cxedge + nBitmapWidth + iListGap + szText.cx + pad.cx
*
* Without Bitmap (I_IMAGENONE):
*
* +-----------------------------------+ ^
* |                     ^             | |
* |                     | centred     | | LISTPAD_CY +
* |                   +------------+  | | szText.cy
* |                   | Text       |  | |
* |                   |            |  | |
* |                   +------------+  | |
* |<----------------->|               | |
* |      cxedge       |<----------->  | |
* |                      szText.cx    | |
* +-----------------------------------+ -
* <----------------------------------->
*          szText.cx + pad.cx
*
* Without text:
*
* +--------------------------------------+ ^
* |                       ^              | |
* |                       | padding.cy/2 | | DEFPAD_CY +
* |                     +------------+   | | nBitmapHeight
* |                     | Bitmap     |   | |
* |                     |            |   | |
* |                     +------------+   | |
* |<------------------->|                | |
* | cxedge + iListGap/2 |<----------->   | |
* |                       nBitmapWidth   | |
* +--------------------------------------+ -
* <-------------------------------------->
*     2*cxedge + nBitmapWidth + iListGap
*
* Non-List
* ========
*
* With bitmap:
*
* +-----------------------------------+ ^
* |                     ^             | |
* |                     | pad.cy / 2  | | nBitmapHeight +
* |                     -             | | szText.cy +
* |                   +------------+  | | DEFPAD_CY + 1
* |    centred        |   Bitmap   |  | |
* |<----------------->|            |  | |
* |                   +------------+  | |
* |                         ^         | |
* |                       1 |         | |
* |                         -         | |
* |     centred     +---------------+ | |
* |<--------------->|      Text     | | |
* |                 +---------------+ | |
* +-----------------------------------+ -
* <----------------------------------->
* pad.cx + max(nBitmapWidth, szText.cx)
*
* Without bitmaps (NULL imagelist or ImageList_GetImageCount() = 0):
*
* +---------------------------------------+ ^
* |                     ^                 | |
* |                     | 2 + pad.cy / 2  | |
* |                     -                 | | szText.cy +
* |    centred      +-----------------+   | | pad.cy + 2
* |<--------------->|   Text          |   | |
* |                 +-----------------+   | |
* |                                       | |
* +---------------------------------------+ -
* <--------------------------------------->
*          2*cxedge + pad.cx + szText.cx
*
* Without text:
*   As for with bitmaps, but with szText.cx zero.
*/
static inline SIZE TOOLBAR_MeasureButton(const TOOLBAR_INFO *infoPtr, SIZE sizeString,
                                         BOOL bHasBitmap, BOOL bValidImageList)
{
    SIZE sizeButton;
    if (infoPtr->dwStyle & TBSTYLE_LIST)
    {
        /* set button height from bitmap / text height... */
        sizeButton.cy = max((bHasBitmap ? infoPtr->nBitmapHeight : 0),
            sizeString.cy);

        /* ... add on the necessary padding */
        if (bValidImageList)
        {
            if (bHasBitmap)
                sizeButton.cy += DEFPAD_CY;
            else
                sizeButton.cy += LISTPAD_CY;
        }
        else
            sizeButton.cy += infoPtr->szPadding.cy;

        /* calculate button width */
        sizeButton.cx = 2*GetSystemMetrics(SM_CXEDGE) +
            infoPtr->nBitmapWidth + infoPtr->iListGap;
        if (sizeString.cx > 0)
            sizeButton.cx += sizeString.cx + infoPtr->szPadding.cx;

    }
    else
    {
        if (bHasBitmap)
        {
            sizeButton.cy = infoPtr->nBitmapHeight + DEFPAD_CY;
            if (sizeString.cy > 0)
                sizeButton.cy += 1 + sizeString.cy;
            sizeButton.cx = infoPtr->szPadding.cx +
                max(sizeString.cx, infoPtr->nBitmapWidth);
        }
        else
        {
            sizeButton.cy = sizeString.cy + infoPtr->szPadding.cy +
                NONLIST_NOTEXT_OFFSET;
            sizeButton.cx = infoPtr->szPadding.cx +
                max(2*GetSystemMetrics(SM_CXEDGE) + sizeString.cx, infoPtr->nBitmapWidth);
        }
    }
    return sizeButton;
}


/***********************************************************************
* 		TOOLBAR_CalcToolbar
*
* This function calculates button and separator placement. It first
* calculates the button sizes, gets the toolbar window width and then
* calls TOOLBAR_WrapToolbar to determine which buttons we need to wrap
* on. It assigns a new location to each item and sends this location to
* the tooltip window if appropriate. Finally, it updates the rcBound
* rect and calculates the new required toolbar window height.
*/
static void
TOOLBAR_CalcToolbar (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    SIZE  sizeString, sizeButton;
    BOOL validImageList = FALSE;

    TOOLBAR_CalcStrings (hwnd, &sizeString);

    TOOLBAR_DumpToolbar (infoPtr, __LINE__);

    if (TOOLBAR_IsValidImageList(infoPtr, 0))
        validImageList = TRUE;
    sizeButton = TOOLBAR_MeasureButton(infoPtr, sizeString, TRUE, validImageList);
    infoPtr->nButtonWidth = sizeButton.cx;
    infoPtr->nButtonHeight = sizeButton.cy;
    infoPtr->iTopMargin = default_top_margin(infoPtr);

    if ( infoPtr->cxMin >= 0 && infoPtr->nButtonWidth < infoPtr->cxMin )
        infoPtr->nButtonWidth = infoPtr->cxMin;
    if ( infoPtr->cxMax > 0 && infoPtr->nButtonWidth > infoPtr->cxMax )
        infoPtr->nButtonWidth = infoPtr->cxMax;

    TOOLBAR_LayoutToolbar(hwnd);
}

static void
TOOLBAR_LayoutToolbar(HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    TBUTTON_INFO *btnPtr;
    SIZE sizeButton;
    INT i, nRows, nSepRows;
    INT x, y, cx, cy;
    BOOL bWrap;
    BOOL validImageList = TOOLBAR_IsValidImageList(infoPtr, 0);
    BOOL hasDropDownArrows = TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle);

    TOOLBAR_WrapToolbar(hwnd, infoPtr->dwStyle);

    x  = infoPtr->nIndent;
    y  = infoPtr->iTopMargin;
    cx = infoPtr->nButtonWidth;
    cy = infoPtr->nButtonHeight;

    nRows = nSepRows = 0;

    infoPtr->rcBound.top = y;
    infoPtr->rcBound.left = x;
    infoPtr->rcBound.bottom = y + cy;
    infoPtr->rcBound.right = x;

    btnPtr = infoPtr->buttons;

    TRACE("cy=%d\n", cy);

    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++ )
    {
	bWrap = FALSE;
	if (btnPtr->fsState & TBSTATE_HIDDEN)
	{
	    SetRectEmpty (&btnPtr->rect);
	    continue;
	}

	cy = infoPtr->nButtonHeight;

	if (btnPtr->fsStyle & BTNS_SEP) {
	    if (infoPtr->dwStyle & CCS_VERT) {
                cy = (btnPtr->iBitmap > 0) ? btnPtr->iBitmap : SEPARATOR_WIDTH;
                cx = (btnPtr->cx > 0) ? btnPtr->cx : infoPtr->nWidth;
	    }
	    else
                cx = (btnPtr->cx > 0) ? btnPtr->cx :
                    (btnPtr->iBitmap > 0) ? btnPtr->iBitmap : SEPARATOR_WIDTH;
	}
	else
	{
            if (btnPtr->cx)
              cx = btnPtr->cx;
            else if ((infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || 
                (btnPtr->fsStyle & BTNS_AUTOSIZE))
            {
              SIZE sz;
	      HDC hdc;
	      HFONT hOldFont;

	      hdc = GetDC (hwnd);
	      hOldFont = SelectObject (hdc, infoPtr->hFont);

              TOOLBAR_MeasureString(infoPtr, btnPtr, hdc, &sz);

	      SelectObject (hdc, hOldFont);
	      ReleaseDC (hwnd, hdc);

              sizeButton = TOOLBAR_MeasureButton(infoPtr, sz,
                  TOOLBAR_IsValidBitmapIndex(infoPtr, infoPtr->buttons[i].iBitmap),
                  validImageList);
              cx = sizeButton.cx;
            }
            else
	      cx = infoPtr->nButtonWidth;

            /* if size has been set manually then don't add on extra space
             * for the drop down arrow */
	    if (!btnPtr->cx && hasDropDownArrows && 
                ((btnPtr->fsStyle & BTNS_DROPDOWN) || (btnPtr->fsStyle & BTNS_WHOLEDROPDOWN)))
	      cx += DDARROW_WIDTH;
	}
	if (btnPtr->fsState & TBSTATE_WRAP )
		    bWrap = TRUE;

	SetRect (&btnPtr->rect, x, y, x + cx, y + cy);

	if (infoPtr->rcBound.left > x)
	    infoPtr->rcBound.left = x;
	if (infoPtr->rcBound.right < x + cx)
	    infoPtr->rcBound.right = x + cx;
	if (infoPtr->rcBound.bottom < y + cy)
	    infoPtr->rcBound.bottom = y + cy;

        TOOLBAR_TooltipSetRect(infoPtr, btnPtr);

	/* btnPtr->nRow is zero based. The space between the rows is 	*/
	/* also considered as a row. 					*/
	btnPtr->nRow = nRows + nSepRows;

	TRACE("button %d style=%x, bWrap=%d, nRows=%d, nSepRows=%d, btnrow=%d, (%d,%d)-(%d,%d)\n",
	      i, btnPtr->fsStyle, bWrap, nRows, nSepRows, btnPtr->nRow,
	      x, y, x+cx, y+cy);

	if( bWrap )
	{
	    if ( !(btnPtr->fsStyle & BTNS_SEP) )
	        y += cy;
	    else
	    {
               if ( !(infoPtr->dwStyle & CCS_VERT))
                    y += cy + ( (btnPtr->cx > 0 ) ?
                                btnPtr->cx : SEPARATOR_WIDTH) * 2 /3;
		else
		    y += cy;

		/* nSepRows is used to calculate the extra height following  */
		/* the last row.					     */
		nSepRows++;
	    }
	    x = infoPtr->nIndent;

	    /* Increment row number unless this is the last button    */
	    /* and it has Wrap set.                                   */
	    if (i != infoPtr->nNumButtons-1)
		nRows++;
	}
	else
	    x += cx;
    }

    /* infoPtr->nRows is the number of rows on the toolbar */
    infoPtr->nRows = nRows + nSepRows + 1;

    TRACE("toolbar button width %d\n", infoPtr->nButtonWidth);
}


static INT
TOOLBAR_InternalHitTest (HWND hwnd, const POINT *lpPt)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT i;

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->fsState & TBSTATE_HIDDEN)
	    continue;

	if (btnPtr->fsStyle & BTNS_SEP) {
	    if (PtInRect (&btnPtr->rect, *lpPt)) {
		TRACE(" ON SEPARATOR %d!\n", i);
		return -i;
	    }
	}
	else {
	    if (PtInRect (&btnPtr->rect, *lpPt)) {
		TRACE(" ON BUTTON %d!\n", i);
		return i;
	    }
	}
    }

    TRACE(" NOWHERE!\n");
    return TOOLBAR_NOWHERE;
}


/* worker for TB_ADDBUTTONS and TB_INSERTBUTTON */
static BOOL
TOOLBAR_InternalInsertButtonsT(TOOLBAR_INFO *infoPtr, INT iIndex, UINT nAddButtons, TBBUTTON *lpTbb, BOOL fUnicode)
{
    INT nOldButtons, nNewButtons, iButton;
    BOOL fHasString = FALSE;

    if (iIndex < 0)  /* iIndex can be negative, what means adding at the end */
        iIndex = infoPtr->nNumButtons;

    nOldButtons = infoPtr->nNumButtons;
    nNewButtons = nOldButtons + nAddButtons;

    infoPtr->buttons = ReAlloc(infoPtr->buttons, sizeof(TBUTTON_INFO)*nNewButtons);
    memmove(&infoPtr->buttons[iIndex + nAddButtons], &infoPtr->buttons[iIndex],
            (nOldButtons - iIndex) * sizeof(TBUTTON_INFO));
    infoPtr->nNumButtons += nAddButtons;

    /* insert new buttons data */
    for (iButton = 0; iButton < nAddButtons; iButton++) {
        TBUTTON_INFO *btnPtr = &infoPtr->buttons[iIndex + iButton];

        TOOLBAR_DumpTBButton(lpTbb + iButton, fUnicode);

        ZeroMemory(btnPtr, sizeof(*btnPtr));

        btnPtr->iBitmap   = lpTbb[iButton].iBitmap;
        btnPtr->idCommand = lpTbb[iButton].idCommand;
        btnPtr->fsState   = lpTbb[iButton].fsState;
        btnPtr->fsStyle   = lpTbb[iButton].fsStyle;
        btnPtr->dwData    = lpTbb[iButton].dwData;
        if (btnPtr->fsStyle & BTNS_SEP)
            btnPtr->iString = -1;
        else if(HIWORD(lpTbb[iButton].iString) && lpTbb[iButton].iString != -1)
        {
            if (fUnicode)
                Str_SetPtrW((LPWSTR*)&btnPtr->iString, (LPWSTR)lpTbb[iButton].iString );
            else
                Str_SetPtrAtoW((LPWSTR*)&btnPtr->iString, (LPSTR)lpTbb[iButton].iString);
            fHasString = TRUE;
        }
        else
            btnPtr->iString   = lpTbb[iButton].iString;

        TOOLBAR_TooltipAddTool(infoPtr, btnPtr);
    }

    if (infoPtr->nNumStrings > 0 || fHasString)
        TOOLBAR_CalcToolbar(infoPtr->hwndSelf);
    else
        TOOLBAR_LayoutToolbar(infoPtr->hwndSelf);
    TOOLBAR_AutoSize(infoPtr->hwndSelf);

    TOOLBAR_DumpToolbar(infoPtr, __LINE__);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return TRUE;
}


static INT
TOOLBAR_GetButtonIndex (const TOOLBAR_INFO *infoPtr, INT idCommand, BOOL CommandIsIndex)
{
    TBUTTON_INFO *btnPtr;
    INT i;

    if (CommandIsIndex) {
	TRACE("command is really index command=%d\n", idCommand);
	if (idCommand >= infoPtr->nNumButtons) return -1;
	return idCommand;
    }
    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->idCommand == idCommand) {
	    TRACE("command=%d index=%d\n", idCommand, i);
	    return i;
	}
    }
    TRACE("no index found for command=%d\n", idCommand);
    return -1;
}


static INT
TOOLBAR_GetCheckedGroupButtonIndex (const TOOLBAR_INFO *infoPtr, INT nIndex)
{
    TBUTTON_INFO *btnPtr;
    INT nRunIndex;

    if ((nIndex < 0) || (nIndex > infoPtr->nNumButtons))
	return -1;

    /* check index button */
    btnPtr = &infoPtr->buttons[nIndex];
    if ((btnPtr->fsStyle & BTNS_CHECKGROUP) == BTNS_CHECKGROUP) {
	if (btnPtr->fsState & TBSTATE_CHECKED)
	    return nIndex;
    }

    /* check previous buttons */
    nRunIndex = nIndex - 1;
    while (nRunIndex >= 0) {
	btnPtr = &infoPtr->buttons[nRunIndex];
	if ((btnPtr->fsStyle & BTNS_GROUP) == BTNS_GROUP) {
	    if (btnPtr->fsState & TBSTATE_CHECKED)
		return nRunIndex;
	}
	else
	    break;
	nRunIndex--;
    }

    /* check next buttons */
    nRunIndex = nIndex + 1;
    while (nRunIndex < infoPtr->nNumButtons) {
	btnPtr = &infoPtr->buttons[nRunIndex];
	if ((btnPtr->fsStyle & BTNS_GROUP) == BTNS_GROUP) {
	    if (btnPtr->fsState & TBSTATE_CHECKED)
		return nRunIndex;
	}
	else
	    break;
	nRunIndex++;
    }

    return -1;
}


static VOID
TOOLBAR_RelayEvent (HWND hwndTip, HWND hwndMsg, UINT uMsg,
		    WPARAM wParam, LPARAM lParam)
{
    MSG msg;

    msg.hwnd = hwndMsg;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = (short)LOWORD(GetMessagePos ());
    msg.pt.y = (short)HIWORD(GetMessagePos ());

    SendMessageW (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

static void
TOOLBAR_TooltipAddTool(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *button)
{
    if (infoPtr->hwndToolTip && !(button->fsStyle & BTNS_SEP)) {
        TTTOOLINFOW ti;

        ZeroMemory(&ti, sizeof(TTTOOLINFOW));
        ti.cbSize   = sizeof (TTTOOLINFOW);
        ti.hwnd     = infoPtr->hwndSelf;
        ti.uId      = button->idCommand;
        ti.hinst    = 0;
        ti.lpszText = LPSTR_TEXTCALLBACKW;
        /* ti.lParam = random value from the stack? */

        SendMessageW(infoPtr->hwndToolTip, TTM_ADDTOOLW,
            0, (LPARAM)&ti);
    }
}

static void
TOOLBAR_TooltipDelTool(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *button)
{
    if ((infoPtr->hwndToolTip) && !(button->fsStyle & BTNS_SEP)) {
        TTTOOLINFOW ti;

        ZeroMemory(&ti, sizeof(ti));
        ti.cbSize   = sizeof(ti);
        ti.hwnd     = infoPtr->hwndSelf;
        ti.uId      = button->idCommand;

        SendMessageW(infoPtr->hwndToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }
}

static void TOOLBAR_TooltipSetRect(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *button)
{
    /* Set the toolTip only for non-hidden, non-separator button */
    if (infoPtr->hwndToolTip && !(button->fsStyle & BTNS_SEP))
    {
        TTTOOLINFOW ti;

        ZeroMemory(&ti, sizeof(ti));
        ti.cbSize = sizeof(ti);
        ti.hwnd = infoPtr->hwndSelf;
        ti.uId = button->idCommand;
        ti.rect = button->rect;
        SendMessageW(infoPtr->hwndToolTip, TTM_NEWTOOLRECTW, 0, (LPARAM)&ti);
    }
}

/* Creates the tooltip control */
static void
TOOLBAR_TooltipCreateControl(TOOLBAR_INFO *infoPtr)
{
    int i;
    NMTOOLTIPSCREATED nmttc;

    infoPtr->hwndToolTip = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, WS_POPUP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            infoPtr->hwndSelf, 0, 0, 0);

    if (!infoPtr->hwndToolTip)
        return;

    /* Send NM_TOOLTIPSCREATED notification */
    nmttc.hwndToolTips = infoPtr->hwndToolTip;
    TOOLBAR_SendNotify(&nmttc.hdr, infoPtr, NM_TOOLTIPSCREATED);

    for (i = 0; i < infoPtr->nNumButtons; i++)
    {
        TOOLBAR_TooltipAddTool(infoPtr, &infoPtr->buttons[i]);
        TOOLBAR_TooltipSetRect(infoPtr, &infoPtr->buttons[i]);
    }
}

/* keeps available button list box sorted by button id */
static void TOOLBAR_Cust_InsertAvailButton(HWND hwnd, PCUSTOMBUTTON btnInfoNew)
{
    int i;
    int count;
    PCUSTOMBUTTON btnInfo;
    HWND hwndAvail = GetDlgItem(hwnd, IDC_AVAILBTN_LBOX);

    TRACE("button %s, idCommand %d\n", debugstr_w(btnInfoNew->text), btnInfoNew->btn.idCommand);

    count = SendMessageW(hwndAvail, LB_GETCOUNT, 0, 0);

    /* position 0 is always separator */
    for (i = 1; i < count; i++)
    {
        btnInfo = (PCUSTOMBUTTON)SendMessageW(hwndAvail, LB_GETITEMDATA, i, 0);
        if (btnInfoNew->btn.idCommand < btnInfo->btn.idCommand)
        {
            i = SendMessageW(hwndAvail, LB_INSERTSTRING, i, 0);
            SendMessageW(hwndAvail, LB_SETITEMDATA, i, (LPARAM)btnInfoNew);
            return;
        }
    }
    /* id higher than all others add to end */
    i = SendMessageW(hwndAvail, LB_ADDSTRING, 0, 0);
    SendMessageW(hwndAvail, LB_SETITEMDATA, i, (LPARAM)btnInfoNew);
}

static void TOOLBAR_Cust_MoveButton(const CUSTDLG_INFO *custInfo, HWND hwnd, INT nIndexFrom, INT nIndexTo)
{
    NMTOOLBARW nmtb;

	TRACE("index from %d, index to %d\n", nIndexFrom, nIndexTo);

    if (nIndexFrom == nIndexTo)
        return;

    /* MSDN states that iItem is the index of the button, rather than the
     * command ID as used by every other NMTOOLBAR notification */
    nmtb.iItem = nIndexFrom;
    if (TOOLBAR_SendNotify(&nmtb.hdr, custInfo->tbInfo, TBN_QUERYINSERT))
    {
        PCUSTOMBUTTON btnInfo;
        NMHDR hdr;
        HWND hwndList = GetDlgItem(hwnd, IDC_TOOLBARBTN_LBOX);
        int count = SendMessageW(hwndList, LB_GETCOUNT, 0, 0);

        btnInfo = (PCUSTOMBUTTON)SendMessageW(hwndList, LB_GETITEMDATA, nIndexFrom, 0);

        SendMessageW(hwndList, LB_DELETESTRING, nIndexFrom, 0);
        SendMessageW(hwndList, LB_INSERTSTRING, nIndexTo, 0);
        SendMessageW(hwndList, LB_SETITEMDATA, nIndexTo, (LPARAM)btnInfo);
        SendMessageW(hwndList, LB_SETCURSEL, nIndexTo, 0);

        if (nIndexTo <= 0)
            EnableWindow(GetDlgItem(hwnd,IDC_MOVEUP_BTN), FALSE);
        else
            EnableWindow(GetDlgItem(hwnd,IDC_MOVEUP_BTN), TRUE);

        /* last item is always separator, so -2 instead of -1 */
        if (nIndexTo >= (count - 2))
            EnableWindow(GetDlgItem(hwnd,IDC_MOVEDN_BTN), FALSE);
        else
            EnableWindow(GetDlgItem(hwnd,IDC_MOVEDN_BTN), TRUE);

        SendMessageW(custInfo->tbHwnd, TB_DELETEBUTTON, nIndexFrom, 0);
        SendMessageW(custInfo->tbHwnd, TB_INSERTBUTTONW, nIndexTo, (LPARAM)&(btnInfo->btn));

        TOOLBAR_SendNotify(&hdr, custInfo->tbInfo, TBN_TOOLBARCHANGE);
    }
}

static void TOOLBAR_Cust_AddButton(const CUSTDLG_INFO *custInfo, HWND hwnd, INT nIndexAvail, INT nIndexTo)
{
    NMTOOLBARW nmtb;

    TRACE("Add: nIndexAvail %d, nIndexTo %d\n", nIndexAvail, nIndexTo);

    /* MSDN states that iItem is the index of the button, rather than the
     * command ID as used by every other NMTOOLBAR notification */
    nmtb.iItem = nIndexAvail;
    if (TOOLBAR_SendNotify(&nmtb.hdr, custInfo->tbInfo, TBN_QUERYINSERT))
    {
        PCUSTOMBUTTON btnInfo;
        NMHDR hdr;
        HWND hwndList = GetDlgItem(hwnd, IDC_TOOLBARBTN_LBOX);
        HWND hwndAvail = GetDlgItem(hwnd, IDC_AVAILBTN_LBOX);
        int count = SendMessageW(hwndAvail, LB_GETCOUNT, 0, 0);

        btnInfo = (PCUSTOMBUTTON)SendMessageW(hwndAvail, LB_GETITEMDATA, nIndexAvail, 0);

        if (nIndexAvail != 0) /* index == 0 indicates separator */
        {
            /* remove from 'available buttons' list */
            SendMessageW(hwndAvail, LB_DELETESTRING, nIndexAvail, 0);
            if (nIndexAvail == count-1)
                SendMessageW(hwndAvail, LB_SETCURSEL, nIndexAvail-1 , 0);
            else
                SendMessageW(hwndAvail, LB_SETCURSEL, nIndexAvail , 0);
        }
        else
        {
            PCUSTOMBUTTON btnNew;

            /* duplicate 'separator' button */
            btnNew = Alloc(sizeof(CUSTOMBUTTON));
            *btnNew = *btnInfo;
            btnInfo = btnNew;
        }

        /* insert into 'toolbar button' list */
        SendMessageW(hwndList, LB_INSERTSTRING, nIndexTo, 0);
        SendMessageW(hwndList, LB_SETITEMDATA, nIndexTo, (LPARAM)btnInfo);

        SendMessageW(custInfo->tbHwnd, TB_INSERTBUTTONW, nIndexTo, (LPARAM)&(btnInfo->btn));

        TOOLBAR_SendNotify(&hdr, custInfo->tbInfo, TBN_TOOLBARCHANGE);
    }
}

static void TOOLBAR_Cust_RemoveButton(const CUSTDLG_INFO *custInfo, HWND hwnd, INT index)
{
    PCUSTOMBUTTON btnInfo;
    HWND hwndList = GetDlgItem(hwnd, IDC_TOOLBARBTN_LBOX);

    TRACE("Remove: index %d\n", index);

    btnInfo = (PCUSTOMBUTTON)SendMessageW(hwndList, LB_GETITEMDATA, index, 0);

    /* send TBN_QUERYDELETE notification */
    if (TOOLBAR_IsButtonRemovable(custInfo->tbInfo, index, btnInfo))
    {
        NMHDR hdr;

        SendMessageW(hwndList, LB_DELETESTRING, index, 0);
        SendMessageW(hwndList, LB_SETCURSEL, index , 0);

        SendMessageW(custInfo->tbHwnd, TB_DELETEBUTTON, index, 0);

        /* insert into 'available button' list */
        if (!(btnInfo->btn.fsStyle & BTNS_SEP))
            TOOLBAR_Cust_InsertAvailButton(hwnd, btnInfo);
        else
            Free(btnInfo);

        TOOLBAR_SendNotify(&hdr, custInfo->tbInfo, TBN_TOOLBARCHANGE);
    }
}

/* drag list notification function for toolbar buttons list box */
static LRESULT TOOLBAR_Cust_ToolbarDragListNotification(const CUSTDLG_INFO *custInfo, HWND hwnd,
                                                        const DRAGLISTINFO *pDLI)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_TOOLBARBTN_LBOX);
    switch (pDLI->uNotification)
    {
    case DL_BEGINDRAG:
    {
        INT nCurrentItem = LBItemFromPt(hwndList, pDLI->ptCursor, TRUE);
        INT nCount = SendMessageW(hwndList, LB_GETCOUNT, 0, 0);
        /* no dragging for last item (separator) */
        if (nCurrentItem >= (nCount - 1)) return FALSE;
        return TRUE;
    }
    case DL_DRAGGING:
    {
        INT nCurrentItem = LBItemFromPt(hwndList, pDLI->ptCursor, TRUE);
        INT nCount = SendMessageW(hwndList, LB_GETCOUNT, 0, 0);
        /* no dragging past last item (separator) */
        if ((nCurrentItem >= 0) && (nCurrentItem < (nCount - 1)))
        {
            DrawInsert(hwnd, hwndList, nCurrentItem);
            /* FIXME: native uses "move button" cursor */
            return DL_COPYCURSOR;
        }

        /* not over toolbar buttons list */
        if (nCurrentItem < 0)
        {
            POINT ptWindow = pDLI->ptCursor;
            HWND hwndListAvail = GetDlgItem(hwnd, IDC_AVAILBTN_LBOX);
            MapWindowPoints(NULL, hwnd, &ptWindow, 1);
            /* over available buttons list? */
            if (ChildWindowFromPoint(hwnd, ptWindow) == hwndListAvail)
                /* FIXME: native uses "move button" cursor */
                return DL_COPYCURSOR;
        }
        /* clear drag arrow */
        DrawInsert(hwnd, hwndList, -1);
        return DL_STOPCURSOR;
    }
    case DL_DROPPED:
    {
        INT nIndexTo = LBItemFromPt(hwndList, pDLI->ptCursor, TRUE);
        INT nIndexFrom = SendMessageW(hwndList, LB_GETCURSEL, 0, 0);
        INT nCount = SendMessageW(hwndList, LB_GETCOUNT, 0, 0);
        if ((nIndexTo >= 0) && (nIndexTo < (nCount - 1)))
        {
            /* clear drag arrow */
            DrawInsert(hwnd, hwndList, -1);
            /* move item */
            TOOLBAR_Cust_MoveButton(custInfo, hwnd, nIndexFrom, nIndexTo);
        }
        /* not over toolbar buttons list */
        if (nIndexTo < 0)
        {
            POINT ptWindow = pDLI->ptCursor;
            HWND hwndListAvail = GetDlgItem(hwnd, IDC_AVAILBTN_LBOX);
            MapWindowPoints(NULL, hwnd, &ptWindow, 1);
            /* over available buttons list? */
            if (ChildWindowFromPoint(hwnd, ptWindow) == hwndListAvail)
                TOOLBAR_Cust_RemoveButton(custInfo, hwnd, nIndexFrom);
        }
        break;
    }
    case DL_CANCELDRAG:
        /* Clear drag arrow */
        DrawInsert(hwnd, hwndList, -1);
        break;
    }

    return 0;
}

/* drag list notification function for available buttons list box */
static LRESULT TOOLBAR_Cust_AvailDragListNotification(const CUSTDLG_INFO *custInfo, HWND hwnd,
                                                      const DRAGLISTINFO *pDLI)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_TOOLBARBTN_LBOX);
    switch (pDLI->uNotification)
    {
    case DL_BEGINDRAG:
        return TRUE;
    case DL_DRAGGING:
    {
        INT nCurrentItem = LBItemFromPt(hwndList, pDLI->ptCursor, TRUE);
        INT nCount = SendMessageW(hwndList, LB_GETCOUNT, 0, 0);
        /* no dragging past last item (separator) */
        if ((nCurrentItem >= 0) && (nCurrentItem < nCount))
        {
            DrawInsert(hwnd, hwndList, nCurrentItem);
            /* FIXME: native uses "move button" cursor */
            return DL_COPYCURSOR;
        }

        /* not over toolbar buttons list */
        if (nCurrentItem < 0)
        {
            POINT ptWindow = pDLI->ptCursor;
            HWND hwndListAvail = GetDlgItem(hwnd, IDC_AVAILBTN_LBOX);
            MapWindowPoints(NULL, hwnd, &ptWindow, 1);
            /* over available buttons list? */
            if (ChildWindowFromPoint(hwnd, ptWindow) == hwndListAvail)
                /* FIXME: native uses "move button" cursor */
                return DL_COPYCURSOR;
        }
        /* clear drag arrow */
        DrawInsert(hwnd, hwndList, -1);
        return DL_STOPCURSOR;
    }
    case DL_DROPPED:
    {
        INT nIndexTo = LBItemFromPt(hwndList, pDLI->ptCursor, TRUE);
        INT nCount = SendMessageW(hwndList, LB_GETCOUNT, 0, 0);
        INT nIndexFrom = SendDlgItemMessageW(hwnd, IDC_AVAILBTN_LBOX, LB_GETCURSEL, 0, 0);
        if ((nIndexTo >= 0) && (nIndexTo < nCount))
        {
            /* clear drag arrow */
            DrawInsert(hwnd, hwndList, -1);
            /* add item */
            TOOLBAR_Cust_AddButton(custInfo, hwnd, nIndexFrom, nIndexTo);
        }
    }
    case DL_CANCELDRAG:
        /* Clear drag arrow */
        DrawInsert(hwnd, hwndList, -1);
        break;
    }
    return 0;
}

extern UINT uDragListMessage;

/***********************************************************************
 * TOOLBAR_CustomizeDialogProc
 * This function implements the toolbar customization dialog.
 */
static INT_PTR CALLBACK
TOOLBAR_CustomizeDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PCUSTDLG_INFO custInfo = (PCUSTDLG_INFO)GetWindowLongPtrW (hwnd, DWLP_USER);
    PCUSTOMBUTTON btnInfo;
    NMTOOLBARA nmtb;
    TOOLBAR_INFO *infoPtr = custInfo ? custInfo->tbInfo : NULL;

    switch (uMsg)
    {
	case WM_INITDIALOG:
	    custInfo = (PCUSTDLG_INFO)lParam;
	    SetWindowLongPtrW (hwnd, DWLP_USER, (LONG_PTR)custInfo);

	    if (custInfo)
	    {
		WCHAR Buffer[256];
		int i = 0;
		int index;
		NMTBINITCUSTOMIZE nmtbic;

		infoPtr = custInfo->tbInfo;

		/* send TBN_QUERYINSERT notification */
		nmtb.iItem = custInfo->tbInfo->nNumButtons;

		if (!TOOLBAR_SendNotify(&nmtb.hdr, infoPtr, TBN_QUERYINSERT))
		    return FALSE;

		nmtbic.hwndDialog = hwnd;
		/* Send TBN_INITCUSTOMIZE notification */
		if (TOOLBAR_SendNotify (&nmtbic.hdr, infoPtr, TBN_INITCUSTOMIZE) ==
		    TBNRF_HIDEHELP)
                {
                    TRACE("TBNRF_HIDEHELP requested\n");
                    ShowWindow(GetDlgItem(hwnd, IDC_HELP_BTN), SW_HIDE);
                }

		/* add items to 'toolbar buttons' list and check if removable */
		for (i = 0; i < custInfo->tbInfo->nNumButtons; i++)
                {
                    btnInfo = Alloc(sizeof(CUSTOMBUTTON));
                    memset (&btnInfo->btn, 0, sizeof(TBBUTTON));
                    btnInfo->btn.fsStyle = BTNS_SEP;
                    btnInfo->bVirtual = FALSE;
		    LoadStringW (COMCTL32_hModule, IDS_SEPARATOR, btnInfo->text, 64);

		    /* send TBN_QUERYDELETE notification */
                    btnInfo->bRemovable = TOOLBAR_IsButtonRemovable(infoPtr, i, btnInfo);

		    index = (int)SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_ADDSTRING, 0, 0);
		    SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);
		}

		SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMHEIGHT, 0, infoPtr->nBitmapHeight + 8);

		/* insert separator button into 'available buttons' list */
                btnInfo = Alloc(sizeof(CUSTOMBUTTON));
		memset (&btnInfo->btn, 0, sizeof(TBBUTTON));
		btnInfo->btn.fsStyle = BTNS_SEP;
		btnInfo->bVirtual = FALSE;
		btnInfo->bRemovable = TRUE;
		LoadStringW (COMCTL32_hModule, IDS_SEPARATOR, btnInfo->text, 64);
		index = (int)SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_ADDSTRING, 0, (LPARAM)btnInfo);
		SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);

		/* insert all buttons into dsa */
		for (i = 0;; i++)
		{
		    /* send TBN_GETBUTTONINFO notification */
                    NMTOOLBARW nmtb;
		    nmtb.iItem = i;
		    nmtb.pszText = Buffer;
		    nmtb.cchText = 256;

                    /* Clear previous button's text */
                    ZeroMemory(nmtb.pszText, nmtb.cchText * sizeof(WCHAR));

                    if (!TOOLBAR_GetButtonInfo(infoPtr, &nmtb))
			break;

		    TRACE("WM_INITDIALOG style: %x iItem(%d) idCommand(%d) iString(%ld) %s\n",
                        nmtb.tbButton.fsStyle, i, 
                        nmtb.tbButton.idCommand,
                        nmtb.tbButton.iString,
                        nmtb.tbButton.iString >= 0 ? debugstr_w(infoPtr->strings[nmtb.tbButton.iString])
                        : "");

		    /* insert button into the apropriate list */
		    index = TOOLBAR_GetButtonIndex (custInfo->tbInfo, nmtb.tbButton.idCommand, FALSE);
		    if (index == -1)
		    {
                        btnInfo = Alloc(sizeof(CUSTOMBUTTON));
			btnInfo->bVirtual = FALSE;
			btnInfo->bRemovable = TRUE;
		    }
		    else
		    {
                        btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageW (hwnd, 
                            IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, index, 0);
                    }

                    btnInfo->btn = nmtb.tbButton;
                    if (!(nmtb.tbButton.fsStyle & BTNS_SEP))
                    {
                        if (lstrlenW(nmtb.pszText))
                            lstrcpyW(btnInfo->text, nmtb.pszText);
                        else if (nmtb.tbButton.iString >= 0 && 
                            nmtb.tbButton.iString < infoPtr->nNumStrings)
                        {
                            lstrcpyW(btnInfo->text, 
                                infoPtr->strings[nmtb.tbButton.iString]);
                        }
		    }

		    if (index == -1)
			TOOLBAR_Cust_InsertAvailButton(hwnd, btnInfo);
		}

		SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_SETITEMHEIGHT, 0, infoPtr->nBitmapHeight + 8);

		/* select first item in the 'available' list */
		SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_SETCURSEL, 0, 0);

		/* append 'virtual' separator button to the 'toolbar buttons' list */
                btnInfo = Alloc(sizeof(CUSTOMBUTTON));
		memset (&btnInfo->btn, 0, sizeof(TBBUTTON));
		btnInfo->btn.fsStyle = BTNS_SEP;
		btnInfo->bVirtual = TRUE;
		btnInfo->bRemovable = FALSE;
		LoadStringW (COMCTL32_hModule, IDS_SEPARATOR, btnInfo->text, 64);
		index = (int)SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_ADDSTRING, 0, (LPARAM)btnInfo);
		SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);

		/* select last item in the 'toolbar' list */
		SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETCURSEL, index, 0);
		SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETTOPINDEX, index, 0);

		MakeDragList(GetDlgItem(hwnd, IDC_TOOLBARBTN_LBOX));
		MakeDragList(GetDlgItem(hwnd, IDC_AVAILBTN_LBOX));

		/* set focus and disable buttons */
		PostMessageW (hwnd, WM_USER, 0, 0);
	    }
	    return TRUE;

	case WM_USER:
	    EnableWindow (GetDlgItem (hwnd,IDC_MOVEUP_BTN), FALSE);
	    EnableWindow (GetDlgItem (hwnd,IDC_MOVEDN_BTN), FALSE);
	    EnableWindow (GetDlgItem (hwnd,IDC_REMOVE_BTN), FALSE);
	    SetFocus (GetDlgItem (hwnd, IDC_TOOLBARBTN_LBOX));
	    return TRUE;

	case WM_CLOSE:
	    EndDialog(hwnd, FALSE);
	    return TRUE;

	case WM_COMMAND:
	    switch (LOWORD(wParam))
	    {
		case IDC_TOOLBARBTN_LBOX:
		    if (HIWORD(wParam) == LBN_SELCHANGE)
		    {
			PCUSTOMBUTTON btnInfo;
			NMTOOLBARA nmtb;
			int count;
			int index;

			count = SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCOUNT, 0, 0);
			index = SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);

			/* send TBN_QUERYINSERT notification */
			nmtb.iItem = index;
		        TOOLBAR_SendNotify(&nmtb.hdr, infoPtr, TBN_QUERYINSERT);

			/* get list box item */
			btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, index, 0);

			if (index == (count - 1))
			{
			    /* last item (virtual separator) */
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEUP_BTN), FALSE);
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEDN_BTN), FALSE);
			}
			else if (index == (count - 2))
			{
			    /* second last item (last non-virtual item) */
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEUP_BTN), TRUE);
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEDN_BTN), FALSE);
			}
			else if (index == 0)
			{
			    /* first item */
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEUP_BTN), FALSE);
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEDN_BTN), TRUE);
			}
			else
			{
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEUP_BTN), TRUE);
			    EnableWindow (GetDlgItem (hwnd,IDC_MOVEDN_BTN), TRUE);
			}

			EnableWindow (GetDlgItem (hwnd,IDC_REMOVE_BTN), btnInfo->bRemovable);
		    }
		    break;

		case IDC_MOVEUP_BTN:
		    {
			int index = SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);
			TOOLBAR_Cust_MoveButton(custInfo, hwnd, index, index-1);
		    }
		    break;

		case IDC_MOVEDN_BTN: /* move down */
		    {
			int index = SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);
			TOOLBAR_Cust_MoveButton(custInfo, hwnd, index, index+1);
		    }
		    break;

		case IDC_REMOVE_BTN: /* remove button */
		    {
			int index = SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);

			if (LB_ERR == index)
				break;

			TOOLBAR_Cust_RemoveButton(custInfo, hwnd, index);
		    }
		    break;
		case IDC_HELP_BTN:
			TOOLBAR_SendNotify(&nmtb.hdr, infoPtr, TBN_CUSTHELP);
			break;
		case IDC_RESET_BTN:
			TOOLBAR_SendNotify(&nmtb.hdr, infoPtr, TBN_RESET);
			break;

		case IDOK: /* Add button */
		    {
			int index;
			int indexto;

			index = SendDlgItemMessageW(hwnd, IDC_AVAILBTN_LBOX, LB_GETCURSEL, 0, 0);
			indexto = SendDlgItemMessageW(hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);

			TOOLBAR_Cust_AddButton(custInfo, hwnd, index, indexto);
		    }
		    break;

		case IDCANCEL:
		    EndDialog(hwnd, FALSE);
		    break;
	    }
	    return TRUE;

	case WM_DESTROY:
	    {
		int count;
		int i;

		/* delete items from 'toolbar buttons' listbox*/
		count = SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCOUNT, 0, 0);
		for (i = 0; i < count; i++)
		{
		    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, i, 0);
		    Free(btnInfo);
		    SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, 0, 0);
		}
		SendDlgItemMessageW (hwnd, IDC_TOOLBARBTN_LBOX, LB_RESETCONTENT, 0, 0);


		/* delete items from 'available buttons' listbox*/
		count = SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_GETCOUNT, 0, 0);
		for (i = 0; i < count; i++)
		{
		    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_GETITEMDATA, i, 0);
		    Free(btnInfo);
		    SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_SETITEMDATA, i, 0);
		}
		SendDlgItemMessageW (hwnd, IDC_AVAILBTN_LBOX, LB_RESETCONTENT, 0, 0);
            }
	    return TRUE;

	case WM_DRAWITEM:
	    if (wParam == IDC_AVAILBTN_LBOX || wParam == IDC_TOOLBARBTN_LBOX)
	    {
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		RECT rcButton;
		RECT rcText;
		HPEN hPen, hOldPen;
		HBRUSH hOldBrush;
		COLORREF oldText = 0;
		COLORREF oldBk = 0;

		/* get item data */
		btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageW (hwnd, wParam, LB_GETITEMDATA, (WPARAM)lpdis->itemID, 0);
		if (btnInfo == NULL)
		{
		    FIXME("btnInfo invalid!\n");
		    return TRUE;
		}

		/* set colors and select objects */
		oldBk = SetBkColor (lpdis->hDC, (lpdis->itemState & ODS_FOCUS)?comctl32_color.clrHighlight:comctl32_color.clrWindow);
		if (btnInfo->bVirtual)
		   oldText = SetTextColor (lpdis->hDC, comctl32_color.clrGrayText);
		else
		   oldText = SetTextColor (lpdis->hDC, (lpdis->itemState & ODS_FOCUS)?comctl32_color.clrHighlightText:comctl32_color.clrWindowText);
                hPen = CreatePen( PS_SOLID, 1,
                     GetSysColor( (lpdis->itemState & ODS_SELECTED)?COLOR_HIGHLIGHT:COLOR_WINDOW));
		hOldPen = SelectObject (lpdis->hDC, hPen );
		hOldBrush = SelectObject (lpdis->hDC, GetSysColorBrush ((lpdis->itemState & ODS_FOCUS)?COLOR_HIGHLIGHT:COLOR_WINDOW));

		/* fill background rectangle */
		Rectangle (lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
			   lpdis->rcItem.right, lpdis->rcItem.bottom);

		/* calculate button and text rectangles */
		CopyRect (&rcButton, &lpdis->rcItem);
		InflateRect (&rcButton, -1, -1);
		CopyRect (&rcText, &rcButton);
		rcButton.right = rcButton.left + custInfo->tbInfo->nBitmapWidth + 6;
		rcText.left = rcButton.right + 2;

		/* draw focus rectangle */
		if (lpdis->itemState & ODS_FOCUS)
		    DrawFocusRect (lpdis->hDC, &lpdis->rcItem);

		/* draw button */
		if (!(infoPtr->dwStyle & TBSTYLE_FLAT))
		    DrawEdge (lpdis->hDC, &rcButton, EDGE_RAISED, BF_RECT|BF_MIDDLE|BF_SOFT);

		/* draw image and text */
		if ((btnInfo->btn.fsStyle & BTNS_SEP) == 0) {
			HIMAGELIST himl = GETDEFIMAGELIST(infoPtr, GETHIMLID(infoPtr, 
				btnInfo->btn.iBitmap));
		    ImageList_Draw (himl, GETIBITMAP(infoPtr, btnInfo->btn.iBitmap), 
				lpdis->hDC, rcButton.left+3, rcButton.top+3, ILD_NORMAL);
		}
		DrawTextW (lpdis->hDC,  btnInfo->text, -1, &rcText,
			       DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		/* delete objects and reset colors */
		SelectObject (lpdis->hDC, hOldBrush);
		SelectObject (lpdis->hDC, hOldPen);
		SetBkColor (lpdis->hDC, oldBk);
		SetTextColor (lpdis->hDC, oldText);
                DeleteObject( hPen );
		return TRUE;
	    }
	    return FALSE;

	case WM_MEASUREITEM:
	    if (wParam == IDC_AVAILBTN_LBOX || wParam == IDC_TOOLBARBTN_LBOX)
	    {
		MEASUREITEMSTRUCT *lpmis = (MEASUREITEMSTRUCT*)lParam;

		lpmis->itemHeight = 15 + 8; /* default height */

		return TRUE;
	    }
	    return FALSE;

	default:
            if (uDragListMessage && (uMsg == uDragListMessage))
            {
                if (wParam == IDC_TOOLBARBTN_LBOX)
                {
                    LRESULT res = TOOLBAR_Cust_ToolbarDragListNotification(
                        custInfo, hwnd, (DRAGLISTINFO *)lParam);
                    SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, res);
                    return TRUE;
                }
                else if (wParam == IDC_AVAILBTN_LBOX)
                {
                    LRESULT res = TOOLBAR_Cust_AvailDragListNotification(
                        custInfo, hwnd, (DRAGLISTINFO *)lParam);
                    SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, res);
                    return TRUE;
                }
            }
            return FALSE;
    }
}

static BOOL
TOOLBAR_AddBitmapToImageList(TOOLBAR_INFO *infoPtr, HIMAGELIST himlDef, const TBITMAP_INFO *bitmap)
{
    HBITMAP hbmLoad;
    INT nCountBefore = ImageList_GetImageCount(himlDef);
    INT nCountAfter;
    INT cxIcon, cyIcon;
    INT nAdded;
    INT nIndex;

    TRACE("adding hInst=%p nID=%d nButtons=%d\n", bitmap->hInst, bitmap->nID, bitmap->nButtons);
    /* Add bitmaps to the default image list */
    if (bitmap->hInst == NULL)         /* a handle was passed */
        hbmLoad = CopyImage(ULongToHandle(bitmap->nID), IMAGE_BITMAP, 0, 0, 0);
    else
        hbmLoad = CreateMappedBitmap(bitmap->hInst, bitmap->nID, 0, NULL, 0);

    /* enlarge the bitmap if needed */
    ImageList_GetIconSize(himlDef, &cxIcon, &cyIcon);
    if (bitmap->hInst != COMCTL32_hModule)
        COMCTL32_EnsureBitmapSize(&hbmLoad, cxIcon*(INT)bitmap->nButtons, cyIcon, comctl32_color.clrBtnFace);
    
    nIndex = ImageList_AddMasked(himlDef, hbmLoad, comctl32_color.clrBtnFace);
    DeleteObject(hbmLoad);
    if (nIndex == -1)
        return FALSE;
    
    nCountAfter = ImageList_GetImageCount(himlDef);
    nAdded =  nCountAfter - nCountBefore;
    if (bitmap->nButtons == 0) /* wParam == 0 is special and means add only one image */
    {
        ImageList_SetImageCount(himlDef, nCountBefore + 1);
    } else if (nAdded > (INT)bitmap->nButtons) {
        TRACE("Added more images than wParam: Previous image number %i added %i while wParam %i. Images in list %i\n",
            nCountBefore, nAdded, bitmap->nButtons, nCountAfter);
    }

    infoPtr->nNumBitmaps += nAdded;
    return TRUE;
}

static void
TOOLBAR_CheckImageListIconSize(TOOLBAR_INFO *infoPtr)
{
    HIMAGELIST himlDef;
    HIMAGELIST himlNew;
    INT cx, cy;
    INT i;
    
    himlDef = GETDEFIMAGELIST(infoPtr, 0);
    if (himlDef == NULL || himlDef != infoPtr->himlInt)
        return;
    if (!ImageList_GetIconSize(himlDef, &cx, &cy))
        return;
    if (cx == infoPtr->nBitmapWidth && cy == infoPtr->nBitmapHeight)
        return;

    TRACE("Update icon size: %dx%d -> %dx%d\n",
        cx, cy, infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);

    himlNew = ImageList_Create(infoPtr->nBitmapWidth, infoPtr->nBitmapHeight,
                                ILC_COLORDDB|ILC_MASK, 8, 2);
    for (i = 0; i < infoPtr->nNumBitmapInfos; i++)
        TOOLBAR_AddBitmapToImageList(infoPtr, himlNew, &infoPtr->bitmaps[i]);
    TOOLBAR_InsertImageList(&infoPtr->himlDef, &infoPtr->cimlDef, himlNew, 0);
    infoPtr->himlInt = himlNew;

    infoPtr->nNumBitmaps -= ImageList_GetImageCount(himlDef);
    ImageList_Destroy(himlDef);
}

/***********************************************************************
 * TOOLBAR_AddBitmap:  Add the bitmaps to the default image list.
 *
 */
static LRESULT
TOOLBAR_AddBitmap (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBADDBITMAP lpAddBmp = (LPTBADDBITMAP)lParam;
    TBITMAP_INFO info;
    INT iSumButtons, i;
    HIMAGELIST himlDef;

    TRACE("hwnd=%p wParam=%lx lParam=%lx\n", hwnd, wParam, lParam);
    if (!lpAddBmp)
	return -1;

    if (lpAddBmp->hInst == HINST_COMMCTRL)
    {
        info.hInst = COMCTL32_hModule;
        switch (lpAddBmp->nID)
        {
            case IDB_STD_SMALL_COLOR:
	        info.nButtons = 15;
	        info.nID = IDB_STD_SMALL;
	        break;
            case IDB_STD_LARGE_COLOR:
	        info.nButtons = 15;
	        info.nID = IDB_STD_LARGE;
	        break;
            case IDB_VIEW_SMALL_COLOR:
	        info.nButtons = 12;
	        info.nID = IDB_VIEW_SMALL;
	        break;
            case IDB_VIEW_LARGE_COLOR:
	        info.nButtons = 12;
	        info.nID = IDB_VIEW_LARGE;
	        break;
            case IDB_HIST_SMALL_COLOR:
	        info.nButtons = 5;
	        info.nID = IDB_HIST_SMALL;
	        break;
            case IDB_HIST_LARGE_COLOR:
	        info.nButtons = 5;
	        info.nID = IDB_HIST_LARGE;
	        break;
	    default:
	        return -1;
	}

	TRACE ("adding %d internal bitmaps!\n", info.nButtons);

	/* Windows resize all the buttons to the size of a newly added standard image */
	if (lpAddBmp->nID & 1)
	{
	    /* large icons: 24x24. Will make the button 31x30 */
	    SendMessageW (hwnd, TB_SETBITMAPSIZE, 0, MAKELPARAM(24, 24));
	}
	else
	{
	    /* small icons: 16x16. Will make the buttons 23x22 */
	    SendMessageW (hwnd, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 16));
	}

	TOOLBAR_CalcToolbar (hwnd);
    }
    else
    {
	info.nButtons = (INT)wParam;
	info.hInst = lpAddBmp->hInst;
	info.nID = lpAddBmp->nID;
	TRACE("adding %d bitmaps!\n", info.nButtons);
    }
    
    /* check if the bitmap is already loaded and compute iSumButtons */
    iSumButtons = 0;
    for (i = 0; i < infoPtr->nNumBitmapInfos; i++)
    {
        if (infoPtr->bitmaps[i].hInst == info.hInst &&
            infoPtr->bitmaps[i].nID == info.nID)
            return iSumButtons;
        iSumButtons += infoPtr->bitmaps[i].nButtons;
    }

    if (!infoPtr->cimlDef) {
	/* create new default image list */
	TRACE ("creating default image list!\n");

        himlDef = ImageList_Create (infoPtr->nBitmapWidth, infoPtr->nBitmapHeight,
                                    ILC_COLORDDB | ILC_MASK, info.nButtons, 2);
	TOOLBAR_InsertImageList(&infoPtr->himlDef, &infoPtr->cimlDef, himlDef, 0);
        infoPtr->himlInt = himlDef;
    }
    else {
        himlDef = GETDEFIMAGELIST(infoPtr, 0);
    }

    if (!himlDef) {
        WARN("No default image list available\n");
        return -1;
    }

    if (!TOOLBAR_AddBitmapToImageList(infoPtr, himlDef, &info))
        return -1;

    TRACE("Number of bitmap infos: %d\n", infoPtr->nNumBitmapInfos);
    infoPtr->bitmaps = ReAlloc(infoPtr->bitmaps, (infoPtr->nNumBitmapInfos + 1) * sizeof(TBITMAP_INFO));
    infoPtr->bitmaps[infoPtr->nNumBitmapInfos] = info;
    infoPtr->nNumBitmapInfos++;
    TRACE("Number of bitmap infos: %d\n", infoPtr->nNumBitmapInfos);

    InvalidateRect(hwnd, NULL, TRUE);
    return iSumButtons;
}


static LRESULT
TOOLBAR_AddButtonsT(HWND hwnd, WPARAM wParam, LPARAM lParam, BOOL fUnicode)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT nAddButtons = (UINT)wParam;

    TRACE("adding %ld buttons (unicode=%d)!\n", wParam, fUnicode);

    return TOOLBAR_InternalInsertButtonsT(infoPtr, -1, nAddButtons, lpTbb, fUnicode);
}


static LRESULT
TOOLBAR_AddStringW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#define MAX_RESOURCE_STRING_LENGTH 512
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    BOOL fFirstString = (infoPtr->nNumStrings == 0);
    INT nIndex = infoPtr->nNumStrings;

    if ((wParam) && (HIWORD(lParam) == 0)) {
	WCHAR szString[MAX_RESOURCE_STRING_LENGTH];
	WCHAR delimiter;
	WCHAR *next_delim;
	WCHAR *p;
	INT len;
	TRACE("adding string from resource!\n");

        len = LoadStringW ((HINSTANCE)wParam, (UINT)lParam,
                             szString, MAX_RESOURCE_STRING_LENGTH);

        TRACE("len=%d %s\n", len, debugstr_w(szString));
        if (len == 0 || len == 1)
            return nIndex;

        TRACE("Delimiter: 0x%x\n", *szString);
        delimiter = *szString;
        p = szString + 1;

        while ((next_delim = strchrW(p, delimiter)) != NULL) {
            *next_delim = 0;
            if (next_delim + 1 >= szString + len)
            {
                /* this may happen if delimiter == '\0' or if the last char is a
                 * delimiter (then it is ignored like the native does) */
                break;
            }

            infoPtr->strings = ReAlloc(infoPtr->strings, sizeof(LPWSTR)*(infoPtr->nNumStrings+1));
            Str_SetPtrW(&infoPtr->strings[infoPtr->nNumStrings], p);
            infoPtr->nNumStrings++;

            p = next_delim + 1;
        }
    }
    else {
	LPWSTR p = (LPWSTR)lParam;
	INT len;

	if (p == NULL)
	    return -1;
	TRACE("adding string(s) from array!\n");
	while (*p) {
            len = strlenW (p);

            TRACE("len=%d %s\n", len, debugstr_w(p));
            infoPtr->strings = ReAlloc(infoPtr->strings, sizeof(LPWSTR)*(infoPtr->nNumStrings+1));
            Str_SetPtrW (&infoPtr->strings[infoPtr->nNumStrings], p);
	    infoPtr->nNumStrings++;

	    p += (len+1);
	}
    }

    if (fFirstString)
        TOOLBAR_CalcToolbar(hwnd);
    return nIndex;
}


static LRESULT
TOOLBAR_AddStringA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    BOOL fFirstString = (infoPtr->nNumStrings == 0);
    LPSTR p;
    INT nIndex;
    INT len;

    if ((wParam) && (HIWORD(lParam) == 0))  /* load from resources */
        return TOOLBAR_AddStringW(hwnd, wParam, lParam);

    p = (LPSTR)lParam;
    if (p == NULL)
        return -1;

    TRACE("adding string(s) from array!\n");
    nIndex = infoPtr->nNumStrings;
    while (*p) {
        len = strlen (p);
        TRACE("len=%d \"%s\"\n", len, p);

        infoPtr->strings = ReAlloc(infoPtr->strings, sizeof(LPWSTR)*(infoPtr->nNumStrings+1));
        Str_SetPtrAtoW(&infoPtr->strings[infoPtr->nNumStrings], p);
        infoPtr->nNumStrings++;

        p += (len+1);
    }

    if (fFirstString)
        TOOLBAR_CalcToolbar(hwnd);
    return nIndex;
}


static LRESULT
TOOLBAR_AutoSize (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    RECT parent_rect;
    HWND parent;
    INT  x, y;
    INT  cx, cy;

    TRACE("auto sizing, style=%x!\n", infoPtr->dwStyle);

    parent = GetParent (hwnd);

    if (!parent || !infoPtr->bDoRedraw)
        return 0;

    GetClientRect(parent, &parent_rect);

    x = parent_rect.left;
    y = parent_rect.top;

    TRACE("nRows: %d, infoPtr->nButtonHeight: %d\n", infoPtr->nRows, infoPtr->nButtonHeight);

    cy = TOP_BORDER + infoPtr->nRows * infoPtr->nButtonHeight + BOTTOM_BORDER;
    cx = parent_rect.right - parent_rect.left;

    if ((infoPtr->dwStyle & TBSTYLE_WRAPABLE) || (infoPtr->dwExStyle & TBSTYLE_EX_UNDOC1))
    {
        TOOLBAR_LayoutToolbar(hwnd);
        InvalidateRect( hwnd, NULL, TRUE );
    }

    if (!(infoPtr->dwStyle & CCS_NORESIZE))
    {
        RECT window_rect;
        UINT uPosFlags = SWP_NOZORDER;

        if ((infoPtr->dwStyle & CCS_BOTTOM) == CCS_NOMOVEY)
        {
            GetWindowRect(hwnd, &window_rect);
            ScreenToClient(parent, (LPPOINT)&window_rect.left);
            y = window_rect.top;
        }
        if ((infoPtr->dwStyle & CCS_BOTTOM) == CCS_BOTTOM)
        {
            GetWindowRect(hwnd, &window_rect);
            y = parent_rect.bottom - ( window_rect.bottom - window_rect.top);
        }

        if (infoPtr->dwStyle & CCS_NOPARENTALIGN)
            uPosFlags |= SWP_NOMOVE;
    
        if (!(infoPtr->dwStyle & CCS_NODIVIDER))
            cy += GetSystemMetrics(SM_CYEDGE);

        if (infoPtr->dwStyle & WS_BORDER)
        {
            x = y = 1; /* FIXME: this looks wrong */
            cy += GetSystemMetrics(SM_CYEDGE);
            cx += GetSystemMetrics(SM_CXEDGE);
        }

        SetWindowPos(hwnd, NULL, x, y, cx, cy, uPosFlags);
    }

    return 0;
}


static LRESULT
TOOLBAR_ButtonCount (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return infoPtr->nNumButtons;
}


static LRESULT
TOOLBAR_ButtonStructSize (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    infoPtr->dwStructSize = (DWORD)wParam;

    return 0;
}


static LRESULT
TOOLBAR_ChangeBitmap (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    TRACE("button %ld, iBitmap now %d\n", wParam, LOWORD(lParam));

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->iBitmap = LOWORD(lParam);

    /* we HAVE to erase the background, the new bitmap could be */
    /* transparent */
    InvalidateRect(hwnd, &btnPtr->rect, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_CheckButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    INT nOldIndex = -1;
    BOOL bChecked = FALSE;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);

    TRACE("hwnd=%p, btn index=%d, lParam=0x%08lx\n", hwnd, nIndex, lParam);

    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    bChecked = (btnPtr->fsState & TBSTATE_CHECKED) ? TRUE : FALSE;

    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_CHECKED;
    else {
	if (btnPtr->fsStyle & BTNS_GROUP) {
	    nOldIndex =
		TOOLBAR_GetCheckedGroupButtonIndex (infoPtr, nIndex);
	    if (nOldIndex == nIndex)
		return 0;
	    if (nOldIndex != -1)
		infoPtr->buttons[nOldIndex].fsState &= ~TBSTATE_CHECKED;
	}
	btnPtr->fsState |= TBSTATE_CHECKED;
    }

    if( bChecked != LOWORD(lParam) )
    {
        if (nOldIndex != -1)
            InvalidateRect(hwnd, &infoPtr->buttons[nOldIndex].rect, TRUE);
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);
    }

    /* FIXME: Send a WM_NOTIFY?? */

    return TRUE;
}


static LRESULT
TOOLBAR_CommandToIndex (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
}


static LRESULT
TOOLBAR_Customize (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    CUSTDLG_INFO custInfo;
    LRESULT ret;
    LPCVOID template;
    HRSRC hRes;
    NMHDR nmhdr;

    custInfo.tbInfo = infoPtr;
    custInfo.tbHwnd = hwnd;

    /* send TBN_BEGINADJUST notification */
    TOOLBAR_SendNotify (&nmhdr, infoPtr, TBN_BEGINADJUST);

    if (!(hRes = FindResourceW (COMCTL32_hModule,
                                MAKEINTRESOURCEW(IDD_TBCUSTOMIZE),
                                (LPWSTR)RT_DIALOG)))
	return FALSE;

    if(!(template = LoadResource (COMCTL32_hModule, hRes)))
	return FALSE;

    ret = DialogBoxIndirectParamW ((HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
                                   template, hwnd, TOOLBAR_CustomizeDialogProc,
                                   (LPARAM)&custInfo);

    /* send TBN_ENDADJUST notification */
    TOOLBAR_SendNotify (&nmhdr, infoPtr, TBN_ENDADJUST);

    return ret;
}


static LRESULT
TOOLBAR_DeleteButton (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex = (INT)wParam;
    NMTOOLBARW nmtb;
    TBUTTON_INFO *btnPtr = &infoPtr->buttons[nIndex];

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
        return FALSE;

    memset(&nmtb, 0, sizeof(nmtb));
    nmtb.iItem = btnPtr->idCommand;
    nmtb.tbButton.iBitmap = btnPtr->iBitmap;
    nmtb.tbButton.idCommand = btnPtr->idCommand;
    nmtb.tbButton.fsState = btnPtr->fsState;
    nmtb.tbButton.fsStyle = btnPtr->fsStyle;
    nmtb.tbButton.dwData = btnPtr->dwData;
    nmtb.tbButton.iString = btnPtr->iString;
    TOOLBAR_SendNotify(&nmtb.hdr, infoPtr, TBN_DELETINGBUTTON);

    TOOLBAR_TooltipDelTool(infoPtr, &infoPtr->buttons[nIndex]);

    if (infoPtr->nNumButtons == 1) {
	TRACE(" simple delete!\n");
	Free (infoPtr->buttons);
	infoPtr->buttons = NULL;
	infoPtr->nNumButtons = 0;
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
        TRACE("complex delete! [nIndex=%d]\n", nIndex);

	infoPtr->nNumButtons--;
	infoPtr->buttons = Alloc (sizeof (TBUTTON_INFO) * infoPtr->nNumButtons);
        if (nIndex > 0) {
            memcpy (&infoPtr->buttons[0], &oldButtons[0],
                    nIndex * sizeof(TBUTTON_INFO));
        }

        if (nIndex < infoPtr->nNumButtons) {
            memcpy (&infoPtr->buttons[nIndex], &oldButtons[nIndex+1],
                    (infoPtr->nNumButtons - nIndex) * sizeof(TBUTTON_INFO));
        }

	Free (oldButtons);
    }

    TOOLBAR_LayoutToolbar(hwnd);

    InvalidateRect (hwnd, NULL, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_EnableButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    DWORD bState;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);

    TRACE("hwnd=%p, btn index=%ld, lParam=0x%08lx\n", hwnd, wParam, lParam);

    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    bState = btnPtr->fsState & TBSTATE_ENABLED;

    /* update the toolbar button state */
    if(LOWORD(lParam) == FALSE) {
 	btnPtr->fsState &= ~(TBSTATE_ENABLED | TBSTATE_PRESSED);
    } else {
	btnPtr->fsState |= TBSTATE_ENABLED;
    }

    /* redraw the button only if the state of the button changed */
    if(bState != (btnPtr->fsState & TBSTATE_ENABLED))
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);

    return TRUE;
}


static inline LRESULT
TOOLBAR_GetAnchorHighlight (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return infoPtr->bAnchor;
}


static LRESULT
TOOLBAR_GetBitmap (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return infoPtr->buttons[nIndex].iBitmap;
}


static inline LRESULT
TOOLBAR_GetBitmapFlags (void)
{
    return (GetDeviceCaps (0, LOGPIXELSX) >= 120) ? TBBF_LARGE : 0;
}


static LRESULT
TOOLBAR_GetButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT nIndex = (INT)wParam;
    TBUTTON_INFO *btnPtr;

    if (lpTbb == NULL)
	return FALSE;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    lpTbb->iBitmap   = btnPtr->iBitmap;
    lpTbb->idCommand = btnPtr->idCommand;
    lpTbb->fsState   = btnPtr->fsState;
    lpTbb->fsStyle   = btnPtr->fsStyle;
    lpTbb->bReserved[0] = 0;
    lpTbb->bReserved[1] = 0;
    lpTbb->dwData    = btnPtr->dwData;
    lpTbb->iString   = btnPtr->iString;

    return TRUE;
}


static LRESULT
TOOLBAR_GetButtonInfoT(HWND hwnd, WPARAM wParam, LPARAM lParam, BOOL bUnicode)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    /* TBBUTTONINFOW and TBBUTTONINFOA have the same layout*/
    LPTBBUTTONINFOW lpTbInfo = (LPTBBUTTONINFOW)lParam;
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    if (lpTbInfo == NULL)
	return -1;

    /* MSDN documents a iImageLabel field added in Vista but it is not present in
     * the headers and tests shows that even with comctl 6 Vista accepts only the
     * original TBBUTTONINFO size
     */
    if (lpTbInfo->cbSize != sizeof(TBBUTTONINFOW))
    {
        WARN("Invalid button size\n");
	return -1;
    }

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam,
				     lpTbInfo->dwMask & 0x80000000);
    if (nIndex == -1)
	return -1;

    if (!(btnPtr = &infoPtr->buttons[nIndex])) return -1;

    if (lpTbInfo->dwMask & TBIF_COMMAND)
	lpTbInfo->idCommand = btnPtr->idCommand;
    if (lpTbInfo->dwMask & TBIF_IMAGE)
	lpTbInfo->iImage = btnPtr->iBitmap;
    if (lpTbInfo->dwMask & TBIF_LPARAM)
	lpTbInfo->lParam = btnPtr->dwData;
    if (lpTbInfo->dwMask & TBIF_SIZE)
        /* tests show that for separators TBIF_SIZE returns not calculated width,
           but cx property, that differs from 0 only if application have
           specifically set it */
        lpTbInfo->cx = (btnPtr->fsStyle & BTNS_SEP)
            ? btnPtr->cx : (WORD)(btnPtr->rect.right - btnPtr->rect.left);
    if (lpTbInfo->dwMask & TBIF_STATE)
	lpTbInfo->fsState = btnPtr->fsState;
    if (lpTbInfo->dwMask & TBIF_STYLE)
	lpTbInfo->fsStyle = btnPtr->fsStyle;
    if (lpTbInfo->dwMask & TBIF_TEXT) {
        /* TB_GETBUTTONINFO doesn't retrieve text from the string list, so we
           can't use TOOLBAR_GetText here */
        if (HIWORD(btnPtr->iString) && (btnPtr->iString != -1)) {
            LPWSTR lpText = (LPWSTR)btnPtr->iString;
            if (bUnicode)
                Str_GetPtrW(lpText, lpTbInfo->pszText, lpTbInfo->cchText);
            else
                Str_GetPtrWtoA(lpText, (LPSTR)lpTbInfo->pszText, lpTbInfo->cchText);
        } else
            lpTbInfo->pszText[0] = '\0';
    }
    return nIndex;
}


static LRESULT
TOOLBAR_GetButtonSize (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return MAKELONG((WORD)infoPtr->nButtonWidth,
                    (WORD)infoPtr->nButtonHeight);
}


static LRESULT
TOOLBAR_GetButtonTextA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;
    LPWSTR lpText;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    lpText = TOOLBAR_GetText(infoPtr,&infoPtr->buttons[nIndex]);

    return WideCharToMultiByte( CP_ACP, 0, lpText, -1,
                                (LPSTR)lParam, lParam ? 0x7fffffff : 0, NULL, NULL ) - 1;
}


static LRESULT
TOOLBAR_GetButtonTextW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;
    LPWSTR lpText;
    LRESULT ret = 0;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    lpText = TOOLBAR_GetText(infoPtr,&infoPtr->buttons[nIndex]);

    if (lpText)
    {
        ret = strlenW (lpText);

        if (lParam)
            strcpyW ((LPWSTR)lParam, lpText);
    }

    return ret;
}


static LRESULT
TOOLBAR_GetDisabledImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p, wParam=%ld, lParam=0x%lx\n", hwnd, wParam, lParam);
    /* UNDOCUMENTED: wParam is actually the ID of the image list to return */
    return (LRESULT)GETDISIMAGELIST(TOOLBAR_GetInfoPtr (hwnd), wParam);
}


static inline LRESULT
TOOLBAR_GetExtendedStyle (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("\n");

    return infoPtr->dwExStyle;
}


static LRESULT
TOOLBAR_GetHotImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p, wParam=%ld, lParam=0x%lx\n", hwnd, wParam, lParam);
    /* UNDOCUMENTED: wParam is actually the ID of the image list to return */
    return (LRESULT)GETHOTIMAGELIST(TOOLBAR_GetInfoPtr (hwnd), wParam);
}


static LRESULT
TOOLBAR_GetHotItem (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (!((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)))
	return -1;

    if (infoPtr->nHotItem < 0)
	return -1;

    return (LRESULT)infoPtr->nHotItem;
}


static LRESULT
TOOLBAR_GetDefImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p, wParam=%ld, lParam=0x%lx\n", hwnd, wParam, lParam);
    /* UNDOCUMENTED: wParam is actually the ID of the image list to return */
    return (LRESULT) GETDEFIMAGELIST(TOOLBAR_GetInfoPtr(hwnd), wParam);
}


static LRESULT
TOOLBAR_GetInsertMark (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBINSERTMARK *lptbim = (TBINSERTMARK*)lParam;

    TRACE("hwnd = %p, lptbim = %p\n", hwnd, lptbim);

    *lptbim = infoPtr->tbim;

    return 0;
}


static LRESULT
TOOLBAR_GetInsertMarkColor (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("hwnd = %p\n", hwnd);

    return (LRESULT)infoPtr->clrInsertMark;
}


static LRESULT
TOOLBAR_GetItemRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    LPRECT     lpRect;
    INT        nIndex;

    nIndex = (INT)wParam;
    btnPtr = &infoPtr->buttons[nIndex];
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;
    lpRect = (LPRECT)lParam;
    if (lpRect == NULL)
	return FALSE;
    if (btnPtr->fsState & TBSTATE_HIDDEN)
	return FALSE;

    lpRect->left   = btnPtr->rect.left;
    lpRect->right  = btnPtr->rect.right;
    lpRect->bottom = btnPtr->rect.bottom;
    lpRect->top    = btnPtr->rect.top;

    return TRUE;
}


static LRESULT
TOOLBAR_GetMaxSize (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPSIZE lpSize = (LPSIZE)lParam;

    if (lpSize == NULL)
	return FALSE;

    lpSize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
    lpSize->cy = infoPtr->rcBound.bottom - infoPtr->rcBound.top;

    TRACE("maximum size %d x %d\n",
	   infoPtr->rcBound.right - infoPtr->rcBound.left,
	   infoPtr->rcBound.bottom - infoPtr->rcBound.top);

    return TRUE;
}


/* << TOOLBAR_GetObject >> */


static LRESULT
TOOLBAR_GetPadding (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD oldPad;

    oldPad = MAKELONG(infoPtr->szPadding.cx, infoPtr->szPadding.cy);
    return (LRESULT) oldPad;
}


static LRESULT
TOOLBAR_GetRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    LPRECT     lpRect;
    INT        nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    btnPtr = &infoPtr->buttons[nIndex];
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;
    lpRect = (LPRECT)lParam;
    if (lpRect == NULL)
	return FALSE;

    lpRect->left   = btnPtr->rect.left;
    lpRect->right  = btnPtr->rect.right;
    lpRect->bottom = btnPtr->rect.bottom;
    lpRect->top    = btnPtr->rect.top;

    return TRUE;
}


static LRESULT
TOOLBAR_GetRows (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return infoPtr->nRows;
}


static LRESULT
TOOLBAR_GetState (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return infoPtr->buttons[nIndex].fsState;
}


static LRESULT
TOOLBAR_GetStyle (HWND hwnd)
{
    return GetWindowLongW(hwnd, GWL_STYLE);
}


static LRESULT
TOOLBAR_GetTextRows (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return infoPtr->nMaxTextRows;
}


static LRESULT
TOOLBAR_GetToolTips (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if ((infoPtr->dwStyle & TBSTYLE_TOOLTIPS) && (infoPtr->hwndToolTip == NULL))
        TOOLBAR_TooltipCreateControl(infoPtr);
    return (LRESULT)infoPtr->hwndToolTip;
}


static LRESULT
TOOLBAR_GetUnicodeFormat (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("%s hwnd=%p\n",
	   infoPtr->bUnicode ? "TRUE" : "FALSE", hwnd);

    return infoPtr->bUnicode;
}


static inline LRESULT
TOOLBAR_GetVersion (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    return infoPtr->iVersion;
}


static LRESULT
TOOLBAR_HideButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    TRACE("\n");

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_HIDDEN;
    else
	btnPtr->fsState |= TBSTATE_HIDDEN;

    TOOLBAR_LayoutToolbar (hwnd);

    InvalidateRect (hwnd, NULL, TRUE);

    return TRUE;
}


static inline LRESULT
TOOLBAR_HitTest (HWND hwnd, LPARAM lParam)
{
    return TOOLBAR_InternalHitTest (hwnd, (LPPOINT)lParam);
}


static LRESULT
TOOLBAR_Indeterminate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    DWORD oldState;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    oldState = btnPtr->fsState;
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_INDETERMINATE;
    else
	btnPtr->fsState |= TBSTATE_INDETERMINATE;

    if(oldState != btnPtr->fsState)
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_InsertButtonT(HWND hwnd, WPARAM wParam, LPARAM lParam, BOOL fUnicode)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT nIndex = (INT)wParam;

    if (lpTbb == NULL)
	return FALSE;

    if (nIndex == -1) {
       /* EPP: this seems to be an undocumented call (from my IE4)
	* I assume in that case that:
	* - index of insertion is at the end of existing buttons
	* I only see this happen with nIndex == -1, but it could have a special
	* meaning (like -nIndex (or ~nIndex) to get the real position of insertion).
	*/
	nIndex = infoPtr->nNumButtons;

    } else if (nIndex < 0)
       return FALSE;

    TRACE("inserting button index=%d\n", nIndex);
    if (nIndex > infoPtr->nNumButtons) {
	nIndex = infoPtr->nNumButtons;
	TRACE("adjust index=%d\n", nIndex);
    }

    return TOOLBAR_InternalInsertButtonsT(infoPtr, nIndex, 1, lpTbb, fUnicode);
}

/* << TOOLBAR_InsertMarkHitTest >> */


static LRESULT
TOOLBAR_IsButtonChecked (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_CHECKED);
}


static LRESULT
TOOLBAR_IsButtonEnabled (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_ENABLED);
}


static LRESULT
TOOLBAR_IsButtonHidden (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_HIDDEN);
}


static LRESULT
TOOLBAR_IsButtonHighlighted (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_MARKED);
}


static LRESULT
TOOLBAR_IsButtonIndeterminate (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_INDETERMINATE);
}


static LRESULT
TOOLBAR_IsButtonPressed (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_PRESSED);
}


static LRESULT
TOOLBAR_LoadImages (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TBADDBITMAP tbab;
    tbab.hInst = (HINSTANCE)lParam;
    tbab.nID = wParam;

    TRACE("hwnd = %p, hInst = %p, nID = %lu\n", hwnd, tbab.hInst, tbab.nID);

    return TOOLBAR_AddBitmap(hwnd, 0, (LPARAM)&tbab);
}


static LRESULT
TOOLBAR_MapAccelerator (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    WCHAR wAccel = (WCHAR)wParam;
    UINT* pIDButton = (UINT*)lParam;
    WCHAR wszAccel[] = {'&',wAccel,0};
    int i;
    
    TRACE("hwnd = %p, wAccel = %x(%s), pIDButton = %p\n",
        hwnd, wAccel, debugstr_wn(&wAccel,1), pIDButton);
    
    for (i = 0; i < infoPtr->nNumButtons; i++)
    {
        TBUTTON_INFO *btnPtr = infoPtr->buttons+i;
        if (!(btnPtr->fsStyle & BTNS_NOPREFIX) &&
            !(btnPtr->fsState & TBSTATE_HIDDEN))
        {
            int iLen = strlenW(wszAccel);
            LPCWSTR lpszStr = TOOLBAR_GetText(infoPtr, btnPtr);
            
            if (!lpszStr)
                continue;

            while (*lpszStr)
            {
                if ((lpszStr[0] == '&') && (lpszStr[1] == '&'))
                {
                    lpszStr += 2;
                    continue;
                }
                if (!strncmpiW(lpszStr, wszAccel, iLen))
                {
                    *pIDButton = btnPtr->idCommand;
                    return TRUE;
                }
                lpszStr++;
            }
        }
    }
    return FALSE;
}


static LRESULT
TOOLBAR_MarkButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;
    DWORD oldState;
    TBUTTON_INFO *btnPtr;

    TRACE("hwnd = %p, wParam = %ld, lParam = 0x%08lx\n", hwnd, wParam, lParam);

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
        return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    oldState = btnPtr->fsState;

    if (LOWORD(lParam))
        btnPtr->fsState |= TBSTATE_MARKED;
    else
        btnPtr->fsState &= ~TBSTATE_MARKED;

    if(oldState != btnPtr->fsState)
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);

    return TRUE;
}


/* fixes up an index of a button affected by a move */
static inline void TOOLBAR_MoveFixupIndex(INT* pIndex, INT nIndex, INT nMoveIndex, BOOL bMoveUp)
{
    if (bMoveUp)
    {
        if (*pIndex > nIndex && *pIndex <= nMoveIndex)
            (*pIndex)--;
        else if (*pIndex == nIndex)
            *pIndex = nMoveIndex;
    }
    else
    {
        if (*pIndex >= nMoveIndex && *pIndex < nIndex)
            (*pIndex)++;
        else if (*pIndex == nIndex)
            *pIndex = nMoveIndex;
    }
}


static LRESULT
TOOLBAR_MoveButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;
    INT nCount;
    INT nMoveIndex = (INT)lParam;
    TBUTTON_INFO button;

    TRACE("hwnd=%p, wParam=%ld, lParam=%ld\n", hwnd, wParam, lParam);

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, TRUE);
    if ((nIndex == -1) || (nMoveIndex < 0))
        return FALSE;

    if (nMoveIndex > infoPtr->nNumButtons - 1)
        nMoveIndex = infoPtr->nNumButtons - 1;

    button = infoPtr->buttons[nIndex];

    /* move button right */
    if (nIndex < nMoveIndex)
    {
        nCount = nMoveIndex - nIndex;
        memmove(&infoPtr->buttons[nIndex], &infoPtr->buttons[nIndex+1], nCount*sizeof(TBUTTON_INFO));
        infoPtr->buttons[nMoveIndex] = button;

        TOOLBAR_MoveFixupIndex(&infoPtr->nButtonDown, nIndex, nMoveIndex, TRUE);
        TOOLBAR_MoveFixupIndex(&infoPtr->nButtonDrag, nIndex, nMoveIndex, TRUE);
        TOOLBAR_MoveFixupIndex(&infoPtr->nOldHit, nIndex, nMoveIndex, TRUE);
        TOOLBAR_MoveFixupIndex(&infoPtr->nHotItem, nIndex, nMoveIndex, TRUE);
    }
    else if (nIndex > nMoveIndex) /* move button left */
    {
        nCount = nIndex - nMoveIndex;
        memmove(&infoPtr->buttons[nMoveIndex+1], &infoPtr->buttons[nMoveIndex], nCount*sizeof(TBUTTON_INFO));
        infoPtr->buttons[nMoveIndex] = button;

        TOOLBAR_MoveFixupIndex(&infoPtr->nButtonDown, nIndex, nMoveIndex, FALSE);
        TOOLBAR_MoveFixupIndex(&infoPtr->nButtonDrag, nIndex, nMoveIndex, FALSE);
        TOOLBAR_MoveFixupIndex(&infoPtr->nOldHit, nIndex, nMoveIndex, FALSE);
        TOOLBAR_MoveFixupIndex(&infoPtr->nHotItem, nIndex, nMoveIndex, FALSE);
    }

    TOOLBAR_LayoutToolbar(hwnd);
    TOOLBAR_AutoSize(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_PressButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    DWORD oldState;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    oldState = btnPtr->fsState;
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_PRESSED;
    else
	btnPtr->fsState |= TBSTATE_PRESSED;

    if(oldState != btnPtr->fsState)
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);

    return TRUE;
}

/* FIXME: there might still be some confusion her between number of buttons
 * and number of bitmaps */
static LRESULT
TOOLBAR_ReplaceBitmap (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBREPLACEBITMAP lpReplace = (LPTBREPLACEBITMAP) lParam;
    HBITMAP hBitmap;
    int i = 0, nOldButtons = 0, pos = 0;
    int nOldBitmaps, nNewBitmaps = 0;
    HIMAGELIST himlDef = 0;

    TRACE("hInstOld %p nIDOld %lx hInstNew %p nIDNew %lx nButtons %x\n",
          lpReplace->hInstOld, lpReplace->nIDOld, lpReplace->hInstNew, lpReplace->nIDNew,
          lpReplace->nButtons);

    if (lpReplace->hInstOld == HINST_COMMCTRL)
    {
        FIXME("changing standard bitmaps not implemented\n");
        return FALSE;
    }
    else if (lpReplace->hInstOld != 0)
        FIXME("resources not in the current module not implemented\n");

    TRACE("To be replaced hInstOld %p nIDOld %lx\n", lpReplace->hInstOld, lpReplace->nIDOld);
    for (i = 0; i < infoPtr->nNumBitmapInfos; i++) {
        TBITMAP_INFO *tbi = &infoPtr->bitmaps[i];
        TRACE("tbimapinfo %d hInstOld %p nIDOld %x\n", i, tbi->hInst, tbi->nID);
        if (tbi->hInst == lpReplace->hInstOld && tbi->nID == lpReplace->nIDOld)
        {
            TRACE("Found: nButtons %d hInst %p nID %x\n", tbi->nButtons, tbi->hInst, tbi->nID);
            nOldButtons = tbi->nButtons;
            tbi->nButtons = lpReplace->nButtons;
            tbi->hInst = lpReplace->hInstNew;
            tbi->nID = lpReplace->nIDNew;
            TRACE("tbimapinfo changed %d hInstOld %p nIDOld %x\n", i, tbi->hInst, tbi->nID);
            break;
        }
        pos += tbi->nButtons;
    }

    if (nOldButtons == 0)
    {
        WARN("No hinst/bitmap found! hInst %p nID %lx\n", lpReplace->hInstOld, lpReplace->nIDOld);
        return FALSE;
    }
    
    /* copy the bitmap before adding it as ImageList_AddMasked modifies the
    * bitmap
    */
    if (lpReplace->hInstNew)
        hBitmap = LoadBitmapW(lpReplace->hInstNew,(LPWSTR)lpReplace->nIDNew);
    else
        hBitmap = CopyImage((HBITMAP)lpReplace->nIDNew, IMAGE_BITMAP, 0, 0, 0);

    himlDef = GETDEFIMAGELIST(infoPtr, 0); /* fixme: correct? */
    nOldBitmaps = ImageList_GetImageCount(himlDef);

    /* ImageList_Replace(GETDEFIMAGELIST(), pos, hBitmap, NULL); */

    for (i = pos + nOldBitmaps - 1; i >= pos; i--)
        ImageList_Remove(himlDef, i);

    if (hBitmap)
    {
       ImageList_AddMasked (himlDef, hBitmap, comctl32_color.clrBtnFace);
       nNewBitmaps = ImageList_GetImageCount(himlDef);
       DeleteObject(hBitmap);
    }

    infoPtr->nNumBitmaps = infoPtr->nNumBitmaps - nOldBitmaps + nNewBitmaps;

    TRACE(" pos %d  %d old bitmaps replaced by %d new ones.\n",
            pos, nOldBitmaps, nNewBitmaps);

    InvalidateRect(hwnd, NULL, TRUE);
    return TRUE;
}


/* helper for TOOLBAR_SaveRestoreW */
static BOOL
TOOLBAR_Save(const TBSAVEPARAMSW *lpSave)
{
    FIXME("save to %s %s\n", debugstr_w(lpSave->pszSubKey),
        debugstr_w(lpSave->pszValueName));

    return FALSE;
}


/* helper for TOOLBAR_Restore */
static void
TOOLBAR_DeleteAllButtons(TOOLBAR_INFO *infoPtr)
{
    INT i;

    for (i = 0; i < infoPtr->nNumButtons; i++)
    {
        TOOLBAR_TooltipDelTool(infoPtr, &infoPtr->buttons[i]);
    }

    Free(infoPtr->buttons);
    infoPtr->buttons = NULL;
    infoPtr->nNumButtons = 0;
}


/* helper for TOOLBAR_SaveRestoreW */
static BOOL
TOOLBAR_Restore(TOOLBAR_INFO *infoPtr, const TBSAVEPARAMSW *lpSave)
{
    LONG res;
    HKEY hkey = NULL;
    BOOL ret = FALSE;
    DWORD dwType;
    DWORD dwSize = 0;
    NMTBRESTORE nmtbr;

    /* restore toolbar information */
    TRACE("restore from %s %s\n", debugstr_w(lpSave->pszSubKey),
        debugstr_w(lpSave->pszValueName));

    memset(&nmtbr, 0, sizeof(nmtbr));

    res = RegOpenKeyExW(lpSave->hkr, lpSave->pszSubKey, 0,
        KEY_QUERY_VALUE, &hkey);
    if (!res)
        res = RegQueryValueExW(hkey, lpSave->pszValueName, NULL, &dwType,
            NULL, &dwSize);
    if (!res && dwType != REG_BINARY)
        res = ERROR_FILE_NOT_FOUND;
    if (!res)
    {
        nmtbr.pData = Alloc(dwSize);
        nmtbr.cbData = dwSize;
        if (!nmtbr.pData) res = ERROR_OUTOFMEMORY;
    }
    if (!res)
        res = RegQueryValueExW(hkey, lpSave->pszValueName, NULL, &dwType,
            (LPBYTE)nmtbr.pData, &dwSize);
    if (!res)
    {
        nmtbr.pCurrent = nmtbr.pData;
        nmtbr.iItem = -1;
        nmtbr.cbBytesPerRecord = sizeof(DWORD);
        nmtbr.cButtons = nmtbr.cbData / nmtbr.cbBytesPerRecord;

        if (!TOOLBAR_SendNotify(&nmtbr.hdr, infoPtr, TBN_RESTORE))
        {
            INT i;

            /* remove all existing buttons as this function is designed to
             * restore the toolbar to a previously saved state */
            TOOLBAR_DeleteAllButtons(infoPtr);

            for (i = 0; i < nmtbr.cButtons; i++)
            {
                nmtbr.iItem = i;
                nmtbr.tbButton.iBitmap = -1;
                nmtbr.tbButton.fsState = 0;
                nmtbr.tbButton.fsStyle = 0;
                nmtbr.tbButton.idCommand = 0;
                if (*nmtbr.pCurrent == (DWORD)-1)
                {
                    /* separator */
                    nmtbr.tbButton.fsStyle = TBSTYLE_SEP;
                    /* when inserting separators, iBitmap controls it's size.
                       0 sets default size (width) */
                    nmtbr.tbButton.iBitmap = 0;
                }
                else if (*nmtbr.pCurrent == (DWORD)-2)
                    /* hidden button */
                    nmtbr.tbButton.fsState = TBSTATE_HIDDEN;
                else
                    nmtbr.tbButton.idCommand = (int)*nmtbr.pCurrent;

                nmtbr.pCurrent++;
                
                TOOLBAR_SendNotify(&nmtbr.hdr, infoPtr, TBN_RESTORE);

                /* can't contain real string as we don't know whether
                 * the client put an ANSI or Unicode string in there */
                if (HIWORD(nmtbr.tbButton.iString))
                    nmtbr.tbButton.iString = 0;

                TOOLBAR_InsertButtonT(infoPtr->hwndSelf, -1,
                    (LPARAM)&nmtbr.tbButton, TRUE);
            }

            /* do legacy notifications */
            if (infoPtr->iVersion < 5)
            {
                /* FIXME: send TBN_BEGINADJUST */
                FIXME("send TBN_GETBUTTONINFO for each button\n");
                /* FIXME: send TBN_ENDADJUST */
            }

            /* remove all uninitialised buttons
             * note: loop backwards to avoid having to fixup i on a
             * delete */
            for (i = infoPtr->nNumButtons - 1; i >= 0; i--)
                if (infoPtr->buttons[i].iBitmap == -1)
                    TOOLBAR_DeleteButton(infoPtr->hwndSelf, i);

            /* only indicate success if at least one button survived */
            if (infoPtr->nNumButtons > 0) ret = TRUE;
        }
    }
    Free (nmtbr.pData);
    RegCloseKey(hkey);

    return ret;
}


static LRESULT
TOOLBAR_SaveRestoreW (HWND hwnd, WPARAM wParam, const TBSAVEPARAMSW *lpSave)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (lpSave == NULL) return 0;

    if (wParam)
        return TOOLBAR_Save(lpSave);
    else
        return TOOLBAR_Restore(infoPtr, lpSave);
}


static LRESULT
TOOLBAR_SaveRestoreA (HWND hwnd, WPARAM wParam, const TBSAVEPARAMSA *lpSave)
{
    LPWSTR pszValueName = 0, pszSubKey = 0;
    TBSAVEPARAMSW SaveW;
    LRESULT result = 0;
    int len;

    if (lpSave == NULL) return 0;

    len = MultiByteToWideChar(CP_ACP, 0, lpSave->pszSubKey, -1, NULL, 0);
    pszSubKey = Alloc(len * sizeof(WCHAR));
    if (pszSubKey) goto exit;
    MultiByteToWideChar(CP_ACP, 0, lpSave->pszSubKey, -1, pszSubKey, len);

    len = MultiByteToWideChar(CP_ACP, 0, lpSave->pszValueName, -1, NULL, 0);
    pszValueName = Alloc(len * sizeof(WCHAR));
    if (!pszValueName) goto exit;
    MultiByteToWideChar(CP_ACP, 0, lpSave->pszValueName, -1, pszValueName, len);

    SaveW.pszValueName = pszValueName;
    SaveW.pszSubKey = pszSubKey;
    SaveW.hkr = lpSave->hkr;
    result = TOOLBAR_SaveRestoreW(hwnd, wParam, &SaveW);

exit:
    Free (pszValueName);
    Free (pszSubKey);

    return result;
}


static LRESULT
TOOLBAR_SetAnchorHighlight (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    BOOL bOldAnchor = infoPtr->bAnchor;

    TRACE("hwnd=%p, bAnchor = %s\n", hwnd, wParam ? "TRUE" : "FALSE");

    infoPtr->bAnchor = (BOOL)wParam;

    /* Native does not remove the hot effect from an already hot button */

    return (LRESULT)bOldAnchor;
}


static LRESULT
TOOLBAR_SetBitmapSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    HIMAGELIST himlDef = GETDEFIMAGELIST(infoPtr, 0);
    short width = (short)LOWORD(lParam);
    short height = (short)HIWORD(lParam);

    TRACE("hwnd=%p, wParam=%ld, lParam=%ld\n", hwnd, wParam, lParam);

    if (wParam != 0)
        FIXME("wParam is %ld. Perhaps image list index?\n", wParam);

    /* 0 width or height is changed to 1 */
    if (width == 0)
        width = 1;
    if (height == 0)
        height = 1;

    if (infoPtr->nNumButtons > 0)
        TRACE("%d buttons, undoc change to bitmap size : %d-%d -> %d-%d\n",
              infoPtr->nNumButtons,
              infoPtr->nBitmapWidth, infoPtr->nBitmapHeight, width, height);

    if (width < -1 || height < -1)
    {
        /* Windows destroys the imagelist and seems to actually use negative
         * values to compute button sizes */
        FIXME("Negative bitmap sizes not supported (%d, %d)\n", width, height);
        return FALSE;
    }

    /* width or height of -1 means no change */
    if (width != -1)
        infoPtr->nBitmapWidth = width;
    if (height != -1)
        infoPtr->nBitmapHeight = height;

    if ((himlDef == infoPtr->himlInt) &&
        (ImageList_GetImageCount(infoPtr->himlInt) == 0))
    {
        ImageList_SetIconSize(infoPtr->himlInt, infoPtr->nBitmapWidth,
            infoPtr->nBitmapHeight);
    }

    TOOLBAR_CalcToolbar(hwnd);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTONINFOA lptbbi = (LPTBBUTTONINFOA)lParam;
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    RECT oldBtnRect;

    if (lptbbi == NULL)
	return FALSE;
    if (lptbbi->cbSize < sizeof(TBBUTTONINFOA))
	return FALSE;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam,
				     lptbbi->dwMask & 0x80000000);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (lptbbi->dwMask & TBIF_COMMAND)
	btnPtr->idCommand = lptbbi->idCommand;
    if (lptbbi->dwMask & TBIF_IMAGE)
	btnPtr->iBitmap = lptbbi->iImage;
    if (lptbbi->dwMask & TBIF_LPARAM)
	btnPtr->dwData = lptbbi->lParam;
    if (lptbbi->dwMask & TBIF_SIZE)
	btnPtr->cx = lptbbi->cx;
    if (lptbbi->dwMask & TBIF_STATE)
	btnPtr->fsState = lptbbi->fsState;
    if (lptbbi->dwMask & TBIF_STYLE)
	btnPtr->fsStyle = lptbbi->fsStyle;

    if ((lptbbi->dwMask & TBIF_TEXT) && ((INT_PTR)lptbbi->pszText != -1)) {
        if ((HIWORD(btnPtr->iString) == 0) || (btnPtr->iString == -1))
	    /* iString is index, zero it to make Str_SetPtr succeed */
	    btnPtr->iString=0;

         Str_SetPtrAtoW ((LPWSTR *)&btnPtr->iString, lptbbi->pszText);
    }

    /* save the button rect to see if we need to redraw the whole toolbar */
    oldBtnRect = btnPtr->rect;
    TOOLBAR_LayoutToolbar(hwnd);

    if (!EqualRect(&oldBtnRect, &btnPtr->rect))
        InvalidateRect(hwnd, NULL, TRUE);
    else
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTONINFOW lptbbi = (LPTBBUTTONINFOW)lParam;
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    RECT oldBtnRect;

    if (lptbbi == NULL)
	return FALSE;
    if (lptbbi->cbSize < sizeof(TBBUTTONINFOW))
	return FALSE;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam,
				     lptbbi->dwMask & 0x80000000);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (lptbbi->dwMask & TBIF_COMMAND)
	btnPtr->idCommand = lptbbi->idCommand;
    if (lptbbi->dwMask & TBIF_IMAGE)
	btnPtr->iBitmap = lptbbi->iImage;
    if (lptbbi->dwMask & TBIF_LPARAM)
	btnPtr->dwData = lptbbi->lParam;
    if (lptbbi->dwMask & TBIF_SIZE)
	btnPtr->cx = lptbbi->cx;
    if (lptbbi->dwMask & TBIF_STATE)
	btnPtr->fsState = lptbbi->fsState;
    if (lptbbi->dwMask & TBIF_STYLE)
	btnPtr->fsStyle = lptbbi->fsStyle;

    if ((lptbbi->dwMask & TBIF_TEXT) && ((INT_PTR)lptbbi->pszText != -1)) {
        if ((HIWORD(btnPtr->iString) == 0) || (btnPtr->iString == -1))
	    /* iString is index, zero it to make Str_SetPtr succeed */
	    btnPtr->iString=0;
        Str_SetPtrW ((LPWSTR *)&btnPtr->iString, lptbbi->pszText);
    }

    /* save the button rect to see if we need to redraw the whole toolbar */
    oldBtnRect = btnPtr->rect;
    TOOLBAR_LayoutToolbar(hwnd);

    if (!EqualRect(&oldBtnRect, &btnPtr->rect))
        InvalidateRect(hwnd, NULL, TRUE);
    else
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonSize (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT cx = (short)LOWORD(lParam), cy = (short)HIWORD(lParam);

    if ((cx < 0) || (cy < 0))
    {
        ERR("invalid parameter 0x%08x\n", (DWORD)lParam);
        return FALSE;
    }

    TRACE("%p, cx = %d, cy = %d\n", hwnd, cx, cy);

    /* The documentation claims you can only change the button size before
     * any button has been added. But this is wrong.
     * WINZIP32.EXE (ver 8) calls this on one of its buttons after adding
     * it to the toolbar, and it checks that the return value is nonzero - mjm
     * Further testing shows that we must actually perform the change too.
     */
    /*
     * The documentation also does not mention that if 0 is supplied for
     * either size, the system changes it to the default of 24 wide and
     * 22 high. Demonstarted in ControlSpy Toolbar. GLA 3/02
     */
    if (cx == 0) cx = 24;
    if (cy == 0) cy = 22;
    
    cx = max(cx, infoPtr->szPadding.cx + infoPtr->nBitmapWidth);
    cy = max(cy, infoPtr->szPadding.cy + infoPtr->nBitmapHeight);

    infoPtr->nButtonWidth = cx;
    infoPtr->nButtonHeight = cy;
    
    infoPtr->iTopMargin = default_top_margin(infoPtr);
    TOOLBAR_LayoutToolbar(hwnd);
    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonWidth (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    /* if setting to current values, ignore */
    if ((infoPtr->cxMin == (short)LOWORD(lParam)) &&
	(infoPtr->cxMax == (short)HIWORD(lParam))) {
	TRACE("matches current width, min=%d, max=%d, no recalc\n",
	      infoPtr->cxMin, infoPtr->cxMax);
	return TRUE;
    }

    /* save new values */
    infoPtr->cxMin = (short)LOWORD(lParam);
    infoPtr->cxMax = (short)HIWORD(lParam);

    /* otherwise we need to recalc the toolbar and in some cases
       recalc the bounding rectangle (does DrawText w/ DT_CALCRECT
       which doesn't actually draw - GA). */
    TRACE("number of buttons %d, cx=%d, cy=%d, recalcing\n",
	infoPtr->nNumButtons, infoPtr->cxMin, infoPtr->cxMax);

    TOOLBAR_CalcToolbar (hwnd);

    InvalidateRect (hwnd, NULL, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_SetCmdId (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex = (INT)wParam;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    infoPtr->buttons[nIndex].idCommand = (INT)lParam;

    if (infoPtr->hwndToolTip) {

	FIXME("change tool tip!\n");

    }

    return TRUE;
}


static LRESULT
TOOLBAR_SetDisabledImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    HIMAGELIST himl = (HIMAGELIST)lParam;
    HIMAGELIST himlTemp;
    INT id = 0;

    if (infoPtr->iVersion >= 5)
        id = wParam;

    himlTemp = TOOLBAR_InsertImageList(&infoPtr->himlDis, 
        &infoPtr->cimlDis, himl, id);

    /* FIXME: redraw ? */

    return (LRESULT)himlTemp;
}


static LRESULT
TOOLBAR_SetDrawTextFlags (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwTemp;

    TRACE("hwnd = %p, dwMask = 0x%08x, dwDTFlags = 0x%08x\n", hwnd, (DWORD)wParam, (DWORD)lParam);

    dwTemp = infoPtr->dwDTFlags;
    infoPtr->dwDTFlags =
	(infoPtr->dwDTFlags & (DWORD)wParam) | (DWORD)lParam;

    return (LRESULT)dwTemp;
}

/* This function differs a bit from what MSDN says it does:
 * 1. lParam contains extended style flags to OR with current style
 *  (MSDN isn't clear on the OR bit)
 * 2. wParam appears to contain extended style flags to be reset
 *  (MSDN says that this parameter is reserved)
 */
static LRESULT
TOOLBAR_SetExtendedStyle (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwOldStyle;

    dwOldStyle = infoPtr->dwExStyle;
    infoPtr->dwExStyle = (DWORD)lParam;

    TRACE("new style 0x%08x\n", infoPtr->dwExStyle);

    if (infoPtr->dwExStyle & ~TBSTYLE_EX_ALL)
	FIXME("Unknown Toolbar Extended Style 0x%08x. Please report.\n",
	      (infoPtr->dwExStyle & ~TBSTYLE_EX_ALL));

    if ((dwOldStyle ^ infoPtr->dwExStyle) & TBSTYLE_EX_MIXEDBUTTONS)
        TOOLBAR_CalcToolbar(hwnd);
    else
        TOOLBAR_LayoutToolbar(hwnd);

    TOOLBAR_AutoSize(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);

    return (LRESULT)dwOldStyle;
}


static LRESULT
TOOLBAR_SetHotImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    HIMAGELIST himlTemp;
    HIMAGELIST himl = (HIMAGELIST)lParam;
    INT id = 0;

    if (infoPtr->iVersion >= 5)
        id = wParam;

    TRACE("hwnd = %p, himl = %p, id = %d\n", hwnd, himl, id);

    himlTemp = TOOLBAR_InsertImageList(&infoPtr->himlHot, 
        &infoPtr->cimlHot, himl, id);

    /* FIXME: redraw ? */

    return (LRESULT)himlTemp;
}


/* Makes previous hot button no longer hot, makes the specified
 * button hot and sends appropriate notifications. dwReason is one or
 * more HICF_ flags. Specify nHit < 0 to make no buttons hot.
 * NOTE 1: this function does not validate nHit
 * NOTE 2: the name of this function is completely made up and
 * not based on any documentation from Microsoft. */
static void
TOOLBAR_SetHotItemEx (TOOLBAR_INFO *infoPtr, INT nHit, DWORD dwReason)
{
    if (infoPtr->nHotItem != nHit)
    {
        NMTBHOTITEM nmhotitem;
        TBUTTON_INFO *btnPtr = NULL, *oldBtnPtr = NULL;

        nmhotitem.dwFlags = dwReason;
        if(infoPtr->nHotItem >= 0)
        {
            oldBtnPtr = &infoPtr->buttons[infoPtr->nHotItem];
            nmhotitem.idOld = oldBtnPtr->idCommand;
        }
        else
        {
            nmhotitem.dwFlags |= HICF_ENTERING;
            nmhotitem.idOld = 0;
        }

        if (nHit >= 0)
        {
            btnPtr = &infoPtr->buttons[nHit];
            nmhotitem.idNew = btnPtr->idCommand;
        }
	else
	{
	    nmhotitem.dwFlags |= HICF_LEAVING;
	    nmhotitem.idNew = 0;
	}

	/* now change the hot and invalidate the old and new buttons - if the
	 * parent agrees */
	if (!TOOLBAR_SendNotify(&nmhotitem.hdr, infoPtr, TBN_HOTITEMCHANGE))
	{
            if (oldBtnPtr) {
                oldBtnPtr->bHot = FALSE;
                InvalidateRect(infoPtr->hwndSelf, &oldBtnPtr->rect, TRUE);
            }
            /* setting disabled buttons as hot fails even if the notify contains the button id */
            if (btnPtr && (btnPtr->fsState & TBSTATE_ENABLED)) {
                btnPtr->bHot = TRUE;
                InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
                infoPtr->nHotItem = nHit;
            }
            else
                infoPtr->nHotItem = -1;            
        }
    }
}

static LRESULT
TOOLBAR_SetHotItem (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    INT nOldHotItem = infoPtr->nHotItem;

    TRACE("hwnd = %p, nHit = %d\n", hwnd, (INT)wParam);

    if ((INT)wParam >= infoPtr->nNumButtons)
        return infoPtr->nHotItem;
    
    if ((INT)wParam < 0)
        wParam = -1;

    /* NOTE: an application can still remove the hot item even if anchor
     * highlighting is enabled */

    TOOLBAR_SetHotItemEx(infoPtr, wParam, HICF_OTHER);

    if (nOldHotItem < 0)
        return -1;

    return (LRESULT)nOldHotItem;
}


static LRESULT
TOOLBAR_SetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    HIMAGELIST himlTemp;
    HIMAGELIST himl = (HIMAGELIST)lParam;
    INT oldButtonWidth = infoPtr->nButtonWidth;
    INT oldBitmapWidth = infoPtr->nBitmapWidth;
    INT oldBitmapHeight = infoPtr->nBitmapHeight;
    INT i, id = 0;

    if (infoPtr->iVersion >= 5)
        id = wParam;

    himlTemp = TOOLBAR_InsertImageList(&infoPtr->himlDef, 
        &infoPtr->cimlDef, himl, id);

    infoPtr->nNumBitmaps = 0;
    for (i = 0; i < infoPtr->cimlDef; i++)
        infoPtr->nNumBitmaps += ImageList_GetImageCount(infoPtr->himlDef[i]->himl);

    if (!ImageList_GetIconSize(himl, &infoPtr->nBitmapWidth,
            &infoPtr->nBitmapHeight))
    {
        infoPtr->nBitmapWidth = 1;
        infoPtr->nBitmapHeight = 1;
    }
    if ((oldBitmapWidth != infoPtr->nBitmapWidth) || (oldBitmapHeight != infoPtr->nBitmapHeight))
    {
        TOOLBAR_CalcToolbar(hwnd);
        if (infoPtr->nButtonWidth < oldButtonWidth)
            TOOLBAR_SetButtonSize(hwnd, MAKELONG(oldButtonWidth, infoPtr->nButtonHeight));
    }

    TRACE("hwnd %p, new himl=%p, id = %d, count=%d, bitmap w=%d, h=%d\n",
	  hwnd, infoPtr->himlDef, id, infoPtr->nNumBitmaps,
	  infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);

    InvalidateRect(hwnd, NULL, TRUE);

    return (LRESULT)himlTemp;
}


static LRESULT
TOOLBAR_SetIndent (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    infoPtr->nIndent = (INT)wParam;

    TRACE("\n");

    /* process only on indent changing */
    if(infoPtr->nIndent != (INT)wParam)
    {
        infoPtr->nIndent = (INT)wParam;
        TOOLBAR_CalcToolbar (hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
    }

    return TRUE;
}


static LRESULT
TOOLBAR_SetInsertMark (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBINSERTMARK *lptbim = (TBINSERTMARK*)lParam;

    TRACE("hwnd = %p, lptbim = { %d, 0x%08x}\n", hwnd, lptbim->iButton, lptbim->dwFlags);

    if ((lptbim->dwFlags & ~TBIMHT_AFTER) != 0)
    {
        FIXME("Unrecognized flag(s): 0x%08x\n", (lptbim->dwFlags & ~TBIMHT_AFTER));
        return 0;
    }

    if ((lptbim->iButton == -1) || 
        ((lptbim->iButton < infoPtr->nNumButtons) &&
         (lptbim->iButton >= 0)))
    {
        infoPtr->tbim = *lptbim;
        /* FIXME: don't need to update entire toolbar */
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else
        ERR("Invalid button index %d\n", lptbim->iButton);

    return 0;
}


static LRESULT
TOOLBAR_SetInsertMarkColor (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    infoPtr->clrInsertMark = (COLORREF)lParam;

    /* FIXME: don't need to update entire toolbar */
    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
}


static LRESULT
TOOLBAR_SetMaxTextRows (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    infoPtr->nMaxTextRows = (INT)wParam;

    TOOLBAR_CalcToolbar(hwnd);
    return TRUE;
}


/* MSDN gives slightly wrong info on padding.
 * 1. It is not only used on buttons with the BTNS_AUTOSIZE style
 * 2. It is not used to create a blank area between the edge of the button
 *    and the text or image if TBSTYLE_LIST is set. It is used to control
 *    the gap between the image and text. 
 * 3. It is not applied to both sides. If TBSTYLE_LIST is set it is used 
 *    to control the bottom and right borders [with the border being
 *    szPadding.cx - (GetSystemMetrics(SM_CXEDGE)+1)], otherwise the padding
 *    is shared evenly on both sides of the button.
 * See blueprints in comments above TOOLBAR_MeasureButton for more info.
 */
static LRESULT
TOOLBAR_SetPadding (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD  oldPad;

    oldPad = MAKELONG(infoPtr->szPadding.cx, infoPtr->szPadding.cy);
    infoPtr->szPadding.cx = min(LOWORD((DWORD)lParam), GetSystemMetrics(SM_CXEDGE));
    infoPtr->szPadding.cy = min(HIWORD((DWORD)lParam), GetSystemMetrics(SM_CYEDGE));
    TRACE("cx=%d, cy=%d\n",
	  infoPtr->szPadding.cx, infoPtr->szPadding.cy);
    return (LRESULT) oldPad;
}


static LRESULT
TOOLBAR_SetParent (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    HWND hwndOldNotify;

    TRACE("\n");

    hwndOldNotify = infoPtr->hwndNotify;
    infoPtr->hwndNotify = (HWND)wParam;

    return (LRESULT)hwndOldNotify;
}


static LRESULT
TOOLBAR_SetRows (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPRECT lprc = (LPRECT)lParam;
    int rows = LOWORD(wParam);
    BOOL bLarger = HIWORD(wParam);

    TRACE("\n");

    TRACE("Setting rows to %d (%d)\n", rows, bLarger);

    if(infoPtr->nRows != rows)
    {
        TBUTTON_INFO *btnPtr = infoPtr->buttons;
        int curColumn = 0; /* Current column                      */
        int curRow    = 0; /* Current row                         */
        int hidden    = 0; /* Number of hidden buttons */
        int seps      = 0; /* Number of separators     */
        int idealWrap = 0; /* Ideal wrap point         */
        int i;
        BOOL wrap;

        /*
           Calculate new size and wrap points - Under windows, setrows will
           change the dimensions if needed to show the number of requested
           rows (if CCS_NORESIZE is set), or will take up the whole window
           (if no CCS_NORESIZE).

           Basic algorithm - If N buttons, and y rows requested, each row
           contains N/y buttons.

           FIXME: Handling of separators not obvious from testing results
           FIXME: Take width of window into account?
         */

        /* Loop through the buttons one by one counting key items  */
        for (i = 0; i < infoPtr->nNumButtons; i++ )
        {
            btnPtr[i].fsState &= ~TBSTATE_WRAP;
            if (btnPtr[i].fsState & TBSTATE_HIDDEN)
                hidden++;
            else if (btnPtr[i].fsStyle & BTNS_SEP)
                seps++;
        }

        /* FIXME: Separators make this quite complex */
        if (seps) FIXME("Separators unhandled\n");

        /* Round up so more per line, i.e., less rows */
        idealWrap = (infoPtr->nNumButtons - hidden + (rows-1)) / rows;

        /* Calculate ideal wrap point if we are allowed to grow, but cannot
           achieve the requested number of rows. */
        if (bLarger && idealWrap > 1)
        {
            int resRows = (infoPtr->nNumButtons + (idealWrap-1)) / idealWrap;
            int moreRows = (infoPtr->nNumButtons + (idealWrap-2)) / (idealWrap-1);

            if (resRows < rows && moreRows > rows)
            {
                idealWrap--;
                TRACE("Changing idealWrap due to bLarger (now %d)\n", idealWrap);
            }
        }

        curColumn = curRow = 0;
        wrap = FALSE;
        TRACE("Trying to wrap at %d (%d,%d,%d)\n", idealWrap,
              infoPtr->nNumButtons, hidden, rows);

        for (i = 0; i < infoPtr->nNumButtons; i++ )
        {
            if (btnPtr[i].fsState & TBSTATE_HIDDEN)
                continue;

            /* Step on, wrap if necessary or flag next to wrap */
            if (!wrap) {
                curColumn++;
            } else {
                wrap = FALSE;
                curColumn = 1;
                curRow++;
            }

            if (curColumn > (idealWrap-1)) {
                wrap = TRUE;
                btnPtr[i].fsState |= TBSTATE_WRAP;
            }
        }

        TRACE("Result - %d rows\n", curRow + 1);

        /* recalculate toolbar */
        TOOLBAR_CalcToolbar (hwnd);

        /* Resize if necessary (Only if NORESIZE is set - odd, but basically
           if NORESIZE is NOT set, then the toolbar will always be resized to
           take up the whole window. With it set, sizing needs to be manual. */
        if (infoPtr->dwStyle & CCS_NORESIZE) {
            SetWindowPos(hwnd, NULL, 0, 0,
                         infoPtr->rcBound.right - infoPtr->rcBound.left,
                         infoPtr->rcBound.bottom - infoPtr->rcBound.top,
                         SWP_NOMOVE);
        }

        /* repaint toolbar */
        InvalidateRect(hwnd, NULL, TRUE);
    }

    /* return bounding rectangle */
    if (lprc) {
	lprc->left   = infoPtr->rcBound.left;
	lprc->right  = infoPtr->rcBound.right;
	lprc->top    = infoPtr->rcBound.top;
	lprc->bottom = infoPtr->rcBound.bottom;
    }

    return 0;
}


static LRESULT
TOOLBAR_SetState (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    /* if hidden state has changed the invalidate entire window and recalc */
    if ((btnPtr->fsState & TBSTATE_HIDDEN) != (LOWORD(lParam) & TBSTATE_HIDDEN)) {
	btnPtr->fsState = LOWORD(lParam);
	TOOLBAR_CalcToolbar (hwnd);
	InvalidateRect(hwnd, 0, TRUE);
	return TRUE;
    }

    /* process state changing if current state doesn't match new state */
    if(btnPtr->fsState != LOWORD(lParam))
    {
        btnPtr->fsState = LOWORD(lParam);
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);
    }

    return TRUE;
}


static LRESULT
TOOLBAR_SetStyle (HWND hwnd, LPARAM lParam)
{
    SetWindowLongW(hwnd, GWL_STYLE, lParam);

    return TRUE;
}


static inline LRESULT
TOOLBAR_SetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("hwnd=%p, hwndTooltip=%p, lParam=0x%lx\n", hwnd, (HWND)wParam, lParam);

    infoPtr->hwndToolTip = (HWND)wParam;
    return 0;
}


static LRESULT
TOOLBAR_SetUnicodeFormat (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    BOOL bTemp;

    TRACE("%s hwnd=%p\n",
	   ((BOOL)wParam) ? "TRUE" : "FALSE", hwnd);

    bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL)wParam;

    return bTemp;
}


static LRESULT
TOOLBAR_GetColorScheme (HWND hwnd, LPCOLORSCHEME lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    lParam->clrBtnHighlight = (infoPtr->clrBtnHighlight == CLR_DEFAULT) ?
	                       comctl32_color.clrBtnHighlight :
                               infoPtr->clrBtnHighlight;
    lParam->clrBtnShadow = (infoPtr->clrBtnShadow == CLR_DEFAULT) ?
	                   comctl32_color.clrBtnShadow : infoPtr->clrBtnShadow;
    return 1;
}


static LRESULT
TOOLBAR_SetColorScheme (HWND hwnd, const COLORSCHEME *lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("new colors Hl=%x Shd=%x, old colors Hl=%x Shd=%x\n",
	  lParam->clrBtnHighlight, lParam->clrBtnShadow,
	  infoPtr->clrBtnHighlight, infoPtr->clrBtnShadow);

    infoPtr->clrBtnHighlight = lParam->clrBtnHighlight;
    infoPtr->clrBtnShadow = lParam->clrBtnShadow;
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;
}


static LRESULT
TOOLBAR_SetVersion (HWND hwnd, INT iVersion)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT iOldVersion = infoPtr->iVersion;

    infoPtr->iVersion = iVersion;

    if (infoPtr->iVersion >= 5)
        TOOLBAR_SetUnicodeFormat(hwnd, TRUE);

    return iOldVersion;
}


static LRESULT
TOOLBAR_GetStringA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    WORD iString = HIWORD(wParam);
    WORD buffersize = LOWORD(wParam);
    LPSTR str = (LPSTR)lParam;
    LRESULT ret = -1;

    TRACE("hwnd=%p, iString=%d, buffersize=%d, string=%p\n", hwnd, iString, buffersize, str);

    if (iString < infoPtr->nNumStrings)
    {
        ret = WideCharToMultiByte(CP_ACP, 0, infoPtr->strings[iString], -1, str, buffersize, NULL, NULL);
        ret--;

        TRACE("returning %s\n", debugstr_a(str));
    }
    else
        WARN("String index %d out of range (largest is %d)\n", iString, infoPtr->nNumStrings - 1);

    return ret;
}


static LRESULT
TOOLBAR_GetStringW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    WORD iString = HIWORD(wParam);
    WORD len = LOWORD(wParam)/sizeof(WCHAR) - 1;
    LPWSTR str = (LPWSTR)lParam;
    LRESULT ret = -1;

    TRACE("hwnd=%p, iString=%d, buffersize=%d, string=%p\n", hwnd, iString, LOWORD(wParam), str);

    if (iString < infoPtr->nNumStrings)
    {
        len = min(len, strlenW(infoPtr->strings[iString]));
        ret = (len+1)*sizeof(WCHAR);
        if (str)
        {
            memcpy(str, infoPtr->strings[iString], ret);
            str[len] = '\0';
        }
        ret = len;

        TRACE("returning %s\n", debugstr_w(str));
    }
    else
        WARN("String index %d out of range (largest is %d)\n", iString, infoPtr->nNumStrings - 1);

    return ret;
}

/* UNDOCUMENTED MESSAGE: This appears to set some kind of size. Perhaps it
 * is the maximum size of the toolbar? */
static LRESULT TOOLBAR_Unkwn45D(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    SIZE * pSize = (SIZE*)lParam;
    FIXME("hwnd=%p, wParam=0x%08lx, size.cx=%d, size.cy=%d stub!\n", hwnd, wParam, pSize->cx, pSize->cy);
    return 0;
}


/* This is an extended version of the TB_SETHOTITEM message. It allows the
 * caller to specify a reason why the hot item changed (rather than just the
 * HICF_OTHER that TB_SETHOTITEM sends). */
static LRESULT
TOOLBAR_SetHotItem2 (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    INT nOldHotItem = infoPtr->nHotItem;

    TRACE("old item=%d, new item=%d, flags=%08x\n",
	  nOldHotItem, infoPtr->nHotItem, (DWORD)lParam);

    if ((INT) wParam < 0 || (INT)wParam > infoPtr->nNumButtons)
        wParam = -1;

    /* NOTE: an application can still remove the hot item even if anchor
     * highlighting is enabled */

    TOOLBAR_SetHotItemEx(infoPtr, wParam, lParam);

    GetFocus();

    return (nOldHotItem < 0) ? -1 : (LRESULT)nOldHotItem;
}

/* Sets the toolbar global iListGap parameter which controls the amount of
 * spacing between the image and the text of buttons for TBSTYLE_LIST
 * toolbars. */
static LRESULT TOOLBAR_SetListGap(HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);

    TRACE("hwnd=%p iListGap=%ld\n", hwnd, wParam);
    
    infoPtr->iListGap = (INT)wParam;

    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
}

/* Returns the number of maximum number of image lists associated with the
 * various states. */
static LRESULT TOOLBAR_GetImageListCount(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);

    TRACE("hwnd=%p wParam %08lx lParam %08lx\n", hwnd, wParam, lParam);

    return max(infoPtr->cimlDef, max(infoPtr->cimlHot, infoPtr->cimlDis));
}

static LRESULT
TOOLBAR_GetIdealSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPSIZE lpsize = (LPSIZE)lParam;

    if (lpsize == NULL)
	return FALSE;

    /*
     * Testing shows the following:
     *   wParam    = 0 adjust cx value
     *             = 1 set cy value to max size.
     *   lParam    pointer to SIZE structure
     *
     */
    TRACE("wParam %ld, lParam 0x%08lx -> 0x%08x 0x%08x\n",
	  wParam, lParam, lpsize->cx, lpsize->cy);

    switch(wParam) {
    case 0:
	if (lpsize->cx == -1) {
	    /* **** this is wrong, native measures each button and sets it */
	    lpsize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
	}
	else if(HIWORD(lpsize->cx)) {
	    RECT rc;
	    HWND hwndParent = GetParent(hwnd);

	    GetWindowRect(hwnd, &rc);
	    MapWindowPoints(0, hwndParent, (LPPOINT)&rc, 2);
            TRACE("mapped to (%s)\n", wine_dbgstr_rect(&rc));
	    lpsize->cx = max(rc.right-rc.left,
			     infoPtr->rcBound.right - infoPtr->rcBound.left);
	}
	else {
	    lpsize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
	}
	break;
    case 1:
	lpsize->cy = infoPtr->rcBound.bottom - infoPtr->rcBound.top;
	break;
    default:
	FIXME("Unknown wParam %ld\n", wParam);
	return 0;
    }
    TRACE("set to -> 0x%08x 0x%08x\n",
	  lpsize->cx, lpsize->cy);
    return 1;
}

static LRESULT TOOLBAR_Unkwn464(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    FIXME("hwnd=%p wParam %08lx lParam %08lx\n", hwnd, wParam, lParam);

    InvalidateRect(hwnd, NULL, TRUE);
    return 1;
}


static LRESULT
TOOLBAR_Create (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongW (hwnd, GWL_STYLE);
    LOGFONTW logFont;

    TRACE("hwnd = %p\n", hwnd);

    /* initialize info structure */
    infoPtr->nButtonWidth = 23;
    infoPtr->nButtonHeight = 22;
    infoPtr->nBitmapHeight = 16;
    infoPtr->nBitmapWidth = 16;

    infoPtr->nMaxTextRows = 1;
    infoPtr->cxMin = -1;
    infoPtr->cxMax = -1;
    infoPtr->nNumBitmaps = 0;
    infoPtr->nNumStrings = 0;

    infoPtr->bCaptured = FALSE;
    infoPtr->nButtonDown = -1;
    infoPtr->nButtonDrag = -1;
    infoPtr->nOldHit = -1;
    infoPtr->nHotItem = -1;
    infoPtr->hwndNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;
    infoPtr->dwDTFlags = (dwStyle & TBSTYLE_LIST) ? DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS: DT_CENTER | DT_END_ELLIPSIS;
    infoPtr->bAnchor = FALSE; /* no anchor highlighting */
    infoPtr->bDragOutSent = FALSE;
    infoPtr->iVersion = 0;
    infoPtr->hwndSelf = hwnd;
    infoPtr->bDoRedraw = TRUE;
    infoPtr->clrBtnHighlight = CLR_DEFAULT;
    infoPtr->clrBtnShadow = CLR_DEFAULT;
    infoPtr->szPadding.cx = DEFPAD_CX;
    infoPtr->szPadding.cy = DEFPAD_CY;
    infoPtr->iListGap = DEFLISTGAP;
    infoPtr->iTopMargin = default_top_margin(infoPtr);
    infoPtr->dwStyle = dwStyle;
    infoPtr->tbim.iButton = -1;
    GetClientRect(hwnd, &infoPtr->client_rect);
    infoPtr->bUnicode = infoPtr->hwndNotify && 
        (NFR_UNICODE == SendMessageW(hwnd, WM_NOTIFYFORMAT, (WPARAM)hwnd, (LPARAM)NF_REQUERY));
    infoPtr->hwndToolTip = NULL; /* if needed the tooltip control will be created after a WM_MOUSEMOVE */

    SystemParametersInfoW (SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
    infoPtr->hFont = infoPtr->hDefaultFont = CreateFontIndirectW (&logFont);
    
    OpenThemeData (hwnd, themeClass);

    TOOLBAR_CheckStyle (hwnd, dwStyle);

    return 0;
}


static LRESULT
TOOLBAR_Destroy (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    /* delete tooltip control */
    if (infoPtr->hwndToolTip)
	DestroyWindow (infoPtr->hwndToolTip);

    /* delete temporary buffer for tooltip text */
    Free (infoPtr->pszTooltipText);
    Free (infoPtr->bitmaps);            /* bitmaps list */

    /* delete button data */
    Free (infoPtr->buttons);

    /* delete strings */
    if (infoPtr->strings) {
	INT i;
	for (i = 0; i < infoPtr->nNumStrings; i++)
	    Free (infoPtr->strings[i]);

	Free (infoPtr->strings);
    }

    /* destroy internal image list */
    if (infoPtr->himlInt)
	ImageList_Destroy (infoPtr->himlInt);

	TOOLBAR_DeleteImageList(&infoPtr->himlDef, &infoPtr->cimlDef);
	TOOLBAR_DeleteImageList(&infoPtr->himlDis, &infoPtr->cimlDis);
	TOOLBAR_DeleteImageList(&infoPtr->himlHot, &infoPtr->cimlHot);

    /* delete default font */
    DeleteObject (infoPtr->hDefaultFont);
        
    CloseThemeData (GetWindowTheme (hwnd));

    /* free toolbar info data */
    Free (infoPtr);
    SetWindowLongPtrW (hwnd, 0, 0);

    return 0;
}


static LRESULT
TOOLBAR_EraseBackground (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    NMTBCUSTOMDRAW tbcd;
    INT ret = FALSE;
    DWORD ntfret;
    HTHEME theme = GetWindowTheme (hwnd);
    DWORD dwEraseCustDraw = 0;

    /* the app has told us not to redraw the toolbar */
    if (!infoPtr->bDoRedraw)
        return FALSE;

    if (infoPtr->dwStyle & TBSTYLE_CUSTOMERASE) {
	ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
	tbcd.nmcd.dwDrawStage = CDDS_PREERASE;
	tbcd.nmcd.hdc = (HDC)wParam;
	ntfret = TOOLBAR_SendNotify (&tbcd.nmcd.hdr, infoPtr, NM_CUSTOMDRAW);
	dwEraseCustDraw = ntfret & 0xffff;

	/* FIXME: in general the return flags *can* be or'ed together */
	switch (dwEraseCustDraw)
	    {
	    case CDRF_DODEFAULT:
		break;
	    case CDRF_SKIPDEFAULT:
		return TRUE;
	    default:
		FIXME("[%p] response %d not handled to NM_CUSTOMDRAW (CDDS_PREERASE)\n",
		      hwnd, ntfret);
	    }
    }

    /* If the toolbar is "transparent" then pass the WM_ERASEBKGND up
     * to my parent for processing.
     */
    if (theme || (infoPtr->dwStyle & TBSTYLE_TRANSPARENT)) {
	POINT pt, ptorig;
	HDC hdc = (HDC)wParam;
	HWND parent;

	pt.x = 0;
	pt.y = 0;
	parent = GetParent(hwnd);
	MapWindowPoints(hwnd, parent, &pt, 1);
	OffsetWindowOrgEx (hdc, pt.x, pt.y, &ptorig);
	ret = SendMessageW (parent, WM_ERASEBKGND, wParam, lParam);
	SetWindowOrgEx (hdc, ptorig.x, ptorig.y, 0);
    }
    if (!ret)
	ret = DefWindowProcW (hwnd, WM_ERASEBKGND, wParam, lParam);

    if (dwEraseCustDraw & CDRF_NOTIFYPOSTERASE) {
	ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
	tbcd.nmcd.dwDrawStage = CDDS_POSTERASE;
	tbcd.nmcd.hdc = (HDC)wParam;
	ntfret = TOOLBAR_SendNotify (&tbcd.nmcd.hdr, infoPtr, NM_CUSTOMDRAW);
	dwEraseCustDraw = ntfret & 0xffff;
	switch (dwEraseCustDraw)
	    {
	    case CDRF_DODEFAULT:
		break;
	    case CDRF_SKIPDEFAULT:
		return TRUE;
	    default:
		FIXME("[%p] response %d not handled to NM_CUSTOMDRAW (CDDS_POSTERASE)\n",
		      hwnd, ntfret);
	    }
    }
    return ret;
}


static LRESULT
TOOLBAR_GetFont (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


static void
TOOLBAR_SetRelativeHotItem(TOOLBAR_INFO *infoPtr, INT iDirection, DWORD dwReason)
{
    INT i;
    INT nNewHotItem = infoPtr->nHotItem;

    for (i = 0; i < infoPtr->nNumButtons; i++)
    {
        /* did we wrap? */
        if ((nNewHotItem + iDirection < 0) ||
            (nNewHotItem + iDirection >= infoPtr->nNumButtons))
        {
            NMTBWRAPHOTITEM nmtbwhi;
            nmtbwhi.idNew = infoPtr->buttons[nNewHotItem].idCommand;
            nmtbwhi.iDirection = iDirection;
            nmtbwhi.dwReason = dwReason;
    
            if (TOOLBAR_SendNotify(&nmtbwhi.hdr, infoPtr, TBN_WRAPHOTITEM))
                return;
        }

        nNewHotItem += iDirection;
        nNewHotItem = (nNewHotItem + infoPtr->nNumButtons) % infoPtr->nNumButtons;

        if ((infoPtr->buttons[nNewHotItem].fsState & TBSTATE_ENABLED) &&
            !(infoPtr->buttons[nNewHotItem].fsStyle & BTNS_SEP))
        {
            TOOLBAR_SetHotItemEx(infoPtr, nNewHotItem, dwReason);
            break;
        }
    }
}

static LRESULT
TOOLBAR_KeyDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    NMKEY nmkey;

    nmkey.nVKey = (UINT)wParam;
    nmkey.uFlags = HIWORD(lParam);

    if (TOOLBAR_SendNotify(&nmkey.hdr, infoPtr, NM_KEYDOWN))
        return DefWindowProcW(hwnd, WM_KEYDOWN, wParam, lParam);

    switch ((UINT)wParam)
    {
    case VK_LEFT:
    case VK_UP:
        TOOLBAR_SetRelativeHotItem(infoPtr, -1, HICF_ARROWKEYS);
        break;
    case VK_RIGHT:
    case VK_DOWN:
        TOOLBAR_SetRelativeHotItem(infoPtr, 1, HICF_ARROWKEYS);
        break;
    case VK_SPACE:
    case VK_RETURN:
        if ((infoPtr->nHotItem >= 0) &&
            (infoPtr->buttons[infoPtr->nHotItem].fsState & TBSTATE_ENABLED))
        {
            SendMessageW (infoPtr->hwndNotify, WM_COMMAND,
                MAKEWPARAM(infoPtr->buttons[infoPtr->nHotItem].idCommand, BN_CLICKED),
                (LPARAM)hwnd);
        }
        break;
    }

    return 0;
}


static LRESULT
TOOLBAR_LButtonDblClk (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    INT   nHit;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    if (nHit >= 0)
        TOOLBAR_LButtonDown (hwnd, wParam, lParam);
    else if (GetWindowLongW (hwnd, GWL_STYLE) & CCS_ADJUSTABLE)
	TOOLBAR_Customize (hwnd);

    return 0;
}


static LRESULT
TOOLBAR_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    POINT pt;
    INT   nHit;
    NMTOOLBARA nmtb;
    NMMOUSE nmmouse;
    BOOL bDragKeyPressed;

    TRACE("\n");

    if (infoPtr->dwStyle & TBSTYLE_ALTDRAG)
        bDragKeyPressed = (GetKeyState(VK_MENU) < 0);
    else
        bDragKeyPressed = (wParam & MK_SHIFT);

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, hwnd,
			    WM_LBUTTONDOWN, wParam, lParam);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    btnPtr = &infoPtr->buttons[nHit];

    if ((nHit >= 0) && bDragKeyPressed && (infoPtr->dwStyle & CCS_ADJUSTABLE))
    {
        infoPtr->nButtonDrag = nHit;
        SetCapture (hwnd);
        
        /* If drag cursor has not been loaded, load it.
         * Note: it doesn't need to be freed */
        if (!hCursorDrag)
            hCursorDrag = LoadCursorW(COMCTL32_hModule, (LPCWSTR)IDC_MOVEBUTTON);
        SetCursor(hCursorDrag);
    }
    else if (nHit >= 0)
    {
	RECT arrowRect;
	infoPtr->nOldHit = nHit;

	CopyRect(&arrowRect, &btnPtr->rect);
	arrowRect.left = max(btnPtr->rect.left, btnPtr->rect.right - DDARROW_WIDTH);

	/* for EX_DRAWDDARROWS style,  click must be in the drop-down arrow rect */
	if ((btnPtr->fsState & TBSTATE_ENABLED) && 
	     ((btnPtr->fsStyle & BTNS_WHOLEDROPDOWN) ||
	      ((btnPtr->fsStyle & BTNS_DROPDOWN) &&
	       ((TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle) && PtInRect(&arrowRect, pt)) ||
	       (!TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle))))))
	{
	    LRESULT res;

	    /* draw in pressed state */
	    if (btnPtr->fsStyle & BTNS_WHOLEDROPDOWN)
	        btnPtr->fsState |= TBSTATE_PRESSED;
	    else
	        btnPtr->bDropDownPressed = TRUE;
	    RedrawWindow(hwnd,&btnPtr->rect,0,
			RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);

	    memset(&nmtb, 0, sizeof(nmtb));
	    nmtb.iItem = btnPtr->idCommand;
	    nmtb.rcButton = btnPtr->rect;
	    res = TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
				  TBN_DROPDOWN);
	    TRACE("TBN_DROPDOWN responded with %ld\n", res);

            if (res != TBDDRET_TREATPRESSED)
            {
                MSG msg;

                /* redraw button in unpressed state */
	        if (btnPtr->fsStyle & BTNS_WHOLEDROPDOWN)
       	            btnPtr->fsState &= ~TBSTATE_PRESSED;
       	        else
       	            btnPtr->bDropDownPressed = FALSE;
       	        InvalidateRect(hwnd, &btnPtr->rect, TRUE);

                /* find and set hot item
                 * NOTE: native doesn't do this, but that is a bug */
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                nHit = TOOLBAR_InternalHitTest(hwnd, &pt);
                if (!infoPtr->bAnchor || (nHit >= 0))
                    TOOLBAR_SetHotItemEx(infoPtr, nHit, HICF_MOUSE | HICF_LMOUSE);
                
                /* remove any left mouse button down or double-click messages
                 * so that we can get a toggle effect on the button */
                while (PeekMessageW(&msg, hwnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE) ||
                       PeekMessageW(&msg, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, PM_REMOVE))
                    ;

		return 0;
            }
	    /* otherwise drop through and process as pushed */
       	}
	infoPtr->bCaptured = TRUE;
	infoPtr->nButtonDown = nHit;
	infoPtr->bDragOutSent = FALSE;

	btnPtr->fsState |= TBSTATE_PRESSED;

        TOOLBAR_SetHotItemEx(infoPtr, nHit, HICF_MOUSE | HICF_LMOUSE);

        if (btnPtr->fsState & TBSTATE_ENABLED)
	    InvalidateRect(hwnd, &btnPtr->rect, TRUE);
	UpdateWindow(hwnd);
	SetCapture (hwnd);
    }

    if (nHit >=0)
    {
        memset(&nmtb, 0, sizeof(nmtb));
        nmtb.iItem = btnPtr->idCommand;
        TOOLBAR_SendNotify((NMHDR *)&nmtb, infoPtr, TBN_BEGINDRAG);
    }

    nmmouse.dwHitInfo = nHit;

    /* !!! Undocumented - sends NM_LDOWN with the NMMOUSE structure. */
    if (nHit < 0)
        nmmouse.dwItemSpec = -1;
    else
    {
        nmmouse.dwItemSpec = infoPtr->buttons[nmmouse.dwHitInfo].idCommand;
        nmmouse.dwItemData = infoPtr->buttons[nmmouse.dwHitInfo].dwData;
    }

    ClientToScreen(hwnd, &pt); 
    nmmouse.pt = pt;

    if (!TOOLBAR_SendNotify(&nmmouse.hdr, infoPtr, NM_LDOWN))
        return DefWindowProcW(hwnd, WM_LBUTTONDOWN, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    POINT pt;
    INT   nHit;
    INT   nOldIndex = -1;
    NMHDR hdr;
    NMMOUSE nmmouse;
    NMTOOLBARA nmtb;

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, hwnd,
			    WM_LBUTTONUP, wParam, lParam);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    if (!infoPtr->bAnchor || (nHit >= 0))
        TOOLBAR_SetHotItemEx(infoPtr, nHit, HICF_MOUSE | HICF_LMOUSE);

    if (infoPtr->nButtonDrag >= 0) {
        RECT rcClient;
        NMHDR hdr;

        btnPtr = &infoPtr->buttons[infoPtr->nButtonDrag];
        ReleaseCapture();
        /* reset cursor */
        SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_ARROW));

        GetClientRect(hwnd, &rcClient);
        if (PtInRect(&rcClient, pt))
        {
            INT nButton = -1;
            if (nHit >= 0)
                nButton = nHit;
            else if (nHit < -1)
                nButton = -nHit;
            else if ((nHit == -1) && PtInRect(&infoPtr->buttons[-nHit].rect, pt))
                nButton = -nHit;

            if (nButton == infoPtr->nButtonDrag)
            {
                /* if the button is moved sightly left and we have a
                 * separator there then remove it */
                if (pt.x < (btnPtr->rect.left + (btnPtr->rect.right - btnPtr->rect.left)/2))
                {
                    if ((nButton > 0) && (infoPtr->buttons[nButton-1].fsStyle & BTNS_SEP))
                        TOOLBAR_DeleteButton(hwnd, nButton - 1);
                }
                else /* else insert a separator before the dragged button */
                {
                    TBBUTTON tbb;
                    memset(&tbb, 0, sizeof(tbb));
                    tbb.fsStyle = BTNS_SEP;
                    tbb.iString = -1;
                    TOOLBAR_InsertButtonT(hwnd, nButton, (LPARAM)&tbb, TRUE);
                }
            }
            else
            {
                if (nButton == -1)
                {
                    if ((infoPtr->nNumButtons > 0) && (pt.x < infoPtr->buttons[0].rect.left))
                        TOOLBAR_MoveButton(hwnd, infoPtr->nButtonDrag, 0);
                    else
                        TOOLBAR_MoveButton(hwnd, infoPtr->nButtonDrag, infoPtr->nNumButtons);
                }
                else
                    TOOLBAR_MoveButton(hwnd, infoPtr->nButtonDrag, nButton);
            }
        }
        else
        {
            TRACE("button %d dragged out of toolbar\n", infoPtr->nButtonDrag);
            TOOLBAR_DeleteButton(hwnd, (WPARAM)infoPtr->nButtonDrag);
        }

        /* button under cursor changed so need to re-set hot item */
        TOOLBAR_SetHotItemEx(infoPtr, nHit, HICF_MOUSE | HICF_LMOUSE);
        infoPtr->nButtonDrag = -1;

        TOOLBAR_SendNotify(&hdr, infoPtr, TBN_TOOLBARCHANGE);
    }
    else if (infoPtr->nButtonDown >= 0) {
	btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
	btnPtr->fsState &= ~TBSTATE_PRESSED;

	if (btnPtr->fsStyle & BTNS_CHECK) {
		if (btnPtr->fsStyle & BTNS_GROUP) {
		    nOldIndex = TOOLBAR_GetCheckedGroupButtonIndex (infoPtr,
			nHit);
		    if ((nOldIndex != nHit) &&
			(nOldIndex != -1))
			infoPtr->buttons[nOldIndex].fsState &= ~TBSTATE_CHECKED;
		    btnPtr->fsState |= TBSTATE_CHECKED;
		}
		else {
		    if (btnPtr->fsState & TBSTATE_CHECKED)
			btnPtr->fsState &= ~TBSTATE_CHECKED;
		    else
			btnPtr->fsState |= TBSTATE_CHECKED;
		}
	}

        if (nOldIndex != -1)
            InvalidateRect(hwnd, &infoPtr->buttons[nOldIndex].rect, TRUE);

	/*
	 * now we can ReleaseCapture, which triggers CAPTURECHANGED msg,
	 * that resets bCaptured and btn TBSTATE_PRESSED flags,
	 * and obliterates nButtonDown and nOldHit (see TOOLBAR_CaptureChanged)
	 */
	if ((infoPtr->bCaptured) && (infoPtr->nButtonDown >= 0))
	    ReleaseCapture ();
	infoPtr->nButtonDown = -1;

	/* Issue NM_RELEASEDCAPTURE to parent to let him know it is released */
	TOOLBAR_SendNotify (&hdr, infoPtr,
			NM_RELEASEDCAPTURE);

	/* native issues TBN_ENDDRAG here, if _LBUTTONDOWN issued the
	 * TBN_BEGINDRAG
	 */
	memset(&nmtb, 0, sizeof(nmtb));
	nmtb.iItem = btnPtr->idCommand;
	TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
			TBN_ENDDRAG);

	if (btnPtr->fsState & TBSTATE_ENABLED)
	{
	    SendMessageW (infoPtr->hwndNotify, WM_COMMAND,
	      MAKEWPARAM(infoPtr->buttons[nHit].idCommand, BN_CLICKED), (LPARAM)hwnd);

            /* In case we have just been destroyed... */
            if(!IsWindow(hwnd))
                return 0;
        }
    }

    /* !!! Undocumented - toolbar at 4.71 level and above sends
    * NM_CLICK with the NMMOUSE structure. */
    nmmouse.dwHitInfo = nHit;

    if (nHit < 0)
        nmmouse.dwItemSpec = -1;
    else
    {
        nmmouse.dwItemSpec = infoPtr->buttons[nmmouse.dwHitInfo].idCommand;
        nmmouse.dwItemData = infoPtr->buttons[nmmouse.dwHitInfo].dwData;
    }

    ClientToScreen(hwnd, &pt); 
    nmmouse.pt = pt;

    if (!TOOLBAR_SendNotify((LPNMHDR)&nmmouse, infoPtr, NM_CLICK))
        return DefWindowProcW(hwnd, WM_LBUTTONUP, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_RButtonUp( HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nHit;
    NMMOUSE nmmouse;
    POINT pt;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    nHit = TOOLBAR_InternalHitTest(hwnd, &pt);
    nmmouse.dwHitInfo = nHit;

    if (nHit < 0) {
	nmmouse.dwItemSpec = -1;
    } else {
	nmmouse.dwItemSpec = infoPtr->buttons[nmmouse.dwHitInfo].idCommand;
	nmmouse.dwItemData = infoPtr->buttons[nmmouse.dwHitInfo].dwData;
    }

    ClientToScreen(hwnd, &pt); 
    nmmouse.pt = pt;

    if (!TOOLBAR_SendNotify((LPNMHDR)&nmmouse, infoPtr, NM_RCLICK))
        return DefWindowProcW(hwnd, WM_RBUTTONUP, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_RButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    NMHDR nmhdr;

    if (!TOOLBAR_SendNotify(&nmhdr, infoPtr, NM_RDBLCLK))
        return DefWindowProcW(hwnd, WM_RBUTTONDBLCLK, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_CaptureChanged(HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;

    infoPtr->bCaptured = FALSE;

    if (infoPtr->nButtonDown >= 0)
    {
        btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
       	btnPtr->fsState &= ~TBSTATE_PRESSED;

        infoPtr->nOldHit = -1;

        if (btnPtr->fsState & TBSTATE_ENABLED)
            InvalidateRect(hwnd, &btnPtr->rect, TRUE);
    }
    return 0;
}

static LRESULT
TOOLBAR_MouseLeave (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    /* don't remove hot effects when in anchor highlighting mode or when a
     * drop-down button is pressed */
    if (infoPtr->nHotItem >= 0 && !infoPtr->bAnchor)
    {
        TBUTTON_INFO *hotBtnPtr = &infoPtr->buttons[infoPtr->nHotItem];
        if (!hotBtnPtr->bDropDownPressed)
            TOOLBAR_SetHotItemEx(infoPtr, TOOLBAR_NOWHERE, HICF_MOUSE);
    }

    if (infoPtr->nOldHit < 0)
      return TRUE;

    /* If the last button we were over is depressed then make it not */
    /* depressed and redraw it */
    if(infoPtr->nOldHit == infoPtr->nButtonDown)
    {
      TBUTTON_INFO *btnPtr;
      RECT rc1;

      btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];

      btnPtr->fsState &= ~TBSTATE_PRESSED;

      rc1 = btnPtr->rect;
      InflateRect (&rc1, 1, 1);
      InvalidateRect (hwnd, &rc1, TRUE);
    }

    if (infoPtr->bCaptured && !infoPtr->bDragOutSent)
    {
        NMTOOLBARW nmt;
        ZeroMemory(&nmt, sizeof(nmt));
        nmt.iItem = infoPtr->buttons[infoPtr->nButtonDown].idCommand;
        TOOLBAR_SendNotify(&nmt.hdr, infoPtr, TBN_DRAGOUT);
        infoPtr->bDragOutSent = TRUE;
    }

    infoPtr->nOldHit = -1; /* reset the old hit index as we've left the toolbar */

    return TRUE;
}

static LRESULT
TOOLBAR_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    POINT pt;
    TRACKMOUSEEVENT trackinfo;
    INT   nHit;
    TBUTTON_INFO *btnPtr;
    
    if ((infoPtr->dwStyle & TBSTYLE_TOOLTIPS) && (infoPtr->hwndToolTip == NULL))
        TOOLBAR_TooltipCreateControl(infoPtr);
    
    if ((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)) {
        /* fill in the TRACKMOUSEEVENT struct */
        trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
        trackinfo.dwFlags = TME_QUERY;

        /* call _TrackMouseEvent to see if we are currently tracking for this hwnd */
        _TrackMouseEvent(&trackinfo);

        /* Make sure tracking is enabled so we receive a WM_MOUSELEAVE message */
        if(trackinfo.hwndTrack != hwnd || !(trackinfo.dwFlags & TME_LEAVE)) {
            trackinfo.dwFlags = TME_LEAVE; /* notify upon leaving */
            trackinfo.hwndTrack = hwnd;

            /* call TRACKMOUSEEVENT so we receive a WM_MOUSELEAVE message */
            /* and can properly deactivate the hot toolbar button */
            _TrackMouseEvent(&trackinfo);
        }
    }

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, hwnd,
			    WM_MOUSEMOVE, wParam, lParam);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    if (((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)) 
        && (!infoPtr->bAnchor || (nHit >= 0)))
        TOOLBAR_SetHotItemEx(infoPtr, nHit, HICF_MOUSE);

    if (infoPtr->nOldHit != nHit)
    {
        if (infoPtr->bCaptured)
        {
            if (!infoPtr->bDragOutSent)
            {
                NMTOOLBARW nmt;
                ZeroMemory(&nmt, sizeof(nmt));
                nmt.iItem = infoPtr->buttons[infoPtr->nButtonDown].idCommand;
                TOOLBAR_SendNotify(&nmt.hdr, infoPtr, TBN_DRAGOUT);
                infoPtr->bDragOutSent = TRUE;
            }

            btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
            if (infoPtr->nOldHit == infoPtr->nButtonDown) {
                btnPtr->fsState &= ~TBSTATE_PRESSED;
                InvalidateRect(hwnd, &btnPtr->rect, TRUE);
            }
            else if (nHit == infoPtr->nButtonDown) {
                btnPtr->fsState |= TBSTATE_PRESSED;
                InvalidateRect(hwnd, &btnPtr->rect, TRUE);
            }
            infoPtr->nOldHit = nHit;
        }
    }

    return 0;
}


static inline LRESULT
TOOLBAR_NCActivate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    if (wndPtr->dwStyle & CCS_NODIVIDER) */
	return DefWindowProcW (hwnd, WM_NCACTIVATE, wParam, lParam);
/*    else */
/*	return TOOLBAR_NCPaint (wndPtr, wParam, lParam); */
}


static inline LRESULT
TOOLBAR_NCCalcSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (!(GetWindowLongW(hwnd, GWL_STYLE) & CCS_NODIVIDER))
	((LPRECT)lParam)->top += GetSystemMetrics(SM_CYEDGE);

    return DefWindowProcW (hwnd, WM_NCCALCSIZE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCCreate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr;
    LPCREATESTRUCTA cs = (LPCREATESTRUCTA)lParam;
    DWORD styleadd = 0;

    /* allocate memory for info structure */
    infoPtr = Alloc (sizeof(TOOLBAR_INFO));
    SetWindowLongPtrW (hwnd, 0, (LONG_PTR)infoPtr);

    /* paranoid!! */
    infoPtr->dwStructSize = sizeof(TBBUTTON);
    infoPtr->nRows = 1;
    infoPtr->nWidth = 0;

    /* fix instance handle, if the toolbar was created by CreateToolbarEx() */
    if (!GetWindowLongPtrW (hwnd, GWLP_HINSTANCE)) {
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW (GetParent (hwnd), GWLP_HINSTANCE);
	SetWindowLongPtrW (hwnd, GWLP_HINSTANCE, (LONG_PTR)hInst);
    }

    /* native control does:
     *    Get a lot of colors and brushes
     *    WM_NOTIFYFORMAT
     *    SystemParametersInfoW(0x1f, 0x3c, adr1, 0)
     *    CreateFontIndirectW(adr1)
     *    CreateBitmap(0x27, 0x24, 1, 1, 0)
     *    hdc = GetDC(toolbar)
     *    GetSystemMetrics(0x48)
     *    fnt2=CreateFontW(0xe, 0, 0, 0, 0x190, 0, 0, 0, 0, 2,
     *                     0, 0, 0, 0, "MARLETT")
     *    oldfnt = SelectObject(hdc, fnt2)
     *    GetCharWidthW(hdc, 0x36, 0x36, adr2)
     *    GetTextMetricsW(hdc, adr3)
     *    SelectObject(hdc, oldfnt)
     *    DeleteObject(fnt2)
     *    ReleaseDC(hdc)
     *    InvalidateRect(toolbar, 0, 1)
     *    SetWindowLongW(toolbar, 0, addr)
     *    SetWindowLongW(toolbar, -16, xxx)  **sometimes**
     *                                          WM_STYLECHANGING
     *                             CallWinEx   old         new
     *                       ie 1  0x56000a4c  0x46000a4c  0x56008a4d
     *                       ie 2  0x4600094c  0x4600094c  0x4600894d
     *                       ie 3  0x56000b4c  0x46000b4c  0x56008b4d
     *                      rebar  0x50008844  0x40008844  0x50008845
     *                      pager  0x50000844  0x40000844  0x50008845
     *                    IC35mgr  0x5400084e  **nochange**
     *           on entry to _NCCREATE         0x5400084e
     *                    rowlist  0x5400004e  **nochange**
     *           on entry to _NCCREATE         0x5400004e
     *
     */

    /* I think the code below is a bug, but it is the way that the native
     * controls seem to work. The effect is that if the user of TBSTYLE_FLAT
     * forgets to specify TBSTYLE_TRANSPARENT but does specify either
     * CCS_TOP or CCS_BOTTOM (_NOMOVEY and _TOP), then the control
     * does *not* set TBSTYLE_TRANSPARENT even though it should!!!!
     * Somehow, the only cases of this seem to be MFC programs.
     *
     * Note also that the addition of _TRANSPARENT occurs *only* here. It
     * does not occur in the WM_STYLECHANGING routine.
     *    (Guy Albertelli   9/2001)
     *
     */
    if (((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)) 
        && !(cs->style & TBSTYLE_TRANSPARENT))
	styleadd |= TBSTYLE_TRANSPARENT;
    if (!(cs->style & (CCS_TOP | CCS_NOMOVEY))) {
	styleadd |= CCS_TOP;   /* default to top */
	SetWindowLongW (hwnd, GWL_STYLE, cs->style | styleadd);
    }

    return DefWindowProcW (hwnd, WM_NCCREATE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCPaint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    RECT rcWindow;
    HDC hdc;

    if (dwStyle & WS_MINIMIZE)
	return 0; /* Nothing to do */

    DefWindowProcW (hwnd, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx (hwnd, 0, DCX_USESTYLE | DCX_WINDOW)))
	return 0;

    if (!(dwStyle & CCS_NODIVIDER))
    {
	GetWindowRect (hwnd, &rcWindow);
	OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);
	if( dwStyle & WS_BORDER )
	    OffsetRect (&rcWindow, 1, 1);
	DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_TOP);
    }

    ReleaseDC( hwnd, hdc );

    return 0;
}


/* handles requests from the tooltip control on what text to display */
static LRESULT TOOLBAR_TTGetDispInfo (TOOLBAR_INFO *infoPtr, NMTTDISPINFOW *lpnmtdi)
{
    int index = TOOLBAR_GetButtonIndex(infoPtr, lpnmtdi->hdr.idFrom, FALSE);

    TRACE("button index = %d\n", index);

    Free (infoPtr->pszTooltipText);
    infoPtr->pszTooltipText = NULL;

    if (index < 0)
        return 0;

    if (infoPtr->bUnicode)
    {
        WCHAR wszBuffer[INFOTIPSIZE+1];
        NMTBGETINFOTIPW tbgit;
        unsigned int len; /* in chars */

        wszBuffer[0] = '\0';
        wszBuffer[INFOTIPSIZE] = '\0';

        tbgit.pszText = wszBuffer;
        tbgit.cchTextMax = INFOTIPSIZE;
        tbgit.iItem = lpnmtdi->hdr.idFrom;
        tbgit.lParam = infoPtr->buttons[index].dwData;

        TOOLBAR_SendNotify(&tbgit.hdr, infoPtr, TBN_GETINFOTIPW);

        TRACE("TBN_GETINFOTIPW - got string %s\n", debugstr_w(tbgit.pszText));

        len = strlenW(tbgit.pszText);
        if (len > sizeof(lpnmtdi->szText)/sizeof(lpnmtdi->szText[0])-1)
        {
            /* need to allocate temporary buffer in infoPtr as there
             * isn't enough space in buffer passed to us by the
             * tooltip control */
            infoPtr->pszTooltipText = Alloc((len+1)*sizeof(WCHAR));
            if (infoPtr->pszTooltipText)
            {
                memcpy(infoPtr->pszTooltipText, tbgit.pszText, (len+1)*sizeof(WCHAR));
                lpnmtdi->lpszText = infoPtr->pszTooltipText;
                return 0;
            }
        }
        else if (len > 0)
        {
            memcpy(lpnmtdi->lpszText, tbgit.pszText, (len+1)*sizeof(WCHAR));
            return 0;
        }
    }
    else
    {
        CHAR szBuffer[INFOTIPSIZE+1];
        NMTBGETINFOTIPA tbgit;
        unsigned int len; /* in chars */

        szBuffer[0] = '\0';
        szBuffer[INFOTIPSIZE] = '\0';

        tbgit.pszText = szBuffer;
        tbgit.cchTextMax = INFOTIPSIZE;
        tbgit.iItem = lpnmtdi->hdr.idFrom;
        tbgit.lParam = infoPtr->buttons[index].dwData;

        TOOLBAR_SendNotify(&tbgit.hdr, infoPtr, TBN_GETINFOTIPA);

        TRACE("TBN_GETINFOTIPA - got string %s\n", debugstr_a(tbgit.pszText));

        len = MultiByteToWideChar(CP_ACP, 0, tbgit.pszText, -1, NULL, 0);
        if (len > sizeof(lpnmtdi->szText)/sizeof(lpnmtdi->szText[0]))
        {
            /* need to allocate temporary buffer in infoPtr as there
             * isn't enough space in buffer passed to us by the
             * tooltip control */
            infoPtr->pszTooltipText = Alloc(len*sizeof(WCHAR));
            if (infoPtr->pszTooltipText)
            {
                MultiByteToWideChar(CP_ACP, 0, tbgit.pszText, -1, infoPtr->pszTooltipText, len);
                lpnmtdi->lpszText = infoPtr->pszTooltipText;
                return 0;
            }
        }
        else if (tbgit.pszText[0])
        {
            MultiByteToWideChar(CP_ACP, 0, tbgit.pszText, -1,
                                lpnmtdi->lpszText, sizeof(lpnmtdi->szText)/sizeof(lpnmtdi->szText[0]));
            return 0;
        }
    }

    /* if button has text, but it is not shown then automatically
     * use that text as tooltip */
    if ((infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) &&
        !(infoPtr->buttons[index].fsStyle & BTNS_SHOWTEXT))
    {
        LPWSTR pszText = TOOLBAR_GetText(infoPtr, &infoPtr->buttons[index]);
        unsigned int len = pszText ? strlenW(pszText) : 0;

        TRACE("using button hidden text %s\n", debugstr_w(pszText));

        if (len > sizeof(lpnmtdi->szText)/sizeof(lpnmtdi->szText[0])-1)
        {
            /* need to allocate temporary buffer in infoPtr as there
             * isn't enough space in buffer passed to us by the
             * tooltip control */
            infoPtr->pszTooltipText = Alloc((len+1)*sizeof(WCHAR));
            if (infoPtr->pszTooltipText)
            {
                memcpy(infoPtr->pszTooltipText, pszText, (len+1)*sizeof(WCHAR));
                lpnmtdi->lpszText = infoPtr->pszTooltipText;
                return 0;
            }
        }
        else if (len > 0)
        {
            memcpy(lpnmtdi->lpszText, pszText, (len+1)*sizeof(WCHAR));
            return 0;
        }
    }

    TRACE("Sending tooltip notification to %p\n", infoPtr->hwndNotify);

    /* last resort: send notification on to app */
    /* FIXME: find out what is really used here */
    return SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, lpnmtdi->hdr.idFrom, (LPARAM)lpnmtdi);
}


static inline LRESULT
TOOLBAR_Notify (HWND hwnd, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPNMHDR lpnmh = (LPNMHDR)lParam;

    switch (lpnmh->code)
    {
    case PGN_CALCSIZE:
    {
        LPNMPGCALCSIZE lppgc = (LPNMPGCALCSIZE)lParam;

        if (lppgc->dwFlag == PGF_CALCWIDTH) {
            lppgc->iWidth = infoPtr->rcBound.right - infoPtr->rcBound.left;
            TRACE("processed PGN_CALCSIZE, returning horz size = %d\n",
                  lppgc->iWidth);
        }
        else {
            lppgc->iHeight = infoPtr->rcBound.bottom - infoPtr->rcBound.top;
            TRACE("processed PGN_CALCSIZE, returning vert size = %d\n",
                  lppgc->iHeight);
        }
    	return 0;
    }

    case PGN_SCROLL:
    {
        LPNMPGSCROLL lppgs = (LPNMPGSCROLL)lParam;

        lppgs->iScroll = (lppgs->iDir & (PGF_SCROLLLEFT | PGF_SCROLLRIGHT)) ?
                          infoPtr->nButtonWidth : infoPtr->nButtonHeight;
        TRACE("processed PGN_SCROLL, returning scroll=%d, dir=%d\n",
              lppgs->iScroll, lppgs->iDir);
        return 0;
    }

    case TTN_GETDISPINFOW:
        return TOOLBAR_TTGetDispInfo(infoPtr, (LPNMTTDISPINFOW)lParam);

    case TTN_GETDISPINFOA:
        FIXME("TTN_GETDISPINFOA - should not be received; please report\n");
        return 0;

    default:
        return 0;
    }
}


static LRESULT
TOOLBAR_NotifyFormat(const TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LRESULT format;

    TRACE("wParam = 0x%lx, lParam = 0x%08lx\n", wParam, lParam);

    if (lParam == NF_QUERY)
        return NFR_UNICODE;

    if (lParam == NF_REQUERY) {
	format = SendMessageW(infoPtr->hwndNotify,
			 WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);
	if ((format != NFR_ANSI) && (format != NFR_UNICODE)) {
	    ERR("wrong response to WM_NOTIFYFORMAT (%ld), assuming ANSI\n",
		format);
	    format = NFR_ANSI;
	}
	return format;
    }
    return 0;
}


static LRESULT
TOOLBAR_Paint (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    HDC hdc;
    PAINTSTRUCT ps;

    /* fill ps.rcPaint with a default rect */
    ps.rcPaint = infoPtr->rcBound;

    hdc = wParam==0 ? BeginPaint(hwnd, &ps) : (HDC)wParam;

    TRACE("psrect=(%s)\n", wine_dbgstr_rect(&ps.rcPaint));

    TOOLBAR_Refresh (hwnd, hdc, &ps);
    if (!wParam) EndPaint (hwnd, &ps);

    return 0;
}


static LRESULT
TOOLBAR_SetFocus (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("nHotItem = %d\n", infoPtr->nHotItem);

    /* make first item hot */
    if (infoPtr->nNumButtons > 0)
        TOOLBAR_SetHotItemEx(infoPtr, 0, HICF_OTHER);

    return 0;
}

static LRESULT
TOOLBAR_SetFont(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    
    TRACE("font=%p redraw=%ld\n", (HFONT)wParam, lParam);
    
    if (wParam == 0)
        infoPtr->hFont = infoPtr->hDefaultFont;
    else
        infoPtr->hFont = (HFONT)wParam;

    TOOLBAR_CalcToolbar(hwnd);

    if (lParam)
        InvalidateRect(hwnd, NULL, TRUE);
    return 1;
}

static LRESULT
TOOLBAR_SetRedraw (HWND hwnd, WPARAM wParam)
     /*****************************************************
      *
      * Function;
      *  Handles the WM_SETREDRAW message.
      *
      * Documentation:
      *  According to testing V4.71 of COMCTL32 returns the
      *  *previous* status of the redraw flag (either 0 or 1)
      *  instead of the MSDN documented value of 0 if handled.
      *  (For laughs see the "consistency" with same function
      *   in rebar.)
      *
      *****************************************************/
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    BOOL oldredraw = infoPtr->bDoRedraw;

    TRACE("set to %s\n",
	  (wParam) ? "TRUE" : "FALSE");
    infoPtr->bDoRedraw = (BOOL) wParam;
    if (wParam) {
	InvalidateRect (infoPtr->hwndSelf, 0, TRUE);
    }
    return (oldredraw) ? 1 : 0;
}


static LRESULT
TOOLBAR_Size (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("sizing toolbar!\n");

    if (infoPtr->dwExStyle & TBSTYLE_EX_HIDECLIPPEDBUTTONS)
    {
        RECT delta_width, delta_height, client, dummy;
        DWORD min_x, max_x, min_y, max_y;
        TBUTTON_INFO *btnPtr;
        INT i;

        GetClientRect(hwnd, &client);
        if(client.right > infoPtr->client_rect.right)
        {
            min_x = infoPtr->client_rect.right;
            max_x = client.right;
        }
        else
        {
            max_x = infoPtr->client_rect.right;
            min_x = client.right;
        }
        if(client.bottom > infoPtr->client_rect.bottom)
        {
            min_y = infoPtr->client_rect.bottom;
            max_y = client.bottom;
        }
        else
        {
            max_y = infoPtr->client_rect.bottom;
            min_y = client.bottom;
        }

        SetRect(&delta_width, min_x, 0, max_x, min_y);
        SetRect(&delta_height, 0, min_y, max_x, max_y);

        TRACE("delta_width %s delta_height %s\n", wine_dbgstr_rect(&delta_width), wine_dbgstr_rect(&delta_height));
        btnPtr = infoPtr->buttons;
        for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++)
            if(IntersectRect(&dummy, &delta_width, &btnPtr->rect) ||
                IntersectRect(&dummy, &delta_height, &btnPtr->rect))
                InvalidateRect(hwnd, &btnPtr->rect, TRUE);
    }
    GetClientRect(hwnd, &infoPtr->client_rect);
    TOOLBAR_AutoSize(hwnd);
    return 0;
}


static LRESULT
TOOLBAR_StyleChanged (HWND hwnd, INT nType, const STYLESTRUCT *lpStyle)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (nType == GWL_STYLE)
    {
        DWORD dwOldStyle = infoPtr->dwStyle;

        if (lpStyle->styleNew & TBSTYLE_LIST)
            infoPtr->dwDTFlags = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
        else
            infoPtr->dwDTFlags = DT_CENTER | DT_END_ELLIPSIS;

        TOOLBAR_CheckStyle (hwnd, lpStyle->styleNew);

        TRACE("new style 0x%08x\n", lpStyle->styleNew);

        infoPtr->dwStyle = lpStyle->styleNew;

        if ((dwOldStyle ^ lpStyle->styleNew) & (TBSTYLE_WRAPABLE | CCS_VERT))
            TOOLBAR_LayoutToolbar(hwnd);

        /* only resize if one of the CCS_* styles was changed */
        if ((dwOldStyle ^ lpStyle->styleNew) & COMMON_STYLES)
        {
            TOOLBAR_AutoSize (hwnd);
    
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    return 0;
}


static LRESULT
TOOLBAR_SysColorChange ()
{
    COMCTL32_RefreshSysColors();

    return 0;
}


/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (HWND hwnd)
{
    HTHEME theme = GetWindowTheme (hwnd);
    CloseThemeData (theme);
    OpenThemeData (hwnd, themeClass);
    return 0;
}


static LRESULT WINAPI
ToolbarWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n",
	  hwnd, uMsg, /* SPY_GetMsgName(uMsg), */ wParam, lParam);

    if (!infoPtr && (uMsg != WM_NCCREATE))
	return DefWindowProcW( hwnd, uMsg, wParam, lParam );

    switch (uMsg)
    {
	case TB_ADDBITMAP:
	    return TOOLBAR_AddBitmap (hwnd, wParam, lParam);

	case TB_ADDBUTTONSA:
	    return TOOLBAR_AddButtonsT(hwnd, wParam, lParam, FALSE);

	case TB_ADDBUTTONSW:
	    return TOOLBAR_AddButtonsT(hwnd, wParam, lParam, TRUE);

	case TB_ADDSTRINGA:
	    return TOOLBAR_AddStringA (hwnd, wParam, lParam);

	case TB_ADDSTRINGW:
	    return TOOLBAR_AddStringW (hwnd, wParam, lParam);

	case TB_AUTOSIZE:
	    return TOOLBAR_AutoSize (hwnd);

	case TB_BUTTONCOUNT:
	    return TOOLBAR_ButtonCount (hwnd);

	case TB_BUTTONSTRUCTSIZE:
	    return TOOLBAR_ButtonStructSize (hwnd, wParam);

	case TB_CHANGEBITMAP:
	    return TOOLBAR_ChangeBitmap (hwnd, wParam, lParam);

	case TB_CHECKBUTTON:
	    return TOOLBAR_CheckButton (hwnd, wParam, lParam);

	case TB_COMMANDTOINDEX:
	    return TOOLBAR_CommandToIndex (hwnd, wParam);

	case TB_CUSTOMIZE:
	    return TOOLBAR_Customize (hwnd);

	case TB_DELETEBUTTON:
	    return TOOLBAR_DeleteButton (hwnd, wParam);

	case TB_ENABLEBUTTON:
	    return TOOLBAR_EnableButton (hwnd, wParam, lParam);

	case TB_GETANCHORHIGHLIGHT:
	    return TOOLBAR_GetAnchorHighlight (hwnd);

	case TB_GETBITMAP:
	    return TOOLBAR_GetBitmap (hwnd, wParam);

	case TB_GETBITMAPFLAGS:
	    return TOOLBAR_GetBitmapFlags ();

	case TB_GETBUTTON:
	    return TOOLBAR_GetButton (hwnd, wParam, lParam);

	case TB_GETBUTTONINFOA:
	    return TOOLBAR_GetButtonInfoT(hwnd, wParam, lParam, FALSE);

	case TB_GETBUTTONINFOW:
	    return TOOLBAR_GetButtonInfoT(hwnd, wParam, lParam, TRUE);

	case TB_GETBUTTONSIZE:
	    return TOOLBAR_GetButtonSize (hwnd);

	case TB_GETBUTTONTEXTA:
	    return TOOLBAR_GetButtonTextA (hwnd, wParam, lParam);

	case TB_GETBUTTONTEXTW:
	    return TOOLBAR_GetButtonTextW (hwnd, wParam, lParam);

	case TB_GETDISABLEDIMAGELIST:
	    return TOOLBAR_GetDisabledImageList (hwnd, wParam, lParam);

	case TB_GETEXTENDEDSTYLE:
	    return TOOLBAR_GetExtendedStyle (hwnd);

	case TB_GETHOTIMAGELIST:
	    return TOOLBAR_GetHotImageList (hwnd, wParam, lParam);

	case TB_GETHOTITEM:
	    return TOOLBAR_GetHotItem (hwnd);

	case TB_GETIMAGELIST:
	    return TOOLBAR_GetDefImageList (hwnd, wParam, lParam);

	case TB_GETINSERTMARK:
	    return TOOLBAR_GetInsertMark (hwnd, lParam);

	case TB_GETINSERTMARKCOLOR:
	    return TOOLBAR_GetInsertMarkColor (hwnd);

	case TB_GETITEMRECT:
	    return TOOLBAR_GetItemRect (hwnd, wParam, lParam);

	case TB_GETMAXSIZE:
	    return TOOLBAR_GetMaxSize (hwnd, lParam);

/*	case TB_GETOBJECT:			*/ /* 4.71 */

	case TB_GETPADDING:
	    return TOOLBAR_GetPadding (hwnd);

	case TB_GETRECT:
	    return TOOLBAR_GetRect (hwnd, wParam, lParam);

	case TB_GETROWS:
	    return TOOLBAR_GetRows (hwnd);

	case TB_GETSTATE:
	    return TOOLBAR_GetState (hwnd, wParam);

	case TB_GETSTRINGA:
        return TOOLBAR_GetStringA (hwnd, wParam, lParam);

	case TB_GETSTRINGW:
	    return TOOLBAR_GetStringW (hwnd, wParam, lParam);

	case TB_GETSTYLE:
	    return TOOLBAR_GetStyle (hwnd);

	case TB_GETTEXTROWS:
	    return TOOLBAR_GetTextRows (hwnd);

	case TB_GETTOOLTIPS:
	    return TOOLBAR_GetToolTips (hwnd);

	case TB_GETUNICODEFORMAT:
	    return TOOLBAR_GetUnicodeFormat (hwnd);

	case TB_HIDEBUTTON:
	    return TOOLBAR_HideButton (hwnd, wParam, lParam);

	case TB_HITTEST:
	    return TOOLBAR_HitTest (hwnd, lParam);

	case TB_INDETERMINATE:
	    return TOOLBAR_Indeterminate (hwnd, wParam, lParam);

	case TB_INSERTBUTTONA:
	    return TOOLBAR_InsertButtonT(hwnd, wParam, lParam, FALSE);

	case TB_INSERTBUTTONW:
	    return TOOLBAR_InsertButtonT(hwnd, wParam, lParam, TRUE);

/*	case TB_INSERTMARKHITTEST:		*/ /* 4.71 */

	case TB_ISBUTTONCHECKED:
	    return TOOLBAR_IsButtonChecked (hwnd, wParam);

	case TB_ISBUTTONENABLED:
	    return TOOLBAR_IsButtonEnabled (hwnd, wParam);

	case TB_ISBUTTONHIDDEN:
	    return TOOLBAR_IsButtonHidden (hwnd, wParam);

	case TB_ISBUTTONHIGHLIGHTED:
	    return TOOLBAR_IsButtonHighlighted (hwnd, wParam);

	case TB_ISBUTTONINDETERMINATE:
	    return TOOLBAR_IsButtonIndeterminate (hwnd, wParam);

	case TB_ISBUTTONPRESSED:
	    return TOOLBAR_IsButtonPressed (hwnd, wParam);

	case TB_LOADIMAGES:
	    return TOOLBAR_LoadImages (hwnd, wParam, lParam);

	case TB_MAPACCELERATORA:
	case TB_MAPACCELERATORW:
	    return TOOLBAR_MapAccelerator (hwnd, wParam, lParam);

	case TB_MARKBUTTON:
	    return TOOLBAR_MarkButton (hwnd, wParam, lParam);

	case TB_MOVEBUTTON:
	    return TOOLBAR_MoveButton (hwnd, wParam, lParam);

	case TB_PRESSBUTTON:
	    return TOOLBAR_PressButton (hwnd, wParam, lParam);

	case TB_REPLACEBITMAP:
            return TOOLBAR_ReplaceBitmap (hwnd, lParam);

	case TB_SAVERESTOREA:
	    return TOOLBAR_SaveRestoreA (hwnd, wParam, (LPTBSAVEPARAMSA)lParam);

	case TB_SAVERESTOREW:
	    return TOOLBAR_SaveRestoreW (hwnd, wParam, (LPTBSAVEPARAMSW)lParam);

	case TB_SETANCHORHIGHLIGHT:
	    return TOOLBAR_SetAnchorHighlight (hwnd, wParam);

	case TB_SETBITMAPSIZE:
	    return TOOLBAR_SetBitmapSize (hwnd, wParam, lParam);

	case TB_SETBUTTONINFOA:
	    return TOOLBAR_SetButtonInfoA (hwnd, wParam, lParam);

	case TB_SETBUTTONINFOW:
	    return TOOLBAR_SetButtonInfoW (hwnd, wParam, lParam);

	case TB_SETBUTTONSIZE:
	    return TOOLBAR_SetButtonSize (hwnd, lParam);

	case TB_SETBUTTONWIDTH:
	    return TOOLBAR_SetButtonWidth (hwnd, lParam);

	case TB_SETCMDID:
	    return TOOLBAR_SetCmdId (hwnd, wParam, lParam);

	case TB_SETDISABLEDIMAGELIST:
	    return TOOLBAR_SetDisabledImageList (hwnd, wParam, lParam);

	case TB_SETDRAWTEXTFLAGS:
	    return TOOLBAR_SetDrawTextFlags (hwnd, wParam, lParam);

	case TB_SETEXTENDEDSTYLE:
	    return TOOLBAR_SetExtendedStyle (hwnd, lParam);

	case TB_SETHOTIMAGELIST:
	    return TOOLBAR_SetHotImageList (hwnd, wParam, lParam);

	case TB_SETHOTITEM:
	    return TOOLBAR_SetHotItem (hwnd, wParam);

	case TB_SETIMAGELIST:
	    return TOOLBAR_SetImageList (hwnd, wParam, lParam);

	case TB_SETINDENT:
	    return TOOLBAR_SetIndent (hwnd, wParam);

	case TB_SETINSERTMARK:
	    return TOOLBAR_SetInsertMark (hwnd, lParam);

	case TB_SETINSERTMARKCOLOR:
	    return TOOLBAR_SetInsertMarkColor (hwnd, lParam);

	case TB_SETMAXTEXTROWS:
	    return TOOLBAR_SetMaxTextRows (hwnd, wParam);

	case TB_SETPADDING:
	    return TOOLBAR_SetPadding (hwnd, lParam);

	case TB_SETPARENT:
	    return TOOLBAR_SetParent (hwnd, wParam);

	case TB_SETROWS:
	    return TOOLBAR_SetRows (hwnd, wParam, lParam);

	case TB_SETSTATE:
	    return TOOLBAR_SetState (hwnd, wParam, lParam);

	case TB_SETSTYLE:
	    return TOOLBAR_SetStyle (hwnd, lParam);

	case TB_SETTOOLTIPS:
	    return TOOLBAR_SetToolTips (hwnd, wParam, lParam);

	case TB_SETUNICODEFORMAT:
	    return TOOLBAR_SetUnicodeFormat (hwnd, wParam);

	case TB_UNKWN45D:
	    return TOOLBAR_Unkwn45D(hwnd, wParam, lParam);

	case TB_SETHOTITEM2:
	    return TOOLBAR_SetHotItem2 (hwnd, wParam, lParam);

	case TB_SETLISTGAP:
	    return TOOLBAR_SetListGap(hwnd, wParam);

	case TB_GETIMAGELISTCOUNT:
	    return TOOLBAR_GetImageListCount(hwnd, wParam, lParam);

	case TB_GETIDEALSIZE:
	    return TOOLBAR_GetIdealSize (hwnd, wParam, lParam);

	case TB_UNKWN464:
	    return TOOLBAR_Unkwn464(hwnd, wParam, lParam);

/* Common Control Messages */

/*	case TB_GETCOLORSCHEME:			*/ /* identical to CCM_ */
	case CCM_GETCOLORSCHEME:
	    return TOOLBAR_GetColorScheme (hwnd, (LPCOLORSCHEME)lParam);

/*	case TB_SETCOLORSCHEME:			*/ /* identical to CCM_ */
	case CCM_SETCOLORSCHEME:
	    return TOOLBAR_SetColorScheme (hwnd, (LPCOLORSCHEME)lParam);

	case CCM_GETVERSION:
	    return TOOLBAR_GetVersion (hwnd);

	case CCM_SETVERSION:
	    return TOOLBAR_SetVersion (hwnd, (INT)wParam);


/*	case WM_CHAR: */

	case WM_CREATE:
	    return TOOLBAR_Create (hwnd, lParam);

	case WM_DESTROY:
	  return TOOLBAR_Destroy (hwnd);

	case WM_ERASEBKGND:
	    return TOOLBAR_EraseBackground (hwnd, wParam, lParam);

	case WM_GETFONT:
		return TOOLBAR_GetFont (hwnd);

	case WM_KEYDOWN:
	    return TOOLBAR_KeyDown (hwnd, wParam, lParam);

/*	case WM_KILLFOCUS: */

	case WM_LBUTTONDBLCLK:
	    return TOOLBAR_LButtonDblClk (hwnd, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return TOOLBAR_LButtonDown (hwnd, wParam, lParam);

	case WM_LBUTTONUP:
	    return TOOLBAR_LButtonUp (hwnd, wParam, lParam);

	case WM_RBUTTONUP:
	    return TOOLBAR_RButtonUp (hwnd, wParam, lParam);

	case WM_RBUTTONDBLCLK:
	    return TOOLBAR_RButtonDblClk (hwnd, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TOOLBAR_MouseMove (hwnd, wParam, lParam);

	case WM_MOUSELEAVE:
	    return TOOLBAR_MouseLeave (hwnd);

	case WM_CAPTURECHANGED:
	    return TOOLBAR_CaptureChanged(hwnd);

	case WM_NCACTIVATE:
	    return TOOLBAR_NCActivate (hwnd, wParam, lParam);

	case WM_NCCALCSIZE:
	    return TOOLBAR_NCCalcSize (hwnd, wParam, lParam);

	case WM_NCCREATE:
	    return TOOLBAR_NCCreate (hwnd, wParam, lParam);

	case WM_NCPAINT:
	    return TOOLBAR_NCPaint (hwnd, wParam, lParam);

	case WM_NOTIFY:
	    return TOOLBAR_Notify (hwnd, lParam);

	case WM_NOTIFYFORMAT:
	    return TOOLBAR_NotifyFormat (infoPtr, wParam, lParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return TOOLBAR_Paint (hwnd, wParam);

	case WM_SETFOCUS:
	    return TOOLBAR_SetFocus (hwnd);

	case WM_SETFONT:
            return TOOLBAR_SetFont(hwnd, wParam, lParam);

	case WM_SETREDRAW:
	    return TOOLBAR_SetRedraw (hwnd, wParam);

	case WM_SIZE:
	    return TOOLBAR_Size (hwnd);

	case WM_STYLECHANGED:
	    return TOOLBAR_StyleChanged (hwnd, (INT)wParam, (LPSTYLESTRUCT)lParam);

	case WM_SYSCOLORCHANGE:
	    return TOOLBAR_SysColorChange ();
            
        case WM_THEMECHANGED:
            return theme_changed (hwnd);

/*	case WM_WININICHANGE: */

	case WM_CHARTOITEM:
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_VKEYTOITEM:
            return SendMessageW (infoPtr->hwndNotify, uMsg, wParam, lParam);

	/* We see this in Outlook Express 5.x and just does DefWindowProc */
        case PGM_FORWARDMOUSE:
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
		ERR("unknown msg %04x wp=%08lx lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


VOID
TOOLBAR_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = ToolbarWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLBAR_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = TOOLBARCLASSNAMEW;

    RegisterClassW (&wndClass);
}


VOID
TOOLBAR_Unregister (void)
{
    UnregisterClassW (TOOLBARCLASSNAMEW, NULL);
}

static HIMAGELIST TOOLBAR_InsertImageList(PIMLENTRY **pies, INT *cies, HIMAGELIST himl, INT id)
{
    HIMAGELIST himlold;
    PIMLENTRY c = NULL;

    /* Check if the entry already exists */
    c = TOOLBAR_GetImageListEntry(*pies, *cies, id);

    /* If this is a new entry we must create it and insert into the array */
    if (!c)
    {
        PIMLENTRY *pnies;

        c = Alloc(sizeof(IMLENTRY));
	c->id = id;

	pnies = Alloc((*cies + 1) * sizeof(PIMLENTRY));
	memcpy(pnies, *pies, ((*cies) * sizeof(PIMLENTRY)));
	pnies[*cies] = c;
	(*cies)++;

	Free(*pies);
	*pies = pnies;
    }

    himlold = c->himl;
    c->himl = himl;

    return himlold;
}


static VOID TOOLBAR_DeleteImageList(PIMLENTRY **pies, INT *cies)
{
    int i;

    for (i = 0; i < *cies; i++)
	Free((*pies)[i]);

    Free(*pies);

    *cies = 0;
    *pies = NULL;
}


static PIMLENTRY TOOLBAR_GetImageListEntry(const PIMLENTRY *pies, INT cies, INT id)
{
    PIMLENTRY c = NULL;

    if (pies != NULL)
    {
	int i;

        for (i = 0; i < cies; i++)
        {
            if (pies[i]->id == id)
            {
                c = pies[i];
                break;
            }
        }
    }

    return c;
}


static HIMAGELIST TOOLBAR_GetImageList(const PIMLENTRY *pies, INT cies, INT id)
{
    HIMAGELIST himlDef = 0;
    PIMLENTRY pie = TOOLBAR_GetImageListEntry(pies, cies, id);

    if (pie)
        himlDef = pie->himl;

    return himlDef;
}


static BOOL TOOLBAR_GetButtonInfo(const TOOLBAR_INFO *infoPtr, NMTOOLBARW *nmtb)
{
    if (infoPtr->bUnicode)
        return TOOLBAR_SendNotify(&nmtb->hdr, infoPtr, TBN_GETBUTTONINFOW);
    else
    {
        CHAR Buffer[256];
        NMTOOLBARA nmtba;
        BOOL bRet = FALSE;

        nmtba.iItem = nmtb->iItem;
        nmtba.pszText = Buffer;
        nmtba.cchText = 256;
        ZeroMemory(nmtba.pszText, nmtba.cchText);

        if (TOOLBAR_SendNotify(&nmtba.hdr, infoPtr, TBN_GETBUTTONINFOA))
        {
            int ccht = strlen(nmtba.pszText);
            if (ccht)
               MultiByteToWideChar(CP_ACP, 0, nmtba.pszText, -1,
                  nmtb->pszText, nmtb->cchText);

            nmtb->tbButton = nmtba.tbButton;
            bRet = TRUE;
        }

        return bRet;
    }
}


static BOOL TOOLBAR_IsButtonRemovable(const TOOLBAR_INFO *infoPtr, int iItem, PCUSTOMBUTTON btnInfo)
{
    NMTOOLBARW nmtb;

    /* MSDN states that iItem is the index of the button, rather than the
     * command ID as used by every other NMTOOLBAR notification */
    nmtb.iItem = iItem;
    memcpy(&nmtb.tbButton, &btnInfo->btn, sizeof(TBBUTTON));

    return TOOLBAR_SendNotify(&nmtb.hdr, infoPtr, TBN_QUERYDELETE);
}
