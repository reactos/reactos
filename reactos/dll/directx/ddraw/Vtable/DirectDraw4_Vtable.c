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
                                LPDIRECTDRAW4 iface,
                                REFIID id,
                                LPVOID *obj);

ULONG WINAPI
Main_DirectDraw_AddRef (LPDIRECTDRAW4 iface);

ULONG WINAPI
Main_DirectDraw_Release (LPDIRECTDRAW4 iface);

HRESULT WINAPI
Main_DirectDraw_Compact(LPDIRECTDRAW4 iface);

HRESULT WINAPI
Main_DirectDraw_CreateClipper(
                              LPDIRECTDRAW4 iface,
                              DWORD dwFlags,
                              LPDIRECTDRAWCLIPPER *ppClipper,
                              IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_CreatePalette(
                              LPDIRECTDRAW4 iface,
                              DWORD dwFlags,
                              LPPALETTEENTRY palent,
                              LPDIRECTDRAWPALETTE* ppPalette,
                              LPUNKNOWN pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_CreateSurface4(
                               LPDIRECTDRAW4 iface,
                               LPDDSURFACEDESC2 pDDSD,
                               LPDIRECTDRAWSURFACE4 *ppSurf,
                               IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_DuplicateSurface(
                                 LPDIRECTDRAW4 iface,
                                 LPDIRECTDRAWSURFACE4 src,
                                 LPDIRECTDRAWSURFACE4* dst);

HRESULT WINAPI
Main_DirectDraw_EnumDisplayModes4(
                                 LPDIRECTDRAW4 iface,
                                 DWORD dwFlags,
                                 LPDDSURFACEDESC2 pDDSD,
                                 LPVOID pContext,
                                 LPDDENUMMODESCALLBACK2 pCallback);

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(
                             LPDIRECTDRAW4 iface,
                             DWORD dwFlags,
                             LPDDSURFACEDESC2 lpDDSD2,
                             LPVOID context,
                             LPDDENUMSURFACESCALLBACK7 callback);

HRESULT WINAPI
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW4 iface);

HRESULT WINAPI
Main_DirectDraw_GetCaps(
                        LPDIRECTDRAW4 iface,
                        LPDDCAPS pDriverCaps,
                        LPDDCAPS pHELCaps);

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode (
                                LPDIRECTDRAW4 iface,
                                LPDDSURFACEDESC2 pDDSD);

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(
                               LPDIRECTDRAW4 iface,
                               LPDWORD lpNumCodes,
                               LPDWORD lpCodes);

HRESULT WINAPI
Main_DirectDraw_GetGDISurface(
                              LPDIRECTDRAW4 iface,
                              LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface);

HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency (
                                     LPDIRECTDRAW4 iface,
                                     LPDWORD lpFreq);


HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDIRECTDRAW4 iface,
                            LPDWORD lpdwScanLine);

HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(
                                       LPDIRECTDRAW4 iface,
                                       LPBOOL lpbIsInVB);

HRESULT WINAPI
Main_DirectDraw_Initialize (
                            LPDIRECTDRAW4 iface,
                            LPGUID lpGUID);


HRESULT WINAPI
Main_DirectDraw_RestoreDisplayMode (LPDIRECTDRAW4 iface);

HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel (
                                     LPDIRECTDRAW4 iface,
                                     HWND hwnd,
                                     DWORD cooplevel);

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode2(
                               LPDIRECTDRAW4 iface,
                               DWORD dwWidth,
                               DWORD dwHeight,
                               DWORD dwBPP,
                               DWORD dwRefreshRate,
                               DWORD dwFlags);

HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(
                                     LPDIRECTDRAW4 iface,
                                     DWORD dwFlags,
                                     HANDLE h);


HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem4(
                                    LPDIRECTDRAW4 iface,
                                    LPDDSCAPS2 ddscaps,
                                    LPDWORD dwTotal,
                                    LPDWORD dwFree);

HRESULT WINAPI
Main_DirectDraw_GetSurfaceFromDC(
                                 LPDIRECTDRAW4 iface,
                                 HDC hdc,
                                 LPDIRECTDRAWSURFACE7 *lpDDS);

HRESULT WINAPI
Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW4 iface);

HRESULT WINAPI
Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW4 iface);

HRESULT WINAPI
Main_DirectDraw_GetDeviceIdentifier(
                                     LPDIRECTDRAW4 iface,
                                     LPDDDEVICEIDENTIFIER pDDDI,
                                     DWORD dwFlags);



IDirectDraw4Vtbl DirectDraw4_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface4,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes4,
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
    Main_DirectDraw_GetAvailableVidMem4,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier
};
