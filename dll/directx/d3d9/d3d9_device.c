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
#include <debug.h>
#include "d3d9_create.h"
#include "d3d9_mipmap.h"

#define LOCK_D3DDEVICE9()     if (This->bLockDevice) EnterCriticalSection(&This->CriticalSection);
#define UNLOCK_D3DDEVICE9()   if (This->bLockDevice) LeaveCriticalSection(&This->CriticalSection);

/* Convert a IDirect3DDevice9 pointer safely to the internal implementation struct */
LPDIRECT3DDEVICE9_INT IDirect3DDevice9ToImpl(LPDIRECT3DDEVICE9 iface)
{
    if (NULL == iface)
        return NULL;

    return (LPDIRECT3DDEVICE9_INT)((ULONG_PTR)iface - FIELD_OFFSET(DIRECT3DDEVICE9_INT, lpVtbl));
}

static HRESULT InvalidCall(LPDIRECT3DDEVICE9_INT This, LPSTR ErrorMsg)
{
    DPRINT1("%s",ErrorMsg);
    UNLOCK_D3DDEVICE9();
    return D3DERR_INVALIDCALL;
}

/* IDirect3DDevice9: IUnknown implementation */
HRESULT WINAPI IDirect3DDevice9Base_QueryInterface(LPDIRECT3DDEVICE9 iface, REFIID riid, void** ppvObject)
{
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirect3DDevice9))
    {
        IUnknown_AddRef(iface);
        *ppvObject = &This->lpVtbl;
        return D3D_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DDevice9Base_AddRef(LPDIRECT3DDEVICE9 iface)
{
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    ULONG ref = InterlockedIncrement(&This->lRefCnt);

    return ref;
}

ULONG WINAPI IDirect3DDevice9Base_Release(LPDIRECT3DDEVICE9 iface)
{
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    ULONG ref = InterlockedDecrement(&This->lRefCnt);

    if (ref == 0)
    {
        DWORD iAdapter;

        EnterCriticalSection(&This->CriticalSection);

        /* TODO: Free resources here */
        for (iAdapter = 0; iAdapter < This->NumAdaptersInDevice; iAdapter++)
        {
            DestroyD3D9DeviceData(&This->DeviceData[iAdapter]);
        }
        This->lpVtbl->VirtualDestructor(iface);

        LeaveCriticalSection(&This->CriticalSection);
        AlignedFree(This);
    }

    return ref;
}

/* IDirect3DDevice9 public interface */
HRESULT WINAPI IDirect3DDevice9Base_TestCooperativeLevel(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetAvailableTextureMem
* @implemented
*
* The function IDirect3DDevice9Base_GetAvailableTextureMem returns a pointer to the IDirect3D9 object
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
UINT WINAPI IDirect3DDevice9Base_GetAvailableTextureMem(LPDIRECT3DDEVICE9 iface)
{
    UINT AvailableTextureMemory = 0;
    D3D9_GETAVAILDRIVERMEMORYDATA d3d9GetAvailDriverMemoryData;

    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
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

HRESULT WINAPI IDirect3DDevice9Base_EvictManagedResources(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetDirect3D
* @implemented
*
* The function IDirect3DDevice9Base_GetDirect3D returns a pointer to the IDirect3D9 object
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
HRESULT WINAPI IDirect3DDevice9Base_GetDirect3D(LPDIRECT3DDEVICE9 iface, IDirect3D9** ppD3D9)
{
    IDirect3D9* pDirect3D9;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == ppD3D9)
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
* The function IDirect3DDevice9Base_GetDeviceCaps fills the pCaps argument with the
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
HRESULT WINAPI IDirect3DDevice9Base_GetDeviceCaps(LPDIRECT3DDEVICE9 iface, D3DCAPS9* pCaps)
{
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == pCaps)
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
* The function IDirect3DDevice9Base_GetDisplayMode fills the pMode argument with the
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
HRESULT WINAPI IDirect3DDevice9Base_GetDisplayMode(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DDISPLAYMODE* pMode)
{
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (iSwapChain >= IDirect3DDevice9_GetNumberOfSwapChains(iface))
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (NULL == pMode)
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
* The function IDirect3DDevice9Base_GetCreationParameters fills the pParameters argument with the
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
HRESULT WINAPI IDirect3DDevice9Base_GetCreationParameters(LPDIRECT3DDEVICE9 iface, D3DDEVICE_CREATION_PARAMETERS* pParameters)
{
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == pParameters)
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

HRESULT WINAPI IDirect3DDevice9Base_SetCursorProperties(LPDIRECT3DDEVICE9 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9Base_SetCursorPosition(LPDIRECT3DDEVICE9 iface, int X, int Y, DWORD Flags)
{
    UNIMPLEMENTED
}

BOOL WINAPI IDirect3DDevice9Base_ShowCursor(LPDIRECT3DDEVICE9 iface, BOOL bShow)
{
    UNIMPLEMENTED

    return TRUE;
}

/*++
* @name IDirect3DDevice9::CreateAdditionalSwapChain
* @implemented
*
* The function IDirect3DDevice9Base_CreateAdditionalSwapChain creates a swap chain object,
* useful when rendering multiple views.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice()
*
* @param D3DPRESENT_PARAMETERS* pPresentationParameters
* Pointer to a D3DPRESENT_PARAMETERS structure describing the parameters for the swap chain
* to be created.
*
* @param IDirect3DSwapChain9** ppSwapChain
* Pointer to a IDirect3DSwapChain9* to receive the swap chain object pointer.
*
* @return HRESULT
* If the method successfully fills the ppSwapChain structure, the return value is D3D_OK.
* If iSwapChain is out of range or ppSwapChain is a bad pointer, the return value
* will be D3DERR_INVALIDCALL. Also D3DERR_OUTOFVIDEOMEMORY can be returned if allocation
* of the new swap chain object failed.
*
*/
HRESULT WINAPI IDirect3DDevice9Base_CreateAdditionalSwapChain(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** ppSwapChain)
{
    UINT iSwapChain;
    IDirect3DSwapChain9* pSwapChain;
    Direct3DSwapChain9_INT* pSwapChain_INT;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == ppSwapChain)
    {
        DPRINT1("Invalid ppSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    *ppSwapChain = NULL;
    iSwapChain = IDirect3DDevice9_GetNumberOfSwapChains(iface) + 1;

    pSwapChain_INT = CreateDirect3DSwapChain9(RT_EXTERNAL, This, iSwapChain);
    if (NULL == pSwapChain_INT)
    {
        DPRINT1("Out of memory");
        UNLOCK_D3DDEVICE9();
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    Direct3DSwapChain9_Init(pSwapChain_INT, pPresentationParameters);

    This->pSwapChains[iSwapChain] = pSwapChain_INT;
    pSwapChain = (IDirect3DSwapChain9*)&pSwapChain_INT->lpVtbl;
    IDirect3DSwapChain9_AddRef(pSwapChain);
    *ppSwapChain = pSwapChain;

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetSwapChain
* @implemented
*
* The function IDirect3DDevice9Base_GetSwapChain returns a pointer to a swap chain object.
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
HRESULT WINAPI IDirect3DDevice9Base_GetSwapChain(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSwapChain9** ppSwapChain)
{
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == ppSwapChain)
    {
        DPRINT1("Invalid ppSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    *ppSwapChain = NULL;

    if (iSwapChain >= IDirect3DDevice9_GetNumberOfSwapChains(iface))
    {
        DPRINT1("Invalid iSwapChain parameter specified");
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
* The function IDirect3DDevice9Base_GetNumberOfSwapChains returns the number of swap chains
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
UINT WINAPI IDirect3DDevice9Base_GetNumberOfSwapChains(LPDIRECT3DDEVICE9 iface)
{
    UINT NumSwapChains;

    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    NumSwapChains = This->NumAdaptersInDevice;

    UNLOCK_D3DDEVICE9();
    return NumSwapChains;
}

HRESULT WINAPI IDirect3DDevice9Base_Reset(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::Present
* @implemented
*
* The function IDirect3DDevice9Base_Present displays the content of the next
* back buffer in sequence for the device.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice().
*
* @param CONST RECT* pSourceRect
* A pointer to a RECT structure representing an area of the back buffer to display where
* NULL means the whole back buffer. This parameter MUST be NULL unless the back buffer
* was created with the D3DSWAPEFFECT_COPY flag.
*
* @param CONST RECT* pDestRect
* A pointer to a RECT structure representing an area of the back buffer where the content
* will be displayed where NULL means the whole back buffer starting at (0,0).
* This parameter MUST be NULL unless the back buffer was created with the D3DSWAPEFFECT_COPY flag.
*
* @param HWND hDestWindowOverride
* A destination window where NULL means the window specified in the hWndDeviceWindow of the
* D3DPRESENT_PARAMETERS structure.
*
* @param CONST RGNDATA* pDirtyRegion
* A pointer to a RGNDATA structure representing an area of the back buffer to display where
* NULL means the whole back buffer. This parameter MUST be NULL unless the back buffer
* was created with the D3DSWAPEFFECT_COPY flag. This is an optimization region only.
*
* @return HRESULT
* If the method successfully displays the back buffer content, the return value is D3D_OK.
* If no swap chains are available, the return value will be D3DERR_INVALIDCALL.
*/
HRESULT WINAPI IDirect3DDevice9Base_Present(LPDIRECT3DDEVICE9 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    UINT i;
    UINT iNumSwapChains;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    iNumSwapChains = IDirect3DDevice9Base_GetNumberOfSwapChains(iface);
    if (0 == iNumSwapChains)
    {
        DPRINT1("Not enough swap chains, Present() fails");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    for (i = 0; i < iNumSwapChains; i++)
    {
        HRESULT hResult;
        IDirect3DSwapChain9* pSwapChain;

        IDirect3DDevice9Base_GetSwapChain(iface, i, &pSwapChain);
        hResult = IDirect3DSwapChain9_Present(pSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);

        if (FAILED(hResult))
        {
            UNLOCK_D3DDEVICE9();
            return hResult;
        }
    }

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetBackBuffer
* @implemented
*
* The function IDirect3DDevice9Base_GetBackBuffer retrieves the back buffer
* for the specified swap chain.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice().
*
* @param UINT iSwapChain
* Swap chain index to get object for.
* The maximum value for this is the value returned by IDirect3DDevice9::GetNumberOfSwapChains() - 1.
*
* @param UINT iBackBuffer
* Back buffer index to get object for.
* The maximum value for this is the the total number of back buffers - 1, as indexing starts at 0.
*
* @param IDirect3DSurface9** ppBackBuffer
* Pointer to a IDirect3DSurface9* to receive the back buffer object
*
* @return HRESULT
* If the method successfully sets the ppBackBuffer pointer, the return value is D3D_OK.
* If iSwapChain or iBackBuffer is out of range, Type is invalid or ppBackBuffer is a bad pointer,
* the return value will be D3DERR_INVALIDCALL.
*/
HRESULT WINAPI IDirect3DDevice9Base_GetBackBuffer(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer)
{
    HRESULT hResult;
    IDirect3DSwapChain9* pSwapChain = NULL;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    IDirect3DDevice9Base_GetSwapChain(iface, iSwapChain, &pSwapChain);
    if (NULL == pSwapChain)
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (NULL == ppBackBuffer)
    {
        DPRINT1("Invalid ppBackBuffer parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    hResult = IDirect3DSwapChain9_GetBackBuffer(pSwapChain, iBackBuffer, Type, ppBackBuffer);

    UNLOCK_D3DDEVICE9();
    return hResult;
}

/*++
* @name IDirect3DDevice9::GetRasterStatus
* @implemented
*
* The function IDirect3DDevice9Base_GetRasterStatus retrieves raster information
* of the monitor for the specified swap chain.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice().
*
* @param UINT iSwapChain
* Swap chain index to get object for.
* The maximum value for this is the value returned by IDirect3DDevice9::GetNumberOfSwapChains() - 1.
*
* @param D3DRASTER_STATUS* pRasterStatus
* Pointer to a D3DRASTER_STATUS to receive the raster information
*
* @return HRESULT
* If the method successfully fills the pRasterStatus structure, the return value is D3D_OK.
* If iSwapChain is out of range or pRasterStatus is a bad pointer, the return value
* will be D3DERR_INVALIDCALL.
*/
HRESULT WINAPI IDirect3DDevice9Base_GetRasterStatus(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)
{
    HRESULT hResult;
    IDirect3DSwapChain9* pSwapChain = NULL;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    IDirect3DDevice9Base_GetSwapChain(iface, iSwapChain, &pSwapChain);
    if (NULL == pSwapChain)
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (NULL == pRasterStatus)
    {
        DPRINT1("Invalid pRasterStatus parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    hResult = IDirect3DSwapChain9_GetRasterStatus(pSwapChain, pRasterStatus);

    UNLOCK_D3DDEVICE9();
    return hResult;
}

HRESULT WINAPI IDirect3DDevice9Base_SetDialogBoxMode(LPDIRECT3DDEVICE9 iface, BOOL bEnableDialogs)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::SetGammaRamp
* @implemented
*
* The function IDirect3DDevice9Base_SetGammaRamp sets the gamma correction ramp values
* for the specified swap chain.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice().
*
* @param UINT iSwapChain
* Swap chain index to get object for.
* The maximum value for this is the value returned by IDirect3DDevice9::GetNumberOfSwapChains() - 1.
*
* @param UINT Flags
* Can be on of the following:
* D3DSGR_CALIBRATE - Detects if a gamma calibrator is installed and if so modifies the values to correspond to
*                    the monitor and system settings before sending them to the display device.
* D3DSGR_NO_CALIBRATION - The gamma calibrations values are sent directly to the display device without
*                         any modification.
*
* @param CONST D3DGAMMARAMP* pRamp
* Pointer to a D3DGAMMARAMP representing the gamma correction ramp values to be set.
*
*/
VOID WINAPI IDirect3DDevice9Base_SetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
    IDirect3DSwapChain9* pSwapChain = NULL;
    Direct3DSwapChain9_INT* pSwapChain_INT;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    IDirect3DDevice9Base_GetSwapChain(iface, iSwapChain, &pSwapChain);
    if (NULL == pSwapChain)
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return;
    }

    if (NULL == pRamp)
    {
        DPRINT1("Invalid pRamp parameter specified");
        UNLOCK_D3DDEVICE9();
        return;
    }

    pSwapChain_INT = IDirect3DSwapChain9ToImpl(pSwapChain);
    Direct3DSwapChain9_SetGammaRamp(pSwapChain_INT, Flags, pRamp);

    UNLOCK_D3DDEVICE9();
}

/*++
* @name IDirect3DDevice9::GetGammaRamp
* @implemented
*
* The function IDirect3DDevice9Base_GetGammaRamp retrieves the gamma correction ramp values
* for the specified swap chain.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice().
*
* @param UINT iSwapChain
* Swap chain index to get object for.
* The maximum value for this is the value returned by IDirect3DDevice9::GetNumberOfSwapChains() - 1.
*
* @param D3DGAMMARAMP* pRamp
* Pointer to a D3DGAMMARAMP to receive the gamma correction ramp values.
*
*/
VOID WINAPI IDirect3DDevice9Base_GetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DGAMMARAMP* pRamp)
{
    IDirect3DSwapChain9* pSwapChain = NULL;
    Direct3DSwapChain9_INT* pSwapChain_INT;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    IDirect3DDevice9Base_GetSwapChain(iface, iSwapChain, &pSwapChain);
    if (NULL == pSwapChain)
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return;
    }

    if (NULL == pRamp)
    {
        DPRINT1("Invalid pRamp parameter specified");
        UNLOCK_D3DDEVICE9();
        return;
    }

    pSwapChain_INT = IDirect3DSwapChain9ToImpl(pSwapChain);
    Direct3DSwapChain9_GetGammaRamp(pSwapChain_INT, pRamp);

    UNLOCK_D3DDEVICE9();
}

/*++
* @name IDirect3DDevice9::CreateTexture
* @implemented
*
* The function IDirect3DDevice9Base_CreateTexture creates a D3D9 texture.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice()
*
* @param UINT Width
* Desired width of the texture
*
* @param UINT Height
* Desired height of the texture
*
* @param UINT Levels
* Number of mip-maps. If Levels are zero, mip-maps down to size 1x1 will be generated.
*
* @param DWORD Usage
* Valid combinations of the D3DUSAGE constants.
*
* @param D3DFORMAT Format
* One of the D3DFORMAT enum members for the surface format.
*
* @param D3DPOOL Pool
* One of the D3DPOOL enum members for where the texture should be placed.
*
* @param IDirect3DTexture9** ppTexture
* Return parameter for the created texture
*
* @param HANDLE* pSharedHandle
* Set to NULL, shared resources are not implemented yet
*
* @return HRESULT
* Returns D3D_OK if everything went well.
*
*/
HRESULT WINAPI IDirect3DDevice9Base_CreateTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
    HRESULT hResult;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == This)
        return InvalidCall(This, "Invalid 'this' parameter specified");

    if (NULL == ppTexture)
        return InvalidCall(This, "Invalid ppTexture parameter specified");

    *ppTexture = NULL;

    if (D3DFMT_UNKNOWN == Format)
        return InvalidCall(This, "Invalid Format parameter specified, D3DFMT_UNKNOWN is not a valid Format");

    if (NULL != pSharedHandle)
    {
        UNIMPLEMENTED;
        return InvalidCall(This, "Invalid pSharedHandle parameter specified, only NULL is supported at the moment");
    }

    hResult = CreateD3D9MipMap(This, Width, Height, Levels, Usage, Format, Pool, ppTexture);
    if (FAILED(hResult))
        DPRINT1("Failed to create texture");

    UNLOCK_D3DDEVICE9();
    return hResult;
}

HRESULT WINAPI IDirect3DDevice9Base_CreateVolumeTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_CreateCubeTexture(LPDIRECT3DDEVICE9 iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_CreateVertexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_CreateIndexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_CreateRenderTarget(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_CreateDepthStencilSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_UpdateSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_UpdateTexture(LPDIRECT3DDEVICE9 iface, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_GetRenderTargetData(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3DDevice9::GetFrontBufferData
* @implemented
*
* The function IDirect3DDevice9Base_GetFrontBufferData copies the content of
* the display device's front buffer in a system memory surface buffer.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3DDevice9 object returned from IDirect3D9::CreateDevice().
*
* @param UINT iSwapChain
* Swap chain index to get object for.
* The maximum value for this is the value returned by IDirect3DDevice9::GetNumberOfSwapChains() - 1.
*
* @param IDirect3DSurface9* pDestSurface
* Pointer to a IDirect3DSurface9 to receive front buffer content
*
* @return HRESULT
* If the method successfully fills the pDestSurface buffer, the return value is D3D_OK.
* If iSwapChain is out of range or pDestSurface is a bad pointer, the return value
* will be D3DERR_INVALIDCALL.
*/
HRESULT WINAPI IDirect3DDevice9Base_GetFrontBufferData(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
    HRESULT hResult;
    IDirect3DSwapChain9* pSwapChain = NULL;
    LPDIRECT3DDEVICE9_INT This = IDirect3DDevice9ToImpl(iface);
    LOCK_D3DDEVICE9();

    IDirect3DDevice9Base_GetSwapChain(iface, iSwapChain, &pSwapChain);
    if (NULL == pSwapChain)
    {
        DPRINT1("Invalid iSwapChain parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (NULL == pDestSurface)
    {
        DPRINT1("Invalid pDestSurface parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    hResult = IDirect3DSwapChain9_GetFrontBufferData(pSwapChain, pDestSurface);

    UNLOCK_D3DDEVICE9();
    return hResult;
}

HRESULT WINAPI IDirect3DDevice9Base_StretchRect(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_ColorFill(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Base_CreateOffscreenPlainSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/* IDirect3DDevice9 private interface */
VOID WINAPI IDirect3DDevice9Base_Destroy(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

VOID WINAPI IDirect3DDevice9Base_VirtualDestructor(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}
