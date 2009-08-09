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
/* $Id$
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

typedef struct _DRIVERGDI {
  DRIVEROBJ    DriverObj;
  LIST_ENTRY   ListEntry;
  FAST_MUTEX   Lock;
} DRIVERGDI, *PDRIVERGDI;

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

typedef BOOL (APIENTRY *PFN_BitBlt)(SURFOBJ *, SURFOBJ *, SURFOBJ *, CLIPOBJ *,
                           XLATEOBJ *, RECTL *, POINTL *, POINTL *,
                           BRUSHOBJ *, POINTL *, ROP4);

typedef BOOL (APIENTRY *PFN_TransparentBlt)(SURFOBJ *, SURFOBJ *, CLIPOBJ *, XLATEOBJ *, RECTL *, RECTL *, ULONG, ULONG);

typedef BOOL (APIENTRY *PFN_StretchBlt)(SURFOBJ *, SURFOBJ *, SURFOBJ *, CLIPOBJ *,
                               XLATEOBJ *, COLORADJUSTMENT *, POINTL *,
                               RECTL *, RECTL *, PPOINT, ULONG);

typedef BOOL (APIENTRY *PFN_TextOut)(SURFOBJ *, STROBJ *, FONTOBJ *, CLIPOBJ *,
                            RECTL *, RECTL *, BRUSHOBJ *, BRUSHOBJ *,
                            POINTL *, MIX);

typedef BOOL (APIENTRY *PFN_Paint)(SURFOBJ *, CLIPOBJ *, BRUSHOBJ *, POINTL *, MIX);

typedef BOOL (APIENTRY *PFN_StrokePath)(SURFOBJ *, PATHOBJ *, CLIPOBJ *, XFORMOBJ *,
                               BRUSHOBJ *, POINTL *, LINEATTRS *, MIX);

typedef BOOL (APIENTRY *PFN_FillPath)(SURFOBJ *, PATHOBJ *, CLIPOBJ *, BRUSHOBJ *,
                             POINTL *, MIX, ULONG);

typedef BOOL (APIENTRY *PFN_StrokeAndFillPath)(SURFOBJ *, PATHOBJ *, CLIPOBJ *,
                XFORMOBJ *, BRUSHOBJ *, LINEATTRS *, BRUSHOBJ *,
                POINTL *, MIX, ULONG);

typedef BOOL (APIENTRY *PFN_LineTo)(SURFOBJ *, CLIPOBJ *, BRUSHOBJ *,
                           LONG, LONG, LONG, LONG, RECTL *, MIX);

typedef BOOL (APIENTRY *PFN_CopyBits)(SURFOBJ *, SURFOBJ *, CLIPOBJ *,
                             XLATEOBJ *, RECTL *, POINTL *);

typedef VOID (APIENTRY *PFN_Synchronize)(DHPDEV, RECTL *);

typedef VOID (APIENTRY *PFN_MovePointer)(SURFOBJ *, LONG, LONG, RECTL *);

typedef ULONG (APIENTRY *PFN_SetPointerShape)(SURFOBJ *, SURFOBJ *, SURFOBJ *, XLATEOBJ *,
			    LONG, LONG, LONG, LONG, RECTL *, FLONG);

typedef HBITMAP (APIENTRY *PFN_CreateDeviceBitmap)(DHPDEV, SIZEL, ULONG);

typedef BOOL (APIENTRY *PFN_SetPalette)(DHPDEV, PALOBJ*, ULONG, ULONG, ULONG);

typedef BOOL (APIENTRY *PFN_GradientFill)(SURFOBJ*, CLIPOBJ*, XLATEOBJ*, TRIVERTEX*, ULONG, PVOID, ULONG, RECTL*, POINTL*, ULONG);

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
