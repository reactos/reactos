/*
 * Direct3D wine types include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Stefan Dösinger
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

#ifndef __WINE_WINED3D_TYPES_H
#define __WINE_WINED3D_TYPES_H

typedef DWORD WINED3DCOLOR;

typedef enum _WINED3DLIGHTTYPE {
  WINED3DLIGHT_POINT          = 1,
  WINED3DLIGHT_SPOT           = 2,
  WINED3DLIGHT_DIRECTIONAL    = 3,
  WINED3DLIGHT_PARALLELPOINT  = 4, /* D3D7 */
  WINED3DLIGHT_GLSPOT         = 5, /* D3D7 */
  WINED3DLIGHT_FORCE_DWORD    = 0x7fffffff
} WINED3DLIGHTTYPE;

typedef enum _WINED3DPRIMITIVETYPE {
    WINED3DPT_POINTLIST       = 1,
    WINED3DPT_LINELIST        = 2,
    WINED3DPT_LINESTRIP       = 3,
    WINED3DPT_TRIANGLELIST    = 4,
    WINED3DPT_TRIANGLESTRIP   = 5,
    WINED3DPT_TRIANGLEFAN     = 6,

    WINED3DPT_FORCE_DWORD     = 0x7fffffff
} WINED3DPRIMITIVETYPE;

typedef struct _WINED3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
} WINED3DCOLORVALUE;

typedef struct _WINED3DVECTOR {
    float x;
    float y;
    float z;
} WINED3DVECTOR;

typedef struct _WINED3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        } DUMMYSTRUCTNAME;
        float m[4][4];
    } DUMMYUNIONNAME;
} WINED3DMATRIX;

typedef struct _WINED3DRECT {
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
} WINED3DRECT;

typedef struct _WINED3DLIGHT {
    WINED3DLIGHTTYPE    Type;
    WINED3DCOLORVALUE   Diffuse;
    WINED3DCOLORVALUE   Specular;
    WINED3DCOLORVALUE   Ambient;
    WINED3DVECTOR       Position;
    WINED3DVECTOR       Direction;
    float               Range;
    float               Falloff;
    float               Attenuation0;
    float               Attenuation1;
    float               Attenuation2;
    float               Theta;
    float               Phi;
} WINED3DLIGHT;

typedef struct _WINED3DMATERIAL {
    WINED3DCOLORVALUE   Diffuse;
    WINED3DCOLORVALUE   Ambient;
    WINED3DCOLORVALUE   Specular;
    WINED3DCOLORVALUE   Emissive;
    float           Power;
} WINED3DMATERIAL;

typedef struct _WINED3DVIEWPORT {
    DWORD       X;
    DWORD       Y;
    DWORD       Width;
    DWORD       Height;
    float       MinZ;
    float       MaxZ;
} WINED3DVIEWPORT;

typedef struct _WINED3DGAMMARAMP {
    WORD                red  [256];
    WORD                green[256];
    WORD                blue [256];
} WINED3DGAMMARAMP;

typedef struct _WINED3DLINEPATTERN {
    WORD    wRepeatFactor;
    WORD    wLinePattern;
} WINED3DLINEPATTERN;

#define WINED3D_VSHADER_MAX_CONSTANTS 96
#define WINED3D_PSHADER_MAX_CONSTANTS 32

typedef struct _WINED3DVECTOR_3 {
    float x;
    float y;
    float z;
} WINED3DVECTOR_3;

typedef struct _WINED3DVECTOR_4 {
    float x;
    float y;
    float z;
    float w;
} WINED3DVECTOR_4;

typedef struct WINED3DSHADERVECTOR {
  float x;
  float y;
  float z;
  float w;
} WINED3DSHADERVECTOR;

typedef struct WINED3DSHADERSCALAR {
  float x;
} WINED3DSHADERSCALAR;

typedef WINED3DSHADERVECTOR WINEVSHADERCONSTANTS8[WINED3D_VSHADER_MAX_CONSTANTS];

typedef struct VSHADERDATA {
  /** Run Time Shader Function Constants */
  /*D3DXBUFFER* constants;*/
  WINEVSHADERCONSTANTS8 C;
  /** Shader Code as char ... */
  CONST DWORD* code;
  UINT codeLength;
} VSHADERDATA;

/** temporary here waiting for buffer code */
typedef struct VSHADERINPUTDATA {
  WINED3DSHADERVECTOR V[17];
} WINEVSHADERINPUTDATA;

/** temporary here waiting for buffer code */
typedef struct VSHADEROUTPUTDATA {
  WINED3DSHADERVECTOR oPos;
  WINED3DSHADERVECTOR oD[2];
  WINED3DSHADERVECTOR oT[8];
  WINED3DSHADERVECTOR oFog;
  WINED3DSHADERVECTOR oPts;
} WINEVSHADEROUTPUTDATA;

typedef WINED3DSHADERVECTOR WINEPSHADERCONSTANTS8[WINED3D_PSHADER_MAX_CONSTANTS];

typedef struct PSHADERDATA {
  /** Run Time Shader Function Constants */
  /*D3DXBUFFER* constants;*/
  WINEPSHADERCONSTANTS8 C;
  /** Shader Code as char ... */
  CONST DWORD* code;
  UINT codeLength;
} PSHADERDATA;

/** temporary here waiting for buffer code */
typedef struct PSHADERINPUTDATA {
  WINED3DSHADERVECTOR V[2];
  WINED3DSHADERVECTOR T[8];
  WINED3DSHADERVECTOR S[16];
  /*D3DSHADERVECTOR R[12];*/
} WINEPSHADERINPUTDATA;

/** temporary here waiting for buffer code */
typedef struct PSHADEROUTPUTDATA {
  WINED3DSHADERVECTOR oC[4];
  WINED3DSHADERVECTOR oDepth;
} WINEPSHADEROUTPUTDATA;

/*****************************************************************************
 * WineD3D Structures to be used when d3d8 and d3d9 are incompatible
 */


typedef enum _WINED3DDEVTYPE {
    WINED3DDEVTYPE_HAL         = 1,
    WINED3DDEVTYPE_REF         = 2,
    WINED3DDEVTYPE_SW          = 3,
    WINED3DDEVTYPE_NULLREF     = 4,

    WINED3DDEVTYPE_FORCE_DWORD = 0xffffffff
} WINED3DDEVTYPE;

typedef enum _WINED3DDEGREETYPE {
    WINED3DDEGREE_LINEAR      = 1,
    WINED3DDEGREE_QUADRATIC   = 2,
    WINED3DDEGREE_CUBIC       = 3,
    WINED3DDEGREE_QUINTIC     = 5,

    WINED3DDEGREE_FORCE_DWORD   = 0x7fffffff
} WINED3DDEGREETYPE;

#define WINEMAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

typedef enum _WINED3DFORMAT {
    WINED3DFMT_UNKNOWN              =   0,

    WINED3DFMT_R8G8B8               =  20,
    WINED3DFMT_A8R8G8B8             =  21,
    WINED3DFMT_X8R8G8B8             =  22,
    WINED3DFMT_R5G6B5               =  23,
    WINED3DFMT_X1R5G5B5             =  24,
    WINED3DFMT_A1R5G5B5             =  25,
    WINED3DFMT_A4R4G4B4             =  26,
    WINED3DFMT_R3G3B2               =  27,
    WINED3DFMT_A8                   =  28,
    WINED3DFMT_A8R3G3B2             =  29,
    WINED3DFMT_X4R4G4B4             =  30,
    WINED3DFMT_A2B10G10R10          =  31,
    WINED3DFMT_A8B8G8R8             =  32,
    WINED3DFMT_X8B8G8R8             =  33,
    WINED3DFMT_G16R16               =  34,
    WINED3DFMT_A2R10G10B10          =  35,
    WINED3DFMT_A16B16G16R16         =  36,


    WINED3DFMT_A8P8                 =  40,
    WINED3DFMT_P8                   =  41,

    WINED3DFMT_L8                   =  50,
    WINED3DFMT_A8L8                 =  51,
    WINED3DFMT_A4L4                 =  52,

    WINED3DFMT_V8U8                 =  60,
    WINED3DFMT_L6V5U5               =  61,
    WINED3DFMT_X8L8V8U8             =  62,
    WINED3DFMT_Q8W8V8U8             =  63,
    WINED3DFMT_V16U16               =  64,
    WINED3DFMT_W11V11U10            =  65,
    WINED3DFMT_A2W10V10U10          =  67,

    WINED3DFMT_UYVY                 =  WINEMAKEFOURCC('U', 'Y', 'V', 'Y'),
    WINED3DFMT_YUY2                 =  WINEMAKEFOURCC('Y', 'U', 'Y', '2'),
    WINED3DFMT_DXT1                 =  WINEMAKEFOURCC('D', 'X', 'T', '1'),
    WINED3DFMT_DXT2                 =  WINEMAKEFOURCC('D', 'X', 'T', '2'),
    WINED3DFMT_DXT3                 =  WINEMAKEFOURCC('D', 'X', 'T', '3'),
    WINED3DFMT_DXT4                 =  WINEMAKEFOURCC('D', 'X', 'T', '4'),
    WINED3DFMT_DXT5                 =  WINEMAKEFOURCC('D', 'X', 'T', '5'),
    WINED3DFMT_MULTI2_ARGB8         =  WINEMAKEFOURCC('M', 'E', 'T', '1'),
    WINED3DFMT_G8R8_G8B8            =  WINEMAKEFOURCC('G', 'R', 'G', 'B'),
    WINED3DFMT_R8G8_B8G8            =  WINEMAKEFOURCC('R', 'G', 'B', 'G'),

    WINED3DFMT_D16_LOCKABLE         =  70,
    WINED3DFMT_D32                  =  71,
    WINED3DFMT_D15S1                =  73,
    WINED3DFMT_D24S8                =  75,
    WINED3DFMT_D24X8                =  77,
    WINED3DFMT_D24X4S4              =  79,
    WINED3DFMT_D16                  =  80,
    WINED3DFMT_L16                  =  81,
    WINED3DFMT_D32F_LOCKABLE        =  82,
    WINED3DFMT_D24FS8               =  83,

    WINED3DFMT_VERTEXDATA           = 100,
    WINED3DFMT_INDEX16              = 101,
    WINED3DFMT_INDEX32              = 102,
    WINED3DFMT_Q16W16V16U16         = 110,
    /* Floating point formats */
    WINED3DFMT_R16F                 = 111,
    WINED3DFMT_G16R16F              = 112,
    WINED3DFMT_A16B16G16R16F        = 113,

    /* IEEE formats */
    WINED3DFMT_R32F                 = 114,
    WINED3DFMT_G32R32F              = 115,
    WINED3DFMT_A32B32G32R32F        = 116,

    WINED3DFMT_CxV8U8               = 117,


    WINED3DFMT_FORCE_DWORD          = 0xFFFFFFFF
} WINED3DFORMAT;

typedef enum _WINED3DRENDERSTATETYPE {
    WINED3DRS_TEXTUREHANDLE             =   1, /* d3d7 */
    WINED3DRS_ANTIALIAS                 =   2, /* d3d7 */
    WINED3DRS_TEXTUREADDRESS            =   3, /* d3d7 */
    WINED3DRS_TEXTUREPERSPECTIVE        =   4, /* d3d7 */
    WINED3DRS_WRAPU                     =   5, /* d3d7 */
    WINED3DRS_WRAPV                     =   6, /* d3d7 */
    WINED3DRS_ZENABLE                   =   7,
    WINED3DRS_FILLMODE                  =   8,
    WINED3DRS_SHADEMODE                 =   9,
    WINED3DRS_LINEPATTERN               =  10, /* d3d7, d3d8 */
    WINED3DRS_MONOENABLE                =  11, /* d3d7 */
    WINED3DRS_ROP2                      =  12, /* d3d7 */
    WINED3DRS_PLANEMASK                 =  13, /* d3d7 */
    WINED3DRS_ZWRITEENABLE              =  14,
    WINED3DRS_ALPHATESTENABLE           =  15,
    WINED3DRS_LASTPIXEL                 =  16,
    WINED3DRS_TEXTUREMAG                =  17, /* d3d7 */
    WINED3DRS_TEXTUREMIN                =  18, /* d3d7 */
    WINED3DRS_SRCBLEND                  =  19,
    WINED3DRS_DESTBLEND                 =  20,
    WINED3DRS_TEXTUREMAPBLEND           =  21, /* d3d7 */
    WINED3DRS_CULLMODE                  =  22,
    WINED3DRS_ZFUNC                     =  23,
    WINED3DRS_ALPHAREF                  =  24,
    WINED3DRS_ALPHAFUNC                 =  25,
    WINED3DRS_DITHERENABLE              =  26,
    WINED3DRS_ALPHABLENDENABLE          =  27,
    WINED3DRS_FOGENABLE                 =  28,
    WINED3DRS_SPECULARENABLE            =  29,
    WINED3DRS_ZVISIBLE                  =  30, /* d3d7, d3d8 */
    WINED3DRS_SUBPIXEL                  =  31, /* d3d7 */
    WINED3DRS_SUBPIXELX                 =  32, /* d3d7 */
    WINED3DRS_STIPPLEDALPHA             =  33, /* d3d7 */
    WINED3DRS_FOGCOLOR                  =  34,
    WINED3DRS_FOGTABLEMODE              =  35,
    WINED3DRS_FOGSTART                  =  36,
    WINED3DRS_FOGEND                    =  37,
    WINED3DRS_FOGDENSITY                =  38,
    WINED3DRS_STIPPLEENABLE             =  39, /* d3d7 */
    WINED3DRS_EDGEANTIALIAS             =  40, /* d3d7, d3d8 */
    WINED3DRS_COLORKEYENABLE            =  41, /* d3d7 */
    WINED3DRS_BORDERCOLOR               =  43, /* d3d7 */
    WINED3DRS_TEXTUREADDRESSU           =  44, /* d3d7 */
    WINED3DRS_TEXTUREADDRESSV           =  45, /* d3d7 */
    WINED3DRS_MIPMAPLODBIAS             =  46, /* d3d7 */
    WINED3DRS_ZBIAS                     =  47, /* d3d7, d3d8 */
    WINED3DRS_RANGEFOGENABLE            =  48,
    WINED3DRS_ANISOTROPY                =  49, /* d3d7 */
    WINED3DRS_FLUSHBATCH                =  50, /* d3d7 */
    WINED3DRS_TRANSLUCENTSORTINDEPENDENT = 51, /* d3d7 */
    WINED3DRS_STENCILENABLE             =  52,
    WINED3DRS_STENCILFAIL               =  53,
    WINED3DRS_STENCILZFAIL              =  54,
    WINED3DRS_STENCILPASS               =  55,
    WINED3DRS_STENCILFUNC               =  56,
    WINED3DRS_STENCILREF                =  57,
    WINED3DRS_STENCILMASK               =  58,
    WINED3DRS_STENCILWRITEMASK          =  59,
    WINED3DRS_TEXTUREFACTOR             =  60,

    WINED3DRS_STIPPLEPATTERN00          = 64,
    WINED3DRS_STIPPLEPATTERN01          = 65,
    WINED3DRS_STIPPLEPATTERN02          = 66,
    WINED3DRS_STIPPLEPATTERN03          = 67,
    WINED3DRS_STIPPLEPATTERN04          = 68,
    WINED3DRS_STIPPLEPATTERN05          = 69,
    WINED3DRS_STIPPLEPATTERN06          = 70,
    WINED3DRS_STIPPLEPATTERN07          = 71,
    WINED3DRS_STIPPLEPATTERN08          = 72,
    WINED3DRS_STIPPLEPATTERN09          = 73,
    WINED3DRS_STIPPLEPATTERN10          = 74,
    WINED3DRS_STIPPLEPATTERN11          = 75,
    WINED3DRS_STIPPLEPATTERN12          = 76,
    WINED3DRS_STIPPLEPATTERN13          = 77,
    WINED3DRS_STIPPLEPATTERN14          = 78,
    WINED3DRS_STIPPLEPATTERN15          = 79,
    WINED3DRS_STIPPLEPATTERN16          = 80,
    WINED3DRS_STIPPLEPATTERN17          = 81,
    WINED3DRS_STIPPLEPATTERN18          = 82,
    WINED3DRS_STIPPLEPATTERN19          = 83,
    WINED3DRS_STIPPLEPATTERN20          = 84,
    WINED3DRS_STIPPLEPATTERN21          = 85,
    WINED3DRS_STIPPLEPATTERN22          = 86,
    WINED3DRS_STIPPLEPATTERN23          = 87,
    WINED3DRS_STIPPLEPATTERN24          = 88,
    WINED3DRS_STIPPLEPATTERN25          = 89,
    WINED3DRS_STIPPLEPATTERN26          = 90,
    WINED3DRS_STIPPLEPATTERN27          = 91,
    WINED3DRS_STIPPLEPATTERN28          = 92,
    WINED3DRS_STIPPLEPATTERN29          = 93,
    WINED3DRS_STIPPLEPATTERN30          = 94,
    WINED3DRS_STIPPLEPATTERN31          = 95,

    WINED3DRS_WRAP0                     = 128,
    WINED3DRS_WRAP1                     = 129,
    WINED3DRS_WRAP2                     = 130,
    WINED3DRS_WRAP3                     = 131,
    WINED3DRS_WRAP4                     = 132,
    WINED3DRS_WRAP5                     = 133,
    WINED3DRS_WRAP6                     = 134,
    WINED3DRS_WRAP7                     = 135,
    WINED3DRS_CLIPPING                  = 136,
    WINED3DRS_LIGHTING                  = 137,
    WINED3DRS_EXTENTS                   = 138, /* d3d7 */
    WINED3DRS_AMBIENT                   = 139,
    WINED3DRS_FOGVERTEXMODE             = 140,
    WINED3DRS_COLORVERTEX               = 141,
    WINED3DRS_LOCALVIEWER               = 142,
    WINED3DRS_NORMALIZENORMALS          = 143,
    WINED3DRS_COLORKEYBLENDENABLE       = 144, /* d3d7 */
    WINED3DRS_DIFFUSEMATERIALSOURCE     = 145,
    WINED3DRS_SPECULARMATERIALSOURCE    = 146,
    WINED3DRS_AMBIENTMATERIALSOURCE     = 147,
    WINED3DRS_EMISSIVEMATERIALSOURCE    = 148,
    WINED3DRS_VERTEXBLEND               = 151,
    WINED3DRS_CLIPPLANEENABLE           = 152,
    WINED3DRS_SOFTWAREVERTEXPROCESSING  = 153, /* d3d8 */
    WINED3DRS_POINTSIZE                 = 154,
    WINED3DRS_POINTSIZE_MIN             = 155,
    WINED3DRS_POINTSPRITEENABLE         = 156,
    WINED3DRS_POINTSCALEENABLE          = 157,
    WINED3DRS_POINTSCALE_A              = 158,
    WINED3DRS_POINTSCALE_B              = 159,
    WINED3DRS_POINTSCALE_C              = 160,
    WINED3DRS_MULTISAMPLEANTIALIAS      = 161,
    WINED3DRS_MULTISAMPLEMASK           = 162,
    WINED3DRS_PATCHEDGESTYLE            = 163,
    WINED3DRS_PATCHSEGMENTS             = 164, /* d3d8 */
    WINED3DRS_DEBUGMONITORTOKEN         = 165,
    WINED3DRS_POINTSIZE_MAX             = 166,
    WINED3DRS_INDEXEDVERTEXBLENDENABLE  = 167,
    WINED3DRS_COLORWRITEENABLE          = 168,
    WINED3DRS_TWEENFACTOR               = 170,
    WINED3DRS_BLENDOP                   = 171,
    WINED3DRS_POSITIONORDER             = 172,
    WINED3DRS_NORMALORDER               = 173,
    WINED3DRS_POSITIONDEGREE            = 172,
    WINED3DRS_NORMALDEGREE              = 173,
    WINED3DRS_SCISSORTESTENABLE         = 174,
    WINED3DRS_SLOPESCALEDEPTHBIAS       = 175,
    WINED3DRS_ANTIALIASEDLINEENABLE     = 176,
    WINED3DRS_MINTESSELLATIONLEVEL      = 178,
    WINED3DRS_MAXTESSELLATIONLEVEL      = 179,
    WINED3DRS_ADAPTIVETESS_X            = 180,
    WINED3DRS_ADAPTIVETESS_Y            = 181,
    WINED3DRS_ADAPTIVETESS_Z            = 182,
    WINED3DRS_ADAPTIVETESS_W            = 183,
    WINED3DRS_ENABLEADAPTIVETESSELLATION= 184,
    WINED3DRS_TWOSIDEDSTENCILMODE       = 185,
    WINED3DRS_CCW_STENCILFAIL           = 186,
    WINED3DRS_CCW_STENCILZFAIL          = 187,
    WINED3DRS_CCW_STENCILPASS           = 188,
    WINED3DRS_CCW_STENCILFUNC           = 189,
    WINED3DRS_COLORWRITEENABLE1         = 190,
    WINED3DRS_COLORWRITEENABLE2         = 191,
    WINED3DRS_COLORWRITEENABLE3         = 192,
    WINED3DRS_BLENDFACTOR               = 193,
    WINED3DRS_SRGBWRITEENABLE           = 194,
    WINED3DRS_DEPTHBIAS                 = 195,
    WINED3DRS_WRAP8                     = 198,
    WINED3DRS_WRAP9                     = 199,
    WINED3DRS_WRAP10                    = 200,
    WINED3DRS_WRAP11                    = 201,
    WINED3DRS_WRAP12                    = 202,
    WINED3DRS_WRAP13                    = 203,
    WINED3DRS_WRAP14                    = 204,
    WINED3DRS_WRAP15                    = 205,
    WINED3DRS_SEPARATEALPHABLENDENABLE  = 206,
    WINED3DRS_SRCBLENDALPHA             = 207,
    WINED3DRS_DESTBLENDALPHA            = 208,
    WINED3DRS_BLENDOPALPHA              = 209,

    WINED3DRS_FORCE_DWORD               = 0x7fffffff
} WINED3DRENDERSTATETYPE;

#define WINEHIGHEST_RENDER_STATE   WINED3DRS_BLENDOPALPHA
        /* Highest WINED3DRS_ value   */

#define WINED3DCOLORWRITEENABLE_RED   (1<<0)
#define WINED3DCOLORWRITEENABLE_GREEN (1<<1)
#define WINED3DCOLORWRITEENABLE_BLUE  (1<<2)
#define WINED3DCOLORWRITEENABLE_ALPHA (1<<3)

typedef enum _WINED3DBLEND {
    WINED3DBLEND_ZERO               =  1,
    WINED3DBLEND_ONE                =  2,
    WINED3DBLEND_SRCCOLOR           =  3,
    WINED3DBLEND_INVSRCCOLOR        =  4,
    WINED3DBLEND_SRCALPHA           =  5,
    WINED3DBLEND_INVSRCALPHA        =  6,
    WINED3DBLEND_DESTALPHA          =  7,
    WINED3DBLEND_INVDESTALPHA       =  8,
    WINED3DBLEND_DESTCOLOR          =  9,
    WINED3DBLEND_INVDESTCOLOR       = 10,
    WINED3DBLEND_SRCALPHASAT        = 11,
    WINED3DBLEND_BOTHSRCALPHA       = 12,
    WINED3DBLEND_BOTHINVSRCALPHA    = 13,
    WINED3DBLEND_BLENDFACTOR        = 14,
    WINED3DBLEND_INVBLENDFACTOR     = 15,
    WINED3DBLEND_FORCE_DWORD        = 0x7fffffff
} WINED3DBLEND;

typedef enum _WINED3DBLENDOP {
    WINED3DBLENDOP_ADD              = 1,
    WINED3DBLENDOP_SUBTRACT         = 2,
    WINED3DBLENDOP_REVSUBTRACT      = 3,
    WINED3DBLENDOP_MIN              = 4,
    WINED3DBLENDOP_MAX              = 5,

    WINED3DBLENDOP_FORCE_DWORD      = 0x7fffffff
} WINED3DBLENDOP;

typedef enum _WINED3DVERTEXBLENDFLAGS {
    WINED3DVBF_DISABLE  =   0,
    WINED3DVBF_1WEIGHTS =   1,
    WINED3DVBF_2WEIGHTS =   2,
    WINED3DVBF_3WEIGHTS =   3,
    WINED3DVBF_TWEENING = 255,
    WINED3DVBF_0WEIGHTS = 256
} WINED3DVERTEXBLENDFLAGS;

typedef enum _WINED3DCMPFUNC {
    WINED3DCMP_NEVER                = 1,
    WINED3DCMP_LESS                 = 2,
    WINED3DCMP_EQUAL                = 3,
    WINED3DCMP_LESSEQUAL            = 4,
    WINED3DCMP_GREATER              = 5,
    WINED3DCMP_NOTEQUAL             = 6,
    WINED3DCMP_GREATEREQUAL         = 7,
    WINED3DCMP_ALWAYS               = 8,

    WINED3DCMP_FORCE_DWORD          = 0x7fffffff
} WINED3DCMPFUNC;

typedef enum _WINED3DZBUFFERTYPE {
    WINED3DZB_FALSE                 = 0,
    WINED3DZB_TRUE                  = 1,
    WINED3DZB_USEW                  = 2,

    WINED3DZB_FORCE_DWORD           = 0x7fffffff
} WINED3DZBUFFERTYPE;

typedef enum _WINED3DFOGMODE {
    WINED3DFOG_NONE                 = 0,
    WINED3DFOG_EXP                  = 1,
    WINED3DFOG_EXP2                 = 2,
    WINED3DFOG_LINEAR               = 3,

    WINED3DFOG_FORCE_DWORD          = 0x7fffffff
} WINED3DFOGMODE;

typedef enum _WINED3DSHADEMODE {
    WINED3DSHADE_FLAT               = 1,
    WINED3DSHADE_GOURAUD            = 2,
    WINED3DSHADE_PHONG              = 3,

    WINED3DSHADE_FORCE_DWORD        = 0x7fffffff
} WINED3DSHADEMODE;

typedef enum _WINED3DFILLMODE {
    WINED3DFILL_POINT               = 1,
    WINED3DFILL_WIREFRAME           = 2,
    WINED3DFILL_SOLID               = 3,

    WINED3DFILL_FORCE_DWORD         = 0x7fffffff
} WINED3DFILLMODE;

typedef enum _WINED3DCULL {
    WINED3DCULL_NONE                = 1,
    WINED3DCULL_CW                  = 2,
    WINED3DCULL_CCW                 = 3,

    WINED3DCULL_FORCE_DWORD         = 0x7fffffff
} WINED3DCULL;

typedef enum _WINED3DSTENCILOP {
    WINED3DSTENCILOP_KEEP           = 1,
    WINED3DSTENCILOP_ZERO           = 2,
    WINED3DSTENCILOP_REPLACE        = 3,
    WINED3DSTENCILOP_INCRSAT        = 4,
    WINED3DSTENCILOP_DECRSAT        = 5,
    WINED3DSTENCILOP_INVERT         = 6,
    WINED3DSTENCILOP_INCR           = 7,
    WINED3DSTENCILOP_DECR           = 8,

    WINED3DSTENCILOP_FORCE_DWORD    = 0x7fffffff
} WINED3DSTENCILOP;

typedef enum _WINED3DMATERIALCOLORSOURCE {
    WINED3DMCS_MATERIAL         = 0,
    WINED3DMCS_COLOR1           = 1,
    WINED3DMCS_COLOR2           = 2,

    WINED3DMCS_FORCE_DWORD      = 0x7fffffff
} WINED3DMATERIALCOLORSOURCE;

typedef enum _WINED3DPATCHEDGESTYLE {
   WINED3DPATCHEDGE_DISCRETE    = 0,
   WINED3DPATCHEDGE_CONTINUOUS  = 1,

   WINED3DPATCHEDGE_FORCE_DWORD = 0x7fffffff,
} WINED3DPATCHEDGESTYLE;

typedef struct _WINED3DDISPLAYMODE {
    UINT            Width;
    UINT            Height;
    UINT            RefreshRate;
    WINED3DFORMAT   Format;
} WINED3DDISPLAYMODE;

typedef enum _WINED3DBACKBUFFER_TYPE {
    WINED3DBACKBUFFER_TYPE_MONO         = 0,
    WINED3DBACKBUFFER_TYPE_LEFT         = 1,
    WINED3DBACKBUFFER_TYPE_RIGHT        = 2,

    WINED3DBACKBUFFER_TYPE_FORCE_DWORD  = 0x7fffffff
} WINED3DBACKBUFFER_TYPE;

#define WINED3DADAPTER_DEFAULT          0
#define WINED3DENUM_NO_WHQL_LEVEL       2
#define WINED3DPRESENT_BACK_BUFFER_MAX  3

typedef enum _WINED3DSWAPEFFECT {
    WINED3DSWAPEFFECT_DISCARD         = 1,
    WINED3DSWAPEFFECT_FLIP            = 2,
    WINED3DSWAPEFFECT_COPY            = 3,
    WINED3DSWAPEFFECT_COPY_VSYNC      = 4,
    WINED3DSWAPEFFECT_FORCE_DWORD     = 0xFFFFFFFF
} WINED3DSWAPEFFECT;

typedef enum _WINED3DSAMPLERSTATETYPE {
    WINED3DSAMP_ADDRESSU       = 1,
    WINED3DSAMP_ADDRESSV       = 2,
    WINED3DSAMP_ADDRESSW       = 3,
    WINED3DSAMP_BORDERCOLOR    = 4,
    WINED3DSAMP_MAGFILTER      = 5,
    WINED3DSAMP_MINFILTER      = 6,
    WINED3DSAMP_MIPFILTER      = 7,
    WINED3DSAMP_MIPMAPLODBIAS  = 8,
    WINED3DSAMP_MAXMIPLEVEL    = 9,
    WINED3DSAMP_MAXANISOTROPY  = 10,
    WINED3DSAMP_SRGBTEXTURE    = 11,
    WINED3DSAMP_ELEMENTINDEX   = 12,
    WINED3DSAMP_DMAPOFFSET     = 13,

    WINED3DSAMP_FORCE_DWORD   = 0x7fffffff,
} WINED3DSAMPLERSTATETYPE;
#define WINED3D_HIGHEST_SAMPLER_STATE WINED3DSAMP_DMAPOFFSET

typedef enum _WINED3DMULTISAMPLE_TYPE {
    WINED3DMULTISAMPLE_NONE          =  0,
    WINED3DMULTISAMPLE_NONMASKABLE   =  1,
    WINED3DMULTISAMPLE_2_SAMPLES     =  2,
    WINED3DMULTISAMPLE_3_SAMPLES     =  3,
    WINED3DMULTISAMPLE_4_SAMPLES     =  4,
    WINED3DMULTISAMPLE_5_SAMPLES     =  5,
    WINED3DMULTISAMPLE_6_SAMPLES     =  6,
    WINED3DMULTISAMPLE_7_SAMPLES     =  7,
    WINED3DMULTISAMPLE_8_SAMPLES     =  8,
    WINED3DMULTISAMPLE_9_SAMPLES     =  9,
    WINED3DMULTISAMPLE_10_SAMPLES    = 10,
    WINED3DMULTISAMPLE_11_SAMPLES    = 11,
    WINED3DMULTISAMPLE_12_SAMPLES    = 12,
    WINED3DMULTISAMPLE_13_SAMPLES    = 13,
    WINED3DMULTISAMPLE_14_SAMPLES    = 14,
    WINED3DMULTISAMPLE_15_SAMPLES    = 15,
    WINED3DMULTISAMPLE_16_SAMPLES    = 16,

    WINED3DMULTISAMPLE_FORCE_DWORD   = 0xffffffff
} WINED3DMULTISAMPLE_TYPE;

typedef enum _WINED3DTEXTURESTAGESTATETYPE {
    WINED3DTSS_COLOROP               =  1,
    WINED3DTSS_COLORARG1             =  2,
    WINED3DTSS_COLORARG2             =  3,
    WINED3DTSS_ALPHAOP               =  4,
    WINED3DTSS_ALPHAARG1             =  5,
    WINED3DTSS_ALPHAARG2             =  6,
    WINED3DTSS_BUMPENVMAT00          =  7,
    WINED3DTSS_BUMPENVMAT01          =  8,
    WINED3DTSS_BUMPENVMAT10          =  9,
    WINED3DTSS_BUMPENVMAT11          = 10,
    WINED3DTSS_TEXCOORDINDEX         = 11,
    WINED3DTSS_ADDRESS               = 12,
    WINED3DTSS_ADDRESSU              = 13,
    WINED3DTSS_ADDRESSV              = 14,
    WINED3DTSS_BORDERCOLOR           = 15,
    WINED3DTSS_MAGFILTER             = 16,
    WINED3DTSS_MINFILTER             = 17,
    WINED3DTSS_MIPFILTER             = 18,
    WINED3DTSS_MIPMAPLODBIAS         = 19,
    WINED3DTSS_MAXMIPLEVEL           = 20,
    WINED3DTSS_MAXANISOTROPY         = 21,
    WINED3DTSS_BUMPENVLSCALE         = 22,
    WINED3DTSS_BUMPENVLOFFSET        = 23,
    WINED3DTSS_TEXTURETRANSFORMFLAGS = 24,
    WINED3DTSS_ADDRESSW              = 25,
    WINED3DTSS_COLORARG0             = 26,
    WINED3DTSS_ALPHAARG0             = 27,
    WINED3DTSS_RESULTARG             = 28,
    WINED3DTSS_CONSTANT              = 32,

    WINED3DTSS_FORCE_DWORD           = 0x7fffffff
} WINED3DTEXTURESTAGESTATETYPE;

#define WINED3DTSS_TCI_PASSTHRU                       0x00000
#define WINED3DTSS_TCI_CAMERASPACENORMAL              0x10000
#define WINED3DTSS_TCI_CAMERASPACEPOSITION            0x20000
#define WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR    0x30000
#define WINED3DTSS_TCI_SPHEREMAP                      0x40000

#define WINED3D_HIGHEST_TEXTURE_STATE WINED3DTSS_CONSTANT

typedef enum _WINED3DTEXTURETRANSFORMFLAGS {
    WINED3DTTFF_DISABLE         =   0,
    WINED3DTTFF_COUNT1          =   1,
    WINED3DTTFF_COUNT2          =   2,
    WINED3DTTFF_COUNT3          =   3,
    WINED3DTTFF_COUNT4          =   4,
    WINED3DTTFF_PROJECTED       = 256,

    WINED3DTTFF_FORCE_DWORD     = 0x7fffffff
} WINED3DTEXTURETRANSFORMFLAGS;

typedef enum _WINED3DTEXTUREOP {
    WINED3DTOP_DISABLE                   =  1,
    WINED3DTOP_SELECTARG1                =  2,
    WINED3DTOP_SELECTARG2                =  3,
    WINED3DTOP_MODULATE                  =  4,
    WINED3DTOP_MODULATE2X                =  5,
    WINED3DTOP_MODULATE4X                =  6,
    WINED3DTOP_ADD                       =  7,
    WINED3DTOP_ADDSIGNED                 =  8,
    WINED3DTOP_ADDSIGNED2X               =  9,
    WINED3DTOP_SUBTRACT                  = 10,
    WINED3DTOP_ADDSMOOTH                 = 11,
    WINED3DTOP_BLENDDIFFUSEALPHA         = 12,
    WINED3DTOP_BLENDTEXTUREALPHA         = 13,
    WINED3DTOP_BLENDFACTORALPHA          = 14,
    WINED3DTOP_BLENDTEXTUREALPHAPM       = 15,
    WINED3DTOP_BLENDCURRENTALPHA         = 16,
    WINED3DTOP_PREMODULATE               = 17,
    WINED3DTOP_MODULATEALPHA_ADDCOLOR    = 18,
    WINED3DTOP_MODULATECOLOR_ADDALPHA    = 19,
    WINED3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
    WINED3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
    WINED3DTOP_BUMPENVMAP                = 22,
    WINED3DTOP_BUMPENVMAPLUMINANCE       = 23,
    WINED3DTOP_DOTPRODUCT3               = 24,
    WINED3DTOP_MULTIPLYADD               = 25,
    WINED3DTOP_LERP                      = 26,

    WINED3DTOP_FORCE_DWORD               = 0x7fffffff,
} WINED3DTEXTUREOP;

#define WINED3DTA_SELECTMASK        0x0000000f
#define WINED3DTA_DIFFUSE           0x00000000
#define WINED3DTA_CURRENT           0x00000001
#define WINED3DTA_TEXTURE           0x00000002
#define WINED3DTA_TFACTOR           0x00000003
#define WINED3DTA_SPECULAR          0x00000004
#define WINED3DTA_TEMP              0x00000005
#define WINED3DTA_CONSTANT          0x00000006
#define WINED3DTA_COMPLEMENT        0x00000010
#define WINED3DTA_ALPHAREPLICATE    0x00000020

typedef enum _WINED3DTEXTUREADDRESS {
    WINED3DTADDRESS_WRAP            = 1,
    WINED3DTADDRESS_MIRROR          = 2,
    WINED3DTADDRESS_CLAMP           = 3,
    WINED3DTADDRESS_BORDER          = 4,
    WINED3DTADDRESS_MIRRORONCE      = 5,

    WINED3DTADDRESS_FORCE_DWORD     = 0x7fffffff
} WINED3DTEXTUREADDRESS;

typedef enum _WINED3DTRANSFORMSTATETYPE {
    WINED3DTS_VIEW            =  2,
    WINED3DTS_PROJECTION      =  3,
    WINED3DTS_TEXTURE0        = 16,
    WINED3DTS_TEXTURE1        = 17,
    WINED3DTS_TEXTURE2        = 18,
    WINED3DTS_TEXTURE3        = 19,
    WINED3DTS_TEXTURE4        = 20,
    WINED3DTS_TEXTURE5        = 21,
    WINED3DTS_TEXTURE6        = 22,
    WINED3DTS_TEXTURE7        = 23,

    WINED3DTS_FORCE_DWORD     = 0x7fffffff
} WINED3DTRANSFORMSTATETYPE;

#define WINED3DTS_WORLD  WINED3DTS_WORLDMATRIX(0)
#define WINED3DTS_WORLD1 WINED3DTS_WORLDMATRIX(1)
#define WINED3DTS_WORLD2 WINED3DTS_WORLDMATRIX(2)
#define WINED3DTS_WORLD3 WINED3DTS_WORLDMATRIX(3)
#define WINED3DTS_WORLDMATRIX(index) (WINED3DTRANSFORMSTATETYPE)(index + 256)

typedef enum _WINED3DBASISTYPE {
   WINED3DBASIS_BEZIER        = 0,
   WINED3DBASIS_BSPLINE       = 1,
   WINED3DBASIS_INTERPOLATE   = 2,

   WINED3DBASIS_FORCE_DWORD   = 0x7fffffff
} WINED3DBASISTYPE;

typedef enum _WINED3DCUBEMAP_FACES {
    WINED3DCUBEMAP_FACE_POSITIVE_X     = 0,
    WINED3DCUBEMAP_FACE_NEGATIVE_X     = 1,
    WINED3DCUBEMAP_FACE_POSITIVE_Y     = 2,
    WINED3DCUBEMAP_FACE_NEGATIVE_Y     = 3,
    WINED3DCUBEMAP_FACE_POSITIVE_Z     = 4,
    WINED3DCUBEMAP_FACE_NEGATIVE_Z     = 5,

    WINED3DCUBEMAP_FACE_FORCE_DWORD    = 0xffffffff
} WINED3DCUBEMAP_FACES;

typedef enum _WINED3DTEXTUREFILTERTYPE {
    WINED3DTEXF_NONE            = 0,
    WINED3DTEXF_POINT           = 1,
    WINED3DTEXF_LINEAR          = 2,
    WINED3DTEXF_ANISOTROPIC     = 3,
    WINED3DTEXF_FLATCUBIC       = 4,
    WINED3DTEXF_GAUSSIANCUBIC   = 5,
    WINED3DTEXF_PYRAMIDALQUAD   = 6,
    WINED3DTEXF_GAUSSIANQUAD    = 7,
    WINED3DTEXF_FORCE_DWORD     = 0x7fffffff
} WINED3DTEXTUREFILTERTYPE;

typedef struct _WINEDD3DRECTPATCH_INFO {
    UINT                StartVertexOffsetWidth;
    UINT                StartVertexOffsetHeight;
    UINT                Width;
    UINT                Height;
    UINT                Stride;
    WINED3DBASISTYPE    Basis;
    WINED3DDEGREETYPE   Degree;
} WINED3DRECTPATCH_INFO;

typedef struct _WINED3DTRIPATCH_INFO {
    UINT                StartVertexOffset;
    UINT                NumVertices;
    WINED3DBASISTYPE    Basis;
    WINED3DDEGREETYPE   Degree;
} WINED3DTRIPATCH_INFO;


typedef struct _WINED3DADAPTER_IDENTIFIER {
    char           *Driver;
    char           *Description;
    char           *DeviceName;
    LARGE_INTEGER  *DriverVersion;
    DWORD          *VendorId;
    DWORD          *DeviceId;
    DWORD          *SubSysId;
    DWORD          *Revision;
    GUID           *DeviceIdentifier;
    DWORD          *WHQLLevel;
} WINED3DADAPTER_IDENTIFIER;

typedef struct _WINED3DPRESENT_PARAMETERS {
    UINT                    BackBufferWidth;
    UINT                    BackBufferHeight;
    WINED3DFORMAT           BackBufferFormat;
    UINT                    BackBufferCount;
    WINED3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD                   MultiSampleQuality;
    WINED3DSWAPEFFECT       SwapEffect;
    HWND                    hDeviceWindow;
    BOOL                    Windowed;
    BOOL                    EnableAutoDepthStencil;
    WINED3DFORMAT           AutoDepthStencilFormat;
    DWORD                   Flags;
    UINT                    FullScreen_RefreshRateInHz;
    UINT                    PresentationInterval;
} WINED3DPRESENT_PARAMETERS;

typedef enum _WINED3DRESOURCETYPE {
    WINED3DRTYPE_SURFACE                =  1,
    WINED3DRTYPE_VOLUME                 =  2,
    WINED3DRTYPE_TEXTURE                =  3,
    WINED3DRTYPE_VOLUMETEXTURE          =  4,
    WINED3DRTYPE_CUBETEXTURE            =  5,
    WINED3DRTYPE_VERTEXBUFFER           =  6,
    WINED3DRTYPE_INDEXBUFFER            =  7,

    WINED3DRTYPE_FORCE_DWORD            = 0x7fffffff
} WINED3DRESOURCETYPE;

#define WINED3DRTYPECOUNT (WINED3DRTYPE_INDEXBUFFER+1)

typedef enum _WINED3DPOOL {
    WINED3DPOOL_DEFAULT                 = 0,
    WINED3DPOOL_MANAGED                 = 1,
    WINED3DPOOL_SYSTEMMEM               = 2,
    WINED3DPOOL_SCRATCH                 = 3,

    WINED3DPOOL_FORCE_DWORD             = 0x7fffffff
} WINED3DPOOL;

typedef struct _WINED3DSURFACE_DESC
{
    WINED3DFORMAT           *Format;
    WINED3DRESOURCETYPE     *Type;
    DWORD                   *Usage;
    WINED3DPOOL             *Pool;
    UINT                    *Size;

    WINED3DMULTISAMPLE_TYPE *MultiSampleType;
    DWORD                   *MultiSampleQuality;
    UINT                    *Width;
    UINT                    *Height;
} WINED3DSURFACE_DESC;

typedef struct _WINED3DVOLUME_DESC
{
    WINED3DFORMAT       *Format;
    WINED3DRESOURCETYPE *Type;
    DWORD               *Usage;
    WINED3DPOOL         *Pool;
    UINT                *Size;

    UINT                *Width;
    UINT                *Height;
    UINT                *Depth;
} WINED3DVOLUME_DESC;

typedef struct _WINED3DCLIPSTATUS {
   DWORD ClipUnion;
   DWORD ClipIntersection;
} WINED3DCLIPSTATUS;


typedef struct _WINED3DVERTEXELEMENT {
  WORD    Stream;
  WORD    Offset;
  BYTE    Type;
  BYTE    Method;
  BYTE    Usage;
  BYTE    UsageIndex;
  int     Reg; /* DirectX 8 */
} WINED3DVERTEXELEMENT, *LPWINED3DVERTEXELEMENT;


typedef enum _WINED3DQUERYTYPE {
    WINED3DQUERYTYPE_VCACHE             = 4,
    WINED3DQUERYTYPE_RESOURCEMANAGER    = 5,
    WINED3DQUERYTYPE_VERTEXSTATS        = 6,
    WINED3DQUERYTYPE_EVENT              = 8,
    WINED3DQUERYTYPE_OCCLUSION          = 9,
    WINED3DQUERYTYPE_TIMESTAMP          = 10,
    WINED3DQUERYTYPE_TIMESTAMPDISJOINT  = 11,
    WINED3DQUERYTYPE_TIMESTAMPFREQ      = 12,
    WINED3DQUERYTYPE_PIPELINETIMINGS    = 13,
    WINED3DQUERYTYPE_INTERFACETIMINGS   = 14,
    WINED3DQUERYTYPE_VERTEXTIMINGS      = 15,
    WINED3DQUERYTYPE_PIXELTIMINGS       = 16,
    WINED3DQUERYTYPE_BANDWIDTHTIMINGS   = 17,
    WINED3DQUERYTYPE_CACHEUTILIZATION   = 18
} WINED3DQUERYTYPE;

#define WINED3DISSUE_BEGIN   (1 << 1)
#define WINED3DISSUE_END     (1 << 0)
#define WINED3DGETDATA_FLUSH (1 << 0)

typedef struct _WINED3DDEVICE_CREATION_PARAMETERS {
    UINT           AdapterOrdinal;
    WINED3DDEVTYPE DeviceType;
    HWND           hFocusWindow;
    DWORD          BehaviorFlags;
} WINED3DDEVICE_CREATION_PARAMETERS;

typedef struct _WINED3DDEVINFO_BANDWIDTHTIMINGS {
    float         MaxBandwidthUtilized;
    float         FrontEndUploadMemoryUtilizedPercent;
    float         VertexRateUtilizedPercent;
    float         TriangleSetupRateUtilizedPercent;
    float         FillRateUtilizedPercent;
} WINED3DDEVINFO_BANDWIDTHTIMINGS;

typedef struct _WINED3DDEVINFO_CACHEUTILIZATION {
    float         TextureCacheHitRate;
    float         PostTransformVertexCacheHitRate;
} WINED3DDEVINFO_CACHEUTILIZATION;

typedef struct _WINED3DDEVINFO_INTERFACETIMINGS {
    float         WaitingForGPUToUseApplicationResourceTimePercent;
    float         WaitingForGPUToAcceptMoreCommandsTimePercent;
    float         WaitingForGPUToStayWithinLatencyTimePercent;
    float         WaitingForGPUExclusiveResourceTimePercent;
    float         WaitingForGPUOtherTimePercent;
} WINED3DDEVINFO_INTERFACETIMINGS;

typedef struct _WINED3DDEVINFO_PIPELINETIMINGS {
    float         VertexProcessingTimePercent;
    float         PixelProcessingTimePercent;
    float         OtherGPUProcessingTimePercent;
    float         GPUIdleTimePercent;
} WINED3DDEVINFO_PIPELINETIMINGS;

typedef struct _WINED3DDEVINFO_STAGETIMINGS {
    float         MemoryProcessingPercent;
    float         ComputationProcessingPercent;
} WINED3DDEVINFO_STAGETIMINGS;

typedef struct _WINED3DRASTER_STATUS {
    BOOL            InVBlank;
    UINT            ScanLine;
} WINED3DRASTER_STATUS;


typedef struct WINED3DRESOURCESTATS {
    BOOL                bThrashing;
    DWORD               ApproxBytesDownloaded;
    DWORD               NumEvicts;
    DWORD               NumVidCreates;
    DWORD               LastPri;
    DWORD               NumUsed;
    DWORD               NumUsedInVidMem;
    DWORD               WorkingSet;
    DWORD               WorkingSetBytes;
    DWORD               TotalManaged;
    DWORD               TotalBytes;
} WINED3DRESOURCESTATS;

typedef struct _WINED3DDEVINFO_RESOURCEMANAGER {
    WINED3DRESOURCESTATS stats[WINED3DRTYPECOUNT];
} WINED3DDEVINFO_RESOURCEMANAGER;

typedef struct _WINED3DDEVINFO_VERTEXSTATS {
    DWORD NumRenderedTriangles;
    DWORD NumExtraClippingTriangles;
} WINED3DDEVINFO_VERTEXSTATS;

typedef struct _WINED3DLOCKED_RECT {
    INT                 Pitch;
    void*               pBits;
} WINED3DLOCKED_RECT;

typedef struct _WINED3DLOCKED_BOX {
    INT                 RowPitch;
    INT                 SlicePitch;
    void*               pBits;
} WINED3DLOCKED_BOX;


typedef struct _WINED3DBOX {
    UINT                Left;
    UINT                Top;
    UINT                Right;
    UINT                Bottom;
    UINT                Front;
    UINT                Back;
} WINED3DBOX;

/*Vertex cache optimization hints.*/
typedef struct WINED3DDEVINFO_VCACHE {
    /*Must be a 4 char code FOURCC (e.g. CACH)*/
    DWORD         Pattern;
    /*0 to get the longest  strips, 1 vertex cache*/
    DWORD         OptMethod;
     /*Cache size to use (only valid if OptMethod==1) */
    DWORD         CacheSize;
    /*internal for deciding when to restart strips, non user modifyable (only valid if OptMethod==1)*/
    DWORD         MagicNumber;
} WINED3DDEVINFO_VCACHE;

typedef struct _WINED3DVERTEXBUFFER_DESC {
    WINED3DFORMAT           Format;
    WINED3DRESOURCETYPE     Type;
    DWORD                   Usage;
    WINED3DPOOL             Pool;
    UINT                    Size;
    DWORD                   FVF;
} WINED3DVERTEXBUFFER_DESC;

typedef struct _WINED3DINDEXBUFFER_DESC {
    WINED3DFORMAT           Format;
    WINED3DRESOURCETYPE     Type;
    DWORD                   Usage;
    WINED3DPOOL             Pool;
    UINT                    Size;
} WINED3DINDEXBUFFER_DESC;

/*
 * The wined3dcaps structure
 */

typedef struct _WINED3DVSHADERCAPS2_0 {
  DWORD  *Caps;
  INT    *DynamicFlowControlDepth;
  INT    *NumTemps;
  INT    *StaticFlowControlDepth;
} WINED3DVSHADERCAPS2_0;

typedef struct _WINED3DPSHADERCAPS2_0 {
  DWORD  *Caps;
  INT    *DynamicFlowControlDepth;
  INT    *NumTemps;
  INT    *StaticFlowControlDepth;
  INT    *NumInstructionSlots;
} WINED3DPSHADERCAPS2_0;

typedef struct _WINED3DCAPS {
  WINED3DDEVTYPE      *DeviceType;
  UINT                *AdapterOrdinal;

  DWORD               *Caps;
  DWORD               *Caps2;
  DWORD               *Caps3;
  DWORD               *PresentationIntervals;

  DWORD               *CursorCaps;

  DWORD               *DevCaps;

  DWORD               *PrimitiveMiscCaps;
  DWORD               *RasterCaps;
  DWORD               *ZCmpCaps;
  DWORD               *SrcBlendCaps;
  DWORD               *DestBlendCaps;
  DWORD               *AlphaCmpCaps;
  DWORD               *ShadeCaps;
  DWORD               *TextureCaps;
  DWORD               *TextureFilterCaps;
  DWORD               *CubeTextureFilterCaps;
  DWORD               *VolumeTextureFilterCaps;
  DWORD               *TextureAddressCaps;
  DWORD               *VolumeTextureAddressCaps;

  DWORD               *LineCaps;

  DWORD               *MaxTextureWidth;
  DWORD               *MaxTextureHeight;
  DWORD               *MaxVolumeExtent;

  DWORD               *MaxTextureRepeat;
  DWORD               *MaxTextureAspectRatio;
  DWORD               *MaxAnisotropy;
  float               *MaxVertexW;

  float               *GuardBandLeft;
  float               *GuardBandTop;
  float               *GuardBandRight;
  float               *GuardBandBottom;

  float               *ExtentsAdjust;
  DWORD               *StencilCaps;

  DWORD               *FVFCaps;
  DWORD               *TextureOpCaps;
  DWORD               *MaxTextureBlendStages;
  DWORD               *MaxSimultaneousTextures;

  DWORD               *VertexProcessingCaps;
  DWORD               *MaxActiveLights;
  DWORD               *MaxUserClipPlanes;
  DWORD               *MaxVertexBlendMatrices;
  DWORD               *MaxVertexBlendMatrixIndex;

  float               *MaxPointSize;

  DWORD               *MaxPrimitiveCount;
  DWORD               *MaxVertexIndex;
  DWORD               *MaxStreams;
  DWORD               *MaxStreamStride;

  DWORD               *VertexShaderVersion;
  DWORD               *MaxVertexShaderConst;

  DWORD               *PixelShaderVersion;
  float               *PixelShader1xMaxValue;

  /* DX 9 */
  DWORD               *DevCaps2;

  float               *MaxNpatchTessellationLevel;
  DWORD               *Reserved5; /*undocumented*/

  UINT                *MasterAdapterOrdinal;
  UINT                *AdapterOrdinalInGroup;
  UINT                *NumberOfAdaptersInGroup;
  DWORD               *DeclTypes;
  DWORD               *NumSimultaneousRTs;
  DWORD               *StretchRectFilterCaps;
  WINED3DVSHADERCAPS2_0   VS20Caps;
  WINED3DPSHADERCAPS2_0   PS20Caps;
  DWORD               *VertexTextureFilterCaps;
  DWORD               *MaxVShaderInstructionsExecuted;
  DWORD               *MaxPShaderInstructionsExecuted;
  DWORD               *MaxVertexShader30InstructionSlots;
  DWORD               *MaxPixelShader30InstructionSlots;
  DWORD               *Reserved2;/* Not in the microsoft headers but documented */
  DWORD               *Reserved3;

} WINED3DCAPS;

typedef enum _WINED3DSTATEBLOCKTYPE {
    WINED3DSBT_INIT          = 0,
    WINED3DSBT_ALL           = 1,
    WINED3DSBT_PIXELSTATE    = 2,
    WINED3DSBT_VERTEXSTATE   = 3,
    WINED3DSBT_RECORDED      = 4,       /* WineD3D private */

    WINED3DSBT_FORCE_DWORD   = 0xffffffff
} WINED3DSTATEBLOCKTYPE;

typedef struct glDescriptor {
    UINT          textureName;
    int           level;
    int           target;
    int/*GLenum*/ glFormat;
    int/*GLenum*/ glFormatInternal;
    int/*GLenum*/ glType;
} glDescriptor;

#define WINED3DDP_MAXTEXCOORD 8

typedef enum _WINED3DDECLMETHOD {
    WINED3DDECLMETHOD_DEFAULT          = 0,
    WINED3DDECLMETHOD_PARTIALU         = 1,
    WINED3DDECLMETHOD_PARTIALV         = 2,
    WINED3DDECLMETHOD_CROSSUV          = 3,
    WINED3DDECLMETHOD_UV               = 4,
    WINED3DDECLMETHOD_LOOKUP           = 5,
    WINED3DDECLMETHOD_LOOKUPPRESAMPLED = 6
} WINED3DDECLMETHOD;

typedef enum _WINED3DDECLTYPE {
  WINED3DDECLTYPE_FLOAT1    =  0,
  WINED3DDECLTYPE_FLOAT2    =  1,
  WINED3DDECLTYPE_FLOAT3    =  2,
  WINED3DDECLTYPE_FLOAT4    =  3,
  WINED3DDECLTYPE_D3DCOLOR  =  4,
  WINED3DDECLTYPE_UBYTE4    =  5,
  WINED3DDECLTYPE_SHORT2    =  6,
  WINED3DDECLTYPE_SHORT4    =  7,
  /* VS 2.0 */
  WINED3DDECLTYPE_UBYTE4N   =  8,
  WINED3DDECLTYPE_SHORT2N   =  9,
  WINED3DDECLTYPE_SHORT4N   = 10,
  WINED3DDECLTYPE_USHORT2N  = 11,
  WINED3DDECLTYPE_USHORT4N  = 12,
  WINED3DDECLTYPE_UDEC3     = 13,
  WINED3DDECLTYPE_DEC3N     = 14,
  WINED3DDECLTYPE_FLOAT16_2 = 15,
  WINED3DDECLTYPE_FLOAT16_4 = 16,
  WINED3DDECLTYPE_UNUSED    = 17,
} WINED3DDECLTYPE;

#define WINED3DDECL_END() {0xFF,0,WINED3DDECLTYPE_UNUSED,0,0,0,-1}

typedef struct WineDirect3DStridedData {
    BYTE     *lpData;        /* Pointer to start of data               */
    DWORD     dwStride;      /* Stride between occurances of this data */
    DWORD     dwType;        /* Type (as in D3DVSDT_TYPE)              */
    int       VBO;           /* Vertex buffer object this data is in   */
    UINT      streamNo;      /* D3D stream number                      */
} WineDirect3DStridedData;

typedef struct WineDirect3DVertexStridedData {
    union {
        struct {

             /* Do not add or reorder fields here,
              * so this can be indexed as an array */
             WineDirect3DStridedData  position;
             WineDirect3DStridedData  blendWeights;
             WineDirect3DStridedData  blendMatrixIndices;
             WineDirect3DStridedData  normal;
             WineDirect3DStridedData  pSize;
             WineDirect3DStridedData  diffuse;
             WineDirect3DStridedData  specular;
             WineDirect3DStridedData  texCoords[WINED3DDP_MAXTEXCOORD];
             WineDirect3DStridedData  position2; /* tween data */
             WineDirect3DStridedData  normal2;   /* tween data */
             WineDirect3DStridedData  tangent;
             WineDirect3DStridedData  binormal;
             WineDirect3DStridedData  tessFactor;
             WineDirect3DStridedData  fog;
             WineDirect3DStridedData  depth;
             WineDirect3DStridedData  sample;

             /* Add fields here */
             BOOL position_transformed;

        } s;
        WineDirect3DStridedData input[16];  /* Indexed by constants in D3DVSDE_REGISTER */
    } u;
} WineDirect3DVertexStridedData;

typedef enum {
    WINED3DDECLUSAGE_POSITION     = 0,
    WINED3DDECLUSAGE_BLENDWEIGHT  = 1,
    WINED3DDECLUSAGE_BLENDINDICES = 2,
    WINED3DDECLUSAGE_NORMAL       = 3,
    WINED3DDECLUSAGE_PSIZE        = 4,
    WINED3DDECLUSAGE_TEXCOORD     = 5,
    WINED3DDECLUSAGE_TANGENT      = 6,
    WINED3DDECLUSAGE_BINORMAL     = 7,
    WINED3DDECLUSAGE_TESSFACTOR   = 8,
    WINED3DDECLUSAGE_POSITIONT    = 9,
    WINED3DDECLUSAGE_COLOR        = 10,
    WINED3DDECLUSAGE_FOG          = 11,
    WINED3DDECLUSAGE_DEPTH        = 12,
    WINED3DDECLUSAGE_SAMPLE       = 13
} WINED3DDECLUSAGE;

#define WINED3DUSAGE_RENDERTARGET                     0x00000001L
#define WINED3DUSAGE_DEPTHSTENCIL                     0x00000002L
#define WINED3DUSAGE_WRITEONLY                        0x00000008L
#define WINED3DUSAGE_SOFTWAREPROCESSING               0x00000010L
#define WINED3DUSAGE_DONOTCLIP                        0x00000020L
#define WINED3DUSAGE_POINTS                           0x00000040L
#define WINED3DUSAGE_RTPATCHES                        0x00000080L
#define WINED3DUSAGE_NPATCHES                         0x00000100L
#define WINED3DUSAGE_DYNAMIC                          0x00000200L
#define WINED3DUSAGE_AUTOGENMIPMAP                    0x00000400L
#define WINED3DUSAGE_DMAP                             0x00004000L
#define WINED3DUSAGE_MASK                             0x00004FFFL
#define WINED3DUSAGE_OVERLAY                          0x00010000L

#define WINED3DUSAGE_QUERY_LEGACYBUMPMAP            0x00008000L
#define WINED3DUSAGE_QUERY_FILTER                   0x00020000L
#define WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING 0x00080000L
#define WINED3DUSAGE_QUERY_SRGBREAD                 0x00010000L
#define WINED3DUSAGE_QUERY_SRGBWRITE                0x00040000L
#define WINED3DUSAGE_QUERY_VERTEXTEXTURE            0x00100000L
#define WINED3DUSAGE_QUERY_WRAPANDMIP               0x00200000L
#define WINED3DUSAGE_QUERY_MASK                     0x002F8000L

typedef enum _WINED3DSURFTYPE {
    SURFACE_UNKNOWN    = 0,   /* Default / Unknown surface type */
    SURFACE_OPENGL,           /* OpenGL surface: Renders using libGL, needed for 3D */
    SURFACE_GDI,              /* User surface. No 3D, DirectDraw rendering with GDI */
    SURFACE_XRENDER           /* Future dreams: Use XRENDER / EXA / whatever stuff */
} WINED3DSURFTYPE;

#define WINED3DCAPS2_NO2DDURING3DSCENE                 0x00000002L
#define WINED3DCAPS2_FULLSCREENGAMMA                   0x00020000L
#define WINED3DCAPS2_CANRENDERWINDOWED                 0x00080000L
#define WINED3DCAPS2_CANCALIBRATEGAMMA                 0x00100000L
#define WINED3DCAPS2_RESERVED                          0x02000000L
#define WINED3DCAPS2_CANMANAGERESOURCE                 0x10000000L
#define WINED3DCAPS2_DYNAMICTEXTURES                   0x20000000L
#define WINED3DCAPS2_CANAUTOGENMIPMAP                  0x40000000L

#define WINED3DPRASTERCAPS_DITHER                     0x00000001L
#define WINED3DPRASTERCAPS_ROP2                       0x00000002L
#define WINED3DPRASTERCAPS_XOR                        0x00000004L
#define WINED3DPRASTERCAPS_PAT                        0x00000008L
#define WINED3DPRASTERCAPS_ZTEST                      0x00000010L
#define WINED3DPRASTERCAPS_SUBPIXEL                   0x00000020L
#define WINED3DPRASTERCAPS_SUBPIXELX                  0x00000040L
#define WINED3DPRASTERCAPS_FOGVERTEX                  0x00000080L
#define WINED3DPRASTERCAPS_FOGTABLE                   0x00000100L
#define WINED3DPRASTERCAPS_STIPPLE                    0x00000200L
#define WINED3DPRASTERCAPS_ANTIALIASSORTDEPENDENT     0x00000400L
#define WINED3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT   0x00000800L
#define WINED3DPRASTERCAPS_ANTIALIASEDGES             0x00001000L
#define WINED3DPRASTERCAPS_MIPMAPLODBIAS              0x00002000L
#define WINED3DPRASTERCAPS_ZBIAS                      0x00004000L
#define WINED3DPRASTERCAPS_ZBUFFERLESSHSR             0x00008000L
#define WINED3DPRASTERCAPS_FOGRANGE                   0x00010000L
#define WINED3DPRASTERCAPS_ANISOTROPY                 0x00020000L
#define WINED3DPRASTERCAPS_WBUFFER                    0x00040000L
#define WINED3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT 0x00080000L
#define WINED3DPRASTERCAPS_WFOG                       0x00100000L
#define WINED3DPRASTERCAPS_ZFOG                       0x00200000L
#define WINED3DPRASTERCAPS_COLORPERSPECTIVE           0x00400000L
#define WINED3DPRASTERCAPS_SCISSORTEST                0x01000000L
#define WINED3DPRASTERCAPS_SLOPESCALEDEPTHBIAS        0x02000000L
#define WINED3DPRASTERCAPS_DEPTHBIAS                  0x04000000L
#define WINED3DPRASTERCAPS_MULTISAMPLE_TOGGLE         0x08000000L

#define WINED3DPSHADECAPS_COLORFLATMONO               0x000001
#define WINED3DPSHADECAPS_COLORFLATRGB                0x000002
#define WINED3DPSHADECAPS_COLORGOURAUDMONO            0x000004
#define WINED3DPSHADECAPS_COLORGOURAUDRGB             0x000008
#define WINED3DPSHADECAPS_COLORPHONGMONO              0x000010
#define WINED3DPSHADECAPS_COLORPHONGRGB               0x000020
#define WINED3DPSHADECAPS_SPECULARFLATMONO            0x000040
#define WINED3DPSHADECAPS_SPECULARFLATRGB             0x000080
#define WINED3DPSHADECAPS_SPECULARGOURAUDMONO         0x000100
#define WINED3DPSHADECAPS_SPECULARGOURAUDRGB          0x000200
#define WINED3DPSHADECAPS_SPECULARPHONGMONO           0x000400
#define WINED3DPSHADECAPS_SPECULARPHONGRGB            0x000800
#define WINED3DPSHADECAPS_ALPHAFLATBLEND              0x001000
#define WINED3DPSHADECAPS_ALPHAFLATSTIPPLED           0x002000
#define WINED3DPSHADECAPS_ALPHAGOURAUDBLEND           0x004000
#define WINED3DPSHADECAPS_ALPHAGOURAUDSTIPPLED        0x008000
#define WINED3DPSHADECAPS_ALPHAPHONGBLEND             0x010000
#define WINED3DPSHADECAPS_ALPHAPHONGSTIPPLED          0x020000
#define WINED3DPSHADECAPS_FOGFLAT                     0x040000
#define WINED3DPSHADECAPS_FOGGOURAUD                  0x080000
#define WINED3DPSHADECAPS_FOGPHONG                    0x100000

#define WINED3DPTEXTURECAPS_PERSPECTIVE              0x00000001L
#define WINED3DPTEXTURECAPS_POW2                     0x00000002L
#define WINED3DPTEXTURECAPS_ALPHA                    0x00000004L
#define WINED3DPTEXTURECAPS_TRANSPARENCY             0x00000008L
#define WINED3DPTEXTURECAPS_BORDER                   0x00000010L
#define WINED3DPTEXTURECAPS_SQUAREONLY               0x00000020L
#define WINED3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE 0x00000040L
#define WINED3DPTEXTURECAPS_ALPHAPALETTE             0x00000080L
#define WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL       0x00000100L
#define WINED3DPTEXTURECAPS_PROJECTED                0x00000400L
#define WINED3DPTEXTURECAPS_CUBEMAP                  0x00000800L
#define WINED3DPTEXTURECAPS_COLORKEYBLEND            0x00001000L
#define WINED3DPTEXTURECAPS_VOLUMEMAP                0x00002000L
#define WINED3DPTEXTURECAPS_MIPMAP                   0x00004000L
#define WINED3DPTEXTURECAPS_MIPVOLUMEMAP             0x00008000L
#define WINED3DPTEXTURECAPS_MIPCUBEMAP               0x00010000L
#define WINED3DPTEXTURECAPS_CUBEMAP_POW2             0x00020000L
#define WINED3DPTEXTURECAPS_VOLUMEMAP_POW2           0x00040000L
#define WINED3DPTEXTURECAPS_NOPROJECTEDBUMPENV       0x00200000L

#define WINED3DPTFILTERCAPS_NEAREST                  0x00000001
#define WINED3DPTFILTERCAPS_LINEAR                   0x00000002
#define WINED3DPTFILTERCAPS_MIPNEAREST               0x00000004
#define WINED3DPTFILTERCAPS_MIPLINEAR                0x00000008
#define WINED3DPTFILTERCAPS_LINEARMIPNEAREST         0x00000010
#define WINED3DPTFILTERCAPS_LINEARMIPLINEAR          0x00000020
#define WINED3DPTFILTERCAPS_MINFPOINT                0x00000100
#define WINED3DPTFILTERCAPS_MINFLINEAR               0x00000200
#define WINED3DPTFILTERCAPS_MINFANISOTROPIC          0x00000400
#define WINED3DPTFILTERCAPS_MIPFPOINT                0x00010000
#define WINED3DPTFILTERCAPS_MIPFLINEAR               0x00020000
#define WINED3DPTFILTERCAPS_MAGFPOINT                0x01000000
#define WINED3DPTFILTERCAPS_MAGFLINEAR               0x02000000
#define WINED3DPTFILTERCAPS_MAGFANISOTROPIC          0x04000000
#define WINED3DPTFILTERCAPS_MAGFPYRAMIDALQUAD        0x08000000
#define WINED3DPTFILTERCAPS_MAGFGAUSSIANQUAD         0x10000000

#define WINED3DVTXPCAPS_TEXGEN                       0x00000001L
#define WINED3DVTXPCAPS_MATERIALSOURCE7              0x00000002L
#define WINED3DVTXPCAPS_VERTEXFOG                    0x00000004L
#define WINED3DVTXPCAPS_DIRECTIONALLIGHTS            0x00000008L
#define WINED3DVTXPCAPS_POSITIONALLIGHTS             0x00000010L
#define WINED3DVTXPCAPS_LOCALVIEWER                  0x00000020L
#define WINED3DVTXPCAPS_TWEENING                     0x00000040L
#define WINED3DVTXPCAPS_TEXGEN_SPHEREMAP             0x00000100L
#define WINED3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER     0x00000200L

#define WINED3DCURSORCAPS_COLOR                      0x000000001
#define WINED3DCURSORCAPS_LOWRES                     0x000000002

#define WINED3DDEVCAPS_FLOATTLVERTEX                 0x000000001
#define WINED3DDEVCAPS_SORTINCREASINGZ               0x000000002
#define WINED3DDEVCAPS_SORTDECREASINGZ               0X000000004
#define WINED3DDEVCAPS_SORTEXACT                     0x000000008
#define WINED3DDEVCAPS_EXECUTESYSTEMMEMORY           0x000000010
#define WINED3DDEVCAPS_EXECUTEVIDEOMEMORY            0x000000020
#define WINED3DDEVCAPS_TLVERTEXSYSTEMMEMORY          0x000000040
#define WINED3DDEVCAPS_TLVERTEXVIDEOMEMORY           0x000000080
#define WINED3DDEVCAPS_TEXTURESYSTEMMEMORY           0x000000100
#define WINED3DDEVCAPS_TEXTUREVIDEOMEMORY            0x000000200
#define WINED3DDEVCAPS_DRAWPRIMTLVERTEX              0x000000400
#define WINED3DDEVCAPS_CANRENDERAFTERFLIP            0x000000800
#define WINED3DDEVCAPS_TEXTURENONLOCALVIDMEM         0x000001000
#define WINED3DDEVCAPS_DRAWPRIMITIVES2               0x000002000
#define WINED3DDEVCAPS_SEPARATETEXTUREMEMORIES       0x000004000
#define WINED3DDEVCAPS_DRAWPRIMITIVES2EX             0x000008000
#define WINED3DDEVCAPS_HWTRANSFORMANDLIGHT           0x000010000
#define WINED3DDEVCAPS_CANBLTSYSTONONLOCAL           0x000020000
#define WINED3DDEVCAPS_HWRASTERIZATION               0x000080000
#define WINED3DDEVCAPS_PUREDEVICE                    0x000100000
#define WINED3DDEVCAPS_QUINTICRTPATCHES              0x000200000
#define WINED3DDEVCAPS_RTPATCHES                     0x000400000
#define WINED3DDEVCAPS_RTPATCHHANDLEZERO             0x000800000
#define WINED3DDEVCAPS_NPATCHES                      0x001000000

#define WINED3DLOCK_READONLY           0x0010
#define WINED3DLOCK_NOSYSLOCK          0x0800
#define WINED3DLOCK_NOOVERWRITE        0x1000
#define WINED3DLOCK_DISCARD            0x2000
#define WINED3DLOCK_DONOTWAIT          0x4000
#define WINED3DLOCK_NO_DIRTY_UPDATE    0x8000

#define WINED3DPRESENT_RATE_DEFAULT                  0x000000000

#define WINED3DPRESENT_INTERVAL_DEFAULT              0x00000000
#define WINED3DPRESENT_INTERVAL_ONE                  0x00000001
#define WINED3DPRESENT_INTERVAL_TWO                  0x00000002
#define WINED3DPRESENT_INTERVAL_THREE                0x00000004
#define WINED3DPRESENT_INTERVAL_FOUR                 0x00000008
#define WINED3DPRESENT_INTERVAL_IMMEDIATE            0x80000000

#define WINED3DMAXUSERCLIPPLANES       32
#define WINED3DCLIPPLANE0              (1 << 0)
#define WINED3DCLIPPLANE1              (1 << 1)
#define WINED3DCLIPPLANE2              (1 << 2)
#define WINED3DCLIPPLANE3              (1 << 3)
#define WINED3DCLIPPLANE4              (1 << 4)
#define WINED3DCLIPPLANE5              (1 << 5)

/* FVF (Flexible Vertex Format) codes */
#define WINED3DFVF_RESERVED0           0x0001
#define WINED3DFVF_POSITION_MASK       0x000E
#define WINED3DFVF_XYZ                 0x0002
#define WINED3DFVF_XYZRHW              0x0004
#define WINED3DFVF_XYZB1               0x0006
#define WINED3DFVF_XYZB2               0x0008
#define WINED3DFVF_XYZB3               0x000a
#define WINED3DFVF_XYZB4               0x000c
#define WINED3DFVF_XYZB5               0x000e
#define WINED3DFVF_XYZW                0x4002
#define WINED3DFVF_NORMAL              0x0010
#define WINED3DFVF_PSIZE               0x0020
#define WINED3DFVF_DIFFUSE             0x0040
#define WINED3DFVF_SPECULAR            0x0080
#define WINED3DFVF_TEXCOUNT_MASK       0x0f00
#define WINED3DFVF_TEXCOUNT_SHIFT           8
#define WINED3DFVF_TEX0                0x0000
#define WINED3DFVF_TEX1                0x0100
#define WINED3DFVF_TEX2                0x0200
#define WINED3DFVF_TEX3                0x0300
#define WINED3DFVF_TEX4                0x0400
#define WINED3DFVF_TEX5                0x0500
#define WINED3DFVF_TEX6                0x0600
#define WINED3DFVF_TEX7                0x0700
#define WINED3DFVF_TEX8                0x0800
#define WINED3DFVF_LASTBETA_UBYTE4     0x1000
#define WINED3DFVF_LASTBETA_D3DCOLOR   0x8000
#define WINED3DFVF_RESERVED2           0x6000

#define WINED3DFVF_TEXTUREFORMAT1 3
#define WINED3DFVF_TEXTUREFORMAT2 0
#define WINED3DFVF_TEXTUREFORMAT3 1
#define WINED3DFVF_TEXTUREFORMAT4 2
#define WINED3DFVF_TEXCOORDSIZE1(CoordIndex) (WINED3DFVF_TEXTUREFORMAT1 << (CoordIndex*2 + 16))
#define WINED3DFVF_TEXCOORDSIZE2(CoordIndex) (WINED3DFVF_TEXTUREFORMAT2)
#define WINED3DFVF_TEXCOORDSIZE3(CoordIndex) (WINED3DFVF_TEXTUREFORMAT3 << (CoordIndex*2 + 16))
#define WINED3DFVF_TEXCOORDSIZE4(CoordIndex) (WINED3DFVF_TEXTUREFORMAT4 << (CoordIndex*2 + 16))

/* Clear flags */
#define WINED3DCLEAR_TARGET   0x00000001
#define WINED3DCLEAR_ZBUFFER  0x00000002
#define WINED3DCLEAR_STENCIL  0x00000004

/* Stream source flags */
#define WINED3DSTREAMSOURCE_INDEXEDDATA  (1 << 30)
#define WINED3DSTREAMSOURCE_INSTANCEDATA (2 << 30)

/* SetPrivateData flags */
#define WINED3DSPD_IUNKNOWN 0x00000001

/* IWineD3D::CreateDevice behaviour flags */
#define WINED3DCREATE_FPU_PRESERVE                  0x00000002
#define WINED3DCREATE_PUREDEVICE                    0x00000010
#define WINED3DCREATE_SOFTWARE_VERTEXPROCESSING     0x00000020
#define WINED3DCREATE_HARDWARE_VERTEXPROCESSING     0x00000040
#define WINED3DCREATE_MIXED_VERTEXPROCESSING        0x00000080
#define WINED3DCREATE_DISABLE_DRIVER_MANAGEMENT     0x00000100
#define WINED3DCREATE_ADAPTERGROUP_DEVICE           0x00000200

/* VTF defines */
#define WINED3DDMAPSAMPLER              0x100
#define WINED3DVERTEXTEXTURESAMPLER0    (WINED3DDMAPSAMPLER + 1)
#define WINED3DVERTEXTEXTURESAMPLER1    (WINED3DDMAPSAMPLER + 2)
#define WINED3DVERTEXTEXTURESAMPLER2    (WINED3DDMAPSAMPLER + 3)
#define WINED3DVERTEXTEXTURESAMPLER3    (WINED3DDMAPSAMPLER + 4)

/* DirectDraw types */

typedef struct _WINEDDCOLORKEY
{
    DWORD       dwColorSpaceLowValue;           /* low boundary of color space that is to
                                                 * be treated as Color Key, inclusive
                                                 */
    DWORD       dwColorSpaceHighValue;          /* high boundary of color space that is
                                                 * to be treated as Color Key, inclusive
                                                 */
} WINEDDCOLORKEY,*LPWINEDDCOLORKEY;

typedef struct _WINEDDBLTFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwDDFX;                         /* FX operations */
    DWORD       dwROP;                          /* Win32 raster operations */
    DWORD       dwDDROP;                        /* Raster operations new for DirectDraw */
    DWORD       dwRotationAngle;                /* Rotation angle for blt */
    DWORD       dwZBufferOpCode;                /* ZBuffer compares */
    DWORD       dwZBufferLow;                   /* Low limit of Z buffer */
    DWORD       dwZBufferHigh;                  /* High limit of Z buffer */
    DWORD       dwZBufferBaseDest;              /* Destination base value */
    DWORD       dwZDestConstBitDepth;           /* Bit depth used to specify Z constant for destination */
    union
    {
        DWORD   dwZDestConst;                   /* Constant to use as Z buffer for dest */
        struct IWineD3DSurface *lpDDSZBufferDest;      /* Surface to use as Z buffer for dest */
    } DUMMYUNIONNAME1;
    DWORD       dwZSrcConstBitDepth;            /* Bit depth used to specify Z constant for source */
    union
    {
        DWORD   dwZSrcConst;                    /* Constant to use as Z buffer for src */
        struct IWineD3DSurface *lpDDSZBufferSrc;/* Surface to use as Z buffer for src */
    } DUMMYUNIONNAME2;
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Alpha for edge blending */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as Alpha Channel */
        struct IWineD3DSurface *lpDDSAlphaDest; /* Surface to use as Alpha Channel */
    } DUMMYUNIONNAME3;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as Alpha Channel */
        struct IWineD3DSurface *lpDDSAlphaSrc;  /* Surface to use as Alpha Channel */
    } DUMMYUNIONNAME4;
    union
    {
        DWORD   dwFillColor;                    /* color in RGB or Palettized */
        DWORD   dwFillDepth;                    /* depth value for z-buffer */
	DWORD   dwFillPixel;			/* pixel val for RGBA or RGBZ */
        struct IWineD3DSurface *lpDDSPattern;   /* Surface to use as pattern */
    } DUMMYUNIONNAME5;
    WINEDDCOLORKEY  ddckDestColorkey;          /* DestColorkey override */
    WINEDDCOLORKEY  ddckSrcColorkey;           /* SrcColorkey override */
} WINEDDBLTFX,*LPWINEDDBLTFX;

typedef struct _WINEDDOVERLAYFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Constant to use as alpha for edge blend */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as alpha channel for dest */
        struct IWineD3DSurface *lpDDSAlphaDest; /* Surface to use as alpha channel for dest */
    } DUMMYUNIONNAME1;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as alpha channel for src */
        struct IWineD3DSurface *lpDDSAlphaSrc;  /* Surface to use as alpha channel for src */
    } DUMMYUNIONNAME2;
    WINEDDCOLORKEY  dckDestColorkey;            /* DestColorkey override */
    WINEDDCOLORKEY  dckSrcColorkey;             /* DestColorkey override */
    DWORD       dwDDFX;                         /* Overlay FX */
    DWORD       dwFlags;                        /* flags */
} WINEDDOVERLAYFX;

/* dwDDFX */
/* arithmetic stretching along y axis */
#define WINEDDBLTFX_ARITHSTRETCHY               0x00000001
/* mirror on y axis */
#define WINEDDBLTFX_MIRRORLEFTRIGHT             0x00000002
/* mirror on x axis */
#define WINEDDBLTFX_MIRRORUPDOWN                0x00000004
/* do not tear */
#define WINEDDBLTFX_NOTEARING                   0x00000008
/* 180 degrees clockwise rotation */
#define WINEDDBLTFX_ROTATE180                   0x00000010
/* 270 degrees clockwise rotation */
#define WINEDDBLTFX_ROTATE270                   0x00000020
/* 90 degrees clockwise rotation */
#define WINEDDBLTFX_ROTATE90                    0x00000040
/* dwZBufferLow and dwZBufferHigh specify limits to the copied Z values */
#define WINEDDBLTFX_ZBUFFERRANGE                0x00000080
/* add dwZBufferBaseDest to every source z value before compare */
#define WINEDDBLTFX_ZBUFFERBASEDEST             0x00000100

/* dwFlags for Blt* */
#define WINEDDBLT_ALPHADEST                     0x00000001
#define WINEDDBLT_ALPHADESTCONSTOVERRIDE        0x00000002
#define WINEDDBLT_ALPHADESTNEG                  0x00000004
#define WINEDDBLT_ALPHADESTSURFACEOVERRIDE      0x00000008
#define WINEDDBLT_ALPHAEDGEBLEND                0x00000010
#define WINEDDBLT_ALPHASRC                      0x00000020
#define WINEDDBLT_ALPHASRCCONSTOVERRIDE         0x00000040
#define WINEDDBLT_ALPHASRCNEG                   0x00000080
#define WINEDDBLT_ALPHASRCSURFACEOVERRIDE       0x00000100
#define WINEDDBLT_ASYNC                         0x00000200
#define WINEDDBLT_COLORFILL                     0x00000400
#define WINEDDBLT_DDFX                          0x00000800
#define WINEDDBLT_DDROPS                        0x00001000
#define WINEDDBLT_KEYDEST                       0x00002000
#define WINEDDBLT_KEYDESTOVERRIDE               0x00004000
#define WINEDDBLT_KEYSRC                        0x00008000
#define WINEDDBLT_KEYSRCOVERRIDE                0x00010000
#define WINEDDBLT_ROP                           0x00020000
#define WINEDDBLT_ROTATIONANGLE                 0x00040000
#define WINEDDBLT_ZBUFFER                       0x00080000
#define WINEDDBLT_ZBUFFERDESTCONSTOVERRIDE      0x00100000
#define WINEDDBLT_ZBUFFERDESTOVERRIDE           0x00200000
#define WINEDDBLT_ZBUFFERSRCCONSTOVERRIDE       0x00400000
#define WINEDDBLT_ZBUFFERSRCOVERRIDE            0x00800000
#define WINEDDBLT_WAIT                          0x01000000
#define WINEDDBLT_DEPTHFILL                     0x02000000
#define WINEDDBLT_DONOTWAIT                     0x08000000

/* dwTrans for BltFast */
#define WINEDDBLTFAST_NOCOLORKEY                0x00000000
#define WINEDDBLTFAST_SRCCOLORKEY               0x00000001
#define WINEDDBLTFAST_DESTCOLORKEY              0x00000002
#define WINEDDBLTFAST_WAIT                      0x00000010
#define WINEDDBLTFAST_DONOTWAIT                 0x00000020

/* DDCAPS.dwPalCaps */
#define WINEDDPCAPS_4BIT                        0x00000001
#define WINEDDPCAPS_8BITENTRIES                 0x00000002
#define WINEDDPCAPS_8BIT                        0x00000004
#define WINEDDPCAPS_INITIALIZE                  0x00000008
#define WINEDDPCAPS_PRIMARYSURFACE              0x00000010
#define WINEDDPCAPS_PRIMARYSURFACELEFT          0x00000020
#define WINEDDPCAPS_ALLOW256                    0x00000040
#define WINEDDPCAPS_VSYNC                       0x00000080
#define WINEDDPCAPS_1BIT                        0x00000100
#define WINEDDPCAPS_2BIT                        0x00000200
#define WINEDDPCAPS_ALPHA                       0x00000400

/* DDSURFACEDESC.dwFlags */
#define WINEDDSD_CAPS                           0x00000001
#define WINEDDSD_HEIGHT                         0x00000002
#define WINEDDSD_WIDTH                          0x00000004
#define WINEDDSD_PITCH                          0x00000008
#define WINEDDSD_BACKBUFFERCOUNT                0x00000020
#define WINEDDSD_ZBUFFERBITDEPTH                0x00000040
#define WINEDDSD_ALPHABITDEPTH                  0x00000080
#define WINEDDSD_LPSURFACE                      0x00000800
#define WINEDDSD_PIXELFORMAT                    0x00001000
#define WINEDDSD_CKDESTOVERLAY                  0x00002000
#define WINEDDSD_CKDESTBLT                      0x00004000
#define WINEDDSD_CKSRCOVERLAY                   0x00008000
#define WINEDDSD_CKSRCBLT                       0x00010000
#define WINEDDSD_MIPMAPCOUNT                    0x00020000
#define WINEDDSD_REFRESHRATE                    0x00040000
#define WINEDDSD_LINEARSIZE                     0x00080000
#define WINEDDSD_TEXTURESTAGE                   0x00100000
#define WINEDDSD_FVF                            0x00200000
#define WINEDDSD_SRCVBHANDLE                    0x00400000
#define WINEDDSD_ALL                            0x007ff9ee

/* Set/Get Colour Key Flags */
#define WINEDDCKEY_COLORSPACE                   0x00000001  /* Struct is single colour space */
#define WINEDDCKEY_DESTBLT                      0x00000002  /* To be used as dest for blt */
#define WINEDDCKEY_DESTOVERLAY                  0x00000004  /* To be used as dest for CK overlays */
#define WINEDDCKEY_SRCBLT                       0x00000008  /* To be used as src for blt */
#define WINEDDCKEY_SRCOVERLAY                   0x00000010  /* To be used as src for CK overlays */

/* dwFlags for GetBltStatus */
#define WINEDDGBS_CANBLT                        0x00000001
#define WINEDDGBS_ISBLTDONE                     0x00000002

/* dwFlags for GetFlipStatus */
#define WINEDDGFS_CANFLIP                       1L
#define WINEDDGFS_ISFLIPDONE                    2L

/* dwFlags for Flip */
#define WINEDDFLIP_WAIT                         0x00000001
#define WINEDDFLIP_EVEN                         0x00000002 /* only valid for overlay */
#define WINEDDFLIP_ODD                          0x00000004 /* only valid for overlay */
#define WINEDDFLIP_NOVSYNC                      0x00000008
#define WINEDDFLIP_STEREO                       0x00000010
#define WINEDDFLIP_DONOTWAIT                    0x00000020
#define WINEDDFLIP_INTERVAL2                    0x02000000
#define WINEDDFLIP_INTERVAL3                    0x03000000
#define WINEDDFLIP_INTERVAL4                    0x04000000

#endif
