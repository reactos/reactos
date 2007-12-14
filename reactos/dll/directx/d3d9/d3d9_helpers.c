/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_helpers.c
 * PURPOSE:         d3d9.dll helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include "d3d9_helpers.h"


static LPCSTR D3dDebugRegPath = "Software\\Microsoft\\Direct3D";

BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize)
{
    HKEY hKey;
    DWORD Type;
    LONG Ret;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, D3dDebugRegPath, 0, KEY_QUERY_VALUE, &hKey))
        return FALSE;

    Ret = RegQueryValueEx(hKey, ValueName, 0, &Type, DataBuffer, DataBufferSize);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != Ret)
        return FALSE;

    if (Type != ValueType)
        return FALSE;

    return TRUE;
}
