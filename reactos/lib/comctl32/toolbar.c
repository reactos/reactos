/*
 * Toolbar control
 *
 * Copyright 1998,1999 Eric Kohl
 * Copyright 2000 Eric Kohl for CodeWeavers
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
 *  Differences between MSDN and actual native control operation:
 *   1. MSDN says: "TBSTYLE_LIST: Creates a flat toolbar with button text
 *                  to the right of the bitmap. Otherwise, this style is
 *                  identical to TBSTYLE_FLAT."
 *      As implemented by both v4.71 and v5.80 of the native COMCTL32.DLL
 *      you can create a TBSTYLE_LIST without TBSTYLE_FLAT and the result
 *      is non-flat non-transparent buttons. Therefore TBSTYLE_LIST does
 *      *not* imply TBSTYLE_FLAT as documented.  (GA 8/2001)
 *
 *
 * TODO:
 *   - Button wrapping (under construction).
 *   - Messages.
 *   - Notifications
 *     - NM_CHAR
 *     - NM_KEYDOWN
 *     - NM_LDOWN
 *     - NM_RCLICK
 *     - NM_RDBLCLICK
 *     - TBN_DELETINGBUTTON
 *     - TBN_DRAGOUT
 *     - TBN_GETOBJECT
 *     - TBN_RESTORE
 *     - TBN_SAVE
 *     - TBN_TOOLBARCHANGE
 *   - Fix TB_SETROWS.
 *   - Tooltip support (almost complete).
 *   - Fix TOOLBAR_SetButtonInfo32A/W.
 *   - iString of -1 is undocumented
 *   - Customization dialog:
 *      - Add flat look.
 *      - Minor buglet in 'available buttons' list:
 *        Buttons are not listed in M$-like order. M$ seems to use a single
 *        internal list to store the button information of both listboxes.
 *      - Drag list support.
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom contains executables addstr.exe, btncount.exe,
 *     btnstate.exe, butstrsz.exe, chkbtn.exe, chngbmp.exe, customiz.exe,
 *     enablebtn.exe, getbmp.exe, getbtn.exe, getflags.exe, hidebtn.exe,
 *     indetbtn.exe, insbtn.exe, pressbtn.exe, setbtnsz.exe, setcmdid.exe,
 *     setparnt.exe, setrows.exe, toolwnd.exe.
 *   - Microsofts controlspy examples.
 *   - Charles Petzold's 'Programming Windows': gadgets.exe
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "winnls.h"
#include "commctrl.h"
#include "imagelist.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolbar);

typedef struct
{
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    DWORD dwData;
    INT iString;

    BOOL bHot;
    INT nRow;
    RECT rect;
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
    DWORD      dwStructSize;   /* size of TBBUTTON struct */
    INT      nHeight;        /* height of the toolbar */
    INT      nWidth;         /* width of the toolbar */
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
    BOOL     bUnicode;        /* ASCII (FALSE) or Unicode (TRUE)? */
    BOOL     bCaptured;       /* mouse captured? */
    INT      nButtonDown;
    INT      nOldHit;
    INT      nHotItem;        /* index of the "hot" item */
    DWORD    dwBaseCustDraw;  /* CDRF_ response (w/o TBCDRF_) from PREPAINT */
    DWORD    dwItemCustDraw;  /* CDRF_ response (w/o TBCDRF_) from ITEMPREP */
    DWORD    dwItemCDFlag;    /* TBCDRF_ flags from last ITEMPREPAINT    */
    SIZE     szPadding;       /* padding values around button */
    HFONT    hDefaultFont;
    HFONT    hFont;           /* text font */
    HIMAGELIST himlInt;         /* image list created internally */
    PIMLENTRY *himlDef;       /* default image list array */
    INT       cimlDef;        /* default image list array count */
    PIMLENTRY *himlHot;       /* hot image list array */
    INT       cimlHot;        /* hot image list array count */
    PIMLENTRY *himlDis;       /* disabled image list array */
    INT       cimlDis;        /* disabled image list array count */
    HWND     hwndToolTip;     /* handle to tool tip control */
    HWND     hwndNotify;      /* handle to the window that gets notifications */
    HWND     hwndSelf;        /* my own handle */
    BOOL     bTransparent;    /* background transparency flag */
    BOOL     bBtnTranspnt;    /* button transparency flag */
    BOOL     bAutoSize;       /* auto size deadlock indicator */
    BOOL     bAnchor;         /* anchor highlight enabled */
    BOOL     bNtfUnicode;     /* TRUE if NOTIFYs use {W} */
    BOOL     bDoRedraw;       /* Redraw status */
    DWORD      dwExStyle;       /* extended toolbar style */
    DWORD      dwDTFlags;       /* DrawText flags */

    COLORREF   clrInsertMark;   /* insert mark color */
    COLORREF   clrBtnHighlight; /* color for Flat Separator */
    COLORREF   clrBtnShadow;    /* color for Flag Separator */
    RECT     rcBound;         /* bounding rectangle */
    INT      iVersion;

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

/* gap between edge of button and image with TBSTYLE_LIST */
#define LIST_IMAGE_OFFSET  3
/* gap between bitmap and text (always present) */
#define LIST_TEXT_OFFSET   2
/* how wide to treat the bitmap if it isn't present */
#define LIST_IMAGE_ABSENT_WIDTH 2

#define TOOLBAR_GetInfoPtr(hwnd) ((TOOLBAR_INFO *)GetWindowLongA(hwnd,0))
#define TOOLBAR_HasText(x, y) (TOOLBAR_GetText(x, y) ? TRUE : FALSE)
#define TOOLBAR_HasDropDownArrows(exStyle) ((exStyle & TBSTYLE_EX_DRAWDDARROWS) ? TRUE : FALSE)

/* Used to find undocumented extended styles */
#define TBSTYLE_EX_ALL (TBSTYLE_EX_DRAWDDARROWS | \
                        TBSTYLE_EX_UNDOC1 | \
                        TBSTYLE_EX_MIXEDBUTTONS | \
                        TBSTYLE_EX_HIDECLIPPEDBUTTONS)

#define GETIBITMAP(infoPtr, i) (infoPtr->iVersion >= 5 ? LOWORD(i) : i)
#define GETHIMLID(infoPtr, i) (infoPtr->iVersion >= 5 ? HIWORD(i) : 0)
#define GETDEFIMAGELIST(infoPtr, id) TOOLBAR_GetImageList(infoPtr->himlDef, infoPtr->cimlDef, id)
#define GETHOTIMAGELIST(infoPtr, id) TOOLBAR_GetImageList(infoPtr->himlHot, infoPtr->cimlHot, id)
#define GETDISIMAGELIST(infoPtr, id) TOOLBAR_GetImageList(infoPtr->himlDis, infoPtr->cimlDis, id)

static BOOL TOOLBAR_GetButtonInfo(TOOLBAR_INFO *infoPtr, NMTOOLBARW *nmtb);
static BOOL TOOLBAR_IsButtonRemovable(TOOLBAR_INFO *infoPtr, int iItem, PCUSTOMBUTTON btnInfo);
static HIMAGELIST TOOLBAR_GetImageList(PIMLENTRY *pies, INT cies, INT id);
static PIMLENTRY TOOLBAR_GetImageListEntry(PIMLENTRY *pies, INT cies, INT id);
static VOID TOOLBAR_DeleteImageList(PIMLENTRY **pies, INT *cies);
static HIMAGELIST TOOLBAR_InsertImageList(PIMLENTRY **pies, INT *cies, HIMAGELIST himl, INT id);

static LRESULT
TOOLBAR_NotifyFormat(TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam);


static LPWSTR
TOOLBAR_GetText(TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr)
{
    LPWSTR lpText = NULL;

    /* FIXME: iString == -1 is undocumented */
    if ((HIWORD(btnPtr->iString) != 0) && (btnPtr->iString != -1))
        lpText = (LPWSTR)btnPtr->iString;
    else if ((btnPtr->iString >= 0) && (btnPtr->iString < infoPtr->nNumStrings))
        lpText = infoPtr->strings[btnPtr->iString];

    return lpText;
}

static void
TOOLBAR_DumpButton(TOOLBAR_INFO *infoPtr, TBUTTON_INFO *bP, INT btn_num, BOOL internal)
{
    if (TRACE_ON(toolbar)){
	TRACE("button %d id %d, bitmap=%d, state=%02x, style=%02x, data=%08lx, stringid=0x%08x\n",
              btn_num, bP->idCommand, GETIBITMAP(infoPtr, bP->iBitmap), 
              bP->fsState, bP->fsStyle, bP->dwData, bP->iString);
	TRACE("string %s\n", debugstr_w(TOOLBAR_GetText(infoPtr,bP)));
	if (internal)
	    TRACE("button %d id %d, hot=%s, row=%d, rect=(%ld,%ld)-(%ld,%ld)\n",
		  btn_num, bP->idCommand,
		  (bP->bHot) ? "TRUE":"FALSE", bP->nRow,
		  bP->rect.left, bP->rect.top,
		  bP->rect.right, bP->rect.bottom);
    }
}


static void
TOOLBAR_DumpToolbar(TOOLBAR_INFO *iP, INT line)
{
    if (TRACE_ON(toolbar)) {
	INT i;
	DWORD dwStyle;

	dwStyle = GetWindowLongA (iP->hwndSelf, GWL_STYLE);
	TRACE("toolbar %p at line %d, exStyle=%08lx, buttons=%d, bitmaps=%d, strings=%d, style=%08lx\n",
	      iP->hwndSelf, line,
	      iP->dwExStyle, iP->nNumButtons, iP->nNumBitmaps,
	      iP->nNumStrings, dwStyle);
	TRACE("toolbar %p at line %d, himlInt=%p, himlDef=%p, himlHot=%p, himlDis=%p, redrawable=%s\n",
	      iP->hwndSelf, line,
	      iP->himlInt, iP->himlDef, iP->himlHot, iP->himlDis,
	      (iP->bDoRedraw) ? "TRUE" : "FALSE");
 	for(i=0; i<iP->nNumButtons; i++) {
	    TOOLBAR_DumpButton(iP, &iP->buttons[i], i, TRUE);
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
    if (dwStyle & TBSTYLE_ALTDRAG)
	FIXME("[%p] TBSTYLE_ALTDRAG not implemented\n", hwnd);
    if (dwStyle & TBSTYLE_REGISTERDROP)
	FIXME("[%p] TBSTYLE_REGISTERDROP not implemented\n", hwnd);
}


static INT
TOOLBAR_SendNotify (NMHDR *nmhdr, TOOLBAR_INFO *infoPtr, UINT code)
{
	if(!IsWindow(infoPtr->hwndSelf))
	    return 0;   /* we have just been destroyed */

    nmhdr->idFrom = GetDlgCtrlID (infoPtr->hwndSelf);
    nmhdr->hwndFrom = infoPtr->hwndSelf;
    nmhdr->code = code;

    TRACE("to window %p, code=%08x, %s\n", infoPtr->hwndNotify, code,
	  (infoPtr->bNtfUnicode) ? "via Unicode" : "via ANSI");

    if (infoPtr->bNtfUnicode)
	return SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
			     (WPARAM) nmhdr->idFrom, (LPARAM)nmhdr);
    else
	return SendMessageA (infoPtr->hwndNotify, WM_NOTIFY,
			     (WPARAM) nmhdr->idFrom, (LPARAM)nmhdr);
}

/***********************************************************************
* 		TOOLBAR_GetBitmapIndex
*
* This function returns the bitmap index associated with a button.
* If the button specifies I_IMAGECALLBACK, then the TBN_GETDISPINFO
* is issued to retrieve the index.
*/
static INT
TOOLBAR_GetBitmapIndex(TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr)
{
    INT ret = btnPtr->iBitmap;

    if (ret == I_IMAGECALLBACK) {
	/* issue TBN_GETDISPINFO */
	NMTBDISPINFOA nmgd;

	nmgd.idCommand = btnPtr->idCommand;
	nmgd.lParam = btnPtr->dwData;
	nmgd.dwMask = TBNF_IMAGE;
	TOOLBAR_SendNotify ((NMHDR *) &nmgd, infoPtr,
			(infoPtr->bNtfUnicode) ? TBN_GETDISPINFOW :
			TBN_GETDISPINFOA);
	if (nmgd.dwMask & TBNF_DI_SETITEM) {
	    btnPtr->iBitmap = nmgd.iImage;
	}
	ret = nmgd.iImage;
	TRACE("TBN_GETDISPINFOA returned bitmap id %d, mask=%08lx, nNumBitmaps=%d\n",
	      ret, nmgd.dwMask, infoPtr->nNumBitmaps);
    }

    if (ret != I_IMAGENONE)
        ret = GETIBITMAP(infoPtr, ret);

    return ret;
}


static BOOL
TOOLBAR_IsValidBitmapIndex(TOOLBAR_INFO *infoPtr, INT index)
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


/***********************************************************************
* 		TOOLBAR_DrawImageList
*
* This function validates the bitmap index (including I_IMAGECALLBACK
* functionality). It then draws the image via the ImageList_Draw
* function. It returns TRUE if the image was drawn, FALSE otherwise.
*/
static BOOL
TOOLBAR_DrawImageList (TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr, IMAGE_LIST_TYPE imagelist,
		        HDC hdc, UINT left, UINT top, UINT draw_flags)
{
    INT index;
    HIMAGELIST himl;

    if (!TOOLBAR_IsValidBitmapIndex(infoPtr,btnPtr->iBitmap)) {
	if (btnPtr->iBitmap == I_IMAGENONE) return FALSE;
	ERR("index %d is not valid, max %d\n",
	    btnPtr->iBitmap, infoPtr->nNumBitmaps);
	return FALSE;
    }

    if ((index = TOOLBAR_GetBitmapIndex(infoPtr, btnPtr)) < 0) {
	if ((index == I_IMAGECALLBACK) ||
	    (index == I_IMAGENONE)) return FALSE;
	ERR("TBN_GETDISPINFO returned invalid index %d\n",
	    index);
	return FALSE;
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
    {
       TRACE("no image list, returning FALSE\n");
       return FALSE;
    }

    TRACE("drawing index=%d, himl=%p, left=%d, top=%d, flags=%08x\n",
	  index, himl, left, top, draw_flags);

    ImageList_Draw (himl, index, hdc, left, top, draw_flags);
    return TRUE;
}


/***********************************************************************
* 		TOOLBAR_TestImageExist
*
* This function is similar to TOOLBAR_DrawImageList, except it does not
* draw the image. The I_IMAGECALLBACK functionality is implemented.
*/
static BOOL
TOOLBAR_TestImageExist (TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr, HIMAGELIST himl)
{
    INT index;

    if (!himl) return FALSE;

    if (!TOOLBAR_IsValidBitmapIndex(infoPtr,btnPtr->iBitmap)) {
	if (btnPtr->iBitmap == I_IMAGENONE) return FALSE;
	ERR("index %d is not valid, max %d\n",
	    btnPtr->iBitmap, infoPtr->nNumBitmaps);
	return FALSE;
    }

    if ((index = TOOLBAR_GetBitmapIndex(infoPtr, btnPtr)) < 0) {
	if ((index == I_IMAGECALLBACK) ||
	    (index == I_IMAGENONE)) return FALSE;
	ERR("TBN_GETDISPINFO returned invalid index %d\n",
	    index);
	return FALSE;
    }
    return TRUE;
}


static void
TOOLBAR_DrawFlatSeparator (LPRECT lpRect, HDC hdc, TOOLBAR_INFO *infoPtr)
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
    ExtTextOutA (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    myrect.left = myrect.right;
    myrect.right = myrect.left + 1;

    newcolor = (infoPtr->clrBtnHighlight == CLR_DEFAULT) ?
	        comctl32_color.clrBtnHighlight : infoPtr->clrBtnHighlight;
    SetBkColor (hdc, newcolor);
    ExtTextOutA (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    SetBkColor (hdc, oldcolor);
}


/***********************************************************************
* 		TOOLBAR_DrawDDFlatSeparator
*
* This function draws the separator that was flaged as TBSTYLE_DROPDOWN.
* In this case, the separator is a pixel high line of COLOR_BTNSHADOW,
* followed by a pixel high line of COLOR_BTNHIGHLIGHT. These separators
* are horizontal as opposed to the vertical separators for not dropdown
* type.
*
* FIXME: It is possible that the height of each line is really SM_CYBORDER.
*/
static void
TOOLBAR_DrawDDFlatSeparator (LPRECT lpRect, HDC hdc, TBUTTON_INFO *btnPtr, TOOLBAR_INFO *infoPtr)
{
    RECT myrect;
    COLORREF oldcolor, newcolor;

    myrect.left = lpRect->left;
    myrect.right = lpRect->right;
    myrect.top = lpRect->top + (lpRect->bottom - lpRect->top - 2)/2;
    myrect.bottom = myrect.top + 1;

    InflateRect (&myrect, -2, 0);

    TRACE("rect=(%ld,%ld)-(%ld,%ld)\n",
	  myrect.left, myrect.top, myrect.right, myrect.bottom);

    newcolor = (infoPtr->clrBtnShadow == CLR_DEFAULT) ?
	        comctl32_color.clrBtnShadow : infoPtr->clrBtnShadow;
    oldcolor = SetBkColor (hdc, newcolor);
    ExtTextOutA (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    myrect.top = myrect.bottom;
    myrect.bottom = myrect.top + 1;

    newcolor = (infoPtr->clrBtnHighlight == CLR_DEFAULT) ?
	        comctl32_color.clrBtnHighlight : infoPtr->clrBtnHighlight;
    SetBkColor (hdc, newcolor);
    ExtTextOutA (hdc, 0, 0, ETO_OPAQUE, &myrect, 0, 0, 0);

    SetBkColor (hdc, oldcolor);
}


static void
TOOLBAR_DrawArrow (HDC hdc, INT left, INT top, INT colorRef)
{
    INT x, y;
    HPEN hPen, hOldPen;

    if (!(hPen = CreatePen( PS_SOLID, 1, GetSysColor( colorRef )))) return;
    hOldPen = SelectObject ( hdc, hPen );
    x = left + 2;
    y = top;
    MoveToEx (hdc, x, y, NULL);
    LineTo (hdc, x+5, y++); x++;
    MoveToEx (hdc, x, y, NULL);
    LineTo (hdc, x+3, y++); x++;
    MoveToEx (hdc, x, y, NULL);
    LineTo (hdc, x+1, y++);
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
TOOLBAR_DrawString (TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr,
		    HDC hdc, DWORD dwStyle,
                    RECT *rcText, LPWSTR lpText, NMTBCUSTOMDRAW *tbcd)
{
    HFONT  hOldFont = 0;
    COLORREF clrOld = 0;
    UINT state = tbcd->nmcd.uItemState;

    /* draw text */
    if (lpText) {
	TRACE("string=%s rect=(%ld,%ld)-(%ld,%ld)\n", debugstr_w(lpText),
	      rcText->left, rcText->top, rcText->right, rcText->bottom);

	hOldFont = SelectObject (hdc, infoPtr->hFont);
	if ((state & CDIS_HOT) && (infoPtr->dwItemCDFlag & TBCDRF_HILITEHOTTRACK )) {
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
	else {
	    clrOld = SetTextColor (hdc, tbcd->clrText);
	}

	DrawTextW (hdc, lpText, -1, rcText, infoPtr->dwDTFlags);
	SetTextColor (hdc, clrOld);
	SelectObject (hdc, hOldFont);
    }
}


static void
TOOLBAR_DrawPattern (HDC hdc, LPRECT lpRect)
{
    HBRUSH hbr = SelectObject (hdc, COMCTL32_hPattern55AABrush);
    INT cx = lpRect->right - lpRect->left;
    INT cy = lpRect->bottom - lpRect->top;
    PatBlt (hdc, lpRect->left, lpRect->top, cx, cy, 0x00FA0089);
    SelectObject (hdc, hbr);
}


static void
TOOLBAR_DrawMasked (TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr,
		    HDC hdc, INT x, INT y)
{
    HIMAGELIST himl = GETDEFIMAGELIST(infoPtr, 0);
    INT cx, cy;
    HBITMAP hbmMask;
    HDC hdcMask;

    if (!himl)
	return;

    ImageList_GetIconSize(himl, &cx, &cy);

    /* create new dc's */
    hdcMask = CreateCompatibleDC (0);

    /* create new bitmap */
    hbmMask = CreateBitmap (cx, cy, 1, 1, NULL);
    SelectObject (hdcMask, hbmMask);

    /* copy the mask bitmap */
    ImageList_DrawEx(himl, btnPtr->iBitmap, hdcMask, 0, 0, 0, 0, RGB(255, 255, 255), RGB(0, 0, 0), ILD_MASK);

    /* draw the new mask */
    SelectObject (hdc, GetSysColorBrush (COLOR_3DHILIGHT));
    BitBlt (hdc, x+1, y+1, cx, cy, hdcMask, 0, 0, 0xB8074A);

    SelectObject (hdc, GetSysColorBrush (COLOR_3DSHADOW));
    BitBlt (hdc, x, y, cx, cy, hdcMask, 0, 0, 0xB8074A);

    DeleteObject (hbmMask);
    DeleteDC (hdcMask);
}


static UINT
TOOLBAR_TranslateState(TBUTTON_INFO *btnPtr)
{
    UINT retstate = 0;

    retstate |= (btnPtr->fsState & TBSTATE_CHECKED) ? CDIS_CHECKED  : 0;
    retstate |= (btnPtr->fsState & TBSTATE_PRESSED) ? CDIS_SELECTED : 0;
    retstate |= (btnPtr->fsState & TBSTATE_ENABLED) ? 0 : CDIS_DISABLED;
    retstate |= (btnPtr->fsState & TBSTATE_MARKED ) ? CDIS_MARKED   : 0;
    retstate |= (btnPtr->bHot                     ) ? CDIS_HOT      : 0;
    retstate |= (btnPtr->fsState & TBSTATE_INDETERMINATE) ? CDIS_INDETERMINATE : 0;
    /* FIXME: don't set CDIS_GRAYED, CDIS_FOCUS, CDIS_DEFAULT       */
    /*        don't test TBSTATE_HIDDEN                             */
    return retstate;
}


static void
TOOLBAR_DrawButton (HWND hwnd, TBUTTON_INFO *btnPtr, HDC hdc)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    BOOL hasDropDownArrow = TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle) &&
	                    (btnPtr->fsStyle & TBSTYLE_DROPDOWN);
    RECT rc, rcArrow, rcBitmap, rcText, rcFill;
    LPWSTR lpText = NULL;
    NMTBCUSTOMDRAW tbcd;
    DWORD ntfret;
    INT offset;

    if (btnPtr->fsState & TBSTATE_HIDDEN)
	return;

    rc = btnPtr->rect;
    CopyRect (&rcFill, &rc);
    CopyRect (&rcArrow, &rc);
    CopyRect(&rcBitmap, &rc);

    /* get a pointer to the text */
    lpText = TOOLBAR_GetText(infoPtr, btnPtr);

    if (hasDropDownArrow)
    {
	if (dwStyle & TBSTYLE_FLAT)
            rc.right = max(rc.left, rc.right - DDARROW_WIDTH);
	else
            rc.right = max(rc.left, rc.right - DDARROW_WIDTH - 2);
	rcArrow.left = rc.right;
    }

    /* copy text rect after adjusting for drop-down arrow
     * so that text is centred in the rectangle not containing
     * the arrow */
    CopyRect(&rcText, &rc);

    /* Center the bitmap horizontally and vertically */
    if (dwStyle & TBSTYLE_LIST)
	rcBitmap.left += LIST_IMAGE_OFFSET;
    else
	rcBitmap.left+=(infoPtr->nButtonWidth - infoPtr->nBitmapWidth) / 2;

    if(lpText)
        rcBitmap.top+=2; /* this looks to be the correct value from vmware comparison - cmm */
    else
        rcBitmap.top+=(infoPtr->nButtonHeight - infoPtr->nBitmapHeight) / 2;

    TRACE("iBitmap: %d, start=(%ld,%ld) w=%d, h=%d\n",
	  btnPtr->iBitmap, rcBitmap.left, rcBitmap.top,
	  infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);
    TRACE ("iString: %x\n", btnPtr->iString);
    TRACE ("Stringtext: %s\n", debugstr_w(lpText));

    /* draw text */
    if (lpText) {

	InflateRect (&rcText, -3, -3);

	if (GETDEFIMAGELIST(infoPtr, 0) &&
            TOOLBAR_IsValidBitmapIndex(infoPtr,btnPtr->iBitmap)) {
	        /* The following test looked like this before
		 * I changed it. IE4 "Links" toolbar would not
		 * draw correctly with the original code.  - GA 8/01
		 *   ((dwStyle & TBSTYLE_LIST) &&
		 *    ((btnPtr->fsStyle & TBSTYLE_AUTOSIZE) == 0) &&
		 *       (btnPtr->iBitmap != I_IMAGENONE))
		 */
	        if (dwStyle & TBSTYLE_LIST) {
		    /* LIST style w/ ICON offset is by matching native. */
		    /* Matches IE4 "Links" bar.   - GA 8/01             */
		    rcText.left += (infoPtr->nBitmapWidth + LIST_TEXT_OFFSET);
		}
		else {
		    rcText.top += infoPtr->nBitmapHeight + 1;
		}
	}
	else {
	        if (dwStyle & TBSTYLE_LIST) {
		    /* LIST style w/o ICON offset is by matching native. */
		    /* Matches IE4 "menu" bar.   - GA  8/01              */
		    rcText.left += LIST_IMAGE_ABSENT_WIDTH + LIST_TEXT_OFFSET;
		}
	}

	if (btnPtr->fsState & (TBSTATE_PRESSED | TBSTATE_CHECKED))
	    OffsetRect (&rcText, 1, 1);
    }

    /* Initialize fields in all cases, because we use these later */
    ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
    tbcd.clrText = comctl32_color.clrBtnText;
    tbcd.clrTextHighlight = comctl32_color.clrHighlightText;
    tbcd.clrBtnFace = comctl32_color.clrBtnFace;
    tbcd.clrBtnHighlight = comctl32_color.clrBtnHighlight;
    tbcd.clrMark = comctl32_color.clrHighlight;
    tbcd.clrHighlightHotTrack = 0;
    tbcd.nStringBkMode = (infoPtr->bBtnTranspnt) ? TRANSPARENT : OPAQUE;
    tbcd.nHLStringBkMode = (infoPtr->bBtnTranspnt) ? TRANSPARENT : OPAQUE;
    /* MSDN says that this is the text rectangle.              */
    /* But (why always a but) tracing of v5.7 of native shows  */
    /* that this is really a *relative* rectangle based on the */
    /* the nmcd.rc. Also the left and top are always 0 ignoring*/
    /* any bitmap that might be present.                       */
    tbcd.rcText.left = 0;
    tbcd.rcText.top = 0;
    tbcd.rcText.right = rcText.right - rc.left;
    tbcd.rcText.bottom = rcText.bottom - rc.top;
    /* we use this state later on to decide how to draw the buttons */
    /* NOTE: applications can and do alter this to customize their  */
    /* toolbars */
    tbcd.nmcd.uItemState = TOOLBAR_TranslateState(btnPtr);

    /* FIXME: what should these be set to ????? */
    tbcd.hbrMonoDither = 0;
    tbcd.hbrLines = 0;
    tbcd.hpenLines = 0;

    /* Issue Item Prepaint notify */
    infoPtr->dwItemCustDraw = 0;
    infoPtr->dwItemCDFlag = 0;
    if (infoPtr->dwBaseCustDraw & CDRF_NOTIFYITEMDRAW)
    {
	tbcd.nmcd.dwDrawStage = CDDS_ITEMPREPAINT;
	tbcd.nmcd.hdc = hdc;
	tbcd.nmcd.rc = rc;
	tbcd.nmcd.dwItemSpec = btnPtr->idCommand;
	tbcd.nmcd.lItemlParam = btnPtr->dwData;
	ntfret = TOOLBAR_SendNotify ((NMHDR *)&tbcd, infoPtr, NM_CUSTOMDRAW);
	infoPtr->dwItemCustDraw = ntfret & 0xffff;
	infoPtr->dwItemCDFlag = ntfret & 0xffff0000;
	if (infoPtr->dwItemCustDraw & CDRF_SKIPDEFAULT)
	    return;
	/* save the only part of the rect that the user can change */
	rcText.right = tbcd.rcText.right + rc.left;
	rcText.bottom = tbcd.rcText.bottom + rc.top;
    }

    if (!infoPtr->bBtnTranspnt)
	FillRect( hdc, &rcFill, GetSysColorBrush(COLOR_BTNFACE));

    /* separator */
    if (btnPtr->fsStyle & TBSTYLE_SEP) {
        /* with the FLAT style, iBitmap is the width and has already */
        /* been taken into consideration in calculating the width    */
        /* so now we need to draw the vertical separator             */
        /* empirical tests show that iBitmap can/will be non-zero    */
        /* when drawing the vertical bar...      */
        if ((dwStyle & TBSTYLE_FLAT) /* && (btnPtr->iBitmap == 0) */) {
	    if (btnPtr->fsStyle & TBSTYLE_DROPDOWN)
		TOOLBAR_DrawDDFlatSeparator (&rc, hdc, btnPtr, infoPtr);
	    else
		TOOLBAR_DrawFlatSeparator (&rc, hdc, infoPtr);
	}
	else if (btnPtr->fsStyle != TBSTYLE_SEP) {
	    FIXME("Draw some kind of separator: fsStyle=%x\n",
		  btnPtr->fsStyle);
	}
	goto FINALNOTIFY;
    }

    if ((dwStyle & TBSTYLE_FLAT) && (tbcd.nmcd.uItemState & CDIS_HOT))
    {
	if ( infoPtr->dwItemCDFlag & TBCDRF_HILITEHOTTRACK )
	{
            COLORREF oldclr;

            oldclr = SetBkColor(hdc, tbcd.clrHighlightHotTrack);
            ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, 0);
            if (hasDropDownArrow)
                ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rcArrow, NULL, 0, 0);
            SetBkColor(hdc, oldclr);
	}
	else
	{
            if (!(tbcd.nmcd.uItemState & CDIS_DISABLED) && !(infoPtr->dwItemCDFlag & TBCDRF_NOEDGES))
            {
                DrawEdge (hdc, &rc, BDR_RAISEDINNER, BF_RECT);
                if (hasDropDownArrow)
		    DrawEdge (hdc, &rcArrow, BDR_RAISEDINNER, BF_RECT);
            }
        }
    }

    /* disabled */
    if (tbcd.nmcd.uItemState & CDIS_DISABLED) {
	if (!(dwStyle & TBSTYLE_FLAT) && !(infoPtr->dwItemCDFlag & TBCDRF_NOEDGES))
	{
	    DrawEdge (hdc, &rc, EDGE_RAISED,
		      BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);
            if (hasDropDownArrow)
            DrawEdge (hdc, &rcArrow, EDGE_RAISED,
		      BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);
	}

        if (hasDropDownArrow)
	{
	    TOOLBAR_DrawArrow(hdc, rcArrow.left+1, rcArrow.top+1 + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, COLOR_3DHIGHLIGHT);
	    TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, COLOR_3DSHADOW);
	}

	if (!TOOLBAR_DrawImageList (infoPtr, btnPtr, IMAGE_LIST_DISABLED,
				   hdc, rcBitmap.left, rcBitmap.top,
				   ILD_NORMAL))
	    TOOLBAR_DrawMasked (infoPtr, btnPtr, hdc, rcBitmap.left, rcBitmap.top);

	if (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || (btnPtr->fsStyle & BTNS_SHOWTEXT))
	    TOOLBAR_DrawString (infoPtr, btnPtr, hdc, dwStyle, &rcText, lpText, &tbcd);
	goto FINALNOTIFY;
    }

    /* pressed TBSTYLE_BUTTON */
    if (tbcd.nmcd.uItemState & CDIS_SELECTED) {
	offset = (infoPtr->dwItemCDFlag & TBCDRF_NOOFFSET) ? 0 : 1;
	if (!(infoPtr->dwItemCDFlag & TBCDRF_NOEDGES))
	{
	    if (dwStyle & TBSTYLE_FLAT)
	    {
		DrawEdge (hdc, &rc, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
		if (hasDropDownArrow)
		    DrawEdge (hdc, &rcArrow, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
	    }
	    else
	    {
		DrawEdge (hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_MIDDLE | BF_ADJUST);
		if (hasDropDownArrow)
		    DrawEdge (hdc, &rcArrow, EDGE_SUNKEN, BF_RECT | BF_MIDDLE | BF_ADJUST);
	    }
	}

        if (hasDropDownArrow)
	    TOOLBAR_DrawArrow(hdc, rcArrow.left + offset, rcArrow.top + offset + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, COLOR_WINDOWFRAME);

	TOOLBAR_DrawImageList (infoPtr, btnPtr, IMAGE_LIST_DEFAULT,
			       hdc, rcBitmap.left+offset, rcBitmap.top+offset,
			       ILD_NORMAL);

	if (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || (btnPtr->fsStyle & BTNS_SHOWTEXT))
	    TOOLBAR_DrawString (infoPtr, btnPtr, hdc, dwStyle, &rcText, lpText, &tbcd);
	goto FINALNOTIFY;
    }

    /* checked TBSTYLE_CHECK */
    if ((tbcd.nmcd.uItemState & CDIS_CHECKED) &&
	(btnPtr->fsStyle & TBSTYLE_CHECK)) {
	if (!(infoPtr->dwItemCDFlag & TBCDRF_NOEDGES))
	{
	    if (dwStyle & TBSTYLE_FLAT)
		DrawEdge (hdc, &rc, BDR_SUNKENOUTER,
			  BF_RECT | BF_ADJUST);
	    else
		DrawEdge (hdc, &rc, EDGE_SUNKEN,
			  BF_RECT | BF_MIDDLE | BF_ADJUST);
	}

	TOOLBAR_DrawPattern (hdc, &rc);

	TOOLBAR_DrawImageList (infoPtr, btnPtr, IMAGE_LIST_DEFAULT,
			       hdc, rcBitmap.left+1, rcBitmap.top+1,
			       ILD_NORMAL);

	if (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || (btnPtr->fsStyle & BTNS_SHOWTEXT))
	    TOOLBAR_DrawString (infoPtr, btnPtr, hdc, dwStyle, &rcText, lpText, &tbcd);
	goto FINALNOTIFY;
    }

    /* indeterminate */
    if (tbcd.nmcd.uItemState & CDIS_INDETERMINATE) {
	if (!(infoPtr->dwItemCDFlag & TBCDRF_NOEDGES))
	    DrawEdge (hdc, &rc, EDGE_RAISED,
		      BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);

	TOOLBAR_DrawPattern (hdc, &rc);
	TOOLBAR_DrawMasked (infoPtr, btnPtr, hdc, rcBitmap.left, rcBitmap.top);
	if (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || (btnPtr->fsStyle & BTNS_SHOWTEXT))
	    TOOLBAR_DrawString (infoPtr, btnPtr, hdc, dwStyle, &rcText, lpText, &tbcd);
	goto FINALNOTIFY;
    }

    /* normal state */
    if (dwStyle & TBSTYLE_FLAT)
    {
        if (hasDropDownArrow)
	    TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, COLOR_WINDOWFRAME);

	if (tbcd.nmcd.uItemState & CDIS_HOT) {
	    /* if hot, attempt to draw with hot image list, if fails, 
	       use default image list */
	    if (!TOOLBAR_DrawImageList (infoPtr, btnPtr,
					IMAGE_LIST_HOT,
					hdc, rcBitmap.left,
					rcBitmap.top, ILD_NORMAL))
		TOOLBAR_DrawImageList (infoPtr, btnPtr, IMAGE_LIST_DEFAULT,
				       hdc, rcBitmap.left, rcBitmap.top,
				       ILD_NORMAL);
	}
	else
	    TOOLBAR_DrawImageList (infoPtr, btnPtr, IMAGE_LIST_DEFAULT,
				   hdc, rcBitmap.left, rcBitmap.top,
				   ILD_NORMAL);
    }
    else
    {
	if (!(infoPtr->dwItemCDFlag & TBCDRF_NOEDGES))
	    DrawEdge (hdc, &rc, EDGE_RAISED,
		      BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);

        if (hasDropDownArrow)
	{
	    if (!(infoPtr->dwItemCDFlag & TBCDRF_NOEDGES))
		DrawEdge (hdc, &rcArrow, EDGE_RAISED,
			  BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);
	    TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, COLOR_WINDOWFRAME);
	}

	TOOLBAR_DrawImageList (infoPtr, btnPtr, IMAGE_LIST_DEFAULT,
			       hdc, rcBitmap.left, rcBitmap.top,
			       ILD_NORMAL);
    }


    if (!(infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || (btnPtr->fsStyle & BTNS_SHOWTEXT))
        TOOLBAR_DrawString (infoPtr, btnPtr, hdc, dwStyle, &rcText, lpText, &tbcd);

 FINALNOTIFY:
    if (infoPtr->dwItemCustDraw & CDRF_NOTIFYPOSTPAINT)
    {
	tbcd.nmcd.dwDrawStage = CDDS_ITEMPOSTPAINT;
	tbcd.nmcd.hdc = hdc;
	tbcd.nmcd.rc = rc;
	tbcd.nmcd.dwItemSpec = btnPtr->idCommand;
	tbcd.nmcd.uItemState = TOOLBAR_TranslateState(btnPtr);
	tbcd.nmcd.lItemlParam = btnPtr->dwData;
	tbcd.rcText = rcText;
	tbcd.nStringBkMode = (infoPtr->bBtnTranspnt) ? TRANSPARENT : OPAQUE;
	tbcd.nHLStringBkMode = (infoPtr->bBtnTranspnt) ? TRANSPARENT : OPAQUE;
	ntfret = TOOLBAR_SendNotify ((NMHDR *)&tbcd, infoPtr, NM_CUSTOMDRAW);
    }

}


static void
TOOLBAR_Refresh (HWND hwnd, HDC hdc, PAINTSTRUCT* ps)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT i, oldBKmode = 0;
    RECT rcTemp, rcClient;
    NMTBCUSTOMDRAW tbcd;
    DWORD ntfret;

    /* if imagelist belongs to the app, it can be changed
       by the app after setting it */
    if (GETDEFIMAGELIST(infoPtr, 0) != infoPtr->himlInt)
    {
        infoPtr->nNumBitmaps = 0;
        for (i = 0; i < infoPtr->cimlDef; i++)
            infoPtr->nNumBitmaps += ImageList_GetImageCount(infoPtr->himlDef[i]->himl);
    }

    TOOLBAR_DumpToolbar (infoPtr, __LINE__);

    /* Send initial notify */
    ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
    tbcd.nmcd.dwDrawStage = CDDS_PREPAINT;
    tbcd.nmcd.hdc = hdc;
    tbcd.nmcd.rc = ps->rcPaint;
    ntfret = TOOLBAR_SendNotify ((NMHDR *)&tbcd, infoPtr, NM_CUSTOMDRAW);
    infoPtr->dwBaseCustDraw = ntfret & 0xffff;

    if (infoPtr->bBtnTranspnt)
	oldBKmode = SetBkMode (hdc, TRANSPARENT);

    GetClientRect(hwnd, &rcClient);

    /* redraw necessary buttons */
    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++)
    {
        BOOL bDraw;
        if (infoPtr->dwExStyle & TBSTYLE_EX_HIDECLIPPEDBUTTONS)
        {
            IntersectRect(&rcTemp, &rcClient, &btnPtr->rect);
            bDraw = EqualRect(&rcTemp, &btnPtr->rect);
        }
        else
            bDraw = TRUE;
        bDraw &= IntersectRect(&rcTemp, &(ps->rcPaint), &(btnPtr->rect));
        if (bDraw)
            TOOLBAR_DrawButton (hwnd, btnPtr, hdc);
    }

    if (infoPtr->bBtnTranspnt && (oldBKmode != TRANSPARENT))
	SetBkMode (hdc, oldBKmode);

    if (infoPtr->dwBaseCustDraw & CDRF_NOTIFYPOSTPAINT)
    {
	ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
	tbcd.nmcd.dwDrawStage = CDDS_POSTPAINT;
	tbcd.nmcd.hdc = hdc;
	tbcd.nmcd.rc = ps->rcPaint;
	ntfret = TOOLBAR_SendNotify ((NMHDR *)&tbcd, infoPtr, NM_CUSTOMDRAW);
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
* only DrawText does. Note that the TBSTYLE_NOPREFIX is handled here.
*/
static void
TOOLBAR_MeasureString(TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr,
		      HDC hdc, LPSIZE lpSize)
{
    RECT myrect;

    lpSize->cx = 0;
    lpSize->cy = 0;

    if (!(btnPtr->fsState & TBSTATE_HIDDEN) &&
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
	    	   DT_CALCRECT | ((btnPtr->fsStyle & TBSTYLE_NOPREFIX) ?
				  DT_NOPREFIX : 0));

	    /* feed back to caller  */
	    lpSize->cx = myrect.right;
	    lpSize->cy = myrect.bottom;
	}
    }

    TRACE("string size %ld x %ld!\n", lpSize->cx, lpSize->cy);
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

    hdc = GetDC (hwnd);
    hOldFont = SelectObject (hdc, infoPtr->hFont);

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

    TRACE("max string size %ld x %ld!\n", lpSize->cx, lpSize->cy);
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
    BOOL bWrap, bButtonWrap;

    /* 	When the toolbar window style is not TBSTYLE_WRAPABLE,	*/
    /*	no layout is necessary. Applications may use this style */
    /*	to perform their own layout on the toolbar. 		*/
    if( !(dwStyle & TBSTYLE_WRAPABLE) &&
	!(infoPtr->dwExStyle & TBSTYLE_EX_UNDOC1) )  return;

    btnPtr = infoPtr->buttons;
    x  = infoPtr->nIndent;

    /* this can get the parents width, to know how far we can extend
     * this toolbar.  We cannot use its height, as there may be multiple
     * toolbars in a rebar control
     */
    GetClientRect( GetParent(hwnd), &rc );
    infoPtr->nWidth = rc.right - rc.left;
    bButtonWrap = FALSE;

    TRACE("start ButtonWidth=%d, BitmapWidth=%d, nWidth=%d, nIndent=%d\n",
	  infoPtr->nButtonWidth, infoPtr->nBitmapWidth, infoPtr->nWidth,
	  infoPtr->nIndent);

    for (i = 0; i < infoPtr->nNumButtons; i++ )
    {
	bWrap = FALSE;
	btnPtr[i].fsState &= ~TBSTATE_WRAP;

	if (btnPtr[i].fsState & TBSTATE_HIDDEN)
	    continue;

	/* UNDOCUMENTED: If a separator has a non zero bitmap index, */
	/* it is the actual width of the separator. This is used for */
	/* custom controls in toolbars.                              */
	/*                                                           */
	/* TBSTYLE_DROPDOWN separators are treated as buttons for    */
	/* width.  - GA 8/01                                         */
	if ((btnPtr[i].fsStyle & TBSTYLE_SEP) &&
	    !(btnPtr[i].fsStyle & TBSTYLE_DROPDOWN))
	    cx = (btnPtr[i].iBitmap > 0) ?
			btnPtr[i].iBitmap : SEPARATOR_WIDTH;
	else
	    cx = infoPtr->nButtonWidth;

	/* Two or more adjacent separators form a separator group.   */
	/* The first separator in a group should be wrapped to the   */
	/* next row if the previous wrapping is on a button.	     */
	if( bButtonWrap &&
		(btnPtr[i].fsStyle & TBSTYLE_SEP) &&
		(i + 1 < infoPtr->nNumButtons ) &&
		(btnPtr[i + 1].fsStyle & TBSTYLE_SEP) )
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
	    while( ( ((btnPtr[i].fsStyle & TBSTYLE_SEP) &&
		      !(btnPtr[i].fsStyle & TBSTYLE_DROPDOWN)) ||
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
		if ((btnPtr[j].fsStyle & TBSTYLE_SEP) &&
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
		bFound = TRUE;
		x = infoPtr->nIndent;
		if (btnPtr[i].fsStyle & TBSTYLE_SEP )
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
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    TBUTTON_INFO *btnPtr;
    INT i, nRows, nSepRows;
    INT x, y, cx, cy;
    SIZE  sizeString;
    BOOL bWrap;
    BOOL usesBitmaps = FALSE;
    BOOL hasDropDownArrows = TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle);

    TOOLBAR_CalcStrings (hwnd, &sizeString);

    for (i = 0; i < infoPtr->nNumButtons && !usesBitmaps; i++)
    {
	if (TOOLBAR_IsValidBitmapIndex(infoPtr,infoPtr->buttons[i].iBitmap))
	    usesBitmaps = TRUE;
    }
    if (dwStyle & TBSTYLE_LIST)
    {
	infoPtr->nButtonHeight = max((usesBitmaps) ? infoPtr->nBitmapHeight :
				     0, sizeString.cy) + infoPtr->szPadding.cy;
	infoPtr->nButtonWidth = ((usesBitmaps) ? infoPtr->nBitmapWidth :
				 0) + sizeString.cx + 6;
	TRACE("LIST style, But w=%d h=%d, useBitmaps=%d, Bit w=%d h=%d\n",
	      infoPtr->nButtonWidth, infoPtr->nButtonHeight, usesBitmaps,
	      infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);
	TOOLBAR_DumpToolbar (infoPtr, __LINE__);
    }
    else {
        if (sizeString.cy > 0)
        {
            if (usesBitmaps)
		infoPtr->nButtonHeight = sizeString.cy +
		    2 + /* this is the space to separate text from bitmap */
                  infoPtr->nBitmapHeight + 6;
            else
                infoPtr->nButtonHeight = sizeString.cy + 6;
        }
        else if (infoPtr->nButtonHeight < infoPtr->nBitmapHeight + 6)
	    infoPtr->nButtonHeight = infoPtr->nBitmapHeight + 6;

        if (sizeString.cx > infoPtr->nBitmapWidth)
	    infoPtr->nButtonWidth = sizeString.cx + 6;
        else if (infoPtr->nButtonWidth < infoPtr->nBitmapWidth + 6)
	    infoPtr->nButtonWidth = infoPtr->nBitmapWidth + 6;
    }

    if ( infoPtr->cxMin >= 0 && infoPtr->nButtonWidth < infoPtr->cxMin )
        infoPtr->nButtonWidth = infoPtr->cxMin;
    if ( infoPtr->cxMax > 0 && infoPtr->nButtonWidth > infoPtr->cxMax )
        infoPtr->nButtonWidth = infoPtr->cxMax;

    TOOLBAR_WrapToolbar( hwnd, dwStyle );

    x  = infoPtr->nIndent;
    y  = 0;

   /*
    * We will set the height below, and we set the width on entry
    * so we do not reset them here..
    */
#if 0
    GetClientRect( hwnd, &rc );
    /* get initial values for toolbar */
    infoPtr->nWidth  = rc.right - rc.left;
    infoPtr->nHeight = rc.bottom - rc.top;
#endif

    /* from above, minimum is a button, and possible text */
    cx = infoPtr->nButtonWidth;

    /* cannot use just ButtonHeight, we may have no buttons! */
    if (infoPtr->nNumButtons > 0)
        infoPtr->nHeight = infoPtr->nButtonHeight;

    cy = infoPtr->nHeight;

    nRows = nSepRows = 0;

    infoPtr->rcBound.top = y;
    infoPtr->rcBound.left = x;
    infoPtr->rcBound.bottom = y + cy;
    infoPtr->rcBound.right = x;

    btnPtr = infoPtr->buttons;

    /* do not base height/width on parent, if the parent is a */
    /* rebar control it could have multiple rows of toolbars  */
/*    GetClientRect( GetParent(hwnd), &rc ); */
/*    cx = rc.right - rc.left; */
/*    cy = rc.bottom - rc.top; */

    TRACE("cy=%d\n", cy);

    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++ )
    {
	bWrap = FALSE;
	if (btnPtr->fsState & TBSTATE_HIDDEN)
	{
	    SetRectEmpty (&btnPtr->rect);
	    continue;
	}

	cy = infoPtr->nHeight;

	/* UNDOCUMENTED: If a separator has a non zero bitmap index, */
	/* it is the actual width of the separator. This is used for */
	/* custom controls in toolbars.                              */
	if (btnPtr->fsStyle & TBSTYLE_SEP) {
	    if (btnPtr->fsStyle & TBSTYLE_DROPDOWN) {
		cy = (btnPtr->iBitmap > 0) ?
		     btnPtr->iBitmap : SEPARATOR_WIDTH;
		cx = infoPtr->nButtonWidth;
	    }
	    else
		cx = (btnPtr->iBitmap > 0) ?
		     btnPtr->iBitmap : SEPARATOR_WIDTH;
	}
	else
	{
            if ((infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || 
                (btnPtr->fsStyle & TBSTYLE_AUTOSIZE))
            {
              SIZE sz;
	      HDC hdc;
	      HFONT hOldFont;

	      hdc = GetDC (hwnd);
	      hOldFont = SelectObject (hdc, infoPtr->hFont);

              TOOLBAR_MeasureString(infoPtr, btnPtr, hdc, &sz);

	      SelectObject (hdc, hOldFont);
	      ReleaseDC (hwnd, hdc);

              if (sz.cx > 0)
                  sz.cx += 2*LIST_TEXT_OFFSET;
              cx = sz.cx + 2*LIST_IMAGE_OFFSET;
              if (TOOLBAR_TestImageExist (infoPtr, btnPtr, GETDEFIMAGELIST(infoPtr,0)))
              {
                if (dwStyle & TBSTYLE_LIST)
                  cx += infoPtr->nBitmapWidth;
                else if (cx < (infoPtr->nBitmapWidth+7))
                  cx = infoPtr->nBitmapWidth+7;
              }
              else if (dwStyle & TBSTYLE_LIST)
                  cx += LIST_IMAGE_ABSENT_WIDTH;
            }
            else
	      cx = infoPtr->nButtonWidth;

	    if (hasDropDownArrows && (btnPtr->fsStyle & TBSTYLE_DROPDOWN))
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

	/* Set the toolTip only for non-hidden, non-separator button */
	if (infoPtr->hwndToolTip && !(btnPtr->fsStyle & TBSTYLE_SEP ))
	{
	    TTTOOLINFOA ti;

	    ZeroMemory (&ti, sizeof(TTTOOLINFOA));
	    ti.cbSize = sizeof(TTTOOLINFOA);
	    ti.hwnd = hwnd;
	    ti.uId = btnPtr->idCommand;
	    ti.rect = btnPtr->rect;
	    SendMessageA (infoPtr->hwndToolTip, TTM_NEWTOOLRECTA,
			    0, (LPARAM)&ti);
	}

	/* btnPtr->nRow is zero based. The space between the rows is 	*/
	/* also considered as a row. 					*/
	btnPtr->nRow = nRows + nSepRows;

	TRACE("button %d style=%x, bWrap=%d, nRows=%d, nSepRows=%d, btnrow=%d, (%d,%d)-(%d,%d)\n",
	      i, btnPtr->fsStyle, bWrap, nRows, nSepRows, btnPtr->nRow,
	      x, y, x+cx, y+cy);

	if( bWrap )
	{
	    if ( !(btnPtr->fsStyle & TBSTYLE_SEP) )
	        y += cy;
	    else
	    {
		/* UNDOCUMENTED: If a separator has a non zero bitmap index, */
		/* it is the actual width of the separator. This is used for */
		/* custom controls in toolbars. 			     */
		if ( !(btnPtr->fsStyle & TBSTYLE_DROPDOWN))
		    y += cy + ( (btnPtr->iBitmap > 0 ) ?
				btnPtr->iBitmap : SEPARATOR_WIDTH) * 2 /3;
		else
		    y += cy;

		/* nSepRows is used to calculate the extra height follwoing  */
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

#if 0
    /********************************************************************
     * The following while interesting, does not match the values       *
     * created above for the button rectangles, nor the rcBound rect.   *
     * We will comment it out and remove it later.                      *
     *                                                                  *
     * The problem showed up as heights in the pager control that was   *
     * wrong.                                                           *
     ********************************************************************/

    /* nSepRows * (infoPtr->nBitmapHeight + 1) is the space following 	*/
    /* the last row. 							*/
    infoPtr->nHeight = TOP_BORDER + (nRows + 1) * infoPtr->nButtonHeight +
		       	nSepRows * (SEPARATOR_WIDTH * 2 / 3) +
			nSepRows * (infoPtr->nBitmapHeight + 1) +
			BOTTOM_BORDER;
#endif

    infoPtr->nHeight = infoPtr->rcBound.bottom - infoPtr->rcBound.top;

    TRACE("toolbar height %d, button width %d\n", infoPtr->nHeight, infoPtr->nButtonWidth);
}


static INT
TOOLBAR_InternalHitTest (HWND hwnd, LPPOINT lpPt)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT i;

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->fsState & TBSTATE_HIDDEN)
	    continue;

	if (btnPtr->fsStyle & TBSTYLE_SEP) {
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
    return -1;
}


static INT
TOOLBAR_GetButtonIndex (TOOLBAR_INFO *infoPtr, INT idCommand, BOOL CommandIsIndex)
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
TOOLBAR_GetCheckedGroupButtonIndex (TOOLBAR_INFO *infoPtr, INT nIndex)
{
    TBUTTON_INFO *btnPtr;
    INT nRunIndex;

    if ((nIndex < 0) || (nIndex > infoPtr->nNumButtons))
	return -1;

    /* check index button */
    btnPtr = &infoPtr->buttons[nIndex];
    if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
	if (btnPtr->fsState & TBSTATE_CHECKED)
	    return nIndex;
    }

    /* check previous buttons */
    nRunIndex = nIndex - 1;
    while (nRunIndex >= 0) {
	btnPtr = &infoPtr->buttons[nRunIndex];
	if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
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
	if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
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
    msg.pt.x = LOWORD(GetMessagePos ());
    msg.pt.y = HIWORD(GetMessagePos ());

    SendMessageA (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}


/***********************************************************************
 * TOOLBAR_CustomizeDialogProc
 * This function implements the toolbar customization dialog.
 */
static INT_PTR CALLBACK
TOOLBAR_CustomizeDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PCUSTDLG_INFO custInfo = (PCUSTDLG_INFO)GetWindowLongA (hwnd, DWL_USER);
    PCUSTOMBUTTON btnInfo;
    NMTOOLBARA nmtb;
    TOOLBAR_INFO *infoPtr = custInfo ? custInfo->tbInfo : NULL;

    switch (uMsg)
    {
	case WM_INITDIALOG:
	    custInfo = (PCUSTDLG_INFO)lParam;
	    SetWindowLongA (hwnd, DWL_USER, (DWORD)custInfo);

	    if (custInfo)
	    {
		WCHAR Buffer[256];
		int i = 0;
		int index;

		infoPtr = custInfo->tbInfo;

		/* send TBN_QUERYINSERT notification */
		nmtb.iItem = custInfo->tbInfo->nNumButtons;

		if (!TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr, TBN_QUERYINSERT))
		    return FALSE;

		/* UNDOCUMENTED: dialog hwnd immediately follows NMHDR */
		nmtb.iItem = (int)hwnd;
		/* Send TBN_INITCUSTOMIZE notification */
		if (TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr, TBN_INITCUSTOMIZE) ==
		    TBNRF_HIDEHELP)
                {
                    TRACE("TBNRF_HIDEHELP requested\n");
                    ShowWindow(GetDlgItem(hwnd, IDC_HELP_BTN), SW_HIDE);
                }

		/* add items to 'toolbar buttons' list and check if removable */
		for (i = 0; i < custInfo->tbInfo->nNumButtons; i++)
                {
		    btnInfo = (PCUSTOMBUTTON)Alloc(sizeof(CUSTOMBUTTON));
                    memset (&btnInfo->btn, 0, sizeof(TBBUTTON));
                    btnInfo->btn.fsStyle = TBSTYLE_SEP;
                    btnInfo->bVirtual = FALSE;
		    LoadStringW (COMCTL32_hModule, IDS_SEPARATOR, btnInfo->text, 64);

		    /* send TBN_QUERYDELETE notification */
                    btnInfo->bRemovable = TOOLBAR_IsButtonRemovable(infoPtr, i, btnInfo);

		    index = (int)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_ADDSTRING, 0, 0);
		    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);
		}

		/* insert separator button into 'available buttons' list */
		btnInfo = (PCUSTOMBUTTON)Alloc(sizeof(CUSTOMBUTTON));
		memset (&btnInfo->btn, 0, sizeof(TBBUTTON));
		btnInfo->btn.fsStyle = TBSTYLE_SEP;
		btnInfo->bVirtual = FALSE;
		btnInfo->bRemovable = TRUE;
		LoadStringW (COMCTL32_hModule, IDS_SEPARATOR, btnInfo->text, 64);
		index = (int)SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_ADDSTRING, 0, (LPARAM)btnInfo);
		SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);

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

		    TRACE("WM_INITDIALOG style: %x iItem(%d) idCommand(%d) iString(%d) %s\n", 
                        nmtb.tbButton.fsStyle, i, 
                        nmtb.tbButton.idCommand,
                        nmtb.tbButton.iString,
                        nmtb.tbButton.iString >= 0 ? debugstr_w(infoPtr->strings[nmtb.tbButton.iString])
                        : "");

		    /* insert button into the apropriate list */
		    index = TOOLBAR_GetButtonIndex (custInfo->tbInfo, nmtb.tbButton.idCommand, FALSE);
		    if (index == -1)
		    {
			btnInfo = (PCUSTOMBUTTON)Alloc(sizeof(CUSTOMBUTTON));
			btnInfo->bVirtual = FALSE;
			btnInfo->bRemovable = TRUE;

			index = SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_ADDSTRING, 0, 0);
			SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, 
				LB_SETITEMDATA, index, (LPARAM)btnInfo);
		    }
		    else
		    {
                        btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, 
                            IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, index, 0);
                    }

                    memcpy (&btnInfo->btn, &nmtb.tbButton, sizeof(TBBUTTON));
                    if (!(nmtb.tbButton.fsStyle & TBSTYLE_SEP))
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
		}

		/* select first item in the 'available' list */
		SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_SETCURSEL, 0, 0);

		/* append 'virtual' separator button to the 'toolbar buttons' list */
		btnInfo = (PCUSTOMBUTTON)Alloc(sizeof(CUSTOMBUTTON));
		memset (&btnInfo->btn, 0, sizeof(TBBUTTON));
		btnInfo->btn.fsStyle = TBSTYLE_SEP;
		btnInfo->bVirtual = TRUE;
		btnInfo->bRemovable = FALSE;
		LoadStringW (COMCTL32_hModule, IDS_SEPARATOR, btnInfo->text, 64);
		index = (int)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_ADDSTRING, 0, (LPARAM)btnInfo);
		SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);

		/* select last item in the 'toolbar' list */
		SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETCURSEL, index, 0);
		SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETTOPINDEX, index, 0);

		/* set focus and disable buttons */
		PostMessageA (hwnd, WM_USER, 0, 0);
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

			count = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCOUNT, 0, 0);
			index = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);

			/* send TBN_QUERYINSERT notification */
			nmtb.iItem = index;
		        TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
					TBN_QUERYINSERT);

			/* get list box item */
			btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, index, 0);

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
			PCUSTOMBUTTON btnInfo;
			int index;
			int count;

			count = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCOUNT, 0, 0);
			index = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);
			TRACE("Move up: index %d\n", index);

			/* send TBN_QUERYINSERT notification */
			nmtb.iItem = index;

		        if (TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
					    TBN_QUERYINSERT))
			{
			    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, index, 0);

			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_DELETESTRING, index, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_INSERTSTRING, index-1, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, index-1, (LPARAM)btnInfo);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETCURSEL, index-1 , 0);

			    if (index <= 1)
				EnableWindow (GetDlgItem (hwnd,IDC_MOVEUP_BTN), FALSE);
			    else if (index >= (count - 3))
				EnableWindow (GetDlgItem (hwnd,IDC_MOVEDN_BTN), TRUE);

			    SendMessageA (custInfo->tbHwnd, TB_DELETEBUTTON, index, 0);
			    SendMessageA (custInfo->tbHwnd, TB_INSERTBUTTONA, index-1, (LPARAM)&(btnInfo->btn));
			}
		    }
		    break;

		case IDC_MOVEDN_BTN: /* move down */
		    {
			PCUSTOMBUTTON btnInfo;
			int index;
			int count;

			count = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCOUNT, 0, 0);
			index = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);
			TRACE("Move up: index %d\n", index);

			/* send TBN_QUERYINSERT notification */
			nmtb.iItem = index;
		        if (TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
					    TBN_QUERYINSERT))
			{
			    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, index, 0);

			    /* move button down */
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_DELETESTRING, index, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_INSERTSTRING, index+1, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, index+1, (LPARAM)btnInfo);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETCURSEL, index+1 , 0);

			    if (index == 0)
				EnableWindow (GetDlgItem (hwnd,IDC_MOVEUP_BTN), TRUE);
			    else if (index >= (count - 3))
				EnableWindow (GetDlgItem (hwnd,IDC_MOVEDN_BTN), FALSE);

			    SendMessageA (custInfo->tbHwnd, TB_DELETEBUTTON, index, 0);
			    SendMessageA (custInfo->tbHwnd, TB_INSERTBUTTONA, index+1, (LPARAM)&(btnInfo->btn));
			}
		    }
		    break;

		case IDC_REMOVE_BTN: /* remove button */
		    {
			PCUSTOMBUTTON btnInfo;
			int index;

			index = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);

			if (LB_ERR == index)
				break;

			TRACE("Remove: index %d\n", index);

			btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, 
				LB_GETITEMDATA, index, 0);

			/* send TBN_QUERYDELETE notification */
			if (TOOLBAR_IsButtonRemovable(infoPtr, index, btnInfo))
			{
			    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, index, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_DELETESTRING, index, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETCURSEL, index , 0);

			    SendMessageA (custInfo->tbHwnd, TB_DELETEBUTTON, index, 0);

			    /* insert into 'available button' list */
			    if (!(btnInfo->btn.fsStyle & TBSTYLE_SEP))
			    {
				index = (int)SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_ADDSTRING, 0, 0);
				SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);
			    }
			    else
				Free (btnInfo);
			}
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
			int count;

			count = SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_GETCOUNT, 0, 0);
			index = SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_GETCURSEL, 0, 0);
			TRACE("Add: index %d\n", index);

			/* send TBN_QUERYINSERT notification */
			nmtb.iItem = index;
		        if (TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
					    TBN_QUERYINSERT))
			{
			    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_GETITEMDATA, index, 0);

			    if (index != 0)
			    {
				/* remove from 'available buttons' list */
				SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_DELETESTRING, index, 0);
				if (index == count-1)
				    SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_SETCURSEL, index-1 , 0);
				else
				    SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_SETCURSEL, index , 0);
			    }
			    else
			    {
				PCUSTOMBUTTON btnNew;

				/* duplicate 'separator' button */
				btnNew = (PCUSTOMBUTTON)Alloc (sizeof(CUSTOMBUTTON));
				memcpy (btnNew, btnInfo, sizeof(CUSTOMBUTTON));
				btnInfo = btnNew;
			    }

			    /* insert into 'toolbar button' list */
			    index = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCURSEL, 0, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_INSERTSTRING, index, 0);
			    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, index, (LPARAM)btnInfo);

			    SendMessageA (custInfo->tbHwnd, TB_INSERTBUTTONA, index, (LPARAM)&(btnInfo->btn));
			}
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
		count = SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETCOUNT, 0, 0);
		for (i = 0; i < count; i++)
		{
		    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_GETITEMDATA, i, 0);
		    Free(btnInfo);
		    SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_SETITEMDATA, 0, 0);
		}
		SendDlgItemMessageA (hwnd, IDC_TOOLBARBTN_LBOX, LB_RESETCONTENT, 0, 0);


		/* delete items from 'available buttons' listbox*/
		count = SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_GETCOUNT, 0, 0);
		for (i = 0; i < count; i++)
		{
		    btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_GETITEMDATA, i, 0);
		    Free(btnInfo);
		    SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_SETITEMDATA, i, 0);
		}
		SendDlgItemMessageA (hwnd, IDC_AVAILBTN_LBOX, LB_RESETCONTENT, 0, 0);
            }
	    return TRUE;

	case WM_DRAWITEM:
	    if (wParam == IDC_AVAILBTN_LBOX || wParam == IDC_TOOLBARBTN_LBOX)
	    {
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    	DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);
		RECT rcButton;
		RECT rcText;
		HPEN hPen, hOldPen;
		HBRUSH hOldBrush;
		COLORREF oldText = 0;
		COLORREF oldBk = 0;

		/* get item data */
		btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageA (hwnd, wParam, LB_GETITEMDATA, (WPARAM)lpdis->itemID, 0);
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
		if (!(dwStyle & TBSTYLE_FLAT))
		DrawEdge (lpdis->hDC, &rcButton, EDGE_RAISED, BF_RECT|BF_MIDDLE|BF_SOFT);

		/* draw image and text */
		if ((btnInfo->btn.fsStyle & TBSTYLE_SEP) == 0) {
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

		if (custInfo && custInfo->tbInfo)
		    lpmis->itemHeight = custInfo->tbInfo->nBitmapHeight + 8;
		else
		    lpmis->itemHeight = 15 + 8; /* default height */

		return TRUE;
	    }
	    return FALSE;

	default:
	    return FALSE;
    }
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
    INT nIndex = 0, nButtons, nCount;
    HBITMAP hbmLoad;
    HIMAGELIST himlDef;

    TRACE("hwnd=%p wParam=%x lParam=%lx\n", hwnd, wParam, lParam);
    if (!lpAddBmp)
	return -1;

    if (lpAddBmp->hInst == HINST_COMMCTRL)
    {
	if ((lpAddBmp->nID & ~1) == IDB_STD_SMALL_COLOR)
	    nButtons = 15;
	else if ((lpAddBmp->nID & ~1) == IDB_VIEW_SMALL_COLOR)
	    nButtons = 13;
	else if ((lpAddBmp->nID & ~1) == IDB_HIST_SMALL_COLOR)
	    nButtons = 5;
	else
	    return -1;

	TRACE ("adding %d internal bitmaps!\n", nButtons);

	/* Windows resize all the buttons to the size of a newly added standard image */
	if (lpAddBmp->nID & 1)
	{
	    /* large icons */
	    /* FIXME: on windows the size of the images is 25x24 but the size of the bitmap
             * in rsrc is only 24x24. Fix the bitmap (how?) and then fix this
             */
	    SendMessageA (hwnd, TB_SETBITMAPSIZE, 0,
			  MAKELPARAM((WORD)24, (WORD)24));
	    SendMessageA (hwnd, TB_SETBUTTONSIZE, 0,
			  MAKELPARAM((WORD)31, (WORD)30));
	}
	else
	{
	    /* small icons */
	    SendMessageA (hwnd, TB_SETBITMAPSIZE, 0,
			  MAKELPARAM((WORD)16, (WORD)16));
	    SendMessageA (hwnd, TB_SETBUTTONSIZE, 0,
			  MAKELPARAM((WORD)22, (WORD)22));
	}

	TOOLBAR_CalcToolbar (hwnd);
    }
    else
    {
	nButtons = (INT)wParam;
	if (nButtons <= 0)
	    return -1;

	TRACE ("adding %d bitmaps!\n", nButtons);
    }

    if (!infoPtr->cimlDef) {
	/* create new default image list */
	TRACE ("creating default image list!\n");

    himlDef = ImageList_Create (infoPtr->nBitmapWidth, infoPtr->nBitmapHeight, 
		ILC_COLOR | ILC_MASK, nButtons, 2);
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

    nCount = ImageList_GetImageCount(himlDef);

    /* Add bitmaps to the default image list */
    if (lpAddBmp->hInst == NULL)
    {
       BITMAP  bmp;
       HBITMAP hOldBitmapBitmap, hOldBitmapLoad;
       HDC     hdcImage, hdcBitmap;

       /* copy the bitmap before adding it so that the user's bitmap
        * doesn't get modified.
        */
       GetObjectA ((HBITMAP)lpAddBmp->nID, sizeof(BITMAP), (LPVOID)&bmp);

       hdcImage  = CreateCompatibleDC(0);
       hdcBitmap = CreateCompatibleDC(0);

       /* create new bitmap */
       hbmLoad = CreateBitmap (bmp.bmWidth, bmp.bmHeight, bmp.bmPlanes, bmp.bmBitsPixel, NULL);
       hOldBitmapBitmap = SelectObject(hdcBitmap, (HBITMAP)lpAddBmp->nID);
       hOldBitmapLoad = SelectObject(hdcImage, hbmLoad);

       /* Copy the user's image */
       BitBlt (hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight,
               hdcBitmap, 0, 0, SRCCOPY);

       SelectObject (hdcImage, hOldBitmapLoad);
       SelectObject (hdcBitmap, hOldBitmapBitmap);
       DeleteDC (hdcImage);
       DeleteDC (hdcBitmap);

       nIndex = ImageList_AddMasked (himlDef, hbmLoad, comctl32_color.clrBtnFace);
       DeleteObject (hbmLoad);
    }
    else if (lpAddBmp->hInst == HINST_COMMCTRL)
    {
	/* Add system bitmaps */
	switch (lpAddBmp->nID)
    {
	    case IDB_STD_SMALL_COLOR:
		hbmLoad = LoadBitmapA (COMCTL32_hModule,
				       MAKEINTRESOURCEA(IDB_STD_SMALL));
		nIndex = ImageList_AddMasked (himlDef,
					      hbmLoad, comctl32_color.clrBtnFace);
		DeleteObject (hbmLoad);
		break;

	    case IDB_STD_LARGE_COLOR:
		hbmLoad = LoadBitmapA (COMCTL32_hModule,
				       MAKEINTRESOURCEA(IDB_STD_LARGE));
		nIndex = ImageList_AddMasked (himlDef,
					      hbmLoad, comctl32_color.clrBtnFace);
		DeleteObject (hbmLoad);
		break;

	    case IDB_VIEW_SMALL_COLOR:
		hbmLoad = LoadBitmapA (COMCTL32_hModule,
				       MAKEINTRESOURCEA(IDB_VIEW_SMALL));
		nIndex = ImageList_AddMasked (himlDef,
					      hbmLoad, comctl32_color.clrBtnFace);
		DeleteObject (hbmLoad);
		break;

	    case IDB_VIEW_LARGE_COLOR:
		hbmLoad = LoadBitmapA (COMCTL32_hModule,
				       MAKEINTRESOURCEA(IDB_VIEW_LARGE));
		nIndex = ImageList_AddMasked (himlDef,
					      hbmLoad, comctl32_color.clrBtnFace);
		DeleteObject (hbmLoad);
		break;

	    case IDB_HIST_SMALL_COLOR:
		hbmLoad = LoadBitmapA (COMCTL32_hModule,
				       MAKEINTRESOURCEA(IDB_HIST_SMALL));
		nIndex = ImageList_AddMasked (himlDef,
					      hbmLoad, comctl32_color.clrBtnFace);
		DeleteObject (hbmLoad);
		break;

	    case IDB_HIST_LARGE_COLOR:
		hbmLoad = LoadBitmapA (COMCTL32_hModule,
				       MAKEINTRESOURCEA(IDB_HIST_LARGE));
		nIndex = ImageList_AddMasked (himlDef,
					      hbmLoad, comctl32_color.clrBtnFace);
		DeleteObject (hbmLoad);
		break;

	    default:
	nIndex = ImageList_GetImageCount (himlDef);
		ERR ("invalid imagelist!\n");
		break;
	}
    }
    else
    {
	hbmLoad = LoadBitmapA (lpAddBmp->hInst, (LPSTR)lpAddBmp->nID);
	nIndex = ImageList_AddMasked (himlDef, hbmLoad, comctl32_color.clrBtnFace);
	DeleteObject (hbmLoad);
    }

    TRACE("Number of bitmap infos: %d\n", infoPtr->nNumBitmapInfos);

    if (infoPtr->nNumBitmapInfos == 0)
    {
        infoPtr->bitmaps = Alloc(sizeof(TBITMAP_INFO));
    }
    else
    {
        TBITMAP_INFO *oldBitmaps = infoPtr->bitmaps;
        infoPtr->bitmaps = Alloc((infoPtr->nNumBitmapInfos + 1) * sizeof(TBITMAP_INFO));
        memcpy(&infoPtr->bitmaps[0], &oldBitmaps[0], infoPtr->nNumBitmapInfos);
    }

    infoPtr->bitmaps[infoPtr->nNumBitmapInfos].nButtons = nButtons;
    infoPtr->bitmaps[infoPtr->nNumBitmapInfos].hInst = lpAddBmp->hInst;
    infoPtr->bitmaps[infoPtr->nNumBitmapInfos].nID = lpAddBmp->nID;

    infoPtr->nNumBitmapInfos++;
    TRACE("Number of bitmap infos: %d\n", infoPtr->nNumBitmapInfos);

    if (nIndex != -1)
    {
       INT imagecount = ImageList_GetImageCount(himlDef);

       if (infoPtr->nNumBitmaps + nButtons != imagecount)
       {
         WARN("Desired images do not match received images : Previous image number %i Previous images in list %i added %i expecting total %i, Images in list %i\n",
	      infoPtr->nNumBitmaps, nCount, imagecount - nCount,
	      infoPtr->nNumBitmaps+nButtons,imagecount);

	 infoPtr->nNumBitmaps = imagecount;
       }
       else
         infoPtr->nNumBitmaps += nButtons;
    }

    InvalidateRect(hwnd, NULL, FALSE);

    return nIndex;
}


static LRESULT
TOOLBAR_AddButtonsA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT nOldButtons, nNewButtons, nAddButtons, nCount;

    TRACE("adding %d buttons!\n", wParam);

    nAddButtons = (UINT)wParam;
    nOldButtons = infoPtr->nNumButtons;
    nNewButtons = nOldButtons + nAddButtons;

    if (infoPtr->nNumButtons == 0) {
	infoPtr->buttons =
	    Alloc (sizeof(TBUTTON_INFO) * nNewButtons);
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
	infoPtr->buttons =
	    Alloc (sizeof(TBUTTON_INFO) * nNewButtons);
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nOldButtons * sizeof(TBUTTON_INFO));
        Free (oldButtons);
    }

    infoPtr->nNumButtons = nNewButtons;

    /* insert new button data */
    for (nCount = 0; nCount < nAddButtons; nCount++) {
	TBUTTON_INFO *btnPtr = &infoPtr->buttons[nOldButtons+nCount];
	btnPtr->iBitmap   = lpTbb[nCount].iBitmap;
	btnPtr->idCommand = lpTbb[nCount].idCommand;
	btnPtr->fsState   = lpTbb[nCount].fsState;
	btnPtr->fsStyle   = lpTbb[nCount].fsStyle;
	btnPtr->dwData    = lpTbb[nCount].dwData;
	btnPtr->iString   = lpTbb[nCount].iString;
	btnPtr->bHot      = FALSE;

	if ((infoPtr->hwndToolTip) && !(btnPtr->fsStyle & TBSTYLE_SEP)) {
	    TTTOOLINFOA ti;

	    ZeroMemory (&ti, sizeof(TTTOOLINFOA));
	    ti.cbSize   = sizeof (TTTOOLINFOA);
	    ti.hwnd     = hwnd;
	    ti.uId      = btnPtr->idCommand;
	    ti.hinst    = 0;
	    ti.lpszText = LPSTR_TEXTCALLBACKA;

	    SendMessageA (infoPtr->hwndToolTip, TTM_ADDTOOLA,
			    0, (LPARAM)&ti);
	}
    }

    TOOLBAR_CalcToolbar (hwnd);

    TOOLBAR_DumpToolbar (infoPtr, __LINE__);

    InvalidateRect(hwnd, NULL, FALSE);

    return TRUE;
}


static LRESULT
TOOLBAR_AddButtonsW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT nOldButtons, nNewButtons, nAddButtons, nCount;

    TRACE("adding %d buttons!\n", wParam);

    nAddButtons = (UINT)wParam;
    nOldButtons = infoPtr->nNumButtons;
    nNewButtons = nOldButtons + nAddButtons;

    if (infoPtr->nNumButtons == 0) {
	infoPtr->buttons =
	    Alloc (sizeof(TBUTTON_INFO) * nNewButtons);
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
	infoPtr->buttons =
	    Alloc (sizeof(TBUTTON_INFO) * nNewButtons);
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nOldButtons * sizeof(TBUTTON_INFO));
        Free (oldButtons);
    }

    infoPtr->nNumButtons = nNewButtons;

    /* insert new button data */
    for (nCount = 0; nCount < nAddButtons; nCount++) {
	TBUTTON_INFO *btnPtr = &infoPtr->buttons[nOldButtons+nCount];
	btnPtr->iBitmap   = lpTbb[nCount].iBitmap;
	btnPtr->idCommand = lpTbb[nCount].idCommand;
	btnPtr->fsState   = lpTbb[nCount].fsState;
	btnPtr->fsStyle   = lpTbb[nCount].fsStyle;
	btnPtr->dwData    = lpTbb[nCount].dwData;
	btnPtr->iString   = lpTbb[nCount].iString;
	btnPtr->bHot      = FALSE;

	if ((infoPtr->hwndToolTip) && !(btnPtr->fsStyle & TBSTYLE_SEP)) {
	    TTTOOLINFOW ti;

	    ZeroMemory (&ti, sizeof(TTTOOLINFOW));
	    ti.cbSize   = sizeof (TTTOOLINFOW);
	    ti.hwnd     = hwnd;
	    ti.uId      = btnPtr->idCommand;
	    ti.hinst    = 0;
	    ti.lpszText = LPSTR_TEXTCALLBACKW;
	    ti.lParam   = lParam;

	    SendMessageW (infoPtr->hwndToolTip, TTM_ADDTOOLW,
			    0, (LPARAM)&ti);
	}
    }

    TOOLBAR_CalcToolbar (hwnd);

    TOOLBAR_DumpToolbar (infoPtr, __LINE__);

    InvalidateRect(hwnd, NULL, FALSE);

    return TRUE;
}


static LRESULT
TOOLBAR_AddStringA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    if ((wParam) && (HIWORD(lParam) == 0)) {
	char szString[256];
	INT len;
	TRACE("adding string from resource!\n");

	len = LoadStringA ((HINSTANCE)wParam, (UINT)lParam,
			     szString, 256);

	TRACE("len=%d \"%s\"\n", len, szString);
	nIndex = infoPtr->nNumStrings;
	if (infoPtr->nNumStrings == 0) {
	    infoPtr->strings =
		Alloc (sizeof(LPWSTR));
	}
	else {
	    LPWSTR *oldStrings = infoPtr->strings;
	    infoPtr->strings =
		Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
	    memcpy (&infoPtr->strings[0], &oldStrings[0],
		    sizeof(LPWSTR) * infoPtr->nNumStrings);
	    Free (oldStrings);
	}

        /*Alloc zeros out the allocated memory*/
        Str_SetPtrAtoW (&infoPtr->strings[infoPtr->nNumStrings], szString );
	infoPtr->nNumStrings++;
    }
    else {
	LPSTR p = (LPSTR)lParam;
	INT len;

	if (p == NULL)
	    return -1;
	TRACE("adding string(s) from array!\n");

	nIndex = infoPtr->nNumStrings;
	while (*p) {
	    len = strlen (p);
	    TRACE("len=%d \"%s\"\n", len, p);

	    if (infoPtr->nNumStrings == 0) {
		infoPtr->strings =
		    Alloc (sizeof(LPWSTR));
	    }
	    else {
		LPWSTR *oldStrings = infoPtr->strings;
		infoPtr->strings =
		    Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
		memcpy (&infoPtr->strings[0], &oldStrings[0],
			sizeof(LPWSTR) * infoPtr->nNumStrings);
		Free (oldStrings);
	    }

            Str_SetPtrAtoW (&infoPtr->strings[infoPtr->nNumStrings], p );
	    infoPtr->nNumStrings++;

	    p += (len+1);
	}
    }

    return nIndex;
}


static LRESULT
TOOLBAR_AddStringW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#define MAX_RESOURCE_STRING_LENGTH 512
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    if ((wParam) && (HIWORD(lParam) == 0)) {
	WCHAR szString[MAX_RESOURCE_STRING_LENGTH];
	INT len;
	TRACE("adding string from resource!\n");

	len = LoadStringW ((HINSTANCE)wParam, (UINT)lParam,
			     szString, MAX_RESOURCE_STRING_LENGTH);

	TRACE("len=%d %s\n", len, debugstr_w(szString));
	TRACE("First char: 0x%x\n", *szString);
	if (szString[0] == L'|')
	{
	    PWSTR p = szString + 1;

            nIndex = infoPtr->nNumStrings;
            while (*p != L'|' && *p != L'\0') {
                PWSTR np;

                if (infoPtr->nNumStrings == 0) {
                    infoPtr->strings = Alloc (sizeof(LPWSTR));
                }
                else
                {
                    LPWSTR *oldStrings = infoPtr->strings;
                    infoPtr->strings = Alloc(sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
                    memcpy(&infoPtr->strings[0], &oldStrings[0],
                           sizeof(LPWSTR) * infoPtr->nNumStrings);
                    Free(oldStrings);
                }

                np=strchrW (p, '|');
                if (np!=NULL) {
                    len = np - p;
                    np++;
                } else {
                    len = strlenW(p);
                    np = p + len;
                }
                TRACE("len=%d %s\n", len, debugstr_w(p));
                infoPtr->strings[infoPtr->nNumStrings] =
                    Alloc (sizeof(WCHAR)*(len+1));
                lstrcpynW (infoPtr->strings[infoPtr->nNumStrings], p, len+1);
                infoPtr->nNumStrings++;

                p = np;
            }
	}
	else
	{
            nIndex = infoPtr->nNumStrings;
            if (infoPtr->nNumStrings == 0) {
                infoPtr->strings =
                    Alloc (sizeof(LPWSTR));
            }
            else {
                LPWSTR *oldStrings = infoPtr->strings;
                infoPtr->strings =
                    Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
                memcpy (&infoPtr->strings[0], &oldStrings[0],
                        sizeof(LPWSTR) * infoPtr->nNumStrings);
                Free (oldStrings);
            }

            Str_SetPtrW (&infoPtr->strings[infoPtr->nNumStrings], szString);
            infoPtr->nNumStrings++;
        }
    }
    else {
	LPWSTR p = (LPWSTR)lParam;
	INT len;

	if (p == NULL)
	    return -1;
	TRACE("adding string(s) from array!\n");
	nIndex = infoPtr->nNumStrings;
	while (*p) {
	    len = strlenW (p);

	    TRACE("len=%d %s\n", len, debugstr_w(p));
	    if (infoPtr->nNumStrings == 0) {
		infoPtr->strings =
		    Alloc (sizeof(LPWSTR));
	    }
	    else {
		LPWSTR *oldStrings = infoPtr->strings;
		infoPtr->strings =
		    Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
		memcpy (&infoPtr->strings[0], &oldStrings[0],
			sizeof(LPWSTR) * infoPtr->nNumStrings);
		Free (oldStrings);
	    }

	    Str_SetPtrW (&infoPtr->strings[infoPtr->nNumStrings], p);
	    infoPtr->nNumStrings++;

	    p += (len+1);
	}
    }

    return nIndex;
}


static LRESULT
TOOLBAR_AutoSize (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT parent_rect;
    RECT window_rect;
    HWND parent;
    INT  x, y;
    INT  cx, cy;
    UINT uPosFlags = SWP_NOZORDER;

    TRACE("resize forced, style=%lx!\n", dwStyle);

    parent = GetParent (hwnd);
    GetClientRect(parent, &parent_rect);

    x = parent_rect.left;
    y = parent_rect.top;

    /* FIXME: we should be able to early out if nothing */
    /* has changed with nWidth != parent_rect width */

    if (dwStyle & CCS_NORESIZE) {
	uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);
	cx = 0;
	cy = 0;
	TOOLBAR_CalcToolbar (hwnd);
    }
    else {
	infoPtr->nWidth = parent_rect.right - parent_rect.left;
	TOOLBAR_CalcToolbar (hwnd);
	InvalidateRect( hwnd, NULL, TRUE );
	cy = infoPtr->nHeight;
	cx = infoPtr->nWidth;

	if ((dwStyle & CCS_BOTTOM) == CCS_NOMOVEY) {
		GetWindowRect(hwnd, &window_rect);
		ScreenToClient(parent, (LPPOINT)&window_rect.left);
		y = window_rect.top;
	}
	if ((dwStyle & CCS_BOTTOM) == CCS_BOTTOM) {
            GetWindowRect(hwnd, &window_rect);
            y = parent_rect.bottom - ( window_rect.bottom - window_rect.top);
        }
    }

    if (dwStyle & CCS_NOPARENTALIGN)
	uPosFlags |= SWP_NOMOVE;

    if (!(dwStyle & CCS_NODIVIDER))
	cy += GetSystemMetrics(SM_CYEDGE);

    if (dwStyle & WS_BORDER)
    {
        x = y = 1;
        cy += GetSystemMetrics(SM_CYEDGE);
        cx += GetSystemMetrics(SM_CYEDGE);
    }

    infoPtr->bAutoSize = TRUE;
    SetWindowPos (hwnd, HWND_TOP,  x, y, cx, cy, uPosFlags);
    /* The following line makes sure that the infoPtr->bAutoSize is turned off
     * after the setwindowpos calls */
    infoPtr->bAutoSize = FALSE;

    return 0;
}


static LRESULT
TOOLBAR_ButtonCount (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return infoPtr->nNumButtons;
}


static LRESULT
TOOLBAR_ButtonStructSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (infoPtr == NULL) {
	ERR("(%p, 0x%x, 0x%lx)\n", hwnd, wParam, lParam);
	ERR("infoPtr == NULL!\n");
	return 0;
    }

    infoPtr->dwStructSize = (DWORD)wParam;

    return 0;
}


static LRESULT
TOOLBAR_ChangeBitmap (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    TRACE("button %d, iBitmap now %d\n", wParam, LOWORD(lParam));

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
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    if (!(btnPtr->fsStyle & TBSTYLE_CHECK))
	return FALSE;

    bChecked = (btnPtr->fsState & TBSTATE_CHECKED) ? TRUE : FALSE;

    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_CHECKED;
    else {
	if (btnPtr->fsStyle & TBSTYLE_GROUP) {
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
        {
            InvalidateRect(hwnd, &infoPtr->buttons[nOldIndex].rect,
                TOOLBAR_HasText(infoPtr, &infoPtr->buttons[nOldIndex]));
        }
        InvalidateRect(hwnd, &btnPtr->rect, TRUE);
    }

    /* FIXME: Send a WM_NOTIFY?? */

    return TRUE;
}


static LRESULT
TOOLBAR_CommandToIndex (HWND hwnd, WPARAM wParam, LPARAM lParam)
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
    TOOLBAR_SendNotify ((NMHDR *) &nmhdr, infoPtr,
		    TBN_BEGINADJUST);

    if (!(hRes = FindResourceA (COMCTL32_hModule,
                                MAKEINTRESOURCEA(IDD_TBCUSTOMIZE),
                                (LPSTR)RT_DIALOG)))
	return FALSE;

    if(!(template = (LPVOID)LoadResource (COMCTL32_hModule, hRes)))
	return FALSE;

    ret = DialogBoxIndirectParamA ((HINSTANCE)GetWindowLongA(hwnd, GWL_HINSTANCE),
                                   (LPDLGTEMPLATEA)template,
                                   hwnd,
                                   TOOLBAR_CustomizeDialogProc,
                                   (LPARAM)&custInfo);

    /* send TBN_ENDADJUST notification */
    TOOLBAR_SendNotify ((NMHDR *) &nmhdr, infoPtr,
		    TBN_ENDADJUST);

    return ret;
}


static LRESULT
TOOLBAR_DeleteButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex = (INT)wParam;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    if ((infoPtr->hwndToolTip) &&
	!(infoPtr->buttons[nIndex].fsStyle & TBSTYLE_SEP)) {
	TTTOOLINFOA ti;

	ZeroMemory (&ti, sizeof(TTTOOLINFOA));
	ti.cbSize   = sizeof (TTTOOLINFOA);
	ti.hwnd     = hwnd;
	ti.uId      = infoPtr->buttons[nIndex].idCommand;

	SendMessageA (infoPtr->hwndToolTip, TTM_DELTOOLA, 0, (LPARAM)&ti);
    }

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

    TOOLBAR_CalcToolbar (hwnd);

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
    {
        InvalidateRect(hwnd, &btnPtr->rect,
            TOOLBAR_HasText(infoPtr, btnPtr));
    }

    return TRUE;
}


static inline LRESULT
TOOLBAR_GetAnchorHighlight (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return infoPtr->bAnchor;
}


static LRESULT
TOOLBAR_GetBitmap (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return infoPtr->buttons[nIndex].iBitmap;
}


static inline LRESULT
TOOLBAR_GetBitmapFlags (HWND hwnd, WPARAM wParam, LPARAM lParam)
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

    if (infoPtr == NULL)
	return FALSE;

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
TOOLBAR_GetButtonInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTONINFOA lpTbInfo = (LPTBBUTTONINFOA)lParam;
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    if (infoPtr == NULL)
	return -1;
    if (lpTbInfo == NULL)
	return -1;
    if (lpTbInfo->cbSize < sizeof(TBBUTTONINFOA))
	return -1;

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
	lpTbInfo->cx = (WORD)(btnPtr->rect.right - btnPtr->rect.left);
    if (lpTbInfo->dwMask & TBIF_STATE)
	lpTbInfo->fsState = btnPtr->fsState;
    if (lpTbInfo->dwMask & TBIF_STYLE)
	lpTbInfo->fsStyle = btnPtr->fsStyle;
     if (lpTbInfo->dwMask & TBIF_TEXT) {
         LPWSTR lpText = TOOLBAR_GetText(infoPtr,btnPtr);
	 Str_GetPtrWtoA (lpText, lpTbInfo->pszText,lpTbInfo->cchText);
         }
    return nIndex;
}


static LRESULT
TOOLBAR_GetButtonInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTONINFOW lpTbInfo = (LPTBBUTTONINFOW)lParam;
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    if (infoPtr == NULL)
	return -1;
    if (lpTbInfo == NULL)
	return -1;
    if (lpTbInfo->cbSize < sizeof(TBBUTTONINFOW))
	return -1;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam,
				     lpTbInfo->dwMask & 0x80000000);
    if (nIndex == -1)
	return -1;

    btnPtr = &infoPtr->buttons[nIndex];

    if(!btnPtr)
        return -1;

    if (lpTbInfo->dwMask & TBIF_COMMAND)
	lpTbInfo->idCommand = btnPtr->idCommand;
    if (lpTbInfo->dwMask & TBIF_IMAGE)
	lpTbInfo->iImage = btnPtr->iBitmap;
    if (lpTbInfo->dwMask & TBIF_LPARAM)
	lpTbInfo->lParam = btnPtr->dwData;
    if (lpTbInfo->dwMask & TBIF_SIZE)
	lpTbInfo->cx = (WORD)(btnPtr->rect.right - btnPtr->rect.left);
    if (lpTbInfo->dwMask & TBIF_STATE)
	lpTbInfo->fsState = btnPtr->fsState;
    if (lpTbInfo->dwMask & TBIF_STYLE)
	lpTbInfo->fsStyle = btnPtr->fsStyle;
    if (lpTbInfo->dwMask & TBIF_TEXT) {
	LPWSTR lpText = TOOLBAR_GetText(infoPtr,btnPtr);
	Str_GetPtrW (lpText,lpTbInfo->pszText,lpTbInfo->cchText);
    }

    return nIndex;
}


static LRESULT
TOOLBAR_GetButtonSize (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (infoPtr->nNumButtons > 0)
	return MAKELONG((WORD)infoPtr->nButtonWidth,
			(WORD)infoPtr->nButtonHeight);
    else
	return MAKELONG(8,7);
}


static LRESULT
TOOLBAR_GetButtonTextA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;
    LPWSTR lpText;

    if (lParam == 0)
	return -1;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    lpText = TOOLBAR_GetText(infoPtr,&infoPtr->buttons[nIndex]);

    return WideCharToMultiByte( CP_ACP, 0, lpText, -1,
                                (LPSTR)lParam, 0x7fffffff, NULL, NULL ) - 1;
}


static LRESULT
TOOLBAR_GetButtonTextW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;
    LPWSTR lpText;

    if (lParam == 0)
	return -1;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    lpText = TOOLBAR_GetText(infoPtr,&infoPtr->buttons[nIndex]);

    strcpyW ((LPWSTR)lParam, lpText);

    return strlenW (lpText);
}


static LRESULT
TOOLBAR_GetDisabledImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT)GETDISIMAGELIST(TOOLBAR_GetInfoPtr (hwnd), 0);
}


inline static LRESULT
TOOLBAR_GetExtendedStyle (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("\n");

    return infoPtr->dwExStyle;
}


static LRESULT
TOOLBAR_GetHotImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT)GETHOTIMAGELIST(TOOLBAR_GetInfoPtr (hwnd), 0);
}


static LRESULT
TOOLBAR_GetHotItem (HWND hwnd)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (!(GetWindowLongA (hwnd, GWL_STYLE) & TBSTYLE_FLAT))
	return -1;

    if (infoPtr->nHotItem < 0)
	return -1;

    return (LRESULT)infoPtr->nHotItem;
}


static LRESULT
TOOLBAR_GetDefImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT) GETDEFIMAGELIST(TOOLBAR_GetInfoPtr(hwnd), 0);
}


/* << TOOLBAR_GetInsertMark >> */
/* << TOOLBAR_GetInsertMarkColor >> */


static LRESULT
TOOLBAR_GetItemRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    LPRECT     lpRect;
    INT        nIndex;

    if (infoPtr == NULL)
	return FALSE;
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
TOOLBAR_GetMaxSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPSIZE lpSize = (LPSIZE)lParam;

    if (lpSize == NULL)
	return FALSE;

    lpSize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
    lpSize->cy = infoPtr->rcBound.bottom - infoPtr->rcBound.top;

    TRACE("maximum size %ld x %ld\n",
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

    if (infoPtr == NULL)
	return FALSE;
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
TOOLBAR_GetRows (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (GetWindowLongA (hwnd, GWL_STYLE) & TBSTYLE_WRAPABLE)
	return infoPtr->nRows;
    else
	return 1;
}


static LRESULT
TOOLBAR_GetState (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return infoPtr->buttons[nIndex].fsState;
}


static LRESULT
TOOLBAR_GetStyle (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return -1;

    return infoPtr->buttons[nIndex].fsStyle;
}


static LRESULT
TOOLBAR_GetTextRows (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (infoPtr == NULL)
	return 0;

    return infoPtr->nMaxTextRows;
}


static LRESULT
TOOLBAR_GetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (infoPtr == NULL)
	return 0;
    return (LRESULT)infoPtr->hwndToolTip;
}


static LRESULT
TOOLBAR_GetUnicodeFormat (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("%s hwnd=%p stub!\n",
	   infoPtr->bUnicode ? "TRUE" : "FALSE", hwnd);

    return infoPtr->bUnicode;
}


inline static LRESULT
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

    TOOLBAR_CalcToolbar (hwnd);

    InvalidateRect (hwnd, NULL, TRUE);

    return TRUE;
}


inline static LRESULT
TOOLBAR_HitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return TOOLBAR_InternalHitTest (hwnd, (LPPOINT)lParam);
}


static LRESULT
TOOLBAR_Indeterminate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_INDETERMINATE;
    else
	btnPtr->fsState |= TBSTATE_INDETERMINATE;

    InvalidateRect(hwnd, &btnPtr->rect, TOOLBAR_HasText(infoPtr, btnPtr));

    return TRUE;
}


static LRESULT
TOOLBAR_InsertButtonA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT nIndex = (INT)wParam;
    TBUTTON_INFO *oldButtons;

    if (lpTbb == NULL)
	return FALSE;

    TOOLBAR_DumpButton(infoPtr, (TBUTTON_INFO *)lpTbb, nIndex, FALSE);

    if (nIndex == -1) {
       /* EPP: this seems to be an undocumented call (from my IE4)
	* I assume in that case that:
	* - lpTbb->iString is a string pointer (not a string index in strings[] table
	* - index of insertion is at the end of existing buttons
	* I only see this happen with nIndex == -1, but it could have a special
	* meaning (like -nIndex (or ~nIndex) to get the real position of insertion).
	*/
	nIndex = infoPtr->nNumButtons;

    } else if (nIndex < 0)
       return FALSE;

    /* If the string passed is not an index, assume address of string
       and do our own AddString */
    if ((HIWORD(lpTbb->iString) != 0) && (lpTbb->iString != -1)) {
        LPSTR ptr;
	INT len;

        TRACE("string %s passed instead of index, adding string\n",
              debugstr_a((LPSTR)lpTbb->iString));
        len = strlen((LPSTR)lpTbb->iString) + 2;
        ptr = Alloc(len);
        strcpy(ptr, (LPSTR)lpTbb->iString);
        ptr[len - 1] = 0; /* ended by two '\0' */
        lpTbb->iString = TOOLBAR_AddStringA(hwnd, 0, (LPARAM)ptr);
        Free(ptr);
    }

    TRACE("inserting button index=%d\n", nIndex);
    if (nIndex > infoPtr->nNumButtons) {
	nIndex = infoPtr->nNumButtons;
	TRACE("adjust index=%d\n", nIndex);
    }

    oldButtons = infoPtr->buttons;
    infoPtr->nNumButtons++;
    infoPtr->buttons = Alloc (sizeof (TBUTTON_INFO) * infoPtr->nNumButtons);
    /* pre insert copy */
    if (nIndex > 0) {
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nIndex * sizeof(TBUTTON_INFO));
    }

    /* insert new button */
    infoPtr->buttons[nIndex].iBitmap   = lpTbb->iBitmap;
    infoPtr->buttons[nIndex].idCommand = lpTbb->idCommand;
    infoPtr->buttons[nIndex].fsState   = lpTbb->fsState;
    infoPtr->buttons[nIndex].fsStyle   = lpTbb->fsStyle;
    infoPtr->buttons[nIndex].dwData    = lpTbb->dwData;
    /* if passed string and not index, then add string */
    if(HIWORD(lpTbb->iString) && lpTbb->iString!=-1) {
        Str_SetPtrAtoW ((LPWSTR *)&infoPtr->buttons[nIndex].iString, (LPCSTR )lpTbb->iString);
    }
    else
        infoPtr->buttons[nIndex].iString   = lpTbb->iString;

    if ((infoPtr->hwndToolTip) && !(lpTbb->fsStyle & TBSTYLE_SEP)) {
	TTTOOLINFOA ti;

	ZeroMemory (&ti, sizeof(TTTOOLINFOA));
	ti.cbSize   = sizeof (TTTOOLINFOA);
	ti.hwnd     = hwnd;
	ti.uId      = lpTbb->idCommand;
	ti.hinst    = 0;
	ti.lpszText = LPSTR_TEXTCALLBACKA;

	SendMessageA (infoPtr->hwndToolTip, TTM_ADDTOOLA,
			0, (LPARAM)&ti);
    }

    /* post insert copy */
    if (nIndex < infoPtr->nNumButtons - 1) {
	memcpy (&infoPtr->buttons[nIndex+1], &oldButtons[nIndex],
		(infoPtr->nNumButtons - nIndex - 1) * sizeof(TBUTTON_INFO));
    }

    Free (oldButtons);

    TOOLBAR_CalcToolbar (hwnd);

    InvalidateRect (hwnd, NULL, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_InsertButtonW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT nIndex = (INT)wParam;
    TBUTTON_INFO *oldButtons;

    if (lpTbb == NULL)
	return FALSE;

    TOOLBAR_DumpButton(infoPtr, (TBUTTON_INFO *)lpTbb, nIndex, FALSE);

    if (nIndex == -1) {
       /* EPP: this seems to be an undocumented call (from my IE4)
	* I assume in that case that:
	* - lpTbb->iString is a string pointer (not a string index in strings[] table
	* - index of insertion is at the end of existing buttons
	* I only see this happen with nIndex == -1, but it could have a special
	* meaning (like -nIndex (or ~nIndex) to get the real position of insertion).
	*/
	nIndex = infoPtr->nNumButtons;

    } else if (nIndex < 0)
       return FALSE;

    /* If the string passed is not an index, assume address of string
       and do our own AddString */
    if ((HIWORD(lpTbb->iString) != 0) && (lpTbb->iString != -1)) {
	LPWSTR ptr;
        INT len;

	TRACE("string %s passed instead of index, adding string\n",
	      debugstr_w((LPWSTR)lpTbb->iString));
	len = strlenW((LPWSTR)lpTbb->iString) + 2;
	ptr = Alloc(len*sizeof(WCHAR));
	strcpyW(ptr, (LPWSTR)lpTbb->iString);
	ptr[len - 1] = 0; /* ended by two '\0' */
	lpTbb->iString = TOOLBAR_AddStringW(hwnd, 0, (LPARAM)ptr);
	Free(ptr);
    }

    TRACE("inserting button index=%d\n", nIndex);
    if (nIndex > infoPtr->nNumButtons) {
	nIndex = infoPtr->nNumButtons;
	TRACE("adjust index=%d\n", nIndex);
    }

    oldButtons = infoPtr->buttons;
    infoPtr->nNumButtons++;
    infoPtr->buttons = Alloc (sizeof (TBUTTON_INFO) * infoPtr->nNumButtons);
    /* pre insert copy */
    if (nIndex > 0) {
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nIndex * sizeof(TBUTTON_INFO));
    }

    /* insert new button */
    infoPtr->buttons[nIndex].iBitmap   = lpTbb->iBitmap;
    infoPtr->buttons[nIndex].idCommand = lpTbb->idCommand;
    infoPtr->buttons[nIndex].fsState   = lpTbb->fsState;
    infoPtr->buttons[nIndex].fsStyle   = lpTbb->fsStyle;
    infoPtr->buttons[nIndex].dwData    = lpTbb->dwData;
    /* if passed string and not index, then add string */
    if(HIWORD(lpTbb->iString) && lpTbb->iString!=-1) {
        Str_SetPtrW ((LPWSTR *)&infoPtr->buttons[nIndex].iString, (LPWSTR)lpTbb->iString);
    }
    else
        infoPtr->buttons[nIndex].iString   = lpTbb->iString;

    if ((infoPtr->hwndToolTip) && !(lpTbb->fsStyle & TBSTYLE_SEP)) {
	TTTOOLINFOW ti;

	ZeroMemory (&ti, sizeof(TTTOOLINFOW));
	ti.cbSize   = sizeof (TTTOOLINFOW);
	ti.hwnd     = hwnd;
	ti.uId      = lpTbb->idCommand;
	ti.hinst    = 0;
	ti.lpszText = LPSTR_TEXTCALLBACKW;

	SendMessageW (infoPtr->hwndToolTip, TTM_ADDTOOLW,
			0, (LPARAM)&ti);
    }

    /* post insert copy */
    if (nIndex < infoPtr->nNumButtons - 1) {
	memcpy (&infoPtr->buttons[nIndex+1], &oldButtons[nIndex],
		(infoPtr->nNumButtons - nIndex - 1) * sizeof(TBUTTON_INFO));
    }

    Free (oldButtons);

    TOOLBAR_CalcToolbar (hwnd);

    InvalidateRect (hwnd, NULL, TRUE);

    return TRUE;
}


/* << TOOLBAR_InsertMarkHitTest >> */


static LRESULT
TOOLBAR_IsButtonChecked (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_CHECKED);
}


static LRESULT
TOOLBAR_IsButtonEnabled (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_ENABLED);
}


static LRESULT
TOOLBAR_IsButtonHidden (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return TRUE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_HIDDEN);
}


static LRESULT
TOOLBAR_IsButtonHighlighted (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_MARKED);
}


static LRESULT
TOOLBAR_IsButtonIndeterminate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_INDETERMINATE);
}


static LRESULT
TOOLBAR_IsButtonPressed (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_PRESSED);
}


/* << TOOLBAR_LoadImages >> */
/* << TOOLBAR_MapAccelerator >> */
/* << TOOLBAR_MarkButton >> */
/* << TOOLBAR_MoveButton >> */


static LRESULT
TOOLBAR_PressButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT)wParam, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_PRESSED;
    else
	btnPtr->fsState |= TBSTATE_PRESSED;

    InvalidateRect(hwnd, &btnPtr->rect, TOOLBAR_HasText(infoPtr, btnPtr));

    return TRUE;
}

/* FIXME: there might still be some confusion her between number of buttons
 * and number of bitmaps */
static LRESULT
TOOLBAR_ReplaceBitmap (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBREPLACEBITMAP lpReplace = (LPTBREPLACEBITMAP) lParam;
    HBITMAP hBitmap;
    int i = 0, nOldButtons = 0, pos = 0;
    int nOldBitmaps, nNewBitmaps;
    HIMAGELIST himlDef = 0;

    TRACE("hInstOld %p nIDOld %x hInstNew %p nIDNew %x nButtons %x\n",
          lpReplace->hInstOld, lpReplace->nIDOld, lpReplace->hInstNew, lpReplace->nIDNew,
          lpReplace->nButtons);

    if (lpReplace->hInstOld == HINST_COMMCTRL)
    {
        FIXME("changing standard bitmaps not implemented\n");
        return FALSE;
    }
    else if (lpReplace->hInstOld != 0)
    {
        FIXME("resources not in the current module not implemented\n");
        return FALSE;
    }
    else
    {
        hBitmap = (HBITMAP) lpReplace->nIDNew;
    }

    TRACE("To be replaced hInstOld %p nIDOld %x\n", lpReplace->hInstOld, lpReplace->nIDOld);
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
        WARN("No hinst/bitmap found! hInst %p nID %x\n", lpReplace->hInstOld, lpReplace->nIDOld);
        return FALSE;
    }
    
    himlDef = GETDEFIMAGELIST(infoPtr, 0); /* fixme: correct? */
    nOldBitmaps = ImageList_GetImageCount(himlDef);

    /* ImageList_Replace(GETDEFIMAGELIST(), pos, hBitmap, NULL); */

    for (i = pos + nOldBitmaps - 1; i >= pos; i--)
        ImageList_Remove(himlDef, i);

    {
       BITMAP  bmp;
       HBITMAP hOldBitmapBitmap, hOldBitmapLoad, hbmLoad;
       HDC     hdcImage, hdcBitmap;

       /* copy the bitmap before adding it so that the user's bitmap
        * doesn't get modified.
        */
       GetObjectA (hBitmap, sizeof(BITMAP), (LPVOID)&bmp);

       hdcImage  = CreateCompatibleDC(0);
       hdcBitmap = CreateCompatibleDC(0);

       /* create new bitmap */
       hbmLoad = CreateBitmap (bmp.bmWidth, bmp.bmHeight, bmp.bmPlanes, bmp.bmBitsPixel, NULL);
       hOldBitmapBitmap = SelectObject(hdcBitmap, hBitmap);
       hOldBitmapLoad = SelectObject(hdcImage, hbmLoad);

       /* Copy the user's image */
       BitBlt (hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight,
               hdcBitmap, 0, 0, SRCCOPY);

       SelectObject (hdcImage, hOldBitmapLoad);
       SelectObject (hdcBitmap, hOldBitmapBitmap);
       DeleteDC (hdcImage);
       DeleteDC (hdcBitmap);

       ImageList_AddMasked (himlDef, hbmLoad, comctl32_color.clrBtnFace);
       nNewBitmaps = ImageList_GetImageCount(himlDef);
       DeleteObject (hbmLoad);
    }

    infoPtr->nNumBitmaps = infoPtr->nNumBitmaps - nOldBitmaps + nNewBitmaps;

    TRACE(" pos %d  %d old bitmaps replaced by %d new ones.\n",
            pos, nOldBitmaps, nNewBitmaps);

    InvalidateRect(hwnd, NULL, FALSE);

    return TRUE;
}

static LRESULT
TOOLBAR_SaveRestoreA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#if 0
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBSAVEPARAMSA lpSave = (LPTBSAVEPARAMSA)lParam;

    if (lpSave == NULL) return 0;

    if ((BOOL)wParam) {
	/* save toolbar information */
	FIXME("save to \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }
    else {
	/* restore toolbar information */

	FIXME("restore from \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }
#endif

    return 0;
}


static LRESULT
TOOLBAR_SaveRestoreW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#if 0
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBSAVEPARAMSW lpSave = (LPTBSAVEPARAMSW)lParam;

    if (lpSave == NULL)
	return 0;

    if ((BOOL)wParam) {
	/* save toolbar information */
	FIXME("save to \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }
    else {
	/* restore toolbar information */

	FIXME("restore from \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }
#endif

    return 0;
}


static LRESULT
TOOLBAR_SetAnchorHighlight (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    BOOL bOldAnchor = infoPtr->bAnchor;

    infoPtr->bAnchor = (BOOL)wParam;

    return (LRESULT)bOldAnchor;
}


static LRESULT
TOOLBAR_SetBitmapSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    HIMAGELIST himlDef = GETDEFIMAGELIST(infoPtr, 0);

    if ((LOWORD(lParam) <= 0) || (HIWORD(lParam)<=0))
	return FALSE;

    if (infoPtr->nNumButtons > 0)
        WARN("%d buttons, undoc increase to bitmap size : %d-%d -> %d-%d\n",
             infoPtr->nNumButtons,
             infoPtr->nBitmapWidth, infoPtr->nBitmapHeight,
             LOWORD(lParam), HIWORD(lParam));

    infoPtr->nBitmapWidth = (INT)LOWORD(lParam);
    infoPtr->nBitmapHeight = (INT)HIWORD(lParam);


    /* uses image list internals directly */
    if (himlDef) {
        himlDef->cx = infoPtr->nBitmapWidth;
        himlDef->cy = infoPtr->nBitmapHeight;
    }

    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTONINFOA lptbbi = (LPTBBUTTONINFOA)lParam;
    TBUTTON_INFO *btnPtr;
    INT nIndex;

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
/*    if (lptbbi->dwMask & TBIF_SIZE) */
/*	btnPtr->cx = lptbbi->cx; */
    if (lptbbi->dwMask & TBIF_STATE)
	btnPtr->fsState = lptbbi->fsState;
    if (lptbbi->dwMask & TBIF_STYLE)
	btnPtr->fsStyle = lptbbi->fsStyle;

    if ((lptbbi->dwMask & TBIF_TEXT) && ((INT)lptbbi->pszText != -1)) {
        if ((HIWORD(btnPtr->iString) == 0) || (btnPtr->iString == -1))
	    /* iString is index, zero it to make Str_SetPtr succeed */
	    btnPtr->iString=0;

         Str_SetPtrAtoW ((LPWSTR *)&btnPtr->iString, lptbbi->pszText);
    }
    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPTBBUTTONINFOW lptbbi = (LPTBBUTTONINFOW)lParam;
    TBUTTON_INFO *btnPtr;
    INT nIndex;

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
/*    if (lptbbi->dwMask & TBIF_SIZE) */
/*	btnPtr->cx = lptbbi->cx; */
    if (lptbbi->dwMask & TBIF_STATE)
	btnPtr->fsState = lptbbi->fsState;
    if (lptbbi->dwMask & TBIF_STYLE)
	btnPtr->fsStyle = lptbbi->fsStyle;

    if ((lptbbi->dwMask & TBIF_TEXT) && ((INT)lptbbi->pszText != -1)) {
        if ((HIWORD(btnPtr->iString) == 0) || (btnPtr->iString == -1))
	    /* iString is index, zero it to make Str_SetPtr succeed */
	    btnPtr->iString=0;
        Str_SetPtrW ((LPWSTR *)&btnPtr->iString, lptbbi->pszText);
    }
    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT cx = LOWORD(lParam), cy = HIWORD(lParam);

    if ((cx < 0) || (cy < 0))
    {
        ERR("invalid parameter 0x%08lx\n", (DWORD)lParam);
	return FALSE;
    }

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
    infoPtr->nButtonWidth = (cx) ? cx : 24;
    infoPtr->nButtonHeight = (cy) ? cy : 22;
    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonWidth (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (infoPtr == NULL) {
	TRACE("Toolbar not initialized yet?????\n");
	return FALSE;
    }

    /* if setting to current values, ignore */
    if ((infoPtr->cxMin == (INT)LOWORD(lParam)) &&
	(infoPtr->cxMax == (INT)HIWORD(lParam))) {
	TRACE("matches current width, min=%d, max=%d, no recalc\n",
	      infoPtr->cxMin, infoPtr->cxMax);
	return TRUE;
    }

    /* save new values */
    infoPtr->cxMin = (INT)LOWORD(lParam);
    infoPtr->cxMax = (INT)HIWORD(lParam);

    /* if both values are 0 then we are done */
    if (lParam == 0) {
	TRACE("setting both min and max to 0, norecalc\n");
	return TRUE;
    }

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

    dwTemp = infoPtr->dwDTFlags;
    infoPtr->dwDTFlags =
	(infoPtr->dwDTFlags & (DWORD)wParam) | (DWORD)lParam;

    return (LRESULT)dwTemp;
}

static LRESULT
TOOLBAR_SetExtendedStyle (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwTemp;

    dwTemp = infoPtr->dwExStyle;
    infoPtr->dwExStyle |= (DWORD)lParam;

    TRACE("new style 0x%08lx\n", infoPtr->dwExStyle);

    if (infoPtr->dwExStyle & ~TBSTYLE_EX_ALL)
	FIXME("Unknown Toolbar Extended Style 0x%08lx. Please report.\n",
	      (infoPtr->dwExStyle & ~TBSTYLE_EX_ALL));

    TOOLBAR_CalcToolbar (hwnd);

    TOOLBAR_AutoSize(hwnd);

    InvalidateRect(hwnd, NULL, FALSE);

    return (LRESULT)dwTemp;
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


static LRESULT
TOOLBAR_SetHotItem (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    INT nOldHotItem = infoPtr->nHotItem;
    TBUTTON_INFO *btnPtr;

    if ((INT) wParam < 0 || (INT)wParam > infoPtr->nNumButtons)
        wParam = -2;

    if (GetWindowLongA (hwnd, GWL_STYLE) & TBSTYLE_FLAT)
    {

    	infoPtr->nHotItem = (INT)wParam;
        if ((INT)wParam >=0)
        {
            btnPtr = &infoPtr->buttons[(INT)wParam];
            btnPtr->bHot = TRUE;
	        InvalidateRect (hwnd, &btnPtr->rect,
                    TOOLBAR_HasText(infoPtr, btnPtr));
        }
        if (nOldHotItem>=0)
        {
            btnPtr = &infoPtr->buttons[nOldHotItem];
            btnPtr->bHot = FALSE;
	        InvalidateRect (hwnd, &btnPtr->rect,
                    TOOLBAR_HasText(infoPtr, btnPtr));
        }
    }

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
    INT i, id = 0;

    if (infoPtr->iVersion >= 5)
        id = wParam;

    himlTemp = TOOLBAR_InsertImageList(&infoPtr->himlDef, 
        &infoPtr->cimlDef, himl, id);

    infoPtr->nNumBitmaps = 0;
    for (i = 0; i < infoPtr->cimlDef; i++)
        infoPtr->nNumBitmaps += ImageList_GetImageCount(infoPtr->himlDef[i]->himl);

    ImageList_GetIconSize(himl, &infoPtr->nBitmapWidth,
			  &infoPtr->nBitmapHeight);
    TRACE("hwnd %p, new himl=%08x, count=%d, bitmap w=%d, h=%d\n",
	  hwnd, (INT)infoPtr->himlDef, infoPtr->nNumBitmaps,
	  infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);

    /* FIXME: redraw ? */
    InvalidateRect(hwnd, NULL, TRUE);

    return (LRESULT)himlTemp;
}


static LRESULT
TOOLBAR_SetIndent (HWND hwnd, WPARAM wParam, LPARAM lParam)
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


/* << TOOLBAR_SetInsertMark >> */


static LRESULT
TOOLBAR_SetInsertMarkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    infoPtr->clrInsertMark = (COLORREF)lParam;

    /* FIXME : redraw ??*/

    return 0;
}


static LRESULT
TOOLBAR_SetMaxTextRows (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (infoPtr == NULL)
	return FALSE;

    infoPtr->nMaxTextRows = (INT)wParam;

    return TRUE;
}


static LRESULT
TOOLBAR_SetPadding (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD  oldPad;

    oldPad = MAKELONG(infoPtr->szPadding.cx, infoPtr->szPadding.cy);
    infoPtr->szPadding.cx = LOWORD((DWORD)lParam);
    infoPtr->szPadding.cy = HIWORD((DWORD)lParam);
    FIXME("stub - nothing done with values, cx=%ld, cy=%ld\n",
	  infoPtr->szPadding.cx, infoPtr->szPadding.cy);
    return (LRESULT) oldPad;
}


static LRESULT
TOOLBAR_SetParent (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    HWND hwndOldNotify;

    TRACE("\n");

    if (infoPtr == NULL)
	return 0;
    hwndOldNotify = infoPtr->hwndNotify;
    infoPtr->hwndNotify = (HWND)wParam;

    return (LRESULT)hwndOldNotify;
}


static LRESULT
TOOLBAR_SetRows (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPRECT lprc = (LPRECT)lParam;

    TRACE("\n");

    if (LOWORD(wParam) > 1) {
	FIXME("multiple rows not supported!\n");
    }

    if(infoPtr->nRows != LOWORD(wParam))
    {
        infoPtr->nRows = LOWORD(wParam);

        /* recalculate toolbar */
        TOOLBAR_CalcToolbar (hwnd);

        /* repaint toolbar */
        InvalidateRect(hwnd, NULL, FALSE);
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
	InvalidateRect(hwnd, 0, TOOLBAR_HasText(infoPtr, btnPtr));
	return TRUE;
    }

    /* process state changing if current state doesn't match new state */
    if(btnPtr->fsState != LOWORD(lParam))
    {
        btnPtr->fsState = LOWORD(lParam);
        InvalidateRect(hwnd, &btnPtr->rect, TOOLBAR_HasText(infoPtr,
            btnPtr));
    }

    return TRUE;
}


static LRESULT
TOOLBAR_SetStyle (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    SetWindowLongW(hwnd, GWL_STYLE, lParam);

    return TRUE;
}


inline static LRESULT
TOOLBAR_SetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (infoPtr == NULL)
	return 0;
    infoPtr->hwndToolTip = (HWND)wParam;
    return 0;
}


static LRESULT
TOOLBAR_SetUnicodeFormat (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    BOOL bTemp;

    TRACE("%s hwnd=%p stub!\n",
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
TOOLBAR_SetColorScheme (HWND hwnd, LPCOLORSCHEME lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("new colors Hl=%lx Shd=%lx, old colors Hl=%lx Shd=%lx\n",
	  lParam->clrBtnHighlight, lParam->clrBtnShadow,
	  infoPtr->clrBtnHighlight, infoPtr->clrBtnShadow);

    infoPtr->clrBtnHighlight = lParam->clrBtnHighlight;
    infoPtr->clrBtnShadow = lParam->clrBtnShadow;
    InvalidateRect(hwnd, 0, 0);
    return 0;
}


static LRESULT
TOOLBAR_SetVersion (HWND hwnd, INT iVersion)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    INT iOldVersion = infoPtr->iVersion;

    infoPtr->iVersion = iVersion;

    if (infoPtr->iVersion >= 5)
        TOOLBAR_SetUnicodeFormat(hwnd, (WPARAM)TRUE, (LPARAM)0);

    return iOldVersion;
}


/*********************************************************************/
/*                                                                   */
/* This is undocumented and appears to be a "Super" TB_SETHOTITEM    */
/* without the restriction of TBSTYLE_FLAT. This implementation is   */
/* based on relay traces of the native control and IE 5.5            */
/*                                                                   */
/*********************************************************************/
static LRESULT
TOOLBAR_Unkwn45E (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    INT nOldHotItem = infoPtr->nHotItem;
    TBUTTON_INFO *btnPtr;
    INT no_hi = 0;
    NMTBHOTITEM nmhotitem;

    if ((INT) wParam < 0 || (INT)wParam > infoPtr->nNumButtons)
        wParam = -2;

    infoPtr->nHotItem = (INT)wParam;
    if (nOldHotItem != infoPtr->nHotItem) {
	nmhotitem.dwFlags = (DWORD)lParam;
	if ( !(nmhotitem.dwFlags & HICF_ENTERING) )
	    nmhotitem.idOld = (nOldHotItem >= 0) ?
		infoPtr->buttons[nOldHotItem].idCommand : 0;
	if ( !(nmhotitem.dwFlags & HICF_LEAVING) )
	    nmhotitem.idNew = (infoPtr->nHotItem >= 0) ?
		infoPtr->buttons[infoPtr->nHotItem].idCommand : 0;
	no_hi = TOOLBAR_SendNotify((NMHDR*)&nmhotitem, infoPtr, TBN_HOTITEMCHANGE);
    }
    if ((INT)wParam >=0) {
	btnPtr = &infoPtr->buttons[(INT)wParam];
	btnPtr->bHot = (no_hi) ? FALSE : TRUE;
	InvalidateRect (hwnd, &btnPtr->rect,
			TOOLBAR_HasText(infoPtr, btnPtr));
    }
    if (nOldHotItem>=0) {
	btnPtr = &infoPtr->buttons[nOldHotItem];
	btnPtr->bHot = FALSE;
	InvalidateRect (hwnd, &btnPtr->rect,
			TOOLBAR_HasText(infoPtr, btnPtr));
    }
    GetFocus();
    TRACE("old item=%d, new item=%d, flags=%08lx, notify=%d\n",
	  nOldHotItem, infoPtr->nHotItem, (DWORD)lParam, no_hi);

    if (nOldHotItem < 0)
	return -1;

    return (LRESULT)nOldHotItem;
}


static LRESULT
TOOLBAR_Unkwn463 (HWND hwnd, WPARAM wParam, LPARAM lParam)
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
    TRACE("[0463] wParam %d, lParam 0x%08lx -> 0x%08lx 0x%08lx\n",
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

	    InvalidateRect(hwnd, 0, 1);
	    GetWindowRect(hwnd, &rc);
	    MapWindowPoints(0, hwndParent, (LPPOINT)&rc, 2);
	    TRACE("mapped to (%ld,%ld)-(%ld,%ld)\n",
		rc.left, rc.top, rc.right, rc.bottom);
	    lpsize->cx = max(rc.right-rc.left,
			     infoPtr->rcBound.right - infoPtr->rcBound.left);
	}
	else {
	    lpsize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
	}
	break;
    case 1:
	lpsize->cy = infoPtr->rcBound.bottom - infoPtr->rcBound.top;
	/* lpsize->cy = infoPtr->nHeight; */
	break;
    default:
	ERR("Unknown wParam %d for Toolbar message [0463]. Please report\n",
	    wParam);
	return 0;
    }
    TRACE("[0463] set to -> 0x%08lx 0x%08lx\n",
	  lpsize->cx, lpsize->cy);
    return 1;
}


static LRESULT
TOOLBAR_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    LOGFONTA logFont;

    /* initialize info structure */
    infoPtr->nButtonHeight = 22;
    infoPtr->nButtonWidth = 24;
    infoPtr->nBitmapHeight = 15;
    infoPtr->nBitmapWidth = 16;

    infoPtr->nHeight = infoPtr->nButtonHeight + TOP_BORDER + BOTTOM_BORDER;
    infoPtr->nMaxTextRows = 1;
    infoPtr->cxMin = -1;
    infoPtr->cxMax = -1;
    infoPtr->nNumBitmaps = 0;
    infoPtr->nNumStrings = 0;

    infoPtr->bCaptured = FALSE;
    infoPtr->bUnicode = IsWindowUnicode (hwnd);
    infoPtr->nButtonDown = -1;
    infoPtr->nOldHit = -1;
    infoPtr->nHotItem = -2; /* It has to be initially different from nOldHit */
    infoPtr->hwndNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;
    infoPtr->bTransparent = (dwStyle & TBSTYLE_TRANSPARENT);
    infoPtr->bBtnTranspnt = (dwStyle & (TBSTYLE_FLAT | TBSTYLE_LIST));
    infoPtr->dwDTFlags = (dwStyle & TBSTYLE_LIST) ? DT_LEFT | DT_VCENTER | DT_SINGLELINE : DT_CENTER;
    infoPtr->bAnchor = FALSE; /* no anchor highlighting */
    infoPtr->iVersion = 0;
    infoPtr->hwndSelf = hwnd;
    infoPtr->bDoRedraw = TRUE;
    infoPtr->clrBtnHighlight = CLR_DEFAULT;
    infoPtr->clrBtnShadow = CLR_DEFAULT;
    infoPtr->szPadding.cx = 7;
    infoPtr->szPadding.cy = 6;
    TOOLBAR_NotifyFormat(infoPtr, (WPARAM)hwnd, (LPARAM)NF_REQUERY);

    SystemParametersInfoA (SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
    infoPtr->hFont = infoPtr->hDefaultFont = CreateFontIndirectA (&logFont);

    if (dwStyle & TBSTYLE_TOOLTIPS) {
	/* Create tooltip control */
	infoPtr->hwndToolTip =
	    CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
			       CW_USEDEFAULT, CW_USEDEFAULT,
			       CW_USEDEFAULT, CW_USEDEFAULT,
			       hwnd, 0, 0, 0);

	/* Send NM_TOOLTIPSCREATED notification */
	if (infoPtr->hwndToolTip) {
	    NMTOOLTIPSCREATED nmttc;

	    nmttc.hwndToolTips = infoPtr->hwndToolTip;

	    TOOLBAR_SendNotify ((NMHDR *) &nmttc, infoPtr,
			    NM_TOOLTIPSCREATED);
	}
    }

    TOOLBAR_CheckStyle (hwnd, dwStyle);

    TOOLBAR_CalcToolbar(hwnd);

    return 0;
}


static LRESULT
TOOLBAR_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    /* delete tooltip control */
    if (infoPtr->hwndToolTip)
	DestroyWindow (infoPtr->hwndToolTip);

    /* delete button data */
    if (infoPtr->buttons)
	Free (infoPtr->buttons);

    /* delete strings */
    if (infoPtr->strings) {
	INT i;
	for (i = 0; i < infoPtr->nNumStrings; i++)
	    if (infoPtr->strings[i])
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
    if (infoPtr->hFont)
	DeleteObject (infoPtr->hDefaultFont);

    /* free toolbar info data */
    Free (infoPtr);
    SetWindowLongA (hwnd, 0, 0);

    return 0;
}


static LRESULT
TOOLBAR_EraseBackground (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    NMTBCUSTOMDRAW tbcd;
    INT ret = FALSE;
    DWORD ntfret;

    if (dwStyle & TBSTYLE_CUSTOMERASE) {
	ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
	tbcd.nmcd.dwDrawStage = CDDS_PREERASE;
	tbcd.nmcd.hdc = (HDC)wParam;
	ntfret = TOOLBAR_SendNotify ((NMHDR *)&tbcd, infoPtr, NM_CUSTOMDRAW);
	infoPtr->dwBaseCustDraw = ntfret & 0xffff;

	/* FIXME: in general the return flags *can* be or'ed together */
	switch (infoPtr->dwBaseCustDraw)
	    {
	    case CDRF_DODEFAULT:
		break;
	    case CDRF_SKIPDEFAULT:
		return TRUE;
	    default:
		FIXME("[%p] response %ld not handled to NM_CUSTOMDRAW (CDDS_PREERASE)\n",
		      hwnd, ntfret);
	    }
    }

    /* If the toolbar is "transparent" then pass the WM_ERASEBKGND up
     * to my parent for processing.
     */
    if (infoPtr->bTransparent) {
	POINT pt, ptorig;
	HDC hdc = (HDC)wParam;
	HWND parent;

	pt.x = 0;
	pt.y = 0;
	parent = GetParent(hwnd);
	MapWindowPoints(hwnd, parent, &pt, 1);
	OffsetWindowOrgEx (hdc, pt.x, pt.y, &ptorig);
	ret = SendMessageA (parent, WM_ERASEBKGND, wParam, lParam);
	SetWindowOrgEx (hdc, ptorig.x, ptorig.y, 0);
    }
    if (!ret)
	ret = DefWindowProcA (hwnd, WM_ERASEBKGND, wParam, lParam);

    if ((dwStyle & TBSTYLE_CUSTOMERASE) &&
	(infoPtr->dwBaseCustDraw & CDRF_NOTIFYPOSTERASE)) {
	ZeroMemory (&tbcd, sizeof(NMTBCUSTOMDRAW));
	tbcd.nmcd.dwDrawStage = CDDS_POSTERASE;
	tbcd.nmcd.hdc = (HDC)wParam;
	ntfret = TOOLBAR_SendNotify ((NMHDR *)&tbcd, infoPtr, NM_CUSTOMDRAW);
	infoPtr->dwBaseCustDraw = ntfret & 0xffff;
	switch (infoPtr->dwBaseCustDraw)
	    {
	    case CDRF_DODEFAULT:
		break;
	    case CDRF_SKIPDEFAULT:
		return TRUE;
	    default:
		FIXME("[%p] response %ld not handled to NM_CUSTOMDRAW (CDDS_PREERASE)\n",
		      hwnd, ntfret);
	    }
    }
    return ret;
}


static LRESULT
TOOLBAR_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


static LRESULT
TOOLBAR_LButtonDblClk (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *btnPtr;
    POINT pt;
    INT   nHit;

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    if (nHit >= 0) {
	btnPtr = &infoPtr->buttons[nHit];
	if (!(btnPtr->fsState & TBSTATE_ENABLED))
	    return 0;
	SetCapture (hwnd);
	infoPtr->bCaptured = TRUE;
	infoPtr->nButtonDown = nHit;

	btnPtr->fsState |= TBSTATE_PRESSED;

        InvalidateRect(hwnd, &btnPtr->rect, TOOLBAR_HasText(infoPtr,
            btnPtr));
    }
    else if (GetWindowLongA (hwnd, GWL_STYLE) & CCS_ADJUSTABLE)
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

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, hwnd,
			    WM_LBUTTONDOWN, wParam, lParam);

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    if (nHit >= 0) {
	RECT arrowRect;
	btnPtr = &infoPtr->buttons[nHit];
	infoPtr->nOldHit = nHit;

	CopyRect(&arrowRect, &btnPtr->rect);
	arrowRect.left = max(btnPtr->rect.left, btnPtr->rect.right - DDARROW_WIDTH);

	/* for EX_DRAWDDARROWS style,  click must be in the drop-down arrow rect */
	if ((btnPtr->fsState & TBSTATE_ENABLED) && (btnPtr->fsStyle & TBSTYLE_DROPDOWN) &&
	     ((TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle) && PtInRect(&arrowRect, pt)) ||
	      (!TOOLBAR_HasDropDownArrows(infoPtr->dwExStyle))))
	{
	    LRESULT res;
	    /*
	     * this time we must force a Redraw, so the btn is
	     * painted down before CaptureChanged repaints it up
	     */
	    RedrawWindow(hwnd,&btnPtr->rect,0,
			RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);

	    nmtb.iItem = btnPtr->idCommand;
	    memset(&nmtb.tbButton, 0, sizeof(TBBUTTON));
	    nmtb.cchText = 0;
	    nmtb.pszText = 0;
	    memset(&nmtb.rcButton, 0, sizeof(RECT));
	    res = TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
				  TBN_DROPDOWN);
	    if (res != TBDDRET_TREATPRESSED)
		/* ??? guess  (GA)  */
		return 0;
	    /* otherwise drop through and process as pushed */
       	}
	/* SetCapture (hwnd); */
	infoPtr->bCaptured = TRUE;
	infoPtr->nButtonDown = nHit;

	btnPtr->fsState |= TBSTATE_PRESSED;
	btnPtr->bHot = FALSE;

        if (btnPtr->fsState & TBSTATE_ENABLED)
	    InvalidateRect(hwnd, &btnPtr->rect, TOOLBAR_HasText(infoPtr, btnPtr));
	UpdateWindow(hwnd);
	SetCapture (hwnd);

	/* native issues the TBN_BEGINDRAG here */
	nmtb.iItem = btnPtr->idCommand;
	nmtb.tbButton.iBitmap = btnPtr->iBitmap;
	nmtb.tbButton.idCommand = btnPtr->idCommand;
	nmtb.tbButton.fsState = btnPtr->fsState;
	nmtb.tbButton.fsStyle = btnPtr->fsStyle;
	nmtb.tbButton.dwData = btnPtr->dwData;
	nmtb.tbButton.iString = btnPtr->iString;
	nmtb.cchText = 0;  /* !!! not correct */
	nmtb.pszText = 0;  /* !!! not correct */
	TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
			TBN_BEGINDRAG);
    }

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
    BOOL  bSendMessage = TRUE;
    NMHDR hdr;
    NMMOUSE nmmouse;
    NMTOOLBARA nmtb;

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, hwnd,
			    WM_LBUTTONUP, wParam, lParam);

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    /* restore hot effect to hot button disabled by TOOLBAR_LButtonDown() */
    /* if the cursor is still inside of the toolbar */
    if((infoPtr->nHotItem >= 0) && (nHit != -1))
        infoPtr->buttons[infoPtr->nHotItem].bHot = TRUE;

    if (0 <= infoPtr->nButtonDown) {
	btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
	btnPtr->fsState &= ~TBSTATE_PRESSED;

	if (btnPtr->fsStyle & TBSTYLE_CHECK) {
		if (btnPtr->fsStyle & TBSTYLE_GROUP) {
		    nOldIndex = TOOLBAR_GetCheckedGroupButtonIndex (infoPtr,
			nHit);
		    if (nOldIndex == nHit)
			bSendMessage = FALSE;
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
        {
            InvalidateRect(hwnd, &infoPtr->buttons[nOldIndex].rect,
                TOOLBAR_HasText(infoPtr, &infoPtr->buttons[nOldIndex]));
        }

	/*
	 * now we can ReleaseCapture, which triggers CAPTURECHANGED msg,
	 * that resets bCaptured and btn TBSTATE_PRESSED flags,
	 * and obliterates nButtonDown and nOldHit (see TOOLBAR_CaptureChanged)
	 */
	if ((infoPtr->bCaptured) && (infoPtr->nButtonDown >= 0))
	    ReleaseCapture ();
	infoPtr->nButtonDown = -1;

	/* Issue NM_RELEASEDCAPTURE to parent to let him know it is released */
	TOOLBAR_SendNotify ((NMHDR *) &hdr, infoPtr,
			NM_RELEASEDCAPTURE);

	/* native issues TBN_ENDDRAG here, if _LBUTTONDOWN issued the
	 * TBN_BEGINDRAG
	 */
	nmtb.iItem = btnPtr->idCommand;
	nmtb.tbButton.iBitmap = btnPtr->iBitmap;
	nmtb.tbButton.idCommand = btnPtr->idCommand;
	nmtb.tbButton.fsState = btnPtr->fsState;
	nmtb.tbButton.fsStyle = btnPtr->fsStyle;
	nmtb.tbButton.dwData = btnPtr->dwData;
	nmtb.tbButton.iString = btnPtr->iString;
	nmtb.cchText = 0;  /* !!! not correct */
	nmtb.pszText = 0;  /* !!! not correct */
	TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
			TBN_ENDDRAG);

	if (btnPtr->fsState & TBSTATE_ENABLED)
	{
	    SendMessageA (infoPtr->hwndNotify, WM_COMMAND,
	      MAKEWPARAM(infoPtr->buttons[nHit].idCommand, 0), (LPARAM)hwnd);

	    /* !!! Undocumented - toolbar at 4.71 level and above sends
	    * either NMRCLICK or NM_CLICK with the NMMOUSE structure.
	    * Only NM_RCLICK is documented.
	    */
	    nmmouse.dwItemSpec = btnPtr->idCommand;
	    nmmouse.dwItemData = btnPtr->dwData;
	    TOOLBAR_SendNotify ((NMHDR *) &nmmouse, infoPtr, NM_CLICK);
	}
    }
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
            InvalidateRect(hwnd, &btnPtr->rect, TOOLBAR_HasText(infoPtr,
              btnPtr));
    }
    return 0;
}

static LRESULT
TOOLBAR_MouseLeave (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    TBUTTON_INFO *hotBtnPtr, *btnPtr;
    RECT rc1;

    if (infoPtr->nOldHit < 0)
      return TRUE;

    hotBtnPtr = &infoPtr->buttons[infoPtr->nOldHit];

    /* Redraw the button if the last button we were over is the hot button and it
       is enabled */
    if((infoPtr->nOldHit == infoPtr->nHotItem) && (hotBtnPtr->fsState & TBSTATE_ENABLED))
    {
	hotBtnPtr->bHot = FALSE;
	rc1 = hotBtnPtr->rect;
	InflateRect (&rc1, 1, 1);
        InvalidateRect (hwnd, &rc1, TOOLBAR_HasText(infoPtr,
            hotBtnPtr));
    }

    /* If the last button we were over is depressed then make it not */
    /* depressed and redraw it */
    if(infoPtr->nOldHit == infoPtr->nButtonDown)
    {
      btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];

      btnPtr->fsState &= ~TBSTATE_PRESSED;

      rc1 = hotBtnPtr->rect;
      InflateRect (&rc1, 1, 1);
      InvalidateRect (hwnd, &rc1, TRUE);
    }

    infoPtr->nOldHit = -1; /* reset the old hit index as we've left the toolbar */
    infoPtr->nHotItem = -2; /* It has to be initially different from nOldHit */

    return TRUE;
}

static LRESULT
TOOLBAR_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TBUTTON_INFO *btnPtr = NULL, *oldBtnPtr = NULL;
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    POINT pt;
    INT   nHit;
    TRACKMOUSEEVENT trackinfo;
    NMTBHOTITEM nmhotitem;

    /* fill in the TRACKMOUSEEVENT struct */
    trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
    trackinfo.dwFlags = TME_QUERY;
    trackinfo.hwndTrack = hwnd;
    trackinfo.dwHoverTime = HOVER_DEFAULT;

    /* call _TrackMouseEvent to see if we are currently tracking for this hwnd */
    _TrackMouseEvent(&trackinfo);

    /* Make sure tracking is enabled so we receive a WM_MOUSELEAVE message */
    if(!(trackinfo.dwFlags & TME_LEAVE)) {
        trackinfo.dwFlags = TME_LEAVE; /* notify upon leaving */

        /* call TRACKMOUSEEVENT so we receive a WM_MOUSELEAVE message */
        /* and can properly deactivate the hot toolbar button */
        _TrackMouseEvent(&trackinfo);
   }

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, hwnd,
			    WM_MOUSEMOVE, wParam, lParam);

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);

    nHit = TOOLBAR_InternalHitTest (hwnd, &pt);

    if (infoPtr->nOldHit != nHit)
    {
	/* Remove the effect of an old hot button if the button was
	   drawn with the hot button effect */
	if(infoPtr->nOldHit >= 0 && infoPtr->nOldHit == infoPtr->nHotItem)
	{
	    oldBtnPtr = &infoPtr->buttons[infoPtr->nOldHit];
	    oldBtnPtr->bHot = FALSE;
      	}

	/* It's not a separator or in nowhere. It's a hot button. */
	if (nHit >= 0)
	{
	    btnPtr = &infoPtr->buttons[nHit];

	    infoPtr->nHotItem = nHit;

            btnPtr->bHot = TRUE;
	}

	nmhotitem.dwFlags = HICF_MOUSE;
	if (oldBtnPtr)
	    nmhotitem.idOld = oldBtnPtr->idCommand;
	else
	    nmhotitem.dwFlags |= HICF_ENTERING;
	if (btnPtr)
	    nmhotitem.idNew = btnPtr->idCommand;
	else
	    nmhotitem.dwFlags |= HICF_LEAVING;
	TOOLBAR_SendNotify((NMHDR*)&nmhotitem, infoPtr, TBN_HOTITEMCHANGE);

	/* now invalidate the old and new buttons so they will be painted */
	if (oldBtnPtr)
	    InvalidateRect (hwnd, &oldBtnPtr->rect,
			    TOOLBAR_HasText(infoPtr, oldBtnPtr));
	if (btnPtr)
	    InvalidateRect(hwnd, &btnPtr->rect,
			   TOOLBAR_HasText(infoPtr, btnPtr));

	if (infoPtr->bCaptured) {
	    btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
	    if (infoPtr->nOldHit == infoPtr->nButtonDown) {
		btnPtr->fsState &= ~TBSTATE_PRESSED;
                InvalidateRect(hwnd, &btnPtr->rect, TRUE);
	    }
	    else if (nHit == infoPtr->nButtonDown) {
		btnPtr->fsState |= TBSTATE_PRESSED;
                InvalidateRect(hwnd, &btnPtr->rect, TRUE);
	    }
	}
	infoPtr->nOldHit = nHit;
    }
    return 0;
}


inline static LRESULT
TOOLBAR_NCActivate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    if (wndPtr->dwStyle & CCS_NODIVIDER) */
	return DefWindowProcA (hwnd, WM_NCACTIVATE, wParam, lParam);
/*    else */
/*	return TOOLBAR_NCPaint (wndPtr, wParam, lParam); */
}


inline static LRESULT
TOOLBAR_NCCalcSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (!(GetWindowLongA (hwnd, GWL_STYLE) & CCS_NODIVIDER))
	((LPRECT)lParam)->top += GetSystemMetrics(SM_CYEDGE);

    return DefWindowProcA (hwnd, WM_NCCALCSIZE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCCreate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr;
    LPCREATESTRUCTA cs = (LPCREATESTRUCTA)lParam;
    DWORD styleadd = 0;

    /* allocate memory for info structure */
    infoPtr = (TOOLBAR_INFO *)Alloc (sizeof(TOOLBAR_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* paranoid!! */
    infoPtr->dwStructSize = sizeof(TBBUTTON);
    infoPtr->nRows = 1;

    /* fix instance handle, if the toolbar was created by CreateToolbarEx() */
    if (!GetWindowLongA (hwnd, GWL_HINSTANCE)) {
        HINSTANCE hInst = (HINSTANCE)GetWindowLongA (GetParent (hwnd), GWL_HINSTANCE);
	SetWindowLongA (hwnd, GWL_HINSTANCE, (DWORD)hInst);
    }

    /* native control does:
     *    Get a lot of colors and brushes
     *    WM_NOTIFYFORMAT
     *    SystemParametersInfoA(0x1f, 0x3c, adr1, 0)
     *    CreateFontIndirectA(adr1)
     *    CreateBitmap(0x27, 0x24, 1, 1, 0)
     *    hdc = GetDC(toolbar)
     *    GetSystemMetrics(0x48)
     *    fnt2=CreateFontA(0xe, 0, 0, 0, 0x190, 0, 0, 0, 0, 2,
     *                     0, 0, 0, 0, "MARLETT")
     *    oldfnt = SelectObject(hdc, fnt2)
     *    GetCharWidthA(hdc, 0x36, 0x36, adr2)
     *    GetTextMetricsA(hdc, adr3)
     *    SelectObject(hdc, oldfnt)
     *    DeleteObject(fnt2)
     *    ReleaseDC(hdc)
     *    InvalidateRect(toolbar, 0, 1)
     *    SetWindowLongA(toolbar, 0, addr)
     *    SetWindowLongA(toolbar, -16, xxx)  **sometimes**
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
     * Some how, the only cases of this seem to be MFC programs.
     *
     * Note also that the addition of _TRANSPARENT occurs *only* here. It
     * does not occur in the WM_STYLECHANGING routine.
     *    (Guy Albertelli   9/2001)
     *
     */
    if ((cs->style & TBSTYLE_FLAT) && !(cs->style & TBSTYLE_TRANSPARENT))
	styleadd |= TBSTYLE_TRANSPARENT;
    if (!(cs->style & (CCS_TOP | CCS_NOMOVEY))) {
	styleadd |= CCS_TOP;   /* default to top */
	SetWindowLongA (hwnd, GWL_STYLE, cs->style | styleadd);
    }

    return DefWindowProcA (hwnd, WM_NCCREATE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCPaint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT rcWindow;
    HDC hdc;

    if (dwStyle & WS_MINIMIZE)
	return 0; /* Nothing to do */

    DefWindowProcA (hwnd, WM_NCPAINT, wParam, lParam);

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


inline static LRESULT
TOOLBAR_Notify (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    LPNMHDR lpnmh = (LPNMHDR)lParam;

    if (lpnmh->code == PGN_CALCSIZE) {
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

    if (lpnmh->code == PGN_SCROLL) {
	LPNMPGSCROLL lppgs = (LPNMPGSCROLL)lParam;

	lppgs->iScroll = (lppgs->iDir & (PGF_SCROLLLEFT | PGF_SCROLLRIGHT)) ?
	                  infoPtr->nButtonWidth : infoPtr->nButtonHeight;
	TRACE("processed PGN_SCROLL, returning scroll=%d, dir=%d\n",
	      lppgs->iScroll, lppgs->iDir);
	return 0;
    }


    TRACE("passing WM_NOTIFY!\n");

    if ((infoPtr->hwndToolTip) && (lpnmh->hwndFrom == infoPtr->hwndToolTip)) {
	if (infoPtr->bNtfUnicode)
	    return SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
				 wParam, lParam);
	else
	    return SendMessageA (infoPtr->hwndNotify, WM_NOTIFY,
				 wParam, lParam);

#if 0
	if (lpnmh->code == TTN_GETDISPINFOA) {
	    LPNMTTDISPINFOA lpdi = (LPNMTTDISPINFOA)lParam;

	    FIXME("retrieving ASCII string\n");

	}
	else if (lpnmh->code == TTN_GETDISPINFOW) {
	    LPNMTTDISPINFOW lpdi = (LPNMTTDISPINFOW)lParam;

	    FIXME("retrieving UNICODE string\n");

	}
#endif
    }

    return 0;
}


static LRESULT
TOOLBAR_NotifyFormatFake(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* remove this routine when Toolbar is improved to pass infoPtr
     * around instead of hwnd.
     */
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    return TOOLBAR_NotifyFormat(infoPtr, wParam, lParam);
}


static LRESULT
TOOLBAR_NotifyFormat(TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT i;

    if (lParam == NF_REQUERY) {
	i = SendMessageA(infoPtr->hwndNotify,
			 WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);
	if ((i < NFR_ANSI) || (i > NFR_UNICODE)) {
	    ERR("wrong response to WM_NOTIFYFORMAT (%d), assuming ANSI\n",
		i);
	    i = NFR_ANSI;
	}
	infoPtr->bNtfUnicode = (i == NFR_UNICODE) ? 1 : 0;
	return (LRESULT)i;
    }
    return (LRESULT)((infoPtr->bUnicode) ? NFR_UNICODE : NFR_ANSI);
}


static LRESULT
TOOLBAR_Paint (HWND hwnd, WPARAM wParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(hwnd);
    HDC hdc;
    PAINTSTRUCT ps;

    /* fill ps.rcPaint with a default rect */
    memcpy(&(ps.rcPaint), &(infoPtr->rcBound), sizeof(infoPtr->rcBound));

    hdc = wParam==0 ? BeginPaint(hwnd, &ps) : (HDC)wParam;

    TRACE("psrect=(%ld,%ld)-(%ld,%ld)\n",
	  ps.rcPaint.left, ps.rcPaint.top,
	  ps.rcPaint.right, ps.rcPaint.bottom);

    TOOLBAR_Refresh (hwnd, hdc, &ps);
    if (!wParam) EndPaint (hwnd, &ps);

    return 0;
}


static LRESULT
TOOLBAR_SetRedraw (HWND hwnd, WPARAM wParam, LPARAM lParam)
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
TOOLBAR_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT parent_rect;
    RECT window_rect;
    HWND parent;
    INT  x, y;
    INT  cx, cy;
    INT  flags;
    UINT uPosFlags = 0;

    /* Resize deadlock check */
    if (infoPtr->bAutoSize) {
	infoPtr->bAutoSize = FALSE;
	return 0;
    }

    /* FIXME: optimize to only update size if the new size doesn't */
    /* match the current size */

    flags = (INT) wParam;

    /* FIXME for flags =
     * SIZE_MAXIMIZED, SIZE_MAXSHOW, SIZE_MINIMIZED
     */

    TRACE("sizing toolbar!\n");

    if (flags == SIZE_RESTORED) {
	/* width and height don't apply */
	parent = GetParent (hwnd);
	GetClientRect(parent, &parent_rect);
	x = parent_rect.left;
	y = parent_rect.top;

	if (dwStyle & CCS_NORESIZE) {
	    uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);

	    /*
             * this sets the working width of the toolbar, and
             * Calc Toolbar will not adjust it, only the height
             */
	    infoPtr->nWidth = parent_rect.right - parent_rect.left;
	    cy = infoPtr->nHeight;
	    cx = infoPtr->nWidth;
	    TOOLBAR_CalcToolbar (hwnd);
	    infoPtr->nWidth = cx;
	    infoPtr->nHeight = cy;
	}
	else {
	    infoPtr->nWidth = parent_rect.right - parent_rect.left;
	    TOOLBAR_CalcToolbar (hwnd);
	    cy = infoPtr->nHeight;
	    cx = infoPtr->nWidth;

	    if ((dwStyle & CCS_BOTTOM) == CCS_NOMOVEY) {
		GetWindowRect(hwnd, &window_rect);
		ScreenToClient(parent, (LPPOINT)&window_rect.left);
                y = window_rect.top;
	    }
            if ((dwStyle & CCS_BOTTOM) == CCS_BOTTOM) {
                GetWindowRect(hwnd, &window_rect);
                y = parent_rect.bottom -
                    ( window_rect.bottom - window_rect.top);
            }
	}

	if (dwStyle & CCS_NOPARENTALIGN) {
	    uPosFlags |= SWP_NOMOVE;
	    cy = infoPtr->nHeight;
	    cx = infoPtr->nWidth;
	}

	if (!(dwStyle & CCS_NODIVIDER))
	    cy += GetSystemMetrics(SM_CYEDGE);

	if (dwStyle & WS_BORDER)
	{
	    x = y = 1;
	    cy += GetSystemMetrics(SM_CYEDGE);
	    cx += GetSystemMetrics(SM_CYEDGE);
	}

	SetWindowPos (hwnd, 0,  x,  y, cx, cy, uPosFlags | SWP_NOZORDER);
    }
    return 0;
}


static LRESULT
TOOLBAR_StyleChanged (HWND hwnd, INT nType, LPSTYLESTRUCT lpStyle)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    if (nType == GWL_STYLE) {
	if (lpStyle->styleNew & TBSTYLE_LIST) {
	    infoPtr->dwDTFlags = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	}
	else {
	    infoPtr->dwDTFlags = DT_CENTER;
	}
	infoPtr->bTransparent = (lpStyle->styleNew & TBSTYLE_TRANSPARENT);
	infoPtr->bBtnTranspnt = (lpStyle->styleNew &
				 (TBSTYLE_FLAT | TBSTYLE_LIST));
	TOOLBAR_CheckStyle (hwnd, lpStyle->styleNew);

        TRACE("new style 0x%08lx\n", lpStyle->styleNew);
    }

    TOOLBAR_CalcToolbar(hwnd);

    TOOLBAR_AutoSize (hwnd);

    InvalidateRect(hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TOOLBAR_SysColorChange (HWND hwnd)
{
    COMCTL32_RefreshSysColors();

    return 0;
}



static LRESULT WINAPI
ToolbarWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr (hwnd);

    TRACE("hwnd=%p msg=%x wparam=%x lparam=%lx\n",
	  hwnd, uMsg, /* SPY_GetMsgName(uMsg), */ wParam, lParam);

    if (!TOOLBAR_GetInfoPtr(hwnd) && (uMsg != WM_NCCREATE))
	return DefWindowProcA( hwnd, uMsg, wParam, lParam );

    switch (uMsg)
    {
	case TB_ADDBITMAP:
	    return TOOLBAR_AddBitmap (hwnd, wParam, lParam);

	case TB_ADDBUTTONSA:
	    return TOOLBAR_AddButtonsA (hwnd, wParam, lParam);

	case TB_ADDBUTTONSW:
	    return TOOLBAR_AddButtonsW (hwnd, wParam, lParam);

	case TB_ADDSTRINGA:
	    return TOOLBAR_AddStringA (hwnd, wParam, lParam);

	case TB_ADDSTRINGW:
	    return TOOLBAR_AddStringW (hwnd, wParam, lParam);

	case TB_AUTOSIZE:
	    return TOOLBAR_AutoSize (hwnd);

	case TB_BUTTONCOUNT:
	    return TOOLBAR_ButtonCount (hwnd, wParam, lParam);

	case TB_BUTTONSTRUCTSIZE:
	    return TOOLBAR_ButtonStructSize (hwnd, wParam, lParam);

	case TB_CHANGEBITMAP:
	    return TOOLBAR_ChangeBitmap (hwnd, wParam, lParam);

	case TB_CHECKBUTTON:
	    return TOOLBAR_CheckButton (hwnd, wParam, lParam);

	case TB_COMMANDTOINDEX:
	    return TOOLBAR_CommandToIndex (hwnd, wParam, lParam);

	case TB_CUSTOMIZE:
	    return TOOLBAR_Customize (hwnd);

	case TB_DELETEBUTTON:
	    return TOOLBAR_DeleteButton (hwnd, wParam, lParam);

	case TB_ENABLEBUTTON:
	    return TOOLBAR_EnableButton (hwnd, wParam, lParam);

	case TB_GETANCHORHIGHLIGHT:
	    return TOOLBAR_GetAnchorHighlight (hwnd);

	case TB_GETBITMAP:
	    return TOOLBAR_GetBitmap (hwnd, wParam, lParam);

	case TB_GETBITMAPFLAGS:
	    return TOOLBAR_GetBitmapFlags (hwnd, wParam, lParam);

	case TB_GETBUTTON:
	    return TOOLBAR_GetButton (hwnd, wParam, lParam);

	case TB_GETBUTTONINFOA:
	    return TOOLBAR_GetButtonInfoA (hwnd, wParam, lParam);

	case TB_GETBUTTONINFOW:
	    return TOOLBAR_GetButtonInfoW (hwnd, wParam, lParam);

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

/*	case TB_GETINSERTMARK:			*/ /* 4.71 */
/*	case TB_GETINSERTMARKCOLOR:		*/ /* 4.71 */

	case TB_GETITEMRECT:
	    return TOOLBAR_GetItemRect (hwnd, wParam, lParam);

	case TB_GETMAXSIZE:
	    return TOOLBAR_GetMaxSize (hwnd, wParam, lParam);

/*	case TB_GETOBJECT:			*/ /* 4.71 */

	case TB_GETPADDING:
	    return TOOLBAR_GetPadding (hwnd);

	case TB_GETRECT:
	    return TOOLBAR_GetRect (hwnd, wParam, lParam);

	case TB_GETROWS:
	    return TOOLBAR_GetRows (hwnd, wParam, lParam);

	case TB_GETSTATE:
	    return TOOLBAR_GetState (hwnd, wParam, lParam);

	case TB_GETSTYLE:
	    return TOOLBAR_GetStyle (hwnd, wParam, lParam);

	case TB_GETTEXTROWS:
	    return TOOLBAR_GetTextRows (hwnd, wParam, lParam);

	case TB_GETTOOLTIPS:
	    return TOOLBAR_GetToolTips (hwnd, wParam, lParam);

	case TB_GETUNICODEFORMAT:
	    return TOOLBAR_GetUnicodeFormat (hwnd, wParam, lParam);

	case TB_HIDEBUTTON:
	    return TOOLBAR_HideButton (hwnd, wParam, lParam);

	case TB_HITTEST:
	    return TOOLBAR_HitTest (hwnd, wParam, lParam);

	case TB_INDETERMINATE:
	    return TOOLBAR_Indeterminate (hwnd, wParam, lParam);

	case TB_INSERTBUTTONA:
	    return TOOLBAR_InsertButtonA (hwnd, wParam, lParam);

	case TB_INSERTBUTTONW:
	    return TOOLBAR_InsertButtonW (hwnd, wParam, lParam);

/*	case TB_INSERTMARKHITTEST:		*/ /* 4.71 */

	case TB_ISBUTTONCHECKED:
	    return TOOLBAR_IsButtonChecked (hwnd, wParam, lParam);

	case TB_ISBUTTONENABLED:
	    return TOOLBAR_IsButtonEnabled (hwnd, wParam, lParam);

	case TB_ISBUTTONHIDDEN:
	    return TOOLBAR_IsButtonHidden (hwnd, wParam, lParam);

	case TB_ISBUTTONHIGHLIGHTED:
	    return TOOLBAR_IsButtonHighlighted (hwnd, wParam, lParam);

	case TB_ISBUTTONINDETERMINATE:
	    return TOOLBAR_IsButtonIndeterminate (hwnd, wParam, lParam);

	case TB_ISBUTTONPRESSED:
	    return TOOLBAR_IsButtonPressed (hwnd, wParam, lParam);

	case TB_LOADIMAGES:			   /* 4.70 */
	    FIXME("missing standard imagelists\n");
	    return 0;

/*	case TB_MAPACCELERATORA:		*/ /* 4.71 */
/*	case TB_MAPACCELERATORW:		*/ /* 4.71 */
/*	case TB_MARKBUTTON:			*/ /* 4.71 */
/*	case TB_MOVEBUTTON:			*/ /* 4.71 */

	case TB_PRESSBUTTON:
	    return TOOLBAR_PressButton (hwnd, wParam, lParam);

	case TB_REPLACEBITMAP:
            return TOOLBAR_ReplaceBitmap (hwnd, wParam, lParam);

	case TB_SAVERESTOREA:
	    return TOOLBAR_SaveRestoreA (hwnd, wParam, lParam);

	case TB_SAVERESTOREW:
	    return TOOLBAR_SaveRestoreW (hwnd, wParam, lParam);

	case TB_SETANCHORHIGHLIGHT:
	    return TOOLBAR_SetAnchorHighlight (hwnd, wParam);

	case TB_SETBITMAPSIZE:
	    return TOOLBAR_SetBitmapSize (hwnd, wParam, lParam);

	case TB_SETBUTTONINFOA:
	    return TOOLBAR_SetButtonInfoA (hwnd, wParam, lParam);

	case TB_SETBUTTONINFOW:
	    return TOOLBAR_SetButtonInfoW (hwnd, wParam, lParam);

	case TB_SETBUTTONSIZE:
	    return TOOLBAR_SetButtonSize (hwnd, wParam, lParam);

	case TB_SETBUTTONWIDTH:
	    return TOOLBAR_SetButtonWidth (hwnd, wParam, lParam);

	case TB_SETCMDID:
	    return TOOLBAR_SetCmdId (hwnd, wParam, lParam);

	case TB_SETDISABLEDIMAGELIST:
	    return TOOLBAR_SetDisabledImageList (hwnd, wParam, lParam);

	case TB_SETDRAWTEXTFLAGS:
	    return TOOLBAR_SetDrawTextFlags (hwnd, wParam, lParam);

	case TB_SETEXTENDEDSTYLE:
	    return TOOLBAR_SetExtendedStyle (hwnd, wParam, lParam);

	case TB_SETHOTIMAGELIST:
	    return TOOLBAR_SetHotImageList (hwnd, wParam, lParam);

	case TB_SETHOTITEM:
	    return TOOLBAR_SetHotItem (hwnd, wParam);

	case TB_SETIMAGELIST:
	    return TOOLBAR_SetImageList (hwnd, wParam, lParam);

	case TB_SETINDENT:
	    return TOOLBAR_SetIndent (hwnd, wParam, lParam);

/*	case TB_SETINSERTMARK:			*/ /* 4.71 */

	case TB_SETINSERTMARKCOLOR:
	    return TOOLBAR_SetInsertMarkColor (hwnd, wParam, lParam);

	case TB_SETMAXTEXTROWS:
	    return TOOLBAR_SetMaxTextRows (hwnd, wParam, lParam);

	case TB_SETPADDING:
	    return TOOLBAR_SetPadding (hwnd, wParam, lParam);

	case TB_SETPARENT:
	    return TOOLBAR_SetParent (hwnd, wParam, lParam);

	case TB_SETROWS:
	    return TOOLBAR_SetRows (hwnd, wParam, lParam);

	case TB_SETSTATE:
	    return TOOLBAR_SetState (hwnd, wParam, lParam);

	case TB_SETSTYLE:
	    return TOOLBAR_SetStyle (hwnd, wParam, lParam);

	case TB_SETTOOLTIPS:
	    return TOOLBAR_SetToolTips (hwnd, wParam, lParam);

	case TB_SETUNICODEFORMAT:
	    return TOOLBAR_SetUnicodeFormat (hwnd, wParam, lParam);

	case TB_UNKWN45E:
	    return TOOLBAR_Unkwn45E (hwnd, wParam, lParam);

	case TB_UNKWN463:
	    return TOOLBAR_Unkwn463 (hwnd, wParam, lParam);


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
	    return TOOLBAR_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	  return TOOLBAR_Destroy (hwnd, wParam, lParam);

	case WM_ERASEBKGND:
	    return TOOLBAR_EraseBackground (hwnd, wParam, lParam);

	case WM_GETFONT:
		return TOOLBAR_GetFont (hwnd, wParam, lParam);

/*	case WM_KEYDOWN: */
/*	case WM_KILLFOCUS: */

	case WM_LBUTTONDBLCLK:
	    return TOOLBAR_LButtonDblClk (hwnd, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return TOOLBAR_LButtonDown (hwnd, wParam, lParam);

	case WM_LBUTTONUP:
	    return TOOLBAR_LButtonUp (hwnd, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TOOLBAR_MouseMove (hwnd, wParam, lParam);

	case WM_MOUSELEAVE:
	    return TOOLBAR_MouseLeave (hwnd, wParam, lParam);

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
	    return TOOLBAR_Notify (hwnd, wParam, lParam);

	case WM_NOTIFYFORMAT:
	    TOOLBAR_NotifyFormatFake (hwnd, wParam, lParam);

	case WM_PAINT:
	    return TOOLBAR_Paint (hwnd, wParam);

	case WM_SETREDRAW:
	    return TOOLBAR_SetRedraw (hwnd, wParam, lParam);

	case WM_SIZE:
	    return TOOLBAR_Size (hwnd, wParam, lParam);

	case WM_STYLECHANGED:
	    return TOOLBAR_StyleChanged (hwnd, (INT)wParam, (LPSTYLESTRUCT)lParam);

	case WM_SYSCOLORCHANGE:
	    return TOOLBAR_SysColorChange (hwnd);

/*	case WM_WININICHANGE: */

	case WM_CHARTOITEM:
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_VKEYTOITEM:
            return SendMessageA (infoPtr->hwndNotify, uMsg, wParam, lParam);

	/* We see this in Outlook Express 5.x and just does DefWindowProc */
        case PGM_FORWARDMOUSE:
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
TOOLBAR_Register (void)
{
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)ToolbarWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLBAR_INFO *);
    wndClass.hCursor       = LoadCursorA (0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TOOLBARCLASSNAMEA;

    RegisterClassA (&wndClass);
}


VOID
TOOLBAR_Unregister (void)
{
    UnregisterClassA (TOOLBARCLASSNAMEA, NULL);
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

	c = (PIMLENTRY) Alloc(sizeof(IMLENTRY));
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


static PIMLENTRY TOOLBAR_GetImageListEntry(PIMLENTRY *pies, INT cies, INT id)
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


static HIMAGELIST TOOLBAR_GetImageList(PIMLENTRY *pies, INT cies, INT id)
{
    HIMAGELIST himlDef = 0;
    PIMLENTRY pie = TOOLBAR_GetImageListEntry(pies, cies, id);

    if (pie)
        himlDef = pie->himl;

    return himlDef;
}


static BOOL TOOLBAR_GetButtonInfo(TOOLBAR_INFO *infoPtr, NMTOOLBARW *nmtb)
{
    if (infoPtr->bUnicode)
        return TOOLBAR_SendNotify ((NMHDR *) nmtb, infoPtr, TBN_GETBUTTONINFOW);
    else
    {
        CHAR Buffer[256];
        NMTOOLBARA nmtba;
        BOOL bRet = FALSE;

        nmtba.iItem = nmtb->iItem;
        nmtba.pszText = Buffer;
        nmtba.cchText = 256;
        ZeroMemory(nmtba.pszText, nmtba.cchText);

        if (TOOLBAR_SendNotify ((NMHDR *) &nmtba, infoPtr, TBN_GETBUTTONINFOA))
        {
            int ccht = strlen(nmtba.pszText);
            if (ccht)
               MultiByteToWideChar(CP_ACP, 0, (LPCSTR)nmtba.pszText, -1, 
                  nmtb->pszText, nmtb->cchText);

            memcpy(&nmtb->tbButton, &nmtba.tbButton, sizeof(TBBUTTON));
            bRet = TRUE;
        }

        return bRet;
    }
}


static BOOL TOOLBAR_IsButtonRemovable(TOOLBAR_INFO *infoPtr,
	int iItem, PCUSTOMBUTTON btnInfo)
{
    NMTOOLBARA nmtb;

    nmtb.iItem = iItem;
    memcpy(&nmtb.tbButton, &btnInfo->btn, sizeof(TBBUTTON));

    return TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr, TBN_QUERYDELETE);
}
