#include "rosdraw.h"

#include <string.h>

HRESULT WINAPI
Main_D3D_QueryInterface(LPDIRECT3D iface, REFIID riid, LPVOID * ppvObj)
{
    DX_WINDBG_trace();
    DX_STUB;
}

ULONG WINAPI
Main_D3D_AddRef(LPDIRECT3D iface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

ULONG WINAPI
Main_D3D_Release(LPDIRECT3D iface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_Initialize(LPDIRECT3D iface, REFIID refiid)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_EnumDevices(LPDIRECT3D iface, LPD3DENUMDEVICESCALLBACK Callback, LPVOID Context)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_CreateLight(LPDIRECT3D iface, LPDIRECT3DLIGHT* Light,IUnknown* pUnkOuter)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_CreateMaterial(LPDIRECT3D iface,LPDIRECT3DMATERIAL* Direct3DLight,IUnknown* pUnkOuter)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_CreateViewport(LPDIRECT3D iface, LPDIRECT3DVIEWPORT* Viewport,IUnknown* pUnkOuter)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_FindDevice(LPDIRECT3D iface, LPD3DFINDDEVICESEARCH D3DDFS, LPD3DFINDDEVICERESULT D3DFDR)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_CreateDevice2(LPDIRECT3D2 iface, REFCLSID rclsid, LPDIRECTDRAWSURFACE lpDDS,
                                     LPDIRECT3DDEVICE2 *lplpD3DDevice2)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_CreateDevice3(LPDIRECT3D3 iface, REFCLSID rclsid,LPDIRECTDRAWSURFACE4 lpDDS,
                      LPDIRECT3DDEVICE3 *lplpD3DDevice3,LPUNKNOWN lpUnk)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_CreateVertexBuffer3(LPDIRECT3D3 iface, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
                            LPDIRECT3DVERTEXBUFFER *lplpD3DVertBuf,DWORD dwFlags,LPUNKNOWN lpUnk)
{
    DX_WINDBG_trace();
    DX_STUB;
}


HRESULT WINAPI
Main_D3D_CreateDevice7(LPDIRECT3D7 iface, REFCLSID rclsid,LPDIRECTDRAWSURFACE7 lpDDS,
                      LPDIRECT3DDEVICE7 *lplpD3DDevice)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_CreateVertexBuffer7(LPDIRECT3D7 iface, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
                            LPDIRECT3DVERTEXBUFFER7 *lplpD3DVertBuf,DWORD dwFlags)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_EnumZBufferFormats(LPDIRECT3D7 iface, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,LPVOID lpContext)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI
Main_D3D_EvictManagedTextures(LPDIRECT3D7 iface)
{
    DX_WINDBG_trace();
    DX_STUB;
}


