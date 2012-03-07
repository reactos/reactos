/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

static const char * const shader_opcode_names[] =
{
    /* WINED3DSIH_ABS           */ "abs",
    /* WINED3DSIH_ADD           */ "add",
    /* WINED3DSIH_AND           */ "and",
    /* WINED3DSIH_BEM           */ "bem",
    /* WINED3DSIH_BREAK         */ "break",
    /* WINED3DSIH_BREAKC        */ "breakc",
    /* WINED3DSIH_BREAKP        */ "breakp",
    /* WINED3DSIH_CALL          */ "call",
    /* WINED3DSIH_CALLNZ        */ "callnz",
    /* WINED3DSIH_CMP           */ "cmp",
    /* WINED3DSIH_CND           */ "cnd",
    /* WINED3DSIH_CRS           */ "crs",
    /* WINED3DSIH_CUT           */ "cut",
    /* WINED3DSIH_DCL           */ "dcl",
    /* WINED3DSIH_DEF           */ "def",
    /* WINED3DSIH_DEFB          */ "defb",
    /* WINED3DSIH_DEFI          */ "defi",
    /* WINED3DSIH_DIV           */ "div",
    /* WINED3DSIH_DP2ADD        */ "dp2add",
    /* WINED3DSIH_DP3           */ "dp3",
    /* WINED3DSIH_DP4           */ "dp4",
    /* WINED3DSIH_DST           */ "dst",
    /* WINED3DSIH_DSX           */ "dsx",
    /* WINED3DSIH_DSY           */ "dsy",
    /* WINED3DSIH_ELSE          */ "else",
    /* WINED3DSIH_EMIT          */ "emit",
    /* WINED3DSIH_ENDIF         */ "endif",
    /* WINED3DSIH_ENDLOOP       */ "endloop",
    /* WINED3DSIH_ENDREP        */ "endrep",
    /* WINED3DSIH_EQ            */ "eq",
    /* WINED3DSIH_EXP           */ "exp",
    /* WINED3DSIH_EXPP          */ "expp",
    /* WINED3DSIH_FRC           */ "frc",
    /* WINED3DSIH_FTOI          */ "ftoi",
    /* WINED3DSIH_GE            */ "ge",
    /* WINED3DSIH_IADD          */ "iadd",
    /* WINED3DSIH_IEQ           */ "ieq",
    /* WINED3DSIH_IF            */ "if",
    /* WINED3DSIH_IFC           */ "ifc",
    /* WINED3DSIH_IGE           */ "ige",
    /* WINED3DSIH_IMUL          */ "imul",
    /* WINED3DSIH_ITOF          */ "itof",
    /* WINED3DSIH_LABEL         */ "label",
    /* WINED3DSIH_LD            */ "ld",
    /* WINED3DSIH_LIT           */ "lit",
    /* WINED3DSIH_LOG           */ "log",
    /* WINED3DSIH_LOGP          */ "logp",
    /* WINED3DSIH_LOOP          */ "loop",
    /* WINED3DSIH_LRP           */ "lrp",
    /* WINED3DSIH_LT            */ "lt",
    /* WINED3DSIH_M3x2          */ "m3x2",
    /* WINED3DSIH_M3x3          */ "m3x3",
    /* WINED3DSIH_M3x4          */ "m3x4",
    /* WINED3DSIH_M4x3          */ "m4x3",
    /* WINED3DSIH_M4x4          */ "m4x4",
    /* WINED3DSIH_MAD           */ "mad",
    /* WINED3DSIH_MAX           */ "max",
    /* WINED3DSIH_MIN           */ "min",
    /* WINED3DSIH_MOV           */ "mov",
    /* WINED3DSIH_MOVA          */ "mova",
    /* WINED3DSIH_MOVC          */ "movc",
    /* WINED3DSIH_MUL           */ "mul",
    /* WINED3DSIH_NOP           */ "nop",
    /* WINED3DSIH_NRM           */ "nrm",
    /* WINED3DSIH_PHASE         */ "phase",
    /* WINED3DSIH_POW           */ "pow",
    /* WINED3DSIH_RCP           */ "rcp",
    /* WINED3DSIH_REP           */ "rep",
    /* WINED3DSIH_RET           */ "ret",
    /* WINED3DSIH_ROUND_NI      */ "round_ni",
    /* WINED3DSIH_RSQ           */ "rsq",
    /* WINED3DSIH_SAMPLE        */ "sample",
    /* WINED3DSIH_SAMPLE_GRAD   */ "sample_d",
    /* WINED3DSIH_SAMPLE_LOD    */ "sample_l",
    /* WINED3DSIH_SETP          */ "setp",
    /* WINED3DSIH_SGE           */ "sge",
    /* WINED3DSIH_SGN           */ "sgn",
    /* WINED3DSIH_SINCOS        */ "sincos",
    /* WINED3DSIH_SLT           */ "slt",
    /* WINED3DSIH_SQRT          */ "sqrt",
    /* WINED3DSIH_SUB           */ "sub",
    /* WINED3DSIH_TEX           */ "texld",
    /* WINED3DSIH_TEXBEM        */ "texbem",
    /* WINED3DSIH_TEXBEML       */ "texbeml",
    /* WINED3DSIH_TEXCOORD      */ "texcrd",
    /* WINED3DSIH_TEXDEPTH      */ "texdepth",
    /* WINED3DSIH_TEXDP3        */ "texdp3",
    /* WINED3DSIH_TEXDP3TEX     */ "texdp3tex",
    /* WINED3DSIH_TEXKILL       */ "texkill",
    /* WINED3DSIH_TEXLDD        */ "texldd",
    /* WINED3DSIH_TEXLDL        */ "texldl",
    /* WINED3DSIH_TEXM3x2DEPTH  */ "texm3x2depth",
    /* WINED3DSIH_TEXM3x2PAD    */ "texm3x2pad",
    /* WINED3DSIH_TEXM3x2TEX    */ "texm3x2tex",
    /* WINED3DSIH_TEXM3x3       */ "texm3x3",
    /* WINED3DSIH_TEXM3x3DIFF   */ "texm3x3diff",
    /* WINED3DSIH_TEXM3x3PAD    */ "texm3x3pad",
    /* WINED3DSIH_TEXM3x3SPEC   */ "texm3x3spec",
    /* WINED3DSIH_TEXM3x3TEX    */ "texm3x3tex",
    /* WINED3DSIH_TEXM3x3VSPEC  */ "texm3x3vspec",
    /* WINED3DSIH_TEXREG2AR     */ "texreg2ar",
    /* WINED3DSIH_TEXREG2GB     */ "texreg2gb",
    /* WINED3DSIH_TEXREG2RGB    */ "texreg2rgb",
    /* WINED3DSIH_UDIV          */ "udiv",
    /* WINED3DSIH_USHR          */ "ushr",
    /* WINED3DSIH_UTOF          */ "utof",
    /* WINED3DSIH_XOR           */ "xor",
};

static const char * const semantic_names[] =
{
    /* WINED3DDECLUSAGE_POSITION        */ "SV_POSITION",
    /* WINED3DDECLUSAGE_BLENDWEIGHT     */ "BLENDWEIGHT",
    /* WINED3DDECLUSAGE_BLENDINDICES    */ "BLENDINDICES",
    /* WINED3DDECLUSAGE_NORMAL          */ "NORMAL",
    /* WINED3DDECLUSAGE_PSIZE           */ "PSIZE",
    /* WINED3DDECLUSAGE_TEXCOORD        */ "TEXCOORD",
    /* WINED3DDECLUSAGE_TANGENT         */ "TANGENT",
    /* WINED3DDECLUSAGE_BINORMAL        */ "BINORMAL",
    /* WINED3DDECLUSAGE_TESSFACTOR      */ "TESSFACTOR",
    /* WINED3DDECLUSAGE_POSITIONT       */ "POSITIONT",
    /* WINED3DDECLUSAGE_COLOR           */ "COLOR",
    /* WINED3DDECLUSAGE_FOG             */ "FOG",
    /* WINED3DDECLUSAGE_DEPTH           */ "DEPTH",
    /* WINED3DDECLUSAGE_SAMPLE          */ "SAMPLE",
};

static const char *shader_semantic_name_from_usage(WINED3DDECLUSAGE usage)
{
    if (usage >= sizeof(semantic_names) / sizeof(*semantic_names))
    {
        FIXME("Unrecognized usage %#x.\n", usage);
        return "UNRECOGNIZED";
    }

    return semantic_names[usage];
}

static WINED3DDECLUSAGE shader_usage_from_semantic_name(const char *name)
{
    unsigned int i;

    for (i = 0; i < sizeof(semantic_names) / sizeof(*semantic_names); ++i)
    {
        if (!strcmp(name, semantic_names[i])) return i;
    }

    return ~0U;
}

BOOL shader_match_semantic(const char *semantic_name, WINED3DDECLUSAGE usage)
{
    return !strcmp(semantic_name, shader_semantic_name_from_usage(usage));
}

static void shader_signature_from_semantic(struct wined3d_shader_signature_element *e,
        const struct wined3d_shader_semantic *s)
{
    e->semantic_name = shader_semantic_name_from_usage(s->usage);
    e->semantic_idx = s->usage_idx;
    e->sysval_semantic = 0;
    e->component_type = 0;
    e->register_idx = s->reg.reg.idx;
    e->mask = s->reg.write_mask;
}

static void shader_signature_from_usage(struct wined3d_shader_signature_element *e,
        WINED3DDECLUSAGE usage, UINT usage_idx, UINT reg_idx, DWORD write_mask)
{
    e->semantic_name = shader_semantic_name_from_usage(usage);
    e->semantic_idx = usage_idx;
    e->sysval_semantic = 0;
    e->component_type = 0;
    e->register_idx = reg_idx;
    e->mask = write_mask;
}

static const struct wined3d_shader_frontend *shader_select_frontend(DWORD version_token)
{
    switch (version_token >> 16)
    {
        case WINED3D_SM1_VS:
        case WINED3D_SM1_PS:
            return &sm1_shader_frontend;

        case WINED3D_SM4_PS:
        case WINED3D_SM4_VS:
        case WINED3D_SM4_GS:
            return &sm4_shader_frontend;

        default:
            FIXME("Unrecognised version token %#x\n", version_token);
            return NULL;
    }
}

void shader_buffer_clear(struct wined3d_shader_buffer *buffer)
{
    buffer->buffer[0] = '\0';
    buffer->bsize = 0;
    buffer->lineNo = 0;
    buffer->newline = TRUE;
}

BOOL shader_buffer_init(struct wined3d_shader_buffer *buffer)
{
    buffer->buffer = HeapAlloc(GetProcessHeap(), 0, SHADER_PGMSIZE);
    if (!buffer->buffer)
    {
        ERR("Failed to allocate shader buffer memory.\n");
        return FALSE;
    }

    shader_buffer_clear(buffer);
    return TRUE;
}

void shader_buffer_free(struct wined3d_shader_buffer *buffer)
{
    HeapFree(GetProcessHeap(), 0, buffer->buffer);
}

int shader_vaddline(struct wined3d_shader_buffer *buffer, const char *format, va_list args)
{
    char *base = buffer->buffer + buffer->bsize;
    int rc;

    rc = vsnprintf(base, SHADER_PGMSIZE - 1 - buffer->bsize, format, args);

    if (rc < 0 /* C89 */ || (unsigned int)rc > SHADER_PGMSIZE - 1 - buffer->bsize /* C99 */)
    {
        ERR("The buffer allocated for the shader program string "
            "is too small at %d bytes.\n", SHADER_PGMSIZE);
        buffer->bsize = SHADER_PGMSIZE - 1;
        return -1;
    }

    if (buffer->newline)
    {
        TRACE("GL HW (%u, %u) : %s", buffer->lineNo + 1, buffer->bsize, base);
        buffer->newline = FALSE;
    }
    else
    {
        TRACE("%s", base);
    }

    buffer->bsize += rc;
    if (buffer->buffer[buffer->bsize-1] == '\n')
    {
        ++buffer->lineNo;
        buffer->newline = TRUE;
    }

    return 0;
}

int shader_addline(struct wined3d_shader_buffer *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = shader_vaddline(buffer, format, args);
    va_end(args);

    return ret;
}

static void shader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    shader->ref = 1;
    shader->device = device;
    shader->parent = parent;
    shader->parent_ops = parent_ops;
    list_init(&shader->linked_programs);
    list_add_head(&device->shaders, &shader->shader_list_entry);
}

/* Convert floating point offset relative to a register file to an absolute
 * offset for float constants. */
static unsigned int shader_get_float_offset(enum wined3d_shader_register_type register_type, UINT register_idx)
{
    switch (register_type)
    {
        case WINED3DSPR_CONST: return register_idx;
        case WINED3DSPR_CONST2: return 2048 + register_idx;
        case WINED3DSPR_CONST3: return 4096 + register_idx;
        case WINED3DSPR_CONST4: return 6144 + register_idx;
        default:
            FIXME("Unsupported register type: %u.\n", register_type);
            return register_idx;
    }
}

static void shader_delete_constant_list(struct list *clist)
{
    struct wined3d_shader_lconst *constant;
    struct list *ptr;

    ptr = list_head(clist);
    while (ptr)
    {
        constant = LIST_ENTRY(ptr, struct wined3d_shader_lconst, entry);
        ptr = list_next(clist, ptr);
        HeapFree(GetProcessHeap(), 0, constant);
    }
    list_init(clist);
}

static inline void set_bitmap_bit(DWORD *bitmap, DWORD bit)
{
    DWORD idx, shift;
    idx = bit >> 5;
    shift = bit & 0x1f;
    bitmap[idx] |= (1 << shift);
}

static void shader_record_register_usage(struct wined3d_shader *shader, struct wined3d_shader_reg_maps *reg_maps,
        const struct wined3d_shader_register *reg, enum wined3d_shader_type shader_type)
{
    switch (reg->type)
    {
        case WINED3DSPR_TEXTURE: /* WINED3DSPR_ADDR */
            if (shader_type == WINED3D_SHADER_TYPE_PIXEL) reg_maps->texcoord |= 1 << reg->idx;
            else reg_maps->address |= 1 << reg->idx;
            break;

        case WINED3DSPR_TEMP:
            reg_maps->temporary |= 1 << reg->idx;
            break;

        case WINED3DSPR_INPUT:
            if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
            {
                if (reg->rel_addr)
                {
                    /* If relative addressing is used, we must assume that all registers
                     * are used. Even if it is a construct like v3[aL], we can't assume
                     * that v0, v1 and v2 aren't read because aL can be negative */
                    unsigned int i;
                    for (i = 0; i < MAX_REG_INPUT; ++i)
                    {
                        shader->u.ps.input_reg_used[i] = TRUE;
                    }
                }
                else
                {
                    shader->u.ps.input_reg_used[reg->idx] = TRUE;
                }
            }
            else reg_maps->input_registers |= 1 << reg->idx;
            break;

        case WINED3DSPR_RASTOUT:
            if (reg->idx == 1) reg_maps->fog = 1;
            break;

        case WINED3DSPR_MISCTYPE:
            if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
            {
                if (!reg->idx) reg_maps->vpos = 1;
                else if (reg->idx == 1) reg_maps->usesfacing = 1;
            }
            break;

        case WINED3DSPR_CONST:
            if (reg->rel_addr)
            {
                if (reg->idx < reg_maps->min_rel_offset) reg_maps->min_rel_offset = reg->idx;
                if (reg->idx > reg_maps->max_rel_offset) reg_maps->max_rel_offset = reg->idx;
                reg_maps->usesrelconstF = TRUE;
            }
            else
            {
                set_bitmap_bit(reg_maps->constf, reg->idx);
            }
            break;

        case WINED3DSPR_CONSTINT:
            reg_maps->integer_constants |= (1 << reg->idx);
            break;

        case WINED3DSPR_CONSTBOOL:
            reg_maps->boolean_constants |= (1 << reg->idx);
            break;

        case WINED3DSPR_COLOROUT:
            reg_maps->rt_mask |= (1 << reg->idx);
            break;

        default:
            TRACE("Not recording register of type %#x and idx %u\n", reg->type, reg->idx);
            break;
    }
}

static unsigned int get_instr_extra_regcount(enum WINED3D_SHADER_INSTRUCTION_HANDLER instr, unsigned int param)
{
    switch (instr)
    {
        case WINED3DSIH_M4x4:
        case WINED3DSIH_M3x4:
            return param == 1 ? 3 : 0;

        case WINED3DSIH_M4x3:
        case WINED3DSIH_M3x3:
            return param == 1 ? 2 : 0;

        case WINED3DSIH_M3x2:
            return param == 1 ? 1 : 0;

        default:
            return 0;
    }
}

/* Note that this does not count the loop register as an address register. */
static HRESULT shader_get_registers_used(struct wined3d_shader *shader, const struct wined3d_shader_frontend *fe,
        struct wined3d_shader_reg_maps *reg_maps, struct wined3d_shader_signature_element *input_signature,
        struct wined3d_shader_signature_element *output_signature, const DWORD *byte_code, DWORD constf_size)
{
    unsigned int cur_loop_depth = 0, max_loop_depth = 0;
    void *fe_data = shader->frontend_data;
    struct wined3d_shader_version shader_version;
    const DWORD *ptr = byte_code;

    memset(reg_maps, 0, sizeof(*reg_maps));
    reg_maps->min_rel_offset = ~0U;

    fe->shader_read_header(fe_data, &ptr, &shader_version);
    reg_maps->shader_version = shader_version;

    reg_maps->constf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(*reg_maps->constf) * ((constf_size + 31) / 32));
    if (!reg_maps->constf)
    {
        ERR("Failed to allocate constant map memory.\n");
        return E_OUTOFMEMORY;
    }

    while (!fe->shader_is_end(fe_data, &ptr))
    {
        struct wined3d_shader_instruction ins;
        const char *comment;
        UINT comment_size;
        UINT param_size;

        /* Skip comments. */
        fe->shader_read_comment(&ptr, &comment, &comment_size);
        if (comment) continue;

        /* Fetch opcode. */
        fe->shader_read_opcode(fe_data, &ptr, &ins, &param_size);

        /* Unhandled opcode, and its parameters. */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            ptr += param_size;
            continue;
        }

        /* Handle declarations. */
        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            struct wined3d_shader_semantic semantic;

            fe->shader_read_semantic(&ptr, &semantic);

            switch (semantic.reg.reg.type)
            {
                /* Mark input registers used. */
                case WINED3DSPR_INPUT:
                    reg_maps->input_registers |= 1 << semantic.reg.reg.idx;
                    shader_signature_from_semantic(&input_signature[semantic.reg.reg.idx], &semantic);
                    break;

                /* Vertex shader: mark 3.0 output registers used, save token. */
                case WINED3DSPR_OUTPUT:
                    reg_maps->output_registers |= 1 << semantic.reg.reg.idx;
                    shader_signature_from_semantic(&output_signature[semantic.reg.reg.idx], &semantic);
                    if (semantic.usage == WINED3DDECLUSAGE_FOG) reg_maps->fog = 1;
                    break;

                /* Save sampler usage token. */
                case WINED3DSPR_SAMPLER:
                    reg_maps->sampler_type[semantic.reg.reg.idx] = semantic.sampler_type;
                    break;

                default:
                    TRACE("Not recording DCL register type %#x.\n", semantic.reg.reg.type);
                    break;
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            struct wined3d_shader_src_param rel_addr;
            struct wined3d_shader_dst_param dst;

            struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
            if (!lconst) return E_OUTOFMEMORY;

            fe->shader_read_dst_param(fe_data, &ptr, &dst, &rel_addr);
            lconst->idx = dst.reg.idx;

            memcpy(lconst->value, ptr, 4 * sizeof(DWORD));
            ptr += 4;

            /* In pixel shader 1.X shaders, the constants are clamped between [-1;1] */
            if (shader_version.major == 1 && shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
            {
                float *value = (float *)lconst->value;
                if (value[0] < -1.0f) value[0] = -1.0f;
                else if (value[0] > 1.0f) value[0] = 1.0f;
                if (value[1] < -1.0f) value[1] = -1.0f;
                else if (value[1] > 1.0f) value[1] = 1.0f;
                if (value[2] < -1.0f) value[2] = -1.0f;
                else if (value[2] > 1.0f) value[2] = 1.0f;
                if (value[3] < -1.0f) value[3] = -1.0f;
                else if (value[3] > 1.0f) value[3] = 1.0f;
            }

            list_add_head(&shader->constantsF, &lconst->entry);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            struct wined3d_shader_src_param rel_addr;
            struct wined3d_shader_dst_param dst;

            struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
            if (!lconst) return E_OUTOFMEMORY;

            fe->shader_read_dst_param(fe_data, &ptr, &dst, &rel_addr);
            lconst->idx = dst.reg.idx;

            memcpy(lconst->value, ptr, 4 * sizeof(DWORD));
            ptr += 4;

            list_add_head(&shader->constantsI, &lconst->entry);
            reg_maps->local_int_consts |= (1 << dst.reg.idx);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            struct wined3d_shader_src_param rel_addr;
            struct wined3d_shader_dst_param dst;

            struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
            if (!lconst) return E_OUTOFMEMORY;

            fe->shader_read_dst_param(fe_data, &ptr, &dst, &rel_addr);
            lconst->idx = dst.reg.idx;

            memcpy(lconst->value, ptr, sizeof(DWORD));
            ++ptr;

            list_add_head(&shader->constantsB, &lconst->entry);
            reg_maps->local_bool_consts |= (1 << dst.reg.idx);
        }
        /* For subroutine prototypes. */
        else if (ins.handler_idx == WINED3DSIH_LABEL)
        {
            struct wined3d_shader_src_param src, rel_addr;

            fe->shader_read_src_param(fe_data, &ptr, &src, &rel_addr);
            reg_maps->labels |= 1 << src.reg.idx;
        }
        /* Set texture, address, temporary registers. */
        else
        {
            BOOL color0_mov = FALSE;
            unsigned int i, limit;

            /* This will loop over all the registers and try to
             * make a bitmask of the ones we're interested in.
             *
             * Relative addressing tokens are ignored, but that's
             * okay, since we'll catch any address registers when
             * they are initialized (required by spec). */
            for (i = 0; i < ins.dst_count; ++i)
            {
                struct wined3d_shader_src_param dst_rel_addr;
                struct wined3d_shader_dst_param dst_param;

                fe->shader_read_dst_param(fe_data, &ptr, &dst_param, &dst_rel_addr);

                shader_record_register_usage(shader, reg_maps, &dst_param.reg, shader_version.type);

                /* WINED3DSPR_TEXCRDOUT is the same as WINED3DSPR_OUTPUT. _OUTPUT can be > MAX_REG_TEXCRD and
                 * is used in >= 3.0 shaders. Filter 3.0 shaders to prevent overflows, and also filter pixel
                 * shaders because TECRDOUT isn't used in them, but future register types might cause issues */
                if (shader_version.type == WINED3D_SHADER_TYPE_VERTEX && shader_version.major < 3)
                {
                    UINT idx = dst_param.reg.idx;

                    switch (dst_param.reg.type)
                    {
                        case WINED3DSPR_RASTOUT:
                            switch (idx)
                            {
                                case 0: /* oPos */
                                    reg_maps->output_registers |= 1 << 10;
                                    shader_signature_from_usage(&output_signature[10],
                                            WINED3DDECLUSAGE_POSITION, 0, 10, WINED3DSP_WRITEMASK_ALL);
                                    break;

                                case 1: /* oFog */
                                    reg_maps->output_registers |= 1 << 11;
                                    shader_signature_from_usage(&output_signature[11],
                                            WINED3DDECLUSAGE_FOG, 0, 11, WINED3DSP_WRITEMASK_0);
                                    break;

                                case 2: /* oPts */
                                    reg_maps->output_registers |= 1 << 11;
                                    shader_signature_from_usage(&output_signature[11],
                                            WINED3DDECLUSAGE_PSIZE, 0, 11, WINED3DSP_WRITEMASK_1);
                                    break;
                            }
                            break;

                        case WINED3DSPR_ATTROUT:
                            if (idx < 2)
                            {
                                idx += 8;
                                if (reg_maps->output_registers & (1 << idx))
                                {
                                    output_signature[idx].mask |= dst_param.write_mask;
                                }
                                else
                                {
                                    reg_maps->output_registers |= 1 << idx;
                                    shader_signature_from_usage(&output_signature[idx],
                                            WINED3DDECLUSAGE_COLOR, idx - 8, idx, dst_param.write_mask);
                                }
                            }
                            break;

                        case WINED3DSPR_TEXCRDOUT:

                            reg_maps->texcoord_mask[idx] |= dst_param.write_mask;
                            if (reg_maps->output_registers & (1 << idx))
                            {
                                output_signature[idx].mask |= dst_param.write_mask;
                            }
                            else
                            {
                                reg_maps->output_registers |= 1 << idx;
                                shader_signature_from_usage(&output_signature[idx],
                                        WINED3DDECLUSAGE_TEXCOORD, idx, idx, dst_param.write_mask);
                            }
                            break;

                        default:
                            break;
                    }
                }

                if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
                {
                    if (dst_param.reg.type == WINED3DSPR_COLOROUT && !dst_param.reg.idx)
                    {
                        /* Many 2.0 and 3.0 pixel shaders end with a MOV from a temp register to
                         * COLOROUT 0. If we know this in advance, the ARB shader backend can skip
                         * the mov and perform the sRGB write correction from the source register.
                         *
                         * However, if the mov is only partial, we can't do this, and if the write
                         * comes from an instruction other than MOV it is hard to do as well. If
                         * COLOROUT 0 is overwritten partially later, the marker is dropped again. */
                        shader->u.ps.color0_mov = FALSE;
                        if (ins.handler_idx == WINED3DSIH_MOV
                                && dst_param.write_mask == WINED3DSP_WRITEMASK_ALL)
                        {
                            /* Used later when the source register is read. */
                            color0_mov = TRUE;
                        }
                    }
                    /* Also drop the MOV marker if the source register is overwritten prior to the shader
                     * end
                     */
                    else if (dst_param.reg.type == WINED3DSPR_TEMP
                            && dst_param.reg.idx == shader->u.ps.color0_reg)
                    {
                        shader->u.ps.color0_mov = FALSE;
                    }
                }

                /* Declare 1.x samplers implicitly, based on the destination reg. number. */
                if (shader_version.major == 1
                        && (ins.handler_idx == WINED3DSIH_TEX
                            || ins.handler_idx == WINED3DSIH_TEXBEM
                            || ins.handler_idx == WINED3DSIH_TEXBEML
                            || ins.handler_idx == WINED3DSIH_TEXDP3TEX
                            || ins.handler_idx == WINED3DSIH_TEXM3x2TEX
                            || ins.handler_idx == WINED3DSIH_TEXM3x3SPEC
                            || ins.handler_idx == WINED3DSIH_TEXM3x3TEX
                            || ins.handler_idx == WINED3DSIH_TEXM3x3VSPEC
                            || ins.handler_idx == WINED3DSIH_TEXREG2AR
                            || ins.handler_idx == WINED3DSIH_TEXREG2GB
                            || ins.handler_idx == WINED3DSIH_TEXREG2RGB))
                {
                    /* Fake sampler usage, only set reserved bit and type. */
                    DWORD sampler_code = dst_param.reg.idx;

                    TRACE("Setting fake 2D sampler for 1.x pixelshader.\n");
                    reg_maps->sampler_type[sampler_code] = WINED3DSTT_2D;

                    /* texbem is only valid with < 1.4 pixel shaders */
                    if (ins.handler_idx == WINED3DSIH_TEXBEM
                            || ins.handler_idx == WINED3DSIH_TEXBEML)
                    {
                        reg_maps->bumpmat |= 1 << dst_param.reg.idx;
                        if (ins.handler_idx == WINED3DSIH_TEXBEML)
                        {
                            reg_maps->luminanceparams |= 1 << dst_param.reg.idx;
                        }
                    }
                }
                else if (ins.handler_idx == WINED3DSIH_BEM)
                {
                    reg_maps->bumpmat |= 1 << dst_param.reg.idx;
                }
            }

            if (ins.handler_idx == WINED3DSIH_NRM) reg_maps->usesnrm = 1;
            else if (ins.handler_idx == WINED3DSIH_DSY) reg_maps->usesdsy = 1;
            else if (ins.handler_idx == WINED3DSIH_DSX) reg_maps->usesdsx = 1;
            else if (ins.handler_idx == WINED3DSIH_TEXLDD) reg_maps->usestexldd = 1;
            else if (ins.handler_idx == WINED3DSIH_TEXLDL) reg_maps->usestexldl = 1;
            else if (ins.handler_idx == WINED3DSIH_MOVA) reg_maps->usesmova = 1;
            else if (ins.handler_idx == WINED3DSIH_IFC) reg_maps->usesifc = 1;
            else if (ins.handler_idx == WINED3DSIH_CALL) reg_maps->usescall = 1;
            else if (ins.handler_idx == WINED3DSIH_POW) reg_maps->usespow = 1;
            else if (ins.handler_idx == WINED3DSIH_LOOP
                    || ins.handler_idx == WINED3DSIH_REP)
            {
                ++cur_loop_depth;
                if (cur_loop_depth > max_loop_depth)
                    max_loop_depth = cur_loop_depth;
            }
            else if (ins.handler_idx == WINED3DSIH_ENDLOOP
                    || ins.handler_idx == WINED3DSIH_ENDREP)
                --cur_loop_depth;

            limit = ins.src_count + (ins.predicate ? 1 : 0);
            for (i = 0; i < limit; ++i)
            {
                struct wined3d_shader_src_param src_param, src_rel_addr;
                unsigned int count;

                fe->shader_read_src_param(fe_data, &ptr, &src_param, &src_rel_addr);
                count = get_instr_extra_regcount(ins.handler_idx, i);

                shader_record_register_usage(shader, reg_maps, &src_param.reg, shader_version.type);
                while (count)
                {
                    ++src_param.reg.idx;
                    shader_record_register_usage(shader, reg_maps, &src_param.reg, shader_version.type);
                    --count;
                }

                if (color0_mov)
                {
                    if (src_param.reg.type == WINED3DSPR_TEMP
                            && src_param.swizzle == WINED3DSP_NOSWIZZLE)
                    {
                        shader->u.ps.color0_mov = TRUE;
                        shader->u.ps.color0_reg = src_param.reg.idx;
                    }
                }
            }
        }
    }
    reg_maps->loop_depth = max_loop_depth;

    /* PS before 2.0 don't have explicit color outputs. Instead the value of
     * R0 is written to the render target. */
    if (shader_version.major < 2 && shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
        reg_maps->rt_mask |= (1 << 0);

    shader->functionLength = ((const char *)ptr - (const char *)byte_code);

    return WINED3D_OK;
}

unsigned int shader_find_free_input_register(const struct wined3d_shader_reg_maps *reg_maps, unsigned int max)
{
    DWORD map = 1 << max;
    map |= map - 1;
    map &= reg_maps->shader_version.major < 3 ? ~reg_maps->texcoord : ~reg_maps->input_registers;

    return wined3d_log2i(map);
}

static void shader_dump_decl_usage(const struct wined3d_shader_semantic *semantic,
        const struct wined3d_shader_version *shader_version)
{
    TRACE("dcl");

    if (semantic->reg.reg.type == WINED3DSPR_SAMPLER)
    {
        switch (semantic->sampler_type)
        {
            case WINED3DSTT_2D: TRACE("_2d"); break;
            case WINED3DSTT_CUBE: TRACE("_cube"); break;
            case WINED3DSTT_VOLUME: TRACE("_volume"); break;
            default: TRACE("_unknown_ttype(0x%08x)", semantic->sampler_type);
        }
    }
    else
    {
        /* Pixel shaders 3.0 don't have usage semantics. */
        if (shader_version->major < 3 && shader_version->type == WINED3D_SHADER_TYPE_PIXEL) return;
        else TRACE("_");

        switch (semantic->usage)
        {
            case WINED3DDECLUSAGE_POSITION:
                TRACE("position%u", semantic->usage_idx);
                break;

            case WINED3DDECLUSAGE_BLENDINDICES:
                TRACE("blend");
                break;

            case WINED3DDECLUSAGE_BLENDWEIGHT:
                TRACE("weight");
                break;

            case WINED3DDECLUSAGE_NORMAL:
                TRACE("normal%u", semantic->usage_idx);
                break;

            case WINED3DDECLUSAGE_PSIZE:
                TRACE("psize");
                break;

            case WINED3DDECLUSAGE_COLOR:
                if (!semantic->usage_idx) TRACE("color");
                else TRACE("specular%u", (semantic->usage_idx - 1));
                break;

            case WINED3DDECLUSAGE_TEXCOORD:
                TRACE("texture%u", semantic->usage_idx);
                break;

            case WINED3DDECLUSAGE_TANGENT:
                TRACE("tangent");
                break;

            case WINED3DDECLUSAGE_BINORMAL:
                TRACE("binormal");
                break;

            case WINED3DDECLUSAGE_TESSFACTOR:
                TRACE("tessfactor");
                break;

            case WINED3DDECLUSAGE_POSITIONT:
                TRACE("positionT%u", semantic->usage_idx);
                break;

            case WINED3DDECLUSAGE_FOG:
                TRACE("fog");
                break;

            case WINED3DDECLUSAGE_DEPTH:
                TRACE("depth");
                break;

            case WINED3DDECLUSAGE_SAMPLE:
                TRACE("sample");
                break;

            default:
                FIXME("unknown_semantics(0x%08x)", semantic->usage);
        }
    }
}

static void shader_dump_register(const struct wined3d_shader_register *reg,
        const struct wined3d_shader_version *shader_version)
{
    static const char * const rastout_reg_names[] = {"oPos", "oFog", "oPts"};
    static const char * const misctype_reg_names[] = {"vPos", "vFace"};
    UINT offset = reg->idx;

    switch (reg->type)
    {
        case WINED3DSPR_TEMP:
            TRACE("r");
            break;

        case WINED3DSPR_INPUT:
            TRACE("v");
            break;

        case WINED3DSPR_CONST:
        case WINED3DSPR_CONST2:
        case WINED3DSPR_CONST3:
        case WINED3DSPR_CONST4:
            TRACE("c");
            offset = shader_get_float_offset(reg->type, reg->idx);
            break;

        case WINED3DSPR_TEXTURE: /* vs: case WINED3DSPR_ADDR */
            TRACE("%c", shader_version->type == WINED3D_SHADER_TYPE_PIXEL ? 't' : 'a');
            break;

        case WINED3DSPR_RASTOUT:
            TRACE("%s", rastout_reg_names[reg->idx]);
            break;

        case WINED3DSPR_COLOROUT:
            TRACE("oC");
            break;

        case WINED3DSPR_DEPTHOUT:
            TRACE("oDepth");
            break;

        case WINED3DSPR_ATTROUT:
            TRACE("oD");
            break;

        case WINED3DSPR_TEXCRDOUT:
            /* Vertex shaders >= 3.0 use general purpose output registers
             * (WINED3DSPR_OUTPUT), which can include an address token. */
            if (shader_version->major >= 3) TRACE("o");
            else TRACE("oT");
            break;

        case WINED3DSPR_CONSTINT:
            TRACE("i");
            break;

        case WINED3DSPR_CONSTBOOL:
            TRACE("b");
            break;

        case WINED3DSPR_LABEL:
            TRACE("l");
            break;

        case WINED3DSPR_LOOP:
            TRACE("aL");
            break;

        case WINED3DSPR_SAMPLER:
            TRACE("s");
            break;

        case WINED3DSPR_MISCTYPE:
            if (reg->idx > 1) FIXME("Unhandled misctype register %u.\n", reg->idx);
            else TRACE("%s", misctype_reg_names[reg->idx]);
            break;

        case WINED3DSPR_PREDICATE:
            TRACE("p");
            break;

        case WINED3DSPR_IMMCONST:
            TRACE("l");
            break;

        case WINED3DSPR_CONSTBUFFER:
            TRACE("cb");
            break;

        case WINED3DSPR_NULL:
            TRACE("null");
            break;

        case WINED3DSPR_RESOURCE:
            TRACE("t");
            break;

        default:
            TRACE("unhandled_rtype(%#x)", reg->type);
            break;
    }

    if (reg->type == WINED3DSPR_IMMCONST)
    {
        TRACE("(");
        switch (reg->immconst_type)
        {
            case WINED3D_IMMCONST_SCALAR:
                TRACE("%.8e", *(const float *)reg->immconst_data);
                break;

            case WINED3D_IMMCONST_VEC4:
                TRACE("%.8e, %.8e, %.8e, %.8e",
                        *(const float *)&reg->immconst_data[0], *(const float *)&reg->immconst_data[1],
                        *(const float *)&reg->immconst_data[2], *(const float *)&reg->immconst_data[3]);
                break;

            default:
                TRACE("<unhandled immconst_type %#x>", reg->immconst_type);
                break;
        }
        TRACE(")");
    }
    else if (reg->type != WINED3DSPR_RASTOUT
            && reg->type != WINED3DSPR_MISCTYPE
            && reg->type != WINED3DSPR_NULL)
    {
        if (reg->array_idx != ~0U)
        {
            TRACE("%u[%u", offset, reg->array_idx);
            if (reg->rel_addr)
            {
                TRACE(" + ");
                shader_dump_src_param(reg->rel_addr, shader_version);
            }
            TRACE("]");
        }
        else
        {
            if (reg->rel_addr)
            {
                TRACE("[");
                shader_dump_src_param(reg->rel_addr, shader_version);
                TRACE(" + ");
            }
            TRACE("%u", offset);
            if (reg->rel_addr) TRACE("]");
        }
    }
}

void shader_dump_dst_param(const struct wined3d_shader_dst_param *param,
        const struct wined3d_shader_version *shader_version)
{
    DWORD write_mask = param->write_mask;

    shader_dump_register(&param->reg, shader_version);

    if (write_mask && write_mask != WINED3DSP_WRITEMASK_ALL)
    {
        static const char *write_mask_chars = "xyzw";

        TRACE(".");
        if (write_mask & WINED3DSP_WRITEMASK_0) TRACE("%c", write_mask_chars[0]);
        if (write_mask & WINED3DSP_WRITEMASK_1) TRACE("%c", write_mask_chars[1]);
        if (write_mask & WINED3DSP_WRITEMASK_2) TRACE("%c", write_mask_chars[2]);
        if (write_mask & WINED3DSP_WRITEMASK_3) TRACE("%c", write_mask_chars[3]);
    }
}

void shader_dump_src_param(const struct wined3d_shader_src_param *param,
        const struct wined3d_shader_version *shader_version)
{
    enum wined3d_shader_src_modifier src_modifier = param->modifiers;
    DWORD swizzle = param->swizzle;

    if (src_modifier == WINED3DSPSM_NEG
            || src_modifier == WINED3DSPSM_BIASNEG
            || src_modifier == WINED3DSPSM_SIGNNEG
            || src_modifier == WINED3DSPSM_X2NEG
            || src_modifier == WINED3DSPSM_ABSNEG)
        TRACE("-");
    else if (src_modifier == WINED3DSPSM_COMP)
        TRACE("1-");
    else if (src_modifier == WINED3DSPSM_NOT)
        TRACE("!");

    if (src_modifier == WINED3DSPSM_ABS || src_modifier == WINED3DSPSM_ABSNEG)
        TRACE("abs(");

    shader_dump_register(&param->reg, shader_version);

    if (src_modifier)
    {
        switch (src_modifier)
        {
            case WINED3DSPSM_NONE:    break;
            case WINED3DSPSM_NEG:     break;
            case WINED3DSPSM_NOT:     break;
            case WINED3DSPSM_BIAS:    TRACE("_bias"); break;
            case WINED3DSPSM_BIASNEG: TRACE("_bias"); break;
            case WINED3DSPSM_SIGN:    TRACE("_bx2"); break;
            case WINED3DSPSM_SIGNNEG: TRACE("_bx2"); break;
            case WINED3DSPSM_COMP:    break;
            case WINED3DSPSM_X2:      TRACE("_x2"); break;
            case WINED3DSPSM_X2NEG:   TRACE("_x2"); break;
            case WINED3DSPSM_DZ:      TRACE("_dz"); break;
            case WINED3DSPSM_DW:      TRACE("_dw"); break;
            case WINED3DSPSM_ABSNEG:  TRACE(")"); break;
            case WINED3DSPSM_ABS:     TRACE(")"); break;
            default:                  TRACE("_unknown_modifier(%#x)", src_modifier);
        }
    }

    if (swizzle != WINED3DSP_NOSWIZZLE)
    {
        static const char *swizzle_chars = "xyzw";
        DWORD swizzle_x = swizzle & 0x03;
        DWORD swizzle_y = (swizzle >> 2) & 0x03;
        DWORD swizzle_z = (swizzle >> 4) & 0x03;
        DWORD swizzle_w = (swizzle >> 6) & 0x03;

        if (swizzle_x == swizzle_y
                && swizzle_x == swizzle_z
                && swizzle_x == swizzle_w)
        {
            TRACE(".%c", swizzle_chars[swizzle_x]);
        }
        else
        {
            TRACE(".%c%c%c%c", swizzle_chars[swizzle_x], swizzle_chars[swizzle_y],
                    swizzle_chars[swizzle_z], swizzle_chars[swizzle_w]);
        }
    }
}

/* Shared code in order to generate the bulk of the shader string.
 * NOTE: A description of how to parse tokens can be found on MSDN. */
void shader_generate_main(const struct wined3d_shader *shader, struct wined3d_shader_buffer *buffer,
        const struct wined3d_shader_reg_maps *reg_maps, const DWORD *byte_code, void *backend_ctx)
{
    struct wined3d_device *device = shader->device;
    const struct wined3d_shader_frontend *fe = shader->frontend;
    void *fe_data = shader->frontend_data;
    struct wined3d_shader_src_param dst_rel_addr[2];
    struct wined3d_shader_src_param src_rel_addr[4];
    struct wined3d_shader_dst_param dst_param[2];
    struct wined3d_shader_src_param src_param[4];
    struct wined3d_shader_version shader_version;
    struct wined3d_shader_loop_state loop_state;
    struct wined3d_shader_instruction ins;
    struct wined3d_shader_tex_mx tex_mx;
    struct wined3d_shader_context ctx;
    const DWORD *ptr = byte_code;
    DWORD i;

    /* Initialize current parsing state. */
    tex_mx.current_row = 0;
    loop_state.current_depth = 0;
    loop_state.current_reg = 0;

    ctx.shader = shader;
    ctx.gl_info = &device->adapter->gl_info;
    ctx.reg_maps = reg_maps;
    ctx.buffer = buffer;
    ctx.tex_mx = &tex_mx;
    ctx.loop_state = &loop_state;
    ctx.backend_data = backend_ctx;

    ins.ctx = &ctx;
    ins.dst = dst_param;
    ins.src = src_param;

    fe->shader_read_header(fe_data, &ptr, &shader_version);

    while (!fe->shader_is_end(fe_data, &ptr))
    {
        const char *comment;
        UINT comment_size;
        UINT param_size;

        /* Skip comment tokens. */
        fe->shader_read_comment(&ptr, &comment, &comment_size);
        if (comment) continue;

        /* Read opcode. */
        fe->shader_read_opcode(fe_data, &ptr, &ins, &param_size);

        /* Unknown opcode and its parameters. */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            ptr += param_size;
            continue;
        }

        /* Nothing to do. */
        if (ins.handler_idx == WINED3DSIH_DCL
                || ins.handler_idx == WINED3DSIH_NOP
                || ins.handler_idx == WINED3DSIH_DEF
                || ins.handler_idx == WINED3DSIH_DEFI
                || ins.handler_idx == WINED3DSIH_DEFB
                || ins.handler_idx == WINED3DSIH_PHASE)
        {
            ptr += param_size;
            continue;
        }

        /* Destination tokens */
        for (i = 0; i < ins.dst_count; ++i)
        {
            fe->shader_read_dst_param(fe_data, &ptr, &dst_param[i], &dst_rel_addr[i]);
        }

        /* Predication token */
        if (ins.predicate)
        {
            FIXME("Predicates not implemented.\n");
            ins.predicate = *ptr++;
        }

        /* Other source tokens */
        for (i = 0; i < ins.src_count; ++i)
        {
            fe->shader_read_src_param(fe_data, &ptr, &src_param[i], &src_rel_addr[i]);
        }

        /* Call appropriate function for output target */
        device->shader_backend->shader_handle_instruction(&ins);
    }
}

static void shader_dump_ins_modifiers(const struct wined3d_shader_dst_param *dst)
{
    DWORD mmask = dst->modifiers;

    switch (dst->shift)
    {
        case 0: break;
        case 13: TRACE("_d8"); break;
        case 14: TRACE("_d4"); break;
        case 15: TRACE("_d2"); break;
        case 1: TRACE("_x2"); break;
        case 2: TRACE("_x4"); break;
        case 3: TRACE("_x8"); break;
        default: TRACE("_unhandled_shift(%d)", dst->shift); break;
    }

    if (mmask & WINED3DSPDM_SATURATE)         TRACE("_sat");
    if (mmask & WINED3DSPDM_PARTIALPRECISION) TRACE("_pp");
    if (mmask & WINED3DSPDM_MSAMPCENTROID)    TRACE("_centroid");

    mmask &= ~(WINED3DSPDM_SATURATE | WINED3DSPDM_PARTIALPRECISION | WINED3DSPDM_MSAMPCENTROID);
    if (mmask) FIXME("_unrecognized_modifier(%#x)", mmask);
}

static void shader_trace_init(const struct wined3d_shader_frontend *fe, void *fe_data, const DWORD *byte_code)
{
    struct wined3d_shader_version shader_version;
    const DWORD *ptr = byte_code;
    const char *type_prefix;
    DWORD i;

    TRACE("Parsing %p.\n", byte_code);

    fe->shader_read_header(fe_data, &ptr, &shader_version);

    switch (shader_version.type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            type_prefix = "vs";
            break;

        case WINED3D_SHADER_TYPE_GEOMETRY:
            type_prefix = "gs";
            break;

        case WINED3D_SHADER_TYPE_PIXEL:
            type_prefix = "ps";
            break;

        default:
            FIXME("Unhandled shader type %#x.\n", shader_version.type);
            type_prefix = "unknown";
            break;
    }

    TRACE("%s_%u_%u\n", type_prefix, shader_version.major, shader_version.minor);

    while (!fe->shader_is_end(fe_data, &ptr))
    {
        struct wined3d_shader_instruction ins;
        const char *comment;
        UINT comment_size;
        UINT param_size;

        /* comment */
        fe->shader_read_comment(&ptr, &comment, &comment_size);
        if (comment)
        {
            if (comment_size > 4 && *(const DWORD *)comment == WINEMAKEFOURCC('T', 'E', 'X', 'T'))
            {
                const char *end = comment + comment_size;
                const char *ptr = comment + 4;
                const char *line = ptr;

                TRACE("// TEXT\n");
                while (ptr != end)
                {
                    if (*ptr == '\n')
                    {
                        UINT len = ptr - line;
                        if (len && *(ptr - 1) == '\r') --len;
                        TRACE("// %s\n", debugstr_an(line, len));
                        line = ++ptr;
                    }
                    else ++ptr;
                }
                if (line != ptr) TRACE("// %s\n", debugstr_an(line, ptr - line));
            }
            else TRACE("// %s\n", debugstr_an(comment, comment_size));
            continue;
        }

        fe->shader_read_opcode(fe_data, &ptr, &ins, &param_size);
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            ptr += param_size;
            continue;
        }

        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            struct wined3d_shader_semantic semantic;

            fe->shader_read_semantic(&ptr, &semantic);

            shader_dump_decl_usage(&semantic, &shader_version);
            shader_dump_ins_modifiers(&semantic.reg);
            TRACE(" ");
            shader_dump_dst_param(&semantic.reg, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            struct wined3d_shader_src_param rel_addr;
            struct wined3d_shader_dst_param dst;

            fe->shader_read_dst_param(fe_data, &ptr, &dst, &rel_addr);

            TRACE("def c%u = %f, %f, %f, %f", shader_get_float_offset(dst.reg.type, dst.reg.idx),
                    *(const float *)(ptr),
                    *(const float *)(ptr + 1),
                    *(const float *)(ptr + 2),
                    *(const float *)(ptr + 3));
            ptr += 4;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            struct wined3d_shader_src_param rel_addr;
            struct wined3d_shader_dst_param dst;

            fe->shader_read_dst_param(fe_data, &ptr, &dst, &rel_addr);

            TRACE("defi i%u = %d, %d, %d, %d", dst.reg.idx,
                    *(ptr),
                    *(ptr + 1),
                    *(ptr + 2),
                    *(ptr + 3));
            ptr += 4;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            struct wined3d_shader_src_param rel_addr;
            struct wined3d_shader_dst_param dst;

            fe->shader_read_dst_param(fe_data, &ptr, &dst, &rel_addr);

            TRACE("defb b%u = %s", dst.reg.idx, *ptr ? "true" : "false");
            ++ptr;
        }
        else
        {
            struct wined3d_shader_src_param dst_rel_addr[2];
            struct wined3d_shader_src_param src_rel_addr;
            struct wined3d_shader_dst_param dst_param[2];
            struct wined3d_shader_src_param src_param;

            for (i = 0; i < ins.dst_count; ++i)
            {
                fe->shader_read_dst_param(fe_data, &ptr, &dst_param[i], &dst_rel_addr[i]);
            }

            /* Print out predication source token first - it follows
             * the destination token. */
            if (ins.predicate)
            {
                fe->shader_read_src_param(fe_data, &ptr, &src_param, &src_rel_addr);
                TRACE("(");
                shader_dump_src_param(&src_param, &shader_version);
                TRACE(") ");
            }

            /* PixWin marks instructions with the coissue flag with a '+' */
            if (ins.coissue) TRACE("+");

            TRACE("%s", shader_opcode_names[ins.handler_idx]);

            if (ins.handler_idx == WINED3DSIH_IFC
                    || ins.handler_idx == WINED3DSIH_BREAKC)
            {
                switch (ins.flags)
                {
                    case WINED3D_SHADER_REL_OP_GT: TRACE("_gt"); break;
                    case WINED3D_SHADER_REL_OP_EQ: TRACE("_eq"); break;
                    case WINED3D_SHADER_REL_OP_GE: TRACE("_ge"); break;
                    case WINED3D_SHADER_REL_OP_LT: TRACE("_lt"); break;
                    case WINED3D_SHADER_REL_OP_NE: TRACE("_ne"); break;
                    case WINED3D_SHADER_REL_OP_LE: TRACE("_le"); break;
                    default: TRACE("_(%u)", ins.flags);
                }
            }
            else if (ins.handler_idx == WINED3DSIH_TEX
                    && shader_version.major >= 2
                    && (ins.flags & WINED3DSI_TEXLD_PROJECT))
            {
                TRACE("p");
            }

            /* We already read the destination tokens, print them. */
            for (i = 0; i < ins.dst_count; ++i)
            {
                shader_dump_ins_modifiers(&dst_param[i]);
                TRACE(!i ? " " : ", ");
                shader_dump_dst_param(&dst_param[i], &shader_version);
            }

            /* Other source tokens */
            for (i = ins.dst_count; i < (ins.dst_count + ins.src_count); ++i)
            {
                fe->shader_read_src_param(fe_data, &ptr, &src_param, &src_rel_addr);
                TRACE(!i ? " " : ", ");
                shader_dump_src_param(&src_param, &shader_version);
            }
        }
        TRACE("\n");
    }
}

static void shader_cleanup(struct wined3d_shader *shader)
{
    shader->device->shader_backend->shader_destroy(shader);
    HeapFree(GetProcessHeap(), 0, shader->reg_maps.constf);
    HeapFree(GetProcessHeap(), 0, shader->function);
    shader_delete_constant_list(&shader->constantsF);
    shader_delete_constant_list(&shader->constantsB);
    shader_delete_constant_list(&shader->constantsI);
    list_remove(&shader->shader_list_entry);

    if (shader->frontend && shader->frontend_data)
        shader->frontend->shader_free(shader->frontend_data);
}

static void shader_none_handle_instruction(const struct wined3d_shader_instruction *ins) {}
static void shader_none_select(const struct wined3d_context *context, BOOL usePS, BOOL useVS) {}
static void shader_none_select_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info,
        enum tex_types tex_type, const SIZE *ds_mask_size) {}
static void shader_none_deselect_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info) {}
static void shader_none_update_float_vertex_constants(struct wined3d_device *device, UINT start, UINT count) {}
static void shader_none_update_float_pixel_constants(struct wined3d_device *device, UINT start, UINT count) {}
static void shader_none_load_constants(const struct wined3d_context *context, char usePS, char useVS) {}
static void shader_none_load_np2fixup_constants(void *shader_priv,
        const struct wined3d_gl_info *gl_info, const struct wined3d_state *state) {}
static void shader_none_destroy(struct wined3d_shader *shader) {}
static HRESULT shader_none_alloc(struct wined3d_device *device) {return WINED3D_OK;}
static void shader_none_free(struct wined3d_device *device) {}
static void shader_none_context_destroyed(void *shader_priv, const struct wined3d_context *context) {}

static void shader_none_get_caps(const struct wined3d_gl_info *gl_info, struct shader_caps *caps)
{
    /* Set the shader caps to 0 for the none shader backend */
    caps->VertexShaderVersion = 0;
    caps->MaxVertexShaderConst = 0;
    caps->PixelShaderVersion = 0;
    caps->PixelShader1xMaxValue = 0.0f;
    caps->MaxPixelShaderConst = 0;
    caps->VSClipping = FALSE;
}

static BOOL shader_none_color_fixup_supported(struct color_fixup_desc fixup)
{
    if (TRACE_ON(d3d_shader) && TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    /* Faked to make some apps happy. */
    if (!is_complex_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    TRACE("[FAILED]\n");
    return FALSE;
}

const struct wined3d_shader_backend_ops none_shader_backend =
{
    shader_none_handle_instruction,
    shader_none_select,
    shader_none_select_depth_blt,
    shader_none_deselect_depth_blt,
    shader_none_update_float_vertex_constants,
    shader_none_update_float_pixel_constants,
    shader_none_load_constants,
    shader_none_load_np2fixup_constants,
    shader_none_destroy,
    shader_none_alloc,
    shader_none_free,
    shader_none_context_destroyed,
    shader_none_get_caps,
    shader_none_color_fixup_supported,
};

static HRESULT shader_set_function(struct wined3d_shader *shader, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, DWORD float_const_count,
        enum wined3d_shader_type type, unsigned int max_version)
{
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_shader_frontend *fe;
    HRESULT hr;
    unsigned int backend_version;

    TRACE("shader %p, byte_code %p, output_signature %p, float_const_count %u.\n",
            shader, byte_code, output_signature, float_const_count);

    fe = shader_select_frontend(*byte_code);
    if (!fe)
    {
        FIXME("Unable to find frontend for shader.\n");
        return WINED3DERR_INVALIDCALL;
    }
    shader->frontend = fe;
    shader->frontend_data = fe->shader_init(byte_code, output_signature);
    if (!shader->frontend_data)
    {
        FIXME("Failed to initialize frontend.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* First pass: trace shader. */
    if (TRACE_ON(d3d_shader))
        shader_trace_init(fe, shader->frontend_data, byte_code);

    /* Initialize immediate constant lists. */
    list_init(&shader->constantsF);
    list_init(&shader->constantsB);
    list_init(&shader->constantsI);

    /* Second pass: figure out which registers are used, what the semantics are, etc. */
    hr = shader_get_registers_used(shader, fe,
            reg_maps, shader->input_signature, shader->output_signature,
            byte_code, float_const_count);
    if (FAILED(hr)) return hr;

    if (reg_maps->shader_version.type != type)
    {
        WARN("Wrong shader type %d.\n", reg_maps->shader_version.type);
        return WINED3DERR_INVALIDCALL;
    }
    if (reg_maps->shader_version.major > max_version)
    {
        WARN("Shader version %d not supported by this D3D API version.\n", reg_maps->shader_version.major);
        return WINED3DERR_INVALIDCALL;
    }
    switch (type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            backend_version = shader->device->vshader_version;
            break;
        case WINED3D_SHADER_TYPE_PIXEL:
            backend_version = shader->device->pshader_version;
            break;
        default:
            FIXME("No backend version-checking for this shader type\n");
            backend_version = 0;
    }
    if (reg_maps->shader_version.major > backend_version)
    {
        WARN("Shader version %d.%d not supported by your GPU with the current shader backend.\n",
                reg_maps->shader_version.major, reg_maps->shader_version.minor);
        return WINED3DERR_INVALIDCALL;
    }

    shader->function = HeapAlloc(GetProcessHeap(), 0, shader->functionLength);
    if (!shader->function)
        return E_OUTOFMEMORY;
    memcpy(shader->function, byte_code, shader->functionLength);

    return WINED3D_OK;
}

ULONG CDECL wined3d_shader_incref(struct wined3d_shader *shader)
{
    ULONG refcount = InterlockedIncrement(&shader->ref);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    return refcount;
}

/* Do not call while under the GL lock. */
ULONG CDECL wined3d_shader_decref(struct wined3d_shader *shader)
{
    ULONG refcount = InterlockedDecrement(&shader->ref);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        shader_cleanup(shader);
        shader->parent_ops->wined3d_object_destroyed(shader->parent);
        HeapFree(GetProcessHeap(), 0, shader);
    }

    return refcount;
}

void * CDECL wined3d_shader_get_parent(const struct wined3d_shader *shader)
{
    TRACE("shader %p.\n", shader);

    return shader->parent;
}

HRESULT CDECL wined3d_shader_get_byte_code(const struct wined3d_shader *shader,
        void *byte_code, UINT *byte_code_size)
{
    TRACE("shader %p, byte_code %p, byte_code_size %p.\n", shader, byte_code, byte_code_size);

    if (!byte_code)
    {
        *byte_code_size = shader->functionLength;
        return WINED3D_OK;
    }

    if (*byte_code_size < shader->functionLength)
    {
        /* MSDN claims (for d3d8 at least) that if *byte_code_size is smaller
         * than the required size we should write the required size and
         * return D3DERR_MOREDATA. That's not actually true. */
        return WINED3DERR_INVALIDCALL;
    }

    memcpy(byte_code, shader->function, shader->functionLength);

    return WINED3D_OK;
}

/* Set local constants for d3d8 shaders. */
HRESULT CDECL wined3d_shader_set_local_constants_float(struct wined3d_shader *shader,
        UINT start_idx, const float *src_data, UINT count)
{
    UINT end_idx = start_idx + count;
    UINT i;

    TRACE("shader %p, start_idx %u, src_data %p, count %u.\n", shader, start_idx, src_data, count);

    if (end_idx > shader->limits.constant_float)
    {
        WARN("end_idx %u > float constants limit %u.\n",
                end_idx, shader->limits.constant_float);
        end_idx = shader->limits.constant_float;
    }

    for (i = start_idx; i < end_idx; ++i)
    {
        struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
        if (!lconst)
            return E_OUTOFMEMORY;

        lconst->idx = i;
        memcpy(lconst->value, src_data + (i - start_idx) * 4 /* 4 components */, 4 * sizeof(float));
        list_add_head(&shader->constantsF, &lconst->entry);
    }

    return WINED3D_OK;
}

void find_vs_compile_args(const struct wined3d_state *state,
        const struct wined3d_shader *shader, struct vs_compile_args *args)
{
    args->fog_src = state->render_states[WINED3D_RS_FOGTABLEMODE]
            == WINED3D_FOG_NONE ? VS_FOG_COORD : VS_FOG_Z;
    args->clip_enabled = state->render_states[WINED3D_RS_CLIPPING]
            && state->render_states[WINED3D_RS_CLIPPLANEENABLE];
    args->swizzle_map = shader->device->strided_streams.swizzle_map;
}

static BOOL match_usage(BYTE usage1, BYTE usage_idx1, BYTE usage2, BYTE usage_idx2)
{
    if (usage_idx1 != usage_idx2) return FALSE;
    if (usage1 == usage2) return TRUE;
    if (usage1 == WINED3DDECLUSAGE_POSITION && usage2 == WINED3DDECLUSAGE_POSITIONT) return TRUE;
    if (usage2 == WINED3DDECLUSAGE_POSITION && usage1 == WINED3DDECLUSAGE_POSITIONT) return TRUE;

    return FALSE;
}

BOOL vshader_get_input(const struct wined3d_shader *shader,
        BYTE usage_req, BYTE usage_idx_req, unsigned int *regnum)
{
    WORD map = shader->reg_maps.input_registers;
    unsigned int i;

    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (match_usage(shader->u.vs.attributes[i].usage,
                shader->u.vs.attributes[i].usage_idx, usage_req, usage_idx_req))
        {
            *regnum = i;
            return TRUE;
        }
    }
    return FALSE;
}

static void vertexshader_set_limits(struct wined3d_shader *shader)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(shader->reg_maps.shader_version.major,
            shader->reg_maps.shader_version.minor);
    struct wined3d_device *device = shader->device;

    shader->limits.texcoord = 0;
    shader->limits.attributes = 16;
    shader->limits.packed_input = 0;

    switch (shader_version)
    {
        case WINED3D_SHADER_VERSION(1, 0):
        case WINED3D_SHADER_VERSION(1, 1):
            shader->limits.temporary = 12;
            shader->limits.constant_bool = 0;
            shader->limits.constant_int = 0;
            shader->limits.address = 1;
            shader->limits.packed_output = 12;
            shader->limits.sampler = 0;
            shader->limits.label = 0;
            /* TODO: vs_1_1 has a minimum of 96 constants. What happens when
             * a vs_1_1 shader is used on a vs_3_0 capable card that has 256
             * constants? */
            shader->limits.constant_float = min(256, device->d3d_vshader_constantF);
            break;

        case WINED3D_SHADER_VERSION(2, 0):
        case WINED3D_SHADER_VERSION(2, 1):
            shader->limits.temporary = 12;
            shader->limits.constant_bool = 16;
            shader->limits.constant_int = 16;
            shader->limits.address = 1;
            shader->limits.packed_output = 12;
            shader->limits.sampler = 0;
            shader->limits.label = 16;
            shader->limits.constant_float = min(256, device->d3d_vshader_constantF);
            break;

        case WINED3D_SHADER_VERSION(4, 0):
            FIXME("Using 3.0 limits for 4.0 shader.\n");
            /* Fall through. */

        case WINED3D_SHADER_VERSION(3, 0):
            shader->limits.temporary = 32;
            shader->limits.constant_bool = 32;
            shader->limits.constant_int = 32;
            shader->limits.address = 1;
            shader->limits.packed_output = 12;
            shader->limits.sampler = 4;
            shader->limits.label = 16; /* FIXME: 2048 */
            /* DX10 cards on Windows advertise a d3d9 constant limit of 256
             * even though they are capable of supporting much more (GL
             * drivers advertise 1024). d3d9.dll and d3d8.dll clamp the
             * wined3d-advertised maximum. Clamp the constant limit for <= 3.0
             * shaders to 256. */
            shader->limits.constant_float = min(256, device->d3d_vshader_constantF);
            break;

        default:
            shader->limits.temporary = 12;
            shader->limits.constant_bool = 16;
            shader->limits.constant_int = 16;
            shader->limits.address = 1;
            shader->limits.packed_output = 12;
            shader->limits.sampler = 0;
            shader->limits.label = 16;
            shader->limits.constant_float = min(256, device->d3d_vshader_constantF);
            FIXME("Unrecognized vertex shader version \"%u.%u\".\n",
                    shader->reg_maps.shader_version.major,
                    shader->reg_maps.shader_version.minor);
    }
}

static HRESULT vertexshader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        void *parent, const struct wined3d_parent_ops *parent_ops, unsigned int max_version)
{
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    unsigned int i;
    HRESULT hr;
    WORD map;

    if (!byte_code) return WINED3DERR_INVALIDCALL;

    shader_init(shader, device, parent, parent_ops);
    hr = shader_set_function(shader, byte_code, output_signature, device->d3d_vshader_constantF,
            WINED3D_SHADER_TYPE_VERTEX, max_version);
    if (FAILED(hr))
    {
        WARN("Failed to set function, hr %#x.\n", hr);
        shader_cleanup(shader);
        return hr;
    }

    map = reg_maps->input_registers;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1) || !shader->input_signature[i].semantic_name)
            continue;

        shader->u.vs.attributes[i].usage =
                shader_usage_from_semantic_name(shader->input_signature[i].semantic_name);
        shader->u.vs.attributes[i].usage_idx = shader->input_signature[i].semantic_idx;
    }

    if (output_signature)
    {
        for (i = 0; i < output_signature->element_count; ++i)
        {
            struct wined3d_shader_signature_element *e = &output_signature->elements[i];
            reg_maps->output_registers |= 1 << e->register_idx;
            shader->output_signature[e->register_idx] = *e;
        }
    }

    vertexshader_set_limits(shader);

    shader->load_local_constsF = reg_maps->usesrelconstF
            && !list_empty(&shader->constantsF);

    return WINED3D_OK;
}

static HRESULT geometryshader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        void *parent, const struct wined3d_parent_ops *parent_ops, unsigned int max_version)
{
    HRESULT hr;

    shader_init(shader, device, parent, parent_ops);
    hr = shader_set_function(shader, byte_code, output_signature, 0,
            WINED3D_SHADER_TYPE_GEOMETRY, max_version);
    if (FAILED(hr))
    {
        WARN("Failed to set function, hr %#x.\n", hr);
        shader_cleanup(shader);
        return hr;
    }

    shader->load_local_constsF = FALSE;

    return WINED3D_OK;
}

void find_ps_compile_args(const struct wined3d_state *state,
        const struct wined3d_shader *shader, struct ps_compile_args *args)
{
    struct wined3d_device *device = shader->device;
    const struct wined3d_texture *texture;
    UINT i;

    memset(args, 0, sizeof(*args)); /* FIXME: Make sure all bits are set. */
    if (state->render_states[WINED3D_RS_SRGBWRITEENABLE])
    {
        const struct wined3d_surface *rt = state->fb->render_targets[0];
        if (rt->resource.format->flags & WINED3DFMT_FLAG_SRGB_WRITE) args->srgb_correction = 1;
    }

    if (shader->reg_maps.shader_version.major == 1
            && shader->reg_maps.shader_version.minor <= 3)
    {
        for (i = 0; i < 4; ++i)
        {
            DWORD flags = state->texture_states[i][WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS];

            if (flags & WINED3D_TTFF_PROJECTED)
            {
                enum wined3d_sampler_texture_type sampler_type = shader->reg_maps.sampler_type[i];
                DWORD tex_transform = flags & ~WINED3D_TTFF_PROJECTED;
                DWORD max_valid = WINED3D_TTFF_COUNT4;

                if (!state->vertex_shader)
                {
                    unsigned int j;
                    unsigned int index = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];
                    for (j = 0; j < state->vertex_declaration->element_count; ++j)
                    {
                        struct wined3d_vertex_declaration_element *element =
                                &state->vertex_declaration->elements[j];

                        if (element->usage == WINED3DDECLUSAGE_TEXCOORD
                                && element->usage_idx == index)
                        {
                            max_valid = element->format->component_count;
                            break;
                        }
                    }
                }

                if (!tex_transform || tex_transform > max_valid)
                {
                    WARN("Fixing up projected texture transform flags from %#x to %#x.\n",
                            tex_transform, max_valid);
                    tex_transform = max_valid;
                }

                if ((sampler_type == WINED3DSTT_1D && tex_transform > WINED3D_TTFF_COUNT1)
                        || (sampler_type == WINED3DSTT_2D && tex_transform > WINED3D_TTFF_COUNT2)
                        || (sampler_type == WINED3DSTT_VOLUME && tex_transform > WINED3D_TTFF_COUNT3))
                    tex_transform |= WINED3D_PSARGS_PROJECTED;
                else
                    WARN("Application requested projected texture with unsuitable texture coordinates.\n");

                args->tex_transform |= tex_transform << i * WINED3D_PSARGS_TEXTRANSFORM_SHIFT;
            }
        }
    }

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
    {
        if (!shader->reg_maps.sampler_type[i])
            continue;

        texture = state->textures[i];
        if (!texture)
        {
            args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
            continue;
        }
        args->color_fixup[i] = texture->resource.format->color_fixup;

        if (texture->resource.format->flags & WINED3DFMT_FLAG_SHADOW)
            args->shadow |= 1 << i;

        /* Flag samplers that need NP2 texcoord fixup. */
        if (!(texture->flags & WINED3D_TEXTURE_POW2_MAT_IDENT))
            args->np2_fixup |= (1 << i);
    }
    if (shader->reg_maps.shader_version.major >= 3)
    {
        if (device->strided_streams.position_transformed)
        {
            args->vp_mode = pretransformed;
        }
        else if (use_vs(state))
        {
            args->vp_mode = vertexshader;
        }
        else
        {
            args->vp_mode = fixedfunction;
        }
        args->fog = FOG_OFF;
    }
    else
    {
        args->vp_mode = vertexshader;
        if (state->render_states[WINED3D_RS_FOGENABLE])
        {
            switch (state->render_states[WINED3D_RS_FOGTABLEMODE])
            {
                case WINED3D_FOG_NONE:
                    if (device->strided_streams.position_transformed || use_vs(state))
                    {
                        args->fog = FOG_LINEAR;
                        break;
                    }

                    switch (state->render_states[WINED3D_RS_FOGVERTEXMODE])
                    {
                        case WINED3D_FOG_NONE: /* Fall through. */
                        case WINED3D_FOG_LINEAR: args->fog = FOG_LINEAR; break;
                        case WINED3D_FOG_EXP:    args->fog = FOG_EXP;    break;
                        case WINED3D_FOG_EXP2:   args->fog = FOG_EXP2;   break;
                    }
                    break;

                case WINED3D_FOG_LINEAR: args->fog = FOG_LINEAR; break;
                case WINED3D_FOG_EXP:    args->fog = FOG_EXP;    break;
                case WINED3D_FOG_EXP2:   args->fog = FOG_EXP2;   break;
            }
        }
        else
        {
            args->fog = FOG_OFF;
        }
    }
}

static void pixelshader_set_limits(struct wined3d_shader *shader)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(shader->reg_maps.shader_version.major,
            shader->reg_maps.shader_version.minor);

    shader->limits.attributes = 0;
    shader->limits.address = 0;
    shader->limits.packed_output = 0;

    switch (shader_version)
    {
        case WINED3D_SHADER_VERSION(1, 0):
        case WINED3D_SHADER_VERSION(1, 1):
        case WINED3D_SHADER_VERSION(1, 2):
        case WINED3D_SHADER_VERSION(1, 3):
            shader->limits.temporary = 2;
            shader->limits.constant_float = 8;
            shader->limits.constant_int = 0;
            shader->limits.constant_bool = 0;
            shader->limits.texcoord = 4;
            shader->limits.sampler = 4;
            shader->limits.packed_input = 0;
            shader->limits.label = 0;
            break;

        case WINED3D_SHADER_VERSION(1, 4):
            shader->limits.temporary = 6;
            shader->limits.constant_float = 8;
            shader->limits.constant_int = 0;
            shader->limits.constant_bool = 0;
            shader->limits.texcoord = 6;
            shader->limits.sampler = 6;
            shader->limits.packed_input = 0;
            shader->limits.label = 0;
            break;

        /* FIXME: Temporaries must match D3DPSHADERCAPS2_0.NumTemps. */
        case WINED3D_SHADER_VERSION(2, 0):
            shader->limits.temporary = 32;
            shader->limits.constant_float = 32;
            shader->limits.constant_int = 16;
            shader->limits.constant_bool = 16;
            shader->limits.texcoord = 8;
            shader->limits.sampler = 16;
            shader->limits.packed_input = 0;
            break;

        case WINED3D_SHADER_VERSION(2, 1):
            shader->limits.temporary = 32;
            shader->limits.constant_float = 32;
            shader->limits.constant_int = 16;
            shader->limits.constant_bool = 16;
            shader->limits.texcoord = 8;
            shader->limits.sampler = 16;
            shader->limits.packed_input = 0;
            shader->limits.label = 16;
            break;

        case WINED3D_SHADER_VERSION(4, 0):
            FIXME("Using 3.0 limits for 4.0 shader.\n");
            /* Fall through. */

        case WINED3D_SHADER_VERSION(3, 0):
            shader->limits.temporary = 32;
            shader->limits.constant_float = 224;
            shader->limits.constant_int = 16;
            shader->limits.constant_bool = 16;
            shader->limits.texcoord = 0;
            shader->limits.sampler = 16;
            shader->limits.packed_input = 12;
            shader->limits.label = 16; /* FIXME: 2048 */
            break;

        default:
            shader->limits.temporary = 32;
            shader->limits.constant_float = 32;
            shader->limits.constant_int = 16;
            shader->limits.constant_bool = 16;
            shader->limits.texcoord = 8;
            shader->limits.sampler = 16;
            shader->limits.packed_input = 0;
            shader->limits.label = 0;
            FIXME("Unrecognized pixel shader version %u.%u\n",
                    shader->reg_maps.shader_version.major,
                    shader->reg_maps.shader_version.minor);
    }
}

static HRESULT pixelshader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        void *parent, const struct wined3d_parent_ops *parent_ops, unsigned int max_version)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i, highest_reg_used = 0, num_regs_used = 0;
    HRESULT hr;

    if (!byte_code) return WINED3DERR_INVALIDCALL;

    shader_init(shader, device, parent, parent_ops);
    hr = shader_set_function(shader, byte_code, output_signature, device->d3d_pshader_constantF,
            WINED3D_SHADER_TYPE_PIXEL, max_version);
    if (FAILED(hr))
    {
        WARN("Failed to set function, hr %#x.\n", hr);
        shader_cleanup(shader);
        return hr;
    }

    pixelshader_set_limits(shader);

    for (i = 0; i < MAX_REG_INPUT; ++i)
    {
        if (shader->u.ps.input_reg_used[i])
        {
            ++num_regs_used;
            highest_reg_used = i;
        }
    }

    /* Don't do any register mapping magic if it is not needed, or if we can't
     * achieve anything anyway */
    if (highest_reg_used < (gl_info->limits.glsl_varyings / 4)
            || num_regs_used > (gl_info->limits.glsl_varyings / 4))
    {
        if (num_regs_used > (gl_info->limits.glsl_varyings / 4))
        {
            /* This happens with relative addressing. The input mapper function
             * warns about this if the higher registers are declared too, so
             * don't write a FIXME here */
            WARN("More varying registers used than supported\n");
        }

        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            shader->u.ps.input_reg_map[i] = i;
        }

        shader->u.ps.declared_in_count = highest_reg_used + 1;
    }
    else
    {
        shader->u.ps.declared_in_count = 0;
        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            if (shader->u.ps.input_reg_used[i])
                shader->u.ps.input_reg_map[i] = shader->u.ps.declared_in_count++;
            else shader->u.ps.input_reg_map[i] = ~0U;
        }
    }

    shader->load_local_constsF = FALSE;

    return WINED3D_OK;
}

void pixelshader_update_samplers(struct wined3d_shader_reg_maps *reg_maps, struct wined3d_texture * const *textures)
{
    enum wined3d_sampler_texture_type *sampler_type = reg_maps->sampler_type;
    unsigned int i;

    if (reg_maps->shader_version.major != 1) return;

    for (i = 0; i < max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS); ++i)
    {
        /* We don't sample from this sampler. */
        if (!sampler_type[i]) continue;

        if (!textures[i])
        {
            WARN("No texture bound to sampler %u, using 2D.\n", i);
            sampler_type[i] = WINED3DSTT_2D;
            continue;
        }

        switch (textures[i]->target)
        {
            case GL_TEXTURE_RECTANGLE_ARB:
            case GL_TEXTURE_2D:
                /* We have to select between texture rectangles and 2D
                 * textures later because 2.0 and 3.0 shaders only have
                 * WINED3DSTT_2D as well. */
                sampler_type[i] = WINED3DSTT_2D;
                break;

            case GL_TEXTURE_3D:
                sampler_type[i] = WINED3DSTT_VOLUME;
                break;

            case GL_TEXTURE_CUBE_MAP_ARB:
                sampler_type[i] = WINED3DSTT_CUBE;
                break;

            default:
                FIXME("Unrecognized texture type %#x, using 2D.\n", textures[i]->target);
                sampler_type[i] = WINED3DSTT_2D;
        }
    }
}

HRESULT CDECL wined3d_shader_create_gs(struct wined3d_device *device, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader, unsigned int max_version)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, byte_code %p, output_signature %p, parent %p, parent_ops %p, shader %p.\n",
            device, byte_code, output_signature, parent, parent_ops, shader);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate shader memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = geometryshader_init(object, device, byte_code, output_signature, parent, parent_ops, max_version);
    if (FAILED(hr))
    {
        WARN("Failed to initialize geometry shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created geometry shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_create_ps(struct wined3d_device *device, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader, unsigned int max_version)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, byte_code %p, output_signature %p, parent %p, parent_ops %p, shader %p.\n",
            device, byte_code, output_signature, parent, parent_ops, shader);

    if (device->ps_selected_mode == SHADER_NONE)
        return WINED3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate shader memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = pixelshader_init(object, device, byte_code, output_signature, parent, parent_ops, max_version);
    if (FAILED(hr))
    {
        WARN("Failed to initialize pixel shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created pixel shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_create_vs(struct wined3d_device *device, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader, unsigned int max_version)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, byte_code %p, output_signature %p, parent %p, parent_ops %p, shader %p.\n",
            device, byte_code, output_signature, parent, parent_ops, shader);

    if (device->vs_selected_mode == SHADER_NONE)
        return WINED3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate shader memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = vertexshader_init(object, device, byte_code, output_signature, parent, parent_ops, max_version);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}
