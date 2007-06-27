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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             subsys/win32k/objects/coord.c
 * PROGRAMER:        Unknown
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

void FASTCALL
IntFixIsotropicMapping(PDC dc)
{
  ULONG xdim = EngMulDiv(dc->vportExtX, dc->GDIInfo->ulHorzSize, dc->GDIInfo->ulHorzRes) / dc->wndExtX;
  ULONG ydim = EngMulDiv(dc->vportExtY, dc->GDIInfo->ulVertSize, dc->GDIInfo->ulVertRes) / dc->wndExtY;

  if (xdim > ydim)
  {
    dc->vportExtX = dc->vportExtX * abs(ydim / xdim);
    if (!dc->vportExtX) dc->vportExtX = 1;
  }
  else
  {
    dc->vportExtY = dc->vportExtY * abs(xdim / ydim);
    if (!dc->vportExtY) dc->vportExtY = 1;
  }
}

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
  XFORM  xform1 = {0}, xform2 = {0};
  NTSTATUS Status = STATUS_SUCCESS;
  BOOL Ret;

  _SEH_TRY
  {
    ProbeForWrite(UnsafeXFormResult,
                  sizeof(XFORM),
                  1);
    ProbeForRead(Unsafexform1,
                 sizeof(XFORM),
                 1);
    ProbeForRead(Unsafexform2,
                 sizeof(XFORM),
                 1);
    xform1 = *Unsafexform1;
    xform2 = *Unsafexform2;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  Ret = IntGdiCombineTransform(&xformTemp, &xform1, &xform2);

  /* Copy the result to xformResult */
  _SEH_TRY
  {
    /* pointer was already probed! */
    *UnsafeXFormResult = xformTemp;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

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
   NTSTATUS Status = STATUS_SUCCESS;
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
     DC_UnlockDc(dc);
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }

   Size = Count * sizeof(POINT);

   Points = (LPPOINT)ExAllocatePoolWithTag(PagedPool, Size, TAG_COORD);
   if(!Points)
   {
     DC_UnlockDc(dc);
     SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
     return FALSE;
   }

   _SEH_TRY
   {
      ProbeForWrite(UnsafePoints,
                    Size,
                    1);
      RtlCopyMemory(Points,
                    UnsafePoints,
                    Size);
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;
   
   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(dc);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }

   IntDPtoLP(dc, Points, Count);

   _SEH_TRY
   {
      /* pointer was already probed! */
      RtlCopyMemory(UnsafePoints,
                    Points,
                    Size);
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(dc);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }

   DC_UnlockDc(dc);
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

BOOL
FASTCALL
IntGdiModifyWorldTransform(PDC pDc,
                           CONST LPXFORM lpXForm,
                           DWORD Mode)
{
   ASSERT(pDc && lpXForm);

   switch(Mode)
   {
     case MWT_IDENTITY:
       pDc->w.xformWorld2Wnd.eM11 = 1.0f;
       pDc->w.xformWorld2Wnd.eM12 = 0.0f;
       pDc->w.xformWorld2Wnd.eM21 = 0.0f;
       pDc->w.xformWorld2Wnd.eM22 = 1.0f;
       pDc->w.xformWorld2Wnd.eDx  = 0.0f;
       pDc->w.xformWorld2Wnd.eDy  = 0.0f;
       break;

     case MWT_LEFTMULTIPLY:
       IntGdiCombineTransform(&pDc->w.xformWorld2Wnd, lpXForm, &pDc->w.xformWorld2Wnd );
       break;

     case MWT_RIGHTMULTIPLY:
       IntGdiCombineTransform(&pDc->w.xformWorld2Wnd, &pDc->w.xformWorld2Wnd, lpXForm);
       break;

     default:
       SetLastWin32Error(ERROR_INVALID_PARAMETER);
       return FALSE;
  }

  DC_UpdateXforms(pDc);
  DC_UnlockDc(pDc);
  return TRUE;
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

  DC_UnlockDc(dc);
  return GraphicsMode;
}

BOOL
STDCALL
NtGdiGetWorldTransform(HDC  hDC,
                      LPXFORM  XForm)
{
  PDC  dc;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc ( hDC );
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (!XForm)
  {
    DC_UnlockDc(dc);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  _SEH_TRY
  {
    ProbeForWrite(XForm,
                  sizeof(XFORM),
                  1);
    *XForm = dc->w.xformWorld2Wnd;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  DC_UnlockDc(dc);
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
   NTSTATUS Status = STATUS_SUCCESS;
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
     DC_UnlockDc(dc);
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }

   Size = Count * sizeof(POINT);

   Points = (LPPOINT)ExAllocatePoolWithTag(PagedPool, Size, TAG_COORD);
   if(!Points)
   {
     DC_UnlockDc(dc);
     SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
     return FALSE;
   }

   _SEH_TRY
   {
      ProbeForWrite(UnsafePoints,
                    Size,
                    1);
      RtlCopyMemory(Points,
                    UnsafePoints,
                    Size);
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(dc);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }

   IntLPtoDP(dc, Points, Count);

   _SEH_TRY
   {
      /* pointer was already probed! */
      RtlCopyMemory(UnsafePoints,
                    Points,
                    Size);
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(dc);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }

   DC_UnlockDc(dc);
   ExFreePool(Points);
   return TRUE;
}

BOOL
STDCALL
NtGdiModifyWorldTransform(HDC hDC,
                          CONST LPXFORM  UnsafeXForm,
                          DWORD Mode)
{
   PDC dc;
   XFORM SafeXForm;
   BOOL Ret = FALSE;

   if (!UnsafeXForm)
   {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }
   
   dc = DC_LockDc(hDC);
   if (!dc)
   {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
   }

   _SEH_TRY
   {
      ProbeForRead(UnsafeXForm, sizeof(XFORM), 1);
      RtlCopyMemory(&SafeXForm, UnsafeXForm, sizeof(XFORM));
      
      Ret = IntGdiModifyWorldTransform(dc, &SafeXForm, Mode);
   }
   _SEH_HANDLE
   {
      SetLastNtError(_SEH_GetExceptionCode());
   }
   _SEH_END;

   DC_UnlockDc(dc);
   return Ret;
}

BOOL
STDCALL
NtGdiOffsetViewportOrgEx(HDC hDC,
                        int XOffset,
                        int YOffset,
                        LPPOINT UnsafePoint)
{
  PDC      dc;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc ( hDC );
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if (UnsafePoint)
    {
        _SEH_TRY
        {
            ProbeForWrite(UnsafePoint,
                          sizeof(POINT),
                          1);
            UnsafePoint->x = dc->vportOrgX;
            UnsafePoint->y = dc->vportOrgY;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

	if ( !NT_SUCCESS(Status) )
	  {
	    SetLastNtError(Status);
	    DC_UnlockDc(dc);
	    return FALSE;
	  }
    }

  dc->vportOrgX += XOffset;
  dc->vportOrgY += YOffset;
  DC_UpdateXforms(dc);

  DC_UnlockDc(dc);
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
      NTSTATUS Status = STATUS_SUCCESS;
      
      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = dc->wndOrgX;
         Point->y = dc->wndOrgY;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  dc->wndOrgX += XOffset;
  dc->wndOrgY += YOffset;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

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
   return FALSE;
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
   return FALSE;
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
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
    }

  ret = dc->w.GraphicsMode;
  dc->w.GraphicsMode = Mode;
  DC_UnlockDc(dc);
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

  if (MapMode != dc->w.MapMode || (MapMode != MM_ISOTROPIC && MapMode != MM_ANISOTROPIC))
  {
    dc->w.MapMode = MapMode;

    switch (MapMode)
    {
      case MM_TEXT:
        dc->wndExtX = 1;
        dc->wndExtY = 1;
        dc->vportExtX = 1;
        dc->vportExtY = 1;
        break;

      case MM_LOMETRIC:
      case MM_ISOTROPIC:
        dc->wndExtX = dc->GDIInfo->ulHorzSize * 10;
        dc->wndExtY = dc->GDIInfo->ulVertSize * 10;
        dc->vportExtX = dc->GDIInfo->ulHorzRes;
        dc->vportExtY = -dc->GDIInfo->ulVertRes;
        break;

      case MM_HIMETRIC:
        dc->wndExtX = dc->GDIInfo->ulHorzSize * 100;
        dc->wndExtY = dc->GDIInfo->ulVertSize * 100;
        dc->vportExtX = dc->GDIInfo->ulHorzRes;
        dc->vportExtY = -dc->GDIInfo->ulVertRes;
        break;

      case MM_LOENGLISH:
        dc->wndExtX = EngMulDiv(1000, dc->GDIInfo->ulHorzSize, 254);
        dc->wndExtY = EngMulDiv(1000, dc->GDIInfo->ulVertSize, 254);
        dc->vportExtX = dc->GDIInfo->ulHorzRes;
        dc->vportExtY = -dc->GDIInfo->ulVertRes;
        break;

      case MM_HIENGLISH:
        dc->wndExtX = EngMulDiv(10000, dc->GDIInfo->ulHorzSize, 254);
        dc->wndExtY = EngMulDiv(10000, dc->GDIInfo->ulVertSize, 254);
        dc->vportExtX = dc->GDIInfo->ulHorzRes;
        dc->vportExtY = -dc->GDIInfo->ulVertRes;
        break;

      case MM_TWIPS:
        dc->wndExtX = EngMulDiv(14400, dc->GDIInfo->ulHorzSize, 254);
        dc->wndExtY = EngMulDiv(14400, dc->GDIInfo->ulVertSize, 254);
        dc->vportExtX = dc->GDIInfo->ulHorzRes;
        dc->vportExtY = -dc->GDIInfo->ulVertRes;
        break;

      case MM_ANISOTROPIC:
        break;
    }

    DC_UpdateXforms(dc);
  }

  DC_UnlockDc(dc);

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
	DC_UnlockDc(dc);
	return FALSE;

      case MM_ISOTROPIC:
	// Here we should (probably) check that SetWindowExtEx *really* has
	// been called
	break;
    }

  if (Size)
    {
      NTSTATUS Status = STATUS_SUCCESS;

      _SEH_TRY
      {
         ProbeForWrite(Size,
                       sizeof(SIZE),
                       1);
         Size->cx = dc->vportExtX;
         Size->cy = dc->vportExtY;

		 dc->vportExtX = XExtent;
         dc->vportExtY = YExtent;

         if (dc->w.MapMode == MM_ISOTROPIC)
             IntFixIsotropicMapping(dc);
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  
  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

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
      NTSTATUS Status = STATUS_SUCCESS;
      
      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = dc->vportOrgX;
         Point->y = dc->vportOrgY;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  dc->vportOrgX = X;
  dc->vportOrgY = Y;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

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
	DC_UnlockDc(dc);
	return FALSE;
    }

  if (Size)
    {
      NTSTATUS Status = STATUS_SUCCESS;
      
      _SEH_TRY
      {
         ProbeForWrite(Size,
                       sizeof(SIZE),
                       1);
         Size->cx = dc->wndExtX;
         Size->cy = dc->wndExtY;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  dc->wndExtX = XExtent;
  dc->wndExtY = YExtent;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

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
      NTSTATUS Status = STATUS_SUCCESS;
      
      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = dc->wndOrgX;
         Point->y = dc->wndOrgY;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  dc->wndOrgX = X;
  dc->wndOrgY = Y;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetWorldTransform(HDC  hDC,
                      CONST LPXFORM  XForm)
{
  PDC  dc;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc (hDC);
  if ( !dc )
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return  FALSE;
  }

  if (!XForm)
  {
    DC_UnlockDc(dc);
    /* Win doesn't set LastError */
    return  FALSE;
  }

  /* Check that graphics mode is GM_ADVANCED */
  if ( dc->w.GraphicsMode != GM_ADVANCED )
  {
    DC_UnlockDc(dc);
    return  FALSE;
  }

  _SEH_TRY
  {
    ProbeForRead(XForm,
                 sizeof(XFORM),
                 1);
    dc->w.xformWorld2Wnd = *XForm;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  if(!NT_SUCCESS(Status))
  {
    DC_UnlockDc(dc);
    return FALSE;
  }

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);
  return  TRUE;
}

/* EOF */
