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

HRESULT WINAPI Main_D3D_QueryInterface(LPDIRECT3D7 iface, REFIID riid, LPVOID * ppvObject);
ULONG WINAPI Main_D3D_AddRef(LPDIRECT3D7 iface);
ULONG WINAPI Main_D3D_Release(LPDIRECT3D7 iface);
HRESULT WINAPI Main_D3D_EnumDevices(LPDIRECT3D7 iface, LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI Main_D3D_CreateDevice7(LPDIRECT3D7 iface, REFCLSID rclsid,LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice);
HRESULT WINAPI Main_D3D_CreateVertexBuffer7(LPDIRECT3D7 iface, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc, LPDIRECT3DVERTEXBUFFER7 *lplpD3DVertBuf,DWORD dwFlags);
HRESULT WINAPI Main_D3D_EnumZBufferFormats(LPDIRECT3D7 iface, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,LPVOID lpContext);
HRESULT WINAPI Main_D3D_EvictManagedTextures(LPDIRECT3D7 iface);

IDirect3D7Vtbl IDirect3D7_Vtbl =
{
    Main_D3D_QueryInterface,
    Main_D3D_AddRef,
    Main_D3D_Release,
    Main_D3D_EnumDevices,
    Main_D3D_CreateDevice7,
    Main_D3D_CreateVertexBuffer7,
    Main_D3D_EnumZBufferFormats,
    Main_D3D_EvictManagedTextures
};

