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
#include "d3d9_cursor.h"

#define LOCK_D3DDEVICE9()   D3D9BaseObject_LockDevice(&This->BaseObject)
#define UNLOCK_D3DDEVICE9() D3D9BaseObject_UnlockDevice(&This->BaseObject)

/* Convert a IDirect3DSwapChain9 pointer safely to the internal implementation struct */
LPDIRECT3DSWAPCHAIN9_INT IDirect3DSwapChain9ToImpl(LPDIRECT3DSWAPCHAIN9 iface)
{
    if (NULL == iface)
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

/*++
* @name IDirect3DSwapChain9::GetDevice
* @implemented
*
* The function Direct3DSwapChain9_GetDevice sets the ppDevice argument
* to the device connected to create the swap chain.
*
* @param LPDIRECT3DSWAPCHAIN9 iface
* Pointer to a IDirect3DSwapChain9 object returned from IDirect3D9Device::GetSwapChain()
*
* @param IDirect3DDevice9** ppDevice
* Pointer to a IDirect3DDevice9* structure to be set to the device object.
*
* @return HRESULT
* If the method successfully sets the ppDevice value, the return value is D3D_OK.
* If ppDevice is a bad pointer the return value will be D3DERR_INVALIDCALL.
* If the swap chain didn't contain any device, the return value will be D3DERR_INVALIDDEVICE.
*
*/
static HRESULT WINAPI Direct3DSwapChain9_GetDevice(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DDevice9** ppDevice)
{
    LPDIRECT3DSWAPCHAIN9_INT This = IDirect3DSwapChain9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == ppDevice)
    {
        DPRINT1("Invalid ppDevice parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(D3D9BaseObject_GetDevice(&This->BaseObject, ppDevice)))
    {
        DPRINT1("Invalid This parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDDEVICE;
    }

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

/*++
* @name IDirect3DSwapChain9::GetPresentParameters
* @implemented
*
* The function Direct3DSwapChain9_GetPresentParameters fills the pPresentationParameters
* argument with the D3DPRESENT_PARAMETERS parameters that was used to create the swap chain.
*
* @param LPDIRECT3DSWAPCHAIN9 iface
* Pointer to a IDirect3DSwapChain9 object returned from IDirect3D9Device::GetSwapChain()
*
* @param D3DPRESENT_PARAMETERS* pPresentationParameters
* Pointer to a D3DPRESENT_PARAMETERS structure to be filled with the creation parameters.
*
* @return HRESULT
* If the method successfully fills the pPresentationParameters structure, the return value is D3D_OK.
* If pPresentationParameters is a bad pointer the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI Direct3DSwapChain9_GetPresentParameters(LPDIRECT3DSWAPCHAIN9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    LPDIRECT3DSWAPCHAIN9_INT This = IDirect3DSwapChain9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == pPresentationParameters)
    {
        DPRINT1("Invalid pPresentationParameters parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    *pPresentationParameters = This->PresentParameters;

    UNLOCK_D3DDEVICE9();
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
    if (FAILED(AlignedAlloc((LPVOID*)&pThisSwapChain, sizeof(Direct3DSwapChain9_INT))))
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

VOID Direct3DSwapChain9_SetDisplayMode(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    pThisSwapChain->dwWidth = pPresentationParameters->BackBufferWidth;
    pThisSwapChain->dwHeight = pPresentationParameters->BackBufferHeight;
}

HRESULT Direct3DSwapChain9_Init(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    int i;
    DIRECT3DDEVICE9_INT* pDevice;

    for (i = 0; i < 256; i++)
    {
        pThisSwapChain->GammaRamp.red[i] =
            pThisSwapChain->GammaRamp.green[i] =
            pThisSwapChain->GammaRamp.blue[i] = i;
    }

    pThisSwapChain->PresentParameters = pPresentationParameters[pThisSwapChain->ChainIndex];
    pThisSwapChain->SwapEffect = pPresentationParameters->SwapEffect;
    Direct3DSwapChain9_SetDisplayMode(pThisSwapChain, &pThisSwapChain->PresentParameters);

    if (FAILED(D3D9BaseObject_GetDeviceInt(&pThisSwapChain->BaseObject, &pDevice)))
    {
        DPRINT1("Could not get the swapchain device");
        return DDERR_GENERIC;
    }

    pThisSwapChain->pCursor = CreateD3D9Cursor(pDevice, pThisSwapChain);
    if (NULL == pThisSwapChain->pCursor)
    {
        DPRINT1("Could not allocate D3D9Cursor");
        return DDERR_OUTOFMEMORY;
    }

    return Direct3DSwapChain9_Reset(pThisSwapChain, pPresentationParameters);
}

HRESULT Direct3DSwapChain9_Reset(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    // TODO: Do all the dirty work...
    return D3D_OK;
}

VOID Direct3DSwapChain9_GetGammaRamp(Direct3DSwapChain9_INT* pThisSwapChain, D3DGAMMARAMP* pRamp)
{
    memcpy(pRamp, &pThisSwapChain->GammaRamp, sizeof(D3DGAMMARAMP));
}

VOID Direct3DSwapChain9_SetGammaRamp(Direct3DSwapChain9_INT* pThisSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
    UNIMPLEMENTED
}
