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

#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <win32k/dc.h>
#include <win32k/pen.h>

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

typedef BOOL STDCALL (*PFN_BitBlt)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*,
                           XLATEOBJ*, RECTL*, POINTL*, POINTL*,
                           ROS_BRUSHOBJ*, POINTL*, ROP4);

typedef BOOL STDCALL (*PFN_TransparentBlt)(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, RECTL*, ULONG, ULONG);

typedef BOOL STDCALL (*PFN_StretchBlt)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*,
                               XLATEOBJ*, COLORADJUSTMENT*, POINTL*,
                               RECTL*, RECTL*, POINT*, ULONG);

typedef BOOL STDCALL (*PFN_TextOut)(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*,
                            RECTL*, RECTL*, ROS_BRUSHOBJ*, ROS_BRUSHOBJ*,
                            POINTL*, MIX);

typedef BOOL STDCALL (*PFN_Paint)(SURFOBJ*, CLIPOBJ*, ROS_BRUSHOBJ*, POINTL*, MIX);

typedef BOOL STDCALL (*PFN_StrokePath)(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*,
                               ROS_BRUSHOBJ*, POINTL*, LINEATTRS*, MIX);

typedef BOOL STDCALL (*PFN_FillPath)(SURFOBJ*, PATHOBJ*, CLIPOBJ*, ROS_BRUSHOBJ*,
                             POINTL*, MIX, ULONG);

typedef BOOL STDCALL (*PFN_StrokeAndFillPath)(SURFOBJ*, PATHOBJ*, CLIPOBJ*,
                XFORMOBJ*, ROS_BRUSHOBJ*, LINEATTRS*, ROS_BRUSHOBJ*,
                POINTL*, MIX, ULONG);

typedef BOOL STDCALL (*PFN_LineTo)(SURFOBJ*, CLIPOBJ*, ROS_BRUSHOBJ*,
                           LONG, LONG, LONG, LONG, RECTL*, MIX);

typedef BOOL STDCALL (*PFN_CopyBits)(SURFOBJ*, SURFOBJ*, CLIPOBJ*,
                             XLATEOBJ*, RECTL*, POINTL*);

typedef VOID STDCALL (*PFN_Synchronize)(DHPDEV, RECTL*);

typedef VOID STDCALL (*PFN_MovePointer)(SURFOBJ*, LONG, LONG, RECTL*);

typedef VOID STDCALL (*PFN_SetPointerShape)(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*,
			    LONG, LONG, LONG, LONG, RECTL*, ULONG);

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
