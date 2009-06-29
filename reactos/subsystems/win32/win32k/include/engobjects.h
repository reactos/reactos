/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Internal Objects
 * FILE:              subsystem/win32/win32k/eng/objects.h
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 21/8/1999: Created
 */
#ifndef __ENG_OBJECTS_H
#define __ENG_OBJECTS_H

#include <ft2build.h>
#include <freetype/freetype.h>

/* Structure of internal gdi objects that win32k manages for ddi engine:
   |---------------------------------|
   |         Public part             |
   |      accessed from engine       |
   |---------------------------------|
   |        Private part             |
   |       managed by gdi            |
   |_________________________________|

---------------------------------------------------------------------------*/

typedef struct _CLIPGDI {
  CLIPOBJ ClipObj;
  ULONG EnumPos;
  ULONG EnumOrder;
  ULONG EnumMax;
  ENUMRECTS EnumRects;
} CLIPGDI, *PCLIPGDI;

/*ei What is this for? */
typedef struct _DRVFUNCTIONSGDI {
  HDEV  hdev;
  DRVFN Functions[INDEX_LAST];
} DRVFUNCTIONSGDI;

typedef struct _FLOATGDI {
  ULONG Dummy;
} FLOATGDI;


#define FDM_TYPE_TEXT_METRIC  0x80000000

typedef struct _FONTGDI {
  FONTOBJ     FontObj;
  ULONG       iUnique;
  FLONG       flType;
  union{
  DHPDEV      dhpdev;
  FT_Face     face;
  };
  FLONG       flRealizedType;

  LONG        lMaxNegA;
  LONG        lMaxNegC;
  LONG        lMinWidthD;

  TEXTMETRICW TextMetric;
  LPWSTR      Filename;
  BYTE        Underline;
  BYTE        StrikeOut;
} FONTGDI, *PFONTGDI;

typedef struct _PATHGDI {
  PATHOBJ PathObj;
} PATHGDI;

typedef struct _WNDGDI {
  WNDOBJ            WndObj;
  LIST_ENTRY        ListEntry;
  HWND              Hwnd;
  CLIPOBJ           *ClientClipObj;
  WNDOBJCHANGEPROC  ChangeProc;
  FLONG             Flags;
  int               PixelFormat;
} WNDGDI, *PWNDGDI;

typedef struct _XFORMGDI {
  ULONG Dummy;
  /* XFORMOBJ has no public members */
} XFORMGDI;

typedef struct _XLATEGDI {
  XLATEOBJ		XlateObj;
  HPALETTE DestPal;
  HPALETTE SourcePal;
  BOOL UseShiftAndMask;

//  union {
//    struct {            /* For Shift Translations */
      ULONG RedMask;
      ULONG GreenMask;
      ULONG BlueMask;
      INT RedShift;
      INT GreenShift;
      INT BlueShift;
//    };
//    struct {            /* For Color -> Mono Translations */
      ULONG BackgroundColor;
//    };
//  };
} XLATEGDI;

/* as the *OBJ structures are located at the beginning of the *GDI structures
   we can simply typecast the pointer */
#define ObjToGDI(ClipObj, Type) (Type##GDI *)(ClipObj)
#define GDIToObj(ClipGDI, Type) (Type##OBJ *)(ClipGDI)


#endif //__ENG_OBJECTS_H
