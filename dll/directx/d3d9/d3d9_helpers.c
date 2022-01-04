/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_helpers.c
 * PURPOSE:         d3d9.dll helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include <d3d9.h>
#include "d3d9_helpers.h"
#include <stdio.h>
#include <ddraw.h>
#include <debug.h>

#define MEM_ALIGNMENT 0x20

static LPCSTR D3D9_DebugRegPath = "Software\\Microsoft\\Direct3D";

BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize)
{
    HKEY hKey;
    DWORD Type;
    LONG Ret;

    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, D3D9_DebugRegPath, 0, KEY_QUERY_VALUE, &hKey))
        return FALSE;

    Ret = RegQueryValueExA(hKey, ValueName, 0, &Type, DataBuffer, DataBufferSize);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != Ret)
        return FALSE;

    if (Type != ValueType)
        return FALSE;

    return TRUE;
}

HRESULT SafeFormatString(OUT LPSTR Buffer, IN DWORD BufferSize, IN LPCSTR FormatString, ... )
{
    DWORD BytesWritten;
    va_list vargs;

    if (BufferSize == 0)
        return DDERR_INVALIDPARAMS;

    va_start(vargs, FormatString);
    BytesWritten = _vsnprintf(Buffer, BufferSize-1, FormatString, vargs);
    va_end(vargs);

    if (BytesWritten < BufferSize)
        return DDERR_GENERIC;

    Buffer[BufferSize-1] = '\0';

    return ERROR_SUCCESS;
}

HRESULT SafeCopyString(OUT LPSTR Dst, IN DWORD DstSize, IN LPCSTR Src)
{
    HRESULT hr = ERROR_SUCCESS;

    if (Dst == NULL || DstSize == 0 || Src == NULL)
        return DDERR_INVALIDPARAMS;

    while (*Src != '\0' && DstSize > 0)
    {
        *Dst++ = *Src++;
        --DstSize;
    }

    if (DstSize == 0)
    {
        --Dst;
        hr = DDERR_GENERIC;
    }

    return hr;
}

HRESULT SafeAppendString(IN OUT LPSTR Dst, IN DWORD DstSize, IN LPCSTR Src)
{
	size_t CurrentDstLength;

    if (Dst == NULL || DstSize == 0)
        return DDERR_INVALIDPARAMS;

    CurrentDstLength = strlen(Dst);

    return SafeCopyString(Dst + CurrentDstLength, DstSize - CurrentDstLength, Src);
}

HRESULT AlignedAlloc(IN OUT LPVOID *ppObject, IN SIZE_T dwSize)
{
    ULONG_PTR AddressOffset;
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

    AlignedPtr = (CHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

    if (AlignedPtr == 0)
        return DDERR_OUTOFMEMORY;

    AddressOffset = MEM_ALIGNMENT - ((ULONG_PTR)AlignedPtr & AlignedMask);

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

    HeapFree(GetProcessHeap(), 0, NonAlignedPtr);
}
