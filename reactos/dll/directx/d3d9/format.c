/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/format.c
 * PURPOSE:         d3d9.dll D3DFORMAT helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include "format.h"
#include <ddrawi.h>
#include <debug.h>

BOOL IsBackBufferFormat(D3DFORMAT Format)
{
    return ((Format >= D3DFMT_A8R8G8B8) && (Format < D3DFMT_A1R5G5B5)) ||
            (IsExtendedFormat(Format));
}

BOOL IsExtendedFormat(D3DFORMAT Format)
{
    return (Format == D3DFMT_A2R10G10B10);
}

BOOL IsZBufferFormat(D3DFORMAT Format)
{
    UNIMPLEMENTED

    return TRUE;
}

BOOL IsMultiElementFormat(D3DFORMAT Format)
{
    return (Format == D3DFMT_MULTI2_ARGB8);
}

BOOL IsFourCCFormat(D3DFORMAT Format)
{
    CHAR* cFormat = (CHAR*)&Format;
    if (isalnum(cFormat[0]) &&
        isalnum(cFormat[1]) &&
        isalnum(cFormat[2]) &&
        isalnum(cFormat[3]))
    {
        return TRUE;
    }

    return FALSE;
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

HRESULT CheckDeviceType(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
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

static D3DFORMAT GetStencilFormat(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT CheckFormat)
{
    switch (CheckFormat)
    {
    case D3DFMT_D15S1:
    case D3DFMT_D24S8:
    case D3DFMT_D24X8:
    case D3DFMT_D24X4S4:
        if (IsSupportedFormatOp(pDriverCaps, CheckFormat - 1, 0))
            return CheckFormat - 1;
        break;

    case D3DFMT_D16:
        if (IsSupportedFormatOp(pDriverCaps, CheckFormat, 0))
            return CheckFormat;
        else
            return D3DFMT_D16_LOCKABLE;

    default:
        /* StencilFormat same as CheckFormat */
        break;
    }

    return CheckFormat;
}

static D3DFORMAT RemoveAlphaChannel(D3DFORMAT CheckFormat)
{
    switch (CheckFormat)
    {
    case D3DFMT_A8R8G8B8:
        return D3DFMT_X8R8G8B8;

    case D3DFMT_A1R5G5B5:
        return D3DFMT_X1R5G5B5;

    case D3DFMT_A4R4G4B4:
        return D3DFMT_X4R4G4B4;

    case D3DFMT_A8B8G8R8:
        return D3DFMT_X8B8G8R8;

    default:
        /* CheckFormat has not relevant alpha channel */
        break;
    }

    return CheckFormat;
}

HRESULT CheckDeviceFormat(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
    const DWORD NumFormatOps = pDriverCaps->NumSupportedFormatOps;
    DWORD NonCompatibleOperations = 0, MustSupportOperations = 0;
    BOOL bSupportedWithAutogen = FALSE;
    DWORD FormatOpIndex;

    if (FALSE == IsSupportedFormatOp(pDriverCaps, AdapterFormat, D3DFORMAT_OP_DISPLAYMODE | D3DFORMAT_OP_3DACCELERATION))
    {
        return D3DERR_NOTAVAILABLE;
    }

    /* Check for driver auto generated mip map support if requested */
    if ((Usage & (D3DUSAGE_AUTOGENMIPMAP)) != 0)
    {
        switch (RType)
        {
        case D3DRTYPE_TEXTURE:
            if ((pDriverCaps->DriverCaps9.TextureCaps & D3DPTEXTURECAPS_MIPMAP) == 0)
                return D3DERR_NOTAVAILABLE;

            break;

        case D3DRTYPE_VOLUME:
        case D3DRTYPE_VOLUMETEXTURE:
            if ((pDriverCaps->DriverCaps9.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP) == 0)
                return D3DERR_NOTAVAILABLE;

            break;

        case D3DRTYPE_CUBETEXTURE:
            if ((pDriverCaps->DriverCaps9.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP) == 0)
                return D3DERR_NOTAVAILABLE;

            break;

        default:
            /* Do nothing */
            break;
        }

        MustSupportOperations |= D3DFORMAT_OP_AUTOGENMIPMAP;
    }

    /* Translate from RType and Usage parameters to FormatOps */
    switch (RType)
    {
    case D3DRTYPE_TEXTURE:
        MustSupportOperations |= D3DFORMAT_OP_TEXTURE;
        break;

    case D3DRTYPE_VOLUME:
    case D3DRTYPE_VOLUMETEXTURE:
        MustSupportOperations |= D3DFORMAT_OP_VOLUMETEXTURE;
        break;

    case D3DRTYPE_CUBETEXTURE:
        MustSupportOperations |= D3DFORMAT_OP_CUBETEXTURE;
        break;

    default:
        /* Do nothing */
        break;
    }

    if (Usage == 0 && RType == D3DRTYPE_SURFACE)
    {
        MustSupportOperations |= D3DFORMAT_OP_OFFSCREENPLAIN;
    }

    if ((Usage & D3DUSAGE_DEPTHSTENCIL) != 0)
    {
        MustSupportOperations |= D3DFORMAT_OP_ZSTENCIL;
    }

    if ((Usage & D3DUSAGE_DMAP) != 0)
    {
        MustSupportOperations |= D3DFORMAT_OP_DMAP;
    }

    if ((Usage & D3DUSAGE_QUERY_LEGACYBUMPMAP) != 0)
    {
        MustSupportOperations |= D3DFORMAT_OP_BUMPMAP;
    }

    if ((Usage & D3DUSAGE_QUERY_SRGBREAD) != 0)
    {
        MustSupportOperations |= D3DFORMAT_OP_SRGBREAD;
    }

    if ((Usage & D3DUSAGE_QUERY_SRGBWRITE) != 0)
    {
        MustSupportOperations |= D3DFORMAT_OP_SRGBWRITE;
    }

    if ((Usage & D3DUSAGE_QUERY_VERTEXTEXTURE) != 0)
    {
        MustSupportOperations |= D3DFORMAT_OP_VERTEXTEXTURE;
    }

    CheckFormat = GetStencilFormat(pDriverCaps, CheckFormat);

    if ((Usage & D3DUSAGE_RENDERTARGET) != 0)
    {
        if (AdapterFormat == CheckFormat)
        {
            MustSupportOperations |= D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
        }
        else
        {
            D3DFORMAT NonAlphaAdapterFormat;
            D3DFORMAT NonAlphaCheckFormat;

            NonAlphaAdapterFormat = RemoveAlphaChannel(AdapterFormat);
            NonAlphaCheckFormat = RemoveAlphaChannel(CheckFormat);

            if (NonAlphaAdapterFormat == NonAlphaCheckFormat &&
                NonAlphaCheckFormat != D3DFMT_UNKNOWN)
            {
                MustSupportOperations |= D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
            }
            else
            {
                MustSupportOperations |= D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
            }
        }
    }

    if ((Usage & D3DUSAGE_QUERY_FILTER) != 0)
    {
        NonCompatibleOperations |= D3DFORMAT_OP_OFFSCREENPLAIN;
    }

    if ((Usage & D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) != 0)
    {
        NonCompatibleOperations |= D3DFORMAT_OP_NOALPHABLEND;
    }

    if ((Usage & D3DUSAGE_QUERY_WRAPANDMIP) != 0)
    {
        NonCompatibleOperations |= D3DFORMAT_OP_NOTEXCOORDWRAPNORMIP;
    }

    for (FormatOpIndex = 0; FormatOpIndex < NumFormatOps; FormatOpIndex++)
    {
        DWORD dwOperations;
        LPDDSURFACEDESC pSurfaceDesc = &pDriverCaps->pSupportedFormatOps[FormatOpIndex];

        if (pSurfaceDesc->ddpfPixelFormat.dwFourCC != CheckFormat)
            continue;

        dwOperations = pSurfaceDesc->ddpfPixelFormat.dwOperations;

        if ((dwOperations & NonCompatibleOperations) != 0)
            continue;

        if ((dwOperations & MustSupportOperations) == MustSupportOperations)
            return D3D_OK;
        
        if (((dwOperations & MustSupportOperations) | D3DFORMAT_OP_AUTOGENMIPMAP) == MustSupportOperations)
            bSupportedWithAutogen = TRUE;
    }

    if (TRUE == bSupportedWithAutogen)
        return D3DOK_NOAUTOGEN;

    return D3DERR_NOTAVAILABLE;
}

HRESULT CheckDeviceFormatConversion(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat)
{
    D3DFORMAT NonAlphaSourceFormat;
    D3DFORMAT NonAlphaTargetFormat;

    NonAlphaSourceFormat = RemoveAlphaChannel(SourceFormat);
    NonAlphaTargetFormat = RemoveAlphaChannel(TargetFormat);

    if (NonAlphaSourceFormat == NonAlphaTargetFormat)
    {
        return D3D_OK;
    }

    if (FALSE == IsFourCCFormat(SourceFormat))
    {
        switch (SourceFormat)
        {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A2R10G10B10:
            /* Do nothing, valid SourceFormat */
            break;

        default:
            return D3DERR_NOTAVAILABLE;
        }
    }
    else if (pDriverCaps->DriverCaps9.DevCaps2 == 0)
    {
        return D3D_OK;
    }

    if (FALSE == IsSupportedFormatOp(pDriverCaps, SourceFormat, D3DFORMAT_OP_CONVERT_TO_ARGB) ||
        FALSE == IsSupportedFormatOp(pDriverCaps, TargetFormat, D3DFORMAT_MEMBEROFGROUP_ARGB))
    {
        return D3DERR_NOTAVAILABLE;
    }

    return D3D_OK;
}
