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
                                LPDIRECTDRAW2 iface,
                                REFIID id,
                                LPVOID *obj);

ULONG WINAPI
Main_DirectDraw_AddRef (LPDIRECTDRAW2 iface);

ULONG WINAPI
Main_DirectDraw_Release (LPDIRECTDRAW2 iface);

HRESULT WINAPI
Main_DirectDraw_Compact(LPDIRECTDRAW2 iface);

HRESULT WINAPI
Main_DirectDraw_CreateClipper(
                              LPDIRECTDRAW2 iface,
                              DWORD dwFlags,
                              LPDIRECTDRAWCLIPPER *ppClipper,
                              IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_CreatePalette(
                              LPDIRECTDRAW2 iface,
                              DWORD dwFlags,
                              LPPALETTEENTRY palent,
                              LPDIRECTDRAWPALETTE* ppPalette,
                              LPUNKNOWN pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_CreateSurface(
                               LPDIRECTDRAW2 iface,
                               LPDDSURFACEDESC pDDSD,
                               LPDIRECTDRAWSURFACE *ppSurf,
                               IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_DuplicateSurface(
                                 LPDIRECTDRAW2 iface,
                                 LPDIRECTDRAWSURFACE2 src,
                                 LPDIRECTDRAWSURFACE2* dst);

HRESULT WINAPI
Main_DirectDraw_EnumDisplayModes(
                                 LPDIRECTDRAW2 iface,
                                 DWORD dwFlags,
                                 LPDDSURFACEDESC pDDSD,
                                 LPVOID pContext,
                                 LPDDENUMMODESCALLBACK pCallback);

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(
                             LPDIRECTDRAW2 iface,
                             DWORD dwFlags,
                             LPDDSURFACEDESC lpDDSD,
                             LPVOID context,
                             LPDDENUMSURFACESCALLBACK callback);

HRESULT WINAPI
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW2 iface);

HRESULT WINAPI
Main_DirectDraw_GetCaps(
                        LPDIRECTDRAW2 iface,
                        LPDDCAPS pDriverCaps,
                        LPDDCAPS pHELCaps);

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode (
                                LPDIRECTDRAW2 iface,
                                LPDDSURFACEDESC pDDSD);

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(
                               LPDIRECTDRAW2 iface,
                               LPDWORD lpNumCodes,
                               LPDWORD lpCodes);

HRESULT WINAPI
Main_DirectDraw_GetGDISurface(
                              LPDIRECTDRAW2 iface,
                              LPDIRECTDRAWSURFACE *lplpGDIDDSSurface);

HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency (
                                     LPDIRECTDRAW2 iface,
                                     LPDWORD lpFreq);


HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDIRECTDRAW2 iface,
                            LPDWORD lpdwScanLine);

HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(
                                       LPDIRECTDRAW2 iface,
                                       LPBOOL lpbIsInVB);

HRESULT WINAPI
Main_DirectDraw_Initialize (
                            LPDIRECTDRAW2 iface,
                            LPGUID lpGUID);


HRESULT WINAPI
Main_DirectDraw_RestoreDisplayMode (LPDIRECTDRAW2 iface);

HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel (
                                     LPDIRECTDRAW2 iface,
                                     HWND hwnd,
                                     DWORD cooplevel);

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode2(
                               LPDIRECTDRAW2 iface,
                               DWORD dwWidth,
                               DWORD dwHeight,
                               DWORD dwBPP,
                               DWORD dwRefreshRate,
                               DWORD dwFlags);

HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(
                                     LPDIRECTDRAW2 iface,
                                     DWORD dwFlags,
                                     HANDLE h);


HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem(
                                    LPDIRECTDRAW2 iface,
                                    LPDDSCAPS ddscaps,
                                    LPDWORD dwTotal,
                                    LPDWORD dwFree);


IDirectDraw2Vtbl DirectDraw2_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode2,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem
};


