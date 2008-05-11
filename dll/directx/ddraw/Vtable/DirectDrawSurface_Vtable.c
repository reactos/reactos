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

ULONG   WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE);
ULONG   WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE);
HRESULT WINAPI Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE, REFIID, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);
HRESULT WINAPI Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE, LPDDBLTBATCH, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE, DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD);
HRESULT WINAPI Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE, DWORD, LPDIRECTDRAWSURFACE);
HRESULT WINAPI Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE, LPVOID, LPDDENUMSURFACESCALLBACK);
HRESULT WINAPI Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE, DWORD, LPVOID,LPDDENUMSURFACESCALLBACK);
HRESULT WINAPI Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE , LPDIRECTDRAWSURFACE, DWORD);
HRESULT WINAPI Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE, LPDDSCAPS, LPDIRECTDRAWSURFACE*);
HRESULT WINAPI Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE, DWORD dwFlags);
HRESULT WINAPI Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE, LPDDSCAPS pCaps);
HRESULT WINAPI Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE, LPDIRECTDRAWCLIPPER*);
HRESULT WINAPI Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE, HDC *);
HRESULT WINAPI Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE, DWORD);
HRESULT WINAPI Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE, LPLONG, LPLONG);
HRESULT WINAPI Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE, LPDIRECTDRAWPALETTE*);
HRESULT WINAPI Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE, LPDDPIXELFORMAT);
HRESULT WINAPI Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC);
HRESULT WINAPI Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE, HDC);
HRESULT WINAPI Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE, LPDIRECTDRAWCLIPPER);
HRESULT WINAPI Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE, LONG, LONG);
HRESULT WINAPI Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE, LPDIRECTDRAWPALETTE);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE, DWORD, LPDIRECTDRAWSURFACE);
HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE, LPVOID);
HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE, LPDIRECTDRAW, LPDDSURFACEDESC);
HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE, LPRECT, LPDDSURFACEDESC, DWORD, HANDLE);
HRESULT WINAPI Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT,
                                                DWORD, LPDDOVERLAYFX);
HRESULT WINAPI Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE);
HRESULT WINAPI Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE, LPRECT);


IDirectDrawSurfaceVtbl DirectDrawSurface_Vtable =
{
    Main_DDrawSurface_QueryInterface,
    Main_DDrawSurface_AddRef,                        /* (Compact done) */
    Main_DDrawSurface_Release,
    Main_DDrawSurface_AddAttachedSurface,
    Main_DDrawSurface_AddOverlayDirtyRect,
    Main_DDrawSurface_Blt,
    Main_DDrawSurface_BltBatch,
    Main_DDrawSurface_BltFast,
    Main_DDrawSurface_DeleteAttachedSurface,
    Main_DDrawSurface_EnumAttachedSurfaces,
    Main_DDrawSurface_EnumOverlayZOrders,
    Main_DDrawSurface_Flip,
    Main_DDrawSurface_GetAttachedSurface,
    Main_DDrawSurface_GetBltStatus,
    Main_DDrawSurface_GetCaps,
    Main_DDrawSurface_GetClipper,
    Main_DDrawSurface_GetColorKey,
    Main_DDrawSurface_GetDC,
    Main_DDrawSurface_GetFlipStatus,
    Main_DDrawSurface_GetOverlayPosition,
    Main_DDrawSurface_GetPalette,
    Main_DDrawSurface_GetPixelFormat,
    Main_DDrawSurface_GetSurfaceDesc,
    Main_DDrawSurface_Initialize,
    Main_DDrawSurface_IsLost,
    Main_DDrawSurface_Lock,
    Main_DDrawSurface_ReleaseDC,
    Main_DDrawSurface_Restore,
    Main_DDrawSurface_SetClipper,
    Main_DDrawSurface_SetColorKey,
    Main_DDrawSurface_SetOverlayPosition,
    Main_DDrawSurface_SetPalette,
    Main_DDrawSurface_Unlock,
    Main_DDrawSurface_UpdateOverlay,
    Main_DDrawSurface_UpdateOverlayDisplay,
    Main_DDrawSurface_UpdateOverlayZOrder,
};
