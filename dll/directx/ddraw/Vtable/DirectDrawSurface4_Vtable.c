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

ULONG   WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE4);
ULONG   WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE4);
HRESULT WINAPI Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE4, REFIID, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE4, HDC);
HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE4, LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DWORD, LPDDBLTFX);
HRESULT WINAPI Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE4, LPDDBLTBATCH, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE4, DWORD, DWORD, LPDIRECTDRAWSURFACE4, LPRECT, DWORD);
HRESULT WINAPI Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE4, DWORD, LPDIRECTDRAWSURFACE4);
HRESULT WINAPI Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE4, LPVOID, LPDDENUMSURFACESCALLBACK2);
HRESULT WINAPI Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE4, DWORD, LPVOID,LPDDENUMSURFACESCALLBACK2);
HRESULT WINAPI Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE4 , LPDIRECTDRAWSURFACE4, DWORD);
HRESULT WINAPI Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE4, REFGUID);
HRESULT WINAPI Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE4, LPDDSCAPS2, LPDIRECTDRAWSURFACE4*);
HRESULT WINAPI Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE4, DWORD dwFlags);
HRESULT WINAPI Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE4, LPDDSCAPS2 pCaps);
HRESULT WINAPI Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE4, LPDIRECTDRAWCLIPPER*);
HRESULT WINAPI Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE4, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE4, HDC *);
HRESULT WINAPI Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE4, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE4, DWORD);
HRESULT WINAPI Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE4, LPLONG, LPLONG);
HRESULT WINAPI Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE4, LPDIRECTDRAWPALETTE*);
HRESULT WINAPI Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE4, LPDDPIXELFORMAT);
HRESULT WINAPI Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE4, REFGUID, LPVOID, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE4, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE4, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE4);
HRESULT WINAPI Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE4, DWORD);
HRESULT WINAPI Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE4, DWORD);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE4, HDC);
HRESULT WINAPI Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE4, LPDIRECTDRAWCLIPPER);
HRESULT WINAPI Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE4, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE4, LONG, LONG);
HRESULT WINAPI Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE4, LPDIRECTDRAWPALETTE);
HRESULT WINAPI Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE4, REFGUID, LPVOID, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE4, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE4, DWORD, LPDIRECTDRAWSURFACE4);
HRESULT WINAPI Main_DDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE4, DDSURFACEDESC2 *, DWORD);
HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE4, LPRECT);
HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE4, LPDIRECTDRAW, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE4, LPRECT, LPDDSURFACEDESC2, DWORD, HANDLE);
HRESULT WINAPI Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE4);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE4, LPRECT, LPDIRECTDRAWSURFACE4, LPRECT,
                                                DWORD, LPDDOVERLAYFX);
HRESULT WINAPI Main_DDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE4);
HRESULT WINAPI Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4);
HRESULT WINAPI Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE4, LPRECT);


IDirectDrawSurface4Vtbl DirectDrawSurface4_Vtable =
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
    Main_DDrawSurface_GetDDInterface,
    Main_DDrawSurface_PageLock,
    Main_DDrawSurface_PageUnlock,
    Main_DDrawSurface_SetSurfaceDesc,
    Main_DDrawSurface_SetPrivateData,
    Main_DDrawSurface_GetPrivateData,
    Main_DDrawSurface_FreePrivateData,
    Main_DDrawSurface_GetUniquenessValue,
    Main_DDrawSurface_ChangeUniquenessValue,
};
