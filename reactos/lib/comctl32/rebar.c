/*
 * Testing: set to 1 to make background brush *always* green
 */
#define GLATESTING 0

/*
 *
 * 2.  At "FIXME:  problem # 2" WinRAR:
 *   if "#if 1" then last band draws in separate row
 *   if "#if 0" then last band draws in previous row *** just like native ***
 *
 */
#define PROBLEM2 0

/*
 * 3. REBAR_MoveChildWindows should have a loop because more than
 *    one pass is made (together with the RBN_CHILDSIZEs) is made on
 *    at least RB_INSERTBAND
 */


/*
 * Rebar control    rev 8e
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 *   An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - vertical placement
 *   - ComboBox and ComboBoxEx placement
 *   - center image
 *   - Layout code.
 *   - Display code.
 *   - Some messages.
 *   - All notifications.

 * Changes Guy Albertelli <galberte@neo.lrun.com>
 *  rev 2,3,4
 *   - Implement initial version of row grouping, row separators,
 *     text and background colors. Support additional messages.
 *     Support RBBS_BREAK. Implement ERASEBKGND and improve painting.
 *  rev 5
 *   - implement support for dragging Gripper left or right in a row. Supports
 *     WM_LBUTTONDOWN, WM_LBUTTONUP, and WM_MOUSEMOVE. Also support
 *     RBS_BANDBORDERS.
 *  rev 6
 *   - Fix or implement notifications for RBN_HEIGHTCHANGE, RBN_CHILDSIZE.
 *   - Correct styles RBBS_NOGRIPPER, RBBS_GRIPPERALWAYS, and RBBS_FIXEDSIZE.
 *   - Fix algorithm for Layout and AdjustBand.
 *
 * rev 7
 *   - Fix algorithm for _Layout and _AdjustBand.
 *   - Fix or implement RBN_ENDDRAG, RB_MOVEBAND, WM_SETREDRAW,
 *     WM_STYLECHANGED, RB_MINIMIZEBAND, RBBS_VARIABLEHEIGHT, RBS_VARHEIGHT,
 *     RBBS_HIDDEN, WM_NOTIFYFORMAT, NM_NCHITTEST, WM_SETREDRAW, RBS_AUTOSIZE,
 *     WM_SETFONT, RBS_BORDERS
 *   - Create structures in WM_NCCREATE
 *   - Additional performance enhancements.
 *
 * rev 8
 *  1. Create array of start and end band indexes by row and use.
 *  2. Fix problem with REBAR_Layout Phase 2b to process only if only
 *     band in row.
 *  3. Set the Caption Font (Regular) as default font for text.
 *  4. Delete font handle on control distruction.
 *  5. Add UpdateWindow call in _MoveChildWindows to match repainting done
 *     by native control
 *  6. Improve some traces.
 *  7. Invalidate window rectangles after SetBandInfo, InsertBand, ShowBand
 *     so that repainting is correct.
 *  8. Implement RB_MAXIMIZEBAND for the "ideal=TRUE" case.
 *  9. Implement item custom draw notifications partially. Only done for
 *     ITEMPREPAINT and ITEMPOSTPAINT. (Used by IE4 for "Favorites" frame
 *     to draw the word "Favorites").
 * rev 8a
 * 10. Handle CCS_NODIVIDER and fix WS_BORDER code.
 * 11. Fix logic error in _AdjustBands where flag was set to valid band
 *     number (0) to indicate *no* band.
 * 12. Fix CCS_VERT errors in _ForceResize, _NCCalcSize, and _NCPaint.
 * 13. Support some special cases of CCS_TOP (and therefore CCS_LEFT),
 *     CCS_BOTTOM (and therefore CCS_RIGHT) and CCS_NOPARENTALIGN. Not
 *     at all sure whether this is all cases.
 * 14. Handle returned value for the RBN_CHILDSIZE notify.
 * 15. Implement RBBS_CHILDEDGE, and set each bands "offChild" at _Layout
 *     time.
 * 16. Fix REBARSPACE. It should depend on CCS_NODIVIDER.
 * rev 8b
 * 17. Fix determination of whether Gripper is needed in _ValidateBand.
 * 18. Fix _AdjustBand processing of RBBS_FIXEDSIZE.
 * rev 8c
 * 19. Fix problem in _Layout when all lengths are 0.
 * 20. If CLR_NONE specified, we will use default BtnFace color when drawing.
 * 21. Fix test in REBAR_Layout.
 * rev 8d
 * 22. Add support for WM_WINDOWPOSCHANGED to save new origin of window.
 * 23. Correct RBN_CHILDSIZE rect value for CCS_VERT rebar.
 * 24. Do UpdateWindow only if doing redraws.
 * rev 8e
 * 25. Adjust setting of offChild.cx based on RBBS_CHILDEDGE.
 *
 *
 *    Still to do:
 *  2. Following still not handled: RBBS_FIXEDBMP,
 *            CCS_NORESIZE,
 *            CCS_NOMOVEX, CCS_NOMOVEY
 *  3. Following are only partially handled:
 *            RBS_AUTOSIZE, RBBS_VARIABLEHEIGHT
 *  5. Native uses (on each draw!!) SM_CYBORDER (or SM_CXBORDER for CCS_VERT)
 *     to set the size of the separator width (the value SEP_WIDTH_SIZE
 *     in here). Should be fixed!!
 *  6. The following messages are not implemented:
 *        RB_BEGINDRAG, RB_DRAGMOVE, RB_ENDDRAG, RB_GETCOLORSCHEME,
 *        RB_GETDROPTARGET, RB_MAXIMIZEBAND,
 *        RB_SETCOLORSCHEME, RB_SETPALETTE, RB_SETTOOLTIPS
 *        WM_CHARTOITEM, WM_LBUTTONDBLCLK, WM_MEASUREITEM,
 *        WM_PALETTECHANGED, WM_PRINTCLIENT, WM_QUERYNEWPALETTE,
 *        WM_RBUTTONDOWN, WM_RBUTTONUP,
 *        WM_SYSCOLORCHANGE, WM_VKEYTOITEM, WM_WININICHANGE
 *  7. The following notifications are not implemented:
 *        NM_CUSTOMDRAW, NM_RELEASEDCAPTURE
 *        RBN_MINMAX
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

    UINT    lcx;            /* minimum cx for band */
    UINT    ccx;            /* current cx for band */
    UINT    hcx;            /* maximum cx for band */
    UINT    lcy;            /* minimum cy for band */
    UINT    ccy;            /* current cy for band */
    UINT    hcy;            /* maximum cy for band */

    SIZE    offChild;       /* x,y offset if child is not FIXEDSIZE */
    UINT    uMinHeight;
    INT     iRow;           /* row this band assigned to */
    UINT    fStatus;        /* status flags, reset only by _Validate */
    UINT    fDraw;          /* drawing flags, reset only by _Layout */
    UINT    uCDret;         /* last return from NM_CUSTOMDRAW */
    RECT    rcoldBand;      /* previous calculated band rectangle */
    RECT    rcBand;         /* calculated band rectangle */
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
#define DRAW_RIGHTSEP   0x00000010
#define DRAW_BOTTOMSEP  0x00000020
#define DRAW_CHEVRONHOT 0x00000040
#define DRAW_CHEVRONPUSHED 0x00000080
#define DRAW_LAST_IN_ROW   0x00000100
#define DRAW_FIRST_IN_ROW  0x00000200
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
    SIZE     calcSize;    /* calculated rebar size */
    SIZE     oldSize;     /* previous calculated rebar size */
    BOOL     bUnicode;    /* TRUE if this window is W type */
    BOOL     NtfUnicode;  /* TRUE if parent wants notify in W format */
    BOOL     DoRedraw;    /* TRUE to acutally draw bands */
    UINT     fStatus;     /* Status flags (see below)  */
    HCURSOR  hcurArrow;   /* handle to the arrow cursor */
    HCURSOR  hcurHorz;    /* handle to the EW cursor */
    HCURSOR  hcurVert;    /* handle to the NS cursor */
    HCURSOR  hcurDrag;    /* handle to the drag cursor */
    INT      iVersion;    /* version number */
    POINTS   dragStart;   /* x,y of button down */
    POINTS   dragNow;     /* x,y of this MouseMove */
    INT      iOldBand;    /* last band that had the mouse cursor over it */
    INT      ihitoffset;  /* offset of hotspot from gripper.left */
    POINT    origin;      /* left/upper corner of client */
    INT      ichevronhotBand; /* last band that had a hot chevron */
    INT      iGrabbedBand;/* band number of band whose gripper was grabbed */

    REBAR_BAND *bands;      /* pointer to the array of rebar bands */
} REBAR_INFO;

/* fStatus flags */
#define BEGIN_DRAG_ISSUED   0x00000001
#define AUTO_RESIZE         0x00000002
#define RESIZE_ANYHOW       0x00000004
#define NTF_HGHTCHG         0x00000008
#define BAND_NEEDS_LAYOUT   0x00000010
#define BAND_NEEDS_REDRAW   0x00000020
#define CREATE_RUNNING      0x00000040

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

/* Height of divider for Rebar if not disabled (CCS_NODIVIDER)     */
/* either top or bottom                                            */
#define REBAR_DIVIDER  2

/* minimium vertical height of a normal bar                        */
/*   or minimum width of a CCS_VERT bar - from experiment on Win2k */
#define REBAR_MINSIZE  23

/* This is the increment that is used over the band height         */
#define REBARSPACE(a)     ((a->fStyle & RBBS_CHILDEDGE) ? 2*REBAR_DIVIDER : 0)

/* ----   End of REBAR layout constants.                      ---- */

#define RB_GETBANDINFO_OLD (WM_USER+5) /* obsoleted after IE3, but we have to support it anyway */

/*  The following 6 defines return the proper rcBand element       */
/*  depending on whether CCS_VERT was set.                         */
#define rcBlt(b) ((infoPtr->dwStyle & CCS_VERT) ? b->rcBand.top : b->rcBand.left)
#define rcBrb(b) ((infoPtr->dwStyle & CCS_VERT) ? b->rcBand.bottom : b->rcBand.right)
#define rcBw(b)  ((infoPtr->dwStyle & CCS_VERT) ? (b->rcBand.bottom - b->rcBand.top) : \
		  (b->rcBand.right - b->rcBand.left))
#define ircBlt(b) ((infoPtr->dwStyle & CCS_VERT) ? b->rcBand.left : b->rcBand.top)
#define ircBrb(b) ((infoPtr->dwStyle & CCS_VERT) ? b->rcBand.right : b->rcBand.bottom)
#define ircBw(b)  ((infoPtr->dwStyle & CCS_VERT) ? (b->rcBand.right - b->rcBand.left) : \
		  (b->rcBand.bottom - b->rcBand.top))

/*  The following define determines if a given band is hidden      */
#define HIDDENBAND(a)  (((a)->fStyle & RBBS_HIDDEN) ||   \
                        ((infoPtr->dwStyle & CCS_VERT) &&         \
                         ((a)->fStyle & RBBS_NOVERT)))

/*  The following defines adjust the right or left end of a rectangle */
#define READJ(b,i) do { if(infoPtr->dwStyle & CCS_VERT) b->rcBand.bottom+=(i); \
                    else b->rcBand.right += (i); } while(0)
#define LEADJ(b,i) do { if(infoPtr->dwStyle & CCS_VERT) b->rcBand.top+=(i); \
                    else b->rcBand.left += (i); } while(0)


#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)GetWindowLongA (hwnd, 0))


/* "constant values" retrieved when DLL was initialized    */
/* FIXME we do this when the classes are registered.       */
static UINT mindragx = 0;
static UINT mindragy = 0;

static const char *band_stylename[] = {
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

static const char *band_maskname[] = {
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
REBAR_DumpBandInfo( LPREBARBANDINFOA pB)
{
    if( !TRACE_ON(rebar) ) return;
    TRACE("band info: ID=%u, size=%u, child=%p, clrF=0x%06lx, clrB=0x%06lx\n",
	  pB->wID, pB->cbSize, pB->hwndChild, pB->clrFore, pB->clrBack);
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
REBAR_DumpBand (REBAR_INFO *iP)
{
    REBAR_BAND *pB;
    UINT i;

    if(! TRACE_ON(rebar) ) return;

    TRACE("hwnd=%p: color=%08lx/%08lx, bands=%u, rows=%u, cSize=%ld,%ld\n",
	  iP->hwndSelf, iP->clrText, iP->clrBk, iP->uNumBands, iP->uNumRows,
	  iP->calcSize.cx, iP->calcSize.cy);
    TRACE("hwnd=%p: flags=%08x, dragStart=%d,%d, dragNow=%d,%d, iGrabbedBand=%d\n",
	  iP->hwndSelf, iP->fStatus, iP->dragStart.x, iP->dragStart.y,
	  iP->dragNow.x, iP->dragNow.y,
	  iP->iGrabbedBand);
    TRACE("hwnd=%p: style=%08lx, I'm Unicode=%s, notify in Unicode=%s, redraw=%s\n",
	  iP->hwndSelf, iP->dwStyle, (iP->bUnicode)?"TRUE":"FALSE",
	  (iP->NtfUnicode)?"TRUE":"FALSE", (iP->DoRedraw)?"TRUE":"FALSE");
    for (i = 0; i < iP->uNumBands; i++) {
	pB = &iP->bands[i];
	TRACE("band # %u: ID=%u, child=%p, row=%u, clrF=0x%06lx, clrB=0x%06lx\n",
	      i, pB->wID, pB->hwndChild, pB->iRow, pB->clrFore, pB->clrBack);
	TRACE("band # %u: mask=0x%08x (%s)\n", i, pB->fMask, REBAR_FmtMask(pB->fMask));
	if (pB->fMask & RBBIM_STYLE)
	    TRACE("band # %u: style=0x%08x (%s)\n",
		  i, pB->fStyle, REBAR_FmtStyle(pB->fStyle));
	TRACE("band # %u: uMinH=%u xHeader=%u",
	      i, pB->uMinHeight, pB->cxHeader);
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
	TRACE("band # %u: lcx=%u, ccx=%u, hcx=%u, lcy=%u, ccy=%u, hcy=%u, offChild=%ld,%ld\n",
	      i, pB->lcx, pB->ccx, pB->hcx, pB->lcy, pB->ccy, pB->hcy, pB->offChild.cx, pB->offChild.cy);
	TRACE("band # %u: fStatus=%08x, fDraw=%08x, Band=(%ld,%ld)-(%ld,%ld), Grip=(%ld,%ld)-(%ld,%ld)\n",
	      i, pB->fStatus, pB->fDraw,
	      pB->rcBand.left, pB->rcBand.top, pB->rcBand.right, pB->rcBand.bottom,
	      pB->rcGripper.left, pB->rcGripper.top, pB->rcGripper.right, pB->rcGripper.bottom);
	TRACE("band # %u: Img=(%ld,%ld)-(%ld,%ld), Txt=(%ld,%ld)-(%ld,%ld), Child=(%ld,%ld)-(%ld,%ld)\n",
	      i,
	      pB->rcCapImage.left, pB->rcCapImage.top, pB->rcCapImage.right, pB->rcCapImage.bottom,
	      pB->rcCapText.left, pB->rcCapText.top, pB->rcCapText.right, pB->rcCapText.bottom,
	      pB->rcChild.left, pB->rcChild.top, pB->rcChild.right, pB->rcChild.bottom);
    }

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
REBAR_GetNotifyParent (REBAR_INFO *infoPtr)
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
REBAR_Notify (NMHDR *nmhdr, REBAR_INFO *infoPtr, UINT code)
{
    HWND parent;

    parent = REBAR_GetNotifyParent (infoPtr);
    nmhdr->idFrom = GetDlgCtrlID (infoPtr->hwndSelf);
    nmhdr->hwndFrom = infoPtr->hwndSelf;
    nmhdr->code = code;

    TRACE("window %p, code=%08x, %s\n", parent, code,
	  (infoPtr->NtfUnicode) ? "via Unicode" : "via ANSI");

    if (infoPtr->NtfUnicode)
	return SendMessageW (parent, WM_NOTIFY, (WPARAM) nmhdr->idFrom,
			     (LPARAM)nmhdr);
    else
	return SendMessageA (parent, WM_NOTIFY, (WPARAM) nmhdr->idFrom,
			     (LPARAM)nmhdr);
}

static INT
REBAR_Notify_NMREBAR (REBAR_INFO *infoPtr, UINT uBand, UINT code)
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
REBAR_DrawBand (HDC hdc, REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{
    HFONT hOldFont = 0;
    INT oldBkMode = 0;
    NMCUSTOMDRAW nmcd;

    if (lpBand->fDraw & DRAW_TEXT) {
	hOldFont = SelectObject (hdc, infoPtr->hFont);
	oldBkMode = SetBkMode (hdc, TRANSPARENT);
    }

    /* should test for CDRF_NOTIFYITEMDRAW here */
    nmcd.dwDrawStage = CDDS_ITEMPREPAINT;
    nmcd.hdc = hdc;
    nmcd.rc = lpBand->rcBand;
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
        DrawEdge (hdc, &lpBand->rcGripper, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);

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

    if (lpBand->uCDret == (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW)) {
	nmcd.dwDrawStage = CDDS_ITEMPOSTPAINT;
	nmcd.hdc = hdc;
	nmcd.rc = lpBand->rcBand;
	nmcd.rc.right = lpBand->rcCapText.right;
	nmcd.rc.bottom = lpBand->rcCapText.bottom;
	nmcd.dwItemSpec = lpBand->wID;
	nmcd.uItemState = 0;
	nmcd.lItemlParam = lpBand->lParam;
	lpBand->uCDret = REBAR_Notify ((NMHDR *)&nmcd, infoPtr, NM_CUSTOMDRAW);
    }
}


static VOID
REBAR_Refresh (REBAR_INFO *infoPtr, HDC hdc)
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
REBAR_FixVert (REBAR_INFO *infoPtr, UINT rowstart, UINT rowend,
		   INT mcy)
     /* Function:                                                    */
     /*   Cycle through bands in row and fix height of each band.    */
     /*   Also determine whether each band has changed.              */
     /* On entry:                                                    */
     /*   all bands at desired size.                                 */
     /*   start and end bands are *not* hidden                       */
{
    REBAR_BAND *lpBand;
    INT i;

    for (i = (INT)rowstart; i<=(INT)rowend; i++) {
        lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;

	/* adjust height of bands in row to "mcy" value */
	if (infoPtr->dwStyle & CCS_VERT) {
	    if (lpBand->rcBand.right != lpBand->rcBand.left + mcy)
	        lpBand->rcBand.right = lpBand->rcBand.left + mcy;
	}
	else {
	    if (lpBand->rcBand.bottom != lpBand->rcBand.top + mcy)
	        lpBand->rcBand.bottom = lpBand->rcBand.top + mcy;

	}

	/* mark whether we need to invalidate this band and trace */
	if ((lpBand->rcoldBand.left !=lpBand->rcBand.left) ||
	    (lpBand->rcoldBand.top !=lpBand->rcBand.top) ||
	    (lpBand->rcoldBand.right !=lpBand->rcBand.right) ||
	    (lpBand->rcoldBand.bottom !=lpBand->rcBand.bottom)) {
	    lpBand->fDraw |= NTF_INVALIDATE;
	    TRACE("band %d row=%d: changed to (%ld,%ld)-(%ld,%ld) from (%ld,%ld)-(%ld,%ld)\n",
		  i, lpBand->iRow,
		  lpBand->rcBand.left, lpBand->rcBand.top,
		  lpBand->rcBand.right, lpBand->rcBand.bottom,
		  lpBand->rcoldBand.left, lpBand->rcoldBand.top,
		  lpBand->rcoldBand.right, lpBand->rcoldBand.bottom);
	}
	else
	    TRACE("band %d row=%d: unchanged (%ld,%ld)-(%ld,%ld)\n",
		  i, lpBand->iRow,
		  lpBand->rcBand.left, lpBand->rcBand.top,
		  lpBand->rcBand.right, lpBand->rcBand.bottom);
    }
}


static void
REBAR_AdjustBands (REBAR_INFO *infoPtr, UINT rowstart, UINT rowend,
		   INT maxx, INT mcy)
     /* Function: This routine distributes the extra space in a row. */
     /*  See algorithm below.                                        */
     /* On entry:                                                    */
     /*   all bands @ ->cxHeader size                                */
     /*   start and end bands are *not* hidden                       */
{
    REBAR_BAND *lpBand;
    UINT x, xsep, extra, curwidth, fudge;
    INT i, last_adjusted;

    TRACE("start=%u, end=%u, max x=%d, max y=%d\n",
	  rowstart, rowend, maxx, mcy);

    /* *******************  Phase 1  ************************ */
    /* Alg:                                                   */
    /*  For each visible band with valid child                */
    /*      a. inflate band till either all extra space used  */
    /*         or band's ->ccx reached.                       */
    /*  If any band modified, add any space left to last band */
    /*  adjusted.                                             */
    /*                                                        */
    /* ****************************************************** */
    lpBand = &infoPtr->bands[rowend];
    extra = maxx - rcBrb(lpBand);
    x = 0;
    last_adjusted = -1;
    for (i=(INT)rowstart; i<=(INT)rowend; i++) {
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	xsep = (x == 0) ? 0 : SEP_WIDTH;
	curwidth = rcBw(lpBand);

	/* set new left/top point */
	if (infoPtr->dwStyle & CCS_VERT)
	    lpBand->rcBand.top = x + xsep;
	else
	    lpBand->rcBand.left = x + xsep;

	/* compute new width */
	if ((lpBand->hwndChild && extra) && !(lpBand->fStyle & RBBS_FIXEDSIZE)) {
	    /* set to the "current" band size less the header */
	    fudge = lpBand->ccx;
	    last_adjusted = i;
	    if ((lpBand->fMask & RBBIM_SIZE) && (lpBand->cx > 0) &&
		(fudge > curwidth)) {
		TRACE("adjusting band %d by %d, fudge=%d, curwidth=%d, extra=%d\n",
		      i, fudge-curwidth, fudge, curwidth, extra);
		if ((fudge - curwidth) > extra)
		    fudge = curwidth + extra;
		extra -= (fudge - curwidth);
		curwidth = fudge;
	    }
	    else {
		TRACE("adjusting band %d by %d, fudge=%d, curwidth=%d\n",
		      i, extra, fudge, curwidth);
		curwidth += extra;
		extra = 0;
	    }
	}

	/* set new right/bottom point */
	if (infoPtr->dwStyle & CCS_VERT)
	    lpBand->rcBand.bottom = lpBand->rcBand.top + curwidth;
	else
	    lpBand->rcBand.right = lpBand->rcBand.left + curwidth;
	TRACE("Phase 1 band %d, (%ld,%ld)-(%ld,%ld), orig x=%d, xsep=%d\n",
	      i, lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom, x, xsep);
	x = rcBrb(lpBand);
    }
    if ((x >= maxx) || (last_adjusted != -1)) {
	if (x > maxx) {
	    ERR("Phase 1 failed, x=%d, maxx=%d, start=%u, end=%u\n",
		x, maxx,  rowstart, rowend);
	}
	/* done, so spread extra space */
	if (x < maxx) {
	    fudge = maxx - x;
	    TRACE("Need to spread %d on last adjusted band %d\n",
		fudge, last_adjusted);
	    for (i=(INT)last_adjusted; i<=(INT)rowend; i++) {
		lpBand = &infoPtr->bands[i];
		if (HIDDENBAND(lpBand)) continue;

		/* set right/bottom point */
		if (i != last_adjusted) {
		    if (infoPtr->dwStyle & CCS_VERT)
			lpBand->rcBand.top += fudge;
		    else
			lpBand->rcBand.left += fudge;
		}

		/* set left/bottom point */
		if (infoPtr->dwStyle & CCS_VERT)
		    lpBand->rcBand.bottom += fudge;
		else
		    lpBand->rcBand.right += fudge;
	    }
	}
	TRACE("Phase 1 succeeded, used x=%d\n", x);
	REBAR_FixVert (infoPtr, rowstart, rowend, mcy);
 	return;
    }

    /* *******************  Phase 2  ************************ */
    /* Alg:                                                   */
    /*  Find first visible band, put all                      */
    /*    extra space there.                                  */
    /*                                                        */
    /* ****************************************************** */

    x = 0;
    for (i=(INT)rowstart; i<=(INT)rowend; i++) {
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	xsep = (x == 0) ? 0 : SEP_WIDTH;
	curwidth = rcBw(lpBand);

	/* set new left/top point */
	if (infoPtr->dwStyle & CCS_VERT)
	    lpBand->rcBand.top = x + xsep;
	else
	    lpBand->rcBand.left = x + xsep;

	/* compute new width */
	if (extra) {
	    curwidth += extra;
	    extra = 0;
	}

	/* set new right/bottom point */
	if (infoPtr->dwStyle & CCS_VERT)
	    lpBand->rcBand.bottom = lpBand->rcBand.top + curwidth;
	else
	    lpBand->rcBand.right = lpBand->rcBand.left + curwidth;
	TRACE("Phase 2 band %d, (%ld,%ld)-(%ld,%ld), orig x=%d, xsep=%d\n",
	      i, lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom, x, xsep);
	x = rcBrb(lpBand);
    }
    if (x >= maxx) {
	if (x > maxx) {
	    ERR("Phase 2 failed, x=%d, maxx=%d, start=%u, end=%u\n",
		x, maxx,  rowstart, rowend);
	}
	/* done, so spread extra space */
	TRACE("Phase 2 succeeded, used x=%d\n", x);
	REBAR_FixVert (infoPtr, rowstart, rowend, mcy);
	return;
    }

    /* *******************  Phase 3  ************************ */
    /* at this point everything is back to ->cxHeader values  */
    /* and should not have gotten here.                       */
    /* ****************************************************** */

    lpBand = &infoPtr->bands[rowstart];
    ERR("Serious problem adjusting row %d, start band %d, end band %d\n",
	lpBand->iRow, rowstart, rowend);
    REBAR_DumpBand (infoPtr);
    return;
}


static void
REBAR_CalcHorzBand (REBAR_INFO *infoPtr, UINT rstart, UINT rend, BOOL notify)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Horizontal bars. ***                   */
{
    REBAR_BAND *lpBand;
    UINT i, xoff, yoff;
    HWND parenthwnd;
    RECT oldChild, work;

    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (infoPtr->hwndSelf);

    for(i=rstart; i<rend; i++){
      lpBand = &infoPtr->bands[i];
      if (HIDDENBAND(lpBand)) {
          SetRect (&lpBand->rcChild,
		   lpBand->rcBand.right, lpBand->rcBand.top,
		   lpBand->rcBand.right, lpBand->rcBand.bottom);
	  continue;
      }

      oldChild = lpBand->rcChild;

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
	  /* update band height
	  if (lpBand->uMinHeight < infoPtr->imageSize.cy + 2) {
	      lpBand->uMinHeight = infoPtr->imageSize.cy + 2;
	      lpBand->rcBand.bottom = lpBand->rcBand.top + lpBand->uMinHeight;
	  }  */
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
      if (lpBand->fMask & RBBIM_CHILD) {
	  xoff = lpBand->offChild.cx;
	  yoff = lpBand->offChild.cy;
	  SetRect (&lpBand->rcChild,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.top+yoff,
		   lpBand->rcBand.right-xoff, lpBand->rcBand.bottom-yoff);
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
      if (notify &&
	  ((oldChild.right-oldChild.left != lpBand->rcChild.right-lpBand->rcChild.left) ||
	   (oldChild.bottom-oldChild.top != lpBand->rcChild.bottom-lpBand->rcChild.top))) {
	  TRACE("Child rectangle changed for band %u\n", i);
	  TRACE("    from (%ld,%ld)-(%ld,%ld)  to (%ld,%ld)-(%ld,%ld)\n",
		oldChild.left, oldChild.top,
	        oldChild.right, oldChild.bottom,
		lpBand->rcChild.left, lpBand->rcChild.top,
	        lpBand->rcChild.right, lpBand->rcChild.bottom);
      }
      if (lpBand->fDraw & NTF_INVALIDATE) {
	  TRACE("invalidating (%ld,%ld)-(%ld,%ld)\n",
		lpBand->rcBand.left,
		lpBand->rcBand.top,
		lpBand->rcBand.right + ((lpBand->fDraw & DRAW_RIGHTSEP) ? SEP_WIDTH_SIZE : 0),
		lpBand->rcBand.bottom + ((lpBand->fDraw & DRAW_BOTTOMSEP) ? SEP_WIDTH_SIZE : 0));
	  lpBand->fDraw &= ~NTF_INVALIDATE;
	  work = lpBand->rcBand;
	  if (lpBand->fDraw & DRAW_RIGHTSEP) work.right += SEP_WIDTH_SIZE;
	  if (lpBand->fDraw & DRAW_BOTTOMSEP) work.bottom += SEP_WIDTH_SIZE;
	  InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
      }

    }

}


static VOID
REBAR_CalcVertBand (REBAR_INFO *infoPtr, UINT rstart, UINT rend, BOOL notify)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Vertical bars. ***                     */
{
    REBAR_BAND *lpBand;
    UINT i, xoff, yoff;
    HWND parenthwnd;
    RECT oldChild, work;

    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (infoPtr->hwndSelf);

    for(i=rstart; i<rend; i++){
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	oldChild = lpBand->rcChild;

	/* set initial gripper rectangle */
	SetRect (&lpBand->rcGripper, lpBand->rcBand.left, lpBand->rcBand.top,
		 lpBand->rcBand.right, lpBand->rcBand.top);

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
		SetRect (&lpBand->rcCapImage, lpBand->rcBand.left,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE,
			 lpBand->rcBand.right,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE);
	    }
	    else {
		/*  horizontal gripper  */
		lpBand->rcGripper.left   += 2;
		lpBand->rcGripper.right  -= 2;
		lpBand->rcGripper.top    += REBAR_PRE_GRIPPER;
		lpBand->rcGripper.bottom  = lpBand->rcGripper.top + GRIPPER_WIDTH;

		/* initialize Caption image rectangle  */
		SetRect (&lpBand->rcCapImage, lpBand->rcBand.left,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE,
			 lpBand->rcBand.right,
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
		     lpBand->rcBand.left, lpBand->rcBand.top+xoff,
		     lpBand->rcBand.right, lpBand->rcBand.top+xoff);
	}

	/* image is visible */
	if (lpBand->fStatus & HAS_IMAGE) {
	    lpBand->fDraw |= DRAW_IMAGE;

	    lpBand->rcCapImage.right  = lpBand->rcCapImage.left + infoPtr->imageSize.cx;
	    lpBand->rcCapImage.bottom += infoPtr->imageSize.cy;

	    /* set initial caption text rectangle */
	    SetRect (&lpBand->rcCapText,
		     lpBand->rcBand.left, lpBand->rcCapImage.bottom+REBAR_POST_IMAGE,
		     lpBand->rcBand.right, lpBand->rcBand.top+lpBand->cxHeader);
	    /* update band height *
	       if (lpBand->uMinHeight < infoPtr->imageSize.cx + 2) {
	       lpBand->uMinHeight = infoPtr->imageSize.cx + 2;
	       lpBand->rcBand.right = lpBand->rcBand.left + lpBand->uMinHeight;
	       } */
	}
	else {
	    /* set initial caption text rectangle */
	    SetRect (&lpBand->rcCapText,
		     lpBand->rcBand.left, lpBand->rcCapImage.bottom,
		     lpBand->rcBand.right, lpBand->rcBand.top+lpBand->cxHeader);
	}

	/* text is visible */
	if ((lpBand->fStatus & HAS_TEXT) && !(lpBand->fStyle & RBBS_HIDETITLE)) {
	    lpBand->fDraw |= DRAW_TEXT;
	    lpBand->rcCapText.bottom = max(lpBand->rcCapText.top,
					   lpBand->rcCapText.bottom);
	}

	/* set initial child window rectangle if there is a child */
	if (lpBand->fMask & RBBIM_CHILD) {
	    yoff = lpBand->offChild.cx;
	    xoff = lpBand->offChild.cy;
	    SetRect (&lpBand->rcChild,
		     lpBand->rcBand.left+xoff, lpBand->rcBand.top+lpBand->cxHeader,
		     lpBand->rcBand.right-xoff, lpBand->rcBand.bottom-yoff);
	}
	else {
	    SetRect (&lpBand->rcChild,
		     lpBand->rcBand.left, lpBand->rcBand.top+lpBand->cxHeader,
		     lpBand->rcBand.right, lpBand->rcBand.bottom);
	}

	/* flag if notify required and invalidate rectangle */
	if (notify &&
	    ((oldChild.right-oldChild.left != lpBand->rcChild.right-lpBand->rcChild.left) ||
	     (oldChild.bottom-oldChild.top != lpBand->rcChild.bottom-lpBand->rcChild.top))) {
	    TRACE("Child rectangle changed for band %u\n", i);
	    TRACE("    from (%ld,%ld)-(%ld,%ld)  to (%ld,%ld)-(%ld,%ld)\n",
		  oldChild.left, oldChild.top,
		  oldChild.right, oldChild.bottom,
		  lpBand->rcChild.left, lpBand->rcChild.top,
		  lpBand->rcChild.right, lpBand->rcChild.bottom);
	}
	if (lpBand->fDraw & NTF_INVALIDATE) {
	    TRACE("invalidating (%ld,%ld)-(%ld,%ld)\n",
		  lpBand->rcBand.left,
		  lpBand->rcBand.top,
		  lpBand->rcBand.right + ((lpBand->fDraw & DRAW_BOTTOMSEP) ? SEP_WIDTH_SIZE : 0),
		  lpBand->rcBand.bottom + ((lpBand->fDraw & DRAW_RIGHTSEP) ? SEP_WIDTH_SIZE : 0));
	    lpBand->fDraw &= ~NTF_INVALIDATE;
	    work = lpBand->rcBand;
	    if (lpBand->fDraw & DRAW_RIGHTSEP) work.bottom += SEP_WIDTH_SIZE;
	    if (lpBand->fDraw & DRAW_BOTTOMSEP) work.right += SEP_WIDTH_SIZE;
	    InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
	}

    }
}


static VOID
REBAR_ForceResize (REBAR_INFO *infoPtr)
     /* Function: This changes the size of the REBAR window to that */
     /*  calculated by REBAR_Layout.                                */
{
    RECT rc;
    INT x, y, width, height;
    INT xedge = GetSystemMetrics(SM_CXEDGE);
    INT yedge = GetSystemMetrics(SM_CYEDGE);

    GetClientRect (infoPtr->hwndSelf, &rc);

    TRACE( " old [%ld x %ld], new [%ld x %ld], client [%ld x %ld]\n",
	   infoPtr->oldSize.cx, infoPtr->oldSize.cy,
	   infoPtr->calcSize.cx, infoPtr->calcSize.cy,
	   rc.right, rc.bottom);

    /* If we need to shrink client, then skip size test */
    if ((infoPtr->calcSize.cy >= rc.bottom) &&
	(infoPtr->calcSize.cx >= rc.right)) {

	/* if size did not change then skip process */
	if ((infoPtr->oldSize.cx == infoPtr->calcSize.cx) &&
	    (infoPtr->oldSize.cy == infoPtr->calcSize.cy) &&
	    !(infoPtr->fStatus & RESIZE_ANYHOW))
	    {
		TRACE("skipping reset\n");
		return;
	    }
    }

    infoPtr->fStatus &= ~RESIZE_ANYHOW;
    /* Set flag to ignore next WM_SIZE message */
    infoPtr->fStatus |= AUTO_RESIZE;

    width = 0;
    height = 0;
    x = 0;
    y = 0;

    if (infoPtr->dwStyle & WS_BORDER) {
	width = 2 * xedge;
	height = 2 * yedge;
    }

    if (!(infoPtr->dwStyle & CCS_NOPARENTALIGN)) {
	INT mode = infoPtr->dwStyle & (CCS_VERT | CCS_TOP | CCS_BOTTOM);
	RECT rcPcl;

	GetClientRect(GetParent(infoPtr->hwndSelf), &rcPcl);
	switch (mode) {
	case CCS_TOP:
	    /* _TOP sets width to parents width */
	    width += (rcPcl.right - rcPcl.left);
	    height += infoPtr->calcSize.cy;
	    x += ((infoPtr->dwStyle & WS_BORDER) ? -xedge : 0);
	    y += ((infoPtr->dwStyle & WS_BORDER) ? -yedge : 0);
	    y += ((infoPtr->dwStyle & CCS_NODIVIDER) ? 0 : REBAR_DIVIDER);
	    break;
	case CCS_BOTTOM:
	    /* FIXME: wrong wrong wrong */
	    /* _BOTTOM sets width to parents width */
	    width += (rcPcl.right - rcPcl.left);
	    height += infoPtr->calcSize.cy;
      	    x += -xedge;
	    y = rcPcl.bottom - height + 1;
	    break;
	case CCS_LEFT:
	    /* _LEFT sets height to parents height */
	    width += infoPtr->calcSize.cx;
	    height += (rcPcl.bottom - rcPcl.top);
	    x += ((infoPtr->dwStyle & WS_BORDER) ? -xedge : 0);
	    x += ((infoPtr->dwStyle & CCS_NODIVIDER) ? 0 : REBAR_DIVIDER);
	    y += ((infoPtr->dwStyle & WS_BORDER) ? -yedge : 0);
	    break;
	case CCS_RIGHT:
	    /* FIXME: wrong wrong wrong */
	    /* _RIGHT sets height to parents height */
	    width += infoPtr->calcSize.cx;
	    height += (rcPcl.bottom - rcPcl.top);
	    x = rcPcl.right - width + 1;
      	    y = -yedge;
	    break;
	default:
	    width += infoPtr->calcSize.cx;
	    height += infoPtr->calcSize.cy;
	}
    }
    else {
	width += infoPtr->calcSize.cx;
	height += infoPtr->calcSize.cy;
	x = infoPtr->origin.x;
	y = infoPtr->origin.y;
    }

    TRACE("hwnd %p, style=%08lx, setting at (%d,%d) for (%d,%d)\n",
	infoPtr->hwndSelf, infoPtr->dwStyle,
	x, y, width, height);
    SetWindowPos (infoPtr->hwndSelf, 0, x, y, width, height,
		    SWP_NOZORDER);
}


static VOID
REBAR_MoveChildWindows (REBAR_INFO *infoPtr, UINT start, UINT endplus)
{
    REBAR_BAND *lpBand;
    CHAR szClassName[40];
    UINT i;
    NMREBARCHILDSIZE  rbcz;
    NMHDR heightchange;
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
	    rbcz.rcBand = lpBand->rcBand;
	    if (infoPtr->dwStyle & CCS_VERT)
		rbcz.rcBand.top += lpBand->cxHeader;
	    else
		rbcz.rcBand.left += lpBand->cxHeader;
	    REBAR_Notify ((NMHDR *)&rbcz, infoPtr, RBN_CHILDSIZE);
	    if (!EqualRect (&lpBand->rcChild, &rbcz.rcChild)) {
		TRACE("Child rect changed by NOTIFY for band %u\n", i);
		TRACE("    from (%ld,%ld)-(%ld,%ld)  to (%ld,%ld)-(%ld,%ld)\n",
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

	    GetClassNameA (lpBand->hwndChild, szClassName, 40);
	    if (!lstrcmpA (szClassName, "ComboBox") ||
		!lstrcmpA (szClassName, WC_COMBOBOXEXA)) {
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

    if (infoPtr->fStatus & NTF_HGHTCHG) {
        infoPtr->fStatus &= ~NTF_HGHTCHG;
        /*
         * We need to force a resize here, because some applications
         * try to get the rebar size during processing of the 
         * RBN_HEIGHTCHANGE notification.
         */
        REBAR_ForceResize (infoPtr);
        REBAR_Notify (&heightchange, infoPtr, RBN_HEIGHTCHANGE);
    }

    /* native (from **1 above) does:
     *      UpdateWindow(rebar)
     *      REBAR_ForceResize
     *      RBN_HEIGHTCHANGE if necessary
     *      if ret from any EqualRect was 0
     *         Goto "BeginDeferWindowPos"
     */

}


static VOID
REBAR_Layout (REBAR_INFO *infoPtr, LPRECT lpRect, BOOL notify, BOOL resetclient)
     /* Function: This routine is resposible for laying out all */
     /*  the bands in a rebar. It assigns each band to a row and*/
     /*  determines when to start a new row.                    */
{
    REBAR_BAND *lpBand, *prevBand;
    RECT rcClient, rcAdj;
    INT initx, inity, x, y, cx, cxsep, mmcy, mcy, clientcx, clientcy;
    INT adjcx, adjcy, row, rightx, bottomy, origheight;
    UINT i, j, rowstart, origrows, cntonrow;
    BOOL dobreak;

    if (!(infoPtr->fStatus & BAND_NEEDS_LAYOUT)) {
	TRACE("no layout done. No band changed.\n");
	REBAR_DumpBand (infoPtr);
	return;
    }
    infoPtr->fStatus &= ~BAND_NEEDS_LAYOUT;
    if (!infoPtr->DoRedraw) infoPtr->fStatus |= BAND_NEEDS_REDRAW;

    GetClientRect (infoPtr->hwndSelf, &rcClient);
    TRACE("Client is (%ld,%ld)-(%ld,%ld)\n",
	  rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

    if (lpRect) {
	rcAdj = *lpRect;
	TRACE("adjustment rect is (%ld,%ld)-(%ld,%ld)\n",
	      rcAdj.left, rcAdj.top, rcAdj.right, rcAdj.bottom);
    }
    else {
        CopyRect (&rcAdj, &rcClient);
    }

    clientcx = rcClient.right - rcClient.left;
    clientcy = rcClient.bottom - rcClient.top;
    adjcx = rcAdj.right - rcAdj.left;
    adjcy = rcAdj.bottom - rcAdj.top;
    if (resetclient) {
        TRACE("window client rect will be set to adj rect\n");
        clientcx = adjcx;
        clientcy = adjcy;
    }

    if (!infoPtr->DoRedraw && (clientcx == 0) && (clientcy == 0)) {
	ERR("no redraw and client is zero, skip layout\n");
	infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
	return;
    }

    /* save height of original control */
    if (infoPtr->dwStyle & CCS_VERT)
        origheight = infoPtr->calcSize.cx;
    else
        origheight = infoPtr->calcSize.cy;
    origrows = infoPtr->uNumRows;

    initx = 0;
    inity = 0;

    /* ******* Start Phase 1 - all bands on row at minimum size ******* */

    TRACE("band loop constants, clientcx=%d, clientcy=%d, adjcx=%d, adjcy=%d\n",
	  clientcx, clientcy, adjcx, adjcy);
    x = initx;
    y = inity;
    row = 1;
    cx = 0;
    mcy = 0;
    rowstart = 0;
    prevBand = NULL;
    cntonrow = 0;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];
	lpBand->fDraw = 0;
	lpBand->iRow = row;

	SetRectEmpty(&lpBand->rcChevron);

	if (HIDDENBAND(lpBand)) continue;

	lpBand->rcoldBand = lpBand->rcBand;

	/* Set the offset of the child window */
	if ((lpBand->fMask & RBBIM_CHILD) &&
	    !(lpBand->fStyle & RBBS_FIXEDSIZE)) {
	    lpBand->offChild.cx = ((lpBand->fStyle & RBBS_CHILDEDGE) ? 4 : 0);
	}
	lpBand->offChild.cy = ((lpBand->fStyle & RBBS_CHILDEDGE) ? 2 : 0);

	/* separator from previous band */
	cxsep = (cntonrow == 0) ? 0 : SEP_WIDTH;
	cx = lpBand->lcx;

	if (infoPtr->dwStyle & CCS_VERT)
	    dobreak = (y + cx + cxsep > adjcy);
        else
	    dobreak = (x + cx + cxsep > adjcx);

	/* This is the check for whether we need to start a new row */
	if ( ( (lpBand->fStyle & RBBS_BREAK) && (i != 0) ) ||
	     ( ((infoPtr->dwStyle & CCS_VERT) ? (y != 0) : (x != 0)) && dobreak)) {

	    for (j = rowstart; j < i; j++) {
		REBAR_BAND *lpB;
		lpB = &infoPtr->bands[j];
		if (infoPtr->dwStyle & CCS_VERT) {
		    lpB->rcBand.right  = lpB->rcBand.left + mcy;
		}
		else {
		    lpB->rcBand.bottom = lpB->rcBand.top + mcy;
		}
	    }

	    TRACE("P1 Spliting to new row %d on band %u\n", row+1, i);
	    if (infoPtr->dwStyle & CCS_VERT) {
		y = inity;
		x += (mcy + SEP_WIDTH);
	    }
	    else {
		x = initx;
		y += (mcy + SEP_WIDTH);
	    }

	    mcy = 0;
	    cxsep = 0;
	    row++;
	    lpBand->iRow = row;
	    prevBand = NULL;
	    rowstart = i;
	    cntonrow = 0;
	}

	if (mcy < lpBand->lcy + REBARSPACE(lpBand))
	    mcy = lpBand->lcy + REBARSPACE(lpBand);

	/* if boundary rect specified then limit mcy */
	if (lpRect) {
	    if (infoPtr->dwStyle & CCS_VERT) {
	        if (x+mcy > adjcx) {
		    mcy = adjcx - x;
		    TRACE("P1 row %u limiting mcy=%d, adjcx=%d, x=%d\n",
			  i, mcy, adjcx, x);
		}
	    }
	    else {
	        if (y+mcy > adjcy) {
		    mcy = adjcy - y;
		    TRACE("P1 row %u limiting mcy=%d, adjcy=%d, y=%d\n",
			  i, mcy, adjcy, y);
		}
	    }
	}

	TRACE("P1 band %u, row %d, x=%d, y=%d, cxsep=%d, cx=%d\n",
	      i, row,
	      x, y, cxsep, cx);
	if (infoPtr->dwStyle & CCS_VERT) {
	    /* bound the bottom side if we have a bounding rectangle */
	    rightx = clientcx;
	    bottomy = (lpRect) ? min(clientcy, y+cxsep+cx) : y+cxsep+cx;
	    lpBand->rcBand.left   = x;
	    lpBand->rcBand.right  = x + min(mcy,
					    lpBand->lcy+REBARSPACE(lpBand));
	    lpBand->rcBand.top    = min(bottomy, y + cxsep);
	    lpBand->rcBand.bottom = bottomy;
	    lpBand->uMinHeight = lpBand->lcy;
	    y = bottomy;
	}
	else {
	    /* bound the right side if we have a bounding rectangle */
	    rightx = (lpRect) ? min(clientcx, x+cxsep+cx) : x+cxsep+cx;
	    bottomy = clientcy;
	    lpBand->rcBand.left   = min(rightx, x + cxsep);
	    lpBand->rcBand.right  = rightx;
	    lpBand->rcBand.top    = y;
	    lpBand->rcBand.bottom = y + min(mcy,
					    lpBand->lcy+REBARSPACE(lpBand));
	    lpBand->uMinHeight = lpBand->lcy;
	    x = rightx;
	}
	TRACE("P1 band %u, row %d, (%ld,%ld)-(%ld,%ld)\n",
	      i, row,
	      lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom);
	prevBand = lpBand;
	cntonrow++;

    } /* for (i = 0; i < infoPtr->uNumBands... */

    if (infoPtr->dwStyle & CCS_VERT)
        x += mcy;
    else
        y += mcy;

    for (j = rowstart; j < infoPtr->uNumBands; j++) {
	lpBand = &infoPtr->bands[j];
	if (infoPtr->dwStyle & CCS_VERT) {
	    lpBand->rcBand.right  = lpBand->rcBand.left + mcy;
	}
	else {
	    lpBand->rcBand.bottom = lpBand->rcBand.top + mcy;
	}
    }

    if (infoPtr->uNumBands)
        infoPtr->uNumRows = row;

    /* ******* End Phase 1 - all bands on row at minimum size ******* */


    /* ******* Start Phase 1a - Adjust heights for RBS_VARHEIGHT off ******* */

    mmcy = 0;
    if (!(infoPtr->dwStyle & RBS_VARHEIGHT)) {
	INT xy;

	/* get the max height of all bands */
	for (i=0; i<infoPtr->uNumBands; i++) {
	    lpBand = &infoPtr->bands[i];
	    if (HIDDENBAND(lpBand)) continue;
	    if (infoPtr->dwStyle & CCS_VERT)
		mmcy = max(mmcy, lpBand->rcBand.right - lpBand->rcBand.left);
	    else
		mmcy = max(mmcy, lpBand->rcBand.bottom - lpBand->rcBand.top);
	}

	/* now adjust all rectangles by using the height found above */
	xy = 0;
	row = 1;
	for (i=0; i<infoPtr->uNumBands; i++) {
	    lpBand = &infoPtr->bands[i];
	    if (HIDDENBAND(lpBand)) continue;
	    if (lpBand->iRow != row)
		xy += (mmcy + SEP_WIDTH);
	    if (infoPtr->dwStyle & CCS_VERT) {
		lpBand->rcBand.left = xy;
		lpBand->rcBand.right = xy + mmcy;
	    }
	    else {
		lpBand->rcBand.top = xy;
		lpBand->rcBand.bottom = xy + mmcy;
	    }
	}

	/* set the x/y values to the correct maximum */
	if (infoPtr->dwStyle & CCS_VERT)
	    x = xy + mmcy;
	else
	    y = xy + mmcy;
    }

    /* ******* End Phase 1a - Adjust heights for RBS_VARHEIGHT off ******* */


    /* ******* Start Phase 2 - split rows till adjustment height full ******* */

    /* assumes that the following variables contain:                 */
    /*   y/x     current height/width of all rows                    */
    if (lpRect) {
        INT i, prev_rh, new_rh, adj_rh, prev_idx, current_idx;
	REBAR_BAND *prev, *current, *walk;
	UINT j;

/* FIXME:  problem # 2 */
	if (((infoPtr->dwStyle & CCS_VERT) ?
#if PROBLEM2
	     (x < adjcx) : (y < adjcy)
#else
	     (adjcx - x > 5) : (adjcy - y > 4)
#endif
	     ) &&
	    (infoPtr->uNumBands > 1)) {
	    for (i=(INT)infoPtr->uNumBands-2; i>=0; i--) {
		TRACE("P2 adjcx=%d, adjcy=%d, x=%d, y=%d\n",
		      adjcx, adjcy, x, y);

		/* find the current band (starts at i+1) */
		current = &infoPtr->bands[i+1];
		current_idx = i+1;
		while (HIDDENBAND(current)) {
		    i--;
		    if (i < 0) break; /* out of bands */
		    current = &infoPtr->bands[i+1];
		    current_idx = i+1;
		}
		if (i < 0) break; /* out of bands */

		/* now find the prev band (starts at i) */
	        prev = &infoPtr->bands[i];
		prev_idx = i;
		while (HIDDENBAND(prev)) {
		    i--;
		    if (i < 0) break; /* out of bands */
		    prev = &infoPtr->bands[i];
		    prev_idx = i;
		}
		if (i < 0) break; /* out of bands */

		prev_rh = ircBw(prev);
		if (prev->iRow == current->iRow) {
		    new_rh = (infoPtr->dwStyle & RBS_VARHEIGHT) ?
			current->lcy + REBARSPACE(current) :
			mmcy;
		    adj_rh = new_rh + SEP_WIDTH;
		    infoPtr->uNumRows++;
		    current->fDraw |= NTF_INVALIDATE;
		    current->iRow++;
		    if (infoPtr->dwStyle & CCS_VERT) {
		        current->rcBand.top = inity;
			current->rcBand.bottom = clientcy;
			current->rcBand.left += (prev_rh + SEP_WIDTH);
			current->rcBand.right = current->rcBand.left + new_rh;
			x += adj_rh;
		    }
		    else {
		        current->rcBand.left = initx;
			current->rcBand.right = clientcx;
			current->rcBand.top += (prev_rh + SEP_WIDTH);
			current->rcBand.bottom = current->rcBand.top + new_rh;
			y += adj_rh;
		    }
		    TRACE("P2 moving band %d to own row at (%ld,%ld)-(%ld,%ld)\n",
			  current_idx,
			  current->rcBand.left, current->rcBand.top,
			  current->rcBand.right, current->rcBand.bottom);
		    TRACE("P2 prev band %d at (%ld,%ld)-(%ld,%ld)\n",
			  prev_idx,
			  prev->rcBand.left, prev->rcBand.top,
			  prev->rcBand.right, prev->rcBand.bottom);
		    TRACE("P2 values: prev_rh=%d, new_rh=%d, adj_rh=%d\n",
			  prev_rh, new_rh, adj_rh);
		    /* for bands below current adjust row # and top/bottom */
		    for (j = current_idx+1; j<infoPtr->uNumBands; j++) {
		        walk = &infoPtr->bands[j];
			if (HIDDENBAND(walk)) continue;
			walk->fDraw |= NTF_INVALIDATE;
			walk->iRow++;
			if (infoPtr->dwStyle & CCS_VERT) {
			    walk->rcBand.left += adj_rh;
			    walk->rcBand.right += adj_rh;
			}
			else {
			    walk->rcBand.top += adj_rh;
			    walk->rcBand.bottom += adj_rh;
			}
		    }
		    if ((infoPtr->dwStyle & CCS_VERT) ? (x >= adjcx) : (y >= adjcy))
		        break; /* all done */
		}
	    }
	}
    }

    /* ******* End Phase 2 - split rows till adjustment height full ******* */


    /* ******* Start Phase 2a - mark first and last band in each ******* */

    prevBand = NULL;
    for (i = 0; i < infoPtr->uNumBands; i++) { 	 
        lpBand = &infoPtr->bands[i]; 	 
        if (HIDDENBAND(lpBand))
            continue;
        if( !prevBand ) {
            lpBand->fDraw |= DRAW_FIRST_IN_ROW;
            prevBand = lpBand;
        }
        else if( prevBand->iRow == lpBand->iRow )
            prevBand = lpBand;
        else {
            prevBand->fDraw |= DRAW_LAST_IN_ROW;
            lpBand->fDraw |= DRAW_FIRST_IN_ROW;
            prevBand = lpBand;
        }
    }
    if( prevBand )
        prevBand->fDraw |= DRAW_LAST_IN_ROW;

    /* ******* End Phase 2a - mark first and last band in each ******* */


    /* ******* Start Phase 2b - adjust all bands for height full ******* */
    /* assumes that the following variables contain:                 */
    /*   y/x     current height/width of all rows                    */
    /*   clientcy/clientcx     height/width of client area           */

    if (((infoPtr->dwStyle & CCS_VERT) ? clientcx > x : clientcy > y) &&
	infoPtr->uNumBands) {
	INT diff, i;
	UINT j;

	diff = (infoPtr->dwStyle & CCS_VERT) ? clientcx - x : clientcy - y;

        /* iterate backwards thru the rows */
        for (i = infoPtr->uNumBands-1; i>=0; i--) {
	    lpBand = &infoPtr->bands[i];
	    if(HIDDENBAND(lpBand)) continue;

	    /* if row has more than 1 band, ignore it */
            if( !(lpBand->fDraw&DRAW_FIRST_IN_ROW) )
                continue;
            if( !(lpBand->fDraw&DRAW_LAST_IN_ROW) )
                continue;

	    if (lpBand->fMask & RBBS_VARIABLEHEIGHT) continue;
	    if (((INT)lpBand->cyMaxChild < 1) ||
		((INT)lpBand->cyIntegral < 1)) {
		if (lpBand->cyMaxChild + lpBand->cyIntegral == 0) continue;
		ERR("P2b band %u RBBS_VARIABLEHEIGHT set but cyMax=%d, cyInt=%d\n",
		    i, lpBand->cyMaxChild, lpBand->cyIntegral);
		continue;
	    }
	    /* j is now the maximum height/width in the client area */
	    j = ((diff / lpBand->cyIntegral) * lpBand->cyIntegral) +
		ircBw(lpBand);
	    if (j > lpBand->cyMaxChild + REBARSPACE(lpBand))
		j = lpBand->cyMaxChild + REBARSPACE(lpBand);
	    diff -= (j - ircBw(lpBand));
	    if (infoPtr->dwStyle & CCS_VERT)
		lpBand->rcBand.right = lpBand->rcBand.left + j;
	    else
		lpBand->rcBand.bottom = lpBand->rcBand.top + j;
	    TRACE("P2b band %d, row %d changed to (%ld,%ld)-(%ld,%ld)\n",
		  i, lpBand->iRow,
		  lpBand->rcBand.left, lpBand->rcBand.top,
		  lpBand->rcBand.right, lpBand->rcBand.bottom);
	    if (diff <= 0) break;
	}
	if (diff < 0) {
	    ERR("P2b allocated more than available, diff=%d\n", diff);
	    diff = 0;
	}
	if (infoPtr->dwStyle & CCS_VERT)
	    x = clientcx - diff;
	else
	    y = clientcy - diff;
    }

    /* ******* End Phase 2b - adjust all bands for height full ******* */


    /* ******* Start Phase 3 - adjust all bands for width full ******* */

    if (infoPtr->uNumBands) {
        int startband;

	/* If RBS_BANDBORDERS set then indicate to draw bottom separator */
	/* on all bands in all rows but last row.                        */
	/* Also indicate to draw the right separator for each band in    */
	/* each row but the rightmost band.                              */
	if (infoPtr->dwStyle & RBS_BANDBORDERS) {

            for (i=0; i<infoPtr->uNumBands; i++) {
	        lpBand = &infoPtr->bands[i];
		if (HIDDENBAND(lpBand))
                    continue;

                /* not righthand bands */
                if( !(lpBand->fDraw & DRAW_LAST_IN_ROW) )
		    lpBand->fDraw |= DRAW_RIGHTSEP;

                /* not the last row */
                if( lpBand->iRow != infoPtr->uNumRows )
		    lpBand->fDraw |= DRAW_BOTTOMSEP;
	    }
	}

	/* Distribute the extra space on the horizontal and adjust  */
	/* all bands in row to same height.                         */
	mcy = 0;
        startband = -1;
        for (i=0; i<infoPtr->uNumBands; i++) {

            lpBand = &infoPtr->bands[i];

            if( lpBand->fDraw & DRAW_FIRST_IN_ROW )
            {
                startband = i;
                mcy = 0;
            }

            if ( (mcy < ircBw(lpBand)) && !HIDDENBAND(lpBand) )
                mcy = ircBw(lpBand);

            if( lpBand->fDraw & DRAW_LAST_IN_ROW )
            {
	        TRACE("P3 processing row %d, starting band %d, ending band %d\n",
		      lpBand->iRow, startband, i);
                if( startband < 0 )
                    ERR("Last band %d with no first, row %d\n", i, lpBand->iRow);

	        REBAR_AdjustBands (infoPtr, startband, i,
			       (infoPtr->dwStyle & CCS_VERT) ?
			       clientcy : clientcx, mcy);
            }
	}

	/* Calculate the other rectangles in each band */
	if (infoPtr->dwStyle & CCS_VERT) {
	    REBAR_CalcVertBand (infoPtr, 0, infoPtr->uNumBands,
				notify);
	}
	else {
	    REBAR_CalcHorzBand (infoPtr, 0, infoPtr->uNumBands,
				notify);
	}
    }

    /* ******* End Phase 3 - adjust all bands for width full ******* */

    /* now compute size of Rebar itself */
    infoPtr->oldSize = infoPtr->calcSize;
    if (infoPtr->uNumBands == 0) {
	/* we have no bands, so make size the size of client */
	x = clientcx;
	y = clientcy;
    }
    if (infoPtr->dwStyle & CCS_VERT) {
        if( x < REBAR_MINSIZE )
            x = REBAR_MINSIZE;
	infoPtr->calcSize.cx = x;
	infoPtr->calcSize.cy = clientcy;
	TRACE("vert, notify=%d, x=%d, origheight=%d\n",
	      notify, x, origheight);
	if (notify && (x != origheight)) infoPtr->fStatus |= NTF_HGHTCHG;
    }
    else {
        if( y < REBAR_MINSIZE )
            y = REBAR_MINSIZE;
	infoPtr->calcSize.cx = clientcx;
	infoPtr->calcSize.cy = y;
	TRACE("horz, notify=%d, y=%d, origheight=%d\n",
	      notify, y, origheight);
	if (notify && (y != origheight)) infoPtr->fStatus |= NTF_HGHTCHG;
    }

    REBAR_DumpBand (infoPtr);

    REBAR_MoveChildWindows (infoPtr, 0, infoPtr->uNumBands);

    REBAR_ForceResize (infoPtr);
}


static VOID
REBAR_ValidateBand (REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
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
    lpBand->ccx = 0;
    lpBand->ccy = 0;
    lpBand->hcx = 0;
    lpBand->hcy = 0;

    /* Data comming in from users into the cx... and cy... fields  */
    /* may be bad, just garbage, because the user never clears     */
    /* the fields. RB_{SET|INSERT}BAND{A|W} just passes the data   */
    /* along if the fields exist in the input area. Here we must   */
    /* determine if the data is valid. I have no idea how MS does  */
    /* the validation, but it does because the RB_GETBANDINFO      */
    /* returns a 0 when I know the sample program passed in an     */
    /* address. Here I will use the algorithim that if the value   */
    /* is greater than 65535 then it is bad and replace it with    */
    /* a zero. Feel free to improve the algorithim.  -  GA 12/2000 */
    if (lpBand->cxMinChild > 65535) lpBand->cxMinChild = 0;
    if (lpBand->cyMinChild > 65535) lpBand->cyMinChild = 0;
    if (lpBand->cx         > 65535) lpBand->cx         = 0;
    if (lpBand->cyChild    > 65535) lpBand->cyChild    = 0;
    if (lpBand->cyMaxChild > 65535) lpBand->cyMaxChild = 0;
    if (lpBand->cyIntegral > 65535) lpBand->cyIntegral = 0;
    if (lpBand->cxIdeal    > 65535) lpBand->cxIdeal    = 0;
    if (lpBand->cxHeader   > 65535) lpBand->cxHeader   = 0;

    /* FIXME: probably should only set NEEDS_LAYOUT flag when */
    /*        values change. Till then always set it.         */
    TRACE("setting NEEDS_LAYOUT\n");
    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;

    /* Header is where the image, text and gripper exist  */
    /* in the band and preceed the child window.          */

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
    if (!(lpBand->fMask & RBBIM_HEADERSIZE))
        lpBand->cxHeader = header;


    /* Now compute minimum size of child window */
    lpBand->offChild.cx = 0;
    lpBand->offChild.cy = 0;
    lpBand->lcy = textheight;
    lpBand->ccy = lpBand->lcy;
    if (lpBand->fMask & RBBIM_CHILDSIZE) {
        lpBand->lcx = lpBand->cxMinChild;

	/* Set the .cy values for CHILDSIZE case */
        lpBand->lcy = max(lpBand->lcy, lpBand->cyMinChild);
	lpBand->ccy = lpBand->lcy;
        lpBand->hcy = lpBand->lcy;
        if (lpBand->cyMaxChild != 0xffffffff) {
	    lpBand->hcy = lpBand->cyMaxChild;
        }
	if (lpBand->cyChild != 0xffffffff)
	    lpBand->ccy = max (lpBand->cyChild, lpBand->lcy);

        TRACE("_CHILDSIZE\n");
    }
    if (lpBand->fMask & RBBIM_SIZE) {
        lpBand->hcx = max (lpBand->cx-lpBand->cxHeader, lpBand->lcx);
        TRACE("_SIZE\n");
    }
    else
        lpBand->hcx = lpBand->lcx;
    lpBand->ccx = lpBand->hcx;

    /* make ->.cx include header size for _Layout */
    lpBand->lcx += lpBand->cxHeader;
    lpBand->ccx += lpBand->cxHeader;
    lpBand->hcx += lpBand->cxHeader;

}

static BOOL
REBAR_CommonSetupBand (HWND hwnd, LPREBARBANDINFOA lprbbi, REBAR_BAND *lpBand)
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
	    /* below in trace fro WinRAR */
	    ShowWindow(lpBand->hwndChild, SW_SHOWNOACTIVATE | SW_SHOWNORMAL);
	    /* above in trace fro WinRAR */
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
	if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	    lpBand->cyChild    = lprbbi->cyChild;
	    lpBand->cyMaxChild = lprbbi->cyMaxChild;
	    lpBand->cyIntegral = lprbbi->cyIntegral;
	}
	else { /* special case - these should be zeroed out since   */
	       /* RBBIM_CHILDSIZE added these in WIN32_IE >= 0x0400 */
	    lpBand->cyChild    = 0;
	    lpBand->cyMaxChild = 0;
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
            bChanged = TRUE;
        }
    }

    return bChanged;
}

static LRESULT
REBAR_InternalEraseBkGnd (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam, RECT *clip)
     /* Function:  This erases the background rectangle by drawing  */
     /*  each band with its background color (or the default) and   */
     /*  draws each bands right separator if necessary. The row     */
     /*  separators are drawn on the first band of the next row.    */
{
    REBAR_BAND *lpBand;
    UINT i;
    INT oldrow;
    HDC hdc = (HDC)wParam;
    RECT rect;
    COLORREF old = CLR_NONE, new;

    oldrow = -1;
    for(i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;

	/* draw band separator between rows */
	if (lpBand->iRow != oldrow) {
	    oldrow = lpBand->iRow;
	    if (lpBand->fDraw & DRAW_BOTTOMSEP) {
		RECT rcRowSep;
		rcRowSep = lpBand->rcBand;
		if (infoPtr->dwStyle & CCS_VERT) {
		    rcRowSep.right += SEP_WIDTH_SIZE;
		    rcRowSep.bottom = infoPtr->calcSize.cy;
		    DrawEdge (hdc, &rcRowSep, EDGE_ETCHED, BF_RIGHT);
		}
		else {
		    rcRowSep.bottom += SEP_WIDTH_SIZE;
		    rcRowSep.right = infoPtr->calcSize.cx;
		    DrawEdge (hdc, &rcRowSep, EDGE_ETCHED, BF_BOTTOM);
		}
		TRACE ("drawing band separator bottom (%ld,%ld)-(%ld,%ld)\n",
		       rcRowSep.left, rcRowSep.top,
		       rcRowSep.right, rcRowSep.bottom);
	    }
	}

	/* draw band separator between bands in a row */
	if (lpBand->fDraw & DRAW_RIGHTSEP) {
	    RECT rcSep;
	    rcSep = lpBand->rcBand;
	    if (infoPtr->dwStyle & CCS_VERT) {
		rcSep.bottom += SEP_WIDTH_SIZE;
		DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_BOTTOM);
	    }
	    else {
		rcSep.right += SEP_WIDTH_SIZE;
		DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_RIGHT);
	    }
	    TRACE("drawing band separator right (%ld,%ld)-(%ld,%ld)\n",
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
	old = SetBkColor (hdc, new);

	rect = lpBand->rcBand;
	TRACE("%s background color=0x%06lx, band (%ld,%ld)-(%ld,%ld), clip (%ld,%ld)-(%ld,%ld)\n",
	      (lpBand->clrBack == CLR_NONE) ? "none" :
	        ((lpBand->clrBack == CLR_DEFAULT) ? "dft" : ""),
	      GetBkColor(hdc),
	      lpBand->rcBand.left,lpBand->rcBand.top,
	      lpBand->rcBand.right,lpBand->rcBand.bottom,
	      clip->left, clip->top,
	      clip->right, clip->bottom);
	ExtTextOutA (hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, 0);
	if (lpBand->clrBack != CLR_NONE)
	    SetBkColor (hdc, old);
    }
    return TRUE;
}

static void
REBAR_InternalHitTest (REBAR_INFO *infoPtr, const LPPOINT lpPt, UINT *pFlags, INT *pBand)
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
		lpBand = &infoPtr->bands[iCount];
		if (HIDDENBAND(lpBand)) continue;
		if (PtInRect (&lpBand->rcBand, *lpPt)) {
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


static INT
REBAR_Shrink (REBAR_INFO *infoPtr, REBAR_BAND *band, INT movement, INT i)
     /* Function:  This attempts to shrink the given band by the  */
     /*  the amount in "movement". A shrink to the left is indi-  */
     /*  cated by "movement" being negative. "i" is merely the    */
     /*  band index for trace messages.                           */
{
    INT Leadjust, Readjust, avail, ret;

    /* Note: a left drag is indicated by "movement" being negative.  */
    /*       Similarly, a right drag is indicated by "movement"      */
    /*       being positive. "movement" should never be 0, but if    */
    /*       it is then the band does not move.                      */

    avail = rcBw(band) - band->lcx;

    /* now compute the Left End adjustment factor and Right End */
    /* adjustment factor. They may be different if shrinking.   */
    if (avail <= 0) {
        /* if this band is not shrinkable, then just move it */
        Leadjust = Readjust = movement;
	ret = movement;
    }
    else {
        if (movement < 0) {
	    /* Drag to left */
	    if (avail <= abs(movement)) {
	        Readjust = movement;
		Leadjust = movement + avail;
		ret = Leadjust;
	    }
	    else {
	        Readjust = movement;
		Leadjust = 0;
		ret = 0;
	    }
	}
	else {
	    /* Drag to right */
	    if (avail <= abs(movement)) {
	        Leadjust = movement;
		Readjust = movement - avail;
		ret = Readjust;
	    }
	    else {
	        Leadjust = movement;
		Readjust = 0;
		ret = 0;
	    }
	}
    }

    /* Reasonability Check */
    if (rcBlt(band) + Leadjust < 0) {
        ERR("adjustment will fail, band %d: left=%d, right=%d, move=%d, rtn=%d\n",
	    i, Leadjust, Readjust, movement, ret);
    }

    LEADJ(band, Leadjust);
    READJ(band, Readjust);

    TRACE("band %d:  left=%d, right=%d, move=%d, rtn=%d, rcBand=(%ld,%ld)-(%ld,%ld)\n",
	  i, Leadjust, Readjust, movement, ret,
	  band->rcBand.left, band->rcBand.top,
	  band->rcBand.right, band->rcBand.bottom);
    return ret;
}


static void
REBAR_HandleLRDrag (REBAR_INFO *infoPtr, POINTS *ptsmove)
     /* Function:  This will implement the functionality of a     */
     /*  Gripper drag within a row. It will not implement "out-   */
     /*  of-row" drags. (They are detected and handled in         */
     /*  REBAR_MouseMove.)                                        */
     /*  **** FIXME Switching order of bands in a row not   ****  */
     /*  ****       yet implemented.                        ****  */
{
    REBAR_BAND *hitBand, *band, *mindBand, *maxdBand;
    RECT newrect;
    INT imindBand = -1, imaxdBand, ihitBand, i, movement;
    INT RHeaderSum = 0, LHeaderSum = 0;
    INT compress;

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

    ihitBand = infoPtr->iGrabbedBand;
    hitBand = &infoPtr->bands[ihitBand];
    imaxdBand = ihitBand; /* to suppress warning message */

    /* find all the bands in the row of the one whose Gripper was seized */
    for (i=0; i<infoPtr->uNumBands; i++) {
        band = &infoPtr->bands[i];
	if (HIDDENBAND(band)) continue;
	if (band->iRow == hitBand->iRow) {
	    imaxdBand = i;
	    if (imindBand == -1) imindBand = i;
	    /* minimum size of each band is size of header plus            */
	    /* size of minimum child plus offset of child from header plus */
	    /* a one to separate each band.                                */
	    if (i < ihitBand)
	        LHeaderSum += (band->lcx + SEP_WIDTH);
	    else
	        RHeaderSum += (band->lcx + SEP_WIDTH);

	}
    }
    if (RHeaderSum) RHeaderSum -= SEP_WIDTH; /* no separator after last band */

    mindBand = &infoPtr->bands[imindBand];
    maxdBand = &infoPtr->bands[imaxdBand];

    if (imindBand == imaxdBand) return; /* nothing to drag against */
    if (imindBand == ihitBand) return; /* first band in row, can't drag */

    /* limit movement to inside adjustable bands - Left */
    if ( (ptsmove->x < mindBand->rcBand.left) ||
	 (ptsmove->x > maxdBand->rcBand.right) ||
	 (ptsmove->y < mindBand->rcBand.top) ||
	 (ptsmove->y > maxdBand->rcBand.bottom))
        return; /* should swap bands */

    if (infoPtr->dwStyle & CCS_VERT)
        movement = ptsmove->y - ((hitBand->rcBand.top+REBAR_PRE_GRIPPER) -
			     infoPtr->ihitoffset);
    else
        movement = ptsmove->x - ((hitBand->rcBand.left+REBAR_PRE_GRIPPER) -
			     infoPtr->ihitoffset);
    infoPtr->dragNow = *ptsmove;

    TRACE("before: movement=%d (%d,%d), imindBand=%d, ihitBand=%d, imaxdBand=%d, LSum=%d, RSum=%d\n",
	  movement, ptsmove->x, ptsmove->y, imindBand, ihitBand,
	  imaxdBand, LHeaderSum, RHeaderSum);
    REBAR_DumpBand (infoPtr);

    if (movement < 0) {

        /* ***  Drag left/up *** */
        compress = rcBlt(hitBand) - rcBlt(mindBand) -
	           LHeaderSum;
	if (compress < abs(movement)) {
	    TRACE("limiting left drag, was %d changed to %d\n",
		  movement, -compress);
	    movement = -compress;
	}

        for (i=ihitBand; i>=imindBand; i--) {
	    band = &infoPtr->bands[i];
	    if (HIDDENBAND(band)) continue;
	    if (i == ihitBand) {
		LEADJ(band, movement);
	    }
	    else
	        movement = REBAR_Shrink (infoPtr, band, movement, i);
	    band->ccx = rcBw(band);
	}
    }
    else {
	BOOL first = TRUE;

        /* ***  Drag right/down *** */
        compress = rcBrb(maxdBand) - rcBlt(hitBand) -
	           RHeaderSum;
	if (compress < abs(movement)) {
	    TRACE("limiting right drag, was %d changed to %d\n",
		  movement, compress);
	    movement = compress;
	}
        for (i=ihitBand-1; i<=imaxdBand; i++) {
	    band = &infoPtr->bands[i];
	    if (HIDDENBAND(band)) continue;
	    if (first) {
		first = FALSE;
		READJ(band, movement);
	    }
	    else
	        movement = REBAR_Shrink (infoPtr, band, movement, i);
	    band->ccx = rcBw(band);
	}
    }

    /* recompute all rectangles */
    if (infoPtr->dwStyle & CCS_VERT) {
	REBAR_CalcVertBand (infoPtr, imindBand, imaxdBand+1,
			    FALSE);
    }
    else {
	REBAR_CalcHorzBand (infoPtr, imindBand, imaxdBand+1,
			    FALSE);
    }

    TRACE("bands after adjustment, see band # %d, %d\n",
	  imindBand, imaxdBand);
    REBAR_DumpBand (infoPtr);

    SetRect (&newrect,
	     mindBand->rcBand.left,
	     mindBand->rcBand.top,
	     maxdBand->rcBand.right,
	     maxdBand->rcBand.bottom);

    REBAR_MoveChildWindows (infoPtr, imindBand, imaxdBand+1);

    InvalidateRect (infoPtr->hwndSelf, &newrect, TRUE);
    UpdateWindow (infoPtr->hwndSelf);

}



/* << REBAR_BeginDrag >> */


static LRESULT
REBAR_DeleteBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    UINT uBand = (UINT)wParam;
    HWND childhwnd = 0;
    REBAR_BAND *lpBand;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("deleting band %u!\n", uBand);
    lpBand = &infoPtr->bands[uBand];
    REBAR_Notify_NMREBAR (infoPtr, uBand, RBN_DELETINGBAND);

    if (infoPtr->uNumBands == 1) {
	TRACE(" simple delete!\n");
	if ((lpBand->fMask & RBBIM_CHILD) && lpBand->hwndChild)
	    childhwnd = lpBand->hwndChild;
	Free (infoPtr->bands);
	infoPtr->bands = NULL;
	infoPtr->uNumBands = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
        TRACE("complex delete! [uBand=%u]\n", uBand);

	if ((lpBand->fMask & RBBIM_CHILD) && lpBand->hwndChild)
	    childhwnd = lpBand->hwndChild;

	infoPtr->uNumBands--;
	infoPtr->bands = Alloc (sizeof (REBAR_BAND) * infoPtr->uNumBands);
        if (uBand > 0) {
            memcpy (&infoPtr->bands[0], &oldBands[0],
                    uBand * sizeof(REBAR_BAND));
        }

        if (uBand < infoPtr->uNumBands) {
            memcpy (&infoPtr->bands[uBand], &oldBands[uBand+1],
                    (infoPtr->uNumBands - uBand) * sizeof(REBAR_BAND));
        }

	Free (oldBands);
    }

    if (childhwnd)
        ShowWindow (childhwnd, SW_HIDE);

    REBAR_Notify_NMREBAR (infoPtr, -1, RBN_DELETEDBAND);

    /* if only 1 band left the re-validate to possible eliminate gripper */
    if (infoPtr->uNumBands == 1)
      REBAR_ValidateBand (infoPtr, &infoPtr->bands[0]);

    TRACE("setting NEEDS_LAYOUT\n");
    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
    infoPtr->fStatus |= RESIZE_ANYHOW;
    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);

    return TRUE;
}


/* << REBAR_DragMove >> */
/* << REBAR_EndDrag >> */


static LRESULT
REBAR_GetBandBorders (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
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


inline static LRESULT
REBAR_GetBandCount (REBAR_INFO *infoPtr)
{
    TRACE("band count %u!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetBandInfoA (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

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

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
      if (lpBand->lpText && (lpBand->fMask & RBBIM_TEXT))
      {
          if (!WideCharToMultiByte( CP_ACP, 0, lpBand->lpText, -1,
                                    lprbbi->lpText, lprbbi->cch, NULL, NULL ))
              lprbbi->lpText[lprbbi->cch-1] = 0;
      }
      else
	*lprbbi->lpText = 0;
    }

    if (lprbbi->fMask & RBBIM_IMAGE) {
      if (lpBand->fMask & RBBIM_IMAGE)
	lprbbi->iImage = lpBand->iImage;
      else
	lprbbi->iImage = -1;
    }

    if (lprbbi->fMask & RBBIM_CHILD)
	lprbbi->hwndChild = lpBand->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lprbbi->cxMinChild = lpBand->cxMinChild;
	lprbbi->cyMinChild = lpBand->cyMinChild;
	if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
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

    REBAR_DumpBandInfo (lprbbi);

    return TRUE;
}


static LRESULT
REBAR_GetBandInfoW (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOW_V3_SIZE)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

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

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
      if (lpBand->lpText && (lpBand->fMask & RBBIM_TEXT))
	lstrcpynW (lprbbi->lpText, lpBand->lpText, lprbbi->cch);
      else
	*lprbbi->lpText = 0;
    }

    if (lprbbi->fMask & RBBIM_IMAGE) {
      if (lpBand->fMask & RBBIM_IMAGE)
	lprbbi->iImage = lpBand->iImage;
      else
	lprbbi->iImage = -1;
    }

    if (lprbbi->fMask & RBBIM_CHILD)
	lprbbi->hwndChild = lpBand->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lprbbi->cxMinChild = lpBand->cxMinChild;
	lprbbi->cyMinChild = lpBand->cyMinChild;
	if (lprbbi->cbSize >= sizeof (REBARBANDINFOW)) {
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
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOW)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lprbbi->cxIdeal = lpBand->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lprbbi->lParam = lpBand->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lprbbi->cxHeader = lpBand->cxHeader;
    }

    REBAR_DumpBandInfo ((LPREBARBANDINFOA)lprbbi);

    return TRUE;
}


static LRESULT
REBAR_GetBarHeight (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT nHeight;

    nHeight = (infoPtr->dwStyle & CCS_VERT) ? infoPtr->calcSize.cx : infoPtr->calcSize.cy;

    TRACE("height = %d\n", nHeight);

    return nHeight;
}


static LRESULT
REBAR_GetBarInfo (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
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


inline static LRESULT
REBAR_GetBkColor (REBAR_INFO *infoPtr)
{
    COLORREF clr = infoPtr->clrBk;

    if (clr == CLR_DEFAULT)
      clr = infoPtr->clrBtnFace;

    TRACE("background color 0x%06lx!\n", clr);

    return clr;
}


/* << REBAR_GetColorScheme >> */
/* << REBAR_GetDropTarget >> */


static LRESULT
REBAR_GetPalette (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    FIXME("empty stub!\n");

    return 0;
}


static LRESULT
REBAR_GetRect (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT iBand = (INT)wParam;
    LPRECT lprc = (LPRECT)lParam;
    REBAR_BAND *lpBand;

    if ((iBand < 0) && ((UINT)iBand >= infoPtr->uNumBands))
	return FALSE;
    if (!lprc)
	return FALSE;

    lpBand = &infoPtr->bands[iBand];
    CopyRect (lprc, &lpBand->rcBand);

    TRACE("band %d, (%ld,%ld)-(%ld,%ld)\n", iBand,
	  lprc->left, lprc->top, lprc->right, lprc->bottom);

    return TRUE;
}


inline static LRESULT
REBAR_GetRowCount (REBAR_INFO *infoPtr)
{
    TRACE("%u\n", infoPtr->uNumRows);

    return infoPtr->uNumRows;
}


static LRESULT
REBAR_GetRowHeight (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT iRow = (INT)wParam;
    int j = 0, ret = 0;
    UINT i;
    REBAR_BAND *lpBand;

    for (i=0; i<infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	if (lpBand->iRow != iRow) continue;
	if (infoPtr->dwStyle & CCS_VERT)
	    j = lpBand->rcBand.right - lpBand->rcBand.left;
	else
	    j = lpBand->rcBand.bottom - lpBand->rcBand.top;
	if (j > ret) ret = j;
    }

    TRACE("row %d, height %d\n", iRow, ret);

    return ret;
}


inline static LRESULT
REBAR_GetTextColor (REBAR_INFO *infoPtr)
{
    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return infoPtr->clrText;
}


inline static LRESULT
REBAR_GetToolTips (REBAR_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hwndToolTip;
}


inline static LRESULT
REBAR_GetUnicodeFormat (REBAR_INFO *infoPtr)
{
    TRACE("%s hwnd=%p\n",
	  infoPtr->bUnicode ? "TRUE" : "FALSE", infoPtr->hwndSelf);

    return infoPtr->bUnicode;
}


inline static LRESULT
REBAR_GetVersion (REBAR_INFO *infoPtr)
{
    TRACE("version %d\n", infoPtr->iVersion);
    return infoPtr->iVersion;
}


static LRESULT
REBAR_HitTest (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPRBHITTESTINFO lprbht = (LPRBHITTESTINFO)lParam;

    if (!lprbht)
	return -1;

    REBAR_InternalHitTest (infoPtr, &lprbht->pt, &lprbht->flags, &lprbht->iBand);

    return lprbht->iBand;
}


static LRESULT
REBAR_IdToIndex (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
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
REBAR_InsertBandA (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    UINT uIndex = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOA_V3_SIZE)
	return FALSE;

    /* trace the index as signed to see the -1 */
    TRACE("insert band at %d!\n", (INT)uIndex);
    REBAR_DumpBandInfo (lprbbi);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT)uIndex == -1) || (uIndex > infoPtr->uNumBands))
	    uIndex = infoPtr->uNumBands;

	/* pre insert copy */
	if (uIndex > 0) {
	    memcpy (&infoPtr->bands[0], &oldBands[0],
		    uIndex * sizeof(REBAR_BAND));
	}

	/* post copy */
	if (uIndex < infoPtr->uNumBands - 1) {
	    memcpy (&infoPtr->bands[uIndex+1], &oldBands[uIndex],
		    (infoPtr->uNumBands - uIndex - 1) * sizeof(REBAR_BAND));
	}

	Free (oldBands);
    }

    infoPtr->uNumBands++;

    TRACE("index %u!\n", uIndex);

    /* initialize band (infoPtr->bands[uIndex])*/
    lpBand = &infoPtr->bands[uIndex];
    lpBand->fMask = 0;
    lpBand->fStatus = 0;
    lpBand->clrFore = infoPtr->clrText;
    lpBand->clrBack = infoPtr->clrBk;
    lpBand->hwndChild = 0;
    lpBand->hwndPrevParent = 0;

    REBAR_CommonSetupBand (infoPtr->hwndSelf, lprbbi, lpBand);
    lpBand->lpText = NULL;
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
        INT len = MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, NULL, 0 );
        if (len > 1) {
            lpBand->lpText = (LPWSTR)Alloc (len*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, lpBand->lpText, len );
	}
    }

    REBAR_ValidateBand (infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (infoPtr, &infoPtr->bands[0]);

    REBAR_DumpBand (infoPtr);

    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);
    InvalidateRect(infoPtr->hwndSelf, 0, 1);

    return TRUE;
}


static LRESULT
REBAR_InsertBandW (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    UINT uIndex = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOW_V3_SIZE)
	return FALSE;

    /* trace the index as signed to see the -1 */
    TRACE("insert band at %d!\n", (INT)uIndex);
    REBAR_DumpBandInfo ((LPREBARBANDINFOA)lprbbi);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT)uIndex == -1) || (uIndex > infoPtr->uNumBands))
	    uIndex = infoPtr->uNumBands;

	/* pre insert copy */
	if (uIndex > 0) {
	    memcpy (&infoPtr->bands[0], &oldBands[0],
		    uIndex * sizeof(REBAR_BAND));
	}

	/* post copy */
	if (uIndex <= infoPtr->uNumBands - 1) {
	    memcpy (&infoPtr->bands[uIndex+1], &oldBands[uIndex],
		    (infoPtr->uNumBands - uIndex) * sizeof(REBAR_BAND));
	}

	Free (oldBands);
    }

    infoPtr->uNumBands++;

    TRACE("index %u!\n", uIndex);

    /* initialize band (infoPtr->bands[uIndex])*/
    lpBand = &infoPtr->bands[uIndex];
    lpBand->fMask = 0;
    lpBand->fStatus = 0;
    lpBand->clrFore = infoPtr->clrText;
    lpBand->clrBack = infoPtr->clrBk;
    lpBand->hwndChild = 0;
    lpBand->hwndPrevParent = 0;

    REBAR_CommonSetupBand (infoPtr->hwndSelf, (LPREBARBANDINFOA)lprbbi, lpBand);
    lpBand->lpText = NULL;
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
	INT len = lstrlenW (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPWSTR)Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (lpBand->lpText, lprbbi->lpText);
	}
    }

    REBAR_ValidateBand (infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (infoPtr, &infoPtr->bands[uIndex ? 0 : 1]);

    REBAR_DumpBand (infoPtr);

    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);
    InvalidateRect(infoPtr->hwndSelf, 0, 1);

    return TRUE;
}


static LRESULT
REBAR_MaximizeBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    UINT uBand = (UINT) wParam;

    /* Validate */
    if ((infoPtr->uNumBands == 0) ||
	((INT)uBand < 0) || (uBand >= infoPtr->uNumBands)) {
	/* error !!! */
	ERR("Illegal MaximizeBand, requested=%d, current band count=%d\n",
	      (INT)uBand, infoPtr->uNumBands);
      	return FALSE;
    }

    lpBand = &infoPtr->bands[uBand];

    if (lParam && (lpBand->fMask & RBBIM_IDEALSIZE)) {
	/* handle setting ideal size */
	lpBand->ccx = lpBand->cxIdeal;
    }
    else {
	/* handle setting to max */
	FIXME("(uBand = %u fIdeal = %s) case not coded\n",
	      (UINT)wParam, lParam ? "TRUE" : "FALSE");
	return FALSE;
    }

    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
    REBAR_Layout (infoPtr, 0, TRUE, TRUE);
    InvalidateRect (infoPtr->hwndSelf, 0, TRUE);

    return TRUE;

}


static LRESULT
REBAR_MinimizeBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *band, *lpBand;
    UINT uBand = (UINT) wParam;
    RECT newrect;
    INT imindBand, imaxdBand, iprevBand, startBand, endBand;
    INT movement, i;

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

    if (infoPtr->dwStyle & CCS_VERT)
	movement = lpBand->rcBand.bottom - lpBand->rcBand.top -
	    lpBand->cxHeader;
    else
	movement = lpBand->rcBand.right - lpBand->rcBand.left -
	    lpBand->cxHeader;
    if (movement < 0) {
	ERR("something is wrong, band=(%ld,%ld)-(%ld,%ld), cxheader=%d\n",
	    lpBand->rcBand.left, lpBand->rcBand.top,
	    lpBand->rcBand.right, lpBand->rcBand.bottom,
	    lpBand->cxHeader);
	return FALSE;
    }

    imindBand = -1;
    imaxdBand = -1;
    iprevBand = -1; /* to suppress warning message */

    /* find the first band in row of the one whose is being minimized */
    for (i=0; i<infoPtr->uNumBands; i++) {
        band = &infoPtr->bands[i];
	if (HIDDENBAND(band)) continue;
	if (band->iRow == lpBand->iRow) {
	    imaxdBand = i;
	    if (imindBand == -1) imindBand = i;
	}
    }

    /* if the selected band is first in row then need to expand */
    /* next visible band                                        */
    if (imindBand == uBand) {
	band = NULL;
	movement = -movement;
	/* find the first visible band to the right of the selected band */
	for (i=uBand+1; i<=imaxdBand; i++) {
	    band = &infoPtr->bands[i];
	    if (!HIDDENBAND(band)) {
		iprevBand = i;
		LEADJ(band, movement);
		band->ccx = rcBw(band);
		break;
	    }
	}
	/* what case is this */
	if (iprevBand == -1) {
	    ERR("no previous visible band\n");
	    return FALSE;
	}
	startBand = uBand;
	endBand = iprevBand;
	SetRect (&newrect,
		 lpBand->rcBand.left,
		 lpBand->rcBand.top,
		 band->rcBand.right,
		 band->rcBand.bottom);
    }
    /* otherwise expand previous visible band                   */
    else {
	band = NULL;
	/* find the first visible band to the left of the selected band */
	for (i=uBand-1; i>=imindBand; i--) {
	    band = &infoPtr->bands[i];
	    if (!HIDDENBAND(band)) {
		iprevBand = i;
		READJ(band, movement);
		band->ccx = rcBw(band);
		break;
	    }
	}
	/* what case is this */
	if (iprevBand == -1) {
	    ERR("no previous visible band\n");
	    return FALSE;
	}
	startBand = iprevBand;
	endBand = uBand;
	SetRect (&newrect,
		 band->rcBand.left,
		 band->rcBand.top,
		 lpBand->rcBand.right,
		 lpBand->rcBand.bottom);
    }

    REBAR_Shrink (infoPtr, lpBand, movement, uBand);

    /* recompute all rectangles */
    if (infoPtr->dwStyle & CCS_VERT) {
	REBAR_CalcVertBand (infoPtr, startBand, endBand+1,
			    FALSE);
    }
    else {
	REBAR_CalcHorzBand (infoPtr, startBand, endBand+1,
			    FALSE);
    }

    TRACE("bands after minimize, see band # %d, %d\n",
	  startBand, endBand);
    REBAR_DumpBand (infoPtr);

    REBAR_MoveChildWindows (infoPtr, startBand, endBand+1);

    InvalidateRect (infoPtr->hwndSelf, &newrect, TRUE);
    UpdateWindow (infoPtr->hwndSelf);
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

    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
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
REBAR_SetBandInfoA (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
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

        if (lprbbi->lpText)
        {
            INT len;
            len = MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, NULL, 0 );
            if (len > 1)
                wstr = (LPWSTR)Alloc (len*sizeof(WCHAR));
            if (wstr)
                MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, wstr, len );
        }
        if (REBAR_strdifW(lpBand->lpText, wstr)) {
	    if (lpBand->lpText) {
	        Free (lpBand->lpText);
	        lpBand->lpText = NULL;
	    }
	    if (wstr) {
                lpBand->lpText = wstr;
                wstr = NULL;
	    }
            bChanged = TRUE;
        }
        if (wstr)
	    Free (wstr);
    }

    REBAR_ValidateBand (infoPtr, lpBand);

    REBAR_DumpBand (infoPtr);

    if (bChanged && (lprbbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE))) {
	  REBAR_Layout (infoPtr, NULL, TRUE, FALSE);
	  InvalidateRect(infoPtr->hwndSelf, 0, 1);
    }

    return TRUE;
}

static LRESULT
REBAR_SetBandInfoW (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;
    BOOL bChanged;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFOW_V3_SIZE)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);
    REBAR_DumpBandInfo ((LPREBARBANDINFOA)lprbbi);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    bChanged = REBAR_CommonSetupBand (infoPtr->hwndSelf, (LPREBARBANDINFOA)lprbbi, lpBand);
    if( (lprbbi->fMask & RBBIM_TEXT) && 
        REBAR_strdifW( lpBand->lpText, lprbbi->lpText ) ) {
	if (lpBand->lpText) {
	    Free (lpBand->lpText);
	    lpBand->lpText = NULL;
	}
	if (lprbbi->lpText) {
	    INT len = lstrlenW (lprbbi->lpText);
	    if (len > 0)
	    {
	        lpBand->lpText = (LPWSTR)Alloc ((len + 1)*sizeof(WCHAR));
	        strcpyW (lpBand->lpText, lprbbi->lpText);
	    }
	}
        bChanged = TRUE;
    }

    REBAR_ValidateBand (infoPtr, lpBand);

    REBAR_DumpBand (infoPtr);

    if ( bChanged && (lprbbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE)) ) {
      REBAR_Layout (infoPtr, NULL, TRUE, FALSE);
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
	TRACE("new image cx=%ld, cy=%ld\n", infoPtr->imageSize.cx,
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

    TRACE("background color 0x%06lx!\n", infoPtr->clrBk);

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

    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return clrTemp;
}


/* << REBAR_SetTooltips >> */


inline static LRESULT
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

    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);
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

    TRACE("[%ld %ld %ld %ld]\n",
	  lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

    /*  what is going on???? */
    GetWindowRect(infoPtr->hwndSelf, &t1);
    TRACE("window rect [%ld %ld %ld %ld]\n",
	  t1.left, t1.top, t1.right, t1.bottom);
    GetClientRect(infoPtr->hwndSelf, &t1);
    TRACE("client rect [%ld %ld %ld %ld]\n",
	  t1.left, t1.top, t1.right, t1.bottom);

    /* force full _Layout processing */
    TRACE("setting NEEDS_LAYOUT\n");
    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
    REBAR_Layout (infoPtr, lpRect, TRUE, FALSE);
    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);
    return TRUE;
}



static LRESULT
REBAR_Create (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTA cs = (LPCREATESTRUCTA) lParam;
    RECT wnrc1, clrc1;

    if (TRACE_ON(rebar)) {
	GetWindowRect(infoPtr->hwndSelf, &wnrc1);
	GetClientRect(infoPtr->hwndSelf, &clrc1);
	TRACE("window=(%ld,%ld)-(%ld,%ld) client=(%ld,%ld)-(%ld,%ld) cs=(%d,%d %dx%d)\n",
	      wnrc1.left, wnrc1.top, wnrc1.right, wnrc1.bottom,
	      clrc1.left, clrc1.top, clrc1.right, clrc1.bottom,
	      cs->x, cs->y, cs->cx, cs->cy);
    }

    TRACE("created!\n");
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
	    if (lpBand->lpText) {
		Free (lpBand->lpText);
		lpBand->lpText = NULL;
	    }
	    /* destroy child window */
	    DestroyWindow (lpBand->hwndChild);
	}

	/* free band array */
	Free (infoPtr->bands);
	infoPtr->bands = NULL;
    }

    DeleteObject (infoPtr->hcurArrow);
    DeleteObject (infoPtr->hcurHorz);
    DeleteObject (infoPtr->hcurVert);
    DeleteObject (infoPtr->hcurDrag);
    if(infoPtr->hDefaultFont) DeleteObject (infoPtr->hDefaultFont);
    SetWindowLongA (infoPtr->hwndSelf, 0, 0);

    /* free rebar info data */
    Free (infoPtr);
    TRACE("destroyed!\n");
    return 0;
}


static LRESULT
REBAR_EraseBkGnd (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    RECT cliprect;

    if (GetClipBox ( (HDC)wParam, &cliprect))
        return REBAR_InternalEraseBkGnd (infoPtr, wParam, lParam, &cliprect);
    return 0;
}


static LRESULT
REBAR_GetFont (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT)infoPtr->hFont;
}

static LRESULT
REBAR_PushChevron(REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if (wParam >= 0 && (UINT)wParam < infoPtr->uNumBands)
    {
        NMREBARCHEVRON nmrbc;
        REBAR_BAND *lpBand = &infoPtr->bands[wParam];

        TRACE("Pressed chevron on band %d\n", wParam);

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
    UINT iHitBand;
    POINT ptMouseDown;
    ptMouseDown.x = (INT)LOWORD(lParam);
    ptMouseDown.y = (INT)HIWORD(lParam);

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
        infoPtr->dragStart = MAKEPOINTS(lParam);
        infoPtr->dragNow = infoPtr->dragStart;
        if (infoPtr->dwStyle & CCS_VERT)
            infoPtr->ihitoffset = infoPtr->dragStart.y - (lpBand->rcBand.top+REBAR_PRE_GRIPPER);
        else
            infoPtr->ihitoffset = infoPtr->dragStart.x - (lpBand->rcBand.left+REBAR_PRE_GRIPPER);
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
    POINTS ptsmove;

    ptsmove = MAKEPOINTS(lParam);

    /* if we are currently dragging a band */
    if (infoPtr->iGrabbedBand >= 0)
    {
        REBAR_BAND *band1, *band2;
    
        if (GetCapture() != infoPtr->hwndSelf)
            ERR("We are dragging but haven't got capture?!?\n");

        band1 = &infoPtr->bands[infoPtr->iGrabbedBand-1];
        band2 = &infoPtr->bands[infoPtr->iGrabbedBand];

        /* if mouse did not move much, exit */
        if ((abs(ptsmove.x - infoPtr->dragNow.x) <= mindragx) &&
            (abs(ptsmove.y - infoPtr->dragNow.y) <= mindragy)) return 0;

        /* Test for valid drag case - must not be first band in row */
        if (infoPtr->dwStyle & CCS_VERT) {
            if ((ptsmove.x < band2->rcBand.left) ||
	      (ptsmove.x > band2->rcBand.right) ||
              ((infoPtr->iGrabbedBand > 0) && (band1->iRow != band2->iRow))) {
                FIXME("Cannot drag to other rows yet!!\n");
            }
            else {
                REBAR_HandleLRDrag (infoPtr, &ptsmove);
            }
        }
        else {
            if ((ptsmove.y < band2->rcBand.top) ||
              (ptsmove.y > band2->rcBand.bottom) ||
              ((infoPtr->iGrabbedBand > 0) && (band1->iRow != band2->iRow))) {
                FIXME("Cannot drag to other rows yet!!\n");
            }
            else {
                REBAR_HandleLRDrag (infoPtr, &ptsmove);
            }
        }
    }
    else
    {
        POINT ptMove;
        INT iHitBand;
        UINT htFlags;
        TRACKMOUSEEVENT trackinfo;

        ptMove.x = (INT)ptsmove.x;
        ptMove.y = (INT)ptsmove.y;
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


inline static LRESULT
REBAR_NCCalcSize (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if (infoPtr->dwStyle & WS_BORDER) {
	InflateRect((LPRECT)lParam, -GetSystemMetrics(SM_CXEDGE),
		    -GetSystemMetrics(SM_CYEDGE));
    }
    TRACE("new client=(%ld,%ld)-(%ld,%ld)\n",
	  ((LPRECT)lParam)->left, ((LPRECT)lParam)->top,
	  ((LPRECT)lParam)->right, ((LPRECT)lParam)->bottom);
    return 0;
}


static LRESULT
REBAR_NCCreate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTA cs = (LPCREATESTRUCTA) lParam;
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT wnrc1, clrc1;
    NONCLIENTMETRICSA ncm;
    HFONT tfont;
    INT i;

    if (infoPtr != NULL) {
	ERR("Strange info structure pointer *not* NULL\n");
	return FALSE;
    }

    if (TRACE_ON(rebar)) {
	GetWindowRect(hwnd, &wnrc1);
	GetClientRect(hwnd, &clrc1);
	TRACE("window=(%ld,%ld)-(%ld,%ld) client=(%ld,%ld)-(%ld,%ld) cs=(%d,%d %dx%d)\n",
	      wnrc1.left, wnrc1.top, wnrc1.right, wnrc1.bottom,
	      clrc1.left, clrc1.top, clrc1.right, clrc1.bottom,
	      cs->x, cs->y, cs->cx, cs->cy);
    }

    /* allocate memory for info structure */
    infoPtr = (REBAR_INFO *)Alloc (sizeof(REBAR_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

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
    infoPtr->hcurArrow = LoadCursorA (0, (LPSTR)IDC_ARROW);
    infoPtr->hcurHorz  = LoadCursorA (0, (LPSTR)IDC_SIZEWE);
    infoPtr->hcurVert  = LoadCursorA (0, (LPSTR)IDC_SIZENS);
    infoPtr->hcurDrag  = LoadCursorA (0, (LPSTR)IDC_SIZE);
    infoPtr->bUnicode = IsWindowUnicode (hwnd);
    infoPtr->fStatus = CREATE_RUNNING;
    infoPtr->hFont = GetStockObject (SYSTEM_FONT);

    /* issue WM_NOTIFYFORMAT to get unicode status of parent */
    i = SendMessageA(REBAR_GetNotifyParent (infoPtr),
		     WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);
    if ((i < NFR_ANSI) || (i > NFR_UNICODE)) {
	ERR("wrong response to WM_NOTIFYFORMAT (%d), assuming ANSI\n",
	    i);
	i = NFR_ANSI;
    }
    infoPtr->NtfUnicode = (i == NFR_UNICODE) ? 1 : 0;

    /* add necessary styles to the requested styles */
    infoPtr->dwStyle = cs->style | WS_VISIBLE | CCS_TOP;
    SetWindowLongA (hwnd, GWL_STYLE, infoPtr->dwStyle);

    /* get font handle for Caption Font */
    ncm.cbSize = sizeof(NONCLIENTMETRICSA);
    SystemParametersInfoA (SPI_GETNONCLIENTMETRICS,
			  ncm.cbSize, &ncm, 0);
    /* if the font is bold, set to normal */
    if (ncm.lfCaptionFont.lfWeight > FW_NORMAL) {
	ncm.lfCaptionFont.lfWeight = FW_NORMAL;
    }
    tfont = CreateFontIndirectA (&ncm.lfCaptionFont);
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
REBAR_NCHitTest (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    NMMOUSE nmmouse;
    POINTS shortpt;
    POINT clpt, pt;
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

    shortpt = MAKEPOINTS (lParam);
    POINTSTOPOINT(pt, shortpt);
    clpt = pt;
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
    TRACE("returning %ld, client point (%ld,%ld)\n", ret, clpt.x, clpt.y);
    return ret;
}


static LRESULT
REBAR_NCPaint (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    RECT rcWindow;
    HDC hdc;

    if (infoPtr->dwStyle & WS_MINIMIZE)
	return 0; /* Nothing to do */

    if (infoPtr->dwStyle & WS_BORDER) {

	/* adjust rectangle and draw the necessary edge */
	if (!(hdc = GetDCEx( infoPtr->hwndSelf, 0, DCX_USESTYLE | DCX_WINDOW )))
	    return 0;
	GetWindowRect (infoPtr->hwndSelf, &rcWindow);
	OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);
	TRACE("rect (%ld,%ld)-(%ld,%ld)\n",
	      rcWindow.left, rcWindow.top,
	      rcWindow.right, rcWindow.bottom);
	DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_RECT);
	ReleaseDC( infoPtr->hwndSelf, hdc );
    }

    return 0;
}


static LRESULT
REBAR_NotifyFormat (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT i;

    if (lParam == NF_REQUERY) {
	i = SendMessageA(REBAR_GetNotifyParent (infoPtr),
			 WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);
	if ((i < NFR_ANSI) || (i > NFR_UNICODE)) {
	    ERR("wrong response to WM_NOTIFYFORMAT (%d), assuming ANSI\n",
		i);
	    i = NFR_ANSI;
	}
	infoPtr->NtfUnicode = (i == NFR_UNICODE) ? 1 : 0;
	return (LRESULT)i;
    }
    return (LRESULT)((infoPtr->bUnicode) ? NFR_UNICODE : NFR_ANSI);
}


static LRESULT
REBAR_Paint (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    GetClientRect(infoPtr->hwndSelf, &rc);
    hdc = wParam==0 ? BeginPaint (infoPtr->hwndSelf, &ps) : (HDC)wParam;

    TRACE("painting (%ld,%ld)-(%ld,%ld) client (%ld,%ld)-(%ld,%ld)\n",
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
REBAR_SetCursor (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
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
    RECT rcClient;
    REBAR_BAND *lpBand;
    UINT i;

    infoPtr->hFont = (HFONT)wParam;

    /* revalidate all bands to change sizes of text in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	REBAR_ValidateBand (infoPtr, lpBand);
    }


    if (LOWORD(lParam)) {
        GetClientRect (infoPtr->hwndSelf, &rcClient);
        REBAR_Layout (infoPtr, &rcClient, FALSE, TRUE);
    }

    return 0;
}


inline static LRESULT
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
    RECT rcClient;

    /* auto resize deadlock check */
    if (infoPtr->fStatus & AUTO_RESIZE) {
	infoPtr->fStatus &= ~AUTO_RESIZE;
	TRACE("AUTO_RESIZE was set, reset, fStatus=%08x lparam=%08lx\n",
	      infoPtr->fStatus, lParam);
	return 0;
    }

    if (infoPtr->fStatus & CREATE_RUNNING) {
	/* still in CreateWindow */
	RECT rcWin;

	if ((INT)wParam != SIZE_RESTORED) {
	    ERR("WM_SIZE in create and flags=%08x, lParam=%08lx\n",
		wParam, lParam);
	}

	TRACE("still in CreateWindow\n");
	infoPtr->fStatus &= ~CREATE_RUNNING;
	GetWindowRect ( infoPtr->hwndSelf, &rcWin);
	TRACE("win rect (%ld,%ld)-(%ld,%ld)\n",
	      rcWin.left, rcWin.top, rcWin.right, rcWin.bottom);

	if ((lParam == 0) && (rcWin.right-rcWin.left == 0) &&
	    (rcWin.bottom-rcWin.top == 0)) {
	    /* native control seems to do this */
	    GetClientRect (GetParent(infoPtr->hwndSelf), &rcClient);
	    TRACE("sizing rebar, message and client zero, parent client (%ld,%ld)\n",
		  rcClient.right, rcClient.bottom);
	}
	else {
	    INT cx, cy;

	    cx = rcWin.right - rcWin.left;
	    cy = rcWin.bottom - rcWin.top;
	    if ((cx == LOWORD(lParam)) && (cy == HIWORD(lParam))) {
		return 0;
	    }

	    /* do the actual WM_SIZE request */
	    GetClientRect (infoPtr->hwndSelf, &rcClient);
	    TRACE("sizing rebar from (%ld,%ld) to (%d,%d), client (%ld,%ld)\n",
		  infoPtr->calcSize.cx, infoPtr->calcSize.cy,
		  LOWORD(lParam), HIWORD(lParam),
		  rcClient.right, rcClient.bottom);
	}
    }
    else {
	if ((INT)wParam != SIZE_RESTORED) {
	    ERR("WM_SIZE out of create and flags=%08x, lParam=%08lx\n",
		wParam, lParam);
	}

	/* Handle cases when outside of the CreateWindow process */

	GetClientRect (infoPtr->hwndSelf, &rcClient);
	if ((lParam == 0) && (rcClient.right + rcClient.bottom != 0) &&
	    (infoPtr->dwStyle & RBS_AUTOSIZE)) {
	    /* on a WM_SIZE to zero and current client not zero and AUTOSIZE */
	    /* native seems to use the current client rect for the size      */
	    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
	    TRACE("sizing rebar to client (%ld,%ld) size is zero but AUTOSIZE set\n",
		  rcClient.right, rcClient.bottom);
	}
	else {
	    TRACE("sizing rebar from (%ld,%ld) to (%d,%d), client (%ld,%ld)\n",
		  infoPtr->calcSize.cx, infoPtr->calcSize.cy,
		  LOWORD(lParam), HIWORD(lParam),
		  rcClient.right, rcClient.bottom);
	}
    }

    if (infoPtr->dwStyle & RBS_AUTOSIZE) {
	NMRBAUTOSIZE autosize;

	GetClientRect(infoPtr->hwndSelf, &autosize.rcTarget);
	autosize.fChanged = 0;  /* ??? */
	autosize.rcActual = autosize.rcTarget;  /* ??? */
	REBAR_Notify((NMHDR *) &autosize, infoPtr, RBN_AUTOSIZE);
	TRACE("RBN_AUTOSIZE client=(%ld,%ld), lp=%08lx\n",
	      autosize.rcTarget.right, autosize.rcTarget.bottom, lParam);
    }

    if ((infoPtr->calcSize.cx != rcClient.right) ||
	(infoPtr->calcSize.cy != rcClient.bottom))
	infoPtr->fStatus |= BAND_NEEDS_LAYOUT;

    REBAR_Layout (infoPtr, &rcClient, TRUE, TRUE);
    infoPtr->fStatus &= ~AUTO_RESIZE;

    return 0;
}


static LRESULT
REBAR_StyleChanged (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    STYLESTRUCT *ss = (STYLESTRUCT *)lParam;

    TRACE("current style=%08lx, styleOld=%08lx, style being set to=%08lx\n",
	  infoPtr->dwStyle, ss->styleOld, ss->styleNew);
    infoPtr->dwStyle = ss->styleNew;

    return FALSE;
}


static LRESULT
REBAR_WindowPosChanged (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    WINDOWPOS *lpwp = (WINDOWPOS *)lParam;
    LRESULT ret;
    RECT rc;

    /* Save the new origin of this window - used by _ForceResize */
    infoPtr->origin.x = lpwp->x;
    infoPtr->origin.y = lpwp->y;
    ret = DefWindowProcA(infoPtr->hwndSelf, WM_WINDOWPOSCHANGED,
			 wParam, lParam);
    GetWindowRect(infoPtr->hwndSelf, &rc);
    TRACE("hwnd %p new pos (%ld,%ld)-(%ld,%ld)\n",
	  infoPtr->hwndSelf, rc.left, rc.top, rc.right, rc.bottom);
    return ret;
}


static LRESULT WINAPI
REBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("hwnd=%p msg=%x wparam=%x lparam=%lx\n",
	  hwnd, uMsg, wParam, lParam);
    if (!infoPtr && (uMsg != WM_NCCREATE))
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
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
	    return REBAR_GetBandInfoA (infoPtr, wParam, lParam);

	case RB_GETBANDINFOW:
	    return REBAR_GetBandInfoW (infoPtr, wParam, lParam);

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
	    return REBAR_InsertBandA (infoPtr, wParam, lParam);

	case RB_INSERTBANDW:
	    return REBAR_InsertBandW (infoPtr, wParam, lParam);

	case RB_MAXIMIZEBAND:
	    return REBAR_MaximizeBand (infoPtr, wParam, lParam);

	case RB_MINIMIZEBAND:
	    return REBAR_MinimizeBand (infoPtr, wParam, lParam);

	case RB_MOVEBAND:
	    return REBAR_MoveBand (infoPtr, wParam, lParam);

	case RB_PUSHCHEVRON:
	    return REBAR_PushChevron (infoPtr, wParam, lParam);

	case RB_SETBANDINFOA:
	    return REBAR_SetBandInfoA (infoPtr, wParam, lParam);

	case RB_SETBANDINFOW:
	    return REBAR_SetBandInfoW (infoPtr, wParam, lParam);

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
	    if (infoPtr->NtfUnicode)
		return SendMessageW (REBAR_GetNotifyParent (infoPtr),
				     uMsg, wParam, lParam);
	    else
		return SendMessageA (REBAR_GetNotifyParent (infoPtr),
				     uMsg, wParam, lParam);


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

	case WM_PAINT:
	    return REBAR_Paint (infoPtr, wParam, lParam);

/*      case WM_PALETTECHANGED: supported according to ControlSpy */
/*      case WM_PRINTCLIENT:    supported according to ControlSpy */
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
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
REBAR_Register (void)
{
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)REBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(REBAR_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
#if GLATESTING
    wndClass.hbrBackground = CreateSolidBrush(RGB(0,128,0));
#endif
    wndClass.lpszClassName = REBARCLASSNAMEA;

    RegisterClassA (&wndClass);

    mindragx = GetSystemMetrics (SM_CXDRAG);
    mindragy = GetSystemMetrics (SM_CYDRAG);

}


VOID
REBAR_Unregister (void)
{
    UnregisterClassA (REBARCLASSNAMEA, NULL);
}
