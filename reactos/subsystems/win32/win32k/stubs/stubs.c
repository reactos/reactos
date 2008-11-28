/*
 * Stubs for unimplemented WIN32K.SYS exports
 */

#include <w32k.h>

#define UNIMPLEMENTED DbgPrint("(%s:%i) WIN32K: %s UNIMPLEMENTED\n", __FILE__, __LINE__, __FUNCTION__ )




/*
 * @unimplemented
 */
BOOL
APIENTRY
EngMapFontFileFD (
	IN  ULONG_PTR  iFile,
	OUT PULONG    *ppjBuf,
	OUT ULONG     *pcjBuf
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0co7.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
APIENTRY
EngUnmapFontFileFD ( IN ULONG_PTR iFile )
{
  // http://www.osr.com/ddk/graphics/gdifncs_6wbr.htm
  UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngMapFontFile (
	ULONG_PTR  iFile,
	PULONG    *ppjBuf,
	ULONG     *pcjBuf
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3up3.htm
  return EngMapFontFileFD ( iFile, ppjBuf, pcjBuf );
}

/*
 * @implemented
 */
VOID
APIENTRY
EngUnmapFontFile ( ULONG_PTR iFile )
{
  // www.osr.com/ddk/graphics/gdifncs_09wn.htm
 EngUnmapFontFileFD ( iFile );
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngTextOut (
	SURFOBJ  *pso,
	STROBJ   *pstro,
	FONTOBJ  *pfo,
	CLIPOBJ  *pco,
	RECTL    *prclExtra,
	RECTL    *prclOpaque,
	BRUSHOBJ *pboFore,
	BRUSHOBJ *pboOpaque,
	POINTL   *pptlOrg,
	MIX       mix
	)
{
  // www.osr.com/ddk/graphics/gdifncs_4tgn.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
PATHOBJ*
APIENTRY
CLIPOBJ_ppoGetPath ( IN CLIPOBJ *pco )
{
  // www.osr.com/ddk/graphics/gdifncs_6hbb.htm
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngCheckAbort ( IN SURFOBJ *pso )
{
  // www.osr.com/ddk/graphics/gdifncs_3u7b.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
FD_GLYPHSET*
APIENTRY
EngComputeGlyphSet(
	IN INT nCodePage,
	IN INT nFirstChar,
	IN INT cChars
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9607.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
PATHOBJ*
APIENTRY
EngCreatePath ( VOID )
{
  // www.osr.com/ddk/graphics/gdifncs_4aav.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
VOID
APIENTRY
EngDeletePath ( IN PATHOBJ *ppo )
{
  // www.osr.com/ddk/graphics/gdifncs_3fl3.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngEnumForms (
	IN  HANDLE   hPrinter,
	IN  DWORD    Level,
	OUT LPBYTE   pForm,
	IN  DWORD    cbBuf,
	OUT LPDWORD  pcbNeeded,
	OUT LPDWORD  pcReturned
	)
{
  // www.osr.com/ddk/graphics/gdifncs_5e07.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngFillPath (
	IN SURFOBJ   *pso,
	IN PATHOBJ   *ppo,
	IN CLIPOBJ   *pco,
	IN BRUSHOBJ  *pbo,
	IN POINTL    *pptlBrushOrg,
	IN MIX        mix,
	IN FLONG      flOptions
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9pyf.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
PVOID
APIENTRY
EngFindResource(
	IN  HANDLE  h,
	IN  int     iName,
	IN  int     iType,
	OUT PULONG  pulSize
	)
{
  // www.osr.com/ddk/graphics/gdifncs_7rjb.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
VOID
APIENTRY
EngFreeModule ( IN HANDLE h )
{
  // www.osr.com/ddk/graphics/gdifncs_9fzb.htm
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
LPWSTR
APIENTRY
EngGetDriverName ( IN HDEV hdev )
{
  // www.osr.com/ddk/graphics/gdifncs_2gx3.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngGetFileChangeTime(
	IN  HANDLE          h,
	OUT LARGE_INTEGER  *pChangeTime
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1i1z.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngGetFilePath(
	IN  HANDLE h,
        OUT WCHAR (*pDest)[MAX_PATH+1]
	)
{
  // www.osr.com/ddk/graphics/gdifncs_5g2v.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngGetForm(
	IN  HANDLE   hPrinter,
	IN  LPWSTR   pFormName,
	IN  DWORD    Level,
	OUT LPBYTE   pForm,
	IN  DWORD    cbBuf,
	OUT LPDWORD  pcbNeeded
	)
{
  // www.osr.com/ddk/graphics/gdifncs_5vvr.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngGetPrinter(
	IN  HANDLE   hPrinter,
	IN  DWORD    dwLevel,
	OUT LPBYTE   pPrinter,
	IN  DWORD    cbBuf,
	OUT LPDWORD  pcbNeeded
	)
{
  // www.osr.com/ddk/graphics/gdifncs_50h3.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
APIENTRY
EngGetPrinterData(
	IN  HANDLE   hPrinter,
	IN  LPWSTR   pValueName,
	OUT LPDWORD  pType,
	OUT LPBYTE   pData,
	IN  DWORD    nSize,
	OUT LPDWORD  pcbNeeded
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8t5z.htm
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
LPWSTR
APIENTRY
EngGetPrinterDataFileName ( IN HDEV hdev )
{
  // www.osr.com/ddk/graphics/gdifncs_2giv.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngGetType1FontList(
	IN  HDEV            hdev,
	OUT TYPE1_FONT     *pType1Buffer,
	IN  ULONG           cjType1Buffer,
	OUT PULONG          pulLocalFonts,
	OUT PULONG          pulRemoteFonts,
	OUT LARGE_INTEGER  *pLastModified
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6e5j.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HANDLE
APIENTRY
EngLoadModuleForWrite(
	IN LPWSTR  pwsz,
	IN ULONG   cjSizeOfModule
	)
{
  // www.osr.com/ddk/graphics/gdifncs_98rr.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
PVOID
APIENTRY
EngMapModule(
	IN  HANDLE  h,
	OUT PULONG  pSize
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9b1j.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngMarkBandingSurface ( IN HSURF hsurf )
{
  // www.osr.com/ddk/graphics/gdifncs_2jon.htm
  UNIMPLEMENTED;
  return FALSE;
}

INT
APIENTRY
EngMultiByteToWideChar(
	IN UINT  CodePage,
	OUT LPWSTR  WideCharString,
	IN INT  BytesInWideCharString,
	IN LPSTR  MultiByteString,
	IN INT  BytesInMultiByteString
	)
{
  // www.osr.com/ddk/graphics/gdifncs_32cn.htm
  UNIMPLEMENTED;
  return 0;
}

VOID
APIENTRY
EngQueryLocalTime ( OUT PENG_TIME_FIELDS ptf )
{
  // www.osr.com/ddk/graphics/gdifncs_389z.htm
  UNIMPLEMENTED;
}

ULONG
APIENTRY
EngQueryPalette(
	IN HPALETTE  hPal,
	OUT ULONG  *piMode,
	IN ULONG  cColors,
	OUT ULONG  *pulColors
	)
{
  // www.osr.com/ddk/graphics/gdifncs_21t3.htm
  UNIMPLEMENTED;
  return 0;
}

BOOL
APIENTRY
EngSetPointerTag(
	IN HDEV  hdev,
	IN SURFOBJ  *psoMask,
	IN SURFOBJ  *psoColor,
	IN XLATEOBJ  *pxlo,
	IN FLONG  fl
	)
{
  // This function is obsolete for Windows 2000 and later.
  // This function is still supported, but always returns FALSE.
  // www.osr.com/ddk/graphics/gdifncs_4yav.htm
  return FALSE;
}

DWORD
APIENTRY
EngSetPrinterData(
	IN HANDLE  hPrinter,
	IN LPWSTR  pType,
	IN DWORD  dwType,
	IN LPBYTE  lpbPrinterData,
	IN DWORD  cjPrinterData
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8drb.htm
  UNIMPLEMENTED;
  return 0;
}

BOOL
APIENTRY
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
	IN FLONG  flOptions
	)
{
  // www.osr.com/ddk/graphics/gdifncs_2xwn.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
APIENTRY
EngStrokePath(
	IN SURFOBJ  *pso,
	IN PATHOBJ  *ppo,
	IN CLIPOBJ  *pco,
	IN XFORMOBJ  *pxo,
	IN BRUSHOBJ  *pbo,
	IN POINTL  *pptlBrushOrg,
	IN LINEATTRS  *plineattrs,
	IN MIX  mix
	)
{
  // www.osr.com/ddk/graphics/gdifncs_4yaw.htm
  UNIMPLEMENTED;
  return FALSE;
}


INT
APIENTRY
EngWideCharToMultiByte(
	IN UINT  CodePage,
	IN LPWSTR  WideCharString,
	IN INT  BytesInWideCharString,
	OUT LPSTR  MultiByteString,
	IN INT  BytesInMultiByteString
	)
{
  // www.osr.com/ddk/graphics/gdifncs_35wn.htm
  UNIMPLEMENTED;
  return 0;
}

BOOL
APIENTRY
EngWritePrinter (
	IN HANDLE    hPrinter,
	IN LPVOID    pBuf,
	IN DWORD     cbBuf,
	OUT LPDWORD  pcWritten
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9v6v.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
ULONG
APIENTRY
FONTOBJ_cGetAllGlyphHandles (
	IN FONTOBJ  *FontObj,
	IN HGLYPH   *Glyphs
	)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
ULONG
APIENTRY
FONTOBJ_cGetGlyphs(
	IN FONTOBJ *FontObj,
	IN ULONG    Mode,
	IN ULONG    NumGlyphs,
	IN HGLYPH  *GlyphHandles,
	IN PVOID   *OutGlyphs
	)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
IFIMETRICS*
APIENTRY
FONTOBJ_pifi ( IN FONTOBJ *FontObj )
{
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
PVOID
APIENTRY
FONTOBJ_pvTrueTypeFontFile (
	IN FONTOBJ  *FontObj,
	IN ULONG    *FileSize)
{
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
XFORMOBJ*
APIENTRY
FONTOBJ_pxoGetXform ( IN FONTOBJ *FontObj )
{
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
VOID
APIENTRY
FONTOBJ_vGetInfo (
	IN  FONTOBJ   *FontObj,
	IN  ULONG      InfoSize,
	OUT PFONTINFO  FontInfo)
{
  UNIMPLEMENTED;
}

LONG
APIENTRY
HT_ComputeRGBGammaTable(
	IN USHORT  GammaTableEntries,
	IN USHORT  GammaTableType,
	IN USHORT  RedGamma,
	IN USHORT  GreenGamma,
	IN USHORT  BlueGamma,
	OUT LPBYTE  pGammaTable
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9dpj.htm
  UNIMPLEMENTED;
  return 0;
}

LONG
APIENTRY
HT_Get8BPPFormatPalette(
	OUT LPPALETTEENTRY  pPaletteEntry,
	IN USHORT  RedGamma,
	IN USHORT  GreenGamma,
	IN USHORT  BlueGamma
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8kvb.htm
  UNIMPLEMENTED;
  return 0;
}

BOOL
APIENTRY
PATHOBJ_bCloseFigure ( IN PATHOBJ *ppo )
{
  // www.osr.com/ddk/graphics/gdifncs_5mhz.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bEnum (
	IN  PATHOBJ   *ppo,
	OUT PATHDATA  *ppd
	)
{
  // www.osr.com/ddk/graphics/gdifncs_98o7.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bEnumClipLines(
	IN PATHOBJ  *ppo,
	IN ULONG  cb,
	OUT CLIPLINE  *pcl
	)
{
  // www.osr.com/ddk/graphics/gdifncs_4147.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bMoveTo(
	IN PATHOBJ  *ppo,
	IN POINTFIX  ptfx
	)
{
  // www.osr.com/ddk/graphics/gdifncs_70vb.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bPolyBezierTo(
	IN PATHOBJ  *ppo,
	IN POINTFIX  *pptfx,
	IN ULONG  cptfx
	)
{
  // www.osr.com/ddk/graphics/gdifncs_2c9z.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bPolyLineTo(
	IN PATHOBJ  *ppo,
	IN POINTFIX  *pptfx,
	IN ULONG  cptfx
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0x47.htm
  UNIMPLEMENTED;
  return FALSE;
}

VOID
APIENTRY
PATHOBJ_vEnumStart ( IN PATHOBJ *ppo )
{
  // www.osr.com/ddk/graphics/gdifncs_74br.htm
  UNIMPLEMENTED;
}

VOID
APIENTRY
PATHOBJ_vEnumStartClipLines(
	IN PATHOBJ  *ppo,
	IN CLIPOBJ  *pco,
	IN SURFOBJ  *pso,
	IN LINEATTRS  *pla
	)
{
  // www.osr.com/ddk/graphics/gdifncs_5grr.htm
  UNIMPLEMENTED;
}

VOID
APIENTRY
PATHOBJ_vGetBounds(
	IN PATHOBJ  *ppo,
	OUT PRECTFX  prectfx
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8qp3.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
ULONG APIENTRY
EngDitherColor(
   IN HDEV hdev,
   IN ULONG iMode,
   IN ULONG rgb,
   OUT ULONG *pul)
{
   return DCR_SOLID;
}

/*
 * @implemented
 */
BOOL APIENTRY
EngQuerySystemAttribute(
   IN ENG_SYSTEM_ATTRIBUTE CapNum,
   OUT PDWORD pCapability)
{
  SYSTEM_BASIC_INFORMATION sbi;
  SYSTEM_PROCESSOR_INFORMATION spi;

   switch (CapNum)
   {
      case EngNumberOfProcessors:
         NtQuerySystemInformation(
                SystemBasicInformation,
               &sbi,
                sizeof(SYSTEM_BASIC_INFORMATION),
                NULL);
         *pCapability = sbi.NumberOfProcessors;
         return TRUE;

      case EngProcessorFeature:
         NtQuerySystemInformation(
                SystemProcessorInformation,
               &spi,
                sizeof(SYSTEM_PROCESSOR_INFORMATION),
                NULL);
         *pCapability = spi.ProcessorFeatureBits;
         return TRUE;

      default:
         break;
   }

   return FALSE;
}


/*
 * @unimplemented
 */
HANDLE APIENTRY
BRUSHOBJ_hGetColorTransform(
   IN BRUSHOBJ *Brush)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
VOID
APIENTRY
EngClearEvent(
   IN PEVENT Event)
{
    /* Forward to the kernel */
    KeClearEvent((PKEVENT)Event);
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngDeleteFile(
   IN LPWSTR FileName)
{
   UNIMPLEMENTED;
   return FALSE;
}



/*
 * @unimplemented
 */
BOOL APIENTRY
EngGetPrinterDriver(
   IN HANDLE Printer,
   IN LPWSTR Environment,
   IN DWORD Level,
   OUT BYTE *DrvInfo,
   IN DWORD Buf,
   OUT DWORD *Needed)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
ULONG APIENTRY
EngHangNotification(
   IN HDEV Dev,
   IN PVOID Reserved)
{
   UNIMPLEMENTED;
   return EHN_ERROR;
}



/*
 * @unimplemented
 */
BOOL APIENTRY
EngLpkInstalled()
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
PVOID APIENTRY
EngMapFile(
   IN LPWSTR Filename,
   IN ULONG Size,
   OUT ULONG_PTR *File)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
EngPlgBlt(
   IN SURFOBJ *Dest,
   IN SURFOBJ *Source,
   IN SURFOBJ *Mask,
   IN CLIPOBJ *Clip,
   IN XLATEOBJ *Xlate,
   IN COLORADJUSTMENT *ColorAdjustment,
   IN POINTL *BrusOrigin,
   IN POINTFIX *DestPoints,
   IN RECTL *SourceRect,
   IN POINTL *MaskPoint,
   IN ULONG Mode)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
EngQueryDeviceAttribute(
   IN HDEV Device,
   IN ENG_DEVICE_ATTRIBUTE Attribute,
   IN VOID *In,
   IN ULONG InSize,
   OUT VOID *Out,
   OUT ULONG OutSize)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
LARGE_INTEGER
APIENTRY
EngQueryFileTimeStamp(IN LPWSTR FileName)
{
   LARGE_INTEGER FileTime;
   FileTime.QuadPart = 0;
   UNIMPLEMENTED;
   return FileTime;
}

/*
 * @unimplemented
 */
LONG APIENTRY
EngReadStateEvent(
   IN PEVENT Event)
{
   UNIMPLEMENTED;
   return 0;
}
BOOL APIENTRY
EngStretchBltROP(
   IN SURFOBJ *Dest,
   IN SURFOBJ *Source,
   IN SURFOBJ *Mask,
   IN CLIPOBJ *Clip,
   IN XLATEOBJ *Xlate,
   IN COLORADJUSTMENT *ColorAdjustment,
   IN POINTL *BrushOrigin,
   IN RECTL *DestRect,
   IN RECTL *SourceRect,
   IN POINTL *MaskPoint,
   IN ULONG Mode,
   IN BRUSHOBJ *BrushObj,
   IN DWORD ROP4)
{
   UNIMPLEMENTED;
   return FALSE;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
EngUnmapFile(
   IN ULONG_PTR File)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
FD_GLYPHSET * APIENTRY
FONTOBJ_pfdg(
   IN FONTOBJ *FontObj)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PBYTE APIENTRY
FONTOBJ_pjOpenTypeTablePointer(
   IN FONTOBJ *FontObj,
   IN ULONG Tag,
   OUT ULONG *Table)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PFD_GLYPHATTR APIENTRY
FONTOBJ_pQueryGlyphAttrs(
   IN FONTOBJ *FontObj,
   IN ULONG Mode)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
LPWSTR APIENTRY
FONTOBJ_pwszFontFilePaths(
   IN FONTOBJ *FontObj,
   OUT ULONG *PathLength)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
LONG APIENTRY
HT_Get8BPPMaskPalette(
   IN OUT LPPALETTEENTRY PaletteEntry,
   IN BOOL Use8BPPMaskPal,
   IN BYTE CMYMask,
   IN USHORT RedGamma,
   IN USHORT GreenGamma,
   IN USHORT BlueGamma)
{
   UNIMPLEMENTED;
   return 0;
}

/*
 * @unimplemented
 */
HANDLE APIENTRY
XLATEOBJ_hGetColorTransform(
   IN XLATEOBJ *XlateObj)
{
   UNIMPLEMENTED;
   return NULL;
}


/*
 * @unimplemented
 */

BOOL
APIENTRY
NtGdiAnyLinkedFonts()
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
HANDLE
APIENTRY
NtGdiBRUSHOBJ_hGetColorTransform(
   IN BRUSHOBJ *Brush)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PVOID APIENTRY
NtGdiBRUSHOBJ_pvAllocRbrush(IN BRUSHOBJ *BrushObj,
                            IN ULONG ObjSize)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PVOID APIENTRY
NtGdiBRUSHOBJ_pvGetRbrush(IN BRUSHOBJ *BrushObj)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
ULONG APIENTRY
NtGdiBRUSHOBJ_ulGetBrushColor(BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiCLIPOBJ_bEnum( IN CLIPOBJ *ClipObj,
                    IN ULONG ObjSize,
                    OUT ULONG *EnumRects)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
ULONG APIENTRY
NtGdiCLIPOBJ_cEnumStart(IN CLIPOBJ *ClipObj,
                        IN BOOL ShouldDoAll,
                        IN ULONG ClipType,
                        IN ULONG BuildOrder,
                        IN ULONG MaxRects)
{
    UNIMPLEMENTED;
    return 0;
}


/*
 * @unimplemented
 */
PATHOBJ* APIENTRY
NtGdiCLIPOBJ_ppoGetPath(CLIPOBJ *ClipObj)
{
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiEnableEudc(BOOL enable)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngAssociateSurface(IN HSURF Surface,
                         IN HDEV Dev,
                         IN ULONG Hooks)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
CLIPOBJ* APIENTRY
NtGdiEngCreateClip(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
VOID APIENTRY
NtGdiEngDeleteClip(CLIPOBJ *ClipRegion)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngCheckAbort(SURFOBJ *pso)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
HSURF APIENTRY
NtGdiEngCreateDeviceSurface( IN DHSURF Surface,
                             IN SIZEL Size,
                             IN ULONG FormatVersion)
{
     UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
HBITMAP APIENTRY
NtGdiEngCreateDeviceBitmap(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormatCompat)
{
     UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngCopyBits(SURFOBJ *Dest,
	    SURFOBJ *Source,
	    CLIPOBJ *Clip,
	    XLATEOBJ *ColorTranslation,
	    RECTL *DestRect,
	    POINTL *SourcePoint)
{
     UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
HBITMAP APIENTRY
NtGdiEngCreateBitmap(IN SIZEL Size,
		IN LONG Width,
		IN ULONG Format,
		IN ULONG Flags,
		IN PVOID Bits)
{
    return EngCreateBitmap(Size,
                          Width,
                         Format,
                          Flags,
                           Bits);
}


/*
 * @implemented
 */
HPALETTE APIENTRY
NtGdiEngCreatePalette(IN ULONG Mode,
		 IN ULONG NumColors,
		 IN ULONG *Colors,
		 IN ULONG Red,
		 IN ULONG Green,
		 IN ULONG Blue)
{
    return EngCreatePalette( Mode,
                        NumColors,
                           Colors,
                              Red,
                            Green,
                             Blue);
}

BOOL APIENTRY
NtGdiEngTransparentBlt(IN SURFOBJ *Dest,
		  IN SURFOBJ *Source,
		  IN CLIPOBJ *Clip,
		  IN XLATEOBJ *ColorTranslation,
		  IN PRECTL DestRect,
		  IN PRECTL SourceRect,
		  IN ULONG TransparentColor,
		  IN ULONG Reserved)
{
     UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngTextOut(SURFOBJ *pso,
                STROBJ *pstro,
                FONTOBJ *pfo,
                CLIPOBJ *pco,
                RECTL *prclExtra,
                RECTL *prclOpaque,
                BRUSHOBJ *pboFore,
                BRUSHOBJ *pboOpaque,
                POINTL *pptlOrg,
                MIX mix)
{
     UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngStrokePath(SURFOBJ *pso,
                   PATHOBJ *ppo,
                   CLIPOBJ *pco,
                   XFORMOBJ *pxo,
                   BRUSHOBJ *pbo,
                   POINTL *pptlBrushOrg,
                   LINEATTRS *plineattrs,
                   MIX mix)
{
     UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtGdiEngDeletePalette(IN HPALETTE Palette)
{
    return EngDeletePalette(Palette);
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngStrokeAndFillPath(SURFOBJ *pso,
                          PATHOBJ *ppo,
                          CLIPOBJ *pco,
                          XFORMOBJ *pxo,
                          BRUSHOBJ *pboStroke,
                          LINEATTRS *plineattrs,
                          BRUSHOBJ *pboFill,
                          POINTL *pptlBrushOrg,
                          MIX mixFill,
                          FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
VOID APIENTRY
NtGdiEngDeletePath(PATHOBJ *ppo)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngDeleteSurface(IN HSURF Surface)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngEraseSurface(SURFOBJ *Surface,
                     RECTL *Rect,
                     ULONG iColor)
{
    UNIMPLEMENTED;
    return FALSE;
}










/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngFillPath(SURFOBJ *pso,
                 PATHOBJ *ppo,
                 CLIPOBJ *pco,
                 BRUSHOBJ *pbo,
                 POINTL *pptlBrushOrg,
                 MIX mix,
                 FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY
NtGdiEngGradientFill(SURFOBJ *psoDest,
                     CLIPOBJ *pco,
                     XLATEOBJ *pxlo,
                     TRIVERTEX *pVertex,
                     ULONG nVertex,
                     PVOID pMesh,
                     ULONG nMesh,
                     RECTL *prclExtents,
                     POINTL *pptlDitherOrg,
                     ULONG ulMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY
NtGdiEngLineTo(SURFOBJ *Surface,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngMarkBandingSurface(HSURF hsurf)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngPaint(IN SURFOBJ *Surface,
	 IN CLIPOBJ *ClipRegion,
	 IN BRUSHOBJ *Brush,
	 IN POINTL *BrushOrigin,
	 IN MIX  Mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngPlgBlt(SURFOBJ *psoTrg,
               SURFOBJ *psoSrc,
               SURFOBJ *psoMsk,
               CLIPOBJ *pco,
               XLATEOBJ *pxlo,
               COLORADJUSTMENT *pca,
               POINTL *pptlBrushOrg,
               POINTFIX *pptfx,
               RECTL *prcl,
               POINTL *pptl,
               ULONG iMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtGdiEngStretchBltROP(SURFOBJ *psoDest,
                      SURFOBJ *psoSrc,
                      SURFOBJ *psoMask,
                      CLIPOBJ *pco,
                      XLATEOBJ *pxlo,
                      COLORADJUSTMENT *pca,
                      POINTL *pptlHTOrg,
                      RECTL *prclDest,
                      RECTL *prclSrc,
                      POINTL *pptlMask,
                      ULONG iMode,
                      BRUSHOBJ *pbo,
                      DWORD rop4)
{
    UNIMPLEMENTED;
    return FALSE;
}

FD_GLYPHSET* APIENTRY
NtGdiEngComputeGlyphSet( INT nCodePage,
                         INT nFirstChar,
                         INT cChars)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiFONTOBJ_cGetAllGlyphHandles(IN FONTOBJ *FontObj,
                            IN HGLYPH  *Glyphs)
{
    UNIMPLEMENTED;
    return 0;
}


/*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(IN FONTOBJ *FontObj,
                        IN ULONG    Mode,
                        IN ULONG    NumGlyphs,
                        IN HGLYPH  *GlyphHandles,
                        IN PVOID   *OutGlyphs)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiAddRemoteFontToDC(
    IN HDC hdc,
    IN PVOID pvBuffer,
    IN ULONG cjBuffer,
    IN OPTIONAL PUNIVERSAL_FONT_ID pufi)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
APIENTRY
NtGdiAddFontMemResourceEx(
    IN PVOID pvBuffer,
    IN DWORD cjBuffer,
    IN DESIGNVECTOR *pdv,
    IN ULONG cjDV,
    OUT DWORD *pNumFonts
)
{
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiRemoveMergeFont(
    IN HDC hdc,
    IN UNIVERSAL_FONT_ID *pufi)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiAddRemoteMMInstanceToDC(
    IN HDC hdc,
    IN DOWNLOADDESIGNVECTOR *pddv,
    IN ULONG cjDDV)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiFontIsLinked(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiCheckBitmapBits(
    IN HDC hdc,
    IN HANDLE hColorTransform,
    IN PVOID pvBits,
    IN ULONG bmFormat,
    IN DWORD dwWidth,
    IN DWORD dwHeight,
    IN DWORD dwStride,
    OUT PBYTE paResults)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
HBITMAP
APIENTRY
NtGdiClearBitmapAttributes(
    IN HBITMAP hbm,
    IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}


 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiGetUFI(
    IN  HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG *pfl)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
PFD_GLYPHATTR
APIENTRY
NtGdiFONTOBJ_pQueryGlyphAttrs(
    IN FONTOBJ *pfo,
    IN ULONG iMode)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
IFIMETRICS*
APIENTRY
NtGdiFONTOBJ_pifi(
    IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
FD_GLYPHSET*
APIENTRY
NtGdiFONTOBJ_pfdg(IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
PVOID
APIENTRY
NtGdiFONTOBJ_pvTrueTypeFontFile(
    IN FONTOBJ *pfo,
    OUT ULONG *pcjFile
)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
VOID
APIENTRY
NtGdiFONTOBJ_vGetInfo(
    IN FONTOBJ *pfo,
    IN ULONG cjSize,
    OUT FONTINFO *pfi)
{
    UNIMPLEMENTED;
}

 /*
 * @unimplemented
 */
XFORMOBJ*
APIENTRY
NtGdiFONTOBJ_pxoGetXform(IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    IN HBRUSH hbm,
    IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiColorCorrectPalette(
    IN HDC hdc,
    IN HPALETTE hpal,
    IN ULONG FirstEntry,
    IN ULONG NumberOfEntries,
    IN OUT PALETTEENTRY *ppalEntry,
    IN ULONG Command)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
HANDLE
APIENTRY
NtGdiCreateColorTransform(
    IN HDC hdc,
    IN LPLOGCOLORSPACEW pLogColorSpaceW,
    IN OPTIONAL PVOID pvSrcProfile,
    IN ULONG cjSrcProfile,
    IN OPTIONAL PVOID pvDestProfile,
    IN ULONG cjDestProfile,
    IN OPTIONAL PVOID pvTargetProfile,
    IN ULONG cjTargetProfile)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiComputeXformCoefficients(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiConsoleTextOut(
    IN HDC hdc,
    IN POLYTEXTW *lpto,
    IN UINT nStrings,
    IN RECTL *prclBounds)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiDeleteColorTransform(
    IN HDC hdc,
    IN HANDLE hColorTransform)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiGetPerBandInfo(
    IN HDC hdc,
    IN OUT PERBANDINFO *ppbi)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiDoBanding(
    IN HDC hdc,
    IN BOOL bStart,
    OUT POINTL *pptl,
    OUT PSIZE pSize)
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiEnumFontChunk(
    IN HDC hdc,
    IN ULONG_PTR idEnum,
    IN ULONG cjEfdw,
    OUT ULONG *pcjEfdw,
    OUT PENUMFONTDATAW pefdw)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiEnumFontClose(
    IN ULONG_PTR idEnum)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
ULONG_PTR
APIENTRY
NtGdiEnumFontOpen(
    IN HDC hdc,
    IN ULONG iEnumType,
    IN FLONG flWin31Compat,
    IN ULONG cwchMax,
    IN OPTIONAL LPWSTR pwszFaceName,
    IN ULONG lfCharSet,
    OUT ULONG *pulCount)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiEudcLoadUnloadLink(
    IN OPTIONAL LPCWSTR pBaseFaceName,
    IN UINT cwcBaseFaceName,
    IN LPCWSTR pEudcFontPath,
    IN UINT cwcEudcFontPath,
    IN INT iPriority,
    IN INT iFontLinkType,
    IN BOOL bLoadLin)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiForceUFIMapping(
    IN HDC hdc,
    IN PUNIVERSAL_FONT_ID pufi)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
NTSTATUS
APIENTRY
NtGdiFullscreenControl(
    IN FULLSCREENCONTROL FullscreenCommand,
    IN PVOID FullscreenInput,
    IN DWORD FullscreenInputLength,
    OUT PVOID FullscreenOutput,
    IN OUT PULONG FullscreenOutputLength)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
DWORD
APIENTRY
NtGdiGetCharacterPlacementW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN INT nCount,
    IN INT nMaxExtent,
    IN OUT LPGCP_RESULTSW pgcpw,
    IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiGetCharWidthInfo(
    IN HDC hdc,
    OUT PCHWIDTHINFO pChWidthInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
ULONG_PTR
APIENTRY
NtGdiGetColorSpaceforBitmap(
    IN HBITMAP hsurf)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiGetETM(
    IN HDC hdc,
    OUT EXTTEXTMETRIC *petm)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiGetEudcTimeStampEx(
    IN OPTIONAL LPWSTR lpBaseFaceName,
    IN ULONG cwcBaseFaceName,
    IN BOOL bSystemTimeStamp)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiInitSpool()
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
INT
APIENTRY
NtGdiQueryFonts( OUT PUNIVERSAL_FONT_ID pufiFontList,
                 IN ULONG nBufferSize,
                 OUT PLARGE_INTEGER pTimeStamp)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
INT
APIENTRY
NtGdiGetSpoolMessage( DWORD u1,
                      DWORD u2,
                      DWORD u3,
                      DWORD u4)
{
    /* FIXME the prototypes */
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
DWORD
APIENTRY
NtGdiGetGlyphIndicesWInternal(
    IN HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD pgi,
    IN DWORD iMode,
    IN BOOL bSubset)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
INT
APIENTRY
NtGdiGetLinkedUFIs(
    IN HDC hdc,
    OUT OPTIONAL PUNIVERSAL_FONT_ID pufiLinkedUFIs,
    IN INT BufferSize)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    IN HBRUSH hbr,
    OUT UINT *piUsage)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiGetMonitorID(
    IN  HDC hdc,
    IN  DWORD dwSize,
    OUT LPWSTR pszMonitorID)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiDrawStream(
    IN HDC hdcDst,
    IN ULONG cjIn,
    IN VOID *pvIn)
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
BOOL
NtGdiUMPDEngFreeUserMem(
    IN KERNEL_PVOID *ppv)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
NtGdiBRUSHOBJ_DeleteRbrush(
    IN BRUSHOBJ *pbo,
    IN BRUSHOBJ *pboB)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
NtGdiSetPUMPDOBJ(
    IN HUMPD humpd,
    IN BOOL bStoreID,
    OUT HUMPD *phumpd,
    OUT BOOL *pbWOW64)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
NtGdiUpdateTransform(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
LONG
APIENTRY
NtGdiHT_Get8BPPMaskPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    IN BOOL Use8BPPMaskPal,
    IN BYTE CMYMask,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
LONG
APIENTRY
NtGdiHT_Get8BPPFormatPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiQueryFontAssocInfo(
    IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
UINT
APIENTRY
NtGdiGetStringBitmapW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN UINT cwc,
    OUT BYTE *lpSB,
    IN UINT cj)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiPATHOBJ_bEnum(
    IN PATHOBJ *ppo,
    OUT PATHDATA *ppd)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiPATHOBJ_bEnumClipLines(
    IN PATHOBJ *ppo,
    IN ULONG cb,
    OUT CLIPLINE *pcl)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
VOID
APIENTRY
NtGdiPATHOBJ_vGetBounds(
    IN PATHOBJ *ppo,
    OUT PRECTFX prectfx)
{
    UNIMPLEMENTED;
}



 /*
 * @unimplemented
 */
VOID
APIENTRY
NtGdiPATHOBJ_vEnumStart(
    IN PATHOBJ *ppo)
{
    UNIMPLEMENTED;
}

 /*
 * @unimplemented
 */
VOID
APIENTRY
NtGdiPATHOBJ_vEnumStartClipLines(
    IN PATHOBJ *ppo,
    IN CLIPOBJ *pco,
    IN SURFOBJ *pso,
    IN LINEATTRS *pla)
{
    UNIMPLEMENTED;
}


 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(
    IN HANDLE hMMFont)
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN ULONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiPolyTextOutW(
    IN HDC hdc,
    IN POLYTEXTW *pptw,
    IN UINT cStr,
    IN DWORD dwCodePage)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
NTSTATUS
APIENTRY
NtGdiGetStats(
    IN HANDLE hProcess,
    IN INT iIndex,
    IN INT iPidType,
    OUT PVOID pResults,
    IN UINT cjResultSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(
    IN XLATEOBJ *pxlo,
    IN ULONG iPal,
    IN ULONG cPal,
    OUT ULONG *pPal)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(
    IN XLATEOBJ *pxlo,
    IN ULONG iColor)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
HANDLE
APIENTRY
NtGdiXLATEOBJ_hGetColorTransform(
    IN XLATEOBJ *pxlo)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiXFORMOBJ_bApplyXform(
    IN XFORMOBJ *pxo,
    IN ULONG iMode,
    IN ULONG cPoints,
    IN  PVOID pvIn,
    OUT PVOID pvOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiXFORMOBJ_iGetXform(
    IN XFORMOBJ *pxo,
    OUT OPTIONAL XFORML *pxform)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
INT
APIENTRY
NtGdiSetupPublicCFONT(
    IN HDC hdc,
    IN OPTIONAL HFONT hf,
    IN ULONG ulAve)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    IN HBRUSH hbm,
    IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiSTROBJ_bEnum(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiSTROBJ_bEnumPositionsOnly(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiSTROBJ_bGetAdvanceWidths(
    IN STROBJ*pstro,
    IN ULONG iFirst,
    IN ULONG c,
    OUT POINTQF*pptqD)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
VOID
APIENTRY
NtGdiSTROBJ_vEnumStart(
    IN STROBJ *pstro)
{
    UNIMPLEMENTED;
}

 /*
 * @unimplemented
 */
DWORD
APIENTRY
NtGdiSTROBJ_dwGetCodePage(
    IN STROBJ *pstro)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiGetEmbUFI(
    IN HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG  *pfl,
    OUT KERNEL_PVOID *embFontID)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiGetUFIPathname(
    IN PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL ULONG* pcwc,
    OUT OPTIONAL LPWSTR pwszPathname,
    OUT OPTIONAL ULONG* pcNumFiles,
    IN FLONG fl,
    OUT OPTIONAL BOOL *pbMemFont,
    OUT OPTIONAL ULONG *pcjView,
    OUT OPTIONAL PVOID pvView,
    OUT OPTIONAL BOOL *pbTTC,
    OUT OPTIONAL ULONG *piTTC)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiGetEmbedFonts(
    VOID)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiChangeGhostFont(
    IN KERNEL_PVOID *pfontID,
    IN BOOL bLoad)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiAddEmbFontToDC(
    IN HDC hdc,
    IN VOID **pFontID)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiGetWidthTable(
    IN HDC hdc,
    IN ULONG cSpecial,
    IN WCHAR *pwc,
    IN ULONG cwc,
    OUT USHORT *psWidth,
    OUT OPTIONAL WIDTHDATA *pwd,
    OUT FLONG *pflInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiIcmBrushInfo(
    IN HDC hdc,
    IN HBRUSH hbrush,
    IN OUT PBITMAPINFO pbmiDIB,
    IN OUT PVOID pvBits,
    IN OUT ULONG *pulBits,
    OUT OPTIONAL DWORD *piUsage,
    OUT OPTIONAL BOOL *pbAlreadyTran,
    IN ULONG Command)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @implemented
 */
BOOL
APIENTRY
NtGdiInit()
{
    return TRUE;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiMakeFontDir(
    IN FLONG flEmbed,
    OUT PBYTE pjFontDir,
    IN unsigned cjFontDir,
    IN LPWSTR pwszPathname,
    IN unsigned cjPathname)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiMonoBitmap(
    IN HBITMAP hbm)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
HBITMAP
APIENTRY
NtGdiSetBitmapAttributes(
    IN HBITMAP hbm,
    IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiSetFontEnumeration(
    IN ULONG ulType)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiSetFontXform(
    IN HDC hdc,
    IN DWORD dwxScale,
    IN DWORD dwyScale)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiSetLinkedUFIs(
    IN HDC hdc,
    IN PUNIVERSAL_FONT_ID pufiLinks,
    IN ULONG uNumUFIs)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiSetMagicColors(
    IN HDC hdc,
    IN PALETTEENTRY peMagic,
    IN ULONG Index)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiUnloadPrinterDriver(
    IN LPWSTR pDriverName,
    IN ULONG cbDriverName)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @implemented
 */
BOOL
APIENTRY
NtGdiUnmapMemFont(
    IN PVOID pvView)
{
    return TRUE;
}

BOOL
STDCALL
EngControlSprites(
  IN WNDOBJ  *pwo,
  IN FLONG  fl)
{
  UNIMPLEMENTED;
  return FALSE;
}

PVOID
APIENTRY
EngFntCacheAlloc(IN ULONG FastCheckSum,
                 IN ULONG ulSize)
{
    UNIMPLEMENTED;
    return NULL;
}
 
VOID
APIENTRY
EngFntCacheFault(IN ULONG ulFastCheckSum,
                 IN ULONG iFaultMode)
{
    UNIMPLEMENTED;
}
 
PVOID
APIENTRY
EngFntCacheLookUp(IN ULONG FastCheckSum,
                  OUT PULONG pulSize)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
FLOATOBJ_AddFloatObj(PFLOATOBJ pFloatObj1,
                     PFLOATOBJ pFloatObj2)
{
    UNIMPLEMENTED;
}
 
VOID
APIENTRY
FLOATOBJ_DivFloatObj(PFLOATOBJ pFloatObj1,
                     PFLOATOBJ pFloatObj2)
{
    UNIMPLEMENTED;
}
 
VOID
APIENTRY
FLOATOBJ_MulFloatObj(PFLOATOBJ pFloatObj1,
                     PFLOATOBJ pFloatObj2)
{
    UNIMPLEMENTED;
}
 
VOID
APIENTRY
FLOATOBJ_SubFloatObj(PFLOATOBJ pFloatObj1,
                     PFLOATOBJ pFloatObj2)
{
    UNIMPLEMENTED;
}
 
PVOID
APIENTRY
EngAllocSectionMem(IN PVOID SectionObject,
                   IN ULONG Flags,
                   IN SIZE_T MemSize,
                   IN ULONG Tag)
{
    UNIMPLEMENTED;
    return NULL;
}
 
NTSTATUS
APIENTRY
EngFileIoControl(IN PFILE_OBJECT FileObject,
                 IN ULONG IoControlCode,
                 IN PVOID InputBuffer,
                 IN SIZE_T InputBufferLength,
                 OUT PVOID OutputBuffer,
                 IN SIZE_T OutputBufferLength,
                 OUT PULONG Information)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
 
VOID
APIENTRY
EngFileWrite(IN PFILE_OBJECT FileObject,
             IN PVOID Buffer,
             IN SIZE_T Length,
             IN PSIZE_T BytesWritten)
{
    UNIMPLEMENTED;
}
 
BOOLEAN
APIENTRY
EngFreeSectionMem(IN PVOID SectionObject OPTIONAL,
                  IN PVOID MappedBase)
{
    UNIMPLEMENTED;
    return FALSE;
}
 
ULONGLONG
APIENTRY
EngGetTickCount(VOID)
{
 return ((ULONGLONG)SharedUserData->TickCountLowDeprecated * SharedUserData->TickCountMultiplier / 16777216);
}
 
BOOLEAN
APIENTRY
EngMapSection(IN PVOID Section,
              IN BOOLEAN Map,
              IN HANDLE Process,
              IN PVOID* BaseAddress)
{
    UNIMPLEMENTED;
    return FALSE;
}
 
BOOLEAN
APIENTRY
EngNineGrid(IN SURFOBJ* pDestSurfaceObj,
            IN SURFOBJ* pSourceSurfaceObj,
            IN CLIPOBJ* pClipObj,
            IN XLATEOBJ* pXlateObj,
            IN RECTL* prclSource,
            IN RECTL* prclDest,
            PVOID pvUnknown1,
            PVOID pvUnknown2,
            DWORD dwReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}
