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
/* $Id: brush.c,v 1.31 2004/01/15 21:03:05 gvg Exp $
 */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>
#include <internal/safe.h>
//#include <win32k/debug.h>
#include <include/object.h>
#include <include/inteng.h>
#include <include/error.h>

#define NDEBUG
#include <win32k/debug1.h>

HBRUSH FASTCALL
IntGdiCreateBrushIndirect(PLOGBRUSH lb)
{
  PBRUSHOBJ brushPtr;
  HBRUSH    hBrush;
  
  hBrush = BRUSHOBJ_AllocBrush();
  if (hBrush == NULL)
  {
    return 0;
  }

  brushPtr = BRUSHOBJ_LockBrush (hBrush);
/* FIXME: Occurs! FiN */
/*  ASSERT( brushPtr ); *///I want to know if this ever occurs

  if( brushPtr ){
  	brushPtr->iSolidColor = lb->lbColor;
  	brushPtr->logbrush.lbStyle = lb->lbStyle;
  	brushPtr->logbrush.lbColor = lb->lbColor;
  	brushPtr->logbrush.lbHatch = lb->lbHatch;

  	BRUSHOBJ_UnlockBrush( hBrush );
  	return  hBrush;
  }
  return NULL;
}

HBRUSH FASTCALL
IntGdiCreateDIBPatternBrush(HGLOBAL  hDIBPacked,
                            UINT  ColorSpec)
{
  return NULL;
#if 0
  LOGBRUSH  logbrush;
  PBITMAPINFO  info, newInfo;
  INT  size;

  DPRINT("%04x\n", hbitmap );

  logbrush.lbStyle = BS_DIBPATTERN;
  logbrush.lbColor = coloruse;
  logbrush.lbHatch = 0;

  /* Make a copy of the bitmap */
  if (!(info = (BITMAPINFO *)GlobalLock( hbitmap )))
  {
    return 0;
  }


  if (info->bmiHeader.biCompression) size = info->bmiHeader.biSizeImage;
  else
    size = DIB_GetDIBImageBytes(info->bmiHeader.biWidth, info->bmiHeader.biHeight, info->bmiHeader.biBitCount);
  size += DIB_BitmapInfoSize(info, coloruse);

  if (!(logbrush.lbHatch = (INT)GlobalAlloc16( GMEM_MOVEABLE, size )))
  {
    GlobalUnlock16( hbitmap );
    return 0;
  }
  newInfo = (BITMAPINFO *) GlobalLock16((HGLOBAL16)logbrush.lbHatch);
  memcpy(newInfo, info, size);
  GlobalUnlock16((HGLOBAL16)logbrush.lbHatch);
  GlobalUnlock(hbitmap);
  return IntGdiCreateBrushIndirect(&logbrush);
#endif
}

HBRUSH FASTCALL
IntGdiCreateDIBPatternBrushPt(CONST VOID  *PackedDIB,
                              UINT  Usage)
{
  INT  size;
  LOGBRUSH  logbrush;
  PBITMAPINFO  info;
  PBITMAPINFO  newInfo;

  info = (BITMAPINFO *) PackedDIB;
  if (info == NULL)
  {
    return 0;
  }
  DPRINT ("%p %ldx%ld %dbpp\n",
          info,
          info->bmiHeader.biWidth,
          info->bmiHeader.biHeight,
          info->bmiHeader.biBitCount);

  logbrush.lbStyle = BS_DIBPATTERN;
  logbrush.lbColor = Usage;
  logbrush.lbHatch = 0;

  /* Make a copy of the bitmap */

  if (info->bmiHeader.biCompression)
  {
    size = info->bmiHeader.biSizeImage;
  }
  else
    {
    size = DIB_GetDIBImageBytes (info->bmiHeader.biWidth, info->bmiHeader.biHeight, info->bmiHeader.biBitCount);
  }
  size += DIB_BitmapInfoSize (info, Usage);

  logbrush.lbHatch = (LONG) GDIOBJ_AllocObj(size, GDI_OBJECT_TYPE_DONTCARE, NULL);
  if (logbrush.lbHatch == 0)
  {
    return 0;
  }
  newInfo = (PBITMAPINFO) GDIOBJ_LockObj ((HGDIOBJ) logbrush.lbHatch, GDI_OBJECT_TYPE_DONTCARE);
  ASSERT(newInfo);
  memcpy(newInfo, info, size);
  GDIOBJ_UnlockObj((HGDIOBJ) logbrush.lbHatch, GDI_OBJECT_TYPE_DONTCARE);

  return  IntGdiCreateBrushIndirect (&logbrush);
}

BOOL FASTCALL
IntPatBlt(DC *dc,
          INT  XLeft,
          INT  YLeft,
          INT  Width,
          INT  Height,
          DWORD  ROP,
          PBRUSHOBJ BrushObj)
{
  RECT DestRect;
  PSURFOBJ SurfObj;
  BOOL ret;

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  if (NULL == SurfObj)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  assert(BrushObj);
  if (BrushObj->logbrush.lbStyle != BS_NULL)
    {
      if (Width > 0)
	{
	  DestRect.left = XLeft + dc->w.DCOrgX;
	  DestRect.right = XLeft + Width + dc->w.DCOrgX;
	}
      else
	{
	  DestRect.left = XLeft + Width + 1 + dc->w.DCOrgX;
	  DestRect.right = XLeft + dc->w.DCOrgX + 1;
	}
      if (Height > 0)
	{
	  DestRect.top = YLeft + dc->w.DCOrgY;
	  DestRect.bottom = YLeft + Height + dc->w.DCOrgY;
	}
      else
	{
	  DestRect.top = YLeft + Height + dc->w.DCOrgY + 1;
	  DestRect.bottom = YLeft + dc->w.DCOrgY + 1;
	}
      ret = IntEngBitBlt(SurfObj,
		         NULL,
		         NULL,
		         dc->CombinedClip,
		         NULL,
		         &DestRect,
		         NULL,
		         NULL,
		         BrushObj,
		         NULL,
		         ROP);
    }

  return ret;
}

BOOL FASTCALL
IntGdiPolyPatBlt(HDC hDC,
                 DWORD dwRop,
                 PPATRECT pRects,
                 int cRects,
                 ULONG Reserved)
{
	int i;
	PPATRECT r;
	PBRUSHOBJ BrushObj;
	DC *dc;
	
    dc = DC_LockDc(hDC);
	if (dc == NULL)
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return(FALSE);
	}
	
	for (r = pRects, i = 0; i < cRects; i++)
	{
		BrushObj = BRUSHOBJ_LockBrush(r->hBrush);
		IntPatBlt(dc,r->r.left,r->r.top,r->r.right,r->r.bottom,dwRop,BrushObj);
		BRUSHOBJ_UnlockBrush(r->hBrush);
		r++;
	}
	DC_UnlockDc( hDC );
	
	return(TRUE);
}

/******************************************************************************/

HBRUSH STDCALL NtGdiCreateBrushIndirect(CONST LOGBRUSH  *lb)
{
  LOGBRUSH  Safelb;
  NTSTATUS  Status;
  
  Status = MmCopyFromCaller(&Safelb, lb, sizeof(LOGBRUSH));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }
  
  return IntGdiCreateBrushIndirect(&Safelb);
}

HBRUSH STDCALL NtGdiCreateDIBPatternBrush(HGLOBAL  hDIBPacked,
                                  UINT  ColorSpec)
{
  /* IntGdiCreateDIBPatternBrush() */
  UNIMPLEMENTED;
}

HBRUSH STDCALL NtGdiCreateDIBPatternBrushPt(CONST VOID  *PackedDIB,
                                    UINT  Usage)
{
  /* FIXME - copy PackedDIB memory first! */
  return IntGdiCreateDIBPatternBrushPt(PackedDIB, Usage);
}

HBRUSH STDCALL NtGdiCreateHatchBrush(INT  Style,
                             COLORREF  Color)
{
  LOGBRUSH  logbrush;

  DPRINT("%d %06lx\n", Style, Color);

  if (Style < 0 || Style >= NB_HATCH_STYLES)
  {
    return 0;
  }
  logbrush.lbStyle = BS_HATCHED;
  logbrush.lbColor = Color;
  logbrush.lbHatch = Style;

  return  IntGdiCreateBrushIndirect (&logbrush);
}

HBRUSH STDCALL NtGdiCreatePatternBrush(HBITMAP  hBitmap)
{
  LOGBRUSH  logbrush = { BS_PATTERN, 0, 0 };

  DPRINT ("%04x\n", hBitmap);
  logbrush.lbHatch = (INT) BITMAPOBJ_CopyBitmap (hBitmap);
  if(!logbrush.lbHatch)
  {
    return 0;
  }
  else
  {
    return IntGdiCreateBrushIndirect( &logbrush );
  }
}

HBRUSH STDCALL NtGdiCreateSolidBrush(COLORREF  Color)
{
  LOGBRUSH logbrush;

  logbrush.lbStyle = BS_SOLID;
  logbrush.lbColor = Color;
  logbrush.lbHatch = 0;

  return IntGdiCreateBrushIndirect(&logbrush);
}

BOOL STDCALL NtGdiFixBrushOrgEx(VOID)
{
  return FALSE;
}

BOOL STDCALL NtGdiPolyPatBlt(HDC hDC,
			DWORD dwRop,
			PPATRECT pRects,
			int cRects,
			ULONG Reserved)
{
	PPATRECT rb;
	NTSTATUS Status;
	BOOL Ret;
    
	if(cRects > 0)
	{
	  rb = ExAllocatePool(PagedPool, sizeof(PATRECT) * cRects);
	  if(!rb)
	  {
	    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
	    return FALSE;
	  }
      Status = MmCopyFromCaller(rb, pRects, sizeof(PATRECT) * cRects);
      if(!NT_SUCCESS(Status))
      {
        ExFreePool(rb);
        SetLastNtError(Status);
        return FALSE;
      }
    }
    
    Ret = IntGdiPolyPatBlt(hDC, dwRop, pRects, cRects, Reserved);
	
	if(cRects > 0)
	  ExFreePool(rb);
	return Ret;
}

BOOL STDCALL NtGdiPatBlt(HDC  hDC,
			INT  XLeft,
			INT  YLeft,
			INT  Width,
			INT  Height,
			DWORD  ROP)
{
  PBRUSHOBJ BrushObj;
  DC *dc = DC_LockDc(hDC);
  BOOL ret;

  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  BrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
  if (NULL == BrushObj)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      DC_UnlockDc(hDC);
      return FALSE;
    }

  ret = IntPatBlt(dc,XLeft,YLeft,Width,Height,ROP,BrushObj);

  BRUSHOBJ_UnlockBrush(dc->w.hBrush);
  DC_UnlockDc(hDC);
  return ret;
}

BOOL STDCALL NtGdiSetBrushOrgEx(HDC  hDC,
                        INT  XOrg,
                        INT  YOrg,
                        LPPOINT  Point)
{
  UNIMPLEMENTED;
}
/* EOF */
