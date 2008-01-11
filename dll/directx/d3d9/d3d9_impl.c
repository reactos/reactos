/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_impl.c
 * PURPOSE:         IDirect3D9 implementation
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */
#include "d3d9_helpers.h"

#include <debug.h>

/* IDirect3D9: IUnknown implementation */
static HRESULT WINAPI IDirect3D9Impl_QueryInterface(LPDIRECT3D9 iface, REFIID riid, LPVOID* ppvObject)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirect3D9))
    {
        IUnknown_AddRef(iface);
        *ppvObject = &This->lpVtbl;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3D9Impl_AddRef(LPDIRECT3D9 iface)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    ULONG ref = InterlockedIncrement(&This->dwRefCnt);

    return ref;
}

static ULONG WINAPI IDirect3D9Impl_Release(LPDIRECT3D9 iface)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    ULONG ref = InterlockedDecrement(&This->dwRefCnt);

    if (ref == 0)
    {
        EnterCriticalSection(&This->d3d9_cs);
        /* TODO: Free resources here */
        LeaveCriticalSection(&This->d3d9_cs);
        AlignedFree(This);
    }

    return ref;
}

/* IDirect3D9 interface */
static HRESULT WINAPI IDirect3D9Impl_RegisterSoftwareDevice(LPDIRECT3D9 iface, void* pInitializeFunction)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static UINT WINAPI IDirect3D9Impl_GetAdapterCount(LPDIRECT3D9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_GetAdapterIdentifier(LPDIRECT3D9 iface, UINT Adapter, DWORD Flags,
                                                          D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static UINT WINAPI IDirect3D9Impl_GetAdapterModeCount(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_EnumAdapterModes(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format,
                                                      UINT Mode, D3DDISPLAYMODE* pMode)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_GetAdapterDisplayMode(LPDIRECT3D9 iface, UINT Adapter, D3DDISPLAYMODE* pMode)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceType(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE CheckType,
                                                     D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormat(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                       D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType,
                                                       D3DFORMAT CheckFormat)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceMultiSampleType(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                                D3DFORMAT SurfaceFormat, BOOL Windowed,
                                                                D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDepthStencilMatch(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                            D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat,
                                                            D3DFORMAT DepthStencilFormat)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormatConversion(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                                 D3DFORMAT SourceFormat, D3DFORMAT TargetFormat)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_GetDeviceCaps(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HMONITOR WINAPI IDirect3D9Impl_GetAdapterMonitor(LPDIRECT3D9 iface, UINT Adapter)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CreateDevice(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                  HWND hFocusWindow, DWORD BehaviourFlags,
                                                  D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                  struct IDirect3DDevice9** ppReturnedDeviceInterface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

const IDirect3D9Vtbl Direct3D9_Vtbl =
{
    /* IUnknown */
    IDirect3D9Impl_QueryInterface,
    IDirect3D9Impl_AddRef,
    IDirect3D9Impl_Release,

    /* IDirect3D9 */
    IDirect3D9Impl_RegisterSoftwareDevice,
    IDirect3D9Impl_GetAdapterCount,
    IDirect3D9Impl_GetAdapterIdentifier,
    IDirect3D9Impl_GetAdapterModeCount,
    IDirect3D9Impl_EnumAdapterModes,
    IDirect3D9Impl_GetAdapterDisplayMode,
    IDirect3D9Impl_CheckDeviceType,
    IDirect3D9Impl_CheckDeviceFormat,
    IDirect3D9Impl_CheckDeviceMultiSampleType,
    IDirect3D9Impl_CheckDepthStencilMatch,
    IDirect3D9Impl_CheckDeviceFormatConversion,
    IDirect3D9Impl_GetDeviceCaps,
    IDirect3D9Impl_GetAdapterMonitor,
    IDirect3D9Impl_CreateDevice
};
