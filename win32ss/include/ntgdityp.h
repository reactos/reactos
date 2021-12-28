/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            win32ss/include/ntgdityp.h
 * PURPOSE:         Win32 Shared GDI Types for NtGdi*
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTGDITYP_
#define _NTGDITYP_

#include "ntwin32.h"

/* ENUMERATIONS **************************************************************/

typedef enum _ARCTYPE
{
    GdiTypeArc,
    GdiTypeArcTo,
    GdiTypeChord,
    GdiTypePie,
} ARCTYPE, *PARCTYPE;

typedef enum _PALFUNCTYPE
{
    GdiPalAnimate,
    GdiPalSetEntries,
    GdiPalGetEntries,
    GdiPalGetSystemEntries,
    GdiPalSetColorTable,
    GdiPalGetColorTable,
} PALFUNCTYPE, *PPALFUNCTYPE;

typedef enum _POLYFUNCTYPE
{
    GdiPolyPolygon = 1,
    GdiPolyPolyLine,
    GdiPolyBezier,
    GdiPolyLineTo,
    GdiPolyBezierTo,
    GdiPolyPolyRgn,
} POLYFUNCTYPE, *PPOLYFUNCTYPE;

typedef enum _GETDCDWORD
{
    GdiGetJournal,
    GdiGetRelAbs,
    GdiGetBreakExtra,
    GdiGetCharBreak,
    GdiGetArcDirection,
    GdiGetEMFRestorDc,
    GdiGetFontLanguageInfo,
    GdiGetIsMemDc,
    GdiGetMapMode,
    GdiGetTextCharExtra,
} GETDCDWORD, *PGETDCDWORD;

typedef enum _GETSETDCDWORD
{
    GdiGetSetEPSPrintingEscape = 1,
    GdiGetSetCopyCount,
    GdiGetSetTextAlign,
    GdiGetSetRelAbs,
    GdiGetSetTextCharExtra,
    GdiGetSetSelectFont,
    GdiGetSetMapperFlagsInternal,
    GdiGetSetMapMode,
    GdiGetSetArcDirection,
} GETSETDCDWORD, *PGETSETDCDWORD;

typedef enum _GETDCPOINT
{
    GdiGetViewPortExt = 1,
    GdiGetWindowExt,
    GdiGetViewPortOrg,
    GdiGetWindowOrg,
    GdiGetAspectRatioFilter,
    GdiGetDCOrg = 6,
} GETDCPOINT, *PGETDCPOINT;


typedef enum _GDIBATCHCMD
{
    GdiBCPatBlt,
    GdiBCPolyPatBlt,
    GdiBCTextOut,
    GdiBCExtTextOut,
    GdiBCSetBrushOrg,
    GdiBCExtSelClipRgn,
    GdiBCSelObj,
    GdiBCDelObj,
    GdiBCDelRgn,
} GDIBATCHCMD, *PGDIBATCHCMD;

typedef enum _TRANSFORMTYPE
{
    GdiDpToLp,
    GdiLpToDp,
} TRANSFORMTYPE, *PTRANSFORMTYPE;

/* MATRIX flAccel flags */
enum
{
    XFORM_SCALE = 1,
    XFORM_UNITY = 2,
    XFORM_Y_NEG = 4,
    XFORM_FORMAT_LTOFX = 8,
    XFORM_FORMAT_FXTOL = 0x10,
    XFORM_FORMAT_LTOL = 0x20,
    XFORM_NO_TRANSLATION = 0x40,

    /* ReactOS specific */
    XFORM_INTEGER = 0x1000,
};

typedef enum GDIObjType
{
    GDIObjType_DEF_TYPE = 0x00,
    GDIObjType_DC_TYPE = 0x01,
    GDIObjType_UNUSED1_TYPE = 0x02,
    GDIObjType_UNUSED2_TYPE = 0x03,
    GDIObjType_RGN_TYPE = 0x04,
    GDIObjType_SURF_TYPE = 0x05,
    GDIObjType_CLIENTOBJ_TYPE = 0x06,
    GDIObjType_PATH_TYPE = 0x07,
    GDIObjType_PAL_TYPE = 0x08,
    GDIObjType_ICMLCS_TYPE = 0x09,
    GDIObjType_LFONT_TYPE = 0x0a,
    GDIObjType_RFONT_TYPE = 0x0b,
    GDIObjType_PFE_TYPE = 0x0c,
    GDIObjType_PFT_TYPE = 0x0d,
    GDIObjType_ICMCXF_TYPE = 0x0e,
    GDIObjType_SPRITE_TYPE = 0x0f,
    GDIObjType_BRUSH_TYPE = 0x10,
    GDIObjType_UMPD_TYPE = 0x11,
    GDIObjType_UNUSED4_TYPE = 0x12,
    GDIObjType_SPACE_TYPE = 0x13,
    GDIObjType_UNUSED5_TYPE = 0x14,
    GDIObjType_META_TYPE = 0x15,
    GDIObjType_EFSTATE_TYPE = 0x16,
    GDIObjType_BMFD_TYPE = 0x17,
    GDIObjType_VTFD_TYPE = 0x18,
    GDIObjType_TTFD_TYPE = 0x19,
    GDIObjType_RC_TYPE = 0x1a,
    GDIObjType_TEMP_TYPE = 0x1b,
    GDIObjType_DRVOBJ_TYPE = 0x1c,
    GDIObjType_DCIOBJ_TYPE = 0x1d,
    GDIObjType_SPOOL_TYPE = 0x1e,
    GDIObjType_MAX_TYPE = 0x1e,
    GDIObjTypeTotal = 0x1f,
} GDIOBJTYPE, *PGDIOBJTYPE;

typedef enum GDILoObjType
{
    GDILoObjType_LO_BRUSH_TYPE = 0x100000,
    GDILoObjType_LO_DC_TYPE = 0x10000,
    GDILoObjType_LO_BITMAP_TYPE = 0x50000,
    GDILoObjType_LO_PALETTE_TYPE = 0x80000,
    GDILoObjType_LO_FONT_TYPE = 0xa0000,
    GDILoObjType_LO_REGION_TYPE = 0x40000,
    GDILoObjType_LO_ICMLCS_TYPE = 0x90000,
    GDILoObjType_LO_CLIENTOBJ_TYPE = 0x60000,
    GDILoObjType_LO_UMPD_TYPE = 0x110000,
    GDILoObjType_LO_META_TYPE = 0x150000,
    GDILoObjType_LO_ALTDC_TYPE = 0x210000,
    GDILoObjType_LO_PEN_TYPE = 0x300000,
    GDILoObjType_LO_EXTPEN_TYPE = 0x500000,
    GDILoObjType_LO_DIBSECTION_TYPE = 0x250000,
    GDILoObjType_LO_METAFILE16_TYPE = 0x260000,
    GDILoObjType_LO_METAFILE_TYPE = 0x460000,
    GDILoObjType_LO_METADC16_TYPE = 0x660000
} GDILOOBJTYPE, *PGDILOOBJTYPE;

/**
       World Transform modification modes
       See [MS-EMF] Section 2.1.24
*/
#define MWT_SET 0x04

#define GdiWorldSpaceToPageSpace    0x203
#define GdiWorldSpaceToDeviceSpace  0x204
#define GdiPageSpaceToDeviceSpace   0x304
#define GdiDeviceSpaceToWorldSpace  0x402

/* FIXME: Unknown */
typedef DWORD FULLSCREENCONTROL;
typedef DWORD LFTYPE;

/* Public LAST_STOCK = 19, plus 2 more internal entries */
#define NB_STOCK_OBJECTS 22

/* extra stock object: default 1x1 bitmap for memory DCs */
#define DEFAULT_BITMAP (21)


/* DEFINES *******************************************************************/

#define GDIBATCHBUFSIZE 0x136*4
#define GDI_BATCH_LIMIT 20

// NtGdiGetCharWidthW Flags
#define GCW_WIN32   0x0001
#define GCW_NOFLOAT 0x0002
#define GCW_INDICES 0x0008

// NtGdiGetCharABCWidthW Flags
#define GCABCW_NOFLOAT 0x0001
#define GCABCW_INDICES 0x0002

// NtGdiGetTextExtent* flags (reactos own)
#define GTEF_INDICES 0x1

/* CAPS1 support */
#define CAPS1             94
//#define C1_TRANSPARENT    0x0001
#define TC_TT_ABLE        0x0002
#define C1_TT_CR_ANY      0x0004
#define C1_EMF_COMPLIANT  0x0008
#define C1_DIBENGINE      0x0010
#define C1_GAMMA_RAMP     0x0040
#define C1_REINIT_ABLE    0x0080
#define C1_GLYPH_INDEX    0x0100
#define C1_BIT_PACKED     0x0200
#define C1_BYTE_PACKED    0x0400
#define C1_COLORCURSOR    0x0800
#define C1_CMYK_ABLE      0x1000
#define C1_SLOW_CARD      0x2000
#define C1_MIRRORING      0X4000

// NtGdiGetRandomRgn iCodes
#define CLIPRGN 1 // GetClipRgn
#define METARGN 2 // GetMetaRgn
#define APIRGN  3

/* New flag for fdwInit in CreateDIBitmap. See support.microsoft.com/kb/kbview/108497*/
#define CBM_CREATDIB 2

/* New color use parameter. See support.microsoft.com/kb/kbview/108497 */
#define DIB_PAL_INDICES 2

/* Get/SetBounds/Rect support. */
#define DCB_WINDOWMGR 0x8000 /* Queries the Windows bounding rectangle instead of the application's */

#define GDITAG_TYPE_EMF 'XEFM' // EnhMetaFile
#define GDITAG_TYPE_MFP '_PFM' // MetaFile Picture

/* TYPES *********************************************************************/

typedef PVOID KERNEL_PVOID;
typedef PVOID PUMDHPDEV;
typedef D3DNTHAL_CONTEXTCREATEDATA D3DNTHAL_CONTEXTCREATEI;
#if !defined(_WINDDI_)
typedef LONG FIX;
#endif

typedef struct _CHWIDTHINFO // Based on FD_DEVICEMETRICS
{
   LONG    lMinA;
   LONG    lMinC;
   LONG    lMinD;
} CHWIDTHINFO, *PCHWIDTHINFO;

typedef struct _UNIVERSAL_FONT_ID
{
    ULONG CheckSum;
    ULONG Index;
} UNIVERSAL_FONT_ID, *PUNIVERSAL_FONT_ID;

#define RI_TECH_BITMAP   1
#define RI_TECH_FIXED    2
#define RI_TECH_SCALABLE 3

typedef struct _REALIZATION_INFO
{
    DWORD  iTechnology;
    DWORD  iUniq;
    DWORD  dwUnknown;
} REALIZATION_INFO, *PREALIZATION_INFO;

typedef struct _WIDTHDATA
{
    USHORT      sOverhang;
    USHORT      sHeight;
    USHORT      sCharInc;
    USHORT      sBreak;
    BYTE        jFirst;
    BYTE        jLast;
    BYTE        jBreak;
    BYTE        jDefault;
    USHORT      sDBCSInc;
    USHORT      sDefaultInc;
} WIDTHDATA, *PWIDTHDATA;

typedef struct _DEVCAPS // Very similar to GDIINFO
{
    ULONG ulVersion;
    ULONG ulTechnology;
    ULONG ulHorzSizeM;
    ULONG ulVertSizeM;
    ULONG ulHorzSize;
    ULONG ulVertSize;
    ULONG ulHorzRes;
    ULONG ulVertRes;
    ULONG ulBitsPixel;
    ULONG ulPlanes;
    ULONG ulNumPens;
    ULONG ulNumFonts;
    ULONG ulNumColors;
    ULONG ulRasterCaps;
    ULONG ulAspectX;
    ULONG ulAspectY;
    ULONG ulAspectXY;
    ULONG ulLogPixelsX;
    ULONG ulLogPixelsY;
    ULONG ulSizePalette;
    ULONG ulColorRes;
    ULONG ulPhysicalWidth;
    ULONG ulPhysicalHeight;
    ULONG ulPhysicalOffsetX;
    ULONG ulPhysicalOffsetY;
    ULONG ulTextCaps;
    ULONG ulVRefresh;
    ULONG ulDesktopHorzRes;
    ULONG ulDesktopVertRes;
    ULONG ulBltAlignment;
    ULONG ulPanningHorzRes;
    ULONG ulPanningVertRes;
    ULONG xPanningAlignment;
    ULONG yPanningAlignment;
    ULONG ulShadeBlend;
    ULONG ulColorMgmtCaps;
} DEVCAPS, *PDEVCAPS;

/* Gdi Handle Cache Types and Structures */
#define GDI_CACHED_HANDLE_TYPES 4
#define CACHE_BRUSH_ENTRIES  10
#define CACHE_PEN_ENTRIES     8
#define CACHE_REGION_ENTRIES  8
#define CACHE_LFONT_ENTRIES   1

typedef enum _HANDLECACHETYPE
{
    hctBrushHandle,
    hctPenHandle,
    hctRegionHandle,
    hctLFontHandle
} HANDLECACHETYPE,*PHANDLECACHETYPE;

typedef struct _GDIHANDLECACHE
{
    ULONG           ulLock;
    ULONG           ulNumHandles[GDI_CACHED_HANDLE_TYPES];
    HANDLE          Handle[CACHE_BRUSH_ENTRIES+CACHE_PEN_ENTRIES+CACHE_REGION_ENTRIES+CACHE_LFONT_ENTRIES];
} GDIHANDLECACHE, *PGDIHANDLECACHE;

/* Font Structures */
typedef struct _TMDIFF
{
    ULONG cjotma;
    CHAR chFirst;
    CHAR chLast;
    CHAR ChDefault;
    CHAR ChBreak;
} TMDIFF, *PTMDIFF;

typedef struct _TMW_INTERNAL
{
    TEXTMETRICW TextMetric;
    TMDIFF Diff;
} TMW_INTERNAL, *PTMW_INTERNAL;

typedef struct _NTMW_INTERNAL
{
    TMDIFF tmd;
    NEWTEXTMETRICEXW ntmw;
} NTMW_INTERNAL, *PNTMW_INTERNAL;

typedef struct _ENUMFONTDATAW
{
    ULONG cbSize;
    ULONG ulNtmwiOffset;
    DWORD dwFontType;
    ENUMLOGFONTEXDVW elfexdv; /* variable size! */
    /* NTMW_INTERNAL ntmwi; use ulNtwmOffset */
} ENUMFONTDATAW, *PENUMFONTDATAW;

/* Number Representation */
typedef struct _EFLOAT_S
{
    LONG lMant;
    LONG lExp;
} EFLOAT_S;

/* XFORM Structures */
typedef struct _MATRIX_S
{
    EFLOAT_S efM11;
    EFLOAT_S efM12;
    EFLOAT_S efM21;
    EFLOAT_S efM22;
    EFLOAT_S efDx;
    EFLOAT_S efDy;
    FIX fxDx;
    FIX fxDy;
    FLONG flAccel;
} MATRIX_S;

typedef struct _MATRIX
{
    FLOATOBJ efM11;
    FLOATOBJ efM12;
    FLOATOBJ efM21;
    FLOATOBJ efM22;
    FLOATOBJ efDx;
    FLOATOBJ efDy;
    FIX fxDx;
    FIX fxDy;
    FLONG flAccel;
} MATRIX, *PMATRIX;

/* Gdi XForm storage union */
typedef union
{
  FLOAT f;
  ULONG l;
} gxf_long;

#define CFONT_REALIZATION 0x0080

typedef struct _CFONT
{
    struct _CFONT   *pcfNext;
    HFONT           hf;
    ULONG           cRef;               // Count of all pointers to this CFONT.
    FLONG           fl;
    LONG            lHeight;            // Precomputed logical height.
    HDC             hdc;                // HDC of realization.  0 for display.
    EFLOAT_S        efM11;              // efM11 of WtoD of DC of realization
    EFLOAT_S        efM22;              // efM22 of WtoD of DC of realization
    EFLOAT_S        efDtoWBaseline;     // Precomputed back transform.  (FXtoL)
    EFLOAT_S        efDtoWAscent;       // Precomputed back transform.  (FXtoL)
    WIDTHDATA       wd;
    FLONG           flInfo;
    USHORT          sWidth[256];        // Widths in pels.
    ULONG           ulAveWidth;         // bogus average used by USER
    TMW_INTERNAL    tmw;                // cached metrics
    DWORD           iTechnology;
    DWORD           iUniq;
    DWORD           dwUnknown;
    DWORD           dwCFCount;
} CFONT, *PCFONT;

/* GDI Batch structures. */
typedef struct _GDIBATCHHDR
{
  SHORT Size;
  SHORT Cmd;
} GDIBATCHHDR, *PGDIBATCHHDR;

typedef struct _GDIBSPATBLT
{
  GDIBATCHHDR gbHdr;
  int nXLeft;
  int nYLeft;
  int nWidth;
  int nHeight;
  HANDLE hbrush;
  DWORD dwRop;
  COLORREF crForegroundClr;
  COLORREF crBackgroundClr;
  COLORREF crBrushClr;
  INT IcmBrushColor;
  POINTL ptlViewportOrg;
  ULONG ulForegroundClr;
  ULONG ulBackgroundClr;
  ULONG ulBrushClr;
} GDIBSPATBLT, *PGDIBSPATBLT;

/* FIXME: this should go to some "public" GDI32 header */
typedef struct _PATRECT
{
  RECT r;
  HBRUSH hBrush;
} PATRECT, * PPATRECT;

typedef struct _GDIBSPPATBLT
{
  GDIBATCHHDR gbHdr;
  DWORD rop4;
  DWORD Mode;
  DWORD Count;
  COLORREF crForegroundClr;
  COLORREF crBackgroundClr;
  COLORREF crBrushClr;
  ULONG ulForegroundClr;
  ULONG ulBackgroundClr;
  ULONG ulBrushClr;
  POINTL ptlViewportOrg;
  PATRECT pRect[1]; // POLYPATBLT
} GDIBSPPATBLT, *PGDIBSPPATBLT;

//
// Both ExtSelectClipRgn and TextOut pass a nill RECT.
//
#define GDIBS_NORECT 0x80000000

typedef struct _GDIBSTEXTOUT
{
  GDIBATCHHDR gbHdr;
  COLORREF crForegroundClr;
  COLORREF crBackgroundClr;
  LONG lBkMode;
  ULONG ulForegroundClr;
  ULONG ulBackgroundClr;
  int x;
  int y;
  UINT Options;
  RECT Rect;
  DWORD iCS_CP;
  UINT cbCount;
  UINT Size;
  HANDLE hlfntNew;
  FLONG flTextAlign;
  POINTL ptlViewportOrg;
  union {
  WCHAR String[2];
  ULONG Buffer[1];
  };
} GDIBSTEXTOUT, *PGDIBSTEXTOUT;

typedef struct _GDIBSEXTTEXTOUT
{
  GDIBATCHHDR gbHdr;
  UINT Count;
  UINT Options;
  RECT Rect;
  POINTL ptlViewportOrg;
  ULONG ulBackgroundClr;
} GDIBSEXTTEXTOUT, *PGDIBSEXTTEXTOUT;

typedef struct _GDIBSSETBRHORG
{
  GDIBATCHHDR gbHdr;
  POINTL ptlBrushOrigin;
} GDIBSSETBRHORG, *PGDIBSSETBRHORG;

typedef struct _GDIBSEXTSELCLPRGN
{
  GDIBATCHHDR gbHdr;
  int fnMode;
  RECTL rcl;
} GDIBSEXTSELCLPRGN, *PGDIBSEXTSELCLPRGN;

/* Use with GdiBCSelObj, GdiBCDelObj and GdiBCDelRgn. */
typedef struct _GDIBSOBJECT
{
  GDIBATCHHDR gbHdr;
  HGDIOBJ hgdiobj;
} GDIBSOBJECT, *PGDIBSOBJECT;

/* Declaration missing in ddk/winddi.h */
typedef VOID (APIENTRY *PFN_DrvMovePanning)(LONG, LONG, FLONG);

/* Most of these are defined in ddk/winddi.h */
typedef struct _DRIVER_FUNCTIONS
{
    PFN_DrvEnablePDEV              EnablePDEV;
    PFN_DrvCompletePDEV            CompletePDEV;
    PFN_DrvDisablePDEV             DisablePDEV;
    PFN_DrvEnableSurface           EnableSurface;
    PFN_DrvDisableSurface          DisableSurface;
    PFN_DrvAssertMode              AssertMode;
    PFN_DrvOffset                  Offset;
    PFN_DrvResetPDEV               ResetPDEV;
    PFN_DrvDisableDriver           DisableDriver;
    PVOID                          Unknown1;
    PFN_DrvCreateDeviceBitmap      CreateDeviceBitmap;
    PFN_DrvDeleteDeviceBitmap      DeleteDeviceBitmap;
    PFN_DrvRealizeBrush            RealizeBrush;
    PFN_DrvDitherColor             DitherColor;
    PFN_DrvStrokePath              StrokePath;
    PFN_DrvFillPath                FillPath;
    PFN_DrvStrokeAndFillPath       StrokeAndFillPath;
    PFN_DrvPaint                   Paint;
    PFN_DrvBitBlt                  BitBlt;
    PFN_DrvCopyBits                CopyBits;
    PFN_DrvStretchBlt              StretchBlt;
    PVOID                          Unknown2;
    PFN_DrvSetPalette              SetPalette;
    PFN_DrvTextOut                 TextOut;
    PFN_DrvEscape                  Escape;
    PFN_DrvDrawEscape              DrawEscape;
    PFN_DrvQueryFont               QueryFont;
    PFN_DrvQueryFontTree           QueryFontTree;
    PFN_DrvQueryFontData           QueryFontData;
    PFN_DrvSetPointerShape         SetPointerShape;
    PFN_DrvMovePointer             MovePointer;
    PFN_DrvLineTo                  LineTo;
    PFN_DrvSendPage                SendPage;
    PFN_DrvStartPage               StartPage;
    PFN_DrvEndDoc                  EndDoc;
    PFN_DrvStartDoc                StartDoc;
    PVOID                          Unknown3;
    PFN_DrvGetGlyphMode            GetGlyphMode;
    PFN_DrvSynchronize             Synchronize;
    PVOID                          Unknown4;
    PFN_DrvSaveScreenBits          SaveScreenBits;
    PFN_DrvGetModes                GetModes;
    PFN_DrvFree                    Free;
    PFN_DrvDestroyFont             DestroyFont;
    PFN_DrvQueryFontCaps           QueryFontCaps;
    PFN_DrvLoadFontFile            LoadFontFile;
    PFN_DrvUnloadFontFile          UnloadFontFile;
    PFN_DrvFontManagement          FontManagement;
    PFN_DrvQueryTrueTypeTable      QueryTrueTypeTable;
    PFN_DrvQueryTrueTypeOutline    QueryTrueTypeOutline;
    PFN_DrvGetTrueTypeFile         GetTrueTypeFile;
    PFN_DrvQueryFontFile           QueryFontFile;
    PFN_DrvMovePanning             MovePanning;
    PFN_DrvQueryAdvanceWidths      QueryAdvanceWidths;
    PFN_DrvSetPixelFormat          SetPixelFormat;
    PFN_DrvDescribePixelFormat     DescribePixelFormat;
    PFN_DrvSwapBuffers             SwapBuffers;
    PFN_DrvStartBanding            StartBanding;
    PFN_DrvNextBand                NextBand;
    PFN_DrvGetDirectDrawInfo       GetDirectDrawInfo;
    PFN_DrvEnableDirectDraw        EnableDirectDraw;
    PFN_DrvDisableDirectDraw       DisableDirectDraw;
    PFN_DrvQuerySpoolType          QuerySpoolType;
    PVOID                          Unknown5;
    PFN_DrvIcmCreateColorTransform IcmCreateColorTransform;
    PFN_DrvIcmDeleteColorTransform IcmDeleteColorTransform;
    PFN_DrvIcmCheckBitmapBits      IcmCheckBitmapBits;
    PFN_DrvIcmSetDeviceGammaRamp   IcmSetDeviceGammaRamp;
    PFN_DrvGradientFill            GradientFill;
    PFN_DrvStretchBltROP           StretchBltROP;
    PFN_DrvPlgBlt                  PlgBlt;
    PFN_DrvAlphaBlend              AlphaBlend;
    PVOID                          SynthesizeFont;
    PVOID                          GetSynthesizedFontFiles;
    PFN_DrvTransparentBlt          TransparentBlt;
    PFN_DrvQueryPerBandInfo        QueryPerBandInfo;
    PFN_DrvQueryDeviceSupport      QueryDeviceSupport;
    PVOID                          Reserved1;
    PVOID                          Reserved2;
    PVOID                          Reserved3;
    PVOID                          Reserved4;
    PVOID                          Reserved5;
    PVOID                          Reserved6;
    PVOID                          Reserved7;
    PVOID                          Reserved8;
    PFN_DrvDeriveSurface           DeriveSurface;
    PFN_DrvQueryGlyphAttrs         QueryGlyphAttrs;
    PFN_DrvNotify                  Notify;
    PFN_DrvSynchronizeSurface      SynchronizeSurface;
    PFN_DrvResetDevice             ResetDevice;
    PVOID                          Reserved9;
    PVOID                          Reserved10;
    PVOID                          Reserved11; /* 92 */

    /* ReactOS specify */
    PFN_DrvEnableDriver            EnableDriver; //ReactOS Extra
} DRIVER_FUNCTIONS, *PDRIVER_FUNCTIONS;

#define ASSERT_PFN(pfn) \
 C_ASSERT(FIELD_OFFSET(DRIVER_FUNCTIONS, pfn) == sizeof(PVOID) * INDEX_Drv##pfn)

ASSERT_PFN(EnablePDEV);
ASSERT_PFN(CompletePDEV);
ASSERT_PFN(DisablePDEV);
ASSERT_PFN(EnableSurface);
ASSERT_PFN(DisableSurface);
ASSERT_PFN(AssertMode);
ASSERT_PFN(Offset);
ASSERT_PFN(ResetPDEV);
ASSERT_PFN(DisableDriver);
ASSERT_PFN(CreateDeviceBitmap);
ASSERT_PFN(DeleteDeviceBitmap);
ASSERT_PFN(RealizeBrush);
ASSERT_PFN(DitherColor);
ASSERT_PFN(StrokePath);
ASSERT_PFN(FillPath);
ASSERT_PFN(StrokeAndFillPath);
ASSERT_PFN(Paint);
ASSERT_PFN(BitBlt);
ASSERT_PFN(CopyBits);
ASSERT_PFN(StretchBlt);
ASSERT_PFN(SetPalette);
ASSERT_PFN(TextOut);
ASSERT_PFN(Escape);
ASSERT_PFN(DrawEscape);
ASSERT_PFN(QueryFont);
ASSERT_PFN(QueryFontTree);
ASSERT_PFN(QueryFontData);
ASSERT_PFN(SetPointerShape);
ASSERT_PFN(MovePointer);
ASSERT_PFN(LineTo);
ASSERT_PFN(SendPage);
ASSERT_PFN(StartPage);
ASSERT_PFN(EndDoc);
ASSERT_PFN(StartDoc);
ASSERT_PFN(GetGlyphMode);
ASSERT_PFN(Synchronize);
ASSERT_PFN(SaveScreenBits);
ASSERT_PFN(GetModes);
ASSERT_PFN(Free);
ASSERT_PFN(DestroyFont);
ASSERT_PFN(QueryFontCaps);
ASSERT_PFN(LoadFontFile);
ASSERT_PFN(UnloadFontFile);
ASSERT_PFN(FontManagement);
ASSERT_PFN(QueryTrueTypeTable);
ASSERT_PFN(QueryTrueTypeOutline);
ASSERT_PFN(GetTrueTypeFile);
ASSERT_PFN(QueryFontFile);
ASSERT_PFN(MovePanning);
ASSERT_PFN(QueryAdvanceWidths);
ASSERT_PFN(SetPixelFormat);
ASSERT_PFN(DescribePixelFormat);
ASSERT_PFN(SwapBuffers);
ASSERT_PFN(StartBanding);
ASSERT_PFN(NextBand);
ASSERT_PFN(GetDirectDrawInfo);
ASSERT_PFN(EnableDirectDraw);
ASSERT_PFN(DisableDirectDraw);
ASSERT_PFN(QuerySpoolType);
ASSERT_PFN(IcmCreateColorTransform);
ASSERT_PFN(IcmDeleteColorTransform);
ASSERT_PFN(IcmCheckBitmapBits);
ASSERT_PFN(IcmSetDeviceGammaRamp);
ASSERT_PFN(GradientFill);
ASSERT_PFN(StretchBltROP);
ASSERT_PFN(PlgBlt);
ASSERT_PFN(AlphaBlend);
ASSERT_PFN(SynthesizeFont);
ASSERT_PFN(GetSynthesizedFontFiles);
ASSERT_PFN(TransparentBlt);
ASSERT_PFN(QueryPerBandInfo);
ASSERT_PFN(QueryDeviceSupport);
ASSERT_PFN(Reserved1);
ASSERT_PFN(Reserved2);
ASSERT_PFN(Reserved3);
ASSERT_PFN(Reserved4);
ASSERT_PFN(Reserved5);
ASSERT_PFN(Reserved6);
ASSERT_PFN(Reserved7);
ASSERT_PFN(Reserved8);
ASSERT_PFN(DeriveSurface);
ASSERT_PFN(QueryGlyphAttrs);
ASSERT_PFN(Notify);
ASSERT_PFN(SynchronizeSurface);
ASSERT_PFN(ResetDevice);

#endif
