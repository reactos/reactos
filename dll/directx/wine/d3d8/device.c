/*
 * IDirect3DDevice8 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2004 Christian Costa
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static void STDMETHODCALLTYPE d3d8_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops d3d8_null_wined3d_parent_ops =
{
    d3d8_null_wined3d_object_destroyed,
};

D3DFORMAT d3dformat_from_wined3dformat(enum wined3d_format_id format)
{
    BYTE *c = (BYTE *)&format;

    /* Don't translate FOURCC formats */
    if (isprint(c[0]) && isprint(c[1]) && isprint(c[2]) && isprint(c[3]))
            return (D3DFORMAT)format;

    switch(format)
    {
        case WINED3DFMT_UNKNOWN: return D3DFMT_UNKNOWN;
        case WINED3DFMT_B8G8R8_UNORM: return D3DFMT_R8G8B8;
        case WINED3DFMT_B8G8R8A8_UNORM: return D3DFMT_A8R8G8B8;
        case WINED3DFMT_B8G8R8X8_UNORM: return D3DFMT_X8R8G8B8;
        case WINED3DFMT_B5G6R5_UNORM: return D3DFMT_R5G6B5;
        case WINED3DFMT_B5G5R5X1_UNORM: return D3DFMT_X1R5G5B5;
        case WINED3DFMT_B5G5R5A1_UNORM: return D3DFMT_A1R5G5B5;
        case WINED3DFMT_B4G4R4A4_UNORM: return D3DFMT_A4R4G4B4;
        case WINED3DFMT_B2G3R3_UNORM: return D3DFMT_R3G3B2;
        case WINED3DFMT_A8_UNORM: return D3DFMT_A8;
        case WINED3DFMT_B2G3R3A8_UNORM: return D3DFMT_A8R3G3B2;
        case WINED3DFMT_B4G4R4X4_UNORM: return D3DFMT_X4R4G4B4;
        case WINED3DFMT_R10G10B10A2_UNORM: return D3DFMT_A2B10G10R10;
        case WINED3DFMT_R16G16_UNORM: return D3DFMT_G16R16;
        case WINED3DFMT_P8_UINT_A8_UNORM: return D3DFMT_A8P8;
        case WINED3DFMT_P8_UINT: return D3DFMT_P8;
        case WINED3DFMT_L8_UNORM: return D3DFMT_L8;
        case WINED3DFMT_L8A8_UNORM: return D3DFMT_A8L8;
        case WINED3DFMT_L4A4_UNORM: return D3DFMT_A4L4;
        case WINED3DFMT_R8G8_SNORM: return D3DFMT_V8U8;
        case WINED3DFMT_R5G5_SNORM_L6_UNORM: return D3DFMT_L6V5U5;
        case WINED3DFMT_R8G8_SNORM_L8X8_UNORM: return D3DFMT_X8L8V8U8;
        case WINED3DFMT_R8G8B8A8_SNORM: return D3DFMT_Q8W8V8U8;
        case WINED3DFMT_R16G16_SNORM: return D3DFMT_V16U16;
        case WINED3DFMT_R10G11B11_SNORM: return D3DFMT_W11V11U10;
        case WINED3DFMT_R10G10B10_SNORM_A2_UNORM: return D3DFMT_A2W10V10U10;
        case WINED3DFMT_D16_LOCKABLE: return D3DFMT_D16_LOCKABLE;
        case WINED3DFMT_D32_UNORM: return D3DFMT_D32;
        case WINED3DFMT_S1_UINT_D15_UNORM: return D3DFMT_D15S1;
        case WINED3DFMT_D24_UNORM_S8_UINT: return D3DFMT_D24S8;
        case WINED3DFMT_X8D24_UNORM: return D3DFMT_D24X8;
        case WINED3DFMT_S4X4_UINT_D24_UNORM: return D3DFMT_D24X4S4;
        case WINED3DFMT_D16_UNORM: return D3DFMT_D16;
        case WINED3DFMT_R16_UINT: return D3DFMT_INDEX16;
        case WINED3DFMT_R32_UINT: return D3DFMT_INDEX32;
        default:
            FIXME("Unhandled wined3d format %#x.\n", format);
            return D3DFMT_UNKNOWN;
    }
}

enum wined3d_format_id wined3dformat_from_d3dformat(D3DFORMAT format)
{
    BYTE *c = (BYTE *)&format;

    /* Don't translate FOURCC formats */
    if (isprint(c[0]) && isprint(c[1]) && isprint(c[2]) && isprint(c[3]))
            return (enum wined3d_format_id)format;

    switch(format)
    {
        case D3DFMT_UNKNOWN: return WINED3DFMT_UNKNOWN;
        case D3DFMT_R8G8B8: return WINED3DFMT_B8G8R8_UNORM;
        case D3DFMT_A8R8G8B8: return WINED3DFMT_B8G8R8A8_UNORM;
        case D3DFMT_X8R8G8B8: return WINED3DFMT_B8G8R8X8_UNORM;
        case D3DFMT_R5G6B5: return WINED3DFMT_B5G6R5_UNORM;
        case D3DFMT_X1R5G5B5: return WINED3DFMT_B5G5R5X1_UNORM;
        case D3DFMT_A1R5G5B5: return WINED3DFMT_B5G5R5A1_UNORM;
        case D3DFMT_A4R4G4B4: return WINED3DFMT_B4G4R4A4_UNORM;
        case D3DFMT_R3G3B2: return WINED3DFMT_B2G3R3_UNORM;
        case D3DFMT_A8: return WINED3DFMT_A8_UNORM;
        case D3DFMT_A8R3G3B2: return WINED3DFMT_B2G3R3A8_UNORM;
        case D3DFMT_X4R4G4B4: return WINED3DFMT_B4G4R4X4_UNORM;
        case D3DFMT_A2B10G10R10: return WINED3DFMT_R10G10B10A2_UNORM;
        case D3DFMT_G16R16: return WINED3DFMT_R16G16_UNORM;
        case D3DFMT_A8P8: return WINED3DFMT_P8_UINT_A8_UNORM;
        case D3DFMT_P8: return WINED3DFMT_P8_UINT;
        case D3DFMT_L8: return WINED3DFMT_L8_UNORM;
        case D3DFMT_A8L8: return WINED3DFMT_L8A8_UNORM;
        case D3DFMT_A4L4: return WINED3DFMT_L4A4_UNORM;
        case D3DFMT_V8U8: return WINED3DFMT_R8G8_SNORM;
        case D3DFMT_L6V5U5: return WINED3DFMT_R5G5_SNORM_L6_UNORM;
        case D3DFMT_X8L8V8U8: return WINED3DFMT_R8G8_SNORM_L8X8_UNORM;
        case D3DFMT_Q8W8V8U8: return WINED3DFMT_R8G8B8A8_SNORM;
        case D3DFMT_V16U16: return WINED3DFMT_R16G16_SNORM;
        case D3DFMT_W11V11U10: return WINED3DFMT_R10G11B11_SNORM;
        case D3DFMT_A2W10V10U10: return WINED3DFMT_R10G10B10_SNORM_A2_UNORM;
        case D3DFMT_D16_LOCKABLE: return WINED3DFMT_D16_LOCKABLE;
        case D3DFMT_D32: return WINED3DFMT_D32_UNORM;
        case D3DFMT_D15S1: return WINED3DFMT_S1_UINT_D15_UNORM;
        case D3DFMT_D24S8: return WINED3DFMT_D24_UNORM_S8_UINT;
        case D3DFMT_D24X8: return WINED3DFMT_X8D24_UNORM;
        case D3DFMT_D24X4S4: return WINED3DFMT_S4X4_UINT_D24_UNORM;
        case D3DFMT_D16: return WINED3DFMT_D16_UNORM;
        case D3DFMT_INDEX16: return WINED3DFMT_R16_UINT;
        case D3DFMT_INDEX32: return WINED3DFMT_R32_UINT;
        default:
            FIXME("Unhandled D3DFORMAT %#x.\n", format);
            return WINED3DFMT_UNKNOWN;
    }
}

unsigned int wined3dmapflags_from_d3dmapflags(unsigned int flags, unsigned int usage)
{
    static const unsigned int handled = D3DLOCK_NOSYSLOCK
            | D3DLOCK_NOOVERWRITE
            | D3DLOCK_DISCARD
            | D3DLOCK_NO_DIRTY_UPDATE;
    unsigned int wined3d_flags;

    wined3d_flags = flags & handled;
    if (~usage & D3DUSAGE_WRITEONLY && !(flags & (D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD)))
        wined3d_flags |= WINED3D_MAP_READ;
    if (!(flags & D3DLOCK_READONLY))
        wined3d_flags |= WINED3D_MAP_WRITE;
    if (!(wined3d_flags & (WINED3D_MAP_READ | WINED3D_MAP_WRITE)))
        wined3d_flags |= WINED3D_MAP_WRITE;
    flags &= ~(handled | D3DLOCK_READONLY);

    if (flags)
        FIXME("Unhandled flags %#x.\n", flags);

    return wined3d_flags;
}

static UINT vertex_count_from_primitive_count(D3DPRIMITIVETYPE primitive_type, UINT primitive_count)
{
    switch (primitive_type)
    {
        case D3DPT_POINTLIST:
            return primitive_count;

        case D3DPT_LINELIST:
            return primitive_count * 2;

        case D3DPT_LINESTRIP:
            return primitive_count + 1;

        case D3DPT_TRIANGLELIST:
            return primitive_count * 3;

        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
            return primitive_count + 2;

        default:
            FIXME("Unhandled primitive type %#x.\n", primitive_type);
            return 0;
    }
}

static D3DSWAPEFFECT d3dswapeffect_from_wined3dswapeffect(enum wined3d_swap_effect effect)
{
    switch (effect)
    {
        case WINED3D_SWAP_EFFECT_DISCARD:
            return D3DSWAPEFFECT_DISCARD;
        case WINED3D_SWAP_EFFECT_SEQUENTIAL:
            return D3DSWAPEFFECT_FLIP;
        case WINED3D_SWAP_EFFECT_COPY:
            return D3DSWAPEFFECT_COPY;
        case WINED3D_SWAP_EFFECT_COPY_VSYNC:
            return D3DSWAPEFFECT_COPY_VSYNC;
        default:
            FIXME("Unhandled swap effect %#x.\n", effect);
            return D3DSWAPEFFECT_FLIP;
    }
}

static void present_parameters_from_wined3d_swapchain_desc(D3DPRESENT_PARAMETERS *present_parameters,
        const struct wined3d_swapchain_desc *swapchain_desc, DWORD presentation_interval)
{
    present_parameters->BackBufferWidth = swapchain_desc->backbuffer_width;
    present_parameters->BackBufferHeight = swapchain_desc->backbuffer_height;
    present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(swapchain_desc->backbuffer_format);
    present_parameters->BackBufferCount = swapchain_desc->backbuffer_count;
    present_parameters->MultiSampleType = d3dmultisample_type_from_wined3d(swapchain_desc->multisample_type);
    present_parameters->SwapEffect = d3dswapeffect_from_wined3dswapeffect(swapchain_desc->swap_effect);
    present_parameters->hDeviceWindow = swapchain_desc->device_window;
    present_parameters->Windowed = swapchain_desc->windowed;
    present_parameters->EnableAutoDepthStencil = swapchain_desc->enable_auto_depth_stencil;
    present_parameters->AutoDepthStencilFormat
            = d3dformat_from_wined3dformat(swapchain_desc->auto_depth_stencil_format);
    present_parameters->Flags = swapchain_desc->flags & D3DPRESENTFLAGS_MASK;
    present_parameters->FullScreen_RefreshRateInHz = swapchain_desc->refresh_rate;
    present_parameters->FullScreen_PresentationInterval = presentation_interval;
}

static enum wined3d_swap_effect wined3dswapeffect_from_d3dswapeffect(D3DSWAPEFFECT effect)
{
    switch (effect)
    {
        case D3DSWAPEFFECT_DISCARD:
            return WINED3D_SWAP_EFFECT_DISCARD;
        case D3DSWAPEFFECT_FLIP:
            return WINED3D_SWAP_EFFECT_SEQUENTIAL;
        case D3DSWAPEFFECT_COPY:
            return WINED3D_SWAP_EFFECT_COPY;
        case D3DSWAPEFFECT_COPY_VSYNC:
            return WINED3D_SWAP_EFFECT_COPY_VSYNC;
        default:
            FIXME("Unhandled swap effect %#x.\n", effect);
            return WINED3D_SWAP_EFFECT_SEQUENTIAL;
    }
}

static enum wined3d_swap_interval wined3dswapinterval_from_d3d(DWORD interval)
{
    switch (interval)
    {
        case D3DPRESENT_INTERVAL_IMMEDIATE:
            return WINED3D_SWAP_INTERVAL_IMMEDIATE;
        case D3DPRESENT_INTERVAL_ONE:
            return WINED3D_SWAP_INTERVAL_ONE;
        case D3DPRESENT_INTERVAL_TWO:
            return WINED3D_SWAP_INTERVAL_TWO;
        case D3DPRESENT_INTERVAL_THREE:
            return WINED3D_SWAP_INTERVAL_THREE;
        case D3DPRESENT_INTERVAL_FOUR:
            return WINED3D_SWAP_INTERVAL_FOUR;
        default:
            FIXME("Unhandled presentation interval %#lx.\n", interval);
        case D3DPRESENT_INTERVAL_DEFAULT:
            return WINED3D_SWAP_INTERVAL_DEFAULT;
    }
}

static enum wined3d_multisample_type wined3d_multisample_type_from_d3d(D3DMULTISAMPLE_TYPE type)
{
    return (enum wined3d_multisample_type)type;
}

static BOOL wined3d_swapchain_desc_from_d3d8(struct wined3d_swapchain_desc *swapchain_desc,
        struct wined3d_output *output, const D3DPRESENT_PARAMETERS *present_parameters)
{
    if (!present_parameters->SwapEffect || present_parameters->SwapEffect > D3DSWAPEFFECT_COPY_VSYNC)
    {
        WARN("Invalid swap effect %u passed.\n", present_parameters->SwapEffect);
        return FALSE;
    }
    if (present_parameters->BackBufferCount > 3
            || ((present_parameters->SwapEffect == D3DSWAPEFFECT_COPY
            || present_parameters->SwapEffect == D3DSWAPEFFECT_COPY_VSYNC)
            && present_parameters->BackBufferCount > 1))
    {
        WARN("Invalid backbuffer count %u.\n", present_parameters->BackBufferCount);
        return FALSE;
    }
    switch (present_parameters->FullScreen_PresentationInterval)
    {
        case D3DPRESENT_INTERVAL_DEFAULT:
        case D3DPRESENT_INTERVAL_ONE:
        case D3DPRESENT_INTERVAL_TWO:
        case D3DPRESENT_INTERVAL_THREE:
        case D3DPRESENT_INTERVAL_FOUR:
        case D3DPRESENT_INTERVAL_IMMEDIATE:
            break;
        default:
            WARN("Invalid presentation interval %#x.\n",
                    present_parameters->FullScreen_PresentationInterval);
            return FALSE;
    }

    swapchain_desc->output = output;
    swapchain_desc->backbuffer_width = present_parameters->BackBufferWidth;
    swapchain_desc->backbuffer_height = present_parameters->BackBufferHeight;
    swapchain_desc->backbuffer_format = wined3dformat_from_d3dformat(present_parameters->BackBufferFormat);
    swapchain_desc->backbuffer_count = max(1, present_parameters->BackBufferCount);
    swapchain_desc->backbuffer_bind_flags = WINED3D_BIND_RENDER_TARGET;
    swapchain_desc->multisample_type = wined3d_multisample_type_from_d3d(present_parameters->MultiSampleType);
    swapchain_desc->multisample_quality = 0; /* d3d9 only */
    swapchain_desc->swap_effect = wined3dswapeffect_from_d3dswapeffect(present_parameters->SwapEffect);
    swapchain_desc->device_window = present_parameters->hDeviceWindow;
    swapchain_desc->windowed = present_parameters->Windowed;
    swapchain_desc->enable_auto_depth_stencil = present_parameters->EnableAutoDepthStencil;
    swapchain_desc->auto_depth_stencil_format
            = wined3dformat_from_d3dformat(present_parameters->AutoDepthStencilFormat);
    swapchain_desc->flags
            = (present_parameters->Flags & D3DPRESENTFLAGS_MASK) | WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH;
    swapchain_desc->refresh_rate = present_parameters->FullScreen_RefreshRateInHz;
    swapchain_desc->auto_restore_display_mode = TRUE;

    if (present_parameters->Flags & ~D3DPRESENTFLAGS_MASK)
        FIXME("Unhandled flags %#lx.\n", present_parameters->Flags & ~D3DPRESENTFLAGS_MASK);

    return TRUE;
}

void d3dcaps_from_wined3dcaps(D3DCAPS8 *caps, const struct wined3d_caps *wined3d_caps,
        unsigned int adapter_ordinal)
{
    caps->DeviceType                = (D3DDEVTYPE)wined3d_caps->DeviceType;
    caps->AdapterOrdinal            = adapter_ordinal;
    caps->Caps                      = wined3d_caps->Caps;
    caps->Caps2                     = wined3d_caps->Caps2;
    caps->Caps3                     = wined3d_caps->Caps3;
    caps->PresentationIntervals     = D3DPRESENT_INTERVAL_IMMEDIATE | D3DPRESENT_INTERVAL_ONE;
    caps->CursorCaps                = wined3d_caps->CursorCaps;
    caps->DevCaps                   = wined3d_caps->DevCaps;
    caps->PrimitiveMiscCaps         = wined3d_caps->PrimitiveMiscCaps;
    caps->RasterCaps                = wined3d_caps->RasterCaps;
    caps->ZCmpCaps                  = wined3d_caps->ZCmpCaps;
    caps->SrcBlendCaps              = wined3d_caps->SrcBlendCaps;
    caps->DestBlendCaps             = wined3d_caps->DestBlendCaps;
    caps->AlphaCmpCaps              = wined3d_caps->AlphaCmpCaps;
    caps->ShadeCaps                 = wined3d_caps->ShadeCaps;
    caps->TextureCaps               = wined3d_caps->TextureCaps;
    caps->TextureFilterCaps         = wined3d_caps->TextureFilterCaps;
    caps->CubeTextureFilterCaps     = wined3d_caps->CubeTextureFilterCaps;
    caps->VolumeTextureFilterCaps   = wined3d_caps->VolumeTextureFilterCaps;
    caps->TextureAddressCaps        = wined3d_caps->TextureAddressCaps;
    caps->VolumeTextureAddressCaps  = wined3d_caps->VolumeTextureAddressCaps;
    caps->LineCaps                  = wined3d_caps->LineCaps;
    caps->MaxTextureWidth           = wined3d_caps->MaxTextureWidth;
    caps->MaxTextureHeight          = wined3d_caps->MaxTextureHeight;
    caps->MaxVolumeExtent           = wined3d_caps->MaxVolumeExtent;
    caps->MaxTextureRepeat          = wined3d_caps->MaxTextureRepeat;
    caps->MaxTextureAspectRatio     = wined3d_caps->MaxTextureAspectRatio;
    caps->MaxAnisotropy             = wined3d_caps->MaxAnisotropy;
    caps->MaxVertexW                = wined3d_caps->MaxVertexW;
    caps->GuardBandLeft             = wined3d_caps->GuardBandLeft;
    caps->GuardBandTop              = wined3d_caps->GuardBandTop;
    caps->GuardBandRight            = wined3d_caps->GuardBandRight;
    caps->GuardBandBottom           = wined3d_caps->GuardBandBottom;
    caps->ExtentsAdjust             = wined3d_caps->ExtentsAdjust;
    caps->StencilCaps               = wined3d_caps->StencilCaps;
    caps->FVFCaps                   = wined3d_caps->FVFCaps;
    caps->TextureOpCaps             = wined3d_caps->TextureOpCaps;
    caps->MaxTextureBlendStages     = wined3d_caps->MaxTextureBlendStages;
    caps->MaxSimultaneousTextures   = wined3d_caps->MaxSimultaneousTextures;
    caps->VertexProcessingCaps      = wined3d_caps->VertexProcessingCaps;
    caps->MaxActiveLights           = wined3d_caps->MaxActiveLights;
    caps->MaxUserClipPlanes         = wined3d_caps->MaxUserClipPlanes;
    caps->MaxVertexBlendMatrices    = wined3d_caps->MaxVertexBlendMatrices;
    caps->MaxVertexBlendMatrixIndex = wined3d_caps->MaxVertexBlendMatrixIndex;
    caps->MaxPointSize              = wined3d_caps->MaxPointSize;
    caps->MaxPrimitiveCount         = wined3d_caps->MaxPrimitiveCount;
    caps->MaxVertexIndex            = wined3d_caps->MaxVertexIndex;
    caps->MaxStreams                = wined3d_caps->MaxStreams;
    caps->MaxStreamStride           = wined3d_caps->MaxStreamStride;
    caps->VertexShaderVersion       = wined3d_caps->VertexShaderVersion;
    caps->MaxVertexShaderConst      = wined3d_caps->MaxVertexShaderConst;
    caps->PixelShaderVersion        = wined3d_caps->PixelShaderVersion;
    caps->MaxPixelShaderValue       = wined3d_caps->PixelShader1xMaxValue;

    caps->Caps2 &= D3DCAPS2_CANCALIBRATEGAMMA | D3DCAPS2_CANRENDERWINDOWED
            | D3DCAPS2_CANMANAGERESOURCE | D3DCAPS2_DYNAMICTEXTURES | D3DCAPS2_FULLSCREENGAMMA
            | D3DCAPS2_NO2DDURING3DSCENE | D3DCAPS2_RESERVED;
    caps->Caps3 &= D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD | D3DCAPS3_RESERVED;
    caps->PrimitiveMiscCaps &= D3DPMISCCAPS_MASKZ | D3DPMISCCAPS_LINEPATTERNREP
            | D3DPMISCCAPS_CULLNONE | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLCCW
            | D3DPMISCCAPS_COLORWRITEENABLE | D3DPMISCCAPS_CLIPPLANESCALEDPOINTS
            | D3DPMISCCAPS_CLIPTLVERTS | D3DPMISCCAPS_TSSARGTEMP | D3DPMISCCAPS_BLENDOP
            | D3DPMISCCAPS_NULLREFERENCE;
    caps->RasterCaps &= D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_PAT | D3DPRASTERCAPS_ZTEST
            | D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_FOGTABLE | D3DPRASTERCAPS_ANTIALIASEDGES
            | D3DPRASTERCAPS_MIPMAPLODBIAS | D3DPRASTERCAPS_ZBUFFERLESSHSR
            | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_ANISOTROPY | D3DPRASTERCAPS_WBUFFER
            | D3DPRASTERCAPS_WFOG | D3DPRASTERCAPS_ZFOG | D3DPRASTERCAPS_COLORPERSPECTIVE
            | D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE | WINED3DPRASTERCAPS_DEPTHBIAS;
    if (caps->RasterCaps & WINED3DPRASTERCAPS_DEPTHBIAS)
        caps->RasterCaps = (caps->RasterCaps | D3DPRASTERCAPS_ZBIAS) & ~WINED3DPRASTERCAPS_DEPTHBIAS;
    caps->SrcBlendCaps &= D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR
            | D3DPBLENDCAPS_INVSRCCOLOR | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA
            | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR
            | D3DPBLENDCAPS_INVDESTCOLOR | D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA
            | D3DPBLENDCAPS_BOTHINVSRCALPHA;
    caps->DestBlendCaps &= D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR
            | D3DPBLENDCAPS_INVSRCCOLOR | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA
            | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR
            | D3DPBLENDCAPS_INVDESTCOLOR | D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA
            | D3DPBLENDCAPS_BOTHINVSRCALPHA;
    caps->TextureCaps &= D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_ALPHA
            | D3DPTEXTURECAPS_SQUAREONLY | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE
            | D3DPTEXTURECAPS_ALPHAPALETTE | D3DPTEXTURECAPS_NONPOW2CONDITIONAL
            | D3DPTEXTURECAPS_PROJECTED | D3DPTEXTURECAPS_CUBEMAP | D3DPTEXTURECAPS_VOLUMEMAP
            | D3DPTEXTURECAPS_MIPMAP | D3DPTEXTURECAPS_MIPVOLUMEMAP | D3DPTEXTURECAPS_MIPCUBEMAP
            | D3DPTEXTURECAPS_CUBEMAP_POW2 | D3DPTEXTURECAPS_VOLUMEMAP_POW2;
    caps->TextureFilterCaps &= D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
            | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MIPFPOINT
            | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
            | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFAFLATCUBIC
            | D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC;
    caps->CubeTextureFilterCaps &= D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
            | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MIPFPOINT
            | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
            | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFAFLATCUBIC
            | D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC;
    caps->VolumeTextureFilterCaps &= D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
            | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MIPFPOINT
            | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
            | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFAFLATCUBIC
            | D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC;
    if (caps->LineCaps & WINED3DLINECAPS_ANTIALIAS)
        caps->RasterCaps |= D3DPRASTERCAPS_ANTIALIASEDGES;
    caps->LineCaps &= ~WINED3DLINECAPS_ANTIALIAS;
    caps->StencilCaps &= ~WINED3DSTENCILCAPS_TWOSIDED;
    caps->VertexProcessingCaps &= D3DVTXPCAPS_TEXGEN | D3DVTXPCAPS_MATERIALSOURCE7
            | D3DVTXPCAPS_DIRECTIONALLIGHTS | D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_LOCALVIEWER
            | D3DVTXPCAPS_TWEENING | D3DVTXPCAPS_NO_VSDT_UBYTE4;

    /* D3D8 doesn't support SM 2.0 or higher, so clamp to 1.x */
    if (caps->PixelShaderVersion)
        caps->PixelShaderVersion = D3DPS_VERSION(1, 4);
    else
        caps->PixelShaderVersion = D3DPS_VERSION(0, 0);
    if (caps->VertexShaderVersion)
        caps->VertexShaderVersion = D3DVS_VERSION(1, 1);
    else
        caps->VertexShaderVersion = D3DVS_VERSION(0, 0);
    caps->MaxVertexShaderConst = min(D3D8_MAX_VERTEX_SHADER_CONSTANTF, caps->MaxVertexShaderConst);
}

static enum wined3d_transform_state wined3d_transform_state_from_d3d(D3DTRANSFORMSTATETYPE state)
{
    return (enum wined3d_transform_state)state;
}

static enum wined3d_render_state wined3d_render_state_from_d3d(D3DRENDERSTATETYPE state)
{
    switch (state)
    {
        case D3DRS_ZBIAS:
            return WINED3D_RS_DEPTHBIAS;
        case D3DRS_EDGEANTIALIAS:
            return WINED3D_RS_ANTIALIASEDLINEENABLE;
        default:
            return (enum wined3d_render_state)state;
    }
}

static enum wined3d_primitive_type wined3d_primitive_type_from_d3d(D3DPRIMITIVETYPE type)
{
    return (enum wined3d_primitive_type)type;
}

/* Handle table functions */
static DWORD d3d8_allocate_handle(struct d3d8_handle_table *t, void *object, enum d3d8_handle_type type)
{
    struct d3d8_handle_entry *entry;

    if (t->free_entries)
    {
        DWORD index = t->free_entries - t->entries;
        /* Use a free handle */
        entry = t->free_entries;
        if (entry->type != D3D8_HANDLE_FREE)
        {
            ERR("Handle %lu(%p) is in the free list, but has type %#x.\n", index, entry, entry->type);
            return D3D8_INVALID_HANDLE;
        }
        t->free_entries = entry->object;
        entry->object = object;
        entry->type = type;

        return index;
    }

    if (!(t->entry_count < t->table_size))
    {
        /* Grow the table */
        UINT new_size = t->table_size + (t->table_size >> 1);
        struct d3d8_handle_entry *new_entries;

        if (!(new_entries = realloc(t->entries, new_size * sizeof(*t->entries))))
        {
            ERR("Failed to grow the handle table.\n");
            return D3D8_INVALID_HANDLE;
        }
        t->entries = new_entries;
        t->table_size = new_size;
    }

    entry = &t->entries[t->entry_count];
    entry->object = object;
    entry->type = type;

    return t->entry_count++;
}

static void *d3d8_free_handle(struct d3d8_handle_table *t, DWORD handle, enum d3d8_handle_type type)
{
    struct d3d8_handle_entry *entry;
    void *object;

    if (handle == D3D8_INVALID_HANDLE || handle >= t->entry_count)
    {
        WARN("Invalid handle %lu passed.\n", handle);
        return NULL;
    }

    entry = &t->entries[handle];
    if (entry->type != type)
    {
        WARN("Handle %lu(%p) is not of type %#x.\n", handle, entry, type);
        return NULL;
    }

    object = entry->object;
    entry->object = t->free_entries;
    entry->type = D3D8_HANDLE_FREE;
    t->free_entries = entry;

    return object;
}

static void *d3d8_get_object(struct d3d8_handle_table *t, DWORD handle, enum d3d8_handle_type type)
{
    struct d3d8_handle_entry *entry;

    if (handle == D3D8_INVALID_HANDLE || handle >= t->entry_count)
    {
        WARN("Invalid handle %lu passed.\n", handle);
        return NULL;
    }

    entry = &t->entries[handle];
    if (entry->type != type)
    {
        WARN("Handle %lu(%p) is not of type %#x.\n", handle, entry, type);
        return NULL;
    }

    return entry->object;
}

static void device_reset_viewport_state(struct d3d8_device *device)
{
    struct wined3d_viewport vp;
    RECT rect;

    wined3d_device_context_get_viewports(device->immediate_context, NULL, &vp);
    wined3d_stateblock_set_viewport(device->state, &vp);
    wined3d_device_context_get_scissor_rects(device->immediate_context, NULL, &rect);
    wined3d_stateblock_set_scissor_rect(device->state, &rect);
}

static HRESULT WINAPI d3d8_device_QueryInterface(IDirect3DDevice8 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n",
            iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DDevice8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DDevice8_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_device_AddRef(IDirect3DDevice8 *iface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    ULONG ref = InterlockedIncrement(&device->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI d3d8_device_Release(IDirect3DDevice8 *iface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    ULONG ref;

    if (device->in_destruction)
        return 0;

    ref = InterlockedDecrement(&device->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, ref);

    if (!ref)
    {
        IDirect3D8 *parent = &device->d3d_parent->IDirect3D8_iface;
        unsigned i;

        TRACE("Releasing wined3d device %p.\n", device->wined3d_device);

        wined3d_mutex_lock();

        device->in_destruction = TRUE;

        for (i = 0; i < device->numConvertedDecls; ++i)
        {
            d3d8_vertex_declaration_destroy(device->decls[i].declaration);
        }
        free(device->decls);

        wined3d_streaming_buffer_cleanup(&device->vertex_buffer);
        wined3d_streaming_buffer_cleanup(&device->index_buffer);

        if (device->recording)
            wined3d_stateblock_decref(device->recording);
        wined3d_stateblock_decref(device->state);

        wined3d_swapchain_decref(device->implicit_swapchain);
        wined3d_device_release_focus_window(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        free(device->handle_table.entries);
        free(device);

        wined3d_mutex_unlock();

        IDirect3D8_Release(parent);
    }
    return ref;
}

static HRESULT WINAPI d3d8_device_TestCooperativeLevel(IDirect3DDevice8 *iface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p.\n", iface);

    TRACE("device state: %#lx.\n", device->device_state);

    switch (device->device_state)
    {
        default:
        case D3D8_DEVICE_STATE_OK:
            return D3D_OK;
        case D3D8_DEVICE_STATE_LOST:
            return D3DERR_DEVICELOST;
        case D3D8_DEVICE_STATE_NOT_RESET:
            return D3DERR_DEVICENOTRESET;
    }
}

static UINT WINAPI d3d8_device_GetAvailableTextureMem(IDirect3DDevice8 *iface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    UINT ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_device_get_available_texture_mem(device->wined3d_device);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d8_device_ResourceManagerDiscardBytes(IDirect3DDevice8 *iface, DWORD byte_count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, byte_count %lu.\n", iface, byte_count);

    if (byte_count)
        FIXME("Byte count ignored.\n");

    wined3d_mutex_lock();
    wined3d_device_evict_managed_resources(device->wined3d_device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetDirect3D(IDirect3DDevice8 *iface, IDirect3D8 **d3d8)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, d3d8 %p.\n", iface, d3d8);

    if (!d3d8)
        return D3DERR_INVALIDCALL;

    return IDirect3D8_QueryInterface(&device->d3d_parent->IDirect3D8_iface, &IID_IDirect3D8,
            (void **)d3d8);
}

static HRESULT WINAPI d3d8_device_GetDeviceCaps(IDirect3DDevice8 *iface, D3DCAPS8 *caps)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_caps wined3d_caps;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    if (!caps)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_device_get_device_caps(device->wined3d_device, &wined3d_caps);
    wined3d_mutex_unlock();

    d3dcaps_from_wined3dcaps(caps, &wined3d_caps, device->adapter_ordinal);

    return hr;
}

static HRESULT WINAPI d3d8_device_GetDisplayMode(IDirect3DDevice8 *iface, D3DDISPLAYMODE *mode)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, mode %p.\n", iface, mode);

    wined3d_mutex_lock();
    hr = wined3d_device_get_display_mode(device->wined3d_device, 0, &wined3d_mode, NULL);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
    }

    return hr;
}

static HRESULT WINAPI d3d8_device_GetCreationParameters(IDirect3DDevice8 *iface,
        D3DDEVICE_CREATION_PARAMETERS *parameters)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, parameters %p.\n", iface, parameters);

    wined3d_mutex_lock();
    wined3d_device_get_creation_parameters(device->wined3d_device,
            (struct wined3d_device_creation_parameters *)parameters);
    wined3d_mutex_unlock();

    parameters->AdapterOrdinal = device->adapter_ordinal;
    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetCursorProperties(IDirect3DDevice8 *iface,
        UINT hotspot_x, UINT hotspot_y, IDirect3DSurface8 *bitmap)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_surface *bitmap_impl = unsafe_impl_from_IDirect3DSurface8(bitmap);
    D3DSURFACE_DESC surface_desc;
    D3DDISPLAYMODE mode;
    HRESULT hr;

    TRACE("iface %p, hotspot_x %u, hotspot_y %u, bitmap %p.\n",
            iface, hotspot_x, hotspot_y, bitmap);

    if (!bitmap)
    {
        WARN("No cursor bitmap, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(hr = IDirect3DSurface8_GetDesc(bitmap, &surface_desc)))
    {
        WARN("Failed to get surface description, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IDirect3D8_GetAdapterDisplayMode(&device->d3d_parent->IDirect3D8_iface,
            device->adapter_ordinal, &mode)))
    {
        WARN("Failed to get device display mode, hr %#lx.\n", hr);
        return hr;
    }

    if (surface_desc.Width > mode.Width || surface_desc.Height > mode.Height)
    {
        WARN("Surface dimension %ux%u exceeds display mode %ux%u.\n", surface_desc.Width,
                surface_desc.Height, mode.Width, mode.Height);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_set_cursor_properties(device->wined3d_device,
            hotspot_x, hotspot_y, bitmap_impl->wined3d_texture, bitmap_impl->sub_resource_idx);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI d3d8_device_SetCursorPosition(IDirect3DDevice8 *iface, UINT x, UINT y, DWORD flags)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, x %u, y %u, flags %#lx.\n", iface, x, y, flags);

    wined3d_mutex_lock();
    wined3d_device_set_cursor_position(device->wined3d_device, x, y, flags);
    wined3d_mutex_unlock();
}

static BOOL WINAPI d3d8_device_ShowCursor(IDirect3DDevice8 *iface, BOOL show)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    BOOL ret;

    TRACE("iface %p, show %#x.\n", iface, show);

    wined3d_mutex_lock();
    ret = wined3d_device_show_cursor(device->wined3d_device, show);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d8_device_CreateAdditionalSwapChain(IDirect3DDevice8 *iface,
        D3DPRESENT_PARAMETERS *present_parameters, IDirect3DSwapChain8 **swapchain)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_swapchain_desc desc;
    struct d3d8_swapchain *object;
    unsigned int swap_interval;
    unsigned int output_idx;
    unsigned int i, count;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p, swapchain %p.\n",
            iface, present_parameters, swapchain);

    if (!present_parameters->Windowed)
    {
        WARN("Trying to create an additional fullscreen swapchain, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    count = wined3d_device_get_swapchain_count(device->wined3d_device);
    for (i = 0; i < count; ++i)
    {
        struct wined3d_swapchain *wined3d_swapchain;

        wined3d_swapchain = wined3d_device_get_swapchain(device->wined3d_device, i);
        wined3d_swapchain_get_desc(wined3d_swapchain, &desc);

        if (!desc.windowed)
        {
            wined3d_mutex_unlock();
            WARN("Trying to create an additional swapchain in fullscreen mode, returning D3DERR_INVALIDCALL.\n");
            return D3DERR_INVALIDCALL;
        }
    }
    wined3d_mutex_unlock();

    output_idx = device->adapter_ordinal;
    if (!wined3d_swapchain_desc_from_d3d8(&desc, device->d3d_parent->wined3d_outputs[output_idx],
            present_parameters))
        return D3DERR_INVALIDCALL;
    swap_interval = wined3dswapinterval_from_d3d(present_parameters->FullScreen_PresentationInterval);
    if (SUCCEEDED(hr = d3d8_swapchain_create(device, &desc, swap_interval, &object)))
        *swapchain = &object->IDirect3DSwapChain8_iface;
    present_parameters_from_wined3d_swapchain_desc(present_parameters,
            &desc, present_parameters->FullScreen_PresentationInterval);

    return hr;
}

static HRESULT CDECL reset_enum_callback(struct wined3d_resource *resource)
{
    struct d3d8_vertexbuffer *vertex_buffer;
    struct d3d8_indexbuffer *index_buffer;
    struct wined3d_resource_desc desc;
    IDirect3DBaseTexture8 *texture;
    struct d3d8_surface *surface;
    IUnknown *parent;

    wined3d_resource_get_desc(resource, &desc);
    if ((desc.access & WINED3D_RESOURCE_ACCESS_CPU) || (desc.usage & WINED3DUSAGE_MANAGED))
        return D3D_OK;

    if (desc.resource_type != WINED3D_RTYPE_TEXTURE_2D)
    {
        if (desc.bind_flags & WINED3D_BIND_VERTEX_BUFFER)
        {
            vertex_buffer = wined3d_resource_get_parent(resource);
            if (vertex_buffer && vertex_buffer->draw_buffer)
                return D3D_OK;
        }
        if (desc.bind_flags & WINED3D_BIND_INDEX_BUFFER)
        {
            index_buffer = wined3d_resource_get_parent(resource);
            if (index_buffer && index_buffer->sysmem)
                return D3D_OK;
        }

        WARN("Resource %p in pool D3DPOOL_DEFAULT blocks the Reset call.\n", resource);
        return D3DERR_DEVICELOST;
    }

    parent = wined3d_resource_get_parent(resource);
    if (parent && SUCCEEDED(IUnknown_QueryInterface(parent, &IID_IDirect3DBaseTexture8, (void **)&texture)))
    {
        IDirect3DBaseTexture8_Release(texture);
        WARN("Texture %p (resource %p) in pool D3DPOOL_DEFAULT blocks the Reset call.\n", texture, resource);
        return D3DERR_DEVICELOST;
    }

    surface = wined3d_texture_get_sub_resource_parent(wined3d_texture_from_resource(resource), 0);
    if (!surface || !surface->resource.refcount)
        return D3D_OK;

    WARN("Surface %p in pool D3DPOOL_DEFAULT blocks the Reset call.\n", surface);
    return D3DERR_DEVICELOST;
}

static HRESULT WINAPI d3d8_device_Reset(IDirect3DDevice8 *iface,
        D3DPRESENT_PARAMETERS *present_parameters)
{
    struct wined3d_swapchain_desc swapchain_desc, old_swapchain_desc;
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_swapchain *implicit_swapchain;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p.\n", iface, present_parameters);

    if (device->device_state == D3D8_DEVICE_STATE_LOST)
    {
        WARN("App not active, returning D3DERR_DEVICELOST.\n");
        return D3DERR_DEVICELOST;
    }
    output_idx = device->adapter_ordinal;
    if (!wined3d_swapchain_desc_from_d3d8(&swapchain_desc,
            device->d3d_parent->wined3d_outputs[output_idx], present_parameters))
        return D3DERR_INVALIDCALL;
    swapchain_desc.flags |= WINED3D_SWAPCHAIN_IMPLICIT;

    wined3d_mutex_lock();

    wined3d_streaming_buffer_cleanup(&device->vertex_buffer);
    wined3d_streaming_buffer_cleanup(&device->index_buffer);

    wined3d_swapchain_get_desc(device->implicit_swapchain, &old_swapchain_desc);

    if (device->recording)
        wined3d_stateblock_decref(device->recording);
    device->recording = NULL;
    device->update_state = device->state;
    wined3d_stateblock_reset(device->state);

    /* wined3d_device_reset() may recreate swapchain textures.
     * We do not need to remove the reference to the wined3d swapchain from the
     * old d3d8 surfaces: we will fail the reset if they have 0 references,
     * and therefore they are not actually holding a reference to the wined3d
     * swapchain, and will not do anything with it when they are destroyed. */

    if (SUCCEEDED(hr = wined3d_device_reset(device->wined3d_device, &swapchain_desc,
            NULL, reset_enum_callback, TRUE)))
    {
        struct wined3d_rendertarget_view *rtv;
        struct d3d8_surface *surface;
        unsigned int i;

        present_parameters->BackBufferCount = swapchain_desc.backbuffer_count;
        implicit_swapchain = wined3d_swapchain_get_parent(device->implicit_swapchain);
        implicit_swapchain->swap_interval
                = wined3dswapinterval_from_d3d(present_parameters->FullScreen_PresentationInterval);
        wined3d_stateblock_set_render_state(device->state, WINED3D_RS_POINTSIZE_MIN, 0);
        wined3d_stateblock_set_render_state(device->state, WINED3D_RS_ZENABLE,
                !!swapchain_desc.enable_auto_depth_stencil);
        device_reset_viewport_state(device);

        /* FIXME: This should be the new backbuffer count, but we don't support
         * changing the backbuffer count in wined3d yet. */
        for (i = 0; i < old_swapchain_desc.backbuffer_count; ++i)
        {
            struct wined3d_texture *backbuffer = wined3d_swapchain_get_back_buffer(device->implicit_swapchain, i);

            if ((surface = d3d8_surface_create(backbuffer, 0, (IUnknown *)&device->IDirect3DDevice8_iface)))
                surface->parent_device = &device->IDirect3DDevice8_iface;
        }

        if ((rtv = wined3d_device_context_get_depth_stencil_view(device->immediate_context)))
        {
            struct wined3d_resource *resource = wined3d_rendertarget_view_get_resource(rtv);

            if ((surface = d3d8_surface_create(wined3d_texture_from_resource(resource), 0,
                    (IUnknown *)&device->IDirect3DDevice8_iface)))
                surface->parent_device = &device->IDirect3DDevice8_iface;
        }

        device->device_state = D3D8_DEVICE_STATE_OK;
    }
    else
    {
        device->device_state = D3D8_DEVICE_STATE_NOT_RESET;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_Present(IDirect3DDevice8 *iface, const RECT *src_rect,
        const RECT *dst_rect, HWND dst_window_override, const RGNDATA *dirty_region)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_swapchain *implicit_swapchain;

    TRACE("iface %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p.\n",
            iface, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect), dst_window_override, dirty_region);

    /* Fraps does not hook IDirect3DDevice8::Present regardless of the hotpatch
     * attribute. It only hooks IDirect3DSwapChain8::Present. Yet it properly
     * shows a framerate on Windows in applications that only call the device
     * method, like e.g. the dx8 sdk samples. The conclusion is that native
     * calls the swapchain's public method from the device. */
    implicit_swapchain = wined3d_swapchain_get_parent(device->implicit_swapchain);
    return IDirect3DSwapChain8_Present(&implicit_swapchain->IDirect3DSwapChain8_iface,
            src_rect, dst_rect, dst_window_override, dirty_region);
}

static HRESULT WINAPI d3d8_device_GetBackBuffer(IDirect3DDevice8 *iface,
        UINT backbuffer_idx, D3DBACKBUFFER_TYPE backbuffer_type, IDirect3DSurface8 **backbuffer)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_texture *wined3d_texture;
    struct d3d8_surface *surface_impl;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, backbuffer_idx, backbuffer_type, backbuffer);

    /* backbuffer_type is ignored by native. */

    /* No need to check for backbuffer == NULL, Windows crashes in that case. */
    wined3d_mutex_lock();

    if (!(wined3d_texture = wined3d_swapchain_get_back_buffer(device->implicit_swapchain, backbuffer_idx)))
    {
        wined3d_mutex_unlock();
        *backbuffer = NULL;
        return D3DERR_INVALIDCALL;
    }

    surface_impl = wined3d_texture_get_sub_resource_parent(wined3d_texture, 0);
    *backbuffer = &surface_impl->IDirect3DSurface8_iface;
    IDirect3DSurface8_AddRef(*backbuffer);

    wined3d_mutex_unlock();
    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetRasterStatus(IDirect3DDevice8 *iface, D3DRASTER_STATUS *raster_status)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, raster_status %p.\n", iface, raster_status);

    wined3d_mutex_lock();
    hr = wined3d_device_get_raster_status(device->wined3d_device, 0, (struct wined3d_raster_status *)raster_status);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI d3d8_device_SetGammaRamp(IDirect3DDevice8 *iface, DWORD flags, const D3DGAMMARAMP *ramp)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, flags %#lx, ramp %p.\n", iface, flags, ramp);

    /* Note: D3DGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
    wined3d_mutex_lock();
    wined3d_device_set_gamma_ramp(device->wined3d_device, 0, flags, (const struct wined3d_gamma_ramp *)ramp);
    wined3d_mutex_unlock();
}

static void WINAPI d3d8_device_GetGammaRamp(IDirect3DDevice8 *iface, D3DGAMMARAMP *ramp)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, ramp %p.\n", iface, ramp);

    /* Note: D3DGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
    wined3d_mutex_lock();
    wined3d_device_get_gamma_ramp(device->wined3d_device, 0, (struct wined3d_gamma_ramp *)ramp);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI d3d8_device_CreateTexture(IDirect3DDevice8 *iface,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DTexture8 **texture)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_texture *object;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, levels %u, usage %#lx, format %#x, pool %#x, texture %p.\n",
            iface, width, height, levels, usage, format, pool, texture);

    if (!format)
        return D3DERR_INVALIDCALL;

    *texture = NULL;
    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = d3d8_texture_2d_init(object, device, width, height, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize texture, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    levels = wined3d_texture_get_level_count(object->wined3d_texture);
    for (i = 0; i < levels; ++i)
    {
        if (!d3d8_surface_create(object->wined3d_texture, i, (IUnknown *)&object->IDirect3DBaseTexture8_iface))
        {
            IDirect3DTexture8_Release(&object->IDirect3DBaseTexture8_iface);
            return E_OUTOFMEMORY;
        }
    }

    TRACE("Created texture %p.\n", object);
    *texture = (IDirect3DTexture8 *)&object->IDirect3DBaseTexture8_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_CreateVolumeTexture(IDirect3DDevice8 *iface,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DVolumeTexture8 **texture)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_texture *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, depth %u, levels %u, usage %#lx, format %#x, pool %#x, texture %p.\n",
            iface, width, height, depth, levels, usage, format, pool, texture);

    if (!format)
        return D3DERR_INVALIDCALL;

    *texture = NULL;
    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = d3d8_texture_3d_init(object, device, width, height, depth, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize volume texture, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created volume texture %p.\n", object);
    *texture = (IDirect3DVolumeTexture8 *)&object->IDirect3DBaseTexture8_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_CreateCubeTexture(IDirect3DDevice8 *iface, UINT edge_length,
        UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture8 **texture)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_texture *object;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, edge_length %u, levels %u, usage %#lx, format %#x, pool %#x, texture %p.\n",
            iface, edge_length, levels, usage, format, pool, texture);

    if (!format)
        return D3DERR_INVALIDCALL;

    *texture = NULL;
    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = d3d8_texture_cube_init(object, device, edge_length, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize cube texture, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    levels = wined3d_texture_get_level_count(object->wined3d_texture);
    for (i = 0; i < levels * 6; ++i)
    {
        if (!d3d8_surface_create(object->wined3d_texture, i, (IUnknown *)&object->IDirect3DBaseTexture8_iface))
        {
            IDirect3DTexture8_Release(&object->IDirect3DBaseTexture8_iface);
            return E_OUTOFMEMORY;
        }
    }

    TRACE("Created cube texture %p.\n", object);
    *texture = (IDirect3DCubeTexture8 *)&object->IDirect3DBaseTexture8_iface;

    return hr;
}

static HRESULT WINAPI d3d8_device_CreateVertexBuffer(IDirect3DDevice8 *iface, UINT size,
        DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer8 **buffer)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertexbuffer *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#lx, fvf %#lx, pool %#x, buffer %p.\n",
            iface, size, usage, fvf, pool, buffer);

    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = vertexbuffer_init(object, device, size, usage, fvf, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex buffer, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created vertex buffer %p.\n", object);
    *buffer = &object->IDirect3DVertexBuffer8_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_CreateIndexBuffer(IDirect3DDevice8 *iface, UINT size,
        DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer8 **buffer)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_indexbuffer *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#lx, format %#x, pool %#x, buffer %p.\n",
            iface, size, usage, format, pool, buffer);

    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = indexbuffer_init(object, device, size, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize index buffer, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created index buffer %p.\n", object);
    *buffer = &object->IDirect3DIndexBuffer8_iface;

    return D3D_OK;
}

static HRESULT d3d8_device_create_surface(struct d3d8_device *device, enum wined3d_format_id format,
        enum wined3d_multisample_type multisample_type, unsigned int bind_flags, unsigned int access,
        unsigned int width, unsigned int height, IDirect3DSurface8 **surface)
{
    struct wined3d_resource_desc desc;
    struct d3d8_surface *surface_impl;
    struct wined3d_texture *texture;
    HRESULT hr;

    TRACE("device %p, format %#x, multisample_type %#x, bind_flags %#x, "
            "access %#x, width %u, height %u, surface %p.\n",
            device, format, multisample_type, bind_flags, access, width, height, surface);

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = format;
    desc.multisample_type = multisample_type;
    desc.multisample_quality = 0;
    desc.usage = 0;
    desc.bind_flags = bind_flags;
    desc.access = access;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.size = 0;

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &desc,
            1, 1, 0, NULL, NULL, &d3d8_null_wined3d_parent_ops, &texture)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to create texture, hr %#lx.\n", hr);
        return hr;
    }

    if (!(surface_impl = d3d8_surface_create(texture, 0, NULL)))
    {
        wined3d_texture_decref(texture);
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }
    surface_impl->parent_device = &device->IDirect3DDevice8_iface;
    *surface = &surface_impl->IDirect3DSurface8_iface;
    IDirect3DSurface8_AddRef(*surface);
    wined3d_texture_decref(texture);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_CreateRenderTarget(IDirect3DDevice8 *iface, UINT width,
        UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type, BOOL lockable,
        IDirect3DSurface8 **surface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    unsigned int access = WINED3D_RESOURCE_ACCESS_GPU;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, lockable %#x, surface %p.\n",
            iface, width, height, format, multisample_type, lockable, surface);

    if (!format)
        return D3DERR_INVALIDCALL;

    *surface = NULL;
    if (lockable)
        access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;

    return d3d8_device_create_surface(device, wined3dformat_from_d3dformat(format),
            wined3d_multisample_type_from_d3d(multisample_type),
            WINED3D_BIND_RENDER_TARGET, access, width, height, surface);
}

static HRESULT WINAPI d3d8_device_CreateDepthStencilSurface(IDirect3DDevice8 *iface,
        UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type,
        IDirect3DSurface8 **surface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, surface %p.\n",
            iface, width, height, format, multisample_type, surface);

    if (!format)
        return D3DERR_INVALIDCALL;

    *surface = NULL;

    return d3d8_device_create_surface(device, wined3dformat_from_d3dformat(format),
            wined3d_multisample_type_from_d3d(multisample_type), WINED3D_BIND_DEPTH_STENCIL,
            WINED3D_RESOURCE_ACCESS_GPU, width, height, surface);
}

/*  IDirect3DDevice8Impl::CreateImageSurface returns surface with pool type SYSTEMMEM */
static HRESULT WINAPI d3d8_device_CreateImageSurface(IDirect3DDevice8 *iface, UINT width,
        UINT height, D3DFORMAT format, IDirect3DSurface8 **surface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    unsigned int access = WINED3D_RESOURCE_ACCESS_CPU
            | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;

    TRACE("iface %p, width %u, height %u, format %#x, surface %p.\n",
            iface, width, height, format, surface);

    *surface = NULL;

    return d3d8_device_create_surface(device, wined3dformat_from_d3dformat(format),
            WINED3D_MULTISAMPLE_NONE, 0, access, width, height, surface);
}

static HRESULT WINAPI d3d8_device_CopyRects(IDirect3DDevice8 *iface,
        IDirect3DSurface8 *src_surface, const RECT *src_rects, UINT rect_count,
        IDirect3DSurface8 *dst_surface, const POINT *dst_points)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_surface *src = unsafe_impl_from_IDirect3DSurface8(src_surface);
    struct d3d8_surface *dst = unsafe_impl_from_IDirect3DSurface8(dst_surface);
    enum wined3d_format_id src_format, dst_format;
    struct wined3d_sub_resource_desc wined3d_desc;
    UINT src_w, src_h;

    TRACE("iface %p, src_surface %p, src_rects %p, rect_count %u, dst_surface %p, dst_points %p.\n",
            iface, src_surface, src_rects, rect_count, dst_surface, dst_points);

    wined3d_mutex_lock();
    wined3d_texture_get_sub_resource_desc(src->wined3d_texture, src->sub_resource_idx, &wined3d_desc);
    if (wined3d_desc.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        WARN("Source %p is a depth stencil surface, returning D3DERR_INVALIDCALL.\n", src_surface);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }
    src_format = wined3d_desc.format;
    src_w = wined3d_desc.width;
    src_h = wined3d_desc.height;

    wined3d_texture_get_sub_resource_desc(dst->wined3d_texture, dst->sub_resource_idx, &wined3d_desc);
    if (wined3d_desc.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        WARN("Destination %p is a depth stencil surface, returning D3DERR_INVALIDCALL.\n", dst_surface);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }
    dst_format = wined3d_desc.format;

    /* Check that the source and destination formats match */
    if (src_format != dst_format)
    {
        WARN("Source %p format must match the destination %p format, returning D3DERR_INVALIDCALL.\n",
                src_surface, dst_surface);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    /* Quick if complete copy ... */
    if (!rect_count && !src_rects && !dst_points)
    {
        RECT rect = {0, 0, src_w, src_h};
        wined3d_device_context_blt(device->immediate_context, dst->wined3d_texture, dst->sub_resource_idx, &rect,
                src->wined3d_texture, src->sub_resource_idx, &rect, 0, NULL, WINED3D_TEXF_POINT);
    }
    else
    {
        unsigned int i;
        /* Copy rect by rect */
        if (src_rects && dst_points)
        {
            for (i = 0; i < rect_count; ++i)
            {
                UINT w = src_rects[i].right - src_rects[i].left;
                UINT h = src_rects[i].bottom - src_rects[i].top;
                RECT dst_rect = {dst_points[i].x, dst_points[i].y,
                        dst_points[i].x + w, dst_points[i].y + h};

                wined3d_device_context_blt(device->immediate_context,
                        dst->wined3d_texture, dst->sub_resource_idx, &dst_rect,
                        src->wined3d_texture, src->sub_resource_idx, &src_rects[i], 0, NULL, WINED3D_TEXF_POINT);
            }
        }
        else
        {
            for (i = 0; i < rect_count; ++i)
            {
                UINT w = src_rects[i].right - src_rects[i].left;
                UINT h = src_rects[i].bottom - src_rects[i].top;
                RECT dst_rect = {0, 0, w, h};

                wined3d_device_context_blt(device->immediate_context,
                        dst->wined3d_texture, dst->sub_resource_idx, &dst_rect,
                        src->wined3d_texture, src->sub_resource_idx, &src_rects[i], 0, NULL, WINED3D_TEXF_POINT);
            }
        }
    }
    wined3d_mutex_unlock();

    return WINED3D_OK;
}

static HRESULT WINAPI d3d8_device_UpdateTexture(IDirect3DDevice8 *iface,
        IDirect3DBaseTexture8 *src_texture, IDirect3DBaseTexture8 *dst_texture)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_texture *src_impl, *dst_impl;
    HRESULT hr;

    TRACE("iface %p, src_texture %p, dst_texture %p.\n", iface, src_texture, dst_texture);

    src_impl = unsafe_impl_from_IDirect3DBaseTexture8(src_texture);
    dst_impl = unsafe_impl_from_IDirect3DBaseTexture8(dst_texture);

    if (src_impl->draw_texture || dst_impl->draw_texture)
    {
        WARN("Source or destination is managed; returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_update_texture(device->wined3d_device,
            src_impl->wined3d_texture, dst_impl->wined3d_texture);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetFrontBuffer(IDirect3DDevice8 *iface, IDirect3DSurface8 *dst_surface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_surface *dst_impl = unsafe_impl_from_IDirect3DSurface8(dst_surface);
    HRESULT hr;

    TRACE("iface %p, dst_surface %p.\n", iface, dst_surface);

    if (!dst_surface)
    {
        WARN("Invalid destination surface passed.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_front_buffer_data(device->implicit_swapchain,
            dst_impl->wined3d_texture, dst_impl->sub_resource_idx);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_SetRenderTarget(IDirect3DDevice8 *iface,
        IDirect3DSurface8 *render_target, IDirect3DSurface8 *depth_stencil)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_surface *rt_impl = unsafe_impl_from_IDirect3DSurface8(render_target);
    struct d3d8_surface *ds_impl = unsafe_impl_from_IDirect3DSurface8(depth_stencil);
    struct wined3d_rendertarget_view *original_dsv, *rtv;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, render_target %p, depth_stencil %p.\n", iface, render_target, depth_stencil);

    if (rt_impl && d3d8_surface_get_device(rt_impl) != device)
    {
        WARN("Render target surface does not match device.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();

    if (ds_impl)
    {
        struct wined3d_sub_resource_desc ds_desc, rt_desc;
        struct wined3d_rendertarget_view *original_rtv;
        struct d3d8_surface *original_surface;

        /* If no render target is passed in check the size against the current RT */
        if (!render_target)
        {
            if (!(original_rtv = wined3d_device_context_get_rendertarget_view(device->immediate_context, 0)))
            {
                wined3d_mutex_unlock();
                return D3DERR_NOTFOUND;
            }
            original_surface = wined3d_rendertarget_view_get_sub_resource_parent(original_rtv);
            wined3d_texture_get_sub_resource_desc(original_surface->wined3d_texture,
                    original_surface->sub_resource_idx, &rt_desc);
        }
        else
            wined3d_texture_get_sub_resource_desc(rt_impl->wined3d_texture,
                    rt_impl->sub_resource_idx, &rt_desc);

        wined3d_texture_get_sub_resource_desc(ds_impl->wined3d_texture, ds_impl->sub_resource_idx, &ds_desc);

        if (ds_desc.width < rt_desc.width || ds_desc.height < rt_desc.height)
        {
            WARN("Depth stencil is smaller than the render target, returning D3DERR_INVALIDCALL\n");
            wined3d_mutex_unlock();
            return D3DERR_INVALIDCALL;
        }
        if (ds_desc.multisample_type != rt_desc.multisample_type
                || ds_desc.multisample_quality != rt_desc.multisample_quality)
        {
            WARN("Multisample settings do not match, returning D3DERR_INVALIDCALL\n");
            wined3d_mutex_unlock();
            return D3DERR_INVALIDCALL;
        }
    }

    original_dsv = wined3d_device_context_get_depth_stencil_view(device->immediate_context);
    rtv = ds_impl ? d3d8_surface_acquire_rendertarget_view(ds_impl) : NULL;
    hr = wined3d_device_context_set_depth_stencil_view(device->immediate_context, rtv);
    d3d8_surface_release_rendertarget_view(ds_impl, rtv);
    if (SUCCEEDED(hr))
    {
        rtv = render_target ? d3d8_surface_acquire_rendertarget_view(rt_impl) : NULL;
        if (render_target)
        {
            if (SUCCEEDED(hr = wined3d_device_context_set_rendertarget_views(device->immediate_context,
                    0, 1, &rtv, TRUE)))
                device_reset_viewport_state(device);
            else
                wined3d_device_context_set_depth_stencil_view(device->immediate_context, original_dsv);
        }
        d3d8_surface_release_rendertarget_view(rt_impl, rtv);
        wined3d_stateblock_depth_buffer_changed(device->state);
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetRenderTarget(IDirect3DDevice8 *iface, IDirect3DSurface8 **render_target)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_rendertarget_view *wined3d_rtv;
    struct d3d8_surface *surface_impl;
    HRESULT hr;

    TRACE("iface %p, render_target %p.\n", iface, render_target);

    if (!render_target)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    if ((wined3d_rtv = wined3d_device_context_get_rendertarget_view(device->immediate_context, 0)))
    {
        /* We want the sub resource parent here, since the view itself may be
         * internal to wined3d and may not have a parent. */
        surface_impl = wined3d_rendertarget_view_get_sub_resource_parent(wined3d_rtv);
        *render_target = &surface_impl->IDirect3DSurface8_iface;
        IDirect3DSurface8_AddRef(*render_target);
        hr = D3D_OK;
    }
    else
    {
        ERR("Failed to get wined3d render target.\n");
        *render_target = NULL;
        hr = D3DERR_NOTFOUND;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetDepthStencilSurface(IDirect3DDevice8 *iface, IDirect3DSurface8 **depth_stencil)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_rendertarget_view *wined3d_dsv;
    struct d3d8_surface *surface_impl;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, depth_stencil %p.\n", iface, depth_stencil);

    if (!depth_stencil)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    if ((wined3d_dsv = wined3d_device_context_get_depth_stencil_view(device->immediate_context)))
    {
        /* We want the sub resource parent here, since the view itself may be
         * internal to wined3d and may not have a parent. */
        surface_impl = wined3d_rendertarget_view_get_sub_resource_parent(wined3d_dsv);
        *depth_stencil = &surface_impl->IDirect3DSurface8_iface;
        IDirect3DSurface8_AddRef(*depth_stencil);
    }
    else
    {
        hr = D3DERR_NOTFOUND;
        *depth_stencil = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_BeginScene(IDirect3DDevice8 *iface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_begin_scene(device->wined3d_device);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d8_device_EndScene(IDirect3DDevice8 *iface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_end_scene(device->wined3d_device);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_Clear(IDirect3DDevice8 *iface, DWORD rect_count,
        const D3DRECT *rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
    const struct wined3d_color c =
    {
        ((color >> 16) & 0xff) / 255.0f,
        ((color >>  8) & 0xff) / 255.0f,
        (color & 0xff) / 255.0f,
        ((color >> 24) & 0xff) / 255.0f,
    };
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, rect_count %lu, rects %p, flags %#lx, color 0x%08lx, z %.8e, stencil %lu.\n",
            iface, rect_count, rects, flags, color, z, stencil);

    if (rect_count && !rects)
    {
        WARN("count %lu with NULL rects.\n", rect_count);
        rect_count = 0;
    }

    wined3d_mutex_lock();
    wined3d_stateblock_apply_clear_state(device->state, device->wined3d_device);
    hr = wined3d_device_clear(device->wined3d_device, rect_count, (const RECT *)rects, flags, &c, z, stencil);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_SetTransform(IDirect3DDevice8 *iface,
        D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    wined3d_stateblock_set_transform(device->update_state,
            wined3d_transform_state_from_d3d(state), (const struct wined3d_matrix *)matrix);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetTransform(IDirect3DDevice8 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    memcpy(matrix, &device->stateblock_state->transforms[state], sizeof(*matrix));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_MultiplyTransform(IDirect3DDevice8 *iface,
        D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    wined3d_stateblock_multiply_transform(device->state,
            wined3d_transform_state_from_d3d(state), (const struct wined3d_matrix *)matrix);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetViewport(IDirect3DDevice8 *iface, const D3DVIEWPORT8 *viewport)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_sub_resource_desc rt_desc;
    struct wined3d_rendertarget_view *rtv;
    struct d3d8_surface *surface;
    struct wined3d_viewport vp;

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    wined3d_mutex_lock();
    if (!(rtv = wined3d_device_context_get_rendertarget_view(device->immediate_context, 0)))
    {
        wined3d_mutex_unlock();
        return D3DERR_NOTFOUND;
    }
    surface = wined3d_rendertarget_view_get_sub_resource_parent(rtv);
    wined3d_texture_get_sub_resource_desc(surface->wined3d_texture, surface->sub_resource_idx, &rt_desc);

    if (viewport->X > rt_desc.width || viewport->Width > rt_desc.width - viewport->X
            || viewport->Y > rt_desc.height || viewport->Height > rt_desc.height - viewport->Y)
    {
        WARN("Invalid viewport, returning D3DERR_INVALIDCALL.\n");
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    vp.x = viewport->X;
    vp.y = viewport->Y;
    vp.width = viewport->Width;
    vp.height = viewport->Height;
    vp.min_z = viewport->MinZ;
    vp.max_z = viewport->MaxZ;

    wined3d_stateblock_set_viewport(device->update_state, &vp);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetViewport(IDirect3DDevice8 *iface, D3DVIEWPORT8 *viewport)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_viewport wined3d_viewport;

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    wined3d_mutex_lock();
    wined3d_viewport = device->stateblock_state->viewport;
    wined3d_mutex_unlock();

    viewport->X = wined3d_viewport.x;
    viewport->Y = wined3d_viewport.y;
    viewport->Width = wined3d_viewport.width;
    viewport->Height = wined3d_viewport.height;
    viewport->MinZ = wined3d_viewport.min_z;
    viewport->MaxZ = wined3d_viewport.max_z;

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetMaterial(IDirect3DDevice8 *iface, const D3DMATERIAL8 *material)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    /* Note: D3DMATERIAL8 is compatible with struct wined3d_material. */
    wined3d_mutex_lock();
    wined3d_stateblock_set_material(device->update_state, (const struct wined3d_material *)material);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetMaterial(IDirect3DDevice8 *iface, D3DMATERIAL8 *material)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    /* Note: D3DMATERIAL8 is compatible with struct wined3d_material. */
    wined3d_mutex_lock();
    memcpy(material, &device->stateblock_state->material, sizeof(*material));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetLight(IDirect3DDevice8 *iface, DWORD index, const D3DLIGHT8 *light)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, light %p.\n", iface, index, light);

    /* Note: D3DLIGHT8 is compatible with struct wined3d_light. */
    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_light(device->update_state, index, (const struct wined3d_light *)light);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetLight(IDirect3DDevice8 *iface, DWORD index, D3DLIGHT8 *light)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    BOOL enabled;
    HRESULT hr;

    TRACE("iface %p, index %lu, light %p.\n", iface, index, light);

    /* Note: D3DLIGHT8 is compatible with struct wined3d_light. */
    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_light(device->state, index, (struct wined3d_light *)light, &enabled);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_LightEnable(IDirect3DDevice8 *iface, DWORD index, BOOL enable)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, enable %#x.\n", iface, index, enable);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_light_enable(device->update_state, index, enable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetLightEnable(IDirect3DDevice8 *iface, DWORD index, BOOL *enabled)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_light light;
    HRESULT hr;

    TRACE("iface %p, index %lu, enabled %p.\n", iface, index, enabled);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_light(device->state, index, &light, enabled);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_SetClipPlane(IDirect3DDevice8 *iface, DWORD index, const float *plane)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, plane %p.\n", iface, index, plane);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_clip_plane(device->update_state, index, (const struct wined3d_vec4 *)plane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetClipPlane(IDirect3DDevice8 *iface, DWORD index, float *plane)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, index %lu, plane %p.\n", iface, index, plane);

    if (index >= device->max_user_clip_planes)
    {
        TRACE("Plane %lu requested, but only %u planes are supported.\n", index, device->max_user_clip_planes);
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    memcpy(plane, &device->stateblock_state->clip_planes[index], sizeof(struct wined3d_vec4));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static void resolve_depth_buffer(struct d3d8_device *device)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    struct wined3d_rendertarget_view *wined3d_dsv;
    struct wined3d_resource *dst_resource;
    struct wined3d_texture *dst_texture;
    struct wined3d_resource_desc desc;
    struct d3d8_surface *d3d8_dsv;

    if (!(dst_texture = state->textures[0]))
        return;
    dst_resource = wined3d_texture_get_resource(dst_texture);
    wined3d_resource_get_desc(dst_resource, &desc);
    if (desc.format != WINED3DFMT_D24_UNORM_S8_UINT
            && desc.format != WINED3DFMT_X8D24_UNORM
            && desc.format != WINED3DFMT_DF16
            && desc.format != WINED3DFMT_DF24
            && desc.format != WINED3DFMT_INTZ)
        return;

    if (!(wined3d_dsv = wined3d_device_context_get_depth_stencil_view(device->immediate_context)))
        return;
    d3d8_dsv = wined3d_rendertarget_view_get_sub_resource_parent(wined3d_dsv);

    wined3d_device_context_resolve_sub_resource(device->immediate_context, dst_resource, 0,
            wined3d_rendertarget_view_get_resource(wined3d_dsv), d3d8_dsv->sub_resource_idx, desc.format);
}

static HRESULT WINAPI d3d8_device_SetRenderState(IDirect3DDevice8 *iface,
        D3DRENDERSTATETYPE state, DWORD value)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, state %#x, value %#lx.\n", iface, state, value);

    wined3d_mutex_lock();
    wined3d_stateblock_set_render_state(device->update_state, wined3d_render_state_from_d3d(state), value);
    if (state == D3DRS_POINTSIZE && value == D3D8_RESZ_CODE)
        resolve_depth_buffer(device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetRenderState(IDirect3DDevice8 *iface,
        D3DRENDERSTATETYPE state, DWORD *value)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    const struct wined3d_stateblock_state *device_state;

    TRACE("iface %p, state %#x, value %p.\n", iface, state, value);

    wined3d_mutex_lock();
    device_state = device->stateblock_state;
    switch (state)
    {
        case D3DRS_ZBIAS:
            *value = device_state->rs[WINED3D_RS_DEPTHBIAS];
            break;

        default:
            *value = device_state->rs[state];
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_BeginStateBlock(IDirect3DDevice8 *iface)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to begin a stateblock while recording, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    if (SUCCEEDED(hr = wined3d_stateblock_create(device->wined3d_device, NULL, WINED3D_SBT_RECORDED, &stateblock)))
        device->update_state = device->recording = stateblock;
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_EndStateBlock(IDirect3DDevice8 *iface, DWORD *token)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;

    TRACE("iface %p, token %p.\n", iface, token);

    /* Tell wineD3D to endstateblock before anything else (in case we run out
     * of memory later and cause locking problems)
     */
    wined3d_mutex_lock();
    if (!device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to end a stateblock, but no stateblock is being recorded.\n");
        return D3DERR_INVALIDCALL;
    }
    stateblock = device->recording;
    wined3d_stateblock_init_contained_states(stateblock);
    device->recording = NULL;
    device->update_state = device->state;

    *token = d3d8_allocate_handle(&device->handle_table, stateblock, D3D8_HANDLE_SB);
    wined3d_mutex_unlock();

    if (*token == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to create a handle\n");
        wined3d_mutex_lock();
        wined3d_stateblock_decref(stateblock);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    ++*token;

    TRACE("Returning %#lx (%p).\n", *token, stateblock);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_ApplyStateBlock(IDirect3DDevice8 *iface, DWORD token)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    const struct wined3d_stateblock_state *state;
    struct d3d8_vertexbuffer *vertex_buffer;
    struct wined3d_stateblock *stateblock;
    struct d3d8_indexbuffer *index_buffer;
    struct wined3d_buffer *wined3d_buffer;
    unsigned int i;

    TRACE("iface %p, token %#lx.\n", iface, token);

    if (!token)
        return D3D_OK;

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to apply stateblock while recording, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    stateblock = d3d8_get_object(&device->handle_table, token - 1, D3D8_HANDLE_SB);
    if (!stateblock)
    {
        WARN("Invalid handle (%#lx) passed.\n", token);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }
    wined3d_stateblock_apply(stateblock, device->state);
    device->sysmem_vb = 0;
    state = device->stateblock_state;
    for (i = 0; i < D3D8_MAX_STREAMS; ++i)
    {
        wined3d_buffer = state->streams[i].buffer;
        if (!wined3d_buffer || !(vertex_buffer = wined3d_buffer_get_parent(wined3d_buffer)))
            continue;
        if (vertex_buffer->draw_buffer)
            device->sysmem_vb |= 1u << i;
    }
    device->sysmem_ib = (wined3d_buffer = state->index_buffer)
            && (index_buffer = wined3d_buffer_get_parent(wined3d_buffer)) && index_buffer->sysmem;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_CaptureStateBlock(IDirect3DDevice8 *iface, DWORD token)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;

    TRACE("iface %p, token %#lx.\n", iface, token);

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to capture stateblock while recording, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    stateblock = d3d8_get_object(&device->handle_table, token - 1, D3D8_HANDLE_SB);
    if (!stateblock)
    {
        WARN("Invalid handle (%#lx) passed.\n", token);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }
    wined3d_stateblock_capture(stateblock, device->state);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_DeleteStateBlock(IDirect3DDevice8 *iface, DWORD token)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;

    TRACE("iface %p, token %#lx.\n", iface, token);

    wined3d_mutex_lock();
    stateblock = d3d8_free_handle(&device->handle_table, token - 1, D3D8_HANDLE_SB);

    if (!stateblock)
    {
        WARN("Invalid handle (%#lx) passed.\n", token);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    if (wined3d_stateblock_decref(stateblock))
    {
        ERR("Stateblock %p has references left, this shouldn't happen.\n", stateblock);
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_CreateStateBlock(IDirect3DDevice8 *iface,
        D3DSTATEBLOCKTYPE type, DWORD *handle)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("iface %p, type %#x, handle %p.\n", iface, type, handle);

    if (type != D3DSBT_ALL
            && type != D3DSBT_PIXELSTATE
            && type != D3DSBT_VERTEXSTATE)
    {
        WARN("Unexpected stateblock type, returning D3DERR_INVALIDCALL\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to create a stateblock while recording, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    hr = wined3d_stateblock_create(device->wined3d_device, device->state, (enum wined3d_stateblock_type)type, &stateblock);
    if (FAILED(hr))
    {
        wined3d_mutex_unlock();
        ERR("Failed to create the state block, hr %#lx.\n", hr);
        return hr;
    }

    *handle = d3d8_allocate_handle(&device->handle_table, stateblock, D3D8_HANDLE_SB);
    wined3d_mutex_unlock();

    if (*handle == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to allocate a handle.\n");
        wined3d_mutex_lock();
        wined3d_stateblock_decref(stateblock);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    ++*handle;

    TRACE("Returning %#lx (%p).\n", *handle, stateblock);

    return hr;
}

static HRESULT WINAPI d3d8_device_SetClipStatus(IDirect3DDevice8 *iface, const D3DCLIPSTATUS8 *clip_status)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);
    /* FIXME: Verify that D3DCLIPSTATUS8 ~= struct wined3d_clip_status. */

    wined3d_mutex_lock();
    hr = wined3d_device_set_clip_status(device->wined3d_device, (const struct wined3d_clip_status *)clip_status);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetClipStatus(IDirect3DDevice8 *iface, D3DCLIPSTATUS8 *clip_status)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);

    wined3d_mutex_lock();
    hr = wined3d_device_get_clip_status(device->wined3d_device, (struct wined3d_clip_status *)clip_status);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetTexture(IDirect3DDevice8 *iface, DWORD stage, IDirect3DBaseTexture8 **texture)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_texture *wined3d_texture;
    struct d3d8_texture *texture_impl;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, texture);

    if (!texture)
        return D3DERR_INVALIDCALL;

    if (stage >= WINED3D_MAX_FRAGMENT_SAMPLERS)
    {
        WARN("Ignoring invalid stage %lu.\n", stage);
        *texture = NULL;
        return D3D_OK;
    }

    wined3d_mutex_lock();
    if ((wined3d_texture = device->stateblock_state->textures[stage]))
    {
        texture_impl = wined3d_texture_get_parent(wined3d_texture);
        *texture = &texture_impl->IDirect3DBaseTexture8_iface;
        IDirect3DBaseTexture8_AddRef(*texture);
    }
    else
    {
        *texture = NULL;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetTexture(IDirect3DDevice8 *iface, DWORD stage, IDirect3DBaseTexture8 *texture)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_texture *texture_impl;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, texture);

    texture_impl = unsafe_impl_from_IDirect3DBaseTexture8(texture);

    wined3d_mutex_lock();
    wined3d_stateblock_set_texture(device->update_state, stage,
            texture_impl ? d3d8_texture_get_draw_texture(texture_impl) : NULL);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static const struct tss_lookup
{
    BOOL sampler_state;
    union
    {
        enum wined3d_texture_stage_state texture_state;
        enum wined3d_sampler_state sampler_state;
    } u;
}
tss_lookup[] =
{
    {FALSE, .u.texture_state = WINED3D_TSS_INVALID},                   /*  0, unused */
    {FALSE, .u.texture_state = WINED3D_TSS_COLOR_OP},                  /*  1, D3DTSS_COLOROP */
    {FALSE, .u.texture_state = WINED3D_TSS_COLOR_ARG1},                /*  2, D3DTSS_COLORARG1 */
    {FALSE, .u.texture_state = WINED3D_TSS_COLOR_ARG2},                /*  3, D3DTSS_COLORARG2 */
    {FALSE, .u.texture_state = WINED3D_TSS_ALPHA_OP},                  /*  4, D3DTSS_ALPHAOP */
    {FALSE, .u.texture_state = WINED3D_TSS_ALPHA_ARG1},                /*  5, D3DTSS_ALPHAARG1 */
    {FALSE, .u.texture_state = WINED3D_TSS_ALPHA_ARG2},                /*  6, D3DTSS_ALPHAARG2 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT00},             /*  7, D3DTSS_BUMPENVMAT00 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT01},             /*  8, D3DTSS_BUMPENVMAT01 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT10},             /*  9, D3DTSS_BUMPENVMAT10 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT11},             /* 10, D3DTSS_BUMPENVMAT11 */
    {FALSE, .u.texture_state = WINED3D_TSS_TEXCOORD_INDEX},            /* 11, D3DTSS_TEXCOORDINDEX */
    {FALSE, .u.texture_state = WINED3D_TSS_INVALID},                   /* 12, unused */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_ADDRESS_U},                /* 13, D3DTSS_ADDRESSU */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_ADDRESS_V},                /* 14, D3DTSS_ADDRESSV */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_BORDER_COLOR},             /* 15, D3DTSS_BORDERCOLOR */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MAG_FILTER},               /* 16, D3DTSS_MAGFILTER */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MIN_FILTER},               /* 17, D3DTSS_MINFILTER */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MIP_FILTER},               /* 18, D3DTSS_MIPFILTER */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MIPMAP_LOD_BIAS},          /* 19, D3DTSS_MIPMAPLODBIAS */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MAX_MIP_LEVEL},            /* 20, D3DTSS_MAXMIPLEVEL */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MAX_ANISOTROPY},           /* 21, D3DTSS_MAXANISOTROPY */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_LSCALE},            /* 22, D3DTSS_BUMPENVLSCALE */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_LOFFSET},           /* 23, D3DTSS_BUMPENVLOFFSET */
    {FALSE, .u.texture_state = WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS},   /* 24, D3DTSS_TEXTURETRANSFORMFLAGS */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_ADDRESS_W},                /* 25, D3DTSS_ADDRESSW */
    {FALSE, .u.texture_state = WINED3D_TSS_COLOR_ARG0},                /* 26, D3DTSS_COLORARG0 */
    {FALSE, .u.texture_state = WINED3D_TSS_ALPHA_ARG0},                /* 27, D3DTSS_ALPHAARG0 */
    {FALSE, .u.texture_state = WINED3D_TSS_RESULT_ARG},                /* 28, D3DTSS_RESULTARG */
};

static HRESULT WINAPI d3d8_device_GetTextureStageState(IDirect3DDevice8 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD *value)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    const struct wined3d_stateblock_state *device_state;
    const struct tss_lookup *l;

    TRACE("iface %p, stage %lu, state %#x, value %p.\n", iface, stage, state, value);

    if (state >= ARRAY_SIZE(tss_lookup))
    {
        WARN("Invalid state %#x.\n", state);
        return D3D_OK;
    }

    l = &tss_lookup[state];

    wined3d_mutex_lock();
    device_state = device->stateblock_state;
    if (l->sampler_state)
        *value = device_state->sampler_states[stage][l->u.sampler_state];
    else
        *value = device_state->texture_states[stage][l->u.texture_state];
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetTextureStageState(IDirect3DDevice8 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    const struct tss_lookup *l;

    TRACE("iface %p, stage %lu, state %#x, value %#lx.\n", iface, stage, type, value);

    if (type >= ARRAY_SIZE(tss_lookup))
    {
        WARN("Invalid type %#x passed.\n", type);
        return D3D_OK;
    }

    l = &tss_lookup[type];

    wined3d_mutex_lock();
    if (l->sampler_state)
        wined3d_stateblock_set_sampler_state(device->update_state, stage, l->u.sampler_state, value);
    else
        wined3d_stateblock_set_texture_stage_state(device->update_state, stage, l->u.texture_state, value);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_ValidateDevice(IDirect3DDevice8 *iface, DWORD *pass_count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, pass_count %p.\n", iface, pass_count);

    wined3d_mutex_lock();
    hr = wined3d_device_validate_device(device->wined3d_device, device->stateblock_state, pass_count);
    wined3d_mutex_unlock();

    /* In d3d8, texture filters are not validated, so errors concerning
     * unsupported ones are ignored here. */
    if (hr == WINED3DERR_UNSUPPORTEDTEXTUREFILTER)
    {
        WARN("Ignoring invalid texture filter settings.\n");
        *pass_count = 1;
        return D3D_OK;
    }

    return hr;
}

static HRESULT WINAPI d3d8_device_GetInfo(IDirect3DDevice8 *iface,
        DWORD info_id, void *info, DWORD info_size)
{
    TRACE("iface %p, info_id %#lx, info %p, info_size %lu.\n", iface, info_id, info, info_size);

    if (info_id < 4)
        return E_FAIL;
    return S_FALSE;
}

static HRESULT WINAPI d3d8_device_SetPaletteEntries(IDirect3DDevice8 *iface,
        UINT palette_idx, const PALETTEENTRY *entries)
{
    WARN("iface %p, palette_idx %u, entries %p unimplemented\n", iface, palette_idx, entries);

    /* GPUs stopped supporting palettized textures with the Shader Model 1 generation. Wined3d
     * does not have a d3d8/9-style palette API */

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetPaletteEntries(IDirect3DDevice8 *iface,
        UINT palette_idx, PALETTEENTRY *entries)
{
    FIXME("iface %p, palette_idx %u, entries %p unimplemented.\n", iface, palette_idx, entries);

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI d3d8_device_SetCurrentTexturePalette(IDirect3DDevice8 *iface, UINT palette_idx)
{
    WARN("iface %p, palette_idx %u unimplemented.\n", iface, palette_idx);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetCurrentTexturePalette(IDirect3DDevice8 *iface, UINT *palette_idx)
{
    FIXME("iface %p, palette_idx %p unimplemented.\n", iface, palette_idx);

    return D3DERR_INVALIDCALL;
}

static void d3d8_device_upload_sysmem_vertex_buffers(struct d3d8_device *device,
        unsigned int start_vertex, unsigned int vertex_count)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    struct wined3d_vertex_declaration *wined3d_decl;
    struct wined3d_box box = {0, 0, 0, 1, 0, 1};
    const struct wined3d_stream_state *stream;
    struct d3d8_vertexbuffer *d3d8_buffer;
    struct wined3d_resource *dst_resource;
    struct d3d8_vertex_declaration *decl;
    struct wined3d_buffer *dst_buffer;
    struct wined3d_resource_desc desc;
    unsigned int stride, map;
    HRESULT hr;

    if (!device->sysmem_vb)
        return;

    if (!(wined3d_decl = state->vertex_declaration))
        return;

    decl = wined3d_vertex_declaration_get_parent(wined3d_decl);
    map = decl->stream_map & device->sysmem_vb;
    while (map)
    {
        stream = &state->streams[wined3d_bit_scan(&map)];
        dst_buffer = stream->buffer;
        stride = stream->stride;

        d3d8_buffer = wined3d_buffer_get_parent(dst_buffer);
        dst_resource = wined3d_buffer_get_resource(dst_buffer);
        wined3d_resource_get_desc(dst_resource, &desc);
        box.left = stream->offset + start_vertex * stride;
        box.right = min(box.left + vertex_count * stride, desc.size);
        if (FAILED(hr = wined3d_device_context_copy_sub_resource_region(device->immediate_context,
                dst_resource, 0, box.left, 0, 0,
                wined3d_buffer_get_resource(d3d8_buffer->wined3d_buffer), 0, &box, 0)))
            ERR("Failed to update buffer.\n");
    }
}

static HRESULT d3d8_device_upload_sysmem_index_buffer(struct d3d8_device *device,
        unsigned int *start_idx, unsigned int idx_count)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    unsigned int src_offset, idx_size, buffer_size, pos;
    struct wined3d_resource_desc resource_desc;
    struct wined3d_resource *index_buffer;
    struct wined3d_map_desc map_desc;
    struct wined3d_box box;
    HRESULT hr;

    if (!device->sysmem_ib)
        return S_OK;

    index_buffer = wined3d_buffer_get_resource(state->index_buffer);
    wined3d_resource_get_desc(index_buffer, &resource_desc);
    idx_size = (state->index_format == WINED3DFMT_R16_UINT) ? 2 : 4;

    src_offset = (*start_idx) * idx_size;
    buffer_size = min(idx_count * idx_size, resource_desc.size - src_offset);

    wined3d_box_set(&box, src_offset, 0, buffer_size, 1, 0, 1);
    if (FAILED(hr = wined3d_resource_map(index_buffer, 0, &map_desc, &box, WINED3D_MAP_READ)))
    {
        ERR("Failed to map index buffer, hr %#lx.\n", hr);
        return hr;
    }
    wined3d_streaming_buffer_upload(device->wined3d_device, &device->index_buffer,
            map_desc.data, buffer_size, idx_size, &pos);
    wined3d_resource_unmap(index_buffer, 0);

    wined3d_device_context_set_index_buffer(device->immediate_context,
            device->index_buffer.buffer, state->index_format, pos);
    *start_idx = 0;
    return S_OK;
}

static void d3d8_device_upload_managed_textures(struct d3d8_device *device)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    unsigned int i;

    for (i = 0; i < WINED3D_MAX_FRAGMENT_SAMPLERS; ++i)
    {
        struct wined3d_texture *wined3d_texture = state->textures[i];
        struct d3d8_texture *d3d8_texture;

        if (!wined3d_texture)
            continue;
        d3d8_texture = wined3d_texture_get_parent(wined3d_texture);
        if (d3d8_texture->draw_texture)
            wined3d_device_update_texture(device->wined3d_device,
                    d3d8_texture->wined3d_texture, d3d8_texture->draw_texture);
    }
}

static HRESULT WINAPI d3d8_device_DrawPrimitive(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE primitive_type, UINT start_vertex, UINT primitive_count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertexbuffer *vb;
    unsigned int vertex_count, i;

    TRACE("iface %p, primitive_type %#x, start_vertex %u, primitive_count %u.\n",
            iface, primitive_type, start_vertex, primitive_count);

    vertex_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    wined3d_mutex_lock();
    d3d8_device_upload_managed_textures(device);
    d3d8_device_upload_sysmem_vertex_buffers(device, start_vertex, vertex_count);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    wined3d_device_context_draw(device->immediate_context, start_vertex, vertex_count, 0, 0);

    for (i = 0; i < ARRAY_SIZE(device->stateblock_state->streams); ++i)
    {
        if (device->stateblock_state->streams[i].buffer)
        {
            vb = wined3d_buffer_get_parent(device->stateblock_state->streams[i].buffer);
            vb->discarded = false;
        }
    }

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_DrawIndexedPrimitive(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE primitive_type, UINT min_vertex_idx, UINT vertex_count,
        UINT start_idx, UINT primitive_count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertexbuffer *vb;
    struct d3d8_indexbuffer *ib;
    unsigned int index_count, i;
    int base_vertex_index;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, min_vertex_idx %u, vertex_count %u, start_idx %u, primitive_count %u.\n",
            iface, primitive_type, min_vertex_idx, vertex_count, start_idx, primitive_count);

    index_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    wined3d_mutex_lock();
    if (!device->stateblock_state->index_buffer)
    {
        wined3d_mutex_unlock();
        WARN("Index buffer not set, returning D3D_OK.\n");
        return D3D_OK;
    }
    base_vertex_index = device->stateblock_state->base_vertex_index;
    d3d8_device_upload_managed_textures(device);
    d3d8_device_upload_sysmem_vertex_buffers(device, base_vertex_index + min_vertex_idx, vertex_count);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    if (FAILED(hr = d3d8_device_upload_sysmem_index_buffer(device, &start_idx, index_count)))
    {
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_device_context_draw_indexed(device->immediate_context, base_vertex_index, start_idx, index_count, 0, 0);

    ib = wined3d_buffer_get_parent(device->stateblock_state->index_buffer);
    ib->discarded = false;
    for (i = 0; i < ARRAY_SIZE(device->stateblock_state->streams); ++i)
    {
        if (device->stateblock_state->streams[i].buffer)
        {
            vb = wined3d_buffer_get_parent(device->stateblock_state->streams[i].buffer);
            vb->discarded = false;
        }
    }

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_DrawPrimitiveUP(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE primitive_type, UINT primitive_count, const void *data,
        UINT stride)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    UINT vtx_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    UINT size = vtx_count * stride;
    unsigned int vb_pos;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, primitive_count %u, data %p, stride %u.\n",
            iface, primitive_type, primitive_count, data, stride);

    if (!primitive_count || !stride)
    {
        WARN("primitive_count or stride is 0, returning D3D_OK.\n");
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device,
            &device->vertex_buffer, data, size, stride, &vb_pos)))
        goto done;

    hr = wined3d_stateblock_set_stream_source(device->state, 0, device->vertex_buffer.buffer, 0, stride);
    if (FAILED(hr))
        goto done;

    d3d8_device_upload_managed_textures(device);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    wined3d_device_context_draw(device->immediate_context, vb_pos / stride, vtx_count, 0, 0);
    wined3d_stateblock_set_stream_source(device->state, 0, NULL, 0, 0);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d8_device_DrawIndexedPrimitiveUP(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE primitive_type, UINT min_vertex_idx, UINT vertex_count,
        UINT primitive_count, const void *index_data, D3DFORMAT index_format,
        const void *vertex_data, UINT vertex_stride)
{
    UINT idx_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    UINT idx_fmt_size = index_format == D3DFMT_INDEX16 ? 2 : 4;
    unsigned int base_vertex_idx, vb_pos, ib_pos;
    UINT vtx_size = vertex_count * vertex_stride;
    UINT idx_size = idx_count * idx_fmt_size;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, min_vertex_idx %u, vertex_count %u, primitive_count %u, "
            "index_data %p, index_format %#x, vertex_data %p, vertex_stride %u.\n",
            iface, primitive_type, min_vertex_idx, vertex_count, primitive_count,
            index_data, index_format, vertex_data, vertex_stride);

    if (!primitive_count || !vertex_stride)
    {
        WARN("primitive_count or vertex_stride is 0, returning D3D_OK.\n");
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device, &device->vertex_buffer,
            (char *)vertex_data + min_vertex_idx * vertex_stride, vtx_size, vertex_stride, &vb_pos)))
        goto done;

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device, &device->index_buffer,
            index_data, idx_size, idx_fmt_size, &ib_pos)))
        goto done;

    hr = wined3d_stateblock_set_stream_source(device->state, 0, device->vertex_buffer.buffer, 0, vertex_stride);
    if (FAILED(hr))
        goto done;

    wined3d_stateblock_set_index_buffer(device->state, device->index_buffer.buffer,
            wined3dformat_from_d3dformat(index_format));
    base_vertex_idx = vb_pos / vertex_stride - min_vertex_idx;
    wined3d_stateblock_set_base_vertex_index(device->state, base_vertex_idx);

    d3d8_device_upload_managed_textures(device);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    wined3d_device_context_draw_indexed(device->immediate_context,
            base_vertex_idx, ib_pos / idx_fmt_size, idx_count, 0, 0);

    wined3d_stateblock_set_stream_source(device->state, 0, NULL, 0, 0);
    wined3d_stateblock_set_index_buffer(device->state, NULL, WINED3DFMT_UNKNOWN);
    wined3d_stateblock_set_base_vertex_index(device->state, 0);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d8_device_ProcessVertices(IDirect3DDevice8 *iface, UINT src_start_idx,
        UINT dst_idx, UINT vertex_count, IDirect3DVertexBuffer8 *dst_buffer, DWORD flags)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertexbuffer *dst = unsafe_impl_from_IDirect3DVertexBuffer8(dst_buffer);
    const struct wined3d_stateblock_state *state;
    const struct wined3d_stream_state *stream;
    struct d3d8_vertexbuffer *d3d8_buffer;
    unsigned int i, map;
    HRESULT hr;

    TRACE("iface %p, src_start_idx %u, dst_idx %u, vertex_count %u, dst_buffer %p, flags %#lx.\n",
            iface, src_start_idx, dst_idx, vertex_count, dst_buffer, flags);

    wined3d_mutex_lock();

    state = device->stateblock_state;

    /* Note that an alternative approach would be to simply create these
     * buffers with WINED3D_RESOURCE_ACCESS_MAP_R and update them here like we
     * do for draws. In some regards that would be easier, but it seems less
     * than optimal to upload data to the GPU only to subsequently download it
     * again. */
    map = device->sysmem_vb;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        stream = &state->streams[i];

        d3d8_buffer = wined3d_buffer_get_parent(stream->buffer);
        if (FAILED(wined3d_stateblock_set_stream_source(device->state,
                i, d3d8_buffer->wined3d_buffer, stream->offset, stream->stride)))
            ERR("Failed to set stream source.\n");
    }

    hr = wined3d_device_process_vertices(device->wined3d_device, device->state, src_start_idx, dst_idx,
            vertex_count, dst->wined3d_buffer, NULL, flags, dst->fvf);

    map = device->sysmem_vb;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        stream = &state->streams[i];

        d3d8_buffer = wined3d_buffer_get_parent(stream->buffer);
        if (FAILED(wined3d_stateblock_set_stream_source(device->state,
                i, d3d8_buffer->draw_buffer, stream->offset, stream->stride)))
            ERR("Failed to set stream source.\n");
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_CreateVertexShader(IDirect3DDevice8 *iface,
        const DWORD *declaration, const DWORD *byte_code, DWORD *shader, DWORD usage)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertex_shader *object;
    DWORD shader_handle;
    DWORD handle;
    HRESULT hr;

    TRACE("iface %p, declaration %p, byte_code %p, shader %p, usage %#lx.\n",
            iface, declaration, byte_code, shader, usage);

    if (!(object = calloc(1, sizeof(*object))))
    {
        *shader = 0;
        return E_OUTOFMEMORY;
    }

    wined3d_mutex_lock();
    handle = d3d8_allocate_handle(&device->handle_table, object, D3D8_HANDLE_VS);
    wined3d_mutex_unlock();
    if (handle == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to allocate vertex shader handle.\n");
        free(object);
        *shader = 0;
        return E_OUTOFMEMORY;
    }

    shader_handle = handle + VS_HIGHESTFIXEDFXF + 1;

    hr = d3d8_vertex_shader_init(object, device, declaration, byte_code, shader_handle, usage);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex shader, hr %#lx.\n", hr);
        wined3d_mutex_lock();
        d3d8_free_handle(&device->handle_table, handle, D3D8_HANDLE_VS);
        wined3d_mutex_unlock();
        free(object);
        *shader = 0;
        return hr;
    }

    TRACE("Created vertex shader %p (handle %#lx).\n", object, shader_handle);
    *shader = shader_handle;

    return D3D_OK;
}

static struct d3d8_vertex_declaration *d3d8_device_get_fvf_declaration(struct d3d8_device *device, DWORD fvf)
{
    struct d3d8_vertex_declaration *d3d8_declaration;
    struct FvfToDecl *convertedDecls = device->decls;
    int p, low, high; /* deliberately signed */
    HRESULT hr;

    TRACE("Searching for declaration for fvf %#lx... ", fvf);

    low = 0;
    high = device->numConvertedDecls - 1;
    while (low <= high)
    {
        p = (low + high) >> 1;
        TRACE("%d ", p);

        if (convertedDecls[p].fvf == fvf)
        {
            TRACE("found %p\n", convertedDecls[p].declaration);
            return convertedDecls[p].declaration;
        }

        if (convertedDecls[p].fvf < fvf)
            low = p + 1;
        else
            high = p - 1;
    }
    TRACE("not found. Creating and inserting at position %d.\n", low);

    if (!(d3d8_declaration = malloc(sizeof(*d3d8_declaration))))
        return NULL;

    if (FAILED(hr = d3d8_vertex_declaration_init_fvf(d3d8_declaration, device, fvf)))
    {
        WARN("Failed to initialize vertex declaration, hr %#lx.\n", hr);
        free(d3d8_declaration);
        return NULL;
    }

    if (device->declArraySize == device->numConvertedDecls)
    {
        UINT grow = device->declArraySize / 2;

        if (!(convertedDecls = realloc(convertedDecls,
                sizeof(*convertedDecls) * (device->numConvertedDecls + grow))))
        {
            d3d8_vertex_declaration_destroy(d3d8_declaration);
            return NULL;
        }
        device->decls = convertedDecls;
        device->declArraySize += grow;
    }

    memmove(convertedDecls + low + 1, convertedDecls + low,
            sizeof(*convertedDecls) * (device->numConvertedDecls - low));
    convertedDecls[low].declaration = d3d8_declaration;
    convertedDecls[low].fvf = fvf;
    ++device->numConvertedDecls;

    TRACE("Returning %p. %u decls in array.\n", d3d8_declaration, device->numConvertedDecls);

    return d3d8_declaration;
}

static HRESULT WINAPI d3d8_device_SetVertexShader(IDirect3DDevice8 *iface, DWORD shader)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertex_shader *shader_impl;

    TRACE("iface %p, shader %#lx.\n", iface, shader);

    if (VS_HIGHESTFIXEDFXF >= shader)
    {
        TRACE("Setting FVF, %#lx\n", shader);

        wined3d_mutex_lock();
        wined3d_stateblock_set_vertex_declaration(device->update_state,
                d3d8_device_get_fvf_declaration(device, shader)->wined3d_vertex_declaration);
        wined3d_stateblock_set_vertex_shader(device->update_state, NULL);
        wined3d_mutex_unlock();

        return D3D_OK;
    }

    TRACE("Setting shader\n");

    wined3d_mutex_lock();
    if (!(shader_impl = d3d8_get_object(&device->handle_table, shader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS)))
    {
        WARN("Invalid handle (%#lx) passed.\n", shader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    wined3d_stateblock_set_vertex_declaration(device->update_state,
            shader_impl->vertex_declaration->wined3d_vertex_declaration);
    wined3d_stateblock_set_vertex_shader(device->update_state, shader_impl->wined3d_shader);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetVertexShader(IDirect3DDevice8 *iface, DWORD *shader)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct d3d8_vertex_declaration *d3d8_declaration;

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    if ((wined3d_declaration = device->stateblock_state->vertex_declaration))
    {
        d3d8_declaration = wined3d_vertex_declaration_get_parent(wined3d_declaration);
        *shader = d3d8_declaration->shader_handle;
    }
    else
    {
        *shader = 0;
    }
    wined3d_mutex_unlock();

    TRACE("Returning %#lx.\n", *shader);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_DeleteVertexShader(IDirect3DDevice8 *iface, DWORD shader)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertex_shader *shader_impl;

    TRACE("iface %p, shader %#lx.\n", iface, shader);

    wined3d_mutex_lock();
    if (!(shader_impl = d3d8_free_handle(&device->handle_table, shader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS)))
    {
        WARN("Invalid handle (%#lx) passed.\n", shader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    if (shader_impl->wined3d_shader
            && device->stateblock_state->vs == shader_impl->wined3d_shader)
        IDirect3DDevice8_SetVertexShader(iface, 0);

    wined3d_mutex_unlock();

    d3d8_vertex_shader_destroy(shader_impl);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetVertexShaderConstant(IDirect3DDevice8 *iface,
        DWORD start_register, const void *data, DWORD count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, start_register %lu, data %p, count %lu.\n",
            iface, start_register, data, count);

    if (start_register + count > D3D8_MAX_VERTEX_SHADER_CONSTANTF)
    {
        WARN("Trying to access %lu constants, but d3d8 only supports %u.\n",
             start_register + count, D3D8_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_vs_consts_f(device->update_state, start_register, count, data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetVertexShaderConstant(IDirect3DDevice8 *iface,
        DWORD start_idx, void *constants, DWORD count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %lu, constants %p, count %lu.\n", iface, start_idx, constants, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_vs_consts_f(device->state, start_idx, count, constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetVertexShaderDeclaration(IDirect3DDevice8 *iface,
        DWORD shader, void *data, DWORD *data_size)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertex_declaration *declaration;
    struct d3d8_vertex_shader *shader_impl;

    TRACE("iface %p, shader %#lx, data %p, data_size %p.\n", iface, shader, data, data_size);

    wined3d_mutex_lock();
    shader_impl = d3d8_get_object(&device->handle_table, shader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS);
    wined3d_mutex_unlock();

    if (!shader_impl)
    {
        WARN("Invalid handle (%#lx) passed.\n", shader);
        return D3DERR_INVALIDCALL;
    }
    declaration = shader_impl->vertex_declaration;

    if (!data)
    {
        *data_size = declaration->elements_size;
        return D3D_OK;
    }

    /* MSDN claims that if *data_size is smaller than the required size
     * we should write the required size and return D3DERR_MOREDATA.
     * That's not actually true. */
    if (*data_size < declaration->elements_size)
        return D3DERR_INVALIDCALL;

    memcpy(data, declaration->elements, declaration->elements_size);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetVertexShaderFunction(IDirect3DDevice8 *iface,
        DWORD shader, void *data, DWORD *data_size)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertex_shader *shader_impl = NULL;
    HRESULT hr;

    TRACE("iface %p, shader %#lx, data %p, data_size %p.\n", iface, shader, data, data_size);

    wined3d_mutex_lock();
    if (!(shader_impl = d3d8_get_object(&device->handle_table, shader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS)))
    {
        WARN("Invalid handle (%#lx) passed.\n", shader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    if (!shader_impl->wined3d_shader)
    {
        wined3d_mutex_unlock();
        *data_size = 0;
        return D3D_OK;
    }

    hr = wined3d_shader_get_byte_code(shader_impl->wined3d_shader, data, (unsigned int *)data_size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_SetIndices(IDirect3DDevice8 *iface,
        IDirect3DIndexBuffer8 *buffer, UINT base_vertex_idx)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_indexbuffer *ib = unsafe_impl_from_IDirect3DIndexBuffer8(buffer);
    struct wined3d_buffer *wined3d_buffer;

    TRACE("iface %p, buffer %p, base_vertex_idx %u.\n", iface, buffer, base_vertex_idx);

    if (!ib)
        wined3d_buffer = NULL;
    else
        wined3d_buffer = ib->wined3d_buffer;

    /* WineD3D takes an INT(due to d3d9), but d3d8 uses UINTs. Do I have to add a check here that
     * the UINT doesn't cause an overflow in the INT? It seems rather unlikely because such large
     * vertex buffers can't be created to address them with an index that requires the 32nd bit
     * (4 Byte minimum vertex size * 2^31-1 -> 8 gb buffer. The index sign would be the least
     * problem)
     */
    wined3d_mutex_lock();
    wined3d_stateblock_set_base_vertex_index(device->update_state, base_vertex_idx);
    wined3d_stateblock_set_index_buffer(device->update_state, wined3d_buffer, ib ? ib->format : WINED3DFMT_UNKNOWN);
    if (!device->recording)
        device->sysmem_ib = ib && ib->sysmem;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetIndices(IDirect3DDevice8 *iface,
        IDirect3DIndexBuffer8 **buffer, UINT *base_vertex_index)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    const struct wined3d_stateblock_state *state;
    struct wined3d_buffer *wined3d_buffer;
    struct d3d8_indexbuffer *buffer_impl;

    TRACE("iface %p, buffer %p, base_vertex_index %p.\n", iface, buffer, base_vertex_index);

    if (!buffer)
        return D3DERR_INVALIDCALL;

    /* The case from UINT to INT is safe because d3d8 will never set negative values */
    wined3d_mutex_lock();
    state = device->stateblock_state;
    *base_vertex_index = state->base_vertex_index;
    if ((wined3d_buffer = state->index_buffer))
    {
        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        *buffer = &buffer_impl->IDirect3DIndexBuffer8_iface;
        IDirect3DIndexBuffer8_AddRef(*buffer);
    }
    else
    {
        *buffer = NULL;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_CreatePixelShader(IDirect3DDevice8 *iface,
        const DWORD *byte_code, DWORD *shader)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_pixel_shader *object;
    DWORD shader_handle;
    DWORD handle;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, shader %p.\n", iface, byte_code, shader);

    if (!shader)
        return D3DERR_INVALIDCALL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_mutex_lock();
    handle = d3d8_allocate_handle(&device->handle_table, object, D3D8_HANDLE_PS);
    wined3d_mutex_unlock();
    if (handle == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to allocate pixel shader handle.\n");
        free(object);
        return E_OUTOFMEMORY;
    }

    shader_handle = handle + VS_HIGHESTFIXEDFXF + 1;

    hr = d3d8_pixel_shader_init(object, device, byte_code, shader_handle);
    if (FAILED(hr))
    {
        WARN("Failed to initialize pixel shader, hr %#lx.\n", hr);
        wined3d_mutex_lock();
        d3d8_free_handle(&device->handle_table, handle, D3D8_HANDLE_PS);
        wined3d_mutex_unlock();
        free(object);
        *shader = 0;
        return hr;
    }

    TRACE("Created pixel shader %p (handle %#lx).\n", object, shader_handle);
    *shader = shader_handle;

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetPixelShader(IDirect3DDevice8 *iface, DWORD shader)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_pixel_shader *shader_impl;

    TRACE("iface %p, shader %#lx.\n", iface, shader);

    wined3d_mutex_lock();

    if (!shader)
    {
        wined3d_stateblock_set_pixel_shader(device->update_state, NULL);
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    if (!(shader_impl = d3d8_get_object(&device->handle_table, shader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_PS)))
    {
        WARN("Invalid handle (%#lx) passed.\n", shader);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    TRACE("Setting shader %p.\n", shader_impl);
    wined3d_stateblock_set_pixel_shader(device->update_state, shader_impl->wined3d_shader);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_GetPixelShader(IDirect3DDevice8 *iface, DWORD *shader)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct wined3d_shader *object;

    TRACE("iface %p, shader %p.\n", iface, shader);

    if (!shader)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    if ((object = device->stateblock_state->ps))
    {
        struct d3d8_pixel_shader *d3d8_shader;
        d3d8_shader = wined3d_shader_get_parent(object);
        *shader = d3d8_shader->handle;
    }
    else
    {
        *shader = 0;
    }
    wined3d_mutex_unlock();

    TRACE("Returning %#lx.\n", *shader);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_DeletePixelShader(IDirect3DDevice8 *iface, DWORD shader)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_pixel_shader *shader_impl;

    TRACE("iface %p, shader %#lx.\n", iface, shader);

    wined3d_mutex_lock();

    if (!(shader_impl = d3d8_free_handle(&device->handle_table, shader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_PS)))
    {
        WARN("Invalid handle (%#lx) passed.\n", shader);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    if (device->stateblock_state->ps == shader_impl->wined3d_shader)
        IDirect3DDevice8_SetPixelShader(iface, 0);

    wined3d_mutex_unlock();

    d3d8_pixel_shader_destroy(shader_impl);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_SetPixelShaderConstant(IDirect3DDevice8 *iface,
        DWORD start_register, const void *data, DWORD count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, start_register %lu, data %p, count %lu.\n", iface, start_register, data, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_ps_consts_f(device->update_state, start_register, count, data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetPixelShaderConstant(IDirect3DDevice8 *iface,
        DWORD start_idx, void *constants, DWORD count)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %lu, constants %p, count %lu.\n", iface, start_idx, constants, count);

    if (!wined3d_bound_range(start_idx, count, D3D8_MAX_PIXEL_SHADER_CONSTANTF))
        return WINED3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_ps_consts_f(device->state, start_idx, count, constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetPixelShaderFunction(IDirect3DDevice8 *iface,
        DWORD shader, void *data, DWORD *data_size)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_pixel_shader *shader_impl = NULL;
    HRESULT hr;

    TRACE("iface %p, shader %#lx, data %p, data_size %p.\n", iface, shader, data, data_size);

    wined3d_mutex_lock();
    if (!(shader_impl = d3d8_get_object(&device->handle_table, shader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_PS)))
    {
        WARN("Invalid handle (%#lx) passed.\n", shader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    hr = wined3d_shader_get_byte_code(shader_impl->wined3d_shader, data, (unsigned int *)data_size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_DrawRectPatch(IDirect3DDevice8 *iface, UINT handle,
        const float *segment_count, const D3DRECTPATCH_INFO *patch_info)
{
    FIXME("iface %p, handle %#x, segment_count %p, patch_info %p unimplemented.\n",
            iface, handle, segment_count, patch_info);
    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_DrawTriPatch(IDirect3DDevice8 *iface, UINT handle,
        const float *segment_count, const D3DTRIPATCH_INFO *patch_info)
{
    FIXME("iface %p, handle %#x, segment_count %p, patch_info %p unimplemented.\n",
            iface, handle, segment_count, patch_info);
    return D3D_OK;
}

static HRESULT WINAPI d3d8_device_DeletePatch(IDirect3DDevice8 *iface, UINT handle)
{
    FIXME("iface %p, handle %#x unimplemented.\n", iface, handle);
    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI d3d8_device_SetStreamSource(IDirect3DDevice8 *iface,
        UINT stream_idx, IDirect3DVertexBuffer8 *buffer, UINT stride)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    struct d3d8_vertexbuffer *buffer_impl = unsafe_impl_from_IDirect3DVertexBuffer8(buffer);
    const struct wined3d_stateblock_state *state;
    struct wined3d_buffer *wined3d_buffer;
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, buffer %p, stride %u.\n",
            iface, stream_idx, buffer, stride);

    if (stream_idx >= ARRAY_SIZE(state->streams))
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();

    if (!buffer_impl)
    {
        wined3d_buffer = NULL;
        state = device->stateblock_state;
        stride = state->streams[stream_idx].stride;
    }
    else if (buffer_impl->draw_buffer)
        wined3d_buffer = buffer_impl->draw_buffer;
    else
        wined3d_buffer = buffer_impl->wined3d_buffer;

    hr = wined3d_stateblock_set_stream_source(device->update_state, stream_idx, wined3d_buffer, 0, stride);
    if (SUCCEEDED(hr) && !device->recording)
    {
        if (buffer_impl && buffer_impl->draw_buffer)
            device->sysmem_vb |= (1u << stream_idx);
        else
            device->sysmem_vb &= ~(1u << stream_idx);
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_device_GetStreamSource(IDirect3DDevice8 *iface,
        UINT stream_idx, IDirect3DVertexBuffer8 **buffer, UINT *stride)
{
    struct d3d8_device *device = impl_from_IDirect3DDevice8(iface);
    const struct wined3d_stateblock_state *state;
    const struct wined3d_stream_state *stream;
    struct d3d8_vertexbuffer *buffer_impl;

    TRACE("iface %p, stream_idx %u, buffer %p, stride %p.\n",
            iface, stream_idx, buffer, stride);

    if (!buffer)
        return D3DERR_INVALIDCALL;

    if (stream_idx >= ARRAY_SIZE(state->streams))
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    state = device->stateblock_state;
    stream = &state->streams[stream_idx];
    if (stream->buffer)
    {
        buffer_impl = wined3d_buffer_get_parent(stream->buffer);
        *buffer = &buffer_impl->IDirect3DVertexBuffer8_iface;
        IDirect3DVertexBuffer8_AddRef(*buffer);
    }
    else
        *buffer = NULL;
    *stride = stream->stride;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static const struct IDirect3DDevice8Vtbl d3d8_device_vtbl =
{
    d3d8_device_QueryInterface,
    d3d8_device_AddRef,
    d3d8_device_Release,
    d3d8_device_TestCooperativeLevel,
    d3d8_device_GetAvailableTextureMem,
    d3d8_device_ResourceManagerDiscardBytes,
    d3d8_device_GetDirect3D,
    d3d8_device_GetDeviceCaps,
    d3d8_device_GetDisplayMode,
    d3d8_device_GetCreationParameters,
    d3d8_device_SetCursorProperties,
    d3d8_device_SetCursorPosition,
    d3d8_device_ShowCursor,
    d3d8_device_CreateAdditionalSwapChain,
    d3d8_device_Reset,
    d3d8_device_Present,
    d3d8_device_GetBackBuffer,
    d3d8_device_GetRasterStatus,
    d3d8_device_SetGammaRamp,
    d3d8_device_GetGammaRamp,
    d3d8_device_CreateTexture,
    d3d8_device_CreateVolumeTexture,
    d3d8_device_CreateCubeTexture,
    d3d8_device_CreateVertexBuffer,
    d3d8_device_CreateIndexBuffer,
    d3d8_device_CreateRenderTarget,
    d3d8_device_CreateDepthStencilSurface,
    d3d8_device_CreateImageSurface,
    d3d8_device_CopyRects,
    d3d8_device_UpdateTexture,
    d3d8_device_GetFrontBuffer,
    d3d8_device_SetRenderTarget,
    d3d8_device_GetRenderTarget,
    d3d8_device_GetDepthStencilSurface,
    d3d8_device_BeginScene,
    d3d8_device_EndScene,
    d3d8_device_Clear,
    d3d8_device_SetTransform,
    d3d8_device_GetTransform,
    d3d8_device_MultiplyTransform,
    d3d8_device_SetViewport,
    d3d8_device_GetViewport,
    d3d8_device_SetMaterial,
    d3d8_device_GetMaterial,
    d3d8_device_SetLight,
    d3d8_device_GetLight,
    d3d8_device_LightEnable,
    d3d8_device_GetLightEnable,
    d3d8_device_SetClipPlane,
    d3d8_device_GetClipPlane,
    d3d8_device_SetRenderState,
    d3d8_device_GetRenderState,
    d3d8_device_BeginStateBlock,
    d3d8_device_EndStateBlock,
    d3d8_device_ApplyStateBlock,
    d3d8_device_CaptureStateBlock,
    d3d8_device_DeleteStateBlock,
    d3d8_device_CreateStateBlock,
    d3d8_device_SetClipStatus,
    d3d8_device_GetClipStatus,
    d3d8_device_GetTexture,
    d3d8_device_SetTexture,
    d3d8_device_GetTextureStageState,
    d3d8_device_SetTextureStageState,
    d3d8_device_ValidateDevice,
    d3d8_device_GetInfo,
    d3d8_device_SetPaletteEntries,
    d3d8_device_GetPaletteEntries,
    d3d8_device_SetCurrentTexturePalette,
    d3d8_device_GetCurrentTexturePalette,
    d3d8_device_DrawPrimitive,
    d3d8_device_DrawIndexedPrimitive,
    d3d8_device_DrawPrimitiveUP,
    d3d8_device_DrawIndexedPrimitiveUP,
    d3d8_device_ProcessVertices,
    d3d8_device_CreateVertexShader,
    d3d8_device_SetVertexShader,
    d3d8_device_GetVertexShader,
    d3d8_device_DeleteVertexShader,
    d3d8_device_SetVertexShaderConstant,
    d3d8_device_GetVertexShaderConstant,
    d3d8_device_GetVertexShaderDeclaration,
    d3d8_device_GetVertexShaderFunction,
    d3d8_device_SetStreamSource,
    d3d8_device_GetStreamSource,
    d3d8_device_SetIndices,
    d3d8_device_GetIndices,
    d3d8_device_CreatePixelShader,
    d3d8_device_SetPixelShader,
    d3d8_device_GetPixelShader,
    d3d8_device_DeletePixelShader,
    d3d8_device_SetPixelShaderConstant,
    d3d8_device_GetPixelShaderConstant,
    d3d8_device_GetPixelShaderFunction,
    d3d8_device_DrawRectPatch,
    d3d8_device_DrawTriPatch,
    d3d8_device_DeletePatch,
};

static inline struct d3d8_device *device_from_device_parent(struct wined3d_device_parent *device_parent)
{
    return CONTAINING_RECORD(device_parent, struct d3d8_device, device_parent);
}

static void CDECL device_parent_wined3d_device_created(struct wined3d_device_parent *device_parent,
        struct wined3d_device *device)
{
    TRACE("device_parent %p, device %p\n", device_parent, device);
}

static void CDECL device_parent_mode_changed(struct wined3d_device_parent *device_parent)
{
    TRACE("device_parent %p.\n", device_parent);
}

static void CDECL device_parent_activate(struct wined3d_device_parent *device_parent, BOOL activate)
{
    struct d3d8_device *device = device_from_device_parent(device_parent);

    TRACE("device_parent %p, activate %#x.\n", device_parent, activate);

    if (!activate)
        InterlockedCompareExchange(&device->device_state, D3D8_DEVICE_STATE_LOST, D3D8_DEVICE_STATE_OK);
    else
        InterlockedCompareExchange(&device->device_state, D3D8_DEVICE_STATE_NOT_RESET, D3D8_DEVICE_STATE_LOST);
}

static HRESULT CDECL device_parent_texture_sub_resource_created(struct wined3d_device_parent *device_parent,
        enum wined3d_resource_type type, struct wined3d_texture *wined3d_texture, unsigned int sub_resource_idx,
        void **parent, const struct wined3d_parent_ops **parent_ops)
{
    TRACE("device_parent %p, type %#x, wined3d_texture %p, sub_resource_idx %u, parent %p, parent_ops %p.\n",
            device_parent, type, wined3d_texture, sub_resource_idx, parent, parent_ops);

    if (type == WINED3D_RTYPE_TEXTURE_3D)
    {
        struct d3d8_volume *d3d_volume;

        if (!(d3d_volume = calloc(1, sizeof(*d3d_volume))))
            return E_OUTOFMEMORY;

        volume_init(d3d_volume, wined3d_texture, sub_resource_idx, parent_ops);
        *parent = d3d_volume;
        TRACE("Created volume %p.\n", d3d_volume);
    }
    else if (type != WINED3D_RTYPE_TEXTURE_2D)
    {
        ERR("Unhandled resource type %#x.\n", type);
        return E_FAIL;
    }

    return D3D_OK;
}

static const struct wined3d_device_parent_ops d3d8_wined3d_device_parent_ops =
{
    device_parent_wined3d_device_created,
    device_parent_mode_changed,
    device_parent_activate,
    device_parent_texture_sub_resource_created,
};

static void setup_fpu(void)
{
#if defined(__i386__) && defined(_MSC_VER)
    WORD cw;
    __asm fnstcw cw;
    cw = (cw & ~0xf3f) | 0x3f;
    __asm fldcw cw;
#elif defined(__i386__) || (defined(__x86_64__) && !defined(__arm64ec__) && (defined(__GNUC__) || defined(__clang__)))
    WORD cw;
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
    cw = (cw & ~0xf3f) | 0x3f;
    __asm__ volatile ("fldcw %0" : : "m" (cw));
#else
    FIXME("FPU setup not implemented for this platform.\n");
#endif
}

HRESULT device_init(struct d3d8_device *device, struct d3d8 *parent, struct wined3d *wined3d, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters)
{
    struct wined3d_swapchain_desc swapchain_desc;
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_adapter *wined3d_adapter;
    struct wined3d_output *wined3d_output;
    struct d3d8_swapchain *d3d_swapchain;
    struct wined3d_caps caps;
    unsigned int output_idx;
    HRESULT hr;

    static const enum wined3d_feature_level feature_levels[] =
    {
        WINED3D_FEATURE_LEVEL_8,
        WINED3D_FEATURE_LEVEL_7,
        WINED3D_FEATURE_LEVEL_6,
        WINED3D_FEATURE_LEVEL_5,
    };

    output_idx = adapter;
    if (output_idx >= parent->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    device->IDirect3DDevice8_iface.lpVtbl = &d3d8_device_vtbl;
    device->device_parent.ops = &d3d8_wined3d_device_parent_ops;
    device->adapter_ordinal = adapter;
    device->ref = 1;
    if (!(device->handle_table.entries = calloc(D3D8_INITIAL_HANDLE_TABLE_SIZE,
            sizeof(*device->handle_table.entries))))
    {
        ERR("Failed to allocate handle table memory.\n");
        return E_OUTOFMEMORY;
    }
    device->handle_table.table_size = D3D8_INITIAL_HANDLE_TABLE_SIZE;

    if (!(flags & D3DCREATE_FPU_PRESERVE)) setup_fpu();

    wined3d_mutex_lock();
    wined3d_output = parent->wined3d_outputs[output_idx];
    wined3d_adapter = wined3d_output_get_adapter(wined3d_output);
    if (FAILED(hr = wined3d_device_create(wined3d, wined3d_adapter, wined3d_device_type_from_d3d(device_type),
            focus_window, flags, 4, feature_levels, ARRAY_SIZE(feature_levels),
            &device->device_parent, &device->wined3d_device)))
    {
        WARN("Failed to create wined3d device, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        free(device->handle_table.entries);
        return hr;
    }

    device->immediate_context = wined3d_device_get_immediate_context(device->wined3d_device);
    wined3d_get_device_caps(wined3d_adapter, wined3d_device_type_from_d3d(device_type), &caps);
    device->max_user_clip_planes = caps.MaxUserClipPlanes;
    device->vs_uniform_count = caps.MaxVertexShaderConst;

    if (FAILED(hr = wined3d_stateblock_create(device->wined3d_device, NULL, WINED3D_SBT_PRIMARY, &device->state)))
    {
        ERR("Failed to create primary stateblock, hr %#lx.\n", hr);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        return hr;
    }
    device->stateblock_state = wined3d_stateblock_get_state(device->state);
    device->update_state = device->state;

    wined3d_streaming_buffer_init(&device->vertex_buffer, WINED3D_BIND_VERTEX_BUFFER);
    wined3d_streaming_buffer_init(&device->index_buffer, WINED3D_BIND_INDEX_BUFFER);

    if (!parameters->Windowed)
    {
        if (!focus_window)
            focus_window = parameters->hDeviceWindow;
        if (FAILED(hr = wined3d_device_acquire_focus_window(device->wined3d_device, focus_window)))
        {
            ERR("Failed to acquire focus window, hr %#lx.\n", hr);
            wined3d_device_decref(device->wined3d_device);
            wined3d_mutex_unlock();
            free(device->handle_table.entries);
            return hr;
        }
    }

    if (flags & D3DCREATE_MULTITHREADED)
        wined3d_device_set_multithreaded(device->wined3d_device);

    if (!wined3d_swapchain_desc_from_d3d8(&swapchain_desc, wined3d_output, parameters))
    {
        wined3d_device_release_focus_window(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        free(device->handle_table.entries);
        return D3DERR_INVALIDCALL;
    }
    swapchain_desc.flags |= WINED3D_SWAPCHAIN_IMPLICIT;

    if (FAILED(hr = d3d8_swapchain_create(device, &swapchain_desc,
            wined3dswapinterval_from_d3d(parameters->FullScreen_PresentationInterval), &d3d_swapchain)))
    {
        WARN("Failed to create implicit swapchain, hr %#lx.\n", hr);
        wined3d_device_release_focus_window(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        free(device->handle_table.entries);
        return hr;
    }

    wined3d_swapchain = d3d_swapchain->wined3d_swapchain;
    wined3d_swapchain_incref(wined3d_swapchain);
    IDirect3DSwapChain8_Release(&d3d_swapchain->IDirect3DSwapChain8_iface);

    wined3d_stateblock_set_render_state(device->state, WINED3D_RS_ZENABLE,
            !!swapchain_desc.enable_auto_depth_stencil);
    wined3d_stateblock_set_render_state(device->state, WINED3D_RS_POINTSIZE_MIN, 0);
    device_reset_viewport_state(device);
    wined3d_mutex_unlock();

    present_parameters_from_wined3d_swapchain_desc(parameters,
            &swapchain_desc, parameters->FullScreen_PresentationInterval);

    device->declArraySize = 16;
    if (!(device->decls = malloc(device->declArraySize * sizeof(*device->decls))))
    {
        ERR("Failed to allocate FVF vertex declaration map memory.\n");
        hr = E_OUTOFMEMORY;
        goto err;
    }

    device->implicit_swapchain = wined3d_swapchain;

    device->d3d_parent = parent;
    IDirect3D8_AddRef(&parent->IDirect3D8_iface);

    return D3D_OK;

err:
    wined3d_mutex_lock();
    wined3d_swapchain_decref(wined3d_swapchain);
    wined3d_device_release_focus_window(device->wined3d_device);
    wined3d_device_decref(device->wined3d_device);
    wined3d_mutex_unlock();
    free(device->handle_table.entries);
    return hr;
}
