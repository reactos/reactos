/*
 * IDirect3DDevice9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static void STDMETHODCALLTYPE d3d9_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops d3d9_null_wined3d_parent_ops =
{
    d3d9_null_wined3d_object_destroyed,
};

static void wined3d_color_from_d3dcolor(struct wined3d_color *wined3d_colour, D3DCOLOR d3d_colour)
{
    wined3d_colour->r = ((d3d_colour >> 16) & 0xff) / 255.0f;
    wined3d_colour->g = ((d3d_colour >>  8) & 0xff) / 255.0f;
    wined3d_colour->b = (d3d_colour & 0xff) / 255.0f;
    wined3d_colour->a = ((d3d_colour >> 24) & 0xff) / 255.0f;
}

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
        case WINED3DFMT_R8G8B8A8_UNORM: return D3DFMT_A8B8G8R8;
        case WINED3DFMT_R8G8B8X8_UNORM: return D3DFMT_X8B8G8R8;
        case WINED3DFMT_R16G16_UNORM: return D3DFMT_G16R16;
        case WINED3DFMT_B10G10R10A2_UNORM: return D3DFMT_A2R10G10B10;
        case WINED3DFMT_R16G16B16A16_UNORM: return D3DFMT_A16B16G16R16;
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
        case WINED3DFMT_R10G10B10_SNORM_A2_UNORM: return D3DFMT_A2W10V10U10;
        case WINED3DFMT_D16_LOCKABLE: return D3DFMT_D16_LOCKABLE;
        case WINED3DFMT_D32_UNORM: return D3DFMT_D32;
        case WINED3DFMT_S1_UINT_D15_UNORM: return D3DFMT_D15S1;
        case WINED3DFMT_D24_UNORM_S8_UINT: return D3DFMT_D24S8;
        case WINED3DFMT_X8D24_UNORM: return D3DFMT_D24X8;
        case WINED3DFMT_S4X4_UINT_D24_UNORM: return D3DFMT_D24X4S4;
        case WINED3DFMT_D16_UNORM: return D3DFMT_D16;
        case WINED3DFMT_L16_UNORM: return D3DFMT_L16;
        case WINED3DFMT_D32_FLOAT: return D3DFMT_D32F_LOCKABLE;
        case WINED3DFMT_S8_UINT_D24_FLOAT: return D3DFMT_D24FS8;
        case WINED3DFMT_R16_UINT: return D3DFMT_INDEX16;
        case WINED3DFMT_R32_UINT: return D3DFMT_INDEX32;
        case WINED3DFMT_R16G16B16A16_SNORM: return D3DFMT_Q16W16V16U16;
        case WINED3DFMT_R16_FLOAT: return D3DFMT_R16F;
        case WINED3DFMT_R16G16_FLOAT: return D3DFMT_G16R16F;
        case WINED3DFMT_R16G16B16A16_FLOAT: return D3DFMT_A16B16G16R16F;
        case WINED3DFMT_R32_FLOAT: return D3DFMT_R32F;
        case WINED3DFMT_R32G32_FLOAT: return D3DFMT_G32R32F;
        case WINED3DFMT_R32G32B32A32_FLOAT: return D3DFMT_A32B32G32R32F;
        case WINED3DFMT_R8G8_SNORM_Cx: return D3DFMT_CxV8U8;
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
        case D3DFMT_A8B8G8R8: return WINED3DFMT_R8G8B8A8_UNORM;
        case D3DFMT_X8B8G8R8: return WINED3DFMT_R8G8B8X8_UNORM;
        case D3DFMT_G16R16: return WINED3DFMT_R16G16_UNORM;
        case D3DFMT_A2R10G10B10: return WINED3DFMT_B10G10R10A2_UNORM;
        case D3DFMT_A16B16G16R16: return WINED3DFMT_R16G16B16A16_UNORM;
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
        case D3DFMT_A2W10V10U10: return WINED3DFMT_R10G10B10_SNORM_A2_UNORM;
        case D3DFMT_D16_LOCKABLE: return WINED3DFMT_D16_LOCKABLE;
        case D3DFMT_D32: return WINED3DFMT_D32_UNORM;
        case D3DFMT_D15S1: return WINED3DFMT_S1_UINT_D15_UNORM;
        case D3DFMT_D24S8: return WINED3DFMT_D24_UNORM_S8_UINT;
        case D3DFMT_D24X8: return WINED3DFMT_X8D24_UNORM;
        case D3DFMT_D24X4S4: return WINED3DFMT_S4X4_UINT_D24_UNORM;
        case D3DFMT_D16: return WINED3DFMT_D16_UNORM;
        case D3DFMT_L16: return WINED3DFMT_L16_UNORM;
        case D3DFMT_D32F_LOCKABLE: return WINED3DFMT_D32_FLOAT;
        case D3DFMT_D24FS8: return WINED3DFMT_S8_UINT_D24_FLOAT;
        case D3DFMT_INDEX16: return WINED3DFMT_R16_UINT;
        case D3DFMT_INDEX32: return WINED3DFMT_R32_UINT;
        case D3DFMT_Q16W16V16U16: return WINED3DFMT_R16G16B16A16_SNORM;
        case D3DFMT_R16F: return WINED3DFMT_R16_FLOAT;
        case D3DFMT_G16R16F: return WINED3DFMT_R16G16_FLOAT;
        case D3DFMT_A16B16G16R16F: return WINED3DFMT_R16G16B16A16_FLOAT;
        case D3DFMT_R32F: return WINED3DFMT_R32_FLOAT;
        case D3DFMT_G32R32F: return WINED3DFMT_R32G32_FLOAT;
        case D3DFMT_A32B32G32R32F: return WINED3DFMT_R32G32B32A32_FLOAT;
        case D3DFMT_CxV8U8: return WINED3DFMT_R8G8_SNORM_Cx;
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
            | D3DLOCK_DONOTWAIT
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
        case WINED3D_SWAP_EFFECT_OVERLAY:
            return D3DSWAPEFFECT_OVERLAY;
        case WINED3D_SWAP_EFFECT_FLIP_SEQUENTIAL:
            return D3DSWAPEFFECT_FLIPEX;
        default:
            FIXME("Unhandled swap effect %#x.\n", effect);
            return D3DSWAPEFFECT_FLIP;
    }
}

void present_parameters_from_wined3d_swapchain_desc(D3DPRESENT_PARAMETERS *present_parameters,
        const struct wined3d_swapchain_desc *swapchain_desc, DWORD presentation_interval)
{
    present_parameters->BackBufferWidth = swapchain_desc->backbuffer_width;
    present_parameters->BackBufferHeight = swapchain_desc->backbuffer_height;
    present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(swapchain_desc->backbuffer_format);
    present_parameters->BackBufferCount = swapchain_desc->backbuffer_count;
    present_parameters->MultiSampleType = d3dmultisample_type_from_wined3d(swapchain_desc->multisample_type);
    present_parameters->MultiSampleQuality = swapchain_desc->multisample_quality;
    present_parameters->SwapEffect = d3dswapeffect_from_wined3dswapeffect(swapchain_desc->swap_effect);
    present_parameters->hDeviceWindow = swapchain_desc->device_window;
    present_parameters->Windowed = swapchain_desc->windowed;
    present_parameters->EnableAutoDepthStencil = swapchain_desc->enable_auto_depth_stencil;
    present_parameters->AutoDepthStencilFormat
            = d3dformat_from_wined3dformat(swapchain_desc->auto_depth_stencil_format);
    present_parameters->Flags = swapchain_desc->flags & D3DPRESENTFLAGS_MASK;
    present_parameters->FullScreen_RefreshRateInHz = swapchain_desc->refresh_rate;
    present_parameters->PresentationInterval = presentation_interval;
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
        case D3DSWAPEFFECT_OVERLAY:
            return WINED3D_SWAP_EFFECT_OVERLAY;
        case D3DSWAPEFFECT_FLIPEX:
            return WINED3D_SWAP_EFFECT_FLIP_SEQUENTIAL;
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

static BOOL wined3d_swapchain_desc_from_d3d9(struct wined3d_swapchain_desc *swapchain_desc,
        struct wined3d_output *output, const D3DPRESENT_PARAMETERS *present_parameters,
        BOOL extended)
{
    D3DSWAPEFFECT highest_swapeffect = extended ? D3DSWAPEFFECT_FLIPEX : D3DSWAPEFFECT_COPY;
    UINT highest_bb_count = extended ? 30 : 3;

    if (!present_parameters->SwapEffect || present_parameters->SwapEffect > highest_swapeffect)
    {
        WARN("Invalid swap effect %u passed.\n", present_parameters->SwapEffect);
        return FALSE;
    }
    if (present_parameters->BackBufferCount > highest_bb_count
            || (present_parameters->SwapEffect == D3DSWAPEFFECT_COPY
            && present_parameters->BackBufferCount > 1))
    {
        WARN("Invalid backbuffer count %u.\n", present_parameters->BackBufferCount);
        return FALSE;
    }
    switch (present_parameters->PresentationInterval)
    {
        case D3DPRESENT_INTERVAL_DEFAULT:
        case D3DPRESENT_INTERVAL_ONE:
        case D3DPRESENT_INTERVAL_TWO:
        case D3DPRESENT_INTERVAL_THREE:
        case D3DPRESENT_INTERVAL_FOUR:
        case D3DPRESENT_INTERVAL_IMMEDIATE:
            break;
        default:
            WARN("Invalid presentation interval %#x.\n", present_parameters->PresentationInterval);
            return FALSE;
    }

    swapchain_desc->output = output;
    swapchain_desc->backbuffer_width = present_parameters->BackBufferWidth;
    swapchain_desc->backbuffer_height = present_parameters->BackBufferHeight;
    swapchain_desc->backbuffer_format = wined3dformat_from_d3dformat(present_parameters->BackBufferFormat);
    swapchain_desc->backbuffer_count = max(1, present_parameters->BackBufferCount);
    swapchain_desc->backbuffer_bind_flags = WINED3D_BIND_RENDER_TARGET;
    swapchain_desc->multisample_type = wined3d_multisample_type_from_d3d(present_parameters->MultiSampleType);
    swapchain_desc->multisample_quality = present_parameters->MultiSampleQuality;
    swapchain_desc->swap_effect = wined3dswapeffect_from_d3dswapeffect(present_parameters->SwapEffect);
    swapchain_desc->device_window = present_parameters->hDeviceWindow;
    swapchain_desc->windowed = present_parameters->Windowed;
    swapchain_desc->enable_auto_depth_stencil = present_parameters->EnableAutoDepthStencil;
    swapchain_desc->auto_depth_stencil_format
            = wined3dformat_from_d3dformat(present_parameters->AutoDepthStencilFormat);
    swapchain_desc->flags
            = (present_parameters->Flags & D3DPRESENTFLAGS_MASK) | WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH;
    if (extended)
        swapchain_desc->flags |= WINED3D_SWAPCHAIN_RESTORE_WINDOW_STATE;
    if ((present_parameters->Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
            && (is_gdi_compat_wined3dformat(swapchain_desc->backbuffer_format)
            /* WINED3DFMT_UNKNOWN creates the swapchain with the current
             * display format, which is always GDI-compatible. */
            || swapchain_desc->backbuffer_format == WINED3DFMT_UNKNOWN))
        swapchain_desc->flags |= WINED3D_SWAPCHAIN_GDI_COMPATIBLE;
    swapchain_desc->refresh_rate = present_parameters->FullScreen_RefreshRateInHz;
    swapchain_desc->auto_restore_display_mode = TRUE;

    if (present_parameters->Flags & ~D3DPRESENTFLAGS_MASK)
        FIXME("Unhandled flags %#lx.\n", present_parameters->Flags & ~D3DPRESENTFLAGS_MASK);

    return TRUE;
}

void d3d9_caps_from_wined3dcaps(const struct d3d9 *d3d9, unsigned int adapter_ordinal,
        D3DCAPS9 *caps, const struct wined3d_caps *wined3d_caps)
{
    static const DWORD ps_minor_version[] = {0, 4, 0, 0};
    static const DWORD vs_minor_version[] = {0, 1, 0, 0};
    static const DWORD texture_filter_caps =
        D3DPTFILTERCAPS_MINFPOINT      | D3DPTFILTERCAPS_MINFLINEAR    | D3DPTFILTERCAPS_MINFANISOTROPIC |
        D3DPTFILTERCAPS_MINFPYRAMIDALQUAD                              | D3DPTFILTERCAPS_MINFGAUSSIANQUAD|
        D3DPTFILTERCAPS_MIPFPOINT      | D3DPTFILTERCAPS_MIPFLINEAR    | D3DPTFILTERCAPS_MAGFPOINT       |
        D3DPTFILTERCAPS_MAGFLINEAR     |D3DPTFILTERCAPS_MAGFANISOTROPIC|D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD|
        D3DPTFILTERCAPS_MAGFGAUSSIANQUAD;
    struct wined3d_adapter *wined3d_adapter;
    struct wined3d_output_desc output_desc;
    struct wined3d_output *wined3d_output;
    unsigned int output_idx;
    HRESULT hr;

    caps->DeviceType                        = (D3DDEVTYPE)wined3d_caps->DeviceType;
    caps->AdapterOrdinal                    = adapter_ordinal;
    caps->Caps                              = wined3d_caps->Caps;
    caps->Caps2                             = wined3d_caps->Caps2;
    caps->Caps3                             = wined3d_caps->Caps3;
    caps->PresentationIntervals             = D3DPRESENT_INTERVAL_IMMEDIATE | D3DPRESENT_INTERVAL_ONE;
    caps->CursorCaps                        = wined3d_caps->CursorCaps;
    caps->DevCaps                           = wined3d_caps->DevCaps;
    caps->PrimitiveMiscCaps                 = wined3d_caps->PrimitiveMiscCaps;
    caps->RasterCaps                        = wined3d_caps->RasterCaps;
    caps->ZCmpCaps                          = wined3d_caps->ZCmpCaps;
    caps->SrcBlendCaps                      = wined3d_caps->SrcBlendCaps;
    caps->DestBlendCaps                     = wined3d_caps->DestBlendCaps;
    caps->AlphaCmpCaps                      = wined3d_caps->AlphaCmpCaps;
    caps->ShadeCaps                         = wined3d_caps->ShadeCaps;
    caps->TextureCaps                       = wined3d_caps->TextureCaps;
    caps->TextureFilterCaps                 = wined3d_caps->TextureFilterCaps;
    caps->CubeTextureFilterCaps             = wined3d_caps->CubeTextureFilterCaps;
    caps->VolumeTextureFilterCaps           = wined3d_caps->VolumeTextureFilterCaps;
    caps->TextureAddressCaps                = wined3d_caps->TextureAddressCaps;
    caps->VolumeTextureAddressCaps          = wined3d_caps->VolumeTextureAddressCaps;
    caps->LineCaps                          = wined3d_caps->LineCaps;
    caps->MaxTextureWidth                   = wined3d_caps->MaxTextureWidth;
    caps->MaxTextureHeight                  = wined3d_caps->MaxTextureHeight;
    caps->MaxVolumeExtent                   = wined3d_caps->MaxVolumeExtent;
    caps->MaxTextureRepeat                  = wined3d_caps->MaxTextureRepeat;
    caps->MaxTextureAspectRatio             = wined3d_caps->MaxTextureAspectRatio;
    caps->MaxAnisotropy                     = wined3d_caps->MaxAnisotropy;
    caps->MaxVertexW                        = wined3d_caps->MaxVertexW;
    caps->GuardBandLeft                     = wined3d_caps->GuardBandLeft;
    caps->GuardBandTop                      = wined3d_caps->GuardBandTop;
    caps->GuardBandRight                    = wined3d_caps->GuardBandRight;
    caps->GuardBandBottom                   = wined3d_caps->GuardBandBottom;
    caps->ExtentsAdjust                     = wined3d_caps->ExtentsAdjust;
    caps->StencilCaps                       = wined3d_caps->StencilCaps;
    caps->FVFCaps                           = wined3d_caps->FVFCaps;
    caps->TextureOpCaps                     = wined3d_caps->TextureOpCaps;
    caps->MaxTextureBlendStages             = wined3d_caps->MaxTextureBlendStages;
    caps->MaxSimultaneousTextures           = wined3d_caps->MaxSimultaneousTextures;
    caps->VertexProcessingCaps              = wined3d_caps->VertexProcessingCaps;
    caps->MaxActiveLights                   = wined3d_caps->MaxActiveLights;
    caps->MaxUserClipPlanes                 = wined3d_caps->MaxUserClipPlanes;
    caps->MaxVertexBlendMatrices            = wined3d_caps->MaxVertexBlendMatrices;
    caps->MaxVertexBlendMatrixIndex         = wined3d_caps->MaxVertexBlendMatrixIndex;
    caps->MaxPointSize                      = wined3d_caps->MaxPointSize;
    caps->MaxPrimitiveCount                 = wined3d_caps->MaxPrimitiveCount;
    caps->MaxVertexIndex                    = wined3d_caps->MaxVertexIndex;
    caps->MaxStreams                        = wined3d_caps->MaxStreams;
    caps->MaxStreamStride                   = wined3d_caps->MaxStreamStride;
    caps->VertexShaderVersion               = wined3d_caps->VertexShaderVersion;
    caps->MaxVertexShaderConst              = wined3d_caps->MaxVertexShaderConst;
    caps->PixelShaderVersion                = wined3d_caps->PixelShaderVersion;
    caps->PixelShader1xMaxValue             = wined3d_caps->PixelShader1xMaxValue;
    caps->DevCaps2                          = wined3d_caps->DevCaps2;
    caps->MaxNpatchTessellationLevel        = wined3d_caps->MaxNpatchTessellationLevel;
    caps->DeclTypes                         = wined3d_caps->DeclTypes;
    caps->NumSimultaneousRTs                = wined3d_caps->NumSimultaneousRTs;
    caps->StretchRectFilterCaps             = wined3d_caps->StretchRectFilterCaps;
    caps->VS20Caps.Caps                     = wined3d_caps->VS20Caps.caps;
    caps->VS20Caps.DynamicFlowControlDepth  = wined3d_caps->VS20Caps.dynamic_flow_control_depth;
    caps->VS20Caps.NumTemps                 = wined3d_caps->VS20Caps.temp_count;
    caps->VS20Caps.StaticFlowControlDepth   = wined3d_caps->VS20Caps.static_flow_control_depth;
    caps->PS20Caps.Caps                     = wined3d_caps->PS20Caps.caps;
    caps->PS20Caps.DynamicFlowControlDepth  = wined3d_caps->PS20Caps.dynamic_flow_control_depth;
    caps->PS20Caps.NumTemps                 = wined3d_caps->PS20Caps.temp_count;
    caps->PS20Caps.StaticFlowControlDepth   = wined3d_caps->PS20Caps.static_flow_control_depth;
    caps->PS20Caps.NumInstructionSlots      = wined3d_caps->PS20Caps.instruction_slot_count;
    caps->VertexTextureFilterCaps           = wined3d_caps->VertexTextureFilterCaps;
    caps->MaxVShaderInstructionsExecuted    = wined3d_caps->MaxVShaderInstructionsExecuted;
    caps->MaxPShaderInstructionsExecuted    = wined3d_caps->MaxPShaderInstructionsExecuted;
    caps->MaxVertexShader30InstructionSlots = wined3d_caps->MaxVertexShader30InstructionSlots;
    caps->MaxPixelShader30InstructionSlots  = wined3d_caps->MaxPixelShader30InstructionSlots;

    /* Some functionality is implemented in d3d9.dll, not wined3d.dll. Add the needed caps. */
    caps->DevCaps2 |= D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;

    /* Filter wined3d caps. */
    caps->TextureFilterCaps &= texture_filter_caps;
    caps->CubeTextureFilterCaps &= texture_filter_caps;
    caps->VolumeTextureFilterCaps &= texture_filter_caps;

    caps->DevCaps &=
        D3DDEVCAPS_EXECUTESYSTEMMEMORY | D3DDEVCAPS_EXECUTEVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TLVERTEXVIDEOMEMORY | D3DDEVCAPS_TEXTURESYSTEMMEMORY| D3DDEVCAPS_TEXTUREVIDEOMEMORY   |
        D3DDEVCAPS_DRAWPRIMTLVERTEX    | D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_TEXTURENONLOCALVIDMEM|
        D3DDEVCAPS_DRAWPRIMITIVES2     | D3DDEVCAPS_SEPARATETEXTUREMEMORIES                              |
        D3DDEVCAPS_DRAWPRIMITIVES2EX   | D3DDEVCAPS_HWTRANSFORMANDLIGHT| D3DDEVCAPS_CANBLTSYSTONONLOCAL  |
        D3DDEVCAPS_HWRASTERIZATION     | D3DDEVCAPS_PUREDEVICE         | D3DDEVCAPS_QUINTICRTPATCHES     |
        D3DDEVCAPS_RTPATCHES           | D3DDEVCAPS_RTPATCHHANDLEZERO  | D3DDEVCAPS_NPATCHES;

    caps->ShadeCaps &=
        D3DPSHADECAPS_COLORGOURAUDRGB  | D3DPSHADECAPS_SPECULARGOURAUDRGB |
        D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_FOGGOURAUD;

    caps->RasterCaps &=
        D3DPRASTERCAPS_DITHER          | D3DPRASTERCAPS_ZTEST          | D3DPRASTERCAPS_FOGVERTEX        |
        D3DPRASTERCAPS_FOGTABLE        | D3DPRASTERCAPS_MIPMAPLODBIAS  | D3DPRASTERCAPS_ZBUFFERLESSHSR   |
        D3DPRASTERCAPS_FOGRANGE        | D3DPRASTERCAPS_ANISOTROPY     | D3DPRASTERCAPS_WBUFFER          |
        D3DPRASTERCAPS_WFOG            | D3DPRASTERCAPS_ZFOG           | D3DPRASTERCAPS_COLORPERSPECTIVE |
        D3DPRASTERCAPS_SCISSORTEST     | D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS                              |
        D3DPRASTERCAPS_DEPTHBIAS       | D3DPRASTERCAPS_MULTISAMPLE_TOGGLE;

    caps->DevCaps2 &=
        D3DDEVCAPS2_STREAMOFFSET       | D3DDEVCAPS2_DMAPNPATCH        | D3DDEVCAPS2_ADAPTIVETESSRTPATCH |
        D3DDEVCAPS2_ADAPTIVETESSNPATCH | D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES                       |
        D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH| D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET;

    caps->Caps2 &=
        D3DCAPS2_FULLSCREENGAMMA       | D3DCAPS2_CANCALIBRATEGAMMA    | D3DCAPS2_RESERVED               |
        D3DCAPS2_CANMANAGERESOURCE     | D3DCAPS2_DYNAMICTEXTURES      | D3DCAPS2_CANAUTOGENMIPMAP;

    caps->VertexProcessingCaps &=
        D3DVTXPCAPS_TEXGEN             | D3DVTXPCAPS_MATERIALSOURCE7   | D3DVTXPCAPS_DIRECTIONALLIGHTS   |
        D3DVTXPCAPS_POSITIONALLIGHTS   | D3DVTXPCAPS_LOCALVIEWER       | D3DVTXPCAPS_TWEENING            |
        D3DVTXPCAPS_TEXGEN_SPHEREMAP   | D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER;

    caps->TextureCaps &=
        D3DPTEXTURECAPS_PERSPECTIVE    | D3DPTEXTURECAPS_POW2          | D3DPTEXTURECAPS_ALPHA           |
        D3DPTEXTURECAPS_SQUAREONLY     | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE                        |
        D3DPTEXTURECAPS_ALPHAPALETTE   | D3DPTEXTURECAPS_NONPOW2CONDITIONAL                              |
        D3DPTEXTURECAPS_PROJECTED      | D3DPTEXTURECAPS_CUBEMAP       | D3DPTEXTURECAPS_VOLUMEMAP       |
        D3DPTEXTURECAPS_MIPMAP         | D3DPTEXTURECAPS_MIPVOLUMEMAP  | D3DPTEXTURECAPS_MIPCUBEMAP      |
        D3DPTEXTURECAPS_CUBEMAP_POW2   | D3DPTEXTURECAPS_VOLUMEMAP_POW2| D3DPTEXTURECAPS_NOPROJECTEDBUMPENV;

    caps->MaxVertexShaderConst = min(D3D9_MAX_VERTEX_SHADER_CONSTANTF, caps->MaxVertexShaderConst);
    caps->NumSimultaneousRTs = min(D3D_MAX_SIMULTANEOUS_RENDERTARGETS, caps->NumSimultaneousRTs);

    if (caps->PixelShaderVersion > 3)
    {
        caps->PixelShaderVersion = D3DPS_VERSION(3, 0);
    }
    else
    {
        DWORD major = caps->PixelShaderVersion;
        caps->PixelShaderVersion = D3DPS_VERSION(major, ps_minor_version[major]);
    }

    if (caps->VertexShaderVersion > 3)
    {
        caps->VertexShaderVersion = D3DVS_VERSION(3, 0);
    }
    else
    {
        DWORD major = caps->VertexShaderVersion;
        caps->VertexShaderVersion = D3DVS_VERSION(major, vs_minor_version[major]);
    }

    /* Get adapter group information */
    output_idx = adapter_ordinal;
    wined3d_output = d3d9->wined3d_outputs[output_idx];

    wined3d_mutex_lock();
    hr = wined3d_output_get_desc(wined3d_output, &output_desc);
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        ERR("Failed to get output desc, hr %#lx.\n", hr);
        return;
    }

    caps->MasterAdapterOrdinal = output_idx - output_desc.ordinal;
    caps->AdapterOrdinalInGroup = output_desc.ordinal;
    if (!caps->AdapterOrdinalInGroup)
    {
        wined3d_adapter = wined3d_output_get_adapter(wined3d_output);
        caps->NumberOfAdaptersInGroup = wined3d_adapter_get_output_count(wined3d_adapter);
    }
    else
    {
        caps->NumberOfAdaptersInGroup = 0;
    }
}

static enum wined3d_texture_filter_type wined3d_texture_filter_type_from_d3d(D3DTEXTUREFILTERTYPE type)
{
    return (enum wined3d_texture_filter_type)type;
}

static enum wined3d_transform_state wined3d_transform_state_from_d3d(D3DTRANSFORMSTATETYPE type)
{
    return (enum wined3d_transform_state)type;
}

static enum wined3d_render_state wined3d_render_state_from_d3d(D3DRENDERSTATETYPE type)
{
    return (enum wined3d_render_state)type;
}

static enum wined3d_sampler_state wined3d_sampler_state_from_d3d(D3DSAMPLERSTATETYPE type)
{
    return (enum wined3d_sampler_state)type;
}

static enum wined3d_primitive_type wined3d_primitive_type_from_d3d(D3DPRIMITIVETYPE type)
{
    return (enum wined3d_primitive_type)type;
}

static void device_reset_viewport_state(struct d3d9_device *device)
{
    struct wined3d_viewport vp;
    RECT rect;

    wined3d_device_context_get_viewports(device->immediate_context, NULL, &vp);
    wined3d_stateblock_set_viewport(device->state, &vp);
    wined3d_device_context_get_scissor_rects(device->immediate_context, NULL, &rect);
    wined3d_stateblock_set_scissor_rect(device->state, &rect);
}

static struct d3d9_device *impl_from_IDirect3DDevice9On12(IDirect3DDevice9On12 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_device, IDirect3DDevice9On12_iface);
}

static HRESULT WINAPI d3d9on12_QueryInterface(IDirect3DDevice9On12 *iface, REFIID iid, void **out)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9On12(iface);
    return IDirect3DDevice9Ex_QueryInterface(&device->IDirect3DDevice9Ex_iface, iid, out);
}

static ULONG WINAPI d3d9on12_AddRef(IDirect3DDevice9On12 *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9On12(iface);
    return IDirect3DDevice9Ex_AddRef(&device->IDirect3DDevice9Ex_iface);
}

static ULONG WINAPI d3d9on12_Release(IDirect3DDevice9On12 *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9On12(iface);
    return IDirect3DDevice9Ex_Release(&device->IDirect3DDevice9Ex_iface);
}

static HRESULT WINAPI d3d9on12_GetD3D12Device(IDirect3DDevice9On12 *iface, REFIID iid, void **out)
{
    FIXME("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (!out)
        return E_INVALIDARG;

    *out = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI d3d9on12_UnwrapUnderlyingResource(IDirect3DDevice9On12 *iface, IDirect3DResource9 *resource, ID3D12CommandQueue *queue, REFIID iid, void **out)
{
    FIXME("iface %p, resource %p, queue %p, iid %s, out %p stub!\n", iface, resource, queue, debugstr_guid(iid), out);
    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9on12_ReturnUnderlyingResource(IDirect3DDevice9On12 *iface, IDirect3DResource9 *resource, UINT fence_count, UINT64 *signal_values, ID3D12Fence **fences)
{
    FIXME("iface %p, resource %p, fence_count %#x, signal_values %p, fences %p stub!\n", iface, resource, fence_count, signal_values, fences);
    return E_NOTIMPL;
}

static const struct IDirect3DDevice9On12Vtbl d3d9on12_vtbl =
{
    /* IUnknown */
    d3d9on12_QueryInterface,
    d3d9on12_AddRef,
    d3d9on12_Release,
    /* IDirect3DDevice9On12 */
    d3d9on12_GetD3D12Device,
    d3d9on12_UnwrapUnderlyingResource,
    d3d9on12_ReturnUnderlyingResource
};

static HRESULT WINAPI d3d9_device_QueryInterface(IDirect3DDevice9Ex *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DDevice9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DDevice9Ex_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirect3DDevice9Ex))
    {
        struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

        /* Find out if the creating d3d9 interface was created with Direct3DCreate9Ex.
         * It doesn't matter with which function the device was created. */
        if (!device->d3d_parent->extended)
        {
            WARN("IDirect3D9 instance wasn't created with CreateDirect3D9Ex, returning E_NOINTERFACE.\n");
            *out = NULL;
            return E_NOINTERFACE;
        }

        IDirect3DDevice9Ex_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirect3DDevice9On12))
    {
        struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

        if (!device->d3d_parent->d3d9on12)
        {
            WARN("IDirect3D9 instance wasn't created with D3D9On12 enabled, returning E_NOINTERFACE.\n");
            *out = NULL;
            return E_NOINTERFACE;
        }

        IDirect3DDevice9Ex_AddRef(iface);
        *out = &device->IDirect3DDevice9On12_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_device_AddRef(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    ULONG refcount = InterlockedIncrement(&device->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI DECLSPEC_HOTPATCH d3d9_device_Release(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    ULONG refcount;

    if (device->in_destruction)
        return 0;

    refcount = InterlockedDecrement(&device->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        unsigned i;
        device->in_destruction = TRUE;

        wined3d_mutex_lock();
        for (i = 0; i < device->fvf_decl_count; ++i)
        {
            wined3d_vertex_declaration_decref(device->fvf_decls[i].decl);
        }
        free(device->fvf_decls);

        wined3d_streaming_buffer_cleanup(&device->vertex_buffer);
        wined3d_streaming_buffer_cleanup(&device->index_buffer);

        for (i = 0; i < device->implicit_swapchain_count; ++i)
        {
            wined3d_swapchain_decref(device->implicit_swapchains[i]);
        }
        free(device->implicit_swapchains);

        if (device->recording)
            wined3d_stateblock_decref(device->recording);
        wined3d_stateblock_decref(device->state);

        wined3d_device_release_focus_window(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();

        IDirect3D9Ex_Release(&device->d3d_parent->IDirect3D9Ex_iface);

        free(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_device_TestCooperativeLevel(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p.\n", iface);

    TRACE("device state: %#lx.\n", device->device_state);

    if (device->d3d_parent->extended)
        return D3D_OK;

    switch (device->device_state)
    {
        default:
        case D3D9_DEVICE_STATE_OK:
            return D3D_OK;
        case D3D9_DEVICE_STATE_LOST:
            return D3DERR_DEVICELOST;
        case D3D9_DEVICE_STATE_NOT_RESET:
            return D3DERR_DEVICENOTRESET;
    }
}

static UINT WINAPI d3d9_device_GetAvailableTextureMem(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    UINT ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_device_get_available_texture_mem(device->wined3d_device);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d9_device_EvictManagedResources(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_device_evict_managed_resources(device->wined3d_device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetDirect3D(IDirect3DDevice9Ex *iface, IDirect3D9 **d3d9)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, d3d9 %p.\n", iface, d3d9);

    if (!d3d9)
        return D3DERR_INVALIDCALL;

    return IDirect3D9Ex_QueryInterface(&device->d3d_parent->IDirect3D9Ex_iface, &IID_IDirect3D9, (void **)d3d9);
}

static HRESULT WINAPI d3d9_device_GetDeviceCaps(IDirect3DDevice9Ex *iface, D3DCAPS9 *caps)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_caps wined3d_caps;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    if (!caps)
        return D3DERR_INVALIDCALL;

    memset(caps, 0, sizeof(*caps));

    wined3d_mutex_lock();
    hr = wined3d_device_get_device_caps(device->wined3d_device, &wined3d_caps);
    wined3d_mutex_unlock();

    d3d9_caps_from_wined3dcaps(device->d3d_parent, device->adapter_ordinal, caps, &wined3d_caps);

    return hr;
}

static HRESULT WINAPI d3d9_device_GetDisplayMode(IDirect3DDevice9Ex *iface, UINT swapchain, D3DDISPLAYMODE *mode)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, swapchain %u, mode %p.\n", iface, swapchain, mode);

    wined3d_mutex_lock();
    hr = wined3d_device_get_display_mode(device->wined3d_device, swapchain, &wined3d_mode, NULL);
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

static HRESULT WINAPI d3d9_device_GetCreationParameters(IDirect3DDevice9Ex *iface,
        D3DDEVICE_CREATION_PARAMETERS *parameters)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, parameters %p.\n", iface, parameters);

    wined3d_mutex_lock();
    wined3d_device_get_creation_parameters(device->wined3d_device,
            (struct wined3d_device_creation_parameters *)parameters);
    wined3d_mutex_unlock();

    parameters->AdapterOrdinal = device->adapter_ordinal;
    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetCursorProperties(IDirect3DDevice9Ex *iface,
        UINT hotspot_x, UINT hotspot_y, IDirect3DSurface9 *bitmap)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *bitmap_impl = unsafe_impl_from_IDirect3DSurface9(bitmap);
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

    if (FAILED(hr = IDirect3DSurface9_GetDesc(bitmap, &surface_desc)))
    {
        WARN("Failed to get surface description, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IDirect3D9_GetAdapterDisplayMode(&device->d3d_parent->IDirect3D9Ex_iface,
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

static void WINAPI d3d9_device_SetCursorPosition(IDirect3DDevice9Ex *iface, int x, int y, DWORD flags)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, x %u, y %u, flags %#lx.\n", iface, x, y, flags);

    wined3d_mutex_lock();
    wined3d_device_set_cursor_position(device->wined3d_device, x, y, flags);
    wined3d_mutex_unlock();
}

static BOOL WINAPI d3d9_device_ShowCursor(IDirect3DDevice9Ex *iface, BOOL show)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    BOOL ret;

    TRACE("iface %p, show %#x.\n", iface, show);

    wined3d_mutex_lock();
    ret = wined3d_device_show_cursor(device->wined3d_device, show);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_CreateAdditionalSwapChain(IDirect3DDevice9Ex *iface,
        D3DPRESENT_PARAMETERS *present_parameters, IDirect3DSwapChain9 **swapchain)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_device_creation_parameters creation_parameters;
    struct wined3d_swapchain_desc desc;
    struct d3d9_swapchain *object;
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
    if (!wined3d_swapchain_desc_from_d3d9(&desc, device->d3d_parent->wined3d_outputs[output_idx],
            present_parameters, device->d3d_parent->extended))
        return D3DERR_INVALIDCALL;
    wined3d_device_get_creation_parameters(device->wined3d_device, &creation_parameters);
    if (creation_parameters.flags & WINED3DCREATE_NOWINDOWCHANGES)
        desc.flags |= WINED3D_SWAPCHAIN_NO_WINDOW_CHANGES;
    swap_interval = wined3dswapinterval_from_d3d(present_parameters->PresentationInterval);
    if (SUCCEEDED(hr = d3d9_swapchain_create(device, &desc, swap_interval, &object)))
        *swapchain = (IDirect3DSwapChain9 *)&object->IDirect3DSwapChain9Ex_iface;
    present_parameters_from_wined3d_swapchain_desc(present_parameters,
            &desc, present_parameters->PresentationInterval);

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_GetSwapChain(IDirect3DDevice9Ex *iface,
        UINT swapchain_idx, IDirect3DSwapChain9 **swapchain)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_swapchain *d3d9_swapchain;
    HRESULT hr;

    TRACE("iface %p, swapchain_idx %u, swapchain %p.\n", iface, swapchain_idx, swapchain);

    wined3d_mutex_lock();
    if (swapchain_idx < device->implicit_swapchain_count)
    {
        d3d9_swapchain = wined3d_swapchain_get_parent(device->implicit_swapchains[swapchain_idx]);
        *swapchain = (IDirect3DSwapChain9 *)&d3d9_swapchain->IDirect3DSwapChain9Ex_iface;
        IDirect3DSwapChain9Ex_AddRef(*swapchain);
        hr = D3D_OK;
    }
    else
    {
        *swapchain = NULL;
        hr = D3DERR_INVALIDCALL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static UINT WINAPI d3d9_device_GetNumberOfSwapChains(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    UINT count;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    count = wined3d_device_get_swapchain_count(device->wined3d_device);
    wined3d_mutex_unlock();

    return count;
}

static HRESULT CDECL reset_enum_callback(struct wined3d_resource *resource)
{
    struct d3d9_vertexbuffer *vertex_buffer;
    struct d3d9_indexbuffer *index_buffer;
    struct wined3d_resource_desc desc;
    IDirect3DBaseTexture9 *texture;
    struct d3d9_surface *surface;
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
        return D3DERR_INVALIDCALL;
    }

    parent = wined3d_resource_get_parent(resource);
    if (parent && SUCCEEDED(IUnknown_QueryInterface(parent, &IID_IDirect3DBaseTexture9, (void **)&texture)))
    {
        IDirect3DBaseTexture9_Release(texture);
        WARN("Texture %p (resource %p) in pool D3DPOOL_DEFAULT blocks the Reset call.\n", texture, resource);
        return D3DERR_INVALIDCALL;
    }

    surface = wined3d_texture_get_sub_resource_parent(wined3d_texture_from_resource(resource), 0);
    if (!surface || !surface->resource.refcount)
        return D3D_OK;

    WARN("Surface %p in pool D3DPOOL_DEFAULT blocks the Reset call.\n", surface);
    return D3DERR_INVALIDCALL;
}

static HRESULT d3d9_device_get_swapchains(struct d3d9_device *device)
{
    UINT i, new_swapchain_count = wined3d_device_get_swapchain_count(device->wined3d_device);

    if (!(device->implicit_swapchains = malloc(new_swapchain_count * sizeof(*device->implicit_swapchains))))
        return E_OUTOFMEMORY;

    for (i = 0; i < new_swapchain_count; ++i)
    {
        device->implicit_swapchains[i] = wined3d_device_get_swapchain(device->wined3d_device, i);
    }
    device->implicit_swapchain_count = new_swapchain_count;

    return D3D_OK;
}

static HRESULT d3d9_device_reset(struct d3d9_device *device,
        D3DPRESENT_PARAMETERS *present_parameters, D3DDISPLAYMODEEX *mode)
{
    struct wined3d_swapchain_desc swapchain_desc, old_swapchain_desc;
    struct d3d9_surface **old_backbuffers = NULL;
    BOOL extended = device->d3d_parent->extended;
    struct wined3d_display_mode wined3d_mode;
    struct wined3d_rendertarget_view *rtv;
    struct d3d9_swapchain *d3d9_swapchain;
    unsigned int output_idx;
    unsigned int i;
    HRESULT hr;

    if (!extended && device->device_state == D3D9_DEVICE_STATE_LOST)
    {
        WARN("App not active, returning D3DERR_DEVICELOST.\n");
        return D3DERR_DEVICELOST;
    }

    if (mode)
    {
        wined3d_mode.width = mode->Width;
        wined3d_mode.height = mode->Height;
        wined3d_mode.refresh_rate = mode->RefreshRate;
        wined3d_mode.format_id = wined3dformat_from_d3dformat(mode->Format);
        wined3d_mode.scanline_ordering = wined3d_scanline_ordering_from_d3d(mode->ScanLineOrdering);
    }

    output_idx = device->adapter_ordinal;
    if (!wined3d_swapchain_desc_from_d3d9(&swapchain_desc,
            device->d3d_parent->wined3d_outputs[output_idx], present_parameters, extended))
        return D3DERR_INVALIDCALL;
    swapchain_desc.flags |= WINED3D_SWAPCHAIN_IMPLICIT;

    wined3d_mutex_lock();

    wined3d_streaming_buffer_cleanup(&device->vertex_buffer);
    wined3d_streaming_buffer_cleanup(&device->index_buffer);

    if (!extended)
    {
        if (device->recording)
            wined3d_stateblock_decref(device->recording);
        device->recording = NULL;
        device->update_state = device->state;
        wined3d_stateblock_reset(device->state);
    }

    if (FAILED(hr = d3d9_device_get_swapchains(device)))
    {
        wined3d_mutex_unlock();
        return hr;
    }
    d3d9_swapchain = wined3d_swapchain_get_parent(device->implicit_swapchains[0]);

    wined3d_swapchain_get_desc(d3d9_swapchain->wined3d_swapchain, &old_swapchain_desc);

    /* wined3d_device_reset() may recreate swapchain textures.
     *
     * If the device is not extended, we don't need to remove the reference to
     * the wined3d swapchain from the old d3d9 surfaces: we will fail the reset
     * if they have 0 references, and therefore they are not actually holding a
     * reference to the wined3d swapchain, and will not do anything with it when
     * they are destroyed.
     *
     * If the device is extended, those swapchain textures can survive the
     * reset, and need to be detached from the implicit swapchain here. To add
     * some complexity, we need to add a temporary reference, but we can only do
     * that in the extended case, otherwise we'll try to validate that there are
     * 0 reference and fail. */

    if (extended)
    {
        if (!(old_backbuffers = malloc(old_swapchain_desc.backbuffer_count * sizeof(*old_backbuffers))))
        {
            wined3d_mutex_unlock();
            return hr;
        }

        for (i = 0; i < old_swapchain_desc.backbuffer_count; ++i)
        {
            old_backbuffers[i] = wined3d_texture_get_sub_resource_parent(
                    wined3d_swapchain_get_back_buffer(d3d9_swapchain->wined3d_swapchain, i), 0);
            /* Resetting might drop the last reference, so grab an extra one here.
             * This also lets us always decref the swapchain, without needing to
             * worry about whether it's externally referenced. */
            IDirect3DSurface9_AddRef(&old_backbuffers[i]->IDirect3DSurface9_iface);
        }
    }

    if (SUCCEEDED(hr = wined3d_device_reset(device->wined3d_device, &swapchain_desc,
            mode ? &wined3d_mode : NULL, reset_enum_callback, !extended)))
    {
        struct d3d9_surface *surface;

        free(device->implicit_swapchains);

        if (!extended)
        {
            device->auto_mipmaps = 0;
            wined3d_stateblock_set_render_state(device->state, WINED3D_RS_ZENABLE,
                    !!swapchain_desc.enable_auto_depth_stencil);
            device_reset_viewport_state(device);
        }

        if (FAILED(hr = d3d9_device_get_swapchains(device)))
        {
            device->device_state = D3D9_DEVICE_STATE_NOT_RESET;
        }
        else
        {
            d3d9_swapchain = wined3d_swapchain_get_parent(device->implicit_swapchains[0]);
            d3d9_swapchain->swap_interval
                    = wined3dswapinterval_from_d3d(present_parameters->PresentationInterval);
            wined3d_swapchain_get_desc(d3d9_swapchain->wined3d_swapchain, &swapchain_desc);
            present_parameters->BackBufferWidth = swapchain_desc.backbuffer_width;
            present_parameters->BackBufferHeight = swapchain_desc.backbuffer_height;
            present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(swapchain_desc.backbuffer_format);
            present_parameters->BackBufferCount = swapchain_desc.backbuffer_count;

            if (extended)
            {
                for (i = 0; i < old_swapchain_desc.backbuffer_count; ++i)
                {
                    surface = old_backbuffers[i];
                    /* We have a reference to this surface, so we can assert that it's
                     * currently holding a reference to the swapchain. */
                    wined3d_swapchain_decref(surface->swapchain);
                    surface->swapchain = NULL;
                    surface->container = (IUnknown *)&device->IDirect3DDevice9Ex_iface;
                }
            }

            /* FIXME: This should be the new backbuffer count, but we don't support
             * changing the backbuffer count in wined3d yet. */
            for (i = 0; i < old_swapchain_desc.backbuffer_count; ++i)
            {
                struct wined3d_texture *backbuffer = wined3d_swapchain_get_back_buffer(d3d9_swapchain->wined3d_swapchain, i);

                if ((surface = d3d9_surface_create(backbuffer, 0, (IUnknown *)&device->IDirect3DDevice9Ex_iface)))
                    surface->parent_device = &device->IDirect3DDevice9Ex_iface;
            }

            device->in_scene = FALSE;
            device->device_state = D3D9_DEVICE_STATE_OK;

            if (extended)
            {
                const struct wined3d_viewport *current = &device->stateblock_state->viewport;
                struct wined3d_viewport vp;
                RECT rect;

                vp.x = 0;
                vp.y = 0;
                vp.width = swapchain_desc.backbuffer_width;
                vp.height = swapchain_desc.backbuffer_height;
                vp.min_z = current->min_z;
                vp.max_z = current->max_z;
                wined3d_stateblock_set_viewport(device->state, &vp);

                SetRect(&rect, 0, 0, swapchain_desc.backbuffer_width, swapchain_desc.backbuffer_height);
                wined3d_stateblock_set_scissor_rect(device->state, &rect);
            }
        }

        rtv = wined3d_device_context_get_rendertarget_view(device->immediate_context, 0);
        device->render_targets[0] = wined3d_rendertarget_view_get_sub_resource_parent(rtv);
        for (i = 1; i < ARRAY_SIZE(device->render_targets); ++i)
            device->render_targets[i] = NULL;

        if ((rtv = wined3d_device_context_get_depth_stencil_view(device->immediate_context)))
        {
            struct wined3d_resource *resource = wined3d_rendertarget_view_get_resource(rtv);

            if ((surface = d3d9_surface_create(wined3d_texture_from_resource(resource), 0,
                    (IUnknown *)&device->IDirect3DDevice9Ex_iface)))
                surface->parent_device = &device->IDirect3DDevice9Ex_iface;
        }
    }
    else if (!extended)
    {
        device->device_state = D3D9_DEVICE_STATE_NOT_RESET;
    }

    if (extended)
    {
        for (i = 0; i < old_swapchain_desc.backbuffer_count; ++i)
            IDirect3DSurface9_Release(&old_backbuffers[i]->IDirect3DSurface9_iface);
        free(old_backbuffers);
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_Reset(IDirect3DDevice9Ex *iface,
        D3DPRESENT_PARAMETERS *present_parameters)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, present_parameters %p.\n", iface, present_parameters);

    return d3d9_device_reset(device, present_parameters, NULL);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_Present(IDirect3DDevice9Ex *iface,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override, const RGNDATA *dirty_region)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_swapchain *swapchain;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p.\n",
            iface, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect), dst_window_override, dirty_region);

    if (device->device_state != D3D9_DEVICE_STATE_OK)
        return device->d3d_parent->extended ? S_PRESENT_OCCLUDED : D3DERR_DEVICELOST;

    if (dirty_region)
        FIXME("Ignoring dirty_region %p.\n", dirty_region);

    wined3d_mutex_lock();
    for (i = 0; i < device->implicit_swapchain_count; ++i)
    {
        swapchain = wined3d_swapchain_get_parent(device->implicit_swapchains[i]);
        if (FAILED(hr = wined3d_swapchain_present(swapchain->wined3d_swapchain,
                src_rect, dst_rect, dst_window_override, swapchain->swap_interval, 0)))
        {
            wined3d_mutex_unlock();
            return hr;
        }
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetBackBuffer(IDirect3DDevice9Ex *iface, UINT swapchain,
        UINT backbuffer_idx, D3DBACKBUFFER_TYPE backbuffer_type, IDirect3DSurface9 **backbuffer)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_swapchain *d3d9_swapchain;
    HRESULT hr;

    TRACE("iface %p, swapchain %u, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, swapchain, backbuffer_idx, backbuffer_type, backbuffer);

    /* backbuffer_type is ignored by native. */

    /* No need to check for backbuffer == NULL, Windows crashes in that case. */
    *backbuffer = NULL;

    wined3d_mutex_lock();
    if (swapchain >= device->implicit_swapchain_count)
    {
        wined3d_mutex_unlock();
        WARN("Swapchain index %u is out of range, returning D3DERR_INVALIDCALL.\n", swapchain);
        return D3DERR_INVALIDCALL;
    }

    d3d9_swapchain = wined3d_swapchain_get_parent(device->implicit_swapchains[swapchain]);
    hr = IDirect3DSwapChain9Ex_GetBackBuffer(&d3d9_swapchain->IDirect3DSwapChain9Ex_iface,
            backbuffer_idx, backbuffer_type, backbuffer);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetRasterStatus(IDirect3DDevice9Ex *iface,
        UINT swapchain, D3DRASTER_STATUS *raster_status)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, swapchain %u, raster_status %p.\n", iface, swapchain, raster_status);

    wined3d_mutex_lock();
    hr = wined3d_device_get_raster_status(device->wined3d_device,
            swapchain, (struct wined3d_raster_status *)raster_status);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetDialogBoxMode(IDirect3DDevice9Ex *iface, BOOL enable)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, enable %#x.\n", iface, enable);

    wined3d_mutex_lock();
    hr = wined3d_device_set_dialog_box_mode(device->wined3d_device, enable);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI d3d9_device_SetGammaRamp(IDirect3DDevice9Ex *iface,
        UINT swapchain, DWORD flags, const D3DGAMMARAMP *ramp)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, swapchain %u, flags %#lx, ramp %p.\n", iface, swapchain, flags, ramp);

    /* Note: D3DGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
    wined3d_mutex_lock();
    wined3d_device_set_gamma_ramp(device->wined3d_device, swapchain, flags, (const struct wined3d_gamma_ramp *)ramp);
    wined3d_mutex_unlock();
}

static void WINAPI d3d9_device_GetGammaRamp(IDirect3DDevice9Ex *iface, UINT swapchain, D3DGAMMARAMP *ramp)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, swapchain %u, ramp %p.\n", iface, swapchain, ramp);

    /* Note: D3DGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
    wined3d_mutex_lock();
    wined3d_device_get_gamma_ramp(device->wined3d_device, swapchain, (struct wined3d_gamma_ramp *)ramp);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI d3d9_device_CreateTexture(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DTexture9 **texture, HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_texture *object;
    BOOL set_mem = FALSE;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, levels %u, usage %#lx, format %#x, pool %#x, texture %p, shared_handle %p.\n",
            iface, width, height, levels, usage, format, pool, texture, shared_handle);

    *texture = NULL;
    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared or user memory texture on a non-ex device.\n");
            return E_NOTIMPL;
        }

        if (pool == D3DPOOL_SYSTEMMEM)
        {
            if (levels != 1)
                return D3DERR_INVALIDCALL;
            set_mem = TRUE;
        }
        else
        {
            if (pool != D3DPOOL_DEFAULT)
            {
                WARN("Trying to create a shared texture in pool %#x.\n", pool);
                return D3DERR_INVALIDCALL;
            }
            FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
        }
    }

    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = d3d9_texture_2d_init(object, device, width, height, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize texture, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    if (set_mem)
    {
        wined3d_mutex_lock();
        wined3d_texture_update_desc(object->wined3d_texture, 0, *shared_handle, 0);
        wined3d_mutex_unlock();
    }

    levels = wined3d_texture_get_level_count(object->wined3d_texture);
    for (i = 0; i < levels; ++i)
    {
        if (!d3d9_surface_create(object->wined3d_texture, i, (IUnknown *)&object->IDirect3DBaseTexture9_iface))
        {
            IDirect3DTexture9_Release(&object->IDirect3DBaseTexture9_iface);
            return E_OUTOFMEMORY;
        }
    }

    TRACE("Created texture %p.\n", object);
    *texture = (IDirect3DTexture9 *)&object->IDirect3DBaseTexture9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_CreateVolumeTexture(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DVolumeTexture9 **texture, HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_texture *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, depth %u, levels %u, "
            "usage %#lx, format %#x, pool %#x, texture %p, shared_handle %p.\n",
            iface, width, height, depth, levels,
            usage, format, pool, texture, shared_handle);

    *texture = NULL;
    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared volume texture on a non-ex device.\n");
            return E_NOTIMPL;
        }

        if (pool != D3DPOOL_DEFAULT)
        {
            WARN("Trying to create a shared volume texture in pool %#x.\n", pool);
            return D3DERR_INVALIDCALL;
        }
        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
    }

    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = d3d9_texture_3d_init(object, device, width, height, depth, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize volume texture, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created volume texture %p.\n", object);
    *texture = (IDirect3DVolumeTexture9 *)&object->IDirect3DBaseTexture9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_CreateCubeTexture(IDirect3DDevice9Ex *iface,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool,
        IDirect3DCubeTexture9 **texture, HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_texture *object;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, edge_length %u, levels %u, usage %#lx, format %#x, pool %#x, texture %p, shared_handle %p.\n",
            iface, edge_length, levels, usage, format, pool, texture, shared_handle);

    *texture = NULL;
    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared cube texture on a non-ex device.\n");
            return E_NOTIMPL;
        }

        if (pool != D3DPOOL_DEFAULT)
        {
            WARN("Trying to create a shared cube texture in pool %#x.\n", pool);
            return D3DERR_INVALIDCALL;
        }
        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
    }

    if (!(object = calloc(1, sizeof(*object))))
        return D3DERR_OUTOFVIDEOMEMORY;

    hr = d3d9_texture_cube_init(object, device, edge_length, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize cube texture, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    levels = wined3d_texture_get_level_count(object->wined3d_texture);
    for (i = 0; i < levels * 6; ++i)
    {
        if (!d3d9_surface_create(object->wined3d_texture, i, (IUnknown *)&object->IDirect3DBaseTexture9_iface))
        {
            IDirect3DTexture9_Release(&object->IDirect3DBaseTexture9_iface);
            return E_OUTOFMEMORY;
        }
    }

    TRACE("Created cube texture %p.\n", object);
    *texture = (IDirect3DCubeTexture9 *)&object->IDirect3DBaseTexture9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_CreateVertexBuffer(IDirect3DDevice9Ex *iface, UINT size,
        DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9 **buffer,
        HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertexbuffer *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#lx, fvf %#lx, pool %#x, buffer %p, shared_handle %p.\n",
            iface, size, usage, fvf, pool, buffer, shared_handle);

    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared vertex buffer on a non-ex device.\n");
            return E_NOTIMPL;
        }

        if (pool != D3DPOOL_DEFAULT)
        {
            WARN("Trying to create a shared vertex buffer in pool %#x.\n", pool);
            return D3DERR_NOTAVAILABLE;
        }
        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
    }

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
    *buffer = &object->IDirect3DVertexBuffer9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_CreateIndexBuffer(IDirect3DDevice9Ex *iface, UINT size,
        DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **buffer,
        HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_indexbuffer *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#lx, format %#x, pool %#x, buffer %p, shared_handle %p.\n",
            iface, size, usage, format, pool, buffer, shared_handle);

    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared index buffer on a non-ex device.\n");
            return E_NOTIMPL;
        }

        if (pool != D3DPOOL_DEFAULT)
        {
            WARN("Trying to create a shared index buffer in pool %#x.\n", pool);
            return D3DERR_NOTAVAILABLE;
        }
        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
    }

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
    *buffer = &object->IDirect3DIndexBuffer9_iface;

    return D3D_OK;
}

static HRESULT d3d9_device_create_surface(struct d3d9_device *device, unsigned int flags,
        enum wined3d_format_id format, enum wined3d_multisample_type multisample_type,
        unsigned int multisample_quality, unsigned int usage, unsigned int bind_flags, unsigned int access,
        unsigned int width, unsigned int height, void *user_mem, IDirect3DSurface9 **surface)
{
    struct wined3d_resource_desc desc;
    struct d3d9_surface *surface_impl;
    struct wined3d_texture *texture;
    HRESULT hr;

    TRACE("device %p, flags %#x, format %#x, multisample_type %#x, multisample_quality %u, "
            "usage %#x, bind_flags %#x, access %#x, width %u, height %u, user_mem %p, surface %p.\n",
            device, flags, format, multisample_type, multisample_quality, usage,
            bind_flags, access, width, height, user_mem, surface);

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = format;
    desc.multisample_type = multisample_type;
    desc.multisample_quality = multisample_quality;
    desc.usage = usage;
    desc.bind_flags = bind_flags;
    desc.access = access;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.size = 0;

    if (is_gdi_compat_wined3dformat(desc.format))
        flags |= WINED3D_TEXTURE_CREATE_GET_DC;

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &desc,
            1, 1, flags, NULL, NULL, &d3d9_null_wined3d_parent_ops, &texture)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to create texture, hr %#lx.\n", hr);
        if (hr == WINED3DERR_NOTAVAILABLE)
            hr = D3DERR_INVALIDCALL;
        return hr;
    }

    if (!(surface_impl = d3d9_surface_create(texture, 0, NULL)))
    {
        wined3d_texture_decref(texture);
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }
    surface_impl->parent_device = &device->IDirect3DDevice9Ex_iface;
    *surface = &surface_impl->IDirect3DSurface9_iface;
    IDirect3DSurface9_AddRef(*surface);

    if (user_mem)
        wined3d_texture_update_desc(texture, 0, user_mem, 0);

    wined3d_texture_decref(texture);

    wined3d_mutex_unlock();

    return D3D_OK;
}

BOOL is_gdi_compat_wined3dformat(enum wined3d_format_id format)
{
    switch (format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
        case WINED3DFMT_B8G8R8X8_UNORM:
        case WINED3DFMT_B5G6R5_UNORM:
        case WINED3DFMT_B5G5R5X1_UNORM:
        case WINED3DFMT_B5G5R5A1_UNORM:
        case WINED3DFMT_B8G8R8_UNORM:
            return TRUE;
        default:
            return FALSE;
    }
}

static HRESULT WINAPI d3d9_device_CreateRenderTarget(IDirect3DDevice9Ex *iface, UINT width, UINT height,
        D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality,
        BOOL lockable, IDirect3DSurface9 **surface, HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    unsigned int access = WINED3D_RESOURCE_ACCESS_GPU;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %lu.\n"
            "lockable %#x, surface %p, shared_handle %p.\n",
            iface, width, height, format, multisample_type, multisample_quality,
            lockable, surface, shared_handle);

    *surface = NULL;
    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared render target on a non-ex device.\n");
            return E_NOTIMPL;
        }

        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
    }

    if (lockable)
        access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;

    return d3d9_device_create_surface(device, 0, wined3dformat_from_d3dformat(format),
            wined3d_multisample_type_from_d3d(multisample_type), multisample_quality, 0,
            WINED3D_BIND_RENDER_TARGET, access, width, height, NULL, surface);
}

static HRESULT WINAPI d3d9_device_CreateDepthStencilSurface(IDirect3DDevice9Ex *iface, UINT width, UINT height,
        D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality,
        BOOL discard, IDirect3DSurface9 **surface, HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    DWORD flags = 0;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %lu.\n"
            "discard %#x, surface %p, shared_handle %p.\n",
            iface, width, height, format, multisample_type, multisample_quality,
            discard, surface, shared_handle);

    *surface = NULL;
    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared depth stencil on a non-ex device.\n");
            return E_NOTIMPL;
        }

        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
    }

    if (discard)
        flags |= WINED3D_TEXTURE_CREATE_DISCARD;

    return d3d9_device_create_surface(device, flags, wined3dformat_from_d3dformat(format),
            wined3d_multisample_type_from_d3d(multisample_type), multisample_quality, 0,
            WINED3D_BIND_DEPTH_STENCIL, WINED3D_RESOURCE_ACCESS_GPU, width, height, NULL, surface);
}


static HRESULT WINAPI d3d9_device_UpdateSurface(IDirect3DDevice9Ex *iface,
        IDirect3DSurface9 *src_surface, const RECT *src_rect,
        IDirect3DSurface9 *dst_surface, const POINT *dst_point)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *src = unsafe_impl_from_IDirect3DSurface9(src_surface);
    struct d3d9_surface *dst = unsafe_impl_from_IDirect3DSurface9(dst_surface);
    struct wined3d_sub_resource_desc src_desc, dst_desc;
    struct wined3d_box src_box;
    HRESULT hr;

    TRACE("iface %p, src_surface %p, src_rect %s, dst_surface %p, dst_point %p.\n",
            iface, src_surface, wine_dbgstr_rect(src_rect), dst_surface, dst_point);

    wined3d_mutex_lock();

    wined3d_texture_get_sub_resource_desc(src->wined3d_texture, src->sub_resource_idx, &src_desc);
    wined3d_texture_get_sub_resource_desc(dst->wined3d_texture, dst->sub_resource_idx, &dst_desc);
    if (src_desc.format != dst_desc.format)
    {
        wined3d_mutex_unlock();
        WARN("Surface formats (%#x/%#x) don't match.\n",
                d3dformat_from_wined3dformat(src_desc.format),
                d3dformat_from_wined3dformat(dst_desc.format));
        return D3DERR_INVALIDCALL;
    }

    if (src_desc.multisample_type != WINED3D_MULTISAMPLE_NONE || dst_desc.multisample_type != WINED3D_MULTISAMPLE_NONE)
    {
        wined3d_mutex_unlock();
        WARN("Cannot use UpdateSurface with multisampled surfaces.\n");
        return D3DERR_INVALIDCALL;
    }

    if (src_rect)
        wined3d_box_set(&src_box, src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1);
    else
        wined3d_box_set(&src_box, 0, 0, src_desc.width, src_desc.height, 0, 1);

    hr = wined3d_device_context_copy_sub_resource_region(device->immediate_context,
            wined3d_texture_get_resource(dst->wined3d_texture), dst->sub_resource_idx, dst_point ? dst_point->x : 0,
            dst_point ? dst_point->y : 0, 0, wined3d_texture_get_resource(src->wined3d_texture),
            src->sub_resource_idx, &src_box, 0);
    if (SUCCEEDED(hr) && dst->texture)
        d3d9_texture_flag_auto_gen_mipmap(dst->texture);

    wined3d_mutex_unlock();

    if (FAILED(hr))
        return D3DERR_INVALIDCALL;

    return hr;
}

static HRESULT WINAPI d3d9_device_UpdateTexture(IDirect3DDevice9Ex *iface,
        IDirect3DBaseTexture9 *src_texture, IDirect3DBaseTexture9 *dst_texture)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_texture *src_impl, *dst_impl;
    HRESULT hr;

    TRACE("iface %p, src_texture %p, dst_texture %p.\n", iface, src_texture, dst_texture);

    src_impl = unsafe_impl_from_IDirect3DBaseTexture9(src_texture);
    dst_impl = unsafe_impl_from_IDirect3DBaseTexture9(dst_texture);

    if (src_impl->draw_texture || dst_impl->draw_texture)
    {
        WARN("Source or destination is managed; returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_update_texture(device->wined3d_device,
            src_impl->wined3d_texture, dst_impl->wined3d_texture);
    if (SUCCEEDED(hr))
        d3d9_texture_flag_auto_gen_mipmap(dst_impl);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetRenderTargetData(IDirect3DDevice9Ex *iface,
        IDirect3DSurface9 *render_target, IDirect3DSurface9 *dst_surface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *rt_impl = unsafe_impl_from_IDirect3DSurface9(render_target);
    struct d3d9_surface *dst_impl = unsafe_impl_from_IDirect3DSurface9(dst_surface);
    struct wined3d_sub_resource_desc wined3d_desc;
    RECT dst_rect, src_rect;
    HRESULT hr;

    TRACE("iface %p, render_target %p, dst_surface %p.\n", iface, render_target, dst_surface);

    if (!render_target || !dst_surface)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    wined3d_texture_get_sub_resource_desc(dst_impl->wined3d_texture, dst_impl->sub_resource_idx, &wined3d_desc);
    SetRect(&dst_rect, 0, 0, wined3d_desc.width, wined3d_desc.height);

    wined3d_texture_get_sub_resource_desc(rt_impl->wined3d_texture, rt_impl->sub_resource_idx, &wined3d_desc);
    SetRect(&src_rect, 0, 0, wined3d_desc.width, wined3d_desc.height);

    /* TODO: Check surface sizes, pools, etc. */
    if (wined3d_desc.multisample_type)
        hr = D3DERR_INVALIDCALL;
    else
        hr = wined3d_device_context_blt(device->immediate_context,
                dst_impl->wined3d_texture, dst_impl->sub_resource_idx, &dst_rect,
                rt_impl->wined3d_texture, rt_impl->sub_resource_idx, &src_rect, 0, NULL, WINED3D_TEXF_POINT);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetFrontBufferData(IDirect3DDevice9Ex *iface,
        UINT swapchain, IDirect3DSurface9 *dst_surface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *dst_impl = unsafe_impl_from_IDirect3DSurface9(dst_surface);
    HRESULT hr = D3DERR_INVALIDCALL;

    TRACE("iface %p, swapchain %u, dst_surface %p.\n", iface, swapchain, dst_surface);

    wined3d_mutex_lock();
    if (swapchain < device->implicit_swapchain_count)
        hr = wined3d_swapchain_get_front_buffer_data(device->implicit_swapchains[swapchain],
                dst_impl->wined3d_texture, dst_impl->sub_resource_idx);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_StretchRect(IDirect3DDevice9Ex *iface, IDirect3DSurface9 *src_surface,
        const RECT *src_rect, IDirect3DSurface9 *dst_surface, const RECT *dst_rect, D3DTEXTUREFILTERTYPE filter)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *src = unsafe_impl_from_IDirect3DSurface9(src_surface);
    struct d3d9_surface *dst = unsafe_impl_from_IDirect3DSurface9(dst_surface);
    struct wined3d_sub_resource_desc src_desc, dst_desc;
    HRESULT hr = D3DERR_INVALIDCALL;
    RECT d, s;

    TRACE("iface %p, src_surface %p, src_rect %s, dst_surface %p, dst_rect %s, filter %#x.\n",
            iface, src_surface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), filter);

    wined3d_mutex_lock();
    wined3d_texture_get_sub_resource_desc(dst->wined3d_texture, dst->sub_resource_idx, &dst_desc);
    if (!dst_rect)
    {
        SetRect(&d, 0, 0, dst_desc.width, dst_desc.height);
        dst_rect = &d;
    }

    wined3d_texture_get_sub_resource_desc(src->wined3d_texture, src->sub_resource_idx, &src_desc);
    if (!src_rect)
    {
        SetRect(&s, 0, 0, src_desc.width, src_desc.height);
        src_rect = &s;
    }

    if (dst_desc.access & WINED3D_RESOURCE_ACCESS_CPU)
    {
        WARN("Destination resource is not in DEFAULT pool.\n");
        goto done;
    }
    if (src_desc.access & WINED3D_RESOURCE_ACCESS_CPU)
    {
        WARN("Source resource is not in DEFAULT pool.\n");
        goto done;
    }

    if (dst->texture && !(dst_desc.bind_flags & (WINED3D_BIND_RENDER_TARGET | WINED3D_BIND_DEPTH_STENCIL)))
    {
        WARN("Destination is a regular texture.\n");
        goto done;
    }

    if (src_desc.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        if (device->in_scene)
        {
            WARN("Rejecting depth / stencil blit while in scene.\n");
            goto done;
        }

        if (src_rect->left || src_rect->top || src_rect->right != src_desc.width
                || src_rect->bottom != src_desc.height)
        {
            WARN("Rejecting depth / stencil blit with invalid source rect %s.\n",
                    wine_dbgstr_rect(src_rect));
            goto done;
        }

        if (dst_rect->left || dst_rect->top || dst_rect->right != dst_desc.width
                || dst_rect->bottom != dst_desc.height)
        {
            WARN("Rejecting depth / stencil blit with invalid destination rect %s.\n",
                    wine_dbgstr_rect(dst_rect));
            goto done;
        }

        if (src_desc.width != dst_desc.width || src_desc.height != dst_desc.height)
        {
            WARN("Rejecting depth / stencil blit with mismatched surface sizes.\n");
            goto done;
        }
    }

    hr = wined3d_device_context_blt(device->immediate_context, dst->wined3d_texture,
            dst->sub_resource_idx, dst_rect, src->wined3d_texture, src->sub_resource_idx,
            src_rect, 0, NULL, wined3d_texture_filter_type_from_d3d(filter));
    if (hr == WINEDDERR_INVALIDRECT)
        hr = D3DERR_INVALIDCALL;
    if (SUCCEEDED(hr) && dst->texture)
        d3d9_texture_flag_auto_gen_mipmap(dst->texture);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d9_device_ColorFill(IDirect3DDevice9Ex *iface,
        IDirect3DSurface9 *surface, const RECT *rect, D3DCOLOR color)
{
    const struct wined3d_color c =
    {
        ((color >> 16) & 0xff) / 255.0f,
        ((color >>  8) & 0xff) / 255.0f,
        (color & 0xff) / 255.0f,
        ((color >> 24) & 0xff) / 255.0f,
    };
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *surface_impl = unsafe_impl_from_IDirect3DSurface9(surface);
    struct wined3d_sub_resource_desc desc;
    struct wined3d_rendertarget_view *rtv;
    HRESULT hr;

    TRACE("iface %p, surface %p, rect %p, color 0x%08lx.\n", iface, surface, rect, color);

    if (!surface)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();

    if (FAILED(wined3d_texture_get_sub_resource_desc(surface_impl->wined3d_texture,
            surface_impl->sub_resource_idx, &desc)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    if (desc.access & WINED3D_RESOURCE_ACCESS_CPU)
    {
        wined3d_mutex_unlock();
        WARN("Colour fills are not allowed on surfaces with resource access %#x.\n", desc.access);
        return D3DERR_INVALIDCALL;
    }
    if ((desc.bind_flags & (WINED3D_BIND_RENDER_TARGET | WINED3D_BIND_SHADER_RESOURCE)) == WINED3D_BIND_SHADER_RESOURCE)
    {
        wined3d_mutex_unlock();
        WARN("Colorfill is not allowed on non-RT textures, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    if (desc.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        wined3d_mutex_unlock();
        WARN("Colorfill is not allowed on depth stencil surfaces, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_stateblock_apply_clear_state(device->state, device->wined3d_device);
    rtv = d3d9_surface_acquire_rendertarget_view(surface_impl);
    hr = wined3d_device_context_clear_rendertarget_view(device->immediate_context,
            rtv, rect, WINED3DCLEAR_TARGET, &c, 0.0f, 0);
    d3d9_surface_release_rendertarget_view(surface_impl, rtv);
    if (SUCCEEDED(hr) && surface_impl->texture)
        d3d9_texture_flag_auto_gen_mipmap(surface_impl->texture);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_CreateOffscreenPlainSurface(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface,
        HANDLE *shared_handle)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    unsigned int usage, access;
    void *user_mem = NULL;

    TRACE("iface %p, width %u, height %u, format %#x, pool %#x, surface %p, shared_handle %p.\n",
            iface, width, height, format, pool, surface, shared_handle);

    *surface = NULL;
    if (pool == D3DPOOL_MANAGED)
    {
        WARN("Attempting to create a managed offscreen plain surface.\n");
        return D3DERR_INVALIDCALL;
    }

    if (shared_handle)
    {
        if (!device->d3d_parent->extended)
        {
            WARN("Trying to create a shared or user memory surface on a non-ex device.\n");
            return E_NOTIMPL;
        }

        if (pool == D3DPOOL_SYSTEMMEM)
            user_mem = *shared_handle;
        else
        {
            if (pool != D3DPOOL_DEFAULT)
            {
                WARN("Trying to create a shared surface in pool %#x.\n", pool);
                return D3DERR_INVALIDCALL;
            }
            FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);
        }
    }

    usage = wined3d_usage_from_d3d(pool, 0);
    access = wined3daccess_from_d3dpool(pool, 0)
            | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;

    if (!device->d3d_parent->extended)
        usage |= WINED3DUSAGE_VIDMEM_ACCOUNTING;

    return d3d9_device_create_surface(device, 0, wined3dformat_from_d3dformat(format),
            WINED3D_MULTISAMPLE_NONE, 0, usage, 0, access, width, height, user_mem, surface);
}

static HRESULT WINAPI d3d9_device_SetRenderTarget(IDirect3DDevice9Ex *iface, DWORD idx, IDirect3DSurface9 *surface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *surface_impl = unsafe_impl_from_IDirect3DSurface9(surface);
    struct wined3d_rendertarget_view *rtv;
    HRESULT hr;

    TRACE("iface %p, idx %lu, surface %p.\n", iface, idx, surface);

    if (idx >= D3D_MAX_SIMULTANEOUS_RENDERTARGETS)
    {
        WARN("Invalid index %lu specified.\n", idx);
        return D3DERR_INVALIDCALL;
    }

    if (!idx && !surface_impl)
    {
        WARN("Trying to set render target 0 to NULL.\n");
        return D3DERR_INVALIDCALL;
    }

    if (surface_impl && d3d9_surface_get_device(surface_impl) != device)
    {
        WARN("Render target surface does not match device.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    rtv = surface_impl ? d3d9_surface_acquire_rendertarget_view(surface_impl) : NULL;
    hr = wined3d_device_context_set_rendertarget_views(device->immediate_context, idx, 1, &rtv, TRUE);
    d3d9_surface_release_rendertarget_view(surface_impl, rtv);
    if (SUCCEEDED(hr))
    {
        if (!idx)
            device_reset_viewport_state(device);
        device->render_targets[idx] = surface_impl;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetRenderTarget(IDirect3DDevice9Ex *iface, DWORD idx, IDirect3DSurface9 **surface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_rendertarget_view *wined3d_rtv;
    struct d3d9_surface *surface_impl;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, idx %lu, surface %p.\n", iface, idx, surface);

    if (!surface)
        return D3DERR_INVALIDCALL;

    if (idx >= D3D_MAX_SIMULTANEOUS_RENDERTARGETS)
    {
        WARN("Invalid index %lu specified.\n", idx);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if ((wined3d_rtv = wined3d_device_context_get_rendertarget_view(device->immediate_context, idx)))
    {
        /* We want the sub resource parent here, since the view itself may be
         * internal to wined3d and may not have a parent. */
        surface_impl = wined3d_rendertarget_view_get_sub_resource_parent(wined3d_rtv);
        *surface = &surface_impl->IDirect3DSurface9_iface;
        IDirect3DSurface9_AddRef(*surface);
    }
    else
    {
        hr = D3DERR_NOTFOUND;
        *surface = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetDepthStencilSurface(IDirect3DDevice9Ex *iface, IDirect3DSurface9 *depth_stencil)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_surface *ds_impl = unsafe_impl_from_IDirect3DSurface9(depth_stencil);
    struct wined3d_rendertarget_view *rtv;
    HRESULT hr;

    TRACE("iface %p, depth_stencil %p.\n", iface, depth_stencil);

    wined3d_mutex_lock();
    rtv = ds_impl ? d3d9_surface_acquire_rendertarget_view(ds_impl) : NULL;
    hr = wined3d_device_context_set_depth_stencil_view(device->immediate_context, rtv);
    wined3d_stateblock_depth_buffer_changed(device->state);
    d3d9_surface_release_rendertarget_view(ds_impl, rtv);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetDepthStencilSurface(IDirect3DDevice9Ex *iface, IDirect3DSurface9 **depth_stencil)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_rendertarget_view *wined3d_dsv;
    struct d3d9_surface *surface_impl;
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
        *depth_stencil = &surface_impl->IDirect3DSurface9_iface;
        IDirect3DSurface9_AddRef(*depth_stencil);
    }
    else
    {
        hr = D3DERR_NOTFOUND;
        *depth_stencil = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_BeginScene(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    if (SUCCEEDED(hr = wined3d_device_begin_scene(device->wined3d_device)))
        device->in_scene = TRUE;
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_EndScene(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    if (SUCCEEDED(hr = wined3d_device_end_scene(device->wined3d_device)))
        device->in_scene = FALSE;
    wined3d_mutex_unlock();

    return hr;
}

static void d3d9_rts_flag_auto_gen_mipmap(struct d3d9_device *device)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(device->render_targets); ++i)
    {
        struct d3d9_surface *surface = device->render_targets[i];

        if (surface && surface->texture)
            d3d9_texture_flag_auto_gen_mipmap(surface->texture);
    }
}

static HRESULT WINAPI d3d9_device_Clear(IDirect3DDevice9Ex *iface, DWORD rect_count,
        const D3DRECT *rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_color c;
    HRESULT hr;

    TRACE("iface %p, rect_count %lu, rects %p, flags %#lx, color 0x%08lx, z %.8e, stencil %lu.\n",
            iface, rect_count, rects, flags, color, z, stencil);

    if (rect_count && !rects)
    {
        WARN("count %lu with NULL rects.\n", rect_count);
        rect_count = 0;
    }

    wined3d_color_from_d3dcolor(&c, color);
    wined3d_mutex_lock();
    wined3d_stateblock_apply_clear_state(device->state, device->wined3d_device);
    hr = wined3d_device_clear(device->wined3d_device, rect_count, (const RECT *)rects, flags, &c, z, stencil);
    if (SUCCEEDED(hr))
        d3d9_rts_flag_auto_gen_mipmap(device);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetTransform(IDirect3DDevice9Ex *iface,
        D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    wined3d_stateblock_set_transform(device->update_state,
            wined3d_transform_state_from_d3d(state), (const struct wined3d_matrix *)matrix);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetTransform(IDirect3DDevice9Ex *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    memcpy(matrix, &device->stateblock_state->transforms[state], sizeof(*matrix));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_MultiplyTransform(IDirect3DDevice9Ex *iface,
        D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    wined3d_stateblock_multiply_transform(device->state,
            wined3d_transform_state_from_d3d(state), (const struct wined3d_matrix *)matrix);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetViewport(IDirect3DDevice9Ex *iface, const D3DVIEWPORT9 *viewport)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_viewport vp;

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    vp.x = viewport->X;
    vp.y = viewport->Y;
    vp.width = viewport->Width;
    vp.height = viewport->Height;
    vp.min_z = viewport->MinZ;
    vp.max_z = viewport->MaxZ;

    wined3d_mutex_lock();
    wined3d_stateblock_set_viewport(device->update_state, &vp);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetViewport(IDirect3DDevice9Ex *iface, D3DVIEWPORT9 *viewport)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
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

static HRESULT WINAPI d3d9_device_SetMaterial(IDirect3DDevice9Ex *iface, const D3DMATERIAL9 *material)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    /* Note: D3DMATERIAL9 is compatible with struct wined3d_material. */
    wined3d_mutex_lock();
    wined3d_stateblock_set_material(device->update_state, (const struct wined3d_material *)material);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetMaterial(IDirect3DDevice9Ex *iface, D3DMATERIAL9 *material)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    /* Note: D3DMATERIAL9 is compatible with struct wined3d_material. */
    wined3d_mutex_lock();
    memcpy(material, &device->stateblock_state->material, sizeof(*material));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetLight(IDirect3DDevice9Ex *iface, DWORD index, const D3DLIGHT9 *light)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, light %p.\n", iface, index, light);

    /* Note: D3DLIGHT9 is compatible with struct wined3d_light. */
    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_light(device->update_state, index, (const struct wined3d_light *)light);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetLight(IDirect3DDevice9Ex *iface, DWORD index, D3DLIGHT9 *light)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    BOOL enabled;
    HRESULT hr;

    TRACE("iface %p, index %lu, light %p.\n", iface, index, light);

    /* Note: D3DLIGHT9 is compatible with struct wined3d_light. */
    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_light(device->state, index, (struct wined3d_light *)light, &enabled);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_LightEnable(IDirect3DDevice9Ex *iface, DWORD index, BOOL enable)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, enable %#x.\n", iface, index, enable);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_light_enable(device->update_state, index, enable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetLightEnable(IDirect3DDevice9Ex *iface, DWORD index, BOOL *enabled)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_light light;
    HRESULT hr;

    TRACE("iface %p, index %lu, enabled %p.\n", iface, index, enabled);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_light(device->state, index, &light, enabled);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetClipPlane(IDirect3DDevice9Ex *iface, DWORD index, const float *plane)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, plane %p.\n", iface, index, plane);

    index = min(index, device->max_user_clip_planes - 1);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_clip_plane(device->update_state, index, (const struct wined3d_vec4 *)plane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetClipPlane(IDirect3DDevice9Ex *iface, DWORD index, float *plane)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, index %lu, plane %p.\n", iface, index, plane);

    index = min(index, device->max_user_clip_planes - 1);

    wined3d_mutex_lock();
    memcpy(plane, &device->stateblock_state->clip_planes[index], sizeof(struct wined3d_vec4));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static void resolve_depth_buffer(struct d3d9_device *device)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    struct wined3d_rendertarget_view *wined3d_dsv;
    struct wined3d_resource *dst_resource;
    struct wined3d_texture *dst_texture;
    struct wined3d_resource_desc desc;
    struct d3d9_surface *d3d9_dsv;

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
    d3d9_dsv = wined3d_rendertarget_view_get_sub_resource_parent(wined3d_dsv);

    wined3d_device_context_resolve_sub_resource(device->immediate_context, dst_resource, 0,
            wined3d_rendertarget_view_get_resource(wined3d_dsv), d3d9_dsv->sub_resource_idx, desc.format);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_SetRenderState(IDirect3DDevice9Ex *iface,
        D3DRENDERSTATETYPE state, DWORD value)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, state %#x, value %#lx.\n", iface, state, value);

    wined3d_mutex_lock();
    wined3d_stateblock_set_render_state(device->update_state, wined3d_render_state_from_d3d(state), value);
    if (state == D3DRS_POINTSIZE && value == D3D9_RESZ_CODE)
        resolve_depth_buffer(device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetRenderState(IDirect3DDevice9Ex *iface,
        D3DRENDERSTATETYPE state, DWORD *value)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    const struct wined3d_stateblock_state *device_state;

    TRACE("iface %p, state %#x, value %p.\n", iface, state, value);

    wined3d_mutex_lock();
    device_state = device->stateblock_state;
    *value = device_state->rs[state];
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_CreateStateBlock(IDirect3DDevice9Ex *iface,
        D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9 **stateblock)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_stateblock *object;
    HRESULT hr;

    TRACE("iface %p, type %#x, stateblock %p.\n", iface, type, stateblock);

    if (type != D3DSBT_ALL && type != D3DSBT_PIXELSTATE && type != D3DSBT_VERTEXSTATE)
    {
        WARN("Unexpected stateblock type, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to create a stateblock while recording, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    wined3d_mutex_unlock();

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = stateblock_init(object, device, type, NULL);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stateblock, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created stateblock %p.\n", object);
    *stateblock = &object->IDirect3DStateBlock9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_BeginStateBlock(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
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

    if (SUCCEEDED(hr = wined3d_stateblock_create(device->wined3d_device, device->state, WINED3D_SBT_RECORDED, &stateblock)))
        device->update_state = device->recording = stateblock;
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_EndStateBlock(IDirect3DDevice9Ex *iface, IDirect3DStateBlock9 **stateblock)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_stateblock *wined3d_stateblock;
    struct d3d9_stateblock *object;
    HRESULT hr;

    TRACE("iface %p, stateblock %p.\n", iface, stateblock);

    wined3d_mutex_lock();
    if (!device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to end a stateblock, but no stateblock is being recorded.\n");
        return D3DERR_INVALIDCALL;
    }
    wined3d_stateblock = device->recording;
    wined3d_stateblock_init_contained_states(wined3d_stateblock);
    device->recording = NULL;
    device->update_state = device->state;
    wined3d_mutex_unlock();

    if (!(object = calloc(1, sizeof(*object))))
    {
        wined3d_stateblock_decref(wined3d_stateblock);
        return E_OUTOFMEMORY;
    }

    hr = stateblock_init(object, device, 0, wined3d_stateblock);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stateblock, hr %#lx.\n", hr);
        wined3d_stateblock_decref(wined3d_stateblock);
        free(object);
        return hr;
    }

    TRACE("Created stateblock %p.\n", object);
    *stateblock = &object->IDirect3DStateBlock9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetClipStatus(IDirect3DDevice9Ex *iface, const D3DCLIPSTATUS9 *clip_status)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);

    wined3d_mutex_lock();
    hr = wined3d_device_set_clip_status(device->wined3d_device, (const struct wined3d_clip_status *)clip_status);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetClipStatus(IDirect3DDevice9Ex *iface, D3DCLIPSTATUS9 *clip_status)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);

    wined3d_mutex_lock();
    hr = wined3d_device_get_clip_status(device->wined3d_device, (struct wined3d_clip_status *)clip_status);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetTexture(IDirect3DDevice9Ex *iface, DWORD stage, IDirect3DBaseTexture9 **texture)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_texture *wined3d_texture = NULL;
    const struct wined3d_stateblock_state *state;
    struct d3d9_texture *texture_impl;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, texture);

    if (!texture)
        return D3DERR_INVALIDCALL;

    if (stage >= D3DVERTEXTEXTURESAMPLER0 && stage <= D3DVERTEXTEXTURESAMPLER3)
        stage -= D3DVERTEXTEXTURESAMPLER0 - WINED3D_VERTEX_SAMPLER_OFFSET;

    if (stage >= ARRAY_SIZE(state->textures))
    {
        WARN("Ignoring invalid stage %lu.\n", stage);
        *texture = NULL;
        return D3D_OK;
    }

    wined3d_mutex_lock();
    state = device->stateblock_state;
    if ((wined3d_texture = state->textures[stage]))
    {
        texture_impl = wined3d_texture_get_parent(wined3d_texture);
        *texture = &texture_impl->IDirect3DBaseTexture9_iface;
        IDirect3DBaseTexture9_AddRef(*texture);
    }
    else
    {
        *texture = NULL;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetTexture(IDirect3DDevice9Ex *iface, DWORD stage, IDirect3DBaseTexture9 *texture)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_texture *texture_impl;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, texture);

    texture_impl = unsafe_impl_from_IDirect3DBaseTexture9(texture);

    if (stage >= D3DVERTEXTEXTURESAMPLER0 && stage <= D3DVERTEXTEXTURESAMPLER3)
        stage -= D3DVERTEXTEXTURESAMPLER0 - WINED3D_VERTEX_SAMPLER_OFFSET;

    wined3d_mutex_lock();
    wined3d_stateblock_set_texture(device->update_state, stage,
            texture_impl ? d3d9_texture_get_draw_texture(texture_impl) : NULL);
    if (!device->recording)
    {
        if (stage < D3D9_MAX_TEXTURE_UNITS)
        {
            if (texture_impl && texture_impl->usage & D3DUSAGE_AUTOGENMIPMAP)
                device->auto_mipmaps |= 1u << stage;
            else
                device->auto_mipmaps &= ~(1u << stage);
        }
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static const enum wined3d_texture_stage_state tss_lookup[] =
{
    WINED3D_TSS_INVALID,                    /*  0, unused */
    WINED3D_TSS_COLOR_OP,                   /*  1, D3DTSS_COLOROP */
    WINED3D_TSS_COLOR_ARG1,                 /*  2, D3DTSS_COLORARG1 */
    WINED3D_TSS_COLOR_ARG2,                 /*  3, D3DTSS_COLORARG2 */
    WINED3D_TSS_ALPHA_OP,                   /*  4, D3DTSS_ALPHAOP */
    WINED3D_TSS_ALPHA_ARG1,                 /*  5, D3DTSS_ALPHAARG1 */
    WINED3D_TSS_ALPHA_ARG2,                 /*  6, D3DTSS_ALPHAARG2 */
    WINED3D_TSS_BUMPENV_MAT00,              /*  7, D3DTSS_BUMPENVMAT00 */
    WINED3D_TSS_BUMPENV_MAT01,              /*  8, D3DTSS_BUMPENVMAT01 */
    WINED3D_TSS_BUMPENV_MAT10,              /*  9, D3DTSS_BUMPENVMAT10 */
    WINED3D_TSS_BUMPENV_MAT11,              /* 10, D3DTSS_BUMPENVMAT11 */
    WINED3D_TSS_TEXCOORD_INDEX,             /* 11, D3DTSS_TEXCOORDINDEX */
    WINED3D_TSS_INVALID,                    /* 12, unused */
    WINED3D_TSS_INVALID,                    /* 13, unused */
    WINED3D_TSS_INVALID,                    /* 14, unused */
    WINED3D_TSS_INVALID,                    /* 15, unused */
    WINED3D_TSS_INVALID,                    /* 16, unused */
    WINED3D_TSS_INVALID,                    /* 17, unused */
    WINED3D_TSS_INVALID,                    /* 18, unused */
    WINED3D_TSS_INVALID,                    /* 19, unused */
    WINED3D_TSS_INVALID,                    /* 20, unused */
    WINED3D_TSS_INVALID,                    /* 21, unused */
    WINED3D_TSS_BUMPENV_LSCALE,             /* 22, D3DTSS_BUMPENVLSCALE */
    WINED3D_TSS_BUMPENV_LOFFSET,            /* 23, D3DTSS_BUMPENVLOFFSET */
    WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS,    /* 24, D3DTSS_TEXTURETRANSFORMFLAGS */
    WINED3D_TSS_INVALID,                    /* 25, unused */
    WINED3D_TSS_COLOR_ARG0,                 /* 26, D3DTSS_COLORARG0 */
    WINED3D_TSS_ALPHA_ARG0,                 /* 27, D3DTSS_ALPHAARG0 */
    WINED3D_TSS_RESULT_ARG,                 /* 28, D3DTSS_RESULTARG */
    WINED3D_TSS_INVALID,                    /* 29, unused */
    WINED3D_TSS_INVALID,                    /* 30, unused */
    WINED3D_TSS_INVALID,                    /* 31, unused */
    WINED3D_TSS_CONSTANT,                   /* 32, D3DTSS_CONSTANT */
};

static HRESULT WINAPI d3d9_device_GetTextureStageState(IDirect3DDevice9Ex *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD *value)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, stage %lu, state %#x, value %p.\n", iface, stage, state, value);

    if (state >= ARRAY_SIZE(tss_lookup) || tss_lookup[state] == WINED3D_TSS_INVALID)
    {
        WARN("Invalid state %#x passed.\n", state);
        return D3D_OK;
    }

    wined3d_mutex_lock();
    *value = device->stateblock_state->texture_states[stage][tss_lookup[state]];
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetTextureStageState(IDirect3DDevice9Ex *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, stage %lu, state %#x, value %#lx.\n", iface, stage, state, value);

    if (state >= ARRAY_SIZE(tss_lookup))
    {
        WARN("Invalid state %#x passed.\n", state);
        return D3D_OK;
    }

    wined3d_mutex_lock();
    wined3d_stateblock_set_texture_stage_state(device->update_state, stage, tss_lookup[state], value);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetSamplerState(IDirect3DDevice9Ex *iface,
        DWORD sampler_idx, D3DSAMPLERSTATETYPE state, DWORD *value)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    const struct wined3d_stateblock_state *device_state;

    TRACE("iface %p, sampler_idx %lu, state %#x, value %p.\n", iface, sampler_idx, state, value);

    if (sampler_idx >= D3DVERTEXTEXTURESAMPLER0 && sampler_idx <= D3DVERTEXTEXTURESAMPLER3)
        sampler_idx -= D3DVERTEXTEXTURESAMPLER0 - WINED3D_VERTEX_SAMPLER_OFFSET;

    if (sampler_idx >= ARRAY_SIZE(device_state->sampler_states))
    {
        WARN("Invalid sampler %lu.\n", sampler_idx);
        *value = 0;
        return D3D_OK;
    }

    wined3d_mutex_lock();
    device_state = device->stateblock_state;
    *value = device_state->sampler_states[sampler_idx][state];
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_SetSamplerState(IDirect3DDevice9Ex *iface,
        DWORD sampler, D3DSAMPLERSTATETYPE state, DWORD value)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, sampler %lu, state %#x, value %#lx.\n", iface, sampler, state, value);

    if (sampler >= D3DVERTEXTEXTURESAMPLER0 && sampler <= D3DVERTEXTEXTURESAMPLER3)
        sampler -= D3DVERTEXTEXTURESAMPLER0 - WINED3D_VERTEX_SAMPLER_OFFSET;

    wined3d_mutex_lock();
    wined3d_stateblock_set_sampler_state(device->update_state, sampler, wined3d_sampler_state_from_d3d(state), value);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_ValidateDevice(IDirect3DDevice9Ex *iface, DWORD *pass_count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, pass_count %p.\n", iface, pass_count);

    wined3d_mutex_lock();
    hr = wined3d_device_validate_device(device->wined3d_device, device->stateblock_state, pass_count);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetPaletteEntries(IDirect3DDevice9Ex *iface,
        UINT palette_idx, const PALETTEENTRY *entries)
{
    WARN("iface %p, palette_idx %u, entries %p unimplemented.\n", iface, palette_idx, entries);

    /* The d3d9 palette API is non-functional on Windows. Getters and setters are implemented,
     * and some drivers allow the creation of P8 surfaces. These surfaces can be copied to
     * other P8 surfaces with StretchRect, but cannot be converted to (A)RGB.
     *
     * Some older(dx7) cards may have support for P8 textures, but games cannot rely on this. */
    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetPaletteEntries(IDirect3DDevice9Ex *iface,
        UINT palette_idx, PALETTEENTRY *entries)
{
    FIXME("iface %p, palette_idx %u, entries %p unimplemented.\n", iface, palette_idx, entries);

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI d3d9_device_SetCurrentTexturePalette(IDirect3DDevice9Ex *iface, UINT palette_idx)
{
    WARN("iface %p, palette_idx %u unimplemented.\n", iface, palette_idx);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetCurrentTexturePalette(IDirect3DDevice9Ex *iface, UINT *palette_idx)
{
    FIXME("iface %p, palette_idx %p.\n", iface, palette_idx);

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI d3d9_device_SetScissorRect(IDirect3DDevice9Ex *iface, const RECT *rect)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, rect %p.\n", iface, rect);

    wined3d_mutex_lock();
    wined3d_stateblock_set_scissor_rect(device->update_state, rect);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetScissorRect(IDirect3DDevice9Ex *iface, RECT *rect)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, rect %p.\n", iface, rect);

    wined3d_mutex_lock();
    *rect = device->stateblock_state->scissor_rect;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetSoftwareVertexProcessing(IDirect3DDevice9Ex *iface, BOOL software)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, software %#x.\n", iface, software);

    wined3d_mutex_lock();
    wined3d_device_set_software_vertex_processing(device->wined3d_device, software);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static BOOL WINAPI d3d9_device_GetSoftwareVertexProcessing(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    BOOL ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_device_get_software_vertex_processing(device->wined3d_device);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d9_device_SetNPatchMode(IDirect3DDevice9Ex *iface, float segment_count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, segment_count %.8e.\n", iface, segment_count);

    wined3d_mutex_lock();
    hr = wined3d_device_set_npatch_mode(device->wined3d_device, segment_count);
    wined3d_mutex_unlock();

    return hr;
}

static float WINAPI d3d9_device_GetNPatchMode(IDirect3DDevice9Ex *iface)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    float ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_device_get_npatch_mode(device->wined3d_device);
    wined3d_mutex_unlock();

    return ret;
}

/* wined3d critical section must be taken by the caller. */
static void d3d9_generate_auto_mipmaps(struct d3d9_device *device)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    struct wined3d_texture *texture;
    unsigned int i, map;

    map = device->auto_mipmaps;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        if ((texture = state->textures[i]))
            d3d9_texture_gen_auto_mipmap(wined3d_texture_get_parent(texture));
    }
}

static void d3d9_device_upload_sysmem_vertex_buffers(struct d3d9_device *device,
        int base_vertex, unsigned int start_vertex, unsigned int vertex_count)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    struct wined3d_vertex_declaration *wined3d_decl;
    struct wined3d_box box = {0, 0, 0, 1, 0, 1};
    const struct wined3d_stream_state *stream;
    struct d3d9_vertexbuffer *d3d9_buffer;
    struct wined3d_resource *dst_resource;
    struct d3d9_vertex_declaration *decl;
    struct wined3d_buffer *dst_buffer;
    struct wined3d_resource_desc desc;
    unsigned int stride, map;
    HRESULT hr;

    if (!device->sysmem_vb)
        return;
    wined3d_decl = state->vertex_declaration;
    if (!wined3d_decl)
        return;

    if (base_vertex >= 0 || start_vertex >= -base_vertex)
        start_vertex += base_vertex;
    else
        FIXME("System memory vertex data offset is negative.\n");

    decl = wined3d_vertex_declaration_get_parent(wined3d_decl);
    map = decl->stream_map & device->sysmem_vb;
    while (map)
    {
        stream = &state->streams[wined3d_bit_scan(&map)];
        dst_buffer = stream->buffer;
        stride = stream->stride;

        d3d9_buffer = wined3d_buffer_get_parent(dst_buffer);
        dst_resource = wined3d_buffer_get_resource(dst_buffer);
        wined3d_resource_get_desc(dst_resource, &desc);
        box.left = stream->offset + start_vertex * stride;
        box.right = min(box.left + vertex_count * stride, desc.size);
        if (FAILED(hr = wined3d_device_context_copy_sub_resource_region(device->immediate_context,
                dst_resource, 0, box.left, 0, 0,
                wined3d_buffer_get_resource(d3d9_buffer->wined3d_buffer), 0, &box, 0)))
            ERR("Failed to update buffer.\n");
    }
}

static HRESULT d3d9_device_upload_sysmem_index_buffer(struct d3d9_device *device,
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

static void d3d9_device_flush_mapped_vertex_buffers(struct d3d9_device *device)
{
    for (unsigned int i = 0; i < WINED3D_MAX_STREAMS; ++i)
    {
        struct wined3d_buffer *buffer = device->stateblock_state->streams[i].buffer;

        if (buffer)
            wined3d_device_context_flush_mapped_buffer(device->immediate_context, buffer);
    }
}

static void d3d9_device_upload_managed_textures(struct d3d9_device *device)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    unsigned int i;

    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        struct wined3d_texture *wined3d_texture = state->textures[i];
        struct d3d9_texture *d3d9_texture;

        if (!wined3d_texture)
            continue;
        d3d9_texture = wined3d_texture_get_parent(wined3d_texture);
        if (d3d9_texture->draw_texture)
            wined3d_device_update_texture(device->wined3d_device,
                    d3d9_texture->wined3d_texture, d3d9_texture->draw_texture);
    }
}

static HRESULT WINAPI d3d9_device_DrawPrimitive(IDirect3DDevice9Ex *iface,
        D3DPRIMITIVETYPE primitive_type, UINT start_vertex, UINT primitive_count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    unsigned int vertex_count;

    TRACE("iface %p, primitive_type %#x, start_vertex %u, primitive_count %u.\n",
            iface, primitive_type, start_vertex, primitive_count);

    wined3d_mutex_lock();
    if (!device->stateblock_state->vertex_declaration)
    {
        wined3d_mutex_unlock();
        WARN("Called without a valid vertex declaration set.\n");
        return D3DERR_INVALIDCALL;
    }
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    vertex_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    d3d9_device_upload_managed_textures(device);
    d3d9_device_upload_sysmem_vertex_buffers(device, 0, start_vertex, vertex_count);
    d3d9_device_flush_mapped_vertex_buffers(device);
    d3d9_generate_auto_mipmaps(device);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);

    /* Instancing is ignored for non-indexed draws. */
    wined3d_device_context_draw(device->immediate_context, start_vertex, vertex_count, 0, 0);

    d3d9_rts_flag_auto_gen_mipmap(device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_DrawIndexedPrimitive(IDirect3DDevice9Ex *iface,
        D3DPRIMITIVETYPE primitive_type, INT base_vertex_idx, UINT min_vertex_idx,
        UINT vertex_count, UINT start_idx, UINT primitive_count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    unsigned int index_count;

    TRACE("iface %p, primitive_type %#x, base_vertex_idx %u, min_vertex_idx %u, "
            "vertex_count %u, start_idx %u, primitive_count %u.\n",
            iface, primitive_type, base_vertex_idx, min_vertex_idx,
            vertex_count, start_idx, primitive_count);

    wined3d_mutex_lock();
    if (!device->stateblock_state->vertex_declaration)
    {
        wined3d_mutex_unlock();
        WARN("Called without a valid vertex declaration set.\n");
        return D3DERR_INVALIDCALL;
    }
    if (!device->stateblock_state->index_buffer)
    {
        wined3d_mutex_unlock();
        WARN("Called without a valid index buffer set.\n");
        return D3DERR_INVALIDCALL;
    }
    index_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    d3d9_device_upload_managed_textures(device);
    d3d9_device_upload_sysmem_vertex_buffers(device, base_vertex_idx, min_vertex_idx, vertex_count);
    d3d9_device_flush_mapped_vertex_buffers(device);
    d3d9_generate_auto_mipmaps(device);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    d3d9_device_upload_sysmem_index_buffer(device, &start_idx, index_count);
    wined3d_device_context_flush_mapped_buffer(device->immediate_context, device->stateblock_state->index_buffer);
    wined3d_device_context_draw_indexed(device->immediate_context, base_vertex_idx, start_idx, index_count, 0,
            device->stateblock_state->streams[0].frequency);
    d3d9_rts_flag_auto_gen_mipmap(device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_DrawPrimitiveUP(IDirect3DDevice9Ex *iface,
        D3DPRIMITIVETYPE primitive_type, UINT primitive_count, const void *data, UINT stride)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    UINT vtx_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    UINT size = vtx_count * stride;
    unsigned int vb_pos;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, primitive_count %u, data %p, stride %u.\n",
            iface, primitive_type, primitive_count, data, stride);

    if (!stride)
    {
        WARN("stride is 0, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    if (!primitive_count)
    {
        WARN("primitive_count is 0, returning D3D_OK.\n");
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (!device->stateblock_state->vertex_declaration)
    {
        wined3d_mutex_unlock();
        WARN("Called without a valid vertex declaration set.\n");
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device,
            &device->vertex_buffer, data, size, stride, &vb_pos)))
        goto done;

    hr = wined3d_stateblock_set_stream_source(device->state, 0, device->vertex_buffer.buffer, 0, stride);
    if (FAILED(hr))
        goto done;

    d3d9_generate_auto_mipmaps(device);
    d3d9_device_upload_managed_textures(device);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);

    /* Instancing is ignored for non-indexed draws. */
    wined3d_device_context_draw(device->immediate_context, vb_pos / stride, vtx_count, 0, 0);

    wined3d_stateblock_set_stream_source(device->state, 0, NULL, 0, 0);
    d3d9_rts_flag_auto_gen_mipmap(device);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d9_device_DrawIndexedPrimitiveUP(IDirect3DDevice9Ex *iface,
        D3DPRIMITIVETYPE primitive_type, UINT min_vertex_idx, UINT vertex_count,
        UINT primitive_count, const void *index_data, D3DFORMAT index_format,
        const void *vertex_data, UINT vertex_stride)
{
    UINT idx_count = vertex_count_from_primitive_count(primitive_type, primitive_count);
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    UINT idx_fmt_size = index_format == D3DFMT_INDEX16 ? 2 : 4;
    UINT vtx_size = vertex_count * vertex_stride;
    UINT idx_size = idx_count * idx_fmt_size;
    UINT vb_pos, ib_pos;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, min_vertex_idx %u, vertex_count %u, primitive_count %u, "
            "index_data %p, index_format %#x, vertex_data %p, vertex_stride %u.\n",
            iface, primitive_type, min_vertex_idx, vertex_count, primitive_count,
            index_data, index_format, vertex_data, vertex_stride);

    if (!vertex_stride)
    {
        WARN("vertex_stride is 0, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    if (!primitive_count)
    {
        WARN("primitive_count is 0, returning D3D_OK.\n");
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (!device->stateblock_state->vertex_declaration)
    {
        wined3d_mutex_unlock();
        WARN("Called without a valid vertex declaration set.\n");
        return D3DERR_INVALIDCALL;
    }

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

    d3d9_generate_auto_mipmaps(device);
    d3d9_device_upload_managed_textures(device);
    wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_d3d(primitive_type), 0);
    wined3d_device_context_draw_indexed(device->immediate_context,
            vb_pos / vertex_stride - min_vertex_idx, ib_pos / idx_fmt_size, idx_count, 0,
            device->stateblock_state->streams[0].frequency);

    wined3d_stateblock_set_stream_source(device->state, 0, NULL, 0, 0);
    wined3d_stateblock_set_index_buffer(device->state, NULL, WINED3DFMT_UNKNOWN);

    d3d9_rts_flag_auto_gen_mipmap(device);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d9_device_ProcessVertices(IDirect3DDevice9Ex *iface,
        UINT src_start_idx, UINT dst_idx, UINT vertex_count, IDirect3DVertexBuffer9 *dst_buffer,
        IDirect3DVertexDeclaration9 *declaration, DWORD flags)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertexbuffer *dst_impl = unsafe_impl_from_IDirect3DVertexBuffer9(dst_buffer);
    struct d3d9_vertex_declaration *decl_impl = unsafe_impl_from_IDirect3DVertexDeclaration9(declaration);
    const struct wined3d_stateblock_state *state;
    const struct wined3d_stream_state *stream;
    struct d3d9_vertexbuffer *d3d9_buffer;
    unsigned int i, map;
    HRESULT hr;

    TRACE("iface %p, src_start_idx %u, dst_idx %u, vertex_count %u, dst_buffer %p, declaration %p, flags %#lx.\n",
            iface, src_start_idx, dst_idx, vertex_count, dst_buffer, declaration, flags);

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

        d3d9_buffer = wined3d_buffer_get_parent(stream->buffer);
        if (FAILED(wined3d_stateblock_set_stream_source(device->state,
                i, d3d9_buffer->wined3d_buffer, stream->offset, stream->stride)))
            ERR("Failed to set stream source.\n");
    }

    hr = wined3d_device_process_vertices(device->wined3d_device, device->state, src_start_idx, dst_idx, vertex_count,
            dst_impl->wined3d_buffer, decl_impl ? decl_impl->wined3d_declaration : NULL,
            flags, dst_impl->fvf);

    map = device->sysmem_vb;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        stream = &state->streams[i];

        d3d9_buffer = wined3d_buffer_get_parent(stream->buffer);
        if (FAILED(wined3d_stateblock_set_stream_source(device->state,
                i, d3d9_buffer->draw_buffer, stream->offset, stream->stride)))
            ERR("Failed to set stream source.\n");
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_CreateVertexDeclaration(IDirect3DDevice9Ex *iface,
        const D3DVERTEXELEMENT9 *elements, IDirect3DVertexDeclaration9 **declaration)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertex_declaration *object;
    HRESULT hr;

    TRACE("iface %p, elements %p, declaration %p.\n", iface, elements, declaration);

    if (!declaration)
    {
        WARN("Caller passed a NULL declaration, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    if (SUCCEEDED(hr = d3d9_vertex_declaration_create(device, elements, &object)))
        *declaration = &object->IDirect3DVertexDeclaration9_iface;

    return hr;
}

static HRESULT WINAPI d3d9_device_SetVertexDeclaration(IDirect3DDevice9Ex *iface,
        IDirect3DVertexDeclaration9 *declaration)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertex_declaration *decl_impl = unsafe_impl_from_IDirect3DVertexDeclaration9(declaration);

    TRACE("iface %p, declaration %p.\n", iface, declaration);

    wined3d_mutex_lock();
    wined3d_stateblock_set_vertex_declaration(device->update_state,
            decl_impl ? decl_impl->wined3d_declaration : NULL);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetVertexDeclaration(IDirect3DDevice9Ex *iface,
        IDirect3DVertexDeclaration9 **declaration)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct d3d9_vertex_declaration *declaration_impl;

    TRACE("iface %p, declaration %p.\n", iface, declaration);

    if (!declaration) return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    if ((wined3d_declaration = device->stateblock_state->vertex_declaration))
    {
        declaration_impl = wined3d_vertex_declaration_get_parent(wined3d_declaration);
        *declaration = &declaration_impl->IDirect3DVertexDeclaration9_iface;
        IDirect3DVertexDeclaration9_AddRef(*declaration);
    }
    else
    {
        *declaration = NULL;
    }
    wined3d_mutex_unlock();

    TRACE("Returning %p.\n", *declaration);
    return D3D_OK;
}

static struct wined3d_vertex_declaration *device_get_fvf_declaration(struct d3d9_device *device, DWORD fvf)
{
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct fvf_declaration *fvf_decls = device->fvf_decls;
    struct d3d9_vertex_declaration *d3d9_declaration;
    D3DVERTEXELEMENT9 *elements;
    int p, low, high; /* deliberately signed */
    HRESULT hr;

    TRACE("Searching for declaration for fvf %08lx... ", fvf);

    low = 0;
    high = device->fvf_decl_count - 1;
    while (low <= high)
    {
        p = (low + high) >> 1;
        TRACE("%d ", p);

        if (fvf_decls[p].fvf == fvf)
        {
            TRACE("found %p.\n", fvf_decls[p].decl);
            return fvf_decls[p].decl;
        }

        if (fvf_decls[p].fvf < fvf)
            low = p + 1;
        else
            high = p - 1;
    }
    TRACE("not found. Creating and inserting at position %d.\n", low);

    if (FAILED(hr = vdecl_convert_fvf(fvf, &elements)))
        return NULL;

    hr = d3d9_vertex_declaration_create(device, elements, &d3d9_declaration);
    free(elements);
    if (FAILED(hr))
        return NULL;

    if (device->fvf_decl_size == device->fvf_decl_count)
    {
        UINT grow = max(device->fvf_decl_size / 2, 8);

        if (!(fvf_decls = realloc(fvf_decls, sizeof(*fvf_decls) * (device->fvf_decl_size + grow))))
        {
            IDirect3DVertexDeclaration9_Release(&d3d9_declaration->IDirect3DVertexDeclaration9_iface);
            return NULL;
        }
        device->fvf_decls = fvf_decls;
        device->fvf_decl_size += grow;
    }

    d3d9_declaration->fvf = fvf;
    wined3d_declaration = d3d9_declaration->wined3d_declaration;
    wined3d_vertex_declaration_incref(wined3d_declaration);
    IDirect3DVertexDeclaration9_Release(&d3d9_declaration->IDirect3DVertexDeclaration9_iface);

    memmove(fvf_decls + low + 1, fvf_decls + low, sizeof(*fvf_decls) * (device->fvf_decl_count - low));
    fvf_decls[low].decl = wined3d_declaration;
    fvf_decls[low].fvf = fvf;
    ++device->fvf_decl_count;

    TRACE("Returning %p. %u declarations in array.\n", wined3d_declaration, device->fvf_decl_count);

    return wined3d_declaration;
}

static HRESULT WINAPI d3d9_device_SetFVF(IDirect3DDevice9Ex *iface, DWORD fvf)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_vertex_declaration *decl;

    TRACE("iface %p, fvf %#lx.\n", iface, fvf);

    if (!fvf)
    {
        WARN("%#lx is not a valid FVF.\n", fvf);
        return D3D_OK;
    }

    wined3d_mutex_lock();
    if (!(decl = device_get_fvf_declaration(device, fvf)))
    {
        wined3d_mutex_unlock();
        ERR("Failed to create a vertex declaration for fvf %#lx.\n", fvf);
        return D3DERR_DRIVERINTERNALERROR;
    }

    wined3d_stateblock_set_vertex_declaration(device->update_state, decl);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetFVF(IDirect3DDevice9Ex *iface, DWORD *fvf)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct d3d9_vertex_declaration *d3d9_declaration;

    TRACE("iface %p, fvf %p.\n", iface, fvf);

    wined3d_mutex_lock();
    if ((wined3d_declaration = device->stateblock_state->vertex_declaration))
    {
        d3d9_declaration = wined3d_vertex_declaration_get_parent(wined3d_declaration);
        *fvf = d3d9_declaration->fvf;
    }
    else
    {
        *fvf = 0;
    }
    wined3d_mutex_unlock();

    TRACE("Returning FVF %#lx.\n", *fvf);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_CreateVertexShader(IDirect3DDevice9Ex *iface,
        const DWORD *byte_code, IDirect3DVertexShader9 **shader)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertexshader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, shader %p.\n", iface, byte_code, shader);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = vertexshader_init(object, device, byte_code);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex shader, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created vertex shader %p.\n", object);
    *shader = &object->IDirect3DVertexShader9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetVertexShader(IDirect3DDevice9Ex *iface, IDirect3DVertexShader9 *shader)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertexshader *shader_obj = unsafe_impl_from_IDirect3DVertexShader9(shader);

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    wined3d_stateblock_set_vertex_shader(device->update_state,
            shader_obj ? shader_obj->wined3d_shader : NULL);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetVertexShader(IDirect3DDevice9Ex *iface, IDirect3DVertexShader9 **shader)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertexshader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    if ((wined3d_shader = device->stateblock_state->vs))
    {
        shader_impl = wined3d_shader_get_parent(wined3d_shader);
        *shader = &shader_impl->IDirect3DVertexShader9_iface;
        IDirect3DVertexShader9_AddRef(*shader);
    }
    else
    {
        *shader = NULL;
    }
    wined3d_mutex_unlock();

    TRACE("Returning %p.\n", *shader);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetVertexShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT reg_idx, const float *data, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, reg_idx %u, data %p, count %u.\n", iface, reg_idx, data, count);

    if (reg_idx + count > D3D9_MAX_VERTEX_SHADER_CONSTANTF)
    {
        WARN("Trying to access %u constants, but d3d9 only supports %u\n",
             reg_idx + count, D3D9_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_vs_consts_f(device->update_state, reg_idx,
            count, (const struct wined3d_vec4 *)data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetVertexShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT start_idx, float *constants, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %u, constants %p, count %u.\n", iface, start_idx, constants, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_vs_consts_f(device->state, start_idx, count, (struct wined3d_vec4 *)constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetVertexShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT reg_idx, const int *data, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, reg_idx %u, data %p, count %u.\n", iface, reg_idx, data, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_vs_consts_i(device->update_state,
            reg_idx, count, (const struct wined3d_ivec4 *)data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetVertexShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT start_idx, int *constants, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %u, constants %p, count %u.\n", iface, start_idx, constants, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_vs_consts_i(device->state, start_idx, count, (struct wined3d_ivec4 *)constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetVertexShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT reg_idx, const BOOL *data, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, reg_idx %u, data %p, count %u.\n", iface, reg_idx, data, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_vs_consts_b(device->update_state, reg_idx, count, data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetVertexShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT start_idx, BOOL *constants, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %u, constants %p, count %u.\n", iface, start_idx, constants, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_vs_consts_b(device->state, start_idx, count, constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetStreamSource(IDirect3DDevice9Ex *iface,
        UINT stream_idx, IDirect3DVertexBuffer9 *buffer, UINT offset, UINT stride)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_vertexbuffer *buffer_impl = unsafe_impl_from_IDirect3DVertexBuffer9(buffer);
    struct wined3d_buffer *wined3d_buffer;
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, buffer %p, offset %u, stride %u.\n",
            iface, stream_idx, buffer, offset, stride);

    if (stream_idx >= ARRAY_SIZE(device->stateblock_state->streams))
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if (!buffer_impl)
    {
        const struct wined3d_stream_state *stream = &device->stateblock_state->streams[stream_idx];

        offset = stream->offset;
        stride = stream->stride;
        wined3d_buffer = NULL;
    }
    else if (buffer_impl->draw_buffer)
    {
        wined3d_buffer = buffer_impl->draw_buffer;
    }
    else
    {
        wined3d_buffer = buffer_impl->wined3d_buffer;
    }

    hr = wined3d_stateblock_set_stream_source(device->update_state, stream_idx, wined3d_buffer, offset, stride);
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

static HRESULT WINAPI d3d9_device_GetStreamSource(IDirect3DDevice9Ex *iface,
        UINT stream_idx, IDirect3DVertexBuffer9 **buffer, UINT *offset, UINT *stride)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    const struct wined3d_stateblock_state *state;
    const struct wined3d_stream_state *stream;
    struct d3d9_vertexbuffer *buffer_impl;

    TRACE("iface %p, stream_idx %u, buffer %p, offset %p, stride %p.\n",
            iface, stream_idx, buffer, offset, stride);

    if (!buffer)
        return D3DERR_INVALIDCALL;

    if (stream_idx >= ARRAY_SIZE(state->streams))
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    stream = &device->stateblock_state->streams[stream_idx];
    if (stream->buffer)
    {
        buffer_impl = wined3d_buffer_get_parent(stream->buffer);
        *buffer = &buffer_impl->IDirect3DVertexBuffer9_iface;
        IDirect3DVertexBuffer9_AddRef(*buffer);
    }
    else
        *buffer = NULL;
    if (offset)
        *offset = stream->offset;
    *stride = stream->stride;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetStreamSourceFreq(IDirect3DDevice9Ex *iface, UINT stream_idx, UINT freq)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, freq %u.\n", iface, stream_idx, freq);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_stream_source_freq(device->update_state, stream_idx, freq);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetStreamSourceFreq(IDirect3DDevice9Ex *iface, UINT stream_idx, UINT *freq)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    const struct wined3d_stream_state *stream;

    TRACE("iface %p, stream_idx %u, freq %p.\n", iface, stream_idx, freq);

    wined3d_mutex_lock();
    stream = &device->stateblock_state->streams[stream_idx];
    *freq = stream->flags | stream->frequency;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetIndices(IDirect3DDevice9Ex *iface, IDirect3DIndexBuffer9 *buffer)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_indexbuffer *ib = unsafe_impl_from_IDirect3DIndexBuffer9(buffer);
    struct wined3d_buffer *wined3d_buffer;

    TRACE("iface %p, buffer %p.\n", iface, buffer);

    if (!ib)
        wined3d_buffer = NULL;
    else
        wined3d_buffer = ib->wined3d_buffer;

    wined3d_mutex_lock();
    wined3d_stateblock_set_index_buffer(device->update_state, wined3d_buffer, ib ? ib->format : WINED3DFMT_UNKNOWN);
    if (!device->recording)
        device->sysmem_ib = ib && ib->sysmem;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetIndices(IDirect3DDevice9Ex *iface, IDirect3DIndexBuffer9 **buffer)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_buffer *wined3d_buffer;
    struct d3d9_indexbuffer *buffer_impl;

    TRACE("iface %p, buffer %p.\n", iface, buffer);

    if (!buffer)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    if ((wined3d_buffer = device->stateblock_state->index_buffer))
    {
        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        *buffer = &buffer_impl->IDirect3DIndexBuffer9_iface;
        IDirect3DIndexBuffer9_AddRef(*buffer);
    }
    else
    {
        *buffer = NULL;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_CreatePixelShader(IDirect3DDevice9Ex *iface,
        const DWORD *byte_code, IDirect3DPixelShader9 **shader)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_pixelshader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, shader %p.\n", iface, byte_code, shader);

    if (!(object = calloc(1, sizeof(*object))))
    {
        FIXME("Failed to allocate pixel shader memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = pixelshader_init(object, device, byte_code);
    if (FAILED(hr))
    {
        WARN("Failed to initialize pixel shader, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created pixel shader %p.\n", object);
    *shader = &object->IDirect3DPixelShader9_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetPixelShader(IDirect3DDevice9Ex *iface, IDirect3DPixelShader9 *shader)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_pixelshader *shader_obj = unsafe_impl_from_IDirect3DPixelShader9(shader);

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    wined3d_stateblock_set_pixel_shader(device->update_state,
            shader_obj ? shader_obj->wined3d_shader : NULL);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetPixelShader(IDirect3DDevice9Ex *iface, IDirect3DPixelShader9 **shader)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_pixelshader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p.\n", iface, shader);

    if (!shader) return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    if ((wined3d_shader = device->stateblock_state->ps))
    {
        shader_impl = wined3d_shader_get_parent(wined3d_shader);
        *shader = &shader_impl->IDirect3DPixelShader9_iface;
        IDirect3DPixelShader9_AddRef(*shader);
    }
    else
    {
        *shader = NULL;
    }
    wined3d_mutex_unlock();

    TRACE("Returning %p.\n", *shader);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetPixelShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT reg_idx, const float *data, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, reg_idx %u, data %p, count %u.\n", iface, reg_idx, data, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_ps_consts_f(device->update_state,
            reg_idx, count, (const struct wined3d_vec4 *)data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetPixelShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT start_idx, float *constants, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %u, constants %p, count %u.\n", iface, start_idx, constants, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_ps_consts_f(device->state, start_idx, count, (struct wined3d_vec4 *)constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetPixelShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT reg_idx, const int *data, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, reg_idx %u, data %p, count %u.\n", iface, reg_idx, data, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_ps_consts_i(device->update_state,
            reg_idx, count, (const struct wined3d_ivec4 *)data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetPixelShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT start_idx, int *constants, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %u, constants %p, count %u.\n", iface, start_idx, constants, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_ps_consts_i(device->state, start_idx, count, (struct wined3d_ivec4 *)constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_SetPixelShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT reg_idx, const BOOL *data, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, reg_idx %u, data %p, count %u.\n", iface, reg_idx, data, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_ps_consts_b(device->update_state, reg_idx, count, data);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_GetPixelShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT start_idx, BOOL *constants, UINT count)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, start_idx %u, constants %p, count %u.\n", iface, start_idx, constants, count);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_ps_consts_b(device->state, start_idx, count, constants);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_device_DrawRectPatch(IDirect3DDevice9Ex *iface, UINT handle,
        const float *segment_count, const D3DRECTPATCH_INFO *patch_info)
{
    FIXME("iface %p, handle %#x, segment_count %p, patch_info %p unimplemented.\n",
            iface, handle, segment_count, patch_info);
    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_DrawTriPatch(IDirect3DDevice9Ex *iface, UINT handle,
        const float *segment_count, const D3DTRIPATCH_INFO *patch_info)
{
    FIXME("iface %p, handle %#x, segment_count %p, patch_info %p unimplemented.\n",
            iface, handle, segment_count, patch_info);
    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_DeletePatch(IDirect3DDevice9Ex *iface, UINT handle)
{
    FIXME("iface %p, handle %#x unimplemented.\n", iface, handle);
    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI d3d9_device_CreateQuery(IDirect3DDevice9Ex *iface, D3DQUERYTYPE type, IDirect3DQuery9 **query)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_query *object;
    HRESULT hr;

    TRACE("iface %p, type %#x, query %p.\n", iface, type, query);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = query_init(object, device, type);
    if (FAILED(hr))
    {
        WARN("Failed to initialize query, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created query %p.\n", object);
    if (query) *query = &object->IDirect3DQuery9_iface;
    else IDirect3DQuery9_Release(&object->IDirect3DQuery9_iface);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_SetConvolutionMonoKernel(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, float *rows, float *columns)
{
    FIXME("iface %p, width %u, height %u, rows %p, columns %p stub!\n",
            iface, width, height, rows, columns);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9_device_ComposeRects(IDirect3DDevice9Ex *iface,
        IDirect3DSurface9 *src_surface, IDirect3DSurface9 *dst_surface, IDirect3DVertexBuffer9 *src_descs,
        UINT rect_count, IDirect3DVertexBuffer9 *dst_descs, D3DCOMPOSERECTSOP operation, INT offset_x, INT offset_y)
{
    FIXME("iface %p, src_surface %p, dst_surface %p, src_descs %p, rect_count %u, "
            "dst_descs %p, operation %#x, offset_x %u, offset_y %u stub!\n",
            iface, src_surface, dst_surface, src_descs, rect_count,
            dst_descs, operation, offset_x, offset_y);

    return E_NOTIMPL;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_PresentEx(IDirect3DDevice9Ex *iface,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct d3d9_swapchain *swapchain;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p, flags %#lx.\n",
            iface, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect),
            dst_window_override, dirty_region, flags);

    if (device->device_state != D3D9_DEVICE_STATE_OK)
        return S_PRESENT_OCCLUDED;

    if (dirty_region)
        FIXME("Ignoring dirty_region %p.\n", dirty_region);

    wined3d_mutex_lock();
    for (i = 0; i < device->implicit_swapchain_count; ++i)
    {
        swapchain = wined3d_swapchain_get_parent(device->implicit_swapchains[i]);
        if (FAILED(hr = wined3d_swapchain_present(swapchain->wined3d_swapchain,
                src_rect, dst_rect, dst_window_override, swapchain->swap_interval, flags)))
        {
            wined3d_mutex_unlock();
            return hr;
        }
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_device_GetGPUThreadPriority(IDirect3DDevice9Ex *iface, INT *priority)
{
    FIXME("iface %p, priority %p stub!\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9_device_SetGPUThreadPriority(IDirect3DDevice9Ex *iface, INT priority)
{
    FIXME("iface %p, priority %d stub!\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9_device_WaitForVBlank(IDirect3DDevice9Ex *iface, UINT swapchain_idx)
{
    FIXME("iface %p, swapchain_idx %u stub!\n", iface, swapchain_idx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9_device_CheckResourceResidency(IDirect3DDevice9Ex *iface,
        IDirect3DResource9 **resources, UINT32 resource_count)
{
    FIXME("iface %p, resources %p, resource_count %u stub!\n",
            iface, resources, resource_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9_device_SetMaximumFrameLatency(IDirect3DDevice9Ex *iface, UINT max_latency)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, max_latency %u.\n", iface, max_latency);

    if (max_latency > 30)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    wined3d_device_set_max_frame_latency(device->wined3d_device, max_latency);
    wined3d_mutex_unlock();

    return S_OK;
}

static HRESULT WINAPI d3d9_device_GetMaximumFrameLatency(IDirect3DDevice9Ex *iface, UINT *max_latency)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, max_latency %p.\n", iface, max_latency);

    wined3d_mutex_lock();
    *max_latency = wined3d_device_get_max_frame_latency(device->wined3d_device);
    wined3d_mutex_unlock();

    return S_OK;
}

static HRESULT WINAPI d3d9_device_CheckDeviceState(IDirect3DDevice9Ex *iface, HWND dst_window)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_swapchain_desc swapchain_desc;

    TRACE("iface %p, dst_window %p.\n", iface, dst_window);

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(device->implicit_swapchains[0], &swapchain_desc);
    wined3d_mutex_unlock();

    if (swapchain_desc.windowed)
        return D3D_OK;

    /* FIXME: This is actually supposed to check if any other device is in
     * fullscreen mode. */
    if (dst_window != swapchain_desc.device_window)
        return device->device_state == D3D9_DEVICE_STATE_OK ? S_PRESENT_OCCLUDED : D3D_OK;

    return device->device_state == D3D9_DEVICE_STATE_OK ? D3D_OK : S_PRESENT_OCCLUDED;
}

static HRESULT WINAPI d3d9_device_CreateRenderTargetEx(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality,
        BOOL lockable, IDirect3DSurface9 **surface, HANDLE *shared_handle, DWORD usage)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    unsigned int access = WINED3D_RESOURCE_ACCESS_GPU;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %lu, "
            "lockable %#x, surface %p, shared_handle %p, usage %#lx.\n",
            iface, width, height, format, multisample_type, multisample_quality,
            lockable, surface, shared_handle, usage);

    *surface = NULL;

    if (usage & (D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_RENDERTARGET))
    {
        WARN("Invalid usage %#lx.\n", usage);
        return D3DERR_INVALIDCALL;
    }

    if (shared_handle)
        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);

    if (lockable)
        access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;

    return d3d9_device_create_surface(device, 0, wined3dformat_from_d3dformat(format),
            wined3d_multisample_type_from_d3d(multisample_type), multisample_quality,
            usage & WINED3DUSAGE_MASK, WINED3D_BIND_RENDER_TARGET, access, width, height, NULL, surface);
}

static HRESULT WINAPI d3d9_device_CreateOffscreenPlainSurfaceEx(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface,
        HANDLE *shared_handle, DWORD usage)
{
    FIXME("iface %p, width %u, height %u, format %#x, pool %#x, surface %p, shared_handle %p, usage %#lx stub!\n",
            iface, width, height, format, pool, surface, shared_handle, usage);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3d9_device_CreateDepthStencilSurfaceEx(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality,
        BOOL discard, IDirect3DSurface9 **surface, HANDLE *shared_handle, DWORD usage)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    DWORD flags = 0;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %lu, "
            "discard %#x, surface %p, shared_handle %p, usage %#lx.\n",
            iface, width, height, format, multisample_type, multisample_quality,
            discard, surface, shared_handle, usage);

    if (usage & (D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_RENDERTARGET))
    {
        WARN("Invalid usage %#lx.\n", usage);
        return D3DERR_INVALIDCALL;
    }

    if (shared_handle)
        FIXME("Resource sharing not implemented, *shared_handle %p.\n", *shared_handle);

    if (discard)
        flags |= WINED3D_TEXTURE_CREATE_DISCARD;

    *surface = NULL;
    return d3d9_device_create_surface(device, flags, wined3dformat_from_d3dformat(format),
            wined3d_multisample_type_from_d3d(multisample_type), multisample_quality, usage & WINED3DUSAGE_MASK,
            WINED3D_BIND_DEPTH_STENCIL, WINED3D_RESOURCE_ACCESS_GPU, width, height, NULL, surface);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_device_ResetEx(IDirect3DDevice9Ex *iface,
        D3DPRESENT_PARAMETERS *present_parameters, D3DDISPLAYMODEEX *mode)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);

    TRACE("iface %p, present_parameters %p, mode %p.\n", iface, present_parameters, mode);

    if (!present_parameters->Windowed == !mode)
    {
        WARN("Mode can be passed if and only if Windowed is FALSE.\n");
        return D3DERR_INVALIDCALL;
    }

    if (mode && (mode->Width != present_parameters->BackBufferWidth
            || mode->Height != present_parameters->BackBufferHeight))
    {
        WARN("Mode and back buffer mismatch (mode %ux%u, backbuffer %ux%u).\n",
                mode->Width, mode->Height,
                present_parameters->BackBufferWidth, present_parameters->BackBufferHeight);
        return D3DERR_INVALIDCALL;
    }

    return d3d9_device_reset(device, present_parameters, mode);
}

static HRESULT WINAPI d3d9_device_GetDisplayModeEx(IDirect3DDevice9Ex *iface,
        UINT swapchain_idx, D3DDISPLAYMODEEX *mode, D3DDISPLAYROTATION *rotation)
{
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, swapchain_idx %u, mode %p, rotation %p.\n",
            iface, swapchain_idx, mode, rotation);

    if (mode->Size != sizeof(*mode))
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_device_get_display_mode(device->wined3d_device, swapchain_idx, &wined3d_mode,
            (enum wined3d_display_rotation *)rotation);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
        mode->ScanLineOrdering = d3dscanlineordering_from_wined3d(wined3d_mode.scanline_ordering);
    }

    return hr;
}

static const struct IDirect3DDevice9ExVtbl d3d9_device_vtbl =
{
    /* IUnknown */
    d3d9_device_QueryInterface,
    d3d9_device_AddRef,
    d3d9_device_Release,
    /* IDirect3DDevice9 */
    d3d9_device_TestCooperativeLevel,
    d3d9_device_GetAvailableTextureMem,
    d3d9_device_EvictManagedResources,
    d3d9_device_GetDirect3D,
    d3d9_device_GetDeviceCaps,
    d3d9_device_GetDisplayMode,
    d3d9_device_GetCreationParameters,
    d3d9_device_SetCursorProperties,
    d3d9_device_SetCursorPosition,
    d3d9_device_ShowCursor,
    d3d9_device_CreateAdditionalSwapChain,
    d3d9_device_GetSwapChain,
    d3d9_device_GetNumberOfSwapChains,
    d3d9_device_Reset,
    d3d9_device_Present,
    d3d9_device_GetBackBuffer,
    d3d9_device_GetRasterStatus,
    d3d9_device_SetDialogBoxMode,
    d3d9_device_SetGammaRamp,
    d3d9_device_GetGammaRamp,
    d3d9_device_CreateTexture,
    d3d9_device_CreateVolumeTexture,
    d3d9_device_CreateCubeTexture,
    d3d9_device_CreateVertexBuffer,
    d3d9_device_CreateIndexBuffer,
    d3d9_device_CreateRenderTarget,
    d3d9_device_CreateDepthStencilSurface,
    d3d9_device_UpdateSurface,
    d3d9_device_UpdateTexture,
    d3d9_device_GetRenderTargetData,
    d3d9_device_GetFrontBufferData,
    d3d9_device_StretchRect,
    d3d9_device_ColorFill,
    d3d9_device_CreateOffscreenPlainSurface,
    d3d9_device_SetRenderTarget,
    d3d9_device_GetRenderTarget,
    d3d9_device_SetDepthStencilSurface,
    d3d9_device_GetDepthStencilSurface,
    d3d9_device_BeginScene,
    d3d9_device_EndScene,
    d3d9_device_Clear,
    d3d9_device_SetTransform,
    d3d9_device_GetTransform,
    d3d9_device_MultiplyTransform,
    d3d9_device_SetViewport,
    d3d9_device_GetViewport,
    d3d9_device_SetMaterial,
    d3d9_device_GetMaterial,
    d3d9_device_SetLight,
    d3d9_device_GetLight,
    d3d9_device_LightEnable,
    d3d9_device_GetLightEnable,
    d3d9_device_SetClipPlane,
    d3d9_device_GetClipPlane,
    d3d9_device_SetRenderState,
    d3d9_device_GetRenderState,
    d3d9_device_CreateStateBlock,
    d3d9_device_BeginStateBlock,
    d3d9_device_EndStateBlock,
    d3d9_device_SetClipStatus,
    d3d9_device_GetClipStatus,
    d3d9_device_GetTexture,
    d3d9_device_SetTexture,
    d3d9_device_GetTextureStageState,
    d3d9_device_SetTextureStageState,
    d3d9_device_GetSamplerState,
    d3d9_device_SetSamplerState,
    d3d9_device_ValidateDevice,
    d3d9_device_SetPaletteEntries,
    d3d9_device_GetPaletteEntries,
    d3d9_device_SetCurrentTexturePalette,
    d3d9_device_GetCurrentTexturePalette,
    d3d9_device_SetScissorRect,
    d3d9_device_GetScissorRect,
    d3d9_device_SetSoftwareVertexProcessing,
    d3d9_device_GetSoftwareVertexProcessing,
    d3d9_device_SetNPatchMode,
    d3d9_device_GetNPatchMode,
    d3d9_device_DrawPrimitive,
    d3d9_device_DrawIndexedPrimitive,
    d3d9_device_DrawPrimitiveUP,
    d3d9_device_DrawIndexedPrimitiveUP,
    d3d9_device_ProcessVertices,
    d3d9_device_CreateVertexDeclaration,
    d3d9_device_SetVertexDeclaration,
    d3d9_device_GetVertexDeclaration,
    d3d9_device_SetFVF,
    d3d9_device_GetFVF,
    d3d9_device_CreateVertexShader,
    d3d9_device_SetVertexShader,
    d3d9_device_GetVertexShader,
    d3d9_device_SetVertexShaderConstantF,
    d3d9_device_GetVertexShaderConstantF,
    d3d9_device_SetVertexShaderConstantI,
    d3d9_device_GetVertexShaderConstantI,
    d3d9_device_SetVertexShaderConstantB,
    d3d9_device_GetVertexShaderConstantB,
    d3d9_device_SetStreamSource,
    d3d9_device_GetStreamSource,
    d3d9_device_SetStreamSourceFreq,
    d3d9_device_GetStreamSourceFreq,
    d3d9_device_SetIndices,
    d3d9_device_GetIndices,
    d3d9_device_CreatePixelShader,
    d3d9_device_SetPixelShader,
    d3d9_device_GetPixelShader,
    d3d9_device_SetPixelShaderConstantF,
    d3d9_device_GetPixelShaderConstantF,
    d3d9_device_SetPixelShaderConstantI,
    d3d9_device_GetPixelShaderConstantI,
    d3d9_device_SetPixelShaderConstantB,
    d3d9_device_GetPixelShaderConstantB,
    d3d9_device_DrawRectPatch,
    d3d9_device_DrawTriPatch,
    d3d9_device_DeletePatch,
    d3d9_device_CreateQuery,
    /* IDirect3DDevice9Ex */
    d3d9_device_SetConvolutionMonoKernel,
    d3d9_device_ComposeRects,
    d3d9_device_PresentEx,
    d3d9_device_GetGPUThreadPriority,
    d3d9_device_SetGPUThreadPriority,
    d3d9_device_WaitForVBlank,
    d3d9_device_CheckResourceResidency,
    d3d9_device_SetMaximumFrameLatency,
    d3d9_device_GetMaximumFrameLatency,
    d3d9_device_CheckDeviceState,
    d3d9_device_CreateRenderTargetEx,
    d3d9_device_CreateOffscreenPlainSurfaceEx,
    d3d9_device_CreateDepthStencilSurfaceEx,
    d3d9_device_ResetEx,
    d3d9_device_GetDisplayModeEx,
};

static inline struct d3d9_device *device_from_device_parent(struct wined3d_device_parent *device_parent)
{
    return CONTAINING_RECORD(device_parent, struct d3d9_device, device_parent);
}

static void CDECL device_parent_wined3d_device_created(struct wined3d_device_parent *device_parent,
        struct wined3d_device *device)
{
    TRACE("device_parent %p, device %p.\n", device_parent, device);
}

static void CDECL device_parent_mode_changed(struct wined3d_device_parent *device_parent)
{
    TRACE("device_parent %p.\n", device_parent);
}

static void CDECL device_parent_activate(struct wined3d_device_parent *device_parent, BOOL activate)
{
    struct d3d9_device *device = device_from_device_parent(device_parent);

    TRACE("device_parent %p, activate %#x.\n", device_parent, activate);

    if (!device->d3d_parent)
        return;

    if (!activate)
        InterlockedCompareExchange(&device->device_state, D3D9_DEVICE_STATE_LOST, D3D9_DEVICE_STATE_OK);
    else if (device->d3d_parent->extended)
        InterlockedCompareExchange(&device->device_state, D3D9_DEVICE_STATE_OK, D3D9_DEVICE_STATE_LOST);
    else
        InterlockedCompareExchange(&device->device_state, D3D9_DEVICE_STATE_NOT_RESET, D3D9_DEVICE_STATE_LOST);
}

static HRESULT CDECL device_parent_texture_sub_resource_created(struct wined3d_device_parent *device_parent,
        enum wined3d_resource_type type, struct wined3d_texture *wined3d_texture, unsigned int sub_resource_idx,
        void **parent, const struct wined3d_parent_ops **parent_ops)
{
    TRACE("device_parent %p, type %#x, wined3d_texture %p, sub_resource_idx %u, parent %p, parent_ops %p.\n",
            device_parent, type, wined3d_texture, sub_resource_idx, parent, parent_ops);

    if (type == WINED3D_RTYPE_TEXTURE_3D)
    {
        struct d3d9_volume *d3d_volume;

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

static const struct wined3d_device_parent_ops d3d9_wined3d_device_parent_ops =
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

HRESULT device_init(struct d3d9_device *device, struct d3d9 *parent, struct wined3d *wined3d,
        UINT adapter, D3DDEVTYPE device_type, HWND focus_window, DWORD flags,
        D3DPRESENT_PARAMETERS *parameters, D3DDISPLAYMODEEX *mode)
{
    struct wined3d_swapchain_desc *swapchain_desc;
    struct wined3d_adapter *wined3d_adapter;
    struct d3d9_swapchain *d3d_swapchain;
    struct wined3d_caps wined3d_caps;
    unsigned int output_idx;
    unsigned i, count = 1;
    D3DCAPS9 caps;
    HRESULT hr;

    static const enum wined3d_feature_level feature_levels[] =
    {
        WINED3D_FEATURE_LEVEL_9_3,
        WINED3D_FEATURE_LEVEL_9_2,
        WINED3D_FEATURE_LEVEL_9_1,
        WINED3D_FEATURE_LEVEL_8,
        WINED3D_FEATURE_LEVEL_7,
        WINED3D_FEATURE_LEVEL_6,
        WINED3D_FEATURE_LEVEL_5,
    };

    output_idx = adapter;
    if (output_idx >= parent->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (mode)
        FIXME("Ignoring display mode.\n");

    device->IDirect3DDevice9Ex_iface.lpVtbl = &d3d9_device_vtbl;
    device->IDirect3DDevice9On12_iface.lpVtbl = &d3d9on12_vtbl;
    device->device_parent.ops = &d3d9_wined3d_device_parent_ops;
    device->adapter_ordinal = adapter;
    device->refcount = 1;

    if (!(flags & D3DCREATE_FPU_PRESERVE)) setup_fpu();

    wined3d_mutex_lock();
    wined3d_adapter = wined3d_output_get_adapter(parent->wined3d_outputs[output_idx]);
    if (FAILED(hr = wined3d_device_create(wined3d, wined3d_adapter, wined3d_device_type_from_d3d(device_type),
            focus_window, flags, 4, feature_levels, ARRAY_SIZE(feature_levels),
            &device->device_parent, &device->wined3d_device)))
    {
        WARN("Failed to create wined3d device, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    device->immediate_context = wined3d_device_get_immediate_context(device->wined3d_device);
    wined3d_get_device_caps(wined3d_adapter, wined3d_device_type_from_d3d(device_type), &wined3d_caps);
    d3d9_caps_from_wined3dcaps(parent, adapter, &caps, &wined3d_caps);
    device->max_user_clip_planes = caps.MaxUserClipPlanes;
    device->vs_uniform_count = caps.MaxVertexShaderConst;
    if (flags & D3DCREATE_ADAPTERGROUP_DEVICE)
        count = caps.NumberOfAdaptersInGroup;

    if (FAILED(hr = wined3d_stateblock_create(device->wined3d_device, NULL, WINED3D_SBT_PRIMARY, &device->state)))
    {
        ERR("Failed to create the primary stateblock, hr %#lx.\n", hr);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        return hr;
    }
    device->stateblock_state = wined3d_stateblock_get_state(device->state);
    device->update_state = device->state;

    wined3d_streaming_buffer_init(&device->vertex_buffer, WINED3D_BIND_VERTEX_BUFFER);
    wined3d_streaming_buffer_init(&device->index_buffer, WINED3D_BIND_INDEX_BUFFER);

    if (flags & D3DCREATE_MULTITHREADED)
        wined3d_device_set_multithreaded(device->wined3d_device);

    if (!parameters->Windowed)
    {
        if (!focus_window)
            focus_window = parameters->hDeviceWindow;
        if (FAILED(hr = wined3d_device_acquire_focus_window(device->wined3d_device, focus_window)))
        {
            ERR("Failed to acquire focus window, hr %#lx.\n", hr);
            wined3d_device_decref(device->wined3d_device);
            wined3d_mutex_unlock();
            return hr;
        }
    }

    if (!(swapchain_desc = malloc(sizeof(*swapchain_desc) * count)))
    {
        ERR("Failed to allocate wined3d parameters.\n");
        wined3d_device_release_focus_window(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        if (!wined3d_swapchain_desc_from_d3d9(&swapchain_desc[i],
                parent->wined3d_outputs[output_idx + i], &parameters[i], parent->extended))
        {
            wined3d_device_release_focus_window(device->wined3d_device);
            wined3d_device_decref(device->wined3d_device);
            free(swapchain_desc);
            wined3d_mutex_unlock();
            return D3DERR_INVALIDCALL;
        }
        swapchain_desc[i].flags |= WINED3D_SWAPCHAIN_IMPLICIT;
        if (flags & D3DCREATE_NOWINDOWCHANGES)
            swapchain_desc[i].flags |= WINED3D_SWAPCHAIN_NO_WINDOW_CHANGES;
    }

    if (FAILED(hr = d3d9_swapchain_create(device, swapchain_desc,
            wined3dswapinterval_from_d3d(parameters->PresentationInterval), &d3d_swapchain)))
    {
        WARN("Failed to create swapchain, hr %#lx.\n", hr);
        wined3d_device_release_focus_window(device->wined3d_device);
        free(swapchain_desc);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        return hr;
    }

    wined3d_swapchain_incref(d3d_swapchain->wined3d_swapchain);
    IDirect3DSwapChain9Ex_Release(&d3d_swapchain->IDirect3DSwapChain9Ex_iface);

    wined3d_stateblock_set_render_state(device->state, WINED3D_RS_ZENABLE,
            !!swapchain_desc->enable_auto_depth_stencil);
    device_reset_viewport_state(device);

    if (FAILED(hr = d3d9_device_get_swapchains(device)))
    {
        wined3d_swapchain_decref(d3d_swapchain->wined3d_swapchain);
        wined3d_device_release_focus_window(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        free(swapchain_desc);
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        present_parameters_from_wined3d_swapchain_desc(&parameters[i],
                &swapchain_desc[i], parameters[i].PresentationInterval);
    }

    wined3d_mutex_unlock();

    free(swapchain_desc);

    /* We could also simply ignore the initial rendertarget since it's known
     * not to be a texture (we currently use these only for automatic mipmap
     * generation). */
    wined3d_mutex_lock();
    device->render_targets[0] = wined3d_rendertarget_view_get_sub_resource_parent(
            wined3d_device_context_get_rendertarget_view(device->immediate_context, 0));
    wined3d_mutex_unlock();

    IDirect3D9Ex_AddRef(&parent->IDirect3D9Ex_iface);
    device->d3d_parent = parent;

    return D3D_OK;
}
