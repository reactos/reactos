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

HRESULT WINAPI Main_D3D_QueryInterface(LPDIRECT3D iface, REFIID riid, LPVOID * ppvObj);
ULONG WINAPI Main_D3D_AddRef(LPDIRECT3D iface);
ULONG WINAPI Main_D3D_Release(LPDIRECT3D iface);
HRESULT WINAPI Main_D3D_Initialize(LPDIRECT3D iface, REFIID refiid);
HRESULT WINAPI Main_D3D_EnumDevices(LPDIRECT3D iface, LPD3DENUMDEVICESCALLBACK Callback, LPVOID Context);
HRESULT WINAPI Main_D3D_CreateLight(LPDIRECT3D iface, LPDIRECT3DLIGHT* Light,IUnknown* pUnkOuter);
HRESULT WINAPI Main_D3D_CreateMaterial(LPDIRECT3D iface,LPDIRECT3DMATERIAL* Direct3DLight,IUnknown* pUnkOuter);
HRESULT WINAPI Main_D3D_CreateViewport(LPDIRECT3D iface, LPDIRECT3DVIEWPORT* Viewport,IUnknown* pUnkOuter);
HRESULT WINAPI Main_D3D_FindDevice(LPDIRECT3D iface, LPD3DFINDDEVICESEARCH D3DDFS, LPD3DFINDDEVICERESULT D3DFDR);

IDirect3DVtbl IDirect3D_Vtbl =
{
    Main_D3D_QueryInterface,
    Main_D3D_AddRef,
    Main_D3D_Release,
    Main_D3D_Initialize,
    Main_D3D_EnumDevices,
    Main_D3D_CreateLight,
    Main_D3D_CreateMaterial,
    Main_D3D_CreateViewport,
    Main_D3D_FindDevice
};

