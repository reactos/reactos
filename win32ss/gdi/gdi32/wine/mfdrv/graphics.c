/*
 * Metafile driver graphics functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
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
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

/**********************************************************************
 *	     MFDRV_MoveTo
 */
BOOL MFDRV_MoveTo(PHYSDEV dev, INT x, INT y)
{
    return MFDRV_MetaParam2(dev,META_MOVETO,x,y);
}

/***********************************************************************
 *           MFDRV_LineTo
 */
BOOL MFDRV_LineTo( PHYSDEV dev, INT x, INT y )
{
     return MFDRV_MetaParam2(dev, META_LINETO, x, y);
}


/***********************************************************************
 *           MFDRV_Arc
 */
BOOL MFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                INT xstart, INT ystart, INT xend, INT yend )
{
     return MFDRV_MetaParam8(dev, META_ARC, left, top, right, bottom,
			     xstart, ystart, xend, yend);
}


/***********************************************************************
 *           MFDRV_Pie
 */
BOOL MFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                INT xstart, INT ystart, INT xend, INT yend )
{
    return MFDRV_MetaParam8(dev, META_PIE, left, top, right, bottom,
			    xstart, ystart, xend, yend);
}


/***********************************************************************
 *           MFDRV_Chord
 */
BOOL MFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                  INT xstart, INT ystart, INT xend, INT yend )
{
    return MFDRV_MetaParam8(dev, META_CHORD, left, top, right, bottom,
			    xstart, ystart, xend, yend);
}

/***********************************************************************
 *           MFDRV_Ellipse
 */
BOOL MFDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return MFDRV_MetaParam4(dev, META_ELLIPSE, left, top, right, bottom);
}

/***********************************************************************
 *           MFDRV_Rectangle
 */
BOOL MFDRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    return MFDRV_MetaParam4(dev, META_RECTANGLE, left, top, right, bottom);
}

/***********************************************************************
 *           MFDRV_RoundRect
 */
BOOL MFDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right,
                      INT bottom, INT ell_width, INT ell_height )
{
    return MFDRV_MetaParam6(dev, META_ROUNDRECT, left, top, right, bottom,
			    ell_width, ell_height);
}

/***********************************************************************
 *           MFDRV_SetPixel
 */
COLORREF MFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    return MFDRV_MetaParam4(dev, META_SETPIXEL, x, y,HIWORD(color),
			    LOWORD(color));
}


/******************************************************************
 *         MFDRV_MetaPoly - implements Polygon and Polyline
 */
static BOOL MFDRV_MetaPoly(PHYSDEV dev, short func, POINTS *pt, short count)
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;

    len = sizeof(METARECORD) + (count * 4);
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len )))
	return FALSE;

    mr->rdSize = len / 2;
    mr->rdFunction = func;
    *(mr->rdParm) = count;
    memcpy(mr->rdParm + 1, pt, count * 4);
    ret = MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}


/**********************************************************************
 *          MFDRV_Polyline
 */
BOOL MFDRV_Polyline( PHYSDEV dev, const POINT* pt, INT count )
{
    int i;
    POINTS *pts;
    BOOL ret;

    pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS)*count );
    if(!pts) return FALSE;
    for (i=count;i--;)
    {
        pts[i].x = pt[i].x;
        pts[i].y = pt[i].y;
    }
    ret = MFDRV_MetaPoly(dev, META_POLYLINE, pts, count);

    HeapFree( GetProcessHeap(), 0, pts );
    return ret;
}


/**********************************************************************
 *          MFDRV_Polygon
 */
BOOL MFDRV_Polygon( PHYSDEV dev, const POINT* pt, INT count )
{
    int i;
    POINTS *pts;
    BOOL ret;

    pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS)*count );
    if(!pts) return FALSE;
    for (i=count;i--;)
    {
        pts[i].x = pt[i].x;
        pts[i].y = pt[i].y;
    }
    ret = MFDRV_MetaPoly(dev, META_POLYGON, pts, count);

    HeapFree( GetProcessHeap(), 0, pts );
    return ret;
}


/**********************************************************************
 *          MFDRV_PolyPolygon
 */
BOOL MFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polygons)
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    unsigned int i,j;
    POINTS *pts;
    INT16 totalpoint16 = 0;
    INT16 * pointcounts;

    for (i=0;i<polygons;i++) {
         totalpoint16 += counts[i];
    }

    /* allocate space for all points */
    pts=HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS) * totalpoint16 );
    pointcounts = HeapAlloc( GetProcessHeap(), 0, sizeof(INT16) * totalpoint16 );

    /* copy point counts */
    for (i=0;i<polygons;i++) {
          pointcounts[i] = counts[i];
    }

    /* convert all points */
    for (j = totalpoint16; j--;){
        pts[j].x = pt[j].x;
        pts[j].y = pt[j].y;
    }

    len = sizeof(METARECORD) + sizeof(WORD) + polygons*sizeof(INT16) + totalpoint16*sizeof(*pts);

    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len ))) {
         HeapFree( GetProcessHeap(), 0, pts );
         HeapFree( GetProcessHeap(), 0, pointcounts );
         return FALSE;
    }

    mr->rdSize = len /2;
    mr->rdFunction = META_POLYPOLYGON;
    *(mr->rdParm) = polygons;
    memcpy(mr->rdParm + 1, pointcounts, polygons*sizeof(INT16));
    memcpy(mr->rdParm + 1+polygons, pts , totalpoint16*sizeof(*pts));
    ret = MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);

    HeapFree( GetProcessHeap(), 0, pts );
    HeapFree( GetProcessHeap(), 0, pointcounts );
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}


/**********************************************************************
 *          MFDRV_ExtFloodFill
 */
BOOL MFDRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType )
{
    return MFDRV_MetaParam4(dev,META_FLOODFILL,x,y,HIWORD(color),
			    LOWORD(color));
}


/******************************************************************
 *         MFDRV_CreateRegion
 *
 * For explanation of the format of the record see MF_Play_MetaCreateRegion in
 * objects/metafile.c
 */
static INT16 MFDRV_CreateRegion(PHYSDEV dev, HRGN hrgn)
{
    DWORD len;
    METARECORD *mr;
    RGNDATA *rgndata;
    RECT *pCurRect, *pEndRect;
    WORD Bands = 0, MaxBands = 0;
    WORD *Param, *StartBand;
    BOOL ret;

    if (!(len = GetRegionData( hrgn, 0, NULL ))) return -1;
    if( !(rgndata = HeapAlloc( GetProcessHeap(), 0, len )) ) {
        WARN("Can't alloc rgndata buffer\n");
	return -1;
    }
    GetRegionData( hrgn, len, rgndata );

    /* Overestimate of length:
     * Assume every rect is a separate band -> 6 WORDs per rect
     */
    len = sizeof(METARECORD) + 20 + (rgndata->rdh.nCount * 12);
    if( !(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len )) ) {
        WARN("Can't alloc METARECORD buffer\n");
	HeapFree( GetProcessHeap(), 0, rgndata );
	return -1;
    }

    Param = mr->rdParm + 11;
    StartBand = NULL;

    pEndRect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
    for(pCurRect = (RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
    {
        if( StartBand && pCurRect->top == *(StartBand + 1) )
        {
	    *Param++ = pCurRect->left;
	    *Param++ = pCurRect->right;
	}
	else
	{
	    if(StartBand)
	    {
	        *StartBand = Param - StartBand - 3;
		*Param++ = *StartBand;
		if(*StartBand > MaxBands)
		    MaxBands = *StartBand;
		Bands++;
	    }
	    StartBand = Param++;
	    *Param++ = pCurRect->top;
	    *Param++ = pCurRect->bottom;
	    *Param++ = pCurRect->left;
	    *Param++ = pCurRect->right;
	}
    }

    if (StartBand)
    {
        *StartBand = Param - StartBand - 3;
        *Param++ = *StartBand;
        if(*StartBand > MaxBands)
            MaxBands = *StartBand;
        Bands++;
    }

    mr->rdParm[0] = 0;
    mr->rdParm[1] = 6;
    mr->rdParm[2] = 0x2f6;
    mr->rdParm[3] = 0;
    mr->rdParm[4] = (Param - &mr->rdFunction) * sizeof(WORD);
    mr->rdParm[5] = Bands;
    mr->rdParm[6] = MaxBands;
    mr->rdParm[7] = rgndata->rdh.rcBound.left;
    mr->rdParm[8] = rgndata->rdh.rcBound.top;
    mr->rdParm[9] = rgndata->rdh.rcBound.right;
    mr->rdParm[10] = rgndata->rdh.rcBound.bottom;
    mr->rdFunction = META_CREATEREGION;
    mr->rdSize = Param - (WORD *)mr;
    ret = MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    HeapFree( GetProcessHeap(), 0, rgndata );
    if(!ret)
    {
        WARN("MFDRV_WriteRecord failed\n");
	return -1;
    }
    return MFDRV_AddHandle( dev, hrgn );
}


/**********************************************************************
 *          MFDRV_PaintRgn
 */
BOOL MFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn )
{
    INT16 index;
    index = MFDRV_CreateRegion( dev, hrgn );
    if(index == -1)
        return FALSE;
    return MFDRV_MetaParam1( dev, META_PAINTREGION, index );
}


/**********************************************************************
 *          MFDRV_InvertRgn
 */
BOOL MFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn )
{
    INT16 index;
    index = MFDRV_CreateRegion( dev, hrgn );
    if(index == -1)
        return FALSE;
    return MFDRV_MetaParam1( dev, META_INVERTREGION, index );
}


/**********************************************************************
 *          MFDRV_FillRgn
 */
BOOL MFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush )
{
    INT16 iRgn, iBrush;
    iRgn = MFDRV_CreateRegion( dev, hrgn );
    if(iRgn == -1)
        return FALSE;
    iBrush = MFDRV_CreateBrushIndirect( dev, hbrush );
    if(!iBrush)
        return FALSE;
    return MFDRV_MetaParam2( dev, META_FILLREGION, iRgn, iBrush );
}

/**********************************************************************
 *          MFDRV_FrameRgn
 */
BOOL MFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT x, INT y )
{
    INT16 iRgn, iBrush;
    iRgn = MFDRV_CreateRegion( dev, hrgn );
    if(iRgn == -1)
        return FALSE;
    iBrush = MFDRV_CreateBrushIndirect( dev, hbrush );
    if(!iBrush)
        return FALSE;
    return MFDRV_MetaParam4( dev, META_FRAMEREGION, iRgn, iBrush, x, y );
}


/**********************************************************************
 *          MFDRV_ExtSelectClipRgn
 */
INT MFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode )
{
    INT16 iRgn;
    INT ret;

    if (mode != RGN_COPY) return ERROR;
    if (!hrgn) return NULLREGION;
    iRgn = MFDRV_CreateRegion( dev, hrgn );
    if(iRgn == -1) return ERROR;
    ret = MFDRV_MetaParam1( dev, META_SELECTOBJECT, iRgn ) ? NULLREGION : ERROR;
    MFDRV_MetaParam1( dev, META_DELETEOBJECT, iRgn );
    MFDRV_RemoveHandle( dev, iRgn );
    return ret;
}


/**********************************************************************
 *          MFDRV_PolyBezier
 * Since MetaFiles don't record Beziers and they don't even record
 * approximations to them using lines, we need this stub function.
 */
BOOL MFDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count )
{
    return FALSE;
}

/**********************************************************************
 *          MFDRV_PolyBezierTo
 * Since MetaFiles don't record Beziers and they don't even record
 * approximations to them using lines, we need this stub function.
 */
BOOL MFDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count )
{
    return FALSE;
}
