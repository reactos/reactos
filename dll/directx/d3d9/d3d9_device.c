/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_device.c
 * PURPOSE:         d3d9.dll internal device methods
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */
#include "d3d9_device.h"
#include "d3d9_helpers.h"
#include "adapter.h"
#include "debug.h"

#define LOCK_D3DDEVICE9()     if (This->bLockDevice) EnterCriticalSection(&This->CriticalSection);
#define UNLOCK_D3DDEVICE9()   if (This->bLockDevice) LeaveCriticalSection(&This->CriticalSection);

/* Convert a IDirect3D9 pointer safely to the internal implementation struct */
LPDIRECT3DDEVICE9_INT impl_from_IDirect3DDevice9(LPDIRECT3DDEVICE9 iface)
{
    if (IsBadWritePtr(iface, sizeof(LPDIRECT3DDEVICE9_INT)))
        return NULL;

    return (LPDIRECT3DDEVICE9_INT)((ULONG_PTR)iface - FIELD_OFFSET(DIRECT3DDEVICE9_INT, lpVtbl));
}

/* IDirect3DDevice9: IUnknown implementation */
static HRESULT WINAPI IDirect3DDevice9Impl_QueryInterface(LPDIRECT3DDEVICE9 iface, REFIID riid, void** ppvObject)
{
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirect3DDevice9))
    {
        IUnknown_AddRef(iface);
        *ppvObject = &This->lpVtbl;
        return D3D_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DDevice9Impl_AddRef(LPDIRECT3DDEVICE9 iface)
{
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    ULONG ref = InterlockedIncrement(&This->lRefCnt);

    return ref;
}

static ULONG WINAPI IDirect3DDevice9Impl_Release(LPDIRECT3DDEVICE9 iface)
{
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    ULONG ref = InterlockedDecrement(&This->lRefCnt);

    if (ref == 0)
    {
        EnterCriticalSection(&This->CriticalSection);
        /* TODO: Free resources here */
        LeaveCriticalSection(&This->CriticalSection);
        AlignedFree(This);
    }

    return ref;
}

/* IDirect3D9Device interface */
static HRESULT WINAPI IDirect3DDevice9Impl_TestCooperativeLevel(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetAvailableTextureMem
* @implemented
*
* The function IDirect3DDevice9Impl_GetAvailableTextureMem returns a pointer to the IDirect3D9 object
* that created this device.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice()
*
* @return UINT
* The method returns an estimated the currently available texture memory in bytes rounded
* to the nearest MB. Applications should NOT use this as an exact number.
*
*/
static UINT WINAPI IDirect3DDevice9Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE9 iface)
{
    UINT AvailableTextureMemory = 0;
    D3D9_GETAVAILDRIVERMEMORYDATA d3d9GetAvailDriverMemoryData;

    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    LOCK_D3DDEVICE9();

    memset(&d3d9GetAvailDriverMemoryData, 0, sizeof(d3d9GetAvailDriverMemoryData));
    d3d9GetAvailDriverMemoryData.pUnknown6BC = This->DeviceData[0].pUnknown6BC;
    d3d9GetAvailDriverMemoryData.dwMemoryType = D3D9_GETAVAILDRIVERMEMORY_TYPE_ALL;

    if (TRUE == (*This->DeviceData[0].D3D9Callbacks.DdGetAvailDriverMemory)(&d3d9GetAvailDriverMemoryData))
    {
        /* Round it up to the nearest MB */
        AvailableTextureMemory = (d3d9GetAvailDriverMemoryData.dwFree + 0x80000) & 0xFFF00000;
    }

    UNLOCK_D3DDEVICE9();
    return AvailableTextureMemory;
}

static HRESULT WINAPI IDirect3DDevice9Impl_EvictManagedResources(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetDirect3D
* @implemented
*
* The function IDirect3DDevice9Impl_GetDirect3D returns a pointer to the IDirect3D9 object
* that created this device.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice()
*
* @param IDirect3D9** ppD3D9
* Pointer to a IDirect3D9* to receive the IDirect3D9 object pointer.
*
* @return HRESULT
* If the method successfully fills the ppD3D9 structure, the return value is D3D_OK.
* If ppD3D9 is a bad pointer, the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3DDevice9Impl_GetDirect3D(LPDIRECT3DDEVICE9 iface, IDirect3D9** ppD3D9)
{
    IDirect3D9* pDirect3D9;
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    LOCK_D3DDEVICE9();

    if (IsBadWritePtr(ppD3D9, sizeof(IDirect3D9*)))
    {
        DPRINT1("Invalid ppD3D9 parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    pDirect3D9 = (IDirect3D9*)&This->pDirect3D9->lpVtbl;
    IDirect3D9_AddRef(pDirect3D9);
    *ppD3D9 = pDirect3D9;

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetDeviceCaps
* @implemented
*
* The function IDirect3DDevice9Impl_GetDeviceCaps fills the pCaps argument with the
* capabilities of the device.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D9 object returned from Direct3DCreate9()
*
* @param D3DCAPS9* pCaps
* Pointer to a D3DCAPS9 structure to be filled with the device's capabilities.
*
* @return HRESULT
* If the method successfully fills the pCaps structure, the return value is D3D_OK.
* If pCaps is a bad pointer the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3DDevice9Impl_GetDeviceCaps(LPDIRECT3DDEVICE9 iface, D3DCAPS9* pCaps)
{
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    LOCK_D3DDEVICE9();

    if (IsBadWritePtr(pCaps, sizeof(D3DCAPS9)))
    {
        DPRINT1("Invalid pCaps parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    GetAdapterCaps(&This->pDirect3D9->DisplayAdapters[0], This->DeviceData[0].DeviceType, pCaps);

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetDisplayMode
* @implemented
*
* The function IDirect3DDevice9Impl_GetDisplayMode fills the pMode argument with the
* display mode for the specified swap chain.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D9 object returned from Direct3DCreate9()
*
* @param UINT iSwapChain
* Swap chain index to get object for.
* The maximum value for this is the value returned by IDirect3DDevice9::GetNumberOfSwapChains() - 1.
*
* @param D3DDISPLAYMODE* pMode
* Pointer to a D3DDISPLAYMODE structure to be filled with the current swap chain's display mode information.
*
* @return HRESULT
* If the method successfully fills the pMode structure, the return value is D3D_OK.
* If iSwapChain is out of range or pMode is a bad pointer, the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3DDevice9Impl_GetDisplayMode(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DDISPLAYMODE* pMode)
{
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    LOCK_D3DDEVICE9();

    if (iSwapChain >= IDirect3DDevice9_GetNumberOfSwapChains(iface))
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (IsBadWritePtr(pMode, sizeof(D3DDISPLAYMODE)))
    {
        DPRINT1("Invalid pMode parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    pMode->Width = This->DeviceData[iSwapChain].DriverCaps.dwDisplayWidth;
    pMode->Height = This->DeviceData[iSwapChain].DriverCaps.dwDisplayHeight;
    pMode->Format = This->DeviceData[iSwapChain].DriverCaps.RawDisplayFormat;
    pMode->RefreshRate = This->DeviceData[iSwapChain].DriverCaps.dwRefreshRate;

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetCreationParameters
* @implemented
*
* The function IDirect3DDevice9Impl_GetCreationParameters fills the pParameters argument with the
* parameters the device was created with.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D9 object returned from Direct3DCreate9()
*
* @param D3DDEVICE_CREATION_PARAMETERS* pParameters
* Pointer to a D3DDEVICE_CREATION_PARAMETERS structure to be filled with the creation parameter
* information for this device.
*
* @return HRESULT
* If the method successfully fills the pParameters structure, the return value is D3D_OK.
* If pParameters is a bad pointer, the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3DDevice9Impl_GetCreationParameters(LPDIRECT3DDEVICE9 iface, D3DDEVICE_CREATION_PARAMETERS* pParameters)
{
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    LOCK_D3DDEVICE9();

    if (IsBadWritePtr(pParameters, sizeof(D3DDEVICE_CREATION_PARAMETERS)))
    {
        DPRINT1("Invalid pParameters parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    pParameters->AdapterOrdinal = This->AdapterIndexInGroup[0];
    pParameters->DeviceType = This->DeviceType;
    pParameters->hFocusWindow = This->hWnd;
    pParameters->BehaviorFlags = This->BehaviourFlags;

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetCursorProperties(LPDIRECT3DDEVICE9 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static void WINAPI IDirect3DDevice9Impl_SetCursorPosition(LPDIRECT3DDEVICE9 iface, int X, int Y, DWORD Flags)
{
    UNIMPLEMENTED
}

static BOOL WINAPI IDirect3DDevice9Impl_ShowCursor(LPDIRECT3DDEVICE9 iface, BOOL bShow)
{
    UNIMPLEMENTED

    return TRUE;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** ppSwapChain)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetSwapChain
* @implemented
*
* The function IDirect3DDevice9Impl_GetSwapChain returns a pointer to a swap chain object.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice()
*
* @param UINT iSwapChain
* Swap chain index to get object for.
* The maximum value for this is the value returned by IDirect3DDevice9::GetNumberOfSwapChains() - 1.
*
* @param IDirect3DSwapChain9** ppSwapChain
* Pointer to a IDirect3DSwapChain9* to receive the swap chain object pointer.
*
* @return HRESULT
* If the method successfully fills the ppSwapChain structure, the return value is D3D_OK.
* If iSwapChain is out of range or ppSwapChain is a bad pointer, the return value
* will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3DDevice9Impl_GetSwapChain(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSwapChain9** ppSwapChain)
{
    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    LOCK_D3DDEVICE9();

    if (iSwapChain >= IDirect3DDevice9_GetNumberOfSwapChains(iface))
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (IsBadWritePtr(ppSwapChain, sizeof(IDirect3DSwapChain9*)))
    {
        DPRINT1("Invalid ppSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (This->pSwapChains[iSwapChain] != NULL)
    {
        IDirect3DSwapChain9* pSwapChain = (IDirect3DSwapChain9*)&This->pSwapChains[iSwapChain]->lpVtbl;
        IDirect3DSwapChain9_AddRef(pSwapChain);
        *ppSwapChain = pSwapChain;
    }
    else
    {
        *ppSwapChain = NULL;
    }

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetNumberOfSwapChains
* @implemented
*
* The function IDirect3DDevice9Impl_GetNumberOfSwapChains returns the number of swap chains
* created by IDirect3D9::CreateDevice().
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice().
*
* @return UINT
* Returns the number of swap chains created by IDirect3D9::CreateDevice().
*
* NOTE: An application can create additional swap chains using the
*       IDirect3DDevice9::CreateAdditionalSwapChain() method.
*
*/
static UINT WINAPI IDirect3DDevice9Impl_GetNumberOfSwapChains(LPDIRECT3DDEVICE9 iface)
{
    UINT NumSwapChains;

    LPDIRECT3DDEVICE9_INT This = impl_from_IDirect3DDevice9(iface);
    LOCK_D3DDEVICE9();

    NumSwapChains = This->NumAdaptersInDevice;

    UNLOCK_D3DDEVICE9();
    return NumSwapChains;
}

static HRESULT WINAPI IDirect3DDevice9Impl_Reset(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_Present(LPDIRECT3DDEVICE9 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetBackBuffer(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetRasterStatus(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetDialogBoxMode(LPDIRECT3DDEVICE9 iface, BOOL bEnableDialogs)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static void WINAPI IDirect3DDevice9Impl_SetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
    UNIMPLEMENTED
}

static void WINAPI IDirect3DDevice9Impl_GetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DGAMMARAMP* pRamp)
{
    UNIMPLEMENTED
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVolumeTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateCubeTexture(LPDIRECT3DDEVICE9 iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateIndexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateRenderTarget(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_UpdateSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_UpdateTexture(LPDIRECT3DDEVICE9 iface, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetRenderTargetData(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetFrontBufferData(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_StretchRect(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_ColorFill(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateOffscreenPlainSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pNewZStencil)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9** ppZStencilSurface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_BeginScene(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_EndScene(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_Clear(LPDIRECT3DDEVICE9 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_MultiplyTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetViewport(LPDIRECT3DDEVICE9 iface, CONST D3DVIEWPORT9* pViewport)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetViewport(LPDIRECT3DDEVICE9 iface, D3DVIEWPORT9* pViewport)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetMaterial(LPDIRECT3DDEVICE9 iface, CONST D3DMATERIAL9* pMaterial)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetMaterial(LPDIRECT3DDEVICE9 iface, D3DMATERIAL9* pMaterial)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST D3DLIGHT9* pLight)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, D3DLIGHT9* pLight)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_LightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL Enable)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetLightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL* pEnable)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST float* pPlane)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, float* pPlane)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateStateBlock(LPDIRECT3DDEVICE9 iface, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_BeginStateBlock(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_EndStateBlock(LPDIRECT3DDEVICE9 iface, IDirect3DStateBlock9** ppSB)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetClipStatus(LPDIRECT3DDEVICE9 iface, CONST D3DCLIPSTATUS9* pClipStatus)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetClipStatus(LPDIRECT3DDEVICE9 iface, D3DCLIPSTATUS9* pClipStatus)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_ValidateDevice(LPDIRECT3DDEVICE9 iface, DWORD* pNumPasses)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, PALETTEENTRY* pEntries)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT* pPaletteNumber)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetScissorRect(LPDIRECT3DDEVICE9 iface, CONST RECT* pRect)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetScissorRect(LPDIRECT3DDEVICE9 iface, RECT* pRect)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface, BOOL bSoftware)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static BOOL WINAPI IDirect3DDevice9Impl_GetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return TRUE;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetNPatchMode(LPDIRECT3DDEVICE9 iface, float nSegments)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static float WINAPI IDirect3DDevice9Impl_GetNPatchMode(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return 0.0f;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_ProcessVertices(LPDIRECT3DDEVICE9 iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexDeclaration(LPDIRECT3DDEVICE9 iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9* pDecl)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9** ppDecl)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetFVF(LPDIRECT3DDEVICE9 iface, DWORD FVF)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetFVF(LPDIRECT3DDEVICE9 iface, DWORD* pFVF)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,UINT Setting)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, UINT* pSetting)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9* pIndexData)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9** ppIndexData)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreatePixelShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawRectPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawTriPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DeletePatch(LPDIRECT3DDEVICE9 iface, UINT Handle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateQuery(LPDIRECT3DDEVICE9 iface, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
{
    UNIMPLEMENTED

    return D3D_OK;
}

IDirect3DDevice9Vtbl Direct3DDevice9_Vtbl =
{
    /* IUnknown */
    IDirect3DDevice9Impl_QueryInterface,
    IDirect3DDevice9Impl_AddRef,
    IDirect3DDevice9Impl_Release,

    /* IDirect3D9Device */
    IDirect3DDevice9Impl_TestCooperativeLevel,
    IDirect3DDevice9Impl_GetAvailableTextureMem,
    IDirect3DDevice9Impl_EvictManagedResources,
    IDirect3DDevice9Impl_GetDirect3D,
    IDirect3DDevice9Impl_GetDeviceCaps,
    IDirect3DDevice9Impl_GetDisplayMode,
    IDirect3DDevice9Impl_GetCreationParameters,
    IDirect3DDevice9Impl_SetCursorProperties,
    IDirect3DDevice9Impl_SetCursorPosition,
    IDirect3DDevice9Impl_ShowCursor,
    IDirect3DDevice9Impl_CreateAdditionalSwapChain,
    IDirect3DDevice9Impl_GetSwapChain,
    IDirect3DDevice9Impl_GetNumberOfSwapChains,
    IDirect3DDevice9Impl_Reset,
    IDirect3DDevice9Impl_Present,
    IDirect3DDevice9Impl_GetBackBuffer,
    IDirect3DDevice9Impl_GetRasterStatus,
    IDirect3DDevice9Impl_SetDialogBoxMode,
    IDirect3DDevice9Impl_SetGammaRamp,
    IDirect3DDevice9Impl_GetGammaRamp,
    IDirect3DDevice9Impl_CreateTexture,
    IDirect3DDevice9Impl_CreateVolumeTexture,
    IDirect3DDevice9Impl_CreateCubeTexture,
    IDirect3DDevice9Impl_CreateVertexBuffer,
    IDirect3DDevice9Impl_CreateIndexBuffer,
    IDirect3DDevice9Impl_CreateRenderTarget,
    IDirect3DDevice9Impl_CreateDepthStencilSurface,
    IDirect3DDevice9Impl_UpdateSurface,
    IDirect3DDevice9Impl_UpdateTexture,
    IDirect3DDevice9Impl_GetRenderTargetData,
    IDirect3DDevice9Impl_GetFrontBufferData,
    IDirect3DDevice9Impl_StretchRect,
    IDirect3DDevice9Impl_ColorFill,
    IDirect3DDevice9Impl_CreateOffscreenPlainSurface,
    IDirect3DDevice9Impl_SetRenderTarget,
    IDirect3DDevice9Impl_GetRenderTarget,
    IDirect3DDevice9Impl_SetDepthStencilSurface,
    IDirect3DDevice9Impl_GetDepthStencilSurface,
    IDirect3DDevice9Impl_BeginScene,
    IDirect3DDevice9Impl_EndScene,
    IDirect3DDevice9Impl_Clear,
    IDirect3DDevice9Impl_SetTransform,
    IDirect3DDevice9Impl_GetTransform,
    IDirect3DDevice9Impl_MultiplyTransform,
    IDirect3DDevice9Impl_SetViewport,
    IDirect3DDevice9Impl_GetViewport,
    IDirect3DDevice9Impl_SetMaterial,
    IDirect3DDevice9Impl_GetMaterial,
    IDirect3DDevice9Impl_SetLight,
    IDirect3DDevice9Impl_GetLight,
    IDirect3DDevice9Impl_LightEnable,
    IDirect3DDevice9Impl_GetLightEnable,
    IDirect3DDevice9Impl_SetClipPlane,
    IDirect3DDevice9Impl_GetClipPlane,
    IDirect3DDevice9Impl_SetRenderState,
    IDirect3DDevice9Impl_GetRenderState,
    IDirect3DDevice9Impl_CreateStateBlock,
    IDirect3DDevice9Impl_BeginStateBlock,
    IDirect3DDevice9Impl_EndStateBlock,
    IDirect3DDevice9Impl_SetClipStatus,
    IDirect3DDevice9Impl_GetClipStatus,
    IDirect3DDevice9Impl_GetTexture,
    IDirect3DDevice9Impl_SetTexture,
    IDirect3DDevice9Impl_GetTextureStageState,
    IDirect3DDevice9Impl_SetTextureStageState,
    IDirect3DDevice9Impl_GetSamplerState,
    IDirect3DDevice9Impl_SetSamplerState,
    IDirect3DDevice9Impl_ValidateDevice,
    IDirect3DDevice9Impl_SetPaletteEntries,
    IDirect3DDevice9Impl_GetPaletteEntries,
    IDirect3DDevice9Impl_SetCurrentTexturePalette,
    IDirect3DDevice9Impl_GetCurrentTexturePalette,
    IDirect3DDevice9Impl_SetScissorRect,
    IDirect3DDevice9Impl_GetScissorRect,
    IDirect3DDevice9Impl_SetSoftwareVertexProcessing,
    IDirect3DDevice9Impl_GetSoftwareVertexProcessing,
    IDirect3DDevice9Impl_SetNPatchMode,
    IDirect3DDevice9Impl_GetNPatchMode,
    IDirect3DDevice9Impl_DrawPrimitive,
    IDirect3DDevice9Impl_DrawIndexedPrimitive,
    IDirect3DDevice9Impl_DrawPrimitiveUP,
    IDirect3DDevice9Impl_DrawIndexedPrimitiveUP,
    IDirect3DDevice9Impl_ProcessVertices,
    IDirect3DDevice9Impl_CreateVertexDeclaration,
    IDirect3DDevice9Impl_SetVertexDeclaration,
    IDirect3DDevice9Impl_GetVertexDeclaration,
    IDirect3DDevice9Impl_SetFVF,
    IDirect3DDevice9Impl_GetFVF,
    IDirect3DDevice9Impl_CreateVertexShader,
    IDirect3DDevice9Impl_SetVertexShader,
    IDirect3DDevice9Impl_GetVertexShader,
    IDirect3DDevice9Impl_SetVertexShaderConstantF,
    IDirect3DDevice9Impl_GetVertexShaderConstantF,
    IDirect3DDevice9Impl_SetVertexShaderConstantI,
    IDirect3DDevice9Impl_GetVertexShaderConstantI,
    IDirect3DDevice9Impl_SetVertexShaderConstantB,
    IDirect3DDevice9Impl_GetVertexShaderConstantB,
    IDirect3DDevice9Impl_SetStreamSource,
    IDirect3DDevice9Impl_GetStreamSource,
    IDirect3DDevice9Impl_SetStreamSourceFreq,
    IDirect3DDevice9Impl_GetStreamSourceFreq,
    IDirect3DDevice9Impl_SetIndices,
    IDirect3DDevice9Impl_GetIndices,
    IDirect3DDevice9Impl_CreatePixelShader,
    IDirect3DDevice9Impl_SetPixelShader,
    IDirect3DDevice9Impl_GetPixelShader,
    IDirect3DDevice9Impl_SetPixelShaderConstantF,
    IDirect3DDevice9Impl_GetPixelShaderConstantF,
    IDirect3DDevice9Impl_SetPixelShaderConstantI,
    IDirect3DDevice9Impl_GetPixelShaderConstantI,
    IDirect3DDevice9Impl_SetPixelShaderConstantB,
    IDirect3DDevice9Impl_GetPixelShaderConstantB,
    IDirect3DDevice9Impl_DrawRectPatch,
    IDirect3DDevice9Impl_DrawTriPatch,
    IDirect3DDevice9Impl_DeletePatch,
    IDirect3DDevice9Impl_CreateQuery,
};

