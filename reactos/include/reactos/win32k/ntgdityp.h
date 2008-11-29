/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/ntgdityp.h
 * PURPOSE:         Win32 Shared GDI Types for NtGdi*
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTGDITYP_
#define _NTGDITYP_

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
    GdiGerCharBreak,
    GdiGetArcDirection,
    GdiGetEMFRestorDc,
    GdiGetFontLanguageInfo,
    GdiGetIsMemDc,
    GdiGetMapMode,
    GdiGetTextCharExtra,
} GETDCDWORD, *PGETDCDWORD;

typedef enum _GETSETDCDWORD
{
    GdiGetSetCopyCount = 2,
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
    MX_SCALE = 1,
    MX_IDENTITYSCALE = 2,
    MX_INTEGER = 4,
    MX_NOTRANSLATE = 8,
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
    GDILoObjType_LO_ALTDC_TYPE = 0x210000,
    GDILoObjType_LO_PEN_TYPE = 0x300000,
    GDILoObjType_LO_EXTPEN_TYPE = 0x500000,
    GDILoObjType_LO_DIBSECTION_TYPE = 0x250000,
    GDILoObjType_LO_METAFILE16_TYPE = 0x260000,
    GDILoObjType_LO_METAFILE_TYPE = 0x460000,
    GDILoObjType_LO_METADC16_TYPE = 0x660000
} GDILOOBJTYPE, *PGDILOOBJTYPE;

#define GdiWorldSpaceToPageSpace    0x203

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

/* TYPES *********************************************************************/

typedef PVOID KERNEL_PVOID;
typedef PVOID PUMDHPDEV;
typedef D3DNTHAL_CONTEXTCREATEDATA D3DNTHAL_CONTEXTCREATEI;
#if !defined(__WINDDI_H)
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

//
// GDI Batch structures.
//
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

#ifndef _NTUSRTYP_
typedef struct _PATRECT
{
  RECT r;
  HBRUSH hBrush;
} PATRECT, * PPATRECT;

typedef struct _W32CLIENTINFO
{
    ULONG CI_flags;
    ULONG cSpins;
    ULONG ulWindowsVersion;
    ULONG ulAppCompatFlags;
    ULONG ulAppCompatFlags2;
    ULONG W32ClientInfo[57];
} W32CLIENTINFO, *PW32CLIENTINFO;
#endif

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

typedef struct _GDIBSTEXTOUT
{
  GDIBATCHHDR gbHdr;
  COLORREF crForegroundClr;
  COLORREF crBackgroundClr;
  LONG lmBkMode;
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
  WCHAR String[2];
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
  LONG right;
  LONG bottom;
  LONG left;
  LONG top;
} GDIBSEXTSELCLPRGN, *PGDIBSEXTSELCLPRGN;
//
//   Use with GdiBCSelObj, GdiBCDelObj and GdiBCDelRgn.
typedef struct _GDIBSOBJECT
{
  GDIBATCHHDR gbHdr;
  HGDIOBJ hgdiobj;
} GDIBSOBJECT, *PGDIBSOBJECT;

//
//  Driver Functions
//
typedef BOOL (NTAPI *PGD_ENABLEDRIVER)(ULONG, ULONG, PDRVENABLEDATA);
typedef DHPDEV (NTAPI *PGD_ENABLEPDEV)(DEVMODEW  *, LPWSTR, ULONG, HSURF *, ULONG, ULONG  *, ULONG, DEVINFO  *, LPWSTR, LPWSTR, HANDLE);
typedef VOID (NTAPI *PGD_COMPLETEPDEV)(DHPDEV, HDEV);
typedef VOID (NTAPI *PGD_DISABLEPDEV)(DHPDEV);
typedef HSURF (NTAPI *PGD_ENABLESURFACE)(DHPDEV);
typedef VOID (NTAPI *PGD_DISABLESURFACE)(DHPDEV);
typedef BOOL (NTAPI *PGD_ASSERTMODE)(DHPDEV, BOOL);
typedef BOOL (NTAPI *PGD_OFFSET)(SURFOBJ*, LONG, LONG, FLONG);
typedef BOOL (NTAPI *PGD_RESETPDEV)(DHPDEV, DHPDEV);
typedef VOID (NTAPI *PGD_DISABLEDRIVER)(VOID);
typedef HBITMAP (NTAPI *PGD_CREATEDEVICEBITMAP)(DHPDEV, SIZEL, ULONG);
typedef VOID (NTAPI *PGD_DELETEDEVICEBITMAP)(DHSURF);
typedef BOOL (NTAPI *PGD_ALPHABLEND)(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, RECTL*, BLENDOBJ*);
typedef BOOL (NTAPI *PGD_REALIZEBRUSH)(BRUSHOBJ*, SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*, ULONG);
typedef ULONG (NTAPI *PGD_DITHERCOLOR)(DHPDEV, ULONG, ULONG, PULONG);
typedef BOOL (NTAPI *PGD_STROKEPATH)(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*, POINTL*, PLINEATTRS, MIX);
typedef BOOL (NTAPI *PGD_FILLPATH)(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*, MIX, ULONG);
typedef BOOL (NTAPI *PGD_STROKEANDFILLPATH)(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*, PLINEATTRS, BRUSHOBJ*, POINTL*, MIX, ULONG);
typedef BOOL (NTAPI *PGD_PAINT)(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*, MIX);
typedef BOOL (NTAPI *PGD_BITBLT)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
typedef BOOL (NTAPI *PGD_TRANSPARENTBLT)(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, RECTL*, ULONG, ULONG);
typedef BOOL (NTAPI *PGD_COPYBITS)(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*);
typedef BOOL (NTAPI *PGD_STRETCHBLT)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, COLORADJUSTMENT*, POINTL*, RECTL*, RECTL*, POINTL*, ULONG);
typedef BOOL (NTAPI *PGD_STRETCHBLTROP)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, COLORADJUSTMENT*, POINTL*, RECTL*, RECTL*, POINTL*, ULONG, BRUSHOBJ*, DWORD);
typedef BOOL (NTAPI *PGD_SETPALETTE)(DHPDEV, PALOBJ*, ULONG, ULONG, ULONG);
typedef BOOL (NTAPI *PGD_TEXTOUT)(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*, RECTL*, BRUSHOBJ*, BRUSHOBJ*, POINTL*, MIX);
typedef ULONG (NTAPI *PGD_ESCAPE)(SURFOBJ*, ULONG, ULONG, PVOID *, ULONG, PVOID *);
typedef ULONG (NTAPI *PGD_DRAWESCAPE)(SURFOBJ*, ULONG, CLIPOBJ*, RECTL*, ULONG, PVOID *);
typedef PIFIMETRICS (NTAPI *PGD_QUERYFONT)(DHPDEV, ULONG, ULONG, PULONG);
typedef PVOID (NTAPI *PGD_QUERYFONTTREE)(DHPDEV, ULONG, ULONG, ULONG, PULONG);
typedef LONG (NTAPI *PGD_QUERYFONTDATA)(DHPDEV, FONTOBJ*, ULONG, HGLYPH, GLYPHDATA*, PVOID, ULONG);
typedef ULONG (NTAPI *PGD_SETPOINTERSHAPE)(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*, LONG, LONG, LONG, LONG, RECTL*, ULONG);
typedef VOID (NTAPI *PGD_MOVEPOINTER)(SURFOBJ*, LONG, LONG, RECTL*);
typedef BOOL (NTAPI *PGD_LINETO)(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, LONG, LONG, LONG, LONG, RECTL*, MIX);
typedef BOOL (NTAPI *PGD_SENDPAGE)(SURFOBJ*);
typedef BOOL (NTAPI *PGD_STARTPAGE)(SURFOBJ*);
typedef BOOL (NTAPI *PGD_ENDDOC)(SURFOBJ*, ULONG);
typedef BOOL (NTAPI *PGD_STARTDOC)(SURFOBJ*, PWSTR, DWORD);
typedef ULONG (NTAPI *PGD_GETGLYPHMODE)(DHPDEV, FONTOBJ*);
typedef VOID (NTAPI *PGD_SYNCHRONIZE)(DHPDEV, RECTL*);
typedef ULONG (NTAPI *PGD_SAVESCREENBITS)(SURFOBJ*, ULONG, ULONG, RECTL*);
typedef ULONG (NTAPI *PGD_GETMODES)(HANDLE, ULONG, PDEVMODEW);
typedef VOID (NTAPI *PGD_FREE)(PVOID, ULONG);
typedef VOID (NTAPI *PGD_DESTROYFONT)(FONTOBJ*);
typedef LONG (NTAPI *PGD_QUERYFONTCAPS)(ULONG, PULONG);
typedef ULONG (NTAPI *PGD_LOADFONTFILE)(ULONG, PVOID, ULONG, ULONG);
typedef BOOL (NTAPI *PGD_UNLOADFONTFILE)(ULONG);
typedef ULONG (NTAPI *PGD_FONTMANAGEMENT)(SURFOBJ*, FONTOBJ*, ULONG, ULONG, PVOID, ULONG, PVOID);
typedef LONG (NTAPI *PGD_QUERYTRUETYPETABLE)(ULONG, ULONG, ULONG, PTRDIFF, ULONG, PBYTE);
typedef LONG (NTAPI *PGD_QUERYTRUETYPEOUTLINE)(DHPDEV, FONTOBJ*, HGLYPH, BOOL, GLYPHDATA*, ULONG, TTPOLYGONHEADER*);
typedef PVOID (NTAPI *PGD_GETTRUETYPEFILE)(ULONG, PULONG);
typedef LONG (NTAPI *PGD_QUERYFONTFILE)(ULONG, ULONG, ULONG, PULONG);
typedef BOOL (NTAPI *PGD_QUERYADVANCEWIDTHS)(DHPDEV, FONTOBJ*, ULONG, HGLYPH *, PVOID *, ULONG);
typedef BOOL (NTAPI *PGD_SETPIXELFORMAT)(SURFOBJ*, LONG, ULONG);
typedef LONG (NTAPI *PGD_DESCRIBEPIXELFORMAT)(DHPDEV, LONG, ULONG, PPIXELFORMATDESCRIPTOR);
typedef BOOL (NTAPI *PGD_SWAPBUFFERS)(SURFOBJ*, PWNDOBJ);
typedef BOOL (NTAPI *PGD_STARTBANDING)(SURFOBJ*, POINTL*);
typedef BOOL (NTAPI *PGD_NEXTBAND)(SURFOBJ*, POINTL*);
typedef BOOL (NTAPI *PGD_GETDIRECTDRAWINFO)(DHPDEV, PDD_HALINFO, PDWORD, VIDEOMEMORY*, PDWORD, PDWORD);
typedef BOOL (NTAPI *PGD_ENABLEDIRECTDRAW)(DHPDEV, PDD_CALLBACKS, PDD_SURFACECALLBACKS, PDD_PALETTECALLBACKS);
typedef VOID (NTAPI *PGD_DISABLEDIRECTDRAW)(DHPDEV);
typedef LONG (NTAPI *PGD_QUERYSPOOLTYPE)(DHPDEV, LPWSTR);
typedef BOOL (NTAPI *PGD_GRADIENTFILL)(SURFOBJ*, CLIPOBJ*, XLATEOBJ*, TRIVERTEX*, ULONG, PVOID, ULONG, RECTL*, POINTL*, ULONG);
typedef VOID (NTAPI *PGD_SYNCHRONIZESURFACE)(SURFOBJ*, RECTL *, FLONG);

//
// Declarations missing in ddk/winddi.h
//
typedef VOID (APIENTRY *PFN_DrvMovePanning)(LONG, LONG, FLONG);


//
// Most of these are defined in ddk/winddi.h
//
typedef struct _DRIVER_FUNCTIONS
{
    PGD_ENABLEPDEV                 EnablePDEV;    
    PGD_COMPLETEPDEV               CompletePDEV;
    PGD_DISABLEPDEV                DisablePDEV;
    PGD_ENABLESURFACE              EnableSurface;
    PGD_DISABLESURFACE             DisableSurface;
    PGD_ASSERTMODE                 AssertMode;
    PGD_OFFSET                     Offset;
    PGD_RESETPDEV                  ResetPDEV;
    PGD_DISABLEDRIVER              DisableDriver;
    PVOID                          Unknown1;
    PGD_CREATEDEVICEBITMAP         CreateDeviceBitmap;
    PGD_DELETEDEVICEBITMAP         DeleteDeviceBitmap;
    PGD_REALIZEBRUSH               RealizeBrush;
    PGD_DITHERCOLOR                DitherColor;
    PGD_STROKEPATH                 StrokePath;
    PGD_FILLPATH                   FillPath;
    PGD_STROKEANDFILLPATH          StrokeAndFillPath;
    PGD_PAINT                      Paint;
    PGD_BITBLT                     BitBlt;
    PGD_COPYBITS                   CopyBits;
    PGD_STRETCHBLT                 StretchBlt;
    PVOID                          Unknown2;
    PGD_SETPALETTE                 SetPalette;
    PGD_TEXTOUT                    TextOut;
    PGD_ESCAPE                     Escape;
    PGD_DRAWESCAPE                 DrawEscape;
    PGD_QUERYFONT                  QueryFont;
    PGD_QUERYFONTTREE              QueryFontTree;
    PGD_QUERYFONTDATA              QueryFontData;
    PGD_SETPOINTERSHAPE            SetPointerShape;
    PGD_MOVEPOINTER                MovePointer;
    PGD_LINETO                     LineTo;
    PGD_SENDPAGE                   SendPage;
    PGD_STARTPAGE                  StartPage;
    PGD_ENDDOC                     EndDoc;
    PGD_STARTDOC                   StartDoc;
    PVOID                          Unknown3; 
    PGD_GETGLYPHMODE               GetGlyphMode;
    PGD_SYNCHRONIZE                Synchronize;
    PVOID                          Unknown4; 
    PGD_SAVESCREENBITS             SaveScreenBits;
    PGD_GETMODES                   GetModes;
    PGD_FREE                       Free;
    PGD_DESTROYFONT                DestroyFont;
    PGD_QUERYFONTCAPS              QueryFontCaps;
    PGD_LOADFONTFILE               LoadFontFile;
    PGD_UNLOADFONTFILE             UnloadFontFile;
    PGD_FONTMANAGEMENT             FontManagement;
    PGD_QUERYTRUETYPETABLE         QueryTrueTypeTable;
    PGD_QUERYTRUETYPEOUTLINE       QueryTrueTypeOutline;
    PGD_GETTRUETYPEFILE            GetTrueTypeFile;
    PGD_QUERYFONTFILE              QueryFontFile;
    PFN_DrvMovePanning             MovePanning;
    PGD_QUERYADVANCEWIDTHS         QueryAdvanceWidths;
    PGD_SETPIXELFORMAT             SetPixelFormat;
    PGD_DESCRIBEPIXELFORMAT        DescribePixelFormat;
    PGD_SWAPBUFFERS                SwapBuffers;
    PGD_STARTBANDING               StartBanding;
    PGD_NEXTBAND                   NextBand;
    PGD_GETDIRECTDRAWINFO          GetDirectDrawInfo;
    PGD_ENABLEDIRECTDRAW           EnableDirectDraw;
    PGD_DISABLEDIRECTDRAW          DisableDirectDraw;
    PGD_QUERYSPOOLTYPE             QuerySpoolType;
    PVOID                          Unknown5;
    PFN_DrvIcmCreateColorTransform IcmCreateColorTransform;
    PFN_DrvIcmDeleteColorTransform IcmDeleteColorTransform;
    PFN_DrvIcmCheckBitmapBits      IcmCheckBitmapBits;
    PFN_DrvIcmSetDeviceGammaRamp   IcmSetDeviceGammaRamp;
    PGD_GRADIENTFILL               GradientFill;
    PGD_STRETCHBLTROP              StretchBltROP;
    PFN_DrvPlgBlt                  PlgBlt;
    PGD_ALPHABLEND                 AlphaBlend;
    PVOID                          SynthesizeFont;
    PVOID                          GetSynthesizedFontFiles;
    PGD_TRANSPARENTBLT             TransparentBlt;
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
    PGD_SYNCHRONIZESURFACE         SynchronizeSurface;
    PFN_DrvResetDevice             ResetDevice;
    PVOID                          Reserved9;
    PVOID                          Reserved10;
    PVOID                          Reserved11; /* 92 */
    
    /* ReactOS specify */
    PGD_ENABLEDRIVER               EnableDriver; //ReactOS Extra
} DRIVER_FUNCTIONS, *PDRIVER_FUNCTIONS;

#endif
