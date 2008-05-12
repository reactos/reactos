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

ULONG   WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7);
ULONG   WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7, REFIID, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7, HDC);
HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX);
HRESULT WINAPI Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7, LPDDBLTBATCH, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE7, DWORD, DWORD, LPDIRECTDRAWSURFACE7, LPRECT, DWORD);
HRESULT WINAPI Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7, DWORD, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7, LPVOID, LPDDENUMSURFACESCALLBACK7);
HRESULT WINAPI Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7, DWORD, LPVOID,LPDDENUMSURFACESCALLBACK7);
HRESULT WINAPI Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 , LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7, REFGUID);
HRESULT WINAPI Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7, LPDDSCAPS2, LPDIRECTDRAWSURFACE7*);
HRESULT WINAPI Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7, DWORD dwFlags);
HRESULT WINAPI Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7, LPDDSCAPS2 pCaps);
HRESULT WINAPI Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWCLIPPER*);
HRESULT WINAPI Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7, HDC *);
HRESULT WINAPI Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7, LPLONG, LPLONG);
HRESULT WINAPI Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWPALETTE*);
HRESULT WINAPI Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7, LPDDPIXELFORMAT);
HRESULT WINAPI Main_DDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7, REFGUID, LPVOID, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7, HDC);
HRESULT WINAPI Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7, LPDIRECTDRAWCLIPPER);
HRESULT WINAPI Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7, LONG, LONG);
HRESULT WINAPI Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE7, LPDIRECTDRAWPALETTE);
HRESULT WINAPI Main_DDrawSurface_SetPriority (LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE7, REFGUID, LPVOID, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE7, DWORD, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7, DDSURFACEDESC2 *, DWORD);
HRESULT WINAPI Main_DDrawSurface_SetLOD(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7, LPRECT);
HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7, LPDIRECTDRAW, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7, LPRECT, LPDDSURFACEDESC2, DWORD, HANDLE);
HRESULT WINAPI Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT,
                                                DWORD, LPDDOVERLAYFX);
HRESULT WINAPI Main_DDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7, LPRECT);


IDirectDrawSurface7Vtbl DirectDrawSurface7_Vtable =
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
    Main_DDrawSurface_SetPriority,
    Main_DDrawSurface_GetPriority,
    Main_DDrawSurface_SetLOD,
    Main_DDrawSurface_GetLOD
};
