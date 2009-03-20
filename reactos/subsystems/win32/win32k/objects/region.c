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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice licence.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 *					  1999 Alex Korobka
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

#include <w32k.h>

#define NDEBUG
#include <debug.h>

PROSRGNDATA prgnDefault = NULL;
HRGN        hrgnDefault = NULL;

// Internal Functions

#if 1
#define COPY_RECTS(dest, src, nRects) \
  do {                                \
    PRECTL xDest = (dest);             \
    PRECTL xSrc = (src);               \
    UINT xRects = (nRects);           \
    while(xRects-- > 0) {             \
      *(xDest++) = *(xSrc++);         \
    }                                 \
  } while(0)
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
    int dx;      /* local storage */ \
\
    /* \
     *  if the edge is horizontal, then it is ignored \
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
 *     This structure contains all of the information needed
 *     to run the bresenham algorithm.
 *     The variables may be hardcoded into the declarations
 *     instead of using this structure to make use of
 *     register declarations.
 */
typedef struct
{
    INT minor_axis;	/* minor axis        */
    INT d;		/* decision variable */
    INT m, m1;       	/* slope and slope+1 */
    INT incr1, incr2;	/* error increments */
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
 *     list of ScanLineList structures containing a list of
 *     edges which are entered at a given scanline.  There is one
 *     ScanLineList per scanline at which an edge is entered.
 *     When we enter a new edge, we move it from the ET to the AET.
 *
 *     From the AET, we can implement the even-odd rule as in
 *     (Foley/Van Dam).
 *     The winding number rule is a little trickier.  We also
 *     keep the EdgeTableEntries in the AET linked by the
 *     nextWETE (winding EdgeTableEntry) link.  This allows
 *     the edges to be linked just as before for updating
 *     purposes, but only uses the edges linked by the nextWETE
 *     link as edges representing spans of the polygon to
 *     drawn (as with the even-odd rule).
 */

/*
 * for the winding number rule
 */
#define CLOCKWISE          1
#define COUNTERCLOCKWISE  -1

typedef struct _EdgeTableEntry
{
     INT ymax;             /* ycoord at which we exit this edge. */
     BRESINFO bres;        /* Bresenham info to run the edge     */
     struct _EdgeTableEntry *next;       /* next in the list     */
     struct _EdgeTableEntry *back;       /* for insertion sort   */
     struct _EdgeTableEntry *nextWETE;   /* for winding num rule */
     int ClockWise;        /* flag for winding number rule       */
} EdgeTableEntry;


typedef struct _ScanLineList
{
     INT scanline;                /* the scanline represented */
     EdgeTableEntry *edgelist;    /* header node              */
     struct _ScanLineList *next;  /* next in the list       */
} ScanLineList;


typedef struct
{
     INT ymax;                 /* ymax for the polygon     */
     INT ymin;                 /* ymin for the polygon     */
     ScanLineList scanlines;   /* header node              */
} EdgeTable;


/*
 * Here is a struct to help with storage allocation
 * so we can allocate a big chunk at a time, and then take
 * pieces from this heap when we need to.
 */
#define SLLSPERBLOCK 25

typedef struct _ScanLineListBlock
{
     ScanLineList SLLs[SLLSPERBLOCK];
     struct _ScanLineListBlock *next;
} ScanLineListBlock;


/*
 *
 *     a few macros for the inner loops of the fill code where
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
   if (pAET->ymax == y) {          /* leaving this edge */ \
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
   if (pAET->ymax == y) {          /* leaving this edge */ \
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

#define LARGE_COORDINATE  0x7fffffff /* FIXME */
#define SMALL_COORDINATE  0x80000000

/*
 *   Check to see if there is enough memory in the present region.
 */
static __inline int xmemcheck(ROSRGNDATA *reg, PRECTL *rect, PRECTL *firstrect)
{
    if ( (reg->rdh.nCount+1) * sizeof(RECT) >= reg->rdh.nRgnSize )
    {
        PRECTL temp;
        DWORD NewSize = 2 * reg->rdh.nRgnSize;
        if (NewSize < (reg->rdh.nCount + 1) * sizeof(RECT))
        {
            NewSize = (reg->rdh.nCount + 1) * sizeof(RECT);
        }
        temp = ExAllocatePoolWithTag(PagedPool, NewSize, TAG_REGION);

        if (temp == NULL)
        {
            return 0;
        }

        /* Copy the rectangles */
        COPY_RECTS(temp, *firstrect, reg->rdh.nCount);

        reg->rdh.nRgnSize = NewSize;
        if (*firstrect != &reg->rdh.rcBound)
        {
            ExFreePoolWithTag(*firstrect, TAG_REGION);
        }
        *firstrect = temp;
        *rect = (*firstrect)+reg->rdh.nCount;
    }
    return 1;
}

#define MEMCHECK(reg, rect, firstrect) xmemcheck(reg,&(rect),(PRECTL *)&(firstrect))

typedef void (FASTCALL *overlapProcp)(PROSRGNDATA, PRECT, PRECT, PRECT, PRECT, INT, INT);
typedef void (FASTCALL *nonOverlapProcp)(PROSRGNDATA, PRECT, PRECT, INT, INT);

// Number of points to buffer before sending them off to scanlines() :  Must be an even number
#define NUMPTSTOBUFFER 200

#define RGN_DEFAULT_RECTS	2

// used to allocate buffers for points and link the buffers together

typedef struct _POINTBLOCK
{
  POINT pts[NUMPTSTOBUFFER];
  struct _POINTBLOCK *next;
} POINTBLOCK;

#ifndef NDEBUG
/*
 * This function is left there for debugging purposes.
 */

VOID FASTCALL
IntDumpRegion(HRGN hRgn)
{
    ROSRGNDATA *Data;

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
#endif /* not NDEBUG */


INT
FASTCALL
REGION_Complexity( PROSRGNDATA obj )
{
    if (!obj) return NULLREGION;
    switch(obj->rdh.nCount)
    {
       DPRINT("Region Complexity -> %d",obj->rdh.nCount);
       case 0:  return NULLREGION;
       case 1:  return SIMPLEREGION;
       default: return COMPLEXREGION;
    }
}

static
BOOL
FASTCALL
REGION_CopyRegion(
    PROSRGNDATA dst,
    PROSRGNDATA src
)
{
    if (dst != src) //  don't want to copy to itself
    {
        if (dst->rdh.nRgnSize < src->rdh.nCount * sizeof(RECT))
        {
            PRECTL temp;

            temp = ExAllocatePoolWithTag(PagedPool, src->rdh.nCount * sizeof(RECT), TAG_REGION );
            if (!temp)
                return FALSE;

            if (dst->Buffer && dst->Buffer != &dst->rdh.rcBound)
                ExFreePoolWithTag(dst->Buffer, TAG_REGION);	//free the old buffer
            dst->Buffer = temp;
            dst->rdh.nRgnSize = src->rdh.nCount * sizeof(RECT);  //size of region buffer
        }
        dst->rdh.nCount = src->rdh.nCount;                 //number of rectangles present in Buffer
        dst->rdh.rcBound.left = src->rdh.rcBound.left;
        dst->rdh.rcBound.top = src->rdh.rcBound.top;
        dst->rdh.rcBound.right = src->rdh.rcBound.right;
        dst->rdh.rcBound.bottom = src->rdh.rcBound.bottom;
        dst->rdh.iType = src->rdh.iType;
        COPY_RECTS(dst->Buffer, src->Buffer, src->rdh.nCount);
    }
    return TRUE;
}

static void FASTCALL
REGION_SetExtents(ROSRGNDATA *pReg)
{
    RECTL *pRect, *pRectEnd, *pExtents;

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

    /*
     * Since pRect is the first rectangle in the region, it must have the
     * smallest top and since pRectEnd is the last rectangle in the region,
     * it must have the largest bottom, because of banding. Initialize left and
     * right from pRect and pRectEnd, resp., as good things to initialize them
     * to...
     */
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

// FIXME: This seems to be wrong
/***********************************************************************
 *           REGION_CropAndOffsetRegion
 */
BOOL FASTCALL
REGION_CropAndOffsetRegion(
    PROSRGNDATA rgnDst,
    PROSRGNDATA rgnSrc,
    const RECTL *rect,
    const POINTL *offset
)
{
    POINT pt = {0,0};
    const POINT *off = offset;

    if (!off) off = &pt;

    if (!rect) // just copy and offset
    {
        PRECTL xrect;
        if (rgnDst == rgnSrc)
        {
            if (off->x || off->y)
                xrect = rgnDst->Buffer;
            else
                return TRUE;
        }
        else
        {
            xrect = ExAllocatePoolWithTag(PagedPool, rgnSrc->rdh.nCount * sizeof(RECT), TAG_REGION);
            if (rgnDst->Buffer && rgnDst->Buffer != &rgnDst->rdh.rcBound)
                ExFreePoolWithTag(rgnDst->Buffer, TAG_REGION); //free the old buffer. will be assigned to xrect below.
        }

        if (xrect)
        {
            ULONG i;

            if (rgnDst != rgnSrc)
            {
                *rgnDst = *rgnSrc;
            }

            if (off->x || off->y)
            {
                for (i = 0; i < rgnDst->rdh.nCount; i++)
                {
                    xrect[i].left = (rgnSrc->Buffer + i)->left + off->x;
                    xrect[i].right = (rgnSrc->Buffer + i)->right + off->x;
                    xrect[i].top = (rgnSrc->Buffer + i)->top + off->y;
                    xrect[i].bottom = (rgnSrc->Buffer + i)->bottom + off->y;
                }
                rgnDst->rdh.rcBound.left   += off->x;
                rgnDst->rdh.rcBound.right  += off->x;
                rgnDst->rdh.rcBound.top    += off->y;
                rgnDst->rdh.rcBound.bottom += off->y;
            }
            else
            {
                COPY_RECTS(xrect, rgnSrc->Buffer, rgnDst->rdh.nCount);
            }

            rgnDst->Buffer = xrect;
        }
        else
            return FALSE;
    }
    else if ((rect->left >= rect->right) ||
             (rect->top >= rect->bottom) ||
             !EXTENTCHECK(rect, &rgnSrc->rdh.rcBound))
    {
        goto empty;
    }
    else // region box and clipping rect appear to intersect
    {
        PRECTL lpr, rpr;
        ULONG i, j, clipa, clipb;
        INT left = rgnSrc->rdh.rcBound.right + off->x;
        INT right = rgnSrc->rdh.rcBound.left + off->x;

        for (clipa = 0; (rgnSrc->Buffer + clipa)->bottom <= rect->top; clipa++)
            //region and rect intersect so we stop before clipa > rgnSrc->rdh.nCount
            ; // skip bands above the clipping rectangle

        for (clipb = clipa; clipb < rgnSrc->rdh.nCount; clipb++)
            if ((rgnSrc->Buffer + clipb)->top >= rect->bottom)
                break;    // and below it

        // clipa - index of the first rect in the first intersecting band
        // clipb - index of the last rect in the last intersecting band

        if ((rgnDst != rgnSrc) && (rgnDst->rdh.nCount < (i = (clipb - clipa))))
        {
            PRECTL temp;
            temp = ExAllocatePoolWithTag(PagedPool, i * sizeof(RECT), TAG_REGION);
            if (!temp)
                return FALSE;

            if (rgnDst->Buffer && rgnDst->Buffer != &rgnDst->rdh.rcBound)
                ExFreePoolWithTag(rgnDst->Buffer, TAG_REGION); //free the old buffer
            rgnDst->Buffer = temp;
            rgnDst->rdh.nCount = i;
            rgnDst->rdh.nRgnSize = i * sizeof(RECT);
        }

        for (i = clipa, j = 0; i < clipb ; i++)
        {
            // i - src index, j - dst index, j is always <= i for obvious reasons

            lpr = rgnSrc->Buffer + i;

            if (lpr->left < rect->right && lpr->right > rect->left)
            {
                rpr = rgnDst->Buffer + j;

                rpr->top = lpr->top + off->y;
                rpr->bottom = lpr->bottom + off->y;
                rpr->left = ((lpr->left > rect->left) ? lpr->left : rect->left) + off->x;
                rpr->right = ((lpr->right < rect->right) ? lpr->right : rect->right) + off->x;

                if (rpr->left < left) left = rpr->left;
                if (rpr->right > right) right = rpr->right;

                j++;
            }
        }

        if (j == 0) goto empty;

        rgnDst->rdh.rcBound.left = left;
        rgnDst->rdh.rcBound.right = right;

        left = rect->top + off->y;
        right = rect->bottom + off->y;

        rgnDst->rdh.nCount = j--;
        for (i = 0; i <= j; i++) // fixup top band
            if ((rgnDst->Buffer + i)->top < left)
                (rgnDst->Buffer + i)->top = left;
            else
                break;

        for (i = j; i > 0; i--) // fixup bottom band
            if ((rgnDst->Buffer + i)->bottom > right)
                (rgnDst->Buffer + i)->bottom = right;
            else
                break;

        rgnDst->rdh.rcBound.top = (rgnDst->Buffer)->top;
        rgnDst->rdh.rcBound.bottom = (rgnDst->Buffer + j)->bottom;

        rgnDst->rdh.iType = RDH_RECTANGLES;
    }

    return TRUE;

empty:
    if (!rgnDst->Buffer)
    {
        rgnDst->Buffer = ExAllocatePoolWithTag(PagedPool, RGN_DEFAULT_RECTS * sizeof(RECT), TAG_REGION);
        if (rgnDst->Buffer)
        {
            rgnDst->rdh.nCount = RGN_DEFAULT_RECTS;
            rgnDst->rdh.nRgnSize = RGN_DEFAULT_RECTS * sizeof(RECT);
        }
        else
            return FALSE;
    }
    EMPTY_REGION(rgnDst);
    return TRUE;
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
static INT FASTCALL
REGION_Coalesce(
    PROSRGNDATA pReg, /* Region to coalesce */
    INT prevStart,  /* Index of start of previous band */
    INT curStart    /* Index of start of current band */
)
{
    RECTL *pPrevRect;          /* Current rect in previous band */
    RECTL *pCurRect;           /* Current rect in current band */
    RECTL *pRegEnd;            /* End of region */
    INT curNumRects;          /* Number of rectangles in current band */
    INT prevNumRects;         /* Number of rectangles in previous band */
    INT bandtop;               /* top coordinate for current band */

    pRegEnd = pReg->Buffer + pReg->rdh.nCount;
    pPrevRect = pReg->Buffer + prevStart;
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in REGION_RegionOp
     * at the end when one region has been exhausted.
     */
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
        /*
         * If more than one band was added, we have to find the start
         * of the last band added so the next coalescing job can start
         * at the right place... (given when multiple bands are added,
         * this may be pointless -- see above).
         */
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
        /*
         * The bands may only be coalesced if the bottom of the previous
         * matches the top scanline of the current.
         */
        if (pPrevRect->bottom == pCurRect->top)
        {
            /*
             * Make sure the bands have rects in the same places. This
             * assumes that rects have been added in such a way that they
             * cover the most area possible. I.e. two rects in a band must
             * have some horizontal space between them.
             */
            do
            {
                if ((pPrevRect->left != pCurRect->left) ||
                        (pPrevRect->right != pCurRect->right))
                {
                    /*
                     * The bands don't line up so they can't be coalesced.
                     */
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

            /*
             * The bands may be merged, so set the bottom of each rect
             * in the previous band to that of the corresponding rect in
             * the current band.
             */
            do
            {
                pPrevRect->bottom = pCurRect->bottom;
                pPrevRect++;
                pCurRect++;
                curNumRects -= 1;
            }
            while (curNumRects != 0);

            /*
             * If only one band was added to the region, we have to backup
             * curStart to the start of the previous band.
             *
             * If more than one band was added to the region, copy the
             * other bands down. The assumption here is that the other bands
             * came from the same region as the current one and no further
             * coalescing can be done on them since it's all been done
             * already... curStart is already in the right place.
             */
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
static void FASTCALL
REGION_RegionOp(
    ROSRGNDATA *newReg, /* Place to store result */
    ROSRGNDATA *reg1,   /* First region in operation */
    ROSRGNDATA *reg2,   /* 2nd region in operation */
    overlapProcp overlapFunc,     /* Function to call for over-lapping bands */
    nonOverlapProcp nonOverlap1Func, /* Function to call for non-overlapping bands in region 1 */
    nonOverlapProcp nonOverlap2Func  /* Function to call for non-overlapping bands in region 2 */
)
{
    RECTL *r1;                         /* Pointer into first region */
    RECTL *r2;                         /* Pointer into 2d region */
    RECTL *r1End;                      /* End of 1st region */
    RECTL *r2End;                      /* End of 2d region */
    INT ybot;                         /* Bottom of intersection */
    INT ytop;                         /* Top of intersection */
    RECTL *oldRects;                   /* Old rects for newReg */
    ULONG prevBand;                   /* Index of start of
						 * previous band in newReg */
    ULONG curBand;                    /* Index of start of current band in newReg */
    RECTL *r1BandEnd;                  /* End of current band in r1 */
    RECTL *r2BandEnd;                  /* End of current band in r2 */
    ULONG top;                        /* Top of non-overlapping band */
    ULONG bot;                        /* Bottom of non-overlapping band */

    /*
     * Initialization:
     *  set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = reg1->Buffer;
    r2 = reg2->Buffer;
    r1End = r1 + reg1->rdh.nCount;
    r2End = r2 + reg2->rdh.nCount;


    /*
     * newReg may be one of the src regions so we can't empty it. We keep a
     * note of its rects pointer (so that we can free them later), preserve its
     * extents and simply set numRects to zero.
     */

    oldRects = newReg->Buffer;
    newReg->rdh.nCount = 0;

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually.
     */
    newReg->rdh.nRgnSize = max(reg1->rdh.nCount,reg2->rdh.nCount) * 2 * sizeof(RECT);

    if (! (newReg->Buffer = ExAllocatePoolWithTag(PagedPool, newReg->rdh.nRgnSize, TAG_REGION)))
    {
        newReg->rdh.nRgnSize = 0;
        return;
    }

    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->rdh.rcBound.top < reg2->rdh.rcBound.top)
        ybot = reg1->rdh.rcBound.top;
    else
        ybot = reg2->rdh.rcBound.top;

    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;

    do
    {
        curBand = newReg->rdh.nCount;

        /*
         * This algorithm proceeds one source-band (as opposed to a
         * destination band, which is determined by where the two regions
         * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
         * rectangle after the last one in the current band for their
         * respective regions.
         */
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

        /*
         * First handle the band that doesn't intersect, if any.
         *
         * Note that attention is restricted to one band in the
         * non-intersecting region at once, so if a region has n
         * bands between the current position and the next place it overlaps
         * the other, this entire loop will be passed through n times.
         */
        if (r1->top < r2->top)
        {
            top = max(r1->top,ybot);
            bot = min(r1->bottom,r2->top);

            if ((top != bot) && (nonOverlap1Func != NULL))
            {
                (* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
            }

            ytop = r2->top;
        }
        else if (r2->top < r1->top)
        {
            top = max(r2->top,ybot);
            bot = min(r2->bottom,r1->top);

            if ((top != bot) && (nonOverlap2Func != NULL))
            {
                (* nonOverlap2Func) (newReg, r2, r2BandEnd, top, bot);
            }

            ytop = r1->top;
        }
        else
        {
            ytop = r1->top;
        }

        /*
         * If any rectangles got added to the region, try and coalesce them
         * with rectangles from the previous band. Note we could just do
         * this test in miCoalesce, but some machines incur a not
         * inconsiderable cost for function calls, so...
         */
        if (newReg->rdh.nCount != curBand)
        {
            prevBand = REGION_Coalesce (newReg, prevBand, curBand);
        }

        /*
         * Now see if we've hit an intersecting band. The two bands only
         * intersect if ybot > ytop
         */
        ybot = min(r1->bottom, r2->bottom);
        curBand = newReg->rdh.nCount;
        if (ybot > ytop)
        {
            (* overlapFunc) (newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);
        }

        if (newReg->rdh.nCount != curBand)
        {
            prevBand = REGION_Coalesce (newReg, prevBand, curBand);
        }

        /*
         * If we've finished with a band (bottom == ybot) we skip forward
         * in the region to the next band.
         */
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

    /*
     * Deal with whichever region still has rectangles left.
     */
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
                (* nonOverlap1Func) (newReg, r1, r1BandEnd,
                                     max(r1->top,ybot), r1->bottom);
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
            (* nonOverlap2Func) (newReg, r2, r2BandEnd,
                                 max(r2->top,ybot), r2->bottom);
            r2 = r2BandEnd;
        }
        while (r2 != r2End);
    }

    if (newReg->rdh.nCount != curBand)
    {
        (void) REGION_Coalesce (newReg, prevBand, curBand);
    }

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...).
     */
    if ((2 * newReg->rdh.nCount*sizeof(RECT) < newReg->rdh.nRgnSize && (newReg->rdh.nCount > 2)))
    {
        if (REGION_NOT_EMPTY(newReg))
        {
            RECTL *prev_rects = newReg->Buffer;
            newReg->Buffer = ExAllocatePoolWithTag(PagedPool, newReg->rdh.nCount*sizeof(RECT), TAG_REGION);

            if (! newReg->Buffer)
                newReg->Buffer = prev_rects;
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
            /*
             * No point in doing the extra work involved in an Xrealloc if
             * the region is empty
             */
            newReg->rdh.nRgnSize = sizeof(RECT);
            if (newReg->Buffer != &newReg->rdh.rcBound)
                ExFreePoolWithTag(newReg->Buffer, TAG_REGION);
            newReg->Buffer = ExAllocatePoolWithTag(PagedPool, sizeof(RECT), TAG_REGION);
            ASSERT(newReg->Buffer);
        }
    }
    newReg->rdh.iType = RDH_RECTANGLES;

    if (oldRects != &newReg->rdh.rcBound)
        ExFreePoolWithTag(oldRects, TAG_REGION);
    return;
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
static void FASTCALL
REGION_IntersectO(
    PROSRGNDATA pReg,
    PRECTL      r1,
    PRECTL      r1End,
    PRECTL      r2,
    PRECTL      r2End,
    INT         top,
    INT         bottom
)
{
    INT       left, right;
    RECTL     *pNextRect;

    pNextRect = pReg->Buffer + pReg->rdh.nCount;

    while ((r1 != r1End) && (r2 != r2End))
    {
        left = max(r1->left, r2->left);
        right =	min(r1->right, r2->right);

        /*
         * If there's any overlap between the two rectangles, add that
         * overlap to the new region.
         * There's no need to check for subsumption because the only way
         * such a need could arise is if some region has two rectangles
         * right next to each other. Since that should never happen...
         */
        if (left < right)
        {
            MEMCHECK(pReg, pNextRect, pReg->Buffer);
            pNextRect->left = left;
            pNextRect->top = top;
            pNextRect->right = right;
            pNextRect->bottom = bottom;
            pReg->rdh.nCount += 1;
            pNextRect++;
        }

        /*
         * Need to advance the pointers. Shift the one that extends
         * to the right the least, since the other still has a chance to
         * overlap with that region's next rectangle, if you see what I mean.
         */
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
    return;
}

/***********************************************************************
 *	     REGION_IntersectRegion
 */
static void FASTCALL
REGION_IntersectRegion(
    ROSRGNDATA *newReg,
    ROSRGNDATA *reg1,
    ROSRGNDATA *reg2
)
{
    /* check for trivial reject */
    if ( (!(reg1->rdh.nCount)) || (!(reg2->rdh.nCount))  ||
         (!EXTENTCHECK(&reg1->rdh.rcBound, &reg2->rdh.rcBound)) )
        newReg->rdh.nCount = 0;
    else
        REGION_RegionOp (newReg, reg1, reg2,
                         REGION_IntersectO, NULL, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */

    REGION_SetExtents(newReg);
}

/***********************************************************************
 *	     Region Union
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
static void FASTCALL
REGION_UnionNonO (
    PROSRGNDATA pReg,
    PRECTL      r,
    PRECTL      rEnd,
    INT         top,
    INT         bottom
)
{
    RECTL *pNextRect;

    pNextRect = pReg->Buffer + pReg->rdh.nCount;

    while (r != rEnd)
    {
        MEMCHECK(pReg, pNextRect, pReg->Buffer);
        pNextRect->left = r->left;
        pNextRect->top = top;
        pNextRect->right = r->right;
        pNextRect->bottom = bottom;
        pReg->rdh.nCount += 1;
        pNextRect++;
        r++;
    }
    return;
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
static void FASTCALL
REGION_UnionO (
    PROSRGNDATA pReg,
    PRECTL      r1,
    PRECTL      r1End,
    PRECTL      r2,
    PRECTL      r2End,
    INT         top,
    INT         bottom
)
{
    RECTL *pNextRect;

    pNextRect = pReg->Buffer + pReg->rdh.nCount;

#define MERGERECT(r) \
    if ((pReg->rdh.nCount != 0) &&  \
		((pNextRect-1)->top == top) &&  \
		((pNextRect-1)->bottom == bottom) &&  \
		((pNextRect-1)->right >= r->left))  \
    {  \
		if ((pNextRect-1)->right < r->right)  \
		{  \
		    (pNextRect-1)->right = r->right;  \
		}  \
    }  \
    else  \
    {  \
		MEMCHECK(pReg, pNextRect, pReg->Buffer);  \
		pNextRect->top = top;  \
		pNextRect->bottom = bottom;  \
		pNextRect->left = r->left;  \
		pNextRect->right = r->right;  \
		pReg->rdh.nCount += 1;  \
		pNextRect += 1;  \
    }  \
    r++;

    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r1->left < r2->left)
        {
            MERGERECT(r1);
        }
        else
        {
            MERGERECT(r2);
        }
    }

    if (r1 != r1End)
    {
        do
        {
            MERGERECT(r1);
        }
        while (r1 != r1End);
    }
    else while (r2 != r2End)
        {
            MERGERECT(r2);
        }
    return;
}

/***********************************************************************
 *	     REGION_UnionRegion
 */
static void FASTCALL
REGION_UnionRegion(
    ROSRGNDATA *newReg,
    ROSRGNDATA *reg1,
    ROSRGNDATA *reg2
)
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if (reg1 == reg2 || 0 == reg1->rdh.nCount ||
            reg1->rdh.rcBound.right <= reg1->rdh.rcBound.left ||
            reg1->rdh.rcBound.bottom <= reg1->rdh.rcBound.top)
    {
        if (newReg != reg2)
        {
            REGION_CopyRegion(newReg, reg2);
        }
        return;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (0 == reg2->rdh.nCount ||
            reg2->rdh.rcBound.right <= reg2->rdh.rcBound.left ||
            reg2->rdh.rcBound.bottom <= reg2->rdh.rcBound.top)
    {
        if (newReg != reg1)
        {
            REGION_CopyRegion(newReg, reg1);
        }
        return;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if (1 == reg1->rdh.nCount &&
            reg1->rdh.rcBound.left <= reg2->rdh.rcBound.left &&
            reg1->rdh.rcBound.top <= reg2->rdh.rcBound.top &&
            reg2->rdh.rcBound.right <= reg1->rdh.rcBound.right &&
            reg2->rdh.rcBound.bottom <= reg1->rdh.rcBound.bottom)
    {
        if (newReg != reg1)
        {
            REGION_CopyRegion(newReg, reg1);
        }
        return;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if (1 == reg2->rdh.nCount &&
            reg2->rdh.rcBound.left <= reg1->rdh.rcBound.left &&
            reg2->rdh.rcBound.top <= reg1->rdh.rcBound.top &&
            reg1->rdh.rcBound.right <= reg2->rdh.rcBound.right &&
            reg1->rdh.rcBound.bottom <= reg2->rdh.rcBound.bottom)
    {
        if (newReg != reg2)
        {
            REGION_CopyRegion(newReg, reg2);
        }
        return;
    }

    REGION_RegionOp (newReg, reg1, reg2, REGION_UnionO,
                     REGION_UnionNonO, REGION_UnionNonO);
    newReg->rdh.rcBound.left = min(reg1->rdh.rcBound.left, reg2->rdh.rcBound.left);
    newReg->rdh.rcBound.top = min(reg1->rdh.rcBound.top, reg2->rdh.rcBound.top);
    newReg->rdh.rcBound.right = max(reg1->rdh.rcBound.right, reg2->rdh.rcBound.right);
    newReg->rdh.rcBound.bottom = max(reg1->rdh.rcBound.bottom, reg2->rdh.rcBound.bottom);
}

/***********************************************************************
 *	     Region Subtraction
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
static void FASTCALL
REGION_SubtractNonO1(
    PROSRGNDATA pReg,
    PRECTL      r,
    PRECTL      rEnd,
    INT         top,
    INT         bottom
)
{
    RECTL *pNextRect;

    pNextRect = pReg->Buffer + pReg->rdh.nCount;

    while (r != rEnd)
    {
        MEMCHECK(pReg, pNextRect, pReg->Buffer);
        pNextRect->left = r->left;
        pNextRect->top = top;
        pNextRect->right = r->right;
        pNextRect->bottom = bottom;
        pReg->rdh.nCount += 1;
        pNextRect++;
        r++;
    }
    return;
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
static void FASTCALL
REGION_SubtractO(
    PROSRGNDATA pReg,
    PRECTL      r1,
    PRECTL      r1End,
    PRECTL      r2,
    PRECTL      r2End,
    INT         top,
    INT         bottom
)
{
    RECTL *pNextRect;
    INT left;

    left = r1->left;
    pNextRect = pReg->Buffer + pReg->rdh.nCount;

    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r2->right <= left)
        {
            /*
             * Subtrahend missed the boat: go to next subtrahend.
             */
            r2++;
        }
        else if (r2->left <= left)
        {
            /*
             * Subtrahend preceeds minuend: nuke left edge of minuend.
             */
            left = r2->right;
            if (left >= r1->right)
            {
                /*
                 * Minuend completely covered: advance to next minuend and
                 * reset left fence to edge of new minuend.
                 */
                r1++;
                if (r1 != r1End)
                    left = r1->left;
            }
            else
            {
                /*
                 * Subtrahend now used up since it doesn't extend beyond
                 * minuend
                 */
                r2++;
            }
        }
        else if (r2->left < r1->right)
        {
            /*
             * Left part of subtrahend covers part of minuend: add uncovered
             * part of minuend to region and skip to next subtrahend.
             */
            MEMCHECK(pReg, pNextRect, pReg->Buffer);
            pNextRect->left = left;
            pNextRect->top = top;
            pNextRect->right = r2->left;
            pNextRect->bottom = bottom;
            pReg->rdh.nCount += 1;
            pNextRect++;
            left = r2->right;
            if (left >= r1->right)
            {
                /*
                 * Minuend used up: advance to new...
                 */
                r1++;
                if (r1 != r1End)
                    left = r1->left;
            }
            else
            {
                /*
                 * Subtrahend used up
                 */
                r2++;
            }
        }
        else
        {
            /*
             * Minuend used up: add any remaining piece before advancing.
             */
            if (r1->right > left)
            {
                MEMCHECK(pReg, pNextRect, pReg->Buffer);
                pNextRect->left = left;
                pNextRect->top = top;
                pNextRect->right = r1->right;
                pNextRect->bottom = bottom;
                pReg->rdh.nCount += 1;
                pNextRect++;
            }
            r1++;
            left = r1->left;
        }
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
        MEMCHECK(pReg, pNextRect, pReg->Buffer);
        pNextRect->left = left;
        pNextRect->top = top;
        pNextRect->right = r1->right;
        pNextRect->bottom = bottom;
        pReg->rdh.nCount += 1;
        pNextRect++;
        r1++;
        if (r1 != r1End)
        {
            left = r1->left;
        }
    }
    return;
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
static void FASTCALL
REGION_SubtractRegion(
    ROSRGNDATA *regD,
    ROSRGNDATA *regM,
    ROSRGNDATA *regS
)
{
    /* check for trivial reject */
    if ( (!(regM->rdh.nCount)) || (!(regS->rdh.nCount))  ||
         (!EXTENTCHECK(&regM->rdh.rcBound, &regS->rdh.rcBound)) )
    {
        REGION_CopyRegion(regD, regM);
        return;
    }

    REGION_RegionOp (regD, regM, regS, REGION_SubtractO,
                     REGION_SubtractNonO1, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents (regD);
}

/***********************************************************************
 *	     REGION_XorRegion
 */
static void FASTCALL
REGION_XorRegion(
    ROSRGNDATA *dr,
    ROSRGNDATA *sra,
    ROSRGNDATA *srb
)
{
    HRGN htra, htrb;
    ROSRGNDATA *tra, *trb;

    // FIXME: don't use a handle
    tra = REGION_AllocRgnWithHandle(sra->rdh.nCount + 1);
    if (!tra )
    {
        return;
    }
    htra = tra->BaseObject.hHmgr;

    // FIXME: don't use a handle
    trb = REGION_AllocRgnWithHandle(srb->rdh.nCount + 1);
    if (!trb)
    {
        REGION_UnlockRgn(tra);
        NtGdiDeleteObject(htra);
        return;
    }
    htrb = trb->BaseObject.hHmgr;

    REGION_SubtractRegion(tra, sra, srb);
    REGION_SubtractRegion(trb, srb, sra);
    REGION_UnionRegion(dr, tra, trb);
    REGION_UnlockRgn(tra);
    REGION_UnlockRgn(trb);

    NtGdiDeleteObject(htra);
    NtGdiDeleteObject(htrb);
    return;
}


/*!
 * Adds a rectangle to a REGION
 */
VOID FASTCALL
REGION_UnionRectWithRgn(
    ROSRGNDATA *rgn,
    const RECTL *rect
)
{
    ROSRGNDATA region;

    region.Buffer = &region.rdh.rcBound;
    region.rdh.nCount = 1;
    region.rdh.nRgnSize = sizeof(RECT);
    region.rdh.rcBound = *rect;
    REGION_UnionRegion(rgn, rgn, &region);
}

BOOL FASTCALL
REGION_CreateSimpleFrameRgn(
    PROSRGNDATA rgn,
    INT x,
    INT y
)
{
    RECTL rc[4];
    PRECTL prc;

    if (x != 0 || y != 0)
    {
        prc = rc;

        if (rgn->rdh.rcBound.bottom - rgn->rdh.rcBound.top > y * 2 &&
                rgn->rdh.rcBound.right - rgn->rdh.rcBound.left > x * 2)
        {
            if (y != 0)
            {
                /* top rectangle */
                prc->left = rgn->rdh.rcBound.left;
                prc->top = rgn->rdh.rcBound.top;
                prc->right = rgn->rdh.rcBound.right;
                prc->bottom = prc->top + y;
                prc++;
            }

            if (x != 0)
            {
                /* left rectangle */
                prc->left = rgn->rdh.rcBound.left;
                prc->top = rgn->rdh.rcBound.top + y;
                prc->right = prc->left + x;
                prc->bottom = rgn->rdh.rcBound.bottom - y;
                prc++;

                /* right rectangle */
                prc->left = rgn->rdh.rcBound.right - x;
                prc->top = rgn->rdh.rcBound.top + y;
                prc->right = rgn->rdh.rcBound.right;
                prc->bottom = rgn->rdh.rcBound.bottom - y;
                prc++;
            }

            if (y != 0)
            {
                /* bottom rectangle */
                prc->left = rgn->rdh.rcBound.left;
                prc->top = rgn->rdh.rcBound.bottom - y;
                prc->right = rgn->rdh.rcBound.right;
                prc->bottom = rgn->rdh.rcBound.bottom;
                prc++;
            }
        }

        if (prc != rc)
        {
            /* The frame results in a complex region. rcBounds remains
               the same, though. */
            rgn->rdh.nCount = (DWORD)(prc - rc);
            ASSERT(rgn->rdh.nCount > 1);
            rgn->rdh.nRgnSize = rgn->rdh.nCount * sizeof(RECT);
            rgn->Buffer = ExAllocatePoolWithTag(PagedPool, rgn->rdh.nRgnSize, TAG_REGION);
            if (!rgn->Buffer)
            {
                rgn->rdh.nRgnSize = 0;
                return FALSE;
            }

            COPY_RECTS(rgn->Buffer, rc, rgn->rdh.nCount);
        }
    }

    return TRUE;
}

BOOL FASTCALL
REGION_CreateFrameRgn(
    HRGN hDest,
    HRGN hSrc,
    INT x,
    INT y
)
{
    PROSRGNDATA srcObj, destObj;
    PRECTL rc;
    ULONG i;

    if (!(srcObj = REGION_LockRgn(hSrc)))
    {
        return FALSE;
    }
    if (!REGION_NOT_EMPTY(srcObj))
    {
        REGION_UnlockRgn(srcObj);
        return FALSE;
    }
    if (!(destObj = REGION_LockRgn(hDest)))
    {
        REGION_UnlockRgn(srcObj);
        return FALSE;
    }

    EMPTY_REGION(destObj);
    if (!REGION_CopyRegion(destObj, srcObj))
    {
        REGION_UnlockRgn(destObj);
        REGION_UnlockRgn(srcObj);
        return FALSE;
    }

    if (REGION_Complexity(srcObj) == SIMPLEREGION)
    {
        if (!REGION_CreateSimpleFrameRgn(destObj, x, y))
        {
            EMPTY_REGION(destObj);
            REGION_UnlockRgn(destObj);
            REGION_UnlockRgn(srcObj);
            return FALSE;
        }
    }
    else
    {
        /* Original region moved to right */
        rc = srcObj->Buffer;
        for (i = 0; i < srcObj->rdh.nCount; i++)
        {
            rc->left += x;
            rc->right += x;
            rc++;
        }
        REGION_IntersectRegion(destObj, destObj, srcObj);

        /* Original region moved to left */
        rc = srcObj->Buffer;
        for (i = 0; i < srcObj->rdh.nCount; i++)
        {
            rc->left -= 2 * x;
            rc->right -= 2 * x;
            rc++;
        }
        REGION_IntersectRegion(destObj, destObj, srcObj);

        /* Original region moved down */
        rc = srcObj->Buffer;
        for (i = 0; i < srcObj->rdh.nCount; i++)
        {
            rc->left += x;
            rc->right += x;
            rc->top += y;
            rc->bottom += y;
            rc++;
        }
        REGION_IntersectRegion(destObj, destObj, srcObj);

        /* Original region moved up */
        rc = srcObj->Buffer;
        for (i = 0; i < srcObj->rdh.nCount; i++)
        {
            rc->top -= 2 * y;
            rc->bottom -= 2 * y;
            rc++;
        }
        REGION_IntersectRegion(destObj, destObj, srcObj);

        /* Restore the original region */
        rc = srcObj->Buffer;
        for (i = 0; i < srcObj->rdh.nCount; i++)
        {
            rc->top += y;
            rc->bottom += y;
            rc++;
        }
        REGION_SubtractRegion(destObj, srcObj, destObj);
    }

    REGION_UnlockRgn(destObj);
    REGION_UnlockRgn(srcObj);
    return TRUE;
}


BOOL FASTCALL
REGION_LPTODP(
    PDC  dc,
    HRGN hDest,
    HRGN hSrc)
{
    RECTL *pCurRect, *pEndRect;
    PROSRGNDATA srcObj = NULL;
    PROSRGNDATA destObj = NULL;

    RECTL tmpRect;
    BOOL ret = FALSE;
    PDC_ATTR pdcattr;

    if (!dc)
        return ret;
    pdcattr = dc->pdcattr;

    if (pdcattr->iMapMode == MM_TEXT) // Requires only a translation
    {
        if (NtGdiCombineRgn(hDest, hSrc, 0, RGN_COPY) == ERROR)
            goto done;

        NtGdiOffsetRgn(hDest, pdcattr->ptlViewportOrg.x - pdcattr->ptlWindowOrg.x,
                       pdcattr->ptlViewportOrg.y - pdcattr->ptlWindowOrg.y);
        ret = TRUE;
        goto done;
    }

    if ( !(srcObj = REGION_LockRgn(hSrc)) )
        goto done;
    if ( !(destObj = REGION_LockRgn(hDest)) )
    {
        REGION_UnlockRgn(srcObj);
        goto done;
    }
    EMPTY_REGION(destObj);

    pEndRect = srcObj->Buffer + srcObj->rdh.nCount;
    for (pCurRect = srcObj->Buffer; pCurRect < pEndRect; pCurRect++)
    {
        tmpRect = *pCurRect;
        tmpRect.left = XLPTODP(pdcattr, tmpRect.left);
        tmpRect.top = YLPTODP(pdcattr, tmpRect.top);
        tmpRect.right = XLPTODP(pdcattr, tmpRect.right);
        tmpRect.bottom = YLPTODP(pdcattr, tmpRect.bottom);

        if (tmpRect.left > tmpRect.right)
        {
            INT tmp = tmpRect.left;
            tmpRect.left = tmpRect.right;
            tmpRect.right = tmp;
        }
        if (tmpRect.top > tmpRect.bottom)
        {
            INT tmp = tmpRect.top;
            tmpRect.top = tmpRect.bottom;
            tmpRect.bottom = tmp;
        }

        REGION_UnionRectWithRgn(destObj, &tmpRect);
    }
    ret = TRUE;

    REGION_UnlockRgn(srcObj);
    REGION_UnlockRgn(destObj);

done:
    return ret;
}

PROSRGNDATA
FASTCALL
REGION_AllocRgnWithHandle(INT nReg)
{
    HRGN hReg;
    PROSRGNDATA pReg;
    
    pReg = (PROSRGNDATA)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_REGION);
    if(!pReg)
    {
        return NULL;
    }
    
    hReg = pReg->BaseObject.hHmgr;

    if (nReg == 1)
    {
        /* Testing shows that > 95% of all regions have only 1 rect.
           Including that here saves us from having to do another allocation */
        pReg->Buffer = &pReg->rdh.rcBound;
    }
    else
    {
        pReg->Buffer = ExAllocatePoolWithTag(PagedPool, nReg * sizeof(RECT), TAG_REGION);
        if (!pReg->Buffer)
        {
            REGION_UnlockRgn(pReg);
            GDIOBJ_FreeObjByHandle(hReg, GDI_OBJECT_TYPE_REGION);
            return NULL;
        }
    }

    EMPTY_REGION(pReg);
    pReg->rdh.dwSize = sizeof(RGNDATAHEADER);
    pReg->rdh.nCount = nReg;
    pReg->rdh.nRgnSize = nReg * sizeof(RECT);

    return pReg;
}

BOOL INTERNAL_CALL
REGION_Cleanup(PVOID ObjectBody)
{
    PROSRGNDATA pRgn = (PROSRGNDATA)ObjectBody;
    if (pRgn->Buffer && pRgn->Buffer != &pRgn->rdh.rcBound)
        ExFreePool(pRgn->Buffer);
    return TRUE;
}

VOID FASTCALL
REGION_Delete(PROSRGNDATA pRgn)
{
  if ( pRgn == prgnDefault) return;
  return REGION_FreeRgn(pRgn);
}


VOID FASTCALL
IntGdiReleaseRaoRgn(PDC pDC)
{
  INT Index = GDI_HANDLE_GET_INDEX(pDC->BaseObject.hHmgr);
  PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
  pDC->fs |= DC_FLAG_DIRTY_RAO;
  Entry->Flags |= GDI_ENTRY_VALIDATE_VIS;
  RECTL_vSetEmptyRect(&pDC->erclClip);
}


VOID FASTCALL
IntGdiReleaseVisRgn(PDC pDC)
{
  INT Index = GDI_HANDLE_GET_INDEX(pDC->BaseObject.hHmgr);
  PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
  pDC->fs |= DC_FLAG_DIRTY_RAO;
  Entry->Flags |= GDI_ENTRY_VALIDATE_VIS;
  RECTL_vSetEmptyRect(&pDC->erclClip);
  REGION_Delete(pDC->prgnVis);
  pDC->prgnVis = prgnDefault;
}

VOID FASTCALL
IntUpdateVisRectRgn(PDC pDC, PROSRGNDATA pRgn)
{
  INT Index = GDI_HANDLE_GET_INDEX(pDC->BaseObject.hHmgr);
  PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
  PDC_ATTR pdcattr;
  RECTL rcl;

  if (Entry->Flags & GDI_ENTRY_VALIDATE_VIS)
  {
     pdcattr = pDC->pdcattr;

     pdcattr->VisRectRegion.Flags = REGION_Complexity(pRgn);

     if (pRgn && pdcattr->VisRectRegion.Flags != NULLREGION)
     {
        rcl.left   = pRgn->rdh.rcBound.left;
        rcl.top    = pRgn->rdh.rcBound.top;
        rcl.right  = pRgn->rdh.rcBound.right;
        rcl.bottom = pRgn->rdh.rcBound.bottom;

        rcl.left   -= pDC->erclWindow.left;
        rcl.top    -= pDC->erclWindow.top;
        rcl.right  -= pDC->erclWindow.left;
        rcl.bottom -= pDC->erclWindow.top;
     }
     else
        RECTL_vSetEmptyRect(&rcl);

     pdcattr->VisRectRegion.Rect = rcl;

     Entry->Flags &= ~GDI_ENTRY_VALIDATE_VIS;
  }
}

INT
FASTCALL
IntGdiCombineRgn(PROSRGNDATA destRgn,
                 PROSRGNDATA src1Rgn,
                 PROSRGNDATA src2Rgn,
                    INT  CombineMode)
{
  INT result = ERROR;

  if (destRgn)
  {
     if (src1Rgn)
     {
        if (CombineMode == RGN_COPY)
        {
           if ( !REGION_CopyRegion(destRgn, src1Rgn) )
               return ERROR;
           result = REGION_Complexity(destRgn);
        }
        else
        {
           if (src2Rgn)
           {
              switch (CombineMode)
              {
                 case RGN_AND:
                     REGION_IntersectRegion(destRgn, src1Rgn, src2Rgn);
                     break;
                 case RGN_OR:
                     REGION_UnionRegion(destRgn, src1Rgn, src2Rgn);
                     break;
                 case RGN_XOR:
                     REGION_XorRegion(destRgn, src1Rgn, src2Rgn);
                     break;
                 case RGN_DIFF:
                     REGION_SubtractRegion(destRgn, src1Rgn, src2Rgn);
                     break;
              }
              result = REGION_Complexity(destRgn);
           }
           else if (src2Rgn == NULL)
           {
               DPRINT1("IntGdiCombineRgn requires hSrc2 != NULL for combine mode %d!\n", CombineMode);
           }
        }
     }
  }
  else
  {
     DPRINT("IntGdiCombineRgn: hDest unavailable\n");
     result = ERROR;
  }
  return result;
}


// NtGdi Exported Functions
INT
APIENTRY
NtGdiCombineRgn(HRGN  hDest,
                HRGN  hSrc1,
                HRGN  hSrc2,
                INT  CombineMode)
{
    INT result = ERROR;
    PROSRGNDATA destRgn, src1Rgn, src2Rgn;

    destRgn = REGION_LockRgn(hDest);
    if (destRgn)
    {
        src1Rgn = REGION_LockRgn(hSrc1);
        if (src1Rgn)
        {
            if (CombineMode == RGN_COPY)
            {
                if ( !REGION_CopyRegion(destRgn, src1Rgn) )
                    return ERROR;
                result = REGION_Complexity(destRgn);
            }
            else
            {
                src2Rgn = REGION_LockRgn(hSrc2);
                if (src2Rgn)
                {
                    switch (CombineMode)
                    {
                    case RGN_AND:
                        REGION_IntersectRegion(destRgn, src1Rgn, src2Rgn);
                        break;
                    case RGN_OR:
                        REGION_UnionRegion(destRgn, src1Rgn, src2Rgn);
                        break;
                    case RGN_XOR:
                        REGION_XorRegion(destRgn, src1Rgn, src2Rgn);
                        break;
                    case RGN_DIFF:
                        REGION_SubtractRegion(destRgn, src1Rgn, src2Rgn);
                        break;
                    }
                    REGION_UnlockRgn(src2Rgn);
                    result = REGION_Complexity(destRgn);
                }
                else if (hSrc2 == NULL)
                {
                    DPRINT1("NtGdiCombineRgn requires hSrc2 != NULL for combine mode %d!\n", CombineMode);
                }
            }

            REGION_UnlockRgn(src1Rgn);
        }

        REGION_UnlockRgn(destRgn);
    }
    else
    {
        DPRINT("NtGdiCombineRgn: hDest unavailable\n");
        result = ERROR;
    }

    return result;
}

HRGN
APIENTRY
NtGdiCreateEllipticRgn(
    INT Left,
    INT Top,
    INT Right,
    INT Bottom
)
{
    return NtGdiCreateRoundRectRgn(Left, Top, Right, Bottom,
                                   Right - Left, Bottom - Top);
}

PROSRGNDATA
FASTCALL
IntGdiCreateRectRgn(INT LeftRect, INT TopRect, INT RightRect, INT BottomRect)
{
  PROSRGNDATA pRgn;

  if (!(pRgn = REGION_AllocRgnWithHandle(1))) return NULL;

  REGION_SetRectRgn(pRgn, LeftRect, TopRect, RightRect, BottomRect);
  REGION_UnlockRgn(pRgn);
  // Return pointer with Share locks.
  pRgn = GDIOBJ_ShareLockObj(pRgn->BaseObject.hHmgr, GDI_OBJECT_TYPE_REGION);

  return pRgn;
}


HRGN APIENTRY
NtGdiCreateRectRgn(INT LeftRect, INT TopRect, INT RightRect, INT BottomRect)
{
    PROSRGNDATA pRgn;
    HRGN hRgn;

    /* Allocate region data structure with space for 1 RECTL */
    if (!(pRgn = REGION_AllocRgnWithHandle(1)))
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hRgn = pRgn->BaseObject.hHmgr;

    REGION_SetRectRgn(pRgn, LeftRect, TopRect, RightRect, BottomRect);
    REGION_UnlockRgn(pRgn);

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
    INT ellipse_height
)
{
    PROSRGNDATA obj;
    HRGN hrgn;
    int asq, bsq, d, xd, yd;
    RECTL rect;

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

    ellipse_width = abs(ellipse_width);
    ellipse_height = abs(ellipse_height);

    /* Check parameters */

    if (ellipse_width > right-left) ellipse_width = right-left;
    if (ellipse_height > bottom-top) ellipse_height = bottom-top;

    /* Check if we can do a normal rectangle instead */

    if ((ellipse_width < 2) || (ellipse_height < 2))
        return NtGdiCreateRectRgn(left, top, right, bottom);

    /* Create region */

    d = (ellipse_height < 128) ? ((3 * ellipse_height) >> 2) : 64;
    if (!(obj = REGION_AllocRgnWithHandle(d))) return 0;
    hrgn = obj->BaseObject.hHmgr;

    /* Ellipse algorithm, based on an article by K. Porter */
    /* in DDJ Graphics Programming Column, 8/89 */

    asq = ellipse_width * ellipse_width / 4;        /* a^2 */
    bsq = ellipse_height * ellipse_height / 4;      /* b^2 */
    d = bsq - asq * ellipse_height / 2 + asq / 4;   /* b^2 - a^2b + a^2/4 */
    xd = 0;
    yd = asq * ellipse_height;                      /* 2a^2b */

    rect.left   = left + ellipse_width / 2;
    rect.right  = right - ellipse_width / 2;

    /* Loop to draw first half of quadrant */

    while (xd < yd)
    {
        if (d > 0)  /* if nearest pixel is toward the center */
        {
            /* move toward center */
            rect.top = top++;
            rect.bottom = rect.top + 1;
            REGION_UnionRectWithRgn(obj, &rect);
            rect.top = --bottom;
            rect.bottom = rect.top + 1;
            REGION_UnionRectWithRgn(obj, &rect);
            yd -= 2*asq;
            d  -= yd;
        }
        rect.left--;        /* next horiz point */
        rect.right++;
        xd += 2*bsq;
        d  += bsq + xd;
    }
    /* Loop to draw second half of quadrant */

    d += (3 * (asq-bsq) / 2 - (xd+yd)) / 2;
    while (yd >= 0)
    {
        /* next vertical point */
        rect.top = top++;
        rect.bottom = rect.top + 1;
        REGION_UnionRectWithRgn(obj, &rect);
        rect.top = --bottom;
        rect.bottom = rect.top + 1;
        REGION_UnionRectWithRgn(obj, &rect);
        if (d < 0)   /* if nearest pixel is outside ellipse */
        {
            rect.left--;     /* move away from center */
            rect.right++;
            xd += 2*bsq;
            d  += xd;
        }
        yd -= 2*asq;
        d  += asq - yd;
    }
    /* Add the inside rectangle */

    if (top <= bottom)
    {
        rect.top = top;
        rect.bottom = bottom;
        REGION_UnionRectWithRgn(obj, &rect);
    }

    REGION_UnlockRgn(obj);
    return hrgn;
}

BOOL
APIENTRY
NtGdiEqualRgn(
    HRGN  hSrcRgn1,
    HRGN  hSrcRgn2
)
{
    PROSRGNDATA rgn1, rgn2;
    PRECTL tRect1, tRect2;
    ULONG i;
    BOOL bRet = FALSE;

    if ( !(rgn1 = REGION_LockRgn(hSrcRgn1)) )
        return ERROR;

    if ( !(rgn2 = REGION_LockRgn(hSrcRgn2)) )
    {
        REGION_UnlockRgn(rgn1);
        return ERROR;
    }

    if (rgn1->rdh.nCount != rgn2->rdh.nCount ||
            rgn1->rdh.nCount == 0 ||
            rgn1->rdh.rcBound.left   != rgn2->rdh.rcBound.left ||
            rgn1->rdh.rcBound.right  != rgn2->rdh.rcBound.right ||
            rgn1->rdh.rcBound.top    != rgn2->rdh.rcBound.top ||
            rgn1->rdh.rcBound.bottom != rgn2->rdh.rcBound.bottom)
        goto exit;

    tRect1 = rgn1->Buffer;
    tRect2 = rgn2->Buffer;

    if (!tRect1 || !tRect2)
        goto exit;

    for (i=0; i < rgn1->rdh.nCount; i++)
    {
        if (tRect1[i].left   != tRect2[i].left ||
                tRect1[i].right  != tRect2[i].right ||
                tRect1[i].top    != tRect2[i].top ||
                tRect1[i].bottom != tRect2[i].bottom)
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
    LPRGNDATA RgnData
)
{
    HRGN hRgn;
    PROSRGNDATA Region;
    DWORD nCount = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    MATRIX matrix;

    DPRINT("NtGdiExtCreateRegion\n");
    _SEH2_TRY
    {
        ProbeForRead(RgnData, Count, 1);
        nCount = RgnData->rdh.nCount;
        if (Count < sizeof(RGNDATAHEADER) + nCount * sizeof(RECT) ||
            nCount == 0 ||
            RgnData->rdh.iType != RDH_RECTANGLES ||
            RgnData->rdh.dwSize != sizeof(RGNDATAHEADER))
        {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
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

    Region = REGION_AllocRgnWithHandle(nCount);

    if (Region == NULL)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    hRgn = Region->BaseObject.hHmgr;

    _SEH2_TRY
    {
        /* Copy header */
        Region->rdh = RgnData->rdh;

        if (Xform)
        {
            ULONG ret;

            /* Init the XFORMOBJ from the Xform struct */
            Status = STATUS_INVALID_PARAMETER;
            ret = XFORMOBJ_iSetXform((XFORMOBJ*)&matrix, (XFORML*)Xform);

            /* Check for error, also no scale and shear allowed */
            if (ret != DDI_ERROR && ret != GX_GENERAL)
            {
                /* Apply the coordinate transformation on the rects */
                if (XFORMOBJ_bApplyXform((XFORMOBJ*)&matrix,
                                         XF_LTOL,
                                         nCount * 2,
                                         RgnData->Buffer,
                                         Region->Buffer))
                {
                    Status = STATUS_SUCCESS;
                }
            }
        }
        else
        {
            /* Copy rect coordinates */
            RtlCopyMemory(Region->Buffer,
                          RgnData->Buffer,
                          nCount * sizeof(RECT));
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if (!NT_SUCCESS(Status))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        REGION_UnlockRgn(Region);
        NtGdiDeleteObject(hRgn);
        return NULL;
    }

    REGION_UnlockRgn(Region);

    return hRgn;
}

BOOL
APIENTRY
NtGdiFillRgn(
    HDC hDC,
    HRGN hRgn,
    HBRUSH hBrush
)
{
    HBRUSH oldhBrush;
    PROSRGNDATA rgn;
    PRECTL r;

    if (NULL == (rgn = REGION_LockRgn(hRgn)))
    {
        return FALSE;
    }

    if (NULL == (oldhBrush = NtGdiSelectBrush(hDC, hBrush)))
    {
        REGION_UnlockRgn(rgn);
        return FALSE;
    }

    for (r = rgn->Buffer; r < rgn->Buffer + rgn->rdh.nCount; r++)
    {
        NtGdiPatBlt(hDC, r->left, r->top, r->right - r->left, r->bottom - r->top, PATCOPY);
    }

    REGION_UnlockRgn(rgn);
    NtGdiSelectBrush(hDC, oldhBrush);

    return TRUE;
}

BOOL
APIENTRY
NtGdiFrameRgn(
    HDC hDC,
    HRGN hRgn,
    HBRUSH hBrush,
    INT Width,
    INT Height
)
{
    HRGN FrameRgn;
    BOOL Ret;

    if (!(FrameRgn = NtGdiCreateRectRgn(0, 0, 0, 0)))
    {
        return FALSE;
    }
    if (!REGION_CreateFrameRgn(FrameRgn, hRgn, Width, Height))
    {
        NtGdiDeleteObject(FrameRgn);
        return FALSE;
    }

    Ret = NtGdiFillRgn(hDC, FrameRgn, hBrush);

    NtGdiDeleteObject(FrameRgn);
    return Ret;
}

INT FASTCALL
REGION_GetRgnBox(
    PROSRGNDATA Rgn,
    PRECTL pRect
)
{
    DWORD ret;

    if (Rgn)
    {
        *pRect = Rgn->rdh.rcBound;
        ret = REGION_Complexity(Rgn);

        return ret;
    }
    return 0; //if invalid region return zero
}


/* See wine, msdn, osr and  Feng Yuan - Windows Graphics Programming Win32 Gdi And Directdraw

   1st: http://www.codeproject.com/gdi/cliprgnguide.asp is wrong!

   The intersection of the clip with the meta region is not Rao it's API!
   Go back and read 7.2 Clipping pages 418-19:
   Rao = API & Vis:
   1) The Rao region is the intersection of the API region and the system region,
      named after the Microsoft engineer who initially proposed it.
   2) The Rao region can be calculated from the API region and the system region.

   API:
      API region is the intersection of the meta region and the clipping region,
      clearly named after the fact that it is controlled by GDI API calls.
*/
INT APIENTRY
NtGdiGetRandomRgn(
    HDC hDC,
    HRGN hDest,
    INT iCode
)
{
    INT ret = 0;
    PDC pDC;
    HRGN hSrc = NULL;
    POINT org;

    pDC = DC_LockDc(hDC);
    if (pDC == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return -1;
    }

    switch (iCode)
    {
    case CLIPRGN:
        hSrc = pDC->rosdc.hClipRgn;
//        if (pDC->dclevel.prgnClip) hSrc = ((PROSRGNDATA)pDC->dclevel.prgnClip)->BaseObject.hHmgr;
        break;
    case METARGN:
        if (pDC->dclevel.prgnMeta) hSrc = ((PROSRGNDATA)pDC->dclevel.prgnMeta)->BaseObject.hHmgr;
        break;
    case APIRGN:
        DPRINT1("hMetaRgn not implemented\n");
        //hSrc = dc->hMetaClipRgn;
        if (!hSrc) hSrc = pDC->rosdc.hClipRgn;
        //if (!hSrc) rgn = dc->hMetaRgn;
//        if (pDC->prgnAPI) hSrc = ((PROSRGNDATA)pDC->prgnAPI)->BaseObject.hHmgr;
//        else if (pDC->dclevel.prgnClip) hSrc = ((PROSRGNDATA)pDC->dclevel.prgnClip)->BaseObject.hHmgr;
//        else if (pDC->dclevel.prgnMeta) hSrc = ((PROSRGNDATA)pDC->dclevel.prgnMeta)->BaseObject.hHmgr;
        break;
    case SYSRGN:
        hSrc = pDC->rosdc.hVisRgn;
//        if (pDC->prgnVis) hSrc = ((PROSRGNDATA)pDC->prgnVis)->BaseObject.hHmgr;
        break;
    default:
        hSrc = 0;
    }
    if (hSrc)
    {
        if (NtGdiCombineRgn(hDest, hSrc, 0, RGN_COPY) == ERROR)
        {
            ret = -1;
        }
        else
        {
            ret = 1;
        }
    }
    if (iCode == SYSRGN)
    {
        IntGdiGetDCOrg(pDC, &org);
        NtGdiOffsetRgn(hDest, org.x, org.y );
    }

    DC_UnlockDc(pDC);

    return ret;
}

INT APIENTRY
IntGdiGetRgnBox(
    HRGN hRgn,
    PRECTL pRect
)
{
    PROSRGNDATA Rgn;
    DWORD ret;

    if (!(Rgn = REGION_LockRgn(hRgn)))
    {
        return ERROR;
    }

    ret = REGION_GetRgnBox(Rgn, pRect);
    REGION_UnlockRgn(Rgn);

    return ret;
}


INT APIENTRY
NtGdiGetRgnBox(
    HRGN hRgn,
    PRECTL pRect
)
{
    PROSRGNDATA  Rgn;
    RECTL SafeRect;
    DWORD ret;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!(Rgn = REGION_LockRgn(hRgn)))
    {
        return ERROR;
    }

    ret = REGION_GetRgnBox(Rgn, &SafeRect);
    REGION_UnlockRgn(Rgn);
    if (ERROR == ret)
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

BOOL
APIENTRY
NtGdiInvertRgn(
    HDC hDC,
    HRGN hRgn
)
{
    PROSRGNDATA RgnData;
    ULONG i;
    PRECTL rc;

    if (!(RgnData = REGION_LockRgn(hRgn)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    rc = RgnData->Buffer;
    for (i = 0; i < RgnData->rdh.nCount; i++)
    {

        if (!NtGdiPatBlt(hDC, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, DSTINVERT))
        {
            REGION_UnlockRgn(RgnData);
            return FALSE;
        }
        rc++;
    }

    REGION_UnlockRgn(RgnData);
    return TRUE;
}

INT
APIENTRY
NtGdiOffsetRgn(
    HRGN hRgn,
    INT XOffset,
    INT YOffset
)
{
    PROSRGNDATA rgn = REGION_LockRgn(hRgn);
    INT ret;

    DPRINT("NtGdiOffsetRgn: hRgn %d Xoffs %d Yoffs %d rgn %x\n", hRgn, XOffset, YOffset, rgn );

    if (!rgn)
    {
        DPRINT("NtGdiOffsetRgn: hRgn error\n");
        return ERROR;
    }

    if (XOffset || YOffset)
    {
        int nbox = rgn->rdh.nCount;
        PRECTL pbox = rgn->Buffer;

        if (nbox && pbox)
        {
            while (nbox--)
            {
                pbox->left += XOffset;
                pbox->right += XOffset;
                pbox->top += YOffset;
                pbox->bottom += YOffset;
                pbox++;
            }
            if (rgn->Buffer != &rgn->rdh.rcBound)
            {
                rgn->rdh.rcBound.left += XOffset;
                rgn->rdh.rcBound.right += XOffset;
                rgn->rdh.rcBound.top += YOffset;
                rgn->rdh.rcBound.bottom += YOffset;
            }
        }
    }
    ret = REGION_Complexity(rgn);
    REGION_UnlockRgn(rgn);
    return ret;
}

BOOL
FASTCALL
IntGdiPaintRgn(
    PDC dc,
    HRGN hRgn
)
{
    HRGN tmpVisRgn;
    PROSRGNDATA visrgn;
    CLIPOBJ* ClipRegion;
    BOOL bRet = FALSE;
    PGDIBRUSHOBJ pBrush;
    GDIBRUSHINST BrushInst;
    POINTL BrushOrigin;
    SURFACE *psurf;
    PDC_ATTR pdcattr;

    if (!dc) return FALSE;
    pdcattr = dc->pdcattr;

    if (!(tmpVisRgn = NtGdiCreateRectRgn(0, 0, 0, 0))) return FALSE;

    // Transform region into device co-ords
    if (!REGION_LPTODP(dc, tmpVisRgn, hRgn) || 
         NtGdiOffsetRgn(tmpVisRgn, dc->ptlDCOrig.x, dc->ptlDCOrig.y) == ERROR)
    {
        NtGdiDeleteObject(tmpVisRgn);
        return FALSE;
    }

    NtGdiCombineRgn(tmpVisRgn, tmpVisRgn, dc->rosdc.hGCClipRgn, RGN_AND);

    visrgn = REGION_LockRgn(tmpVisRgn);
    if (visrgn == NULL)
    {
        NtGdiDeleteObject(tmpVisRgn);
        return FALSE;
    }

    ClipRegion = IntEngCreateClipRegion(visrgn->rdh.nCount,
                                        visrgn->Buffer,
                                        &visrgn->rdh.rcBound );
    ASSERT(ClipRegion);
    pBrush = BRUSHOBJ_LockBrush(pdcattr->hbrush);
    ASSERT(pBrush);
    IntGdiInitBrushInstance(&BrushInst, pBrush, dc->rosdc.XlateBrush);

    BrushOrigin.x = pdcattr->ptlBrushOrigin.x;
    BrushOrigin.y = pdcattr->ptlBrushOrigin.y;
    psurf = SURFACE_LockSurface(dc->rosdc.hBitmap);
    /* FIXME - Handle psurf == NULL !!!! */

    bRet = IntEngPaint(&psurf->SurfObj,
                       ClipRegion,
                       &BrushInst.BrushObject,
                       &BrushOrigin,
                       0xFFFF);//FIXME:don't know what to put here

    SURFACE_UnlockSurface(psurf);
    BRUSHOBJ_UnlockBrush(pBrush);
    REGION_UnlockRgn(visrgn);
    NtGdiDeleteObject(tmpVisRgn);

    // Fill the region
    return TRUE;
}

BOOL
APIENTRY
NtGdiPtInRegion(
    HRGN hRgn,
    INT X,
    INT Y
)
{
    PROSRGNDATA rgn;
    ULONG i;
    PRECTL r;

    if (!(rgn = REGION_LockRgn(hRgn) ) )
        return FALSE;

    if (rgn->rdh.nCount > 0 && INRECT(rgn->rdh.rcBound, X, Y))
    {
        r =  rgn->Buffer;
        for (i = 0; i < rgn->rdh.nCount; i++)
        {
            if (INRECT(*r, X, Y))
            {
                REGION_UnlockRgn(rgn);
                return TRUE;
            }
            r++;
        }
    }
    REGION_UnlockRgn(rgn);
    return FALSE;
}

BOOL
FASTCALL
REGION_RectInRegion(
    PROSRGNDATA Rgn,
    const RECTL *rc
)
{
    PRECTL pCurRect, pRectEnd;

    // this is (just) a useful optimization
    if ((Rgn->rdh.nCount > 0) && EXTENTCHECK(&Rgn->rdh.rcBound, rc))
    {
        for (pCurRect = Rgn->Buffer, pRectEnd = pCurRect + Rgn->rdh.nCount; pCurRect < pRectEnd; pCurRect++)
        {
            if (pCurRect->bottom <= rc->top) continue; // not far enough down yet
            if (pCurRect->top >= rc->bottom) break;    // too far down
            if (pCurRect->right <= rc->left) continue; // not far enough over yet
            if (pCurRect->left >= rc->right) continue;

            return TRUE;
        }
    }
    return FALSE;
}

BOOL
APIENTRY
NtGdiRectInRegion(
    HRGN  hRgn,
    LPRECTL unsaferc
)
{
    PROSRGNDATA Rgn;
    RECTL rc = {0};
    BOOL Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!(Rgn = REGION_LockRgn(hRgn)))
    {
        return ERROR;
    }

    _SEH2_TRY
    {
        ProbeForRead(unsaferc, sizeof(RECT), 1);
        rc = *unsaferc;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        REGION_UnlockRgn(Rgn);
        SetLastNtError(Status);
        DPRINT1("NtGdiRectInRegion: bogus rc\n");
        return ERROR;
    }

    Ret = REGION_RectInRegion(Rgn, &rc);
    REGION_UnlockRgn(Rgn);
    return Ret;
}

VOID
FASTCALL
REGION_SetRectRgn(
    PROSRGNDATA rgn,
    INT LeftRect,
    INT TopRect,
    INT RightRect,
    INT BottomRect
)
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
        EMPTY_REGION(rgn);
}

BOOL
APIENTRY
NtGdiSetRectRgn(
    HRGN hRgn,
    INT LeftRect,
    INT TopRect,
    INT RightRect,
    INT BottomRect
)
{
    PROSRGNDATA rgn;

    if ( !(rgn = REGION_LockRgn(hRgn)) )
    {
        return 0; //per documentation
    }

    REGION_SetRectRgn(rgn, LeftRect, TopRect, RightRect, BottomRect);

    REGION_UnlockRgn(rgn);
    return TRUE;
}

HRGN APIENTRY
NtGdiUnionRectWithRgn(
    HRGN hDest,
    const RECTL *UnsafeRect
)
{
    RECTL SafeRect = {0};
    PROSRGNDATA Rgn;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!(Rgn = REGION_LockRgn(hDest)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
    }

    _SEH2_TRY
    {
        ProbeForRead(UnsafeRect, sizeof(RECT), 1);
        SafeRect = *UnsafeRect;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (! NT_SUCCESS(Status))
    {
        REGION_UnlockRgn(Rgn);
        SetLastNtError(Status);
        return NULL;
    }

    REGION_UnionRectWithRgn(Rgn, &SafeRect);
    REGION_UnlockRgn(Rgn);
    return hDest;
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
DWORD APIENTRY
NtGdiGetRegionData(
    HRGN hrgn,
    DWORD count,
    LPRGNDATA rgndata
)
{
    DWORD size;
    PROSRGNDATA obj = REGION_LockRgn(hrgn);
    NTSTATUS Status = STATUS_SUCCESS;

    if (!obj)
        return 0;

    size = obj->rdh.nCount * sizeof(RECT);
    if (count < (size + sizeof(RGNDATAHEADER)) || rgndata == NULL)
    {
        REGION_UnlockRgn(obj);
        if (rgndata) /* buffer is too small, signal it by return 0 */
            return 0;
        else         /* user requested buffer size with rgndata NULL */
            return size + sizeof(RGNDATAHEADER);
    }

    _SEH2_TRY
    {
        ProbeForWrite(rgndata, count, 1);
        RtlCopyMemory(rgndata, &obj->rdh, sizeof(RGNDATAHEADER));
        RtlCopyMemory(rgndata->Buffer, obj->Buffer, size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        REGION_UnlockRgn(obj);
        return 0;
    }

    REGION_UnlockRgn(obj);
    return size + sizeof(RGNDATAHEADER);
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
static void FASTCALL
REGION_InsertEdgeInET(
    EdgeTable *ET,
    EdgeTableEntry *ETE,
    INT scanline,
    ScanLineListBlock **SLLBlock,
    INT *iSLLBlock
)
{
    EdgeTableEntry *start, *prev;
    ScanLineList *pSLL, *pPrevSLL;
    ScanLineListBlock *tmpSLLBlock;

    /*
     * find the right bucket to put the edge into
     */
    pPrevSLL = &ET->scanlines;
    pSLL = pPrevSLL->next;
    while (pSLL && (pSLL->scanline < scanline))
    {
        pPrevSLL = pSLL;
        pSLL = pSLL->next;
    }

    /*
     * reassign pSLL (pointer to ScanLineList) if necessary
     */
    if ((!pSLL) || (pSLL->scanline > scanline))
    {
        if (*iSLLBlock > SLLSPERBLOCK-1)
        {
            tmpSLLBlock = ExAllocatePoolWithTag(PagedPool, sizeof(ScanLineListBlock), TAG_REGION);
            if (!tmpSLLBlock)
            {
                DPRINT1("REGION_InsertEdgeInETL(): Can't alloc SLLB\n");
                /* FIXME - free resources? */
                return;
            }
            (*SLLBlock)->next = tmpSLLBlock;
            tmpSLLBlock->next = (ScanLineListBlock *)NULL;
            *SLLBlock = tmpSLLBlock;
            *iSLLBlock = 0;
        }
        pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

        pSLL->next = pPrevSLL->next;
        pSLL->edgelist = (EdgeTableEntry *)NULL;
        pPrevSLL->next = pSLL;
    }
    pSLL->scanline = scanline;

    /*
     * now insert the edge in the right bucket
     */
    prev = (EdgeTableEntry *)NULL;
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
 *     This routine moves EdgeTableEntries from the
 *     EdgeTable into the Active Edge Table,
 *     leaving them sorted by smaller x coordinate.
 *
 */
static void FASTCALL
REGION_loadAET(
    EdgeTableEntry *AET,
    EdgeTableEntry *ETEs
)
{
    EdgeTableEntry *pPrevAET;
    EdgeTableEntry *tmp;

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
 *     nextWETE (winding EdgeTableEntry) link for
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
static void FASTCALL
REGION_computeWAET(EdgeTableEntry *AET)
{
    register EdgeTableEntry *pWETE;
    register int inside = 1;
    register int isInside = 0;

    AET->nextWETE = (EdgeTableEntry *)NULL;
    pWETE = AET;
    AET = AET->next;
    while (AET)
    {
        if (AET->ClockWise)
            isInside++;
        else
            isInside--;

        if ( (!inside && !isInside) ||
             ( inside &&  isInside) )
        {
            pWETE->nextWETE = AET;
            pWETE = AET;
            inside = !inside;
        }
        AET = AET->next;
    }
    pWETE->nextWETE = (EdgeTableEntry *)NULL;
}

/***********************************************************************
 *     REGION_InsertionSort
 *
 *     Just a simple insertion sort using
 *     pointers and back pointers to sort the Active
 *     Edge Table.
 *
 */
static BOOL FASTCALL
REGION_InsertionSort(EdgeTableEntry *AET)
{
    EdgeTableEntry *pETEchase;
    EdgeTableEntry *pETEinsert;
    EdgeTableEntry *pETEchaseBackTMP;
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
static void FASTCALL
REGION_FreeStorage(ScanLineListBlock *pSLLBlock)
{
    ScanLineListBlock   *tmpSLLBlock;

    while (pSLLBlock)
    {
        tmpSLLBlock = pSLLBlock->next;
        ExFreePool(pSLLBlock);
        pSLLBlock = tmpSLLBlock;
    }
}


/***********************************************************************
 *     REGION_PtsToRegion
 *
 *     Create an array of rectangles from a list of points.
 */
static int FASTCALL
REGION_PtsToRegion(
    int numFullPtBlocks,
    int iCurPtBlock,
    POINTBLOCK *FirstPtBlock,
    ROSRGNDATA *reg)
{
    RECTL *rects;
    POINT *pts;
    POINTBLOCK *CurPtBlock;
    int i;
    RECTL *extents, *temp;
    INT numRects;

    extents = &reg->rdh.rcBound;

    numRects = ((numFullPtBlocks * NUMPTSTOBUFFER) + iCurPtBlock) >> 1;

    if (!(temp = ExAllocatePoolWithTag(PagedPool, numRects * sizeof(RECT), TAG_REGION)))
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
        /* the loop uses 2 points per iteration */
        i = NUMPTSTOBUFFER >> 1;
        if (!numFullPtBlocks)
            i = iCurPtBlock >> 1;
        for (pts = CurPtBlock->pts; i--; pts += 2)
        {
            if (pts->x == pts[1].x)
                continue;
            if (numRects && pts->x == rects->left && pts->y == rects->bottom &&
                    pts[1].x == rects->right &&
                    (numRects == 1 || rects[-1].top != rects->top) &&
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
 *     REGION_CreateEdgeTable
 *
 *     This routine creates the edge table for
 *     scan converting polygons.
 *     The Edge Table (ET) looks like:
 *
 *    EdgeTable
 *     --------
 *    |  ymax  |        ScanLineLists
 *    |scanline|-->------------>-------------->...
 *     --------   |scanline|   |scanline|
 *                |edgelist|   |edgelist|
 *                ---------    ---------
 *                    |             |
 *                    |             |
 *                    V             V
 *              list of ETEs   list of ETEs
 *
 *     where ETE is an EdgeTableEntry data structure,
 *     and there is one ScanLineList per scanline at
 *     which an edge is initially entered.
 *
 */
static void FASTCALL
REGION_CreateETandAET(
    const ULONG *Count,
    INT nbpolygons,
    const POINT *pts,
    EdgeTable *ET,
    EdgeTableEntry *AET,
    EdgeTableEntry *pETEs,
    ScanLineListBlock *pSLLBlock
)
{
    const POINT *top, *bottom;
    const POINT *PrevPt, *CurrPt, *EndPt;
    INT poly, count;
    int iSLLBlock = 0;
    int dy;


    /*
     *  initialize the Active Edge Table
     */
    AET->next = (EdgeTableEntry *)NULL;
    AET->back = (EdgeTableEntry *)NULL;
    AET->nextWETE = (EdgeTableEntry *)NULL;
    AET->bres.minor_axis = SMALL_COORDINATE;

    /*
     *  initialize the Edge Table.
     */
    ET->scanlines.next = (ScanLineList *)NULL;
    ET->ymax = SMALL_COORDINATE;
    ET->ymin = LARGE_COORDINATE;
    pSLLBlock->next = (ScanLineListBlock *)NULL;

    EndPt = pts - 1;
    for (poly = 0; poly < nbpolygons; poly++)
    {
        count = Count[poly];
        EndPt += count;
        if (count < 2)
            continue;

        PrevPt = EndPt;

        /*
         *  for each vertex in the array of points.
         *  In this loop we are dealing with two vertices at
         *  a time -- these make up one edge of the polygon.
         */
        while (count--)
        {
            CurrPt = pts++;

            /*
             *  find out which point is above and which is below.
             */
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

            /*
             * don't add horizontal edges to the Edge table.
             */
            if (bottom->y != top->y)
            {
                pETEs->ymax = bottom->y-1;
                /* -1 so we don't get last scanline */

                /*
                 *  initialize integer edge algorithm
                 */
                dy = bottom->y - top->y;
                BRESINITPGONSTRUCT(dy, top->x, bottom->x, pETEs->bres);

                REGION_InsertEdgeInET(ET, pETEs, top->y, &pSLLBlock,
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

HRGN FASTCALL
IntCreatePolyPolygonRgn(
    POINT *Pts,
    PULONG Count,
    INT nbpolygons,
    INT mode
)
{
    HRGN hrgn;
    ROSRGNDATA *region;
    EdgeTableEntry *pAET;     /* Active Edge Table       */
    INT y;                    /* current scanline        */
    int iPts = 0;             /* number of pts in buffer */
    EdgeTableEntry *pWETE;    /* Winding Edge Table Entry*/
    ScanLineList *pSLL;       /* current scanLineList    */
    POINT *pts;               /* output buffer           */
    EdgeTableEntry *pPrevAET; /* ptr to previous AET     */
    EdgeTable ET;             /* header node for ET      */
    EdgeTableEntry AET;       /* header node for AET     */
    EdgeTableEntry *pETEs;    /* EdgeTableEntries pool   */
    ScanLineListBlock SLLBlock; /* header for scanlinelist */
    int fixWAET = FALSE;
    POINTBLOCK FirstPtBlock, *curPtBlock; /* PtBlock buffers    */
    POINTBLOCK *tmpPtBlock;
    int numFullPtBlocks = 0;
    INT poly, total;

    if (mode == 0 || mode > 2) return 0;

    if (!(region = REGION_AllocRgnWithHandle(nbpolygons)))
        return 0;
    hrgn = region->BaseObject.hHmgr;

    /* special case a rectangle */

    if (((nbpolygons == 1) && ((*Count == 4) ||
                               ((*Count == 5) && (Pts[4].x == Pts[0].x) && (Pts[4].y == Pts[0].y)))) &&
            (((Pts[0].y == Pts[1].y) &&
              (Pts[1].x == Pts[2].x) &&
              (Pts[2].y == Pts[3].y) &&
              (Pts[3].x == Pts[0].x)) ||
             ((Pts[0].x == Pts[1].x) &&
              (Pts[1].y == Pts[2].y) &&
              (Pts[2].x == Pts[3].x) &&
              (Pts[3].y == Pts[0].y))))
    {
        REGION_UnlockRgn(region);
        NtGdiSetRectRgn(hrgn, min(Pts[0].x, Pts[2].x), min(Pts[0].y, Pts[2].y),
                        max(Pts[0].x, Pts[2].x), max(Pts[0].y, Pts[2].y));
        return hrgn;
    }

    for (poly = total = 0; poly < nbpolygons; poly++)
        total += Count[poly];
    if (! (pETEs = ExAllocatePoolWithTag(PagedPool, sizeof(EdgeTableEntry) * total, TAG_REGION)) )
    {
        NtGdiDeleteObject(hrgn);
        return 0;
    }
    pts = FirstPtBlock.pts;
    REGION_CreateETandAET(Count, nbpolygons, Pts, &ET, &AET, pETEs, &SLLBlock);
    pSLL = ET.scanlines.next;
    curPtBlock = &FirstPtBlock;

    if (mode != WINDING)
    {
        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++)
        {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL != NULL && y == pSLL->scanline)
            {
                REGION_loadAET(&AET, pSLL->edgelist);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;

            /*
             *  for each active edge
             */
            while (pAET)
            {
                pts->x = pAET->bres.minor_axis,  pts->y = y;
                pts++, iPts++;

                /*
                 *  send out the buffer
                 */
                if (iPts == NUMPTSTOBUFFER)
                {
                    tmpPtBlock = ExAllocatePoolWithTag(PagedPool, sizeof(POINTBLOCK), TAG_REGION);
                    if (!tmpPtBlock)
                    {
                        DPRINT1("Can't alloc tPB\n");
                        ExFreePoolWithTag(pETEs, TAG_REGION);
                        return 0;
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
        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++)
        {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL != NULL && y == pSLL->scanline)
            {
                REGION_loadAET(&AET, pSLL->edgelist);
                REGION_computeWAET(&AET);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;
            pWETE = pAET;

            /*
             *  for each active edge
             */
            while (pAET)
            {
                /*
                 *  add to the buffer only those edges that
                 *  are in the Winding active edge table.
                 */
                if (pWETE == pAET)
                {
                    pts->x = pAET->bres.minor_axis,  pts->y = y;
                    pts++, iPts++;

                    /*
                     *  send out the buffer
                     */
                    if (iPts == NUMPTSTOBUFFER)
                    {
                        tmpPtBlock = ExAllocatePoolWithTag(PagedPool,
                                                           sizeof(POINTBLOCK), TAG_REGION);
                        if (!tmpPtBlock)
                        {
                            DPRINT1("Can't alloc tPB\n");
                            ExFreePoolWithTag(pETEs, TAG_REGION);
                            NtGdiDeleteObject(hrgn);
                            return 0;
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

            /*
             *  recompute the winding active edge table if
             *  we just resorted or have exited an edge.
             */
            if (REGION_InsertionSort(&AET) || fixWAET)
            {
                REGION_computeWAET(&AET);
                fixWAET = FALSE;
            }
        }
    }
    REGION_FreeStorage(SLLBlock.next);
    REGION_PtsToRegion(numFullPtBlocks, iPts, &FirstPtBlock, region);

    for (curPtBlock = FirstPtBlock.next; --numFullPtBlocks >= 0;)
    {
        tmpPtBlock = curPtBlock->next;
        ExFreePoolWithTag(curPtBlock, TAG_REGION);
        curPtBlock = tmpPtBlock;
    }
    ExFreePoolWithTag(pETEs, TAG_REGION);
    REGION_UnlockRgn(region);
    return hrgn;
}

/* EOF */
