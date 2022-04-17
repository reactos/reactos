/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_callbacks.c
 * PURPOSE:         Direct3D9's callback functions
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_common.h"
#include "d3d9_callbacks.h"
#include <d3d9.h>
#include <dll/directx/d3d8thk.h>
#include "d3d9_private.h"
#include "ddrawi.h"

DWORD WINAPI D3d9GetAvailDriverMemory(LPD3D9_GETAVAILDRIVERMEMORYDATA pData)
{
    DWORD Ret = FALSE;
    DDHAL_GETAVAILDRIVERMEMORYDATA Data;
    ZeroMemory(&Data, sizeof(Data));

    if (D3D9_GETAVAILDRIVERMEMORY_TYPE_ALL == pData->dwMemoryType)
    {
        Data.DDSCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    }
    else if (D3D9_GETAVAILDRIVERMEMORY_TYPE_LOCAL == pData->dwMemoryType)
    {
        Data.DDSCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
    }
    else if (D3D9_GETAVAILDRIVERMEMORY_TYPE_NONLOCAL == pData->dwMemoryType)
    {
        Data.DDSCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM;
    }

    if (Data.DDSCaps.dwCaps != 0)
    {
        if (0 == pData->dwTextureType)
            Data.DDSCaps.dwCaps |= DDSCAPS_TEXTURE;

        if (pData->dwTextureType & D3D9_TEXTURETYPE_HALSURFACE)
            Data.DDSCaps.dwCaps |= DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;

        if (pData->dwTextureType & D3D9_TEXTURETYPE_BACKBUFFER)
            Data.DDSCaps.dwCaps |= DDSCAPS_ZBUFFER;

        Ret = OsThunkDdGetAvailDriverMemory(pData->pUnknown6BC->hDirectDrawLocal, (DD_GETAVAILDRIVERMEMORYDATA*)&Data);
        pData->dwFree = Data.dwFree;
    }

    return Ret;
}
