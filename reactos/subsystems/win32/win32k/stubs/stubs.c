/*
 * Stubs for unimplemented WIN32K.SYS exports
 */

#include <w32k.h>

#define STUB(x) void x(void) { DbgPrint("WIN32K: Stub for %s\n", #x); }
#define UNIMPLEMENTED DbgPrint("(%s:%i) WIN32K: %s UNIMPLEMENTED\n", __FILE__, __LINE__, __FUNCTION__ )




/*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
EngUnmapFontFileFD ( IN ULONG_PTR iFile )
{
  // http://www.osr.com/ddk/graphics/gdifncs_6wbr.htm
  UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOL
STDCALL
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
STDCALL
EngUnmapFontFile ( ULONG_PTR iFile )
{
  // www.osr.com/ddk/graphics/gdifncs_09wn.htm
  return EngUnmapFontFileFD ( iFile );
}

/*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
EngDeletePath ( IN PATHOBJ *ppo )
{
  // www.osr.com/ddk/graphics/gdifncs_3fl3.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
EngFreeModule ( IN HANDLE h )
{
  // www.osr.com/ddk/graphics/gdifncs_9fzb.htm
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
LPWSTR
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
EngMarkBandingSurface ( IN HSURF hsurf )
{
  // www.osr.com/ddk/graphics/gdifncs_2jon.htm
  UNIMPLEMENTED;
  return FALSE;
}

INT
STDCALL
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
STDCALL
EngQueryLocalTime ( OUT PENG_TIME_FIELDS ptf )
{
  // www.osr.com/ddk/graphics/gdifncs_389z.htm
  UNIMPLEMENTED;
}

ULONG
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
FONTOBJ_pifi ( IN FONTOBJ *FontObj )
{
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
PVOID
STDCALL
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
STDCALL
FONTOBJ_pxoGetXform ( IN FONTOBJ *FontObj )
{
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
VOID
STDCALL
FONTOBJ_vGetInfo (
	IN  FONTOBJ   *FontObj,
	IN  ULONG      InfoSize,
	OUT PFONTINFO  FontInfo)
{
  UNIMPLEMENTED;
}

LONG
STDCALL
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
STDCALL
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
STDCALL
PATHOBJ_bCloseFigure ( IN PATHOBJ *ppo )
{
  // www.osr.com/ddk/graphics/gdifncs_5mhz.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
PATHOBJ_vEnumStart ( IN PATHOBJ *ppo )
{
  // www.osr.com/ddk/graphics/gdifncs_74br.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
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
STDCALL
PATHOBJ_vGetBounds(
	IN PATHOBJ  *ppo,
	OUT PRECTFX  prectfx
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8qp3.htm
  UNIMPLEMENTED;
}

BOOL
STDCALL
STROBJ_bEnum(
	IN STROBJ  *pstro,
	OUT ULONG  *pc,
	OUT PGLYPHPOS  *ppgpos
	)
{
  // www.osr.com/ddk/graphics/gdifncs_65uv.htm
  UNIMPLEMENTED;
  return FALSE;
}

DWORD
STDCALL
STROBJ_dwGetCodePage ( IN STROBJ *pstro )
{
  // www.osr.com/ddk/graphics/gdifncs_9jmv.htm
  UNIMPLEMENTED;
  return 0;
}

VOID
STDCALL
STROBJ_vEnumStart ( IN STROBJ *pstro )
{
  // www.osr.com/ddk/graphics/gdifncs_32uf.htm
  UNIMPLEMENTED;
}

BOOL
STDCALL
XFORMOBJ_bApplyXform(
	IN XFORMOBJ  *pxo,
	IN ULONG  iMode,
	IN ULONG  cPoints,
	IN PVOID  pvIn,
	OUT PVOID  pvOut
	)
{
  // www.osr.com/ddk/graphics/gdifncs_027b.htm
  UNIMPLEMENTED;
  return FALSE;
}

ULONG
STDCALL
XFORMOBJ_iGetFloatObjXform(
	IN XFORMOBJ  *pxo,
	OUT FLOATOBJ_XFORM  *pxfo
	)
{
  // www.osr.com/ddk/graphics/gdifncs_5ig7.htm
  UNIMPLEMENTED;
  return 0;
}

ULONG
STDCALL
XFORMOBJ_iGetXform(
	IN XFORMOBJ  *pxo,
	OUT XFORML  *pxform
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0s2v.htm
  UNIMPLEMENTED;
  return 0;
}

// below here aren't in DDK!!!

STUB(FLOATOBJ_AddFloatObj)
STUB(FLOATOBJ_DivFloatObj)
STUB(FLOATOBJ_MulFloatObj)
STUB(FLOATOBJ_SubFloatObj)

/*
 * @unimplemented
 */
ULONG STDCALL
EngDitherColor(
   IN HDEV hdev,
   IN ULONG iMode,
   IN ULONG rgb,
   OUT ULONG *pul)
{
   return DCR_SOLID;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngQuerySystemAttribute(
   IN ENG_SYSTEM_ATTRIBUTE CapNum,
   OUT PDWORD pCapability)
{
   switch (CapNum)
   {
      case EngNumberOfProcessors:
         *pCapability = 1;
         return TRUE;

      case EngProcessorFeature:
         *pCapability = 0;
         return TRUE;

      default:
         break;
   }

   return FALSE;
}

/*
 * @unimplemented
 */
FLATPTR STDCALL
HeapVidMemAllocAligned(
   IN LPVIDMEM lpVidMem,
   IN DWORD dwWidth,
   IN DWORD dwHeight,
   IN LPSURFACEALIGNMENT lpAlignment,
   OUT LPLONG lpNewPitch)
{
   UNIMPLEMENTED;
   return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
VidMemFree(
   IN LPVMEMHEAP pvmh,
   IN FLATPTR ptr)
{
   UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
BRUSHOBJ_hGetColorTransform(
   IN BRUSHOBJ *Brush)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PVOID STDCALL
EngAllocPrivateUserMem(
   IN PDD_SURFACE_LOCAL  psl,
   IN SIZE_T  cj,
   IN ULONG  tag)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngClearEvent(
   IN PEVENT Event)
{
   UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngDeleteFile(
   IN LPWSTR FileName)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngFreePrivateUserMem(
   IN PDD_SURFACE_LOCAL  psl,
   IN PVOID  pv)
{
   UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL STDCALL
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
ULONG STDCALL
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
PDD_SURFACE_LOCAL STDCALL
EngLockDirectDrawSurface(
   IN HANDLE Surface)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngLpkInstalled()
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
PVOID STDCALL
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
BOOL STDCALL
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
BOOL STDCALL
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
LONG STDCALL
EngReadStateEvent(
   IN PEVENT Event)
{
   UNIMPLEMENTED;
   return 0;
}
BOOL STDCALL
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
BOOL STDCALL
EngUnlockDirectDrawSurface(
   IN PDD_SURFACE_LOCAL Surface)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngUnmapFile(
   IN ULONG_PTR File)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
FD_GLYPHSET * STDCALL
FONTOBJ_pfdg(
   IN FONTOBJ *FontObj)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PBYTE STDCALL
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
PFD_GLYPHATTR STDCALL
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
LPWSTR STDCALL
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
LONG STDCALL
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
BOOL STDCALL
STROBJ_bEnumPositionsOnly(
   IN STROBJ *StringObj,
   OUT ULONG *Count,
   OUT PGLYPHPOS *Pos)
{
   UNIMPLEMENTED;
   return (BOOL) DDI_ERROR;
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bGetAdvanceWidths(
   IN STROBJ *StringObj,
   IN ULONG First,
   IN ULONG Count,
   OUT POINTQF *Widths)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
FIX STDCALL
STROBJ_fxBreakExtra(
   IN STROBJ *StringObj)
{
   UNIMPLEMENTED;
   return (FIX) 0;
}

/*
 * @unimplemented
 */
FIX STDCALL
STROBJ_fxCharacterExtra(
   IN STROBJ *StringObj)
{
   UNIMPLEMENTED;
   return (FIX) 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
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
STDCALL
NtGdiAnyLinkedFonts()
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
NtGdiBRUSHOBJ_hGetColorTransform(
   IN BRUSHOBJ *Brush)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PVOID STDCALL
NtGdiBRUSHOBJ_pvAllocRbrush(IN BRUSHOBJ *BrushObj,
                            IN ULONG ObjSize)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
PVOID STDCALL
NtGdiBRUSHOBJ_pvGetRbrush(IN BRUSHOBJ *BrushObj)
{
   UNIMPLEMENTED;
   return NULL;
}

/*
 * @unimplemented
 */
ULONG STDCALL
NtGdiBRUSHOBJ_ulGetBrushColor(BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
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
ULONG STDCALL
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
PATHOBJ* STDCALL
NtGdiCLIPOBJ_ppoGetPath(CLIPOBJ *ClipObj)
{
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiEnableEudc(BOOL enable)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
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
CLIPOBJ* STDCALL
NtGdiEngCreateClip(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
VOID STDCALL
NtGdiEngDeleteClip(CLIPOBJ *ClipRegion)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtGdiEngCheckAbort(SURFOBJ *pso)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
HSURF STDCALL
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
HBITMAP STDCALL
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
BOOL STDCALL
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
 * @unimplemented
 */
HBITMAP STDCALL
NtGdiEngCreateBitmap(IN SIZEL Size,
		IN LONG Width,
		IN ULONG Format,
		IN ULONG Flags,
		IN PVOID Bits)
{
     UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
HPALETTE STDCALL
NtGdiEngCreatePalette(IN ULONG Mode,
		 IN ULONG NumColors,
		 IN ULONG *Colors,
		 IN ULONG Red,
		 IN ULONG Green,
		 IN ULONG Blue)
{
     UNIMPLEMENTED;
    return NULL;
}

BOOL STDCALL
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
BOOL STDCALL
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
BOOL STDCALL
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
 * @unimplemented
 */
BOOL STDCALL
NtGdiEngDeletePalette(IN HPALETTE Palette)
{
     UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL
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
VOID STDCALL
NtGdiEngDeletePath(PATHOBJ *ppo)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL STDCALL
NtGdiEngDeleteSurface(IN HSURF Surface)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL
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
BOOL STDCALL
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

BOOL STDCALL
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

BOOL STDCALL
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
BOOL STDCALL
NtGdiEngMarkBandingSurface(HSURF hsurf)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
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
BOOL STDCALL
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
BOOL STDCALL
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

FD_GLYPHSET* STDCALL
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
STDCALL
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
STDCALL
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
INT
STDCALL
NtGdiAddFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN FLONG f,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{
    UNIMPLEMENTED;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiFontIsLinked(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
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
HANDLE
STDCALL
NtGdiCreateServerMetaFile(
    IN DWORD iType,
    IN ULONG cjData,
    IN LPBYTE pjData,
    IN DWORD mm,
    IN DWORD xExt,
    IN DWORD yExt)
{
    UNIMPLEMENTED;
    return NULL;
}


 /*
 * @unimplemented
 */

HDC
STDCALL
NtGdiCreateMetafileDC(IN HDC hdc)
{
    UNIMPLEMENTED;
    return NULL;
}


 /*
 * @unimplemented
 */

HBITMAP
STDCALL
NtGdiCreateDIBitmapInternal(
    IN HDC hdc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL LPBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN UINT cjMaxInitInfo,
    IN UINT cjMaxBits,
    IN FLONG f,
    IN HANDLE hcmXform)
{
    UNIMPLEMENTED;
    return NULL;
}


 /*
 * @unimplemented
 */
HBITMAP
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiFONTOBJ_pfdg(IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
PVOID
STDCALL
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
STDCALL
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
STDCALL
NtGdiFONTOBJ_pxoGetXform(IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpCanCreateVideoPort(
    IN HANDLE hDirectDraw,
    IN OUT PDD_CANCREATEVPORTDATA puCanCreateVPortData)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpColorControl(
    IN HANDLE hVideoPort,
    IN OUT PDD_VPORTCOLORDATA puVPortColorData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
HANDLE
STDCALL
NtGdiDvpCreateVideoPort(
    IN HANDLE hDirectDraw,
    IN OUT PDD_CREATEVPORTDATA puCreateVPortData)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpDestroyVideoPort(
    IN HANDLE hVideoPort,
    IN OUT PDD_DESTROYVPORTDATA puDestroyVPortData)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpFlipVideoPort(
    IN HANDLE hVideoPort,
    IN HANDLE hDDSurfaceCurrent,
    IN HANDLE hDDSurfaceTarget,
    IN OUT PDD_FLIPVPORTDATA puFlipVPortData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpGetVideoPortBandwidth(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
HBRUSH
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiComputeXformCoefficients(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
BOOL
STDCALL
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
LONG
STDCALL
NtGdiConvertMetafileRect(
    IN HDC hdc,
    IN OUT PRECTL prect)
{
    UNIMPLEMENTED;
    return 0;
}




 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpGetVideoPortFlipStatus(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpGetVideoPortInputFormats(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpGetVideoPortLine(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTLINEDATA puGetVPortLineData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpGetVideoPortOutputFormats(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpGetVideoPortConnectInfo(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTCONNECTDATA puGetVPortConnectData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpGetVideoSignalStatus(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTSIGNALDATA puGetVPortSignalData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpUpdateVideoPort(
    IN HANDLE hVideoPort,
    IN HANDLE* phSurfaceVideo,
    IN HANDLE* phSurfaceVbi,
    IN OUT PDD_UPDATEVPORTDATA puUpdateVPortData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpWaitForVideoPortSync(
    IN HANDLE hVideoPort,
    IN OUT PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpAcquireNotification(
    IN HANDLE hVideoPort,
    IN OUT HANDLE* hEvent,
    IN LPDDVIDEOPORTNOTIFY pNotify)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDvpReleaseNotification(
    IN HANDLE hVideoPort,
    IN HANDLE hEvent)
{
    UNIMPLEMENTED;
    return 0;
}



 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiDxgGenericThunk(
    IN ULONG_PTR ulIndex,
    IN ULONG_PTR ulHandle,
    IN OUT SIZE_T *pdwSizeOfPtr1,
    IN OUT  PVOID pvPtr1,
    IN OUT SIZE_T *pdwSizeOfPtr2,
    IN OUT  PVOID pvPtr2)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiExtTextOutW(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    IN UINT flOpts,
    IN OPTIONAL LPRECT prcl,
    IN LPWSTR pwsz,
    IN INT cwc,
    IN OPTIONAL LPINT pdx,
    IN DWORD dwCodePage)
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
INT
STDCALL
NtGdiGetAppClipBox(
    IN HDC hdc,
    OUT LPRECT prc)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiGetCharABCWidthsW(
    IN HDC hdc,
    IN UINT wchFirst,
    IN ULONG cwch,
    IN OPTIONAL PWCHAR pwch,
    IN FLONG fl,
    OUT PVOID pvBuf)
 {
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
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
STDCALL
NtGdiGetCharWidthW(
    IN HDC hdc,
    IN UINT wcFirst,
    IN UINT cwc,
    IN OPTIONAL PWCHAR pwc,
    IN FLONG fl,
    OUT PVOID pvBuf)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
NtGdiGetColorSpaceforBitmap(
    IN HBITMAP hsurf)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
HDC
STDCALL
NtGdiGetDCforBitmap(
    IN HBITMAP hsurf)
{
    UNIMPLEMENTED;
    return NULL;
}


 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiGetDeviceCapsAll (
    IN HDC hdc,
    OUT PDEVCAPS pDevCaps)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
 DWORD
STDCALL
NtGdiDvpGetVideoPortField(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTFIELDDATA puGetVPortFieldData)
{
    UNIMPLEMENTED;
    return 0;
}


 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiInitSpool()
{
    UNIMPLEMENTED;
    return FALSE;
}


 /*
 * @unimplemented
 */
INT
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
NtGdiGetRealizationInfo(
    IN HDC hdc,
    OUT PREALIZATION_INFO pri,
    IN HFONT hf)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiQueryFontAssocInfo(
    IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
DWORD
NtGdiGetFontUnicodeRanges(
    IN HDC hdc,
    OUT OPTIONAL LPGLYPHSET pgs)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
UINT
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiPATHOBJ_vEnumStart(
    IN PATHOBJ *ppo)
{
    UNIMPLEMENTED;
}

 /*
 * @unimplemented
 */
VOID
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
ULONG
STDCALL
NtGdiGetServerMetaFileBits(
    IN HANDLE hmo,
    IN ULONG cjData,
    OUT OPTIONAL LPBYTE pjData,
    OUT PDWORD piType,
    OUT PDWORD pmm,
    OUT PDWORD pxExt,
    OUT PDWORD pyExt)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
NTSTATUS
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
BOOL
STDCALL
NtGdiSetSizeDevice(
    IN HDC hdc,
    IN INT cxVirtualDevice,
    IN INT cyVirtualDevice)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiSetVirtualResolution(
    IN HDC hdc,
    IN INT cxVirtualDevicePixel,
    IN INT cyVirtualDevicePixel,
    IN INT cxVirtualDeviceMm,
    IN INT cyVirtualDeviceMm)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
INT
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiSTROBJ_vEnumStart(
    IN STROBJ *pstro)
{
    UNIMPLEMENTED;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiSTROBJ_dwGetCodePage(
    IN STROBJ *pstro)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
DHPDEV
NtGdiGetDhpdev(
    IN HDEV hdev)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
NtGdiInit()
{
    return TRUE;
}

 /*
 * @unimplemented
 */
ULONG
STDCALL
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
STDCALL
NtGdiMakeInfoDC(
    IN HDC hdc,
    IN BOOL bSet)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiMonoBitmap(
    IN HBITMAP hbm)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiMoveTo(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    OUT OPTIONAL LPPOINT pptOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
HBITMAP
STDCALL
NtGdiSelectBitmap(
    IN HDC hdc,
    IN HBITMAP hbm)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
HBRUSH
STDCALL
NtGdiSelectBrush(
    IN HDC hdc,
    IN HBRUSH hbrush)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
HFONT
STDCALL
NtGdiSelectFont(
    IN HDC hdc,
    IN HFONT hf)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
HPEN
STDCALL
NtGdiSelectPen(
    IN HDC hdc,
    IN HPEN hpen)
{
    UNIMPLEMENTED;
    return NULL;
}

 /*
 * @unimplemented
 */
HBITMAP
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
LONG
STDCALL
NtGdiGetDeviceWidth(
    IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiMirrorWindowOrg(
    IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
DWORD
STDCALL
NtGdiSetLayout(
    IN HDC hdc,
    IN LONG wox,
    IN DWORD dwLayout)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
INT
STDCALL
NtGdiStretchDIBitsInternal(
    IN HDC hdc,
    IN INT xDst,
    IN INT yDst,
    IN INT cxDst,
    IN INT cyDst,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN OPTIONAL LPBYTE pjInit,
    IN LPBITMAPINFO pbmi,
    IN DWORD dwUsage,
    IN DWORD dwRop4,
    IN UINT cjMaxInfo,
    IN UINT cjMaxBits,
    IN HANDLE hcmXform)
{
    UNIMPLEMENTED;
    return 0;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiUnloadPrinterDriver(
    IN LPWSTR pDriverName,
    IN ULONG cbDriverName)
{
    UNIMPLEMENTED;
    return FALSE;
}

 /*
 * @unimplemented
 */
BOOL
STDCALL
NtGdiUnmapMemFont(
    IN PVOID pvView)
{
    UNIMPLEMENTED;
    return FALSE;
}

