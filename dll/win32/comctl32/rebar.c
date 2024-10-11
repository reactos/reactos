/*
 * Rebar control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 2007, 2008 Mikolaj Zalewski
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
 *    one pass (together with the RBN_CHILDSIZEs) is made on
 *    at least RB_INSERTBAND
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vssym32.h"
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

    INT     cxEffective;    /* current cx for band */
    UINT    cyHeader;       /* the height of the header */
    UINT    cxMinBand;      /* minimum cx for band */
    UINT    cyMinBand;      /* minimum cy for band */

    UINT    cyRowSoFar;     /* for RBS_VARHEIGHT - the height of the row if it would break on this band (set by _Layout) */
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

/* has a value of: 0, CCS_TOP, CCS_NOMOVEY, CCS_BOTTOM */
#define CCS_LAYOUT_MASK 0x3

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
    BOOL     DoRedraw;    /* TRUE to actually draw bands */
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

    HDPA     bands;       /* pointer to the array of rebar bands */
} REBAR_INFO;

/* fStatus flags */
#define BEGIN_DRAG_ISSUED   0x00000001
#define SELF_RESIZE         0x00000002
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

/* minimum vertical height of a normal bar                        */
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

static LRESULT REBAR_NotifyFormat(REBAR_INFO *infoPtr, LPARAM lParam);
static void REBAR_AutoSize(REBAR_INFO *infoPtr, BOOL needsLayout);

/* no index check here */
static inline REBAR_BAND* REBAR_GetBand(const REBAR_INFO *infoPtr, INT i)
{
    assert(i >= 0 && i < infoPtr->uNumBands);
    return DPA_GetPtr(infoPtr->bands, i);
}

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
    "RBBS_USECHEVRON",         /* 0200 */
    "RBBS_HIDETITLE",          /* 0400 */
    "RBBS_TOPALIGN",           /* 0800 */
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
    "RBBIM_CHEVRONLOCATION", /*  0x00001000 */
    "RBBIM_CHEVRONSTATE",  /*    0x00002000 */
    NULL };

static const WCHAR themeClass[] = L"Rebar";

static CHAR *
REBAR_FmtStyle(char *buffer, UINT style)
{
    INT i = 0;

    *buffer = 0;
    while (band_stylename[i]) {
	if (style & (1<<i)) {
	    if (*buffer) strcat(buffer, " | ");
	    strcat(buffer, band_stylename[i]);
	}
	i++;
    }
    return buffer;
}


static CHAR *
REBAR_FmtMask(char *buffer, UINT mask)
{
    INT i = 0;

    *buffer = 0;
    while (band_maskname[i]) {
	if (mask & (1<<i)) {
	    if (*buffer) strcat(buffer, " | ");
	    strcat(buffer, band_maskname[i]);
	}
	i++;
    }
    return buffer;
}


static VOID
REBAR_DumpBandInfo(const REBARBANDINFOW *pB)
{
    char buff[300];

    if( !TRACE_ON(rebar) ) return;
    TRACE("band info: ");
    if (pB->fMask & RBBIM_ID)
        TRACE("ID=%u, ", pB->wID);
    TRACE("size=%u, child=%p", pB->cbSize, pB->hwndChild);
    if (pB->fMask & RBBIM_COLORS)
        TRACE(", clrF=%#lx, clrB=%#lx", pB->clrFore, pB->clrBack);
    TRACE("\n");

    TRACE("band info: mask=0x%08x (%s)\n", pB->fMask, REBAR_FmtMask(buff, pB->fMask));
    if (pB->fMask & RBBIM_STYLE)
	TRACE("band info: style=0x%08x (%s)\n", pB->fStyle, REBAR_FmtStyle(buff, pB->fStyle));
    if (pB->fMask & (RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_HEADERSIZE | RBBIM_LPARAM )) {
	TRACE("band info:");
	if (pB->fMask & RBBIM_SIZE)
	    TRACE(" cx=%u", pB->cx);
	if (pB->fMask & RBBIM_IDEALSIZE)
	    TRACE(" xIdeal=%u", pB->cxIdeal);
	if (pB->fMask & RBBIM_HEADERSIZE)
	    TRACE(" xHeader=%u", pB->cxHeader);
	if (pB->fMask & RBBIM_LPARAM)
	    TRACE(" lParam %Ix", pB->lParam);
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
    char buff[300];
    REBAR_BAND *pB;
    UINT i;

    if(! TRACE_ON(rebar) ) return;

    TRACE("hwnd=%p: color=%#lx/%#lx, bands=%u, rows=%u, cSize=%ld,%ld\n",
	  iP->hwndSelf, iP->clrText, iP->clrBk, iP->uNumBands, iP->uNumRows,
	  iP->calcSize.cx, iP->calcSize.cy);
    TRACE("hwnd=%p: flags=%#x, dragStart=%ld,%ld, dragNow=%ld,%ld, iGrabbedBand=%d\n",
	  iP->hwndSelf, iP->fStatus, iP->dragStart.x, iP->dragStart.y,
	  iP->dragNow.x, iP->dragNow.y,
	  iP->iGrabbedBand);
    TRACE("hwnd=%p: style=%#lx, notify in Unicode=%s, redraw=%s\n",
          iP->hwndSelf, iP->dwStyle, (iP->bUnicode)?"TRUE":"FALSE",
          (iP->DoRedraw)?"TRUE":"FALSE");
    for (i = 0; i < iP->uNumBands; i++) {
	pB = REBAR_GetBand(iP, i);
	TRACE("band # %u:", i);
	if (pB->fMask & RBBIM_ID)
	    TRACE(" ID=%u", pB->wID);
	if (pB->fMask & RBBIM_CHILD)
	    TRACE(" child=%p", pB->hwndChild);
	if (pB->fMask & RBBIM_COLORS)
            TRACE(" clrF=%#lx clrB=%#lx", pB->clrFore, pB->clrBack);
	TRACE("\n");
	TRACE("band # %u: mask=0x%08x (%s)\n", i, pB->fMask, REBAR_FmtMask(buff, pB->fMask));
	if (pB->fMask & RBBIM_STYLE)
	    TRACE("band # %u: style=0x%08x (%s)\n", i, pB->fStyle, REBAR_FmtStyle(buff, pB->fStyle));
	TRACE("band # %u: xHeader=%u",
	      i, pB->cxHeader);
	if (pB->fMask & (RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_LPARAM )) {
	    if (pB->fMask & RBBIM_SIZE)
		TRACE(" cx=%u", pB->cx);
	    if (pB->fMask & RBBIM_IDEALSIZE)
		TRACE(" xIdeal=%u", pB->cxIdeal);
	    if (pB->fMask & RBBIM_LPARAM)
		TRACE(" lParam %Ix", pB->lParam);
	}
	TRACE("\n");
	if (RBBIM_CHILDSIZE)
	    TRACE("band # %u: xMin=%u, yMin=%u, yChild=%u, yMax=%u, yIntgl=%u\n",
		  i, pB->cxMinChild, pB->cyMinChild, pB->cyChild, pB->cyMaxChild, pB->cyIntegral);
	if (pB->fMask & RBBIM_TEXT)
	    TRACE("band # %u: text=%s\n",
		  i, (pB->lpText) ? debugstr_w(pB->lpText) : "(null)");
        TRACE("band # %u: cxMinBand=%u, cxEffective=%u, cyMinBand=%u\n",
              i, pB->cxMinBand, pB->cxEffective, pB->cyMinBand);
        TRACE("band # %u: fStatus=%08x, fDraw=%08x, Band=(%s), Grip=(%s)\n",
              i, pB->fStatus, pB->fDraw, wine_dbgstr_rect(&pB->rcBand),
              wine_dbgstr_rect(&pB->rcGripper));
        TRACE("band # %u: Img=(%s), Txt=(%s), Child=(%s)\n",
              i, wine_dbgstr_rect(&pB->rcCapImage),
              wine_dbgstr_rect(&pB->rcCapText), wine_dbgstr_rect(&pB->rcChild));
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

static int round_child_height(const REBAR_BAND *lpBand, int cyHeight)
{
    int cy = 0;
    if (lpBand->cyIntegral == 0)
        return cyHeight;
    cy = max(cyHeight - (int)lpBand->cyMinChild, 0);
    cy = lpBand->cyMinChild + (cy/lpBand->cyIntegral) * lpBand->cyIntegral;
    cy = min(cy, lpBand->cyMaxChild);
    return cy;
}

static void update_min_band_height(const REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{
    lpBand->cyMinBand = max(lpBand->cyHeader,
        (lpBand->hwndChild ? lpBand->cyChild + REBARSPACE(lpBand) : REBAR_NO_CHILD_HEIGHT));
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
    LineTo (hdc, x+1, y);
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

    return SendMessageW(parent, WM_NOTIFY, nmhdr->idFrom, (LPARAM)nmhdr);
}

static INT
REBAR_Notify_NMREBAR (const REBAR_INFO *infoPtr, UINT uBand, UINT code)
{
    NMREBAR notify_rebar;

    notify_rebar.dwMask = 0;
    if (uBand != -1) {
	REBAR_BAND *lpBand = REBAR_GetBand(infoPtr, uBand);

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
	lpBand = REBAR_GetBand(infoPtr, i);

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
    UINT i, xoff;
    RECT work;

    for(i=rstart; i<rend; i++){
      lpBand = REBAR_GetBand(infoPtr, i);
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
          InflateRect(&lpBand->rcGripper, 0, -2);

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
      if (lpBand->hwndChild) {

          lpBand->rcChild.left  = lpBand->rcBand.left + lpBand->cxHeader;
          lpBand->rcChild.right = lpBand->rcBand.right - REBAR_POST_CHILD;

          if (lpBand->cyChild > 0) {

              UINT yoff = (lpBand->rcBand.bottom - lpBand->rcBand.top - lpBand->cyChild) / 2;

              /* center child if height is known */
              lpBand->rcChild.top = lpBand->rcBand.top + yoff;
              lpBand->rcChild.bottom = lpBand->rcBand.top + yoff + lpBand->cyChild;
          }
          else {
              lpBand->rcChild.top = lpBand->rcBand.top;
              lpBand->rcChild.bottom = lpBand->rcBand.bottom;
          }

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
	  lpBand->fDraw &= ~NTF_INVALIDATE;
	  work = lpBand->rcBand;
	  work.right += SEP_WIDTH;
	  work.bottom += SEP_WIDTH;
	  TRACE("invalidating %s\n", wine_dbgstr_rect(&work));
	  InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
	  if (lpBand->hwndChild) InvalidateRect(lpBand->hwndChild, NULL, TRUE);
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
        lpBand = REBAR_GetBand(infoPtr, i);
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
                InflateRect(&lpBand->rcGripper, -2, 0);
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
	if (lpBand->hwndChild) {
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
	    lpBand->fDraw &= ~NTF_INVALIDATE;
	    work = rcBand;
	    work.bottom += SEP_WIDTH;
	    work.right += SEP_WIDTH;
	    TRACE("invalidating %s\n", wine_dbgstr_rect(&work));
	    InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
	    if (lpBand->hwndChild) InvalidateRect(lpBand->hwndChild, NULL, TRUE);
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

    TRACE("new size [%ld x %ld]\n", infoPtr->calcSize.cx, infoPtr->calcSize.cy);

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

    TRACE("hwnd %p, style %#lx, setting at (%d,%d) for (%d,%d)\n", infoPtr->hwndSelf, infoPtr->dwStyle,
            x, y, width, height);

    /* Set flag to ignore next WM_SIZE message and resize the window */
    infoPtr->fStatus |= SELF_RESIZE;
    if ((infoPtr->dwStyle & CCS_VERT) == 0)
        SetWindowPos(infoPtr->hwndSelf, 0, x, y, width, height, SWP_NOZORDER);
    else
        SetWindowPos(infoPtr->hwndSelf, 0, y, x, height, width, SWP_NOZORDER);
    infoPtr->fStatus &= ~SELF_RESIZE;
}


static VOID
REBAR_MoveChildWindows (const REBAR_INFO *infoPtr, UINT start, UINT endplus)
{
    REBAR_BAND *lpBand;
    WCHAR szClassName[40];
    UINT i;
    NMREBARCHILDSIZE  rbcz;
    HDWP deferpos;

    if (!(deferpos = BeginDeferWindowPos(infoPtr->uNumBands)))
        ERR("BeginDeferWindowPos returned NULL\n");

    for (i = start; i < endplus; i++) {
	lpBand = REBAR_GetBand(infoPtr, i);

	if (HIDDENBAND(lpBand)) continue;
	if (lpBand->hwndChild) {
	    TRACE("hwndChild = %p\n", lpBand->hwndChild);

	    /* Always generate the RBN_CHILDSIZE even if child
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
                TRACE("    from (%s)  to (%s)\n",
                      wine_dbgstr_rect(&lpBand->rcChild),
                      wine_dbgstr_rect(&rbcz.rcChild));
		lpBand->rcChild = rbcz.rcChild;  /* *** ??? */
            }

	    GetClassNameW (lpBand->hwndChild, szClassName, ARRAY_SIZE(szClassName));
	    if (!lstrcmpW (szClassName, WC_COMBOBOXW) ||
		!lstrcmpW (szClassName, WC_COMBOBOXEXW)) {
		INT nEditHeight, yPos;
		RECT rc;

		/* special placement code for combo or comboex box */


		/* get size of edit line */
		GetWindowRect (lpBand->hwndChild, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		TRACE("moving child (Combo(Ex)) %p to (%ld,%d) for (%ld,%d)\n",
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
		TRACE("moving child (Other) %p to (%ld,%ld) for (%ld,%ld)\n",
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
}

/* Returns the next visible band (the first visible band in [i+1; infoPtr->uNumBands) )
 * or infoPtr->uNumBands if none */
static int next_visible(const REBAR_INFO *infoPtr, int i)
{
    unsigned int n;
    for (n = i + 1; n < infoPtr->uNumBands; n++)
        if (!HIDDENBAND(REBAR_GetBand(infoPtr, n)))
            break;
    return n;
}

/* Returns the previous visible band (the last visible band in [0; i) )
 * or -1 if none */
static int prev_visible(const REBAR_INFO *infoPtr, int i)
{
    int n;
    for (n = i - 1; n >= 0; n--)
        if (!HIDDENBAND(REBAR_GetBand(infoPtr, n)))
            break;
    return n;
}

/* Returns the first visible band or infoPtr->uNumBands if none */
static int first_visible(const REBAR_INFO *infoPtr)
{
    return next_visible(infoPtr, -1); /* this works*/
}

/* Returns the first visible band for the given row (or iBand if none) */
static int get_row_begin_for_band(const REBAR_INFO *infoPtr, INT iBand)
{
    int iLastBand = iBand;
    int iRow = REBAR_GetBand(infoPtr, iBand)->iRow;
    while ((iBand = prev_visible(infoPtr, iBand)) >= 0) {
        if (REBAR_GetBand(infoPtr, iBand)->iRow != iRow)
            break;
        else
            iLastBand = iBand;
    }
    return iLastBand;
}

/* Returns the first visible band for the next row (or infoPtr->uNumBands if none) */
static int get_row_end_for_band(const REBAR_INFO *infoPtr, INT iBand)
{
    int iRow = REBAR_GetBand(infoPtr, iBand)->iRow;
    while ((iBand = next_visible(infoPtr, iBand)) < infoPtr->uNumBands)
        if (REBAR_GetBand(infoPtr, iBand)->iRow != iRow)
            break;
    return iBand;
}

/* Compute the rcBand.{left,right} from the cxEffective bands widths computed earlier.
 * iBeginBand must be visible */
static void REBAR_SetRowRectsX(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand)
{
    int xPos = 0, i;
    for (i = iBeginBand; i < iEndBand; i = next_visible(infoPtr, i))
    {
        REBAR_BAND *lpBand = REBAR_GetBand(infoPtr, i);
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
    INT cxMinFirstBand = 0, i;

    cxMinFirstBand = REBAR_GetBand(infoPtr, iBeginBand)->cxMinBand;

    for (i = prev_visible(infoPtr, iEndBand); i >= iBeginBand; i = prev_visible(infoPtr, i))
        if (REBAR_GetBand(infoPtr, i)->cxEffective > cxMinFirstBand &&
          !(REBAR_GetBand(infoPtr, i)->fStyle & RBBS_FIXEDSIZE))
            break;

    if (i < iBeginBand)
        for (i = prev_visible(infoPtr, iEndBand); i >= iBeginBand; i = prev_visible(infoPtr, i))
            if (REBAR_GetBand(infoPtr, i)->cxMinBand == cxMinFirstBand)
                break;

    TRACE("Extra space for row [%d..%d) should be added to band %d\n", iBeginBand, iEndBand, i);
    return REBAR_GetBand(infoPtr, i);
}

/* Try to shrink the visible bands in [iBeginBand; iEndBand) by cxShrink, starting from the right */
static int REBAR_ShrinkBandsRTL(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand, INT cxShrink, BOOL bEnforce)
{
    REBAR_BAND *lpBand;
    INT width, i;

    TRACE("Shrinking bands [%d..%d) by %d, right-to-left\n", iBeginBand, iEndBand, cxShrink);
    for (i = prev_visible(infoPtr, iEndBand); i >= iBeginBand; i = prev_visible(infoPtr, i))
    {
        lpBand = REBAR_GetBand(infoPtr, i);

        width = max(lpBand->cxEffective - cxShrink, (int)lpBand->cxMinBand);
        cxShrink -= lpBand->cxEffective - width;
        lpBand->cxEffective = width;
        if (bEnforce && lpBand->cx > lpBand->cxEffective)
            lpBand->cx = lpBand->cxEffective;
        if (cxShrink == 0)
            break;
    }
    return cxShrink;
}


/* Try to shrink the visible bands in [iBeginBand; iEndBand) by cxShrink, starting from the left.
 * iBeginBand must be visible */
static int REBAR_ShrinkBandsLTR(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand, INT cxShrink, BOOL bEnforce)
{
    REBAR_BAND *lpBand;
    INT width, i;

    TRACE("Shrinking bands [%d..%d) by %d, left-to-right\n", iBeginBand, iEndBand, cxShrink);
    for (i = iBeginBand; i < iEndBand; i = next_visible(infoPtr, i))
    {
        lpBand = REBAR_GetBand(infoPtr, i);

        width = max(lpBand->cxEffective - cxShrink, (int)lpBand->cxMinBand);
        cxShrink -= lpBand->cxEffective - width;
        lpBand->cxEffective = width;
        if (bEnforce)
            lpBand->cx = lpBand->cxEffective;
        if (cxShrink == 0)
            break;
    }
    return cxShrink;
}

/* Tries to move a band to a given offset within a row. */
static int REBAR_MoveBandToRowOffset(REBAR_INFO *infoPtr, INT iBand, INT iFirstBand,
    INT iLastBand, INT xOff, BOOL reorder)
{
    REBAR_BAND *insertBand = REBAR_GetBand(infoPtr, iBand);
    int xPos = 0, i;
    const BOOL setBreak = REBAR_GetBand(infoPtr, iFirstBand)->fStyle & RBBS_BREAK;

    /* Find the band's new position */
    if(reorder)
    {
        /* Used during an LR band reorder drag */
        for (i = iFirstBand; i < iLastBand; i = next_visible(infoPtr, i))
        {
            if(xPos > xOff)
                break;
            xPos += REBAR_GetBand(infoPtr, i)->cxEffective + SEP_WIDTH;
        }
    }
    else
    {
        /* Used during a UD band insertion drag */
        for (i = iFirstBand; i < iLastBand; i = next_visible(infoPtr, i))
        {
            const REBAR_BAND *band = REBAR_GetBand(infoPtr, i);
            if(xPos + band->cxMinBand / 2 > xOff)
                break;
            xPos += band->cxEffective + SEP_WIDTH;
        }
    }

    /* Move the band to its new position */
    DPA_DeletePtr(infoPtr->bands, iBand);
    if(i > iBand)
        i--;
    DPA_InsertPtr(infoPtr->bands, i, insertBand);

    /* Ensure only the last band has the RBBS_BREAK flag set */
    insertBand->fStyle &= ~RBBS_BREAK;
    if(setBreak)
        REBAR_GetBand(infoPtr, iFirstBand)->fStyle |= RBBS_BREAK;

    /* Return the currently grabbed band */
    if(infoPtr->iGrabbedBand == iBand)
    {
        infoPtr->iGrabbedBand = i;
        return i;
    }
    else return -1;
}

/* Set the heights of the visible bands in [iBeginBand; iEndBand) to the max height. iBeginBand must be visible */
static int REBAR_SetBandsHeight(const REBAR_INFO *infoPtr, INT iBeginBand, INT iEndBand, INT yStart)
{
    REBAR_BAND *lpBand;
    int yMaxHeight = 0;
    int yPos = yStart;
    int row, i;

    for (i = iBeginBand; i < iEndBand; i = next_visible(infoPtr, i))
    {
        lpBand = REBAR_GetBand(infoPtr, i);
        lpBand->cyRowSoFar = yMaxHeight;
        yMaxHeight = max(yMaxHeight, lpBand->cyMinBand);
    }
    TRACE("Bands [%d; %d) height: %d\n", iBeginBand, iEndBand, yMaxHeight);

    row = iBeginBand < iEndBand ? REBAR_GetBand(infoPtr, iBeginBand)->iRow : 0;

    for (i = iBeginBand; i < iEndBand; i = next_visible(infoPtr, i))
    {
        lpBand = REBAR_GetBand(infoPtr, i);
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

/* Layout the row [iBeginBand; iEndBand). iBeginBand must be visible */
static void REBAR_LayoutRow(const REBAR_INFO *infoPtr, int iBeginBand, int iEndBand, int cx, int *piRow, int *pyPos)
{
    REBAR_BAND *lpBand;
    int i, extra;
    int width = 0;

    TRACE("Adjusting row [%d;%d). Width: %d\n", iBeginBand, iEndBand, cx);
    for (i = iBeginBand; i < iEndBand; i++)
        REBAR_GetBand(infoPtr, i)->iRow = *piRow;

    /* compute the extra space */
    for (i = iBeginBand; i < iEndBand; i = next_visible(infoPtr, i))
    {
        lpBand = REBAR_GetBand(infoPtr, i);
        if (i > iBeginBand)
            width += SEP_WIDTH;
        lpBand->cxEffective = max(lpBand->cxMinBand, lpBand->cx);
        width += lpBand->cxEffective;
    }

    extra = cx - width;
    TRACE("Extra space: %d\n", extra);
    if (extra < 0) {
        int ret = REBAR_ShrinkBandsRTL(infoPtr, iBeginBand, iEndBand, -extra, FALSE);
        if (ret > 0 && next_visible(infoPtr, iBeginBand) != iEndBand)  /* one band may be longer than expected... */
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
REBAR_Layout(REBAR_INFO *infoPtr)
{
    REBAR_BAND *lpBand;
    RECT rcAdj;
    SIZE oldSize;
    INT adjcx, i;
    INT rowstart;
    INT row = 0;
    INT xMin, yPos;

    if (infoPtr->dwStyle & (CCS_NORESIZE | CCS_NOPARENTALIGN) || GetParent(infoPtr->hwndSelf) == NULL)
        GetClientRect(infoPtr->hwndSelf, &rcAdj);
    else
        GetClientRect(GetParent(infoPtr->hwndSelf), &rcAdj);
    TRACE("adjustment rect is (%s)\n", wine_dbgstr_rect(&rcAdj));

    adjcx = get_rect_cx(infoPtr, &rcAdj);

    if (infoPtr->uNumBands == 0) {
        TRACE("No bands - setting size to (0,%d), style: %#lx\n", adjcx, infoPtr->dwStyle);
        infoPtr->calcSize.cx = adjcx;
        /* the calcSize.cy won't change for a 0 band rebar */
        infoPtr->uNumRows = 0;
        REBAR_ForceResize(infoPtr);
        return;
    }

    yPos = 0;
    xMin = 0;
    rowstart = first_visible(infoPtr);
    /* divide rows */
    for (i = rowstart; i < infoPtr->uNumBands; i = next_visible(infoPtr, i))
    {
        lpBand = REBAR_GetBand(infoPtr, i);

        if (i > rowstart && (lpBand->fStyle & RBBS_BREAK || xMin + lpBand->cxMinBand > adjcx)) {
            TRACE("%s break on band %d\n", (lpBand->fStyle & RBBS_BREAK ? "Hard" : "Soft"), i - 1);
            REBAR_LayoutRow(infoPtr, rowstart, i, adjcx, &row, &yPos);
            rowstart = i;
            xMin = 0;
        }
        else
            xMin += SEP_WIDTH;

        xMin += lpBand->cxMinBand;
    }
    if (rowstart < infoPtr->uNumBands)
        REBAR_LayoutRow(infoPtr, rowstart, infoPtr->uNumBands, adjcx, &row, &yPos);

    if (!(infoPtr->dwStyle & RBS_VARHEIGHT))
        yPos = REBAR_SetBandsHeight(infoPtr, first_visible(infoPtr), infoPtr->uNumBands, 0);

    infoPtr->uNumRows = row;

    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, 0, infoPtr->uNumBands);
    else
        REBAR_CalcHorzBand(infoPtr, 0, infoPtr->uNumBands);
    /* now compute size of Rebar itself */
    oldSize = infoPtr->calcSize;

    infoPtr->calcSize.cx = adjcx;
    infoPtr->calcSize.cy = yPos;
    TRACE("calcsize size=(%ld, %ld), origheight=(%ld,%ld)\n", infoPtr->calcSize.cx, infoPtr->calcSize.cy,
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
        REBAR_AutoSize(infoPtr, FALSE);
    }
}

/* iBeginBand must be visible */
static int
REBAR_SizeChildrenToHeight(const REBAR_INFO *infoPtr, int iBeginBand, int iEndBand, int extra, BOOL *fChanged)
{
    int cyBandsOld;
    int cyBandsNew = 0;
    int i;

    TRACE("[%d;%d) by %d\n", iBeginBand, iEndBand, extra);

    cyBandsOld = REBAR_GetBand(infoPtr, iBeginBand)->rcBand.bottom -
                 REBAR_GetBand(infoPtr, iBeginBand)->rcBand.top;
    for (i = iBeginBand; i < iEndBand; i = next_visible(infoPtr, i))
    {
        REBAR_BAND *lpBand = REBAR_GetBand(infoPtr, i);
        int cyMaxChild = cyBandsOld - REBARSPACE(lpBand) + extra;
        int cyChild = round_child_height(lpBand, cyMaxChild);

        if (lpBand->hwndChild && cyChild != lpBand->cyChild && (lpBand->fStyle & RBBS_VARIABLEHEIGHT))
        {
            TRACE("Resizing %d: %d -> %d [%d]\n", i, lpBand->cyChild, cyChild, lpBand->cyMaxChild);
            *fChanged = TRUE;
            lpBand->cyChild = cyChild;
            lpBand->fDraw |= NTF_INVALIDATE;
            update_min_band_height(infoPtr, lpBand);
        }
        cyBandsNew = max(cyBandsNew, lpBand->cyMinBand);
    }
    return cyBandsNew - cyBandsOld;
}

/* worker function for RB_SIZETORECT and RBS_AUTOSIZE */
static VOID
REBAR_SizeToHeight(REBAR_INFO *infoPtr, int height)
{
    int extra = height - infoPtr->calcSize.cy;  /* may be negative */
    BOOL fChanged = FALSE;
    UINT uNumRows = infoPtr->uNumRows;
    int i;

    if (uNumRows == 0)  /* avoid division by 0 */
        return;

    /* That's not exactly what Windows does but should be similar */

    /* Pass one: break-up/glue rows */
    if (extra > 0)
    {
        for (i = prev_visible(infoPtr, infoPtr->uNumBands); i > 0; i = prev_visible(infoPtr, i))
        {
            REBAR_BAND *lpBand = REBAR_GetBand(infoPtr, i);
            int cyBreakExtra;  /* additional cy for the rebar after a RBBS_BREAK on this band */

            height = lpBand->rcBand.bottom - lpBand->rcBand.top;

            if (infoPtr->dwStyle & RBS_VARHEIGHT)
                cyBreakExtra = lpBand->cyRowSoFar; /* 'height' => 'lpBand->cyRowSoFar' + 'height'*/
            else
                cyBreakExtra = height;             /* 'height' => 'height' + 'height'*/
            cyBreakExtra += SEP_WIDTH;

            if (extra <= cyBreakExtra / 2)
                break;

            if (!(lpBand->fStyle & RBBS_BREAK))
            {
                TRACE("Adding break on band %d - extra %d -> %d\n", i, extra, extra - cyBreakExtra);
                lpBand->fStyle |= RBBS_BREAK;
                lpBand->fDraw |= NTF_INVALIDATE;
                fChanged = TRUE;
                extra -= cyBreakExtra;
                uNumRows++;
                /* temporary change for _SizeControlsToHeight. The true values will be computed in _Layout */
                if (infoPtr->dwStyle & RBS_VARHEIGHT)
                    lpBand->rcBand.bottom = lpBand->rcBand.top + lpBand->cyMinBand;
            }
        }
    }
    /* TODO: else if (extra < 0) { try to remove some RBBS_BREAKs } */

    /* Pass two: increase/decrease control height */
    if (infoPtr->dwStyle & RBS_VARHEIGHT)
    {
        int i = first_visible(infoPtr);
        int iRow = 0;
        while (i < infoPtr->uNumBands)
        {
            REBAR_BAND *lpBand = REBAR_GetBand(infoPtr, i);
            int extraForRow = extra / (int)(uNumRows - iRow);
            int rowEnd;

            /* we can't use get_row_end_for_band as we might have added RBBS_BREAK in the first phase */
            for (rowEnd = next_visible(infoPtr, i); rowEnd < infoPtr->uNumBands; rowEnd = next_visible(infoPtr, rowEnd))
                if (REBAR_GetBand(infoPtr, rowEnd)->iRow != lpBand->iRow ||
                    REBAR_GetBand(infoPtr, rowEnd)->fStyle & RBBS_BREAK)
                    break;

            extra -= REBAR_SizeChildrenToHeight(infoPtr, i, rowEnd, extraForRow, &fChanged);
            TRACE("extra = %d\n", extra);
            i = rowEnd;
            iRow++;
        }
    }
    else
        REBAR_SizeChildrenToHeight(infoPtr, first_visible(infoPtr), infoPtr->uNumBands, extra / infoPtr->uNumRows, &fChanged);

    if (fChanged)
        REBAR_Layout(infoPtr);
}

static VOID
REBAR_AutoSize(REBAR_INFO *infoPtr, BOOL needsLayout)
{
    RECT rc, rcNew;
    NMRBAUTOSIZE autosize;

    if (needsLayout)
        REBAR_Layout(infoPtr);
    GetClientRect(infoPtr->hwndSelf, &rc);
    REBAR_SizeToHeight(infoPtr, get_rect_cy(infoPtr, &rc));
    GetClientRect(infoPtr->hwndSelf, &rcNew);

    GetClientRect(infoPtr->hwndSelf, &autosize.rcTarget);
    autosize.fChanged = EqualRect(&rc, &rcNew);
    autosize.rcTarget = rc;
    autosize.rcActual = rcNew;
    REBAR_Notify((NMHDR *)&autosize, infoPtr, RBN_AUTOSIZE);
}

static VOID
REBAR_ValidateBand (const REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
     /* Function:  This routine evaluates the band specs supplied */
     /*  by the user and updates the following 5 fields in        */
     /*  the internal band structure: cxHeader, cyHeader, cxMinBand, cyMinBand, fStatus */
{
    UINT header=0;
    UINT textheight=0, imageheight = 0;
    UINT i, nonfixed;
    REBAR_BAND *tBand;

    lpBand->fStatus = 0;
    lpBand->cxMinBand = 0;
    lpBand->cyMinBand = 0;

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
	tBand = REBAR_GetBand(infoPtr, i);
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
    if (lpBand->iImage != -1 && (infoPtr->himl)) {
	lpBand->fStatus |= HAS_IMAGE;
        if (infoPtr->dwStyle & CCS_VERT) {
	   header += (infoPtr->imageSize.cy + REBAR_POST_IMAGE);
           imageheight = infoPtr->imageSize.cx + 4;
	}
	else {
	   header += (infoPtr->imageSize.cx + REBAR_POST_IMAGE);
           imageheight = infoPtr->imageSize.cy + 4;
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
    lpBand->cyHeader = max(textheight, imageheight);

    /* Now compute minimum size of child window */
    update_min_band_height(infoPtr, lpBand);       /* update lpBand->cyMinBand from cyHeader and cyChild*/

    lpBand->cxMinBand = lpBand->cxMinChild + lpBand->cxHeader + REBAR_POST_CHILD;
    if (lpBand->fStyle & RBBS_USECHEVRON && lpBand->cxMinChild < lpBand->cxIdeal)
        lpBand->cxMinBand += CHEVRON_WIDTH;
}

static UINT
REBAR_CommonSetupBand(HWND hwnd, const REBARBANDINFOW *lprbbi, REBAR_BAND *lpBand)
     /* Function:  This routine copies the supplied values from   */
     /*  user input (lprbbi) to the internal band structure.      */
     /*  It returns the mask of what changed.   */
{
    UINT uChanged = 0x0;

    lpBand->fMask |= lprbbi->fMask;

    if( (lprbbi->fMask & RBBIM_STYLE) &&
        (lpBand->fStyle != lprbbi->fStyle ) )
    {
	lpBand->fStyle = lprbbi->fStyle;
        uChanged |= RBBIM_STYLE;
    }

    if( (lprbbi->fMask & RBBIM_COLORS) &&
       ( ( lpBand->clrFore != lprbbi->clrFore ) ||
         ( lpBand->clrBack != lprbbi->clrBack ) ) )
    {
	lpBand->clrFore = lprbbi->clrFore;
	lpBand->clrBack = lprbbi->clrBack;
        uChanged |= RBBIM_COLORS;
    }

    if( (lprbbi->fMask & RBBIM_IMAGE) &&
       ( lpBand->iImage != lprbbi->iImage ) )
    {
	lpBand->iImage = lprbbi->iImage;
        uChanged |= RBBIM_IMAGE;
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
        uChanged |= RBBIM_CHILD;
    }

    if( (lprbbi->fMask & RBBIM_CHILDSIZE) &&
        ( (lpBand->cxMinChild != lprbbi->cxMinChild) ||
          (lpBand->cyMinChild != lprbbi->cyMinChild ) ||
          ( (lprbbi->cbSize >= REBARBANDINFOA_V6_SIZE && (lpBand->fStyle & RBBS_VARIABLEHEIGHT)) &&
            ( (lpBand->cyChild    != lprbbi->cyChild ) ||
              (lpBand->cyMaxChild != lprbbi->cyMaxChild ) ||
              (lpBand->cyIntegral != lprbbi->cyIntegral ) ) ) ||
          ( (lprbbi->cbSize < REBARBANDINFOA_V6_SIZE) &&
            ( (lpBand->cyChild || 
               lpBand->cyMaxChild || 
               lpBand->cyIntegral ) ) ) ) )
    {
	lpBand->cxMinChild = lprbbi->cxMinChild;
	lpBand->cyMinChild = lprbbi->cyMinChild;
        /* These fields where added in WIN32_IE == 0x400 and are set only for RBBS_VARIABLEHEIGHT bands */
        if (lprbbi->cbSize >= REBARBANDINFOA_V6_SIZE && (lpBand->fStyle & RBBS_VARIABLEHEIGHT)) {
	    lpBand->cyMaxChild = lprbbi->cyMaxChild;
            lpBand->cyIntegral = lprbbi->cyIntegral;

            lpBand->cyChild = round_child_height(lpBand, lprbbi->cyChild);  /* make (cyChild - cyMinChild) a multiple of cyIntergral */
        }
	else {
	    lpBand->cyChild    = lpBand->cyMinChild;
	    lpBand->cyMaxChild = 0x7fffffff;
	    lpBand->cyIntegral = 0;
	}
        uChanged |= RBBIM_CHILDSIZE;
    }

    if( (lprbbi->fMask & RBBIM_SIZE) &&
        (lpBand->cx != lprbbi->cx ) )
    {
	lpBand->cx = lprbbi->cx;
        uChanged |= RBBIM_SIZE;
    }

    if( (lprbbi->fMask & RBBIM_BACKGROUND) &&
       ( lpBand->hbmBack != lprbbi->hbmBack ) )
    {
	lpBand->hbmBack = lprbbi->hbmBack;
        uChanged |= RBBIM_BACKGROUND;
    }

    if( (lprbbi->fMask & RBBIM_ID) &&
        (lpBand->wID != lprbbi->wID ) )
    {
	lpBand->wID = lprbbi->wID;
        uChanged |= RBBIM_ID;
    }

    /* check for additional data */
    if (lprbbi->cbSize >= REBARBANDINFOA_V6_SIZE) {
	if( (lprbbi->fMask & RBBIM_IDEALSIZE) &&
            ( lpBand->cxIdeal != lprbbi->cxIdeal ) )
        {
	    lpBand->cxIdeal = lprbbi->cxIdeal;
            uChanged |= RBBIM_IDEALSIZE;
        }

	if( (lprbbi->fMask & RBBIM_LPARAM) &&
            (lpBand->lParam != lprbbi->lParam ) )
        {
	    lpBand->lParam = lprbbi->lParam;
            uChanged |= RBBIM_LPARAM;
        }

	if( (lprbbi->fMask & RBBIM_HEADERSIZE) &&
            (lpBand->cxHeader != lprbbi->cxHeader ) )
        {
	    lpBand->cxHeader = lprbbi->cxHeader;
            lpBand->fStyle |= RBBS_UNDOC_FIXEDHEADER;
            uChanged |= RBBIM_HEADERSIZE;
        }
    }

    return uChanged;
}

static LRESULT REBAR_EraseBkGnd (const REBAR_INFO *infoPtr, HDC hdc)
     /* Function:  This erases the background rectangle by drawing  */
     /*  each band with its background color (or the default) and   */
     /*  draws each bands right separator if necessary. The row     */
     /*  separators are drawn on the first band of the next row.    */
{
    REBAR_BAND *lpBand;
    UINT i;
    INT oldrow;
    RECT cr;
    COLORREF old = CLR_NONE, new;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);

    GetClientRect (infoPtr->hwndSelf, &cr);

    oldrow = -1;
    for(i=0; i<infoPtr->uNumBands; i++) {
        RECT rcBand;
        lpBand = REBAR_GetBand(infoPtr, i);
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
                TRACE ("drawing band separator bottom (%s)\n",
                       wine_dbgstr_rect(&rcRowSep));
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
            TRACE("drawing band separator right (%s)\n",
                  wine_dbgstr_rect(&rcSep));
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
            TRACE("%s background color %#lx, band %s\n",
                  (lpBand->clrBack == CLR_NONE) ? "none" :
                    ((lpBand->clrBack == CLR_DEFAULT) ? "dft" : ""),
                  GetBkColor(hdc), wine_dbgstr_rect(&rcBand));
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
		lpBand = REBAR_GetBand(infoPtr, iCount);
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
{
    REBAR_BAND *hitBand;
    INT iHitBand, iRowBegin, iRowEnd;
    INT movement, xBand, cxLeft = 0;
    BOOL shrunkBands = FALSE;

    iHitBand = infoPtr->iGrabbedBand;
    iRowBegin = get_row_begin_for_band(infoPtr, iHitBand);
    iRowEnd = get_row_end_for_band(infoPtr, iHitBand);
    hitBand = REBAR_GetBand(infoPtr, iHitBand);

    xBand = hitBand->rcBand.left;
    movement = (infoPtr->dwStyle&CCS_VERT ? ptsmove->y : ptsmove->x)
                    - (xBand + REBAR_PRE_GRIPPER - infoPtr->ihitoffset);

    /* Dragging the first band in a row cannot cause shrinking */
    if(iHitBand != iRowBegin)
    {
        if (movement < 0) {
            cxLeft = REBAR_ShrinkBandsRTL(infoPtr, iRowBegin, iHitBand, -movement, TRUE);

            if(cxLeft < -movement)
            {
                hitBand->cxEffective += -movement - cxLeft;
                hitBand->cx = hitBand->cxEffective;
                shrunkBands = TRUE;
            }

        } else if (movement > 0) {

            cxLeft = movement;
            if (prev_visible(infoPtr, iHitBand) >= 0)
                cxLeft = REBAR_ShrinkBandsLTR(infoPtr, iHitBand, iRowEnd, movement, TRUE);

            if(cxLeft < movement)
            {
                REBAR_BAND *lpPrev = REBAR_GetBand(infoPtr, prev_visible(infoPtr, iHitBand));
                lpPrev->cxEffective += movement - cxLeft;
                lpPrev->cx = hitBand->cxEffective;
                shrunkBands = TRUE;
            }

        }
    }

    if(!shrunkBands)
    {
        /* It was not possible to move the band by shrinking bands.
         * Try relocating the band instead. */
        REBAR_MoveBandToRowOffset(infoPtr, iHitBand, iRowBegin,
            iRowEnd, xBand + movement, TRUE);
    }

    REBAR_SetRowRectsX(infoPtr, iRowBegin, iRowEnd);
    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, 0, infoPtr->uNumBands);
    else
        REBAR_CalcHorzBand(infoPtr, 0, infoPtr->uNumBands);
    REBAR_MoveChildWindows(infoPtr, iRowBegin, iRowEnd);
}

static void
REBAR_HandleUDDrag (REBAR_INFO *infoPtr, const POINT *ptsmove)
{
    INT yOff = (infoPtr->dwStyle & CCS_VERT) ? ptsmove->x : ptsmove->y;
    INT iHitBand, iRowBegin, iNextRowBegin;
    REBAR_BAND *hitBand, *rowBeginBand;

    if(infoPtr->uNumBands <= 0)
        ERR("There are no bands in this rebar\n");

    /* Up/down dragging can only occur when there is more than one
     * band in the rebar */
    if(infoPtr->uNumBands <= 1)
        return;

    iHitBand = infoPtr->iGrabbedBand;
    hitBand = REBAR_GetBand(infoPtr, iHitBand);

    /* If we're taking a band that has the RBBS_BREAK style set, this
     * style needs to be reapplied to the band that is going to become
     * the new start of the row. */
    if((hitBand->fStyle & RBBS_BREAK) &&
        (iHitBand < infoPtr->uNumBands - 1))
        REBAR_GetBand(infoPtr, iHitBand + 1)->fStyle |= RBBS_BREAK;

    if(yOff < 0)
    {
        /* Place the band above the current top row */
        if(iHitBand==0 && (infoPtr->uNumBands==1 || REBAR_GetBand(infoPtr, 1)->fStyle&RBBS_BREAK))
            return;
        DPA_DeletePtr(infoPtr->bands, iHitBand);
        hitBand->fStyle &= ~RBBS_BREAK;
        REBAR_GetBand(infoPtr, 0)->fStyle |= RBBS_BREAK;
        infoPtr->iGrabbedBand = DPA_InsertPtr(
            infoPtr->bands, 0, hitBand);
    }
    else if(yOff > REBAR_GetBand(infoPtr, infoPtr->uNumBands - 1)->rcBand.bottom)
    {
        /* Place the band below the current bottom row */
        if(iHitBand == infoPtr->uNumBands-1 && hitBand->fStyle&RBBS_BREAK)
            return;
        DPA_DeletePtr(infoPtr->bands, iHitBand);
        hitBand->fStyle |= RBBS_BREAK;
        infoPtr->iGrabbedBand = DPA_InsertPtr(
            infoPtr->bands, infoPtr->uNumBands - 1, hitBand);
    }
    else
    {
        /* Place the band in the preexisting row the mouse is hovering over */
        iRowBegin = first_visible(infoPtr);
        while(iRowBegin < infoPtr->uNumBands)
        {
            iNextRowBegin = get_row_end_for_band(infoPtr, iRowBegin);
            rowBeginBand = REBAR_GetBand(infoPtr, iRowBegin);
            if(rowBeginBand->rcBand.bottom > yOff)
            {
                REBAR_MoveBandToRowOffset(
                    infoPtr, iHitBand, iRowBegin, iNextRowBegin,
                    ((infoPtr->dwStyle & CCS_VERT) ? ptsmove->y : ptsmove->x)
                        - REBAR_PRE_GRIPPER - infoPtr->ihitoffset, FALSE);
                break;
            }

            iRowBegin = iNextRowBegin;
        }
    }

    REBAR_Layout(infoPtr);
}


/* << REBAR_BeginDrag >> */


static LRESULT
REBAR_DeleteBand (REBAR_INFO *infoPtr, WPARAM wParam)
{
    UINT uBand = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("deleting band %u!\n", uBand);
    lpBand = REBAR_GetBand(infoPtr, uBand);
    REBAR_Notify_NMREBAR (infoPtr, uBand, RBN_DELETINGBAND);
    /* TODO: a return of 1 should probably cancel the deletion */

    if (lpBand->hwndChild)
        ShowWindow(lpBand->hwndChild, SW_HIDE);
    Free(lpBand->lpText);
    Free(lpBand);

    infoPtr->uNumBands--;
    DPA_DeletePtr(infoPtr->bands, uBand);

    REBAR_Notify_NMREBAR (infoPtr, -1, RBN_DELETEDBAND);

    /* if only 1 band left the re-validate to possible eliminate gripper */
    if (infoPtr->uNumBands == 1)
      REBAR_ValidateBand (infoPtr, REBAR_GetBand(infoPtr, 0));

    REBAR_Layout(infoPtr);

    return TRUE;
}


/* << REBAR_DragMove >> */
/* << REBAR_EndDrag >> */


static LRESULT
REBAR_GetBandBorders (const REBAR_INFO *infoPtr, UINT uBand, RECT *lpRect)
{
    REBAR_BAND *lpBand;

    if (!lpRect)
	return 0;
    if (uBand >= infoPtr->uNumBands)
	return 0;

    lpBand = REBAR_GetBand(infoPtr, uBand);

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
REBAR_GetBandInfoT(const REBAR_INFO *infoPtr, UINT uIndex, LPREBARBANDINFOW lprbbi, BOOL bUnicode)
{
    REBAR_BAND *lpBand;

    if (!lprbbi || lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;

    if (uIndex >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u (bUnicode=%d)\n", uIndex, bUnicode);

    /* copy band information */
    lpBand = REBAR_GetBand(infoPtr, uIndex);

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
        /* to make tests pass we follow Windows' behaviour and allow reading these fields only
         * for RBBS_VARIABLEHEIGHTS bands */
        if (lprbbi->cbSize >= REBARBANDINFOW_V6_SIZE && (lpBand->fStyle & RBBS_VARIABLEHEIGHT)) {
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
    if (lprbbi->cbSize >= REBARBANDINFOW_V6_SIZE) {
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
REBAR_GetBarHeight (const REBAR_INFO *infoPtr)
{
    INT nHeight;

    nHeight = infoPtr->calcSize.cy;

    TRACE("height = %d\n", nHeight);

    return nHeight;
}


static LRESULT
REBAR_GetBarInfo (const REBAR_INFO *infoPtr, LPREBARINFO lpInfo)
{
    if (!lpInfo || lpInfo->cbSize < sizeof (REBARINFO))
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

    TRACE("background color %#lx\n", clr);

    return clr;
}


/* << REBAR_GetColorScheme >> */
/* << REBAR_GetDropTarget >> */


static LRESULT
REBAR_GetPalette (const REBAR_INFO *infoPtr)
{
    FIXME("empty stub!\n");

    return 0;
}


static LRESULT
REBAR_GetRect (const REBAR_INFO *infoPtr, INT iBand, RECT *lprc)
{
    REBAR_BAND *lpBand;

    if (iBand < 0 || iBand >= infoPtr->uNumBands)
	return FALSE;
    if (!lprc)
	return FALSE;

    lpBand = REBAR_GetBand(infoPtr, iBand);
    /* For CCS_VERT the coordinates will be swapped - like on Windows */
    *lprc = lpBand->rcBand;

    TRACE("band %d, (%s)\n", iBand, wine_dbgstr_rect(lprc));

    return TRUE;
}


static inline LRESULT
REBAR_GetRowCount (const REBAR_INFO *infoPtr)
{
    TRACE("%u\n", infoPtr->uNumRows);

    return infoPtr->uNumRows;
}


static LRESULT
REBAR_GetRowHeight (const REBAR_INFO *infoPtr, INT iRow)
{
    int j = 0, ret = 0;
    UINT i;
    REBAR_BAND *lpBand;

    for (i=0; i<infoPtr->uNumBands; i++) {
	lpBand = REBAR_GetBand(infoPtr, i);
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
    TRACE("text color %#lx\n", infoPtr->clrText);

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
REBAR_HitTest (const REBAR_INFO *infoPtr, LPRBHITTESTINFO lprbht)
{
    if (!lprbht)
	return -1;

    REBAR_InternalHitTest (infoPtr, &lprbht->pt, &lprbht->flags, &lprbht->iBand);

    return lprbht->iBand;
}


static LRESULT
REBAR_IdToIndex (const REBAR_INFO *infoPtr, UINT uId)
{
    UINT i;

    if (infoPtr->uNumBands < 1)
	return -1;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	if (REBAR_GetBand(infoPtr, i)->wID == uId) {
	    TRACE("id %u is band %u found!\n", uId, i);
	    return i;
	}
    }

    TRACE("id %u is not found\n", uId);
    return -1;
}


static LRESULT
REBAR_InsertBandT(REBAR_INFO *infoPtr, INT iIndex, const REBARBANDINFOW *lprbbi, BOOL bUnicode)
{
    REBAR_BAND *lpBand;

    if (!lprbbi || lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;

    /* trace the index as signed to see the -1 */
    TRACE("insert band at %d (bUnicode=%d)!\n", iIndex, bUnicode);
    REBAR_DumpBandInfo(lprbbi);

    if (!(lpBand = Alloc(sizeof(REBAR_BAND)))) return FALSE;
    if ((iIndex == -1) || (iIndex > infoPtr->uNumBands))
        iIndex = infoPtr->uNumBands;
    if (DPA_InsertPtr(infoPtr->bands, iIndex, lpBand) == -1)
    {
        Free(lpBand);
        return FALSE;
    }
    infoPtr->uNumBands++;

    TRACE("index %d!\n", iIndex);

    /* initialize band */
    memset(lpBand, 0, sizeof(*lpBand));
    lpBand->clrFore = infoPtr->clrText == CLR_NONE ? infoPtr->clrBtnText :
                                                     infoPtr->clrText;
    lpBand->clrBack = infoPtr->clrBk == CLR_NONE ? infoPtr->clrBtnFace :
                                                   infoPtr->clrBk;
    lpBand->iImage = -1;

    REBAR_CommonSetupBand(infoPtr->hwndSelf, lprbbi, lpBand);

    /* Make sure the defaults for these are correct */
    if (lprbbi->cbSize < REBARBANDINFOA_V6_SIZE || !(lpBand->fStyle & RBBS_VARIABLEHEIGHT)) {
        lpBand->cyChild    = lpBand->cyMinChild;
        lpBand->cyMaxChild = 0x7fffffff;
        lpBand->cyIntegral = 0;
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
        if (bUnicode)
            Str_SetPtrW(&lpBand->lpText, lprbbi->lpText);
        else
            Str_SetPtrAtoW(&lpBand->lpText, (LPSTR)lprbbi->lpText);
    }

    REBAR_ValidateBand (infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (infoPtr, REBAR_GetBand(infoPtr, 0));

    REBAR_DumpBand (infoPtr);

    REBAR_Layout(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return TRUE;
}


static LRESULT
REBAR_MaximizeBand (const REBAR_INFO *infoPtr, INT iBand, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    int iRowBegin, iRowEnd;
    int cxDesired, extra, extraOrig;
    int cxIdealBand;

    /* Validate */
    if (infoPtr->uNumBands == 0 || iBand < 0 || iBand >= infoPtr->uNumBands) {
	/* error !!! */
	ERR("Illegal MaximizeBand, requested=%d, current band count=%d\n",
	      iBand, infoPtr->uNumBands);
      	return FALSE;
    }

    lpBand = REBAR_GetBand(infoPtr, iBand);

    if (lpBand->fStyle & RBBS_HIDDEN)
    {
        /* Windows is buggy and creates a hole */
        WARN("Ignoring maximize request on a hidden band (%d)\n", iBand);
        return FALSE;
    }

    cxIdealBand = lpBand->cxIdeal + lpBand->cxHeader + REBAR_POST_CHILD;
    if (lParam && (lpBand->cxEffective < cxIdealBand))
        cxDesired = cxIdealBand;
    else
        cxDesired = infoPtr->calcSize.cx;

    iRowBegin = get_row_begin_for_band(infoPtr, iBand);
    iRowEnd   = get_row_end_for_band(infoPtr, iBand);
    extraOrig = extra = cxDesired - lpBand->cxEffective;
    if (extra > 0)
        extra = REBAR_ShrinkBandsRTL(infoPtr, iRowBegin, iBand, extra, TRUE);
    if (extra > 0)
        extra = REBAR_ShrinkBandsLTR(infoPtr, next_visible(infoPtr, iBand), iRowEnd, extra, TRUE);
    lpBand->cxEffective += extraOrig - extra;
    lpBand->cx = lpBand->cxEffective;
    TRACE("(%d, %Id): Wanted size %d, obtained %d (shrink %d, %d)\n", iBand, lParam, cxDesired, lpBand->cx, extraOrig, extra);
    REBAR_SetRowRectsX(infoPtr, iRowBegin, iRowEnd);

    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, iRowBegin, iRowEnd);
    else
        REBAR_CalcHorzBand(infoPtr, iRowBegin, iRowEnd);
    REBAR_MoveChildWindows(infoPtr, iRowBegin, iRowEnd);
    return TRUE;

}


static LRESULT
REBAR_MinimizeBand (const REBAR_INFO *infoPtr, INT iBand)
{
    REBAR_BAND *lpBand;
    int iPrev, iRowBegin, iRowEnd;

    /* A "minimize" band is equivalent to "dragging" the gripper
     * of than band to the right till the band is only the size
     * of the cxHeader.
     */

    /* Validate */
    if (infoPtr->uNumBands == 0 || iBand < 0 || iBand >= infoPtr->uNumBands) {
	/* error !!! */
	ERR("Illegal MinimizeBand, requested=%d, current band count=%d\n",
	      iBand, infoPtr->uNumBands);
      	return FALSE;
    }

    /* compute amount of movement and validate */
    lpBand = REBAR_GetBand(infoPtr, iBand);

    if (lpBand->fStyle & RBBS_HIDDEN)
    {
        /* Windows is buggy and creates a hole/overlap */
        WARN("Ignoring minimize request on a hidden band (%d)\n", iBand);
        return FALSE;
    }

    iPrev = prev_visible(infoPtr, iBand);
    /* if first band in row */
    if (iPrev < 0 || REBAR_GetBand(infoPtr, iPrev)->iRow != lpBand->iRow) {
        int iNext = next_visible(infoPtr, iBand);
        if (iNext < infoPtr->uNumBands && REBAR_GetBand(infoPtr, iNext)->iRow == lpBand->iRow) {
            TRACE("(%d): Minimizing the first band in row is by maximizing the second\n", iBand);
            REBAR_MaximizeBand(infoPtr, iNext, FALSE);
        }
        else
            TRACE("(%d): Only one band in row - nothing to do\n", iBand);
        return TRUE;
    }

    REBAR_GetBand(infoPtr, iPrev)->cxEffective += lpBand->cxEffective - lpBand->cxMinBand;
    REBAR_GetBand(infoPtr, iPrev)->cx = REBAR_GetBand(infoPtr, iPrev)->cxEffective;
    lpBand->cx = lpBand->cxEffective = lpBand->cxMinBand;

    iRowBegin = get_row_begin_for_band(infoPtr, iBand);
    iRowEnd = get_row_end_for_band(infoPtr, iBand);
    REBAR_SetRowRectsX(infoPtr, iRowBegin, iRowEnd);

    if (infoPtr->dwStyle & CCS_VERT)
        REBAR_CalcVertBand(infoPtr, iRowBegin, iRowEnd);
    else
        REBAR_CalcHorzBand(infoPtr, iRowBegin, iRowEnd);
    REBAR_MoveChildWindows(infoPtr, iRowBegin, iRowEnd);
    return FALSE;
}


static LRESULT
REBAR_MoveBand (REBAR_INFO *infoPtr, INT iFrom, INT iTo)
{
    REBAR_BAND *lpBand;

    /* Validate */
    if ((infoPtr->uNumBands == 0) ||
	(iFrom < 0) || iFrom >= infoPtr->uNumBands ||
	(iTo < 0)   || iTo >= infoPtr->uNumBands) {
	/* error !!! */
	ERR("Illegal MoveBand, from=%d, to=%d, current band count=%d\n",
	      iFrom, iTo, infoPtr->uNumBands);
      	return FALSE;
    }

    lpBand = REBAR_GetBand(infoPtr, iFrom);
    DPA_DeletePtr(infoPtr->bands, iFrom);
    DPA_InsertPtr(infoPtr->bands, iTo, lpBand);

    TRACE("moved band %d to index %d\n", iFrom, iTo);
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
REBAR_SetBandInfoT(REBAR_INFO *infoPtr, INT iBand, const REBARBANDINFOW *lprbbi, BOOL bUnicode)
{
    REBAR_BAND *lpBand;
    UINT uChanged;

    if (!lprbbi || lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;

    if (iBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %d\n", iBand);
    REBAR_DumpBandInfo (lprbbi);

    /* set band information */
    lpBand = REBAR_GetBand(infoPtr, iBand);

    uChanged = REBAR_CommonSetupBand (infoPtr->hwndSelf, lprbbi, lpBand);
    if (lprbbi->fMask & RBBIM_TEXT) {
        LPWSTR wstr = NULL;
        if (bUnicode)
            Str_SetPtrW(&wstr, lprbbi->lpText);
        else
            Str_SetPtrAtoW(&wstr, (LPSTR)lprbbi->lpText);

        if (REBAR_strdifW(wstr, lpBand->lpText)) {
            Free(lpBand->lpText);
            lpBand->lpText = wstr;
            uChanged |= RBBIM_TEXT;
        }
        else
            Free(wstr);
    }

    REBAR_ValidateBand (infoPtr, lpBand);

    REBAR_DumpBand (infoPtr);

    if (uChanged & (RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE | RBBIM_IMAGE)) {
	  REBAR_Layout(infoPtr);
	  InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBarInfo (REBAR_INFO *infoPtr, const REBARINFO *lpInfo)
{
    REBAR_BAND *lpBand;
    UINT i;

    if (!lpInfo || lpInfo->cbSize < sizeof (REBARINFO))
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
	TRACE("new image cx=%ld, cy=%ld\n", infoPtr->imageSize.cx, infoPtr->imageSize.cy);
    }

    /* revalidate all bands to reset flags for images in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = REBAR_GetBand(infoPtr, i);
	REBAR_ValidateBand (infoPtr, lpBand);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBkColor (REBAR_INFO *infoPtr, COLORREF clr)
{
    COLORREF clrTemp;

    clrTemp = infoPtr->clrBk;
    infoPtr->clrBk = clr;

    TRACE("background color %#lx\n", infoPtr->clrBk);

    return clrTemp;
}


/* << REBAR_SetColorScheme >> */
/* << REBAR_SetPalette >> */


static LRESULT
REBAR_SetParent (REBAR_INFO *infoPtr, HWND parent)
{
    HWND hwndTemp = infoPtr->hwndNotify;

    infoPtr->hwndNotify = parent;

    return (LRESULT)hwndTemp;
}


static LRESULT
REBAR_SetTextColor (REBAR_INFO *infoPtr, COLORREF clr)
{
    COLORREF clrTemp;

    clrTemp = infoPtr->clrText;
    infoPtr->clrText = clr;

    TRACE("text color %#lx\n", infoPtr->clrText);

    return clrTemp;
}


/* << REBAR_SetTooltips >> */


static inline LRESULT
REBAR_SetUnicodeFormat (REBAR_INFO *infoPtr, BOOL unicode)
{
    BOOL bTemp = infoPtr->bUnicode;

    TRACE("to %s hwnd=%p, was %s\n",
	   unicode ? "TRUE" : "FALSE", infoPtr->hwndSelf,
	  (bTemp) ? "TRUE" : "FALSE");

    infoPtr->bUnicode = unicode;

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
REBAR_ShowBand (REBAR_INFO *infoPtr, INT iBand, BOOL show)
{
    REBAR_BAND *lpBand;

    if (iBand < 0 || iBand >= infoPtr->uNumBands)
	return FALSE;

    lpBand = REBAR_GetBand(infoPtr, iBand);

    if (show) {
	TRACE("show band %d\n", iBand);
	lpBand->fStyle = lpBand->fStyle & ~RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_SHOW);
    }
    else {
	TRACE("hide band %d\n", iBand);
	lpBand->fStyle = lpBand->fStyle | RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_HIDE);
    }

    REBAR_Layout(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return TRUE;
}


static LRESULT
REBAR_SizeToRect (REBAR_INFO *infoPtr, const RECT *lpRect)
{
    if (!lpRect) return FALSE;

    TRACE("[%s]\n", wine_dbgstr_rect(lpRect));
    REBAR_SizeToHeight(infoPtr, get_rect_cy(infoPtr, lpRect));
    return TRUE;
}



static LRESULT
REBAR_Create (REBAR_INFO *infoPtr, LPCREATESTRUCTW cs)
{
    RECT wnrc1, clrc1;

    if (TRACE_ON(rebar)) {
	GetWindowRect(infoPtr->hwndSelf, &wnrc1);
	GetClientRect(infoPtr->hwndSelf, &clrc1);
        TRACE("window=(%s) client=(%s) cs=(%d,%d %dx%d)\n",
              wine_dbgstr_rect(&wnrc1), wine_dbgstr_rect(&clrc1),
	      cs->x, cs->y, cs->cx, cs->cy);
    }

    OpenThemeData(infoPtr->hwndSelf, themeClass);

    TRACE("created!\n");
    return 0;
}


static LRESULT
REBAR_Destroy (REBAR_INFO *infoPtr)
{
    REBAR_BAND *lpBand;
    UINT i;

    /* clean up each band */
    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = REBAR_GetBand(infoPtr, i);

	/* delete text strings */
        Free (lpBand->lpText);
	lpBand->lpText = NULL;
	/* destroy child window */
	DestroyWindow (lpBand->hwndChild);
	Free (lpBand);
    }

    /* free band array */
    DPA_Destroy (infoPtr->bands);
    infoPtr->bands = NULL;

    DestroyCursor (infoPtr->hcurArrow);
    DestroyCursor (infoPtr->hcurHorz);
    DestroyCursor (infoPtr->hcurVert);
    DestroyCursor (infoPtr->hcurDrag);
    if (infoPtr->hDefaultFont) DeleteObject (infoPtr->hDefaultFont);
    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);
    
    CloseThemeData (GetWindowTheme (infoPtr->hwndSelf));

    /* free rebar info data */
    Free (infoPtr);
    TRACE("destroyed!\n");
    return 0;
}

static LRESULT
REBAR_GetFont (const REBAR_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hFont;
}

static LRESULT
REBAR_PushChevron(const REBAR_INFO *infoPtr, UINT uBand, LPARAM lParam)
{
    if (uBand < infoPtr->uNumBands)
    {
        NMREBARCHEVRON nmrbc;
        REBAR_BAND *lpBand = REBAR_GetBand(infoPtr, uBand);

        TRACE("Pressed chevron on band %u\n", uBand);

        /* redraw chevron in pushed state */
        lpBand->fDraw |= DRAW_CHEVRONPUSHED;
        RedrawWindow(infoPtr->hwndSelf, &lpBand->rcChevron,0,
          RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);

        /* notify app so it can display a popup menu or whatever */
        nmrbc.uBand = uBand;
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
REBAR_LButtonDown (REBAR_INFO *infoPtr, LPARAM lParam)
{
    UINT htFlags;
    INT iHitBand;
    POINT ptMouseDown;
    ptMouseDown.x = (short)LOWORD(lParam);
    ptMouseDown.y = (short)HIWORD(lParam);

    REBAR_InternalHitTest(infoPtr, &ptMouseDown, &htFlags, &iHitBand);

    if (htFlags == RBHT_CHEVRON)
    {
        REBAR_PushChevron(infoPtr, iHitBand, 0);
    }
    else if (htFlags == RBHT_GRABBER || htFlags == RBHT_CAPTION)
    {
        REBAR_BAND *lpBand;

        TRACE("Starting drag\n");

        lpBand = REBAR_GetBand(infoPtr, iHitBand);

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
REBAR_LButtonUp (REBAR_INFO *infoPtr)
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
REBAR_MouseLeave (REBAR_INFO *infoPtr)
{
    if (infoPtr->ichevronhotBand >= 0)
    {
        REBAR_BAND *lpChevronBand = REBAR_GetBand(infoPtr, infoPtr->ichevronhotBand);
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
REBAR_MouseMove (REBAR_INFO *infoPtr, LPARAM lParam)
{
    REBAR_BAND *lpChevronBand;
    POINT ptMove;

    ptMove.x = (short)LOWORD(lParam);
    ptMove.y = (short)HIWORD(lParam);

    /* if we are currently dragging a band */
    if (infoPtr->iGrabbedBand >= 0)
    {
        REBAR_BAND *band;
        int yPtMove = (infoPtr->dwStyle & CCS_VERT ? ptMove.x : ptMove.y);

        if (GetCapture() != infoPtr->hwndSelf)
            ERR("We are dragging but haven't got capture?!?\n");

        band = REBAR_GetBand(infoPtr, infoPtr->iGrabbedBand);

        /* if mouse did not move much, exit */
        if ((abs(ptMove.x - infoPtr->dragNow.x) <= mindragx) &&
            (abs(ptMove.y - infoPtr->dragNow.y) <= mindragy)) return 0;

        /* on first significant mouse movement, issue notify */
        if (!(infoPtr->fStatus & BEGIN_DRAG_ISSUED)) {
            if (REBAR_Notify_NMREBAR (infoPtr, -1, RBN_BEGINDRAG)) {
                /* Notify returned TRUE - abort drag */
                infoPtr->dragStart.x = 0;
                infoPtr->dragStart.y = 0;
                infoPtr->dragNow = infoPtr->dragStart;
                infoPtr->iGrabbedBand = -1;
                ReleaseCapture ();
                return 0;
            }
            infoPtr->fStatus |= BEGIN_DRAG_ISSUED;
        }

        /* Test for valid drag case - must not be first band in row */
        if ((yPtMove < band->rcBand.top) ||
              (yPtMove > band->rcBand.bottom)) {
            REBAR_HandleUDDrag (infoPtr, &ptMove);
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
            lpChevronBand = REBAR_GetBand(infoPtr, infoPtr->ichevronhotBand);
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

            lpChevronBand = REBAR_GetBand(infoPtr, iHitBand);
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
REBAR_NCCalcSize (const REBAR_INFO *infoPtr, RECT *rect)
{
    if (infoPtr->dwStyle & WS_BORDER) {
        rect->left   = min(rect->left + GetSystemMetrics(SM_CXEDGE), rect->right);
        rect->right  = max(rect->right - GetSystemMetrics(SM_CXEDGE), rect->left);
        rect->top    = min(rect->top + GetSystemMetrics(SM_CYEDGE), rect->bottom);
        rect->bottom = max(rect->bottom - GetSystemMetrics(SM_CYEDGE), rect->top);
    }

    TRACE("new client=(%s)\n", wine_dbgstr_rect(rect));
    return 0;
}


static LRESULT
REBAR_NCCreate (HWND hwnd, const CREATESTRUCTW *cs)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT wnrc1, clrc1;
    NONCLIENTMETRICSW ncm;
    HFONT tfont;

    if (infoPtr) {
	ERR("Strange info structure pointer *not* NULL\n");
	return FALSE;
    }

    if (TRACE_ON(rebar)) {
	GetWindowRect(hwnd, &wnrc1);
	GetClientRect(hwnd, &clrc1);
        TRACE("window=(%s) client=(%s) cs=(%d,%d %dx%d)\n",
              wine_dbgstr_rect(&wnrc1), wine_dbgstr_rect(&clrc1),
	      cs->x, cs->y, cs->cx, cs->cy);
    }

    /* allocate memory for info structure */
    infoPtr = Alloc (sizeof(REBAR_INFO));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    /* initialize info structure - initial values are 0 */
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = CLR_NONE;
    infoPtr->clrBtnText = comctl32_color.clrBtnText;
    infoPtr->clrBtnFace = comctl32_color.clrBtnFace;
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
    infoPtr->bands = DPA_Create(8);

    /* issue WM_NOTIFYFORMAT to get unicode status of parent */
    REBAR_NotifyFormat(infoPtr, NF_REQUERY);

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

    return TRUE;
}


static LRESULT
REBAR_NCHitTest (const REBAR_INFO *infoPtr, LPARAM lParam)
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
    if ((i = REBAR_Notify((NMHDR *) &nmmouse, infoPtr, NM_NCHITTEST)))
    {
        TRACE("notify changed return value from %Id to %d\n", ret, i);
        ret = (LRESULT) i;
    }
    TRACE("returning %Id, client point %s\n", ret, wine_dbgstr_point(&clpt));
    return ret;
}


static LRESULT
REBAR_NCPaint (const REBAR_INFO *infoPtr)
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
        TRACE("rect (%s)\n", wine_dbgstr_rect(&rcWindow));
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
        TRACE("rect (%s)\n", wine_dbgstr_rect(&rcWindow));
        DrawThemeEdge (theme, hdc, 0, 0, &rcWindow, BDR_RAISEDINNER, BF_TOP, NULL);
        ReleaseDC( infoPtr->hwndSelf, hdc );
    }

    return 0;
}


static LRESULT
REBAR_NotifyFormat (REBAR_INFO *infoPtr, LPARAM cmd)
{
    INT i;

    if (cmd == NF_REQUERY) {
	i = SendMessageW(REBAR_GetNotifyParent (infoPtr),
			 WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);
        if ((i != NFR_ANSI) && (i != NFR_UNICODE)) {
	    ERR("wrong response to WM_NOTIFYFORMAT (%d), assuming ANSI\n", i);
	    i = NFR_ANSI;
	}
        infoPtr->bUnicode = (i == NFR_UNICODE);
	return (LRESULT)i;
    }
    return (LRESULT)((infoPtr->bUnicode) ? NFR_UNICODE : NFR_ANSI);
}


static LRESULT
REBAR_Paint (const REBAR_INFO *infoPtr, HDC hdc)
{
    if (hdc) {
        TRACE("painting\n");
        REBAR_Refresh (infoPtr, hdc);
    } else {
        PAINTSTRUCT ps;
        hdc = BeginPaint (infoPtr->hwndSelf, &ps);
        TRACE("painting (%s)\n", wine_dbgstr_rect(&ps.rcPaint));
        if (ps.fErase) {
            /* Erase area of paint if requested */
            REBAR_EraseBkGnd (infoPtr, hdc);
        }
        REBAR_Refresh (infoPtr, hdc);
	EndPaint (infoPtr->hwndSelf, &ps);
    }

    return 0;
}


static LRESULT
REBAR_SetCursor (const REBAR_INFO *infoPtr, LPARAM lParam)
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
REBAR_SetFont (REBAR_INFO *infoPtr, HFONT font)
{
    REBAR_BAND *lpBand;
    UINT i;

    infoPtr->hFont = font;

    /* revalidate all bands to change sizes of text in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = REBAR_GetBand(infoPtr, i);
	REBAR_ValidateBand (infoPtr, lpBand);
    }

    REBAR_Layout(infoPtr);
    return 0;
}


/*****************************************************
 *
 *  Handles the WM_SETREDRAW message.
 *
 * Documentation:
 *  According to testing V4.71 of COMCTL32 returns the
 *  *previous* status of the redraw flag (either 0 or -1)
 *  instead of the MSDN documented value of 0 if handled
 *
 *****************************************************/
static inline LRESULT
REBAR_SetRedraw (REBAR_INFO *infoPtr, BOOL redraw)
{
    BOOL oldredraw = infoPtr->DoRedraw;

    TRACE("set to %s, fStatus=%08x\n",
	  (redraw) ? "TRUE" : "FALSE", infoPtr->fStatus);
    infoPtr->DoRedraw = redraw;
    if (redraw) {
	if (infoPtr->fStatus & BAND_NEEDS_REDRAW) {
	    REBAR_MoveChildWindows (infoPtr, 0, infoPtr->uNumBands);
	    REBAR_ForceResize (infoPtr);
	    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);
	}
	infoPtr->fStatus &= ~BAND_NEEDS_REDRAW;
    }
    return (oldredraw) ? -1 : 0;
}


static LRESULT
REBAR_Size (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("wParam %Ix, lParam %Ix\n", wParam, lParam);

    /* avoid _Layout resize recursion (but it shouldn't be infinite and it seems Windows does recurse) */
    if (infoPtr->fStatus & SELF_RESIZE) {
	infoPtr->fStatus &= ~SELF_RESIZE;
	TRACE("SELF_RESIZE was set, reset, fStatus=%08x lparam %Ix\n", infoPtr->fStatus, lParam);
	return 0;
    }
    
    if (infoPtr->dwStyle & RBS_AUTOSIZE)
        REBAR_AutoSize(infoPtr, TRUE);
    else
        REBAR_Layout(infoPtr);

    return 0;
}


static LRESULT
REBAR_StyleChanged (REBAR_INFO *infoPtr, INT nType, const STYLESTRUCT *lpStyle)
{
    TRACE("current style %#lx, styleOld %#lx, style being set to %#lx\n",
	  infoPtr->dwStyle, lpStyle->styleOld, lpStyle->styleNew);
    if (nType == GWL_STYLE)
    {
        infoPtr->orgStyle = infoPtr->dwStyle = lpStyle->styleNew;
        /* maybe it should be COMMON_STYLES like in toolbar */
        if ((lpStyle->styleNew ^ lpStyle->styleOld) & CCS_VERT)
            REBAR_Layout(infoPtr);
    }
    return FALSE;
}

/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (REBAR_INFO* infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    CloseThemeData (theme);
    OpenThemeData(infoPtr->hwndSelf, themeClass);
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
    TRACE("hwnd %p new pos (%s)\n", infoPtr->hwndSelf, wine_dbgstr_rect(&rc));
    return ret;
}


static LRESULT WINAPI
REBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("hwnd %p, msg %x, wparam %Ix, lparam %Ix\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_NCCREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
/*	case RB_BEGINDRAG: */

	case RB_DELETEBAND:
	    return REBAR_DeleteBand (infoPtr, wParam);

/*	case RB_DRAGMOVE: */
/*	case RB_ENDDRAG: */

	case RB_GETBANDBORDERS:
	    return REBAR_GetBandBorders (infoPtr, wParam, (LPRECT)lParam);

	case RB_GETBANDCOUNT:
	    return REBAR_GetBandCount (infoPtr);

	case RB_GETBANDINFO_OLD:
	case RB_GETBANDINFOA:
	case RB_GETBANDINFOW:
	    return REBAR_GetBandInfoT(infoPtr, wParam, (LPREBARBANDINFOW)lParam,
	                                                uMsg == RB_GETBANDINFOW);
	case RB_GETBARHEIGHT:
	    return REBAR_GetBarHeight (infoPtr);

	case RB_GETBARINFO:
	    return REBAR_GetBarInfo (infoPtr, (LPREBARINFO)lParam);

	case RB_GETBKCOLOR:
	    return REBAR_GetBkColor (infoPtr);

/*	case RB_GETCOLORSCHEME: */
/*	case RB_GETDROPTARGET: */

	case RB_GETPALETTE:
	    return REBAR_GetPalette (infoPtr);

	case RB_GETRECT:
	    return REBAR_GetRect (infoPtr, wParam, (LPRECT)lParam);

	case RB_GETROWCOUNT:
	    return REBAR_GetRowCount (infoPtr);

	case RB_GETROWHEIGHT:
	    return REBAR_GetRowHeight (infoPtr, wParam);

	case RB_GETTEXTCOLOR:
	    return REBAR_GetTextColor (infoPtr);

	case RB_GETTOOLTIPS:
	    return REBAR_GetToolTips (infoPtr);

	case RB_GETUNICODEFORMAT:
	    return REBAR_GetUnicodeFormat (infoPtr);

	case CCM_GETVERSION:
	    return REBAR_GetVersion (infoPtr);

	case RB_HITTEST:
	    return REBAR_HitTest (infoPtr, (LPRBHITTESTINFO)lParam);

	case RB_IDTOINDEX:
	    return REBAR_IdToIndex (infoPtr, wParam);

	case RB_INSERTBANDA:
	case RB_INSERTBANDW:
	    return REBAR_InsertBandT(infoPtr, wParam, (LPREBARBANDINFOW)lParam,
	                                               uMsg == RB_INSERTBANDW);
	case RB_MAXIMIZEBAND:
	    return REBAR_MaximizeBand (infoPtr, wParam, lParam);

	case RB_MINIMIZEBAND:
	    return REBAR_MinimizeBand (infoPtr, wParam);

	case RB_MOVEBAND:
	    return REBAR_MoveBand (infoPtr, wParam, lParam);

	case RB_PUSHCHEVRON:
	    return REBAR_PushChevron (infoPtr, wParam, lParam);

	case RB_SETBANDINFOA:
	case RB_SETBANDINFOW:
	    return REBAR_SetBandInfoT(infoPtr, wParam, (LPREBARBANDINFOW)lParam,
	                                                uMsg == RB_SETBANDINFOW);
	case RB_SETBARINFO:
	    return REBAR_SetBarInfo (infoPtr, (LPREBARINFO)lParam);

	case RB_SETBKCOLOR:
	    return REBAR_SetBkColor (infoPtr, lParam);

/*	case RB_SETCOLORSCHEME: */
/*	case RB_SETPALETTE: */

	case RB_SETPARENT:
	    return REBAR_SetParent (infoPtr, (HWND)wParam);

	case RB_SETTEXTCOLOR:
	    return REBAR_SetTextColor (infoPtr, lParam);

/*	case RB_SETTOOLTIPS: */

	case RB_SETUNICODEFORMAT:
	    return REBAR_SetUnicodeFormat (infoPtr, wParam);

	case CCM_SETVERSION:
	    return REBAR_SetVersion (infoPtr, (INT)wParam);

	case RB_SHOWBAND:
	    return REBAR_ShowBand (infoPtr, wParam, lParam);

	case RB_SIZETORECT:
	    return REBAR_SizeToRect (infoPtr, (LPCRECT)lParam);


/*    Messages passed to parent */
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_NOTIFY:
        case WM_MEASUREITEM:
            return SendMessageW(REBAR_GetNotifyParent (infoPtr), uMsg, wParam, lParam);


/*      case WM_CHARTOITEM:     supported according to ControlSpy */

	case WM_CREATE:
	    return REBAR_Create (infoPtr, (LPCREATESTRUCTW)lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (infoPtr);

        case WM_ERASEBKGND:
	    return REBAR_EraseBkGnd (infoPtr, (HDC)wParam);

	case WM_GETFONT:
	    return REBAR_GetFont (infoPtr);

/*      case WM_LBUTTONDBLCLK:  supported according to ControlSpy */

	case WM_LBUTTONDOWN:
	    return REBAR_LButtonDown (infoPtr, lParam);

	case WM_LBUTTONUP:
	    return REBAR_LButtonUp (infoPtr);

	case WM_MOUSEMOVE:
	    return REBAR_MouseMove (infoPtr, lParam);

	case WM_MOUSELEAVE:
	    return REBAR_MouseLeave (infoPtr);

	case WM_NCCALCSIZE:
	    return REBAR_NCCalcSize (infoPtr, (RECT*)lParam);

        case WM_NCCREATE:
	    return REBAR_NCCreate (hwnd, (LPCREATESTRUCTW)lParam);

        case WM_NCHITTEST:
	    return REBAR_NCHitTest (infoPtr, lParam);

	case WM_NCPAINT:
	    return REBAR_NCPaint (infoPtr);

        case WM_NOTIFYFORMAT:
	    return REBAR_NotifyFormat (infoPtr, lParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return REBAR_Paint (infoPtr, (HDC)wParam);

/*      case WM_PALETTECHANGED: supported according to ControlSpy */
/*      case WM_QUERYNEWPALETTE:supported according to ControlSpy */
/*      case WM_RBUTTONDOWN:    supported according to ControlSpy */
/*      case WM_RBUTTONUP:      supported according to ControlSpy */

	case WM_SETCURSOR:
	    return REBAR_SetCursor (infoPtr, lParam);

	case WM_SETFONT:
	    return REBAR_SetFont (infoPtr, (HFONT)wParam);

        case WM_SETREDRAW:
	    return REBAR_SetRedraw (infoPtr, wParam);

	case WM_SIZE:
	    return REBAR_Size (infoPtr, wParam, lParam);

        case WM_STYLECHANGED:
	    return REBAR_StyleChanged (infoPtr, wParam, (LPSTYLESTRUCT)lParam);

        case WM_THEMECHANGED:
            return theme_changed (infoPtr);

        case WM_SYSCOLORCHANGE:
            COMCTL32_RefreshSysColors();
            return 0;

/*      case WM_VKEYTOITEM:     supported according to ControlSpy */
/*	case WM_WININICHANGE: */

        case WM_WINDOWPOSCHANGED:
	    return REBAR_WindowPosChanged (infoPtr, wParam, lParam);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
		ERR("unknown msg %04x, wp %Ix, lp %Ix\n", uMsg, wParam, lParam);
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
