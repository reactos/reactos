/*
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
  ULONG NumRegionRects;
  ULONG NumIntersectRects;
  RECTL *RegionRects;
  RECTL *IntersectRects;

  ULONG EnumPos;
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
  TEXTMETRIC TextMetric;
} FONTGDI, *PFONTGDI;

typedef struct _PALGDI {
  ENGOBJ 		Header;
  PALOBJ		PalObj;

  ULONG Mode; // PAL_INDEXED, PAL_BITFIELDS, PAL_RGB, PAL_BGR
  ULONG NumColors;
  ULONG *IndexedColors;
  ULONG RedMask;
  ULONG GreenMask;
  ULONG BlueMask;
} PALGDI, *PPALGDI;

typedef struct _PATHGDI {
  ENGOBJ 		Header;
  PATHOBJ		PathObj;
} PATHGDI;

/*ei Fixme! Fix STROBJ */
typedef struct _STRGDI {
  ENGOBJ 		Header;
  //STROBJ		StrObj;
} STRGDI;

typedef BOOL STDCALL (*PFN_BitBlt)(PSURFOBJ, PSURFOBJ, PSURFOBJ, PCLIPOBJ,
                           PXLATEOBJ, PRECTL, PPOINTL, PPOINTL,
                           PBRUSHOBJ, PPOINTL, ROP4);

typedef BOOL STDCALL (*PFN_TransparentBlt)(PSURFOBJ, PSURFOBJ, PCLIPOBJ, PXLATEOBJ, PRECTL, PRECTL, ULONG, ULONG);

typedef BOOL STDCALL (*PFN_StretchBlt)(PSURFOBJ, PSURFOBJ, PSURFOBJ, PCLIPOBJ,
                               PXLATEOBJ, PCOLORADJUSTMENT, PPOINTL,
                               PRECTL, PRECTL, PPOINT, ULONG);

typedef BOOL STDCALL (*PFN_TextOut)(PSURFOBJ, PSTROBJ, PFONTOBJ, PCLIPOBJ,
                            PRECTL, PRECTL, PBRUSHOBJ, PBRUSHOBJ,
                            PPOINTL, MIX);

typedef BOOL STDCALL (*PFN_Paint)(PSURFOBJ, PCLIPOBJ, PBRUSHOBJ, PPOINTL, MIX);

typedef BOOL STDCALL (*PFN_StrokePath)(PSURFOBJ, PPATHOBJ, PCLIPOBJ, PXFORMOBJ,
                               PBRUSHOBJ, PPOINTL, PLINEATTRS, MIX);

typedef BOOL STDCALL (*PFN_FillPath)(PSURFOBJ, PPATHOBJ, PCLIPOBJ, PBRUSHOBJ,
                             PPOINTL, MIX, ULONG);

typedef BOOL STDCALL (*PFN_StrokeAndFillPath)(PSURFOBJ, PPATHOBJ, PCLIPOBJ,
                PXFORMOBJ, PBRUSHOBJ, PLINEATTRS, PBRUSHOBJ,
                PPOINTL, MIX, ULONG);

typedef BOOL STDCALL (*PFN_LineTo)(PSURFOBJ, PCLIPOBJ, PBRUSHOBJ,
                           LONG, LONG, LONG, LONG, PRECTL, MIX);

typedef BOOL STDCALL (*PFN_CopyBits)(PSURFOBJ, PSURFOBJ, PCLIPOBJ,
                             PXLATEOBJ, PRECTL, PPOINTL);

typedef VOID STDCALL (*PFN_Synchronize)(DHPDEV, PRECTL);

typedef VOID STDCALL (*PFN_MovePointer)(PSURFOBJ, LONG, LONG, PRECTL);

typedef VOID STDCALL (*PFN_SetPointerShape)(PSURFOBJ, PSURFOBJ, PSURFOBJ, PXLATEOBJ,
			    LONG, LONG, LONG, LONG, PRECTL, ULONG);

typedef HBITMAP STDCALL (*PFN_CreateDeviceBitmap)(DHPDEV, SIZEL, ULONG);

typedef BOOL STDCALL (*PFN_SetPalette)(DHPDEV, PALOBJ*, ULONG, ULONG, ULONG);

typedef struct _SURFGDI {
  ENGOBJ 		Header;
  SURFOBJ		SurfObj;

  INT BitsPerPixel;

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
} SURFGDI, *PSURFGDI;

typedef struct _XFORMGDI {
  ENGOBJ 		Header;
  /* XFORMOBJ has no public members */
} XFORMGDI;

typedef struct _XLATEGDI {
  ENGOBJ 		Header;
  XLATEOBJ		XlateObj;
  HPALETTE DestPal;
  HPALETTE SourcePal;

  ULONG *translationTable;
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
