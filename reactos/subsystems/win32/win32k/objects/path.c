/*
 * Graphics paths (BeginPath, EndPath etc.)
 *
 * Copyright 1997, 1998 Martin Boehme
 *                 1999 Huw D M Davies
 * Copyright 2005 Dmitry Timoshkov
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
/*
 *
 * Addaped for the use in ReactOS.
 *
 */
/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/path.c
 * PURPOSE:         Freetype library support
 * PROGRAMMER:
 */

#include <w32k.h>
#include "math.h"

#define NDEBUG
#include <debug.h>

#define NUM_ENTRIES_INITIAL 16  /* Initial size of points / flags arrays  */
#define GROW_FACTOR_NUMER    2  /* Numerator of grow factor for the array */
#define GROW_FACTOR_DENOM    1  /* Denominator of grow factor             */

BOOL FASTCALL PATH_AddEntry (PPATH pPath, const POINT *pPoint, BYTE flags);
BOOL FASTCALL PATH_AddFlatBezier (PPATH pPath, POINT *pt, BOOL closed);
BOOL FASTCALL PATH_DoArcPart (PPATH pPath, FLOAT_POINT corners[], double angleStart, double angleEnd, BYTE startEntryType);
BOOL FASTCALL PATH_FillPath( PDC dc, PPATH pPath );
BOOL FASTCALL PATH_FlattenPath (PPATH pPath);
VOID FASTCALL PATH_NormalizePoint (FLOAT_POINT corners[], const FLOAT_POINT *pPoint, double *pX, double *pY);

BOOL FASTCALL PATH_ReserveEntries (PPATH pPath, INT numEntries);
VOID FASTCALL PATH_ScaleNormalizedPoint (FLOAT_POINT corners[], double x, double y, POINT *pPoint);
BOOL FASTCALL PATH_StrokePath(DC *dc, PPATH pPath);
BOOL PATH_CheckCorners(DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2);

VOID FASTCALL IntGetCurrentPositionEx(PDC dc, LPPOINT pt);

/***********************************************************************
 * Internal functions
 */

BOOL
FASTCALL
PATH_Delete(HPATH hPath)
{
  if (!hPath) return FALSE;
  PPATH pPath = PATH_LockPath( hPath );
  if (!pPath) return FALSE;
  PATH_DestroyGdiPath( pPath );
  PATH_UnlockPath( pPath );
  PATH_FreeExtPathByHandle(hPath);
  return TRUE;
}


VOID
FASTCALL
IntGdiCloseFigure(PPATH pPath)
{
   ASSERT(pPath->state == PATH_Open);

   // FIXME: Shouldn't we draw a line to the beginning of the figure?
   // Set PT_CLOSEFIGURE on the last entry and start a new stroke
   if(pPath->numEntriesUsed)
   {
      pPath->pFlags[pPath->numEntriesUsed-1]|=PT_CLOSEFIGURE;
      pPath->newStroke=TRUE;
   }
}

/* PATH_FillPath
 *
 *
 */
BOOL
FASTCALL
PATH_FillPath( PDC dc, PPATH pPath )
{
  INT   mapMode, graphicsMode;
  SIZE  ptViewportExt, ptWindowExt;
  POINTL ptViewportOrg, ptWindowOrg;
  XFORM xform;
  HRGN  hrgn;
  PDC_ATTR Dc_Attr = dc->pDc_Attr;

  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  if( pPath->state != PATH_Closed )
  {
    SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
    return FALSE;
  }

  if( PATH_PathToRegion( pPath, Dc_Attr->jFillMode, &hrgn ))
  {
    /* Since PaintRgn interprets the region as being in logical coordinates
     * but the points we store for the path are already in device
     * coordinates, we have to set the mapping mode to MM_TEXT temporarily.
     * Using SaveDC to save information about the mapping mode / world
     * transform would be easier but would require more overhead, especially
     * now that SaveDC saves the current path.
     */

    /* Save the information about the old mapping mode */
    mapMode = Dc_Attr->iMapMode;
    ptViewportExt = Dc_Attr->szlViewportExt;
    ptViewportOrg = Dc_Attr->ptlViewportOrg;
    ptWindowExt   = Dc_Attr->szlWindowExt;
    ptWindowOrg   = Dc_Attr->ptlWindowOrg;

    /* Save world transform
     * NB: The Windows documentation on world transforms would lead one to
     * believe that this has to be done only in GM_ADVANCED; however, my
     * tests show that resetting the graphics mode to GM_COMPATIBLE does
     * not reset the world transform.
     */
    MatrixS2XForm(&xform, &dc->DcLevel.mxWorldToPage);

    /* Set MM_TEXT */
//    IntGdiSetMapMode( dc, MM_TEXT );
//    Dc_Attr->ptlViewportOrg.x = 0;
//    Dc_Attr->ptlViewportOrg.y = 0;
//    Dc_Attr->ptlWindowOrg.x = 0;
//    Dc_Attr->ptlWindowOrg.y = 0;

    graphicsMode = Dc_Attr->iGraphicsMode;
//    Dc_Attr->iGraphicsMode = GM_ADVANCED;
//    IntGdiModifyWorldTransform( dc, &xform, MWT_IDENTITY );
//    Dc_Attr->iGraphicsMode =  graphicsMode;

    /* Paint the region */
    IntGdiPaintRgn( dc, hrgn );
    NtGdiDeleteObject( hrgn );
    /* Restore the old mapping mode */
//    IntGdiSetMapMode( dc, mapMode );
//    Dc_Attr->szlViewportExt = ptViewportExt;
//    Dc_Attr->ptlViewportOrg = ptViewportOrg;
//    Dc_Attr->szlWindowExt   = ptWindowExt;
//    Dc_Attr->ptlWindowOrg   = ptWindowOrg;

    /* Go to GM_ADVANCED temporarily to restore the world transform */
    graphicsMode = Dc_Attr->iGraphicsMode;
//    Dc_Attr->iGraphicsMode = GM_ADVANCED;
//    IntGdiModifyWorldTransform( dc, &xform, MWT_MAX+1 );
//    Dc_Attr->iGraphicsMode = graphicsMode;
    return TRUE;
  }
  return FALSE;
}

/* PATH_InitGdiPath
 *
 * Initializes the GdiPath structure.
 */
VOID
FASTCALL
PATH_InitGdiPath ( PPATH pPath )
{
  ASSERT(pPath!=NULL);

  pPath->state=PATH_Null;
  pPath->pPoints=NULL;
  pPath->pFlags=NULL;
  pPath->numEntriesUsed=0;
  pPath->numEntriesAllocated=0;
}

/* PATH_DestroyGdiPath
 *
 * Destroys a GdiPath structure (frees the memory in the arrays).
 */
VOID
FASTCALL
PATH_DestroyGdiPath ( PPATH pPath )
{
  ASSERT(pPath!=NULL);

  if (pPath->pPoints) ExFreePoolWithTag(pPath->pPoints, TAG_PATH);
  if (pPath->pFlags) ExFreePoolWithTag(pPath->pFlags, TAG_PATH);
}

/* PATH_AssignGdiPath
 *
 * Copies the GdiPath structure "pPathSrc" to "pPathDest". A deep copy is
 * performed, i.e. the contents of the pPoints and pFlags arrays are copied,
 * not just the pointers. Since this means that the arrays in pPathDest may
 * need to be resized, pPathDest should have been initialized using
 * PATH_InitGdiPath (in C++, this function would be an assignment operator,
 * not a copy constructor).
 * Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_AssignGdiPath ( PPATH pPathDest, const PPATH pPathSrc )
{
  ASSERT(pPathDest!=NULL && pPathSrc!=NULL);

  /* Make sure destination arrays are big enough */
  if ( !PATH_ReserveEntries(pPathDest, pPathSrc->numEntriesUsed) )
    return FALSE;

  /* Perform the copy operation */
  memcpy(pPathDest->pPoints, pPathSrc->pPoints,
    sizeof(POINT)*pPathSrc->numEntriesUsed);
  memcpy(pPathDest->pFlags, pPathSrc->pFlags,
    sizeof(BYTE)*pPathSrc->numEntriesUsed);

  pPathDest->state=pPathSrc->state;
  pPathDest->numEntriesUsed=pPathSrc->numEntriesUsed;
  pPathDest->newStroke=pPathSrc->newStroke;
  return TRUE;
}

/* PATH_MoveTo
 *
 * Should be called when a MoveTo is performed on a DC that has an
 * open path. This starts a new stroke. Returns TRUE if successful, else
 * FALSE.
 */
BOOL
FASTCALL
PATH_MoveTo ( PDC dc )
{
  PPATH pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    /* FIXME: Do we have to call SetLastError? */
    return FALSE;
  }
  /* Start a new stroke */
  pPath->newStroke = TRUE;
  PATH_UnlockPath( pPath );
  return TRUE;
}

/* PATH_LineTo
 *
 * Should be called when a LineTo is performed on a DC that has an
 * open path. This adds a PT_LINETO entry to the path (and possibly
 * a PT_MOVETO entry, if this is the first LineTo in a stroke).
 * Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_LineTo ( PDC dc, INT x, INT y )
{
  BOOL Ret;
  PPATH pPath;
  POINT point, pointCurPos;

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  /* Convert point to device coordinates */
  point.x=x;
  point.y=y;
  CoordLPtoDP ( dc, &point );

  /* Add a PT_MOVETO if necessary */
  if ( pPath->newStroke )
  {
    pPath->newStroke = FALSE;
    IntGetCurrentPositionEx ( dc, &pointCurPos );
    CoordLPtoDP ( dc, &pointCurPos );
    if ( !PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO) )
    {
      PATH_UnlockPath( pPath );
      return FALSE;
    }
  }

  /* Add a PT_LINETO entry */
  Ret = PATH_AddEntry(pPath, &point, PT_LINETO);
  PATH_UnlockPath( pPath );
  return Ret;
}

/* PATH_Rectangle
 *
 * Should be called when a call to Rectangle is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_Rectangle ( PDC dc, INT x1, INT y1, INT x2, INT y2 )
{
  PPATH pPath;
  POINT corners[2], pointTemp;
  INT   temp;

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  /* Convert points to device coordinates */
  corners[0].x=x1;
  corners[0].y=y1;
  corners[1].x=x2;
  corners[1].y=y2;
  IntLPtoDP ( dc, corners, 2 );

  /* Make sure first corner is top left and second corner is bottom right */
  if ( corners[0].x > corners[1].x )
  {
    temp=corners[0].x;
    corners[0].x=corners[1].x;
    corners[1].x=temp;
  }
  if ( corners[0].y > corners[1].y )
  {
    temp=corners[0].y;
    corners[0].y=corners[1].y;
    corners[1].y=temp;
  }

  /* In GM_COMPATIBLE, don't include bottom and right edges */
  if ( IntGetGraphicsMode(dc) == GM_COMPATIBLE )
  {
    corners[1].x--;
    corners[1].y--;
  }

  /* Close any previous figure */
  IntGdiCloseFigure(pPath);

  /* Add four points to the path */
  pointTemp.x=corners[1].x;
  pointTemp.y=corners[0].y;
  if ( !PATH_AddEntry(pPath, &pointTemp, PT_MOVETO) )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }
  if ( !PATH_AddEntry(pPath, corners, PT_LINETO) )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }
  pointTemp.x=corners[0].x;
  pointTemp.y=corners[1].y;
  if ( !PATH_AddEntry(pPath, &pointTemp, PT_LINETO) )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }
  if ( !PATH_AddEntry(pPath, corners+1, PT_LINETO) )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  /* Close the rectangle figure */
  IntGdiCloseFigure(pPath) ;
  PATH_UnlockPath( pPath );
  return TRUE;
}

/* PATH_RoundRect
 *
 * Should be called when a call to RoundRect is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 *
 * FIXME: it adds the same entries to the path as windows does, but there
 * is an error in the bezier drawing code so that there are small pixel-size
 * gaps when the resulting path is drawn by StrokePath()
 */
BOOL FASTCALL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height)
{
   PPATH pPath;
   POINT corners[2], pointTemp;
   FLOAT_POINT ellCorners[2];

   pPath = PATH_LockPath( dc->DcLevel.hPath );
   if (!pPath) return FALSE;   

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }

   if(!PATH_CheckCorners(dc,corners,x1,y1,x2,y2))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }

   /* Add points to the roundrect path */
   ellCorners[0].x = corners[1].x-ell_width;
   ellCorners[0].y = corners[0].y;
   ellCorners[1].x = corners[1].x;
   ellCorners[1].y = corners[0].y+ell_height;
   if(!PATH_DoArcPart(pPath, ellCorners, 0, -M_PI_2, PT_MOVETO))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }
   pointTemp.x = corners[0].x+ell_width/2;
   pointTemp.y = corners[0].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }
   ellCorners[0].x = corners[0].x;
   ellCorners[1].x = corners[0].x+ell_width;
   if(!PATH_DoArcPart(pPath, ellCorners, -M_PI_2, -M_PI, FALSE))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }
   pointTemp.x = corners[0].x;
   pointTemp.y = corners[1].y-ell_height/2;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }
   ellCorners[0].y = corners[1].y-ell_height;
   ellCorners[1].y = corners[1].y;
   if(!PATH_DoArcPart(pPath, ellCorners, M_PI, M_PI_2, FALSE))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }
   pointTemp.x = corners[1].x-ell_width/2;
   pointTemp.y = corners[1].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }
   ellCorners[0].x = corners[1].x-ell_width;
   ellCorners[1].x = corners[1].x;
   if(!PATH_DoArcPart(pPath, ellCorners, M_PI_2, 0, FALSE))
   {
      PATH_UnlockPath( pPath );
      return FALSE;
   }

   IntGdiCloseFigure(pPath);
   PATH_UnlockPath( pPath );
   return TRUE;
}

/* PATH_Ellipse
 *
 * Should be called when a call to Ellipse is performed on a DC that has
 * an open path. This adds four Bezier splines representing the ellipse
 * to the path. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_Ellipse ( PDC dc, INT x1, INT y1, INT x2, INT y2 )
{
  PPATH pPath;
  /* TODO: This should probably be revised to call PATH_AngleArc */
  /* (once it exists) */
  BOOL Ret = PATH_Arc ( dc, x1, y1, x2, y2, x1, (y1+y2)/2, x1, (y1+y2)/2, GdiTypeArc );
  if (Ret)
  {
     pPath = PATH_LockPath( dc->DcLevel.hPath );
     if (!pPath) return FALSE;
     IntGdiCloseFigure(pPath);
     PATH_UnlockPath( pPath );
  }
  return Ret;
}

/* PATH_Arc
 *
 * Should be called when a call to Arc is performed on a DC that has
 * an open path. This adds up to five Bezier splines representing the arc
 * to the path. When 'lines' is 1, we add 1 extra line to get a chord,
 * when 'lines' is 2, we add 2 extra lines to get a pie, and when 'lines' is
 * -1 we add 1 extra line from the current DC position to the starting position
 * of the arc before drawing the arc itself (arcto). Returns TRUE if successful,
 * else FALSE.
 */
BOOL
FASTCALL
PATH_Arc ( PDC dc, INT x1, INT y1, INT x2, INT y2,
   INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines)
{
  double  angleStart, angleEnd, angleStartQuadrant, angleEndQuadrant=0.0;
          /* Initialize angleEndQuadrant to silence gcc's warning */
  double  x, y;
  FLOAT_POINT corners[2], pointStart, pointEnd;
  POINT   centre, pointCurPos;
  BOOL    start, end, Ret = TRUE;
  INT     temp;
  BOOL    clockwise;
  PPATH   pPath;

  /* FIXME: This function should check for all possible error returns */
  /* FIXME: Do we have to respect newStroke? */

  ASSERT ( dc );

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  clockwise = ((dc->DcLevel.flPath & DCPATH_CLOCKWISE) != 0);

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    Ret = FALSE;
    goto ArcExit;    
  }

  /* Check for zero height / width */
  /* FIXME: Only in GM_COMPATIBLE? */
  if ( x1==x2 || y1==y2 )
  {
    Ret = TRUE;
    goto ArcExit;
  }
  /* Convert points to device coordinates */
  corners[0].x=(FLOAT)x1;
  corners[0].y=(FLOAT)y1;
  corners[1].x=(FLOAT)x2;
  corners[1].y=(FLOAT)y2;
  pointStart.x=(FLOAT)xStart;
  pointStart.y=(FLOAT)yStart;
  pointEnd.x=(FLOAT)xEnd;
  pointEnd.y=(FLOAT)yEnd;
  INTERNAL_LPTODP_FLOAT(dc, corners);
  INTERNAL_LPTODP_FLOAT(dc, corners+1);
  INTERNAL_LPTODP_FLOAT(dc, &pointStart);
  INTERNAL_LPTODP_FLOAT(dc, &pointEnd);

  /* Make sure first corner is top left and second corner is bottom right */
  if ( corners[0].x > corners[1].x )
  {
    temp=corners[0].x;
    corners[0].x=corners[1].x;
    corners[1].x=temp;
  }
  if ( corners[0].y > corners[1].y )
  {
    temp=corners[0].y;
    corners[0].y=corners[1].y;
    corners[1].y=temp;
  }

  /* Compute start and end angle */
  PATH_NormalizePoint(corners, &pointStart, &x, &y);
  angleStart=atan2(y, x);
  PATH_NormalizePoint(corners, &pointEnd, &x, &y);
  angleEnd=atan2(y, x);

  /* Make sure the end angle is "on the right side" of the start angle */
  if ( clockwise )
  {
    if ( angleEnd <= angleStart )
    {
      angleEnd+=2*M_PI;
      ASSERT(angleEnd>=angleStart);
    }
  }
  else
  {
    if(angleEnd>=angleStart)
    {
      angleEnd-=2*M_PI;
      ASSERT(angleEnd<=angleStart);
    }
  }

  /* In GM_COMPATIBLE, don't include bottom and right edges */
  if ( IntGetGraphicsMode(dc) == GM_COMPATIBLE )
  {
    corners[1].x--;
    corners[1].y--;
  }

  /* arcto: Add a PT_MOVETO only if this is the first entry in a stroke */
  if(lines==GdiTypeArcTo && pPath->newStroke) // -1
  {
     pPath->newStroke=FALSE;
     IntGetCurrentPositionEx ( dc, &pointCurPos );
     CoordLPtoDP(dc, &pointCurPos);
     if(!PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO))
     {
       Ret = FALSE;
       goto ArcExit;
     }
  }

  /* Add the arc to the path with one Bezier spline per quadrant that the
   * arc spans */
  start=TRUE;
  end=FALSE;
  do
  {
    /* Determine the start and end angles for this quadrant */
    if(start)
    {
      angleStartQuadrant=angleStart;
      if ( clockwise )
        angleEndQuadrant=(floor(angleStart/M_PI_2)+1.0)*M_PI_2;
      else
        angleEndQuadrant=(ceil(angleStart/M_PI_2)-1.0)*M_PI_2;
    }
    else
    {
      angleStartQuadrant=angleEndQuadrant;
      if ( clockwise )
        angleEndQuadrant+=M_PI_2;
      else
        angleEndQuadrant-=M_PI_2;
    }

    /* Have we reached the last part of the arc? */
    if ( (clockwise && angleEnd<angleEndQuadrant)
      || (!clockwise && angleEnd>angleEndQuadrant)
      )
    {
      /* Adjust the end angle for this quadrant */
     angleEndQuadrant = angleEnd;
     end = TRUE;
    }

    /* Add the Bezier spline to the path */
    PATH_DoArcPart ( pPath, corners, angleStartQuadrant, angleEndQuadrant,
       start ? (lines==GdiTypeArcTo ? PT_LINETO : PT_MOVETO) : FALSE ); // -1
    start = FALSE;
  } while(!end);

  /* chord: close figure. pie: add line and close figure */
  if (lines==GdiTypeChord) // 1
  {
      IntGdiCloseFigure(pPath);
  }
  else if (lines==GdiTypePie) // 2
  {
      centre.x = (corners[0].x+corners[1].x)/2;
      centre.y = (corners[0].y+corners[1].y)/2;
      if(!PATH_AddEntry(pPath, &centre, PT_LINETO | PT_CLOSEFIGURE))
         Ret = FALSE;
  }
ArcExit:
  PATH_UnlockPath( pPath );
  return Ret;
}

BOOL
FASTCALL
PATH_PolyBezierTo ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT pt;
  ULONG i;
  PPATH pPath;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

   pPath = PATH_LockPath( dc->DcLevel.hPath );
   if (!pPath) return FALSE;
   
  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  /* Add a PT_MOVETO if necessary */
  if ( pPath->newStroke )
  {
    pPath->newStroke=FALSE;
    IntGetCurrentPositionEx ( dc, &pt );
    CoordLPtoDP ( dc, &pt );
    if ( !PATH_AddEntry(pPath, &pt, PT_MOVETO) )
    {
        PATH_UnlockPath( pPath );
        return FALSE;
    }
  }

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, PT_BEZIERTO);
  }
  PATH_UnlockPath( pPath );
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyBezier ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;
  PPATH   pPath;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

   /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  for ( i = 0; i < cbPoints; i++ )
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry ( pPath, &pt, (i == 0) ? PT_MOVETO : PT_BEZIERTO );
  }
  PATH_UnlockPath( pPath );
  return TRUE;
}

BOOL
FASTCALL
PATH_Polyline ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;
  PPATH   pPath;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }
  for ( i = 0; i < cbPoints; i++ )
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, (i == 0) ? PT_MOVETO : PT_LINETO);
  }
  PATH_UnlockPath( pPath );
  return TRUE;
}

BOOL
FASTCALL
PATH_PolylineTo ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;
  PPATH   pPath;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;
   
  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  /* Add a PT_MOVETO if necessary */
  if ( pPath->newStroke )
  {
    pPath->newStroke = FALSE;
    IntGetCurrentPositionEx ( dc, &pt );
    CoordLPtoDP ( dc, &pt );
    if ( !PATH_AddEntry(pPath, &pt, PT_MOVETO) )
    {
      PATH_UnlockPath( pPath );
      return FALSE;
    }
  }

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, PT_LINETO);
  }
  PATH_UnlockPath( pPath );
  return TRUE;
}


BOOL
FASTCALL
PATH_Polygon ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;
  PPATH   pPath;

  ASSERT ( dc );
  ASSERT ( pts );

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, (i == 0) ? PT_MOVETO :
      ((i == cbPoints-1) ? PT_LINETO | PT_CLOSEFIGURE :
      PT_LINETO));
  }
  PATH_UnlockPath( pPath );
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyPolygon ( PDC dc, const POINT* pts, const INT* counts, UINT polygons )
{
  POINT   pt, startpt;
  ULONG   poly, point, i;
  PPATH   pPath;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( counts );
  ASSERT ( polygons );

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  for(i = 0, poly = 0; poly < polygons; poly++)
  {
    for(point = 0; point < (ULONG) counts[poly]; point++, i++)
    {
      pt = pts[i];
      CoordLPtoDP ( dc, &pt );
      if(point == 0) startpt = pt;
        PATH_AddEntry(pPath, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
    }
    /* win98 adds an extra line to close the figure for some reason */
    PATH_AddEntry(pPath, &startpt, PT_LINETO | PT_CLOSEFIGURE);
  }
  PATH_UnlockPath( pPath );
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyPolyline ( PDC dc, const POINT* pts, const DWORD* counts, DWORD polylines )
{
  POINT   pt;
  ULONG   poly, point, i;
  PPATH   pPath;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( counts );
  ASSERT ( polylines );

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath) return FALSE;

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
  {
    PATH_UnlockPath( pPath );
    return FALSE;
  }

  for(i = 0, poly = 0; poly < polylines; poly++)
  {
    for(point = 0; point < counts[poly]; point++, i++)
    {
      pt = pts[i];
      CoordLPtoDP ( dc, &pt );
      PATH_AddEntry(pPath, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
    }
  }
  PATH_UnlockPath( pPath );
  return TRUE;
}


/* PATH_CheckCorners
 *
 * Helper function for PATH_RoundRect() and PATH_Rectangle()
 */
BOOL PATH_CheckCorners(DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2)
{
   INT temp;
   PDC_ATTR Dc_Attr = dc->pDc_Attr;
   if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

   /* Convert points to device coordinates */
   corners[0].x=x1;
   corners[0].y=y1;
   corners[1].x=x2;
   corners[1].y=y2;
   CoordLPtoDP(dc, &corners[0]);
   CoordLPtoDP(dc, &corners[1]);

   /* Make sure first corner is top left and second corner is bottom right */
   if(corners[0].x>corners[1].x)
   {
      temp=corners[0].x;
      corners[0].x=corners[1].x;
      corners[1].x=temp;
   }
   if(corners[0].y>corners[1].y)
   {
      temp=corners[0].y;
      corners[0].y=corners[1].y;
      corners[1].y=temp;
   }

   /* In GM_COMPATIBLE, don't include bottom and right edges */
   if(Dc_Attr->iGraphicsMode==GM_COMPATIBLE)
   {
      corners[1].x--;
      corners[1].y--;
   }

   return TRUE;
}


/* PATH_AddFlatBezier
 *
 */
BOOL
FASTCALL
PATH_AddFlatBezier ( PPATH pPath, POINT *pt, BOOL closed )
{
  POINT *pts;
  INT no, i;

  pts = GDI_Bezier( pt, 4, &no );
  if ( !pts ) return FALSE;

  for(i = 1; i < no; i++)
    PATH_AddEntry(pPath, &pts[i],  (i == no-1 && closed) ? PT_LINETO | PT_CLOSEFIGURE : PT_LINETO);

  ExFreePoolWithTag(pts, TAG_BEZIER);
  return TRUE;
}

/* PATH_FlattenPath
 *
 * Replaces Beziers with line segments
 *
 */
BOOL
FASTCALL
PATH_FlattenPath(PPATH pPath)
{
  PATH newPath;
  INT srcpt;

  RtlZeroMemory(&newPath, sizeof(newPath));
  newPath.state = PATH_Open;
  for(srcpt = 0; srcpt < pPath->numEntriesUsed; srcpt++) {
    switch(pPath->pFlags[srcpt] & ~PT_CLOSEFIGURE) {
      case PT_MOVETO:
      case PT_LINETO:
        PATH_AddEntry(&newPath, &pPath->pPoints[srcpt], pPath->pFlags[srcpt]);
        break;
      case PT_BEZIERTO:
        PATH_AddFlatBezier(&newPath, &pPath->pPoints[srcpt-1], pPath->pFlags[srcpt+2] & PT_CLOSEFIGURE);
        srcpt += 2;
        break;
    }
  }
  newPath.state = PATH_Closed;
  PATH_AssignGdiPath(pPath, &newPath);
  PATH_EmptyPath(&newPath);
  return TRUE;
}


/* PATH_PathToRegion
 *
 * Creates a region from the specified path using the specified polygon
 * filling mode. The path is left unchanged. A handle to the region that
 * was created is stored in *pHrgn. If successful, TRUE is returned; if an
 * error occurs, SetLastError is called with the appropriate value and
 * FALSE is returned.
 */
BOOL
FASTCALL
PATH_PathToRegion ( PPATH pPath, INT nPolyFillMode, HRGN *pHrgn )
{
  int    numStrokes, iStroke, i;
  PULONG  pNumPointsInStroke;
  HRGN hrgn = 0;

  ASSERT(pPath!=NULL);
  ASSERT(pHrgn!=NULL);

  PATH_FlattenPath ( pPath );

  /* FIXME: What happens when number of points is zero? */

  /* First pass: Find out how many strokes there are in the path */
  /* FIXME: We could eliminate this with some bookkeeping in GdiPath */
  numStrokes=0;
  for(i=0; i<pPath->numEntriesUsed; i++)
    if((pPath->pFlags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
      numStrokes++;

  /* Allocate memory for number-of-points-in-stroke array */
  pNumPointsInStroke = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) * numStrokes, TAG_PATH);
  if(!pNumPointsInStroke)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  /* Second pass: remember number of points in each polygon */
  iStroke=-1;  /* Will get incremented to 0 at beginning of first stroke */
  for(i=0; i<pPath->numEntriesUsed; i++)
  {
    /* Is this the beginning of a new stroke? */
    if((pPath->pFlags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
    {
      iStroke++;
      pNumPointsInStroke[iStroke]=0;
    }

    pNumPointsInStroke[iStroke]++;
  }

  /* Create a region from the strokes */
  hrgn = IntCreatePolyPolygonRgn( pPath->pPoints,
                              pNumPointsInStroke,
                                      numStrokes,
                                   nPolyFillMode);
  if(hrgn==(HRGN)0)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  /* Free memory for number-of-points-in-stroke array */
  ExFreePoolWithTag(pNumPointsInStroke, TAG_PATH);

  /* Success! */
  *pHrgn=hrgn;
  return TRUE;
}

/* PATH_EmptyPath
 *
 * Removes all entries from the path and sets the path state to PATH_Null.
 */
VOID
FASTCALL
PATH_EmptyPath ( PPATH pPath )
{
  ASSERT(pPath!=NULL);

  pPath->state=PATH_Null;
  pPath->numEntriesUsed=0;
}

/* PATH_AddEntry
 *
 * Adds an entry to the path. For "flags", pass either PT_MOVETO, PT_LINETO
 * or PT_BEZIERTO, optionally ORed with PT_CLOSEFIGURE. Returns TRUE if
 * successful, FALSE otherwise (e.g. if not enough memory was available).
 */
BOOL
FASTCALL
PATH_AddEntry ( PPATH pPath, const POINT *pPoint, BYTE flags )
{
  ASSERT(pPath!=NULL);

  /* FIXME: If newStroke is true, perhaps we want to check that we're
   * getting a PT_MOVETO
   */

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  /* Reserve enough memory for an extra path entry */
  if ( !PATH_ReserveEntries(pPath, pPath->numEntriesUsed+1) )
    return FALSE;

  /* Store information in path entry */
  pPath->pPoints[pPath->numEntriesUsed]=*pPoint;
  pPath->pFlags[pPath->numEntriesUsed]=flags;

  /* If this is PT_CLOSEFIGURE, we have to start a new stroke next time */
  if((flags & PT_CLOSEFIGURE) == PT_CLOSEFIGURE)
    pPath->newStroke=TRUE;

  /* Increment entry count */
  pPath->numEntriesUsed++;

  return TRUE;
}

/* PATH_ReserveEntries
 *
 * Ensures that at least "numEntries" entries (for points and flags) have
 * been allocated; allocates larger arrays and copies the existing entries
 * to those arrays, if necessary. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_ReserveEntries ( PPATH pPath, INT numEntries )
{
  INT   numEntriesToAllocate;
  POINT *pPointsNew;
  BYTE    *pFlagsNew;

  ASSERT(pPath!=NULL);
  ASSERT(numEntries>=0);

  /* Do we have to allocate more memory? */
  if(numEntries > pPath->numEntriesAllocated)
  {
    /* Find number of entries to allocate. We let the size of the array
     * grow exponentially, since that will guarantee linear time
     * complexity. */
    if(pPath->numEntriesAllocated)
    {
      numEntriesToAllocate=pPath->numEntriesAllocated;
      while(numEntriesToAllocate<numEntries)
        numEntriesToAllocate=numEntriesToAllocate*GROW_FACTOR_NUMER/GROW_FACTOR_DENOM;
    } else
       numEntriesToAllocate=numEntries;

    /* Allocate new arrays */
    pPointsNew=(POINT *)ExAllocatePoolWithTag(PagedPool, numEntriesToAllocate * sizeof(POINT), TAG_PATH);
    if(!pPointsNew)
      return FALSE;
    pFlagsNew=(BYTE *)ExAllocatePoolWithTag(PagedPool, numEntriesToAllocate * sizeof(BYTE), TAG_PATH);
    if(!pFlagsNew)
    {
      ExFreePoolWithTag(pPointsNew, TAG_PATH);
      return FALSE;
    }

    /* Copy old arrays to new arrays and discard old arrays */
    if(pPath->pPoints)
    {
      ASSERT(pPath->pFlags);

      memcpy(pPointsNew, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
      memcpy(pFlagsNew, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

      ExFreePoolWithTag(pPath->pPoints, TAG_PATH);
      ExFreePoolWithTag(pPath->pFlags, TAG_PATH);
    }
    pPath->pPoints=pPointsNew;
    pPath->pFlags=pFlagsNew;
    pPath->numEntriesAllocated=numEntriesToAllocate;
  }

  return TRUE;
}

/* PATH_DoArcPart
 *
 * Creates a Bezier spline that corresponds to part of an arc and appends the
 * corresponding points to the path. The start and end angles are passed in
 * "angleStart" and "angleEnd"; these angles should span a quarter circle
 * at most. If "startEntryType" is non-zero, an entry of that type for the first
 * control point is added to the path; otherwise, it is assumed that the current
 * position is equal to the first control point.
 */
BOOL
FASTCALL
PATH_DoArcPart ( PPATH pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BYTE startEntryType )
{
  double  halfAngle, a;
  double  xNorm[4], yNorm[4];
  POINT point;
  int i;

  ASSERT(fabs(angleEnd-angleStart)<=M_PI_2);

  /* FIXME: Is there an easier way of computing this? */

  /* Compute control points */
  halfAngle=(angleEnd-angleStart)/2.0;
  if(fabs(halfAngle)>1e-8)
  {
    a=4.0/3.0*(1-cos(halfAngle))/sin(halfAngle);
    xNorm[0]=cos(angleStart);
    yNorm[0]=sin(angleStart);
    xNorm[1]=xNorm[0] - a*yNorm[0];
    yNorm[1]=yNorm[0] + a*xNorm[0];
    xNorm[3]=cos(angleEnd);
    yNorm[3]=sin(angleEnd);
    xNorm[2]=xNorm[3] + a*yNorm[3];
    yNorm[2]=yNorm[3] - a*xNorm[3];
  } else
    for(i=0; i<4; i++)
    {
      xNorm[i]=cos(angleStart);
      yNorm[i]=sin(angleStart);
    }

  /* Add starting point to path if desired */
  if(startEntryType)
  {
    PATH_ScaleNormalizedPoint(corners, xNorm[0], yNorm[0], &point);
    if(!PATH_AddEntry(pPath, &point, startEntryType))
      return FALSE;
  }

  /* Add remaining control points */
  for(i=1; i<4; i++)
  {
    PATH_ScaleNormalizedPoint(corners, xNorm[i], yNorm[i], &point);
    if(!PATH_AddEntry(pPath, &point, PT_BEZIERTO))
      return FALSE;
  }

  return TRUE;
}

/* PATH_ScaleNormalizedPoint
 *
 * Scales a normalized point (x, y) with respect to the box whose corners are
 * passed in "corners". The point is stored in "*pPoint". The normalized
 * coordinates (-1.0, -1.0) correspond to corners[0], the coordinates
 * (1.0, 1.0) correspond to corners[1].
 */
VOID
FASTCALL
PATH_ScaleNormalizedPoint ( FLOAT_POINT corners[], double x,
   double y, POINT *pPoint )
{
  ASSERT ( corners );
  ASSERT ( pPoint );
  pPoint->x=GDI_ROUND( (double)corners[0].x + (double)(corners[1].x-corners[0].x)*0.5*(x+1.0) );
  pPoint->y=GDI_ROUND( (double)corners[0].y + (double)(corners[1].y-corners[0].y)*0.5*(y+1.0) );
}

/* PATH_NormalizePoint
 *
 * Normalizes a point with respect to the box whose corners are passed in
 * corners. The normalized coordinates are stored in *pX and *pY.
 */
VOID
FASTCALL
PATH_NormalizePoint ( FLOAT_POINT corners[],
   const FLOAT_POINT *pPoint,
   double *pX, double *pY)
{
  ASSERT ( corners );
  ASSERT ( pPoint );
  ASSERT ( pX );
  ASSERT ( pY );
  *pX=(double)(pPoint->x-corners[0].x)/(double)(corners[1].x-corners[0].x) * 2.0 - 1.0;
  *pY=(double)(pPoint->y-corners[0].y)/(double)(corners[1].y-corners[0].y) * 2.0 - 1.0;
}


BOOL FASTCALL PATH_StrokePath(DC *dc, PPATH pPath)
{
    BOOL ret = FALSE;
    INT i=0;
    INT nLinePts, nAlloc;
    POINT *pLinePts = NULL;
    POINT ptViewportOrg, ptWindowOrg;
    SIZE szViewportExt, szWindowExt;
    DWORD mapMode, graphicsMode;
    XFORM xform;
    PDC_ATTR Dc_Attr = dc->pDc_Attr;

    DPRINT("Enter %s\n", __FUNCTION__);

    if (pPath->state != PATH_Closed)
        return FALSE;

    if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;


    /* Save the mapping mode info */
    mapMode = Dc_Attr->iMapMode;
    IntGetViewportExtEx(dc, &szViewportExt);
    IntGetViewportOrgEx(dc, &ptViewportOrg);
    IntGetWindowExtEx(dc, &szWindowExt);
    IntGetWindowOrgEx(dc, &ptWindowOrg);
    
    MatrixS2XForm(&xform, &dc->DcLevel.mxWorldToPage);

    /* Set MM_TEXT */
    Dc_Attr->iMapMode = MM_TEXT;
    Dc_Attr->ptlViewportOrg.x = 0;
    Dc_Attr->ptlViewportOrg.y = 0;
    Dc_Attr->ptlWindowOrg.x = 0;
    Dc_Attr->ptlWindowOrg.y = 0;
    graphicsMode = Dc_Attr->iGraphicsMode;
    Dc_Attr->iGraphicsMode = GM_ADVANCED;
    IntGdiModifyWorldTransform(dc, &xform, MWT_IDENTITY);
    Dc_Attr->iGraphicsMode = graphicsMode;

    /* Allocate enough memory for the worst case without beziers (one PT_MOVETO
     * and the rest PT_LINETO with PT_CLOSEFIGURE at the end) plus some buffer
     * space in case we get one to keep the number of reallocations small. */
    nAlloc = pPath->numEntriesUsed + 1 + 300;
    pLinePts = ExAllocatePoolWithTag(PagedPool, nAlloc * sizeof(POINT), TAG_PATH);
    if(!pLinePts)
    {
        DPRINT1("Can't allocate pool!\n");
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        goto end;
    }
    nLinePts = 0;

    for(i = 0; i < pPath->numEntriesUsed; i++)
    {
        if((i == 0 || (pPath->pFlags[i-1] & PT_CLOSEFIGURE))
                && (pPath->pFlags[i] != PT_MOVETO))
        {
            DPRINT1("Expected PT_MOVETO %s, got path flag %d\n",
                i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
                (INT)pPath->pFlags[i]);
                goto end;
        }

        switch(pPath->pFlags[i])
        {
        case PT_MOVETO:
            DPRINT("Got PT_MOVETO (%ld, %ld)\n",
                pPath->pPoints[i].x, pPath->pPoints[i].y);
            if(nLinePts >= 2) IntGdiPolyline(dc, pLinePts, nLinePts);
            nLinePts = 0;
            pLinePts[nLinePts++] = pPath->pPoints[i];
            break;
        case PT_LINETO:
        case (PT_LINETO | PT_CLOSEFIGURE):
            DPRINT("Got PT_LINETO (%ld, %ld)\n",
              pPath->pPoints[i].x, pPath->pPoints[i].y);
            pLinePts[nLinePts++] = pPath->pPoints[i];
            break;
        case PT_BEZIERTO:
            DPRINT("Got PT_BEZIERTO\n");
            if(pPath->pFlags[i+1] != PT_BEZIERTO ||
               (pPath->pFlags[i+2] & ~PT_CLOSEFIGURE) != PT_BEZIERTO)
            {
                DPRINT1("Path didn't contain 3 successive PT_BEZIERTOs\n");
                ret = FALSE;
                goto end;
            }
            else
            {
                INT nBzrPts, nMinAlloc;
                POINT *pBzrPts = GDI_Bezier(&pPath->pPoints[i-1], 4, &nBzrPts);
                /* Make sure we have allocated enough memory for the lines of
                 * this bezier and the rest of the path, assuming we won't get
                 * another one (since we won't reallocate again then). */
                nMinAlloc = nLinePts + (pPath->numEntriesUsed - i) + nBzrPts;
                if(nAlloc < nMinAlloc)
                {
                    // Reallocate memory

                    POINT *Realloc = NULL;
                    nAlloc = nMinAlloc * 2;

                    Realloc = ExAllocatePoolWithTag(PagedPool,
                        nAlloc * sizeof(POINT),
                        TAG_PATH);

                    if(!Realloc)
                    {
                        DPRINT1("Can't allocate pool!\n");
                        goto end;
                    }

                    memcpy(Realloc, pLinePts, nLinePts*sizeof(POINT));
                    ExFreePoolWithTag(pLinePts, TAG_PATH);
                    pLinePts = Realloc;
                }
                memcpy(&pLinePts[nLinePts], &pBzrPts[1], (nBzrPts - 1) * sizeof(POINT));
                nLinePts += nBzrPts - 1;
                ExFreePoolWithTag(pBzrPts, TAG_BEZIER);
                i += 2;
            }
            break;
        default:
            DPRINT1("Got path flag %d (not supported)\n", (INT)pPath->pFlags[i]);
            goto end;
        }

        if(pPath->pFlags[i] & PT_CLOSEFIGURE)
        {
            pLinePts[nLinePts++] = pLinePts[0];
        }
    }
    if(nLinePts >= 2)
        IntGdiPolyline(dc, pLinePts, nLinePts);

    ret = TRUE;

end:
    if(pLinePts) ExFreePoolWithTag(pLinePts, TAG_PATH);

    /* Restore the old mapping mode */
    Dc_Attr->iMapMode =  mapMode;
    Dc_Attr->szlWindowExt.cx = szWindowExt.cx;
    Dc_Attr->szlWindowExt.cy = szWindowExt.cy;
    Dc_Attr->ptlWindowOrg.x = ptWindowOrg.x;
    Dc_Attr->ptlWindowOrg.y = ptWindowOrg.y;

    Dc_Attr->szlViewportExt.cx = szViewportExt.cx;
    Dc_Attr->szlViewportExt.cy = szViewportExt.cy;
    Dc_Attr->ptlViewportOrg.x = ptViewportOrg.x;
    Dc_Attr->ptlViewportOrg.y = ptViewportOrg.y;

    /* Restore the world transform */
    XForm2MatrixS(&dc->DcLevel.mxWorldToPage, &xform);

    /* If we've moved the current point then get its new position
       which will be in device (MM_TEXT) co-ords, convert it to
       logical co-ords and re-set it.  This basically updates
       dc->CurPosX|Y so that their values are in the correct mapping
       mode.
    */
    if(i > 0)
    {
        POINT pt;
        IntGetCurrentPositionEx(dc, &pt);
        IntDPtoLP(dc, &pt, 1);
        IntGdiMoveToEx(dc, pt.x, pt.y, NULL);
    }
    DPRINT("Leave %s, ret=%d\n", __FUNCTION__, ret);
    return ret;
}

#define round(x) ((int)((x)>0?(x)+0.5:(x)-0.5))

static
BOOL
FASTCALL
PATH_WidenPath(DC *dc)
{
    INT i, j, numStrokes, numOldStrokes, penWidth, penWidthIn, penWidthOut, size, penStyle;
    BOOL ret = FALSE;
    PPATH pPath, pNewPath, *pStrokes, *pOldStrokes, pUpPath, pDownPath;
    EXTLOGPEN *elp;
    DWORD obj_type, joint, endcap, penType;
    PDC_ATTR Dc_Attr = dc->pDc_Attr;

    pPath = PATH_LockPath( dc->DcLevel.hPath );
    if (!pPath) return FALSE;

    if(pPath->state == PATH_Open)
    {
       PATH_UnlockPath( pPath );
       SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
       return FALSE;
    }

    if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

    PATH_FlattenPath(pPath);

    size = IntGdiGetObject( Dc_Attr->hpen, 0, NULL);
    if (!size)
    {
        PATH_UnlockPath( pPath );
        SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }

    elp = ExAllocatePoolWithTag(PagedPool, size, TAG_PATH);
    (VOID) IntGdiGetObject( Dc_Attr->hpen, size, elp);

    obj_type = GDIOBJ_GetObjectType(Dc_Attr->hpen);
    if(obj_type == GDI_OBJECT_TYPE_PEN)
    {
        penStyle = ((LOGPEN*)elp)->lopnStyle;
    }
    else if(obj_type == GDI_OBJECT_TYPE_EXTPEN)
    {
        penStyle = elp->elpPenStyle;
    }
    else
    {
        SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
        ExFreePoolWithTag(elp, TAG_PATH);
        PATH_UnlockPath( pPath );
        return FALSE;
    }

    penWidth = elp->elpWidth;
    ExFreePoolWithTag(elp, TAG_PATH);

    endcap = (PS_ENDCAP_MASK & penStyle);
    joint = (PS_JOIN_MASK & penStyle);
    penType = (PS_TYPE_MASK & penStyle);

    /* The function cannot apply to cosmetic pens */
    if(obj_type == GDI_OBJECT_TYPE_EXTPEN && penType == PS_COSMETIC)
    {
        PATH_UnlockPath( pPath );
        SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }

    penWidthIn = penWidth / 2;
    penWidthOut = penWidth / 2;
    if(penWidthIn + penWidthOut < penWidth)
        penWidthOut++;

    numStrokes = 0;
    numOldStrokes = 1;

    pStrokes    = ExAllocatePoolWithTag(PagedPool, sizeof(PPATH), TAG_PATH);
    pStrokes[0] = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);
    PATH_InitGdiPath(pStrokes[0]);
    pStrokes[0]->pFlags =   ExAllocatePoolWithTag(PagedPool, pPath->numEntriesUsed * sizeof(INT), TAG_PATH);
    pStrokes[0]->pPoints =  ExAllocatePoolWithTag(PagedPool, pPath->numEntriesUsed * sizeof(POINT), TAG_PATH);
    pStrokes[0]->numEntriesUsed = 0;

    for(i = 0, j = 0; i < pPath->numEntriesUsed; i++, j++)
    {
        POINT point;
        if((i == 0 || (pPath->pFlags[i-1] & PT_CLOSEFIGURE)) &&
            (pPath->pFlags[i] != PT_MOVETO))
        {
            DPRINT1("Expected PT_MOVETO %s, got path flag %c\n",
                i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
                pPath->pFlags[i]);
            return FALSE;
        }
        switch(pPath->pFlags[i])
        {
            case PT_MOVETO:
                if(numStrokes > 0)
                {
                    pStrokes[numStrokes - 1]->state = PATH_Closed;
                }
                numStrokes++;
                j = 0;
                pOldStrokes = pStrokes; // Save old pointer.
                pStrokes = ExAllocatePoolWithTag(PagedPool, numStrokes * sizeof(PPATH), TAG_PATH);
                RtlCopyMemory(pStrokes, pOldStrokes, numOldStrokes * sizeof(PPATH));
                numOldStrokes = numStrokes; // Save orig count.
                ExFreePoolWithTag(pOldStrokes, TAG_PATH); // Free old pointer.
                pStrokes[numStrokes - 1] = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);

                PATH_InitGdiPath(pStrokes[numStrokes - 1]);
                pStrokes[numStrokes - 1]->state = PATH_Open;
            case PT_LINETO:
            case (PT_LINETO | PT_CLOSEFIGURE):
                point.x = pPath->pPoints[i].x;
                point.y = pPath->pPoints[i].y;
                PATH_AddEntry(pStrokes[numStrokes - 1], &point, pPath->pFlags[i]);
                break;
            case PT_BEZIERTO:
                /* should never happen because of the FlattenPath call */
                DPRINT1("Should never happen\n");
                break;
            default:
                DPRINT1("Got path flag %c\n", pPath->pFlags[i]);
                return FALSE;
        }
    }

    pNewPath = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);  
    PATH_InitGdiPath(pNewPath);
    pNewPath->state = PATH_Open;

    for(i = 0; i < numStrokes; i++)
    {
        pUpPath = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);
        PATH_InitGdiPath(pUpPath);
        pUpPath->state = PATH_Open;
        pDownPath = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);
        PATH_InitGdiPath(pDownPath);
        pDownPath->state = PATH_Open;

        for(j = 0; j < pStrokes[i]->numEntriesUsed; j++)
        {
            /* Beginning or end of the path if not closed */
            if((!(pStrokes[i]->pFlags[pStrokes[i]->numEntriesUsed - 1] & PT_CLOSEFIGURE)) && (j == 0 || j == pStrokes[i]->numEntriesUsed - 1) )
            {
                /* Compute segment angle */
                double xo, yo, xa, ya, theta;
                POINT pt;
                FLOAT_POINT corners[2];
                if(j == 0)
                {
                    xo = pStrokes[i]->pPoints[j].x;
                    yo = pStrokes[i]->pPoints[j].y;
                    xa = pStrokes[i]->pPoints[1].x;
                    ya = pStrokes[i]->pPoints[1].y;
                }
                else
                {
                    xa = pStrokes[i]->pPoints[j - 1].x;
                    ya = pStrokes[i]->pPoints[j - 1].y;
                    xo = pStrokes[i]->pPoints[j].x;
                    yo = pStrokes[i]->pPoints[j].y;
                }
                theta = atan2( ya - yo, xa - xo );
                switch(endcap)
                {
                    case PS_ENDCAP_SQUARE :
                        pt.x = xo + round(sqrt(2) * penWidthOut * cos(M_PI_4 + theta));
                        pt.y = yo + round(sqrt(2) * penWidthOut * sin(M_PI_4 + theta));
                        PATH_AddEntry(pUpPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO) );
                        pt.x = xo + round(sqrt(2) * penWidthIn * cos(- M_PI_4 + theta));
                        pt.y = yo + round(sqrt(2) * penWidthIn * sin(- M_PI_4 + theta));
                        PATH_AddEntry(pUpPath, &pt, PT_LINETO);
                        break;
                    case PS_ENDCAP_FLAT :
                        pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                        pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        PATH_AddEntry(pUpPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO));
                        pt.x = xo - round( penWidthIn * cos(theta + M_PI_2) );
                        pt.y = yo - round( penWidthIn * sin(theta + M_PI_2) );
                        PATH_AddEntry(pUpPath, &pt, PT_LINETO);
                        break;
                    case PS_ENDCAP_ROUND :
                    default :
                        corners[0].x = xo - penWidthIn;
                        corners[0].y = yo - penWidthIn;
                        corners[1].x = xo + penWidthOut;
                        corners[1].y = yo + penWidthOut;
                        PATH_DoArcPart(pUpPath ,corners, theta + M_PI_2 , theta + 3 * M_PI_4, (j == 0 ? PT_MOVETO : FALSE));
                        PATH_DoArcPart(pUpPath ,corners, theta + 3 * M_PI_4 , theta + M_PI, FALSE);
                        PATH_DoArcPart(pUpPath ,corners, theta + M_PI, theta +  5 * M_PI_4, FALSE);
                        PATH_DoArcPart(pUpPath ,corners, theta + 5 * M_PI_4 , theta + 3 * M_PI_2, FALSE);
                        break;
                }
            }
            /* Corpse of the path */
            else
            {
                /* Compute angle */
                INT previous, next;
                double xa, ya, xb, yb, xo, yo;
                double alpha, theta, miterWidth;
                DWORD _joint = joint;
                POINT pt;
		PPATH pInsidePath, pOutsidePath;
                if(j > 0 && j < pStrokes[i]->numEntriesUsed - 1)
                {
                    previous = j - 1;
                    next = j + 1;
                }
                else if (j == 0)
                {
                    previous = pStrokes[i]->numEntriesUsed - 1;
                    next = j + 1;
                }
                else
                {
                    previous = j - 1;
                    next = 0;
                }
                xo = pStrokes[i]->pPoints[j].x;
                yo = pStrokes[i]->pPoints[j].y;
                xa = pStrokes[i]->pPoints[previous].x;
                ya = pStrokes[i]->pPoints[previous].y;
                xb = pStrokes[i]->pPoints[next].x;
                yb = pStrokes[i]->pPoints[next].y;
                theta = atan2( yo - ya, xo - xa );
                alpha = atan2( yb - yo, xb - xo ) - theta;
                if (alpha > 0) alpha -= M_PI;
                else alpha += M_PI;
                if(_joint == PS_JOIN_MITER && dc->DcLevel.laPath.eMiterLimit < fabs(1 / sin(alpha/2)))
                {
                    _joint = PS_JOIN_BEVEL;
                }
                if(alpha > 0)
                {
                    pInsidePath = pUpPath;
                    pOutsidePath = pDownPath;
                }
                else if(alpha < 0)
                {
                    pInsidePath = pDownPath;
                    pOutsidePath = pUpPath;
                }
                else
                {
                    continue;
                }
                /* Inside angle points */
                if(alpha > 0)
                {
                    pt.x = xo - round( penWidthIn * cos(theta + M_PI_2) );
                    pt.y = yo - round( penWidthIn * sin(theta + M_PI_2) );
                }
                else
                {
                    pt.x = xo + round( penWidthIn * cos(theta + M_PI_2) );
                    pt.y = yo + round( penWidthIn * sin(theta + M_PI_2) );
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                if(alpha > 0)
                {
                    pt.x = xo + round( penWidthIn * cos(M_PI_2 + alpha + theta) );
                    pt.y = yo + round( penWidthIn * sin(M_PI_2 + alpha + theta) );
                }
                else
                {
                    pt.x = xo - round( penWidthIn * cos(M_PI_2 + alpha + theta) );
                    pt.y = yo - round( penWidthIn * sin(M_PI_2 + alpha + theta) );
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                /* Outside angle point */
                switch(_joint)
                {
                     case PS_JOIN_MITER :
                        miterWidth = fabs(penWidthOut / cos(M_PI_2 - fabs(alpha) / 2));
                        pt.x = xo + round( miterWidth * cos(theta + alpha / 2) );
                        pt.y = yo + round( miterWidth * sin(theta + alpha / 2) );
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_BEVEL :
                        if(alpha > 0)
                        {
                            pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        else
                        {
                            pt.x = xo - round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo - round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        if(alpha > 0)
                        {
                            pt.x = xo - round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo - round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        else
                        {
                            pt.x = xo + round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo + round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_ROUND :
                    default :
                        if(alpha > 0)
                        {
                            pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        else
                        {
                            pt.x = xo - round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo - round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        pt.x = xo + round( penWidthOut * cos(theta + alpha / 2) );
                        pt.y = yo + round( penWidthOut * sin(theta + alpha / 2) );
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        if(alpha > 0)
                        {
                            pt.x = xo - round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo - round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        else
                        {
                            pt.x = xo + round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo + round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        break;
                }
            }
        }
        for(j = 0; j < pUpPath->numEntriesUsed; j++)
        {
            POINT pt;
            pt.x = pUpPath->pPoints[j].x;
            pt.y = pUpPath->pPoints[j].y;
            PATH_AddEntry(pNewPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO));
        }
        for(j = 0; j < pDownPath->numEntriesUsed; j++)
        {
            POINT pt;
            pt.x = pDownPath->pPoints[pDownPath->numEntriesUsed - j - 1].x;
            pt.y = pDownPath->pPoints[pDownPath->numEntriesUsed - j - 1].y;
            PATH_AddEntry(pNewPath, &pt, ( (j == 0 && (pStrokes[i]->pFlags[pStrokes[i]->numEntriesUsed - 1] & PT_CLOSEFIGURE)) ? PT_MOVETO : PT_LINETO));
        }

        PATH_DestroyGdiPath(pStrokes[i]);
        ExFreePoolWithTag(pStrokes[i], TAG_PATH);
        PATH_DestroyGdiPath(pUpPath);
        ExFreePoolWithTag(pUpPath, TAG_PATH);
        PATH_DestroyGdiPath(pDownPath);
        ExFreePoolWithTag(pDownPath, TAG_PATH);
    }
    ExFreePoolWithTag(pStrokes, TAG_PATH);

    pNewPath->state = PATH_Closed;
    if (!(ret = PATH_AssignGdiPath(pPath, pNewPath)))
        DPRINT1("Assign path failed\n");
    PATH_DestroyGdiPath(pNewPath);
    ExFreePoolWithTag(pNewPath, TAG_PATH);
    return ret;
}

static inline INT int_from_fixed(FIXED f)
{
    return (f.fract >= 0x8000) ? (f.value + 1) : f.value;
}

/**********************************************************************
 *      PATH_BezierTo
 *
 * internally used by PATH_add_outline
 */
static
VOID
FASTCALL
PATH_BezierTo(PPATH pPath, POINT *lppt, INT n)
{
    if (n < 2) return;

    if (n == 2)
    {
        PATH_AddEntry(pPath, &lppt[1], PT_LINETO);
    }
    else if (n == 3)
    {
        PATH_AddEntry(pPath, &lppt[0], PT_BEZIERTO);
        PATH_AddEntry(pPath, &lppt[1], PT_BEZIERTO);
        PATH_AddEntry(pPath, &lppt[2], PT_BEZIERTO);
    }
    else
    {
        POINT pt[3];
        INT i = 0;

        pt[2] = lppt[0];
        n--;

        while (n > 2)
        {
            pt[0] = pt[2];
            pt[1] = lppt[i+1];
            pt[2].x = (lppt[i+2].x + lppt[i+1].x) / 2;
            pt[2].y = (lppt[i+2].y + lppt[i+1].y) / 2;
            PATH_BezierTo(pPath, pt, 3);
            n--;
            i++;
        }

        pt[0] = pt[2];
        pt[1] = lppt[i+1];
        pt[2] = lppt[i+2];
        PATH_BezierTo(pPath, pt, 3);
    }
}

static
BOOL
FASTCALL
PATH_add_outline(PDC dc, INT x, INT y, TTPOLYGONHEADER *header, DWORD size)
{
  PPATH pPath;
  TTPOLYGONHEADER *start;
  POINT pt;

  start = header;

  pPath = PATH_LockPath(dc->DcLevel.hPath);
  {
     return FALSE;
  }

  while ((char *)header < (char *)start + size)
  {
     TTPOLYCURVE *curve;

     if (header->dwType != TT_POLYGON_TYPE)
     {
        DPRINT1("Unknown header type %d\n", header->dwType);
        return FALSE;
     }

     pt.x = x + int_from_fixed(header->pfxStart.x);
     pt.y = y - int_from_fixed(header->pfxStart.y);
     IntLPtoDP(dc, &pt, 1);
     PATH_AddEntry(pPath, &pt, PT_MOVETO);

     curve = (TTPOLYCURVE *)(header + 1);

     while ((char *)curve < (char *)header + header->cb)
     {
        /*DPRINT1("curve->wType %d\n", curve->wType);*/

        switch(curve->wType)
        {
           case TT_PRIM_LINE:
           {
              WORD i;

              for (i = 0; i < curve->cpfx; i++)
              {
                 pt.x = x + int_from_fixed(curve->apfx[i].x);
                 pt.y = y - int_from_fixed(curve->apfx[i].y);
                 IntLPtoDP(dc, &pt, 1);
                 PATH_AddEntry(pPath, &pt, PT_LINETO);
              }
              break;
           }

           case TT_PRIM_QSPLINE:
           case TT_PRIM_CSPLINE:
           {
              WORD i;
              POINTFX ptfx;
              POINT *pts = ExAllocatePoolWithTag(PagedPool, (curve->cpfx + 1) * sizeof(POINT), TAG_PATH);

              if (!pts) return FALSE;

              ptfx = *(POINTFX *)((char *)curve - sizeof(POINTFX));

              pts[0].x = x + int_from_fixed(ptfx.x);
              pts[0].y = y - int_from_fixed(ptfx.y);
              IntLPtoDP(dc, &pts[0], 1);

              for (i = 0; i < curve->cpfx; i++)
              {
                  pts[i + 1].x = x + int_from_fixed(curve->apfx[i].x);
                  pts[i + 1].y = y - int_from_fixed(curve->apfx[i].y);
                  IntLPtoDP(dc, &pts[i + 1], 1);
              }

              PATH_BezierTo(pPath, pts, curve->cpfx + 1);

              ExFreePoolWithTag(pts, TAG_PATH);
              break;
           }

           default:
              DPRINT1("Unknown curve type %04x\n", curve->wType);
              return FALSE;
        }

        curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
     }
     header = (TTPOLYGONHEADER *)((char *)header + header->cb);
  }

  IntGdiCloseFigure( pPath );
  PATH_UnlockPath( pPath );     
  return TRUE;
}

/**********************************************************************
 *      PATH_ExtTextOut
 */
BOOL
FASTCALL 
PATH_ExtTextOut(PDC dc, INT x, INT y, UINT flags, const RECTL *lprc,
                     LPCWSTR str, UINT count, const INT *dx)
{
    unsigned int idx;
    double cosEsc, sinEsc;
    PDC_ATTR Dc_Attr;
    PTEXTOBJ TextObj;
    LOGFONTW lf;
    POINTL org;
    INT offset = 0, xoff = 0, yoff = 0;

    if (!count) return TRUE;

    Dc_Attr = dc->pDc_Attr;
    if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

    TextObj = RealizeFontInit( Dc_Attr->hlfntNew);
    if ( !TextObj ) return FALSE;

    FontGetObject( TextObj, sizeof(lf), &lf);

    if (lf.lfEscapement != 0)
    {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    } else
    {
        cosEsc = 1;
        sinEsc = 0;
    }

    IntGdiGetDCOrg(dc, &org);

    for (idx = 0; idx < count; idx++)
    {
        GLYPHMETRICS gm;
        DWORD dwSize;
        void *outline;

        dwSize = ftGdiGetGlyphOutline( dc,
                                       str[idx],
                                       GGO_GLYPH_INDEX | GGO_NATIVE,
                                       &gm,
                                       0,
                                       NULL,
                                       NULL,
                                       TRUE);
        if (!dwSize) return FALSE;

        outline = ExAllocatePoolWithTag(PagedPool, dwSize, TAG_PATH);
        if (!outline) return FALSE;

        ftGdiGetGlyphOutline( dc,
                              str[idx],
                              GGO_GLYPH_INDEX | GGO_NATIVE,
                              &gm,
                              dwSize,
                              outline,
                              NULL,
                              TRUE);

        PATH_add_outline(dc, org.x + x + xoff, org.x + y + yoff, outline, dwSize);

        ExFreePoolWithTag(outline, TAG_PATH);

        if (dx)
        {
            offset += dx[idx];
            xoff = offset * cosEsc;
            yoff = offset * -sinEsc;
        }
        else
        {
            xoff += gm.gmCellIncX;
            yoff += gm.gmCellIncY;
        }
    }
    return TRUE;
}


/***********************************************************************
 * Exported functions
 */

BOOL
APIENTRY
NtGdiAbortPath(HDC  hDC)
{
  PPATH pPath;
  PDC dc = DC_LockDc ( hDC );
  if ( !dc )
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  pPath = PATH_LockPath(dc->DcLevel.hPath);
  {
      DC_UnlockDc(dc);
      return FALSE;
  }

  PATH_EmptyPath(pPath);

  PATH_UnlockPath(pPath);
  DC_UnlockDc ( dc );
  return TRUE;
}

BOOL
APIENTRY
NtGdiBeginPath( HDC  hDC )
{
  PPATH pPath;
  PDC dc;

  dc = DC_LockDc ( hDC );
  if ( !dc )
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  /* If path is already open, do nothing. Check if not Save DC state */
    if ((dc->DcLevel.flPath & DCPATH_ACTIVE) && !(dc->DcLevel.flPath & DCPATH_SAVE))
  {
     DC_UnlockDc ( dc );
     return TRUE;
  }

  if ( dc->DcLevel.hPath )
  {
     DPRINT1("BeginPath 1 0x%x\n", dc->DcLevel.hPath);
     if ( !(dc->DcLevel.flPath & DCPATH_SAVE) )
     {  // Remove previous handle.
        if (!PATH_Delete(dc->DcLevel.hPath))
        {
           DC_UnlockDc ( dc );
           return FALSE;
        }
     }
     else
     {  // Clear flags and Handle.
        dc->DcLevel.flPath &= ~(DCPATH_SAVE|DCPATH_ACTIVE);
        dc->DcLevel.hPath = NULL;
     }
  }
  pPath = PATH_AllocPathWithHandle();
  if (!pPath)
  {
     SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
     return FALSE;
  }
  dc->DcLevel.flPath |= DCPATH_ACTIVE; // Set active ASAP!

  dc->DcLevel.hPath = pPath->BaseObject.hHmgr;

  DPRINT1("BeginPath 2 h 0x%x p 0x%x\n", dc->DcLevel.hPath, pPath);
  // Path handles are shared. Also due to recursion with in the same thread.
  GDIOBJ_UnlockObjByPtr((POBJ)pPath);       // Unlock
  pPath = PATH_LockPath(dc->DcLevel.hPath); // Share Lock.

  /* Make sure that path is empty */
  PATH_EmptyPath( pPath );

  /* Initialize variables for new path */
  pPath->newStroke = TRUE;
  pPath->state = PATH_Open;

  PATH_UnlockPath(pPath);
  DC_UnlockDc ( dc );
  return TRUE;
}

BOOL
APIENTRY
NtGdiCloseFigure(HDC hDC)
{
  BOOL Ret = FALSE; // default to failure
  PDC pDc;
  PPATH pPath;

  DPRINT("Enter %s\n", __FUNCTION__);

  pDc = DC_LockDc(hDC);
  if (!pDc)
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
  }   
  pPath = PATH_LockPath( pDc->DcLevel.hPath );
  if (!pPath)
  {
     DC_UnlockDc(pDc);
     return FALSE;
  }

  if (pPath->state==PATH_Open)
  {
     IntGdiCloseFigure(pPath);
     Ret = TRUE;
  }
  else
  {
     // FIXME: check if lasterror is set correctly
     SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
  }

  PATH_UnlockPath( pPath );
  DC_UnlockDc(pDc);
  return Ret;
}

BOOL
APIENTRY
NtGdiEndPath(HDC  hDC)
{
  BOOL ret = TRUE;
  PPATH pPath;
  PDC dc = DC_LockDc ( hDC );

  if ( !dc )
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath)
  {
     DC_UnlockDc ( dc );
     return FALSE;
  }
  /* Check that path is currently being constructed */
  if ( (pPath->state != PATH_Open) || !(dc->DcLevel.flPath & DCPATH_ACTIVE) )
  {
    DPRINT1("EndPath ERROR! 0x%x\n", dc->DcLevel.hPath);
    SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
    ret = FALSE;
  }
  /* Set flag to indicate that path is finished */
  else
  {
     DPRINT1("EndPath 0x%x\n", dc->DcLevel.hPath);
     pPath->state = PATH_Closed;
     dc->DcLevel.flPath &= ~DCPATH_ACTIVE;
  }
  PATH_UnlockPath( pPath );
  DC_UnlockDc ( dc );
  return ret;
}

BOOL
APIENTRY
NtGdiFillPath(HDC  hDC)
{
  BOOL ret = FALSE;
  PPATH pPath;
  PDC_ATTR pDc_Attr;
  PDC dc = DC_LockDc ( hDC );
 
  if ( !dc )
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
  }
  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath)
  {
     DC_UnlockDc ( dc );
     return FALSE;
  }

  pDc_Attr = dc->pDc_Attr;
  if (!pDc_Attr) pDc_Attr = &dc->Dc_Attr;

  if (pDc_Attr->ulDirty_ & DC_BRUSH_DIRTY)
     IntGdiSelectBrush(dc,pDc_Attr->hbrush);

  ret = PATH_FillPath( dc, pPath );
  if ( ret )
  {
    /* FIXME: Should the path be emptied even if conversion
       failed? */
    PATH_EmptyPath( pPath );
  }

  PATH_UnlockPath( pPath );
  DC_UnlockDc ( dc );
  return ret;
}

BOOL
APIENTRY
NtGdiFlattenPath(HDC  hDC)
{
   BOOL Ret = FALSE;
   DC *pDc;
   PPATH pPath;

   DPRINT("Enter %s\n", __FUNCTION__);

   pDc = DC_LockDc(hDC);
   if (!pDc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);  
      return FALSE;
   }

   pPath = PATH_LockPath( pDc->DcLevel.hPath );
   if (!pPath)
   {
      DC_UnlockDc ( pDc );
      return FALSE;
   }
   if (pPath->state == PATH_Open)
      Ret = PATH_FlattenPath(pPath);

   PATH_UnlockPath( pPath );
   DC_UnlockDc(pDc);
   return Ret;
}


BOOL
APIENTRY
NtGdiGetMiterLimit(
    IN HDC hdc,
    OUT PDWORD pdwOut)
{
  DC *pDc;
  gxf_long worker;
  NTSTATUS Status = STATUS_SUCCESS;

  if (!(pDc = DC_LockDc(hdc)))
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
  }

  worker.f = pDc->DcLevel.laPath.eMiterLimit;

  if (pdwOut)
  {
      _SEH2_TRY
      {
          ProbeForWrite(pdwOut,
                 sizeof(DWORD),
                             1);
          *pdwOut = worker.l;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
          Status = _SEH2_GetExceptionCode();
      }
       _SEH2_END;
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         DC_UnlockDc(pDc);
         return FALSE;
      }
  }

  DC_UnlockDc(pDc);
  return TRUE;

}

INT
APIENTRY
NtGdiGetPath(
   HDC hDC,
   LPPOINT Points,
   LPBYTE Types,
   INT nSize)
{
  INT ret = -1;
  PPATH pPath;

  DC *dc = DC_LockDc(hDC);
  if (!dc)
  {
     DPRINT1("Can't lock dc!\n");
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return -1;
  }

  pPath = PATH_LockPath( dc->DcLevel.hPath );
  if (!pPath)
  {
     DC_UnlockDc ( dc );
     return -1;
  }

  if (pPath->state != PATH_Closed)
  {
     SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
     goto done;
  }

  if (nSize==0)
  {
     ret = pPath->numEntriesUsed;
  }
  else if(nSize<pPath->numEntriesUsed)
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     goto done;
  }
  else
  {
      _SEH2_TRY
      {
         memcpy(Points, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
         memcpy(Types, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

         /* Convert the points to logical coordinates */
         IntDPtoLP(dc, Points, pPath->numEntriesUsed);

         ret = pPath->numEntriesUsed;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         SetLastNtError(_SEH2_GetExceptionCode());
      }
      _SEH2_END
  }

done:
  PATH_UnlockPath( pPath );
  DC_UnlockDc(dc);
  return ret;
}

HRGN
APIENTRY
NtGdiPathToRegion(HDC  hDC)
{
  PPATH pPath;
  HRGN  hrgnRval = 0;
  DC *pDc;
  PDC_ATTR Dc_Attr;

  DPRINT("Enter %s\n", __FUNCTION__);

  pDc = DC_LockDc(hDC);
  if (!pDc)
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return NULL;
  }

  Dc_Attr = pDc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &pDc->Dc_Attr;

  pPath = PATH_LockPath( pDc->DcLevel.hPath );
  if (!pPath)
  {
     DC_UnlockDc ( pDc );
     return NULL;
  }

  if (pPath->state!=PATH_Closed)
  {
     //FIXME: check that setlasterror is being called correctly
     SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
  }
  else
  {
     /* FIXME: Should we empty the path even if conversion failed? */
     if(PATH_PathToRegion(pPath, Dc_Attr->jFillMode, &hrgnRval))
          PATH_EmptyPath(pPath);
  }

  PATH_UnlockPath( pPath );
  DC_UnlockDc(pDc);
  return hrgnRval;
}

BOOL
APIENTRY
NtGdiSetMiterLimit(
    IN HDC hdc,
    IN DWORD dwNew,
    IN OUT OPTIONAL PDWORD pdwOut)
{
  DC *pDc;
  gxf_long worker, worker1;
  NTSTATUS Status = STATUS_SUCCESS;

  if (!(pDc = DC_LockDc(hdc)))
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
  }

  worker.l  = dwNew;
  worker1.f = pDc->DcLevel.laPath.eMiterLimit;
  pDc->DcLevel.laPath.eMiterLimit = worker.f;

  if (pdwOut)
  {
      _SEH2_TRY
      {
          ProbeForWrite(pdwOut,
                 sizeof(DWORD),
                             1);
          *pdwOut = worker1.l;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
          Status = _SEH2_GetExceptionCode();
      }
       _SEH2_END;
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         DC_UnlockDc(pDc);
         return FALSE;
      }
  }

  DC_UnlockDc(pDc);
  return TRUE;
}

BOOL
APIENTRY
NtGdiStrokeAndFillPath(HDC hDC)
{
  DC *pDc;
  PDC_ATTR pDc_Attr;
  PPATH pPath;
  BOOL bRet = FALSE;

  DPRINT1("Enter %s\n", __FUNCTION__);

  if (!(pDc = DC_LockDc(hDC)))
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
  }
  pPath = PATH_LockPath( pDc->DcLevel.hPath );
  if (!pPath)
  {
     DC_UnlockDc ( pDc );
     return FALSE;
  }

  pDc_Attr = pDc->pDc_Attr;
  if (!pDc_Attr) pDc_Attr = &pDc->Dc_Attr;

  if (pDc_Attr->ulDirty_ & DC_BRUSH_DIRTY)
     IntGdiSelectBrush(pDc,pDc_Attr->hbrush);
  if (pDc_Attr->ulDirty_ & DC_PEN_DIRTY)
     IntGdiSelectPen(pDc,pDc_Attr->hpen);

  bRet = PATH_FillPath(pDc, pPath);
  if (bRet) bRet = PATH_StrokePath(pDc, pPath);
  if (bRet) PATH_EmptyPath(pPath);

  PATH_UnlockPath( pPath );
  DC_UnlockDc(pDc);
  return bRet;
}

BOOL
APIENTRY
NtGdiStrokePath(HDC hDC)
{
  DC *pDc;
  PDC_ATTR pDc_Attr;
  PPATH pPath;
  BOOL bRet = FALSE;

  DPRINT("Enter %s\n", __FUNCTION__);

  if (!(pDc = DC_LockDc(hDC)))
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
  }
  pPath = PATH_LockPath( pDc->DcLevel.hPath );
  if (!pPath)
  {
     DC_UnlockDc ( pDc );
     return FALSE;
  }

  pDc_Attr = pDc->pDc_Attr;
  if (!pDc_Attr) pDc_Attr = &pDc->Dc_Attr;

  if (pDc_Attr->ulDirty_ & DC_PEN_DIRTY)
     IntGdiSelectPen(pDc,pDc_Attr->hpen);

  bRet = PATH_StrokePath(pDc, pPath);
  PATH_EmptyPath(pPath);

  PATH_UnlockPath( pPath );
  DC_UnlockDc(pDc);
  return bRet;
}

BOOL
APIENTRY
NtGdiWidenPath(HDC  hDC)
{
  BOOL Ret;
  PDC pdc = DC_LockDc ( hDC );    
  if ( !pdc )
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
  }
  Ret = PATH_WidenPath(pdc);
  DC_UnlockDc ( pdc );
  return Ret;
}

BOOL
APIENTRY
NtGdiSelectClipPath(HDC  hDC,
                   int  Mode)
{
 HRGN  hrgnPath;
 PPATH pPath;
 BOOL  success = FALSE;
 PDC_ATTR Dc_Attr;
 PDC dc = DC_LockDc ( hDC );
 
 if ( !dc )
 {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
 }

 Dc_Attr = dc->pDc_Attr;
 if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

 pPath = PATH_LockPath( dc->DcLevel.hPath );
 if (!pPath)
 {
    DC_UnlockDc ( dc );
    return FALSE;
 }
 /* Check that path is closed */
 if( pPath->state != PATH_Closed )
 {
   SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
   return FALSE;
 }
 /* Construct a region from the path */
 else if( PATH_PathToRegion( pPath, Dc_Attr->jFillMode, &hrgnPath ) )
 {
   success = GdiExtSelectClipRgn( dc, hrgnPath, Mode ) != ERROR;
   NtGdiDeleteObject( hrgnPath );

   /* Empty the path */
   if( success )
     PATH_EmptyPath( pPath);
   /* FIXME: Should this function delete the path even if it failed? */
 }
 PATH_UnlockPath( pPath );
 DC_UnlockDc ( dc );
 return success;
}

/* EOF */
