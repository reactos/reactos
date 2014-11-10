/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/device.c
 * PURPOSE:         Direct3D9's device creation
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */

#include "device.h"
#include <debug.h>
#include "d3d9_helpers.h"
#include "d3d9_create.h"

static HRESULT InitD3D9ResourceManager(D3D9ResourceManager* pThisResourceManager, LPDIRECT3DDEVICE9_INT pDirect3DDevice9)
{
    DWORD MaxSimultaneousTextures;

    MaxSimultaneousTextures = max(1, pDirect3DDevice9->DeviceData[0].DriverCaps.DriverCaps9.MaxSimultaneousTextures);

    if (FAILED(AlignedAlloc((LPVOID *)&pThisResourceManager->pTextureHeap, sizeof(DWORD) + MaxSimultaneousTextures * sizeof(int) * 3)))
    {
        DPRINT1("Could not allocate texture heap");
        return DDERR_OUTOFMEMORY;
    }

    // TODO: Init texture heap

    pThisResourceManager->MaxSimultaneousTextures = MaxSimultaneousTextures;
    pThisResourceManager->pBaseDevice = pDirect3DDevice9;

    return D3D_OK;
}

HRESULT InitD3D9BaseDevice(LPDIRECT3DDEVICE9_INT pThisBaseDevice, LPDIRECT3D9_INT pDirect3D9,
                           UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviourFlags,
                           D3DPRESENT_PARAMETERS* pPresentationParameters, DWORD NumAdaptersToCreate)
{
    D3D9ResourceManager* pResourceManager;
    DWORD i;

    // Insert Reset/Ctor here

    if (FAILED(AlignedAlloc((LPVOID *)&pResourceManager, sizeof(D3D9ResourceManager))) ||
        FAILED(InitD3D9ResourceManager(pResourceManager, pThisBaseDevice)))
    {
        DPRINT1("Could not create resource manager");
        return DDERR_OUTOFMEMORY;
    }

    pThisBaseDevice->pResourceManager = pResourceManager;

    pThisBaseDevice->lpVtbl = &Direct3DDevice9HAL_Vtbl;
    pThisBaseDevice->lRefCnt = 1;
    pThisBaseDevice->dwProcessId = GetCurrentThreadId();
    pThisBaseDevice->pUnknown = (IUnknown*) &pThisBaseDevice->lpVtbl;
    InitializeCriticalSection(&pThisBaseDevice->CriticalSection);

    pThisBaseDevice->pDirect3D9 = pDirect3D9;
    pThisBaseDevice->DeviceType = DeviceType;
    pThisBaseDevice->hWnd = hFocusWindow;
    pThisBaseDevice->AdjustedBehaviourFlags = BehaviourFlags;
    pThisBaseDevice->BehaviourFlags = BehaviourFlags;
    pThisBaseDevice->NumAdaptersInDevice = NumAdaptersToCreate;

    // TODO: Query driver for correct DX version
    pThisBaseDevice->dwDXVersion = 9;

    for (i = 0; i < NumAdaptersToCreate; i++)
    {
        if (FALSE == CreateD3D9DeviceData(&pDirect3D9->DisplayAdapters[i], &pThisBaseDevice->DeviceData[i]))
        {
            DPRINT1("Failed to get device data for adapter: %d", i);
            return DDERR_GENERIC;
        }

        pThisBaseDevice->AdapterIndexInGroup[i] = i;
        pThisBaseDevice->CurrentDisplayMode[i].Width = pDirect3D9->DisplayAdapters[i].DriverCaps.dwDisplayWidth;
        pThisBaseDevice->CurrentDisplayMode[i].Height = pDirect3D9->DisplayAdapters[i].DriverCaps.dwDisplayHeight;
        pThisBaseDevice->CurrentDisplayMode[i].RefreshRate = pDirect3D9->DisplayAdapters[i].DriverCaps.dwRefreshRate;
        pThisBaseDevice->CurrentDisplayMode[i].Format = pDirect3D9->DisplayAdapters[i].DriverCaps.RawDisplayFormat;

        pThisBaseDevice->pSwapChains[i] = CreateDirect3DSwapChain9(RT_BUILTIN, pThisBaseDevice, i);
        pThisBaseDevice->pSwapChains2[i] = pThisBaseDevice->pSwapChains[i];

        if (FAILED(Direct3DSwapChain9_Init(pThisBaseDevice->pSwapChains[i], pPresentationParameters)))
        {
            DPRINT1("Failed to init swap chain: %d", i);
            return DDERR_GENERIC;
        }
    }

    return D3D_OK;
}

HRESULT CreateD3D9HalDevice(LPDIRECT3D9_INT pDirect3D9, UINT Adapter,
                            HWND hFocusWindow, DWORD BehaviourFlags,
                            D3DPRESENT_PARAMETERS* pPresentationParameters,
                            DWORD NumAdaptersToCreate,
                            struct IDirect3DDevice9** ppReturnedDeviceInterface)
{
    HRESULT Ret;

    if (FAILED(AlignedAlloc((LPVOID *)ppReturnedDeviceInterface, sizeof(D3D9HALDEVICE))))
    {
        DPRINT1("Not enough memory to create HAL device");
        return DDERR_OUTOFMEMORY;
    }

    Ret = InitD3D9BaseDevice((LPDIRECT3DDEVICE9_INT)*ppReturnedDeviceInterface, pDirect3D9, Adapter,
                             D3DDEVTYPE_HAL, hFocusWindow, BehaviourFlags,
                             pPresentationParameters, NumAdaptersToCreate);

    if (FAILED(Ret))
    {
        AlignedFree((LPVOID)*ppReturnedDeviceInterface);
        return Ret;
    }

    return D3D_OK;
}
