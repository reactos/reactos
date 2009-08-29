/*
 * Copyright (C) 2002-2003 Jason Edmeades
 *                         Raphael Junqueira
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

#ifndef __WINE_D3D9CAPS_H
#define __WINE_D3D9CAPS_H

/*
 * Definitions
 */
#define D3DCAPS_READ_SCANLINE 0x20000

#define D3DCURSORCAPS_COLOR   1
#define D3DCURSORCAPS_LOWRES  2


#define D3DDEVCAPS2_STREAMOFFSET                        0x00000001L
#define D3DDEVCAPS2_DMAPNPATCH                          0x00000002L
#define D3DDEVCAPS2_ADAPTIVETESSRTPATCH                 0x00000004L
#define D3DDEVCAPS2_ADAPTIVETESSNPATCH                  0x00000008L
#define D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES       0x00000010L
#define D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH                0x00000020L
#define D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET  0x00000040L

#define D3DDEVCAPS_EXECUTESYSTEMMEMORY     0x0000010
#define D3DDEVCAPS_EXECUTEVIDEOMEMORY      0x0000020
#define D3DDEVCAPS_TLVERTEXSYSTEMMEMORY    0x0000040
#define D3DDEVCAPS_TLVERTEXVIDEOMEMORY     0x0000080
#define D3DDEVCAPS_TEXTURESYSTEMMEMORY     0x0000100
#define D3DDEVCAPS_TEXTUREVIDEOMEMORY      0x0000200
#define D3DDEVCAPS_DRAWPRIMTLVERTEX        0x0000400
#define D3DDEVCAPS_CANRENDERAFTERFLIP      0x0000800
#define D3DDEVCAPS_TEXTURENONLOCALVIDMEM   0x0001000
#define D3DDEVCAPS_DRAWPRIMITIVES2         0x0002000
#define D3DDEVCAPS_SEPARATETEXTUREMEMORIES 0x0004000
#define D3DDEVCAPS_DRAWPRIMITIVES2EX       0x0008000
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT     0x0010000
#define D3DDEVCAPS_CANBLTSYSTONONLOCAL     0x0020000
#define D3DDEVCAPS_HWRASTERIZATION         0x0080000
#define D3DDEVCAPS_PUREDEVICE              0x0100000
#define D3DDEVCAPS_QUINTICRTPATCHES        0x0200000
#define D3DDEVCAPS_RTPATCHES               0x0400000
#define D3DDEVCAPS_RTPATCHHANDLEZERO       0x0800000
#define D3DDEVCAPS_NPATCHES                0x1000000

#define D3DFVFCAPS_TEXCOORDCOUNTMASK  0x00FFFF
#define D3DFVFCAPS_DONOTSTRIPELEMENTS 0x080000
#define D3DFVFCAPS_PSIZE              0x100000

#define D3DLINECAPS_TEXTURE           0x01
#define D3DLINECAPS_ZTEST             0x02
#define D3DLINECAPS_BLEND             0x04
#define D3DLINECAPS_ALPHACMP          0x08
#define D3DLINECAPS_FOG               0x10
#define D3DLINECAPS_ANTIALIAS         0x20

#define D3DPBLENDCAPS_ZERO            0x00000001
#define D3DPBLENDCAPS_ONE             0x00000002
#define D3DPBLENDCAPS_SRCCOLOR        0x00000004
#define D3DPBLENDCAPS_INVSRCCOLOR     0x00000008
#define D3DPBLENDCAPS_SRCALPHA        0x00000010
#define D3DPBLENDCAPS_INVSRCALPHA     0x00000020
#define D3DPBLENDCAPS_DESTALPHA       0x00000040
#define D3DPBLENDCAPS_INVDESTALPHA    0x00000080
#define D3DPBLENDCAPS_DESTCOLOR       0x00000100
#define D3DPBLENDCAPS_INVDESTCOLOR    0x00000200
#define D3DPBLENDCAPS_SRCALPHASAT     0x00000400
#define D3DPBLENDCAPS_BOTHSRCALPHA    0x00000800
#define D3DPBLENDCAPS_BOTHINVSRCALPHA 0x00001000
#define D3DPBLENDCAPS_BLENDFACTOR     0x00002000

#define D3DPCMPCAPS_NEVER        0x01
#define D3DPCMPCAPS_LESS         0x02
#define D3DPCMPCAPS_EQUAL        0x04
#define D3DPCMPCAPS_LESSEQUAL    0x08
#define D3DPCMPCAPS_GREATER      0x10
#define D3DPCMPCAPS_NOTEQUAL     0x20
#define D3DPCMPCAPS_GREATEREQUAL 0x40
#define D3DPCMPCAPS_ALWAYS       0x80

#define D3DPMISCCAPS_MASKZ                      0x00000002L
#define D3DPMISCCAPS_LINEPATTERNREP             0x00000004L
#define D3DPMISCCAPS_CULLNONE                   0x00000010L
#define D3DPMISCCAPS_CULLCW                     0x00000020L
#define D3DPMISCCAPS_CULLCCW                    0x00000040L
#define D3DPMISCCAPS_COLORWRITEENABLE           0x00000080L
#define D3DPMISCCAPS_CLIPPLANESCALEDPOINTS      0x00000100L
#define D3DPMISCCAPS_CLIPTLVERTS                0x00000200L
#define D3DPMISCCAPS_TSSARGTEMP                 0x00000400L
#define D3DPMISCCAPS_BLENDOP                    0x00000800L
#define D3DPMISCCAPS_NULLREFERENCE              0x00001000L
#define D3DPMISCCAPS_INDEPENDENTWRITEMASKS      0x00004000L
#define D3DPMISCCAPS_PERSTAGECONSTANT           0x00008000L
#define D3DPMISCCAPS_FOGANDSPECULARALPHA        0x00010000L
#define D3DPMISCCAPS_SEPARATEALPHABLEND         0x00020000L
#define D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS    0x00040000L
#define D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING 0x00080000L
#define D3DPMISCCAPS_FOGVERTEXCLAMPED           0x00100000L


#define D3DPRASTERCAPS_DITHER                     0x00000001L
#define D3DPRASTERCAPS_PAT                        0x00000008L
#define D3DPRASTERCAPS_ZTEST                      0x00000010L
#define D3DPRASTERCAPS_FOGVERTEX                  0x00000080L
#define D3DPRASTERCAPS_FOGTABLE                   0x00000100L
#define D3DPRASTERCAPS_ANTIALIASEDGES             0x00001000L
#define D3DPRASTERCAPS_MIPMAPLODBIAS              0x00002000L
#define D3DPRASTERCAPS_ZBIAS                      0x00004000L
#define D3DPRASTERCAPS_ZBUFFERLESSHSR             0x00008000L
#define D3DPRASTERCAPS_FOGRANGE                   0x00010000L
#define D3DPRASTERCAPS_ANISOTROPY                 0x00020000L
#define D3DPRASTERCAPS_WBUFFER                    0x00040000L
#define D3DPRASTERCAPS_WFOG                       0x00100000L
#define D3DPRASTERCAPS_ZFOG                       0x00200000L
#define D3DPRASTERCAPS_COLORPERSPECTIVE           0x00400000L
#define D3DPRASTERCAPS_SCISSORTEST                0x01000000L
#define D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS        0x02000000L
#define D3DPRASTERCAPS_DEPTHBIAS                  0x04000000L 
#define D3DPRASTERCAPS_MULTISAMPLE_TOGGLE         0x08000000L

#define D3DPRESENT_INTERVAL_DEFAULT               0x00000000
#define D3DPRESENT_INTERVAL_ONE                   0x00000001
#define D3DPRESENT_INTERVAL_TWO                   0x00000002
#define D3DPRESENT_INTERVAL_THREE                 0x00000004
#define D3DPRESENT_INTERVAL_FOUR                  0x00000008
#define D3DPRESENT_INTERVAL_IMMEDIATE             0x80000000

#define D3DPSHADECAPS_COLORGOURAUDRGB             0x00008
#define D3DPSHADECAPS_SPECULARGOURAUDRGB          0x00200
#define D3DPSHADECAPS_ALPHAGOURAUDBLEND           0x04000
#define D3DPSHADECAPS_FOGGOURAUD                  0x80000

#define D3DPTADDRESSCAPS_WRAP                     0x01
#define D3DPTADDRESSCAPS_MIRROR                   0x02
#define D3DPTADDRESSCAPS_CLAMP                    0x04
#define D3DPTADDRESSCAPS_BORDER                   0x08
#define D3DPTADDRESSCAPS_INDEPENDENTUV            0x10
#define D3DPTADDRESSCAPS_MIRRORONCE               0x20

#define D3DPTEXTURECAPS_PERSPECTIVE              0x00000001L
#define D3DPTEXTURECAPS_POW2                     0x00000002L
#define D3DPTEXTURECAPS_ALPHA                    0x00000004L
#define D3DPTEXTURECAPS_SQUAREONLY               0x00000020L
#define D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE 0x00000040L
#define D3DPTEXTURECAPS_ALPHAPALETTE             0x00000080L
#define D3DPTEXTURECAPS_NONPOW2CONDITIONAL       0x00000100L
#define D3DPTEXTURECAPS_PROJECTED                0x00000400L
#define D3DPTEXTURECAPS_CUBEMAP                  0x00000800L
#define D3DPTEXTURECAPS_VOLUMEMAP                0x00002000L
#define D3DPTEXTURECAPS_MIPMAP                   0x00004000L
#define D3DPTEXTURECAPS_MIPVOLUMEMAP             0x00008000L
#define D3DPTEXTURECAPS_MIPCUBEMAP               0x00010000L
#define D3DPTEXTURECAPS_CUBEMAP_POW2             0x00020000L
#define D3DPTEXTURECAPS_VOLUMEMAP_POW2           0x00040000L
#define D3DPTEXTURECAPS_NOPROJECTEDBUMPENV       0x00200000L

#define D3DPTFILTERCAPS_MINFPOINT                0x00000100
#define D3DPTFILTERCAPS_MINFLINEAR               0x00000200
#define D3DPTFILTERCAPS_MINFANISOTROPIC          0x00000400
#define D3DPTFILTERCAPS_MINFPYRAMIDALQUAD        0x00000800
#define D3DPTFILTERCAPS_MINFGAUSSIANQUAD         0x00001000
#define D3DPTFILTERCAPS_MIPFPOINT                0x00010000
#define D3DPTFILTERCAPS_MIPFLINEAR               0x00020000
#define D3DPTFILTERCAPS_MAGFPOINT                0x01000000
#define D3DPTFILTERCAPS_MAGFLINEAR               0x02000000
#define D3DPTFILTERCAPS_MAGFANISOTROPIC          0x04000000
#define D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD        0x08000000
#define D3DPTFILTERCAPS_MAGFGAUSSIANQUAD         0x10000000

#define D3DSTENCILCAPS_KEEP                      0x01
#define D3DSTENCILCAPS_ZERO                      0x02
#define D3DSTENCILCAPS_REPLACE                   0x04
#define D3DSTENCILCAPS_INCRSAT                   0x08
#define D3DSTENCILCAPS_DECRSAT                   0x10
#define D3DSTENCILCAPS_INVERT                    0x20
#define D3DSTENCILCAPS_INCR                      0x40
#define D3DSTENCILCAPS_DECR                      0x80
#define D3DSTENCILCAPS_TWOSIDED                  0x100

#define D3DTEXOPCAPS_DISABLE                     0x0000001
#define D3DTEXOPCAPS_SELECTARG1                  0x0000002
#define D3DTEXOPCAPS_SELECTARG2                  0x0000004
#define D3DTEXOPCAPS_MODULATE                    0x0000008
#define D3DTEXOPCAPS_MODULATE2X                  0x0000010
#define D3DTEXOPCAPS_MODULATE4X                  0x0000020
#define D3DTEXOPCAPS_ADD                         0x0000040
#define D3DTEXOPCAPS_ADDSIGNED                   0x0000080
#define D3DTEXOPCAPS_ADDSIGNED2X                 0x0000100
#define D3DTEXOPCAPS_SUBTRACT                    0x0000200
#define D3DTEXOPCAPS_ADDSMOOTH                   0x0000400
#define D3DTEXOPCAPS_BLENDDIFFUSEALPHA           0x0000800
#define D3DTEXOPCAPS_BLENDTEXTUREALPHA           0x0001000
#define D3DTEXOPCAPS_BLENDFACTORALPHA            0x0002000
#define D3DTEXOPCAPS_BLENDTEXTUREALPHAPM         0x0004000
#define D3DTEXOPCAPS_BLENDCURRENTALPHA           0x0008000
#define D3DTEXOPCAPS_PREMODULATE                 0x0010000
#define D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR      0x0020000
#define D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA      0x0040000
#define D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR   0x0080000
#define D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA   0x0100000
#define D3DTEXOPCAPS_BUMPENVMAP                  0x0200000
#define D3DTEXOPCAPS_BUMPENVMAPLUMINANCE         0x0400000
#define D3DTEXOPCAPS_DOTPRODUCT3                 0x0800000
#define D3DTEXOPCAPS_MULTIPLYADD                 0x1000000
#define D3DTEXOPCAPS_LERP                        0x2000000

#define D3DVTXPCAPS_TEXGEN                         0x00000001L
#define D3DVTXPCAPS_MATERIALSOURCE7                0x00000002L
#define D3DVTXPCAPS_DIRECTIONALLIGHTS              0x00000008L
#define D3DVTXPCAPS_POSITIONALLIGHTS               0x00000010L
#define D3DVTXPCAPS_LOCALVIEWER                    0x00000020L
#define D3DVTXPCAPS_TWEENING                       0x00000040L
#define D3DVTXPCAPS_TEXGEN_SPHEREMAP               0x00000100L
#define D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER       0x00000200L

#define D3DDTCAPS_UBYTE4                           0x00000001L
#define D3DDTCAPS_UBYTE4N                          0x00000002L
#define D3DDTCAPS_SHORT2N                          0x00000004L
#define D3DDTCAPS_SHORT4N                          0x00000008L
#define D3DDTCAPS_USHORT2N                         0x00000010L
#define D3DDTCAPS_USHORT4N                         0x00000020L
#define D3DDTCAPS_UDEC3                            0x00000040L
#define D3DDTCAPS_DEC3N                            0x00000080L
#define D3DDTCAPS_FLOAT16_2                        0x00000100L
#define D3DDTCAPS_FLOAT16_4                        0x00000200L

#define D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD  0x00000020L
#define D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION       0x00000080L
#define D3DCAPS3_COPY_TO_VIDMEM                    0x00000100L 
#define D3DCAPS3_COPY_TO_SYSTEMMEM                 0x00000200L
#define D3DCAPS3_RESERVED                          0x8000001FL

#define D3DCAPS2_NO2DDURING3DSCENE                 0x00000002L
#define D3DCAPS2_FULLSCREENGAMMA                   0x00020000L
#define D3DCAPS2_CANRENDERWINDOWED                 0x00080000L
#define D3DCAPS2_CANCALIBRATEGAMMA                 0x00100000L
#define D3DCAPS2_RESERVED                          0x02000000L
#define D3DCAPS2_CANMANAGERESOURCE                 0x10000000L
#define D3DCAPS2_DYNAMICTEXTURES                   0x20000000L
#define D3DCAPS2_CANAUTOGENMIPMAP                  0x40000000L


#define D3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH  24
#define D3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH  0
#define D3DVS20_MAX_NUMTEMPS                 32
#define D3DVS20_MIN_NUMTEMPS                 12
#define D3DVS20_MAX_STATICFLOWCONTROLDEPTH   4
#define D3DVS20_MIN_STATICFLOWCONTROLDEPTH   1

#define D3DVS20CAPS_PREDICATION              (1 << 0)

#define D3DPS20CAPS_ARBITRARYSWIZZLE         (1 << 0)
#define D3DPS20CAPS_GRADIENTINSTRUCTIONS     (1 << 1)
#define D3DPS20CAPS_PREDICATION              (1 << 2)
#define D3DPS20CAPS_NODEPENDENTREADLIMIT     (1 << 3)
#define D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT    (1 << 4)

#define D3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH  24
#define D3DPS20_MIN_DYNAMICFLOWCONTROLDEPTH  0
#define D3DPS20_MAX_NUMTEMPS                 32
#define D3DPS20_MIN_NUMTEMPS                 12
#define D3DPS20_MAX_STATICFLOWCONTROLDEPTH   4
#define D3DPS20_MIN_STATICFLOWCONTROLDEPTH   0
#define D3DPS20_MAX_NUMINSTRUCTIONSLOTS      512
#define D3DPS20_MIN_NUMINSTRUCTIONSLOTS      96

#define D3DMIN30SHADERINSTRUCTIONS          512
#define D3DMAX30SHADERINSTRUCTIONS          32768


typedef struct _D3DVSHADERCAPS2_0 {
  DWORD  Caps;
  INT    DynamicFlowControlDepth;
  INT    NumTemps;
  INT    StaticFlowControlDepth;
} D3DVSHADERCAPS2_0;

typedef struct _D3DPSHADERCAPS2_0 {
  DWORD  Caps;
  INT    DynamicFlowControlDepth;
  INT    NumTemps;
  INT    StaticFlowControlDepth;
  INT    NumInstructionSlots;
} D3DPSHADERCAPS2_0;

/*
 * The d3dcaps9 structure
 */
typedef struct _D3DCAPS9 {
  D3DDEVTYPE          DeviceType;
  UINT                AdapterOrdinal;
  
  DWORD               Caps;
  DWORD               Caps2;
  DWORD               Caps3;
  DWORD               PresentationIntervals;
  
  DWORD               CursorCaps;
  
  DWORD               DevCaps;
  
  DWORD               PrimitiveMiscCaps;
  DWORD               RasterCaps;
  DWORD               ZCmpCaps;
  DWORD               SrcBlendCaps;
  DWORD               DestBlendCaps;
  DWORD               AlphaCmpCaps;
  DWORD               ShadeCaps;
  DWORD               TextureCaps;
  DWORD               TextureFilterCaps;
  DWORD               CubeTextureFilterCaps;
  DWORD               VolumeTextureFilterCaps;
  DWORD               TextureAddressCaps;
  DWORD               VolumeTextureAddressCaps;
  
  DWORD               LineCaps;
  
  DWORD               MaxTextureWidth, MaxTextureHeight;
  DWORD               MaxVolumeExtent;
  
  DWORD               MaxTextureRepeat;
  DWORD               MaxTextureAspectRatio;
  DWORD               MaxAnisotropy;
  float               MaxVertexW;
  
  float               GuardBandLeft;
  float               GuardBandTop;
  float               GuardBandRight;
  float               GuardBandBottom;
  
  float               ExtentsAdjust;
  DWORD               StencilCaps;
  
  DWORD               FVFCaps;
  DWORD               TextureOpCaps;
  DWORD               MaxTextureBlendStages;
  DWORD               MaxSimultaneousTextures;
  
  DWORD               VertexProcessingCaps;
  DWORD               MaxActiveLights;
  DWORD               MaxUserClipPlanes;
  DWORD               MaxVertexBlendMatrices;
  DWORD               MaxVertexBlendMatrixIndex;
  
  float               MaxPointSize;
  
  DWORD               MaxPrimitiveCount;
  DWORD               MaxVertexIndex;
  DWORD               MaxStreams;
  DWORD               MaxStreamStride;
  
  DWORD               VertexShaderVersion;
  DWORD               MaxVertexShaderConst;
  
  DWORD               PixelShaderVersion;
  float               PixelShader1xMaxValue;

  /* DX 9 */
  DWORD               DevCaps2;

  float               MaxNpatchTessellationLevel;
  DWORD               Reserved5;

  UINT                MasterAdapterOrdinal;   
  UINT                AdapterOrdinalInGroup;  
  UINT                NumberOfAdaptersInGroup;
  DWORD               DeclTypes;              
  DWORD               NumSimultaneousRTs;     
  DWORD               StretchRectFilterCaps;  
  D3DVSHADERCAPS2_0   VS20Caps;
  D3DPSHADERCAPS2_0   PS20Caps;
  DWORD               VertexTextureFilterCaps;
  DWORD               MaxVShaderInstructionsExecuted;
  DWORD               MaxPShaderInstructionsExecuted;
  DWORD               MaxVertexShader30InstructionSlots; 
  DWORD               MaxPixelShader30InstructionSlots;

} D3DCAPS9;

#endif
