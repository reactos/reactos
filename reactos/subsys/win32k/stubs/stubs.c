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
HSEMAPHORE
STDCALL
EngCreateSemaphore ( VOID )
{
  // www.osr.com/ddk/graphics/gdifncs_95lz.htm
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngAcquireSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_14br.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_5u3r.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_13c7.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EngIsSemaphoreOwned ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_6wmf.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EngIsSemaphoreOwnedByCurrentThread ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_9yxz.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EngTextOut (
	IN SURFOBJ  *pso,
	IN STROBJ   *pstro,
	IN FONTOBJ  *pfo,
	IN CLIPOBJ  *pco,
	IN RECTL    *prclExtra,
	IN RECTL    *prclOpaque,
	IN BRUSHOBJ *pboFore,
	IN BRUSHOBJ *pboOpaque,
	IN POINTL    pptlOrg,
	IN MIX       mix
	)
{
  // www.osr.com/ddk/graphics/gdifncs_4tgn.htm
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
ULONG
STDCALL
BRUSHOBJ_ulGetBrushColor ( IN BRUSHOBJ  *pbo )
{
  // www.osr.com/ddk/graphics/gdifncs_0ch3.htm
  UNIMPLEMENTED;
  return 0;
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
 * @implemented
 */
ULONG
STDCALL
EngGetLastError ( VOID )
{
  // www.osr.com/ddk/graphics/gdifncs_3non.htm
  return GetLastNtError();
}

/*
 * @implemented
 */
VOID
STDCALL
EngSetLastError ( IN ULONG iError )
{
  // www.osr.com/ddk/graphics/gdifncs_95m0.htm
  SetLastNtError ( iError );
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
BOOL
STDCALL
EngMapFontFile(
	ULONG_PTR  iFile,
	PULONG    *ppjBuf,
	ULONG     *pcjBuf
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3up3.htm
  UNIMPLEMENTED;
  return FALSE;
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

/*
 * @unimplemented
 */
VOID
STDCALL
EngMovePointer(
	IN SURFOBJ  *pso,
	IN LONG      x,
	IN LONG      y,
	IN RECTL    *prcl
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8wfb.htm
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
FONTOBJ_cGetGlyphs(IN PFONTOBJ FontObj,
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
IFIMETRICS*
STDCALL
FONTOBJ_pifi(IN PFONTOBJ  FontObj)
{
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
PVOID
STDCALL
FONTOBJ_pvTrueTypeFontFile(IN PFONTOBJ  FontObj,
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
FONTOBJ_pxoGetXform(IN PFONTOBJ  FontObj)
{
  UNIMPLEMENTED;
  return NULL;
}

/*
 * @unimplemented
 */
VOID
STDCALL
FONTOBJ_vGetInfo(IN  PFONTOBJ   FontObj,
                 IN  ULONG      InfoSize,
                 OUT PFONTINFO  FontInfo)
{
  UNIMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

STUB(EngMultiByteToWideChar)
STUB(EngProbeForRead)
STUB(EngProbeForReadAndWrite)
STUB(EngQueryLocalTime)
STUB(EngQueryPalette)
STUB(EngRestoreFloatingPointState)
STUB(EngSaveFloatingPointState)
STUB(EngSetEvent)
STUB(EngSetPointerShape)
STUB(EngSetPointerTag)
STUB(EngSetPrinterData)
STUB(EngStretchBlt)
STUB(EngStrokeAndFillPath)
STUB(EngStrokePath)
STUB(EngUnloadImage)
STUB(EngUnlockDriverObj)
STUB(EngUnmapEvent)
STUB(EngUnmapFontFile)
STUB(EngWaitForSingleObject)
STUB(EngWideCharToMultiByte)
STUB(EngWritePrinter)
STUB(FLOATOBJ_Add)
STUB(FLOATOBJ_AddFloat)
STUB(FLOATOBJ_AddFloatObj)
STUB(FLOATOBJ_AddLong)
STUB(FLOATOBJ_Div)
STUB(FLOATOBJ_DivFloat)
STUB(FLOATOBJ_DivFloatObj)
STUB(FLOATOBJ_DivLong)
STUB(FLOATOBJ_Equal)
STUB(FLOATOBJ_EqualLong)
STUB(FLOATOBJ_GetFloat)
STUB(FLOATOBJ_GetLong)
STUB(FLOATOBJ_GreaterThan)
STUB(FLOATOBJ_GreaterThanLong)
STUB(FLOATOBJ_LessThan)
STUB(FLOATOBJ_LessThanLong)
STUB(FLOATOBJ_Mul)
STUB(FLOATOBJ_MulFloat)
STUB(FLOATOBJ_MulFloatObj)
STUB(FLOATOBJ_MulLong)
STUB(FLOATOBJ_Neg)
STUB(FLOATOBJ_SetFloat)
STUB(FLOATOBJ_SetLong)
STUB(FLOATOBJ_Sub)
STUB(FLOATOBJ_SubFloat)
STUB(FLOATOBJ_SubFloatObj)
STUB(FLOATOBJ_SubLong)
STUB(HT_ComputeRGBGammaTable)
STUB(HT_Get8BPPFormatPalette)
STUB(PATHOBJ_bCloseFigure)
STUB(PATHOBJ_bEnum)
STUB(PATHOBJ_bEnumClipLines)
STUB(PATHOBJ_bMoveTo)
STUB(PATHOBJ_bPolyBezierTo)
STUB(PATHOBJ_bPolyLineTo)
STUB(PATHOBJ_vEnumStart)
STUB(PATHOBJ_vEnumStartClipLines)
STUB(PATHOBJ_vGetBounds)
STUB(STROBJ_bEnum)
STUB(STROBJ_dwGetCodePage)
STUB(STROBJ_vEnumStart)
STUB(WNDOBJ_bEnum)
STUB(WNDOBJ_cEnumStart)
STUB(WNDOBJ_vSetConsumer)
STUB(XFORMOBJ_bApplyXform)
STUB(XFORMOBJ_iGetFloatObjXform)
STUB(XFORMOBJ_iGetXform)

