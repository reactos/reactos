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

#include "config.h"

#define NONAMELESSUNION

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static void DDRAW_dump_pixelformat(const DDPIXELFORMAT *pf);

/*****************************************************************************
 * PixelFormat_WineD3DtoDD
 *
 * Converts an WINED3DFORMAT value into a DDPIXELFORMAT structure
 *
 * Params:
 *  DDPixelFormat: Address of the structure to write the pixel format to
 *  WineD3DFormat: Source format
 *
 *****************************************************************************/
void
PixelFormat_WineD3DtoDD(DDPIXELFORMAT *DDPixelFormat,
                        WINED3DFORMAT WineD3DFormat)
{
    DWORD Size = DDPixelFormat->dwSize;
    TRACE("Converting WINED3DFORMAT %d to DDRAW\n", WineD3DFormat);

    if(Size==0) return;

    memset(DDPixelFormat, 0x00, Size);
    DDPixelFormat->dwSize = Size;
    switch(WineD3DFormat)
    {
        case WINED3DFMT_R8G8B8:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 24;
            DDPixelFormat->u2.dwRBitMask = 0x00ff0000;
            DDPixelFormat->u3.dwGBitMask = 0x0000ff00;
            DDPixelFormat->u4.dwBBitMask = 0x000000ff;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_A8R8G8B8:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 32;
            DDPixelFormat->u2.dwRBitMask = 0x00ff0000;
            DDPixelFormat->u3.dwGBitMask = 0x0000ff00;
            DDPixelFormat->u4.dwBBitMask = 0x000000ff;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0xff000000;
            break;

        case WINED3DFMT_X8R8G8B8:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 32;
            DDPixelFormat->u2.dwRBitMask = 0x00ff0000;
            DDPixelFormat->u3.dwGBitMask = 0x0000ff00;
            DDPixelFormat->u4.dwBBitMask = 0x000000ff;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_X8B8G8R8:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 32;
            DDPixelFormat->u2.dwRBitMask = 0x000000ff;
            DDPixelFormat->u3.dwGBitMask = 0x0000ff00;
            DDPixelFormat->u4.dwBBitMask = 0x00ff0000;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_R5G6B5:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 16;
            DDPixelFormat->u2.dwRBitMask = 0xF800;
            DDPixelFormat->u3.dwGBitMask = 0x07E0;
            DDPixelFormat->u4.dwBBitMask = 0x001F;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_X1R5G5B5:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 16;
            DDPixelFormat->u2.dwRBitMask = 0x7C00;
            DDPixelFormat->u3.dwGBitMask = 0x03E0;
            DDPixelFormat->u4.dwBBitMask = 0x001F;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_A1R5G5B5:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 16;
            DDPixelFormat->u2.dwRBitMask = 0x7C00;
            DDPixelFormat->u3.dwGBitMask = 0x03E0;
            DDPixelFormat->u4.dwBBitMask = 0x001F;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x8000;
            break;

        case WINED3DFMT_A4R4G4B4:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 16;
            DDPixelFormat->u2.dwRBitMask = 0x0F00;
            DDPixelFormat->u3.dwGBitMask = 0x00F0;
            DDPixelFormat->u4.dwBBitMask = 0x000F;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0xF000;
            break;

        case WINED3DFMT_R3G3B2:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 8;
            DDPixelFormat->u2.dwRBitMask = 0xE0;
            DDPixelFormat->u3.dwGBitMask = 0x1C;
            DDPixelFormat->u4.dwBBitMask = 0x03;
            DDPixelFormat->u5.dwLuminanceAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_P8:
            DDPixelFormat->dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 8;
            DDPixelFormat->u2.dwRBitMask = 0x00;
            DDPixelFormat->u3.dwGBitMask = 0x00;
            DDPixelFormat->u4.dwBBitMask = 0x00;
            break;

        case WINED3DFMT_A8_UNORM:
            DDPixelFormat->dwFlags = DDPF_ALPHA;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwAlphaBitDepth = 8;
            DDPixelFormat->u2.dwRBitMask = 0x0;
            DDPixelFormat->u3.dwZBitMask = 0x0;
            DDPixelFormat->u4.dwStencilBitMask = 0x0;
            DDPixelFormat->u5.dwLuminanceAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_A8R3G3B2:
            DDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 16;
            DDPixelFormat->u2.dwRBitMask = 0x00E0;
            DDPixelFormat->u3.dwGBitMask = 0x001C;
            DDPixelFormat->u4.dwBBitMask = 0x0003;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0xF000;
            break;

        case WINED3DFMT_X4R4G4B4:
            DDPixelFormat->dwFlags = DDPF_RGB;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwRGBBitCount = 16;
            DDPixelFormat->u2.dwRBitMask = 0x0F00;
            DDPixelFormat->u3.dwGBitMask = 0x00F0;
            DDPixelFormat->u4.dwBBitMask = 0x000F;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            return;

        /* How are Z buffer bit depth and Stencil buffer bit depth related?
         */
        case WINED3DFMT_D16_UNORM:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwZBufferBitDepth = 16;
            DDPixelFormat->u2.dwStencilBitDepth = 0;
            DDPixelFormat->u3.dwZBitMask = 0x0000FFFF;
            DDPixelFormat->u4.dwStencilBitMask = 0x0;
            DDPixelFormat->u5.dwRGBZBitMask = 0x00000000;
            break;

        case WINED3DFMT_D32:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwZBufferBitDepth = 32;
            DDPixelFormat->u2.dwStencilBitDepth = 0;
            DDPixelFormat->u3.dwZBitMask = 0xFFFFFFFF;
            DDPixelFormat->u4.dwStencilBitMask = 0x0;
            DDPixelFormat->u5.dwRGBZBitMask = 0x00000000;
            break;

        case WINED3DFMT_D24X4S4:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
            DDPixelFormat->dwFourCC = 0;
            /* Should I set dwZBufferBitDepth to 32 here? */
            DDPixelFormat->u1.dwZBufferBitDepth = 32;
            DDPixelFormat->u2.dwStencilBitDepth = 4;
            DDPixelFormat->u3.dwZBitMask = 0x00FFFFFF;
            DDPixelFormat->u4.dwStencilBitMask = 0x0F000000;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_D24S8:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
            DDPixelFormat->dwFourCC = 0;
            /* Should I set dwZBufferBitDepth to 32 here? */
            DDPixelFormat->u1.dwZBufferBitDepth = 32;
            DDPixelFormat->u2.dwStencilBitDepth = 8;
            DDPixelFormat->u3.dwZBitMask = 0x00FFFFFFFF;
            DDPixelFormat->u4.dwStencilBitMask = 0xFF000000;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_D24X8:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwZBufferBitDepth = 32;
            DDPixelFormat->u2.dwStencilBitDepth = 0;
            DDPixelFormat->u3.dwZBitMask = 0x00FFFFFFFF;
            DDPixelFormat->u4.dwStencilBitMask = 0x00000000;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;

            break;
        case WINED3DFMT_D15S1:
            DDPixelFormat->dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwZBufferBitDepth = 16;
            DDPixelFormat->u2.dwStencilBitDepth = 1;
            DDPixelFormat->u3.dwZBitMask = 0x7fff;
            DDPixelFormat->u4.dwStencilBitMask = 0x8000;
            DDPixelFormat->u5.dwRGBAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_UYVY:
        case WINED3DFMT_YUY2:
            DDPixelFormat->u1.dwYUVBitCount = 16;
            DDPixelFormat->dwFlags = DDPF_FOURCC;
            DDPixelFormat->dwFourCC = WineD3DFormat;
            break;

        case WINED3DFMT_YV12:
            DDPixelFormat->u1.dwYUVBitCount = 12;
            DDPixelFormat->dwFlags = DDPF_FOURCC;
            DDPixelFormat->dwFourCC = WineD3DFormat;
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
            DDPixelFormat->dwFourCC = WineD3DFormat;
            break;

        /* Luminance */
        case WINED3DFMT_L8:
            DDPixelFormat->dwFlags = DDPF_LUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwLuminanceBitCount = 8;
            DDPixelFormat->u2.dwLuminanceBitMask = 0xff;
            DDPixelFormat->u3.dwBumpDvBitMask = 0x0;
            DDPixelFormat->u4.dwBumpLuminanceBitMask = 0x0;
            DDPixelFormat->u5.dwLuminanceAlphaBitMask = 0x0;
            break;

        case WINED3DFMT_A4L4:
            DDPixelFormat->dwFlags = DDPF_ALPHAPIXELS | DDPF_LUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwLuminanceBitCount = 4;
            DDPixelFormat->u2.dwLuminanceBitMask = 0x0f;
            DDPixelFormat->u3.dwBumpDvBitMask = 0x0;
            DDPixelFormat->u4.dwBumpLuminanceBitMask = 0x0;
            DDPixelFormat->u5.dwLuminanceAlphaBitMask = 0xf0;
            break;

        case WINED3DFMT_A8L8:
            DDPixelFormat->dwFlags = DDPF_ALPHAPIXELS | DDPF_LUMINANCE;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwLuminanceBitCount = 16;
            DDPixelFormat->u2.dwLuminanceBitMask = 0x00ff;
            DDPixelFormat->u3.dwBumpDvBitMask = 0x0;
            DDPixelFormat->u4.dwBumpLuminanceBitMask = 0x0;
            DDPixelFormat->u5.dwLuminanceAlphaBitMask = 0xff00;
            break;

        /* Bump mapping */
        case WINED3DFMT_R8G8_SNORM:
            DDPixelFormat->dwFlags = DDPF_BUMPDUDV;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwBumpBitCount = 16;
            DDPixelFormat->u2.dwBumpDuBitMask =         0x000000ff;
            DDPixelFormat->u3.dwBumpDvBitMask =         0x0000ff00;
            DDPixelFormat->u4.dwBumpLuminanceBitMask =  0x00000000;
            DDPixelFormat->u5.dwLuminanceAlphaBitMask = 0x00000000;
            break;

        case WINED3DFMT_L6V5U5:
            DDPixelFormat->dwFlags = DDPF_BUMPDUDV;
            DDPixelFormat->dwFourCC = 0;
            DDPixelFormat->u1.dwBumpBitCount = 16;
            DDPixelFormat->u2.dwBumpDuBitMask =         0x0000001f;
            DDPixelFormat->u3.dwBumpDvBitMask =         0x000003e0;
            DDPixelFormat->u4.dwBumpLuminanceBitMask =  0x0000fc00;
            DDPixelFormat->u5.dwLuminanceAlphaBitMask = 0x00000000;
            break;

        default:
            ERR("Can't translate this Pixelformat %d\n", WineD3DFormat);
    }

    if(TRACE_ON(ddraw)) {
        TRACE("Returning: ");
        DDRAW_dump_pixelformat(DDPixelFormat);
    }
}
/*****************************************************************************
 * PixelFormat_DD2WineD3D
 *
 * Reads a DDPIXELFORMAT structure and returns the equal WINED3DFORMAT
 *
 * Params:
 *  DDPixelFormat: The source format
 *
 * Returns:
 *  The WINED3DFORMAT equal to the DDraw format
 *  WINED3DFMT_UNKNOWN if a matching format wasn't found
 *****************************************************************************/
WINED3DFORMAT
PixelFormat_DD2WineD3D(const DDPIXELFORMAT *DDPixelFormat)
{
    TRACE("Convert a DirectDraw Pixelformat to a WineD3D Pixelformat\n");    
    if(TRACE_ON(ddraw))
    {
        DDRAW_dump_pixelformat(DDPixelFormat);
    }

    if(DDPixelFormat->dwFlags & DDPF_PALETTEINDEXED8)
    {
        return WINED3DFMT_P8;
    }
    else if(DDPixelFormat->dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4) )
    {
        FIXME("DDPF_PALETTEINDEXED1 to DDPF_PALETTEINDEXED4 are not supported by WineD3D (yet). Returning WINED3DFMT_P8\n");
        return WINED3DFMT_P8;
    }
    else if(DDPixelFormat->dwFlags & DDPF_RGB)
    {
        switch(DDPixelFormat->u1.dwRGBBitCount)
        {
            case 8:
                /* This is the only format that can match here */
                return WINED3DFMT_R3G3B2;

            case 16:
                /* Read the Color masks */
                if( (DDPixelFormat->u2.dwRBitMask == 0xF800) &&
                    (DDPixelFormat->u3.dwGBitMask == 0x07E0) &&
                    (DDPixelFormat->u4.dwBBitMask == 0x001F) )
                {
                    return WINED3DFMT_R5G6B5;
                }

                if( (DDPixelFormat->u2.dwRBitMask == 0x7C00) &&
                    (DDPixelFormat->u3.dwGBitMask == 0x03E0) &&
                    (DDPixelFormat->u4.dwBBitMask == 0x001F) )
                {
                    if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                        (DDPixelFormat->u5.dwRGBAlphaBitMask == 0x8000))
                        return WINED3DFMT_A1R5G5B5;
                    else
                        return WINED3DFMT_X1R5G5B5;
                }

                if( (DDPixelFormat->u2.dwRBitMask == 0x0F00) &&
                    (DDPixelFormat->u3.dwGBitMask == 0x00F0) &&
                    (DDPixelFormat->u4.dwBBitMask == 0x000F) )
                {
                    if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                        (DDPixelFormat->u5.dwRGBAlphaBitMask == 0xF000))
                        return WINED3DFMT_A4R4G4B4;
                    else
                        return WINED3DFMT_X4R4G4B4;
                }

                if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                    (DDPixelFormat->u5.dwRGBAlphaBitMask == 0xFF00) &&
                    (DDPixelFormat->u2.dwRBitMask == 0x00E0) &&
                    (DDPixelFormat->u3.dwGBitMask == 0x001C) &&
                    (DDPixelFormat->u4.dwBBitMask == 0x0003) )
                {
                    return WINED3DFMT_A8R3G3B2;
                }
                ERR("16 bit RGB Pixel format does not match\n");
                return WINED3DFMT_UNKNOWN;

            case 24:
                return WINED3DFMT_R8G8B8;

            case 32:
                /* Read the Color masks */
                if( (DDPixelFormat->u2.dwRBitMask == 0x00FF0000) &&
                    (DDPixelFormat->u3.dwGBitMask == 0x0000FF00) &&
                    (DDPixelFormat->u4.dwBBitMask == 0x000000FF) )
                {
                    if( (DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS) &&
                        (DDPixelFormat->u5.dwRGBAlphaBitMask == 0xFF000000))
                        return WINED3DFMT_A8R8G8B8;
                    else
                        return WINED3DFMT_X8R8G8B8;

                }
                ERR("32 bit RGB pixel format does not match\n");

            default:
                ERR("Invalid dwRGBBitCount in Pixelformat structure\n");
                return WINED3DFMT_UNKNOWN;
        }
    }
    else if( (DDPixelFormat->dwFlags & DDPF_ALPHA) )
    {
        /* Alpha only Pixelformat */
        switch(DDPixelFormat->u1.dwAlphaBitDepth)
        {
            case 1:
            case 2:
            case 4:
                ERR("Unsupported Alpha-Only bit depth 0x%x\n", DDPixelFormat->u1.dwAlphaBitDepth);
            case 8:
                return WINED3DFMT_A8_UNORM;

            default:
                ERR("Invalid AlphaBitDepth in Alpha-Only Pixelformat\n");
                return WINED3DFMT_UNKNOWN;
        }
    }
    else if(DDPixelFormat->dwFlags & DDPF_LUMINANCE)
    {
        /* Luminance-only or luminance-alpha */
        if(DDPixelFormat->dwFlags & DDPF_ALPHAPIXELS)
        {
            /* Luminance with Alpha */
            switch(DDPixelFormat->u1.dwLuminanceBitCount)
            {
                case 4:
                    if(DDPixelFormat->u1.dwAlphaBitDepth == 4)
                        return WINED3DFMT_A4L4;
                    ERR("Unknown Alpha / Luminance bit depth combination\n");
                    return WINED3DFMT_UNKNOWN;

                case 6:
                    ERR("A luminance Pixelformat shouldn't have 6 luminance bits. Returning D3DFMT_L6V5U5 for now!!\n");
                    return WINED3DFMT_L6V5U5;

                case 8:
                    if(DDPixelFormat->u1.dwAlphaBitDepth == 8)
                        return WINED3DFMT_A8L8;
                    ERR("Unknown Alpha / Lumincase bit depth combination\n");
                    return WINED3DFMT_UNKNOWN;
            }
        }
        else
        {
            /* Luminance-only */
            switch(DDPixelFormat->u1.dwLuminanceBitCount)
            {
                case 6:
                    ERR("A luminance Pixelformat shouldn't have 6 luminance bits. Returning D3DFMT_L6V5U5 for now!!\n");
                    return WINED3DFMT_L6V5U5;

                case 8:
                    return WINED3DFMT_L8;

                default:
                    ERR("Unknown luminance-only bit depth 0x%x\n", DDPixelFormat->u1.dwLuminanceBitCount);
                    return WINED3DFMT_UNKNOWN;
             }
        }
    }
    else if(DDPixelFormat->dwFlags & DDPF_ZBUFFER)
    {
        /* Z buffer */
        if(DDPixelFormat->dwFlags & DDPF_STENCILBUFFER)
        {
            switch(DDPixelFormat->u1.dwZBufferBitDepth)
            {
                case 8:
                    FIXME("8 Bits Z+Stencil buffer pixelformat is not supported. Returning WINED3DFMT_UNKNOWN\n");
                    return WINED3DFMT_UNKNOWN;

                case 15:
                    FIXME("15 bit depth buffer not handled yet, assuming 16 bit\n");
                case 16:
                    if(DDPixelFormat->u2.dwStencilBitDepth == 1)
                        return WINED3DFMT_D15S1;

                    FIXME("Don't know how to handle a 16 bit Z buffer with %d bit stencil buffer pixelformat\n", DDPixelFormat->u2.dwStencilBitDepth);
                    return WINED3DFMT_UNKNOWN;

                case 24:
                    FIXME("Don't know how to handle a 24 bit depth buffer with stencil bits\n");
                    return WINED3DFMT_D24S8;

                case 32:
                    if(DDPixelFormat->u2.dwStencilBitDepth == 8)
                        return WINED3DFMT_D24S8;
                    else
                        return WINED3DFMT_D24X4S4;

                default:
                    ERR("Unknown Z buffer depth %d\n", DDPixelFormat->u1.dwZBufferBitDepth);
                    return WINED3DFMT_UNKNOWN;
            }
        }
        else
        {
            switch(DDPixelFormat->u1.dwZBufferBitDepth)
            {
                case 8:
                    ERR("8 Bit Z buffers are not supported. Trying a 16 Bit one\n");
                    return WINED3DFMT_D16_UNORM;

                case 16:
                    return WINED3DFMT_D16_UNORM;

                case 24:
                    FIXME("24 Bit depth buffer, treating like a 32 bit one\n");
                case 32:
                    if(DDPixelFormat->u3.dwZBitMask == 0x00FFFFFF) {
                        return WINED3DFMT_D24X8;
                    } else if(DDPixelFormat->u3.dwZBitMask == 0xFFFFFFFF) {
                        return WINED3DFMT_D32;
                    }
                    FIXME("Unhandled 32 bit depth buffer bitmasks, returning WINED3DFMT_D24X8\n");
                    return WINED3DFMT_D24X8; /* That's most likely to make games happy */

                default:
                    ERR("Unsupported Z buffer depth %d\n", DDPixelFormat->u1.dwZBufferBitDepth);
                    return WINED3DFMT_UNKNOWN;
            }
        }
    }
    else if(DDPixelFormat->dwFlags & DDPF_FOURCC)
    {
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y'))
        {
            return WINED3DFMT_UYVY;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2'))
        {
            return WINED3DFMT_YUY2;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('Y', 'V', '1', '2'))
        {
            return WINED3DFMT_YV12;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
        {
            return WINED3DFMT_DXT1;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('D', 'X', 'T', '2'))
        {
            return WINED3DFMT_DXT2;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('D', 'X', 'T', '3'))
        {
           return WINED3DFMT_DXT3;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('D', 'X', 'T', '4'))
        {
            return WINED3DFMT_DXT4;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
        {
	    return WINED3DFMT_DXT5;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('G', 'R', 'G', 'B'))
        {
            return WINED3DFMT_G8R8_G8B8;
        }
        if(DDPixelFormat->dwFourCC == MAKEFOURCC('R', 'G', 'B', 'G'))
        {
            return WINED3DFMT_R8G8_B8G8;
        }
        return WINED3DFMT_UNKNOWN;  /* Abuse this as an error value */
    }
    else if(DDPixelFormat->dwFlags & DDPF_BUMPDUDV)
    {
        if( (DDPixelFormat->u1.dwBumpBitCount         == 16        ) &&
            (DDPixelFormat->u2.dwBumpDuBitMask        == 0x000000ff) &&
            (DDPixelFormat->u3.dwBumpDvBitMask        == 0x0000ff00) &&
            (DDPixelFormat->u4.dwBumpLuminanceBitMask == 0x00000000) )
        {
            return WINED3DFMT_R8G8_SNORM;
        }
        else if ( (DDPixelFormat->u1.dwBumpBitCount         == 16        ) &&
                  (DDPixelFormat->u2.dwBumpDuBitMask        == 0x0000001f) &&
                  (DDPixelFormat->u3.dwBumpDvBitMask        == 0x000003e0) &&
                  (DDPixelFormat->u4.dwBumpLuminanceBitMask == 0x0000fc00) )
        {
            return WINED3DFMT_L6V5U5;
        }
    }

    ERR("Unknown Pixelformat!\n");
    return WINED3DFMT_UNKNOWN;
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
    TRACE("%d\n", *((const DWORD *) in));
}
static void
DDRAW_dump_PTR(const void *in)
{
    TRACE("%p\n", *((const void * const*) in));
}
static void
DDRAW_dump_DDCOLORKEY(const DDCOLORKEY *ddck)
{
    TRACE("Low : %d  - High : %d\n", ddck->dwColorSpaceLowValue, ddck->dwColorSpaceHighValue);
}

static void DDRAW_dump_flags_nolf(DWORD flags, const flag_info* names,
                                  size_t num_names)
{
    unsigned int	i;

    for (i=0; i < num_names; i++)
        if ((flags & names[i].val) ||      /* standard flag value */
            ((!flags) && (!names[i].val))) /* zero value only */
            TRACE("%s ", names[i].name);
}

static void DDRAW_dump_flags(DWORD flags, const flag_info* names, size_t num_names)
{
    DDRAW_dump_flags_nolf(flags, names, num_names);
    TRACE("\n");
}

void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in)
{
    static const flag_info flags[] = {
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
    static const flag_info flags2[] = {
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

    DDRAW_dump_flags_nolf(in->dwCaps, flags, sizeof(flags)/sizeof(flags[0]));
    DDRAW_dump_flags(in->dwCaps2, flags2, sizeof(flags2)/sizeof(flags2[0]));
}

static void
DDRAW_dump_DDSCAPS(const DDSCAPS *in)
{
    DDSCAPS2 in_bis;

    in_bis.dwCaps = in->dwCaps;
    in_bis.dwCaps2 = 0;
    in_bis.dwCaps3 = 0;
    in_bis.u1.dwCaps4 = 0;

    DDRAW_dump_DDSCAPS2(&in_bis);
}

static void
DDRAW_dump_pixelformat_flag(DWORD flagmask)
{
    static const flag_info flags[] =
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

    DDRAW_dump_flags_nolf(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

static void
DDRAW_dump_members(DWORD flags,
                   const void* data,
                   const member_info* mems,
                   size_t num_mems)
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
    {
        TRACE(", dwFourCC code '%c%c%c%c' (0x%08x) - %d bits per pixel",
                (unsigned char)( pf->dwFourCC     &0xff),
                (unsigned char)((pf->dwFourCC>> 8)&0xff),
                (unsigned char)((pf->dwFourCC>>16)&0xff),
                (unsigned char)((pf->dwFourCC>>24)&0xff),
                pf->dwFourCC,
                pf->u1.dwYUVBitCount
        );
    }
    if (pf->dwFlags & DDPF_RGB)
    {
        const char *cmd;
        TRACE(", RGB bits: %d, ", pf->u1.dwRGBBitCount);
        switch (pf->u1.dwRGBBitCount)
        {
        case 4: cmd = "%1lx"; break;
        case 8: cmd = "%02lx"; break;
        case 16: cmd = "%04lx"; break;
        case 24: cmd = "%06lx"; break;
        case 32: cmd = "%08lx"; break;
        default: ERR("Unexpected bit depth !\n"); cmd = "%d"; break;
        }
        TRACE(" R "); TRACE(cmd, pf->u2.dwRBitMask);
        TRACE(" G "); TRACE(cmd, pf->u3.dwGBitMask);
        TRACE(" B "); TRACE(cmd, pf->u4.dwBBitMask);
        if (pf->dwFlags & DDPF_ALPHAPIXELS)
        {
            TRACE(" A "); TRACE(cmd, pf->u5.dwRGBAlphaBitMask);
        }
        if (pf->dwFlags & DDPF_ZPIXELS)
        {
            TRACE(" Z "); TRACE(cmd, pf->u5.dwRGBZBitMask);
        }
    }
    if (pf->dwFlags & DDPF_ZBUFFER)
    {
        TRACE(", Z bits : %d", pf->u1.dwZBufferBitDepth);
    }
    if (pf->dwFlags & DDPF_ALPHA)
    {
        TRACE(", Alpha bits : %d", pf->u1.dwAlphaBitDepth);
    }
    if (pf->dwFlags & DDPF_BUMPDUDV)
    {
        const char *cmd = "%08lx";
        TRACE(", Bump bits: %d, ", pf->u1.dwBumpBitCount);
        TRACE(" U "); TRACE(cmd, pf->u2.dwBumpDuBitMask);
        TRACE(" V "); TRACE(cmd, pf->u3.dwBumpDvBitMask);
        TRACE(" L "); TRACE(cmd, pf->u4.dwBumpLuminanceBitMask);
    }
    TRACE(")\n");
}

void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd)
{
#define STRUCT DDSURFACEDESC2
    static const member_info members[] =
        {
            ME(DDSD_HEIGHT, DDRAW_dump_DWORD, dwHeight),
            ME(DDSD_WIDTH, DDRAW_dump_DWORD, dwWidth),
            ME(DDSD_PITCH, DDRAW_dump_DWORD, u1 /* lPitch */),
            ME(DDSD_LINEARSIZE, DDRAW_dump_DWORD, u1 /* dwLinearSize */),
            ME(DDSD_BACKBUFFERCOUNT, DDRAW_dump_DWORD, u5.dwBackBufferCount),
            ME(DDSD_MIPMAPCOUNT, DDRAW_dump_DWORD, u2 /* dwMipMapCount */),
            ME(DDSD_ZBUFFERBITDEPTH, DDRAW_dump_DWORD, u2 /* dwZBufferBitDepth */), /* This is for 'old-style' D3D */
            ME(DDSD_REFRESHRATE, DDRAW_dump_DWORD, u2 /* dwRefreshRate */),
            ME(DDSD_ALPHABITDEPTH, DDRAW_dump_DWORD, dwAlphaBitDepth),
            ME(DDSD_LPSURFACE, DDRAW_dump_PTR, lpSurface),
            ME(DDSD_CKDESTOVERLAY, DDRAW_dump_DDCOLORKEY, u3 /* ddckCKDestOverlay */),
            ME(DDSD_CKDESTBLT, DDRAW_dump_DDCOLORKEY, ddckCKDestBlt),
            ME(DDSD_CKSRCOVERLAY, DDRAW_dump_DDCOLORKEY, ddckCKSrcOverlay),
            ME(DDSD_CKSRCBLT, DDRAW_dump_DDCOLORKEY, ddckCKSrcBlt),
            ME(DDSD_PIXELFORMAT, DDRAW_dump_pixelformat, u4 /* ddpfPixelFormat */)
        };
    static const member_info members_caps[] =
        {
            ME(DDSD_CAPS, DDRAW_dump_DDSCAPS, ddsCaps)
        };
    static const member_info members_caps2[] =
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
      DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members,
                          sizeof(members)/sizeof(members[0]));
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
    pOut->u1.dwCaps4 = 0;
}

void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2* pIn, DDDEVICEIDENTIFIER* pOut)
{
    /* 2 adds a dwWHQLLevel field to the end. Both structures are
     * unversioned. */
    memcpy(pOut, pIn, sizeof(*pOut));
}

void DDRAW_dump_cooperativelevel(DWORD cooplevel)
{
    static const flag_info flags[] =
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
        DDRAW_dump_flags(cooplevel, flags, sizeof(flags)/sizeof(flags[0]));
    }
}

void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps)
{
    static const flag_info flags1[] =
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
    static const flag_info flags2[] =
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
    static const flag_info flags3[] =
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
    static const flag_info flags4[] =
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
    static const flag_info flags5[] =
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
    static const flag_info flags6[] =
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
    static const flag_info flags7[] =
    {
      FE(DDSVCAPS_RESERVED1),
      FE(DDSVCAPS_RESERVED2),
      FE(DDSVCAPS_RESERVED3),
      FE(DDSVCAPS_RESERVED4),
      FE(DDSVCAPS_STEREOSEQUENTIAL),
    };

    TRACE(" - dwSize : %d\n", lpcaps->dwSize);
    TRACE(" - dwCaps : "); DDRAW_dump_flags(lpcaps->dwCaps, flags1, sizeof(flags1)/sizeof(flags1[0]));
    TRACE(" - dwCaps2 : "); DDRAW_dump_flags(lpcaps->dwCaps2, flags2, sizeof(flags2)/sizeof(flags2[0]));
    TRACE(" - dwCKeyCaps : "); DDRAW_dump_flags(lpcaps->dwCKeyCaps, flags3, sizeof(flags3)/sizeof(flags3[0]));
    TRACE(" - dwFXCaps : "); DDRAW_dump_flags(lpcaps->dwFXCaps, flags4, sizeof(flags4)/sizeof(flags4[0]));
    TRACE(" - dwFXAlphaCaps : "); DDRAW_dump_flags(lpcaps->dwFXAlphaCaps, flags5, sizeof(flags5)/sizeof(flags5[0]));
    TRACE(" - dwPalCaps : "); DDRAW_dump_flags(lpcaps->dwPalCaps, flags6, sizeof(flags6)/sizeof(flags6[0]));
    TRACE(" - dwSVCaps : "); DDRAW_dump_flags(lpcaps->dwSVCaps, flags7, sizeof(flags7)/sizeof(flags7[0]));
    TRACE("...\n");
    TRACE(" - dwNumFourCCCodes : %d\n", lpcaps->dwNumFourCCCodes);
    TRACE(" - dwCurrVisibleOverlays : %d\n", lpcaps->dwCurrVisibleOverlays);
    TRACE(" - dwMinOverlayStretch : %d\n", lpcaps->dwMinOverlayStretch);
    TRACE(" - dwMaxOverlayStretch : %d\n", lpcaps->dwMaxOverlayStretch);
    TRACE("...\n");
    TRACE(" - ddsCaps : "); DDRAW_dump_DDSCAPS2(&lpcaps->ddsCaps);
}

/*****************************************************************************
 * multiply_matrix
 *
 * Multiplies 2 4x4 matrices src1 and src2, and stores the result in dest.
 *
 * Params:
 *  dest: Pointer to the destination matrix
 *  src1: Pointer to the first source matrix
 *  src2: Pointer to the second source matrix
 *
 *****************************************************************************/
void
multiply_matrix(D3DMATRIX *dest,
                const D3DMATRIX *src1,
                const D3DMATRIX *src2)
{
    D3DMATRIX temp;

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

    /* And copy the new matrix in the good storage.. */
    memcpy(dest, &temp, 16 * sizeof(D3DVALUE));
}

void multiply_matrix_D3D_way(D3DMATRIX* result, const D3DMATRIX *m1, const D3DMATRIX *m2)
{
    D3DMATRIX temp;

    temp._11 = m1->_11 * m2->_11 + m1->_12 * m2->_21 + m1->_13 * m2->_31 + m1->_14 * m2->_41;
    temp._12 = m1->_11 * m2->_12 + m1->_12 * m2->_22 + m1->_13 * m2->_32 + m1->_14 * m2->_42;
    temp._13 = m1->_11 * m2->_13 + m1->_12 * m2->_23 + m1->_13 * m2->_33 + m1->_14 * m2->_43;
    temp._14 = m1->_11 * m2->_14 + m1->_12 * m2->_24 + m1->_13 * m2->_34 + m1->_14 * m2->_44;
    temp._21 = m1->_21 * m2->_11 + m1->_22 * m2->_21 + m1->_23 * m2->_31 + m1->_24 * m2->_41;
    temp._22 = m1->_21 * m2->_12 + m1->_22 * m2->_22 + m1->_23 * m2->_32 + m1->_24 * m2->_42;
    temp._23 = m1->_21 * m2->_13 + m1->_22 * m2->_23 + m1->_23 * m2->_33 + m1->_24 * m2->_43;
    temp._24 = m1->_21 * m2->_14 + m1->_22 * m2->_24 + m1->_23 * m2->_34 + m1->_24 * m2->_44;
    temp._31 = m1->_31 * m2->_11 + m1->_32 * m2->_21 + m1->_33 * m2->_31 + m1->_34 * m2->_41;
    temp._32 = m1->_31 * m2->_12 + m1->_32 * m2->_22 + m1->_33 * m2->_32 + m1->_34 * m2->_42;
    temp._33 = m1->_31 * m2->_13 + m1->_32 * m2->_23 + m1->_33 * m2->_33 + m1->_34 * m2->_43;
    temp._34 = m1->_31 * m2->_14 + m1->_32 * m2->_24 + m1->_33 * m2->_34 + m1->_34 * m2->_44;
    temp._41 = m1->_41 * m2->_11 + m1->_42 * m2->_21 + m1->_43 * m2->_31 + m1->_44 * m2->_41;
    temp._42 = m1->_41 * m2->_12 + m1->_42 * m2->_22 + m1->_43 * m2->_32 + m1->_44 * m2->_42;
    temp._43 = m1->_41 * m2->_13 + m1->_42 * m2->_23 + m1->_43 * m2->_33 + m1->_44 * m2->_43;
    temp._44 = m1->_41 * m2->_14 + m1->_42 * m2->_24 + m1->_43 * m2->_34 + m1->_44 * m2->_44;

    *result = temp;

    return;
}

HRESULT
hr_ddraw_from_wined3d(HRESULT hr)
{
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL: return DDERR_INVALIDPARAMS;
        default: return hr;
    }
}
