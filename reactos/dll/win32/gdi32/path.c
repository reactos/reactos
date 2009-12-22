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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#if defined(HAVE_FLOAT_H)
#include <float.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"

#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

/* Notes on the implementation
 *
 * The implementation is based on dynamically resizable arrays of points and
 * flags. I dithered for a bit before deciding on this implementation, and
 * I had even done a bit of work on a linked list version before switching
 * to arrays. It's a bit of a tradeoff. When you use linked lists, the
 * implementation of FlattenPath is easier, because you can rip the
 * PT_BEZIERTO entries out of the middle of the list and link the
 * corresponding PT_LINETO entries in. However, when you use arrays,
 * PathToRegion becomes easier, since you can essentially just pass your array
 * of points to CreatePolyPolygonRgn. Also, if I'd used linked lists, I would
 * have had the extra effort of creating a chunk-based allocation scheme
 * in order to use memory effectively. That's why I finally decided to use
 * arrays. Note by the way that the array based implementation has the same
 * linear time complexity that linked lists would have since the arrays grow
 * exponentially.
 *
 * The points are stored in the path in device coordinates. This is
 * consistent with the way Windows does things (for instance, see the Win32
 * SDK documentation for GetPath).
 *
 * The word "stroke" appears in several places (e.g. in the flag
 * GdiPath.newStroke). A stroke consists of a PT_MOVETO followed by one or
 * more PT_LINETOs or PT_BEZIERTOs, up to, but not including, the next
 * PT_MOVETO. Note that this is not the same as the definition of a figure;
 * a figure can contain several strokes.
 *
 * I modified the drawing functions (MoveTo, LineTo etc.) to test whether
 * the path is open and to call the corresponding function in path.c if this
 * is the case. A more elegant approach would be to modify the function
 * pointers in the DC_FUNCTIONS structure; however, this would be a lot more
 * complex. Also, the performance degradation caused by my approach in the
 * case where no path is open is so small that it cannot be measured.
 *
 * Martin Boehme
 */

/* FIXME: A lot of stuff isn't implemented yet. There is much more to come. */

#define NUM_ENTRIES_INITIAL 16  /* Initial size of points / flags arrays  */
#define GROW_FACTOR_NUMER    2  /* Numerator of grow factor for the array */
#define GROW_FACTOR_DENOM    1  /* Denominator of grow factor             */

/* A floating point version of the POINT structure */
typedef struct tagFLOAT_POINT
{
   double x, y;
} FLOAT_POINT;


static BOOL PATH_AddEntry(GdiPath *pPath, const POINT *pPoint, BYTE flags);
static BOOL PATH_PathToRegion(GdiPath *pPath, INT nPolyFillMode,
   HRGN *pHrgn);
static void   PATH_EmptyPath(GdiPath *pPath);
static BOOL PATH_ReserveEntries(GdiPath *pPath, INT numEntries);
static BOOL PATH_DoArcPart(GdiPath *pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BYTE startEntryType);
static void PATH_ScaleNormalizedPoint(FLOAT_POINT corners[], double x,
   double y, POINT *pPoint);
static void PATH_NormalizePoint(FLOAT_POINT corners[], const FLOAT_POINT
   *pPoint, double *pX, double *pY);
static BOOL PATH_CheckCorners(DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2);

/* Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static inline void INTERNAL_LPTODP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    double x, y;

    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * dc->xformWorld2Vport.eM11 +
               y * dc->xformWorld2Vport.eM21 +
               dc->xformWorld2Vport.eDx;
    point->y = x * dc->xformWorld2Vport.eM12 +
               y * dc->xformWorld2Vport.eM22 +
               dc->xformWorld2Vport.eDy;
}


/***********************************************************************
 *           BeginPath    (GDI32.@)
 */
BOOL WINAPI BeginPath(HDC hdc)
{
    BOOL ret = TRUE;
    DC *dc = get_dc_ptr( hdc );

    if(!dc) return FALSE;

    if(dc->funcs->pBeginPath)
        ret = dc->funcs->pBeginPath(dc->physDev);
    else
    {
        /* If path is already open, do nothing */
        if(dc->path.state != PATH_Open)
        {
            /* Make sure that path is empty */
            PATH_EmptyPath(&dc->path);

            /* Initialize variables for new path */
            dc->path.newStroke=TRUE;
            dc->path.state=PATH_Open;
        }
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           EndPath    (GDI32.@)
 */
BOOL WINAPI EndPath(HDC hdc)
{
    BOOL ret = TRUE;
    DC *dc = get_dc_ptr( hdc );

    if(!dc) return FALSE;

    if(dc->funcs->pEndPath)
        ret = dc->funcs->pEndPath(dc->physDev);
    else
    {
        /* Check that path is currently being constructed */
        if(dc->path.state!=PATH_Open)
        {
            SetLastError(ERROR_CAN_NOT_COMPLETE);
            ret = FALSE;
        }
        /* Set flag to indicate that path is finished */
        else dc->path.state=PATH_Closed;
    }
    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * AbortPath [GDI32.@]
 * Closes and discards paths from device context
 *
 * NOTES
 *    Check that SetLastError is being called correctly
 *
 * PARAMS
 *    hdc [I] Handle to device context
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI AbortPath( HDC hdc )
{
    BOOL ret = TRUE;
    DC *dc = get_dc_ptr( hdc );

    if(!dc) return FALSE;

    if(dc->funcs->pAbortPath)
        ret = dc->funcs->pAbortPath(dc->physDev);
    else /* Remove all entries from the path */
        PATH_EmptyPath( &dc->path );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           CloseFigure    (GDI32.@)
 *
 * FIXME: Check that SetLastError is being called correctly
 */
BOOL WINAPI CloseFigure(HDC hdc)
{
    BOOL ret = TRUE;
    DC *dc = get_dc_ptr( hdc );

    if(!dc) return FALSE;

    if(dc->funcs->pCloseFigure)
        ret = dc->funcs->pCloseFigure(dc->physDev);
    else
    {
        /* Check that path is open */
        if(dc->path.state!=PATH_Open)
        {
            SetLastError(ERROR_CAN_NOT_COMPLETE);
            ret = FALSE;
        }
        else
        {
            /* Set PT_CLOSEFIGURE on the last entry and start a new stroke */
            /* It is not necessary to draw a line, PT_CLOSEFIGURE is a virtual closing line itself */
            if(dc->path.numEntriesUsed)
            {
                dc->path.pFlags[dc->path.numEntriesUsed-1]|=PT_CLOSEFIGURE;
                dc->path.newStroke=TRUE;
            }
        }
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetPath    (GDI32.@)
 */
INT WINAPI GetPath(HDC hdc, LPPOINT pPoints, LPBYTE pTypes,
   INT nSize)
{
   INT ret = -1;
   GdiPath *pPath;
   DC *dc = get_dc_ptr( hdc );

   if(!dc) return -1;

   pPath = &dc->path;

   /* Check that path is closed */
   if(pPath->state!=PATH_Closed)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      goto done;
   }

   if(nSize==0)
      ret = pPath->numEntriesUsed;
   else if(nSize<pPath->numEntriesUsed)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      goto done;
   }
   else
   {
      memcpy(pPoints, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
      memcpy(pTypes, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

      /* Convert the points to logical coordinates */
      if(!DPtoLP(hdc, pPoints, pPath->numEntriesUsed))
      {
	 /* FIXME: Is this the correct value? */
         SetLastError(ERROR_CAN_NOT_COMPLETE);
	goto done;
      }
     else ret = pPath->numEntriesUsed;
   }
 done:
   release_dc_ptr( dc );
   return ret;
}


/***********************************************************************
 *           PathToRegion    (GDI32.@)
 *
 * FIXME
 *   Check that SetLastError is being called correctly
 *
 * The documentation does not state this explicitly, but a test under Windows
 * shows that the region which is returned should be in device coordinates.
 */
HRGN WINAPI PathToRegion(HDC hdc)
{
   GdiPath *pPath;
   HRGN  hrgnRval = 0;
   DC *dc = get_dc_ptr( hdc );

   /* Get pointer to path */
   if(!dc) return 0;

    pPath = &dc->path;

   /* Check that path is closed */
   if(pPath->state!=PATH_Closed) SetLastError(ERROR_CAN_NOT_COMPLETE);
   else
   {
       /* FIXME: Should we empty the path even if conversion failed? */
       if(PATH_PathToRegion(pPath, GetPolyFillMode(hdc), &hrgnRval))
           PATH_EmptyPath(pPath);
       else
           hrgnRval=0;
   }
   release_dc_ptr( dc );
   return hrgnRval;
}

static BOOL PATH_FillPath(DC *dc, GdiPath *pPath)
{
   INT   mapMode, graphicsMode;
   SIZE  ptViewportExt, ptWindowExt;
   POINT ptViewportOrg, ptWindowOrg;
   XFORM xform;
   HRGN  hrgn;

   if(dc->funcs->pFillPath)
       return dc->funcs->pFillPath(dc->physDev);

   /* Check that path is closed */
   if(pPath->state!=PATH_Closed)
   {
      SetLastError(ERROR_CAN_NOT_COMPLETE);
      return FALSE;
   }

   /* Construct a region from the path and fill it */
   if(PATH_PathToRegion(pPath, dc->polyFillMode, &hrgn))
   {
      /* Since PaintRgn interprets the region as being in logical coordinates
       * but the points we store for the path are already in device
       * coordinates, we have to set the mapping mode to MM_TEXT temporarily.
       * Using SaveDC to save information about the mapping mode / world
       * transform would be easier but would require more overhead, especially
       * now that SaveDC saves the current path.
       */

      /* Save the information about the old mapping mode */
      mapMode=GetMapMode(dc->hSelf);
      GetViewportExtEx(dc->hSelf, &ptViewportExt);
      GetViewportOrgEx(dc->hSelf, &ptViewportOrg);
      GetWindowExtEx(dc->hSelf, &ptWindowExt);
      GetWindowOrgEx(dc->hSelf, &ptWindowOrg);

      /* Save world transform
       * NB: The Windows documentation on world transforms would lead one to
       * believe that this has to be done only in GM_ADVANCED; however, my
       * tests show that resetting the graphics mode to GM_COMPATIBLE does
       * not reset the world transform.
       */
      GetWorldTransform(dc->hSelf, &xform);

      /* Set MM_TEXT */
      SetMapMode(dc->hSelf, MM_TEXT);
      SetViewportOrgEx(dc->hSelf, 0, 0, NULL);
      SetWindowOrgEx(dc->hSelf, 0, 0, NULL);
      graphicsMode=GetGraphicsMode(dc->hSelf);
      SetGraphicsMode(dc->hSelf, GM_ADVANCED);
      ModifyWorldTransform(dc->hSelf, &xform, MWT_IDENTITY);
      SetGraphicsMode(dc->hSelf, graphicsMode);

      /* Paint the region */
      PaintRgn(dc->hSelf, hrgn);
      DeleteObject(hrgn);
      /* Restore the old mapping mode */
      SetMapMode(dc->hSelf, mapMode);
      SetViewportExtEx(dc->hSelf, ptViewportExt.cx, ptViewportExt.cy, NULL);
      SetViewportOrgEx(dc->hSelf, ptViewportOrg.x, ptViewportOrg.y, NULL);
      SetWindowExtEx(dc->hSelf, ptWindowExt.cx, ptWindowExt.cy, NULL);
      SetWindowOrgEx(dc->hSelf, ptWindowOrg.x, ptWindowOrg.y, NULL);

      /* Go to GM_ADVANCED temporarily to restore the world transform */
      graphicsMode=GetGraphicsMode(dc->hSelf);
      SetGraphicsMode(dc->hSelf, GM_ADVANCED);
      SetWorldTransform(dc->hSelf, &xform);
      SetGraphicsMode(dc->hSelf, graphicsMode);
      return TRUE;
   }
   return FALSE;
}


/***********************************************************************
 *           FillPath    (GDI32.@)
 *
 * FIXME
 *    Check that SetLastError is being called correctly
 */
BOOL WINAPI FillPath(HDC hdc)
{
    DC *dc = get_dc_ptr( hdc );
    BOOL bRet = FALSE;

    if(!dc) return FALSE;

    if(dc->funcs->pFillPath)
        bRet = dc->funcs->pFillPath(dc->physDev);
    else
    {
        bRet = PATH_FillPath(dc, &dc->path);
        if(bRet)
        {
            /* FIXME: Should the path be emptied even if conversion
               failed? */
            PATH_EmptyPath(&dc->path);
        }
    }
    release_dc_ptr( dc );
    return bRet;
}


/***********************************************************************
 *           SelectClipPath    (GDI32.@)
 * FIXME
 *  Check that SetLastError is being called correctly
 */
BOOL WINAPI SelectClipPath(HDC hdc, INT iMode)
{
   GdiPath *pPath;
   HRGN  hrgnPath;
   BOOL  success = FALSE;
   DC *dc = get_dc_ptr( hdc );

   if(!dc) return FALSE;

   if(dc->funcs->pSelectClipPath)
     success = dc->funcs->pSelectClipPath(dc->physDev, iMode);
   else
   {
       pPath = &dc->path;

       /* Check that path is closed */
       if(pPath->state!=PATH_Closed)
           SetLastError(ERROR_CAN_NOT_COMPLETE);
       /* Construct a region from the path */
       else if(PATH_PathToRegion(pPath, GetPolyFillMode(hdc), &hrgnPath))
       {
           success = ExtSelectClipRgn( hdc, hrgnPath, iMode ) != ERROR;
           DeleteObject(hrgnPath);

           /* Empty the path */
           if(success)
               PATH_EmptyPath(pPath);
           /* FIXME: Should this function delete the path even if it failed? */
       }
   }
   release_dc_ptr( dc );
   return success;
}


/***********************************************************************
 * Exported functions
 */

/* PATH_InitGdiPath
 *
 * Initializes the GdiPath structure.
 */
void PATH_InitGdiPath(GdiPath *pPath)
{
   assert(pPath!=NULL);

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
void PATH_DestroyGdiPath(GdiPath *pPath)
{
   assert(pPath!=NULL);

   HeapFree( GetProcessHeap(), 0, pPath->pPoints );
   HeapFree( GetProcessHeap(), 0, pPath->pFlags );
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
BOOL PATH_AssignGdiPath(GdiPath *pPathDest, const GdiPath *pPathSrc)
{
   assert(pPathDest!=NULL && pPathSrc!=NULL);

   /* Make sure destination arrays are big enough */
   if(!PATH_ReserveEntries(pPathDest, pPathSrc->numEntriesUsed))
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
BOOL PATH_MoveTo(DC *dc)
{
   GdiPath *pPath = &dc->path;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      /* FIXME: Do we have to call SetLastError? */
      return FALSE;

   /* Start a new stroke */
   pPath->newStroke=TRUE;

   return TRUE;
}

/* PATH_LineTo
 *
 * Should be called when a LineTo is performed on a DC that has an
 * open path. This adds a PT_LINETO entry to the path (and possibly
 * a PT_MOVETO entry, if this is the first LineTo in a stroke).
 * Returns TRUE if successful, else FALSE.
 */
BOOL PATH_LineTo(DC *dc, INT x, INT y)
{
   GdiPath *pPath = &dc->path;
   POINT point, pointCurPos;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* Convert point to device coordinates */
   point.x=x;
   point.y=y;
   if(!LPtoDP(dc->hSelf, &point, 1))
      return FALSE;

   /* Add a PT_MOVETO if necessary */
   if(pPath->newStroke)
   {
      pPath->newStroke=FALSE;
      pointCurPos.x = dc->CursPosX;
      pointCurPos.y = dc->CursPosY;
      if(!LPtoDP(dc->hSelf, &pointCurPos, 1))
         return FALSE;
      if(!PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO))
         return FALSE;
   }

   /* Add a PT_LINETO entry */
   return PATH_AddEntry(pPath, &point, PT_LINETO);
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
BOOL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height)
{
   GdiPath *pPath = &dc->path;
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
   if(!PATH_DoArcPart(pPath, ellCorners, 0, -M_PI_2, PT_MOVETO))
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

   /* Close the roundrect figure */
   if(!CloseFigure(dc->hSelf))
      return FALSE;

   return TRUE;
}

/* PATH_Rectangle
 *
 * Should be called when a call to Rectangle is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 */
BOOL PATH_Rectangle(DC *dc, INT x1, INT y1, INT x2, INT y2)
{
   GdiPath *pPath = &dc->path;
   POINT corners[2], pointTemp;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   if(!PATH_CheckCorners(dc,corners,x1,y1,x2,y2))
      return FALSE;

   /* Close any previous figure */
   if(!CloseFigure(dc->hSelf))
   {
      /* The CloseFigure call shouldn't have failed */
      assert(FALSE);
      return FALSE;
   }

   /* Add four points to the path */
   pointTemp.x=corners[1].x;
   pointTemp.y=corners[0].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_MOVETO))
      return FALSE;
   if(!PATH_AddEntry(pPath, corners, PT_LINETO))
      return FALSE;
   pointTemp.x=corners[0].x;
   pointTemp.y=corners[1].y;
   if(!PATH_AddEntry(pPath, &pointTemp, PT_LINETO))
      return FALSE;
   if(!PATH_AddEntry(pPath, corners+1, PT_LINETO))
      return FALSE;

   /* Close the rectangle figure */
   if(!CloseFigure(dc->hSelf))
   {
      /* The CloseFigure call shouldn't have failed */
      assert(FALSE);
      return FALSE;
   }

   return TRUE;
}

/* PATH_Ellipse
 *
 * Should be called when a call to Ellipse is performed on a DC that has
 * an open path. This adds four Bezier splines representing the ellipse
 * to the path. Returns TRUE if successful, else FALSE.
 */
BOOL PATH_Ellipse(DC *dc, INT x1, INT y1, INT x2, INT y2)
{
   return( PATH_Arc(dc, x1, y1, x2, y2, x1, (y1+y2)/2, x1, (y1+y2)/2,0) &&
           CloseFigure(dc->hSelf) );
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
BOOL PATH_Arc(DC *dc, INT x1, INT y1, INT x2, INT y2,
   INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines)
{
   GdiPath     *pPath = &dc->path;
   double      angleStart, angleEnd, angleStartQuadrant, angleEndQuadrant=0.0;
               /* Initialize angleEndQuadrant to silence gcc's warning */
   double      x, y;
   FLOAT_POINT corners[2], pointStart, pointEnd;
   POINT       centre, pointCurPos;
   BOOL      start, end;
   INT       temp;

   /* FIXME: This function should check for all possible error returns */
   /* FIXME: Do we have to respect newStroke? */

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* Check for zero height / width */
   /* FIXME: Only in GM_COMPATIBLE? */
   if(x1==x2 || y1==y2)
      return TRUE;

   /* Convert points to device coordinates */
   corners[0].x = x1;
   corners[0].y = y1;
   corners[1].x = x2;
   corners[1].y = y2;
   pointStart.x = xStart;
   pointStart.y = yStart;
   pointEnd.x = xEnd;
   pointEnd.y = yEnd;
   INTERNAL_LPTODP_FLOAT(dc, corners);
   INTERNAL_LPTODP_FLOAT(dc, corners+1);
   INTERNAL_LPTODP_FLOAT(dc, &pointStart);
   INTERNAL_LPTODP_FLOAT(dc, &pointEnd);

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

   /* Compute start and end angle */
   PATH_NormalizePoint(corners, &pointStart, &x, &y);
   angleStart=atan2(y, x);
   PATH_NormalizePoint(corners, &pointEnd, &x, &y);
   angleEnd=atan2(y, x);

   /* Make sure the end angle is "on the right side" of the start angle */
   if(dc->ArcDirection==AD_CLOCKWISE)
   {
      if(angleEnd<=angleStart)
      {
         angleEnd+=2*M_PI;
	 assert(angleEnd>=angleStart);
      }
   }
   else
   {
      if(angleEnd>=angleStart)
      {
         angleEnd-=2*M_PI;
	 assert(angleEnd<=angleStart);
      }
   }

   /* In GM_COMPATIBLE, don't include bottom and right edges */
   if(dc->GraphicsMode==GM_COMPATIBLE)
   {
      corners[1].x--;
      corners[1].y--;
   }

   /* arcto: Add a PT_MOVETO only if this is the first entry in a stroke */
   if(lines==-1 && pPath->newStroke)
   {
      pPath->newStroke=FALSE;
      pointCurPos.x = dc->CursPosX;
      pointCurPos.y = dc->CursPosY;
      if(!LPtoDP(dc->hSelf, &pointCurPos, 1))
         return FALSE;
      if(!PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO))
         return FALSE;
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
	 if(dc->ArcDirection==AD_CLOCKWISE)
	    angleEndQuadrant=(floor(angleStart/M_PI_2)+1.0)*M_PI_2;
	 else
	    angleEndQuadrant=(ceil(angleStart/M_PI_2)-1.0)*M_PI_2;
      }
      else
      {
	 angleStartQuadrant=angleEndQuadrant;
	 if(dc->ArcDirection==AD_CLOCKWISE)
	    angleEndQuadrant+=M_PI_2;
	 else
	    angleEndQuadrant-=M_PI_2;
      }

      /* Have we reached the last part of the arc? */
      if((dc->ArcDirection==AD_CLOCKWISE &&
         angleEnd<angleEndQuadrant) ||
	 (dc->ArcDirection==AD_COUNTERCLOCKWISE &&
	 angleEnd>angleEndQuadrant))
      {
	 /* Adjust the end angle for this quadrant */
         angleEndQuadrant=angleEnd;
	 end=TRUE;
      }

      /* Add the Bezier spline to the path */
      PATH_DoArcPart(pPath, corners, angleStartQuadrant, angleEndQuadrant,
         start ? (lines==-1 ? PT_LINETO : PT_MOVETO) : FALSE);
      start=FALSE;
   }  while(!end);

   /* chord: close figure. pie: add line and close figure */
   if(lines==1)
   {
      if(!CloseFigure(dc->hSelf))
         return FALSE;
   }
   else if(lines==2)
   {
      centre.x = (corners[0].x+corners[1].x)/2;
      centre.y = (corners[0].y+corners[1].y)/2;
      if(!PATH_AddEntry(pPath, &centre, PT_LINETO | PT_CLOSEFIGURE))
         return FALSE;
   }

   return TRUE;
}

BOOL PATH_PolyBezierTo(DC *dc, const POINT *pts, DWORD cbPoints)
{
   GdiPath     *pPath = &dc->path;
   POINT       pt;
   UINT        i;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* Add a PT_MOVETO if necessary */
   if(pPath->newStroke)
   {
      pPath->newStroke=FALSE;
      pt.x = dc->CursPosX;
      pt.y = dc->CursPosY;
      if(!LPtoDP(dc->hSelf, &pt, 1))
         return FALSE;
      if(!PATH_AddEntry(pPath, &pt, PT_MOVETO))
         return FALSE;
   }

   for(i = 0; i < cbPoints; i++) {
       pt = pts[i];
       if(!LPtoDP(dc->hSelf, &pt, 1))
	   return FALSE;
       PATH_AddEntry(pPath, &pt, PT_BEZIERTO);
   }
   return TRUE;
}

BOOL PATH_PolyBezier(DC *dc, const POINT *pts, DWORD cbPoints)
{
   GdiPath     *pPath = &dc->path;
   POINT       pt;
   UINT        i;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   for(i = 0; i < cbPoints; i++) {
       pt = pts[i];
       if(!LPtoDP(dc->hSelf, &pt, 1))
	   return FALSE;
       PATH_AddEntry(pPath, &pt, (i == 0) ? PT_MOVETO : PT_BEZIERTO);
   }
   return TRUE;
}

/* PATH_PolyDraw
 *
 * Should be called when a call to PolyDraw is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 */
BOOL PATH_PolyDraw(DC *dc, const POINT *pts, const BYTE *types,
    DWORD cbPoints)
{
        GdiPath     *pPath = &dc->path;
        POINT       lastmove, orig_pos;
        INT         i;

        lastmove.x = orig_pos.x = dc->CursPosX;
        lastmove.y = orig_pos.y = dc->CursPosY;

        for(i = pPath->numEntriesUsed - 1; i >= 0; i--){
            if(pPath->pFlags[i] == PT_MOVETO){
                lastmove.x = pPath->pPoints[i].x;
                lastmove.y = pPath->pPoints[i].y;
                if(!DPtoLP(dc->hSelf, &lastmove, 1))
                    return FALSE;
                break;
            }
        }

        for(i = 0; i < cbPoints; i++){
            if(types[i] == PT_MOVETO){
                pPath->newStroke = TRUE;
                lastmove.x = pts[i].x;
                lastmove.y = pts[i].y;
            }
            else if((types[i] & ~PT_CLOSEFIGURE) == PT_LINETO){
                PATH_LineTo(dc, pts[i].x, pts[i].y);
            }
            else if(types[i] == PT_BEZIERTO){
                if(!((i + 2 < cbPoints) && (types[i + 1] == PT_BEZIERTO)
                    && ((types[i + 2] & ~PT_CLOSEFIGURE) == PT_BEZIERTO)))
                    goto err;
                PATH_PolyBezierTo(dc, &(pts[i]), 3);
                i += 2;
            }
            else
                goto err;

            dc->CursPosX = pts[i].x;
            dc->CursPosY = pts[i].y;

            if(types[i] & PT_CLOSEFIGURE){
                pPath->pFlags[pPath->numEntriesUsed-1] |= PT_CLOSEFIGURE;
                pPath->newStroke = TRUE;
                dc->CursPosX = lastmove.x;
                dc->CursPosY = lastmove.y;
            }
        }

        return TRUE;

err:
        if((dc->CursPosX != orig_pos.x) || (dc->CursPosY != orig_pos.y)){
            pPath->newStroke = TRUE;
            dc->CursPosX = orig_pos.x;
            dc->CursPosY = orig_pos.y;
        }

        return FALSE;
}

BOOL PATH_Polyline(DC *dc, const POINT *pts, DWORD cbPoints)
{
   GdiPath     *pPath = &dc->path;
   POINT       pt;
   UINT        i;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   for(i = 0; i < cbPoints; i++) {
       pt = pts[i];
       if(!LPtoDP(dc->hSelf, &pt, 1))
	   return FALSE;
       PATH_AddEntry(pPath, &pt, (i == 0) ? PT_MOVETO : PT_LINETO);
   }
   return TRUE;
}

BOOL PATH_PolylineTo(DC *dc, const POINT *pts, DWORD cbPoints)
{
   GdiPath     *pPath = &dc->path;
   POINT       pt;
   UINT        i;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* Add a PT_MOVETO if necessary */
   if(pPath->newStroke)
   {
      pPath->newStroke=FALSE;
      pt.x = dc->CursPosX;
      pt.y = dc->CursPosY;
      if(!LPtoDP(dc->hSelf, &pt, 1))
         return FALSE;
      if(!PATH_AddEntry(pPath, &pt, PT_MOVETO))
         return FALSE;
   }

   for(i = 0; i < cbPoints; i++) {
       pt = pts[i];
       if(!LPtoDP(dc->hSelf, &pt, 1))
	   return FALSE;
       PATH_AddEntry(pPath, &pt, PT_LINETO);
   }

   return TRUE;
}


BOOL PATH_Polygon(DC *dc, const POINT *pts, DWORD cbPoints)
{
   GdiPath     *pPath = &dc->path;
   POINT       pt;
   UINT        i;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   for(i = 0; i < cbPoints; i++) {
       pt = pts[i];
       if(!LPtoDP(dc->hSelf, &pt, 1))
	   return FALSE;
       PATH_AddEntry(pPath, &pt, (i == 0) ? PT_MOVETO :
		     ((i == cbPoints-1) ? PT_LINETO | PT_CLOSEFIGURE :
		      PT_LINETO));
   }
   return TRUE;
}

BOOL PATH_PolyPolygon( DC *dc, const POINT* pts, const INT* counts,
		       UINT polygons )
{
   GdiPath     *pPath = &dc->path;
   POINT       pt, startpt;
   UINT        poly, i;
   INT         point;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   for(i = 0, poly = 0; poly < polygons; poly++) {
       for(point = 0; point < counts[poly]; point++, i++) {
	   pt = pts[i];
	   if(!LPtoDP(dc->hSelf, &pt, 1))
	       return FALSE;
	   if(point == 0) startpt = pt;
	   PATH_AddEntry(pPath, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
       }
       /* win98 adds an extra line to close the figure for some reason */
       PATH_AddEntry(pPath, &startpt, PT_LINETO | PT_CLOSEFIGURE);
   }
   return TRUE;
}

BOOL PATH_PolyPolyline( DC *dc, const POINT* pts, const DWORD* counts,
			DWORD polylines )
{
   GdiPath     *pPath = &dc->path;
   POINT       pt;
   UINT        poly, point, i;

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   for(i = 0, poly = 0; poly < polylines; poly++) {
       for(point = 0; point < counts[poly]; point++, i++) {
	   pt = pts[i];
	   if(!LPtoDP(dc->hSelf, &pt, 1))
	       return FALSE;
	   PATH_AddEntry(pPath, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
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
static BOOL PATH_CheckCorners(DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2)
{
   INT temp;

   /* Convert points to device coordinates */
   corners[0].x=x1;
   corners[0].y=y1;
   corners[1].x=x2;
   corners[1].y=y2;
   if(!LPtoDP(dc->hSelf, corners, 2))
      return FALSE;

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
   if(dc->GraphicsMode==GM_COMPATIBLE)
   {
      corners[1].x--;
      corners[1].y--;
   }

   return TRUE;
}

/* PATH_AddFlatBezier
 */
static BOOL PATH_AddFlatBezier(GdiPath *pPath, POINT *pt, BOOL closed)
{
    POINT *pts;
    INT no, i;

    pts = GDI_Bezier( pt, 4, &no );
    if(!pts) return FALSE;

    for(i = 1; i < no; i++)
        PATH_AddEntry(pPath, &pts[i],
	    (i == no-1 && closed) ? PT_LINETO | PT_CLOSEFIGURE : PT_LINETO);
    HeapFree( GetProcessHeap(), 0, pts );
    return TRUE;
}

/* PATH_FlattenPath
 *
 * Replaces Beziers with line segments
 *
 */
static BOOL PATH_FlattenPath(GdiPath *pPath)
{
    GdiPath newPath;
    INT srcpt;

    memset(&newPath, 0, sizeof(newPath));
    newPath.state = PATH_Open;
    for(srcpt = 0; srcpt < pPath->numEntriesUsed; srcpt++) {
        switch(pPath->pFlags[srcpt] & ~PT_CLOSEFIGURE) {
	case PT_MOVETO:
	case PT_LINETO:
	    PATH_AddEntry(&newPath, &pPath->pPoints[srcpt],
			  pPath->pFlags[srcpt]);
	    break;
	case PT_BEZIERTO:
	  PATH_AddFlatBezier(&newPath, &pPath->pPoints[srcpt-1],
			     pPath->pFlags[srcpt+2] & PT_CLOSEFIGURE);
	    srcpt += 2;
	    break;
	}
    }
    newPath.state = PATH_Closed;
    PATH_AssignGdiPath(pPath, &newPath);
    PATH_DestroyGdiPath(&newPath);
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
static BOOL PATH_PathToRegion(GdiPath *pPath, INT nPolyFillMode,
   HRGN *pHrgn)
{
   int    numStrokes, iStroke, i;
   INT  *pNumPointsInStroke;
   HRGN hrgn;

   assert(pPath!=NULL);
   assert(pHrgn!=NULL);

   PATH_FlattenPath(pPath);

   /* FIXME: What happens when number of points is zero? */

   /* First pass: Find out how many strokes there are in the path */
   /* FIXME: We could eliminate this with some bookkeeping in GdiPath */
   numStrokes=0;
   for(i=0; i<pPath->numEntriesUsed; i++)
      if((pPath->pFlags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
         numStrokes++;

   /* Allocate memory for number-of-points-in-stroke array */
   pNumPointsInStroke=HeapAlloc( GetProcessHeap(), 0, sizeof(int) * numStrokes );
   if(!pNumPointsInStroke)
   {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
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
   hrgn=CreatePolyPolygonRgn(pPath->pPoints, pNumPointsInStroke,
      numStrokes, nPolyFillMode);

   /* Free memory for number-of-points-in-stroke array */
   HeapFree( GetProcessHeap(), 0, pNumPointsInStroke );

   if(hrgn==NULL)
   {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   /* Success! */
   *pHrgn=hrgn;
   return TRUE;
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
static void PATH_BezierTo(GdiPath *pPath, POINT *lppt, INT n)
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

static BOOL PATH_add_outline(DC *dc, INT x, INT y, TTPOLYGONHEADER *header, DWORD size)
{
    GdiPath *pPath = &dc->path;
    TTPOLYGONHEADER *start;
    POINT pt;

    start = header;

    while ((char *)header < (char *)start + size)
    {
        TTPOLYCURVE *curve;

        if (header->dwType != TT_POLYGON_TYPE)
        {
            FIXME("Unknown header type %d\n", header->dwType);
            return FALSE;
        }

        pt.x = x + int_from_fixed(header->pfxStart.x);
        pt.y = y - int_from_fixed(header->pfxStart.y);
        PATH_AddEntry(pPath, &pt, PT_MOVETO);

        curve = (TTPOLYCURVE *)(header + 1);

        while ((char *)curve < (char *)header + header->cb)
        {
            /*TRACE("curve->wType %d\n", curve->wType);*/

            switch(curve->wType)
            {
            case TT_PRIM_LINE:
            {
                WORD i;

                for (i = 0; i < curve->cpfx; i++)
                {
                    pt.x = x + int_from_fixed(curve->apfx[i].x);
                    pt.y = y - int_from_fixed(curve->apfx[i].y);
                    PATH_AddEntry(pPath, &pt, PT_LINETO);
                }
                break;
            }

            case TT_PRIM_QSPLINE:
            case TT_PRIM_CSPLINE:
            {
                WORD i;
                POINTFX ptfx;
                POINT *pts = HeapAlloc(GetProcessHeap(), 0, (curve->cpfx + 1) * sizeof(POINT));

                if (!pts) return FALSE;

                ptfx = *(POINTFX *)((char *)curve - sizeof(POINTFX));

                pts[0].x = x + int_from_fixed(ptfx.x);
                pts[0].y = y - int_from_fixed(ptfx.y);

                for(i = 0; i < curve->cpfx; i++)
                {
                    pts[i + 1].x = x + int_from_fixed(curve->apfx[i].x);
                    pts[i + 1].y = y - int_from_fixed(curve->apfx[i].y);
                }

                PATH_BezierTo(pPath, pts, curve->cpfx + 1);

                HeapFree(GetProcessHeap(), 0, pts);
                break;
            }

            default:
                FIXME("Unknown curve type %04x\n", curve->wType);
                return FALSE;
            }

            curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
        }

        header = (TTPOLYGONHEADER *)((char *)header + header->cb);
    }

    return CloseFigure(dc->hSelf);
}

/**********************************************************************
 *      PATH_ExtTextOut
 */
BOOL PATH_ExtTextOut(DC *dc, INT x, INT y, UINT flags, const RECT *lprc,
                     LPCWSTR str, UINT count, const INT *dx)
{
    unsigned int idx;
    double cosEsc, sinEsc;
    LOGFONTW lf;
    HDC hdc = dc->hSelf;
    INT offset = 0, xoff = 0, yoff = 0;

    TRACE("%p, %d, %d, %08x, %s, %s, %d, %p)\n", hdc, x, y, flags,
	  wine_dbgstr_rect(lprc), debugstr_wn(str, count), count, dx);

    if (!count) return TRUE;

    GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf);

    if (lf.lfEscapement != 0)
    {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    } else
    {
        cosEsc = 1;
        sinEsc = 0;
    }

    for (idx = 0; idx < count; idx++)
    {
        static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };
        GLYPHMETRICS gm;
        DWORD dwSize;
        void *outline;

        dwSize = GetGlyphOutlineW(hdc, str[idx], GGO_GLYPH_INDEX | GGO_NATIVE, &gm, 0, NULL, &identity);
        if (dwSize == GDI_ERROR) return FALSE;

        /* add outline only if char is printable */
        if(dwSize)
        {
            outline = HeapAlloc(GetProcessHeap(), 0, dwSize);
            if (!outline) return FALSE;

            GetGlyphOutlineW(hdc, str[idx], GGO_GLYPH_INDEX | GGO_NATIVE, &gm, dwSize, outline, &identity);

            PATH_add_outline(dc, x + xoff, y + yoff, outline, dwSize);

            HeapFree(GetProcessHeap(), 0, outline);
        }

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

/* PATH_EmptyPath
 *
 * Removes all entries from the path and sets the path state to PATH_Null.
 */
static void PATH_EmptyPath(GdiPath *pPath)
{
   assert(pPath!=NULL);

   pPath->state=PATH_Null;
   pPath->numEntriesUsed=0;
}

/* PATH_AddEntry
 *
 * Adds an entry to the path. For "flags", pass either PT_MOVETO, PT_LINETO
 * or PT_BEZIERTO, optionally ORed with PT_CLOSEFIGURE. Returns TRUE if
 * successful, FALSE otherwise (e.g. if not enough memory was available).
 */
static BOOL PATH_AddEntry(GdiPath *pPath, const POINT *pPoint, BYTE flags)
{
   assert(pPath!=NULL);

   /* FIXME: If newStroke is true, perhaps we want to check that we're
    * getting a PT_MOVETO
    */
   TRACE("(%d,%d) - %d\n", pPoint->x, pPoint->y, flags);

   /* Check that path is open */
   if(pPath->state!=PATH_Open)
      return FALSE;

   /* Reserve enough memory for an extra path entry */
   if(!PATH_ReserveEntries(pPath, pPath->numEntriesUsed+1))
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
static BOOL PATH_ReserveEntries(GdiPath *pPath, INT numEntries)
{
   INT   numEntriesToAllocate;
   POINT *pPointsNew;
   BYTE    *pFlagsNew;

   assert(pPath!=NULL);
   assert(numEntries>=0);

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
	    numEntriesToAllocate=numEntriesToAllocate*GROW_FACTOR_NUMER/
	       GROW_FACTOR_DENOM;
      }
      else
         numEntriesToAllocate=numEntries;

      /* Allocate new arrays */
      pPointsNew=HeapAlloc( GetProcessHeap(), 0, numEntriesToAllocate * sizeof(POINT) );
      if(!pPointsNew)
         return FALSE;
      pFlagsNew=HeapAlloc( GetProcessHeap(), 0, numEntriesToAllocate * sizeof(BYTE) );
      if(!pFlagsNew)
      {
         HeapFree( GetProcessHeap(), 0, pPointsNew );
	 return FALSE;
      }

      /* Copy old arrays to new arrays and discard old arrays */
      if(pPath->pPoints)
      {
         assert(pPath->pFlags);

	 memcpy(pPointsNew, pPath->pPoints,
	     sizeof(POINT)*pPath->numEntriesUsed);
	 memcpy(pFlagsNew, pPath->pFlags,
	     sizeof(BYTE)*pPath->numEntriesUsed);

	 HeapFree( GetProcessHeap(), 0, pPath->pPoints );
	 HeapFree( GetProcessHeap(), 0, pPath->pFlags );
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
static BOOL PATH_DoArcPart(GdiPath *pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BYTE startEntryType)
{
   double  halfAngle, a;
   double  xNorm[4], yNorm[4];
   POINT point;
   int     i;

   assert(fabs(angleEnd-angleStart)<=M_PI_2);

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
   }
   else
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
static void PATH_ScaleNormalizedPoint(FLOAT_POINT corners[], double x,
   double y, POINT *pPoint)
{
   pPoint->x=GDI_ROUND( (double)corners[0].x +
      (double)(corners[1].x-corners[0].x)*0.5*(x+1.0) );
   pPoint->y=GDI_ROUND( (double)corners[0].y +
      (double)(corners[1].y-corners[0].y)*0.5*(y+1.0) );
}

/* PATH_NormalizePoint
 *
 * Normalizes a point with respect to the box whose corners are passed in
 * "corners". The normalized coordinates are stored in "*pX" and "*pY".
 */
static void PATH_NormalizePoint(FLOAT_POINT corners[],
   const FLOAT_POINT *pPoint,
   double *pX, double *pY)
{
   *pX=(double)(pPoint->x-corners[0].x)/(double)(corners[1].x-corners[0].x) *
      2.0 - 1.0;
   *pY=(double)(pPoint->y-corners[0].y)/(double)(corners[1].y-corners[0].y) *
      2.0 - 1.0;
}


/*******************************************************************
 *      FlattenPath [GDI32.@]
 *
 *
 */
BOOL WINAPI FlattenPath(HDC hdc)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if(!dc) return FALSE;

    if(dc->funcs->pFlattenPath) ret = dc->funcs->pFlattenPath(dc->physDev);
    else
    {
	GdiPath *pPath = &dc->path;
        if(pPath->state != PATH_Closed)
	    ret = PATH_FlattenPath(pPath);
    }
    release_dc_ptr( dc );
    return ret;
}


static BOOL PATH_StrokePath(DC *dc, GdiPath *pPath)
{
    INT i, nLinePts, nAlloc;
    POINT *pLinePts;
    POINT ptViewportOrg, ptWindowOrg;
    SIZE szViewportExt, szWindowExt;
    DWORD mapMode, graphicsMode;
    XFORM xform;
    BOOL ret = TRUE;

    if(dc->funcs->pStrokePath)
        return dc->funcs->pStrokePath(dc->physDev);

    if(pPath->state != PATH_Closed)
        return FALSE;
    
    /* Save the mapping mode info */
    mapMode=GetMapMode(dc->hSelf);
    GetViewportExtEx(dc->hSelf, &szViewportExt);
    GetViewportOrgEx(dc->hSelf, &ptViewportOrg);
    GetWindowExtEx(dc->hSelf, &szWindowExt);
    GetWindowOrgEx(dc->hSelf, &ptWindowOrg);
    GetWorldTransform(dc->hSelf, &xform);

    /* Set MM_TEXT */
    SetMapMode(dc->hSelf, MM_TEXT);
    SetViewportOrgEx(dc->hSelf, 0, 0, NULL);
    SetWindowOrgEx(dc->hSelf, 0, 0, NULL);
    graphicsMode=GetGraphicsMode(dc->hSelf);
    SetGraphicsMode(dc->hSelf, GM_ADVANCED);
    ModifyWorldTransform(dc->hSelf, &xform, MWT_IDENTITY);
    SetGraphicsMode(dc->hSelf, graphicsMode);

    /* Allocate enough memory for the worst case without beziers (one PT_MOVETO
     * and the rest PT_LINETO with PT_CLOSEFIGURE at the end) plus some buffer 
     * space in case we get one to keep the number of reallocations small. */
    nAlloc = pPath->numEntriesUsed + 1 + 300; 
    pLinePts = HeapAlloc(GetProcessHeap(), 0, nAlloc * sizeof(POINT));
    nLinePts = 0;
    
    for(i = 0; i < pPath->numEntriesUsed; i++) {
        if((i == 0 || (pPath->pFlags[i-1] & PT_CLOSEFIGURE)) &&
	   (pPath->pFlags[i] != PT_MOVETO)) {
	    ERR("Expected PT_MOVETO %s, got path flag %d\n", 
	        i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
		(INT)pPath->pFlags[i]);
	    ret = FALSE;
	    goto end;
	}
        switch(pPath->pFlags[i]) {
	case PT_MOVETO:
            TRACE("Got PT_MOVETO (%d, %d)\n",
		  pPath->pPoints[i].x, pPath->pPoints[i].y);
	    if(nLinePts >= 2)
	        Polyline(dc->hSelf, pLinePts, nLinePts);
	    nLinePts = 0;
	    pLinePts[nLinePts++] = pPath->pPoints[i];
	    break;
	case PT_LINETO:
	case (PT_LINETO | PT_CLOSEFIGURE):
            TRACE("Got PT_LINETO (%d, %d)\n",
		  pPath->pPoints[i].x, pPath->pPoints[i].y);
	    pLinePts[nLinePts++] = pPath->pPoints[i];
	    break;
	case PT_BEZIERTO:
	    TRACE("Got PT_BEZIERTO\n");
	    if(pPath->pFlags[i+1] != PT_BEZIERTO ||
	       (pPath->pFlags[i+2] & ~PT_CLOSEFIGURE) != PT_BEZIERTO) {
	        ERR("Path didn't contain 3 successive PT_BEZIERTOs\n");
		ret = FALSE;
		goto end;
	    } else {
	        INT nBzrPts, nMinAlloc;
	        POINT *pBzrPts = GDI_Bezier(&pPath->pPoints[i-1], 4, &nBzrPts);
		/* Make sure we have allocated enough memory for the lines of 
		 * this bezier and the rest of the path, assuming we won't get
		 * another one (since we won't reallocate again then). */
		nMinAlloc = nLinePts + (pPath->numEntriesUsed - i) + nBzrPts;
		if(nAlloc < nMinAlloc)
		{
		    nAlloc = nMinAlloc * 2;
		    pLinePts = HeapReAlloc(GetProcessHeap(), 0, pLinePts,
		                           nAlloc * sizeof(POINT));
		}
		memcpy(&pLinePts[nLinePts], &pBzrPts[1],
		       (nBzrPts - 1) * sizeof(POINT));
		nLinePts += nBzrPts - 1;
		HeapFree(GetProcessHeap(), 0, pBzrPts);
		i += 2;
	    }
	    break;
	default:
	    ERR("Got path flag %d\n", (INT)pPath->pFlags[i]);
	    ret = FALSE;
	    goto end;
	}
	if(pPath->pFlags[i] & PT_CLOSEFIGURE)
	    pLinePts[nLinePts++] = pLinePts[0];
    }
    if(nLinePts >= 2)
        Polyline(dc->hSelf, pLinePts, nLinePts);

 end:
    HeapFree(GetProcessHeap(), 0, pLinePts);

    /* Restore the old mapping mode */
    SetMapMode(dc->hSelf, mapMode);
    SetWindowExtEx(dc->hSelf, szWindowExt.cx, szWindowExt.cy, NULL);
    SetWindowOrgEx(dc->hSelf, ptWindowOrg.x, ptWindowOrg.y, NULL);
    SetViewportExtEx(dc->hSelf, szViewportExt.cx, szViewportExt.cy, NULL);
    SetViewportOrgEx(dc->hSelf, ptViewportOrg.x, ptViewportOrg.y, NULL);

    /* Go to GM_ADVANCED temporarily to restore the world transform */
    graphicsMode=GetGraphicsMode(dc->hSelf);
    SetGraphicsMode(dc->hSelf, GM_ADVANCED);
    SetWorldTransform(dc->hSelf, &xform);
    SetGraphicsMode(dc->hSelf, graphicsMode);

    /* If we've moved the current point then get its new position
       which will be in device (MM_TEXT) co-ords, convert it to
       logical co-ords and re-set it.  This basically updates
       dc->CurPosX|Y so that their values are in the correct mapping
       mode.
    */
    if(i > 0) {
        POINT pt;
        GetCurrentPositionEx(dc->hSelf, &pt);
        DPtoLP(dc->hSelf, &pt, 1);
        MoveToEx(dc->hSelf, pt.x, pt.y, NULL);
    }

    return ret;
}

#define round(x) ((int)((x)>0?(x)+0.5:(x)-0.5))

static BOOL PATH_WidenPath(DC *dc)
{
    INT i, j, numStrokes, penWidth, penWidthIn, penWidthOut, size, penStyle;
    BOOL ret = FALSE;
    GdiPath *pPath, *pNewPath, **pStrokes = NULL, *pUpPath, *pDownPath;
    EXTLOGPEN *elp;
    DWORD obj_type, joint, endcap, penType;

    pPath = &dc->path;

    if(pPath->state == PATH_Open) {
       SetLastError(ERROR_CAN_NOT_COMPLETE);
       return FALSE;
    }

    PATH_FlattenPath(pPath);

    size = GetObjectW( dc->hPen, 0, NULL );
    if (!size) {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }

    elp = HeapAlloc( GetProcessHeap(), 0, size );
    GetObjectW( dc->hPen, size, elp );

    obj_type = GetObjectType(dc->hPen);
    if(obj_type == OBJ_PEN) {
        penStyle = ((LOGPEN*)elp)->lopnStyle;
    }
    else if(obj_type == OBJ_EXTPEN) {
        penStyle = elp->elpPenStyle;
    }
    else {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        HeapFree( GetProcessHeap(), 0, elp );
        return FALSE;
    }

    penWidth = elp->elpWidth;
    HeapFree( GetProcessHeap(), 0, elp );

    endcap = (PS_ENDCAP_MASK & penStyle);
    joint = (PS_JOIN_MASK & penStyle);
    penType = (PS_TYPE_MASK & penStyle);

    /* The function cannot apply to cosmetic pens */
    if(obj_type == OBJ_EXTPEN && penType == PS_COSMETIC) {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }

    penWidthIn = penWidth / 2;
    penWidthOut = penWidth / 2;
    if(penWidthIn + penWidthOut < penWidth)
        penWidthOut++;

    numStrokes = 0;

    for(i = 0, j = 0; i < pPath->numEntriesUsed; i++, j++) {
        POINT point;
        if((i == 0 || (pPath->pFlags[i-1] & PT_CLOSEFIGURE)) &&
            (pPath->pFlags[i] != PT_MOVETO)) {
            ERR("Expected PT_MOVETO %s, got path flag %c\n",
                i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
                pPath->pFlags[i]);
            return FALSE;
        }
        switch(pPath->pFlags[i]) {
            case PT_MOVETO:
                if(numStrokes > 0) {
                    pStrokes[numStrokes - 1]->state = PATH_Closed;
                }
                numStrokes++;
                j = 0;
                if(numStrokes == 1)
                    pStrokes = HeapAlloc(GetProcessHeap(), 0, sizeof(GdiPath*));
                else
                    pStrokes = HeapReAlloc(GetProcessHeap(), 0, pStrokes, numStrokes * sizeof(GdiPath*));
                if(!pStrokes) return FALSE;
                pStrokes[numStrokes - 1] = HeapAlloc(GetProcessHeap(), 0, sizeof(GdiPath));
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
                ERR("Should never happen\n");
                break;
            default:
                ERR("Got path flag %c\n", pPath->pFlags[i]);
                return FALSE;
        }
    }

    pNewPath = HeapAlloc(GetProcessHeap(), 0, sizeof(GdiPath));
    PATH_InitGdiPath(pNewPath);
    pNewPath->state = PATH_Open;

    for(i = 0; i < numStrokes; i++) {
        pUpPath = HeapAlloc(GetProcessHeap(), 0, sizeof(GdiPath));
        PATH_InitGdiPath(pUpPath);
        pUpPath->state = PATH_Open;
        pDownPath = HeapAlloc(GetProcessHeap(), 0, sizeof(GdiPath));
        PATH_InitGdiPath(pDownPath);
        pDownPath->state = PATH_Open;

        for(j = 0; j < pStrokes[i]->numEntriesUsed; j++) {
            /* Beginning or end of the path if not closed */
            if((!(pStrokes[i]->pFlags[pStrokes[i]->numEntriesUsed - 1] & PT_CLOSEFIGURE)) && (j == 0 || j == pStrokes[i]->numEntriesUsed - 1) ) {
                /* Compute segment angle */
                double xo, yo, xa, ya, theta;
                POINT pt;
                FLOAT_POINT corners[2];
                if(j == 0) {
                    xo = pStrokes[i]->pPoints[j].x;
                    yo = pStrokes[i]->pPoints[j].y;
                    xa = pStrokes[i]->pPoints[1].x;
                    ya = pStrokes[i]->pPoints[1].y;
                }
                else {
                    xa = pStrokes[i]->pPoints[j - 1].x;
                    ya = pStrokes[i]->pPoints[j - 1].y;
                    xo = pStrokes[i]->pPoints[j].x;
                    yo = pStrokes[i]->pPoints[j].y;
                }
                theta = atan2( ya - yo, xa - xo );
                switch(endcap) {
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
            else {
                /* Compute angle */
                INT previous, next;
                double xa, ya, xb, yb, xo, yo;
                double alpha, theta, miterWidth;
                DWORD _joint = joint;
                POINT pt;
		GdiPath *pInsidePath, *pOutsidePath;
                if(j > 0 && j < pStrokes[i]->numEntriesUsed - 1) {
                    previous = j - 1;
                    next = j + 1;
                }
                else if (j == 0) {
                    previous = pStrokes[i]->numEntriesUsed - 1;
                    next = j + 1;
                }
                else {
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
                if(_joint == PS_JOIN_MITER && dc->miterLimit < fabs(1 / sin(alpha/2))) {
                    _joint = PS_JOIN_BEVEL;
                }
                if(alpha > 0) {
                    pInsidePath = pUpPath;
                    pOutsidePath = pDownPath;
                }
                else if(alpha < 0) {
                    pInsidePath = pDownPath;
                    pOutsidePath = pUpPath;
                }
                else {
                    continue;
                }
                /* Inside angle points */
                if(alpha > 0) {
                    pt.x = xo - round( penWidthIn * cos(theta + M_PI_2) );
                    pt.y = yo - round( penWidthIn * sin(theta + M_PI_2) );
                }
                else {
                    pt.x = xo + round( penWidthIn * cos(theta + M_PI_2) );
                    pt.y = yo + round( penWidthIn * sin(theta + M_PI_2) );
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                if(alpha > 0) {
                    pt.x = xo + round( penWidthIn * cos(M_PI_2 + alpha + theta) );
                    pt.y = yo + round( penWidthIn * sin(M_PI_2 + alpha + theta) );
                }
                else {
                    pt.x = xo - round( penWidthIn * cos(M_PI_2 + alpha + theta) );
                    pt.y = yo - round( penWidthIn * sin(M_PI_2 + alpha + theta) );
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                /* Outside angle point */
                switch(_joint) {
                     case PS_JOIN_MITER :
                        miterWidth = fabs(penWidthOut / cos(M_PI_2 - fabs(alpha) / 2));
                        pt.x = xo + round( miterWidth * cos(theta + alpha / 2) );
                        pt.y = yo + round( miterWidth * sin(theta + alpha / 2) );
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_BEVEL :
                        if(alpha > 0) {
                            pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        else {
                            pt.x = xo - round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo - round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        if(alpha > 0) {
                            pt.x = xo - round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo - round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        else {
                            pt.x = xo + round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo + round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_ROUND :
                    default :
                        if(alpha > 0) {
                            pt.x = xo + round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo + round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        else {
                            pt.x = xo - round( penWidthOut * cos(theta + M_PI_2) );
                            pt.y = yo - round( penWidthOut * sin(theta + M_PI_2) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        pt.x = xo + round( penWidthOut * cos(theta + alpha / 2) );
                        pt.y = yo + round( penWidthOut * sin(theta + alpha / 2) );
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        if(alpha > 0) {
                            pt.x = xo - round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo - round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        else {
                            pt.x = xo + round( penWidthOut * cos(M_PI_2 + alpha + theta) );
                            pt.y = yo + round( penWidthOut * sin(M_PI_2 + alpha + theta) );
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        break;
                }
            }
        }
        for(j = 0; j < pUpPath->numEntriesUsed; j++) {
            POINT pt;
            pt.x = pUpPath->pPoints[j].x;
            pt.y = pUpPath->pPoints[j].y;
            PATH_AddEntry(pNewPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO));
        }
        for(j = 0; j < pDownPath->numEntriesUsed; j++) {
            POINT pt;
            pt.x = pDownPath->pPoints[pDownPath->numEntriesUsed - j - 1].x;
            pt.y = pDownPath->pPoints[pDownPath->numEntriesUsed - j - 1].y;
            PATH_AddEntry(pNewPath, &pt, ( (j == 0 && (pStrokes[i]->pFlags[pStrokes[i]->numEntriesUsed - 1] & PT_CLOSEFIGURE)) ? PT_MOVETO : PT_LINETO));
        }

        PATH_DestroyGdiPath(pStrokes[i]);
        HeapFree(GetProcessHeap(), 0, pStrokes[i]);
        PATH_DestroyGdiPath(pUpPath);
        HeapFree(GetProcessHeap(), 0, pUpPath);
        PATH_DestroyGdiPath(pDownPath);
        HeapFree(GetProcessHeap(), 0, pDownPath);
    }
    HeapFree(GetProcessHeap(), 0, pStrokes);

    pNewPath->state = PATH_Closed;
    if (!(ret = PATH_AssignGdiPath(pPath, pNewPath)))
        ERR("Assign path failed\n");
    PATH_DestroyGdiPath(pNewPath);
    HeapFree(GetProcessHeap(), 0, pNewPath);
    return ret;
}


/*******************************************************************
 *      StrokeAndFillPath [GDI32.@]
 *
 *
 */
BOOL WINAPI StrokeAndFillPath(HDC hdc)
{
   DC *dc = get_dc_ptr( hdc );
   BOOL bRet = FALSE;

   if(!dc) return FALSE;

   if(dc->funcs->pStrokeAndFillPath)
       bRet = dc->funcs->pStrokeAndFillPath(dc->physDev);
   else
   {
       bRet = PATH_FillPath(dc, &dc->path);
       if(bRet) bRet = PATH_StrokePath(dc, &dc->path);
       if(bRet) PATH_EmptyPath(&dc->path);
   }
   release_dc_ptr( dc );
   return bRet;
}


/*******************************************************************
 *      StrokePath [GDI32.@]
 *
 *
 */
BOOL WINAPI StrokePath(HDC hdc)
{
    DC *dc = get_dc_ptr( hdc );
    GdiPath *pPath;
    BOOL bRet = FALSE;

    TRACE("(%p)\n", hdc);
    if(!dc) return FALSE;

    if(dc->funcs->pStrokePath)
        bRet = dc->funcs->pStrokePath(dc->physDev);
    else
    {
        pPath = &dc->path;
        bRet = PATH_StrokePath(dc, pPath);
        PATH_EmptyPath(pPath);
    }
    release_dc_ptr( dc );
    return bRet;
}


/*******************************************************************
 *      WidenPath [GDI32.@]
 *
 *
 */
BOOL WINAPI WidenPath(HDC hdc)
{
   DC *dc = get_dc_ptr( hdc );
   BOOL ret = FALSE;

   if(!dc) return FALSE;

   if(dc->funcs->pWidenPath)
      ret = dc->funcs->pWidenPath(dc->physDev);
   else
      ret = PATH_WidenPath(dc);
   release_dc_ptr( dc );
   return ret;
}
