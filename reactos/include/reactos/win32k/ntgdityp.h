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
    GdtGetSetCopyCount = 2,
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

#define GdiWorldSpaceToPageSpace    0x203

/* FIXME: Unknown */
typedef DWORD FULLSCREENCONTROL;
typedef DWORD LFTYPE;

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


/* TYPES *********************************************************************/

typedef PVOID KERNEL_PVOID;
typedef DWORD UNIVERSAL_FONT_ID;
typedef UNIVERSAL_FONT_ID *PUNIVERSAL_FONT_ID;
typedef DWORD CHWIDTHINFO;
typedef CHWIDTHINFO *PCHWIDTHINFO;
typedef D3DNTHAL_CONTEXTCREATEDATA D3DNTHAL_CONTEXTCREATEI;
typedef LONG FIX;

/* FIXME: Unknown; easy to guess, usually based on public types and converted */
typedef struct _WIDTHDATA WIDTHDATA, *PWIDTHDATA;
typedef struct _DEVCAPS DEVCAPS, *PDEVCAPS;
typedef struct _REALIZATION_INFO REALIZATION_INFO, *PREALIZATION_INFO;

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

/* Gdi XForm storage union */
typedef union
{
  FLOAT f;
  ULONG l;
} gxf_long;

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
// Most of these are definded in ddk/winddi.h
//
typedef struct _DRIVER_FUNCTIONS
{
  PGD_ENABLEDRIVER               EnableDriver; //ReactOS Extra
  PGD_ENABLEPDEV                 EnablePDEV;
  PGD_COMPLETEPDEV               CompletePDEV;
  PGD_DISABLEPDEV                DisablePDEV;
  PGD_ENABLESURFACE              EnableSurface;
  PGD_DISABLESURFACE             DisableSurface;
  PGD_ASSERTMODE                 AssertMode;
  PGD_OFFSET                     Offset;
  PGD_RESETPDEV                  ResetPDEV;
  PGD_DISABLEDRIVER              DisableDriver;
  PVOID                          Unknown0;
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
  PVOID                          Unknown1;
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
  PVOID                          Unknown2;
  PGD_GETGLYPHMODE               GetGlyphMode;
  PGD_SYNCHRONIZE                Synchronize;
  PVOID                          Unknown3;
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
  PVOID                          Unknown4;
  PFN_DrvIcmCreateColorTransform IcmCreateColorTransform;
  PFN_DrvIcmDeleteColorTransform IcmDeleteColorTransform;
  PFN_DrvIcmCheckBitmapBits      IcmCheckBitmapBits;
  PFN_DrvIcmSetDeviceGammaRamp   IcmSetDeviceGammaRamp;
  PGD_GRADIENTFILL               GradientFill;
  PGD_STRETCHBLTROP              StretchBltROP;
  PFN_DrvPlgBlt                  PlgBlt;
  PGD_ALPHABLEND                 AlphaBlend;
//  PFN_DrvSynthesizeFont          SynthesizeFont;
  PVOID                          Unknown5;
//  PFN_DrvGetSynthesizedFontFiles GetSynthesizedFontFiles;
  PVOID                          Unknown6;
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
  PVOID                          Reserved11;
} DRIVER_FUNCTIONS, *PDRIVER_FUNCTIONS;

#endif
