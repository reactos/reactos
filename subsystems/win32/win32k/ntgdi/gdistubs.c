/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdistubs.c
 * PURPOSE:         Syscall stubs
 * PROGRAMMERS:     Olaf Siejka
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/


BOOL
APIENTRY
NtGdiAbortDoc(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
NtGdiAbortPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
NtGdiFillPath(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
    return FALSE;
}



BOOL
APIENTRY
NtGdiStrokeAndFillPath(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
NtGdiStrokePath(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
NtGdiWidenPath(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
NtGdiFlattenPath(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
    return FALSE;
}


NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


HRGN
APIENTRY
NtGdiPathToRegion(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiSetMiterLimit(
    IN HDC hdc,
    IN DWORD dwNew,
    IN OUT OPTIONAL PDWORD pdwOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetFontXform(
    IN HDC hdc,
    IN DWORD dwxScale,
    IN DWORD dwyScale
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetMiterLimit(
    IN HDC hdc,
    OUT PDWORD pdwOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEllipse(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiRectangle(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiRoundRect(
    IN HDC hdc,
    IN INT x1,
    IN INT y1,
    IN INT x2,
    IN INT y2,
    IN INT x3,
    IN INT y3
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiPlgBlt(
    IN HDC hdcTrg,
    IN LPPOINT pptlTrg,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN HBITMAP hbmMask,
    IN INT xMask,
    IN INT yMask,
    IN DWORD crBackColor
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiMaskBlt(
    IN HDC hdc,
    IN INT xDst,
    IN INT yDst,
    IN INT cx,
    IN INT cy,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN HBITMAP hbmMask,
    IN INT xMask,
    IN INT yMask,
    IN DWORD dwRop4,
    IN DWORD crBackColor
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiExtFloodFill(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    IN COLORREF crColor,
    IN UINT iFillType
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiFillRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN HBRUSH hbrush
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiFrameRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN HBRUSH hbrush,
    IN INT xWidth,
    IN INT yHeight
)
{
    UNIMPLEMENTED;
	return FALSE;
}


COLORREF
APIENTRY
NtGdiSetPixel(
    IN HDC hdcDst,
    IN INT x,
    IN INT y,
    IN COLORREF crColor
)
{
    UNIMPLEMENTED;
	return 0;
}


DWORD
APIENTRY
NtGdiGetPixel(
    IN HDC hdc,
    IN INT x,
    IN INT y
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiStartPage(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEndPage(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiStartDoc(
    IN HDC hdc,
    IN DOCINFOW *pdi,
    OUT BOOL *pbBanding,
    IN INT iJob
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiEndDoc(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiUpdateColors(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetCharWidthW(
    IN HDC hdc,
    IN UINT wcFirst,
    IN UINT cwc,
    IN OPTIONAL PWCHAR pwc,
    IN FLONG fl,
    OUT PVOID pvBuf
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetCharWidthInfo(
    IN HDC hdc,
    OUT PCHWIDTHINFO pChWidthInfo
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiDrawEscape(
    IN HDC hdc,
    IN INT iEsc,
    IN INT cjIn,
    IN OPTIONAL LPSTR pjIn
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiExtEscape(
    IN HDC hdc,
    IN OPTIONAL PWCHAR pDriver,
    IN INT nDriver,
    IN INT iEsc,
    IN INT cjIn,
    IN OPTIONAL LPSTR pjIn,
    IN INT cjOut,
    OUT OPTIONAL LPSTR pjOut
)
{
    UNIMPLEMENTED;
	return 0;
}


ULONG
APIENTRY
NtGdiGetFontData(
    IN HDC hdc,
    IN DWORD dwTable,
    IN DWORD dwOffset,
    OUT OPTIONAL PVOID pvBuf,
    IN ULONG cjBuf
)
{
    UNIMPLEMENTED;
	return 0;
}


ULONG
APIENTRY
NtGdiGetGlyphOutline(
    IN HDC hdc,
    IN WCHAR wch,
    IN UINT iFormat,
    OUT LPGLYPHMETRICS pgm,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf,
    IN LPMAT2 pmat2,
    IN BOOL bIgnoreRotation
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetETM(
    IN HDC hdc,
    OUT EXTTEXTMETRIC *petm
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    OUT LPRASTERIZER_STATUS praststat,
    IN ULONG cjBytes
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG
APIENTRY
NtGdiGetKerningPairs(
    IN HDC hdc,
    IN ULONG cPairs,
    OUT OPTIONAL KERNINGPAIR *pkpDst
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiMonoBitmap(
    IN HBITMAP hbm
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    IN HBRUSH hbr,
    OUT UINT *piUsage
)
{
    UNIMPLEMENTED;
	return NULL;
}


ULONG
APIENTRY
NtGdiEnumObjects(
    IN HDC hdc,
    IN INT iObjectType,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiResetDC(
    IN HDC hdc,
    IN LPDEVMODEW pdm,
    OUT PBOOL pbBanding,
    IN OPTIONAL VOID *pDriverInfo2, // this is "typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;"
    OUT VOID *ppUMdhpdev
)
{
    UNIMPLEMENTED;
	return FALSE;
}


DWORD
APIENTRY
NtGdiSetBoundsRect(
    IN HDC hdc,
    IN LPRECT prc,
    IN DWORD f
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetColorAdjustment(
    IN HDC hdc,
    OUT PCOLORADJUSTMENT pcaOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetColorAdjustment(
    IN HDC hdc,
    IN PCOLORADJUSTMENT pca
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiCancelDC(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HDC
APIENTRY
NtGdiOpenDCW(
    IN OPTIONAL PUNICODE_STRING pustrDevice,
    IN DEVMODEW *pdm,  // See note for NtGdiResetDC
    IN PUNICODE_STRING pustrLogAddr,
    IN ULONG iType,
    IN OPTIONAL HANDLE hspool,
    IN OPTIONAL VOID *pDriverInfo2, // this is  "typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;"
    OUT VOID *pUMdhpdev
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiGetDCDword(
    IN HDC hdc,
    IN UINT u,
    OUT DWORD *Result
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetDCPoint(
    IN HDC hdc,
    IN UINT iPoint,
    OUT PPOINTL pptOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiScaleViewportExtEx(
    IN HDC hdc,
    IN INT xNum,
    IN INT xDenom,
    IN INT yNum,
    IN INT yDenom,
    OUT OPTIONAL LPSIZE pszOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiScaleWindowExtEx(
    IN HDC hdc,
    IN INT xNum,
    IN INT xDenom,
    IN INT yNum,
    IN INT yDenom,
    OUT OPTIONAL LPSIZE pszOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetVirtualResolution(
    IN HDC hdc,
    IN INT cxVirtualDevicePixel,
    IN INT cyVirtualDevicePixel,
    IN INT cxVirtualDeviceMm,
    IN INT cyVirtualDeviceMm
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetSizeDevice(
    IN HDC hdc,
    IN INT cxVirtualDevice,
    IN INT cyVirtualDevice
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetTransform(
    IN HDC hdc,
    IN DWORD iXform,
    OUT LPXFORM pxf
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiModifyWorldTransform(
    IN HDC hdc,
    IN OPTIONAL LPXFORM pxf,
    IN DWORD iXform
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiCombineTransform(
    OUT LPXFORM pxfDst,
    IN LPXFORM pxfSrc1,
    IN LPXFORM pxfSrc2
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiTransformPoints(
    IN HDC hdc,
    IN PPOINT pptIn,
    OUT PPOINT pptOut,
    IN INT c,
    IN INT iMode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


LONG
APIENTRY
NtGdiConvertMetafileRect(
    IN HDC hdc,
    IN OUT PRECTL prect
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiGetTextCharsetInfo(
    IN HDC hdc,
    OUT OPTIONAL LPFONTSIGNATURE lpSig,
    IN DWORD dwFlags
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiDoBanding(
    IN HDC hdc,
    IN BOOL bStart,
    OUT POINTL *pptl,
    OUT PSIZE pSize
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG
APIENTRY
NtGdiGetPerBandInfo(
    IN HDC hdc,
    IN OUT PERBANDINFO *ppbi
)
{
    UNIMPLEMENTED;
	return 0;
}


NTSTATUS
APIENTRY
NtGdiGetStats(
    IN HANDLE hProcess,
    IN INT iIndex,
    IN INT iPidType,
    OUT PVOID pResults,
    IN UINT cjResultSize
)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


BOOL
APIENTRY
NtGdiSetMagicColors(
    IN HDC hdc,
    IN PALETTEENTRY peMagic,
    IN ULONG Index
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HBRUSH
APIENTRY
NtGdiSelectBrush(
    IN HDC hdc,
    IN HBRUSH hbrush
)
{
    UNIMPLEMENTED;
	return NULL;
}


HPEN
APIENTRY
NtGdiSelectPen(
    IN HDC hdc,
    IN HPEN hpen
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBITMAP
APIENTRY
NtGdiSelectBitmap(
    IN HDC hdc,
    IN HBITMAP hbm
)
{
    UNIMPLEMENTED;
	return NULL;
}


HFONT
APIENTRY
NtGdiSelectFont(
    IN HDC hdc,
    IN HFONT hf
)
{
    UNIMPLEMENTED;
	return NULL;
}


INT
APIENTRY
NtGdiExtSelectClipRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN INT iMode
)
{
    UNIMPLEMENTED;
	return 0;
}


HPEN
APIENTRY
NtGdiCreatePen(
    IN INT iPenStyle,
    IN INT iPenWidth,
    IN COLORREF cr,
    IN HBRUSH hbr
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiBitBlt(
    IN HDC hdcDst,
    IN INT x,
    IN INT y,
    IN INT cx,
    IN INT cy,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN DWORD rop4,
    IN DWORD crBackColor,
    IN FLONG fl
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiTileBitBlt(
    IN HDC hdcDst,
    IN RECTL * prectDst,
    IN HDC hdcSrc,
    IN RECTL * prectSrc,
    IN POINTL * pptlOrigin,
    IN DWORD rop4,
    IN DWORD crBackColor
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiTransparentBlt(
    IN HDC hdcDst,
    IN INT xDst,
    IN INT yDst,
    IN INT cxDst,
    IN INT cyDst,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN COLORREF TransColor
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetTextExtent(
    IN HDC hdc,
    IN LPWSTR lpwsz,
    IN INT cwc,
    OUT LPSIZE psize,
    IN UINT flOpts
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetTextMetricsW(
    IN HDC hdc,
    OUT TMW_INTERNAL * ptm,
    IN ULONG cj
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiGetTextFaceW(
    IN HDC hdc,
    IN INT cChar,
    OUT OPTIONAL LPWSTR pszOut,
    IN BOOL bAliasName
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiExtTextOutW(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    IN UINT flOpts,
    IN OPTIONAL LPRECT prcl,
    IN LPWSTR pwsz,
    IN INT cwc,
    IN OPTIONAL LPINT pdx,
    IN DWORD dwCodePage
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiIntersectClipRect(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
)
{
    UNIMPLEMENTED;
	return 0;
}


HRGN
APIENTRY
NtGdiCreateRectRgn(
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiPatBlt(
    IN HDC hdcDst,
    IN INT x,
    IN INT y,
    IN INT cx,
    IN INT cy,
    IN DWORD rop4
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiPolyPatBlt(
    IN HDC hdc,
    IN DWORD rop4,
    IN PPOLYPATBLT pPoly,
    IN DWORD Count,
    IN DWORD Mode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiUnrealizeObject(
    IN HANDLE h
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HBITMAP
APIENTRY
NtGdiCreateCompatibleBitmap(
    IN HDC hdc,
    IN INT cx,
    IN INT cy
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiLineTo(
    IN HDC hdc,
    IN INT x,
    IN INT y
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiMoveTo(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    OUT OPTIONAL LPPOINT pptOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiExtGetObjectW(
    IN HANDLE h,
    IN INT cj,
    OUT OPTIONAL LPVOID pvOut
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiGetDeviceCaps(
    IN HDC hdc,
    IN INT i
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetDeviceCapsAll (
    IN HDC hdc,
    OUT PDEVCAPS pDevCaps
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiStretchBlt(
    IN HDC hdcDst,
    IN INT xDst,
    IN INT yDst,
    IN INT cxDst,
    IN INT cyDst,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN DWORD dwRop,
    IN DWORD dwBackColor
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetBrushOrg(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    OUT LPPOINT pptOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HBITMAP
APIENTRY
NtGdiCreateBitmap(
    IN INT cx,
    IN INT cy,
    IN UINT cPlanes,
    IN UINT cBPP,
    IN OPTIONAL LPBYTE pjInit
)
{
    UNIMPLEMENTED;
	return NULL;
}


HPALETTE
APIENTRY
NtGdiCreateHalftonePalette(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiRestoreDC(
    IN HDC hdc,
    IN INT iLevel
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiExcludeClipRect(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiSaveDC(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiCombineRgn(
    IN HRGN hrgnDst,
    IN HRGN hrgnSrc1,
    IN HRGN hrgnSrc2,
    IN INT iMode
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiSetRectRgn(
    IN HRGN hrgn,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
)
{
    UNIMPLEMENTED;
	return FALSE;
}


LONG
APIENTRY
NtGdiSetBitmapBits(
    IN HBITMAP hbm,
    IN ULONG cj,
    IN PBYTE pjInit
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiGetDIBitsInternal(
    IN HDC hdc,
    IN HBITMAP hbm,
    IN UINT iStartScan,
    IN UINT cScans,
    OUT OPTIONAL LPBYTE pBits,
    IN OUT LPBITMAPINFO pbmi,
    IN UINT iUsage,
    IN UINT cjMaxBits,
    IN UINT cjMaxInfo
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiOffsetRgn(
    IN HRGN hrgn,
    IN INT cx,
    IN INT cy
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiGetRandomRgn(
    IN HDC hDC,
    OUT HRGN hDest,
    IN INT iCode
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiGetRgnBox(
    IN HRGN hrgn,
    OUT LPRECT prcOut
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiRectInRegion(
    IN HRGN hrgn,
    IN OUT LPRECT prcl
)
{
    UNIMPLEMENTED;
	return FALSE;
}


DWORD
APIENTRY
NtGdiGetBoundsRect(
    IN HDC hdc,
    OUT LPRECT prc,
    IN DWORD f
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiPtInRegion(
    IN HRGN hrgn,
    IN INT x,
    IN INT y
)
{
    UNIMPLEMENTED;
	return FALSE;
}


COLORREF
APIENTRY
NtGdiGetNearestColor(
    IN HDC hdc,
    IN COLORREF cr
)
{
    UNIMPLEMENTED;
	return 0;
}


UINT
APIENTRY
NtGdiGetSystemPaletteUse(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return 0;
}


UINT
APIENTRY
NtGdiSetSystemPaletteUse(
    IN HDC hdc,
    IN UINT ui
)
{
    UNIMPLEMENTED;
	return 0;
}


DWORD
APIENTRY
NtGdiGetRegionData(
    IN HRGN hrgn,
    IN DWORD nCount,
    OUT OPTIONAL LPRGNDATA lpRgnData
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiInvertRgn(
    IN HDC hdc,
    IN HRGN hrgn
)
{
    UNIMPLEMENTED;
	return FALSE;
}

INT
APIENTRY
NtGdiAddFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN FLONG f,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv
)
{
    UNIMPLEMENTED;
	return 0;
}

HFONT
APIENTRY
NtGdiHfontCreate(
    IN ENUMLOGFONTEXDVW *pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG  fl,
    IN PVOID pvCliData
)
{
    UNIMPLEMENTED;
	return NULL;
}


ULONG
APIENTRY
NtGdiSetFontEnumeration(
    IN ULONG ulType
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiEnumFontClose(
    IN ULONG_PTR idEnum
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEnumFontChunk(
    IN HDC hdc,
    IN ULONG_PTR idEnum,
    IN ULONG cjEfdw,
    OUT ULONG *pcjEfdw,
    OUT PENUMFONTDATAW pefdw
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG_PTR
APIENTRY
NtGdiEnumFontOpen(
    IN HDC hdc,
    IN ULONG iEnumType,
    IN FLONG flWin31Compat,
    IN ULONG cwchMax,
    IN OPTIONAL LPWSTR pwszFaceName,
    IN ULONG lfCharSet,
    OUT ULONG *pulCount
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiQueryFonts(
    OUT PUNIVERSAL_FONT_ID pufiFontList,
    IN ULONG nBufferSize,
    OUT PLARGE_INTEGER pTimeStamp
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiConsoleTextOut(
    IN HDC hdc,
    IN POLYTEXTW *lpto,
    IN UINT nStrings,
    IN RECTL *prclBounds
)
{
    UNIMPLEMENTED;
	return FALSE;
}


NTSTATUS
APIENTRY
NtGdiFullscreenControl(
    IN FULLSCREENCONTROL FullscreenCommand,
    IN PVOID FullscreenInput,
    IN DWORD FullscreenInputLength,
    OUT PVOID FullscreenOutput,
    IN OUT PULONG FullscreenOutputLength
)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


DWORD
NtGdiGetCharSet(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiEnableEudc(
    IN BOOL Param1
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEudcLoadUnloadLink(
    IN OPTIONAL LPCWSTR pBaseFaceName,
    IN UINT cwcBaseFaceName,
    IN LPCWSTR pEudcFontPath,
    IN UINT cwcEudcFontPath,
    IN INT iPriority,
    IN INT iFontLinkType,
    IN BOOL bLoadLin
)
{
    UNIMPLEMENTED;
	return FALSE;
}


UINT
APIENTRY
NtGdiGetStringBitmapW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN UINT cwc,
    OUT BYTE *lpSB,
    IN UINT cj
)
{
    UNIMPLEMENTED;
	return 0;
}


ULONG
APIENTRY
NtGdiGetEudcTimeStampEx(
    IN OPTIONAL LPWSTR lpBaseFaceName,
    IN ULONG cwcBaseFaceName,
    IN BOOL bSystemTimeStamp
)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
NtGdiGetFontUnicodeRanges(
    IN HDC hdc,
    OUT OPTIONAL LPGLYPHSET pgs
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
NtGdiGetRealizationInfo(
    IN HDC hdc,
    OUT PREALIZATION_INFO pri,
    IN HFONT hf
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
NtGdiAddRemoteMMInstanceToDC(
    IN HDC hdc,
    IN DOWNLOADDESIGNVECTOR *pddv,
    IN ULONG cjDDV
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiUnloadPrinterDriver(
    IN LPWSTR pDriverName,
    IN ULONG cbDriverName
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngAssociateSurface(
    IN HSURF hsurf,
    IN HDEV hdev,
    IN FLONG flHooks
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngEraseSurface(
    IN SURFOBJ *pso,
    IN RECTL *prcl,
    IN ULONG iColor
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HBITMAP
APIENTRY
NtGdiEngCreateBitmap(
    IN SIZEL sizl,
    IN LONG lWidth,
    IN ULONG iFormat,
    IN FLONG fl,
    IN OPTIONAL PVOID pvBits
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiEngDeleteSurface(
    IN HSURF hsurf
)
{
    UNIMPLEMENTED;
	return FALSE;
}


SURFOBJ*
APIENTRY
NtGdiEngLockSurface(
    IN HSURF hsurf
)
{
    UNIMPLEMENTED;
	return NULL;
}


VOID
APIENTRY
NtGdiEngUnlockSurface(
    IN SURFOBJ *SurfObj
)
{
    UNIMPLEMENTED;
}


BOOL
APIENTRY
NtGdiEngMarkBandingSurface(
    IN HSURF hsurf
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HSURF
APIENTRY
NtGdiEngCreateDeviceSurface(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormatCompat
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBITMAP
APIENTRY
NtGdiEngCreateDeviceBitmap(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormatCompat
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiEngCopyBits(
    IN SURFOBJ *psoDst,
    IN SURFOBJ *psoSrc,
    IN OPTIONAL CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDst,
    IN POINTL *pptlSrc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngStretchBlt(
    IN SURFOBJ *psoDest,
    IN SURFOBJ *psoSrc,
    IN SURFOBJ *psoMask,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN COLORADJUSTMENT *pca,
    IN POINTL *pptlHTOrg,
    IN RECTL *prclDest,
    IN RECTL *prclSrc,
    IN POINTL *pptlMask,
    IN ULONG iMode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngBitBlt(
    IN SURFOBJ *psoDst,
    IN SURFOBJ *psoSrc,
    IN SURFOBJ *psoMask,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDst,
    IN POINTL *pptlSrc,
    IN POINTL *pptlMask,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrush,
    IN ROP4 rop4
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngPlgBlt(
    IN SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc,
    IN OPTIONAL SURFOBJ *psoMsk,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN COLORADJUSTMENT *pca,
    IN POINTL *pptlBrushOrg,
    IN POINTFIX *pptfxDest,
    IN RECTL *prclSrc,
    IN OPTIONAL POINTL *pptlMask,
    IN ULONG iMode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HPALETTE
APIENTRY
NtGdiEngCreatePalette(
    IN ULONG iMode,
    IN ULONG cColors,
    IN ULONG *pulColors,
    IN FLONG flRed,
    IN FLONG flGreen,
    IN FLONG flBlue
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiEngDeletePalette(
    IN HPALETTE hPal
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngStrokePath(
    IN SURFOBJ *pso,
    IN PATHOBJ *ppo,
    IN CLIPOBJ *pco,
    IN XFORMOBJ *pxo,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN LINEATTRS *plineattrs,
    IN MIX mix
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngFillPath(
    IN SURFOBJ *pso,
    IN PATHOBJ *ppo,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN MIX mix,
    IN FLONG flOptions
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngStrokeAndFillPath(
    IN SURFOBJ *pso,
    IN PATHOBJ *ppo,
    IN CLIPOBJ *pco,IN XFORMOBJ *pxo,
    IN BRUSHOBJ *pboStroke,
    IN LINEATTRS *plineattrs,
    IN BRUSHOBJ *pboFill,
    IN POINTL *pptlBrushOrg,
    IN MIX mix,
    IN FLONG flOptions
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngPaint(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN MIX mix
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngLineTo(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN LONG x1,
    IN LONG y1,
    IN LONG x2,
    IN LONG y2,
    IN RECTL *prclBounds,
    IN MIX mix
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngAlphaBlend(
    IN SURFOBJ *psoDest,
    IN SURFOBJ *psoSrc,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDest,
    IN RECTL *prclSrc,
    IN BLENDOBJ *pBlendObj
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngGradientFill(
    IN SURFOBJ *psoDest,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN TRIVERTEX *pVertex,
    IN ULONG nVertex,
    IN PVOID pMesh,
    IN ULONG nMesh,
    IN RECTL *prclExtents,
    IN POINTL *pptlDitherOrg,
    IN ULONG ulMode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngTransparentBlt(
    IN SURFOBJ *psoDst,
    IN SURFOBJ *psoSrc,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDst,
    IN RECTL *prclSrc,
    IN ULONG iTransColor,
    IN ULONG ulReserved
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngTextOut(
    IN SURFOBJ *pso,
    IN STROBJ *pstro,
    IN FONTOBJ *pfo,
    IN CLIPOBJ *pco,
    IN RECTL *prclExtra,
    IN RECTL *prclOpaque,
    IN BRUSHOBJ *pboFore,
    IN BRUSHOBJ *pboOpaque,
    IN POINTL *pptlOrg,
    IN MIX mix
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngStretchBltROP(
    IN SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc,
    IN SURFOBJ *psoMask,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN COLORADJUSTMENT *pca,
    IN POINTL *pptlBrushOrg,
    IN RECTL *prclTrg,
    IN RECTL *prclSrc,
    IN POINTL *pptlMask,
    IN ULONG iMode,
    IN BRUSHOBJ *pbo,
    IN ROP4 rop4
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(
    IN XLATEOBJ *pxlo,
    IN ULONG iPal,
    IN ULONG cPal,
    OUT ULONG *pPal
)
{
    UNIMPLEMENTED;
	return 0;
}


ULONG
APIENTRY
NtGdiCLIPOBJ_cEnumStart(
    IN CLIPOBJ *pco,
    IN BOOL bAll,
    IN ULONG iType,
    IN ULONG iDirection,
    IN ULONG cLimit
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiCLIPOBJ_bEnum(
    IN CLIPOBJ *pco,
    IN ULONG cj,
    OUT ULONG *pul
)
{
    UNIMPLEMENTED;
	return FALSE;
}


PATHOBJ*
APIENTRY
NtGdiCLIPOBJ_ppoGetPath(
    IN CLIPOBJ *pco
)
{
    UNIMPLEMENTED;
	return NULL;
}


CLIPOBJ*
APIENTRY
NtGdiEngCreateClip(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}


VOID
APIENTRY
NtGdiEngDeleteClip(
    IN CLIPOBJ*pco
)
{
    UNIMPLEMENTED;
}


PVOID
APIENTRY
NtGdiBRUSHOBJ_pvAllocRbrush(
    IN BRUSHOBJ *pbo,
    IN ULONG cj
)
{
    UNIMPLEMENTED;
	return NULL;
}


PVOID
APIENTRY
NtGdiBRUSHOBJ_pvGetRbrush(
    IN BRUSHOBJ *pbo
)
{
    UNIMPLEMENTED;
	return NULL;
}


ULONG
APIENTRY
NtGdiBRUSHOBJ_ulGetBrushColor(
    IN BRUSHOBJ *pbo
)
{
    UNIMPLEMENTED;
	return 0;
}


HANDLE
APIENTRY
NtGdiBRUSHOBJ_hGetColorTransform(
    IN BRUSHOBJ *pbo
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiXFORMOBJ_bApplyXform(
    IN XFORMOBJ *pxo,
    IN ULONG iMode,
    IN ULONG cPoints,
    IN  PVOID pvIn,
    OUT PVOID pvOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG
APIENTRY
NtGdiXFORMOBJ_iGetXform(
    IN XFORMOBJ *pxo,
    OUT OPTIONAL XFORML *pxform
)
{
    UNIMPLEMENTED;
	return 0;
}


VOID
APIENTRY
NtGdiFONTOBJ_vGetInfo(
    IN FONTOBJ *pfo,
    IN ULONG cjSize,
    OUT FONTINFO *pfi
)
{
    UNIMPLEMENTED;
}


ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(
    IN FONTOBJ *pfo,
    IN ULONG iMode,
    IN ULONG cGlyph,
    IN HGLYPH *phg,
    OUT PVOID *ppvGlyph
)
{
    UNIMPLEMENTED;
	return 0;
}


XFORMOBJ*
APIENTRY
NtGdiFONTOBJ_pxoGetXform(
    IN FONTOBJ *pfo
)
{
    UNIMPLEMENTED;
	return NULL;
}


IFIMETRICS*
APIENTRY
NtGdiFONTOBJ_pifi(
    IN FONTOBJ *pfo
)
{
    UNIMPLEMENTED;
	return NULL;
}


FD_GLYPHSET*
APIENTRY
NtGdiFONTOBJ_pfdg(
    IN FONTOBJ *pfo
)
{
    UNIMPLEMENTED;
	return NULL;
}


ULONG
APIENTRY
NtGdiFONTOBJ_cGetAllGlyphHandles(
    IN FONTOBJ *pfo,
    OUT OPTIONAL HGLYPH *phg
)
{
    UNIMPLEMENTED;
	return 0;
}


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


PFD_GLYPHATTR
APIENTRY
NtGdiFONTOBJ_pQueryGlyphAttrs(
    IN FONTOBJ *pfo,
    IN ULONG iMode
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiSTROBJ_bEnum(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSTROBJ_bEnumPositionsOnly(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos
)
{
    UNIMPLEMENTED;
	return FALSE;
}


VOID
APIENTRY
NtGdiSTROBJ_vEnumStart(
    IN STROBJ *pstro
)
{
    UNIMPLEMENTED;
}


DWORD
APIENTRY
NtGdiSTROBJ_dwGetCodePage(
    IN STROBJ *pstro
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiSTROBJ_bGetAdvanceWidths(
    IN STROBJ*pstro,
    IN ULONG iFirst,
    IN ULONG c,
    OUT POINTQF*pptqD
)
{
    UNIMPLEMENTED;
	return FALSE;
}


FD_GLYPHSET*
APIENTRY
NtGdiEngComputeGlyphSet(
    IN INT nCodePage,
    IN INT nFirstChar,
    IN INT cChars
)
{
    UNIMPLEMENTED;
	return NULL;
}


ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(
    IN XLATEOBJ *pxlo,
    IN ULONG iColor
)
{
    UNIMPLEMENTED;
	return 0;
}


HANDLE
APIENTRY
NtGdiXLATEOBJ_hGetColorTransform(
    IN XLATEOBJ *pxlo
)
{
    UNIMPLEMENTED;
	return NULL;
}


VOID
APIENTRY
NtGdiPATHOBJ_vGetBounds(
    IN PATHOBJ *ppo,
    OUT PRECTFX prectfx
)
{
    UNIMPLEMENTED;
}


BOOL
APIENTRY
NtGdiPATHOBJ_bEnum(
    IN PATHOBJ *ppo,
    OUT PATHDATA *ppd
)
{
    UNIMPLEMENTED;
	return FALSE;
}


VOID
APIENTRY
NtGdiPATHOBJ_vEnumStart(
    IN PATHOBJ *ppo
)
{
    UNIMPLEMENTED;
}


VOID
APIENTRY
NtGdiEngDeletePath(
    IN PATHOBJ *ppo
)
{
    UNIMPLEMENTED;
}


VOID
APIENTRY
NtGdiPATHOBJ_vEnumStartClipLines(
    IN PATHOBJ *ppo,
    IN CLIPOBJ *pco,
    IN SURFOBJ *pso,
    IN LINEATTRS *pla
)
{
    UNIMPLEMENTED;
}


BOOL
APIENTRY
NtGdiPATHOBJ_bEnumClipLines(
    IN PATHOBJ *ppo,
    IN ULONG cb,
    OUT CLIPLINE *pcl
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEngCheckAbort(
    IN SURFOBJ *pso
)
{
    UNIMPLEMENTED;
	return FALSE;
}


DHPDEV
NtGdiGetDhpdev(
    IN HDEV hdev
)
{
    UNIMPLEMENTED;
	return FALSE;
}


LONG
APIENTRY
NtGdiHT_Get8BPPFormatPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma
)
{
    UNIMPLEMENTED;
	return 0;
}


LONG
APIENTRY
NtGdiHT_Get8BPPMaskPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    IN BOOL Use8BPPMaskPal,
    IN BYTE CMYMask,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
NtGdiUpdateTransform(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


DWORD
APIENTRY
NtGdiSetLayout(
    IN HDC hdc,
    IN LONG wox,
    IN DWORD dwLayout
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiMirrorWindowOrg(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


LONG
APIENTRY
NtGdiGetDeviceWidth(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
NtGdiSetPUMPDOBJ(
    IN HUMPD humpd,
    IN BOOL bStoreID,
    OUT HUMPD *phumpd,
    OUT BOOL *pbWOW64
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
NtGdiBRUSHOBJ_DeleteRbrush(
    IN BRUSHOBJ *pbo,
    IN BRUSHOBJ *pboB
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
NtGdiUMPDEngFreeUserMem(
    IN KERNEL_PVOID *ppv
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HBITMAP
APIENTRY
NtGdiSetBitmapAttributes(
    IN HBITMAP hbm,
    IN DWORD dwFlags
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBITMAP
APIENTRY
NtGdiClearBitmapAttributes(
    IN HBITMAP hbm,
    IN DWORD dwFlags
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    IN HBRUSH hbm,
    IN DWORD dwFlags
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    IN HBRUSH hbm,
    IN DWORD dwFlags
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiDrawStream(
    IN HDC hdcDst,
    IN ULONG cjIn,
    IN VOID *pvIn
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiMakeObjectXferable(
    IN HANDLE h,
    IN DWORD dwProcessId
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiMakeObjectUnXferable(
    IN HANDLE h
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiInitSpool()
{
    UNIMPLEMENTED;
	return FALSE;
}

/* FIXME wrong prototypes */

INT
APIENTRY
NtGdiGetSpoolMessage( DWORD u1,
                      DWORD u2,
                      DWORD u3,
                      DWORD u4)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiSetDIBitsToDeviceInternal(
    IN HDC hdcDest,
    IN INT xDst,
    IN INT yDst,
    IN DWORD cx,
    IN DWORD cy,
    IN INT xSrc,
    IN INT ySrc,
    IN DWORD iStartScan,
    IN DWORD cNumScan,
    IN LPBYTE pInitBits,
    IN LPBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN UINT cjMaxBits,
    IN UINT cjMaxInfo,
    IN BOOL bTransformCoordinates,
    IN OPTIONAL HANDLE hcmXform
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetFontResourceInfoInternalW(
    IN LPWSTR pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN UINT cjIn,
    OUT LPDWORD pdwBytes,
    OUT LPVOID pvBuf,
    IN DWORD iType
)
{
    UNIMPLEMENTED;
	return FALSE;
}


DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    IN HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD pgi,
    IN DWORD iMode
)
{
    UNIMPLEMENTED;
	return 0;
}


DWORD
APIENTRY
NtGdiGetGlyphIndicesWInternal(
    IN HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD pgi,
    IN DWORD iMode,
    IN BOOL bSubset
)
{
    UNIMPLEMENTED;
	return 0;
}


HPALETTE
APIENTRY
NtGdiCreatePaletteInternal(
    IN LPLOGPALETTE pLogPal,
    IN UINT cEntries
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiArcInternal(
    IN ARCTYPE arctype,
    IN HDC hdc,
    IN INT x1,
    IN INT y1,
    IN INT x2,
    IN INT y2,
    IN INT x3,
    IN INT y3,
    IN INT x4,
    IN INT y4
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
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
    IN HANDLE hcmXform
)
{
    UNIMPLEMENTED;
	return 0;
}


ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(
    IN HDC hdc,
    IN ULONG cjotm,
    OUT OPTIONAL OUTLINETEXTMETRICW *potmw,
    OUT TMDIFF *ptmd
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetAndSetDCDword(
    IN HDC hdc,
    IN UINT u,
    IN DWORD dwIn,
    OUT DWORD *pdwResult
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HANDLE
APIENTRY
NtGdiGetDCObject(
    IN  HDC hdc,
    IN  INT itype
)
{
    UNIMPLEMENTED;
	return NULL;
}


HDC
APIENTRY
NtGdiGetDCforBitmap(
    IN HBITMAP hsurf
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiGetMonitorID(
    IN  HDC hdc,
    IN  DWORD dwSize,
    OUT LPWSTR pszMonitorID
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiGetLinkedUFIs(
    IN HDC hdc,
    OUT OPTIONAL PUNIVERSAL_FONT_ID pufiLinkedUFIs,
    IN INT BufferSize
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiSetLinkedUFIs(
    IN HDC hdc,
    IN PUNIVERSAL_FONT_ID pufiLinks,
    IN ULONG uNumUFIs
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetUFI(
    IN  HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG *pfl
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiForceUFIMapping(
    IN HDC hdc,
    IN PUNIVERSAL_FONT_ID pufi
)
{
    UNIMPLEMENTED;
	return FALSE;
}


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
    OUT OPTIONAL ULONG *piTTC
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiAddRemoteFontToDC(
    IN HDC hdc,
    IN PVOID pvBuffer,
    IN ULONG cjBuffer,
    IN OPTIONAL PUNIVERSAL_FONT_ID pufi
)
{
    UNIMPLEMENTED;
	return FALSE;
}


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


BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(
    IN HANDLE hMMFont
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiUnmapMemFont(
    IN PVOID pvView
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiRemoveMergeFont(
    IN HDC hdc,
    IN UNIVERSAL_FONT_ID *pufi
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiAnyLinkedFonts(
    VOID
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetEmbUFI(
    IN HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG  *pfl,
    OUT KERNEL_PVOID *embFontID
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG
APIENTRY
NtGdiGetEmbedFonts(
    VOID
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiChangeGhostFont(
    IN KERNEL_PVOID *pfontID,
    IN BOOL bLoad
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiAddEmbFontToDC(
    IN HDC hdc,
    IN VOID **pFontID
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiFontIsLinked(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG_PTR
APIENTRY
NtGdiPolyPolyDraw(
    IN HDC hdc,
    IN PPOINT ppt,
    IN PULONG pcpt,
    IN ULONG ccpt,
    IN INT iFunc
)
{
    UNIMPLEMENTED;
	return 0;
}


LONG
APIENTRY
NtGdiDoPalette(
    IN HGDIOBJ hObj,
    IN WORD iStart,
    IN WORD cEntries,
    IN LPVOID pEntries,
    IN DWORD iFunc,
    IN BOOL bInbound
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiComputeXformCoefficients(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetWidthTable(
    IN HDC hdc,
    IN ULONG cSpecial,
    IN WCHAR *pwc,
    IN ULONG cwc,
    OUT USHORT *psWidth,
    OUT OPTIONAL WIDTHDATA *pwd,
    OUT FLONG *pflInfo
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiDescribePixelFormat(
    IN HDC hdc,
    IN INT ipfd,
    IN UINT cjpfd,
    OUT PPIXELFORMATDESCRIPTOR ppfd
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiSetPixelFormat(
    IN HDC hdc,
    IN INT ipfd
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSwapBuffers(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiSetupPublicCFONT(
    IN HDC hdc,
    IN OPTIONAL HFONT hf,
    IN ULONG ulAve
)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
APIENTRY
NtGdiAlphaBlend(
    IN HDC hdcDst,
    IN LONG DstX,
    IN LONG DstY,
    IN LONG DstCx,
    IN LONG DstCy,
    IN HDC hdcSrc,
    IN LONG SrcX,
    IN LONG SrcY,
    IN LONG SrcCx,
    IN LONG SrcCy,
    IN BLENDFUNCTION BlendFunction,
    IN HANDLE hcmXform
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGradientFill(
    IN HDC hdc,
    IN PTRIVERTEX pVertex,
    IN ULONG nVertex,
    IN PVOID pMesh,
    IN ULONG nMesh,
    IN ULONG ulMode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetIcmMode(
    IN HDC hdc,
    IN ULONG nCommand,
    IN ULONG ulMode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HANDLE
APIENTRY
NtGdiCreateColorSpace(
    IN PLOGCOLORSPACEEXW pLogColorSpace
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiDeleteColorSpace(
    IN HANDLE hColorSpace
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetColorSpace(
    IN HDC hdc,
    IN HCOLORSPACE hColorSpace
)
{
    UNIMPLEMENTED;
	return FALSE;
}


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
    IN ULONG cjTargetProfile
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiDeleteColorTransform(
    IN HDC hdc,
    IN HANDLE hColorTransform
)
{
    UNIMPLEMENTED;
	return FALSE;
}


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
    OUT PBYTE paResults
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG
APIENTRY
NtGdiColorCorrectPalette(
    IN HDC hdc,
    IN HPALETTE hpal,
    IN ULONG FirstEntry,
    IN ULONG NumberOfEntries,
    IN OUT PALETTEENTRY *ppalEntry,
    IN ULONG Command
)
{
    UNIMPLEMENTED;
	return 0;
}


ULONG_PTR
APIENTRY
NtGdiGetColorSpaceforBitmap(
    IN HBITMAP hsurf
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetDeviceGammaRamp(
    IN HDC hdc,
    OUT LPVOID lpGammaRamp
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetDeviceGammaRamp(
    IN HDC hdc,
    IN LPVOID lpGammaRamp
)
{
    UNIMPLEMENTED;
	return FALSE;
}


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
    IN ULONG Command
)
{
    UNIMPLEMENTED;
	return FALSE;
}


VOID
APIENTRY
NtGdiFlush(VOID)
{
    UNIMPLEMENTED;
}


HDC
APIENTRY
NtGdiCreateMetafileDC(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiMakeInfoDC(
    IN HDC hdc,
    IN BOOL bSet
)
{
    UNIMPLEMENTED;
	return FALSE;
}


HANDLE
APIENTRY
NtGdiCreateClientObj(
    IN ULONG ulType
)
{
    UNIMPLEMENTED;
	return NULL;
}


BOOL
APIENTRY
NtGdiDeleteClientObj(
    IN HANDLE h
)
{
    UNIMPLEMENTED;
	return FALSE;
}


LONG
APIENTRY
NtGdiGetBitmapBits(
    IN HBITMAP hbm,
    IN ULONG cjMax,
    OUT OPTIONAL PBYTE pjOut
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiDeleteObjectApp(
    IN HANDLE hobj
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiGetPath(
    IN HDC hdc,
    OUT OPTIONAL LPPOINT pptlBuf,
    OUT OPTIONAL LPBYTE pjTypes,
    IN INT cptBuf
)
{
    UNIMPLEMENTED;
	return 0;
}


HDC
APIENTRY
NtGdiCreateCompatibleDC(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBITMAP
APIENTRY
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
    IN HANDLE hcmXform
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBITMAP
APIENTRY
NtGdiCreateDIBSection(
    IN HDC hdc,
    IN OPTIONAL HANDLE hSectionApp,
    IN DWORD dwOffset,
    IN LPBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN UINT cjHeader,
    IN FLONG fl,
    IN ULONG_PTR dwColorSpace,
    OUT PVOID *ppvBits
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBRUSH
APIENTRY
NtGdiCreateSolidBrush(
    IN COLORREF cr,
    IN OPTIONAL HBRUSH hbr
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBRUSH
APIENTRY
NtGdiCreateDIBBrush(
    IN PVOID pv,
    IN FLONG fl,
    IN UINT  cj,
    IN BOOL  b8X8,
    IN BOOL bPen,
    IN PVOID pClient
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(
    IN HBITMAP hbm,
    IN BOOL bPen,
    IN BOOL b8X8
)
{
    UNIMPLEMENTED;
	return NULL;
}


HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(
    IN ULONG ulStyle,
    IN COLORREF clrr,
    IN BOOL bPen
)
{
    UNIMPLEMENTED;
	return NULL;
}


HPEN
APIENTRY
NtGdiExtCreatePen(
    IN ULONG flPenStyle,
    IN ULONG ulWidth,
    IN ULONG iBrushStyle,
    IN ULONG ulColor,
    IN ULONG_PTR lClientHatch,
    IN ULONG_PTR lHatch,
    IN ULONG cstyle,
    IN OPTIONAL PULONG pulStyle,
    IN ULONG cjDIB,
    IN BOOL bOldStylePen,
    IN OPTIONAL HBRUSH hbrush
)
{
    UNIMPLEMENTED;
	return NULL;
}


HRGN
APIENTRY
NtGdiCreateEllipticRgn(
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
)
{
    UNIMPLEMENTED;
	return NULL;
}


HRGN
APIENTRY
NtGdiCreateRoundRectRgn(
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom,
    IN INT xWidth,
    IN INT yHeight
)
{
    UNIMPLEMENTED;
	return NULL;
}


HANDLE
APIENTRY
NtGdiCreateServerMetaFile(
    IN DWORD iType,
    IN ULONG cjData,
    IN LPBYTE pjData,
    IN DWORD mm,
    IN DWORD xExt,
    IN DWORD yExt
)
{
    UNIMPLEMENTED;
	return NULL;
}


HRGN
APIENTRY
NtGdiExtCreateRegion(
    IN OPTIONAL LPXFORM px,
    IN DWORD cj,
    IN LPRGNDATA prgn
)
{
    UNIMPLEMENTED;
	return NULL;
}


ULONG
APIENTRY
NtGdiMakeFontDir(
    IN FLONG flEmbed,
    OUT PBYTE pjFontDir,
    IN unsigned cjFontDir,
    IN LPWSTR pwszPathname,
    IN unsigned cjPathname
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiPolyDraw(
    IN HDC hdc,
    IN LPPOINT ppt,
    IN LPBYTE pjAttr,
    IN ULONG cpt
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiPolyTextOutW(
    IN HDC hdc,
    IN POLYTEXTW *pptw,
    IN UINT cStr,
    IN DWORD dwCodePage
)
{
    UNIMPLEMENTED;
	return FALSE;
}


ULONG
APIENTRY
NtGdiGetServerMetaFileBits(
    IN HANDLE hmo,
    IN ULONG cjData,
    OUT OPTIONAL LPBYTE pjData,
    OUT PDWORD piType,
    OUT PDWORD pmm,
    OUT PDWORD pxExt,
    OUT PDWORD pyExt
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiEqualRgn(
    IN HRGN hrgn1,
    IN HRGN hrgn2
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetBitmapDimension(
    IN HBITMAP hbm,
    OUT LPSIZE psize
)
{
    UNIMPLEMENTED;
	return FALSE;
}


UINT
APIENTRY
NtGdiGetNearestPaletteIndex(
    IN HPALETTE hpal,
    IN COLORREF crColor
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiPtVisible(
    IN HDC hdc,
    IN INT x,
    IN INT y
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiRectVisible(
    IN HDC hdc,
    IN LPRECT prc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN ULONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiResizePalette(
    IN HPALETTE hpal,
    IN UINT cEntry
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSetBitmapDimension(
    IN HBITMAP hbm,
    IN INT cx,
    IN INT cy,
    OUT OPTIONAL LPSIZE psizeOut
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiOffsetClipRgn(
    IN HDC hdc,
    IN INT x,
    IN INT y
)
{
    UNIMPLEMENTED;
	return 0;
}


INT
APIENTRY
NtGdiSetMetaRgn(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiSetTextJustification(
    IN HDC hdc,
    IN INT lBreakExtra,
    IN INT cBreak
)
{
    UNIMPLEMENTED;
	return FALSE;
}


INT
APIENTRY
NtGdiGetAppClipBox(
    IN HDC hdc,
    OUT LPRECT prc
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiGetTextExtentExW(
    IN HDC hdc,
    IN OPTIONAL LPWSTR lpwsz,
    IN ULONG cwc,
    IN ULONG dxMax,
    OUT OPTIONAL ULONG *pcCh,
    OUT OPTIONAL PULONG pdxOut,
    OUT LPSIZE psize,
    IN FLONG fl
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    IN HDC hdc,
    IN UINT wchFirst,
    IN ULONG cwch,
    IN OPTIONAL PWCHAR pwch,
    IN FLONG fl,
    OUT PVOID pvBuf
)
{
    UNIMPLEMENTED;
	return FALSE;
}


DWORD
APIENTRY
NtGdiGetCharacterPlacementW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN INT nCount,
    IN INT nMaxExtent,
    IN OUT LPGCP_RESULTSW pgcpw,
    IN DWORD dwFlags
)
{
    UNIMPLEMENTED;
	return 0;
}


BOOL
APIENTRY
NtGdiAngleArc(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    IN DWORD dwRadius,
    IN DWORD dwStartAngle,
    IN DWORD dwSweepAngle
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiBeginPath(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiSelectClipPath(
    IN HDC hdc,
    IN INT iMode
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiCloseFigure(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL
APIENTRY
NtGdiEndPath(
    IN HDC hdc
)
{
    UNIMPLEMENTED;
	return FALSE;
}
