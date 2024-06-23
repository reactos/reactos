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
 *     - TB_GETOBJECT
 *     - TB_INSERTMARKHITTEST
 *     - TB_SAVERESTORE
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
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolbar);

static HCURSOR hCursorDrag = NULL;

typedef struct
{
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    BOOL  bHot;
    BOOL  bDropDownPressed;
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
#ifdef __REACTOS__
    SIZE     szBarPadding;       /* padding values around the toolbar (NOT USED BUT STORED) */
    SIZE     szSpacing;       /* spacing values between buttons */
    MARGINS  themeMargins;
#endif
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

/* default padding inside a button */
#define DEFPAD_CX 7
#define DEFPAD_CY 6

#ifdef __REACTOS__
/* default space between buttons and between rows */
#define DEFSPACE_CX 7
#define DEFSPACE_CY 6
#endif

#define DEFLISTGAP 4

/* vertical padding used in list mode when image is present */
#ifdef __REACTOS__
#define LISTPAD_CY 2
#else
#define LISTPAD_CY 9
#endif

/* how wide to treat the bitmap if it isn't present */
#define NONLIST_NOTEXT_OFFSET 2

#define TOOLBAR_NOWHERE (-1)

/* Used to find undocumented extended styles */
#define TBSTYLE_EX_ALL (TBSTYLE_EX_DRAWDDARROWS | \
                        TBSTYLE_EX_VERTICAL | \
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
static BOOL TOOLBAR_IsButtonRemovable(const TOOLBAR_INFO *infoPtr, int iItem, const CUSTOMBUTTON *btnInfo);
static HIMAGELIST TOOLBAR_GetImageList(const PIMLENTRY *pies, INT cies, INT id);
static PIMLENTRY TOOLBAR_GetImageListEntry(const PIMLENTRY *pies, INT cies, INT id);
static VOID TOOLBAR_DeleteImageList(PIMLENTRY **pies, INT *cies);
static HIMAGELIST TOOLBAR_InsertImageList(PIMLENTRY **pies, INT *cies, HIMAGELIST himl, INT id);
static LRESULT TOOLBAR_LButtonDown(TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam);
static void TOOLBAR_LayoutToolbar(TOOLBAR_INFO *infoPtr);
static LRESULT TOOLBAR_AutoSize(TOOLBAR_INFO *infoPtr);
static void TOOLBAR_CheckImageListIconSize(TOOLBAR_INFO *infoPtr);
static void TOOLBAR_TooltipAddTool(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *button);
static void TOOLBAR_TooltipSetRect(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *button);
static LRESULT TOOLBAR_SetButtonInfo(TOOLBAR_INFO *infoPtr, INT Id,
                                     const TBBUTTONINFOW *lptbbi, BOOL isW);


static inline int default_top_margin(const TOOLBAR_INFO *infoPtr)
{
#ifdef __REACTOS__
    if (infoPtr->iVersion == 6)
        return infoPtr->szBarPadding.cy;
#endif
    return (infoPtr->dwStyle & TBSTYLE_FLAT ? 0 : TOP_BORDER);
}

static inline BOOL TOOLBAR_HasDropDownArrows(DWORD exStyle)
{
    return (exStyle & TBSTYLE_EX_DRAWDDARROWS) != 0;
}

static inline BOOL button_has_ddarrow(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *btnPtr)
{
    return (TOOLBAR_HasDropDownArrows( infoPtr->dwExStyle ) && (btnPtr->fsStyle & BTNS_DROPDOWN)) ||
        (btnPtr->fsStyle & BTNS_WHOLEDROPDOWN);
}

#ifdef __REACTOS__
static inline DWORD TOOLBAR_GetButtonDTFlags(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *btnPtr)
{
    DWORD dwDTFlags = infoPtr->dwDTFlags;
    if (btnPtr->fsStyle & BTNS_NOPREFIX)
        dwDTFlags |= DT_NOPREFIX;
    return dwDTFlags;
}
#endif

static LPWSTR
TOOLBAR_GetText(const TOOLBAR_INFO *infoPtr, const TBUTTON_INFO *btnPtr)
{
    LPWSTR lpText = NULL;

    /* NOTE: iString == -1 is undocumented */
    if (!IS_INTRESOURCE(btnPtr->iString) && (btnPtr->iString != -1))
        lpText = (LPWSTR)btnPtr->iString;
    else if ((btnPtr->iString >= 0) && (btnPtr->iString < infoPtr->nNumStrings))
        lpText = infoPtr->strings[btnPtr->iString];

    return lpText;
}

static void
TOOLBAR_DumpTBButton(const TBBUTTON *tbb, BOOL fUnicode)
{
    TRACE("TBBUTTON: id %d, bitmap=%d, state=%02x, style=%02x, data=%p, stringid=%p (%s)\n", tbb->idCommand,
        tbb->iBitmap, tbb->fsState, tbb->fsStyle, (void *)tbb->dwData, (void *)tbb->iString,
        tbb->iString != -1 ? (fUnicode ? debugstr_w((LPWSTR)tbb->iString) : debugstr_a((LPSTR)tbb->iString)) : "");
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

static inline BOOL
TOOLBAR_ButtonHasString(const TBUTTON_INFO *btnPtr)
{
    return HIWORD(btnPtr->iString) && btnPtr->iString != -1;
}

static void set_string_index( TBUTTON_INFO *btn, INT_PTR str, BOOL unicode )
{
    if (!IS_INTRESOURCE( str ) && str != -1)
    {
        if (!TOOLBAR_ButtonHasString( btn )) btn->iString = 0;

        if (unicode)
            Str_SetPtrW( (WCHAR **)&btn->iString, (WCHAR *)str );
        else
            Str_SetPtrAtoW( (WCHAR **)&btn->iString, (char *)str );
    }
    else
    {
        if (TOOLBAR_ButtonHasString( btn )) Free( (WCHAR *)btn->iString );

        btn->iString  = str;
    }
}

static void set_stringT( TBUTTON_INFO *btn, const WCHAR *str, BOOL unicode )
{
    if (IS_INTRESOURCE( (DWORD_PTR)str ) || (DWORD_PTR)str == -1) return;
    set_string_index( btn, (DWORD_PTR)str, unicode );
}

static void free_string( TBUTTON_INFO *btn )
{
    set_string_index( btn, 0, TRUE );

}

/***********************************************************************
* 		TOOLBAR_CheckStyle
*
* This function validates that the styles set are implemented and
* issues FIXMEs warning of possible problems. In a perfect world this
* function should be null.
*/
static void
TOOLBAR_CheckStyle (const TOOLBAR_INFO *infoPtr)
{
    if (infoPtr->dwStyle & TBSTYLE_REGISTERDROP)
	FIXME("[%p] TBSTYLE_REGISTERDROP not implemented\n", infoPtr->hwndSelf);
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
	WARN("bitmap for ID %d, index %d is not valid, number of bitmaps in imagelist: %d\n",
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
#ifdef __REACTOS__
                    const TBUTTON_INFO *btnPtr,
#endif
                    const NMTBCUSTOMDRAW *tbcd, DWORD dwItemCDFlag)
{
    HDC hdc = tbcd->nmcd.hdc;
    HFONT  hOldFont = 0;
    COLORREF clrOld = 0;
    COLORREF clrOldBk = 0;
    int oldBkMode = 0;
    UINT state = tbcd->nmcd.uItemState;
#ifdef __REACTOS__
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    DWORD dwDTFlags = TOOLBAR_GetButtonDTFlags(infoPtr, btnPtr);
#endif

    /* draw text */
    if (lpText && infoPtr->nMaxTextRows > 0) {
        TRACE("string=%s rect=(%s)\n", debugstr_w(lpText),
              wine_dbgstr_rect(rcText));

	hOldFont = SelectObject (hdc, infoPtr->hFont);
#ifdef __REACTOS__
    if (theme)
    {
        DWORD dwDTFlags2 = 0;
        int partId = TP_BUTTON;
        int stateId = TS_NORMAL;

        if (state & CDIS_DISABLED)
        {
            stateId = TS_DISABLED;
            dwDTFlags2 = DTT_GRAYED;
        }
        else if (state & CDIS_SELECTED)
            stateId = TS_PRESSED;
        else if (state & CDIS_CHECKED)
            stateId = (state & CDIS_HOT) ? TS_HOTCHECKED : TS_HOT;
        else if (state & CDIS_HOT)
            stateId = TS_HOT;

        DrawThemeText(theme, hdc, partId, stateId, lpText, -1, dwDTFlags, dwDTFlags2, rcText);
        SelectObject (hdc, hOldFont);
        return;
    }
#endif

	if ((state & CDIS_HOT) && (dwItemCDFlag & TBCDRF_HILITEHOTTRACK )) {
	    clrOld = SetTextColor (hdc, tbcd->clrTextHighlight);
	}
	else if (state & CDIS_DISABLED) {
	    clrOld = SetTextColor (hdc, tbcd->clrBtnHighlight);
	    OffsetRect (rcText, 1, 1);
#ifdef __REACTOS__
	    DrawTextW (hdc, lpText, -1, rcText, dwDTFlags);
#else
	    DrawTextW (hdc, lpText, -1, rcText, infoPtr->dwDTFlags);
#endif
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

#ifdef __REACTOS__
	DrawTextW (hdc, lpText, -1, rcText, dwDTFlags);
#else
	DrawTextW (hdc, lpText, -1, rcText, infoPtr->dwDTFlags);
#endif
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
TOOLBAR_TranslateState(const TBUTTON_INFO *btnPtr, BOOL captured)
{
    UINT retstate = 0;

    retstate |= (btnPtr->fsState & TBSTATE_CHECKED) ? CDIS_CHECKED  : 0;
    retstate |= (btnPtr->fsState & TBSTATE_PRESSED) ? CDIS_SELECTED : 0;
    retstate |= (btnPtr->fsState & TBSTATE_ENABLED) ? 0 : CDIS_DISABLED;
    retstate |= (btnPtr->fsState & TBSTATE_MARKED ) ? CDIS_MARKED   : 0;
    retstate |= (btnPtr->bHot & !captured         ) ? CDIS_HOT      : 0;
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
    BOOL draw_masked = FALSE, draw_desaturated = FALSE;
    INT index;
    INT offset = 0;
    UINT draw_flags = ILD_TRANSPARENT;
#ifdef __REACTOS__
    IMAGEINFO info = {0};
    BITMAP bm = {0};
#endif

    if (tbcd->nmcd.uItemState & (CDIS_DISABLED | CDIS_INDETERMINATE))
    {
        himl = TOOLBAR_GetImageListForDrawing(infoPtr, btnPtr, IMAGE_LIST_DISABLED, &index);
        if (!himl)
        {
            himl = TOOLBAR_GetImageListForDrawing(infoPtr, btnPtr, IMAGE_LIST_DEFAULT, &index);

#ifdef __REACTOS__
            ImageList_GetImageInfo(himl, index, &info);
            GetObjectW(info.hbmImage, sizeof(bm), &bm);

            if (bm.bmBitsPixel == 32)
            {
                draw_desaturated = TRUE;
            }
            else
            {
                draw_masked = TRUE;
            }
#else
            draw_masked = TRUE;
#endif
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
    {
        /* code path for drawing flat disabled icons without alpha channel */
        TOOLBAR_DrawMasked (himl, index, tbcd->nmcd.hdc, left + offset, top + offset, draw_flags);
    }
    else if (draw_desaturated)
    {
        /* code path for drawing disabled, alpha-blended (32bpp) icons */
        IMAGELISTDRAWPARAMS imldp = {0};

        imldp.cbSize = sizeof(imldp);
        imldp.himl   = himl;
        imldp.i      = index;
        imldp.hdcDst = tbcd->nmcd.hdc,
        imldp.x      = offset + left;
        imldp.y      = offset + top;
        imldp.rgbBk  = CLR_NONE;
        imldp.rgbFg  = CLR_DEFAULT;
        imldp.fStyle = ILD_TRANSPARENT;
        imldp.fState = ILS_ALPHA | ILS_SATURATE;
        imldp.Frame  = 192;

        ImageList_DrawIndirect (&imldp);
    }
    else
    {
        /* code path for drawing standard icons as-is */
        ImageList_Draw (himl, index, tbcd->nmcd.hdc, left + offset, top + offset, draw_flags);
    }
}

/* draws a blank frame for a toolbar button */
static void
TOOLBAR_DrawFrame(const TOOLBAR_INFO *infoPtr, const NMTBCUSTOMDRAW *tbcd, const RECT *rect, DWORD dwItemCDFlag)
{
    HDC hdc = tbcd->nmcd.hdc;
    RECT rc = *rect;
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
TOOLBAR_DrawButton (const TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr, HDC hdc, DWORD dwBaseCustDraw)
{
    DWORD dwStyle = infoPtr->dwStyle;
    BOOL hasDropDownArrow = button_has_ddarrow( infoPtr, btnPtr );
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
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);

    rc = btnPtr->rect;
    rcArrow = rc;

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
            if (dwStyle & CCS_VERT) {
                RECT rcsep = rc;
                InflateRect(&rcsep, -infoPtr->szPadding.cx, -infoPtr->szPadding.cy);
                TOOLBAR_DrawFlatHorizontalSeparator (&rcsep, hdc, infoPtr);
            }
            else {
                TOOLBAR_DrawFlatSeparator (&rc, hdc, infoPtr);
            }
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
    rcText = rc;
    rcBitmap = rc;

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
#ifdef __REACTOS__
    rcBitmap.top += infoPtr->themeMargins.cyTopHeight;
#endif

    TRACE("iBitmap=%d, start=(%d,%d) w=%d, h=%d\n",
      btnPtr->iBitmap, rcBitmap.left, rcBitmap.top,
      infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);
    TRACE("Text=%s\n", debugstr_w(lpText));
    TRACE("iListGap=%d, padding = { %d, %d }\n", infoPtr->iListGap, infoPtr->szPadding.cx, infoPtr->szPadding.cy);

    /* calculate text position */
    if (lpText)
    {
        InflateRect(&rcText, -GetSystemMetrics(SM_CXEDGE), 0);
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
    tbcd.rcText.left = 0;
    tbcd.rcText.top = 0;
    tbcd.rcText.right = rcText.right - rc.left;
    tbcd.rcText.bottom = rcText.bottom - rc.top;
    tbcd.nmcd.uItemState = TOOLBAR_TranslateState(btnPtr, infoPtr->bCaptured);
    tbcd.nmcd.hdc = hdc;
    tbcd.nmcd.rc = btnPtr->rect;
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
        tbcd.nmcd.rc = btnPtr->rect;

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

#ifdef __REACTOS__
    if (theme && !(dwItemCDFlag & TBCDRF_NOBACKGROUND))
#else
    if (theme)
#endif
    {
        int partId = drawSepDropDownArrow ? TP_SPLITBUTTON : TP_BUTTON;
        int stateId = TS_NORMAL;
        
        if (tbcd.nmcd.uItemState & CDIS_DISABLED)
            stateId = TS_DISABLED;
        else if (tbcd.nmcd.uItemState & CDIS_SELECTED)
            stateId = TS_PRESSED;
        else if (tbcd.nmcd.uItemState & CDIS_CHECKED)
#ifdef __REACTOS__
            stateId = (tbcd.nmcd.uItemState & CDIS_HOT) ? TS_HOTCHECKED : TS_CHECKED;
#else
            stateId = (tbcd.nmcd.uItemState & CDIS_HOT) ? TS_HOTCHECKED : TS_HOT;
#endif
        else if ((tbcd.nmcd.uItemState & CDIS_HOT)
            || (drawSepDropDownArrow && btnPtr->bDropDownPressed))
            stateId = TS_HOT;
            
        DrawThemeBackground (theme, hdc, partId, stateId, &rc, NULL);
    }

#ifdef __REACTOS__
    if (!theme)
#else
    else
#endif
        TOOLBAR_DrawFrame(infoPtr, &tbcd, &rc, dwItemCDFlag);

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
#ifdef __REACTOS__
                stateId = (tbcd.nmcd.uItemState & CDIS_HOT) ? TS_HOTCHECKED : TS_CHECKED;
#else
                stateId = (tbcd.nmcd.uItemState & CDIS_HOT) ? TS_HOTCHECKED : TS_HOT;
#endif
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
#ifdef __REACTOS__
        TOOLBAR_DrawString(infoPtr, &rcText, lpText, btnPtr, &tbcd, dwItemCDFlag);
#else
        TOOLBAR_DrawString (infoPtr, &rcText, lpText, &tbcd, dwItemCDFlag);
#endif
    SetBkMode (hdc, oldBkMode);

    TOOLBAR_DrawImage(infoPtr, btnPtr, rcBitmap.left, rcBitmap.top, &tbcd, dwItemCDFlag);

    if (hasDropDownArrow && !drawSepDropDownArrow)
    {
        if (tbcd.nmcd.uItemState & (CDIS_DISABLED | CDIS_INDETERMINATE))
        {
            TOOLBAR_DrawArrow(hdc, rcArrow.left+1, rcArrow.top+1 + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnHighlight);
            TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clr3dShadow);
        }
#ifndef __REACTOS__
        else if (tbcd.nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED))
        {
            offset = (dwItemCDFlag & TBCDRF_NOOFFSET) ? 0 : 1;
            TOOLBAR_DrawArrow(hdc, rcArrow.left + offset, rcArrow.top + offset + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnText);
        }
        else
            TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, comctl32_color.clrBtnText);
#else
        else
        {
            COLORREF clr = comctl32_color.clrBtnText;
            if (theme)
                GetThemeColor(theme, TP_BUTTON, TS_NORMAL, TMT_TEXTCOLOR, &clr);

            if (tbcd.nmcd.uItemState & (CDIS_SELECTED | CDIS_CHECKED))
            {
                offset = (dwItemCDFlag & TBCDRF_NOOFFSET) ? 0 : 1;
                TOOLBAR_DrawArrow(hdc, rcArrow.left + offset, rcArrow.top + offset + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, clr);
            }
            else
                TOOLBAR_DrawArrow(hdc, rcArrow.left, rcArrow.top + (rcArrow.bottom - rcArrow.top - ARROW_HEIGHT) / 2, clr);
        }
#endif
    }

    if (dwItemCustDraw & CDRF_NOTIFYPOSTPAINT)
    {
        tbcd.nmcd.dwDrawStage = CDDS_ITEMPOSTPAINT;
        TOOLBAR_SendNotify(&tbcd.nmcd.hdr, infoPtr, NM_CUSTOMDRAW);
    }

}


static void
TOOLBAR_Refresh (TOOLBAR_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps)
{
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

    GetClientRect(infoPtr->hwndSelf, &rcClient);

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
            TOOLBAR_DrawButton(infoPtr, btnPtr, hdc, dwBaseCustDraw);
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
	    GetTextExtentPoint32W (hdc, lpText, lstrlenW (lpText), lpSize);

	    /* feed above size into the rectangle for DrawText */
            SetRect(&myrect, 0, 0, lpSize->cx, lpSize->cy);

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
TOOLBAR_CalcStrings (const TOOLBAR_INFO *infoPtr, LPSIZE lpSize)
{
    TBUTTON_INFO *btnPtr;
    INT i;
    SIZE sz;
    HDC hdc;
    HFONT hOldFont;

    lpSize->cx = 0;
    lpSize->cy = 0;

    if (infoPtr->nMaxTextRows == 0)
        return;

    hdc = GetDC (infoPtr->hwndSelf);
    hOldFont = SelectObject (hdc, infoPtr->hFont);

    if (infoPtr->nNumButtons == 0 && infoPtr->nNumStrings > 0)
    {
        TEXTMETRICW tm;

        GetTextMetricsW(hdc, &tm);
        lpSize->cy = tm.tmHeight;
    }

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
        if(TOOLBAR_GetText(infoPtr, btnPtr))
        {
            TOOLBAR_MeasureString(infoPtr, btnPtr, hdc, &sz);
            if (sz.cx > lpSize->cx)
                lpSize->cx = sz.cx;
            if (sz.cy > lpSize->cy)
                lpSize->cy = sz.cy;
        }
    }

    SelectObject (hdc, hOldFont);
    ReleaseDC (infoPtr->hwndSelf, hdc);

    TRACE("max string size %d x %d\n", lpSize->cx, lpSize->cy);
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
* Note: TBSTYLE_WRAPABLE or TBSTYLE_EX_VERTICAL can be used also to allow
* vertical toolbar lists.
*/

static void
TOOLBAR_WrapToolbar(TOOLBAR_INFO *infoPtr)
{
    TBUTTON_INFO *btnPtr;
    INT x, cx, i, j, width;
    BOOL bButtonWrap;

    /* 	When the toolbar window style is not TBSTYLE_WRAPABLE,	*/
    /*	no layout is necessary. Applications may use this style */
    /*	to perform their own layout on the toolbar. 		*/
    if( !(infoPtr->dwStyle & TBSTYLE_WRAPABLE) &&
	!(infoPtr->dwExStyle & TBSTYLE_EX_VERTICAL) )  return;

#ifdef __REACTOS__ /* workaround CORE-16169 part 1 of 2 */
    /* if width is zero then return */
    if (infoPtr->client_rect.right == 0) return;
#endif

    btnPtr = infoPtr->buttons;
    x  = infoPtr->nIndent;
    width = infoPtr->client_rect.right - infoPtr->client_rect.left;

    bButtonWrap = FALSE;

    TRACE("start ButtonWidth=%d, BitmapWidth=%d, width=%d, nIndent=%d\n",
	  infoPtr->nButtonWidth, infoPtr->nBitmapWidth, width,
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

        if (!btnPtr[i].cx && button_has_ddarrow( infoPtr, btnPtr + i ))
            cx += DDARROW_WIDTH;

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
        if ((x + cx - (infoPtr->nButtonWidth - infoPtr->nBitmapWidth) / 2 > width) ||
            ((x == infoPtr->nIndent) && (cx > width)))
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
* |                    | pad.cy / 2          | centered    | |
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
* |                     | centered    | | LISTPAD_CY +
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
* |    centered       |   Bitmap   |  | |
* |<----------------->|            |  | |
* |                   +------------+  | |
* |                         ^         | |
* |                       1 |         | |
* |                         -         | |
* |     centered    +---------------+ | |
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
* |    centered     +-----------------+   | | pad.cy + 2
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
#ifdef __REACTOS__
            sizeButton.cy += infoPtr->szPadding.cy;
            if (!bHasBitmap)
#else
            if (bHasBitmap)
                sizeButton.cy += DEFPAD_CY;
            else
#endif
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
#ifdef __REACTOS__
            sizeButton.cy = infoPtr->nBitmapHeight + infoPtr->szPadding.cy;
#else
            sizeButton.cy = infoPtr->nBitmapHeight + DEFPAD_CY;
#endif
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

#ifdef __REACTOS__
    sizeButton.cx += infoPtr->themeMargins.cxLeftWidth + infoPtr->themeMargins.cxRightWidth;
    sizeButton.cy += infoPtr->themeMargins.cyTopHeight + infoPtr->themeMargins.cyBottomHeight;
#endif

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
TOOLBAR_CalcToolbar (TOOLBAR_INFO *infoPtr)
{
    SIZE  sizeString, sizeButton;
    BOOL validImageList = FALSE;

    TOOLBAR_CalcStrings (infoPtr, &sizeString);

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

    TOOLBAR_LayoutToolbar(infoPtr);
}

static void
TOOLBAR_LayoutToolbar(TOOLBAR_INFO *infoPtr)
{
    TBUTTON_INFO *btnPtr;
    SIZE sizeButton;
    INT i, nRows, nSepRows;
    INT x, y, cx, cy;
    BOOL bWrap;
    BOOL validImageList = TOOLBAR_IsValidImageList(infoPtr, 0);

    TOOLBAR_WrapToolbar(infoPtr);

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
	    TOOLBAR_TooltipSetRect(infoPtr, btnPtr);
	    continue;
	}

	cy = infoPtr->nButtonHeight;

	if (btnPtr->fsStyle & BTNS_SEP) {
	    if (infoPtr->dwStyle & CCS_VERT) {
                cy = (btnPtr->iBitmap > 0) ? btnPtr->iBitmap : SEPARATOR_WIDTH;
                cx = (btnPtr->cx > 0) ? btnPtr->cx : infoPtr->nButtonWidth;
	    }
	    else
                cx = (btnPtr->cx > 0) ? btnPtr->cx :
                    (btnPtr->iBitmap > 0) ? btnPtr->iBitmap : SEPARATOR_WIDTH;
	}
	else
	{
            if (btnPtr->cx)
              cx = btnPtr->cx;
#ifdef __REACTOS__
            /* Revert Wine Commit 5b7b911 as it breaks Explorer Toolbar Buttons
               FIXME: Revisit this when the bug is fixed. CORE-9970 */
            else if ((infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) || 
                (btnPtr->fsStyle & BTNS_AUTOSIZE))
#else
            else if (btnPtr->fsStyle & BTNS_AUTOSIZE)
#endif
            {
              SIZE sz;
	      HDC hdc;
	      HFONT hOldFont;

	      hdc = GetDC (infoPtr->hwndSelf);
	      hOldFont = SelectObject (hdc, infoPtr->hFont);

              TOOLBAR_MeasureString(infoPtr, btnPtr, hdc, &sz);

	      SelectObject (hdc, hOldFont);
	      ReleaseDC (infoPtr->hwndSelf, hdc);

              sizeButton = TOOLBAR_MeasureButton(infoPtr, sz,
                  TOOLBAR_IsValidBitmapIndex(infoPtr, infoPtr->buttons[i].iBitmap),
                  validImageList);
              cx = sizeButton.cx;
            }
            else
	      cx = infoPtr->nButtonWidth;

            /* if size has been set manually then don't add on extra space
             * for the drop down arrow */
            if (!btnPtr->cx && button_has_ddarrow( infoPtr, btnPtr ))
              cx += DDARROW_WIDTH;
	}
	if (btnPtr->fsState & TBSTATE_WRAP)
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
#ifdef __REACTOS__
	        y += cy + infoPtr->szSpacing.cy;
#else
	        y += cy;
#endif
	    else
	    {
               if ( !(infoPtr->dwStyle & CCS_VERT))
                    y += cy + ( (btnPtr->cx > 0 ) ?
                                btnPtr->cx : SEPARATOR_WIDTH) * 2 /3;
		else
#ifdef __REACTOS__
		    y += cy + infoPtr->szSpacing.cy;
#else
		    y += cy;
#endif

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
#ifdef __REACTOS__
	    x += cx + infoPtr->szSpacing.cx;
#else
	    x += cx;
#endif
    }

    /* infoPtr->nRows is the number of rows on the toolbar */
    infoPtr->nRows = nRows + nSepRows + 1;

    TRACE("toolbar button width %d\n", infoPtr->nButtonWidth);
}


static INT
TOOLBAR_InternalHitTest (const TOOLBAR_INFO *infoPtr, const POINT *lpPt, BOOL *button)
{
    TBUTTON_INFO *btnPtr;
    INT i;

    if (button)
        *button = FALSE;

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->fsState & TBSTATE_HIDDEN)
	    continue;

	if (btnPtr->fsStyle & BTNS_SEP) {
	    if (PtInRect (&btnPtr->rect, *lpPt)) {
		TRACE(" ON SEPARATOR %d\n", i);
		return -i;
	    }
	}
	else {
	    if (PtInRect (&btnPtr->rect, *lpPt)) {
		TRACE(" ON BUTTON %d\n", i);
                if (button)
                    *button = TRUE;
		return i;
	    }
	}
    }

    TRACE(" NOWHERE\n");
    return TOOLBAR_NOWHERE;
}


/* worker for TB_ADDBUTTONS and TB_INSERTBUTTON */
static BOOL
TOOLBAR_InternalInsertButtonsT(TOOLBAR_INFO *infoPtr, INT iIndex, UINT nAddButtons, const TBBUTTON *lpTbb, BOOL fUnicode)
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
        INT_PTR str;

        TOOLBAR_DumpTBButton(lpTbb + iButton, fUnicode);

        ZeroMemory(btnPtr, sizeof(*btnPtr));

        btnPtr->iBitmap   = lpTbb[iButton].iBitmap;
        btnPtr->idCommand = lpTbb[iButton].idCommand;
        btnPtr->fsState   = lpTbb[iButton].fsState;
        btnPtr->fsStyle   = lpTbb[iButton].fsStyle;
        btnPtr->dwData    = lpTbb[iButton].dwData;

        if (btnPtr->fsStyle & BTNS_SEP)
            str = -1;
        else
            str = lpTbb[iButton].iString;
        set_string_index( btnPtr, str, fUnicode );
        fHasString |= TOOLBAR_ButtonHasString( btnPtr );

        TOOLBAR_TooltipAddTool(infoPtr, btnPtr);
    }

    if (infoPtr->nNumStrings > 0 || fHasString)
        TOOLBAR_CalcToolbar(infoPtr);
    else
        TOOLBAR_LayoutToolbar(infoPtr);
    TOOLBAR_AutoSize(infoPtr);

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

extern UINT uDragListMessage DECLSPEC_HIDDEN;

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

		    /* insert button into the appropriate list */
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
                        if (*nmtb.pszText)
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
               btnInfo = (PCUSTOMBUTTON)SendDlgItemMessageW (hwnd, wParam, LB_GETITEMDATA, lpdis->itemID, 0);
		if (btnInfo == NULL)
		{
		    FIXME("btnInfo invalid\n");
		    return TRUE;
		}

		/* set colors and select objects */
		oldBk = SetBkColor (lpdis->hDC, (lpdis->itemState & ODS_FOCUS)?comctl32_color.clrHighlight:comctl32_color.clrWindow);
		if (btnInfo->bVirtual)
		   oldText = SetTextColor (lpdis->hDC, comctl32_color.clrGrayText);
		else
		   oldText = SetTextColor (lpdis->hDC, (lpdis->itemState & ODS_FOCUS)?comctl32_color.clrHighlightText:comctl32_color.clrWindowText);
                hPen = CreatePen( PS_SOLID, 1,
                                 (lpdis->itemState & ODS_SELECTED)?comctl32_color.clrHighlight:comctl32_color.clrWindow);
		hOldPen = SelectObject (lpdis->hDC, hPen );
		hOldBrush = SelectObject (lpdis->hDC, GetSysColorBrush ((lpdis->itemState & ODS_FOCUS)?COLOR_HIGHLIGHT:COLOR_WINDOW));

		/* fill background rectangle */
		Rectangle (lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
			   lpdis->rcItem.right, lpdis->rcItem.bottom);

		/* calculate button and text rectangles */
                rcButton = lpdis->rcItem;
		InflateRect (&rcButton, -1, -1);
                rcText = rcButton;
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
    else if (bitmap->hInst == COMCTL32_hModule)
        hbmLoad = LoadImageW( bitmap->hInst, MAKEINTRESOURCEW(bitmap->nID),
                              IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
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
                                ILC_COLOR32|ILC_MASK, 8, 2);
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
TOOLBAR_AddBitmap (TOOLBAR_INFO *infoPtr, INT count, const TBADDBITMAP *lpAddBmp)
{
    TBITMAP_INFO info;
    INT iSumButtons, i;
    HIMAGELIST himlDef;

    TRACE("hwnd=%p count=%d lpAddBmp=%p\n", infoPtr->hwndSelf, count, lpAddBmp);
    if (!lpAddBmp)
	return -1;

    if (lpAddBmp->hInst == HINST_COMMCTRL)
    {
        info.hInst = COMCTL32_hModule;
        switch (lpAddBmp->nID)
        {
            case IDB_STD_SMALL_COLOR:
            case 2:
	        info.nButtons = 15;
	        info.nID = IDB_STD_SMALL;
	        break;
            case IDB_STD_LARGE_COLOR:
            case 3:
	        info.nButtons = 15;
	        info.nID = IDB_STD_LARGE;
	        break;
            case IDB_VIEW_SMALL_COLOR:
            case 6:
	        info.nButtons = 12;
	        info.nID = IDB_VIEW_SMALL;
	        break;
            case IDB_VIEW_LARGE_COLOR:
            case 7:
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
                WARN("unknown bitmap id, %ld\n", lpAddBmp->nID);
	        return -1;
	}

        TRACE ("adding %d internal bitmaps\n", info.nButtons);

	/* Windows resize all the buttons to the size of a newly added standard image */
	if (lpAddBmp->nID & 1)
	{
	    /* large icons: 24x24. Will make the button 31x30 */
	    SendMessageW (infoPtr->hwndSelf, TB_SETBITMAPSIZE, 0, MAKELPARAM(24, 24));
	}
	else
	{
	    /* small icons: 16x16. Will make the buttons 23x22 */
	    SendMessageW (infoPtr->hwndSelf, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 16));
	}

	TOOLBAR_CalcToolbar (infoPtr);
    }
    else
    {
	info.nButtons = count;
	info.hInst = lpAddBmp->hInst;
	info.nID = lpAddBmp->nID;
	TRACE("adding %d bitmaps\n", info.nButtons);
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
        TRACE ("creating default image list\n");

        himlDef = ImageList_Create (infoPtr->nBitmapWidth, infoPtr->nBitmapHeight,
                                    ILC_COLOR32 | ILC_MASK, info.nButtons, 2);
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

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return iSumButtons;
}


static LRESULT
TOOLBAR_AddButtonsT(TOOLBAR_INFO *infoPtr, INT nAddButtons, const TBBUTTON* lpTbb, BOOL fUnicode)
{
    TRACE("adding %d buttons (unicode=%d)\n", nAddButtons, fUnicode);

    return TOOLBAR_InternalInsertButtonsT(infoPtr, -1, nAddButtons, lpTbb, fUnicode);
}


static LRESULT
TOOLBAR_AddStringW (TOOLBAR_INFO *infoPtr, HINSTANCE hInstance, LPARAM lParam)
{
#define MAX_RESOURCE_STRING_LENGTH 512
    BOOL fFirstString = (infoPtr->nNumStrings == 0);
    INT nIndex = infoPtr->nNumStrings;

    TRACE("%p, %lx\n", hInstance, lParam);

    if (IS_INTRESOURCE(lParam)) {
	WCHAR szString[MAX_RESOURCE_STRING_LENGTH];
	WCHAR delimiter;
	WCHAR *next_delim;
        HRSRC hrsrc;
	WCHAR *p;
	INT len;

	TRACE("adding string from resource\n");

        if (!hInstance) return -1;

        hrsrc = FindResourceW( hInstance, MAKEINTRESOURCEW((LOWORD(lParam) >> 4) + 1),
                               (LPWSTR)RT_STRING );
        if (!hrsrc)
        {
            TRACE("string not found in resources\n");
            return -1;
        }

        len = LoadStringW (hInstance, (UINT)lParam,
                             szString, MAX_RESOURCE_STRING_LENGTH);

        TRACE("len=%d %s\n", len, debugstr_w(szString));
        if (len == 0 || len == 1)
            return nIndex;

        TRACE("delimiter: 0x%x\n", *szString);
        delimiter = *szString;
        p = szString + 1;

        while ((next_delim = wcschr(p, delimiter)) != NULL) {
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
	TRACE("adding string(s) from array\n");
	while (*p) {
            len = lstrlenW (p);

            TRACE("len=%d %s\n", len, debugstr_w(p));
            infoPtr->strings = ReAlloc(infoPtr->strings, sizeof(LPWSTR)*(infoPtr->nNumStrings+1));
            Str_SetPtrW (&infoPtr->strings[infoPtr->nNumStrings], p);
	    infoPtr->nNumStrings++;

	    p += (len+1);
	}
    }

    if (fFirstString)
        TOOLBAR_CalcToolbar(infoPtr);
    return nIndex;
}


static LRESULT
TOOLBAR_AddStringA (TOOLBAR_INFO *infoPtr, HINSTANCE hInstance, LPARAM lParam)
{
    BOOL fFirstString = (infoPtr->nNumStrings == 0);
    LPSTR p;
    INT nIndex;
    INT len;

    TRACE("%p, %lx\n", hInstance, lParam);

    if (IS_INTRESOURCE(lParam))  /* load from resources */
        return TOOLBAR_AddStringW(infoPtr, hInstance, lParam);

    p = (LPSTR)lParam;
    if (p == NULL)
        return -1;

    TRACE("adding string(s) from array\n");
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
        TOOLBAR_CalcToolbar(infoPtr);
    return nIndex;
}


static LRESULT
TOOLBAR_AutoSize (TOOLBAR_INFO *infoPtr)
{
    TRACE("auto sizing, style=%#x\n", infoPtr->dwStyle);
    TRACE("nRows: %d, infoPtr->nButtonHeight: %d\n", infoPtr->nRows, infoPtr->nButtonHeight);

#ifdef __REACTOS__ /* workaround CORE-16169 part 2 of 2 */
    if ((infoPtr->dwStyle & TBSTYLE_WRAPABLE) || (infoPtr->dwExStyle & TBSTYLE_EX_VERTICAL))
    {
        TOOLBAR_LayoutToolbar(infoPtr);
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    }
#endif

    if (!(infoPtr->dwStyle & CCS_NORESIZE))
    {
        RECT window_rect, parent_rect;
        UINT uPosFlags = SWP_NOZORDER | SWP_NOACTIVATE;
        HWND parent;
        INT  x, y, cx, cy;

        parent = GetParent (infoPtr->hwndSelf);

        if (!parent || !infoPtr->bDoRedraw)
            return 0;

        GetClientRect(parent, &parent_rect);

        x = parent_rect.left;
        y = parent_rect.top;

        cy = TOP_BORDER + infoPtr->nRows * infoPtr->nButtonHeight + BOTTOM_BORDER;
        cx = parent_rect.right - parent_rect.left;

        if ((infoPtr->dwStyle & CCS_BOTTOM) == CCS_NOMOVEY)
        {
            GetWindowRect(infoPtr->hwndSelf, &window_rect);
            MapWindowPoints( 0, parent, (POINT *)&window_rect, 2 );
            y = window_rect.top;
        }
        if ((infoPtr->dwStyle & CCS_BOTTOM) == CCS_BOTTOM)
        {
            GetWindowRect(infoPtr->hwndSelf, &window_rect);
            y = parent_rect.bottom - ( window_rect.bottom - window_rect.top);
        }

        if (infoPtr->dwStyle & CCS_NOPARENTALIGN)
            uPosFlags |= SWP_NOMOVE;
    
        if (!(infoPtr->dwStyle & CCS_NODIVIDER))
            cy += GetSystemMetrics(SM_CYEDGE);

        if (infoPtr->dwStyle & WS_BORDER)
        {
            cx += 2 * GetSystemMetrics(SM_CXBORDER);
            cy += 2 * GetSystemMetrics(SM_CYBORDER);
        }

        SetWindowPos(infoPtr->hwndSelf, NULL, x, y, cx, cy, uPosFlags);
    }

    if ((infoPtr->dwStyle & TBSTYLE_WRAPABLE) || (infoPtr->dwExStyle & TBSTYLE_EX_VERTICAL))
    {
        TOOLBAR_LayoutToolbar(infoPtr);
        InvalidateRect( infoPtr->hwndSelf, NULL, TRUE );
    }

    return 0;
}


static inline LRESULT
TOOLBAR_ButtonCount (const TOOLBAR_INFO *infoPtr)
{
    return infoPtr->nNumButtons;
}


static inline LRESULT
TOOLBAR_ButtonStructSize (TOOLBAR_INFO *infoPtr, DWORD Size)
{
    infoPtr->dwStructSize = Size;

    return 0;
}


static LRESULT
TOOLBAR_ChangeBitmap (TOOLBAR_INFO *infoPtr, INT Id, INT Index)
{
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    TRACE("button %d, iBitmap now %d\n", Id, Index);

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->iBitmap = Index;

    /* we HAVE to erase the background, the new bitmap could be */
    /* transparent */
    InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_CheckButton (TOOLBAR_INFO *infoPtr, INT Id, LPARAM lParam)
{
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    INT nOldIndex = -1;
    BOOL bChecked = FALSE;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);

    TRACE("hwnd=%p, btn index=%d, lParam=0x%08lx\n", infoPtr->hwndSelf, nIndex, lParam);

    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    bChecked = (btnPtr->fsState & TBSTATE_CHECKED) != 0;

    if (!LOWORD(lParam))
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
            InvalidateRect(infoPtr->hwndSelf, &infoPtr->buttons[nOldIndex].rect, TRUE);
        InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
    }

    /* FIXME: Send a WM_NOTIFY?? */

    return TRUE;
}


static LRESULT
TOOLBAR_CommandToIndex (const TOOLBAR_INFO *infoPtr, INT Id)
{
    return TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
}


static LRESULT
TOOLBAR_Customize (TOOLBAR_INFO *infoPtr)
{
    CUSTDLG_INFO custInfo;
    LRESULT ret;
    NMHDR nmhdr;

    custInfo.tbInfo = infoPtr;
    custInfo.tbHwnd = infoPtr->hwndSelf;

    /* send TBN_BEGINADJUST notification */
    TOOLBAR_SendNotify (&nmhdr, infoPtr, TBN_BEGINADJUST);

    ret = DialogBoxParamW (COMCTL32_hModule, MAKEINTRESOURCEW(IDD_TBCUSTOMIZE),
                           infoPtr->hwndSelf, TOOLBAR_CustomizeDialogProc, (LPARAM)&custInfo);

    /* send TBN_ENDADJUST notification */
    TOOLBAR_SendNotify (&nmhdr, infoPtr, TBN_ENDADJUST);

    return ret;
}


static LRESULT
TOOLBAR_DeleteButton (TOOLBAR_INFO *infoPtr, INT nIndex)
{
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

    infoPtr->nHotItem = -1;
    if (infoPtr->nNumButtons == 1) {
	TRACE(" simple delete\n");
        free_string( infoPtr->buttons );
	Free (infoPtr->buttons);
	infoPtr->buttons = NULL;
	infoPtr->nNumButtons = 0;
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
        TRACE("complex delete [nIndex=%d]\n", nIndex);

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

        free_string( oldButtons + nIndex );
	Free (oldButtons);
    }

    TOOLBAR_LayoutToolbar(infoPtr);

    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_EnableButton (TOOLBAR_INFO *infoPtr, INT Id, LPARAM lParam)
{
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    DWORD bState;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);

    TRACE("hwnd=%p, btn id=%d, lParam=0x%08lx\n", infoPtr->hwndSelf, Id, lParam);

    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    bState = btnPtr->fsState & TBSTATE_ENABLED;

    /* update the toolbar button state */
    if(!LOWORD(lParam)) {
 	btnPtr->fsState &= ~(TBSTATE_ENABLED | TBSTATE_PRESSED);
    } else {
	btnPtr->fsState |= TBSTATE_ENABLED;
    }

    /* redraw the button only if the state of the button changed */
    if(bState != (btnPtr->fsState & TBSTATE_ENABLED))
        InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);

    return TRUE;
}


static inline LRESULT
TOOLBAR_GetAnchorHighlight (const TOOLBAR_INFO *infoPtr)
{
    return infoPtr->bAnchor;
}


static LRESULT
TOOLBAR_GetBitmap (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
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
TOOLBAR_GetButton (const TOOLBAR_INFO *infoPtr, INT nIndex, TBBUTTON *lpTbb)
{
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
TOOLBAR_GetButtonInfoT(const TOOLBAR_INFO *infoPtr, INT Id, LPTBBUTTONINFOW lpTbInfo, BOOL bUnicode)
{
    /* TBBUTTONINFOW and TBBUTTONINFOA have the same layout*/
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    if (lpTbInfo == NULL)
	return -1;

    /* MSDN documents an iImageLabel field added in Vista but it is not present in
     * the headers and tests shows that even with comctl 6 Vista accepts only the
     * original TBBUTTONINFO size
     */
    if (lpTbInfo->cbSize != sizeof(TBBUTTONINFOW))
    {
        WARN("Invalid button size\n");
	return -1;
    }

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, lpTbInfo->dwMask & TBIF_BYINDEX);
    if (nIndex == -1)
	return -1;

    btnPtr = &infoPtr->buttons[nIndex];
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
        if (!IS_INTRESOURCE(btnPtr->iString) && (btnPtr->iString != -1)) {
            LPWSTR lpText = (LPWSTR)btnPtr->iString;
            if (bUnicode)
                Str_GetPtrW(lpText, lpTbInfo->pszText, lpTbInfo->cchText);
            else
                Str_GetPtrWtoA(lpText, (LPSTR)lpTbInfo->pszText, lpTbInfo->cchText);
        } else if (!bUnicode || lpTbInfo->pszText)
            lpTbInfo->pszText[0] = '\0';
    }
    return nIndex;
}


static inline LRESULT
TOOLBAR_GetButtonSize (const TOOLBAR_INFO *infoPtr)
{
    return MAKELONG((WORD)infoPtr->nButtonWidth,
                    (WORD)infoPtr->nButtonHeight);
}


static LRESULT
TOOLBAR_GetButtonText (const TOOLBAR_INFO *infoPtr, INT Id, LPWSTR lpStr, BOOL isW)
{
    INT nIndex;
    LPWSTR lpText;
    LRESULT ret = 0;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    lpText = TOOLBAR_GetText(infoPtr,&infoPtr->buttons[nIndex]);

    if (isW)
    {
        if (lpText)
        {
            ret = lstrlenW (lpText);
            if (lpStr) lstrcpyW (lpStr, lpText);
        }
    }
    else
        ret = WideCharToMultiByte( CP_ACP, 0, lpText, -1,
                                  (LPSTR)lpStr, lpStr ? 0x7fffffff : 0, NULL, NULL ) - 1;
    return ret;
}


static LRESULT
TOOLBAR_GetDisabledImageList (const TOOLBAR_INFO *infoPtr, WPARAM wParam)
{
    TRACE("hwnd=%p, wParam=%ld\n", infoPtr->hwndSelf, wParam);
    /* UNDOCUMENTED: wParam is actually the ID of the image list to return */
    return (LRESULT)GETDISIMAGELIST(infoPtr, wParam);
}


static inline LRESULT
TOOLBAR_GetExtendedStyle (const TOOLBAR_INFO *infoPtr)
{
    TRACE("\n");

    return infoPtr->dwExStyle;
}


static LRESULT
TOOLBAR_GetHotImageList (const TOOLBAR_INFO *infoPtr, WPARAM wParam)
{
    TRACE("hwnd=%p, wParam=%ld\n", infoPtr->hwndSelf, wParam);
    /* UNDOCUMENTED: wParam is actually the ID of the image list to return */
    return (LRESULT)GETHOTIMAGELIST(infoPtr, wParam);
}


static LRESULT
TOOLBAR_GetHotItem (const TOOLBAR_INFO *infoPtr)
{
    if (!((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)))
	return -1;

    if (infoPtr->nHotItem < 0)
	return -1;

    return (LRESULT)infoPtr->nHotItem;
}


static LRESULT
TOOLBAR_GetDefImageList (const TOOLBAR_INFO *infoPtr, WPARAM wParam)
{
    TRACE("hwnd=%p, wParam=%ld\n", infoPtr->hwndSelf, wParam);
    /* UNDOCUMENTED: wParam is actually the ID of the image list to return */
    return (LRESULT) GETDEFIMAGELIST(infoPtr, wParam);
}


static LRESULT
TOOLBAR_GetInsertMark (const TOOLBAR_INFO *infoPtr, TBINSERTMARK *lptbim)
{
    TRACE("hwnd = %p, lptbim = %p\n", infoPtr->hwndSelf, lptbim);

    *lptbim = infoPtr->tbim;

    return 0;
}


static inline LRESULT
TOOLBAR_GetInsertMarkColor (const TOOLBAR_INFO *infoPtr)
{
    TRACE("hwnd = %p\n", infoPtr->hwndSelf);

    return (LRESULT)infoPtr->clrInsertMark;
}


static LRESULT
TOOLBAR_GetItemRect (const TOOLBAR_INFO *infoPtr, INT nIndex, LPRECT lpRect)
{
    TBUTTON_INFO *btnPtr;

    btnPtr = &infoPtr->buttons[nIndex];
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

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
TOOLBAR_GetMaxSize (const TOOLBAR_INFO *infoPtr, LPSIZE lpSize)
{
    if (lpSize == NULL)
	return FALSE;

    lpSize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
    lpSize->cy = infoPtr->rcBound.bottom - infoPtr->rcBound.top;

    TRACE("maximum size %d x %d\n",
	   infoPtr->rcBound.right - infoPtr->rcBound.left,
	   infoPtr->rcBound.bottom - infoPtr->rcBound.top);

    return TRUE;
}

#ifdef __REACTOS__
static LRESULT
TOOLBAR_GetMetrics(const TOOLBAR_INFO *infoPtr, TBMETRICS *pMetrics)
{
    if (pMetrics == NULL || pMetrics->cbSize != sizeof(TBMETRICS))
        return 0;

    if (pMetrics->dwMask & TBMF_PAD)
    {
        pMetrics->cxPad = infoPtr->szPadding.cx;
        pMetrics->cyPad = infoPtr->szPadding.cy;
    }

    if (pMetrics->dwMask & TBMF_BARPAD)
    {
        pMetrics->cxBarPad = infoPtr->szBarPadding.cx;
        pMetrics->cyBarPad = infoPtr->szBarPadding.cy;
    }

    if (pMetrics->dwMask & TBMF_BUTTONSPACING)
    {
        pMetrics->cxButtonSpacing = infoPtr->szSpacing.cx;
        pMetrics->cyButtonSpacing = infoPtr->szSpacing.cy;
    }

    return 0;
}
#endif

/* << TOOLBAR_GetObject >> */


static inline LRESULT
TOOLBAR_GetPadding (const TOOLBAR_INFO *infoPtr)
{
    return MAKELONG(infoPtr->szPadding.cx, infoPtr->szPadding.cy);
}


static LRESULT
TOOLBAR_GetRect (const TOOLBAR_INFO *infoPtr, INT Id, LPRECT lpRect)
{
    TBUTTON_INFO *btnPtr;
    INT        nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    btnPtr = &infoPtr->buttons[nIndex];
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    if (lpRect == NULL)
	return FALSE;

    lpRect->left   = btnPtr->rect.left;
    lpRect->right  = btnPtr->rect.right;
    lpRect->bottom = btnPtr->rect.bottom;
    lpRect->top    = btnPtr->rect.top;

    return TRUE;
}


static inline LRESULT
TOOLBAR_GetRows (const TOOLBAR_INFO *infoPtr)
{
    return infoPtr->nRows;
}


static LRESULT
TOOLBAR_GetState (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    return infoPtr->buttons[nIndex].fsState;
}


static inline LRESULT
TOOLBAR_GetStyle (const TOOLBAR_INFO *infoPtr)
{
    return infoPtr->dwStyle;
}


static inline LRESULT
TOOLBAR_GetTextRows (const TOOLBAR_INFO *infoPtr)
{
    return infoPtr->nMaxTextRows;
}


static LRESULT
TOOLBAR_GetToolTips (TOOLBAR_INFO *infoPtr)
{
    if ((infoPtr->dwStyle & TBSTYLE_TOOLTIPS) && (infoPtr->hwndToolTip == NULL))
        TOOLBAR_TooltipCreateControl(infoPtr);
    return (LRESULT)infoPtr->hwndToolTip;
}


static LRESULT
TOOLBAR_GetUnicodeFormat (const TOOLBAR_INFO *infoPtr)
{
    TRACE("%s hwnd=%p\n",
	   infoPtr->bUnicode ? "TRUE" : "FALSE", infoPtr->hwndSelf);

    return infoPtr->bUnicode;
}


static inline LRESULT
TOOLBAR_GetVersion (const TOOLBAR_INFO *infoPtr)
{
    return infoPtr->iVersion;
}


static LRESULT
TOOLBAR_HideButton (TOOLBAR_INFO *infoPtr, INT Id, BOOL fHide)
{
    TBUTTON_INFO *btnPtr;
    BYTE oldState;
    INT nIndex;

    TRACE("\n");

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    oldState = btnPtr->fsState;

    if (fHide)
	btnPtr->fsState |= TBSTATE_HIDDEN;
    else
	btnPtr->fsState &= ~TBSTATE_HIDDEN;

    if (oldState != btnPtr->fsState) {
        TOOLBAR_LayoutToolbar (infoPtr);
        InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);
    }

    return TRUE;
}


static inline LRESULT
TOOLBAR_HitTest (const TOOLBAR_INFO *infoPtr, const POINT* lpPt)
{
    return TOOLBAR_InternalHitTest (infoPtr, lpPt, NULL);
}


static LRESULT
TOOLBAR_Indeterminate (const TOOLBAR_INFO *infoPtr, INT Id, BOOL fIndeterminate)
{
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    DWORD oldState;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    oldState = btnPtr->fsState;

    if (fIndeterminate)
	btnPtr->fsState |= TBSTATE_INDETERMINATE;
    else
	btnPtr->fsState &= ~TBSTATE_INDETERMINATE;

    if(oldState != btnPtr->fsState)
        InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_InsertButtonT(TOOLBAR_INFO *infoPtr, INT nIndex, const TBBUTTON *lpTbb, BOOL fUnicode)
{
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
TOOLBAR_IsButtonChecked (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_CHECKED);
}


static LRESULT
TOOLBAR_IsButtonEnabled (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_ENABLED);
}


static LRESULT
TOOLBAR_IsButtonHidden (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_HIDDEN);
}


static LRESULT
TOOLBAR_IsButtonHighlighted (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_MARKED);
}


static LRESULT
TOOLBAR_IsButtonIndeterminate (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_INDETERMINATE);
}


static LRESULT
TOOLBAR_IsButtonPressed (const TOOLBAR_INFO *infoPtr, INT Id)
{
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return -1;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_PRESSED);
}


static LRESULT
TOOLBAR_LoadImages (TOOLBAR_INFO *infoPtr, WPARAM wParam, HINSTANCE hInstance)
{
    TBADDBITMAP tbab;
    tbab.hInst = hInstance;
    tbab.nID = wParam;

    TRACE("hwnd = %p, hInst = %p, nID = %lu\n", infoPtr->hwndSelf, tbab.hInst, tbab.nID);

    return TOOLBAR_AddBitmap(infoPtr, 0, &tbab);
}


static LRESULT
TOOLBAR_MapAccelerator (const TOOLBAR_INFO *infoPtr, WCHAR wAccel, UINT *pIDButton)
{
    WCHAR wszAccel[] = {'&',wAccel,0};
    int i;
    
    TRACE("hwnd = %p, wAccel = %x(%s), pIDButton = %p\n",
        infoPtr->hwndSelf, wAccel, debugstr_wn(&wAccel,1), pIDButton);
    
    for (i = 0; i < infoPtr->nNumButtons; i++)
    {
        TBUTTON_INFO *btnPtr = infoPtr->buttons+i;
        if (!(btnPtr->fsStyle & BTNS_NOPREFIX) &&
            !(btnPtr->fsState & TBSTATE_HIDDEN))
        {
            int iLen = lstrlenW(wszAccel);
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
                if (!wcsnicmp(lpszStr, wszAccel, iLen))
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
TOOLBAR_MarkButton (const TOOLBAR_INFO *infoPtr, INT Id, BOOL fMark)
{
    INT nIndex;
    DWORD oldState;
    TBUTTON_INFO *btnPtr;

    TRACE("hwnd = %p, Id = %d, fMark = 0%d\n", infoPtr->hwndSelf, Id, fMark);

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
        return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    oldState = btnPtr->fsState;

    if (fMark)
        btnPtr->fsState |= TBSTATE_MARKED;
    else
        btnPtr->fsState &= ~TBSTATE_MARKED;

    if(oldState != btnPtr->fsState)
        InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);

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
TOOLBAR_MoveButton (TOOLBAR_INFO *infoPtr, INT Id, INT nMoveIndex)
{
    INT nIndex;
    INT nCount;
    TBUTTON_INFO button;

    TRACE("hwnd=%p, Id=%d, nMoveIndex=%d\n", infoPtr->hwndSelf, Id, nMoveIndex);

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, TRUE);
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

    TOOLBAR_LayoutToolbar(infoPtr);
    TOOLBAR_AutoSize(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_PressButton (const TOOLBAR_INFO *infoPtr, INT Id, BOOL fPress)
{
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    DWORD oldState;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    oldState = btnPtr->fsState;

    if (fPress)
	btnPtr->fsState |= TBSTATE_PRESSED;
    else
	btnPtr->fsState &= ~TBSTATE_PRESSED;

    if(oldState != btnPtr->fsState)
        InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);

    return TRUE;
}

/* FIXME: there might still be some confusion her between number of buttons
 * and number of bitmaps */
static LRESULT
TOOLBAR_ReplaceBitmap (TOOLBAR_INFO *infoPtr, const TBREPLACEBITMAP *lpReplace)
{
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
    else if (lpReplace->hInstOld != 0 && lpReplace->hInstOld != lpReplace->hInstNew)
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

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return TRUE;
}


/* helper for TOOLBAR_SaveRestoreW */
static BOOL
TOOLBAR_Save(TOOLBAR_INFO *infoPtr, const TBSAVEPARAMSW *params)
{
    NMTBSAVE save;
    INT ret, i;
    BOOL alloced = FALSE;
    HKEY key;

    TRACE( "save to %s %s\n", debugstr_w(params->pszSubKey), debugstr_w(params->pszValueName) );

    memset( &save, 0, sizeof(save) );
    save.cbData = infoPtr->nNumButtons * sizeof(DWORD);
    save.iItem = -1;
    save.cButtons = infoPtr->nNumButtons;
    save.tbButton.idCommand = -1;
    TOOLBAR_SendNotify( &save.hdr, infoPtr, TBN_SAVE );

    if (!save.pData)
    {
        save.pData = Alloc( save.cbData );
        if (!save.pData) return FALSE;
        alloced = TRUE;
    }
    if (!save.pCurrent) save.pCurrent = save.pData;

    for (i = 0; i < infoPtr->nNumButtons; i++)
    {
        save.iItem = i;
        save.tbButton.iBitmap = infoPtr->buttons[i].iBitmap;
        save.tbButton.idCommand = infoPtr->buttons[i].idCommand;
        save.tbButton.fsState = infoPtr->buttons[i].fsState;
        save.tbButton.fsStyle = infoPtr->buttons[i].fsStyle;
        memset( save.tbButton.bReserved, 0, sizeof(save.tbButton.bReserved) );
        save.tbButton.dwData = infoPtr->buttons[i].dwData;
        save.tbButton.iString = infoPtr->buttons[i].iString;

        *save.pCurrent++ = save.tbButton.idCommand;

        TOOLBAR_SendNotify( &save.hdr, infoPtr, TBN_SAVE );
    }

    ret = RegCreateKeyW( params->hkr, params->pszSubKey, &key );
    if (ret == ERROR_SUCCESS)
    {
        ret = RegSetValueExW( key, params->pszValueName, 0, REG_BINARY, (BYTE *)save.pData, save.cbData );
        RegCloseKey( key );
    }

    if (alloced) Free( save.pData );
    return !ret;
}


/* helper for TOOLBAR_Restore */
static void
TOOLBAR_DeleteAllButtons(TOOLBAR_INFO *infoPtr)
{
    INT i;

    for (i = 0; i < infoPtr->nNumButtons; i++)
    {
        free_string( infoPtr->buttons + i );
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
    NMHDR hdr;

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
            INT i, count = nmtbr.cButtons;

            /* remove all existing buttons as this function is designed to
             * restore the toolbar to a previously saved state */
            TOOLBAR_DeleteAllButtons(infoPtr);

            for (i = 0; i < count; i++)
            {
                nmtbr.iItem = i;
                nmtbr.tbButton.iBitmap = -1;
                nmtbr.tbButton.fsState = 0;
                nmtbr.tbButton.fsStyle = 0;
                nmtbr.tbButton.dwData = 0;
                nmtbr.tbButton.iString = 0;

                if (*nmtbr.pCurrent & 0x80000000)
                {
                    /* separator */
                    nmtbr.tbButton.iBitmap = SEPARATOR_WIDTH;
                    nmtbr.tbButton.idCommand = 0;
                    nmtbr.tbButton.fsStyle = BTNS_SEP;
                    if (*nmtbr.pCurrent != (DWORD)-1)
                        nmtbr.tbButton.fsState = TBSTATE_HIDDEN;
                }
                else
                    nmtbr.tbButton.idCommand = (int)*nmtbr.pCurrent;

                nmtbr.pCurrent++;
                
                TOOLBAR_SendNotify(&nmtbr.hdr, infoPtr, TBN_RESTORE);

                /* All returned ptrs and -1 are ignored */
                if (!IS_INTRESOURCE(nmtbr.tbButton.iString))
                    nmtbr.tbButton.iString = 0;

                TOOLBAR_InsertButtonT(infoPtr, -1, &nmtbr.tbButton, TRUE);
            }

            TOOLBAR_SendNotify( &hdr, infoPtr, TBN_BEGINADJUST );
            for (i = 0; ; i++)
            {
                NMTOOLBARW tb;
                TBBUTTONINFOW bi;
                WCHAR buf[128];
                UINT code = infoPtr->bUnicode ? TBN_GETBUTTONINFOW : TBN_GETBUTTONINFOA;
                INT idx;

                memset( &tb, 0, sizeof(tb) );
                tb.iItem = i;
                tb.cchText = ARRAY_SIZE(buf);
                tb.pszText = buf;

                /* Use the same struct for both A and W versions since the layout is the same. */
                if (!TOOLBAR_SendNotify( &tb.hdr, infoPtr, code ))
                    break;

                idx = TOOLBAR_GetButtonIndex( infoPtr, tb.tbButton.idCommand, FALSE );
                if (idx == -1) continue;

                /* tb.pszText is ignored - the string comes from tb.tbButton.iString, which may
                   be an index or a ptr.  Either way it is simply copied.  There is no api to change
                   the string index, so we set it manually.  The other properties can be set with SetButtonInfo. */
                free_string( infoPtr->buttons + idx );
                infoPtr->buttons[idx].iString = tb.tbButton.iString;

                memset( &bi, 0, sizeof(bi) );
                bi.cbSize = sizeof(bi);
                bi.dwMask = TBIF_IMAGE | TBIF_STATE | TBIF_STYLE | TBIF_LPARAM;
                bi.iImage = tb.tbButton.iBitmap;
                bi.fsState = tb.tbButton.fsState;
                bi.fsStyle = tb.tbButton.fsStyle;
                bi.lParam = tb.tbButton.dwData;

                TOOLBAR_SetButtonInfo( infoPtr, tb.tbButton.idCommand, &bi, TRUE );
            }
            TOOLBAR_SendNotify( &hdr, infoPtr, TBN_ENDADJUST );

            /* remove all uninitialised buttons
             * note: loop backwards to avoid having to fixup i on a
             * delete */
            for (i = infoPtr->nNumButtons - 1; i >= 0; i--)
                if (infoPtr->buttons[i].iBitmap == -1)
                    TOOLBAR_DeleteButton(infoPtr, i);

            /* only indicate success if at least one button survived */
            if (infoPtr->nNumButtons > 0) ret = TRUE;
        }
    }
    Free (nmtbr.pData);
    RegCloseKey(hkey);

    return ret;
}


static LRESULT
TOOLBAR_SaveRestoreW (TOOLBAR_INFO *infoPtr, WPARAM wParam, const TBSAVEPARAMSW *lpSave)
{
    if (lpSave == NULL) return 0;

    if (wParam)
        return TOOLBAR_Save(infoPtr, lpSave);
    else
        return TOOLBAR_Restore(infoPtr, lpSave);
}


static LRESULT
TOOLBAR_SaveRestoreA (TOOLBAR_INFO *infoPtr, WPARAM wParam, const TBSAVEPARAMSA *lpSave)
{
    LPWSTR pszValueName = 0, pszSubKey = 0;
    TBSAVEPARAMSW SaveW;
    LRESULT result = 0;
    int len;

    if (lpSave == NULL) return 0;

    len = MultiByteToWideChar(CP_ACP, 0, lpSave->pszSubKey, -1, NULL, 0);
    pszSubKey = Alloc(len * sizeof(WCHAR));
    if (!pszSubKey) goto exit;
    MultiByteToWideChar(CP_ACP, 0, lpSave->pszSubKey, -1, pszSubKey, len);

    len = MultiByteToWideChar(CP_ACP, 0, lpSave->pszValueName, -1, NULL, 0);
    pszValueName = Alloc(len * sizeof(WCHAR));
    if (!pszValueName) goto exit;
    MultiByteToWideChar(CP_ACP, 0, lpSave->pszValueName, -1, pszValueName, len);

    SaveW.pszValueName = pszValueName;
    SaveW.pszSubKey = pszSubKey;
    SaveW.hkr = lpSave->hkr;
    result = TOOLBAR_SaveRestoreW(infoPtr, wParam, &SaveW);

exit:
    Free (pszValueName);
    Free (pszSubKey);

    return result;
}


static LRESULT
TOOLBAR_SetAnchorHighlight (TOOLBAR_INFO *infoPtr, BOOL bAnchor)
{
    BOOL bOldAnchor = infoPtr->bAnchor;

    TRACE("hwnd=%p, bAnchor = %s\n", infoPtr->hwndSelf, bAnchor ? "TRUE" : "FALSE");

    infoPtr->bAnchor = bAnchor;

    /* Native does not remove the hot effect from an already hot button */

    return (LRESULT)bOldAnchor;
}


static LRESULT
TOOLBAR_SetBitmapSize (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HIMAGELIST himlDef = GETDEFIMAGELIST(infoPtr, 0);
    short width = (short)LOWORD(lParam);
    short height = (short)HIWORD(lParam);

    TRACE("hwnd=%p, wParam=%ld, size %d x %d\n", infoPtr->hwndSelf, wParam, width, height);

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

    TOOLBAR_CalcToolbar(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonInfo (TOOLBAR_INFO *infoPtr, INT Id,
                       const TBBUTTONINFOW *lptbbi, BOOL isW)
{
    TBUTTON_INFO *btnPtr;
    INT nIndex;
    RECT oldBtnRect;

    if (lptbbi == NULL)
	return FALSE;
    if (lptbbi->cbSize < sizeof(TBBUTTONINFOW))
	return FALSE;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, lptbbi->dwMask & TBIF_BYINDEX);
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

    if (lptbbi->dwMask & TBIF_TEXT)
        set_stringT( btnPtr, lptbbi->pszText, isW );

    /* save the button rect to see if we need to redraw the whole toolbar */
    oldBtnRect = btnPtr->rect;
    TOOLBAR_LayoutToolbar(infoPtr);

    if (!EqualRect(&oldBtnRect, &btnPtr->rect))
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    else
        InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonSize (TOOLBAR_INFO *infoPtr, LPARAM lParam)
{
    INT cx = (short)LOWORD(lParam), cy = (short)HIWORD(lParam);
    int top = default_top_margin(infoPtr);

    if ((cx < 0) || (cy < 0))
    {
        ERR("invalid parameter 0x%08x\n", (DWORD)lParam);
        return FALSE;
    }

    TRACE("%p, cx = %d, cy = %d\n", infoPtr->hwndSelf, cx, cy);

    /* The documentation claims you can only change the button size before
     * any button has been added. But this is wrong.
     * WINZIP32.EXE (ver 8) calls this on one of its buttons after adding
     * it to the toolbar, and it checks that the return value is nonzero - mjm
     * Further testing shows that we must actually perform the change too.
     */
    /*
     * The documentation also does not mention that if 0 is supplied for
     * either size, the system changes it to the default of 24 wide and
     * 22 high. Demonstrated in ControlSpy Toolbar. GLA 3/02
     */
    if (cx == 0) cx = 24;
    if (cy == 0) cy = 22;

#ifdef __REACTOS__
    cx = max(cx, infoPtr->szPadding.cx + infoPtr->nBitmapWidth + infoPtr->themeMargins.cxLeftWidth + infoPtr->themeMargins.cxRightWidth);
    cy = max(cy, infoPtr->szPadding.cy + infoPtr->nBitmapHeight + infoPtr->themeMargins.cyTopHeight + infoPtr->themeMargins.cyBottomHeight);
#else
    cx = max(cx, infoPtr->szPadding.cx + infoPtr->nBitmapWidth);
    cy = max(cy, infoPtr->szPadding.cy + infoPtr->nBitmapHeight);
#endif

    if (cx != infoPtr->nButtonWidth || cy != infoPtr->nButtonHeight ||
        top != infoPtr->iTopMargin)
    {
        infoPtr->nButtonWidth = cx;
        infoPtr->nButtonHeight = cy;
        infoPtr->iTopMargin = top;

        TOOLBAR_LayoutToolbar( infoPtr );
        InvalidateRect( infoPtr->hwndSelf, NULL, TRUE );
    }
    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonWidth (TOOLBAR_INFO *infoPtr, LPARAM lParam)
{
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

    TOOLBAR_CalcToolbar (infoPtr);

    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);

    return TRUE;
}


static LRESULT
TOOLBAR_SetCmdId (TOOLBAR_INFO *infoPtr, INT nIndex, INT nId)
{
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    infoPtr->buttons[nIndex].idCommand = nId;

    if (infoPtr->hwndToolTip) {

	FIXME("change tool tip\n");

    }

    return TRUE;
}


static LRESULT
TOOLBAR_SetDisabledImageList (TOOLBAR_INFO *infoPtr, WPARAM wParam, HIMAGELIST himl)
{
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
TOOLBAR_SetDrawTextFlags (TOOLBAR_INFO *infoPtr, DWORD mask, DWORD flags)
{
    DWORD old_flags;

    TRACE("hwnd = %p, mask = 0x%08x, flags = 0x%08x\n", infoPtr->hwndSelf, mask, flags);

    old_flags = infoPtr->dwDTFlags;
    infoPtr->dwDTFlags = (old_flags & ~mask) | (flags & mask);

    return (LRESULT)old_flags;
}

/* This function differs a bit from what MSDN says it does:
 * 1. lParam contains extended style flags to OR with current style
 *  (MSDN isn't clear on the OR bit)
 * 2. wParam appears to contain extended style flags to be reset
 *  (MSDN says that this parameter is reserved)
 */
static LRESULT
TOOLBAR_SetExtendedStyle (TOOLBAR_INFO *infoPtr, DWORD mask, DWORD style)
{
    DWORD old_style = infoPtr->dwExStyle;

    TRACE("mask=0x%08x, style=0x%08x\n", mask, style);

    if (mask)
	infoPtr->dwExStyle = (old_style & ~mask) | (style & mask);
    else
	infoPtr->dwExStyle = style;

    if (infoPtr->dwExStyle & ~TBSTYLE_EX_ALL)
	FIXME("Unknown Toolbar Extended Style 0x%08x. Please report.\n",
	      (infoPtr->dwExStyle & ~TBSTYLE_EX_ALL));

    if ((old_style ^ infoPtr->dwExStyle) & TBSTYLE_EX_MIXEDBUTTONS)
        TOOLBAR_CalcToolbar(infoPtr);
    else
        TOOLBAR_LayoutToolbar(infoPtr);

    TOOLBAR_AutoSize(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return old_style;
}


static LRESULT
TOOLBAR_SetHotImageList (TOOLBAR_INFO *infoPtr, WPARAM wParam, HIMAGELIST himl)
{
    HIMAGELIST himlTemp;
    INT id = 0;

    if (infoPtr->iVersion >= 5)
        id = wParam;

    TRACE("hwnd = %p, himl = %p, id = %d\n", infoPtr->hwndSelf, himl, id);

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
TOOLBAR_SetHotItem (TOOLBAR_INFO *infoPtr, INT nHotItem)
{
    INT nOldHotItem = infoPtr->nHotItem;

    TRACE("hwnd = %p, nHotItem = %d\n", infoPtr->hwndSelf, nHotItem);

    if (nHotItem >= infoPtr->nNumButtons)
        return infoPtr->nHotItem;
    
    if (nHotItem < 0)
        nHotItem = -1;

    /* NOTE: an application can still remove the hot item even if anchor
     * highlighting is enabled */

    TOOLBAR_SetHotItemEx(infoPtr, nHotItem, HICF_OTHER);

    if (nOldHotItem < 0)
        return -1;

    return (LRESULT)nOldHotItem;
}


static LRESULT
TOOLBAR_SetImageList (TOOLBAR_INFO *infoPtr, WPARAM wParam, HIMAGELIST himl)
{
    HIMAGELIST himlTemp;
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
        TOOLBAR_CalcToolbar(infoPtr);
        if (infoPtr->nButtonWidth < oldButtonWidth)
            TOOLBAR_SetButtonSize(infoPtr, MAKELONG(oldButtonWidth, infoPtr->nButtonHeight));
    }

    TRACE("hwnd %p, new himl=%p, id = %d, count=%d, bitmap w=%d, h=%d\n",
	  infoPtr->hwndSelf, infoPtr->himlDef, id, infoPtr->nNumBitmaps,
	  infoPtr->nBitmapWidth, infoPtr->nBitmapHeight);

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return (LRESULT)himlTemp;
}


static LRESULT
TOOLBAR_SetIndent (TOOLBAR_INFO *infoPtr, INT nIndent)
{
    infoPtr->nIndent = nIndent;

    TRACE("\n");

    /* process only on indent changing */
    if(infoPtr->nIndent != nIndent)
    {
        infoPtr->nIndent = nIndent;
        TOOLBAR_CalcToolbar (infoPtr);
        InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    }

    return TRUE;
}


static LRESULT
TOOLBAR_SetInsertMark (TOOLBAR_INFO *infoPtr, const TBINSERTMARK *lptbim)
{
    TRACE("hwnd = %p, lptbim = { %d, 0x%08x}\n", infoPtr->hwndSelf, lptbim->iButton, lptbim->dwFlags);

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
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    }
    else
        ERR("Invalid button index %d\n", lptbim->iButton);

    return 0;
}


static LRESULT
TOOLBAR_SetInsertMarkColor (TOOLBAR_INFO *infoPtr, COLORREF clr)
{
    infoPtr->clrInsertMark = clr;

    /* FIXME: don't need to update entire toolbar */
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
TOOLBAR_SetMaxTextRows (TOOLBAR_INFO *infoPtr, INT nMaxRows)
{
    infoPtr->nMaxTextRows = nMaxRows;

    TOOLBAR_CalcToolbar(infoPtr);
    return TRUE;
}

#ifdef __REACTOS__
static LRESULT
TOOLBAR_SetMetrics(TOOLBAR_INFO *infoPtr, TBMETRICS *pMetrics)
{
    BOOL changed = FALSE;

    if (!pMetrics)
        return FALSE;

    /* TODO: check if cbSize is a valid value */

    if (pMetrics->dwMask & TBMF_PAD)
    {
        infoPtr->szPadding.cx = pMetrics->cxPad;
        infoPtr->szPadding.cy = pMetrics->cyPad;
        changed = TRUE;
    }

    if (pMetrics->dwMask & TBMF_PAD)
    {
        infoPtr->szBarPadding.cx = pMetrics->cxBarPad;
        infoPtr->szBarPadding.cy = pMetrics->cyBarPad;
        changed = TRUE;
    }

    if (pMetrics->dwMask & TBMF_BUTTONSPACING)
    {
        infoPtr->szSpacing.cx = pMetrics->cxButtonSpacing;
        infoPtr->szSpacing.cy = pMetrics->cyButtonSpacing;
        changed = TRUE;
    }

    if (changed)
        TOOLBAR_CalcToolbar(infoPtr);

    return TRUE;
}
#endif

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
TOOLBAR_SetPadding (TOOLBAR_INFO *infoPtr, LPARAM lParam)
{
    DWORD  oldPad;

    oldPad = MAKELONG(infoPtr->szPadding.cx, infoPtr->szPadding.cy);
    infoPtr->szPadding.cx = min(LOWORD((DWORD)lParam), GetSystemMetrics(SM_CXEDGE));
    infoPtr->szPadding.cy = min(HIWORD((DWORD)lParam), GetSystemMetrics(SM_CYEDGE));
    TRACE("cx=%d, cy=%d\n",
	  infoPtr->szPadding.cx, infoPtr->szPadding.cy);
    return (LRESULT) oldPad;
}


static LRESULT
TOOLBAR_SetParent (TOOLBAR_INFO *infoPtr, HWND hParent)
{
    HWND hwndOldNotify;

    TRACE("\n");

    hwndOldNotify = infoPtr->hwndNotify;
    infoPtr->hwndNotify = hParent;

    return (LRESULT)hwndOldNotify;
}


static LRESULT
TOOLBAR_SetRows (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPRECT lprc)
{
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
        idealWrap = (infoPtr->nNumButtons - hidden + (rows-1)) / (rows ? rows : 1);

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
        TOOLBAR_CalcToolbar (infoPtr);

        /* Resize if necessary (Only if NORESIZE is set - odd, but basically
           if NORESIZE is NOT set, then the toolbar will always be resized to
           take up the whole window. With it set, sizing needs to be manual. */
        if (infoPtr->dwStyle & CCS_NORESIZE) {
            SetWindowPos(infoPtr->hwndSelf, NULL, 0, 0,
                         infoPtr->rcBound.right - infoPtr->rcBound.left,
                         infoPtr->rcBound.bottom - infoPtr->rcBound.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        /* repaint toolbar */
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
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
TOOLBAR_SetState (TOOLBAR_INFO *infoPtr, INT Id, LPARAM lParam)
{
    TBUTTON_INFO *btnPtr;
    INT nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, Id, FALSE);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    /* if hidden state has changed the invalidate entire window and recalc */
    if ((btnPtr->fsState & TBSTATE_HIDDEN) != (LOWORD(lParam) & TBSTATE_HIDDEN)) {
	btnPtr->fsState = LOWORD(lParam);
	TOOLBAR_CalcToolbar (infoPtr);
	InvalidateRect(infoPtr->hwndSelf, 0, TRUE);
	return TRUE;
    }

    /* process state changing if current state doesn't match new state */
    if(btnPtr->fsState != LOWORD(lParam))
    {
        btnPtr->fsState = LOWORD(lParam);
        InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
    }

    return TRUE;
}

static inline void unwrap(TOOLBAR_INFO *info)
{
    int i;

    for (i = 0; i < info->nNumButtons; i++)
	info->buttons[i].fsState &= ~TBSTATE_WRAP;
}

static LRESULT
TOOLBAR_SetStyle (TOOLBAR_INFO *infoPtr, DWORD style)
{
    DWORD dwOldStyle = infoPtr->dwStyle;

    TRACE("new style 0x%08x\n", style);

    if (style & TBSTYLE_LIST)
        infoPtr->dwDTFlags = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
    else
        infoPtr->dwDTFlags = DT_CENTER | DT_END_ELLIPSIS;

    infoPtr->dwStyle = style;
    TOOLBAR_CheckStyle(infoPtr);

    if ((dwOldStyle ^ style) & TBSTYLE_WRAPABLE)
    {
        if (dwOldStyle & TBSTYLE_WRAPABLE)
            unwrap(infoPtr);
        TOOLBAR_CalcToolbar(infoPtr);
    }
    else if ((dwOldStyle ^ style) & CCS_VERT)
        TOOLBAR_LayoutToolbar(infoPtr);

    /* only resize if one of the CCS_* styles was changed */
    if ((dwOldStyle ^ style) & COMMON_STYLES)
    {
        TOOLBAR_AutoSize(infoPtr);
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    }

    return 0;
}


static inline LRESULT
TOOLBAR_SetToolTips (TOOLBAR_INFO *infoPtr, HWND hwndTooltip)
{
    TRACE("hwnd=%p, hwndTooltip=%p\n", infoPtr->hwndSelf, hwndTooltip);

    infoPtr->hwndToolTip = hwndTooltip;
    return 0;
}


static LRESULT
TOOLBAR_SetUnicodeFormat (TOOLBAR_INFO *infoPtr, WPARAM wParam)
{
    BOOL bTemp;

    TRACE("%s hwnd=%p\n",
	   ((BOOL)wParam) ? "TRUE" : "FALSE", infoPtr->hwndSelf);

    bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL)wParam;

    return bTemp;
}


static LRESULT
TOOLBAR_GetColorScheme (const TOOLBAR_INFO *infoPtr, LPCOLORSCHEME lParam)
{
    lParam->clrBtnHighlight = (infoPtr->clrBtnHighlight == CLR_DEFAULT) ?
	                       comctl32_color.clrBtnHighlight :
                               infoPtr->clrBtnHighlight;
    lParam->clrBtnShadow = (infoPtr->clrBtnShadow == CLR_DEFAULT) ?
	                   comctl32_color.clrBtnShadow : infoPtr->clrBtnShadow;
    return 1;
}


static LRESULT
TOOLBAR_SetColorScheme (TOOLBAR_INFO *infoPtr, const COLORSCHEME *lParam)
{
    TRACE("new colors Hl=%x Shd=%x, old colors Hl=%x Shd=%x\n",
	  lParam->clrBtnHighlight, lParam->clrBtnShadow,
	  infoPtr->clrBtnHighlight, infoPtr->clrBtnShadow);

    infoPtr->clrBtnHighlight = lParam->clrBtnHighlight;
    infoPtr->clrBtnShadow = lParam->clrBtnShadow;
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 0;
}


static LRESULT
TOOLBAR_SetVersion (TOOLBAR_INFO *infoPtr, INT iVersion)
{
    INT iOldVersion = infoPtr->iVersion;

#ifdef __REACTOS__
    /* The v6 control doesn't support changing its version */
    if (iOldVersion == 6)
        return iOldVersion;

    /* And a control that is not v6 can't be set to be a v6 one */
    if (iVersion >= 6)
        return -1;
#endif

    infoPtr->iVersion = iVersion;

    if (infoPtr->iVersion >= 5)
        TOOLBAR_SetUnicodeFormat(infoPtr, TRUE);

    return iOldVersion;
}


static LRESULT
TOOLBAR_GetStringA (const TOOLBAR_INFO *infoPtr, WPARAM wParam, LPSTR str)
{
    WORD iString = HIWORD(wParam);
    WORD buffersize = LOWORD(wParam);
    LRESULT ret = -1;

    TRACE("hwnd=%p, iString=%d, buffersize=%d, string=%p\n", infoPtr->hwndSelf, iString, buffersize, str);

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
TOOLBAR_GetStringW (const TOOLBAR_INFO *infoPtr, WPARAM wParam, LPWSTR str)
{
    WORD iString = HIWORD(wParam);
    WORD len = LOWORD(wParam)/sizeof(WCHAR) - 1;
    LRESULT ret = -1;

    TRACE("hwnd=%p, iString=%d, buffersize=%d, string=%p\n", infoPtr->hwndSelf, iString, LOWORD(wParam), str);

    if (iString < infoPtr->nNumStrings)
    {
        len = min(len, lstrlenW(infoPtr->strings[iString]));
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

static LRESULT TOOLBAR_SetBoundingSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    SIZE * pSize = (SIZE*)lParam;
    FIXME("hwnd=%p, wParam=0x%08lx, size.cx=%d, size.cy=%d stub\n", hwnd, wParam, pSize->cx, pSize->cy);
    return 0;
}

/* This is an extended version of the TB_SETHOTITEM message. It allows the
 * caller to specify a reason why the hot item changed (rather than just the
 * HICF_OTHER that TB_SETHOTITEM sends). */
static LRESULT
TOOLBAR_SetHotItem2 (TOOLBAR_INFO *infoPtr, INT nHotItem, LPARAM lParam)
{
    INT nOldHotItem = infoPtr->nHotItem;

    TRACE("old item=%d, new item=%d, flags=%08x\n",
	  nOldHotItem, nHotItem, (DWORD)lParam);

    if (nHotItem < 0 || nHotItem > infoPtr->nNumButtons)
        nHotItem = -1;

    /* NOTE: an application can still remove the hot item even if anchor
     * highlighting is enabled */

    TOOLBAR_SetHotItemEx(infoPtr, nHotItem, lParam);

    return (nOldHotItem < 0) ? -1 : (LRESULT)nOldHotItem;
}

/* Sets the toolbar global iListGap parameter which controls the amount of
 * spacing between the image and the text of buttons for TBSTYLE_LIST
 * toolbars. */
static LRESULT TOOLBAR_SetListGap(TOOLBAR_INFO *infoPtr, INT iListGap)
{
    TRACE("hwnd=%p iListGap=%d\n", infoPtr->hwndSelf, iListGap);
    
    infoPtr->iListGap = iListGap;

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}

/* Returns the number of maximum number of image lists associated with the
 * various states. */
static LRESULT TOOLBAR_GetImageListCount(const TOOLBAR_INFO *infoPtr)
{
    TRACE("hwnd=%p\n", infoPtr->hwndSelf);

    return max(infoPtr->cimlDef, max(infoPtr->cimlHot, infoPtr->cimlDis));
}

static LRESULT
TOOLBAR_GetIdealSize (const TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
	    lpsize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
	}
	else if(HIWORD(lpsize->cx)) {
	    RECT rc;
	    HWND hwndParent = GetParent(infoPtr->hwndSelf);

	    GetWindowRect(infoPtr->hwndSelf, &rc);
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
TOOLBAR_Create (HWND hwnd, const CREATESTRUCTW *lpcs)
{
    TOOLBAR_INFO *infoPtr = (TOOLBAR_INFO *)GetWindowLongPtrW(hwnd, 0);
    LOGFONTW logFont;

    TRACE("hwnd = %p, style=0x%08x\n", hwnd, lpcs->style);

    infoPtr->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    GetClientRect(hwnd, &infoPtr->client_rect);
    infoPtr->bUnicode = infoPtr->hwndNotify && 
        (NFR_UNICODE == SendMessageW(hwnd, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_REQUERY));
    infoPtr->hwndToolTip = NULL; /* if needed the tooltip control will be created after a WM_MOUSEMOVE */

    SystemParametersInfoW (SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
    infoPtr->hFont = infoPtr->hDefaultFont = CreateFontIndirectW (&logFont);

#ifdef __REACTOS__
    {
        HTHEME theme = OpenThemeData (hwnd, themeClass);
        if (theme)
            GetThemeMargins(theme, NULL, TP_BUTTON, TS_NORMAL, TMT_CONTENTMARGINS, NULL, &infoPtr->themeMargins);
    }
#else
    OpenThemeData (hwnd, themeClass);
#endif

    TOOLBAR_CheckStyle (infoPtr);

    return 0;
}


static LRESULT
TOOLBAR_Destroy (TOOLBAR_INFO *infoPtr)
{
    INT i;

    /* delete tooltip control */
    if (infoPtr->hwndToolTip)
	DestroyWindow (infoPtr->hwndToolTip);

    /* delete temporary buffer for tooltip text */
    Free (infoPtr->pszTooltipText);
    Free (infoPtr->bitmaps);            /* bitmaps list */

    /* delete button data */
    for (i = 0; i < infoPtr->nNumButtons; i++)
        free_string( infoPtr->buttons + i );
    Free (infoPtr->buttons);

    /* delete strings */
    if (infoPtr->strings) {
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
        
    CloseThemeData (GetWindowTheme (infoPtr->hwndSelf));

    /* free toolbar info data */
    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);
    Free (infoPtr);

    return 0;
}


static LRESULT
TOOLBAR_EraseBackground (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    NMTBCUSTOMDRAW tbcd;
    INT ret = FALSE;
    DWORD ntfret;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
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
		      infoPtr->hwndSelf, ntfret);
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
	parent = GetParent(infoPtr->hwndSelf);
	MapWindowPoints(infoPtr->hwndSelf, parent, &pt, 1);
	OffsetWindowOrgEx (hdc, pt.x, pt.y, &ptorig);
	ret = SendMessageW (parent, WM_ERASEBKGND, wParam, lParam);
	SetWindowOrgEx (hdc, ptorig.x, ptorig.y, 0);
    }
    if (!ret)
	ret = DefWindowProcW (infoPtr->hwndSelf, WM_ERASEBKGND, wParam, lParam);

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
		      infoPtr->hwndSelf, ntfret);
	    }
    }
    return ret;
}


static inline LRESULT
TOOLBAR_GetFont (const TOOLBAR_INFO *infoPtr)
{
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
TOOLBAR_KeyDown (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    NMKEY nmkey;

    nmkey.nVKey = (UINT)wParam;
    nmkey.uFlags = HIWORD(lParam);

    if (TOOLBAR_SendNotify(&nmkey.hdr, infoPtr, NM_KEYDOWN))
        return DefWindowProcW(infoPtr->hwndSelf, WM_KEYDOWN, wParam, lParam);

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
                (LPARAM)infoPtr->hwndSelf);
        }
        break;
    }

    return 0;
}


static LRESULT
TOOLBAR_LButtonDblClk (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    BOOL button;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    TOOLBAR_InternalHitTest (infoPtr, &pt, &button);

    if (button)
        TOOLBAR_LButtonDown (infoPtr, wParam, lParam);
    else if (infoPtr->dwStyle & CCS_ADJUSTABLE)
	TOOLBAR_Customize (infoPtr);

    return 0;
}


static LRESULT
TOOLBAR_LButtonDown (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TBUTTON_INFO *btnPtr;
    POINT pt;
    INT   nHit;
    NMTOOLBARA nmtb;
    NMMOUSE nmmouse;
    BOOL bDragKeyPressed;
    BOOL button;

    TRACE("\n");

    if (infoPtr->dwStyle & TBSTYLE_ALTDRAG)
        bDragKeyPressed = (GetKeyState(VK_MENU) < 0);
    else
        bDragKeyPressed = (wParam & MK_SHIFT);

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, infoPtr->hwndSelf,
			    WM_LBUTTONDOWN, wParam, lParam);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (infoPtr, &pt, &button);

    if (button)
    {
        btnPtr = &infoPtr->buttons[nHit];

        if (bDragKeyPressed && (infoPtr->dwStyle & CCS_ADJUSTABLE))
        {
            infoPtr->nButtonDrag = nHit;
            SetCapture (infoPtr->hwndSelf);

            /* If drag cursor has not been loaded, load it.
             * Note: it doesn't need to be freed */
            if (!hCursorDrag)
#ifndef __REACTOS__
                hCursorDrag = LoadCursorW(COMCTL32_hModule, (LPCWSTR)IDC_MOVEBUTTON);
#else
                hCursorDrag = LoadCursorW(COMCTL32_hModule, MAKEINTRESOURCEW(IDC_MOVEBUTTON));
#endif
            SetCursor(hCursorDrag);
        }
        else
        {
            RECT arrowRect;
            infoPtr->nOldHit = nHit;

            arrowRect = btnPtr->rect;
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
                RedrawWindow(infoPtr->hwndSelf, &btnPtr->rect, 0, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);

                memset(&nmtb, 0, sizeof(nmtb));
                nmtb.iItem = btnPtr->idCommand;
                nmtb.rcButton = btnPtr->rect;
                res = TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr, TBN_DROPDOWN);
                TRACE("TBN_DROPDOWN responded with %ld\n", res);

                if (res != TBDDRET_TREATPRESSED)
                {
                    MSG msg;

                    /* redraw button in unpressed state */
                    if (btnPtr->fsStyle & BTNS_WHOLEDROPDOWN)
                        btnPtr->fsState &= ~TBSTATE_PRESSED;
                    else
                        btnPtr->bDropDownPressed = FALSE;
                    InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);

                    /* find and set hot item */
                    GetCursorPos(&pt);
                    ScreenToClient(infoPtr->hwndSelf, &pt);
                    nHit = TOOLBAR_InternalHitTest(infoPtr, &pt, &button);
                    if (!infoPtr->bAnchor || button)
                        TOOLBAR_SetHotItemEx(infoPtr, nHit, HICF_MOUSE | HICF_LMOUSE);

                    /* remove any left mouse button down or double-click messages
                     * so that we can get a toggle effect on the button */
                    while (PeekMessageW(&msg, infoPtr->hwndSelf, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE) ||
                           PeekMessageW(&msg, infoPtr->hwndSelf, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, PM_REMOVE))
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
                InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
            UpdateWindow(infoPtr->hwndSelf);
            SetCapture (infoPtr->hwndSelf);
        }

        memset(&nmtb, 0, sizeof(nmtb));
        nmtb.iItem = btnPtr->idCommand;
        TOOLBAR_SendNotify((NMHDR *)&nmtb, infoPtr, TBN_BEGINDRAG);
    }

    nmmouse.dwHitInfo = nHit;

    /* !!! Undocumented - sends NM_LDOWN with the NMMOUSE structure. */
    if (!button)
        nmmouse.dwItemSpec = -1;
    else
    {
        nmmouse.dwItemSpec = infoPtr->buttons[nmmouse.dwHitInfo].idCommand;
        nmmouse.dwItemData = infoPtr->buttons[nmmouse.dwHitInfo].dwData;
    }

    ClientToScreen(infoPtr->hwndSelf, &pt);
    nmmouse.pt = pt;

    if (!TOOLBAR_SendNotify(&nmmouse.hdr, infoPtr, NM_LDOWN))
        return DefWindowProcW(infoPtr->hwndSelf, WM_LBUTTONDOWN, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_LButtonUp (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TBUTTON_INFO *btnPtr;
    POINT pt;
    INT   nHit;
    INT   nOldIndex = -1;
    NMHDR hdr;
    NMMOUSE nmmouse;
    NMTOOLBARA nmtb;
    BOOL button;

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, infoPtr->hwndSelf,
			    WM_LBUTTONUP, wParam, lParam);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (infoPtr, &pt, &button);

    if (!infoPtr->bAnchor || button)
        TOOLBAR_SetHotItemEx(infoPtr, button ? nHit : TOOLBAR_NOWHERE, HICF_MOUSE | HICF_LMOUSE);

    if (infoPtr->nButtonDrag >= 0) {
        RECT rcClient;
        NMHDR hdr;

        btnPtr = &infoPtr->buttons[infoPtr->nButtonDrag];
        ReleaseCapture();
        /* reset cursor */
        SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_ARROW));

        GetClientRect(infoPtr->hwndSelf, &rcClient);
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
                /* if the button is moved slightly left and we have a
                 * separator there then remove it */
                if (pt.x < (btnPtr->rect.left + (btnPtr->rect.right - btnPtr->rect.left)/2))
                {
                    if ((nButton > 0) && (infoPtr->buttons[nButton-1].fsStyle & BTNS_SEP))
                        TOOLBAR_DeleteButton(infoPtr, nButton - 1);
                }
                else /* else insert a separator before the dragged button */
                {
                    TBBUTTON tbb;
                    memset(&tbb, 0, sizeof(tbb));
                    tbb.fsStyle = BTNS_SEP;
                    tbb.iString = -1;
                    TOOLBAR_InsertButtonT(infoPtr, nButton, &tbb, TRUE);
                }
            }
            else
            {
                if (nButton == -1)
                {
                    if ((infoPtr->nNumButtons > 0) && (pt.x < infoPtr->buttons[0].rect.left))
                        TOOLBAR_MoveButton(infoPtr, infoPtr->nButtonDrag, 0);
                    else
                        TOOLBAR_MoveButton(infoPtr, infoPtr->nButtonDrag, infoPtr->nNumButtons);
                }
                else
                    TOOLBAR_MoveButton(infoPtr, infoPtr->nButtonDrag, nButton);
            }
        }
        else
        {
            TRACE("button %d dragged out of toolbar\n", infoPtr->nButtonDrag);
            TOOLBAR_DeleteButton(infoPtr, infoPtr->nButtonDrag);
        }

        /* button under cursor changed so need to re-set hot item */
        TOOLBAR_SetHotItemEx(infoPtr, button ? nHit : TOOLBAR_NOWHERE, HICF_MOUSE | HICF_LMOUSE);
        infoPtr->nButtonDrag = -1;

        TOOLBAR_SendNotify(&hdr, infoPtr, TBN_TOOLBARCHANGE);
    }
    else if (infoPtr->nButtonDown >= 0)
    {
        BOOL was_clicked = nHit == infoPtr->nButtonDown;

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
            InvalidateRect(infoPtr->hwndSelf, &infoPtr->buttons[nOldIndex].rect, TRUE);

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

	memset(&nmtb, 0, sizeof(nmtb));
	nmtb.iItem = btnPtr->idCommand;
	TOOLBAR_SendNotify ((NMHDR *) &nmtb, infoPtr,
			TBN_ENDDRAG);

	if (was_clicked && btnPtr->fsState & TBSTATE_ENABLED)
	{
	    SendMessageW (infoPtr->hwndNotify, WM_COMMAND,
	      MAKEWPARAM(infoPtr->buttons[nHit].idCommand, BN_CLICKED), (LPARAM)infoPtr->hwndSelf);

            /* In case we have just been destroyed... */
            if(!IsWindow(infoPtr->hwndSelf))
                return 0;
        }
    }

    /* !!! Undocumented - toolbar at 4.71 level and above sends
    * NM_CLICK with the NMMOUSE structure. */
    nmmouse.dwHitInfo = nHit;

    if (!button)
        nmmouse.dwItemSpec = -1;
    else
    {
        nmmouse.dwItemSpec = infoPtr->buttons[nmmouse.dwHitInfo].idCommand;
        nmmouse.dwItemData = infoPtr->buttons[nmmouse.dwHitInfo].dwData;
    }

    ClientToScreen(infoPtr->hwndSelf, &pt);
    nmmouse.pt = pt;

    if (!TOOLBAR_SendNotify((LPNMHDR)&nmmouse, infoPtr, NM_CLICK))
        return DefWindowProcW(infoPtr->hwndSelf, WM_LBUTTONUP, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_RButtonUp(TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT nHit;
    NMMOUSE nmmouse;
    POINT pt;
    BOOL button;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    nHit = TOOLBAR_InternalHitTest(infoPtr, &pt, &button);
    nmmouse.dwHitInfo = nHit;

    if (!button) {
	nmmouse.dwItemSpec = -1;
    } else {
	nmmouse.dwItemSpec = infoPtr->buttons[nmmouse.dwHitInfo].idCommand;
	nmmouse.dwItemData = infoPtr->buttons[nmmouse.dwHitInfo].dwData;
    }

    ClientToScreen(infoPtr->hwndSelf, &pt);
    nmmouse.pt = pt;

    if (!TOOLBAR_SendNotify((LPNMHDR)&nmmouse, infoPtr, NM_RCLICK))
        return DefWindowProcW(infoPtr->hwndSelf, WM_RBUTTONUP, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_RButtonDblClk( TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    NMHDR nmhdr;

    if (!TOOLBAR_SendNotify(&nmhdr, infoPtr, NM_RDBLCLK))
        return DefWindowProcW(infoPtr->hwndSelf, WM_RBUTTONDBLCLK, wParam, lParam);

    return 0;
}

static LRESULT
TOOLBAR_CaptureChanged(TOOLBAR_INFO *infoPtr)
{
    TBUTTON_INFO *btnPtr;

    infoPtr->bCaptured = FALSE;

    if (infoPtr->nButtonDown >= 0)
    {
        btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
       	btnPtr->fsState &= ~TBSTATE_PRESSED;

        infoPtr->nOldHit = -1;

        if (btnPtr->fsState & TBSTATE_ENABLED)
            InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
    }
    return 0;
}

static LRESULT
TOOLBAR_MouseLeave (TOOLBAR_INFO *infoPtr)
{
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
      InvalidateRect (infoPtr->hwndSelf, &rc1, TRUE);
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
TOOLBAR_MouseMove (TOOLBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    TRACKMOUSEEVENT trackinfo;
    INT   nHit;
    TBUTTON_INFO *btnPtr;
    BOOL button;
    
    if ((infoPtr->dwStyle & TBSTYLE_TOOLTIPS) && (infoPtr->hwndToolTip == NULL))
        TOOLBAR_TooltipCreateControl(infoPtr);
    
    if ((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)) {
        /* fill in the TRACKMOUSEEVENT struct */
        trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
        trackinfo.dwFlags = TME_QUERY;

        /* call _TrackMouseEvent to see if we are currently tracking for this hwnd */
        _TrackMouseEvent(&trackinfo);

        /* Make sure tracking is enabled so we receive a WM_MOUSELEAVE message */
        if(trackinfo.hwndTrack != infoPtr->hwndSelf || !(trackinfo.dwFlags & TME_LEAVE)) {
            trackinfo.dwFlags = TME_LEAVE; /* notify upon leaving */
            trackinfo.hwndTrack = infoPtr->hwndSelf;

            /* call TRACKMOUSEEVENT so we receive a WM_MOUSELEAVE message */
            /* and can properly deactivate the hot toolbar button */
            _TrackMouseEvent(&trackinfo);
        }
    }

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, infoPtr->hwndSelf,
			    WM_MOUSEMOVE, wParam, lParam);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    nHit = TOOLBAR_InternalHitTest (infoPtr, &pt, &button);

    if (((infoPtr->dwStyle & TBSTYLE_FLAT) || GetWindowTheme (infoPtr->hwndSelf)) 
        && (!infoPtr->bAnchor || button))
        TOOLBAR_SetHotItemEx(infoPtr, button ? nHit : TOOLBAR_NOWHERE, HICF_MOUSE);

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
                InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
            }
            else if (nHit == infoPtr->nButtonDown) {
                btnPtr->fsState |= TBSTATE_PRESSED;
                InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
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
#ifdef __REACTOS__
TOOLBAR_NCCreate (HWND hwnd, WPARAM wParam, const CREATESTRUCTW *lpcs, int iVersion)
#else
TOOLBAR_NCCreate (HWND hwnd, WPARAM wParam, const CREATESTRUCTW *lpcs)
#endif
{
    TOOLBAR_INFO *infoPtr;
    DWORD styleadd = 0;

    /* allocate memory for info structure */
    infoPtr = Alloc (sizeof(TOOLBAR_INFO));
    SetWindowLongPtrW (hwnd, 0, (LONG_PTR)infoPtr);

    /* paranoid!! */
    infoPtr->dwStructSize = sizeof(TBBUTTON);
    infoPtr->nRows = 1;

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
    infoPtr->hwndNotify = lpcs->hwndParent;
    infoPtr->dwDTFlags = (lpcs->style & TBSTYLE_LIST) ? DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS: DT_CENTER | DT_END_ELLIPSIS;
    infoPtr->bAnchor = FALSE; /* no anchor highlighting */
    infoPtr->bDragOutSent = FALSE;
#ifdef __REACTOS__
    infoPtr->iVersion = iVersion;
#else
    infoPtr->iVersion = 0;
#endif
    infoPtr->hwndSelf = hwnd;
    infoPtr->bDoRedraw = TRUE;
    infoPtr->clrBtnHighlight = CLR_DEFAULT;
    infoPtr->clrBtnShadow = CLR_DEFAULT;
    infoPtr->szPadding.cx = DEFPAD_CX;
    infoPtr->szPadding.cy = DEFPAD_CY;
#ifdef __REACTOS__
    infoPtr->szSpacing.cx = 0;
    infoPtr->szSpacing.cy = 0;
    memset(&infoPtr->themeMargins, 0 , sizeof(infoPtr->themeMargins));
#endif
    infoPtr->iListGap = DEFLISTGAP;
    infoPtr->iTopMargin = default_top_margin(infoPtr);
    infoPtr->dwStyle = lpcs->style;
    infoPtr->tbim.iButton = -1;

    /* fix instance handle, if the toolbar was created by CreateToolbarEx() */
    if (!GetWindowLongPtrW (hwnd, GWLP_HINSTANCE)) {
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW (GetParent (hwnd), GWLP_HINSTANCE);
	SetWindowLongPtrW (hwnd, GWLP_HINSTANCE, (LONG_PTR)hInst);
    }

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
        && !(lpcs->style & TBSTYLE_TRANSPARENT))
	styleadd |= TBSTYLE_TRANSPARENT;
    if (!(lpcs->style & (CCS_TOP | CCS_NOMOVEY))) {
	styleadd |= CCS_TOP;   /* default to top */
	SetWindowLongW (hwnd, GWL_STYLE, lpcs->style | styleadd);
    }

    return DefWindowProcW (hwnd, WM_NCCREATE, wParam, (LPARAM)lpcs);
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
	    InflateRect (&rcWindow, -1, -1);
	DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_TOP);
    }

    ReleaseDC( hwnd, hdc );

    return 0;
}


/* handles requests from the tooltip control on what text to display */
static LRESULT TOOLBAR_TTGetDispInfo (TOOLBAR_INFO *infoPtr, NMTTDISPINFOW *lpnmtdi)
{
    int index = TOOLBAR_GetButtonIndex(infoPtr, lpnmtdi->hdr.idFrom, FALSE);
    NMTTDISPINFOA nmtdi;
    unsigned int len;
    LRESULT ret;

    TRACE("button index = %d\n", index);

    Free (infoPtr->pszTooltipText);
    infoPtr->pszTooltipText = NULL;

    if (index < 0)
        return 0;

    if (infoPtr->bUnicode)
    {
        WCHAR wszBuffer[INFOTIPSIZE+1];
        NMTBGETINFOTIPW tbgit;

        wszBuffer[0] = '\0';
        wszBuffer[INFOTIPSIZE] = '\0';

        tbgit.pszText = wszBuffer;
        tbgit.cchTextMax = INFOTIPSIZE;
        tbgit.iItem = lpnmtdi->hdr.idFrom;
        tbgit.lParam = infoPtr->buttons[index].dwData;

        TOOLBAR_SendNotify(&tbgit.hdr, infoPtr, TBN_GETINFOTIPW);

        TRACE("TBN_GETINFOTIPW - got string %s\n", debugstr_w(tbgit.pszText));

        len = tbgit.pszText ? lstrlenW(tbgit.pszText) : 0;
        if (len > ARRAY_SIZE(lpnmtdi->szText) - 1)
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

        szBuffer[0] = '\0';
        szBuffer[INFOTIPSIZE] = '\0';

        tbgit.pszText = szBuffer;
        tbgit.cchTextMax = INFOTIPSIZE;
        tbgit.iItem = lpnmtdi->hdr.idFrom;
        tbgit.lParam = infoPtr->buttons[index].dwData;

        TOOLBAR_SendNotify(&tbgit.hdr, infoPtr, TBN_GETINFOTIPA);

        TRACE("TBN_GETINFOTIPA - got string %s\n", debugstr_a(tbgit.pszText));

        len = MultiByteToWideChar(CP_ACP, 0, tbgit.pszText, -1, NULL, 0);
        if (len > ARRAY_SIZE(lpnmtdi->szText))
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
        else if (tbgit.pszText && tbgit.pszText[0])
        {
            MultiByteToWideChar(CP_ACP, 0, tbgit.pszText, -1, lpnmtdi->lpszText, ARRAY_SIZE(lpnmtdi->szText));
            return 0;
        }
    }

    /* if button has text, but it is not shown then automatically
     * use that text as tooltip */
    if ((infoPtr->dwExStyle & TBSTYLE_EX_MIXEDBUTTONS) &&
        !(infoPtr->buttons[index].fsStyle & BTNS_SHOWTEXT))
    {
        LPWSTR pszText = TOOLBAR_GetText(infoPtr, &infoPtr->buttons[index]);
        len = pszText ? lstrlenW(pszText) : 0;

        TRACE("using button hidden text %s\n", debugstr_w(pszText));

        if (len > ARRAY_SIZE(lpnmtdi->szText) - 1)
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

    /* Last resort, forward TTN_GETDISPINFO to the app:

       - NFR_UNICODE gets TTN_GETDISPINFOW, and TTN_GETDISPINFOA if -W returned no text;
       - NFR_ANSI gets only TTN_GETDISPINFOA.
    */
    if (infoPtr->bUnicode)
    {
        ret = SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, lpnmtdi->hdr.idFrom, (LPARAM)lpnmtdi);

        TRACE("TTN_GETDISPINFOW - got string %s\n", debugstr_w(lpnmtdi->lpszText));

        if (IS_INTRESOURCE(lpnmtdi->lpszText))
            return ret;

        if (lpnmtdi->lpszText && *lpnmtdi->lpszText)
            return ret;
    }

    nmtdi.hdr.hwndFrom = lpnmtdi->hdr.hwndFrom;
    nmtdi.hdr.idFrom = lpnmtdi->hdr.idFrom;
    nmtdi.hdr.code = TTN_GETDISPINFOA;
    nmtdi.lpszText = nmtdi.szText;
    nmtdi.szText[0] = 0;
    nmtdi.hinst = lpnmtdi->hinst;
    nmtdi.uFlags = lpnmtdi->uFlags;
    nmtdi.lParam = lpnmtdi->lParam;

    ret = SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, nmtdi.hdr.idFrom, (LPARAM)&nmtdi);

    TRACE("TTN_GETDISPINFOA - got string %s\n", debugstr_a(nmtdi.lpszText));

    lpnmtdi->hinst = nmtdi.hinst;
    lpnmtdi->uFlags = nmtdi.uFlags;
    lpnmtdi->lParam = nmtdi.lParam;

    if (IS_INTRESOURCE(nmtdi.lpszText))
    {
        lpnmtdi->lpszText = (WCHAR *)nmtdi.lpszText;
        return ret;
    }

    if (!nmtdi.lpszText || !*nmtdi.lpszText)
        return ret;

    len = MultiByteToWideChar(CP_ACP, 0, nmtdi.lpszText, -1, NULL, 0);
    if (len > ARRAY_SIZE(lpnmtdi->szText))
    {
        infoPtr->pszTooltipText = Alloc(len * sizeof(WCHAR));
        if (infoPtr->pszTooltipText)
        {
            MultiByteToWideChar(CP_ACP, 0, nmtdi.lpszText, -1, infoPtr->pszTooltipText, len);
            lpnmtdi->lpszText = infoPtr->pszTooltipText;
            return 0;
        }
    }
    else
    {
        MultiByteToWideChar(CP_ACP, 0, nmtdi.lpszText, -1, lpnmtdi->lpszText, ARRAY_SIZE(nmtdi.szText));
        return 0;
    }

    return ret;
}


static inline LRESULT
TOOLBAR_Notify (TOOLBAR_INFO *infoPtr, LPNMHDR lpnmh)
{
    switch (lpnmh->code)
    {
    case PGN_CALCSIZE:
    {
        LPNMPGCALCSIZE lppgc = (LPNMPGCALCSIZE)lpnmh;

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
        LPNMPGSCROLL lppgs = (LPNMPGSCROLL)lpnmh;

        lppgs->iScroll = (lppgs->iDir & (PGF_SCROLLLEFT | PGF_SCROLLRIGHT)) ?
                          infoPtr->nButtonWidth : infoPtr->nButtonHeight;
        TRACE("processed PGN_SCROLL, returning scroll=%d, dir=%d\n",
              lppgs->iScroll, lppgs->iDir);
        return 0;
    }

    case TTN_GETDISPINFOW:
        return TOOLBAR_TTGetDispInfo(infoPtr, (LPNMTTDISPINFOW)lpnmh);

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
TOOLBAR_Paint (TOOLBAR_INFO *infoPtr, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    /* fill ps.rcPaint with a default rect */
    ps.rcPaint = infoPtr->rcBound;

    hdc = wParam==0 ? BeginPaint(infoPtr->hwndSelf, &ps) : (HDC)wParam;

    TRACE("psrect=(%s)\n", wine_dbgstr_rect(&ps.rcPaint));

    TOOLBAR_Refresh (infoPtr, hdc, &ps);
    if (!wParam) EndPaint (infoPtr->hwndSelf, &ps);

    return 0;
}


static LRESULT
TOOLBAR_SetFocus (TOOLBAR_INFO *infoPtr)
{
    TRACE("nHotItem = %d\n", infoPtr->nHotItem);

    /* make first item hot */
    if (infoPtr->nNumButtons > 0)
        TOOLBAR_SetHotItemEx(infoPtr, 0, HICF_OTHER);

    return 0;
}

static LRESULT
TOOLBAR_SetFont(TOOLBAR_INFO *infoPtr, HFONT hFont, WORD Redraw)
{
    TRACE("font=%p redraw=%d\n", hFont, Redraw);
    
    if (hFont == 0)
        infoPtr->hFont = infoPtr->hDefaultFont;
    else
        infoPtr->hFont = hFont;

    TOOLBAR_CalcToolbar(infoPtr);

    if (Redraw)
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 1;
}

static LRESULT
TOOLBAR_SetRedraw (TOOLBAR_INFO *infoPtr, WPARAM wParam)
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
TOOLBAR_Size (TOOLBAR_INFO *infoPtr)
{
    TRACE("sizing toolbar\n");

    if (infoPtr->dwExStyle & TBSTYLE_EX_HIDECLIPPEDBUTTONS)
    {
        RECT delta_width, delta_height, client, dummy;
        DWORD min_x, max_x, min_y, max_y;
        TBUTTON_INFO *btnPtr;
        INT i;

        GetClientRect(infoPtr->hwndSelf, &client);
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
                InvalidateRect(infoPtr->hwndSelf, &btnPtr->rect, TRUE);
    }
    GetClientRect(infoPtr->hwndSelf, &infoPtr->client_rect);
    TOOLBAR_AutoSize(infoPtr);
    return 0;
}


static LRESULT
TOOLBAR_StyleChanged (TOOLBAR_INFO *infoPtr, INT nType, const STYLESTRUCT *lpStyle)
{
    if (nType == GWL_STYLE)
        return TOOLBAR_SetStyle(infoPtr, lpStyle->styleNew);

    return 0;
}


static LRESULT
TOOLBAR_SysColorChange (void)
{
    COMCTL32_RefreshSysColors();

    return 0;
}

#ifdef __REACTOS__
/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (TOOLBAR_INFO *infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    CloseThemeData (theme);
    OpenThemeData (infoPtr->hwndSelf, themeClass);
    theme = GetWindowTheme (infoPtr->hwndSelf);
    if (theme)
        GetThemeMargins(theme, NULL, TP_BUTTON, TS_NORMAL, TMT_CONTENTMARGINS, NULL, &infoPtr->themeMargins);
    else
        memset(&infoPtr->themeMargins, 0 ,sizeof(infoPtr->themeMargins));

    return 0;
}
#else
static LRESULT theme_changed (HWND hwnd)
{
    HTHEME theme = GetWindowTheme (hwnd);
    CloseThemeData (theme);
    OpenThemeData (hwnd, themeClass);
    return 0;
}
#endif

static LRESULT WINAPI
ToolbarWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = (TOOLBAR_INFO *)GetWindowLongPtrW(hwnd, 0);

    TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n",
	  hwnd, uMsg, /* SPY_GetMsgName(uMsg), */ wParam, lParam);

    if (!infoPtr && (uMsg != WM_NCCREATE))
	return DefWindowProcW( hwnd, uMsg, wParam, lParam );

    switch (uMsg)
    {
	case TB_ADDBITMAP:
	    return TOOLBAR_AddBitmap (infoPtr, (INT)wParam, (TBADDBITMAP*)lParam);

	case TB_ADDBUTTONSA:
	case TB_ADDBUTTONSW:
	    return TOOLBAR_AddButtonsT (infoPtr, wParam, (LPTBBUTTON)lParam,
	                                uMsg == TB_ADDBUTTONSW);
	case TB_ADDSTRINGA:
	    return TOOLBAR_AddStringA (infoPtr, (HINSTANCE)wParam, lParam);

	case TB_ADDSTRINGW:
	    return TOOLBAR_AddStringW (infoPtr, (HINSTANCE)wParam, lParam);

	case TB_AUTOSIZE:
	    return TOOLBAR_AutoSize (infoPtr);

	case TB_BUTTONCOUNT:
	    return TOOLBAR_ButtonCount (infoPtr);

	case TB_BUTTONSTRUCTSIZE:
	    return TOOLBAR_ButtonStructSize (infoPtr, wParam);

	case TB_CHANGEBITMAP:
	    return TOOLBAR_ChangeBitmap (infoPtr, wParam, LOWORD(lParam));

	case TB_CHECKBUTTON:
	    return TOOLBAR_CheckButton (infoPtr, wParam, lParam);

	case TB_COMMANDTOINDEX:
	    return TOOLBAR_CommandToIndex (infoPtr, wParam);

	case TB_CUSTOMIZE:
	    return TOOLBAR_Customize (infoPtr);

	case TB_DELETEBUTTON:
	    return TOOLBAR_DeleteButton (infoPtr, wParam);

	case TB_ENABLEBUTTON:
	    return TOOLBAR_EnableButton (infoPtr, wParam, lParam);

	case TB_GETANCHORHIGHLIGHT:
	    return TOOLBAR_GetAnchorHighlight (infoPtr);

	case TB_GETBITMAP:
	    return TOOLBAR_GetBitmap (infoPtr, wParam);

	case TB_GETBITMAPFLAGS:
	    return TOOLBAR_GetBitmapFlags ();

	case TB_GETBUTTON:
	    return TOOLBAR_GetButton (infoPtr, wParam, (TBBUTTON*)lParam);

	case TB_GETBUTTONINFOA:
	case TB_GETBUTTONINFOW:
	    return TOOLBAR_GetButtonInfoT (infoPtr, wParam, (LPTBBUTTONINFOW)lParam,
	                                   uMsg == TB_GETBUTTONINFOW);
	case TB_GETBUTTONSIZE:
	    return TOOLBAR_GetButtonSize (infoPtr);

	case TB_GETBUTTONTEXTA:
	case TB_GETBUTTONTEXTW:
	    return TOOLBAR_GetButtonText (infoPtr, wParam, (LPWSTR)lParam,
	                                  uMsg == TB_GETBUTTONTEXTW);

	case TB_GETDISABLEDIMAGELIST:
	    return TOOLBAR_GetDisabledImageList (infoPtr, wParam);

	case TB_GETEXTENDEDSTYLE:
	    return TOOLBAR_GetExtendedStyle (infoPtr);

	case TB_GETHOTIMAGELIST:
	    return TOOLBAR_GetHotImageList (infoPtr, wParam);

	case TB_GETHOTITEM:
	    return TOOLBAR_GetHotItem (infoPtr);

	case TB_GETIMAGELIST:
	    return TOOLBAR_GetDefImageList (infoPtr, wParam);

	case TB_GETINSERTMARK:
	    return TOOLBAR_GetInsertMark (infoPtr, (TBINSERTMARK*)lParam);

	case TB_GETINSERTMARKCOLOR:
	    return TOOLBAR_GetInsertMarkColor (infoPtr);

	case TB_GETITEMRECT:
	    return TOOLBAR_GetItemRect (infoPtr, wParam, (LPRECT)lParam);

	case TB_GETMAXSIZE:
	    return TOOLBAR_GetMaxSize (infoPtr, (LPSIZE)lParam);
#ifdef __REACTOS__
	case TB_GETMETRICS:
	    return TOOLBAR_GetMetrics (infoPtr, (TBMETRICS*)lParam);
#endif

/*	case TB_GETOBJECT:			*/ /* 4.71 */

	case TB_GETPADDING:
	    return TOOLBAR_GetPadding (infoPtr);

	case TB_GETRECT:
	    return TOOLBAR_GetRect (infoPtr, wParam, (LPRECT)lParam);

	case TB_GETROWS:
	    return TOOLBAR_GetRows (infoPtr);

	case TB_GETSTATE:
	    return TOOLBAR_GetState (infoPtr, wParam);

	case TB_GETSTRINGA:
            return TOOLBAR_GetStringA (infoPtr, wParam, (LPSTR)lParam);

	case TB_GETSTRINGW:
	    return TOOLBAR_GetStringW (infoPtr, wParam, (LPWSTR)lParam);

	case TB_GETSTYLE:
	    return TOOLBAR_GetStyle (infoPtr);

	case TB_GETTEXTROWS:
	    return TOOLBAR_GetTextRows (infoPtr);

	case TB_GETTOOLTIPS:
	    return TOOLBAR_GetToolTips (infoPtr);

	case TB_GETUNICODEFORMAT:
	    return TOOLBAR_GetUnicodeFormat (infoPtr);

	case TB_HIDEBUTTON:
	    return TOOLBAR_HideButton (infoPtr, wParam, LOWORD(lParam));

	case TB_HITTEST:
	    return TOOLBAR_HitTest (infoPtr, (LPPOINT)lParam);

	case TB_INDETERMINATE:
	    return TOOLBAR_Indeterminate (infoPtr, wParam, LOWORD(lParam));

	case TB_INSERTBUTTONA:
	case TB_INSERTBUTTONW:
	    return TOOLBAR_InsertButtonT(infoPtr, wParam, (TBBUTTON*)lParam,
	                                 uMsg == TB_INSERTBUTTONW);

/*	case TB_INSERTMARKHITTEST:		*/ /* 4.71 */

	case TB_ISBUTTONCHECKED:
	    return TOOLBAR_IsButtonChecked (infoPtr, wParam);

	case TB_ISBUTTONENABLED:
	    return TOOLBAR_IsButtonEnabled (infoPtr, wParam);

	case TB_ISBUTTONHIDDEN:
	    return TOOLBAR_IsButtonHidden (infoPtr, wParam);

	case TB_ISBUTTONHIGHLIGHTED:
	    return TOOLBAR_IsButtonHighlighted (infoPtr, wParam);

	case TB_ISBUTTONINDETERMINATE:
	    return TOOLBAR_IsButtonIndeterminate (infoPtr, wParam);

	case TB_ISBUTTONPRESSED:
	    return TOOLBAR_IsButtonPressed (infoPtr, wParam);

	case TB_LOADIMAGES:
	    return TOOLBAR_LoadImages (infoPtr, wParam, (HINSTANCE)lParam);

	case TB_MAPACCELERATORA:
	case TB_MAPACCELERATORW:
	    return TOOLBAR_MapAccelerator (infoPtr, wParam, (UINT*)lParam);

	case TB_MARKBUTTON:
	    return TOOLBAR_MarkButton (infoPtr, wParam, LOWORD(lParam));

	case TB_MOVEBUTTON:
	    return TOOLBAR_MoveButton (infoPtr, wParam, lParam);

	case TB_PRESSBUTTON:
	    return TOOLBAR_PressButton (infoPtr, wParam, LOWORD(lParam));

	case TB_REPLACEBITMAP:
            return TOOLBAR_ReplaceBitmap (infoPtr, (LPTBREPLACEBITMAP)lParam);

	case TB_SAVERESTOREA:
	    return TOOLBAR_SaveRestoreA (infoPtr, wParam, (LPTBSAVEPARAMSA)lParam);

	case TB_SAVERESTOREW:
	    return TOOLBAR_SaveRestoreW (infoPtr, wParam, (LPTBSAVEPARAMSW)lParam);

	case TB_SETANCHORHIGHLIGHT:
	    return TOOLBAR_SetAnchorHighlight (infoPtr, (BOOL)wParam);

	case TB_SETBITMAPSIZE:
	    return TOOLBAR_SetBitmapSize (infoPtr, wParam, lParam);

	case TB_SETBUTTONINFOA:
	case TB_SETBUTTONINFOW:
	    return TOOLBAR_SetButtonInfo (infoPtr, wParam, (LPTBBUTTONINFOW)lParam,
                                          uMsg == TB_SETBUTTONINFOW);
	case TB_SETBUTTONSIZE:
	    return TOOLBAR_SetButtonSize (infoPtr, lParam);

	case TB_SETBUTTONWIDTH:
	    return TOOLBAR_SetButtonWidth (infoPtr, lParam);

	case TB_SETCMDID:
	    return TOOLBAR_SetCmdId (infoPtr, wParam, lParam);

	case TB_SETDISABLEDIMAGELIST:
	    return TOOLBAR_SetDisabledImageList (infoPtr, wParam, (HIMAGELIST)lParam);

	case TB_SETDRAWTEXTFLAGS:
	    return TOOLBAR_SetDrawTextFlags (infoPtr, wParam, lParam);

	case TB_SETEXTENDEDSTYLE:
	    return TOOLBAR_SetExtendedStyle (infoPtr, wParam, lParam);

	case TB_SETHOTIMAGELIST:
	    return TOOLBAR_SetHotImageList (infoPtr, wParam, (HIMAGELIST)lParam);

	case TB_SETHOTITEM:
	    return TOOLBAR_SetHotItem (infoPtr, wParam);

	case TB_SETIMAGELIST:
	    return TOOLBAR_SetImageList (infoPtr, wParam, (HIMAGELIST)lParam);

	case TB_SETINDENT:
	    return TOOLBAR_SetIndent (infoPtr, wParam);

	case TB_SETINSERTMARK:
	    return TOOLBAR_SetInsertMark (infoPtr, (TBINSERTMARK*)lParam);

	case TB_SETINSERTMARKCOLOR:
	    return TOOLBAR_SetInsertMarkColor (infoPtr, lParam);

	case TB_SETMAXTEXTROWS:
	    return TOOLBAR_SetMaxTextRows (infoPtr, wParam);

#ifdef __REACTOS__
	case TB_SETMETRICS:
	    return TOOLBAR_SetMetrics (infoPtr, (TBMETRICS*)lParam);
#endif

	case TB_SETPADDING:
	    return TOOLBAR_SetPadding (infoPtr, lParam);

	case TB_SETPARENT:
	    return TOOLBAR_SetParent (infoPtr, (HWND)wParam);

	case TB_SETROWS:
	    return TOOLBAR_SetRows (infoPtr, wParam, (LPRECT)lParam);

	case TB_SETSTATE:
	    return TOOLBAR_SetState (infoPtr, wParam, lParam);

	case TB_SETSTYLE:
	    return TOOLBAR_SetStyle (infoPtr, lParam);

	case TB_SETTOOLTIPS:
	    return TOOLBAR_SetToolTips (infoPtr, (HWND)wParam);

	case TB_SETUNICODEFORMAT:
	    return TOOLBAR_SetUnicodeFormat (infoPtr, wParam);

	case TB_SETBOUNDINGSIZE:
	    return TOOLBAR_SetBoundingSize(hwnd, wParam, lParam);

	case TB_SETHOTITEM2:
	    return TOOLBAR_SetHotItem2 (infoPtr, wParam, lParam);

	case TB_SETLISTGAP:
	    return TOOLBAR_SetListGap(infoPtr, wParam);

	case TB_GETIMAGELISTCOUNT:
	    return TOOLBAR_GetImageListCount(infoPtr);

	case TB_GETIDEALSIZE:
	    return TOOLBAR_GetIdealSize (infoPtr, wParam, lParam);

	case TB_UNKWN464:
	    return TOOLBAR_Unkwn464(hwnd, wParam, lParam);

/* Common Control Messages */

/*	case TB_GETCOLORSCHEME:			*/ /* identical to CCM_ */
	case CCM_GETCOLORSCHEME:
	    return TOOLBAR_GetColorScheme (infoPtr, (LPCOLORSCHEME)lParam);

/*	case TB_SETCOLORSCHEME:			*/ /* identical to CCM_ */
	case CCM_SETCOLORSCHEME:
	    return TOOLBAR_SetColorScheme (infoPtr, (LPCOLORSCHEME)lParam);

	case CCM_GETVERSION:
	    return TOOLBAR_GetVersion (infoPtr);

	case CCM_SETVERSION:
	    return TOOLBAR_SetVersion (infoPtr, (INT)wParam);


/*	case WM_CHAR: */

	case WM_CREATE:
	    return TOOLBAR_Create (hwnd, (CREATESTRUCTW*)lParam);

	case WM_DESTROY:
	  return TOOLBAR_Destroy (infoPtr);

	case WM_ERASEBKGND:
	    return TOOLBAR_EraseBackground (infoPtr, wParam, lParam);

	case WM_GETFONT:
		return TOOLBAR_GetFont (infoPtr);

	case WM_KEYDOWN:
	    return TOOLBAR_KeyDown (infoPtr, wParam, lParam);

/*	case WM_KILLFOCUS: */

	case WM_LBUTTONDBLCLK:
	    return TOOLBAR_LButtonDblClk (infoPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return TOOLBAR_LButtonDown (infoPtr, wParam, lParam);

	case WM_LBUTTONUP:
	    return TOOLBAR_LButtonUp (infoPtr, wParam, lParam);

	case WM_RBUTTONUP:
	    return TOOLBAR_RButtonUp (infoPtr, wParam, lParam);

	case WM_RBUTTONDBLCLK:
	    return TOOLBAR_RButtonDblClk (infoPtr, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TOOLBAR_MouseMove (infoPtr, wParam, lParam);

	case WM_MOUSELEAVE:
	    return TOOLBAR_MouseLeave (infoPtr);

	case WM_CAPTURECHANGED:
	    if (hwnd == (HWND)lParam) return 0;
	    return TOOLBAR_CaptureChanged(infoPtr);

	case WM_NCACTIVATE:
	    return TOOLBAR_NCActivate (hwnd, wParam, lParam);

	case WM_NCCALCSIZE:
	    return TOOLBAR_NCCalcSize (hwnd, wParam, lParam);

	case WM_NCCREATE:
#ifdef __REACTOS__
	    return TOOLBAR_NCCreate (hwnd, wParam, (CREATESTRUCTW*)lParam, 0);
#else
	    return TOOLBAR_NCCreate (hwnd, wParam, (CREATESTRUCTW*)lParam);
#endif

	case WM_NCPAINT:
	    return TOOLBAR_NCPaint (hwnd, wParam, lParam);

	case WM_NOTIFY:
	    return TOOLBAR_Notify (infoPtr, (LPNMHDR)lParam);

	case WM_NOTIFYFORMAT:
	    return TOOLBAR_NotifyFormat (infoPtr, wParam, lParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return TOOLBAR_Paint (infoPtr, wParam);

	case WM_SETFOCUS:
	    return TOOLBAR_SetFocus (infoPtr);

	case WM_SETFONT:
            return TOOLBAR_SetFont(infoPtr, (HFONT)wParam, (WORD)lParam);

	case WM_SETREDRAW:
	    return TOOLBAR_SetRedraw (infoPtr, wParam);

	case WM_SIZE:
	    return TOOLBAR_Size (infoPtr);

	case WM_STYLECHANGED:
	    return TOOLBAR_StyleChanged (infoPtr, (INT)wParam, (LPSTYLESTRUCT)lParam);

	case WM_SYSCOLORCHANGE:
	    return TOOLBAR_SysColorChange ();
    case WM_THEMECHANGED:
#ifdef __REACTOS__
            return theme_changed (infoPtr);
#else
            return theme_changed (hwnd);
#endif

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

#ifdef __REACTOS__
static LRESULT WINAPI
ToolbarV6WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_NCCREATE)
        return TOOLBAR_NCCreate (hwnd, wParam, (CREATESTRUCTW*)lParam, 6);
    else
        return ToolbarWindowProc(hwnd, uMsg, wParam, lParam);
}

VOID
TOOLBARv6_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = ToolbarV6WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLBAR_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = TOOLBARCLASSNAMEW;

    RegisterClassW (&wndClass);
}

VOID
TOOLBARv6_Unregister (void)
{
    UnregisterClassW (TOOLBARCLASSNAMEW, NULL);
}
#endif

static HIMAGELIST TOOLBAR_InsertImageList(PIMLENTRY **pies, INT *cies, HIMAGELIST himl, INT id)
{
    HIMAGELIST himlold;
    PIMLENTRY c = NULL;

    /* Check if the entry already exists */
    c = TOOLBAR_GetImageListEntry(*pies, *cies, id);

    /* Don't add new entry for NULL imagelist */
    if (!c && !himl)
        return NULL;

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


static BOOL TOOLBAR_IsButtonRemovable(const TOOLBAR_INFO *infoPtr, int iItem, const CUSTOMBUTTON *btnInfo)
{
    NMTOOLBARW nmtb;

    /* MSDN states that iItem is the index of the button, rather than the
     * command ID as used by every other NMTOOLBAR notification */
    nmtb.iItem = iItem;
    memcpy(&nmtb.tbButton, &btnInfo->btn, sizeof(TBBUTTON));

    return TOOLBAR_SendNotify(&nmtb.hdr, infoPtr, TBN_QUERYDELETE);
}
