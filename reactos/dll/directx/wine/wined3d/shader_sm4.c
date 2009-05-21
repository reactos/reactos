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

#define WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT    24
#define WINED3D_SM4_INSTRUCTION_LENGTH_MASK     (0xf << WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT)

#define WINED3D_SM4_OPCODE_MASK                 0xff

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
    WINED3D_SM4_OP_ADD      = 0x00,
    WINED3D_SM4_OP_EXP      = 0x19,
    WINED3D_SM4_OP_MOV      = 0x36,
    WINED3D_SM4_OP_MUL      = 0x38,
    WINED3D_SM4_OP_RET      = 0x3e,
    WINED3D_SM4_OP_SINCOS   = 0x4d,
};

enum wined3d_sm4_register_type
{
    WINED3D_SM4_RT_TEMP     = 0x0,
    WINED3D_SM4_RT_INPUT    = 0x1,
    WINED3D_SM4_RT_OUTPUT   = 0x2,
    WINED3D_SM4_RT_IMMCONST = 0x4,
};

enum wined3d_sm4_immconst_type
{
    WINED3D_SM4_IMMCONST_FLOAT  = 0x1,
    WINED3D_SM4_IMMCONST_FLOAT4 = 0x2,
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
    WINED3DSHADER_PARAM_REGISTER_TYPE register_type;
    UINT register_idx;
};

static const struct wined3d_sm4_opcode_info opcode_table[] =
{
    {WINED3D_SM4_OP_ADD,    WINED3DSIH_ADD,         1,  2},
    {WINED3D_SM4_OP_EXP,    WINED3DSIH_EXP,         1,  1},
    {WINED3D_SM4_OP_MOV,    WINED3DSIH_MOV,         1,  1},
    {WINED3D_SM4_OP_MUL,    WINED3DSIH_MUL,         1,  2},
    {WINED3D_SM4_OP_RET,    WINED3DSIH_RET,         0,  0},
    {WINED3D_SM4_OP_SINCOS, WINED3DSIH_SINCOS,      1,  2},
};

static const WINED3DSHADER_PARAM_REGISTER_TYPE register_type_table[] =
{
    /* WINED3D_SM4_RT_TEMP */       WINED3DSPR_TEMP,
    /* WINED3D_SM4_RT_INPUT */      WINED3DSPR_INPUT,
    /* WINED3D_SM4_RT_OUTPUT */     WINED3DSPR_OUTPUT,
    /* UNKNOWN */                   0,
    /* WINED3D_SM4_RT_IMMCONST */   WINED3DSPR_IMMCONST,
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

static void map_register(struct wined3d_sm4_data *priv, struct wined3d_shader_register *reg)
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
}

static void shader_sm4_read_src_param(void *data, const DWORD **ptr, struct wined3d_shader_src_param *src_param,
        struct wined3d_shader_src_param *src_rel_addr)
{
    struct wined3d_sm4_data *priv = data;
    DWORD token = *(*ptr)++;
    enum wined3d_sm4_register_type register_type;

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

    if (register_type == WINED3D_SM4_RT_IMMCONST)
    {
        enum wined3d_sm4_immconst_type immconst_type =
                (token & WINED3D_SM4_IMMCONST_TYPE_MASK) >> WINED3D_SM4_IMMCONST_TYPE_SHIFT;
        src_param->swizzle = WINED3DSP_NOSWIZZLE;

        switch(immconst_type)
        {
            case WINED3D_SM4_IMMCONST_FLOAT:
                src_param->reg.immconst_type = WINED3D_IMMCONST_FLOAT;
                memcpy(src_param->reg.immconst_data, *ptr, 1 * sizeof(DWORD));
                *ptr += 1;
                break;

            case WINED3D_SM4_IMMCONST_FLOAT4:
                src_param->reg.immconst_type = WINED3D_IMMCONST_FLOAT4;
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
        src_param->reg.idx = *(*ptr)++;
        src_param->swizzle = (token & WINED3D_SM4_SWIZZLE_MASK) >> WINED3D_SM4_SWIZZLE_SHIFT;
    }

    src_param->modifiers = 0;
    src_param->reg.rel_addr = NULL;

    map_register(priv, &src_param->reg);
}

static void shader_sm4_read_dst_param(void *data, const DWORD **ptr, struct wined3d_shader_dst_param *dst_param,
        struct wined3d_shader_src_param *dst_rel_addr)
{
    struct wined3d_sm4_data *priv = data;
    DWORD token = *(*ptr)++;
    UINT register_idx = *(*ptr)++;
    enum wined3d_sm4_register_type register_type;

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

    dst_param->reg.idx = register_idx;
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

static void shader_sm4_read_comment(const DWORD **ptr, const char **comment)
{
    FIXME("ptr %p, comment %p stub!\n", ptr, comment);
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
