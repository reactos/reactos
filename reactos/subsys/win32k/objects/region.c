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
/* $Id: region.c,v 1.54 2004/05/18 13:57:41 weiden Exp $ */
#include <w32k.h>
#include <win32k/float.h>

BOOL STDCALL
IntEngPaint(IN SURFOBJ *Surface,IN CLIPOBJ *ClipRegion,IN BRUSHOBJ *Brush,IN POINTL *BrushOrigin,
	 IN MIX  Mix);

// Internal Functions

#define EMPTY_REGION(pReg) { \
  (pReg)->rdh.nCount = 0; \
  (pReg)->rdh.rcBound.left = (pReg)->rdh.rcBound.top = 0; \
  (pReg)->rdh.rcBound.right = (pReg)->rdh.rcBound.bottom = 0; \
  (pReg)->rdh.iType = NULLREGION; \
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
 *   Check to see if there is enough memory in the present region.
 */
static inline int xmemcheck(ROSRGNDATA *reg, PRECT *rect, PRECT *firstrect ) {
	if ( (reg->rdh.nCount+1)*sizeof( RECT ) >= reg->rdh.nRgnSize ) {
		PRECT temp;
		temp = ExAllocatePoolWithTag( PagedPool, (2 * (reg->rdh.nRgnSize)), TAG_REGION);

		if (temp == 0)
		    return 0;
		RtlCopyMemory( temp, *firstrect, reg->rdh.nRgnSize );
		reg->rdh.nRgnSize *= 2;
		if (*firstrect != &reg->BuiltInRect)
		    ExFreePool( *firstrect );
		*firstrect = temp;
		*rect = (*firstrect)+reg->rdh.nCount;
    }
    return 1;
}

#define MEMCHECK(reg, rect, firstrect) xmemcheck(reg,&(rect),(LPRECT *)&(firstrect))

typedef void FASTCALL (*overlapProcp)(PROSRGNDATA, PRECT, PRECT, PRECT, PRECT, INT, INT);
typedef void FASTCALL (*nonOverlapProcp)(PROSRGNDATA, PRECT, PRECT, INT, INT);

// Number of points to buffer before sending them off to scanlines() :  Must be an even number
#define NUMPTSTOBUFFER 200

#define RGN_DEFAULT_RECTS	2

// used to allocate buffers for points and link the buffers together

typedef struct _POINTBLOCK {
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

   Data = RGNDATA_LockRgn(hRgn);
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

   RGNDATA_UnlockRgn(hRgn);
}
#endif /* NDEBUG */

static BOOL FASTCALL REGION_CopyRegion(PROSRGNDATA dst, PROSRGNDATA src)
{
  if(dst != src) //  don't want to copy to itself
  {
    if (dst->rdh.nRgnSize < src->rdh.nCount * sizeof(RECT))
    {
	  PRECT temp;

	  temp = ExAllocatePoolWithTag(PagedPool, src->rdh.nCount * sizeof(RECT), TAG_REGION );
	  if( !temp )
		return FALSE;

	  if( dst->Buffer && dst->Buffer != &dst->BuiltInRect )
	  	ExFreePool( dst->Buffer );	//free the old buffer
	  dst->Buffer = temp;
      dst->rdh.nRgnSize = src->rdh.nCount * sizeof(RECT);  //size of region buffer
    }
    dst->rdh.nCount = src->rdh.nCount;                 //number of rectangles present in Buffer
    dst->rdh.rcBound.left = src->rdh.rcBound.left;
    dst->rdh.rcBound.top = src->rdh.rcBound.top;
    dst->rdh.rcBound.right = src->rdh.rcBound.right;
    dst->rdh.rcBound.bottom = src->rdh.rcBound.bottom;
    dst->rdh.iType = src->rdh.iType;
    RtlCopyMemory(dst->Buffer, src->Buffer, (int)(src->rdh.nCount * sizeof(RECT)));
  }
  return TRUE;
}

static void FASTCALL REGION_SetExtents (ROSRGNDATA *pReg)
{
    RECT *pRect, *pRectEnd, *pExtents;

    if (pReg->rdh.nCount == 0)
    {
		pReg->rdh.rcBound.left = 0;
		pReg->rdh.rcBound.top = 0;
		pReg->rdh.rcBound.right = 0;
		pReg->rdh.rcBound.bottom = 0;
		pReg->rdh.iType = NULLREGION;
		return;
    }

    pExtents = &pReg->rdh.rcBound;
    pRect = (PRECT)pReg->Buffer;
    pRectEnd = (PRECT)pReg->Buffer + pReg->rdh.nCount - 1;

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
    pReg->rdh.iType = (1 == pReg->rdh.nCount ? SIMPLEREGION : COMPLEXREGION);
}


/***********************************************************************
 *           REGION_CropAndOffsetRegion
 */
static BOOL FASTCALL REGION_CropAndOffsetRegion(const PPOINT off, const PRECT rect, PROSRGNDATA rgnSrc, PROSRGNDATA rgnDst)
{
  if(!rect) // just copy and offset
  {
    PRECT xrect;
    if(rgnDst == rgnSrc)
    {
      if(off->x || off->y)
        xrect = (PRECT)rgnDst->Buffer;
      else
        return TRUE;
    }
    else{
      xrect = ExAllocatePoolWithTag(PagedPool, rgnSrc->rdh.nCount * sizeof(RECT), TAG_REGION);
	  if( rgnDst->Buffer && rgnDst->Buffer != &rgnDst->BuiltInRect )
	  	ExFreePool( rgnDst->Buffer ); //free the old buffer. will be assigned to xrect below.
	}

    if(xrect)
    {
      ULONG i;

      if(rgnDst != rgnSrc)
	  	RtlCopyMemory(rgnDst, rgnSrc, sizeof(ROSRGNDATA));

      if(off->x || off->y)
      {
        for(i = 0; i < rgnDst->rdh.nCount; i++)
        {
          xrect[i].left = ((PRECT)rgnSrc->Buffer + i)->left + off->x;
          xrect[i].right = ((PRECT)rgnSrc->Buffer + i)->right + off->x;
          xrect[i].top = ((PRECT)rgnSrc->Buffer + i)->top + off->y;
          xrect[i].bottom = ((PRECT)rgnSrc->Buffer + i)->bottom + off->y;
        }
        rgnDst->rdh.rcBound.left   += off->x;
        rgnDst->rdh.rcBound.right  += off->x;
        rgnDst->rdh.rcBound.top    += off->y;
        rgnDst->rdh.rcBound.bottom += off->y;
      }
      else
        RtlCopyMemory(xrect, rgnSrc->Buffer, rgnDst->rdh.nCount * sizeof(RECT));

	  rgnDst->Buffer = xrect;
    } else
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
    PRECT lpr, rpr;
    ULONG i, j, clipa, clipb;
    INT left = rgnSrc->rdh.rcBound.right + off->x;
    INT right = rgnSrc->rdh.rcBound.left + off->x;

    for(clipa = 0; ((PRECT)rgnSrc->Buffer + clipa)->bottom <= rect->top; clipa++)
	//region and rect intersect so we stop before clipa > rgnSrc->rdh.nCount
      ; // skip bands above the clipping rectangle

    for(clipb = clipa; clipb < rgnSrc->rdh.nCount; clipb++)
      if(((PRECT)rgnSrc->Buffer + clipb)->top >= rect->bottom)
        break;    // and below it

    // clipa - index of the first rect in the first intersecting band
    // clipb - index of the last rect in the last intersecting band

    if((rgnDst != rgnSrc) && (rgnDst->rdh.nCount < (i = (clipb - clipa))))
    {
	  PRECT temp;
	  temp = ExAllocatePoolWithTag( PagedPool, i * sizeof(RECT), TAG_REGION );
      if(!temp)
	      return FALSE;

	  if( rgnDst->Buffer && rgnDst->Buffer != &rgnDst->BuiltInRect )
	  	ExFreePool( rgnDst->Buffer ); //free the old buffer
      rgnDst->Buffer = temp;
      rgnDst->rdh.nCount = i;
	  rgnDst->rdh.nRgnSize = i * sizeof(RECT);
    }

    for(i = clipa, j = 0; i < clipb ; i++)
    {
      // i - src index, j - dst index, j is always <= i for obvious reasons

      lpr = (PRECT)rgnSrc->Buffer + i;

      if(lpr->left < rect->right && lpr->right > rect->left)
      {
	    rpr = (PRECT)rgnDst->Buffer + j;

        rpr->top = lpr->top + off->y;
        rpr->bottom = lpr->bottom + off->y;
        rpr->left = ((lpr->left > rect->left) ? lpr->left : rect->left) + off->x;
        rpr->right = ((lpr->right < rect->right) ? lpr->right : rect->right) + off->x;

        if(rpr->left < left) left = rpr->left;
        if(rpr->right > right) right = rpr->right;

        j++;
      }
    }

    if(j == 0) goto empty;

    rgnDst->rdh.rcBound.left = left;
    rgnDst->rdh.rcBound.right = right;

    left = rect->top + off->y;
    right = rect->bottom + off->y;

    rgnDst->rdh.nCount = j--;
    for(i = 0; i <= j; i++) // fixup top band
      if((rgnDst->Buffer + i)->top < left)
        (rgnDst->Buffer + i)->top = left;
      else
        break;

    for(i = j; i >= 0; i--) // fixup bottom band
      if(((PRECT)rgnDst->Buffer + i)->bottom > right)
        ((PRECT)rgnDst->Buffer + i)->bottom = right;
      else
        break;

    rgnDst->rdh.rcBound.top = ((PRECT)rgnDst->Buffer)->top;
    rgnDst->rdh.rcBound.bottom = ((PRECT)rgnDst->Buffer + j)->bottom;

    rgnDst->rdh.iType = (j >= 1) ? COMPLEXREGION : SIMPLEREGION;
  }

  return TRUE;

empty:
	if(!rgnDst->Buffer)
	{
	  rgnDst->Buffer = (PRECT)ExAllocatePoolWithTag(PagedPool, RGN_DEFAULT_RECTS * sizeof(RECT), TAG_REGION);
	  if(rgnDst->Buffer){
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
 * \param
 * hSrc: 	Region to crop and offset.
 * lpRect: 	Clipping rectangle. Can be NULL (no clipping).
 * lpPt:	Points to offset the cropped region. Can be NULL (no offset).
 *
 * hDst: Region to hold the result (a new region is created if it's 0).
 *       Allowed to be the same region as hSrc in which case everything
 *	 will be done in place, with no memory reallocations.
 *
 * \return	hDst if success, 0 otherwise.
 */
HRGN FASTCALL REGION_CropRgn(HRGN hDst, HRGN hSrc, const PRECT lpRect, PPOINT lpPt)
{
  PROSRGNDATA objSrc, rgnDst;
  HRGN hNewDst, hRet = NULL;
  GDIMULTILOCK Lock[2] = {{hDst, 0, GDI_OBJECT_TYPE_REGION}, {hSrc, 0, GDI_OBJECT_TYPE_REGION}};

  if( !hDst )
    {
      if( !( hNewDst = RGNDATA_AllocRgn(1) ) )
	{
	  return 0;
	}
      Lock[0].hObj = hNewDst;
    }

  if ( !GDIOBJ_LockMultipleObj(Lock, sizeof(Lock)/sizeof(Lock[0])) )
    {
      DPRINT1("GDIOBJ_LockMultipleObj() failed\n" );
      return 0;
    }
  rgnDst = Lock[0].pObj;
  objSrc = Lock[1].pObj;

  if( objSrc && rgnDst )
  {
    if(rgnDst)
    {
      POINT pt = { 0, 0 };

	  if(!lpPt)
	  	lpPt = &pt;

      if(REGION_CropAndOffsetRegion(lpPt, lpRect, objSrc, rgnDst) == FALSE)
	  { // ve failed cleanup and return
		hRet = NULL;
      }
      else{ // ve are fine. unlock the correct pointer and return correct handle
		hRet = Lock[0].hObj;
	  }
    }
  }
  GDIOBJ_UnlockMultipleObj(Lock, sizeof(Lock)/sizeof(Lock[0]));
  return hRet;
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
static INT FASTCALL REGION_Coalesce (
	     PROSRGNDATA pReg, /* Region to coalesce */
	     INT prevStart,  /* Index of start of previous band */
	     INT curStart    /* Index of start of current band */
) {
    RECT *pPrevRect;          /* Current rect in previous band */
    RECT *pCurRect;           /* Current rect in current band */
    RECT *pRegEnd;            /* End of region */
    INT curNumRects;          /* Number of rectangles in current band */
    INT prevNumRects;         /* Number of rectangles in previous band */
    INT bandtop;               /* top coordinate for current band */

    pRegEnd = (PRECT)pReg->Buffer + pReg->rdh.nCount;
    pPrevRect = (PRECT)pReg->Buffer + prevStart;
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in REGION_RegionOp
     * at the end when one region has been exhausted.
     */
    pCurRect = (PRECT)pReg->Buffer + curStart;
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
		curStart = pRegEnd - (PRECT)pReg->Buffer;
		pRegEnd = (PRECT)pReg->Buffer + pReg->rdh.nCount;
    }

    if ((curNumRects == prevNumRects) && (curNumRects != 0)) {
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
		    } while (prevNumRects != 0);

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
		    } while (curNumRects != 0);

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
				} while (pCurRect != pRegEnd);
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
    RECT *r1;                         /* Pointer into first region */
    RECT *r2;                         /* Pointer into 2d region */
    RECT *r1End;                      /* End of 1st region */
    RECT *r2End;                      /* End of 2d region */
    INT ybot;                         /* Bottom of intersection */
    INT ytop;                         /* Top of intersection */
    RECT *oldRects;                   /* Old rects for newReg */
    ULONG prevBand;                   /* Index of start of
						 * previous band in newReg */
    ULONG curBand;                    /* Index of start of current band in newReg */
    RECT *r1BandEnd;                  /* End of current band in r1 */
    RECT *r2BandEnd;                  /* End of current band in r2 */
    ULONG top;                        /* Top of non-overlapping band */
    ULONG bot;                        /* Bottom of non-overlapping band */

    /*
     * Initialization:
     *  set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = (PRECT)reg1->Buffer;
    r2 = (PRECT)reg2->Buffer;
    r1End = r1 + reg1->rdh.nCount;
    r2End = r2 + reg2->rdh.nCount;


    /*
     * newReg may be one of the src regions so we can't empty it. We keep a
     * note of its rects pointer (so that we can free them later), preserve its
     * extents and simply set numRects to zero.
     */

    oldRects = (PRECT)newReg->Buffer;
    newReg->rdh.nCount = 0;

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually.
     */
    newReg->rdh.nRgnSize = max(reg1->rdh.nCount,reg2->rdh.nCount) * 2 * sizeof(RECT);

    if (! (newReg->Buffer = ExAllocatePoolWithTag( PagedPool, newReg->rdh.nRgnSize, TAG_REGION )))
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
    } while ((r1 != r1End) && (r2 != r2End));

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
		    } while (r1 != r1End);
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
		} while (r2 != r2End);
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
		    RECT *prev_rects = (PRECT)newReg->Buffer;
		    newReg->Buffer = ExAllocatePoolWithTag( PagedPool, newReg->rdh.nCount*sizeof(RECT), TAG_REGION );

		    if (! newReg->Buffer)
				newReg->Buffer = prev_rects;
			else{
				newReg->rdh.nRgnSize = newReg->rdh.nCount*sizeof(RECT);
				RtlCopyMemory( newReg->Buffer, prev_rects, newReg->rdh.nRgnSize );
				if (prev_rects != &newReg->BuiltInRect)
					ExFreePool( prev_rects );
			}
		}
		else
		{
		    /*
		     * No point in doing the extra work involved in an Xrealloc if
		     * the region is empty
		     */
		    newReg->rdh.nRgnSize = sizeof(RECT);
		    if (newReg->Buffer != &newReg->BuiltInRect)
			ExFreePool( newReg->Buffer );
		    newReg->Buffer = ExAllocatePoolWithTag( PagedPool, sizeof(RECT), TAG_REGION );
			ASSERT( newReg->Buffer );
		}
    }

	if( newReg->rdh.nCount == 0 )
		newReg->rdh.iType = NULLREGION;
	else
		newReg->rdh.iType = (newReg->rdh.nCount > 1)? COMPLEXREGION : SIMPLEREGION;

	if (oldRects != &newReg->BuiltInRect)
		ExFreePool( oldRects );
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
REGION_IntersectO (
	PROSRGNDATA pReg,
	PRECT       r1,
	PRECT       r1End,
	PRECT       r2,
	PRECT       r2End,
	INT         top,
	INT         bottom
	)
{
    INT       left, right;
    RECT      *pNextRect;

    pNextRect = (PRECT)pReg->Buffer + pReg->rdh.nCount;

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
static void FASTCALL REGION_IntersectRegion(ROSRGNDATA *newReg, ROSRGNDATA *reg1,
				   ROSRGNDATA *reg2)
{
  /* check for trivial reject */
  if ( (!(reg1->rdh.nCount)) || (!(reg2->rdh.nCount))  ||
    (!EXTENTCHECK(&reg1->rdh.rcBound, &reg2->rdh.rcBound)))
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
	PRECT       r,
	PRECT       rEnd,
	INT         top,
	INT         bottom
	)
{
    RECT *pNextRect;

    pNextRect = (PRECT)pReg->Buffer + pReg->rdh.nCount;

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
	PRECT       r1,
	PRECT       r1End,
	PRECT       r2,
	PRECT       r2End,
	INT         top,
	INT         bottom
	)
{
    RECT *pNextRect;

    pNextRect = (PRECT)pReg->Buffer + pReg->rdh.nCount;

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
		} while (r1 != r1End);
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
static void FASTCALL REGION_UnionRegion(ROSRGNDATA *newReg, ROSRGNDATA *reg1,
			       ROSRGNDATA *reg2)
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

  REGION_RegionOp ( newReg, reg1, reg2, REGION_UnionO,
		  REGION_UnionNonO, REGION_UnionNonO );
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
REGION_SubtractNonO1 (
	PROSRGNDATA pReg,
	PRECT       r,
	PRECT       rEnd,
	INT         top,
	INT         bottom
	)
{
    RECT *pNextRect;

    pNextRect = (PRECT)pReg->Buffer + pReg->rdh.nCount;

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
REGION_SubtractO (
	PROSRGNDATA pReg,
	PRECT       r1,
	PRECT       r1End,
	PRECT       r2,
	PRECT       r2End,
	INT         top,
	INT         bottom
	)
{
    RECT *pNextRect;
    INT left;

    left = r1->left;
    pNextRect = (PRECT)pReg->Buffer + pReg->rdh.nCount;

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
static void FASTCALL REGION_SubtractRegion(ROSRGNDATA *regD, ROSRGNDATA *regM,
				                       ROSRGNDATA *regS )
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
static void FASTCALL REGION_XorRegion(ROSRGNDATA *dr, ROSRGNDATA *sra,
							ROSRGNDATA *srb)
{
  HRGN htra, htrb;
  ROSRGNDATA *tra, *trb;

  if(!(htra = RGNDATA_AllocRgn(sra->rdh.nCount + 1)))
    return;
  if(!(htrb = RGNDATA_AllocRgn(srb->rdh.nCount + 1)))
  {
    NtGdiDeleteObject( htra );
    return;
  }
  tra = RGNDATA_LockRgn( htra );
  if(!tra ){
    NtGdiDeleteObject( htra );
    NtGdiDeleteObject( htrb );
    return;
  }

  trb = RGNDATA_LockRgn( htrb );
  if( !trb ){
    RGNDATA_UnlockRgn( htra );
    NtGdiDeleteObject( htra );
    NtGdiDeleteObject( htrb );
    return;
  }

  REGION_SubtractRegion(tra,sra,srb);
  REGION_SubtractRegion(trb,srb,sra);
  REGION_UnionRegion(dr,tra,trb);
  RGNDATA_UnlockRgn( htra );
  RGNDATA_UnlockRgn( htrb );

  NtGdiDeleteObject( htra );
  NtGdiDeleteObject( htrb );
  return;
}


/*!
 * Adds a rectangle to a REGION
 */
void FASTCALL REGION_UnionRectWithRegion(const RECT *rect, ROSRGNDATA *rgn)
{
    ROSRGNDATA region;

    region.Buffer = &region.rdh.rcBound;
    region.rdh.nCount = 1;
    region.rdh.nRgnSize = sizeof( RECT );
    region.rdh.rcBound = *rect;
    REGION_UnionRegion(rgn, rgn, &region);
}


BOOL FASTCALL REGION_CreateFrameRgn(HRGN hDest, HRGN hSrc, INT x, INT y)
{
  PROSRGNDATA srcObj, destObj;
  PRECT rc;
  INT dx, dy;
  ULONG i;
  
  if(!(srcObj = (PROSRGNDATA)RGNDATA_LockRgn(hSrc)))
  {
    return FALSE;
  }
  if(!REGION_NOT_EMPTY(srcObj))
  {
    RGNDATA_UnlockRgn(hSrc);
    return FALSE;
  }
  if(!(destObj = (PROSRGNDATA)RGNDATA_LockRgn(hDest)))
  {
    RGNDATA_UnlockRgn(hSrc);
    return FALSE;
  }
  
  EMPTY_REGION(destObj);
  if(!REGION_CopyRegion(destObj, srcObj))
  {
    RGNDATA_UnlockRgn(hDest);
    RGNDATA_UnlockRgn(hSrc);
    return FALSE;
  }
  
  /* left-top */
  dx = x * 2;
  dy = y * 2;
  rc = (PRECT)srcObj->Buffer;
  for(i = 0; i < srcObj->rdh.nCount; i++)
  {
    rc->left += x;
    rc->top += y;
    rc->right += x;
    rc->bottom += y;
    rc++;
  }
  REGION_IntersectRegion(destObj, destObj, srcObj);
  
  /* right-top */
  rc = (PRECT)srcObj->Buffer;
  for(i = 0; i < srcObj->rdh.nCount; i++)
  {
    rc->left -= dx;
    rc->right -= dx;
    rc++;
  }
  REGION_IntersectRegion(destObj, destObj, srcObj);
  
  /* right-bottom */
  rc = (PRECT)srcObj->Buffer;
  for(i = 0; i < srcObj->rdh.nCount; i++)
  {
    rc->top -= dy;
    rc->bottom -= dy;
    rc++;
  }
  REGION_IntersectRegion(destObj, destObj, srcObj);
  
  /* left-bottom */
  rc = (PRECT)srcObj->Buffer;
  for(i = 0; i < srcObj->rdh.nCount; i++)
  {
    rc->left += dx;
    rc->right += dx;
    rc++;
  }
  REGION_IntersectRegion(destObj, destObj, srcObj);
  
  
  rc = (PRECT)srcObj->Buffer;
  for(i = 0; i < srcObj->rdh.nCount; i++)
  {
    rc->left -= x;
    rc->top += y;
    rc->right -= x;
    rc->bottom += y;
    rc++;
  }
  REGION_SubtractRegion(destObj, srcObj, destObj);
  
  RGNDATA_UnlockRgn(hDest);
  RGNDATA_UnlockRgn(hSrc);
  return TRUE;
}


BOOL FASTCALL REGION_LPTODP(HDC hdc, HRGN hDest, HRGN hSrc)
{
  RECT *pCurRect, *pEndRect;
  PROSRGNDATA srcObj = NULL;
  PROSRGNDATA destObj = NULL;

  DC * dc = DC_LockDc(hdc);
  RECT tmpRect;
  BOOL ret = FALSE;

  if(!dc)
    return ret;

  if(dc->w.MapMode == MM_TEXT) // Requires only a translation
  {
    if(NtGdiCombineRgn(hDest, hSrc, 0, RGN_COPY) == ERROR)
      goto done;

    NtGdiOffsetRgn(hDest, dc->vportOrgX - dc->wndOrgX, dc->vportOrgY - dc->wndOrgY);
    ret = TRUE;
    goto done;
  }

  if(!( srcObj = (PROSRGNDATA) RGNDATA_LockRgn( hSrc ) ))
        goto done;
  if(!( destObj = (PROSRGNDATA) RGNDATA_LockRgn( hDest ) ))
  {
    RGNDATA_UnlockRgn( hSrc );
    goto done;
  }
  EMPTY_REGION(destObj);

  pEndRect = (PRECT)srcObj->Buffer + srcObj->rdh.nCount;
  for(pCurRect = (PRECT)srcObj->Buffer; pCurRect < pEndRect; pCurRect++)
  {
    tmpRect = *pCurRect;
    tmpRect.left = XLPTODP(dc, tmpRect.left);
    tmpRect.top = YLPTODP(dc, tmpRect.top);
    tmpRect.right = XLPTODP(dc, tmpRect.right);
    tmpRect.bottom = YLPTODP(dc, tmpRect.bottom);

    if(tmpRect.left > tmpRect.right)
      { INT tmp = tmpRect.left; tmpRect.left = tmpRect.right; tmpRect.right = tmp; }
    if(tmpRect.top > tmpRect.bottom)
      { INT tmp = tmpRect.top; tmpRect.top = tmpRect.bottom; tmpRect.bottom = tmp; }

    REGION_UnionRectWithRegion(&tmpRect, destObj);
  }
  ret = TRUE;

  RGNDATA_UnlockRgn( hSrc );
  RGNDATA_UnlockRgn( hDest );

done:
  DC_UnlockDc( hdc );
  return ret;
}

HRGN FASTCALL RGNDATA_AllocRgn(INT n)
{
  HRGN hReg;
  PROSRGNDATA pReg;
  BOOL bRet;

  if ((hReg = (HRGN) GDIOBJ_AllocObj(sizeof(ROSRGNDATA), GDI_OBJECT_TYPE_REGION,
                                     (GDICLEANUPPROC) RGNDATA_InternalDelete)))
    {
      if (NULL != (pReg = RGNDATA_LockRgn(hReg)))
        {
          if (1 == n)
            {
              pReg->Buffer = &pReg->BuiltInRect;
            }
          else
            {
              pReg->Buffer = ExAllocatePoolWithTag(PagedPool, n * sizeof(RECT), TAG_REGION);
            }
          if (NULL != pReg->Buffer)
            {
              EMPTY_REGION(pReg);
              pReg->rdh.dwSize = sizeof(RGNDATAHEADER);
              pReg->rdh.nCount = n;
              pReg->rdh.nRgnSize = n*sizeof(RECT);

              bRet = RGNDATA_UnlockRgn(hReg);
              ASSERT(bRet);

              return hReg;
            }
        }
      else
        {
          RGNDATA_FreeRgn(hReg);
        }
    }

  return NULL;
}

BOOL FASTCALL RGNDATA_InternalDelete( PROSRGNDATA pRgn )
{
  ASSERT(pRgn);
  if(pRgn->Buffer && pRgn->Buffer != &pRgn->BuiltInRect)
    ExFreePool(pRgn->Buffer);
  return TRUE;
}

// NtGdi Exported Functions
INT
STDCALL
NtGdiCombineRgn(HRGN  hDest,
                    HRGN  hSrc1,
                    HRGN  hSrc2,
                    INT  CombineMode)
{
  INT result = ERROR;
  GDIMULTILOCK Lock[3] = {{hDest, 0, GDI_OBJECT_TYPE_REGION}, {hSrc1, 0, GDI_OBJECT_TYPE_REGION}, {hSrc2, 0, GDI_OBJECT_TYPE_REGION}};
  PROSRGNDATA destRgn, src1Rgn, src2Rgn;

  if ( !GDIOBJ_LockMultipleObj(Lock, sizeof(Lock)/sizeof(Lock[0])) )
    {
      DPRINT1("GDIOBJ_LockMultipleObj() failed\n" );
      return ERROR;
    }

  destRgn = (PROSRGNDATA) Lock[0].pObj;
  src1Rgn = (PROSRGNDATA) Lock[1].pObj;
  src2Rgn = (PROSRGNDATA) Lock[2].pObj;

  if( destRgn )
    {
      if( src1Rgn )
	  {
	    if (CombineMode == RGN_COPY)
	      {
		if( !REGION_CopyRegion(destRgn, src1Rgn) )
		  return ERROR;
		result = destRgn->rdh.iType;
	      }
	    else
	    {
	      if( src2Rgn )
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
		  result = destRgn->rdh.iType;
		}
	    }
	  }
    }
  else
    {
      DPRINT("NtGdiCombineRgn: hDest unavailable\n");
      result = ERROR;
    }
  GDIOBJ_UnlockMultipleObj(Lock, sizeof(Lock)/sizeof(Lock[0]));
  return result;
}

HRGN
STDCALL
NtGdiCreateEllipticRgn(INT Left,
                       INT Top,
                       INT Right,
                       INT Bottom)
{
   return NtGdiCreateRoundRectRgn(Left, Top, Right, Bottom,
      Right - Left, Bottom - Top);
}

HRGN
STDCALL
NtGdiCreateEllipticRgnIndirect(CONST PRECT Rect)
{
  RECT SafeRect;
  NTSTATUS Status;
  
  Status = MmCopyFromCaller(&SafeRect, Rect, sizeof(RECT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return NULL;
  }
  
  return NtGdiCreateRoundRectRgn(SafeRect.left, SafeRect.top, SafeRect.right, SafeRect.bottom,
                                 SafeRect.right - SafeRect.left, SafeRect.bottom - SafeRect.top);
}

HRGN FASTCALL
IntCreatePolyPolgonRgn(PPOINT pt,
                       PINT PolyCounts,
		       INT Count,
                       INT PolyFillMode)
{
  return (HRGN)0;
}

HRGN
STDCALL
NtGdiCreatePolygonRgn(CONST PPOINT  pt,
                      INT  Count,
                      INT  PolyFillMode)
{
   POINT *SafePoints;
   NTSTATUS Status;
   HRGN hRgn;
   
   
   if (pt == NULL || Count == 0 ||
       (PolyFillMode != WINDING && PolyFillMode != ALTERNATE))
   {
      /* Windows doesn't set a last error here */
      return (HRGN)0;
   }
   
   if (Count == 1)
   {
      /* can't create a region with only one point! */
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return (HRGN)0;
   }
   
   if (Count == 2)
   {
      /* Windows creates an empty region! */
      ROSRGNDATA *rgn;
      
      if(!(hRgn = RGNDATA_AllocRgn(1)))
      {
	 return (HRGN)0;
      }
      if(!(rgn = RGNDATA_LockRgn(hRgn)))
      {
        NtGdiDeleteObject(hRgn);
	return (HRGN)0;
      }
      
      EMPTY_REGION(rgn);
      
      RGNDATA_UnlockRgn(hRgn);
      return hRgn;
   }
   
   if (!(SafePoints = ExAllocatePool(PagedPool, Count * sizeof(POINT))))
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return (HRGN)0;
   }
   
   Status = MmCopyFromCaller(SafePoints, pt, Count * sizeof(POINT));
   if (!NT_SUCCESS(Status))
   {
      ExFreePool(SafePoints);
      SetLastNtError(Status);
      return (HRGN)0;
   }
   
   hRgn = IntCreatePolyPolgonRgn(SafePoints, &Count, 1, PolyFillMode);
   
   ExFreePool(SafePoints);
   return hRgn;
}

HRGN
STDCALL
NtGdiCreatePolyPolygonRgn(CONST PPOINT  pt,
                          CONST PINT  PolyCounts,
                          INT  Count,
                          INT  PolyFillMode)
{
   POINT *Safept;
   INT *SafePolyCounts;
   INT nPoints, nEmpty, nInvalid, i;
   HRGN hRgn;
   NTSTATUS Status;
   
   if (pt == NULL || PolyCounts == NULL || Count == 0 ||
       (PolyFillMode != WINDING && PolyFillMode != ALTERNATE))
   {
      /* Windows doesn't set a last error here */
      return (HRGN)0;
   }
   
   if (!(SafePolyCounts = ExAllocatePool(PagedPool, Count * sizeof(INT))))
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return (HRGN)0;
   }
   
   Status = MmCopyFromCaller(SafePolyCounts, PolyCounts, Count * sizeof(INT));
   if (!NT_SUCCESS(Status))
   {
      ExFreePool(SafePolyCounts);
      SetLastNtError(Status);
      return (HRGN)0;
   }
   
   /* validate poligons */
   nPoints = 0;
   nEmpty = 0;
   nInvalid = 0;
   for (i = 0; i < Count; i++)
   {
      if (SafePolyCounts[i] == 0)
      {
         nEmpty++;
      }
      if (SafePolyCounts[i] == 1)
      {
         nInvalid++;
      }
      nPoints += SafePolyCounts[i];
   }
   
   if (nEmpty == Count)
   {
      /* if all polygon counts are zero, return without setting a last error code. */
      ExFreePool(SafePolyCounts);
      return (HRGN)0;
   }
   if (nInvalid != 0)
   {
     /* if at least one poly count is 1, fail */
     ExFreePool(SafePolyCounts);
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return (HRGN)0;
   }
   
   /* copy points */
   if (!(Safept = ExAllocatePool(PagedPool, nPoints * sizeof(POINT))))
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return (HRGN)0;
   }
   
   Status = MmCopyFromCaller(Safept, pt, nPoints * sizeof(POINT));
   if (!NT_SUCCESS(Status))
   {
      ExFreePool(Safept);
      ExFreePool(SafePolyCounts);
      SetLastNtError(Status);
      return (HRGN)0;
   }
   
   /* now we're ready to calculate the region safely */
   hRgn = IntCreatePolyPolgonRgn(Safept, SafePolyCounts, Count, PolyFillMode);
   
   ExFreePool(Safept);
   ExFreePool(SafePolyCounts);
   return hRgn;
}

HRGN STDCALL
NtGdiCreateRectRgn(INT LeftRect, INT TopRect, INT RightRect, INT BottomRect)
{
   HRGN hRgn;

   /* Allocate region data structure with space for 1 RECT */
   if ((hRgn = RGNDATA_AllocRgn(1)))
   {
      if (NtGdiSetRectRgn(hRgn, LeftRect, TopRect, RightRect, BottomRect))
         return hRgn;
      NtGdiDeleteObject(hRgn);
   }

   SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
   return NULL;
}

HRGN STDCALL
NtGdiCreateRectRgnIndirect(CONST PRECT rc)
{
  RECT SafeRc;
  NTSTATUS Status;

  Status = MmCopyFromCaller(&SafeRc, rc, sizeof(RECT));
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  return(UnsafeIntCreateRectRgnIndirect(&SafeRc));
}

HRGN
STDCALL
NtGdiCreateRoundRectRgn(INT left, INT top, INT right, INT bottom,
   INT ellipse_width, INT ellipse_height)
{
    PROSRGNDATA obj;
    HRGN hrgn;
    int asq, bsq, d, xd, yd;
    RECT rect;

      /* Make the dimensions sensible */

    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    ellipse_width = abs(ellipse_width);
    ellipse_height = abs(ellipse_height);

      /* Check parameters */

    if (ellipse_width > right-left) ellipse_width = right-left;
    if (ellipse_height > bottom-top) ellipse_height = bottom-top;

      /* Check if we can do a normal rectangle instead */

    if ((ellipse_width < 2) || (ellipse_height < 2))
        return NtGdiCreateRectRgn( left, top, right, bottom );

      /* Create region */

    d = (ellipse_height < 128) ? ((3 * ellipse_height) >> 2) : 64;
    if (!(hrgn = RGNDATA_AllocRgn(d))) return 0;
    if (!(obj = RGNDATA_LockRgn(hrgn))) return 0;

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
	    UnsafeIntUnionRectWithRgn( obj, &rect );
	    rect.top = --bottom;
	    rect.bottom = rect.top + 1;
	    UnsafeIntUnionRectWithRgn( obj, &rect );
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
	UnsafeIntUnionRectWithRgn( obj, &rect );
	rect.top = --bottom;
	rect.bottom = rect.top + 1;
	UnsafeIntUnionRectWithRgn( obj, &rect );
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
	UnsafeIntUnionRectWithRgn( obj, &rect );
    }
    RGNDATA_UnlockRgn( hrgn );
    return hrgn;
}

BOOL
STDCALL
NtGdiEqualRgn(HRGN  hSrcRgn1,
                   HRGN  hSrcRgn2)
{
  PROSRGNDATA rgn1, rgn2;
  PRECT tRect1, tRect2;
  ULONG i;
  BOOL bRet = FALSE;

  if( !(rgn1 = RGNDATA_LockRgn(hSrcRgn1)))
	return ERROR;

  if( !(rgn2 = RGNDATA_LockRgn(hSrcRgn2))){
	RGNDATA_UnlockRgn( hSrcRgn1 );
	return ERROR;
  }

  if(rgn1->rdh.nCount != rgn2->rdh.nCount ||
     	rgn1->rdh.nCount == 0 ||
     	rgn1->rdh.rcBound.left   != rgn2->rdh.rcBound.left ||
     	rgn1->rdh.rcBound.right  != rgn2->rdh.rcBound.right ||
     	rgn1->rdh.rcBound.top    != rgn2->rdh.rcBound.top ||
     	rgn1->rdh.rcBound.bottom != rgn2->rdh.rcBound.bottom)
	goto exit;

  tRect1 = (PRECT)rgn1->Buffer;
  tRect2 = (PRECT)rgn2->Buffer;

  if( !tRect1 || !tRect2 )
	goto exit;

  for(i=0; i < rgn1->rdh.nCount; i++)
  {
    if(tRect1[i].left   != tRect2[i].left ||
       	tRect1[i].right  != tRect2[i].right ||
       	tRect1[i].top    != tRect2[i].top ||
       	tRect1[i].bottom != tRect2[i].bottom)
	   goto exit;;
  }
  bRet = TRUE;

exit:
  RGNDATA_UnlockRgn( hSrcRgn1 );
  RGNDATA_UnlockRgn( hSrcRgn2 );
  return bRet;
}

HRGN
STDCALL
NtGdiExtCreateRegion(CONST PXFORM  Xform,
                          DWORD  Count,
                          CONST PROSRGNDATA  RgnData)
{
  HRGN hRgn;

  // FIXME: Apply Xform transformation to the regional data
  if(Xform != NULL) {

  }

  return hRgn;
}

BOOL
STDCALL
NtGdiFillRgn(HDC hDC, HRGN hRgn, HBRUSH hBrush)
{
  HBRUSH oldhBrush;
  PROSRGNDATA rgn;
  PRECT r;

  if (NULL == (rgn = RGNDATA_LockRgn(hRgn)))
    {
      return FALSE;
    }

  if (NULL == (oldhBrush = NtGdiSelectObject(hDC, hBrush)))
    {
      RGNDATA_UnlockRgn(hRgn);
      return FALSE;
    }

  for (r = rgn->Buffer; r < rgn->Buffer + rgn->rdh.nCount; r++)
    {
      NtGdiPatBlt(hDC, r->left, r->top, r->right - r->left, r->bottom - r->top, PATCOPY);
    }

  NtGdiSelectObject(hDC, oldhBrush);
  RGNDATA_UnlockRgn( hRgn );

  return TRUE;
}

BOOL
STDCALL
NtGdiFrameRgn(HDC hDC, HRGN  hRgn, HBRUSH  hBrush, INT  Width, INT  Height)
{
  HRGN FrameRgn;
  BOOL Ret;
  
  if(!(FrameRgn = NtGdiCreateRectRgn(0, 0, 0, 0)))
  {
    return FALSE;
  }
  if(!REGION_CreateFrameRgn(FrameRgn, hRgn, Width, Height))
  {
    NtGdiDeleteObject(FrameRgn);
    return FALSE;
  }
  
  Ret = NtGdiFillRgn(hDC, FrameRgn, hBrush);
  
  NtGdiDeleteObject(FrameRgn);
  return Ret;
}

INT FASTCALL
UnsafeIntGetRgnBox(PROSRGNDATA  Rgn,
		    LPRECT  pRect)
{
  DWORD ret;

  if (Rgn)
    {
      *pRect = Rgn->rdh.rcBound;
      ret = Rgn->rdh.iType;

      return ret;
    }
  return 0; //if invalid region return zero
}


INT STDCALL
NtGdiGetRgnBox(HRGN  hRgn,
	      LPRECT  pRect)
{
  PROSRGNDATA  Rgn;
  RECT SafeRect;
  DWORD ret;

  if (!(Rgn = RGNDATA_LockRgn(hRgn)))
    {
      return ERROR;
    }

  ret = UnsafeIntGetRgnBox(Rgn, &SafeRect);
  RGNDATA_UnlockRgn(hRgn);
  if (ERROR == ret)
    {
      return ret;
    }

  if (!NT_SUCCESS(MmCopyToCaller(pRect, &SafeRect, sizeof(RECT))))
    {
      return ERROR;
    }

  return ret;
}

BOOL
STDCALL
NtGdiInvertRgn(HDC  hDC,
                    HRGN  hRgn)
{
  PROSRGNDATA RgnData;
  ULONG i;
  PRECT rc;
  
  if(!(RgnData = RGNDATA_LockRgn(hRgn)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  rc = (PRECT)RgnData->Buffer;
  for(i = 0; i < RgnData->rdh.nCount; i++)
  {
    
    if(!NtGdiPatBlt(hDC, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, DSTINVERT))
    {
      RGNDATA_UnlockRgn(hRgn);
      return FALSE;
    }
    rc++;
  }
  
  RGNDATA_UnlockRgn(hRgn);
  return TRUE;
}

INT
STDCALL
NtGdiOffsetRgn(HRGN  hRgn,
                   INT  XOffset,
                   INT  YOffset)
{
  PROSRGNDATA rgn = RGNDATA_LockRgn(hRgn);
  INT ret;

  DPRINT("NtGdiOffsetRgn: hRgn %d Xoffs %d Yoffs %d rgn %x\n", hRgn, XOffset, YOffset, rgn );

  if( !rgn ){
	  DPRINT("NtGdiOffsetRgn: hRgn error\n");
	  return ERROR;
  }

  if(XOffset || YOffset) {
    int nbox = rgn->rdh.nCount;
	PRECT pbox = (PRECT)rgn->Buffer;

    if(nbox && pbox) {
      while(nbox--) {
        pbox->left += XOffset;
        pbox->right += XOffset;
        pbox->top += YOffset;
        pbox->bottom += YOffset;
        pbox++;
      }
      rgn->rdh.rcBound.left += XOffset;
      rgn->rdh.rcBound.right += XOffset;
      rgn->rdh.rcBound.top += YOffset;
      rgn->rdh.rcBound.bottom += YOffset;
    }
  }
  ret = rgn->rdh.iType;
  RGNDATA_UnlockRgn( hRgn );
  return ret;
}

BOOL
STDCALL
NtGdiPaintRgn(HDC  hDC,
                   HRGN  hRgn)
{
  //RECT box;
  HRGN tmpVisRgn; //, prevVisRgn;
  DC *dc = DC_LockDc(hDC);
  PROSRGNDATA visrgn;
  CLIPOBJ* ClipRegion;
  BOOL bRet = FALSE;
  PGDIBRUSHOBJ pBrush;
  POINTL BrushOrigin;
  SURFOBJ	*SurfObj;

  if( !dc )
	return FALSE;

  if(!(tmpVisRgn = NtGdiCreateRectRgn(0, 0, 0, 0))){
	DC_UnlockDc( hDC );
  	return FALSE;
  }

/* ei enable later
  // Transform region into device co-ords
  if(!REGION_LPTODP(hDC, tmpVisRgn, hRgn) || NtGdiOffsetRgn(tmpVisRgn, dc->w.DCOrgX, dc->w.DCOrgY) == ERROR) {
    NtGdiDeleteObject( tmpVisRgn );
	DC_UnlockDc( hDC );
    return FALSE;
  }
*/
  /* enable when clipping is implemented
  NtGdiCombineRgn(tmpVisRgn, tmpVisRgn, dc->w.hGCClipRgn, RGN_AND);
  */

  //visrgn = RGNDATA_LockRgn(tmpVisRgn);
  visrgn = RGNDATA_LockRgn(hRgn);
  if (visrgn == NULL)
  {
  	DC_UnlockDc( hDC );
    return FALSE;
  }

  ClipRegion = IntEngCreateClipRegion (
	  visrgn->rdh.nCount, (PRECTL)visrgn->Buffer, (PRECTL)&visrgn->rdh.rcBound );
  ASSERT( ClipRegion );
  pBrush = BRUSHOBJ_LockBrush(dc->w.hBrush);
  ASSERT(pBrush);
  BrushOrigin.x = dc->w.brushOrgX;
  BrushOrigin.y = dc->w.brushOrgY;
  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);

  bRet = IntEngPaint(SurfObj,
	 ClipRegion,
	 &pBrush->BrushObject,
	 &BrushOrigin,
	 0xFFFF);//FIXME:don't know what to put here

  RGNDATA_UnlockRgn( tmpVisRgn );

  // Fill the region
  DC_UnlockDc( hDC );
  return TRUE;
}

BOOL
STDCALL
NtGdiPtInRegion(HRGN  hRgn,
                     INT  X,
                     INT  Y)
{
  PROSRGNDATA rgn;
  ULONG i;
  PRECT r;

  if(!(rgn = RGNDATA_LockRgn(hRgn) ) )
	  return FALSE;
  
  if(rgn->rdh.nCount > 0 && INRECT(rgn->rdh.rcBound, X, Y)){
    r = (PRECT) rgn->Buffer;
    for(i = 0; i < rgn->rdh.nCount; i++) {
      if(INRECT(*r, X, Y)){
	RGNDATA_UnlockRgn(hRgn);
	return TRUE;
      }
      r++;
    }
  }
  RGNDATA_UnlockRgn(hRgn);
  return FALSE;
}

BOOL
FASTCALL
UnsafeIntRectInRegion(PROSRGNDATA Rgn,
                      CONST LPRECT rc)
{
  PRECT pCurRect, pRectEnd;

  // this is (just) a useful optimization
  if((Rgn->rdh.nCount > 0) && EXTENTCHECK(&Rgn->rdh.rcBound, rc))
  {
    for (pCurRect = (PRECT)Rgn->Buffer, pRectEnd = pCurRect + Rgn->rdh.nCount; pCurRect < pRectEnd; pCurRect++)
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
STDCALL
NtGdiRectInRegion(HRGN  hRgn,
                       CONST LPRECT  unsaferc)
{
  PROSRGNDATA Rgn;
  RECT rc;
  BOOL Ret;
  
  if(!(Rgn = RGNDATA_LockRgn(hRgn)))
  {
    return ERROR;
  }

  if (!NT_SUCCESS(MmCopyFromCaller(&rc, unsaferc, sizeof(RECT))))
    {
      RGNDATA_UnlockRgn(hRgn);
      DPRINT1("NtGdiRectInRegion: bogus rc\n");
      return ERROR;
    }
  
  Ret = UnsafeIntRectInRegion(Rgn, &rc);
  RGNDATA_UnlockRgn(hRgn);
  return Ret;
}

BOOL
STDCALL
NtGdiSetRectRgn(HRGN  hRgn,
                     INT  LeftRect,
                     INT  TopRect,
                     INT  RightRect,
                     INT  BottomRect)
{
  PROSRGNDATA rgn;
  PRECT firstRect;



  if( !( rgn = RGNDATA_LockRgn(hRgn) ) )
	  return 0; //per documentation

  if (LeftRect > RightRect) { INT tmp = LeftRect; LeftRect = RightRect; RightRect = tmp; }
  if (TopRect > BottomRect) { INT tmp = TopRect; TopRect = BottomRect; BottomRect = tmp; }

  if((LeftRect != RightRect) && (TopRect != BottomRect))
  {
    firstRect = (PRECT)rgn->Buffer;
	ASSERT( firstRect );
	firstRect->left = rgn->rdh.rcBound.left = LeftRect;
    firstRect->top = rgn->rdh.rcBound.top = TopRect;
    firstRect->right = rgn->rdh.rcBound.right = RightRect;
    firstRect->bottom = rgn->rdh.rcBound.bottom = BottomRect;
    rgn->rdh.nCount = 1;
    rgn->rdh.iType = SIMPLEREGION;
  } else
    EMPTY_REGION(rgn);

  RGNDATA_UnlockRgn( hRgn );
  return TRUE;
}

HRGN STDCALL
NtGdiUnionRectWithRgn(HRGN hDest, CONST PRECT UnsafeRect)
{
  RECT SafeRect;
  PROSRGNDATA Rgn;
  
  if(!(Rgn = (PROSRGNDATA)RGNDATA_UnlockRgn(hDest)))
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return NULL;
  }

  if (! NT_SUCCESS(MmCopyFromCaller(&SafeRect, UnsafeRect, sizeof(RECT))))
    {
      RGNDATA_UnlockRgn(hDest);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
    }
  
  UnsafeIntUnionRectWithRgn(Rgn, &SafeRect);
  RGNDATA_UnlockRgn(hDest);
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
DWORD STDCALL NtGdiGetRegionData(HRGN hrgn, DWORD count, LPRGNDATA rgndata)
{
    DWORD size;
    PROSRGNDATA obj = RGNDATA_LockRgn( hrgn );

    if(!obj)
		return 0;

    size = obj->rdh.nCount * sizeof(RECT);
    if(count < (size + sizeof(RGNDATAHEADER)) || rgndata == NULL)
    {
        RGNDATA_UnlockRgn( hrgn );
		if (rgndata) /* buffer is too small, signal it by return 0 */
		    return 0;
		else		/* user requested buffer size with rgndata NULL */
		    return size + sizeof(RGNDATAHEADER);
    }

	//first we copy the header then we copy buffer
	if( !NT_SUCCESS( MmCopyToCaller( rgndata, obj, sizeof( RGNDATAHEADER )))){
		RGNDATA_UnlockRgn( hrgn );
		return 0;
	}
	if( !NT_SUCCESS( MmCopyToCaller( (char*)rgndata+sizeof( RGNDATAHEADER ), obj->Buffer, size ))){
		RGNDATA_UnlockRgn( hrgn );
		return 0;
	}

    RGNDATA_UnlockRgn( hrgn );
    return size + sizeof(RGNDATAHEADER);
}
/* EOF */
