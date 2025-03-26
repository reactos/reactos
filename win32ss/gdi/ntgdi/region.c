/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice licence.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 *                      1999 Alex Korobka
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/************************************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

            All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */

// X11 sources for ReactOS region processing.
//
// libX11/src/PolyReg.c
// libX11/src/Region.c
//
//

#include <win32k.h>
#include <suppress.h>

#define NDEBUG
#include <debug.h>

PREGION prgnDefault = NULL;
HRGN    hrgnDefault = NULL;

// Internal Functions

#if 1
#define COPY_RECTS(dest, src, nRects) \
  do {                                \
    PRECTL xDest = (dest);            \
    PRECTL xSrc = (src);              \
    UINT xRects = (nRects);           \
    while (xRects-- > 0) {            \
      *(xDest++) = *(xSrc++);         \
    }                                 \
  } while (0)
#else
#define COPY_RECTS(dest, src, nRects) RtlCopyMemory(dest, src, (nRects) * sizeof(RECTL))
#endif

#define EMPTY_REGION(pReg) { \
  (pReg)->rdh.nCount = 0; \
  (pReg)->rdh.rcBound.left = (pReg)->rdh.rcBound.top = 0; \
  (pReg)->rdh.rcBound.right = (pReg)->rdh.rcBound.bottom = 0; \
  (pReg)->rdh.iType = RDH_RECTANGLES; \
}

#define REGION_NOT_EMPTY(pReg) pReg->rdh.nCount

#define INRECT(r, x, y) \
      ( ( ((r).right >  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >  y)) && \
        ( ((r).top <= y)) )

/*  1 if two RECTs overlap.
 *  0 if two RECTs do not overlap.
 */
#define EXTENTCHECK(r1, r2) \
    ((r1)->right > (r2)->left && \
     (r1)->left < (r2)->right && \
     (r1)->bottom > (r2)->top && \
     (r1)->top < (r2)->bottom)

/*
 *  In scan converting polygons, we want to choose those pixels
 *  which are inside the polygon.  Thus, we add .5 to the starting
 *  x coordinate for both left and right edges.  Now we choose the
 *  first pixel which is inside the pgon for the left edge and the
 *  first pixel which is outside the pgon for the right edge.
 *  Draw the left pixel, but not the right.
 *
 *  How to add .5 to the starting x coordinate:
 *      If the edge is moving to the right, then subtract dy from the
 *  error term from the general form of the algorithm.
 *      If the edge is moving to the left, then add dy to the error term.
 *
 *  The reason for the difference between edges moving to the left
 *  and edges moving to the right is simple:  If an edge is moving
 *  to the right, then we want the algorithm to flip immediately.
 *  If it is moving to the left, then we don't want it to flip until
 *  we traverse an entire pixel.
 */
#define BRESINITPGON(dy, x1, x2, xStart, d, m, m1, incr1, incr2) { \
    int dx;      /* Local storage */ \
\
    /* \
     *  If the edge is horizontal, then it is ignored \
     *  and assumed not to be processed.  Otherwise, do this stuff. \
     */ \
    if ((dy) != 0) { \
        xStart = (x1); \
        dx = (x2) - xStart; \
        if (dx < 0) { \
            m = dx / (dy); \
            m1 = m - 1; \
            incr1 = -2 * dx + 2 * (dy) * m1; \
            incr2 = -2 * dx + 2 * (dy) * m; \
            d = 2 * m * (dy) - 2 * dx - 2 * (dy); \
        } else { \
            m = dx / (dy); \
            m1 = m + 1; \
            incr1 = 2 * dx - 2 * (dy) * m1; \
            incr2 = 2 * dx - 2 * (dy) * m; \
            d = -2 * m * (dy) + 2 * dx; \
        } \
    } \
}

#define BRESINCRPGON(d, minval, m, m1, incr1, incr2) { \
    if (m1 > 0) { \
        if (d > 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } else {\
        if (d >= 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } \
}

/*
 * This structure contains all of the information needed
 * to run the bresenham algorithm.
 * The variables may be hardcoded into the declarations
 * instead of using this structure to make use of
 * register declarations.
 */
typedef struct
{
    INT minor_axis;   /* Minor axis        */
    INT d;            /* Decision variable */
    INT m, m1;        /* Slope and slope+1 */
    INT incr1, incr2; /* Error increments */
} BRESINFO;

#define BRESINITPGONSTRUCT(dmaj, min1, min2, bres) \
    BRESINITPGON(dmaj, min1, min2, bres.minor_axis, bres.d, \
                     bres.m, bres.m1, bres.incr1, bres.incr2)

#define BRESINCRPGONSTRUCT(bres) \
        BRESINCRPGON(bres.d, bres.minor_axis, bres.m, bres.m1, bres.incr1, bres.incr2)

/*
 *     These are the data structures needed to scan
 *     convert regions.  Two different scan conversion
 *     methods are available -- the even-odd method, and
 *     the winding number method.
 *     The even-odd rule states that a point is inside
 *     the polygon if a ray drawn from that point in any
 *     direction will pass through an odd number of
 *     path segments.
 *     By the winding number rule, a point is decided
 *     to be inside the polygon if a ray drawn from that
 *     point in any direction passes through a different
 *     number of clockwise and counter-clockwise path
 *     segments.
 *
 *     These data structures are adapted somewhat from
 *     the algorithm in (Foley/Van Dam) for scan converting
 *     polygons.
 *     The basic algorithm is to start at the top (smallest y)
 *     of the polygon, stepping down to the bottom of
 *     the polygon by incrementing the y coordinate.  We
 *     keep a list of edges which the current scanline crosses,
 *     sorted by x.  This list is called the Active Edge Table (AET)
 *     As we change the y-coordinate, we update each entry in
 *     in the active edge table to reflect the edges new xcoord.
 *     This list must be sorted at each scanline in case
 *     two edges intersect.
 *     We also keep a data structure known as the Edge Table (ET),
 *     which keeps track of all the edges which the current
 *     scanline has not yet reached.  The ET is basically a
 *     list of SCANLINE_LIST structures containing a list of
 *     edges which are entered at a given scanline.  There is one
 *     SCANLINE_LIST per scanline at which an edge is entered.
 *     When we enter a new edge, we move it from the ET to the AET.
 *
 *     From the AET, we can implement the even-odd rule as in
 *     (Foley/Van Dam).
 *     The winding number rule is a little trickier.  We also
 *     keep the EDGE_TABLEEntries in the AET linked by the
 *     nextWETE (winding EDGE_TABLE_ENTRY) link.  This allows
 *     the edges to be linked just as before for updating
 *     purposes, but only uses the edges linked by the nextWETE
 *     link as edges representing spans of the polygon to
 *     drawn (as with the even-odd rule).
 */

/*
 * For the winding number rule
 */
#define CLOCKWISE          1
#define COUNTERCLOCKWISE  -1

typedef struct _EDGE_TABLE_ENTRY
{
    INT ymax;             /* ycoord at which we exit this edge. */
    BRESINFO bres;        /* Bresenham info to run the edge     */
    struct _EDGE_TABLE_ENTRY *next;       /* Next in the list     */
    struct _EDGE_TABLE_ENTRY *back;       /* For insertion sort   */
    struct _EDGE_TABLE_ENTRY *nextWETE;   /* For winding num rule */
    INT ClockWise;        /* Flag for winding number rule       */
} EDGE_TABLE_ENTRY;

typedef struct _SCANLINE_LIST
{
    INT scanline;                /* The scanline represented */
    EDGE_TABLE_ENTRY *edgelist;    /* Header node              */
    struct _SCANLINE_LIST *next;  /* Next in the list       */
} SCANLINE_LIST;

typedef struct
{
    INT ymax;                 /* ymax for the polygon     */
    INT ymin;                 /* ymin for the polygon     */
    SCANLINE_LIST scanlines;   /* Header node              */
} EDGE_TABLE;

/*
 * Here is a struct to help with storage allocation
 * so we can allocate a big chunk at a time, and then take
 * pieces from this heap when we need to.
 */
#define SLLSPERBLOCK 25

typedef struct _SCANLINE_LISTBLOCK
{
    SCANLINE_LIST SLLs[SLLSPERBLOCK];
    struct _SCANLINE_LISTBLOCK *next;
} SCANLINE_LISTBLOCK;

/*
 *     A few macros for the inner loops of the fill code where
 *     performance considerations don't allow a procedure call.
 *
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The winding number rule is in effect, so we must notify
 *     the caller when the edge has been removed so he
 *     can reorder the Winding Active Edge Table.
 */
#define EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET) { \
   if (pAET->ymax == y) {          /* Leaving this edge */ \
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      fixWAET = 1; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres); \
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}

/*
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The even-odd rule is in effect.
 */
#define EVALUATEEDGEEVENODD(pAET, pPrevAET, y) { \
   if (pAET->ymax == y) {          /* Leaving this edge */ \
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres); \
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}

/**************************************************************************
 *
 *    Poly Regions
 *
 *************************************************************************/

#define LARGE_COORDINATE  INT_MAX
#define SMALL_COORDINATE  INT_MIN

static
BOOL
REGION_bGrowBufferSize(
    _Inout_ PREGION prgn,
    _In_ UINT cRects)
{
    ULONG cjNewSize;
    PVOID pvBuffer;
    NT_ASSERT(cRects > 0);

    /* Make sure we don't overflow */
    if (cRects > MAXULONG / sizeof(RECTL))
    {
        return FALSE;
    }

    /* Calculate new buffer size */
    cjNewSize = cRects * sizeof(RECTL);

    /* Avoid allocating too often, by duplicating the old buffer size
       Note: we don't do an overflow check, since the old size will never
       get that large before running out of memory. */
    if (2 * prgn->rdh.nRgnSize > cjNewSize)
    {
        cjNewSize = 2 * prgn->rdh.nRgnSize;
    }

    /* Allocate the new buffer */
    pvBuffer = ExAllocatePoolWithTag(PagedPool, cjNewSize, TAG_REGION);
    if (pvBuffer == NULL)
    {
        return FALSE;
    }

    /* Copy the rects into the new buffer */
    COPY_RECTS(pvBuffer, prgn->Buffer, prgn->rdh.nCount);

    /* Free the old buffer */
    if (prgn->Buffer != &prgn->rdh.rcBound)
    {
        ExFreePoolWithTag(prgn->Buffer, TAG_REGION);
    }

    /* Set the new buffer */
    prgn->Buffer = pvBuffer;
    prgn->rdh.nRgnSize = cjNewSize;

    return TRUE;
}

static __inline
BOOL
REGION_bEnsureBufferSize(
    _Inout_ PREGION prgn,
    _In_ UINT cRects)
{
    /* Check if the current region size is too small */
    if (cRects > prgn->rdh.nRgnSize / sizeof(RECTL))
    {
        /* Allocate a new buffer */
        return REGION_bGrowBufferSize(prgn, cRects);
    }

    return TRUE;
}

FORCEINLINE
VOID
REGION_vAddRect(
    _Inout_ PREGION prgn,
    _In_ LONG left,
    _In_ LONG top,
    _In_ LONG right,
    _In_ LONG bottom)
{
    PRECTL prcl;
    NT_ASSERT((prgn->rdh.nCount + 1) * sizeof(RECT) <= prgn->rdh.nRgnSize);

    prcl = &prgn->Buffer[prgn->rdh.nCount];
    prcl->left = left;
    prcl->top = top;
    prcl->right = right;
    prcl->bottom = bottom;
    prgn->rdh.nCount++;
}

static __inline
BOOL
REGION_bAddRect(
    _Inout_ PREGION prgn,
    _In_ LONG left,
    _In_ LONG top,
    _In_ LONG right,
    _In_ LONG bottom)
{
    if (!REGION_bEnsureBufferSize(prgn, prgn->rdh.nCount + 1))
    {
        return FALSE;
    }

    REGION_vAddRect(prgn, left, top, right, bottom);
    return TRUE;
}

typedef BOOL (FASTCALL *overlapProcp)(PREGION, PRECT, PRECT, PRECT, PRECT, INT, INT);
typedef BOOL (FASTCALL *nonOverlapProcp)(PREGION, PRECT, PRECT, INT, INT);

// Number of points to buffer before sending them off to scanlines() :  Must be an even number
#define NUMPTSTOBUFFER 200

#define RGN_DEFAULT_RECTS    2

// Used to allocate buffers for points and link the buffers together
typedef struct _POINTBLOCK
{
    POINT pts[NUMPTSTOBUFFER];
    struct _POINTBLOCK *next;
} POINTBLOCK;

#ifndef NDEBUG
/*
 * This function is left there for debugging purposes.
 */
VOID
FASTCALL
IntDumpRegion(HRGN hRgn)
{
    PREGION Data;

    Data = REGION_LockRgn(hRgn);
    if (Data == NULL)
    {
        DbgPrint("IntDumpRegion called with invalid region!\n");
        return;
    }

    DbgPrint("IntDumpRegion(%x): %d,%d-%d,%d %d\n",
             hRgn,
             Data->rdh.rcBound.left,
             Data->rdh.rcBound.top,
             Data->rdh.rcBound.right,
             Data->rdh.rcBound.bottom,
             Data->rdh.iType);

    REGION_UnlockRgn(Data);
}
#endif /* Not NDEBUG */

INT
FASTCALL
REGION_Complexity(PREGION prgn)
{
    if (prgn == NULL)
        return NULLREGION;

    DPRINT("Region Complexity: %lu\n", prgn->rdh.nCount);
    switch (prgn->rdh.nCount)
    {
        case 0:
            return NULLREGION;
        case 1:
            return SIMPLEREGION;
        default:
            return COMPLEXREGION;
    }
}

static
BOOL
FASTCALL
REGION_CopyRegion(
    PREGION dst,
    PREGION src)
{
    /* Only copy if source and dest are not equal */
    if (dst != src)
    {
        /* Check if we need to increase our buffer */
        if (dst->rdh.nRgnSize < src->rdh.nCount * sizeof(RECT))
        {
            PRECTL temp;

            /* Allocate a new buffer */
            temp = ExAllocatePoolWithTag(PagedPool,
                                         src->rdh.nCount * sizeof(RECT),
                                         TAG_REGION);
            if (temp == NULL)
                return FALSE;

            /* Free the old buffer */
            if ((dst->Buffer != NULL) && (dst->Buffer != &dst->rdh.rcBound))
                ExFreePoolWithTag(dst->Buffer, TAG_REGION);

            /* Set the new buffer and the size */
            dst->Buffer = temp;
            dst->rdh.nRgnSize = src->rdh.nCount * sizeof(RECT);
        }

        dst->rdh.nCount = src->rdh.nCount;
        dst->rdh.rcBound.left = src->rdh.rcBound.left;
        dst->rdh.rcBound.top = src->rdh.rcBound.top;
        dst->rdh.rcBound.right = src->rdh.rcBound.right;
        dst->rdh.rcBound.bottom = src->rdh.rcBound.bottom;
        dst->rdh.iType = src->rdh.iType;
        COPY_RECTS(dst->Buffer, src->Buffer, src->rdh.nCount);
    }

    return TRUE;
}

static
VOID
FASTCALL
REGION_SetExtents(
    PREGION pReg)
{
    RECTL *pRect, *pRectEnd, *pExtents;

    /* Quick check for NULLREGION */
    if (pReg->rdh.nCount == 0)
    {
        pReg->rdh.rcBound.left = 0;
        pReg->rdh.rcBound.top = 0;
        pReg->rdh.rcBound.right = 0;
        pReg->rdh.rcBound.bottom = 0;
        pReg->rdh.iType = RDH_RECTANGLES;
        return;
    }

    pExtents = &pReg->rdh.rcBound;
    pRect = pReg->Buffer;
    pRectEnd = pReg->Buffer + pReg->rdh.nCount - 1;

    /* Since pRect is the first rectangle in the region, it must have the
     * smallest top and since pRectEnd is the last rectangle in the region,
     * it must have the largest bottom, because of banding. Initialize left and
     * right from pRect and pRectEnd, resp., as good things to initialize them
     * to... */
    pExtents->left = pRect->left;
    pExtents->top = pRect->top;
    pExtents->right = pRectEnd->right;
    pExtents->bottom = pRectEnd->bottom;

    while (pRect <= pRectEnd)
    {
        if (pRect->left < pExtents->left)
            pExtents->left = pRect->left;
        if (pRect->right > pExtents->right)
            pExtents->right = pRect->right;
        pRect++;
    }

    pReg->rdh.iType = RDH_RECTANGLES;
}

// FIXME: This function needs review and testing
/***********************************************************************
 *           REGION_CropRegion
 */
INT
FASTCALL
REGION_CropRegion(
    PREGION rgnDst,
    PREGION rgnSrc,
    const RECTL *rect)
{
    PRECTL lpr, rpr;
    ULONG i, j, clipa, clipb, nRgnSize;
    INT left = MAXLONG;
    INT right = MINLONG;
    INT top = MAXLONG;
    INT bottom = MINLONG;

    if ((rect->left >= rect->right) ||
        (rect->top >= rect->bottom) ||
        (EXTENTCHECK(rect, &rgnSrc->rdh.rcBound) == 0))
    {
        goto empty;
    }

    /* Skip all rects that are completely above our intersect rect */
    for (clipa = 0; clipa < rgnSrc->rdh.nCount; clipa++)
    {
        /* bottom is exclusive, so break when we go above it */
        if (rgnSrc->Buffer[clipa].bottom > rect->top) break;
    }

    /* Bail out, if there is nothing left */
    if (clipa == rgnSrc->rdh.nCount) goto empty;

    /* Find the last rect that is still within the intersect rect (exclusive) */
    for (clipb = clipa; clipb < rgnSrc->rdh.nCount; clipb++)
    {
        /* bottom is exclusive, so stop, when we start at that y pos */
        if (rgnSrc->Buffer[clipb].top >= rect->bottom) break;
    }

    /* Bail out, if there is nothing left */
    if (clipb == clipa) goto empty;

    // clipa - index of the first rect in the first intersecting band
    // clipb - index of the last rect in the last intersecting band plus 1

    /* Check if the buffer in the dest region is large enough,
       otherwise allocate a new one */
    nRgnSize = (clipb - clipa) * sizeof(RECT);
    if ((rgnDst != rgnSrc) && (rgnDst->rdh.nRgnSize < nRgnSize))
    {
        PRECTL temp;
        temp = ExAllocatePoolWithTag(PagedPool, nRgnSize, TAG_REGION);
        if (temp == NULL)
            return ERROR;

        /* Free the old buffer */
        if (rgnDst->Buffer && (rgnDst->Buffer != &rgnDst->rdh.rcBound))
            ExFreePoolWithTag(rgnDst->Buffer, TAG_REGION);

        rgnDst->Buffer = temp;
        rgnDst->rdh.nCount = 0;
        rgnDst->rdh.nRgnSize = nRgnSize;
        rgnDst->rdh.iType = RDH_RECTANGLES;
    }

    /* Loop all rects within the intersect rect from the y perspective */
    for (i = clipa, j = 0; i < clipb ; i++)
    {
        /* i - src index, j - dst index, j is always <= i for obvious reasons */

        lpr = &rgnSrc->Buffer[i];

        /* Make sure the source rect is not retarded */
        ASSERT(lpr->bottom > lpr->top);
        ASSERT(lpr->right > lpr->left);

        /* We already checked above, this should hold true */
        ASSERT(lpr->bottom > rect->top);
        ASSERT(lpr->top < rect->bottom);

        /* Check if this rect is really inside the intersect rect */
        if ((lpr->left < rect->right) && (lpr->right > rect->left))
        {
            rpr = &rgnDst->Buffer[j];

            /* Crop the rect with the intersect rect */
            rpr->top = max(lpr->top, rect->top);
            rpr->bottom = min(lpr->bottom, rect->bottom);
            rpr->left = max(lpr->left, rect->left);
            rpr->right = min(lpr->right, rect->right);

            /* Make sure the resulting rect is not retarded */
            ASSERT(rpr->bottom > rpr->top);
            ASSERT(rpr->right > rpr->left);

            /* Track new bounds */
            if (rpr->left < left) left = rpr->left;
            if (rpr->right > right) right = rpr->right;
            if (rpr->top < top) top = rpr->top;
            if (rpr->bottom > bottom) bottom = rpr->bottom;

            /* Next target rect */
            j++;
        }
    }

    if (j == 0) goto empty;

    /* Update the bounds rect */
    rgnDst->rdh.rcBound.left = left;
    rgnDst->rdh.rcBound.right = right;
    rgnDst->rdh.rcBound.top = top;
    rgnDst->rdh.rcBound.bottom = bottom;

    /* Set new rect count */
    rgnDst->rdh.nCount = j;

    return REGION_Complexity(rgnDst);

empty:
    if (rgnDst->Buffer == NULL)
    {
        rgnDst->Buffer = &rgnDst->rdh.rcBound;
    }

    EMPTY_REGION(rgnDst);
    return NULLREGION;
}

/*!
 *      Attempt to merge the rects in the current band with those in the
 *      previous one. Used only by REGION_RegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * \note Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their bottom fields
 *            altered.
 *          - pReg->numRects will be decreased.
 *
 */
static
INT
FASTCALL
REGION_Coalesce(
    PREGION pReg,  /* Region to coalesce */
    INT prevStart, /* Index of start of previous band */
    INT curStart)  /* Index of start of current band */
{
    RECTL *pPrevRect;  /* Current rect in previous band */
    RECTL *pCurRect;   /* Current rect in current band */
    RECTL *pRegEnd;    /* End of region */
    INT curNumRects;   /* Number of rectangles in current band */
    INT prevNumRects;  /* Number of rectangles in previous band */
    INT bandtop;       /* Top coordinate for current band */

    pRegEnd = pReg->Buffer + pReg->rdh.nCount;
    pPrevRect = pReg->Buffer + prevStart;
    prevNumRects = curStart - prevStart;

    /* Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in REGION_RegionOp
     * at the end when one region has been exhausted. */
    pCurRect = pReg->Buffer + curStart;
    bandtop = pCurRect->top;
    for (curNumRects = 0;
         (pCurRect != pRegEnd) && (pCurRect->top == bandtop);
         curNumRects++)
    {
        pCurRect++;
    }

    if (pCurRect != pRegEnd)
    {
        /* If more than one band was added, we have to find the start
         * of the last band added so the next coalescing job can start
         * at the right place... (given when multiple bands are added,
         * this may be pointless -- see above). */
        pRegEnd--;
        while ((pRegEnd-1)->top == pRegEnd->top)
        {
            pRegEnd--;
        }

        curStart = pRegEnd - pReg->Buffer;
        pRegEnd = pReg->Buffer + pReg->rdh.nCount;
    }

    if ((curNumRects == prevNumRects) && (curNumRects != 0))
    {
        pCurRect -= curNumRects;

        /* The bands may only be coalesced if the bottom of the previous
         * matches the top scanline of the current. */
        if (pPrevRect->bottom == pCurRect->top)
        {
            /* Make sure the bands have rects in the same places. This
             * assumes that rects have been added in such a way that they
             * cover the most area possible. I.e. two rects in a band must
             * have some horizontal space between them. */
            do
            {
                if ((pPrevRect->left != pCurRect->left) ||
                    (pPrevRect->right != pCurRect->right))
                {
                    /* The bands don't line up so they can't be coalesced. */
                    return (curStart);
                }

                pPrevRect++;
                pCurRect++;
                prevNumRects -= 1;
            }
            while (prevNumRects != 0);

            pReg->rdh.nCount -= curNumRects;
            pCurRect -= curNumRects;
            pPrevRect -= curNumRects;

            /* The bands may be merged, so set the bottom of each rect
             * in the previous band to that of the corresponding rect in
             * the current band. */
            do
            {
                pPrevRect->bottom = pCurRect->bottom;
                pPrevRect++;
                pCurRect++;
                curNumRects -= 1;
            }
            while (curNumRects != 0);

            /* If only one band was added to the region, we have to backup
             * curStart to the start of the previous band.
             *
             * If more than one band was added to the region, copy the
             * other bands down. The assumption here is that the other bands
             * came from the same region as the current one and no further
             * coalescing can be done on them since it's all been done
             * already... curStart is already in the right place. */
            if (pCurRect == pRegEnd)
            {
                curStart = prevStart;
            }
            else
            {
                do
                {
                    *pPrevRect++ = *pCurRect++;
                }
                while (pCurRect != pRegEnd);
            }
        }
    }

    return (curStart);
}

/*!
 *      Apply an operation to two regions. Called by REGION_Union,
 *      REGION_Inverse, REGION_Subtract, REGION_Intersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 *\note The idea behind this function is to view the two regions as sets.
 *      Together they cover a rectangle of area that this function divides
 *      into horizontal bands where points are covered only by one region
 *      or by both. For the first case, the nonOverlapFunc is called with
 *      each the band and the band's upper and lower extents. For the
 *      second, the overlapFunc is called to process the entire band. It
 *      is responsible for clipping the rectangles in the band, though
 *      this function provides the boundaries.
 *      At the end of each band, the new region is coalesced, if possible,
 *      to reduce the number of rectangles in the region.
 *
 */
static
BOOL
FASTCALL
REGION_RegionOp(
    PREGION newReg, /* Place to store result */
    PREGION reg1,   /* First region in operation */
    PREGION reg2,   /* 2nd region in operation */
    overlapProcp overlapFunc,     /* Function to call for over-lapping bands */
    nonOverlapProcp nonOverlap1Func, /* Function to call for non-overlapping bands in region 1 */
    nonOverlapProcp nonOverlap2Func)  /* Function to call for non-overlapping bands in region 2 */
{
    RECTL *r1;                         /* Pointer into first region */
    RECTL *r2;                         /* Pointer into 2d region */
    RECTL *r1End;                      /* End of 1st region */
    RECTL *r2End;                      /* End of 2d region */
    INT ybot;                          /* Bottom of intersection */
    INT ytop;                          /* Top of intersection */
    RECTL *oldRects;                   /* Old rects for newReg */
    ULONG prevBand;                    /* Index of start of
                                        * Previous band in newReg */
    ULONG curBand;                     /* Index of start of current band in newReg */
    RECTL *r1BandEnd;                  /* End of current band in r1 */
    RECTL *r2BandEnd;                  /* End of current band in r2 */
    ULONG top;                         /* Top of non-overlapping band */
    ULONG bot;                         /* Bottom of non-overlapping band */

    /* Initialization:
     *  set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use. */
    r1 = reg1->Buffer;
    r2 = reg2->Buffer;
    r1End = r1 + reg1->rdh.nCount;
    r2End = r2 + reg2->rdh.nCount;

    /* newReg may be one of the src regions so we can't empty it. We keep a
     * note of its rects pointer (so that we can free them later), preserve its
     * extents and simply set numRects to zero. */
    oldRects = newReg->Buffer;
    newReg->rdh.nCount = 0;

    /* Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually. */
    newReg->rdh.nRgnSize = max(reg1->rdh.nCount + 1, reg2->rdh.nCount) * 2 * sizeof(RECT);

    newReg->Buffer = ExAllocatePoolWithTag(PagedPool,
                                           newReg->rdh.nRgnSize,
                                           TAG_REGION);
    if (newReg->Buffer == NULL)
    {
        newReg->rdh.nRgnSize = 0;
        return FALSE;
    }

    /* Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms. */
    if (reg1->rdh.rcBound.top < reg2->rdh.rcBound.top)
        ybot = reg1->rdh.rcBound.top;
    else
        ybot = reg2->rdh.rcBound.top;

    /* prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles. */
    prevBand = 0;
    do
    {
        curBand = newReg->rdh.nCount;

        /* This algorithm proceeds one source-band (as opposed to a
         * destination band, which is determined by where the two regions
         * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
         * rectangle after the last one in the current band for their
         * respective regions. */
        r1BandEnd = r1;
        while ((r1BandEnd != r1End) && (r1BandEnd->top == r1->top))
        {
            r1BandEnd++;
        }

        r2BandEnd = r2;
        while ((r2BandEnd != r2End) && (r2BandEnd->top == r2->top))
        {
            r2BandEnd++;
        }

        /* First handle the band that doesn't intersect, if any.
         *
         * Note that attention is restricted to one band in the
         * non-intersecting region at once, so if a region has n
         * bands between the current position and the next place it overlaps
         * the other, this entire loop will be passed through n times. */
        if (r1->top < r2->top)
        {
            top = max(r1->top,ybot);
            bot = min(r1->bottom,r2->top);

            if ((top != bot) && (nonOverlap1Func != NULL))
            {
                if (!(*nonOverlap1Func)(newReg, r1, r1BandEnd, top, bot)) return FALSE;
            }

            ytop = r2->top;
        }
        else if (r2->top < r1->top)
        {
            top = max(r2->top,ybot);
            bot = min(r2->bottom,r1->top);

            if ((top != bot) && (nonOverlap2Func != NULL))
            {
                if (!(*nonOverlap2Func)(newReg, r2, r2BandEnd, top, bot) ) return FALSE;
            }

            ytop = r1->top;
        }
        else
        {
            ytop = r1->top;
        }

        /* If any rectangles got added to the region, try and coalesce them
         * with rectangles from the previous band. Note we could just do
         * this test in miCoalesce, but some machines incur a not
         * inconsiderable cost for function calls, so... */
        if (newReg->rdh.nCount != curBand)
        {
            prevBand = REGION_Coalesce(newReg, prevBand, curBand);
        }

        /* Now see if we've hit an intersecting band. The two bands only
         * intersect if ybot > ytop */
        ybot = min(r1->bottom, r2->bottom);
        curBand = newReg->rdh.nCount;
        if (ybot > ytop)
        {
            if (!(*overlapFunc)(newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot)) return FALSE;
        }

        if (newReg->rdh.nCount != curBand)
        {
            prevBand = REGION_Coalesce(newReg, prevBand, curBand);
        }

        /* If we've finished with a band (bottom == ybot) we skip forward
         * in the region to the next band. */
        if (r1->bottom == ybot)
        {
            r1 = r1BandEnd;
        }
        if (r2->bottom == ybot)
        {
            r2 = r2BandEnd;
        }
    }
    while ((r1 != r1End) && (r2 != r2End));

    /* Deal with whichever region still has rectangles left. */
    curBand = newReg->rdh.nCount;
    if (r1 != r1End)
    {
        if (nonOverlap1Func != NULL)
        {
            do
            {
                r1BandEnd = r1;
                while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top))
                {
                    r1BandEnd++;
                }

                if (!(*nonOverlap1Func)(newReg,
                                   r1,
                                   r1BandEnd,
                                   max(r1->top,ybot),
                                   r1->bottom))
                    return FALSE;
                r1 = r1BandEnd;
            }
            while (r1 != r1End);
        }
    }
    else if ((r2 != r2End) && (nonOverlap2Func != NULL))
    {
        do
        {
            r2BandEnd = r2;
            while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top))
            {
                r2BandEnd++;
            }

            if (!(*nonOverlap2Func)(newReg,
                               r2,
                               r2BandEnd,
                               max(r2->top,ybot),
                               r2->bottom))
                return FALSE;
            r2 = r2BandEnd;
        }
        while (r2 != r2End);
    }

    if (newReg->rdh.nCount != curBand)
    {
        (VOID)REGION_Coalesce(newReg, prevBand, curBand);
    }

    /* A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...). */
    if ((newReg->rdh.nRgnSize > (2 * newReg->rdh.nCount * sizeof(RECT))) &&
        (newReg->rdh.nCount > 2))
    {
        if (REGION_NOT_EMPTY(newReg))
        {
            RECTL *prev_rects = newReg->Buffer;
            newReg->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                   newReg->rdh.nCount * sizeof(RECT),
                                                   TAG_REGION);

            if (newReg->Buffer == NULL)
            {
                newReg->Buffer = prev_rects;
            }
            else
            {
                newReg->rdh.nRgnSize = newReg->rdh.nCount*sizeof(RECT);
                COPY_RECTS(newReg->Buffer, prev_rects, newReg->rdh.nCount);
                if (prev_rects != &newReg->rdh.rcBound)
                    ExFreePoolWithTag(prev_rects, TAG_REGION);
            }
        }
        else
        {
            /* No point in doing the extra work involved in an Xrealloc if
             * the region is empty */
            newReg->rdh.nRgnSize = sizeof(RECT);
            if (newReg->Buffer != &newReg->rdh.rcBound)
                ExFreePoolWithTag(newReg->Buffer, TAG_REGION);

            newReg->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                   sizeof(RECT),
                                                   TAG_REGION);
            ASSERT(newReg->Buffer);
        }
    }

    newReg->rdh.iType = RDH_RECTANGLES;

    if (oldRects != &newReg->rdh.rcBound)
        ExFreePoolWithTag(oldRects, TAG_REGION);
    return TRUE;
}

/***********************************************************************
 *          Region Intersection
 ***********************************************************************/


/*!
 * Handle an overlapping band for REGION_Intersect.
 *
 * Results:
 *      None.
 *
 * \note Side Effects:
 *      Rectangles may be added to the region.
 *
 */
static
BOOL
FASTCALL
REGION_IntersectO(
    PREGION pReg,
    PRECTL  r1,
    PRECTL  r1End,
    PRECTL  r2,
    PRECTL  r2End,
    INT     top,
    INT     bottom)
{
    INT left, right;

    while ((r1 != r1End) && (r2 != r2End))
    {
        left = max(r1->left, r2->left);
        right = min(r1->right, r2->right);

        /* If there's any overlap between the two rectangles, add that
         * overlap to the new region.
         * There's no need to check for subsumption because the only way
         * such a need could arise is if some region has two rectangles
         * right next to each other. Since that should never happen... */
        if (left < right)
        {
            if (!REGION_bAddRect(pReg, left, top, right, bottom))
            {
                return FALSE;
            }
        }

        /* Need to advance the pointers. Shift the one that extends
         * to the right the least, since the other still has a chance to
         * overlap with that region's next rectangle, if you see what I mean. */
        if (r1->right < r2->right)
        {
            r1++;
        }
        else if (r2->right < r1->right)
        {
            r2++;
        }
        else
        {
            r1++;
            r2++;
        }
    }

    return TRUE;
}

/***********************************************************************
 * REGION_IntersectRegion
 */
static
BOOL
FASTCALL
REGION_IntersectRegion(
    PREGION newReg,
    PREGION reg1,
    PREGION reg2)
{
    /* Check for trivial reject */
    if ((reg1->rdh.nCount == 0) ||
        (reg2->rdh.nCount == 0) ||
        (EXTENTCHECK(&reg1->rdh.rcBound, &reg2->rdh.rcBound) == 0))
    {
        newReg->rdh.nCount = 0;
    }
    else
    {
        if (!REGION_RegionOp(newReg,
                        reg1,
                        reg2,
                        REGION_IntersectO,
                        NULL,
                        NULL))
            return FALSE;
    }

    /* Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles. */
    REGION_SetExtents(newReg);
    return TRUE;
}

/***********************************************************************
 * Region Union
 ***********************************************************************/

/*!
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * \note Side Effects:
 *      pReg->numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 */
static
BOOL
FASTCALL
REGION_UnionNonO(
    PREGION pReg,
    PRECTL  r,
    PRECTL  rEnd,
    INT     top,
    INT     bottom)
{
    if (r != rEnd)
    {
        if (!REGION_bEnsureBufferSize(pReg, pReg->rdh.nCount + (rEnd - r)))
        {
            return FALSE;
        }

        do
        {
            REGION_vAddRect(pReg, r->left, top, r->right, bottom);
            r++;
        }
        while (r != rEnd);
    }

    return TRUE;
}

static __inline
BOOL
REGION_bMergeRect(
    _Inout_ PREGION prgn,
    _In_ LONG left,
    _In_ LONG top,
    _In_ LONG right,
    _In_ LONG bottom)
{
    if ((prgn->rdh.nCount != 0) &&
        (prgn->Buffer[prgn->rdh.nCount - 1].top == top) &&
        (prgn->Buffer[prgn->rdh.nCount - 1].bottom == bottom) &&
        (prgn->Buffer[prgn->rdh.nCount - 1].right >= left))
    {
        if (prgn->Buffer[prgn->rdh.nCount - 1].right < right)
        {
            prgn->Buffer[prgn->rdh.nCount - 1].right = right;
        }
    }
    else
    {
        if (!REGION_bAddRect(prgn, left, top, right, bottom))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*!
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * \note Side Effects:
 *      Rectangles are overwritten in pReg->rects and pReg->numRects will
 *      be changed.
 *
 */
static
BOOL
FASTCALL
REGION_UnionO (
    PREGION pReg,
    PRECTL  r1,
    PRECTL  r1End,
    PRECTL  r2,
    PRECTL  r2End,
    INT     top,
    INT     bottom)
{
    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r1->left < r2->left)
        {
            if (!REGION_bMergeRect(pReg, r1->left, top, r1->right, bottom)) return FALSE;
            r1++;
        }
        else
        {
            if (!REGION_bMergeRect(pReg, r2->left, top, r2->right, bottom)) return FALSE;
            r2++;
        }
    }

    if (r1 != r1End)
    {
        do
        {
            if (!REGION_bMergeRect(pReg, r1->left, top, r1->right, bottom)) return FALSE;
            r1++;
        }
        while (r1 != r1End);
    }
    else
    {
        while (r2 != r2End)
        {
            if (!REGION_bMergeRect(pReg, r2->left, top, r2->right, bottom)) return FALSE;
            r2++;
        }
    }

    return TRUE;
}

/***********************************************************************
 * REGION_UnionRegion
 */
static
BOOL
FASTCALL
REGION_UnionRegion(
    PREGION newReg,
    PREGION reg1,
    PREGION reg2)
{
    BOOL ret = TRUE;

    /* Checks all the simple cases
     * Region 1 and 2 are the same or region 1 is empty */
    if ((reg1 == reg2) || (reg1->rdh.nCount == 0) ||
        (reg1->rdh.rcBound.right <= reg1->rdh.rcBound.left) ||
        (reg1->rdh.rcBound.bottom <= reg1->rdh.rcBound.top))
    {
        if (newReg != reg2)
        {
            ret = REGION_CopyRegion(newReg, reg2);
        }

        return ret;
    }

    /* If nothing to union (region 2 empty) */
    if ((reg2->rdh.nCount == 0) ||
        (reg2->rdh.rcBound.right <= reg2->rdh.rcBound.left) ||
        (reg2->rdh.rcBound.bottom <= reg2->rdh.rcBound.top))
    {
        if (newReg != reg1)
        {
            ret = REGION_CopyRegion(newReg, reg1);
        }

        return ret;
    }

    /* Region 1 completely subsumes region 2 */
    if ((reg1->rdh.nCount == 1) &&
        (reg1->rdh.rcBound.left <= reg2->rdh.rcBound.left) &&
        (reg1->rdh.rcBound.top <= reg2->rdh.rcBound.top) &&
        (reg2->rdh.rcBound.right <= reg1->rdh.rcBound.right) &&
        (reg2->rdh.rcBound.bottom <= reg1->rdh.rcBound.bottom))
    {
        if (newReg != reg1)
        {
            ret = REGION_CopyRegion(newReg, reg1);
        }

        return ret;
    }

    /* Region 2 completely subsumes region 1 */
    if ((reg2->rdh.nCount == 1) &&
        (reg2->rdh.rcBound.left <= reg1->rdh.rcBound.left) &&
        (reg2->rdh.rcBound.top <= reg1->rdh.rcBound.top) &&
        (reg1->rdh.rcBound.right <= reg2->rdh.rcBound.right) &&
        (reg1->rdh.rcBound.bottom <= reg2->rdh.rcBound.bottom))
    {
        if (newReg != reg2)
        {
            ret = REGION_CopyRegion(newReg, reg2);
        }

        return ret;
    }

    if ((ret = REGION_RegionOp(newReg,
                    reg1,
                    reg2,
                    REGION_UnionO,
                    REGION_UnionNonO,
                    REGION_UnionNonO)))
    {
    newReg->rdh.rcBound.left = min(reg1->rdh.rcBound.left, reg2->rdh.rcBound.left);
    newReg->rdh.rcBound.top = min(reg1->rdh.rcBound.top, reg2->rdh.rcBound.top);
    newReg->rdh.rcBound.right = max(reg1->rdh.rcBound.right, reg2->rdh.rcBound.right);
    newReg->rdh.rcBound.bottom = max(reg1->rdh.rcBound.bottom, reg2->rdh.rcBound.bottom);
    }
    return ret;
}

/***********************************************************************
 * Region Subtraction
 ***********************************************************************/

/*!
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * \note Side Effects:
 *      pReg may be affected.
 *
 */
static
BOOL
FASTCALL
REGION_SubtractNonO1(
    PREGION pReg,
    PRECTL  r,
    PRECTL  rEnd,
    INT     top,
    INT     bottom)
{
    if (r != rEnd)
    {
        if (!REGION_bEnsureBufferSize(pReg, pReg->rdh.nCount + (rEnd - r)))
        {
            return FALSE;
        }

        do
        {
            REGION_vAddRect(pReg, r->left, top, r->right, bottom);
            r++;
        }
        while (r != rEnd);
    }

    return TRUE;
}

/*!
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * \note Side Effects:
 *      pReg may have rectangles added to it.
 *
 */
static
BOOL
FASTCALL
REGION_SubtractO(
    PREGION pReg,
    PRECTL  r1,
    PRECTL  r1End,
    PRECTL  r2,
    PRECTL  r2End,
    INT     top,
    INT     bottom)
{
    INT left;

    left = r1->left;

    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r2->right <= left)
        {
            /* Subtrahend missed the boat: go to next subtrahend. */
            r2++;
        }
        else if (r2->left <= left)
        {
            /* Subtrahend preceeds minuend: nuke left edge of minuend. */
            left = r2->right;
            if (left >= r1->right)
            {
                /* Minuend completely covered: advance to next minuend and
                 * reset left fence to edge of new minuend. */
                r1++;
                if (r1 != r1End)
                    left = r1->left;
            }
            else
            {
                /* Subtrahend now used up since it doesn't extend beyond
                 * minuend */
                r2++;
            }
        }
        else if (r2->left < r1->right)
        {
            /* Left part of subtrahend covers part of minuend: add uncovered
             * part of minuend to region and skip to next subtrahend. */
            if (!REGION_bAddRect(pReg, left, top, r2->left, bottom))
            {
                return FALSE;
            }

            left = r2->right;
            if (left >= r1->right)
            {
                /* Minuend used up: advance to new... */
                r1++;
                if (r1 != r1End)
                    left = r1->left;
            }
            else
            {
                /* Subtrahend used up */
                r2++;
            }
        }
        else
        {
            /* Minuend used up: add any remaining piece before advancing. */
            if (r1->right > left)
            {
                if (!REGION_bAddRect(pReg, left, top, r1->right, bottom))
                {
                    return FALSE;
                }
            }

            r1++;
            if (r1 != r1End)
                left = r1->left;
        }
    }

    /* Make sure the buffer is large enough for all remaining operations */
    if (r1 != r1End)
    {
        if (!REGION_bEnsureBufferSize(pReg, pReg->rdh.nCount + (r1End - r1)))
        {
            return FALSE;
        }

        /* Add remaining minuend rectangles to region. */
        do
        {
            REGION_vAddRect(pReg, left, top, r1->right, bottom);
            r1++;
            if (r1 != r1End)
            {
                left = r1->left;
            }
        }
        while (r1 != r1End);
    }

    return TRUE;
}

/*!
 *      Subtract regS from regM and leave the result in regD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *      TRUE.
 *
 * \note Side Effects:
 *      regD is overwritten.
 *
 */
static
BOOL
FASTCALL
REGION_SubtractRegion(
    PREGION regD,
    PREGION regM,
    PREGION regS)
{
    /* Check for trivial reject */
    if ((regM->rdh.nCount == 0) ||
        (regS->rdh.nCount == 0) ||
        (EXTENTCHECK(&regM->rdh.rcBound, &regS->rdh.rcBound) == 0))
    {
        return REGION_CopyRegion(regD, regM);
    }

    if (!REGION_RegionOp(regD,
                    regM,
                    regS,
                    REGION_SubtractO,
                    REGION_SubtractNonO1,
                    NULL))
        return FALSE;

    /* Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles. */
    REGION_SetExtents(regD);
    return TRUE;
}

/***********************************************************************
 * REGION_XorRegion
 */
static
BOOL
FASTCALL
REGION_XorRegion(
    PREGION dr,
    PREGION sra,
    PREGION srb)
{
    HRGN htra, htrb;
    PREGION tra, trb;
    BOOL ret;

    // FIXME: Don't use a handle
    tra = REGION_AllocRgnWithHandle(sra->rdh.nCount + 1);
    if (tra == NULL)
    {
        return FALSE;
    }
    htra = tra->BaseObject.hHmgr;

    // FIXME: Don't use a handle
    trb = REGION_AllocRgnWithHandle(srb->rdh.nCount + 1);
    if (trb == NULL)
    {
        REGION_UnlockRgn(tra);
        GreDeleteObject(htra);
        return FALSE;
    }
    htrb = trb->BaseObject.hHmgr;

    ret = REGION_SubtractRegion(tra, sra, srb) &&
          REGION_SubtractRegion(trb, srb, sra) &&
          REGION_UnionRegion(dr, tra, trb);
    REGION_UnlockRgn(tra);
    REGION_UnlockRgn(trb);

    GreDeleteObject(htra);
    GreDeleteObject(htrb);
    return ret;
}

/*!
 * Adds a rectangle to a REGION
 */
BOOL
FASTCALL
REGION_UnionRectWithRgn(
    PREGION rgn,
    const RECTL *rect)
{
    REGION region;

    region.Buffer = &region.rdh.rcBound;
    region.rdh.nCount = 1;
    region.rdh.nRgnSize = sizeof(RECT);
    region.rdh.rcBound = *rect;
    return REGION_UnionRegion(rgn, rgn, &region);
}

INT
FASTCALL
REGION_SubtractRectFromRgn(
    PREGION prgnDest,
    PREGION prgnSrc,
    const RECTL *prcl)
{
    REGION rgnLocal;

    rgnLocal.Buffer = &rgnLocal.rdh.rcBound;
    rgnLocal.rdh.nCount = 1;
    rgnLocal.rdh.nRgnSize = sizeof(RECT);
    rgnLocal.rdh.rcBound = *prcl;
    REGION_SubtractRegion(prgnDest, prgnSrc, &rgnLocal);
    return REGION_Complexity(prgnDest);
}

BOOL
FASTCALL
REGION_bCopy(
    PREGION dst,
    PREGION src)
{
    if ( !dst || !src ) return FALSE;
    return REGION_CopyRegion( dst, src);
}

BOOL
FASTCALL
REGION_bIntersectRegion(
    PREGION newReg,
    PREGION reg1,
    PREGION reg2)
{
    if ( !newReg || !reg1 || !reg2 ) return FALSE;
    return REGION_IntersectRegion( newReg, reg1, reg2);
}

static
BOOL
REGION_bMakeSimpleFrameRgn(
    _Inout_ PREGION prgn,
    _In_ PRECTL prclSrc,
    _In_ INT cx,
    _In_ INT cy)
{
    RECTL arcl[4];
    UINT i;

    NT_ASSERT((cx >= 0) && (cy >= 0));
    NT_ASSERT((prclSrc->bottom > prclSrc->top) &&
              (prclSrc->right > prclSrc->left));

    /* Start with an empty region */
    EMPTY_REGION(prgn);

    /* Check for the case where the frame covers the whole rect */
    if (((prclSrc->bottom - prclSrc->top) <= cy * 2) ||
        ((prclSrc->right - prclSrc->left) <= cx * 2))
    {
        prgn->rdh.rcBound = *prclSrc;
        prgn->Buffer[0] = *prclSrc;
        prgn->rdh.nCount = 1;
        return TRUE;
    }

    i = 0;

    if (cy != 0)
    {
        /* Top rectangle */
        arcl[i].left = prclSrc->left;
        arcl[i].top = prclSrc->top;
        arcl[i].right = prclSrc->right;
        arcl[i].bottom = prclSrc->top + cy;
        i++;
    }

    if (cx != 0)
    {
        /* Left rectangle */
        arcl[i].left = prclSrc->left;
        arcl[i].top = prclSrc->top + cy;
        arcl[i].right = prclSrc->left + cx;
        arcl[i].bottom = prclSrc->bottom - cy;
        i++;

        /* Right rectangle */
        arcl[i].left = prclSrc->right - cx;
        arcl[i].top = prclSrc->top + cy;
        arcl[i].right = prclSrc->right;
        arcl[i].bottom = prclSrc->bottom - cy;
        i++;
    }

    if (cy != 0)
    {
        /* Bottom rectangle */
        arcl[i].left = prclSrc->left;
        arcl[i].top = prclSrc->bottom - cy;
        arcl[i].right = prclSrc->right;
        arcl[i].bottom = prclSrc->bottom;
        i++;
    }

    if (i != 0)
    {
        /* The frame results in a complex region. rcBounds remains
           the same, though. */
        prgn->rdh.nCount = i;
        NT_ASSERT(prgn->rdh.nCount > 1);
        prgn->rdh.nRgnSize = prgn->rdh.nCount * sizeof(RECT);
        NT_ASSERT(prgn->Buffer == &prgn->rdh.rcBound);
        prgn->Buffer = ExAllocatePoolWithTag(PagedPool,
                                            prgn->rdh.nRgnSize,
                                            TAG_REGION);
        if (prgn->Buffer == NULL)
        {
            prgn->rdh.nRgnSize = 0;
            return FALSE;
        }

        _PRAGMA_WARNING_SUPPRESS(__WARNING_MAYBE_UNINIT_VAR) // arcl is initialized
        COPY_RECTS(prgn->Buffer, arcl, prgn->rdh.nCount);
    }

    return TRUE;
}

static
BOOL
REGION_bMakeFrameRegion(
    _Inout_ PREGION prgnDest,
    _Inout_ PREGION prgnSrc,
    _In_ INT cx,
    _In_ INT cy)
{
    /* Handle negative cx / cy */
    cx = abs(cx);
    cy = abs(cy);

    /* Check border size (the cast is necessary to catch cx/cy == INT_MIN!) */
    if (((UINT)cx > MAX_COORD) || ((UINT)cy > MAX_COORD))
    {
        return FALSE;
    }

    /* Fail on empty source region */
    if (!REGION_NOT_EMPTY(prgnSrc))
    {
        return FALSE;
    }

    /* Handle trivial case */
    if ((cx == 0) && (cy == 0))
    {
        EMPTY_REGION(prgnDest);
        return TRUE;
    }

    /* Handle simple source region */
    if (REGION_Complexity(prgnSrc) == SIMPLEREGION)
    {
        return REGION_bMakeSimpleFrameRgn(prgnDest, &prgnSrc->rdh.rcBound, cx, cy);
    }

    /* Check if we can move the region to create the frame region */
    if ((prgnSrc->rdh.rcBound.left < (MIN_COORD + cx)) ||
        (prgnSrc->rdh.rcBound.top < (MIN_COORD + cy)) ||
        (prgnSrc->rdh.rcBound.right > (MAX_COORD - cx)) ||
        (prgnSrc->rdh.rcBound.bottom > (MAX_COORD - cy)))
    {
        return FALSE;
    }

    /* Copy the source region */
    if (!REGION_CopyRegion(prgnDest, prgnSrc))
    {
        return FALSE;
    }

    /* Move the source region to the bottom-right */
    NT_VERIFY(REGION_bOffsetRgn(prgnSrc, cx, cy));

    /* Intersect with the source region (this crops the top-left frame) */
    REGION_IntersectRegion(prgnDest, prgnDest, prgnSrc);

    /* Move the source region to the bottom-left */
    NT_VERIFY(REGION_bOffsetRgn(prgnSrc, -2 * cx, 0));

    /* Intersect with the source region (this crops the top-right frame) */
    REGION_IntersectRegion(prgnDest, prgnDest, prgnSrc);

    /* Move the source region to the top-left */
    NT_VERIFY(REGION_bOffsetRgn(prgnSrc, 0, -2 * cy));

    /* Intersect with the source region (this crops the bottom-right frame) */
    REGION_IntersectRegion(prgnDest, prgnDest, prgnSrc);

    /* Move the source region to the top-right  */
    NT_VERIFY(REGION_bOffsetRgn(prgnSrc, 2 * cx, 0));

    /* Intersect with the source region (this crops the bottom-left frame) */
    REGION_IntersectRegion(prgnDest, prgnDest, prgnSrc);

    /* Move the source region back to the original position */
    NT_VERIFY(REGION_bOffsetRgn(prgnSrc, -cx, cy));

    /* Finally subtract the cropped region from the source */
    REGION_SubtractRegion(prgnDest, prgnSrc, prgnDest);

    return TRUE;
}

HRGN
FASTCALL
GreCreateFrameRgn(
    HRGN hrgn,
    INT cx,
    INT cy)
{
    PREGION prgnFrame, prgnSrc;
    HRGN hrgnFrame;

    /* Allocate a new region */
    prgnFrame = REGION_AllocUserRgnWithHandle(1);
    if (prgnFrame == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Lock the source region */
    prgnSrc = REGION_LockRgn(hrgn);
    if (prgnSrc == NULL)
    {
        REGION_Delete(prgnFrame);
        return FALSE;
    }

    if (REGION_bMakeFrameRegion(prgnFrame, prgnSrc, cx, cy))
    {
        hrgnFrame = prgnFrame->BaseObject.hHmgr;
        REGION_UnlockRgn(prgnFrame);
    }
    else
    {
        REGION_Delete(prgnFrame);
        hrgnFrame = NULL;
    }

    REGION_UnlockRgn(prgnSrc);
    return hrgnFrame;
}

BOOL
FASTCALL
REGION_bXformRgn(
    _Inout_ PREGION prgn,
    _In_ PMATRIX pmx)
{
    XFORMOBJ xo;
    ULONG i, cjSize;
    PPOINT ppt;
    PULONG pcPoints;
    RECT rect;
    BOOL bResult;

    /* Check for zero rectangles and return TRUE for translation only matrices */
    if (prgn->rdh.nCount < 1)
        return (pmx->flAccel & XFORM_UNITY) != 0;

    /* Check if this is a scaling only matrix (off-diagonal elements are 0 */
    if (pmx->flAccel & XFORM_SCALE)
    {
        /* Check if this is a translation only matrix */
        if (pmx->flAccel & XFORM_UNITY)
        {
            /* Just offset the region */
            return REGION_bOffsetRgn(prgn, (pmx->fxDx + 8) / 16, (pmx->fxDy + 8) / 16);
        }
        else
        {
            /* Initialize the xform object */
            XFORMOBJ_vInit(&xo, pmx);

            /* Scaling can move the rects out of the coordinate space, so
             * we first need to check whether we can apply the transformation
             * on the bounds rect without modifying the region */
            if (!XFORMOBJ_bApplyXform(&xo, XF_LTOL, 2, &prgn->rdh.rcBound, &rect))
            {
                return FALSE;
            }

            /* Apply the xform to the rects in the region */
            if (!XFORMOBJ_bApplyXform(&xo,
                                      XF_LTOL,
                                      prgn->rdh.nCount * 2,
                                      prgn->Buffer,
                                      prgn->Buffer))
            {
                /* This can not happen, since we already checked the bounds! */
                NT_ASSERT(FALSE);
            }

            /* Reset bounds */
            RECTL_vSetEmptyRect(&prgn->rdh.rcBound);

            /* Loop all rects in the region */
            for (i = 0; i < prgn->rdh.nCount; i++)
            {
                /* Make sure the rect is well-ordered after the xform */
                RECTL_vMakeWellOrdered(&prgn->Buffer[i]);

                /* Update bounds */
                if (!RECTL_bUnionRect(&prgn->rdh.rcBound,
                                 &prgn->rdh.rcBound,
                                 &prgn->Buffer[i]))
                {
                    DPRINT1("NULL Set in Union Rects\n");
                    return FALSE;
                }
            }

            /* Loop all rects in the region */
            for (i = 0; i < prgn->rdh.nCount - 1; i++)
            {
                NT_ASSERT(prgn->Buffer[i].top <= prgn->Buffer[i].bottom);
                NT_ASSERT(prgn->Buffer[i + 1].top >= prgn->Buffer[i].top);
            }

            return TRUE;
        }
    }
    else
    {
        /* Allocate a buffer for the polygons */
        cjSize = prgn->rdh.nCount * (4 * sizeof(POINT) + sizeof(ULONG));
        ppt = ExAllocatePoolWithTag(PagedPool, cjSize, GDITAG_REGION);
        if (ppt == NULL)
        {
            return FALSE;
        }

        /* Fill the buffer with the rects */
        pcPoints = (PULONG)&ppt[4 * prgn->rdh.nCount];
        for (i = 0; i < prgn->rdh.nCount; i++)
        {
            /* Make sure the rect is within the legal range */
            pcPoints[i] = 4;
            ppt[4 * i + 0].x = prgn->Buffer[i].left;
            ppt[4 * i + 0].y = prgn->Buffer[i].top;
            ppt[4 * i + 1].x = prgn->Buffer[i].right;
            ppt[4 * i + 1].y = prgn->Buffer[i].top;
            ppt[4 * i + 2].x = prgn->Buffer[i].right;
            ppt[4 * i + 2].y = prgn->Buffer[i].bottom;
            ppt[4 * i + 3].x = prgn->Buffer[i].left;
            ppt[4 * i + 3].y = prgn->Buffer[i].bottom;
        }

        /* Initialize the xform object */
        XFORMOBJ_vInit(&xo, pmx);

        /* Apply the xform to the rects in the buffer */
        if (!XFORMOBJ_bApplyXform(&xo,
                                  XF_LTOL,
                                  prgn->rdh.nCount * 2,
                                  ppt,
                                  ppt))
        {
            /* This means, there were coordinates that would go outside of
               the coordinate space after the transformation */
            ExFreePoolWithTag(ppt, GDITAG_REGION);
            return FALSE;
        }

        /* Now use the polygons to create a polygon region */
        bResult = REGION_SetPolyPolygonRgn(prgn,
                                           ppt,
                                           pcPoints,
                                           prgn->rdh.nCount,
                                           WINDING);

        /* Free the polygon buffer */
        ExFreePoolWithTag(ppt, GDITAG_REGION);

        return bResult;
    }
}

PREGION
FASTCALL
REGION_AllocRgnWithHandle(
    INT nReg)
{
    //HRGN hReg;
    PREGION pReg;

    pReg = (PREGION)GDIOBJ_AllocateObject(GDIObjType_RGN_TYPE,
                                          sizeof(REGION),
                                          BASEFLAG_LOOKASIDE);
    if (pReg == NULL)
    {
        DPRINT1("Could not allocate a palette.\n");
        return NULL;
    }

    //hReg = pReg->BaseObject.hHmgr;

    if ((nReg == 0) || (nReg == 1))
    {
        /* Testing shows that > 95% of all regions have only 1 rect.
           Including that here saves us from having to do another allocation */
        pReg->Buffer = &pReg->rdh.rcBound;
    }
    else
    {
        pReg->Buffer = ExAllocatePoolWithTag(PagedPool,
                                             nReg * sizeof(RECT),
                                             TAG_REGION);
        if (pReg->Buffer == NULL)
        {
            DPRINT1("Could not allocate region buffer\n");
            GDIOBJ_vDeleteObject(&pReg->BaseObject);
            return NULL;
        }
    }

    EMPTY_REGION(pReg);
    pReg->rdh.dwSize = sizeof(RGNDATAHEADER);
    pReg->rdh.nCount = nReg;
    pReg->rdh.nRgnSize = nReg * sizeof(RECT);
    pReg->prgnattr = &pReg->rgnattr;

    /* Initialize the region attribute */
    pReg->rgnattr.AttrFlags = 0;
    pReg->rgnattr.iComplexity = SIMPLEREGION;
    pReg->rgnattr.Rect = pReg->rdh.rcBound;

    /* Finally insert the region into the handle table */
    if (!GDIOBJ_hInsertObject(&pReg->BaseObject, GDI_OBJ_HMGR_POWNED))
    {
        DPRINT1("Could not insert palette into handle table.\n");
        GDIOBJ_vFreeObject(&pReg->BaseObject);
        return NULL;
    }

    return pReg;
}

BOOL
NTAPI
REGION_bAllocRgnAttr(
    PREGION prgn)
{
    PPROCESSINFO ppi;
    PRGN_ATTR prgnattr;

    NT_ASSERT(prgn->prgnattr == &prgn->rgnattr);

    ppi = PsGetCurrentProcessWin32Process();
    ASSERT(ppi);

    prgnattr = GdiPoolAllocate(ppi->pPoolRgnAttr);
    if (prgnattr == NULL)
    {
        DPRINT1("Could not allocate RGN attr\n");
        return FALSE;
    }

    /* Copy the current region attribute */
    *prgnattr = prgn->rgnattr;

    /* Set the object attribute in the handle table */
    prgn->prgnattr = prgnattr;
    GDIOBJ_vSetObjectAttr(&prgn->BaseObject, prgnattr);

    return TRUE;
}

//
// Allocate User Space Region Handle.
//
PREGION
FASTCALL
REGION_AllocUserRgnWithHandle(
    INT nRgn)
{
    PREGION prgn;

    prgn = REGION_AllocRgnWithHandle(nRgn);
    if (prgn == NULL)
    {
        return NULL;
    }

    if (!REGION_bAllocRgnAttr(prgn))
    {
        ASSERT(FALSE);
    }

    return prgn;
}

static
VOID
REGION_vSyncRegion(
    _In_ PREGION prgn)
{
    PRGN_ATTR prgnattr;

    NT_ASSERT(prgn != NULL);
    NT_ASSERT(prgn->prgnattr != NULL);
    NT_ASSERT((prgn->prgnattr == &prgn->rgnattr) ||
              (prgn->prgnattr->AttrFlags & ATTR_RGN_VALID));

    /* Get the region attribute and check if it's dirty (modified) */
    prgnattr = prgn->prgnattr;
    if (prgnattr->AttrFlags & ATTR_RGN_DIRTY)
    {
        NT_ASSERT(GreGetObjectOwner(prgn->BaseObject.hHmgr) == GDI_OBJ_HMGR_POWNED);
        NT_ASSERT(prgnattr != &prgn->rgnattr);

        if (prgnattr->iComplexity == NULLREGION)
        {
            EMPTY_REGION(prgn);
        }
        else if (prgnattr->iComplexity == SIMPLEREGION)
        {
            REGION_SetRectRgn(prgn,
                              prgnattr->Rect.left,
                              prgnattr->Rect.top,
                              prgnattr->Rect.right,
                              prgnattr->Rect.bottom);
        }
        else
        {
            /* Should not happen, region attribute is corrupted! */
            DPRINT1("Region attribute is corrupted, ignoring\n");
            NT_ASSERT(FALSE);
        }
    }

    /* Reset the flags */
    prgnattr->AttrFlags &= ~(ATTR_RGN_DIRTY | ATTR_RGN_VALID);
}

PREGION
FASTCALL
REGION_LockRgn(
    _In_ HRGN hrgn)
{
    PREGION prgn;

    prgn = GDIOBJ_LockObject(hrgn, GDIObjType_RGN_TYPE);
    if (prgn == NULL)
        return NULL;

    REGION_vSyncRegion(prgn);
    return prgn;
}

VOID
FASTCALL
REGION_UnlockRgn(
    _In_ PREGION prgn)
{
    PRGN_ATTR prgnattr;

    NT_ASSERT(prgn != NULL);
    NT_ASSERT(prgn->prgnattr != NULL);

    /* Get the region attribute and check if it's user mode */
    prgnattr = prgn->prgnattr;
    if (prgnattr != &prgn->rgnattr)
    {
        NT_ASSERT(GreGetObjectOwner(prgn->BaseObject.hHmgr) == GDI_OBJ_HMGR_POWNED);
        prgnattr->iComplexity = REGION_Complexity(prgn);
        prgnattr->Rect.left   = prgn->rdh.rcBound.left;
        prgnattr->Rect.top    = prgn->rdh.rcBound.top;
        prgnattr->Rect.right  = prgn->rdh.rcBound.right;
        prgnattr->Rect.bottom = prgn->rdh.rcBound.bottom;
        prgnattr->AttrFlags |= ATTR_RGN_VALID;
    }

    GDIOBJ_vUnlockObject(&prgn->BaseObject);
}

/*
  System Regions:
    These regions do not use attribute sections and when allocated, use gdiobj
    level functions.
*/
//
// System Region Functions
//
PREGION
FASTCALL
IntSysCreateRectpRgn(
    INT LeftRect,
    INT TopRect,
    INT RightRect,
    INT BottomRect)
{
    PREGION prgn;

    /* Allocate a region, without a handle */
    prgn = (PREGION)GDIOBJ_AllocateObject(GDIObjType_RGN_TYPE, sizeof(REGION), BASEFLAG_LOOKASIDE);
    if (prgn == NULL)
    {
        return NULL;
    }

    /* Initialize it */
    prgn->Buffer = &prgn->rdh.rcBound;
    prgn->prgnattr = &prgn->rgnattr;
    prgn->prgnattr->AttrFlags = ATTR_RGN_VALID;
    REGION_SetRectRgn(prgn, LeftRect, TopRect, RightRect, BottomRect);

    return prgn;
}

VOID
NTAPI
REGION_vCleanup(PVOID ObjectBody)
{
    PREGION pRgn = (PREGION)ObjectBody;
    PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
    ASSERT(ppi);

    ASSERT(pRgn->prgnattr);
    if (pRgn->prgnattr != &pRgn->rgnattr)
        GdiPoolFree(ppi->pPoolRgnAttr, pRgn->prgnattr);

    if (pRgn->Buffer && pRgn->Buffer != &pRgn->rdh.rcBound)
        ExFreePoolWithTag(pRgn->Buffer, TAG_REGION);
}

VOID
FASTCALL
REGION_Delete(PREGION pRgn)
{
    if (pRgn == prgnDefault)
        return;

    GDIOBJ_vDeleteObject(&pRgn->BaseObject);
}

BOOL
FASTCALL
IntGdiSetRegionOwner(HRGN hRgn, DWORD OwnerMask)
{
    PREGION prgn;
    PRGN_ATTR prgnattr;
    PPROCESSINFO ppi;

    prgn = REGION_LockRgn(hRgn);
    if (prgn == NULL)
    {
        return FALSE;
    }

    prgnattr = prgn->prgnattr;
    if (prgnattr != &prgn->rgnattr)
    {
        GDIOBJ_vSetObjectAttr(&prgn->BaseObject, NULL);
        prgn->prgnattr = &prgn->rgnattr;
        ppi = PsGetCurrentProcessWin32Process();
        GdiPoolFree(ppi->pPoolRgnAttr, prgnattr);
    }

    REGION_UnlockRgn(prgn);

    return GreSetObjectOwner(hRgn, OwnerMask);
}

INT
FASTCALL
IntGdiCombineRgn(
    PREGION prgnDest,
    PREGION prgnSrc1,
    PREGION prgnSrc2,
    INT iCombineMode)
{
    BOOL Ret = TRUE;

    if (prgnDest == NULL)
    {
        DPRINT("IntGdiCombineRgn: hDest unavailable\n");
        return ERROR;
    }

    if (prgnSrc1 == NULL)
    {
        DPRINT("IntGdiCombineRgn: hSrc1 unavailable\n");
        return ERROR;
    }

    if (iCombineMode == RGN_COPY)
    {
        if (!REGION_CopyRegion(prgnDest, prgnSrc1))
            return ERROR;

        return REGION_Complexity(prgnDest);
    }

    if (prgnSrc2 == NULL)
    {
        DPRINT1("IntGdiCombineRgn requires hSrc2 != NULL for combine mode %d!\n", iCombineMode);
        ASSERT(FALSE);
        return ERROR;
    }

    switch (iCombineMode)
    {
        case RGN_AND:
            Ret = REGION_IntersectRegion(prgnDest, prgnSrc1, prgnSrc2);
            break;
        case RGN_OR:
            Ret = REGION_UnionRegion(prgnDest, prgnSrc1, prgnSrc2);
            break;
        case RGN_XOR:
            Ret = REGION_XorRegion(prgnDest, prgnSrc1, prgnSrc2);
            break;
        case RGN_DIFF:
            Ret = REGION_SubtractRegion(prgnDest, prgnSrc1, prgnSrc2);
            break;
    }

    return Ret ? REGION_Complexity(prgnDest) : ERROR;
}

INT
FASTCALL
REGION_GetRgnBox(
    PREGION Rgn,
    PRECTL pRect)
{
    DWORD ret;

    if (Rgn != NULL)
    {
        *pRect = Rgn->rdh.rcBound;
        ret = REGION_Complexity(Rgn);

        return ret;
    }
    return 0; // If invalid region return zero
}

INT
APIENTRY
IntGdiGetRgnBox(
    HRGN hRgn,
    PRECTL pRect)
{
    PREGION Rgn;
    DWORD ret;

    Rgn = REGION_LockRgn(hRgn);
    if (Rgn == NULL)
    {
        return ERROR;
    }

    ret = REGION_GetRgnBox(Rgn, pRect);
    REGION_UnlockRgn(Rgn);

    return ret;
}

BOOL
FASTCALL
REGION_PtInRegion(
    PREGION prgn,
    INT X,
    INT Y)
{
    ULONG i;
    PRECT r;

    if (prgn->rdh.nCount > 0 && INRECT(prgn->rdh.rcBound, X, Y))
    {
        r =  prgn->Buffer;
        for (i = 0; i < prgn->rdh.nCount; i++)
        {
            if (INRECT(r[i], X, Y))
                return TRUE;
        }
    }

    return FALSE;
}

BOOL
FASTCALL
REGION_RectInRegion(
    PREGION Rgn,
    const RECTL *rect)
{
    PRECTL pCurRect, pRectEnd;
    RECT rc;

    /* Swap the coordinates to make right >= left and bottom >= top */
    /* (region building rectangles are normalized the same way) */
    if (rect->top > rect->bottom)
    {
        rc.top = rect->bottom;
        rc.bottom = rect->top;
    }
    else
    {
        rc.top = rect->top;
        rc.bottom = rect->bottom;
    }

    if (rect->right < rect->left)
    {
        rc.right = rect->left;
        rc.left = rect->right;
    }
    else
    {
        rc.right = rect->right;
        rc.left = rect->left;
    }

    /* This is (just) a useful optimization */
    if ((Rgn->rdh.nCount > 0) && EXTENTCHECK(&Rgn->rdh.rcBound, &rc))
    {
        for (pCurRect = Rgn->Buffer, pRectEnd = pCurRect +
                                                Rgn->rdh.nCount; pCurRect < pRectEnd; pCurRect++)
        {
            if (pCurRect->bottom <= rc.top)
                continue;             /* Not far enough down yet */

            if (pCurRect->top >= rc.bottom)
                break;                /* Too far down */

            if (pCurRect->right <= rc.left)
                continue;              /* Not far enough over yet */

            if (pCurRect->left >= rc.right)
            {
                continue;
            }

            return TRUE;
        }
    }

    return FALSE;
}

VOID
FASTCALL
REGION_SetRectRgn(
    PREGION rgn,
    INT LeftRect,
    INT TopRect,
    INT RightRect,
    INT BottomRect)
{
    PRECTL firstRect;

    if (LeftRect > RightRect)
    {
        INT tmp = LeftRect;
        LeftRect = RightRect;
        RightRect = tmp;
    }

    if (TopRect > BottomRect)
    {
        INT tmp = TopRect;
        TopRect = BottomRect;
        BottomRect = tmp;
    }

    if ((LeftRect != RightRect) && (TopRect != BottomRect))
    {
        firstRect = rgn->Buffer;
        ASSERT(firstRect);
        firstRect->left = rgn->rdh.rcBound.left = LeftRect;
        firstRect->top = rgn->rdh.rcBound.top = TopRect;
        firstRect->right = rgn->rdh.rcBound.right = RightRect;
        firstRect->bottom = rgn->rdh.rcBound.bottom = BottomRect;
        rgn->rdh.nCount = 1;
        rgn->rdh.iType = RDH_RECTANGLES;
    }
    else
    {
        EMPTY_REGION(rgn);
    }
}

BOOL
FASTCALL
REGION_bOffsetRgn(
    _Inout_ PREGION prgn,
    _In_ INT cx,
    _In_ INT cy)
{
    PRECTL prcl;
    UINT i;

    NT_ASSERT(prgn != NULL);

    /* Check for trivial case */
    if ((cx == 0) && (cy == 0))
    {
        return TRUE;
    }

    /* Check for empty regions, we ignore the offset values here */
    if (prgn->rdh.nCount == 0)
    {
        return TRUE;
    }

    /* Make sure the offset is within the legal range */
    if ((cx > MAX_COORD) || (cx < MIN_COORD) ||
        (cy > MAX_COORD) || (cy < MIN_COORD))
    {
        return FALSE;
    }

    /* Are we moving right? */
    if (cx > 0)
    {
        /* Check if we stay inside the bounds on the right side */
        if (prgn->rdh.rcBound.right > (MAX_COORD - cx))
        {
            return FALSE;
        }
    }
    else
    {
        /* Check if we stay inside the bounds on the left side */
        if (prgn->rdh.rcBound.left < (MIN_COORD - cx))
        {
            return FALSE;
        }
    }

    /* Are we moving down? */
    if (cy > 0)
    {
        /* Check if we stay inside the bounds on the right side */
        if (prgn->rdh.rcBound.bottom > (MAX_COORD - cy))
        {
            return FALSE;
        }
    }
    else
    {
        /* Check if we stay inside the bounds on the left side */
        if (prgn->rdh.rcBound.top < (MIN_COORD - cy))
        {
            return FALSE;
        }
    }

    /* Loop to move the rects */
    prcl = prgn->Buffer;
    for (i = 0; i < prgn->rdh.nCount; i++)
    {
        prcl[i].left += cx;
        prcl[i].right += cx;
        prcl[i].top += cy;
        prcl[i].bottom += cy;
    }

    /* Finally update the bounds rect */
    if (prgn->Buffer != &prgn->rdh.rcBound)
    {
        prgn->rdh.rcBound.left += cx;
        prgn->rdh.rcBound.right += cx;
        prgn->rdh.rcBound.top += cy;
        prgn->rdh.rcBound.bottom += cy;
    }

    return TRUE;
}

/***********************************************************************
 *     REGION_InsertEdgeInET
 *
 *     Insert the given edge into the edge table.
 *     First we must find the correct bucket in the
 *     Edge table, then find the right slot in the
 *     bucket.  Finally, we can insert it.
 *
 */
static
VOID
FASTCALL
REGION_InsertEdgeInET(
    EDGE_TABLE *ET,
    EDGE_TABLE_ENTRY *ETE,
    INT scanline,
    SCANLINE_LISTBLOCK **SLLBlock,
    INT *iSLLBlock)
{
    EDGE_TABLE_ENTRY *start, *prev;
    SCANLINE_LIST *pSLL, *pPrevSLL;
    SCANLINE_LISTBLOCK *tmpSLLBlock;

    /* Find the right bucket to put the edge into */
    pPrevSLL = &ET->scanlines;
    pSLL = pPrevSLL->next;
    while (pSLL && (pSLL->scanline < scanline))
    {
        pPrevSLL = pSLL;
        pSLL = pSLL->next;
    }

    /* Reassign pSLL (pointer to SCANLINE_LIST) if necessary */
    if ((!pSLL) || (pSLL->scanline > scanline))
    {
        if (*iSLLBlock > SLLSPERBLOCK-1)
        {
            tmpSLLBlock = ExAllocatePoolWithTag(PagedPool,
                                                sizeof(SCANLINE_LISTBLOCK),
                                                TAG_REGION);
            if (tmpSLLBlock == NULL)
            {
                DPRINT1("REGION_InsertEdgeInETL(): Can't alloc SLLB\n");
                /* FIXME: Free resources? */
                return;
            }

            (*SLLBlock)->next = tmpSLLBlock;
            tmpSLLBlock->next = (SCANLINE_LISTBLOCK *)NULL;
            *SLLBlock = tmpSLLBlock;
            *iSLLBlock = 0;
        }

        pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

        pSLL->next = pPrevSLL->next;
        pSLL->edgelist = (EDGE_TABLE_ENTRY *)NULL;
        pPrevSLL->next = pSLL;
    }

    pSLL->scanline = scanline;

    /* Now insert the edge in the right bucket */
    prev = (EDGE_TABLE_ENTRY *)NULL;
    start = pSLL->edgelist;
    while (start && (start->bres.minor_axis < ETE->bres.minor_axis))
    {
        prev = start;
        start = start->next;
    }

    ETE->next = start;

    if (prev)
        prev->next = ETE;
    else
        pSLL->edgelist = ETE;
}

/***********************************************************************
 *     REGION_loadAET
 *
 *     This routine moves EDGE_TABLEEntries from the
 *     EDGE_TABLE into the Active Edge Table,
 *     leaving them sorted by smaller x coordinate.
 *
 */
static
VOID
FASTCALL
REGION_loadAET(
    EDGE_TABLE_ENTRY *AET,
    EDGE_TABLE_ENTRY *ETEs)
{
    EDGE_TABLE_ENTRY *pPrevAET;
    EDGE_TABLE_ENTRY *tmp;

    pPrevAET = AET;
    AET = AET->next;
    while (ETEs)
    {
        while (AET && (AET->bres.minor_axis < ETEs->bres.minor_axis))
        {
            pPrevAET = AET;
            AET = AET->next;
        }

        tmp = ETEs->next;
        ETEs->next = AET;
        if (AET)
            AET->back = ETEs;

        ETEs->back = pPrevAET;
        pPrevAET->next = ETEs;
        pPrevAET = ETEs;

        ETEs = tmp;
    }
}

/***********************************************************************
 *     REGION_computeWAET
 *
 *     This routine links the AET by the
 *     nextWETE (winding EDGE_TABLE_ENTRY) link for
 *     use by the winding number rule.  The final
 *     Active Edge Table (AET) might look something
 *     like:
 *
 *     AET
 *     ----------  ---------   ---------
 *     |ymax    |  |ymax    |  |ymax    |
 *     | ...    |  |...     |  |...     |
 *     |next    |->|next    |->|next    |->...
 *     |nextWETE|  |nextWETE|  |nextWETE|
 *     ---------   ---------   ^--------
 *         |                   |       |
 *         V------------------->       V---> ...
 *
 */
static
VOID
FASTCALL
REGION_computeWAET(
    EDGE_TABLE_ENTRY *AET)
{
    register EDGE_TABLE_ENTRY *pWETE;
    register INT inside = 1;
    register INT isInside = 0;

    AET->nextWETE = (EDGE_TABLE_ENTRY *)NULL;
    pWETE = AET;
    AET = AET->next;
    while (AET)
    {
        if (AET->ClockWise)
            isInside++;
        else
            isInside--;

        if ((!inside && !isInside) ||
            ( inside &&  isInside))
        {
            pWETE->nextWETE = AET;
            pWETE = AET;
            inside = !inside;
        }
        AET = AET->next;
    }

    pWETE->nextWETE = (EDGE_TABLE_ENTRY *)NULL;
}

/***********************************************************************
 *     REGION_InsertionSort
 *
 *     Just a simple insertion sort using
 *     pointers and back pointers to sort the Active
 *     Edge Table.
 *
 */
static
BOOL
FASTCALL
REGION_InsertionSort(
    EDGE_TABLE_ENTRY *AET)
{
    EDGE_TABLE_ENTRY *pETEchase;
    EDGE_TABLE_ENTRY *pETEinsert;
    EDGE_TABLE_ENTRY *pETEchaseBackTMP;
    BOOL changed = FALSE;

    AET = AET->next;
    while (AET)
    {
        pETEinsert = AET;
        pETEchase = AET;
        while (pETEchase->back->bres.minor_axis > AET->bres.minor_axis)
            pETEchase = pETEchase->back;

        AET = AET->next;
        if (pETEchase != pETEinsert)
        {
            pETEchaseBackTMP = pETEchase->back;
            pETEinsert->back->next = AET;
            if (AET)
                AET->back = pETEinsert->back;

            pETEinsert->next = pETEchase;
            pETEchase->back->next = pETEinsert;
            pETEchase->back = pETEinsert;
            pETEinsert->back = pETEchaseBackTMP;
            changed = TRUE;
        }
    }

    return changed;
}

/***********************************************************************
 *     REGION_FreeStorage
 *
 *     Clean up our act.
 */
static
VOID
FASTCALL
REGION_FreeStorage(
    SCANLINE_LISTBLOCK *pSLLBlock)
{
    SCANLINE_LISTBLOCK   *tmpSLLBlock;

    while (pSLLBlock)
    {
        tmpSLLBlock = pSLLBlock->next;
        ExFreePoolWithTag(pSLLBlock, TAG_REGION);
        pSLLBlock = tmpSLLBlock;
    }
}

/***********************************************************************
 *     REGION_PtsToRegion
 *
 *     Create an array of rectangles from a list of points.
 */
static
INT
FASTCALL
REGION_PtsToRegion(
    INT numFullPtBlocks,
    INT iCurPtBlock,
    POINTBLOCK *FirstPtBlock,
    PREGION reg)
{
    RECTL *rects;
    POINT *pts;
    POINTBLOCK *CurPtBlock;
    INT i;
    RECTL *extents, *temp;
    INT numRects;

    extents = &reg->rdh.rcBound;

    numRects = ((numFullPtBlocks * NUMPTSTOBUFFER) + iCurPtBlock) >> 1;

    /* Make sure, we have at least one rect */
    if (numRects == 0)
    {
        numRects = 1;
    }

    temp = ExAllocatePoolWithTag(PagedPool, numRects * sizeof(RECT), TAG_REGION);
    if (temp == NULL)
    {
        return 0;
    }

    if (reg->Buffer != NULL)
    {
        COPY_RECTS(temp, reg->Buffer, reg->rdh.nCount);
        if (reg->Buffer != &reg->rdh.rcBound)
            ExFreePoolWithTag(reg->Buffer, TAG_REGION);
    }
    reg->Buffer = temp;

    reg->rdh.nCount = numRects;
    CurPtBlock = FirstPtBlock;
    rects = reg->Buffer - 1;
    numRects = 0;
    extents->left = LARGE_COORDINATE,  extents->right = SMALL_COORDINATE;

    for ( ; numFullPtBlocks >= 0; numFullPtBlocks--)
    {
        /* The loop uses 2 points per iteration */
        i = NUMPTSTOBUFFER >> 1;
        if (numFullPtBlocks == 0)
            i = iCurPtBlock >> 1;

        for (pts = CurPtBlock->pts; i--; pts += 2)
        {
            if (pts->x == pts[1].x)
                continue;

            if ((numRects && pts->x == rects->left) &&
                (pts->y == rects->bottom) &&
                (pts[1].x == rects->right) &&
                ((numRects == 1) || (rects[-1].top != rects->top)) &&
                (i && pts[2].y > pts[1].y))
            {
                rects->bottom = pts[1].y + 1;
                continue;
            }

            numRects++;
            rects++;
            rects->left = pts->x;
            rects->top = pts->y;
            rects->right = pts[1].x;
            rects->bottom = pts[1].y + 1;

            if (rects->left < extents->left)
                extents->left = rects->left;
            if (rects->right > extents->right)
                extents->right = rects->right;
        }

        CurPtBlock = CurPtBlock->next;
    }

    if (numRects)
    {
        extents->top = reg->Buffer->top;
        extents->bottom = rects->bottom;
    }
    else
    {
        extents->left = 0;
        extents->top = 0;
        extents->right = 0;
        extents->bottom = 0;
    }

    reg->rdh.nCount = numRects;

    return(TRUE);
}

/***********************************************************************
 *     REGION_CreateETandAET
 *
 *     This routine creates the edge table for
 *     scan converting polygons.
 *     The Edge Table (ET) looks like:
 *
 *    EDGE_TABLE
 *     --------
 *    |  ymax  |        SCANLINE_LISTs
 *    |scanline|-->------------>-------------->...
 *     --------   |scanline|   |scanline|
 *                |edgelist|   |edgelist|
 *                ---------    ---------
 *                    |             |
 *                    |             |
 *                    V             V
 *              list of ETEs   list of ETEs
 *
 *     where ETE is an EDGE_TABLE_ENTRY data structure,
 *     and there is one SCANLINE_LIST per scanline at
 *     which an edge is initially entered.
 *
 */
static
VOID
FASTCALL
REGION_CreateETandAET(
    const ULONG *Count,
    INT nbpolygons,
    const POINT *pts,
    EDGE_TABLE *ET,
    EDGE_TABLE_ENTRY *AET,
    EDGE_TABLE_ENTRY *pETEs,
    SCANLINE_LISTBLOCK *pSLLBlock)
{
    const POINT *top, *bottom;
    const POINT *PrevPt, *CurrPt, *EndPt;
    INT poly, count;
    INT iSLLBlock = 0;
    INT dy;

    /* Initialize the Active Edge Table */
    AET->next = (EDGE_TABLE_ENTRY *)NULL;
    AET->back = (EDGE_TABLE_ENTRY *)NULL;
    AET->nextWETE = (EDGE_TABLE_ENTRY *)NULL;
    AET->bres.minor_axis = SMALL_COORDINATE;

    /* Initialize the Edge Table. */
    ET->scanlines.next = (SCANLINE_LIST *)NULL;
    ET->ymax = SMALL_COORDINATE;
    ET->ymin = LARGE_COORDINATE;
    pSLLBlock->next = (SCANLINE_LISTBLOCK *)NULL;

    EndPt = pts - 1;
    for (poly = 0; poly < nbpolygons; poly++)
    {
        count = Count[poly];
        EndPt += count;
        if (count < 2)
            continue;

        PrevPt = EndPt;

        /*  For each vertex in the array of points.
         *  In this loop we are dealing with two vertices at
         *  a time -- these make up one edge of the polygon. */
        while (count--)
        {
            CurrPt = pts++;

            /*  Find out which point is above and which is below. */
            if (PrevPt->y > CurrPt->y)
            {
                bottom = PrevPt, top = CurrPt;
                pETEs->ClockWise = 0;
            }
            else
            {
                bottom = CurrPt, top = PrevPt;
                pETEs->ClockWise = 1;
            }

            /* Don't add horizontal edges to the Edge table. */
            if (bottom->y != top->y)
            {
                /* -1 so we don't get last scanline */
                pETEs->ymax = bottom->y - 1;

                /*  Initialize integer edge algorithm */
                dy = bottom->y - top->y;
                BRESINITPGONSTRUCT(dy, top->x, bottom->x, pETEs->bres);

                REGION_InsertEdgeInET(ET,
                                      pETEs,
                                      top->y,
                                      &pSLLBlock,
                                      &iSLLBlock);

                if (PrevPt->y > ET->ymax)
                    ET->ymax = PrevPt->y;
                if (PrevPt->y < ET->ymin)
                    ET->ymin = PrevPt->y;
                pETEs++;
            }

            PrevPt = CurrPt;
        }
    }
}

BOOL
FASTCALL
REGION_SetPolyPolygonRgn(
    _Inout_ PREGION prgn,
    _In_ const POINT *ppt,
    _In_ const ULONG *pcPoints,
    _In_ ULONG cPolygons,
    _In_ INT iMode)
{
    EDGE_TABLE_ENTRY *pAET;               /* Active Edge Table        */
    INT y;                                /* Current scanline         */
    INT iPts = 0;                         /* Number of pts in buffer  */
    EDGE_TABLE_ENTRY *pWETE;              /* Winding Edge Table Entry */
    SCANLINE_LIST *pSLL;                  /* Current SCANLINE_LIST    */
    POINT *pts;                           /* Output buffer            */
    EDGE_TABLE_ENTRY *pPrevAET;           /* Pointer to previous AET  */
    EDGE_TABLE ET;                        /* Header node for ET       */
    EDGE_TABLE_ENTRY AET;                 /* Header node for AET      */
    EDGE_TABLE_ENTRY *pETEs;              /* EDGE_TABLEEntries pool   */
    SCANLINE_LISTBLOCK SLLBlock;          /* Header for SCANLINE_LIST */
    INT fixWAET = FALSE;
    POINTBLOCK FirstPtBlock, *curPtBlock; /* PtBlock buffers          */
    POINTBLOCK *tmpPtBlock;
    UINT numFullPtBlocks = 0;
    UINT poly, total;
    BOOL bResult = FALSE;

    /* Check if iMode is valid */
    if ((iMode != ALTERNATE) && (iMode != WINDING))
    {
        DPRINT1("Invalid iMode: %lu\n", iMode);
        return FALSE;
    }

    /* Special case a rectangle */
    if (((cPolygons == 1) && ((pcPoints[0] == 4) ||
         ((pcPoints[0] == 5) && (ppt[4].x == ppt[0].x) && (ppt[4].y == ppt[0].y)))) &&
        (((ppt[0].y == ppt[1].y) &&
          (ppt[1].x == ppt[2].x) &&
          (ppt[2].y == ppt[3].y) &&
          (ppt[3].x == ppt[0].x)) ||
         ((ppt[0].x == ppt[1].x) &&
          (ppt[1].y == ppt[2].y) &&
          (ppt[2].x == ppt[3].x) &&
          (ppt[3].y == ppt[0].y))))
    {
        REGION_SetRectRgn(prgn,
                          min(ppt[0].x, ppt[2].x),
                          min(ppt[0].y, ppt[2].y),
                          max(ppt[0].x, ppt[2].x),
                          max(ppt[0].y, ppt[2].y));
        return TRUE;
    }

    for (poly = total = 0; poly < cPolygons; poly++)
        total += pcPoints[poly];

    pETEs = ExAllocatePoolWithTag(PagedPool,
                                  sizeof(EDGE_TABLE_ENTRY) * total,
                                  TAG_REGION);
    if (pETEs == NULL)
    {
        DPRINT1("Failed to allocate %lu edge entries\n", total);
        return FALSE;
    }

    pts = FirstPtBlock.pts;
    REGION_CreateETandAET(pcPoints, cPolygons, ppt, &ET, &AET, pETEs, &SLLBlock);
    pSLL = ET.scanlines.next;
    curPtBlock = &FirstPtBlock;

    if (iMode != WINDING)
    {
        /*  For each scanline */
        for (y = ET.ymin; y < ET.ymax; y++)
        {
            /*  Add a new edge to the active edge table when we
             *  get to the next edge. */
            if (pSLL != NULL && y == pSLL->scanline)
            {
                REGION_loadAET(&AET, pSLL->edgelist);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;

            /*  For each active edge */
            while (pAET)
            {
                pts->x = pAET->bres.minor_axis,  pts->y = y;
                pts++, iPts++;

                /* Send out the buffer */
                if (iPts == NUMPTSTOBUFFER)
                {
                    tmpPtBlock = ExAllocatePoolWithTag(PagedPool,
                                                       sizeof(POINTBLOCK),
                                                       TAG_REGION);
                    if (tmpPtBlock == NULL)
                    {
                        DPRINT1("Can't alloc tmpPtBlock\n");
                        goto Cleanup;
                    }

                    curPtBlock->next = tmpPtBlock;
                    curPtBlock = tmpPtBlock;
                    pts = curPtBlock->pts;
                    numFullPtBlocks++;
                    iPts = 0;
                }

                EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
            }

            REGION_InsertionSort(&AET);
        }
    }
    else
    {
        /* For each scanline */
        for (y = ET.ymin; y < ET.ymax; y++)
        {
            /*  Add a new edge to the active edge table when we
             *  get to the next edge. */
            if (pSLL != NULL && y == pSLL->scanline)
            {
                REGION_loadAET(&AET, pSLL->edgelist);
                REGION_computeWAET(&AET);
                pSLL = pSLL->next;
            }

            pPrevAET = &AET;
            pAET = AET.next;
            pWETE = pAET;

            /* For each active edge */
            while (pAET)
            {
                /* Add to the buffer only those edges that
                 * are in the Winding active edge table. */
                if (pWETE == pAET)
                {
                    pts->x = pAET->bres.minor_axis;
                    pts->y = y;
                    pts++;
                    iPts++;

                    /* Send out the buffer */
                    if (iPts == NUMPTSTOBUFFER)
                    {
                        tmpPtBlock = ExAllocatePoolWithTag(PagedPool,
                                                           sizeof(POINTBLOCK),
                                                           TAG_REGION);
                        if (tmpPtBlock == NULL)
                        {
                            DPRINT1("Can't alloc tPB\n");
                            goto Cleanup;
                        }
                        curPtBlock->next = tmpPtBlock;
                        curPtBlock = tmpPtBlock;
                        pts = curPtBlock->pts;
                        numFullPtBlocks++;
                        iPts = 0;
                    }

                    pWETE = pWETE->nextWETE;
                }

                EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
            }

            /* Recompute the winding active edge table if
             * we just resorted or have exited an edge. */
            if (REGION_InsertionSort(&AET) || fixWAET)
            {
                REGION_computeWAET(&AET);
                fixWAET = FALSE;
            }
        }
    }

    REGION_PtsToRegion(numFullPtBlocks, iPts, &FirstPtBlock, prgn);
    bResult = TRUE;

Cleanup:
    REGION_FreeStorage(SLLBlock.next);

    for (curPtBlock = FirstPtBlock.next; numFullPtBlocks-- > 0;)
    {
        tmpPtBlock = curPtBlock->next;
        ExFreePoolWithTag(curPtBlock, TAG_REGION);
        curPtBlock = tmpPtBlock;
    }

    ExFreePoolWithTag(pETEs, TAG_REGION);
    return bResult;
}

HRGN
NTAPI
GreCreatePolyPolygonRgn(
    _In_ const POINT *ppt,
    _In_ const ULONG *pcPoints,
    _In_ ULONG cPolygons,
    _In_ INT iMode)
{
    PREGION prgn;
    HRGN hrgn;

    /* Allocate a new region */
    prgn = REGION_AllocUserRgnWithHandle(0);
    if (prgn == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Call the internal function and check for success */
    if (REGION_SetPolyPolygonRgn(prgn, ppt, pcPoints, cPolygons, iMode))
    {
        /* Success, get the handle and unlock the region */
        hrgn = prgn->BaseObject.hHmgr;
        REGION_UnlockRgn(prgn);
    }
    else
    {
        /* Failure, delete the region */
        REGION_Delete(prgn);
        hrgn = NULL;
    }

    return hrgn;
}

BOOL
FASTCALL
IntRectInRegion(
    HRGN  hRgn,
    LPRECTL rc)
{
    PREGION Rgn;
    BOOL Ret;

    Rgn = REGION_LockRgn(hRgn);
    if (Rgn == NULL)
    {
        return ERROR;
    }

    Ret = REGION_RectInRegion(Rgn, rc);
    REGION_UnlockRgn(Rgn);
    return Ret;
}

//
// NtGdi Exported Functions
//
INT
APIENTRY
NtGdiCombineRgn(
    IN HRGN hrgnDst,
    IN HRGN hrgnSrc1,
    IN HRGN hrgnSrc2,
    IN INT iMode)
{
    HRGN ahrgn[3];
    PREGION aprgn[3];
    INT iResult;

    /* Validate the combine mode */
    if ((iMode < RGN_AND) || (iMode > RGN_COPY))
    {
        return ERROR;
    }

    /* Validate that we have the required regions */
    if ((hrgnDst == NULL) ||
        (hrgnSrc1 == NULL) ||
        ((iMode != RGN_COPY) && (hrgnSrc2 == NULL)))
    {
        DPRINT1("NtGdiCombineRgn invalid parameters: %p, %p, %p, %d\n",
                hrgnDst, hrgnSrc1, hrgnSrc2, iMode);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    /* Lock all regions */
    ahrgn[0] = hrgnDst;
    ahrgn[1] = hrgnSrc1;
    ahrgn[2] = iMode != RGN_COPY ? hrgnSrc2 : NULL;
    if (!GDIOBJ_bLockMultipleObjects(3, (HGDIOBJ*)ahrgn, (PVOID*)aprgn, GDIObjType_RGN_TYPE))
    {
        DPRINT1("NtGdiCombineRgn failed to lock regions: %p, %p, %p, %d\n",
                hrgnDst, hrgnSrc1, hrgnSrc2, iMode);
        return ERROR;
    }

    /* HACK: Sync usermode attributes */
    REGION_vSyncRegion(aprgn[0]);
    if (aprgn[1] != aprgn[0])
        REGION_vSyncRegion(aprgn[1]);
    if ((aprgn[2] != NULL) && (aprgn[2] != aprgn[0]) && (aprgn[2] != aprgn[1]))
        REGION_vSyncRegion(aprgn[2]);

    /* Call the internal function */
    iResult = IntGdiCombineRgn(aprgn[0], aprgn[1], aprgn[2], iMode);

    /* Unlock and return */
    REGION_UnlockRgn(aprgn[0]);
    REGION_UnlockRgn(aprgn[1]);
    if (aprgn[2] != NULL)
        REGION_UnlockRgn(aprgn[2]);

    return iResult;
}

HRGN
APIENTRY
NtGdiCreateEllipticRgn(
    INT Left,
    INT Top,
    INT Right,
    INT Bottom)
{
    return NtGdiCreateRoundRectRgn(Left,
                                   Top,
                                   Right, Bottom,
                                   Right - Left,
                                   Bottom - Top);
}

HRGN
APIENTRY
NtGdiCreateRectRgn(
    INT LeftRect,
    INT TopRect,
    INT RightRect,
    INT BottomRect)
{
    PREGION pRgn;
    HRGN hRgn;

    /* Allocate region data structure with space for 1 RECTL */
    pRgn = REGION_AllocUserRgnWithHandle(1);
    if (pRgn == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    hRgn = pRgn->BaseObject.hHmgr;

    REGION_SetRectRgn(pRgn, LeftRect, TopRect, RightRect, BottomRect);
    REGION_UnlockRgn(pRgn);

    DPRINT("Returning %p.\n", hRgn);

    return hRgn;
}

HRGN
APIENTRY
NtGdiCreateRoundRectRgn(
    INT left,
    INT top,
    INT right,
    INT bottom,
    INT ellipse_width,
    INT ellipse_height)
{
    PREGION obj;
    HRGN hrgn;
    int a, b, i, x, y;
    INT64 asq, bsq, dx, dy, err;
    RECT *rects;

    /* Make the dimensions sensible */
    if (left > right)
    {
        INT tmp = left;
        left = right;
        right = tmp;
    }

    if (top > bottom)
    {
        INT tmp = top;
        top = bottom;
        bottom = tmp;
    }

    /* the region is for the rectangle interior, but only at right and bottom for some reason */
    right--;
    bottom--;

    ellipse_width = min( right - left, abs( ellipse_width ));
    ellipse_height = min( bottom - top, abs( ellipse_height ));

    /* Check if we can do a normal rectangle instead */

    if ((ellipse_width < 2) || (ellipse_height < 2))
        return NtGdiCreateRectRgn(left, top, right, bottom);

    obj = REGION_AllocUserRgnWithHandle( ellipse_height );
    if (obj == NULL)
        return 0;

    hrgn = obj->BaseObject.hHmgr;

    obj->rdh.rcBound.left   = left;
    obj->rdh.rcBound.top    = top;
    obj->rdh.rcBound.right  = right;
    obj->rdh.rcBound.bottom = bottom;
    rects = obj->Buffer;

    /* based on an algorithm by Alois Zingl */

    a = ellipse_width - 1;
    b = ellipse_height - 1;
    asq = (INT64)8 * a * a;
    bsq = (INT64)8 * b * b;
    dx  = (INT64)4 * b * b * (1 - a);
    dy  = (INT64)4 * a * a * (1 + (b % 2));
    err = dx + dy + a * a * (b % 2);

    x = 0;
    y = ellipse_height / 2;

    rects[y].left = left;
    rects[y].right = right;

    while (x <= ellipse_width / 2)
    {
        INT64 e2 = 2 * err;
        if (e2 >= dx)
        {
            x++;
            err += dx += bsq;
        }
        if (e2 <= dy)
        {
            y++;
            err += dy += asq;
            rects[y].left = left + x;
            rects[y].right = right - x;
        }
    }
    for (i = 0; i < ellipse_height / 2; i++)
    {
        rects[i].left = rects[b - i].left;
        rects[i].right = rects[b - i].right;
        rects[i].top = top + i;
        rects[i].bottom = rects[i].top + 1;
    }
    for (; i < ellipse_height; i++)
    {
        rects[i].top = bottom - ellipse_height + i;
        rects[i].bottom = rects[i].top + 1;
    }
    rects[ellipse_height / 2].top = top + ellipse_height / 2;  /* extend to top of rectangle */

    REGION_UnlockRgn(obj);
    return hrgn;
}

BOOL
APIENTRY
NtGdiEqualRgn(
    HRGN  hSrcRgn1,
    HRGN  hSrcRgn2)
{
    HRGN ahrgn[2];
    PREGION aprgn[2];
    PREGION rgn1, rgn2;
    PRECTL tRect1, tRect2;
    ULONG i;
    BOOL bRet = FALSE;

    /* Check if we got 2 regions */
    if ((hSrcRgn1 == NULL) || (hSrcRgn2 == NULL))
    {
        return FALSE;
    }

    /* Check if these are the same regions */
    if (hSrcRgn1 == hSrcRgn2)
    {
        /* Make sure this region is valid */
        if ((GDI_HANDLE_GET_TYPE(hSrcRgn1) == GDILoObjType_LO_REGION_TYPE) &&
            GreIsHandleValid(hSrcRgn1))
        {
            return TRUE;
        }
        return FALSE;
    }

    /* Lock both regions */
    ahrgn[0] = hSrcRgn1;
    ahrgn[1] = hSrcRgn2;
    if (!GDIOBJ_bLockMultipleObjects(2, (HGDIOBJ*)ahrgn, (PVOID*)aprgn, GDIObjType_RGN_TYPE))
    {
        DPRINT1("NtGdiEqualRgn failed to lock regions: %p, %p\n",
                hSrcRgn1, hSrcRgn2);
        return FALSE;
    }

    REGION_vSyncRegion(aprgn[0]);
    REGION_vSyncRegion(aprgn[1]);

    rgn1 = aprgn[0];
    rgn2 = aprgn[1];

    if (rgn1->rdh.nCount != rgn2->rdh.nCount)
        goto exit;

    if (rgn1->rdh.nCount == 0)
    {
        bRet = TRUE;
        goto exit;
    }

    if ((rgn1->rdh.rcBound.left   != rgn2->rdh.rcBound.left)  ||
        (rgn1->rdh.rcBound.right  != rgn2->rdh.rcBound.right) ||
        (rgn1->rdh.rcBound.top    != rgn2->rdh.rcBound.top)   ||
        (rgn1->rdh.rcBound.bottom != rgn2->rdh.rcBound.bottom))
        goto exit;

    tRect1 = rgn1->Buffer;
    tRect2 = rgn2->Buffer;

    if ((tRect1 == NULL) || (tRect2 == NULL))
        goto exit;

    for (i=0; i < rgn1->rdh.nCount; i++)
    {
        if ((tRect1[i].left   != tRect2[i].left)  ||
            (tRect1[i].right  != tRect2[i].right) ||
            (tRect1[i].top    != tRect2[i].top)   ||
            (tRect1[i].bottom != tRect2[i].bottom))
            goto exit;
    }

    bRet = TRUE;

exit:
    REGION_UnlockRgn(rgn1);
    REGION_UnlockRgn(rgn2);
    return bRet;
}

HRGN
APIENTRY
NtGdiExtCreateRegion(
    OPTIONAL LPXFORM Xform,
    DWORD Count,
    LPRGNDATA RgnData)
{
    HRGN hRgn;
    PREGION Region;
    DWORD nCount = 0;
    DWORD iType = 0;
    DWORD dwSize = 0;
    UINT i;
    RECT* rects;
    NTSTATUS Status = STATUS_SUCCESS;
    MATRIX matrix;
    XFORMOBJ xo;

    DPRINT("NtGdiExtCreateRegion\n");
    _SEH2_TRY
    {
        ProbeForRead(RgnData, Count, 1);
        nCount = RgnData->rdh.nCount;
        iType = RgnData->rdh.iType;
        dwSize = RgnData->rdh.dwSize;
        rects = (RECT*)RgnData->Buffer;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    /* Check parameters, but don't set last error here */
    if ((Count < sizeof(RGNDATAHEADER) + nCount * sizeof(RECT)) ||
        (iType != RDH_RECTANGLES) ||
        (dwSize != sizeof(RGNDATAHEADER)))
    {
        return NULL;
    }

    Region = REGION_AllocUserRgnWithHandle(nCount);

    if (Region == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    hRgn = Region->BaseObject.hHmgr;

    _SEH2_TRY
    {
        /* Insert the rectangles one by one */
        for(i=0; i<nCount; i++)
        {
            if ( rects[i].left < rects[i].right && rects[i].top < rects[i].bottom )
            {
                if (!REGION_UnionRectWithRgn(Region, &rects[i]))
                {
                   REGION_UnlockRgn(Region);
                   GreDeleteObject(hRgn);
                   hRgn = NULL;
                   _SEH2_LEAVE;
                }
            }
        }

        if (Xform != NULL)
        {
            ULONG ret;

            /* Init the XFORMOBJ from the Xform struct */
            Status = STATUS_INVALID_PARAMETER;
            XFORMOBJ_vInit(&xo, &matrix);
            ret = XFORMOBJ_iSetXform(&xo, (XFORML*)Xform);

            /* Check for error */
            if (ret != DDI_ERROR)
            {
                /* Apply the coordinate transformation on the rects */
                if (XFORMOBJ_bApplyXform(&xo,
                                         XF_LTOL,
                                         Region->rdh.nCount * 2,
                                         Region->Buffer,
                                         Region->Buffer))
                {
                    Status = STATUS_SUCCESS;
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        REGION_UnlockRgn(Region);
        GreDeleteObject(hRgn);
        return NULL;
    }

    if (hRgn) REGION_UnlockRgn(Region);

    return hRgn;
}

INT
APIENTRY
NtGdiGetRgnBox(
    HRGN hRgn,
    PRECTL pRect)
{
    PREGION Rgn;
    RECTL SafeRect;
    DWORD ret;
    NTSTATUS Status = STATUS_SUCCESS;

    Rgn = REGION_LockRgn(hRgn);
    if (Rgn == NULL)
    {
        return ERROR;
    }

    ret = REGION_GetRgnBox(Rgn, &SafeRect);
    REGION_UnlockRgn(Rgn);
    if (ret == ERROR)
    {
        return ret;
    }

    _SEH2_TRY
    {
        ProbeForWrite(pRect, sizeof(RECT), 1);
        *pRect = SafeRect;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if (!NT_SUCCESS(Status))
    {
        return ERROR;
    }

    return ret;
}

INT
APIENTRY
NtGdiOffsetRgn(
    _In_ HRGN hrgn,
    _In_ INT cx,
    _In_ INT cy)
{
    PREGION prgn;
    INT iResult;

    DPRINT("NtGdiOffsetRgn: hrgn %p cx %d cy %d\n", hrgn, cx, cy);

    /* Lock the region */
    prgn = REGION_LockRgn(hrgn);
    if (prgn == NULL)
    {
        DPRINT1("NtGdiOffsetRgn: failed to lock region %p\n", hrgn);
        return ERROR;
    }

    /* Call the internal function */
    if (!REGION_bOffsetRgn(prgn, cx, cy))
    {
        iResult = ERROR;
    }
    else
    {
        iResult = REGION_Complexity(prgn);
    }

    /* Unlock and return the result */
    REGION_UnlockRgn(prgn);
    return iResult;
}

BOOL
APIENTRY
NtGdiPtInRegion(
    _In_ HRGN hrgn,
    _In_ INT x,
    _In_ INT y)
{
    PREGION prgn;
    BOOL bResult;

    /* Lock the region */
    prgn = REGION_LockRgn(hrgn);
    if (prgn == NULL)
    {
        DPRINT1("NtGdiPtInRegion: hrgn error\n");
        return FALSE;
    }

    /* Call the internal function */
    bResult = REGION_PtInRegion(prgn, x, y);

    /* Unlock and return the result */
    REGION_UnlockRgn(prgn);
    return bResult;
}

__kernel_entry
BOOL
APIENTRY
NtGdiRectInRegion(
    _In_ HRGN hrgn,
    _Inout_ LPRECT prclUnsafe)
{
    RECTL rcTemp;

    /* Probe and copy the rect */
    _SEH2_TRY
    {
        ProbeForRead(prclUnsafe, sizeof(RECT), 1);
        rcTemp = *prclUnsafe;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("NtGdiRectInRegion: Exception accessing the rect\n");
        return FALSE;
    }
    _SEH2_END;

    /* Call the internal function */
    return IntRectInRegion(hrgn, &rcTemp);
}

BOOL
APIENTRY
NtGdiSetRectRgn(
    _In_ HRGN hrgn,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom)
{
    PREGION prgn;

    /* Lock the region */
    prgn = REGION_LockRgn(hrgn);
    if (prgn == NULL)
    {
        return FALSE;
    }

    /* Call the internal API */
    REGION_SetRectRgn(prgn, xLeft, yTop, xRight, yBottom);

    /* Unlock the region and return success */
    REGION_UnlockRgn(prgn);
    return TRUE;
}

/*!
 * MSDN: GetRegionData, Return Values:
 *
 * "If the function succeeds and dwCount specifies an adequate number of bytes,
 * the return value is always dwCount. If dwCount is too small or the function
 * fails, the return value is 0. If lpRgnData is NULL, the return value is the
 * required number of bytes.
 *
 * If the function fails, the return value is zero."
 */
_Success_(return!=0)
__kernel_entry
ULONG
APIENTRY
NtGdiGetRegionData(
    _In_ HRGN hrgn,
    _In_ ULONG cjBuffer,
    _Out_writes_bytes_to_opt_(cjBuffer, return) LPRGNDATA lpRgnData)
{
    ULONG cjRects, cjSize;
    PREGION prgn;

    /* Lock the region */
    prgn = REGION_LockRgn(hrgn);
    if (prgn == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Calculate the region sizes */
    cjRects = prgn->rdh.nCount * sizeof(RECT);
    cjSize = cjRects + sizeof(RGNDATAHEADER);

    /* Check if region data is requested */
    if (lpRgnData)
    {
        /* Check if the buffer is large enough */
        if (cjBuffer >= cjSize)
        {
            /* Probe the buffer and copy the data */
            _SEH2_TRY
            {
                ProbeForWrite(lpRgnData, cjSize, sizeof(ULONG));
                RtlCopyMemory(lpRgnData, &prgn->rdh, sizeof(RGNDATAHEADER));
                RtlCopyMemory(lpRgnData->Buffer, prgn->Buffer, cjRects);
                lpRgnData->rdh.iType = RDH_RECTANGLES;
                lpRgnData->rdh.nRgnSize = cjRects;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                cjSize = 0;
            }
            _SEH2_END;
        }
        else
        {
            /* Buffer is too small */
            EngSetLastError(ERROR_INVALID_PARAMETER);
            cjSize = 0;
        }
    }

    /* Unlock the region and return the size */
    REGION_UnlockRgn(prgn);
    return cjSize;
}

/* EOF */
