/*
 * WinDDI.h - definition of the GDI - DDI interface
 */

#include <windows.h>

typedef HANDLE  HDEV;

typedef HANDLE  DHPDEV;

typedef ULONG  (*PFN)();

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
} DEVINFO, *PDEVINFO;

#define  DMMAXDEVICENAME  32
#define  DMMAXFORMNAME  32

typedef struct _DEVMODEW 
{
  WCHAR  dmDeviceName[DMMAXDEVICENAME];
  WORD  dmSpecVersion;
  WORD  dmDriverVersion;
  WORD  dmSize;
  WORD  dmDriverExtra;
  DWORD  dmFields;
  short  dmOrientation;
  short  dmPaperSize;
  short  dmPaperLength;
  short  dmPaperWidth;
  short  dmScale;
  short  dmCopies;
  short  dmDefaultSource;
  short  dmPrintQuality;
  short  dmColor;
  short  dmDuplex;
  short  dmYResolution;
  short  dmTTOption;
  short  dmCollate;
  WCHAR  dmFormName[DMMAXFORMNAME];
  WORD  dmLogPixels;
  DWORD  dmBitsPerPel;
  DWORD  dmPelsWidth;
  DWORD  dmPelsHeight;
  DWORD  dmDisplayFlags;
  DWORD  dmDisplayFrequency;
} DEVMODEW;

typedef struct _BRUSHOBJ 
{
  ULONG  iSolidColor;
  PVOID  pvRbrush;
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

typedef struct _XLATEOBJ 
{
  ULONG  iUniq;
  FLONG  flXlate;
  USHORT  iSrcType;
  USHORT  iDstType;
  ULONG  cEntries;
  ULONG  *pulXlate;
} XLATEOBJ, *PXLATEOBJ;

/*  Options for CLIPOBJ_cEnumStart Type field  */
#define  CT_RECTANGLE  1

/*  Options for CLIPOBJ_cEnumStart BuildOrder field  */
enum _CD_ORDERS
{
  CD_RIGHTDOWN = 1,
  CD_LEFTDOWN,
  CD_RIGHTUP,
  CD_LEFTUP,
  CD_ANY
};

/*  EngAssocateSurface hook flags  */
#define  HOOK_BITBLT             0x00000001
#define  HOOK_STRETCHBLT         0x00000002
#define  HOOK_TEXTOUT            0x00000004
#define  HOOK_PAINT              0x00000008
#define  HOOK_STROKEPATH         0x00000010
#define  HOOK_FILLPATH           0x00000020
#define  HOOK_STROKEANDFILLPATH  0x00000040
#define  HOOK_LINETO             0x00000080
#define  HOOK_COPYBITS           0x00000100
#define  HOOK_SYNCHRONIZE        0x00000200
#define  HOOK_SYNCHRONIZEACCESS  0x00000400

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
  BMF_8RLE
};

#define  RASTER_FONTTYPE    0x00000001
#define  DEVICE_FONTTYPE    0x00000002
#define  TRUETYPE_FONTTYPE  0x00000004
#define  FO_SIM_ITALIC      0x00000008
#define  FO_SIM_BOLD        0x00000010
#define  FO_EM_HEIGHT       0x00000020
#define  FO_4_LEVEL         0x00000040
#define  FO_16_LEVEL        0x00000080
#define  FO_64_LEVEL        0x00000100

enum _SURF_TYPES
{
  STYPE_BITMAP = 1,
  STYPE_DEVICE, 
  STYPE_DEVBITMAP
};

#define  BMF_TOPDOWN     0x00000001
#define  BMF_NOZEROINIT  0x00000002
#define  BMF_USERMEM     0x00000004

/*  EngCreatePalette mode types  */
enum _PAL_TYPES
{
  PAL_INDEXED = 1,
  PAL_BITFIELDS,
  PAL_RGB,
  PAL_BGR
};

enum _DITHER_COLOR_MODES
{
  DM_DEFAULT,
  DM_MONOCHROME
};

enum _ESCAPE_CODES
{
  ESC_PASSTHROUGH,
  ESC_QUERYESCSUPPORT 
};

enum _END_DOC_FLAGS
{
  ED_ABORTDOC
};

#define  FP_WINDINGMODE    0x00000001
#define  FP_ALTERNATEMODE  0x00000002

enum _GLYPH_MODE
{
  FO_GLYPHBITS = 1,
  FO_HGLYPHS,
  FO_PATHOBJ
};
 
enum _QUERY_ADVANCE_WIDTH_TYPES
{
  QAW_GETWIDTHS = 1,
  QAW_GETEASYWIDTHS 
};

enum _WIN_CHARSET
{
  ANSI_CHARSET = 1,
  SYMBOL_CHARSET, 
  SHIFTJIS_CHARSET, 
  HANGEUL_CHARSET, 
  CHINESEBIG5_CHARSET, 
  OEM_CHARSET 
};

#define  FIXED_PITCH     0x00000000
#define  VARIABLE_PITCH  0x00000001
#define  FF_DECORATIVE   0x00000010
#define  FF_DONTCARE     0x00000020
#define  FF_MODERN       0x00000030
#define  FF_ROMAN        0x00000040
#define  FF_SCRIPT       0x00000050
#define  FF_SWISS        0x00000060

#define  FM_INFO_TECH_TRUETYPE  0x00000001
#define  FM_INFO_TECH_BITMAP    0x00000002
#define  FM_INFO_TECH_STROKE    0x00000004
#define  FM_INFO_TECH_OUTLINE_NOT_TRUETYPE  0x00000008
#define  FM_INFO_ARB_XFORMS     0x00000010
#define  FM_INFO_1BBP           0x00000020
#define  FM_INFO_INTEGER_WIDTH  0x00000040
#define  FM_INFO_CONSTANT_WIDTH 0x00000080
#define  FM_INFO_NOT_CONTIGUOUS 0x00000100
#define  FM_INFO_PID_EMBEDDED 0x00000200
#define  FM_INFO_RETURNS_OUTLINES 0x00000400
#define  FM_INFO_RETURNS_STROKES 0x00000800
#define  FM_INFO_RETURNS_BITMAPS 0x00001000
#define  FM_INFO_UNICODE_COMPLIANT 0x00002000
#define  FM_INFO_RIGHT_HANDED 0x00004000
#define  FM_INFO_INTEGRAL_SCALING 0x00008000
#define  FM_INFO_90_DEGREE_ROTATIONS 0x00010000
#define  FM_INFO_OPTICALLY_FIXED_PITCH    0x00020000
#define  FM_INFO_DO_NOT_ENUMERATE         0x00040000
#define  FM_INFO_ISOTROPIC_SCALING_ONLY   0x00080000
#define  FM_INFO_ANISOTROPIC_SCALING_ONLY 0x00100000
#define  FM_INFO_TID_EMBEDDED             0x00200000
#define  FM_INFO_FAMILY_EQUIV             0x00400000
#define  FM_INFO_DBCS_FIXED_PITCH         0x00800000
#define  FM_INFO_NONNEGATIVE_AC           0x01000000
#define  FM_INFO_IGNORE_TC_RA_ABLE        0x02000000

#define  FM_SEL_ITALIC 0x00000001
#define  FM_SEL_UNDERSCORE 0x00000002
#define  FM_SEL_NEGATIVE 0x00000004
#define  FM_SEL_OUTLINED 0x00000008
#define  FM_SEL_STRIKEOUT 0x00000010
#define  FM_SEL_BOLD 0x00000020
#define  FM_SEL_REGULAR 0x00000040

#define  FM_TYPE_LICENSED 0x00000001
#define  FM_READONLY_EMBED 0x00000002
#define  FM_EDITABLE_EMBED 0x00000004

/*
 * Functions Prefixed with Drv are calls made from GDI to DDI, and
 * everything else are calls made from DDI to GDI.  DDI is
 * not allowed to make calls to any other kernel or user modules.
 */

/*  DDI --> GDI calls  */
PVOID BRUSHOBJ_pvAllocRbrush(IN PBRUSHOBJ  BrushObj, 
                             IN ULONG  ObjSize); 
PVOID BRUSHOBJ_pvGetRbrush(IN PBRUSHOBJ  BrushObj); 
BOOL CLIPOBJ_bEnum(IN PCLIPOBJ  ClipObj, 
                   IN ULONG  ObjSize, 
                   OUT ULONG  *EnumRects); 
ULONG CLIPOBJ_cEnumStart(IN PCLIPOBJ  ClipObj, 
                         IN BOOL  ShouldDoAll, 
                         IN ULONG  ClipType, 
                         IN ULONG  BuildOrder, 
                         IN ULONG  MaxRects); 
PPATHOBJ CLIPOBJ_ppoGetPath(PCLIPOBJ ClipObj);

/*  GDI --> DDI calls  */
VOID DrvAssertMode(IN DHPDEV  PDev, 
                   IN BOOL  ShouldEnable);
BOOL DrvBitBlt(IN PSURFOBJ  DestSurface, 
               IN PSURFOBJ  SrcSurface, 
               IN PSURFOBJ  MaskSurface, 
               IN PCLIPOBJ  ClipObj, 
               IN PXLATEOBJ  XLateObj, 
               IN PRECTL  DestRectL, 
               IN PPOINTL  SrcPointL, 
               IN PPOINTL  MaskPointL, 
               IN PBRUSHOBJ  BrushObj, 
               IN PPOINTL  BrushPointL, 
               IN ROP4  RasterOp); 
VOID DrvCompletePDEV(IN DHPDEV  PDev,
                     IN HDEV  Dev);
BOOL DrvCopyBits(OUT PSURFOBJ  DestSurface, 
                 IN PSURFOBJ  SrcSurface, 
                 IN PCLIPOBJ  ClipObj, 
                 IN PXLATEOBJ  XLateObj, 
                 IN PRECTL  DestRectL, 
                 IN PPOINTL  SrcPointL); 
HBITMAP DrvCreateDeviceBitmap(IN DHPDEV  DPev, 
                              IN SIZEL  SizeL, 
                              IN ULONG  Format); 
VOID DrvDeleteDeviceBitmap(IN DHSURF  Surface); 
LONG DrvDescribePixelFormat(IN DHPDEV  DPev, 
                            IN LONG  PixelFormat, 
                            IN ULONG  DescriptorSize, 
                            OUT PPIXELFORMATDESCRIPTOR  PFD); 
VOID DrvDestroyFont(IN PFONTOBJ FontObj); 
VOID DrvDisableDriver(VOID);
VOID DrvDisablePDEV(IN DHPDEV PDev); 
VOID DrvDisableSurface(IN DHPDEV PDev); 
ULONG DrvDitherColor(IN DHPDEV  DPev, 
                     IN ULONG  Mode, 
                     IN ULONG  RGB, 
                     OUT PULONG  DitherBits); 
ULONG DrvDrawEscape(IN PSURFOBJ  SurfObj, 
                    IN ULONG  EscCode, 
                    IN PCLIPOBJ  ClipObj, 
                    IN PRECTL  RectL, 
                    IN ULONG  InputSize, 
                    IN PVOID  *InputData); 
BOOL DrvEnableDriver(IN ULONG Version, 
                     IN ULONG DEDSize, 
                     OUT PDRVENABLEDATA DED);
DHPDEV DrvEnablePDEV(IN DEVMODEW  *DM,
                     IN LPWSTR  LogAddress,
                     IN ULONG  PatternCount,
                     OUT HSURF  *SurfPatterns,
                     IN ULONG  CapsSize,
                     OUT ULONG  *DevCaps,
                     IN ULONG  DevInfoSize,
                     OUT DEVINFO  *DI,
                     IN LPWSTR  DevDataFile,
                     IN LPWSTR  DeviceName,
                     IN HANDLE  Driver);
HSURF DrvEnableSurface(IN DHPDEV  PDev);
BOOL DrvEndDoc(IN PSURFOBJ  SurfObj, 
               IN ULONG  Flags); 
ULONG DrvEscape(IN PSURFOBJ  SurfObj, 
                IN ULONG  EscCode, 
                IN ULONG  InputSize, 
                IN PVOID  *InputData, 
                IN ULONG  OutputSize, 
                OUT PVOID  *OutputData); 
BOOL DrvFillPath(IN PSURFOBJ  SurfObj, 
                 IN PPATHOBJ  PathObj, 
                 IN PCLIPOBJ  ClipObj, 
                 IN PBRUSHOBJ  BrushObj, 
                 IN PPOINTL  BrushOrg, 
                 IN MIX  Mix, 
                 IN ULONG  Options); 
ULONG DrvFontManagement(IN PSURFOBJ  SurfObj, 
                        IN PFONTOBJ  FontObj, 
                        IN ULONG  Mode, 
                        IN ULONG  InputSize, 
                        IN PVOID  InputData, 
                        IN ULONG  OutputSize, 
                        OUT PVOID  OutputData); 
VOID DrvFree(IN PVOID  Obj, 
             IN ULONG  ID); 
ULONG DrvGetGlyphMode(IN DHPDEV  DPev,
                      IN PFONTOBJ  FontObj); 
ULONG DrvGetModes(IN HANDLE Driver,
                  IN ULONG DataSize,
                  OUT PDEVMODEW DM);
PVOID DrvGetTrueTypeFile(IN ULONG  FileNumber, 
                         IN PULONG  Size); 
BOOL DrvLineTo(IN PSURFOBJ SurfObj, 
               IN PCLIPOBJ ClipObj, 
               IN PBRUSHOBJ  BrushObj, 
               IN LONG  x1, 
               IN LONG  y1, 
               IN LONG  x2, 
               IN LONG  y2, 
               IN PRECTL  Bounds, 
               IN MIX  Mix); 
ULONG DrvLoadFontFile(IN ULONG  FileNumber, 
                      IN PVOID  ViewData, 
                      IN ULONG  ViewSize, 
                      IN ULONG  LangID); 
VOID DrvMovePointer(IN PSURFOBJ  SurfObj, 
                    IN LONG  x, 
                    IN LONG  y, 
                    IN PRECTL  RectL); 
BOOL DrvNextBand(IN PSURFOBJ  SurfObj, 
                 OUT PPOINTL  PointL); 
BOOL DrvPaint(IN PSURFOBJ  SurfObj, 
              IN PCLIPOBJ  ClipObj, 
              IN PBRUSHOBJ  BrushObj, 
              IN PPOINTL  BrushOrg, 
              IN MIX  Mix); 
BOOL DrvQueryAdvanceWidths(IN DHPDEV  DPev, 
                           IN PFONTOBJ  FontObj, 
                           IN ULONG  Mode, 
                           IN HGLYPH  Glyph, 
                           OUT PVOID  *Widths, 
                           IN ULONG  NumGlyphs); 
PIFIMETRICS DrvQueryFont(IN DHPDEV  PDev, 
                         IN ULONG  FileNumber, 
                         IN ULONG  FaceIndex, 
                         IN PULONG  Identifier); 
LONG DrvQueryFontCaps(IN ULONG  CapsSize, 
                      OUT PULONG  CapsData); 
LONG DrvQueryFontData(IN DHPDEV  DPev, 
                      IN PFONTOBJ  FontObj, 
                      IN ULONG  Mode, 
                      IN HGLYPH  Glyph, 
                      IN PGLYPHDATA  GlyphData, 
                      IN PVOID  DataBuffer, 
                      IN ULONG  BufferSize); 
 

BOOL EngAssociateSurface(IN HSURF  Surface,
                         IN HDEV  Dev,
                         IN ULONG  Hooks);
HBITMAP EngCreateBitmap(IN SIZEL  Size,
                        IN LONG  Width,
                        IN ULONG  Format,
                        IN ULONG  Flags,
                        IN PVOID  Bits);
HSURF EngCreateDeviceSurface(IN DHSURF  Surface,
                             IN SIZEL  Size,
                             IN ULONG  FormatVersion);
HPALETTE EngCreatePalette(IN ULONG  Mode,
                          IN ULONG  Colors, 
                          IN PULONG  *Colors, 
                          IN ULONG  Red, 
                          IN ULONG  Green, 
                          IN ULONG  Blue); 


