/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
 

#ifndef BRW_DEFINES_H
#define BRW_DEFINES_H

/*
 */
#define MI_NOOP                              0x00
#define MI_USER_INTERRUPT                    0x02
#define MI_WAIT_FOR_EVENT                    0x03
#define MI_FLUSH                             0x04
#define MI_REPORT_HEAD                       0x07
#define MI_ARB_ON_OFF                        0x08
#define MI_BATCH_BUFFER_END                  0x0A
#define MI_OVERLAY_FLIP                      0x11
#define MI_LOAD_SCAN_LINES_INCL              0x12
#define MI_LOAD_SCAN_LINES_EXCL              0x13
#define MI_DISPLAY_BUFFER_INFO               0x14
#define MI_SET_CONTEXT                       0x18
#define MI_STORE_DATA_IMM                    0x20
#define MI_STORE_DATA_INDEX                  0x21
#define MI_LOAD_REGISTER_IMM                 0x22
#define MI_STORE_REGISTER_MEM                0x24
#define MI_BATCH_BUFFER_START                0x31

#define MI_SYNCHRONOUS_FLIP                  0x0 
#define MI_ASYNCHRONOUS_FLIP                 0x1

#define MI_BUFFER_SECURE                     0x0 
#define MI_BUFFER_NONSECURE                  0x1

#define MI_ARBITRATE_AT_CHAIN_POINTS         0x0 
#define MI_ARBITRATE_BETWEEN_INSTS           0x1
#define MI_NO_ARBITRATION                    0x3 

#define MI_CONDITION_CODE_WAIT_DISABLED      0x0
#define MI_CONDITION_CODE_WAIT_0             0x1
#define MI_CONDITION_CODE_WAIT_1             0x2
#define MI_CONDITION_CODE_WAIT_2             0x3
#define MI_CONDITION_CODE_WAIT_3             0x4
#define MI_CONDITION_CODE_WAIT_4             0x5

#define MI_DISPLAY_PIPE_A                    0x0
#define MI_DISPLAY_PIPE_B                    0x1

#define MI_DISPLAY_PLANE_A                   0x0 
#define MI_DISPLAY_PLANE_B                   0x1
#define MI_DISPLAY_PLANE_C                   0x2

#define MI_STANDARD_FLIP                                 0x0
#define MI_ENQUEUE_FLIP_PERFORM_BASE_FRAME_NUMBER_LOAD   0x1
#define MI_ENQUEUE_FLIP_TARGET_FRAME_NUMBER_RELATIVE     0x2
#define MI_ENQUEUE_FLIP_ABSOLUTE_TARGET_FRAME_NUMBER     0x3

#define MI_PHYSICAL_ADDRESS                  0x0
#define MI_VIRTUAL_ADDRESS                   0x1

#define MI_BUFFER_MEMORY_MAIN                0x0 
#define MI_BUFFER_MEMORY_GTT                 0x2
#define MI_BUFFER_MEMORY_PER_PROCESS_GTT     0x3 

#define MI_FLIP_CONTINUE                     0x0
#define MI_FLIP_ON                           0x1
#define MI_FLIP_OFF                          0x2

#define MI_UNTRUSTED_REGISTER_SPACE          0x0
#define MI_TRUSTED_REGISTER_SPACE            0x1

/* 3D state:
 */
#define _3DOP_3DSTATE_PIPELINED       0x0
#define _3DOP_3DSTATE_NONPIPELINED    0x1
#define _3DOP_3DCONTROL               0x2
#define _3DOP_3DPRIMITIVE             0x3

#define _3DSTATE_PIPELINED_POINTERS       0x00
#define _3DSTATE_BINDING_TABLE_POINTERS   0x01
#define _3DSTATE_VERTEX_BUFFERS           0x08
#define _3DSTATE_VERTEX_ELEMENTS          0x09
#define _3DSTATE_INDEX_BUFFER             0x0A
#define _3DSTATE_VF_STATISTICS            0x0B
#define _3DSTATE_DRAWING_RECTANGLE            0x00
#define _3DSTATE_CONSTANT_COLOR               0x01
#define _3DSTATE_SAMPLER_PALETTE_LOAD         0x02
#define _3DSTATE_CHROMA_KEY                   0x04
#define _3DSTATE_DEPTH_BUFFER                 0x05
#define _3DSTATE_POLY_STIPPLE_OFFSET          0x06
#define _3DSTATE_POLY_STIPPLE_PATTERN         0x07
#define _3DSTATE_LINE_STIPPLE                 0x08
#define _3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP    0x09
#define _3DCONTROL    0x00
#define _3DPRIMITIVE  0x00

#define PIPE_CONTROL_NOWRITE          0x00
#define PIPE_CONTROL_WRITEIMMEDIATE   0x01
#define PIPE_CONTROL_WRITEDEPTH       0x02
#define PIPE_CONTROL_WRITETIMESTAMP   0x03

#define PIPE_CONTROL_GTTWRITE_PROCESS_LOCAL 0x00
#define PIPE_CONTROL_GTTWRITE_GLOBAL        0x01

#define _3DPRIM_POINTLIST         0x01
#define _3DPRIM_LINELIST          0x02
#define _3DPRIM_LINESTRIP         0x03
#define _3DPRIM_TRILIST           0x04
#define _3DPRIM_TRISTRIP          0x05
#define _3DPRIM_TRIFAN            0x06
#define _3DPRIM_QUADLIST          0x07
#define _3DPRIM_QUADSTRIP         0x08
#define _3DPRIM_LINELIST_ADJ      0x09
#define _3DPRIM_LINESTRIP_ADJ     0x0A
#define _3DPRIM_TRILIST_ADJ       0x0B
#define _3DPRIM_TRISTRIP_ADJ      0x0C
#define _3DPRIM_TRISTRIP_REVERSE  0x0D
#define _3DPRIM_POLYGON           0x0E
#define _3DPRIM_RECTLIST          0x0F
#define _3DPRIM_LINELOOP          0x10
#define _3DPRIM_POINTLIST_BF      0x11
#define _3DPRIM_LINESTRIP_CONT    0x12
#define _3DPRIM_LINESTRIP_BF      0x13
#define _3DPRIM_LINESTRIP_CONT_BF 0x14
#define _3DPRIM_TRIFAN_NOSTIPPLE  0x15

#define _3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL 0
#define _3DPRIM_VERTEXBUFFER_ACCESS_RANDOM     1

#define BRW_ANISORATIO_2     0 
#define BRW_ANISORATIO_4     1 
#define BRW_ANISORATIO_6     2 
#define BRW_ANISORATIO_8     3 
#define BRW_ANISORATIO_10    4 
#define BRW_ANISORATIO_12    5 
#define BRW_ANISORATIO_14    6 
#define BRW_ANISORATIO_16    7

#define BRW_BLENDFACTOR_ONE                 0x1
#define BRW_BLENDFACTOR_SRC_COLOR           0x2
#define BRW_BLENDFACTOR_SRC_ALPHA           0x3
#define BRW_BLENDFACTOR_DST_ALPHA           0x4
#define BRW_BLENDFACTOR_DST_COLOR           0x5
#define BRW_BLENDFACTOR_SRC_ALPHA_SATURATE  0x6
#define BRW_BLENDFACTOR_CONST_COLOR         0x7
#define BRW_BLENDFACTOR_CONST_ALPHA         0x8
#define BRW_BLENDFACTOR_SRC1_COLOR          0x9
#define BRW_BLENDFACTOR_SRC1_ALPHA          0x0A
#define BRW_BLENDFACTOR_ZERO                0x11
#define BRW_BLENDFACTOR_INV_SRC_COLOR       0x12
#define BRW_BLENDFACTOR_INV_SRC_ALPHA       0x13
#define BRW_BLENDFACTOR_INV_DST_ALPHA       0x14
#define BRW_BLENDFACTOR_INV_DST_COLOR       0x15
#define BRW_BLENDFACTOR_INV_CONST_COLOR     0x17
#define BRW_BLENDFACTOR_INV_CONST_ALPHA     0x18
#define BRW_BLENDFACTOR_INV_SRC1_COLOR      0x19
#define BRW_BLENDFACTOR_INV_SRC1_ALPHA      0x1A

#define BRW_BLENDFUNCTION_ADD               0
#define BRW_BLENDFUNCTION_SUBTRACT          1
#define BRW_BLENDFUNCTION_REVERSE_SUBTRACT  2
#define BRW_BLENDFUNCTION_MIN               3
#define BRW_BLENDFUNCTION_MAX               4

#define BRW_ALPHATEST_FORMAT_UNORM8         0
#define BRW_ALPHATEST_FORMAT_FLOAT32        1

#define BRW_CHROMAKEY_KILL_ON_ANY_MATCH  0
#define BRW_CHROMAKEY_REPLACE_BLACK      1

#define BRW_CLIP_API_OGL     0
#define BRW_CLIP_API_DX      1

#define BRW_CLIPMODE_NORMAL              0
#define BRW_CLIPMODE_CLIP_ALL            1
#define BRW_CLIPMODE_CLIP_NON_REJECTED   2
#define BRW_CLIPMODE_REJECT_ALL          3
#define BRW_CLIPMODE_ACCEPT_ALL          4

#define BRW_CLIP_NDCSPACE     0
#define BRW_CLIP_SCREENSPACE  1

#define BRW_COMPAREFUNCTION_ALWAYS       0
#define BRW_COMPAREFUNCTION_NEVER        1
#define BRW_COMPAREFUNCTION_LESS         2
#define BRW_COMPAREFUNCTION_EQUAL        3
#define BRW_COMPAREFUNCTION_LEQUAL       4
#define BRW_COMPAREFUNCTION_GREATER      5
#define BRW_COMPAREFUNCTION_NOTEQUAL     6
#define BRW_COMPAREFUNCTION_GEQUAL       7

#define BRW_COVERAGE_PIXELS_HALF     0
#define BRW_COVERAGE_PIXELS_1        1
#define BRW_COVERAGE_PIXELS_2        2
#define BRW_COVERAGE_PIXELS_4        3

#define BRW_CULLMODE_BOTH        0
#define BRW_CULLMODE_NONE        1
#define BRW_CULLMODE_FRONT       2
#define BRW_CULLMODE_BACK        3

#define BRW_DEFAULTCOLOR_R8G8B8A8_UNORM      0
#define BRW_DEFAULTCOLOR_R32G32B32A32_FLOAT  1

#define BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT     0
#define BRW_DEPTHFORMAT_D32_FLOAT                1
#define BRW_DEPTHFORMAT_D24_UNORM_S8_UINT        2
#define BRW_DEPTHFORMAT_D16_UNORM                5

#define BRW_FLOATING_POINT_IEEE_754        0
#define BRW_FLOATING_POINT_NON_IEEE_754    1

#define BRW_FRONTWINDING_CW      0
#define BRW_FRONTWINDING_CCW     1

#define BRW_INDEX_BYTE     0
#define BRW_INDEX_WORD     1
#define BRW_INDEX_DWORD    2

#define BRW_LOGICOPFUNCTION_CLEAR            0
#define BRW_LOGICOPFUNCTION_NOR              1
#define BRW_LOGICOPFUNCTION_AND_INVERTED     2
#define BRW_LOGICOPFUNCTION_COPY_INVERTED    3
#define BRW_LOGICOPFUNCTION_AND_REVERSE      4
#define BRW_LOGICOPFUNCTION_INVERT           5
#define BRW_LOGICOPFUNCTION_XOR              6
#define BRW_LOGICOPFUNCTION_NAND             7
#define BRW_LOGICOPFUNCTION_AND              8
#define BRW_LOGICOPFUNCTION_EQUIV            9
#define BRW_LOGICOPFUNCTION_NOOP             10
#define BRW_LOGICOPFUNCTION_OR_INVERTED      11
#define BRW_LOGICOPFUNCTION_COPY             12
#define BRW_LOGICOPFUNCTION_OR_REVERSE       13
#define BRW_LOGICOPFUNCTION_OR               14
#define BRW_LOGICOPFUNCTION_SET              15  

#define BRW_MAPFILTER_NEAREST        0x0 
#define BRW_MAPFILTER_LINEAR         0x1 
#define BRW_MAPFILTER_ANISOTROPIC    0x2

#define BRW_MIPFILTER_NONE        0   
#define BRW_MIPFILTER_NEAREST     1   
#define BRW_MIPFILTER_LINEAR      3

#define BRW_POLYGON_FRONT_FACING     0
#define BRW_POLYGON_BACK_FACING      1

#define BRW_PREFILTER_ALWAYS     0x0 
#define BRW_PREFILTER_NEVER      0x1
#define BRW_PREFILTER_LESS       0x2
#define BRW_PREFILTER_EQUAL      0x3
#define BRW_PREFILTER_LEQUAL     0x4
#define BRW_PREFILTER_GREATER    0x5
#define BRW_PREFILTER_NOTEQUAL   0x6
#define BRW_PREFILTER_GEQUAL     0x7

#define BRW_PROVOKING_VERTEX_0    0
#define BRW_PROVOKING_VERTEX_1    1 
#define BRW_PROVOKING_VERTEX_2    2

#define BRW_RASTRULE_UPPER_LEFT  0    
#define BRW_RASTRULE_UPPER_RIGHT 1

#define BRW_RENDERTARGET_CLAMPRANGE_UNORM    0
#define BRW_RENDERTARGET_CLAMPRANGE_SNORM    1
#define BRW_RENDERTARGET_CLAMPRANGE_FORMAT   2

#define BRW_STENCILOP_KEEP               0
#define BRW_STENCILOP_ZERO               1
#define BRW_STENCILOP_REPLACE            2
#define BRW_STENCILOP_INCRSAT            3
#define BRW_STENCILOP_DECRSAT            4
#define BRW_STENCILOP_INCR               5
#define BRW_STENCILOP_DECR               6
#define BRW_STENCILOP_INVERT             7

#define BRW_SURFACE_MIPMAPLAYOUT_BELOW   0
#define BRW_SURFACE_MIPMAPLAYOUT_RIGHT   1

#define BRW_SURFACEFORMAT_R32G32B32A32_FLOAT             0x000 
#define BRW_SURFACEFORMAT_R32G32B32A32_SINT              0x001 
#define BRW_SURFACEFORMAT_R32G32B32A32_UINT              0x002 
#define BRW_SURFACEFORMAT_R32G32B32A32_UNORM             0x003 
#define BRW_SURFACEFORMAT_R32G32B32A32_SNORM             0x004 
#define BRW_SURFACEFORMAT_R64G64_FLOAT                   0x005 
#define BRW_SURFACEFORMAT_R32G32B32X32_FLOAT             0x006 
#define BRW_SURFACEFORMAT_R32G32B32A32_SSCALED           0x007
#define BRW_SURFACEFORMAT_R32G32B32A32_USCALED           0x008
#define BRW_SURFACEFORMAT_R32G32B32_FLOAT                0x040 
#define BRW_SURFACEFORMAT_R32G32B32_SINT                 0x041 
#define BRW_SURFACEFORMAT_R32G32B32_UINT                 0x042 
#define BRW_SURFACEFORMAT_R32G32B32_UNORM                0x043 
#define BRW_SURFACEFORMAT_R32G32B32_SNORM                0x044 
#define BRW_SURFACEFORMAT_R32G32B32_SSCALED              0x045 
#define BRW_SURFACEFORMAT_R32G32B32_USCALED              0x046 
#define BRW_SURFACEFORMAT_R16G16B16A16_UNORM             0x080 
#define BRW_SURFACEFORMAT_R16G16B16A16_SNORM             0x081 
#define BRW_SURFACEFORMAT_R16G16B16A16_SINT              0x082 
#define BRW_SURFACEFORMAT_R16G16B16A16_UINT              0x083 
#define BRW_SURFACEFORMAT_R16G16B16A16_FLOAT             0x084 
#define BRW_SURFACEFORMAT_R32G32_FLOAT                   0x085 
#define BRW_SURFACEFORMAT_R32G32_SINT                    0x086 
#define BRW_SURFACEFORMAT_R32G32_UINT                    0x087 
#define BRW_SURFACEFORMAT_R32_FLOAT_X8X24_TYPELESS       0x088 
#define BRW_SURFACEFORMAT_X32_TYPELESS_G8X24_UINT        0x089 
#define BRW_SURFACEFORMAT_L32A32_FLOAT                   0x08A 
#define BRW_SURFACEFORMAT_R32G32_UNORM                   0x08B 
#define BRW_SURFACEFORMAT_R32G32_SNORM                   0x08C 
#define BRW_SURFACEFORMAT_R64_FLOAT                      0x08D 
#define BRW_SURFACEFORMAT_R16G16B16X16_UNORM             0x08E 
#define BRW_SURFACEFORMAT_R16G16B16X16_FLOAT             0x08F 
#define BRW_SURFACEFORMAT_A32X32_FLOAT                   0x090 
#define BRW_SURFACEFORMAT_L32X32_FLOAT                   0x091 
#define BRW_SURFACEFORMAT_I32X32_FLOAT                   0x092 
#define BRW_SURFACEFORMAT_R16G16B16A16_SSCALED           0x093
#define BRW_SURFACEFORMAT_R16G16B16A16_USCALED           0x094
#define BRW_SURFACEFORMAT_R32G32_SSCALED                 0x095
#define BRW_SURFACEFORMAT_R32G32_USCALED                 0x096
#define BRW_SURFACEFORMAT_B8G8R8A8_UNORM                 0x0C0 
#define BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB            0x0C1 
#define BRW_SURFACEFORMAT_R10G10B10A2_UNORM              0x0C2 
#define BRW_SURFACEFORMAT_R10G10B10A2_UNORM_SRGB         0x0C3 
#define BRW_SURFACEFORMAT_R10G10B10A2_UINT               0x0C4 
#define BRW_SURFACEFORMAT_R10G10B10_SNORM_A2_UNORM       0x0C5 
#define BRW_SURFACEFORMAT_R8G8B8A8_UNORM                 0x0C7 
#define BRW_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB            0x0C8 
#define BRW_SURFACEFORMAT_R8G8B8A8_SNORM                 0x0C9 
#define BRW_SURFACEFORMAT_R8G8B8A8_SINT                  0x0CA 
#define BRW_SURFACEFORMAT_R8G8B8A8_UINT                  0x0CB 
#define BRW_SURFACEFORMAT_R16G16_UNORM                   0x0CC 
#define BRW_SURFACEFORMAT_R16G16_SNORM                   0x0CD 
#define BRW_SURFACEFORMAT_R16G16_SINT                    0x0CE 
#define BRW_SURFACEFORMAT_R16G16_UINT                    0x0CF 
#define BRW_SURFACEFORMAT_R16G16_FLOAT                   0x0D0 
#define BRW_SURFACEFORMAT_B10G10R10A2_UNORM              0x0D1 
#define BRW_SURFACEFORMAT_B10G10R10A2_UNORM_SRGB         0x0D2 
#define BRW_SURFACEFORMAT_R11G11B10_FLOAT                0x0D3 
#define BRW_SURFACEFORMAT_R32_SINT                       0x0D6 
#define BRW_SURFACEFORMAT_R32_UINT                       0x0D7 
#define BRW_SURFACEFORMAT_R32_FLOAT                      0x0D8 
#define BRW_SURFACEFORMAT_R24_UNORM_X8_TYPELESS          0x0D9 
#define BRW_SURFACEFORMAT_X24_TYPELESS_G8_UINT           0x0DA 
#define BRW_SURFACEFORMAT_L16A16_UNORM                   0x0DF 
#define BRW_SURFACEFORMAT_I24X8_UNORM                    0x0E0 
#define BRW_SURFACEFORMAT_L24X8_UNORM                    0x0E1 
#define BRW_SURFACEFORMAT_A24X8_UNORM                    0x0E2 
#define BRW_SURFACEFORMAT_I32_FLOAT                      0x0E3 
#define BRW_SURFACEFORMAT_L32_FLOAT                      0x0E4 
#define BRW_SURFACEFORMAT_A32_FLOAT                      0x0E5 
#define BRW_SURFACEFORMAT_B8G8R8X8_UNORM                 0x0E9 
#define BRW_SURFACEFORMAT_B8G8R8X8_UNORM_SRGB            0x0EA 
#define BRW_SURFACEFORMAT_R8G8B8X8_UNORM                 0x0EB 
#define BRW_SURFACEFORMAT_R8G8B8X8_UNORM_SRGB            0x0EC 
#define BRW_SURFACEFORMAT_R9G9B9E5_SHAREDEXP             0x0ED 
#define BRW_SURFACEFORMAT_B10G10R10X2_UNORM              0x0EE 
#define BRW_SURFACEFORMAT_L16A16_FLOAT                   0x0F0 
#define BRW_SURFACEFORMAT_R32_UNORM                      0x0F1 
#define BRW_SURFACEFORMAT_R32_SNORM                      0x0F2 
#define BRW_SURFACEFORMAT_R10G10B10X2_USCALED            0x0F3
#define BRW_SURFACEFORMAT_R8G8B8A8_SSCALED               0x0F4
#define BRW_SURFACEFORMAT_R8G8B8A8_USCALED               0x0F5
#define BRW_SURFACEFORMAT_R16G16_SSCALED                 0x0F6
#define BRW_SURFACEFORMAT_R16G16_USCALED                 0x0F7
#define BRW_SURFACEFORMAT_R32_SSCALED                    0x0F8
#define BRW_SURFACEFORMAT_R32_USCALED                    0x0F9
#define BRW_SURFACEFORMAT_B5G6R5_UNORM                   0x100 
#define BRW_SURFACEFORMAT_B5G6R5_UNORM_SRGB              0x101 
#define BRW_SURFACEFORMAT_B5G5R5A1_UNORM                 0x102 
#define BRW_SURFACEFORMAT_B5G5R5A1_UNORM_SRGB            0x103 
#define BRW_SURFACEFORMAT_B4G4R4A4_UNORM                 0x104 
#define BRW_SURFACEFORMAT_B4G4R4A4_UNORM_SRGB            0x105 
#define BRW_SURFACEFORMAT_R8G8_UNORM                     0x106 
#define BRW_SURFACEFORMAT_R8G8_SNORM                     0x107 
#define BRW_SURFACEFORMAT_R8G8_SINT                      0x108 
#define BRW_SURFACEFORMAT_R8G8_UINT                      0x109 
#define BRW_SURFACEFORMAT_R16_UNORM                      0x10A 
#define BRW_SURFACEFORMAT_R16_SNORM                      0x10B 
#define BRW_SURFACEFORMAT_R16_SINT                       0x10C 
#define BRW_SURFACEFORMAT_R16_UINT                       0x10D 
#define BRW_SURFACEFORMAT_R16_FLOAT                      0x10E 
#define BRW_SURFACEFORMAT_I16_UNORM                      0x111 
#define BRW_SURFACEFORMAT_L16_UNORM                      0x112 
#define BRW_SURFACEFORMAT_A16_UNORM                      0x113 
#define BRW_SURFACEFORMAT_L8A8_UNORM                     0x114 
#define BRW_SURFACEFORMAT_I16_FLOAT                      0x115
#define BRW_SURFACEFORMAT_L16_FLOAT                      0x116
#define BRW_SURFACEFORMAT_A16_FLOAT                      0x117 
#define BRW_SURFACEFORMAT_R5G5_SNORM_B6_UNORM            0x119 
#define BRW_SURFACEFORMAT_B5G5R5X1_UNORM                 0x11A 
#define BRW_SURFACEFORMAT_B5G5R5X1_UNORM_SRGB            0x11B
#define BRW_SURFACEFORMAT_R8G8_SSCALED                   0x11C
#define BRW_SURFACEFORMAT_R8G8_USCALED                   0x11D
#define BRW_SURFACEFORMAT_R16_SSCALED                    0x11E
#define BRW_SURFACEFORMAT_R16_USCALED                    0x11F
#define BRW_SURFACEFORMAT_R8_UNORM                       0x140 
#define BRW_SURFACEFORMAT_R8_SNORM                       0x141 
#define BRW_SURFACEFORMAT_R8_SINT                        0x142 
#define BRW_SURFACEFORMAT_R8_UINT                        0x143 
#define BRW_SURFACEFORMAT_A8_UNORM                       0x144 
#define BRW_SURFACEFORMAT_I8_UNORM                       0x145 
#define BRW_SURFACEFORMAT_L8_UNORM                       0x146 
#define BRW_SURFACEFORMAT_P4A4_UNORM                     0x147 
#define BRW_SURFACEFORMAT_A4P4_UNORM                     0x148
#define BRW_SURFACEFORMAT_R8_SSCALED                     0x149
#define BRW_SURFACEFORMAT_R8_USCALED                     0x14A
#define BRW_SURFACEFORMAT_R1_UINT                        0x181 
#define BRW_SURFACEFORMAT_YCRCB_NORMAL                   0x182 
#define BRW_SURFACEFORMAT_YCRCB_SWAPUVY                  0x183 
#define BRW_SURFACEFORMAT_BC1_UNORM                      0x186 
#define BRW_SURFACEFORMAT_BC2_UNORM                      0x187 
#define BRW_SURFACEFORMAT_BC3_UNORM                      0x188 
#define BRW_SURFACEFORMAT_BC4_UNORM                      0x189 
#define BRW_SURFACEFORMAT_BC5_UNORM                      0x18A 
#define BRW_SURFACEFORMAT_BC1_UNORM_SRGB                 0x18B 
#define BRW_SURFACEFORMAT_BC2_UNORM_SRGB                 0x18C 
#define BRW_SURFACEFORMAT_BC3_UNORM_SRGB                 0x18D 
#define BRW_SURFACEFORMAT_MONO8                          0x18E 
#define BRW_SURFACEFORMAT_YCRCB_SWAPUV                   0x18F 
#define BRW_SURFACEFORMAT_YCRCB_SWAPY                    0x190 
#define BRW_SURFACEFORMAT_DXT1_RGB                       0x191 
#define BRW_SURFACEFORMAT_FXT1                           0x192 
#define BRW_SURFACEFORMAT_R8G8B8_UNORM                   0x193 
#define BRW_SURFACEFORMAT_R8G8B8_SNORM                   0x194 
#define BRW_SURFACEFORMAT_R8G8B8_SSCALED                 0x195 
#define BRW_SURFACEFORMAT_R8G8B8_USCALED                 0x196 
#define BRW_SURFACEFORMAT_R64G64B64A64_FLOAT             0x197 
#define BRW_SURFACEFORMAT_R64G64B64_FLOAT                0x198 
#define BRW_SURFACEFORMAT_BC4_SNORM                      0x199 
#define BRW_SURFACEFORMAT_BC5_SNORM                      0x19A 
#define BRW_SURFACEFORMAT_R16G16B16_UNORM                0x19C 
#define BRW_SURFACEFORMAT_R16G16B16_SNORM                0x19D 
#define BRW_SURFACEFORMAT_R16G16B16_SSCALED              0x19E 
#define BRW_SURFACEFORMAT_R16G16B16_USCALED              0x19F

#define BRW_SURFACERETURNFORMAT_FLOAT32  0
#define BRW_SURFACERETURNFORMAT_S1       1

#define BRW_SURFACE_1D      0
#define BRW_SURFACE_2D      1
#define BRW_SURFACE_3D      2
#define BRW_SURFACE_CUBE    3
#define BRW_SURFACE_BUFFER  4
#define BRW_SURFACE_NULL    7

#define BRW_TEXCOORDMODE_WRAP            0
#define BRW_TEXCOORDMODE_MIRROR          1
#define BRW_TEXCOORDMODE_CLAMP           2
#define BRW_TEXCOORDMODE_CUBE            3
#define BRW_TEXCOORDMODE_CLAMP_BORDER    4
#define BRW_TEXCOORDMODE_MIRROR_ONCE     5

#define BRW_THREAD_PRIORITY_NORMAL   0
#define BRW_THREAD_PRIORITY_HIGH     1

#define BRW_TILEWALK_XMAJOR                 0
#define BRW_TILEWALK_YMAJOR                 1

#define BRW_VERTEX_SUBPIXEL_PRECISION_8BITS  0
#define BRW_VERTEX_SUBPIXEL_PRECISION_4BITS  1

#define BRW_VERTEXBUFFER_ACCESS_VERTEXDATA     0
#define BRW_VERTEXBUFFER_ACCESS_INSTANCEDATA   1

#define BRW_VFCOMPONENT_NOSTORE      0
#define BRW_VFCOMPONENT_STORE_SRC    1
#define BRW_VFCOMPONENT_STORE_0      2
#define BRW_VFCOMPONENT_STORE_1_FLT  3
#define BRW_VFCOMPONENT_STORE_1_INT  4
#define BRW_VFCOMPONENT_STORE_VID    5
#define BRW_VFCOMPONENT_STORE_IID    6
#define BRW_VFCOMPONENT_STORE_PID    7



/* Execution Unit (EU) defines
 */

#define BRW_ALIGN_1   0
#define BRW_ALIGN_16  1

#define BRW_ADDRESS_DIRECT                        0
#define BRW_ADDRESS_REGISTER_INDIRECT_REGISTER    1

#define BRW_CHANNEL_X     0
#define BRW_CHANNEL_Y     1
#define BRW_CHANNEL_Z     2
#define BRW_CHANNEL_W     3

#define BRW_COMPRESSION_NONE          0
#define BRW_COMPRESSION_2NDHALF       1
#define BRW_COMPRESSION_COMPRESSED    2

#define BRW_CONDITIONAL_NONE  0
#define BRW_CONDITIONAL_Z     1
#define BRW_CONDITIONAL_NZ    2
#define BRW_CONDITIONAL_EQ    1	/* Z */
#define BRW_CONDITIONAL_NEQ   2	/* NZ */
#define BRW_CONDITIONAL_G     3
#define BRW_CONDITIONAL_GE    4
#define BRW_CONDITIONAL_L     5
#define BRW_CONDITIONAL_LE    6
#define BRW_CONDITIONAL_C     7
#define BRW_CONDITIONAL_O     8

#define BRW_DEBUG_NONE        0
#define BRW_DEBUG_BREAKPOINT  1

#define BRW_DEPENDENCY_NORMAL         0
#define BRW_DEPENDENCY_NOTCLEARED     1
#define BRW_DEPENDENCY_NOTCHECKED     2
#define BRW_DEPENDENCY_DISABLE        3

#define BRW_EXECUTE_1     0
#define BRW_EXECUTE_2     1
#define BRW_EXECUTE_4     2
#define BRW_EXECUTE_8     3
#define BRW_EXECUTE_16    4
#define BRW_EXECUTE_32    5

#define BRW_HORIZONTAL_STRIDE_0   0
#define BRW_HORIZONTAL_STRIDE_1   1
#define BRW_HORIZONTAL_STRIDE_2   2
#define BRW_HORIZONTAL_STRIDE_4   3

#define BRW_INSTRUCTION_NORMAL    0
#define BRW_INSTRUCTION_SATURATE  1

#define BRW_MASK_ENABLE   0
#define BRW_MASK_DISABLE  1

#define BRW_OPCODE_MOV        1
#define BRW_OPCODE_SEL        2
#define BRW_OPCODE_NOT        4
#define BRW_OPCODE_AND        5
#define BRW_OPCODE_OR         6
#define BRW_OPCODE_XOR        7
#define BRW_OPCODE_SHR        8
#define BRW_OPCODE_SHL        9
#define BRW_OPCODE_RSR        10
#define BRW_OPCODE_RSL        11
#define BRW_OPCODE_ASR        12
#define BRW_OPCODE_CMP        16
#define BRW_OPCODE_JMPI       32
#define BRW_OPCODE_IF         34
#define BRW_OPCODE_IFF        35
#define BRW_OPCODE_ELSE       36
#define BRW_OPCODE_ENDIF      37
#define BRW_OPCODE_DO         38
#define BRW_OPCODE_WHILE      39
#define BRW_OPCODE_BREAK      40
#define BRW_OPCODE_CONTINUE   41
#define BRW_OPCODE_HALT       42
#define BRW_OPCODE_MSAVE      44
#define BRW_OPCODE_MRESTORE   45
#define BRW_OPCODE_PUSH       46
#define BRW_OPCODE_POP        47
#define BRW_OPCODE_WAIT       48
#define BRW_OPCODE_SEND       49
#define BRW_OPCODE_ADD        64
#define BRW_OPCODE_MUL        65
#define BRW_OPCODE_AVG        66
#define BRW_OPCODE_FRC        67
#define BRW_OPCODE_RNDU       68
#define BRW_OPCODE_RNDD       69
#define BRW_OPCODE_RNDE       70
#define BRW_OPCODE_RNDZ       71
#define BRW_OPCODE_MAC        72
#define BRW_OPCODE_MACH       73
#define BRW_OPCODE_LZD        74
#define BRW_OPCODE_SAD2       80
#define BRW_OPCODE_SADA2      81
#define BRW_OPCODE_DP4        84
#define BRW_OPCODE_DPH        85
#define BRW_OPCODE_DP3        86
#define BRW_OPCODE_DP2        87
#define BRW_OPCODE_DPA2       88
#define BRW_OPCODE_LINE       89
#define BRW_OPCODE_NOP        126

#define BRW_PREDICATE_NONE             0
#define BRW_PREDICATE_NORMAL           1
#define BRW_PREDICATE_ALIGN1_ANYV             2
#define BRW_PREDICATE_ALIGN1_ALLV             3
#define BRW_PREDICATE_ALIGN1_ANY2H            4
#define BRW_PREDICATE_ALIGN1_ALL2H            5
#define BRW_PREDICATE_ALIGN1_ANY4H            6
#define BRW_PREDICATE_ALIGN1_ALL4H            7
#define BRW_PREDICATE_ALIGN1_ANY8H            8
#define BRW_PREDICATE_ALIGN1_ALL8H            9
#define BRW_PREDICATE_ALIGN1_ANY16H           10
#define BRW_PREDICATE_ALIGN1_ALL16H           11
#define BRW_PREDICATE_ALIGN16_REPLICATE_X     2
#define BRW_PREDICATE_ALIGN16_REPLICATE_Y     3
#define BRW_PREDICATE_ALIGN16_REPLICATE_Z     4
#define BRW_PREDICATE_ALIGN16_REPLICATE_W     5
#define BRW_PREDICATE_ALIGN16_ANY4H           6
#define BRW_PREDICATE_ALIGN16_ALL4H           7

#define BRW_ARCHITECTURE_REGISTER_FILE    0
#define BRW_GENERAL_REGISTER_FILE         1
#define BRW_MESSAGE_REGISTER_FILE         2
#define BRW_IMMEDIATE_VALUE               3

#define BRW_REGISTER_TYPE_UD  0
#define BRW_REGISTER_TYPE_D   1
#define BRW_REGISTER_TYPE_UW  2
#define BRW_REGISTER_TYPE_W   3
#define BRW_REGISTER_TYPE_UB  4
#define BRW_REGISTER_TYPE_B   5
#define BRW_REGISTER_TYPE_VF  5	/* packed float vector, immediates only? */
#define BRW_REGISTER_TYPE_HF  6
#define BRW_REGISTER_TYPE_V   6	/* packed int vector, immediates only, uword dest only */
#define BRW_REGISTER_TYPE_F   7

#define BRW_ARF_NULL                  0x00
#define BRW_ARF_ADDRESS               0x10
#define BRW_ARF_ACCUMULATOR           0x20   
#define BRW_ARF_FLAG                  0x30
#define BRW_ARF_MASK                  0x40
#define BRW_ARF_MASK_STACK            0x50
#define BRW_ARF_MASK_STACK_DEPTH      0x60
#define BRW_ARF_STATE                 0x70
#define BRW_ARF_CONTROL               0x80
#define BRW_ARF_NOTIFICATION_COUNT    0x90
#define BRW_ARF_IP                    0xA0

#define BRW_AMASK   0
#define BRW_IMASK   1
#define BRW_LMASK   2
#define BRW_CMASK   3



#define BRW_THREAD_NORMAL     0
#define BRW_THREAD_ATOMIC     1
#define BRW_THREAD_SWITCH     2

#define BRW_VERTICAL_STRIDE_0                 0
#define BRW_VERTICAL_STRIDE_1                 1
#define BRW_VERTICAL_STRIDE_2                 2
#define BRW_VERTICAL_STRIDE_4                 3
#define BRW_VERTICAL_STRIDE_8                 4
#define BRW_VERTICAL_STRIDE_16                5
#define BRW_VERTICAL_STRIDE_32                6
#define BRW_VERTICAL_STRIDE_64                7
#define BRW_VERTICAL_STRIDE_128               8
#define BRW_VERTICAL_STRIDE_256               9
#define BRW_VERTICAL_STRIDE_ONE_DIMENSIONAL   0xF

#define BRW_WIDTH_1       0
#define BRW_WIDTH_2       1
#define BRW_WIDTH_4       2
#define BRW_WIDTH_8       3
#define BRW_WIDTH_16      4

#define BRW_STATELESS_BUFFER_BOUNDARY_1K      0
#define BRW_STATELESS_BUFFER_BOUNDARY_2K      1
#define BRW_STATELESS_BUFFER_BOUNDARY_4K      2
#define BRW_STATELESS_BUFFER_BOUNDARY_8K      3
#define BRW_STATELESS_BUFFER_BOUNDARY_16K     4
#define BRW_STATELESS_BUFFER_BOUNDARY_32K     5
#define BRW_STATELESS_BUFFER_BOUNDARY_64K     6
#define BRW_STATELESS_BUFFER_BOUNDARY_128K    7
#define BRW_STATELESS_BUFFER_BOUNDARY_256K    8
#define BRW_STATELESS_BUFFER_BOUNDARY_512K    9
#define BRW_STATELESS_BUFFER_BOUNDARY_1M      10
#define BRW_STATELESS_BUFFER_BOUNDARY_2M      11

#define BRW_POLYGON_FACING_FRONT      0
#define BRW_POLYGON_FACING_BACK       1

#define BRW_MESSAGE_TARGET_NULL               0
#define BRW_MESSAGE_TARGET_MATH               1
#define BRW_MESSAGE_TARGET_SAMPLER            2
#define BRW_MESSAGE_TARGET_GATEWAY            3
#define BRW_MESSAGE_TARGET_DATAPORT_READ      4
#define BRW_MESSAGE_TARGET_DATAPORT_WRITE     5
#define BRW_MESSAGE_TARGET_URB                6
#define BRW_MESSAGE_TARGET_THREAD_SPAWNER     7

#define BRW_SAMPLER_RETURN_FORMAT_FLOAT32     0
#define BRW_SAMPLER_RETURN_FORMAT_UINT32      2
#define BRW_SAMPLER_RETURN_FORMAT_SINT32      3

#define BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE              0
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE             0
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS        0
#define BRW_SAMPLER_MESSAGE_SIMD8_KILLPIX             1
#define BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_LOD        1
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_LOD         1
#define BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_GRADIENTS  2
#define BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_GRADIENTS    2
#define BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_COMPARE    0
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_COMPARE     2
#define BRW_SAMPLER_MESSAGE_SIMD4X2_RESINFO           2
#define BRW_SAMPLER_MESSAGE_SIMD8_RESINFO             2
#define BRW_SAMPLER_MESSAGE_SIMD16_RESINFO            2
#define BRW_SAMPLER_MESSAGE_SIMD4X2_LD                3
#define BRW_SAMPLER_MESSAGE_SIMD8_LD                  3
#define BRW_SAMPLER_MESSAGE_SIMD16_LD                 3

#define BRW_DATAPORT_OWORD_BLOCK_1_OWORDLOW   0
#define BRW_DATAPORT_OWORD_BLOCK_1_OWORDHIGH  1
#define BRW_DATAPORT_OWORD_BLOCK_2_OWORDS     2
#define BRW_DATAPORT_OWORD_BLOCK_4_OWORDS     3
#define BRW_DATAPORT_OWORD_BLOCK_8_OWORDS     4

#define BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD     0
#define BRW_DATAPORT_OWORD_DUAL_BLOCK_4OWORDS    2

#define BRW_DATAPORT_DWORD_SCATTERED_BLOCK_8DWORDS   2
#define BRW_DATAPORT_DWORD_SCATTERED_BLOCK_16DWORDS  3

#define BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ          0
#define BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ     1
#define BRW_DATAPORT_READ_MESSAGE_DWORD_BLOCK_READ          2
#define BRW_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ      3

#define BRW_DATAPORT_READ_TARGET_DATA_CACHE      0
#define BRW_DATAPORT_READ_TARGET_RENDER_CACHE    1
#define BRW_DATAPORT_READ_TARGET_SAMPLER_CACHE   2

#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE                0
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE_REPLICATED     1
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_DUAL_SOURCE_SUBSPAN01         2
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_DUAL_SOURCE_SUBSPAN23         3
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_SINGLE_SOURCE_SUBSPAN01       4

#define BRW_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE                0
#define BRW_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE           1
#define BRW_DATAPORT_WRITE_MESSAGE_DWORD_BLOCK_WRITE                2
#define BRW_DATAPORT_WRITE_MESSAGE_DWORD_SCATTERED_WRITE            3
#define BRW_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE              4
#define BRW_DATAPORT_WRITE_MESSAGE_STREAMED_VERTEX_BUFFER_WRITE     5
#define BRW_DATAPORT_WRITE_MESSAGE_FLUSH_RENDER_CACHE               7

#define BRW_MATH_FUNCTION_INV                              1
#define BRW_MATH_FUNCTION_LOG                              2
#define BRW_MATH_FUNCTION_EXP                              3
#define BRW_MATH_FUNCTION_SQRT                             4
#define BRW_MATH_FUNCTION_RSQ                              5
#define BRW_MATH_FUNCTION_SIN                              6 /* was 7 */
#define BRW_MATH_FUNCTION_COS                              7 /* was 8 */
#define BRW_MATH_FUNCTION_SINCOS                           8 /* was 6 */
#define BRW_MATH_FUNCTION_TAN                              9
#define BRW_MATH_FUNCTION_POW                              10
#define BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER   11
#define BRW_MATH_FUNCTION_INT_DIV_QUOTIENT                 12
#define BRW_MATH_FUNCTION_INT_DIV_REMAINDER                13

#define BRW_MATH_INTEGER_UNSIGNED     0
#define BRW_MATH_INTEGER_SIGNED       1

#define BRW_MATH_PRECISION_FULL        0
#define BRW_MATH_PRECISION_PARTIAL     1

#define BRW_MATH_SATURATE_NONE         0
#define BRW_MATH_SATURATE_SATURATE     1

#define BRW_MATH_DATA_VECTOR  0
#define BRW_MATH_DATA_SCALAR  1

#define BRW_URB_OPCODE_WRITE  0

#define BRW_URB_SWIZZLE_NONE          0
#define BRW_URB_SWIZZLE_INTERLEAVE    1
#define BRW_URB_SWIZZLE_TRANSPOSE     2

#define BRW_SCRATCH_SPACE_SIZE_1K     0
#define BRW_SCRATCH_SPACE_SIZE_2K     1
#define BRW_SCRATCH_SPACE_SIZE_4K     2
#define BRW_SCRATCH_SPACE_SIZE_8K     3
#define BRW_SCRATCH_SPACE_SIZE_16K    4
#define BRW_SCRATCH_SPACE_SIZE_32K    5
#define BRW_SCRATCH_SPACE_SIZE_64K    6
#define BRW_SCRATCH_SPACE_SIZE_128K   7
#define BRW_SCRATCH_SPACE_SIZE_256K   8
#define BRW_SCRATCH_SPACE_SIZE_512K   9
#define BRW_SCRATCH_SPACE_SIZE_1M     10
#define BRW_SCRATCH_SPACE_SIZE_2M     11




#define CMD_URB_FENCE                 0x6000
#define CMD_CONST_BUFFER_STATE        0x6001
#define CMD_CONST_BUFFER              0x6002

#define CMD_STATE_BASE_ADDRESS        0x6101
#define CMD_STATE_INSN_POINTER        0x6102
#define CMD_PIPELINE_SELECT           0x6104

#define CMD_PIPELINED_STATE_POINTERS  0x7800
#define CMD_BINDING_TABLE_PTRS        0x7801
#define CMD_VERTEX_BUFFER             0x7808
#define CMD_VERTEX_ELEMENT            0x7809
#define CMD_INDEX_BUFFER              0x780a
#define CMD_VF_STATISTICS             0x780b

#define CMD_DRAW_RECT                 0x7900
#define CMD_BLEND_CONSTANT_COLOR      0x7901
#define CMD_CHROMA_KEY                0x7904
#define CMD_DEPTH_BUFFER              0x7905
#define CMD_POLY_STIPPLE_OFFSET       0x7906
#define CMD_POLY_STIPPLE_PATTERN      0x7907
#define CMD_LINE_STIPPLE_PATTERN      0x7908
#define CMD_GLOBAL_DEPTH_OFFSET_CLAMP 0x7909

#define CMD_PIPE_CONTROL              0x7a00

#define CMD_3D_PRIM                   0x7b00

#define CMD_MI_FLUSH                  0x0200


/* Various values from the R0 vertex header:
 */
#define R02_PRIM_END    0x1
#define R02_PRIM_START  0x2



#endif
