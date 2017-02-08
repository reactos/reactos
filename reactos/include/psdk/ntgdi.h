/*
 * NtGdi Entrypoints
 */
#ifndef _NTGDI_
#define _NTGDI_

#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

#ifndef _WINDOWBLT_NOTIFICATION_
#define _WINDOWBLT_NOTIFICATION_
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

W32KAPI
BOOL
APIENTRY
NtGdiInit(VOID);

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
    _In_ DWORD iType
);

W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    _In_ HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    _In_ INT cwc,
    OUT OPTIONAL LPWORD pgi,
    _In_ DWORD iMode
);

W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesWInternal(
    _In_ HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    _In_ INT cwc,
    OUT OPTIONAL LPWORD pgi,
    _In_ DWORD iMode,
    _In_ BOOL bSubset
);

W32KAPI
HPALETTE
APIENTRY
NtGdiCreatePaletteInternal(
    IN LPLOGPALETTE pLogPal,
    _In_ UINT cEntries
);

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
    _In_opt_ LPBYTE pjInit,
    _In_ LPBITMAPINFO pbmi,
    _In_ DWORD dwUsage,
    _In_ DWORD dwRop4,
    _In_ UINT cjMaxInfo,
    _In_ UINT cjMaxBits,
    _In_opt_ HANDLE hcmXform);

W32KAPI
ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(
    _In_ HDC hdc,
    _In_ ULONG cjotm,
    _Out_opt_ OUTLINETEXTMETRICW *potmw,
    _Out_ TMDIFF *ptmd);

_Success_(return != FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiGetAndSetDCDword(
    _In_ HDC hdc,
    _In_ UINT u,
    _In_ DWORD dwIn,
    _Out_ DWORD *pdwResult);

W32KAPI
HANDLE
APIENTRY
NtGdiGetDCObject(
    _In_ HDC hdc,
    _In_ INT itype);

W32KAPI
HDC
APIENTRY
NtGdiGetDCforBitmap(
    _In_ HBITMAP hsurf);

W32KAPI
BOOL
APIENTRY
NtGdiGetMonitorID(
    _In_ HDC hdc,
    _In_ DWORD dwSize,
    _Out_ LPWSTR pszMonitorID);

W32KAPI
INT
APIENTRY
NtGdiGetLinkedUFIs(
    _In_ HDC hdc,
    OUT OPTIONAL PUNIVERSAL_FONT_ID pufiLinkedUFIs,
    _In_ INT BufferSize
);

W32KAPI
BOOL
APIENTRY
NtGdiSetLinkedUFIs(
    _In_ HDC hdc,
    IN PUNIVERSAL_FONT_ID pufiLinks,
    _In_ ULONG uNumUFIs
);

W32KAPI
BOOL
APIENTRY
NtGdiGetUFI(
    _In_ HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG *pfl
);

W32KAPI
BOOL
APIENTRY
NtGdiForceUFIMapping(
    _In_ HDC hdc,
    IN PUNIVERSAL_FONT_ID pufi
);

W32KAPI
BOOL
APIENTRY
NtGdiGetUFIPathname(
    IN PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL ULONG* pcwc,
    OUT OPTIONAL LPWSTR pwszPathname,
    OUT OPTIONAL ULONG* pcNumFiles,
    _In_ FLONG fl,
    OUT OPTIONAL BOOL *pbMemFont,
    OUT OPTIONAL ULONG *pcjView,
    OUT OPTIONAL PVOID pvView,
    OUT OPTIONAL BOOL *pbTTC,
    OUT OPTIONAL ULONG *piTTC
);

W32KAPI
BOOL
APIENTRY
NtGdiAddRemoteFontToDC(
    _In_ HDC hdc,
    _In_ PVOID pvBuffer,
    _In_ ULONG cjBuffer,
    IN OPTIONAL PUNIVERSAL_FONT_ID pufi
);

W32KAPI
HANDLE
APIENTRY
NtGdiAddFontMemResourceEx(
    _In_ PVOID pvBuffer,
    _In_ DWORD cjBuffer,
    IN DESIGNVECTOR *pdv,
    _In_ ULONG cjDV,
    OUT DWORD *pNumFonts
);

W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(
    _In_ HANDLE hMMFont);

W32KAPI
BOOL
APIENTRY
NtGdiUnmapMemFont(
    _In_ PVOID pvView);

W32KAPI
BOOL
APIENTRY
NtGdiRemoveMergeFont(
    _In_ HDC hdc,
    IN UNIVERSAL_FONT_ID *pufi
);

W32KAPI
BOOL
APIENTRY
NtGdiAnyLinkedFonts(VOID);

W32KAPI
BOOL
APIENTRY
NtGdiGetEmbUFI(
    _In_ HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG  *pfl,
    OUT KERNEL_PVOID *embFontID
);

W32KAPI
ULONG
APIENTRY
NtGdiGetEmbedFonts(VOID);

W32KAPI
BOOL
APIENTRY
NtGdiChangeGhostFont(
    IN KERNEL_PVOID *pfontID,
    _In_ BOOL bLoad
);

W32KAPI
BOOL
APIENTRY
NtGdiAddEmbFontToDC(
    _In_ HDC hdc,
    IN VOID **pFontID
);

W32KAPI
BOOL
APIENTRY
NtGdiFontIsLinked(
    _In_ HDC hdc);

W32KAPI
ULONG_PTR
APIENTRY
NtGdiPolyPolyDraw(
    _In_ HDC hdc,
    _In_ PPOINT ppt,
    _In_ PULONG pcpt,
    _In_ ULONG ccpt,
    _In_ INT iFunc);

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

W32KAPI
BOOL
APIENTRY
NtGdiComputeXformCoefficients(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiGetWidthTable(
    _In_ HDC hdc,
    _In_ ULONG cSpecial,
    IN WCHAR *pwc,
    _In_ ULONG cwc,
    OUT USHORT *psWidth,
    OUT OPTIONAL WIDTHDATA *pwd,
    OUT FLONG *pflInfo
);

_Success_(return != 0)
W32KAPI
INT
APIENTRY
NtGdiDescribePixelFormat(
    _In_ HDC hdc,
    _In_ INT ipfd,
    _In_ UINT cjpfd,
    _When_(cjpfd != 0, _Out_) PPIXELFORMATDESCRIPTOR ppfd);

W32KAPI
BOOL
APIENTRY
NtGdiSetPixelFormat(
    _In_ HDC hdc,
    _In_ INT ipfd);

W32KAPI
BOOL
APIENTRY
NtGdiSwapBuffers(
    _In_ HDC hdc);

W32KAPI
INT
APIENTRY
NtGdiSetupPublicCFONT(
    _In_ HDC hdc,
    IN OPTIONAL HFONT hf,
    _In_ ULONG ulAve
);

W32KAPI
DWORD
APIENTRY
NtGdiDxgGenericThunk(
    _In_ ULONG_PTR ulIndex,
    _In_ ULONG_PTR ulHandle,
    IN OUT SIZE_T *pdwSizeOfPtr1,
    IN OUT  PVOID pvPtr1,
    IN OUT SIZE_T *pdwSizeOfPtr2,
    IN OUT  PVOID pvPtr2
);

W32KAPI
DWORD
APIENTRY
NtGdiDdAddAttachedSurface(
    _In_ HANDLE hSurface,
    _In_ HANDLE hSurfaceAttached,
    IN OUT PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
);

W32KAPI
BOOL
APIENTRY
NtGdiDdAttachSurface(
    _In_ HANDLE hSurfaceFrom,
    _In_ HANDLE hSurfaceTo);

W32KAPI
DWORD
APIENTRY
NtGdiDdBlt(
    _In_ HANDLE hSurfaceDest,
    _In_ HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdCanCreateSurface(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdColorControl(
    _In_ HANDLE hSurface,
    IN OUT PDD_COLORCONTROLDATA puColorControlData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateDirectDrawObject(
    _In_ HDC hdc);

W32KAPI
DWORD
APIENTRY
NtGdiDdCreateSurface(
    _In_ HANDLE hDirectDraw,
    IN HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
    OUT HANDLE* puhSurface
);

W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateSurfaceObject(
    _In_ HANDLE hDirectDrawLocal,
    _In_ HANDLE hSurface,
    IN PDD_SURFACE_LOCAL puSurfaceLocal,
    IN PDD_SURFACE_MORE puSurfaceMore,
    IN PDD_SURFACE_GLOBAL puSurfaceGlobal,
    _In_ BOOL bComplete
);

W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteSurfaceObject(
    _In_ HANDLE hSurface);

W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteDirectDrawObject(
    _In_ HANDLE hDirectDrawLocal);

W32KAPI
DWORD
APIENTRY
NtGdiDdDestroySurface(
    _In_ HANDLE hSurface,
    _In_ BOOL bRealDestroy);

W32KAPI
DWORD
APIENTRY
NtGdiDdFlip(
    _In_ HANDLE hSurfaceCurrent,
    _In_ HANDLE hSurfaceTarget,
    _In_ HANDLE hSurfaceCurrentLeft,
    _In_ HANDLE hSurfaceTargetLeft,
    IN OUT PDD_FLIPDATA puFlipData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetAvailDriverMemory(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetBltStatus(
    _In_ HANDLE hSurface,
    IN OUT PDD_GETBLTSTATUSDATA puGetBltStatusData
);

W32KAPI
HDC
APIENTRY
NtGdiDdGetDC(
    _In_ HANDLE hSurface,
    IN PALETTEENTRY* puColorTable
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetDriverInfo(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETDRIVERINFODATA puGetDriverInfoData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetFlipStatus(
    _In_ HANDLE hSurface,
    IN OUT PDD_GETFLIPSTATUSDATA puGetFlipStatusData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetScanLine(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETSCANLINEDATA puGetScanLineData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdSetExclusiveMode(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdFlipToGDISurface(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdLock(
    _In_ HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData,
    _In_ HDC hdcClip
);

W32KAPI
BOOL
APIENTRY
NtGdiDdQueryDirectDrawObject(
    _In_ HANDLE hDirectDrawLocal,
    OUT PDD_HALINFO pHalInfo,
    OUT DWORD* pCallBackFlags,
    OUT OPTIONAL LPD3DNTHAL_CALLBACKS puD3dCallbacks,
    OUT OPTIONAL LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
    OUT OPTIONAL PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
    OUT OPTIONAL LPDDSURFACEDESC puD3dTextureFormats,
    OUT DWORD* puNumHeaps,
    OUT OPTIONAL VIDEOMEMORY* puvmList,
    OUT DWORD* puNumFourCC,
    OUT OPTIONAL DWORD* puFourCC
);

W32KAPI
BOOL
APIENTRY
NtGdiDdReenableDirectDrawObject(
    _In_ HANDLE hDirectDrawLocal,
    IN OUT BOOL* pubNewMode
);

W32KAPI
BOOL
APIENTRY
NtGdiDdReleaseDC(
    _In_ HANDLE hSurface);

W32KAPI
BOOL
APIENTRY
NtGdiDdResetVisrgn(
    _In_ HANDLE hSurface,
    _In_ HWND hwnd);

W32KAPI
DWORD
APIENTRY
NtGdiDdSetColorKey(
    _In_ HANDLE hSurface,
    IN OUT PDD_SETCOLORKEYDATA puSetColorKeyData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdSetOverlayPosition(
    _In_ HANDLE hSurfaceSource,
    _In_ HANDLE hSurfaceDestination,
    IN OUT PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdUnattachSurface(
    _In_ HANDLE hSurface,
    _In_ HANDLE hSurfaceAttached
);

W32KAPI
DWORD
APIENTRY
NtGdiDdUnlock(
    _In_ HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdUpdateOverlay(
    _In_ HANDLE hSurfaceDestination,
    _In_ HANDLE hSurfaceSource,
    IN OUT PDD_UPDATEOVERLAYDATA puUpdateOverlayData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdWaitForVerticalBlank(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDdGetDxHandle(
    _In_opt_ HANDLE hDirectDraw,
    _In_opt_ HANDLE hSurface,
    _In_ BOOL bRelease);

W32KAPI
BOOL
APIENTRY
NtGdiDdSetGammaRamp(
    _In_ HANDLE hDirectDraw,
    _In_ HDC hdc,
    _In_ LPVOID lpGammaRamp);

W32KAPI
DWORD
APIENTRY
NtGdiDdLockD3D(
    _In_ HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdUnlockD3D(
    _In_ HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdCreateD3DBuffer(
    _In_ HANDLE hDirectDraw,
    IN OUT HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
    IN OUT HANDLE* puhSurface
);

W32KAPI
DWORD
APIENTRY
NtGdiDdCanCreateD3DBuffer(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdDestroyD3DBuffer(
    _In_ HANDLE hSurface);

W32KAPI
BOOL
APIENTRY
NtGdiD3dContextCreate(
    _In_ HANDLE hDirectDrawLocal,
    _In_ HANDLE hSurfColor,
    _In_ HANDLE hSurfZ,
    IN OUT D3DNTHAL_CONTEXTCREATEI *pdcci
);

W32KAPI
DWORD
APIENTRY
NtGdiD3dContextDestroy(
    IN LPD3DNTHAL_CONTEXTDESTROYDATA pdcdd
);

W32KAPI
DWORD
APIENTRY
NtGdiD3dContextDestroyAll(
    OUT LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcdad
);

W32KAPI
DWORD
APIENTRY
NtGdiD3dValidateTextureStageState(
    IN OUT LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
);

W32KAPI
DWORD
APIENTRY
NtGdiD3dDrawPrimitives2(
    _In_ HANDLE hCmdBuf,
    _In_ HANDLE hVBuf,
    IN OUT LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    IN OUT FLATPTR* pfpVidMemCmd,
    IN OUT DWORD* pdwSizeCmd,
    IN OUT FLATPTR* pfpVidMemVtx,
    IN OUT DWORD* pdwSizeVtx
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetDriverState(
    IN OUT PDD_GETDRIVERSTATEDATA pdata
);

W32KAPI
DWORD
APIENTRY
NtGdiDdCreateSurfaceEx(
    _In_ HANDLE hDirectDraw,
    _In_ HANDLE hSurface,
    _In_ DWORD dwSurfaceHandle
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpCanCreateVideoPort(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_CANCREATEVPORTDATA puCanCreateVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpColorControl(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_VPORTCOLORDATA puVPortColorData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDvpCreateVideoPort(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_CREATEVPORTDATA puCreateVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpDestroyVideoPort(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_DESTROYVPORTDATA puDestroyVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpFlipVideoPort(
    _In_ HANDLE hVideoPort,
    _In_ HANDLE hDDSurfaceCurrent,
    _In_ HANDLE hDDSurfaceTarget,
    IN OUT PDD_FLIPVPORTDATA puFlipVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortBandwidth(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortField(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_GETVPORTFIELDDATA puGetVPortFieldData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortFlipStatus(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortInputFormats(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortLine(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_GETVPORTLINEDATA puGetVPortLineData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortOutputFormats(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortConnectInfo(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTCONNECTDATA puGetVPortConnectData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoSignalStatus(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_GETVPORTSIGNALDATA puGetVPortSignalData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpUpdateVideoPort(
    _In_ HANDLE hVideoPort,
    IN HANDLE* phSurfaceVideo,
    IN HANDLE* phSurfaceVbi,
    IN OUT PDD_UPDATEVPORTDATA puUpdateVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpWaitForVideoPortSync(
    _In_ HANDLE hVideoPort,
    IN OUT PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpAcquireNotification(
    _In_ HANDLE hVideoPort,
    IN OUT HANDLE* hEvent,
    IN LPDDVIDEOPORTNOTIFY pNotify
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpReleaseNotification(
    _In_ HANDLE hVideoPort,
    _In_ HANDLE hEvent);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompGuids(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompFormats(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompBuffInfo(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetInternalMoCompInfo(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_GETINTERNALMOCOMPDATA puGetInternalData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateMoComp(
    _In_ HANDLE hDirectDraw,
    IN OUT PDD_CREATEMOCOMPDATA puCreateMoCompData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdDestroyMoComp(
    _In_ HANDLE hMoComp,
    IN OUT PDD_DESTROYMOCOMPDATA puDestroyMoCompData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdBeginMoCompFrame(
    _In_ HANDLE hMoComp,
    IN OUT PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdEndMoCompFrame(
    _In_ HANDLE hMoComp,
    IN OUT PDD_ENDMOCOMPFRAMEDATA puEndFrameData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdRenderMoComp(
    _In_ HANDLE hMoComp,
    IN OUT PDD_RENDERMOCOMPDATA puRenderMoCompData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdQueryMoCompStatus(
    _In_ HANDLE hMoComp,
    IN OUT PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdAlphaBlt(
    _In_ HANDLE hSurfaceDest,
    _In_opt_ HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData
);

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

W32KAPI
BOOL
APIENTRY
NtGdiSetIcmMode(
    _In_ HDC hdc,
    _In_ ULONG nCommand,
    _In_ ULONG ulMode);

W32KAPI
HANDLE
APIENTRY
NtGdiCreateColorSpace(
    _In_ PLOGCOLORSPACEEXW pLogColorSpace);

W32KAPI
BOOL
APIENTRY
NtGdiDeleteColorSpace(
    _In_ HANDLE hColorSpace);

W32KAPI
BOOL
APIENTRY
NtGdiSetColorSpace(
    _In_ HDC hdc,
    _In_ HCOLORSPACE hColorSpace);

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

W32KAPI
BOOL
APIENTRY
NtGdiDeleteColorTransform(
    _In_ HDC hdc,
    _In_ HANDLE hColorTransform);

W32KAPI
BOOL
APIENTRY
NtGdiCheckBitmapBits(
    _In_ HDC hdc,
    _In_ HANDLE hColorTransform,
    _In_ PVOID pvBits,
    _In_ ULONG bmFormat,
    _In_ DWORD dwWidth,
    _In_ DWORD dwHeight,
    _In_ DWORD dwStride,
    OUT PBYTE paResults
);

W32KAPI
ULONG
APIENTRY
NtGdiColorCorrectPalette(
    _In_ HDC hdc,
    _In_ HPALETTE hpal,
    _In_ ULONG FirstEntry,
    _In_ ULONG NumberOfEntries,
    IN OUT PALETTEENTRY *ppalEntry,
    _In_ ULONG Command
);

W32KAPI
ULONG_PTR
APIENTRY
NtGdiGetColorSpaceforBitmap(
    _In_ HBITMAP hsurf);

_Success_(return != FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiGetDeviceGammaRamp(
    _In_ HDC hdc,
    _Out_writes_bytes_(sizeof(GAMMARAMP)) LPVOID lpGammaRamp);

W32KAPI
BOOL
APIENTRY
NtGdiSetDeviceGammaRamp(
    _In_ HDC hdc,
    _In_reads_bytes_(sizeof(GAMMARAMP)) LPVOID lpGammaRamp);

W32KAPI
BOOL
APIENTRY
NtGdiIcmBrushInfo(
    _In_ HDC hdc,
    _In_ HBRUSH hbrush,
    IN OUT PBITMAPINFO pbmiDIB,
    IN OUT PVOID pvBits,
    IN OUT ULONG *pulBits,
    OUT OPTIONAL DWORD *piUsage,
    OUT OPTIONAL BOOL *pbAlreadyTran,
    _In_ ULONG Command
);

W32KAPI
VOID
APIENTRY
NtGdiFlush(VOID);

W32KAPI
HDC
APIENTRY
NtGdiCreateMetafileDC(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiMakeInfoDC(
    _In_ HDC hdc,
    _In_ BOOL bSet);

W32KAPI
HANDLE
APIENTRY
NtGdiCreateClientObj(
    _In_ ULONG ulType);

W32KAPI
BOOL
APIENTRY
NtGdiDeleteClientObj(
    _In_ HANDLE h);

W32KAPI
LONG
APIENTRY
NtGdiGetBitmapBits(
    _In_ HBITMAP hbm,
    _In_ ULONG cjMax,
    OUT OPTIONAL PBYTE pjOut
);

W32KAPI
BOOL
APIENTRY
NtGdiDeleteObjectApp(
    _In_ HANDLE hobj);

W32KAPI
INT
APIENTRY
NtGdiGetPath(
    _In_ HDC hdc,
    OUT OPTIONAL LPPOINT pptlBuf,
    OUT OPTIONAL LPBYTE pjTypes,
    _In_ INT cptBuf
);

W32KAPI
HDC
APIENTRY
NtGdiCreateCompatibleDC(
    _In_opt_ HDC hdc);

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateDIBitmapInternal(
    _In_ HDC hdc,
    _In_ INT cx,
    _In_ INT cy,
    _In_ DWORD fInit,
    _In_opt_ LPBYTE pjInit,
    _In_opt_ LPBITMAPINFO pbmi,
    _In_ DWORD iUsage,
    _In_ UINT cjMaxInitInfo,
    _In_ UINT cjMaxBits,
    _In_ FLONG f,
    _In_ HANDLE hcmXform);

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateDIBSection(
    _In_ HDC hdc,
    _In_opt_ HANDLE hSectionApp,
    _In_ DWORD dwOffset,
    _In_ LPBITMAPINFO pbmi,
    _In_ DWORD iUsage,
    _In_ UINT cjHeader,
    _In_ FLONG fl,
    _In_ ULONG_PTR dwColorSpace,
    _Out_opt_ PVOID *ppvBits);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreateSolidBrush(
    _In_ COLORREF cr,
    _In_opt_ HBRUSH hbr);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreateDIBBrush(
    _In_ PVOID pv,
    _In_ FLONG fl,
    _In_ UINT  cj,
    _In_ BOOL  b8X8,
    _In_ BOOL bPen,
    _In_ PVOID pClient);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(
    _In_ HBITMAP hbm,
    _In_ BOOL bPen,
    _In_ BOOL b8X8);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(
    _In_ ULONG ulStyle,
    _In_ COLORREF clrr,
    _In_ BOOL bPen);

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
    _In_opt_ PULONG pulStyle,
    _In_ ULONG cjDIB,
    _In_ BOOL bOldStylePen,
    _In_opt_ HBRUSH hbrush);

W32KAPI
HRGN
APIENTRY
NtGdiCreateEllipticRgn(
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

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

W32KAPI
HRGN
APIENTRY
NtGdiExtCreateRegion(
    _In_opt_ LPXFORM px,
    _In_ DWORD cj,
    _In_ LPRGNDATA prgn);

W32KAPI
ULONG
APIENTRY
NtGdiMakeFontDir(
    _In_ FLONG flEmbed,
    _Out_writes_bytes_(cjFontDir) PBYTE pjFontDir,
    _In_ unsigned cjFontDir,
    _In_z_bytecount_(cjPathname) LPWSTR pwszPathname,
    _In_ unsigned cjPathname);

W32KAPI
BOOL
APIENTRY
NtGdiPolyDraw(
    _In_ HDC hdc,
    _In_count_(cpt) LPPOINT ppt,
    _In_count_(cpt) LPBYTE pjAttr,
    _In_ ULONG cpt);

W32KAPI
BOOL
APIENTRY
NtGdiPolyTextOutW(
    _In_ HDC hdc,
    _In_ POLYTEXTW *pptw,
    _In_ UINT cStr,
    _In_ DWORD dwCodePage);

W32KAPI
ULONG
APIENTRY
NtGdiGetServerMetaFileBits(
    _In_ HANDLE hmo,
    _In_ ULONG cjData,
    OUT OPTIONAL LPBYTE pjData,
    OUT PDWORD piType,
    OUT PDWORD pmm,
    OUT PDWORD pxExt,
    OUT PDWORD pyExt
);

W32KAPI
BOOL
APIENTRY
NtGdiEqualRgn(
    _In_ HRGN hrgn1,
    _In_ HRGN hrgn2);

_Must_inspect_result_
W32KAPI
BOOL
APIENTRY
NtGdiGetBitmapDimension(
    _In_ HBITMAP hbm,
    _When_(return != FALSE, _Out_) LPSIZE psize);

W32KAPI
UINT
APIENTRY
NtGdiGetNearestPaletteIndex(
    _In_ HPALETTE hpal,
    _In_ COLORREF crColor);

W32KAPI
BOOL
APIENTRY
NtGdiPtVisible(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

W32KAPI
BOOL
APIENTRY
NtGdiRectVisible(
    _In_ HDC hdc,
    _In_ LPRECT prc);

W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    _In_z_count_(cwc) WCHAR *pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ ULONG fl,
    _In_ DWORD dwPidTid,
    _In_opt_ DESIGNVECTOR *pdv);

W32KAPI
BOOL
APIENTRY
NtGdiResizePalette(
    _In_ HPALETTE hpal,
    _In_ UINT cEntry);

W32KAPI
BOOL
APIENTRY
NtGdiSetBitmapDimension(
    _In_ HBITMAP hbm,
    _In_ INT cx,
    _In_ INT cy,
    _In_opt_ LPSIZE psizeOut);

W32KAPI
INT
APIENTRY
NtGdiOffsetClipRgn(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

W32KAPI
INT
APIENTRY
NtGdiSetMetaRgn(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiSetTextJustification(
    _In_ HDC hdc,
    _In_ INT lBreakExtra,
    _In_ INT cBreak);

W32KAPI
INT
APIENTRY
NtGdiGetAppClipBox(
    _In_ HDC hdc,
    _Out_ LPRECT prc);

W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtentExW(
    _In_ HDC hdc,
    IN OPTIONAL LPWSTR lpwsz,
    _In_ ULONG cwc,
    _In_ ULONG dxMax,
    OUT OPTIONAL ULONG *pcCh,
    OUT OPTIONAL PULONG pdxOut,
    OUT LPSIZE psize,
    _In_ FLONG fl
);

W32KAPI
BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    _In_ HDC hdc,
    _In_ UINT wchFirst,
    _In_ ULONG cwch,
    IN OPTIONAL PWCHAR pwch,
    _In_ FLONG fl,
    OUT PVOID pvBuf
);

W32KAPI
DWORD
APIENTRY
NtGdiGetCharacterPlacementW(
    _In_ HDC hdc,
    IN LPWSTR pwsz,
    _In_ INT nCount,
    _In_ INT nMaxExtent,
    IN OUT LPGCP_RESULTSW pgcpw,
    _In_ DWORD dwFlags
);

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

W32KAPI
BOOL
APIENTRY
NtGdiBeginPath(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiSelectClipPath(
    _In_ HDC hdc,
    _In_ INT iMode);

W32KAPI
BOOL
APIENTRY
NtGdiCloseFigure(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiEndPath(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiAbortPath(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiFillPath(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiStrokeAndFillPath(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiStrokePath(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiWidenPath(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiFlattenPath(
    _In_ HDC hdc);

W32KAPI
NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID);

W32KAPI
HRGN
APIENTRY
NtGdiPathToRegion(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiSetMiterLimit(
    _In_ HDC hdc,
    _In_ DWORD dwNew,
    _Out_opt_ PDWORD pdwOut);

W32KAPI
BOOL
APIENTRY
NtGdiSetFontXform(
    _In_ HDC hdc,
    _In_ DWORD dwxScale,
    _In_ DWORD dwyScale);

_Success_(return != FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiGetMiterLimit(
    _In_ HDC hdc,
    _Out_ PDWORD pdwOut);

W32KAPI
BOOL
APIENTRY
NtGdiEllipse(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

W32KAPI
BOOL
APIENTRY
NtGdiRectangle(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

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

W32KAPI
BOOL
APIENTRY
NtGdiPlgBlt(
    _In_ HDC hdcTrg,
    _In_ LPPOINT pptlTrg,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_ HBITMAP hbmMask,
    _In_ INT xMask,
    _In_ INT yMask,
    _In_ DWORD crBackColor);

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

W32KAPI
BOOL
APIENTRY
NtGdiExtFloodFill(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ COLORREF crColor,
    _In_ UINT iFillType);

W32KAPI
BOOL
APIENTRY
NtGdiFillRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush);

W32KAPI
BOOL
APIENTRY
NtGdiFrameRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush,
    _In_ INT xWidth,
    _In_ INT yHeight);

W32KAPI
COLORREF
APIENTRY
NtGdiSetPixel(
    _In_ HDC hdcDst,
    _In_ INT x,
    _In_ INT y,
    _In_ COLORREF crColor);

W32KAPI
DWORD
APIENTRY
NtGdiGetPixel(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

W32KAPI
BOOL
APIENTRY
NtGdiStartPage(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiEndPage(
    _In_ HDC hdc);

W32KAPI
INT
APIENTRY
NtGdiStartDoc(
    _In_ HDC hdc,
    IN DOCINFOW *pdi,
    OUT BOOL *pbBanding,
    _In_ INT iJob
);

W32KAPI
BOOL
APIENTRY
NtGdiEndDoc(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiAbortDoc(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiUpdateColors(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthW(
    _In_ HDC hdc,
    _In_ UINT wcFirst,
    _In_ UINT cwc,
    IN OPTIONAL PWCHAR pwc,
    _In_ FLONG fl,
    OUT PVOID pvBuf
);

W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthInfo(
    _In_ HDC hdc,
    OUT PCHWIDTHINFO pChWidthInfo
);

W32KAPI
INT
APIENTRY
NtGdiDrawEscape(
    _In_ HDC hdc,
    _In_ INT iEsc,
    _In_ INT cjIn,
    IN OPTIONAL LPSTR pjIn
);

W32KAPI
INT
APIENTRY
NtGdiExtEscape(
    _In_ HDC hdc,
    IN OPTIONAL PWCHAR pDriver,
    _In_ INT nDriver,
    _In_ INT iEsc,
    _In_ INT cjIn,
    IN OPTIONAL LPSTR pjIn,
    _In_ INT cjOut,
    OUT OPTIONAL LPSTR pjOut
);

W32KAPI
ULONG
APIENTRY
NtGdiGetFontData(
    _In_ HDC hdc,
    _In_ DWORD dwTable,
    _In_ DWORD dwOffset,
    OUT OPTIONAL PVOID pvBuf,
    _In_ ULONG cjBuf
);

W32KAPI
ULONG
APIENTRY
NtGdiGetGlyphOutline(
    _In_ HDC hdc,
    IN WCHAR wch,
    _In_ UINT iFormat,
    OUT LPGLYPHMETRICS pgm,
    _In_ ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf,
    IN LPMAT2 pmat2,
    _In_ BOOL bIgnoreRotation
);

W32KAPI
BOOL
APIENTRY
NtGdiGetETM(
    _In_ HDC hdc,
    OUT EXTTEXTMETRIC *petm
);

W32KAPI
BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    OUT LPRASTERIZER_STATUS praststat,
    _In_ ULONG cjBytes
);

W32KAPI
ULONG
APIENTRY
NtGdiGetKerningPairs(
    _In_ HDC hdc,
    _In_ ULONG cPairs,
    OUT OPTIONAL KERNINGPAIR *pkpDst
);

W32KAPI
BOOL
APIENTRY
NtGdiMonoBitmap(
    _In_ HBITMAP hbm);

W32KAPI
HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    _In_ HBRUSH hbr,
    OUT UINT *piUsage
);

W32KAPI
ULONG
APIENTRY
NtGdiEnumObjects(
    _In_ HDC hdc,
    _In_ INT iObjectType,
    _In_ ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf
);

// Note from SDK:
//
// NtGdiResetDC
// The exact size of the buffer at pdm is pdm->dmSize + pdm->dmDriverExtra.
// But this can't be specified with current annotation language.
//
// typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;
//
// :end note.
W32KAPI
BOOL
APIENTRY
NtGdiResetDC(
    _In_ HDC hdc,
    _In_ LPDEVMODEW pdm,
    OUT PBOOL pbBanding,
    IN OPTIONAL VOID *pDriverInfo2, // this is "typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;"
    OUT VOID *ppUMdhpdev
);

W32KAPI
DWORD
APIENTRY
NtGdiSetBoundsRect(
    _In_ HDC hdc,
    _In_ LPRECT prc,
    _In_ DWORD f);

W32KAPI
BOOL
APIENTRY
NtGdiGetColorAdjustment(
    _In_ HDC hdc,
    _Out_ PCOLORADJUSTMENT pcaOut);

W32KAPI
BOOL
APIENTRY
NtGdiSetColorAdjustment(
    _In_ HDC hdc,
    _In_ PCOLORADJUSTMENT pca);

W32KAPI
BOOL
APIENTRY
NtGdiCancelDC(
    _In_ HDC hdc);

W32KAPI
HDC
APIENTRY
NtGdiOpenDCW(
    IN OPTIONAL PUNICODE_STRING pustrDevice,
    IN DEVMODEW *pdm,  // See note for NtGdiResetDC
    IN PUNICODE_STRING pustrLogAddr,
    _In_ ULONG iType,
    _In_ BOOL bDisplay,
    IN OPTIONAL HANDLE hspool,
    IN OPTIONAL VOID *pDriverInfo2, // this is  "typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;"
    OUT VOID *pUMdhpdev
);

W32KAPI
BOOL
APIENTRY
NtGdiGetDCDword(
    _In_ HDC hdc,
    _In_ UINT u,
    OUT DWORD *Result
);

_Success_(return!=FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiGetDCPoint(
    _In_ HDC hdc,
    _In_ UINT iPoint,
    _Out_ PPOINTL pptOut);

W32KAPI
BOOL
APIENTRY
NtGdiScaleViewportExtEx(
    _In_ HDC hdc,
    _In_ INT xNum,
    _In_ INT xDenom,
    _In_ INT yNum,
    _In_ INT yDenom,
    OUT OPTIONAL LPSIZE pszOut
);

W32KAPI
BOOL
APIENTRY
NtGdiScaleWindowExtEx(
    _In_ HDC hdc,
    _In_ INT xNum,
    _In_ INT xDenom,
    _In_ INT yNum,
    _In_ INT yDenom,
    OUT OPTIONAL LPSIZE pszOut
);

W32KAPI
BOOL
APIENTRY
NtGdiSetVirtualResolution(
    _In_ HDC hdc,
    _In_ INT cxVirtualDevicePixel,
    _In_ INT cyVirtualDevicePixel,
    _In_ INT cxVirtualDeviceMm,
    _In_ INT cyVirtualDeviceMm);

W32KAPI
BOOL
APIENTRY
NtGdiSetSizeDevice(
    _In_ HDC hdc,
    _In_ INT cxVirtualDevice,
    _In_ INT cyVirtualDevice);

_Success_(return !=FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiGetTransform(
    _In_ HDC hdc,
    _In_ DWORD iXform,
    _Out_ LPXFORM pxf);

W32KAPI
BOOL
APIENTRY
NtGdiModifyWorldTransform(
    _In_ HDC hdc,
    _In_opt_ LPXFORM pxf,
    _In_ DWORD iXform);

W32KAPI
BOOL
APIENTRY
NtGdiCombineTransform(
    _Out_ LPXFORM pxfDst,
    _In_ LPXFORM pxfSrc1,
    _In_ LPXFORM pxfSrc2);

W32KAPI
BOOL
APIENTRY
NtGdiTransformPoints(
    _In_ HDC hdc,
    _In_reads_(c) PPOINT pptIn,
    _Out_writes_(c) PPOINT pptOut,
    _In_ INT c,
    _In_ INT iMode);

W32KAPI
LONG
APIENTRY
NtGdiConvertMetafileRect(
    _In_ HDC hdc,
    _Inout_ PRECTL prect);

W32KAPI
INT
APIENTRY
NtGdiGetTextCharsetInfo(
    _In_ HDC hdc,
    OUT OPTIONAL LPFONTSIGNATURE lpSig,
    _In_ DWORD dwFlags
);

W32KAPI
BOOL
APIENTRY
NtGdiDoBanding(
    _In_ HDC hdc,
    _In_ BOOL bStart,
    OUT POINTL *pptl,
    OUT PSIZE pSize
);

W32KAPI
ULONG
APIENTRY
NtGdiGetPerBandInfo(
    _In_ HDC hdc,
    IN OUT PERBANDINFO *ppbi
);

W32KAPI
NTSTATUS
APIENTRY
NtGdiGetStats(
    _In_ HANDLE hProcess,
    _In_ INT iIndex,
    _In_ INT iPidType,
    OUT PVOID pResults,
    _In_ UINT cjResultSize
);

W32KAPI
BOOL
APIENTRY
NtGdiSetMagicColors(
    _In_ HDC hdc,
    IN PALETTEENTRY peMagic,
    _In_ ULONG Index
);

W32KAPI
HBRUSH
APIENTRY
NtGdiSelectBrush(
    _In_ HDC hdc,
    _In_ HBRUSH hbrush
);

W32KAPI
HPEN
APIENTRY
NtGdiSelectPen(
    _In_ HDC hdc,
    _In_ HPEN hpen);

W32KAPI
HBITMAP
APIENTRY
NtGdiSelectBitmap(
    _In_ HDC hdc,
    _In_ HBITMAP hbm);

W32KAPI
HFONT
APIENTRY
NtGdiSelectFont(
    _In_ HDC hdc,
    _In_ HFONT hf);

W32KAPI
INT
APIENTRY
NtGdiExtSelectClipRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ INT iMode);

W32KAPI
HPEN
APIENTRY
NtGdiCreatePen(
    _In_ INT iPenStyle,
    _In_ INT iPenWidth,
    _In_ COLORREF cr,
    _In_ HBRUSH hbr);

#ifdef _WINDOWBLT_NOTIFICATION_
W32KAPI
BOOL
APIENTRY
NtGdiBitBlt(
    _In_ HDC hdcDst,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ DWORD rop4,
    _In_ DWORD crBackColor,
    _In_ FLONG fl);
#else
W32KAPI
BOOL
APIENTRY
NtGdiBitBlt(
    _In_ HDC hdcDst,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ DWORD rop4,
    _In_ DWORD crBackColor);
#endif

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

W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtent(
    _In_ HDC hdc,
    _In_z_count_(cwc) LPWSTR lpwsz,
    _In_ INT cwc,
    _Out_ LPSIZE psize,
    _In_ UINT flOpts);

_Success_(return != FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiGetTextMetricsW(
    _In_ HDC hdc,
    _Out_bytecap_(cj) TMW_INTERNAL * ptm,
    _In_ ULONG cj);

W32KAPI
INT
APIENTRY
NtGdiGetTextFaceW(
    _In_ HDC hdc,
    _In_ INT cChar,
    OUT OPTIONAL LPWSTR pszOut,
    _In_ BOOL bAliasName
);

W32KAPI
INT
APIENTRY
NtGdiGetRandomRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ INT iRgn);

W32KAPI
BOOL
APIENTRY
NtGdiExtTextOutW(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ UINT flOpts,
    IN OPTIONAL LPRECT prcl,
    _In_z_count_(cwc) LPWSTR pwsz,
    _In_ INT cwc,
    IN OPTIONAL LPINT pdx,
    _In_ DWORD dwCodePage
);

W32KAPI
INT
APIENTRY
NtGdiIntersectClipRect(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

W32KAPI
HRGN
APIENTRY
NtGdiCreateRectRgn(
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

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

W32KAPI
BOOL
APIENTRY
NtGdiPolyPatBlt(
    _In_ HDC hdc,
    _In_ DWORD rop4,
    _In_ PPOLYPATBLT pPoly,
    _In_ DWORD Count,
    _In_ DWORD Mode);

W32KAPI
BOOL
APIENTRY
NtGdiUnrealizeObject(
    _In_ HANDLE h);

W32KAPI
HANDLE
APIENTRY
NtGdiGetStockObject(
    _In_ INT iObject);

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateCompatibleBitmap(
    _In_ HDC hdc,
    _In_ INT cx,
    _In_ INT cy);

W32KAPI
BOOL
APIENTRY
NtGdiLineTo(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y);

_Success_(return != FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiMoveTo(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _Out_opt_ LPPOINT pptOut);

_Success_(return != 0)
W32KAPI
INT
APIENTRY
NtGdiExtGetObjectW(
    _In_ HANDLE h,
    _In_ INT cj,
    _Out_opt_bytecap_(cj) LPVOID pvOut);

W32KAPI
INT
APIENTRY
NtGdiGetDeviceCaps(
    _In_ HDC hdc,
    _In_ INT i);

_Success_(return!=FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiGetDeviceCapsAll (
    _In_ HDC hdc,
    _Out_ PDEVCAPS pDevCaps);

W32KAPI
BOOL
APIENTRY
NtGdiStretchBlt(
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
    _In_ DWORD dwRop,
    _In_ DWORD dwBackColor);

_Success_(return != FALSE)
W32KAPI
BOOL
APIENTRY
NtGdiSetBrushOrg(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _Out_opt_ LPPOINT pptOut);

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateBitmap(
    _In_ INT cx,
    _In_ INT cy,
    _In_ UINT cPlanes,
    _In_ UINT cBPP,
    _In_opt_ LPBYTE pjInit);

W32KAPI
HPALETTE
APIENTRY
NtGdiCreateHalftonePalette(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiRestoreDC(
    _In_ HDC hdc,
    _In_ INT iLevel);

W32KAPI
INT
APIENTRY
NtGdiExcludeClipRect(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

W32KAPI
INT
APIENTRY
NtGdiSaveDC(
    _In_ HDC hdc);

W32KAPI
INT
APIENTRY
NtGdiCombineRgn(
    _In_ HRGN hrgnDst,
    _In_ HRGN hrgnSrc1,
    _In_opt_ HRGN hrgnSrc2,
    _In_ INT iMode);

W32KAPI
BOOL
APIENTRY
NtGdiSetRectRgn(
    _In_ HRGN hrgn,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom);

W32KAPI
LONG
APIENTRY
NtGdiSetBitmapBits(
    _In_ HBITMAP hbm,
    _In_ ULONG cj,
    _In_bytecount_(cj) PBYTE pjInit);

W32KAPI
INT
APIENTRY
NtGdiGetDIBitsInternal(
    _In_ HDC hdc,
    _In_ HBITMAP hbm,
    _In_ UINT iStartScan,
    _In_ UINT cScans,
    _Out_opt_ LPBYTE pBits,
    _Inout_ LPBITMAPINFO pbmi,
    _In_ UINT iUsage,
    _In_ UINT cjMaxBits,
    _In_ UINT cjMaxInfo);

W32KAPI
INT
APIENTRY
NtGdiOffsetRgn(
    _In_ HRGN hrgn,
    _In_ INT cx,
    _In_ INT cy);

_Success_(return!=ERROR)
W32KAPI
INT
APIENTRY
NtGdiGetRgnBox(
    _In_ HRGN hrgn,
    _Out_ LPRECT prcOut);

W32KAPI
BOOL
APIENTRY
NtGdiRectInRegion(
    _In_ HRGN hrgn,
    IN OUT LPRECT prcl
);

_Success_(return!=0)
W32KAPI
DWORD
APIENTRY
NtGdiGetBoundsRect(
    _In_ HDC hdc,
    _Out_ LPRECT prc,
    _In_ DWORD f);

W32KAPI
BOOL
APIENTRY
NtGdiPtInRegion(
    _In_ HRGN hrgn,
    _In_ INT x,
    _In_ INT y);

W32KAPI
COLORREF
APIENTRY
NtGdiGetNearestColor(
    _In_ HDC hdc,
    _In_ COLORREF cr);

W32KAPI
UINT
APIENTRY
NtGdiGetSystemPaletteUse(
    _In_ HDC hdc);

W32KAPI
UINT
APIENTRY
NtGdiSetSystemPaletteUse(
    _In_ HDC hdc,
    _In_ UINT ui);

_Success_(return!=0)
W32KAPI
ULONG
APIENTRY
NtGdiGetRegionData(
    _In_ HRGN hrgn,
    _In_ ULONG cjBuffer,
    _Out_opt_bytecap_(cjBuffer) LPRGNDATA lpRgnData);

W32KAPI
BOOL
APIENTRY
NtGdiInvertRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn);

INT
W32KAPI
APIENTRY
NtGdiAddFontResourceW(
    _In_z_count_(cwc) WCHAR *pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ FLONG f,
    _In_ DWORD dwPidTid,
    _In_opt_ DESIGNVECTOR *pdv);

#if (_WIN32_WINNT >= 0x0500)
W32KAPI
HFONT
APIENTRY
NtGdiHfontCreate(
    _In_bytecount_(cjElfw) ENUMLOGFONTEXDVW *pelfw,
    _In_ ULONG cjElfw,
    _In_ LFTYPE lft,
    _In_ FLONG fl,
    _In_ PVOID pvCliData);
#else
W32KAPI
HFONT
APIENTRY
NtGdiHfontCreate(
    _In_bytecount_(cjElfw) LPEXTLOGFONTW pelfw,
    _In_ ULONG cjElfw,
    _In_ LFTYPE lft,
    _In_ FLONG fl,
    _In_ PVOID pvCliData
);
#endif

W32KAPI
ULONG
APIENTRY
NtGdiSetFontEnumeration(
    _In_ ULONG ulType);

W32KAPI
BOOL
APIENTRY
NtGdiEnumFontClose(
    _In_ ULONG_PTR idEnum);

#if (_WIN32_WINNT >= 0x0500)
W32KAPI
BOOL
APIENTRY
NtGdiEnumFontChunk(
    _In_ HDC hdc,
    _In_ ULONG_PTR idEnum,
    _In_ ULONG cjEfdw,
    OUT ULONG *pcjEfdw,
    OUT PENUMFONTDATAW pefdw
);
#endif

W32KAPI
ULONG_PTR
APIENTRY
NtGdiEnumFontOpen(
    _In_ HDC hdc,
    _In_ ULONG iEnumType,
    _In_ FLONG flWin31Compat,
    _In_ ULONG cwchMax,
    IN OPTIONAL LPWSTR pwszFaceName,
    _In_ ULONG lfCharSet,
    OUT ULONG *pulCount
);

W32KAPI
INT
APIENTRY
NtGdiQueryFonts(
    OUT PUNIVERSAL_FONT_ID pufiFontList,
    _In_ ULONG nBufferSize,
    OUT PLARGE_INTEGER pTimeStamp
);

W32KAPI
BOOL
APIENTRY
NtGdiConsoleTextOut(
    _In_ HDC hdc,
    _In_ POLYTEXTW *lpto,
    _In_ UINT nStrings,
    _In_ RECTL *prclBounds);

W32KAPI
NTSTATUS
APIENTRY
NtGdiFullscreenControl(
    IN FULLSCREENCONTROL FullscreenCommand,
    IN PVOID FullscreenInput,
    _In_ DWORD FullscreenInputLength,
    OUT PVOID FullscreenOutput,
    IN OUT PULONG FullscreenOutputLength
);

W32KAPI
DWORD
APIENTRY
NtGdiGetCharSet(
    _In_ HDC hdc);

W32KAPI
BOOL
APIENTRY
NtGdiEnableEudc(
    _In_ BOOL b);

W32KAPI
BOOL
APIENTRY
NtGdiEudcLoadUnloadLink(
    IN OPTIONAL LPCWSTR pBaseFaceName,
    _In_ UINT cwcBaseFaceName,
    IN LPCWSTR pEudcFontPath,
    _In_ UINT cwcEudcFontPath,
    _In_ INT iPriority,
    _In_ INT iFontLinkType,
    _In_ BOOL bLoadLin
);

W32KAPI
UINT
APIENTRY
NtGdiGetStringBitmapW(
    _In_ HDC hdc,
    IN LPWSTR pwsz,
    _In_ UINT cwc,
    OUT BYTE *lpSB,
    _In_ UINT cj
);

W32KAPI
ULONG
APIENTRY
NtGdiGetEudcTimeStampEx(
    _In_opt_z_count_(cwcBaseFaceName) LPWSTR lpBaseFaceName,
    _In_ ULONG cwcBaseFaceName,
    _In_ BOOL bSystemTimeStamp);

W32KAPI
ULONG
APIENTRY
NtGdiQueryFontAssocInfo(
    _In_ HDC hdc);

#if (_WIN32_WINNT >= 0x0500)
W32KAPI
DWORD
APIENTRY
NtGdiGetFontUnicodeRanges(
    _In_ HDC hdc,
    _Out_opt_ LPGLYPHSET pgs);
#endif

#ifdef LANGPACK
W32KAPI
BOOL
APIENTRY
NtGdiGetRealizationInfo(
    _In_ HDC hdc,
    _Out_ PREALIZATION_INFO pri,
    _In_ HFONT hf);
#endif

W32KAPI
BOOL
APIENTRY
NtGdiAddRemoteMMInstanceToDC(
    _In_ HDC hdc,
    _In_ DOWNLOADDESIGNVECTOR *pddv,
    _In_ ULONG cjDDV);

W32KAPI
BOOL
APIENTRY
NtGdiUnloadPrinterDriver(
    _In_z_bytecount_(cbDriverName) LPWSTR pDriverName,
    _In_ ULONG cbDriverName);

W32KAPI
BOOL
APIENTRY
NtGdiEngAssociateSurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks);

W32KAPI
BOOL
APIENTRY
NtGdiEngEraseSurface(
    _In_ SURFOBJ *pso,
    _In_ RECTL *prcl,
    _In_ ULONG iColor);

W32KAPI
HBITMAP
APIENTRY
NtGdiEngCreateBitmap(
    _In_ SIZEL sizl,
    _In_ LONG lWidth,
    _In_ ULONG iFormat,
    _In_ FLONG fl,
    _In_opt_ PVOID pvBits);

W32KAPI
BOOL
APIENTRY
NtGdiEngDeleteSurface(
    _In_ HSURF hsurf);

W32KAPI
SURFOBJ*
APIENTRY
NtGdiEngLockSurface(
    _In_ HSURF hsurf);

W32KAPI
VOID
APIENTRY
NtGdiEngUnlockSurface(
    _In_ SURFOBJ *pso);

W32KAPI
BOOL
APIENTRY
NtGdiEngMarkBandingSurface(
    _In_ HSURF hsurf);

W32KAPI
HSURF
APIENTRY
NtGdiEngCreateDeviceSurface(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat);

W32KAPI
HBITMAP
APIENTRY
NtGdiEngCreateDeviceBitmap(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat);

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
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_ ULONG iMode);

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
    _When_(psoSrc, _In_) POINTL *pptlSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _When_(pbo, _In_) POINTL *pptlBrush,
    _In_ ROP4 rop4);

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
    _When_(psoMsk, _In_) POINTL *pptl,
    _In_ ULONG iMode);

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

W32KAPI
BOOL
APIENTRY
NtGdiEngDeletePalette(
    _In_ HPALETTE hPal);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiEngPaint(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN MIX mix
);

W32KAPI
BOOL
APIENTRY
NtGdiEngLineTo(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    IN RECTL *prclBounds,
    IN MIX mix
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiEngGradientFill(
    IN SURFOBJ *psoDest,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN TRIVERTEX *pVertex,
    _In_ ULONG nVertex,
    IN PVOID pMesh,
    _In_ ULONG nMesh,
    IN RECTL *prclExtents,
    IN POINTL *pptlDitherOrg,
    _In_ ULONG ulMode
);

W32KAPI
BOOL
APIENTRY
NtGdiEngTransparentBlt(
    IN SURFOBJ *psoDst,
    IN SURFOBJ *psoSrc,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDst,
    IN RECTL *prclSrc,
    _In_ ULONG iTransColor,
    _In_ ULONG ulReserved
);

W32KAPI
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
);

W32KAPI
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
    _In_ ULONG iMode,
    IN BRUSHOBJ *pbo,
    IN ROP4 rop4
);

W32KAPI
ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(
    IN XLATEOBJ *pxlo,
    _In_ ULONG iPal,
    _In_ ULONG cPal,
    OUT ULONG *pPal
);

W32KAPI
ULONG
APIENTRY
NtGdiCLIPOBJ_cEnumStart(
    IN CLIPOBJ *pco,
    _In_ BOOL bAll,
    _In_ ULONG iType,
    _In_ ULONG iDirection,
    _In_ ULONG cLimit
);

W32KAPI
BOOL
APIENTRY
NtGdiCLIPOBJ_bEnum(
    IN CLIPOBJ *pco,
    _In_ ULONG cj,
    OUT ULONG *pul
);

W32KAPI
PATHOBJ*
APIENTRY
NtGdiCLIPOBJ_ppoGetPath(
    IN CLIPOBJ *pco
);

W32KAPI
CLIPOBJ*
APIENTRY
NtGdiEngCreateClip(VOID);

W32KAPI
VOID
APIENTRY
NtGdiEngDeleteClip(
    IN CLIPOBJ*pco
);

W32KAPI
PVOID
APIENTRY
NtGdiBRUSHOBJ_pvAllocRbrush(
    IN BRUSHOBJ *pbo,
    _In_ ULONG cj
);

W32KAPI
PVOID
APIENTRY
NtGdiBRUSHOBJ_pvGetRbrush(
    IN BRUSHOBJ *pbo
);

W32KAPI
ULONG
APIENTRY
NtGdiBRUSHOBJ_ulGetBrushColor(
    IN BRUSHOBJ *pbo
);

W32KAPI
HANDLE
APIENTRY
NtGdiBRUSHOBJ_hGetColorTransform(
    IN BRUSHOBJ *pbo
);

W32KAPI
BOOL
APIENTRY
NtGdiXFORMOBJ_bApplyXform(
    IN XFORMOBJ *pxo,
    _In_ ULONG iMode,
    _In_ ULONG cPoints,
    IN  PVOID pvIn,
    OUT PVOID pvOut
);

W32KAPI
ULONG
APIENTRY
NtGdiXFORMOBJ_iGetXform(
    IN XFORMOBJ *pxo,
    OUT OPTIONAL XFORML *pxform
);

W32KAPI
VOID
APIENTRY
NtGdiFONTOBJ_vGetInfo(
    IN FONTOBJ *pfo,
    _In_ ULONG cjSize,
    OUT FONTINFO *pfi
);

W32KAPI
ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(
    IN FONTOBJ *pfo,
    _In_ ULONG iMode,
    _In_ ULONG cGlyph,
    IN HGLYPH *phg,
    OUT PVOID *ppvGlyph
);

W32KAPI
XFORMOBJ*
APIENTRY
NtGdiFONTOBJ_pxoGetXform(
    IN FONTOBJ *pfo
);

W32KAPI
IFIMETRICS*
APIENTRY
NtGdiFONTOBJ_pifi(
    IN FONTOBJ *pfo
);

W32KAPI
FD_GLYPHSET*
APIENTRY
NtGdiFONTOBJ_pfdg(
    IN FONTOBJ *pfo
);

W32KAPI
ULONG
APIENTRY
NtGdiFONTOBJ_cGetAllGlyphHandles(
    IN FONTOBJ *pfo,
    OUT OPTIONAL HGLYPH *phg
);

W32KAPI
PVOID
APIENTRY
NtGdiFONTOBJ_pvTrueTypeFontFile(
    IN FONTOBJ *pfo,
    OUT ULONG *pcjFile
);

W32KAPI
PFD_GLYPHATTR
APIENTRY
NtGdiFONTOBJ_pQueryGlyphAttrs(
    IN FONTOBJ *pfo,
    _In_ ULONG iMode
);

W32KAPI
BOOL
APIENTRY
NtGdiSTROBJ_bEnum(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos
);

W32KAPI
BOOL
APIENTRY
NtGdiSTROBJ_bEnumPositionsOnly(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos
);

W32KAPI
VOID
APIENTRY
NtGdiSTROBJ_vEnumStart(
    _Inout_ STROBJ *pstro);

W32KAPI
DWORD
APIENTRY
NtGdiSTROBJ_dwGetCodePage(
    IN STROBJ *pstro
);

W32KAPI
BOOL
APIENTRY
NtGdiSTROBJ_bGetAdvanceWidths(
    IN STROBJ*pstro,
    _In_ ULONG iFirst,
    _In_ ULONG c,
    OUT POINTQF*pptqD
);

W32KAPI
FD_GLYPHSET*
APIENTRY
NtGdiEngComputeGlyphSet(
    _In_ INT nCodePage,
    _In_ INT nFirstChar,
    _In_ INT cChars
);

W32KAPI
ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(
    IN XLATEOBJ *pxlo,
    _In_ ULONG iColor
);

W32KAPI
HANDLE
APIENTRY
NtGdiXLATEOBJ_hGetColorTransform(
    IN XLATEOBJ *pxlo
);

W32KAPI
VOID
APIENTRY
NtGdiPATHOBJ_vGetBounds(
    IN PATHOBJ *ppo,
    OUT PRECTFX prectfx
);

W32KAPI
BOOL
APIENTRY
NtGdiPATHOBJ_bEnum(
    IN PATHOBJ *ppo,
    OUT PATHDATA *ppd
);

W32KAPI
VOID
APIENTRY
NtGdiPATHOBJ_vEnumStart(
    IN PATHOBJ *ppo
);

W32KAPI
VOID
APIENTRY
NtGdiEngDeletePath(
    IN PATHOBJ *ppo
);

W32KAPI
VOID
APIENTRY
NtGdiPATHOBJ_vEnumStartClipLines(
    IN PATHOBJ *ppo,
    IN CLIPOBJ *pco,
    IN SURFOBJ *pso,
    IN LINEATTRS *pla
);

W32KAPI
BOOL
APIENTRY
NtGdiPATHOBJ_bEnumClipLines(
    IN PATHOBJ *ppo,
    _In_ ULONG cb,
    OUT CLIPLINE *pcl
);

W32KAPI
BOOL
APIENTRY
NtGdiEngCheckAbort(
    IN SURFOBJ *pso
);

W32KAPI
DHPDEV
APIENTRY
NtGdiGetDhpdev(
    IN HDEV hdev
);

W32KAPI
LONG
APIENTRY
NtGdiHT_Get8BPPFormatPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma
);

W32KAPI
LONG
APIENTRY
NtGdiHT_Get8BPPMaskPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    _In_ BOOL Use8BPPMaskPal,
    IN BYTE CMYMask,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma
);

W32KAPI
BOOL
APIENTRY
NtGdiUpdateTransform(
    _In_ HDC hdc
);

W32KAPI
DWORD
APIENTRY
NtGdiSetLayout(
    _In_ HDC hdc,
    _In_ LONG wox,
    _In_ DWORD dwLayout
);

W32KAPI
BOOL
APIENTRY
NtGdiMirrorWindowOrg(
    _In_ HDC hdc
);

W32KAPI
LONG
APIENTRY
NtGdiGetDeviceWidth(
    _In_ HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiSetPUMPDOBJ(
    IN HUMPD humpd,
    _In_ BOOL bStoreID,
    OUT HUMPD *phumpd,
    OUT BOOL *pbWOW64
);

W32KAPI
BOOL
APIENTRY
NtGdiBRUSHOBJ_DeleteRbrush(
    IN BRUSHOBJ *pbo,
    IN BRUSHOBJ *pboB
);

W32KAPI
BOOL
APIENTRY
NtGdiUMPDEngFreeUserMem(
    IN KERNEL_PVOID *ppv
);

W32KAPI
HBITMAP
APIENTRY
NtGdiSetBitmapAttributes(
    IN HBITMAP hbm,
    _In_ DWORD dwFlags
);

W32KAPI
HBITMAP
APIENTRY
NtGdiClearBitmapAttributes(
    IN HBITMAP hbm,
    _In_ DWORD dwFlags
);

W32KAPI
HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    _In_ HBRUSH hbm,
    _In_ DWORD dwFlags
);

W32KAPI
HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    _In_ HBRUSH hbm,
    _In_ DWORD dwFlags
);

W32KAPI
BOOL
APIENTRY
NtGdiDrawStream(
    _In_ HDC hdcDst,
    _In_ ULONG cjIn,
    IN VOID *pvIn
);

W32KAPI
BOOL
APIENTRY
NtGdiMakeObjectXferable(
    _In_ HANDLE h,
    _In_ DWORD dwProcessId
);

W32KAPI
BOOL
APIENTRY
NtGdiMakeObjectUnXferable(
    _In_ HANDLE h
);

W32KAPI
BOOL
APIENTRY
NtGdiInitSpool(VOID);

/* FIXME wrong prototypes fix the build */
W32KAPI
INT
APIENTRY
NtGdiGetSpoolMessage( DWORD u1,
                      DWORD u2,
                      DWORD u3,
                      DWORD u4);
#endif
