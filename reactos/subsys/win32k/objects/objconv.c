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
/* $Id: objconv.c,v 1.9 2003/05/18 17:16:18 ea Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>

/* FIXME: Surely we should just have one include file that includes all of these.. */
#include <win32k/bitmaps.h>
#include <win32k/coord.h>
#include <win32k/driver.h>
#include <win32k/dc.h>
#include <win32k/print.h>
#include <win32k/region.h>
#include <win32k/gdiobj.h>
#include <win32k/pen.h>
#include "../eng/objects.h"
#include <include/object.h>
#include <include/surface.h>

#define NDEBUG
#include <win32k/debug1.h>


PBRUSHOBJ FASTCALL PenToBrushObj(PDC dc, PENOBJ *pen)
{
  BRUSHOBJ *BrushObj;
  XLATEOBJ *RGBtoVGA16;

  ASSERT( pen );

  BrushObj = ExAllocatePool(NonPagedPool, sizeof(BRUSHOBJ));
  BrushObj->iSolidColor = pen->logpen.lopnColor;

  return BrushObj;
}

HBITMAP FASTCALL BitmapToSurf(PBITMAPOBJ BitmapObj)
{
  HBITMAP BitmapHandle;

  if (NULL != BitmapObj->dib)
    {
    BitmapHandle = EngCreateBitmap(BitmapObj->size, BitmapObj->dib->dsBm.bmWidthBytes,
                                   BitmapFormat(BitmapObj->dib->dsBm.bmBitsPixel, BI_RGB),
                                   0, BitmapObj->dib->dsBm.bmBits);
    }
  else
    {
    BitmapHandle = EngCreateBitmap(BitmapObj->size, BitmapObj->bitmap.bmWidthBytes,
                                   BitmapFormat(BitmapObj->bitmap.bmBitsPixel, BI_RGB),
                                   0, BitmapObj->bitmap.bmBits);
    }

  return BitmapHandle;
}
/* EOF */
