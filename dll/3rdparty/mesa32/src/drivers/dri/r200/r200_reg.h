/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_reg.h,v 1.2 2002/12/16 16:18:54 dawes Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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
*/

#ifndef _R200_REG_H_
#define _R200_REG_H_

#define R200_PP_MISC                      0x1c14 
#define     R200_REF_ALPHA_MASK        0x000000ff
#define     R200_ALPHA_TEST_FAIL       (0 << 8)
#define     R200_ALPHA_TEST_LESS       (1 << 8)
#define     R200_ALPHA_TEST_LEQUAL     (2 << 8)
#define     R200_ALPHA_TEST_EQUAL      (3 << 8)
#define     R200_ALPHA_TEST_GEQUAL     (4 << 8)
#define     R200_ALPHA_TEST_GREATER    (5 << 8)
#define     R200_ALPHA_TEST_NEQUAL     (6 << 8)
#define     R200_ALPHA_TEST_PASS       (7 << 8)
#define     R200_ALPHA_TEST_OP_MASK    (7 << 8)
#define     R200_CHROMA_FUNC_FAIL      (0 << 16)
#define     R200_CHROMA_FUNC_PASS      (1 << 16)
#define     R200_CHROMA_FUNC_NEQUAL    (2 << 16)
#define     R200_CHROMA_FUNC_EQUAL     (3 << 16)
#define     R200_CHROMA_KEY_NEAREST    (0 << 18)
#define     R200_CHROMA_KEY_ZERO       (1 << 18)
#define     R200_RIGHT_HAND_CUBE_D3D   (0 << 24)
#define     R200_RIGHT_HAND_CUBE_OGL   (1 << 24)
#define R200_PP_FOG_COLOR                 0x1c18 
#define     R200_FOG_COLOR_MASK        0x00ffffff
#define     R200_FOG_VERTEX            (0 << 24)
#define     R200_FOG_TABLE             (1 << 24)
#define     R200_FOG_USE_DEPTH         (0 << 25)
#define     R200_FOG_USE_W             (1 << 25)
#define     R200_FOG_USE_DIFFUSE_ALPHA (2 << 25)
#define     R200_FOG_USE_SPEC_ALPHA    (3 << 25)
#define     R200_FOG_USE_VTX_FOG       (4 << 25)
#define     R200_FOG_USE_MASK          (7 << 25)
#define R200_RE_SOLID_COLOR               0x1c1c 
#define R200_RB3D_BLENDCNTL               0x1c20
#define     R200_COMB_FCN_MASK                    (7  << 12)
#define     R200_COMB_FCN_ADD_CLAMP               (0  << 12)
#define     R200_COMB_FCN_ADD_NOCLAMP             (1  << 12)
#define     R200_COMB_FCN_SUB_CLAMP               (2  << 12)
#define     R200_COMB_FCN_SUB_NOCLAMP             (3  << 12)
#define     R200_COMB_FCN_MIN                     (4  << 12)
#define     R200_COMB_FCN_MAX                     (5  << 12)
#define     R200_COMB_FCN_RSUB_CLAMP              (6  << 12)
#define     R200_COMB_FCN_RSUB_NOCLAMP            (7  << 12)
#define       R200_BLEND_GL_ZERO                  (32)
#define       R200_BLEND_GL_ONE                   (33)
#define       R200_BLEND_GL_SRC_COLOR             (34)
#define       R200_BLEND_GL_ONE_MINUS_SRC_COLOR   (35)
#define       R200_BLEND_GL_DST_COLOR             (36)
#define       R200_BLEND_GL_ONE_MINUS_DST_COLOR   (37)
#define       R200_BLEND_GL_SRC_ALPHA             (38)
#define       R200_BLEND_GL_ONE_MINUS_SRC_ALPHA   (39)
#define       R200_BLEND_GL_DST_ALPHA             (40)
#define       R200_BLEND_GL_ONE_MINUS_DST_ALPHA   (41)
#define       R200_BLEND_GL_SRC_ALPHA_SATURATE    (42) /* src factor only */
#define       R200_BLEND_GL_CONST_COLOR           (43)
#define       R200_BLEND_GL_ONE_MINUS_CONST_COLOR (44)
#define       R200_BLEND_GL_CONST_ALPHA           (45)
#define       R200_BLEND_GL_ONE_MINUS_CONST_ALPHA (46)
#define       R200_BLEND_MASK                     (63)
#define     R200_SRC_BLEND_SHIFT                  (16)
#define     R200_DST_BLEND_SHIFT                  (24)
#define R200_RB3D_DEPTHOFFSET             0x1c24
#define R200_RB3D_DEPTHPITCH              0x1c28
#define     R200_DEPTHPITCH_MASK         0x00001ff8
#define     R200_DEPTH_HYPERZ            (3 << 16)
#define     R200_DEPTH_ENDIAN_NO_SWAP    (0 << 18)
#define     R200_DEPTH_ENDIAN_WORD_SWAP  (1 << 18)
#define     R200_DEPTH_ENDIAN_DWORD_SWAP (2 << 18)
#define R200_RB3D_ZSTENCILCNTL            0x1c2c 
#define     R200_DEPTH_FORMAT_MASK          (0xf << 0)
#define     R200_DEPTH_FORMAT_16BIT_INT_Z   (0  <<  0)
#define     R200_DEPTH_FORMAT_24BIT_INT_Z   (2  <<  0)
#define     R200_DEPTH_FORMAT_24BIT_FLOAT_Z (3  <<  0)
#define     R200_DEPTH_FORMAT_32BIT_INT_Z   (4  <<  0)
#define     R200_DEPTH_FORMAT_32BIT_FLOAT_Z (5  <<  0)
#define     R200_DEPTH_FORMAT_24BIT_FLOAT_W (9  <<  0)
#define     R200_DEPTH_FORMAT_32BIT_FLOAT_W (11 <<  0)
#define     R200_Z_TEST_NEVER               (0  <<  4)
#define     R200_Z_TEST_LESS                (1  <<  4)
#define     R200_Z_TEST_LEQUAL              (2  <<  4)
#define     R200_Z_TEST_EQUAL               (3  <<  4)
#define     R200_Z_TEST_GEQUAL              (4  <<  4)
#define     R200_Z_TEST_GREATER             (5  <<  4)
#define     R200_Z_TEST_NEQUAL              (6  <<  4)
#define     R200_Z_TEST_ALWAYS              (7  <<  4)
#define     R200_Z_TEST_MASK                (7  <<  4)
#define     R200_Z_HIERARCHY_ENABLE         (1  <<  8)
#define     R200_STENCIL_TEST_NEVER         (0  << 12)
#define     R200_STENCIL_TEST_LESS          (1  << 12)
#define     R200_STENCIL_TEST_LEQUAL        (2  << 12)
#define     R200_STENCIL_TEST_EQUAL         (3  << 12)
#define     R200_STENCIL_TEST_GEQUAL        (4  << 12)
#define     R200_STENCIL_TEST_GREATER       (5  << 12)
#define     R200_STENCIL_TEST_NEQUAL        (6  << 12)
#define     R200_STENCIL_TEST_ALWAYS        (7  << 12)
#define     R200_STENCIL_TEST_MASK          (0x7 << 12)
#define     R200_STENCIL_FAIL_KEEP          (0  << 16)
#define     R200_STENCIL_FAIL_ZERO          (1  << 16)
#define     R200_STENCIL_FAIL_REPLACE       (2  << 16)
#define     R200_STENCIL_FAIL_INC           (3  << 16)
#define     R200_STENCIL_FAIL_DEC           (4  << 16)
#define     R200_STENCIL_FAIL_INVERT        (5  << 16)
#define     R200_STENCIL_FAIL_INC_WRAP      (6  << 16)
#define     R200_STENCIL_FAIL_DEC_WRAP      (7  << 16)
#define     R200_STENCIL_FAIL_MASK          (0x7 << 16)
#define     R200_STENCIL_ZPASS_KEEP         (0  << 20)
#define     R200_STENCIL_ZPASS_ZERO         (1  << 20)
#define     R200_STENCIL_ZPASS_REPLACE      (2  << 20)
#define     R200_STENCIL_ZPASS_INC          (3  << 20)
#define     R200_STENCIL_ZPASS_DEC          (4  << 20)
#define     R200_STENCIL_ZPASS_INVERT       (5  << 20)
#define     R200_STENCIL_ZPASS_INC_WRAP     (6  << 20)
#define     R200_STENCIL_ZPASS_DEC_WRAP     (7  << 20)
#define     R200_STENCIL_ZPASS_MASK         (0x7 << 20)
#define     R200_STENCIL_ZFAIL_KEEP         (0  << 24)
#define     R200_STENCIL_ZFAIL_ZERO         (1  << 24)
#define     R200_STENCIL_ZFAIL_REPLACE      (2  << 24)
#define     R200_STENCIL_ZFAIL_INC          (3  << 24)
#define     R200_STENCIL_ZFAIL_DEC          (4  << 24)
#define     R200_STENCIL_ZFAIL_INVERT       (5  << 24)
#define     R200_STENCIL_ZFAIL_INC_WRAP     (6  << 24)
#define     R200_STENCIL_ZFAIL_DEC_WRAP     (7  << 24)
#define     R200_STENCIL_ZFAIL_MASK         (0x7 << 24)
#define     R200_Z_COMPRESSION_ENABLE       (1  << 28)
#define     R200_FORCE_Z_DIRTY              (1  << 29)
#define     R200_Z_WRITE_ENABLE             (1  << 30)
#define     R200_Z_DECOMPRESSION_ENABLE     (1  << 31)
/*gap*/
#define R200_PP_CNTL                      0x1c38 
#define     R200_TEX_0_ENABLE                         0x00000010
#define     R200_TEX_1_ENABLE                         0x00000020
#define     R200_TEX_2_ENABLE                         0x00000040
#define     R200_TEX_3_ENABLE                         0x00000080
#define     R200_TEX_4_ENABLE                         0x00000100
#define     R200_TEX_5_ENABLE                         0x00000200
#define     R200_TEX_ENABLE_MASK                      0x000003f0
#define     R200_FILTER_ROUND_MODE_MASK               0x00000400
#define     R200_TEX_BLEND_7_ENABLE                   0x00000800
#define     R200_TEX_BLEND_0_ENABLE                   0x00001000
#define     R200_TEX_BLEND_1_ENABLE                   0x00002000
#define     R200_TEX_BLEND_2_ENABLE                   0x00004000
#define     R200_TEX_BLEND_3_ENABLE                   0x00008000
#define     R200_TEX_BLEND_4_ENABLE                   0x00010000
#define     R200_TEX_BLEND_5_ENABLE                   0x00020000
#define     R200_TEX_BLEND_6_ENABLE                   0x00040000
#define     R200_TEX_BLEND_ENABLE_MASK                0x0007f800
#define     R200_TEX_BLEND_0_ENABLE_SHIFT             (12)
#define     R200_MULTI_PASS_ENABLE                    0x00080000
#define     R200_SPECULAR_ENABLE                      0x00200000
#define     R200_FOG_ENABLE                           0x00400000
#define     R200_ALPHA_TEST_ENABLE                    0x00800000
#define     R200_ANTI_ALIAS_NONE                       0x00000000
#define     R200_ANTI_ALIAS_LINE                       0x01000000
#define     R200_ANTI_ALIAS_POLY                       0x02000000
#define     R200_ANTI_ALIAS_MASK                       0x03000000
#define R200_RB3D_CNTL                    0x1c3c 
#define     R200_ALPHA_BLEND_ENABLE       (1  <<  0)
#define     R200_PLANE_MASK_ENABLE        (1  <<  1)
#define     R200_DITHER_ENABLE            (1  <<  2)
#define     R200_ROUND_ENABLE             (1  <<  3)
#define     R200_SCALE_DITHER_ENABLE      (1  <<  4)
#define     R200_DITHER_INIT              (1  <<  5)
#define     R200_ROP_ENABLE               (1  <<  6)
#define     R200_STENCIL_ENABLE           (1  <<  7)
#define     R200_Z_ENABLE                 (1  <<  8)
#define     R200_DEPTH_XZ_OFFEST_ENABLE   (1  <<  9)
#define     R200_COLOR_FORMAT_ARGB1555    (3  << 10)
#define     R200_COLOR_FORMAT_RGB565      (4  << 10)
#define     R200_COLOR_FORMAT_ARGB8888    (6  << 10)
#define     R200_COLOR_FORMAT_RGB332      (7  << 10)
#define     R200_COLOR_FORMAT_Y8          (8  << 10)
#define     R200_COLOR_FORMAT_RGB8        (9  << 10)
#define     R200_COLOR_FORMAT_YUV422_VYUY (11 << 10)
#define     R200_COLOR_FORMAT_YUV422_YVYU (12 << 10)
#define     R200_COLOR_FORMAT_aYUV444     (14 << 10)
#define     R200_COLOR_FORMAT_ARGB4444    (15 << 10)
#define     R200_CLRCMP_FLIP_ENABLE       (1  << 14)
#define     R200_SEPARATE_ALPHA_ENABLE    (1  << 16)
#define R200_RB3D_COLOROFFSET             0x1c40 
#define     R200_COLOROFFSET_MASK      0xfffffff0
#define R200_RE_WIDTH_HEIGHT              0x1c44 
#define     R200_RE_WIDTH_SHIFT        0
#define     R200_RE_HEIGHT_SHIFT       16
#define R200_RB3D_COLORPITCH              0x1c48 
#define     R200_COLORPITCH_MASK         0x000001ff8
#define     R200_COLOR_TILE_ENABLE       (1 << 16)
#define     R200_COLOR_MICROTILE_ENABLE  (1 << 17)
#define     R200_COLOR_ENDIAN_NO_SWAP    (0 << 18)
#define     R200_COLOR_ENDIAN_WORD_SWAP  (1 << 18)
#define     R200_COLOR_ENDIAN_DWORD_SWAP (2 << 18)
#define R200_SE_CNTL                      0x1c4c 
#define     R200_FFACE_CULL_CW          (0 <<  0)
#define     R200_FFACE_CULL_CCW         (1 <<  0)
#define     R200_FFACE_CULL_DIR_MASK    (1 <<  0)
#define     R200_BFACE_CULL             (0 <<  1)
#define     R200_BFACE_SOLID            (3 <<  1)
#define     R200_FFACE_CULL             (0 <<  3)
#define     R200_FFACE_SOLID            (3 <<  3)
#define     R200_FFACE_CULL_MASK        (3 <<  3)
#define     R200_FLAT_SHADE_VTX_0       (0 <<  6)
#define     R200_FLAT_SHADE_VTX_1       (1 <<  6)
#define     R200_FLAT_SHADE_VTX_2       (2 <<  6)
#define     R200_FLAT_SHADE_VTX_LAST    (3 <<  6)
#define     R200_DIFFUSE_SHADE_SOLID    (0 <<  8)
#define     R200_DIFFUSE_SHADE_FLAT     (1 <<  8)
#define     R200_DIFFUSE_SHADE_GOURAUD  (2 <<  8)
#define     R200_DIFFUSE_SHADE_MASK     (3 <<  8)
#define     R200_ALPHA_SHADE_SOLID      (0 << 10)
#define     R200_ALPHA_SHADE_FLAT       (1 << 10)
#define     R200_ALPHA_SHADE_GOURAUD    (2 << 10)
#define     R200_ALPHA_SHADE_MASK       (3 << 10)
#define     R200_SPECULAR_SHADE_SOLID   (0 << 12)
#define     R200_SPECULAR_SHADE_FLAT    (1 << 12)
#define     R200_SPECULAR_SHADE_GOURAUD (2 << 12)
#define     R200_SPECULAR_SHADE_MASK    (3 << 12)
#define     R200_FOG_SHADE_SOLID        (0 << 14)
#define     R200_FOG_SHADE_FLAT         (1 << 14)
#define     R200_FOG_SHADE_GOURAUD      (2 << 14)
#define     R200_FOG_SHADE_MASK         (3 << 14)
#define     R200_ZBIAS_ENABLE_POINT     (1 << 16)
#define     R200_ZBIAS_ENABLE_LINE      (1 << 17)
#define     R200_ZBIAS_ENABLE_TRI       (1 << 18)
#define     R200_WIDELINE_ENABLE        (1 << 20)
#define     R200_DISC_FOG_SHADE_SOLID   (0 << 24)
#define     R200_DISC_FOG_SHADE_FLAT    (1 << 24)
#define     R200_DISC_FOG_SHADE_GOURAUD (2 << 24)
#define     R200_DISC_FOG_SHADE_MASK    (3 << 24)
#define     R200_VTX_PIX_CENTER_D3D     (0 << 27)
#define     R200_VTX_PIX_CENTER_OGL     (1 << 27)
#define     R200_ROUND_MODE_TRUNC       (0 << 28)
#define     R200_ROUND_MODE_ROUND       (1 << 28)
#define     R200_ROUND_MODE_ROUND_EVEN  (2 << 28)
#define     R200_ROUND_MODE_ROUND_ODD   (3 << 28)
#define     R200_ROUND_PREC_16TH_PIX    (0 << 30)
#define     R200_ROUND_PREC_8TH_PIX     (1 << 30)
#define     R200_ROUND_PREC_4TH_PIX     (2 << 30)
#define     R200_ROUND_PREC_HALF_PIX    (3 << 30)
#define R200_RE_CNTL                      0x1c50 
#define     R200_STIPPLE_ENABLE                     0x1
#define     R200_SCISSOR_ENABLE                     0x2
#define     R200_PATTERN_ENABLE                     0x4
#define     R200_PERSPECTIVE_ENABLE                 0x8
#define     R200_POINT_SMOOTH                       0x20
#define     R200_VTX_STQ0_D3D                       0x00010000
#define     R200_VTX_STQ1_D3D                       0x00040000
#define     R200_VTX_STQ2_D3D                       0x00100000
#define     R200_VTX_STQ3_D3D                       0x00400000
#define     R200_VTX_STQ4_D3D                       0x01000000
#define     R200_VTX_STQ5_D3D                       0x04000000
/* gap */
#define R200_RE_STIPPLE_ADDR              0x1cc8
#define R200_RE_STIPPLE_DATA              0x1ccc
#define R200_RE_LINE_PATTERN              0x1cd0 
#define     R200_LINE_PATTERN_MASK             0x0000ffff
#define     R200_LINE_REPEAT_COUNT_SHIFT       16
#define     R200_LINE_PATTERN_START_SHIFT      24
#define     R200_LINE_PATTERN_LITTLE_BIT_ORDER (0 << 28)
#define     R200_LINE_PATTERN_BIG_BIT_ORDER    (1 << 28)
#define     R200_LINE_PATTERN_AUTO_RESET       (1 << 29)
#define R200_RE_LINE_STATE                0x1cd4 
#define     R200_LINE_CURRENT_PTR_SHIFT       0
#define     R200_LINE_CURRENT_COUNT_SHIFT     8
#define R200_RE_SCISSOR_TL_0              0x1cd8
#define R200_RE_SCISSOR_BR_0              0x1cdc
#define R200_RE_SCISSOR_TL_1              0x1ce0
#define R200_RE_SCISSOR_BR_1              0x1ce4
#define R200_RE_SCISSOR_TL_2              0x1ce8
#define R200_RE_SCISSOR_BR_2              0x1cec
/* gap */
#define R200_RB3D_DEPTHXY_OFFSET          0x1d60 
#define     R200_DEPTHX_SHIFT  0
#define     R200_DEPTHY_SHIFT  16
/* gap */
#define R200_RB3D_STENCILREFMASK          0x1d7c 
#define     R200_STENCIL_REF_SHIFT           0
#define     R200_STENCIL_REF_MASK            (0xff << 0)
#define     R200_STENCIL_MASK_SHIFT          16
#define     R200_STENCIL_VALUE_MASK          (0xff << 16)
#define     R200_STENCIL_WRITEMASK_SHIFT     24
#define     R200_STENCIL_WRITE_MASK          (0xff << 24)
#define R200_RB3D_ROPCNTL                 0x1d80 
#define     R200_ROP_MASK                    (15 << 8)
#define     R200_ROP_CLEAR                   (0  << 8)
#define     R200_ROP_NOR                     (1  << 8)
#define     R200_ROP_AND_INVERTED            (2  << 8)
#define     R200_ROP_COPY_INVERTED           (3  << 8)
#define     R200_ROP_AND_REVERSE             (4  << 8)
#define     R200_ROP_INVERT                  (5  << 8)
#define     R200_ROP_XOR                     (6  << 8)
#define     R200_ROP_NAND                    (7  << 8)
#define     R200_ROP_AND                     (8  << 8)
#define     R200_ROP_EQUIV                   (9  << 8)
#define     R200_ROP_NOOP                    (10 << 8)
#define     R200_ROP_OR_INVERTED             (11 << 8)
#define     R200_ROP_COPY                    (12 << 8)
#define     R200_ROP_OR_REVERSE              (13 << 8)
#define     R200_ROP_OR                      (14 << 8)
#define     R200_ROP_SET                     (15 << 8)
#define R200_RB3D_PLANEMASK               0x1d84 
/* gap */
#define R200_SE_VPORT_XSCALE              0x1d98 
#define R200_SE_VPORT_XOFFSET             0x1d9c 
#define R200_SE_VPORT_YSCALE              0x1da0 
#define R200_SE_VPORT_YOFFSET             0x1da4 
#define R200_SE_VPORT_ZSCALE              0x1da8 
#define R200_SE_VPORT_ZOFFSET             0x1dac 
#define R200_SE_ZBIAS_FACTOR              0x1db0 
#define R200_SE_ZBIAS_CONSTANT            0x1db4 
#define R200_SE_LINE_WIDTH                0x1db8 
#define	    R200_LINE_WIDTH_SHIFT                   0x00000000
#define	    R200_MINPOINTSIZE_SHIFT                 0x00000010
/* gap */
#define R200_SE_VAP_CNTL                           0x2080
#define     R200_VAP_TCL_ENABLE                       0x00000001
#define     R200_VAP_PROG_VTX_SHADER_ENABLE           0x00000004
#define     R200_VAP_SINGLE_BUF_STATE_ENABLE          0x00000010
#define     R200_VAP_FORCE_W_TO_ONE                   0x00010000
#define     R200_VAP_D3D_TEX_DEFAULT                  0x00020000
#define     R200_VAP_VF_MAX_VTX_NUM__SHIFT            18
#define     R200_VAP_DX_CLIP_SPACE_DEF                0x00400000
#define R200_SE_VF_CNTL                           0x2084
#define     R200_VF_PRIM_NONE                         0x00000000
#define     R200_VF_PRIM_POINTS                       0x00000001
#define     R200_VF_PRIM_LINES                        0x00000002
#define     R200_VF_PRIM_LINE_STRIP                   0x00000003
#define     R200_VF_PRIM_TRIANGLES                    0x00000004
#define     R200_VF_PRIM_TRIANGLE_FAN                 0x00000005
#define     R200_VF_PRIM_TRIANGLE_STRIP               0x00000006
#define     R200_VF_PRIM_RECT_LIST                    0x00000008
#define     R200_VF_PRIM_3VRT_POINTS                  0x00000009
#define     R200_VF_PRIM_3VRT_LINES                   0x0000000a
#define     R200_VF_PRIM_POINT_SPRITES                0x0000000b
#define     R200_VF_PRIM_LINE_LOOP                    0x0000000c
#define     R200_VF_PRIM_QUADS                        0x0000000d
#define     R200_VF_PRIM_QUAD_STRIP                   0x0000000e
#define     R200_VF_PRIM_POLYGON                      0x0000000f
#define     R200_VF_PRIM_MASK                         0x0000000f
#define     R200_VF_PRIM_WALK_IND                     0x00000010
#define     R200_VF_PRIM_WALK_LIST                    0x00000020
#define     R200_VF_PRIM_WALK_RING                    0x00000030
#define     R200_VF_PRIM_WALK_MASK                    0x00000030
#define     R200_VF_COLOR_ORDER_RGBA                  0x00000040
#define     R200_VF_TCL_OUTPUT_VTX_ENABLE             0x00000200
#define     R200_VF_INDEX_SZ_4                        0x00000800
#define     R200_VF_VERTEX_NUMBER_MASK                0xffff0000
#define     R200_VF_VERTEX_NUMBER_SHIFT               16
#define R200_SE_VTX_FMT_0                 0x2088
#define     R200_VTX_XY                     0 /* always have xy */
#define     R200_VTX_Z0                     (1<<0)
#define     R200_VTX_W0                     (1<<1)
#define     R200_VTX_WEIGHT_COUNT_SHIFT     (2)
#define     R200_VTX_PV_MATRIX_SEL          (1<<5)
#define     R200_VTX_N0                     (1<<6)
#define     R200_VTX_POINT_SIZE             (1<<7)
#define     R200_VTX_DISCRETE_FOG           (1<<8)
#define     R200_VTX_SHININESS_0            (1<<9)
#define     R200_VTX_SHININESS_1            (1<<10)
#define       R200_VTX_COLOR_NOT_PRESENT      0
#define       R200_VTX_PK_RGBA          1
#define       R200_VTX_FP_RGB           2
#define       R200_VTX_FP_RGBA          3
#define       R200_VTX_COLOR_MASK             3
#define     R200_VTX_COLOR_0_SHIFT          11
#define     R200_VTX_COLOR_1_SHIFT          13
#define     R200_VTX_COLOR_2_SHIFT          15
#define     R200_VTX_COLOR_3_SHIFT          17
#define     R200_VTX_COLOR_4_SHIFT          19
#define     R200_VTX_COLOR_5_SHIFT          21
#define     R200_VTX_COLOR_6_SHIFT          23
#define     R200_VTX_COLOR_7_SHIFT          25
#define     R200_VTX_XY1                    (1<<28)
#define     R200_VTX_Z1                     (1<<29)
#define     R200_VTX_W1                     (1<<30)
#define     R200_VTX_N1                     (1<<31)
#define R200_SE_VTX_FMT_1                 0x208c
#define     R200_VTX_TEX0_COMP_CNT_SHIFT        0
#define     R200_VTX_TEX1_COMP_CNT_SHIFT        3
#define     R200_VTX_TEX2_COMP_CNT_SHIFT        6
#define     R200_VTX_TEX3_COMP_CNT_SHIFT        9
#define     R200_VTX_TEX4_COMP_CNT_SHIFT        12
#define     R200_VTX_TEX5_COMP_CNT_SHIFT        15
#define R200_SE_TCL_OUTPUT_VTX_FMT_0      0x2090 
#define R200_SE_TCL_OUTPUT_VTX_FMT_1      0x2094 
/* gap */
#define R200_SE_VTE_CNTL                  0x20b0
#define     R200_VPORT_X_SCALE_ENA                0x00000001
#define     R200_VPORT_X_OFFSET_ENA               0x00000002
#define     R200_VPORT_Y_SCALE_ENA                0x00000004
#define     R200_VPORT_Y_OFFSET_ENA               0x00000008
#define     R200_VPORT_Z_SCALE_ENA                0x00000010
#define     R200_VPORT_Z_OFFSET_ENA               0x00000020
#define     R200_VTX_XY_FMT                       0x00000100
#define     R200_VTX_Z_FMT                        0x00000200
#define     R200_VTX_W0_FMT                       0x00000400
#define     R200_VTX_W0_NORMALIZE                 0x00000800
#define     R200_VTX_ST_DENORMALIZED              0x00001000
/* gap */
#define R200_SE_VTX_NUM_ARRAYS            0x20c0
#define R200_SE_VTX_AOS_ATTR01            0x20c4
#define R200_SE_VTX_AOS_ADDR0             0x20c8
#define R200_SE_VTX_AOS_ADDR1             0x20cc
#define R200_SE_VTX_AOS_ATTR23            0x20d0
#define R200_SE_VTX_AOS_ADDR2             0x20d4
#define R200_SE_VTX_AOS_ADDR3             0x20d8
#define R200_SE_VTX_AOS_ATTR45            0x20dc
#define R200_SE_VTX_AOS_ADDR4             0x20e0
#define R200_SE_VTX_AOS_ADDR5             0x20e4
#define R200_SE_VTX_AOS_ATTR67            0x20e8
#define R200_SE_VTX_AOS_ADDR6             0x20ec
#define R200_SE_VTX_AOS_ADDR7             0x20f0
#define R200_SE_VTX_AOS_ATTR89            0x20f4
#define R200_SE_VTX_AOS_ADDR8             0x20f8
#define R200_SE_VTX_AOS_ADDR9             0x20fc
#define R200_SE_VTX_AOS_ATTR1011          0x2100
#define R200_SE_VTX_AOS_ADDR10            0x2104
#define R200_SE_VTX_AOS_ADDR11            0x2108
#define R200_SE_VF_MAX_VTX_INDX           0x210c
#define R200_SE_VF_MIN_VTX_INDX           0x2110
/* gap */
#define R200_SE_VAP_CNTL_STATUS           0x2140
#define     R200_VC_NO_SWAP                  (0 << 0)
#define     R200_VC_16BIT_SWAP               (1 << 0)
#define     R200_VC_32BIT_SWAP               (2 << 0)
/* gap */
#define R200_SE_VTX_STATE_CNTL                     0x2180
#define     R200_VSC_COLOR_0_ASSEMBLY_CNTL_SHIFT    0x00000000
#define     R200_VSC_COLOR_1_ASSEMBLY_CNTL_SHIFT    0x00000002
#define     R200_VSC_COLOR_2_ASSEMBLY_CNTL_SHIFT    0x00000004
#define     R200_VSC_COLOR_3_ASSEMBLY_CNTL_SHIFT    0x00000006
#define     R200_VSC_COLOR_4_ASSEMBLY_CNTL_SHIFT    0x00000008
#define     R200_VSC_COLOR_5_ASSEMBLY_CNTL_SHIFT    0x0000000a
#define     R200_VSC_COLOR_6_ASSEMBLY_CNTL_SHIFT    0x0000000c
#define     R200_VSC_COLOR_7_ASSEMBLY_CNTL_SHIFT    0x0000000e
#define     R200_VSC_UPDATE_USER_COLOR_0_ENABLE    0x00010000
#define     R200_VSC_UPDATE_USER_COLOR_1_ENABLE    0x00020000
/* gap */
#define R200_SE_TCL_VECTOR_INDX_REG                0x2200
#define R200_SE_TCL_VECTOR_DATA_REG                0x2204
#define R200_SE_TCL_SCALAR_INDX_REG                0x2208
#define R200_SE_TCL_SCALAR_DATA_REG                0x220c
/* gap */
#define R200_SE_TCL_MATRIX_SEL_0                   0x2230
#define     R200_MODELVIEW_0_SHIFT           (0) 
#define     R200_MODELVIEW_1_SHIFT           (8) 
#define     R200_MODELVIEW_2_SHIFT           (16) 
#define     R200_MODELVIEW_3_SHIFT           (24) 
#define R200_SE_TCL_MATRIX_SEL_1                   0x2234
#define     R200_IT_MODELVIEW_0_SHIFT        (0)
#define     R200_IT_MODELVIEW_1_SHIFT        (8) 
#define     R200_IT_MODELVIEW_2_SHIFT        (16)
#define     R200_IT_MODELVIEW_3_SHIFT        (24)
#define R200_SE_TCL_MATRIX_SEL_2                   0x2238
#define     R200_MODELPROJECT_0_SHIFT         (0) 
#define     R200_MODELPROJECT_1_SHIFT         (8) 
#define     R200_MODELPROJECT_2_SHIFT         (16) 
#define     R200_MODELPROJECT_3_SHIFT         (24) 
#define R200_SE_TCL_MATRIX_SEL_3                   0x223c
#define     R200_TEXMAT_0_SHIFT    0
#define     R200_TEXMAT_1_SHIFT    8
#define     R200_TEXMAT_2_SHIFT    16
#define     R200_TEXMAT_3_SHIFT    24
#define R200_SE_TCL_MATRIX_SEL_4                   0x2240
#define     R200_TEXMAT_4_SHIFT    0
#define     R200_TEXMAT_5_SHIFT    8
/* gap */
#define R200_SE_TCL_OUTPUT_VTX_COMP_SEL     0x2250
#define     R200_OUTPUT_XYZW                    (1<<0)
#define     R200_OUTPUT_COLOR_0                 (1<<8)
#define     R200_OUTPUT_COLOR_1                 (1<<9)
#define     R200_OUTPUT_TEX_0                   (1<<16)
#define     R200_OUTPUT_TEX_1                   (1<<17)
#define     R200_OUTPUT_TEX_2                   (1<<18)
#define     R200_OUTPUT_TEX_3                   (1<<19)
#define     R200_OUTPUT_TEX_4                   (1<<20)
#define     R200_OUTPUT_TEX_5                   (1<<21)
#define     R200_OUTPUT_TEX_MASK                (0x3f<<16)
#define     R200_OUTPUT_DISCRETE_FOG            (1<<24)
#define     R200_OUTPUT_PT_SIZE                 (1<<25)
#define     R200_FORCE_INORDER_PROC             (1<<31)
#define R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0  0x2254
#define	    R200_VERTEX_POSITION_ADDR__SHIFT     0x00000000
#define R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_1  0x2258
#define	    R200_VTX_COLOR_0_ADDR__SHIFT         0x00000000
#define	    R200_VTX_COLOR_1_ADDR__SHIFT         0x00000008
#define R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_2  0x225c
#define	    R200_VTX_TEX_0_ADDR__SHIFT           0x00000000
#define	    R200_VTX_TEX_1_ADDR__SHIFT           0x00000008
#define	    R200_VTX_TEX_2_ADDR__SHIFT           0x00000010
#define	    R200_VTX_TEX_3_ADDR__SHIFT           0x00000018
#define R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_3  0x2260
#define	    R200_VTX_TEX_4_ADDR__SHIFT           0x00000000
#define	    R200_VTX_TEX_5_ADDR__SHIFT           0x00000008

/* gap */
#define R200_SE_TCL_LIGHT_MODEL_CTL_0       0x2268 
#define     R200_LIGHTING_ENABLE                (1<<0)
#define     R200_LIGHT_IN_MODELSPACE            (1<<1)
#define     R200_LOCAL_VIEWER                   (1<<2)
#define     R200_NORMALIZE_NORMALS              (1<<3)
#define     R200_RESCALE_NORMALS                (1<<4)
#define     R200_SPECULAR_LIGHTS                (1<<5)
#define     R200_DIFFUSE_SPECULAR_COMBINE       (1<<6)
#define     R200_LIGHT_ALPHA                    (1<<7)
#define     R200_LOCAL_LIGHT_VEC_GL             (1<<8)
#define     R200_LIGHT_NO_NORMAL_AMBIENT_ONLY   (1<<9)
#define     R200_LIGHT_TWOSIDE                  (1<<10)
#define     R200_FRONT_SHININESS_SOURCE_SHIFT       (0xb)
#define     R200_BACK_SHININESS_SOURCE_SHIFT        (0xd)
#define       R200_LM0_SOURCE_MATERIAL_0           (0)
#define       R200_LM0_SOURCE_MATERIAL_1           (1)
#define       R200_LM0_SOURCE_VERTEX_SHININESS_0   (2)
#define       R200_LM0_SOURCE_VERTEX_SHININESS_1   (3)
#define R200_SE_TCL_LIGHT_MODEL_CTL_1       0x226c 
#define       R200_LM1_SOURCE_LIGHT_PREMULT        (0)
#define       R200_LM1_SOURCE_MATERIAL_0           (1)
#define       R200_LM1_SOURCE_VERTEX_COLOR_0       (2)
#define       R200_LM1_SOURCE_VERTEX_COLOR_1       (3)
#define       R200_LM1_SOURCE_VERTEX_COLOR_2       (4)
#define       R200_LM1_SOURCE_VERTEX_COLOR_3       (5)
#define       R200_LM1_SOURCE_VERTEX_COLOR_4       (6)
#define       R200_LM1_SOURCE_VERTEX_COLOR_5       (7)
#define       R200_LM1_SOURCE_VERTEX_COLOR_6       (8)
#define       R200_LM1_SOURCE_VERTEX_COLOR_7       (9)
#define       R200_LM1_SOURCE_MATERIAL_1           (0xf)
#define     R200_FRONT_EMISSIVE_SOURCE_SHIFT        (0)
#define     R200_FRONT_AMBIENT_SOURCE_SHIFT         (4)
#define     R200_FRONT_DIFFUSE_SOURCE_SHIFT         (8)
#define     R200_FRONT_SPECULAR_SOURCE_SHIFT        (12)
#define     R200_BACK_EMISSIVE_SOURCE_SHIFT         (16)
#define     R200_BACK_AMBIENT_SOURCE_SHIFT          (20)
#define     R200_BACK_DIFFUSE_SOURCE_SHIFT          (24)
#define     R200_BACK_SPECULAR_SOURCE_SHIFT         (28)
#define R200_SE_TCL_PER_LIGHT_CTL_0       0x2270 
#define     R200_LIGHT_0_ENABLE                    (1<<0)
#define     R200_LIGHT_0_ENABLE_AMBIENT            (1<<1)
#define     R200_LIGHT_0_ENABLE_SPECULAR           (1<<2)
#define     R200_LIGHT_0_IS_LOCAL                  (1<<3)
#define     R200_LIGHT_0_IS_SPOT                   (1<<4)
#define     R200_LIGHT_0_DUAL_CONE                 (1<<5)
#define     R200_LIGHT_0_ENABLE_RANGE_ATTEN        (1<<6)
#define     R200_LIGHT_0_CONSTANT_RANGE_ATTEN      (1<<7)
#define     R200_LIGHT_1_ENABLE                    (1<<16)
#define     R200_LIGHT_1_ENABLE_AMBIENT            (1<<17)
#define     R200_LIGHT_1_ENABLE_SPECULAR           (1<<18)
#define     R200_LIGHT_1_IS_LOCAL                  (1<<19)
#define     R200_LIGHT_1_IS_SPOT                   (1<<20)
#define     R200_LIGHT_1_DUAL_CONE                 (1<<21)
#define     R200_LIGHT_1_ENABLE_RANGE_ATTEN        (1<<22)
#define     R200_LIGHT_1_CONSTANT_RANGE_ATTEN      (1<<23)
#define     R200_LIGHT_0_SHIFT                   (0)
#define     R200_LIGHT_1_SHIFT                   (16)
#define R200_SE_TCL_PER_LIGHT_CTL_1       0x2274 
#define     R200_LIGHT_2_SHIFT                   (0)
#define     R200_LIGHT_3_SHIFT                   (16)
#define R200_SE_TCL_PER_LIGHT_CTL_2       0x2278 
#define     R200_LIGHT_4_SHIFT                   (0)
#define     R200_LIGHT_5_SHIFT                   (16)
#define R200_SE_TCL_PER_LIGHT_CTL_3       0x227c 
#define     R200_LIGHT_6_SHIFT                   (0)
#define     R200_LIGHT_7_SHIFT                   (16)
/* gap */
#define R200_SE_TCL_TEX_PROC_CTL_2        0x22a8 
#define     R200_TEXGEN_COMP_MASK                (0xf)
#define     R200_TEXGEN_COMP_S                   (0x1)
#define     R200_TEXGEN_COMP_T                   (0x2)
#define     R200_TEXGEN_COMP_R                   (0x4)
#define     R200_TEXGEN_COMP_Q                   (0x8)
#define     R200_TEXGEN_0_COMP_MASK_SHIFT        (0)
#define     R200_TEXGEN_1_COMP_MASK_SHIFT        (4)
#define     R200_TEXGEN_2_COMP_MASK_SHIFT        (8)
#define     R200_TEXGEN_3_COMP_MASK_SHIFT        (12)
#define     R200_TEXGEN_4_COMP_MASK_SHIFT        (16)
#define     R200_TEXGEN_5_COMP_MASK_SHIFT        (20)
#define R200_SE_TCL_TEX_PROC_CTL_3        0x22ac 
#define     R200_TEXGEN_0_INPUT_TEX_SHIFT        (0)
#define     R200_TEXGEN_1_INPUT_TEX_SHIFT        (4)
#define     R200_TEXGEN_2_INPUT_TEX_SHIFT        (8)
#define     R200_TEXGEN_3_INPUT_TEX_SHIFT        (12)
#define     R200_TEXGEN_4_INPUT_TEX_SHIFT        (16)
#define     R200_TEXGEN_5_INPUT_TEX_SHIFT        (20)
#define R200_SE_TCL_TEX_PROC_CTL_0        0x22b0 
#define     R200_TEXGEN_TEXMAT_0_ENABLE         (1<<0)
#define     R200_TEXGEN_TEXMAT_1_ENABLE         (1<<1)
#define     R200_TEXGEN_TEXMAT_2_ENABLE         (1<<2)
#define     R200_TEXGEN_TEXMAT_3_ENABLE         (1<<3)
#define     R200_TEXGEN_TEXMAT_4_ENABLE         (1<<4)
#define     R200_TEXGEN_TEXMAT_5_ENABLE         (1<<5)
#define     R200_TEXMAT_0_ENABLE                (1<<8)
#define     R200_TEXMAT_1_ENABLE                (1<<9)
#define     R200_TEXMAT_2_ENABLE                (1<<10)
#define     R200_TEXMAT_3_ENABLE                (1<<11)
#define     R200_TEXMAT_4_ENABLE                (1<<12)
#define     R200_TEXMAT_5_ENABLE                (1<<13)
#define     R200_TEXGEN_FORCE_W_TO_ONE          (1<<16)
#define R200_SE_TCL_TEX_PROC_CTL_1        0x22b4 
#define       R200_TEXGEN_INPUT_MASK           (0xf)
#define       R200_TEXGEN_INPUT_TEXCOORD_0     (0)
#define       R200_TEXGEN_INPUT_TEXCOORD_1     (1)
#define       R200_TEXGEN_INPUT_TEXCOORD_2     (2)
#define       R200_TEXGEN_INPUT_TEXCOORD_3     (3)
#define       R200_TEXGEN_INPUT_TEXCOORD_4     (4)
#define       R200_TEXGEN_INPUT_TEXCOORD_5     (5)
#define       R200_TEXGEN_INPUT_OBJ            (8)
#define       R200_TEXGEN_INPUT_EYE            (9)
#define       R200_TEXGEN_INPUT_EYE_NORMAL     (0xa)
#define       R200_TEXGEN_INPUT_EYE_REFLECT    (0xb)
#define       R200_TEXGEN_INPUT_SPHERE         (0xd)
#define     R200_TEXGEN_0_INPUT_SHIFT        (0)
#define     R200_TEXGEN_1_INPUT_SHIFT        (4)
#define     R200_TEXGEN_2_INPUT_SHIFT        (8)
#define     R200_TEXGEN_3_INPUT_SHIFT        (12)
#define     R200_TEXGEN_4_INPUT_SHIFT        (16)
#define     R200_TEXGEN_5_INPUT_SHIFT        (20)
#define R200_SE_TC_TEX_CYL_WRAP_CTL       0x22b8
/* gap */
#define R200_SE_TCL_UCP_VERT_BLEND_CTL    0x22c0 
#define     R200_UCP_IN_CLIP_SPACE              (1<<0)
#define     R200_UCP_IN_MODEL_SPACE             (1<<1)
#define     R200_UCP_ENABLE_0                   (1<<2)
#define     R200_UCP_ENABLE_1                   (1<<3)
#define     R200_UCP_ENABLE_2                   (1<<4)
#define     R200_UCP_ENABLE_3                   (1<<5)
#define     R200_UCP_ENABLE_4                   (1<<6)
#define     R200_UCP_ENABLE_5                   (1<<7)
#define     R200_TCL_FOG_MASK                   (3<<8)
#define     R200_TCL_FOG_DISABLE                (0<<8)
#define     R200_TCL_FOG_EXP                    (1<<8)
#define     R200_TCL_FOG_EXP2                   (2<<8)
#define     R200_TCL_FOG_LINEAR                 (3<<8)
#define     R200_RNG_BASED_FOG                  (1<<10)
#define     R200_CLIP_DISABLE                   (1<<11)
#define     R200_CULL_FRONT_IS_CW               (0<<28)
#define     R200_CULL_FRONT_IS_CCW              (1<<28)
#define     R200_CULL_FRONT                     (1<<29)
#define     R200_CULL_BACK                      (1<<30)
#define R200_SE_TCL_POINT_SPRITE_CNTL     0x22c4
#define     R200_PS_MULT_PVATTENCONST           (0<<0)
#define     R200_PS_MULT_PVATTEN                (1<<0)
#define     R200_PS_MULT_ATTENCONST             (2<<0)
#define     R200_PS_MULT_PVCONST                (3<<0)
#define     R200_PS_MULT_CONST                  (4<<0)
#define     R200_PS_MULT_MASK                   (7<<0)
#define     R200_PS_LIN_ATT_ZERO                (1<<3)
#define     R200_PS_USE_MODEL_EYE_VEC           (1<<4)
#define     R200_PS_ATT_ALPHA                   (1<<5)
#define     R200_PS_UCP_MODE_MASK               (3<<6)
#define     R200_PS_GEN_TEX_0                   (1<<8)
#define     R200_PS_GEN_TEX_1                   (1<<9)
#define     R200_PS_GEN_TEX_2                   (1<<10)
#define     R200_PS_GEN_TEX_3                   (1<<11)
#define     R200_PS_GEN_TEX_4                   (1<<12)
#define     R200_PS_GEN_TEX_5                   (1<<13)
#define     R200_PS_GEN_TEX_0_SHIFT             (8)
#define     R200_PS_GEN_TEX_MASK                (0x3f<<8)
#define     R200_PS_SE_SEL_STATE                (1<<16)
/* gap */
/* taken from r300, see comments there */
#define R200_VAP_PVS_CNTL_1                 0x22d0
#       define R200_PVS_CNTL_1_PROGRAM_START_SHIFT   0
#       define R200_PVS_CNTL_1_POS_END_SHIFT         10
#       define R200_PVS_CNTL_1_PROGRAM_END_SHIFT     20
/* Addresses are relative the the vertex program parameters area. */
#define R200_VAP_PVS_CNTL_2                 0x22d4
#       define R200_PVS_CNTL_2_PARAM_OFFSET_SHIFT 0
#       define R200_PVS_CNTL_2_PARAM_COUNT_SHIFT  16
/* gap */

#define R200_SE_VTX_ST_POS_0_X_4                   0x2300
#define R200_SE_VTX_ST_POS_0_Y_4                   0x2304
#define R200_SE_VTX_ST_POS_0_Z_4                   0x2308
#define R200_SE_VTX_ST_POS_0_W_4                   0x230c
#define R200_SE_VTX_ST_NORM_0_X                    0x2310
#define R200_SE_VTX_ST_NORM_0_Y                    0x2314
#define R200_SE_VTX_ST_NORM_0_Z                    0x2318
#define R200_SE_VTX_ST_PVMS                        0x231c
#define R200_SE_VTX_ST_CLR_0_R                     0x2320
#define R200_SE_VTX_ST_CLR_0_G                     0x2324
#define R200_SE_VTX_ST_CLR_0_B                     0x2328
#define R200_SE_VTX_ST_CLR_0_A                     0x232c
#define R200_SE_VTX_ST_CLR_1_R                     0x2330
#define R200_SE_VTX_ST_CLR_1_G                     0x2334
#define R200_SE_VTX_ST_CLR_1_B                     0x2338
#define R200_SE_VTX_ST_CLR_1_A                     0x233c
#define R200_SE_VTX_ST_CLR_2_R                     0x2340
#define R200_SE_VTX_ST_CLR_2_G                     0x2344
#define R200_SE_VTX_ST_CLR_2_B                     0x2348
#define R200_SE_VTX_ST_CLR_2_A                     0x234c
#define R200_SE_VTX_ST_CLR_3_R                     0x2350
#define R200_SE_VTX_ST_CLR_3_G                     0x2354
#define R200_SE_VTX_ST_CLR_3_B                     0x2358
#define R200_SE_VTX_ST_CLR_3_A                     0x235c
#define R200_SE_VTX_ST_CLR_4_R                     0x2360
#define R200_SE_VTX_ST_CLR_4_G                     0x2364
#define R200_SE_VTX_ST_CLR_4_B                     0x2368
#define R200_SE_VTX_ST_CLR_4_A                     0x236c
#define R200_SE_VTX_ST_CLR_5_R                     0x2370
#define R200_SE_VTX_ST_CLR_5_G                     0x2374
#define R200_SE_VTX_ST_CLR_5_B                     0x2378
#define R200_SE_VTX_ST_CLR_5_A                     0x237c
#define R200_SE_VTX_ST_CLR_6_R                     0x2380
#define R200_SE_VTX_ST_CLR_6_G                     0x2384
#define R200_SE_VTX_ST_CLR_6_B                     0x2388
#define R200_SE_VTX_ST_CLR_6_A                     0x238c
#define R200_SE_VTX_ST_CLR_7_R                     0x2390
#define R200_SE_VTX_ST_CLR_7_G                     0x2394
#define R200_SE_VTX_ST_CLR_7_B                     0x2398
#define R200_SE_VTX_ST_CLR_7_A                     0x239c
#define R200_SE_VTX_ST_TEX_0_S                     0x23a0
#define R200_SE_VTX_ST_TEX_0_T                     0x23a4
#define R200_SE_VTX_ST_TEX_0_R                     0x23a8
#define R200_SE_VTX_ST_TEX_0_Q                     0x23ac
#define R200_SE_VTX_ST_TEX_1_S                     0x23b0
#define R200_SE_VTX_ST_TEX_1_T                     0x23b4
#define R200_SE_VTX_ST_TEX_1_R                     0x23b8
#define R200_SE_VTX_ST_TEX_1_Q                     0x23bc
#define R200_SE_VTX_ST_TEX_2_S                     0x23c0
#define R200_SE_VTX_ST_TEX_2_T                     0x23c4
#define R200_SE_VTX_ST_TEX_2_R                     0x23c8
#define R200_SE_VTX_ST_TEX_2_Q                     0x23cc
#define R200_SE_VTX_ST_TEX_3_S                     0x23d0
#define R200_SE_VTX_ST_TEX_3_T                     0x23d4
#define R200_SE_VTX_ST_TEX_3_R                     0x23d8
#define R200_SE_VTX_ST_TEX_3_Q                     0x23dc
#define R200_SE_VTX_ST_TEX_4_S                     0x23e0
#define R200_SE_VTX_ST_TEX_4_T                     0x23e4
#define R200_SE_VTX_ST_TEX_4_R                     0x23e8
#define R200_SE_VTX_ST_TEX_4_Q                     0x23ec
#define R200_SE_VTX_ST_TEX_5_S                     0x23f0
#define R200_SE_VTX_ST_TEX_5_T                     0x23f4
#define R200_SE_VTX_ST_TEX_5_R                     0x23f8
#define R200_SE_VTX_ST_TEX_5_Q                     0x23fc
#define R200_SE_VTX_ST_PNT_SPRT_SZ                 0x2400
#define R200_SE_VTX_ST_DISC_FOG                    0x2404
#define R200_SE_VTX_ST_SHININESS_0                 0x2408
#define R200_SE_VTX_ST_SHININESS_1                 0x240c
#define R200_SE_VTX_ST_BLND_WT_0                   0x2410
#define R200_SE_VTX_ST_BLND_WT_1                   0x2414
#define R200_SE_VTX_ST_BLND_WT_2                   0x2418
#define R200_SE_VTX_ST_BLND_WT_3                   0x241c
#define R200_SE_VTX_ST_POS_1_X                     0x2420
#define R200_SE_VTX_ST_POS_1_Y                     0x2424
#define R200_SE_VTX_ST_POS_1_Z                     0x2428
#define R200_SE_VTX_ST_POS_1_W                     0x242c
#define R200_SE_VTX_ST_NORM_1_X                    0x2430
#define R200_SE_VTX_ST_NORM_1_Y                    0x2434
#define R200_SE_VTX_ST_NORM_1_Z                    0x2438
#define R200_SE_VTX_ST_USR_CLR_0_R                 0x2440
#define R200_SE_VTX_ST_USR_CLR_0_G                 0x2444
#define R200_SE_VTX_ST_USR_CLR_0_B                 0x2448
#define R200_SE_VTX_ST_USR_CLR_0_A                 0x244c
#define R200_SE_VTX_ST_USR_CLR_1_R                 0x2450
#define R200_SE_VTX_ST_USR_CLR_1_G                 0x2454
#define R200_SE_VTX_ST_USR_CLR_1_B                 0x2458
#define R200_SE_VTX_ST_USR_CLR_1_A                 0x245c
#define R200_SE_VTX_ST_CLR_0_PKD                   0x2460
#define R200_SE_VTX_ST_CLR_1_PKD                   0x2464
#define R200_SE_VTX_ST_CLR_2_PKD                   0x2468
#define R200_SE_VTX_ST_CLR_3_PKD                   0x246c
#define R200_SE_VTX_ST_CLR_4_PKD                   0x2470
#define R200_SE_VTX_ST_CLR_5_PKD                   0x2474
#define R200_SE_VTX_ST_CLR_6_PKD                   0x2478
#define R200_SE_VTX_ST_CLR_7_PKD                   0x247c
#define R200_SE_VTX_ST_POS_0_X_2                   0x2480
#define R200_SE_VTX_ST_POS_0_Y_2                   0x2484
#define R200_SE_VTX_ST_PAR_CLR_LD                  0x2488
#define R200_SE_VTX_ST_USR_CLR_PKD                 0x248c
#define R200_SE_VTX_ST_POS_0_X_3                   0x2490
#define R200_SE_VTX_ST_POS_0_Y_3                   0x2494
#define R200_SE_VTX_ST_POS_0_Z_3                   0x2498
#define R200_SE_VTX_ST_END_OF_PKT                  0x249c
/* gap */
#define R200_RE_POINTSIZE                          0x2648
#define     R200_POINTSIZE_SHIFT                       0
#define     R200_MAXPOINTSIZE_SHIFT                    16
/* gap */
#define R200_RE_TOP_LEFT                  0x26c0 
#define     R200_RE_LEFT_SHIFT         0
#define     R200_RE_TOP_SHIFT          16
#define R200_RE_MISC                      0x26c4 
#define     R200_STIPPLE_COORD_MASK           0x1f
#define     R200_STIPPLE_X_OFFSET_SHIFT       0
#define     R200_STIPPLE_X_OFFSET_MASK        (0x1f << 0)
#define     R200_STIPPLE_Y_OFFSET_SHIFT       8
#define     R200_STIPPLE_Y_OFFSET_MASK        (0x1f << 8)
#define     R200_STIPPLE_LITTLE_BIT_ORDER     (0 << 16)
#define     R200_STIPPLE_BIG_BIT_ORDER        (1 << 16)
/* gap */
#define R200_RE_AUX_SCISSOR_CNTL                   0x26f0
#define     R200_EXCLUSIVE_SCISSOR_0      0x01000000
#define     R200_EXCLUSIVE_SCISSOR_1      0x02000000
#define     R200_EXCLUSIVE_SCISSOR_2      0x04000000
#define     R200_SCISSOR_ENABLE_0         0x10000000
#define     R200_SCISSOR_ENABLE_1         0x20000000
#define     R200_SCISSOR_ENABLE_2         0x40000000
/* gap */
#define R200_PP_TXFILTER_0                0x2c00 
#define     R200_MAG_FILTER_NEAREST                   (0  <<  0)
#define     R200_MAG_FILTER_LINEAR                    (1  <<  0)
#define     R200_MAG_FILTER_MASK                      (1  <<  0)
#define     R200_MIN_FILTER_NEAREST                   (0  <<  1)
#define     R200_MIN_FILTER_LINEAR                    (1  <<  1)
#define     R200_MIN_FILTER_NEAREST_MIP_NEAREST       (2  <<  1)
#define     R200_MIN_FILTER_NEAREST_MIP_LINEAR        (3  <<  1)
#define     R200_MIN_FILTER_LINEAR_MIP_NEAREST        (6  <<  1)
#define     R200_MIN_FILTER_LINEAR_MIP_LINEAR         (7  <<  1)
#define     R200_MIN_FILTER_ANISO_NEAREST             (8  <<  1)
#define     R200_MIN_FILTER_ANISO_LINEAR              (9  <<  1)
#define     R200_MIN_FILTER_ANISO_NEAREST_MIP_NEAREST (10 <<  1)
#define     R200_MIN_FILTER_ANISO_NEAREST_MIP_LINEAR  (11 <<  1)
#define     R200_MIN_FILTER_MASK                      (15 <<  1)
#define     R200_MAX_ANISO_1_TO_1                     (0  <<  5)
#define     R200_MAX_ANISO_2_TO_1                     (1  <<  5)
#define     R200_MAX_ANISO_4_TO_1                     (2  <<  5)
#define     R200_MAX_ANISO_8_TO_1                     (3  <<  5)
#define     R200_MAX_ANISO_16_TO_1                    (4  <<  5)
#define     R200_MAX_ANISO_MASK                       (7  <<  5)
#define     R200_MAX_MIP_LEVEL_MASK                   (0x0f << 16)
#define     R200_MAX_MIP_LEVEL_SHIFT                  16
#define     R200_YUV_TO_RGB                           (1  << 20)
#define     R200_YUV_TEMPERATURE_COOL                 (0  << 21)
#define     R200_YUV_TEMPERATURE_HOT                  (1  << 21)
#define     R200_YUV_TEMPERATURE_MASK                 (1  << 21)
#define     R200_WRAPEN_S                             (1  << 22)
#define     R200_CLAMP_S_WRAP                         (0  << 23)
#define     R200_CLAMP_S_MIRROR                       (1  << 23)
#define     R200_CLAMP_S_CLAMP_LAST                   (2  << 23)
#define     R200_CLAMP_S_MIRROR_CLAMP_LAST            (3  << 23)
#define     R200_CLAMP_S_CLAMP_BORDER                 (4  << 23)
#define     R200_CLAMP_S_MIRROR_CLAMP_BORDER          (5  << 23)
#define     R200_CLAMP_S_CLAMP_GL                     (6  << 23)
#define     R200_CLAMP_S_MIRROR_CLAMP_GL              (7  << 23)
#define     R200_CLAMP_S_MASK                         (7  << 23)
#define     R200_WRAPEN_T                             (1  << 26)
#define     R200_CLAMP_T_WRAP                         (0  << 27)
#define     R200_CLAMP_T_MIRROR                       (1  << 27)
#define     R200_CLAMP_T_CLAMP_LAST                   (2  << 27)
#define     R200_CLAMP_T_MIRROR_CLAMP_LAST            (3  << 27)
#define     R200_CLAMP_T_CLAMP_BORDER                 (4  << 27)
#define     R200_CLAMP_T_MIRROR_CLAMP_BORDER          (5  << 27)
#define     R200_CLAMP_T_CLAMP_GL                     (6  << 27)
#define     R200_CLAMP_T_MIRROR_CLAMP_GL              (7  << 27)
#define     R200_CLAMP_T_MASK                         (7  << 27)
#define     R200_KILL_LT_ZERO                         (1  << 30)
#define     R200_BORDER_MODE_OGL                      (0  << 31)
#define     R200_BORDER_MODE_D3D                      (1  << 31)
#define R200_PP_TXFORMAT_0                0x2c04
#define     R200_TXFORMAT_I8                 (0  <<  0)
#define     R200_TXFORMAT_AI88               (1  <<  0)
#define     R200_TXFORMAT_RGB332             (2  <<  0)
#define     R200_TXFORMAT_ARGB1555           (3  <<  0)
#define     R200_TXFORMAT_RGB565             (4  <<  0)
#define     R200_TXFORMAT_ARGB4444           (5  <<  0)
#define     R200_TXFORMAT_ARGB8888           (6  <<  0)
#define     R200_TXFORMAT_RGBA8888           (7  <<  0)
#define     R200_TXFORMAT_Y8                 (8  <<  0)
#define     R200_TXFORMAT_AVYU4444           (9  <<  0)
#define     R200_TXFORMAT_VYUY422            (10  <<  0)
#define     R200_TXFORMAT_YVYU422            (11  <<  0)
#define     R200_TXFORMAT_DXT1               (12  <<  0)
#define     R200_TXFORMAT_DXT23              (14  <<  0)
#define     R200_TXFORMAT_DXT45              (15  <<  0)
#define     R200_TXFORMAT_DVDU88             (18  <<  0)
#define     R200_TXFORMAT_LDVDU655           (19  <<  0)
#define     R200_TXFORMAT_LDVDU8888          (20  <<  0)
#define     R200_TXFORMAT_GR1616             (21  <<  0)
#define     R200_TXFORMAT_ABGR8888           (22  <<  0)
#define     R200_TXFORMAT_BGR111110          (23  <<  0)
#define     R200_TXFORMAT_FORMAT_MASK        (31 <<  0)
#define     R200_TXFORMAT_FORMAT_SHIFT       0
#define     R200_TXFORMAT_APPLE_YUV          (1  <<  5)
#define     R200_TXFORMAT_ALPHA_IN_MAP       (1  <<  6)
#define     R200_TXFORMAT_NON_POWER2         (1  <<  7)
#define     R200_TXFORMAT_WIDTH_MASK         (15 <<  8)
#define     R200_TXFORMAT_WIDTH_SHIFT        8
#define     R200_TXFORMAT_HEIGHT_MASK        (15 << 12)
#define     R200_TXFORMAT_HEIGHT_SHIFT       12
#define     R200_TXFORMAT_F5_WIDTH_MASK      (15 << 16)	/* cube face 5 */
#define     R200_TXFORMAT_F5_WIDTH_SHIFT     16
#define     R200_TXFORMAT_F5_HEIGHT_MASK     (15 << 20)
#define     R200_TXFORMAT_F5_HEIGHT_SHIFT    20
#define     R200_TXFORMAT_ST_ROUTE_STQ0      (0  << 24)
#define     R200_TXFORMAT_ST_ROUTE_STQ1      (1  << 24)
#define     R200_TXFORMAT_ST_ROUTE_STQ2      (2  << 24)
#define     R200_TXFORMAT_ST_ROUTE_STQ3      (3  << 24)
#define     R200_TXFORMAT_ST_ROUTE_STQ4      (4  << 24)
#define     R200_TXFORMAT_ST_ROUTE_STQ5      (5  << 24)
#define     R200_TXFORMAT_ST_ROUTE_MASK      (7  << 24)
#define     R200_TXFORMAT_ST_ROUTE_SHIFT     24
#define     R200_TXFORMAT_LOOKUP_DISABLE     (1  << 27)
#define     R200_TXFORMAT_ALPHA_MASK_ENABLE  (1  << 28)
#define     R200_TXFORMAT_CHROMA_KEY_ENABLE  (1  << 29)
#define     R200_TXFORMAT_CUBIC_MAP_ENABLE   (1  << 30)
#define R200_PP_TXFORMAT_X_0              0x2c08
#define     R200_DEPTH_LOG2_MASK                      (0xf << 0)
#define     R200_DEPTH_LOG2_SHIFT                     0
#define     R200_VOLUME_FILTER_SHIFT                  4
#define     R200_VOLUME_FILTER_MASK                   (1 << 4)
#define     R200_VOLUME_FILTER_NEAREST                (0 << 4)
#define     R200_VOLUME_FILTER_LINEAR                 (1 << 4)
#define     R200_WRAPEN_Q                             (1  << 8)
#define     R200_CLAMP_Q_WRAP                         (0  << 9)
#define     R200_CLAMP_Q_MIRROR                       (1  << 9)
#define     R200_CLAMP_Q_CLAMP_LAST                   (2  << 9)
#define     R200_CLAMP_Q_MIRROR_CLAMP_LAST            (3  << 9)
#define     R200_CLAMP_Q_CLAMP_BORDER                 (4  << 9)
#define     R200_CLAMP_Q_MIRROR_CLAMP_BORDER          (5  << 9)
#define     R200_CLAMP_Q_CLAMP_GL                     (6  << 9)
#define     R200_CLAMP_Q_MIRROR_CLAMP_GL              (7  << 9)
#define     R200_CLAMP_Q_MASK                         (7  << 9)
#define     R200_MIN_MIP_LEVEL_MASK                   (0xff << 12)
#define     R200_MIN_MIP_LEVEL_SHIFT                  12
#define     R200_TEXCOORD_NONPROJ                     (0  << 16)
#define     R200_TEXCOORD_CUBIC_ENV                   (1  << 16)
#define     R200_TEXCOORD_VOLUME                      (2  << 16)
#define     R200_TEXCOORD_PROJ                        (3  << 16)
#define     R200_TEXCOORD_DEPTH                       (4  << 16)
#define     R200_TEXCOORD_1D_PROJ                     (5  << 16)
#define     R200_TEXCOORD_1D                          (6  << 16)
#define     R200_TEXCOORD_ZERO                        (7  << 16)
#define     R200_TEXCOORD_MASK                        (7  << 16)
#define     R200_LOD_BIAS_MASK                        (0xfff80000)
#define     R200_LOD_BIAS_SHIFT                       19
#define R200_PP_TXSIZE_0                  0x2c0c /* NPOT only */
#define R200_PP_TXPITCH_0                 0x2c10 /* NPOT only */
#define R200_PP_BORDER_COLOR_0            0x2c14
#define R200_PP_CUBIC_FACES_0             0x2c18
#define     R200_FACE_WIDTH_1_SHIFT                   0
#define     R200_FACE_HEIGHT_1_SHIFT                  4
#define     R200_FACE_WIDTH_1_MASK                   (0xf << 0)
#define     R200_FACE_HEIGHT_1_MASK                  (0xf << 4)
#define     R200_FACE_WIDTH_2_SHIFT                   8
#define     R200_FACE_HEIGHT_2_SHIFT                 12
#define     R200_FACE_WIDTH_2_MASK                   (0xf << 8)
#define     R200_FACE_HEIGHT_2_MASK                  (0xf << 12)
#define     R200_FACE_WIDTH_3_SHIFT                  16
#define     R200_FACE_HEIGHT_3_SHIFT                 20
#define     R200_FACE_WIDTH_3_MASK                   (0xf << 16)
#define     R200_FACE_HEIGHT_3_MASK                  (0xf << 20)
#define     R200_FACE_WIDTH_4_SHIFT                  24
#define     R200_FACE_HEIGHT_4_SHIFT                 28
#define     R200_FACE_WIDTH_4_MASK                   (0xf << 24)
#define     R200_FACE_HEIGHT_4_MASK                  (0xf << 28)
#define R200_PP_TXMULTI_CTL_0                  0x2c1c /* name from ddx, rest RE... */
#define     R200_PASS1_TXFORMAT_LOOKUP_DISABLE (1 << 0)
#define     R200_PASS1_TEXCOORD_NONPROJ        (0 << 1)
#define     R200_PASS1_TEXCOORD_CUBIC_ENV      (1 << 1)
#define     R200_PASS1_TEXCOORD_VOLUME         (2 << 1)
#define     R200_PASS1_TEXCOORD_PROJ           (3 << 1)
#define     R200_PASS1_TEXCOORD_DEPTH          (4 << 1)
#define     R200_PASS1_TEXCOORD_1D_PROJ        (5 << 1)
#define     R200_PASS1_TEXCOORD_1D             (6 << 1) /* pass1 texcoords only */
#define     R200_PASS1_TEXCOORD_ZERO           (7 << 1) /* verifed for 2d targets! */
#define     R200_PASS1_TEXCOORD_MASK           (7 << 1) /* assumed same values as for pass2 */
#define     R200_PASS1_ST_ROUTE_STQ0           (0 << 4)
#define     R200_PASS1_ST_ROUTE_STQ1           (1 << 4)
#define     R200_PASS1_ST_ROUTE_STQ2           (2 << 4)
#define     R200_PASS1_ST_ROUTE_STQ3           (3 << 4)
#define     R200_PASS1_ST_ROUTE_STQ4           (4 << 4)
#define     R200_PASS1_ST_ROUTE_STQ5           (5 << 4)
#define     R200_PASS1_ST_ROUTE_MASK           (7 << 4)
#define     R200_PASS1_ST_ROUTE_SHIFT          (4)
#define     R200_PASS2_COORDS_REG_0            (2 << 24)
#define     R200_PASS2_COORDS_REG_1            (3 << 24)
#define     R200_PASS2_COORDS_REG_2            (4 << 24)
#define     R200_PASS2_COORDS_REG_3            (5 << 24)
#define     R200_PASS2_COORDS_REG_4            (6 << 24)
#define     R200_PASS2_COORDS_REG_5            (7 << 24)
#define     R200_PASS2_COORDS_REG_MASK         (0x7 << 24)
#define     R200_PASS2_COORDS_REG_SHIFT        (24)
#define R200_PP_TXFILTER_1                0x2c20
#define R200_PP_TXFORMAT_1                0x2c24
#define R200_PP_TXFORMAT_X_1              0x2c28
#define R200_PP_TXSIZE_1                  0x2c2c
#define R200_PP_TXPITCH_1                 0x2c30
#define R200_PP_BORDER_COLOR_1            0x2c34
#define R200_PP_CUBIC_FACES_1             0x2c38
#define R200_PP_TXMULTI_CTL_1             0x2c3c
#define R200_PP_TXFILTER_2                0x2c40
#define R200_PP_TXFORMAT_2                0x2c44
#define R200_PP_TXSIZE_2                  0x2c4c
#define R200_PP_TXFORMAT_X_2              0x2c48
#define R200_PP_TXPITCH_2                 0x2c50
#define R200_PP_BORDER_COLOR_2            0x2c54
#define R200_PP_CUBIC_FACES_2             0x2c58
#define R200_PP_TXMULTI_CTL_2             0x2c5c
#define R200_PP_TXFILTER_3                0x2c60
#define R200_PP_TXFORMAT_3                0x2c64
#define R200_PP_TXSIZE_3                  0x2c6c
#define R200_PP_TXFORMAT_X_3              0x2c68
#define R200_PP_TXPITCH_3                 0x2c70
#define R200_PP_BORDER_COLOR_3            0x2c74
#define R200_PP_CUBIC_FACES_3             0x2c78
#define R200_PP_TXMULTI_CTL_3             0x2c7c
#define R200_PP_TXFILTER_4                0x2c80
#define R200_PP_TXFORMAT_4                0x2c84
#define R200_PP_TXSIZE_4                  0x2c8c
#define R200_PP_TXFORMAT_X_4              0x2c88
#define R200_PP_TXPITCH_4                 0x2c90
#define R200_PP_BORDER_COLOR_4            0x2c94
#define R200_PP_CUBIC_FACES_4             0x2c98
#define R200_PP_TXMULTI_CTL_4             0x2c9c
#define R200_PP_TXFILTER_5                0x2ca0
#define R200_PP_TXFORMAT_5                0x2ca4
#define R200_PP_TXSIZE_5                  0x2cac
#define R200_PP_TXFORMAT_X_5              0x2ca8
#define R200_PP_TXPITCH_5                 0x2cb0
#define R200_PP_BORDER_COLOR_5            0x2cb4
#define R200_PP_CUBIC_FACES_5             0x2cb8
#define R200_PP_TXMULTI_CTL_5             0x2cbc
/* gap */
#define R200_PP_CNTL_X             0x2cc4  /* Reveree engineered from fglrx */
#define     R200_PPX_TEX_0_ENABLE      (1 <<  0)
#define     R200_PPX_TEX_1_ENABLE      (1 <<  1)
#define     R200_PPX_TEX_2_ENABLE      (1 <<  2)
#define     R200_PPX_TEX_3_ENABLE      (1 <<  3)
#define     R200_PPX_TEX_4_ENABLE      (1 <<  4)
#define     R200_PPX_TEX_5_ENABLE      (1 <<  5)
#define     R200_PPX_TEX_ENABLE_MASK   (0x3f << 0)
#define     R200_PPX_OUTPUT_REG_0      (1 <<  6)
#define     R200_PPX_OUTPUT_REG_1      (1 <<  7)
#define     R200_PPX_OUTPUT_REG_2      (1 <<  8)
#define     R200_PPX_OUTPUT_REG_3      (1 <<  9)
#define     R200_PPX_OUTPUT_REG_4      (1 << 10)
#define     R200_PPX_OUTPUT_REG_5      (1 << 11)
#define     R200_PPX_OUTPUT_REG_MASK   (0x3f << 6)
#define     R200_PPX_OUTPUT_REG_0_SHIFT (6)
#define     R200_PPX_PFS_INST0_ENABLE  (1 << 12)
#define     R200_PPX_PFS_INST1_ENABLE  (1 << 13)
#define     R200_PPX_PFS_INST2_ENABLE  (1 << 14)
#define     R200_PPX_PFS_INST3_ENABLE  (1 << 15)
#define     R200_PPX_PFS_INST4_ENABLE  (1 << 16)
#define     R200_PPX_PFS_INST5_ENABLE  (1 << 17)
#define     R200_PPX_PFS_INST6_ENABLE  (1 << 18)
#define     R200_PPX_PFS_INST7_ENABLE  (1 << 19)
#define     R200_PPX_PFS_INST_ENABLE_MASK (0xff << 12)
#define     R200_PPX_FPS_INST0_ENABLE_SHIFT (12)
/* gap */
#define R200_PP_TRI_PERF                  0x2cf8
#define     R200_TRI_CUTOFF_MASK            (0x1f << 0)
#define R200_PP_PERF_CNTL                 0x2cfc
#define R200_PP_TXOFFSET_0                0x2d00
#define     R200_TXO_ENDIAN_NO_SWAP     (0 << 0)
#define     R200_TXO_ENDIAN_BYTE_SWAP   (1 << 0)
#define     R200_TXO_ENDIAN_WORD_SWAP   (2 << 0)
#define     R200_TXO_ENDIAN_HALFDW_SWAP (3 << 0)
#define     R200_TXO_MACRO_TILE         (1 << 2)
#define     R200_TXO_MICRO_TILE         (1 << 3)
#define     R200_TXO_OFFSET_MASK        0xffffffe0
#define     R200_TXO_OFFSET_SHIFT       5
#define R200_PP_CUBIC_OFFSET_F1_0         0x2d04
#define R200_PP_CUBIC_OFFSET_F2_0         0x2d08
#define R200_PP_CUBIC_OFFSET_F3_0         0x2d0c
#define R200_PP_CUBIC_OFFSET_F4_0         0x2d10
#define R200_PP_CUBIC_OFFSET_F5_0         0x2d14
#define R200_PP_TXOFFSET_1                0x2d18
#define R200_PP_CUBIC_OFFSET_F1_1         0x2d1c
#define R200_PP_CUBIC_OFFSET_F2_1         0x2d20
#define R200_PP_CUBIC_OFFSET_F3_1         0x2d24
#define R200_PP_CUBIC_OFFSET_F4_1         0x2d28
#define R200_PP_CUBIC_OFFSET_F5_1         0x2d2c
#define R200_PP_TXOFFSET_2                0x2d30
#define R200_PP_CUBIC_OFFSET_F1_2         0x2d34
#define R200_PP_CUBIC_OFFSET_F2_2         0x2d38
#define R200_PP_CUBIC_OFFSET_F3_2         0x2d3c
#define R200_PP_CUBIC_OFFSET_F4_2         0x2d40
#define R200_PP_CUBIC_OFFSET_F5_2         0x2d44
#define R200_PP_TXOFFSET_3                0x2d48
#define R200_PP_CUBIC_OFFSET_F1_3         0x2d4c
#define R200_PP_CUBIC_OFFSET_F2_3         0x2d50
#define R200_PP_CUBIC_OFFSET_F3_3         0x2d54
#define R200_PP_CUBIC_OFFSET_F4_3         0x2d58
#define R200_PP_CUBIC_OFFSET_F5_3         0x2d5c
#define R200_PP_TXOFFSET_4                0x2d60
#define R200_PP_CUBIC_OFFSET_F1_4         0x2d64
#define R200_PP_CUBIC_OFFSET_F2_4         0x2d68
#define R200_PP_CUBIC_OFFSET_F3_4         0x2d6c
#define R200_PP_CUBIC_OFFSET_F4_4         0x2d70
#define R200_PP_CUBIC_OFFSET_F5_4         0x2d74
#define R200_PP_TXOFFSET_5                0x2d78
#define R200_PP_CUBIC_OFFSET_F1_5         0x2d7c
#define R200_PP_CUBIC_OFFSET_F2_5         0x2d80
#define R200_PP_CUBIC_OFFSET_F3_5         0x2d84
#define R200_PP_CUBIC_OFFSET_F4_5         0x2d88
#define R200_PP_CUBIC_OFFSET_F5_5         0x2d8c
/* gap */
#define R200_PP_TAM_DEBUG3                0x2d9c
/* gap */
#define R200_PP_TFACTOR_0                 0x2ee0
#define R200_PP_TFACTOR_1                 0x2ee4
#define R200_PP_TFACTOR_2                 0x2ee8
#define R200_PP_TFACTOR_3                 0x2eec
#define R200_PP_TFACTOR_4                 0x2ef0
#define R200_PP_TFACTOR_5                 0x2ef4
#define R200_PP_TFACTOR_6                 0x2ef8
#define R200_PP_TFACTOR_7                 0x2efc
#define R200_PP_TXCBLEND_0                0x2f00
#define     R200_TXC_ARG_A_ZERO                (0)
#define     R200_TXC_ARG_A_CURRENT_COLOR       (2)
#define     R200_TXC_ARG_A_CURRENT_ALPHA       (3)
#define     R200_TXC_ARG_A_DIFFUSE_COLOR       (4)
#define     R200_TXC_ARG_A_DIFFUSE_ALPHA       (5)
#define     R200_TXC_ARG_A_SPECULAR_COLOR      (6)
#define     R200_TXC_ARG_A_SPECULAR_ALPHA      (7)
#define     R200_TXC_ARG_A_TFACTOR_COLOR       (8)
#define     R200_TXC_ARG_A_TFACTOR_ALPHA       (9)
#define     R200_TXC_ARG_A_R0_COLOR            (10)
#define     R200_TXC_ARG_A_R0_ALPHA            (11)
#define     R200_TXC_ARG_A_R1_COLOR            (12)
#define     R200_TXC_ARG_A_R1_ALPHA            (13)
#define     R200_TXC_ARG_A_R2_COLOR            (14)
#define     R200_TXC_ARG_A_R2_ALPHA            (15)
#define     R200_TXC_ARG_A_R3_COLOR            (16)
#define     R200_TXC_ARG_A_R3_ALPHA            (17)
#define     R200_TXC_ARG_A_R4_COLOR            (18)
#define     R200_TXC_ARG_A_R4_ALPHA            (19)
#define     R200_TXC_ARG_A_R5_COLOR            (20)
#define     R200_TXC_ARG_A_R5_ALPHA            (21)
#define     R200_TXC_ARG_A_TFACTOR1_COLOR      (26)
#define     R200_TXC_ARG_A_TFACTOR1_ALPHA      (27)
#define     R200_TXC_ARG_A_MASK			(31 << 0)
#define     R200_TXC_ARG_A_SHIFT			0
#define     R200_TXC_ARG_B_ZERO                (0<<5)
#define     R200_TXC_ARG_B_CURRENT_COLOR       (2<<5)
#define     R200_TXC_ARG_B_CURRENT_ALPHA       (3<<5)
#define     R200_TXC_ARG_B_DIFFUSE_COLOR       (4<<5)
#define     R200_TXC_ARG_B_DIFFUSE_ALPHA       (5<<5)
#define     R200_TXC_ARG_B_SPECULAR_COLOR      (6<<5)
#define     R200_TXC_ARG_B_SPECULAR_ALPHA      (7<<5)
#define     R200_TXC_ARG_B_TFACTOR_COLOR       (8<<5)
#define     R200_TXC_ARG_B_TFACTOR_ALPHA       (9<<5)
#define     R200_TXC_ARG_B_R0_COLOR            (10<<5)
#define     R200_TXC_ARG_B_R0_ALPHA            (11<<5)
#define     R200_TXC_ARG_B_R1_COLOR            (12<<5)
#define     R200_TXC_ARG_B_R1_ALPHA            (13<<5)
#define     R200_TXC_ARG_B_R2_COLOR            (14<<5)
#define     R200_TXC_ARG_B_R2_ALPHA            (15<<5)
#define     R200_TXC_ARG_B_R3_COLOR            (16<<5)
#define     R200_TXC_ARG_B_R3_ALPHA            (17<<5)
#define     R200_TXC_ARG_B_R4_COLOR            (18<<5)
#define     R200_TXC_ARG_B_R4_ALPHA            (19<<5)
#define     R200_TXC_ARG_B_R5_COLOR            (20<<5)
#define     R200_TXC_ARG_B_R5_ALPHA            (21<<5)
#define     R200_TXC_ARG_B_TFACTOR1_COLOR      (26<<5)
#define     R200_TXC_ARG_B_TFACTOR1_ALPHA      (27<<5)
#define     R200_TXC_ARG_B_MASK			(31 << 5)
#define     R200_TXC_ARG_B_SHIFT			5
#define     R200_TXC_ARG_C_ZERO                (0<<10)
#define     R200_TXC_ARG_C_CURRENT_COLOR       (2<<10)
#define     R200_TXC_ARG_C_CURRENT_ALPHA       (3<<10)
#define     R200_TXC_ARG_C_DIFFUSE_COLOR       (4<<10)
#define     R200_TXC_ARG_C_DIFFUSE_ALPHA       (5<<10)
#define     R200_TXC_ARG_C_SPECULAR_COLOR      (6<<10)
#define     R200_TXC_ARG_C_SPECULAR_ALPHA      (7<<10)
#define     R200_TXC_ARG_C_TFACTOR_COLOR       (8<<10)
#define     R200_TXC_ARG_C_TFACTOR_ALPHA       (9<<10)
#define     R200_TXC_ARG_C_R0_COLOR            (10<<10)
#define     R200_TXC_ARG_C_R0_ALPHA            (11<<10)
#define     R200_TXC_ARG_C_R1_COLOR            (12<<10)
#define     R200_TXC_ARG_C_R1_ALPHA            (13<<10)
#define     R200_TXC_ARG_C_R2_COLOR            (14<<10)
#define     R200_TXC_ARG_C_R2_ALPHA            (15<<10)
#define     R200_TXC_ARG_C_R3_COLOR            (16<<10)
#define     R200_TXC_ARG_C_R3_ALPHA            (17<<10)
#define     R200_TXC_ARG_C_R4_COLOR            (18<<10)
#define     R200_TXC_ARG_C_R4_ALPHA            (19<<10)
#define     R200_TXC_ARG_C_R5_COLOR            (20<<10)
#define     R200_TXC_ARG_C_R5_ALPHA            (21<<10)
#define     R200_TXC_ARG_C_TFACTOR1_COLOR      (26<<10)
#define     R200_TXC_ARG_C_TFACTOR1_ALPHA      (27<<10)
#define     R200_TXC_ARG_C_MASK			(31 << 10)
#define     R200_TXC_ARG_C_SHIFT			10
#define     R200_TXC_COMP_ARG_A                    (1 << 16)
#define     R200_TXC_COMP_ARG_A_SHIFT              (16)
#define     R200_TXC_BIAS_ARG_A                    (1 << 17)
#define     R200_TXC_SCALE_ARG_A                   (1 << 18)
#define     R200_TXC_NEG_ARG_A                     (1 << 19)
#define     R200_TXC_COMP_ARG_B                    (1 << 20)
#define     R200_TXC_COMP_ARG_B_SHIFT              (20)
#define     R200_TXC_BIAS_ARG_B                    (1 << 21)
#define     R200_TXC_SCALE_ARG_B                   (1 << 22)
#define     R200_TXC_NEG_ARG_B                     (1 << 23)
#define     R200_TXC_COMP_ARG_C                    (1 << 24)
#define     R200_TXC_COMP_ARG_C_SHIFT              (24)
#define     R200_TXC_BIAS_ARG_C                    (1 << 25)
#define     R200_TXC_SCALE_ARG_C                   (1 << 26)
#define     R200_TXC_NEG_ARG_C                     (1 << 27)
#define     R200_TXC_OP_MADD                        (0 << 28)
#define     R200_TXC_OP_CND0                       (2 << 28)
#define     R200_TXC_OP_LERP                       (3 << 28)
#define     R200_TXC_OP_DOT3                       (4 << 28)
#define     R200_TXC_OP_DOT4                       (5 << 28)
#define     R200_TXC_OP_CONDITIONAL                (6 << 28)
#define     R200_TXC_OP_DOT2_ADD                   (7 << 28)
#define     R200_TXC_OP_MASK                       (7 << 28)
#define R200_PP_TXCBLEND2_0                0x2f04
#define     R200_TXC_TFACTOR_SEL_SHIFT             0
#define     R200_TXC_TFACTOR_SEL_MASK              0x7
#define     R200_TXC_TFACTOR1_SEL_SHIFT            4
#define     R200_TXC_TFACTOR1_SEL_MASK             (0x7 << 4)
#define     R200_TXC_SCALE_SHIFT                   8
#define     R200_TXC_SCALE_MASK                    (7 << 8)
#define     R200_TXC_SCALE_1X                      (0 << 8)
#define     R200_TXC_SCALE_2X                      (1 << 8)
#define     R200_TXC_SCALE_4X                      (2 << 8)
#define     R200_TXC_SCALE_8X                      (3 << 8)
#define     R200_TXC_SCALE_INV2                    (5 << 8)
#define     R200_TXC_SCALE_INV4                    (6 << 8)
#define     R200_TXC_SCALE_INV8                    (7 << 8)
#define     R200_TXC_CLAMP_SHIFT                   12
#define     R200_TXC_CLAMP_MASK                    (3 << 12)
#define     R200_TXC_CLAMP_WRAP                    (0 << 12)
#define     R200_TXC_CLAMP_0_1                     (1 << 12)
#define     R200_TXC_CLAMP_8_8                     (2 << 12)
#define     R200_TXC_OUTPUT_REG_SHIFT              16
#define     R200_TXC_OUTPUT_REG_MASK               (7 << 16)
#define     R200_TXC_OUTPUT_REG_NONE               (0 << 16)
#define     R200_TXC_OUTPUT_REG_R0                 (1 << 16)
#define     R200_TXC_OUTPUT_REG_R1                 (2 << 16)
#define     R200_TXC_OUTPUT_REG_R2                 (3 << 16)
#define     R200_TXC_OUTPUT_REG_R3                 (4 << 16)
#define     R200_TXC_OUTPUT_REG_R4                 (5 << 16)
#define     R200_TXC_OUTPUT_REG_R5                 (6 << 16)
#define     R200_TXC_OUTPUT_MASK_MASK              (7 << 20)
#define     R200_TXC_OUTPUT_MASK_RGB               (0 << 20)
#define     R200_TXC_OUTPUT_MASK_RG                (1 << 20)
#define     R200_TXC_OUTPUT_MASK_RB                (2 << 20)
#define     R200_TXC_OUTPUT_MASK_R                 (3 << 20)
#define     R200_TXC_OUTPUT_MASK_GB                (4 << 20)
#define     R200_TXC_OUTPUT_MASK_G                 (5 << 20)
#define     R200_TXC_OUTPUT_MASK_B                 (6 << 20)
#define     R200_TXC_OUTPUT_MASK_NONE              (7 << 20)
#define     R200_TXC_REPL_NORMAL                   0
#define     R200_TXC_REPL_RED                      1
#define     R200_TXC_REPL_GREEN                    2
#define     R200_TXC_REPL_BLUE                     3
#define     R200_TXC_REPL_ARG_A_SHIFT              26
#define     R200_TXC_REPL_ARG_A_MASK               (3 << 26)
#define     R200_TXC_REPL_ARG_B_SHIFT              28
#define     R200_TXC_REPL_ARG_B_MASK               (3 << 28)
#define     R200_TXC_REPL_ARG_C_SHIFT              30
#define     R200_TXC_REPL_ARG_C_MASK               (3 << 30)
#define R200_PP_TXABLEND_0                0x2f08
#define     R200_TXA_ARG_A_ZERO              (0)
#define     R200_TXA_ARG_A_CURRENT_ALPHA     (2) /* guess */
#define     R200_TXA_ARG_A_CURRENT_BLUE      (3) /* guess */
#define     R200_TXA_ARG_A_DIFFUSE_ALPHA     (4)
#define     R200_TXA_ARG_A_DIFFUSE_BLUE      (5)
#define     R200_TXA_ARG_A_SPECULAR_ALPHA    (6)
#define     R200_TXA_ARG_A_SPECULAR_BLUE     (7)
#define     R200_TXA_ARG_A_TFACTOR_ALPHA     (8)
#define     R200_TXA_ARG_A_TFACTOR_BLUE      (9)
#define     R200_TXA_ARG_A_R0_ALPHA          (10)
#define     R200_TXA_ARG_A_R0_BLUE           (11)
#define     R200_TXA_ARG_A_R1_ALPHA          (12)
#define     R200_TXA_ARG_A_R1_BLUE           (13)
#define     R200_TXA_ARG_A_R2_ALPHA          (14)
#define     R200_TXA_ARG_A_R2_BLUE           (15)
#define     R200_TXA_ARG_A_R3_ALPHA          (16)
#define     R200_TXA_ARG_A_R3_BLUE           (17)
#define     R200_TXA_ARG_A_R4_ALPHA          (18)
#define     R200_TXA_ARG_A_R4_BLUE           (19)
#define     R200_TXA_ARG_A_R5_ALPHA          (20)
#define     R200_TXA_ARG_A_R5_BLUE           (21)
#define     R200_TXA_ARG_A_TFACTOR1_ALPHA    (26)
#define     R200_TXA_ARG_A_TFACTOR1_BLUE     (27)
#define     R200_TXA_ARG_A_MASK			(31 << 0)
#define     R200_TXA_ARG_A_SHIFT			0
#define     R200_TXA_ARG_B_ZERO              (0<<5)
#define     R200_TXA_ARG_B_CURRENT_ALPHA     (2<<5) /* guess */
#define     R200_TXA_ARG_B_CURRENT_BLUE      (3<<5) /* guess */
#define     R200_TXA_ARG_B_DIFFUSE_ALPHA     (4<<5)
#define     R200_TXA_ARG_B_DIFFUSE_BLUE      (5<<5)
#define     R200_TXA_ARG_B_SPECULAR_ALPHA    (6<<5)
#define     R200_TXA_ARG_B_SPECULAR_BLUE     (7<<5)
#define     R200_TXA_ARG_B_TFACTOR_ALPHA     (8<<5)
#define     R200_TXA_ARG_B_TFACTOR_BLUE      (9<<5)
#define     R200_TXA_ARG_B_R0_ALPHA          (10<<5)
#define     R200_TXA_ARG_B_R0_BLUE           (11<<5)
#define     R200_TXA_ARG_B_R1_ALPHA          (12<<5)
#define     R200_TXA_ARG_B_R1_BLUE           (13<<5)
#define     R200_TXA_ARG_B_R2_ALPHA          (14<<5)
#define     R200_TXA_ARG_B_R2_BLUE           (15<<5)
#define     R200_TXA_ARG_B_R3_ALPHA          (16<<5)
#define     R200_TXA_ARG_B_R3_BLUE           (17<<5)
#define     R200_TXA_ARG_B_R4_ALPHA          (18<<5)
#define     R200_TXA_ARG_B_R4_BLUE           (19<<5)
#define     R200_TXA_ARG_B_R5_ALPHA          (20<<5)
#define     R200_TXA_ARG_B_R5_BLUE           (21<<5)
#define     R200_TXA_ARG_B_TFACTOR1_ALPHA    (26<<5)
#define     R200_TXA_ARG_B_TFACTOR1_BLUE     (27<<5)
#define     R200_TXA_ARG_B_MASK			(31 << 5)
#define     R200_TXA_ARG_B_SHIFT			5
#define     R200_TXA_ARG_C_ZERO              (0<<10)
#define     R200_TXA_ARG_C_CURRENT_ALPHA     (2<<10) /* guess */
#define     R200_TXA_ARG_C_CURRENT_BLUE      (3<<10) /* guess */
#define     R200_TXA_ARG_C_DIFFUSE_ALPHA     (4<<10)
#define     R200_TXA_ARG_C_DIFFUSE_BLUE      (5<<10)
#define     R200_TXA_ARG_C_SPECULAR_ALPHA    (6<<10)
#define     R200_TXA_ARG_C_SPECULAR_BLUE     (7<<10)
#define     R200_TXA_ARG_C_TFACTOR_ALPHA     (8<<10)
#define     R200_TXA_ARG_C_TFACTOR_BLUE      (9<<10)
#define     R200_TXA_ARG_C_R0_ALPHA          (10<<10)
#define     R200_TXA_ARG_C_R0_BLUE           (11<<10)
#define     R200_TXA_ARG_C_R1_ALPHA          (12<<10)
#define     R200_TXA_ARG_C_R1_BLUE           (13<<10)
#define     R200_TXA_ARG_C_R2_ALPHA          (14<<10)
#define     R200_TXA_ARG_C_R2_BLUE           (15<<10)
#define     R200_TXA_ARG_C_R3_ALPHA          (16<<10)
#define     R200_TXA_ARG_C_R3_BLUE           (17<<10)
#define     R200_TXA_ARG_C_R4_ALPHA          (18<<10)
#define     R200_TXA_ARG_C_R4_BLUE           (19<<10)
#define     R200_TXA_ARG_C_R5_ALPHA          (20<<10)
#define     R200_TXA_ARG_C_R5_BLUE           (21<<10)
#define     R200_TXA_ARG_C_TFACTOR1_ALPHA    (26<<10)
#define     R200_TXA_ARG_C_TFACTOR1_BLUE     (27<<10)
#define     R200_TXA_ARG_C_MASK			(31 << 10)
#define     R200_TXA_ARG_C_SHIFT			10
#define     R200_TXA_COMP_ARG_A                    (1 << 16)
#define     R200_TXA_COMP_ARG_A_SHIFT              (16)
#define     R200_TXA_BIAS_ARG_A                    (1 << 17)
#define     R200_TXA_SCALE_ARG_A                   (1 << 18)
#define     R200_TXA_NEG_ARG_A                     (1 << 19)
#define     R200_TXA_COMP_ARG_B                    (1 << 20)
#define     R200_TXA_COMP_ARG_B_SHIFT              (20)
#define     R200_TXA_BIAS_ARG_B                    (1 << 21)
#define     R200_TXA_SCALE_ARG_B                   (1 << 22)
#define     R200_TXA_NEG_ARG_B                     (1 << 23)
#define     R200_TXA_COMP_ARG_C                    (1 << 24)
#define     R200_TXA_COMP_ARG_C_SHIFT              (24)
#define     R200_TXA_BIAS_ARG_C                    (1 << 25)
#define     R200_TXA_SCALE_ARG_C                   (1 << 26)
#define     R200_TXA_NEG_ARG_C                     (1 << 27)
#define     R200_TXA_OP_MADD                       (0 << 28)
#define     R200_TXA_OP_CND0                       (2 << 28)
#define     R200_TXA_OP_LERP                       (3 << 28)
#define     R200_TXA_OP_CONDITIONAL                (6 << 28)
#define     R200_TXA_OP_MASK                       (7 << 28)
#define R200_PP_TXABLEND2_0                0x2f0c
#define     R200_TXA_TFACTOR_SEL_SHIFT             0
#define     R200_TXA_TFACTOR_SEL_MASK              0x7
#define     R200_TXA_TFACTOR1_SEL_SHIFT            4
#define     R200_TXA_TFACTOR1_SEL_MASK             (0x7 << 4)
#define     R200_TXA_SCALE_SHIFT                   8
#define     R200_TXA_SCALE_MASK                    (7 << 8)
#define     R200_TXA_SCALE_1X                      (0 << 8)
#define     R200_TXA_SCALE_2X                      (1 << 8)
#define     R200_TXA_SCALE_4X                      (2 << 8)
#define     R200_TXA_SCALE_8X                      (3 << 8)
#define     R200_TXA_SCALE_INV2                    (5 << 8)
#define     R200_TXA_SCALE_INV4                    (6 << 8)
#define     R200_TXA_SCALE_INV8                    (7 << 8)
#define     R200_TXA_CLAMP_SHIFT                   12
#define     R200_TXA_CLAMP_MASK                    (3 << 12)
#define     R200_TXA_CLAMP_WRAP                    (0 << 12)
#define     R200_TXA_CLAMP_0_1                     (1 << 12)
#define     R200_TXA_CLAMP_8_8                     (2 << 12)
#define     R200_TXA_OUTPUT_REG_SHIFT              16
#define     R200_TXA_OUTPUT_REG_MASK               (7 << 16)
#define     R200_TXA_OUTPUT_REG_NONE               (0 << 16)
#define     R200_TXA_OUTPUT_REG_R0                 (1 << 16)
#define     R200_TXA_OUTPUT_REG_R1                 (2 << 16)
#define     R200_TXA_OUTPUT_REG_R2                 (3 << 16)
#define     R200_TXA_OUTPUT_REG_R3                 (4 << 16)
#define     R200_TXA_OUTPUT_REG_R4                 (5 << 16)
#define     R200_TXA_OUTPUT_REG_R5                 (6 << 16)
#define     R200_TXA_DOT_ALPHA                     (1 << 20)
#define     R200_TXA_REPL_NORMAL                   0
#define     R200_TXA_REPL_RED                      1
#define     R200_TXA_REPL_GREEN                    2
#define     R200_TXA_REPL_ARG_A_SHIFT              26
#define     R200_TXA_REPL_ARG_A_MASK               (3 << 26)
#define     R200_TXA_REPL_ARG_B_SHIFT              28
#define     R200_TXA_REPL_ARG_B_MASK               (3 << 28)
#define     R200_TXA_REPL_ARG_C_SHIFT              30
#define     R200_TXA_REPL_ARG_C_MASK               (3 << 30)
#define R200_PP_TXCBLEND_1                0x2f10
#define R200_PP_TXCBLEND2_1               0x2f14
#define R200_PP_TXABLEND_1                0x2f18
#define R200_PP_TXABLEND2_1               0x2f1c
#define R200_PP_TXCBLEND_2                0x2f20
#define R200_PP_TXCBLEND2_2               0x2f24
#define R200_PP_TXABLEND_2                0x2f28
#define R200_PP_TXABLEND2_2               0x2f2c
#define R200_PP_TXCBLEND_3                0x2f30
#define R200_PP_TXCBLEND2_3               0x2f34
#define R200_PP_TXABLEND_3                0x2f38
#define R200_PP_TXABLEND2_3               0x2f3c
#define R200_PP_TXCBLEND_4                0x2f40
#define R200_PP_TXCBLEND2_4               0x2f44
#define R200_PP_TXABLEND_4                0x2f48
#define R200_PP_TXABLEND2_4               0x2f4c
#define R200_PP_TXCBLEND_5                0x2f50
#define R200_PP_TXCBLEND2_5               0x2f54
#define R200_PP_TXABLEND_5                0x2f58
#define R200_PP_TXABLEND2_5               0x2f5c
#define R200_PP_TXCBLEND_6                0x2f60
#define R200_PP_TXCBLEND2_6               0x2f64
#define R200_PP_TXABLEND_6                0x2f68
#define R200_PP_TXABLEND2_6               0x2f6c
#define R200_PP_TXCBLEND_7                0x2f70
#define R200_PP_TXCBLEND2_7               0x2f74
#define R200_PP_TXABLEND_7                0x2f78
#define R200_PP_TXABLEND2_7               0x2f7c
#define R200_PP_TXCBLEND_8                0x2f80
#define R200_PP_TXCBLEND2_8               0x2f84
#define R200_PP_TXABLEND_8                0x2f88
#define R200_PP_TXABLEND2_8               0x2f8c
#define R200_PP_TXCBLEND_9                0x2f90
#define R200_PP_TXCBLEND2_9               0x2f94
#define R200_PP_TXABLEND_9                0x2f98
#define R200_PP_TXABLEND2_9               0x2f9c
#define R200_PP_TXCBLEND_10               0x2fa0
#define R200_PP_TXCBLEND2_10              0x2fa4
#define R200_PP_TXABLEND_10               0x2fa8
#define R200_PP_TXABLEND2_10              0x2fac
#define R200_PP_TXCBLEND_11               0x2fb0
#define R200_PP_TXCBLEND2_11              0x2fb4
#define R200_PP_TXABLEND_11               0x2fb8
#define R200_PP_TXABLEND2_11              0x2fbc
#define R200_PP_TXCBLEND_12               0x2fc0
#define R200_PP_TXCBLEND2_12              0x2fc4
#define R200_PP_TXABLEND_12               0x2fc8
#define R200_PP_TXABLEND2_12              0x2fcc
#define R200_PP_TXCBLEND_13               0x2fd0
#define R200_PP_TXCBLEND2_13              0x2fd4
#define R200_PP_TXABLEND_13               0x2fd8
#define R200_PP_TXABLEND2_13              0x2fdc
#define R200_PP_TXCBLEND_14               0x2fe0
#define R200_PP_TXCBLEND2_14              0x2fe4
#define R200_PP_TXABLEND_14               0x2fe8
#define R200_PP_TXABLEND2_14              0x2fec
#define R200_PP_TXCBLEND_15               0x2ff0
#define R200_PP_TXCBLEND2_15              0x2ff4
#define R200_PP_TXABLEND_15               0x2ff8
#define R200_PP_TXABLEND2_15              0x2ffc
/* gap */
#define R200_RB3D_BLENDCOLOR               0x3218 /* ARGB 8888 */
#define R200_RB3D_ABLENDCNTL               0x321C /* see BLENDCTL */
#define R200_RB3D_CBLENDCNTL               0x3220 /* see BLENDCTL */


/*
 * Offsets in TCL vector state.  NOTE: Hardwiring matrix positions.
 * Multiple contexts could collaberate to eliminate state bouncing.
 */
#define R200_VS_LIGHT_AMBIENT_ADDR          0x00000028
#define R200_VS_LIGHT_DIFFUSE_ADDR          0x00000030
#define R200_VS_LIGHT_SPECULAR_ADDR         0x00000038
#define R200_VS_LIGHT_DIRPOS_ADDR           0x00000040
#define R200_VS_LIGHT_HWVSPOT_ADDR          0x00000048
#define R200_VS_LIGHT_ATTENUATION_ADDR      0x00000050
#define R200_VS_SPOT_DUAL_CONE              0x00000058
#define R200_VS_GLOBAL_AMBIENT_ADDR         0x0000005C
#define R200_VS_FOG_PARAM_ADDR              0x0000005D
#define R200_VS_EYE_VECTOR_ADDR             0x0000005E
#define R200_VS_UCP_ADDR                    0x00000060
#define R200_VS_PNT_SPRITE_VPORT_SCALE      0x00000068
#define R200_VS_MATRIX_0_MV                 0x00000080
#define R200_VS_MATRIX_1_INV_MV        	    0x00000084
#define R200_VS_MATRIX_2_MVP        	    0x00000088
#define R200_VS_MATRIX_3_TEX0        	    0x0000008C
#define R200_VS_MATRIX_4_TEX1        	    0x00000090
#define R200_VS_MATRIX_5_TEX2        	    0x00000094
#define R200_VS_MATRIX_6_TEX3        	    0x00000098
#define R200_VS_MATRIX_7_TEX4        	    0x0000009C
#define R200_VS_MATRIX_8_TEX5        	    0x000000A0
#define R200_VS_MAT_0_EMISS                 0x000000B0
#define R200_VS_MAT_0_AMB                   0x000000B1
#define R200_VS_MAT_0_DIF                   0x000000B2
#define R200_VS_MAT_0_SPEC                  0x000000B3
#define R200_VS_MAT_1_EMISS                 0x000000B4
#define R200_VS_MAT_1_AMB                   0x000000B5
#define R200_VS_MAT_1_DIF                   0x000000B6
#define R200_VS_MAT_1_SPEC                  0x000000B7
#define R200_VS_EYE2CLIP_MTX                0x000000B8
#define R200_VS_PNT_SPRITE_ATT_CONST        0x000000BC
#define R200_VS_PNT_SPRITE_EYE_IN_MODEL     0x000000BD
#define R200_VS_PNT_SPRITE_CLAMP            0x000000BE
#define R200_VS_MAX                         0x000001C0

#define R200_PVS_PROG0                      0x00000080
#define R200_PVS_PROG1                      0x00000180
#define R200_PVS_PARAM0                     0x00000000
#define R200_PVS_PARAM1                     0x00000100

/*
 * Offsets in TCL scalar state
 */
#define R200_SS_LIGHT_DCD_ADDR              0x00000000
#define R200_SS_LIGHT_DCM_ADDR              0x00000008
#define R200_SS_LIGHT_SPOT_EXPONENT_ADDR    0x00000010
#define R200_SS_LIGHT_SPOT_CUTOFF_ADDR      0x00000018
#define R200_SS_LIGHT_SPECULAR_THRESH_ADDR  0x00000020
#define R200_SS_LIGHT_RANGE_CUTOFF_SQRD     0x00000028
#define R200_SS_LIGHT_RANGE_ATT_CONST       0x00000030
#define R200_SS_VERT_GUARD_CLIP_ADJ_ADDR    0x00000080
#define R200_SS_VERT_GUARD_DISCARD_ADJ_ADDR 0x00000081
#define R200_SS_HORZ_GUARD_CLIP_ADJ_ADDR    0x00000082
#define R200_SS_HORZ_GUARD_DISCARD_ADJ_ADDR 0x00000083
#define R200_SS_MAT_0_SHININESS             0x00000100
#define R200_SS_MAT_1_SHININESS             0x00000101


/*
 * Matrix indices
 */
#define R200_MTX_MV                        0
#define R200_MTX_IMV                       1
#define R200_MTX_MVP                       2
#define R200_MTX_TEX0                      3
#define R200_MTX_TEX1                      4
#define R200_MTX_TEX2                      5
#define R200_MTX_TEX3                      6
#define R200_MTX_TEX4                      7
#define R200_MTX_TEX5                      8

/* Color formats for 2d packets
 */
#define R200_CP_COLOR_FORMAT_CI8	2
#define R200_CP_COLOR_FORMAT_ARGB1555	3
#define R200_CP_COLOR_FORMAT_RGB565	4
#define R200_CP_COLOR_FORMAT_ARGB8888	6
#define R200_CP_COLOR_FORMAT_RGB332	7
#define R200_CP_COLOR_FORMAT_RGB8	9
#define R200_CP_COLOR_FORMAT_ARGB4444	15


/*
 * CP type-3 packets
 */
#define R200_CP_CMD_NOP                 0xC0001000
#define R200_CP_CMD_NEXT_CHAR           0xC0001900
#define R200_CP_CMD_PLY_NEXTSCAN        0xC0001D00
#define R200_CP_CMD_SET_SCISSORS        0xC0001E00
#define R200_CP_CMD_LOAD_MICROCODE      0xC0002400
#define R200_CP_CMD_WAIT_FOR_IDLE       0xC0002600
#define R200_CP_CMD_3D_DRAW_VBUF        0xC0002800
#define R200_CP_CMD_3D_DRAW_IMMD        0xC0002900
#define R200_CP_CMD_3D_DRAW_INDX        0xC0002A00
#define R200_CP_CMD_LOAD_PALETTE        0xC0002C00
#define R200_CP_CMD_3D_LOAD_VBPNTR      0xC0002F00
#define R200_CP_CMD_INDX_BUFFER         0xC0003300
#define R200_CP_CMD_3D_DRAW_VBUF_2      0xC0003400
#define R200_CP_CMD_3D_DRAW_IMMD_2      0xC0003500
#define R200_CP_CMD_3D_DRAW_INDX_2      0xC0003600
#define R200_CP_CMD_PAINT		0xC0009100
#define R200_CP_CMD_BITBLT		0xC0009200
#define R200_CP_CMD_SMALLTEXT		0xC0009300
#define R200_CP_CMD_HOSTDATA_BLT	0xC0009400
#define R200_CP_CMD_POLYLINE		0xC0009500
#define R200_CP_CMD_POLYSCANLINES	0xC0009800
#define R200_CP_CMD_PAINT_MULTI		0xC0009A00
#define R200_CP_CMD_BITBLT_MULTI	0xC0009B00
#define R200_CP_CMD_TRANS_BITBLT	0xC0009C00

#endif

