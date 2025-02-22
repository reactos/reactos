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
#include <d3d9types.h>

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

BOOL IsStencilFormat(D3DFORMAT Format)
{
    switch (Format)
    {
    case D3DFMT_D15S1:
    case D3DFMT_D24S8:
    case D3DFMT_D24X4S4:
    case D3DFMT_D24FS8:
        return TRUE;

    default:
        return FALSE;
    }
}

DWORD GetBytesPerPixel(D3DFORMAT Format)
{
    switch (Format)
    {
    case D3DFMT_R3G3B2:
    case D3DFMT_A8:
        return 1;

    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
    case D3DFMT_A8R3G3B2:
    case D3DFMT_X4R4G4B4:
        return 2;

    case D3DFMT_R8G8B8:
        return 3;

    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_A2B10G10R10:
    case D3DFMT_A8B8G8R8:
    case D3DFMT_X8B8G8R8:
    case D3DFMT_G16R16:
    case D3DFMT_A2R10G10B10:
        return 4;

    case D3DFMT_A16B16G16R16:
        return 8;


    case D3DFMT_P8:
    case D3DFMT_L8:
    case D3DFMT_A4L4:
        return 1;

    case D3DFMT_A8P8:
    case D3DFMT_A8L8:
        return 2;


    case D3DFMT_V8U8:
    case D3DFMT_L6V5U5:
        return 2;

    case D3DFMT_X8L8V8U8:
    case D3DFMT_Q8W8V8U8:
    case D3DFMT_V16U16:
    case D3DFMT_A2W10V10U10:
        return 4;


    case D3DFMT_S8_LOCKABLE:
        return 1;

    case D3DFMT_D16_LOCKABLE:
    case D3DFMT_D15S1:
    case D3DFMT_D16:
        return 2;

    case D3DFMT_D32:
    case D3DFMT_D24S8:
    case D3DFMT_D24X8:
    case D3DFMT_D24X4S4:
    case D3DFMT_D32F_LOCKABLE:
    case D3DFMT_D24FS8:
    case D3DFMT_D32_LOCKABLE:
        return 4;


    case D3DFMT_L16:
        return 2;

    /* TODO: Handle D3DFMT_VERTEXDATA? */
    case D3DFMT_INDEX16:
        return 2;
    case D3DFMT_INDEX32:
        return 4;


    case D3DFMT_Q16W16V16U16:
        return 8;


    case D3DFMT_R16F:
        return 2;
    case D3DFMT_G16R16F:
        return 4;
    case D3DFMT_A16B16G16R16F:
        return 8;


    case D3DFMT_R32F:
        return 4;
    case D3DFMT_G32R32F:
        return 8;
    case D3DFMT_A32B32G32R32F:
        return 16;

    case D3DFMT_CxV8U8:
        return 2;


    /* Known FourCC formats */
    case D3DFMT_UYVY:
    case D3DFMT_R8G8_B8G8:
    case D3DFMT_YUY2:
    case D3DFMT_G8R8_G8B8:
        return 2;

    case D3DFMT_DXT1:
        return 0xFFFFFFF8;

    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        return 0xFFFFFFF0;

    case D3DFMT_MULTI2_ARGB8:
        return 8;

    default:
        return 0;
    }
}

DWORD GetPixelStride(D3DFORMAT Format)
{
    DWORD Bpp = GetBytesPerPixel(Format);

    if (0 == Bpp)
    {
        /* TODO: Handle unknown formats here */
    }

    return Bpp;
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

HRESULT CheckDepthStencilMatch(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
{
    const DWORD NumFormatOps = pDriverCaps->NumSupportedFormatOps;
    BOOL bRenderTargetAvailable = FALSE;
    BOOL bDepthStencilAvailable = FALSE;
    BOOL bForceSameDepthStencilBits = FALSE;
    DWORD FormatIndex;

    if (FALSE == IsSupportedFormatOp(pDriverCaps, AdapterFormat, D3DFORMAT_OP_DISPLAYMODE | D3DFORMAT_OP_3DACCELERATION))
    {
        return D3DERR_NOTAVAILABLE;
    }

    if (DepthStencilFormat != D3DFMT_D16_LOCKABLE &&
        DepthStencilFormat != D3DFMT_D32F_LOCKABLE)
    {
        if (TRUE == IsStencilFormat(DepthStencilFormat))
        {
            bForceSameDepthStencilBits = TRUE;
        }
    }

    if (FALSE == bForceSameDepthStencilBits &&
        (DepthStencilFormat == D3DFMT_D32 || DepthStencilFormat == D3DFMT_D24X8))
    {
        bForceSameDepthStencilBits = TRUE;
    }

    DepthStencilFormat = GetStencilFormat(pDriverCaps, DepthStencilFormat);

    /* Observe the multiple conditions */
    for (FormatIndex = 0; FormatIndex < NumFormatOps && (bRenderTargetAvailable == FALSE || bDepthStencilAvailable == FALSE); FormatIndex++)
    {
        const LPDDSURFACEDESC pSurfaceDesc = &pDriverCaps->pSupportedFormatOps[FormatIndex];
        const DWORD FourCC = pSurfaceDesc->ddpfPixelFormat.dwFourCC;
        const DWORD FormatOperations = pSurfaceDesc->ddpfPixelFormat.dwOperations;

        if (FALSE == bRenderTargetAvailable &&
            FourCC == RenderTargetFormat &&
            (FormatOperations & D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET) != 0)
        {
            bRenderTargetAvailable = TRUE;
        }

        if (FALSE == bDepthStencilAvailable &&
            FourCC == DepthStencilFormat &&
            (FormatOperations & D3DFORMAT_OP_ZSTENCIL) != 0)
        {
            bDepthStencilAvailable = TRUE;

            if ((FormatOperations & D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH) != 0)
            {
                bForceSameDepthStencilBits = FALSE;
            }
        }
    }

    if (FALSE == bRenderTargetAvailable || FALSE == bDepthStencilAvailable)
    {
        return D3DERR_INVALIDCALL;
    }

    if (TRUE == bForceSameDepthStencilBits)
    {
        if (GetPixelStride(RenderTargetFormat) != GetPixelStride(DepthStencilFormat))
            return D3DERR_NOTAVAILABLE;
    }

    return D3D_OK;
}
