/*
 * WinDDI.h - definition of the GDI - DDI interface
 */

typedef HANDLE  HBITMAP;
typedef HANDLE  HDEV;
typedef HANDLE  HPALETTE;
typedef HANDLE  HSURF;

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

/*
 * Functions Prefixed with Drv are calls made from GDI to DDI, and
 * those prefixed with Eng are calls made from DDI to GDI.  DDI is
 * not allowed to make calls to any other kernel or user modules.
 */

/*  GDI --> DDI calls  */
VOID DrvCompletePDEV(IN DHPDEV PDev,
                     IN HDEV Dev);
VOID DrvDisablePDEV(IN DHPDEV PDev); 
VOID DrvDisableSurface(IN DHPDEV PDev); 
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
HSURF DrvEnableSurface(IN DHPDEV PDev);
ULONG DrvGetModes(IN HANDLE Driver,
                  IN ULONG DataSize,
                  OUT PDEVMODEW DM);

/*  DDI --> GDI calls  */
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


