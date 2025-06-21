/*
 * d3dbc (Direct3D shader models 1-3 bytecode) support
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009, 2021 Henri Verbeet for CodeWeavers
 * Copyright 2019-2020 Zebediah Figura for CodeWeavers
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

#include "hlsl.h"

#define VKD3D_SM1_VS  0xfffeu
#define VKD3D_SM1_PS  0xffffu

#define VKD3D_SM1_DCL_USAGE_SHIFT              0u
#define VKD3D_SM1_DCL_USAGE_MASK               (0xfu << VKD3D_SM1_DCL_USAGE_SHIFT)
#define VKD3D_SM1_DCL_USAGE_INDEX_SHIFT        16u
#define VKD3D_SM1_DCL_USAGE_INDEX_MASK         (0xfu << VKD3D_SM1_DCL_USAGE_INDEX_SHIFT)

#define VKD3D_SM1_RESOURCE_TYPE_SHIFT          27u
#define VKD3D_SM1_RESOURCE_TYPE_MASK           (0xfu << VKD3D_SM1_RESOURCE_TYPE_SHIFT)

#define VKD3D_SM1_OPCODE_MASK                  0x0000ffffu

#define VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT      16u
#define VKD3D_SM1_INSTRUCTION_FLAGS_MASK       (0xffu << VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT)

#define VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT     24u
#define VKD3D_SM1_INSTRUCTION_LENGTH_MASK      (0xfu << VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT)

#define VKD3D_SM1_COISSUE                      (0x1u << 30u)

#define VKD3D_SM1_COMMENT_SIZE_SHIFT           16u
#define VKD3D_SM1_COMMENT_SIZE_MASK            (0x7fffu << VKD3D_SM1_COMMENT_SIZE_SHIFT)

#define VKD3D_SM1_INSTRUCTION_PREDICATED       (0x1u << 28u)

#define VKD3D_SM1_INSTRUCTION_PARAMETER        (0x1u << 31u)

#define VKD3D_SM1_REGISTER_NUMBER_MASK         0x000007ffu

#define VKD3D_SM1_REGISTER_TYPE_SHIFT          28u
#define VKD3D_SM1_REGISTER_TYPE_MASK           (0x7u << VKD3D_SM1_REGISTER_TYPE_SHIFT)
#define VKD3D_SM1_REGISTER_TYPE_SHIFT2         8u
#define VKD3D_SM1_REGISTER_TYPE_MASK2          (0x18u << VKD3D_SM1_REGISTER_TYPE_SHIFT2)

#define VKD3D_SM1_ADDRESS_MODE_SHIFT           13u
#define VKD3D_SM1_ADDRESS_MODE_MASK            (0x1u << VKD3D_SM1_ADDRESS_MODE_SHIFT)

#define VKD3D_SM1_DST_MODIFIER_SHIFT           20u
#define VKD3D_SM1_DST_MODIFIER_MASK            (0xfu << VKD3D_SM1_DST_MODIFIER_SHIFT)

#define VKD3D_SM1_DSTSHIFT_SHIFT               24u
#define VKD3D_SM1_DSTSHIFT_MASK                (0xfu << VKD3D_SM1_DSTSHIFT_SHIFT)

#define VKD3D_SM1_WRITEMASK_SHIFT              16u
#define VKD3D_SM1_WRITEMASK_MASK               (0xfu << VKD3D_SM1_WRITEMASK_SHIFT)

#define VKD3D_SM1_SWIZZLE_SHIFT                16u
#define VKD3D_SM1_SWIZZLE_MASK                 (0xffu << VKD3D_SM1_SWIZZLE_SHIFT)
#define VKD3D_SM1_SWIZZLE_DEFAULT              (0u | (1u << 2) | (2u << 4) | (3u << 6))

#define VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(idx) (2u * (idx))
#define VKD3D_SM1_SWIZZLE_COMPONENT_MASK(idx)  (0x3u << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(idx))

#define VKD3D_SM1_SRC_MODIFIER_SHIFT           24u
#define VKD3D_SM1_SRC_MODIFIER_MASK            (0xfu << VKD3D_SM1_SRC_MODIFIER_SHIFT)

#define VKD3D_SM1_END                          0x0000ffffu

#define VKD3D_SM1_VERSION_MAJOR(version)       (((version) >> 8u) & 0xffu)
#define VKD3D_SM1_VERSION_MINOR(version)       (((version) >> 0u) & 0xffu)

enum vkd3d_sm1_address_mode_type
{
    VKD3D_SM1_ADDRESS_MODE_ABSOLUTE = 0x0,
    VKD3D_SM1_ADDRESS_MODE_RELATIVE = 0x1,
};

enum vkd3d_sm1_resource_type
{
    VKD3D_SM1_RESOURCE_UNKNOWN      = 0x0,
    VKD3D_SM1_RESOURCE_TEXTURE_1D   = 0x1,
    VKD3D_SM1_RESOURCE_TEXTURE_2D   = 0x2,
    VKD3D_SM1_RESOURCE_TEXTURE_CUBE = 0x3,
    VKD3D_SM1_RESOURCE_TEXTURE_3D   = 0x4,
};

enum vkd3d_sm1_misc_register
{
    VKD3D_SM1_MISC_POSITION         = 0x0,
    VKD3D_SM1_MISC_FACE             = 0x1,
};

enum vkd3d_sm1_opcode
{
    VKD3D_SM1_OP_NOP          = 0x00,
    VKD3D_SM1_OP_MOV          = 0x01,
    VKD3D_SM1_OP_ADD          = 0x02,
    VKD3D_SM1_OP_SUB          = 0x03,
    VKD3D_SM1_OP_MAD          = 0x04,
    VKD3D_SM1_OP_MUL          = 0x05,
    VKD3D_SM1_OP_RCP          = 0x06,
    VKD3D_SM1_OP_RSQ          = 0x07,
    VKD3D_SM1_OP_DP3          = 0x08,
    VKD3D_SM1_OP_DP4          = 0x09,
    VKD3D_SM1_OP_MIN          = 0x0a,
    VKD3D_SM1_OP_MAX          = 0x0b,
    VKD3D_SM1_OP_SLT          = 0x0c,
    VKD3D_SM1_OP_SGE          = 0x0d,
    VKD3D_SM1_OP_EXP          = 0x0e,
    VKD3D_SM1_OP_LOG          = 0x0f,
    VKD3D_SM1_OP_LIT          = 0x10,
    VKD3D_SM1_OP_DST          = 0x11,
    VKD3D_SM1_OP_LRP          = 0x12,
    VKD3D_SM1_OP_FRC          = 0x13,
    VKD3D_SM1_OP_M4x4         = 0x14,
    VKD3D_SM1_OP_M4x3         = 0x15,
    VKD3D_SM1_OP_M3x4         = 0x16,
    VKD3D_SM1_OP_M3x3         = 0x17,
    VKD3D_SM1_OP_M3x2         = 0x18,
    VKD3D_SM1_OP_CALL         = 0x19,
    VKD3D_SM1_OP_CALLNZ       = 0x1a,
    VKD3D_SM1_OP_LOOP         = 0x1b,
    VKD3D_SM1_OP_RET          = 0x1c,
    VKD3D_SM1_OP_ENDLOOP      = 0x1d,
    VKD3D_SM1_OP_LABEL        = 0x1e,
    VKD3D_SM1_OP_DCL          = 0x1f,
    VKD3D_SM1_OP_POW          = 0x20,
    VKD3D_SM1_OP_CRS          = 0x21,
    VKD3D_SM1_OP_SGN          = 0x22,
    VKD3D_SM1_OP_ABS          = 0x23,
    VKD3D_SM1_OP_NRM          = 0x24,
    VKD3D_SM1_OP_SINCOS       = 0x25,
    VKD3D_SM1_OP_REP          = 0x26,
    VKD3D_SM1_OP_ENDREP       = 0x27,
    VKD3D_SM1_OP_IF           = 0x28,
    VKD3D_SM1_OP_IFC          = 0x29,
    VKD3D_SM1_OP_ELSE         = 0x2a,
    VKD3D_SM1_OP_ENDIF        = 0x2b,
    VKD3D_SM1_OP_BREAK        = 0x2c,
    VKD3D_SM1_OP_BREAKC       = 0x2d,
    VKD3D_SM1_OP_MOVA         = 0x2e,
    VKD3D_SM1_OP_DEFB         = 0x2f,
    VKD3D_SM1_OP_DEFI         = 0x30,

    VKD3D_SM1_OP_TEXCOORD     = 0x40,
    VKD3D_SM1_OP_TEXKILL      = 0x41,
    VKD3D_SM1_OP_TEX          = 0x42,
    VKD3D_SM1_OP_TEXBEM       = 0x43,
    VKD3D_SM1_OP_TEXBEML      = 0x44,
    VKD3D_SM1_OP_TEXREG2AR    = 0x45,
    VKD3D_SM1_OP_TEXREG2GB    = 0x46,
    VKD3D_SM1_OP_TEXM3x2PAD   = 0x47,
    VKD3D_SM1_OP_TEXM3x2TEX   = 0x48,
    VKD3D_SM1_OP_TEXM3x3PAD   = 0x49,
    VKD3D_SM1_OP_TEXM3x3TEX   = 0x4a,
    VKD3D_SM1_OP_TEXM3x3DIFF  = 0x4b,
    VKD3D_SM1_OP_TEXM3x3SPEC  = 0x4c,
    VKD3D_SM1_OP_TEXM3x3VSPEC = 0x4d,
    VKD3D_SM1_OP_EXPP         = 0x4e,
    VKD3D_SM1_OP_LOGP         = 0x4f,
    VKD3D_SM1_OP_CND          = 0x50,
    VKD3D_SM1_OP_DEF          = 0x51,
    VKD3D_SM1_OP_TEXREG2RGB   = 0x52,
    VKD3D_SM1_OP_TEXDP3TEX    = 0x53,
    VKD3D_SM1_OP_TEXM3x2DEPTH = 0x54,
    VKD3D_SM1_OP_TEXDP3       = 0x55,
    VKD3D_SM1_OP_TEXM3x3      = 0x56,
    VKD3D_SM1_OP_TEXDEPTH     = 0x57,
    VKD3D_SM1_OP_CMP          = 0x58,
    VKD3D_SM1_OP_BEM          = 0x59,
    VKD3D_SM1_OP_DP2ADD       = 0x5a,
    VKD3D_SM1_OP_DSX          = 0x5b,
    VKD3D_SM1_OP_DSY          = 0x5c,
    VKD3D_SM1_OP_TEXLDD       = 0x5d,
    VKD3D_SM1_OP_SETP         = 0x5e,
    VKD3D_SM1_OP_TEXLDL       = 0x5f,
    VKD3D_SM1_OP_BREAKP       = 0x60,

    VKD3D_SM1_OP_PHASE        = 0xfffd,
    VKD3D_SM1_OP_COMMENT      = 0xfffe,
    VKD3D_SM1_OP_END          = 0Xffff,
};

struct vkd3d_sm1_opcode_info
{
    enum vkd3d_sm1_opcode sm1_opcode;
    unsigned int dst_count;
    unsigned int src_count;
    enum vkd3d_shader_opcode vkd3d_opcode;
    struct
    {
        unsigned int major, minor;
    } min_version, max_version;
};

struct vkd3d_shader_sm1_parser
{
    const struct vkd3d_sm1_opcode_info *opcode_table;
    const uint32_t *start, *end, *ptr;
    bool abort;

    struct vkd3d_shader_parser p;

    struct
    {
#define MAX_CONSTANT_COUNT 8192
        uint32_t def_mask[VKD3D_BITMAP_SIZE(MAX_CONSTANT_COUNT)];
        uint32_t count;
    } constants[3];
};

/* This table is not order or position dependent. */
static const struct vkd3d_sm1_opcode_info vs_opcode_table[] =
{
    /* Arithmetic */
    {VKD3D_SM1_OP_NOP,          0, 0, VKD3DSIH_NOP},
    {VKD3D_SM1_OP_MOV,          1, 1, VKD3DSIH_MOV},
    {VKD3D_SM1_OP_MOVA,         1, 1, VKD3DSIH_MOVA,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ADD,          1, 2, VKD3DSIH_ADD},
    {VKD3D_SM1_OP_SUB,          1, 2, VKD3DSIH_SUB},
    {VKD3D_SM1_OP_MAD,          1, 3, VKD3DSIH_MAD},
    {VKD3D_SM1_OP_MUL,          1, 2, VKD3DSIH_MUL},
    {VKD3D_SM1_OP_RCP,          1, 1, VKD3DSIH_RCP},
    {VKD3D_SM1_OP_RSQ,          1, 1, VKD3DSIH_RSQ},
    {VKD3D_SM1_OP_DP3,          1, 2, VKD3DSIH_DP3},
    {VKD3D_SM1_OP_DP4,          1, 2, VKD3DSIH_DP4},
    {VKD3D_SM1_OP_MIN,          1, 2, VKD3DSIH_MIN},
    {VKD3D_SM1_OP_MAX,          1, 2, VKD3DSIH_MAX},
    {VKD3D_SM1_OP_SLT,          1, 2, VKD3DSIH_SLT},
    {VKD3D_SM1_OP_SGE,          1, 2, VKD3DSIH_SGE},
    {VKD3D_SM1_OP_ABS,          1, 1, VKD3DSIH_ABS},
    {VKD3D_SM1_OP_EXP,          1, 1, VKD3DSIH_EXP},
    {VKD3D_SM1_OP_LOG,          1, 1, VKD3DSIH_LOG},
    {VKD3D_SM1_OP_EXPP,         1, 1, VKD3DSIH_EXPP},
    {VKD3D_SM1_OP_LOGP,         1, 1, VKD3DSIH_LOGP},
    {VKD3D_SM1_OP_LIT,          1, 1, VKD3DSIH_LIT},
    {VKD3D_SM1_OP_DST,          1, 2, VKD3DSIH_DST},
    {VKD3D_SM1_OP_LRP,          1, 3, VKD3DSIH_LRP},
    {VKD3D_SM1_OP_FRC,          1, 1, VKD3DSIH_FRC},
    {VKD3D_SM1_OP_POW,          1, 2, VKD3DSIH_POW},
    {VKD3D_SM1_OP_CRS,          1, 2, VKD3DSIH_CRS},
    {VKD3D_SM1_OP_SGN,          1, 3, VKD3DSIH_SGN,          {2, 0}, {  2,   1}},
    {VKD3D_SM1_OP_SGN,          1, 1, VKD3DSIH_SGN,          {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_NRM,          1, 1, VKD3DSIH_NRM,},
    {VKD3D_SM1_OP_SINCOS,       1, 3, VKD3DSIH_SINCOS,       {2, 0}, {  2,   1}},
    {VKD3D_SM1_OP_SINCOS,       1, 1, VKD3DSIH_SINCOS,       {3, 0}, {~0u, ~0u}},
    /* Matrix */
    {VKD3D_SM1_OP_M4x4,         1, 2, VKD3DSIH_M4x4},
    {VKD3D_SM1_OP_M4x3,         1, 2, VKD3DSIH_M4x3},
    {VKD3D_SM1_OP_M3x4,         1, 2, VKD3DSIH_M3x4},
    {VKD3D_SM1_OP_M3x3,         1, 2, VKD3DSIH_M3x3},
    {VKD3D_SM1_OP_M3x2,         1, 2, VKD3DSIH_M3x2},
    /* Declarations */
    {VKD3D_SM1_OP_DCL,          0, 0, VKD3DSIH_DCL},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 1, VKD3DSIH_DEF},
    {VKD3D_SM1_OP_DEFB,         1, 1, VKD3DSIH_DEFB},
    {VKD3D_SM1_OP_DEFI,         1, 1, VKD3DSIH_DEFI},
    /* Control flow */
    {VKD3D_SM1_OP_REP,          0, 1, VKD3DSIH_REP,          {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDREP,       0, 0, VKD3DSIH_ENDREP,       {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IF,           0, 1, VKD3DSIH_IF,           {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IFC,          0, 2, VKD3DSIH_IFC,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ELSE,         0, 0, VKD3DSIH_ELSE,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDIF,        0, 0, VKD3DSIH_ENDIF,        {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAK,        0, 0, VKD3DSIH_BREAK,        {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKC,       0, 2, VKD3DSIH_BREAKC,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKP,       0, 1, VKD3DSIH_BREAKP},
    {VKD3D_SM1_OP_CALL,         0, 1, VKD3DSIH_CALL,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_CALLNZ,       0, 2, VKD3DSIH_CALLNZ,       {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LOOP,         0, 2, VKD3DSIH_LOOP,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_RET,          0, 0, VKD3DSIH_RET,          {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDLOOP,      0, 0, VKD3DSIH_ENDLOOP,      {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LABEL,        0, 1, VKD3DSIH_LABEL,        {2, 0}, {~0u, ~0u}},

    {VKD3D_SM1_OP_SETP,         1, 2, VKD3DSIH_SETP},
    {VKD3D_SM1_OP_TEXLDL,       1, 2, VKD3DSIH_TEXLDL,       {3, 0}, {~0u, ~0u}},
    {0,                         0, 0, VKD3DSIH_INVALID},
};

static const struct vkd3d_sm1_opcode_info ps_opcode_table[] =
{
    /* Arithmetic */
    {VKD3D_SM1_OP_NOP,          0, 0, VKD3DSIH_NOP},
    {VKD3D_SM1_OP_MOV,          1, 1, VKD3DSIH_MOV},
    {VKD3D_SM1_OP_ADD,          1, 2, VKD3DSIH_ADD},
    {VKD3D_SM1_OP_SUB,          1, 2, VKD3DSIH_SUB},
    {VKD3D_SM1_OP_MAD,          1, 3, VKD3DSIH_MAD},
    {VKD3D_SM1_OP_MUL,          1, 2, VKD3DSIH_MUL},
    {VKD3D_SM1_OP_RCP,          1, 1, VKD3DSIH_RCP},
    {VKD3D_SM1_OP_RSQ,          1, 1, VKD3DSIH_RSQ},
    {VKD3D_SM1_OP_DP3,          1, 2, VKD3DSIH_DP3},
    {VKD3D_SM1_OP_DP4,          1, 2, VKD3DSIH_DP4},
    {VKD3D_SM1_OP_MIN,          1, 2, VKD3DSIH_MIN},
    {VKD3D_SM1_OP_MAX,          1, 2, VKD3DSIH_MAX},
    {VKD3D_SM1_OP_SLT,          1, 2, VKD3DSIH_SLT},
    {VKD3D_SM1_OP_SGE,          1, 2, VKD3DSIH_SGE},
    {VKD3D_SM1_OP_ABS,          1, 1, VKD3DSIH_ABS},
    {VKD3D_SM1_OP_EXP,          1, 1, VKD3DSIH_EXP},
    {VKD3D_SM1_OP_LOG,          1, 1, VKD3DSIH_LOG},
    {VKD3D_SM1_OP_EXPP,         1, 1, VKD3DSIH_EXPP},
    {VKD3D_SM1_OP_LOGP,         1, 1, VKD3DSIH_LOGP},
    {VKD3D_SM1_OP_DST,          1, 2, VKD3DSIH_DST},
    {VKD3D_SM1_OP_LRP,          1, 3, VKD3DSIH_LRP},
    {VKD3D_SM1_OP_FRC,          1, 1, VKD3DSIH_FRC},
    {VKD3D_SM1_OP_CND,          1, 3, VKD3DSIH_CND,          {1, 0}, {  1,   4}},
    {VKD3D_SM1_OP_CMP,          1, 3, VKD3DSIH_CMP,          {1, 2}, {  3,   0}},
    {VKD3D_SM1_OP_POW,          1, 2, VKD3DSIH_POW},
    {VKD3D_SM1_OP_CRS,          1, 2, VKD3DSIH_CRS},
    {VKD3D_SM1_OP_NRM,          1, 1, VKD3DSIH_NRM},
    {VKD3D_SM1_OP_SINCOS,       1, 3, VKD3DSIH_SINCOS,       {2, 0}, {  2,   1}},
    {VKD3D_SM1_OP_SINCOS,       1, 1, VKD3DSIH_SINCOS,       {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_DP2ADD,       1, 3, VKD3DSIH_DP2ADD,       {2, 0}, {~0u, ~0u}},
    /* Matrix */
    {VKD3D_SM1_OP_M4x4,         1, 2, VKD3DSIH_M4x4},
    {VKD3D_SM1_OP_M4x3,         1, 2, VKD3DSIH_M4x3},
    {VKD3D_SM1_OP_M3x4,         1, 2, VKD3DSIH_M3x4},
    {VKD3D_SM1_OP_M3x3,         1, 2, VKD3DSIH_M3x3},
    {VKD3D_SM1_OP_M3x2,         1, 2, VKD3DSIH_M3x2},
    /* Declarations */
    {VKD3D_SM1_OP_DCL,          0, 0, VKD3DSIH_DCL},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 1, VKD3DSIH_DEF},
    {VKD3D_SM1_OP_DEFB,         1, 1, VKD3DSIH_DEFB},
    {VKD3D_SM1_OP_DEFI,         1, 1, VKD3DSIH_DEFI},
    /* Control flow */
    {VKD3D_SM1_OP_REP,          0, 1, VKD3DSIH_REP,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDREP,       0, 0, VKD3DSIH_ENDREP,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IF,           0, 1, VKD3DSIH_IF,           {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IFC,          0, 2, VKD3DSIH_IFC,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ELSE,         0, 0, VKD3DSIH_ELSE,         {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDIF,        0, 0, VKD3DSIH_ENDIF,        {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAK,        0, 0, VKD3DSIH_BREAK,        {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKC,       0, 2, VKD3DSIH_BREAKC,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKP,       0, 1, VKD3DSIH_BREAKP},
    {VKD3D_SM1_OP_CALL,         0, 1, VKD3DSIH_CALL,         {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_CALLNZ,       0, 2, VKD3DSIH_CALLNZ,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LOOP,         0, 2, VKD3DSIH_LOOP,         {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_RET,          0, 0, VKD3DSIH_RET,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDLOOP,      0, 0, VKD3DSIH_ENDLOOP,      {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LABEL,        0, 1, VKD3DSIH_LABEL,        {2, 1}, {~0u, ~0u}},
    /* Texture */
    {VKD3D_SM1_OP_TEXCOORD,     1, 0, VKD3DSIH_TEXCOORD,     {0, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXCOORD,     1, 1, VKD3DSIH_TEXCOORD,     {1 ,4}, {  1,   4}},
    {VKD3D_SM1_OP_TEXKILL,      1, 0, VKD3DSIH_TEXKILL,      {1 ,0}, {  3,   0}},
    {VKD3D_SM1_OP_TEX,          1, 0, VKD3DSIH_TEX,          {0, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEX,          1, 1, VKD3DSIH_TEX,          {1, 4}, {  1,   4}},
    {VKD3D_SM1_OP_TEX,          1, 2, VKD3DSIH_TEX,          {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_TEXBEM,       1, 1, VKD3DSIH_TEXBEM,       {0, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXBEML,      1, 1, VKD3DSIH_TEXBEML,      {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXREG2AR,    1, 1, VKD3DSIH_TEXREG2AR,    {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXREG2GB,    1, 1, VKD3DSIH_TEXREG2GB,    {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXREG2RGB,   1, 1, VKD3DSIH_TEXREG2RGB,   {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x2PAD,   1, 1, VKD3DSIH_TEXM3x2PAD,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x2TEX,   1, 1, VKD3DSIH_TEXM3x2TEX,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3PAD,   1, 1, VKD3DSIH_TEXM3x3PAD,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3DIFF,  1, 1, VKD3DSIH_TEXM3x3DIFF,  {0, 0}, {  0,   0}},
    {VKD3D_SM1_OP_TEXM3x3SPEC,  1, 2, VKD3DSIH_TEXM3x3SPEC,  {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3VSPEC, 1, 1, VKD3DSIH_TEXM3x3VSPEC, {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3TEX,   1, 1, VKD3DSIH_TEXM3x3TEX,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXDP3TEX,    1, 1, VKD3DSIH_TEXDP3TEX,    {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x2DEPTH, 1, 1, VKD3DSIH_TEXM3x2DEPTH, {1, 3}, {  1,   3}},
    {VKD3D_SM1_OP_TEXDP3,       1, 1, VKD3DSIH_TEXDP3,       {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3,      1, 1, VKD3DSIH_TEXM3x3,      {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXDEPTH,     1, 0, VKD3DSIH_TEXDEPTH,     {1, 4}, {  1,   4}},
    {VKD3D_SM1_OP_BEM,          1, 2, VKD3DSIH_BEM,          {1, 4}, {  1,   4}},
    {VKD3D_SM1_OP_DSX,          1, 1, VKD3DSIH_DSX,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_DSY,          1, 1, VKD3DSIH_DSY,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_TEXLDD,       1, 4, VKD3DSIH_TEXLDD,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_SETP,         1, 2, VKD3DSIH_SETP},
    {VKD3D_SM1_OP_TEXLDL,       1, 2, VKD3DSIH_TEXLDL,       {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_PHASE,        0, 0, VKD3DSIH_PHASE},
    {0,                         0, 0, VKD3DSIH_INVALID},
};

static const enum vkd3d_shader_resource_type resource_type_table[] =
{
    /* VKD3D_SM1_RESOURCE_UNKNOWN */      VKD3D_SHADER_RESOURCE_NONE,
    /* VKD3D_SM1_RESOURCE_TEXTURE_1D */   VKD3D_SHADER_RESOURCE_TEXTURE_1D,
    /* VKD3D_SM1_RESOURCE_TEXTURE_2D */   VKD3D_SHADER_RESOURCE_TEXTURE_2D,
    /* VKD3D_SM1_RESOURCE_TEXTURE_CUBE */ VKD3D_SHADER_RESOURCE_TEXTURE_CUBE,
    /* VKD3D_SM1_RESOURCE_TEXTURE_3D */   VKD3D_SHADER_RESOURCE_TEXTURE_3D,
};

static uint32_t read_u32(const uint32_t **ptr)
{
    return *(*ptr)++;
}

static bool has_relative_address(uint32_t param)
{
    enum vkd3d_sm1_address_mode_type address_mode;

    address_mode = (param & VKD3D_SM1_ADDRESS_MODE_MASK) >> VKD3D_SM1_ADDRESS_MODE_SHIFT;

    return address_mode == VKD3D_SM1_ADDRESS_MODE_RELATIVE;
}

static const struct vkd3d_sm1_opcode_info *shader_sm1_get_opcode_info(
        const struct vkd3d_shader_sm1_parser *sm1, enum vkd3d_sm1_opcode opcode)
{
    const struct vkd3d_shader_version *version = &sm1->p.program->shader_version;
    const struct vkd3d_sm1_opcode_info *info;
    unsigned int i = 0;

    for (;;)
    {
        info = &sm1->opcode_table[i++];
        if (info->vkd3d_opcode == VKD3DSIH_INVALID)
            return NULL;

        if (opcode == info->sm1_opcode
                && vkd3d_shader_ver_ge(version, info->min_version.major, info->min_version.minor)
                && (vkd3d_shader_ver_le(version, info->max_version.major, info->max_version.minor)
                        || !info->max_version.major))
            return info;
    }
}

static unsigned int shader_sm1_get_swizzle_component(uint32_t swizzle, unsigned int idx)
{
    return (swizzle & VKD3D_SM1_SWIZZLE_COMPONENT_MASK(idx)) >> VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(idx);
}

static uint32_t swizzle_from_sm1(uint32_t swizzle)
{
    return vkd3d_shader_create_swizzle(shader_sm1_get_swizzle_component(swizzle, 0),
            shader_sm1_get_swizzle_component(swizzle, 1),
            shader_sm1_get_swizzle_component(swizzle, 2),
            shader_sm1_get_swizzle_component(swizzle, 3));
}

/* D3DBC doesn't have the concept of index count. All registers implicitly have
 * exactly one index. However for some register types the index doesn't make
 * sense, so we remove it. */
static unsigned int idx_count_from_reg_type(enum vkd3d_shader_register_type reg_type)
{
    switch (reg_type)
    {
        case VKD3DSPR_DEPTHOUT:
            return 0;

        default:
            return 1;
    }
}

static void shader_sm1_parse_src_param(uint32_t param, struct vkd3d_shader_src_param *rel_addr,
        struct vkd3d_shader_src_param *src)
{
    enum vkd3d_shader_register_type reg_type = ((param & VKD3D_SM1_REGISTER_TYPE_MASK) >> VKD3D_SM1_REGISTER_TYPE_SHIFT)
            | ((param & VKD3D_SM1_REGISTER_TYPE_MASK2) >> VKD3D_SM1_REGISTER_TYPE_SHIFT2);
    unsigned int idx_count = idx_count_from_reg_type(reg_type);

    vsir_register_init(&src->reg, reg_type, VKD3D_DATA_FLOAT, idx_count);
    src->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    src->reg.non_uniform = false;
    if (idx_count == 1)
    {
        src->reg.idx[0].offset = param & VKD3D_SM1_REGISTER_NUMBER_MASK;
        src->reg.idx[0].rel_addr = rel_addr;
    }
    if (src->reg.type == VKD3DSPR_SAMPLER)
        src->reg.dimension = VSIR_DIMENSION_NONE;
    else if (src->reg.type == VKD3DSPR_DEPTHOUT)
        src->reg.dimension = VSIR_DIMENSION_SCALAR;
    else
        src->reg.dimension = VSIR_DIMENSION_VEC4;
    src->swizzle = swizzle_from_sm1((param & VKD3D_SM1_SWIZZLE_MASK) >> VKD3D_SM1_SWIZZLE_SHIFT);
    src->modifiers = (param & VKD3D_SM1_SRC_MODIFIER_MASK) >> VKD3D_SM1_SRC_MODIFIER_SHIFT;
}

static void shader_sm1_parse_dst_param(uint32_t param, struct vkd3d_shader_src_param *rel_addr,
        struct vkd3d_shader_dst_param *dst)
{
    enum vkd3d_shader_register_type reg_type = ((param & VKD3D_SM1_REGISTER_TYPE_MASK) >> VKD3D_SM1_REGISTER_TYPE_SHIFT)
            | ((param & VKD3D_SM1_REGISTER_TYPE_MASK2) >> VKD3D_SM1_REGISTER_TYPE_SHIFT2);
    unsigned int idx_count = idx_count_from_reg_type(reg_type);

    vsir_register_init(&dst->reg, reg_type, VKD3D_DATA_FLOAT, idx_count);
    dst->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    dst->reg.non_uniform = false;
    if (idx_count == 1)
    {
        dst->reg.idx[0].offset = param & VKD3D_SM1_REGISTER_NUMBER_MASK;
        dst->reg.idx[0].rel_addr = rel_addr;
    }
    if (dst->reg.type == VKD3DSPR_SAMPLER)
        dst->reg.dimension = VSIR_DIMENSION_NONE;
    else if (dst->reg.type == VKD3DSPR_DEPTHOUT)
        dst->reg.dimension = VSIR_DIMENSION_SCALAR;
    else
        dst->reg.dimension = VSIR_DIMENSION_VEC4;
    dst->modifiers = (param & VKD3D_SM1_DST_MODIFIER_MASK) >> VKD3D_SM1_DST_MODIFIER_SHIFT;
    dst->shift = (param & VKD3D_SM1_DSTSHIFT_MASK) >> VKD3D_SM1_DSTSHIFT_SHIFT;

    switch (dst->reg.dimension)
    {
        case VSIR_DIMENSION_SCALAR:
            dst->write_mask = VKD3DSP_WRITEMASK_0;
            break;

        case VSIR_DIMENSION_VEC4:
            dst->write_mask = (param & VKD3D_SM1_WRITEMASK_MASK) >> VKD3D_SM1_WRITEMASK_SHIFT;
            break;

        default:
            dst->write_mask = 0;
            break;
    }
}

static struct signature_element *find_signature_element(const struct shader_signature *signature,
        const char *semantic_name, unsigned int semantic_index)
{
    struct signature_element *e = signature->elements;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        if (!ascii_strcasecmp(e[i].semantic_name, semantic_name)
                && e[i].semantic_index == semantic_index)
            return &e[i];
    }

    return NULL;
}

static struct signature_element *find_signature_element_by_register_index(
        const struct shader_signature *signature, unsigned int register_index)
{
    struct signature_element *e = signature->elements;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        if (e[i].register_index == register_index)
            return &e[i];
    }

    return NULL;
}

/* Add missing bits to a mask to make it contiguous. */
static unsigned int make_mask_contiguous(unsigned int mask)
{
    static const unsigned int table[] =
    {
        0x0, 0x1, 0x2, 0x3,
        0x4, 0x7, 0x6, 0x7,
        0x8, 0xf, 0xe, 0xf,
        0xc, 0xf, 0xe, 0xf,
    };

    VKD3D_ASSERT(mask < ARRAY_SIZE(table));
    return table[mask];
}

static bool add_signature_element(struct vkd3d_shader_sm1_parser *sm1, bool output,
        const char *name, unsigned int index, enum vkd3d_shader_sysval_semantic sysval,
        unsigned int register_index, bool is_dcl, unsigned int mask)
{
    struct vsir_program *program = sm1->p.program;
    struct shader_signature *signature;
    struct signature_element *element;

    if (output)
        signature = &program->output_signature;
    else
        signature = &program->input_signature;

    if ((element = find_signature_element(signature, name, index)))
    {
        element->mask = make_mask_contiguous(element->mask | mask);
        if (!is_dcl)
            element->used_mask |= mask;
        return true;
    }

    if (!vkd3d_array_reserve((void **)&signature->elements, &signature->elements_capacity,
            signature->element_count + 1, sizeof(*signature->elements)))
        return false;
    element = &signature->elements[signature->element_count++];

    memset(element, 0, sizeof(*element));
    if (!(element->semantic_name = vkd3d_strdup(name)))
        return false;
    element->semantic_index = index;
    element->sysval_semantic = sysval;
    element->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    element->register_index = register_index;
    element->target_location = register_index;
    element->register_count = 1;
    element->mask = make_mask_contiguous(mask);
    element->used_mask = is_dcl ? 0 : mask;
    if (program->shader_version.type == VKD3D_SHADER_TYPE_PIXEL && !output)
        element->interpolation_mode = VKD3DSIM_LINEAR;

    return true;
}

static void add_signature_mask(struct vkd3d_shader_sm1_parser *sm1, bool output,
        unsigned int register_index, unsigned int mask)
{
    struct vsir_program *program = sm1->p.program;
    struct shader_signature *signature;
    struct signature_element *element;

    if (output)
        signature = &program->output_signature;
    else
        signature = &program->input_signature;

    if (!(element = find_signature_element_by_register_index(signature, register_index)))
    {
        vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNDECLARED_SEMANTIC,
                "%s register %u was used without being declared.", output ? "Output" : "Input", register_index);
        return;
    }

    element->used_mask |= mask;
}

static bool add_signature_element_from_register(struct vkd3d_shader_sm1_parser *sm1,
        const struct vkd3d_shader_register *reg, bool is_dcl, unsigned int mask)
{
    const struct vkd3d_shader_version *version = &sm1->p.program->shader_version;
    unsigned int register_index = reg->idx_count > 0 ? reg->idx[0].offset : 0;

    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            if (version->type == VKD3D_SHADER_TYPE_PIXEL && version->major == 1 && !register_index)
                return add_signature_element(sm1, true, "COLOR", 0, VKD3D_SHADER_SV_TARGET, 0, is_dcl, mask);
            return true;

        case VKD3DSPR_INPUT:
            /* For vertex shaders or sm3 pixel shaders, we should have already
             * had a DCL instruction. Otherwise, this is a colour input. */
            if (version->type == VKD3D_SHADER_TYPE_VERTEX || version->major == 3)
            {
                add_signature_mask(sm1, false, register_index, mask);
                return true;
            }
            return add_signature_element(sm1, false, "COLOR", register_index,
                    VKD3D_SHADER_SV_NONE, SM1_COLOR_REGISTER_OFFSET + register_index, is_dcl, mask);

        case VKD3DSPR_TEXTURE:
            /* For vertex shaders, this is ADDR. */
            if (version->type == VKD3D_SHADER_TYPE_VERTEX)
                return true;
            return add_signature_element(sm1, false, "TEXCOORD", register_index,
                    VKD3D_SHADER_SV_NONE, register_index, is_dcl, mask);

        case VKD3DSPR_OUTPUT:
            if (version->type == VKD3D_SHADER_TYPE_VERTEX)
            {
                /* For sm < 2 vertex shaders, this is TEXCRDOUT.
                 *
                 * For sm3 vertex shaders, this is OUTPUT, but we already
                 * should have had a DCL instruction. */
                if (version->major == 3)
                {
                    add_signature_mask(sm1, true, register_index, mask);
                    return true;
                }
                return add_signature_element(sm1, true, "TEXCOORD", register_index,
                        VKD3D_SHADER_SV_NONE, register_index, is_dcl, mask);
            }
            /* fall through */

        case VKD3DSPR_ATTROUT:
            return add_signature_element(sm1, true, "COLOR", register_index,
                    VKD3D_SHADER_SV_NONE, SM1_COLOR_REGISTER_OFFSET + register_index, is_dcl, mask);

        case VKD3DSPR_COLOROUT:
            return add_signature_element(sm1, true, "COLOR", register_index,
                    VKD3D_SHADER_SV_TARGET, register_index, is_dcl, mask);

        case VKD3DSPR_DEPTHOUT:
            return add_signature_element(sm1, true, "DEPTH", 0,
                    VKD3D_SHADER_SV_DEPTH, register_index, is_dcl, 0x1);

        case VKD3DSPR_RASTOUT:
            switch (register_index)
            {
                case 0:
                    return add_signature_element(sm1, true, "POSITION", 0,
                            VKD3D_SHADER_SV_POSITION, SM1_RASTOUT_REGISTER_OFFSET + register_index, is_dcl, mask);

                case 1:
                    return add_signature_element(sm1, true, "FOG", 0,
                            VKD3D_SHADER_SV_NONE, SM1_RASTOUT_REGISTER_OFFSET + register_index, is_dcl, 0x1);

                case 2:
                    return add_signature_element(sm1, true, "PSIZE", 0,
                            VKD3D_SHADER_SV_NONE, SM1_RASTOUT_REGISTER_OFFSET + register_index, is_dcl, 0x1);

                default:
                    vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_INDEX,
                            "Invalid rasterizer output index %u.", register_index);
                    return true;
            }

        case VKD3DSPR_MISCTYPE:
            switch (register_index)
            {
                case 0:
                    return add_signature_element(sm1, false, "VPOS", 0,
                            VKD3D_SHADER_SV_POSITION, register_index, is_dcl, mask);

                case 1:
                    return add_signature_element(sm1, false, "VFACE", 0,
                            VKD3D_SHADER_SV_IS_FRONT_FACE, register_index, is_dcl, 0x1);

                default:
                    vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_INDEX,
                            "Invalid miscellaneous fragment input index %u.", register_index);
                    return true;
            }

        default:
            return true;
    }
}

static bool add_signature_element_from_semantic(struct vkd3d_shader_sm1_parser *sm1,
        const struct vkd3d_shader_semantic *semantic)
{
    const struct vkd3d_shader_version *version = &sm1->p.program->shader_version;
    const struct vkd3d_shader_register *reg = &semantic->resource.reg.reg;
    enum vkd3d_shader_sysval_semantic sysval = VKD3D_SHADER_SV_NONE;
    unsigned int mask = semantic->resource.reg.write_mask;
    bool output;

    static const char sm1_semantic_names[][13] =
    {
        [VKD3D_DECL_USAGE_POSITION     ] = "POSITION",
        [VKD3D_DECL_USAGE_BLEND_WEIGHT ] = "BLENDWEIGHT",
        [VKD3D_DECL_USAGE_BLEND_INDICES] = "BLENDINDICES",
        [VKD3D_DECL_USAGE_NORMAL       ] = "NORMAL",
        [VKD3D_DECL_USAGE_PSIZE        ] = "PSIZE",
        [VKD3D_DECL_USAGE_TEXCOORD     ] = "TEXCOORD",
        [VKD3D_DECL_USAGE_TANGENT      ] = "TANGENT",
        [VKD3D_DECL_USAGE_BINORMAL     ] = "BINORMAL",
        [VKD3D_DECL_USAGE_TESS_FACTOR  ] = "TESSFACTOR",
        [VKD3D_DECL_USAGE_POSITIONT    ] = "POSITIONT",
        [VKD3D_DECL_USAGE_COLOR        ] = "COLOR",
        [VKD3D_DECL_USAGE_FOG          ] = "FOG",
        [VKD3D_DECL_USAGE_DEPTH        ] = "DEPTH",
        [VKD3D_DECL_USAGE_SAMPLE       ] = "SAMPLE",
    };

    if (reg->type == VKD3DSPR_OUTPUT)
        output = true;
    else if (reg->type == VKD3DSPR_INPUT || reg->type == VKD3DSPR_TEXTURE)
        output = false;
    else /* vpos and vface don't have a semantic. */
        return add_signature_element_from_register(sm1, reg, true, mask);

    /* sm2 pixel shaders use DCL but don't provide a semantic. */
    if (version->type == VKD3D_SHADER_TYPE_PIXEL && version->major == 2)
        return add_signature_element_from_register(sm1, reg, true, mask);

    /* With the exception of vertex POSITION output, none of these are system
     * values. Pixel POSITION input is not equivalent to SV_Position; the closer
     * equivalent is VPOS, which is not declared as a semantic. */
    if (version->type == VKD3D_SHADER_TYPE_VERTEX
            && output && semantic->usage == VKD3D_DECL_USAGE_POSITION)
        sysval = VKD3D_SHADER_SV_POSITION;

    return add_signature_element(sm1, output, sm1_semantic_names[semantic->usage],
            semantic->usage_idx, sysval, reg->idx[0].offset, true, mask);
}

static void record_constant_register(struct vkd3d_shader_sm1_parser *sm1,
        enum vkd3d_shader_d3dbc_constant_register set, uint32_t index, bool from_def)
{
    sm1->constants[set].count = max(sm1->constants[set].count, index + 1);
    if (from_def)
    {
        /* d3d shaders have a maximum of 8192 constants; we should not overrun
         * this array. */
        VKD3D_ASSERT((index / 32) <= ARRAY_SIZE(sm1->constants[set].def_mask));
        bitmap_set(sm1->constants[set].def_mask, index);
    }
}

static void shader_sm1_scan_register(struct vkd3d_shader_sm1_parser *sm1,
        const struct vkd3d_shader_register *reg, unsigned int mask, bool from_def)
{
    struct vsir_program *program = sm1->p.program;
    uint32_t register_index = reg->idx[0].offset;

    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            program->temp_count = max(program->temp_count, register_index + 1);
            break;

        case VKD3DSPR_CONST:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, register_index, from_def);
            break;

        case VKD3DSPR_CONST2:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 2048 + register_index, from_def);
            break;

        case VKD3DSPR_CONST3:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 4096 + register_index, from_def);
            break;

        case VKD3DSPR_CONST4:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 6144 + register_index, from_def);
            break;

        case VKD3DSPR_CONSTINT:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER, register_index, from_def);
            break;

        case VKD3DSPR_CONSTBOOL:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER, register_index, from_def);
            break;

        default:
            break;
    }

    add_signature_element_from_register(sm1, reg, false, mask);
}

/* Read a parameter token from the input stream, and possibly a relative
 * addressing token. */
static void shader_sm1_read_param(struct vkd3d_shader_sm1_parser *sm1,
        const uint32_t **ptr, uint32_t *token, uint32_t *addr_token)
{
    if (*ptr >= sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a parameter token, but no more tokens are remaining.");
        sm1->abort = true;
        *token = 0;
        return;
    }
    *token = read_u32(ptr);
    if (!has_relative_address(*token))
        return;

    /* PS >= 3.0 have relative addressing (with token)
     * VS >= 2.0 have relative addressing (with token)
     * VS >= 1.0 < 2.0 have relative addressing (without token)
     * The version check below should work in general. */
    if (sm1->p.program->shader_version.major < 2)
    {
        *addr_token = (1u << 31)
                | ((VKD3DSPR_ADDR << VKD3D_SM1_REGISTER_TYPE_SHIFT2) & VKD3D_SM1_REGISTER_TYPE_MASK2)
                | ((VKD3DSPR_ADDR << VKD3D_SM1_REGISTER_TYPE_SHIFT) & VKD3D_SM1_REGISTER_TYPE_MASK)
                | (VKD3D_SM1_SWIZZLE_DEFAULT << VKD3D_SM1_SWIZZLE_SHIFT);
        return;
    }

    if (*ptr >= sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read an indirect addressing token, but no more tokens are remaining.");
        sm1->abort = true;
        *addr_token = 0;
        return;
    }
    *addr_token = read_u32(ptr);
}

/* Skip the parameter tokens for an opcode. */
static void shader_sm1_skip_opcode(const struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        const struct vkd3d_sm1_opcode_info *opcode_info, uint32_t opcode_token)
{
    unsigned int length;

    /* Version 2.0+ shaders may contain address tokens, but fortunately they
     * have a useful length mask - use it here. Version 1.x shaders contain no
     * such tokens. */
    if (sm1->p.program->shader_version.major >= 2)
    {
        length = (opcode_token & VKD3D_SM1_INSTRUCTION_LENGTH_MASK) >> VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT;
        *ptr += length;
        return;
    }

    /* DCL instructions do not have sources or destinations, but they
     * read two tokens to a semantic. See
     * shader_sm1_read_semantic(). */
    if (opcode_info->vkd3d_opcode == VKD3DSIH_DCL)
    {
        *ptr += 2;
    }
    /* Somewhat similarly, DEF and DEFI have a single source, but need to read
     * four tokens for that source. See shader_sm1_read_immconst().
     * Technically shader model 1 doesn't have integer registers or DEFI; we
     * handle it here anyway because it's easy. */
    else if (opcode_info->vkd3d_opcode == VKD3DSIH_DEF || opcode_info->vkd3d_opcode == VKD3DSIH_DEFI)
    {
        *ptr += 3;
    }

    *ptr += (opcode_info->dst_count + opcode_info->src_count);
}

static void shader_sm1_read_src_param(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_src_param *src_param)
{
    struct vkd3d_shader_src_param *src_rel_addr = NULL;
    uint32_t token, addr_token;

    shader_sm1_read_param(sm1, ptr, &token, &addr_token);
    if (has_relative_address(token))
    {
        if (!(src_rel_addr = vsir_program_get_src_params(sm1->p.program, 1)))
        {
            vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY,
                    "Out of memory.");
            sm1->abort = true;
            return;
        }
        shader_sm1_parse_src_param(addr_token, NULL, src_rel_addr);
    }
    shader_sm1_parse_src_param(token, src_rel_addr, src_param);
}

static void shader_sm1_read_dst_param(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_dst_param *dst_param)
{
    struct vkd3d_shader_src_param *dst_rel_addr = NULL;
    uint32_t token, addr_token;

    shader_sm1_read_param(sm1, ptr, &token, &addr_token);
    if (has_relative_address(token))
    {
        if (!(dst_rel_addr = vsir_program_get_src_params(sm1->p.program, 1)))
        {
            vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY,
                    "Out of memory.");
            sm1->abort = true;
            return;
        }
        shader_sm1_parse_src_param(addr_token, NULL, dst_rel_addr);
    }
    shader_sm1_parse_dst_param(token, dst_rel_addr, dst_param);

    if (dst_param->reg.type == VKD3DSPR_RASTOUT && dst_param->reg.idx[0].offset == VSIR_RASTOUT_POINT_SIZE)
        sm1->p.program->has_point_size = true;
}

static void shader_sm1_read_semantic(struct vkd3d_shader_sm1_parser *sm1,
        const uint32_t **ptr, struct vkd3d_shader_semantic *semantic)
{
    enum vkd3d_sm1_resource_type resource_type;
    struct vkd3d_shader_register_range *range;
    uint32_t usage_token, dst_token;

    if (*ptr >= sm1->end || sm1->end - *ptr < 2)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a declaration instruction, but not enough tokens are remaining.");
        sm1->abort = true;
        return;
    }

    usage_token = read_u32(ptr);
    dst_token = read_u32(ptr);

    semantic->usage = (usage_token & VKD3D_SM1_DCL_USAGE_MASK) >> VKD3D_SM1_DCL_USAGE_SHIFT;
    semantic->usage_idx = (usage_token & VKD3D_SM1_DCL_USAGE_INDEX_MASK) >> VKD3D_SM1_DCL_USAGE_INDEX_SHIFT;
    resource_type = (usage_token & VKD3D_SM1_RESOURCE_TYPE_MASK) >> VKD3D_SM1_RESOURCE_TYPE_SHIFT;
    if (resource_type >= ARRAY_SIZE(resource_type_table))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_RESOURCE_TYPE,
                "Invalid resource type %#x.", resource_type);
        semantic->resource_type = VKD3D_SHADER_RESOURCE_NONE;
    }
    else
    {
        semantic->resource_type = resource_type_table[resource_type];
    }
    semantic->resource_data_type[0] = VKD3D_DATA_FLOAT;
    semantic->resource_data_type[1] = VKD3D_DATA_FLOAT;
    semantic->resource_data_type[2] = VKD3D_DATA_FLOAT;
    semantic->resource_data_type[3] = VKD3D_DATA_FLOAT;
    shader_sm1_parse_dst_param(dst_token, NULL, &semantic->resource.reg);
    range = &semantic->resource.range;
    range->space = 0;
    range->first = range->last = semantic->resource.reg.reg.idx[0].offset;

    add_signature_element_from_semantic(sm1, semantic);
}

static void shader_sm1_read_immconst(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_src_param *src_param, enum vsir_dimension dimension, enum vkd3d_data_type data_type)
{
    unsigned int count = dimension == VSIR_DIMENSION_VEC4 ? 4 : 1;

    if (*ptr >= sm1->end || sm1->end - *ptr < count)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a constant definition, but not enough tokens are remaining. "
                "%zu token(s) available, %u required.", sm1->end - *ptr, count);
        sm1->abort = true;
        return;
    }

    src_param->reg.type = VKD3DSPR_IMMCONST;
    src_param->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    src_param->reg.non_uniform = false;
    src_param->reg.data_type = data_type;
    src_param->reg.idx[0].offset = ~0u;
    src_param->reg.idx[0].rel_addr = NULL;
    src_param->reg.idx[1].offset = ~0u;
    src_param->reg.idx[1].rel_addr = NULL;
    src_param->reg.idx[2].offset = ~0u;
    src_param->reg.idx[2].rel_addr = NULL;
    src_param->reg.idx_count = 0;
    src_param->reg.dimension = dimension;
    memcpy(src_param->reg.u.immconst_u32, *ptr, count * sizeof(uint32_t));
    src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    src_param->modifiers = 0;

    *ptr += count;
}

static void shader_sm1_read_comment(struct vkd3d_shader_sm1_parser *sm1)
{
    const uint32_t **ptr = &sm1->ptr;
    const char *comment;
    unsigned int size;
    size_t remaining;
    uint32_t token;

    if (*ptr >= sm1->end)
        return;

    remaining = sm1->end - *ptr;

    token = **ptr;
    while ((token & VKD3D_SM1_OPCODE_MASK) == VKD3D_SM1_OP_COMMENT)
    {
        size = (token & VKD3D_SM1_COMMENT_SIZE_MASK) >> VKD3D_SM1_COMMENT_SIZE_SHIFT;

        if (size > --remaining)
        {
            vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                    "Encountered a %u token comment, but only %zu token(s) is/are remaining.",
                    size, remaining);
            return;
        }

        comment = (const char *)++(*ptr);
        remaining -= size;
        *ptr += size;

        if (size > 1 && *(const uint32_t *)comment == TAG_TEXT)
        {
            const char *end = comment + size * sizeof(token);
            const char *p = comment + sizeof(token);
            const char *line;

            TRACE("// TEXT\n");
            for (line = p; line < end; line = p)
            {
                if (!(p = memchr(line, '\n', end - line)))
                    p = end;
                else
                    ++p;
                TRACE("// %s\n", debugstr_an(line, p - line));
            }
        }
        else if (size)
        {
            TRACE("// %s\n", debugstr_an(comment, size * sizeof(token)));
        }
        else
            break;

        if (!remaining)
            break;
        token = **ptr;
    }
}

static void shader_sm1_validate_instruction(struct vkd3d_shader_sm1_parser *sm1, struct vkd3d_shader_instruction *ins)
{
    if ((ins->opcode == VKD3DSIH_BREAKP || ins->opcode == VKD3DSIH_IF) && ins->flags)
    {
        vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_WARNING_D3DBC_IGNORED_INSTRUCTION_FLAGS,
                "Ignoring unexpected instruction flags %#x.", ins->flags);
        ins->flags = 0;
    }
}

static unsigned int mask_from_swizzle(uint32_t swizzle)
{
    return (1u << vsir_swizzle_get_component(swizzle, 0))
            | (1u << vsir_swizzle_get_component(swizzle, 1))
            | (1u << vsir_swizzle_get_component(swizzle, 2))
            | (1u << vsir_swizzle_get_component(swizzle, 3));
}

static void shader_sm1_read_instruction(struct vkd3d_shader_sm1_parser *sm1, struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_src_param *src_params, *predicate;
    const struct vkd3d_sm1_opcode_info *opcode_info;
    struct vsir_program *program = sm1->p.program;
    struct vkd3d_shader_dst_param *dst_param;
    const uint32_t **ptr = &sm1->ptr;
    uint32_t opcode_token;
    const uint32_t *p;
    bool predicated;
    unsigned int i;

    shader_sm1_read_comment(sm1);

    if (*ptr >= sm1->end)
    {
        WARN("End of byte-code, failed to read opcode.\n");
        goto fail;
    }

    ++sm1->p.location.line;
    opcode_token = read_u32(ptr);
    if (!(opcode_info = shader_sm1_get_opcode_info(sm1, opcode_token & VKD3D_SM1_OPCODE_MASK)))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                "Invalid opcode %#x (token 0x%08x, shader version %u.%u).",
                opcode_token & VKD3D_SM1_OPCODE_MASK, opcode_token,
                program->shader_version.major, program->shader_version.minor);
        goto fail;
    }

    vsir_instruction_init(ins, &sm1->p.location, opcode_info->vkd3d_opcode);
    ins->flags = (opcode_token & VKD3D_SM1_INSTRUCTION_FLAGS_MASK) >> VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT;
    ins->coissue = opcode_token & VKD3D_SM1_COISSUE;
    ins->raw = false;
    ins->structured = false;
    predicated = !!(opcode_token & VKD3D_SM1_INSTRUCTION_PREDICATED);
    ins->predicate = predicate = predicated ? vsir_program_get_src_params(program, 1) : NULL;
    ins->dst_count = opcode_info->dst_count;
    ins->dst = dst_param = vsir_program_get_dst_params(program, ins->dst_count);
    ins->src_count = opcode_info->src_count;
    ins->src = src_params = vsir_program_get_src_params(program, ins->src_count);
    if ((!predicate && predicated) || (!src_params && ins->src_count) || (!dst_param && ins->dst_count))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY, "Out of memory.");
        goto fail;
    }

    ins->resource_type = VKD3D_SHADER_RESOURCE_NONE;
    ins->resource_stride = 0;
    ins->resource_data_type[0] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[1] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[2] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[3] = VKD3D_DATA_FLOAT;
    memset(&ins->texel_offset, 0, sizeof(ins->texel_offset));

    p = *ptr;
    shader_sm1_skip_opcode(sm1, ptr, opcode_info, opcode_token);
    if (*ptr > sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "The current instruction ends %zu token(s) past the end of the shader.",
                *ptr - sm1->end);
        goto fail;
    }

    if (ins->opcode == VKD3DSIH_DCL)
    {
        shader_sm1_read_semantic(sm1, &p, &ins->declaration.semantic);
    }
    else if (ins->opcode == VKD3DSIH_DEF)
    {
        shader_sm1_read_dst_param(sm1, &p, dst_param);
        shader_sm1_read_immconst(sm1, &p, &src_params[0], VSIR_DIMENSION_VEC4, VKD3D_DATA_FLOAT);
        shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, true);
    }
    else if (ins->opcode == VKD3DSIH_DEFB)
    {
        shader_sm1_read_dst_param(sm1, &p, dst_param);
        shader_sm1_read_immconst(sm1, &p, &src_params[0], VSIR_DIMENSION_SCALAR, VKD3D_DATA_UINT);
        shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, true);
    }
    else if (ins->opcode == VKD3DSIH_DEFI)
    {
        shader_sm1_read_dst_param(sm1, &p, dst_param);
        shader_sm1_read_immconst(sm1, &p, &src_params[0], VSIR_DIMENSION_VEC4, VKD3D_DATA_INT);
        shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, true);
    }
    else
    {
        /* Destination token */
        if (ins->dst_count)
        {
            shader_sm1_read_dst_param(sm1, &p, dst_param);
            shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, false);
        }

        /* Predication token */
        if (ins->predicate)
            shader_sm1_read_src_param(sm1, &p, predicate);

        /* Other source tokens */
        for (i = 0; i < ins->src_count; ++i)
        {
            shader_sm1_read_src_param(sm1, &p, &src_params[i]);
            shader_sm1_scan_register(sm1, &src_params[i].reg, mask_from_swizzle(src_params[i].swizzle), false);
        }
    }

    if (sm1->abort)
    {
        sm1->abort = false;
        goto fail;
    }

    shader_sm1_validate_instruction(sm1, ins);
    return;

fail:
    ins->opcode = VKD3DSIH_INVALID;
    *ptr = sm1->end;
}

static bool shader_sm1_is_end(struct vkd3d_shader_sm1_parser *sm1)
{
    const uint32_t **ptr = &sm1->ptr;

    shader_sm1_read_comment(sm1);

    if (*ptr >= sm1->end)
        return true;

    if (**ptr == VKD3D_SM1_END)
    {
        ++(*ptr);
        return true;
    }

    return false;
}

static enum vkd3d_result shader_sm1_init(struct vkd3d_shader_sm1_parser *sm1, struct vsir_program *program,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_message_context *message_context)
{
    const struct vkd3d_shader_location location = {.source_name = compile_info->source_name};
    const uint32_t *code = compile_info->source.code;
    size_t code_size = compile_info->source.size;
    struct vkd3d_shader_version version;
    uint16_t shader_type;
    size_t token_count;

    token_count = code_size / sizeof(*sm1->start);

    if (token_count < 2)
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Invalid shader size %zu (token count %zu). At least 2 tokens are required.",
                code_size, token_count);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    TRACE("Version: 0x%08x.\n", code[0]);

    shader_type = code[0] >> 16;
    version.major = VKD3D_SM1_VERSION_MAJOR(code[0]);
    version.minor = VKD3D_SM1_VERSION_MINOR(code[0]);

    switch (shader_type)
    {
        case VKD3D_SM1_VS:
            version.type = VKD3D_SHADER_TYPE_VERTEX;
            sm1->opcode_table = vs_opcode_table;
            break;

        case VKD3D_SM1_PS:
            version.type = VKD3D_SHADER_TYPE_PIXEL;
            sm1->opcode_table = ps_opcode_table;
            break;

        default:
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_D3DBC_INVALID_VERSION_TOKEN,
                    "Invalid shader type %#x (token 0x%08x).", shader_type, code[0]);
            return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!vkd3d_shader_ver_le(&version, 3, 0))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_D3DBC_INVALID_VERSION_TOKEN,
                "Invalid shader version %u.%u (token 0x%08x).", version.major, version.minor, code[0]);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    sm1->start = &code[1];
    sm1->end = &code[token_count];

    /* Estimate instruction count to avoid reallocation in most shaders. */
    if (!vsir_program_init(program, compile_info, &version,
            code_size != ~(size_t)0 ? token_count / 4u + 4 : 16, VSIR_CF_STRUCTURED, VSIR_NOT_NORMALISED))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    vkd3d_shader_parser_init(&sm1->p, program, message_context, compile_info->source_name);
    sm1->ptr = sm1->start;

    return VKD3D_OK;
}

static uint32_t get_external_constant_count(struct vkd3d_shader_sm1_parser *sm1,
        enum vkd3d_shader_d3dbc_constant_register set)
{
    unsigned int j;

    /* Find the highest constant index which is not written by a DEF
     * instruction. We can't (easily) use an FFZ function for this since it
     * needs to be limited by the highest used register index. */
    for (j = sm1->constants[set].count; j > 0; --j)
    {
        if (!bitmap_is_set(sm1->constants[set].def_mask, j - 1))
            return j;
    }

    return 0;
}

int d3dbc_parse(const struct vkd3d_shader_compile_info *compile_info, uint64_t config_flags,
        struct vkd3d_shader_message_context *message_context, struct vsir_program *program)
{
    struct vkd3d_shader_instruction_array *instructions;
    struct vkd3d_shader_sm1_parser sm1 = {0};
    struct vkd3d_shader_instruction *ins;
    unsigned int i;
    int ret;

    if ((ret = shader_sm1_init(&sm1, program, compile_info, message_context)) < 0)
    {
        WARN("Failed to initialise shader parser, ret %d.\n", ret);
        return ret;
    }

    instructions = &program->instructions;
    while (!shader_sm1_is_end(&sm1))
    {
        if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
        {
            ERR("Failed to allocate instructions.\n");
            vkd3d_shader_parser_error(&sm1.p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY, "Out of memory.");
            vsir_program_cleanup(program);
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }
        ins = &instructions->elements[instructions->count];
        shader_sm1_read_instruction(&sm1, ins);

        if (ins->opcode == VKD3DSIH_INVALID)
        {
            WARN("Encountered unrecognized or invalid instruction.\n");
            vsir_program_cleanup(program);
            return VKD3D_ERROR_INVALID_SHADER;
        }
        ++instructions->count;
    }

    for (i = 0; i < ARRAY_SIZE(program->flat_constant_count); ++i)
        program->flat_constant_count[i] = get_external_constant_count(&sm1, i);

    if (sm1.p.failed && ret >= 0)
        ret = VKD3D_ERROR_INVALID_SHADER;

    if (ret < 0)
    {
        vsir_program_cleanup(program);
        return ret;
    }

    return VKD3D_OK;
}

bool sm1_register_from_semantic_name(const struct vkd3d_shader_version *version, const char *semantic_name,
        unsigned int semantic_index, bool output, enum vkd3d_shader_register_type *type, unsigned int *reg)
{
    unsigned int i;

    static const struct
    {
        const char *semantic;
        bool output;
        enum vkd3d_shader_type shader_type;
        unsigned int major_version;
        enum vkd3d_shader_register_type type;
        unsigned int offset;
    }
    register_table[] =
    {
        {"color",       false, VKD3D_SHADER_TYPE_PIXEL, 1, VKD3DSPR_INPUT},
        {"texcoord",    false, VKD3D_SHADER_TYPE_PIXEL, 1, VKD3DSPR_TEXTURE},

        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3DSPR_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3DSPR_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3DSPR_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3DSPR_COLOROUT},
        {"color",       false, VKD3D_SHADER_TYPE_PIXEL, 2, VKD3DSPR_INPUT},
        {"texcoord",    false, VKD3D_SHADER_TYPE_PIXEL, 2, VKD3DSPR_TEXTURE},

        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3DSPR_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3DSPR_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3DSPR_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3DSPR_COLOROUT},
        {"sv_position", false, VKD3D_SHADER_TYPE_PIXEL, 3, VKD3DSPR_MISCTYPE,     VKD3D_SM1_MISC_POSITION},
        {"vface",       false, VKD3D_SHADER_TYPE_PIXEL, 3, VKD3DSPR_MISCTYPE,     VKD3D_SM1_MISC_FACE},
        {"vpos",        false, VKD3D_SHADER_TYPE_PIXEL, 3, VKD3DSPR_MISCTYPE,     VKD3D_SM1_MISC_POSITION},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3DSPR_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3DSPR_TEXCRDOUT},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3DSPR_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3DSPR_RASTOUT,     VSIR_RASTOUT_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3DSPR_TEXCRDOUT},
    };

    for (i = 0; i < ARRAY_SIZE(register_table); ++i)
    {
        if (!ascii_strcasecmp(semantic_name, register_table[i].semantic)
                && output == register_table[i].output
                && version->type == register_table[i].shader_type
                && version->major == register_table[i].major_version)
        {
            *type = register_table[i].type;
            if (register_table[i].type == VKD3DSPR_MISCTYPE || register_table[i].type == VKD3DSPR_RASTOUT)
                *reg = register_table[i].offset;
            else
                *reg = semantic_index;
            return true;
        }
    }

    return false;
}

bool sm1_usage_from_semantic_name(const char *semantic_name,
        uint32_t semantic_index, enum vkd3d_decl_usage *usage, uint32_t *usage_idx)
{
    static const struct
    {
        const char *name;
        enum vkd3d_decl_usage usage;
    }
    semantics[] =
    {
        {"binormal",        VKD3D_DECL_USAGE_BINORMAL},
        {"blendindices",    VKD3D_DECL_USAGE_BLEND_INDICES},
        {"blendweight",     VKD3D_DECL_USAGE_BLEND_WEIGHT},
        {"color",           VKD3D_DECL_USAGE_COLOR},
        {"depth",           VKD3D_DECL_USAGE_DEPTH},
        {"fog",             VKD3D_DECL_USAGE_FOG},
        {"normal",          VKD3D_DECL_USAGE_NORMAL},
        {"position",        VKD3D_DECL_USAGE_POSITION},
        {"positiont",       VKD3D_DECL_USAGE_POSITIONT},
        {"psize",           VKD3D_DECL_USAGE_PSIZE},
        {"sample",          VKD3D_DECL_USAGE_SAMPLE},
        {"sv_depth",        VKD3D_DECL_USAGE_DEPTH},
        {"sv_position",     VKD3D_DECL_USAGE_POSITION},
        {"sv_target",       VKD3D_DECL_USAGE_COLOR},
        {"tangent",         VKD3D_DECL_USAGE_TANGENT},
        {"tessfactor",      VKD3D_DECL_USAGE_TESS_FACTOR},
        {"texcoord",        VKD3D_DECL_USAGE_TEXCOORD},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(semantics); ++i)
    {
        if (!ascii_strcasecmp(semantic_name, semantics[i].name))
        {
            *usage = semantics[i].usage;
            *usage_idx = semantic_index;
            return true;
        }
    }

    return false;
}

struct d3dbc_compiler
{
    const struct vkd3d_sm1_opcode_info *opcode_table;
    struct vsir_program *program;
    struct vkd3d_bytecode_buffer buffer;
    struct vkd3d_shader_message_context *message_context;
    bool failed;
};

static uint32_t sm1_version(enum vkd3d_shader_type type, unsigned int major, unsigned int minor)
{
    return vkd3d_make_u32(vkd3d_make_u16(minor, major),
            type == VKD3D_SHADER_TYPE_VERTEX ? VKD3D_SM1_VS : VKD3D_SM1_PS);
}

D3DXPARAMETER_CLASS hlsl_sm1_class(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_ARRAY:
            return hlsl_sm1_class(type->e.array.type);
        case HLSL_CLASS_MATRIX:
            VKD3D_ASSERT(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);
            if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                return D3DXPC_MATRIX_COLUMNS;
            else
                return D3DXPC_MATRIX_ROWS;
        case HLSL_CLASS_SCALAR:
            return D3DXPC_SCALAR;
        case HLSL_CLASS_STRUCT:
            return D3DXPC_STRUCT;
        case HLSL_CLASS_VECTOR:
            return D3DXPC_VECTOR;
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_VERTEX_SHADER:
            return D3DXPC_OBJECT;
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            break;
    }

    vkd3d_unreachable();
}

D3DXPARAMETER_TYPE hlsl_sm1_base_type(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            switch (type->e.numeric.type)
            {
                case HLSL_TYPE_BOOL:
                    return D3DXPT_BOOL;
                /* Actually double behaves differently depending on DLL version:
                 * For <= 36, it maps to D3DXPT_FLOAT.
                 * For 37-40, it maps to zero (D3DXPT_VOID).
                 * For >= 41, it maps to 39, which is D3D_SVT_DOUBLE (note D3D_SVT_*
                 *            values are mostly compatible with D3DXPT_*).
                 * However, the latter two cases look like bugs, and a reasonable
                 * application certainly wouldn't know what to do with them.
                 * For fx_2_0 it's always D3DXPT_FLOAT regardless of DLL version. */
                case HLSL_TYPE_DOUBLE:
                case HLSL_TYPE_FLOAT:
                case HLSL_TYPE_HALF:
                    return D3DXPT_FLOAT;
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    return D3DXPT_INT;
                default:
                    vkd3d_unreachable();
            }

        case HLSL_CLASS_SAMPLER:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3DXPT_SAMPLER1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3DXPT_SAMPLER2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3DXPT_SAMPLER3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3DXPT_SAMPLERCUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3DXPT_SAMPLER;
                default:
                    ERR("Invalid dimension %#x.\n", type->sampler_dim);
                    vkd3d_unreachable();
            }
            break;

        case HLSL_CLASS_TEXTURE:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3DXPT_TEXTURE1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3DXPT_TEXTURE2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3DXPT_TEXTURE3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3DXPT_TEXTURECUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3DXPT_TEXTURE;
                default:
                    ERR("Invalid dimension %#x.\n", type->sampler_dim);
                    vkd3d_unreachable();
            }
            break;

        case HLSL_CLASS_ARRAY:
            return hlsl_sm1_base_type(type->e.array.type);

        case HLSL_CLASS_STRUCT:
            return D3DXPT_VOID;

        case HLSL_CLASS_STRING:
            return D3DXPT_STRING;

        case HLSL_CLASS_PIXEL_SHADER:
            return D3DXPT_PIXELSHADER;

        case HLSL_CLASS_VERTEX_SHADER:
            return D3DXPT_VERTEXSHADER;

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            break;
    }

    vkd3d_unreachable();
}

static void write_sm1_type(struct vkd3d_bytecode_buffer *buffer, struct hlsl_type *type, unsigned int ctab_start)
{
    const struct hlsl_type *array_type = hlsl_get_multiarray_element_type(type);
    unsigned int array_size = hlsl_get_multiarray_size(type);
    unsigned int field_count = 0;
    size_t fields_offset = 0;
    size_t i;

    if (type->bytecode_offset)
        return;

    if (array_type->class == HLSL_CLASS_STRUCT)
    {
        field_count = array_type->e.record.field_count;

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            field->name_bytecode_offset = put_string(buffer, field->name);
            write_sm1_type(buffer, field->type, ctab_start);
        }

        fields_offset = bytecode_align(buffer) - ctab_start;

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            put_u32(buffer, field->name_bytecode_offset - ctab_start);
            put_u32(buffer, field->type->bytecode_offset - ctab_start);
        }
    }

    type->bytecode_offset = put_u32(buffer, vkd3d_make_u32(hlsl_sm1_class(type), hlsl_sm1_base_type(array_type)));
    put_u32(buffer, vkd3d_make_u32(type->dimy, type->dimx));
    put_u32(buffer, vkd3d_make_u32(array_size, field_count));
    put_u32(buffer, fields_offset);
}

static void sm1_sort_extern(struct list *sorted, struct hlsl_ir_var *to_sort)
{
    struct hlsl_ir_var *var;

    list_remove(&to_sort->extern_entry);

    LIST_FOR_EACH_ENTRY(var, sorted, struct hlsl_ir_var, extern_entry)
    {
        if (strcmp(to_sort->name, var->name) < 0)
        {
            list_add_before(&var->extern_entry, &to_sort->extern_entry);
            return;
        }
    }

    list_add_tail(sorted, &to_sort->extern_entry);
}

static void sm1_sort_externs(struct hlsl_ctx *ctx)
{
    struct list sorted = LIST_INIT(sorted);
    struct hlsl_ir_var *var, *next;

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_uniform)
            sm1_sort_extern(&sorted, var);
    }
    list_move_tail(&ctx->extern_vars, &sorted);
}

void write_sm1_uniforms(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer)
{
    size_t ctab_offset, ctab_start, ctab_end, vars_offset, vars_start, size_offset, creator_offset, offset;
    unsigned int uniform_count = 0;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int r;

        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            if (var->semantic.name || !var->regs[r].allocated || !var->last_read)
                continue;

            ++uniform_count;

            if (var->is_param && var->is_uniform)
            {
                char *new_name;

                if (!(new_name = hlsl_sprintf_alloc(ctx, "$%s", var->name)))
                    return;
                vkd3d_free((char *)var->name);
                var->name = new_name;
            }
        }
    }

    sm1_sort_externs(ctx);

    size_offset = put_u32(buffer, 0);
    ctab_offset = put_u32(buffer, VKD3D_MAKE_TAG('C','T','A','B'));

    ctab_start = put_u32(buffer, 7 * sizeof(uint32_t)); /* CTAB header size. */
    creator_offset = put_u32(buffer, 0);
    put_u32(buffer, sm1_version(ctx->profile->type, ctx->profile->major_version, ctx->profile->minor_version));
    put_u32(buffer, uniform_count);
    vars_offset = put_u32(buffer, 0);
    put_u32(buffer, 0); /* FIXME: flags */
    put_u32(buffer, 0); /* FIXME: target string */

    vars_start = bytecode_align(buffer);
    set_u32(buffer, vars_offset, vars_start - ctab_start);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int r;

        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            if (var->semantic.name || !var->regs[r].allocated || !var->last_read)
                continue;

            put_u32(buffer, 0); /* name */
            if (r == HLSL_REGSET_NUMERIC)
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_FLOAT4, var->regs[r].id));
                put_u32(buffer, var->bind_count[r]);
            }
            else
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_SAMPLER, var->regs[r].index));
                put_u32(buffer, var->bind_count[r]);
            }
            put_u32(buffer, 0); /* type */
            put_u32(buffer, 0); /* default value */
        }
    }

    uniform_count = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int r;

        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            size_t var_offset, name_offset;

            if (var->semantic.name || !var->regs[r].allocated || !var->last_read)
                continue;

            var_offset = vars_start + (uniform_count * 5 * sizeof(uint32_t));

            name_offset = put_string(buffer, var->name);
            set_u32(buffer, var_offset, name_offset - ctab_start);

            write_sm1_type(buffer, var->data_type, ctab_start);
            set_u32(buffer, var_offset + 3 * sizeof(uint32_t), var->data_type->bytecode_offset - ctab_start);

            if (var->default_values)
            {
                unsigned int reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
                unsigned int comp_count = hlsl_type_component_count(var->data_type);
                unsigned int default_value_offset;
                unsigned int k;

                default_value_offset = bytecode_reserve_bytes(buffer, reg_size * sizeof(uint32_t));
                set_u32(buffer, var_offset + 4 * sizeof(uint32_t), default_value_offset - ctab_start);

                for (k = 0; k < comp_count; ++k)
                {
                    struct hlsl_type *comp_type = hlsl_type_get_component_type(ctx, var->data_type, k);
                    unsigned int comp_offset;
                    enum hlsl_regset regset;

                    comp_offset = hlsl_type_get_component_offset(ctx, var->data_type, k, &regset);
                    if (regset == HLSL_REGSET_NUMERIC)
                    {
                        union
                        {
                            uint32_t u;
                            float f;
                        } uni;

                        switch (comp_type->e.numeric.type)
                        {
                            case HLSL_TYPE_DOUBLE:
                                if (ctx->double_as_float_alias)
                                    uni.u = var->default_values[k].number.u;
                                else
                                    uni.u = 0;
                                break;

                            case HLSL_TYPE_INT:
                                uni.f = var->default_values[k].number.i;
                                break;

                            case HLSL_TYPE_UINT:
                            case HLSL_TYPE_BOOL:
                                uni.f = var->default_values[k].number.u;
                                break;

                            case HLSL_TYPE_HALF:
                            case HLSL_TYPE_FLOAT:
                                uni.u = var->default_values[k].number.u;
                                break;

                            default:
                                vkd3d_unreachable();
                        }

                        set_u32(buffer, default_value_offset + comp_offset * sizeof(uint32_t), uni.u);
                    }
                }
            }

            ++uniform_count;
        }
    }

    offset = put_string(buffer, vkd3d_shader_get_version(NULL, NULL));
    set_u32(buffer, creator_offset, offset - ctab_start);

    ctab_end = bytecode_align(buffer);
    set_u32(buffer, size_offset, vkd3d_make_u32(VKD3D_SM1_OP_COMMENT, (ctab_end - ctab_offset) / sizeof(uint32_t)));
}

static uint32_t sm1_encode_register_type(enum vkd3d_shader_register_type type)
{
    return ((type << VKD3D_SM1_REGISTER_TYPE_SHIFT) & VKD3D_SM1_REGISTER_TYPE_MASK)
            | ((type << VKD3D_SM1_REGISTER_TYPE_SHIFT2) & VKD3D_SM1_REGISTER_TYPE_MASK2);
}

struct sm1_instruction
{
    enum vkd3d_sm1_opcode opcode;
    unsigned int flags;

    struct sm1_dst_register
    {
        enum vkd3d_shader_register_type type;
        enum vkd3d_shader_dst_modifier mod;
        unsigned int writemask;
        uint32_t reg;
    } dst;

    struct sm1_src_register
    {
        enum vkd3d_shader_register_type type;
        enum vkd3d_shader_src_modifier mod;
        unsigned int swizzle;
        uint32_t reg;
    } srcs[4];
    unsigned int src_count;

    unsigned int has_dst;
};

static bool is_inconsequential_instr(const struct sm1_instruction *instr)
{
    const struct sm1_src_register *src = &instr->srcs[0];
    const struct sm1_dst_register *dst = &instr->dst;
    unsigned int i;

    if (instr->opcode != VKD3D_SM1_OP_MOV)
        return false;
    if (dst->mod != VKD3DSPDM_NONE)
        return false;
    if (src->mod != VKD3DSPSM_NONE)
        return false;
    if (src->type != dst->type)
        return false;
    if (src->reg != dst->reg)
        return false;

    for (i = 0; i < 4; ++i)
    {
        if ((dst->writemask & (1 << i)) && (vsir_swizzle_get_component(src->swizzle, i) != i))
            return false;
    }

    return true;
}

static void write_sm1_dst_register(struct vkd3d_bytecode_buffer *buffer, const struct sm1_dst_register *reg)
{
    VKD3D_ASSERT(reg->writemask);
    put_u32(buffer, VKD3D_SM1_INSTRUCTION_PARAMETER
            | sm1_encode_register_type(reg->type)
            | (reg->mod << VKD3D_SM1_DST_MODIFIER_SHIFT)
            | (reg->writemask << VKD3D_SM1_WRITEMASK_SHIFT) | reg->reg);
}

static void write_sm1_src_register(struct vkd3d_bytecode_buffer *buffer,
        const struct sm1_src_register *reg)
{
    put_u32(buffer, VKD3D_SM1_INSTRUCTION_PARAMETER
            | sm1_encode_register_type(reg->type)
            | (reg->mod << VKD3D_SM1_SRC_MODIFIER_SHIFT)
            | (reg->swizzle << VKD3D_SM1_SWIZZLE_SHIFT) | reg->reg);
}

static void d3dbc_write_instruction(struct d3dbc_compiler *d3dbc, const struct sm1_instruction *instr)
{
    const struct vkd3d_shader_version *version = &d3dbc->program->shader_version;
    struct vkd3d_bytecode_buffer *buffer = &d3dbc->buffer;
    uint32_t token = instr->opcode;
    unsigned int i;

    if (is_inconsequential_instr(instr))
        return;

    token |= VKD3D_SM1_INSTRUCTION_FLAGS_MASK & (instr->flags << VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT);

    if (version->major > 1)
        token |= (instr->has_dst + instr->src_count) << VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT;
    put_u32(buffer, token);

    if (instr->has_dst)
        write_sm1_dst_register(buffer, &instr->dst);

    for (i = 0; i < instr->src_count; ++i)
        write_sm1_src_register(buffer, &instr->srcs[i]);
};

static const struct vkd3d_sm1_opcode_info *shader_sm1_get_opcode_info_from_vsir(
        struct d3dbc_compiler *d3dbc, enum vkd3d_shader_opcode vkd3d_opcode)
{
    const struct vkd3d_shader_version *version = &d3dbc->program->shader_version;
    const struct vkd3d_sm1_opcode_info *info;
    unsigned int i = 0;

    for (;;)
    {
        info = &d3dbc->opcode_table[i++];
        if (info->vkd3d_opcode == VKD3DSIH_INVALID)
            return NULL;

        if (vkd3d_opcode == info->vkd3d_opcode
                && vkd3d_shader_ver_ge(version, info->min_version.major, info->min_version.minor)
                && (vkd3d_shader_ver_le(version, info->max_version.major, info->max_version.minor)
                        || !info->max_version.major))
            return info;
    }
}

static uint32_t swizzle_from_vsir(uint32_t swizzle)
{
    uint32_t x = vsir_swizzle_get_component(swizzle, 0);
    uint32_t y = vsir_swizzle_get_component(swizzle, 1);
    uint32_t z = vsir_swizzle_get_component(swizzle, 2);
    uint32_t w = vsir_swizzle_get_component(swizzle, 3);

    if (x & ~0x3u || y & ~0x3u || z & ~0x3u || w & ~0x3u)
        ERR("Unexpected vsir swizzle: 0x%08x.\n", swizzle);

    return ((x & 0x3u) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(0))
            | ((y & 0x3) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(1))
            | ((z & 0x3) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(2))
            | ((w & 0x3) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(3));
}

static void sm1_src_reg_from_vsir(struct d3dbc_compiler *d3dbc, const struct vkd3d_shader_src_param *param,
        struct sm1_src_register *src, const struct vkd3d_shader_location *loc)
{
    src->mod = param->modifiers;
    src->reg = param->reg.idx[0].offset;
    src->type = param->reg.type;
    src->swizzle = swizzle_from_vsir(param->swizzle);

    if (param->reg.idx[0].rel_addr)
    {
        vkd3d_shader_error(d3dbc->message_context, loc, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                "Unhandled relative addressing on source register.");
        d3dbc->failed = true;
    }
}

static void sm1_dst_reg_from_vsir(struct d3dbc_compiler *d3dbc, const struct vkd3d_shader_dst_param *param,
        struct sm1_dst_register *dst, const struct vkd3d_shader_location *loc)
{
    dst->mod = param->modifiers;
    dst->reg = param->reg.idx[0].offset;
    dst->type = param->reg.type;
    dst->writemask = param->write_mask;

    if (param->reg.idx[0].rel_addr)
    {
        vkd3d_shader_error(d3dbc->message_context, loc, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                "Unhandled relative addressing on destination register.");
        d3dbc->failed = true;
    }
}

static void d3dbc_write_vsir_def(struct d3dbc_compiler *d3dbc, const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_shader_version *version = &d3dbc->program->shader_version;
    struct vkd3d_bytecode_buffer *buffer = &d3dbc->buffer;
    uint32_t token;

    const struct sm1_dst_register reg =
    {
        .type = VKD3DSPR_CONST,
        .writemask = VKD3DSP_WRITEMASK_ALL,
        .reg = ins->dst[0].reg.idx[0].offset,
    };

    token = VKD3D_SM1_OP_DEF;
    if (version->major > 1)
        token |= 5 << VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT;
    put_u32(buffer, token);

    write_sm1_dst_register(buffer, &reg);
    for (unsigned int x = 0; x < 4; ++x)
        put_f32(buffer, ins->src[0].reg.u.immconst_f32[x]);
}

static void d3dbc_write_vsir_sampler_dcl(struct d3dbc_compiler *d3dbc,
        unsigned int reg_id, enum vkd3d_sm1_resource_type res_type)
{
    const struct vkd3d_shader_version *version = &d3dbc->program->shader_version;
    struct vkd3d_bytecode_buffer *buffer = &d3dbc->buffer;
    struct sm1_dst_register reg = {0};
    uint32_t token;

    token = VKD3D_SM1_OP_DCL;
    if (version->major > 1)
        token |= 2 << VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT;
    put_u32(buffer, token);

    token = VKD3D_SM1_INSTRUCTION_PARAMETER;
    token |= res_type << VKD3D_SM1_RESOURCE_TYPE_SHIFT;
    put_u32(buffer, token);

    reg.type = VKD3DSPR_COMBINED_SAMPLER;
    reg.writemask = VKD3DSP_WRITEMASK_ALL;
    reg.reg = reg_id;

    write_sm1_dst_register(buffer, &reg);
}

static void d3dbc_write_vsir_dcl(struct d3dbc_compiler *d3dbc, const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_shader_version *version = &d3dbc->program->shader_version;
    const struct vkd3d_shader_semantic *semantic = &ins->declaration.semantic;
    unsigned int reg_id;

    if (version->major < 2)
        return;

    reg_id = semantic->resource.reg.reg.idx[0].offset;

    if (semantic->resource.reg.reg.type != VKD3DSPR_SAMPLER)
    {
        vkd3d_shader_error(d3dbc->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_TYPE,
                "dcl instruction with register type %u.", semantic->resource.reg.reg.type);
        d3dbc->failed = true;
        return;
    }

    switch (semantic->resource_type)
    {
        case VKD3D_SHADER_RESOURCE_TEXTURE_2D:
            d3dbc_write_vsir_sampler_dcl(d3dbc, reg_id, VKD3D_SM1_RESOURCE_TEXTURE_2D);
            break;

        case VKD3D_SHADER_RESOURCE_TEXTURE_CUBE:
            d3dbc_write_vsir_sampler_dcl(d3dbc, reg_id, VKD3D_SM1_RESOURCE_TEXTURE_CUBE);
            break;

        case VKD3D_SHADER_RESOURCE_TEXTURE_3D:
            d3dbc_write_vsir_sampler_dcl(d3dbc, reg_id, VKD3D_SM1_RESOURCE_TEXTURE_3D);
            break;

        default:
            vkd3d_shader_error(d3dbc->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_RESOURCE_TYPE,
                   "dcl instruction with resource_type %u.", semantic->resource_type);
            d3dbc->failed = true;
            return;
    }
}

static const struct vkd3d_sm1_opcode_info *shader_sm1_get_opcode_info_from_vsir_instruction(
        struct d3dbc_compiler *d3dbc, const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_sm1_opcode_info *info;

    if (!(info = shader_sm1_get_opcode_info_from_vsir(d3dbc, ins->opcode)))
    {
        vkd3d_shader_error(d3dbc->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                "Opcode %#x not supported for shader profile.", ins->opcode);
        d3dbc->failed = true;
        return NULL;
    }

    if (ins->dst_count != info->dst_count)
    {
        vkd3d_shader_error(d3dbc->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_COUNT,
                "Invalid destination count %u for vsir instruction %#x (expected %u).",
                ins->dst_count, ins->opcode, info->dst_count);
        d3dbc->failed = true;
        return NULL;
    }
    if (ins->src_count != info->src_count)
    {
        vkd3d_shader_error(d3dbc->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_COUNT,
                "Invalid source count %u for vsir instruction %#x (expected %u).",
                ins->src_count, ins->opcode, info->src_count);
        d3dbc->failed = true;
        return NULL;
    }

    return info;
}

static void d3dbc_write_vsir_simple_instruction(struct d3dbc_compiler *d3dbc,
        const struct vkd3d_shader_instruction *ins)
{
    struct sm1_instruction instr = {0};
    const struct vkd3d_sm1_opcode_info *info;

    if (!(info = shader_sm1_get_opcode_info_from_vsir_instruction(d3dbc, ins)))
        return;

    instr.opcode = info->sm1_opcode;
    instr.flags = ins->flags;
    instr.has_dst = info->dst_count;
    instr.src_count = info->src_count;

    if (instr.has_dst)
        sm1_dst_reg_from_vsir(d3dbc, &ins->dst[0], &instr.dst, &ins->location);
    for (unsigned int i = 0; i < instr.src_count; ++i)
        sm1_src_reg_from_vsir(d3dbc, &ins->src[i], &instr.srcs[i], &ins->location);

    d3dbc_write_instruction(d3dbc, &instr);
}

static void d3dbc_write_vsir_instruction(struct d3dbc_compiler *d3dbc, const struct vkd3d_shader_instruction *ins)
{
    uint32_t writemask;

    switch (ins->opcode)
    {
        case VKD3DSIH_DEF:
            d3dbc_write_vsir_def(d3dbc, ins);
            break;

        case VKD3DSIH_DCL:
            d3dbc_write_vsir_dcl(d3dbc, ins);
            break;

        case VKD3DSIH_ABS:
        case VKD3DSIH_ADD:
        case VKD3DSIH_CMP:
        case VKD3DSIH_DP2ADD:
        case VKD3DSIH_DP3:
        case VKD3DSIH_DP4:
        case VKD3DSIH_DSX:
        case VKD3DSIH_DSY:
        case VKD3DSIH_ELSE:
        case VKD3DSIH_ENDIF:
        case VKD3DSIH_FRC:
        case VKD3DSIH_IFC:
        case VKD3DSIH_MAD:
        case VKD3DSIH_MAX:
        case VKD3DSIH_MIN:
        case VKD3DSIH_MOV:
        case VKD3DSIH_MUL:
        case VKD3DSIH_SINCOS:
        case VKD3DSIH_SLT:
        case VKD3DSIH_TEX:
        case VKD3DSIH_TEXKILL:
        case VKD3DSIH_TEXLDD:
            d3dbc_write_vsir_simple_instruction(d3dbc, ins);
            break;

        case VKD3DSIH_EXP:
        case VKD3DSIH_LOG:
        case VKD3DSIH_RCP:
        case VKD3DSIH_RSQ:
            writemask = ins->dst->write_mask;
            if (writemask != VKD3DSP_WRITEMASK_0 && writemask != VKD3DSP_WRITEMASK_1
                    && writemask != VKD3DSP_WRITEMASK_2 && writemask != VKD3DSP_WRITEMASK_3)
            {
                vkd3d_shader_error(d3dbc->message_context, &ins->location,
                        VKD3D_SHADER_ERROR_D3DBC_INVALID_WRITEMASK,
                        "writemask %#x for vsir instruction with opcode %#x is not single component.",
                        writemask, ins->opcode);
                d3dbc->failed = true;
            }
            d3dbc_write_vsir_simple_instruction(d3dbc, ins);
            break;

        default:
            vkd3d_shader_error(d3dbc->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                   "vsir instruction with opcode %#x.", ins->opcode);
            d3dbc->failed = true;
            break;
    }
}

static void d3dbc_write_semantic_dcl(struct d3dbc_compiler *d3dbc,
        const struct signature_element *element, bool output)
{
    const struct vkd3d_shader_version *version = &d3dbc->program->shader_version;
    struct vkd3d_bytecode_buffer *buffer = &d3dbc->buffer;
    struct sm1_dst_register reg = {0};
    enum vkd3d_decl_usage usage;
    uint32_t token, usage_idx;
    bool ret;

    if (sm1_register_from_semantic_name(version, element->semantic_name,
            element->semantic_index, output, &reg.type, &reg.reg))
    {
        usage = 0;
        usage_idx = 0;
    }
    else
    {
        ret = sm1_usage_from_semantic_name(element->semantic_name, element->semantic_index, &usage, &usage_idx);
        VKD3D_ASSERT(ret);
        reg.type = output ? VKD3DSPR_OUTPUT : VKD3DSPR_INPUT;
        reg.reg = element->register_index;
    }

    token = VKD3D_SM1_OP_DCL;
    if (version->major > 1)
        token |= 2 << VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT;
    put_u32(buffer, token);

    token = (1u << 31);
    token |= usage << VKD3D_SM1_DCL_USAGE_SHIFT;
    token |= usage_idx << VKD3D_SM1_DCL_USAGE_INDEX_SHIFT;
    put_u32(buffer, token);

    reg.writemask = element->mask;
    write_sm1_dst_register(buffer, &reg);
}

static void d3dbc_write_semantic_dcls(struct d3dbc_compiler *d3dbc)
{
    struct vsir_program *program = d3dbc->program;
    const struct vkd3d_shader_version *version;
    bool write_in = false, write_out = false;

    version = &program->shader_version;
    if (version->type == VKD3D_SHADER_TYPE_PIXEL && version->major >= 2)
        write_in = true;
    else if (version->type == VKD3D_SHADER_TYPE_VERTEX && version->major == 3)
        write_in = write_out = true;
    else if (version->type == VKD3D_SHADER_TYPE_VERTEX && version->major < 3)
        write_in = true;

    if (write_in)
    {
        for (unsigned int i = 0; i < program->input_signature.element_count; ++i)
            d3dbc_write_semantic_dcl(d3dbc, &program->input_signature.elements[i], false);
    }

    if (write_out)
    {
        for (unsigned int i = 0; i < program->output_signature.element_count; ++i)
            d3dbc_write_semantic_dcl(d3dbc, &program->output_signature.elements[i], true);
    }
}

static void d3dbc_write_program_instructions(struct d3dbc_compiler *d3dbc)
{
    struct vsir_program *program = d3dbc->program;
    unsigned int i;

    for (i = 0; i < program->instructions.count; ++i)
        d3dbc_write_vsir_instruction(d3dbc, &program->instructions.elements[i]);
}

int d3dbc_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, const struct vkd3d_shader_code *ctab,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    struct d3dbc_compiler d3dbc = {0};
    struct vkd3d_bytecode_buffer *buffer = &d3dbc.buffer;
    int result;

    d3dbc.program = program;
    d3dbc.message_context = message_context;
    switch (version->type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            d3dbc.opcode_table = vs_opcode_table;
            break;

        case VKD3D_SHADER_TYPE_PIXEL:
            d3dbc.opcode_table = ps_opcode_table;
            break;

        default:
            vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_D3DBC_INVALID_PROFILE,
                    "Invalid shader type %u.", version->type);
            return VKD3D_ERROR_INVALID_SHADER;
    }

    put_u32(buffer, sm1_version(version->type, version->major, version->minor));

    bytecode_put_bytes(buffer, ctab->code, ctab->size);

    d3dbc_write_semantic_dcls(&d3dbc);
    d3dbc_write_program_instructions(&d3dbc);

    put_u32(buffer, VKD3D_SM1_OP_END);

    result = VKD3D_OK;
    if (buffer->status)
        result = buffer->status;
    if (d3dbc.failed)
        result = VKD3D_ERROR_INVALID_SHADER;

    if (!result)
    {
        out->code = buffer->data;
        out->size = buffer->size;
    }
    else
    {
        vkd3d_free(buffer->data);
    }
    return result;
}
