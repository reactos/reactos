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
/* $Id: objconv.c,v 1.17 2004/04/09 20:03:20 navaraf Exp $ */

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

//#define NDEBUG
#include <win32k/debug1.h>

HBITMAP FASTCALL BitmapToSurf(PBITMAPOBJ BitmapObj, HDEV GDIDevice)
{
  HBITMAP BitmapHandle;
  SIZE Size;

  ASSERT ( BitmapObj );
  Size.cx = BitmapObj->bitmap.bmWidth;
  Size.cy = BitmapObj->bitmap.bmHeight;
  if (NULL != BitmapObj->dib)
    {
    BitmapHandle = EngCreateBitmap(Size, BitmapObj->dib->dsBm.bmWidthBytes,
                                   BitmapFormat(BitmapObj->dib->dsBm.bmBitsPixel, BI_RGB),
                                   0, BitmapObj->dib->dsBm.bmBits);
    }
  else
    {
    BitmapHandle = EngCreateBitmap(Size, BitmapObj->bitmap.bmWidthBytes,
                                   BitmapFormat(BitmapObj->bitmap.bmBitsPixel, BI_RGB),
                                   0, BitmapObj->bitmap.bmBits);
    }
  if (NULL != BitmapHandle && NULL != GDIDevice)
    {
      EngAssociateSurface((HSURF)BitmapHandle, GDIDevice, 0);
    }

  return BitmapHandle;
}

/* EOF */
