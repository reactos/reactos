/*
* Copyright (C) the Wine project
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/


#ifdef __cplusplus
extern "C" {
#endif

    /*****************************************************************************
    * IDirectDraw3 interface
    */

#if defined( _WIN32 ) && !defined( _NO_COM )
    #define INTERFACE IDirectDraw3
    DECLARE_INTERFACE_(IDirectDraw3,IUnknown)
    {
        /*** IUnknown methods ***/
        /*00*/    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
        /*04*/    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
        /*08*/    STDMETHOD_(ULONG,Release)(THIS) PURE;
        /*** IDirectDraw2 methods ***/
        /*0c*/    STDMETHOD(Compact)(THIS) PURE;
        /*10*/    STDMETHOD(CreateClipper)(THIS_ DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper, IUnknown *pUnkOuter) PURE;
        /*14*/    STDMETHOD(CreatePalette)(THIS_ DWORD dwFlags, LPPALETTEENTRY lpColorTable, LPDIRECTDRAWPALETTE *lplpDDPalette, IUnknown *pUnkOuter) PURE;
        /*18*/    STDMETHOD(CreateSurface)(THIS_ LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface, IUnknown *pUnkOuter) PURE;
        /*1c*/    STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE *lplpDupDDSurface) PURE;
        /*20*/    STDMETHOD(EnumDisplayModes)(THIS_ DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback) PURE;
        /*24*/    STDMETHOD(EnumSurfaces)(THIS_ DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) PURE;
        /*28*/    STDMETHOD(FlipToGDISurface)(THIS) PURE;
        /*2c*/    STDMETHOD(GetCaps)(THIS_ LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps) PURE;
        /*30*/    STDMETHOD(GetDisplayMode)(THIS_ LPDDSURFACEDESC lpDDSurfaceDesc) PURE;
        /*34*/    STDMETHOD(GetFourCCCodes)(THIS_ LPDWORD lpNumCodes, LPDWORD lpCodes) PURE;
        /*38*/    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE *lplpGDIDDSurface) PURE;
        /*3c*/    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD lpdwFrequency) PURE;
        /*40*/    STDMETHOD(GetScanLine)(THIS_ LPDWORD lpdwScanLine) PURE;
        /*44*/    STDMETHOD(GetVerticalBlankStatus)(THIS_ BOOL *lpbIsInVB) PURE;
        /*48*/    STDMETHOD(Initialize)(THIS_ GUID *lpGUID) PURE;
        /*4c*/    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
        /*50*/    STDMETHOD(SetCooperativeLevel)(THIS_ HWND hWnd, DWORD dwFlags) PURE;
        /*54*/    STDMETHOD(SetDisplayMode)(THIS_ DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags) PURE;
        /*58*/    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD dwFlags, HANDLE hEvent) PURE;
        /* added in v2 */
        /*5c*/    STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS lpDDCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) PURE;
        /* added in v3 */
        /*60*/    STDMETHOD(GetSurfaceFromDC)(THIS_ HDC hdc, LPDIRECTDRAWSURFACE *pSurf) PURE;
    };
    #undef INTERFACE

    #if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
        #define IDirectDraw3_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
        #define IDirectDraw3_AddRef(p)             (p)->lpVtbl->AddRef(p)
        #define IDirectDraw3_Release(p)            (p)->lpVtbl->Release(p)
        /*** IDirectDraw methods ***/
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
        /*** IDirectDraw2 methods ***/
        #define IDirectDraw3_GetAvailableVidMem(p,a,b,c) (p)->lpVtbl->GetAvailableVidMem(p,a,b,c)
        /*** IDirectDraw3 methods ***/
        #define IDirectDraw3_GetSurfaceFromDC(p,a,b)    (p)->lpVtbl->GetSurfaceFromDC(p,a,b)
    #else
        /*** IUnknown methods ***/
        #define IDirectDraw3_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
        #define IDirectDraw3_AddRef(p)             (p)->AddRef()
        #define IDirectDraw3_Release(p)            (p)->Release()
        /*** IDirectDraw methods ***/
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
        /*** IDirectDraw2 methods ***/
        #define IDirectDraw3_GetAvailableVidMem(p,a,b,c) (p)->GetAvailableVidMem(a,b,c)
        /*** IDirectDraw3 methods ***/
        #define IDirectDraw3_GetSurfaceFromDC(p,a,b)    (p)->GetSurfaceFromDC(a,b)
    #endif
#undef INTERFACE
#endif

#ifdef __cplusplus
}
#endif
