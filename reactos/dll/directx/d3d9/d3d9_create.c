/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_create.c
 * PURPOSE:         d3d9.dll internal create functions and data
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include <d3d9.h>
#include "d3d9_create.h"
#include "d3d9_helpers.h"
#include <debug.h>
#include <ddrawi.h>
#include <ddrawgdi.h>
#include <strsafe.h>

static const GUID DISPLAY_GUID = { 0x67685559, 0x3106, 0x11D0, { 0xB9, 0x71, 0x00, 0xAA, 0x00, 0x34, 0x2F, 0x9F } };

static CHAR D3D9_PrimaryDeviceName[CCHDEVICENAME];

static BOOL IsDirectDrawSupported()
{
    HDC hDC;
    DWORD Planes;
    DWORD Bpp;

    hDC = GetDC(NULL);
    Planes = GetDeviceCaps(hDC, PLANES);
    Bpp = GetDeviceCaps(hDC, BITSPIXEL);
    ReleaseDC(NULL, hDC);

    if (Planes * Bpp < 8)
        return FALSE;

    return TRUE;
}

static VOID SetAdapterInfo(IN OUT LPDIRECT3D9_DISPLAYADAPTER_INT pDisplayAdapter, IN LPDISPLAY_DEVICEA pDisplayDevice)
{
    memcpy(&pDisplayAdapter->DisplayGuid, &DISPLAY_GUID, sizeof(GUID));

    lstrcpynA(pDisplayAdapter->szDeviceName, pDisplayDevice->DeviceName, MAX_PATH);

    pDisplayAdapter->dwStateFlags = pDisplayDevice->StateFlags;
    pDisplayAdapter->bInUseFlag = TRUE;
}

static BOOL IsGDIDriver(HDC hDC)
{
    COLORREF NearestBlack = GetNearestColor(hDC, RGB(0, 0, 0));
    COLORREF NearestWhite = GetNearestColor(hDC, RGB(255, 255, 255));

    if (NearestBlack != RGB(0, 0, 0) || NearestWhite != RGB(255, 255, 255))
        return TRUE;

    return FALSE;
}

static BOOL GetDirect3DAdapterInfo(IN OUT LPDIRECT3D9_DISPLAYADAPTER_INT pDisplayAdapter)
{
    HDC hDC;
    LPD3D9_DEVICEDATA pDeviceData;

    /* Test DC creation for the display device */
    if (NULL == (hDC = CreateDCA(NULL, pDisplayAdapter->szDeviceName, NULL, NULL)))
        return FALSE;

    pDeviceData = LocalAlloc(LMEM_ZEROINIT, sizeof(D3D9_DEVICEDATA));
    if (NULL == pDeviceData)
    {
        DPRINT1("Out of memory, could not initialize Direct3D adapter");
        DeleteDC(hDC);
        return FALSE;
    }

    pDeviceData->hDC = hDC;
    pDeviceData->DisplayGuid = pDisplayAdapter->DisplayGuid;
    pDeviceData->DeviceType = D3DDEVTYPE_HAL;
    lstrcpynA(pDeviceData->szDeviceName, pDisplayAdapter->szDeviceName, CCHDEVICENAME);
    pDeviceData->szDeviceName[CCHDEVICENAME-1] = '\0';

    if (pDisplayAdapter->bInUseFlag)
    {
        pDeviceData->_UnknownA8h.DeviceType = D3DDEVTYPE_HAL;
    }
    else if (IsGDIDriver(hDC))
    {
        pDeviceData->_UnknownA8h.DeviceType = D3DDEVTYPE_REF;
    }

    //GetDeviceData(pDeviceData);

    DeleteDC(hDC);
    return TRUE;
}

static BOOL GetDisplayDeviceInfo(IN OUT LPDIRECT3D9_INT pDirect3D9)
{
    DISPLAY_DEVICEA DisplayDevice;
    DWORD AdapterIndex;

    memset(&DisplayDevice, 0, sizeof(DISPLAY_DEVICEA));
    DisplayDevice.cb = sizeof(DISPLAY_DEVICEA);   

    pDirect3D9->NumDisplayAdapters = 0;
    D3D9_PrimaryDeviceName[0] = '\0';

    AdapterIndex = 0;
    while (EnumDisplayDevicesA(NULL, AdapterIndex, &DisplayDevice, 0) == TRUE &&
           pDirect3D9->NumDisplayAdapters < DX_D3D9_MAX_NUM_ADAPTERS)
    {
        if ((DisplayDevice.StateFlags & (DISPLAY_DEVICE_DISCONNECT | DISPLAY_DEVICE_MIRRORING_DRIVER)) == 0 &&
            (DisplayDevice.StateFlags & (DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) != 0)
        {
            SetAdapterInfo(&pDirect3D9->DisplayAdapters[pDirect3D9->NumDisplayAdapters], &DisplayDevice);

            if (pDirect3D9->NumDisplayAdapters == 0)
                lstrcpynA(D3D9_PrimaryDeviceName, DisplayDevice.DeviceName, sizeof(D3D9_PrimaryDeviceName));

            ++pDirect3D9->NumDisplayAdapters;
            break;
        }

        ++AdapterIndex;
    }

    AdapterIndex = 0;
    while (EnumDisplayDevicesA(NULL, AdapterIndex, &DisplayDevice, 0) == TRUE &&
           pDirect3D9->NumDisplayAdapters < DX_D3D9_MAX_NUM_ADAPTERS)
    {
        if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0 &&
            (DisplayDevice.StateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | DISPLAY_DEVICE_PRIMARY_DEVICE)) == 0)
        {
            SetAdapterInfo(&pDirect3D9->DisplayAdapters[pDirect3D9->NumDisplayAdapters], &DisplayDevice);
            ++pDirect3D9->NumDisplayAdapters;
        }

        ++AdapterIndex;
    }

    /* Check if minimum DirectDraw is supported */
    if (IsDirectDrawSupported() == FALSE)
        return FALSE;

    for (AdapterIndex = 0; AdapterIndex < pDirect3D9->NumDisplayAdapters; AdapterIndex++)
    {
        GetDirect3DAdapterInfo(&pDirect3D9->DisplayAdapters[AdapterIndex]);
    }

    return TRUE;
}

HRESULT CreateD3D9(OUT LPDIRECT3D9 *ppDirect3D9, UINT SDKVersion)
{
    LPDIRECT3D9_INT pDirect3D9;

    if (ppDirect3D9 == 0)
        return DDERR_INVALIDPARAMS;

    if (AlignedAlloc((LPVOID *)&pDirect3D9, sizeof(DIRECT3D9_INT)) != S_OK)
        return DDERR_OUTOFMEMORY;

    if (pDirect3D9 == 0)
        return DDERR_OUTOFMEMORY;

    pDirect3D9->unknown000007 = 0;
    pDirect3D9->lpInt = 0;

    pDirect3D9->lpVtbl = &Direct3D9_Vtbl;
    pDirect3D9->dwProcessId = GetCurrentThreadId();
    pDirect3D9->dwRefCnt = 1;

    pDirect3D9->unknown004576 = 0;
    pDirect3D9->unknown004578 = 0;
    pDirect3D9->unknown004579 = 0;
    pDirect3D9->unknown004580 = 0;
    pDirect3D9->unknown004581 = 0;
    pDirect3D9->unknown004582 = 0;
    pDirect3D9->unknown004583 = 0;
    pDirect3D9->SDKVersion = SDKVersion;

    pDirect3D9->lpInt = pDirect3D9;
    pDirect3D9->unknown000007 = 1;

    InitializeCriticalSection(&pDirect3D9->d3d9_cs);

    memset(pDirect3D9->DisplayAdapters, 0, sizeof(pDirect3D9->DisplayAdapters));
    GetDisplayDeviceInfo(pDirect3D9);

    *ppDirect3D9 = (LPDIRECT3D9)&pDirect3D9->lpVtbl;

    return ERROR_SUCCESS;
}
