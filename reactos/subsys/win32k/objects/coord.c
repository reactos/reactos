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
/* $Id: coord.c,v 1.20 2003/12/21 18:38:37 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             subsys/win32k/objects/coord.c
 * PROGRAMER:        Unknown
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/coord.h>
#include <win32k/dc.h>
#include <include/error.h>
#define NDEBUG
#include <win32k/debug1.h>

/* FUNCTIONS *****************************************************************/

BOOL STDCALL NtGdiCombineTransform(LPXFORM  UnsafeXFormResult,
                           CONST LPXFORM  Unsafexform1,
                           CONST LPXFORM  Unsafexform2)
{
  XFORM  xformTemp;
  XFORM  xform1, xform2;

  /* Check for illegal parameters */
  if (!UnsafeXFormResult || !Unsafexform1 || !Unsafexform2)
  {
    return  FALSE;
  }

  MmCopyFromCaller( &xform1, Unsafexform1, sizeof(XFORM) );
  MmCopyFromCaller( &xform2, Unsafexform2, sizeof(XFORM) );

  /* Create the result in a temporary XFORM, since xformResult may be
   * equal to xform1 or xform2 */
  xformTemp.eM11 = xform1.eM11 * xform2.eM11 + xform1.eM12 * xform2.eM21;
  xformTemp.eM12 = xform1.eM11 * xform2.eM12 + xform1.eM12 * xform2.eM22;
  xformTemp.eM21 = xform1.eM21 * xform2.eM11 + xform1.eM22 * xform2.eM21;
  xformTemp.eM22 = xform1.eM21 * xform2.eM12 + xform1.eM22 * xform2.eM22;
  xformTemp.eDx  = xform1.eDx  * xform2.eM11 + xform1.eDy  * xform2.eM21 + xform2.eDx;
  xformTemp.eDy  = xform1.eDx  * xform2.eM12 + xform1.eDy  * xform2.eM22 + xform2.eDy;

  /* Copy the result to xformResult */
  MmCopyToCaller(  UnsafeXFormResult, &xformTemp, sizeof(XFORM) );

  return  TRUE;
}

VOID FASTCALL
CoordDPtoLP(PDC Dc, LPPOINT Point)
{
FLOAT x, y;
  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;
  Point->x = x * Dc->w.xformVport2World.eM11 +
    y * Dc->w.xformVport2World.eM21 + Dc->w.xformVport2World.eDx;
  Point->y = x * Dc->w.xformVport2World.eM12 +
    y * Dc->w.xformVport2World.eM22 + Dc->w.xformVport2World.eDy;
}

VOID
FASTCALL
IntDPtoLP ( PDC dc, LPPOINT Points, INT Count )
{
  INT i;

  ASSERT ( Points );

  for ( i = 0; i < Count; i++ )
    CoordDPtoLP ( dc, &Points[i] );
}

/*!
 * Converts points from device coordinates into logical coordinates. Conversion depends on the mapping mode,
 * world transfrom, viewport origin settings for the given device context.
 * \param	hDC		device context.
 * \param	Points	an array of POINT structures (in/out).
 * \param	Count	number of elements in the array of POINT structures.
 * \return  TRUE 	if success.
*/
BOOL STDCALL
NtGdiDPtoLP(HDC  hDC,
	   LPPOINT  UnsafePoints,
	   int  Count)
{
   PDC dc;
   LPPOINT Points = (LPPOINT)ExAllocatePool(PagedPool, Count * sizeof(POINT));
   BOOL ret = FALSE; // default to failure

   if (!Points)
      return FALSE;

   dc = DC_LockDc(hDC);
   if (dc)
   {
      ret = TRUE;
      MmCopyFromCaller(Points, UnsafePoints, Count * sizeof(POINT));
      IntDPtoLP(dc, Points, Count);
      MmCopyToCaller(UnsafePoints, Points, Count * sizeof(POINT));
      DC_UnlockDc(hDC);
   }
   ExFreePool(Points);

   return ret;
}

int
FASTCALL
IntGetGraphicsMode ( PDC dc )
{
  ASSERT ( dc );
  return dc->w.GraphicsMode;
}

int
STDCALL
NtGdiGetGraphicsMode ( HDC hDC )
{
  PDC dc = DC_LockDc ( hDC );
  int GraphicsMode = 0; // default to failure

  if ( dc )
  {
    GraphicsMode = dc->w.GraphicsMode;
    DC_UnlockDc ( hDC );
  }

  return GraphicsMode;
}

BOOL
STDCALL
NtGdiGetWorldTransform(HDC  hDC,
                      LPXFORM  XForm)
{
  PDC  dc;

  if (!XForm)
    return FALSE;

  dc = DC_LockDc (hDC);
  if (!dc)
    return FALSE;

  *XForm = dc->w.xformWorld2Wnd;

  DC_UnlockDc ( hDC );

  return  TRUE;
}

VOID
FASTCALL
CoordLPtoDP ( PDC Dc, LPPOINT Point )
{
  FLOAT x, y;

  ASSERT ( Dc );
  ASSERT ( Point );

  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;
  Point->x = x * Dc->w.xformWorld2Vport.eM11 +
    y * Dc->w.xformWorld2Vport.eM21 + Dc->w.xformWorld2Vport.eDx;
  Point->y = x * Dc->w.xformWorld2Vport.eM12 +
    y * Dc->w.xformWorld2Vport.eM22 + Dc->w.xformWorld2Vport.eDy;
}

VOID
FASTCALL
IntLPtoDP ( PDC dc, LPPOINT Points, INT Count )
{
  INT i;

  ASSERT ( Points );

  for ( i = 0; i < Count; i++ )
    CoordLPtoDP ( dc, &Points[i] );
}

/*!
 * Converts points from logical coordinates into device coordinates. Conversion depends on the mapping mode,
 * world transfrom, viewport origin settings for the given device context.
 * \param	hDC		device context.
 * \param	Points	an array of POINT structures (in/out).
 * \param	Count	number of elements in the array of POINT structures.
 * \return  TRUE 	if success.
*/
BOOL STDCALL
NtGdiLPtoDP ( HDC hDC, LPPOINT UnsafePoints, INT Count )
{
  PDC dc;
  LPPOINT Points = (LPPOINT)ExAllocatePool ( PagedPool, Count*sizeof(POINT) );
  BOOL    ret = FALSE; // default to failure

  ASSERT(Points);
  if ( !Points )
    return FALSE;

  dc = DC_LockDc ( hDC );

  if ( dc )
  {
    ret = TRUE;

    MmCopyFromCaller( Points, UnsafePoints, Count*sizeof(POINT) );

    IntLPtoDP ( dc, Points, Count );

    MmCopyToCaller ( UnsafePoints, Points, Count*sizeof(POINT) );

    DC_UnlockDc ( hDC );
  }

  ExFreePool ( Points );

  return ret;
}

BOOL
STDCALL
NtGdiModifyWorldTransform(HDC            hDC,
                          CONST LPXFORM  UnsafeXForm,
                          DWORD          Mode)
{
  PDC  dc;
  LPXFORM XForm = (LPXFORM) ExAllocatePool( PagedPool, sizeof( XFORM ) );
  BOOL ret = FALSE; // default to failure

  ASSERT( XForm );
  if (!XForm)
    return FALSE;

  MmCopyFromCaller( XForm, UnsafeXForm, sizeof( XFORM ) );

  dc = DC_LockDc (hDC);
  if ( dc )
  {
    /* Check that graphics mode is GM_ADVANCED */
    if ( dc->w.GraphicsMode == GM_ADVANCED )
    {
      ret = TRUE; // switch to a default of success
      switch (Mode)
      {
	case MWT_IDENTITY:
	  dc->w.xformWorld2Wnd.eM11 = 1.0f;
	  dc->w.xformWorld2Wnd.eM12 = 0.0f;
	  dc->w.xformWorld2Wnd.eM21 = 0.0f;
	  dc->w.xformWorld2Wnd.eM22 = 1.0f;
	  dc->w.xformWorld2Wnd.eDx  = 0.0f;
	  dc->w.xformWorld2Wnd.eDy  = 0.0f;
	  break;

	case MWT_LEFTMULTIPLY:
	  NtGdiCombineTransform(&dc->w.xformWorld2Wnd, XForm, &dc->w.xformWorld2Wnd );
	  break;

	case MWT_RIGHTMULTIPLY:
	  NtGdiCombineTransform(&dc->w.xformWorld2Wnd, &dc->w.xformWorld2Wnd, XForm);
	  break;

	default:
	  ret = FALSE;
	  break;
      }
      if ( ret )
        DC_UpdateXforms ( dc );
    }
    DC_UnlockDc ( hDC );
  }
  ExFreePool ( XForm );
  return ret;
}

BOOL
STDCALL
NtGdiOffsetViewportOrgEx(HDC hDC,
                        int XOffset,
                        int YOffset,
                        LPPOINT UnsafePoint)
{
  PDC      dc;
  POINT    Point;
  NTSTATUS Status;
  BOOL     ret = FALSE; // default to failure

  dc = DC_LockDc ( hDC );
  if ( dc )
  {
    ret = TRUE;
    if (NULL != UnsafePoint)
      {
	Point.x = dc->vportOrgX;
	Point.y = dc->vportOrgY;
	Status = MmCopyToCaller(UnsafePoint, &Point, sizeof(POINT));
	if ( !NT_SUCCESS(Status) )
	  {
	    SetLastNtError(Status);
	    ret = FALSE;
	  }
      }

    if ( ret )
    {
      dc->vportOrgX += XOffset;
      dc->vportOrgY += YOffset;
      DC_UpdateXforms(dc);

      dc->w.DCOrgX += XOffset;
      dc->w.DCOrgY += YOffset;
    }

    DC_UnlockDc ( hDC );
  }
  return TRUE;
}

BOOL
STDCALL
NtGdiOffsetWindowOrgEx(HDC  hDC,
                      int  XOffset,
                      int  YOffset,
                      LPPOINT  Point)
{
  PDC dc;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      return FALSE;
    }

  if (Point)
    {
      Point->x = dc->wndOrgX;
      Point->y = dc->wndOrgY;
    }

  dc->wndOrgX += XOffset;
  dc->wndOrgY += YOffset;

  DC_UnlockDc(hDC);

  return TRUE;
}

BOOL
STDCALL
NtGdiScaleViewportExtEx(HDC  hDC,
                             int  Xnum,
                             int  Xdenom,
                             int  Ynum,
                             int  Ydenom,
                             LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiScaleWindowExtEx(HDC  hDC,
                           int  Xnum,
                           int  Xdenom,
                           int  Ynum,
                           int  Ydenom,
                           LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int
STDCALL
NtGdiSetGraphicsMode(HDC  hDC,
                    int  Mode)
{
  INT ret;
  PDC dc;

  dc = DC_LockDc (hDC);
  if (!dc)
  {
    return 0;
  }

  /* One would think that setting the graphics mode to GM_COMPATIBLE
   * would also reset the world transformation matrix to the unity
   * matrix. However, in Windows, this is not the case. This doesn't
   * make a lot of sense to me, but that's the way it is.
   */

  if ((Mode != GM_COMPATIBLE) && (Mode != GM_ADVANCED))
    {
      DC_UnlockDc( hDC );
      return 0;
    }

  ret = dc->w.GraphicsMode;
  dc->w.GraphicsMode = Mode;
  DC_UnlockDc( hDC );
  return  ret;
}

int
STDCALL
NtGdiSetMapMode(HDC  hDC,
                int  MapMode)
{
  int PrevMapMode;
  PDC dc;

  dc = DC_LockDc(hDC);
  if (!dc)
    return 0;

  PrevMapMode = dc->w.MapMode;
  dc->w.MapMode = MapMode;

  DC_UnlockDc ( hDC );

  return PrevMapMode;
}

BOOL
STDCALL
NtGdiSetViewportExtEx(HDC  hDC,
                      int  XExtent,
                      int  YExtent,
                      LPSIZE  Size)
{
  PDC dc;

  dc = DC_LockDc(hDC);
  if ( !dc )
    return FALSE;

  switch (dc->w.MapMode)
    {
      case MM_HIENGLISH:
      case MM_HIMETRIC:
      case MM_LOENGLISH:
      case MM_LOMETRIC:
      case MM_TEXT:
      case MM_TWIPS:
	DC_UnlockDc(hDC);
	return FALSE;

      case MM_ISOTROPIC:
	// Here we should (probably) check that SetWindowExtEx *really* has
	// been called
	break;
    }

  if (Size)
    {
      Size->cx = dc->vportExtX;
      Size->cy = dc->vportExtY;
    }

  dc->vportExtX = XExtent;
  dc->vportExtY = YExtent;

  DC_UnlockDc(hDC);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetViewportOrgEx(HDC  hDC,
                     int  X,
                     int  Y,
                     LPPOINT  Point)
{
  PDC dc;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      return FALSE;
    }

  if (Point)
    {
      Point->x = dc->vportOrgX;
      Point->y = dc->vportOrgY;
    }

  dc->vportOrgX = X;
  dc->vportOrgY = Y;

  DC_UnlockDc(hDC);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetWindowExtEx(HDC  hDC,
                   int  XExtent,
                   int  YExtent,
                   LPSIZE  Size)
{
  PDC dc;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      return FALSE;
    }

  switch (dc->w.MapMode)
    {
      case MM_HIENGLISH:
      case MM_HIMETRIC:
      case MM_LOENGLISH:
      case MM_LOMETRIC:
      case MM_TEXT:
      case MM_TWIPS:
	DC_UnlockDc(hDC);
	return FALSE;
    }

  if (Size)
    {
      Size->cx = dc->wndExtX;
      Size->cy = dc->wndExtY;
    }

  dc->wndExtX = XExtent;
  dc->wndExtY = YExtent;

  DC_UnlockDc(hDC);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetWindowOrgEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point)
{
  PDC dc;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      return FALSE;
    }

  if (Point)
    {
      Point->x = dc->wndOrgX;
      Point->y = dc->wndOrgY;
    }

  dc->wndOrgX = X;
  dc->wndOrgY = Y;

  DC_UnlockDc(hDC);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetWorldTransform(HDC  hDC,
                      CONST LPXFORM  XForm)
{
  PDC  dc;

  if (!XForm)
    return  FALSE;

  dc = DC_LockDc (hDC);
  if ( !dc )
  {
    return  FALSE;
  }

  /* Check that graphics mode is GM_ADVANCED */
  if ( dc->w.GraphicsMode != GM_ADVANCED )
  {
    DC_UnlockDc( hDC );
    return  FALSE;
  }
  dc->w.xformWorld2Wnd = *XForm;
  DC_UpdateXforms (dc);
  DC_UnlockDc( hDC );
  return  TRUE;
}

/* EOF */
