/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_helpers.c
 * PURPOSE:         d3d9.dll helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include "d3d9_helpers.h"
#include <stdio.h>
#include <ddraw.h>
#include <debug.h>

#define MEM_ALIGNMENT 0x20

static LPCSTR D3D9_DebugRegPath = "Software\\Microsoft\\Direct3D";

static const GUID DISPLAY_GUID = { 0x67685559, 0x3106, 0x11D0, { 0xB9, 0x71, 0x00, 0xAA, 0x00, 0x34, 0x2F, 0x9F } };

static CHAR D3D9_PrimaryDeviceName[32];

LPDIRECT3D9_INT impl_from_IDirect3D9(LPDIRECT3D9 iface)
{
    return (LPDIRECT3D9_INT)((ULONG_PTR)iface - FIELD_OFFSET(DIRECT3D9_INT, lpVtbl));
}

BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize)
{
    HKEY hKey;
    DWORD Type;
    LONG Ret;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, D3D9_DebugRegPath, 0, KEY_QUERY_VALUE, &hKey))
        return FALSE;

    Ret = RegQueryValueEx(hKey, ValueName, 0, &Type, DataBuffer, DataBufferSize);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != Ret)
        return FALSE;

    if (Type != ValueType)
        return FALSE;

    return TRUE;
}

HRESULT FormatDebugString(IN OUT LPSTR Buffer, IN LONG BufferSize, IN LPCSTR FormatString, ... )
{
    int BytesWritten;
    va_list vargs;

    if (BufferSize == 0)
        return DDERR_INVALIDPARAMS;

    va_start(vargs, FormatString);
    BytesWritten = _vsnprintf(Buffer, BufferSize-1, FormatString, vargs);

    if (BytesWritten < BufferSize)
        return DDERR_GENERIC;

    Buffer[BufferSize-1] = '\0';

    return 0;
}

static BOOL GetDisplayDeviceInfo(IN OUT LPDIRECT3D9_INT pDirect3D9)
{
    DISPLAY_DEVICEA DisplayDevice;
    DWORD AdapterIndex;
    DWORD Planes;
    DWORD Bpp;
    HDC hDC;

    memset(&DisplayDevice, 0, sizeof(DISPLAY_DEVICEA));
    DisplayDevice.cb = sizeof(DISPLAY_DEVICEA);   

    pDirect3D9->dwNumDisplayAdapters = 0;
    D3D9_PrimaryDeviceName[0] = '\0';

    AdapterIndex = 0;
    while (EnumDisplayDevicesA(NULL, AdapterIndex, &DisplayDevice, 0) == TRUE)
    {
        if ((DisplayDevice.StateFlags & (DISPLAY_DEVICE_DISCONNECT | DISPLAY_DEVICE_MIRRORING_DRIVER)) == 0 &&
            (DisplayDevice.StateFlags & (DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) != 0)
        {
            memcpy(&pDirect3D9->DisplayAdapters[0].DisplayGuid, &DISPLAY_GUID, sizeof(GUID));

            lstrcpynA(pDirect3D9->DisplayAdapters[0].szDeviceName, DisplayDevice.DeviceName, MAX_PATH);

            if (pDirect3D9->dwNumDisplayAdapters == 0)
                lstrcpynA(D3D9_PrimaryDeviceName, DisplayDevice.DeviceName, sizeof(D3D9_PrimaryDeviceName));

            pDirect3D9->DisplayAdapters[0].dwStateFlags = DisplayDevice.StateFlags;
            pDirect3D9->DisplayAdapters[0].bInUseFlag = TRUE;

            ++pDirect3D9->dwNumDisplayAdapters;
        }
    }

    AdapterIndex = 0;
    while (EnumDisplayDevicesA(NULL, AdapterIndex, &DisplayDevice, 0) == TRUE)
    {
        if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0 &&
            (DisplayDevice.StateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | DISPLAY_DEVICE_PRIMARY_DEVICE)) == 0)
        {
            memcpy(&pDirect3D9->DisplayAdapters[pDirect3D9->dwNumDisplayAdapters].DisplayGuid, &DISPLAY_GUID, sizeof(GUID));

            lstrcpynA(pDirect3D9->DisplayAdapters[pDirect3D9->dwNumDisplayAdapters].szDeviceName, DisplayDevice.DeviceName, MAX_PATH);

            pDirect3D9->DisplayAdapters[pDirect3D9->dwNumDisplayAdapters].dwStateFlags = DisplayDevice.StateFlags;
            pDirect3D9->DisplayAdapters[pDirect3D9->dwNumDisplayAdapters].bInUseFlag = TRUE;

            ++pDirect3D9->dwNumDisplayAdapters;
        }
    }

    hDC = GetDC(NULL);
    Planes = GetDeviceCaps(hDC, PLANES);
    Bpp = GetDeviceCaps(hDC, BITSPIXEL);
    ReleaseDC(NULL, hDC);

    if (Planes * Bpp < 8)
        return FALSE;

    return TRUE;
}

HRESULT CreateD3D9(OUT LPDIRECT3D9 *ppDirect3D9)
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
    //pDirect3D9->dwProcessId = GetCurrentThreadId();
    pDirect3D9->dwRefCnt = 1;

    pDirect3D9->unknown004576 = 0;
    pDirect3D9->unknown004578 = 0;
    pDirect3D9->unknown004579 = 0;
    pDirect3D9->unknown004580 = 0;
    pDirect3D9->unknown004581 = 0;
    pDirect3D9->unknown004582 = 0;
    pDirect3D9->unknown004583 = 0;
    pDirect3D9->unknown004589 = 0x20;

    pDirect3D9->lpInt = pDirect3D9;
    pDirect3D9->unknown000007 = 1;

    //InitializeCriticalSection(&pDirect3D9->d3d9_cs);

    //memset(pDirect3D9->DisplayAdapters, 0, sizeof(pDirect3D9->DisplayAdapters));
    GetDisplayDeviceInfo(pDirect3D9);

    *ppDirect3D9 = (LPDIRECT3D9)&pDirect3D9->lpVtbl;

    return ERROR_SUCCESS;
}

HRESULT AlignedAlloc(IN OUT LPVOID *ppObject, IN SIZE_T dwSize)
{
    ULONG AddressOffset;
    ULONG AlignedMask = MEM_ALIGNMENT - 1;
    CHAR *AlignedPtr;
    ULONG_PTR *AlignedOffsetPtr;

    if (ppObject == 0)
        return DDERR_INVALIDPARAMS;

    if (dwSize == 0)
    {
        *ppObject = NULL;
        return S_OK;
    }

    dwSize += MEM_ALIGNMENT;

    AlignedPtr = (CHAR *)LocalAlloc(LMEM_ZEROINIT, dwSize);

    if (AlignedPtr == 0)
        return DDERR_OUTOFMEMORY;

    AddressOffset = MEM_ALIGNMENT - ((ULONG)AlignedPtr & AlignedMask);
	
    AlignedPtr += AddressOffset;

    AlignedOffsetPtr = (ULONG_PTR *)(AlignedPtr - sizeof(ULONG));
    *AlignedOffsetPtr = AddressOffset;

    *ppObject = (ULONG_PTR *)AlignedPtr;

    return S_OK;
}

VOID AlignedFree(IN OUT LPVOID pObject)
{
    CHAR *NonAlignedPtr = pObject;
    ULONG_PTR *AlignedPtr = pObject;

    if (pObject == 0)
        return;

    NonAlignedPtr -= *(AlignedPtr - 1);

    LocalFree(NonAlignedPtr);
}
