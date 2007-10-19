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
/* $Id$ */

#include <w32k.h>
#include "math.h"

#define NDEBUG
#include <debug.h>

#define NUM_ENTRIES_INITIAL 16  /* Initial size of points / flags arrays  */
#define GROW_FACTOR_NUMER    2  /* Numerator of grow factor for the array */
#define GROW_FACTOR_DENOM    1  /* Denominator of grow factor             */

BOOL FASTCALL PATH_AddEntry (GdiPath *pPath, const POINT *pPoint, BYTE flags);
BOOL FASTCALL PATH_AddFlatBezier (GdiPath *pPath, POINT *pt, BOOL closed);
BOOL FASTCALL PATH_DoArcPart (GdiPath *pPath, FLOAT_POINT corners[], double angleStart, double angleEnd, BOOL addMoveTo);
BOOL FASTCALL PATH_FillPath( PDC dc, GdiPath *pPath );
BOOL FASTCALL PATH_FlattenPath (GdiPath *pPath);
VOID FASTCALL PATH_NormalizePoint (FLOAT_POINT corners[], const FLOAT_POINT *pPoint, double *pX, double *pY);
BOOL FASTCALL PATH_PathToRegion (GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn);
BOOL FASTCALL PATH_ReserveEntries (GdiPath *pPath, INT numEntries);
VOID FASTCALL PATH_ScaleNormalizedPoint (FLOAT_POINT corners[], double x, double y, POINT *pPoint);
BOOL FASTCALL PATH_StrokePath(DC *dc, GdiPath *pPath);
BOOL PATH_CheckCorners(DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2);

VOID FASTCALL
IntGetCurrentPositionEx(PDC dc, LPPOINT pt);


BOOL
STDCALL
NtGdiAbortPath(HDC  hDC)
{
  BOOL ret = TRUE;
  PDC dc = DC_LockDc ( hDC );

  if( !dc ) return FALSE;

  PATH_EmptyPath(&dc->w.path);

  DC_UnlockDc ( dc );
  return ret;
}

BOOL
STDCALL
NtGdiBeginPath( HDC  hDC )
{
  BOOL ret = TRUE;
  PDC dc = DC_LockDc ( hDC );

  if( !dc ) return FALSE;

  /* If path is already open, do nothing */
  if ( dc->w.path.state != PATH_Open )
  {
    /* Make sure that path is empty */
    PATH_EmptyPath( &dc->w.path );

    /* Initialize variables for new path */
    dc->w.path.newStroke = TRUE;
    dc->w.path.state = PATH_Open;
  }

  DC_UnlockDc ( dc );
  return ret;
}

VOID
FASTCALL
IntGdiCloseFigure(PDC pDc)
{
   ASSERT(pDc);
   ASSERT(pDc->w.path.state == PATH_Open);

   // FIXME: Shouldn't we draw a line to the beginning of the figure?
   // Set PT_CLOSEFIGURE on the last entry and start a new stroke
   if(pDc->w.path.numEntriesUsed)
   {
      pDc->w.path.pFlags[pDc->w.path.numEntriesUsed-1]|=PT_CLOSEFIGURE;
      pDc->w.path.newStroke=TRUE;
   }
}

BOOL
STDCALL
NtGdiCloseFigure(HDC hDC)
{
   BOOL Ret = FALSE; // default to failure
   PDC pDc;

   DPRINT("Enter %s\n", __FUNCTION__);

   pDc = DC_LockDc(hDC);
   if(!pDc) return FALSE;

   if(pDc->w.path.state==PATH_Open)
   {
      IntGdiCloseFigure(pDc);
      Ret = TRUE;
   }
   else
   {
      // FIXME: check if lasterror is set correctly
      SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
   }

   DC_UnlockDc(pDc);

   return Ret;
}

BOOL
STDCALL
NtGdiEndPath(HDC  hDC)
{
  BOOL ret = TRUE;
  PDC dc = DC_LockDc ( hDC );

  if ( !dc ) return FALSE;

  /* Check that path is currently being constructed */
  if( dc->w.path.state != PATH_Open )
  {
    ret = FALSE;
  }
  /* Set flag to indicate that path is finished */
  else dc->w.path.state = PATH_Closed;

  DC_UnlockDc ( dc );
  return ret;
}

BOOL
STDCALL
NtGdiFillPath(HDC  hDC)
{
  BOOL ret = TRUE;
  PDC dc = DC_LockDc ( hDC );

  if ( !dc ) return FALSE;

  ret = PATH_FillPath( dc, &dc->w.path );
  if( ret )
  {
    /* FIXME: Should the path be emptied even if conversion
       failed? */
    PATH_EmptyPath( &dc->w.path );
  }

  DC_UnlockDc ( dc );
  return ret;
}

BOOL
STDCALL
NtGdiFlattenPath(HDC  hDC)
{
    BOOL Ret = FALSE;
    DC *pDc;

    DPRINT("Enter %s\n", __FUNCTION__);

    pDc = DC_LockDc(hDC);
    if(!pDc) return FALSE;

    if(pDc->w.path.state == PATH_Open)
	    Ret = PATH_FlattenPath(&pDc->w.path);

    DC_UnlockDc(pDc);
    return Ret;
}


BOOL
APIENTRY
NtGdiGetMiterLimit(
    IN HDC hdc,
    OUT PDWORD pdwOut)
{
  UNIMPLEMENTED;
  return FALSE;
}

INT
STDCALL
NtGdiGetPath(
   HDC hDC,
   LPPOINT Points,
   LPBYTE Types,
   INT nSize)
{
   INT ret = -1;
   GdiPath *pPath;

   DC *dc = DC_LockDc(hDC);
   if(!dc)
   {
      DPRINT1("Can't lock dc!\n");
      return -1;
   }

   pPath = &dc->w.path;

   if(pPath->state != PATH_Closed)
   {
      SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
      goto done;
   }

   if(nSize==0)
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
      _SEH_TRY
      {
         memcpy(Points, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
         memcpy(Types, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

         /* Convert the points to logical coordinates */
         IntDPtoLP(dc, Points, pPath->numEntriesUsed);

         ret = pPath->numEntriesUsed;
      }
      _SEH_HANDLE
      {
         SetLastNtError(_SEH_GetExceptionCode());
      }
      _SEH_END
   }

done:
   DC_UnlockDc(dc);
   return ret;
}

HRGN
STDCALL
NtGdiPathToRegion(HDC  hDC)
{
   GdiPath *pPath;
   HRGN  hrgnRval = 0;
   DC *pDc;

   DPRINT("Enter %s\n", __FUNCTION__);

   pDc = DC_LockDc(hDC);
   if(!pDc) return NULL;

   pPath = &pDc->w.path;

   if(pPath->state!=PATH_Closed)
   {
      //FIXME: check that setlasterror is being called correctly
      SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
   }
   else
   {
      /* FIXME: Should we empty the path even if conversion failed? */
      if(PATH_PathToRegion(pPath, pDc->Dc_Attr.jFillMode, &hrgnRval))
           PATH_EmptyPath(pPath);
   }

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
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiStrokeAndFillPath(HDC hDC)
{
   DC *pDc;
   BOOL bRet = FALSE;

   DPRINT("Enter %s\n", __FUNCTION__);

   if(!(pDc = DC_LockDc(hDC))) return FALSE;

   bRet = PATH_FillPath(pDc, &pDc->w.path);
   if(bRet) bRet = PATH_StrokePath(pDc, &pDc->w.path);
   if(bRet) PATH_EmptyPath(&pDc->w.path);

   DC_UnlockDc(pDc);
   return bRet;
}

BOOL
STDCALL
NtGdiStrokePath(HDC hDC)
{
    DC *pDc;
    BOOL bRet = FALSE;

    DPRINT("Enter %s\n", __FUNCTION__);

    if(!(pDc = DC_LockDc(hDC))) return FALSE;

    bRet = PATH_StrokePath(pDc, &pDc->w.path);
    PATH_EmptyPath(&pDc->w.path);

    DC_UnlockDc(pDc);
    return bRet;
}

BOOL
STDCALL
NtGdiWidenPath(HDC  hDC)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL STDCALL NtGdiSelectClipPath(HDC  hDC,
                         int  Mode)
{
 HRGN  hrgnPath;
 BOOL  success = FALSE;
 PDC dc = DC_LockDc ( hDC );

 if( !dc ) return FALSE;

 /* Check that path is closed */
 if( dc->w.path.state != PATH_Closed )
 {
   SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
   return FALSE;
 }
 /* Construct a region from the path */
 else if( PATH_PathToRegion( &dc->w.path, dc->Dc_Attr.jFillMode, &hrgnPath ) )
 {
   success = IntGdiExtSelectClipRgn( dc, hrgnPath, Mode ) != ERROR;
   NtGdiDeleteObject( hrgnPath );

   /* Empty the path */
   if( success )
     PATH_EmptyPath( &dc->w.path);
   /* FIXME: Should this function delete the path even if it failed? */
 }

 DC_UnlockDc ( dc );
 return success;
}

/***********************************************************************
 * Exported functions
 */


/* PATH_FillPath
 *
 *
 */
BOOL
FASTCALL
PATH_FillPath( PDC dc, GdiPath *pPath )
{
  INT   mapMode, graphicsMode;
  SIZE  ptViewportExt, ptWindowExt;
  POINT ptViewportOrg, ptWindowOrg;
  XFORM xform;
  HRGN  hrgn;

  if( pPath->state != PATH_Closed )
  {
    SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
    return FALSE;
  }

  if( PATH_PathToRegion( pPath, dc->Dc_Attr.jFillMode, &hrgn ))
  {
    /* Since PaintRgn interprets the region as being in logical coordinates
     * but the points we store for the path are already in device
     * coordinates, we have to set the mapping mode to MM_TEXT temporarily.
     * Using SaveDC to save information about the mapping mode / world
     * transform would be easier but would require more overhead, especially
     * now that SaveDC saves the current path.
     */

    /* Save the information about the old mapping mode */
    mapMode = dc->Dc_Attr.iMapMode;
    ptViewportExt = dc->Dc_Attr.szlViewportExt;
    ptViewportOrg = dc->Dc_Attr.ptlViewportOrg;
    ptWindowExt   = dc->Dc_Attr.szlWindowExt;
    ptWindowOrg   = dc->Dc_Attr.ptlWindowOrg;

    /* Save world transform
     * NB: The Windows documentation on world transforms would lead one to
     * believe that this has to be done only in GM_ADVANCED; however, my
     * tests show that resetting the graphics mode to GM_COMPATIBLE does
     * not reset the world transform.
     */
    xform = dc->w.xformWorld2Wnd;

    /* Set MM_TEXT */
    IntGdiSetMapMode( dc, MM_TEXT );
    dc->Dc_Attr.ptlViewportOrg.x = 0;
    dc->Dc_Attr.ptlViewportOrg.y = 0;
    dc->Dc_Attr.ptlWindowOrg.x = 0;
    dc->Dc_Attr.ptlWindowOrg.y = 0;

    graphicsMode = dc->Dc_Attr.iGraphicsMode;
    dc->Dc_Attr.iGraphicsMode = GM_ADVANCED;
    IntGdiModifyWorldTransform( dc, &xform, MWT_IDENTITY );
    dc->Dc_Attr.iGraphicsMode =  graphicsMode;

    /* Paint the region */
    IntGdiPaintRgn( dc, hrgn );
    NtGdiDeleteObject( hrgn );
    /* Restore the old mapping mode */
    IntGdiSetMapMode( dc, mapMode );
    dc->Dc_Attr.szlViewportExt = ptViewportExt;
    dc->Dc_Attr.ptlViewportOrg = ptViewportOrg;
    dc->Dc_Attr.szlWindowExt   = ptWindowExt;
    dc->Dc_Attr.ptlWindowOrg   = ptWindowOrg;

    /* Go to GM_ADVANCED temporarily to restore the world transform */
    graphicsMode = dc->Dc_Attr.iGraphicsMode;
    dc->Dc_Attr.iGraphicsMode = GM_ADVANCED;
    IntGdiModifyWorldTransform( dc, &xform, MWT_MAX+1 );
    dc->Dc_Attr.iGraphicsMode = graphicsMode;
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
PATH_InitGdiPath ( GdiPath *pPath )
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
PATH_DestroyGdiPath ( GdiPath *pPath )
{
  ASSERT(pPath!=NULL);

  ExFreePool(pPath->pPoints);
  ExFreePool(pPath->pFlags);
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
PATH_AssignGdiPath ( GdiPath *pPathDest, const GdiPath *pPathSrc )
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

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    /* FIXME: Do we have to call SetLastError? */
    return FALSE;

  /* Start a new stroke */
  dc->w.path.newStroke = TRUE;

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
  POINT point, pointCurPos;

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  /* Convert point to device coordinates */
  point.x=x;
  point.y=y;
  CoordLPtoDP ( dc, &point );

  /* Add a PT_MOVETO if necessary */
  if ( dc->w.path.newStroke )
  {
    dc->w.path.newStroke = FALSE;
    IntGetCurrentPositionEx ( dc, &pointCurPos );
    CoordLPtoDP ( dc, &pointCurPos );
    if ( !PATH_AddEntry(&dc->w.path, &pointCurPos, PT_MOVETO) )
      return FALSE;
  }

  /* Add a PT_LINETO entry */
  return PATH_AddEntry(&dc->w.path, &point, PT_LINETO);
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
  POINT corners[2], pointTemp;
  INT   temp;

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

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
  IntGdiCloseFigure(dc);

  /* Add four points to the path */
  pointTemp.x=corners[1].x;
  pointTemp.y=corners[0].y;
  if ( !PATH_AddEntry(&dc->w.path, &pointTemp, PT_MOVETO) )
    return FALSE;
  if ( !PATH_AddEntry(&dc->w.path, corners, PT_LINETO) )
    return FALSE;
  pointTemp.x=corners[0].x;
  pointTemp.y=corners[1].y;
  if ( !PATH_AddEntry(&dc->w.path, &pointTemp, PT_LINETO) )
    return FALSE;
  if ( !PATH_AddEntry(&dc->w.path, corners+1, PT_LINETO) )
    return FALSE;

  /* Close the rectangle figure */
  IntGdiCloseFigure(dc) ;

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
FASTCALL BOOL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height)
{
   GdiPath *pPath = &dc->w.path;
   POINT corners[2], pointTemp;
   FLOAT_POINT ellCorners[2];

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   if(!PATH_CheckCorners(dc,corners,x1,y1,x2,y2))
      return FALSE;

   /* Add points to the roundrect path */
   ellCorners[0].x = corners[1].x-ell_width;
   ellCorners[0].y = corners[0].y;
   ellCorners[1].x = corners[1].x;
   ellCorners[1].y = corners[0].y+ell_height;
   if(!PATH_DoArcPart(pPath, ellCorners, 0, -M_PI_2, TRUE))
      return FALSE;
   pointTemp.x = corners[0].x+ell_width/2;
   pointTemp.y = corners[0].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
      return FALSE;
   ellCorners[0].x = corners[0].x;
   ellCorners[1].x = corners[0].x+ell_width;
   if(!PATH_DoArcPart(pPath, ellCorners, -M_PI_2, -M_PI, FALSE))
      return FALSE;
   pointTemp.x = corners[0].x;
   pointTemp.y = corners[1].y-ell_height/2;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
      return FALSE;
   ellCorners[0].y = corners[1].y-ell_height;
   ellCorners[1].y = corners[1].y;
   if(!PATH_DoArcPart(pPath, ellCorners, M_PI, M_PI_2, FALSE))
      return FALSE;
   pointTemp.x = corners[1].x-ell_width/2;
   pointTemp.y = corners[1].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
      return FALSE;
   ellCorners[0].x = corners[1].x-ell_width;
   ellCorners[1].x = corners[1].x;
   if(!PATH_DoArcPart(pPath, ellCorners, M_PI_2, 0, FALSE))
      return FALSE;

   IntGdiCloseFigure(dc);

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
  /* TODO: This should probably be revised to call PATH_AngleArc */
  /* (once it exists) */
  BOOL Ret = PATH_Arc ( dc, x1, y1, x2, y2, x1, (y1+y2)/2, x1, (y1+y2)/2, GdiTypeArc );
  if (Ret) IntGdiCloseFigure(dc);
  return Ret;
}

/* PATH_Arc
 *
 * Should be called when a call to Arc is performed on a DC that has
 * an open path. This adds up to five Bezier splines representing the arc
 * to the path. When 'lines' is 1, we add 1 extra line to get a chord,
 * and when 'lines' is 2, we add 2 extra lines to get a pie.
 * Returns TRUE if successful, else FALSE.
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
  POINT   centre;
  BOOL    start, end;
  INT     temp;
  BOOL    clockwise;

  /* FIXME: This function should check for all possible error returns */
  /* FIXME: Do we have to respect newStroke? */

  ASSERT ( dc );

  clockwise = ( dc->w.ArcDirection == AD_CLOCKWISE );

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  /* FIXME: Do we have to close the current figure? */

  /* Check for zero height / width */
  /* FIXME: Only in GM_COMPATIBLE? */
  if ( x1==x2 || y1==y2 )
    return TRUE;

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
    PATH_DoArcPart ( &dc->w.path, corners, angleStartQuadrant, angleEndQuadrant, start );
    start = FALSE;
  } while(!end);

  /* chord: close figure. pie: add line and close figure */
  if(lines==GdiTypeChord) // 1
   {
      IntGdiCloseFigure(dc);
   }
  else if(lines==GdiTypePie) // 2
   {
      centre.x = (corners[0].x+corners[1].x)/2;
      centre.y = (corners[0].y+corners[1].y)/2;
      if(!PATH_AddEntry(&dc->w.path, &centre, PT_LINETO | PT_CLOSEFIGURE))
         return FALSE;
   }

  return TRUE;
}

BOOL
FASTCALL
PATH_PolyBezierTo ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT pt;
  ULONG i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  /* Add a PT_MOVETO if necessary */
  if ( dc->w.path.newStroke )
  {
    dc->w.path.newStroke=FALSE;
    IntGetCurrentPositionEx ( dc, &pt );
    CoordLPtoDP ( dc, &pt );
    if ( !PATH_AddEntry(&dc->w.path, &pt, PT_MOVETO) )
        return FALSE;
  }

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(&dc->w.path, &pt, PT_BEZIERTO);
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyBezier ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

   /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  for ( i = 0; i < cbPoints; i++ )
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry ( &dc->w.path, &pt, (i == 0) ? PT_MOVETO : PT_BEZIERTO );
  }

  return TRUE;
}

BOOL
FASTCALL
PATH_Polyline ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  for ( i = 0; i < cbPoints; i++ )
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(&dc->w.path, &pt, (i == 0) ? PT_MOVETO : PT_LINETO);
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolylineTo ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  /* Add a PT_MOVETO if necessary */
  if ( dc->w.path.newStroke )
  {
    dc->w.path.newStroke = FALSE;
    IntGetCurrentPositionEx ( dc, &pt );
    CoordLPtoDP ( dc, &pt );
    if ( !PATH_AddEntry(&dc->w.path, &pt, PT_MOVETO) )
      return FALSE;
  }

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(&dc->w.path, &pt, PT_LINETO);
  }

  return TRUE;
}


BOOL
FASTCALL
PATH_Polygon ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(&dc->w.path, &pt, (i == 0) ? PT_MOVETO :
      ((i == cbPoints-1) ? PT_LINETO | PT_CLOSEFIGURE :
      PT_LINETO));
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyPolygon ( PDC dc, const POINT* pts, const INT* counts, UINT polygons )
{
  POINT   pt, startpt;
  ULONG   poly, point, i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( counts );
  ASSERT ( polygons );

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open );
    return FALSE;

  for(i = 0, poly = 0; poly < polygons; poly++)
  {
    for(point = 0; point < (ULONG) counts[poly]; point++, i++)
    {
      pt = pts[i];
      CoordLPtoDP ( dc, &pt );
      if(point == 0) startpt = pt;
        PATH_AddEntry(&dc->w.path, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
    }
    /* win98 adds an extra line to close the figure for some reason */
    PATH_AddEntry(&dc->w.path, &startpt, PT_LINETO | PT_CLOSEFIGURE);
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyPolyline ( PDC dc, const POINT* pts, const DWORD* counts, DWORD polylines )
{
  POINT   pt;
  ULONG   poly, point, i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( counts );
  ASSERT ( polylines );

  /* Check that path is open */
  if ( dc->w.path.state != PATH_Open )
    return FALSE;

  for(i = 0, poly = 0; poly < polylines; poly++)
  {
    for(point = 0; point < counts[poly]; point++, i++)
    {
      pt = pts[i];
      CoordLPtoDP ( dc, &pt );
      PATH_AddEntry(&dc->w.path, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
    }
  }
  return TRUE;
}

/***********************************************************************
 * Internal functions
 */

/* PATH_CheckCorners
 *
 * Helper function for PATH_RoundRect() and PATH_Rectangle()
 */
BOOL PATH_CheckCorners(DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2)
{
   INT temp;

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
   if(dc->Dc_Attr.iGraphicsMode==GM_COMPATIBLE)
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
PATH_AddFlatBezier ( GdiPath *pPath, POINT *pt, BOOL closed )
{
  POINT *pts;
  INT no, i;

  pts = GDI_Bezier( pt, 4, &no );
  if ( !pts ) return FALSE;

  for(i = 1; i < no; i++)
    PATH_AddEntry(pPath, &pts[i],  (i == no-1 && closed) ? PT_LINETO | PT_CLOSEFIGURE : PT_LINETO);

  ExFreePool(pts);
  return TRUE;
}

/* PATH_FlattenPath
 *
 * Replaces Beziers with line segments
 *
 */
BOOL
FASTCALL
PATH_FlattenPath(GdiPath *pPath)
{
  GdiPath newPath;
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
PATH_PathToRegion ( GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn )
{
  int    numStrokes, iStroke, i;
  INT  *pNumPointsInStroke;
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
  pNumPointsInStroke=(int *)ExAllocatePoolWithTag(PagedPool, sizeof(int) * numStrokes, TAG_PATH);
  if(!pNumPointsInStroke)
  {
//    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
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
/*  hrgn=CreatePolyPolygonRgn(pPath->pPoints, pNumPointsInStroke,
    numStrokes, nPolyFillMode); FIXME: reinclude when region code implemented */
  if(hrgn==(HRGN)0)
  {
//    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  /* Free memory for number-of-points-in-stroke array */
  ExFreePool(pNumPointsInStroke);

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
PATH_EmptyPath ( GdiPath *pPath )
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
PATH_AddEntry ( GdiPath *pPath, const POINT *pPoint, BYTE flags )
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
PATH_ReserveEntries ( GdiPath *pPath, INT numEntries )
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
      ExFreePool(pPointsNew);
      return FALSE;
    }

    /* Copy old arrays to new arrays and discard old arrays */
    if(pPath->pPoints)
    {
      ASSERT(pPath->pFlags);

      memcpy(pPointsNew, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
      memcpy(pFlagsNew, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

      ExFreePool(pPath->pPoints);
      ExFreePool(pPath->pFlags);
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
 * at most. If "addMoveTo" is true, a PT_MOVETO entry for the first control
 * point is added to the path; otherwise, it is assumed that the current
 * position is equal to the first control point.
 */
BOOL
FASTCALL
PATH_DoArcPart ( GdiPath *pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BOOL addMoveTo )
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
  if(addMoveTo)
  {
    PATH_ScaleNormalizedPoint(corners, xNorm[0], yNorm[0], &point);
    if(!PATH_AddEntry(pPath, &point, PT_MOVETO))
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


BOOL FASTCALL PATH_StrokePath(DC *dc, GdiPath *pPath)
{
    BOOL ret = FALSE;
    INT i=0;
    INT nLinePts, nAlloc;
    POINT *pLinePts = NULL;
    POINT ptViewportOrg, ptWindowOrg;
    SIZE szViewportExt, szWindowExt;
    DWORD mapMode, graphicsMode;
    XFORM xform;

    DPRINT("Enter %s\n", __FUNCTION__);

    if(pPath->state != PATH_Closed)
        return FALSE;

    /* Save the mapping mode info */
    mapMode=dc->Dc_Attr.iMapMode;
    IntGetViewportExtEx(dc, &szViewportExt);
    IntGetViewportOrgEx(dc, &ptViewportOrg);
    IntGetWindowExtEx(dc, &szWindowExt);
    IntGetWindowOrgEx(dc, &ptWindowOrg);
    xform = dc->w.xformWorld2Wnd;

    /* Set MM_TEXT */
    dc->Dc_Attr.iMapMode = MM_TEXT;
    dc->Dc_Attr.ptlViewportOrg.x = 0;
    dc->Dc_Attr.ptlViewportOrg.y = 0;
    dc->Dc_Attr.ptlWindowOrg.x = 0;
    dc->Dc_Attr.ptlWindowOrg.y = 0;
    graphicsMode = dc->Dc_Attr.iGraphicsMode;
    dc->Dc_Attr.iGraphicsMode = GM_ADVANCED;
    IntGdiModifyWorldTransform(dc, &xform, MWT_IDENTITY);
    dc->Dc_Attr.iGraphicsMode = graphicsMode;

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
                    ExFreePool(pLinePts);
                    pLinePts = Realloc;
                }
                memcpy(&pLinePts[nLinePts], &pBzrPts[1], (nBzrPts - 1) * sizeof(POINT));
                nLinePts += nBzrPts - 1;
                ExFreePool(pBzrPts);
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
    }//for

    if(nLinePts >= 2)
        IntGdiPolyline(dc, pLinePts, nLinePts);

    ret = TRUE;

end:
    if(pLinePts)ExFreePool(pLinePts);

    /* Restore the old mapping mode */
    dc->Dc_Attr.iMapMode =  mapMode;
    dc->Dc_Attr.szlWindowExt.cx = szWindowExt.cx;
    dc->Dc_Attr.szlWindowExt.cy = szWindowExt.cy;
    dc->Dc_Attr.ptlWindowOrg.x = ptWindowOrg.x;
    dc->Dc_Attr.ptlWindowOrg.y = ptWindowOrg.y;

    dc->Dc_Attr.szlViewportExt.cx = szViewportExt.cx;
    dc->Dc_Attr.szlViewportExt.cy = szViewportExt.cy;
    dc->Dc_Attr.ptlViewportOrg.x = ptViewportOrg.x;
    dc->Dc_Attr.ptlViewportOrg.y = ptViewportOrg.y;

    /* Restore the world transform */
    dc->w.xformWorld2Wnd = xform;

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

/* EOF */
