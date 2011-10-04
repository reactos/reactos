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

#define WINED3D_OK                                              S_OK

#define _FACWINED3D                                             0x876
#define MAKE_WINED3DSTATUS(code)                                MAKE_HRESULT(0, _FACWINED3D, code)
#define WINED3DOK_NOAUTOGEN                                     MAKE_WINED3DSTATUS(2159)

#define MAKE_WINED3DHRESULT(code)                               MAKE_HRESULT(1, _FACWINED3D, code)
#define WINED3DERR_WRONGTEXTUREFORMAT                           MAKE_WINED3DHRESULT(2072)
#define WINED3DERR_UNSUPPORTEDCOLOROPERATION                    MAKE_WINED3DHRESULT(2073)
#define WINED3DERR_UNSUPPORTEDCOLORARG                          MAKE_WINED3DHRESULT(2074)
#define WINED3DERR_UNSUPPORTEDALPHAOPERATION                    MAKE_WINED3DHRESULT(2075)
#define WINED3DERR_UNSUPPORTEDALPHAARG                          MAKE_WINED3DHRESULT(2076)
#define WINED3DERR_TOOMANYOPERATIONS                            MAKE_WINED3DHRESULT(2077)
#define WINED3DERR_CONFLICTINGTEXTUREFILTER                     MAKE_WINED3DHRESULT(2078)
#define WINED3DERR_UNSUPPORTEDFACTORVALUE                       MAKE_WINED3DHRESULT(2079)
#define WINED3DERR_CONFLICTINGRENDERSTATE                       MAKE_WINED3DHRESULT(2081)
#define WINED3DERR_UNSUPPORTEDTEXTUREFILTER                     MAKE_WINED3DHRESULT(2082)
#define WINED3DERR_CONFLICTINGTEXTUREPALETTE                    MAKE_WINED3DHRESULT(2086)
#define WINED3DERR_DRIVERINTERNALERROR                          MAKE_WINED3DHRESULT(2087)
#define WINED3DERR_NOTFOUND                                     MAKE_WINED3DHRESULT(2150)
#define WINED3DERR_MOREDATA                                     MAKE_WINED3DHRESULT(2151)
#define WINED3DERR_DEVICELOST                                   MAKE_WINED3DHRESULT(2152)
#define WINED3DERR_DEVICENOTRESET                               MAKE_WINED3DHRESULT(2153)
#define WINED3DERR_NOTAVAILABLE                                 MAKE_WINED3DHRESULT(2154)
#define WINED3DERR_OUTOFVIDEOMEMORY                             MAKE_WINED3DHRESULT(380)
#define WINED3DERR_INVALIDDEVICE                                MAKE_WINED3DHRESULT(2155)
#define WINED3DERR_INVALIDCALL                                  MAKE_WINED3DHRESULT(2156)
#define WINED3DERR_DRIVERINVALIDCALL                            MAKE_WINED3DHRESULT(2157)
#define WINED3DERR_WASSTILLDRAWING                              MAKE_WINED3DHRESULT(540)
#define WINEDDERR_NOTAOVERLAYSURFACE                            MAKE_WINED3DHRESULT(580)
#define WINEDDERR_NOTLOCKED                                     MAKE_WINED3DHRESULT(584)
#define WINEDDERR_NODC                                          MAKE_WINED3DHRESULT(586)
#define WINEDDERR_DCALREADYCREATED                              MAKE_WINED3DHRESULT(620)
#define WINEDDERR_NOTFLIPPABLE                                  MAKE_WINED3DHRESULT(582)
#define WINEDDERR_SURFACEBUSY                                   MAKE_WINED3DHRESULT(430)
#define WINEDDERR_INVALIDRECT                                   MAKE_WINED3DHRESULT(150)
#define WINEDDERR_NOCLIPLIST                                    MAKE_WINED3DHRESULT(205)
#define WINEDDERR_OVERLAYNOTVISIBLE                             MAKE_WINED3DHRESULT(577)

typedef DWORD WINED3DCOLOR;

typedef enum _WINED3DLIGHTTYPE
{
    WINED3DLIGHT_POINT                      = 1,
    WINED3DLIGHT_SPOT                       = 2,
    WINED3DLIGHT_DIRECTIONAL                = 3,
    WINED3DLIGHT_PARALLELPOINT              = 4, /* D3D7 */
    WINED3DLIGHT_GLSPOT                     = 5, /* D3D7 */
    WINED3DLIGHT_FORCE_DWORD                = 0x7fffffff
} WINED3DLIGHTTYPE;

typedef enum _WINED3DPRIMITIVETYPE
{
    WINED3DPT_UNDEFINED                     = 0,
    WINED3DPT_POINTLIST                     = 1,
    WINED3DPT_LINELIST                      = 2,
    WINED3DPT_LINESTRIP                     = 3,
    WINED3DPT_TRIANGLELIST                  = 4,
    WINED3DPT_TRIANGLESTRIP                 = 5,
    WINED3DPT_TRIANGLEFAN                   = 6,
    WINED3DPT_LINELIST_ADJ                  = 10,
    WINED3DPT_LINESTRIP_ADJ                 = 11,
    WINED3DPT_TRIANGLELIST_ADJ              = 12,
    WINED3DPT_TRIANGLESTRIP_ADJ             = 13,
    WINED3DPT_FORCE_DWORD                   = 0x7fffffff
} WINED3DPRIMITIVETYPE;

typedef enum _WINED3DDEVTYPE
{
    WINED3DDEVTYPE_HAL                      = 1,
    WINED3DDEVTYPE_REF                      = 2,
    WINED3DDEVTYPE_SW                       = 3,
    WINED3DDEVTYPE_NULLREF                  = 4,
    WINED3DDEVTYPE_FORCE_DWORD              = 0xffffffff
} WINED3DDEVTYPE;

typedef enum _WINED3DDEGREETYPE
{
    WINED3DDEGREE_LINEAR                    = 1,
    WINED3DDEGREE_QUADRATIC                 = 2,
    WINED3DDEGREE_CUBIC                     = 3,
    WINED3DDEGREE_QUINTIC                   = 5,
    WINED3DDEGREE_FORCE_DWORD               = 0x7fffffff
} WINED3DDEGREETYPE;

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
    WINED3DFMT_R10G10B10_SNORM_A2_UNORM,
    WINED3DFMT_D16_LOCKABLE,
    WINED3DFMT_D32_UNORM,
    WINED3DFMT_S1_UINT_D15_UNORM,
    WINED3DFMT_X8D24_UNORM,
    WINED3DFMT_S4X4_UINT_D24_UNORM,
    WINED3DFMT_L16_UNORM,
    WINED3DFMT_S8_UINT_D24_FLOAT,
    WINED3DFMT_VERTEXDATA,
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
    WINED3DFMT_ATI2N                        = WINEMAKEFOURCC('A','T','I','2'),
    WINED3DFMT_INST                         = WINEMAKEFOURCC('I','N','S','T'),
    WINED3DFMT_NVDB                         = WINEMAKEFOURCC('N','V','D','B'),
    WINED3DFMT_NVHU                         = WINEMAKEFOURCC('N','V','H','U'),
    WINED3DFMT_NVHS                         = WINEMAKEFOURCC('N','V','H','S'),
    WINED3DFMT_INTZ                         = WINEMAKEFOURCC('I','N','T','Z'),
    WINED3DFMT_NULL                         = WINEMAKEFOURCC('N','U','L','L'),

    WINED3DFMT_FORCE_DWORD = 0xffffffff
};

typedef enum _WINED3DRENDERSTATETYPE
{
    WINED3DRS_ANTIALIAS                     = 2, /* d3d7 */
    WINED3DRS_TEXTUREPERSPECTIVE            = 4, /* d3d7 */
    WINED3DRS_WRAPU                         = 5, /* d3d7 */
    WINED3DRS_WRAPV                         = 6, /* d3d7 */
    WINED3DRS_ZENABLE                       = 7,
    WINED3DRS_FILLMODE                      = 8,
    WINED3DRS_SHADEMODE                     = 9,
    WINED3DRS_LINEPATTERN                   = 10, /* d3d7, d3d8 */
    WINED3DRS_MONOENABLE                    = 11, /* d3d7 */
    WINED3DRS_ROP2                          = 12, /* d3d7 */
    WINED3DRS_PLANEMASK                     = 13, /* d3d7 */
    WINED3DRS_ZWRITEENABLE                  = 14,
    WINED3DRS_ALPHATESTENABLE               = 15,
    WINED3DRS_LASTPIXEL                     = 16,
    WINED3DRS_SRCBLEND                      = 19,
    WINED3DRS_DESTBLEND                     = 20,
    WINED3DRS_CULLMODE                      = 22,
    WINED3DRS_ZFUNC                         = 23,
    WINED3DRS_ALPHAREF                      = 24,
    WINED3DRS_ALPHAFUNC                     = 25,
    WINED3DRS_DITHERENABLE                  = 26,
    WINED3DRS_ALPHABLENDENABLE              = 27,
    WINED3DRS_FOGENABLE                     = 28,
    WINED3DRS_SPECULARENABLE                = 29,
    WINED3DRS_ZVISIBLE                      = 30, /* d3d7, d3d8 */
    WINED3DRS_SUBPIXEL                      = 31, /* d3d7 */
    WINED3DRS_SUBPIXELX                     = 32, /* d3d7 */
    WINED3DRS_STIPPLEDALPHA                 = 33, /* d3d7 */
    WINED3DRS_FOGCOLOR                      = 34,
    WINED3DRS_FOGTABLEMODE                  = 35,
    WINED3DRS_FOGSTART                      = 36,
    WINED3DRS_FOGEND                        = 37,
    WINED3DRS_FOGDENSITY                    = 38,
    WINED3DRS_STIPPLEENABLE                 = 39, /* d3d7 */
    WINED3DRS_EDGEANTIALIAS                 = 40, /* d3d7, d3d8 */
    WINED3DRS_COLORKEYENABLE                = 41, /* d3d7 */
    WINED3DRS_MIPMAPLODBIAS                 = 46, /* d3d7 */
    WINED3DRS_RANGEFOGENABLE                = 48,
    WINED3DRS_ANISOTROPY                    = 49, /* d3d7 */
    WINED3DRS_FLUSHBATCH                    = 50, /* d3d7 */
    WINED3DRS_TRANSLUCENTSORTINDEPENDENT    = 51, /* d3d7 */
    WINED3DRS_STENCILENABLE                 = 52,
    WINED3DRS_STENCILFAIL                   = 53,
    WINED3DRS_STENCILZFAIL                  = 54,
    WINED3DRS_STENCILPASS                   = 55,
    WINED3DRS_STENCILFUNC                   = 56,
    WINED3DRS_STENCILREF                    = 57,
    WINED3DRS_STENCILMASK                   = 58,
    WINED3DRS_STENCILWRITEMASK              = 59,
    WINED3DRS_TEXTUREFACTOR                 = 60,
    WINED3DRS_WRAP0                         = 128,
    WINED3DRS_WRAP1                         = 129,
    WINED3DRS_WRAP2                         = 130,
    WINED3DRS_WRAP3                         = 131,
    WINED3DRS_WRAP4                         = 132,
    WINED3DRS_WRAP5                         = 133,
    WINED3DRS_WRAP6                         = 134,
    WINED3DRS_WRAP7                         = 135,
    WINED3DRS_CLIPPING                      = 136,
    WINED3DRS_LIGHTING                      = 137,
    WINED3DRS_EXTENTS                       = 138, /* d3d7 */
    WINED3DRS_AMBIENT                       = 139,
    WINED3DRS_FOGVERTEXMODE                 = 140,
    WINED3DRS_COLORVERTEX                   = 141,
    WINED3DRS_LOCALVIEWER                   = 142,
    WINED3DRS_NORMALIZENORMALS              = 143,
    WINED3DRS_COLORKEYBLENDENABLE           = 144, /* d3d7 */
    WINED3DRS_DIFFUSEMATERIALSOURCE         = 145,
    WINED3DRS_SPECULARMATERIALSOURCE        = 146,
    WINED3DRS_AMBIENTMATERIALSOURCE         = 147,
    WINED3DRS_EMISSIVEMATERIALSOURCE        = 148,
    WINED3DRS_VERTEXBLEND                   = 151,
    WINED3DRS_CLIPPLANEENABLE               = 152,
    WINED3DRS_SOFTWAREVERTEXPROCESSING      = 153, /* d3d8 */
    WINED3DRS_POINTSIZE                     = 154,
    WINED3DRS_POINTSIZE_MIN                 = 155,
    WINED3DRS_POINTSPRITEENABLE             = 156,
    WINED3DRS_POINTSCALEENABLE              = 157,
    WINED3DRS_POINTSCALE_A                  = 158,
    WINED3DRS_POINTSCALE_B                  = 159,
    WINED3DRS_POINTSCALE_C                  = 160,
    WINED3DRS_MULTISAMPLEANTIALIAS          = 161,
    WINED3DRS_MULTISAMPLEMASK               = 162,
    WINED3DRS_PATCHEDGESTYLE                = 163,
    WINED3DRS_PATCHSEGMENTS                 = 164, /* d3d8 */
    WINED3DRS_DEBUGMONITORTOKEN             = 165,
    WINED3DRS_POINTSIZE_MAX                 = 166,
    WINED3DRS_INDEXEDVERTEXBLENDENABLE      = 167,
    WINED3DRS_COLORWRITEENABLE              = 168,
    WINED3DRS_TWEENFACTOR                   = 170,
    WINED3DRS_BLENDOP                       = 171,
    WINED3DRS_POSITIONDEGREE                = 172,
    WINED3DRS_NORMALDEGREE                  = 173,
    WINED3DRS_SCISSORTESTENABLE             = 174,
    WINED3DRS_SLOPESCALEDEPTHBIAS           = 175,
    WINED3DRS_ANTIALIASEDLINEENABLE         = 176,
    WINED3DRS_MINTESSELLATIONLEVEL          = 178,
    WINED3DRS_MAXTESSELLATIONLEVEL          = 179,
    WINED3DRS_ADAPTIVETESS_X                = 180,
    WINED3DRS_ADAPTIVETESS_Y                = 181,
    WINED3DRS_ADAPTIVETESS_Z                = 182,
    WINED3DRS_ADAPTIVETESS_W                = 183,
    WINED3DRS_ENABLEADAPTIVETESSELLATION    = 184,
    WINED3DRS_TWOSIDEDSTENCILMODE           = 185,
    WINED3DRS_CCW_STENCILFAIL               = 186,
    WINED3DRS_CCW_STENCILZFAIL              = 187,
    WINED3DRS_CCW_STENCILPASS               = 188,
    WINED3DRS_CCW_STENCILFUNC               = 189,
    WINED3DRS_COLORWRITEENABLE1             = 190,
    WINED3DRS_COLORWRITEENABLE2             = 191,
    WINED3DRS_COLORWRITEENABLE3             = 192,
    WINED3DRS_BLENDFACTOR                   = 193,
    WINED3DRS_SRGBWRITEENABLE               = 194,
    WINED3DRS_DEPTHBIAS                     = 195,
    WINED3DRS_WRAP8                         = 198,
    WINED3DRS_WRAP9                         = 199,
    WINED3DRS_WRAP10                        = 200,
    WINED3DRS_WRAP11                        = 201,
    WINED3DRS_WRAP12                        = 202,
    WINED3DRS_WRAP13                        = 203,
    WINED3DRS_WRAP14                        = 204,
    WINED3DRS_WRAP15                        = 205,
    WINED3DRS_SEPARATEALPHABLENDENABLE      = 206,
    WINED3DRS_SRCBLENDALPHA                 = 207,
    WINED3DRS_DESTBLENDALPHA                = 208,
    WINED3DRS_BLENDOPALPHA                  = 209,
    WINED3DRS_FORCE_DWORD                   = 0x7fffffff
} WINED3DRENDERSTATETYPE;
#define WINEHIGHEST_RENDER_STATE                                WINED3DRS_BLENDOPALPHA

typedef enum _WINED3DBLEND
{
    WINED3DBLEND_ZERO                       =  1,
    WINED3DBLEND_ONE                        =  2,
    WINED3DBLEND_SRCCOLOR                   =  3,
    WINED3DBLEND_INVSRCCOLOR                =  4,
    WINED3DBLEND_SRCALPHA                   =  5,
    WINED3DBLEND_INVSRCALPHA                =  6,
    WINED3DBLEND_DESTALPHA                  =  7,
    WINED3DBLEND_INVDESTALPHA               =  8,
    WINED3DBLEND_DESTCOLOR                  =  9,
    WINED3DBLEND_INVDESTCOLOR               = 10,
    WINED3DBLEND_SRCALPHASAT                = 11,
    WINED3DBLEND_BOTHSRCALPHA               = 12,
    WINED3DBLEND_BOTHINVSRCALPHA            = 13,
    WINED3DBLEND_BLENDFACTOR                = 14,
    WINED3DBLEND_INVBLENDFACTOR             = 15,
    WINED3DBLEND_FORCE_DWORD                = 0x7fffffff
} WINED3DBLEND;

typedef enum _WINED3DBLENDOP
{
    WINED3DBLENDOP_ADD                      = 1,
    WINED3DBLENDOP_SUBTRACT                 = 2,
    WINED3DBLENDOP_REVSUBTRACT              = 3,
    WINED3DBLENDOP_MIN                      = 4,
    WINED3DBLENDOP_MAX                      = 5,
    WINED3DBLENDOP_FORCE_DWORD              = 0x7fffffff
} WINED3DBLENDOP;

typedef enum _WINED3DVERTEXBLENDFLAGS
{
    WINED3DVBF_DISABLE                      = 0,
    WINED3DVBF_1WEIGHTS                     = 1,
    WINED3DVBF_2WEIGHTS                     = 2,
    WINED3DVBF_3WEIGHTS                     = 3,
    WINED3DVBF_TWEENING                     = 255,
    WINED3DVBF_0WEIGHTS                     = 256
} WINED3DVERTEXBLENDFLAGS;

typedef enum _WINED3DCMPFUNC
{
    WINED3DCMP_NEVER                        = 1,
    WINED3DCMP_LESS                         = 2,
    WINED3DCMP_EQUAL                        = 3,
    WINED3DCMP_LESSEQUAL                    = 4,
    WINED3DCMP_GREATER                      = 5,
    WINED3DCMP_NOTEQUAL                     = 6,
    WINED3DCMP_GREATEREQUAL                 = 7,
    WINED3DCMP_ALWAYS                       = 8,
    WINED3DCMP_FORCE_DWORD                  = 0x7fffffff
} WINED3DCMPFUNC;

typedef enum _WINED3DZBUFFERTYPE
{
    WINED3DZB_FALSE                         = 0,
    WINED3DZB_TRUE                          = 1,
    WINED3DZB_USEW                          = 2,
    WINED3DZB_FORCE_DWORD                   = 0x7fffffff
} WINED3DZBUFFERTYPE;

typedef enum _WINED3DFOGMODE
{
    WINED3DFOG_NONE                         = 0,
    WINED3DFOG_EXP                          = 1,
    WINED3DFOG_EXP2                         = 2,
    WINED3DFOG_LINEAR                       = 3,
    WINED3DFOG_FORCE_DWORD                  = 0x7fffffff
} WINED3DFOGMODE;

typedef enum _WINED3DSHADEMODE
{
    WINED3DSHADE_FLAT                       = 1,
    WINED3DSHADE_GOURAUD                    = 2,
    WINED3DSHADE_PHONG                      = 3,
    WINED3DSHADE_FORCE_DWORD                = 0x7fffffff
} WINED3DSHADEMODE;

typedef enum _WINED3DFILLMODE
{
    WINED3DFILL_POINT                       = 1,
    WINED3DFILL_WIREFRAME                   = 2,
    WINED3DFILL_SOLID                       = 3,
    WINED3DFILL_FORCE_DWORD                 = 0x7fffffff
} WINED3DFILLMODE;

typedef enum _WINED3DCULL
{
    WINED3DCULL_NONE                        = 1,
    WINED3DCULL_CW                          = 2,
    WINED3DCULL_CCW                         = 3,
    WINED3DCULL_FORCE_DWORD                 = 0x7fffffff
} WINED3DCULL;

typedef enum _WINED3DSTENCILOP
{
    WINED3DSTENCILOP_KEEP                   = 1,
    WINED3DSTENCILOP_ZERO                   = 2,
    WINED3DSTENCILOP_REPLACE                = 3,
    WINED3DSTENCILOP_INCRSAT                = 4,
    WINED3DSTENCILOP_DECRSAT                = 5,
    WINED3DSTENCILOP_INVERT                 = 6,
    WINED3DSTENCILOP_INCR                   = 7,
    WINED3DSTENCILOP_DECR                   = 8,
    WINED3DSTENCILOP_FORCE_DWORD            = 0x7fffffff
} WINED3DSTENCILOP;

typedef enum _WINED3DMATERIALCOLORSOURCE
{
    WINED3DMCS_MATERIAL                     = 0,
    WINED3DMCS_COLOR1                       = 1,
    WINED3DMCS_COLOR2                       = 2,
    WINED3DMCS_FORCE_DWORD                  = 0x7fffffff
} WINED3DMATERIALCOLORSOURCE;

typedef enum _WINED3DPATCHEDGESTYLE
{
    WINED3DPATCHEDGE_DISCRETE               = 0,
    WINED3DPATCHEDGE_CONTINUOUS             = 1,
    WINED3DPATCHEDGE_FORCE_DWORD            = 0x7fffffff
} WINED3DPATCHEDGESTYLE;

typedef enum _WINED3DBACKBUFFER_TYPE
{
    WINED3DBACKBUFFER_TYPE_MONO             = 0,
    WINED3DBACKBUFFER_TYPE_LEFT             = 1,
    WINED3DBACKBUFFER_TYPE_RIGHT            = 2,
    WINED3DBACKBUFFER_TYPE_FORCE_DWORD      = 0x7fffffff
} WINED3DBACKBUFFER_TYPE;

typedef enum _WINED3DSWAPEFFECT
{
    WINED3DSWAPEFFECT_DISCARD               = 1,
    WINED3DSWAPEFFECT_FLIP                  = 2,
    WINED3DSWAPEFFECT_COPY                  = 3,
    WINED3DSWAPEFFECT_COPY_VSYNC            = 4,
    WINED3DSWAPEFFECT_FORCE_DWORD           = 0xffffffff
} WINED3DSWAPEFFECT;

typedef enum _WINED3DSAMPLERSTATETYPE
{
    WINED3DSAMP_ADDRESSU                    = 1,
    WINED3DSAMP_ADDRESSV                    = 2,
    WINED3DSAMP_ADDRESSW                    = 3,
    WINED3DSAMP_BORDERCOLOR                 = 4,
    WINED3DSAMP_MAGFILTER                   = 5,
    WINED3DSAMP_MINFILTER                   = 6,
    WINED3DSAMP_MIPFILTER                   = 7,
    WINED3DSAMP_MIPMAPLODBIAS               = 8,
    WINED3DSAMP_MAXMIPLEVEL                 = 9,
    WINED3DSAMP_MAXANISOTROPY               = 10,
    WINED3DSAMP_SRGBTEXTURE                 = 11,
    WINED3DSAMP_ELEMENTINDEX                = 12,
    WINED3DSAMP_DMAPOFFSET                  = 13,
    WINED3DSAMP_FORCE_DWORD                 = 0x7fffffff,
} WINED3DSAMPLERSTATETYPE;
#define WINED3D_HIGHEST_SAMPLER_STATE                           WINED3DSAMP_DMAPOFFSET

typedef enum _WINED3DMULTISAMPLE_TYPE
{
    WINED3DMULTISAMPLE_NONE                 = 0,
    WINED3DMULTISAMPLE_NONMASKABLE          = 1,
    WINED3DMULTISAMPLE_2_SAMPLES            = 2,
    WINED3DMULTISAMPLE_3_SAMPLES            = 3,
    WINED3DMULTISAMPLE_4_SAMPLES            = 4,
    WINED3DMULTISAMPLE_5_SAMPLES            = 5,
    WINED3DMULTISAMPLE_6_SAMPLES            = 6,
    WINED3DMULTISAMPLE_7_SAMPLES            = 7,
    WINED3DMULTISAMPLE_8_SAMPLES            = 8,
    WINED3DMULTISAMPLE_9_SAMPLES            = 9,
    WINED3DMULTISAMPLE_10_SAMPLES           = 10,
    WINED3DMULTISAMPLE_11_SAMPLES           = 11,
    WINED3DMULTISAMPLE_12_SAMPLES           = 12,
    WINED3DMULTISAMPLE_13_SAMPLES           = 13,
    WINED3DMULTISAMPLE_14_SAMPLES           = 14,
    WINED3DMULTISAMPLE_15_SAMPLES           = 15,
    WINED3DMULTISAMPLE_16_SAMPLES           = 16,
    WINED3DMULTISAMPLE_FORCE_DWORD          = 0xffffffff
} WINED3DMULTISAMPLE_TYPE;

typedef enum _WINED3DTEXTURESTAGESTATETYPE
{
    WINED3DTSS_COLOROP                      = 0,
    WINED3DTSS_COLORARG1                    = 1,
    WINED3DTSS_COLORARG2                    = 2,
    WINED3DTSS_ALPHAOP                      = 3,
    WINED3DTSS_ALPHAARG1                    = 4,
    WINED3DTSS_ALPHAARG2                    = 5,
    WINED3DTSS_BUMPENVMAT00                 = 6,
    WINED3DTSS_BUMPENVMAT01                 = 7,
    WINED3DTSS_BUMPENVMAT10                 = 8,
    WINED3DTSS_BUMPENVMAT11                 = 9,
    WINED3DTSS_TEXCOORDINDEX                = 10,
    WINED3DTSS_BUMPENVLSCALE                = 11,
    WINED3DTSS_BUMPENVLOFFSET               = 12,
    WINED3DTSS_TEXTURETRANSFORMFLAGS        = 13,
    WINED3DTSS_COLORARG0                    = 14,
    WINED3DTSS_ALPHAARG0                    = 15,
    WINED3DTSS_RESULTARG                    = 16,
    WINED3DTSS_CONSTANT                     = 17,
    WINED3DTSS_FORCE_DWORD                  = 0x7fffffff
} WINED3DTEXTURESTAGESTATETYPE;
#define WINED3D_HIGHEST_TEXTURE_STATE                           WINED3DTSS_CONSTANT

typedef enum _WINED3DTEXTURETRANSFORMFLAGS
{
    WINED3DTTFF_DISABLE                     = 0,
    WINED3DTTFF_COUNT1                      = 1,
    WINED3DTTFF_COUNT2                      = 2,
    WINED3DTTFF_COUNT3                      = 3,
    WINED3DTTFF_COUNT4                      = 4,
    WINED3DTTFF_PROJECTED                   = 256,
    WINED3DTTFF_FORCE_DWORD                 = 0x7fffffff
} WINED3DTEXTURETRANSFORMFLAGS;

typedef enum _WINED3DTEXTUREOP
{
    WINED3DTOP_DISABLE                      = 1,
    WINED3DTOP_SELECTARG1                   = 2,
    WINED3DTOP_SELECTARG2                   = 3,
    WINED3DTOP_MODULATE                     = 4,
    WINED3DTOP_MODULATE2X                   = 5,
    WINED3DTOP_MODULATE4X                   = 6,
    WINED3DTOP_ADD                          = 7,
    WINED3DTOP_ADDSIGNED                    = 8,
    WINED3DTOP_ADDSIGNED2X                  = 9,
    WINED3DTOP_SUBTRACT                     = 10,
    WINED3DTOP_ADDSMOOTH                    = 11,
    WINED3DTOP_BLENDDIFFUSEALPHA            = 12,
    WINED3DTOP_BLENDTEXTUREALPHA            = 13,
    WINED3DTOP_BLENDFACTORALPHA             = 14,
    WINED3DTOP_BLENDTEXTUREALPHAPM          = 15,
    WINED3DTOP_BLENDCURRENTALPHA            = 16,
    WINED3DTOP_PREMODULATE                  = 17,
    WINED3DTOP_MODULATEALPHA_ADDCOLOR       = 18,
    WINED3DTOP_MODULATECOLOR_ADDALPHA       = 19,
    WINED3DTOP_MODULATEINVALPHA_ADDCOLOR    = 20,
    WINED3DTOP_MODULATEINVCOLOR_ADDALPHA    = 21,
    WINED3DTOP_BUMPENVMAP                   = 22,
    WINED3DTOP_BUMPENVMAPLUMINANCE          = 23,
    WINED3DTOP_DOTPRODUCT3                  = 24,
    WINED3DTOP_MULTIPLYADD                  = 25,
    WINED3DTOP_LERP                         = 26,
    WINED3DTOP_FORCE_DWORD                  = 0x7fffffff,
} WINED3DTEXTUREOP;

typedef enum _WINED3DTEXTUREADDRESS
{
    WINED3DTADDRESS_WRAP                    = 1,
    WINED3DTADDRESS_MIRROR                  = 2,
    WINED3DTADDRESS_CLAMP                   = 3,
    WINED3DTADDRESS_BORDER                  = 4,
    WINED3DTADDRESS_MIRRORONCE              = 5,
    WINED3DTADDRESS_FORCE_DWORD             = 0x7fffffff
} WINED3DTEXTUREADDRESS;

typedef enum _WINED3DTRANSFORMSTATETYPE
{
    WINED3DTS_VIEW                          = 2,
    WINED3DTS_PROJECTION                    = 3,
    WINED3DTS_TEXTURE0                      = 16,
    WINED3DTS_TEXTURE1                      = 17,
    WINED3DTS_TEXTURE2                      = 18,
    WINED3DTS_TEXTURE3                      = 19,
    WINED3DTS_TEXTURE4                      = 20,
    WINED3DTS_TEXTURE5                      = 21,
    WINED3DTS_TEXTURE6                      = 22,
    WINED3DTS_TEXTURE7                      = 23,
    WINED3DTS_WORLD                         = 256, /*WINED3DTS_WORLDMATRIX(0)*/
    WINED3DTS_WORLD1                        = 257,
    WINED3DTS_WORLD2                        = 258,
    WINED3DTS_WORLD3                        = 259,
    WINED3DTS_FORCE_DWORD                   = 0x7fffffff
} WINED3DTRANSFORMSTATETYPE;

#define WINED3DTS_WORLDMATRIX(index)                            (WINED3DTRANSFORMSTATETYPE)(index + 256)

typedef enum _WINED3DBASISTYPE
{
    WINED3DBASIS_BEZIER                     = 0,
    WINED3DBASIS_BSPLINE                    = 1,
    WINED3DBASIS_INTERPOLATE                = 2,
    WINED3DBASIS_FORCE_DWORD                = 0x7fffffff
} WINED3DBASISTYPE;

typedef enum _WINED3DCUBEMAP_FACES
{
    WINED3DCUBEMAP_FACE_POSITIVE_X          = 0,
    WINED3DCUBEMAP_FACE_NEGATIVE_X          = 1,
    WINED3DCUBEMAP_FACE_POSITIVE_Y          = 2,
    WINED3DCUBEMAP_FACE_NEGATIVE_Y          = 3,
    WINED3DCUBEMAP_FACE_POSITIVE_Z          = 4,
    WINED3DCUBEMAP_FACE_NEGATIVE_Z          = 5,
    WINED3DCUBEMAP_FACE_FORCE_DWORD         = 0xffffffff
} WINED3DCUBEMAP_FACES;

typedef enum _WINED3DTEXTUREFILTERTYPE
{
    WINED3DTEXF_NONE                        = 0,
    WINED3DTEXF_POINT                       = 1,
    WINED3DTEXF_LINEAR                      = 2,
    WINED3DTEXF_ANISOTROPIC                 = 3,
    WINED3DTEXF_FLATCUBIC                   = 4,
    WINED3DTEXF_GAUSSIANCUBIC               = 5,
    WINED3DTEXF_PYRAMIDALQUAD               = 6,
    WINED3DTEXF_GAUSSIANQUAD                = 7,
    WINED3DTEXF_FORCE_DWORD                 = 0x7fffffff
} WINED3DTEXTUREFILTERTYPE;

typedef enum _WINED3DRESOURCETYPE
{
    WINED3DRTYPE_SURFACE                    = 1,
    WINED3DRTYPE_VOLUME                     = 2,
    WINED3DRTYPE_TEXTURE                    = 3,
    WINED3DRTYPE_VOLUMETEXTURE              = 4,
    WINED3DRTYPE_CUBETEXTURE                = 5,
    WINED3DRTYPE_BUFFER                     = 6,
    WINED3DRTYPE_FORCE_DWORD                = 0x7fffffff
} WINED3DRESOURCETYPE;
#define WINED3DRTYPECOUNT                                       WINED3DRTYPE_BUFFER

typedef enum _WINED3DPOOL
{
    WINED3DPOOL_DEFAULT                     = 0,
    WINED3DPOOL_MANAGED                     = 1,
    WINED3DPOOL_SYSTEMMEM                   = 2,
    WINED3DPOOL_SCRATCH                     = 3,
    WINED3DPOOL_FORCE_DWORD                 = 0x7fffffff
} WINED3DPOOL;

typedef enum _WINED3DQUERYTYPE
{
    WINED3DQUERYTYPE_VCACHE                 = 4,
    WINED3DQUERYTYPE_RESOURCEMANAGER        = 5,
    WINED3DQUERYTYPE_VERTEXSTATS            = 6,
    WINED3DQUERYTYPE_EVENT                  = 8,
    WINED3DQUERYTYPE_OCCLUSION              = 9,
    WINED3DQUERYTYPE_TIMESTAMP              = 10,
    WINED3DQUERYTYPE_TIMESTAMPDISJOINT      = 11,
    WINED3DQUERYTYPE_TIMESTAMPFREQ          = 12,
    WINED3DQUERYTYPE_PIPELINETIMINGS        = 13,
    WINED3DQUERYTYPE_INTERFACETIMINGS       = 14,
    WINED3DQUERYTYPE_VERTEXTIMINGS          = 15,
    WINED3DQUERYTYPE_PIXELTIMINGS           = 16,
    WINED3DQUERYTYPE_BANDWIDTHTIMINGS       = 17,
    WINED3DQUERYTYPE_CACHEUTILIZATION       = 18
} WINED3DQUERYTYPE;

#define WINED3DISSUE_BEGIN                                      (1 << 1)
#define WINED3DISSUE_END                                        (1 << 0)
#define WINED3DGETDATA_FLUSH                                    (1 << 0)

typedef enum _WINED3DSTATEBLOCKTYPE
{
    WINED3DSBT_INIT                         = 0,
    WINED3DSBT_ALL                          = 1,
    WINED3DSBT_PIXELSTATE                   = 2,
    WINED3DSBT_VERTEXSTATE                  = 3,
    WINED3DSBT_RECORDED                     = 4, /* WineD3D private */
    WINED3DSBT_FORCE_DWORD                  = 0xffffffff
} WINED3DSTATEBLOCKTYPE;

typedef enum _WINED3DDECLMETHOD
{
    WINED3DDECLMETHOD_DEFAULT               = 0,
    WINED3DDECLMETHOD_PARTIALU              = 1,
    WINED3DDECLMETHOD_PARTIALV              = 2,
    WINED3DDECLMETHOD_CROSSUV               = 3,
    WINED3DDECLMETHOD_UV                    = 4,
    WINED3DDECLMETHOD_LOOKUP                = 5,
    WINED3DDECLMETHOD_LOOKUPPRESAMPLED      = 6
} WINED3DDECLMETHOD;

typedef enum _WINED3DDECLUSAGE
{
    WINED3DDECLUSAGE_POSITION               = 0,
    WINED3DDECLUSAGE_BLENDWEIGHT            = 1,
    WINED3DDECLUSAGE_BLENDINDICES           = 2,
    WINED3DDECLUSAGE_NORMAL                 = 3,
    WINED3DDECLUSAGE_PSIZE                  = 4,
    WINED3DDECLUSAGE_TEXCOORD               = 5,
    WINED3DDECLUSAGE_TANGENT                = 6,
    WINED3DDECLUSAGE_BINORMAL               = 7,
    WINED3DDECLUSAGE_TESSFACTOR             = 8,
    WINED3DDECLUSAGE_POSITIONT              = 9,
    WINED3DDECLUSAGE_COLOR                  = 10,
    WINED3DDECLUSAGE_FOG                    = 11,
    WINED3DDECLUSAGE_DEPTH                  = 12,
    WINED3DDECLUSAGE_SAMPLE                 = 13
} WINED3DDECLUSAGE;

typedef enum _WINED3DSURFTYPE
{
    SURFACE_UNKNOWN                         = 0,    /* Default / Unknown surface type */
    SURFACE_OPENGL,                                 /* OpenGL surface: Renders using libGL, needed for 3D */
    SURFACE_GDI,                                    /* User surface. No 3D, DirectDraw rendering with GDI */
} WINED3DSURFTYPE;

enum wined3d_sysval_semantic
{
    WINED3D_SV_DEPTH = 0xffffffff,
    WINED3D_SV_TARGET0 = 0,
    WINED3D_SV_TARGET1 = 1,
    WINED3D_SV_TARGET2 = 2,
    WINED3D_SV_TARGET3 = 3,
    WINED3D_SV_TARGET4 = 4,
    WINED3D_SV_TARGET5 = 5,
    WINED3D_SV_TARGET6 = 6,
    WINED3D_SV_TARGET7 = 7,
};

#define WINED3DCOLORWRITEENABLE_RED                             (1 << 0)
#define WINED3DCOLORWRITEENABLE_GREEN                           (1 << 1)
#define WINED3DCOLORWRITEENABLE_BLUE                            (1 << 2)
#define WINED3DCOLORWRITEENABLE_ALPHA                           (1 << 3)

#define WINED3DADAPTER_DEFAULT                                  0
#define WINED3DENUM_NO_WHQL_LEVEL                               2
#define WINED3DPRESENT_BACK_BUFFER_MAX                          3

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

#define WINED3DPRESENTFLAG_LOCKABLE_BACKBUFFER                  0x00000001
#define WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL                 0x00000002
#define WINED3DPRESENTFLAG_DEVICECLIP                           0x00000004
#define WINED3DPRESENTFLAG_VIDEO                                0x00000010
#define WINED3DPRESENTFLAG_NOAUTOROTATE                         0x00000020
#define WINED3DPRESENTFLAG_UNPRUNEDMODE                         0x00000040

#define WINED3DDP_MAXTEXCOORD                                   8

#define WINED3DUSAGE_RENDERTARGET                               0x00000001
#define WINED3DUSAGE_DEPTHSTENCIL                               0x00000002
#define WINED3DUSAGE_WRITEONLY                                  0x00000008
#define WINED3DUSAGE_SOFTWAREPROCESSING                         0x00000010
#define WINED3DUSAGE_DONOTCLIP                                  0x00000020
#define WINED3DUSAGE_POINTS                                     0x00000040
#define WINED3DUSAGE_RTPATCHES                                  0x00000080
#define WINED3DUSAGE_NPATCHES                                   0x00000100
#define WINED3DUSAGE_DYNAMIC                                    0x00000200
#define WINED3DUSAGE_AUTOGENMIPMAP                              0x00000400
#define WINED3DUSAGE_DMAP                                       0x00004000
#define WINED3DUSAGE_MASK                                       0x00004fff
#define WINED3DUSAGE_STATICDECL                                 0x40000000
#define WINED3DUSAGE_OVERLAY                                    0x80000000

#define WINED3DUSAGE_QUERY_LEGACYBUMPMAP                        0x00008000
#define WINED3DUSAGE_QUERY_FILTER                               0x00020000
#define WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING             0x00080000
#define WINED3DUSAGE_QUERY_SRGBREAD                             0x00010000
#define WINED3DUSAGE_QUERY_SRGBWRITE                            0x00040000
#define WINED3DUSAGE_QUERY_VERTEXTEXTURE                        0x00100000
#define WINED3DUSAGE_QUERY_WRAPANDMIP                           0x00200000
#define WINED3DUSAGE_QUERY_MASK                                 0x003f8000

#define WINED3DLOCK_READONLY                                    0x0010
#define WINED3DLOCK_NOSYSLOCK                                   0x0800
#define WINED3DLOCK_NOOVERWRITE                                 0x1000
#define WINED3DLOCK_DISCARD                                     0x2000
#define WINED3DLOCK_DONOTWAIT                                   0x4000
#define WINED3DLOCK_NO_DIRTY_UPDATE                             0x8000

#define WINED3DPRESENT_RATE_DEFAULT                             0x00000000

#define WINED3DPRESENT_INTERVAL_DEFAULT                         0x00000000
#define WINED3DPRESENT_INTERVAL_ONE                             0x00000001
#define WINED3DPRESENT_INTERVAL_TWO                             0x00000002
#define WINED3DPRESENT_INTERVAL_THREE                           0x00000004
#define WINED3DPRESENT_INTERVAL_FOUR                            0x00000008
#define WINED3DPRESENT_INTERVAL_IMMEDIATE                       0x80000000

#define WINED3DMAXUSERCLIPPLANES                                32
#define WINED3DCLIPPLANE0                                       (1 << 0)
#define WINED3DCLIPPLANE1                                       (1 << 1)
#define WINED3DCLIPPLANE2                                       (1 << 2)
#define WINED3DCLIPPLANE3                                       (1 << 3)
#define WINED3DCLIPPLANE4                                       (1 << 4)
#define WINED3DCLIPPLANE5                                       (1 << 5)

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

#define WINED3DFVF_TEXTUREFORMAT1                               3
#define WINED3DFVF_TEXTUREFORMAT2                               0
#define WINED3DFVF_TEXTUREFORMAT3                               1
#define WINED3DFVF_TEXTUREFORMAT4                               2
#define WINED3DFVF_TEXCOORDSIZE1(idx)                           (WINED3DFVF_TEXTUREFORMAT1 << (idx * 2 + 16))
#define WINED3DFVF_TEXCOORDSIZE2(idx)                           (WINED3DFVF_TEXTUREFORMAT2 << (idx * 2 + 16))
#define WINED3DFVF_TEXCOORDSIZE3(idx)                           (WINED3DFVF_TEXTUREFORMAT3 << (idx * 2 + 16))
#define WINED3DFVF_TEXCOORDSIZE4(idx)                           (WINED3DFVF_TEXTUREFORMAT4 << (idx * 2 + 16))

/* Clear flags */
#define WINED3DCLEAR_TARGET                                     0x00000001
#define WINED3DCLEAR_ZBUFFER                                    0x00000002
#define WINED3DCLEAR_STENCIL                                    0x00000004

/* Stream source flags */
#define WINED3DSTREAMSOURCE_INDEXEDDATA                         (1 << 30)
#define WINED3DSTREAMSOURCE_INSTANCEDATA                        (2 << 30)

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
#define WINED3DCAPS2_CANAUTOGENMIPMAP                           0x40000000

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
#define WINED3DPRASTERCAPS_ZBIAS                                0x00004000
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

/* dwFlags for Blt* */
#define WINEDDBLT_ALPHADEST                                     0x00000001
#define WINEDDBLT_ALPHADESTCONSTOVERRIDE                        0x00000002
#define WINEDDBLT_ALPHADESTNEG                                  0x00000004
#define WINEDDBLT_ALPHADESTSURFACEOVERRIDE                      0x00000008
#define WINEDDBLT_ALPHAEDGEBLEND                                0x00000010
#define WINEDDBLT_ALPHASRC                                      0x00000020
#define WINEDDBLT_ALPHASRCCONSTOVERRIDE                         0x00000040
#define WINEDDBLT_ALPHASRCNEG                                   0x00000080
#define WINEDDBLT_ALPHASRCSURFACEOVERRIDE                       0x00000100
#define WINEDDBLT_ASYNC                                         0x00000200
#define WINEDDBLT_COLORFILL                                     0x00000400
#define WINEDDBLT_DDFX                                          0x00000800
#define WINEDDBLT_DDROPS                                        0x00001000
#define WINEDDBLT_KEYDEST                                       0x00002000
#define WINEDDBLT_KEYDESTOVERRIDE                               0x00004000
#define WINEDDBLT_KEYSRC                                        0x00008000
#define WINEDDBLT_KEYSRCOVERRIDE                                0x00010000
#define WINEDDBLT_ROP                                           0x00020000
#define WINEDDBLT_ROTATIONANGLE                                 0x00040000
#define WINEDDBLT_ZBUFFER                                       0x00080000
#define WINEDDBLT_ZBUFFERDESTCONSTOVERRIDE                      0x00100000
#define WINEDDBLT_ZBUFFERDESTOVERRIDE                           0x00200000
#define WINEDDBLT_ZBUFFERSRCCONSTOVERRIDE                       0x00400000
#define WINEDDBLT_ZBUFFERSRCOVERRIDE                            0x00800000
#define WINEDDBLT_WAIT                                          0x01000000
#define WINEDDBLT_DEPTHFILL                                     0x02000000
#define WINEDDBLT_DONOTWAIT                                     0x08000000

/* dwTrans for BltFast */
#define WINEDDBLTFAST_NOCOLORKEY                                0x00000000
#define WINEDDBLTFAST_SRCCOLORKEY                               0x00000001
#define WINEDDBLTFAST_DESTCOLORKEY                              0x00000002
#define WINEDDBLTFAST_WAIT                                      0x00000010
#define WINEDDBLTFAST_DONOTWAIT                                 0x00000020

/* DDSURFACEDESC.dwFlags */
#define WINEDDSD_CAPS                                           0x00000001
#define WINEDDSD_HEIGHT                                         0x00000002
#define WINEDDSD_WIDTH                                          0x00000004
#define WINEDDSD_PITCH                                          0x00000008
#define WINEDDSD_BACKBUFFERCOUNT                                0x00000020
#define WINEDDSD_ZBUFFERBITDEPTH                                0x00000040
#define WINEDDSD_ALPHABITDEPTH                                  0x00000080
#define WINEDDSD_LPSURFACE                                      0x00000800
#define WINEDDSD_PIXELFORMAT                                    0x00001000
#define WINEDDSD_CKDESTOVERLAY                                  0x00002000
#define WINEDDSD_CKDESTBLT                                      0x00004000
#define WINEDDSD_CKSRCOVERLAY                                   0x00008000
#define WINEDDSD_CKSRCBLT                                       0x00010000
#define WINEDDSD_MIPMAPCOUNT                                    0x00020000
#define WINEDDSD_REFRESHRATE                                    0x00040000
#define WINEDDSD_LINEARSIZE                                     0x00080000
#define WINEDDSD_TEXTURESTAGE                                   0x00100000
#define WINEDDSD_FVF                                            0x00200000
#define WINEDDSD_SRCVBHANDLE                                    0x00400000
#define WINEDDSD_ALL                                            0x007ff9ee

/* Set/Get Colour Key Flags */
#define WINEDDCKEY_COLORSPACE                                   0x00000001 /* Struct is single colour space */
#define WINEDDCKEY_DESTBLT                                      0x00000002 /* To be used as dest for blt */
#define WINEDDCKEY_DESTOVERLAY                                  0x00000004 /* To be used as dest for CK overlays */
#define WINEDDCKEY_SRCBLT                                       0x00000008 /* To be used as src for blt */
#define WINEDDCKEY_SRCOVERLAY                                   0x00000010 /* To be used as src for CK overlays */

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

/* DDCAPS.d */
#define WINEDDPCAPS_4BIT                                        0x00000001
#define WINEDDPCAPS_8BITENTRIES                                 0x00000002
#define WINEDDPCAPS_8BIT                                        0x00000004
#define WINEDDPCAPS_INITIALIZE                                  0x00000008
#define WINEDDPCAPS_PRIMARYSURFACE                              0x00000010
#define WINEDDPCAPS_PRIMARYSURFACELEFT                          0x00000020
#define WINEDDPCAPS_ALLOW256                                    0x00000040
#define WINEDDPCAPS_VSYNC                                       0x00000080
#define WINEDDPCAPS_1BIT                                        0x00000100
#define WINEDDPCAPS_2BIT                                        0x00000200
#define WINEDDPCAPS_ALPHA                                       0x00000400

typedef struct _WINED3DDISPLAYMODE
{
    UINT Width;
    UINT Height;
    UINT RefreshRate;
    enum wined3d_format_id Format;
} WINED3DDISPLAYMODE;

typedef struct _WINED3DCOLORVALUE
{
    float r;
    float g;
    float b;
    float a;
} WINED3DCOLORVALUE;

typedef struct _WINED3DVECTOR
{
    float x;
    float y;
    float z;
} WINED3DVECTOR;

typedef struct _WINED3DMATRIX
{
    union
    {
        struct
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        } DUMMYSTRUCTNAME;
        float m[4][4];
    } DUMMYUNIONNAME;
} WINED3DMATRIX;

typedef struct _WINED3DLIGHT
{
    WINED3DLIGHTTYPE Type;
    WINED3DCOLORVALUE Diffuse;
    WINED3DCOLORVALUE Specular;
    WINED3DCOLORVALUE Ambient;
    WINED3DVECTOR Position;
    WINED3DVECTOR Direction;
    float Range;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    float Theta;
    float Phi;
} WINED3DLIGHT;

typedef struct _WINED3DMATERIAL
{
    WINED3DCOLORVALUE Diffuse;
    WINED3DCOLORVALUE Ambient;
    WINED3DCOLORVALUE Specular;
    WINED3DCOLORVALUE Emissive;
    float Power;
} WINED3DMATERIAL;

typedef struct _WINED3DVIEWPORT
{
    DWORD X;
    DWORD Y;
    DWORD Width;
    DWORD Height;
    float MinZ;
    float MaxZ;
} WINED3DVIEWPORT;

typedef struct _WINED3DGAMMARAMP
{
    WORD red[256];
    WORD green[256];
    WORD blue[256];
} WINED3DGAMMARAMP;

typedef struct _WINED3DLINEPATTERN
{
    WORD wRepeatFactor;
    WORD wLinePattern;
} WINED3DLINEPATTERN;

typedef struct _WINEDD3DRECTPATCH_INFO
{
    UINT StartVertexOffsetWidth;
    UINT StartVertexOffsetHeight;
    UINT Width;
    UINT Height;
    UINT Stride;
    WINED3DBASISTYPE Basis;
    WINED3DDEGREETYPE Degree;
} WINED3DRECTPATCH_INFO;

typedef struct _WINED3DTRIPATCH_INFO
{
    UINT StartVertexOffset;
    UINT NumVertices;
    WINED3DBASISTYPE Basis;
    WINED3DDEGREETYPE Degree;
} WINED3DTRIPATCH_INFO;

typedef struct _WINED3DADAPTER_IDENTIFIER
{
    char *driver;
    UINT driver_size;
    char *description;
    UINT description_size;
    char *device_name;
    UINT device_name_size;
    LARGE_INTEGER driver_version;
    DWORD vendor_id;
    DWORD device_id;
    DWORD subsystem_id;
    DWORD revision;
    GUID device_identifier;
    DWORD whql_level;
    LUID adapter_luid;
    SIZE_T video_memory;
} WINED3DADAPTER_IDENTIFIER;

typedef struct _WINED3DPRESENT_PARAMETERS
{
    UINT BackBufferWidth;
    UINT BackBufferHeight;
    enum wined3d_format_id BackBufferFormat;
    UINT BackBufferCount;
    WINED3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality;
    WINED3DSWAPEFFECT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    enum wined3d_format_id AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT PresentationInterval;
    BOOL AutoRestoreDisplayMode;
} WINED3DPRESENT_PARAMETERS;

struct wined3d_resource_desc
{
    WINED3DRESOURCETYPE resource_type;
    enum wined3d_format_id format;
    WINED3DMULTISAMPLE_TYPE multisample_type;
    UINT multisample_quality;
    DWORD usage;
    WINED3DPOOL pool;
    UINT width;
    UINT height;
    UINT depth;
    UINT size;
};

typedef struct _WINED3DCLIPSTATUS
{
   DWORD ClipUnion;
   DWORD ClipIntersection;
} WINED3DCLIPSTATUS;

typedef struct _WINED3DVERTEXELEMENT
{
    enum wined3d_format_id format;
    WORD input_slot;
    WORD offset;
    UINT output_slot; /* D3D 8 & 10 */
    BYTE method;
    BYTE usage;
    BYTE usage_idx;
} WINED3DVERTEXELEMENT;

typedef struct _WINED3DDEVICE_CREATION_PARAMETERS
{
    UINT AdapterOrdinal;
    WINED3DDEVTYPE DeviceType;
    HWND hFocusWindow;
    DWORD BehaviorFlags;
} WINED3DDEVICE_CREATION_PARAMETERS;

typedef struct _WINED3DDEVINFO_BANDWIDTHTIMINGS
{
    float MaxBandwidthUtilized;
    float FrontEndUploadMemoryUtilizedPercent;
    float VertexRateUtilizedPercent;
    float TriangleSetupRateUtilizedPercent;
    float FillRateUtilizedPercent;
} WINED3DDEVINFO_BANDWIDTHTIMINGS;

typedef struct _WINED3DDEVINFO_CACHEUTILIZATION
{
    float TextureCacheHitRate;
    float PostTransformVertexCacheHitRate;
} WINED3DDEVINFO_CACHEUTILIZATION;

typedef struct _WINED3DDEVINFO_INTERFACETIMINGS
{
    float WaitingForGPUToUseApplicationResourceTimePercent;
    float WaitingForGPUToAcceptMoreCommandsTimePercent;
    float WaitingForGPUToStayWithinLatencyTimePercent;
    float WaitingForGPUExclusiveResourceTimePercent;
    float WaitingForGPUOtherTimePercent;
} WINED3DDEVINFO_INTERFACETIMINGS;

typedef struct _WINED3DDEVINFO_PIPELINETIMINGS
{
    float VertexProcessingTimePercent;
    float PixelProcessingTimePercent;
    float OtherGPUProcessingTimePercent;
    float GPUIdleTimePercent;
} WINED3DDEVINFO_PIPELINETIMINGS;

typedef struct _WINED3DDEVINFO_STAGETIMINGS
{
    float MemoryProcessingPercent;
    float ComputationProcessingPercent;
} WINED3DDEVINFO_STAGETIMINGS;

typedef struct _WINED3DRASTER_STATUS
{
    BOOL InVBlank;
    UINT ScanLine;
} WINED3DRASTER_STATUS;

typedef struct WINED3DRESOURCESTATS
{
    BOOL bThrashing;
    DWORD ApproxBytesDownloaded;
    DWORD NumEvicts;
    DWORD NumVidCreates;
    DWORD LastPri;
    DWORD NumUsed;
    DWORD NumUsedInVidMem;
    DWORD WorkingSet;
    DWORD WorkingSetBytes;
    DWORD TotalManaged;
    DWORD TotalBytes;
} WINED3DRESOURCESTATS;

typedef struct _WINED3DDEVINFO_RESOURCEMANAGER
{
    WINED3DRESOURCESTATS stats[WINED3DRTYPECOUNT];
} WINED3DDEVINFO_RESOURCEMANAGER;

typedef struct _WINED3DDEVINFO_VERTEXSTATS
{
    DWORD NumRenderedTriangles;
    DWORD NumExtraClippingTriangles;
} WINED3DDEVINFO_VERTEXSTATS;

typedef struct _WINED3DLOCKED_RECT
{
    INT Pitch;
    void *pBits;
} WINED3DLOCKED_RECT;

typedef struct _WINED3DLOCKED_BOX
{
    INT RowPitch;
    INT SlicePitch;
    void *pBits;
} WINED3DLOCKED_BOX;

typedef struct _WINED3DBOX
{
    UINT Left;
    UINT Top;
    UINT Right;
    UINT Bottom;
    UINT Front;
    UINT Back;
} WINED3DBOX;

/*Vertex cache optimization hints.*/
typedef struct WINED3DDEVINFO_VCACHE
{
    DWORD Pattern;      /* Must be a 4 char code FOURCC (e.g. CACH) */
    DWORD OptMethod;    /* 0 to get the longest  strips, 1 vertex cache */
    DWORD CacheSize;    /* Cache size to use (only valid if OptMethod==1) */
    DWORD MagicNumber;  /* Internal for deciding when to restart strips,
                           non user modifiable (only valid if OptMethod==1) */
} WINED3DDEVINFO_VCACHE;

typedef struct WineDirect3DStridedData
{
    enum wined3d_format_id format;   /* Format of the data */
    const BYTE *lpData;     /* Pointer to start of data */
    DWORD dwStride;         /* Stride between occurrences of this data */
} WineDirect3DStridedData;

typedef struct WineDirect3DVertexStridedData
{
    WineDirect3DStridedData position;
    WineDirect3DStridedData normal;
    WineDirect3DStridedData diffuse;
    WineDirect3DStridedData specular;
    WineDirect3DStridedData texCoords[WINED3DDP_MAXTEXCOORD];
    BOOL position_transformed;
} WineDirect3DVertexStridedData;

typedef struct _WINED3DVSHADERCAPS2_0
{
    DWORD Caps;
    INT DynamicFlowControlDepth;
    INT NumTemps;
    INT StaticFlowControlDepth;
} WINED3DVSHADERCAPS2_0;

typedef struct _WINED3DPSHADERCAPS2_0
{
    DWORD Caps;
    INT DynamicFlowControlDepth;
    INT NumTemps;
    INT StaticFlowControlDepth;
    INT NumInstructionSlots;
} WINED3DPSHADERCAPS2_0;

typedef struct _WINEDDCAPS
{
    DWORD Caps;
    DWORD Caps2;
    DWORD CKeyCaps;
    DWORD FXCaps;
    DWORD FXAlphaCaps;
    DWORD PalCaps;
    DWORD SVCaps;
    DWORD SVBCaps;
    DWORD SVBCKeyCaps;
    DWORD SVBFXCaps;
    DWORD VSBCaps;
    DWORD VSBCKeyCaps;
    DWORD VSBFXCaps;
    DWORD SSBCaps;
    DWORD SSBCKeyCaps;
    DWORD SSBFXCaps;
    DWORD ddsCaps;
    DWORD StrideAlign;
} WINEDDCAPS;

typedef struct _WINED3DCAPS
{
    WINED3DDEVTYPE DeviceType;
    UINT AdapterOrdinal;

    DWORD Caps;
    DWORD Caps2;
    DWORD Caps3;
    DWORD PresentationIntervals;

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
    DWORD Reserved5; /* undocumented */

    UINT MasterAdapterOrdinal;
    UINT AdapterOrdinalInGroup;
    UINT NumberOfAdaptersInGroup;
    DWORD DeclTypes;
    DWORD NumSimultaneousRTs;
    DWORD StretchRectFilterCaps;
    WINED3DVSHADERCAPS2_0 VS20Caps;
    WINED3DPSHADERCAPS2_0 PS20Caps;
    DWORD VertexTextureFilterCaps;
    DWORD MaxVShaderInstructionsExecuted;
    DWORD MaxPShaderInstructionsExecuted;
    DWORD MaxVertexShader30InstructionSlots;
    DWORD MaxPixelShader30InstructionSlots;
    DWORD Reserved2; /* Not in the microsoft headers but documented */
    DWORD Reserved3;

    WINEDDCAPS DirectDrawCaps;
} WINED3DCAPS;

/* DirectDraw types */

typedef struct _WINEDDCOLORKEY
{
    DWORD dwColorSpaceLowValue;     /* low boundary of color space that is to
                                     * be treated as Color Key, inclusive */
    DWORD dwColorSpaceHighValue;    /* high boundary of color space that is
                                     * to be treated as Color Key, inclusive */
} WINEDDCOLORKEY,*LPWINEDDCOLORKEY;

typedef struct _WINEDDBLTFX
{
    DWORD dwSize;                                   /* size of structure */
    DWORD dwDDFX;                                   /* FX operations */
    DWORD dwROP;                                    /* Win32 raster operations */
    DWORD dwDDROP;                                  /* Raster operations new for DirectDraw */
    DWORD dwRotationAngle;                          /* Rotation angle for blt */
    DWORD dwZBufferOpCode;                          /* ZBuffer compares */
    DWORD dwZBufferLow;                             /* Low limit of Z buffer */
    DWORD dwZBufferHigh;                            /* High limit of Z buffer */
    DWORD dwZBufferBaseDest;                        /* Destination base value */
    DWORD dwZDestConstBitDepth;                     /* Bit depth used to specify Z constant for destination */
    union
    {
        DWORD dwZDestConst;                         /* Constant to use as Z buffer for dest */
        struct wined3d_surface *lpDDSZBufferDest;   /* Surface to use as Z buffer for dest */
    } DUMMYUNIONNAME1;
    DWORD dwZSrcConstBitDepth;                      /* Bit depth used to specify Z constant for source */
    union
    {
        DWORD dwZSrcConst;                          /* Constant to use as Z buffer for src */
        struct wined3d_surface *lpDDSZBufferSrc;    /* Surface to use as Z buffer for src */
    } DUMMYUNIONNAME2;
    DWORD dwAlphaEdgeBlendBitDepth;                 /* Bit depth used to specify constant for alpha edge blend */
    DWORD dwAlphaEdgeBlend;                         /* Alpha for edge blending */
    DWORD dwReserved;
    DWORD dwAlphaDestConstBitDepth;                 /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD dwAlphaDestConst;                     /* Constant to use as Alpha Channel */
        struct wined3d_surface *lpDDSAlphaDest;     /* Surface to use as Alpha Channel */
    } DUMMYUNIONNAME3;
    DWORD dwAlphaSrcConstBitDepth;                  /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD dwAlphaSrcConst;                      /* Constant to use as Alpha Channel */
        struct wined3d_surface *lpDDSAlphaSrc;      /* Surface to use as Alpha Channel */
    } DUMMYUNIONNAME4;
    union
    {
        DWORD dwFillColor;                          /* color in RGB or Palettized */
        DWORD dwFillDepth;                          /* depth value for z-buffer */
        DWORD dwFillPixel;                          /* pixel val for RGBA or RGBZ */
        struct wined3d_surface *lpDDSPattern;       /* Surface to use as pattern */
    } DUMMYUNIONNAME5;
    WINEDDCOLORKEY ddckDestColorkey;                /* DestColorkey override */
    WINEDDCOLORKEY ddckSrcColorkey;                 /* SrcColorkey override */
} WINEDDBLTFX,*LPWINEDDBLTFX;

typedef struct _WINEDDOVERLAYFX
{
    DWORD dwSize;                                   /* size of structure */
    DWORD dwAlphaEdgeBlendBitDepth;                 /* Bit depth used to specify constant for alpha edge blend */
    DWORD dwAlphaEdgeBlend;                         /* Constant to use as alpha for edge blend */
    DWORD dwReserved;
    DWORD dwAlphaDestConstBitDepth;                 /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD dwAlphaDestConst;                     /* Constant to use as alpha channel for dest */
        struct wined3d_surface *lpDDSAlphaDest;     /* Surface to use as alpha channel for dest */
    } DUMMYUNIONNAME1;
    DWORD dwAlphaSrcConstBitDepth;                  /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD dwAlphaSrcConst;                      /* Constant to use as alpha channel for src */
        struct wined3d_surface *lpDDSAlphaSrc;      /* Surface to use as alpha channel for src */
    } DUMMYUNIONNAME2;
    WINEDDCOLORKEY dckDestColorkey;                 /* DestColorkey override */
    WINEDDCOLORKEY dckSrcColorkey;                  /* SrcColorkey override */
    DWORD dwDDFX;                                   /* Overlay FX */
    DWORD dwFlags;                                  /* flags */
} WINEDDOVERLAYFX;

struct wined3d_buffer_desc
{
    UINT byte_width;
    DWORD usage;
    UINT bind_flags;
    UINT cpu_access_flags;
    UINT misc_flags;
};

struct wined3d_shader_signature_element
{
    const char *semantic_name;
    UINT semantic_idx;
    enum wined3d_sysval_semantic sysval_semantic;
    DWORD component_type;
    UINT register_idx;
    DWORD mask;
};

struct wined3d_shader_signature
{
    UINT element_count;
    struct wined3d_shader_signature_element *elements;
    char *string_data;
};

struct wined3d_parent_ops
{
    void (__stdcall *wined3d_object_destroyed)(void *parent);
};

struct wined3d;
struct wined3d_buffer;
struct wined3d_clipper;
struct wined3d_device;
struct wined3d_palette;
struct wined3d_query;
struct wined3d_rendertarget_view;
struct wined3d_resource;
struct wined3d_shader;
struct wined3d_stateblock;
struct wined3d_surface;
struct wined3d_swapchain;
struct wined3d_texture;
struct wined3d_vertex_declaration;
struct wined3d_volume;

struct wined3d_device_parent
{
    const struct wined3d_device_parent_ops *ops;
};

struct wined3d_device_parent_ops
{
    void (__cdecl *wined3d_device_created)(struct wined3d_device_parent *device_parent, struct wined3d_device *device);
    HRESULT (__cdecl *create_surface)(struct wined3d_device_parent *device_parent, void *container_parent,
            UINT width, UINT height, enum wined3d_format_id format_id, DWORD usage, WINED3DPOOL pool,
            UINT level, WINED3DCUBEMAP_FACES face, struct wined3d_surface **surface);
    HRESULT (__cdecl *create_rendertarget)(struct wined3d_device_parent *device_parent, void *container_parent,
            UINT width, UINT height, enum wined3d_format_id format_id, WINED3DMULTISAMPLE_TYPE multisample_type,
            DWORD multisample_quality, BOOL lockable, struct wined3d_surface **surface);
    HRESULT (__cdecl *create_depth_stencil)(struct wined3d_device_parent *device_parent,
            UINT width, UINT height, enum wined3d_format_id format_id, WINED3DMULTISAMPLE_TYPE multisample_type,
            DWORD multisample_quality, BOOL discard, struct wined3d_surface **surface);
    HRESULT (__cdecl *create_volume)(struct wined3d_device_parent *device_parent, void *container_parent,
            UINT width, UINT height, UINT depth, enum wined3d_format_id format_id, WINED3DPOOL pool, DWORD usage,
            struct wined3d_volume **volume);
    HRESULT (__cdecl *create_swapchain)(struct wined3d_device_parent *device_parent,
            WINED3DPRESENT_PARAMETERS *present_parameters, struct wined3d_swapchain **swapchain);
};

typedef HRESULT (__stdcall *D3DCB_ENUMRESOURCES)(struct wined3d_resource *resource, void *pData);

void __stdcall wined3d_mutex_lock(void);
void __stdcall wined3d_mutex_unlock(void);

HRESULT __cdecl wined3d_check_depth_stencil_match(const struct wined3d *wined3d, UINT adapter_idx,
        WINED3DDEVTYPE device_type, enum wined3d_format_id adapter_format_id,
        enum wined3d_format_id render_target_format_id, enum wined3d_format_id depth_stencil_format_id);
HRESULT __cdecl wined3d_check_device_format(const struct wined3d *wined3d, UINT adaper_idx,
        WINED3DDEVTYPE device_type, enum wined3d_format_id adapter_format_id, DWORD usage,
        WINED3DRESOURCETYPE resource_type, enum wined3d_format_id check_format_id,
        WINED3DSURFTYPE surface_type);
HRESULT __cdecl wined3d_check_device_format_conversion(const struct wined3d *wined3d, UINT adapter_idx,
        WINED3DDEVTYPE device_type, enum wined3d_format_id source_format_id,
        enum wined3d_format_id target_format_id);
HRESULT __cdecl wined3d_check_device_multisample_type(const struct wined3d *wined3d, UINT adapter_idx,
        WINED3DDEVTYPE device_type, enum wined3d_format_id surface_format_id, BOOL windowed,
        WINED3DMULTISAMPLE_TYPE multisample_type, DWORD *quality_levels);
HRESULT __cdecl wined3d_check_device_type(const struct wined3d *wined3d, UINT adapter_idx,
        WINED3DDEVTYPE device_type, enum wined3d_format_id display_format_id,
        enum wined3d_format_id backbuffer_format_id, BOOL windowed);
struct wined3d * __cdecl wined3d_create(UINT dxVersion, void *parent);
ULONG __cdecl wined3d_decref(struct wined3d *wined3d);
HRESULT __cdecl wined3d_enum_adapter_modes(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, UINT mode_idx, WINED3DDISPLAYMODE *mode);
UINT __cdecl wined3d_get_adapter_count(const struct wined3d *wined3d);
HRESULT __cdecl wined3d_get_adapter_display_mode(const struct wined3d *wined3d, UINT adapter_idx,
        WINED3DDISPLAYMODE *mode);
HRESULT __cdecl wined3d_get_adapter_identifier(const struct wined3d *wined3d, UINT adapter_idx,
        DWORD flags, WINED3DADAPTER_IDENTIFIER *identifier);
UINT __cdecl wined3d_get_adapter_mode_count(const struct wined3d *wined3d,
        UINT adapter_idx, enum wined3d_format_id format_id);
HMONITOR __cdecl wined3d_get_adapter_monitor(const struct wined3d *wined3d, UINT adapter_idx);
HRESULT __cdecl wined3d_get_device_caps(const struct wined3d *wined3d, UINT adapter_idx,
        WINED3DDEVTYPE device_type, WINED3DCAPS *caps);
void * __cdecl wined3d_get_parent(const struct wined3d *wined3d);
ULONG __cdecl wined3d_incref(struct wined3d *wined3d);
HRESULT __cdecl wined3d_register_software_device(struct wined3d *wined3d, void *init_function);

HRESULT __cdecl wined3d_buffer_create(struct wined3d_device *device, struct wined3d_buffer_desc *desc,
        const void *data, void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_buffer **buffer);
HRESULT __cdecl wined3d_buffer_create_ib(struct wined3d_device *device, UINT length, DWORD usage, WINED3DPOOL pool,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_buffer **buffer);
HRESULT __cdecl wined3d_buffer_create_vb(struct wined3d_device *device, UINT length, DWORD usage, WINED3DPOOL pool,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_buffer **buffer);
ULONG __cdecl wined3d_buffer_decref(struct wined3d_buffer *buffer);
HRESULT __cdecl wined3d_buffer_free_private_data(struct wined3d_buffer *buffer, REFGUID guid);
void * __cdecl wined3d_buffer_get_parent(const struct wined3d_buffer *buffer);
DWORD __cdecl wined3d_buffer_get_priority(const struct wined3d_buffer *buffer);
HRESULT __cdecl wined3d_buffer_get_private_data(const struct wined3d_buffer *buffer,
        REFGUID guid, void *data, DWORD *data_size);
struct wined3d_resource * __cdecl wined3d_buffer_get_resource(struct wined3d_buffer *buffer);
ULONG __cdecl wined3d_buffer_incref(struct wined3d_buffer *buffer);
HRESULT __cdecl wined3d_buffer_map(struct wined3d_buffer *buffer, UINT offset, UINT size, BYTE **data, DWORD flags);
void  __cdecl wined3d_buffer_preload(struct wined3d_buffer *buffer);
DWORD __cdecl wined3d_buffer_set_priority(struct wined3d_buffer *buffer, DWORD new_priority);
HRESULT __cdecl wined3d_buffer_set_private_data(struct wined3d_buffer *buffer,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags);
void __cdecl wined3d_buffer_unmap(struct wined3d_buffer *buffer);

struct wined3d_clipper * __cdecl wined3d_clipper_create(void);
ULONG __cdecl wined3d_clipper_decref(struct wined3d_clipper *clipper);
HRESULT __cdecl wined3d_clipper_get_clip_list(const struct wined3d_clipper *clipper,
        const RECT *rect, RGNDATA *clip_list, DWORD *clip_list_size);
HRESULT __cdecl wined3d_clipper_get_window(const struct wined3d_clipper *clipper, HWND *hwnd);
ULONG __cdecl wined3d_clipper_incref(struct wined3d_clipper *clipper);
HRESULT __cdecl wined3d_clipper_is_clip_list_changed(const struct wined3d_clipper *clipper, BOOL *changed);
HRESULT __cdecl wined3d_clipper_set_clip_list(struct wined3d_clipper *clipper, const RGNDATA *clip_list, DWORD flags);
HRESULT __cdecl wined3d_clipper_set_window(struct wined3d_clipper *clipper, DWORD flags, HWND hwnd);

HRESULT __cdecl wined3d_device_acquire_focus_window(struct wined3d_device *device, HWND window);
HRESULT __cdecl wined3d_device_begin_scene(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_begin_stateblock(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_clear(struct wined3d_device *device, DWORD rect_count, const RECT *rects, DWORD flags,
        WINED3DCOLOR color, float z, DWORD stencil);
void __cdecl wined3d_device_clear_rendertarget_view(struct wined3d_device *device,
        struct wined3d_rendertarget_view *rendertarget_view, const WINED3DCOLORVALUE *color);
HRESULT __cdecl wined3d_device_color_fill(struct wined3d_device *device, struct wined3d_surface *surface,
        const RECT *rect, const WINED3DCOLORVALUE *color);
HRESULT __cdecl wined3d_device_create(struct wined3d *wined3d, UINT adapter_idx,
        WINED3DDEVTYPE device_type, HWND focus_window, DWORD behaviour_flags,
        struct wined3d_device_parent *device_parent, struct wined3d_device **device);
ULONG __cdecl wined3d_device_decref(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_delete_patch(struct wined3d_device *device, UINT handle);
HRESULT __cdecl wined3d_device_draw_indexed_primitive(struct wined3d_device *device, UINT start_idx, UINT index_count);
HRESULT __cdecl wined3d_device_draw_indexed_primitive_strided(struct wined3d_device *device, UINT index_count,
        const WineDirect3DVertexStridedData *strided_data, UINT vertex_count, const void *index_data,
        enum wined3d_format_id index_data_format_id);
HRESULT __cdecl wined3d_device_draw_indexed_primitive_up(struct wined3d_device *device,
        UINT index_count, const void *index_data, enum wined3d_format_id index_data_format_id,
        const void *stream_data, UINT stream_stride);
HRESULT __cdecl wined3d_device_draw_primitive(struct wined3d_device *device, UINT start_vertex, UINT vertex_count);
HRESULT __cdecl wined3d_device_draw_primitive_strided(struct wined3d_device *device,
        UINT vertex_count, const WineDirect3DVertexStridedData *strided_data);
HRESULT __cdecl wined3d_device_draw_primitive_up(struct wined3d_device *device,
        UINT vertex_count, const void *stream_data, UINT stream_stride);
HRESULT __cdecl wined3d_device_draw_rect_patch(struct wined3d_device *device, UINT handle,
        const float *num_segs, const WINED3DRECTPATCH_INFO *rect_patch_info);
HRESULT __cdecl wined3d_device_draw_tri_patch(struct wined3d_device *device, UINT handle,
        const float *num_segs, const WINED3DTRIPATCH_INFO *tri_patch_info);
HRESULT __cdecl wined3d_device_end_scene(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_end_stateblock(struct wined3d_device *device, struct wined3d_stateblock **stateblock);
HRESULT __cdecl wined3d_device_enum_resources(struct wined3d_device *device, D3DCB_ENUMRESOURCES callback, void *data);
HRESULT __cdecl wined3d_device_evict_managed_resources(struct wined3d_device *device);
UINT __cdecl wined3d_device_get_available_texture_mem(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_get_back_buffer(struct wined3d_device *device, UINT swapchain_idx,
        UINT backbuffer_idx, WINED3DBACKBUFFER_TYPE backbuffer_type, struct wined3d_surface **backbuffer);
INT __cdecl wined3d_device_get_base_vertex_index(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_get_clip_plane(struct wined3d_device *device, UINT plane_idx, float *plane);
HRESULT __cdecl wined3d_device_get_clip_status(struct wined3d_device *device, WINED3DCLIPSTATUS *clip_status);
HRESULT __cdecl wined3d_device_get_creation_parameters(struct wined3d_device *device,
        WINED3DDEVICE_CREATION_PARAMETERS *creation_parameters);
HRESULT __cdecl wined3d_device_get_current_texture_palette(struct wined3d_device *device, UINT *palette_idx);
HRESULT __cdecl wined3d_device_get_depth_stencil(struct wined3d_device *device,
        struct wined3d_surface **depth_stencil);
HRESULT __cdecl wined3d_device_get_device_caps(struct wined3d_device *device, WINED3DCAPS *caps);
HRESULT __cdecl wined3d_device_get_display_mode(struct wined3d_device *device,
        UINT swapchain_idx, WINED3DDISPLAYMODE *mode);
HRESULT __cdecl wined3d_device_get_front_buffer_data(struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_surface *dst_surface);
void __cdecl wined3d_device_get_gamma_ramp(struct wined3d_device *device, UINT swapchain_idx, WINED3DGAMMARAMP *ramp);
HRESULT __cdecl wined3d_device_get_index_buffer(struct wined3d_device *device, struct wined3d_buffer **index_buffer);
HRESULT __cdecl wined3d_device_get_light(struct wined3d_device *device, UINT light_idx, WINED3DLIGHT *light);
HRESULT __cdecl wined3d_device_get_light_enable(struct wined3d_device *device, UINT light_idx, BOOL *enable);
HRESULT __cdecl wined3d_device_get_material(struct wined3d_device *device, WINED3DMATERIAL *material);
float __cdecl wined3d_device_get_npatch_mode(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_get_palette_entries(struct wined3d_device *device,
        UINT palette_idx, PALETTEENTRY *entries);
struct wined3d_shader * __cdecl wined3d_device_get_pixel_shader(struct wined3d_device *device);
void __cdecl wined3d_device_get_primitive_type(struct wined3d_device *device, WINED3DPRIMITIVETYPE *primitive_topology);
HRESULT __cdecl wined3d_device_get_ps_consts_b(struct wined3d_device *device,
        UINT start_register, BOOL *constants, UINT bool_count);
HRESULT __cdecl wined3d_device_get_ps_consts_f(struct wined3d_device *device,
        UINT start_register, float *constants, UINT vector4f_count);
HRESULT __cdecl wined3d_device_get_ps_consts_i(struct wined3d_device *device,
        UINT start_register, int *constants, UINT vector4i_count);
HRESULT __cdecl wined3d_device_get_raster_status(struct wined3d_device *device,
        UINT swapchain_idx, WINED3DRASTER_STATUS *raster_status);
HRESULT __cdecl wined3d_device_get_render_state(struct wined3d_device *device,
        WINED3DRENDERSTATETYPE state, DWORD *value);
HRESULT __cdecl wined3d_device_get_render_target(struct wined3d_device *device,
        UINT render_target_idx, struct wined3d_surface **render_target);
HRESULT __cdecl wined3d_device_get_sampler_state(struct wined3d_device *device,
        UINT sampler_idx, WINED3DSAMPLERSTATETYPE state, DWORD *value);
HRESULT __cdecl wined3d_device_get_scissor_rect(struct wined3d_device *device, RECT *rect);
BOOL __cdecl wined3d_device_get_software_vertex_processing(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_get_stream_source(struct wined3d_device *device,
        UINT stream_idx, struct wined3d_buffer **buffer, UINT *offset, UINT *stride);
HRESULT __cdecl wined3d_device_get_stream_source_freq(struct wined3d_device *device, UINT stream_idx, UINT *divider);
HRESULT __cdecl wined3d_device_get_surface_from_dc(struct wined3d_device *device,
        HDC dc, struct wined3d_surface **surface);
HRESULT __cdecl wined3d_device_get_swapchain(struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_swapchain **swapchain);
UINT __cdecl wined3d_device_get_swapchain_count(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_get_texture(struct wined3d_device *device,
        UINT stage, struct wined3d_texture **texture);
HRESULT __cdecl wined3d_device_get_texture_stage_state(struct wined3d_device *device,
        UINT stage, WINED3DTEXTURESTAGESTATETYPE state, DWORD *value);
HRESULT __cdecl wined3d_device_get_transform(struct wined3d_device *device,
        WINED3DTRANSFORMSTATETYPE state, WINED3DMATRIX *matrix);
HRESULT __cdecl wined3d_device_get_vertex_declaration(struct wined3d_device *device,
        struct wined3d_vertex_declaration **declaration);
struct wined3d_shader * __cdecl wined3d_device_get_vertex_shader(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_get_viewport(struct wined3d_device *device, WINED3DVIEWPORT *viewport);
HRESULT __cdecl wined3d_device_get_vs_consts_b(struct wined3d_device *device,
        UINT start_register, BOOL *constants, UINT bool_count);
HRESULT __cdecl wined3d_device_get_vs_consts_f(struct wined3d_device *device,
        UINT start_register, float *constants, UINT vector4f_count);
HRESULT __cdecl wined3d_device_get_vs_consts_i(struct wined3d_device *device,
        UINT start_register, int *constants, UINT vector4i_count);
HRESULT __cdecl wined3d_device_get_wined3d(struct wined3d_device *device, struct wined3d **d3d);
ULONG __cdecl wined3d_device_incref(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_init_3d(struct wined3d_device *device, WINED3DPRESENT_PARAMETERS *present_parameters);
HRESULT __cdecl wined3d_device_init_gdi(struct wined3d_device *device, WINED3DPRESENT_PARAMETERS *present_parameters);
HRESULT __cdecl wined3d_device_multiply_transform(struct wined3d_device *device,
        WINED3DTRANSFORMSTATETYPE state, const WINED3DMATRIX *matrix);
HRESULT __cdecl wined3d_device_present(struct wined3d_device *device, const RECT *src_rect,
        const RECT *dst_rect, HWND dst_window_override, const RGNDATA *dirty_region);
HRESULT __cdecl wined3d_device_process_vertices(struct wined3d_device *device,
        UINT src_start_idx, UINT dst_idx, UINT vertex_count, struct wined3d_buffer *dst_buffer,
        struct wined3d_vertex_declaration *declaration, DWORD flags, DWORD dst_fvf);
void __cdecl wined3d_device_release_focus_window(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_reset(struct wined3d_device *device, WINED3DPRESENT_PARAMETERS *present_parameters);
void __cdecl wined3d_device_restore_fullscreen_window(struct wined3d_device *device, HWND window);
HRESULT __cdecl wined3d_device_set_base_vertex_index(struct wined3d_device *device, INT base_index);
HRESULT __cdecl wined3d_device_set_clip_plane(struct wined3d_device *device, UINT plane_idx, const float *plane);
HRESULT __cdecl wined3d_device_set_clip_status(struct wined3d_device *device, const WINED3DCLIPSTATUS *clip_status);
HRESULT __cdecl wined3d_device_set_current_texture_palette(struct wined3d_device *device, UINT palette_idx);
void __cdecl wined3d_device_set_cursor_position(struct wined3d_device *device,
        int x_screen_space, int y_screen_space, DWORD flags);
HRESULT __cdecl wined3d_device_set_cursor_properties(struct wined3d_device *device,
        UINT x_hotspot, UINT y_hotspot, struct wined3d_surface *cursor_surface);
HRESULT __cdecl wined3d_device_set_depth_stencil(struct wined3d_device *device, struct wined3d_surface *depth_stencil);
HRESULT __cdecl wined3d_device_set_dialog_box_mode(struct wined3d_device *device, BOOL enable_dialogs);
HRESULT __cdecl wined3d_device_set_display_mode(struct wined3d_device *device,
        UINT swapchain_idx, const WINED3DDISPLAYMODE *mode);
void __cdecl wined3d_device_set_gamma_ramp(struct wined3d_device *device,
        UINT swapchain_idx, DWORD flags, const WINED3DGAMMARAMP *ramp);
HRESULT __cdecl wined3d_device_set_index_buffer(struct wined3d_device *device,
        struct wined3d_buffer *index_buffer, enum wined3d_format_id format_id);
HRESULT __cdecl wined3d_device_set_light(struct wined3d_device *device, UINT light_idx, const WINED3DLIGHT *light);
HRESULT __cdecl wined3d_device_set_light_enable(struct wined3d_device *device, UINT light_idx, BOOL enable);
HRESULT __cdecl wined3d_device_set_material(struct wined3d_device *device, const WINED3DMATERIAL *material);
void __cdecl wined3d_device_set_multithreaded(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_set_npatch_mode(struct wined3d_device *device, float segments);
HRESULT __cdecl wined3d_device_set_palette_entries(struct wined3d_device *device,
        UINT palette_idx, const PALETTEENTRY *entries);
HRESULT __cdecl wined3d_device_set_pixel_shader(struct wined3d_device *device, struct wined3d_shader *shader);
void __cdecl wined3d_device_set_primitive_type(struct wined3d_device *device, WINED3DPRIMITIVETYPE primitive_topology);
HRESULT __cdecl wined3d_device_set_ps_consts_b(struct wined3d_device *device,
        UINT start_register, const BOOL *constants, UINT bool_count);
HRESULT __cdecl wined3d_device_set_ps_consts_f(struct wined3d_device *device,
        UINT start_register, const float *constants, UINT vector4f_count);
HRESULT __cdecl wined3d_device_set_ps_consts_i(struct wined3d_device *device,
        UINT start_register, const int *constants, UINT vector4i_count);
HRESULT __cdecl wined3d_device_set_render_state(struct wined3d_device *device,
        WINED3DRENDERSTATETYPE state, DWORD value);
HRESULT __cdecl wined3d_device_set_render_target(struct wined3d_device *device,
        UINT render_target_idx, struct wined3d_surface *render_target, BOOL set_viewport);
HRESULT __cdecl wined3d_device_set_sampler_state(struct wined3d_device *device,
        UINT sampler_idx, WINED3DSAMPLERSTATETYPE state, DWORD value);
HRESULT __cdecl wined3d_device_set_scissor_rect(struct wined3d_device *device, const RECT *rect);
HRESULT __cdecl wined3d_device_set_software_vertex_processing(struct wined3d_device *device, BOOL software);
HRESULT __cdecl wined3d_device_set_stream_source(struct wined3d_device *device,
        UINT stream_idx, struct wined3d_buffer *buffer, UINT offset, UINT stride);
HRESULT __cdecl wined3d_device_set_stream_source_freq(struct wined3d_device *device, UINT stream_idx, UINT divider);
HRESULT __cdecl wined3d_device_set_texture(struct wined3d_device *device, UINT stage, struct wined3d_texture *texture);
HRESULT __cdecl wined3d_device_set_texture_stage_state(struct wined3d_device *device,
        UINT stage, WINED3DTEXTURESTAGESTATETYPE state, DWORD value);
HRESULT __cdecl wined3d_device_set_transform(struct wined3d_device *device,
        WINED3DTRANSFORMSTATETYPE state, const WINED3DMATRIX *matrix);
HRESULT __cdecl wined3d_device_set_vertex_declaration(struct wined3d_device *device,
        struct wined3d_vertex_declaration *declaration);
HRESULT __cdecl wined3d_device_set_vertex_shader(struct wined3d_device *device, struct wined3d_shader *shader);
HRESULT __cdecl wined3d_device_set_viewport(struct wined3d_device *device, const WINED3DVIEWPORT *viewport);
HRESULT __cdecl wined3d_device_set_vs_consts_b(struct wined3d_device *device,
        UINT start_register, const BOOL *constants, UINT bool_count);
HRESULT __cdecl wined3d_device_set_vs_consts_f(struct wined3d_device *device,
        UINT start_register, const float *constants, UINT vector4f_count);
HRESULT __cdecl wined3d_device_set_vs_consts_i(struct wined3d_device *device,
        UINT start_register, const int *constants, UINT vector4i_count);
void __cdecl wined3d_device_setup_fullscreen_window(struct wined3d_device *device, HWND window, UINT w, UINT h);
BOOL __cdecl wined3d_device_show_cursor(struct wined3d_device *device, BOOL show);
HRESULT __cdecl wined3d_device_uninit_3d(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_uninit_gdi(struct wined3d_device *device);
HRESULT __cdecl wined3d_device_update_surface(struct wined3d_device *device, struct wined3d_surface *src_surface,
        const RECT *src_rect, struct wined3d_surface *dst_surface, const POINT *dst_point);
HRESULT __cdecl wined3d_device_update_texture(struct wined3d_device *device,
        struct wined3d_texture *src_texture, struct wined3d_texture *dst_texture);
HRESULT __cdecl wined3d_device_validate_device(struct wined3d_device *device, DWORD *num_passes);

HRESULT __cdecl wined3d_palette_create(struct wined3d_device *device, DWORD flags,
        const PALETTEENTRY *entries, void *parent, struct wined3d_palette **palette);
ULONG __cdecl wined3d_palette_decref(struct wined3d_palette *palette);
HRESULT __cdecl wined3d_palette_get_entries(const struct wined3d_palette *palette,
        DWORD flags, DWORD start, DWORD count, PALETTEENTRY *entries);
DWORD __cdecl wined3d_palette_get_flags(const struct wined3d_palette *palette);
void * __cdecl wined3d_palette_get_parent(const struct wined3d_palette *palette);
ULONG __cdecl wined3d_palette_incref(struct wined3d_palette *palette);
HRESULT __cdecl wined3d_palette_set_entries(struct wined3d_palette *palette,
        DWORD flags, DWORD start, DWORD count, const PALETTEENTRY *entries);

HRESULT __cdecl wined3d_query_create(struct wined3d_device *device,
        WINED3DQUERYTYPE type, struct wined3d_query **query);
ULONG __cdecl wined3d_query_decref(struct wined3d_query *query);
HRESULT __cdecl wined3d_query_get_data(struct wined3d_query *query, void *data, UINT data_size, DWORD flags);
UINT __cdecl wined3d_query_get_data_size(const struct wined3d_query *query);
WINED3DQUERYTYPE __cdecl wined3d_query_get_type(const struct wined3d_query *query);
ULONG __cdecl wined3d_query_incref(struct wined3d_query *query);
HRESULT __cdecl wined3d_query_issue(struct wined3d_query *query, DWORD flags);

void __cdecl wined3d_resource_get_desc(const struct wined3d_resource *resource,
        struct wined3d_resource_desc *desc);
void * __cdecl wined3d_resource_get_parent(const struct wined3d_resource *resource);

HRESULT __cdecl wined3d_rendertarget_view_create(struct wined3d_resource *resource,
        void *parent, struct wined3d_rendertarget_view **rendertarget_view);
ULONG __cdecl wined3d_rendertarget_view_decref(struct wined3d_rendertarget_view *view);
void * __cdecl wined3d_rendertarget_view_get_parent(const struct wined3d_rendertarget_view *view);
struct wined3d_resource * __cdecl wined3d_rendertarget_view_get_resource(const struct wined3d_rendertarget_view *view);
ULONG __cdecl wined3d_rendertarget_view_incref(struct wined3d_rendertarget_view *view);

HRESULT __cdecl wined3d_shader_create_gs(struct wined3d_device *device, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
HRESULT __cdecl wined3d_shader_create_ps(struct wined3d_device *device, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
HRESULT __cdecl wined3d_shader_create_vs(struct wined3d_device *device, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader);
ULONG __cdecl wined3d_shader_decref(struct wined3d_shader *shader);
HRESULT __cdecl wined3d_shader_get_byte_code(const struct wined3d_shader *shader,
        void *byte_code, UINT *byte_code_size);
void * __cdecl wined3d_shader_get_parent(const struct wined3d_shader *shader);
ULONG __cdecl wined3d_shader_incref(struct wined3d_shader *shader);
HRESULT __cdecl wined3d_shader_set_local_constants_float(struct wined3d_shader *shader,
        UINT start_idx, const float *src_data, UINT vector4f_count);

HRESULT __cdecl wined3d_stateblock_apply(const struct wined3d_stateblock *stateblock);
HRESULT __cdecl wined3d_stateblock_capture(struct wined3d_stateblock *stateblock);
HRESULT __cdecl wined3d_stateblock_create(struct wined3d_device *device,
        WINED3DSTATEBLOCKTYPE type, struct wined3d_stateblock **stateblock);
ULONG __cdecl wined3d_stateblock_decref(struct wined3d_stateblock *stateblock);
ULONG __cdecl wined3d_stateblock_incref(struct wined3d_stateblock *stateblock);

HRESULT __cdecl wined3d_surface_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const WINEDDBLTFX *blt_fx, WINED3DTEXTUREFILTERTYPE filter);
HRESULT __cdecl wined3d_surface_bltfast(struct wined3d_surface *dst_surface, DWORD dst_x, DWORD dst_y,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD trans);
HRESULT __cdecl wined3d_surface_create(struct wined3d_device *device, UINT width, UINT height,
        enum wined3d_format_id format_id, BOOL lockable, BOOL discard, UINT level, DWORD usage, WINED3DPOOL pool,
        WINED3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality, WINED3DSURFTYPE surface_type,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_surface **surface);
ULONG __cdecl wined3d_surface_decref(struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_flip(struct wined3d_surface *surface, struct wined3d_surface *override, DWORD flags);
HRESULT __cdecl wined3d_surface_free_private_data(struct wined3d_surface *surface, REFGUID guid);
HRESULT __cdecl wined3d_surface_get_blt_status(const struct wined3d_surface *surface, DWORD flags);
struct wined3d_clipper * __cdecl wined3d_surface_get_clipper(const struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_get_flip_status(const struct wined3d_surface *surface, DWORD flags);
HRESULT __cdecl wined3d_surface_get_overlay_position(const struct wined3d_surface *surface, LONG *x, LONG *y);
struct wined3d_palette * __cdecl wined3d_surface_get_palette(const struct wined3d_surface *surface);
void * __cdecl wined3d_surface_get_parent(const struct wined3d_surface *surface);
DWORD __cdecl wined3d_surface_get_pitch(const struct wined3d_surface *surface);
DWORD __cdecl wined3d_surface_get_priority(const struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_get_private_data(const struct wined3d_surface *surface,
        REFGUID guid, void *data, DWORD *data_size);
struct wined3d_resource * __cdecl wined3d_surface_get_resource(struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_getdc(struct wined3d_surface *surface, HDC *dc);
ULONG __cdecl wined3d_surface_incref(struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_is_lost(const struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_map(struct wined3d_surface *surface,
        WINED3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags);
void __cdecl wined3d_surface_preload(struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_releasedc(struct wined3d_surface *surface, HDC dc);
HRESULT __cdecl wined3d_surface_restore(struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_set_clipper(struct wined3d_surface *surface, struct wined3d_clipper *clipper);
HRESULT __cdecl wined3d_surface_set_color_key(struct wined3d_surface *surface,
        DWORD flags, const WINEDDCOLORKEY *color_key);
HRESULT __cdecl wined3d_surface_set_format(struct wined3d_surface *surface, enum wined3d_format_id format_id);
HRESULT __cdecl wined3d_surface_set_mem(struct wined3d_surface *surface, void *mem);
HRESULT __cdecl wined3d_surface_set_overlay_position(struct wined3d_surface *surface, LONG x, LONG y);
HRESULT __cdecl wined3d_surface_set_palette(struct wined3d_surface *surface, struct wined3d_palette *palette);
DWORD __cdecl wined3d_surface_set_priority(struct wined3d_surface *surface, DWORD new_priority);
HRESULT __cdecl wined3d_surface_set_private_data(struct wined3d_surface *surface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags);
HRESULT __cdecl wined3d_surface_unmap(struct wined3d_surface *surface);
HRESULT __cdecl wined3d_surface_update_overlay(struct wined3d_surface *surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const RECT *dst_rect, DWORD flags, const WINEDDOVERLAYFX *fx);
HRESULT __cdecl wined3d_surface_update_overlay_z_order(struct wined3d_surface *surface,
        DWORD flags, struct wined3d_surface *ref);

HRESULT __cdecl wined3d_swapchain_create(struct wined3d_device *device,
        WINED3DPRESENT_PARAMETERS *present_parameters, WINED3DSURFTYPE surface_type, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_swapchain **swapchain);
ULONG __cdecl wined3d_swapchain_decref(struct wined3d_swapchain *swapchain);
HRESULT __cdecl wined3d_swapchain_get_back_buffer(const struct wined3d_swapchain *swapchain,
        UINT backbuffer_idx, WINED3DBACKBUFFER_TYPE backbuffer_type, struct wined3d_surface **backbuffer);
struct wined3d_device * __cdecl wined3d_swapchain_get_device(const struct wined3d_swapchain *swapchain);
HRESULT __cdecl wined3d_swapchain_get_display_mode(const struct wined3d_swapchain *swapchain,
        WINED3DDISPLAYMODE *mode);
HRESULT __cdecl wined3d_swapchain_get_front_buffer_data(const struct wined3d_swapchain *swapchain,
        struct wined3d_surface *dst_surface);
HRESULT __cdecl wined3d_swapchain_get_gamma_ramp(const struct wined3d_swapchain *swapchain,
        WINED3DGAMMARAMP *ramp);
void * __cdecl wined3d_swapchain_get_parent(const struct wined3d_swapchain *swapchain);
HRESULT __cdecl wined3d_swapchain_get_present_parameters(const struct wined3d_swapchain *swapchain,
        WINED3DPRESENT_PARAMETERS *present_parameters);
HRESULT __cdecl wined3d_swapchain_get_raster_status(const struct wined3d_swapchain *swapchain,
        WINED3DRASTER_STATUS *raster_status);
ULONG __cdecl wined3d_swapchain_incref(struct wined3d_swapchain *swapchain);
HRESULT __cdecl wined3d_swapchain_present(struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags);
HRESULT __cdecl wined3d_swapchain_set_gamma_ramp(const struct wined3d_swapchain *swapchain,
        DWORD flags, const WINED3DGAMMARAMP *ramp);
HRESULT __cdecl wined3d_swapchain_set_window(struct wined3d_swapchain *swapchain, HWND window);

HRESULT __cdecl wined3d_texture_add_dirty_region(struct wined3d_texture *texture,
        UINT layer, const WINED3DBOX *dirty_region);
HRESULT __cdecl wined3d_texture_create_2d(struct wined3d_device *device, UINT width, UINT height,
        UINT level_count, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture);
HRESULT __cdecl wined3d_texture_create_3d(struct wined3d_device *device, UINT width, UINT height, UINT depth,
        UINT level_count, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture);
HRESULT __cdecl wined3d_texture_create_cube(struct wined3d_device *device, UINT edge_length,
        UINT level_count, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture);
ULONG __cdecl wined3d_texture_decref(struct wined3d_texture *texture);
HRESULT __cdecl wined3d_texture_free_private_data(struct wined3d_texture *texture, REFGUID guid);
void __cdecl wined3d_texture_generate_mipmaps(struct wined3d_texture *texture);
WINED3DTEXTUREFILTERTYPE __cdecl wined3d_texture_get_autogen_filter_type(const struct wined3d_texture *texture);
DWORD __cdecl wined3d_texture_get_level_count(const struct wined3d_texture *texture);
DWORD __cdecl wined3d_texture_get_lod(const struct wined3d_texture *texture);
void * __cdecl wined3d_texture_get_parent(const struct wined3d_texture *texture);
DWORD __cdecl wined3d_texture_get_priority(const struct wined3d_texture *texture);
HRESULT __cdecl wined3d_texture_get_private_data(const struct wined3d_texture *texture,
        REFGUID guid, void *data, DWORD *data_size);
struct wined3d_resource * __cdecl wined3d_texture_get_sub_resource(struct wined3d_texture *texture,
        UINT sub_resource_idx);
WINED3DRESOURCETYPE __cdecl wined3d_texture_get_type(const struct wined3d_texture *texture);
ULONG __cdecl wined3d_texture_incref(struct wined3d_texture *texture);
void __cdecl wined3d_texture_preload(struct wined3d_texture *texture);
HRESULT __cdecl wined3d_texture_set_autogen_filter_type(struct wined3d_texture *texture,
        WINED3DTEXTUREFILTERTYPE filter_type);
DWORD __cdecl wined3d_texture_set_lod(struct wined3d_texture *texture, DWORD lod);
DWORD __cdecl wined3d_texture_set_priority(struct wined3d_texture *texture, DWORD priority);
HRESULT __cdecl wined3d_texture_set_private_data(struct wined3d_texture *texture,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags);

HRESULT __cdecl wined3d_vertex_declaration_create(struct wined3d_device *device,
        const WINED3DVERTEXELEMENT *elements, UINT element_count, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_vertex_declaration **declaration);
HRESULT __cdecl wined3d_vertex_declaration_create_from_fvf(struct wined3d_device *device,
        DWORD fvf, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_vertex_declaration **declaration);
ULONG __cdecl wined3d_vertex_declaration_decref(struct wined3d_vertex_declaration *declaration);
void * __cdecl wined3d_vertex_declaration_get_parent(const struct wined3d_vertex_declaration *declaration);
ULONG __cdecl wined3d_vertex_declaration_incref(struct wined3d_vertex_declaration *declaration);

HRESULT __cdecl wined3d_volume_create(struct wined3d_device *device, UINT width, UINT height, UINT depth,
        DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_volume **volume);
ULONG __cdecl wined3d_volume_decref(struct wined3d_volume *volume);
HRESULT __cdecl wined3d_volume_free_private_data(struct wined3d_volume *volume, REFGUID guid);
struct wined3d_volume * __cdecl wined3d_volume_from_resource(struct wined3d_resource *resource);
void * __cdecl wined3d_volume_get_parent(const struct wined3d_volume *volume);
DWORD __cdecl wined3d_volume_get_priority(const struct wined3d_volume *volume);
HRESULT __cdecl wined3d_volume_get_private_data(const struct wined3d_volume *volume,
        REFGUID guid, void *data, DWORD *data_size);
struct wined3d_resource * __cdecl wined3d_volume_get_resource(struct wined3d_volume *volume);
ULONG __cdecl wined3d_volume_incref(struct wined3d_volume *volume);
HRESULT __cdecl wined3d_volume_map(struct wined3d_volume *volume,
        WINED3DLOCKED_BOX *locked_box, const WINED3DBOX *box, DWORD flags);
void __cdecl wined3d_volume_preload(struct wined3d_volume *volume);
DWORD __cdecl wined3d_volume_set_priority(struct wined3d_volume *volume, DWORD new_priority);
HRESULT __cdecl wined3d_volume_set_private_data(struct wined3d_volume *volume,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags);
HRESULT __cdecl wined3d_volume_unmap(struct wined3d_volume *volume);

#endif /* __WINE_WINED3D_H */
