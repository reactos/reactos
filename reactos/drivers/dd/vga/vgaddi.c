/*
 *  VGADDI.C - Generic VGA DDI graphics driver
 * 
 */

#include <winddi.h>

VOID VGADDIAssertMode(IN DHPDEV  DPev, 
                      IN BOOL  Enable); 
VOID VGADDICompletePDEV(IN DHPDEV  PDev,
                        IN HDEV  Dev);
VOID VGADDIDisablePDEV(IN DHPDEV PDev); 
VOID VGADDIDisableSurface(IN DHPDEV PDev); 
DHPDEV VGADDIEnablePDEV(IN DEVMODEW  *DM,
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
HSURF VGADDIEnableSurface(IN DHPDEV  PDev);
ULONG VGADDIGetModes(IN HANDLE Driver,
                     IN ULONG DataSize,
                     OUT PDEVMODEW DM);

DRVFN FuncList[] =
{
  /*  Required Display driver fuctions  */
  {INDEX_DrvAssertMode, (PFN) VGADDIAssertMode},
  {INDEX_DrvCompletePDEV, (PFN) VGADDICompletePDEV},
  {INDEX_DrvDisablePDEV, (PFN) VGADDIDisablePDEV},
  {INDEX_DrvDisableSurface, (PFN) VGADDIDisableSurface},
  {INDEX_DrvEnablePDEV, (PFN) VGADDIEnablePDEV},
  {INDEX_DrvEnableSurface, (PFN) VGADDIEnableSurface},
  {INDEX_DrvGetModes, (PFN) VGADDIGetModes},

#if 0
  /*  Optional Display driver functions  */
  {INDEX_, (PFN) },
  {INDEX_DrvBitBlt, (PFN) VGADDIBitBlt},
  {INDEX_DrvCopyBits, (PFN) VGADDICopyBits},
  {INDEX_DescribePixelFormat, (PFN) VGADDIDescribePixelFormat},
  {INDEX_DrvDitherColor, (PFN) VGADDIDitherColor},
  {INDEX_DrvFillPath, (PFN) VGADDIFillPath},
  {INDEX_DrvGetTrueTypeFile, (PFN) VGADDIGetTrueTypeFile},
  {INDEX_DrvLoadFontFile, (PFN) VGADDILoadFontFile},
  {INDEX_DrvMovePointer, (PFN) VGADDIMovePointer},
  {INDEX_DrvPaint, (PFN) VGADDIPaint}
  {INDEX_DrvQueryFont, (PFN) VGADDIQueryFont},
  {INDEX_DrvQueryFontCaps, (PFN) VGADDIQueryFontCaps},
  {INDEX_DrvQueryFontData, (PFN) VGADDIQueryFontData},
  {INDEX_DrvQueryFontFile, (PFN) VGADDIQueryFontFile},
  {INDEX_DrvQueryFontTree, (PFN) VGADDIQueryFontTree},
  {INDEX_DrvQueryTrueTypeOutline, (PFN) VGADDIQueryTrueTypeOutline},
  {INDEX_DrvQueryTrueTypeTable, (PFN) VGADDIQueryTrueTypeTable},
  {INDEX_DrvRealizeBrush, (PFN) VGADDIRealizeBrush},
  {INDEX_DrvResetPDEV, (PFN) VGADDIResetPDEV},
  {INDEX_DrvSetPalette, (PFN) VGADDISetPalette},
  {INDEX_DrvSetPixelFormat, (PFN) VGADDISetPixelFormat},
  {INDEX_DrvSetPointerShape, (PFN) VGADDISetPointerShape},
  {INDEX_DrvStretchBlt, (PFN) VGADDIStretchBlt},
  {INDEX_DrvStrokePath, (PFN) VGADDIStrokePath},
  {INDEX_DrvSwapBuffers, (PFN) VGADDISwapBuffers},
  {INDEX_DrvTextOut, (PFN) VGADDITextOut},
  {INDEX_DrvUnloadFontFile, (PFN) VGADDIUnloadFontFile},
#endif
};

//  -----------------------------------------------------  Public Interface

//    DrvEnableDriver
//  DESCRIPTION:
//    This function is called by the KMGDI immediately after load of
//    the DDI module.  This is the entry point for the DDI.
//  ARGUMENTS:
//    IN ULONG            EngineVersion    Version of KMGDI
//    IN ULONG            SizeOfDED        Size of the DRVENABLEDATA struct
//    OUT PDRVENABLEDATA  DriveEnableData  Struct to fill in for KMGDI
//  RETURNS:
//    BOOL  TRUE if driver is enabled, FALSE and log an error otherwise

BOOL 
DrvEnableDriver(IN ULONG  EngineVersion, 
                IN ULONG  SizeOfDED, 
                OUT PDRVENABLEDATA  DriveEnableData)
{
  DriveEnableData->pdrvfn = FuncList;
  DriveEnableData->c = sizeof FuncList / sizeof &FuncList[0];
  
}

//    DrvDisableDriver
//  DESCRIPTION:
//    This function is called by the KMGDI at exit.  It should cleanup.
//  ARGUMENTS:
//    NONE
//  RETURNS:
//    NONE

VOID DrvDisableDriver(VOID)
{
}

//  -----------------------------------------------  Driver Implementation 

VOID VGADDIAssertMode(IN DHPDEV  DPev, 
                      IN BOOL  Enable); 
VOID VGADDICompletePDEV(IN DHPDEV  PDev,
                        IN HDEV  Dev);
VOID VGADDIDisablePDEV(IN DHPDEV PDev); 
VOID VGADDIDisableSurface(IN DHPDEV PDev); 
DHPDEV VGADDIEnablePDEV(IN DEVMODEW  *DM,
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
HSURF VGADDIEnableSurface(IN DHPDEV  PDev);
ULONG VGADDIGetModes(IN HANDLE Driver,
                     IN ULONG DataSize,
                     OUT PDEVMODEW DM);

