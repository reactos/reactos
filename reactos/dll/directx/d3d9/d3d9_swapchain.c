/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_swapchain.c
 * PURPOSE:         Direct3D9's swap chain object
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_swapchain.h"

#include <debug.h>
#include <ddraw.h>

#include "d3d9_helpers.h"
#include "d3d9_device.h"

/* Convert a IDirect3DSwapChain9 pointer safely to the internal implementation struct */
static LPDIRECT3DSWAPCHAIN9_INT IDirect3DSwapChain9ToImpl(LPDIRECT3DSWAPCHAIN9 iface)
{
    if (IsBadWritePtr(iface, sizeof(LPDIRECT3DSWAPCHAIN9_INT)))
        return NULL;

    return (LPDIRECT3DSWAPCHAIN9_INT)((ULONG_PTR)iface - FIELD_OFFSET(Direct3DSwapChain9_INT, lpVtbl));
}

/* IDirect3DSwapChain9: IUnknown implementation */
static HRESULT WINAPI Direct3DSwapChain9_QueryInterface(LPDIRECT3DSWAPCHAIN9 iface, REFIID riid, void** ppvObject)
{
    LPDIRECT3DSWAPCHAIN9_INT This = IDirect3DSwapChain9ToImpl(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirect3DSwapChain9))
    {
        IUnknown_AddRef(iface);
        *ppvObject = &This->lpVtbl;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Direct3DSwapChain9_AddRef(LPDIRECT3DSWAPCHAIN9 iface)
{
    LPDIRECT3DSWAPCHAIN9_INT This = IDirect3DSwapChain9ToImpl(iface);
    return D3D9BaseObject_AddRef((D3D9BaseObject*) &This->BaseObject.lpVtbl);
}

static ULONG WINAPI Direct3DSwapChain9_Release(LPDIRECT3DSWAPCHAIN9 iface)
{
    LPDIRECT3DSWAPCHAIN9_INT This = IDirect3DSwapChain9ToImpl(iface);
    return D3D9BaseObject_Release((D3D9BaseObject*) &This->BaseObject.lpVtbl);
}

/* IDirect3DSwapChain9 interface */
static HRESULT WINAPI Direct3DSwapChain9_Present(LPDIRECT3DSWAPCHAIN9 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags)
{
    UNIMPLEMENTED
    return D3D_OK;
}

static HRESULT WINAPI Direct3DSwapChain9_GetFrontBufferData(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DSurface9* pDestSurface)
{
    UNIMPLEMENTED
    return D3D_OK;
}

static HRESULT WINAPI Direct3DSwapChain9_GetBackBuffer(LPDIRECT3DSWAPCHAIN9 iface, UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer)
{
    UNIMPLEMENTED
    return D3D_OK;
}

static HRESULT WINAPI Direct3DSwapChain9_GetRasterStatus(LPDIRECT3DSWAPCHAIN9 iface, D3DRASTER_STATUS* pRasterStatus)
{
    UNIMPLEMENTED
    return D3D_OK;
}

static HRESULT WINAPI Direct3DSwapChain9_GetDisplayMode(LPDIRECT3DSWAPCHAIN9 iface, D3DDISPLAYMODE* pMode)
{
    UNIMPLEMENTED
    return D3D_OK;
}

static HRESULT WINAPI Direct3DSwapChain9_GetDevice(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DDevice9** ppDevice)
{
    UNIMPLEMENTED
    return D3D_OK;
}

static HRESULT WINAPI Direct3DSwapChain9_GetPresentParameters(LPDIRECT3DSWAPCHAIN9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    UNIMPLEMENTED
    return D3D_OK;
}

static IDirect3DSwapChain9Vtbl Direct3DSwapChain9_Vtbl =
{
    /* IUnknown */
    Direct3DSwapChain9_QueryInterface,
    Direct3DSwapChain9_AddRef,
    Direct3DSwapChain9_Release,

    /* IDirect3DSwapChain9 */
    Direct3DSwapChain9_Present,
    Direct3DSwapChain9_GetFrontBufferData,
    Direct3DSwapChain9_GetBackBuffer,
    Direct3DSwapChain9_GetRasterStatus,
    Direct3DSwapChain9_GetDisplayMode,
    Direct3DSwapChain9_GetDevice,
    Direct3DSwapChain9_GetPresentParameters,
};

Direct3DSwapChain9_INT* CreateDirect3DSwapChain9(enum REF_TYPE RefType, struct _Direct3DDevice9_INT* pBaseDevice, DWORD ChainIndex)
{
    Direct3DSwapChain9_INT* pThisSwapChain;
    if (FAILED(AlignedAlloc((LPVOID *)&pThisSwapChain, sizeof(Direct3DSwapChain9_INT))))
    {
        DPRINT1("Could not create Direct3DSwapChain9_INT");
        return NULL;
    }

    InitD3D9BaseObject((D3D9BaseObject*) &pThisSwapChain->BaseObject.lpVtbl, RefType, (IUnknown*) &pBaseDevice->lpVtbl);

    pThisSwapChain->lpVtbl = &Direct3DSwapChain9_Vtbl;

    pThisSwapChain->ChainIndex = ChainIndex;
    pThisSwapChain->AdapterGroupIndex = ChainIndex;
    pThisSwapChain->pUnknown6BC = pBaseDevice->DeviceData[ChainIndex].pUnknown6BC;
    pThisSwapChain->pUnknown015c = (LPVOID)0xD3D9D3D9;

    return pThisSwapChain;
}

VOID Direct3DSwapChain9_SetDisplayMode(Direct3DSwapChain9_INT* pThisSwapChain, D3DDISPLAYMODE* pMode)
{
    pThisSwapChain->dwWidth = pMode->Width;
    pThisSwapChain->dwHeight = pMode->Height;
}

HRESULT Direct3DSwapChain9_Init(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    int i;

    for (i = 0; i < 256; i++)
    {
        pThisSwapChain->GammaRamp.red[i] = 
            pThisSwapChain->GammaRamp.green[i] =
            pThisSwapChain->GammaRamp.blue[i] = i;
    }

    pThisSwapChain->PresentParameters = pPresentationParameters[pThisSwapChain->ChainIndex];
    pThisSwapChain->SwapEffect = pPresentationParameters->SwapEffect;

    return Direct3DSwapChain9_Reset(pThisSwapChain, pPresentationParameters);
}

HRESULT Direct3DSwapChain9_Reset(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    // TODO: Do all the dirty work...
    return D3D_OK;
}
