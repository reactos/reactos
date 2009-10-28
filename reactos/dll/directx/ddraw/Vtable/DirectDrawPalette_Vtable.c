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

ULONG WINAPI Main_DirectDrawPalette_Release(LPDIRECTDRAWPALETTE iface);
ULONG WINAPI Main_DirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE iface);
HRESULT WINAPI Main_DirectDrawPalette_Initialize(LPDIRECTDRAWPALETTE iface, LPDIRECTDRAW ddraw, DWORD dwFlags, LPPALETTEENTRY palent);
HRESULT WINAPI Main_DirectDrawPalette_GetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags, DWORD dwStart, DWORD dwCount, LPPALETTEENTRY palent);
HRESULT WINAPI Main_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags, DWORD dwStart, DWORD dwCount, LPPALETTEENTRY palent);
HRESULT WINAPI Main_DirectDrawPalette_GetCaps(LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps);
HRESULT WINAPI Main_DirectDrawPalette_QueryInterface(LPDIRECTDRAWPALETTE iface, REFIID refiid, LPVOID *obj);

IDirectDrawPaletteVtbl DirectDrawPalette_Vtable =
{
    Main_DirectDrawPalette_QueryInterface,
    Main_DirectDrawPalette_AddRef,
    Main_DirectDrawPalette_Release,
    Main_DirectDrawPalette_GetCaps,
    Main_DirectDrawPalette_GetEntries,
    Main_DirectDrawPalette_Initialize,
    Main_DirectDrawPalette_SetEntries
};

