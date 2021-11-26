/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_cursor.c
 * PURPOSE:         d3d9.dll internal cursor methods
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_cursor.h"

#include <debug.h>
#include <d3d9.h>
#include "d3d9_private.h"
#include "adapter.h"
#include "d3d9_device.h"
#include "d3d9_swapchain.h"
#include "d3d9_helpers.h"

D3D9Cursor* CreateD3D9Cursor(struct _Direct3DDevice9_INT* pBaseDevice, struct _Direct3DSwapChain9_INT* pSwapChain)
{
    D3D9Cursor* pCursor;

    if (FAILED(AlignedAlloc((LPVOID*)&pCursor, sizeof(D3D9Cursor))))
    {
        DPRINT1("Failed to allocate D3D9Cursor");
        return NULL;
    }

    pCursor->pBaseDevice = pBaseDevice;
    pCursor->pSwapChain = pSwapChain;
    pCursor->dwWidth = pSwapChain->dwWidth / 2;
    pCursor->dwHeight = pSwapChain->dwHeight / 2;

    return pCursor;
}
