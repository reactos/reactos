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
/* $Id: coord.c,v 1.25 2004/06/18 15:18:54 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             subsys/win32k/objects/coord.c
 * PROGRAMER:        Unknown
 */

/* INCLUDES ******************************************************************/
#include <w32k.h>

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL
IntGdiCombineTransform(LPXFORM XFormResult,
                       LPXFORM xform1,
                       LPXFORM xform2)
{
  /* Check for illegal parameters */
  if (!XFormResult || !xform1 || !xform2)
  {
    return  FALSE;
  }
  
  /* Create the result in a temporary XFORM, since xformResult may be
   * equal to xform1 or xform2 */
  XFormResult->eM11 = xform1->eM11 * xform2->eM11 + xform1->eM12 * xform2->eM21;
  XFormResult->eM12 = xform1->eM11 * xform2->eM12 + xform1->eM12 * xform2->eM22;
  XFormResult->eM21 = xform1->eM21 * xform2->eM11 + xform1->eM22 * xform2->eM21;
  XFormResult->eM22 = xform1->eM21 * xform2->eM12 + xform1->eM22 * xform2->eM22;
  XFormResult->eDx  = xform1->eDx  * xform2->eM11 + xform1->eDy  * xform2->eM21 + xform2->eDx;
  XFormResult->eDy  = xform1->eDx  * xform2->eM12 + xform1->eDy  * xform2->eM22 + xform2->eDy;
  
  return TRUE;
}

BOOL STDCALL NtGdiCombineTransform(LPXFORM  UnsafeXFormResult,
                           CONST LPXFORM  Unsafexform1,
                           CONST LPXFORM  Unsafexform2)
{
  XFORM  xformTemp;
  XFORM  xform1, xform2;
  NTSTATUS Status;
  BOOL Ret;


  Status = MmCopyFromCaller( &xform1, Unsafexform1, sizeof(XFORM) );
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  Status = MmCopyFromCaller( &xform2, Unsafexform2, sizeof(XFORM) );
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  Ret = IntGdiCombineTransform(&xformTemp, &xform1, &xform2);

  /* Copy the result to xformResult */
  Status = MmCopyToCaller(  UnsafeXFormResult, &xformTemp, sizeof(XFORM) );
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  return Ret;
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
   NTSTATUS Status;
   LPPOINT Points;
   ULONG Size;

   dc = DC_LockDc(hDC);
   if (!dc)
   {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
   }
   
   if (!UnsafePoints || Count <= 0)
   {
     DC_UnlockDc(hDC);
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }
   
   Size = Count * sizeof(POINT);
   
   Points = (LPPOINT)ExAllocatePoolWithTag(PagedPool, Size, TAG_COORD);
   if(!Points)
   {
     DC_UnlockDc(hDC);
     SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
     return FALSE;
   }
   
   Status = MmCopyFromCaller(Points, UnsafePoints, Size);
   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(hDC);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }
   
   IntDPtoLP(dc, Points, Count);
   
   Status = MmCopyToCaller(UnsafePoints, Points, Size);
   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(hDC);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }
   
   DC_UnlockDc(hDC);
   ExFreePool(Points);
   return TRUE;
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
  PDC dc;
  int GraphicsMode; // default to failure
  
  dc = DC_LockDc ( hDC );
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }
  
  GraphicsMode = dc->w.GraphicsMode;
  
  DC_UnlockDc ( hDC );
  return GraphicsMode;
}

BOOL
STDCALL
NtGdiGetWorldTransform(HDC  hDC,
                      LPXFORM  XForm)
{
  PDC  dc;
  NTSTATUS Status;
  
  dc = DC_LockDc ( hDC );
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (!XForm)
  {
    DC_UnlockDc ( hDC );
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Status = MmCopyToCaller(XForm, &dc->w.xformWorld2Wnd, sizeof(XFORM));

  DC_UnlockDc ( hDC );
  return NT_SUCCESS(Status);
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
   NTSTATUS Status;
   LPPOINT Points;
   ULONG Size;

   dc = DC_LockDc(hDC);
   if (!dc)
   {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
   }
   
   if (!UnsafePoints || Count <= 0)
   {
     DC_UnlockDc(hDC);
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }
   
   Size = Count * sizeof(POINT);
   
   Points = (LPPOINT)ExAllocatePoolWithTag(PagedPool, Size, TAG_COORD);
   if(!Points)
   {
     DC_UnlockDc(hDC);
     SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
     return FALSE;
   }
   
   Status = MmCopyFromCaller(Points, UnsafePoints, Size);
   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(hDC);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }
   
   IntLPtoDP(dc, Points, Count);
   
   Status = MmCopyToCaller(UnsafePoints, Points, Size);
   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(hDC);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }
   
   DC_UnlockDc(hDC);
   ExFreePool(Points);
   return TRUE;
}

BOOL
STDCALL
NtGdiModifyWorldTransform(HDC            hDC,
                          CONST LPXFORM  UnsafeXForm,
                          DWORD          Mode)
{
   PDC dc;
   XFORM SafeXForm;
   NTSTATUS Status;
   
   dc = DC_LockDc(hDC);
   if (!dc)
   {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
   }
   
   if (!UnsafeXForm)
   {
     DC_UnlockDc(hDC);
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }
   
   Status = MmCopyFromCaller(&SafeXForm, UnsafeXForm, sizeof(XFORM));
   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(hDC);
     SetLastNtError(Status);
     return FALSE;
   }
   
   switch(Mode)
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
       IntGdiCombineTransform(&dc->w.xformWorld2Wnd, &SafeXForm, &dc->w.xformWorld2Wnd );
       break;
     
     case MWT_RIGHTMULTIPLY:
       IntGdiCombineTransform(&dc->w.xformWorld2Wnd, &dc->w.xformWorld2Wnd, &SafeXForm);
       break;
     
     default:
       DC_UnlockDc(hDC);
       SetLastWin32Error(ERROR_INVALID_PARAMETER);
       return FALSE;
  }
  
  DC_UpdateXforms(dc);
  DC_UnlockDc(hDC);
  return TRUE;
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

  dc = DC_LockDc ( hDC );
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if (UnsafePoint)
    {
	Point.x = dc->vportOrgX;
	Point.y = dc->vportOrgY;
	Status = MmCopyToCaller(UnsafePoint, &Point, sizeof(POINT));
	if ( !NT_SUCCESS(Status) )
	  {
	    SetLastNtError(Status);
	    DC_UnlockDc ( hDC );
	    return FALSE;
	  }
    }

  dc->vportOrgX += XOffset;
  dc->vportOrgY += YOffset;
  DC_UpdateXforms(dc);
  
  DC_UnlockDc ( hDC );
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
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (Point)
    {
      POINT SafePoint;
      NTSTATUS Status;
      
      SafePoint.x = dc->wndOrgX;
      SafePoint.y = dc->wndOrgY;
      
      Status = MmCopyToCaller(Point, &SafePoint, sizeof(POINT));
      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(hDC);
        return FALSE;
      }
    }

  dc->wndOrgX += XOffset;
  dc->wndOrgY += YOffset;

  DC_UpdateXforms(dc);
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
    SetLastWin32Error(ERROR_INVALID_HANDLE);
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
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

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
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
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

      case MM_ISOTROPIC:
	// Here we should (probably) check that SetWindowExtEx *really* has
	// been called
	break;
    }

  if (Size)
    {
      SIZE SafeSize;
      NTSTATUS Status;
      
      SafeSize.cx = dc->vportExtX;
      SafeSize.cy = dc->vportExtY;
      
      Status = MmCopyToCaller(Size, &SafeSize, sizeof(SIZE));
      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(hDC);
        return FALSE;
      }
    }

  dc->vportExtX = XExtent;
  dc->vportExtY = YExtent;

  DC_UpdateXforms(dc);
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
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (Point)
    {
      POINT SafePoint;
      NTSTATUS Status;
      
      SafePoint.x = dc->vportOrgX;
      SafePoint.y = dc->vportOrgY;
      
      Status = MmCopyToCaller(Point, &SafePoint, sizeof(POINT));
      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(hDC);
        return FALSE;
      }
    }

  dc->vportOrgX = X;
  dc->vportOrgY = Y;

  DC_UpdateXforms(dc);
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
      SetLastWin32Error(ERROR_INVALID_HANDLE);
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
      SIZE SafeSize;
      NTSTATUS Status;
      
      SafeSize.cx = dc->wndExtX;
      SafeSize.cy = dc->wndExtY;
      
      Status = MmCopyToCaller(Size, &SafeSize, sizeof(SIZE));
      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(hDC);
        return FALSE;
      }
    }
  
  dc->wndExtX = XExtent;
  dc->wndExtY = YExtent;
  
  DC_UpdateXforms(dc);
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
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  
  if (Point)
    {
      POINT SafePoint;
      NTSTATUS Status;
      
      SafePoint.x = dc->wndOrgX;
      SafePoint.y = dc->wndOrgY;
      
      Status = MmCopyToCaller(Point, &SafePoint, sizeof(POINT));
      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(hDC);
        return FALSE;
      }
    }
  
  dc->wndOrgX = X;
  dc->wndOrgY = Y;
  
  DC_UpdateXforms(dc);
  DC_UnlockDc(hDC);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetWorldTransform(HDC  hDC,
                      CONST LPXFORM  XForm)
{
  PDC  dc;
  NTSTATUS Status;
  
  dc = DC_LockDc (hDC);
  if ( !dc )
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return  FALSE;
  }
  
  if (!XForm)
  {
    DC_UnlockDc( hDC );
    /* Win doesn't set LastError */
    return  FALSE;
  }
  
  /* Check that graphics mode is GM_ADVANCED */
  if ( dc->w.GraphicsMode != GM_ADVANCED )
  {
    DC_UnlockDc( hDC );
    return  FALSE;
  }
  
  Status = MmCopyFromCaller(&dc->w.xformWorld2Wnd, XForm, sizeof(XFORM));
  if(!NT_SUCCESS(Status))
  {
    DC_UnlockDc( hDC );
    return FALSE;
  }
  
  DC_UpdateXforms (dc);
  DC_UnlockDc( hDC );
  return  TRUE;
}

/* EOF */
