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
/* $Id: brush.c,v 1.21 2003/06/28 08:39:18 gvg Exp $
 */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>
//#include <win32k/debug.h>
#include <include/object.h>
#include <include/inteng.h>
#include <include/error.h>

#define NDEBUG
#include <win32k/debug1.h>

HBRUSH STDCALL W32kCreateBrushIndirect(CONST LOGBRUSH  *lb)
{
  PBRUSHOBJ  brushPtr;
  HBRUSH  hBrush;

  hBrush = BRUSHOBJ_AllocBrush();
  if (hBrush == NULL)
  {
    return 0;
  }

  brushPtr = BRUSHOBJ_LockBrush (hBrush);
  ASSERT( brushPtr ); //I want to know if this ever occurs

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

HBRUSH STDCALL W32kCreateDIBPatternBrush(HGLOBAL  hDIBPacked,
                                  UINT  ColorSpec)
{
  UNIMPLEMENTED;
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
  return W32kCreateBrushIndirect(&logbrush);
#endif
}

HBRUSH STDCALL W32kCreateDIBPatternBrushPt(CONST VOID  *PackedDIB,
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

  logbrush.lbHatch = (LONG) GDIOBJ_AllocObj(size, GO_MAGIC_DONTCARE);
  if (logbrush.lbHatch == 0)
  {
    return 0;
  }
  newInfo = (PBITMAPINFO) GDIOBJ_LockObj ((HGDIOBJ) logbrush.lbHatch, GO_MAGIC_DONTCARE);
  ASSERT(newInfo);
  memcpy(newInfo, info, size);
  GDIOBJ_UnlockObj( (HGDIOBJ) logbrush.lbHatch, GO_MAGIC_DONTCARE );

  return  W32kCreateBrushIndirect (&logbrush);
}

HBRUSH STDCALL W32kCreateHatchBrush(INT  Style,
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

  return  W32kCreateBrushIndirect (&logbrush);
}

HBRUSH STDCALL W32kCreatePatternBrush(HBITMAP  hBitmap)
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
    return W32kCreateBrushIndirect( &logbrush );
  }
}

HBRUSH STDCALL W32kCreateSolidBrush(COLORREF  Color)
{
  LOGBRUSH logbrush;

  logbrush.lbStyle = BS_SOLID;
  logbrush.lbColor = Color;
  logbrush.lbHatch = 0;

  return W32kCreateBrushIndirect(&logbrush);
}

BOOL STDCALL W32kFixBrushOrgEx(VOID)
{
  return FALSE;
}

BOOL STDCALL W32kPatBlt(HDC  hDC,
			INT  XLeft,
			INT  YLeft,
			INT  Width,
			INT  Height,
			DWORD  ROP)
{
  RECT DestRect;
  PBRUSHOBJ BrushObj;
  PSURFOBJ SurfObj;
  DC *dc = DC_HandleToPtr(hDC);
  BOOL ret;

  if (dc == NULL)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return(FALSE);
    }

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);

  BrushObj = (BRUSHOBJ*) GDIOBJ_LockObj(dc->w.hBrush, GO_BRUSH_MAGIC);
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
	  DestRect.left = XLeft + Width + dc->w.DCOrgX;
	  DestRect.right = XLeft + dc->w.DCOrgX;
	}
      if (Height > 0)
	{
	  DestRect.top = YLeft + dc->w.DCOrgY;
	  DestRect.bottom = YLeft + Height + dc->w.DCOrgY;
	}
      else
	{
	  DestRect.top = YLeft + Height + dc->w.DCOrgY;
	  DestRect.bottom = YLeft + dc->w.DCOrgY;
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
		         PATCOPY);
    }
  GDIOBJ_UnlockObj( dc->w.hBrush, GO_BRUSH_MAGIC );
  DC_ReleasePtr( hDC );
  return(ret);
}

BOOL STDCALL W32kSetBrushOrgEx(HDC  hDC,
                        INT  XOrg,
                        INT  YOrg,
                        LPPOINT  Point)
{
  UNIMPLEMENTED;
}
/* EOF */
