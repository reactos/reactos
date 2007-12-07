#ifndef __DDRAWEXH__
#define __DDRAWEXH__

#ifdef __cplusplus
extern "C" {
#endif

#include <ddraw.h>
DEFINE_GUID(CLSID_DirectDrawFactory,            0x4FD2A832, 0x86C8, 0x11D0, 0x8F, 0xCA, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D);
DEFINE_GUID(IID_IDirectDrawFactory,             0x4FD2A833, 0x86C8, 0x11D0, 0x8F, 0xCA, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D);
DEFINE_GUID(IID_IDirectDraw3,                   0x618F8AD4, 0x8B7A, 0x11D0, 0x8F, 0xCC, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D);

#define DDSCAPS_DATAEXCHANGE                    (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY)
#define DDERR_LOADFAILED                        MAKE_DDHRESULT( 901 )
#define DDERR_BADVERSIONINFO                    MAKE_DDHRESULT( 902 )
#define DDERR_BADPROCADDRESS                    MAKE_DDHRESULT( 903 )
#define DDERR_LEGACYUSAGE                       MAKE_DDHRESULT( 904 )

typedef struct IDirectDraw3 *LPDIRECTDRAW3;

#ifndef DIRECTDRAW_VERSION
    #define DDSD_LPSURFACE  0x00000800

    DEFINE_GUID( IID_IDirectDrawSurface3,   0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB );
    typedef struct IDirectDrawSurface3  *LPDIRECTDRAWSURFACE3;
    #if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
        #define INTERFACE IDirectDrawSurface3
        DECLARE_INTERFACE_( IDirectDrawSurface3, IUnknown )
        {
            STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
            STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
            STDMETHOD_(ULONG,Release) (THIS) PURE;
            STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE3) PURE;
            STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
            STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE3, LPRECT,DWORD, LPDDBLTFX) PURE;
            STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
            STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE3, LPRECT,DWORD) PURE;
            STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE3) PURE;
            STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
            STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
            STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE3, DWORD) PURE;
            STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE3*) PURE;
            STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
            STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
            STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER *) PURE;
            STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
            STDMETHOD(GetDC)(THIS_ HDC *) PURE;
            STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
            STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
            STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE *) PURE;
            STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
            STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
            STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
            STDMETHOD(IsLost)(THIS) PURE;
            STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
            STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
            STDMETHOD(Restore)(THIS) PURE;
            STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
            STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
            STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
            STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
            STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
            STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE3,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
            STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
            STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE3) PURE;
            STDMETHOD(GetDDInterface)(THIS_ LPVOID *) PURE;
            STDMETHOD(PageLock)(THIS_ DWORD) PURE;
            STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
            STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC, DWORD ) PURE;
        };
    #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
#undef  INTERFACE
#define INTERFACE IDirectDrawFactory

DECLARE_INTERFACE_(IDirectDrawFactory, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(CreateDirectDraw) (THIS_ GUID * pGUID, HWND hWnd, DWORD dwCoopLevelFlags, DWORD dwReserved, IUnknown *pUnkOuter, IDirectDraw **ppDirectDraw) PURE;
    STDMETHOD(DirectDrawEnumerate) (THIS_ LPDDENUMCALLBACK lpCallback, LPVOID lpContext) PURE;
};
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDraw3
    DECLARE_INTERFACE_(IDirectDraw3,IUnknown)
    {
        STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
        STDMETHOD_(ULONG,AddRef)(THIS) PURE;
        STDMETHOD_(ULONG,Release)(THIS) PURE;
        STDMETHOD(Compact)(THIS) PURE;
        STDMETHOD(CreateClipper)(THIS_ DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper, IUnknown *pUnkOuter) PURE;
        STDMETHOD(CreatePalette)(THIS_ DWORD dwFlags, LPPALETTEENTRY lpColorTable, LPDIRECTDRAWPALETTE *lplpDDPalette, IUnknown *pUnkOuter) PURE;
        STDMETHOD(CreateSurface)(THIS_ LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface, IUnknown *pUnkOuter) PURE;
        STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE *lplpDupDDSurface) PURE;
        STDMETHOD(EnumDisplayModes)(THIS_ DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback) PURE;
        STDMETHOD(EnumSurfaces)(THIS_ DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) PURE;
        STDMETHOD(FlipToGDISurface)(THIS) PURE;
        STDMETHOD(GetCaps)(THIS_ LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps) PURE;
        STDMETHOD(GetDisplayMode)(THIS_ LPDDSURFACEDESC lpDDSurfaceDesc) PURE;
        STDMETHOD(GetFourCCCodes)(THIS_ LPDWORD lpNumCodes, LPDWORD lpCodes) PURE;
        STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE *lplpGDIDDSurface) PURE;
        STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD lpdwFrequency) PURE;
        STDMETHOD(GetScanLine)(THIS_ LPDWORD lpdwScanLine) PURE;
        STDMETHOD(GetVerticalBlankStatus)(THIS_ BOOL *lpbIsInVB) PURE;
        STDMETHOD(Initialize)(THIS_ GUID *lpGUID) PURE;
        STDMETHOD(RestoreDisplayMode)(THIS) PURE;
        STDMETHOD(SetCooperativeLevel)(THIS_ HWND hWnd, DWORD dwFlags) PURE;
        STDMETHOD(SetDisplayMode)(THIS_ DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags) PURE;
        STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD dwFlags, HANDLE hEvent) PURE;
        STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS lpDDCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) PURE;
        STDMETHOD(GetSurfaceFromDC)(THIS_ HDC hdc, LPDIRECTDRAWSURFACE *pSurf) PURE;
    };
    #if !defined(__cplusplus) || defined(CINTERFACE)
        #define IDirectDraw3_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
        #define IDirectDraw3_AddRef(p)                   (p)->lpVtbl->AddRef(p)
        #define IDirectDraw3_Release(p)                  (p)->lpVtbl->Release(p)
        #define IDirectDraw3_Compact(p)                  (p)->lpVtbl->Compact(p)
        #define IDirectDraw3_CreateClipper(p,a,b,c)      (p)->lpVtbl->CreateClipper(p,a,b,c)
        #define IDirectDraw3_CreatePalette(p,a,b,c,d)    (p)->lpVtbl->CreatePalette(p,a,b,c,d)
        #define IDirectDraw3_CreateSurface(p,a,b,c)      (p)->lpVtbl->CreateSurface(p,a,b,c)
        #define IDirectDraw3_DuplicateSurface(p,a,b)     (p)->lpVtbl->DuplicateSurface(p,a,b)
        #define IDirectDraw3_EnumDisplayModes(p,a,b,c,d) (p)->lpVtbl->EnumDisplayModes(p,a,b,c,d)
        #define IDirectDraw3_EnumSurfaces(p,a,b,c,d)     (p)->lpVtbl->EnumSurfaces(p,a,b,c,d)
        #define IDirectDraw3_FlipToGDISurface(p)         (p)->lpVtbl->FlipToGDISurface(p)
        #define IDirectDraw3_GetCaps(p,a,b)              (p)->lpVtbl->GetCaps(p,a,b)
        #define IDirectDraw3_GetDisplayMode(p,a)         (p)->lpVtbl->GetDisplayMode(p,a)
        #define IDirectDraw3_GetFourCCCodes(p,a,b)       (p)->lpVtbl->GetFourCCCodes(p,a,b)
        #define IDirectDraw3_GetGDISurface(p,a)          (p)->lpVtbl->GetGDISurface(p,a)
        #define IDirectDraw3_GetMonitorFrequency(p,a)    (p)->lpVtbl->GetMonitorFrequency(p,a)
        #define IDirectDraw3_GetScanLine(p,a)            (p)->lpVtbl->GetScanLine(p,a)
        #define IDirectDraw3_GetVerticalBlankStatus(p,a) (p)->lpVtbl->GetVerticalBlankStatus(p,a)
        #define IDirectDraw3_Initialize(p,a)             (p)->lpVtbl->Initialize(p,a)
        #define IDirectDraw3_RestoreDisplayMode(p)       (p)->lpVtbl->RestoreDisplayMode(p)
        #define IDirectDraw3_SetCooperativeLevel(p,a,b)  (p)->lpVtbl->SetCooperativeLevel(p,a,b)
        #define IDirectDraw3_SetDisplayMode(p,a,b,c,d,e) (p)->lpVtbl->SetDisplayMode(p,a,b,c,d,e)
        #define IDirectDraw3_WaitForVerticalBlank(p,a,b) (p)->lpVtbl->WaitForVerticalBlank(p,a,b)
        #define IDirectDraw3_GetAvailableVidMem(p,a,b,c) (p)->lpVtbl->GetAvailableVidMem(p,a,b,c)
        #define IDirectDraw3_GetSurfaceFromDC(p,a,b)    (p)->lpVtbl->GetSurfaceFromDC(p,a,b)
    #else
        #define IDirectDraw3_QueryInterface(p,a,b)       (p)->QueryInterface(a,b)
        #define IDirectDraw3_AddRef(p)                   (p)->AddRef()
        #define IDirectDraw3_Release(p)                  (p)->Release()
        #define IDirectDraw3_Compact(p)                  (p)->Compact()
        #define IDirectDraw3_CreateClipper(p,a,b,c)      (p)->CreateClipper(a,b,c)
        #define IDirectDraw3_CreatePalette(p,a,b,c,d)    (p)->CreatePalette(a,b,c,d)
        #define IDirectDraw3_CreateSurface(p,a,b,c)      (p)->CreateSurface(a,b,c)
        #define IDirectDraw3_DuplicateSurface(p,a,b)     (p)->DuplicateSurface(a,b)
        #define IDirectDraw3_EnumDisplayModes(p,a,b,c,d) (p)->EnumDisplayModes(a,b,c,d)
        #define IDirectDraw3_EnumSurfaces(p,a,b,c,d)     (p)->EnumSurfaces(a,b,c,d)
        #define IDirectDraw3_FlipToGDISurface(p)         (p)->FlipToGDISurface()
        #define IDirectDraw3_GetCaps(p,a,b)              (p)->GetCaps(a,b)
        #define IDirectDraw3_GetDisplayMode(p,a)         (p)->GetDisplayMode(a)
        #define IDirectDraw3_GetFourCCCodes(p,a,b)       (p)->GetFourCCCodes(a,b)
        #define IDirectDraw3_GetGDISurface(p,a)          (p)->GetGDISurface(a)
        #define IDirectDraw3_GetMonitorFrequency(p,a)    (p)->GetMonitorFrequency(a)
        #define IDirectDraw3_GetScanLine(p,a)            (p)->GetScanLine(a)
        #define IDirectDraw3_GetVerticalBlankStatus(p,a) (p)->GetVerticalBlankStatus(a)
        #define IDirectDraw3_Initialize(p,a)             (p)->Initialize(a)
        #define IDirectDraw3_RestoreDisplayMode(p)       (p)->RestoreDisplayMode()
        #define IDirectDraw3_SetCooperativeLevel(p,a,b)  (p)->SetCooperativeLevel(a,b)
        #define IDirectDraw3_SetDisplayMode(p,a,b,c,d,e) (p)->SetDisplayMode(a,b,c,d,e)
        #define IDirectDraw3_WaitForVerticalBlank(p,a,b) (p)->WaitForVerticalBlank(a,b)
        #define IDirectDraw3_GetAvailableVidMem(p,a,b,c) (p)->GetAvailableVidMem(a,b,c)
        #define IDirectDraw3_GetSurfaceFromDC(p,a,b)    (p)->GetSurfaceFromDC(a,b)
    #endif
#endif

#ifdef __cplusplus
}
#endif
#endif
