/*
 * winddi.h
 *
 * GDI device driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
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

#ifndef __WINDDI_H
#define __WINDDI_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __VIDEO_H
#error video.h cannot be included with winddi.h
#else

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"
#include <wingdi.h>

#ifndef __DD_INCLUDED__
/* FIXME: Some DirectDraw structures not added yet */
typedef ULONG_PTR FLATPTR;
typedef struct _DD_SURFACECALLBACKS {
} DD_SURFACECALLBACKS, *PDD_SURFACECALLBACKS;
typedef struct _DD_PALETTECALLBACKS {
} DD_PALETTECALLBACKS, *PDD_PALETTECALLBACKS;
typedef struct _DD_CALLBACKS {
} DD_CALLBACKS, *PDD_CALLBACKS;
typedef struct _DD_HALINFO {
} DD_HALINFO, *PDD_HALINFO;
typedef struct _VIDEOMEMORY {
} VIDEOMEMORY, *LPVIDEOMEMORY;
typedef struct _DD_DIRECTDRAW_GLOBAL {
} DD_DIRECTDRAW_GLOBAL, *LPDD_DIRECTDRAW_GLOBAL;
typedef struct _DD_SURFACE_LOCAL {
} DD_SURFACE_LOCAL, *PDD_SURFACE_LOCAL, *LPDD_SURFACE_LOCAL;
#endif

#ifndef __DDRAWI_INCLUDED__
typedef PVOID LPVIDMEM;
#endif

#if !defined(__DD_INCLUDED__) && !defined(__DDRAWI_INCLUDED__)
typedef struct _DDSCAPS {
} DDSCAPS, *PDDSCAPS;
typedef struct _DDSCAPSEX {
} DDSCAPSEX, *PDDSCAPSEX;
typedef PVOID LPVMEMHEAP;
#endif

#if defined(_WIN32K_)
#define WIN32KAPI DECL_EXPORT
#else
#define WIN32KAPI DECL_IMPORT
#endif

#define DDI_DRIVER_VERSION_NT4            0x00020000
#define DDI_DRIVER_VERSION_SP3            0x00020003
#define DDI_DRIVER_VERSION_NT5            0x00030000
#define DDI_DRIVER_VERSION_NT5_01         0x00030100

#define GDI_DRIVER_VERSION                0x4000

#ifdef _X86_

typedef DWORD FLOATL;

#else /* !_X86_ */

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

typedef LONG_PTR (DDKAPI *PFN)();

DECLARE_HANDLE(HBM);
DECLARE_HANDLE(HDEV);
DECLARE_HANDLE(HSURF);
DECLARE_HANDLE(DHSURF);
DECLARE_HANDLE(DHPDEV);
DECLARE_HANDLE(HDRVOBJ);


#define GDI_DRIVER_VERSION                0x4000

typedef struct _ENG_EVENT *PEVENT;

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

#define DCR_SOLID                         0
#define DCR_DRIVER                        1
#define DCR_HALFTONE                      2

#define GX_IDENTITY                       0
#define GX_OFFSET                         1
#define GX_SCALE                          2
#define GX_GENERAL                        3

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
  RUN  arun[1];
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

typedef struct _DRIVEROBJ *PDRIVEROBJ;

typedef BOOL DDKAPI CALLBACK
(*FREEOBJPROC)(
  IN PDRIVEROBJ  pDriverObj);

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
#define INDEX_DrvGetGlyphMode             37L
#define INDEX_DrvSynchronize              38L
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
#define INDEX_LAST                        93L

typedef struct _DRVFN {
  ULONG  iFunc;
  PFN  pfn;
} DRVFN, *PDRVFN;

/* DRVENABLEDATA.iDriverVersion constants */
#define DDI_DRIVER_VERSION_NT4            0x00020000
#define DDI_DRIVER_VERSION_SP3            0x00020003
#define DDI_DRIVER_VERSION_NT5            0x00030000
#define DDI_DRIVER_VERSION_NT5_01         0x00030100

typedef struct _DRVENABLEDATA {
  ULONG  iDriverVersion;
  ULONG  c;
  DRVFN  *pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;

DECLARE_HANDLE(HSEMAPHORE);

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
  USHORT  usYear;
  USHORT  usMonth;
  USHORT  usDay;
  USHORT  usHour;
  USHORT  usMinute;
  USHORT  usSecond;
  USHORT  usMilliseconds;
  USHORT  usWeekday;
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
  BYTE  aGlyphAttr[1];
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
  WCRUN  awcrun[1];
} FD_GLYPHSET, *PFD_GLYPHSET;

typedef struct _FD_KERNINGPAIR {
  WCHAR  wcFirst;
  WCHAR  wcSecond;
  FWORD  fwdKern;
} FD_KERNINGPAIR;

typedef struct _FLOATOBJ
{
  ULONG  ul1;
  ULONG  ul2;
} FLOATOBJ, *PFLOATOBJ;

typedef struct _FLOATOBJ_XFORM {
  FLOATOBJ  eM11;
  FLOATOBJ  eM12;
  FLOATOBJ  eM21;
  FLOATOBJ  eM22;
  FLOATOBJ  eDx;
  FLOATOBJ  eDy;
} FLOATOBJ_XFORM, *PFLOATOBJ_XFORM, FAR *LPFLOATOBJ_XFORM;

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
  POINTFIX  *glypptfx;
} PATHDATA, *PPATHDATA;

/* PATHOBJ.fl constants */
#define PO_BEZIERS                        0x00000001
#define PO_ELLIPSE                        0x00000002
#define PO_ALL_INTEGERS                   0x00000004
#define PO_ENUM_AS_INTEGERS               0x00000008

typedef struct _PATHOBJ {
  FLONG  fl;
  ULONG  cCurves;
} PATHOBJ;

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

#define MAXCHARSETS                       16

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

typedef struct _SURFACEALIGNMENT {
  union {
		struct {
			DWORD  dwStartAlignment;
			DWORD  dwPitchAlignment;
			DWORD  dwReserved1;
			DWORD  dwReserved2;
		} Linear;
		struct {
			DWORD  dwXAlignment;
			DWORD  dwYAlignment;
			DWORD  dwReserved1;
			DWORD  dwReserved2;
		} Rectangular;
  };
} SURFACEALIGNMENT, *LPSURFACEALIGNMENT;

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
#define BMF_RESERVED                      0xFF00

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

typedef VOID DDKAPI (CALLBACK *WNDOBJCHANGEPROC)(
  IN WNDOBJ  *pwo,
  IN FLONG  fl);


WIN32KAPI
HANDLE
DDKAPI
BRUSHOBJ_hGetColorTransform(
  IN BRUSHOBJ  *pbo);

WIN32KAPI
PVOID
DDKAPI
BRUSHOBJ_pvAllocRbrush(
  IN BRUSHOBJ  *pbo,
  IN ULONG  cj);

WIN32KAPI
PVOID
DDKAPI
BRUSHOBJ_pvGetRbrush(
  IN BRUSHOBJ  *pbo);

WIN32KAPI
ULONG
DDKAPI
BRUSHOBJ_ulGetBrushColor(
  IN BRUSHOBJ  *pbo);

WIN32KAPI
BOOL
DDKAPI
CLIPOBJ_bEnum(
  IN CLIPOBJ  *pco,
  IN ULONG  cj,
  OUT ULONG  *pv);

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

WIN32KAPI
ULONG
DDKAPI
CLIPOBJ_cEnumStart(
  IN CLIPOBJ  *pco,
  IN BOOL  bAll,
  IN ULONG  iType,
  IN ULONG  iDirection,
  IN ULONG  cLimit);

WIN32KAPI
PATHOBJ*
DDKAPI
CLIPOBJ_ppoGetPath(
  IN CLIPOBJ  *pco);

WIN32KAPI
VOID
DDKAPI
EngAcquireSemaphore(
  IN HSEMAPHORE  hsem);

#define FL_ZERO_MEMORY                    0x00000001
#define FL_NONPAGED_MEMORY                0x00000002

WIN32KAPI
PVOID
DDKAPI
EngAllocMem(
  IN ULONG  Flags,
  IN ULONG  MemSize,
  IN ULONG  Tag);

WIN32KAPI
PVOID
DDKAPI
EngAllocPrivateUserMem(
  IN PDD_SURFACE_LOCAL  psl,
  IN SIZE_T  cj,
  IN ULONG  tag);

WIN32KAPI
PVOID
DDKAPI
EngAllocUserMem(
  IN SIZE_T  cj,
  IN ULONG  tag);

WIN32KAPI
BOOL
DDKAPI
EngAlphaBlend(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN BLENDOBJ  *pBlendObj);

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
#define HOOK_FLAGS                        0x0003b5ff

WIN32KAPI
BOOL
DDKAPI
EngAssociateSurface(
  IN HSURF  hsurf,
  IN HDEV  hdev,
  IN FLONG  flHooks);

WIN32KAPI
BOOL
DDKAPI
EngBitBlt(
  IN SURFOBJ  *psoTrg,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclTrg,
  IN POINTL  *pptlSrc,
  IN POINTL  *pptlMask,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrush,
  IN ROP4  rop4);

WIN32KAPI
BOOL
DDKAPI
EngCheckAbort(
  IN SURFOBJ  *pso);

WIN32KAPI
VOID
DDKAPI
EngClearEvent(
  IN PEVENT  pEvent);

WIN32KAPI
FD_GLYPHSET*
DDKAPI
EngComputeGlyphSet(
  IN INT  nCodePage,
  IN INT  nFirstChar,
  IN INT  cChars);

/* EngControlSprites.fl constants */
#define ECS_TEARDOWN                      0x00000001
#define ECS_REDRAW                        0x00000002

WIN32KAPI
BOOL
DDKAPI
EngControlSprites(
  IN WNDOBJ  *pwo,
  IN FLONG  fl);

WIN32KAPI
BOOL
DDKAPI
EngCopyBits(
  OUT SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDest,
  IN POINTL  *pptlSrc);

WIN32KAPI
HBITMAP
DDKAPI
EngCreateBitmap(
  IN SIZEL  sizl,
  IN LONG  lWidth,
  IN ULONG  iFormat,
  IN FLONG  fl,
  IN PVOID  pvBits);

WIN32KAPI
CLIPOBJ*
DDKAPI
EngCreateClip(
  VOID);

WIN32KAPI
HBITMAP
DDKAPI
EngCreateDeviceBitmap(
  IN DHSURF  dhsurf,
  IN SIZEL  sizl,
  IN ULONG  iFormatCompat);

WIN32KAPI
HSURF
DDKAPI
EngCreateDeviceSurface(
  DHSURF  dhsurf,
  SIZEL  sizl,
  ULONG  iFormatCompat);

#if 0
WIN32KAPI
HDRVOBJ
DDKAPI
EngCreateDriverObj(
  PVOID  pvObj,
  FREEOBJPROC  pFreeObjProc,
  HDEV  hdev);
#endif

WIN32KAPI
BOOL
DDKAPI
EngCreateEvent(
  OUT PEVENT  *ppEvent);

/* EngCreatePalette.iMode constants */
#define PAL_INDEXED                       0x00000001
#define PAL_BITFIELDS                     0x00000002
#define PAL_RGB                           0x00000004
#define PAL_BGR                           0x00000008
#define PAL_CMYK                          0x00000010

WIN32KAPI
HPALETTE
DDKAPI
EngCreatePalette(
  IN ULONG  iMode,
  IN ULONG  cColors,
  IN ULONG  *pulColors,
  IN FLONG  flRed,
  IN FLONG  flGreen,
  IN FLONG  flBlue);

WIN32KAPI
PATHOBJ*
DDKAPI
EngCreatePath(
  VOID);

WIN32KAPI
HSEMAPHORE
DDKAPI
EngCreateSemaphore(
  VOID);

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

WIN32KAPI
WNDOBJ*
DDKAPI
EngCreateWnd(
  SURFOBJ  *pso,
  HWND  hwnd,
  WNDOBJCHANGEPROC  pfn,
  FLONG  fl,
  int  iPixelFormat);

WIN32KAPI
VOID
DDKAPI
EngDebugBreak(
  VOID);

WIN32KAPI
VOID
DDKAPI
EngDebugPrint(
  IN PCHAR StandardPrefix,
  IN PCHAR DebugMessage,
  IN va_list ap);

WIN32KAPI
VOID
DDKAPI
EngDeleteClip(
  IN CLIPOBJ  *pco);

WIN32KAPI
BOOL
DDKAPI
EngDeleteDriverObj(
  IN HDRVOBJ  hdo,
  IN BOOL  bCallBack,
  IN BOOL  bLocked);

WIN32KAPI
BOOL
DDKAPI
EngDeleteEvent(
  IN PEVENT  pEvent);

WIN32KAPI
BOOL
DDKAPI
EngDeleteFile(
  IN LPWSTR  pwszFileName);

WIN32KAPI
BOOL
DDKAPI
EngDeletePalette(
  IN HPALETTE  hpal);

WIN32KAPI
VOID
DDKAPI
EngDeletePath(
  IN PATHOBJ  *ppo);

WIN32KAPI
VOID
DDKAPI
EngDeleteSafeSemaphore(
  IN OUT ENGSAFESEMAPHORE  *pssem);

WIN32KAPI
VOID
DDKAPI
EngDeleteSemaphore(
  IN OUT HSEMAPHORE  hsem);

WIN32KAPI
BOOL
DDKAPI
EngDeleteSurface(
  IN HSURF  hsurf);

WIN32KAPI
VOID
DDKAPI
EngDeleteWnd(
  IN WNDOBJ  *pwo);

WIN32KAPI
DWORD
DDKAPI
EngDeviceIoControl(
  IN HANDLE  hDevice,
  IN DWORD  dwIoControlCode,
  IN LPVOID  lpInBuffer,
  IN DWORD  nInBufferSize,
  IN OUT LPVOID  lpOutBuffer,
  IN DWORD  nOutBufferSize,
  OUT LPDWORD  lpBytesReturned);

WIN32KAPI
ULONG
DDKAPI
EngDitherColor(
  IN HDEV  hdev,
  IN ULONG  iMode,
  IN ULONG  rgb,
  OUT ULONG  *pul);

WIN32KAPI
BOOL
DDKAPI
EngEnumForms(
  IN HANDLE  hPrinter,
  IN DWORD  Level,
  OUT LPBYTE  pForm,
  IN DWORD  cbBuf,
  OUT LPDWORD  pcbNeeded,
  OUT LPDWORD  pcReturned);

WIN32KAPI
BOOL
DDKAPI
EngEraseSurface(
  IN SURFOBJ  *pso,
  IN RECTL  *prcl,
  IN ULONG  iColor);

WIN32KAPI
BOOL
DDKAPI
EngFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix,
  IN FLONG  flOptions);

WIN32KAPI
PVOID
DDKAPI
EngFindImageProcAddress(
  IN HANDLE  hModule,
  IN LPSTR  lpProcName);

WIN32KAPI
PVOID
DDKAPI
EngFindResource(
  IN HANDLE  h,
  IN int  iName,
  IN int  iType,
  OUT PULONG  pulSize);

WIN32KAPI
PVOID
DDKAPI
EngFntCacheAlloc(
  IN ULONG  FastCheckSum,
  IN ULONG  ulSize);

/* EngFntCacheFault.iFaultMode constants */
#define ENG_FNT_CACHE_READ_FAULT          0x00000001
#define ENG_FNT_CACHE_WRITE_FAULT         0x00000002

WIN32KAPI
VOID
DDKAPI
EngFntCacheFault(
  IN ULONG  ulFastCheckSum,
  IN ULONG  iFaultMode);

WIN32KAPI
PVOID
DDKAPI
EngFntCacheLookUp(
  IN ULONG  FastCheckSum,
  OUT ULONG  *pulSize);

WIN32KAPI
VOID
DDKAPI
EngFreeMem(
  IN PVOID  Mem);

WIN32KAPI
VOID
DDKAPI
EngFreeModule(
  IN HANDLE  h);

WIN32KAPI
VOID
DDKAPI
EngFreePrivateUserMem(
  IN PDD_SURFACE_LOCAL  psl,
  IN PVOID  pv);

WIN32KAPI
VOID
DDKAPI
EngFreeUserMem(
  IN PVOID  pv);

WIN32KAPI
VOID
DDKAPI
EngGetCurrentCodePage(
  OUT PUSHORT  OemCodePage,
  OUT PUSHORT  AnsiCodePage);

WIN32KAPI
HANDLE
DDKAPI
EngGetCurrentProcessId(
  VOID);

WIN32KAPI
HANDLE
DDKAPI
EngGetCurrentThreadId(
  VOID);

WIN32KAPI
LPWSTR
DDKAPI
EngGetDriverName(
  IN HDEV  hdev);

WIN32KAPI
BOOL
DDKAPI
EngGetFileChangeTime(
  IN HANDLE  h,
  OUT LARGE_INTEGER  *pChangeTime);

WIN32KAPI
BOOL
DDKAPI
EngGetFilePath(
  IN HANDLE  h,
  OUT WCHAR  (*pDest)[MAX_PATH+1]);

WIN32KAPI
BOOL
DDKAPI
EngGetForm(
  IN HANDLE  hPrinter,
  IN LPWSTR  pFormName,
  IN DWORD  Level,
  OUT LPBYTE  pForm,
  IN DWORD  cbBuf,
  OUT LPDWORD  pcbNeeded);

WIN32KAPI
ULONG
DDKAPI
EngGetLastError(
  VOID);

WIN32KAPI
BOOL
DDKAPI
EngGetPrinter(
  IN HANDLE  hPrinter,
  IN DWORD  dwLevel,
  OUT LPBYTE  pPrinter,
  IN DWORD  cbBuf,
  OUT LPDWORD  pcbNeeded);

WIN32KAPI
DWORD
DDKAPI
EngGetPrinterData(
  IN HANDLE  hPrinter,
  IN LPWSTR  pValueName,
  OUT LPDWORD  pType,
  OUT LPBYTE  pData,
  IN DWORD  nSize,
  OUT LPDWORD  pcbNeeded);

WIN32KAPI
LPWSTR
DDKAPI
EngGetPrinterDataFileName(
  IN HDEV  hdev);

WIN32KAPI
BOOL
DDKAPI
EngGetPrinterDriver(
  IN HANDLE  hPrinter,
  IN LPWSTR  pEnvironment,
  IN DWORD  dwLevel,
  OUT BYTE  *lpbDrvInfo,
  IN DWORD  cbBuf,
  OUT DWORD  *pcbNeeded);

WIN32KAPI
HANDLE
DDKAPI
EngGetProcessHandle(
  VOID);

WIN32KAPI
BOOL
DDKAPI
EngGetType1FontList(
  IN HDEV  hdev,
  OUT TYPE1_FONT  *pType1Buffer,
  IN ULONG  cjType1Buffer,
  OUT PULONG  pulLocalFonts,
  OUT PULONG  pulRemoteFonts,
  OUT LARGE_INTEGER  *pLastModified);

WIN32KAPI
BOOL
DDKAPI
EngGradientFill(
  IN SURFOBJ  *psoDest,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN TRIVERTEX  *pVertex,
  IN ULONG  nVertex,
  IN PVOID  pMesh,
  IN ULONG  nMesh,
  IN RECTL  *prclExtents,
  IN POINTL  *pptlDitherOrg,
  IN ULONG  ulMode);

/* EngHangNotification return values */
#define EHN_RESTORED                      0x00000000
#define EHN_ERROR                         0x00000001

WIN32KAPI
ULONG
DDKAPI
EngHangNotification(
  IN HDEV  hDev,
  IN PVOID  Reserved);

WIN32KAPI
BOOL
DDKAPI
EngInitializeSafeSemaphore(
  OUT ENGSAFESEMAPHORE  *pssem);

WIN32KAPI
BOOL
DDKAPI
EngIsSemaphoreOwned(
  IN HSEMAPHORE  hsem);

WIN32KAPI
BOOL
DDKAPI
EngIsSemaphoreOwnedByCurrentThread(
  IN HSEMAPHORE  hsem);

WIN32KAPI
BOOL
DDKAPI
EngLineTo(
  SURFOBJ  *pso,
  CLIPOBJ  *pco,
  BRUSHOBJ  *pbo,
  LONG  x1,
  LONG  y1,
  LONG  x2,
  LONG  y2,
  RECTL  *prclBounds,
  MIX  mix);

WIN32KAPI
HANDLE
DDKAPI
EngLoadImage(
  IN LPWSTR  pwszDriver);

WIN32KAPI
HANDLE
DDKAPI
EngLoadModule(
  IN LPWSTR  pwsz);

WIN32KAPI
HANDLE
DDKAPI
EngLoadModuleForWrite(
  IN LPWSTR  pwsz,
  IN ULONG  cjSizeOfModule);

WIN32KAPI
PDD_SURFACE_LOCAL
DDKAPI
EngLockDirectDrawSurface(
  IN HANDLE  hSurface);

WIN32KAPI
DRIVEROBJ*
DDKAPI
EngLockDriverObj(
  IN HDRVOBJ  hdo);

WIN32KAPI
SURFOBJ*
DDKAPI
EngLockSurface(
  IN HSURF  hsurf);

WIN32KAPI
BOOL
DDKAPI
EngLpkInstalled(
  VOID);

WIN32KAPI
PEVENT
DDKAPI
EngMapEvent(
  IN HDEV  hDev,
  IN HANDLE  hUserObject,
  IN PVOID  Reserved1,
  IN PVOID  Reserved2,
  IN PVOID  Reserved3);

WIN32KAPI
PVOID
DDKAPI
EngMapFile(
  IN LPWSTR  pwsz,
  IN ULONG  cjSize,
  OUT ULONG_PTR  *piFile);

WIN32KAPI
BOOL
DDKAPI
EngMapFontFile(
  ULONG_PTR  iFile,
  PULONG  *ppjBuf,
  ULONG  *pcjBuf);

WIN32KAPI
BOOL
DDKAPI
EngMapFontFileFD(
  IN ULONG_PTR  iFile,
  OUT PULONG  *ppjBuf,
  OUT ULONG  *pcjBuf);

WIN32KAPI
PVOID
DDKAPI
EngMapModule(
  IN HANDLE  h,
  OUT PULONG  pSize);

WIN32KAPI
BOOL
DDKAPI
EngMarkBandingSurface(
  IN HSURF  hsurf);

/* EngModifySurface.flSurface constants */
#define MS_NOTSYSTEMMEMORY                0x00000001
#define MS_SHAREDACCESS                   0x00000002

WIN32KAPI
BOOL
DDKAPI
EngModifySurface(
  IN HSURF  hsurf,
  IN HDEV  hdev,
  IN FLONG  flHooks,
  IN FLONG  flSurface,
  IN DHSURF  dhsurf,
  IN VOID  *pvScan0,
  IN LONG  lDelta,
  IN VOID  *pvReserved);

WIN32KAPI
VOID
DDKAPI
EngMovePointer(
  IN SURFOBJ  *pso,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl);

WIN32KAPI
int
DDKAPI
EngMulDiv(
  IN int  a,
  IN int  b,
  IN int  c);

WIN32KAPI
VOID
DDKAPI
EngMultiByteToUnicodeN(
  OUT LPWSTR  UnicodeString,
  IN ULONG  MaxBytesInUnicodeString,
  OUT PULONG  BytesInUnicodeString,
  IN PCHAR  MultiByteString,
  IN ULONG  BytesInMultiByteString);

WIN32KAPI
INT
DDKAPI
EngMultiByteToWideChar(
  IN UINT  CodePage,
  OUT LPWSTR  WideCharString,
  IN INT  BytesInWideCharString,
  IN LPSTR  MultiByteString,
  IN INT  BytesInMultiByteString);

WIN32KAPI
BOOL
DDKAPI
EngPaint(
  IN SURFOBJ  *pso,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix);

WIN32KAPI
BOOL
DDKAPI
EngPlgBlt(
  IN SURFOBJ  *psoTrg,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMsk,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlBrushOrg,
  IN POINTFIX  *pptfx,
  IN RECTL  *prcl,
  IN POINTL  *pptl,
  IN ULONG  iMode);

WIN32KAPI
VOID
DDKAPI
EngProbeForRead(
  IN PVOID  Address,
  IN ULONG  Length,
  IN ULONG  Alignment);

WIN32KAPI
VOID
DDKAPI
EngProbeForReadAndWrite(
  IN PVOID  Address,
  IN ULONG  Length,
  IN ULONG  Alignment);

typedef enum _ENG_DEVICE_ATTRIBUTE {
  QDA_RESERVED = 0,
  QDA_ACCELERATION_LEVEL
} ENG_DEVICE_ATTRIBUTE;

WIN32KAPI
BOOL
DDKAPI
EngQueryDeviceAttribute(
  IN HDEV  hdev,
  IN ENG_DEVICE_ATTRIBUTE  devAttr,
  IN VOID  *pvIn,
  IN ULONG  ulInSize,
  OUT VOID  *pvOut,
  OUT ULONG  ulOutSize);

WIN32KAPI
LARGE_INTEGER
DDKAPI
EngQueryFileTimeStamp(
  IN LPWSTR  pwsz);

WIN32KAPI
VOID
DDKAPI
EngQueryLocalTime(
  OUT PENG_TIME_FIELDS  ptf);

WIN32KAPI
ULONG
DDKAPI
EngQueryPalette(
  IN HPALETTE  hPal,
  OUT ULONG  *piMode,
  IN ULONG  cColors,
  OUT ULONG  *pulColors);

WIN32KAPI
VOID
DDKAPI
EngQueryPerformanceCounter(
  OUT LONGLONG  *pPerformanceCount);

WIN32KAPI
VOID
DDKAPI
EngQueryPerformanceFrequency(
  OUT LONGLONG  *pFrequency);

typedef enum _ENG_SYSTEM_ATTRIBUTE {
  EngProcessorFeature = 1,
  EngNumberOfProcessors,
  EngOptimumAvailableUserMemory,
  EngOptimumAvailableSystemMemory,
} ENG_SYSTEM_ATTRIBUTE;

#define QSA_MMX                           0x00000100
#define QSA_SSE                           0x00002000
#define QSA_3DNOW                         0x00004000

WIN32KAPI
BOOL
DDKAPI
EngQuerySystemAttribute(
  IN ENG_SYSTEM_ATTRIBUTE  CapNum,
  OUT PDWORD  pCapability);

WIN32KAPI
LONG
DDKAPI
EngReadStateEvent(
  IN PEVENT  pEvent);

WIN32KAPI
VOID
DDKAPI
EngReleaseSemaphore(
  IN HSEMAPHORE  hsem);

WIN32KAPI
BOOL
DDKAPI
EngRestoreFloatingPointState(
  IN VOID  *pBuffer);

WIN32KAPI
ULONG
DDKAPI
EngSaveFloatingPointState(
  OUT VOID  *pBuffer,
  IN ULONG  cjBufferSize);

WIN32KAPI
HANDLE
DDKAPI
EngSecureMem(
  IN PVOID  Address,
  IN ULONG  Length);

WIN32KAPI
LONG
DDKAPI
EngSetEvent(
  IN PEVENT  pEvent);

WIN32KAPI
VOID
DDKAPI
EngSetLastError(
  IN ULONG  iError);

WIN32KAPI
ULONG
DDKAPI
EngSetPointerShape(
  IN SURFOBJ  *pso,
  IN SURFOBJ  *psoMask,
  IN SURFOBJ  *psoColor,
  IN XLATEOBJ  *pxlo,
  IN LONG  xHot,
  IN LONG  yHot,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl,
  IN FLONG  fl);

WIN32KAPI
BOOL
DDKAPI
EngSetPointerTag(
  IN HDEV  hdev,
  IN SURFOBJ  *psoMask,
  IN SURFOBJ  *psoColor,
  IN XLATEOBJ  *pxlo,
  IN FLONG  fl);

WIN32KAPI
DWORD
DDKAPI
EngSetPrinterData(
  IN HANDLE  hPrinter,
  IN LPWSTR  pType,
  IN DWORD  dwType,
  IN LPBYTE  lpbPrinterData,
  IN DWORD  cjPrinterData);

typedef int DDKCDECLAPI (*SORTCOMP)(const void *pv1, const void *pv2);

WIN32KAPI
VOID
DDKAPI
EngSort(
  IN OUT PBYTE  pjBuf,
  IN ULONG  c,
  IN ULONG  cjElem,
  IN SORTCOMP  pfnComp);

WIN32KAPI
BOOL
DDKAPI
EngStretchBlt(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlHTOrg,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN POINTL  *pptlMask,
  IN ULONG  iMode);

WIN32KAPI
BOOL
DDKAPI
EngStretchBltROP(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlHTOrg,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN POINTL  *pptlMask,
  IN ULONG  iMode,
  IN BRUSHOBJ  *pbo,
  IN DWORD  rop4);

WIN32KAPI
BOOL
DDKAPI
EngStrokeAndFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pboStroke,
  IN LINEATTRS  *plineattrs,
  IN BRUSHOBJ  *pboFill,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mixFill,
  IN FLONG  flOptions);

WIN32KAPI
BOOL
DDKAPI
EngStrokePath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN LINEATTRS  *plineattrs,
  IN MIX  mix);

WIN32KAPI
BOOL
DDKAPI
EngTextOut(
  IN SURFOBJ  *pso,
  IN STROBJ  *pstro,
  IN FONTOBJ  *pfo,
  IN CLIPOBJ  *pco,
  IN RECTL  *prclExtra,
  IN RECTL  *prclOpaque,
  IN BRUSHOBJ  *pboFore,
  IN BRUSHOBJ  *pboOpaque,
  IN POINTL  *pptlOrg,
  IN MIX  mix);

WIN32KAPI
BOOL
DDKAPI
EngTransparentBlt(
  IN SURFOBJ  *psoDst,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDst,
  IN RECTL  *prclSrc,
  IN ULONG  iTransColor,
  IN ULONG  ulReserved);

WIN32KAPI
VOID
DDKAPI
EngUnicodeToMultiByteN(
  OUT PCHAR  MultiByteString,
  IN ULONG  MaxBytesInMultiByteString,
  OUT PULONG  BytesInMultiByteString,
  IN PWSTR  UnicodeString,
  IN ULONG  BytesInUnicodeString);

WIN32KAPI
VOID
DDKAPI
EngUnloadImage(
  IN HANDLE  hModule);

WIN32KAPI
BOOL
DDKAPI
EngUnlockDirectDrawSurface(
  IN PDD_SURFACE_LOCAL  pSurface);

WIN32KAPI
BOOL
DDKAPI
EngUnlockDriverObj(
  IN HDRVOBJ  hdo);

WIN32KAPI
VOID
DDKAPI
EngUnlockSurface(
  IN SURFOBJ  *pso);

WIN32KAPI
BOOL
DDKAPI
EngUnmapEvent(
  IN PEVENT  pEvent);

WIN32KAPI
BOOL
DDKAPI
EngUnmapFile(
  IN ULONG_PTR  iFile);

WIN32KAPI
VOID
DDKAPI
EngUnmapFontFile(
  ULONG_PTR  iFile);

WIN32KAPI
VOID
DDKAPI
EngUnmapFontFileFD(
  IN ULONG_PTR  iFile);

WIN32KAPI
VOID
DDKAPI
EngUnsecureMem(
  IN HANDLE  hSecure);

WIN32KAPI
BOOL
DDKAPI
EngWaitForSingleObject(
  IN PEVENT  pEvent,
  IN PLARGE_INTEGER  pTimeOut);

WIN32KAPI
INT
DDKAPI
EngWideCharToMultiByte(
  IN UINT  CodePage,
  IN LPWSTR  WideCharString,
  IN INT  BytesInWideCharString,
  OUT LPSTR  MultiByteString,
  IN INT  BytesInMultiByteString);

WIN32KAPI
BOOL
DDKAPI
EngWritePrinter(
  IN HANDLE  hPrinter,
  IN LPVOID  pBuf,
  IN DWORD  cbBuf,
  OUT LPDWORD  pcWritten);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_Add(
  IN OUT PFLOATOBJ  pf,
  IN PFLOATOBJ  pf1);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_AddFloat(
  IN OUT PFLOATOBJ  pf,
  IN FLOATL  f);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_AddLong(
  IN OUT PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_Div(
  IN OUT PFLOATOBJ  pf,
  IN PFLOATOBJ  pf1);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_DivFloat(
  IN OUT PFLOATOBJ  pf,
  IN FLOATL  f);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_DivLong(
  IN OUT PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
BOOL
DDKAPI
FLOATOBJ_Equal(
  IN PFLOATOBJ  pf,
  IN PFLOATOBJ  pf1);

WIN32KAPI
BOOL
DDKAPI
FLOATOBJ_EqualLong(
  IN PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
LONG
DDKAPI
FLOATOBJ_GetFloat(
  IN PFLOATOBJ  pf);

WIN32KAPI
LONG
DDKAPI
FLOATOBJ_GetLong(
  IN PFLOATOBJ  pf);

WIN32KAPI
BOOL
DDKAPI
FLOATOBJ_GreaterThan(
  IN PFLOATOBJ  pf,
  IN PFLOATOBJ  pf1);

WIN32KAPI
BOOL
DDKAPI
FLOATOBJ_GreaterThanLong(
  IN PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
BOOL
DDKAPI
FLOATOBJ_LessThan(
  IN PFLOATOBJ  pf,
  IN PFLOATOBJ  pf1);

WIN32KAPI
BOOL
DDKAPI
FLOATOBJ_LessThanLong(
  IN PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_Mul(
  IN OUT PFLOATOBJ  pf,
  IN PFLOATOBJ  pf1);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_MulFloat(
  IN OUT PFLOATOBJ  pf,
  IN FLOATL  f);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_MulLong(
  IN OUT PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_Neg(
  IN OUT PFLOATOBJ  pf);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_SetFloat(
  OUT PFLOATOBJ  pf,
  IN FLOATL  f);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_SetLong(
  OUT PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_Sub(
  IN OUT PFLOATOBJ  pf,
  IN PFLOATOBJ  pf1);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_SubFloat(
  IN OUT PFLOATOBJ  pf,
  IN FLOATL  f);

WIN32KAPI
VOID
DDKAPI
FLOATOBJ_SubLong(
  IN OUT PFLOATOBJ  pf,
  IN LONG  l);

WIN32KAPI
ULONG
DDKAPI
FONTOBJ_cGetAllGlyphHandles(
  IN FONTOBJ  *pfo,
  OUT HGLYPH  *phg);

WIN32KAPI
ULONG
DDKAPI
FONTOBJ_cGetGlyphs(
  IN FONTOBJ  *pfo,
  IN ULONG  iMode,
  IN ULONG  cGlyph,
  IN HGLYPH  *phg,
  OUT PVOID  *ppvGlyph);

WIN32KAPI
FD_GLYPHSET*
DDKAPI
FONTOBJ_pfdg(
  IN FONTOBJ  *pfo);

WIN32KAPI
IFIMETRICS*
DDKAPI
FONTOBJ_pifi(
  IN FONTOBJ  *pfo);

WIN32KAPI
PBYTE
DDKAPI
FONTOBJ_pjOpenTypeTablePointer(
  IN FONTOBJ  *pfo,
  IN ULONG  ulTag,
  OUT ULONG  *pcjTable);

WIN32KAPI
PFD_GLYPHATTR
DDKAPI 
FONTOBJ_pQueryGlyphAttrs(
  IN FONTOBJ  *pfo,
  IN ULONG  iMode);

WIN32KAPI
PVOID
DDKAPI
FONTOBJ_pvTrueTypeFontFile(
  IN FONTOBJ  *pfo,
  OUT ULONG  *pcjFile);

WIN32KAPI
LPWSTR
DDKAPI
FONTOBJ_pwszFontFilePaths(
  IN FONTOBJ  *pfo,
  OUT ULONG  *pcwc);

WIN32KAPI
XFORMOBJ*
DDKAPI
FONTOBJ_pxoGetXform(
  IN FONTOBJ  *pfo);

WIN32KAPI
VOID
DDKAPI
FONTOBJ_vGetInfo(
  IN FONTOBJ  *pfo,
  IN ULONG  cjSize,
  OUT FONTINFO  *pfi);

WIN32KAPI
FLATPTR
DDKAPI
HeapVidMemAllocAligned(
  IN LPVIDMEM  lpVidMem,
  IN DWORD  dwWidth,
  IN DWORD  dwHeight,
  IN LPSURFACEALIGNMENT  lpAlignment,
  OUT LPLONG  lpNewPitch);

WIN32KAPI
LONG
DDKAPI
HT_ComputeRGBGammaTable(
  IN USHORT  GammaTableEntries,
  IN USHORT  GammaTableType,
  IN USHORT  RedGamma,
  IN USHORT  GreenGamma,
  IN USHORT  BlueGamma,
  OUT LPBYTE  pGammaTable);

WIN32KAPI
LONG
DDKAPI
HT_Get8BPPFormatPalette(
  OUT LPPALETTEENTRY  pPaletteEntry,
  IN USHORT  RedGamma,
  IN USHORT  GreenGamma,
  IN USHORT  BlueGamma);

WIN32KAPI
LONG
DDKAPI
HT_Get8BPPMaskPalette(
  IN OUT LPPALETTEENTRY  pPaletteEntry,
  IN BOOL  Use8BPPMaskPal,
  IN BYTE  CMYMask,
  IN USHORT  RedGamma,
  IN USHORT  GreenGamma,
  IN USHORT  BlueGamma);

WIN32KAPI
LONG
DDKAPI
HTUI_DeviceColorAdjustment(
  IN LPSTR  pDeviceName,
  OUT PDEVHTADJDATA  pDevHTAdjData);

WIN32KAPI
ULONG
DDKAPI
PALOBJ_cGetColors(
  IN PALOBJ  *ppalo,
  IN ULONG  iStart,
  IN ULONG  cColors,
  OUT ULONG  *pulColors);

WIN32KAPI
BOOL
DDKAPI
PATHOBJ_bCloseFigure(
  IN PATHOBJ  *ppo);

WIN32KAPI
BOOL
DDKAPI
PATHOBJ_bEnum(
  IN PATHOBJ  *ppo,
  OUT PATHDATA  *ppd);

WIN32KAPI
BOOL
DDKAPI
PATHOBJ_bEnumClipLines(
  IN PATHOBJ  *ppo,
  IN ULONG  cb,
  OUT CLIPLINE  *pcl);

WIN32KAPI
BOOL
DDKAPI
PATHOBJ_bMoveTo(
  IN PATHOBJ  *ppo,
  IN POINTFIX  ptfx);

WIN32KAPI
BOOL
DDKAPI
PATHOBJ_bPolyBezierTo(
  IN PATHOBJ  *ppo,
  IN POINTFIX  *pptfx,
  IN ULONG  cptfx);

WIN32KAPI
BOOL
DDKAPI
PATHOBJ_bPolyLineTo(
  IN PATHOBJ  *ppo,
  IN POINTFIX  *pptfx,
  IN ULONG  cptfx);

WIN32KAPI
VOID
DDKAPI
PATHOBJ_vEnumStart(
  IN PATHOBJ  *ppo);

WIN32KAPI
VOID
DDKAPI
PATHOBJ_vEnumStartClipLines(
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN SURFOBJ  *pso,
  IN LINEATTRS  *pla);

WIN32KAPI
VOID
DDKAPI
PATHOBJ_vGetBounds(
  IN PATHOBJ  *ppo,
  OUT PRECTFX  prectfx);

WIN32KAPI
BOOL
DDKAPI
STROBJ_bEnum(
  IN STROBJ  *pstro,
  OUT ULONG  *pc,
  OUT PGLYPHPOS  *ppgpos);

WIN32KAPI
BOOL
DDKAPI
STROBJ_bEnumPositionsOnly(
  IN STROBJ  *pstro,
  OUT ULONG  *pc,
  OUT PGLYPHPOS  *ppgpos);

WIN32KAPI
BOOL
DDKAPI
STROBJ_bGetAdvanceWidths(
  IN STROBJ  *pso,
  IN ULONG  iFirst,
  IN ULONG  c,
  OUT POINTQF  *pptqD);

WIN32KAPI
DWORD
DDKAPI
STROBJ_dwGetCodePage(
  IN STROBJ  *pstro);

WIN32KAPI
FIX
DDKAPI
STROBJ_fxBreakExtra(
  IN STROBJ  *pstro);

WIN32KAPI
FIX
DDKAPI
STROBJ_fxCharacterExtra(
  IN STROBJ  *pstro);

WIN32KAPI
VOID
DDKAPI
STROBJ_vEnumStart(
  IN STROBJ  *pstro);

WIN32KAPI
VOID
DDKAPI
VidMemFree(
  IN LPVMEMHEAP  pvmh,
  IN FLATPTR  ptr);

WIN32KAPI
BOOL
DDKAPI
WNDOBJ_bEnum(
  IN WNDOBJ  *pwo,
  IN ULONG  cj,
  OUT ULONG  *pul);

WIN32KAPI
ULONG
DDKAPI
WNDOBJ_cEnumStart(
  IN WNDOBJ  *pwo,
  IN ULONG  iType,
  IN ULONG  iDirection,
  IN ULONG  cLimit);

WIN32KAPI
VOID
DDKAPI
WNDOBJ_vSetConsumer(
  IN WNDOBJ  *pwo,
  IN PVOID  pvConsumer);

/* XFORMOBJ_bApplyXform.iMode constants */
#define XF_LTOL                           0L
#define XF_INV_LTOL                       1L
#define XF_LTOFX                          2L
#define XF_INV_FXTOL                      3L

WIN32KAPI
BOOL
DDKAPI
XFORMOBJ_bApplyXform(
  IN XFORMOBJ  *pxo,
  IN ULONG  iMode,
  IN ULONG  cPoints,
  IN PVOID  pvIn,
  OUT PVOID  pvOut);

WIN32KAPI
ULONG
DDKAPI
XFORMOBJ_iGetFloatObjXform(
  IN XFORMOBJ  *pxo,
  OUT FLOATOBJ_XFORM  *pxfo);

WIN32KAPI
ULONG
DDKAPI
XFORMOBJ_iGetXform(
  IN XFORMOBJ  *pxo,
  OUT XFORML  *pxform);

/* XLATEOBJ_cGetPalette.iPal constants */
#define XO_SRCPALETTE                     1
#define XO_DESTPALETTE                    2
#define XO_DESTDCPALETTE                  3
#define XO_SRCBITFIELDS                   4
#define XO_DESTBITFIELDS                  5

WIN32KAPI
ULONG
DDKAPI
XLATEOBJ_cGetPalette(
  IN XLATEOBJ  *pxlo,
  IN ULONG  iPal,
  IN ULONG  cPal,
  OUT ULONG  *pPal);

WIN32KAPI
HANDLE
DDKAPI
XLATEOBJ_hGetColorTransform(
  IN XLATEOBJ  *pxlo);

WIN32KAPI
ULONG
DDKAPI
XLATEOBJ_iXlate(
  IN XLATEOBJ  *pxlo,
  IN ULONG  iColor);

WIN32KAPI
ULONG*
DDKAPI
XLATEOBJ_piVector(
  IN XLATEOBJ  *pxlo);



/* Graphics Driver Functions */

BOOL
DDKAPI
DrvAlphaBlend(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN BLENDOBJ  *pBlendObj);

BOOL
DDKAPI
DrvAssertMode(
  IN DHPDEV  dhpdev,
  IN BOOL  bEnable);

BOOL
DDKAPI
DrvBitBlt(
  IN SURFOBJ  *psoTrg,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclTrg,
  IN POINTL  *pptlSrc,
  IN POINTL  *pptlMask,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrush,
  IN ROP4  rop4);

VOID
DDKAPI
DrvCompletePDEV(
  IN DHPDEV  dhpdev,
  IN HDEV  hdev);

BOOL
DDKAPI
DrvCopyBits(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDest,
  IN POINTL  *pptlSrc);

HBITMAP
DDKAPI
DrvCreateDeviceBitmap(
  IN DHPDEV  dhpdev,
  IN SIZEL  sizl,
  IN ULONG  iFormat);

VOID
DDKAPI
DrvDeleteDeviceBitmap(
  IN DHSURF  dhsurf);

HBITMAP
DDKAPI
DrvDeriveSurface(
  DD_DIRECTDRAW_GLOBAL  *pDirectDraw,
  DD_SURFACE_LOCAL  *pSurface);

LONG
DDKAPI
DrvDescribePixelFormat(
  IN DHPDEV  dhpdev,
  IN LONG  iPixelFormat,
  IN ULONG  cjpfd,
  OUT PIXELFORMATDESCRIPTOR  *ppfd);

VOID
DDKAPI
DrvDestroyFont(
  IN FONTOBJ  *pfo);

VOID
DDKAPI
DrvDisableDriver(
  VOID);

VOID
DDKAPI
DrvDisablePDEV(
  IN DHPDEV  dhpdev);

VOID
DDKAPI
DrvDisableSurface(
  IN DHPDEV  dhpdev);

#define DM_DEFAULT                        0x00000001
#define DM_MONOCHROME                     0x00000002

ULONG
DDKAPI
DrvDitherColor(
  IN DHPDEV  dhpdev,
  IN ULONG  iMode,
  IN ULONG  rgb,
  OUT ULONG  *pul);

ULONG
DDKAPI
DrvDrawEscape(
  IN SURFOBJ  *pso,
  IN ULONG  iEsc,
  IN CLIPOBJ  *pco,
  IN RECTL  *prcl,
  IN ULONG  cjIn,
  IN PVOID  pvIn);

BOOL
DDKAPI
DrvEnableDriver(
  IN ULONG  iEngineVersion,
  IN ULONG  cj,
  OUT DRVENABLEDATA  *pded);

DHPDEV
DDKAPI
DrvEnablePDEV(
  IN DEVMODEW  *pdm,
  IN LPWSTR  pwszLogAddress,
  IN ULONG  cPat,
  OUT HSURF  *phsurfPatterns,
  IN ULONG  cjCaps,
  OUT ULONG  *pdevcaps,
  IN ULONG  cjDevInfo,
  OUT DEVINFO  *pdi,
  IN HDEV  hdev,
  IN LPWSTR  pwszDeviceName,
  IN HANDLE  hDriver);

HSURF
DDKAPI
DrvEnableSurface(
  IN DHPDEV  dhpdev);

/* DrvEndDoc.fl constants */
#define ED_ABORTDOC                       0x00000001

BOOL
DDKAPI
DrvEndDoc(
  IN SURFOBJ  *pso,
  IN FLONG  fl);

ULONG
DDKAPI
DrvEscape(
  IN SURFOBJ  *pso,
  IN ULONG  iEsc,
  IN ULONG  cjIn,
  IN PVOID  pvIn,
  IN ULONG  cjOut,
  OUT PVOID  pvOut);

BOOL
DDKAPI
DrvFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix,
  IN FLONG  flOptions);

ULONG
DDKAPI
DrvFontManagement(
  IN SURFOBJ  *pso,
  IN FONTOBJ  *pfo,
  IN ULONG  iMode,
  IN ULONG  cjIn,
  IN PVOID  pvIn,
  IN ULONG  cjOut,
  OUT PVOID  pvOut);

VOID
DDKAPI
DrvFree(
  IN PVOID  pv,
  IN ULONG_PTR  id);

/* DrvGetGlyphMode return values */
#define FO_HGLYPHS                        0L
#define FO_GLYPHBITS                      1L
#define FO_PATHOBJ                        2L

ULONG
DDKAPI
DrvGetGlyphMode(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo);

ULONG
DDKAPI
DrvGetModes(
  IN HANDLE  hDriver,
  IN ULONG  cjSize,
  OUT DEVMODEW  *pdm);

PVOID
DDKAPI
DrvGetTrueTypeFile(
  IN ULONG_PTR  iFile,
  IN ULONG  *pcj);

BOOL
DDKAPI
DrvGradientFill(
  IN SURFOBJ  *psoDest,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN TRIVERTEX  *pVertex,
  IN ULONG  nVertex,
  IN PVOID  pMesh,
  IN ULONG  nMesh,
  IN RECTL  *prclExtents,
  IN POINTL  *pptlDitherOrg,
  IN ULONG  ulMode);

BOOL
DDKAPI
DrvIcmCheckBitmapBits(
  IN DHPDEV  dhpdev,
  IN HANDLE  hColorTransform,
  IN SURFOBJ  *pso,
  OUT PBYTE  paResults);

HANDLE
DDKAPI
DrvIcmCreateColorTransform(
  IN DHPDEV  dhpdev,
  IN LPLOGCOLORSPACEW  pLogColorSpace,
  IN PVOID  pvSourceProfile,
  IN ULONG  cjSourceProfile,
  IN PVOID  pvDestProfile,
  IN ULONG  cjDestProfile,
  IN PVOID  pvTargetProfile,
  IN ULONG  cjTargetProfile,
  IN DWORD  dwReserved);

BOOL
DDKAPI
DrvIcmDeleteColorTransform(
  IN DHPDEV  dhpdev,
  IN HANDLE  hcmXform);

/* DrvIcmSetDeviceGammaRamp.iFormat constants */
#define IGRF_RGB_256BYTES                 0x00000000
#define IGRF_RGB_256WORDS                 0x00000001

BOOL
DDKAPI
DrvIcmSetDeviceGammaRamp(
  IN DHPDEV  dhpdev,
  IN ULONG  iFormat,
  IN LPVOID  lpRamp);

BOOL
DDKAPI
DrvLineTo(
  SURFOBJ  *pso,
  CLIPOBJ  *pco,
  BRUSHOBJ  *pbo,
  LONG  x1,
  LONG  y1,
  LONG  x2,
  LONG  y2,
  RECTL  *prclBounds,
  MIX  mix);

ULONG_PTR
DDKAPI
DrvLoadFontFile(
  ULONG  cFiles,
  ULONG_PTR  *piFile,
  PVOID  *ppvView,
  ULONG  *pcjView,
  DESIGNVECTOR  *pdv,
  ULONG  ulLangID,
  ULONG  ulFastCheckSum);

VOID
DDKAPI
DrvMovePointer(
  IN SURFOBJ  *pso,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl);

BOOL
DDKAPI
DrvNextBand(
  IN SURFOBJ  *pso,
  IN POINTL  *pptl);

VOID
DDKAPI
DrvNotify(
  IN SURFOBJ  *pso,
  IN ULONG  iType,
  IN PVOID  pvData);

BOOL
DDKAPI
DrvOffset(
  IN SURFOBJ  *pso,
  IN LONG  x,
  IN LONG  y,
  IN FLONG  flReserved);

BOOL
DDKAPI
DrvPaint(
  IN SURFOBJ  *pso,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix);

BOOL
DDKAPI
DrvPlgBlt(
  IN SURFOBJ  *psoTrg,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMsk,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlBrushOrg,
  IN POINTFIX  *pptfx,
  IN RECTL  *prcl,
  IN POINTL  *pptl,
  IN ULONG  iMode);

/* DrvQueryAdvanceWidths.iMode constants */
#define QAW_GETWIDTHS                     0
#define QAW_GETEASYWIDTHS                 1

BOOL
DDKAPI
DrvQueryAdvanceWidths(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo,
  IN ULONG  iMode,
  IN HGLYPH  *phg,
  OUT PVOID  pvWidths,
  IN ULONG  cGlyphs);

/* DrvQueryDeviceSupport.iType constants */
#define QDS_CHECKJPEGFORMAT               0x00000000
#define QDS_CHECKPNGFORMAT                0x00000001

BOOL
DDKAPI
DrvQueryDeviceSupport(
  SURFOBJ  *pso,
  XLATEOBJ  *pxlo,
  XFORMOBJ  *pxo,
  ULONG  iType,
  ULONG  cjIn,
  PVOID  pvIn,
  ULONG  cjOut,
  PVOID  pvOut);

/* DrvQueryDriverInfo.dwMode constants */
#define DRVQUERY_USERMODE                 0x00000001

BOOL
DDKAPI
DrvQueryDriverInfo(
  DWORD  dwMode,
  PVOID  pBuffer,
  DWORD  cbBuf,
  PDWORD  pcbNeeded);

PIFIMETRICS
DDKAPI
DrvQueryFont(
  IN DHPDEV  dhpdev,
  IN ULONG_PTR  iFile,
  IN ULONG  iFace,
  IN ULONG_PTR  *pid);

/* DrvQueryFontCaps.pulCaps constants */
#define QC_OUTLINES                       0x00000001
#define QC_1BIT                           0x00000002
#define QC_4BIT                           0x00000004

#define QC_FONTDRIVERCAPS (QC_OUTLINES | QC_1BIT | QC_4BIT)

LONG
DDKAPI
DrvQueryFontCaps(
  IN ULONG  culCaps,
  OUT ULONG  *pulCaps);

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

LONG
DDKAPI
DrvQueryFontData(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo,
  IN ULONG  iMode,
  IN HGLYPH  hg,
  IN OUT GLYPHDATA  *pgd,
  IN OUT PVOID  pv,
  IN ULONG  cjSize);

/* DrvQueryFontFile.ulMode constants */
#define QFF_DESCRIPTION                   0x00000001
#define QFF_NUMFACES                      0x00000002

LONG
DDKAPI
DrvQueryFontFile(
  IN ULONG_PTR  iFile,
  IN ULONG  ulMode,
  IN ULONG  cjBuf,
  IN ULONG  *pulBuf);

/* DrvQueryFontTree.iMode constants */
#define QFT_UNICODE                       0L
#define QFT_LIGATURES                     1L
#define QFT_KERNPAIRS                     2L
#define QFT_GLYPHSET                      3L

PVOID
DDKAPI
DrvQueryFontTree(
  IN DHPDEV  dhpdev,
  IN ULONG_PTR  iFile,
  IN ULONG  iFace,
  IN ULONG  iMode,
  IN ULONG_PTR  *pid);

PFD_GLYPHATTR
DDKAPI
DrvQueryGlyphAttrs(
  IN FONTOBJ  *pfo,
  IN ULONG  iMode);

ULONG
DDKAPI
DrvQueryPerBandInfo(
  IN SURFOBJ  *pso,
  IN OUT PERBANDINFO  *pbi);

/* DrvQueryTrueTypeOutline.bMetricsOnly constants */
#define TTO_METRICS_ONLY                  0x00000001
#define TTO_QUBICS                        0x00000002
#define TTO_UNHINTED                      0x00000004

LONG
DDKAPI
DrvQueryTrueTypeOutline(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo,
  IN HGLYPH  hglyph,
  IN BOOL  bMetricsOnly,
  IN GLYPHDATA  *pgldt,
  IN ULONG  cjBuf,
  OUT TTPOLYGONHEADER  *ppoly);

LONG
DDKAPI
DrvQueryTrueTypeTable(
  IN ULONG_PTR  iFile,
  IN ULONG  ulFont,
  IN ULONG  ulTag,
  IN PTRDIFF  dpStart,
  IN ULONG  cjBuf,
  OUT BYTE  *pjBuf,
  OUT PBYTE  *ppjTable,
  OUT ULONG *pcjTable);

/* DrvRealizeBrush.iHatch constants */
#define RB_DITHERCOLOR                    0x80000000L

#define HS_DDI_MAX                        6

BOOL
DDKAPI
DrvRealizeBrush(
  IN BRUSHOBJ  *pbo,
  IN SURFOBJ  *psoTarget,
  IN SURFOBJ  *psoPattern,
  IN SURFOBJ  *psoMask,
  IN XLATEOBJ  *pxlo,
  IN ULONG  iHatch);

/* DrvResetDevice return values */
#define DRD_SUCCESS                       0
#define DRD_ERROR                         1

ULONG
DDKAPI
DrvResetDevice(
  IN DHPDEV dhpdev,
  IN PVOID Reserved);

BOOL
DDKAPI
DrvResetPDEV(
  DHPDEV  dhpdevOld,
  DHPDEV  dhpdevNew);

/* DrvSaveScreenBits.iMode constants */
#define SS_SAVE                           0x00000000
#define SS_RESTORE                        0x00000001
#define SS_FREE                           0x00000002

ULONG_PTR
DDKAPI
DrvSaveScreenBits(
  IN SURFOBJ  *pso,
  IN ULONG  iMode,
  IN ULONG_PTR  ident,
  IN RECTL  *prcl);

BOOL
DDKAPI
DrvSendPage(
  IN SURFOBJ  *pso);

BOOL
DDKAPI
DrvSetPalette(
  IN DHPDEV  dhpdev,
  IN PALOBJ  *ppalo,
  IN FLONG  fl,
  IN ULONG  iStart,
  IN ULONG  cColors);

BOOL
DDKAPI
DrvSetPixelFormat(
  IN SURFOBJ  *pso,
  IN LONG  iPixelFormat,
  IN HWND  hwnd);

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
#define SPS_LENGTHMASK                    0x00000F00L
#define SPS_FREQMASK                      0x000FF000L

ULONG
DDKAPI
DrvSetPointerShape(
  IN SURFOBJ  *pso,
  IN SURFOBJ  *psoMask,
  IN SURFOBJ  *psoColor,
  IN XLATEOBJ  *pxlo,
  IN LONG  xHot,
  IN LONG  yHot,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl,
  IN FLONG  fl);

BOOL
DDKAPI
DrvStartBanding(
  IN SURFOBJ  *pso,
  IN POINTL  *pptl);

BOOL
DDKAPI
DrvStartDoc(
  IN SURFOBJ  *pso,
  IN LPWSTR  pwszDocName,
  IN DWORD  dwJobId);

BOOL
DDKAPI
DrvStartPage(
  IN SURFOBJ  *pso);

BOOL
DDKAPI
DrvStretchBlt(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlHTOrg,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN POINTL  *pptlMask,
  IN ULONG  iMode);

BOOL
DDKAPI
DrvStretchBltROP(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlHTOrg,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN POINTL  *pptlMask,
  IN ULONG  iMode,
  IN BRUSHOBJ  *pbo,
  IN DWORD  rop4);

BOOL
DDKAPI
DrvStrokeAndFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pboStroke,
  IN LINEATTRS  *plineattrs,
  IN BRUSHOBJ  *pboFill,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mixFill,
  IN FLONG  flOptions);

BOOL
DDKAPI
DrvStrokePath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN LINEATTRS  *plineattrs,
  IN MIX  mix);

BOOL
DDKAPI
DrvSwapBuffers(
  IN SURFOBJ  *pso,
  IN WNDOBJ  *pwo);

VOID
DDKAPI
DrvSynchronize(
  IN DHPDEV  dhpdev,
  IN RECTL  *prcl);

/* DrvSynchronizeSurface.fl constants */
#define DSS_TIMER_EVENT                   0x00000001
#define DSS_FLUSH_EVENT                   0x00000002

VOID
DDKAPI
DrvSynchronizeSurface(
  IN SURFOBJ  *pso,
  IN RECTL  *prcl,
  IN FLONG  fl);

BOOL
DDKAPI
DrvTextOut(
  IN SURFOBJ  *pso,
  IN STROBJ  *pstro,
  IN FONTOBJ  *pfo,
  IN CLIPOBJ  *pco,
  IN RECTL  *prclExtra,
  IN RECTL  *prclOpaque,
  IN BRUSHOBJ  *pboFore,
  IN BRUSHOBJ  *pboOpaque,
  IN POINTL  *pptlOrg,
  IN MIX  mix);

BOOL
DDKAPI
DrvTransparentBlt(
  IN SURFOBJ  *psoDst,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDst,
  IN RECTL  *prclSrc,
  IN ULONG  iTransColor,
  IN ULONG  ulReserved);

BOOL
DDKAPI
DrvUnloadFontFile(
  IN ULONG_PTR  iFile);

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

typedef VOID DDKAPI
(CALLBACK * WNDOBJCHANGEPROC)(
  WNDOBJ  *pwo,
  FLONG  fl);


typedef BOOL DDKAPI
(*PFN_DrvAlphaBlend)(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN BLENDOBJ  *pBlendObj);

typedef BOOL DDKAPI
(*PFN_DrvAssertMode)(
  IN DHPDEV  dhpdev,
  IN BOOL  bEnable);

typedef BOOL DDKAPI
(*PFN_DrvBitBlt)(
  IN SURFOBJ  *psoTrg,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclTrg,
  IN POINTL  *pptlSrc,
  IN POINTL  *pptlMask,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrush,
  IN ROP4  rop4);

typedef VOID DDKAPI
(*PFN_DrvCompletePDEV)(
  IN DHPDEV  dhpdev,
  IN HDEV  hdev);

typedef BOOL DDKAPI
(*PFN_DrvCopyBits)(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDest,
  IN POINTL  *pptlSrc);

typedef HBITMAP DDKAPI
(*PFN_DrvCreateDeviceBitmap)(
  IN DHPDEV  dhpdev,
  IN SIZEL  sizl,
  IN ULONG  iFormat);

typedef VOID DDKAPI
(*PFN_DrvDeleteDeviceBitmap)(
  IN DHSURF  dhsurf);

typedef HBITMAP DDKAPI
(*PFN_DrvDeriveSurface)(
  DD_DIRECTDRAW_GLOBAL  *pDirectDraw,
  DD_SURFACE_LOCAL  *pSurface);

typedef LONG DDKAPI
(*PFN_DrvDescribePixelFormat)(
  IN DHPDEV  dhpdev,
  IN LONG  iPixelFormat,
  IN ULONG  cjpfd,
  OUT PIXELFORMATDESCRIPTOR  *ppfd);

typedef VOID DDKAPI
(*PFN_DrvDestroyFont)(
  IN FONTOBJ  *pfo);

typedef VOID DDKAPI
(*PFN_DrvDisableDriver)(
  VOID);

typedef VOID DDKAPI
(*PFN_DrvDisablePDEV)(
  IN DHPDEV  dhpdev);

typedef VOID DDKAPI
(*PFN_DrvDisableSurface)(
  IN DHPDEV  dhpdev);

typedef ULONG DDKAPI
(*PFN_DrvDitherColor)(
  IN DHPDEV  dhpdev,
  IN ULONG  iMode,
  IN ULONG  rgb,
  OUT ULONG  *pul);

typedef ULONG DDKAPI
(*PFN_DrvDrawEscape)(
  IN SURFOBJ  *pso,
  IN ULONG  iEsc,
  IN CLIPOBJ  *pco,
  IN RECTL  *prcl,
  IN ULONG  cjIn,
  IN PVOID  pvIn);

typedef BOOL DDKAPI
(*PFN_DrvEnableDriver)(
  IN ULONG  iEngineVersion,
  IN ULONG  cj,
  OUT DRVENABLEDATA  *pded);
#if 0
typedef DHPDEV DDKAPI
(*PFN_DrvEnablePDEV)(
  IN DEVMODEW  *pdm,
  IN LPWSTR  pwszLogAddress,
  IN ULONG  cPat,
  OUT HSURF  *phsurfPatterns,
  IN ULONG  cjCaps,
  OUT ULONG  *pdevcaps,
  IN ULONG  cjDevInfo,
  OUT DEVINFO  *pdi,
  IN HDEV  hdev,
  IN LPWSTR  pwszDeviceName,
  IN HANDLE  hDriver);
#endif
typedef HSURF DDKAPI
(*PFN_DrvEnableSurface)(
  IN DHPDEV  dhpdev);

typedef BOOL DDKAPI
(*PFN_DrvEndDoc)(
  IN SURFOBJ  *pso,
  IN FLONG  fl);

typedef ULONG DDKAPI
(*PFN_DrvEscape)(
  IN SURFOBJ  *pso,
  IN ULONG  iEsc,
  IN ULONG  cjIn,
  IN PVOID  pvIn,
  IN ULONG  cjOut,
  OUT PVOID  pvOut);

typedef BOOL DDKAPI
(*PFN_DrvFillPath)(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix,
  IN FLONG  flOptions);

typedef ULONG DDKAPI
(*PFN_DrvFontManagement)(
  IN SURFOBJ  *pso,
  IN FONTOBJ  *pfo,
  IN ULONG  iMode,
  IN ULONG  cjIn,
  IN PVOID  pvIn,
  IN ULONG  cjOut,
  OUT PVOID  pvOut);

typedef VOID DDKAPI
(*PFN_DrvFree)(
  IN PVOID  pv,
  IN ULONG_PTR  id);

typedef ULONG DDKAPI
(*PFN_DrvGetGlyphMode)(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo);

typedef ULONG DDKAPI
(*PFN_DrvGetModes)(
  IN HANDLE  hDriver,
  IN ULONG  cjSize,
  OUT DEVMODEW  *pdm);

typedef PVOID DDKAPI
(*PFN_DrvGetTrueTypeFile)(
  IN ULONG_PTR  iFile,
  IN ULONG  *pcj);

typedef BOOL DDKAPI
(*PFN_DrvGradientFill)(
  IN SURFOBJ  *psoDest,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN TRIVERTEX  *pVertex,
  IN ULONG  nVertex,
  IN PVOID  pMesh,
  IN ULONG  nMesh,
  IN RECTL  *prclExtents,
  IN POINTL  *pptlDitherOrg,
  IN ULONG  ulMode);

typedef BOOL DDKAPI
(*PFN_DrvIcmCheckBitmapBits)(
  IN DHPDEV  dhpdev,
  IN HANDLE  hColorTransform,
  IN SURFOBJ  *pso,
  OUT PBYTE  paResults);

typedef HANDLE DDKAPI
(*PFN_DrvIcmCreateColorTransform)(
  IN DHPDEV  dhpdev,
  IN LPLOGCOLORSPACEW  pLogColorSpace,
  IN PVOID  pvSourceProfile,
  IN ULONG  cjSourceProfile,
  IN PVOID  pvDestProfile,
  IN ULONG  cjDestProfile,
  IN PVOID  pvTargetProfile,
  IN ULONG  cjTargetProfile,
  IN DWORD  dwReserved);

typedef BOOL DDKAPI
(*PFN_DrvIcmDeleteColorTransform)(
  IN DHPDEV  dhpdev,
  IN HANDLE  hcmXform);

typedef BOOL DDKAPI
(*PFN_DrvIcmSetDeviceGammaRamp)(
  IN DHPDEV  dhpdev,
  IN ULONG  iFormat,
  IN LPVOID  lpRamp);

typedef BOOL DDKAPI
(*PFN_DrvLineTo)(
  SURFOBJ  *pso,
  CLIPOBJ  *pco,
  BRUSHOBJ  *pbo,
  LONG  x1,
  LONG  y1,
  LONG  x2,
  LONG  y2,
  RECTL  *prclBounds,
  MIX  mix);

typedef ULONG_PTR DDKAPI
(*PFN_DrvLoadFontFile)(
  ULONG  cFiles,
  ULONG_PTR  *piFile,
  PVOID  *ppvView,
  ULONG  *pcjView,
  DESIGNVECTOR  *pdv,
  ULONG  ulLangID,
  ULONG  ulFastCheckSum);

typedef VOID DDKAPI
(*PFN_DrvMovePointer)(
  IN SURFOBJ  *pso,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl);

typedef BOOL DDKAPI
(*PFN_DrvNextBand)(
  IN SURFOBJ  *pso,
  IN POINTL  *pptl);

typedef VOID DDKAPI
(*PFN_DrvNotify)(
  IN SURFOBJ  *pso,
  IN ULONG  iType,
  IN PVOID  pvData);

typedef BOOL DDKAPI
(*PFN_DrvOffset)(
  IN SURFOBJ  *pso,
  IN LONG  x,
  IN LONG  y,
  IN FLONG  flReserved);

typedef BOOL DDKAPI
(*PFN_DrvPaint)(
  IN SURFOBJ  *pso,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix);

typedef BOOL DDKAPI
(*PFN_DrvPlgBlt)(
  IN SURFOBJ  *psoTrg,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMsk,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlBrushOrg,
  IN POINTFIX  *pptfx,
  IN RECTL  *prcl,
  IN POINTL  *pptl,
  IN ULONG  iMode);

typedef BOOL DDKAPI
(*PFN_DrvQueryAdvanceWidths)(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo,
  IN ULONG  iMode,
  IN HGLYPH  *phg,
  OUT PVOID  pvWidths,
  IN ULONG  cGlyphs);

typedef BOOL DDKAPI
(*PFN_DrvQueryDeviceSupport)(
  SURFOBJ  *pso,
  XLATEOBJ  *pxlo,
  XFORMOBJ  *pxo,
  ULONG  iType,
  ULONG  cjIn,
  PVOID  pvIn,
  ULONG  cjOut,
  PVOID  pvOut);

typedef BOOL DDKAPI
(*PFN_DrvQueryDriverInfo)(
  DWORD  dwMode,
  PVOID  pBuffer,
  DWORD  cbBuf,
  PDWORD  pcbNeeded);

typedef PIFIMETRICS DDKAPI
(*PFN_DrvQueryFont)(
  IN DHPDEV  dhpdev,
  IN ULONG_PTR  iFile,
  IN ULONG  iFace,
  IN ULONG_PTR  *pid);

typedef LONG DDKAPI
(*PFN_DrvQueryFontCaps)(
  IN ULONG  culCaps,
  OUT ULONG  *pulCaps);

typedef LONG DDKAPI
(*PFN_DrvQueryFontData)(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo,
  IN ULONG  iMode,
  IN HGLYPH  hg,
  IN OUT GLYPHDATA  *pgd,
  IN OUT PVOID  pv,
  IN ULONG  cjSize);

typedef LONG DDKAPI
(*PFN_DrvQueryFontFile)(
  IN ULONG_PTR  iFile,
  IN ULONG  ulMode,
  IN ULONG  cjBuf,
  IN ULONG  *pulBuf);

typedef PVOID DDKAPI
(*PFN_DrvQueryFontTree)(
  IN DHPDEV  dhpdev,
  IN ULONG_PTR  iFile,
  IN ULONG  iFace,
  IN ULONG  iMode,
  IN ULONG_PTR  *pid);

typedef PFD_GLYPHATTR DDKAPI
(*PFN_DrvQueryGlyphAttrs)(
  IN FONTOBJ  *pfo,
  IN ULONG  iMode);

typedef ULONG DDKAPI
(*PFN_DrvQueryPerBandInfo)(
  IN SURFOBJ  *pso,
  IN OUT PERBANDINFO  *pbi);

typedef LONG DDKAPI
(*PFN_DrvQueryTrueTypeOutline)(
  IN DHPDEV  dhpdev,
  IN FONTOBJ  *pfo,
  IN HGLYPH  hglyph,
  IN BOOL  bMetricsOnly,
  IN GLYPHDATA  *pgldt,
  IN ULONG  cjBuf,
  OUT TTPOLYGONHEADER  *ppoly);

typedef LONG DDKAPI
(*PFN_DrvQueryTrueTypeTable)(
  IN ULONG_PTR  iFile,
  IN ULONG  ulFont,
  IN ULONG  ulTag,
  IN PTRDIFF  dpStart,
  IN ULONG  cjBuf,
  OUT BYTE  *pjBuf,
  OUT PBYTE  *ppjTable,
  OUT ULONG *pcjTable);

typedef BOOL DDKAPI
(*PFN_DrvRealizeBrush)(
  IN BRUSHOBJ  *pbo,
  IN SURFOBJ  *psoTarget,
  IN SURFOBJ  *psoPattern,
  IN SURFOBJ  *psoMask,
  IN XLATEOBJ  *pxlo,
  IN ULONG  iHatch);

typedef ULONG DDKAPI
(*PFN_DrvResetDevice)(
  IN DHPDEV dhpdev,
  IN PVOID Reserved);

typedef BOOL DDKAPI
(*PFN_DrvResetPDEV)(
  DHPDEV  dhpdevOld,
  DHPDEV  dhpdevNew);

typedef ULONG_PTR DDKAPI
(*PFN_DrvSaveScreenBits)(
  IN SURFOBJ  *pso,
  IN ULONG  iMode,
  IN ULONG_PTR  ident,
  IN RECTL  *prcl);

typedef BOOL DDKAPI
(*PFN_DrvSendPage)(
  IN SURFOBJ  *pso);

typedef BOOL DDKAPI
(*PFN_DrvSetPalette)(
  IN DHPDEV  dhpdev,
  IN PALOBJ  *ppalo,
  IN FLONG  fl,
  IN ULONG  iStart,
  IN ULONG  cColors);

typedef BOOL DDKAPI
(*PFN_DrvSetPixelFormat)(
  IN SURFOBJ  *pso,
  IN LONG  iPixelFormat,
  IN HWND  hwnd);

typedef ULONG DDKAPI
(*PFN_DrvSetPointerShape)(
  IN SURFOBJ  *pso,
  IN SURFOBJ  *psoMask,
  IN SURFOBJ  *psoColor,
  IN XLATEOBJ  *pxlo,
  IN LONG  xHot,
  IN LONG  yHot,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl,
  IN FLONG  fl);

typedef BOOL DDKAPI
(*PFN_DrvStartBanding)(
  IN SURFOBJ  *pso,
  IN POINTL  *pptl);

typedef BOOL DDKAPI
(*PFN_DrvStartDoc)(
  IN SURFOBJ  *pso,
  IN LPWSTR  pwszDocName,
  IN DWORD  dwJobId);

typedef BOOL DDKAPI
(*PFN_DrvStartPage)(
  IN SURFOBJ  *pso);

typedef BOOL DDKAPI
(*PFN_DrvStretchBlt)(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlHTOrg,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN POINTL  *pptlMask,
  IN ULONG  iMode);

typedef BOOL DDKAPI
(*PFN_DrvStretchBltROP)(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN SURFOBJ  *psoMask,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN COLORADJUSTMENT  *pca,
  IN POINTL  *pptlHTOrg,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN POINTL  *pptlMask,
  IN ULONG  iMode,
  IN BRUSHOBJ  *pbo,
  IN DWORD  rop4);

typedef BOOL DDKAPI
(*PFN_DrvStrokeAndFillPath)(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pboStroke,
  IN LINEATTRS  *plineattrs,
  IN BRUSHOBJ  *pboFill,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mixFill,
  IN FLONG  flOptions);

typedef BOOL DDKAPI
(*PFN_DrvStrokePath)(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN LINEATTRS  *plineattrs,
  IN MIX  mix);

typedef BOOL DDKAPI
(*PFN_DrvSwapBuffers)(
  IN SURFOBJ  *pso,
  IN WNDOBJ  *pwo);

typedef VOID DDKAPI
(*PFN_DrvSynchronize)(
  IN DHPDEV  dhpdev,
  IN RECTL  *prcl);

typedef VOID DDKAPI
(*PFN_DrvSynchronizeSurface)(
  IN SURFOBJ  *pso,
  IN RECTL  *prcl,
  IN FLONG  fl);

typedef BOOL DDKAPI
(*PFN_DrvTextOut)(
  IN SURFOBJ  *pso,
  IN STROBJ  *pstro,
  IN FONTOBJ  *pfo,
  IN CLIPOBJ  *pco,
  IN RECTL  *prclExtra,
  IN RECTL  *prclOpaque,
  IN BRUSHOBJ  *pboFore,
  IN BRUSHOBJ  *pboOpaque,
  IN POINTL  *pptlOrg,
  IN MIX  mix);

typedef BOOL DDKAPI
(*PFN_DrvTransparentBlt)(
  IN SURFOBJ  *psoDst,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDst,
  IN RECTL  *prclSrc,
  IN ULONG  iTransColor,
  IN ULONG  ulReserved);

typedef BOOL DDKAPI
(*PFN_DrvUnloadFontFile)(
  IN ULONG_PTR  iFile);


WIN32KAPI
VOID
DDKAPI
DrvDisableDirectDraw(
  IN DHPDEV  dhpdev);

WIN32KAPI
BOOL
DDKAPI
DrvEnableDirectDraw(
  IN DHPDEV  dhpdev,
  OUT DD_CALLBACKS  *pCallBacks,
  OUT DD_SURFACECALLBACKS  *pSurfaceCallBacks,
  OUT DD_PALETTECALLBACKS  *pPaletteCallBacks);

WIN32KAPI
BOOL
DDKAPI
DrvGetDirectDrawInfo(
  IN DHPDEV  dhpdev,
  OUT DD_HALINFO  *pHalInfo,
  OUT DWORD  *pdwNumHeaps,
  OUT VIDEOMEMORY  *pvmList,
  OUT DWORD  *pdwNumFourCCCodes,
  OUT DWORD  *pdwFourCC);

#ifdef __cplusplus
}
#endif

#endif /* defined __VIDEO_H */

#endif /* __WINDDI_H */
