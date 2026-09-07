// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*==========================================================================;
 *

 *
 *  File:       d3d9caps.h
 *  Content:    Direct3D capabilities include file
 *
 ***************************************************************************/

#ifndef _d3d9CAPS_H
#define _d3d9CAPS_H

#ifndef DIRECT3D_VERSION
#define DIRECT3D_VERSION         0x0900
#endif  //DIRECT3D_VERSION

// include this file content only if compiling for DX9 interfaces
#if(DIRECT3D_VERSION >= 0x0900)

#if defined(_X86_) || defined(_IA64)
#pragma pack(4)
#endif

typedef struct _D3DVSHADERCAPS2_0
{
        DWORD Caps;
        INT DynamicFlowControlDepth;
        INT NumTemps;
        INT StaticFlowControlDepth;
} D3DVSHADERCAPS2_0;

#define D3DVS20CAPS_PREDICATION             (1<<0)

#define D3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH  24
#define D3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH  0
#define D3DVS20_MAX_NUMTEMPS    32
#define D3DVS20_MIN_NUMTEMPS    12
#define D3DVS20_MAX_STATICFLOWCONTROLDEPTH    4
#define D3DVS20_MIN_STATICFLOWCONTROLDEPTH    1

typedef struct _D3DPSHADERCAPS2_0
{
    DWORD Caps;
    INT DynamicFlowControlDepth;
    INT NumTemps;
    INT StaticFlowControlDepth;
    INT NumInstructionSlots;
} D3DPSHADERCAPS2_0;

#define D3DPS20CAPS_ARBITRARYSWIZZLE        (1<<0)
#define D3DPS20CAPS_GRADIENTINSTRUCTIONS    (1<<1)
#define D3DPS20CAPS_PREDICATION             (1<<2)
#define D3DPS20CAPS_NODEPENDENTREADLIMIT    (1<<3)
#define D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT   (1<<4)

#define D3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH    24
#define D3DPS20_MIN_DYNAMICFLOWCONTROLDEPTH    0
#define D3DPS20_MAX_NUMTEMPS    32
#define D3DPS20_MIN_NUMTEMPS    12
#define D3DPS20_MAX_STATICFLOWCONTROLDEPTH    4
#define D3DPS20_MIN_STATICFLOWCONTROLDEPTH    0
#define D3DPS20_MAX_NUMINSTRUCTIONSLOTS    512
#define D3DPS20_MIN_NUMINSTRUCTIONSLOTS    96

#define D3DMIN30SHADERINSTRUCTIONS 512
#define D3DMAX30SHADERINSTRUCTIONS 32768

typedef struct _D3DCAPS9
{
    /* Device Info */
    D3DDEVTYPE  DeviceType;
    UINT        AdapterOrdinal;

    /* Caps from DX7 Draw */
    DWORD   Caps;
    DWORD   Caps2;
    DWORD   Caps3;
    DWORD   PresentationIntervals;

    /* Cursor Caps */
    DWORD   CursorCaps;

    /* 3D Device Caps */
    DWORD   DevCaps;

    DWORD   PrimitiveMiscCaps;
    DWORD   RasterCaps;
    DWORD   ZCmpCaps;
    DWORD   SrcBlendCaps;
    DWORD   DestBlendCaps;
    DWORD   AlphaCmpCaps;
    DWORD   ShadeCaps;
    DWORD   TextureCaps;
    DWORD   TextureFilterCaps;          // D3DPTFILTERCAPS for IDirect3DTexture9's
    DWORD   CubeTextureFilterCaps;      // D3DPTFILTERCAPS for IDirect3DCubeTexture9's
    DWORD   VolumeTextureFilterCaps;    // D3DPTFILTERCAPS for IDirect3DVolumeTexture9's
    DWORD   TextureAddressCaps;         // D3DPTADDRESSCAPS for IDirect3DTexture9's
    DWORD   VolumeTextureAddressCaps;   // D3DPTADDRESSCAPS for IDirect3DVolumeTexture9's

    DWORD   LineCaps;                   // D3DLINECAPS

    DWORD   MaxTextureWidth, MaxTextureHeight;
    DWORD   MaxVolumeExtent;

    DWORD   MaxTextureRepeat;
    DWORD   MaxTextureAspectRatio;
    DWORD   MaxAnisotropy;
    float   MaxVertexW;

    float   GuardBandLeft;
    float   GuardBandTop;
    float   GuardBandRight;
    float   GuardBandBottom;

    float   ExtentsAdjust;
    DWORD   StencilCaps;

    DWORD   FVFCaps;
    DWORD   TextureOpCaps;
    DWORD   MaxTextureBlendStages;
    DWORD   MaxSimultaneousTextures;

    DWORD   VertexProcessingCaps;
    DWORD   MaxActiveLights;
    DWORD   MaxUserClipPlanes;
    DWORD   MaxVertexBlendMatrices;
    DWORD   MaxVertexBlendMatrixIndex;

    float   MaxPointSize;

    DWORD   MaxPrimitiveCount;          // max number of primitives per DrawPrimitive call
    DWORD   MaxVertexIndex;
    DWORD   MaxStreams;
    DWORD   MaxStreamStride;            // max stride for SetStreamSource

    DWORD   VertexShaderVersion;
    DWORD   MaxVertexShaderConst;       // number of vertex shader constant registers

    DWORD   PixelShaderVersion;
    float   PixelShader1xMaxValue;      // max value storable in registers of ps.1.x shaders

    // Here are the DX9 specific ones
    DWORD   DevCaps2;

    float   MaxNpatchTessellationLevel;
    DWORD   Reserved5;

    UINT    MasterAdapterOrdinal;       // ordinal of master adaptor for adapter group
    UINT    AdapterOrdinalInGroup;      // ordinal inside the adapter group
    UINT    NumberOfAdaptersInGroup;    // number of adapters in this adapter group (only if master)
    DWORD   DeclTypes;                  // Data types, supported in vertex declarations
    DWORD   NumSimultaneousRTs;         // Will be at least 1
    DWORD   StretchRectFilterCaps;      // Filter caps supported by StretchRect
    D3DVSHADERCAPS2_0 VS20Caps;
    D3DPSHADERCAPS2_0 PS20Caps;
    DWORD   VertexTextureFilterCaps;    // D3DPTFILTERCAPS for IDirect3DTexture9's for texture, used in vertex shaders
    DWORD   MaxVShaderInstructionsExecuted; // maximum number of vertex shader instructions that can be executed
    DWORD   MaxPShaderInstructionsExecuted; // maximum number of pixel shader instructions that can be executed
    DWORD   MaxVertexShader30InstructionSlots; 
    DWORD   MaxPixelShader30InstructionSlots;
} D3DCAPS9;

//
// BIT DEFINES FOR D3DCAPS9 DWORD MEMBERS
//

//
// Caps
//
#define D3DCAPS_READ_SCANLINE           0x00020000L

//
// Caps2
//
#define D3DCAPS2_FULLSCREENGAMMA        0x00020000L
#define D3DCAPS2_CANCALIBRATEGAMMA      0x00100000L
#define D3DCAPS2_RESERVED               0x02000000L
#define D3DCAPS2_CANMANAGERESOURCE      0x10000000L
#define D3DCAPS2_DYNAMICTEXTURES        0x20000000L
#define D3DCAPS2_CANAUTOGENMIPMAP       0x40000000L

//
// Caps3
//
#define D3DCAPS3_RESERVED               0x8000001fL

// Indicates that the device can respect the ALPHABLENDENABLE render state
// when fullscreen while using the FLIP or DISCARD swap effect.
// COPY and COPYVSYNC swap effects work whether or not this flag is set.
#define D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD   0x00000020L

// Indicates that the device can perform a gamma correction from 
// a windowed back buffer containing linear content to the sRGB desktop.
#define D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION 0x00000080L

#define D3DCAPS3_COPY_TO_VIDMEM         0x00000100L /* Device can acclerate copies from sysmem to local vidmem */
#define D3DCAPS3_COPY_TO_SYSTEMMEM      0x00000200L /* Device can acclerate copies from local vidmem to sysmem */


//
// PresentationIntervals
//
#define D3DPRESENT_INTERVAL_DEFAULT     0x00000000L
#define D3DPRESENT_INTERVAL_ONE         0x00000001L
#define D3DPRESENT_INTERVAL_TWO         0x00000002L
#define D3DPRESENT_INTERVAL_THREE       0x00000004L
#define D3DPRESENT_INTERVAL_FOUR        0x00000008L
#define D3DPRESENT_INTERVAL_IMMEDIATE   0x80000000L

//
// CursorCaps
//
// Driver supports HW color cursor in at least hi-res modes(height >=400)
#define D3DCURSORCAPS_COLOR             0x00000001L
// Driver supports HW cursor also in low-res modes(height < 400)
#define D3DCURSORCAPS_LOWRES            0x00000002L

//
// DevCaps
//
#define D3DDEVCAPS_EXECUTESYSTEMMEMORY  0x00000010L /* Device can use execute buffers from system memory */
#define D3DDEVCAPS_EXECUTEVIDEOMEMORY   0x00000020L /* Device can use execute buffers from video memory */
#define D3DDEVCAPS_TLVERTEXSYSTEMMEMORY 0x00000040L /* Device can use TL buffers from system memory */
#define D3DDEVCAPS_TLVERTEXVIDEOMEMORY  0x00000080L /* Device can use TL buffers from video memory */
#define D3DDEVCAPS_TEXTURESYSTEMMEMORY  0x00000100L /* Device can texture from system memory */
#define D3DDEVCAPS_TEXTUREVIDEOMEMORY   0x00000200L /* Device can texture from device memory */
#define D3DDEVCAPS_DRAWPRIMTLVERTEX     0x00000400L /* Device can draw TLVERTEX primitives */
#define D3DDEVCAPS_CANRENDERAFTERFLIP   0x00000800L /* Device can render without waiting for flip to complete */
#define D3DDEVCAPS_TEXTURENONLOCALVIDMEM 0x00001000L /* Device can texture from nonlocal video memory */
#define D3DDEVCAPS_DRAWPRIMITIVES2      0x00002000L /* Device can support DrawPrimitives2 */
#define D3DDEVCAPS_SEPARATETEXTUREMEMORIES 0x00004000L /* Device is texturing from separate memory pools */
#define D3DDEVCAPS_DRAWPRIMITIVES2EX    0x00008000L /* Device can support Extended DrawPrimitives2 i.e. DX7 compliant driver*/
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT  0x00010000L /* Device can support transformation and lighting in hardware and DRAWPRIMITIVES2EX must be also */
#define D3DDEVCAPS_CANBLTSYSTONONLOCAL  0x00020000L /* Device supports a Tex Blt from system memory to non-local vidmem */
#define D3DDEVCAPS_HWRASTERIZATION      0x00080000L /* Device has HW acceleration for rasterization */
#define D3DDEVCAPS_PUREDEVICE           0x00100000L /* Device supports D3DCREATE_PUREDEVICE */
#define D3DDEVCAPS_QUINTICRTPATCHES     0x00200000L /* Device supports quintic Beziers and BSplines */
#define D3DDEVCAPS_RTPATCHES            0x00400000L /* Device supports Rect and Tri patches */
#define D3DDEVCAPS_RTPATCHHANDLEZERO    0x00800000L /* Indicates that RT Patches may be drawn efficiently using handle 0 */
#define D3DDEVCAPS_NPATCHES             0x01000000L /* Device supports N-Patches */

//
// PrimitiveMiscCaps
//
#define D3DPMISCCAPS_MASKZ              0x00000002L
#define D3DPMISCCAPS_CULLNONE           0x00000010L
#define D3DPMISCCAPS_CULLCW             0x00000020L
#define D3DPMISCCAPS_CULLCCW            0x00000040L
#define D3DPMISCCAPS_COLORWRITEENABLE   0x00000080L
#define D3DPMISCCAPS_CLIPPLANESCALEDPOINTS 0x00000100L /* Device correctly clips scaled points to clip planes */
#define D3DPMISCCAPS_CLIPTLVERTS        0x00000200L /* device will clip post-transformed vertex primitives */
#define D3DPMISCCAPS_TSSARGTEMP         0x00000400L /* device supports D3DTA_TEMP for temporary register */
#define D3DPMISCCAPS_BLENDOP            0x00000800L /* device supports D3DRS_BLENDOP */
#define D3DPMISCCAPS_NULLREFERENCE      0x00001000L /* Reference Device that doesnt render */
#define D3DPMISCCAPS_INDEPENDENTWRITEMASKS     0x00004000L /* Device supports independent write masks for MET or MRT */
#define D3DPMISCCAPS_PERSTAGECONSTANT   0x00008000L /* Device supports per-stage constants */
#define D3DPMISCCAPS_FOGANDSPECULARALPHA   0x00010000L /* Device supports separate fog and specular alpha (many devices
                                                          use the specular alpha channel to store fog factor) */
#define D3DPMISCCAPS_SEPARATEALPHABLEND         0x00020000L /* Device supports separate blend settings for the alpha channel */
#define D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS    0x00040000L /* Device supports different bit depths for MRT */
#define D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING 0x00080000L /* Device supports post-pixel shader operations for MRT */
#define D3DPMISCCAPS_FOGVERTEXCLAMPED           0x00100000L /* Device clamps fog blend factor per vertex */

//
// LineCaps
//
#define D3DLINECAPS_TEXTURE             0x00000001L
#define D3DLINECAPS_ZTEST               0x00000002L
#define D3DLINECAPS_BLEND               0x00000004L
#define D3DLINECAPS_ALPHACMP            0x00000008L
#define D3DLINECAPS_FOG                 0x00000010L
#define D3DLINECAPS_ANTIALIAS           0x00000020L

//
// RasterCaps
//
#define D3DPRASTERCAPS_DITHER                 0x00000001L
#define D3DPRASTERCAPS_ZTEST                  0x00000010L
#define D3DPRASTERCAPS_FOGVERTEX              0x00000080L
#define D3DPRASTERCAPS_FOGTABLE               0x00000100L
#define D3DPRASTERCAPS_MIPMAPLODBIAS          0x00002000L
#define D3DPRASTERCAPS_ZBUFFERLESSHSR         0x00008000L
#define D3DPRASTERCAPS_FOGRANGE               0x00010000L
#define D3DPRASTERCAPS_ANISOTROPY             0x00020000L
#define D3DPRASTERCAPS_WBUFFER                0x00040000L
#define D3DPRASTERCAPS_WFOG                   0x00100000L
#define D3DPRASTERCAPS_ZFOG                   0x00200000L
#define D3DPRASTERCAPS_COLORPERSPECTIVE       0x00400000L /* Device iterates colors perspective correct */
#define D3DPRASTERCAPS_SCISSORTEST            0x01000000L
#define D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS    0x02000000L
#define D3DPRASTERCAPS_DEPTHBIAS              0x04000000L 
#define D3DPRASTERCAPS_MULTISAMPLE_TOGGLE     0x08000000L

//
// ZCmpCaps, AlphaCmpCaps
//
#define D3DPCMPCAPS_NEVER               0x00000001L
#define D3DPCMPCAPS_LESS                0x00000002L
#define D3DPCMPCAPS_EQUAL               0x00000004L
#define D3DPCMPCAPS_LESSEQUAL           0x00000008L
#define D3DPCMPCAPS_GREATER             0x00000010L
#define D3DPCMPCAPS_NOTEQUAL            0x00000020L
#define D3DPCMPCAPS_GREATEREQUAL        0x00000040L
#define D3DPCMPCAPS_ALWAYS              0x00000080L

//
// SourceBlendCaps, DestBlendCaps
//
#define D3DPBLENDCAPS_ZERO              0x00000001L
#define D3DPBLENDCAPS_ONE               0x00000002L
#define D3DPBLENDCAPS_SRCCOLOR          0x00000004L
#define D3DPBLENDCAPS_INVSRCCOLOR       0x00000008L
#define D3DPBLENDCAPS_SRCALPHA          0x00000010L
#define D3DPBLENDCAPS_INVSRCALPHA       0x00000020L
#define D3DPBLENDCAPS_DESTALPHA         0x00000040L
#define D3DPBLENDCAPS_INVDESTALPHA      0x00000080L
#define D3DPBLENDCAPS_DESTCOLOR         0x00000100L
#define D3DPBLENDCAPS_INVDESTCOLOR      0x00000200L
#define D3DPBLENDCAPS_SRCALPHASAT       0x00000400L
#define D3DPBLENDCAPS_BOTHSRCALPHA      0x00000800L
#define D3DPBLENDCAPS_BOTHINVSRCALPHA   0x00001000L
#define D3DPBLENDCAPS_BLENDFACTOR       0x00002000L /* Supports both D3DBLEND_BLENDFACTOR and D3DBLEND_INVBLENDFACTOR */

//
// ShadeCaps
//
#define D3DPSHADECAPS_COLORGOURAUDRGB       0x00000008L
#define D3DPSHADECAPS_SPECULARGOURAUDRGB    0x00000200L
#define D3DPSHADECAPS_ALPHAGOURAUDBLEND     0x00004000L
#define D3DPSHADECAPS_FOGGOURAUD            0x00080000L

//
// TextureCaps
//
#define D3DPTEXTURECAPS_PERSPECTIVE         0x00000001L /* Perspective-correct texturing is supported */
#define D3DPTEXTURECAPS_POW2                0x00000002L /* Power-of-2 texture dimensions are required - applies to non-Cube/Volume textures only. */
#define D3DPTEXTURECAPS_ALPHA               0x00000004L /* Alpha in texture pixels is supported */
#define D3DPTEXTURECAPS_SQUAREONLY          0x00000020L /* Only square textures are supported */
#define D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE 0x00000040L /* Texture indices are not scaled by the texture size prior to interpolation */
#define D3DPTEXTURECAPS_ALPHAPALETTE        0x00000080L /* Device can draw alpha from texture palettes */
// Device can use non-POW2 textures if:
//  1) D3DTEXTURE_ADDRESS is set to CLAMP for this texture's stage
//  2) D3DRS_WRAP(N) is zero for this texture's coordinates
//  3) mip mapping is not enabled (use magnification filter only)
#define D3DPTEXTURECAPS_NONPOW2CONDITIONAL  0x00000100L
#define D3DPTEXTURECAPS_PROJECTED           0x00000400L /* Device can do D3DTTFF_PROJECTED */
#define D3DPTEXTURECAPS_CUBEMAP             0x00000800L /* Device can do cubemap textures */
#define D3DPTEXTURECAPS_VOLUMEMAP           0x00002000L /* Device can do volume textures */
#define D3DPTEXTURECAPS_MIPMAP              0x00004000L /* Device can do mipmapped textures */
#define D3DPTEXTURECAPS_MIPVOLUMEMAP        0x00008000L /* Device can do mipmapped volume textures */
#define D3DPTEXTURECAPS_MIPCUBEMAP          0x00010000L /* Device can do mipmapped cube maps */
#define D3DPTEXTURECAPS_CUBEMAP_POW2        0x00020000L /* Device requires that cubemaps be power-of-2 dimension */
#define D3DPTEXTURECAPS_VOLUMEMAP_POW2      0x00040000L /* Device requires that volume maps be power-of-2 dimension */
#define D3DPTEXTURECAPS_NOPROJECTEDBUMPENV  0x00200000L /* Device does not support projected bump env lookup operation 
                                                           in programmable and fixed function pixel shaders */

//
// TextureFilterCaps, StretchRectFilterCaps
//
#define D3DPTFILTERCAPS_MINFPOINT           0x00000100L /* Min Filter */
#define D3DPTFILTERCAPS_MINFLINEAR          0x00000200L
#define D3DPTFILTERCAPS_MINFANISOTROPIC     0x00000400L
#define D3DPTFILTERCAPS_MINFPYRAMIDALQUAD   0x00000800L
#define D3DPTFILTERCAPS_MINFGAUSSIANQUAD    0x00001000L
#define D3DPTFILTERCAPS_MIPFPOINT           0x00010000L /* Mip Filter */
#define D3DPTFILTERCAPS_MIPFLINEAR          0x00020000L
#define D3DPTFILTERCAPS_MAGFPOINT           0x01000000L /* Mag Filter */
#define D3DPTFILTERCAPS_MAGFLINEAR          0x02000000L
#define D3DPTFILTERCAPS_MAGFANISOTROPIC     0x04000000L
#define D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD   0x08000000L
#define D3DPTFILTERCAPS_MAGFGAUSSIANQUAD    0x10000000L

//
// TextureAddressCaps
//
#define D3DPTADDRESSCAPS_WRAP           0x00000001L
#define D3DPTADDRESSCAPS_MIRROR         0x00000002L
#define D3DPTADDRESSCAPS_CLAMP          0x00000004L
#define D3DPTADDRESSCAPS_BORDER         0x00000008L
#define D3DPTADDRESSCAPS_INDEPENDENTUV  0x00000010L
#define D3DPTADDRESSCAPS_MIRRORONCE     0x00000020L

//
// StencilCaps
//
#define D3DSTENCILCAPS_KEEP             0x00000001L
#define D3DSTENCILCAPS_ZERO             0x00000002L
#define D3DSTENCILCAPS_REPLACE          0x00000004L
#define D3DSTENCILCAPS_INCRSAT          0x00000008L
#define D3DSTENCILCAPS_DECRSAT          0x00000010L
#define D3DSTENCILCAPS_INVERT           0x00000020L
#define D3DSTENCILCAPS_INCR             0x00000040L
#define D3DSTENCILCAPS_DECR             0x00000080L
#define D3DSTENCILCAPS_TWOSIDED         0x00000100L

//
// TextureOpCaps
//
#define D3DTEXOPCAPS_DISABLE                    0x00000001L
#define D3DTEXOPCAPS_SELECTARG1                 0x00000002L
#define D3DTEXOPCAPS_SELECTARG2                 0x00000004L
#define D3DTEXOPCAPS_MODULATE                   0x00000008L
#define D3DTEXOPCAPS_MODULATE2X                 0x00000010L
#define D3DTEXOPCAPS_MODULATE4X                 0x00000020L
#define D3DTEXOPCAPS_ADD                        0x00000040L
#define D3DTEXOPCAPS_ADDSIGNED                  0x00000080L
#define D3DTEXOPCAPS_ADDSIGNED2X                0x00000100L
#define D3DTEXOPCAPS_SUBTRACT                   0x00000200L
#define D3DTEXOPCAPS_ADDSMOOTH                  0x00000400L
#define D3DTEXOPCAPS_BLENDDIFFUSEALPHA          0x00000800L
#define D3DTEXOPCAPS_BLENDTEXTUREALPHA          0x00001000L
#define D3DTEXOPCAPS_BLENDFACTORALPHA           0x00002000L
#define D3DTEXOPCAPS_BLENDTEXTUREALPHAPM        0x00004000L
#define D3DTEXOPCAPS_BLENDCURRENTALPHA          0x00008000L
#define D3DTEXOPCAPS_PREMODULATE                0x00010000L
#define D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR     0x00020000L
#define D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA     0x00040000L
#define D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR  0x00080000L
#define D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA  0x00100000L
#define D3DTEXOPCAPS_BUMPENVMAP                 0x00200000L
#define D3DTEXOPCAPS_BUMPENVMAPLUMINANCE        0x00400000L
#define D3DTEXOPCAPS_DOTPRODUCT3                0x00800000L
#define D3DTEXOPCAPS_MULTIPLYADD                0x01000000L
#define D3DTEXOPCAPS_LERP                       0x02000000L

//
// FVFCaps
//
#define D3DFVFCAPS_TEXCOORDCOUNTMASK    0x0000ffffL /* mask for texture coordinate count field */
#define D3DFVFCAPS_DONOTSTRIPELEMENTS   0x00080000L /* Device prefers that vertex elements not be stripped */
#define D3DFVFCAPS_PSIZE                0x00100000L /* Device can receive point size */

//
// VertexProcessingCaps
//
#define D3DVTXPCAPS_TEXGEN              0x00000001L /* device can do texgen */
#define D3DVTXPCAPS_MATERIALSOURCE7     0x00000002L /* device can do DX7-level colormaterialsource ops */
#define D3DVTXPCAPS_DIRECTIONALLIGHTS   0x00000008L /* device can do directional lights */
#define D3DVTXPCAPS_POSITIONALLIGHTS    0x00000010L /* device can do positional lights (includes point and spot) */
#define D3DVTXPCAPS_LOCALVIEWER         0x00000020L /* device can do local viewer */
#define D3DVTXPCAPS_TWEENING            0x00000040L /* device can do vertex tweening */
#define D3DVTXPCAPS_TEXGEN_SPHEREMAP    0x00000100L /* device supports D3DTSS_TCI_SPHEREMAP */
#define D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER   0x00000200L /* device does not support TexGen in non-local
                                                            viewer mode */

//
// DevCaps2
//
#define D3DDEVCAPS2_STREAMOFFSET                        0x00000001L /* Device supports offsets in streams. Must be set by DX9 drivers */
#define D3DDEVCAPS2_DMAPNPATCH                          0x00000002L /* Device supports displacement maps for N-Patches*/
#define D3DDEVCAPS2_ADAPTIVETESSRTPATCH                 0x00000004L /* Device supports adaptive tesselation of RT-patches*/
#define D3DDEVCAPS2_ADAPTIVETESSNPATCH                  0x00000008L /* Device supports adaptive tesselation of N-patches*/
#define D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES       0x00000010L /* Device supports StretchRect calls with a texture as the source*/
#define D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH                0x00000020L /* Device supports presampled displacement maps for N-Patches */
#define D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET  0x00000040L /* Vertex elements in a vertex declaration can share the same stream offset */

//
// DeclTypes
//
#define D3DDTCAPS_UBYTE4     0x00000001L
#define D3DDTCAPS_UBYTE4N    0x00000002L
#define D3DDTCAPS_SHORT2N    0x00000004L
#define D3DDTCAPS_SHORT4N    0x00000008L
#define D3DDTCAPS_USHORT2N   0x00000010L
#define D3DDTCAPS_USHORT4N   0x00000020L
#define D3DDTCAPS_UDEC3      0x00000040L
#define D3DDTCAPS_DEC3N      0x00000080L
#define D3DDTCAPS_FLOAT16_2  0x00000100L
#define D3DDTCAPS_FLOAT16_4  0x00000200L


#pragma pack()

#endif /* (DIRECT3D_VERSION >= 0x0900) */
#endif /* _d3d9CAPS_H_ */



