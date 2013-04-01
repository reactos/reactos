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

HRESULT WINAPI Main_D3D_QueryInterface(LPDIRECT3D2 iface, REFIID riid, LPVOID * ppvObj);
ULONG WINAPI Main_D3D_AddRef(LPDIRECT3D2 iface);
ULONG WINAPI Main_D3D_Release(LPDIRECT3D2 iface);
HRESULT WINAPI Main_D3D_EnumDevices(LPDIRECT3D2 iface, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI Main_D3D_CreateLight(LPDIRECT3D2 iface, LPDIRECT3DLIGHT *lplpDirect3DLight, IUnknown *pUnkOuter);
HRESULT WINAPI Main_D3D_CreateMaterial(LPDIRECT3D2 iface, LPDIRECT3DMATERIAL2 *lplpDirect3DMaterial2, IUnknown *pUnkOuter);
HRESULT WINAPI Main_D3D_CreateViewport(LPDIRECT3D2 iface, LPDIRECT3DVIEWPORT2 *lplpD3DViewport2, IUnknown *pUnkOuter);
HRESULT WINAPI Main_D3D_FindDevice(LPDIRECT3D2 iface, LPD3DFINDDEVICESEARCH lpD3DDFS, LPD3DFINDDEVICERESULT lpD3DFDR);
HRESULT WINAPI Main_D3D_CreateDevice2(LPDIRECT3D2 iface, REFCLSID rclsid, LPDIRECTDRAWSURFACE lpDDS, LPDIRECT3DDEVICE2 *lplpD3DDevice2);

IDirect3D2Vtbl IDirect3D2_Vtbl =
{
    Main_D3D_QueryInterface,
    Main_D3D_AddRef,
    Main_D3D_Release,
    Main_D3D_EnumDevices,
    Main_D3D_CreateLight,
    Main_D3D_CreateMaterial,
    Main_D3D_CreateViewport,
    Main_D3D_FindDevice,
    Main_D3D_CreateDevice2,
};


