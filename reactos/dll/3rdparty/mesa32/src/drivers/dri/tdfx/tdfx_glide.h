/*
 * This file defines macros and types necessary for accessing glide3.
 */

/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_glide.h,v 1.1 2002/02/22 21:45:03 dawes Exp $ */

#ifndef NEWGLIDE_H
#define NEWGLIDE_H

#define FX_CALL

typedef unsigned char FxU8;
typedef signed char FxI8;
typedef unsigned short FxU16;
typedef signed short FxI16;
#if defined(__alpha__) || defined (__LP64__)
typedef signed int FxI32;
typedef unsigned int FxU32;
#else
typedef signed long FxI32;
typedef unsigned long FxU32;
#endif
typedef unsigned long AnyPtr;
typedef int FxBool;
typedef float FxFloat;
typedef double FxDouble;

typedef unsigned long FxColor_t;
typedef struct
{
   float r, g, b, a;
}
FxColor4;

typedef FxU32 GrColor_t;
typedef FxU8 GrAlpha_t;
typedef FxU32 GrMipMapId_t;
typedef FxU32 GrStipplePattern_t;
typedef FxU8 GrFog_t;
typedef FxU32 GrContext_t;
typedef int (FX_CALL * GrProc) (void);

#define FXTRUE 1
#define FXFALSE 0

#define FXBIT(i) (1L << (i))

#define GR_NULL_MIPMAP_HANDLE  ((GrMipMapId_t) -1)

#define GR_MIPMAPLEVELMASK_EVEN FXBIT(0)
#define GR_MIPMAPLEVELMASK_ODD FXBIT(1)
#define GR_MIPMAPLEVELMASK_BOTH (GR_MIPMAPLEVELMASK_EVEN | GR_MIPMAPLEVELMASK_ODD )

typedef FxI32 GrChipID_t;
#define GR_TMU0 0x0
#define GR_TMU1 0x1
#define GR_TMU2 0x2

#define GR_FBI  0x0

typedef FxI32 GrCombineFunction_t;
#define GR_COMBINE_FUNCTION_ZERO        0x0
#define GR_COMBINE_FUNCTION_NONE        GR_COMBINE_FUNCTION_ZERO
#define GR_COMBINE_FUNCTION_LOCAL       0x1
#define GR_COMBINE_FUNCTION_LOCAL_ALPHA 0x2
#define GR_COMBINE_FUNCTION_SCALE_OTHER 0x3
#define GR_COMBINE_FUNCTION_BLEND_OTHER GR_COMBINE_FUNCTION_SCALE_OTHER
#define GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL 0x4
#define GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA 0x5
#define GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL 0x6
#define GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL 0x7
#define GR_COMBINE_FUNCTION_BLEND GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL
#define GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA 0x8
#define GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL 0x9
#define GR_COMBINE_FUNCTION_BLEND_LOCAL GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL
#define GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA 0x10

typedef FxI32 GrCombineFactor_t;
#define GR_COMBINE_FACTOR_ZERO          0x0
#define GR_COMBINE_FACTOR_NONE          GR_COMBINE_FACTOR_ZERO
#define GR_COMBINE_FACTOR_LOCAL         0x1
#define GR_COMBINE_FACTOR_OTHER_ALPHA   0x2
#define GR_COMBINE_FACTOR_LOCAL_ALPHA   0x3
#define GR_COMBINE_FACTOR_TEXTURE_ALPHA 0x4
#define GR_COMBINE_FACTOR_TEXTURE_RGB   0x5
#define GR_COMBINE_FACTOR_DETAIL_FACTOR GR_COMBINE_FACTOR_TEXTURE_ALPHA
#define GR_COMBINE_FACTOR_LOD_FRACTION  0x5
#define GR_COMBINE_FACTOR_ONE           0x8
#define GR_COMBINE_FACTOR_ONE_MINUS_LOCAL 0x9
#define GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA 0xa
#define GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA 0xb
#define GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA 0xc
#define GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA
#define GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION 0xd

typedef FxI32 GrCombineLocal_t;
#define GR_COMBINE_LOCAL_ITERATED 0x0
#define GR_COMBINE_LOCAL_CONSTANT 0x1
#define GR_COMBINE_LOCAL_NONE GR_COMBINE_LOCAL_CONSTANT
#define GR_COMBINE_LOCAL_DEPTH  0x2

typedef FxI32 GrCombineOther_t;
#define GR_COMBINE_OTHER_ITERATED 0x0
#define GR_COMBINE_OTHER_TEXTURE 0x1
#define GR_COMBINE_OTHER_CONSTANT 0x2
#define GR_COMBINE_OTHER_NONE GR_COMBINE_OTHER_CONSTANT

typedef FxI32 GrAlphaSource_t;
#define GR_ALPHASOURCE_CC_ALPHA 0x0
#define GR_ALPHASOURCE_ITERATED_ALPHA 0x1
#define GR_ALPHASOURCE_TEXTURE_ALPHA 0x2
#define GR_ALPHASOURCE_TEXTURE_ALPHA_TIMES_ITERATED_ALPHA 0x3

typedef FxI32 GrColorCombineFnc_t;
#define GR_COLORCOMBINE_ZERO 0x0
#define GR_COLORCOMBINE_CCRGB 0x1
#define GR_COLORCOMBINE_ITRGB 0x2
#define GR_COLORCOMBINE_ITRGB_DELTA0 0x3
#define GR_COLORCOMBINE_DECAL_TEXTURE 0x4
#define GR_COLORCOMBINE_TEXTURE_TIMES_CCRGB 0x5
#define GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB 0x6
#define GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_DELTA0 0x7
#define GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_ADD_ALPHA 0x8
#define GR_COLORCOMBINE_TEXTURE_TIMES_ALPHA 0x9
#define GR_COLORCOMBINE_TEXTURE_TIMES_ALPHA_ADD_ITRGB 0xa
#define GR_COLORCOMBINE_TEXTURE_ADD_ITRGB 0xb
#define GR_COLORCOMBINE_TEXTURE_SUB_ITRGB 0xc
#define GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA 0xd
#define GR_COLORCOMBINE_DIFF_SPEC_A 0xe
#define GR_COLORCOMBINE_DIFF_SPEC_B 0xf
#define GR_COLORCOMBINE_ONE 0x10

typedef FxI32 GrAlphaBlendFnc_t;
#define GR_BLEND_ZERO 0x0
#define GR_BLEND_SRC_ALPHA 0x1
#define GR_BLEND_SRC_COLOR 0x2
#define GR_BLEND_DST_COLOR GR_BLEND_SRC_COLOR
#define GR_BLEND_DST_ALPHA 0x3
#define GR_BLEND_ONE 0x4
#define GR_BLEND_ONE_MINUS_SRC_ALPHA 0x5
#define GR_BLEND_ONE_MINUS_SRC_COLOR 0x6
#define GR_BLEND_ONE_MINUS_DST_COLOR GR_BLEND_ONE_MINUS_SRC_COLOR
#define GR_BLEND_ONE_MINUS_DST_ALPHA 0x7
#define GR_BLEND_RESERVED_8 0x8
#define GR_BLEND_RESERVED_9 0x9
#define GR_BLEND_RESERVED_A 0xa
#define GR_BLEND_RESERVED_B 0xb
#define GR_BLEND_RESERVED_C 0xc
#define GR_BLEND_RESERVED_D 0xd
#define GR_BLEND_RESERVED_E 0xe
#define GR_BLEND_ALPHA_SATURATE 0xf
#define GR_BLEND_PREFOG_COLOR GR_BLEND_ALPHA_SATURATE
#define GR_BLEND_SAME_COLOR_EXT           0x08
#define GR_BLEND_ONE_MINUS_SAME_COLOR_EXT 0x09

typedef FxI32 GrAspectRatio_t;
#define GR_ASPECT_LOG2_8x1        3
#define GR_ASPECT_LOG2_4x1        2
#define GR_ASPECT_LOG2_2x1        1
#define GR_ASPECT_LOG2_1x1        0
#define GR_ASPECT_LOG2_1x2       -1
#define GR_ASPECT_LOG2_1x4       -2
#define GR_ASPECT_LOG2_1x8       -3

typedef FxI32 GrBuffer_t;
#define GR_BUFFER_FRONTBUFFER   0x0
#define GR_BUFFER_BACKBUFFER    0x1
#define GR_BUFFER_AUXBUFFER     0x2
#define GR_BUFFER_DEPTHBUFFER   0x3
#define GR_BUFFER_ALPHABUFFER   0x4
#define GR_BUFFER_TRIPLEBUFFER  0x5

typedef FxI32 GrChromakeyMode_t;
#define GR_CHROMAKEY_DISABLE    0x0
#define GR_CHROMAKEY_ENABLE     0x1

typedef FxI32 GrChromaRangeMode_t;
#define GR_CHROMARANGE_RGB_ALL_EXT  0x0

#define GR_CHROMARANGE_DISABLE_EXT  0x00
#define GR_CHROMARANGE_ENABLE_EXT   0x01

typedef FxI32 GrTexChromakeyMode_t;
#define GR_TEXCHROMA_DISABLE_EXT               0x0
#define GR_TEXCHROMA_ENABLE_EXT                0x1

#define GR_TEXCHROMARANGE_RGB_ALL_EXT  0x0

typedef FxI32 GrCmpFnc_t;
#define GR_CMP_NEVER    0x0
#define GR_CMP_LESS     0x1
#define GR_CMP_EQUAL    0x2
#define GR_CMP_LEQUAL   0x3
#define GR_CMP_GREATER  0x4
#define GR_CMP_NOTEQUAL 0x5
#define GR_CMP_GEQUAL   0x6
#define GR_CMP_ALWAYS   0x7

typedef FxI32 GrColorFormat_t;
#define GR_COLORFORMAT_ARGB     0x0
#define GR_COLORFORMAT_ABGR     0x1

#define GR_COLORFORMAT_RGBA     0x2
#define GR_COLORFORMAT_BGRA     0x3

typedef FxI32 GrCullMode_t;
#define GR_CULL_DISABLE         0x0
#define GR_CULL_NEGATIVE        0x1
#define GR_CULL_POSITIVE        0x2

typedef FxI32 GrDepthBufferMode_t;
#define GR_DEPTHBUFFER_DISABLE                  0x0
#define GR_DEPTHBUFFER_ZBUFFER                  0x1
#define GR_DEPTHBUFFER_WBUFFER                  0x2
#define GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS  0x3
#define GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS  0x4

typedef FxI32 GrDitherMode_t;
#define GR_DITHER_DISABLE       0x0
#define GR_DITHER_2x2           0x1
#define GR_DITHER_4x4           0x2

typedef FxI32 GrStippleMode_t;
#define GR_STIPPLE_DISABLE	0x0
#define GR_STIPPLE_PATTERN	0x1
#define GR_STIPPLE_ROTATE	0x2

typedef FxI32 GrFogMode_t;
#define GR_FOG_DISABLE                     0x0
#define GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT  0x1
#define GR_FOG_WITH_TABLE_ON_Q             0x2
#define GR_FOG_WITH_TABLE_ON_W             GR_FOG_WITH_TABLE_ON_Q
#define GR_FOG_WITH_ITERATED_Z             0x3
#define GR_FOG_WITH_ITERATED_ALPHA_EXT     0x4
#define GR_FOG_MULT2                       0x100
#define GR_FOG_ADD2                        0x200

typedef FxU32 GrLock_t;
#define GR_LFB_READ_ONLY  0x00
#define GR_LFB_WRITE_ONLY 0x01
#define GR_LFB_IDLE       0x00
#define GR_LFB_NOIDLE     0x10

typedef FxI32 GrLfbBypassMode_t;
#define GR_LFBBYPASS_DISABLE    0x0
#define GR_LFBBYPASS_ENABLE     0x1

typedef FxI32 GrLfbWriteMode_t;
#define GR_LFBWRITEMODE_565        0x0
#define GR_LFBWRITEMODE_555        0x1
#define GR_LFBWRITEMODE_1555       0x2
#define GR_LFBWRITEMODE_RESERVED1  0x3
#define GR_LFBWRITEMODE_888        0x4
#define GR_LFBWRITEMODE_8888       0x5
#define GR_LFBWRITEMODE_RESERVED2  0x6
#define GR_LFBWRITEMODE_RESERVED3  0x7
#define GR_LFBWRITEMODE_RESERVED4  0x8
#define GR_LFBWRITEMODE_RESERVED5  0x9
#define GR_LFBWRITEMODE_RESERVED6  0xa
#define GR_LFBWRITEMODE_RESERVED7  0xb
#define GR_LFBWRITEMODE_565_DEPTH  0xc
#define GR_LFBWRITEMODE_555_DEPTH  0xd
#define GR_LFBWRITEMODE_1555_DEPTH 0xe
#define GR_LFBWRITEMODE_ZA16       0xf
#define GR_LFBWRITEMODE_ANY        0xFF

typedef FxI32 GrOriginLocation_t;
#define GR_ORIGIN_UPPER_LEFT    0x0
#define GR_ORIGIN_LOWER_LEFT    0x1
#define GR_ORIGIN_ANY           0xFF

typedef struct
{
   int size;
   void *lfbPtr;
   FxU32 strideInBytes;
   GrLfbWriteMode_t writeMode;
   GrOriginLocation_t origin;
}
GrLfbInfo_t;

typedef FxI32 GrLOD_t;
#define GR_LOD_LOG2_2048        0xb
#define GR_LOD_LOG2_1024        0xa
#define GR_LOD_LOG2_512         0x9
#define GR_LOD_LOG2_256         0x8
#define GR_LOD_LOG2_128         0x7
#define GR_LOD_LOG2_64          0x6
#define GR_LOD_LOG2_32          0x5
#define GR_LOD_LOG2_16          0x4
#define GR_LOD_LOG2_8           0x3
#define GR_LOD_LOG2_4           0x2
#define GR_LOD_LOG2_2           0x1
#define GR_LOD_LOG2_1           0x0

typedef FxI32 GrMipMapMode_t;
#define GR_MIPMAP_DISABLE               0x0
#define GR_MIPMAP_NEAREST               0x1
#define GR_MIPMAP_NEAREST_DITHER        0x2

typedef FxI32 GrSmoothingMode_t;
#define GR_SMOOTHING_DISABLE    0x0
#define GR_SMOOTHING_ENABLE     0x1

typedef FxI32 GrTextureClampMode_t;
#define GR_TEXTURECLAMP_WRAP        0x0
#define GR_TEXTURECLAMP_CLAMP       0x1
#define GR_TEXTURECLAMP_MIRROR_EXT  0x2

typedef FxI32 GrTextureCombineFnc_t;
#define GR_TEXTURECOMBINE_ZERO          0x0
#define GR_TEXTURECOMBINE_DECAL         0x1
#define GR_TEXTURECOMBINE_OTHER         0x2
#define GR_TEXTURECOMBINE_ADD           0x3
#define GR_TEXTURECOMBINE_MULTIPLY      0x4
#define GR_TEXTURECOMBINE_SUBTRACT      0x5
#define GR_TEXTURECOMBINE_DETAIL        0x6
#define GR_TEXTURECOMBINE_DETAIL_OTHER  0x7
#define GR_TEXTURECOMBINE_TRILINEAR_ODD 0x8
#define GR_TEXTURECOMBINE_TRILINEAR_EVEN 0x9
#define GR_TEXTURECOMBINE_ONE           0xa

typedef FxI32 GrTextureFilterMode_t;
#define GR_TEXTUREFILTER_POINT_SAMPLED  0x0
#define GR_TEXTUREFILTER_BILINEAR       0x1

typedef FxI32 GrTextureFormat_t;
#define GR_TEXFMT_8BIT                  0x0
#define GR_TEXFMT_RGB_332 GR_TEXFMT_8BIT
#define GR_TEXFMT_YIQ_422               0x1
#define GR_TEXFMT_ALPHA_8               0x2
#define GR_TEXFMT_INTENSITY_8           0x3
#define GR_TEXFMT_ALPHA_INTENSITY_44    0x4
#define GR_TEXFMT_P_8                   0x5
#define GR_TEXFMT_RSVD0                 0x6
#define GR_TEXFMT_RSVD1                 0x7
#define GR_TEXFMT_16BIT                 0x8
#define GR_TEXFMT_ARGB_8332 GR_TEXFMT_16BIT
#define GR_TEXFMT_AYIQ_8422             0x9
#define GR_TEXFMT_RGB_565               0xa
#define GR_TEXFMT_ARGB_1555             0xb
#define GR_TEXFMT_ARGB_4444             0xc
#define GR_TEXFMT_ALPHA_INTENSITY_88    0xd
#define GR_TEXFMT_AP_88                 0xe
#define GR_TEXFMT_RSVD2                 0xf
#define GR_TEXFMT_ARGB_CMP_FXT1           0x11
#define GR_TEXFMT_ARGB_8888               0x12
#define GR_TEXFMT_YUYV_422                0x13
#define GR_TEXFMT_UYVY_422                0x14
#define GR_TEXFMT_AYUV_444                0x15
#define GR_TEXFMT_ARGB_CMP_DXT1           0x16
#define GR_TEXFMT_ARGB_CMP_DXT2           0x17
#define GR_TEXFMT_ARGB_CMP_DXT3           0x18
#define GR_TEXFMT_ARGB_CMP_DXT4           0x19
#define GR_TEXFMT_ARGB_CMP_DXT5           0x1A

typedef FxU32 GrTexTable_t;
#define GR_TEXTABLE_NCC0                 0x0
#define GR_TEXTABLE_NCC1                 0x1
#define GR_TEXTABLE_PALETTE              0x2
#define GR_TEXTABLE_PALETTE_6666_EXT     0x3

typedef FxU32 GrNCCTable_t;
#define GR_NCCTABLE_NCC0    0x0
#define GR_NCCTABLE_NCC1    0x1

typedef FxU32 GrTexBaseRange_t;
#define GR_TEXBASE_256      0x3
#define GR_TEXBASE_128      0x2
#define GR_TEXBASE_64       0x1
#define GR_TEXBASE_32_TO_1  0x0
#define GR_TEXBASE_2048     0x7
#define GR_TEXBASE_1024     0x6
#define GR_TEXBASE_512      0x5
#define GR_TEXBASE_256_TO_1 0x4

typedef FxU32 GrEnableMode_t;
#define GR_MODE_DISABLE     0x0
#define GR_MODE_ENABLE      0x1

#define GR_AA_ORDERED            0x01
#define GR_ALLOW_MIPMAP_DITHER   0x02
#define GR_PASSTHRU              0x03
#define GR_SHAMELESS_PLUG        0x04
#define GR_VIDEO_SMOOTHING       0x05

typedef FxU32 GrCoordinateSpaceMode_t;
#define GR_WINDOW_COORDS    0x00
#define GR_CLIP_COORDS      0x01

/* Parameters for strips */
#define GR_PARAM_XY       0x01
#define GR_PARAM_Z        0x02
#define GR_PARAM_W        0x03
#define GR_PARAM_Q        0x04
#define GR_PARAM_FOG_EXT  0x05

#define GR_PARAM_A        0x10

#define GR_PARAM_RGB      0x20

#define GR_PARAM_PARGB    0x30

#define GR_PARAM_ST0      0x40
#define GR_PARAM_ST1      GR_PARAM_ST0+1
#define GR_PARAM_ST2      GR_PARAM_ST0+2

#define GR_PARAM_Q0       0x50
#define GR_PARAM_Q1       GR_PARAM_Q0+1
#define GR_PARAM_Q2       GR_PARAM_Q0+2

#define GR_PARAM_DISABLE  0x00
#define GR_PARAM_ENABLE   0x01

/* grDrawVertexArray/grDrawVertexArrayContiguous */
#define GR_POINTS                        0
#define GR_LINE_STRIP                    1
#define GR_LINES                         2
#define GR_POLYGON                       3
#define GR_TRIANGLE_STRIP                4
#define GR_TRIANGLE_FAN                  5
#define GR_TRIANGLES                     6
#define GR_TRIANGLE_STRIP_CONTINUE       7
#define GR_TRIANGLE_FAN_CONTINUE         8

/* grGet/grReset */
#define GR_BITS_DEPTH                   0x01
#define GR_BITS_RGBA                    0x02
#define GR_FIFO_FULLNESS                0x03
#define GR_FOG_TABLE_ENTRIES            0x04
#define GR_GAMMA_TABLE_ENTRIES          0x05
#define GR_GLIDE_STATE_SIZE             0x06
#define GR_GLIDE_VERTEXLAYOUT_SIZE      0x07
#define GR_IS_BUSY                      0x08
#define GR_LFB_PIXEL_PIPE               0x09
#define GR_MAX_TEXTURE_SIZE             0x0a
#define GR_MAX_TEXTURE_ASPECT_RATIO     0x0b
#define GR_MEMORY_FB                    0x0c
#define GR_MEMORY_TMU                   0x0d
#define GR_MEMORY_UMA                   0x0e
#define GR_NUM_BOARDS                   0x0f
#define GR_NON_POWER_OF_TWO_TEXTURES    0x10
#define GR_NUM_FB                       0x11
#define GR_NUM_SWAP_HISTORY_BUFFER      0x12
#define GR_NUM_TMU                      0x13
#define GR_PENDING_BUFFERSWAPS          0x14
#define GR_REVISION_FB                  0x15
#define GR_REVISION_TMU                 0x16
#define GR_STATS_LINES                  0x17
#define GR_STATS_PIXELS_AFUNC_FAIL      0x18
#define GR_STATS_PIXELS_CHROMA_FAIL     0x19
#define GR_STATS_PIXELS_DEPTHFUNC_FAIL  0x1a
#define GR_STATS_PIXELS_IN              0x1b
#define GR_STATS_PIXELS_OUT             0x1c
#define GR_STATS_PIXELS                 0x1d
#define GR_STATS_POINTS                 0x1e
#define GR_STATS_TRIANGLES_IN           0x1f
#define GR_STATS_TRIANGLES_OUT          0x20
#define GR_STATS_TRIANGLES              0x21
#define GR_SWAP_HISTORY                 0x22
#define GR_SUPPORTS_PASSTHRU            0x23
#define GR_TEXTURE_ALIGN                0x24
#define GR_VIDEO_POSITION               0x25
#define GR_VIEWPORT                     0x26
#define GR_WDEPTH_MIN_MAX               0x27
#define GR_ZDEPTH_MIN_MAX               0x28
#define GR_VERTEX_PARAMETER             0x29
#define GR_BITS_GAMMA                   0x2a
#define GR_GET_RESERVED_1               0x1000

/* grGetString types */
#define GR_EXTENSION                    0xa0
#define GR_HARDWARE                     0xa1
#define GR_RENDERER                     0xa2
#define GR_VENDOR                       0xa3
#define GR_VERSION                      0xa4

typedef FxI32 GrScreenRefresh_t;
#define GR_REFRESH_NONE   0xff

typedef FxI32 GrScreenResolution_t;
#define GR_RESOLUTION_NONE      0xff

typedef struct
{
   GrLOD_t smallLodLog2;
   GrLOD_t largeLodLog2;
   GrAspectRatio_t aspectRatioLog2;
   GrTextureFormat_t format;
   void *data;
}
GrTexInfo;

typedef struct GrSstPerfStats_s
{
   FxU32 pixelsIn;
   FxU32 chromaFail;
   FxU32 zFuncFail;
   FxU32 aFuncFail;
   FxU32 pixelsOut;
}
GrSstPerfStats_t;

typedef struct
{
   GrScreenResolution_t resolution;
   GrScreenRefresh_t refresh;
   int numColorBuffers;
   int numAuxBuffers;
}
GrResolution;

typedef GrResolution GlideResolution;
#define GR_QUERY_ANY  ((FxU32)(~0))

typedef FxU32 GrLfbSrcFmt_t;
#define GR_LFB_SRC_FMT_565          0x00
#define GR_LFB_SRC_FMT_555          0x01
#define GR_LFB_SRC_FMT_1555         0x02
#define GR_LFB_SRC_FMT_888          0x04
#define GR_LFB_SRC_FMT_8888         0x05
#define GR_LFB_SRC_FMT_565_DEPTH    0x0c
#define GR_LFB_SRC_FMT_555_DEPTH    0x0d
#define GR_LFB_SRC_FMT_1555_DEPTH   0x0e
#define GR_LFB_SRC_FMT_ZA16         0x0f
#define GR_LFB_SRC_FMT_RLE16        0x80

typedef FxU32 GrPixelFormat_t;
#define GR_PIXFMT_I_8                           0x0001
#define GR_PIXFMT_AI_88                         0x0002
#define GR_PIXFMT_RGB_565                       0x0003
#define GR_PIXFMT_ARGB_1555                     0x0004
#define GR_PIXFMT_ARGB_8888                     0x0005
#define GR_PIXFMT_AA_2_RGB_565                  0x0006
#define GR_PIXFMT_AA_2_ARGB_1555                0x0007
#define GR_PIXFMT_AA_2_ARGB_8888                0x0008
#define GR_PIXFMT_AA_4_RGB_565                  0x0009
#define GR_PIXFMT_AA_4_ARGB_1555                0x000a
#define GR_PIXFMT_AA_4_ARGB_8888                0x000b

#define GR_LFBWRITEMODE_Z32                     0x0008

typedef FxU32 GrAAMode_t;
#define GR_AA_NONE                              0x0000
#define GR_AA_4SAMPLES                          0x0001

typedef FxU8 GrStencil_t;

typedef FxU32 GrStencilOp_t;
#define GR_STENCILOP_KEEP        0x00
#define GR_STENCILOP_ZERO        0x01
#define GR_STENCILOP_REPLACE     0x02
#define GR_STENCILOP_INCR_CLAMP  0x03
#define GR_STENCILOP_DECR_CLAMP  0x04
#define GR_STENCILOP_INVERT      0x05
#define GR_STENCILOP_INCR_WRAP   0x06
#define GR_STENCILOP_DECR_WRAP   0x07

#define GR_TEXTURE_UMA_EXT       0x06
#define GR_STENCIL_MODE_EXT      0x07
#define GR_OPENGL_MODE_EXT       0x08

typedef FxU32 GrCCUColor_t;
typedef FxU32 GrACUColor_t;
typedef FxU32 GrTCCUColor_t;
typedef FxU32 GrTACUColor_t;
#define GR_CMBX_ZERO                      0x00
#define GR_CMBX_TEXTURE_ALPHA             0x01
#define GR_CMBX_ALOCAL                    0x02
#define GR_CMBX_AOTHER                    0x03
#define GR_CMBX_B                         0x04
#define GR_CMBX_CONSTANT_ALPHA            0x05
#define GR_CMBX_CONSTANT_COLOR            0x06
#define GR_CMBX_DETAIL_FACTOR             0x07
#define GR_CMBX_ITALPHA                   0x08
#define GR_CMBX_ITRGB                     0x09
#define GR_CMBX_LOCAL_TEXTURE_ALPHA       0x0a
#define GR_CMBX_LOCAL_TEXTURE_RGB         0x0b
#define GR_CMBX_LOD_FRAC                  0x0c
#define GR_CMBX_OTHER_TEXTURE_ALPHA       0x0d
#define GR_CMBX_OTHER_TEXTURE_RGB         0x0e
#define GR_CMBX_TEXTURE_RGB               0x0f
#define GR_CMBX_TMU_CALPHA                0x10
#define GR_CMBX_TMU_CCOLOR                0x11

typedef FxU32 GrCombineMode_t;
#define GR_FUNC_MODE_ZERO                 0x00
#define GR_FUNC_MODE_X                    0x01
#define GR_FUNC_MODE_ONE_MINUS_X          0x02
#define GR_FUNC_MODE_NEGATIVE_X           0x03
#define GR_FUNC_MODE_X_MINUS_HALF         0x04

typedef FxU32 GrAlphaBlendOp_t;
#define GR_BLEND_OP_ADD                   0x00
#define GR_BLEND_OP_SUB                   0x01
#define GR_BLEND_OP_REVSUB                0x02

typedef struct
{
   FxU32 data[256];
}
GuTexPalette;

typedef void (*GrErrorCallbackFnc_t) (const char *string, FxBool fatal);

#endif
