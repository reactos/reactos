/*
 * WinDDI.h - definition of the GDI - DDI interface
 */

#ifndef __DDK_WINDDI_H
#define __DDK_WINDDI_H

#ifdef __USE_W32API

#include_next <ddk/winddi.h>

#else /* __USE_W32API */

#if defined(WIN32_LEAN_AND_MEAN) && defined(_GNU_H_WINDOWS32_STRUCTURES)
#error "windows.h cannot be included before winddi.h if WIN32_LEAN_AND_MEAN is defined"
#endif

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#else
#include <windows.h>
#endif
#include <ddk/ddrawint.h>

#ifndef IN
#define IN
#define OUT
#define OPTIONAL
#endif

#ifndef PTRDIFF
typedef DWORD PTRDIFF;
#endif

#define DM_SPECVERSION 0x0320

#define DDI_DRIVER_VERSION_NT4 0x20000
#define DDI_DRIVER_VERSION_SP3 0x20003
#define DDI_DRIVER_VERSION_NT5 0x30000
#define DDI_DRIVER_VERSION_NT5_01 0x30100

#define GDI_DRIVER_VERSION 0x4000   /* NT 4 compatibility */

typedef struct _FONTINFO
{
    ULONG   cjThis;
    FLONG   flCaps;
    ULONG   cGlyphsSupported;
    ULONG   cjMaxGlyph1;
    ULONG   cjMaxGlyph4;
    ULONG   cjMaxGlyph8;
    ULONG   cjMaxGlyph32;
} FONTINFO, *PFONTINFO;

typedef BYTE GAMMA_TABLES[2][256];
typedef GAMMA_TABLES *PGAMMA_TABLES;
typedef COLORADJUSTMENT *PCOLORADJUSTMENT;

typedef ULONG  MIX;
typedef ULONG  ROP4;
#define  DDI_DRIVER_VERSION  0x00010000

#define  HS_DDI_MAX  6

/* XLate types */
#define XO_TRIVIAL      0x00000001
#define XO_TABLE        0x00000002
#define XO_TO_MONO      0x00000004

#define XO_SRCPALETTE    1
#define XO_DESTPALETTE   2
#define XO_DESTDCPALETTE 3

/*  EngCreateBitmap format types  */
enum _BMF_TYPES
{
  BMF_1BPP = 1,
  BMF_4BPP,
  BMF_8BPP,
  BMF_16BPP,
  BMF_24BPP,
  BMF_32BPP,
  BMF_4RLE,
  BMF_8RLE,
  BMF_JPEG,
  BMF_PNG
};

#define  BMF_TOPDOWN     0x00000001
#define  BMF_NOZEROINIT  0x00000002
#define  BMF_DONTCACHE   0x00000004
#define  BMF_USERMEM     0x00000008
#define  BMF_KMSECTION   0x00000010
#define  BMF_NOTSYSMEM  0x0020
#define  BMF_WINDOW_BLT 0x0040
#define  BMF_UMPDMEM    0x0080
#define  BMF_RESERVED   0xFF00

#define DC_TRIVIAL      0
#define DC_RECT         1
#define DC_COMPLEX      3

#define FC_RECT         1
#define FC_RECT4        2
#define FC_COMPLEX      3

#define TC_RECTANGLES   0
#define TC_PATHOBJ      2

#define OC_BANK_CLIP    1

#define CT_RECTANGLES   0L

#define CD_LEFTWARDS    1L
#define CD_UPWARDS      2L

/*  Options for CLIPOBJ_cEnumStart BuildOrder field  */
enum _CD_ORDERS
{
  CD_RIGHTDOWN,
  CD_LEFTDOWN,
  CD_RIGHTUP,
  CD_LEFTUP,
  CD_ANY
};

/*  Options for CLIPOBJ_cEnumStart Type field  */
#define  CT_RECTANGLE  1

#define  DCR_SOLID     0
#define  DCR_DRIVER    1
#define  DCR_HALFTONE  2

#define  DMMAXDEVICENAME  32
#define  DMMAXFORMNAME  32

#define  DM_DEFAULT     0x00000001
#define  DM_MONOCHROME  0x00000002

#define  ED_ABORTDOC  0x00000001

enum _ESCAPE_CODES
{
  ESC_PASSTHROUGH,
  ESC_QUERYESCSUPPORT
};

#define  FM_INFO_TECH_TRUETYPE              0x00000001
#define  FM_INFO_TECH_BITMAP                0x00000002
#define  FM_INFO_TECH_STROKE                0x00000004
#define  FM_INFO_TECH_OUTLINE_NOT_TRUETYPE  0x00000008
#define  FM_INFO_ARB_XFORMS                 0x00000010
#define  FM_INFO_1BPP                       0x00000020
#define  FM_INFO_4BPP                       0x00000040
#define  FM_INFO_8BPP                       0x00000080
#define  FM_INFO_16BPP                      0x00000100
#define  FM_INFO_24BPP                      0x00000200
#define  FM_INFO_32BPP                      0x00000400
#define  FM_INFO_INTEGER_WIDTH              0x00000800
#define  FM_INFO_CONSTANT_WIDTH             0x00001000
#define  FM_INFO_NOT_CONTIGUOUS             0x00002000
#define  FM_INFO_PID_EMBEDDED               0x00004000
#define  FM_INFO_RETURNS_OUTLINES           0x00008000
#define  FM_INFO_RETURNS_STROKES            0x00010000
#define  FM_INFO_RETURNS_BITMAPS            0x00020000
#define  FM_INFO_UNICODE_COMPLIANT          0x00040000
#define  FM_INFO_RIGHT_HANDED               0x00080000
#define  FM_INFO_INTEGRAL_SCALING           0x00100000
#define  FM_INFO_90DEGREE_ROTATIONS         0x00200000
#define  FM_INFO_OPTICALLY_FIXED_PITCH      0x00400000
#define  FM_INFO_DO_NOT_ENUMERATE           0x00800000
#define  FM_INFO_ISOTROPIC_SCALING_ONLY     0x01000000
#define  FM_INFO_ANISOTROPIC_SCALING_ONLY   0x02000000
#define  FM_INFO_TID_EMBEDDED               0x04000000
#define  FM_INFO_FAMILY_EQUIV               0x08000000
#define  FM_INFO_DBCS_FIXED_PITCH           0x10000000
#define  FM_INFO_NONNEGATIVE_AC             0x20000000
#define  FM_INFO_IGNORE_TC_RA_ABLE          0x40000000
#define  FM_INFO_TECH_TYPE1                 0x80000000

#define  FM_SEL_ITALIC      0x00000001
#define  FM_SEL_UNDERSCORE  0x00000002
#define  FM_SEL_NEGATIVE    0x00000004
#define  FM_SEL_OUTLINED    0x00000008
#define  FM_SEL_STRIKEOUT   0x00000010
#define  FM_SEL_BOLD        0x00000020
#define  FM_SEL_REGULAR     0x00000040

#define  FM_TYPE_LICENSED   0x00000002
#define  FM_READONLY_EMBED  0x00000004
#define  FM_EDITABLE_EMBED  0x00000008
#define  FM_NO_EMBEDDING    0x00000002

#define  FO_TYPE_RASTER    RASTER_FONTTYPE
#define  FO_TYPE_DEVICE    DEVICE_FONTTYPE
#define  FO_TYPE_TRUETYPE  TRUETYPE_FONTTYPE
#define  FO_SIM_BOLD       0x00002000
#define  FO_SIM_ITALIC     0x00004000
#define  FO_EM_HEIGHT      0x00008000
#define  FO_GRAY16         0x00010000
#define  FO_NOGRAY16       0x00020000
#define  FO_NOHINTS        0x00040000
#define  FO_NO_CHOICE      0x00080000

enum _FP_MODES
{
  FP_ALTERNATEMODE = 1,
  FP_WINDINGMODE
};

typedef struct _FD_GLYPHATTR {
    ULONG    cjThis;
    ULONG    cGlyphs;
    ULONG    iMode;
    BYTE     aGlyphAttr[1];
} FD_GLYPHATTR, *PFD_GLYPHATTR;

enum _GLYPH_MODE
{
  FO_HGLYPHS,
  FO_GLYPHBITS,
  FO_PATHOBJ
};

/* Allowed values for GDIINFO.ulPrimaryOrder. */

#define PRIMARY_ORDER_ABC       0
#define PRIMARY_ORDER_ACB       1
#define PRIMARY_ORDER_BAC       2
#define PRIMARY_ORDER_BCA       3
#define PRIMARY_ORDER_CBA       4
#define PRIMARY_ORDER_CAB       5

/* Allowed values for GDIINFO.ulHTPatternSize. */

#define HT_PATSIZE_2x2          0
#define HT_PATSIZE_2x2_M        1
#define HT_PATSIZE_4x4          2
#define HT_PATSIZE_4x4_M        3
#define HT_PATSIZE_6x6          4
#define HT_PATSIZE_6x6_M        5
#define HT_PATSIZE_8x8          6
#define HT_PATSIZE_8x8_M        7
#define HT_PATSIZE_10x10        8
#define HT_PATSIZE_10x10_M      9
#define HT_PATSIZE_12x12        10
#define HT_PATSIZE_12x12_M      11
#define HT_PATSIZE_14x14        12
#define HT_PATSIZE_14x14_M      13
#define HT_PATSIZE_16x16        14
#define HT_PATSIZE_16x16_M      15
#define HT_PATSIZE_MAX_INDEX    HT_PATSIZE_16x16_M
#define HT_PATSIZE_DEFAULT      HT_PATSIZE_4x4_M

/* Allowed values for GDIINFO.ulHTOutputFormat. */

#define HT_FORMAT_1BPP          0
#define HT_FORMAT_4BPP          2
#define HT_FORMAT_4BPP_IRGB     3
#define HT_FORMAT_8BPP          4
#define HT_FORMAT_16BPP         5
#define HT_FORMAT_24BPP         6
#define HT_FORMAT_32BPP         7

/* Allowed values for GDIINFO.flHTFlags. */

#define HT_FLAG_SQUARE_DEVICE_PEL    0x00000001
#define HT_FLAG_HAS_BLACK_DYE        0x00000002
#define HT_FLAG_ADDITIVE_PRIMS       0x00000004
#define HT_FLAG_OUTPUT_CMY           0x00000100

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

/*  EngAssocateSurface hook flags  */
#define  HOOK_BITBLT             0x00000001
#define  HOOK_STRETCHBLT         0x00000002
#define  HOOK_PLGBLT             0x00000004
#define  HOOK_TEXTOUT            0x00000008
#define  HOOK_PAINT              0x00000010
#define  HOOK_STROKEPATH         0x00000020
#define  HOOK_FILLPATH           0x00000040
#define  HOOK_STROKEANDFILLPATH  0x00000080
#define  HOOK_LINETO             0x00000100
#define  HOOK_COPYBITS           0x00000400
#define  HOOK_SYNCHRONIZE        0x00001000
#define  HOOK_SYNCHRONIZEACCESS  0x00004000
#define  HOOK_TRANSPARENTBLT     0x00008000
#define HOOK_ALPHABLEND          0x00010000
#define HOOK_GRADIENTFILL        0x00020000
#define HOOK_FLAGS               0x0003b5ff

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

/*  EngCreatePalette mode types  */
#define  PAL_INDEXED    0x00000001
#define  PAL_BITFIELDS  0x00000002
#define  PAL_RGB        0x00000004
#define  PAL_BGR        0x00000008

enum _QUERY_ADVANCE_WIDTH_TYPES
{
  QAW_GETWIDTHS = 1,
  QAW_GETEASYWIDTHS
};

#define  QC_OUTLINES  0x00000001
#define  QC_1BIT      0x00000002
#define  QC_4BIT      0x00000004

enum _QFF_MODES
{
  QFF_DESCRIPTION = 1,
  QFF_NUMFACES
};

#define  RB_DITHERCOLOR  0x80000000

enum _SPS_RC
{
  SPS_ERROR,
  SPS_DECLINE,
  SPS_ACCEPT_NOEXCLUDE,
  SPS_ACCEPT_EXCLUDE
};

#define SPS_CHANGE        0x00000001L
#define SPS_ASYNCCHANGE   0x00000002L
#define SPS_ANIMATESTART  0x00000004L
#define SPS_ANIMATEUPDATE 0x00000008L

#define  SS_SAVE     0
#define  SS_RESTORE  1
#define  SS_FREE     2

enum _SURF_TYPES
{
  STYPE_BITMAP = 0,
  STYPE_DEVICE = 1,
  STYPE_DEVBITMAP = 3
};

#define  WO_RGN_CLIENT_DELTA   0x00000001
#define  WO_RGN_CLIENT         0x00000002
#define  WO_RGN_SURFACE_DELTA  0x00000004
#define  WO_RGN_SURFACE        0x00000008
#define  WO_RGN_UPDATE_ALL     0x00000010

#define  WOC_RGN_CLIENT_DELTA   0x00000001
#define  WOC_RGN_CLIENT         0x00000002
#define  WOC_RGN_SURFACE_DELTA  0x00000004
#define  WOC_RGN_SURFACE        0x00000008
#define  WOC_CHANGED            0x00000010
#define  WOC_DELETE             0x00000020

typedef HANDLE  HDEV;
typedef HANDLE  HGLYPH;
typedef HANDLE  HSURF;
typedef HANDLE  DHPDEV;
typedef HANDLE  DHSURF;
typedef ULONG  (*PFN)(VOID);
typedef ULONG IDENT;

typedef struct _DRVFN
{
  ULONG  iFunc;
  PFN  pfn;
} DRVFN, *PDRVFN;

/*
 * DRVENABLEDATA - this structure is passed to the DDI from the GDI
 *   in the function DrvEnableDriver to determine driver parameters.
 */

typedef struct _DRVENABLEDATA
{
  ULONG  iDriverVersion;
  ULONG  c;
  DRVFN  *pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;

typedef LONG  LDECI4;

typedef struct _CIECHROMA
{
  LDECI4  x;
  LDECI4  y;
  LDECI4  Y;
} CIECHROMA, *PCIECHROMA;

typedef struct _COLORINFO
{
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

typedef struct _DEVINFO
{
  ULONG  flGraphicsCaps;
  LOGFONTW  lfDefaultFont;
  LOGFONTW  lfAnsiVarFont;
  LOGFONTW  lfAnsiFixFont;
  ULONG  cFonts;
  ULONG  iDitherFormat;
  USHORT  cxDither;
  USHORT  cyDither;
  HPALETTE  hpalDefault;
  ULONG  flGraphicsCaps2;
} DEVINFO, *PDEVINFO;

typedef struct _GDIINFO
{
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

typedef struct _BRUSHOBJ
{
  ULONG  iSolidColor;
  PVOID  pvRbrush;
  FLONG  flColorType;
  /*  remainder of fields are for GDI internal use  */
  LOGBRUSH  logbrush;
} BRUSHOBJ, *PBRUSHOBJ;

typedef struct _CLIPOBJ
{
  ULONG  iUniq;
  RECTL  rclBounds;
  BYTE  iDComplexity;
  BYTE  iFComplexity;
  BYTE  iMode;
  BYTE  fjOptions;
} CLIPOBJ, *PCLIPOBJ;

typedef struct _ENUMRECTS
{
  ULONG  c;
  RECTL  arcl[1];
} ENUMRECTS, *PENUMRECTS;

typedef struct _BLENDOBJ
{
    BLENDFUNCTION BlendFunction;
}BLENDOBJ,*PBLENDOBJ;

typedef struct
{
   DWORD nSize;
   HDC   hdc;
   PBYTE pvEMF;
   PBYTE pvCurrentRecord;
} EMFINFO, *PEMFINFO;

typedef struct _FONTOBJ
{
  ULONG  iUniq;
  ULONG  iFace;
  ULONG  cxMax;
  ULONG  flFontType;
  ULONG  iTTUniq;
  ULONG  iFile;
  SIZE  sizLogResPpi;
  ULONG  ulStyleSize;
  PVOID  pvConsumer;
  PVOID  pvProducer;
} FONTOBJ, *PFONTOBJ;

typedef struct _IFIMETRICS
{
  ULONG cjThis;
  ULONG ulVersion;
  PTRDIFF dpwszFamilyName;
  PTRDIFF dpwszStyleName;
  PTRDIFF dpwszFaceName;
  PTRDIFF dpwszUniqueName;
  PTRDIFF dpFontSim;
  LONG lEmbedId;
  LONG lItalicAngle;
  LONG lCharBias;
  PTRDIFF dpCharSets;
  BYTE jWinCharSet;
  BYTE jWinPitchAndFamily;
  USHORT usWinWeight;
  ULONG flInfo;
  USHORT fsSelection;
  USHORT fsType;
  WORD fwdUnitsPerEm;
  WORD fwdLowestPPEm;
  WORD fwdWinAscender;
  WORD fwdWinDescender;
  WORD fwdMacAscender;
  WORD fwdMacDescender;
  WORD fwdMacLineGap;
  WORD fwdTypoAscender;
  WORD fwdTypoDescender;
  WORD fwdTypoLineGap;
  WORD fwdAveCharWidth;
  WORD fwdMaxCharInc;
  WORD fwdCapHeight;
  WORD fwdXHeight;
  WORD fwdSubScriptXSize;
  WORD fwdSubScriptYSize;
  WORD fwdSubScriptXOffset;
  WORD fwdSubScriptYOffset;
  WORD fwdSuperScriptXSize;
  WORD fwdSuperScriptYSize;
  WORD fwdSuperScriptXOffset;
  WORD fwdSuperScriptYOffset;
  WORD fwdUnderscoreSize;
  WORD fwdUnderscorePosition;
  WORD fwdStrikeoutSize;
  WORD fwdStrikeoutPosition;
  BYTE chFirstChar;
  BYTE chLastChar;
  BYTE chDefaultChar;
  BYTE chBreakChar;
  WCHAR wcFirstChar;
  WCHAR wcLastChar;
  WCHAR wcDefaultChar;
  WCHAR wcBreakChar;
  POINTL ptlBaseline;
  POINTL ptlAspect;
  POINTL ptlCaret;
  RECTL rclFontBox;
  BYTE achVendId[4];
  ULONG cKerningPairs;
  ULONG ulPanoseCulture;
  PANOSE panose;
} IFIMETRICS, *PIFIMETRICS;

#define NB_RESERVED_COLORS              20 /* number of fixed colors in system palette */

typedef struct _XLATEOBJ
{
  ULONG  iUniq;
  ULONG  flXlate;
  USHORT  iSrcType;
  USHORT  iDstType;
  ULONG  cEntries;
  ULONG  *pulXlate;
} XLATEOBJ, *PXLATEOBJ;

typedef struct _PALOBJ
{
  ULONG   ulReserved;
  PXLATEOBJ logicalToSystem;
  int *mapping;
} PALOBJ, *PPALOBJ;

typedef struct _PATHOBJ
{
  ULONG  fl;
  ULONG  cCurves;
} PATHOBJ, *PPATHOBJ;

typedef struct _SURFOBJ
{
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
} SURFOBJ, *PSURFOBJ;

typedef struct _WNDOBJ
{
  CLIPOBJ  coClient;
  PVOID  pvConsumer;
  RECTL  rclClient;
} WNDOBJ, *PWNDOBJ;

typedef VOID (CALLBACK * WNDOBJCHANGEPROC)(PWNDOBJ WndObj, ULONG Flags);

typedef struct _XFORMOBJ
{
    ULONG ulReserved;
} XFORMOBJ, *PXFORMOBJ;

typedef struct _GLYPHBITS
{
  POINTL ptlOrigin;
  SIZEL  sizlBitmap;
  BYTE   aj[1];
} GLYPHBITS;

typedef union _GLYPHDEF
{
  GLYPHBITS  *pgb;
  PATHOBJ    *ppo;
} GLYPHDEF;

typedef struct _POINTQF
{
    LARGE_INTEGER x;
    LARGE_INTEGER y;
} POINTQF, *PPOINTQF;

typedef struct _GLYPHDATA {
        GLYPHDEF gdf;
        HGLYPH   hg;
        FIX      fxD;
        FIX      fxA;
        FIX      fxAB;
        FIX      fxInkTop;
        FIX      fxInkBottom;
        RECTL    rclInk;
        POINTQF  ptqD;
} GLYPHDATA, *PGLYPHDATA;

typedef struct _GLYPHPOS
{
  HGLYPH     hg;
  GLYPHDEF  *pgdf;
  POINTL    *ptl;
} GLYPHPOS, *PGLYPHPOS;

typedef struct _STROBJ
{
  ULONG      cGlyphs;
  FLONG      flAccel;
  ULONG      ulCharInc;
  RECTL      rclBkGround;
  GLYPHPOS  *pgp;
  LPWSTR     pwszOrg;
} STROBJ, *PSTROBJ;

typedef struct _WCRUN
{
  WCHAR   wcLow;
  USHORT  cGlyphs;
  HGLYPH *phg;
} WCRUN, *PWCRUN;

typedef struct _FD_GLYPHSET
{
  ULONG  cjThis;
  FLONG  flAccel;
  ULONG  cGlyphsSupported;
  ULONG  cRuns;
  WCRUN  awcrun[1];
} FD_GLYPHSET, *PFD_GLYPHSET;

struct _DRIVEROBJ;

typedef BOOL (CALLBACK * FREEOBJPROC) (struct _DRIVEROBJ* pDriverObj);

typedef struct _DRIVEROBJ
{
  PVOID  pvObj;
  FREEOBJPROC  pFreeProc;
  HDEV  hdev;
  DHPDEV  dhpdev;
} DRIVEROBJ;

typedef struct _TYPE1_FONT
{
  HANDLE  hPFM;
  HANDLE  hPFB;
  ULONG  ulIdentifier;
} TYPE1_FONT;

typedef struct _ENG_TIME_FIELDS
{
  USHORT  usYear;
  USHORT  usMonth;
  USHORT  usDay;
  USHORT  usHour;
  USHORT  usMinute;
  USHORT  usSecond;
  USHORT  usMilliseconds;
  USHORT  usWeekday;
} ENG_TIME_FIELDS, *PENG_TIME_FIELDS;

typedef enum _ENG_SYSTEM_ATTRIBUTE {
  EngProcessorFeature = 1,
  EngNumberOfProcessors,
  EngOptimumAvailableUserMemory,
  EngOptimumAvailableSystemMemory,
} ENG_SYSTEM_ATTRIBUTE;

typedef struct _LINEATTRS
{
  FLONG  fl;
  ULONG  iJoin;
  ULONG  iEndCap;
  FLOAT_LONG  elWidth;
  FLOATL  eMiterLimit;
  ULONG  cstyle;
  PFLOAT_LONG  pstyle;
  FLOAT_LONG  elStyleState;
} LINEATTRS, *PLINEATTRS;

typedef struct _FLOATOBJ
{
  ULONG ul1,
	ul2;
} FLOATOBJ, *PFLOATOBJ;

typedef struct _POINTFIX
{
  FIX x,
      y;
} POINTFIX;

typedef struct _PATHDATA
{
  FLONG      flags;
  ULONG      count;
  POINTFIX  *pptfx;
} PATHDATA, *PPATHDATA;

typedef struct _RUN
{
  LONG  iStart;
  LONG  iStop;
} RUN, *PRUN;

typedef struct _CLIPLINE
{
  POINTFIX  ptfxA;
  POINTFIX  ptfxB;
  LONG      lStyleState;
  ULONG     c;
  RUN       arun[1];
} CLIPLINE, *PCLIPLINE;

typedef struct _RECTFX
{
  FIX xLeft,
      yTop,
      xRight,
      yBottom;
} RECTFX, *PRECTFX;

typedef struct
{
  FLOATOBJ  eM11,
            eM12,
            eM21,
            eM22,
            eDx,
            eDy;
} FLOATOBJ_XFORM, *PFLOATOBJ_XFORM, FAR *LPFLOATOBJ_XFORM;

typedef struct _XFORML
{
  FLOATL  eM11,
          eM12,
          eM21,
          eM22,
          eDx,
          eDy;
} XFORML, *PXFORML;

/*
 * Functions Prefixed with Drv are calls made from GDI to DDI, and
 * everything else are calls made from DDI to GDI.  DDI is
 * not allowed to make calls to any other kernel or user modules.
 */

/*  GDI --> DDI calls  */
BOOL STDCALL
DrvAssertMode(IN DHPDEV PDev,
	      IN BOOL ShouldEnable);

BOOL STDCALL
DrvBitBlt(IN PSURFOBJ DestSurface,
	  IN PSURFOBJ SrcSurface,
	  IN PSURFOBJ MaskSurface,
	  IN PCLIPOBJ ClipObj,
	  IN PXLATEOBJ XLateObj,
	  IN PRECTL DestRectL,
	  IN PPOINTL SrcPointL,
	  IN PPOINTL MaskPointL,
	  IN PBRUSHOBJ BrushObj,
	  IN PPOINTL BrushPointL,
	  IN ROP4 RasterOp);
VOID STDCALL
DrvCompletePDEV(IN DHPDEV PDev,
		IN HDEV Dev);
BOOL STDCALL
DrvCopyBits(OUT PSURFOBJ DestSurface,
	    IN PSURFOBJ SrcSurface,
	    IN PCLIPOBJ ClipObj,
	    IN PXLATEOBJ XLateObj,
	    IN PRECTL DestRectL,
	    IN PPOINTL SrcPointL);
HBITMAP STDCALL
DrvCreateDeviceBitmap(IN DHPDEV DPev,
		      IN SIZEL SizeL,
		      IN ULONG Format);
VOID STDCALL
DrvDeleteDeviceBitmap(IN DHSURF Surface);
LONG STDCALL
DrvDescribePixelFormat(IN DHPDEV DPev,
		       IN LONG PixelFormat,
		       IN ULONG DescriptorSize,
		       OUT PPIXELFORMATDESCRIPTOR PFD);
VOID STDCALL
DrvDestroyFont(IN PFONTOBJ FontObj);
VOID STDCALL
DrvDisableDirectDraw(IN DHPDEV PDev);
VOID STDCALL
DrvDisableDriver(VOID);
VOID STDCALL
DrvDisablePDEV(IN DHPDEV PDev);
VOID STDCALL
DrvDisableSurface(IN DHPDEV PDev);
ULONG STDCALL
DrvDitherColor(IN DHPDEV DPev,
	       IN ULONG Mode,
	       IN ULONG RGB,
	       OUT PULONG DitherBits);
ULONG STDCALL
DrvDrawEscape(IN PSURFOBJ SurfObj,
	      IN ULONG EscCode,
	      IN PCLIPOBJ ClipObj,
	      IN PRECTL RectL,
	      IN ULONG InputSize,
	      IN PVOID *InputData);
BOOL STDCALL
DrvEnableDirectDraw(IN DHPDEV PDev,
		    IN PDD_CALLBACKS Callbacks,
		    IN PDD_SURFACECALLBACKS SurfaceCallbacks,
		    IN PDD_PALETTECALLBACKS PaletteCallbacks);
BOOL STDCALL
DrvEnableDriver(IN ULONG Version,
		IN ULONG DEDSize,
		OUT PDRVENABLEDATA DED);
DHPDEV STDCALL
DrvEnablePDEV(IN DEVMODEW *DM,
	      IN LPWSTR LogAddress,
	      IN ULONG PatternCount,
	      OUT HSURF *SurfPatterns,
	      IN ULONG GDIInfoSize,
	      OUT ULONG *GDIInfo,
	      IN ULONG DevInfoSize,
	      OUT DEVINFO *DevInfo,
	      IN HDEV Dev,
	      IN LPWSTR DeviceName,
	      IN HANDLE Driver);
HSURF STDCALL
DrvEnableSurface(IN DHPDEV PDev);
BOOL STDCALL
DrvEndDoc(IN PSURFOBJ SurfObj,
	  IN ULONG Flags);
ULONG STDCALL
DrvEscape(IN PSURFOBJ SurfObj,
	  IN ULONG EscCode,
	  IN ULONG InputSize,
	  IN PVOID *InputData,
	  IN ULONG OutputSize,
	  OUT PVOID *OutputData);
BOOL STDCALL
DrvFillPath(IN PSURFOBJ SurfObj,
	    IN PPATHOBJ PathObj,
	    IN PCLIPOBJ ClipObj,
	    IN PBRUSHOBJ BrushObj,
	    IN PPOINTL BrushOrg,
	    IN MIX Mix,
	    IN ULONG Options);
ULONG STDCALL
DrvFontManagement(IN PSURFOBJ SurfObj,
		  IN PFONTOBJ FontObj,
		  IN ULONG Mode,
		  IN ULONG InputSize,
		  IN PVOID InputData,
		  IN ULONG OutputSize,
		  OUT PVOID OutputData);
VOID STDCALL
DrvFree(IN PVOID Obj,
	IN ULONG ID);
BOOL STDCALL
DrvGetDirectDrawInfo(IN DHPDEV PDev,
		     IN PDD_HALINFO HalInfo,
		     IN PDWORD NumHeaps,
		     IN PVIDEOMEMORY List,
		     IN PDWORD NumFourCCCodes,
		     IN PDWORD FourCC);
ULONG STDCALL
DrvGetGlyphMode(IN DHPDEV DPev,
		IN PFONTOBJ FontObj);
ULONG STDCALL
DrvGetModes(IN HANDLE Driver,
	    IN ULONG DataSize,
	    OUT PDEVMODEW DM);
PVOID STDCALL
DrvGetTrueTypeFile(IN ULONG FileNumber,
		   IN PULONG Size);
BOOL STDCALL
DrvGradientFill(IN SURFOBJ  *psoDest,
                IN CLIPOBJ  *pco,
                IN XLATEOBJ  *pxlo,
                IN TRIVERTEX  *pVertex,
                IN ULONG  nVertex,
                IN PVOID  pMesh,
                IN ULONG  nMesh,
                IN RECTL  *prclExtents,
                IN POINTL  *pptlDitherOrg,
                IN ULONG  ulMode);
BOOL STDCALL
DrvLineTo(IN PSURFOBJ SurfObj,
	  IN PCLIPOBJ ClipObj,
	  IN PBRUSHOBJ BrushObj,
	  IN LONG x1,
	  IN LONG y1,
	  IN LONG x2,
	  IN LONG y2,
	  IN PRECTL Bounds,
	  IN MIX Mix);
ULONG STDCALL
DrvLoadFontFile(IN ULONG FileNumber,
		IN PVOID ViewData,
		IN ULONG ViewSize,
		IN ULONG LangID);
VOID STDCALL
DrvMovePointer(IN PSURFOBJ SurfObj,
	       IN LONG x,
	       IN LONG y,
	       IN PRECTL RectL);
BOOL STDCALL
DrvNextBand(IN PSURFOBJ SurfObj,
	    OUT PPOINTL PointL);
BOOL STDCALL
DrvPaint(IN PSURFOBJ SurfObj,
	 IN PCLIPOBJ ClipObj,
	 IN PBRUSHOBJ BrushObj,
	 IN PPOINTL BrushOrg,
	 IN MIX Mix);
BOOL STDCALL
DrvQueryAdvanceWidths(IN DHPDEV DPev,
		      IN PFONTOBJ FontObj,
		      IN ULONG Mode,
		      IN HGLYPH Glyph,
		      OUT PVOID *Widths,
		      IN ULONG NumGlyphs);
PIFIMETRICS STDCALL
DrvQueryFont(IN DHPDEV PDev,
	     IN ULONG FileNumber,
	     IN ULONG FaceIndex,
	     IN PULONG Identifier);
LONG STDCALL
DrvQueryFontCaps(IN ULONG CapsSize,
		 OUT PULONG CapsData);
LONG STDCALL
DrvQueryFontData(IN DHPDEV DPev,
		 IN PFONTOBJ FontObj,
		 IN ULONG Mode,
		 IN HGLYPH Glyph,
		 IN PGLYPHDATA GlyphData,
		 IN PVOID DataBuffer,
		 IN ULONG BufferSize);
LONG STDCALL
DrvQueryFontFile(IN ULONG FileNumber,
		 IN ULONG Mode,
		 IN ULONG BufSize,
		 OUT PULONG Buf);
PVOID STDCALL
DrvQueryFontTree(IN DHPDEV PDev,
		 IN ULONG FileNumber,
		 IN ULONG FaceIndex,
		 IN ULONG Mode,
		 OUT ULONG *ID);
BOOL STDCALL
DrvQuerySpoolType(DHPDEV PDev,
		  LPWSTR SpoolType);
LONG STDCALL
DrvQueryTrueTypeOutline(IN DHPDEV PDev,
			IN PFONTOBJ FontObj,
			IN HGLYPH Glyph,
			IN BOOL MetricsOnly,
			IN PGLYPHDATA GlyphData,
			IN ULONG BufSize,
			OUT PTTPOLYGONHEADER Polygons);
LONG STDCALL
DrvQueryTrueTypeTable(IN ULONG FileNumber,
		      IN ULONG Font,
		      IN ULONG Tag,
		      IN PTRDIFF Start,
		      IN ULONG BufSize,
		      OUT BYTE *Buf);
BOOL STDCALL
DrvRealizeBrush(IN PBRUSHOBJ BrushObj,
		IN PSURFOBJ TargetSurface,
		IN PSURFOBJ PatternSurface,
		IN PSURFOBJ MaskSurface,
		IN PXLATEOBJ XLateObj,
		IN ULONG iHatch);
BOOL STDCALL
DrvResetPDEV(IN DHPDEV PDevOld,
	     IN DHPDEV PDevNew);
ULONG STDCALL
DrvSaveScreenBits(IN PSURFOBJ SurfObj,
		  IN ULONG Mode,
		  IN ULONG ID,
		  IN PRECTL RectL);
BOOL STDCALL
DrvSendPage(IN PSURFOBJ SurfObj);
BOOL STDCALL
DrvSetPalette(IN DHPDEV PDev,
	      IN PPALOBJ PaletteObj,
	      IN ULONG Flags,
	      IN ULONG Start,
	      IN ULONG NumColors);
ULONG STDCALL
DrvSetPointerShape(IN PSURFOBJ SurfObj,
		   IN PSURFOBJ MaskSurface,
		   IN PSURFOBJ ColorSurface,
		   IN PXLATEOBJ XLateObj,
		   IN LONG xHot,
		   IN LONG yHot,
		   IN LONG x,
		   IN LONG y,
		   IN PRECTL RectL,
		   IN ULONG Flags);
BOOL STDCALL
DrvStartBanding(IN PSURFOBJ SurfObj,
		IN PPOINTL PointL);
BOOL STDCALL
DrvStartDoc(IN PSURFOBJ SurfObj,
	    IN LPWSTR DocName,
	    IN DWORD JobID);
BOOL STDCALL
DrvStartPage(IN PSURFOBJ SurfObj);
BOOL STDCALL
DrvStretchBlt(IN PSURFOBJ DestSurface,
	      IN PSURFOBJ SrcSurface,
	      IN PSURFOBJ MaskSurface,
	      IN PCLIPOBJ ClipObj,
	      IN PXLATEOBJ XLateObj,
	      IN PCOLORADJUSTMENT CA,
	      IN PPOINTL HTOrg,
	      IN PRECTL Dest,
	      IN PRECTL Src,
	      IN PPOINTL Mask,
	      IN ULONG Mode);
BOOL STDCALL
DrvStrokeAndFillPath(IN PSURFOBJ SurfObj,
		     IN PPATHOBJ PathObj,
		     IN PCLIPOBJ ClipObj,
		     IN PXFORMOBJ XFormObj,
		     IN PBRUSHOBJ StrokeBrush,
		     IN PLINEATTRS LineAttrs,
		     IN PBRUSHOBJ FillBrush,
		     IN PPOINTL BrushOrg,
		     IN MIX MixFill,
		     IN ULONG Options);
BOOL STDCALL
DrvStrokePath(IN PSURFOBJ SurfObj,
	      IN PPATHOBJ PathObj,
	      IN PCLIPOBJ PClipObj,
	      IN PXFORMOBJ XFormObj,
	      IN PBRUSHOBJ BrushObj,
	      IN PPOINTL BrushOrg,
	      IN PLINEATTRS LineAttrs,
	      IN MIX Mix);
VOID STDCALL
DrvSynchronize(IN DHPDEV PDev,
	       IN PRECTL RectL);
BOOL STDCALL
DrvTextOut(IN PSURFOBJ SurfObj,
	   IN PSTROBJ StrObj,
	   IN PFONTOBJ FontObj,
	   IN PCLIPOBJ ClipObj,
	   IN PRECTL ExtraRect,
	   IN PRECTL OpaqueRect,
	   IN PBRUSHOBJ ForegroundBrush,
	   IN PBRUSHOBJ OpaqueBrush,
	   IN PPOINTL OrgPoint,
	   IN MIX Mix);
BOOL STDCALL
DrvTransparentBlt(PSURFOBJ Dest,
		  PSURFOBJ Source,
		  PCLIPOBJ Clip,
		  PXLATEOBJ ColorTranslation,
		  PRECTL DestRect,
		  PRECTL SourceRect,
		  ULONG TransparentColor,
		  ULONG Reserved);
BOOL STDCALL
DrvUnloadFontFile(IN ULONG FileNumber);

/*  DDI --> GDI calls  */
PVOID STDCALL
BRUSHOBJ_pvAllocRbrush(IN PBRUSHOBJ BrushObj,
		       IN ULONG ObjSize);
PVOID STDCALL
BRUSHOBJ_pvGetRbrush(IN PBRUSHOBJ BrushObj);

BOOL STDCALL
CLIPOBJ_bEnum(IN PCLIPOBJ ClipObj,
	      IN ULONG ObjSize,
	      OUT ULONG *EnumRects);

ULONG STDCALL
CLIPOBJ_cEnumStart(IN PCLIPOBJ ClipObj,
		   IN BOOL ShouldDoAll,
		   IN ULONG ClipType,
		   IN ULONG BuildOrder,
		   IN ULONG MaxRects);

PPATHOBJ STDCALL
CLIPOBJ_ppoGetPath(PCLIPOBJ ClipObj);

#define  FL_ZERO_MEMORY  1
#define  FL_NONPAGED_MEMORY 2

PVOID STDCALL
EngAllocMem(ULONG Flags,
	    ULONG MemSize,
	    ULONG Tag);

PVOID STDCALL
EngAllocUserMem(ULONG cj,
		ULONG tag);

BOOL STDCALL
EngAssociateSurface(IN HSURF Surface,
		    IN HDEV Dev,
		    IN ULONG Hooks);

BOOL STDCALL
EngBitBlt(SURFOBJ *Dest,
	  SURFOBJ *Source,
	  SURFOBJ *Mask,
	  CLIPOBJ *ClipRegion,
	  XLATEOBJ *ColorTranslation,
	  RECTL *DestRect,
	  POINTL *SourcePoint,
	  POINTL *MaskRect,
	  BRUSHOBJ *Brush,
	  POINTL *BrushOrigin,
	  ROP4 rop4);

/*
EngComputeGlyphSet
*/

BOOL STDCALL
EngCopyBits(SURFOBJ *Dest,
	    SURFOBJ *Source,
	    CLIPOBJ *Clip,
	    XLATEOBJ *ColorTranslation,
	    RECTL *DestRect,
	    POINTL *SourcePoint);

HBITMAP STDCALL
EngCreateBitmap(IN SIZEL Size,
		IN LONG Width,
		IN ULONG Format,
		IN ULONG Flags,
		IN PVOID Bits);

PCLIPOBJ STDCALL
EngCreateClip(VOID);

HBITMAP STDCALL
EngCreateDeviceBitmap(IN DHSURF Surface,
		      IN SIZEL Size,
		      IN ULONG Format);

HSURF STDCALL
EngCreateDeviceSurface(IN DHSURF Surface,
		       IN SIZEL Size,
		       IN ULONG FormatVersion);

/*
EngCreateDriverObj
EngCreateEvent
*/

HPALETTE STDCALL
EngCreatePalette(IN ULONG Mode,
		 IN ULONG NumColors,
		 IN ULONG *Colors,
		 IN ULONG Red,
		 IN ULONG Green,
		 IN ULONG Blue);

HSEMAPHORE
STDCALL
EngCreateSemaphore ( VOID );

VOID
STDCALL
EngAcquireSemaphore ( IN HSEMAPHORE hsem );

VOID
STDCALL
EngReleaseSemaphore ( IN HSEMAPHORE hsem );

VOID
STDCALL
EngDeleteSemaphore ( IN HSEMAPHORE hsem );

BOOL
STDCALL
EngIsSemaphoreOwned ( IN HSEMAPHORE hsem );

BOOL
STDCALL
EngIsSemaphoreOwnedByCurrentThread ( IN HSEMAPHORE hsem );

/*
EngCreatePath
EngCreateSemaphore
EngCreateWnd
*/

VOID STDCALL
EngDebugBreak(VOID);

VOID STDCALL
EngDebugPrint(PCHAR StandardPrefix,
	      PCHAR DebugMessage,
	      va_list ArgList);

VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion);

/*
EngDeleteDriverObj
EngDeleteEvent
*/

BOOL STDCALL
EngDeletePalette(IN HPALETTE Palette);

BOOL STDCALL
EngDeleteSurface(IN HSURF Surface);

/*
EngDeleteWnd
*/

DWORD STDCALL
EngDeviceIoControl(HANDLE hDevice,
		   DWORD dwIoControlCode,
		   LPVOID lpInBuffer,
		   DWORD nInBufferSize,
		   LPVOID lpOutBuffer,
		   DWORD nOutBufferSize,
		   DWORD *lpBytesReturned);

/*
EngEnumForms
*/

BOOL STDCALL
EngEraseSurface(SURFOBJ *Surface,
		RECTL *Rect,
		ULONG iColor);

/*
EngFindImageProcAddress
EngFindResource
*/

VOID STDCALL
EngFreeMem(PVOID Mem);

/*
EngFreeModule
*/

VOID STDCALL
EngFreeUserMem(PVOID pv);

VOID STDCALL
EngGetCurrentCodePage(OUT PUSHORT OemCodePage,
		      OUT PUSHORT AnsiCodePage);

/*
EngGetFileChangeTime
EngGetFilePath
EngGetForm
EngGetLastError
EngGetPrinter
EngGetPrinterData
EngGetProcessHandle
EngGetType1FontList
*/

BOOL STDCALL
EngLineTo(SURFOBJ *Surface,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix);

HANDLE STDCALL
EngLoadImage(LPWSTR DriverName);

/*
EngLoadModuleForWrite
EngLockDriverObj
*/

SURFOBJ * STDCALL
EngLockSurface(IN HSURF Surface);

/*
EngMapEvent
EngMapFontFile
EngMapModule
EngMovePointer
*/

INT STDCALL
EngMulDiv(IN INT nMultiplicand,
	  IN INT nMultiplier,
	  IN INT nDivisor);

VOID STDCALL
EngMultiByteToUnicodeN(OUT LPWSTR UnicodeString,
		       IN ULONG MaxBytesInUnicodeString,
		       OUT PULONG BytesInUnicodeString,
		       IN PCHAR MultiByteString,
		       IN ULONG BytesInMultiByteString);

BOOL STDCALL
EngPaint(IN SURFOBJ *Surface,
	 IN CLIPOBJ *ClipRegion,
	 IN BRUSHOBJ *Brush,
	 IN POINTL *BrushOrigin,
	 IN MIX  Mix);

/*
EngProbeForRead
EngProbeForReadAndWrite = NTOSKRNL.ProbeForWrite
EngQueryLocalTime
EngQueryPalette
EngQueryPerformanceCounter
EngQueryPerformanceFrequency
EngRestoreFloatingPointState
EngSaveFloatingPointState
EngSecureMem
EngSetEvent
EngSetLastError
EngSetPointerTag
EngSetPrinterData
*/

ULONG
STDCALL
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
	IN FLONG  fl
	);

VOID
STDCALL
EngMovePointer(
	IN SURFOBJ  *pso,
	IN LONG      x,
	IN LONG      y,
	IN RECTL    *prcl
	);

typedef int CDECL (*SORTCOMP)(const void *Elem1, const void *Elem2);

void STDCALL
EngSort(IN OUT PBYTE Buf, IN ULONG ElemSize, IN ULONG ElemCount, IN SORTCOMP CompFunc);

/*
EngStrokeAndFillPath
EngStrokePath
EngTextOut
*/

BOOL STDCALL
EngTransparentBlt(IN PSURFOBJ Dest,
		  IN PSURFOBJ Source,
		  IN PCLIPOBJ Clip,
		  IN PXLATEOBJ ColorTranslation,
		  IN PRECTL DestRect,
		  IN PRECTL SourceRect,
		  IN ULONG TransparentColor,
		  IN ULONG Reserved);

VOID STDCALL
EngUnicodeToMultiByteN(OUT PCHAR MultiByteString,
		       IN ULONG  MaxBytesInMultiByteString,
		       OUT PULONG  BytesInMultiByteString,
		       IN PWSTR  UnicodeString,
		       IN ULONG  BytesInUnicodeString);

/*
EngUnloadImage
EngUnlockDriverObj
EngUnmapEvent
EngUnmapFontFile
EngUnsecureMem = NTOSKRNL.MmUnsecureVirtualMemory
EngWaitForSingleObject
EngWideCharToMultiByte
EngWritePrinter
FLOATOBJ_Add
FLOATOBJ_AddFloat
FLOATOBJ_AddFloatObj
FLOATOBJ_AddLong
FLOATOBJ_Div
FLOATOBJ_DivFloat
FLOATOBJ_DivFloatObj
FLOATOBJ_DivLong
FLOATOBJ_Equal
FLOATOBJ_EqualLong
FLOATOBJ_GetFloat
FLOATOBJ_GetLong
FLOATOBJ_GreaterThan
FLOATOBJ_GreaterThanLong
FLOATOBJ_LessThan
FLOATOBJ_LessThanLong
FLOATOBJ_Mul
FLOATOBJ_MulFloat
FLOATOBJ_MulFloatObj
FLOATOBJ_MulLong
FLOATOBJ_Neg
FLOATOBJ_SetFloat
FLOATOBJ_SetLong
FLOATOBJ_Sub
FLOATOBJ_SubFloat
FLOATOBJ_SubFloatObj
FLOATOBJ_SubLong
*/

ULONG
STDCALL
FONTOBJ_cGetAllGlyphHandles(IN PFONTOBJ  FontObj,
                            IN HGLYPH  *Glyphs);

ULONG
STDCALL
FONTOBJ_cGetGlyphs(IN PFONTOBJ FontObj,
                   IN ULONG    Mode,
                   IN ULONG    NumGlyphs,
                   IN HGLYPH  *GlyphHandles,
                   IN PVOID   *OutGlyphs);

PGAMMA_TABLES
STDCALL
FONTOBJ_pGetGammaTables(IN PFONTOBJ FontObj);

IFIMETRICS*
STDCALL
FONTOBJ_pifi(IN PFONTOBJ  FontObj);

PVOID
STDCALL
FONTOBJ_pvTrueTypeFontFile(IN PFONTOBJ  FontObj,
                           IN ULONG    *FileSize);

XFORMOBJ*
STDCALL
FONTOBJ_pxoGetXform(IN PFONTOBJ  FontObj);

VOID
STDCALL
FONTOBJ_vGetInfo(IN  PFONTOBJ   FontObj,
                 IN  ULONG      InfoSize,
                 OUT PFONTINFO  FontInfo);

/*
HT_ComputeRGBGammaTable
HT_Get8BPPFormatPalette
*/

ULONG
STDCALL
PALOBJ_cGetColors(PALOBJ *PalObj,
		  ULONG Start,
		  ULONG Colors,
		  ULONG *PaletteEntry);

/*
PATHOBJ_bCloseFigure
PATHOBJ_bEnumClipLines
PATHOBJ_bMoveTo
PATHOBJ_bPolyBezierTo
PATHOBJ_bPolyLineTo
PATHOBJ_vEnumStart
PATHOBJ_vEnumStartClipLines
RtlAnsiCharToUnicodeChar = NTOSKRNL.RtlAnsiCharToUnicodeChar
RtlMultiByteToUnicodeN = NTOSKRNL.RtlMultiByteToUnicodeN
RtlRaiseException = NTOSKRNL.RtlRaiseException
RtlUnicodeToMultiByteN = NTOSKRNL.RtlUnicodeToMultiByteN
RtlUnicodeToMultiByteSize = NTOSKRNL.RtlUnicodeToMultiByteSize
RtlUnwind = NTOSKRNL.RtlUnwind
RtlUpcaseUnicodeChar = NTOSKRNL.RtlUpcaseUnicodeChar
RtlUpcaseUnicodeToMultiByteN = NTOSKRNL.RtlUpcaseUnicodeToMultiByteN
STROBJ_bEnum
STROBJ_dwGetCodePage
STROBJ_vEnumStart
WNDOBJ_bEnum
WNDOBJ_cEnumStart
WNDOBJ_vSetConsumer
XFORMOBJ_bApplyXform
XFORMOBJ_iGetFloatObjXform
XFORMOBJ_iGetXform
*/

ULONG STDCALL
XLATEOBJ_cGetPalette(XLATEOBJ *XlateObj,
		     ULONG PalOutType,
		     ULONG cPal,
		     ULONG *OutPal);

ULONG STDCALL
XLATEOBJ_iXlate(XLATEOBJ *XlateObj,
		ULONG Color);

ULONG * STDCALL
XLATEOBJ_piVector(XLATEOBJ *XlateObj);

HANDLE STDCALL
BRUSHOBJ_hGetColorTransform(BRUSHOBJ *pbo);
ULONG STDCALL
BRUSHOBJ_ulGetBrushColor(BRUSHOBJ *pbo);
BOOL STDCALL 
EngAlphaBlend(SURFOBJ *psoDest,SURFOBJ *psoSrc,CLIPOBJ *pco,XLATEOBJ *pxlo,RECTL *prclDest,RECTL *prclSrc,BLENDOBJ *pBlendObj);
BOOL STDCALL
EngCheckAbort(SURFOBJ *pso);
FD_GLYPHSET* STDCALL
EngComputeGlyphSet(INT nCodePage,INT nFirstChar,INT cChars);
HANDLE STDCALL
EngGetCurrentProcessId(VOID);
HANDLE STDCALL
EngGetCurrentThreadId(VOID);
VOID STDCALL
EngDeletePath(PATHOBJ *ppo);
BOOL STDCALL
EngFillPath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,BRUSHOBJ *pbo,POINTL *pptlBrushOrg,MIX mix,FLONG flOptions);
PVOID STDCALL
EngFindResource(HANDLE h,int iName,int iType,PULONG pulSize);
VOID STDCALL 
EngFreeModule(HANDLE h);
LPWSTR STDCALL
EngGetDriverName(HDEV hdev);
LPWSTR STDCALL
EngGetPrinterDataFileName(HDEV hdev);
HANDLE STDCALL
EngGetProcessHandle(VOID);
BOOL STDCALL 
EngGradientFill(SURFOBJ *psoDest,CLIPOBJ *pco,XLATEOBJ *pxlo,TRIVERTEX *pVertex,ULONG nVertex,PVOID pMesh,ULONG nMesh,RECTL *prclExtents,POINTL *pptlDitherOrg,ULONG ulMode);
HANDLE STDCALL 
EngLoadModule(LPWSTR pwsz);
BOOL STDCALL 
EngMarkBandingSurface(HSURF hsurf);
INT STDCALL 
EngMultiByteToWideChar(UINT CodePage,LPWSTR WideCharString,INT BytesInWideCharString,LPSTR MultiByteString,INT BytesInMultiByteString);
BOOL STDCALL 
EngPlgBlt(SURFOBJ *psoTrg,SURFOBJ *psoSrc,SURFOBJ *psoMsk,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlBrushOrg,POINTFIX *pptfx,RECTL *prcl,POINTL *pptl,ULONG iMode);
BOOL STDCALL
EngQueryEMFInfo(HDEV hdev,EMFINFO *pEMFInfo);
VOID STDCALL 
EngQueryLocalTime(PENG_TIME_FIELDS etf);
BOOL STDCALL 
EngStretchBlt(SURFOBJ *psoDest,SURFOBJ *psoSrc,SURFOBJ *psoMask,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlHTOrg,RECTL *prclDest,RECTL *prclSrc,POINTL *pptlMask,ULONG iMode);
BOOL STDCALL 
EngStretchBltROP(SURFOBJ *psoDest,SURFOBJ *psoSrc,SURFOBJ *psoMask,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlHTOrg,RECTL *prclDest,RECTL *prclSrc,POINTL *pptlMask,ULONG iMode,BRUSHOBJ *pbo,DWORD rop4);
BOOL STDCALL 
EngStrokeAndFillPath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,XFORMOBJ *pxo,BRUSHOBJ *pboStroke,LINEATTRS *plineattrs,BRUSHOBJ *pboFill,POINTL *pptlBrushOrg,MIX mixFill,FLONG flOptions);
BOOL STDCALL
EngStrokePath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,XFORMOBJ *pxo,BRUSHOBJ *pbo,POINTL *pptlBrushOrg,LINEATTRS *plineattrs,MIX mix);
BOOL STDCALL 
EngTextOut(SURFOBJ *pso,STROBJ *pstro,FONTOBJ *pfo,CLIPOBJ *pco,RECTL *prclExtra,RECTL *prclOpaque,BRUSHOBJ *pboFore,BRUSHOBJ *pboOpaque,POINTL *pptlOrg,MIX mix);
VOID STDCALL 
EngUnlockSurface(SURFOBJ *pso);
INT STDCALL 
EngWideCharToMultiByte(UINT CodePage,LPWSTR WideCharString,INT BytesInWideCharString,LPSTR MultiByteString,INT BytesInMultiByteString);
PFD_GLYPHATTR STDCALL
FONTOBJ_pQueryGlyphAttrs(FONTOBJ *pfo,ULONG iMode);
VOID STDCALL
PATHOBJ_vGetBounds(PATHOBJ *ppo,PRECTFX prectfx);
FD_GLYPHSET *STDCALL
FONTOBJ_pfdg(FONTOBJ *pfo);
BOOL STDCALL
PATHOBJ_bEnum(PATHOBJ *ppo,PATHDATA *ppd);
BOOL STDCALL 
PATHOBJ_bEnumClipLines(PATHOBJ *ppo,ULONG cb,CLIPLINE *pcl);
VOID STDCALL 
PATHOBJ_vEnumStart(PATHOBJ *ppo);
VOID STDCALL
PATHOBJ_vEnumStartClipLines(PATHOBJ *ppo,CLIPOBJ *pco,SURFOBJ *pso,LINEATTRS *pla);
BOOL STDCALL
STROBJ_bEnum(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos);
BOOL STDCALL
STROBJ_bEnumPositionsOnly(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos);
BOOL STDCALL
STROBJ_bGetAdvanceWidths(STROBJ *pso,ULONG iFirst,ULONG c,POINTQF *pptqD);
DWORD STDCALL
STROBJ_dwGetCodePage(STROBJ  *pstro);
VOID STDCALL
STROBJ_vEnumStart(STROBJ *pstro);
ULONG STDCALL
XFORMOBJ_iGetXform(XFORMOBJ *pxo,XFORML *pxform);
BOOL STDCALL
XFORMOBJ_bApplyXform(XFORMOBJ *pxo,ULONG iMode,ULONG cPoints,PVOID pvIn,PVOID pvOut);
HANDLE STDCALL
XLATEOBJ_hGetColorTransform(XLATEOBJ *pxlo);

#endif /* __USE_W32API */

#endif
