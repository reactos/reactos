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
/* $Id: objects.h,v 1.26 2004/01/17 15:20:25 navaraf Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Internal Objects
 * FILE:              subsys/win32k/eng/objects.h
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 21/8/1999: Created
 */
#ifndef __ENG_OBJECTS_H
#define __ENG_OBJECTS_H

#include <freetype/freetype.h>

/* Structure of internal gdi objects that win32k manages for ddi engine:
   |---------------------------------|
   |           EngObj                |
   |---------------------------------|
   |         Public part             |
   |      accessed from engine       |
   |---------------------------------|
   |        Private part             |
   |       managed by gdi            |
   |_________________________________|

---------------------------------------------------------------------------*/

typedef struct _ENGOBJ {
	ULONG  hObj;
	ULONG  InternalSize;
	ULONG  UserSize;
}ENGOBJ, *PENGOBJ;



typedef struct _BRUSHGDI {
  ENGOBJ 		Header;
  BRUSHOBJ	BrushObj;
} BRUSHGDI;

typedef struct _CLIPGDI {
  ENGOBJ 		Header;
  CLIPOBJ		ClipObj;
  /* ei what were these for?
  ULONG NumRegionRects;
  ULONG NumIntersectRects;
  RECTL *RegionRects;
  RECTL *IntersectRects;
  */
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

} FLOATGDI;

typedef struct _FONTGDI {
  ENGOBJ 		Header;
  FONTOBJ		FontObj;

  LPCWSTR Filename;
  FT_Face face;
  TEXTMETRICW TextMetric;
} FONTGDI, *PFONTGDI;

typedef struct _PATHGDI {
  ENGOBJ 		Header;
  PATHOBJ		PathObj;
} PATHGDI;

typedef struct _STRGDI {
  ENGOBJ 		Header;
  STROBJ		StrObj;
} STRGDI;

typedef BOOL STDCALL (*PFN_BitBlt)(SURFOBJ *, SURFOBJ *, SURFOBJ *, CLIPOBJ *,
                           XLATEOBJ *, RECTL *, POINTL *, POINTL *,
                           BRUSHOBJ *, POINTL *, ROP4);

typedef BOOL STDCALL (*PFN_TransparentBlt)(SURFOBJ *, SURFOBJ *, CLIPOBJ *, XLATEOBJ *, RECTL *, RECTL *, ULONG, ULONG);

typedef BOOL STDCALL (*PFN_StretchBlt)(SURFOBJ *, SURFOBJ *, SURFOBJ *, CLIPOBJ *,
                               XLATEOBJ *, COLORADJUSTMENT *, POINTL *,
                               RECTL *, RECTL *, PPOINT, ULONG);

typedef BOOL STDCALL (*PFN_TextOut)(SURFOBJ *, STROBJ *, FONTOBJ *, CLIPOBJ *,
                            RECTL *, RECTL *, BRUSHOBJ *, BRUSHOBJ *,
                            POINTL *, MIX);

typedef BOOL STDCALL (*PFN_Paint)(SURFOBJ *, CLIPOBJ *, BRUSHOBJ *, POINTL *, MIX);

typedef BOOL STDCALL (*PFN_StrokePath)(SURFOBJ *, PATHOBJ *, CLIPOBJ *, XFORMOBJ *,
                               BRUSHOBJ *, POINTL *, LINEATTRS *, MIX);

typedef BOOL STDCALL (*PFN_FillPath)(SURFOBJ *, PATHOBJ *, CLIPOBJ *, BRUSHOBJ *,
                             POINTL *, MIX, ULONG);

typedef BOOL STDCALL (*PFN_StrokeAndFillPath)(SURFOBJ *, PATHOBJ *, CLIPOBJ *,
                XFORMOBJ *, BRUSHOBJ *, LINEATTRS *, BRUSHOBJ *,
                POINTL *, MIX, ULONG);

typedef BOOL STDCALL (*PFN_LineTo)(SURFOBJ *, CLIPOBJ *, BRUSHOBJ *,
                           LONG, LONG, LONG, LONG, RECTL *, MIX);

typedef BOOL STDCALL (*PFN_CopyBits)(SURFOBJ *, SURFOBJ *, CLIPOBJ *,
                             XLATEOBJ *, RECTL *, POINTL *);

typedef VOID STDCALL (*PFN_Synchronize)(DHPDEV, RECTL *);

typedef VOID STDCALL (*PFN_MovePointer)(SURFOBJ *, LONG, LONG, RECTL *);

typedef ULONG STDCALL (*PFN_SetPointerShape)(SURFOBJ *, SURFOBJ *, SURFOBJ *, XLATEOBJ *,
			    LONG, LONG, LONG, LONG, RECTL *, FLONG);

typedef HBITMAP STDCALL (*PFN_CreateDeviceBitmap)(DHPDEV, SIZEL, ULONG);

typedef BOOL STDCALL (*PFN_SetPalette)(DHPDEV, PALOBJ*, ULONG, ULONG, ULONG);

/* Forward declare (circular reference) */
typedef struct _SURFGDI *PSURFGDI;

typedef VOID    (*PFN_DIB_PutPixel)(SURFOBJ *, LONG, LONG, ULONG);
typedef ULONG   (*PFN_DIB_GetPixel)(SURFOBJ *, LONG, LONG);
typedef VOID    (*PFN_DIB_HLine)   (SURFOBJ *, LONG, LONG, LONG, ULONG);
typedef VOID    (*PFN_DIB_VLine)   (SURFOBJ *, LONG, LONG, LONG, ULONG);
typedef BOOLEAN (*PFN_DIB_BitBlt)  (SURFOBJ * DestSurf, SURFOBJ * SourceSurf,
                                    PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                    RECTL *  DestRect, POINTL *  SourcePoint,
				                    BRUSHOBJ *BrushObj, POINTL * BrushOrigin,
                                    XLATEOBJ *ColorTranslation, ULONG Rop4);
typedef BOOLEAN (*PFN_DIB_StretchBlt)  (SURFOBJ * DestSurf, SURFOBJ * SourceSurf,
                                    PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                    RECTL *  DestRect, RECTL *  SourceRect,
				                    POINTL *MaskOrigin, POINTL * BrushOrigin,
                                    XLATEOBJ *ColorTranslation, ULONG Mode);

typedef struct _SURFGDI {
  ENGOBJ 		Header;
  SURFOBJ		SurfObj;

  INT BitsPerPixel;

  /* Driver functions */
  PFN_BitBlt BitBlt;
  PFN_TransparentBlt TransparentBlt;
  PFN_StretchBlt StretchBlt;
  PFN_TextOut TextOut;
  PFN_Paint Paint;
  PFN_StrokePath StrokePath;
  PFN_FillPath FillPath;
  PFN_StrokeAndFillPath StrokeAndFillPath;
  PFN_LineTo LineTo;
  PFN_CopyBits CopyBits;
  PFN_Synchronize Synchronize;
  BOOL SynchronizeAccess;
  PFN_CreateDeviceBitmap CreateDeviceBitmap;
  PFN_SetPalette SetPalette;
  PFN_MovePointer MovePointer;
  PFN_SetPointerShape SetPointerShape;

  /* DIB functions */
  PFN_DIB_PutPixel   DIB_PutPixel;
  PFN_DIB_GetPixel   DIB_GetPixel;
  PFN_DIB_HLine      DIB_HLine;
  PFN_DIB_VLine      DIB_VLine;
  PFN_DIB_BitBlt     DIB_BitBlt;
  PFN_DIB_StretchBlt DIB_StretchBlt;

  /* misc */
  ULONG       PointerStatus;
  PFAST_MUTEX DriverLock;
} SURFGDI;

typedef struct _XFORMGDI {
  ENGOBJ 		Header;
  /* XFORMOBJ has no public members */
} XFORMGDI;

typedef struct _XLATEGDI {
  ENGOBJ 		Header;
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
//    struct {            /* For Table Translations */
      ULONG *translationTable;
//    };
//    struct {            /* For Color -> Mono Translations */
      ULONG BackgroundColor;
//    };
//  };
} XLATEGDI;

// List of GDI objects
// FIXME: Make more dynamic

#define MAX_GDI_BRUSHES      255
#define MAX_GDI_CLIPS        255
#define MAX_GDI_DRVFUNCTIONS  16
#define MAX_GDI_FLOATS       255
#define MAX_GDI_FONTS        255
#define MAX_GDI_PALS         255
#define MAX_GDI_PATHS        255
#define MAX_GDI_STRS         255
#define MAX_GDI_SURFS        255
#define MAX_GDI_XFORMS       255
#define MAX_GDI_XLATES       255

#endif //__ENG_OBJECTS_H
