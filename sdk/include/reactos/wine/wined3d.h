/*
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Stefan Dösinger
 * Copyright 2006 Stefan Dösinger for CodeWeavers
 * Copyright 2007 Henri Verbeet
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_WINED3D_H
#define __WINE_WINED3D_H

#include "wine/list.h"

DEFINE_GUID(IID_IWineD3DDevice, 0xd56e2a4c, 0x5127, 0x8437, 0x65, 0x8a, 0x98, 0xc5, 0xbb, 0x78, 0x94, 0x98);

#define WINED3D_OK                                              S_OK

#define _FACWINED3D                                             0x876
#define MAKE_WINED3DSTATUS(code)                                MAKE_HRESULT(0, _FACWINED3D, code)
#define WINED3DOK_NOMIPGEN                                      MAKE_WINED3DSTATUS(2159)

#define MAKE_WINED3DHRESULT(code)                               MAKE_HRESULT(1, _FACWINED3D, code)
#define WINED3DERR_CONFLICTINGRENDERSTATE                       MAKE_WINED3DHRESULT(2081)
#define WINED3DERR_UNSUPPORTEDTEXTUREFILTER                     MAKE_WINED3DHRESULT(2082)
#define WINED3DERR_NOTAVAILABLE                                 MAKE_WINED3DHRESULT(2154)
#define WINED3DERR_OUTOFVIDEOMEMORY                             MAKE_WINED3DHRESULT(380)
#define WINED3DERR_INVALIDCALL                                  MAKE_WINED3DHRESULT(2156)
#define WINEDDERR_NOTAOVERLAYSURFACE                            MAKE_WINED3DHRESULT(580)
#define WINEDDERR_NOTLOCKED                                     MAKE_WINED3DHRESULT(584)
#define WINEDDERR_SURFACEBUSY                                   MAKE_WINED3DHRESULT(430)
#define WINEDDERR_INVALIDRECT                                   MAKE_WINED3DHRESULT(150)
#define WINEDDERR_OVERLAYNOTVISIBLE                             MAKE_WINED3DHRESULT(577)

#define WINED3D_RESOURCE_ACCESS_GPU                             0x1u
#define WINED3D_RESOURCE_ACCESS_CPU                             0x2u
#define WINED3D_RESOURCE_ACCESS_MAP_R                           0x4u
#define WINED3D_RESOURCE_ACCESS_MAP_W                           0x8u

enum wined3d_light_type
{
    WINED3D_LIGHT_POINT                     = 1,
    WINED3D_LIGHT_SPOT                      = 2,
    WINED3D_LIGHT_DIRECTIONAL               = 3,
    WINED3D_LIGHT_PARALLELPOINT             = 4, /* < D3D7 */
    WINED3D_LIGHT_GLSPOT                    = 5, /* < D3D5, not actually usable */
};

enum wined3d_primitive_type
{
    WINED3D_PT_UNDEFINED                    = 0,
    WINED3D_PT_POINTLIST                    = 1,
    WINED3D_PT_LINELIST                     = 2,
    WINED3D_PT_LINESTRIP                    = 3,
    WINED3D_PT_TRIANGLELIST                 = 4,
    WINED3D_PT_TRIANGLESTRIP                = 5,
    WINED3D_PT_TRIANGLEFAN                  = 6,
    WINED3D_PT_LINELIST_ADJ                 = 10,
    WINED3D_PT_LINESTRIP_ADJ                = 11,
    WINED3D_PT_TRIANGLELIST_ADJ             = 12,
    WINED3D_PT_TRIANGLESTRIP_ADJ            = 13,
    WINED3D_PT_PATCH                        = 14,
};

enum wined3d_device_type
{
    WINED3D_DEVICE_TYPE_HAL                 = 1,
    WINED3D_DEVICE_TYPE_REF                 = 2,
    WINED3D_DEVICE_TYPE_SW                  = 3,
    WINED3D_DEVICE_TYPE_NULLREF             = 4,
};

enum wined3d_feature_level
{
    WINED3D_FEATURE_LEVEL_NONE   = 0x0000,
    WINED3D_FEATURE_LEVEL_5      = 0x5000,
    WINED3D_FEATURE_LEVEL_6      = 0x6000,
    WINED3D_FEATURE_LEVEL_7      = 0x7000,
    WINED3D_FEATURE_LEVEL_8      = 0x8000,
    WINED3D_FEATURE_LEVEL_9_1    = 0x9100,
    WINED3D_FEATURE_LEVEL_9_2    = 0x9200,
    WINED3D_FEATURE_LEVEL_9_3    = 0x9300,
    WINED3D_FEATURE_LEVEL_10     = 0xa000,
    WINED3D_FEATURE_LEVEL_10_1   = 0xa100,
    WINED3D_FEATURE_LEVEL_11     = 0xb000,
    WINED3D_FEATURE_LEVEL_11_1   = 0xb100,
};

enum wined3d_degree_type
{
    WINED3D_DEGREE_LINEAR                   = 1,
    WINED3D_DEGREE_QUADRATIC                = 2,
    WINED3D_DEGREE_CUBIC                    = 3,
    WINED3D_DEGREE_QUINTIC                  = 5,
};

#define WINEMAKEFOURCC(ch0, ch1, ch2, ch3) \
        ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
        ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

enum wined3d_format_id
{
    WINED3DFMT_UNKNOWN,
    WINED3DFMT_B8G8R8_UNORM,
    WINED3DFMT_B5G5R5X1_UNORM,
    WINED3DFMT_B4G4R4A4_UNORM,
    WINED3DFMT_B2G3R3_UNORM,
    WINED3DFMT_B2G3R3A8_UNORM,
    WINED3DFMT_B4G4R4X4_UNORM,
    WINED3DFMT_R8G8B8X8_UNORM,
    WINED3DFMT_B10G10R10A2_UNORM,
    WINED3DFMT_P8_UINT_A8_UNORM,
    WINED3DFMT_P8_UINT,
    WINED3DFMT_L8_UNORM,
    WINED3DFMT_L8A8_UNORM,
    WINED3DFMT_L4A4_UNORM,
    WINED3DFMT_R5G5_SNORM_L6_UNORM,
    WINED3DFMT_R8G8_SNORM_L8X8_UNORM,
    WINED3DFMT_R10G11B11_SNORM,
    WINED3DFMT_R10G10B10X2_TYPELESS,
    WINED3DFMT_R10G10B10X2_UINT,
    WINED3DFMT_R10G10B10X2_SNORM,
    WINED3DFMT_R10G10B10_SNORM_A2_UNORM,
    WINED3DFMT_D16_LOCKABLE,
    WINED3DFMT_D32_UNORM,
    WINED3DFMT_S1_UINT_D15_UNORM,
    WINED3DFMT_X8D24_UNORM,
    WINED3DFMT_S4X4_UINT_D24_UNORM,
    WINED3DFMT_L16_UNORM,
    WINED3DFMT_S8_UINT_D24_FLOAT,
    WINED3DFMT_R8G8_SNORM_Cx,
    WINED3DFMT_R32G32B32A32_TYPELESS,
    WINED3DFMT_R32G32B32A32_FLOAT,
    WINED3DFMT_R32G32B32A32_UINT,
    WINED3DFMT_R32G32B32A32_SINT,
    WINED3DFMT_R32G32B32_TYPELESS,
    WINED3DFMT_R32G32B32_FLOAT,
    WINED3DFMT_R32G32B32_UINT,
    WINED3DFMT_R32G32B32_SINT,
    WINED3DFMT_R16G16B16A16_TYPELESS,
    WINED3DFMT_R16G16B16A16_FLOAT,
    WINED3DFMT_R16G16B16A16_UNORM,
    WINED3DFMT_R16G16B16A16_UINT,
    WINED3DFMT_R16G16B16A16_SNORM,
    WINED3DFMT_R16G16B16A16_SINT,
    WINED3DFMT_R32G32_TYPELESS,
    WINED3DFMT_R32G32_FLOAT,
    WINED3DFMT_R32G32_UINT,
    WINED3DFMT_R32G32_SINT,
    WINED3DFMT_R32G8X24_TYPELESS,
    WINED3DFMT_D32_FLOAT_S8X24_UINT,
    WINED3DFMT_R32_FLOAT_X8X24_TYPELESS,
    WINED3DFMT_X32_TYPELESS_G8X24_UINT,
    WINED3DFMT_R10G10B10A2_TYPELESS,
    WINED3DFMT_R10G10B10A2_UNORM,
    WINED3DFMT_R10G10B10A2_UINT,
    WINED3DFMT_R10G10B10A2_SNORM,
    WINED3DFMT_R10G10B10_XR_BIAS_A2_UNORM,
    WINED3DFMT_R11G11B10_FLOAT,
    WINED3DFMT_R8G8B8A8_TYPELESS,
    WINED3DFMT_R8G8B8A8_UNORM,
    WINED3DFMT_R8G8B8A8_UNORM_SRGB,
    WINED3DFMT_R8G8B8A8_UINT,
    WINED3DFMT_R8G8B8A8_SNORM,
    WINED3DFMT_R8G8B8A8_SINT,
    WINED3DFMT_R16G16_TYPELESS,
    WINED3DFMT_R16G16_FLOAT,
    WINED3DFMT_R16G16_UNORM,
    WINED3DFMT_R16G16_UINT,
    WINED3DFMT_R16G16_SNORM,
    WINED3DFMT_R16G16_SINT,
    WINED3DFMT_R32_TYPELESS,
    WINED3DFMT_D32_FLOAT,
    WINED3DFMT_R32_FLOAT,
    WINED3DFMT_R32_UINT,
    WINED3DFMT_R32_SINT,
    WINED3DFMT_R24G8_TYPELESS,
    WINED3DFMT_D24_UNORM_S8_UINT,
    WINED3DFMT_R24_UNORM_X8_TYPELESS,
    WINED3DFMT_X24_TYPELESS_G8_UINT,
    WINED3DFMT_R8G8_TYPELESS,
    WINED3DFMT_R8G8_UNORM,
    WINED3DFMT_R8G8_UINT,
    WINED3DFMT_R8G8_SNORM,
    WINED3DFMT_R8G8_SINT,
    WINED3DFMT_R16_TYPELESS,
    WINED3DFMT_R16_FLOAT,
    WINED3DFMT_D16_UNORM,
    WINED3DFMT_R16_UNORM,
    WINED3DFMT_R16_UINT,
    WINED3DFMT_R16_SNORM,
    WINED3DFMT_R16_SINT,
    WINED3DFMT_R8_TYPELESS,
    WINED3DFMT_R8_UNORM,
    WINED3DFMT_R8_UINT,
    WINED3DFMT_R8_SNORM,
    WINED3DFMT_R8_SINT,
    WINED3DFMT_A8_UNORM,
    WINED3DFMT_R1_UNORM,
    WINED3DFMT_R9G9B9E5_SHAREDEXP,
    WINED3DFMT_R8G8_B8G8_UNORM,
    WINED3DFMT_G8R8_G8B8_UNORM,
    WINED3DFMT_BC1_TYPELESS,
    WINED3DFMT_BC1_UNORM,
    WINED3DFMT_BC1_UNORM_SRGB,
    WINED3DFMT_BC2_TYPELESS,
    WINED3DFMT_BC2_UNORM,
    WINED3DFMT_BC2_UNORM_SRGB,
    WINED3DFMT_BC3_TYPELESS,
    WINED3DFMT_BC3_UNORM,
    WINED3DFMT_BC3_UNORM_SRGB,
    WINED3DFMT_BC4_TYPELESS,
    WINED3DFMT_BC4_UNORM,
    WINED3DFMT_BC4_SNORM,
    WINED3DFMT_BC5_TYPELESS,
    WINED3DFMT_BC5_UNORM,
    WINED3DFMT_BC5_SNORM,
    WINED3DFMT_B5G6R5_UNORM,
    WINED3DFMT_B5G5R5A1_UNORM,
    WINED3DFMT_B8G8R8A8_UNORM,
    WINED3DFMT_B8G8R8X8_UNORM,
    WINED3DFMT_B8G8R8A8_TYPELESS,
    WINED3DFMT_B8G8R8A8_UNORM_SRGB,
    WINED3DFMT_B8G8R8X8_TYPELESS,
    WINED3DFMT_B8G8R8X8_UNORM_SRGB,
    WINED3DFMT_BC6H_TYPELESS,
    WINED3DFMT_BC6H_UF16,
    WINED3DFMT_BC6H_SF16,
    WINED3DFMT_BC7_TYPELESS,
    WINED3DFMT_BC7_UNORM,
    WINED3DFMT_BC7_UNORM_SRGB,
    /* FOURCC formats. */
    WINED3DFMT_UYVY                         = WINEMAKEFOURCC('U','Y','V','Y'),
    WINED3DFMT_YUY2                         = WINEMAKEFOURCC('Y','U','Y','2'),
    WINED3DFMT_YV12                         = WINEMAKEFOURCC('Y','V','1','2'),
    WINED3DFMT_DXT1                         = WINEMAKEFOURCC('D','X','T','1'),
    WINED3DFMT_DXT2                         = WINEMAKEFOURCC('D','X','T','2'),
    WINED3DFMT_DXT3                         = WINEMAKEFOURCC('D','X','T','3'),
    WINED3DFMT_DXT4                         = WINEMAKEFOURCC('D','X','T','4'),
    WINED3DFMT_DXT5                         = WINEMAKEFOURCC('D','X','T','5'),
    WINED3DFMT_MULTI2_ARGB8                 = WINEMAKEFOURCC('M','E','T','1'),
    WINED3DFMT_G8R8_G8B8                    = WINEMAKEFOURCC('G','R','G','B'),
    WINED3DFMT_R8G8_B8G8                    = WINEMAKEFOURCC('R','G','B','G'),
    WINED3DFMT_ATI1N                        = WINEMAKEFOURCC('A','T','I','1'),
    WINED3DFMT_ATI2N                        = WINEMAKEFOURCC('A','T','I','2'),
    WINED3DFMT_INST                         = WINEMAKEFOURCC('I','N','S','T'),
    WINED3DFMT_NVDB                         = WINEMAKEFOURCC('N','V','D','B'),
    WINED3DFMT_NVHU                         = WINEMAKEFOURCC('N','V','H','U'),
    WINED3DFMT_NVHS                         = WINEMAKEFOURCC('N','V','H','S'),
    WINED3DFMT_INTZ                         = WINEMAKEFOURCC('I','N','T','Z'),
    WINED3DFMT_RESZ                         = WINEMAKEFOURCC('R','E','S','Z'),
    WINED3DFMT_NULL                         = WINEMAKEFOURCC('N','U','L','L'),
    WINED3DFMT_R16                          = WINEMAKEFOURCC(' ','R','1','6'),
    WINED3DFMT_AL16                         = WINEMAKEFOURCC('A','L','1','6'),
    WINED3DFMT_NV12                         = WINEMAKEFOURCC('N','V','1','2'),

    WINED3DFMT_FORCE_DWORD = 0xffffffff
};

enum wined3d_render_state
{
    WINED3D_RS_ANTIALIAS                    = 2,
    WINED3D_RS_TEXTUREPERSPECTIVE           = 4,
    WINED3D_RS_WRAPU                        = 5,
    WINED3D_RS_WRAPV                        = 6,
    WINED3D_RS_ZENABLE                      = 7,
    WINED3D_RS_FILLMODE                     = 8,
    WINED3D_RS_SHADEMODE                    = 9,
    WINED3D_RS_LINEPATTERN                  = 10,
    WINED3D_RS_MONOENABLE                   = 11,
    WINED3D_RS_ROP2                         = 12,
    WINED3D_RS_PLANEMASK                    = 13,
    WINED3D_RS_ZWRITEENABLE                 = 14,
    WINED3D_RS_ALPHATESTENABLE              = 15,
    WINED3D_RS_LASTPIXEL                    = 16,
    WINED3D_RS_SRCBLEND                     = 19,
    WINED3D_RS_DESTBLEND                    = 20,
    WINED3D_RS_CULLMODE                     = 22,
    WINED3D_RS_ZFUNC                        = 23,
    WINED3D_RS_ALPHAREF                     = 24,
    WINED3D_RS_ALPHAFUNC                    = 25,
    WINED3D_RS_DITHERENABLE                 = 26,
    WINED3D_RS_ALPHABLENDENABLE             = 27,
    WINED3D_RS_FOGENABLE                    = 28,
    WINED3D_RS_SPECULARENABLE               = 29,
    WINED3D_RS_ZVISIBLE                     = 30,
    WINED3D_RS_SUBPIXEL                     = 31,
    WINED3D_RS_SUBPIXELX                    = 32,
    WINED3D_RS_STIPPLEDALPHA                = 33,
    WINED3D_RS_FOGCOLOR                     = 34,
    WINED3D_RS_FOGTABLEMODE                 = 35,
    WINED3D_RS_FOGSTART                     = 36,
    WINED3D_RS_FOGEND                       = 37,
    WINED3D_RS_FOGDENSITY                   = 38,
    WINED3D_RS_STIPPLEENABLE                = 39,
    WINED3D_RS_EDGEANTIALIAS                = 40,
    WINED3D_RS_COLORKEYENABLE               = 41,
    WINED3D_RS_MIPMAPLODBIAS                = 46,
    WINED3D_RS_RANGEFOGENABLE               = 48,
    WINED3D_RS_ANISOTROPY                   = 49,
    WINED3D_RS_FLUSHBATCH                   = 50,
    WINED3D_RS_TRANSLUCENTSORTINDEPENDENT   = 51,
    WINED3D_RS_STENCILENABLE                = 52,
    WINED3D_RS_STENCILFAIL                  = 53,
    WINED3D_RS_STENCILZFAIL                 = 54,
    WINED3D_RS_STENCILPASS                  = 55,
    WINED3D_RS_STENCILFUNC                  = 56,
    WINED3D_RS_STENCILREF                   = 57,
    WINED3D_RS_STENCILMASK                  = 58,
    WINED3D_RS_STENCILWRITEMASK             = 59,
    WINED3D_RS_TEXTUREFACTOR                = 60,
    WINED3D_RS_WRAP0                        = 128,
    WINED3D_RS_WRAP1                        = 129,
    WINED3D_RS_WRAP2                        = 130,
    WINED3D_RS_WRAP3                        = 131,
    WINED3D_RS_WRAP4                        = 132,
    WINED3D_RS_WRAP5                        = 133,
    WINED3D_RS_WRAP6                        = 134,
    WINED3D_RS_WRAP7                        = 135,
    WINED3D_RS_CLIPPING                     = 136,
    WINED3D_RS_LIGHTING                     = 137,
    WINED3D_RS_EXTENTS                      = 138,
    WINED3D_RS_AMBIENT                      = 139,
    WINED3D_RS_FOGVERTEXMODE                = 140,
    WINED3D_RS_COLORVERTEX                  = 141,
    WINED3D_RS_LOCALVIEWER                  = 142,
    WINED3D_RS_NORMALIZENORMALS             = 143,
    WINED3D_RS_COLORKEYBLENDENABLE          = 144,
    WINED3D_RS_DIFFUSEMATERIALSOURCE        = 145,
    WINED3D_RS_SPECULARMATERIALSOURCE       = 146,
    WINED3D_RS_AMBIENTMATERIALSOURCE        = 147,
    WINED3D_RS_EMISSIVEMATERIALSOURCE       = 148,
    WINED3D_RS_VERTEXBLEND                  = 151,
    WINED3D_RS_CLIPPLANEENABLE              = 152,
    WINED3D_RS_SOFTWAREVERTEXPROCESSING     = 153,
    WINED3D_RS_POINTSIZE                    = 154,
    WINED3D_RS_POINTSIZE_MIN                = 155,
    WINED3D_RS_POINTSPRITEENABLE            = 156,
    WINED3D_RS_POINTSCALEENABLE             = 157,
    WINED3D_RS_POINTSCALE_A                 = 158,
    WINED3D_RS_POINTSCALE_B                 = 159,
    WINED3D_RS_POINTSCALE_C                 = 160,
    WINED3D_RS_MULTISAMPLEANTIALIAS         = 161,
    WINED3D_RS_MULTISAMPLEMASK              = 162,
    WINED3D_RS_PATCHEDGESTYLE               = 163,
    WINED3D_RS_PATCHSEGMENTS                = 164,
    WINED3D_RS_DEBUGMONITORTOKEN            = 165,
    WINED3D_RS_POINTSIZE_MAX                = 166,
    WINED3D_RS_INDEXEDVERTEXBLENDENABLE     = 167,
    WINED3D_RS_COLORWRITEENABLE             = 168,
    WINED3D_RS_TWEENFACTOR                  = 170,
    WINED3D_RS_BLENDOP                      = 171,
    WINED3D_RS_POSITIONDEGREE               = 172,
    WINED3D_RS_NORMALDEGREE                 = 173,
    WINED3D_RS_SCISSORTESTENABLE            = 174,
    WINED3D_RS_SLOPESCALEDEPTHBIAS          = 175,
    WINED3D_RS_ANTIALIASEDLINEENABLE        = 176,
    WINED3D_RS_MINTESSELLATIONLEVEL         = 178,
    WINED3D_RS_MAXTESSELLATIONLEVEL         = 179,
    WINED3D_RS_ADAPTIVETESS_X               = 180,
    WINED3D_RS_ADAPTIVETESS_Y               = 181,
    WINED3D_RS_ADAPTIVETESS_Z               = 182,
    WINED3D_RS_ADAPTIVETESS_W               = 183,
    WINED3D_RS_ENABLEADAPTIVETESSELLATION   = 184,
    WINED3D_RS_TWOSIDEDSTENCILMODE          = 185,
    WINED3D_RS_BACK_STENCILFAIL             = 186,
    WINED3D_RS_BACK_STENCILZFAIL            = 187,
    WINED3D_RS_BACK_STENCILPASS             = 188,
    WINED3D_RS_BACK_STENCILFUNC             = 189,
    WINED3D_RS_COLORWRITEENABLE1            = 190,
    WINED3D_RS_COLORWRITEENABLE2            = 191,
    WINED3D_RS_COLORWRITEENABLE3            = 192,
    WINED3D_RS_SRGBWRITEENABLE              = 194,
    WINED3D_RS_DEPTHBIAS                    = 195,
    WINED3D_RS_WRAP8                        = 198,
    WINED3D_RS_WRAP9                        = 199,
    WINED3D_RS_WRAP10                       = 200,
    WINED3D_RS_WRAP11                       = 201,
    WINED3D_RS_WRAP12                       = 202,
    WINED3D_RS_WRAP13                       = 203,
    WINED3D_RS_WRAP14                       = 204,
    WINED3D_RS_WRAP15                       = 205,
    WINED3D_RS_SEPARATEALPHABLENDENABLE     = 206,
    WINED3D_RS_SRCBLENDALPHA                = 207,
    WINED3D_RS_DESTBLENDALPHA               = 208,
    WINED3D_RS_BLENDOPALPHA                 = 209,
    WINED3D_RS_DEPTHCLIP                    = 210,
    WINED3D_RS_DEPTHBIASCLAMP               = 211,
    WINED3D_RS_COLORWRITEENABLE4            = 212,
    WINED3D_RS_COLORWRITEENABLE5            = 213,
    WINED3D_RS_COLORWRITEENABLE6            = 214,
    WINED3D_RS_COLORWRITEENABLE7            = 215,
};
#define WINEHIGHEST_RENDER_STATE                                WINED3D_RS_COLORWRITEENABLE7

static inline enum wined3d_render_state WINED3D_RS_COLORWRITE(int index)
{
    if (index == 0) return WINED3D_RS_COLORWRITEENABLE;
    if (index <= 3) return WINED3D_RS_COLORWRITEENABLE1 + index - 1;
    if (index <= 7) return WINED3D_RS_COLORWRITEENABLE4 + index - 4;
    return WINED3D_RS_COLORWRITEENABLE;
}

enum wined3d_blend
{
    WINED3D_BLEND_ZERO                      =  1,
    WINED3D_BLEND_ONE                       =  2,
    WINED3D_BLEND_SRCCOLOR                  =  3,
    WINED3D_BLEND_INVSRCCOLOR               =  4,
    WINED3D_BLEND_SRCALPHA                  =  5,
    WINED3D_BLEND_INVSRCALPHA               =  6,
    WINED3D_BLEND_DESTALPHA                 =  7,
    WINED3D_BLEND_INVDESTALPHA              =  8,
    WINED3D_BLEND_DESTCOLOR                 =  9,
    WINED3D_BLEND_INVDESTCOLOR              = 10,
    WINED3D_BLEND_SRCALPHASAT               = 11,
    WINED3D_BLEND_BOTHSRCALPHA              = 12,
    WINED3D_BLEND_BOTHINVSRCALPHA           = 13,
    WINED3D_BLEND_BLENDFACTOR               = 14,
    WINED3D_BLEND_INVBLENDFACTOR            = 15,
    WINED3D_BLEND_SRC1COLOR                 = 16,
    WINED3D_BLEND_INVSRC1COLOR              = 17,
    WINED3D_BLEND_SRC1ALPHA                 = 18,
    WINED3D_BLEND_INVSRC1ALPHA              = 19,
};

enum wined3d_blend_op
{
    WINED3D_BLEND_OP_ADD                    = 1,
    WINED3D_BLEND_OP_SUBTRACT               = 2,
    WINED3D_BLEND_OP_REVSUBTRACT            = 3,
    WINED3D_BLEND_OP_MIN                    = 4,
    WINED3D_BLEND_OP_MAX                    = 5,
};

enum wined3d_vertex_blend_flags
{
    WINED3D_VBF_DISABLE                     = 0,
    WINED3D_VBF_1WEIGHTS                    = 1,
    WINED3D_VBF_2WEIGHTS                    = 2,
    WINED3D_VBF_3WEIGHTS                    = 3,
    WINED3D_VBF_TWEENING                    = 255,
    WINED3D_VBF_0WEIGHTS                    = 256,
};

enum wined3d_cmp_func
{
    WINED3D_CMP_NEVER                       = 1,
    WINED3D_CMP_LESS                        = 2,
    WINED3D_CMP_EQUAL                       = 3,
    WINED3D_CMP_LESSEQUAL                   = 4,
    WINED3D_CMP_GREATER                     = 5,
    WINED3D_CMP_NOTEQUAL                    = 6,
    WINED3D_CMP_GREATEREQUAL                = 7,
    WINED3D_CMP_ALWAYS                      = 8,
};

enum wined3d_depth_buffer_type
{
    WINED3D_ZB_FALSE                        = 0,
    WINED3D_ZB_TRUE                         = 1,
    WINED3D_ZB_USEW                         = 2,
};

enum wined3d_fog_mode
{
    WINED3D_FOG_NONE                        = 0,
    WINED3D_FOG_EXP                         = 1,
    WINED3D_FOG_EXP2                        = 2,
    WINED3D_FOG_LINEAR                      = 3,
};

enum wined3d_shade_mode
{
    WINED3D_SHADE_FLAT                      = 1,
    WINED3D_SHADE_GOURAUD                   = 2,
    WINED3D_SHADE_PHONG                     = 3,
};

enum wined3d_fill_mode
{
    WINED3D_FILL_POINT                      = 1,
    WINED3D_FILL_WIREFRAME                  = 2,
    WINED3D_FILL_SOLID                      = 3,
};

enum wined3d_cull
{
    WINED3D_CULL_NONE                       = 1,
    WINED3D_CULL_FRONT                      = 2,
    WINED3D_CULL_BACK                       = 3,
};

enum wined3d_stencil_op
{
    WINED3D_STENCIL_OP_KEEP                 = 1,
    WINED3D_STENCIL_OP_ZERO                 = 2,
    WINED3D_STENCIL_OP_REPLACE              = 3,
    WINED3D_STENCIL_OP_INCR_SAT             = 4,
    WINED3D_STENCIL_OP_DECR_SAT             = 5,
    WINED3D_STENCIL_OP_INVERT               = 6,
    WINED3D_STENCIL_OP_INCR                 = 7,
    WINED3D_STENCIL_OP_DECR                 = 8,
};

enum wined3d_material_color_source
{
    WINED3D_MCS_MATERIAL                    = 0,
    WINED3D_MCS_COLOR1                      = 1,
    WINED3D_MCS_COLOR2                      = 2,
};

enum wined3d_patch_edge_style
{
    WINED3D_PATCH_EDGE_DISCRETE             = 0,
    WINED3D_PATCH_EDGE_CONTINUOUS           = 1,
};

enum wined3d_swap_effect
{
    WINED3D_SWAP_EFFECT_DISCARD,
    WINED3D_SWAP_EFFECT_SEQUENTIAL,
    WINED3D_SWAP_EFFECT_FLIP_DISCARD,
    WINED3D_SWAP_EFFECT_FLIP_SEQUENTIAL,
    WINED3D_SWAP_EFFECT_COPY,
    WINED3D_SWAP_EFFECT_COPY_VSYNC,
    WINED3D_SWAP_EFFECT_OVERLAY,
};

enum wined3d_swap_interval
{
    WINED3D_SWAP_INTERVAL_IMMEDIATE = 0,
    WINED3D_SWAP_INTERVAL_ONE       = 1,
    WINED3D_SWAP_INTERVAL_TWO       = 2,
    WINED3D_SWAP_INTERVAL_THREE     = 3,
    WINED3D_SWAP_INTERVAL_FOUR      = 4,
    WINED3D_SWAP_INTERVAL_DEFAULT   = ~0u,
};

enum wined3d_sampler_state
{
    WINED3D_SAMP_ADDRESS_U                  = 1,
    WINED3D_SAMP_ADDRESS_V                  = 2,
    WINED3D_SAMP_ADDRESS_W                  = 3,
    WINED3D_SAMP_BORDER_COLOR               = 4,
    WINED3D_SAMP_MAG_FILTER                 = 5,
    WINED3D_SAMP_MIN_FILTER                 = 6,
    WINED3D_SAMP_MIP_FILTER                 = 7,
    WINED3D_SAMP_MIPMAP_LOD_BIAS            = 8,
    WINED3D_SAMP_MAX_MIP_LEVEL              = 9,
    WINED3D_SAMP_MAX_ANISOTROPY             = 10,
    WINED3D_SAMP_SRGB_TEXTURE               = 11,
    WINED3D_SAMP_ELEMENT_INDEX              = 12,
    WINED3D_SAMP_DMAP_OFFSET                = 13,
};
#define WINED3D_HIGHEST_SAMPLER_STATE                           WINED3D_SAMP_DMAP_OFFSET

enum wined3d_multisample_type
{
    WINED3D_MULTISAMPLE_NONE                = 0,
    WINED3D_MULTISAMPLE_NON_MASKABLE        = 1,
    WINED3D_MULTISAMPLE_2_SAMPLES           = 2,
    WINED3D_MULTISAMPLE_3_SAMPLES           = 3,
    WINED3D_MULTISAMPLE_4_SAMPLES           = 4,
    WINED3D_MULTISAMPLE_5_SAMPLES           = 5,
    WINED3D_MULTISAMPLE_6_SAMPLES           = 6,
    WINED3D_MULTISAMPLE_7_SAMPLES           = 7,
    WINED3D_MULTISAMPLE_8_SAMPLES           = 8,
    WINED3D_MULTISAMPLE_9_SAMPLES           = 9,
    WINED3D_MULTISAMPLE_10_SAMPLES          = 10,
    WINED3D_MULTISAMPLE_11_SAMPLES          = 11,
    WINED3D_MULTISAMPLE_12_SAMPLES          = 12,
    WINED3D_MULTISAMPLE_13_SAMPLES          = 13,
    WINED3D_MULTISAMPLE_14_SAMPLES          = 14,
    WINED3D_MULTISAMPLE_15_SAMPLES          = 15,
    WINED3D_MULTISAMPLE_16_SAMPLES          = 16,
};

enum wined3d_texture_stage_state
{
    WINED3D_TSS_COLOR_OP                    = 0,
    WINED3D_TSS_COLOR_ARG1                  = 1,
    WINED3D_TSS_COLOR_ARG2                  = 2,
    WINED3D_TSS_ALPHA_OP                    = 3,
    WINED3D_TSS_ALPHA_ARG1                  = 4,
    WINED3D_TSS_ALPHA_ARG2                  = 5,
    WINED3D_TSS_BUMPENV_MAT00               = 6,
    WINED3D_TSS_BUMPENV_MAT01               = 7,
    WINED3D_TSS_BUMPENV_MAT10               = 8,
    WINED3D_TSS_BUMPENV_MAT11               = 9,
    WINED3D_TSS_TEXCOORD_INDEX              = 10,
    WINED3D_TSS_BUMPENV_LSCALE              = 11,
    WINED3D_TSS_BUMPENV_LOFFSET             = 12,
    WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS     = 13,
    WINED3D_TSS_COLOR_ARG0                  = 14,
    WINED3D_TSS_ALPHA_ARG0                  = 15,
    WINED3D_TSS_RESULT_ARG                  = 16,
    WINED3D_TSS_CONSTANT                    = 17,
    WINED3D_TSS_INVALID                     = ~0u,
};
#define WINED3D_HIGHEST_TEXTURE_STATE                           WINED3D_TSS_CONSTANT

enum wined3d_texture_transform_flags
{
    WINED3D_TTFF_DISABLE                    = 0,
    WINED3D_TTFF_COUNT1                     = 1,
    WINED3D_TTFF_COUNT2                     = 2,
    WINED3D_TTFF_COUNT3                     = 3,
    WINED3D_TTFF_COUNT4                     = 4,
    WINED3D_TTFF_PROJECTED                  = 256,
};

enum wined3d_texture_op
{
    WINED3D_TOP_DISABLE                     = 1,
    WINED3D_TOP_SELECT_ARG1                 = 2,
    WINED3D_TOP_SELECT_ARG2                 = 3,
    WINED3D_TOP_MODULATE                    = 4,
    WINED3D_TOP_MODULATE_2X                 = 5,
    WINED3D_TOP_MODULATE_4X                 = 6,
    WINED3D_TOP_ADD                         = 7,
    WINED3D_TOP_ADD_SIGNED                  = 8,
    WINED3D_TOP_ADD_SIGNED_2X               = 9,
    WINED3D_TOP_SUBTRACT                    = 10,
    WINED3D_TOP_ADD_SMOOTH                  = 11,
    WINED3D_TOP_BLEND_DIFFUSE_ALPHA         = 12,
    WINED3D_TOP_BLEND_TEXTURE_ALPHA         = 13,
    WINED3D_TOP_BLEND_FACTOR_ALPHA          = 14,
    WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM      = 15,
    WINED3D_TOP_BLEND_CURRENT_ALPHA         = 16,
    WINED3D_TOP_PREMODULATE                 = 17,
    WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR    = 18,
    WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA    = 19,
    WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR = 20,
    WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA = 21,
    WINED3D_TOP_BUMPENVMAP                  = 22,
    WINED3D_TOP_BUMPENVMAP_LUMINANCE        = 23,
    WINED3D_TOP_DOTPRODUCT3                 = 24,
    WINED3D_TOP_MULTIPLY_ADD                = 25,
    WINED3D_TOP_LERP                        = 26,
};

enum wined3d_texture_address
{
    WINED3D_TADDRESS_WRAP                   = 1,
    WINED3D_TADDRESS_MIRROR                 = 2,
    WINED3D_TADDRESS_CLAMP                  = 3,
    WINED3D_TADDRESS_BORDER                 = 4,
    WINED3D_TADDRESS_MIRROR_ONCE            = 5,
};

enum wined3d_transform_state
{
    WINED3D_TS_VIEW                         = 2,
    WINED3D_TS_PROJECTION                   = 3,
    WINED3D_TS_TEXTURE0                     = 16,
    WINED3D_TS_TEXTURE1                     = 17,
    WINED3D_TS_TEXTURE2                     = 18,
    WINED3D_TS_TEXTURE3                     = 19,
    WINED3D_TS_TEXTURE4                     = 20,
    WINED3D_TS_TEXTURE5                     = 21,
    WINED3D_TS_TEXTURE6                     = 22,
    WINED3D_TS_TEXTURE7                     = 23,
    WINED3D_TS_WORLD                        = 256, /* WINED3D_TS_WORLD_MATRIX(0) */
    WINED3D_TS_WORLD1                       = 257,
    WINED3D_TS_WORLD2                       = 258,
    WINED3D_TS_WORLD3                       = 259,
};

#define WINED3D_TS_WORLD_MATRIX(index)                          (enum wined3d_transform_state)(index + 256)

enum wined3d_basis_type
{
    WINED3D_BASIS_BEZIER                    = 0,
    WINED3D_BASIS_BSPLINE                   = 1,
    WINED3D_BASIS_INTERPOLATE               = 2,
};

enum wined3d_cubemap_face
{
    WINED3D_CUBEMAP_FACE_POSITIVE_X         = 0,
    WINED3D_CUBEMAP_FACE_NEGATIVE_X         = 1,
    WINED3D_CUBEMAP_FACE_POSITIVE_Y         = 2,
    WINED3D_CUBEMAP_FACE_NEGATIVE_Y         = 3,
    WINED3D_CUBEMAP_FACE_POSITIVE_Z         = 4,
    WINED3D_CUBEMAP_FACE_NEGATIVE_Z         = 5,
};

enum wined3d_texture_filter_type
{
    WINED3D_TEXF_NONE                       = 0,
    WINED3D_TEXF_POINT                      = 1,
    WINED3D_TEXF_LINEAR                     = 2,
    WINED3D_TEXF_ANISOTROPIC                = 3,
    WINED3D_TEXF_FLAT_CUBIC                 = 4,
    WINED3D_TEXF_GAUSSIAN_CUBIC             = 5,
    WINED3D_TEXF_PYRAMIDAL_QUAD             = 6,
    WINED3D_TEXF_GAUSSIAN_QUAD              = 7,
};

enum wined3d_resource_type
{
    WINED3D_RTYPE_NONE                      = 0,
    WINED3D_RTYPE_BUFFER                    = 1,
    WINED3D_RTYPE_TEXTURE_1D                = 2,
    WINED3D_RTYPE_TEXTURE_2D                = 3,
    WINED3D_RTYPE_TEXTURE_3D                = 4,
};

enum wined3d_query_type
{
    WINED3D_QUERY_TYPE_VCACHE                = 4,
    WINED3D_QUERY_TYPE_RESOURCE_MANAGER      = 5,
    WINED3D_QUERY_TYPE_VERTEX_STATS          = 6,
    WINED3D_QUERY_TYPE_EVENT                 = 8,
    WINED3D_QUERY_TYPE_OCCLUSION             = 9,
    WINED3D_QUERY_TYPE_TIMESTAMP             = 10,
    WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT    = 11,
    WINED3D_QUERY_TYPE_TIMESTAMP_FREQ        = 12,
    WINED3D_QUERY_TYPE_PIPELINE_TIMINGS      = 13,
    WINED3D_QUERY_TYPE_INTERFACE_TIMINGS     = 14,
    WINED3D_QUERY_TYPE_VERTEX_TIMINGS        = 15,
    WINED3D_QUERY_TYPE_PIXEL_TIMINGS         = 16,
    WINED3D_QUERY_TYPE_BANDWIDTH_TIMINGS     = 17,
    WINED3D_QUERY_TYPE_CACHE_UTILIZATION     = 18,
    WINED3D_QUERY_TYPE_MEMORY_PRESSURE       = 19,
    WINED3D_QUERY_TYPE_PIPELINE_STATISTICS   = 20,
    WINED3D_QUERY_TYPE_SO_STATISTICS         = 21,
    WINED3D_QUERY_TYPE_SO_OVERFLOW           = 22,
    WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM0 = 23,
    WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM1 = 24,
    WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM2 = 25,
    WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM3 = 26,
    WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM0   = 27,
    WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM1   = 28,
    WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM2   = 29,
    WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM3   = 30,
};

struct wined3d_query_data_timestamp_disjoint
{
    UINT64 frequency;
    BOOL disjoint;
};

struct wined3d_query_data_so_statistics
{
    UINT64 primitives_written;
    UINT64 primitives_generated;
};

struct wined3d_query_data_pipeline_statistics
{
    UINT64 vertices_submitted;
    UINT64 primitives_submitted;
    UINT64 vs_invocations;
    UINT64 gs_invocations;
    UINT64 gs_primitives;
    UINT64 clipping_input_primitives;
    UINT64 clipping_output_primitives;
    UINT64 ps_invocations;
    UINT64 hs_invocations;
    UINT64 ds_invocations;
    UINT64 cs_invocations;
};

#define WINED3DISSUE_BEGIN                                      (1u << 1)
#define WINED3DISSUE_END                                        (1u << 0)
#define WINED3DGETDATA_FLUSH                                    (1u << 0)

enum wined3d_stateblock_type
{
    WINED3D_SBT_ALL                         = 1,
    WINED3D_SBT_PIXEL_STATE                 = 2,
    WINED3D_SBT_VERTEX_STATE                = 3,
    WINED3D_SBT_RECORDED                    = 4, /* WineD3D private */
    WINED3D_SBT_PRIMARY                     = 5, /* WineD3D private */
};

enum wined3d_decl_method
{
    WINED3D_DECL_METHOD_DEFAULT             = 0,
    WINED3D_DECL_METHOD_PARTIAL_U           = 1,
    WINED3D_DECL_METHOD_PARTIAL_V           = 2,
    WINED3D_DECL_METHOD_CROSS_UV            = 3,
    WINED3D_DECL_METHOD_UV                  = 4,
    WINED3D_DECL_METHOD_LOOKUP              = 5,
    WINED3D_DECL_METHOD_LOOKUP_PRESAMPLED   = 6,
};

enum wined3d_decl_usage
{
    WINED3D_DECL_USAGE_POSITION             = 0,
    WINED3D_DECL_USAGE_BLEND_WEIGHT         = 1,
    WINED3D_DECL_USAGE_BLEND_INDICES        = 2,
    WINED3D_DECL_USAGE_NORMAL               = 3,
    WINED3D_DECL_USAGE_PSIZE                = 4,
    WINED3D_DECL_USAGE_TEXCOORD             = 5,
    WINED3D_DECL_USAGE_TANGENT              = 6,
    WINED3D_DECL_USAGE_BINORMAL             = 7,
    WINED3D_DECL_USAGE_TESS_FACTOR          = 8,
    WINED3D_DECL_USAGE_POSITIONT            = 9,
    WINED3D_DECL_USAGE_COLOR                = 10,
    WINED3D_DECL_USAGE_FOG                  = 11,
    WINED3D_DECL_USAGE_DEPTH                = 12,
    WINED3D_DECL_USAGE_SAMPLE               = 13
};

enum wined3d_sysval_semantic
{
    WINED3D_SV_POSITION                     = 1,
    WINED3D_SV_CLIP_DISTANCE                = 2,
    WINED3D_SV_CULL_DISTANCE                = 3,
    WINED3D_SV_RENDER_TARGET_ARRAY_INDEX    = 4,
    WINED3D_SV_VIEWPORT_ARRAY_INDEX         = 5,
    WINED3D_SV_VERTEX_ID                    = 6,
    WINED3D_SV_PRIMITIVE_ID                 = 7,
    WINED3D_SV_INSTANCE_ID                  = 8,
    WINED3D_SV_IS_FRONT_FACE                = 9,
    WINED3D_SV_SAMPLE_INDEX                 = 10,
    WINED3D_SV_TESS_FACTOR_QUADEDGE         = 11,
    WINED3D_SV_TESS_FACTOR_QUADINT          = 12,
    WINED3D_SV_TESS_FACTOR_TRIEDGE          = 13,
    WINED3D_SV_TESS_FACTOR_TRIINT           = 14,
    WINED3D_SV_TESS_FACTOR_LINEDET          = 15,
    WINED3D_SV_TESS_FACTOR_LINEDEN          = 16,
};

enum wined3d_component_type
{
    WINED3D_TYPE_UNKNOWN = 0,
    WINED3D_TYPE_UINT    = 1,
    WINED3D_TYPE_INT     = 2,
    WINED3D_TYPE_FLOAT   = 3,
};

enum wined3d_scanline_ordering
{
    WINED3D_SCANLINE_ORDERING_UNKNOWN       = 0,
    WINED3D_SCANLINE_ORDERING_PROGRESSIVE   = 1,
    WINED3D_SCANLINE_ORDERING_INTERLACED    = 2,
};

enum wined3d_display_rotation
{
    WINED3D_DISPLAY_ROTATION_UNSPECIFIED    = 0,
    WINED3D_DISPLAY_ROTATION_0              = 1,
    WINED3D_DISPLAY_ROTATION_90             = 2,
    WINED3D_DISPLAY_ROTATION_180            = 3,
    WINED3D_DISPLAY_ROTATION_270            = 4,
};

enum wined3d_shader_type
{
    WINED3D_SHADER_TYPE_PIXEL,
    WINED3D_SHADER_TYPE_VERTEX,
    WINED3D_SHADER_TYPE_GEOMETRY,
    WINED3D_SHADER_TYPE_HULL,
    WINED3D_SHADER_TYPE_DOMAIN,
    WINED3D_SHADER_TYPE_GRAPHICS_COUNT,

    WINED3D_SHADER_TYPE_COMPUTE = WINED3D_SHADER_TYPE_GRAPHICS_COUNT,
    WINED3D_SHADER_TYPE_COUNT,
    WINED3D_SHADER_TYPE_INVALID = WINED3D_SHADER_TYPE_COUNT,
};

#define WINED3DCOLORWRITEENABLE_RED                             (1u << 0)
#define WINED3DCOLORWRITEENABLE_GREEN                           (1u << 1)
#define WINED3DCOLORWRITEENABLE_BLUE                            (1u << 2)
#define WINED3DCOLORWRITEENABLE_ALPHA                           (1u << 3)

#define WINED3DADAPTER_DEFAULT                                  0
#define WINED3DENUM_NO_WHQL_LEVEL                               2

#define WINED3DTSS_TCI_PASSTHRU                                 0x00000
#define WINED3DTSS_TCI_CAMERASPACENORMAL                        0x10000
#define WINED3DTSS_TCI_CAMERASPACEPOSITION                      0x20000
#define WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR              0x30000
#define WINED3DTSS_TCI_SPHEREMAP                                0x40000

#define WINED3DTA_SELECTMASK                                    0x0000000f
#define WINED3DTA_DIFFUSE                                       0x00000000
#define WINED3DTA_CURRENT                                       0x00000001
#define WINED3DTA_TEXTURE                                       0x00000002
#define WINED3DTA_TFACTOR                                       0x00000003
#define WINED3DTA_SPECULAR                                      0x00000004
#define WINED3DTA_TEMP                                          0x00000005
#define WINED3DTA_CONSTANT                                      0x00000006
#define WINED3DTA_COMPLEMENT                                    0x00000010
#define WINED3DTA_ALPHAREPLICATE                                0x00000020

#define WINED3D_SWAPCHAIN_LOCKABLE_BACKBUFFER                   0x00000001u
#define WINED3D_SWAPCHAIN_DISCARD_DEPTHSTENCIL                  0x00000002u
#define WINED3D_SWAPCHAIN_DEVICECLIP                            0x00000004u
#define WINED3D_SWAPCHAIN_VIDEO                                 0x00000010u
#define WINED3D_SWAPCHAIN_NOAUTOROTATE                          0x00000020u
#define WINED3D_SWAPCHAIN_UNPRUNEDMODE                          0x00000040u
#define WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH                     0x00001000u
#define WINED3D_SWAPCHAIN_USE_CLOSEST_MATCHING_MODE             0x00002000u
#define WINED3D_SWAPCHAIN_RESTORE_WINDOW_RECT                   0x00004000u
#define WINED3D_SWAPCHAIN_GDI_COMPATIBLE                        0x00008000u
#define WINED3D_SWAPCHAIN_IMPLICIT                              0x00010000u
#define WINED3D_SWAPCHAIN_HOOK                                  0x00020000u

#define WINED3DDP_MAXTEXCOORD                                   8

#define WINED3D_BIND_VERTEX_BUFFER                              0x00000001
#define WINED3D_BIND_INDEX_BUFFER                               0x00000002
#define WINED3D_BIND_CONSTANT_BUFFER                            0x00000004
#define WINED3D_BIND_SHADER_RESOURCE                            0x00000008
#define WINED3D_BIND_STREAM_OUTPUT                              0x00000010
#define WINED3D_BIND_RENDER_TARGET                              0x00000020
#define WINED3D_BIND_DEPTH_STENCIL                              0x00000040
#define WINED3D_BIND_UNORDERED_ACCESS                           0x00000080

#define WINED3DUSAGE_SOFTWAREPROCESSING                         0x00000010
#define WINED3DUSAGE_DONOTCLIP                                  0x00000020
#define WINED3DUSAGE_POINTS                                     0x00000040
#define WINED3DUSAGE_RTPATCHES                                  0x00000080
#define WINED3DUSAGE_NPATCHES                                   0x00000100
#define WINED3DUSAGE_DYNAMIC                                    0x00000200
#define WINED3DUSAGE_RESTRICTED_CONTENT                         0x00000800
#define WINED3DUSAGE_RESTRICT_SHARED_RESOURCE_DRIVER            0x00001000
#define WINED3DUSAGE_RESTRICT_SHARED_RESOURCE                   0x00002000
#define WINED3DUSAGE_DMAP                                       0x00004000
#define WINED3DUSAGE_TEXTAPI                                    0x10000000
#define WINED3DUSAGE_MASK                                       0x10007bf0

#define WINED3DUSAGE_SCRATCH                                    0x00400000
#define WINED3DUSAGE_PRIVATE                                    0x00800000
#define WINED3DUSAGE_LEGACY_CUBEMAP                             0x01000000
#define WINED3DUSAGE_OWNDC                                      0x02000000
#define WINED3DUSAGE_STATICDECL                                 0x04000000
#define WINED3DUSAGE_OVERLAY                                    0x08000000

#define WINED3DUSAGE_QUERY_GENMIPMAP                            0x00000400
#define WINED3DUSAGE_QUERY_LEGACYBUMPMAP                        0x00008000
#define WINED3DUSAGE_QUERY_FILTER                               0x00020000
#define WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING             0x00080000
#define WINED3DUSAGE_QUERY_SRGBREAD                             0x00010000
#define WINED3DUSAGE_QUERY_SRGBWRITE                            0x00040000
#define WINED3DUSAGE_QUERY_VERTEXTEXTURE                        0x00100000
#define WINED3DUSAGE_QUERY_WRAPANDMIP                           0x00200000
#define WINED3DUSAGE_QUERY_MASK                                 0x003f8400

#define WINED3D_MAP_NOSYSLOCK                                   0x00000800
#define WINED3D_MAP_NOOVERWRITE                                 0x00001000
#define WINED3D_MAP_DISCARD                                     0x00002000
#define WINED3D_MAP_DONOTWAIT                                   0x00004000
#define WINED3D_MAP_NO_DIRTY_UPDATE                             0x00008000
#define WINED3D_MAP_WRITE                                       0x40000000
#define WINED3D_MAP_READ                                        0x80000000

#define WINED3DPRESENT_RATE_DEFAULT                             0x00000000

#define WINED3DCLIPPLANE0                                       (1u << 0)
#define WINED3DCLIPPLANE1                                       (1u << 1)
#define WINED3DCLIPPLANE2                                       (1u << 2)
#define WINED3DCLIPPLANE3                                       (1u << 3)
#define WINED3DCLIPPLANE4                                       (1u << 4)
#define WINED3DCLIPPLANE5                                       (1u << 5)

/* FVF (Flexible Vertex Format) codes */
#define WINED3DFVF_RESERVED0                                    0x0001
#define WINED3DFVF_POSITION_MASK                                0x400e
#define WINED3DFVF_XYZ                                          0x0002
#define WINED3DFVF_XYZRHW                                       0x0004
#define WINED3DFVF_XYZB1                                        0x0006
#define WINED3DFVF_XYZB2                                        0x0008
#define WINED3DFVF_XYZB3                                        0x000a
#define WINED3DFVF_XYZB4                                        0x000c
#define WINED3DFVF_XYZB5                                        0x000e
#define WINED3DFVF_XYZW                                         0x4002
#define WINED3DFVF_NORMAL                                       0x0010
#define WINED3DFVF_PSIZE                                        0x0020
#define WINED3DFVF_DIFFUSE                                      0x0040
#define WINED3DFVF_SPECULAR                                     0x0080
#define WINED3DFVF_TEXCOUNT_MASK                                0x0f00
#define WINED3DFVF_TEXCOUNT_SHIFT                               8
#define WINED3DFVF_TEX0                                         0x0000
#define WINED3DFVF_TEX1                                         0x0100
#define WINED3DFVF_TEX2                                         0x0200
#define WINED3DFVF_TEX3                                         0x0300
#define WINED3DFVF_TEX4                                         0x0400
#define WINED3DFVF_TEX5                                         0x0500
#define WINED3DFVF_TEX6                                         0x0600
#define WINED3DFVF_TEX7                                         0x0700
#define WINED3DFVF_TEX8                                         0x0800
#define WINED3DFVF_LASTBETA_UBYTE4                              0x1000
#define WINED3DFVF_LASTBETA_D3DCOLOR                            0x8000
#define WINED3DFVF_RESERVED2                                    0x6000

#define WINED3DFVF_TEXTUREFORMAT1                               3u
#define WINED3DFVF_TEXTUREFORMAT2                               0u
#define WINED3DFVF_TEXTUREFORMAT3                               1u
#define WINED3DFVF_TEXTUREFORMAT4                               2u
#define WINED3DFVF_TEXCOORDSIZE1(idx)                           (WINED3DFVF_TEXTUREFORMAT1 << (idx * 2 + 16))
#define WINED3DFVF_TEXCOORDSIZE2(idx)                           (WINED3DFVF_TEXTUREFORMAT2 << (idx * 2 + 16))
#define WINED3DFVF_TEXCOORDSIZE3(idx)                           (WINED3DFVF_TEXTUREFORMAT3 << (idx * 2 + 16))
#define WINED3DFVF_TEXCOORDSIZE4(idx)                           (WINED3DFVF_TEXTUREFORMAT4 << (idx * 2 + 16))

/* Clear flags */
#define WINED3DCLEAR_TARGET                                     0x00000001
#define WINED3DCLEAR_ZBUFFER                                    0x00000002
#define WINED3DCLEAR_STENCIL                                    0x00000004
#define WINED3DCLEAR_SYNCHRONOUS                                0x80000000

/* Stream source flags */
#define WINED3DSTREAMSOURCE_INDEXEDDATA                         (1u << 30)
#define WINED3DSTREAMSOURCE_INSTANCEDATA                        (2u << 30)

/* SetPrivateData flags */
#define WINED3DSPD_IUNKNOWN                                     0x00000001

/* IWineD3D::CreateDevice behaviour flags */
#define WINED3DCREATE_FPU_PRESERVE                              0x00000002
#define WINED3DCREATE_PUREDEVICE                                0x00000010
#define WINED3DCREATE_SOFTWARE_VERTEXPROCESSING                 0x00000020
#define WINED3DCREATE_HARDWARE_VERTEXPROCESSING                 0x00000040
#define WINED3DCREATE_MIXED_VERTEXPROCESSING                    0x00000080
#define WINED3DCREATE_DISABLE_DRIVER_MANAGEMENT                 0x00000100
#define WINED3DCREATE_ADAPTERGROUP_DEVICE                       0x00000200
#define WINED3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX              0x00000400
#define WINED3DCREATE_NOWINDOWCHANGES                           0x00000800
#define WINED3DCREATE_DISABLE_PSGP_THREADING                    0x00002000
#define WINED3DCREATE_ENABLE_PRESENTSTATS                       0x00004000
#define WINED3DCREATE_DISABLE_PRINTSCREEN                       0x00008000
#define WINED3DCREATE_SCREENSAVER                               0x10000000

/* VTF defines */
#define WINED3DDMAPSAMPLER                                      0x100
#define WINED3DVERTEXTEXTURESAMPLER0                            (WINED3DDMAPSAMPLER + 1)
#define WINED3DVERTEXTEXTURESAMPLER1                            (WINED3DDMAPSAMPLER + 2)
#define WINED3DVERTEXTEXTURESAMPLER2                            (WINED3DDMAPSAMPLER + 3)
#define WINED3DVERTEXTEXTURESAMPLER3                            (WINED3DDMAPSAMPLER + 4)

#define WINED3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD           0x00000020
#define WINED3DCAPS3_LINEAR_TO_SRGB_PRESENTATION                0x00000080
#define WINED3DCAPS3_COPY_TO_VIDMEM                             0x00000100
#define WINED3DCAPS3_COPY_TO_SYSTEMMEM                          0x00000200
#define WINED3DCAPS3_RESERVED                                   0x8000001f

#define WINED3DDEVCAPS2_STREAMOFFSET                            0x00000001
#define WINED3DDEVCAPS2_DMAPNPATCH                              0x00000002
#define WINED3DDEVCAPS2_ADAPTIVETESSRTPATCH                     0x00000004
#define WINED3DDEVCAPS2_ADAPTIVETESSNPATCH                      0x00000008
#define WINED3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES           0x00000010
#define WINED3DDEVCAPS2_PRESAMPLEDDMAPNPATCH                    0x00000020
#define WINED3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET      0x00000040

#define WINED3DDTCAPS_UBYTE4                                    0x00000001
#define WINED3DDTCAPS_UBYTE4N                                   0x00000002
#define WINED3DDTCAPS_SHORT2N                                   0x00000004
#define WINED3DDTCAPS_SHORT4N                                   0x00000008
#define WINED3DDTCAPS_USHORT2N                                  0x00000010
#define WINED3DDTCAPS_USHORT4N                                  0x00000020
#define WINED3DDTCAPS_UDEC3                                     0x00000040
#define WINED3DDTCAPS_DEC3N                                     0x00000080
#define WINED3DDTCAPS_FLOAT16_2                                 0x00000100
#define WINED3DDTCAPS_FLOAT16_4                                 0x00000200

#define WINED3DFVFCAPS_TEXCOORDCOUNTMASK                        0x0000ffff
#define WINED3DFVFCAPS_DONOTSTRIPELEMENTS                       0x00080000
#define WINED3DFVFCAPS_PSIZE                                    0x00100000

#define WINED3DLINECAPS_TEXTURE                                 0x00000001
#define WINED3DLINECAPS_ZTEST                                   0x00000002
#define WINED3DLINECAPS_BLEND                                   0x00000004
#define WINED3DLINECAPS_ALPHACMP                                0x00000008
#define WINED3DLINECAPS_FOG                                     0x00000010
#define WINED3DLINECAPS_ANTIALIAS                               0x00000020

#define WINED3DMAX30SHADERINSTRUCTIONS                          32768
#define WINED3DMIN30SHADERINSTRUCTIONS                          512

#define WINED3DPBLENDCAPS_ZERO                                  0x00000001
#define WINED3DPBLENDCAPS_ONE                                   0x00000002
#define WINED3DPBLENDCAPS_SRCCOLOR                              0x00000004
#define WINED3DPBLENDCAPS_INVSRCCOLOR                           0x00000008
#define WINED3DPBLENDCAPS_SRCALPHA                              0x00000010
#define WINED3DPBLENDCAPS_INVSRCALPHA                           0x00000020
#define WINED3DPBLENDCAPS_DESTALPHA                             0x00000040
#define WINED3DPBLENDCAPS_INVDESTALPHA                          0x00000080
#define WINED3DPBLENDCAPS_DESTCOLOR                             0x00000100
#define WINED3DPBLENDCAPS_INVDESTCOLOR                          0x00000200
#define WINED3DPBLENDCAPS_SRCALPHASAT                           0x00000400
#define WINED3DPBLENDCAPS_BOTHSRCALPHA                          0x00000800
#define WINED3DPBLENDCAPS_BOTHINVSRCALPHA                       0x00001000
#define WINED3DPBLENDCAPS_BLENDFACTOR                           0x00002000

#define WINED3DPCMPCAPS_NEVER                                   0x00000001
#define WINED3DPCMPCAPS_LESS                                    0x00000002
#define WINED3DPCMPCAPS_EQUAL                                   0x00000004
#define WINED3DPCMPCAPS_LESSEQUAL                               0x00000008
#define WINED3DPCMPCAPS_GREATER                                 0x00000010
#define WINED3DPCMPCAPS_NOTEQUAL                                0x00000020
#define WINED3DPCMPCAPS_GREATEREQUAL                            0x00000040
#define WINED3DPCMPCAPS_ALWAYS                                  0x00000080

#define WINED3DPMISCCAPS_MASKZ                                  0x00000002
#define WINED3DPMISCCAPS_LINEPATTERNREP                         0x00000004
#define WINED3DPMISCCAPS_CULLNONE                               0x00000010
#define WINED3DPMISCCAPS_CULLCW                                 0x00000020
#define WINED3DPMISCCAPS_CULLCCW                                0x00000040
#define WINED3DPMISCCAPS_COLORWRITEENABLE                       0x00000080
#define WINED3DPMISCCAPS_CLIPPLANESCALEDPOINTS                  0x00000100
#define WINED3DPMISCCAPS_CLIPTLVERTS                            0x00000200
#define WINED3DPMISCCAPS_TSSARGTEMP                             0x00000400
#define WINED3DPMISCCAPS_BLENDOP                                0x00000800
#define WINED3DPMISCCAPS_NULLREFERENCE                          0x00001000
#define WINED3DPMISCCAPS_INDEPENDENTWRITEMASKS                  0x00004000
#define WINED3DPMISCCAPS_PERSTAGECONSTANT                       0x00008000
#define WINED3DPMISCCAPS_FOGANDSPECULARALPHA                    0x00010000
#define WINED3DPMISCCAPS_SEPARATEALPHABLEND                     0x00020000
#define WINED3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS                0x00040000
#define WINED3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING             0x00080000
#define WINED3DPMISCCAPS_FOGVERTEXCLAMPED                       0x00100000
#define WINED3DPMISCCAPS_POSTBLENDSRGBCONVERT                   0x00200000

#define WINED3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH                 24
#define WINED3DPS20_MIN_DYNAMICFLOWCONTROLDEPTH                 0
#define WINED3DPS20_MAX_NUMTEMPS                                32
#define WINED3DPS20_MIN_NUMTEMPS                                12
#define WINED3DPS20_MAX_STATICFLOWCONTROLDEPTH                  4
#define WINED3DPS20_MIN_STATICFLOWCONTROLDEPTH                  0
#define WINED3DPS20_MAX_NUMINSTRUCTIONSLOTS                     512
#define WINED3DPS20_MIN_NUMINSTRUCTIONSLOTS                     96

#define WINED3DPS20CAPS_ARBITRARYSWIZZLE                        0x00000001
#define WINED3DPS20CAPS_GRADIENTINSTRUCTIONS                    0x00000002
#define WINED3DPS20CAPS_PREDICATION                             0x00000004
#define WINED3DPS20CAPS_NODEPENDENTREADLIMIT                    0x00000008
#define WINED3DPS20CAPS_NOTEXINSTRUCTIONLIMIT                   0x00000010

#define WINED3DPTADDRESSCAPS_WRAP                               0x00000001
#define WINED3DPTADDRESSCAPS_MIRROR                             0x00000002
#define WINED3DPTADDRESSCAPS_CLAMP                              0x00000004
#define WINED3DPTADDRESSCAPS_BORDER                             0x00000008
#define WINED3DPTADDRESSCAPS_INDEPENDENTUV                      0x00000010
#define WINED3DPTADDRESSCAPS_MIRRORONCE                         0x00000020

#define WINED3DSTENCILCAPS_KEEP                                 0x00000001
#define WINED3DSTENCILCAPS_ZERO                                 0x00000002
#define WINED3DSTENCILCAPS_REPLACE                              0x00000004
#define WINED3DSTENCILCAPS_INCRSAT                              0x00000008
#define WINED3DSTENCILCAPS_DECRSAT                              0x00000010
#define WINED3DSTENCILCAPS_INVERT                               0x00000020
#define WINED3DSTENCILCAPS_INCR                                 0x00000040
#define WINED3DSTENCILCAPS_DECR                                 0x00000080
#define WINED3DSTENCILCAPS_TWOSIDED                             0x00000100

#define WINED3DTEXOPCAPS_DISABLE                                0x00000001
#define WINED3DTEXOPCAPS_SELECTARG1                             0x00000002
#define WINED3DTEXOPCAPS_SELECTARG2                             0x00000004
#define WINED3DTEXOPCAPS_MODULATE                               0x00000008
#define WINED3DTEXOPCAPS_MODULATE2X                             0x00000010
#define WINED3DTEXOPCAPS_MODULATE4X                             0x00000020
#define WINED3DTEXOPCAPS_ADD                                    0x00000040
#define WINED3DTEXOPCAPS_ADDSIGNED                              0x00000080
#define WINED3DTEXOPCAPS_ADDSIGNED2X                            0x00000100
#define WINED3DTEXOPCAPS_SUBTRACT                               0x00000200
#define WINED3DTEXOPCAPS_ADDSMOOTH                              0x00000400
#define WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA                      0x00000800
#define WINED3DTEXOPCAPS_BLENDTEXTUREALPHA                      0x00001000
#define WINED3DTEXOPCAPS_BLENDFACTORALPHA                       0x00002000
#define WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM                    0x00004000
#define WINED3DTEXOPCAPS_BLENDCURRENTALPHA                      0x00008000
#define WINED3DTEXOPCAPS_PREMODULATE                            0x00010000
#define WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR                 0x00020000
#define WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA                 0x00040000
#define WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR              0x00080000
#define WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA              0x00100000
#define WINED3DTEXOPCAPS_BUMPENVMAP                             0x00200000
#define WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE                    0x00400000
#define WINED3DTEXOPCAPS_DOTPRODUCT3                            0x00800000
#define WINED3DTEXOPCAPS_MULTIPLYADD                            0x01000000
#define WINED3DTEXOPCAPS_LERP                                   0x02000000

#define WINED3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH                 24
#define WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH                 0
#define WINED3DVS20_MAX_NUMTEMPS                                32
#define WINED3DVS20_MIN_NUMTEMPS                                12
#define WINED3DVS20_MAX_STATICFLOWCONTROLDEPTH                  4
#define WINED3DVS20_MIN_STATICFLOWCONTROLDEPTH                  1

#define WINED3DVS20CAPS_PREDICATION                             0x00000001

#define WINED3DCAPS2_NO2DDURING3DSCENE                          0x00000002
#define WINED3DCAPS2_FULLSCREENGAMMA                            0x00020000
#define WINED3DCAPS2_CANRENDERWINDOWED                          0x00080000
#define WINED3DCAPS2_CANCALIBRATEGAMMA                          0x00100000
#define WINED3DCAPS2_RESERVED                                   0x02000000
#define WINED3DCAPS2_CANMANAGERESOURCE                          0x10000000
#define WINED3DCAPS2_DYNAMICTEXTURES                            0x20000000
#define WINED3DCAPS2_CANGENMIPMAP                               0x40000000

#define WINED3DPRASTERCAPS_DITHER                               0x00000001
#define WINED3DPRASTERCAPS_ROP2                                 0x00000002
#define WINED3DPRASTERCAPS_XOR                                  0x00000004
#define WINED3DPRASTERCAPS_PAT                                  0x00000008
#define WINED3DPRASTERCAPS_ZTEST                                0x00000010
#define WINED3DPRASTERCAPS_SUBPIXEL                             0x00000020
#define WINED3DPRASTERCAPS_SUBPIXELX                            0x00000040
#define WINED3DPRASTERCAPS_FOGVERTEX                            0x00000080
#define WINED3DPRASTERCAPS_FOGTABLE                             0x00000100
#define WINED3DPRASTERCAPS_STIPPLE                              0x00000200
#define WINED3DPRASTERCAPS_ANTIALIASSORTDEPENDENT               0x00000400
#define WINED3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT             0x00000800
#define WINED3DPRASTERCAPS_ANTIALIASEDGES                       0x00001000
#define WINED3DPRASTERCAPS_MIPMAPLODBIAS                        0x00002000
#define WINED3DPRASTERCAPS_ZBUFFERLESSHSR                       0x00008000
#define WINED3DPRASTERCAPS_FOGRANGE                             0x00010000
#define WINED3DPRASTERCAPS_ANISOTROPY                           0x00020000
#define WINED3DPRASTERCAPS_WBUFFER                              0x00040000
#define WINED3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT           0x00080000
#define WINED3DPRASTERCAPS_WFOG                                 0x00100000
#define WINED3DPRASTERCAPS_ZFOG                                 0x00200000
#define WINED3DPRASTERCAPS_COLORPERSPECTIVE                     0x00400000
#define WINED3DPRASTERCAPS_SCISSORTEST                          0x01000000
#define WINED3DPRASTERCAPS_SLOPESCALEDEPTHBIAS                  0x02000000
#define WINED3DPRASTERCAPS_DEPTHBIAS                            0x04000000
#define WINED3DPRASTERCAPS_MULTISAMPLE_TOGGLE                   0x08000000

#define WINED3DPSHADECAPS_COLORFLATMONO                         0x00000001
#define WINED3DPSHADECAPS_COLORFLATRGB                          0x00000002
#define WINED3DPSHADECAPS_COLORGOURAUDMONO                      0x00000004
#define WINED3DPSHADECAPS_COLORGOURAUDRGB                       0x00000008
#define WINED3DPSHADECAPS_COLORPHONGMONO                        0x00000010
#define WINED3DPSHADECAPS_COLORPHONGRGB                         0x00000020
#define WINED3DPSHADECAPS_SPECULARFLATMONO                      0x00000040
#define WINED3DPSHADECAPS_SPECULARFLATRGB                       0x00000080
#define WINED3DPSHADECAPS_SPECULARGOURAUDMONO                   0x00000100
#define WINED3DPSHADECAPS_SPECULARGOURAUDRGB                    0x00000200
#define WINED3DPSHADECAPS_SPECULARPHONGMONO                     0x00000400
#define WINED3DPSHADECAPS_SPECULARPHONGRGB                      0x00000800
#define WINED3DPSHADECAPS_ALPHAFLATBLEND                        0x00001000
#define WINED3DPSHADECAPS_ALPHAFLATSTIPPLED                     0x00002000
#define WINED3DPSHADECAPS_ALPHAGOURAUDBLEND                     0x00004000
#define WINED3DPSHADECAPS_ALPHAGOURAUDSTIPPLED                  0x00008000
#define WINED3DPSHADECAPS_ALPHAPHONGBLEND                       0x00010000
#define WINED3DPSHADECAPS_ALPHAPHONGSTIPPLED                    0x00020000
#define WINED3DPSHADECAPS_FOGFLAT                               0x00040000
#define WINED3DPSHADECAPS_FOGGOURAUD                            0x00080000
#define WINED3DPSHADECAPS_FOGPHONG                              0x00100000

#define WINED3DPTEXTURECAPS_PERSPECTIVE                         0x00000001
#define WINED3DPTEXTURECAPS_POW2                                0x00000002
#define WINED3DPTEXTURECAPS_ALPHA                               0x00000004
#define WINED3DPTEXTURECAPS_TRANSPARENCY                        0x00000008
#define WINED3DPTEXTURECAPS_BORDER                              0x00000010
#define WINED3DPTEXTURECAPS_SQUAREONLY                          0x00000020
#define WINED3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE            0x00000040
#define WINED3DPTEXTURECAPS_ALPHAPALETTE                        0x00000080
#define WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL                  0x00000100
#define WINED3DPTEXTURECAPS_PROJECTED                           0x00000400
#define WINED3DPTEXTURECAPS_CUBEMAP                             0x00000800
#define WINED3DPTEXTURECAPS_COLORKEYBLEND                       0x00001000
#define WINED3DPTEXTURECAPS_VOLUMEMAP                           0x00002000
#define WINED3DPTEXTURECAPS_MIPMAP                              0x00004000
#define WINED3DPTEXTURECAPS_MIPVOLUMEMAP                        0x00008000
#define WINED3DPTEXTURECAPS_MIPCUBEMAP                          0x00010000
#define WINED3DPTEXTURECAPS_CUBEMAP_POW2                        0x00020000
#define WINED3DPTEXTURECAPS_VOLUMEMAP_POW2                      0x00040000
#define WINED3DPTEXTURECAPS_NOPROJECTEDBUMPENV                  0x00200000

#define WINED3DPTFILTERCAPS_NEAREST                             0x00000001
#define WINED3DPTFILTERCAPS_LINEAR                              0x00000002
#define WINED3DPTFILTERCAPS_MIPNEAREST                          0x00000004
#define WINED3DPTFILTERCAPS_MIPLINEAR                           0x00000008
#define WINED3DPTFILTERCAPS_LINEARMIPNEAREST                    0x00000010
#define WINED3DPTFILTERCAPS_LINEARMIPLINEAR                     0x00000020
#define WINED3DPTFILTERCAPS_MINFPOINT                           0x00000100
#define WINED3DPTFILTERCAPS_MINFLINEAR                          0x00000200
#define WINED3DPTFILTERCAPS_MINFANISOTROPIC                     0x00000400
#define WINED3DPTFILTERCAPS_MIPFPOINT                           0x00010000
#define WINED3DPTFILTERCAPS_MIPFLINEAR                          0x00020000
#define WINED3DPTFILTERCAPS_MAGFPOINT                           0x01000000
#define WINED3DPTFILTERCAPS_MAGFLINEAR                          0x02000000
#define WINED3DPTFILTERCAPS_MAGFANISOTROPIC                     0x04000000
#define WINED3DPTFILTERCAPS_MAGFPYRAMIDALQUAD                   0x08000000
#define WINED3DPTFILTERCAPS_MAGFGAUSSIANQUAD                    0x10000000

#define WINED3DVTXPCAPS_TEXGEN                                  0x00000001
#define WINED3DVTXPCAPS_MATERIALSOURCE7                         0x00000002
#define WINED3DVTXPCAPS_VERTEXFOG                               0x00000004
#define WINED3DVTXPCAPS_DIRECTIONALLIGHTS                       0x00000008
#define WINED3DVTXPCAPS_POSITIONALLIGHTS                        0x00000010
#define WINED3DVTXPCAPS_LOCALVIEWER                             0x00000020
#define WINED3DVTXPCAPS_TWEENING                                0x00000040
#define WINED3DVTXPCAPS_TEXGEN_SPHEREMAP                        0x00000100
#define WINED3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER                0x00000200

#define WINED3DCURSORCAPS_COLOR                                 0x00000001
#define WINED3DCURSORCAPS_LOWRES                                0x00000002

#define WINED3DDEVCAPS_FLOATTLVERTEX                            0x00000001
#define WINED3DDEVCAPS_SORTINCREASINGZ                          0x00000002
#define WINED3DDEVCAPS_SORTDECREASINGZ                          0X00000004
#define WINED3DDEVCAPS_SORTEXACT                                0x00000008
#define WINED3DDEVCAPS_EXECUTESYSTEMMEMORY                      0x00000010
#define WINED3DDEVCAPS_EXECUTEVIDEOMEMORY                       0x00000020
#define WINED3DDEVCAPS_TLVERTEXSYSTEMMEMORY                     0x00000040
#define WINED3DDEVCAPS_TLVERTEXVIDEOMEMORY                      0x00000080
#define WINED3DDEVCAPS_TEXTURESYSTEMMEMORY                      0x00000100
#define WINED3DDEVCAPS_TEXTUREVIDEOMEMORY                       0x00000200
#define WINED3DDEVCAPS_DRAWPRIMTLVERTEX                         0x00000400
#define WINED3DDEVCAPS_CANRENDERAFTERFLIP                       0x00000800
#define WINED3DDEVCAPS_TEXTURENONLOCALVIDMEM                    0x00001000
#define WINED3DDEVCAPS_DRAWPRIMITIVES2                          0x00002000
#define WINED3DDEVCAPS_SEPARATETEXTUREMEMORIES                  0x00004000
#define WINED3DDEVCAPS_DRAWPRIMITIVES2EX                        0x00008000
#define WINED3DDEVCAPS_HWTRANSFORMANDLIGHT                      0x00010000
#define WINED3DDEVCAPS_CANBLTSYSTONONLOCAL                      0x00020000
#define WINED3DDEVCAPS_HWRASTERIZATION                          0x00080000
#define WINED3DDEVCAPS_PUREDEVICE                               0x00100000
#define WINED3DDEVCAPS_QUINTICRTPATCHES                         0x00200000
#define WINED3DDEVCAPS_RTPATCHES                                0x00400000
#define WINED3DDEVCAPS_RTPATCHHANDLEZERO                        0x00800000
#define WINED3DDEVCAPS_NPATCHES                                 0x01000000

#define WINED3D_LEGACY_DEPTH_BIAS                               0x00000001
#define WINED3D_NO3D                                            0x00000002
#define WINED3D_VIDMEM_ACCOUNTING                               0x00000004
#define WINED3D_PRESENT_CONVERSION                              0x00000008
#define WINED3D_RESTORE_MODE_ON_ACTIVATE                        0x00000010
#define WINED3D_FOCUS_MESSAGES                                  0x00000020
#define WINED3D_HANDLE_RESTORE                                  0x00000040
#define WINED3D_PIXEL_CENTER_INTEGER                            0x00000080
#define WINED3D_LEGACY_FFP_LIGHTING                             0x00000100
#define WINED3D_SRGB_READ_WRITE_CONTROL                         0x00000200
#define WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR                   0x00000400
#define WINED3D_NO_PRIMITIVE_RESTART                            0x00000800
#define WINED3D_LEGACY_CUBEMAP_FILTERING                        0x00001000
#define WINED3D_NORMALIZED_DEPTH_BIAS                           0x00002000
#define WINED3D_REQUEST_D3D10                                   0x00004000
#define WINED3D_LIMIT_VIEWPORT                                  0x00008000

#define WINED3D_RESZ_CODE                                       0x7fa05000

#define WINED3D_CKEY_DST_BLT                                    0x00000002
#define WINED3D_CKEY_DST_OVERLAY                                0x00000004
#define WINED3D_CKEY_SRC_BLT                                    0x00000008
#define WINED3D_CKEY_SRC_OVERLAY                                0x00000010

/* dwDDFX */
/* arithmetic stretching along y axis */
#define WINEDDBLTFX_ARITHSTRETCHY                               0x00000001
/* mirror on y axis */
#define WINEDDBLTFX_MIRRORLEFTRIGHT                             0x00000002
/* mirror on x axis */
#define WINEDDBLTFX_MIRRORUPDOWN                                0x00000004
/* do not tear */
#define WINEDDBLTFX_NOTEARING                                   0x00000008
/* 180 degrees clockwise rotation */
#define WINEDDBLTFX_ROTATE180                                   0x00000010
/* 270 degrees clockwise rotation */
#define WINEDDBLTFX_ROTATE270                                   0x00000020
/* 90 degrees clockwise rotation */
#define WINEDDBLTFX_ROTATE90                                    0x00000040
/* dwZBufferLow and dwZBufferHigh specify limits to the copied Z values */
#define WINEDDBLTFX_ZBUFFERRANGE                                0x00000080
/* add dwZBufferBaseDest to every source z value before compare */
#define WINEDDBLTFX_ZBUFFERBASEDEST                             0x00000100

#define WINED3D_BLT_FX                                          0x00000800
#define WINED3D_BLT_DST_CKEY                                    0x00002000
#define WINED3D_BLT_DST_CKEY_OVERRIDE                           0x00004000
#define WINED3D_BLT_SRC_CKEY                                    0x00008000
#define WINED3D_BLT_SRC_CKEY_OVERRIDE                           0x00010000
#define WINED3D_BLT_WAIT                                        0x01000000
#define WINED3D_BLT_DO_NOT_WAIT                                 0x08000000
#define WINED3D_BLT_RAW                                         0x20000000
#define WINED3D_BLT_SYNCHRONOUS                                 0x40000000
#define WINED3D_BLT_ALPHA_TEST                                  0x80000000
#define WINED3D_BLT_MASK                                        0x0901e800

/* dwFlags for GetBltStatus */
#define WINEDDGBS_CANBLT                                        0x00000001
#define WINEDDGBS_ISBLTDONE                                     0x00000002

/* dwFlags for GetFlipStatus */
#define WINEDDGFS_CANFLIP                                       0x00000001
#define WINEDDGFS_ISFLIPDONE                                    0x00000002

/* dwFlags for Flip */
#define WINEDDFLIP_WAIT                                         0x00000001
#define WINEDDFLIP_EVEN                                         0x00000002 /* only valid for overlay */
#define WINEDDFLIP_ODD                                          0x00000004 /* only valid for overlay */
#define WINEDDFLIP_NOVSYNC                                      0x00000008
#define WINEDDFLIP_STEREO                                       0x00000010
#define WINEDDFLIP_DONOTWAIT                                    0x00000020
#define WINEDDFLIP_INTERVAL2                                    0x02000000
#define WINEDDFLIP_INTERVAL3                                    0x03000000
#define WINEDDFLIP_INTERVAL4                                    0x04000000

#define WINEDDOVER_ALPHADEST                                    0x00000001
#define WINEDDOVER_ALPHADESTCONSTOVERRIDE                       0x00000002
#define WINEDDOVER_ALPHADESTNEG                                 0x00000004
#define WINEDDOVER_ALPHADESTSURFACEOVERRIDE                     0x00000008
#define WINEDDOVER_ALPHAEDGEBLEND                               0x00000010
#define WINEDDOVER_ALPHASRC                                     0x00000020
#define WINEDDOVER_ALPHASRCCONSTOVERRIDE                        0x00000040
#define WINEDDOVER_ALPHASRCNEG                                  0x00000080
#define WINEDDOVER_ALPHASRCSURFACEOVERRIDE                      0x00000100
#define WINEDDOVER_HIDE                                         0x00000200
#define WINEDDOVER_KEYDEST                                      0x00000400
#define WINEDDOVER_KEYDESTOVERRIDE                              0x00000800
#define WINEDDOVER_KEYSRC                                       0x00001000
#define WINEDDOVER_KEYSRCOVERRIDE                               0x00002000
#define WINEDDOVER_SHOW                                         0x00004000
#define WINEDDOVER_ADDDIRTYRECT                                 0x00008000
#define WINEDDOVER_REFRESHDIRTYRECTS                            0x00010000
#define WINEDDOVER_REFRESHALL                                   0x00020000
#define WINEDDOVER_DDFX                                         0x00080000
#define WINEDDOVER_AUTOFLIP                                     0x00100000
#define WINEDDOVER_BOB                                          0x00200000
#define WINEDDOVER_OVERRIDEBOBWEAVE                             0x00400000
#define WINEDDOVER_INTERLEAVED                                  0x00800000

/* DirectDraw Caps */
#define WINEDDSCAPS_RESERVED1                                   0x00000001
#define WINEDDSCAPS_ALPHA                                       0x00000002
#define WINEDDSCAPS_BACKBUFFER                                  0x00000004
#define WINEDDSCAPS_COMPLEX                                     0x00000008
#define WINEDDSCAPS_FLIP                                        0x00000010
#define WINEDDSCAPS_FRONTBUFFER                                 0x00000020
#define WINEDDSCAPS_OFFSCREENPLAIN                              0x00000040
#define WINEDDSCAPS_OVERLAY                                     0x00000080
#define WINEDDSCAPS_PALETTE                                     0x00000100
#define WINEDDSCAPS_PRIMARYSURFACE                              0x00000200
#define WINEDDSCAPS_PRIMARYSURFACELEFT                          0x00000400
#define WINEDDSCAPS_SYSTEMMEMORY                                0x00000800
#define WINEDDSCAPS_TEXTURE                                     0x00001000
#define WINEDDSCAPS_3DDEVICE                                    0x00002000
#define WINEDDSCAPS_VIDEOMEMORY                                 0x00004000
#define WINEDDSCAPS_VISIBLE                                     0x00008000
#define WINEDDSCAPS_WRITEONLY                                   0x00010000
#define WINEDDSCAPS_ZBUFFER                                     0x00020000
#define WINEDDSCAPS_OWNDC                                       0x00040000
#define WINEDDSCAPS_LIVEVIDEO                                   0x00080000
#define WINEDDSCAPS_HWCODEC                                     0x00100000
#define WINEDDSCAPS_MODEX                                       0x00200000
#define WINEDDSCAPS_MIPMAP                                      0x00400000
#define WINEDDSCAPS_RESERVED2                                   0x00800000
#define WINEDDSCAPS_ALLOCONLOAD                                 0x04000000
#define WINEDDSCAPS_VIDEOPORT                                   0x08000000
#define WINEDDSCAPS_LOCALVIDMEM                                 0x10000000
#define WINEDDSCAPS_NONLOCALVIDMEM                              0x20000000
#define WINEDDSCAPS_STANDARDVGAMODE                             0x40000000
#define WINEDDSCAPS_OPTIMIZED                                   0x80000000

#define WINEDDCKEYCAPS_DESTBLT                                  0x00000001
#define WINEDDCKEYCAPS_DESTBLTCLRSPACE                          0x00000002
#define WINEDDCKEYCAPS_DESTBLTCLRSPACEYUV                       0x00000004
#define WINEDDCKEYCAPS_DESTBLTYUV                               0x00000008
#define WINEDDCKEYCAPS_DESTOVERLAY                              0x00000010
#define WINEDDCKEYCAPS_DESTOVERLAYCLRSPACE                      0x00000020
#define WINEDDCKEYCAPS_DESTOVERLAYCLRSPACEYUV                   0x00000040
#define WINEDDCKEYCAPS_DESTOVERLAYONEACTIVE                     0x00000080
#define WINEDDCKEYCAPS_DESTOVERLAYYUV                           0x00000100
#define WINEDDCKEYCAPS_SRCBLT                                   0x00000200
#define WINEDDCKEYCAPS_SRCBLTCLRSPACE                           0x00000400
#define WINEDDCKEYCAPS_SRCBLTCLRSPACEYUV                        0x00000800
#define WINEDDCKEYCAPS_SRCBLTYUV                                0x00001000
#define WINEDDCKEYCAPS_SRCOVERLAY                               0x00002000
#define WINEDDCKEYCAPS_SRCOVERLAYCLRSPACE                       0x00004000
#define WINEDDCKEYCAPS_SRCOVERLAYCLRSPACEYUV                    0x00008000
#define WINEDDCKEYCAPS_SRCOVERLAYONEACTIVE                      0x00010000
#define WINEDDCKEYCAPS_SRCOVERLAYYUV                            0x00020000
#define WINEDDCKEYCAPS_NOCOSTOVERLAY                            0x00040000

#define WINEDDFXCAPS_BLTALPHA                                   0x00000001
#define WINEDDFXCAPS_OVERLAYALPHA                               0x00000004
#define WINEDDFXCAPS_BLTARITHSTRETCHYN                          0x00000010
#define WINEDDFXCAPS_BLTARITHSTRETCHY                           0x00000020
#define WINEDDFXCAPS_BLTMIRRORLEFTRIGHT                         0x00000040
#define WINEDDFXCAPS_BLTMIRRORUPDOWN                            0x00000080
#define WINEDDFXCAPS_BLTROTATION                                0x00000100
#define WINEDDFXCAPS_BLTROTATION90                              0x00000200
#define WINEDDFXCAPS_BLTSHRINKX                                 0x00000400
#define WINEDDFXCAPS_BLTSHRINKXN                                0x00000800
#define WINEDDFXCAPS_BLTSHRINKY                                 0x00001000
#define WINEDDFXCAPS_BLTSHRINKYN                                0x00002000
#define WINEDDFXCAPS_BLTSTRETCHX                                0x00004000
#define WINEDDFXCAPS_BLTSTRETCHXN                               0x00008000
#define WINEDDFXCAPS_BLTSTRETCHY                                0x00010000
#define WINEDDFXCAPS_BLTSTRETCHYN                               0x00020000
#define WINEDDFXCAPS_OVERLAYARITHSTRETCHY                       0x00040000
#define WINEDDFXCAPS_OVERLAYARITHSTRETCHYN                      0x00000008
#define WINEDDFXCAPS_OVERLAYSHRINKX                             0x00080000
#define WINEDDFXCAPS_OVERLAYSHRINKXN                            0x00100000
#define WINEDDFXCAPS_OVERLAYSHRINKY                             0x00200000
#define WINEDDFXCAPS_OVERLAYSHRINKYN                            0x00400000
#define WINEDDFXCAPS_OVERLAYSTRETCHX                            0x00800000
#define WINEDDFXCAPS_OVERLAYSTRETCHXN                           0x01000000
#define WINEDDFXCAPS_OVERLAYSTRETCHY                            0x02000000
#define WINEDDFXCAPS_OVERLAYSTRETCHYN                           0x04000000
#define WINEDDFXCAPS_OVERLAYMIRRORLEFTRIGHT                     0x08000000
#define WINEDDFXCAPS_OVERLAYMIRRORUPDOWN                        0x10000000

#define WINEDDCAPS_3D                                           0x00000001
#define WINEDDCAPS_ALIGNBOUNDARYDEST                            0x00000002
#define WINEDDCAPS_ALIGNSIZEDEST                                0x00000004
#define WINEDDCAPS_ALIGNBOUNDARYSRC                             0x00000008
#define WINEDDCAPS_ALIGNSIZESRC                                 0x00000010
#define WINEDDCAPS_ALIGNSTRIDE                                  0x00000020
#define WINEDDCAPS_BLT                                          0x00000040
#define WINEDDCAPS_BLTQUEUE                                     0x00000080
#define WINEDDCAPS_BLTFOURCC                                    0x00000100
#define WINEDDCAPS_BLTSTRETCH                                   0x00000200
#define WINEDDCAPS_GDI                                          0x00000400
#define WINEDDCAPS_OVERLAY                                      0x00000800
#define WINEDDCAPS_OVERLAYCANTCLIP                              0x00001000
#define WINEDDCAPS_OVERLAYFOURCC                                0x00002000
#define WINEDDCAPS_OVERLAYSTRETCH                               0x00004000
#define WINEDDCAPS_PALETTE                                      0x00008000
#define WINEDDCAPS_PALETTEVSYNC                                 0x00010000
#define WINEDDCAPS_READSCANLINE                                 0x00020000
#define WINEDDCAPS_STEREOVIEW                                   0x00040000
#define WINEDDCAPS_VBI                                          0x00080000
#define WINEDDCAPS_ZBLTS                                        0x00100000
#define WINEDDCAPS_ZOVERLAYS                                    0x00200000
#define WINEDDCAPS_COLORKEY                                     0x00400000
#define WINEDDCAPS_ALPHA                                        0x00800000
#define WINEDDCAPS_COLORKEYHWASSIST                             0x01000000
#define WINEDDCAPS_NOHARDWARE                                   0x02000000
#define WINEDDCAPS_BLTCOLORFILL                                 0x04000000
#define WINEDDCAPS_BANKSWITCHED                                 0x08000000
#define WINEDDCAPS_BLTDEPTHFILL                                 0x10000000
#define WINEDDCAPS_CANCLIP                                      0x20000000
#define WINEDDCAPS_CANCLIPSTRETCHED                             0x40000000
#define WINEDDCAPS_CANBLTSYSMEM                                 0x80000000

#define WINEDDCAPS2_CERTIFIED                                   0x00000001
#define WINEDDCAPS2_NO2DDURING3DSCENE                           0x00000002
#define WINEDDCAPS2_VIDEOPORT                                   0x00000004
#define WINEDDCAPS2_AUTOFLIPOVERLAY                             0x00000008
#define WINEDDCAPS2_CANBOBINTERLEAVED                           0x00000010
#define WINEDDCAPS2_CANBOBNONINTERLEAVED                        0x00000020
#define WINEDDCAPS2_COLORCONTROLOVERLAY                         0x00000040
#define WINEDDCAPS2_COLORCONTROLPRIMARY                         0x00000080
#define WINEDDCAPS2_CANDROPZ16BIT                               0x00000100
#define WINEDDCAPS2_NONLOCALVIDMEM                              0x00000200
#define WINEDDCAPS2_NONLOCALVIDMEMCAPS                          0x00000400
#define WINEDDCAPS2_NOPAGELOCKREQUIRED                          0x00000800
#define WINEDDCAPS2_WIDESURFACES                                0x00001000
#define WINEDDCAPS2_CANFLIPODDEVEN                              0x00002000
#define WINEDDCAPS2_CANBOBHARDWARE                              0x00004000
#define WINEDDCAPS2_COPYFOURCC                                  0x00008000
#define WINEDDCAPS2_PRIMARYGAMMA                                0x00020000
#define WINEDDCAPS2_CANRENDERWINDOWED                           0x00080000
#define WINEDDCAPS2_CANCALIBRATEGAMMA                           0x00100000
#define WINEDDCAPS2_FLIPINTERVAL                                0x00200000
#define WINEDDCAPS2_FLIPNOVSYNC                                 0x00400000
#define WINEDDCAPS2_CANMANAGETEXTURE                            0x00800000
#define WINEDDCAPS2_TEXMANINNONLOCALVIDMEM                      0x01000000
#define WINEDDCAPS2_STEREO                                      0x02000000
#define WINEDDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL                 0x04000000

#define WINED3D_PALETTE_8BIT_ENTRIES                            0x00000001
#define WINED3D_PALETTE_ALLOW_256                               0x00000002
#define WINED3D_PALETTE_ALPHA                                   0x00000004

#define WINED3D_TEXTURE_CREATE_DISCARD                          0x00000002
#define WINED3D_TEXTURE_CREATE_GET_DC_LENIENT                   0x00000004
#define WINED3D_TEXTURE_CREATE_GET_DC                           0x00000008
#define WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS                 0x00000010

#define WINED3D_STANDARD_MULTISAMPLE_PATTERN                    0xffffffff

#define WINED3D_APPEND_ALIGNED_ELEMENT                          0xffffffff

#define WINED3D_OUTPUT_SLOT_SEMANTIC                            0xffffffff
#define WINED3D_OUTPUT_SLOT_UNUSED                              0xfffffffe

#define WINED3D_MAX_STREAM_OUTPUT_BUFFERS                       4
#define WINED3D_NO_RASTERIZER_STREAM                            0xffffffff

#define WINED3D_VIEW_BUFFER_RAW                                 0x00000001
#define WINED3D_VIEW_BUFFER_APPEND                              0x00000002
#define WINED3D_VIEW_BUFFER_COUNTER                             0x00000004
#define WINED3D_VIEW_TEXTURE_CUBE                               0x00000008
#define WINED3D_VIEW_TEXTURE_ARRAY                              0x00000010

#define WINED3D_MAX_VIEWPORTS                                   16

#define WINED3D_REGISTER_WINDOW_NO_WINDOW_CHANGES               0x00000001u
#define WINED3D_REGISTER_WINDOW_NO_ALT_ENTER                    0x00000002u
#define WINED3D_REGISTER_WINDOW_NO_PRINT_SCREEN                 0x00000004u

struct wined3d_display_mode
{
    UINT width;
    UINT height;
    UINT refresh_rate;
    enum wined3d_format_id format_id;
    enum wined3d_scanline_ordering scanline_ordering;
};

struct wined3d_color
{
    float r;
    float g;
    float b;
    float a;
};

struct wined3d_vec3
{
    float x;
    float y;
    float z;
};

struct wined3d_vec4
{
    float x;
    float y;
    float z;
    float w;
};

struct wined3d_dvec4
{
    double x;
    double y;
    double z;
    double w;
};

struct wined3d_ivec4
{
    int x;
    int y;
    int z;
    int w;
};

struct wined3d_uvec4
{
    unsigned int x;
    unsigned int y;
    unsigned int z;
    unsigned int w;
};

struct wined3d_matrix
{
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
};

struct wined3d_light
{
    enum wined3d_light_type type;
    struct wined3d_color diffuse;
    struct wined3d_color specular;
    struct wined3d_color ambient;
    struct wined3d_vec3 position;
    struct wined3d_vec3 direction;
    float range;
    float falloff;
    float attenuation0;
    float attenuation1;
    float attenuation2;
    float theta;
    float phi;
};

struct wined3d_material
{
    struct wined3d_color diffuse;
    struct wined3d_color ambient;
    struct wined3d_color specular;
    struct wined3d_color emissive;
    float power;
};

struct wined3d_viewport
{
    float x;
    float y;
    float width;
    float height;
    float min_z;
    float max_z;
};

struct wined3d_gamma_ramp
{
    WORD red[256];
    WORD green[256];
    WORD blue[256];
};

struct wined3d_line_pattern
{
    WORD repeat_factor;
    WORD line_pattern;
};

struct wined3d_rect_patch_info
{
    UINT start_vertex_offset_width;
    UINT start_vertex_offset_height;
    UINT width;
    UINT height;
    UINT stride;
    enum wined3d_basis_type basis;
    enum wined3d_degree_type degree;
};

struct wined3d_tri_patch_info
{
    UINT start_vertex_offset;
    UINT vertex_count;
    enum wined3d_basis_type basis;
    enum wined3d_degree_type degree;
};

struct wined3d_adapter_identifier
{
    char *driver;
    unsigned int driver_size;
    char *description;
    unsigned int description_size;
    char *device_name;
    unsigned int device_name_size;
    LARGE_INTEGER driver_version;
    DWORD vendor_id;
    DWORD device_id;
    DWORD subsystem_id;
    DWORD revision;
    GUID device_identifier;
    GUID driver_uuid;
    GUID device_uuid;
    DWORD whql_level;
    LUID adapter_luid;
    SIZE_T video_memory;
    SIZE_T shared_system_memory;
};

struct wined3d_swapchain_desc
{
    unsigned int backbuffer_width;
    unsigned int backbuffer_height;
    enum wined3d_format_id backbuffer_format;
    unsigned int backbuffer_count;
    unsigned int backbuffer_bind_flags;
    enum wined3d_multisample_type multisample_type;
    DWORD multisample_quality;
    enum wined3d_swap_effect swap_effect;
    HWND device_window;
    BOOL windowed;
    BOOL enable_auto_depth_stencil;
    enum wined3d_format_id auto_depth_stencil_format;
    DWORD flags;
    unsigned int refresh_rate;
    BOOL auto_restore_display_mode;
};

struct wined3d_resource_desc
{
    enum wined3d_resource_type resource_type;
    enum wined3d_format_id format;
    enum wined3d_multisample_type multisample_type;
    unsigned int multisample_quality;
    unsigned int usage;
    unsigned int bind_flags;
    unsigned int access;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int size;
};

struct wined3d_sub_resource_desc
{
    enum wined3d_format_id format;
    enum wined3d_multisample_type multisample_type;
    unsigned int multisample_quality;
    unsigned int usage;
    unsigned int bind_flags;
    unsigned int access;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int size;
};

struct wined3d_clip_status
{
   DWORD clip_union;
   DWORD clip_intersection;
};

enum wined3d_input_classification
{
    WINED3D_INPUT_PER_VERTEX_DATA,
    WINED3D_INPUT_PER_INSTANCE_DATA,
};

struct wined3d_vertex_element
{
    enum wined3d_format_id format;
    unsigned int input_slot;
    unsigned int offset;
    unsigned int output_slot; /* D3D 8 & 10 */
    enum wined3d_input_classification input_slot_class;
    unsigned int instance_data_step_rate;
    BYTE method;
    BYTE usage;
    BYTE usage_idx;
};

struct wined3d_device_creation_parameters
{
    UINT adapter_idx;
    enum wined3d_device_type device_type;
    HWND focus_window;
    DWORD flags;
};

struct wined3d_raster_status
{
    BOOL in_vblank;
    UINT scan_line;
};

struct wined3d_map_desc
{
    UINT row_pitch;
    UINT slice_pitch;
    void *data;
};

struct wined3d_map_info
{
    UINT row_pitch;
    UINT slice_pitch;
    UINT size;
};

struct wined3d_sub_resource_data
{
    const void *data;
    unsigned int row_pitch;
    unsigned int slice_pitch;
};

struct wined3d_box
{
    UINT left;
    UINT top;
    UINT right;
    UINT bottom;
    UINT front;
    UINT back;
};

struct wined3d_vertex_shader_caps
{
    DWORD caps;
    INT dynamic_flow_control_depth;
    INT temp_count;
    INT static_flow_control_depth;
};

struct wined3d_pixel_shader_caps
{
    DWORD caps;
    INT dynamic_flow_control_depth;
    INT temp_count;
    INT static_flow_control_depth;
    INT instruction_slot_count;
};

struct wined3d_ddraw_caps
{
    DWORD caps;
    DWORD caps2;
    DWORD color_key_caps;
    DWORD fx_caps;
    DWORD fx_alpha_caps;
    DWORD sv_caps;
    DWORD svb_caps;
    DWORD svb_color_key_caps;
    DWORD svb_fx_caps;
    DWORD vsb_caps;
    DWORD vsb_color_key_caps;
    DWORD vsb_fx_caps;
    DWORD ssb_caps;
    DWORD ssb_color_key_caps;
    DWORD ssb_fx_caps;
    DWORD dds_caps;
};

struct wined3d_caps
{
    enum wined3d_device_type DeviceType;
    UINT AdapterOrdinal;

    DWORD Caps;
    DWORD Caps2;
    DWORD Caps3;

    DWORD CursorCaps;
    DWORD DevCaps;
    DWORD PrimitiveMiscCaps;
    DWORD RasterCaps;
    DWORD ZCmpCaps;
    DWORD SrcBlendCaps;
    DWORD DestBlendCaps;
    DWORD AlphaCmpCaps;
    DWORD ShadeCaps;
    DWORD TextureCaps;
    DWORD TextureFilterCaps;
    DWORD CubeTextureFilterCaps;
    DWORD VolumeTextureFilterCaps;
    DWORD TextureAddressCaps;
    DWORD VolumeTextureAddressCaps;
    DWORD LineCaps;

    DWORD MaxTextureWidth;
    DWORD MaxTextureHeight;
    DWORD MaxVolumeExtent;
    DWORD MaxTextureRepeat;
    DWORD MaxTextureAspectRatio;
    DWORD MaxAnisotropy;
    float MaxVertexW;

    float GuardBandLeft;
    float GuardBandTop;
    float GuardBandRight;
    float GuardBandBottom;

    float ExtentsAdjust;
    DWORD StencilCaps;

    DWORD FVFCaps;
    DWORD TextureOpCaps;
    DWORD MaxTextureBlendStages;
    DWORD MaxSimultaneousTextures;

    DWORD VertexProcessingCaps;
    DWORD MaxActiveLights;
    DWORD MaxUserClipPlanes;
    DWORD MaxVertexBlendMatrices;
    DWORD MaxVertexBlendMatrixIndex;

    float MaxPointSize;

    DWORD MaxPrimitiveCount;
    DWORD MaxVertexIndex;
    DWORD MaxStreams;
    DWORD MaxStreamStride;

    DWORD VertexShaderVersion;
    DWORD MaxVertexShaderConst;

    DWORD PixelShaderVersion;
    float PixelShader1xMaxValue;

    /* DX 9 */
    DWORD DevCaps2;

    float MaxNpatchTessellationLevel;

    UINT MasterAdapterOrdinal;
    UINT AdapterOrdinalInGroup;
    UINT NumberOfAdaptersInGroup;
    DWORD DeclTypes;
    DWORD NumSimultaneousRTs;
    DWORD StretchRectFilterCaps;
    struct wined3d_vertex_shader_caps VS20Caps;
    struct wined3d_pixel_shader_caps PS20Caps;
    DWORD VertexTextureFilterCaps;
    DWORD MaxVShaderInstructionsExecuted;
    DWORD MaxPShaderInstructionsExecuted;
    DWORD MaxVertexShader30InstructionSlots;
    DWORD MaxPixelShader30InstructionSlots;

    struct wined3d_ddraw_caps ddraw_caps;

    BOOL shader_double_precision;
    BOOL viewport_array_index_any_shader;

    enum wined3d_feature_level max_feature_level;
};

struct wined3d_color_key
{
    DWORD color_space_low_value;    /* low boundary of color space that is to
                                     * be treated as Color Key, inclusive */
    DWORD color_space_high_value;   /* high boundary of color space that is
                                     * to be treated as Color Key, inclusive */
};

struct wined3d_blt_fx
{
    DWORD fx;
    struct wined3d_color_key dst_color_key;
    struct wined3d_color_key src_color_key;
};

struct wined3d_buffer_desc
{
    unsigned int byte_width;
    unsigned int usage;
    unsigned int bind_flags;
    unsigned int access;
    unsigned int misc_flags;
    unsigned int structure_byte_stride;
};

struct wined3d_blend_state_desc
{
    BOOL alpha_to_coverage;
};

struct wined3d_rasterizer_state_desc
{
    BOOL front_ccw;
    float depth_bias_clamp;
    BOOL depth_clip;
};

struct wined3d_sampler_desc
{
    enum wined3d_texture_address address_u;
    enum wined3d_texture_address address_v;
    enum wined3d_texture_address address_w;
    float border_color[4];
    enum wined3d_texture_filter_type mag_filter;
    enum wined3d_texture_filter_type min_filter;
    enum wined3d_texture_filter_type mip_filter;
    float lod_bias;
    float min_lod;
    float max_lod;
    unsigned int mip_base_level;
    unsigned int max_anisotropy;
    BOOL compare;
    enum wined3d_cmp_func comparison_func;
    BOOL srgb_decode;
};

struct wined3d_shader_signature_element
{
    const char *semantic_name;
    unsigned int semantic_idx;
    unsigned int stream_idx;
    enum wined3d_sysval_semantic sysval_semantic;
    enum wined3d_component_type component_type;
    unsigned int register_idx;
    DWORD mask;
};

struct wined3d_shader_signature
{
    UINT element_count;
    struct wined3d_shader_signature_element *elements;
};

struct wined3d_shader_desc
{
    const DWORD *byte_code;
    size_t byte_code_size;
};

struct wined3d_stream_output_element
{
    unsigned int stream_idx;
    const char *semantic_name;
    unsigned int semantic_idx;
    BYTE component_idx;
    BYTE component_count;
    BYTE output_slot;
};

struct wined3d_stream_output_desc
{
    const struct wined3d_stream_output_element *elements;
    unsigned int element_count;
    unsigned int buffer_strides[WINED3D_MAX_STREAM_OUTPUT_BUFFERS];
    unsigned int buffer_stride_count;
    unsigned int rasterizer_stream_idx;
};

struct wined3d_view_desc
{
    enum wined3d_format_id format_id;
    unsigned int flags;
    union
    {
        struct
        {
            unsigned int start_idx;
            unsigned int count;
        } buffer;
        struct
        {
            unsigned int level_idx;
            unsigned int level_count;
            unsigned int layer_idx;
            unsigned int layer_count;
        } texture;
    } u;
};

struct wined3d_output_desc
{
    WCHAR device_name[CCHDEVICENAME];
    RECT desktop_rect;
    BOOL attached_to_desktop;
    enum wined3d_display_rotation rotation;
    HMONITOR monitor;
};

struct wined3d_parent_ops
{
    void (__stdcall *wined3d_object_destroyed)(void *parent);
};

struct wined3d;
struct wined3d_buffer;
struct wined3d_device;
struct wined3d_palette;
struct wined3d_query;
struct wined3d_blend_state;
struct wined3d_rasterizer_state;
struct wined3d_rendertarget_view;
struct wined3d_resource;
struct wined3d_sampler;
struct wined3d_shader;
struct wined3d_shader_resource_view;
struct wined3d_stateblock;
struct wined3d_swapchain;
struct wined3d_swapchain_state;
struct wined3d_texture;
struct wined3d_unordered_access_view;
struct wined3d_vertex_declaration;

struct wined3d_device_parent
{
    const struct wined3d_device_parent_ops *ops;
};

struct wined3d_device_parent_ops
{
    void (__cdecl *wined3d_device_created)(struct wined3d_device_parent *device_parent, struct wined3d_device *device);
    void (__cdecl *mode_changed)(struct wined3d_device_parent *device_parent);
    void (__cdecl *activate)(struct wined3d_device_parent *device_parent, BOOL activate);
    HRESULT (__cdecl *texture_sub_resource_created)(struct wined3d_device_parent *device_parent,
            enum wined3d_resource_type type, struct wined3d_texture *texture, unsigned int sub_resource_idx,
            void **parent, const struct wined3d_parent_ops **parent_ops);
    HRESULT (__cdecl *create_swapchain_texture)(struct wined3d_device_parent *device_parent, void *parent,
            const struct wined3d_resource_desc *desc, DWORD texture_flags, struct wined3d_texture **texture);
};

struct wined3d_private_store
{
    struct list content;
};

struct wined3d_private_data
{
    struct list entry;

    GUID tag;
    DWORD flags; /* DDSPD_* */
    DWORD size;
    union
    {
        BYTE data[1];
        IUnknown *object;
    } content;
};

typedef HRESULT (CDECL *wined3d_device_reset_cb)(struct wined3d_resource *resource);

void __stdcall wined3d_mutex_lock(void);
void __stdcall wined3d_mutex_unlock(void);

UINT __cdecl wined3d_calculate_format_pitch(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, UINT width);
HRESULT __cdecl wined3d_check_depth_stencil_match(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id adapter_format_id,
        enum wined3d_format_id render_target_format_id, enum wined3d_format_id depth_stencil_format_id);
HRESULT __cdecl wined3d_check_device_format(const struct wined3d *wined3d, UINT adaper_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id adapter_format_id, DWORD usage,
        unsigned int bind_flags, enum wined3d_resource_type resource_type, enum wined3d_format_id check_format_id);
HRESULT __cdecl wined3d_check_device_format_conversion(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id source_format_id,
        enum wined3d_format_id target_format_id);
HRESULT __cdecl wined3d_check_device_multisample_type(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id surface_format_id, BOOL windowed,
        enum wined3d_multisample_type multisample_type, DWORD *quality_levels);
HRESULT __cdecl wined3d_check_device_type(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id display_format_id,
        enum wined3d_format_id backbuffer_format_id, BOOL windowed);
struct wined3d * __cdecl wined3d_create(DWORD flags);
ULONG __cdecl wined3d_decref(struct wined3d *wined3d);
HRESULT __cdecl wined3d_enum_adapter_modes(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, enum wined3d_scanline_ordering scanline_ordering,
        UINT mode_idx, struct wined3d_display_mode *mode);
HRESULT __cdecl wined3d_find_closest_matching_adapter_mode(const struct wined3d *wined3d,
        unsigned int adapter_idx, struct wined3d_display_mode *mode);
UINT __cdecl wined3d_get_adapter_count(const struct wined3d *wined3d);
HRESULT __cdecl wined3d_get_adapter_display_mode(const struct wined3d *wined3d, UINT adapter_idx,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation);
HRESULT __cdecl wined3d_get_adapter_identifier(const struct wined3d *wined3d, UINT adapter_idx,
        DWORD flags, struct wined3d_adapter_identifier *identifier);
UINT __cdecl wined3d_get_adapter_mode_count(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, enum wined3d_scanline_ordering scanline_ordering);
HRESULT __cdecl wined3d_get_adapter_raster_status(const struct wined3d *wined3d, UINT adapter_idx,
        struct wined3d_raster_status *raster_status);
HRESULT __cdecl wined3d_get_device_caps(const struct wined3d *wined3d, unsigned int adapter_idx,
        enum wined3d_device_type device_type, struct wined3d_caps *caps);
HRESULT __cdecl wined3d_get_output_desc(const struct wined3d *wined3d, unsigned int adapter_idx,
        struct wined3d_output_desc *desc);
ULONG __cdecl wined3d_incref(struct wined3d *wined3d);
HRESULT __cdecl wined3d_register_software_device(struct wined3d *wined3d, void *init_function);
BOOL __cdecl wined3d_register_window(struct wined3d *wined3d, HWND window,
        struct wined3d_device *device, unsigned int flags);
HRESULT __cdecl wined3d_set_adapter_display_mode(struct wined3d *wined3d,
        UINT adapter_idx, const struct wined3d_display_mode *mode);
void __cdecl wined3d_unregister_windows(struct wined3d *wined3d);

HRESULT __cdecl wined3d_buffer_create(struct wined3d_device *device, const struct wined3d_buffer_desc *desc,
        const struct wined3d_sub_resource_data *data, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_buffer **buffer);
ULONG __cdecl wined3d_buffer_decref(struct wined3d_buffer *buffer);
void * __cdecl wined3d_buffer_get_parent(const struct wined3d_buffer *buffer);
struct wined3d_resource * __cdecl wined3d_buffer_get_resource(struct wined3d_buffer *buffer);
ULONG __cdecl wined3d_buffer_incref(struct wined3d_buffer *buffer);

HRESULT __cdecl wined3d_device_acquire_focus_window(struct wined3d_device *device, HWND window);
HRESULT __cdecl wined3d_device_begin_scene(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_begin_stateblock(struct wined3d_device *device, struct wined3d_stateblock **stateblock);
HRESULT __cdecl wined3d_device_clear(struct wined3d_device *device, DWORD rect_count, const RECT *rects, DWORD flags,
        const struct wined3d_color *color, float z, DWORD stencil);
HRESULT __cdecl wined3d_device_clear_rendertarget_view(struct wined3d_device *device,
        struct wined3d_rendertarget_view *view, const RECT *rect, DWORD flags,
        const struct wined3d_color *color, float depth, DWORD stencil);
void __cdecl wined3d_device_clear_unordered_access_view_uint(struct wined3d_device *device,
        struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value);
void __cdecl wined3d_device_copy_resource(struct wined3d_device *device,
        struct wined3d_resource *dst_resource, struct wined3d_resource *src_resource);
HRESULT __cdecl wined3d_device_copy_sub_resource_region(struct wined3d_device *device,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx, unsigned int dst_x,
        unsigned int dst_y, unsigned int dst_z, struct wined3d_resource *src_resource,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box, unsigned int flags);
void __cdecl wined3d_device_copy_uav_counter(struct wined3d_device *device,
        struct wined3d_buffer *dst_buffer, unsigned int offset, struct wined3d_unordered_access_view *uav);
HRESULT __cdecl wined3d_device_create(struct wined3d *wined3d, unsigned int adapter_idx,
        enum wined3d_device_type device_type, HWND focus_window, DWORD behaviour_flags, BYTE surface_alignment,
        const enum wined3d_feature_level *feature_levels, unsigned int feature_level_count,
        struct wined3d_device_parent *device_parent, struct wined3d_device **device);
ULONG __cdecl wined3d_device_decref(struct wined3d_device *device);
void __cdecl wined3d_device_dispatch_compute(struct wined3d_device *device,
        unsigned int group_count_x, unsigned int group_count_y, unsigned int group_count_z);
void __cdecl wined3d_device_dispatch_compute_indirect(struct wined3d_device *device,
        struct wined3d_buffer *buffer, unsigned int offset);
HRESULT __cdecl wined3d_device_draw_indexed_primitive(struct wined3d_device *device, UINT start_idx, UINT index_count);
void __cdecl wined3d_device_draw_indexed_primitive_instanced(struct wined3d_device *device,
        UINT start_idx, UINT index_count, UINT start_instance, UINT instance_count);
void __cdecl wined3d_device_draw_indexed_primitive_instanced_indirect(struct wined3d_device *device,
        struct wined3d_buffer *buffer, unsigned int offset);
HRESULT __cdecl wined3d_device_draw_primitive(struct wined3d_device *device, UINT start_vertex, UINT vertex_count);
void __cdecl wined3d_device_draw_primitive_instanced(struct wined3d_device *device,
        UINT start_vertex, UINT vertex_count, UINT start_instance, UINT instance_count);
void __cdecl wined3d_device_draw_primitive_instanced_indirect(struct wined3d_device *device,
        struct wined3d_buffer *buffer, unsigned int offset);
HRESULT __cdecl wined3d_device_end_scene(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_end_stateblock(struct wined3d_device *device);
void __cdecl wined3d_device_evict_managed_resources(struct wined3d_device *device);
UINT __cdecl wined3d_device_get_available_texture_mem(const struct wined3d_device *device);
INT __cdecl wined3d_device_get_base_vertex_index(const struct wined3d_device *device);
struct wined3d_blend_state * __cdecl wined3d_device_get_blend_state(const struct wined3d_device *device,
        struct wined3d_color *blend_factor);
HRESULT __cdecl wined3d_device_get_clip_plane(const struct wined3d_device *device,
        UINT plane_idx, struct wined3d_vec4 *plane);
HRESULT __cdecl wined3d_device_get_clip_status(const struct wined3d_device *device,
        struct wined3d_clip_status *clip_status);
struct wined3d_shader * __cdecl wined3d_device_get_compute_shader(const struct wined3d_device *device);
struct wined3d_buffer * __cdecl wined3d_device_get_constant_buffer(const struct wined3d_device *device,
        enum wined3d_shader_type shader_type, unsigned int idx);
void __cdecl wined3d_device_get_creation_parameters(const struct wined3d_device *device,
        struct wined3d_device_creation_parameters *creation_parameters);
struct wined3d_shader_resource_view * __cdecl wined3d_device_get_cs_resource_view(const struct wined3d_device *device,
        unsigned int idx);
struct wined3d_sampler * __cdecl wined3d_device_get_cs_sampler(const struct wined3d_device *device, unsigned int idx);
struct wined3d_unordered_access_view * __cdecl wined3d_device_get_cs_uav(const struct wined3d_device *device,
        unsigned int idx);
struct wined3d_rendertarget_view * __cdecl wined3d_device_get_depth_stencil_view(const struct wined3d_device *device);
HRESULT __cdecl wined3d_device_get_device_caps(const struct wined3d_device *device, struct wined3d_caps *caps);
HRESULT __cdecl wined3d_device_get_display_mode(const struct wined3d_device *device, UINT swapchain_idx,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation);
struct wined3d_shader * __cdecl wined3d_device_get_domain_shader(const struct wined3d_device *device);
struct wined3d_shader_resource_view * __cdecl wined3d_device_get_ds_resource_view(const struct wined3d_device *device,
        unsigned int idx);
struct wined3d_sampler * __cdecl wined3d_device_get_ds_sampler(const struct wined3d_device *device, unsigned int idx);
enum wined3d_feature_level __cdecl wined3d_device_get_feature_level(const struct wined3d_device *device);
void __cdecl wined3d_device_get_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_gamma_ramp *ramp);
struct wined3d_shader * __cdecl wined3d_device_get_geometry_shader(const struct wined3d_device *device);
struct wined3d_shader_resource_view * __cdecl wined3d_device_get_gs_resource_view(const struct wined3d_device *device,
        UINT idx);
struct wined3d_sampler * __cdecl wined3d_device_get_gs_sampler(const struct wined3d_device *device, UINT idx);
struct wined3d_shader_resource_view * __cdecl wined3d_device_get_hs_resource_view(const struct wined3d_device *device,
        unsigned int idx);
struct wined3d_sampler * __cdecl wined3d_device_get_hs_sampler(const struct wined3d_device *device, unsigned int idx);
struct wined3d_shader * __cdecl wined3d_device_get_hull_shader(const struct wined3d_device *device);
struct wined3d_buffer * __cdecl wined3d_device_get_index_buffer(const struct wined3d_device *device,
        enum wined3d_format_id *format, unsigned int *offset);
HRESULT __cdecl wined3d_device_get_light(const struct wined3d_device *device,
        UINT light_idx, struct wined3d_light *light);
HRESULT __cdecl wined3d_device_get_light_enable(const struct wined3d_device *device, UINT light_idx, BOOL *enable);
void __cdecl wined3d_device_get_material(const struct wined3d_device *device, struct wined3d_material *material);
unsigned int __cdecl wined3d_device_get_max_frame_latency(const struct wined3d_device *device);
float __cdecl wined3d_device_get_npatch_mode(const struct wined3d_device *device);
struct wined3d_shader * __cdecl wined3d_device_get_pixel_shader(const struct wined3d_device *device);
struct wined3d_query * __cdecl wined3d_device_get_predication(struct wined3d_device *device, BOOL *value);
void __cdecl wined3d_device_get_primitive_type(const struct wined3d_device *device,
        enum wined3d_primitive_type *primitive_topology, unsigned int *patch_vertex_count);
HRESULT __cdecl wined3d_device_get_ps_consts_b(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, BOOL *constants);
HRESULT __cdecl wined3d_device_get_ps_consts_f(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_vec4 *constants);
HRESULT __cdecl wined3d_device_get_ps_consts_i(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_ivec4 *constants);
struct wined3d_shader_resource_view * __cdecl wined3d_device_get_ps_resource_view(const struct wined3d_device *device,
        UINT idx);
struct wined3d_sampler * __cdecl wined3d_device_get_ps_sampler(const struct wined3d_device *device, UINT idx);
HRESULT __cdecl wined3d_device_get_raster_status(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_raster_status *raster_status);
struct wined3d_rasterizer_state * __cdecl wined3d_device_get_rasterizer_state(struct wined3d_device *device);
DWORD __cdecl wined3d_device_get_render_state(const struct wined3d_device *device, enum wined3d_render_state state);
struct wined3d_rendertarget_view * __cdecl wined3d_device_get_rendertarget_view(const struct wined3d_device *device,
        unsigned int view_idx);
DWORD __cdecl wined3d_device_get_sampler_state(const struct wined3d_device *device,
        UINT sampler_idx, enum wined3d_sampler_state state);
void __cdecl wined3d_device_get_scissor_rects(const struct wined3d_device *device, unsigned int *rect_count,
        RECT *rect);
BOOL __cdecl wined3d_device_get_software_vertex_processing(const struct wined3d_device *device);
struct wined3d_buffer * __cdecl wined3d_device_get_stream_output(struct wined3d_device *device,
        UINT idx, UINT *offset);
HRESULT __cdecl wined3d_device_get_stream_source(const struct wined3d_device *device,
        UINT stream_idx, struct wined3d_buffer **buffer, UINT *offset, UINT *stride);
HRESULT __cdecl wined3d_device_get_stream_source_freq(const struct wined3d_device *device,
        UINT stream_idx, UINT *divider);
struct wined3d_swapchain * __cdecl wined3d_device_get_swapchain(const struct wined3d_device *device,
        UINT swapchain_idx);
UINT __cdecl wined3d_device_get_swapchain_count(const struct wined3d_device *device);
struct wined3d_texture * __cdecl wined3d_device_get_texture(const struct wined3d_device *device, UINT stage);
DWORD __cdecl wined3d_device_get_texture_stage_state(const struct wined3d_device *device,
        UINT stage, enum wined3d_texture_stage_state state);
void __cdecl wined3d_device_get_transform(const struct wined3d_device *device,
        enum wined3d_transform_state state, struct wined3d_matrix *matrix);
struct wined3d_unordered_access_view * __cdecl wined3d_device_get_unordered_access_view(
        const struct wined3d_device *device, unsigned int idx);
struct wined3d_vertex_declaration * __cdecl wined3d_device_get_vertex_declaration(const struct wined3d_device *device);
struct wined3d_shader * __cdecl wined3d_device_get_vertex_shader(const struct wined3d_device *device);
void __cdecl wined3d_device_get_viewports(const struct wined3d_device *device, unsigned int *viewport_count,
        struct wined3d_viewport *viewports);
HRESULT __cdecl wined3d_device_get_vs_consts_b(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, BOOL *constants);
HRESULT __cdecl wined3d_device_get_vs_consts_f(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_vec4 *constants);
HRESULT __cdecl wined3d_device_get_vs_consts_i(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_ivec4 *constants);
struct wined3d_shader_resource_view * __cdecl wined3d_device_get_vs_resource_view(const struct wined3d_device *device,
        UINT idx);
struct wined3d_sampler * __cdecl wined3d_device_get_vs_sampler(const struct wined3d_device *device, UINT idx);
struct wined3d * __cdecl wined3d_device_get_wined3d(const struct wined3d_device *device);
ULONG __cdecl wined3d_device_incref(struct wined3d_device *device);
void __cdecl wined3d_device_multiply_transform(struct wined3d_device *device,
        enum wined3d_transform_state state, const struct wined3d_matrix *matrix);
HRESULT __cdecl wined3d_device_process_vertices(struct wined3d_device *device,
        UINT src_start_idx, UINT dst_idx, UINT vertex_count, struct wined3d_buffer *dst_buffer,
        const struct wined3d_vertex_declaration *declaration, DWORD flags, DWORD dst_fvf);
void __cdecl wined3d_device_release_focus_window(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_reset(struct wined3d_device *device,
        const struct wined3d_swapchain_desc *swapchain_desc, const struct wined3d_display_mode *mode,
        wined3d_device_reset_cb callback, BOOL reset_state);
void __cdecl wined3d_device_resolve_sub_resource(struct wined3d_device *device,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx,
        struct wined3d_resource *src_resource, unsigned int src_sub_resource_idx,
        enum wined3d_format_id format_id);
void __cdecl wined3d_device_set_base_vertex_index(struct wined3d_device *device, INT base_index);
void __cdecl wined3d_device_set_blend_state(struct wined3d_device *device, struct wined3d_blend_state *blend_state,
        const struct wined3d_color *blend_factor);
HRESULT __cdecl wined3d_device_set_clip_plane(struct wined3d_device *device,
        UINT plane_idx, const struct wined3d_vec4 *plane);
HRESULT __cdecl wined3d_device_set_clip_status(struct wined3d_device *device,
        const struct wined3d_clip_status *clip_status);
void __cdecl wined3d_device_set_compute_shader(struct wined3d_device *device, struct wined3d_shader *shader);
void __cdecl wined3d_device_set_constant_buffer(struct wined3d_device *device, enum wined3d_shader_type type, UINT idx,
        struct wined3d_buffer *buffer);
void __cdecl wined3d_device_set_cs_resource_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_shader_resource_view *view);
void __cdecl wined3d_device_set_cs_sampler(struct wined3d_device *device,
        unsigned int idx, struct wined3d_sampler *sampler);
void __cdecl wined3d_device_set_cs_uav(struct wined3d_device *device, unsigned int idx,
        struct wined3d_unordered_access_view *uav, unsigned int initial_count);
void __cdecl wined3d_device_set_cursor_position(struct wined3d_device *device,
        int x_screen_space, int y_screen_space, DWORD flags);
HRESULT __cdecl wined3d_device_set_cursor_properties(struct wined3d_device *device,
        UINT x_hotspot, UINT y_hotspot, struct wined3d_texture *texture, unsigned int sub_resource_idx);
HRESULT __cdecl wined3d_device_set_depth_stencil_view(struct wined3d_device *device,
        struct wined3d_rendertarget_view *view);
HRESULT __cdecl wined3d_device_set_dialog_box_mode(struct wined3d_device *device, BOOL enable_dialogs);
void __cdecl wined3d_device_set_domain_shader(struct wined3d_device *device, struct wined3d_shader *shader);
void __cdecl wined3d_device_set_ds_resource_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_shader_resource_view *view);
void __cdecl wined3d_device_set_ds_sampler(struct wined3d_device *device,
        unsigned int idx, struct wined3d_sampler *sampler);
void __cdecl wined3d_device_set_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, DWORD flags, const struct wined3d_gamma_ramp *ramp);
void __cdecl wined3d_device_set_geometry_shader(struct wined3d_device *device, struct wined3d_shader *shader);
void __cdecl wined3d_device_set_gs_resource_view(struct wined3d_device *device,
        UINT idx, struct wined3d_shader_resource_view *view);
void __cdecl wined3d_device_set_gs_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler);
void __cdecl wined3d_device_set_hs_resource_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_shader_resource_view *view);
void __cdecl wined3d_device_set_hs_sampler(struct wined3d_device *device,
        unsigned int idx, struct wined3d_sampler *sampler);
void __cdecl wined3d_device_set_hull_shader(struct wined3d_device *device, struct wined3d_shader *shader);
void __cdecl wined3d_device_set_index_buffer(struct wined3d_device *device,
        struct wined3d_buffer *index_buffer, enum wined3d_format_id format_id, unsigned int offset);
HRESULT __cdecl wined3d_device_set_light(struct wined3d_device *device,
        UINT light_idx, const struct wined3d_light *light);
HRESULT __cdecl wined3d_device_set_light_enable(struct wined3d_device *device, UINT light_idx, BOOL enable);
void __cdecl wined3d_device_set_material(struct wined3d_device *device, const struct wined3d_material *material);
void __cdecl wined3d_device_set_max_frame_latency(struct wined3d_device *device, unsigned int max_frame_latency);
void __cdecl wined3d_device_set_multithreaded(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_set_npatch_mode(struct wined3d_device *device, float segments);
void __cdecl wined3d_device_set_pixel_shader(struct wined3d_device *device, struct wined3d_shader *shader);
void __cdecl wined3d_device_set_predication(struct wined3d_device *device,
        struct wined3d_query *predicate, BOOL value);
void __cdecl wined3d_device_set_primitive_type(struct wined3d_device *device,
        enum wined3d_primitive_type primitive_topology, unsigned int patch_vertex_count);
HRESULT __cdecl wined3d_device_set_ps_consts_b(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const BOOL *constants);
HRESULT __cdecl wined3d_device_set_ps_consts_f(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants);
HRESULT __cdecl wined3d_device_set_ps_consts_i(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants);
void __cdecl wined3d_device_set_ps_resource_view(struct wined3d_device *device,
        UINT idx, struct wined3d_shader_resource_view *view);
void __cdecl wined3d_device_set_ps_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler);
void __cdecl wined3d_device_set_rasterizer_state(struct wined3d_device *device,
        struct wined3d_rasterizer_state *rasterizer_state);
void __cdecl wined3d_device_set_render_state(struct wined3d_device *device,
        enum wined3d_render_state state, DWORD value);
HRESULT __cdecl wined3d_device_set_rendertarget_view(struct wined3d_device *device,
        unsigned int view_idx, struct wined3d_rendertarget_view *view, BOOL set_viewport);
void __cdecl wined3d_device_set_sampler_state(struct wined3d_device *device,
        UINT sampler_idx, enum wined3d_sampler_state state, DWORD value);
void __cdecl wined3d_device_set_scissor_rects(struct wined3d_device *device,
        unsigned int rect_count, const RECT *rect);
void __cdecl wined3d_device_set_software_vertex_processing(struct wined3d_device *device, BOOL software);
void __cdecl wined3d_device_set_stream_output(struct wined3d_device *device, UINT idx,
        struct wined3d_buffer *buffer, UINT offset);
HRESULT __cdecl wined3d_device_set_stream_source(struct wined3d_device *device,
        UINT stream_idx, struct wined3d_buffer *buffer, UINT offset, UINT stride);
HRESULT __cdecl wined3d_device_set_stream_source_freq(struct wined3d_device *device, UINT stream_idx, UINT divider);
void __cdecl wined3d_device_set_texture(struct wined3d_device *device, UINT stage, struct wined3d_texture *texture);
void __cdecl wined3d_device_set_texture_stage_state(struct wined3d_device *device,
        UINT stage, enum wined3d_texture_stage_state state, DWORD value);
void __cdecl wined3d_device_set_transform(struct wined3d_device *device,
        enum wined3d_transform_state state, const struct wined3d_matrix *matrix);
void __cdecl wined3d_device_set_unordered_access_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_unordered_access_view *uav, unsigned int initial_count);
void __cdecl wined3d_device_set_vertex_declaration(struct wined3d_device *device,
        struct wined3d_vertex_declaration *declaration);
void __cdecl wined3d_device_set_vertex_shader(struct wined3d_device *device, struct wined3d_shader *shader);
void __cdecl wined3d_device_set_viewports(struct wined3d_device *device, unsigned int viewport_count,
        const struct wined3d_viewport *viewports);
HRESULT __cdecl wined3d_device_set_vs_consts_b(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const BOOL *constants);
HRESULT __cdecl wined3d_device_set_vs_consts_f(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants);
HRESULT __cdecl wined3d_device_set_vs_consts_i(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants);
void __cdecl wined3d_device_set_vs_resource_view(struct wined3d_device *device,
        UINT idx, struct wined3d_shader_resource_view *view);
void __cdecl wined3d_device_set_vs_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler);
BOOL __cdecl wined3d_device_show_cursor(struct wined3d_device *device, BOOL show);
void __cdecl wined3d_device_update_sub_resource(struct wined3d_device *device, struct wined3d_resource *resource,
        unsigned int sub_resource_idx, const struct wined3d_box *box, const void *data, unsigned int row_pitch,
        unsigned int depth_pitch, unsigned int flags);
HRESULT __cdecl wined3d_device_update_texture(struct wined3d_device *device,
        struct wined3d_texture *src_texture, struct wined3d_texture *dst_texture);
HRESULT __cdecl wined3d_device_validate_device(const struct wined3d_device *device, DWORD *num_passes);

HRESULT __cdecl wined3d_palette_create(struct wined3d_device *device, DWORD flags,
        unsigned int entry_count, const PALETTEENTRY *entries, struct wined3d_palette **palette);
ULONG __cdecl wined3d_palette_decref(struct wined3d_palette *palette);
HRESULT __cdecl wined3d_palette_get_entries(const struct wined3d_palette *palette,
        DWORD flags, DWORD start, DWORD count, PALETTEENTRY *entries);
void __cdecl wined3d_palette_apply_to_dc(const struct wined3d_palette *palette, HDC dc);
ULONG __cdecl wined3d_palette_incref(struct wined3d_palette *palette);
HRESULT __cdecl wined3d_palette_set_entries(struct wined3d_palette *palette,
        DWORD flags, DWORD start, DWORD count, const PALETTEENTRY *entries);

HRESULT __cdecl wined3d_query_create(struct wined3d_device *device, enum wined3d_query_type type,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_query **query);
ULONG __cdecl wined3d_query_decref(struct wined3d_query *query);
HRESULT __cdecl wined3d_query_get_data(struct wined3d_query *query, void *data, UINT data_size, DWORD flags);
UINT __cdecl wined3d_query_get_data_size(const struct wined3d_query *query);
void * __cdecl wined3d_query_get_parent(const struct wined3d_query *query);
enum wined3d_query_type __cdecl wined3d_query_get_type(const struct wined3d_query *query);
ULONG __cdecl wined3d_query_incref(struct wined3d_query *query);
HRESULT __cdecl wined3d_query_issue(struct wined3d_query *query, DWORD flags);

static inline void wined3d_private_store_init(struct wined3d_private_store *store)
{
    list_init(&store->content);
}

static inline struct wined3d_private_data *wined3d_private_store_get_private_data(
        const struct wined3d_private_store *store, const GUID *tag)
{
    struct wined3d_private_data *data;
    struct list *entry;

    LIST_FOR_EACH(entry, &store->content)
    {
        data = LIST_ENTRY(entry, struct wined3d_private_data, entry);
        if (IsEqualGUID(&data->tag, tag))
            return data;
    }
    return NULL;
}

static inline void wined3d_private_store_free_private_data(struct wined3d_private_store *store,
        struct wined3d_private_data *entry)
{
    if (entry->flags & WINED3DSPD_IUNKNOWN)
        IUnknown_Release(entry->content.object);
    list_remove(&entry->entry);
    HeapFree(GetProcessHeap(), 0, entry);
}

static inline void wined3d_private_store_cleanup(struct wined3d_private_store *store)
{
    struct wined3d_private_data *data;
    struct list *e1, *e2;

    LIST_FOR_EACH_SAFE(e1, e2, &store->content)
    {
        data = LIST_ENTRY(e1, struct wined3d_private_data, entry);
        wined3d_private_store_free_private_data(store, data);
    }
}

static inline HRESULT wined3d_private_store_set_private_data(struct wined3d_private_store *store,
        const GUID *guid, const void *data, DWORD data_size, DWORD flags)
{
    struct wined3d_private_data *d, *old;
    const void *ptr = data;

    if (flags & WINED3DSPD_IUNKNOWN)
    {
        if (data_size != sizeof(IUnknown *))
            return WINED3DERR_INVALIDCALL;
        ptr = &data;
    }

    if (!(d = HeapAlloc(GetProcessHeap(), 0,
            FIELD_OFFSET(struct wined3d_private_data, content.data[data_size]))))
        return E_OUTOFMEMORY;

    d->tag = *guid;
    d->flags = flags;
    d->size = data_size;

    memcpy(d->content.data, ptr, data_size);
    if (flags & WINED3DSPD_IUNKNOWN)
        IUnknown_AddRef(d->content.object);

    old = wined3d_private_store_get_private_data(store, guid);
    if (old)
        wined3d_private_store_free_private_data(store, old);
    list_add_tail(&store->content, &d->entry);

    return WINED3D_OK;
}

HRESULT __cdecl wined3d_blend_state_create(struct wined3d_device *device,
        const struct wined3d_blend_state_desc *desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_blend_state **state);
ULONG __cdecl wined3d_blend_state_decref(struct wined3d_blend_state *state);
void * __cdecl wined3d_blend_state_get_parent(const struct wined3d_blend_state *state);
ULONG __cdecl wined3d_blend_state_incref(struct wined3d_blend_state *state);

HRESULT __cdecl wined3d_rasterizer_state_create(struct wined3d_device *device,
        const struct wined3d_rasterizer_state_desc *desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_rasterizer_state **state);
ULONG __cdecl wined3d_rasterizer_state_decref(struct wined3d_rasterizer_state *state);
void * __cdecl wined3d_rasterizer_state_get_parent(const struct wined3d_rasterizer_state *state);
ULONG __cdecl wined3d_rasterizer_state_incref(struct wined3d_rasterizer_state *state);

void __cdecl wined3d_resource_get_desc(const struct wined3d_resource *resource,
        struct wined3d_resource_desc *desc);
void * __cdecl wined3d_resource_get_parent(const struct wined3d_resource *resource);
DWORD __cdecl wined3d_resource_get_priority(const struct wined3d_resource *resource);
HRESULT __cdecl wined3d_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, DWORD flags);
HRESULT __cdecl wined3d_resource_map_info(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_info *info, DWORD flags);
void __cdecl wined3d_resource_preload(struct wined3d_resource *resource);
void __cdecl wined3d_resource_set_parent(struct wined3d_resource *resource, void *parent);
DWORD __cdecl wined3d_resource_set_priority(struct wined3d_resource *resource, DWORD priority);
HRESULT __cdecl wined3d_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx);
UINT __cdecl wined3d_resource_update_info(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        const struct wined3d_box *box, unsigned int row_pitch, unsigned int depth_pitch);

HRESULT __cdecl wined3d_rendertarget_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view);
HRESULT __cdecl wined3d_rendertarget_view_create_from_sub_resource(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view);
ULONG __cdecl wined3d_rendertarget_view_decref(struct wined3d_rendertarget_view *view);
void * __cdecl wined3d_rendertarget_view_get_parent(const struct wined3d_rendertarget_view *view);
struct wined3d_resource * __cdecl wined3d_rendertarget_view_get_resource(const struct wined3d_rendertarget_view *view);
void * __cdecl wined3d_rendertarget_view_get_sub_resource_parent(const struct wined3d_rendertarget_view *view);
ULONG __cdecl wined3d_rendertarget_view_incref(struct wined3d_rendertarget_view *view);
void __cdecl wined3d_rendertarget_view_set_parent(struct wined3d_rendertarget_view *view, void *parent);

HRESULT __cdecl wined3d_sampler_create(struct wined3d_device *device, const struct wined3d_sampler_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_sampler **sampler);
ULONG __cdecl wined3d_sampler_decref(struct wined3d_sampler *sampler);
void * __cdecl wined3d_sampler_get_parent(const struct wined3d_sampler *sampler);
ULONG __cdecl wined3d_sampler_incref(struct wined3d_sampler *sampler);

HRESULT __cdecl wined3d_shader_create_cs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
HRESULT __cdecl wined3d_shader_create_ds(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
HRESULT __cdecl wined3d_shader_create_gs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        const struct wined3d_stream_output_desc *so_desc, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_shader **shader);
HRESULT __cdecl wined3d_shader_create_hs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
HRESULT __cdecl wined3d_shader_create_ps(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
HRESULT __cdecl wined3d_shader_create_vs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
ULONG __cdecl wined3d_shader_decref(struct wined3d_shader *shader);
HRESULT __cdecl wined3d_shader_get_byte_code(const struct wined3d_shader *shader,
        void *byte_code, UINT *byte_code_size);
void * __cdecl wined3d_shader_get_parent(const struct wined3d_shader *shader);
ULONG __cdecl wined3d_shader_incref(struct wined3d_shader *shader);
HRESULT __cdecl wined3d_shader_set_local_constants_float(struct wined3d_shader *shader,
        UINT start_idx, const float *src_data, UINT vector4f_count);

HRESULT __cdecl wined3d_shader_resource_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_shader_resource_view **view);
ULONG __cdecl wined3d_shader_resource_view_decref(struct wined3d_shader_resource_view *view);
void __cdecl wined3d_shader_resource_view_generate_mipmaps(struct wined3d_shader_resource_view *view);
void * __cdecl wined3d_shader_resource_view_get_parent(const struct wined3d_shader_resource_view *view);
ULONG __cdecl wined3d_shader_resource_view_incref(struct wined3d_shader_resource_view *view);

void __cdecl wined3d_stateblock_apply(const struct wined3d_stateblock *stateblock);
void __cdecl wined3d_stateblock_capture(struct wined3d_stateblock *stateblock);
HRESULT __cdecl wined3d_stateblock_create(struct wined3d_device *device,
        enum wined3d_stateblock_type type, struct wined3d_stateblock **stateblock);
ULONG __cdecl wined3d_stateblock_decref(struct wined3d_stateblock *stateblock);
ULONG __cdecl wined3d_stateblock_incref(struct wined3d_stateblock *stateblock);
void __cdecl wined3d_stateblock_set_blend_factor(struct wined3d_stateblock *stateblock,
        const struct wined3d_color *blend_factor);
void __cdecl wined3d_stateblock_set_pixel_shader(struct wined3d_stateblock *stateblock, struct wined3d_shader *shader);
HRESULT __cdecl wined3d_stateblock_set_ps_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const BOOL *constants);
HRESULT __cdecl wined3d_stateblock_set_ps_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants);
HRESULT __cdecl wined3d_stateblock_set_ps_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants);
void __cdecl wined3d_stateblock_set_render_state(struct wined3d_stateblock *stateblock,
        enum wined3d_render_state state, DWORD value);
void __cdecl wined3d_stateblock_set_vertex_declaration(struct wined3d_stateblock *stateblock,
        struct wined3d_vertex_declaration *declaration);
void __cdecl wined3d_stateblock_set_vertex_shader(struct wined3d_stateblock *stateblock, struct wined3d_shader *shader);
HRESULT __cdecl wined3d_stateblock_set_vs_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const BOOL *constants);
HRESULT __cdecl wined3d_stateblock_set_vs_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants);
HRESULT __cdecl wined3d_stateblock_set_vs_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants);

HRESULT __cdecl wined3d_swapchain_create(struct wined3d_device *device, struct wined3d_swapchain_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_swapchain **swapchain);
ULONG __cdecl wined3d_swapchain_decref(struct wined3d_swapchain *swapchain);
struct wined3d_texture * __cdecl wined3d_swapchain_get_back_buffer(const struct wined3d_swapchain *swapchain,
        UINT backbuffer_idx);
struct wined3d_device * __cdecl wined3d_swapchain_get_device(const struct wined3d_swapchain *swapchain);
HRESULT __cdecl wined3d_swapchain_get_display_mode(const struct wined3d_swapchain *swapchain,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation);
HRESULT __cdecl wined3d_swapchain_get_front_buffer_data(const struct wined3d_swapchain *swapchain,
        struct wined3d_texture *dst_texture, unsigned int sub_resource_idx);
HRESULT __cdecl wined3d_swapchain_get_gamma_ramp(const struct wined3d_swapchain *swapchain,
        struct wined3d_gamma_ramp *ramp);
void * __cdecl wined3d_swapchain_get_parent(const struct wined3d_swapchain *swapchain);
void __cdecl wined3d_swapchain_get_desc(const struct wined3d_swapchain *swapchain,
        struct wined3d_swapchain_desc *desc);
HRESULT __cdecl wined3d_swapchain_get_raster_status(const struct wined3d_swapchain *swapchain,
        struct wined3d_raster_status *raster_status);
struct wined3d_swapchain_state * __cdecl wined3d_swapchain_get_state(struct wined3d_swapchain *swapchain);
ULONG __cdecl wined3d_swapchain_incref(struct wined3d_swapchain *swapchain);
HRESULT __cdecl wined3d_swapchain_present(struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override, DWORD swap_interval, DWORD flags);
HRESULT __cdecl wined3d_swapchain_resize_buffers(struct wined3d_swapchain *swapchain, unsigned int buffer_count,
        unsigned int width, unsigned int height, enum wined3d_format_id format_id,
        enum wined3d_multisample_type multisample_type, unsigned int multisample_quality);
HRESULT __cdecl wined3d_swapchain_set_gamma_ramp(const struct wined3d_swapchain *swapchain,
        DWORD flags, const struct wined3d_gamma_ramp *ramp);
void __cdecl wined3d_swapchain_set_palette(struct wined3d_swapchain *swapchain, struct wined3d_palette *palette);
void __cdecl wined3d_swapchain_set_window(struct wined3d_swapchain *swapchain, HWND window);

HRESULT __cdecl wined3d_swapchain_state_create(const struct wined3d_swapchain_desc *desc,
        HWND window, struct wined3d *wined3d, unsigned int adapter_idx, struct wined3d_swapchain_state **state);
void __cdecl wined3d_swapchain_state_destroy(struct wined3d_swapchain_state *state);
HRESULT __cdecl wined3d_swapchain_state_resize_target(struct wined3d_swapchain_state *state,
        struct wined3d *wined3d, unsigned int adapter_idx, const struct wined3d_display_mode *mode);
HRESULT __cdecl wined3d_swapchain_state_set_fullscreen(struct wined3d_swapchain_state *state,
        const struct wined3d_swapchain_desc *desc, struct wined3d *wined3d,
        unsigned int adapter_idx, const struct wined3d_display_mode *mode);

HRESULT __cdecl wined3d_texture_add_dirty_region(struct wined3d_texture *texture,
        UINT layer, const struct wined3d_box *dirty_region);
HRESULT __cdecl wined3d_texture_blt(struct wined3d_texture *dst_texture, unsigned int dst_idx, const RECT *dst_rect_in,
        struct wined3d_texture *src_texture, unsigned int src_idx, const RECT *src_rect_in, DWORD flags,
        const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter);
HRESULT __cdecl wined3d_texture_create(struct wined3d_device *device, const struct wined3d_resource_desc *desc,
        UINT layer_count, UINT level_count, DWORD flags, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture);
struct wined3d_texture * __cdecl wined3d_texture_from_resource(struct wined3d_resource *resource);
ULONG __cdecl wined3d_texture_decref(struct wined3d_texture *texture);
HRESULT __cdecl wined3d_texture_get_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC *dc);
DWORD __cdecl wined3d_texture_get_level_count(const struct wined3d_texture *texture);
DWORD __cdecl wined3d_texture_get_lod(const struct wined3d_texture *texture);
HRESULT __cdecl wined3d_texture_get_overlay_position(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG *x, LONG *y);
void * __cdecl wined3d_texture_get_parent(const struct wined3d_texture *texture);
void __cdecl wined3d_texture_get_pitch(const struct wined3d_texture *texture,
        unsigned int level, unsigned int *row_pitch, unsigned int *slice_pitch);
struct wined3d_resource * __cdecl wined3d_texture_get_resource(struct wined3d_texture *texture);
HRESULT __cdecl wined3d_texture_get_sub_resource_desc(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc);
void * __cdecl wined3d_texture_get_sub_resource_parent(struct wined3d_texture *texture, unsigned int sub_resource_idx);
ULONG __cdecl wined3d_texture_incref(struct wined3d_texture *texture);
HRESULT __cdecl wined3d_texture_release_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC dc);
HRESULT __cdecl wined3d_texture_set_color_key(struct wined3d_texture *texture,
        DWORD flags, const struct wined3d_color_key *color_key);
DWORD __cdecl wined3d_texture_set_lod(struct wined3d_texture *texture, DWORD lod);
HRESULT __cdecl wined3d_texture_set_overlay_position(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG x, LONG y);
void __cdecl wined3d_texture_set_sub_resource_parent(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *parent);
HRESULT __cdecl wined3d_texture_update_desc(struct wined3d_texture *texture,
        UINT width, UINT height, enum wined3d_format_id format_id,
        enum wined3d_multisample_type multisample_type, UINT multisample_quality,
        void *mem, UINT pitch);
HRESULT __cdecl wined3d_texture_update_overlay(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const RECT *src_rect, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect, DWORD flags);

HRESULT __cdecl wined3d_unordered_access_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_unordered_access_view **view);
ULONG __cdecl wined3d_unordered_access_view_decref(struct wined3d_unordered_access_view *view);
void * __cdecl wined3d_unordered_access_view_get_parent(const struct wined3d_unordered_access_view *view);
ULONG __cdecl wined3d_unordered_access_view_incref(struct wined3d_unordered_access_view *view);

HRESULT __cdecl wined3d_vertex_declaration_create(struct wined3d_device *device,
        const struct wined3d_vertex_element *elements, UINT element_count, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_vertex_declaration **declaration);
HRESULT __cdecl wined3d_vertex_declaration_create_from_fvf(struct wined3d_device *device,
        DWORD fvf, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_vertex_declaration **declaration);
ULONG __cdecl wined3d_vertex_declaration_decref(struct wined3d_vertex_declaration *declaration);
void * __cdecl wined3d_vertex_declaration_get_parent(const struct wined3d_vertex_declaration *declaration);
ULONG __cdecl wined3d_vertex_declaration_incref(struct wined3d_vertex_declaration *declaration);

HRESULT __cdecl wined3d_extract_shader_input_signature_from_dxbc(struct wined3d_shader_signature *signature,
        const void *byte_code, SIZE_T byte_code_size);

/* Return the integer base-2 logarithm of x. Undefined for x == 0. */
static inline unsigned int wined3d_log2i(unsigned int x)
{
#if defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
    return __builtin_clz(x) ^ 0x1f;
#else
    static const unsigned int l[] =
    {
        ~0u, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
          4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    };
    unsigned int i;

    return (i = x >> 16) ? (x = i >> 8) ? l[x] + 24 : l[i] + 16 : (i = x >> 8) ? l[i] + 8 : l[x];
#endif
}

static inline int wined3d_bit_scan(unsigned int *x)
{
    int bit_offset;
#if defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
    bit_offset = __builtin_ffs(*x) - 1;
#else
    for (bit_offset = 0; bit_offset < 32; bit_offset++)
        if (*x & (1u << bit_offset)) break;
#endif
    *x ^= 1u << bit_offset;
    return bit_offset;
}

static inline void wined3d_box_set(struct wined3d_box *box, unsigned int left, unsigned int top,
        unsigned int right, unsigned int bottom, unsigned int front, unsigned int back)
{
    box->left = left;
    box->top = top;
    box->right = right;
    box->bottom = bottom;
    box->front = front;
    box->back = back;
}

BOOL wined3d_dxt1_decode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                         enum wined3d_format_id format, unsigned int w, unsigned int h);
BOOL wined3d_dxt1_encode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                         enum wined3d_format_id format, unsigned int w, unsigned int h);
BOOL wined3d_dxt3_decode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                         enum wined3d_format_id format, unsigned int w, unsigned int h);
BOOL wined3d_dxt3_encode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                         enum wined3d_format_id format, unsigned int w, unsigned int h);
BOOL wined3d_dxt5_decode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                         enum wined3d_format_id format, unsigned int w, unsigned int h);
BOOL wined3d_dxt5_encode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                         enum wined3d_format_id format, unsigned int w, unsigned int h);
BOOL wined3d_dxtn_supported(void);

#endif /* __WINE_WINED3D_H */
