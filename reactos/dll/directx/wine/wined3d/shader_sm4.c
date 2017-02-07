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

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

#define WINED3D_SM4_INSTRUCTION_MODIFIER        (1 << 31)

#define WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT    24
#define WINED3D_SM4_INSTRUCTION_LENGTH_MASK     (0xf << WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT)

#define WINED3D_SM4_OPCODE_MASK                 0xff

#define WINED3D_SM4_REGISTER_MODIFIER           (1 << 31)

#define WINED3D_SM4_REGISTER_ORDER_SHIFT        20
#define WINED3D_SM4_REGISTER_ORDER_MASK         (0x3 << WINED3D_SM4_REGISTER_ORDER_SHIFT)

#define WINED3D_SM4_REGISTER_TYPE_SHIFT         12
#define WINED3D_SM4_REGISTER_TYPE_MASK          (0xf << WINED3D_SM4_REGISTER_TYPE_SHIFT)

#define WINED3D_SM4_IMMCONST_TYPE_SHIFT         0
#define WINED3D_SM4_IMMCONST_TYPE_MASK          (0x3 << WINED3D_SM4_IMMCONST_TYPE_SHIFT)

#define WINED3D_SM4_WRITEMASK_SHIFT             4
#define WINED3D_SM4_WRITEMASK_MASK              (0xf << WINED3D_SM4_WRITEMASK_SHIFT)

#define WINED3D_SM4_SWIZZLE_SHIFT               4
#define WINED3D_SM4_SWIZZLE_MASK                (0xff << WINED3D_SM4_SWIZZLE_SHIFT)

#define WINED3D_SM4_VERSION_MAJOR(version)      (((version) >> 4) & 0xf)
#define WINED3D_SM4_VERSION_MINOR(version)      (((version) >> 0) & 0xf)

enum wined3d_sm4_opcode
{
    WINED3D_SM4_OP_ADD          = 0x00,
    WINED3D_SM4_OP_AND          = 0x01,
    WINED3D_SM4_OP_BREAK        = 0x02,
    WINED3D_SM4_OP_BREAKC       = 0x03,
    WINED3D_SM4_OP_CUT          = 0x09,
    WINED3D_SM4_OP_DERIV_RTX    = 0x0b,
    WINED3D_SM4_OP_DERIV_RTY    = 0x0c,
    WINED3D_SM4_OP_DIV          = 0x0e,
    WINED3D_SM4_OP_DP3          = 0x10,
    WINED3D_SM4_OP_DP4          = 0x11,
    WINED3D_SM4_OP_EMIT         = 0x13,
    WINED3D_SM4_OP_ENDIF        = 0x15,
    WINED3D_SM4_OP_ENDLOOP      = 0x16,
    WINED3D_SM4_OP_EQ           = 0x18,
    WINED3D_SM4_OP_EXP          = 0x19,
    WINED3D_SM4_OP_FRC          = 0x1a,
    WINED3D_SM4_OP_FTOI         = 0x1b,
    WINED3D_SM4_OP_GE           = 0x1d,
    WINED3D_SM4_OP_IADD         = 0x1e,
    WINED3D_SM4_OP_IF           = 0x1f,
    WINED3D_SM4_OP_IEQ          = 0x20,
    WINED3D_SM4_OP_IGE          = 0x21,
    WINED3D_SM4_OP_IMUL         = 0x26,
    WINED3D_SM4_OP_ITOF         = 0x2b,
    WINED3D_SM4_OP_LD           = 0x2d,
    WINED3D_SM4_OP_LOG          = 0x2f,
    WINED3D_SM4_OP_LOOP         = 0x30,
    WINED3D_SM4_OP_LT           = 0x31,
    WINED3D_SM4_OP_MAD          = 0x32,
    WINED3D_SM4_OP_MIN          = 0x33,
    WINED3D_SM4_OP_MAX          = 0x34,
    WINED3D_SM4_OP_MOV          = 0x36,
    WINED3D_SM4_OP_MOVC         = 0x37,
    WINED3D_SM4_OP_MUL          = 0x38,
    WINED3D_SM4_OP_RET          = 0x3e,
    WINED3D_SM4_OP_ROUND_NI     = 0x41,
    WINED3D_SM4_OP_RSQ          = 0x44,
    WINED3D_SM4_OP_SAMPLE       = 0x45,
    WINED3D_SM4_OP_SAMPLE_LOD   = 0x48,
    WINED3D_SM4_OP_SAMPLE_GRAD  = 0x49,
    WINED3D_SM4_OP_SQRT         = 0x4b,
    WINED3D_SM4_OP_SINCOS       = 0x4d,
    WINED3D_SM4_OP_UDIV         = 0x4e,
    WINED3D_SM4_OP_USHR         = 0x55,
    WINED3D_SM4_OP_UTOF         = 0x56,
    WINED3D_SM4_OP_XOR          = 0x57,
};

enum wined3d_sm4_register_type
{
    WINED3D_SM4_RT_TEMP         = 0x0,
    WINED3D_SM4_RT_INPUT        = 0x1,
    WINED3D_SM4_RT_OUTPUT       = 0x2,
    WINED3D_SM4_RT_IMMCONST     = 0x4,
    WINED3D_SM4_RT_SAMPLER      = 0x6,
    WINED3D_SM4_RT_CONSTBUFFER  = 0x8,
    WINED3D_SM4_RT_NULL         = 0xd,
};

enum wined3d_sm4_immconst_type
{
    WINED3D_SM4_IMMCONST_SCALAR = 0x1,
    WINED3D_SM4_IMMCONST_VEC4   = 0x2,
};

struct wined3d_sm4_data
{
    struct wined3d_shader_version shader_version;
    const DWORD *end;
    const struct wined3d_shader_signature *output_signature;
};

struct wined3d_sm4_opcode_info
{
    enum wined3d_sm4_opcode opcode;
    enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx;
    UINT dst_count;
    UINT src_count;
};

struct sysval_map
{
    enum wined3d_sysval_semantic sysval;
    enum wined3d_shader_register_type register_type;
    UINT register_idx;
};

static const struct wined3d_sm4_opcode_info opcode_table[] =
{
    {WINED3D_SM4_OP_ADD,        WINED3DSIH_ADD,         1,  2},
    {WINED3D_SM4_OP_AND,        WINED3DSIH_AND,         1,  2},
    {WINED3D_SM4_OP_BREAK,      WINED3DSIH_BREAK,       0,  0},
    {WINED3D_SM4_OP_BREAKC,     WINED3DSIH_BREAKP,      0,  1},
    {WINED3D_SM4_OP_CUT,        WINED3DSIH_CUT,         0,  0},
    {WINED3D_SM4_OP_DERIV_RTX,  WINED3DSIH_DSX,         1,  1},
    {WINED3D_SM4_OP_DERIV_RTY,  WINED3DSIH_DSY,         1,  1},
    {WINED3D_SM4_OP_DIV,        WINED3DSIH_DIV,         1,  2},
    {WINED3D_SM4_OP_DP3,        WINED3DSIH_DP3,         1,  2},
    {WINED3D_SM4_OP_DP4,        WINED3DSIH_DP4,         1,  2},
    {WINED3D_SM4_OP_EMIT,       WINED3DSIH_EMIT,        0,  0},
    {WINED3D_SM4_OP_ENDIF,      WINED3DSIH_ENDIF,       0,  0},
    {WINED3D_SM4_OP_ENDLOOP,    WINED3DSIH_ENDLOOP,     0,  0},
    {WINED3D_SM4_OP_EQ,         WINED3DSIH_EQ,          1,  2},
    {WINED3D_SM4_OP_EXP,        WINED3DSIH_EXP,         1,  1},
    {WINED3D_SM4_OP_FRC,        WINED3DSIH_FRC,         1,  1},
    {WINED3D_SM4_OP_FTOI,       WINED3DSIH_FTOI,        1,  1},
    {WINED3D_SM4_OP_GE,         WINED3DSIH_GE,          1,  2},
    {WINED3D_SM4_OP_IADD,       WINED3DSIH_IADD,        1,  2},
    {WINED3D_SM4_OP_IF,         WINED3DSIH_IF,          0,  1},
    {WINED3D_SM4_OP_IEQ,        WINED3DSIH_IEQ,         1,  2},
    {WINED3D_SM4_OP_IGE,        WINED3DSIH_IGE,         1,  2},
    {WINED3D_SM4_OP_IMUL,       WINED3DSIH_IMUL,        2,  2},
    {WINED3D_SM4_OP_ITOF,       WINED3DSIH_ITOF,        1,  1},
    {WINED3D_SM4_OP_LD,         WINED3DSIH_LD,          1,  2},
    {WINED3D_SM4_OP_LOG,        WINED3DSIH_LOG,         1,  1},
    {WINED3D_SM4_OP_LOOP,       WINED3DSIH_LOOP,        0,  0},
    {WINED3D_SM4_OP_LT,         WINED3DSIH_LT,          1,  2},
    {WINED3D_SM4_OP_MAD,        WINED3DSIH_MAD,         1,  3},
    {WINED3D_SM4_OP_MIN,        WINED3DSIH_MIN,         1,  2},
    {WINED3D_SM4_OP_MAX,        WINED3DSIH_MAX,         1,  2},
    {WINED3D_SM4_OP_MOV,        WINED3DSIH_MOV,         1,  1},
    {WINED3D_SM4_OP_MOVC,       WINED3DSIH_MOVC,        1,  3},
    {WINED3D_SM4_OP_MUL,        WINED3DSIH_MUL,         1,  2},
    {WINED3D_SM4_OP_RET,        WINED3DSIH_RET,         0,  0},
    {WINED3D_SM4_OP_ROUND_NI,   WINED3DSIH_ROUND_NI,    1,  1},
    {WINED3D_SM4_OP_RSQ,        WINED3DSIH_RSQ,         1,  1},
    {WINED3D_SM4_OP_SAMPLE,     WINED3DSIH_SAMPLE,      1,  3},
    {WINED3D_SM4_OP_SAMPLE_LOD, WINED3DSIH_SAMPLE_LOD,  1,  4},
    {WINED3D_SM4_OP_SAMPLE_GRAD,WINED3DSIH_SAMPLE_GRAD, 1,  5},
    {WINED3D_SM4_OP_SQRT,       WINED3DSIH_SQRT,        1,  1},
    {WINED3D_SM4_OP_SINCOS,     WINED3DSIH_SINCOS,      2,  1},
    {WINED3D_SM4_OP_UDIV,       WINED3DSIH_UDIV,        2,  2},
    {WINED3D_SM4_OP_USHR,       WINED3DSIH_USHR,        1,  2},
    {WINED3D_SM4_OP_UTOF,       WINED3DSIH_UTOF,        1,  1},
    {WINED3D_SM4_OP_XOR,        WINED3DSIH_XOR,         1,  2},
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
    /* UNKNOWN */                       0,
    /* UNKNOWN */                       0,
    /* WINED3D_SM4_RT_NULL */           WINED3DSPR_NULL,
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

static const struct wined3d_sm4_opcode_info *get_opcode_info(enum wined3d_sm4_opcode opcode)
{
    unsigned int i;

    for (i = 0; i < sizeof(opcode_table) / sizeof(*opcode_table); ++i)
    {
        if (opcode == opcode_table[i].opcode) return &opcode_table[i];
    }

    return NULL;
}

static void map_sysval(enum wined3d_sysval_semantic sysval, struct wined3d_shader_register *reg)
{
    unsigned int i;

    for (i = 0; i < sizeof(sysval_map) / sizeof(*sysval_map); ++i)
    {
        if (sysval == sysval_map[i].sysval)
        {
            reg->type = sysval_map[i].register_type;
            reg->idx = sysval_map[i].register_idx;
        }
    }
}

static void map_register(const struct wined3d_sm4_data *priv, struct wined3d_shader_register *reg)
{
    switch (priv->shader_version.type)
    {
        case WINED3D_SHADER_TYPE_PIXEL:
            if (reg->type == WINED3DSPR_OUTPUT)
            {
                unsigned int i;
                const struct wined3d_shader_signature *s = priv->output_signature;

                if (!s)
                {
                    ERR("Shader has no output signature, unable to map register.\n");
                    break;
                }

                for (i = 0; i < s->element_count; ++i)
                {
                    if (s->elements[i].register_idx == reg->idx)
                    {
                        map_sysval(s->elements[i].sysval_semantic, reg);
                        break;
                    }
                }
            }
            break;

        default:
            break;
    }
}

static void *shader_sm4_init(const DWORD *byte_code, const struct wined3d_shader_signature *output_signature)
{
    struct wined3d_sm4_data *priv = HeapAlloc(GetProcessHeap(), 0, sizeof(*priv));
    if (!priv)
    {
        ERR("Failed to allocate private data\n");
        return NULL;
    }

    priv->output_signature = output_signature;

    return priv;
}

static void shader_sm4_free(void *data)
{
    HeapFree(GetProcessHeap(), 0, data);
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

static void shader_sm4_read_opcode(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins,
        UINT *param_size)
{
    const struct wined3d_sm4_opcode_info *opcode_info;
    DWORD token = *(*ptr)++;
    DWORD opcode = token & WINED3D_SM4_OPCODE_MASK;

    *param_size = ((token & WINED3D_SM4_INSTRUCTION_LENGTH_MASK) >> WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT) - 1;

    opcode_info = get_opcode_info(opcode);
    if (!opcode_info)
    {
        FIXME("Unrecognized opcode %#x, token 0x%08x\n", opcode, token);
        ins->handler_idx = WINED3DSIH_TABLE_SIZE;
        return;
    }

    ins->handler_idx = opcode_info->handler_idx;
    ins->flags = 0;
    ins->coissue = 0;
    ins->predicate = 0;
    ins->dst_count = opcode_info->dst_count;
    ins->src_count = opcode_info->src_count;

    if (token & WINED3D_SM4_INSTRUCTION_MODIFIER)
    {
        DWORD modifier = *(*ptr)++;
        FIXME("Skipping modifier 0x%08x.\n", modifier);
    }
}

static void shader_sm4_read_src_param(void *data, const DWORD **ptr, struct wined3d_shader_src_param *src_param,
        struct wined3d_shader_src_param *src_rel_addr)
{
    struct wined3d_sm4_data *priv = data;
    DWORD token = *(*ptr)++;
    enum wined3d_sm4_register_type register_type;
    DWORD order;

    register_type = (token & WINED3D_SM4_REGISTER_TYPE_MASK) >> WINED3D_SM4_REGISTER_TYPE_SHIFT;
    if (register_type >= sizeof(register_type_table) / sizeof(*register_type_table))
    {
        FIXME("Unhandled register type %#x\n", register_type);
        src_param->reg.type = WINED3DSPR_TEMP;
    }
    else
    {
        src_param->reg.type = register_type_table[register_type];
    }

    if (token & WINED3D_SM4_REGISTER_MODIFIER)
    {
        DWORD modifier = *(*ptr)++;

        /* FIXME: This will probably break down at some point. The SM4
         * modifiers look like flags, while wined3d currently has an enum
         * with possible combinations, e.g. WINED3DSPSM_ABSNEG. */
        switch (modifier)
        {
            case 0x41:
                src_param->modifiers = WINED3DSPSM_NEG;
                break;

            case 0x81:
                src_param->modifiers = WINED3DSPSM_ABS;
                break;

            default:
                FIXME("Skipping modifier 0x%08x.\n", modifier);
                src_param->modifiers = WINED3DSPSM_NONE;
                break;
        }
    }
    else
    {
        src_param->modifiers = WINED3DSPSM_NONE;
    }

    order = (token & WINED3D_SM4_REGISTER_ORDER_MASK) >> WINED3D_SM4_REGISTER_ORDER_SHIFT;

    if (order < 1) src_param->reg.idx = ~0U;
    else src_param->reg.idx = *(*ptr)++;

    if (order < 2) src_param->reg.array_idx = ~0U;
    else src_param->reg.array_idx = *(*ptr)++;

    if (order > 2) FIXME("Unhandled order %u.\n", order);

    if (register_type == WINED3D_SM4_RT_IMMCONST)
    {
        enum wined3d_sm4_immconst_type immconst_type =
                (token & WINED3D_SM4_IMMCONST_TYPE_MASK) >> WINED3D_SM4_IMMCONST_TYPE_SHIFT;
        src_param->swizzle = WINED3DSP_NOSWIZZLE;

        switch(immconst_type)
        {
            case WINED3D_SM4_IMMCONST_SCALAR:
                src_param->reg.immconst_type = WINED3D_IMMCONST_SCALAR;
                memcpy(src_param->reg.immconst_data, *ptr, 1 * sizeof(DWORD));
                *ptr += 1;
                break;

            case WINED3D_SM4_IMMCONST_VEC4:
                src_param->reg.immconst_type = WINED3D_IMMCONST_VEC4;
                memcpy(src_param->reg.immconst_data, *ptr, 4 * sizeof(DWORD));
                *ptr += 4;
                break;

            default:
                FIXME("Unhandled immediate constant type %#x\n", immconst_type);
                break;
        }
    }
    else
    {
        src_param->swizzle = (token & WINED3D_SM4_SWIZZLE_MASK) >> WINED3D_SM4_SWIZZLE_SHIFT;
    }

    src_param->reg.rel_addr = NULL;

    map_register(priv, &src_param->reg);
}

static void shader_sm4_read_dst_param(void *data, const DWORD **ptr, struct wined3d_shader_dst_param *dst_param,
        struct wined3d_shader_src_param *dst_rel_addr)
{
    struct wined3d_sm4_data *priv = data;
    DWORD token = *(*ptr)++;
    enum wined3d_sm4_register_type register_type;
    DWORD order;

    register_type = (token & WINED3D_SM4_REGISTER_TYPE_MASK) >> WINED3D_SM4_REGISTER_TYPE_SHIFT;
    if (register_type >= sizeof(register_type_table) / sizeof(*register_type_table))
    {
        FIXME("Unhandled register type %#x\n", register_type);
        dst_param->reg.type = WINED3DSPR_TEMP;
    }
    else
    {
        dst_param->reg.type = register_type_table[register_type];
    }

    order = (token & WINED3D_SM4_REGISTER_ORDER_MASK) >> WINED3D_SM4_REGISTER_ORDER_SHIFT;

    if (order < 1) dst_param->reg.idx = ~0U;
    else dst_param->reg.idx = *(*ptr)++;

    if (order < 2) dst_param->reg.array_idx = ~0U;
    else dst_param->reg.array_idx = *(*ptr)++;

    if (order > 2) FIXME("Unhandled order %u.\n", order);

    dst_param->write_mask = (token & WINED3D_SM4_WRITEMASK_MASK) >> WINED3D_SM4_WRITEMASK_SHIFT;
    dst_param->modifiers = 0;
    dst_param->shift = 0;
    dst_param->reg.rel_addr = NULL;

    map_register(priv, &dst_param->reg);
}

static void shader_sm4_read_semantic(const DWORD **ptr, struct wined3d_shader_semantic *semantic)
{
    FIXME("ptr %p, semantic %p stub!\n", ptr, semantic);
}

static void shader_sm4_read_comment(const DWORD **ptr, const char **comment, UINT *comment_size)
{
    FIXME("ptr %p, comment %p, comment_size %p stub!\n", ptr, comment, comment_size);
    *comment = NULL;
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
    shader_sm4_read_opcode,
    shader_sm4_read_src_param,
    shader_sm4_read_dst_param,
    shader_sm4_read_semantic,
    shader_sm4_read_comment,
    shader_sm4_is_end,
};
