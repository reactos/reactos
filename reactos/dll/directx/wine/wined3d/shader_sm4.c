/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_bytecode);

#define WINED3D_SM4_INSTRUCTION_MODIFIER        (1 << 31)

#define WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT    24
#define WINED3D_SM4_INSTRUCTION_LENGTH_MASK     (0x1f << WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT)

#define WINED3D_SM4_RESOURCE_TYPE_SHIFT         11
#define WINED3D_SM4_RESOURCE_TYPE_MASK          (0xf << WINED3D_SM4_RESOURCE_TYPE_SHIFT)

#define WINED3D_SM4_PRIMITIVE_TYPE_SHIFT        11
#define WINED3D_SM4_PRIMITIVE_TYPE_MASK         (0x7 << WINED3D_SM4_PRIMITIVE_TYPE_SHIFT)

#define WINED3D_SM4_INDEX_TYPE_SHIFT            11
#define WINED3D_SM4_INDEX_TYPE_MASK             (0x1 << WINED3D_SM4_INDEX_TYPE_SHIFT)

#define WINED3D_SM4_OPCODE_MASK                 0xff

#define WINED3D_SM4_REGISTER_MODIFIER           (1 << 31)

#define WINED3D_SM4_ADDRESSING_SHIFT1           25
#define WINED3D_SM4_ADDRESSING_MASK1            (0x3 << WINED3D_SM4_ADDRESSING_SHIFT1)

#define WINED3D_SM4_ADDRESSING_SHIFT0           22
#define WINED3D_SM4_ADDRESSING_MASK0            (0x3 << WINED3D_SM4_ADDRESSING_SHIFT0)

#define WINED3D_SM4_REGISTER_ORDER_SHIFT        20
#define WINED3D_SM4_REGISTER_ORDER_MASK         (0x3 << WINED3D_SM4_REGISTER_ORDER_SHIFT)

#define WINED3D_SM4_REGISTER_TYPE_SHIFT         12
#define WINED3D_SM4_REGISTER_TYPE_MASK          (0xf << WINED3D_SM4_REGISTER_TYPE_SHIFT)

#define WINED3D_SM4_SWIZZLE_TYPE_SHIFT          2
#define WINED3D_SM4_SWIZZLE_TYPE_MASK           (0x3 << WINED3D_SM4_SWIZZLE_TYPE_SHIFT)

#define WINED3D_SM4_IMMCONST_TYPE_SHIFT         0
#define WINED3D_SM4_IMMCONST_TYPE_MASK          (0x3 << WINED3D_SM4_IMMCONST_TYPE_SHIFT)

#define WINED3D_SM4_WRITEMASK_SHIFT             4
#define WINED3D_SM4_WRITEMASK_MASK              (0xf << WINED3D_SM4_WRITEMASK_SHIFT)

#define WINED3D_SM4_SWIZZLE_SHIFT               4
#define WINED3D_SM4_SWIZZLE_MASK                (0xff << WINED3D_SM4_SWIZZLE_SHIFT)

#define WINED3D_SM4_VERSION_MAJOR(version)      (((version) >> 4) & 0xf)
#define WINED3D_SM4_VERSION_MINOR(version)      (((version) >> 0) & 0xf)

#define WINED3D_SM4_ADDRESSING_RELATIVE         0x2
#define WINED3D_SM4_ADDRESSING_OFFSET           0x1

enum wined3d_sm4_opcode
{
    WINED3D_SM4_OP_ADD                  = 0x00,
    WINED3D_SM4_OP_AND                  = 0x01,
    WINED3D_SM4_OP_BREAK                = 0x02,
    WINED3D_SM4_OP_BREAKC               = 0x03,
    WINED3D_SM4_OP_CUT                  = 0x09,
    WINED3D_SM4_OP_DERIV_RTX            = 0x0b,
    WINED3D_SM4_OP_DERIV_RTY            = 0x0c,
    WINED3D_SM4_OP_DISCARD              = 0x0d,
    WINED3D_SM4_OP_DIV                  = 0x0e,
    WINED3D_SM4_OP_DP2                  = 0x0f,
    WINED3D_SM4_OP_DP3                  = 0x10,
    WINED3D_SM4_OP_DP4                  = 0x11,
    WINED3D_SM4_OP_EMIT                 = 0x13,
    WINED3D_SM4_OP_ENDIF                = 0x15,
    WINED3D_SM4_OP_ENDLOOP              = 0x16,
    WINED3D_SM4_OP_EQ                   = 0x18,
    WINED3D_SM4_OP_EXP                  = 0x19,
    WINED3D_SM4_OP_FRC                  = 0x1a,
    WINED3D_SM4_OP_FTOI                 = 0x1b,
    WINED3D_SM4_OP_GE                   = 0x1d,
    WINED3D_SM4_OP_IADD                 = 0x1e,
    WINED3D_SM4_OP_IF                   = 0x1f,
    WINED3D_SM4_OP_IEQ                  = 0x20,
    WINED3D_SM4_OP_IGE                  = 0x21,
    WINED3D_SM4_OP_IMUL                 = 0x26,
    WINED3D_SM4_OP_ISHL                 = 0x29,
    WINED3D_SM4_OP_ITOF                 = 0x2b,
    WINED3D_SM4_OP_LD                   = 0x2d,
    WINED3D_SM4_OP_LOG                  = 0x2f,
    WINED3D_SM4_OP_LOOP                 = 0x30,
    WINED3D_SM4_OP_LT                   = 0x31,
    WINED3D_SM4_OP_MAD                  = 0x32,
    WINED3D_SM4_OP_MIN                  = 0x33,
    WINED3D_SM4_OP_MAX                  = 0x34,
    WINED3D_SM4_OP_MOV                  = 0x36,
    WINED3D_SM4_OP_MOVC                 = 0x37,
    WINED3D_SM4_OP_MUL                  = 0x38,
    WINED3D_SM4_OP_NE                   = 0x39,
    WINED3D_SM4_OP_OR                   = 0x3c,
    WINED3D_SM4_OP_RET                  = 0x3e,
    WINED3D_SM4_OP_ROUND_NI             = 0x41,
    WINED3D_SM4_OP_RSQ                  = 0x44,
    WINED3D_SM4_OP_SAMPLE               = 0x45,
    WINED3D_SM4_OP_SAMPLE_LOD           = 0x48,
    WINED3D_SM4_OP_SAMPLE_GRAD          = 0x49,
    WINED3D_SM4_OP_SQRT                 = 0x4b,
    WINED3D_SM4_OP_SINCOS               = 0x4d,
    WINED3D_SM4_OP_UDIV                 = 0x4e,
    WINED3D_SM4_OP_UGE                  = 0x50,
    WINED3D_SM4_OP_USHR                 = 0x55,
    WINED3D_SM4_OP_UTOF                 = 0x56,
    WINED3D_SM4_OP_XOR                  = 0x57,
    WINED3D_SM4_OP_DCL_RESOURCE         = 0x58,
    WINED3D_SM4_OP_DCL_CONSTANT_BUFFER  = 0x59,
    WINED3D_SM4_OP_DCL_OUTPUT_TOPOLOGY  = 0x5c,
    WINED3D_SM4_OP_DCL_INPUT_PRIMITIVE  = 0x5d,
    WINED3D_SM4_OP_DCL_VERTICES_OUT     = 0x5e,
};

enum wined3d_sm4_register_type
{
    WINED3D_SM4_RT_TEMP         = 0x0,
    WINED3D_SM4_RT_INPUT        = 0x1,
    WINED3D_SM4_RT_OUTPUT       = 0x2,
    WINED3D_SM4_RT_IMMCONST     = 0x4,
    WINED3D_SM4_RT_SAMPLER      = 0x6,
    WINED3D_SM4_RT_RESOURCE     = 0x7,
    WINED3D_SM4_RT_CONSTBUFFER  = 0x8,
    WINED3D_SM4_RT_PRIMID       = 0xb,
    WINED3D_SM4_RT_NULL         = 0xd,
};

enum wined3d_sm4_output_primitive_type
{
    WINED3D_SM4_OUTPUT_PT_POINTLIST     = 0x1,
    WINED3D_SM4_OUTPUT_PT_LINELIST      = 0x3,
    WINED3D_SM4_OUTPUT_PT_TRIANGLESTRIP = 0x5,
};

enum wined3d_sm4_input_primitive_type
{
    WINED3D_SM4_INPUT_PT_POINT          = 0x1,
    WINED3D_SM4_INPUT_PT_LINE           = 0x2,
    WINED3D_SM4_INPUT_PT_TRIANGLE       = 0x3,
    WINED3D_SM4_INPUT_PT_LINEADJ        = 0x6,
    WINED3D_SM4_INPUT_PT_TRIANGLEADJ    = 0x7,
};

enum wined3d_sm4_swizzle_type
{
    WINED3D_SM4_SWIZZLE_VEC4            = 0x1,
    WINED3D_SM4_SWIZZLE_SCALAR          = 0x2,
};

enum wined3d_sm4_immconst_type
{
    WINED3D_SM4_IMMCONST_SCALAR = 0x1,
    WINED3D_SM4_IMMCONST_VEC4   = 0x2,
};

enum wined3d_sm4_resource_type
{
    WINED3D_SM4_RESOURCE_BUFFER             = 0x1,
    WINED3D_SM4_RESOURCE_TEXTURE_1D         = 0x2,
    WINED3D_SM4_RESOURCE_TEXTURE_2D         = 0x3,
    WINED3D_SM4_RESOURCE_TEXTURE_2DMS       = 0x4,
    WINED3D_SM4_RESOURCE_TEXTURE_3D         = 0x5,
    WINED3D_SM4_RESOURCE_TEXTURE_CUBE       = 0x6,
    WINED3D_SM4_RESOURCE_TEXTURE_1DARRAY    = 0x7,
    WINED3D_SM4_RESOURCE_TEXTURE_2DARRAY    = 0x8,
    WINED3D_SM4_RESOURCE_TEXTURE_2DMSARRAY  = 0x9,
};

enum wined3d_sm4_data_type
{
    WINED3D_SM4_DATA_UNORM  = 0x1,
    WINED3D_SM4_DATA_SNORM  = 0x2,
    WINED3D_SM4_DATA_INT    = 0x3,
    WINED3D_SM4_DATA_UINT   = 0x4,
    WINED3D_SM4_DATA_FLOAT  = 0x5,
};

struct wined3d_shader_src_param_entry
{
    struct list entry;
    struct wined3d_shader_src_param param;
};

struct wined3d_sm4_data
{
    struct wined3d_shader_version shader_version;
    const DWORD *end;

    struct
    {
        enum wined3d_shader_register_type register_type;
        UINT register_idx;
    } output_map[MAX_REG_OUTPUT];

    struct wined3d_shader_src_param src_param[5];
    struct wined3d_shader_dst_param dst_param[2];
    struct list src_free;
    struct list src;
};

struct wined3d_sm4_opcode_info
{
    enum wined3d_sm4_opcode opcode;
    enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx;
    const char *dst_info;
    const char *src_info;
};

struct sysval_map
{
    enum wined3d_sysval_semantic sysval;
    enum wined3d_shader_register_type register_type;
    UINT register_idx;
};

/*
 * F -> WINED3D_DATA_FLOAT
 * I -> WINED3D_DATA_INT
 * R -> WINED3D_DATA_RESOURCE
 * S -> WINED3D_DATA_SAMPLER
 * U -> WINED3D_DATA_UINT
 */
static const struct wined3d_sm4_opcode_info opcode_table[] =
{
    {WINED3D_SM4_OP_ADD,                    WINED3DSIH_ADD,                 "F",    "FF"},
    {WINED3D_SM4_OP_AND,                    WINED3DSIH_AND,                 "U",    "UU"},
    {WINED3D_SM4_OP_BREAK,                  WINED3DSIH_BREAK,               "",     ""},
    {WINED3D_SM4_OP_BREAKC,                 WINED3DSIH_BREAKP,              "",     "U"},
    {WINED3D_SM4_OP_CUT,                    WINED3DSIH_CUT,                 "",     ""},
    {WINED3D_SM4_OP_DERIV_RTX,              WINED3DSIH_DSX,                 "F",    "F"},
    {WINED3D_SM4_OP_DERIV_RTY,              WINED3DSIH_DSY,                 "F",    "F"},
    {WINED3D_SM4_OP_DISCARD,                WINED3DSIH_TEXKILL,             "",     "U"},
    {WINED3D_SM4_OP_DIV,                    WINED3DSIH_DIV,                 "F",    "FF"},
    {WINED3D_SM4_OP_DP2,                    WINED3DSIH_DP2,                 "F",    "FF"},
    {WINED3D_SM4_OP_DP3,                    WINED3DSIH_DP3,                 "F",    "FF"},
    {WINED3D_SM4_OP_DP4,                    WINED3DSIH_DP4,                 "F",    "FF"},
    {WINED3D_SM4_OP_EMIT,                   WINED3DSIH_EMIT,                "",     ""},
    {WINED3D_SM4_OP_ENDIF,                  WINED3DSIH_ENDIF,               "",     ""},
    {WINED3D_SM4_OP_ENDLOOP,                WINED3DSIH_ENDLOOP,             "",     ""},
    {WINED3D_SM4_OP_EQ,                     WINED3DSIH_EQ,                  "U",    "FF"},
    {WINED3D_SM4_OP_EXP,                    WINED3DSIH_EXP,                 "F",    "F"},
    {WINED3D_SM4_OP_FRC,                    WINED3DSIH_FRC,                 "F",    "F"},
    {WINED3D_SM4_OP_FTOI,                   WINED3DSIH_FTOI,                "I",    "F"},
    {WINED3D_SM4_OP_GE,                     WINED3DSIH_GE,                  "U",    "FF"},
    {WINED3D_SM4_OP_IADD,                   WINED3DSIH_IADD,                "I",    "II"},
    {WINED3D_SM4_OP_IF,                     WINED3DSIH_IF,                  "",     "U"},
    {WINED3D_SM4_OP_IEQ,                    WINED3DSIH_IEQ,                 "U",    "II"},
    {WINED3D_SM4_OP_IGE,                    WINED3DSIH_IGE,                 "U",    "II"},
    {WINED3D_SM4_OP_IMUL,                   WINED3DSIH_IMUL,                "II",   "II"},
    {WINED3D_SM4_OP_ISHL,                   WINED3DSIH_ISHL,                "I",    "II"},
    {WINED3D_SM4_OP_ITOF,                   WINED3DSIH_ITOF,                "F",    "I"},
    {WINED3D_SM4_OP_LD,                     WINED3DSIH_LD,                  "U",    "FR"},
    {WINED3D_SM4_OP_LOG,                    WINED3DSIH_LOG,                 "F",    "F"},
    {WINED3D_SM4_OP_LOOP,                   WINED3DSIH_LOOP,                "",     ""},
    {WINED3D_SM4_OP_LT,                     WINED3DSIH_LT,                  "U",    "FF"},
    {WINED3D_SM4_OP_MAD,                    WINED3DSIH_MAD,                 "F",    "FFF"},
    {WINED3D_SM4_OP_MIN,                    WINED3DSIH_MIN,                 "F",    "FF"},
    {WINED3D_SM4_OP_MAX,                    WINED3DSIH_MAX,                 "F",    "FF"},
    {WINED3D_SM4_OP_MOV,                    WINED3DSIH_MOV,                 "F",    "F"},
    {WINED3D_SM4_OP_MOVC,                   WINED3DSIH_MOVC,                "F",    "UFF"},
    {WINED3D_SM4_OP_MUL,                    WINED3DSIH_MUL,                 "F",    "FF"},
    {WINED3D_SM4_OP_NE,                     WINED3DSIH_NE,                  "U",    "FF"},
    {WINED3D_SM4_OP_OR,                     WINED3DSIH_OR,                  "U",    "UU"},
    {WINED3D_SM4_OP_RET,                    WINED3DSIH_RET,                 "",     ""},
    {WINED3D_SM4_OP_ROUND_NI,               WINED3DSIH_ROUND_NI,            "F",    "F"},
    {WINED3D_SM4_OP_RSQ,                    WINED3DSIH_RSQ,                 "F",    "F"},
    {WINED3D_SM4_OP_SAMPLE,                 WINED3DSIH_SAMPLE,              "U",    "FRS"},
    {WINED3D_SM4_OP_SAMPLE_LOD,             WINED3DSIH_SAMPLE_LOD,          "U",    "FRSF"},
    {WINED3D_SM4_OP_SAMPLE_GRAD,            WINED3DSIH_SAMPLE_GRAD,         "U",    "FRSFF"},
    {WINED3D_SM4_OP_SQRT,                   WINED3DSIH_SQRT,                "F",    "F"},
    {WINED3D_SM4_OP_SINCOS,                 WINED3DSIH_SINCOS,              "FF",   "F"},
    {WINED3D_SM4_OP_UDIV,                   WINED3DSIH_UDIV,                "UU",   "UU"},
    {WINED3D_SM4_OP_UGE,                    WINED3DSIH_UGE,                 "U",    "UU"},
    {WINED3D_SM4_OP_USHR,                   WINED3DSIH_USHR,                "U",    "UU"},
    {WINED3D_SM4_OP_UTOF,                   WINED3DSIH_UTOF,                "F",    "U"},
    {WINED3D_SM4_OP_XOR,                    WINED3DSIH_XOR,                 "U",    "UU"},
    {WINED3D_SM4_OP_DCL_RESOURCE,           WINED3DSIH_DCL,                 "R",    ""},
    {WINED3D_SM4_OP_DCL_CONSTANT_BUFFER,    WINED3DSIH_DCL_CONSTANT_BUFFER, "",     ""},
    {WINED3D_SM4_OP_DCL_OUTPUT_TOPOLOGY,    WINED3DSIH_DCL_OUTPUT_TOPOLOGY, "",     ""},
    {WINED3D_SM4_OP_DCL_INPUT_PRIMITIVE,    WINED3DSIH_DCL_INPUT_PRIMITIVE, "",     ""},
    {WINED3D_SM4_OP_DCL_VERTICES_OUT,       WINED3DSIH_DCL_VERTICES_OUT,    "",     ""},
};

static const enum wined3d_shader_register_type register_type_table[] =
{
    /* WINED3D_SM4_RT_TEMP */           WINED3DSPR_TEMP,
    /* WINED3D_SM4_RT_INPUT */          WINED3DSPR_INPUT,
    /* WINED3D_SM4_RT_OUTPUT */         WINED3DSPR_OUTPUT,
    /* UNKNOWN */                       0,
    /* WINED3D_SM4_RT_IMMCONST */       WINED3DSPR_IMMCONST,
    /* UNKNOWN */                       0,
    /* WINED3D_SM4_RT_SAMPLER */        WINED3DSPR_SAMPLER,
    /* WINED3D_SM4_RT_RESOURCE */       WINED3DSPR_RESOURCE,
    /* WINED3D_SM4_RT_CONSTBUFFER */    WINED3DSPR_CONSTBUFFER,
    /* UNKNOWN */                       0,
    /* UNKNOWN */                       0,
    /* WINED3D_SM4_RT_PRIMID */         WINED3DSPR_PRIMID,
    /* UNKNOWN */                       0,
    /* WINED3D_SM4_RT_NULL */           WINED3DSPR_NULL,
};

static const enum wined3d_primitive_type output_primitive_type_table[] =
{
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_OUTPUT_PT_POINTLIST */       WINED3D_PT_POINTLIST,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_OUTPUT_PT_LINELIST */        WINED3D_PT_LINELIST,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_OUTPUT_PT_TRIANGLESTRIP */   WINED3D_PT_TRIANGLESTRIP,
};

static const enum wined3d_primitive_type input_primitive_type_table[] =
{
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_INPUT_PT_POINT */            WINED3D_PT_POINTLIST,
    /* WINED3D_SM4_INPUT_PT_LINE */             WINED3D_PT_LINELIST,
    /* WINED3D_SM4_INPUT_PT_TRIANGLE */         WINED3D_PT_TRIANGLELIST,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_INPUT_PT_LINEADJ */          WINED3D_PT_LINELIST_ADJ,
    /* WINED3D_SM4_INPUT_PT_TRIANGLEADJ */      WINED3D_PT_TRIANGLELIST_ADJ,
};

static const struct sysval_map sysval_map[] =
{
    {WINED3D_SV_DEPTH,      WINED3DSPR_DEPTHOUT,    0},
    {WINED3D_SV_TARGET0,    WINED3DSPR_COLOROUT,    0},
    {WINED3D_SV_TARGET1,    WINED3DSPR_COLOROUT,    1},
    {WINED3D_SV_TARGET2,    WINED3DSPR_COLOROUT,    2},
    {WINED3D_SV_TARGET3,    WINED3DSPR_COLOROUT,    3},
    {WINED3D_SV_TARGET4,    WINED3DSPR_COLOROUT,    4},
    {WINED3D_SV_TARGET5,    WINED3DSPR_COLOROUT,    5},
    {WINED3D_SV_TARGET6,    WINED3DSPR_COLOROUT,    6},
    {WINED3D_SV_TARGET7,    WINED3DSPR_COLOROUT,    7},
};

static const enum wined3d_shader_resource_type resource_type_table[] =
{
    /* 0 */                                         WINED3D_SHADER_RESOURCE_NONE,
    /* WINED3D_SM4_RESOURCE_BUFFER */               WINED3D_SHADER_RESOURCE_BUFFER,
    /* WINED3D_SM4_RESOURCE_TEXTURE_1D */           WINED3D_SHADER_RESOURCE_TEXTURE_1D,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2D */           WINED3D_SHADER_RESOURCE_TEXTURE_2D,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2DMS */         WINED3D_SHADER_RESOURCE_TEXTURE_2DMS,
    /* WINED3D_SM4_RESOURCE_TEXTURE_3D */           WINED3D_SHADER_RESOURCE_TEXTURE_3D,
    /* WINED3D_SM4_RESOURCE_TEXTURE_CUBE */         WINED3D_SHADER_RESOURCE_TEXTURE_CUBE,
    /* WINED3D_SM4_RESOURCE_TEXTURE_1DARRAY */      WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2DARRAY */      WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2DMSARRAY */    WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY,
};

static const enum wined3d_data_type data_type_table[] =
{
    /* 0 */                         WINED3D_DATA_FLOAT,
    /* WINED3D_SM4_DATA_UNORM */    WINED3D_DATA_UNORM,
    /* WINED3D_SM4_DATA_SNORM */    WINED3D_DATA_SNORM,
    /* WINED3D_SM4_DATA_INT */      WINED3D_DATA_INT,
    /* WINED3D_SM4_DATA_UINT */     WINED3D_DATA_UINT,
    /* WINED3D_SM4_DATA_FLOAT */    WINED3D_DATA_FLOAT,
};

static BOOL shader_sm4_read_src_param(struct wined3d_sm4_data *priv, const DWORD **ptr,
        enum wined3d_data_type data_type, struct wined3d_shader_src_param *src_param);

static const struct wined3d_sm4_opcode_info *get_opcode_info(enum wined3d_sm4_opcode opcode)
{
    unsigned int i;

    for (i = 0; i < sizeof(opcode_table) / sizeof(*opcode_table); ++i)
    {
        if (opcode == opcode_table[i].opcode) return &opcode_table[i];
    }

    return NULL;
}

static void map_register(const struct wined3d_sm4_data *priv, struct wined3d_shader_register *reg)
{
    switch (priv->shader_version.type)
    {
        case WINED3D_SHADER_TYPE_PIXEL:
            if (reg->type == WINED3DSPR_OUTPUT)
            {
                unsigned int reg_idx = reg->idx[0].offset;

                if (reg_idx >= ARRAY_SIZE(priv->output_map))
                {
                    ERR("Invalid output index %u.\n", reg_idx);
                    break;
                }

                reg->type = priv->output_map[reg_idx].register_type;
                reg->idx[0].offset = priv->output_map[reg_idx].register_idx;
            }
            break;

        default:
            break;
    }
}

static enum wined3d_data_type map_data_type(char t)
{
    switch (t)
    {
        case 'F':
            return WINED3D_DATA_FLOAT;
        case 'I':
            return WINED3D_DATA_INT;
        case 'R':
            return WINED3D_DATA_RESOURCE;
        case 'S':
            return WINED3D_DATA_SAMPLER;
        case 'U':
            return WINED3D_DATA_UINT;
        default:
            ERR("Invalid data type '%c'.\n", t);
            return WINED3D_DATA_FLOAT;
    }
}

static void *shader_sm4_init(const DWORD *byte_code, const struct wined3d_shader_signature *output_signature)
{
    struct wined3d_sm4_data *priv;
    unsigned int i, j;

    if (!(priv = HeapAlloc(GetProcessHeap(), 0, sizeof(*priv))))
    {
        ERR("Failed to allocate private data\n");
        return NULL;
    }

    memset(priv->output_map, 0xff, sizeof(priv->output_map));
    for (i = 0; i < output_signature->element_count; ++i)
    {
        struct wined3d_shader_signature_element *e = &output_signature->elements[i];

        if (e->register_idx >= ARRAY_SIZE(priv->output_map))
        {
            WARN("Invalid output index %u.\n", e->register_idx);
            continue;
        }

        for (j = 0; j < ARRAY_SIZE(sysval_map); ++j)
        {
            if (e->sysval_semantic == sysval_map[j].sysval)
            {
                priv->output_map[e->register_idx].register_type = sysval_map[j].register_type;
                priv->output_map[e->register_idx].register_idx = sysval_map[j].register_idx;
                break;
            }
        }
    }

    list_init(&priv->src_free);
    list_init(&priv->src);

    return priv;
}

static void shader_sm4_free(void *data)
{
    struct wined3d_shader_src_param_entry *e1, *e2;
    struct wined3d_sm4_data *priv = data;

    list_move_head(&priv->src_free, &priv->src);
    LIST_FOR_EACH_ENTRY_SAFE(e1, e2, &priv->src_free, struct wined3d_shader_src_param_entry, entry)
    {
        HeapFree(GetProcessHeap(), 0, e1);
    }
    HeapFree(GetProcessHeap(), 0, priv);
}

static struct wined3d_shader_src_param *get_src_param(struct wined3d_sm4_data *priv)
{
    struct wined3d_shader_src_param_entry *e;
    struct list *elem;

    if (!list_empty(&priv->src_free))
    {
        elem = list_head(&priv->src_free);
        list_remove(elem);
    }
    else
    {
        if (!(e = HeapAlloc(GetProcessHeap(), 0, sizeof(*e))))
            return NULL;
        elem = &e->entry;
    }

    list_add_tail(&priv->src, elem);
    e = LIST_ENTRY(elem, struct wined3d_shader_src_param_entry, entry);
    return &e->param;
}

static void shader_sm4_read_header(void *data, const DWORD **ptr, struct wined3d_shader_version *shader_version)
{
    struct wined3d_sm4_data *priv = data;
    DWORD version_token;

    priv->end = *ptr;

    version_token = *(*ptr)++;
    TRACE("version: 0x%08x\n", version_token);

    TRACE("token count: %u\n", **ptr);
    priv->end += *(*ptr)++;

    switch (version_token >> 16)
    {
        case WINED3D_SM4_PS:
            priv->shader_version.type = WINED3D_SHADER_TYPE_PIXEL;
            break;

        case WINED3D_SM4_VS:
            priv->shader_version.type = WINED3D_SHADER_TYPE_VERTEX;
            break;

        case WINED3D_SM4_GS:
            priv->shader_version.type = WINED3D_SHADER_TYPE_GEOMETRY;
            break;

        default:
            FIXME("Unrecognized shader type %#x\n", version_token >> 16);
    }
    priv->shader_version.major = WINED3D_SM4_VERSION_MAJOR(version_token);
    priv->shader_version.minor = WINED3D_SM4_VERSION_MINOR(version_token);

    *shader_version = priv->shader_version;
}

static BOOL shader_sm4_read_reg_idx(struct wined3d_sm4_data *priv, const DWORD **ptr,
        DWORD addressing, struct wined3d_shader_register_index *reg_idx)
{
    if (addressing & WINED3D_SM4_ADDRESSING_RELATIVE)
    {
        struct wined3d_shader_src_param *rel_addr = get_src_param(priv);

        if (!(reg_idx->rel_addr = rel_addr))
        {
            ERR("Failed to get src param for relative addressing.\n");
            return FALSE;
        }

        if (addressing & WINED3D_SM4_ADDRESSING_OFFSET)
            reg_idx->offset = *(*ptr)++;
        else
            reg_idx->offset = 0;
        shader_sm4_read_src_param(priv, ptr, WINED3D_DATA_INT, rel_addr);
    }
    else
    {
        reg_idx->rel_addr = NULL;
        reg_idx->offset = *(*ptr)++;
    }

    return TRUE;
}

static BOOL shader_sm4_read_param(struct wined3d_sm4_data *priv, const DWORD **ptr,
        enum wined3d_data_type data_type, struct wined3d_shader_register *param,
        enum wined3d_shader_src_modifier *modifier)
{
    enum wined3d_sm4_register_type register_type;
    DWORD token = *(*ptr)++;
    DWORD order;

    register_type = (token & WINED3D_SM4_REGISTER_TYPE_MASK) >> WINED3D_SM4_REGISTER_TYPE_SHIFT;
    if (register_type >= sizeof(register_type_table) / sizeof(*register_type_table))
    {
        FIXME("Unhandled register type %#x.\n", register_type);
        param->type = WINED3DSPR_TEMP;
    }
    else
    {
        param->type = register_type_table[register_type];
    }
    param->data_type = data_type;

    if (token & WINED3D_SM4_REGISTER_MODIFIER)
    {
        DWORD m = *(*ptr)++;

        /* FIXME: This will probably break down at some point. The SM4
         * modifiers look like flags, while wined3d currently has an enum
         * with possible combinations, e.g. WINED3DSPSM_ABSNEG. */
        switch (m)
        {
            case 0x41:
                *modifier = WINED3DSPSM_NEG;
                break;

            case 0x81:
                *modifier = WINED3DSPSM_ABS;
                break;

            default:
                FIXME("Skipping modifier 0x%08x.\n", m);
                *modifier = WINED3DSPSM_NONE;
                break;
        }
    }
    else
    {
        *modifier = WINED3DSPSM_NONE;
    }

    order = (token & WINED3D_SM4_REGISTER_ORDER_MASK) >> WINED3D_SM4_REGISTER_ORDER_SHIFT;

    if (order < 1)
        param->idx[0].offset = ~0U;
    else
    {
        DWORD addressing = (token & WINED3D_SM4_ADDRESSING_MASK0) >> WINED3D_SM4_ADDRESSING_SHIFT0;
        if (!(shader_sm4_read_reg_idx(priv, ptr, addressing, &param->idx[0])))
        {
            ERR("Failed to read register index.\n");
            return FALSE;
        }
    }

    if (order < 2)
        param->idx[1].offset = ~0U;
    else
    {
        DWORD addressing = (token & WINED3D_SM4_ADDRESSING_MASK1) >> WINED3D_SM4_ADDRESSING_SHIFT1;
        if (!(shader_sm4_read_reg_idx(priv, ptr, addressing, &param->idx[1])))
        {
            ERR("Failed to read register index.\n");
            return FALSE;
        }
    }

    if (order > 2)
        FIXME("Unhandled order %u.\n", order);

    if (register_type == WINED3D_SM4_RT_IMMCONST)
    {
        enum wined3d_sm4_immconst_type immconst_type =
                (token & WINED3D_SM4_IMMCONST_TYPE_MASK) >> WINED3D_SM4_IMMCONST_TYPE_SHIFT;

        switch (immconst_type)
        {
            case WINED3D_SM4_IMMCONST_SCALAR:
                param->immconst_type = WINED3D_IMMCONST_SCALAR;
                memcpy(param->immconst_data, *ptr, 1 * sizeof(DWORD));
                *ptr += 1;
                break;

            case WINED3D_SM4_IMMCONST_VEC4:
                param->immconst_type = WINED3D_IMMCONST_VEC4;
                memcpy(param->immconst_data, *ptr, 4 * sizeof(DWORD));
                *ptr += 4;
                break;

            default:
                FIXME("Unhandled immediate constant type %#x.\n", immconst_type);
                break;
        }
    }

    map_register(priv, param);

    return TRUE;
}

static BOOL shader_sm4_read_src_param(struct wined3d_sm4_data *priv, const DWORD **ptr,
        enum wined3d_data_type data_type, struct wined3d_shader_src_param *src_param)
{
    DWORD token = **ptr;

    if (!shader_sm4_read_param(priv, ptr, data_type, &src_param->reg, &src_param->modifiers))
    {
        ERR("Failed to read parameter.\n");
        return FALSE;
    }

    if (src_param->reg.type == WINED3DSPR_IMMCONST)
    {
        src_param->swizzle = WINED3DSP_NOSWIZZLE;
    }
    else
    {
        enum wined3d_sm4_swizzle_type swizzle_type =
                (token & WINED3D_SM4_SWIZZLE_TYPE_MASK) >> WINED3D_SM4_SWIZZLE_TYPE_SHIFT;

        switch (swizzle_type)
        {
            case WINED3D_SM4_SWIZZLE_SCALAR:
                src_param->swizzle = (token & WINED3D_SM4_SWIZZLE_MASK) >> WINED3D_SM4_SWIZZLE_SHIFT;
                src_param->swizzle = (src_param->swizzle & 0x3) * 0x55;
                break;

            case WINED3D_SM4_SWIZZLE_VEC4:
                src_param->swizzle = (token & WINED3D_SM4_SWIZZLE_MASK) >> WINED3D_SM4_SWIZZLE_SHIFT;
                break;

            default:
                FIXME("Unhandled swizzle type %#x.\n", swizzle_type);
                break;
        }
    }

    return TRUE;
}

static BOOL shader_sm4_read_dst_param(struct wined3d_sm4_data *priv, const DWORD **ptr,
        enum wined3d_data_type data_type, struct wined3d_shader_dst_param *dst_param)
{
    enum wined3d_shader_src_modifier modifier;
    DWORD token = **ptr;

    if (!shader_sm4_read_param(priv, ptr, data_type, &dst_param->reg, &modifier))
    {
        ERR("Failed to read parameter.\n");
        return FALSE;
    }

    if (modifier != WINED3DSPSM_NONE)
    {
        ERR("Invalid source modifier %#x on destination register.\n", modifier);
        return FALSE;
    }

    dst_param->write_mask = (token & WINED3D_SM4_WRITEMASK_MASK) >> WINED3D_SM4_WRITEMASK_SHIFT;
    dst_param->modifiers = 0;
    dst_param->shift = 0;

    return TRUE;
}

static void shader_sm4_read_instruction(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins)
{
    const struct wined3d_sm4_opcode_info *opcode_info;
    struct wined3d_sm4_data *priv = data;
    DWORD opcode_token, opcode;
    const DWORD *p;
    UINT i, len;

    list_move_head(&priv->src_free, &priv->src);

    opcode_token = *(*ptr)++;
    opcode = opcode_token & WINED3D_SM4_OPCODE_MASK;
    len = ((opcode_token & WINED3D_SM4_INSTRUCTION_LENGTH_MASK) >> WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT) - 1;

    if (TRACE_ON(d3d_bytecode))
    {
        TRACE_(d3d_bytecode)("[ %08x ", opcode_token);
        for (i = 0; i < len; ++i)
        {
            TRACE_(d3d_bytecode)("%08x ", (*ptr)[i]);
        }
        TRACE_(d3d_bytecode)("]\n");
    }

    if (!(opcode_info = get_opcode_info(opcode)))
    {
        FIXME("Unrecognized opcode %#x, opcode_token 0x%08x.\n", opcode, opcode_token);
        ins->handler_idx = WINED3DSIH_TABLE_SIZE;
        *ptr += len;
        return;
    }

    ins->handler_idx = opcode_info->handler_idx;
    ins->flags = 0;
    ins->coissue = 0;
    ins->predicate = NULL;
    ins->dst_count = strlen(opcode_info->dst_info);
    ins->dst = priv->dst_param;
    ins->src_count = strlen(opcode_info->src_info);
    ins->src = priv->src_param;

    p = *ptr;
    *ptr += len;

    if (opcode_token & WINED3D_SM4_INSTRUCTION_MODIFIER)
    {
        DWORD modifier = *p++;
        FIXME("Skipping modifier 0x%08x.\n", modifier);
    }

    if (opcode == WINED3D_SM4_OP_DCL_RESOURCE)
    {
        enum wined3d_sm4_resource_type resource_type;
        enum wined3d_sm4_data_type data_type;
        DWORD components;

        resource_type = (opcode_token & WINED3D_SM4_RESOURCE_TYPE_MASK) >> WINED3D_SM4_RESOURCE_TYPE_SHIFT;
        if (!resource_type || (resource_type >= ARRAY_SIZE(resource_type_table)))
        {
            FIXME("Unhandled resource type %#x.\n", resource_type);
            ins->declaration.semantic.resource_type = WINED3D_SHADER_RESOURCE_NONE;
        }
        else
        {
            ins->declaration.semantic.resource_type = resource_type_table[resource_type];
        }
        shader_sm4_read_dst_param(priv, &p, WINED3D_DATA_RESOURCE, &ins->declaration.semantic.reg);

        components = *p++;
        if ((components & 0xfff0) != (components & 0xf) * 0x1110)
            FIXME("Components (%#x) have different data types.\n", components);
        data_type = components & 0xf;

        if (!data_type || (data_type >= ARRAY_SIZE(data_type_table)))
        {
            FIXME("Unhandled data type %#x.\n", data_type);
            ins->declaration.semantic.resource_data_type = WINED3D_DATA_FLOAT;
        }
        else
        {
            ins->declaration.semantic.resource_data_type = data_type_table[data_type];
        }
    }
    else if (opcode == WINED3D_SM4_OP_DCL_CONSTANT_BUFFER)
    {
        shader_sm4_read_src_param(priv, &p, WINED3D_DATA_FLOAT, &ins->declaration.src);
        if (opcode_token & WINED3D_SM4_INDEX_TYPE_MASK)
            ins->flags |= WINED3DSI_INDEXED_DYNAMIC;
    }
    else if (opcode == WINED3D_SM4_OP_DCL_OUTPUT_TOPOLOGY)
    {
        enum wined3d_sm4_output_primitive_type primitive_type;

        primitive_type = (opcode_token & WINED3D_SM4_PRIMITIVE_TYPE_MASK) >> WINED3D_SM4_PRIMITIVE_TYPE_SHIFT;
        if (primitive_type >= sizeof(output_primitive_type_table) / sizeof(*output_primitive_type_table))
        {
            FIXME("Unhandled output primitive type %#x.\n", primitive_type);
            ins->declaration.primitive_type = WINED3D_PT_UNDEFINED;
        }
        else
        {
            ins->declaration.primitive_type = output_primitive_type_table[primitive_type];
        }
    }
    else if (opcode == WINED3D_SM4_OP_DCL_INPUT_PRIMITIVE)
    {
        enum wined3d_sm4_input_primitive_type primitive_type;

        primitive_type = (opcode_token & WINED3D_SM4_PRIMITIVE_TYPE_MASK) >> WINED3D_SM4_PRIMITIVE_TYPE_SHIFT;
        if (primitive_type >= sizeof(input_primitive_type_table) / sizeof(*input_primitive_type_table))
        {
            FIXME("Unhandled input primitive type %#x.\n", primitive_type);
            ins->declaration.primitive_type = WINED3D_PT_UNDEFINED;
        }
        else
        {
            ins->declaration.primitive_type = input_primitive_type_table[primitive_type];
        }
    }
    else if (opcode == WINED3D_SM4_OP_DCL_VERTICES_OUT)
    {
        ins->declaration.count = *p++;
    }
    else
    {
        for (i = 0; i < ins->dst_count; ++i)
        {
            if (!(shader_sm4_read_dst_param(priv, &p, map_data_type(opcode_info->dst_info[i]), &priv->dst_param[i])))
            {
                ins->handler_idx = WINED3DSIH_TABLE_SIZE;
                return;
            }
        }

        for (i = 0; i < ins->src_count; ++i)
        {
            if (!(shader_sm4_read_src_param(priv, &p, map_data_type(opcode_info->src_info[i]), &priv->src_param[i])))
            {
                ins->handler_idx = WINED3DSIH_TABLE_SIZE;
                return;
            }
        }
    }
}

static BOOL shader_sm4_is_end(void *data, const DWORD **ptr)
{
    struct wined3d_sm4_data *priv = data;
    return *ptr == priv->end;
}

const struct wined3d_shader_frontend sm4_shader_frontend =
{
    shader_sm4_init,
    shader_sm4_free,
    shader_sm4_read_header,
    shader_sm4_read_instruction,
    shader_sm4_is_end,
};
