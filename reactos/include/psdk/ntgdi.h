/*
 * NtGdi Entrypoints
 */
#pragma once
#ifndef _NTGDI_
#define _NTGDI_

#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

#ifndef _WINDOWBLT_NOTIFICATION_
#define _WINDOWBLT_NOTIFICATION_
#endif

#ifdef  COMBOX_SANDBOX
#define DX_LONGHORN_PRESERVEDC
#endif

#define TRACE_SURFACE_ALLOCS        (DBG || 0)

/* NtGdiGetLinkedUfis */
#define FL_UFI_PRIVATEFONT          1
#define FL_UFI_DESIGNVECTOR_PFF     2
#define FL_UFI_MEMORYFONT           4

/* NtGdiSetIcmMode */
#define ICM_SET_MODE                1
#define ICM_SET_CALIBRATE_MODE      2
#define ICM_SET_COLOR_MODE          3
#define ICM_CHECK_COLOR_MODE        4

/* NtGdiCreateColorSpace */
#define LCSEX_ANSICREATED           1
#define LCSEX_TEMPPROFILE           2

/* NtGdiGetStats */
#define GS_NUM_OBJS_ALL             0
#define GS_HANDOBJ_CURRENT          1
#define GS_HANDOBJ_MAX              2
#define GS_HANDOBJ_ALLOC            3
#define GS_LOOKASIDE_INFO           4

/* NtGdiEnumFontOpen */
#define TYPE_ENUMFONTS              1
#define TYPE_ENUMFONTFAMILIES       2
#define TYPE_ENUMFONTFAMILIESEX     3

typedef enum _COLORPALETTEINFO
{
    ColorPaletteQuery,
    ColorPaletteSet
} COLORPALETTEINFO, *PCOLORPALETTEINFO;

/* NtGdiIcmBrushInfo */
typedef enum _ICM_DIB_INFO_CMD
{
    IcmQueryBrush,
    IcmSetBrush
} ICM_DIB_INFO, *PICM_DIB_INFO;

/* NtGdiCreateColorSpace */
typedef struct _LOGCOLORSPACEEXW
{
    LOGCOLORSPACEW lcsColorSpace;
    DWORD dwFlags;
} LOGCOLORSPACEEXW, *PLOGCOLORSPACEEXW;

typedef struct _POLYPATBLT
{
  INT nXLeft;
  INT nYLeft;
  INT nWidth;
  INT nHeight;
  HBRUSH hBrush;
} POLYPATBLT, *PPOLYPATBLT;

/* NtGdiAddRemoteMMInstanceToDC */
typedef struct tagDOWNLOADDESIGNVECTOR
{
    UNIVERSAL_FONT_ID ufiBase;
    DESIGNVECTOR dv;
} DOWNLOADDESIGNVECTOR;

/* NtGdiResetDC */
typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;

#if 0
typedef struct _HLSURF_INFORMATION_PROBE {
    union {
        HLSURF_INFORMATION_SURFACE       Surface;
        HLSURF_INFORMATION_PRESENTFLAGS  PresentFlags;
        HLSURF_INFORMATION_TOKENUPDATEID UpdateId;
        HLSURF_INFORMATION_SET_SIGNALING SetSignaling;
        DWMSURFACEDATA                   SurfaceData;
        HLSURF_INFORMATION_DIRTYREGIONS  DirtyRegions;
        HLSURF_INFORMATION_REDIRSTYLE    RedirStyle;
        HLSURF_INFORMATION_SET_GERNERATE_MOVE_DATA SetGenerateMoveData;
    } u;
} HLSURF_INFORMATION_PROBE, *PHLSURF_INFORMATION_PROBE;
#endif // 0

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiInit(
    VOID);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiSetDIBitsToDeviceInternal(
    _In_ HDC hdcDest,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ DWORD cx,
    _In_ DWORD cy,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ DWORD iStartScan,
    _In_ DWORD cNumScan,
    _In_reads_bytes_(cjMaxBits) LPBYTE pInitBits,
    _In_reads_bytes_(cjMaxInfo) LPBITMAPINFO pbmi,
    _In_ DWORD iUsage,
    _In_ UINT cjMaxBits,
    _In_ UINT cjMaxInfo,
    _In_ BOOL bTransformCoordinates,
    _In_opt_ HANDLE hcmXform
);

#if WINVER >= 0x601
__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiCreateSessionMappedDIBSection(
    _In_opt_ HDC hdc,
    _In_opt_ HANDLE hSectionApp,
    _In_ DWORD dwOffset,
    _In_reads_bytes_opt_(cjHeader) LPBITMAPINFO pbmi,
    _In_ DWORD iUsage,
    _In_ UINT cjHeader,
    _In_ FLONG fl,
    _In_ ULONG_PTR dwColorSpace);
#endif

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetFontResourceInfoInternalW(
    _In_reads_z_(cwc) LPWSTR pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ UINT cjBuf,
    _Out_ LPDWORD pdwBytes,
    _Out_writes_bytes_(cjBuf) LPVOID pvBuf,
    _In_ DWORD iType);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) LPWSTR pwc,
    _In_ INT cwc,
    _Out_writes_opt_(cwc) LPWORD pgi,
    _In_ DWORD iMode);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesWInternal(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) LPWSTR pwc,
    _In_ INT cwc,
    _Out_writes_opt_(cwc) LPWORD pgi,
    _In_ DWORD iMode,
    _In_ BOOL bSubset);

__kernel_entry
W32KAPI
HPALETTE
APIENTRY
NtGdiCreatePaletteInternal(
    _In_reads_bytes_(cEntries * 4 + 4) LPLOGPALETTE pLogPal,
    _In_ UINT cEntries);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiArcInternal(
    _In_ ARCTYPE arctype,
    _In_ HDC hdc,
    _In_ INT x1,
    _In_ INT y1,
    _In_ INT x2,
    _In_ INT y2,
    _In_ INT x3,
    _In_ INT y3,
    _In_ INT x4,
    _In_ INT y4);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiStretchDIBitsInternal(
    _In_ HDC hdc,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ INT cxDst,
    _In_ INT cyDst,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_reads_bytes_opt_(cjMaxBits) LPBYTE pjInit,
    _In_ LPBITMAPINFO pbmi,
    _In_ DWORD dwUsage,
    _In_ DWORD dwRop4,
    _In_ UINT cjMaxInfo,
    _In_ UINT cjMaxBits,
    _In_opt_ HANDLE hcmXform);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(
    _In_ HDC hdc,
    _In_ ULONG cjotm,
    _Out_writes_bytes_opt_(cjotm) OUTLINETEXTMETRICW *potmw,
    _Out_ TMDIFF *ptmd);

_Success_(return != FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetAndSetDCDword(
    _In_ HDC hdc,
    _In_ UINT u,
    _In_ DWORD dwIn,
    _Out_ DWORD *pdwResult);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiGetDCObject(
    _In_ HDC hdc,
    _In_ INT itype);

__kernel_entry
W32KAPI
HDC
APIENTRY
NtGdiGetDCforBitmap(
    _In_ HBITMAP hsurf);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetMonitorID(
    _In_ HDC hdc,
    _In_ DWORD cjSize,
    _Out_writes_bytes_(cjSize) LPWSTR pszMonitorID);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetLinkedUFIs(
    _In_ HDC hdc,
    _Out_writes_opt_(cBufferSize) PUNIVERSAL_FONT_ID pufiLinkedUFIs,
    _In_ INT cBufferSize);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetLinkedUFIs(
    _In_ HDC hdc,
     _In_reads_(uNumUFIs) PUNIVERSAL_FONT_ID pufiLinks,
    _In_ ULONG uNumUFIs);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetUFI(
    _In_ HDC hdc,
    _Out_ PUNIVERSAL_FONT_ID pufi,
    _Out_opt_ DESIGNVECTOR *pdv,
    _Out_ ULONG *pcjDV,
    _Out_ ULONG *pulBaseCheckSum,
    _Out_ FLONG *pfl);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiForceUFIMapping(
    _In_ HDC hdc,
    _In_ PUNIVERSAL_FONT_ID pufi);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetUFIPathname(
    _In_ PUNIVERSAL_FONT_ID pufi,
    _Deref_out_range_(0, MAX_PATH * 3) ULONG* pcwc,
    _Out_writes_to_opt_(MAX_PATH * 3, *pcwc) LPWSTR pwszPathname,
    _Out_opt_ ULONG* pcNumFiles,
    _In_ FLONG fl,
    _Out_opt_ BOOL *pbMemFont,
    _Out_opt_ ULONG *pcjView,
    _Out_opt_ PVOID pvView,
    _Out_opt_ BOOL *pbTTC,
    _Out_opt_ ULONG *piTTC);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAddRemoteFontToDC(
    _In_ HDC hdc,
    _In_reads_bytes_(cjBuffer) PVOID pvBuffer,
    _In_ ULONG cjBuffer,
    _In_opt_ PUNIVERSAL_FONT_ID pufi);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiAddFontMemResourceEx(
    _In_reads_bytes_(cjBuffer) PVOID pvBuffer,
    _In_ DWORD cjBuffer,
    _In_reads_bytes_opt_(cjDV) DESIGNVECTOR *pdv,
    _In_ ULONG cjDV,
    _Out_ DWORD *pNumFonts);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(
    _In_ HANDLE hMMFont);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiUnmapMemFont(
    _In_ PVOID pvView);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRemoveMergeFont(
    _In_ HDC hdc,
    _In_ UNIVERSAL_FONT_ID *pufi);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAnyLinkedFonts(
    VOID);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetEmbUFI(
    _In_ HDC hdc,
    _Out_ PUNIVERSAL_FONT_ID pufi,
    _Out_opt_ DESIGNVECTOR *pdv,
    _Out_ ULONG *pcjDV,
    _Out_ ULONG *pulBaseCheckSum,
    _Out_ FLONG  *pfl,
    _Out_ KERNEL_PVOID *embFontID);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetEmbedFonts(
    VOID);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiChangeGhostFont(
    _In_ KERNEL_PVOID *pfontID,
    _In_ BOOL bLoad);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAddEmbFontToDC(
    _In_ HDC hdc,
    _In_ PVOID *pFontID);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiFontIsLinked(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
ULONG_PTR
APIENTRY
NtGdiPolyPolyDraw(
    _In_ HDC hdc,
    _In_ PPOINT ppt,
    _In_reads_(ccpt) PULONG pcpt,
    _In_ ULONG ccpt,
    _In_ INT iFunc);

__kernel_entry
W32KAPI
LONG
APIENTRY
NtGdiDoPalette(
    _In_ HGDIOBJ hObj,
    _In_ WORD iStart,
    _In_ WORD cEntries,
    _When_((iFunc == GdiPalGetEntries) || (iFunc == GdiPalGetSystemEntries), _Out_writes_bytes_(cEntries*sizeof(PALETTEENTRY)))
    _When_((iFunc != GdiPalGetEntries) && (iFunc != GdiPalGetSystemEntries), _In_reads_bytes_(cEntries*sizeof(PALETTEENTRY))) LPVOID pEntries,
    _In_ DWORD iFunc,
    _In_ BOOL bInbound);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiComputeXformCoefficients(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetWidthTable(
    _In_ HDC hdc,
    _In_ ULONG cSpecial,
    _In_reads_(cwc) WCHAR *pwc,
    _In_ ULONG cwc,
    _Out_writes_(cwc) USHORT *psWidth,
    _Out_opt_ WIDTHDATA *pwd,
    _Out_ FLONG *pflInfo);

_Success_(return != 0)
__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiDescribePixelFormat(
    _In_ HDC hdc,
    _In_ INT ipfd,
    _In_ UINT cjpfd,
    _Out_writes_bytes_(cjpfd) PPIXELFORMATDESCRIPTOR ppfd);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetPixelFormat(
    _In_ HDC hdc,
    _In_ INT ipfd);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSwapBuffers(
    _In_ HDC hdc);

/* Not in MS ntgdi.h */
__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiSetupPublicCFONT(
    _In_ HDC hdc,
    _In_opt_ HFONT hf,
    _In_ ULONG ulAve);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDxgGenericThunk(
    _In_ ULONG_PTR ulIndex,
    _In_ ULONG_PTR ulHandle,
    _Inout_ SIZE_T *pdwSizeOfPtr1,
    _Inout_  PVOID pvPtr1,
    _Inout_ SIZE_T *pdwSizeOfPtr2,
    _Inout_  PVOID pvPtr2);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdAddAttachedSurface(
    _In_ HANDLE hSurface,
    _In_ HANDLE hSurfaceAttached,
    _Inout_ PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdAttachSurface(
    _In_ HANDLE hSurfaceFrom,
    _In_ HANDLE hSurfaceTo);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdBlt(
    _In_ HANDLE hSurfaceDest,
    _In_ HANDLE hSurfaceSrc,
    _Inout_ PDD_BLTDATA puBltData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdCanCreateSurface(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdColorControl(
    _In_ HANDLE hSurface,
    _Inout_ PDD_COLORCONTROLDATA puColorControlData);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateDirectDrawObject(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdCreateSurface(
    _In_ HANDLE hDirectDraw,
    _In_ HANDLE* hSurface,
    _Inout_ DDSURFACEDESC* puSurfaceDescription,
    _Inout_ DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    _Inout_ DD_SURFACE_LOCAL* puSurfaceLocalData,
    _Inout_ DD_SURFACE_MORE* puSurfaceMoreData,
    _Inout_ DD_CREATESURFACEDATA* puCreateSurfaceData,
    _Out_ HANDLE* puhSurface);

#ifdef DX_LONGHORN_PRESERVEDC
__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdChangeSurfacePointer(
    _In_ HANDLE hSurface,
    _In_ PVOID pSurfacePointer);
#endif /* DX_LONGHORN_PRESERVEDC */

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateSurfaceObject(
    _In_ HANDLE hDirectDrawLocal,
    _In_ HANDLE hSurface,
    _In_ PDD_SURFACE_LOCAL puSurfaceLocal,
    _In_ PDD_SURFACE_MORE puSurfaceMore,
    _In_ PDD_SURFACE_GLOBAL puSurfaceGlobal,
    _In_ BOOL bComplete);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteSurfaceObject(
    _In_ HANDLE hSurface);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteDirectDrawObject(
    _In_ HANDLE hDirectDrawLocal);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdDestroySurface(
    _In_ HANDLE hSurface,
    _In_ BOOL bRealDestroy);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdFlip(
    _In_ HANDLE hSurfaceCurrent,
    _In_ HANDLE hSurfaceTarget,
    _In_ HANDLE hSurfaceCurrentLeft,
    _In_ HANDLE hSurfaceTargetLeft,
    _Inout_ PDD_FLIPDATA puFlipData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetAvailDriverMemory(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetBltStatus(
    _In_ HANDLE hSurface,
    _Inout_ PDD_GETBLTSTATUSDATA puGetBltStatusData);

__kernel_entry
W32KAPI
HDC
APIENTRY
NtGdiDdGetDC(
    _In_ HANDLE hSurface,
    _In_ PALETTEENTRY* puColorTable);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetDriverInfo(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETDRIVERINFODATA puGetDriverInfoData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetFlipStatus(
    _In_ HANDLE hSurface,
    _Inout_ PDD_GETFLIPSTATUSDATA puGetFlipStatusData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetScanLine(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETSCANLINEDATA puGetScanLineData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdSetExclusiveMode(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdFlipToGDISurface(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdLock(
    _In_ HANDLE hSurface,
    _Inout_ PDD_LOCKDATA puLockData,
    _In_ HDC hdcClip);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdQueryDirectDrawObject(
    _In_ HANDLE hDirectDrawLocal,
    _Out_ PDD_HALINFO pHalInfo,
    _Out_writes_(3) DWORD* pCallBackFlags,
    _Out_opt_ LPD3DNTHAL_CALLBACKS puD3dCallbacks,
    _Out_opt_ LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
    _Out_opt_ PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
    _Out_opt_ LPDDSURFACEDESC puD3dTextureFormats,
    _Out_ DWORD* puNumHeaps,
    _Out_opt_ VIDEOMEMORY* puvmList,
    _Out_ DWORD* puNumFourCC,
    _Out_opt_ DWORD* puFourCC);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdReenableDirectDrawObject(
    _In_ HANDLE hDirectDrawLocal,
    _Inout_ BOOL* pubNewMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdReleaseDC(
    _In_ HANDLE hSurface);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdResetVisrgn(
    _In_ HANDLE hSurface,
    _In_ HWND hwnd);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdSetColorKey(
    _In_ HANDLE hSurface,
    _Inout_ PDD_SETCOLORKEYDATA puSetColorKeyData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdSetOverlayPosition(
    _In_ HANDLE hSurfaceSource,
    _In_ HANDLE hSurfaceDestination,
    _Inout_ PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiDdUnattachSurface(
    _In_ HANDLE hSurface,
    _In_ HANDLE hSurfaceAttached);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdUnlock(
    _In_ HANDLE hSurface,
    _Inout_ PDD_UNLOCKDATA puUnlockData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdUpdateOverlay(
    _In_ HANDLE hSurfaceDestination,
    _In_ HANDLE hSurfaceSource,
    _Inout_ PDD_UPDATEOVERLAYDATA puUpdateOverlayData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdWaitForVerticalBlank(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiDdGetDxHandle(
    _In_opt_ HANDLE hDirectDraw,
    _In_opt_ HANDLE hSurface,
    _In_ BOOL bRelease);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDdSetGammaRamp(
    _In_ HANDLE hDirectDraw,
    _In_ HDC hdc,
    _In_reads_bytes_(sizeof(GAMMARAMP)) LPVOID lpGammaRamp);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdLockD3D(
    _In_ HANDLE hSurface,
    _Inout_ PDD_LOCKDATA puLockData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdUnlockD3D(
    _In_ HANDLE hSurface,
    _Inout_ PDD_UNLOCKDATA puUnlockData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdCreateD3DBuffer(
    _In_ HANDLE hDirectDraw,
    _Inout_ HANDLE* hSurface,
    _Inout_ DDSURFACEDESC* puSurfaceDescription,
    _Inout_ DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    _Inout_ DD_SURFACE_LOCAL* puSurfaceLocalData,
    _Inout_ DD_SURFACE_MORE* puSurfaceMoreData,
    _Inout_ DD_CREATESURFACEDATA* puCreateSurfaceData,
    _Inout_ HANDLE* puhSurface);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdCanCreateD3DBuffer(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdDestroyD3DBuffer(
    _In_ HANDLE hSurface);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiD3dContextCreate(
    _In_ HANDLE hDirectDrawLocal,
    _In_ HANDLE hSurfColor,
    _In_ HANDLE hSurfZ,
    _Inout_ D3DNTHAL_CONTEXTCREATEI *pdcci);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiD3dContextDestroy(
    _In_ LPD3DNTHAL_CONTEXTDESTROYDATA pdcdd);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiD3dContextDestroyAll(
    _Out_ LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcdad);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiD3dValidateTextureStageState(
    _Inout_ LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiD3dDrawPrimitives2(
    _In_ HANDLE hCmdBuf,
    _In_ HANDLE hVBuf,
    _Inout_ LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    _Inout_ FLATPTR* pfpVidMemCmd,
    _Inout_ DWORD* pdwSizeCmd,
    _Inout_ FLATPTR* pfpVidMemVtx,
    _Inout_ DWORD* pdwSizeVtx);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetDriverState(
    _Inout_ PDD_GETDRIVERSTATEDATA pdata);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdCreateSurfaceEx(
    _In_ HANDLE hDirectDraw,
    _In_ HANDLE hSurface,
    _In_ DWORD dwSurfaceHandle);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpCanCreateVideoPort(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_CANCREATEVPORTDATA puCanCreateVPortData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpColorControl(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_VPORTCOLORDATA puVPortColorData);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiDvpCreateVideoPort(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_CREATEVPORTDATA puCreateVPortData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpDestroyVideoPort(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_DESTROYVPORTDATA puDestroyVPortData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpFlipVideoPort(
    _In_ HANDLE hVideoPort,
    _In_ HANDLE hDDSurfaceCurrent,
    _In_ HANDLE hDDSurfaceTarget,
    _Inout_ PDD_FLIPVPORTDATA puFlipVPortData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortBandwidth(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortField(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_GETVPORTFIELDDATA puGetVPortFieldData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortFlipStatus(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortInputFormats(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortLine(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_GETVPORTLINEDATA puGetVPortLineData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortOutputFormats(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortConnectInfo(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETVPORTCONNECTDATA puGetVPortConnectData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoSignalStatus(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_GETVPORTSIGNALDATA puGetVPortSignalData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpUpdateVideoPort(
    _In_ HANDLE hVideoPort,
    _In_ HANDLE* phSurfaceVideo,
    _In_ HANDLE* phSurfaceVbi,
    _Inout_ PDD_UPDATEVPORTDATA puUpdateVPortData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpWaitForVideoPortSync(
    _In_ HANDLE hVideoPort,
    _Inout_ PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpAcquireNotification(
    _In_ HANDLE hVideoPort,
    _Inout_ HANDLE* hEvent,
    _In_ LPDDVIDEOPORTNOTIFY pNotify);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDvpReleaseNotification(
    _In_ HANDLE hVideoPort,
    _In_ HANDLE hEvent);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompGuids(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompFormats(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompBuffInfo(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdGetInternalMoCompInfo(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_GETINTERNALMOCOMPDATA puGetInternalData);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateMoComp(
    _In_ HANDLE hDirectDraw,
    _Inout_ PDD_CREATEMOCOMPDATA puCreateMoCompData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdDestroyMoComp(
    _In_ HANDLE hMoComp,
    _Inout_ PDD_DESTROYMOCOMPDATA puDestroyMoCompData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdBeginMoCompFrame(
    _In_ HANDLE hMoComp,
    _Inout_ PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdEndMoCompFrame(
    _In_ HANDLE hMoComp,
    _Inout_ PDD_ENDMOCOMPFRAMEDATA puEndFrameData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdRenderMoComp(
    _In_ HANDLE hMoComp,
    _Inout_ PDD_RENDERMOCOMPDATA puRenderMoCompData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdQueryMoCompStatus(
    _In_ HANDLE hMoComp,
    _Inout_ PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiDdAlphaBlt(
    _In_ HANDLE hSurfaceDest,
    _In_opt_ HANDLE hSurfaceSrc,
    _Inout_ PDD_BLTDATA puBltData);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAlphaBlend(
    _In_ HDC hdcDst,
    _In_ LONG DstX,
    _In_ LONG DstY,
    _In_ LONG DstCx,
    _In_ LONG DstCy,
    _In_ HDC hdcSrc,
    _In_ LONG SrcX,
    _In_ LONG SrcY,
    _In_ LONG SrcCx,
    _In_ LONG SrcCy,
    _In_ BLENDFUNCTION BlendFunction,
    _In_ HANDLE hcmXform);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGradientFill(
    _In_ HDC hdc,
    _In_ PTRIVERTEX pVertex,
    _In_ ULONG nVertex,
    _In_ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ ULONG ulMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetIcmMode(
    _In_ HDC hdc,
    _In_ ULONG nCommand,
    _In_ ULONG ulMode);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiCreateColorSpace(
    _In_ PLOGCOLORSPACEEXW pLogColorSpace);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDeleteColorSpace(
    _In_ HANDLE hColorSpace);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetColorSpace(
    _In_ HDC hdc,
    _In_ HCOLORSPACE hColorSpace);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiCreateColorTransform(
    _In_ HDC hdc,
    _In_ LPLOGCOLORSPACEW pLogColorSpaceW,
    _In_reads_bytes_opt_(cjSrcProfile) PVOID pvSrcProfile,
    _In_ ULONG cjSrcProfile,
    _In_reads_bytes_opt_(cjDestProfile) PVOID pvDestProfile,
    _In_ ULONG cjDestProfile,
    _In_reads_bytes_opt_(cjTargetProfile) PVOID pvTargetProfile,
    _In_ ULONG cjTargetProfile);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDeleteColorTransform(
    _In_ HDC hdc,
    _In_ HANDLE hColorTransform);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiCheckBitmapBits(
    _In_ HDC hdc,
    _In_ HANDLE hColorTransform,
    _In_reads_bytes_(dwStride * dwHeight) PVOID pvBits,
    _In_ ULONG bmFormat,
    _In_ DWORD dwWidth,
    _In_ DWORD dwHeight,
    _In_ DWORD dwStride,
    _Out_writes_bytes_(dwWidth * dwHeight) PBYTE paResults);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiColorCorrectPalette(
    _In_ HDC hdc,
    _In_ HPALETTE hpal,
    _In_ ULONG uFirstEntry,
    _In_ ULONG cPalEntries,
    _Inout_updates_(cPalEntries) PALETTEENTRY *ppalEntry,
    _In_ ULONG uCommand);

__kernel_entry
W32KAPI
ULONG_PTR
APIENTRY
NtGdiGetColorSpaceforBitmap(
    _In_ HBITMAP hsurf);

_Success_(return!=FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetDeviceGammaRamp(
    _In_ HDC hdc,
    _Out_writes_bytes_(sizeof(GAMMARAMP)) LPVOID lpGammaRamp);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetDeviceGammaRamp(
    _In_ HDC hdc,
    _In_reads_bytes_(sizeof(GAMMARAMP)) LPVOID lpGammaRamp);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiIcmBrushInfo(
    _In_ HDC hdc,
    _In_ HBRUSH hbrush,
    _Inout_updates_bytes_(sizeof(BITMAPINFO) + ((/*MAX_COLORTABLE*/256 - 1) * sizeof(RGBQUAD))) PBITMAPINFO pbmiDIB,
    _Inout_updates_bytes_(*pulBits) PVOID pvBits,
    _Inout_ ULONG *pulBits,
    _Out_opt_ DWORD *piUsage,
    _Out_opt_ BOOL *pbAlreadyTran,
    _In_ ULONG Command);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiFlush(
    VOID);

__kernel_entry
W32KAPI
HDC
APIENTRY
NtGdiCreateMetafileDC(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiMakeInfoDC(
    _In_ HDC hdc,
    _In_ BOOL bSet);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiCreateClientObj(
    _In_ ULONG ulType);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDeleteClientObj(
    _In_ HANDLE h);

__kernel_entry
W32KAPI
LONG
APIENTRY
NtGdiGetBitmapBits(
    _In_ HBITMAP hbm,
    _In_ ULONG cjMax,
    _Out_writes_bytes_opt_(cjMax) PBYTE pjOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDeleteObjectApp(
    _In_ HANDLE hobj);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetPath(
    _In_ HDC hdc,
    _Out_writes_opt_(cptBuf) LPPOINT pptlBuf,
    _Out_writes_opt_(cptBuf) LPBYTE pjTypes,
    _In_ INT cptBuf);

__kernel_entry
W32KAPI
HDC
APIENTRY
NtGdiCreateCompatibleDC(
    _In_opt_ HDC hdc);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiCreateDIBitmapInternal(
    _In_ HDC hdc,
    _In_ INT cx,
    _In_ INT cy,
    _In_ DWORD fInit,
    _In_reads_bytes_opt_(cjMaxBits) LPBYTE pjInit,
    _In_reads_bytes_opt_(cjMaxInitInfo) LPBITMAPINFO pbmi,
    _In_ DWORD iUsage,
    _In_ UINT cjMaxInitInfo,
    _In_ UINT cjMaxBits,
    _In_ FLONG f,
    _In_ HANDLE hcmXform);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiCreateDIBSection(
    _In_ HDC hdc,
    _In_opt_ HANDLE hSectionApp,
    _In_ DWORD dwOffset,
    _In_reads_bytes_opt_(cjHeader) LPBITMAPINFO pbmi,
    _In_ DWORD iUsage,
    _In_ UINT cjHeader,
    _In_ FLONG fl,
    _In_ ULONG_PTR dwColorSpace,
    _Outptr_ PVOID *ppvBits);

__kernel_entry
W32KAPI
HBRUSH
APIENTRY
NtGdiCreateSolidBrush(
    _In_ COLORREF cr,
    _In_opt_ HBRUSH hbr);

__kernel_entry
W32KAPI
HBRUSH
APIENTRY
NtGdiCreateDIBBrush(
    _In_reads_bytes_(cj) PVOID pv,
    _In_ FLONG fl,
    _In_ UINT  cj,
    _In_ BOOL  b8X8,
    _In_ BOOL bPen,
    _In_ PVOID pClient);

__kernel_entry
W32KAPI
HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(
    _In_ HBITMAP hbm,
    _In_ BOOL bPen,
    _In_ BOOL b8X8);

__kernel_entry
W32KAPI
HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(
    _In_ ULONG ulStyle,
    _In_ COLORREF clrr,
    _In_ BOOL bPen);

__kernel_entry
W32KAPI
HPEN
APIENTRY
NtGdiExtCreatePen(
    _In_ ULONG flPenStyle,
    _In_ ULONG ulWidth,
    _In_ ULONG iBrushStyle,
    _In_ ULONG ulColor,
    _In_ ULONG_PTR lClientHatch,
    _In_ ULONG_PTR lHatch,
    _In_ ULONG cstyle,
    _In_reads_opt_(cstyle) PULONG pulStyle,
    _In_ ULONG cjDIB,
    _In_ BOOL bOldStylePen,
    _In_opt_ HBRUSH hbrush);

__kernel_entry
W32KAPI
HRGN
APIENTRY
NtGdiCreateEllipticRgn(
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

__kernel_entry
W32KAPI
HRGN
APIENTRY
NtGdiCreateRoundRectRgn(
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom,
    _In_ INT xWidth,
    _In_ INT yHeight);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiCreateServerMetaFile(
    _In_ DWORD iType,
    _In_ ULONG cjData,
    _In_reads_bytes_(cjData) LPBYTE pjData,
    _In_ DWORD mm,
    _In_ DWORD xExt,
    _In_ DWORD yExt);

__kernel_entry
W32KAPI
HRGN
APIENTRY
NtGdiExtCreateRegion(
    _In_opt_ LPXFORM px,
    _In_ DWORD cj,
    _In_reads_bytes_(cj) LPRGNDATA prgndata);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiMakeFontDir(
    _In_ FLONG flEmbed,
    _Out_writes_bytes_(cjFontDir) PBYTE pjFontDir,
    _In_ UINT cjFontDir,
    _In_reads_bytes_(cjPathname) LPWSTR pwszPathname,
    _In_ UINT cjPathname);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPolyDraw(
    _In_ HDC hdc,
    _In_reads_(cpt) LPPOINT ppt,
    _In_reads_(cpt) LPBYTE pjAttr,
    _In_ ULONG cpt);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPolyTextOutW(
    _In_ HDC hdc,
    _In_reads_(cStr) POLYTEXTW *pptw,
    _In_ UINT cStr,
    _In_ DWORD dwCodePage);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetServerMetaFileBits(
    _In_ HANDLE hmo,
    _In_ ULONG cjData,
    _Out_writes_bytes_opt_(cjData) LPBYTE pjData,
    _Out_ PDWORD piType,
    _Out_ PDWORD pmm,
    _Out_ PDWORD pxExt,
    _Out_ PDWORD pyExt);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEqualRgn(
    _In_ HRGN hrgn1,
    _In_ HRGN hrgn2);

_Must_inspect_result_
_Success_(return!=FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetBitmapDimension(
    _In_ HBITMAP hbm,
    _Out_ LPSIZE psize);

__kernel_entry
W32KAPI
UINT
APIENTRY
NtGdiGetNearestPaletteIndex(
    _In_ HPALETTE hpal,
    _In_ COLORREF crColor);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPtVisible(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRectVisible(
    _In_ HDC hdc,
    _In_ LPRECT prc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    _In_reads_(cwc) WCHAR *pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ ULONG fl,
    _In_ DWORD dwPidTid,
    _In_opt_ DESIGNVECTOR *pdv);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiResizePalette(
    _In_ HPALETTE hpal,
    _In_ UINT cEntry);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetBitmapDimension(
    _In_ HBITMAP hbm,
    _In_ INT cx,
    _In_ INT cy,
    _In_opt_ LPSIZE psizeOut);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiOffsetClipRgn(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiSetMetaRgn(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetTextJustification(
    _In_ HDC hdc,
    _In_ INT lBreakExtra,
    _In_ INT cBreak);

_Success_(return!=ERROR)
__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetAppClipBox(
    _In_ HDC hdc,
    _Out_ LPRECT prc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtentExW(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) LPWSTR pwsz,
    _In_ ULONG cwc,
    _In_ ULONG dxMax,
    _Out_opt_ ULONG *pcCh,
    _Out_writes_to_opt_(cwc, *pcCh) PULONG pdxOut,
    _Out_ LPSIZE psize,
    _In_ FLONG fl);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    _In_ HDC hdc,
    _In_ UINT wchFirst,
    _In_ ULONG cwch,
    _In_reads_opt_(cwch) PWCHAR pwch,
    _In_ FLONG fl,
    _Out_writes_bytes_(cwch * sizeof(ABC)) PVOID pvBuf);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetCharacterPlacementW(
    _In_ HDC hdc,
    _In_reads_z_(nCount) LPWSTR pwsz,
    _In_ INT nCount,
    _In_ INT nMaxExtent,
    _Inout_ LPGCP_RESULTSW pgcpw,
    _In_ DWORD dwFlags);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAngleArc(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ DWORD dwRadius,
    _In_ DWORD dwStartAngle,
    _In_ DWORD dwSweepAngle);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiBeginPath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSelectClipPath(
    _In_ HDC hdc,
    _In_ INT iMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiCloseFigure(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEndPath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAbortPath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiFillPath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiStrokeAndFillPath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiStrokePath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiWidenPath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiFlattenPath(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
HRGN
APIENTRY
NtGdiPathToRegion(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetMiterLimit(
    _In_ HDC hdc,
    _In_ DWORD dwNew,
    _Inout_opt_ PDWORD pdwOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetFontXform(
    _In_ HDC hdc,
    _In_ DWORD dwxScale,
    _In_ DWORD dwyScale);

_Success_(return != FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetMiterLimit(
    _In_ HDC hdc,
    _Out_ PDWORD pdwOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEllipse(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRectangle(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRoundRect(
    _In_ HDC hdc,
    _In_ INT x1,
    _In_ INT y1,
    _In_ INT x2,
    _In_ INT y2,
    _In_ INT x3,
    _In_ INT y3);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPlgBlt(
    _In_ HDC hdcTrg,
    _In_reads_(3) LPPOINT pptlTrg,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_opt_ HBITMAP hbmMask,
    _In_ INT xMask,
    _In_ INT yMask,
    _In_ DWORD crBackColor);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiMaskBlt(
    _In_ HDC hdc,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ INT cx,
    _In_ INT cy,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_opt_ HBITMAP hbmMask,
    _In_ INT xMask,
    _In_ INT yMask,
    _In_ DWORD dwRop4,
    _In_ DWORD crBackColor);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiExtFloodFill(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ COLORREF crColor,
    _In_ UINT iFillType);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiFillRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiFrameRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush,
    _In_ INT xWidth,
    _In_ INT yHeight);

__kernel_entry
W32KAPI
COLORREF
APIENTRY
NtGdiSetPixel(
    _In_ HDC hdcDst,
    _In_ INT x,
    _In_ INT y,
    _In_ COLORREF crColor);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetPixel(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiStartPage(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEndPage(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiStartDoc(
    _In_ HDC hdc,
    _In_ DOCINFOW *pdi,
    _Out_ BOOL *pbBanding,
    _In_ INT iJob);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEndDoc(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAbortDoc(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiUpdateColors(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthW(
    _In_ HDC hdc,
    _In_ UINT wcFirst,
    _In_ UINT cwc,
    _In_reads_opt_(cwc) PWCHAR pwc,
    _In_ FLONG fl,
    _Out_writes_bytes_(cwc * sizeof(ULONG)) PVOID pvBuf);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthInfo(
    _In_ HDC hdc,
    _Out_ PCHWIDTHINFO pChWidthInfo);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiDrawEscape(
    _In_ HDC hdc,
    _In_ INT iEsc,
    _In_ INT cjIn,
    _In_reads_bytes_opt_(cjIn) LPSTR pjIn);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiExtEscape(
    _In_opt_ HDC hdc,
    _In_reads_opt_(cwcDriver) PWCHAR pDriver,
    _In_ INT cwcDriver,
    _In_ INT iEsc,
    _In_ INT cjIn,
    _In_reads_bytes_opt_(cjIn) LPSTR pjIn,
    _In_ INT cjOut,
    _Out_writes_bytes_opt_(cjOut) LPSTR pjOut);

_Success_(return != GDI_ERROR)
__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetFontData(
    _In_ HDC hdc,
    _In_ DWORD dwTable,
    _In_ DWORD dwOffset,
    _Out_writes_bytes_to_opt_(cjBuf, return) PVOID pvBuf,
    _In_ ULONG cjBuf);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetFontFileData(
    _In_ UINT uFileCollectionID,
    _In_ UINT uFileIndex,
    _In_ PULONGLONG pullFileOffset,
    _Out_writes_bytes_(cjBuf) PVOID pvBuf,
    _In_ SIZE_T cjBuf);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetFontFileInfo(
    _In_ UINT uFileCollectionID,
    _In_ UINT uFileIndex,
    _Out_writes_bytes_(cjSize) PFONT_FILE_INFO pffi,
    _In_ SIZE_T cjSize,
    _Out_opt_ PSIZE_T pcjActualSize);
#endif /* (_WIN32_WINNT >= _WIN32_WINNT_WIN7) */

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetGlyphOutline(
    _In_ HDC hdc,
    _In_ WCHAR wch,
    _In_ UINT iFormat,
    _Out_ LPGLYPHMETRICS pgm,
    _In_ ULONG cjBuf,
    _Out_writes_bytes_opt_(cjBuf) PVOID pvBuf,
    _In_ LPMAT2 pmat2,
    _In_ BOOL bIgnoreRotation);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetETM(
    _In_ HDC hdc,
    _Out_opt_ EXTTEXTMETRIC *petm);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    _Out_writes_bytes_(cjBytes) LPRASTERIZER_STATUS praststat,
    _In_ ULONG cjBytes);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetKerningPairs(
    _In_ HDC hdc,
    _In_ ULONG cPairs,
    _Out_writes_to_opt_(cPairs, return) KERNINGPAIR *pkpDst);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiMonoBitmap(
    _In_ HBITMAP hbm);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    _In_ HBRUSH hbr,
    _Out_ UINT *piUsage);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiEnumObjects(
    _In_ HDC hdc,
    _In_ INT iObjectType,
    _In_ ULONG cjBuf,
    _Out_writes_bytes_opt_(cjBuf) PVOID pvBuf);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiResetDC(
    _In_ HDC hdc,
    _In_ LPDEVMODEW pdm,
    _Out_ PBOOL pbBanding,
    _In_opt_ DRIVER_INFO_2W *pDriverInfo2,
    _At_((PUMDHPDEV*)ppUMdhpdev, _Out_) PVOID ppUMdhpdev);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiSetBoundsRect(
    _In_ HDC hdc,
    _In_ LPRECT prc,
    _In_ DWORD f);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetColorAdjustment(
    _In_ HDC hdc,
    _Out_ PCOLORADJUSTMENT pcaOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetColorAdjustment(
    _In_ HDC hdc,
    _In_ PCOLORADJUSTMENT pca);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiCancelDC(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
HDC
APIENTRY
NtGdiOpenDCW(
    _In_opt_ PUNICODE_STRING pustrDevice,
    _In_ DEVMODEW *pdm,
    _In_ PUNICODE_STRING pustrLogAddr,
    _In_ ULONG iType,
    _In_ BOOL bDisplay,
    _In_opt_ HANDLE hspool,
    _In_opt_ DRIVER_INFO_2W *pDriverInfo2,
    _At_((PUMDHPDEV*)pUMdhpdev, _Out_) PVOID pUMdhpdev);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetDCDword(
    _In_ HDC hdc,
    _In_ UINT u,
    _Out_ DWORD *Result);

_Success_(return!=FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetDCPoint(
    _In_ HDC hdc,
    _In_ UINT iPoint,
    _Out_ PPOINTL pptOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiScaleViewportExtEx(
    _In_ HDC hdc,
    _In_ INT xNum,
    _In_ INT xDenom,
    _In_ INT yNum,
    _In_ INT yDenom,
    _Out_opt_ LPSIZE pszOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiScaleWindowExtEx(
    _In_ HDC hdc,
    _In_ INT xNum,
    _In_ INT xDenom,
    _In_ INT yNum,
    _In_ INT yDenom,
    _Out_opt_ LPSIZE pszOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetVirtualResolution(
    _In_ HDC hdc,
    _In_ INT cxVirtualDevicePixel,
    _In_ INT cyVirtualDevicePixel,
    _In_ INT cxVirtualDeviceMm,
    _In_ INT cyVirtualDeviceMm);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetSizeDevice(
    _In_ HDC hdc,
    _In_ INT cxVirtualDevice,
    _In_ INT cyVirtualDevice);

_Success_(return !=FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetTransform(
    _In_ HDC hdc,
    _In_ DWORD iXform,
    _Out_ LPXFORM pxf);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiModifyWorldTransform(
    _In_ HDC hdc,
    _In_opt_ LPXFORM pxf,
    _In_ DWORD iXform);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiCombineTransform(
    _Out_ LPXFORM pxfDst,
    _In_ LPXFORM pxfSrc1,
    _In_ LPXFORM pxfSrc2);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiTransformPoints(
    _In_ HDC hdc,
    _In_reads_(c) PPOINT pptIn,
    _Out_writes_(c) PPOINT pptOut,
    _In_ INT c,
    _In_ INT iMode);

__kernel_entry
W32KAPI
LONG
APIENTRY
NtGdiConvertMetafileRect(
    _In_ HDC hdc,
    _Inout_ PRECTL prect);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetTextCharsetInfo(
    _In_ HDC hdc,
    _Out_opt_ LPFONTSIGNATURE lpSig,
    _In_ DWORD dwFlags);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDoBanding(
    _In_ HDC hdc,
    _In_ BOOL bStart,
    _Out_ POINTL *pptl,
    _Out_ PSIZE pSize);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetPerBandInfo(
    _In_ HDC hdc,
    _Inout_ PERBANDINFO *ppbi);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiGetStats(
    _In_ HANDLE hProcess,
    _In_ INT iIndex,
    _In_ INT iPidType,
    _Out_writes_bytes_(cjResultSize) PVOID pResults,
    _In_ UINT cjResultSize);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetMagicColors(
    _In_ HDC hdc,
    _In_ PALETTEENTRY peMagic,
    _In_ ULONG Index);

__kernel_entry
W32KAPI
HBRUSH
APIENTRY
NtGdiSelectBrush(
    _In_ HDC hdc,
    _In_ HBRUSH hbrush);

__kernel_entry
W32KAPI
HPEN
APIENTRY
NtGdiSelectPen(
    _In_ HDC hdc,
    _In_ HPEN hpen);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiSelectBitmap(
    _In_ HDC hdc,
    _In_ HBITMAP hbm);

__kernel_entry
W32KAPI
HFONT
APIENTRY
NtGdiSelectFont(
    _In_ HDC hdc,
    _In_ HFONT hf);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiExtSelectClipRgn(
    _In_ HDC hdc,
    _In_opt_ HRGN hrgn,
    _In_ INT iMode);

__kernel_entry
W32KAPI
HPEN
APIENTRY
NtGdiCreatePen(
    _In_ INT iPenStyle,
    _In_ INT iPenWidth,
    _In_ COLORREF cr,
    _In_opt_ HBRUSH hbr);

#ifdef _WINDOWBLT_NOTIFICATION_
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiBitBlt(
    _In_ HDC hdcDst,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_opt_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ DWORD rop4,
    _In_ DWORD crBackColor,
    _In_ FLONG fl);
#else
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiBitBlt(
    _In_ HDC hdcDst,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_opt_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ DWORD rop4,
    _In_ DWORD crBackColor);
#endif

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiTileBitBlt(
    _In_ HDC hdcDst,
    _In_ RECTL *prectDst,
    _In_ HDC hdcSrc,
    _In_ RECTL *prectSrc,
    _In_ POINTL *pptlOrigin,
    _In_ DWORD rop4,
    _In_ DWORD crBackColor);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiTransparentBlt(
    _In_ HDC hdcDst,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ INT cxDst,
    _In_ INT cyDst,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_ COLORREF TransColor);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtent(
    _In_ HDC hdc,
    _In_reads_(cwc) LPWSTR lpwsz,
    _In_ INT cwc,
    _Out_ LPSIZE psize,
    _In_ UINT flOpts);

_Success_(return != FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetTextMetricsW(
    _In_ HDC hdc,
    _Out_writes_bytes_(cj) TMW_INTERNAL *ptm,
    _In_ ULONG cj);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetTextFaceW(
    _In_ HDC hdc,
    _In_ INT cChar,
    _Out_writes_to_opt_(cChar, return) LPWSTR pszOut,
    _In_ BOOL bAliasName);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetRandomRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ INT iRgn);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiExtTextOutW(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ UINT flOpts,
    _In_opt_ LPRECT prcl,
    _In_reads_opt_(cwc) LPWSTR pwsz,
    _In_range_(0, 0xffff) INT cwc,
    _In_reads_opt_(_Inexpressible_(cwc)) LPINT pdx,
    _In_ DWORD dwCodePage);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiIntersectClipRect(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

__kernel_entry
W32KAPI
HRGN
APIENTRY
NtGdiCreateRectRgn(
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPatBlt(
    _In_ HDC hdcDest,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_ DWORD dwRop);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPolyPatBlt(
    _In_ HDC hdc,
    _In_ DWORD rop4,
    _In_reads_(cPoly) PPOLYPATBLT pPoly,
    _In_ DWORD cPoly,
    _In_ DWORD dwMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiUnrealizeObject(
    _In_ HANDLE h);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiGetStockObject(
    _In_ INT iObject);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiCreateCompatibleBitmap(
    _In_ HDC hdc,
    _In_ INT cx,
    _In_ INT cy);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiCreateBitmapFromDxSurface(
    _In_ HDC hdc,
    _In_ UINT uiWidth,
    _In_ UINT uiHeight,
    _In_ DWORD Format,
    _In_opt_ HANDLE hDxSharedSurface);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiCreateBitmapFromDxSurface2(
    _In_ HDC hdc,
    _In_ UINT uiWidth,
    _In_ UINT uiHeight,
    _In_ DWORD Format,
    _In_ DWORD SubresourceIndex,
    _In_ BOOL bSharedSurfaceNtHandle,
    _In_opt_ HANDLE hDxSharedSurface);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiBeginGdiRendering(
    _In_ HBITMAP hbm,
    _In_ BOOL bDiscard,
    _In_ PVOID KernelModeDeviceHandle);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEndGdiRendering(
    _In_ HBITMAP hbm,
    _In_ BOOL bDiscard,
    _Out_ BOOL* pbDeviceRemoved,
    _In_ PVOID KernelModeDeviceHandle);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiLineTo(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

_Success_(return != FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiMoveTo(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _Out_opt_ LPPOINT pptOut);

_Success_(return != 0)
__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiExtGetObjectW(
    _In_ HANDLE h,
    _In_ INT cj,
    _Out_writes_bytes_opt_(cj) LPVOID pvOut);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetDeviceCaps(
    _In_ HDC hdc,
    _In_ INT i);

_Success_(return!=FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetDeviceCapsAll (
    _In_opt_ HDC hdc,
    _Out_ PDEVCAPS pDevCaps);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiStretchBlt(
    _In_ HDC hdcDst,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ INT cxDst,
    _In_ INT cyDst,
    _In_opt_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_ DWORD dwRop,
    _In_ DWORD dwBackColor);

_Success_(return!=FALSE)
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetBrushOrg(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _Out_opt_ LPPOINT pptOut);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiCreateBitmap(
    _In_ INT cx,
    _In_ INT cy,
    _In_ UINT cPlanes,
    _In_ UINT cBPP,
    _In_opt_ LPBYTE pjInit);

__kernel_entry
W32KAPI
HPALETTE
APIENTRY
NtGdiCreateHalftonePalette(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRestoreDC(
    _In_ HDC hdc,
    _In_ INT iLevel);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiExcludeClipRect(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiSaveDC(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiCombineRgn(
    _In_ HRGN hrgnDst,
    _In_ HRGN hrgnSrc1,
    _In_opt_ HRGN hrgnSrc2,
    _In_ INT iMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetRectRgn(
    _In_ HRGN hrgn,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

__kernel_entry
W32KAPI
LONG
APIENTRY
NtGdiSetBitmapBits(
    _In_ HBITMAP hbm,
    _In_ ULONG cj,
    _In_reads_bytes_(cj) PBYTE pjInit);

_Success_(return!=0)
__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetDIBitsInternal(
    _In_ HDC hdc,
    _In_ HBITMAP hbm,
    _In_ UINT iStartScan,
    _In_ UINT cScans,
    _Out_writes_bytes_opt_(cjMaxBits) LPBYTE pjBits,
    _Inout_ LPBITMAPINFO pbmi,
    _In_ UINT iUsage,
    _In_ UINT cjMaxBits,
    _In_ UINT cjMaxInfo);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiOffsetRgn(
    _In_ HRGN hrgn,
    _In_ INT cx,
    _In_ INT cy);

_Success_(return!=ERROR)
__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetRgnBox(
    _In_ HRGN hrgn,
    _Out_ LPRECT prcOut);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiRectInRegion(
    _In_ HRGN hrgn,
    _Inout_ LPRECT prcl);

_Success_(return!=0)
__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetBoundsRect(
    _In_ HDC hdc,
    _Out_ LPRECT prc,
    _In_ DWORD f);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPtInRegion(
    _In_ HRGN hrgn,
    _In_ INT x,
    _In_ INT y);

__kernel_entry
W32KAPI
COLORREF
APIENTRY
NtGdiGetNearestColor(
    _In_ HDC hdc,
    _In_ COLORREF cr);

__kernel_entry
W32KAPI
UINT
APIENTRY
NtGdiGetSystemPaletteUse(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
UINT
APIENTRY
NtGdiSetSystemPaletteUse(
    _In_ HDC hdc,
    _In_ UINT ui);

_Success_(return!=0)
__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetRegionData(
    _In_ HRGN hrgn,
    _In_ ULONG cjBuffer,
    _Out_writes_bytes_to_opt_(cjBuffer, return) LPRGNDATA lpRgnData);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiInvertRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn);

__kernel_entry
INT
W32KAPI
APIENTRY
NtGdiAddFontResourceW(
    _In_reads_(cwc) WCHAR *pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ FLONG f,
    _In_ DWORD dwPidTid,
    _In_opt_ DESIGNVECTOR *pdv);

__kernel_entry
W32KAPI
HFONT
APIENTRY
NtGdiHfontCreate(
    _In_reads_bytes_(cjElfw) ENUMLOGFONTEXDVW *pelfw,
    _In_ ULONG cjElfw,
    _In_ LFTYPE lft,
    _In_ FLONG fl,
    _In_ PVOID pvCliData);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiSetFontEnumeration(
    _In_ ULONG ulType);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEnumFonts(
    _In_ HDC hdc,
    _In_ ULONG iEnumType,
    _In_ FLONG flWin31Compat,
    _In_ ULONG cchFaceName,
    _In_reads_opt_(cchFaceName) LPCWSTR pwszFaceName,
    _In_ ULONG lfCharSet,
    _Inout_ ULONG *pulCount,
    _Out_writes_bytes_opt_(*pulCount) PVOID pvUserModeBuffer);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEnumFontClose(
    _In_ ULONG_PTR idEnum);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEnumFontChunk(
    _In_ HDC hdc,
    _In_ ULONG_PTR idEnum,
    _In_ ULONG cjEfdw,
    _Out_ ULONG *pcjEfdw,
    _Out_ PENUMFONTDATAW pefdw);

__kernel_entry
W32KAPI
ULONG_PTR
APIENTRY
NtGdiEnumFontOpen(
    _In_ HDC hdc,
    _In_ ULONG iEnumType,
    _In_ FLONG flWin31Compat,
    _In_ ULONG cwchMax,
    _In_opt_ LPWSTR pwszFaceName,
    _In_ ULONG lfCharSet,
    _Out_ ULONG *pulCount);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiQueryFonts(
    _Out_writes_(nBufferSize) PUNIVERSAL_FONT_ID pufiFontList,
    _In_ ULONG nBufferSize,
    _Out_ PLARGE_INTEGER pTimeStamp);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiConsoleTextOut(
    _In_ HDC hdc,
    _In_ POLYTEXTW *lpto,
    _In_ UINT nStrings,
    _In_ RECTL *prclBounds);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiFullscreenControl(
    _In_ FULLSCREENCONTROL FullscreenCommand,
    _In_ PVOID FullscreenInput,
    _In_ DWORD FullscreenInputLength,
    _Out_ PVOID FullscreenOutput,
    _Inout_ PULONG FullscreenOutputLength);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetCharSet(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEnableEudc(
    _In_ BOOL b);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEudcLoadUnloadLink(
    _In_reads_opt_(cwcBaseFaceName) LPCWSTR pBaseFaceName,
    _In_ UINT cwcBaseFaceName,
    _In_reads_(cwcEudcFontPath) LPCWSTR pEudcFontPath,
    _In_ UINT cwcEudcFontPath,
    _In_ INT iPriority,
    _In_ INT iFontLinkType,
    _In_ BOOL bLoadLin);

__kernel_entry
W32KAPI
UINT
APIENTRY
NtGdiGetStringBitmapW(
    _In_ HDC hdc,
    _In_ LPWSTR pwsz,
    _In_ UINT cwc,
    _Out_writes_bytes_(cj) BYTE *lpSB,
    _In_ UINT cj);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiGetEudcTimeStampEx(
    _In_reads_opt_(cwcBaseFaceName) LPWSTR lpBaseFaceName,
    _In_ ULONG cwcBaseFaceName,
    _In_ BOOL bSystemTimeStamp);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiQueryFontAssocInfo(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetFontUnicodeRanges(
    _In_ HDC hdc,
    _Out_ _Post_bytecount_(return) LPGLYPHSET pgs);

#ifdef LANGPACK
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
__kernel_entry
W32KAPI
BOOL
NtGdiGetRealizationInfo(
    _In_ HDC hdc,
    _Out_ PFONT_REALIZATION_INFO pri);
#else
__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiGetRealizationInfo(
    _In_ HDC hdc,
    _Out_ PREALIZATION_INFO pri,
    _In_ HFONT hf);
#endif
#endif

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiAddRemoteMMInstanceToDC(
    _In_ HDC hdc,
    _In_reads_bytes_(cjDDV) DOWNLOADDESIGNVECTOR *pddv,
    _In_ ULONG cjDDV);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiUnloadPrinterDriver(
    _In_reads_bytes_(cbDriverName) LPWSTR pDriverName,
    _In_ ULONG cbDriverName);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiInitSpool(
    VOID);

__kernel_entry
W32KAPI
INT
APIENTRY
NtGdiGetSpoolMessage(
    DWORD u1,
    DWORD u2,
    DWORD u3,
    DWORD u4);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngAssociateSurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngEraseSurface(
    _In_ SURFOBJ *pso,
    _In_ RECTL *prcl,
    _In_ ULONG iColor);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiEngCreateBitmap(
    _In_ SIZEL sizl,
    _In_ LONG lWidth,
    _In_ ULONG iFormat,
    _In_ FLONG fl,
    _In_opt_ PVOID pvBits);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngDeleteSurface(
    _In_ HSURF hsurf);

__kernel_entry
W32KAPI
SURFOBJ*
APIENTRY
NtGdiEngLockSurface(
    _In_ HSURF hsurf);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiEngUnlockSurface(
    _In_ SURFOBJ *pso);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngMarkBandingSurface(
    _In_ HSURF hsurf);

__kernel_entry
W32KAPI
HSURF
APIENTRY
NtGdiEngCreateDeviceSurface(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiEngCreateDeviceBitmap(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngCopyBits(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ POINTL *pptlSrc);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngStretchBlt(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlHTOrg,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_opt_ POINTL *pptlMask,
    _In_ ULONG iMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngBitBlt(
    _In_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclTrg,
    _In_opt_ POINTL *pptlSrc,
    _In_opt_ POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ POINTL *pptlBrush,
    _In_ ROP4 rop4);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngPlgBlt(
    _In_ SURFOBJ *psoTrg,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMsk,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlBrushOrg,
    _In_ POINTFIX *pptfx,
    _In_ RECTL *prcl,
    _In_opt_ POINTL *pptl,
    _In_ ULONG iMode);

__kernel_entry
W32KAPI
HPALETTE
APIENTRY
NtGdiEngCreatePalette(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_ ULONG *pulColors,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngDeletePalette(
    _In_ HPALETTE hPal);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngStrokePath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ LINEATTRS *plineattrs,
    _In_ MIX mix);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngStrokeAndFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,IN XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pboStroke,
    _In_ LINEATTRS *plineattrs,
    _In_ BRUSHOBJ *pboFill,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngPaint(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngLineTo(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_ RECTL *prclBounds,
    _In_ MIX mix);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngAlphaBlend(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_ BLENDOBJ *pBlendObj);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngGradientFill(
    _In_ SURFOBJ *psoDest,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_reads_(nVertex) TRIVERTEX *pVertex,
    _In_ ULONG nVertex,
    _In_ /* _In_reads_(nMesh) */ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ RECTL *prclExtents,
    _In_ POINTL *pptlDitherOrg,
    _In_ ULONG ulMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngTransparentBlt(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ RECTL *prclSrc,
    _In_ ULONG iTransColor,
    _In_ ULONG ulReserved);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngTextOut(
    _In_ SURFOBJ *pso,
    _In_ STROBJ *pstro,
    _In_ FONTOBJ *pfo,
    _In_ CLIPOBJ *pco,
    _In_ RECTL *prclExtra,
    _In_ RECTL *prclOpaque,
    _In_ BRUSHOBJ *pboFore,
    _In_ BRUSHOBJ *pboOpaque,
    _In_ POINTL *pptlOrg,
    _In_ MIX mix);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngStretchBltROP(
    _In_ SURFOBJ *psoTrg,
    _In_ SURFOBJ *psoSrc,
    _In_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlBrushOrg,
    _In_ RECTL *prclTrg,
    _In_ RECTL *prclSrc,
    _In_ POINTL *pptlMask,
    _In_ ULONG iMode,
    _In_ BRUSHOBJ *pbo,
    _In_ ROP4 rop4);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iPal,
    _In_ ULONG cPal,
    _Out_writes_(cPal) ULONG *pPal);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiCLIPOBJ_cEnumStart(
    _In_ CLIPOBJ *pco,
    _In_ BOOL bAll,
    _In_ ULONG iType,
    _In_ ULONG iDirection,
    _In_ ULONG cLimit);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiCLIPOBJ_bEnum(
    _In_ CLIPOBJ *pco,
    _In_ ULONG cj,
    _Out_writes_bytes_(cj) ULONG *pul);

__kernel_entry
W32KAPI
PATHOBJ*
APIENTRY
NtGdiCLIPOBJ_ppoGetPath(
    _In_ CLIPOBJ *pco);

__kernel_entry
W32KAPI
CLIPOBJ*
APIENTRY
NtGdiEngCreateClip(
    VOID);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiEngDeleteClip(
    _In_ CLIPOBJ*pco);

__kernel_entry
W32KAPI
PVOID
APIENTRY
NtGdiBRUSHOBJ_pvAllocRbrush(
    _In_ BRUSHOBJ *pbo,
    _In_ ULONG cj);

__kernel_entry
W32KAPI
PVOID
APIENTRY
NtGdiBRUSHOBJ_pvGetRbrush(
    _In_ BRUSHOBJ *pbo);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiBRUSHOBJ_ulGetBrushColor(
    _In_ BRUSHOBJ *pbo);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiBRUSHOBJ_hGetColorTransform(
    _In_ BRUSHOBJ *pbo);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiXFORMOBJ_bApplyXform(
    _In_ XFORMOBJ *pxo,
    _In_ ULONG iMode,
    _In_ ULONG cPoints,
    _In_reads_(cPoints) PPOINTL pptIn,
    _Out_writes_(cPoints) PPOINTL pptOut);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiXFORMOBJ_iGetXform(
    _In_ XFORMOBJ *pxo,
    _Out_opt_ XFORML *pxform);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiFONTOBJ_vGetInfo(
    _In_ FONTOBJ *pfo,
    _In_ ULONG cjSize,
    _Out_writes_bytes_(cjSize) FONTINFO *pfi);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode,
    _In_ ULONG cGlyph,
    _In_ HGLYPH *phg,
    _At_((GLYPHDATA**)ppvGlyph, _Outptr_) PVOID *ppvGlyph);

__kernel_entry
W32KAPI
XFORMOBJ*
APIENTRY
NtGdiFONTOBJ_pxoGetXform(
    _In_ FONTOBJ *pfo);

__kernel_entry
W32KAPI
IFIMETRICS*
APIENTRY
NtGdiFONTOBJ_pifi(
    _In_ FONTOBJ *pfo);

__kernel_entry
W32KAPI
FD_GLYPHSET*
APIENTRY
NtGdiFONTOBJ_pfdg(
    _In_ FONTOBJ *pfo);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiFONTOBJ_cGetAllGlyphHandles(
    _In_ FONTOBJ *pfo,
    _Out_opt_ _Post_count_(return) HGLYPH *phg);

__kernel_entry
W32KAPI
PVOID
APIENTRY
NtGdiFONTOBJ_pvTrueTypeFontFile(
    _In_ FONTOBJ *pfo,
    _Out_ ULONG *pcjFile);

__kernel_entry
W32KAPI
PFD_GLYPHATTR
APIENTRY
NtGdiFONTOBJ_pQueryGlyphAttrs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSTROBJ_bEnum(
    _In_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Outptr_result_buffer_(*pc) PGLYPHPOS *ppgpos);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSTROBJ_bEnumPositionsOnly(
    _In_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Outptr_result_buffer_(*pc) PGLYPHPOS *ppgpos);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiSTROBJ_vEnumStart(
    _Inout_ STROBJ *pstro);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiSTROBJ_dwGetCodePage(
    _In_ STROBJ *pstro);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSTROBJ_bGetAdvanceWidths(
    _In_ STROBJ*pstro,
    _In_ ULONG iFirst,
    _In_ ULONG c,
    _Out_writes_(c) POINTQF*pptqD);

__kernel_entry
W32KAPI
FD_GLYPHSET*
APIENTRY
NtGdiEngComputeGlyphSet(
    _In_ INT nCodePage,
    _In_ INT nFirstChar,
    _In_ INT cChars);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iColor);

__kernel_entry
W32KAPI
HANDLE
APIENTRY
NtGdiXLATEOBJ_hGetColorTransform(
    _In_ XLATEOBJ *pxlo);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiPATHOBJ_vGetBounds(
    _In_ PATHOBJ *ppo,
    _Out_ PRECTFX prectfx);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPATHOBJ_bEnum(
    _In_ PATHOBJ *ppo,
    _Out_ PATHDATA *ppd);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiPATHOBJ_vEnumStart(
    _In_ PATHOBJ *ppo);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiEngDeletePath(
    _In_ PATHOBJ *ppo);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiPATHOBJ_vEnumStartClipLines(
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ SURFOBJ *pso,
    _In_ LINEATTRS *pla);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiPATHOBJ_bEnumClipLines(
    _In_ PATHOBJ *ppo,
    _In_ ULONG cb,
    _Out_writes_bytes_(cb) CLIPLINE *pcl);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngCheckAbort(
    _In_ SURFOBJ *pso);

__kernel_entry
W32KAPI
DHPDEV
APIENTRY
NtGdiGetDhpdev(
    _In_ HDEV hdev);

__kernel_entry
W32KAPI
LONG
APIENTRY
NtGdiHT_Get8BPPFormatPalette(
    _Out_opt_ _Post_count_(return) LPPALETTEENTRY pPaletteEntry,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma);

__kernel_entry
W32KAPI
LONG
APIENTRY
NtGdiHT_Get8BPPMaskPalette(
    _Out_opt_ _Post_count_(return) LPPALETTEENTRY pPaletteEntry,
    _In_ BOOL Use8BPPMaskPal,
    _In_ BYTE CMYMask,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiUpdateTransform(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiSetLayout(
    _In_ HDC hdc,
    _In_ LONG wox,
    _In_ DWORD dwLayout);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiMirrorWindowOrg(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
LONG
APIENTRY
NtGdiGetDeviceWidth(
    _In_ HDC hdc);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiSetUMPDSandboxState(
    _In_ BOOL bEnabled);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiSetPUMPDOBJ(
    _In_opt_ HUMPD humpd,
    _In_ BOOL bStoreID,
    _Inout_opt_ HUMPD *phumpd,
    _Out_opt_ BOOL *pbWOW64);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiBRUSHOBJ_DeleteRbrush(
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ BRUSHOBJ *pboB);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiUMPDEngFreeUserMem(
    _In_ KERNEL_PVOID *ppv);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiSetBitmapAttributes(
    _In_ HBITMAP hbm,
    _In_ DWORD dwFlags);

__kernel_entry
W32KAPI
HBITMAP
APIENTRY
NtGdiClearBitmapAttributes(
    _In_ HBITMAP hbm,
    _In_ DWORD dwFlags);

__kernel_entry
W32KAPI
HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    _In_ HBRUSH hbm,
    _In_ DWORD dwFlags);

__kernel_entry
W32KAPI
HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    _In_ HBRUSH hbm,
    _In_ DWORD dwFlags);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDrawStream(
    _In_ HDC hdcDst,
    _In_ ULONG cjIn,
    _In_reads_bytes_(cjIn) VOID *pvIn);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiMakeObjectXferable(
    _In_ HANDLE h,
    _In_ DWORD dwProcessId);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiMakeObjectUnXferable(
    _In_ HANDLE h);

#ifdef PRIVATE_DWM_INTERFACE

__kernel_entry
W32KAPI
BOOL
NtGdiSfmRegisterLogicalSurfaceForSignaling(
    _In_ HLSURF hlsurf,
    BOOL fSignalOnDirty);

__kernel_entry
W32KAPI
BOOL
NtGdiDwmGetHighColorMode(
    _Out_ DXGI_FORMAT* pdxgiFormat);

__kernel_entry
W32KAPI
BOOL
NtGdiDwmSetHighColorMode(
    _In_ DXGI_FORMAT dxgiFormat);

__kernel_entry
W32KAPI
HANDLE
NtGdiDwmCaptureScreen(
    _In_ const RECT* prcCapture,
    _In_ DXGI_FORMAT dxgiFormat);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiDdCreateFullscreenSprite(
    _In_ HDC hdc,
    _In_ COLORREF crKey,
    _Out_ HANDLE* phSprite,
    _Out_ HDC* phdcSprite);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiDdNotifyFullscreenSpriteUpdate(
    _In_ HDC hdc,
    _In_ HANDLE hSprite);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiDdDestroyFullscreenSprite(
    _In_ HDC hdc,
    _In_ HANDLE hSprite);

__kernel_entry
W32KAPI
ULONG
APIENTRY
NtGdiDdQueryVisRgnUniqueness(
    VOID);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiHLSurfGetInformation(
    _In_ HLSURF hlsurf,
    _In_ HLSURF_INFORMATION_CLASS InformationClass,
    _In_reads_bytes_opt_(*pcjInfoBuffer) PVOID pvInfoBuffer,
    _Inout_ PULONG pcjInfoBuffer);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiHLSurfSetInformation(
    _In_ HLSURF hlsurf,
    _In_ HLSURF_INFORMATION_CLASS InformationClass,
    _In_reads_bytes_opt_(cjInfoBuffer) PVOID pvInfoBuffer,
    _In_ ULONG cjInfoBuffer);

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiDwmCreatedBitmapRemotingOutput(
    VOID);

__kernel_entry
W32KAPI
NTSTATUS
APIENTRY
NtGdiGetCurrentDpiInfo(
    _In_ HMONITOR hmon,
    _Out_ PVOID pvStruct);

#endif /* PRIVATE_DWM_INTERFACE */

#endif /* _NTGDI_ */
