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


