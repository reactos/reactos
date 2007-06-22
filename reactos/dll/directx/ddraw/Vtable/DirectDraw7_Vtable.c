
#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <ddrawgdi.h>

#if defined(_WIN32) && !defined(_NO_COM )
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#define IUnknown void
#if !defined(NT_BUILD_ENVIRONMENT) && !defined(WINNT)
        #define CO_E_NOTINITIALIZED 0x800401F0
#endif
#endif




HRESULT WINAPI
Main_DirectDraw_QueryInterface (
                                LPDIRECTDRAW7 iface,
                                REFIID id,
                                LPVOID *obj);

ULONG WINAPI
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface);

ULONG WINAPI
Main_DirectDraw_Release (LPDIRECTDRAW7 iface);

HRESULT WINAPI
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface);

HRESULT WINAPI
Main_DirectDraw_CreateClipper(
                              LPDIRECTDRAW7 iface,
                              DWORD dwFlags,
                              LPDIRECTDRAWCLIPPER *ppClipper,
                              IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_CreatePalette(
                              LPDIRECTDRAW7 iface,
                              DWORD dwFlags,
                              LPPALETTEENTRY palent,
                              LPDIRECTDRAWPALETTE* ppPalette,
                              LPUNKNOWN pUnkOuter);

HRESULT WINAPI 
Main_DirectDraw_CreateSurface4(
                               LPDIRECTDRAW7 iface,
                               LPDDSURFACEDESC2 pDDSD,
                               LPDIRECTDRAWSURFACE7 *ppSurf,
                               IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_DuplicateSurface(
                                 LPDIRECTDRAW7 iface,
                                 LPDIRECTDRAWSURFACE7 src,
                                 LPDIRECTDRAWSURFACE7* dst);

HRESULT WINAPI 
Main_DirectDraw_EnumDisplayModes(
                                 LPDIRECTDRAW7 iface,
                                 DWORD dwFlags,
                                 LPDDSURFACEDESC2 pDDSD,
                                 LPVOID pContext,
                                 LPDDENUMMODESCALLBACK2 pCallback);

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(
                             LPDIRECTDRAW7 iface,
                             DWORD dwFlags,
                             LPDDSURFACEDESC2 lpDDSD2,
                             LPVOID context,
                             LPDDENUMSURFACESCALLBACK7 callback);

HRESULT WINAPI
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface);

HRESULT WINAPI
Main_DirectDraw_GetCaps(
                        LPDIRECTDRAW7 iface,
                        LPDDCAPS pDriverCaps,
                        LPDDCAPS pHELCaps);

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode (
                                LPDIRECTDRAW7 iface,
                                LPDDSURFACEDESC2 pDDSD);

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(
                               LPDIRECTDRAW7 iface,
                               LPDWORD lpNumCodes,
                               LPDWORD lpCodes);

HRESULT WINAPI
Main_DirectDraw_GetGDISurface(
                              LPDIRECTDRAW7 iface,
                              LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface);

HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency (
                                     LPDIRECTDRAW7 iface,
                                     LPDWORD lpFreq);


HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface,
                            LPDWORD lpdwScanLine);

HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(
                                       LPDIRECTDRAW7 iface,
                                       LPBOOL lpbIsInVB);

HRESULT WINAPI
Main_DirectDraw_Initialize (
                            LPDIRECTDRAW7 iface,
                            LPGUID lpGUID);


HRESULT WINAPI
Main_DirectDraw_RestoreDisplayMode (LPDIRECTDRAW7 iface);

HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel (
                                     LPDIRECTDRAW7 iface,
                                     HWND hwnd,
                                     DWORD cooplevel);

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode(
                               LPDIRECTDRAW7 iface,
                               DWORD dwWidth,
                               DWORD dwHeight,
                               DWORD dwBPP,
                               DWORD dwRefreshRate,
                               DWORD dwFlags);

HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(
                                     LPDIRECTDRAW7 iface,
                                     DWORD dwFlags,
                                     HANDLE h);


HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem4(
                                    LPDIRECTDRAW7 iface,
                                    LPDDSCAPS2 ddscaps,
                                    LPDWORD dwTotal,
                                    LPDWORD dwFree);

HRESULT WINAPI 
Main_DirectDraw_GetSurfaceFromDC(
                                 LPDIRECTDRAW7 iface,
                                 HDC hdc,
                                 LPDIRECTDRAWSURFACE7 *lpDDS);

HRESULT WINAPI 
Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface);

HRESULT WINAPI 
Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface);

HRESULT WINAPI
Main_DirectDraw_GetDeviceIdentifier7(
                                     LPDIRECTDRAW7 iface,
                                     LPDDDEVICEIDENTIFIER2 pDDDI,
                                     DWORD dwFlags);

HRESULT WINAPI 
Main_DirectDraw_StartModeTest(
                              LPDIRECTDRAW7 iface,
                              LPSIZE pModes,
                              DWORD dwNumModes,
                              DWORD dwFlags);

HRESULT WINAPI
Main_DirectDraw_EvaluateMode(
                             LPDIRECTDRAW7 iface,
                             DWORD a,
                             DWORD* b);

IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,             /* (QueryInterface testing / devloping) */
    Main_DirectDraw_AddRef,                     /* (AddRef done) */
    Main_DirectDraw_Release,                    /* (QueryInterface testing / devloping) */
    Main_DirectDraw_Compact,                    /* (Compact done) */
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface4,             /* (CreateSurface4 testing / devloping) */
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,           /* (EnumDisplayModes testing / devloping) */
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,                    /* (GetCaps done) */
    Main_DirectDraw_GetDisplayMode,             /* (GetDisplayMode testing / devloping) */
    Main_DirectDraw_GetFourCCCodes,             /* (GetFourCCCodes done) */
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,        /* (GetMonitorFrequency done) */
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,                 /* (Initialize done) */
    Main_DirectDraw_RestoreDisplayMode,         /* (RestoreDisplayMode testing / devloping) */
    Main_DirectDraw_SetCooperativeLevel,        /* (SetCooperativeLevel testing / devloping) */
    Main_DirectDraw_SetDisplayMode,             /* (SetDisplayMode testing / devloping) */
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem4,        /* (GetAvailableVidMem4 done) */
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier7,       /* (GetDeviceIdentifier done) */
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};

