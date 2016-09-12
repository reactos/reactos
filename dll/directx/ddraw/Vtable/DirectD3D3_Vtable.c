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

HRESULT WINAPI Main_D3D_QueryInterface(LPDIRECT3D3 iface, REFIID riid, LPVOID * ppvObj);
ULONG WINAPI Main_D3D_AddRef(LPDIRECT3D3 iface);
ULONG WINAPI Main_D3D_Release(LPDIRECT3D3 iface);
HRESULT WINAPI Main_D3D_EnumDevices(LPDIRECT3D3 iface, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI Main_D3D_CreateLight(LPDIRECT3D3 iface, LPDIRECT3DLIGHT *lplpDirect3DLight, IUnknown *pUnkOuter);
HRESULT WINAPI Main_D3D_CreateMaterial(LPDIRECT3D3 iface, LPDIRECT3DMATERIAL3 *lplpDirect3DMaterial3, IUnknown *pUnkOuter);
HRESULT WINAPI Main_D3D_CreateViewport(LPDIRECT3D3 iface, LPDIRECT3DVIEWPORT3 *lplpD3DViewport3, IUnknown *pUnkOuter);
HRESULT WINAPI Main_D3D_FindDevice(LPDIRECT3D3 iface, LPD3DFINDDEVICESEARCH lpD3DDFS, LPD3DFINDDEVICERESULT lpD3DFDR);
HRESULT WINAPI Main_D3D_CreateDevice3(LPDIRECT3D3 iface, REFCLSID rclsid,LPDIRECTDRAWSURFACE4 lpDDS, LPDIRECT3DDEVICE3 *lplpD3DDevice3,LPUNKNOWN lpUnk);
HRESULT WINAPI Main_D3D_CreateVertexBuffer3(LPDIRECT3D3 iface, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc, LPDIRECT3DVERTEXBUFFER *lplpD3DVertBuf,DWORD dwFlags,LPUNKNOWN lpUnk);
HRESULT WINAPI Main_D3D_EnumZBufferFormats(LPDIRECT3D3 iface, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,LPVOID lpContext);
HRESULT WINAPI Main_D3D_EvictManagedTextures(LPDIRECT3D3 iface);

IDirect3D3Vtbl IDirect3D3_Vtbl =
{
    Main_D3D_QueryInterface,
    Main_D3D_AddRef,
    Main_D3D_Release,
    Main_D3D_EnumDevices,
    Main_D3D_CreateLight,
    Main_D3D_CreateMaterial,
    Main_D3D_CreateViewport,
    Main_D3D_FindDevice,
    Main_D3D_CreateDevice3,
    Main_D3D_CreateVertexBuffer3,
    Main_D3D_EnumZBufferFormats,
    Main_D3D_EvictManagedTextures
};


