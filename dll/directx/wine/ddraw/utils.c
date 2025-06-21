/*
 * DirectDraw helper functions
 *
 * Copyright (c) 1997-2000 Marcus Meissner
 * Copyright (c) 1998 Lionel Ulmer
 * Copyright (c) 2000 TransGaming Technologies Inc.
 * Copyright (c) 2006 Stefan Dösinger
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static void DDRAW_dump_pixelformat(const DDPIXELFORMAT *pf);

void ddrawformat_from_wined3dformat(DDPIXELFORMAT *DDPixelFormat, enum wined3d_format_id wined3d_format)
{
    DWORD Size = DDPixelFormat->dwSize;

    if(Size==0) return;

    memset(DDPixelFormat, 0x00, Size);
    DDPixelFormat->dwSize = Size;
    switch (wined3d_format)
    {
        case WINED3DFMT_B8G8R8_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 24;
            DDPixelFormat->dwRBitMask = 0x00ff0000;
            DDPixelFormat->dwGBitMask = 0x0000ff00;
            DDPixelFormat->dwBBitMask = 0x000000ff;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_B8G8R8A8_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 32;
            DDPixelFormat->dwRBitMask = 0x00ff0000;
            DDPixelFormat->dwGBitMask = 0x0000ff00;
            DDPixelFormat->dwBBitMask = 0x000000ff;
            DDPixelFormat->dwRGBAlphaBitMask = 0xff000000;
            break;

        case WINED3DFMT_B8G8R8X8_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 32;
            DDPixelFormat->dwRBitMask = 0x00ff0000;
            DDPixelFormat->dwGBitMask = 0x0000ff00;
            DDPixelFormat->dwBBitMask = 0x000000ff;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_R8G8B8X8_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 32;
            DDPixelFormat->dwRBitMask = 0x000000ff;
            DDPixelFormat->dwGBitMask = 0x0000ff00;
            DDPixelFormat->dwBBitMask = 0x00ff0000;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_B5G6R5_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 16;
            DDPixelFormat->dwRBitMask = 0xF800;
            DDPixelFormat->dwGBitMask = 0x07E0;
            DDPixelFormat->dwBBitMask = 0x001F;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_B5G5R5X1_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 16;
            DDPixelFormat->dwRBitMask = 0x7C00;
            DDPixelFormat->dwGBitMask = 0x03E0;
            DDPixelFormat->dwBBitMask = 0x001F;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_B5G5R5A1_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 16;
            DDPixelFormat->dwRBitMask = 0x7C00;
            DDPixelFormat->dwGBitMask = 0x03E0;
            DDPixelFormat->dwBBitMask = 0x001F;
            DDPixelFormat->dwRGBAlphaBitMask = 0x8000;
            break;

        case WINED3DFMT_B4G4R4A4_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 16;
            DDPixelFormat->dwRBitMask = 0x0F00;
            DDPixelFormat->dwGBitMask = 0x00F0;
            DDPixelFormat->dwBBitMask = 0x000F;
            DDPixelFormat->dwRGBAlphaBitMask = 0xF000;
            break;

        case WINED3DFMT_B2G3R3_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 8;
            DDPixelFormat->dwRBitMask = 0xE0;
            DDPixelFormat->dwGBitMask = 0x1C;
            DDPixelFormat->dwBBitMask = 0x03;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_P8_UINT:
            DDPixelFormat->dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 8;
            DDPixelFormat->dwRBitMask = 0x00;
            DDPixelFormat->dwGBitMask = 0x00;
            DDPixelFormat->dwBBitMask = 0x00;
            break;

        case WINED3DFMT_A8_UNORM:
            DDPixelFormat->dwFlags = DDPF_ALPHA;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwAlphaBitDepth = 8;
            DDPixelFormat->dwRBitMask = 0x0;
            DDPixelFormat->dwZBitMask = 0x0;
            DDPixelFormat->dwStencilBitMask = 0x0;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_B2G3R3A8_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 16;
            DDPixelFormat->dwRBitMask = 0x00E0;
            DDPixelFormat->dwGBitMask = 0x001C;
            DDPixelFormat->dwBBitMask = 0x0003;
            DDPixelFormat->dwRGBAlphaBitMask = 0xFF00;
            break;

        case WINED3DFMT_B4G4R4X4_UNORM:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwRGBBitCount = 16;
            DDPixelFormat->dwRBitMask = 0x0F00;
            DDPixelFormat->dwGBitMask = 0x00F0;
            DDPixelFormat->dwBBitMask = 0x000F;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        /* How are Z buffer bit depth and Stencil buffer bit depth related?
         */
        case WINED3DFMT_D16_UNORM:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwZBufferBitDepth = 16;
            DDPixelFormat->dwStencilBitDepth = 0;
            DDPixelFormat->dwZBitMask = 0x0000FFFF;
            DDPixelFormat->dwStencilBitMask = 0x0;
            DDPixelFormat->dwRGBZBitMask = 0x00000000;
            break;

        case WINED3DFMT_D32_UNORM:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwZBufferBitDepth = 32;
            DDPixelFormat->dwStencilBitDepth = 0;
            DDPixelFormat->dwZBitMask = 0xFFFFFFFF;
            DDPixelFormat->dwStencilBitMask = 0x0;
            DDPixelFormat->dwRGBZBitMask = 0x00000000;
            break;

        case WINED3DFMT_S4X4_UINT_D24_UNORM:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
            DDPixelFormat->dwFourCC = 0;
            /* Should I set dwZBufferBitDepth to 32 here? */
            DDPixelFormat->dwZBufferBitDepth = 32;
            DDPixelFormat->dwStencilBitDepth = 4;
            DDPixelFormat->dwZBitMask = 0x00FFFFFF;
            DDPixelFormat->dwStencilBitMask = 0x0F000000;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_D24_UNORM_S8_UINT:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwZBufferBitDepth = 32;
            DDPixelFormat->dwStencilBitDepth = 8;
            DDPixelFormat->dwZBitMask = 0x00FFFFFF;
            DDPixelFormat->dwStencilBitMask = 0xFF000000;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_X8D24_UNORM:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwZBufferBitDepth = 32;
            DDPixelFormat->dwStencilBitDepth = 0;
            DDPixelFormat->dwZBitMask = 0x00FFFFFF;
            DDPixelFormat->dwStencilBitMask = 0x00000000;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_S1_UINT_D15_UNORM:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwZBufferBitDepth = 16;
            DDPixelFormat->dwStencilBitDepth = 1;
            DDPixelFormat->dwZBitMask = 0x7fff;
            DDPixelFormat->dwStencilBitMask = 0x8000;
            DDPixelFormat->dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_UYVY:
        case WINED3DFMT_YUY2:
            DDPixelFormat->dwYUVBitCount = 16;
            DDPixelFormat->dwFlags = DDPF_FOURCC;
            DDPixelFormat->dwFourCC = wined3d_format;
            break;

        case WINED3DFMT_YV12:
            DDPixelFormat->dwYUVBitCount = 12;
            DDPixelFormat->dwFlags = DDPF_FOURCC;
            DDPixelFormat->dwFourCC = wined3d_format;
            break;

        case WINED3DFMT_DXT1:
        case WINED3DFMT_DXT2:
        case WINED3DFMT_DXT3:
        case WINED3DFMT_DXT4:
        case WINED3DFMT_DXT5:
        case WINED3DFMT_MULTI2_ARGB8:
        case WINED3DFMT_G8R8_G8B8:
        case WINED3DFMT_R8G8_B8G8:
            DDPixelFormat->dwFlags = DDPF_FOURCC;
            DDPixelFormat->dwFourCC = wined3d_format;
            break;

        /* Luminance */
        case WINED3DFMT_L8_UNORM:
            DDPixelFormat->dwFlags = DDPF_LUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwLuminanceBitCount = 8;
            DDPixelFormat->dwLuminanceBitMask = 0xff;
            DDPixelFormat->dwBumpDvBitMask = 0x0;
            DDPixelFormat->dwBumpLuminanceBitMask = 0x0;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_L4A4_UNORM:
            DDPixelFormat->dwFlags = DDPF_ALPHAPIXELS | DDPF_LUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwLuminanceBitCount = 4;
            DDPixelFormat->dwLuminanceBitMask = 0x0f;
            DDPixelFormat->dwBumpDvBitMask = 0x0;
            DDPixelFormat->dwBumpLuminanceBitMask = 0x0;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0xf0;
            break;

        case WINED3DFMT_L8A8_UNORM:
            DDPixelFormat->dwFlags = DDPF_ALPHAPIXELS | DDPF_LUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwLuminanceBitCount = 16;
            DDPixelFormat->dwLuminanceBitMask = 0x00ff;
            DDPixelFormat->dwBumpDvBitMask = 0x0;
            DDPixelFormat->dwBumpLuminanceBitMask = 0x0;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0xff00;
            break;

        /* Bump mapping */
        case WINED3DFMT_R8G8_SNORM:
            DDPixelFormat->dwFlags = DDPF_BUMPDUDV;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwBumpBitCount = 16;
            DDPixelFormat->dwBumpDuBitMask =         0x000000ff;
            DDPixelFormat->dwBumpDvBitMask =         0x0000ff00;
            DDPixelFormat->dwBumpLuminanceBitMask =  0x00000000;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0x00000000;
            break;

        case WINED3DFMT_R5G5_SNORM_L6_UNORM:
            DDPixelFormat->dwFlags = DDPF_BUMPDUDV | DDPF_BUMPLUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwBumpBitCount = 16;
            DDPixelFormat->dwBumpDuBitMask =         0x0000001f;
            DDPixelFormat->dwBumpDvBitMask =         0x000003e0;
            DDPixelFormat->dwBumpLuminanceBitMask =  0x0000fc00;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0x00000000;
            break;

        case WINED3DFMT_R8G8_SNORM_L8X8_UNORM:
            DDPixelFormat->dwFlags = DDPF_BUMPDUDV | DDPF_BUMPLUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->dwBumpBitCount = 32;
            DDPixelFormat->dwBumpDuBitMask =         0x000000ff;
            DDPixelFormat->dwBumpDvBitMask =         0x0000ff00;
            DDPixelFormat->dwBumpLuminanceBitMask =  0x00ff0000;
            DDPixelFormat->dwLuminanceAlphaBitMask = 0x00000000;
            break;

        default:
            FIXME("Unhandled wined3d format %#x.\n", wined3d_format);
            break;
    }

    if(TRACE_ON(ddraw)) {
        TRACE("Returning: ");
        DDRAW_dump_pixelformat(DDPixelFormat);
    }
}

enum wined3d_format_id wined3dformat_from_ddrawformat(const DDPIXELFORMAT *DDPixelFormat)
{
    TRACE("Convert a DirectDraw Pixelformat to a WineD3D Pixelformat\n");
    if(TRACE_ON(ddraw))
    {
        DDRAW_dump_pixelformat(DDPixelFormat);
    }

    if(DDPixelFormat->dwFlags & DDPF_PALETTEINDEXED8)
    {
        return WINED3DFMT_P8_UINT;
    }
    else if(DDPixelFormat->dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4) )
    {
        FIXME("DDPF_PALETTEINDEXED1 to DDPF_PALETTEINDEXED4 are not supported by WineD3D (yet). Returning WINED3DFMT_P8\n");
        return WINED3DFMT_P8_UINT;
    }
    else if(DDPixelFormat->dwFlags & DDPF_RGB)
    {
        switch(DDPixelFormat->dwRGBBitCount)
        {
            case 8:
                /* This is the only format that can match here */
                return WINED3DFMT_B2G3R3_UNORM;

            case 16:
                /* Read the Color masks */
                if( (DDPixelFormat->dwRBitMask == 0xF800) &&
                    (DDPixelFormat->dwGBitMask == 0x07E0) &&
                    (DDPixelFormat->dwBBitMask == 0x001F) )
                {
                    return WINED3DFMT_B5G6R5_UNORM;
                }

                if( (DDPixelFormat->dwRBitMask == 0x7C00) &&
                    (DDPixelFormat->dwGBitMask == 0x03E0) &&
                    (DDPixelFormat->dwBBitMask == 0x001F) )
                {
                    if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                        (DDPixelFormat->dwRGBAlphaBitMask == 0x8000))
                        return WINED3DFMT_B5G5R5A1_UNORM;
                    else
                        return WINED3DFMT_B5G5R5X1_UNORM;
                }

                if( (DDPixelFormat->dwRBitMask == 0x0F00) &&
                    (DDPixelFormat->dwGBitMask == 0x00F0) &&
                    (DDPixelFormat->dwBBitMask == 0x000F) )
                {
                    if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                        (DDPixelFormat->dwRGBAlphaBitMask == 0xF000))
                        return WINED3DFMT_B4G4R4A4_UNORM;
                    else
                        return WINED3DFMT_B4G4R4X4_UNORM;
                }

                if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                    (DDPixelFormat->dwRGBAlphaBitMask == 0xFF00) &&
                    (DDPixelFormat->dwRBitMask == 0x00E0) &&
                    (DDPixelFormat->dwGBitMask == 0x001C) &&
                    (DDPixelFormat->dwBBitMask == 0x0003) )
                {
                    return WINED3DFMT_B2G3R3A8_UNORM;
                }
                WARN("16 bit RGB Pixel format does not match.\n");
                return WINED3DFMT_UNKNOWN;

            case 24:
                return WINED3DFMT_B8G8R8_UNORM;

            case 32:
                /* Read the Color masks */
                if( (DDPixelFormat->dwRBitMask == 0x00FF0000) &&
                    (DDPixelFormat->dwGBitMask == 0x0000FF00) &&
                    (DDPixelFormat->dwBBitMask == 0x000000FF) )
                {
                    if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                        (DDPixelFormat->dwRGBAlphaBitMask == 0xFF000000))
                        return WINED3DFMT_B8G8R8A8_UNORM;
                    else
                        return WINED3DFMT_B8G8R8X8_UNORM;

                }
                WARN("32 bit RGB pixel format does not match.\n");
                return WINED3DFMT_UNKNOWN;

            default:
                WARN("Invalid dwRGBBitCount in Pixelformat structure.\n");
                return WINED3DFMT_UNKNOWN;
        }
    }
    else if( (DDPixelFormat->dwFlags & DDPF_ALPHA) )
    {
        /* Alpha only Pixelformat */
        switch(DDPixelFormat->dwAlphaBitDepth)
        {
            case 8:
                return WINED3DFMT_A8_UNORM;

            default:
                WARN("Invalid AlphaBitDepth in Alpha-Only Pixelformat.\n");
                return WINED3DFMT_UNKNOWN;
        }
    }
    else if(DDPixelFormat->dwFlags & DDPF_LUMINANCE)
    {
        /* Luminance-only or luminance-alpha */
        if(DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS)
        {
            /* Luminance with Alpha */
            switch(DDPixelFormat->dwLuminanceBitCount)
            {
                case 4:
                    if(DDPixelFormat->dwAlphaBitDepth == 4)
                        return WINED3DFMT_L4A4_UNORM;
                    WARN("Unknown Alpha / Luminance bit depth combination.\n");
                    return WINED3DFMT_UNKNOWN;

                case 6:
                    FIXME("A luminance Pixelformat shouldn't have 6 luminance bits. Returning D3DFMT_L6V5U5 for now.\n");
                    return WINED3DFMT_R5G5_SNORM_L6_UNORM;

                case 8:
                    if(DDPixelFormat->dwAlphaBitDepth == 8)
                        return WINED3DFMT_L8A8_UNORM;
                    WARN("Unknown Alpha / Lumincase bit depth combination.\n");
                    return WINED3DFMT_UNKNOWN;
            }
        }
        else
        {
            /* Luminance-only */
            switch(DDPixelFormat->dwLuminanceBitCount)
            {
                case 6:
                    FIXME("A luminance Pixelformat shouldn't have 6 luminance bits. Returning D3DFMT_L6V5U5 for now.\n");
                    return WINED3DFMT_R5G5_SNORM_L6_UNORM;

                case 8:
                    return WINED3DFMT_L8_UNORM;

                default:
                    WARN("Unknown luminance-only bit depth %lu.\n", DDPixelFormat->dwLuminanceBitCount);
                    return WINED3DFMT_UNKNOWN;
             }
        }
    }
    else if(DDPixelFormat->dwFlags & DDPF_ZBUFFER)
    {
        /* Z buffer */
        if(DDPixelFormat->dwFlags & DDPF_STENCILBUFFER)
        {
            switch(DDPixelFormat->dwZBufferBitDepth)
            {
                case 16:
                    if (DDPixelFormat->dwStencilBitDepth == 1) return WINED3DFMT_S1_UINT_D15_UNORM;
                    WARN("Unknown depth stencil format: 16 z bits, %lu stencil bits.\n",
                            DDPixelFormat->dwStencilBitDepth);
                    return WINED3DFMT_UNKNOWN;

                case 32:
                    if (DDPixelFormat->dwStencilBitDepth == 8) return WINED3DFMT_D24_UNORM_S8_UINT;
                    else if (DDPixelFormat->dwStencilBitDepth == 4) return WINED3DFMT_S4X4_UINT_D24_UNORM;
                    WARN("Unknown depth stencil format: 32 z bits, %lu stencil bits.\n",
                            DDPixelFormat->dwStencilBitDepth);
                    return WINED3DFMT_UNKNOWN;

                default:
                    WARN("Unknown depth stencil format: %lu z bits, %lu stencil bits.\n",
                            DDPixelFormat->dwZBufferBitDepth, DDPixelFormat->dwStencilBitDepth);
                    return WINED3DFMT_UNKNOWN;
            }
        }
        else
        {
            switch(DDPixelFormat->dwZBufferBitDepth)
            {
                case 16:
                    return WINED3DFMT_D16_UNORM;

                case 24:
                    return WINED3DFMT_X8D24_UNORM;

                case 32:
                    if (DDPixelFormat->dwZBitMask == 0x00FFFFFF) return WINED3DFMT_X8D24_UNORM;
                    else if (DDPixelFormat->dwZBitMask == 0xFFFFFF00) return WINED3DFMT_X8D24_UNORM;
                    else if (DDPixelFormat->dwZBitMask == 0xFFFFFFFF) return WINED3DFMT_D32_UNORM;
                    WARN("Unknown depth-only format: 32 z bits, mask 0x%08lx\n",
                            DDPixelFormat->dwZBitMask);
                    return WINED3DFMT_UNKNOWN;

                default:
                    WARN("Unknown depth-only format: %lu z bits, mask 0x%08lx\n",
                            DDPixelFormat->dwZBufferBitDepth, DDPixelFormat->dwZBitMask);
                    return WINED3DFMT_UNKNOWN;
            }
        }
    }
    else if(DDPixelFormat->dwFlags & DDPF_FOURCC)
    {
        return DDPixelFormat->dwFourCC;
    }
    else if(DDPixelFormat->dwFlags & DDPF_BUMPDUDV)
    {
        if( (DDPixelFormat->dwBumpBitCount         == 16        ) &&
            (DDPixelFormat->dwBumpDuBitMask        == 0x000000ff) &&
            (DDPixelFormat->dwBumpDvBitMask        == 0x0000ff00) &&
            (DDPixelFormat->dwBumpLuminanceBitMask == 0x00000000) )
        {
            return WINED3DFMT_R8G8_SNORM;
        }
        else if ( (DDPixelFormat->dwBumpBitCount         == 16        ) &&
                  (DDPixelFormat->dwBumpDuBitMask        == 0x0000001f) &&
                  (DDPixelFormat->dwBumpDvBitMask        == 0x000003e0) &&
                  (DDPixelFormat->dwBumpLuminanceBitMask == 0x0000fc00) )
        {
            return WINED3DFMT_R5G5_SNORM_L6_UNORM;
        }
        else if ( (DDPixelFormat->dwBumpBitCount         == 32        ) &&
                  (DDPixelFormat->dwBumpDuBitMask        == 0x000000ff) &&
                  (DDPixelFormat->dwBumpDvBitMask        == 0x0000ff00) &&
                  (DDPixelFormat->dwBumpLuminanceBitMask == 0x00ff0000) )
        {
            return WINED3DFMT_R8G8_SNORM_L8X8_UNORM;
        }
    }

    WARN("Unknown Pixelformat.\n");
    return WINED3DFMT_UNKNOWN;
}

unsigned int wined3dmapflags_from_ddrawmapflags(unsigned int flags)
{
    static const unsigned int handled = DDLOCK_NOSYSLOCK
            | DDLOCK_NOOVERWRITE
            | DDLOCK_DISCARDCONTENTS
            | DDLOCK_DONOTWAIT;
    unsigned int wined3d_flags;

    wined3d_flags = flags & handled;
    if (!(flags & (DDLOCK_NOOVERWRITE | DDLOCK_DISCARDCONTENTS | DDLOCK_WRITEONLY)))
        wined3d_flags |= WINED3D_MAP_READ;
    if (!(flags & DDLOCK_READONLY))
        wined3d_flags |= WINED3D_MAP_WRITE;
    if (!(wined3d_flags & (WINED3D_MAP_READ | WINED3D_MAP_WRITE)))
        wined3d_flags |= WINED3D_MAP_READ | WINED3D_MAP_WRITE;
    if (flags & DDLOCK_NODIRTYUPDATE)
        wined3d_flags |= WINED3D_MAP_NO_DIRTY_UPDATE;
    flags &= ~(handled | DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_WRITEONLY | DDLOCK_NODIRTYUPDATE);

    if (flags)
        FIXME("Unhandled flags %#x.\n", flags);

    return wined3d_flags;
}

static float colour_to_float(DWORD colour, DWORD mask)
{
    if (!mask)
        return 0.0f;
    return (float)(colour & mask) / (float)mask;
}

BOOL wined3d_colour_from_ddraw_colour(const DDPIXELFORMAT *pf, const struct ddraw_palette *palette,
        DWORD colour, struct wined3d_color *wined3d_colour)
{
    if (pf->dwFlags & DDPF_ALPHA)
    {
        DWORD size, mask;

        size = pf->dwAlphaBitDepth;
        mask = size < 32 ? (1u << size) - 1 : ~0u;
        wined3d_colour->r = 0.0f;
        wined3d_colour->g = 0.0f;
        wined3d_colour->b = 0.0f;
        wined3d_colour->a = colour_to_float(colour, mask);
        return TRUE;
    }

    if (pf->dwFlags & DDPF_FOURCC)
    {
        WARN("FourCC formats not supported.\n");
        goto fail;
    }

    if (pf->dwFlags & DDPF_PALETTEINDEXED8)
    {
        PALETTEENTRY entry;

        colour &= 0xff;
        if (!palette || FAILED(wined3d_palette_get_entries(palette->wined3d_palette, 0, colour, 1, &entry)))
        {
            wined3d_colour->r = 0.0f;
            wined3d_colour->g = 0.0f;
            wined3d_colour->b = 0.0f;
        }
        else
        {
            wined3d_colour->r = entry.peRed / 255.0f;
            wined3d_colour->g = entry.peGreen / 255.0f;
            wined3d_colour->b = entry.peBlue / 255.0f;
        }
        wined3d_colour->a = colour / 255.0f;
        return TRUE;
    }

    if (pf->dwFlags & DDPF_RGB)
    {
        wined3d_colour->r = colour_to_float(colour, pf->dwRBitMask);
        wined3d_colour->g = colour_to_float(colour, pf->dwGBitMask);
        wined3d_colour->b = colour_to_float(colour, pf->dwBBitMask);
        if (pf->dwFlags & DDPF_ALPHAPIXELS)
            wined3d_colour->a = colour_to_float(colour, pf->dwRGBAlphaBitMask);
        else
            wined3d_colour->a = 0.0f;
        return TRUE;
    }

    if (pf->dwFlags & DDPF_ZBUFFER)
    {
        wined3d_colour->r = colour_to_float(colour, pf->dwZBitMask);
        if (pf->dwFlags & DDPF_STENCILBUFFER)
            wined3d_colour->g = colour_to_float(colour, pf->dwStencilBitMask);
        else
            wined3d_colour->g = 0.0f;
        wined3d_colour->b = 0.0f;
        wined3d_colour->a = 0.0f;
        return TRUE;
    }

    FIXME("Unhandled pixel format.\n");
    DDRAW_dump_pixelformat(pf);

fail:
    wined3d_colour->r = 0.0f;
    wined3d_colour->g = 0.0f;
    wined3d_colour->b = 0.0f;
    wined3d_colour->a = 0.0f;

    return FALSE;
}

/*****************************************************************************
 * Various dumping functions.
 *
 * They write the contents of a specific function to a TRACE.
 *
 *****************************************************************************/
static void
DDRAW_dump_DWORD(const void *in)
{
    TRACE("%ld\n", *((const DWORD *) in));
}
static void
DDRAW_dump_PTR(const void *in)
{
    TRACE("%p\n", *((const void * const*) in));
}
static void
DDRAW_dump_DDCOLORKEY(const DDCOLORKEY *ddck)
{
    TRACE("Low : 0x%08lx  - High : 0x%08lx\n", ddck->dwColorSpaceLowValue, ddck->dwColorSpaceHighValue);
}

static void DDRAW_dump_flags_nolf(DWORD flags, const struct flag_info *names, size_t num_names)
{
    unsigned int i;

    for (i=0; i < num_names; i++)
        if ((flags & names[i].val) ||      /* standard flag value */
            ((!flags) && (!names[i].val))) /* zero value only */
            TRACE("%s ", names[i].name);
}

static void DDRAW_dump_flags(DWORD flags, const struct flag_info *names, size_t num_names)
{
    DDRAW_dump_flags_nolf(flags, names, num_names);
    TRACE("\n");
}

void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in)
{
    static const struct flag_info flags[] =
    {
        FE(DDSCAPS_RESERVED1),
        FE(DDSCAPS_ALPHA),
        FE(DDSCAPS_BACKBUFFER),
        FE(DDSCAPS_COMPLEX),
        FE(DDSCAPS_FLIP),
        FE(DDSCAPS_FRONTBUFFER),
        FE(DDSCAPS_OFFSCREENPLAIN),
        FE(DDSCAPS_OVERLAY),
        FE(DDSCAPS_PALETTE),
        FE(DDSCAPS_PRIMARYSURFACE),
        FE(DDSCAPS_PRIMARYSURFACELEFT),
        FE(DDSCAPS_SYSTEMMEMORY),
        FE(DDSCAPS_TEXTURE),
        FE(DDSCAPS_3DDEVICE),
        FE(DDSCAPS_VIDEOMEMORY),
        FE(DDSCAPS_VISIBLE),
        FE(DDSCAPS_WRITEONLY),
        FE(DDSCAPS_ZBUFFER),
        FE(DDSCAPS_OWNDC),
        FE(DDSCAPS_LIVEVIDEO),
        FE(DDSCAPS_HWCODEC),
        FE(DDSCAPS_MODEX),
        FE(DDSCAPS_MIPMAP),
        FE(DDSCAPS_RESERVED2),
        FE(DDSCAPS_ALLOCONLOAD),
        FE(DDSCAPS_VIDEOPORT),
        FE(DDSCAPS_LOCALVIDMEM),
        FE(DDSCAPS_NONLOCALVIDMEM),
        FE(DDSCAPS_STANDARDVGAMODE),
        FE(DDSCAPS_OPTIMIZED)
    };
    static const struct flag_info flags2[] =
    {
        FE(DDSCAPS2_HARDWAREDEINTERLACE),
        FE(DDSCAPS2_HINTDYNAMIC),
        FE(DDSCAPS2_HINTSTATIC),
        FE(DDSCAPS2_TEXTUREMANAGE),
        FE(DDSCAPS2_RESERVED1),
        FE(DDSCAPS2_RESERVED2),
        FE(DDSCAPS2_OPAQUE),
        FE(DDSCAPS2_HINTANTIALIASING),
        FE(DDSCAPS2_CUBEMAP),
        FE(DDSCAPS2_CUBEMAP_POSITIVEX),
        FE(DDSCAPS2_CUBEMAP_NEGATIVEX),
        FE(DDSCAPS2_CUBEMAP_POSITIVEY),
        FE(DDSCAPS2_CUBEMAP_NEGATIVEY),
        FE(DDSCAPS2_CUBEMAP_POSITIVEZ),
        FE(DDSCAPS2_CUBEMAP_NEGATIVEZ),
        FE(DDSCAPS2_MIPMAPSUBLEVEL),
        FE(DDSCAPS2_D3DTEXTUREMANAGE),
        FE(DDSCAPS2_DONOTPERSIST),
        FE(DDSCAPS2_STEREOSURFACELEFT)
    };

    DDRAW_dump_flags_nolf(in->dwCaps, flags, ARRAY_SIZE(flags));
    DDRAW_dump_flags(in->dwCaps2, flags2, ARRAY_SIZE(flags2));
}

static void
DDRAW_dump_DDSCAPS(const DDSCAPS *in)
{
    DDSCAPS2 in_bis;

    in_bis.dwCaps = in->dwCaps;
    in_bis.dwCaps2 = 0;
    in_bis.dwCaps3 = 0;
    in_bis.dwCaps4 = 0;

    DDRAW_dump_DDSCAPS2(&in_bis);
}

static void
DDRAW_dump_pixelformat_flag(DWORD flagmask)
{
    static const struct flag_info flags[] =
    {
        FE(DDPF_ALPHAPIXELS),
        FE(DDPF_ALPHA),
        FE(DDPF_FOURCC),
        FE(DDPF_PALETTEINDEXED4),
        FE(DDPF_PALETTEINDEXEDTO8),
        FE(DDPF_PALETTEINDEXED8),
        FE(DDPF_RGB),
        FE(DDPF_COMPRESSED),
        FE(DDPF_RGBTOYUV),
        FE(DDPF_YUV),
        FE(DDPF_ZBUFFER),
        FE(DDPF_PALETTEINDEXED1),
        FE(DDPF_PALETTEINDEXED2),
        FE(DDPF_ZPIXELS)
    };

    DDRAW_dump_flags_nolf(flagmask, flags, ARRAY_SIZE(flags));
}

static void DDRAW_dump_members(DWORD flags, const void *data, const struct member_info *mems, size_t num_mems)
{
    unsigned int i;

    for (i=0; i < num_mems; i++)
    {
        if (mems[i].val & flags)
        {
            TRACE(" - %s : ", mems[i].name);
            mems[i].func((const char *)data + mems[i].offset);
        }
    }
}

static void
DDRAW_dump_pixelformat(const DDPIXELFORMAT *pf)
{
    TRACE("( ");
    DDRAW_dump_pixelformat_flag(pf->dwFlags);
    if (pf->dwFlags & DDPF_FOURCC)
        TRACE(", dwFourCC code %s - %lu bits per pixel",
                debugstr_fourcc(pf->dwFourCC),
                pf->dwYUVBitCount);
    if (pf->dwFlags & DDPF_RGB)
    {
        TRACE(", RGB bits: %lu, R 0x%08lx G 0x%08lx B 0x%08lx",
                pf->dwRGBBitCount,
                pf->dwRBitMask,
                pf->dwGBitMask,
                pf->dwBBitMask);
        if (pf->dwFlags & DDPF_ALPHAPIXELS)
            TRACE(" A 0x%08lx", pf->dwRGBAlphaBitMask);
        if (pf->dwFlags & DDPF_ZPIXELS)
            TRACE(" Z 0x%08lx", pf->dwRGBZBitMask);
    }
    if (pf->dwFlags & DDPF_ZBUFFER)
        TRACE(", Z bits: %lu", pf->dwZBufferBitDepth);
    if (pf->dwFlags & DDPF_ALPHA)
        TRACE(", Alpha bits: %lu", pf->dwAlphaBitDepth);
    if (pf->dwFlags & DDPF_BUMPDUDV)
        TRACE(", Bump bits: %lu, U 0x%08lx V 0x%08lx L 0x%08lx",
                pf->dwBumpBitCount,
                pf->dwBumpDuBitMask,
                pf->dwBumpDvBitMask,
                pf->dwBumpLuminanceBitMask);
    TRACE(")\n");
}

void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd)
{
#define STRUCT DDSURFACEDESC2
    static const struct member_info members[] =
    {
        ME(DDSD_HEIGHT, DDRAW_dump_DWORD, dwHeight),
        ME(DDSD_WIDTH, DDRAW_dump_DWORD, dwWidth),
        ME(DDSD_PITCH, DDRAW_dump_DWORD, lPitch),
        ME(DDSD_LINEARSIZE, DDRAW_dump_DWORD, dwLinearSize),
        ME(DDSD_BACKBUFFERCOUNT, DDRAW_dump_DWORD, dwBackBufferCount),
        ME(DDSD_MIPMAPCOUNT, DDRAW_dump_DWORD, dwMipMapCount),
        ME(DDSD_REFRESHRATE, DDRAW_dump_DWORD, dwRefreshRate),
        ME(DDSD_ALPHABITDEPTH, DDRAW_dump_DWORD, dwAlphaBitDepth),
        ME(DDSD_LPSURFACE, DDRAW_dump_PTR, lpSurface),
        ME(DDSD_CKDESTOVERLAY, DDRAW_dump_DDCOLORKEY, ddckCKDestOverlay),
        ME(DDSD_CKDESTBLT, DDRAW_dump_DDCOLORKEY, ddckCKDestBlt),
        ME(DDSD_CKSRCOVERLAY, DDRAW_dump_DDCOLORKEY, ddckCKSrcOverlay),
        ME(DDSD_CKSRCBLT, DDRAW_dump_DDCOLORKEY, ddckCKSrcBlt),
        ME(DDSD_PIXELFORMAT, DDRAW_dump_pixelformat, ddpfPixelFormat)
    };
    static const struct member_info members_caps[] =
    {
        ME(DDSD_CAPS, DDRAW_dump_DDSCAPS, ddsCaps)
    };
    static const struct member_info members_caps2[] =
    {
        ME(DDSD_CAPS, DDRAW_dump_DDSCAPS2, ddsCaps)
    };
#undef STRUCT

    if (NULL == lpddsd)
    {
        TRACE("(null)\n");
    }
    else
    {
      if (lpddsd->dwSize >= sizeof(DDSURFACEDESC2))
      {
          DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members_caps2, 1);
      }
      else
      {
          DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members_caps, 1);
      }
      DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members, ARRAY_SIZE(members));
    }
}

void
dump_D3DMATRIX(const D3DMATRIX *mat)
{
    TRACE("  %f %f %f %f\n", mat->_11, mat->_12, mat->_13, mat->_14);
    TRACE("  %f %f %f %f\n", mat->_21, mat->_22, mat->_23, mat->_24);
    TRACE("  %f %f %f %f\n", mat->_31, mat->_32, mat->_33, mat->_34);
    TRACE("  %f %f %f %f\n", mat->_41, mat->_42, mat->_43, mat->_44);
}

DWORD
get_flexible_vertex_size(DWORD d3dvtVertexType)
{
    DWORD size = 0;
    DWORD i;

    if (d3dvtVertexType & D3DFVF_NORMAL) size += 3 * sizeof(D3DVALUE);
    if (d3dvtVertexType & D3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (d3dvtVertexType & D3DFVF_SPECULAR) size += sizeof(DWORD);
    if (d3dvtVertexType & D3DFVF_RESERVED1) size += sizeof(DWORD);
    switch (d3dvtVertexType & D3DFVF_POSITION_MASK)
    {
        case D3DFVF_XYZ:    size += 3 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZRHW: size += 4 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZB1:  size += 4 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZB2:  size += 5 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZB3:  size += 6 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZB4:  size += 7 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZB5:  size += 8 * sizeof(D3DVALUE); break;
        default: ERR("Unexpected position mask\n");
    }
    for (i = 0; i < GET_TEXCOUNT_FROM_FVF(d3dvtVertexType); i++)
    {
        size += GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, i) * sizeof(D3DVALUE);
    }

    return size;
}

void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS* pIn, DDSCAPS2* pOut)
{
    /* 2 adds three additional caps fields to the end. Both versions
     * are unversioned. */
    pOut->dwCaps = pIn->dwCaps;
    pOut->dwCaps2 = 0;
    pOut->dwCaps3 = 0;
    pOut->dwCaps4 = 0;
}

void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2* pIn, DDDEVICEIDENTIFIER* pOut)
{
    /* 2 adds a dwWHQLLevel field to the end. Both structures are
     * unversioned. */
    memcpy(pOut, pIn, sizeof(*pOut));
}

void DDRAW_dump_cooperativelevel(DWORD cooplevel)
{
    static const struct flag_info flags[] =
    {
        FE(DDSCL_FULLSCREEN),
        FE(DDSCL_ALLOWREBOOT),
        FE(DDSCL_NOWINDOWCHANGES),
        FE(DDSCL_NORMAL),
        FE(DDSCL_ALLOWMODEX),
        FE(DDSCL_EXCLUSIVE),
        FE(DDSCL_SETFOCUSWINDOW),
        FE(DDSCL_SETDEVICEWINDOW),
        FE(DDSCL_CREATEDEVICEWINDOW)
    };

    if (TRACE_ON(ddraw))
    {
        TRACE(" - ");
        DDRAW_dump_flags(cooplevel, flags, ARRAY_SIZE(flags));
    }
}

void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps)
{
    static const struct flag_info flags1[] =
    {
        FE(DDCAPS_3D),
        FE(DDCAPS_ALIGNBOUNDARYDEST),
        FE(DDCAPS_ALIGNSIZEDEST),
        FE(DDCAPS_ALIGNBOUNDARYSRC),
        FE(DDCAPS_ALIGNSIZESRC),
        FE(DDCAPS_ALIGNSTRIDE),
        FE(DDCAPS_BLT),
        FE(DDCAPS_BLTQUEUE),
        FE(DDCAPS_BLTFOURCC),
        FE(DDCAPS_BLTSTRETCH),
        FE(DDCAPS_GDI),
        FE(DDCAPS_OVERLAY),
        FE(DDCAPS_OVERLAYCANTCLIP),
        FE(DDCAPS_OVERLAYFOURCC),
        FE(DDCAPS_OVERLAYSTRETCH),
        FE(DDCAPS_PALETTE),
        FE(DDCAPS_PALETTEVSYNC),
        FE(DDCAPS_READSCANLINE),
        FE(DDCAPS_STEREOVIEW),
        FE(DDCAPS_VBI),
        FE(DDCAPS_ZBLTS),
        FE(DDCAPS_ZOVERLAYS),
        FE(DDCAPS_COLORKEY),
        FE(DDCAPS_ALPHA),
        FE(DDCAPS_COLORKEYHWASSIST),
        FE(DDCAPS_NOHARDWARE),
        FE(DDCAPS_BLTCOLORFILL),
        FE(DDCAPS_BANKSWITCHED),
        FE(DDCAPS_BLTDEPTHFILL),
        FE(DDCAPS_CANCLIP),
        FE(DDCAPS_CANCLIPSTRETCHED),
        FE(DDCAPS_CANBLTSYSMEM)
    };
    static const struct flag_info flags2[] =
    {
        FE(DDCAPS2_CERTIFIED),
        FE(DDCAPS2_NO2DDURING3DSCENE),
        FE(DDCAPS2_VIDEOPORT),
        FE(DDCAPS2_AUTOFLIPOVERLAY),
        FE(DDCAPS2_CANBOBINTERLEAVED),
        FE(DDCAPS2_CANBOBNONINTERLEAVED),
        FE(DDCAPS2_COLORCONTROLOVERLAY),
        FE(DDCAPS2_COLORCONTROLPRIMARY),
        FE(DDCAPS2_CANDROPZ16BIT),
        FE(DDCAPS2_NONLOCALVIDMEM),
        FE(DDCAPS2_NONLOCALVIDMEMCAPS),
        FE(DDCAPS2_NOPAGELOCKREQUIRED),
        FE(DDCAPS2_WIDESURFACES),
        FE(DDCAPS2_CANFLIPODDEVEN),
        FE(DDCAPS2_CANBOBHARDWARE),
        FE(DDCAPS2_COPYFOURCC),
        FE(DDCAPS2_PRIMARYGAMMA),
        FE(DDCAPS2_CANRENDERWINDOWED),
        FE(DDCAPS2_CANCALIBRATEGAMMA),
        FE(DDCAPS2_FLIPINTERVAL),
        FE(DDCAPS2_FLIPNOVSYNC),
        FE(DDCAPS2_CANMANAGETEXTURE),
        FE(DDCAPS2_TEXMANINNONLOCALVIDMEM),
        FE(DDCAPS2_STEREO),
        FE(DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL)
    };
    static const struct flag_info flags3[] =
    {
        FE(DDCKEYCAPS_DESTBLT),
        FE(DDCKEYCAPS_DESTBLTCLRSPACE),
        FE(DDCKEYCAPS_DESTBLTCLRSPACEYUV),
        FE(DDCKEYCAPS_DESTBLTYUV),
        FE(DDCKEYCAPS_DESTOVERLAY),
        FE(DDCKEYCAPS_DESTOVERLAYCLRSPACE),
        FE(DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV),
        FE(DDCKEYCAPS_DESTOVERLAYONEACTIVE),
        FE(DDCKEYCAPS_DESTOVERLAYYUV),
        FE(DDCKEYCAPS_SRCBLT),
        FE(DDCKEYCAPS_SRCBLTCLRSPACE),
        FE(DDCKEYCAPS_SRCBLTCLRSPACEYUV),
        FE(DDCKEYCAPS_SRCBLTYUV),
        FE(DDCKEYCAPS_SRCOVERLAY),
        FE(DDCKEYCAPS_SRCOVERLAYCLRSPACE),
        FE(DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV),
        FE(DDCKEYCAPS_SRCOVERLAYONEACTIVE),
        FE(DDCKEYCAPS_SRCOVERLAYYUV),
        FE(DDCKEYCAPS_NOCOSTOVERLAY)
    };
    static const struct flag_info flags4[] =
    {
        FE(DDFXCAPS_BLTALPHA),
        FE(DDFXCAPS_OVERLAYALPHA),
        FE(DDFXCAPS_BLTARITHSTRETCHYN),
        FE(DDFXCAPS_BLTARITHSTRETCHY),
        FE(DDFXCAPS_BLTMIRRORLEFTRIGHT),
        FE(DDFXCAPS_BLTMIRRORUPDOWN),
        FE(DDFXCAPS_BLTROTATION),
        FE(DDFXCAPS_BLTROTATION90),
        FE(DDFXCAPS_BLTSHRINKX),
        FE(DDFXCAPS_BLTSHRINKXN),
        FE(DDFXCAPS_BLTSHRINKY),
        FE(DDFXCAPS_BLTSHRINKYN),
        FE(DDFXCAPS_BLTSTRETCHX),
        FE(DDFXCAPS_BLTSTRETCHXN),
        FE(DDFXCAPS_BLTSTRETCHY),
        FE(DDFXCAPS_BLTSTRETCHYN),
        FE(DDFXCAPS_OVERLAYARITHSTRETCHY),
        FE(DDFXCAPS_OVERLAYARITHSTRETCHYN),
        FE(DDFXCAPS_OVERLAYSHRINKX),
        FE(DDFXCAPS_OVERLAYSHRINKXN),
        FE(DDFXCAPS_OVERLAYSHRINKY),
        FE(DDFXCAPS_OVERLAYSHRINKYN),
        FE(DDFXCAPS_OVERLAYSTRETCHX),
        FE(DDFXCAPS_OVERLAYSTRETCHXN),
        FE(DDFXCAPS_OVERLAYSTRETCHY),
        FE(DDFXCAPS_OVERLAYSTRETCHYN),
        FE(DDFXCAPS_OVERLAYMIRRORLEFTRIGHT),
        FE(DDFXCAPS_OVERLAYMIRRORUPDOWN)
    };
    static const struct flag_info flags5[] =
    {
        FE(DDFXALPHACAPS_BLTALPHAEDGEBLEND),
        FE(DDFXALPHACAPS_BLTALPHAPIXELS),
        FE(DDFXALPHACAPS_BLTALPHAPIXELSNEG),
        FE(DDFXALPHACAPS_BLTALPHASURFACES),
        FE(DDFXALPHACAPS_BLTALPHASURFACESNEG),
        FE(DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND),
        FE(DDFXALPHACAPS_OVERLAYALPHAPIXELS),
        FE(DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG),
        FE(DDFXALPHACAPS_OVERLAYALPHASURFACES),
        FE(DDFXALPHACAPS_OVERLAYALPHASURFACESNEG)
    };
    static const struct flag_info flags6[] =
    {
        FE(DDPCAPS_4BIT),
        FE(DDPCAPS_8BITENTRIES),
        FE(DDPCAPS_8BIT),
        FE(DDPCAPS_INITIALIZE),
        FE(DDPCAPS_PRIMARYSURFACE),
        FE(DDPCAPS_PRIMARYSURFACELEFT),
        FE(DDPCAPS_ALLOW256),
        FE(DDPCAPS_VSYNC),
        FE(DDPCAPS_1BIT),
        FE(DDPCAPS_2BIT),
        FE(DDPCAPS_ALPHA),
    };
    static const struct flag_info flags7[] =
    {
        FE(DDSVCAPS_RESERVED1),
        FE(DDSVCAPS_RESERVED2),
        FE(DDSVCAPS_RESERVED3),
        FE(DDSVCAPS_RESERVED4),
        FE(DDSVCAPS_STEREOSEQUENTIAL),
    };

    TRACE(" - dwSize : %lu\n", lpcaps->dwSize);
    TRACE(" - dwCaps : "); DDRAW_dump_flags(lpcaps->dwCaps, flags1, ARRAY_SIZE(flags1));
    TRACE(" - dwCaps2 : "); DDRAW_dump_flags(lpcaps->dwCaps2, flags2, ARRAY_SIZE(flags2));
    TRACE(" - dwCKeyCaps : "); DDRAW_dump_flags(lpcaps->dwCKeyCaps, flags3, ARRAY_SIZE(flags3));
    TRACE(" - dwFXCaps : "); DDRAW_dump_flags(lpcaps->dwFXCaps, flags4, ARRAY_SIZE(flags4));
    TRACE(" - dwFXAlphaCaps : "); DDRAW_dump_flags(lpcaps->dwFXAlphaCaps, flags5, ARRAY_SIZE(flags5));
    TRACE(" - dwPalCaps : "); DDRAW_dump_flags(lpcaps->dwPalCaps, flags6, ARRAY_SIZE(flags6));
    TRACE(" - dwSVCaps : "); DDRAW_dump_flags(lpcaps->dwSVCaps, flags7, ARRAY_SIZE(flags7));
    TRACE("...\n");
    TRACE(" - dwNumFourCCCodes : %lu\n", lpcaps->dwNumFourCCCodes);
    TRACE(" - dwCurrVisibleOverlays : %lu\n", lpcaps->dwCurrVisibleOverlays);
    TRACE(" - dwMinOverlayStretch : %lu\n", lpcaps->dwMinOverlayStretch);
    TRACE(" - dwMaxOverlayStretch : %lu\n", lpcaps->dwMaxOverlayStretch);
    TRACE("...\n");
    TRACE(" - ddsCaps : "); DDRAW_dump_DDSCAPS2(&lpcaps->ddsCaps);
}

void multiply_matrix(struct wined3d_matrix *dst, const struct wined3d_matrix *src1, const struct wined3d_matrix *src2)
{
    struct wined3d_matrix temp;

    /* Now do the multiplication 'by hand'.
       I know that all this could be optimised, but this will be done later :-) */
    temp._11 = (src1->_11 * src2->_11) + (src1->_21 * src2->_12) + (src1->_31 * src2->_13) + (src1->_41 * src2->_14);
    temp._21 = (src1->_11 * src2->_21) + (src1->_21 * src2->_22) + (src1->_31 * src2->_23) + (src1->_41 * src2->_24);
    temp._31 = (src1->_11 * src2->_31) + (src1->_21 * src2->_32) + (src1->_31 * src2->_33) + (src1->_41 * src2->_34);
    temp._41 = (src1->_11 * src2->_41) + (src1->_21 * src2->_42) + (src1->_31 * src2->_43) + (src1->_41 * src2->_44);

    temp._12 = (src1->_12 * src2->_11) + (src1->_22 * src2->_12) + (src1->_32 * src2->_13) + (src1->_42 * src2->_14);
    temp._22 = (src1->_12 * src2->_21) + (src1->_22 * src2->_22) + (src1->_32 * src2->_23) + (src1->_42 * src2->_24);
    temp._32 = (src1->_12 * src2->_31) + (src1->_22 * src2->_32) + (src1->_32 * src2->_33) + (src1->_42 * src2->_34);
    temp._42 = (src1->_12 * src2->_41) + (src1->_22 * src2->_42) + (src1->_32 * src2->_43) + (src1->_42 * src2->_44);

    temp._13 = (src1->_13 * src2->_11) + (src1->_23 * src2->_12) + (src1->_33 * src2->_13) + (src1->_43 * src2->_14);
    temp._23 = (src1->_13 * src2->_21) + (src1->_23 * src2->_22) + (src1->_33 * src2->_23) + (src1->_43 * src2->_24);
    temp._33 = (src1->_13 * src2->_31) + (src1->_23 * src2->_32) + (src1->_33 * src2->_33) + (src1->_43 * src2->_34);
    temp._43 = (src1->_13 * src2->_41) + (src1->_23 * src2->_42) + (src1->_33 * src2->_43) + (src1->_43 * src2->_44);

    temp._14 = (src1->_14 * src2->_11) + (src1->_24 * src2->_12) + (src1->_34 * src2->_13) + (src1->_44 * src2->_14);
    temp._24 = (src1->_14 * src2->_21) + (src1->_24 * src2->_22) + (src1->_34 * src2->_23) + (src1->_44 * src2->_24);
    temp._34 = (src1->_14 * src2->_31) + (src1->_24 * src2->_32) + (src1->_34 * src2->_33) + (src1->_44 * src2->_34);
    temp._44 = (src1->_14 * src2->_41) + (src1->_24 * src2->_42) + (src1->_34 * src2->_43) + (src1->_44 * src2->_44);

    *dst = temp;
}

HRESULT
hr_ddraw_from_wined3d(HRESULT hr)
{
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        case WINED3DERR_NOTAVAILABLE:       return DDERR_UNSUPPORTED;
        case WINEDDERR_NOTAOVERLAYSURFACE:  return DDERR_NOTAOVERLAYSURFACE;
        case WINEDDERR_OVERLAYNOTVISIBLE:   return DDERR_OVERLAYNOTVISIBLE;
        default:                            return hr;
    }
}

/* Note that this function writes the full sizeof(DDSURFACEDESC2) size, don't use it
 * for writing into application-provided DDSURFACEDESC2 structures if the size may
 * be different */
void DDSD_to_DDSD2(const DDSURFACEDESC *in, DDSURFACEDESC2 *out)
{
    /* The output of this function is never passed to the application directly, so
     * the memset is not strictly needed. CreateSurface still has problems with this
     * though. Don't forget to set ddsCaps.dwCaps2/3/4 to 0 when removing this */
    memset(out, 0x00, sizeof(*out));
    out->dwSize = sizeof(*out);
    out->dwFlags = in->dwFlags & ~DDSD_ZBUFFERBITDEPTH;
    if (in->dwFlags & DDSD_WIDTH) out->dwWidth = in->dwWidth;
    if (in->dwFlags & DDSD_HEIGHT) out->dwHeight = in->dwHeight;
    if (in->dwFlags & DDSD_PIXELFORMAT) out->ddpfPixelFormat = in->ddpfPixelFormat;
    else if(in->dwFlags & DDSD_ZBUFFERBITDEPTH)
    {
        out->dwFlags |= DDSD_PIXELFORMAT;
        memset(&out->ddpfPixelFormat, 0, sizeof(out->ddpfPixelFormat));
        out->ddpfPixelFormat.dwSize = sizeof(out->ddpfPixelFormat);
        out->ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
        out->ddpfPixelFormat.dwZBufferBitDepth = in->dwZBufferBitDepth;
        /* 0 is not a valid DDSURFACEDESC / DDPIXELFORMAT on either side of the
         * conversion */
        out->ddpfPixelFormat.dwZBitMask = ~0U >> (32 - in->dwZBufferBitDepth);
    }
    /* ddsCaps is read even without DDSD_CAPS set. See dsurface:no_ddsd_caps_test */
    out->ddsCaps.dwCaps = in->ddsCaps.dwCaps;
    if (in->dwFlags & DDSD_PITCH) out->lPitch = in->lPitch;
    if (in->dwFlags & DDSD_BACKBUFFERCOUNT) out->dwBackBufferCount = in->dwBackBufferCount;
    if (in->dwFlags & DDSD_ALPHABITDEPTH) out->dwAlphaBitDepth = in->dwAlphaBitDepth;
    /* DDraw(native, and wine) does not set the DDSD_LPSURFACE, so always copy */
    out->lpSurface = in->lpSurface;
    if (in->dwFlags & DDSD_CKDESTOVERLAY) out->ddckCKDestOverlay = in->ddckCKDestOverlay;
    if (in->dwFlags & DDSD_CKDESTBLT) out->ddckCKDestBlt = in->ddckCKDestBlt;
    if (in->dwFlags & DDSD_CKSRCOVERLAY) out->ddckCKSrcOverlay = in->ddckCKSrcOverlay;
    if (in->dwFlags & DDSD_CKSRCBLT) out->ddckCKSrcBlt = in->ddckCKSrcBlt;
    if (in->dwFlags & DDSD_MIPMAPCOUNT) out->dwMipMapCount = in->dwMipMapCount;
    if (in->dwFlags & DDSD_REFRESHRATE) out->dwRefreshRate = in->dwRefreshRate;
    if (in->dwFlags & DDSD_LINEARSIZE) out->dwLinearSize = in->dwLinearSize;
    /* Does not exist in DDSURFACEDESC:
     * DDSD_TEXTURESTAGE, DDSD_FVF, DDSD_SRCVBHANDLE,
     */
}

/* Note that this function writes the full sizeof(DDSURFACEDESC) size, don't use it
 * for writing into application-provided DDSURFACEDESC structures if the size may
 * be different */
void DDSD2_to_DDSD(const DDSURFACEDESC2 *in, DDSURFACEDESC *out)
{
    memset(out, 0, sizeof(*out));
    out->dwSize = sizeof(*out);
    out->dwFlags = in->dwFlags;
    if (in->dwFlags & DDSD_WIDTH) out->dwWidth = in->dwWidth;
    if (in->dwFlags & DDSD_HEIGHT) out->dwHeight = in->dwHeight;
    if (in->dwFlags & DDSD_PIXELFORMAT)
    {
        out->ddpfPixelFormat = in->ddpfPixelFormat;
        if ((in->dwFlags & DDSD_CAPS) && (in->ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
        {
            /* Z buffers have DDSD_ZBUFFERBITDEPTH set, but not DDSD_PIXELFORMAT. They do
             * have valid data in ddpfPixelFormat though */
            out->dwFlags &= ~DDSD_PIXELFORMAT;
            out->dwFlags |= DDSD_ZBUFFERBITDEPTH;
            out->dwZBufferBitDepth = in->ddpfPixelFormat.dwZBufferBitDepth;
        }
    }
    /* ddsCaps is read even without DDSD_CAPS set. See dsurface:no_ddsd_caps_test */
    out->ddsCaps.dwCaps = in->ddsCaps.dwCaps;
    if (in->dwFlags & DDSD_PITCH) out->lPitch = in->lPitch;
    if (in->dwFlags & DDSD_BACKBUFFERCOUNT) out->dwBackBufferCount = in->dwBackBufferCount;
    if (in->dwFlags & DDSD_ZBUFFERBITDEPTH) out->dwZBufferBitDepth = in->dwMipMapCount; /* same union */
    if (in->dwFlags & DDSD_ALPHABITDEPTH) out->dwAlphaBitDepth = in->dwAlphaBitDepth;
    /* DDraw(native, and wine) does not set the DDSD_LPSURFACE, so always copy */
    out->lpSurface = in->lpSurface;
    if (in->dwFlags & DDSD_CKDESTOVERLAY) out->ddckCKDestOverlay = in->ddckCKDestOverlay;
    if (in->dwFlags & DDSD_CKDESTBLT) out->ddckCKDestBlt = in->ddckCKDestBlt;
    if (in->dwFlags & DDSD_CKSRCOVERLAY) out->ddckCKSrcOverlay = in->ddckCKSrcOverlay;
    if (in->dwFlags & DDSD_CKSRCBLT) out->ddckCKSrcBlt = in->ddckCKSrcBlt;
    if (in->dwFlags & DDSD_MIPMAPCOUNT) out->dwMipMapCount = in->dwMipMapCount;
    if (in->dwFlags & DDSD_REFRESHRATE) out->dwRefreshRate = in->dwRefreshRate;
    if (in->dwFlags & DDSD_LINEARSIZE) out->dwLinearSize = in->dwLinearSize;
    /* Does not exist in DDSURFACEDESC:
     * DDSD_TEXTURESTAGE, DDSD_FVF, DDSD_SRCVBHANDLE,
     */
    if (in->dwFlags & DDSD_TEXTURESTAGE) WARN("Does not exist in DDSURFACEDESC: DDSD_TEXTURESTAGE\n");
    if (in->dwFlags & DDSD_FVF) WARN("Does not exist in DDSURFACEDESC: DDSD_FVF\n");
    if (in->dwFlags & DDSD_SRCVBHANDLE) WARN("Does not exist in DDSURFACEDESC: DDSD_SRCVBHANDLE\n");
    out->dwFlags &= ~(DDSD_TEXTURESTAGE | DDSD_FVF | DDSD_SRCVBHANDLE);
}
