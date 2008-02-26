/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/format.c
 * PURPOSE:         d3d9.dll D3DFORMAT helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include "format.h"
#include <ddrawi.h>

BOOL IsBackBufferFormat(D3DFORMAT Format)
{
    return ((Format >= D3DFMT_A8R8G8B8) && (Format < D3DFMT_A1R5G5B5)) ||
            (IsExtendedFormat(Format));
}

BOOL IsExtendedFormat(D3DFORMAT Format)
{
    return (Format == D3DFMT_A2R10G10B10);
}

BOOL IsSupportedFormatOp(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT DisplayFormat, DWORD FormatOp)
{
    const DWORD NumFormatOps = pDriverCaps->NumSupportedFormatOps;
    DWORD FormatOpIndex;

    for (FormatOpIndex = 0; FormatOpIndex < NumFormatOps; FormatOpIndex++)
    {
        const LPDDSURFACEDESC pSurfaceDesc = &pDriverCaps->pSupportedFormatOps[FormatOpIndex];
        if (pSurfaceDesc->ddpfPixelFormat.dwFourCC == DisplayFormat &&
            (pSurfaceDesc->ddpfPixelFormat.dwOperations & FormatOp) == FormatOp)
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT CheckDeviceFormat(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
{
    if (FALSE == IsSupportedFormatOp(pDriverCaps, DisplayFormat, D3DFORMAT_OP_DISPLAYMODE | D3DFORMAT_OP_3DACCELERATION))
    {
        return D3DERR_NOTAVAILABLE;
    }

    if (DisplayFormat != BackBufferFormat)
    {
        D3DFORMAT AdjustedDisplayFormat = DisplayFormat;

        if (DisplayFormat == D3DFMT_X8R8G8B8)
        {
            DisplayFormat = D3DFMT_A8R8G8B8;
        }
        else if (DisplayFormat == D3DFMT_X1R5G5B5)
        {
            DisplayFormat = D3DFMT_A1R5G5B5;
        }

        if (AdjustedDisplayFormat == BackBufferFormat)
        {
            if (FALSE == IsSupportedFormatOp(pDriverCaps, AdjustedDisplayFormat, D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET))
            {
                return D3DERR_NOTAVAILABLE;
            }

            return D3D_OK;
        }
        else if (FALSE == Windowed)
        {
            return D3DERR_NOTAVAILABLE;
        }

        if (FALSE == IsSupportedFormatOp(pDriverCaps, BackBufferFormat, D3DFORMAT_OP_OFFSCREEN_RENDERTARGET) ||
            FALSE == IsSupportedFormatOp(pDriverCaps, BackBufferFormat, D3DFORMAT_OP_CONVERT_TO_ARGB) ||
            FALSE == IsSupportedFormatOp(pDriverCaps, BackBufferFormat, D3DFORMAT_MEMBEROFGROUP_ARGB))
        {
            return D3DERR_NOTAVAILABLE;
        }
    }
    else
    {
        if (FALSE == IsSupportedFormatOp(pDriverCaps, DisplayFormat, D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET))
        {
            return D3DERR_NOTAVAILABLE;
        }
    }

    return D3D_OK;
}
