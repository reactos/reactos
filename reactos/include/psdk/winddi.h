/*
 * winddi.h
 *
 * GDI device driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *   Annotations by Timo Kreuzer <timo.kreuzer@reactos.org>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _WINDDI_
#define _WINDDI_
#pragma once

#ifdef __VIDEO_H__
#error video.h cannot be included with winddi.h
#else

//#include <winapifamily.h>
#include <ddrawint.h>
#include <d3dnthal.h>
#include <specstrings.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT __attribute__((dllimport))
#endif

#if defined(_ENGINE_EXPORT_)
 #define ENGAPI
#else
 #define ENGAPI DECLSPEC_IMPORT
#endif

#ifndef _NO_DDRAWINT_NO_COM

#if !defined(EXTERN_C)
 #ifdef __cplusplus
  #define EXTERN_C extern "C"
  #define __EXTERN_C extern "C"
 #else
  #define EXTERN_C extern
  #define __EXTERN_C
 #endif
#endif /* !defined(EXTERN_C) */

#if !defined(DEFINE_GUID)
 #ifdef INITGUID
  #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
      __EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
 #else /* !INITGUID */
  #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
      EXTERN_C const GUID FAR name
 #endif /* !INITGUID */
#endif /* !defined(DEFINE_GUID) */

#if !defined(DEFINE_GUIDEX)
 #define DEFINE_GUIDEX(name) EXTERN_C const CDECL GUID name
#endif /* !defined(DEFINE_GUIDEX) */

#if !defined(STATICGUIDOF)
 #define STATICGUIDOF(guid) STATIC_##guid
#endif /* !defined(STATICGUIDOF) */

#if !defined(GUID_DEFINED)
 #define GUID_DEFINED
 typedef struct _GUID
 {
     ULONG Data1;
     USHORT Data2;
     USHORT Data3;
     UCHAR Data4[8];
 } GUID;
#endif /* !defined(GUID_DEFINED) */

#if !defined(IsEqualGUID)
 #define IsEqualGUID(guid1, guid2) \
     (!memcmp((guid1), (guid2), sizeof(GUID)))
#endif /* !defined(IsEqualGUID) */

#ifndef IsEqualIID
 #define IsEqualIID IsEqualGUID
#endif /* !defined(IsEqualIID) */

#endif /* !_NO_DDRAWINT_NO_COM */

#define DDI_DRIVER_VERSION_NT4            0x00020000
#define DDI_DRIVER_VERSION_SP3            0x00020003
#define DDI_DRIVER_VERSION_NT5            0x00030000
#define DDI_DRIVER_VERSION_NT5_01         0x00030100
#define DDI_DRIVER_VERSION_NT5_01_SP1     0x00030101

#define GDI_DRIVER_VERSION                0x4000

#if defined(_X86_) && !defined(USERMODE_DRIVER) && !defined(BUILD_WOW6432)
typedef DWORD FLOATL;
#else
typedef FLOAT FLOATL;
#endif

typedef SHORT FWORD;
typedef LONG LDECI4;
typedef ULONG IDENT;

typedef ULONG_PTR HFF;
typedef ULONG_PTR HFC;

typedef LONG PTRDIFF;
typedef PTRDIFF *PPTRDIFF;
typedef LONG FIX;
typedef FIX *PFIX;
typedef ULONG ROP4;
typedef ULONG MIX;
typedef ULONG HGLYPH;
typedef HGLYPH *PHGLYPH;

typedef LONG_PTR (APIENTRY *PFN)();

DECLARE_HANDLE(HBM);
DECLARE_HANDLE(HDEV);
DECLARE_HANDLE(HSURF);
DECLARE_HANDLE(DHSURF);
DECLARE_HANDLE(DHPDEV);
DECLARE_HANDLE(HDRVOBJ);
DECLARE_HANDLE(HSEMAPHORE);

typedef _Return_type_success_(return >= 0) long NTSTATUS;

#ifndef _NTDDVDEO_
typedef struct _ENG_EVENT *PEVENT;
#endif

#define OPENGL_CMD                        4352
#define OPENGL_GETINFO                    4353
#define WNDOBJ_SETUP                      4354

#define FD_ERROR                          0xFFFFFFFF
#define DDI_ERROR                         0xFFFFFFFF

#define HFF_INVALID                       ((HFF) 0)
#define HFC_INVALID                       ((HFC) 0)
#define HGLYPH_INVALID                    ((HGLYPH) -1)

#define FP_ALTERNATEMODE                  1
#define FP_WINDINGMODE                    2

#define DN_ACCELERATION_LEVEL             1
#define DN_DEVICE_ORIGIN                  2
#define DN_SLEEP_MODE                     3
#define DN_DRAWING_BEGIN                  4
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define DN_ASSOCIATE_WINDOW               5
#define DN_COMPOSITION_CHANGED            6
#define DN_DRAWING_BEGIN_APIBITMAP        7
#define DN_SURFOBJ_DESTRUCTION            8
#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

#define SGI_EXTRASPACE                    0

#define DCR_SOLID                         0
#define DCR_DRIVER                        1
#define DCR_HALFTONE                      2

#define GX_IDENTITY                       0
#define GX_OFFSET                         1
#define GX_SCALE                          2
#define GX_GENERAL                        3

#define LTOFX(x)        ((x) << 4)
#define FXTOL(x)        ((x) >> 4)
#define FXTOLFLOOR(x)   ((x) >> 4)
#define FXTOLCEILING(x) ((x + 0x0F) >> 4)
#define FXTOLROUND(x)   ((((x) >> 3) + 1) >> 1)

#define SIZEOFDV(cAxes)   (offsetof(DESIGNVECTOR, dvValues) + (cAxes) * sizeof(LONG))
#define SIZEOFAXIW(cAxes) (offsetof(AXESLISTW, axlAxisInfo) + (cAxes) * sizeof(AXISINFOW))
#define SIZEOFAXIA(cAxes) (offsetof(AXESLISTA, axlAxisInfo) + (cAxes) * sizeof(AXISINFOA))

typedef struct _POINTE {
	FLOATL  x;
	FLOATL  y;
} POINTE, *PPOINTE;

typedef union _FLOAT_LONG {
  FLOATL  e;
  LONG  l;
} FLOAT_LONG, *PFLOAT_LONG;

typedef struct _POINTFIX {
  FIX  x;
  FIX  y;
} POINTFIX, *PPOINTFIX;

typedef struct _RECTFX {
  FIX  xLeft;
  FIX  yTop;
  FIX  xRight;
  FIX  yBottom;
} RECTFX, *PRECTFX;

typedef struct _POINTQF {
  LARGE_INTEGER  x;
  LARGE_INTEGER  y;
} POINTQF, *PPOINTQF;


typedef struct _BLENDOBJ {
  BLENDFUNCTION  BlendFunction;
} BLENDOBJ,*PBLENDOBJ;

/* BRUSHOBJ.flColorType */
#define BR_DEVICE_ICM    0x01
#define BR_HOST_ICM      0x02
#define BR_CMYKCOLOR     0x04
#define BR_ORIGCOLOR     0x08

typedef struct _BRUSHOBJ {
  ULONG  iSolidColor;
  PVOID  pvRbrush;
  FLONG  flColorType;
} BRUSHOBJ;

typedef struct _CIECHROMA {
  LDECI4  x;
  LDECI4  y;
  LDECI4  Y;
} CIECHROMA;

typedef struct _RUN {
  LONG  iStart;
  LONG  iStop;
} RUN, *PRUN;

typedef struct _CLIPLINE {
  POINTFIX  ptfxA;
  POINTFIX  ptfxB;
  LONG  lStyleState;
  ULONG  c;
  _Field_size_(c) RUN  arun[1];
} CLIPLINE, *PCLIPLINE;

/* CLIPOBJ.iDComplexity constants */
#define DC_TRIVIAL                        0
#define DC_RECT                           1
#define DC_COMPLEX                        3

/* CLIPOBJ.iFComplexity constants */
#define FC_RECT                           1
#define FC_RECT4                          2
#define FC_COMPLEX                        3

/* CLIPOBJ.iMode constants */
#define TC_RECTANGLES                     0
#define TC_PATHOBJ                        2

/* CLIPOBJ.fjOptions constants */
#define OC_BANK_CLIP                      1

typedef struct _CLIPOBJ {
  ULONG  iUniq;
  RECTL  rclBounds;
  BYTE  iDComplexity;
  BYTE  iFComplexity;
  BYTE  iMode;
  BYTE  fjOptions;
} CLIPOBJ;

typedef struct _COLORINFO {
  CIECHROMA  Red;
  CIECHROMA  Green;
  CIECHROMA  Blue;
  CIECHROMA  Cyan;
  CIECHROMA  Magenta;
  CIECHROMA  Yellow;
  CIECHROMA  AlignmentWhite;
  LDECI4  RedGamma;
  LDECI4  GreenGamma;
  LDECI4  BlueGamma;
  LDECI4  MagentaInCyanDye;
  LDECI4  YellowInCyanDye;
  LDECI4  CyanInMagentaDye;
  LDECI4  YellowInMagentaDye;
  LDECI4  CyanInYellowDye;
  LDECI4  MagentaInYellowDye;
} COLORINFO, *PCOLORINFO;

/* DEVHTADJDATA.DeviceFlags constants */
#define DEVHTADJF_COLOR_DEVICE            0x00000001
#define DEVHTADJF_ADDITIVE_DEVICE         0x00000002

typedef struct _DEVHTINFO {
  DWORD  HTFlags;
  DWORD  HTPatternSize;
  DWORD  DevPelsDPI;
  COLORINFO  ColorInfo;
} DEVHTINFO, *PDEVHTINFO;

typedef struct _DEVHTADJDATA {
  DWORD   DeviceFlags;
  DWORD   DeviceXDPI;
  DWORD   DeviceYDPI;
  PDEVHTINFO  pDefHTInfo;
  PDEVHTINFO  pAdjHTInfo;
} DEVHTADJDATA, *PDEVHTADJDATA;

/* DEVINFO.flGraphicsCaps flags */
#define GCAPS_BEZIERS           0x00000001
#define GCAPS_GEOMETRICWIDE     0x00000002
#define GCAPS_ALTERNATEFILL     0x00000004
#define GCAPS_WINDINGFILL       0x00000008
#define GCAPS_HALFTONE          0x00000010
#define GCAPS_COLOR_DITHER      0x00000020
#define GCAPS_HORIZSTRIKE       0x00000040
#define GCAPS_VERTSTRIKE        0x00000080
#define GCAPS_OPAQUERECT        0x00000100
#define GCAPS_VECTORFONT        0x00000200
#define GCAPS_MONO_DITHER       0x00000400
#define GCAPS_ASYNCCHANGE       0x00000800
#define GCAPS_ASYNCMOVE         0x00001000
#define GCAPS_DONTJOURNAL       0x00002000
#define GCAPS_DIRECTDRAW        0x00004000
#define GCAPS_ARBRUSHOPAQUE     0x00008000
#define GCAPS_PANNING           0x00010000
#define GCAPS_HIGHRESTEXT       0x00040000
#define GCAPS_PALMANAGED        0x00080000
#define GCAPS_DITHERONREALIZE   0x00200000
#define GCAPS_NO64BITMEMACCESS  0x00400000
#define GCAPS_FORCEDITHER       0x00800000
#define GCAPS_GRAY16            0x01000000
#define GCAPS_ICM               0x02000000
#define GCAPS_CMYKCOLOR         0x04000000
#define GCAPS_LAYERED           0x08000000
#define GCAPS_ARBRUSHTEXT       0x10000000
#define GCAPS_SCREENPRECISION   0x20000000
#define GCAPS_FONT_RASTERIZER   0x40000000
#define GCAPS_NUP               0x80000000

/* DEVINFO.iDitherFormat constants */
#define BMF_1BPP       1L
#define BMF_4BPP       2L
#define BMF_8BPP       3L
#define BMF_16BPP      4L
#define BMF_24BPP      5L
#define BMF_32BPP      6L
#define BMF_4RLE       7L
#define BMF_8RLE       8L
#define BMF_JPEG       9L
#define BMF_PNG       10L

/* DEVINFO.flGraphicsCaps2 flags */
#define GCAPS2_JPEGSRC          0x00000001
#define GCAPS2_xxxx             0x00000002
#define GCAPS2_PNGSRC           0x00000008
#define GCAPS2_CHANGEGAMMARAMP  0x00000010
#define GCAPS2_ALPHACURSOR      0x00000020
#define GCAPS2_SYNCFLUSH        0x00000040
#define GCAPS2_SYNCTIMER        0x00000080
#define GCAPS2_ICD_MULTIMON     0x00000100
#define GCAPS2_MOUSETRAILS      0x00000200
#define GCAPS2_RESERVED1        0x00000400
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define GCAPS2_EXCLUDELAYERED    0x00000800
#define GCAPS2_INCLUDEAPIBITMAPS 0x00001000
#define GCAPS2_SHOWHIDDENPOINTER 0x00002000
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define GCAPS2_CLEARTYPE         0x00004000
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define GCAPS2_ACC_DRIVER        0x00008000
#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

typedef struct _DEVINFO {
  FLONG  flGraphicsCaps;
  LOGFONTW  lfDefaultFont;
  LOGFONTW  lfAnsiVarFont;
  LOGFONTW  lfAnsiFixFont;
  ULONG  cFonts;
  ULONG  iDitherFormat;
  USHORT  cxDither;
  USHORT  cyDither;
  HPALETTE  hpalDefault;
  FLONG  flGraphicsCaps2;
} DEVINFO, *PDEVINFO;

struct _DRIVEROBJ;

typedef BOOL
(APIENTRY CALLBACK *FREEOBJPROC)(
    _In_ struct _DRIVEROBJ  *pDriverObj);

typedef struct _DRIVEROBJ {
  PVOID  pvObj;
  FREEOBJPROC  pFreeProc;
  HDEV  hdev;
  DHPDEV  dhpdev;
} DRIVEROBJ;

/* DRVFN.iFunc constants */
#define INDEX_DrvEnablePDEV               0L
#define INDEX_DrvCompletePDEV             1L
#define INDEX_DrvDisablePDEV              2L
#define INDEX_DrvEnableSurface            3L
#define INDEX_DrvDisableSurface           4L
#define INDEX_DrvAssertMode               5L
#define INDEX_DrvOffset                   6L
#define INDEX_DrvResetPDEV                7L
#define INDEX_DrvDisableDriver            8L
#define INDEX_DrvUnknown1                 9L
#define INDEX_DrvCreateDeviceBitmap       10L
#define INDEX_DrvDeleteDeviceBitmap       11L
#define INDEX_DrvRealizeBrush             12L
#define INDEX_DrvDitherColor              13L
#define INDEX_DrvStrokePath               14L
#define INDEX_DrvFillPath                 15L
#define INDEX_DrvStrokeAndFillPath        16L
#define INDEX_DrvPaint                    17L
#define INDEX_DrvBitBlt                   18L
#define INDEX_DrvCopyBits                 19L
#define INDEX_DrvStretchBlt               20L
#define INDEX_DrvUnknown2                 21L
#define INDEX_DrvSetPalette               22L
#define INDEX_DrvTextOut                  23L
#define INDEX_DrvEscape                   24L
#define INDEX_DrvDrawEscape               25L
#define INDEX_DrvQueryFont                26L
#define INDEX_DrvQueryFontTree            27L
#define INDEX_DrvQueryFontData            28L
#define INDEX_DrvSetPointerShape          29L
#define INDEX_DrvMovePointer              30L
#define INDEX_DrvLineTo                   31L
#define INDEX_DrvSendPage                 32L
#define INDEX_DrvStartPage                33L
#define INDEX_DrvEndDoc                   34L
#define INDEX_DrvStartDoc                 35L
#define INDEX_DrvUnknown3                 36L
#define INDEX_DrvGetGlyphMode             37L
#define INDEX_DrvSynchronize              38L
#define INDEX_DrvUnknown4                 39L
#define INDEX_DrvSaveScreenBits           40L
#define INDEX_DrvGetModes                 41L
#define INDEX_DrvFree                     42L
#define INDEX_DrvDestroyFont              43L
#define INDEX_DrvQueryFontCaps            44L
#define INDEX_DrvLoadFontFile             45L
#define INDEX_DrvUnloadFontFile           46L
#define INDEX_DrvFontManagement           47L
#define INDEX_DrvQueryTrueTypeTable       48L
#define INDEX_DrvQueryTrueTypeOutline     49L
#define INDEX_DrvGetTrueTypeFile          50L
#define INDEX_DrvQueryFontFile            51L
#define INDEX_DrvMovePanning              52L
#define INDEX_DrvQueryAdvanceWidths       53L
#define INDEX_DrvSetPixelFormat           54L
#define INDEX_DrvDescribePixelFormat      55L
#define INDEX_DrvSwapBuffers              56L
#define INDEX_DrvStartBanding             57L
#define INDEX_DrvNextBand                 58L
#define INDEX_DrvGetDirectDrawInfo        59L
#define INDEX_DrvEnableDirectDraw         60L
#define INDEX_DrvDisableDirectDraw        61L
#define INDEX_DrvQuerySpoolType           62L
#define INDEX_DrvUnknown5                 63L
#define INDEX_DrvIcmCreateColorTransform  64L
#define INDEX_DrvIcmDeleteColorTransform  65L
#define INDEX_DrvIcmCheckBitmapBits       66L
#define INDEX_DrvIcmSetDeviceGammaRamp    67L
#define INDEX_DrvGradientFill             68L
#define INDEX_DrvStretchBltROP            69L
#define INDEX_DrvPlgBlt                   70L
#define INDEX_DrvAlphaBlend               71L
#define INDEX_DrvSynthesizeFont           72L
#define INDEX_DrvGetSynthesizedFontFiles  73L
#define INDEX_DrvTransparentBlt           74L
#define INDEX_DrvQueryPerBandInfo         75L
#define INDEX_DrvQueryDeviceSupport       76L
#define INDEX_DrvReserved1                77L
#define INDEX_DrvReserved2                78L
#define INDEX_DrvReserved3                79L
#define INDEX_DrvReserved4                80L
#define INDEX_DrvReserved5                81L
#define INDEX_DrvReserved6                82L
#define INDEX_DrvReserved7                83L
#define INDEX_DrvReserved8                84L
#define INDEX_DrvDeriveSurface            85L
#define INDEX_DrvQueryGlyphAttrs          86L
#define INDEX_DrvNotify                   87L
#define INDEX_DrvSynchronizeSurface       88L
#define INDEX_DrvResetDevice              89L
#define INDEX_DrvReserved9                90L
#define INDEX_DrvReserved10               91L
#define INDEX_DrvReserved11               92L
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define INDEX_DrvRenderHint               93L
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define INDEX_DrvCreateDeviceBitmapEx     94L
#define INDEX_DrvDeleteDeviceBitmapEx     95L
#define INDEX_DrvAssociateSharedSurface   96L
#define INDEX_DrvSynchronizeRedirectionBitmaps  97L
#define INDEX_DrvAccumulateD3DDirtyRect   98L
#define INDEX_DrvStartDxInterop           99L
#define INDEX_DrvEndDxInterop            100L
#define INDEX_DrvLockDisplayArea         101L
#define INDEX_DrvUnlockDisplayArea       102L
#define INDEX_LAST                       103L
#else /* (NTDDI_VERSION >= NTDDI_WIN7) */
#define INDEX_LAST                        94L
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */
#else /* (NTDDI_VERSION >= NTDDI_VISTA) */
#define INDEX_LAST                        93L
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

typedef struct _DRVFN {
  ULONG  iFunc;
  PFN  pfn;
} DRVFN, *PDRVFN;

/* DRVENABLEDATA.iDriverVersion constants */
#define DDI_DRIVER_VERSION_NT4            0x00020000
#define DDI_DRIVER_VERSION_SP3            0x00020003
#define DDI_DRIVER_VERSION_NT5            0x00030000
#define DDI_DRIVER_VERSION_NT5_01         0x00030100
#define DDI_DRIVER_VERSION_NT5_01_SP1     0x00030101

typedef struct tagDRVENABLEDATA {
  ULONG  iDriverVersion;
  ULONG  c;
  DRVFN  *pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;

/* Font file status values */
#define FF_SIGNATURE_VERIFIED             0x00000001L
#define FF_IGNORED_SIGNATURE              0x00000002L

 /* Obsolete in Windows 2000 and later */
typedef struct {
  DWORD  nSize;
  HDC  hdc;
  PBYTE  pvEMF;
  PBYTE  pvCurrentRecord;
} EMFINFO, *PEMFINFO;

typedef struct _ENGSAFESEMAPHORE {
  HSEMAPHORE  hsem;
  LONG  lCount;
} ENGSAFESEMAPHORE;

typedef struct _ENG_TIME_FIELDS {
  _Field_range_(1601,MAXUSHORT) USHORT  usYear;
  _Field_range_(1,12) USHORT  usMonth;
  _Field_range_(1,31) USHORT  usDay;
  _Field_range_(0,23) USHORT  usHour;
  _Field_range_(0,59) USHORT  usMinute;
  _Field_range_(0,59) USHORT  usSecond;
  _Field_range_(0,999) USHORT  usMilliseconds;
  _Field_range_(0,6) USHORT  usWeekday; // 0 == Sunday
} ENG_TIME_FIELDS, *PENG_TIME_FIELDS;

typedef struct _ENUMRECTS {
  ULONG  c;
  RECTL  arcl[1];
} ENUMRECTS;

typedef struct _FD_XFORM {
  FLOATL  eXX;
  FLOATL  eXY;
  FLOATL  eYX;
  FLOATL  eYY;
} FD_XFORM, *PFD_XFORM;

/* FD_DEVICEMETRICS.flRealizedType constants */
#define FDM_TYPE_BM_SIDE_CONST            0x00000001
#define FDM_TYPE_MAXEXT_EQUAL_BM_SIDE     0x00000002
#define FDM_TYPE_CHAR_INC_EQUAL_BM_BASE   0x00000004
#define FDM_TYPE_ZERO_BEARINGS            0x00000008
#define FDM_TYPE_CONST_BEARINGS           0x00000010

typedef struct _FD_DEVICEMETRICS {
  FLONG  flRealizedType;
  POINTE  pteBase;
  POINTE  pteSide;
  LONG  lD;
  FIX  fxMaxAscender;
  FIX  fxMaxDescender;
  POINTL  ptlUnderline1;
  POINTL  ptlStrikeout;
  POINTL  ptlULThickness;
  POINTL  ptlSOThickness;
  ULONG  cxMax;
  ULONG  cyMax;
  ULONG  cjGlyphMax;
  FD_XFORM  fdxQuantized;
  LONG  lNonLinearExtLeading;
  LONG  lNonLinearIntLeading;
  LONG  lNonLinearMaxCharWidth;
  LONG  lNonLinearAvgCharWidth;
  LONG  lMinA;
  LONG  lMinC;
  LONG  lMinD;
  LONG  alReserved[1];
} FD_DEVICEMETRICS, *PFD_DEVICEMETRICS;

/* FD_GLYPHATTR.iMode constants */
#define FO_ATTR_MODE_ROTATE               1

typedef struct _FD_GLYPHATTR {
  ULONG  cjThis;
  ULONG  cGlyphs;
  ULONG  iMode;
  _Field_size_((cGlyphs+7)/8) BYTE  aGlyphAttr[1];
} FD_GLYPHATTR, *PFD_GLYPHATTR;

/* FD_GLYPHSET.flAccel */
#define GS_UNICODE_HANDLES                0x00000001
#define GS_8BIT_HANDLES                   0x00000002
#define GS_16BIT_HANDLES                  0x00000004

typedef struct _WCRUN {
  WCHAR  wcLow;
  USHORT  cGlyphs;
  HGLYPH  *phg;
} WCRUN, *PWCRUN;

typedef struct _FD_GLYPHSET {
  ULONG  cjThis;
  FLONG  flAccel;
  ULONG  cGlyphsSupported;
  ULONG  cRuns;
  _Field_size_(cRuns) WCRUN  awcrun[1];
} FD_GLYPHSET, *PFD_GLYPHSET;

typedef struct _FD_KERNINGPAIR {
  WCHAR  wcFirst;
  WCHAR  wcSecond;
  FWORD  fwdKern;
} FD_KERNINGPAIR;

 /* Obsolete in Windows 2000 and later */
typedef struct _LIGATURE {
  ULONG culSize;
  LPWSTR pwsz;
  ULONG chglyph;
  HGLYPH ahglyph[1];
} LIGATURE, *PLIGATURE;

 /* Obsolete in Windows 2000 and later */
typedef struct _FD_LIGATURE {
  ULONG culThis;
  ULONG ulType;
  ULONG cLigatures;
  LIGATURE alig[1];
} FD_LIGATURE;

#if defined(_X86_) && !defined(USERMODE_DRIVER)
typedef struct _FLOATOBJ
{
  ULONG  ul1;
  ULONG  ul2;
} FLOATOBJ, *PFLOATOBJ;
#else
typedef FLOAT FLOATOBJ, *PFLOATOBJ;
#endif

#ifndef USERMODE_DRIVER
typedef struct tagFLOATOBJ_XFORM {
  FLOATOBJ  eM11;
  FLOATOBJ  eM12;
  FLOATOBJ  eM21;
  FLOATOBJ  eM22;
  FLOATOBJ  eDx;
  FLOATOBJ  eDy;
} FLOATOBJ_XFORM, *PFLOATOBJ_XFORM, FAR *LPFLOATOBJ_XFORM;
#else
typedef XFORML FLOATOBJ_XFORM, *PFLOATOBJ_XFORM, FAR *LPFLOATOBJ_XFORM;
#endif

/* FONTDIFF.fsSelection */
#define FM_SEL_ITALIC                     0x0001
#define FM_SEL_UNDERSCORE                 0x0002
#define FM_SEL_NEGATIVE                   0x0004
#define FM_SEL_OUTLINED                   0x0008
#define FM_SEL_STRIKEOUT                  0x0010
#define FM_SEL_BOLD                       0x0020
#define FM_SEL_REGULAR                    0x0040

typedef struct _FONTDIFF {
  BYTE  jReserved1;
  BYTE  jReserved2;
  BYTE  jReserved3;
  BYTE  bWeight;
  USHORT  usWinWeight;
  FSHORT  fsSelection;
  FWORD  fwdAveCharWidth;
  FWORD  fwdMaxCharInc;
  POINTL  ptlCaret;
} FONTDIFF;

typedef struct _FONTSIM {
  PTRDIFF  dpBold;
  PTRDIFF  dpItalic;
  PTRDIFF  dpBoldItalic;
} FONTSIM;

/* FONTINFO.flCaps constants */
#define FO_DEVICE_FONT                    1L
#define FO_OUTLINE_CAPABLE                2L

typedef struct _FONTINFO {
  ULONG  cjThis;
  FLONG  flCaps;
  ULONG  cGlyphsSupported;
  ULONG  cjMaxGlyph1;
  ULONG  cjMaxGlyph4;
  ULONG  cjMaxGlyph8;
  ULONG  cjMaxGlyph32;
} FONTINFO, *PFONTINFO;

/* FONTOBJ.flFontType constants */
#define FO_TYPE_RASTER   RASTER_FONTTYPE
#define FO_TYPE_DEVICE   DEVICE_FONTTYPE
#define FO_TYPE_TRUETYPE TRUETYPE_FONTTYPE
#define FO_TYPE_OPENTYPE OPENTYPE_FONTTYPE

#define FO_SIM_BOLD      0x00002000
#define FO_SIM_ITALIC    0x00004000
#define FO_EM_HEIGHT     0x00008000
#define FO_GRAY16        0x00010000
#define FO_NOGRAY16      0x00020000
#define FO_NOHINTS       0x00040000
#define FO_NO_CHOICE     0x00080000
#define FO_CFF            0x00100000
#define FO_POSTSCRIPT     0x00200000
#define FO_MULTIPLEMASTER 0x00400000
#define FO_VERT_FACE      0x00800000
#define FO_DBCS_FONT      0X01000000
#define FO_NOCLEARTYPE    0x02000000
#define FO_CLEARTYPE_X    0x10000000
#define FO_CLEARTYPE_Y    0x20000000
#define FO_CLEARTYPENATURAL_X 0x40000000

#define FD_NEGATIVE_FONT   1L /* Obsolete in Windows 2000 and later */
#define FO_DEVICE_FONT     1L
#define FO_OUTLINE_CAPABLE 2L

typedef struct _FONTOBJ {
  ULONG  iUniq;
  ULONG  iFace;
  ULONG  cxMax;
  FLONG  flFontType;
  ULONG_PTR  iTTUniq;
  ULONG_PTR  iFile;
  SIZE  sizLogResPpi;
  ULONG  ulStyleSize;
  PVOID  pvConsumer;
  PVOID  pvProducer;
} FONTOBJ;

typedef struct _GAMMARAMP {
  WORD  Red[256];
  WORD  Green[256];
  WORD  Blue[256];
} GAMMARAMP, *PGAMMARAMP;

/* GDIINFO.ulPrimaryOrder constants */
#define PRIMARY_ORDER_ABC                 0
#define PRIMARY_ORDER_ACB                 1
#define PRIMARY_ORDER_BAC                 2
#define PRIMARY_ORDER_BCA                 3
#define PRIMARY_ORDER_CBA                 4
#define PRIMARY_ORDER_CAB                 5

/* GDIINFO.ulHTPatternSize constants */
#define HT_PATSIZE_2x2                    0
#define HT_PATSIZE_2x2_M                  1
#define HT_PATSIZE_4x4                    2
#define HT_PATSIZE_4x4_M                  3
#define HT_PATSIZE_6x6                    4
#define HT_PATSIZE_6x6_M                  5
#define HT_PATSIZE_8x8                    6
#define HT_PATSIZE_8x8_M                  7
#define HT_PATSIZE_10x10                  8
#define HT_PATSIZE_10x10_M                9
#define HT_PATSIZE_12x12                  10
#define HT_PATSIZE_12x12_M                11
#define HT_PATSIZE_14x14                  12
#define HT_PATSIZE_14x14_M                13
#define HT_PATSIZE_16x16                  14
#define HT_PATSIZE_16x16_M                15
#define HT_PATSIZE_SUPERCELL              16
#define HT_PATSIZE_SUPERCELL_M            17
#define HT_PATSIZE_USER                   18
#define HT_PATSIZE_MAX_INDEX              HT_PATSIZE_USER
#define HT_PATSIZE_DEFAULT                HT_PATSIZE_SUPERCELL_M
#define HT_USERPAT_CX_MIN                 4
#define HT_USERPAT_CX_MAX                 256
#define HT_USERPAT_CY_MIN                 4
#define HT_USERPAT_CY_MAX                 256

/* GDIINFO.ulHTOutputFormat constants */
#define HT_FORMAT_1BPP                    0
#define HT_FORMAT_4BPP                    2
#define HT_FORMAT_4BPP_IRGB               3
#define HT_FORMAT_8BPP                    4
#define HT_FORMAT_16BPP                   5
#define HT_FORMAT_24BPP                   6
#define HT_FORMAT_32BPP                   7

/* GDIINFO.flHTFlags */
#define HT_FLAG_SQUARE_DEVICE_PEL         0x00000001
#define HT_FLAG_HAS_BLACK_DYE             0x00000002
#define HT_FLAG_ADDITIVE_PRIMS            0x00000004
#define HT_FLAG_USE_8BPP_BITMASK          0x00000008
#define HT_FLAG_INK_HIGH_ABSORPTION       0x00000010
#define HT_FLAG_INK_ABSORPTION_INDICES    0x00000060
#define HT_FLAG_DO_DEVCLR_XFORM           0x00000080
#define HT_FLAG_OUTPUT_CMY                0x00000100
#define HT_FLAG_PRINT_DRAFT_MODE          0x00000200
#define HT_FLAG_INVERT_8BPP_BITMASK_IDX   0x00000400
#define HT_FLAG_8BPP_CMY332_MASK          0xFF000000

#define MAKE_CMYMASK_BYTE(c,m,y)          ((BYTE)(((BYTE)(c) & 0x07) << 5) \
                                          |(BYTE)(((BYTE)(m) & 0x07) << 2) \
                                          |(BYTE)((BYTE)(y) & 0x03))

#define MAKE_CMY332_MASK(c,m,y)           ((DWORD)(((DWORD)(c) & 0x07) << 29)\
                                          |(DWORD)(((DWORD)(m) & 0x07) << 26)\
                                          |(DWORD)(((DWORD)(y) & 0x03) << 24))

/* GDIINFO.flHTFlags constants */
#define HT_FLAG_INK_ABSORPTION_IDX0       0x00000000
#define HT_FLAG_INK_ABSORPTION_IDX1       0x00000020
#define HT_FLAG_INK_ABSORPTION_IDX2       0x00000040
#define HT_FLAG_INK_ABSORPTION_IDX3       0x00000060

#define HT_FLAG_HIGHEST_INK_ABSORPTION    (HT_FLAG_INK_HIGH_ABSORPTION \
                                          |HT_FLAG_INK_ABSORPTION_IDX3)
#define HT_FLAG_HIGHER_INK_ABSORPTION     (HT_FLAG_INK_HIGH_ABSORPTION \
                                          |HT_FLAG_INK_ABSORPTION_IDX2)
#define HT_FLAG_HIGH_INK_ABSORPTION       (HT_FLAG_INK_HIGH_ABSORPTION \
                                          |HT_FLAG_INK_ABSORPTION_IDX1)
#define HT_FLAG_NORMAL_INK_ABSORPTION     HT_FLAG_INK_ABSORPTION_IDX0
#define HT_FLAG_LOW_INK_ABSORPTION        HT_FLAG_INK_ABSORPTION_IDX1
#define HT_FLAG_LOWER_INK_ABSORPTION      HT_FLAG_INK_ABSORPTION_IDX2
#define HT_FLAG_LOWEST_INK_ABSORPTION     HT_FLAG_INK_ABSORPTION_IDX3

#define HT_BITMASKPALRGB                  (DWORD)'0BGR'
#define HT_SET_BITMASKPAL2RGB(pPal)       (*((LPDWORD)(pPal)) = HT_BITMASKPALRGB)
#define HT_IS_BITMASKPALRGB(pPal)         (*((LPDWORD)(pPal)) == (DWORD)0)

/* GDIINFO.ulPhysicalPixelCharacteristics constants */
#define PPC_DEFAULT                       0x0
#define PPC_UNDEFINED                     0x1
#define PPC_RGB_ORDER_VERTICAL_STRIPES    0x2
#define PPC_BGR_ORDER_VERTICAL_STRIPES    0x3
#define PPC_RGB_ORDER_HORIZONTAL_STRIPES  0x4
#define PPC_BGR_ORDER_HORIZONTAL_STRIPES  0x5

#define PPG_DEFAULT                       0
#define PPG_SRGB                          1

typedef struct _GDIINFO {
  ULONG  ulVersion;
  ULONG  ulTechnology;
  ULONG  ulHorzSize;
  ULONG  ulVertSize;
  ULONG  ulHorzRes;
  ULONG  ulVertRes;
  ULONG  cBitsPixel;
  ULONG  cPlanes;
  ULONG  ulNumColors;
  ULONG  flRaster;
  ULONG  ulLogPixelsX;
  ULONG  ulLogPixelsY;
  ULONG  flTextCaps;
  ULONG  ulDACRed;
  ULONG  ulDACGreen;
  ULONG  ulDACBlue;
  ULONG  ulAspectX;
  ULONG  ulAspectY;
  ULONG  ulAspectXY;
  LONG  xStyleStep;
  LONG  yStyleStep;
  LONG  denStyleStep;
  POINTL  ptlPhysOffset;
  SIZEL  szlPhysSize;
  ULONG  ulNumPalReg;
  COLORINFO  ciDevice;
  ULONG  ulDevicePelsDPI;
  ULONG  ulPrimaryOrder;
  ULONG  ulHTPatternSize;
  ULONG  ulHTOutputFormat;
  ULONG  flHTFlags;
  ULONG  ulVRefresh;
  ULONG  ulBltAlignment;
  ULONG  ulPanningHorzRes;
  ULONG  ulPanningVertRes;
  ULONG  xPanningAlignment;
  ULONG  yPanningAlignment;
  ULONG  cxHTPat;
  ULONG  cyHTPat;
  LPBYTE  pHTPatA;
  LPBYTE  pHTPatB;
  LPBYTE  pHTPatC;
  ULONG  flShadeBlend;
  ULONG  ulPhysicalPixelCharacteristics;
  ULONG  ulPhysicalPixelGamma;
} GDIINFO, *PGDIINFO;

/* PATHDATA.flags constants */
#define PD_BEGINSUBPATH                   0x00000001
#define PD_ENDSUBPATH                     0x00000002
#define PD_RESETSTYLE                     0x00000004
#define PD_CLOSEFIGURE                    0x00000008
#define PD_BEZIERS                        0x00000010
#define PD_ALL                            (PD_BEGINSUBPATH \
                                          |PD_ENDSUBPATH \
                                          |PD_RESETSTYLE \
                                          |PD_CLOSEFIGURE \
                                          PD_BEZIERS)

typedef struct _PATHDATA {
  FLONG  flags;
  ULONG  count;
  POINTFIX  *pptfx;
} PATHDATA, *PPATHDATA;

/* PATHOBJ.fl constants */
#define PO_BEZIERS                        0x00000001
#define PO_ELLIPSE                        0x00000002
#define PO_ALL_INTEGERS                   0x00000004
#define PO_ENUM_AS_INTEGERS               0x00000008
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define PO_WIDENED                        0x00000010
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

typedef struct _PATHOBJ {
  FLONG  fl;
  ULONG  cCurves;
} PATHOBJ;

typedef BYTE GAMMA_TABLES[2][256];

typedef struct _GLYPHBITS {
  POINTL  ptlOrigin;
  SIZEL  sizlBitmap;
  BYTE  aj[1];
} GLYPHBITS;

typedef union _GLYPHDEF {
  GLYPHBITS  *pgb;
  PATHOBJ  *ppo;
} GLYPHDEF;

typedef struct _GLYPHPOS {
  HGLYPH  hg;
  GLYPHDEF  *pgdf;
  POINTL  ptl;
} GLYPHPOS, *PGLYPHPOS;

typedef struct _GLYPHDATA {
  GLYPHDEF  gdf;
  HGLYPH  hg;
  FIX  fxD;
  FIX  fxA;
  FIX  fxAB;
  FIX  fxInkTop;
  FIX  fxInkBottom;
  RECTL  rclInk;
  POINTQF  ptqD;
} GLYPHDATA;

typedef struct _IFIEXTRA {
  ULONG  ulIdentifier;
  PTRDIFF  dpFontSig;
  ULONG  cig;
  PTRDIFF  dpDesignVector;
  PTRDIFF  dpAxesInfoW;
  ULONG  aulReserved[1];
} IFIEXTRA, *PIFIEXTRA;

/* IFIMETRICS constants */

#define FM_VERSION_NUMBER                 0x0

/* IFIMETRICS.fsType constants */
#define FM_TYPE_LICENSED                  0x2
#define FM_READONLY_EMBED                 0x4
#define FM_EDITABLE_EMBED                 0x8
#define FM_NO_EMBEDDING                   FM_TYPE_LICENSED

/* IFIMETRICS.flInfo constants */
#define FM_INFO_TECH_TRUETYPE             0x00000001
#define FM_INFO_TECH_BITMAP               0x00000002
#define FM_INFO_TECH_STROKE               0x00000004
#define FM_INFO_TECH_OUTLINE_NOT_TRUETYPE 0x00000008
#define FM_INFO_ARB_XFORMS                0x00000010
#define FM_INFO_1BPP                      0x00000020
#define FM_INFO_4BPP                      0x00000040
#define FM_INFO_8BPP                      0x00000080
#define FM_INFO_16BPP                     0x00000100
#define FM_INFO_24BPP                     0x00000200
#define FM_INFO_32BPP                     0x00000400
#define FM_INFO_INTEGER_WIDTH             0x00000800
#define FM_INFO_CONSTANT_WIDTH            0x00001000
#define FM_INFO_NOT_CONTIGUOUS            0x00002000
#define FM_INFO_TECH_MM                   0x00004000
#define FM_INFO_RETURNS_OUTLINES          0x00008000
#define FM_INFO_RETURNS_STROKES           0x00010000
#define FM_INFO_RETURNS_BITMAPS           0x00020000
#define FM_INFO_DSIG                      0x00040000
#define FM_INFO_RIGHT_HANDED              0x00080000
#define FM_INFO_INTEGRAL_SCALING          0x00100000
#define FM_INFO_90DEGREE_ROTATIONS        0x00200000
#define FM_INFO_OPTICALLY_FIXED_PITCH     0x00400000
#define FM_INFO_DO_NOT_ENUMERATE          0x00800000
#define FM_INFO_ISOTROPIC_SCALING_ONLY    0x01000000
#define FM_INFO_ANISOTROPIC_SCALING_ONLY  0x02000000
#define FM_INFO_TECH_CFF                  0x04000000
#define FM_INFO_FAMILY_EQUIV              0x08000000
#define FM_INFO_DBCS_FIXED_PITCH          0x10000000
#define FM_INFO_NONNEGATIVE_AC            0x20000000
#define FM_INFO_IGNORE_TC_RA_ABLE         0x40000000
#define FM_INFO_TECH_TYPE1                0x80000000

#define MAXCHARSETS                       16 /* Obsolete in Windows 2000 and later */

/* IFIMETRICS.ulPanoseCulture constants */
#define  FM_PANOSE_CULTURE_LATIN          0x0

typedef struct _IFIMETRICS {
  ULONG  cjThis;
  ULONG  cjIfiExtra;
  PTRDIFF  dpwszFamilyName;
  PTRDIFF  dpwszStyleName;
  PTRDIFF  dpwszFaceName;
  PTRDIFF  dpwszUniqueName;
  PTRDIFF  dpFontSim;
  LONG  lEmbedId;
  LONG  lItalicAngle;
  LONG  lCharBias;
  PTRDIFF  dpCharSets;
  BYTE  jWinCharSet;
  BYTE  jWinPitchAndFamily;
  USHORT  usWinWeight;
  ULONG  flInfo;
  USHORT  fsSelection;
  USHORT  fsType;
  FWORD  fwdUnitsPerEm;
  FWORD  fwdLowestPPEm;
  FWORD  fwdWinAscender;
  FWORD  fwdWinDescender;
  FWORD  fwdMacAscender;
  FWORD  fwdMacDescender;
  FWORD  fwdMacLineGap;
  FWORD  fwdTypoAscender;
  FWORD  fwdTypoDescender;
  FWORD  fwdTypoLineGap;
  FWORD  fwdAveCharWidth;
  FWORD  fwdMaxCharInc;
  FWORD  fwdCapHeight;
  FWORD  fwdXHeight;
  FWORD  fwdSubscriptXSize;
  FWORD  fwdSubscriptYSize;
  FWORD  fwdSubscriptXOffset;
  FWORD  fwdSubscriptYOffset;
  FWORD  fwdSuperscriptXSize;
  FWORD  fwdSuperscriptYSize;
  FWORD  fwdSuperscriptXOffset;
  FWORD  fwdSuperscriptYOffset;
  FWORD  fwdUnderscoreSize;
  FWORD  fwdUnderscorePosition;
  FWORD  fwdStrikeoutSize;
  FWORD  fwdStrikeoutPosition;
  BYTE  chFirstChar;
  BYTE  chLastChar;
  BYTE  chDefaultChar;
  BYTE  chBreakChar;
  WCHAR  wcFirstChar;
  WCHAR  wcLastChar;
  WCHAR  wcDefaultChar;
  WCHAR  wcBreakChar;
  POINTL  ptlBaseline;
  POINTL  ptlAspect;
  POINTL  ptlCaret;
  RECTL  rclFontBox;
  BYTE  achVendId[4];
  ULONG  cKerningPairs;
  ULONG  ulPanoseCulture;
  PANOSE  panose;
#if defined(_WIN64)
  PVOID  Align;
#endif
} IFIMETRICS, *PIFIMETRICS;

/* LINEATTRS.fl */
#define LA_GEOMETRIC                      0x00000001
#define LA_ALTERNATE                      0x00000002
#define LA_STARTGAP                       0x00000004
#define LA_STYLED                         0x00000008

/* LINEATTRS.iJoin */
#define JOIN_ROUND                        0L
#define JOIN_BEVEL                        1L
#define JOIN_MITER                        2L

/* LINEATTRS.iEndCap */
#define ENDCAP_ROUND                      0L
#define ENDCAP_SQUARE                     1L
#define ENDCAP_BUTT                       2L

typedef struct _LINEATTRS {
  FLONG  fl;
  ULONG  iJoin;
  ULONG  iEndCap;
  FLOAT_LONG  elWidth;
  FLOATL  eMiterLimit;
  ULONG  cstyle;
  PFLOAT_LONG  pstyle;
  FLOAT_LONG  elStyleState;
} LINEATTRS, *PLINEATTRS;

typedef struct _PALOBJ {
  ULONG  ulReserved;
} PALOBJ;

typedef struct _PERBANDINFO {
  BOOL  bRepeatThisBand;
  SIZEL  szlBand;
  ULONG  ulHorzRes;
  ULONG  ulVertRes;
} PERBANDINFO, *PPERBANDINFO;

/* STROBJ.flAccel constants */
#define SO_FLAG_DEFAULT_PLACEMENT        0x00000001
#define SO_HORIZONTAL                    0x00000002
#define SO_VERTICAL                      0x00000004
#define SO_REVERSED                      0x00000008
#define SO_ZERO_BEARINGS                 0x00000010
#define SO_CHAR_INC_EQUAL_BM_BASE        0x00000020
#define SO_MAXEXT_EQUAL_BM_SIDE          0x00000040
#define SO_DO_NOT_SUBSTITUTE_DEVICE_FONT 0x00000080
#define SO_GLYPHINDEX_TEXTOUT            0x00000100
#define SO_ESC_NOT_ORIENT                0x00000200
#define SO_DXDY                          0x00000400
#define SO_CHARACTER_EXTRA               0x00000800
#define SO_BREAK_EXTRA                   0x00001000

typedef struct _STROBJ {
  ULONG  cGlyphs;
  FLONG  flAccel;
  ULONG  ulCharInc;
  RECTL  rclBkGround;
  GLYPHPOS  *pgp;
  LPWSTR  pwszOrg;
} STROBJ;


/* SURFOBJ.iType constants */
#define STYPE_BITMAP                      0L
#define STYPE_DEVICE                      1L
#define STYPE_DEVBITMAP                   3L

/* SURFOBJ.fjBitmap constants */
#define BMF_TOPDOWN                       0x0001
#define BMF_NOZEROINIT                    0x0002
#define BMF_DONTCACHE                     0x0004
#define BMF_USERMEM                       0x0008
#define BMF_KMSECTION                     0x0010
#define BMF_NOTSYSMEM                     0x0020
#define BMF_WINDOW_BLT                    0x0040
#define BMF_UMPDMEM                       0x0080
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define BMF_TEMP_ALPHA                    0x0100
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define BMF_ACC_NOTIFY                    0x8000
#define BMF_RMT_ENTER                     0x4000
#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

#if (NTDDI_VERSION >= NTDDI_WIN8)
#define BMF_RESERVED                      0x3E00
#elif (NTDDI_VERSION >= NTDDI_WIN7)
#define BMF_RESERVED                      0xFE00
#else
#define BMF_RESERVED                      0xFF00
#endif

typedef struct _SURFOBJ {
  DHSURF  dhsurf;
  HSURF  hsurf;
  DHPDEV  dhpdev;
  HDEV  hdev;
  SIZEL  sizlBitmap;
  ULONG  cjBits;
  PVOID  pvBits;
  PVOID  pvScan0;
  LONG  lDelta;
  ULONG  iUniq;
  ULONG  iBitmapFormat;
  USHORT  iType;
  USHORT  fjBitmap;
} SURFOBJ;

typedef struct _TYPE1_FONT {
  HANDLE  hPFM;
  HANDLE  hPFB;
  ULONG  ulIdentifier;
} TYPE1_FONT;

typedef struct _WNDOBJ {
  CLIPOBJ  coClient;
  PVOID  pvConsumer;
  RECTL  rclClient;
  SURFOBJ  *psoOwner;
} WNDOBJ, *PWNDOBJ;

typedef struct _XFORML {
  FLOATL  eM11;
  FLOATL  eM12;
  FLOATL  eM21;
  FLOATL  eM22;
  FLOATL  eDx;
  FLOATL  eDy;
} XFORML, *PXFORML;

typedef struct _XFORMOBJ {
  ULONG  ulReserved;
} XFORMOBJ;

/* XLATEOBJ.flXlate constants */
#define XO_TRIVIAL                        0x00000001
#define XO_TABLE                          0x00000002
#define XO_TO_MONO                        0x00000004
#define XO_FROM_CMYK                      0x00000008
#define XO_DEVICE_ICM                     0x00000010
#define XO_HOST_ICM                       0x00000020

typedef struct _XLATEOBJ {
  ULONG  iUniq;
  FLONG  flXlate;
  USHORT  iSrcType;
  USHORT  iDstType;
  ULONG  cEntries;
  ULONG  *pulXlate;
} XLATEOBJ;

/* WNDOBJCHANGEPROC.fl constants */
#define WOC_RGN_CLIENT_DELTA              0x00000001
#define WOC_RGN_CLIENT                    0x00000002
#define WOC_RGN_SURFACE_DELTA             0x00000004
#define WOC_RGN_SURFACE                   0x00000008
#define WOC_CHANGED                       0x00000010
#define WOC_DELETE                        0x00000020
#define WOC_DRAWN                         0x00000040
#define WOC_SPRITE_OVERLAP                0x00000080
#define WOC_SPRITE_NO_OVERLAP             0x00000100
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define WOC_RGN_SPRITE                    0x00000200
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

typedef VOID (APIENTRY CALLBACK *WNDOBJCHANGEPROC)(
    _In_ WNDOBJ *pwo,
    _In_ FLONG fl);


ENGAPI
HANDLE
APIENTRY
BRUSHOBJ_hGetColorTransform(
    _In_ BRUSHOBJ *pbo);

_Ret_opt_bytecount_(cj)
ENGAPI
PVOID
APIENTRY
BRUSHOBJ_pvAllocRbrush(
    _In_ BRUSHOBJ *pbo,
    _In_ ULONG cj);

ENGAPI
PVOID
APIENTRY
BRUSHOBJ_pvGetRbrush(
    _In_ BRUSHOBJ *pbo);

ENGAPI
ULONG
APIENTRY
BRUSHOBJ_ulGetBrushColor(
    _In_ BRUSHOBJ *pbo);

ENGAPI
BOOL
APIENTRY
CLIPOBJ_bEnum(
    _In_ CLIPOBJ *pco,
    _In_ ULONG cj,
    _Out_bytecap_(cj) ULONG *pul);

/* CLIPOBJ_cEnumStart.iType constants */
#define CT_RECTANGLES                     0L

/* CLIPOBJ_cEnumStart.iDirection constants */
#define CD_RIGHTDOWN                      0x00000000
#define CD_LEFTDOWN                       0x00000001
#define CD_LEFTWARDS                      0x00000001
#define CD_RIGHTUP                        0x00000002
#define CD_UPWARDS                        0x00000002
#define CD_LEFTUP                         0x00000003
#define CD_ANY                            0x00000004

ENGAPI
ULONG
APIENTRY
CLIPOBJ_cEnumStart(
    _Inout_ CLIPOBJ *pco,
    _In_ BOOL bAll,
    _In_ ULONG iType,
    _In_ ULONG iDirection,
    _In_ ULONG cLimit);

ENGAPI
PATHOBJ*
APIENTRY
CLIPOBJ_ppoGetPath(
    _In_ CLIPOBJ *pco);

#if (NTDDI_VERSION >= NTDDI_VISTA)
ENGAPI
HANDLE
APIENTRY
CLIPOBJ_GetRgn(
    _In_ CLIPOBJ* pco);
#endif

ENGAPI
VOID
APIENTRY
EngAcquireSemaphore(
    _Inout_ HSEMAPHORE hsem);

#define FL_ZERO_MEMORY                    0x00000001
#define FL_NONPAGED_MEMORY                0x00000002
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define FL_NON_SESSION                    0x00000004
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#ifdef USERMODE_DRIVER

#define EngAllocMem(fl, cj, tag) ((PVOID)GlobalAlloc(((fl) & FL_ZERO_MEMORY) ? GPTR : GMEM_FIXED, cj))
#define EngAllocPrivateUserMem(psl, cj, tag) ((PVOID)GlobalAlloc(GMEM_FIXED, cj))
#define EngAllocUserMem(cj, tag) ((PVOID)GlobalAlloc(GMEM_FIXED, cj))

#else /* !USERMODE_DRIVER */

_Must_inspect_result_
_When_(fl & FL_ZERO_MEMORY, _Ret_opt_bytecount_(cjMemSize))
_When_(!(fl & FL_ZERO_MEMORY), _Ret_opt_bytecap_(cjMemSize))
ENGAPI
PVOID
APIENTRY
EngAllocMem(
    _In_ ULONG fl,
    _In_ ULONG cjMemSize,
    _In_ ULONG ulTag);

_Must_inspect_result_
_Ret_opt_bytecount_(cjMemSize)
ENGAPI
PVOID
APIENTRY
EngAllocPrivateUserMem(
    _In_ PDD_SURFACE_LOCAL psl,
    _In_ SIZE_T cjMemSize,
    _In_ ULONG ulTag);

_Must_inspect_result_
_Ret_opt_bytecount_(cjMemSize)
ENGAPI
PVOID
APIENTRY
EngAllocUserMem(
    _In_ SIZE_T cjMemSize,
    _In_ ULONG ulTag);

#endif /* !USERMODE_DRIVER */

ENGAPI
BOOL
APIENTRY
EngAlphaBlend(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_ BLENDOBJ *pBlendObj);

/* EngAssociateSurface.flHooks constants */
#define HOOK_BITBLT                       0x00000001
#define HOOK_STRETCHBLT                   0x00000002
#define HOOK_PLGBLT                       0x00000004
#define HOOK_TEXTOUT                      0x00000008
#define HOOK_PAINT                        0x00000010
#define HOOK_STROKEPATH                   0x00000020
#define HOOK_FILLPATH                     0x00000040
#define HOOK_STROKEANDFILLPATH            0x00000080
#define HOOK_LINETO                       0x00000100
#define HOOK_COPYBITS                     0x00000400
#define HOOK_MOVEPANNING                  0x00000800
#define HOOK_SYNCHRONIZE                  0x00001000
#define HOOK_STRETCHBLTROP                0x00002000
#define HOOK_SYNCHRONIZEACCESS            0x00004000
#define HOOK_TRANSPARENTBLT               0x00008000
#define HOOK_ALPHABLEND                   0x00010000
#define HOOK_GRADIENTFILL                 0x00020000
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define HOOK_FLAGS                        0x0003b5ef
#else
#define HOOK_FLAGS                        0x0003b5ff
#endif

ENGAPI
BOOL
APIENTRY
EngAssociateSurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks);

ENGAPI
BOOL
APIENTRY
EngBitBlt(
    _Inout_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclTrg,
    _In_opt_ POINTL *pptlSrc,
    _In_opt_ POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ POINTL *pptlBrush,
    _In_ ROP4 rop4);

ENGAPI
BOOL
APIENTRY
EngCheckAbort(
    _In_ SURFOBJ *pso);

ENGAPI
VOID
APIENTRY
EngClearEvent(
    _In_ PEVENT pEvent);

_Success_(return != 0)
ENGAPI
FD_GLYPHSET*
APIENTRY
EngComputeGlyphSet(
    _In_ INT nCodePage,
    _In_ INT nFirstChar,
    _In_ INT cChars);

/* EngControlSprites.fl constants */
#define ECS_TEARDOWN                      0x00000001
#define ECS_REDRAW                        0x00000002

ENGAPI
BOOL
APIENTRY
EngControlSprites(
    _Inout_ WNDOBJ *pwo,
    _In_ FLONG fl);

ENGAPI
BOOL
APIENTRY
EngCopyBits(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ __in_data_source(USER_MODE) RECTL *prclDest,
    _In_ __in_data_source(USER_MODE) POINTL *pptlSrc);

ENGAPI
HBITMAP
APIENTRY
EngCreateBitmap(
    _In_ SIZEL sizl,
    _In_ LONG lWidth,
    _In_ ULONG iFormat,
    _In_ FLONG fl,
    _In_opt_ PVOID pvBits);

ENGAPI
CLIPOBJ*
APIENTRY
EngCreateClip(VOID);

_Must_inspect_result_
ENGAPI
HBITMAP
APIENTRY
EngCreateDeviceBitmap(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat);

_Must_inspect_result_
ENGAPI
HSURF
APIENTRY
EngCreateDeviceSurface(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat);

_Must_inspect_result_
ENGAPI
HDRVOBJ
APIENTRY
EngCreateDriverObj(
    _In_ PVOID pvObj,
    _In_opt_ FREEOBJPROC pFreeObjProc,
    _In_ HDEV hdev);

_Must_inspect_result_
_Success_(return != FALSE)
ENGAPI
BOOL
APIENTRY
EngCreateEvent(
    _Outptr_ PEVENT *ppEvent);

/* EngCreatePalette.iMode constants */
#define PAL_INDEXED                       0x00000001
#define PAL_BITFIELDS                     0x00000002
#define PAL_RGB                           0x00000004
#define PAL_BGR                           0x00000008
#define PAL_CMYK                          0x00000010

_Must_inspect_result_
ENGAPI
HPALETTE
APIENTRY
EngCreatePalette(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_ ULONG *pulColors,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue);

ENGAPI
PATHOBJ*
APIENTRY
EngCreatePath(VOID);

ENGAPI
HSEMAPHORE
APIENTRY
EngCreateSemaphore(VOID);

/* EngCreateWnd.fl constants */
#define WO_RGN_CLIENT_DELTA               0x00000001
#define WO_RGN_CLIENT                     0x00000002
#define WO_RGN_SURFACE_DELTA              0x00000004
#define WO_RGN_SURFACE                    0x00000008
#define WO_RGN_UPDATE_ALL                 0x00000010
#define WO_RGN_WINDOW                     0x00000020
#define WO_DRAW_NOTIFY                    0x00000040
#define WO_SPRITE_NOTIFY                  0x00000080
#define WO_RGN_DESKTOP_COORD              0x00000100
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define WO_RGN_SPRITE                     0x00000200
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

_Must_inspect_result_
ENGAPI
WNDOBJ*
APIENTRY
EngCreateWnd(
    _In_ SURFOBJ *pso,
    _In_ HWND hwnd,
    _In_ WNDOBJCHANGEPROC pfn,
    _In_ FLONG fl,
    _In_ INT iPixelFormat);

_Analysis_noreturn_
ENGAPI
VOID
APIENTRY
EngDebugBreak(VOID);

ENGAPI
VOID
APIENTRY
EngDebugPrint(
    _In_z_ PCHAR StandardPrefix,
    _In_z_ PCHAR DebugMessage,
    _In_ va_list ap);

ENGAPI
VOID
APIENTRY
EngDeleteClip(
    _In_ _Post_ptr_invalid_ CLIPOBJ *pco);

ENGAPI
BOOL
APIENTRY
EngDeleteDriverObj(
    _In_ _Post_ptr_invalid_ HDRVOBJ hdo,
    _In_ BOOL bCallBack,
    _In_ BOOL bLocked);

ENGAPI
BOOL
APIENTRY
EngDeleteEvent(
    _In_ _Post_ptr_invalid_ PEVENT pEvent);

ENGAPI
BOOL
APIENTRY
EngDeleteFile(
    _In_ LPWSTR pwszFileName);

ENGAPI
BOOL
APIENTRY
EngDeletePalette(
    _In_ _Post_ptr_invalid_ HPALETTE hpal);

ENGAPI
VOID
APIENTRY
EngDeletePath(
    _In_ _Post_ptr_invalid_ PATHOBJ *ppo);

ENGAPI
VOID
APIENTRY
EngDeleteSafeSemaphore(
    _Inout_ ENGSAFESEMAPHORE *pssem);

ENGAPI
VOID
APIENTRY
EngDeleteSemaphore(
    _In_ _Post_ptr_invalid_ HSEMAPHORE hsem);

ENGAPI
BOOL
APIENTRY
EngDeleteSurface(
    _In_ _Post_ptr_invalid_ HSURF hsurf);

ENGAPI
VOID
APIENTRY
EngDeleteWnd(
    _In_ _Post_ptr_invalid_ WNDOBJ *pwo);

_Success_(return==0)
ENGAPI
DWORD
APIENTRY
EngDeviceIoControl(
    _In_ HANDLE hDevice,
    _In_ DWORD dwIoControlCode,
    _In_reads_bytes_opt_(cjInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD cjInBufferSize,
    _Out_writes_bytes_opt_(cjOutBufferSize) LPVOID lpOutBuffer,
    _In_ DWORD cjOutBufferSize,
    _Out_ LPDWORD lpBytesReturned);

#define DM_DEFAULT                        0x00000001
#define DM_MONOCHROME                     0x00000002

ENGAPI
ULONG
APIENTRY
EngDitherColor(
    _In_ HDEV hdev,
    _In_ ULONG iMode,
    _In_ ULONG rgb,
    _When_(iMode == DM_DEFAULT, _Out_writes_bytes_(16*8))
    _When_(iMode == DM_MONOCHROME, _Out_writes_bytes_(8))
        ULONG *pul);

/* Obsolete in Windows 2000 and later */
ENGAPI
HRESULT
APIENTRY
EngDxIoctl(
    _In_ ULONG ulIoctl,
    _Inout_ PVOID pBuffer,
    _In_ ULONG ulBufferSize);

#ifdef USERMODE_DRIVER
#define EngEnumForms        EnumForms
#else /* !USERMODE_DRIVER */
ENGAPI
BOOL
APIENTRY
EngEnumForms(
    _In_ HANDLE hPrinter,
    _In_ DWORD Level,
    _Out_writes_bytes_opt_(cbBuf) LPBYTE pForm,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);
#endif /* !USERMODE_DRIVER */

ENGAPI
BOOL
APIENTRY
EngEraseSurface(
    _In_ SURFOBJ *pso,
    _In_ RECTL *prcl,
    _In_ ULONG iColor);

ENGAPI
BOOL
APIENTRY
EngFillPath(
    _Inout_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions);

#ifdef USERMODE_DRIVER
#define EngFindImageProcAddress(h, procname) ((PVOID) GetProcAddress(h, procname))
#else /* !USERMODE_DRIVER */
ENGAPI
PVOID
APIENTRY
EngFindImageProcAddress(
    _In_ HANDLE hModule,
    _In_ LPSTR lpProcName);
#endif /* !USERMODE_DRIVER */

ENGAPI
PVOID
APIENTRY
EngFindResource(
    _In_ HANDLE h,
    _In_ INT iName,
    _In_ INT iType,
    _Out_ PULONG pulSize);

_Must_inspect_result_
_Ret_opt_bytecap_(cjSize)
ENGAPI
PVOID
APIENTRY
EngFntCacheAlloc(
    _In_ ULONG ulFastCheckSum,
    _In_ ULONG cjSize);

/* EngFntCacheFault.iFaultMode constants */
#define ENG_FNT_CACHE_READ_FAULT          0x00000001
#define ENG_FNT_CACHE_WRITE_FAULT         0x00000002

ENGAPI
VOID
APIENTRY
EngFntCacheFault(
    _In_ ULONG ulFastCheckSum,
    _In_ ULONG iFaultMode);

ENGAPI
PVOID
APIENTRY
EngFntCacheLookUp(
    _In_ ULONG FastCheckSum,
    _Out_ ULONG *pulSize);

#ifdef USERMODE_DRIVER

#define EngFreeMem(p)               GlobalFree((HGLOBAL) (p))
#define EngFreePrivateUserMem( psl, p)        GlobalFree((HGLOBAL) (p))
#define EngFreeUserMem(p)           GlobalFree((HGLOBAL) (p))

#else /* !USERMODE_DRIVER */

ENGAPI
VOID
APIENTRY
EngFreeMem(
    _In_ _Post_ptr_invalid_ PVOID pv);

ENGAPI
VOID
APIENTRY
EngFreePrivateUserMem(
    _In_ PDD_SURFACE_LOCAL psl,
    _In_ _Post_ptr_invalid_ PVOID  pv);

ENGAPI
VOID
APIENTRY
EngFreeUserMem(
    _In_ _Post_ptr_invalid_ PVOID pv);

#endif /* !USERMODE_DRIVER */

ENGAPI
VOID
APIENTRY
EngFreeModule(
    _In_ HANDLE h);


ENGAPI
VOID
APIENTRY
EngGetCurrentCodePage(
    _Out_ PUSHORT OemCodePage,
    _Out_ PUSHORT AnsiCodePage);

ENGAPI
HANDLE
APIENTRY
EngGetCurrentProcessId(VOID);

ENGAPI
HANDLE
APIENTRY
EngGetCurrentThreadId(VOID);

_Must_inspect_result_ _Ret_z_
ENGAPI
LPWSTR
APIENTRY
EngGetDriverName(
    _In_ HDEV hdev);

ENGAPI
BOOL
APIENTRY
EngGetFileChangeTime(
    _In_ HANDLE h,
    _Out_ LARGE_INTEGER *pChangeTime);

ENGAPI
BOOL
APIENTRY
EngGetFilePath(
    _In_ HANDLE h,
    _Out_ WCHAR (*pDest)[MAX_PATH+1]);

#ifdef USERMODE_DRIVER
#define EngGetForm GetForm
#define EngGetLastError GetLastError
#define EngGetPrinter GetPrinter
#define EngGetPrinterData GetPrinterData
#else /* !USERMODE_DRIVER */
ENGAPI
BOOL
APIENTRY
EngGetForm(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pFormName,
    _In_ DWORD Level,
    _In_reads_bytes_opt_(cbBuf) LPBYTE pForm,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded);

ENGAPI
ULONG
APIENTRY
EngGetLastError(VOID);

ENGAPI
BOOL
APIENTRY
EngGetPrinter(
    _In_ HANDLE hPrinter,
    _In_ DWORD dwLevel,
    _Out_writes_bytes_opt_(cbBuf) LPBYTE pPrinter,
    _In_ DWORD  cbBuf,
    _Out_ LPDWORD pcbNeeded);

ENGAPI
DWORD
APIENTRY
EngGetPrinterData(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pValueName,
    _Out_opt_ LPDWORD pType,
    _Out_writes_bytes_opt_(cjSize) LPBYTE pData,
    _In_ DWORD cjSize,
    _Out_ LPDWORD pcjNeeded);
#endif /* !USERMODE_DRIVER */

ENGAPI
LPWSTR
APIENTRY
EngGetPrinterDataFileName(
    _In_ HDEV hdev);

#ifdef USERMODE_DRIVER
#define EngGetPrinterDriver GetPrinterDriver
#else /* !USERMODE_DRIVER */
ENGAPI
BOOL
APIENTRY
EngGetPrinterDriver(
    _In_ HANDLE hPrinter,
    _In_opt_ LPWSTR pEnvironment,
    _In_ DWORD dwLevel,
    _Out_writes_bytes_opt_(cjBufSize) BYTE *lpbDrvInfo,
    _In_ DWORD cjBufSize,
    _Out_ DWORD *pcjNeeded);
#endif /* !USERMODE_DRIVER */

ENGAPI
HANDLE
APIENTRY
EngGetProcessHandle(VOID);

ENGAPI
BOOL
APIENTRY
EngGetType1FontList(
    _In_ HDEV hdev,
    _Out_writes_bytes_opt_(cjType1Buffer) TYPE1_FONT *pType1Buffer,
    _In_ ULONG cjType1Buffer,
    _Out_ PULONG pulLocalFonts,
    _Out_ PULONG pulRemoteFonts,
    _Out_ LARGE_INTEGER *pLastModified);

ENGAPI
BOOL
APIENTRY
EngGradientFill(
    _Inout_ SURFOBJ *psoDest,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ TRIVERTEX *pVertex,
    _In_ ULONG nVertex,
    _In_ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ RECTL *prclExtents,
    _In_ POINTL *pptlDitherOrg,
    _In_ ULONG ulMode);

/* EngHangNotification return values */
#define EHN_RESTORED                      0x00000000
#define EHN_ERROR                         0x00000001

ENGAPI
ULONG
APIENTRY
EngHangNotification(
    _In_ HDEV hDev,
    _Reserved_ PVOID Reserved);

ENGAPI
BOOL
APIENTRY
EngInitializeSafeSemaphore(
    _Out_ ENGSAFESEMAPHORE *pssem);

ENGAPI
BOOL
APIENTRY
EngIsSemaphoreOwned(
    _In_ HSEMAPHORE hsem);

ENGAPI
BOOL
APIENTRY
EngIsSemaphoreOwnedByCurrentThread(
    _In_ HSEMAPHORE hsem);

ENGAPI
BOOL
APIENTRY
EngLineTo(
    _Inout_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_opt_ RECTL *prclBounds,
    _In_ MIX mix);

#ifdef USERMODE_DRIVER
#define EngLoadImage(pwszDriver) ((HANDLE)LoadLibraryW(pwszDriver))
#else /* !USERMODE_DRIVER */
ENGAPI
HANDLE
APIENTRY
EngLoadImage(
    _In_ LPWSTR pwszDriver);
#endif /* !USERMODE_DRIVER */

ENGAPI
HANDLE
APIENTRY
EngLoadModule(
    _In_ LPWSTR pwsz);

ENGAPI
HANDLE
APIENTRY
EngLoadModuleForWrite(
    _In_ LPWSTR pwsz,
    _In_ ULONG cjSizeOfModule);

ENGAPI
PDD_SURFACE_LOCAL
APIENTRY
EngLockDirectDrawSurface(
    _In_ HANDLE hSurface);

ENGAPI
DRIVEROBJ*
APIENTRY
EngLockDriverObj(
    _In_ HDRVOBJ hdo);

ENGAPI
SURFOBJ*
APIENTRY
EngLockSurface(
    _In_ HSURF hsurf);

ENGAPI
BOOL
APIENTRY
EngLpkInstalled(VOID);

ENGAPI
PEVENT
APIENTRY
EngMapEvent(
    _In_ HDEV hDev,
    _In_ HANDLE hUserObject,
    _Reserved_ PVOID Reserved1,
    _Reserved_ PVOID Reserved2,
    _Reserved_ PVOID Reserved3);

_Success_(return != 0)
_When_(cjSize != 0, _At_(return, _Out_writes_bytes_(cjSize)))
ENGAPI
PVOID
APIENTRY
EngMapFile(
    _In_ LPWSTR pwsz,
    _In_ ULONG cjSize,
    _Out_ ULONG_PTR *piFile);

__drv_preferredFunction("EngMapFontFileFD", "Obsolete")
ENGAPI
BOOL
APIENTRY
EngMapFontFile(
    _In_ ULONG_PTR iFile,
    _Outptr_result_bytebuffer_(*pcjBuf) PULONG *ppjBuf,
    _Out_ ULONG *pcjBuf);

ENGAPI
BOOL
APIENTRY
EngMapFontFileFD(
    _In_ ULONG_PTR iFile,
    _Outptr_result_bytebuffer_(*pcjBuf) PULONG *ppjBuf,
    _Out_ ULONG *pcjBuf);

ENGAPI
PVOID
APIENTRY
EngMapModule(
    _In_ HANDLE h,
    _Out_ PULONG pSize);

ENGAPI
BOOL
APIENTRY
EngMarkBandingSurface(
    _In_ HSURF hsurf);

/* EngModifySurface.flSurface constants */
#define MS_NOTSYSTEMMEMORY                0x00000001
#define MS_SHAREDACCESS                   0x00000002
#define MS_CDDDEVICEBITMAP                0x00000004

ENGAPI
BOOL
APIENTRY
EngModifySurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks,
    _In_ FLONG flSurface,
    _In_ DHSURF dhsurf,
    _In_ PVOID pvScan0,
    _In_ LONG lDelta,
    _Reserved_ PVOID pvReserved);

ENGAPI
VOID
APIENTRY
EngMovePointer(
    _In_ SURFOBJ *pso,
    _In_ LONG x,
    _In_ LONG y,
    _In_ RECTL *prcl);

#ifdef USERMODE_DRIVER
#define EngMulDiv MulDiv
#else /* !USERMODE_DRIVER */
ENGAPI
INT
APIENTRY
EngMulDiv(
    _In_ INT a,
    _In_ INT b,
    _In_ INT c);
#endif /* !USERMODE_DRIVER */

ENGAPI
VOID
APIENTRY
EngMultiByteToUnicodeN(
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) LPWSTR UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInMultiByteString) PCHAR MultiByteString,
    _In_ ULONG BytesInMultiByteString);

ENGAPI
INT
APIENTRY
EngMultiByteToWideChar(
    _In_ UINT CodePage,
    _Out_writes_bytes_opt_(cjWideCharString) LPWSTR WideCharString,
    _In_ INT cjWideCharString,
    _In_reads_bytes_opt_(cjMultiByteString) LPSTR MultiByteString,
    _In_ INT cjMultiByteString);

ENGAPI
BOOL
APIENTRY
EngPaint(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ __in_data_source(USER_MODE) MIX mix);

ENGAPI
BOOL
APIENTRY
EngPlgBlt(
    _In_ SURFOBJ *psoTrg,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMsk,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlBrushOrg,
    _In_ POINTFIX *pptfx,
    _In_ RECTL *prcl,
    _When_(psoMsk, _In_) POINTL *pptl,
    _In_ __in_data_source(USER_MODE) ULONG iMode);

ENGAPI
VOID
APIENTRY
EngProbeForRead(
    _In_reads_bytes_(Length) PVOID Address,
    _In_ SIZE_T Length,
    _In_ ULONG Alignment);

ENGAPI
VOID
APIENTRY
EngProbeForReadAndWrite(
    _Inout_updates_bytes_(Length) PVOID Address,
    _In_ SIZE_T Length,
    _In_ ULONG Alignment);

typedef enum _ENG_DEVICE_ATTRIBUTE {
  QDA_RESERVED = 0,
  QDA_ACCELERATION_LEVEL
} ENG_DEVICE_ATTRIBUTE;

ENGAPI
BOOL
APIENTRY
EngQueryDeviceAttribute(
    _In_ HDEV hdev,
    _In_ ENG_DEVICE_ATTRIBUTE devAttr,
    _In_reads_bytes_(cjInSize) PVOID pvIn,
    _In_ ULONG cjInSize,
    _Out_writes_bytes_(cjOutSize) PVOID pvOut,
    _In_ ULONG cjOutSize);

/* Obsolete in Windows 2000 and later */
DECLSPEC_DEPRECATED_DDK
BOOL APIENTRY
EngQueryEMFInfo(
    _In_ HDEV hdev,
    _Out_ EMFINFO *pEMFInfo);

ENGAPI
LARGE_INTEGER
APIENTRY
EngQueryFileTimeStamp(
    _In_ LPWSTR pwsz);

ENGAPI
VOID
APIENTRY
EngQueryLocalTime(
    _Out_ PENG_TIME_FIELDS ptf);

ENGAPI
ULONG
APIENTRY
EngQueryPalette(
    _In_ HPALETTE hPal,
    _Out_ ULONG *piMode,
    _In_ ULONG cColors,
    _Out_writes_opt_(cColors) ULONG *pulColors);

ENGAPI
VOID
APIENTRY
EngQueryPerformanceCounter(
    _Out_ LONGLONG *pPerformanceCount);

ENGAPI
VOID
APIENTRY
EngQueryPerformanceFrequency(
    _Out_ LONGLONG *pFrequency);

typedef enum _ENG_SYSTEM_ATTRIBUTE {
  EngProcessorFeature = 1,
  EngNumberOfProcessors,
  EngOptimumAvailableUserMemory,
  EngOptimumAvailableSystemMemory,
} ENG_SYSTEM_ATTRIBUTE;

#define QSA_MMX                           0x00000100
#define QSA_SSE                           0x00002000
#define QSA_3DNOW                         0x00004000
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define QSA_SSE1                          QSA_SSE
#define QSA_SSE2                          0x00010000
#define QSA_SSE3                          0x00080000
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

_Check_return_
_Success_(return)
ENGAPI
BOOL
APIENTRY
EngQuerySystemAttribute(
    _In_ ENG_SYSTEM_ATTRIBUTE CapNum,
    _Out_ PDWORD pCapability);

ENGAPI
LONG
APIENTRY
EngReadStateEvent(
    _In_ PEVENT pEvent);

ENGAPI
VOID
APIENTRY
EngReleaseSemaphore(
    _In_ HSEMAPHORE hsem);

#if defined(_M_AMD64) && (NTDDI_VERSION >= NTDDI_VISTA)

_Check_return_
_Success_(return)
_Kernel_float_restored_
_At_(*pBuffer, _Kernel_requires_resource_held_(EngFloatState)
               _Kernel_releases_resource_(EngFloatState))
ENGAPI
BOOL
APIENTRY
EngRestoreFloatingPointState(
    _In_reads_(_Inexpressible_(statesize))
    PVOID pBuffer)
{
    UNREFERENCED_PARAMETER(pBuffer);
    return TRUE;
}

_Check_return_
_Success_(((pBuffer != NULL && cjBufferSize != 0) && return == 1) ||
          ((pBuffer == NULL || cjBufferSize == 0) && return > 0))
_When_(pBuffer != NULL && cjBufferSize != 0 && return == 1, _Kernel_float_saved_
    _At_(*pBuffer, _Post_valid_ _Kernel_acquires_resource_(EngFloatState)))
_On_failure_(_Post_satisfies_(return == 0))
ENGAPI
ULONG
APIENTRY
EngSaveFloatingPointState(
    _At_(*pBuffer, _Kernel_requires_resource_not_held_(EngFloatState))
    _Out_writes_bytes_opt_(cjBufferSize) PVOID pBuffer,
    _Inout_ ULONG cjBufferSize)
{
    return ((((pBuffer) == NULL) || ((cjBufferSize) == 0)) ? 8 : TRUE);
}

#else /* !(defined(_M_AMD64) && (NTDDI_VERSION >= NTDDI_VISTA)) */

_Check_return_
_Success_(return)
_Kernel_float_restored_
_At_(*pBuffer, _Kernel_requires_resource_held_(EngFloatState)
               _Kernel_releases_resource_(EngFloatState))
ENGAPI
BOOL
APIENTRY
EngRestoreFloatingPointState(
    _In_reads_(_Inexpressible_(statesize)) PVOID pBuffer);

_Check_return_
_Success_(((pBuffer != NULL && cjBufferSize != 0) && return == 1) ||
          ((pBuffer == NULL || cjBufferSize == 0) && return > 0))
_When_(pBuffer != NULL && cjBufferSize != 0 && return == 1, _Kernel_float_saved_
    _At_(*pBuffer, _Post_valid_ _Kernel_acquires_resource_(EngFloatState)))
_On_failure_(_Post_satisfies_(return == 0))
ENGAPI
ULONG
APIENTRY
EngSaveFloatingPointState(
    _At_(*pBuffer, _Kernel_requires_resource_not_held_(EngFloatState))
    _Out_writes_bytes_opt_(cjBufferSize) PVOID pBuffer,
    _Inout_ ULONG cjBufferSize);


#endif /* defined(_M_AMD64) && (NTDDI_VERSION >= NTDDI_VISTA) */

ENGAPI
HANDLE
APIENTRY
EngSecureMem(
    _In_reads_bytes_(cjLength) PVOID Address,
    _In_ ULONG cjLength);

ENGAPI
LONG
APIENTRY
EngSetEvent(
    _In_ PEVENT pEvent);

#ifdef USERMODE_DRIVER
#define EngSetLastError SetLastError
#else /* !USERMODE_DRIVER */
ENGAPI
VOID
APIENTRY
EngSetLastError(
    _In_ ULONG iError);
#endif /* !USERMODE_DRIVER */

ENGAPI
ULONG
APIENTRY
EngSetPointerShape(
    _In_ SURFOBJ *pso,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ SURFOBJ *psoColor,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ LONG xHot,
    _In_ LONG yHot,
    _In_ LONG x,
    _In_ LONG y,
    _In_ RECTL *prcl,
    _In_ FLONG fl);

__drv_preferredFunction("(see documentation)", "Obsolete, always returns false. ")
ENGAPI
BOOL
APIENTRY
EngSetPointerTag(
    _In_ HDEV hdev,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ SURFOBJ *psoColor,
    _Reserved_ XLATEOBJ *pxlo,
    _In_ FLONG fl);

#ifdef USERMODE_DRIVER
#define EngSetPrinterData   SetPrinterData
#else /* !USERMODE_DRIVER */
ENGAPI
DWORD
APIENTRY
EngSetPrinterData(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pType,
    _In_ DWORD dwType,
    _In_reads_bytes_(cjPrinterData) LPBYTE lpbPrinterData,
    _In_ DWORD cjPrinterData);
#endif /* !USERMODE_DRIVER */

typedef int (CDECL *SORTCOMP)(const void *pv1, const void *pv2);

ENGAPI
VOID
APIENTRY
EngSort(
    _Inout_updates_bytes_(c * cjElem) PBYTE pjBuf,
    _In_ ULONG c,
    _In_ ULONG cjElem,
    _In_ SORTCOMP pfnComp);

ENGAPI
BOOL
APIENTRY
EngStretchBlt(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlHTOrg,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_ ULONG iMode);

ENGAPI
BOOL
APIENTRY
EngStretchBltROP(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlHTOrg,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_ ULONG iMode,
    _In_opt_ BRUSHOBJ *pbo,
    _In_ DWORD rop4);

ENGAPI
BOOL
APIENTRY
EngStrokeAndFillPath(
    _Inout_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pboStroke,
    _In_ LINEATTRS *plineattrs,
    _In_ BRUSHOBJ *pboFill,
    _In_ POINTL *pptlBrushOrg,
    _In_ __in_data_source(USER_MODE) MIX mixFill,
    _In_ __in_data_source(USER_MODE) FLONG flOptions);

ENGAPI
BOOL
APIENTRY
EngStrokePath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ LINEATTRS *plineattrs,
    _In_ MIX mix);

ENGAPI
BOOL
APIENTRY
EngTextOut(
    _In_ SURFOBJ *pso,
    _In_ STROBJ *pstro,
    _In_ FONTOBJ *pfo,
    _In_ CLIPOBJ *pco,
    _Null_ RECTL *prclExtra,
    _In_opt_ RECTL *prclOpaque,
    _In_ BRUSHOBJ *pboFore,
    _In_ BRUSHOBJ *pboOpaque,
    _In_ POINTL *pptlOrg,
    _In_ MIX mix);

ENGAPI
BOOL
APIENTRY
EngTransparentBlt(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ RECTL *prclSrc,
    _In_ ULONG iTransColor,
    _In_ ULONG ulReserved);

ENGAPI
VOID
APIENTRY
EngUnicodeToMultiByteN(
    _Out_writes_bytes_to_(cjMultiByteBuf, *pcjMultiByteString) PCHAR pchMultiByteString,
    _In_ ULONG cjMultiByteBuf,
    _Out_opt_ PULONG pcjMultiByteString,
    _In_reads_bytes_(cjUnicodeString) PWSTR pwszUnicodeString,
    _In_ ULONG cjUnicodeString);

#ifdef USERMODE_DRIVER
#define EngUnloadImage(h) FreeLibrary((HMODULE) (h))
#else /* !USERMODE_DRIVER */
ENGAPI
VOID
APIENTRY
EngUnloadImage(
    _In_ HANDLE hModule);
#endif /* !USERMODE_DRIVER */

ENGAPI
BOOL
APIENTRY
EngUnlockDirectDrawSurface(
    _In_ PDD_SURFACE_LOCAL pSurface);

ENGAPI
BOOL
APIENTRY
EngUnlockDriverObj(
    _In_ _Post_ptr_invalid_ HDRVOBJ hdo);

ENGAPI
VOID
APIENTRY
EngUnlockSurface(
    _In_ _Post_ptr_invalid_ SURFOBJ *pso);

ENGAPI
BOOL
APIENTRY
EngUnmapEvent(
    _In_ PEVENT pEvent);

ENGAPI
BOOL
APIENTRY
EngUnmapFile(
    _In_ ULONG_PTR iFile);

__drv_preferredFunction("EngUnmapFontFileFD", "Obsolete")
ENGAPI
VOID
APIENTRY
EngUnmapFontFile(
    _In_ ULONG_PTR iFile);

ENGAPI
VOID
APIENTRY
EngUnmapFontFileFD(
    _In_ ULONG_PTR iFile);

ENGAPI
VOID
APIENTRY
EngUnsecureMem(
    _In_ HANDLE hSecure);

ENGAPI
BOOL
APIENTRY
EngWaitForSingleObject(
    _In_ PEVENT pEvent,
    _In_opt_ PLARGE_INTEGER pTimeOut);

ENGAPI
INT
APIENTRY
EngWideCharToMultiByte(
    _In_ UINT CodePage,
    _In_reads_bytes_opt_(cjWideCharString) LPWSTR pwszWideCharString,
    _In_ INT cjWideCharString,
    _Out_z_bytecap_(cjMultiByteString) LPSTR pszMultiByteString,
    _In_ INT cjMultiByteString);

#ifdef USERMODE_DRIVER
#define EngWritePrinter WritePrinter
#else /* !USERMODE_DRIVER */
ENGAPI
BOOL
APIENTRY
EngWritePrinter(
    _In_ HANDLE hPrinter,
    _In_reads_bytes_(cjBuf) LPVOID pvBuf,
    _In_ DWORD cjBuf,
    _Out_ LPDWORD pcWritten);
#endif /* !USERMODE_DRIVER */

#if defined(_X86_) && !defined(USERMODE_DRIVER)
ENGAPI
VOID
APIENTRY
FLOATOBJ_Add(
    _Inout_ PFLOATOBJ pf,
    _In_ PFLOATOBJ pf1);

ENGAPI
VOID
APIENTRY
FLOATOBJ_AddFloat(
    _Inout_ PFLOATOBJ pf,
    _In_ FLOATL f);

ENGAPI
VOID
APIENTRY
FLOATOBJ_AddLong(
    _Inout_ PFLOATOBJ pf,
    _In_ LONG l);

ENGAPI
VOID
APIENTRY
FLOATOBJ_Div(
    _Inout_ PFLOATOBJ pf,
    _In_ PFLOATOBJ pf1);

ENGAPI
VOID
APIENTRY
FLOATOBJ_DivFloat(
    _Inout_ PFLOATOBJ pf,
    _In_ FLOATL f);

ENGAPI
VOID
APIENTRY
FLOATOBJ_DivLong(
    _Inout_ PFLOATOBJ pf,
    _In_ LONG l);

ENGAPI
BOOL
APIENTRY
FLOATOBJ_Equal(
    _In_ PFLOATOBJ pf,
    _In_ PFLOATOBJ pf1);

ENGAPI
BOOL
APIENTRY
FLOATOBJ_EqualLong(
    _In_ PFLOATOBJ pf,
    _In_ LONG l);

ENGAPI
LONG
APIENTRY
FLOATOBJ_GetFloat(
    _In_ PFLOATOBJ pf);

ENGAPI
LONG
APIENTRY
FLOATOBJ_GetLong(
    _In_ PFLOATOBJ pf);

ENGAPI
BOOL
APIENTRY
FLOATOBJ_GreaterThan(
    _In_ PFLOATOBJ pf,
    _In_ PFLOATOBJ pf1);

ENGAPI
BOOL
APIENTRY
FLOATOBJ_GreaterThanLong(
    _In_ PFLOATOBJ pf,
    _In_ LONG l);

ENGAPI
BOOL
APIENTRY
FLOATOBJ_LessThan(
    _In_ PFLOATOBJ pf,
    _In_ PFLOATOBJ pf1);

ENGAPI
BOOL
APIENTRY
FLOATOBJ_LessThanLong(
    _In_ PFLOATOBJ pf,
    _In_ LONG l);

ENGAPI
VOID
APIENTRY
FLOATOBJ_Mul(
    _Inout_ PFLOATOBJ pf,
    _In_ PFLOATOBJ pf1);

ENGAPI
VOID
APIENTRY
FLOATOBJ_MulFloat(
    _Inout_ PFLOATOBJ pf,
    _In_ FLOATL f);

ENGAPI
VOID
APIENTRY
FLOATOBJ_MulLong(
    _Inout_ PFLOATOBJ pf,
    _In_ LONG l);

ENGAPI
VOID
APIENTRY
FLOATOBJ_Neg(
    _Inout_ PFLOATOBJ pf);

ENGAPI
VOID
APIENTRY
FLOATOBJ_SetFloat(
    _Out_ PFLOATOBJ pf,
    _In_ FLOATL f);

ENGAPI
VOID
APIENTRY
FLOATOBJ_SetLong(
    _Out_ PFLOATOBJ pf,
    _In_ LONG l);

ENGAPI
VOID
APIENTRY
FLOATOBJ_Sub(
    _Inout_ PFLOATOBJ pf,
    _In_ PFLOATOBJ pf1);

ENGAPI
VOID
APIENTRY
FLOATOBJ_SubFloat(
    _Inout_ PFLOATOBJ pf,
    _In_ FLOATL f);

ENGAPI
VOID
APIENTRY
FLOATOBJ_SubLong(
    _Inout_ PFLOATOBJ pf,
    _In_ LONG l);

#else

#define FLOATOBJ_SetFloat(pf, f)        {*(pf) = (f);}
#define FLOATOBJ_SetLong(pf, l)         {*(pf) = (FLOAT)(l);}
#define FLOATOBJ_GetFloat(pf)           (*(PULONG)(pf))
#define FLOATOBJ_GetLong(pf)            ((LONG)*(pf))
#define FLOATOBJ_Add(pf, pf1)           {*(pf) += *(pf1);}
#define FLOATOBJ_AddFloat(pf, f)        {*(pf) += (f);}
#define FLOATOBJ_AddLong(pf, l)         {*(pf) += (l);}
#define FLOATOBJ_Sub(pf, pf1)           {*(pf) -= *(pf1);}
#define FLOATOBJ_SubFloat(pf, f)        {*(pf) -= (f);}
#define FLOATOBJ_SubLong(pf, l)         {*(pf) -= (l);}
#define FLOATOBJ_Mul(pf, pf1)           {*(pf) *= *(pf1);}
#define FLOATOBJ_MulFloat(pf, f)        {*(pf) *= (f);}
#define FLOATOBJ_MulLong(pf, l)         {*(pf) *= (l);}
#define FLOATOBJ_Div(pf, pf1)           {*(pf) /= *(pf1);}
#define FLOATOBJ_DivFloat(pf, f)        {*(pf) /= (f);}
#define FLOATOBJ_DivLong(pf, l)         {*(pf) /= (l);}
#define FLOATOBJ_Neg(pf)                {*(pf) = -(*(pf));}
#define FLOATOBJ_Equal(pf, pf1)         (*(pf) == *(pf1))
#define FLOATOBJ_GreaterThan(pf, pf1)   (*(pf) > *(pf1))
#define FLOATOBJ_LessThan(pf, pf1)      (*(pf) < *(pf1))
#define FLOATOBJ_EqualLong(pf, l)       (*(pf) == (FLOAT)(l))
#define FLOATOBJ_GreaterThanLong(pf, l) (*(pf) > (FLOAT)(l))
#define FLOATOBJ_LessThanLong(pf, l)    (*(pf) < (FLOAT)(l))

#endif

ENGAPI
ULONG
APIENTRY
FONTOBJ_cGetAllGlyphHandles(
    _In_ FONTOBJ *pfo,
    _Out_opt_ HGLYPH *phg);

ENGAPI
ULONG
APIENTRY
FONTOBJ_cGetGlyphs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode,
    _In_ ULONG cGlyph,
    _In_count_(cGlyph) HGLYPH *phg,
    _Out_ PVOID *ppvGlyph);

ENGAPI
FD_GLYPHSET*
APIENTRY
FONTOBJ_pfdg(
    _In_ FONTOBJ *pfo);

ENGAPI
IFIMETRICS*
APIENTRY
FONTOBJ_pifi(
    _In_ FONTOBJ *pfo);

_Ret_opt_bytecount_(*pcjTable)
ENGAPI
PBYTE
APIENTRY
FONTOBJ_pjOpenTypeTablePointer(
    _In_ FONTOBJ *pfo,
    _In_ ULONG ulTag,
    _Out_ ULONG *pcjTable);

ENGAPI
PFD_GLYPHATTR
APIENTRY
FONTOBJ_pQueryGlyphAttrs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode);

_Ret_opt_bytecount_(*pcjFile)
ENGAPI
PVOID
APIENTRY
FONTOBJ_pvTrueTypeFontFile(
    _In_ FONTOBJ *pfo,
    _Out_ ULONG *pcjFile);

ENGAPI
LPWSTR
APIENTRY
FONTOBJ_pwszFontFilePaths(
    _In_ FONTOBJ *pfo,
    _Out_ ULONG *pcwc);

ENGAPI
XFORMOBJ*
APIENTRY
FONTOBJ_pxoGetXform(
    _In_ FONTOBJ *pfo);

ENGAPI
VOID
APIENTRY
FONTOBJ_vGetInfo(
    _In_ FONTOBJ *pfo,
    _In_ ULONG cjSize,
    _Out_bytecap_(cjSize) FONTINFO *pfi);

#if (NTDDI_VERSION <= NTDDI_WINXP)
 /* Obsolete in Windows 2000 and later */
GAMMA_TABLES*
APIENTRY
FONTOBJ_pGetGammaTables(
    _In_ FONTOBJ *pfo);
#endif

ENGAPI
LONG
APIENTRY
HT_ComputeRGBGammaTable(
    _In_ USHORT GammaTableEntries,
    _In_ USHORT GammaTableType,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma,
    _Out_writes_bytes_(GammaTableEntries * 3) LPBYTE pGammaTable);

ENGAPI
LONG
APIENTRY
HT_Get8BPPFormatPalette(
    _Out_opt_ LPPALETTEENTRY pPaletteEntry,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma);

ENGAPI
LONG
APIENTRY
HT_Get8BPPMaskPalette(
    _Out_opt_ LPPALETTEENTRY pPaletteEntry,
    _In_ BOOL Use8BPPMaskPal,
    _In_ BYTE CMYMask,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma);

ENGAPI
LONG
APIENTRY
HTUI_DeviceColorAdjustment(
    _In_ LPSTR pDeviceName,
    _Out_ PDEVHTADJDATA pDevHTAdjData);

ENGAPI
ULONG
APIENTRY
PALOBJ_cGetColors(
    _In_ PALOBJ *ppalo,
    _In_ ULONG iStart,
    _In_ ULONG cColors,
    _Out_writes_(cColors) ULONG *pulColors);

ENGAPI
BOOL
APIENTRY
PATHOBJ_bCloseFigure(
    _In_ PATHOBJ *ppo);

ENGAPI
BOOL
APIENTRY
PATHOBJ_bEnum(
    _In_ PATHOBJ *ppo,
    _Out_ PATHDATA *ppd);

ENGAPI
BOOL
APIENTRY
PATHOBJ_bEnumClipLines(
    _In_ PATHOBJ *ppo,
    _In_ ULONG cj,
    _Out_bytecap_(cj) CLIPLINE *pcl);

ENGAPI
BOOL
APIENTRY
PATHOBJ_bMoveTo(
    _In_ PATHOBJ *ppo,
    _In_ POINTFIX ptfx);

ENGAPI
BOOL
APIENTRY
PATHOBJ_bPolyBezierTo(
    _In_ PATHOBJ *ppo,
    _In_count_(cptfx) POINTFIX *pptfx,
    _In_ ULONG cptfx);

ENGAPI
BOOL
APIENTRY
PATHOBJ_bPolyLineTo(
    _In_ PATHOBJ *ppo,
    _In_count_(cptfx) POINTFIX *pptfx,
    _In_ ULONG cptfx);

ENGAPI
VOID
APIENTRY
PATHOBJ_vEnumStart(
    _Inout_ PATHOBJ *ppo);

ENGAPI
VOID
APIENTRY
PATHOBJ_vEnumStartClipLines(
    _Inout_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ SURFOBJ *pso,
    _In_ LINEATTRS *pla);

ENGAPI
VOID
APIENTRY
PATHOBJ_vGetBounds(
    _In_ PATHOBJ *ppo,
    _Out_ PRECTFX prectfx);

ENGAPI
BOOL
APIENTRY
STROBJ_bEnum(
    _Inout_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Out_ PGLYPHPOS *ppgpos); // FIXME_ size?

ENGAPI
BOOL
APIENTRY
STROBJ_bEnumPositionsOnly(
    _In_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Out_ PGLYPHPOS *ppgpos); // FIXME_ size?

ENGAPI
BOOL
APIENTRY
STROBJ_bGetAdvanceWidths(
    _In_ STROBJ *pso,
    _In_ ULONG iFirst,
    _In_ ULONG c,
    _Out_cap_(c) POINTQF *pptqD);

ENGAPI
DWORD
APIENTRY
STROBJ_dwGetCodePage(
    _In_ STROBJ *pstro);

ENGAPI
FIX
APIENTRY
STROBJ_fxBreakExtra(
    _In_ STROBJ *pstro);

ENGAPI
FIX
APIENTRY
STROBJ_fxCharacterExtra(
    _In_ STROBJ *pstro);

ENGAPI
VOID
APIENTRY
STROBJ_vEnumStart(
    _Inout_ STROBJ *pstro);

ENGAPI
BOOL
APIENTRY
WNDOBJ_bEnum(
    _Inout_ WNDOBJ *pwo,
    _In_ ULONG cj,
    _Out_bytecap_(cj) ULONG *pul);

ENGAPI
ULONG
APIENTRY
WNDOBJ_cEnumStart(
    _Inout_ WNDOBJ *pwo,
    _In_ ULONG iType,
    _In_ ULONG iDirection,
    _In_ ULONG cLimit);

ENGAPI
VOID
APIENTRY
WNDOBJ_vSetConsumer(
    _Inout_ WNDOBJ *pwo,
    _In_ PVOID pvConsumer);

/* XFORMOBJ_bApplyXform.iMode constants */
#define XF_LTOL                           0L
#define XF_INV_LTOL                       1L
#define XF_LTOFX                          2L
#define XF_INV_FXTOL                      3L

ENGAPI
BOOL
APIENTRY
XFORMOBJ_bApplyXform(
    _In_ XFORMOBJ *pxo,
    _In_ ULONG iMode,
    _In_ ULONG cPoints,
    _In_reads_bytes_(cPoints * sizeof(POINTL)) PVOID pvIn,
    _Out_writes_bytes_(cPoints * sizeof(POINTL)) PVOID pvOut);

/* Obsolete in Windows 2000 and later */
DECLSPEC_DEPRECATED_DDK
ENGAPI
HANDLE
APIENTRY
XFORMOBJ_cmGetTransform(
    XFORMOBJ *pxo);

#if !defined(USERMODE_DRIVER)
ENGAPI
ULONG
APIENTRY
XFORMOBJ_iGetFloatObjXform(
    _In_ XFORMOBJ *pxo,
    _Out_ FLOATOBJ_XFORM *pxfo);
#else
#define XFORMOBJ_iGetFloatObjXform XFORMOBJ_iGetXform
#endif

ENGAPI
ULONG
APIENTRY
XFORMOBJ_iGetXform(
    _In_ XFORMOBJ *pxo,
    _Out_ XFORML *pxform);

/* XLATEOBJ_cGetPalette.iPal constants */
#define XO_SRCPALETTE                     1
#define XO_DESTPALETTE                    2
#define XO_DESTDCPALETTE                  3
#define XO_SRCBITFIELDS                   4
#define XO_DESTBITFIELDS                  5

ENGAPI
ULONG
APIENTRY
XLATEOBJ_cGetPalette(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iPal,
    _In_ ULONG cPal,
    _Out_cap_(cPal) ULONG *pPal);

ENGAPI
HANDLE
APIENTRY
XLATEOBJ_hGetColorTransform(
    _In_ XLATEOBJ *pxlo);

ENGAPI
ULONG
APIENTRY
XLATEOBJ_iXlate(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iColor);

ENGAPI
ULONG*
APIENTRY
XLATEOBJ_piVector(
    _In_ XLATEOBJ *pxlo);

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)

ENGAPI
VOID
APIENTRY
EngBugCheckEx(
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR P1,
    _In_ ULONG_PTR P2,
    _In_ ULONG_PTR P3,
    _In_ ULONG_PTR P4);

#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP2) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

ENGAPI
HANDLE
APIENTRY
EngCreateRectRgn(
    _In_ INT left,
    _In_ INT top,
    _In_ INT right,
    _In_ INT bottom);

ENGAPI
VOID
APIENTRY
EngDeleteRgn(
    _In_ HANDLE hrgn);

ENGAPI
INT
APIENTRY
EngCombineRgn(
    _In_ HANDLE hrgnTrg,
    _In_ HANDLE hrgnSrc1,
    _In_ HANDLE hrgnSrc2,
    _In_ INT iMode);

ENGAPI
INT
APIENTRY
EngCopyRgn(
    _In_ HANDLE hrgnDst,
    _In_ HANDLE hrgnSrc);

ENGAPI
INT
APIENTRY
EngIntersectRgn(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);

ENGAPI
INT
APIENTRY
EngSubtractRgn(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);

ENGAPI
INT
APIENTRY
EngUnionRgn(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);

ENGAPI
INT
APIENTRY
EngXorRgn(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);

ENGAPI
BOOL
APIENTRY
EngRectInRgn(
    _In_ HANDLE hrgn,
    _In_ LPRECT prcl);

ENGAPI
BOOL
APIENTRY
EngEqualRgn(
    _In_ HANDLE hrgn1,
    _In_ HANDLE hrgn2);

ENGAPI
DWORD
APIENTRY
EngGetRgnData(
    _In_ HANDLE hrgn,
    _In_ DWORD nCount,
    _Out_cap_(nCount) LPRGNDATA lpRgnData);

ENGAPI
BOOL
APIENTRY
EngSetRectRgn(
    _In_ HANDLE hrgn,
    _In_ INT left,
    _In_ INT top,
    _In_ INT right,
    _In_ INT bottom);

ENGAPI
INT
APIENTRY
EngGetRgnBox(
    _In_ HANDLE hrgn,
    _Out_ LPRECT prcl);

ENGAPI
INT
APIENTRY
EngOffsetRgn(
    _In_ HANDLE hrgn,
    _In_ INT x,
    _In_ INT y);

ENGAPI
VOID
APIENTRY
EngRenderHint(
    _In_ DHPDEV dhpdev,
    _In_ ULONG NotifyCode,
    _In_ SIZE_T Length,
    _In_reads_bytes_opt_(Length) PVOID Data);

ENGAPI
VOID
APIENTRY
EngAcquireSemaphoreShared(
    _In_ HSEMAPHORE hsem);

ENGAPI
BOOL
APIENTRY
EngAcquireSemaphoreNoWait(
    _In_ HSEMAPHORE hsem);

ENGAPI
BOOL
APIENTRY
EngAcquireSemaphoreSharedNoWait(
    _In_ HSEMAPHORE hsem);

ENGAPI
BOOL
APIENTRY
EngIsSemaphoreSharedByCurrentThread(
    _In_ HSEMAPHORE hsem);

DECLARE_HANDLE(HFASTMUTEX);

ENGAPI
HFASTMUTEX
APIENTRY
EngCreateFastMutex(
    VOID);

ENGAPI
VOID
APIENTRY
EngDeleteFastMutex(
    _In_ HFASTMUTEX hfm);

ENGAPI
VOID
APIENTRY
EngAcquireFastMutex(
    _In_ HFASTMUTEX hfm);

ENGAPI
VOID
APIENTRY
EngReleaseFastMutex(
    _In_ HFASTMUTEX hfm);

ENGAPI
BOOL
APIENTRY
EngUpdateDeviceSurface(
    _In_ SURFOBJ *pso,
    _Inout_ CLIPOBJ **ppco);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

ENGAPI
HBITMAP
APIENTRY
EngCreateRedirectionDeviceBitmap(
    __in_data_source(USER_MODE) DHSURF dhsurf,
    __in_data_source(USER_MODE) SIZEL sizl,
    _In_ ULONG iFormatCompat);

VOID
APIENTRY
EngCTGetGammaTable(
    _In_ ULONG ulGamma,
    _Out_ CONST BYTE** pGammaTable,
    _Out_ CONST BYTE** pInverseGammaTable);

ULONG
APIENTRY
EngCTGetCurrentGamma(
    _In_ HDEV hdev);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

/* Graphics Driver Functions */

typedef BOOL
(APIENTRY FN_DrvAlphaBlend)(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_ BLENDOBJ *pBlendObj);
typedef FN_DrvAlphaBlend *PFN_DrvAlphaBlend;
extern FN_DrvAlphaBlend DrvAlphaBlend;

typedef BOOL
(APIENTRY FN_DrvAssertMode)(
    _In_ DHPDEV dhpdev,
    _In_ BOOL bEnable);
typedef FN_DrvAssertMode *PFN_DrvAssertMode;
extern FN_DrvAssertMode DrvAssertMode;

typedef BOOL
(APIENTRY FN_DrvBitBlt)(
    _Inout_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclTrg,
    _In_opt_ POINTL *pptlSrc,
    _In_opt_ POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ POINTL *pptlBrush,
    _In_ ROP4 rop4);
typedef FN_DrvBitBlt *PFN_DrvBitBlt;
extern FN_DrvBitBlt DrvBitBlt;

typedef VOID
(APIENTRY FN_DrvCompletePDEV)(
    _In_ DHPDEV dhpdev,
    _In_ HDEV hdev);
typedef FN_DrvCompletePDEV *PFN_DrvCompletePDEV;
extern FN_DrvCompletePDEV DrvCompletePDEV;

typedef BOOL
(APIENTRY FN_DrvCopyBits)(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDest,
    _In_ POINTL *pptlSrc);
typedef FN_DrvCopyBits *PFN_DrvCopyBits;
extern FN_DrvCopyBits DrvCopyBits;

typedef HBITMAP
(APIENTRY FN_DrvCreateDeviceBitmap)(
    _In_ DHPDEV  dhpdev,
    _In_ SIZEL  sizl,
    _In_ ULONG  iFormat);
typedef FN_DrvCreateDeviceBitmap *PFN_DrvCreateDeviceBitmap;
extern FN_DrvCreateDeviceBitmap DrvCreateDeviceBitmap;

typedef VOID
(APIENTRY FN_DrvDeleteDeviceBitmap)(
    _In_ _Post_ptr_invalid_ DHSURF dhsurf);
typedef FN_DrvDeleteDeviceBitmap *PFN_DrvDeleteDeviceBitmap;
extern FN_DrvDeleteDeviceBitmap DrvDeleteDeviceBitmap;

typedef HBITMAP
(APIENTRY FN_DrvDeriveSurface)(
    _In_ DD_DIRECTDRAW_GLOBAL *pDirectDraw,
    _In_ DD_SURFACE_LOCAL *pSurface);
typedef FN_DrvDeriveSurface *PFN_DrvDeriveSurface;
extern FN_DrvDeriveSurface DrvDeriveSurface;

typedef LONG
(APIENTRY FN_DrvDescribePixelFormat)(
    _In_ DHPDEV dhpdev,
    _In_ LONG iPixelFormat,
    _In_ ULONG cjpfd,
    _Out_opt_ PIXELFORMATDESCRIPTOR *ppfd);
typedef FN_DrvDescribePixelFormat *PFN_DrvDescribePixelFormat;
extern FN_DrvDescribePixelFormat DrvDescribePixelFormat;

typedef VOID
(APIENTRY FN_DrvDestroyFont)(
    _In_ FONTOBJ *pfo);
typedef FN_DrvDestroyFont *PFN_DrvDestroyFont;
extern FN_DrvDestroyFont DrvDestroyFont;

typedef VOID
(APIENTRY FN_DrvDisableDriver)(VOID);
typedef FN_DrvDisableDriver *PFN_DrvDisableDriver;
extern FN_DrvDisableDriver DrvDisableDriver;

typedef VOID
(APIENTRY FN_DrvDisablePDEV)(
    _In_ DHPDEV dhpdev);
typedef FN_DrvDisablePDEV *PFN_DrvDisablePDEV;
extern FN_DrvDisablePDEV DrvDisablePDEV;

typedef VOID
(APIENTRY FN_DrvDisableSurface)(
    _In_ DHPDEV dhpdev);
typedef FN_DrvDisableSurface *PFN_DrvDisableSurface;
extern FN_DrvDisableSurface DrvDisableSurface;

typedef ULONG
(APIENTRY FN_DrvDitherColor)(
    _In_ DHPDEV dhpdev,
    _In_ ULONG iMode,
    _In_ ULONG rgb,
    _Inout_updates_(8 * 8) ULONG *pul); // FIXME: compare EngDitherColor
typedef FN_DrvDitherColor *PFN_DrvDitherColor;
extern FN_DrvDitherColor DrvDitherColor;

typedef ULONG
(APIENTRY FN_DrvDrawEscape)(
    _In_ SURFOBJ *pso,
    _In_ ULONG iEsc,
    _In_ CLIPOBJ *pco,
    _In_ RECTL *prcl,
    _In_ ULONG cjIn,
    _In_reads_bytes_(cjIn) PVOID pvIn);
typedef FN_DrvDrawEscape *PFN_DrvDrawEscape;
extern FN_DrvDrawEscape DrvDrawEscape;

typedef BOOL
(APIENTRY FN_DrvEnableDriver)(
    _In_ ULONG iEngineVersion,
    _In_ ULONG cj,
    _Inout_bytecount_(cj) DRVENABLEDATA *pded); // Zero initialized
typedef FN_DrvEnableDriver *PFN_DrvEnableDriver;
extern FN_DrvEnableDriver DrvEnableDriver;

typedef DHPDEV
(APIENTRY FN_DrvEnablePDEV)(
    _In_ DEVMODEW *pdm,
    _In_ LPWSTR pwszLogAddress,
    _In_ ULONG cPat,
    _Out_opt_cap_(cPat) HSURF *phsurfPatterns,
    _In_ ULONG cjCaps,
    _Out_bytecap_(cjCaps) ULONG *pdevcaps,
    _In_ ULONG cjDevInfo,
    _Out_ DEVINFO *pdi,
    _In_ HDEV hdev,
    _In_ LPWSTR pwszDeviceName,
    _In_ HANDLE hDriver);
typedef FN_DrvEnablePDEV *PFN_DrvEnablePDEV;
extern FN_DrvEnablePDEV DrvEnablePDEV;

typedef HSURF
(APIENTRY FN_DrvEnableSurface)(
    _In_ DHPDEV dhpdev);
typedef FN_DrvEnableSurface *PFN_DrvEnableSurface;
extern FN_DrvEnableSurface DrvEnableSurface;

/* DrvEndDoc.fl constants */
#define ED_ABORTDOC                       0x00000001

typedef BOOL
(APIENTRY FN_DrvEndDoc)(
    _In_ SURFOBJ *pso,
    _In_ FLONG fl);
typedef FN_DrvEndDoc *PFN_DrvEndDoc;
extern FN_DrvEndDoc DrvEndDoc;

typedef ULONG
(APIENTRY FN_DrvEscape)(
    _In_ SURFOBJ *pso,
    _In_ ULONG iEsc,
    _In_ ULONG cjIn,
    _In_bytecount_(cjIn) PVOID pvIn,
    _In_ ULONG cjOut,
    _Out_bytecap_(cjOut) PVOID pvOut);
typedef FN_DrvEscape *PFN_DrvEscape;
extern FN_DrvEscape DrvEscape;

typedef BOOL
(APIENTRY FN_DrvFillPath)(
    _Inout_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions);
typedef FN_DrvFillPath *PFN_DrvFillPath;
extern FN_DrvFillPath DrvFillPath;

typedef ULONG
(APIENTRY FN_DrvFontManagement)(
    _In_ SURFOBJ  *pso,
    _In_opt_ FONTOBJ  *pfo,
    _In_ ULONG  iMode,
    _In_ ULONG  cjIn,
    _In_bytecount_(cjIn) PVOID  pvIn,
    _In_ ULONG  cjOut,
    _Out_bytecap_(cjOut) PVOID  pvOut);
typedef FN_DrvFontManagement *PFN_DrvFontManagement;
extern FN_DrvFontManagement DrvFontManagement;

typedef VOID
(APIENTRY FN_DrvFree)(
    _In_ _Post_ptr_invalid_ PVOID pv,
    _In_ ULONG_PTR id);
typedef FN_DrvFree *PFN_DrvFree;
extern FN_DrvFree DrvFree;

/* DrvGetGlyphMode return values */
#define FO_HGLYPHS                        0L
#define FO_GLYPHBITS                      1L
#define FO_PATHOBJ                        2L

typedef ULONG
(APIENTRY FN_DrvGetGlyphMode)(
    _In_ DHPDEV dhpdev,
    _In_ FONTOBJ *pfo);
typedef FN_DrvGetGlyphMode *PFN_DrvGetGlyphMode;
extern FN_DrvGetGlyphMode DrvGetGlyphMode;

typedef ULONG
(APIENTRY FN_DrvGetModes)(
    _In_ HANDLE hDriver,
    _In_ ULONG cjSize,
    _Out_opt_bytecap_(cjSize) DEVMODEW *pdm);
typedef FN_DrvGetModes *PFN_DrvGetModes;
extern FN_DrvGetModes DrvGetModes;

typedef PVOID
(APIENTRY FN_DrvGetTrueTypeFile)(
    _In_ ULONG_PTR iFile,
    _In_ ULONG *pcj);
typedef FN_DrvGetTrueTypeFile *PFN_DrvGetTrueTypeFile;
extern FN_DrvGetTrueTypeFile DrvGetTrueTypeFile;

typedef BOOL
(APIENTRY FN_DrvGradientFill)(
    _Inout_ SURFOBJ *psoDest,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ TRIVERTEX *pVertex,
    _In_ ULONG nVertex,
    _In_ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ RECTL *prclExtents,
    _In_ POINTL *pptlDitherOrg,
    _In_ ULONG ulMode);
typedef FN_DrvGradientFill *PFN_DrvGradientFill;
extern FN_DrvGradientFill DrvGradientFill;

typedef BOOL
(APIENTRY FN_DrvIcmCheckBitmapBits)(
    _In_ DHPDEV dhpdev,
    _In_ HANDLE hColorTransform,
    _In_ SURFOBJ *pso,
    _Out_bytecap_(pso->sizlBitmap.cx * pso->sizlBitmap.cy) PBYTE paResults);
typedef FN_DrvIcmCheckBitmapBits *PFN_DrvIcmCheckBitmapBits;
extern FN_DrvIcmCheckBitmapBits DrvIcmCheckBitmapBits;

typedef HANDLE
(APIENTRY FN_DrvIcmCreateColorTransform)(
    _In_ DHPDEV dhpdev,
    _In_ LPLOGCOLORSPACEW pLogColorSpace,
    _In_reads_bytes_opt_(cjSourceProfile) PVOID pvSourceProfile,
    _In_ ULONG cjSourceProfile,
    _In_reads_bytes_(cjDestProfile) PVOID pvDestProfile,
    _In_ ULONG cjDestProfile,
    _In_reads_bytes_opt_(cjTargetProfile) PVOID pvTargetProfile,
    _In_ ULONG cjTargetProfile,
    _In_ DWORD dwReserved);
typedef FN_DrvIcmCreateColorTransform *PFN_DrvIcmCreateColorTransform;
extern FN_DrvIcmCreateColorTransform DrvIcmCreateColorTransform;

typedef BOOL
(APIENTRY FN_DrvIcmDeleteColorTransform)(
    _In_ DHPDEV dhpdev,
    _In_ HANDLE hcmXform);
typedef FN_DrvIcmDeleteColorTransform *PFN_DrvIcmDeleteColorTransform;
extern FN_DrvIcmDeleteColorTransform DrvIcmDeleteColorTransform;

/* DrvIcmSetDeviceGammaRamp.iFormat constants */
#define IGRF_RGB_256BYTES                 0x00000000
#define IGRF_RGB_256WORDS                 0x00000001

typedef BOOL
(APIENTRY FN_DrvIcmSetDeviceGammaRamp)(
    _In_ DHPDEV dhpdev,
    _In_ ULONG iFormat,
    _In_bytecount_(sizeof(GAMMARAMP)) LPVOID lpRamp);
typedef FN_DrvIcmSetDeviceGammaRamp *PFN_DrvIcmSetDeviceGammaRamp;
extern FN_DrvIcmSetDeviceGammaRamp DrvIcmSetDeviceGammaRamp;

typedef BOOL
(APIENTRY FN_DrvLineTo)(
    _Inout_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_opt_ RECTL *prclBounds,
    _In_ MIX mix);
typedef FN_DrvLineTo *PFN_DrvLineTo;
extern FN_DrvLineTo DrvLineTo;

typedef ULONG_PTR
(APIENTRY FN_DrvLoadFontFile)(
    _In_ ULONG cFiles,
    _In_ ULONG_PTR *piFile,
    _In_count_(cFiles) PVOID *ppvView,
    _In_count_(cFiles) ULONG *pcjView,
    _In_opt_ DESIGNVECTOR *pdv,
    _In_ ULONG ulLangID,
    _In_ ULONG ulFastCheckSum);
typedef FN_DrvLoadFontFile *PFN_DrvLoadFontFile;
extern FN_DrvLoadFontFile DrvLoadFontFile;

typedef VOID
(APIENTRY FN_DrvMovePointer)(
    _In_ SURFOBJ *pso,
    _In_ LONG x,
    _In_ LONG y,
    _In_opt_ RECTL *prcl);
typedef FN_DrvMovePointer *PFN_DrvMovePointer;
extern FN_DrvMovePointer DrvMovePointer;

typedef BOOL
(APIENTRY FN_DrvNextBand)(
    _In_ SURFOBJ *pso,
    _In_ POINTL *pptl);
typedef FN_DrvNextBand *PFN_DrvNextBand;
extern FN_DrvNextBand DrvNextBand;

typedef VOID
(APIENTRY FN_DrvNotify)(
    _In_ SURFOBJ *pso,
    _In_ ULONG iType,
    _In_opt_ PVOID pvData);
typedef FN_DrvNotify *PFN_DrvNotify;
extern FN_DrvNotify DrvNotify;

typedef BOOL
(APIENTRY FN_DrvOffset)(
    _In_ SURFOBJ *pso,
    _In_ LONG x,
    _In_ LONG y,
    _In_ FLONG flReserved);
typedef FN_DrvOffset *PFN_DrvOffset;
extern FN_DrvOffset DrvOffset;

typedef BOOL
(APIENTRY FN_DrvPaint)(
    _Inout_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix);
typedef FN_DrvPaint *PFN_DrvPaint;
extern FN_DrvPaint DrvPaint;

typedef BOOL
(APIENTRY FN_DrvPlgBlt)(
    _Inout_ SURFOBJ *psoTrg,
    _Inout_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMsk,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_opt_ POINTL *pptlBrushOrg,
    _In_ POINTFIX *pptfx,
    _In_ RECTL *prcl,
    _In_opt_ POINTL *pptl,
    _In_ ULONG iMode);
typedef FN_DrvPlgBlt *PFN_DrvPlgBlt;
extern FN_DrvPlgBlt DrvPlgBlt;

/* DrvQueryAdvanceWidths.iMode constants */
#define QAW_GETWIDTHS                     0
#define QAW_GETEASYWIDTHS                 1

typedef BOOL
(APIENTRY FN_DrvQueryAdvanceWidths)(
    _In_ DHPDEV dhpdev,
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode,
    _In_reads_(cGlyphs) HGLYPH *phg,
    _Out_writes_bytes_(cGlyphs * sizeof(USHORT)) PVOID pvWidths,
    _In_ ULONG cGlyphs);
typedef FN_DrvQueryAdvanceWidths *PFN_DrvQueryAdvanceWidths;
extern FN_DrvQueryAdvanceWidths DrvQueryAdvanceWidths;

/* DrvQueryDeviceSupport.iType constants */
#define QDS_CHECKJPEGFORMAT               0x00000000
#define QDS_CHECKPNGFORMAT                0x00000001

typedef BOOL
(APIENTRY FN_DrvQueryDeviceSupport)(
    _In_ SURFOBJ *pso,
    _In_ XLATEOBJ *pxlo,
    _In_ XFORMOBJ *pxo,
    _In_ ULONG iType,
    _In_ ULONG cjIn,
    _In_bytecount_(cjIn) PVOID pvIn,
    _In_ ULONG cjOut,
    _Out_bytecap_(cjOut) PVOID pvOut);
typedef FN_DrvQueryDeviceSupport *PFN_DrvQueryDeviceSupport;
extern FN_DrvQueryDeviceSupport DrvQueryDeviceSupport;

/* DrvQueryDriverInfo.dwMode constants */
#define DRVQUERY_USERMODE                 0x00000001

typedef BOOL
(APIENTRY FN_DrvQueryDriverInfo)(
    _In_ DWORD dwMode,
    _Out_bytecap_(cjBuf) PVOID pBuffer,
    _In_ DWORD cjBuf,
    _Out_ PDWORD pcbNeeded);
typedef FN_DrvQueryDriverInfo *PFN_DrvQueryDriverInfo;
extern FN_DrvQueryDriverInfo DrvQueryDriverInfo;

typedef PIFIMETRICS
(APIENTRY FN_DrvQueryFont)(
    _In_ DHPDEV dhpdev,
    _In_ ULONG_PTR iFile,
    _In_ ULONG iFace,
    _Out_ ULONG_PTR *pid);
typedef FN_DrvQueryFont *PFN_DrvQueryFont;
extern FN_DrvQueryFont DrvQueryFont;

/* DrvQueryFontCaps.pulCaps constants */
#define QC_OUTLINES                       0x00000001
#define QC_1BIT                           0x00000002
#define QC_4BIT                           0x00000004
#define QC_FONTDRIVERCAPS (QC_OUTLINES | QC_1BIT | QC_4BIT)

typedef LONG
(APIENTRY FN_DrvQueryFontCaps)(
    _In_ ULONG culCaps,
    _Out_cap_(culCaps) ULONG *pulCaps);
typedef FN_DrvQueryFontCaps *PFN_DrvQueryFontCaps;
extern FN_DrvQueryFontCaps DrvQueryFontCaps;

/* DrvQueryFontData.iMode constants */
#define QFD_GLYPHANDBITMAP                1L
#define QFD_GLYPHANDOUTLINE               2L
#define QFD_MAXEXTENTS                    3L
#define QFD_TT_GLYPHANDBITMAP             4L
#define QFD_TT_GRAY1_BITMAP               5L
#define QFD_TT_GRAY2_BITMAP               6L
#define QFD_TT_GRAY4_BITMAP               8L
#define QFD_TT_GRAY8_BITMAP               9L

#define QFD_TT_MONO_BITMAP QFD_TT_GRAY1_BITMAP

typedef LONG
(APIENTRY FN_DrvQueryFontData)(
    _In_ DHPDEV dhpdev,
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode,
    _In_ HGLYPH hg,
    _In_opt_ GLYPHDATA *pgd,
    _Out_bytecap_(cjSize) PVOID pv,
    _In_ ULONG cjSize);
typedef FN_DrvQueryFontData *PFN_DrvQueryFontData;
extern FN_DrvQueryFontData DrvQueryFontData;

/* DrvQueryFontFile.ulMode constants */
#define QFF_DESCRIPTION                   0x00000001
#define QFF_NUMFACES                      0x00000002

typedef LONG
(APIENTRY FN_DrvQueryFontFile)(
    _In_ ULONG_PTR iFile,
    _In_ ULONG ulMode,
    _In_ ULONG cjBuf,
    _Out_bytecap_(cjBuf) ULONG *pulBuf);
typedef FN_DrvQueryFontFile *PFN_DrvQueryFontFile;
extern FN_DrvQueryFontFile DrvQueryFontFile;

/* DrvQueryFontTree.iMode constants */
#define QFT_UNICODE                       0L
#define QFT_LIGATURES                     1L
#define QFT_KERNPAIRS                     2L
#define QFT_GLYPHSET                      3L

typedef PVOID
(APIENTRY FN_DrvQueryFontTree)(
    _In_ DHPDEV dhpdev,
    _In_ ULONG_PTR iFile,
    _In_ ULONG iFace,
    _In_ ULONG iMode,
    _Out_ ULONG_PTR *pid);
typedef FN_DrvQueryFontTree *PFN_DrvQueryFontTree;
extern FN_DrvQueryFontTree DrvQueryFontTree;

typedef PFD_GLYPHATTR
(APIENTRY FN_DrvQueryGlyphAttrs)(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode);
typedef FN_DrvQueryGlyphAttrs *PFN_DrvQueryGlyphAttrs;
extern FN_DrvQueryGlyphAttrs DrvQueryGlyphAttrs;

typedef ULONG
(APIENTRY FN_DrvQueryPerBandInfo)(
    _In_ SURFOBJ *pso,
    _Inout_ PERBANDINFO *pbi);
typedef FN_DrvQueryPerBandInfo *PFN_DrvQueryPerBandInfo;
extern FN_DrvQueryPerBandInfo DrvQueryPerBandInfo;

/* DrvQueryTrueTypeOutline.bMetricsOnly constants */
#define TTO_METRICS_ONLY                  0x00000001
#define TTO_QUBICS                        0x00000002
#define TTO_UNHINTED                      0x00000004

typedef LONG
(APIENTRY FN_DrvQueryTrueTypeOutline)(
    _In_ DHPDEV dhpdev,
    _In_ FONTOBJ *pfo,
    _In_ HGLYPH hglyph,
    _In_ BOOL bMetricsOnly,
    _Out_opt_ GLYPHDATA *pgldt,
    _In_ ULONG cjBuf,
    _Out_bytecap_(cjBuf) TTPOLYGONHEADER *ppoly);
typedef FN_DrvQueryTrueTypeOutline *PFN_DrvQueryTrueTypeOutline;
extern FN_DrvQueryTrueTypeOutline DrvQueryTrueTypeOutline;

typedef LONG
(APIENTRY FN_DrvQueryTrueTypeTable)(
    _In_ ULONG_PTR iFile,
    _In_ ULONG ulFont,
    _In_ ULONG ulTag,
    _In_ PTRDIFF dpStart,
    _In_ ULONG cjBuf,
    _Out_opt_bytecap_(cjBuf) BYTE *pjBuf,
    _Outptr_opt_result_bytebuffer_all_maybenull_(*pcjTable) PBYTE *ppjTable,
    _Out_opt_ ULONG *pcjTable);
typedef FN_DrvQueryTrueTypeTable *PFN_DrvQueryTrueTypeTable;
extern FN_DrvQueryTrueTypeTable DrvQueryTrueTypeTable;

/* DrvRealizeBrush.iHatch constants */
#define RB_DITHERCOLOR                    0x80000000L

#define HS_DDI_MAX                        6

typedef BOOL
(APIENTRY FN_DrvRealizeBrush)(
    _In_ BRUSHOBJ *pbo,
    _Inout_ SURFOBJ *psoTarget, // CHECKME
    _In_opt_ SURFOBJ *psoPattern,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ ULONG iHatch);
typedef FN_DrvRealizeBrush *PFN_DrvRealizeBrush;
extern FN_DrvRealizeBrush DrvRealizeBrush;

/* DrvResetDevice return values */
#define DRD_SUCCESS                       0
#define DRD_ERROR                         1

typedef ULONG
(APIENTRY FN_DrvResetDevice)(
    _In_ DHPDEV dhpdev,
    _Reserved_ PVOID Reserved);
typedef FN_DrvResetDevice *PFN_DrvResetDevice;
extern FN_DrvResetDevice DrvResetDevice;

typedef BOOL
(APIENTRY FN_DrvResetPDEV)(
    _In_ DHPDEV dhpdevOld,
    _In_ DHPDEV dhpdevNew);
typedef FN_DrvResetPDEV *PFN_DrvResetPDEV;
extern FN_DrvResetPDEV DrvResetPDEV;

/* DrvSaveScreenBits.iMode constants */
#define SS_SAVE                           0x00000000
#define SS_RESTORE                        0x00000001
#define SS_FREE                           0x00000002

typedef ULONG_PTR
(APIENTRY FN_DrvSaveScreenBits)(
    _In_ SURFOBJ *pso,
    _In_ ULONG iMode,
    _In_ ULONG_PTR ident,
    _In_ RECTL *prcl);
typedef FN_DrvSaveScreenBits *PFN_DrvSaveScreenBits;
extern FN_DrvSaveScreenBits DrvSaveScreenBits;

typedef BOOL
(APIENTRY FN_DrvSendPage)(
    _In_ SURFOBJ *pso);
typedef FN_DrvSendPage *PFN_DrvSendPage;
extern FN_DrvSendPage DrvSendPage;

/* DrvSetPalette range */
#define WINDDI_MAXSETPALETTECOLORS     256
#define WINDDI_MAXSETPALETTECOLORINDEX 255

typedef BOOL
(APIENTRY FN_DrvSetPalette)(
    _In_ DHPDEV dhpdev,
    _In_ PALOBJ *ppalo,
    _In_ FLONG fl,
    _In_range_(0, WINDDI_MAXSETPALETTECOLORINDEX) ULONG iStart,
    _In_range_(0, WINDDI_MAXSETPALETTECOLORS - iStart) ULONG cColors);
typedef FN_DrvSetPalette *PFN_DrvSetPalette;
extern FN_DrvSetPalette DrvSetPalette;

typedef BOOL
(APIENTRY FN_DrvSetPixelFormat)(
    _In_ SURFOBJ *pso,
    _In_ LONG iPixelFormat,
    _In_ HWND hwnd);
typedef FN_DrvSetPixelFormat *PFN_DrvSetPixelFormat;
extern FN_DrvSetPixelFormat DrvSetPixelFormat;

/* DrvSetPointerShape return values */
#define SPS_ERROR                         0x00000000
#define SPS_DECLINE                       0x00000001
#define SPS_ACCEPT_NOEXCLUDE              0x00000002
#define SPS_ACCEPT_EXCLUDE                0x00000003
#define SPS_ACCEPT_SYNCHRONOUS            0x00000004

/* DrvSetPointerShape.fl constants */
#define SPS_CHANGE                        0x00000001L
#define SPS_ASYNCCHANGE                   0x00000002L
#define SPS_ANIMATESTART                  0x00000004L
#define SPS_ANIMATEUPDATE                 0x00000008L
#define SPS_ALPHA                         0x00000010L
#define SPS_RESERVED                      0x00000020L /* Force s/w cursor rendering */
#define SPS_RESERVED1                     0x00000040L /* Force show/hide system cursor */
#define SPS_FLAGSMASK                     0x000000FFL
#define SPS_LENGTHMASK                    0x00000F00L
#define SPS_FREQMASK                      0x000FF000L

typedef ULONG
(APIENTRY FN_DrvSetPointerShape)(
    _In_ SURFOBJ *pso,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ SURFOBJ *psoColor,
    _In_ XLATEOBJ *pxlo,
    _In_ LONG xHot,
    _In_ LONG yHot,
    _In_ LONG x,
    _In_ LONG y,
    _In_ RECTL *prcl,
    _In_ FLONG fl);
typedef FN_DrvSetPointerShape *PFN_DrvSetPointerShape;
extern FN_DrvSetPointerShape DrvSetPointerShape;

typedef BOOL
(APIENTRY FN_DrvStartBanding)(
    _In_ SURFOBJ *pso,
    _In_ POINTL *pptl);
typedef FN_DrvStartBanding *PFN_DrvStartBanding;
extern FN_DrvStartBanding DrvStartBanding;

typedef BOOL
(APIENTRY FN_DrvStartDoc)(
    _In_ SURFOBJ *pso,
    _In_ LPWSTR pwszDocName,
    _In_ DWORD dwJobId);
typedef FN_DrvStartDoc *PFN_DrvStartDoc;
extern FN_DrvStartDoc DrvStartDoc;

typedef BOOL
(APIENTRY FN_DrvStartPage)(
    _In_ SURFOBJ *pso);
typedef FN_DrvStartPage *PFN_DrvStartPage;
extern FN_DrvStartPage DrvStartPage;

typedef BOOL
(APIENTRY FN_DrvStretchBlt)(
    _Inout_ SURFOBJ *psoDest,
    _Inout_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlHTOrg,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_ ULONG iMode);
typedef FN_DrvStretchBlt *PFN_DrvStretchBlt;
extern FN_DrvStretchBlt DrvStretchBlt;

typedef BOOL
(APIENTRY FN_DrvStretchBltROP)(
    _Inout_ SURFOBJ *psoDest,
    _Inout_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlHTOrg,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_ ULONG iMode,
    _In_ BRUSHOBJ *pbo,
    _In_ DWORD rop4);
typedef FN_DrvStretchBltROP *PFN_DrvStretchBltROP;
extern FN_DrvStretchBltROP DrvStretchBltROP;

typedef BOOL
(APIENTRY FN_DrvStrokeAndFillPath)(
    _Inout_ SURFOBJ *pso,
    _Inout_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_opt_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pboStroke,
    _In_ LINEATTRS *plineattrs,
    _In_ BRUSHOBJ *pboFill,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mixFill,
    _In_ FLONG flOptions);
typedef FN_DrvStrokeAndFillPath *PFN_DrvStrokeAndFillPath;
extern FN_DrvStrokeAndFillPath DrvStrokeAndFillPath;

typedef BOOL
(APIENTRY FN_DrvStrokePath)(
    _Inout_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_opt_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ LINEATTRS *plineattrs,
    _In_ MIX mix);
typedef FN_DrvStrokePath *PFN_DrvStrokePath;
extern FN_DrvStrokePath DrvStrokePath;

typedef BOOL
(APIENTRY FN_DrvSwapBuffers)(
    _In_ SURFOBJ *pso,
    _In_ WNDOBJ *pwo);
typedef FN_DrvSwapBuffers *PFN_DrvSwapBuffers;
extern FN_DrvSwapBuffers DrvSwapBuffers;

typedef VOID
(APIENTRY FN_DrvSynchronize)(
    _In_ DHPDEV dhpdev,
    _In_count_c_(0) RECTL *prcl); // should be ignored
typedef FN_DrvSynchronize *PFN_DrvSynchronize;
extern FN_DrvSynchronize DrvSynchronize;

/* DrvSynchronizeSurface.fl constants */
#define DSS_TIMER_EVENT                   0x00000001
#define DSS_FLUSH_EVENT                   0x00000002
#define DSS_RESERVED                      0x00000004
#define DSS_RESERVED1                     0x00000008

typedef VOID
(APIENTRY FN_DrvSynchronizeSurface)(
    _In_ SURFOBJ *pso,
    _In_opt_ RECTL *prcl,
    _In_ FLONG fl);
typedef FN_DrvSynchronizeSurface *PFN_DrvSynchronizeSurface;
extern FN_DrvSynchronizeSurface DrvSynchronizeSurface;

typedef BOOL
(APIENTRY FN_DrvTextOut)(
    _In_ SURFOBJ *pso,
    _In_ STROBJ *pstro,
    _In_ FONTOBJ *pfo,
    _In_ CLIPOBJ *pco,
    _Null_ RECTL *prclExtra,
    _In_opt_ RECTL *prclOpaque,
    _In_ BRUSHOBJ *pboFore,
    _In_ BRUSHOBJ *pboOpaque,
    _In_ POINTL *pptlOrg,
    _In_ MIX mix);
typedef FN_DrvTextOut *PFN_DrvTextOut;
extern FN_DrvTextOut DrvTextOut;

typedef BOOL
(APIENTRY FN_DrvTransparentBlt)(
    _Inout_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ RECTL *prclSrc,
    _In_ ULONG iTransColor,
    _In_ ULONG ulReserved);
typedef FN_DrvTransparentBlt *PFN_DrvTransparentBlt;
extern FN_DrvTransparentBlt DrvTransparentBlt;

typedef BOOL
(APIENTRY FN_DrvUnloadFontFile)(
    _In_ ULONG_PTR iFile);
typedef FN_DrvUnloadFontFile *PFN_DrvUnloadFontFile;
extern FN_DrvUnloadFontFile DrvUnloadFontFile;

/* Direct draw */

typedef VOID
(APIENTRY FN_DrvDisableDirectDraw)(
    _In_ DHPDEV dhpdev);
typedef FN_DrvDisableDirectDraw *PFN_DrvDisableDirectDraw;
extern FN_DrvDisableDirectDraw DrvDisableDirectDraw;

typedef BOOL
(APIENTRY FN_DrvEnableDirectDraw)(
    _In_ DHPDEV dhpdev,
    _Out_ DD_CALLBACKS *pCallBacks,
    _Out_ DD_SURFACECALLBACKS *pSurfaceCallBacks,
    _Out_ DD_PALETTECALLBACKS *pPaletteCallBacks);
typedef FN_DrvEnableDirectDraw *PFN_DrvEnableDirectDraw;
extern FN_DrvEnableDirectDraw DrvEnableDirectDraw;

typedef BOOL
(APIENTRY FN_DrvGetDirectDrawInfo)(
    _In_ DHPDEV dhpdev,
    _Out_ DD_HALINFO *pHalInfo,
    _Out_ DWORD *pdwNumHeaps,
    _Out_ VIDEOMEMORY *pvmList,
    _Out_ DWORD *pdwNumFourCCCodes,
    _Out_ DWORD *pdwFourCC);
typedef FN_DrvGetDirectDrawInfo *PFN_DrvGetDirectDrawInfo;
extern FN_DrvGetDirectDrawInfo DrvGetDirectDrawInfo;

typedef BOOL //DECLSPEC_DEPRECATED_DDK
(APIENTRY FN_DrvQuerySpoolType)(
    _In_ DHPDEV dhpdev,
    _In_ LPWSTR pwchType);
typedef FN_DrvQuerySpoolType *PFN_DrvQuerySpoolType;
extern FN_DrvQuerySpoolType DrvQuerySpoolType;

typedef LONG
(APIENTRY FN_DrvQueryTrueTypeSection)(
    ULONG,
    ULONG,
    ULONG,
    HANDLE *,
    PTRDIFF *);
typedef FN_DrvQueryTrueTypeSection *PFN_DrvQueryTrueTypeSection;
extern FN_DrvQueryTrueTypeSection DrvQueryTrueTypeSection;

DECLSPEC_DEPRECATED_DDK
typedef VOID
(APIENTRY FN_DrvMovePanning)(
    _In_ LONG x,
    _In_ LONG y,
    _In_ FLONG fl);
typedef FN_DrvMovePanning *PFN_DrvMovePanning;
extern FN_DrvMovePanning DrvMovePanning;

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef LONG
(APIENTRY FN_DrvRenderHint)(
    _In_ DHPDEV dhpdev,
    _In_ ULONG NotifyCode,
    _In_ SIZE_T Length,
    _In_reads_bytes_opt_(Length) PVOID Data);
typedef FN_DrvRenderHint *PFN_DrvRenderHint;
extern FN_DrvRenderHint DrvRenderHint;

typedef struct _DRH_APIBITMAPDATA
{
    SURFOBJ *pso;
    BOOL b;
} DRH_APIBITMAPDATA, *PDRH_APIBITMAPDATA;

#define DRH_APIBITMAP 0x00000001

typedef HANDLE
(APIENTRY FN_EngCreateRectRgn)(
    _In_ INT left,
    _In_ INT top,
    _In_ INT right,
    _In_ INT bottom);
typedef FN_EngCreateRectRgn *PFN_EngCreateRectRgn;

typedef VOID
(APIENTRY FN_EngDeleteRgn)(
    HANDLE hrgn);
typedef FN_EngDeleteRgn *PFN_EngDeleteRgn;

typedef INT
(APIENTRY FN_EngCombineRgn)(
    _In_ HANDLE hrgnTrg,
    _In_ HANDLE hrgnSrc1,
    _In_ HANDLE hrgnSrc2,
    _In_ INT iMode);
typedef FN_EngCombineRgn *PFN_EngCombineRgn;

typedef INT
(APIENTRY FN_EngCopyRgn)(
    _In_ HANDLE hrgnDst,
    _In_ HANDLE hrgnSrc);
typedef FN_EngCopyRgn *PFN_EngCopyRgn;

typedef INT
(APIENTRY FN_EngIntersectRgn)(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);
typedef FN_EngIntersectRgn *PFN_EngIntersectRgn;

typedef INT
(APIENTRY FN_EngSubtractRgn)(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);
typedef FN_EngSubtractRgn *PFN_EngSubtractRgn;

typedef INT
(APIENTRY FN_EngUnionRgn)(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);
typedef FN_EngUnionRgn *PFN_EngUnionRgn;

typedef INT
(APIENTRY FN_EngXorRgn)(
    _In_ HANDLE hrgnResult,
    _In_ HANDLE hRgnA,
    _In_ HANDLE hRgnB);
typedef FN_EngXorRgn *PFN_EngXorRgn;

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

/* DrvCreateDeviceBitmapEx() flags */
#define CDBEX_REDIRECTION  0x00000001
#define CDBEX_DXINTEROP    0x00000002

typedef HBITMAP
(APIENTRY FN_DrvCreateDeviceBitmapEx)(
    _In_ DHPDEV dhpdev,
    _In_ SIZEL sizl,
    _In_ ULONG iFormat,
    _In_ DWORD Flags,
    _In_ DHSURF dhsurfGroup,
    _In_ DWORD DxFormat,
#if (NTDDI_VERSION >= NTDDI_WIN8)
    _In_ DWORD SubresourceIndex,
#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */
    _Out_ HANDLE* phSharedSurface);
typedef FN_DrvCreateDeviceBitmapEx *PFN_DrvCreateDeviceBitmapEx;
extern DrvCreateDeviceBitmapEx DrvCreateDeviceBitmapEx

typedef VOID
(APIENTRY FN_DrvDeleteDeviceBitmapEx)(
    _Inout_ DHSURF);
typedef FN_DrvDeleteDeviceBitmapEx *PFN_DrvDeleteDeviceBitmapEx;
extern FN_DrvDeleteDeviceBitmapEx DrvDeleteDeviceBitmapEx;

typedef BOOL
(APIENTRY FN_DrvAssociateSharedSurface)(
    _In_ SURFOBJ* psoSurf,
    _In_ HANDLE   hPhysicalSurface,
    _In_ HANDLE   hLogicalSurface,
    _In_ SIZEL    sizl);
typedef FN_DrvAssociateSharedSurface *PFN_DrvAssociateSharedSurface;
extern FN_DrvAssociateSharedSurface DrvAssociateSharedSurface;

typedef LONG
(APIENTRY FN_DrvSynchronizeRedirectionBitmaps)(
    _In_ DHPDEV  dhpdev,
    _Out_ UINT64* puiFenceID);
typedef FN_DrvSynchronizeRedirectionBitmaps *PFN_DrvSynchronizeRedirectionBitmaps;
extern FN_DrvSynchronizeRedirectionBitmaps DrvSynchronizeRedirectionBitmaps;

#define WINDDI_MAX_BROADCAST_CONTEXT 64
typedef struct tagCDDDXGK_REDIRBITMAPPRESENTINFO
{
    UINT NumDirtyRects;
    PRECT DirtyRect;
    UINT NumContexts;
    HANDLE hContext[WINDDI_MAX_BROADCAST_CONTEXT+1];
} CDDDXGK_REDIRBITMAPPRESENTINFO;

typedef BOOL
(APIENTRY FN_DrvAccumulateD3DDirtyRect)(
    _In_ SURFOBJ* psoSurf,
    _In_ CDDDXGK_REDIRBITMAPPRESENTINFO* pDirty);
typedef FN_DrvAccumulateD3DDirtyRect *PFN_DrvAccumulateD3DDirtyRect;
extern FN_DrvAccumulateD3DDirtyRect DrvAccumulateD3DDirtyRect;

typedef BOOL
(APIENTRY FN_DrvStartDxInterop)(
    _In_ SURFOBJ* psoSurf,
    _In_ BOOL bDiscard,
    _In_ PVOID KernelModeDeviceHandle);
typedef FN_DrvStartDxInterop *PFN_DrvStartDxInterop;
extern FN_DrvStartDxInterop DrvStartDxInterop;

typedef BOOL
(APIENTRY FN_DrvEndDxInterop)(
    _In_ SURFOBJ* psoSurf,
    _In_ BOOL bDiscard,
    _Out_ BOOL* bDeviceLost,
    _In_ PVOID KernelModeDeviceHandle);
typedef FN_DrvEndDxInterop *PFN_DrvEndDxInterop;
extern FN_DrvEndDxInterop DrvEndDxInterop;

typedef VOID
(APIENTRY FN_DrvLockDisplayArea)(
    _In_ DHPDEV dhpdev,
    _In_opt_ RECTL  *prcl);
typedef FN_DrvLockDisplayArea *PFN_DrvLockDisplayArea;
extern FN_DrvLockDisplayArea DrvLockDisplayArea;

typedef VOID
(APIENTRY FN_DrvUnlockDisplayArea)(
    _In_ DHPDEV dhpdev,
    _In_opt_ RECTL *prcl);
typedef FN_DrvUnlockDisplayArea *PFN_DrvUnlockDisplayArea;
extern FN_DrvUnlockDisplayArea DrvUnlockDisplayArea;

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#ifdef __cplusplus
}
#endif

#endif /* defined __VIDEO_H__ */

#endif /* _WINDDI_ */
