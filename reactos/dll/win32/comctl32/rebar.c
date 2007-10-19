/*
 * Rebar control
 *
 * Copyright 1998, 1999 Eric Kohl
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
 * of Comctl32.dll version 6.0 on Oct. 19, 2004, by Robert Shearman.
 *
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features or bugs please note them below.
 *
 * TODO
 *   Styles:
 *   - RBS_DBLCLKTOGGLE
 *   - RBS_FIXEDORDER
 *   - RBS_REGISTERDROP
 *   - RBS_TOOLTIPS
 *   - RBS_AUTOSIZE
 *   Messages:
 *   - RB_BEGINDRAG
 *   - RB_DRAGMOVE
 *   - RB_ENDDRAG
 *   - RB_GETBANDMARGINS
 *   - RB_GETCOLORSCHEME
 *   - RB_GETDROPTARGET
 *   - RB_GETPALETTE
 *   - RB_SETCOLORSCHEME
 *   - RB_SETPALETTE
 *   - RB_SETTOOLTIPS
 *   - WM_CHARTOITEM
 *   - WM_LBUTTONDBLCLK
 *   - WM_MEASUREITEM
 *   - WM_PALETTECHANGED
 *   - WM_QUERYNEWPALETTE
 *   - WM_RBUTTONDOWN
 *   - WM_RBUTTONUP
 *   - WM_SYSCOLORCHANGE
 *   - WM_VKEYTOITEM
 *   - WM_WININICHANGE
 *   Notifications:
 *   - NM_HCHITTEST
 *   - NM_RELEASEDCAPTURE
 *   - RBN_AUTOBREAK
 *   - RBN_GETOBJECT
 *   - RBN_MINMAX
 *   Band styles:
 *   - RBBS_FIXEDBMP
 *   Native uses (on each draw!!) SM_CYBORDER (or SM_CXBORDER for CCS_VERT)
 *   to set the size of the separator width (the value SEP_WIDTH_SIZE
 *   in here). Should be fixed!!
 */

/*
 * Testing: set to 1 to make background brush *always* green
 */
#define GLATESTING 0

/*
 * 3. REBAR_MoveChildWindows should have a loop because more than
 *    one pass is made (together with the RBN_CHILDSIZEs) is made on
 *    at least RB_INSERTBAND
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/unicode.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "tmschema.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rebar);

typedef struct
{
    UINT    fStyle;
    UINT    fMask;
    COLORREF  clrFore;
    COLORREF  clrBack;
    INT     iImage;
    HWND    hwndChild;
    UINT    cxMinChild;     /* valid if _CHILDSIZE */
    UINT    cyMinChild;     /* valid if _CHILDSIZE */
    UINT    cx;             /* valid if _SIZE */
    HBITMAP hbmBack;
    UINT    wID;
    UINT    cyChild;        /* valid if _CHILDSIZE */
    UINT    cyMaxChild;     /* valid if _CHILDSIZE */
    UINT    cyIntegral;     /* valid if _CHILDSIZE */
    UINT    cxIdeal;
    LPARAM    lParam;
    UINT    cxHeader;

    INT     cxEffective;     /* current cx for band */
    UINT    lcx;            /* minimum cx for band */
    UINT    lcy;            /* minimum cy for band */

    INT     iRow;           /* zero-based index of the row this band assigned to */
    UINT    fStatus;        /* status flags, reset only by _Validate */
    UINT    fDraw;          /* drawing flags, reset only by _Layout */
    UINT    uCDret;         /* last return from NM_CUSTOMDRAW */
    RECT    rcBand;         /* calculated band rectangle - coordinates swapped for CCS_VERT */
    RECT    rcGripper;      /* calculated gripper rectangle */
    RECT    rcCapImage;     /* calculated caption image rectangle */
    RECT    rcCapText;      /* calculated caption text rectangle */
    RECT    rcChild;        /* calculated child rectangle */
    RECT    rcChevron;      /* calculated chevron rectangle */

    LPWSTR    lpText;
    HWND    hwndPrevParent;
} REBAR_BAND;

/* fStatus flags */
#define HAS_GRIPPER    0x00000001
#define HAS_IMAGE      0x00000002
#define HAS_TEXT       0x00000004

/* fDraw flags */
#define DRAW_GRIPPER    0x00000001
#define DRAW_IMAGE      0x00000002
#define DRAW_TEXT       0x00000004
#define DRAW_CHEVRONHOT 0x00000040
#define DRAW_CHEVRONPUSHED 0x00000080
#define NTF_INVALIDATE  0x01000000

typedef struct
{
    COLORREF   clrBk;       /* background color */
    COLORREF   clrText;     /* text color */
    COLORREF   clrBtnText;  /* system color for BTNTEXT */
    COLORREF   clrBtnFace;  /* system color for BTNFACE */
    HIMAGELIST himl;        /* handle to imagelist */
    UINT     uNumBands;   /* # of bands in rebar (first=0, last=uNumBands-1 */
    UINT     uNumRows;    /* # of rows of bands (first=1, last=uNumRows */
    HWND     hwndSelf;    /* handle of REBAR window itself */
    HWND     hwndToolTip; /* handle to the tool tip control */
    HWND     hwndNotify;  /* notification window (parent) */
    HFONT    hDefaultFont;
    HFONT    hFont;       /* handle to the rebar's font */
    SIZE     imageSize;   /* image size (image list) */
    DWORD    dwStyle;     /* window style */
    DWORD    orgStyle;    /* original style (dwStyle may change) */
    SIZE     calcSize;    /* calculated rebar size - coordinates swapped for CCS_VERT */
    BOOL     bUnicode;    /* TRUE if parent wants notify in W format */
    BOOL     DoRedraw;    /* TRUE to acutally draw bands */
    UINT     fStatus;     /* Status flags (see below)  */
    HCURSOR  hcurArrow;   /* handle to the arrow cursor */
    HCURSOR  hcurHorz;    /* handle to the EW cursor */
    HCURSOR  hcurVert;    /* handle to the NS cursor */
    HCURSOR  hcurDrag;    /* handle to the drag cursor */
    INT      iVersion;    /* version number */
    POINT    dragStart;   /* x,y of button down */
    POINT    dragNow;     /* x,y of this MouseMove */
    INT      iOldBand;    /* last band that had the mouse cursor over it */
    INT      ihitoffset;  /* offset of hotspot from gripper.left */
    INT      ichevronhotBand; /* last band that had a hot chevron */
    INT      iGrabbedBand;/* band number of band whose gripper was grabbed */

    REBAR_BAND *bands;      /* pointer to the array of rebar bands */
} REBAR_INFO;

/* fStatus flags */
#define BEGIN_DRAG_ISSUED   0x00000001
#define AUTO_RESIZE         0x00000002
#define BAND_NEEDS_REDRAW   0x00000020

/* used by Windows to mark that the header size has been set by the user and shouldn't be changed */
#define RBBS_UNDOC_FIXEDHEADER 0x40000000

/* ----   REBAR layout constants. Mostly determined by        ---- */
/* ----   experiment on WIN 98.                               ---- */

/* Width (or height) of separators between bands (either horz. or  */
/* vert.). True only if RBS_BANDBORDERS is set                     */
#define SEP_WIDTH_SIZE  2
#define SEP_WIDTH       ((infoPtr->dwStyle & RBS_BANDBORDERS) ? SEP_WIDTH_SIZE : 0)

/* Blank (background color) space between Gripper (if present)     */
/* and next item (image, text, or window). Always present          */
#define REBAR_ALWAYS_SPACE  4

/* Blank (background color) space after Image (if present).        */
#define REBAR_POST_IMAGE  2

/* Blank (background color) space after Text (if present).         */
#define REBAR_POST_TEXT  4

/* Height of vertical gripper in a CCS_VERT rebar.                 */
#define GRIPPER_HEIGHT  16

/* Blank (background color) space before Gripper (if present).     */
#define REBAR_PRE_GRIPPER   2

/* Width (of normal vertical gripper) or height (of horz. gripper) */
/* if present.                                                     */
#define GRIPPER_WIDTH  3

/* Width of the chevron button if present */
#define CHEVRON_WIDTH  10

/* the gap between the child and the next band */
#define REBAR_POST_CHILD 4

/* Height of divider for Rebar if not disabled (CCS_NODIVIDER)     */
/* either top or bottom                                            */
#define REBAR_DIVIDER  2

/* height of a rebar without a child */
#define REBAR_NO_CHILD_HEIGHT 4

/* minimium vertical height of a normal bar                        */
/*   or minimum width of a CCS_VERT bar - from experiment on Win2k */
#define REBAR_MINSIZE  23

/* This is the increment that is used over the band height         */
#define REBARSPACE(a)     ((a->fStyle & RBBS_CHILDEDGE) ? 2*REBAR_DIVIDER : 0)

/* ----   End of REBAR layout constants.                      ---- */

#define RB_GETBANDINFO_OLD (WM_USER+5) /* obsoleted after IE3, but we have to support it anyway */

/*  The following define determines if a given band is hidden      */
#define HIDDENBAND(a)  (((a)->fStyle & RBBS_HIDDEN) ||   \
                        ((infoPtr->dwStyle & CCS_VERT) &&         \
                         ((a)->fStyle & RBBS_NOVERT)))

#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)GetWindowLongPtrW (hwnd, 0))

static LRESULT REBAR_NotifyFormat(REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam);


/* "constant values" retrieved when DLL was initialized    */
/* FIXME we do this when the classes are registered.       */
static UINT mindragx = 0;
static UINT mindragy = 0;

static const char * const band_stylename[] = {
    "RBBS_BREAK",              /* 0001 */
    "RBBS_FIXEDSIZE",          /* 0002 */
    "RBBS_CHILDEDGE",          /* 0004 */
    "RBBS_HIDDEN",             /* 0008 */
    "RBBS_NOVERT",             /* 0010 */
    "RBBS_FIXEDBMP",           /* 0020 */
    "RBBS_VARIABLEHEIGHT",     /* 0040 */
    "RBBS_GRIPPERALWAYS",      /* 0080 */
    "RBBS_NOGRIPPER",          /* 0100 */
    NULL };

static const char * const band_maskname[] = {
    "RBBIM_STYLE",         /*    0x00000001 */
    "RBBIM_COLORS",        /*    0x00000002 */
    "RBBIM_TEXT",          /*    0x00000004 */
    "RBBIM_IMAGE",         /*    0x00000008 */
    "RBBIM_CHILD",         /*    0x00000010 */
    "RBBIM_CHILDSIZE",     /*    0x00000020 */
    "RBBIM_SIZE",          /*    0x00000040 */
    "RBBIM_BACKGROUND",    /*    0x00000080 */
    "RBBIM_ID",            /*    0x00000100 */
    "RBBIM_IDEALSIZE",     /*    0x00000200 */
    "RBBIM_LPARAM",        /*    0x00000400 */
    "RBBIM_HEADERSIZE",    /*    0x00000800 */
    NULL };


static CHAR line[200];

static const WCHAR themeClass[] = { 'R','e','b','a','r',0 };

static CHAR *
REBAR_FmtStyle( UINT style)
{
    INT i = 0;

    *line = 0;
    while (band_stylename[i]) {
	if (style & (1<<i)) {
	    if (*line != 0) strcat(line, " | ");
	    strcat(line, band_stylename[i]);
	}
	i++;
    }
    return line;
}


static CHAR *
REBAR_FmtMask( UINT mask)
{
    INT i = 0;

    *line = 0;
    while (band_maskname[i]) {
	if (mask & (1<<i)) {
	    if (*line != 0) strcat(line, " | ");
	    strcat(line, band_maskname[i]);
	}
	i++;
    }
    return line;
}


static VOID
REBAR_DumpBandInfo(const REBARBANDINFOW *pB)
{
    if( !TRACE_ON(rebar) ) return;
    TRACE("band info: ");
    if (pB->fMask & RBBIM_ID)
        TRACE("ID=%u, ", pB->wID);
    TRACE("size=%u, child=%p", pB->cbSize, pB->hwndChild);
    if (pB->fMask & RBBIM_COLORS)
        TRACE(", clrF=0x%06x, clrB=0x%06x", pB->clrFore, pB->clrBack);
    TRACE("\n");

    TRACE("band info: mask=0x%08x (%s)\n", pB->fMask, REBAR_FmtMask(pB->fMask));
    if (pB->fMask & RBBIM_STYLE)
	TRACE("band info: style=0x%08x (%s)\n", pB->fStyle, REBAR_FmtStyle(pB->fStyle));
    if (pB->fMask & (RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_HEADERSIZE | RBBIM_LPARAM )) {
	TRACE("band info:");
	if (pB->fMask & RBBIM_SIZE)
	    TRACE(" cx=%u", pB->cx);
	if (pB->fMask & RBBIM_IDEALSIZE)
	    TRACE(" xIdeal=%u", pB->cxIdeal);
	if (pB->fMask & RBBIM_HEADERSIZE)
	    TRACE(" xHeader=%u", pB->cxHeader);
	if (pB->fMask & RBBIM_LPARAM)
	    TRACE(" lParam=0x%08lx", pB->lParam);
	TRACE("\n");
    }
    if (pB->fMask & RBBIM_CHILDSIZE)
	TRACE("band info: xMin=%u, yMin=%u, yChild=%u, yMax=%u, yIntgl=%u\n",
	      pB->cxMinChild,
	      pB->cyMinChild, pB->cyChild, pB->cyMaxChild, pB->cyIntegral);
}

static VOID
REBAR_DumpBand (const REBAR_INFO *iP)
{
    REBAR_BAND *pB;
    UINT i;

    if(! TRACE_ON(rebar) ) return;

    TRACE("hwnd=%p: color=%08x/%08x, bands=%u, rows=%u, cSize=%d,%d\n",
	  iP->hwndSelf, iP->clrText, iP->clrBk, iP->uNumBands, iP->uNumRows,
	  iP->calcSize.cx, iP->calcSize.cy);
    TRACE("hwnd=%p: flags=%08x, dragStart=%d,%d, dragNow=%d,%d, iGrabbedBand=%d\n",
	  iP->hwndSelf, iP->fStatus, iP->dragStart.x, iP->dragStart.y,
	  iP->dragNow.x, iP->dragNow.y,
	  iP->iGrabbedBand);
    TRACE("hwnd=%p: style=%08x, notify in Unicode=%s, redraw=%s\n",
          iP->hwndSelf, iP->dwStyle, (iP->bUnicode)?"TRUE":"FALSE",
          (iP->DoRedraw)?"TRUE":"FALSE");
    for (i = 0; i < iP->uNumBands; i++) {
	pB = &iP->bands[i];
	TRACE("band # %u:", i);
	if (pB->fMask & RBBIM_ID)
	    TRACE(" ID=%u", pB->wID);
	if (pB->fMask & RBBIM_CHILD)
	    TRACE(" child=%p", pB->hwndChild);
	if (pB->fMask & RBBIM_COLORS)
            TRACE(" clrF=0x%06x clrB=0x%06x", pB->clrFore, pB->clrBack);
	TRACE("\n");
	TRACE("band # %u: mask=0x%08x (%s)\n", i, pB->fMask, REBAR_FmtMask(pB->fMask));
	if (pB->fMask & RBBIM_STYLE)
	    TRACE("band # %u: style=0x%08x (%s)\n",
		  i, pB->fStyle, REBAR_FmtStyle(pB->fStyle));
	TRACE("band # %u: xHeader=%u",
	      i, pB->cxHeader);
	if (pB->fMask & (RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_LPARAM )) {
	    if (pB->fMask & RBBIM_SIZE)
		TRACE(" cx=%u", pB->cx);
	    if (pB->fMask & RBBIM_IDEALSIZE)
		TRACE(" xIdeal=%u", pB->cxIdeal);
	    if (pB->fMask & RBBIM_LPARAM)
		TRACE(" lParam=0x%08lx", pB->lParam);
	}
	TRACE("\n");
	if (RBBIM_CHILDSIZE)
	    TRACE("band # %u: xMin=%u, yMin=%u, yChild=%u, yMax=%u, yIntgl=%u\n",
		  i, pB->cxMinChild, pB->cyMinChild, pB->cyChild, pB->cyMaxChild, pB->cyIntegral);
	if (pB->fMask & RBBIM_TEXT)
	    TRACE("band # %u: text=%s\n",
		  i, (pB->lpText) ? debugstr_w(pB->lpText) : "(null)");
	TRACE("band # %u: lcx=%u, cxEffective=%u, lcy=%u\n",
	      i, pB->lcx, pB->cxEffective, pB->lcy);
	TRACE("band # %u: fStatus=%08x, fDraw=%08x, Band=(%d,%d)-(%d,%d), Grip=(%d,%d)-(%d,%d)\n",
	      i, pB->fStatus, pB->fDraw,
	      pB->rcBand.left, pB->rcBand.top, pB->rcBand.right, pB->rcBand.bottom,
	      pB->rcGripper.left, pB->rcGripper.top, pB->rcGripper.right, pB->rcGripper.bottom);
	TRACE("band # %u: Img=(%d,%d)-(%d,%d), Txt=(%d,%d)-(%d,%d), Child=(%d,%d)-(%d,%d)\n",
	      i,
	      pB->rcCapImage.left, pB->rcCapImage.top, pB->rcCapImage.right, pB->rcCapImage.bottom,
	      pB->rcCapText.left, pB->rcCapText.top, pB->rcCapText.right, pB->rcCapText.bottom,
	      pB->rcChild.left, pB->rcChild.top, pB->rcChild.right, pB->rcChild.bottom);
    }

}

/* dest can be equal to src */
static void translate_rect(const REBAR_INFO *infoPtr, RECT *dest, const RECT *src)
{
    if (infoPtr->dwStyle & CCS_VERT) {
        int tmp;
        tmp = src->left;
        dest->left = src->top;
        dest->top = tmp;

        tmp = src->right;
        dest->right = src->bottom;
        dest->bottom = tmp;
    } else {
        *dest = *src;
    }
}

static int get_rect_cx(const REBAR_INFO *infoPtr, const RECT *lpRect)
{
    if (infoPtr->dwStyle & CCS_VERT)
        return lpRect->bottom - lpRect->top;
    return lpRect->right - lpRect->left;
}

static int get_rect_cy(const REBAR_INFO *infoPtr, const RECT *lpRect)
{
    if (infoPtr->dwStyle & CCS_VERT)
        return lpRect->right - lpRect->left;
    return lpRect->bottom - lpRect->top;
}

static void round_child_height(REBAR_BAND *lpBand, int cyHeight)
{
    int cy = 0;
    if (lpBand->cyIntegral == 0)
        return;
    cy = max(cyHeight - (int)lpBand->cyMinChild, 0);
    cy = lpBand->cyMinChild + (cy/lpBand->cyIntegral) * lpBand->cyIntegral;
    cy = min(cy, lpBand->cyMaxChild);
    lpBand->cyChild = cy;
}

static void
REBAR_DrawChevron (HDC hdc, INT left, INT top, INT colorRef)
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

static HWND
REBAR_GetNotifyParent (const REBAR_INFO *infoPtr)
{
    HWND parent, owner;

    parent = infoPtr->hwndNotify;
    if (!parent) {
        parent = GetParent (infoPtr->hwndSelf);
	owner = GetWindow (infoPtr->hwndSelf, GW_OWNER);
	if (owner) parent = owner;
    }
    return parent;
}


static INT
REBAR_Notify (NMHDR *nmhdr, const REBAR_INFO *infoPtr, UINT code)
{
    HWND parent;

    parent = REBAR_GetNotifyParent (infoPtr);
    nmhdr->idFrom = GetDlgCtrlID (infoPtr->hwndSelf);
    nmhdr->hwndFrom = infoPtr->hwndSelf;
    nmhdr->code = code;

    TRACE("window %p, code=%08x, via %s\n", parent, code, (infoPtr->bUnicode)?"Unicode":"ANSI");

    return SendMessageW(parent, WM_NOTIFY, (WPARAM)nmhdr->idFrom, (LPARAM)nmhdr);
}

static INT
REBAR_Notify_NMREBAR (const REBAR_INFO *infoPtr, UINT uBand, UINT code)
{
    NMREBAR notify_rebar;
    REBAR_BAND *lpBand;

    notify_rebar.dwMask = 0;
    if (uBand!=-1) {
	lpBand = &infoPtr->bands[uBand];
	if (lpBand->fMask & RBBIM_ID) {
	    notify_rebar.dwMask |= RBNM_ID;
	    notify_rebar.wID = lpBand->wID;
	}
	if (lpBand->fMask & RBBIM_LPARAM) {
	    notify_rebar.dwMask |= RBNM_LPARAM;
	    notify_rebar.lParam = lpBand->lParam;
	}
	if (lpBand->fMask & RBBIM_STYLE) {
	    notify_rebar.dwMask |= RBNM_STYLE;
	    notify_rebar.fStyle = lpBand->fStyle;
	}
    }
    notify_rebar.uBand = uBand;
    return REBAR_Notify ((NMHDR *)&notify_rebar, infoPtr, code);
}

static VOID
REBAR_DrawBand (HDC hdc, const REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{
    HFONT hOldFont = 0;
    INT oldBkMode = 0;
    NMCUSTOMDRAW nmcd;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    RECT rcBand;

    translate_rect(infoPtr, &rcBand, &lpBand->rcBand);

    if (lpBand->fDraw & DRAW_TEXT) {
	hOldFont = SelectObject (hdc, infoPtr->hFont);
	oldBkMode = SetBkMode (hdc, TRANSPARENT);
    }

    /* should test for CDRF_NOTIFYITEMDRAW here */
    nmcd.dwDrawStage = CDDS_ITEMPREPAINT;
    nmcd.hdc = hdc;
    nmcd.rc = rcBand;
    nmcd.rc.right = lpBand->rcCapText.right;
    nmcd.rc.bottom = lpBand->rcCapText.bottom;
    nmcd.dwItemSpec = lpBand->wID;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = lpBand->lParam;
    lpBand->uCDret = REBAR_Notify ((NMHDR *)&nmcd, infoPtr, NM_CUSTOMDRAW);
    if (lpBand->uCDret == CDRF_SKIPDEFAULT) {
	if (oldBkMode != TRANSPARENT)
	    SetBkMode (hdc, oldBkMode);
	SelectObject (hdc, hOldFont);
	return;
    }

    /* draw gripper */
    if (lpBand->fDraw & DRAW_GRIPPER)
    {
        if (theme)
        {
            RECT rcGripper = lpBand->rcGripper;
            int partId = (infoPtr->dwStyle & CCS_VERT) ? RP_GRIPPERVERT : RP_GRIPPER;
            GetThemeBackgroundExtent (theme, hdc, partId, 0, &rcGripper, &rcGripper);
            OffsetRect (&rcGripper, lpBand->rcGripper.left - rcGripper.left,
                lpBand->rcGripper.top - rcGripper.top);
            DrawThemeBackground (theme, hdc, partId, 0, &rcGripper, NULL);
        }
        else
            DrawEdge (hdc, &lpBand->rcGripper, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
    }

    /* draw caption image */
    if (lpBand->fDraw & DRAW_IMAGE) {
	POINT pt;

	/* center image */
	pt.y = (lpBand->rcCapImage.bottom + lpBand->rcCapImage.top - infoPtr->imageSize.cy)/2;
	pt.x = (lpBand->rcCapImage.right + lpBand->rcCapImage.left - infoPtr->imageSize.cx)/2;

	ImageList_Draw (infoPtr->himl, lpBand->iImage, hdc,
			pt.x, pt.y,
			ILD_TRANSPARENT);
    }

    /* draw caption text */
    if (lpBand->fDraw & DRAW_TEXT) {
	/* need to handle CDRF_NEWFONT here */
	INT oldBkMode = SetBkMode (hdc, TRANSPARENT);
	COLORREF oldcolor = CLR_NONE;
	COLORREF new;
	if (lpBand->clrFore != CLR_NONE) {
	    new = (lpBand->clrFore == CLR_DEFAULT) ? infoPtr->clrBtnText :
		    lpBand->clrFore;
	    oldcolor = SetTextColor (hdc, new);
	}
	DrawTextW (hdc, lpBand->lpText, -1, &lpBand->rcCapText,
		   DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (oldBkMode != TRANSPARENT)
	    SetBkMode (hdc, oldBkMode);
	if (lpBand->clrFore != CLR_NONE)
	    SetTextColor (hdc, oldcolor);
	SelectObject (hdc, hOldFont);
    }

    if (!IsRectEmpty(&lpBand->rcChevron))
    {
        if (theme)
        {
            int stateId;
            if (lpBand->fDraw & DRAW_CHEVRONPUSHED)
                stateId = CHEVS_PRESSED;
            else if (lpBand->fDraw & DRAW_CHEVRONHOT)
                stateId = CHEVS_HOT;
            else
                stateId = CHEVS_NORMAL;
            DrawThemeBackground (theme, hdc, RP_CHEVRON, stateId, &lpBand->rcChevron, NULL);
        }
        else
        {
            if (lpBand->fDraw & DRAW_CHEVRONPUSHED)
            {
                DrawEdge(hdc, &lpBand->rcChevron, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);
                REBAR_DrawChevron(hdc, lpBand->rcChevron.left+1, lpBand->rcChevron.top + 11, COLOR_WINDOWFRAME);
            }
            else if (lpBand->fDraw & DRAW_CHEVRONHOT)
            {
                DrawEdge(hdc, &lpBand->rcChevron, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
                REBAR_DrawChevron(hdc, lpBand->rcChevron.left, lpBand->rcChevron.top + 10, COLOR_WINDOWFRAME);
            }
            else
                REBAR_DrawChevron(hdc, lpBand->rcChevron.left, lpBand->rcChevron.top + 10, COLOR_WINDOWFRAME);
        }
    }

    if (lpBand->uCDret == (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW)) {
	nmcd.dwDrawStage = CDDS_ITEMPOSTPAINT;
	nmcd.hdc = hdc;
	nmcd.rc = rcBand;
	nmcd.rc.right = lpBand->rcCapText.right;
	nmcd.rc.bottom = lpBand->rcCapText.bottom;
	nmcd.dwItemSpec = lpBand->wID;
	nmcd.uItemState = 0;
	nmcd.lItemlParam = lpBand->lParam;
	lpBand->uCDret = REBAR_Notify ((NMHDR *)&nmcd, infoPtr, NM_CUSTOMDRAW);
    }
}


static VOID
REBAR_Refresh (const REBAR_INFO *infoPtr, HDC hdc)
{
    REBAR_BAND *lpBand;
    UINT i;

    if (!infoPtr->DoRedraw) return;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if (HIDDENBAND(lpBand)) continue;

	/* now draw the band */
	TRACE("[%p] drawing band %i, flags=%08x\n",
	      infoPtr->hwndSelf, i, lpBand->fDraw);
	REBAR_DrawBand (hdc, infoPtr, lpBand);

    }
}


static void
REBAR_CalcHorzBand (const REBAR_INFO *infoPtr, UINT rstart, UINT rend)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Horizontal bars. ***                   */
{
    REBAR_BAND *lpBand;
    UINT i, xoff, yoff;
    RECT work;

    for(i=rstart; i<rend; i++){
      lpBand = &infoPtr->bands[i];
      if (HIDDENBAND(lpBand)) {
          SetRect (&lpBand->rcChild,
		   lpBand->rcBand.right, lpBand->rcBand.top,
		   lpBand->rcBand.right, lpBand->rcBand.bottom);
	  continue;
      }

      /* set initial gripper rectangle */
      SetRect (&lpBand->rcGripper, lpBand->rcBand.left, lpBand->rcBand.top,
	       lpBand->rcBand.left, lpBand->rcBand.bottom);

      /* calculate gripper rectangle */
      if ( lpBand->fStatus & HAS_GRIPPER) {
	  lpBand->fDraw |= DRAW_GRIPPER;
	  lpBand->rcGripper.left   += REBAR_PRE_GRIPPER;
	  lpBand->rcGripper.right  = lpBand->rcGripper.left + GRIPPER_WIDTH;
	  lpBand->rcGripper.top    += 2;
	  lpBand->rcGripper.bottom -= 2;

	  SetRect (&lpBand->rcCapImage,
		   lpBand->rcGripper.right+REBAR_ALWAYS_SPACE, lpBand->rcBand.top,
		   lpBand->rcGripper.right+REBAR_ALWAYS_SPACE, lpBand->rcBand.bottom);
      }
      else {  /* no gripper will be drawn */
	  xoff = 0;
	  if (lpBand->fStatus & (HAS_IMAGE | HAS_TEXT))
	      /* if no gripper but either image or text, then leave space */
	      xoff = REBAR_ALWAYS_SPACE;
	  SetRect (&lpBand->rcCapImage,
		   lpBand->rcBand.left+xoff, lpBand->rcBand.top,
		   lpBand->rcBand.left+xoff, lpBand->rcBand.bottom);
      }

      /* image is visible */
      if (lpBand->fStatus & HAS_IMAGE) {
	  lpBand->fDraw |= DRAW_IMAGE;
	  lpBand->rcCapImage.right  += infoPtr->imageSize.cx;
	  lpBand->rcCapImage.bottom = lpBand->rcCapImage.top + infoPtr->imageSize.cy;

	  /* set initial caption text rectangle */
	  SetRect (&lpBand->rcCapText,
		   lpBand->rcCapImage.right+REBAR_POST_IMAGE, lpBand->rcBand.top+1,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.bottom-1);
      }
      else {
	  /* set initial caption text rectangle */
	  SetRect (&lpBand->rcCapText, lpBand->rcCapImage.right, lpBand->rcBand.top+1,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.bottom-1);
      }

      /* text is visible */
      if ((lpBand->fStatus & HAS_TEXT) && !(lpBand->fStyle & RBBS_HIDETITLE)) {
	  lpBand->fDraw |= DRAW_TEXT;
	  lpBand->rcCapText.right = max(lpBand->rcCapText.left,
					lpBand->rcCapText.right-REBAR_POST_TEXT);
      }

      /* set initial child window rectangle if there is a child */
      if (lpBand->hwndChild != NULL) {
          int cyBand = lpBand->rcBand.bottom - lpBand->rcBand.top;
          yoff = (cyBand - lpBand->cyChild) / 2;
	  SetRect (&lpBand->rcChild,
                   lpBand->rcBand.left + lpBand->cxHeader, lpBand->rcBand.top + yoff,
                   lpBand->rcBand.right - REBAR_POST_CHILD, lpBand->rcBand.top + yoff + lpBand->cyChild);
	  if ((lpBand->fStyle & RBBS_USECHEVRON) && (lpBand->rcChild.right - lpBand->rcChild.left < lpBand->cxIdeal))
	  {
	      lpBand->rcChild.right -= CHEVRON_WIDTH;
	      SetRect(&lpBand->rcChevron, lpBand->rcChild.right,
	              lpBand->rcChild.top, lpBand->rcChild.right + CHEVRON_WIDTH,
	              lpBand->rcChild.bottom);
	  }
      }
      else {
          SetRect (&lpBand->rcChild,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.top,
		   lpBand->rcBand.right, lpBand->rcBand.bottom);
      }

      /* flag if notify required and invalidate rectangle */
      if (lpBand->fDraw & NTF_INVALIDATE) {
          TRACE("invalidating (%d,%d)-(%d,%d)\n",
		lpBand->rcBand.left,
		lpBand->rcBand.top,
		lpBand->rcBand.right + SEP_WIDTH,
		lpBand->rcBand.bottom + SEP_WIDTH);
	  lpBand->fDraw &= ~NTF_INVALIDATE;
	  work = lpBand->rcBand;
	  work.right += SEP_WIDTH;
	  work.bottom += SEP_WIDTH;
	  InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
      }

    }

}


static VOID
REBAR_CalcVertBand (const REBAR_INFO *infoPtr, UINT rstart, UINT rend)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Vertical bars. ***                     */
{
    REBAR_BAND *lpBand;
    UINT i, xoff;
    RECT work;

    for(i=rstart; i<rend; i++){
        RECT rcBand;
        lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;

        translate_rect(infoPtr, &rcBand, &lpBand->rcBand);

        /* set initial gripper rectangle */
	SetRect (&lpBand->rcGripper, rcBand.left, rcBand.top, rcBand.right, rcBand.top);

	/* calculate gripper rectangle */
	if (lpBand->fStatus & HAS_GRIPPER) {
	    lpBand->fDraw |= DRAW_GRIPPER;

	    if (infoPtr->dwStyle & RBS_VERTICALGRIPPER) {
		/*  vertical gripper  */
		lpBand->rcGripper.left   += 3;
		lpBand->rcGripper.right  = lpBand->rcGripper.left + GRIPPER_WIDTH;
		lpBand->rcGripper.top    += REBAR_PRE_GRIPPER;
		lpBand->rcGripper.bottom = lpBand->rcGripper.top + GRIPPER_HEIGHT;

		/* initialize Caption image rectangle  */
		SetRect (&lpBand->rcCapImage, rcBand.left,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE,
			 rcBand.right,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE);
	    }
	    else {
		/*  horizontal gripper  */
		lpBand->rcGripper.left   += 2;
		lpBand->rcGripper.right  -= 2;
		lpBand->rcGripper.top    += REBAR_PRE_GRIPPER;
		lpBand->rcGripper.bottom  = lpBand->rcGripper.top + GRIPPER_WIDTH;

		/* initialize Caption image rectangle  */
		SetRect (&lpBand->rcCapImage, rcBand.left,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE,
			 rcBand.right,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE);
	    }
	}
	else {  /* no gripper will be drawn */
	    xoff = 0;
	    if (lpBand->fStatus & (HAS_IMAGE | HAS_TEXT))
		/* if no gripper but either image or text, then leave space */
		xoff = REBAR_ALWAYS_SPACE;
	    /* initialize Caption image rectangle  */
	    SetRect (&lpBand->rcCapImage,
                      rcBand.left, rcBand.top+xoff,
                      rcBand.right, rcBand.top+xoff);
	}

	/* image is visible */
	if (lpBand->fStatus & HAS_IMAGE) {
	    lpBand->fDraw |= DRAW_IMAGE;

	    lpBand->rcCapImage.right  = lpBand->rcCapImage.left + infoPtr->imageSize.cx;
	    lpBand->rcCapImage.bottom += infoPtr->imageSize.cy;

	    /* set initial caption text rectangle */
	    SetRect (&lpBand->rcCapText,
		     rcBand.left, lpBand->rcCapImage.bottom+REBAR_POST_IMAGE,
		     rcBand.right, rcBand.top+lpBand->cxHeader);
	}
	else {
	    /* set initial caption text rectangle */
	    SetRect (&lpBand->rcCapText,
		     rcBand.left, lpBand->rcCapImage.bottom,
		     rcBand.right, rcBand.top+lpBand->cxHeader);
	}

	/* text is visible */
	if ((lpBand->fStatus & HAS_TEXT) && !(lpBand->fStyle & RBBS_HIDETITLE)) {
	    lpBand->fDraw |= DRAW_TEXT;
	    lpBand->rcCapText.bottom = max(lpBand->rcCapText.top,
					   lpBand->rcCapText.bottom);
	}

	/* set initial child window rectangle if there is a child */
	if (lpBand->hwndChild != NULL) {
            int cxBand = rcBand.right - rcBand.left;
            xoff = (cxBand - lpBand->cyChild) / 2;
	    SetRect (&lpBand->rcChild,
		     rcBand.left + xoff,                   rcBand.top + lpBand->cxHeader,
                     rcBand.left + xoff + lpBand->cyChild, rcBand.bottom - REBAR_POST_CHILD);
	}
	else {
	    SetRect (&lpBand->rcChild,
		     rcBand.left, rcBand.top+lpBand->cxHeader,
		     rcBand.right, rcBand.bottom);
	}

	if (lpBand->fDraw & NTF_INVALIDATE) {
            TRACE("invalidating (%d,%d)-(%d,%d)\n",
                  rcBand.left,
                  rcBand.top,
                  rcBand.right + SEP_WIDTH,
                  rcBand.bottom + SEP_WIDTH);
	    lpBand->fDraw &= ~NTF_INVALIDATE;
	    work = rcBand;
	    work.bottom += SEP_WIDTH;
	    work.right += SEP_WIDTH;
	    InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
	}

    }
}


static VOID
REBAR_ForceResize (REBAR_INFO *infoPtr)
     /* Function: This changes the size of the REBAR window to that */
     /*  calculated by REBAR_Layout.                                */
{
    INT x, y, width, height;
    INT xedge = 0, yedge = 0;
    RECT rcSelf;

    TRACE("new size [%d x %d]\n", infoPtr->calcSize.cx, infoPtr->calcSize.cy);

    if (infoPtr->dwStyle & CCS_NORESIZE)
        return;

    if (infoPtr->dwStyle & WS_BORDER)
    {
        xedge = GetSystemMetrics(SM_CXEDGE);
        yedge = GetSystemMetrics(SM_CYEDGE);
        /* swap for CCS_VERT? */
    }

    /* compute rebar window rect in parent client coordinates */
    GetWindowRect(infoPtr->hwndSelf, &rcSelf);
    MapWindowPoints(HWND_DESKTOP, GetParent(infoPtr->hwndSelf), (LPPOINT)&rcSelf, 2);
    translate_rect(infoPtr, &rcSelf, &rcSelf);

    height = infoPtr->calcSize.cy + 2*yedge;
    if (!(infoPtr->dwStyle & CCS_NOPARENTALIGN)) {
        RECT rcParent;

        x = -xedge;
        width = infoPtr->calcSize.cx + 2*xedge;
        y = 0; /* quiet compiler warning */
        switch ( infoPtr->dwStyle & CCS_LAYOUT_MASK) {
            case 0:     /* shouldn't happen - see NCCreate */
            case CCS_TOP:
                y = ((infoPtr->dwStyle & CCS_NODIVIDER) ? 0 : REBAR_DIVIDER) - yedge;
                break;
            case CCS_NOMOVEY:
                y = rcSelf.top;
                break;
            case CCS_BOTTOM:
                GetClientRect(GetParent(infoPtr->hwndSelf), &rcParent);
                translate_rect(infoPtr, &rcParent, &rcParent);
                y = rcParent.bottom - infoPtr->calcSize.cy - yedge;
                break;
	}
    }
    else {
	x = rcSelf.left;
	/* As on Windows if the CCS_NODIVIDER is not present the control will move
	 * 2 pixel down after every layout */
	y = rcSelf.top + ((infoPtr->dwStyle & CCS_NODIVIDER) ? 0 : REBAR_DIVIDER);
	width = rcSelf.right - rcSelf.left;
    }

    TRACE("hwnd %p, style=%08x, setting at (%d,%d) for (%d,%d)\n",
	infoPtr->hwndSelf, infoPtr->dwStyle, x, y, width, height);

    /* Set flag to ignore next WM_SIZE message and resize the window */
    infoPtr->fStatus |= AUTO_RESIZE;
    if ((infoPtr->dwStyle & CCS_VERT) == 0)
        SetWindowPos(infoPtr->hwndSelf, 0, x, y, width, height, SWP_NOZORDER);
    else
        SetWindowPos(infoPtr->hwndSelf, 0, y, x, height, width, SWP_NOZORDER);
    infoPtr->fStatus &= ~AUTO_RESIZE;
}


static VOID
REBAR_MoveChildWindows (const REBAR_INFO *infoPtr, UINT start, UINT endplus)
{
    static const WCHAR strComboBox[] = { 'C','o','m','b','o','B','o','x',0 };
    REBAR_BAND *lpBand;
    WCHAR szClassName[40];
    UINT i;
    NMREBARCHILDSIZE  rbcz;
    HDWP deferpos;

    if (!(deferpos = BeginDeferWindowPos(infoPtr->uNumBands)))
        ERR("BeginDeferWindowPos returned NULL\n");

    for (i = start; i < endplus; i++) {
	lpBand = &infoPtr->bands[i];

	if (HIDDENBAND(lpBand)) continue;
	if (lpBand->hwndChild) {
	    TRACE("hwndChild = %p\n", lpBand->hwndChild);

	    /* Always geterate the RBN_CHILDSIZE even it child
		   did not change */
	    rbcz.uBand = i;
	    rbcz.wID = lpBand->wID;
	    rbcz.rcChild = lpBand->rcChild;
            translate_rect(infoPtr, &rbcz.rcBand, &lpBand->rcBand);
	    if (infoPtr->dwStyle & CCS_VERT)
		rbcz.rcBand.top += lpBand->cxHeader;
	    else
		rbcz.rcBand.left += lpBand->cxHeader;
	    REBAR_Notify ((NMHDR *)&rbcz, infoPtr, RBN_CHILDSIZE);
	    if (!EqualRect (&lpBand->rcChild, &rbcz.rcChild)) {
		TRACE("Child rect changed by NOTIFY for band %u\n", i);
		TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
		      lpBand->rcChild.left, lpBand->rcChild.top,
		      lpBand->rcChild.right, lpBand->rcChild.bottom,
		      rbcz.rcChild.left, rbcz.rcChild.top,
		      rbcz.rcChild.right, rbcz.rcChild.bottom);
		lpBand->rcChild = rbcz.rcChild;  /* *** ??? */
            }

	    /* native (IE4 in "Favorites" frame **1) does:
	     *   SetRect (&rc, -1, -1, -1, -1)
	     *   EqualRect (&rc,band->rc???)
	     *   if ret==0
	     *     CopyRect (band->rc????, &rc)
	     *     set flag outside of loop
	     */

	    GetClassNameW (lpBand->hwndChild, szClassName, sizeof(szClassName)/sizeof(szClassName[0]));
	    if (!lstrcmpW (szClassName, strComboBox) ||
		!lstrcmpW (szClassName, WC_COMBOBOXEXW)) {
		INT nEditHeight, yPos;
		RECT rc;

		/* special placement code for combo or comboex box */


		/* get size of edit line */
		GetWindowRect (lpBand->hwndChild, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		TRACE("moving child (Combo(Ex)) %p to (%d,%d) for (%d,%d)\n",
		      lpBand->hwndChild,
		      lpBand->rcChild.left, yPos,
		      lpBand->rcChild.right - lpBand->rcChild.left,
		      nEditHeight);
		deferpos = DeferWindowPos (deferpos, lpBand->hwndChild, HWND_TOP,
					   lpBand->rcChild.left,
					   /*lpBand->rcChild.top*/ yPos,
					   lpBand->rcChild.right - lpBand->rcChild.left,
					   nEditHeight,
					   SWP_NOZORDER);
		if (!deferpos)
		    ERR("DeferWindowPos returned NULL\n");
	    }
	    else {
		TRACE("moving child (Other) %p to (%d,%d) for (%d,%d)\n",
		      lpBand->hwndChild,
		      lpBand->rcChild.left, lpBand->rcChild.top,
		      lpBand->rcChild.right - lpBand->rcChild.left,
		      lpBand->rcChild.bottom - lpBand->rcChild.top);
		deferpos = DeferWindowPos (deferpos, lpBand->hwndChild, HWND_TOP,
					   lpBand->rcChild.left,
					   lpBand->rcChild.top,
					   lpBand->rcChild.right - lpBand->rcChild.left,
					   lpBand->rcChild.bottom - lpBand->rcChild.top,
					   SWP_NOZORDER);
		if (!deferpos)
		    ERR("DeferWindowPos returned NULL\n");
	    }
	}
    }
    if (!EndDeferWindowPos(deferpos))
        ERR("EndDeferWindowPos returned NULL\n");

    if (infoPtr->DoRedraw)
	UpdateWindow (infoPtr->hwndSelf);

    /* native (from **1 above) does:
     *      UpdateWindow(rebar)
     *      REBAR_ForceResize
     *      RBN_HEIGHTCHANGE if necessary
     *      if ret from any EqualRect was 0
     *         Goto "BeginDeferWindowPos"
     */

}

static int next_band(const REBAR_INFO *infoPtr, int i)
{
    int n;
    for (n = i + 1; n < infoPtr->uNumBands; n++)
        if (!HIDDENBAND(&infoPtr->bands[n]))
            break;
    return n;
}

static int prev_band(const REBAR_INFO *infoPtr, int i)
{
    int n;
    for (n = i - 1; n >= 0; n--)
        if (!HIDDENBAND(&infoPtr->bands[n]))
            break;
    return n;
}

static int get_row_begin_for_band(const REBAR_INFO *infoPtr, INT iBand)
{
    int iLastBand = iBand;
    int iRow = infoPtr->bands[iBand].iRow;
    while ((iBand = prev_band(infoPtr, iBand)) >= 0) {
        if (infoPtr->bands[iBand].iRow != iRow)
            break;
        else
            iLastBand = iBand;
    }
    return iLastBand;
}

static int get_row_end_for_band(const REBAR_INFO *infoPtr, INT iBand)
{
    int iRow = infoPtr->bands[iBand].iRow;
    while ((iBand = next_band(infoPtr, iBand)) < infoPtr->uNumBands)
        if (infoPtr->bands[iBand].iRow != iRow)
            break;
    return iBand;
}

static void REBAR_SetRowRectsX(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand)
{
    int xPos = 0, i;
    for (i = iBeginBand; i < iEndBand; i = next_band(infoPtr, i))
    {
        REBAR_BAND *lpBand = &infoPtr->bands[i];

        lpBand = &infoPtr->bands[i];
        if (lpBand->rcBand.left != xPos || lpBand->rcBand.right != xPos + lpBand->cxEffective) {
            lpBand->fDraw |= NTF_INVALIDATE;
            TRACE("Setting rect %d to %d,%d\n", i, xPos, xPos + lpBand->cxEffective);
            lpBand->rcBand.left = xPos;
            lpBand->rcBand.right = xPos + lpBand->cxEffective;
        }
        xPos += lpBand->cxEffective + SEP_WIDTH;
    }
}

/* The rationale of this function is probably as follows: if we have some space
 * to distribute we want to add it to a band on the right. However we don't want
 * to unminimize a minimized band so we search for a band that is big enough.
 * For some reason "big enough" is defined as bigger than the minimum size of the
 * first band in the row
 */
static REBAR_BAND *REBAR_FindBandToGrow(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand)
{
    INT iLcx = 0, i;

    iLcx = infoPtr->bands[iBeginBand].lcx;

    for (i = prev_band(infoPtr, iEndBand); i >= iBeginBand; i = prev_band(infoPtr, i))
        if (infoPtr->bands[i].cxEffective > iLcx && !(infoPtr->bands[i].fStyle&RBBS_FIXEDSIZE))
            break;

    if (i < iBeginBand)
        for (i = prev_band(infoPtr, iEndBand); i >= iBeginBand; i = prev_band(infoPtr, i))
            if (infoPtr->bands[i].lcx == iLcx)
                break;

    TRACE("Extra space for row [%d..%d) should be added to band %d\n", iBeginBand, iEndBand, i);
    return &infoPtr->bands[i];
}

static int REBAR_ShrinkBandsRTL(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand, INT cxShrink, BOOL bEnforce)
{
    REBAR_BAND *lpBand;
    INT width, i;

    TRACE("Shrinking bands [%d..%d) by %d, right-to-left\n", iBeginBand, iEndBand, cxShrink);
    for (i = prev_band(infoPtr, iEndBand); i >= iBeginBand; i = prev_band(infoPtr, i))
    {
        lpBand = &infoPtr->bands[i];

        width = max(lpBand->cxEffective - cxShrink, (int)lpBand->lcx);
        cxShrink -= lpBand->cxEffective - width;
        lpBand->cxEffective = width;
        if (bEnforce && lpBand->cx > lpBand->cxEffective)
            lpBand->cx = lpBand->cxEffective;
        if (cxShrink == 0)
            break;
    }
    return cxShrink;
}


static int REBAR_ShrinkBandsLTR(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand, INT cxShrink, BOOL bEnforce)
{
    REBAR_BAND *lpBand;
    INT width, i;

    TRACE("Shrinking bands [%d..%d) by %d, left-to-right\n", iBeginBand, iEndBand, cxShrink);
    for (i = iBeginBand; i < iEndBand; i = next_band(infoPtr, i))
    {
        lpBand = &infoPtr->bands[i];

        width = max(lpBand->cxEffective - cxShrink, (int)lpBand->lcx);
        cxShrink -= lpBand->cxEffective - width;
        lpBand->cxEffective = width;
        if (bEnforce)
            lpBand->cx = lpBand->cxEffective;
        if (cxShrink == 0)
            break;
    }
    return cxShrink;
}

static int REBAR_SetBandsHeight(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand, INT yStart)
{
    REBAR_BAND *lpBand;
    int yMaxHeight = 0;
    int yPos = yStart;
    int row = infoPtr->bands[iBeginBand].iRow;
    int i;
    for (i = iBeginBand; i < iEndBand; i = next_band(infoPtr, i))
    {
        lpBand = &infoPtr->bands[i];
        yMaxHeight = max(yMaxHeight, lpBand->lcy);
    }
    TRACE("Bands [%d; %d) height: %d\n", iBeginBand, iEndBand, yMaxHeight);

    for (i = iBeginBand; i < iEndBand; i = next_band(infoPtr, i))
    {
        lpBand = &infoPtr->bands[i];
        /* we may be called for multiple rows if RBS_VARHEIGHT not set */
        if (lpBand->iRow != row) {
            yPos += yMaxHeight + SEP_WIDTH;
            row = lpBand->iRow;
        }

        if (lpBand->rcBand.top != yPos || lpBand->rcBand.bottom != yPos + yMaxHeight) {
            lpBand->fDraw |= NTF_INVALIDATE;
            lpBand->rcBand.top = yPos;
            lpBand->rcBand.bottom = yPos + yMaxHeight;
            TRACE("Band %d: %s\n", i, wine_dbgstr_rect(&lpBand->rcBand));
        }
    }
    return yPos + yMaxHeight;
}

static void REBAR_LayoutRow(const REBAR_INFO *infoPtr, int iBeginBand, int iEndBand, int cx, int *piRow, int *pyPos)
{
    REBAR_BAND *lpBand;
    int i, extra;
    int width = 0;

    TRACE("Adjusting row [%d;%d). Width: %d\n", iBeginBand, iEndBand, cx);
    for (i = iBeginBand; i < iEndBand; i++)
        infoPtr->bands[i].iRow = *piRow;

    /* compute the extra space */
    for (i = iBeginBand; i < iEndBand; i = next_band(infoPtr, i))
    {
        lpBand = &infoPtr->bands[i];
        if (i > iBeginBand)
            width += SEP_WIDTH;
        lpBand->cxEffective = max(lpBand->lcx, lpBand->cx);
        width += lpBand->cxEffective;
    }

    extra = cx - width;
    TRACE("Extra space: %d\n", extra);
    if (extra < 0) {
        int ret = REBAR_ShrinkBandsRTL(infoPtr, iBeginBand, iEndBand, -extra, FALSE);
        if (ret > 0 && next_band(infoPtr, iBeginBand) != iEndBand)  /* one band may be longer than expected... */
            ERR("Error layouting row %d - couldn't shrink for %d pixels (%d total shrink)\n", *piRow, ret, -extra);
    } else
    if (extra > 0) {
        lpBand = REBAR_FindBandToGrow(infoPtr, iBeginBand, iEndBand);
        lpBand->cxEffective += extra;
    }

    REBAR_SetRowRectsX(infoPtr, iBeginBand, iEndBand);
    if (infoPtr->dwStyle & RBS_VARHEIGHT)
    {
        if (*piRow > 0)
            *pyPos += SEP_WIDTH;
        *pyPos = REBAR_SetBandsHeight(infoPtr, iBeginBand, iEndBand, *pyPos);
    }
    (*piRow)++;
}

static VOID
REBAR_Layout(REBAR_INFO *infoPtr, const RECT *lpRect)
{
    REBAR_BAND *lpBand;
    RECT rcAdj;
    SIZE oldSize;
    INT adjcx, adjcy, i;
    INT rowstart = 0;
    INT row = 0;
    INT xMin, yPos;
    INT cyTarget;
    const INT yInit = 0;

    cyTarget = 0;
    if (lpRect) {
        rcAdj = *lpRect;
        cyTarget = get_rect_cy(infoPtr, lpRect);
    } else if (infoPtr->dwStyle & (CCS_NORESIZE | CCS_NOPARENTALIGN) || GetParent(infoPtr->hwndSelf) == NULL)
        GetClientRect(infoPtr->hwndSelf, &rcAdj);
    else
        GetClientRect(GetParent(infoPtr->hwndSelf), &rcAdj);
    TRACE("adjustment rect is (%d,%d)-(%d,%d)\n", rcAdj.left, rcAdj.top, rcAdj.right, rcAdj.bottom);

    adjcx = get_rect_cx(infoPtr, &rcAdj);
    adjcy = get_rect_cy(infoPtr, &rcAdj);

    if (infoPtr->uNumBands == 0) {
        TRACE("No bands - setting size to (0,%d), vert: %lx\n", adjcx, infoPtr->dwStyle & CCS_VERT);
        infoPtr->calcSize.cx = adjcx;
        /* the calcSize.cy won't change for a 0 band rebar */
        infoPtr->uNumRows = 0;
        REBAR_ForceResize(infoPtr);
        return;
    }

    yPos = yInit;
    xMin = 0;
    /* divide rows */
    i = 0;
    for (i = 0; i < infoPtr->uNumBands; i++)
    {
        lpBand = &infoPtr->bands[i];
        if (HIDDENBAND(lpBand)) continue;

        if (i > rowstart && (lpBand->fStyle & RBBS_BREAK || xMin + lpBand->lcx > adjcx)) {
            TRACE("%s break on band %d\n", (lpBand->fStyle & RBBS_BREAK ? "Hard" : "Soft"), i - 1);
            REBAR_LayoutRow(infoPtr, rowstart, i, adjcx, &row, &yPos);
            rowstart = i;
            xMin = 0;
        }
        else
            xMin += SEP_WIDTH;

        xMin += lpBand->lcx;
    }
    REBAR_LayoutRow(infoPtr, rowstart, infoPtr->uNumBands, adjcx, &row, &yPos);

    if (!(infoPtr->dwStyle & RBS_VARHEIGHT))
        yPos = REBAR_SetBandsHeight(infoPtr, 0, infoPtr->uNumBands, yInit);

    infoPtr->uNumRows = row;

    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, 0, infoPtr->uNumBands);
    else
        REBAR_CalcHorzBand(infoPtr, 0, infoPtr->uNumBands);
    /* now compute size of Rebar itself */
    oldSize = infoPtr->calcSize;

    infoPtr->calcSize.cx = adjcx;
    infoPtr->calcSize.cy = yPos;
    TRACE("calcsize size=(%d, %d), origheight=(%d,%d)\n",
            infoPtr->calcSize.cx, infoPtr->calcSize.cy,
	    oldSize.cx, oldSize.cy);

    REBAR_DumpBand (infoPtr);
    REBAR_MoveChildWindows (infoPtr, 0, infoPtr->uNumBands);
    REBAR_ForceResize (infoPtr);

    /* note: after a RBN_HEIGHTCHANGE native sends once again all the RBN_CHILDSIZE
     * and does another ForceResize */
    if (oldSize.cy != infoPtr->calcSize.cy)
    {
        NMHDR heightchange;
        REBAR_Notify(&heightchange, infoPtr, RBN_HEIGHTCHANGE);
    }
}


static VOID
REBAR_ValidateBand (const REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
     /* Function:  This routine evaluates the band specs supplied */
     /*  by the user and updates the following 5 fields in        */
     /*  the internal band structure: cxHeader, lcx, lcy, hcx, hcy*/
{
    UINT header=0;
    UINT textheight=0;
    UINT i, nonfixed;
    REBAR_BAND *tBand;

    lpBand->fStatus = 0;
    lpBand->lcx = 0;
    lpBand->lcy = 0;

    /* Data coming in from users into the cx... and cy... fields   */
    /* may be bad, just garbage, because the user never clears     */
    /* the fields. RB_{SET|INSERT}BAND{A|W} just passes the data   */
    /* along if the fields exist in the input area. Here we must   */
    /* determine if the data is valid. I have no idea how MS does  */
    /* the validation, but it does because the RB_GETBANDINFO      */
    /* returns a 0 when I know the sample program passed in an     */
    /* address. Here I will use the algorithm that if the value    */
    /* is greater than 65535 then it is bad and replace it with    */
    /* a zero. Feel free to improve the algorithm.  -  GA 12/2000  */
    if (lpBand->cxMinChild > 65535) lpBand->cxMinChild = 0;
    if (lpBand->cyMinChild > 65535) lpBand->cyMinChild = 0;
    if (lpBand->cx         > 65535) lpBand->cx         = 0;
    if (lpBand->cyChild    > 65535) lpBand->cyChild    = 0;
    if (lpBand->cyIntegral > 65535) lpBand->cyIntegral = 0;
    if (lpBand->cxIdeal    > 65535) lpBand->cxIdeal    = 0;
    if (lpBand->cxHeader   > 65535) lpBand->cxHeader   = 0;

    /* TODO : we could try return to the caller if a value changed so that */
    /*        a REBAR_Layout is needed. Till now the caller should call it */
    /*        it always (we should also check what native does)            */

    /* Header is where the image, text and gripper exist  */
    /* in the band and precede the child window.          */

    /* count number of non-FIXEDSIZE and non-Hidden bands */
    nonfixed = 0;
    for (i=0; i<infoPtr->uNumBands; i++){
	tBand = &infoPtr->bands[i];
	if (!HIDDENBAND(tBand) && !(tBand->fStyle & RBBS_FIXEDSIZE))
	    nonfixed++;
    }

    /* calculate gripper rectangle */
    if (  (!(lpBand->fStyle & RBBS_NOGRIPPER)) &&
	  ( (lpBand->fStyle & RBBS_GRIPPERALWAYS) ||
	    ( !(lpBand->fStyle & RBBS_FIXEDSIZE) && (nonfixed > 1)))
       ) {
	lpBand->fStatus |= HAS_GRIPPER;
        if (infoPtr->dwStyle & CCS_VERT)
	    if (infoPtr->dwStyle & RBS_VERTICALGRIPPER)
                header += (GRIPPER_HEIGHT + REBAR_PRE_GRIPPER);
            else
	        header += (GRIPPER_WIDTH + REBAR_PRE_GRIPPER);
        else
            header += (REBAR_PRE_GRIPPER + GRIPPER_WIDTH);
        /* Always have 4 pixels before anything else */
        header += REBAR_ALWAYS_SPACE;
    }

    /* image is visible */
    if ((lpBand->fMask & RBBIM_IMAGE) && (infoPtr->himl)) {
	lpBand->fStatus |= HAS_IMAGE;
        if (infoPtr->dwStyle & CCS_VERT) {
	   header += (infoPtr->imageSize.cy + REBAR_POST_IMAGE);
	   lpBand->lcy = infoPtr->imageSize.cx + 2;
	}
	else {
	   header += (infoPtr->imageSize.cx + REBAR_POST_IMAGE);
	   lpBand->lcy = infoPtr->imageSize.cy + 2;
	}
    }

    /* text is visible */
    if ((lpBand->fMask & RBBIM_TEXT) && (lpBand->lpText) &&
        !(lpBand->fStyle & RBBS_HIDETITLE)) {
	HDC hdc = GetDC (0);
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	SIZE size;

	lpBand->fStatus |= HAS_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlenW (lpBand->lpText), &size);
	header += ((infoPtr->dwStyle & CCS_VERT) ? (size.cy + REBAR_POST_TEXT) : (size.cx + REBAR_POST_TEXT));
	textheight = (infoPtr->dwStyle & CCS_VERT) ? 0 : size.cy;

	SelectObject (hdc, hOldFont);
	ReleaseDC (0, hdc);
    }

    /* if no gripper but either image or text, then leave space */
    if ((lpBand->fStatus & (HAS_IMAGE | HAS_TEXT)) &&
	!(lpBand->fStatus & HAS_GRIPPER)) {
	header += REBAR_ALWAYS_SPACE;
    }

    /* check if user overrode the header value */
    if (!(lpBand->fStyle & RBBS_UNDOC_FIXEDHEADER))
        lpBand->cxHeader = header;


    /* Now compute minimum size of child window */
    lpBand->lcy = textheight;
    if (lpBand->hwndChild != NULL) {
	/* Set the .cy values for CHILDSIZE case */
        lpBand->lcy = max(lpBand->lcy, lpBand->cyChild + REBARSPACE(lpBand));
        TRACE("_CHILDSIZE\n");
    }
    else
        lpBand->lcy = max(lpBand->lcy, REBAR_NO_CHILD_HEIGHT);

    lpBand->lcx = lpBand->cxMinChild + lpBand->cxHeader + REBAR_POST_CHILD;
    if (lpBand->fStyle & RBBS_USECHEVRON && lpBand->cxMinChild < lpBand->cxIdeal)
        lpBand->lcx += CHEVRON_WIDTH;
}

static BOOL
REBAR_CommonSetupBand(HWND hwnd, const REBARBANDINFOW *lprbbi, REBAR_BAND *lpBand)
     /* Function:  This routine copies the supplied values from   */
     /*  user input (lprbbi) to the internal band structure.      */
     /*  It returns true if something changed and false if not.   */
{
    BOOL bChanged = FALSE;

    lpBand->fMask |= lprbbi->fMask;

    if( (lprbbi->fMask & RBBIM_STYLE) &&
        (lpBand->fStyle != lprbbi->fStyle ) )
    {
	lpBand->fStyle = lprbbi->fStyle;
        bChanged = TRUE;
    }

    if( (lprbbi->fMask & RBBIM_COLORS) &&
       ( ( lpBand->clrFore != lprbbi->clrFore ) ||
         ( lpBand->clrBack != lprbbi->clrBack ) ) )
    {
	lpBand->clrFore = lprbbi->clrFore;
	lpBand->clrBack = lprbbi->clrBack;
        bChanged = TRUE;
    }

    if( (lprbbi->fMask & RBBIM_IMAGE) &&
       ( lpBand->iImage != lprbbi->iImage ) )
    {
	lpBand->iImage = lprbbi->iImage;
        bChanged = TRUE;
    }

    if( (lprbbi->fMask & RBBIM_CHILD) &&
       (lprbbi->hwndChild != lpBand->hwndChild ) )
    {
	if (lprbbi->hwndChild) {
	    lpBand->hwndChild = lprbbi->hwndChild;
	    lpBand->hwndPrevParent =
		SetParent (lpBand->hwndChild, hwnd);
	    /* below in trace from WinRAR */
	    ShowWindow(lpBand->hwndChild, SW_SHOWNOACTIVATE | SW_SHOWNORMAL);
	    /* above in trace from WinRAR */
	}
	else {
	    TRACE("child: %p  prev parent: %p\n",
		   lpBand->hwndChild, lpBand->hwndPrevParent);
	    lpBand->hwndChild = 0;
	    lpBand->hwndPrevParent = 0;
	}
        bChanged = TRUE;
    }

    if( (lprbbi->fMask & RBBIM_CHILDSIZE) &&
        ( (lpBand->cxMinChild != lprbbi->cxMinChild) ||
          (lpBand->cyMinChild != lprbbi->cyMinChild ) ||
          ( (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) &&
            ( (lpBand->cyChild    != lprbbi->cyChild ) ||
              (lpBand->cyMaxChild != lprbbi->cyMaxChild ) ||
              (lpBand->cyIntegral != lprbbi->cyIntegral ) ) ) ||
          ( (lprbbi->cbSize < sizeof (REBARBANDINFOA)) &&
            ( (lpBand->cyChild ||
               lpBand->cyMaxChild ||
               lpBand->cyIntegral ) ) ) ) )
    {
	lpBand->cxMinChild = lprbbi->cxMinChild;
	lpBand->cyMinChild = lprbbi->cyMinChild;
        /* These fields where added in WIN32_IE == 0x400 and are set only for RBBS_VARIABLEHEIGHT bands */
        if (lprbbi->cbSize >= sizeof (REBARBANDINFOA) && (lpBand->fStyle & RBBS_VARIABLEHEIGHT)) {
	    lpBand->cyMaxChild = lprbbi->cyMaxChild;
            lpBand->cyIntegral = lprbbi->cyIntegral;

            lpBand->cyChild = lpBand->cyMinChild;
            round_child_height(lpBand, lprbbi->cyChild);  /* try to increase cyChild */
        }
	else {
	    lpBand->cyChild    = lpBand->cyMinChild;
	    lpBand->cyMaxChild = 0x7fffffff;
	    lpBand->cyIntegral = 0;
	}
        bChanged = TRUE;
    }

    if( (lprbbi->fMask & RBBIM_SIZE) &&
        (lpBand->cx != lprbbi->cx ) )
    {
	lpBand->cx = lprbbi->cx;
        bChanged = TRUE;
    }

    if( (lprbbi->fMask & RBBIM_BACKGROUND) &&
       ( lpBand->hbmBack != lprbbi->hbmBack ) )
    {
	lpBand->hbmBack = lprbbi->hbmBack;
        bChanged = TRUE;
    }

    if( (lprbbi->fMask & RBBIM_ID) &&
        (lpBand->wID != lprbbi->wID ) )
    {
	lpBand->wID = lprbbi->wID;
        bChanged = TRUE;
    }

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	if( (lprbbi->fMask & RBBIM_IDEALSIZE) &&
            ( lpBand->cxIdeal != lprbbi->cxIdeal ) )
        {
	    lpBand->cxIdeal = lprbbi->cxIdeal;
            bChanged = TRUE;
        }

	if( (lprbbi->fMask & RBBIM_LPARAM) &&
            (lpBand->lParam != lprbbi->lParam ) )
        {
	    lpBand->lParam = lprbbi->lParam;
            bChanged = TRUE;
        }

	if( (lprbbi->fMask & RBBIM_HEADERSIZE) &&
            (lpBand->cxHeader != lprbbi->cxHeader ) )
        {
	    lpBand->cxHeader = lprbbi->cxHeader;
            lpBand->fStyle |= RBBS_UNDOC_FIXEDHEADER;
            bChanged = TRUE;
        }
    }

    return bChanged;
}

static LRESULT
REBAR_InternalEraseBkGnd (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam, const RECT *clip)
     /* Function:  This erases the background rectangle by drawing  */
     /*  each band with its background color (or the default) and   */
     /*  draws each bands right separator if necessary. The row     */
     /*  separators are drawn on the first band of the next row.    */
{
    REBAR_BAND *lpBand;
    UINT i;
    INT oldrow;
    HDC hdc = (HDC)wParam;
    RECT cr;
    COLORREF old = CLR_NONE, new;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);

    GetClientRect (infoPtr->hwndSelf, &cr);

    oldrow = -1;
    for(i=0; i<infoPtr->uNumBands; i++) {
        RECT rcBand;
        lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
        translate_rect(infoPtr, &rcBand, &lpBand->rcBand);

	/* draw band separator between rows */
	if (lpBand->iRow != oldrow) {
	    oldrow = lpBand->iRow;
	    if (infoPtr->dwStyle & RBS_BANDBORDERS) {
		RECT rcRowSep;
		rcRowSep = rcBand;
		if (infoPtr->dwStyle & CCS_VERT) {
		    rcRowSep.right += SEP_WIDTH_SIZE;
		    rcRowSep.bottom = infoPtr->calcSize.cx;
                    if (theme)
                        DrawThemeEdge (theme, hdc, RP_BAND, 0, &rcRowSep, EDGE_ETCHED, BF_RIGHT, NULL);
                    else
		        DrawEdge (hdc, &rcRowSep, EDGE_ETCHED, BF_RIGHT);
		}
		else {
		    rcRowSep.bottom += SEP_WIDTH_SIZE;
		    rcRowSep.right = infoPtr->calcSize.cx;
                    if (theme)
                        DrawThemeEdge (theme, hdc, RP_BAND, 0, &rcRowSep, EDGE_ETCHED, BF_BOTTOM, NULL);
                    else
		        DrawEdge (hdc, &rcRowSep, EDGE_ETCHED, BF_BOTTOM);
		}
		TRACE ("drawing band separator bottom (%d,%d)-(%d,%d)\n",
		       rcRowSep.left, rcRowSep.top,
		       rcRowSep.right, rcRowSep.bottom);
	    }
	}

	/* draw band separator between bands in a row */
        if (infoPtr->dwStyle & RBS_BANDBORDERS && lpBand->rcBand.left > 0) {
	    RECT rcSep;
	    rcSep = rcBand;
	    if (infoPtr->dwStyle & CCS_VERT) {
                rcSep.bottom = rcSep.top;
		rcSep.top -= SEP_WIDTH_SIZE;
                if (theme)
                    DrawThemeEdge (theme, hdc, RP_BAND, 0, &rcSep, EDGE_ETCHED, BF_BOTTOM, NULL);
                else
		    DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_BOTTOM);
	    }
	    else {
                rcSep.right = rcSep.left;
		rcSep.left -= SEP_WIDTH_SIZE;
                if (theme)
                    DrawThemeEdge (theme, hdc, RP_BAND, 0, &rcSep, EDGE_ETCHED, BF_RIGHT, NULL);
                else
		    DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_RIGHT);
	    }
            TRACE("drawing band separator right (%d,%d)-(%d,%d)\n",
		  rcSep.left, rcSep.top, rcSep.right, rcSep.bottom);
	}

	/* draw the actual background */
	if (lpBand->clrBack != CLR_NONE) {
	    new = (lpBand->clrBack == CLR_DEFAULT) ? infoPtr->clrBtnFace :
		    lpBand->clrBack;
#if GLATESTING
	    /* testing only - make background green to see it */
	    new = RGB(0,128,0);
#endif
	}
	else {
	    /* In the absence of documentation for Rebar vs. CLR_NONE,
	     * we will use the default BtnFace color. Note documentation
	     * exists for Listview and Imagelist.
	     */
	    new = infoPtr->clrBtnFace;
#if GLATESTING
	    /* testing only - make background green to see it */
	    new = RGB(0,128,0);
#endif
	}

        if (theme)
        {
            /* When themed, the background color is ignored (but not a
             * background bitmap */
            DrawThemeBackground (theme, hdc, 0, 0, &cr, &rcBand);
        }
        else
        {
            old = SetBkColor (hdc, new);
            TRACE("%s background color=0x%06x, band (%d,%d)-(%d,%d), clip (%d,%d)-(%d,%d)\n",
                  (lpBand->clrBack == CLR_NONE) ? "none" :
                    ((lpBand->clrBack == CLR_DEFAULT) ? "dft" : ""),
                  GetBkColor(hdc),
                  rcBand.left,rcBand.top,
                  rcBand.right,rcBand.bottom,
                  clip->left, clip->top,
                  clip->right, clip->bottom);
            ExtTextOutW (hdc, 0, 0, ETO_OPAQUE, &rcBand, NULL, 0, 0);
            if (lpBand->clrBack != CLR_NONE)
                SetBkColor (hdc, old);
        }
    }
    return TRUE;
}

static void
REBAR_InternalHitTest (const REBAR_INFO *infoPtr, const POINT *lpPt, UINT *pFlags, INT *pBand)
{
    REBAR_BAND *lpBand;
    RECT rect;
    UINT  iCount;

    GetClientRect (infoPtr->hwndSelf, &rect);

    *pFlags = RBHT_NOWHERE;
    if (PtInRect (&rect, *lpPt))
    {
	if (infoPtr->uNumBands == 0) {
	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;
	    TRACE("NOWHERE\n");
	    return;
	}
	else {
	    /* somewhere inside */
	    for (iCount = 0; iCount < infoPtr->uNumBands; iCount++) {
                RECT rcBand;
		lpBand = &infoPtr->bands[iCount];
                translate_rect(infoPtr, &rcBand, &lpBand->rcBand);
                if (HIDDENBAND(lpBand)) continue;
		if (PtInRect (&rcBand, *lpPt)) {
		    if (pBand)
			*pBand = iCount;
		    if (PtInRect (&lpBand->rcGripper, *lpPt)) {
			*pFlags = RBHT_GRABBER;
			TRACE("ON GRABBER %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcCapImage, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE("ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcCapText, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE("ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcChild, *lpPt)) {
			*pFlags = RBHT_CLIENT;
			TRACE("ON CLIENT %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcChevron, *lpPt)) {
			*pFlags = RBHT_CHEVRON;
			TRACE("ON CHEVRON %d\n", iCount);
			return;
		    }
		    else {
			*pFlags = RBHT_NOWHERE;
			TRACE("NOWHERE %d\n", iCount);
			return;
		    }
		}
	    }

	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;

	    TRACE("NOWHERE\n");
	    return;
	}
    }
    else {
	*pFlags = RBHT_NOWHERE;
	if (pBand)
	    *pBand = -1;
	TRACE("NOWHERE\n");
	return;
    }
}

static void
REBAR_HandleLRDrag (REBAR_INFO *infoPtr, const POINT *ptsmove)
     /* Function:  This will implement the functionality of a     */
     /*  Gripper drag within a row. It will not implement "out-   */
     /*  of-row" drags. (They are detected and handled in         */
     /*  REBAR_MouseMove.)                                        */
     /*  **** FIXME Switching order of bands in a row not   ****  */
     /*  ****       yet implemented.                        ****  */
{
    REBAR_BAND *hitBand;
    INT iHitBand, iRowBegin, iRowEnd;
    INT movement, xBand;

    /* on first significant mouse movement, issue notify */
    if (!(infoPtr->fStatus & BEGIN_DRAG_ISSUED)) {
	if (REBAR_Notify_NMREBAR (infoPtr, -1, RBN_BEGINDRAG)) {
	    /* Notify returned TRUE - abort drag */
	    infoPtr->dragStart.x = 0;
	    infoPtr->dragStart.y = 0;
	    infoPtr->dragNow = infoPtr->dragStart;
	    infoPtr->iGrabbedBand = -1;
	    ReleaseCapture ();
	    return ;
	}
	infoPtr->fStatus |= BEGIN_DRAG_ISSUED;
    }

    iHitBand = infoPtr->iGrabbedBand;
    iRowBegin = get_row_begin_for_band(infoPtr, iHitBand);
    iRowEnd = get_row_end_for_band(infoPtr, iHitBand);
    hitBand = &infoPtr->bands[iHitBand];

    xBand = hitBand->rcBand.left;
    movement = (infoPtr->dwStyle&CCS_VERT ? ptsmove->y : ptsmove->x)
                    - (xBand + REBAR_PRE_GRIPPER - infoPtr->ihitoffset);

    if (movement < 0) {
        int cxLeft = REBAR_ShrinkBandsRTL(infoPtr, iRowBegin, iHitBand, -movement, TRUE);
        hitBand->cxEffective += -movement - cxLeft;
        hitBand->cx = hitBand->cxEffective;
    } else if (movement > 0) {
        int cxLeft = REBAR_ShrinkBandsLTR(infoPtr, iHitBand, iRowEnd, movement, TRUE);
        REBAR_BAND *lpPrev = &infoPtr->bands[prev_band(infoPtr, iHitBand)];
        lpPrev->cxEffective += movement - cxLeft;
        lpPrev->cx = lpPrev->cxEffective;
    }

    REBAR_SetRowRectsX(infoPtr, iRowBegin, iRowEnd);
    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, 0, infoPtr->uNumBands);
    else
        REBAR_CalcHorzBand(infoPtr, 0, infoPtr->uNumBands);
    REBAR_MoveChildWindows(infoPtr, iRowBegin, iRowEnd);
}



/* << REBAR_BeginDrag >> */


static LRESULT
REBAR_DeleteBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    UINT uBand = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("deleting band %u!\n", uBand);
    lpBand = &infoPtr->bands[uBand];
    REBAR_Notify_NMREBAR (infoPtr, uBand, RBN_DELETINGBAND);
    /* TODO: a return of 1 should probably cancel the deletion */

    if (lpBand->hwndChild)
        ShowWindow(lpBand->hwndChild, SW_HIDE);
    Free(lpBand->lpText);

    infoPtr->uNumBands--;
    memmove(&infoPtr->bands[uBand], &infoPtr->bands[uBand+1],
        (infoPtr->uNumBands - uBand) * sizeof(REBAR_BAND));
    infoPtr->bands = ReAlloc(infoPtr->bands, infoPtr->uNumBands * sizeof(REBAR_BAND));

    REBAR_Notify_NMREBAR (infoPtr, -1, RBN_DELETEDBAND);

    /* if only 1 band left the re-validate to possible eliminate gripper */
    if (infoPtr->uNumBands == 1)
      REBAR_ValidateBand (infoPtr, &infoPtr->bands[0]);

    REBAR_Layout(infoPtr, NULL);

    return TRUE;
}


/* << REBAR_DragMove >> */
/* << REBAR_EndDrag >> */


static LRESULT
REBAR_GetBandBorders (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPRECT lpRect = (LPRECT)lParam;
    REBAR_BAND *lpBand;

    if (!lParam)
	return 0;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return 0;

    lpBand = &infoPtr->bands[(UINT)wParam];

    /* FIXME - the following values were determined by experimentation */
    /* with the REBAR Control Spy. I have guesses as to what the 4 and */
    /* 1 are, but I am not sure. There doesn't seem to be any actual   */
    /* difference in size of the control area with and without the     */
    /* style.  -  GA                                                   */
    if (infoPtr->dwStyle & RBS_BANDBORDERS) {
	if (infoPtr->dwStyle & CCS_VERT) {
	    lpRect->left = 1;
	    lpRect->top = lpBand->cxHeader + 4;
	    lpRect->right = 1;
	    lpRect->bottom = 0;
	}
	else {
	    lpRect->left = lpBand->cxHeader + 4;
	    lpRect->top = 1;
	    lpRect->right = 0;
	    lpRect->bottom = 1;
	}
    }
    else {
	lpRect->left = lpBand->cxHeader;
    }
    return 0;
}


static inline LRESULT
REBAR_GetBandCount (const REBAR_INFO *infoPtr)
{
    TRACE("band count %u!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetBandInfoT(const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam, BOOL bUnicode)
{
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u (bUnicode=%d)\n", (UINT)wParam, bUnicode);

    /* copy band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lprbbi->fStyle = lpBand->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lprbbi->clrFore = lpBand->clrFore;
	lprbbi->clrBack = lpBand->clrBack;
	if (lprbbi->clrBack == CLR_DEFAULT)
	    lprbbi->clrBack = infoPtr->clrBtnFace;
    }

    if (lprbbi->fMask & RBBIM_TEXT) {
        if (bUnicode)
            Str_GetPtrW(lpBand->lpText, lprbbi->lpText, lprbbi->cch);
        else
            Str_GetPtrWtoA(lpBand->lpText, (LPSTR)lprbbi->lpText, lprbbi->cch);
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lprbbi->iImage = lpBand->iImage;

    if (lprbbi->fMask & RBBIM_CHILD)
	lprbbi->hwndChild = lpBand->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lprbbi->cxMinChild = lpBand->cxMinChild;
	lprbbi->cyMinChild = lpBand->cyMinChild;
        /* to make tests pass we follow Windows behaviour and allow to read these fields only
         * for RBBS_VARIABLEHEIGHTS bands */
        if (lprbbi->cbSize >= sizeof (REBARBANDINFOA) && (lpBand->fStyle & RBBS_VARIABLEHEIGHT)) {
	    lprbbi->cyChild    = lpBand->cyChild;
	    lprbbi->cyMaxChild = lpBand->cyMaxChild;
	    lprbbi->cyIntegral = lpBand->cyIntegral;
	}
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lprbbi->cx = lpBand->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lprbbi->hbmBack = lpBand->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lprbbi->wID = lpBand->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lprbbi->cxIdeal = lpBand->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lprbbi->lParam = lpBand->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lprbbi->cxHeader = lpBand->cxHeader;
    }

    REBAR_DumpBandInfo(lprbbi);

    return TRUE;
}


static LRESULT
REBAR_GetBarHeight (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT nHeight;

    nHeight = infoPtr->calcSize.cy;

    TRACE("height = %d\n", nHeight);

    return nHeight;
}


static LRESULT
REBAR_GetBarInfo (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE("getting bar info!\n");

    if (infoPtr->himl) {
	lpInfo->himl = infoPtr->himl;
	lpInfo->fMask |= RBIM_IMAGELIST;
    }

    return TRUE;
}


static inline LRESULT
REBAR_GetBkColor (const REBAR_INFO *infoPtr)
{
    COLORREF clr = infoPtr->clrBk;

    if (clr == CLR_DEFAULT)
      clr = infoPtr->clrBtnFace;

    TRACE("background color 0x%06x!\n", clr);

    return clr;
}


/* << REBAR_GetColorScheme >> */
/* << REBAR_GetDropTarget >> */


static LRESULT
REBAR_GetPalette (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    FIXME("empty stub!\n");

    return 0;
}


static LRESULT
REBAR_GetRect (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT iBand = (INT)wParam;
    LPRECT lprc = (LPRECT)lParam;
    REBAR_BAND *lpBand;

    if ((iBand < 0) || ((UINT)iBand >= infoPtr->uNumBands))
	return FALSE;
    if (!lprc)
	return FALSE;

    lpBand = &infoPtr->bands[iBand];
    /* For CCS_VERT the coordintes will be swapped - like on Windows */
    CopyRect (lprc, &lpBand->rcBand);

    TRACE("band %d, (%d,%d)-(%d,%d)\n", iBand,
	  lprc->left, lprc->top, lprc->right, lprc->bottom);

    return TRUE;
}


static inline LRESULT
REBAR_GetRowCount (const REBAR_INFO *infoPtr)
{
    TRACE("%u\n", infoPtr->uNumRows);

    return infoPtr->uNumRows;
}


static LRESULT
REBAR_GetRowHeight (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT iRow = (INT)wParam;
    int j = 0, ret = 0;
    UINT i;
    REBAR_BAND *lpBand;

    for (i=0; i<infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	if (lpBand->iRow != iRow) continue;
        j = lpBand->rcBand.bottom - lpBand->rcBand.top;
	if (j > ret) ret = j;
    }

    TRACE("row %d, height %d\n", iRow, ret);

    return ret;
}


static inline LRESULT
REBAR_GetTextColor (const REBAR_INFO *infoPtr)
{
    TRACE("text color 0x%06x!\n", infoPtr->clrText);

    return infoPtr->clrText;
}


static inline LRESULT
REBAR_GetToolTips (const REBAR_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hwndToolTip;
}


static inline LRESULT
REBAR_GetUnicodeFormat (const REBAR_INFO *infoPtr)
{
    TRACE("%s hwnd=%p\n",
	  infoPtr->bUnicode ? "TRUE" : "FALSE", infoPtr->hwndSelf);

    return infoPtr->bUnicode;
}


static inline LRESULT
REBAR_GetVersion (const REBAR_INFO *infoPtr)
{
    TRACE("version %d\n", infoPtr->iVersion);
    return infoPtr->iVersion;
}


static LRESULT
REBAR_HitTest (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPRBHITTESTINFO lprbht = (LPRBHITTESTINFO)lParam;

    if (!lprbht)
	return -1;

    REBAR_InternalHitTest (infoPtr, &lprbht->pt, &lprbht->flags, &lprbht->iBand);

    return lprbht->iBand;
}


static LRESULT
REBAR_IdToIndex (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    UINT i;

    if (infoPtr == NULL)
	return -1;

    if (infoPtr->uNumBands < 1)
	return -1;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	if (infoPtr->bands[i].wID == (UINT)wParam) {
	    TRACE("id %u is band %u found!\n", (UINT)wParam, i);
	    return i;
	}
    }

    TRACE("id %u is not found\n", (UINT)wParam);
    return -1;
}


static LRESULT
REBAR_InsertBandT(REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam, BOOL bUnicode)
{
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    UINT uIndex = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;

    /* trace the index as signed to see the -1 */
    TRACE("insert band at %d (bUnicode=%d)!\n", (INT)uIndex, bUnicode);
    REBAR_DumpBandInfo(lprbbi);

    infoPtr->bands = ReAlloc(infoPtr->bands, (infoPtr->uNumBands+1) * sizeof(REBAR_BAND));
    if (((INT)uIndex == -1) || (uIndex > infoPtr->uNumBands))
        uIndex = infoPtr->uNumBands;
    memmove(&infoPtr->bands[uIndex+1], &infoPtr->bands[uIndex],
        sizeof(REBAR_BAND) * (infoPtr->uNumBands - uIndex));
    infoPtr->uNumBands++;

    TRACE("index %u!\n", uIndex);

    /* initialize band (infoPtr->bands[uIndex])*/
    lpBand = &infoPtr->bands[uIndex];
    ZeroMemory(lpBand, sizeof(*lpBand));
    lpBand->clrFore = infoPtr->clrText;
    lpBand->clrBack = infoPtr->clrBk;
    lpBand->iImage = -1;

    REBAR_CommonSetupBand(infoPtr->hwndSelf, lprbbi, lpBand);
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
        if (bUnicode)
            Str_SetPtrW(&lpBand->lpText, lprbbi->lpText);
        else
            Str_SetPtrAtoW(&lpBand->lpText, (LPSTR)lprbbi->lpText);
    }

    REBAR_ValidateBand (infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (infoPtr, &infoPtr->bands[0]);

    REBAR_DumpBand (infoPtr);

    REBAR_Layout(infoPtr, NULL);
    InvalidateRect(infoPtr->hwndSelf, 0, TRUE);

    return TRUE;
}


static LRESULT
REBAR_MaximizeBand (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    UINT uBand = (UINT) wParam;
    int iRowBegin, iRowEnd;
    int cxDesired, extra, extraOrig;
    int cxIdealBand;

    /* Validate */
    if ((infoPtr->uNumBands == 0) ||
	((INT)uBand < 0) || (uBand >= infoPtr->uNumBands)) {
	/* error !!! */
	ERR("Illegal MaximizeBand, requested=%d, current band count=%d\n",
	      (INT)uBand, infoPtr->uNumBands);
      	return FALSE;
    }

    lpBand = &infoPtr->bands[uBand];

    cxIdealBand = lpBand->cxIdeal + lpBand->cxHeader + REBAR_POST_CHILD;
    if (lParam && (lpBand->cxEffective < cxIdealBand))
        cxDesired = cxIdealBand;
    else
        cxDesired = infoPtr->calcSize.cx;

    iRowBegin = get_row_begin_for_band(infoPtr, uBand);
    iRowEnd   = get_row_end_for_band(infoPtr, uBand);
    extraOrig = extra = cxDesired - lpBand->cxEffective;
    if (extra > 0)
        extra = REBAR_ShrinkBandsRTL(infoPtr, iRowBegin, uBand, extra, TRUE);
    if (extra > 0)
        extra = REBAR_ShrinkBandsLTR(infoPtr, next_band(infoPtr, uBand), iRowEnd, extra, TRUE);
    lpBand->cxEffective += extraOrig - extra;
    lpBand->cx = lpBand->cxEffective;
    TRACE("(%ld, %ld): Wanted size %d, obtained %d (shrink %d, %d)\n", wParam, lParam, cxDesired, lpBand->cx, extraOrig, extra);
    REBAR_SetRowRectsX(infoPtr, iRowBegin, iRowEnd);

    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, iRowBegin, iRowEnd);
    else
        REBAR_CalcHorzBand(infoPtr, iRowBegin, iRowEnd);
    REBAR_MoveChildWindows(infoPtr, iRowBegin, iRowEnd);
    return TRUE;

}


static LRESULT
REBAR_MinimizeBand (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    UINT uBand = (UINT) wParam;
    int iPrev, iRowBegin, iRowEnd;

    /* A "minimize" band is equivalent to "dragging" the gripper
     * of than band to the right till the band is only the size
     * of the cxHeader.
     */

    /* Validate */
    if ((infoPtr->uNumBands == 0) ||
	((INT)uBand < 0) || (uBand >= infoPtr->uNumBands)) {
	/* error !!! */
	ERR("Illegal MinimizeBand, requested=%d, current band count=%d\n",
	      (INT)uBand, infoPtr->uNumBands);
      	return FALSE;
    }

    /* compute amount of movement and validate */
    lpBand = &infoPtr->bands[uBand];
    iPrev = prev_band(infoPtr, uBand);
    /* if first band in row */
    if (iPrev < 0 || infoPtr->bands[iPrev].iRow != lpBand->iRow) {
        int iNext = next_band(infoPtr, uBand);
        if (iNext < infoPtr->uNumBands && infoPtr->bands[iNext].iRow == lpBand->iRow) {
            TRACE("(%ld): Minimizing the first band in row is by maximizing the second\n", wParam);
            REBAR_MaximizeBand(infoPtr, iNext, FALSE);
        }
        else
            TRACE("(%ld): Only one band in row - nothing to do\n", wParam);
        return TRUE;
    }

    infoPtr->bands[iPrev].cxEffective += lpBand->cxEffective - lpBand->lcx;
    infoPtr->bands[iPrev].cx = infoPtr->bands[iPrev].cxEffective;
    lpBand->cx = lpBand->cxEffective = lpBand->lcx;

    iRowBegin = get_row_begin_for_band(infoPtr, uBand);
    iRowEnd = get_row_end_for_band(infoPtr, uBand);
    REBAR_SetRowRectsX(infoPtr, iRowBegin, iRowEnd);

    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, iRowBegin, iRowEnd);
    else
        REBAR_CalcHorzBand(infoPtr, iRowBegin, iRowEnd);
    REBAR_MoveChildWindows(infoPtr, iRowBegin, iRowEnd);
    return FALSE;
}


static LRESULT
REBAR_MoveBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *oldBands = infoPtr->bands;
    REBAR_BAND holder;
    UINT uFrom = (UINT)wParam;
    UINT uTo = (UINT)lParam;

    /* Validate */
    if ((infoPtr->uNumBands == 0) ||
	((INT)uFrom < 0) || (uFrom >= infoPtr->uNumBands) ||
	((INT)uTo < 0)   || (uTo >= infoPtr->uNumBands)) {
	/* error !!! */
	ERR("Illegal MoveBand, from=%d, to=%d, current band count=%d\n",
	      (INT)uFrom, (INT)uTo, infoPtr->uNumBands);
      	return FALSE;
    }

    /* save one to be moved */
    memcpy (&holder, &oldBands[uFrom], sizeof(REBAR_BAND));

    /* close up rest of bands (pseudo delete) */
    if (uFrom < infoPtr->uNumBands - 1) {
	memcpy (&oldBands[uFrom], &oldBands[uFrom+1],
		(infoPtr->uNumBands - uFrom - 1) * sizeof(REBAR_BAND));
    }

    /* allocate new space and copy rest of bands into it */
    infoPtr->bands =
	(REBAR_BAND *)Alloc ((infoPtr->uNumBands)*sizeof(REBAR_BAND));

    /* pre insert copy */
    if (uTo > 0) {
	memcpy (&infoPtr->bands[0], &oldBands[0],
		uTo * sizeof(REBAR_BAND));
    }

    /* set moved band */
    memcpy (&infoPtr->bands[uTo], &holder, sizeof(REBAR_BAND));

    /* post copy */
    if (uTo < infoPtr->uNumBands - 1) {
	memcpy (&infoPtr->bands[uTo+1], &oldBands[uTo],
		(infoPtr->uNumBands - uTo - 1) * sizeof(REBAR_BAND));
    }

    Free (oldBands);

    TRACE("moved band %d to index %d\n", uFrom, uTo);
    REBAR_DumpBand (infoPtr);

    /* **************************************************** */
    /*                                                      */
    /* We do not do a REBAR_Layout here because the native  */
    /* control does not do that. The actual layout and      */
    /* repaint is done by the *next* real action, ex.:      */
    /* RB_INSERTBAND, RB_DELETEBAND, RB_SIZETORECT, etc.    */
    /*                                                      */
    /* **************************************************** */

    return TRUE;
}


/* return TRUE if two strings are different */
static BOOL
REBAR_strdifW( LPCWSTR a, LPCWSTR b )
{
    return ( (a && !b) || (b && !a) || (a && b && lstrcmpW(a, b) ) );
}

static LRESULT
REBAR_SetBandInfoT(REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam, BOOL bUnicode)
{
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;
    BOOL bChanged;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);
    REBAR_DumpBandInfo (lprbbi);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    bChanged = REBAR_CommonSetupBand (infoPtr->hwndSelf, lprbbi, lpBand);
    if (lprbbi->fMask & RBBIM_TEXT) {
        LPWSTR wstr = NULL;
        if (bUnicode)
            Str_SetPtrW(&wstr, lprbbi->lpText);
        else
            Str_SetPtrAtoW(&wstr, (LPSTR)lprbbi->lpText);

        if (REBAR_strdifW(wstr, lprbbi->lpText)) {
            Free(lpBand->lpText);
            lpBand->lpText = wstr;
            bChanged = TRUE;
        }
        else
            Free(wstr);
    }

    REBAR_ValidateBand (infoPtr, lpBand);

    REBAR_DumpBand (infoPtr);

    if (bChanged && (lprbbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE))) {
	  REBAR_Layout(infoPtr, NULL);
	  InvalidateRect(infoPtr->hwndSelf, 0, 1);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBarInfo (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;
    REBAR_BAND *lpBand;
    UINT i;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE("setting bar info!\n");

    if (lpInfo->fMask & RBIM_IMAGELIST) {
	infoPtr->himl = lpInfo->himl;
	if (infoPtr->himl) {
            INT cx, cy;
	    ImageList_GetIconSize (infoPtr->himl, &cx, &cy);
	    infoPtr->imageSize.cx = cx;
	    infoPtr->imageSize.cy = cy;
	}
	else {
	    infoPtr->imageSize.cx = 0;
	    infoPtr->imageSize.cy = 0;
	}
	TRACE("new image cx=%d, cy=%d\n", infoPtr->imageSize.cx,
	      infoPtr->imageSize.cy);
    }

    /* revalidate all bands to reset flags for images in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	REBAR_ValidateBand (infoPtr, lpBand);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBkColor (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    COLORREF clrTemp;

    clrTemp = infoPtr->clrBk;
    infoPtr->clrBk = (COLORREF)lParam;

    TRACE("background color 0x%06x!\n", infoPtr->clrBk);

    return clrTemp;
}


/* << REBAR_SetColorScheme >> */
/* << REBAR_SetPalette >> */


static LRESULT
REBAR_SetParent (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HWND hwndTemp = infoPtr->hwndNotify;

    infoPtr->hwndNotify = (HWND)wParam;

    return (LRESULT)hwndTemp;
}


static LRESULT
REBAR_SetTextColor (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    COLORREF clrTemp;

    clrTemp = infoPtr->clrText;
    infoPtr->clrText = (COLORREF)lParam;

    TRACE("text color 0x%06x!\n", infoPtr->clrText);

    return clrTemp;
}


/* << REBAR_SetTooltips >> */


static inline LRESULT
REBAR_SetUnicodeFormat (REBAR_INFO *infoPtr, WPARAM wParam)
{
    BOOL bTemp = infoPtr->bUnicode;

    TRACE("to %s hwnd=%p, was %s\n",
	  ((BOOL)wParam) ? "TRUE" : "FALSE", infoPtr->hwndSelf,
	  (bTemp) ? "TRUE" : "FALSE");

    infoPtr->bUnicode = (BOOL)wParam;

   return bTemp;
}


static LRESULT
REBAR_SetVersion (REBAR_INFO *infoPtr, INT iVersion)
{
    INT iOldVersion = infoPtr->iVersion;

    if (iVersion > COMCTL32_VERSION)
	return -1;

    infoPtr->iVersion = iVersion;

    TRACE("new version %d\n", iVersion);

    return iOldVersion;
}


static LRESULT
REBAR_ShowBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;

    if (((INT)wParam < 0) || ((INT)wParam > infoPtr->uNumBands))
	return FALSE;

    lpBand = &infoPtr->bands[(INT)wParam];

    if ((BOOL)lParam) {
	TRACE("show band %d\n", (INT)wParam);
	lpBand->fStyle = lpBand->fStyle & ~RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_SHOW);
    }
    else {
	TRACE("hide band %d\n", (INT)wParam);
	lpBand->fStyle = lpBand->fStyle | RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_HIDE);
    }

    REBAR_Layout(infoPtr, NULL);
    InvalidateRect(infoPtr->hwndSelf, 0, 1);

    return TRUE;
}


static LRESULT
REBAR_SizeToRect (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPRECT lpRect = (LPRECT)lParam;
    RECT t1;

    if (lpRect == NULL)
       return FALSE;

    TRACE("[%d %d %d %d]\n",
	  lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

    /*  what is going on???? */
    GetWindowRect(infoPtr->hwndSelf, &t1);
    TRACE("window rect [%d %d %d %d]\n",
	  t1.left, t1.top, t1.right, t1.bottom);
    GetClientRect(infoPtr->hwndSelf, &t1);
    TRACE("client rect [%d %d %d %d]\n",
	  t1.left, t1.top, t1.right, t1.bottom);

    /* force full _Layout processing */
    REBAR_Layout(infoPtr, lpRect);
    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);
    return TRUE;
}



static LRESULT
REBAR_Create (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTW cs = (LPCREATESTRUCTW) lParam;
    RECT wnrc1, clrc1;
    HTHEME theme;

    if (TRACE_ON(rebar)) {
	GetWindowRect(infoPtr->hwndSelf, &wnrc1);
	GetClientRect(infoPtr->hwndSelf, &clrc1);
	TRACE("window=(%d,%d)-(%d,%d) client=(%d,%d)-(%d,%d) cs=(%d,%d %dx%d)\n",
	      wnrc1.left, wnrc1.top, wnrc1.right, wnrc1.bottom,
	      clrc1.left, clrc1.top, clrc1.right, clrc1.bottom,
	      cs->x, cs->y, cs->cx, cs->cy);
    }

    TRACE("created!\n");

    if ((theme = OpenThemeData (infoPtr->hwndSelf, themeClass)))
    {
        /* native seems to clear WS_BORDER when themed */
        infoPtr->dwStyle &= ~WS_BORDER;
    }

    return 0;
}


static LRESULT
REBAR_Destroy (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    UINT i;


    /* free rebar bands */
    if ((infoPtr->uNumBands > 0) && infoPtr->bands) {
	/* clean up each band */
	for (i = 0; i < infoPtr->uNumBands; i++) {
	    lpBand = &infoPtr->bands[i];

	    /* delete text strings */
            Free (lpBand->lpText);
            lpBand->lpText = NULL;
	    /* destroy child window */
	    DestroyWindow (lpBand->hwndChild);
	}

	/* free band array */
	Free (infoPtr->bands);
	infoPtr->bands = NULL;
    }

    DestroyCursor (infoPtr->hcurArrow);
    DestroyCursor (infoPtr->hcurHorz);
    DestroyCursor (infoPtr->hcurVert);
    DestroyCursor (infoPtr->hcurDrag);
    if(infoPtr->hDefaultFont) DeleteObject (infoPtr->hDefaultFont);
    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);

    CloseThemeData (GetWindowTheme (infoPtr->hwndSelf));

    /* free rebar info data */
    Free (infoPtr);
    TRACE("destroyed!\n");
    return 0;
}


static LRESULT
REBAR_EraseBkGnd (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    RECT cliprect;

    if (GetClipBox ( (HDC)wParam, &cliprect))
        return REBAR_InternalEraseBkGnd (infoPtr, wParam, lParam, &cliprect);
    return 0;
}


static LRESULT
REBAR_GetFont (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT)infoPtr->hFont;
}

static LRESULT
REBAR_PushChevron(const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if (wParam >= 0 && (UINT)wParam < infoPtr->uNumBands)
    {
        NMREBARCHEVRON nmrbc;
        REBAR_BAND *lpBand = &infoPtr->bands[wParam];

        TRACE("Pressed chevron on band %ld\n", wParam);

        /* redraw chevron in pushed state */
        lpBand->fDraw |= DRAW_CHEVRONPUSHED;
        RedrawWindow(infoPtr->hwndSelf, &lpBand->rcChevron,0,
          RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);

        /* notify app so it can display a popup menu or whatever */
        nmrbc.uBand = wParam;
        nmrbc.wID = lpBand->wID;
        nmrbc.lParam = lpBand->lParam;
        nmrbc.rc = lpBand->rcChevron;
        nmrbc.lParamNM = lParam;
        REBAR_Notify((NMHDR*)&nmrbc, infoPtr, RBN_CHEVRONPUSHED);

        /* redraw chevron in previous state */
        lpBand->fDraw &= ~DRAW_CHEVRONPUSHED;
        InvalidateRect(infoPtr->hwndSelf, &lpBand->rcChevron, TRUE);

        return TRUE;
    }
    return FALSE;
}

static LRESULT
REBAR_LButtonDown (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    UINT htFlags;
    INT iHitBand;
    POINT ptMouseDown;
    ptMouseDown.x = (short)LOWORD(lParam);
    ptMouseDown.y = (short)HIWORD(lParam);

    REBAR_InternalHitTest(infoPtr, &ptMouseDown, &htFlags, &iHitBand);
    lpBand = &infoPtr->bands[iHitBand];

    if (htFlags == RBHT_CHEVRON)
    {
        REBAR_PushChevron(infoPtr, iHitBand, 0);
    }
    else if (htFlags == RBHT_GRABBER || htFlags == RBHT_CAPTION)
    {
        TRACE("Starting drag\n");

        SetCapture (infoPtr->hwndSelf);
        infoPtr->iGrabbedBand = iHitBand;

        /* save off the LOWORD and HIWORD of lParam as initial x,y */
        infoPtr->dragStart.x = (short)LOWORD(lParam);
        infoPtr->dragStart.y = (short)HIWORD(lParam);
        infoPtr->dragNow = infoPtr->dragStart;
        if (infoPtr->dwStyle & CCS_VERT)
            infoPtr->ihitoffset = infoPtr->dragStart.y - (lpBand->rcBand.left + REBAR_PRE_GRIPPER);
        else
            infoPtr->ihitoffset = infoPtr->dragStart.x - (lpBand->rcBand.left + REBAR_PRE_GRIPPER);
    }
    return 0;
}

static LRESULT
REBAR_LButtonUp (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if (infoPtr->iGrabbedBand >= 0)
    {
        NMHDR layout;
        RECT rect;

        infoPtr->dragStart.x = 0;
        infoPtr->dragStart.y = 0;
        infoPtr->dragNow = infoPtr->dragStart;

        ReleaseCapture ();

        if (infoPtr->fStatus & BEGIN_DRAG_ISSUED) {
            REBAR_Notify(&layout, infoPtr, RBN_LAYOUTCHANGED);
            REBAR_Notify_NMREBAR (infoPtr, infoPtr->iGrabbedBand, RBN_ENDDRAG);
            infoPtr->fStatus &= ~BEGIN_DRAG_ISSUED;
        }

        infoPtr->iGrabbedBand = -1;

        GetClientRect(infoPtr->hwndSelf, &rect);
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    }

    return 0;
}

static LRESULT
REBAR_MouseLeave (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if (infoPtr->ichevronhotBand >= 0)
    {
        REBAR_BAND *lpChevronBand = &infoPtr->bands[infoPtr->ichevronhotBand];
        if (lpChevronBand->fDraw & DRAW_CHEVRONHOT)
        {
            lpChevronBand->fDraw &= ~DRAW_CHEVRONHOT;
            InvalidateRect(infoPtr->hwndSelf, &lpChevronBand->rcChevron, TRUE);
        }
    }
    infoPtr->iOldBand = -1;
    infoPtr->ichevronhotBand = -2;

    return TRUE;
}

static LRESULT
REBAR_MouseMove (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpChevronBand;
    POINT ptMove;

    ptMove.x = (short)LOWORD(lParam);
    ptMove.y = (short)HIWORD(lParam);

    /* if we are currently dragging a band */
    if (infoPtr->iGrabbedBand >= 0)
    {
        REBAR_BAND *band1, *band2;
        int yPtMove = (infoPtr->dwStyle & CCS_VERT ? ptMove.x : ptMove.y);

        if (GetCapture() != infoPtr->hwndSelf)
            ERR("We are dragging but haven't got capture?!?\n");

        band1 = &infoPtr->bands[infoPtr->iGrabbedBand-1];
        band2 = &infoPtr->bands[infoPtr->iGrabbedBand];

        /* if mouse did not move much, exit */
        if ((abs(ptMove.x - infoPtr->dragNow.x) <= mindragx) &&
            (abs(ptMove.y - infoPtr->dragNow.y) <= mindragy)) return 0;

        /* Test for valid drag case - must not be first band in row */
        if ((yPtMove < band2->rcBand.top) ||
	      (yPtMove > band2->rcBand.bottom) ||
              ((infoPtr->iGrabbedBand > 0) && (band1->iRow != band2->iRow))) {
            FIXME("Cannot drag to other rows yet!!\n");
        }
        else {
            REBAR_HandleLRDrag (infoPtr, &ptMove);
        }
    }
    else
    {
        INT iHitBand;
        UINT htFlags;
        TRACKMOUSEEVENT trackinfo;

        REBAR_InternalHitTest(infoPtr, &ptMove, &htFlags, &iHitBand);

        if (infoPtr->iOldBand >= 0 && infoPtr->iOldBand == infoPtr->ichevronhotBand)
        {
            lpChevronBand = &infoPtr->bands[infoPtr->ichevronhotBand];
            if (lpChevronBand->fDraw & DRAW_CHEVRONHOT)
            {
                lpChevronBand->fDraw &= ~DRAW_CHEVRONHOT;
                InvalidateRect(infoPtr->hwndSelf, &lpChevronBand->rcChevron, TRUE);
            }
            infoPtr->ichevronhotBand = -2;
        }

        if (htFlags == RBHT_CHEVRON)
        {
            /* fill in the TRACKMOUSEEVENT struct */
            trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
            trackinfo.dwFlags = TME_QUERY;
            trackinfo.hwndTrack = infoPtr->hwndSelf;
            trackinfo.dwHoverTime = 0;

            /* call _TrackMouseEvent to see if we are currently tracking for this hwnd */
            _TrackMouseEvent(&trackinfo);

            /* Make sure tracking is enabled so we receive a WM_MOUSELEAVE message */
            if(!(trackinfo.dwFlags & TME_LEAVE))
            {
                trackinfo.dwFlags = TME_LEAVE; /* notify upon leaving */

                /* call TRACKMOUSEEVENT so we receive a WM_MOUSELEAVE message */
                /* and can properly deactivate the hot chevron */
                _TrackMouseEvent(&trackinfo);
            }

            lpChevronBand = &infoPtr->bands[iHitBand];
            if (!(lpChevronBand->fDraw & DRAW_CHEVRONHOT))
            {
                lpChevronBand->fDraw |= DRAW_CHEVRONHOT;
                InvalidateRect(infoPtr->hwndSelf, &lpChevronBand->rcChevron, TRUE);
                infoPtr->ichevronhotBand = iHitBand;
            }
        }
        infoPtr->iOldBand = iHitBand;
    }

    return 0;
}


static inline LRESULT
REBAR_NCCalcSize (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HTHEME theme;
    RECT *rect = (RECT *)lParam;

    if (infoPtr->dwStyle & WS_BORDER) {
        rect->left   = min(rect->left + GetSystemMetrics(SM_CXEDGE), rect->right);
        rect->right  = max(rect->right - GetSystemMetrics(SM_CXEDGE), rect->left);
        rect->top    = min(rect->top + GetSystemMetrics(SM_CYEDGE), rect->bottom);
        rect->bottom = max(rect->bottom - GetSystemMetrics(SM_CYEDGE), rect->top);
    }
    else if ((theme = GetWindowTheme (infoPtr->hwndSelf)))
    {
        /* FIXME: should use GetThemeInt */
        rect->top = min(rect->top + 1, rect->bottom);
    }
    TRACE("new client=(%d,%d)-(%d,%d)\n", rect->left, rect->top, rect->right, rect->bottom);
    return 0;
}


static LRESULT
REBAR_NCCreate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTW cs = (LPCREATESTRUCTW) lParam;
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT wnrc1, clrc1;
    NONCLIENTMETRICSW ncm;
    HFONT tfont;

    if (infoPtr != NULL) {
	ERR("Strange info structure pointer *not* NULL\n");
	return FALSE;
    }

    if (TRACE_ON(rebar)) {
	GetWindowRect(hwnd, &wnrc1);
	GetClientRect(hwnd, &clrc1);
	TRACE("window=(%d,%d)-(%d,%d) client=(%d,%d)-(%d,%d) cs=(%d,%d %dx%d)\n",
	      wnrc1.left, wnrc1.top, wnrc1.right, wnrc1.bottom,
	      clrc1.left, clrc1.top, clrc1.right, clrc1.bottom,
	      cs->x, cs->y, cs->cx, cs->cy);
    }

    /* allocate memory for info structure */
    infoPtr = (REBAR_INFO *)Alloc (sizeof(REBAR_INFO));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    /* initialize info structure - initial values are 0 */
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = CLR_NONE;
    infoPtr->clrBtnText = GetSysColor (COLOR_BTNTEXT);
    infoPtr->clrBtnFace = GetSysColor (COLOR_BTNFACE);
    infoPtr->iOldBand = -1;
    infoPtr->ichevronhotBand = -2;
    infoPtr->iGrabbedBand = -1;
    infoPtr->hwndSelf = hwnd;
    infoPtr->DoRedraw = TRUE;
    infoPtr->hcurArrow = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    infoPtr->hcurHorz  = LoadCursorW (0, (LPWSTR)IDC_SIZEWE);
    infoPtr->hcurVert  = LoadCursorW (0, (LPWSTR)IDC_SIZENS);
    infoPtr->hcurDrag  = LoadCursorW (0, (LPWSTR)IDC_SIZE);
    infoPtr->fStatus = 0;
    infoPtr->hFont = GetStockObject (SYSTEM_FONT);

    /* issue WM_NOTIFYFORMAT to get unicode status of parent */
    REBAR_NotifyFormat(infoPtr, 0, NF_REQUERY);

    /* Stow away the original style */
    infoPtr->orgStyle = cs->style;
    /* add necessary styles to the requested styles */
    infoPtr->dwStyle = cs->style | WS_VISIBLE;
    if ((infoPtr->dwStyle & CCS_LAYOUT_MASK) == 0)
        infoPtr->dwStyle |= CCS_TOP;
    SetWindowLongW (hwnd, GWL_STYLE, infoPtr->dwStyle);

    /* get font handle for Caption Font */
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    /* if the font is bold, set to normal */
    if (ncm.lfCaptionFont.lfWeight > FW_NORMAL) {
	ncm.lfCaptionFont.lfWeight = FW_NORMAL;
    }
    tfont = CreateFontIndirectW (&ncm.lfCaptionFont);
    if (tfont) {
        infoPtr->hFont = infoPtr->hDefaultFont = tfont;
    }

/* native does:
	    GetSysColor (numerous);
	    GetSysColorBrush (numerous) (see WM_SYSCOLORCHANGE);
	   *GetStockObject (SYSTEM_FONT);
	   *SetWindowLong (hwnd, 0, info ptr);
	   *WM_NOTIFYFORMAT;
	   *SetWindowLong (hwnd, GWL_STYLE, style+0x10000001);
                                    WS_VISIBLE = 0x10000000;
                                    CCS_TOP    = 0x00000001;
	   *SystemParametersInfo (SPI_GETNONCLIENTMETRICS...);
	   *CreateFontIndirect (lfCaptionFont from above);
	    GetDC ();
	    SelectObject (hdc, fontabove);
	    GetTextMetrics (hdc, );    guessing is tmHeight
	    SelectObject (hdc, oldfont);
	    ReleaseDC ();
	    GetWindowRect ();
	    MapWindowPoints (0, parent, rectabove, 2);
	    GetWindowRect ();
	    GetClientRect ();
	    ClientToScreen (clientrect);
	    SetWindowPos (hwnd, 0, 0, 0, 0, 0, SWP_NOZORDER);
 */
    return TRUE;
}


static LRESULT
REBAR_NCHitTest (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    NMMOUSE nmmouse;
    POINT clpt;
    INT i;
    UINT scrap;
    LRESULT ret = HTCLIENT;

    /*
     * Differences from doc at MSDN (as observed with version 4.71 of
     *      comctl32.dll
     * 1. doc says nmmouse.pt is in screen coord, trace shows client coord.
     * 2. if band is not identified .dwItemSpec is 0xffffffff.
     * 3. native always seems to return HTCLIENT if notify return is 0.
     */

    clpt.x = (short)LOWORD(lParam);
    clpt.y = (short)HIWORD(lParam);
    ScreenToClient (infoPtr->hwndSelf, &clpt);
    REBAR_InternalHitTest (infoPtr, &clpt, &scrap,
			   (INT *)&nmmouse.dwItemSpec);
    nmmouse.dwItemData = 0;
    nmmouse.pt = clpt;
    nmmouse.dwHitInfo = 0;
    if ((i = REBAR_Notify((NMHDR *) &nmmouse, infoPtr, NM_NCHITTEST))) {
	TRACE("notify changed return value from %ld to %d\n",
	      ret, i);
	ret = (LRESULT) i;
    }
    TRACE("returning %ld, client point (%d,%d)\n", ret, clpt.x, clpt.y);
    return ret;
}


static LRESULT
REBAR_NCPaint (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    RECT rcWindow;
    HDC hdc;
    HTHEME theme;

    if (infoPtr->dwStyle & WS_MINIMIZE)
	return 0; /* Nothing to do */

    if (infoPtr->dwStyle & WS_BORDER) {

	/* adjust rectangle and draw the necessary edge */
	if (!(hdc = GetDCEx( infoPtr->hwndSelf, 0, DCX_USESTYLE | DCX_WINDOW )))
	    return 0;
	GetWindowRect (infoPtr->hwndSelf, &rcWindow);
	OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);
	TRACE("rect (%d,%d)-(%d,%d)\n",
	      rcWindow.left, rcWindow.top,
	      rcWindow.right, rcWindow.bottom);
	DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_RECT);
	ReleaseDC( infoPtr->hwndSelf, hdc );
    }
    else if ((theme = GetWindowTheme (infoPtr->hwndSelf)))
    {
        /* adjust rectangle and draw the necessary edge */
        if (!(hdc = GetDCEx( infoPtr->hwndSelf, 0, DCX_USESTYLE | DCX_WINDOW )))
            return 0;
        GetWindowRect (infoPtr->hwndSelf, &rcWindow);
        OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);
        TRACE("rect (%d,%d)-(%d,%d)\n",
              rcWindow.left, rcWindow.top,
              rcWindow.right, rcWindow.bottom);
        DrawThemeEdge (theme, hdc, 0, 0, &rcWindow, BDR_RAISEDINNER, BF_TOP, NULL);
        ReleaseDC( infoPtr->hwndSelf, hdc );
    }

    return 0;
}


static LRESULT
REBAR_NotifyFormat (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT i;

    if (lParam == NF_REQUERY) {
	i = SendMessageW(REBAR_GetNotifyParent (infoPtr),
			 WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);
        if ((i != NFR_ANSI) && (i != NFR_UNICODE)) {
	    ERR("wrong response to WM_NOTIFYFORMAT (%d), assuming ANSI\n", i);
	    i = NFR_ANSI;
	}
        infoPtr->bUnicode = (i == NFR_UNICODE) ? 1 : 0;
	return (LRESULT)i;
    }
    return (LRESULT)((infoPtr->bUnicode) ? NFR_UNICODE : NFR_ANSI);
}


static LRESULT
REBAR_Paint (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    GetClientRect(infoPtr->hwndSelf, &rc);
    hdc = wParam==0 ? BeginPaint (infoPtr->hwndSelf, &ps) : (HDC)wParam;

    TRACE("painting (%d,%d)-(%d,%d) client (%d,%d)-(%d,%d)\n",
	  ps.rcPaint.left, ps.rcPaint.top,
	  ps.rcPaint.right, ps.rcPaint.bottom,
	  rc.left, rc.top, rc.right, rc.bottom);

    if (ps.fErase) {
	/* Erase area of paint if requested */
        REBAR_InternalEraseBkGnd (infoPtr, wParam, lParam, &ps.rcPaint);
    }

    REBAR_Refresh (infoPtr, hdc);
    if (!wParam)
	EndPaint (infoPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
REBAR_SetCursor (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    UINT  flags;

    TRACE("code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (infoPtr->hwndSelf, &pt);

    REBAR_InternalHitTest (infoPtr, &pt, &flags, NULL);

    if (flags == RBHT_GRABBER) {
	if ((infoPtr->dwStyle & CCS_VERT) &&
	    !(infoPtr->dwStyle & RBS_VERTICALGRIPPER))
	    SetCursor (infoPtr->hcurVert);
	else
	    SetCursor (infoPtr->hcurHorz);
    }
    else if (flags != RBHT_CLIENT)
	SetCursor (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
REBAR_SetFont (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    UINT i;

    infoPtr->hFont = (HFONT)wParam;

    /* revalidate all bands to change sizes of text in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	REBAR_ValidateBand (infoPtr, lpBand);
    }

    REBAR_Layout(infoPtr, NULL);
    return 0;
}


static inline LRESULT
REBAR_SetRedraw (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
     /*****************************************************
      *
      * Function;
      *  Handles the WM_SETREDRAW message.
      *
      * Documentation:
      *  According to testing V4.71 of COMCTL32 returns the
      *  *previous* status of the redraw flag (either 0 or -1)
      *  instead of the MSDN documented value of 0 if handled
      *
      *****************************************************/
{
    BOOL oldredraw = infoPtr->DoRedraw;

    TRACE("set to %s, fStatus=%08x\n",
	  (wParam) ? "TRUE" : "FALSE", infoPtr->fStatus);
    infoPtr->DoRedraw = (BOOL) wParam;
    if (wParam) {
	if (infoPtr->fStatus & BAND_NEEDS_REDRAW) {
	    REBAR_MoveChildWindows (infoPtr, 0, infoPtr->uNumBands);
	    REBAR_ForceResize (infoPtr);
	    InvalidateRect (infoPtr->hwndSelf, 0, TRUE);
	}
	infoPtr->fStatus &= ~BAND_NEEDS_REDRAW;
    }
    return (oldredraw) ? -1 : 0;
}


static LRESULT
REBAR_Size (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("wParam=%lx, lParam=%lx\n", wParam, lParam);

    /* avoid auto resize infinite recursion */
    if (infoPtr->fStatus & AUTO_RESIZE) {
	infoPtr->fStatus &= ~AUTO_RESIZE;
	TRACE("AUTO_RESIZE was set, reset, fStatus=%08x lparam=%08lx\n",
	      infoPtr->fStatus, lParam);
	return 0;
    }

    /* FIXME: wrong */
    if (infoPtr->dwStyle & RBS_AUTOSIZE) {
	NMRBAUTOSIZE autosize;

	GetClientRect(infoPtr->hwndSelf, &autosize.rcTarget);
	autosize.fChanged = 0;  /* ??? */
	autosize.rcActual = autosize.rcTarget;  /* ??? */
	REBAR_Notify((NMHDR *) &autosize, infoPtr, RBN_AUTOSIZE);
	TRACE("RBN_AUTOSIZE client=(%d,%d), lp=%08lx\n",
	      autosize.rcTarget.right, autosize.rcTarget.bottom, lParam);
    }

    REBAR_Layout(infoPtr, NULL);

    return 0;
}


static LRESULT
REBAR_StyleChanged (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    STYLESTRUCT *ss = (STYLESTRUCT *)lParam;

    TRACE("current style=%08x, styleOld=%08x, style being set to=%08x\n",
	  infoPtr->dwStyle, ss->styleOld, ss->styleNew);
    infoPtr->orgStyle = infoPtr->dwStyle = ss->styleNew;
    if (GetWindowTheme (infoPtr->hwndSelf))
        infoPtr->dwStyle &= ~WS_BORDER;
    /* maybe it should be COMMON_STYLES like in toolbar */
    if ((ss->styleNew ^ ss->styleOld) & CCS_VERT)
        REBAR_Layout(infoPtr, NULL);

    return FALSE;
}

/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (REBAR_INFO* infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    CloseThemeData (theme);
    theme = OpenThemeData (infoPtr->hwndSelf, themeClass);
    /* WS_BORDER disappears when theming is enabled and reappears when
     * disabled... */
    infoPtr->dwStyle &= ~WS_BORDER;
    infoPtr->dwStyle |= theme ? 0 : (infoPtr->orgStyle & WS_BORDER);
    return 0;
}

static LRESULT
REBAR_WindowPosChanged (const REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    RECT rc;

    ret = DefWindowProcW(infoPtr->hwndSelf, WM_WINDOWPOSCHANGED,
			 wParam, lParam);
    GetWindowRect(infoPtr->hwndSelf, &rc);
    TRACE("hwnd %p new pos (%d,%d)-(%d,%d)\n",
	  infoPtr->hwndSelf, rc.left, rc.top, rc.right, rc.bottom);
    return ret;
}


static LRESULT WINAPI
REBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n",
	  hwnd, uMsg, wParam, lParam);
    if (!infoPtr && (uMsg != WM_NCCREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
/*	case RB_BEGINDRAG: */

	case RB_DELETEBAND:
	    return REBAR_DeleteBand (infoPtr, wParam, lParam);

/*	case RB_DRAGMOVE: */
/*	case RB_ENDDRAG: */

	case RB_GETBANDBORDERS:
	    return REBAR_GetBandBorders (infoPtr, wParam, lParam);

	case RB_GETBANDCOUNT:
	    return REBAR_GetBandCount (infoPtr);

	case RB_GETBANDINFO_OLD:
	case RB_GETBANDINFOA:
	    return REBAR_GetBandInfoT(infoPtr, wParam, lParam, FALSE);

	case RB_GETBANDINFOW:
	    return REBAR_GetBandInfoT(infoPtr, wParam, lParam, TRUE);

	case RB_GETBARHEIGHT:
	    return REBAR_GetBarHeight (infoPtr, wParam, lParam);

	case RB_GETBARINFO:
	    return REBAR_GetBarInfo (infoPtr, wParam, lParam);

	case RB_GETBKCOLOR:
	    return REBAR_GetBkColor (infoPtr);

/*	case RB_GETCOLORSCHEME: */
/*	case RB_GETDROPTARGET: */

	case RB_GETPALETTE:
	    return REBAR_GetPalette (infoPtr, wParam, lParam);

	case RB_GETRECT:
	    return REBAR_GetRect (infoPtr, wParam, lParam);

	case RB_GETROWCOUNT:
	    return REBAR_GetRowCount (infoPtr);

	case RB_GETROWHEIGHT:
	    return REBAR_GetRowHeight (infoPtr, wParam, lParam);

	case RB_GETTEXTCOLOR:
	    return REBAR_GetTextColor (infoPtr);

	case RB_GETTOOLTIPS:
	    return REBAR_GetToolTips (infoPtr);

	case RB_GETUNICODEFORMAT:
	    return REBAR_GetUnicodeFormat (infoPtr);

	case CCM_GETVERSION:
	    return REBAR_GetVersion (infoPtr);

	case RB_HITTEST:
	    return REBAR_HitTest (infoPtr, wParam, lParam);

	case RB_IDTOINDEX:
	    return REBAR_IdToIndex (infoPtr, wParam, lParam);

	case RB_INSERTBANDA:
	    return REBAR_InsertBandT(infoPtr, wParam, lParam, FALSE);

	case RB_INSERTBANDW:
	    return REBAR_InsertBandT(infoPtr, wParam, lParam, TRUE);

	case RB_MAXIMIZEBAND:
	    return REBAR_MaximizeBand (infoPtr, wParam, lParam);

	case RB_MINIMIZEBAND:
	    return REBAR_MinimizeBand (infoPtr, wParam, lParam);

	case RB_MOVEBAND:
	    return REBAR_MoveBand (infoPtr, wParam, lParam);

	case RB_PUSHCHEVRON:
	    return REBAR_PushChevron (infoPtr, wParam, lParam);

	case RB_SETBANDINFOA:
	    return REBAR_SetBandInfoT(infoPtr, wParam, lParam, FALSE);

	case RB_SETBANDINFOW:
	    return REBAR_SetBandInfoT(infoPtr, wParam, lParam, TRUE);

	case RB_SETBARINFO:
	    return REBAR_SetBarInfo (infoPtr, wParam, lParam);

	case RB_SETBKCOLOR:
	    return REBAR_SetBkColor (infoPtr, wParam, lParam);

/*	case RB_SETCOLORSCHEME: */
/*	case RB_SETPALETTE: */
/*	    return REBAR_GetPalette (infoPtr, wParam, lParam); */

	case RB_SETPARENT:
	    return REBAR_SetParent (infoPtr, wParam, lParam);

	case RB_SETTEXTCOLOR:
	    return REBAR_SetTextColor (infoPtr, wParam, lParam);

/*	case RB_SETTOOLTIPS: */

	case RB_SETUNICODEFORMAT:
	    return REBAR_SetUnicodeFormat (infoPtr, wParam);

	case CCM_SETVERSION:
	    return REBAR_SetVersion (infoPtr, (INT)wParam);

	case RB_SHOWBAND:
	    return REBAR_ShowBand (infoPtr, wParam, lParam);

	case RB_SIZETORECT:
	    return REBAR_SizeToRect (infoPtr, wParam, lParam);


/*    Messages passed to parent */
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_NOTIFY:
            return SendMessageW(REBAR_GetNotifyParent (infoPtr), uMsg, wParam, lParam);


/*      case WM_CHARTOITEM:     supported according to ControlSpy */

	case WM_CREATE:
	    return REBAR_Create (infoPtr, wParam, lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (infoPtr, wParam, lParam);

        case WM_ERASEBKGND:
	    return REBAR_EraseBkGnd (infoPtr, wParam, lParam);

	case WM_GETFONT:
	    return REBAR_GetFont (infoPtr, wParam, lParam);

/*      case WM_LBUTTONDBLCLK:  supported according to ControlSpy */

	case WM_LBUTTONDOWN:
	    return REBAR_LButtonDown (infoPtr, wParam, lParam);

	case WM_LBUTTONUP:
	    return REBAR_LButtonUp (infoPtr, wParam, lParam);

/*      case WM_MEASUREITEM:    supported according to ControlSpy */

	case WM_MOUSEMOVE:
	    return REBAR_MouseMove (infoPtr, wParam, lParam);

	case WM_MOUSELEAVE:
	    return REBAR_MouseLeave (infoPtr, wParam, lParam);

	case WM_NCCALCSIZE:
	    return REBAR_NCCalcSize (infoPtr, wParam, lParam);

        case WM_NCCREATE:
	    return REBAR_NCCreate (hwnd, wParam, lParam);

        case WM_NCHITTEST:
	    return REBAR_NCHitTest (infoPtr, wParam, lParam);

	case WM_NCPAINT:
	    return REBAR_NCPaint (infoPtr, wParam, lParam);

        case WM_NOTIFYFORMAT:
	    return REBAR_NotifyFormat (infoPtr, wParam, lParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return REBAR_Paint (infoPtr, wParam, lParam);

/*      case WM_PALETTECHANGED: supported according to ControlSpy */
/*      case WM_QUERYNEWPALETTE:supported according to ControlSpy */
/*      case WM_RBUTTONDOWN:    supported according to ControlSpy */
/*      case WM_RBUTTONUP:      supported according to ControlSpy */

	case WM_SETCURSOR:
	    return REBAR_SetCursor (infoPtr, wParam, lParam);

	case WM_SETFONT:
	    return REBAR_SetFont (infoPtr, wParam, lParam);

        case WM_SETREDRAW:
	    return REBAR_SetRedraw (infoPtr, wParam, lParam);

	case WM_SIZE:
	    return REBAR_Size (infoPtr, wParam, lParam);

        case WM_STYLECHANGED:
	    return REBAR_StyleChanged (infoPtr, wParam, lParam);

        case WM_THEMECHANGED:
            return theme_changed (infoPtr);

/*      case WM_SYSCOLORCHANGE: supported according to ControlSpy */
/*      "Applications that have brushes using the existing system colors
         should delete those brushes and recreate them using the new
         system colors."  per MSDN                                */

/*      case WM_VKEYTOITEM:     supported according to ControlSpy */
/*	case WM_WININICHANGE: */

        case WM_WINDOWPOSCHANGED:
	    return REBAR_WindowPosChanged (infoPtr, wParam, lParam);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
		ERR("unknown msg %04x wp=%08lx lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


VOID
REBAR_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = REBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(REBAR_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
#if GLATESTING
    wndClass.hbrBackground = CreateSolidBrush(RGB(0,128,0));
#endif
    wndClass.lpszClassName = REBARCLASSNAMEW;

    RegisterClassW (&wndClass);

    mindragx = GetSystemMetrics (SM_CXDRAG);
    mindragy = GetSystemMetrics (SM_CYDRAG);

}


VOID
REBAR_Unregister (void)
{
    UnregisterClassW (REBARCLASSNAMEW, NULL);
}
