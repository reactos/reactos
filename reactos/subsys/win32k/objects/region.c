#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/float.h>
#include <win32k/dc.h>
#include <win32k/bitmaps.h>
#include <win32k/region.h>
#include <win32k/cliprgn.h>
#include <win32k/brush.h>
#include <include/rect.h>


// #define NDEBUG
#include <win32k/debug1.h>

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
static inline int xmemcheck(ROSRGNDATA *reg, LPRECT *rect, LPRECT *firstrect ) {
	if ( (reg->rdh.nCount+1)*sizeof( RECT ) >= reg->rdh.nRgnSize ) {
		PRECT temp;
		temp = ExAllocatePool( PagedPool, (2 * (reg->rdh.nRgnSize)));

		if (temp == 0)
		    return 0;
		RtlCopyMemory( temp, *firstrect, reg->rdh.nRgnSize );
		reg->rdh.nRgnSize *= 2;
		ExFreePool( *firstrect );
		*firstrect = temp;
		*rect = (*firstrect)+reg->rdh.nCount;
    }
    return 1;
}

#define MEMCHECK(reg, rect, firstrect) xmemcheck(reg,&(rect),&(firstrect))

typedef void (*voidProcp)();

// Number of points to buffer before sending them off to scanlines() :  Must be an even number
#define NUMPTSTOBUFFER 200

#define RGN_DEFAULT_RECTS	2

// used to allocate buffers for points and link the buffers together

typedef struct _POINTBLOCK {
  POINT pts[NUMPTSTOBUFFER];
  struct _POINTBLOCK *next;
} POINTBLOCK;

static BOOL REGION_CopyRegion(PROSRGNDATA dst, PROSRGNDATA src)
{
  if(dst != src) //  don't want to copy to itself
  {
    if (dst->rdh.nRgnSize < src->rdh.nCount * sizeof(RECT))
    {
	  PCHAR temp;

	  temp = ExAllocatePool(PagedPool, src->rdh.nCount * sizeof(RECT) );
	  if( !temp )
		return FALSE;

	  if( dst->Buffer )
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

static void REGION_SetExtents (ROSRGNDATA *pReg)
{
    RECT *pRect, *pRectEnd, *pExtents;

    if (pReg->rdh.nCount == 0)
    {
		pReg->rdh.rcBound.left = 0;
		pReg->rdh.rcBound.top = 0;
		pReg->rdh.rcBound.right = 0;
		pReg->rdh.rcBound.bottom = 0;
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
}


/***********************************************************************
 *           REGION_CropAndOffsetRegion
 */
static BOOL REGION_CropAndOffsetRegion(const PPOINT off, const PRECT rect, PROSRGNDATA rgnSrc, PROSRGNDATA rgnDst)
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
      xrect = ExAllocatePool(PagedPool, rgnSrc->rdh.nCount * sizeof(RECT));
	  if( rgnDst->Buffer )
	  	ExFreePool( rgnDst->Buffer ); //free the old buffer. will be assigned to xrect below.
	}

    if(xrect)
    {
      INT i;

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

	  rgnDst->Buffer = (char*)xrect;
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
    INT i, j, clipa, clipb;
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
	  PCHAR temp;
	  temp = ExAllocatePool( PagedPool, i * sizeof(RECT) );
      if(!temp)
	      return FALSE;

	  if( rgnDst->Buffer )
	  	ExFreePool( rgnDst->Buffer ); //free the old buffer
      (PRECT)rgnDst->Buffer = temp;
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
      if(((PRECT)rgnDst->Buffer + i)->top < left)
        ((PRECT)rgnDst->Buffer + i)->top = left;
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
	  rgnDst->Buffer = (char*)ExAllocatePool(PagedPool, RGN_DEFAULT_RECTS * sizeof(RECT));
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

/***********************************************************************
 *           REGION_CropRgn
 *
 *
 * hSrc: 	Region to crop and offset.
 * lpRect: 	Clipping rectangle. Can be NULL (no clipping).
 * lpPt:	Points to offset the cropped region. Can be NULL (no offset).
 *
 * hDst: Region to hold the result (a new region is created if it's 0).
 *       Allowed to be the same region as hSrc in which case everything
 *	 will be done in place, with no memory reallocations.
 *
 * Returns: hDst if success, 0 otherwise.
 */
HRGN REGION_CropRgn(HRGN hDst, HRGN hSrc, const PRECT lpRect, PPOINT lpPt)
{
  PROSRGNDATA objSrc = RGNDATA_LockRgn(hSrc);
  HRGN hNewDst;

  if(objSrc)
  {
	PROSRGNDATA rgnDst;

    if(hDst)
    {
      if(!(rgnDst = RGNDATA_LockRgn(hDst)))
      {
        hDst = 0;
        goto done;
      }
    }
	else{
	  if( !( hNewDst = RGNDATA_AllocRgn(1) ) ){
		RGNDATA_UnlockRgn( hSrc );
		return 0;
	  }

      if(!(rgnDst = RGNDATA_LockRgn(hNewDst)))
      {
        RGNDATA_FreeRgn( hNewDst );
		RGNDATA_UnlockRgn( hSrc );
        return 0;
      }
	}

    if(rgnDst)
    {
      POINT pt = { 0, 0 };

	  if(!lpPt)
	  	lpPt = &pt;

      if(REGION_CropAndOffsetRegion(lpPt, lpRect, objSrc, rgnDst) == FALSE)
	  { // ve failed cleanup and return
		RGNDATA_UnlockRgn( hSrc );

		if(hDst) // unlock new region if allocated
		  RGNDATA_UnlockRgn( hDst );
		else
		  RGNDATA_UnlockRgn( hNewDst );

		return 0;
      }
      else{ // ve are fine. unlock the correct pointer and return correct handle
		RGNDATA_UnlockRgn( hSrc );

	  	if(hDst == 0){
			RGNDATA_UnlockRgn( hNewDst );
			return hNewDst;
		}
		else {
			RGNDATA_UnlockRgn( hDst );
			return hDst;
		}
	  }
    }
done:
	RGNDATA_UnlockRgn( hSrc );
  }
  return 0;
}

/***********************************************************************
 *           REGION_Coalesce
 *
 *      Attempt to merge the rects in the current band with those in the
 *      previous one. Used only by REGION_RegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their bottom fields
 *            altered.
 *          - pReg->numRects will be decreased.
 *
 */
static INT REGION_Coalesce (
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

/***********************************************************************
 *           REGION_RegionOp
 *
 *      Apply an operation to two regions. Called by REGION_Union,
 *      REGION_Inverse, REGION_Subtract, REGION_Intersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 * Notes:
 *      The idea behind this function is to view the two regions as sets.
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
static void REGION_RegionOp(
	    ROSRGNDATA *newReg, /* Place to store result */
	    ROSRGNDATA *reg1,   /* First region in operation */
        ROSRGNDATA *reg2,   /* 2nd region in operation */
	    void (*overlapFunc)(),     /* Function to call for over-lapping bands */
	    void (*nonOverlap1Func)(), /* Function to call for non-overlapping bands in region 1 */
	    void (*nonOverlap2Func)()  /* Function to call for non-overlapping bands in region 2 */
) {
    RECT *r1;                         /* Pointer into first region */
    RECT *r2;                         /* Pointer into 2d region */
    RECT *r1End;                      /* End of 1st region */
    RECT *r2End;                      /* End of 2d region */
    INT ybot;                         /* Bottom of intersection */
    INT ytop;                         /* Top of intersection */
    RECT *oldRects;                   /* Old rects for newReg */
    INT prevBand;                     /* Index of start of
						 * previous band in newReg */
    INT curBand;                      /* Index of start of current
						 * band in newReg */
    RECT *r1BandEnd;                  /* End of current band in r1 */
    RECT *r2BandEnd;                  /* End of current band in r2 */
    INT top;                          /* Top of non-overlapping band */
    INT bot;                          /* Bottom of non-overlapping band */

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

    if (! (newReg->Buffer = ExAllocatePool( PagedPool, newReg->rdh.nRgnSize )))
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

		    if ((top != bot) && (nonOverlap1Func != (void (*)())NULL))
		    {
				(* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
		    }

		    ytop = r2->top;
		}
		else if (r2->top < r1->top)
		{
		    top = max(r2->top,ybot);
		    bot = min(r2->bottom,r1->top);

		    if ((top != bot) && (nonOverlap2Func != (void (*)())NULL))
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
		if (nonOverlap1Func != (void (*)())NULL)
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
    else if ((r2 != r2End) && (nonOverlap2Func != (void (*)())NULL))
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
    if ((newReg->rdh.nCount*sizeof(RECT) < 2*newReg->rdh.nRgnSize && (newReg->rdh.nCount > 2)))
    {
		if (REGION_NOT_EMPTY(newReg))
		{
		    RECT *prev_rects = (PRECT)newReg->Buffer;
		    newReg->Buffer = ExAllocatePool( PagedPool, newReg->rdh.nCount*sizeof(RECT) );

		    if (! newReg->Buffer)
				newReg->Buffer = (char*)prev_rects;
			else{
				newReg->rdh.nRgnSize = newReg->rdh.nCount*sizeof(RECT);
				RtlCopyMemory( newReg->Buffer, prev_rects, newReg->rdh.nRgnSize );
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
		    ExFreePool( newReg->Buffer );
		    newReg->Buffer = ExAllocatePool( PagedPool, sizeof(RECT) );
			ASSERT( newReg->Buffer );
		}
    }

	if( newReg->rdh.nCount == 0 )
		newReg->rdh.iType = NULLREGION;
	else
		newReg->rdh.iType = (newReg->rdh.nCount > 1)? COMPLEXREGION : SIMPLEREGION;

	ExFreePool( oldRects );
    return;
}

/***********************************************************************
 *          Region Intersection
 ***********************************************************************/


/***********************************************************************
 *	     REGION_IntersectO
 *
 * Handle an overlapping band for REGION_Intersect.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles may be added to the region.
 *
 */
static void REGION_IntersectO(ROSRGNDATA *pReg,  RECT *r1, RECT *r1End,
		RECT *r2, RECT *r2End, INT top, INT bottom)

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
static void REGION_IntersectRegion(ROSRGNDATA *newReg, ROSRGNDATA *reg1,
				   ROSRGNDATA *reg2)
{
   /* check for trivial reject */
    if ( (!(reg1->rdh.nCount)) || (!(reg2->rdh.nCount))  ||
		(!EXTENTCHECK(&reg1->rdh.rcBound, &reg2->rdh.rcBound)))
		newReg->rdh.nCount = 0;
    else
		REGION_RegionOp (newReg, reg1, reg2,
	 		(voidProcp) REGION_IntersectO, (voidProcp) NULL, (voidProcp) NULL);

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

/***********************************************************************
 *	     REGION_UnionNonO
 *
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg->numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 */
static void REGION_UnionNonO (ROSRGNDATA *pReg, RECT *r, RECT *rEnd,
			      INT top, INT bottom)
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

/***********************************************************************
 *	     REGION_UnionO
 *
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles are overwritten in pReg->rects and pReg->numRects will
 *      be changed.
 *
 */
static void REGION_UnionO (ROSRGNDATA *pReg, RECT *r1, RECT *r1End,
			   RECT *r2, RECT *r2End, INT top, INT bottom)
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
static void REGION_UnionRegion(ROSRGNDATA *newReg, ROSRGNDATA *reg1,
			       ROSRGNDATA *reg2)
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->rdh.nCount)) )
    {
		if (newReg != reg2)
		    REGION_CopyRegion(newReg, reg2);
		return;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->rdh.nCount))
    {
		if (newReg != reg1)
		    REGION_CopyRegion(newReg, reg1);
		return;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if ((reg1->rdh.nCount == 1) &&
		(reg1->rdh.rcBound.left <= reg2->rdh.rcBound.left) &&
		(reg1->rdh.rcBound.top <= reg2->rdh.rcBound.top) &&
		(reg1->rdh.rcBound.right >= reg2->rdh.rcBound.right) &&
		(reg1->rdh.rcBound.bottom >= reg2->rdh.rcBound.bottom))
    {
		if (newReg != reg1)
		    REGION_CopyRegion(newReg, reg1);
		return;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if ((reg2->rdh.nCount == 1) &&
		(reg2->rdh.rcBound.left <= reg1->rdh.rcBound.left) &&
		(reg2->rdh.rcBound.top <= reg1->rdh.rcBound.top) &&
		(reg2->rdh.rcBound.right >= reg1->rdh.rcBound.right) &&
		(reg2->rdh.rcBound.bottom >= reg1->rdh.rcBound.bottom))
    {
		if (newReg != reg2)
		    REGION_CopyRegion(newReg, reg2);
		return;
    }

    REGION_RegionOp (newReg, reg1, reg2, (voidProcp) REGION_UnionO,
		(voidProcp) REGION_UnionNonO, (voidProcp) REGION_UnionNonO);
    newReg->rdh.rcBound.left = min(reg1->rdh.rcBound.left, reg2->rdh.rcBound.left);
    newReg->rdh.rcBound.top = min(reg1->rdh.rcBound.top, reg2->rdh.rcBound.top);
    newReg->rdh.rcBound.right = max(reg1->rdh.rcBound.right, reg2->rdh.rcBound.right);
    newReg->rdh.rcBound.bottom = max(reg1->rdh.rcBound.bottom, reg2->rdh.rcBound.bottom);
}

/***********************************************************************
 *	     Region Subtraction
 ***********************************************************************/

/***********************************************************************
 *	     REGION_SubtractNonO1
 *
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may be affected.
 *
 */
static void REGION_SubtractNonO1 (ROSRGNDATA *pReg, RECT *r, RECT *rEnd,
		INT top, INT bottom)
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


/***********************************************************************
 *	     REGION_SubtractO
 *
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may have rectangles added to it.
 *
 */
static void REGION_SubtractO (ROSRGNDATA *pReg, RECT *r1, RECT *r1End,
		RECT *r2, RECT *r2End, INT top, INT bottom)
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

/***********************************************************************
 *	     REGION_SubtractRegion
 *
 *      Subtract regS from regM and leave the result in regD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *      TRUE.
 *
 * Side Effects:
 *      regD is overwritten.
 *
 */
static void REGION_SubtractRegion(ROSRGNDATA *regD, ROSRGNDATA *regM,
				                       ROSRGNDATA *regS )
{
   /* check for trivial reject */
    if ( (!(regM->rdh.nCount)) || (!(regS->rdh.nCount))  ||
		(!EXTENTCHECK(&regM->rdh.rcBound, &regS->rdh.rcBound)) )
    {
		REGION_CopyRegion(regD, regM);
		return;
    }

    REGION_RegionOp (regD, regM, regS, (voidProcp) REGION_SubtractO,
		(voidProcp) REGION_SubtractNonO1, (voidProcp) NULL);

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
static void REGION_XorRegion(ROSRGNDATA *dr, ROSRGNDATA *sra,
							ROSRGNDATA *srb)
{
	HRGN htra, htrb;
    ROSRGNDATA *tra, *trb;

    if ((! (htra = RGNDATA_AllocRgn(sra->rdh.nCount + 1))) ||
		(! (htrb = RGNDATA_AllocRgn(srb->rdh.nCount + 1))))
	return;
	tra = RGNDATA_LockRgn( htra );
	if( !tra ){
		W32kDeleteObject( htra );
		W32kDeleteObject( htrb );
		return;
	}

	trb = RGNDATA_LockRgn( htrb );
	if( !trb ){
		RGNDATA_UnlockRgn( htra );
		W32kDeleteObject( htra );
		W32kDeleteObject( htrb );
		return;
	}

    REGION_SubtractRegion(tra,sra,srb);
    REGION_SubtractRegion(trb,srb,sra);
    REGION_UnionRegion(dr,tra,trb);
	RGNDATA_UnlockRgn( htra );
	RGNDATA_UnlockRgn( htrb );

    W32kDeleteObject( htra );
    W32kDeleteObject( htrb );
    return;
}


/***********************************************************************
 *           REGION_UnionRectWithRegion
 *           Adds a rectangle to a WINEREGION
 */
static void REGION_UnionRectWithRegion(const RECT *rect, ROSRGNDATA *rgn)
{
    ROSRGNDATA region;

    region.Buffer = (char*)(&(region.rdh.rcBound));
    region.rdh.nCount = 1;
    region.rdh.nRgnSize = sizeof( RECT );
    region.rdh.rcBound = *rect;
    REGION_UnionRegion(rgn, rgn, &region);
}


BOOL REGION_LPTODP(HDC hdc, HRGN hDest, HRGN hSrc)
{
  RECT *pCurRect, *pEndRect;
  PROSRGNDATA srcObj = NULL;
  PROSRGNDATA destObj = NULL;

  DC * dc = DC_HandleToPtr(hdc);
  RECT tmpRect;
  BOOL ret = FALSE;

  if(!dc)
    return ret;

  if(dc->w.MapMode == MM_TEXT) // Requires only a translation
  {
    if(W32kCombineRgn(hDest, hSrc, 0, RGN_COPY) == ERROR)
	  goto done;

    W32kOffsetRgn(hDest, dc->vportOrgX - dc->wndOrgX, dc->vportOrgY - dc->wndOrgY);
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
  DC_ReleasePtr( hdc );
  return ret;
}

HRGN RGNDATA_AllocRgn(INT n)
{
  HRGN hReg;
  PROSRGNDATA pReg;
  BOOL bRet;

  if((hReg = (HRGN)GDIOBJ_AllocObj(sizeof(ROSRGNDATA), GO_REGION_MAGIC))){
	if( (pReg = GDIOBJ_LockObj( hReg, GO_REGION_MAGIC )) ){

      if ((pReg->Buffer = ExAllocatePool(PagedPool, n * sizeof(RECT)))){
      	EMPTY_REGION(pReg);
      	pReg->rdh.dwSize = sizeof(RGNDATAHEADER);
      	pReg->rdh.nCount = n;
      	pReg->rdh.nRgnSize = n*sizeof(RECT);

        bRet = GDIOBJ_UnlockObj( hReg, GO_REGION_MAGIC );
        ASSERT(bRet);

      	return hReg;
	  }

	}
	else
		GDIOBJ_FreeObj( hReg, GO_REGION_MAGIC, GDIOBJFLAG_DEFAULT );
  }
  return NULL;
}

BOOL RGNDATA_InternalDelete( PROSRGNDATA pRgn )
{
  ASSERT(pRgn);
  if(pRgn->Buffer)
    ExFreePool(pRgn->Buffer);
  return TRUE;
}

// W32k Exported Functions
INT
STDCALL
W32kCombineRgn(HRGN  hDest,
                    HRGN  hSrc1,
                    HRGN  hSrc2,
                    INT  CombineMode)
{
  INT result = ERROR;
  PROSRGNDATA destRgn = RGNDATA_LockRgn(hDest);

  if( destRgn ){
  	PROSRGNDATA src1Rgn = RGNDATA_LockRgn(hSrc1);

	if( src1Rgn ){
  		if (CombineMode == RGN_COPY)
  		{
  		  if( !REGION_CopyRegion(destRgn, src1Rgn) )
  				return ERROR;
  		  result = destRgn->rdh.iType;
  		}
  		else
  		{
  		  PROSRGNDATA src2Rgn = RGNDATA_LockRgn(hSrc2);
		  if( src2Rgn ){
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
			  RGNDATA_UnlockRgn( hSrc2 );
		  }
		  RGNDATA_UnlockRgn( hSrc1 );
  		}
		RGNDATA_UnlockRgn( hDest );
	}
	else{
		DPRINT("W32kCombineRgn: hDest unavailable\n");
		return ERROR;
	}
  }
  return result;
}

HRGN
STDCALL
W32kCreateEllipticRgn(INT  LeftRect,
                            INT  TopRect,
                            INT  RightRect,
                            INT  BottomRect)
{
  UNIMPLEMENTED;
}

HRGN
STDCALL
W32kCreateEllipticRgnIndirect(CONST PRECT  rc)
{
  UNIMPLEMENTED;
}

HRGN
STDCALL
W32kCreatePolygonRgn(CONST PPOINT  pt,
                           INT  Count,
                           INT  PolyFillMode)
{
  UNIMPLEMENTED;
}

HRGN
STDCALL
W32kCreatePolyPolygonRgn(CONST PPOINT  pt,
                               CONST PINT  PolyCounts,
                               INT  Count,
                               INT  PolyFillMode)
{
  UNIMPLEMENTED;
}

HRGN
STDCALL
W32kCreateRectRgn(INT  LeftRect,
                        INT  TopRect,
                        INT  RightRect,
                        INT  BottomRect)
{
  HRGN hRgn;
  PROSRGNDATA pRgnData;
  PRECT pRect;

  // Allocate region data structure with space for 1 RECT
  if( ( hRgn = RGNDATA_AllocRgn(1) ) ){
	if( ( pRgnData = RGNDATA_LockRgn( hRgn ) ) ){
    	pRect = (PRECT)pRgnData->Buffer;
    	ASSERT(pRect);

    	// Fill in the region data header
    	pRgnData->rdh.iType = SIMPLEREGION;
    	W32kSetRect(&(pRgnData->rdh.rcBound), LeftRect, TopRect, RightRect, BottomRect);

    	// use W32kCopyRect when implemented
    	W32kSetRect(pRect, LeftRect, TopRect, RightRect, BottomRect);
		RGNDATA_UnlockRgn( hRgn );

    	return hRgn;
	}
	W32kDeleteObject( hRgn );
  }
  DPRINT("W32kCreateRectRgn: can't allocate region\n");
  return NULL;
}

HRGN STDCALL
W32kCreateRectRgnIndirect(CONST PRECT rc)
{
  RECT SafeRc;
  NTSTATUS Status;

  Status = MmCopyFromCaller(&SafeRc, rc, sizeof(RECT));
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  return(UnsafeW32kCreateRectRgnIndirect(&SafeRc));
}

HRGN STDCALL
UnsafeW32kCreateRectRgnIndirect(CONST PRECT  rc)
{
  return(W32kCreateRectRgn(rc->left, rc->top, rc->right, rc->bottom));
}

HRGN
STDCALL
W32kCreateRoundRectRgn(INT  LeftRect,
                             INT  TopRect,
                             INT  RightRect,
                             INT  BottomRect,
                             INT  WidthEllipse,
                             INT  HeightEllipse)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kEqualRgn(HRGN  hSrcRgn1,
                   HRGN  hSrcRgn2)
{
  PROSRGNDATA rgn1, rgn2;
  PRECT tRect1, tRect2;
  int i;
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
W32kExtCreateRegion(CONST PXFORM  Xform,
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
W32kFillRgn(HDC  hDC,
                  HRGN  hRgn,
                  HBRUSH  hBrush)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kFrameRgn(HDC  hDC,
                   HRGN  hRgn,
                   HBRUSH  hBrush,
                   INT  Width,
                   INT  Height)
{
  UNIMPLEMENTED;
}

INT STDCALL
UnsafeW32kGetRgnBox(HRGN  hRgn,
		    LPRECT  pRect)
{
  PROSRGNDATA rgn = RGNDATA_LockRgn(hRgn);
  DWORD ret;

  if (rgn)
    {
      *pRect = rgn->rdh.rcBound;
      ret = rgn->rdh.iType;
      RGNDATA_UnlockRgn( hRgn );

      return ret;
    }
  return 0; //if invalid region return zero
}


INT STDCALL
W32kGetRgnBox(HRGN  hRgn,
	      LPRECT  pRect)
{
  PROSRGNDATA rgn = RGNDATA_LockRgn(hRgn);
  DWORD ret;

  if (rgn)
    {
      RECT SafeRect;
      SafeRect.left = rgn->rdh.rcBound.left;
      SafeRect.top = rgn->rdh.rcBound.top;
      SafeRect.right = rgn->rdh.rcBound.right;
      SafeRect.bottom = rgn->rdh.rcBound.bottom;
      ret = rgn->rdh.iType;
      RGNDATA_UnlockRgn( hRgn );

      if(!NT_SUCCESS(MmCopyToCaller(pRect, &SafeRect, sizeof(RECT))))
	return 0;

      return ret;
    }
  return 0; //if invalid region return zero
}

BOOL
STDCALL
W32kInvertRgn(HDC  hDC,
                    HRGN  hRgn)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kOffsetRgn(HRGN  hRgn,
                   INT  XOffset,
                   INT  YOffset)
{
  PROSRGNDATA rgn = RGNDATA_LockRgn(hRgn);
  INT ret;

  DPRINT("W32kOffsetRgn: hRgn %d Xoffs %d Yoffs %d rgn %x\n", hRgn, XOffset, YOffset, rgn );

  if( !rgn ){
	  DPRINT("W32kOffsetRgn: hRgn error\n");
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
W32kPaintRgn(HDC  hDC,
                   HRGN  hRgn)
{
  RECT box;
  HRGN tmpVisRgn, prevVisRgn;
  DC *dc = DC_HandleToPtr(hDC);
  PROSRGNDATA visrgn;
  CLIPOBJ* ClipRegion;
  BOOL bRet = FALSE;
  PBRUSHOBJ pBrush;
  POINTL BrushOrigin;
  SURFOBJ	*SurfObj;

  if( !dc )
	return FALSE;

  if(!(tmpVisRgn = W32kCreateRectRgn(0, 0, 0, 0))){
	DC_ReleasePtr( hDC );
  	return FALSE;
  }

/* ei enable later
  // Transform region into device co-ords
  if(!REGION_LPTODP(hDC, tmpVisRgn, hRgn) || W32kOffsetRgn(tmpVisRgn, dc->w.DCOrgX, dc->w.DCOrgY) == ERROR) {
    W32kDeleteObject( tmpVisRgn );
	DC_ReleasePtr( hDC );
    return FALSE;
  }
*/
  /* enable when clipping is implemented
  W32kCombineRgn(tmpVisRgn, tmpVisRgn, dc->w.hGCClipRgn, RGN_AND);
  */

  //visrgn = RGNDATA_LockRgn(tmpVisRgn);
  visrgn = RGNDATA_LockRgn(hRgn);

  ClipRegion = IntEngCreateClipRegion( visrgn->rdh.nCount, visrgn->Buffer, visrgn->rdh.rcBound );
  ASSERT( ClipRegion );
  pBrush = BRUSHOBJ_LockBrush(dc->w.hBrush);
  ASSERT(pBrush);
  BrushOrigin.x = dc->w.brushOrgX;
  BrushOrigin.y = dc->w.brushOrgY;
  SurfObj = (SURFOBJ*)AccessUserObject(dc->Surface);

  bRet = IntEngPaint(SurfObj,
	 ClipRegion,
	 pBrush,
	 &BrushOrigin,
	 0xFFFF);//don't know what to put here

  RGNDATA_UnlockRgn( tmpVisRgn );

  // Fill the region
  DC_ReleasePtr( hDC );
  return TRUE;
}

BOOL
STDCALL
W32kPtInRegion(HRGN  hRgn,
                     INT  X,
                     INT  Y)
{
  PROSRGNDATA rgn;
  int i;

  if( (rgn = RGNDATA_LockRgn(hRgn) ) )
	  return FALSE;

  if(rgn->rdh.nCount > 0 && INRECT(rgn->rdh.rcBound, X, Y)){
    for(i = 0; i < rgn->rdh.nCount; i++) {
      if(INRECT(*(PRECT)&rgn->Buffer[i], X, Y)){
		RGNDATA_UnlockRgn(hRgn);
		return TRUE;
	  }
    }
  }
  RGNDATA_UnlockRgn(hRgn);
  return FALSE;
}

BOOL
STDCALL
W32kRectInRegion(HRGN  hRgn,
                       CONST LPRECT  unsaferc)
{
  PROSRGNDATA rgn;
  PRECT pCurRect, pRectEnd;
  PRECT rc;
  BOOL bRet = FALSE;

  if( !NT_SUCCESS( MmCopyFromCaller( rc, unsaferc, sizeof( RECT ) ) ) ){
	DPRINT("W32kRectInRegion: bogus rc\n");
	return ERROR;
  }

  if( !( rgn  = RGNDATA_LockRgn(hRgn) ) )
	return ERROR;

  // this is (just) a useful optimization
  if((rgn->rdh.nCount > 0) && EXTENTCHECK(&rgn->rdh.rcBound, rc))
  {
    for (pCurRect = (PRECT)rgn->Buffer, pRectEnd = pCurRect + rgn->rdh.nCount; pCurRect < pRectEnd; pCurRect++)
    {
      if (pCurRect->bottom <= rc->top) continue; // not far enough down yet
      if (pCurRect->top >= rc->bottom) break;    // too far down
      if (pCurRect->right <= rc->left) continue; // not far enough over yet
      if (pCurRect->left >= rc->right) continue;
      bRet = TRUE;
      break;
    }
  }
  RGNDATA_UnlockRgn(hRgn);
  return bRet;
}

BOOL
STDCALL
W32kSetRectRgn(HRGN  hRgn,
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
W32kUnionRectWithRgn(HRGN hDest, const RECT* unsafeRect)
{
	PRECT pRect;
	PROSRGNDATA pRgn;

	if( !NT_SUCCESS( MmCopyFromCaller( pRect, unsafeRect, sizeof( RECT ) ) ) )
		return NULL;

	if( !(pRgn = RGNDATA_LockRgn( hDest ) ) )
		return NULL;

	REGION_UnionRectWithRegion( pRect, pRgn );
	RGNDATA_UnlockRgn( hDest );
	return hDest;
}

/***********************************************************************
 *           GetRegionData   (GDI32.@)
 *
 * MSDN: GetRegionData, Return Values:
 *
 * "If the function succeeds and dwCount specifies an adequate number of bytes,
 * the return value is always dwCount. If dwCount is too small or the function
 * fails, the return value is 0. If lpRgnData is NULL, the return value is the
 * required number of bytes.
 *
 * If the function fails, the return value is zero."
 */
DWORD STDCALL W32kGetRegionData(HRGN hrgn, DWORD count, LPRGNDATA rgndata)
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

