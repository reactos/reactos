/*
 * Stubs for unimplemented WIN32K.SYS exports
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include <include/error.h>

#define STUB(x) void x(void) { DbgPrint("WIN32K: Stub for %s\n", #x); }

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED DbgPrint("(%s:%i) WIN32K: %s UNIMPLEMENTED\n", __FILE__, __LINE__, __FUNCTION__ )
#endif//UNIMPLEMENTED

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
HDRVOBJ
STDCALL
EngCreateDriverObj(
	PVOID        pvObj,
	FREEOBJPROC  pFreeObjProc,
	HDEV         hdev
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8svb.htm
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
WNDOBJ*
STDCALL
EngCreateWnd(
	SURFOBJ          *pso,
	HWND              hwnd,
	WNDOBJCHANGEPROC  pfn,
	FLONG             fl,
	int               iPixelFormat
	)
{
  // www.osr.com/ddk/graphics/gdifncs_2ip3.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EngDeleteDriverObj(
	IN HDRVOBJ  hdo,
	IN BOOL  bCallBack,
	IN BOOL  bLocked
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0qlj.htm
  UNIMPLEMENTED;
  return FALSE;
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
VOID
STDCALL
EngDeleteWnd ( IN WNDOBJ *pwo )
{
  // www.osr.com/ddk/graphics/gdifncs_2z3b.htm
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
EngFindImageProcAddress(
	IN HANDLE  hModule,
	IN LPSTR   lpProcName
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0oiw.htm
  UNIMPLEMENTED;
  return NULL;
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
	OUT WCHAR* pDest
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
 * @unimplemented ( for NT4 only )
 */
HANDLE
STDCALL
EngGetProcessHandle ( VOID )
{
  // www.osr.com/ddk/graphics/gdifncs_3tif.htm
  // In Windows 2000 and later, the EngGetProcessHandle function always returns NULL.
  // FIXME - what does NT4 return?
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
DRIVEROBJ*
STDCALL
EngLockDriverObj ( IN HDRVOBJ hdo )
{
  // www.osr.com/ddk/graphics/gdifncs_41if.htm
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
EngRestoreFloatingPointState ( IN VOID *pBuffer )
{
  // www.osr.com/ddk/graphics/gdifncs_9l0n.htm
  UNIMPLEMENTED;
  return FALSE;
}

ULONG
STDCALL
EngSaveFloatingPointState(
	OUT VOID  *pBuffer,
	IN ULONG  cjBufferSize
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9tif.htm
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

VOID
STDCALL
EngUnloadImage ( IN HANDLE hModule )
{
  // www.osr.com/ddk/graphics/gdifncs_586f.htm
  UNIMPLEMENTED;
}

BOOL
STDCALL
EngUnlockDriverObj ( IN HDRVOBJ hdo )
{
  // www.osr.com/ddk/graphics/gdifncs_0l5z.htm
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

VOID
STDCALL
FLOATOBJ_Add (
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ      pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_2i3r.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_AddFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0ip3.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_AddLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_12jr.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_Div(
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3ndz.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_DivFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0gfb.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_DivLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6jdz.htm
  UNIMPLEMENTED;
}

BOOL
STDCALL
FLOATOBJ_Equal(
	IN PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6ysn.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_EqualLong(
	IN PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1pgn.htm
  UNIMPLEMENTED;
  return FALSE;
}

LONG
STDCALL
FLOATOBJ_GetFloat ( IN PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_4d5z.htm
  UNIMPLEMENTED;
  return 0;
}

LONG
STDCALL
FLOATOBJ_GetLong ( IN PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_0tgn.htm
  UNIMPLEMENTED;
  return 0;
}

BOOL
STDCALL
FLOATOBJ_GreaterThan(
	IN PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8n53.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_GreaterThanLong(
	IN PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6gx3.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_LessThan(
	IN PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1ynb.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_LessThanLong(
	IN PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9nzb.htm
  UNIMPLEMENTED;
  return FALSE;
}

VOID
STDCALL
FLOATOBJ_Mul(
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8ppj.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_MulFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3puv.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_MulLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_56lj.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_Neg ( IN OUT PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_14pz.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SetFloat(
	OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1prb.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SetLong(
	OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0gpz.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_Sub(
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6lyf.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SubFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_2zvr.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SubLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_852f.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
ULONG
STDCALL
FONTOBJ_cGetAllGlyphHandles (
	IN PFONTOBJ  FontObj,
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
	IN PFONTOBJ FontObj,
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
FONTOBJ_pifi ( IN PFONTOBJ FontObj )
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
	IN PFONTOBJ  FontObj,
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
FONTOBJ_pxoGetXform ( IN PFONTOBJ FontObj )
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
	IN  PFONTOBJ   FontObj,
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
WNDOBJ_bEnum(
	IN WNDOBJ  *pwo,
	IN ULONG  cj,
	OUT ULONG  *pul
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3jqf.htm
  UNIMPLEMENTED;
  return FALSE;
}

ULONG
STDCALL
WNDOBJ_cEnumStart(
	IN WNDOBJ  *pwo,
	IN ULONG  iType,
	IN ULONG  iDirection,
	IN ULONG  cLimit
	)
{
  // www.osr.com/ddk/graphics/gdifncs_18o7.htm
  UNIMPLEMENTED;
  return 0;
}

VOID
STDCALL
WNDOBJ_vSetConsumer(
	IN WNDOBJ  *pwo,
	IN PVOID  pvConsumer
	)
{
  // www.osr.com/ddk/graphics/gdifncs_484n.htm
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
