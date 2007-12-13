#include "rosdraw.h"

#include <string.h>

/* PSEH for SEH Support */
#include <pseh/pseh.h>

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

