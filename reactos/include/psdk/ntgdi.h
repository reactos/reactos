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

/* NtGdiQueryFonts */
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

typedef struct _POLYPATBLT POLYPATBLT, *PPOLYPATBLT;

/* NtGdiAddRemoteMMInstanceToDC */
typedef struct tagDOWNLOADDESIGNVECTOR
{
    UNIVERSAL_FONT_ID ufiBase;
    DESIGNVECTOR dv;
} DOWNLOADDESIGNVECTOR;

W32KAPI
BOOL
APIENTRY
NtGdiInit();

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    IN HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD pgi,
    IN DWORD iMode
);

W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesWInternal(
    IN HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD pgi,
    IN DWORD iMode,
    IN BOOL bSubset
);

W32KAPI
HPALETTE
APIENTRY
NtGdiCreatePaletteInternal(
    IN LPLOGPALETTE pLogPal,
    IN UINT cEntries
);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(
    IN HDC hdc,
    IN ULONG cjotm,
    OUT OPTIONAL OUTLINETEXTMETRICW *potmw,
    OUT TMDIFF *ptmd
);

W32KAPI
BOOL
APIENTRY
NtGdiGetAndSetDCDword(
    IN HDC hdc,
    IN UINT u,
    IN DWORD dwIn,
    OUT DWORD *pdwResult
);

W32KAPI
HANDLE
APIENTRY
NtGdiGetDCObject(
    IN  HDC hdc,
    IN  INT itype
);

W32KAPI
HDC
APIENTRY
NtGdiGetDCforBitmap(
    IN HBITMAP hsurf
);

W32KAPI
BOOL
APIENTRY
NtGdiGetMonitorID(
    IN  HDC hdc,
    IN  DWORD dwSize,
    OUT LPWSTR pszMonitorID
);

W32KAPI
INT
APIENTRY
NtGdiGetLinkedUFIs(
    IN HDC hdc,
    OUT OPTIONAL PUNIVERSAL_FONT_ID pufiLinkedUFIs,
    IN INT BufferSize
);

W32KAPI
BOOL
APIENTRY
NtGdiSetLinkedUFIs(
    IN HDC hdc,
    IN PUNIVERSAL_FONT_ID pufiLinks,
    IN ULONG uNumUFIs
);

W32KAPI
BOOL
APIENTRY
NtGdiGetUFI(
    IN  HDC hdc,
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
    IN HDC hdc,
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
    IN FLONG fl,
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
    IN HDC hdc,
    IN PVOID pvBuffer,
    IN ULONG cjBuffer,
    IN OPTIONAL PUNIVERSAL_FONT_ID pufi
);

W32KAPI
HANDLE
APIENTRY
NtGdiAddFontMemResourceEx(
    IN PVOID pvBuffer,
    IN DWORD cjBuffer,
    IN DESIGNVECTOR *pdv,
    IN ULONG cjDV,
    OUT DWORD *pNumFonts
);

W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(
    IN HANDLE hMMFont
);

W32KAPI
BOOL
APIENTRY
NtGdiUnmapMemFont(
    IN PVOID pvView
);

W32KAPI
BOOL
APIENTRY
NtGdiRemoveMergeFont(
    IN HDC hdc,
    IN UNIVERSAL_FONT_ID *pufi
);

W32KAPI
BOOL
APIENTRY
NtGdiAnyLinkedFonts(
    VOID
);

W32KAPI
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
);

W32KAPI
ULONG
APIENTRY 
NtGdiGetEmbedFonts(
    VOID
);

W32KAPI
BOOL
APIENTRY 
NtGdiChangeGhostFont(
    IN KERNEL_PVOID *pfontID,
    IN BOOL bLoad
);

W32KAPI
BOOL
APIENTRY 
NtGdiAddEmbFontToDC(
    IN HDC hdc,
    IN VOID **pFontID
);

W32KAPI
BOOL
APIENTRY
NtGdiFontIsLinked(
    IN HDC hdc
);

W32KAPI
ULONG_PTR
APIENTRY
NtGdiPolyPolyDraw(
    IN HDC hdc,
    IN PPOINT ppt,
    IN PULONG pcpt,
    IN ULONG ccpt,
    IN INT iFunc
);

W32KAPI
LONG
APIENTRY 
NtGdiDoPalette(
    IN HGDIOBJ hObj,
    IN WORD iStart,
    IN WORD cEntries,
    IN LPVOID pEntries,
    IN DWORD iFunc,
    IN BOOL bInbound
);

W32KAPI
BOOL
APIENTRY
NtGdiComputeXformCoefficients(
    IN HDC hdc
);

W32KAPI
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
);

W32KAPI
INT
APIENTRY
NtGdiDescribePixelFormat(
    IN HDC hdc,
    IN INT ipfd,
    IN UINT cjpfd,
    OUT PPIXELFORMATDESCRIPTOR ppfd
);

W32KAPI
BOOL
APIENTRY
NtGdiSetPixelFormat(
    IN HDC hdc,
    IN INT ipfd
);

W32KAPI
BOOL
APIENTRY
NtGdiSwapBuffers(
    IN HDC hdc
);

W32KAPI
INT
APIENTRY
NtGdiSetupPublicCFONT(
    IN HDC hdc,
    IN OPTIONAL HFONT hf,
    IN ULONG ulAve
);

W32KAPI
DWORD
APIENTRY
NtGdiDxgGenericThunk(
    IN ULONG_PTR ulIndex,
    IN ULONG_PTR ulHandle,
    IN OUT SIZE_T *pdwSizeOfPtr1,
    IN OUT  PVOID pvPtr1,
    IN OUT SIZE_T *pdwSizeOfPtr2,
    IN OUT  PVOID pvPtr2
);

W32KAPI
DWORD
APIENTRY
NtGdiDdAddAttachedSurface(
    IN HANDLE hSurface,
    IN HANDLE hSurfaceAttached,
    IN OUT PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
);

W32KAPI
BOOL
APIENTRY
NtGdiDdAttachSurface(
    IN HANDLE  hSurfaceFrom,
    IN HANDLE  hSurfaceTo
);

W32KAPI
DWORD
APIENTRY
NtGdiDdBlt(
    IN HANDLE hSurfaceDest,
    IN HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdCanCreateSurface(
    IN HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdColorControl(
    IN HANDLE hSurface,
    IN OUT PDD_COLORCONTROLDATA puColorControlData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateDirectDrawObject(
    IN HDC hdc
);

W32KAPI
DWORD
APIENTRY
NtGdiDdCreateSurface(
    IN HANDLE hDirectDraw,
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
    IN HANDLE hDirectDrawLocal,
    IN HANDLE hSurface,
    IN PDD_SURFACE_LOCAL puSurfaceLocal,
    IN PDD_SURFACE_MORE puSurfaceMore,
    IN PDD_SURFACE_GLOBAL puSurfaceGlobal,
    IN BOOL bComplete
);

W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteSurfaceObject(
    IN HANDLE hSurface
);

W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteDirectDrawObject(
    IN HANDLE hDirectDrawLocal
);

W32KAPI
DWORD
APIENTRY
NtGdiDdDestroySurface(
    IN HANDLE hSurface,
    IN BOOL bRealDestroy
);

W32KAPI
DWORD
APIENTRY
NtGdiDdFlip(
    IN HANDLE hSurfaceCurrent,
    IN HANDLE hSurfaceTarget,
    IN HANDLE hSurfaceCurrentLeft,
    IN HANDLE hSurfaceTargetLeft,
    IN OUT PDD_FLIPDATA puFlipData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetAvailDriverMemory(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetBltStatus(
    IN HANDLE hSurface,
    IN OUT PDD_GETBLTSTATUSDATA puGetBltStatusData
);

W32KAPI
HDC
APIENTRY
NtGdiDdGetDC(
    IN HANDLE hSurface,
    IN PALETTEENTRY* puColorTable
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetDriverInfo(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETDRIVERINFODATA puGetDriverInfoData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetFlipStatus(
    IN HANDLE hSurface,
    IN OUT PDD_GETFLIPSTATUSDATA puGetFlipStatusData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetScanLine(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETSCANLINEDATA puGetScanLineData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdSetExclusiveMode(
    IN HANDLE hDirectDraw,
    IN OUT PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdFlipToGDISurface(
    IN HANDLE hDirectDraw,
    IN OUT PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdLock(
    IN HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData,
    IN HDC hdcClip
);

W32KAPI
BOOL
APIENTRY
NtGdiDdQueryDirectDrawObject(
    IN HANDLE hDirectDrawLocal,
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
    IN HANDLE hDirectDrawLocal,
    IN OUT BOOL* pubNewMode
);

W32KAPI
BOOL
APIENTRY
NtGdiDdReleaseDC(
    IN HANDLE hSurface
);

W32KAPI
BOOL
APIENTRY
NtGdiDdResetVisrgn(
    IN HANDLE hSurface,
    IN HWND hwnd
);

W32KAPI
DWORD
APIENTRY
NtGdiDdSetColorKey(
    IN HANDLE hSurface,
    IN OUT PDD_SETCOLORKEYDATA puSetColorKeyData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdSetOverlayPosition(
    IN HANDLE hSurfaceSource,
    IN HANDLE hSurfaceDestination,
    IN OUT PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
);

W32KAPI
VOID
APIENTRY
NtGdiDdUnattachSurface(
    IN HANDLE hSurface,
    IN HANDLE hSurfaceAttached
);

W32KAPI
DWORD
APIENTRY
NtGdiDdUnlock(
    IN HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdUpdateOverlay(
    IN HANDLE hSurfaceDestination,
    IN HANDLE hSurfaceSource,
    IN OUT PDD_UPDATEOVERLAYDATA puUpdateOverlayData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdWaitForVerticalBlank(
    IN HANDLE hDirectDraw,
    IN OUT PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDdGetDxHandle(
    IN OPTIONAL HANDLE hDirectDraw,
    IN OPTIONAL HANDLE hSurface,
    IN BOOL bRelease
);

W32KAPI
BOOL
APIENTRY
NtGdiDdSetGammaRamp(
    IN HANDLE hDirectDraw,
    IN HDC hdc,
    IN LPVOID lpGammaRamp
);

W32KAPI
DWORD
APIENTRY
NtGdiDdLockD3D(
    IN HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdUnlockD3D(
    IN HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdCreateD3DBuffer(
    IN HANDLE hDirectDraw,
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
    IN HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdDestroyD3DBuffer(
    IN HANDLE hSurface
);

W32KAPI
DWORD
APIENTRY
NtGdiD3dContextCreate(
    IN HANDLE hDirectDrawLocal,
    IN HANDLE hSurfColor,
    IN HANDLE hSurfZ,
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
    IN HANDLE hCmdBuf,
    IN HANDLE hVBuf,
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
    IN HANDLE hDirectDraw,
    IN HANDLE hSurface,
    IN DWORD dwSurfaceHandle
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpCanCreateVideoPort(
    IN HANDLE hDirectDraw,
    IN OUT PDD_CANCREATEVPORTDATA puCanCreateVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpColorControl(
    IN HANDLE hVideoPort,
    IN OUT PDD_VPORTCOLORDATA puVPortColorData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDvpCreateVideoPort(
    IN HANDLE hDirectDraw,
    IN OUT PDD_CREATEVPORTDATA puCreateVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpDestroyVideoPort(
    IN HANDLE hVideoPort,
    IN OUT PDD_DESTROYVPORTDATA puDestroyVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpFlipVideoPort(
    IN HANDLE hVideoPort,
    IN HANDLE hDDSurfaceCurrent,
    IN HANDLE hDDSurfaceTarget,
    IN OUT PDD_FLIPVPORTDATA puFlipVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortBandwidth(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortField(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTFIELDDATA puGetVPortFieldData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortFlipStatus(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortInputFormats(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortLine(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTLINEDATA puGetVPortLineData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortOutputFormats(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoPortConnectInfo(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTCONNECTDATA puGetVPortConnectData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpGetVideoSignalStatus(
    IN HANDLE hVideoPort,
    IN OUT PDD_GETVPORTSIGNALDATA puGetVPortSignalData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpUpdateVideoPort(
    IN HANDLE hVideoPort,
    IN HANDLE* phSurfaceVideo,
    IN HANDLE* phSurfaceVbi,
    IN OUT PDD_UPDATEVPORTDATA puUpdateVPortData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpWaitForVideoPortSync(
    IN HANDLE hVideoPort,
    IN OUT PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpAcquireNotification(
    IN HANDLE hVideoPort,
    IN OUT HANDLE* hEvent,
    IN LPDDVIDEOPORTNOTIFY pNotify
);

W32KAPI
DWORD
APIENTRY
NtGdiDvpReleaseNotification(
    IN HANDLE hVideoPort,
    IN HANDLE hEvent
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompGuids(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompFormats(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetMoCompBuffInfo(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdGetInternalMoCompInfo(
    IN HANDLE hDirectDraw,
    IN OUT PDD_GETINTERNALMOCOMPDATA puGetInternalData
);

W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateMoComp(
    IN HANDLE hDirectDraw,
    IN OUT PDD_CREATEMOCOMPDATA puCreateMoCompData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdDestroyMoComp(
    IN HANDLE hMoComp,
    IN OUT PDD_DESTROYMOCOMPDATA puDestroyMoCompData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdBeginMoCompFrame(
    IN HANDLE hMoComp,
    IN OUT PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdEndMoCompFrame(
    IN HANDLE hMoComp,
    IN OUT PDD_ENDMOCOMPFRAMEDATA puEndFrameData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdRenderMoComp(
    IN HANDLE hMoComp,
    IN OUT PDD_RENDERMOCOMPDATA puRenderMoCompData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdQueryMoCompStatus(
    IN HANDLE hMoComp,
    IN OUT PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
);

W32KAPI
DWORD
APIENTRY
NtGdiDdAlphaBlt(
    IN HANDLE hSurfaceDest,
    IN OPTIONAL HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiGradientFill(
    IN HDC hdc,
    IN PTRIVERTEX pVertex,
    IN ULONG nVertex,
    IN PVOID pMesh,
    IN ULONG nMesh,
    IN ULONG ulMode
);

W32KAPI
BOOL
APIENTRY
NtGdiSetIcmMode(
    IN HDC hdc,
    IN ULONG nCommand,
    IN ULONG ulMode
);

W32KAPI
HANDLE
APIENTRY
NtGdiCreateColorSpace(
    IN PLOGCOLORSPACEEXW pLogColorSpace
);

W32KAPI
BOOL
APIENTRY
NtGdiDeleteColorSpace(
    IN HANDLE hColorSpace
);

W32KAPI
BOOL
APIENTRY
NtGdiSetColorSpace(
    IN HDC hdc,
    IN HCOLORSPACE hColorSpace
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiDeleteColorTransform(
    IN HDC hdc,
    IN HANDLE hColorTransform
);

W32KAPI
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
);

W32KAPI
ULONG
APIENTRY
NtGdiColorCorrectPalette(
    IN HDC hdc,
    IN HPALETTE hpal,
    IN ULONG FirstEntry,
    IN ULONG NumberOfEntries,
    IN OUT PALETTEENTRY *ppalEntry,
    IN ULONG Command
);

W32KAPI
ULONG_PTR
APIENTRY
NtGdiGetColorSpaceforBitmap(
    IN HBITMAP hsurf
);

W32KAPI
BOOL
APIENTRY
NtGdiGetDeviceGammaRamp(
    IN HDC hdc,
    OUT LPVOID lpGammaRamp
);

W32KAPI
BOOL
APIENTRY
NtGdiSetDeviceGammaRamp(
    IN HDC hdc,
    IN LPVOID lpGammaRamp
);

W32KAPI
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
);

W32KAPI
VOID
APIENTRY
NtGdiFlush();

W32KAPI
HDC
APIENTRY
NtGdiCreateMetafileDC(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiMakeInfoDC(
    IN HDC hdc,
    IN BOOL bSet
);

W32KAPI
HANDLE
APIENTRY
NtGdiCreateClientObj(
    IN ULONG ulType
);

W32KAPI
BOOL
APIENTRY
NtGdiDeleteClientObj(
    IN HANDLE h
);

W32KAPI
LONG
APIENTRY 
NtGdiGetBitmapBits(
    IN HBITMAP hbm,
    IN ULONG cjMax,
    OUT OPTIONAL PBYTE pjOut
);

W32KAPI
BOOL
APIENTRY
NtGdiDeleteObjectApp(
    IN HANDLE hobj
);

W32KAPI
INT
APIENTRY
NtGdiGetPath(
    IN HDC hdc,
    OUT OPTIONAL LPPOINT pptlBuf,
    OUT OPTIONAL LPBYTE pjTypes,
    IN INT cptBuf
);

W32KAPI
HDC
APIENTRY
NtGdiCreateCompatibleDC(
    IN HDC hdc
);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreateSolidBrush(
    IN COLORREF cr,
    IN OPTIONAL HBRUSH hbr
);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreateDIBBrush(
    IN PVOID pv,
    IN FLONG fl,
    IN UINT  cj,
    IN BOOL  b8X8,
    IN BOOL bPen,
    IN PVOID pClient
);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(
    IN HBITMAP hbm,
    IN BOOL bPen,
    IN BOOL b8X8
);

W32KAPI
HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(
    IN ULONG ulStyle,
    IN COLORREF clrr,
    IN BOOL bPen
);

W32KAPI
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
);

W32KAPI
HRGN
APIENTRY 
NtGdiCreateEllipticRgn(
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
);

W32KAPI
HRGN
APIENTRY 
NtGdiCreateRoundRectRgn(
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom,
    IN INT xWidth,
    IN INT yHeight
);

W32KAPI
HANDLE
APIENTRY
NtGdiCreateServerMetaFile(
    IN DWORD iType,
    IN ULONG cjData,
    IN LPBYTE pjData,
    IN DWORD mm,
    IN DWORD xExt,
    IN DWORD yExt
);

W32KAPI
HRGN
APIENTRY 
NtGdiExtCreateRegion(
    IN OPTIONAL LPXFORM px,
    IN DWORD cj,
    IN LPRGNDATA prgn
);

W32KAPI
ULONG
APIENTRY
NtGdiMakeFontDir(
    IN FLONG flEmbed,
    OUT PBYTE pjFontDir,
    IN unsigned cjFontDir,
    IN LPWSTR pwszPathname,
    IN unsigned cjPathname
);

W32KAPI
BOOL
APIENTRY
NtGdiPolyDraw(
    IN HDC hdc,
    IN LPPOINT ppt,
    IN LPBYTE pjAttr,
    IN ULONG cpt
);

W32KAPI
BOOL
APIENTRY
NtGdiPolyTextOutW(
    IN HDC hdc,
    IN POLYTEXTW *pptw,
    IN UINT cStr,
    IN DWORD dwCodePage
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiEqualRgn(
    IN HRGN hrgn1,
    IN HRGN hrgn2
);

W32KAPI
BOOL
APIENTRY
NtGdiGetBitmapDimension(
    IN HBITMAP hbm,
    OUT LPSIZE psize
);

W32KAPI
UINT
APIENTRY 
NtGdiGetNearestPaletteIndex(
    IN HPALETTE hpal,
    IN COLORREF crColor
);

W32KAPI
BOOL
APIENTRY
NtGdiPtVisible(
    IN HDC hdc,
    IN INT x,
    IN INT y
);

W32KAPI
BOOL
APIENTRY
NtGdiRectVisible(
    IN HDC hdc,
    IN LPRECT prc
);

W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN ULONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv
);

W32KAPI
BOOL
APIENTRY
NtGdiResizePalette(
    IN HPALETTE hpal,
    IN UINT cEntry
);

W32KAPI
BOOL
APIENTRY
NtGdiSetBitmapDimension(
    IN HBITMAP hbm,
    IN INT cx,
    IN INT cy,
    OUT OPTIONAL LPSIZE psizeOut
);

W32KAPI
INT
APIENTRY
NtGdiOffsetClipRgn(
    IN HDC hdc,
    IN INT x,
    IN INT y
);

W32KAPI
INT
APIENTRY
NtGdiSetMetaRgn(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiSetTextJustification(
    IN HDC hdc,
    IN INT lBreakExtra,
    IN INT cBreak
);

W32KAPI
INT
APIENTRY
NtGdiGetAppClipBox(
    IN HDC hdc,
    OUT LPRECT prc
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    IN HDC hdc,
    IN UINT wchFirst,
    IN ULONG cwch,
    IN OPTIONAL PWCHAR pwch,
    IN FLONG fl,
    OUT PVOID pvBuf
);                                               

W32KAPI
DWORD
APIENTRY
NtGdiGetCharacterPlacementW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN INT nCount,
    IN INT nMaxExtent,
    IN OUT LPGCP_RESULTSW pgcpw,
    IN DWORD dwFlags
);

W32KAPI
BOOL
APIENTRY
NtGdiAngleArc(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    IN DWORD dwRadius,
    IN DWORD dwStartAngle,
    IN DWORD dwSweepAngle
);

W32KAPI
BOOL
APIENTRY
NtGdiBeginPath(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiSelectClipPath(
    IN HDC hdc,
    IN INT iMode
);

W32KAPI
BOOL
APIENTRY
NtGdiCloseFigure(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiEndPath(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiAbortPath(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiFillPath(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiStrokeAndFillPath(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiStrokePath(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiWidenPath(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiFlattenPath(
    IN HDC hdc
);

W32KAPI
HRGN
APIENTRY 
NtGdiPathToRegion(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiSetMiterLimit(
    IN HDC hdc,
    IN DWORD dwNew,
    IN OUT OPTIONAL PDWORD pdwOut
);

W32KAPI
BOOL
APIENTRY
NtGdiSetFontXform(
    IN HDC hdc,
    IN DWORD dwxScale,
    IN DWORD dwyScale
);

W32KAPI
BOOL
APIENTRY
NtGdiGetMiterLimit(
    IN HDC hdc,
    OUT PDWORD pdwOut
);

W32KAPI
BOOL
APIENTRY
NtGdiEllipse(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
);

W32KAPI
BOOL
APIENTRY
NtGdiRectangle(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiExtFloodFill(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    IN COLORREF crColor,
    IN UINT iFillType
);

W32KAPI
BOOL
APIENTRY
NtGdiFillRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN HBRUSH hbrush
);

W32KAPI
BOOL
APIENTRY
NtGdiFrameRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN HBRUSH hbrush,
    IN INT xWidth,
    IN INT yHeight
);

W32KAPI
COLORREF
APIENTRY
NtGdiSetPixel(
    IN HDC hdcDst,
    IN INT x,
    IN INT y,
    IN COLORREF crColor
);

W32KAPI
DWORD
APIENTRY
NtGdiGetPixel(
    IN HDC hdc,
    IN INT x,
    IN INT y
);

W32KAPI
BOOL
APIENTRY
NtGdiStartPage(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiEndPage(
    IN HDC hdc
);

W32KAPI
INT
APIENTRY
NtGdiStartDoc(
    IN HDC hdc,
    IN DOCINFOW *pdi,
    OUT BOOL *pbBanding,
    IN INT iJob
);

W32KAPI
BOOL
APIENTRY
NtGdiEndDoc(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiAbortDoc(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiUpdateColors(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthW(
    IN HDC hdc,
    IN UINT wcFirst,
    IN UINT cwc,
    IN OPTIONAL PWCHAR pwc,
    IN FLONG fl,
    OUT PVOID pvBuf
);

W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthInfo(
    IN HDC hdc,
    OUT PCHWIDTHINFO pChWidthInfo
);

W32KAPI
INT
APIENTRY
NtGdiDrawEscape(
    IN HDC hdc,
    IN INT iEsc,
    IN INT cjIn,
    IN OPTIONAL LPSTR pjIn
);

W32KAPI
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
);

W32KAPI
ULONG
APIENTRY
NtGdiGetFontData(
    IN HDC hdc,
    IN DWORD dwTable,
    IN DWORD dwOffset,
    OUT OPTIONAL PVOID pvBuf,
    IN ULONG cjBuf
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiGetETM(
    IN HDC hdc,
    OUT EXTTEXTMETRIC *petm
);

W32KAPI
BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    OUT LPRASTERIZER_STATUS praststat,
    IN ULONG cjBytes
);

W32KAPI
ULONG
APIENTRY
NtGdiGetKerningPairs(
    IN HDC hdc,
    IN ULONG cPairs,
    OUT OPTIONAL KERNINGPAIR *pkpDst
);

W32KAPI
BOOL
APIENTRY
NtGdiMonoBitmap(
    IN HBITMAP hbm
);

W32KAPI
HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    IN HBRUSH hbr,
    OUT UINT *piUsage
);

W32KAPI
ULONG
APIENTRY
NtGdiEnumObjects(
    IN HDC hdc,
    IN INT iObjectType,
    IN ULONG cjBuf,
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
    IN HDC hdc,
    IN LPDEVMODEW pdm,
    OUT PBOOL pbBanding,
    IN OPTIONAL VOID *pDriverInfo2, // this is "typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;"
    OUT VOID *ppUMdhpdev
);

W32KAPI
DWORD
APIENTRY
NtGdiSetBoundsRect(
    IN HDC hdc,
    IN LPRECT prc,
    IN DWORD f
);

W32KAPI
BOOL
APIENTRY
NtGdiGetColorAdjustment(
    IN HDC hdc,
    OUT PCOLORADJUSTMENT pcaOut
);

W32KAPI
BOOL
APIENTRY
NtGdiSetColorAdjustment(
    IN HDC hdc,
    IN PCOLORADJUSTMENT pca
);

W32KAPI
BOOL
APIENTRY
NtGdiCancelDC(
    IN HDC hdc
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiGetDCDword(
    IN HDC hdc,
    IN UINT u,
    OUT DWORD *Result
);

W32KAPI
BOOL
APIENTRY
NtGdiGetDCPoint(
    IN HDC hdc,
    IN UINT iPoint,
    OUT PPOINTL pptOut
);

W32KAPI
BOOL
APIENTRY
NtGdiScaleViewportExtEx(
    IN HDC hdc,
    IN INT xNum,
    IN INT xDenom,
    IN INT yNum,
    IN INT yDenom,
    OUT OPTIONAL LPSIZE pszOut
);

W32KAPI
BOOL
APIENTRY
NtGdiScaleWindowExtEx(
    IN HDC hdc,
    IN INT xNum,
    IN INT xDenom,
    IN INT yNum,
    IN INT yDenom,
    OUT OPTIONAL LPSIZE pszOut
);

W32KAPI
BOOL
APIENTRY
NtGdiSetVirtualResolution(
    IN HDC hdc,
    IN INT cxVirtualDevicePixel,
    IN INT cyVirtualDevicePixel,
    IN INT cxVirtualDeviceMm,
    IN INT cyVirtualDeviceMm
);

W32KAPI
BOOL
APIENTRY
NtGdiSetSizeDevice(
    IN HDC hdc,
    IN INT cxVirtualDevice,
    IN INT cyVirtualDevice
);

W32KAPI
BOOL
APIENTRY
NtGdiGetTransform(
    IN HDC hdc,
    IN DWORD iXform,
    OUT LPXFORM pxf
);

W32KAPI
BOOL
APIENTRY
NtGdiModifyWorldTransform(
    IN HDC hdc,
    IN OPTIONAL LPXFORM pxf,
    IN DWORD iXform
);

W32KAPI
BOOL
APIENTRY
NtGdiCombineTransform(
    OUT LPXFORM pxfDst,
    IN LPXFORM pxfSrc1,
    IN LPXFORM pxfSrc2
);

W32KAPI
BOOL
APIENTRY
NtGdiTransformPoints(
    IN HDC hdc,
    IN PPOINT pptIn,
    OUT PPOINT pptOut,
    IN INT c,
    IN INT iMode
);

W32KAPI
LONG
APIENTRY 
NtGdiConvertMetafileRect(
    IN HDC hdc,
    IN OUT PRECTL prect
);

W32KAPI
INT
APIENTRY
NtGdiGetTextCharsetInfo(
    IN HDC hdc,
    OUT OPTIONAL LPFONTSIGNATURE lpSig,
    IN DWORD dwFlags
);

W32KAPI
BOOL
APIENTRY
NtGdiDoBanding(
    IN HDC hdc,
    IN BOOL bStart,
    OUT POINTL *pptl,
    OUT PSIZE pSize
);

W32KAPI
ULONG
APIENTRY
NtGdiGetPerBandInfo(
    IN HDC hdc,
    IN OUT PERBANDINFO *ppbi
);

W32KAPI
NTSTATUS
APIENTRY
NtGdiGetStats(
    IN HANDLE hProcess,
    IN INT iIndex,
    IN INT iPidType,
    OUT PVOID pResults,
    IN UINT cjResultSize
);

W32KAPI
BOOL
APIENTRY
NtGdiSetMagicColors(
    IN HDC hdc,
    IN PALETTEENTRY peMagic,
    IN ULONG Index
);

W32KAPI
HBRUSH
APIENTRY
NtGdiSelectBrush(
    IN HDC hdc,
    IN HBRUSH hbrush
);

W32KAPI
HPEN
APIENTRY 
NtGdiSelectPen(
    IN HDC hdc,
    IN HPEN hpen
);

W32KAPI
HBITMAP
APIENTRY
NtGdiSelectBitmap(
    IN HDC hdc,
    IN HBITMAP hbm
);

W32KAPI
HFONT
APIENTRY
NtGdiSelectFont(
    IN HDC hdc,
    IN HFONT hf
);

W32KAPI
INT
APIENTRY
NtGdiExtSelectClipRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN INT iMode
);

W32KAPI
HPEN
APIENTRY 
NtGdiCreatePen(
    IN INT iPenStyle,
    IN INT iPenWidth,
    IN COLORREF cr,
    IN HBRUSH hbr
);

#ifdef _WINDOWBLT_NOTIFICATION_
W32KAPI
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
);
#else
W32KAPI
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
    IN DWORD crBackColor
);
#endif

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtent(
    IN HDC hdc,
    IN LPWSTR lpwsz,
    IN INT cwc,
    OUT LPSIZE psize,
    IN UINT flOpts
);

W32KAPI
BOOL
APIENTRY
NtGdiGetTextMetricsW(
    IN HDC hdc,
    OUT TMW_INTERNAL * ptm,
    IN ULONG cj
);

W32KAPI
INT
APIENTRY
NtGdiGetTextFaceW(
    IN HDC hdc,
    IN INT cChar,
    OUT OPTIONAL LPWSTR pszOut,
    IN BOOL bAliasName
);

W32KAPI
INT
APIENTRY
NtGdiGetRandomRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN INT iRgn
);

W32KAPI
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
);

W32KAPI
INT
APIENTRY
NtGdiIntersectClipRect(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
);

W32KAPI
HRGN
APIENTRY 
NtGdiCreateRectRgn(
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
);

W32KAPI
BOOL
APIENTRY
NtGdiPatBlt(
    IN HDC hdcDst,
    IN INT x,
    IN INT y,
    IN INT cx,
    IN INT cy,
    IN DWORD rop4
);

W32KAPI
BOOL
APIENTRY
NtGdiPolyPatBlt(
    IN HDC hdc,
    IN DWORD rop4,
    IN PPOLYPATBLT pPoly,
    IN DWORD Count,
    IN DWORD Mode
);

W32KAPI
BOOL
APIENTRY
NtGdiUnrealizeObject(
    IN HANDLE h
);

W32KAPI
HANDLE
APIENTRY
NtGdiGetStockObject(
    IN INT iObject
);

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateCompatibleBitmap(
    IN HDC hdc,
    IN INT cx,
    IN INT cy
);

W32KAPI
BOOL
APIENTRY
NtGdiLineTo(
    IN HDC hdc,
    IN INT x,
    IN INT y
);

W32KAPI
BOOL
APIENTRY
NtGdiMoveTo(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    OUT OPTIONAL LPPOINT pptOut
);

W32KAPI
INT
APIENTRY
NtGdiExtGetObjectW(
    IN HANDLE h,
    IN INT cj,
    OUT OPTIONAL LPVOID pvOut
);

W32KAPI
INT
APIENTRY
NtGdiGetDeviceCaps(
    IN HDC hdc,
    IN INT i
);

W32KAPI
BOOL
APIENTRY
NtGdiGetDeviceCapsAll (
    IN HDC hdc,
    OUT PDEVCAPS pDevCaps
);

W32KAPI
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
);

W32KAPI
BOOL
APIENTRY
NtGdiSetBrushOrg(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    OUT LPPOINT pptOut
);

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateBitmap(
    IN INT cx,
    IN INT cy,
    IN UINT cPlanes,
    IN UINT cBPP,
    IN OPTIONAL LPBYTE pjInit
);

W32KAPI
HPALETTE
APIENTRY
NtGdiCreateHalftonePalette(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY
NtGdiRestoreDC(
    IN HDC hdc,
    IN INT iLevel
);

W32KAPI
INT
APIENTRY
NtGdiExcludeClipRect(
    IN HDC hdc,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
);

W32KAPI
INT
APIENTRY
NtGdiSaveDC(
    IN HDC hdc
);

W32KAPI
INT
APIENTRY
NtGdiCombineRgn(
    IN HRGN hrgnDst,
    IN HRGN hrgnSrc1,
    IN HRGN hrgnSrc2,
    IN INT iMode
);

W32KAPI
BOOL
APIENTRY
NtGdiSetRectRgn(
    IN HRGN hrgn,
    IN INT xLeft,
    IN INT yTop,
    IN INT xRight,
    IN INT yBottom
);

W32KAPI
LONG
APIENTRY 
NtGdiSetBitmapBits(
    IN HBITMAP hbm,
    IN ULONG cj,
    IN PBYTE pjInit
);

W32KAPI
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
);

W32KAPI
INT
APIENTRY
NtGdiOffsetRgn(
    IN HRGN hrgn,
    IN INT cx,
    IN INT cy
);

W32KAPI
INT
APIENTRY
NtGdiGetRandomRgn(
    IN HDC hDC, 
    OUT HRGN hDest,
    IN INT iCode
);

W32KAPI
INT
APIENTRY
NtGdiGetRgnBox(
    IN HRGN hrgn,
    OUT LPRECT prcOut
);

W32KAPI
BOOL
APIENTRY
NtGdiRectInRegion(
    IN HRGN hrgn,
    IN OUT LPRECT prcl
);

W32KAPI
DWORD
APIENTRY
NtGdiGetBoundsRect(
    IN HDC hdc,
    OUT LPRECT prc,
    IN DWORD f
);

W32KAPI
BOOL
APIENTRY
NtGdiPtInRegion(
    IN HRGN hrgn,
    IN INT x,
    IN INT y
);

W32KAPI
COLORREF
APIENTRY
NtGdiGetNearestColor(
    IN HDC hdc,
    IN COLORREF cr
);

W32KAPI
UINT
APIENTRY 
NtGdiGetSystemPaletteUse(
    IN HDC hdc
);

W32KAPI
UINT
APIENTRY 
NtGdiSetSystemPaletteUse(
    IN HDC hdc,
    IN UINT ui
);

W32KAPI
DWORD
APIENTRY
NtGdiGetRegionData(
    IN HRGN hrgn,
    IN DWORD nCount,
    OUT OPTIONAL LPRGNDATA lpRgnData
);

W32KAPI
BOOL
APIENTRY
NtGdiInvertRgn(
    IN HDC hdc,
    IN HRGN hrgn
);

INT
W32KAPI
APIENTRY
NtGdiAddFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN FLONG f,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv
);

#if (_WIN32_WINNT >= 0x0500)
W32KAPI
HFONT
APIENTRY
NtGdiHfontCreate(
    IN ENUMLOGFONTEXDVW *pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG  fl,
    IN PVOID pvCliData
);
#else
W32KAPI
HFONT
APIENTRY
NtGdiHfontCreate(
    IN LPEXTLOGFONTW pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG fl,
    IN PVOID pvCliData
);
#endif

W32KAPI
ULONG
APIENTRY
NtGdiSetFontEnumeration(
    IN ULONG ulType
);

W32KAPI
BOOL
APIENTRY
NtGdiEnumFontClose(
    IN ULONG_PTR idEnum
);

#if (_WIN32_WINNT >= 0x0500)
W32KAPI
BOOL
APIENTRY
NtGdiEnumFontChunk(
    IN HDC hdc,
    IN ULONG_PTR idEnum,
    IN ULONG cjEfdw,
    OUT ULONG *pcjEfdw,
    OUT PENUMFONTDATAW pefdw
);
#endif

W32KAPI
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
);

W32KAPI
INT
APIENTRY
NtGdiQueryFonts(
    OUT PUNIVERSAL_FONT_ID pufiFontList,
    IN ULONG nBufferSize,
    OUT PLARGE_INTEGER pTimeStamp 
);

W32KAPI
BOOL
APIENTRY
NtGdiConsoleTextOut(
    IN HDC hdc,
    IN POLYTEXTW *lpto,
    IN UINT nStrings,
    IN RECTL *prclBounds
);

W32KAPI
NTSTATUS
APIENTRY
NtGdiFullscreenControl(
    IN FULLSCREENCONTROL FullscreenCommand,
    IN PVOID FullscreenInput,
    IN DWORD FullscreenInputLength,
    OUT PVOID FullscreenOutput,
    IN OUT PULONG FullscreenOutputLength
);

W32KAPI
DWORD    
NtGdiGetCharSet(
    IN HDC hdc
);

W32KAPI
BOOL
APIENTRY 
NtGdiEnableEudc(
    IN BOOL
);

W32KAPI
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
);

W32KAPI
UINT
APIENTRY  
NtGdiGetStringBitmapW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN UINT cwc,
    OUT BYTE *lpSB,
    IN UINT cj
);

W32KAPI
ULONG
APIENTRY
NtGdiGetEudcTimeStampEx(
    IN OPTIONAL LPWSTR lpBaseFaceName,
    IN ULONG cwcBaseFaceName,
    IN BOOL bSystemTimeStamp
);

W32KAPI
ULONG
APIENTRY
NtGdiQueryFontAssocInfo(
    IN HDC hdc
);

#if (_WIN32_WINNT >= 0x0500)
W32KAPI
DWORD 
NtGdiGetFontUnicodeRanges(
    IN HDC hdc,
    OUT OPTIONAL LPGLYPHSET pgs
);
#endif

#ifdef LANGPACK
W32KAPI
BOOL 
NtGdiGetRealizationInfo(
    IN HDC hdc,
    OUT PREALIZATION_INFO pri,
    IN HFONT hf
);
#endif

W32KAPI
BOOL 
NtGdiAddRemoteMMInstanceToDC(
    IN HDC hdc,
    IN DOWNLOADDESIGNVECTOR *pddv,
    IN ULONG cjDDV
);

W32KAPI
BOOL
APIENTRY
NtGdiUnloadPrinterDriver(
    IN LPWSTR pDriverName,
    IN ULONG cbDriverName
);

W32KAPI
BOOL
APIENTRY
NtGdiEngAssociateSurface(
    IN HSURF hsurf,
    IN HDEV hdev,
    IN FLONG flHooks
);

W32KAPI
BOOL
APIENTRY
NtGdiEngEraseSurface(
    IN SURFOBJ *pso,
    IN RECTL *prcl,
    IN ULONG iColor
);

W32KAPI
HBITMAP
APIENTRY
NtGdiEngCreateBitmap(
    IN SIZEL sizl,
    IN LONG lWidth,
    IN ULONG iFormat,
    IN FLONG fl,
    IN OPTIONAL PVOID pvBits
);

W32KAPI
BOOL
APIENTRY
NtGdiEngDeleteSurface(
    IN HSURF hsurf
);

W32KAPI
SURFOBJ*
APIENTRY
NtGdiEngLockSurface(
    IN HSURF hsurf
);

W32KAPI
VOID
APIENTRY
NtGdiEngUnlockSurface(
    IN SURFOBJ *
);

W32KAPI
BOOL
APIENTRY
NtGdiEngMarkBandingSurface(
    IN HSURF hsurf
);

W32KAPI
HSURF
APIENTRY
NtGdiEngCreateDeviceSurface(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormatCompat
);

W32KAPI
HBITMAP
APIENTRY
NtGdiEngCreateDeviceBitmap(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormatCompat
);

W32KAPI
BOOL
APIENTRY
NtGdiEngCopyBits(
    IN SURFOBJ *psoDst,
    IN SURFOBJ *psoSrc,
    IN OPTIONAL CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDst,
    IN POINTL *pptlSrc
);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
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
);

W32KAPI
HPALETTE
APIENTRY
NtGdiEngCreatePalette(
    IN ULONG iMode,
    IN ULONG cColors,
    IN ULONG *pulColors,
    IN FLONG flRed,
    IN FLONG flGreen,
    IN FLONG flBlue
);

W32KAPI
BOOL
APIENTRY
NtGdiEngDeletePalette(
    IN HPALETTE hPal
);

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
    IN LONG x1,
    IN LONG y1,
    IN LONG x2,
    IN LONG y2,
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
    IN ULONG nVertex,
    IN PVOID pMesh,
    IN ULONG nMesh,
    IN RECTL *prclExtents,
    IN POINTL *pptlDitherOrg,
    IN ULONG ulMode
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
    IN ULONG iTransColor,
    IN ULONG ulReserved
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
    IN ULONG iMode,
    IN BRUSHOBJ *pbo,
    IN ROP4 rop4
);

W32KAPI
ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(
    IN XLATEOBJ *pxlo,
    IN ULONG iPal,
    IN ULONG cPal,
    OUT ULONG *pPal
);

W32KAPI
ULONG
APIENTRY
NtGdiCLIPOBJ_cEnumStart(
    IN CLIPOBJ *pco,
    IN BOOL bAll,
    IN ULONG iType,
    IN ULONG iDirection,
    IN ULONG cLimit
);

W32KAPI
BOOL
APIENTRY
NtGdiCLIPOBJ_bEnum(
    IN CLIPOBJ *pco,
    IN ULONG cj,
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
NtGdiEngCreateClip();

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
    IN ULONG cj
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
    IN ULONG iMode,
    IN ULONG cPoints,
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
    IN ULONG cjSize,
    OUT FONTINFO *pfi
);

W32KAPI
ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(
    IN FONTOBJ *pfo,
    IN ULONG iMode,
    IN ULONG cGlyph,
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
    IN ULONG iMode
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
    IN STROBJ *pstro
);

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
    IN ULONG iFirst,
    IN ULONG c,
    OUT POINTQF*pptqD
);

W32KAPI
FD_GLYPHSET*
APIENTRY
NtGdiEngComputeGlyphSet(
    IN INT nCodePage,
    IN INT nFirstChar,
    IN INT cChars
);

W32KAPI
ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(
    IN XLATEOBJ *pxlo,
    IN ULONG iColor
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
    IN ULONG cb,
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
    IN BOOL Use8BPPMaskPal,
    IN BYTE CMYMask,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma
);

W32KAPI
BOOL
NtGdiUpdateTransform(
    IN HDC hdc
);

W32KAPI
DWORD
APIENTRY
NtGdiSetLayout(
    IN HDC hdc,
    IN LONG wox,
    IN DWORD dwLayout
);

W32KAPI
BOOL
APIENTRY
NtGdiMirrorWindowOrg(
    IN HDC hdc
);

W32KAPI
LONG
APIENTRY 
NtGdiGetDeviceWidth(
    IN HDC hdc
);

W32KAPI
BOOL              
NtGdiSetPUMPDOBJ(
    IN HUMPD humpd,
    IN BOOL bStoreID,
    OUT HUMPD *phumpd,
    OUT BOOL *pbWOW64
);

W32KAPI
BOOL              
NtGdiBRUSHOBJ_DeleteRbrush(
    IN BRUSHOBJ *pbo,
    IN BRUSHOBJ *pboB
);

W32KAPI
BOOL              
NtGdiUMPDEngFreeUserMem(
    IN KERNEL_PVOID *ppv
);

W32KAPI
HBITMAP
APIENTRY
NtGdiSetBitmapAttributes(
    IN HBITMAP hbm,
    IN DWORD dwFlags
);

W32KAPI
HBITMAP
APIENTRY
NtGdiClearBitmapAttributes(
    IN HBITMAP hbm,
    IN DWORD dwFlags
);

W32KAPI
HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    IN HBRUSH hbm,
    IN DWORD dwFlags
);

W32KAPI
HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    IN HBRUSH hbm,
    IN DWORD dwFlags
);

W32KAPI
BOOL
APIENTRY
NtGdiDrawStream(
    IN HDC hdcDst,
    IN ULONG cjIn,
    IN VOID *pvIn
);

W32KAPI
BOOL
APIENTRY
NtGdiMakeObjectXferable(
    IN HANDLE h,
    IN DWORD dwProcessId
);

W32KAPI
BOOL
APIENTRY
NtGdiMakeObjectUnXferable(
    IN HANDLE h
);

W32KAPI
BOOL
APIENTRY
NtGdiInitSpool();

/* FIXME wrong prototypes fix the build */
W32KAPI
INT
APIENTRY
NtGdiGetSpoolMessage( DWORD u1, 
                      DWORD u2, 
                      DWORD u3, 
                      DWORD u4);
#endif
