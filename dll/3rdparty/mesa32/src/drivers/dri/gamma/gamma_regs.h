/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_regs.h,v 1.5 2002/02/22 21:33:02 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 */

#ifndef _GAMMA_REGS_H_
#define _GAMMA_REGS_H_

#include "gamma_client.h"

/**************** MX FLAGS ****************/
/* FBReadMode */
#define FBReadSrcDisable              0x00000000
#define FBReadSrcEnable               0x00000200
#define FBReadDstDisable              0x00000000
#define FBReadDstEnable               0x00000400
#define FBDataTypeDefault             0x00000000
#define FBDataTypeColor               0x00008000
#define FBWindowOriginTop             0x00000000
#define FBWindowOriginBot             0x00010000
#define FBScanLineInt1                0x00000000
#define FBScanLineInt2                0x00800000
#define FBScanLineInt4                0x01000000
#define FBScanLineInt8                0x01800000
#define FBSrcAddrConst                0x00000000
#define FBSrcAddrIndex                0x10000000
#define FBSrcAddrCoord                0x20000000

/* LBReadMode */
#define LBPartialProdMask             0x000001ff
#define LBReadSrcDisable              0x00000000
#define LBReadSrcEnable               0x00000200
#define LBReadDstDisable              0x00000000
#define LBReadDstEnable               0x00000400
#define LBDataTypeDefault             0x00000000
#define LBDataTypeStencil             0x00010000
#define LBDataTypeDepth               0x00020000
#define LBWindowOriginTop             0x00000000
#define LBWindowOriginBot             0x00040000
#define LBScanLineInt1                0x00000000
#define LBScanLineInt2                0x00100000
#define LBScanLineInt4                0x00200000
#define LBScanLineInt8                0x00300000

/* ColorDDAMode */
#define ColorDDADisable               0x00000000
#define ColorDDAEnable                0x00000001
#define ColorDDAFlat                  0x00000000
#define ColorDDAGouraud               0x00000002
#define ColorDDAShadingMask           0x00000002

/* AlphaTestMode */
#define AlphaTestModeDisable          0x00000000
#define AlphaTestModeEnable           0x00000001
#define AT_Never                      0x00000000
#define AT_Less                       0x00000002
#define AT_Equal                      0x00000004
#define AT_LessEqual                  0x00000006
#define AT_Greater                    0x00000008
#define AT_NotEqual                   0x0000000a
#define AT_GreaterEqual               0x0000000c
#define AT_Always                     0x0000000e
#define AT_CompareMask                0x0000000e
#define AT_RefValueMask               0x00000ff0

/* AlphaBlendMode */
#define AlphaBlendModeDisable         0x00000000
#define AlphaBlendModeEnable          0x00000001
#define AB_Src_Zero                   0x00000000
#define AB_Src_One                    0x00000002
#define AB_Src_DstColor               0x00000004
#define AB_Src_OneMinusDstColor       0x00000006
#define AB_Src_SrcAlpha               0x00000008
#define AB_Src_OneMinusSrcAlpha       0x0000000a
#define AB_Src_DstAlpha               0x0000000c
#define AB_Src_OneMinusDstAlpha       0x0000000e
#define AB_Src_SrcAlphaSaturate       0x00000010
#define AB_SrcBlendMask               0x0000001e
#define AB_Dst_Zero                   0x00000000
#define AB_Dst_One                    0x00000020
#define AB_Dst_SrcColor               0x00000040
#define AB_Dst_OneMinusSrcColor       0x00000060
#define AB_Dst_SrcAlpha               0x00000080
#define AB_Dst_OneMinusSrcAlpha       0x000000a0
#define AB_Dst_DstAlpha               0x000000c0
#define AB_Dst_OneMinusDstAlpha       0x000000e0
#define AB_DstBlendMask               0x000000e0
#define AB_ColorFmt_8888              0x00000000
#define AB_ColorFmt_5555              0x00000100
#define AB_ColorFmt_4444              0x00000200
#define AB_ColorFmt_4444Front         0x00000300
#define AB_ColorFmt_4444Back          0x00000400
#define AB_ColorFmt_332Front          0x00000500
#define AB_ColorFmt_332Back           0x00000600
#define AB_ColorFmt_121Front          0x00000700
#define AB_ColorFmt_121Back           0x00000800
#define AB_ColorFmt_555Back           0x00000d00
#define AB_ColorFmt_CI8               0x00000e00
#define AB_ColorFmt_CI4               0x00000f00
#define AB_AlphaBufferPresent         0x00000000
#define AB_NoAlphaBufferPresent       0x00001000
#define AB_ColorOrder_BGR             0x00000000
#define AB_ColorOrder_RGB             0x00002000
#define AB_OpenGLType                 0x00000000
#define AB_QuickDraw3DType            0x00004000
#define AB_AlphaDst_FBData            0x00000000
#define AB_AlphaDst_FBSourceData      0x00008000
#define AB_ColorConversionScale       0x00000000
#define AB_ColorConversionShift       0x00010000
#define AB_AlphaConversionScale       0x00000000
#define AB_AlphaConversionShift       0x00020000

/* AntialiasMode */
#define AntialiasModeDisable          0x00000000
#define AntialiasModeEnable           0x00000001

/* AreaStippleMode */
#define AreaStippleModeDisable        0x00000000
#define AreaStippleModeEnable         0x00000001
#define ASM_X32                       0x00000008
#define ASM_Y32                       0x00000040

/* DepthMode */
#define DepthModeDisable              0x00000000
#define DepthModeEnable               0x00000001
#define DM_WriteMask                  0x00000002
#define DM_SourceFragment             0x00000000
#define DM_SourceLBData               0x00000004
#define DM_SourceDepthRegister        0x00000008
#define DM_SourceLBSourceData         0x0000000c
#define DM_SourceMask                 0x0000000c
#define DM_Never                      0x00000000
#define DM_Less                       0x00000010
#define DM_Equal                      0x00000020
#define DM_LessEqual                  0x00000030
#define DM_Greater                    0x00000040
#define DM_NotEqual                   0x00000050
#define DM_GreaterEqual               0x00000060
#define DM_Always                     0x00000070
#define DM_CompareMask                0x00000070

/* FBWriteMode */
#define FBWriteModeDisable            0x00000000
#define FBWriteModeEnable             0x00000001
#define FBW_UploadColorData           0x00000008

/* FogMode */
#define FogModeDisable                0x00000000
#define FogModeEnable                 0x00000001

/* LBWriteMode */
#define LBWriteModeDisable            0x00000000
#define LBWriteModeEnable             0x00000001
#define LBW_UploadNone                0x00000000
#define LBW_UploadDepth               0x00000002
#define LBW_UploadStencil             0x00000004

/* LBRead/Write Format */
#define LBRF_DepthWidth15   0x03  /* only permedia */
#define LBRF_DepthWidth16   0x00
#define LBRF_DepthWidth24   0x01
#define LBRF_DepthWidth32   0x02
#define LBRF_StencilWidth0  (0 << 2)
#define LBRF_StencilWidth4  (1 << 2)
#define LBRF_StencilWidth8  (2 << 2)
#define LBRF_StencilPos16   (0 << 4)
#define LBRF_StencilPos20   (1 << 4)
#define LBRF_StencilPos24   (2 << 4)
#define LBRF_StencilPos28   (3 << 4)
#define LBRF_StencilPos32   (4 << 4)
#define LBRF_FrameCount0    (0 << 7)
#define LBRF_FrameCount4    (1 << 7)
#define LBRF_FrameCount8    (2 << 7)
#define LBRF_FrameCountPos16  (0 << 9)
#define LBRF_FrameCountPos20  (1 << 9)
#define LBRF_FrameCountPos24  (2 << 9)
#define LBRF_FrameCountPos28  (3 << 9)
#define LBRF_FrameCountPos32  (4 << 9)
#define LBRF_FrameCountPos36  (5 << 9)
#define LBRF_FrameCountPos40  (6 << 9)
#define LBRF_GIDWidth0 (0 << 12)
#define LBRF_GIDWidth4 (1 << 12)
#define LBRF_GIDPos16  (0 << 13)
#define LBRF_GIDPos20  (1 << 13)
#define LBRF_GIDPos24  (2 << 13)
#define LBRF_GIDPos28  (3 << 13)
#define LBRF_GIDPos32  (4 << 13)
#define LBRF_GIDPos36  (5 << 13)
#define LBRF_GIDPos40  (6 << 13)
#define LBRF_GIDPos44  (7 << 13)
#define LBRF_GIDPos48  (8 << 13)
#define LBRF_Compact32  (1 << 17)

/* StencilMode */
#define StencilDisable                0x00000000
#define StencilEnable                 0x00000001

/* RouterMode */
#define R_Order_TextureDepth          0x00000000
#define R_Order_DepthTexture          0x00000001

/* ScissorMode */
#define UserScissorDisable            0x00000000
#define UserScissorEnable             0x00000001
#define ScreenScissorDisable          0x00000000
#define ScreenScissorEnable           0x00000002

/* DitherMode */
#define DitherModeDisable             0x00000000
#define DitherModeEnable              0x00000001
#define DM_DitherDisable              0x00000000
#define DM_DitherEnable               0x00000002
#define DM_ColorFmt_8888              0x00000000
#define DM_ColorFmt_5555              0x00000004
#define DM_ColorFmt_4444              0x00000008
#define DM_ColorFmt_4444Front         0x0000000c
#define DM_ColorFmt_4444Back          0x00000010
#define DM_ColorFmt_332Front          0x00000014
#define DM_ColorFmt_332Back           0x00000018
#define DM_ColorFmt_121Front          0x0000001c
#define DM_ColorFmt_121Back           0x00000020
#define DM_ColorFmt_555Back           0x00000024
#define DM_ColorFmt_CI8               0x00000028
#define DM_ColorFmt_CI4               0x0000002c
#define DM_XOffsetMask                0x000000c0
#define DM_YOffsetMask                0x00000300
#define DM_ColorOrder_BGR             0x00000000
#define DM_ColorOrder_RGB             0x00000400
#define DM_AlphaDitherDefault         0x00000000
#define DM_AlphaDitherNone            0x00004000
#define DM_Truncate                   0x00000000
#define DM_Round                      0x00008000

/* RasterizerMode */
#define RM_MirrorBitMask              0x00000001
#define RM_InvertBitMask              0x00000002
#define RM_FractionAdjNo              0x00000000
#define RM_FractionAdjZero            0x00000004
#define RM_FractionAdjHalf            0x00000008
#define RM_FractionAdjNearHalf        0x0000000c
#define RM_BiasCoordZero              0x00000000
#define RM_BiasCoordHalf              0x00000010
#define RM_BiasCoordNearHalf          0x00000020
#define RM_BitMaskByteSwap_ABCD       0x00000000
#define RM_BitMaskByteSwap_BADC       0x00000080
#define RM_BitMaskByteSwap_CDAB       0x00000100
#define RM_BitMaskByteSwap_DCBA       0x00000180
#define RM_BitMaskPacked              0x00000000
#define RM_BitMaskEveryScanline       0x00000200
#define RM_BitMaskOffsetMask          0x00007c00
#define RM_HostDataByteSwap_ABCD      0x00000000
#define RM_HostDataByteSwap_BADC      0x00008000
#define RM_HostDataByteSwap_CDAB      0x00010000
#define RM_HostDataByteSwap_DCBA      0x00018000
#define RM_SingleGLINT                0x00000000
#define RM_MultiGLINT                 0x00020000
#define RM_YLimitsEnable              0x00040000

/* Window */
#define WindowDisable                 0x00000000
#define WindowEnable                  0x00000001
#define W_AlwaysPass                  0x00000000
#define W_NeverPass                   0x00000002
#define W_PassIfEqual                 0x00000004
#define W_PassIfNotEqual              0x00000006
#define W_CompareMask                 0x00000006
#define W_ForceLBUpdate               0x00000008
#define W_LBUpdateFromSource          0x00000000
#define W_LBUpdateFromRegisters       0x00000010
#define W_GIDMask                     0x000001e0
#define W_FrameCountMask              0x0001fe00
#define W_StencilFCP                  0x00020000
#define W_DepthFCP                    0x00040000
#define W_OverrideWriteFiltering      0x00080000

/* TextureAddressMode */
#define TextureAddressModeDisable     0x00000000
#define TextureAddressModeEnable      0x00000001
#define TAM_SWrap_Clamp               0x00000000
#define TAM_SWrap_Repeat              0x00000002
#define TAM_SWrap_Mirror              0x00000004
#define TAM_SWrap_Mask                0x00000006
#define TAM_TWrap_Clamp               0x00000000
#define TAM_TWrap_Repeat              0x00000008
#define TAM_TWrap_Mirror              0x00000010
#define TAM_TWrap_Mask                0x00000018
#define TAM_Operation_2D              0x00000000
#define TAM_Operation_3D              0x00000020
#define TAM_InhibitDDAInit            0x00000040
#define TAM_LODDisable                0x00000000
#define TAM_LODEnable                 0x00000080
#define TAM_DY_Disable                0x00000000
#define TAM_DY_Enable                 0x00000100
#define TAM_WidthMask                 0x00001e00
#define TAM_HeightMask                0x0001e000
#define TAM_TexMapType_1D             0x00000000
#define TAM_TexMapType_2D             0x00020000
#define TAM_TexMapType_Mask           0x00020000

/* TextureReadMode */
#define TextureReadModeDisable        0x00000000
#define TextureReadModeEnable         0x00000001
#define TRM_WidthMask                 0x0000001e
#define TRM_HeightMask                0x000001e0
#define TRM_Depth1                    0x00000000
#define TRM_Depth2                    0x00000200
#define TRM_Depth4                    0x00000400
#define TRM_Depth8                    0x00000600
#define TRM_Depth16                   0x00000800
#define TRM_Depth32                   0x00000a00
#define TRM_DepthMask                 0x00000e00
#define TRM_Border                    0x00001000
#define TRM_Patch                     0x00002000
#define TRM_Mag_Nearest               0x00000000
#define TRM_Mag_Linear                0x00004000
#define TRM_Mag_Mask                  0x00004000
#define TRM_Min_Nearest               0x00000000
#define TRM_Min_Linear                0x00008000
#define TRM_Min_NearestMMNearest      0x00010000
#define TRM_Min_NearestMMLinear       0x00018000
#define TRM_Min_LinearMMNearest       0x00020000
#define TRM_Min_LinearMMLinear        0x00028000
#define TRM_Min_Mask                  0x00038000
#define TRM_UWrap_Clamp               0x00000000
#define TRM_UWrap_Repeat              0x00040000
#define TRM_UWrap_Mirror              0x00080000
#define TRM_UWrap_Mask                0x000c0000
#define TRM_VWrap_Clamp               0x00000000
#define TRM_VWrap_Repeat              0x00100000
#define TRM_VWrap_Mirror              0x00200000
#define TRM_VWrap_Mask                0x00300000
#define TRM_TexMapType_1D             0x00000000
#define TRM_TexMapType_2D             0x00400000
#define TRM_TexMapType_Mask           0x00400000
#define TRM_MipMapDisable             0x00000000
#define TRM_MipMapEnable              0x00800000
#define TRM_PrimaryCacheDisable       0x00000000
#define TRM_PrimaryCacheEnable        0x01000000
#define TRM_FBSourceAddr_None         0x00000000
#define TRM_FBSourceAddr_Index        0x02000000
#define TRM_FBSourceAddr_Coord        0x04000000
#define TRM_BorderClamp               0x08000000

/* TextureColorMode */
#define TextureColorModeDisable       0x00000000
#define TextureColorModeEnable        0x00000001
#define TCM_Modulate                  0x00000000
#define TCM_Decal                     0x00000002
#define TCM_Blend                     0x00000004
#define TCM_Replace                   0x00000006
#define TCM_ApplicationMask           0x0000000e
#define TCM_OpenGLType                0x00000000
#define TCM_QuickDraw3DType           0x00000010
#define TCM_KdDDA_Disable             0x00000000
#define TCM_KdDDA_Enable              0x00000020
#define TCM_KsDDA_Disable             0x00000000
#define TCM_KsDDA_Enable              0x00000040
#define TCM_BaseFormat_Alpha          0x00000000
#define TCM_BaseFormat_Lum            0x00000080
#define TCM_BaseFormat_LumAlpha       0x00000100
#define TCM_BaseFormat_Intensity      0x00000180
#define TCM_BaseFormat_RGB            0x00000200
#define TCM_BaseFormat_RGBA           0x00000280
#define TCM_BaseFormatMask            0x00000380
#define TCM_LoadMode_None             0x00000000
#define TCM_LoadMode_Ks               0x00000400
#define TCM_LoadMode_Kd               0x00000800

/* TextureCacheControl */
#define TCC_Invalidate                0x00000001
#define TCC_Disable                   0x00000000
#define TCC_Enable                    0x00000002

/* TextureFilterMode */
#define TextureFilterModeDisable      0x00000000
#define TextureFilterModeEnable       0x00000001
#define TFM_AlphaMapEnable            0x00000002
#define TFM_AlphaMapSense             0x00000004

/* TextureFormat */
#define TF_LittleEndian               0x00000000
#define TF_BigEndian                  0x00000001
#define TF_16Bit_565                  0x00000000
#define TF_16Bit_555                  0x00000002
#define TF_ColorOrder_BGR             0x00000000
#define TF_ColorOrder_RGB             0x00000004
#define TF_Compnents_1                0x00000000
#define TF_Compnents_2                0x00000008
#define TF_Compnents_3                0x00000010
#define TF_Compnents_4                0x00000018
#define TF_CompnentsMask              0x00000018
#define TF_OutputFmt_Texel            0x00000000
#define TF_OutputFmt_Color            0x00000020
#define TF_OutputFmt_BitMask          0x00000040
#define TF_OutputFmtMask              0x00000060
#define TF_MirrorEnable               0x00000080
#define TF_InvertEnable               0x00000100
#define TF_ByteSwapEnable             0x00000200
#define TF_LUTOffsetMask              0x0003fc00
#define TF_OneCompFmt_Lum             0x00000000
#define TF_OneCompFmt_Alpha           0x00040000
#define TF_OneCompFmt_Intensity        0x00080000
#define TF_OneCompFmt_Mask            0x000c0000
/**************** MX FLAGS ****************/

/************** GAMMA FLAGS ***************/
/* GeometryMode */
#define GM_TextureDisable             0x00000000
#define GM_TextureEnable              0x00000001
#define GM_FogDisable                 0x00000000
#define GM_FogEnable                  0x00000002
#define GM_FogLinear                  0x00000000
#define GM_FogExp                     0x00000004
#define GM_FogExpSquared              0x00000008
#define GM_FogMask                    0x0000000C
#define GM_FrontPolyPoint             0x00000000
#define GM_FrontPolyLine              0x00000010
#define GM_FrontPolyFill              0x00000020
#define GM_BackPolyPoint              0x00000000
#define GM_BackPolyLine               0x00000040
#define GM_BackPolyFill               0x00000080
#define GM_FB_PolyMask                0x000000F0
#define GM_FrontFaceCW                0x00000000
#define GM_FrontFaceCCW               0x00000100
#define GM_FFMask                     0x00000100
#define GM_PolyCullDisable            0x00000000
#define GM_PolyCullEnable             0x00000200
#define GM_PolyCullFront              0x00000000
#define GM_PolyCullBack               0x00000400
#define GM_PolyCullBoth               0x00000800
#define GM_PolyCullMask               0x00000c00
#define GM_ClipShortLinesDisable      0x00000000
#define GM_ClipShortLinesEnable       0x00001000
#define GM_ClipSmallTrisDisable       0x00000000
#define GM_ClipSmallTrisEnable        0x00002000
#define GM_RenderMode                 0x00000000
#define GM_SelectMode                 0x00004000
#define GM_FeedbackMode               0x00008000
#define GM_Feedback2D                 0x00000000
#define GM_Feedback3D                 0x00010000
#define GM_Feedback3DColor            0x00020000
#define GM_Feedback3DColorTexture     0x00030000
#define GM_Feedback4DColorTexture     0x00040000
#define GM_CullFaceNormDisable        0x00000000
#define GM_CullFaceNormEnable         0x00080000
#define GM_AutoFaceNormDisable        0x00000000
#define GM_AutoFaceNormEnable         0x00100000
#define GM_GouraudShading             0x00000000
#define GM_FlatShading                0x00200000
#define GM_ShadingMask                0x00200000
#define GM_UserClipNone               0x00000000
#define GM_UserClip0                  0x00400000
#define GM_UserClip1                  0x00800000
#define GM_UserClip2                  0x01000000
#define GM_UserClip3                  0x02000000
#define GM_UserClip4                  0x04000000
#define GM_UserClip5                  0x08000000
#define GM_PolyOffsetPointDisable     0x00000000
#define GM_PolyOffsetPointEnable      0x10000000
#define GM_PolyOffsetLineDisable      0x00000000
#define GM_PolyOffsetLineEnable       0x20000000
#define GM_PolyOffsetFillDisable      0x00000000
#define GM_PolyOffsetFillEnable       0x40000000
#define GM_InvertFaceNormCullDisable  0x00000000
#define GM_InvertFaceNormCullEnable   0x80000000

/* Begin */
#define B_AreaStippleDisable          0x00000000
#define B_AreaStippleEnable           0x00000001
#define B_LineStippleDisable          0x00000000
#define B_LineStippleEnable           0x00000002
#define B_AntiAliasDisable            0x00000000
#define B_AntiAliasEnable             0x00000100
#define B_TextureDisable              0x00000000
#define B_TextureEnable               0x00002000
#define B_FogDisable                  0x00000000
#define B_FogEnable                   0x00004000
#define B_SubPixelCorrectDisable      0x00000000
#define B_SubPixelCorrectEnable       0x00010000
#define B_PrimType_Null               0x00000000
#define B_PrimType_Points             0x10000000
#define B_PrimType_Lines              0x20000000
#define B_PrimType_LineLoop           0x30000000
#define B_PrimType_LineStrip          0x40000000
#define B_PrimType_Triangles          0x50000000
#define B_PrimType_TriangleStrip      0x60000000
#define B_PrimType_TriangleFan        0x70000000
#define B_PrimType_Quads              0x80000000
#define B_PrimType_QuadStrip          0x90000000
#define B_PrimType_Polygon            0xa0000000
#define B_PrimType_Mask               0xf0000000

/* EdgeFlag */
#define EdgeFlagDisable               0x00000000
#define EdgeFlagEnable                0x00000001

/* NormalizeMode */
#define NormalizeModeDisable          0x00000000
#define NormalizeModeEnable           0x00000001
#define FaceNormalDisable             0x00000000
#define FaceNormalEnable              0x00000002
#define InvertAutoFaceNormal          0x00000004

/* LightingMode */
#define LightingModeDisable           0x00000000
#define LightingModeEnable            0x00000001
#define LightingModeTwoSides          0x00000004
#define LightingModeLocalViewer       0x00000008
#define LightingModeSpecularEnable    0x00008000

/* Light0Mode */
#define Light0ModeDisable             0x00000000
#define Light0ModeEnable              0x00000001
#define Light0ModeSpotLight           0x00000002
#define Light0ModeAttenuation         0x00000004
#define Light0ModeLocal               0x00000008

/* Light0Mode */
#define Light1ModeDisable             0x00000000
#define Light1ModeEnable              0x00000001
#define Light1ModeSpotLight           0x00000002
#define Light1ModeAttenuation         0x00000004
#define Light1ModeLocal               0x00000008

/* ColorMaterialMode */
#define ColorMaterialModeDisable      0x00000000
#define ColorMaterialModeEnable       0x00000001
#define ColorMaterialModeFront        0x00000000
#define ColorMaterialModeBack         0x00000002
#define ColorMaterialModeFrontAndBack 0x00000004
#define ColorMaterialModeEmission     0x00000000
#define ColorMaterialModeAmbient      0x00000008
#define ColorMaterialModeDiffuse      0x00000010
#define ColorMaterialModeSpecular     0x00000018
#define ColorMaterialModeAmbAndDiff   0x00000020
#define ColorMaterialModeMask         0x0000003e

/* MaterialMode */
#define MaterialModeDisable           0x00000000
#define MaterialModeEnable            0x00000001
#define MaterialModeTwoSides          0x00000080

/* DeltaMode */
#define DM_Target300SX                0x00000000
#define DM_Target500TXMX              0x00000001
#define DM_Depth16                    0x00000004
#define DM_Depth24                    0x00000008
#define DM_Depth32                    0x0000000c
#define DM_FogEnable                  0x00000010
#define DM_TextureEnable              0x00000020
#define DM_SmoothShadingEnable        0x00000040
#define DM_DepthEnable                0x00000080
#define DM_SpecularEnable             0x00000100
#define DM_DiffuseEnable              0x00000200
#define DM_SubPixlCorrectionEnable    0x00000400
#define DM_DiamondExit                0x00000800
#define DM_NoDraw                     0x00001000
#define DM_ClampEnable                0x00002000
#define DM_TextureParameterAsGiven    0x00000000
#define DM_TextureParameterClamped    0x00004000
#define DM_TextureParameterNormalized 0x00008000
#define DM_BiasCoords                 0x00080000
#define DM_ColorDiffuse               0x00100000
#define DM_ColorSpecular              0x00200000
#define DM_FlatShadingMethod          0x00400000

/* PointMode */
#define PM_AntialiasDisable           0x00000000
#define PM_AntialiasEnable            0x00000001
#define PM_AntialiasQuality_4x4       0x00000000
#define PM_AntialiasQuality_8x8       0x00000002

/* LogicalOpMode */
#define LogicalOpModeDisable          0x00000000
#define LogicalOpModeEnable           0x00000001
#define LogicalOpModeMask             0x0000001e

/* LineMode */
#define LM_StippleDisable             0x00000000
#define LM_StippleEnable              0x00000001
#define LM_RepeatFactorMask           0x000003fe
#define LM_StippleMask                0x03fffc00
#define LM_MirrorDisable              0x00000000
#define LM_MirrorEnable               0x04000000
#define LM_AntialiasDisable           0x00000000
#define LM_AntialiasEnable            0x08000000
#define LM_AntialiasQuality_4x4       0x00000000
#define LM_AntialiasQuality_8x8       0x10000000

/* TriangleMode */
#define TM_AntialiasDisable           0x00000000
#define TM_AntialiasEnable            0x00000001
#define TM_AntialiasQuality_4x4       0x00000000
#define TM_AntialiasQuality_8x8       0x00000002
#define TM_UseTriPacketInterface      0x00000004

/* TransformMode */
#define XM_UseModelViewMatrix         0x00000001
#define XM_UseModelViewProjMatrix     0x00000002
#define XM_XformNormals               0x00000004
#define XM_XformFaceNormals           0x00000008
#define XM_XformTexture               0x00000010
#define XM_XMask                      0x00000013
#define XM_TexGenModeS_None           0x00000000
#define XM_TexGenModeS_ObjLinear      0x00000020
#define XM_TexGenModeS_EyeLinear      0x00000040
#define XM_TexGenModeS_SphereMap      0x00000060
#define XM_TexGenModeT_None           0x00000000
#define XM_TexGenModeT_ObjLinear      0x00000080
#define XM_TexGenModeT_EyeLinear      0x00000100
#define XM_TexGenModeT_SphereMap      0x00000180
#define XM_TexGenModeR_None           0x00000000
#define XM_TexGenModeR_ObjLinear      0x00000200
#define XM_TexGenModeR_EyeLinear      0x00000400
#define XM_TexGenModeR_SphereMap      0x00000600
#define XM_TexGenModeQ_None           0x00000000
#define XM_TexGenModeQ_ObjLinear      0x00000800
#define XM_TexGenModeQ_EyeLinear      0x00001000
#define XM_TexGenModeQQSphereMap      0x00001800
#define XM_TexGenS                    0x00002000
#define XM_TexGenT                    0x00004000
#define XM_TexGenR                    0x00008000
#define XM_TexGenQ                    0x00010000

/* LightNMode */
#define LNM_Off                       0x00000000
#define LNM_On                        0x00000001
/************** GAMMA FLAGS ***************/

#endif /* _GAMMA_REGS_H_ */
