/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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
#include <string.h>
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

static const char *shader_opcode_names[] =
{
    /* WINED3DSIH_ABS           */ "abs",
    /* WINED3DSIH_ADD           */ "add",
    /* WINED3DSIH_BEM           */ "bem",
    /* WINED3DSIH_BREAK         */ "break",
    /* WINED3DSIH_BREAKC        */ "breakc",
    /* WINED3DSIH_BREAKP        */ "breakp",
    /* WINED3DSIH_CALL          */ "call",
    /* WINED3DSIH_CALLNZ        */ "callnz",
    /* WINED3DSIH_CMP           */ "cmp",
    /* WINED3DSIH_CND           */ "cnd",
    /* WINED3DSIH_CRS           */ "crs",
    /* WINED3DSIH_DCL           */ "dcl",
    /* WINED3DSIH_DEF           */ "def",
    /* WINED3DSIH_DEFB          */ "defb",
    /* WINED3DSIH_DEFI          */ "defi",
    /* WINED3DSIH_DP2ADD        */ "dp2add",
    /* WINED3DSIH_DP3           */ "dp3",
    /* WINED3DSIH_DP4           */ "dp4",
    /* WINED3DSIH_DST           */ "dst",
    /* WINED3DSIH_DSX           */ "dsx",
    /* WINED3DSIH_DSY           */ "dsy",
    /* WINED3DSIH_ELSE          */ "else",
    /* WINED3DSIH_ENDIF         */ "endif",
    /* WINED3DSIH_ENDLOOP       */ "endloop",
    /* WINED3DSIH_ENDREP        */ "endrep",
    /* WINED3DSIH_EXP           */ "exp",
    /* WINED3DSIH_EXPP          */ "expp",
    /* WINED3DSIH_FRC           */ "frc",
    /* WINED3DSIH_IF            */ "if",
    /* WINED3DSIH_IFC           */ "ifc",
    /* WINED3DSIH_LABEL         */ "label",
    /* WINED3DSIH_LIT           */ "lit",
    /* WINED3DSIH_LOG           */ "log",
    /* WINED3DSIH_LOGP          */ "logp",
    /* WINED3DSIH_LOOP          */ "loop",
    /* WINED3DSIH_LRP           */ "lrp",
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
    /* WINED3DSIH_MUL           */ "mul",
    /* WINED3DSIH_NOP           */ "nop",
    /* WINED3DSIH_NRM           */ "nrm",
    /* WINED3DSIH_PHASE         */ "phase",
    /* WINED3DSIH_POW           */ "pow",
    /* WINED3DSIH_RCP           */ "rcp",
    /* WINED3DSIH_REP           */ "rep",
    /* WINED3DSIH_RET           */ "ret",
    /* WINED3DSIH_RSQ           */ "rsq",
    /* WINED3DSIH_SETP          */ "setp",
    /* WINED3DSIH_SGE           */ "sge",
    /* WINED3DSIH_SGN           */ "sgn",
    /* WINED3DSIH_SINCOS        */ "sincos",
    /* WINED3DSIH_SLT           */ "slt",
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
};

const struct wined3d_shader_frontend *shader_select_frontend(DWORD version_token)
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
    char* base = buffer->buffer + buffer->bsize;
    int rc;

    rc = vsnprintf(base, SHADER_PGMSIZE - 1 - buffer->bsize, format, args);

    if (rc < 0 /* C89 */ || (unsigned int)rc > SHADER_PGMSIZE - 1 - buffer->bsize /* C99 */)
    {
        ERR("The buffer allocated for the shader program string "
            "is too small at %d bytes.\n", SHADER_PGMSIZE);
        buffer->bsize = SHADER_PGMSIZE - 1;
        return -1;
    }

    if (buffer->newline) {
        TRACE("GL HW (%u, %u) : %s", buffer->lineNo + 1, buffer->bsize, base);
        buffer->newline = FALSE;
    } else {
        TRACE("%s", base);
    }

    buffer->bsize += rc;
    if (buffer->buffer[buffer->bsize-1] == '\n') {
        buffer->lineNo++;
        buffer->newline = TRUE;
    }
    return 0;
}

int shader_addline(struct wined3d_shader_buffer *buffer, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = shader_vaddline(buffer, format, args);
    va_end(args);

    return ret;
}

void shader_init(struct IWineD3DBaseShaderClass *shader, IWineD3DDeviceImpl *device,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops)
{
    shader->ref = 1;
    shader->device = (IWineD3DDevice *)device;
    shader->parent = parent;
    shader->parent_ops = parent_ops;
    list_init(&shader->linked_programs);
    list_add_head(&device->shaders, &shader->shader_list_entry);
}

/* Convert floating point offset relative
 * to a register file to an absolute offset for float constants */
static unsigned int shader_get_float_offset(WINED3DSHADER_PARAM_REGISTER_TYPE register_type, UINT register_idx)
{
    switch (register_type)
    {
        case WINED3DSPR_CONST: return register_idx;
        case WINED3DSPR_CONST2: return 2048 + register_idx;
        case WINED3DSPR_CONST3: return 4096 + register_idx;
        case WINED3DSPR_CONST4: return 6144 + register_idx;
        default:
            FIXME("Unsupported register type: %d\n", register_type);
            return register_idx;
    }
}

static void shader_delete_constant_list(struct list* clist) {

    struct list *ptr;
    struct local_constant* constant;

    ptr = list_head(clist);
    while (ptr) {
        constant = LIST_ENTRY(ptr, struct local_constant, entry);
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

static void shader_record_register_usage(IWineD3DBaseShaderImpl *This, struct shader_reg_maps *reg_maps,
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
                        ((IWineD3DPixelShaderImpl *)This)->input_reg_used[i] = TRUE;
                    }
                }
                else
                {
                    ((IWineD3DPixelShaderImpl *)This)->input_reg_used[reg->idx] = TRUE;
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
                if (reg->idx == 0) reg_maps->vpos = 1;
                else if (reg->idx == 1) reg_maps->usesfacing = 1;
            }
            break;

        case WINED3DSPR_CONST:
            if (reg->rel_addr)
            {
                if (shader_type != WINED3D_SHADER_TYPE_PIXEL)
                {
                    if (reg->idx < ((IWineD3DVertexShaderImpl *)This)->min_rel_offset)
                    {
                        ((IWineD3DVertexShaderImpl *)This)->min_rel_offset = reg->idx;
                    }
                    if (reg->idx > ((IWineD3DVertexShaderImpl *)This)->max_rel_offset)
                    {
                        ((IWineD3DVertexShaderImpl *)This)->max_rel_offset = reg->idx;
                    }
                }
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
            reg_maps->highest_render_target = max(reg_maps->highest_render_target, reg->idx);
            break;

        default:
            TRACE("Not recording register of type %#x and idx %u\n", reg->type, reg->idx);
            break;
    }
}

static unsigned int get_instr_extra_regcount(enum WINED3D_SHADER_INSTRUCTION_HANDLER instr, unsigned int param)
{
    switch(instr)
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

static const char *semantic_names[] =
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
        FIXME("Unrecognized usage %#x\n", usage);
        return "UNRECOGNIZED";
    }

    return semantic_names[usage];
}

WINED3DDECLUSAGE shader_usage_from_semantic_name(const char *name)
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

/* Note that this does not count the loop register
 * as an address register. */

HRESULT shader_get_registers_used(IWineD3DBaseShader *iface, const struct wined3d_shader_frontend *fe,
        struct shader_reg_maps *reg_maps, struct wined3d_shader_signature_element *input_signature,
        struct wined3d_shader_signature_element *output_signature, const DWORD *byte_code, DWORD constf_size)
{
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    void *fe_data = This->baseShader.frontend_data;
    struct wined3d_shader_version shader_version;
    unsigned int cur_loop_depth = 0, max_loop_depth = 0;
    const DWORD* pToken = byte_code;

    /* There are some minor differences between pixel and vertex shaders */

    memset(reg_maps, 0, sizeof(*reg_maps));

    /* get_registers_used is called on every compile on some 1.x shaders, which can result
     * in stacking up a collection of local constants. Delete the old constants if existing
     */
    shader_delete_constant_list(&This->baseShader.constantsF);
    shader_delete_constant_list(&This->baseShader.constantsB);
    shader_delete_constant_list(&This->baseShader.constantsI);

    fe->shader_read_header(fe_data, &pToken, &shader_version);
    reg_maps->shader_version = shader_version;

    reg_maps->constf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 sizeof(*reg_maps->constf) * ((constf_size + 31) / 32));
    if(!reg_maps->constf) {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    while (!fe->shader_is_end(fe_data, &pToken))
    {
        struct wined3d_shader_instruction ins;
        const char *comment;
        UINT param_size;

        /* Skip comments */
        fe->shader_read_comment(&pToken, &comment);
        if (comment) continue;

        /* Fetch opcode */
        fe->shader_read_opcode(fe_data, &pToken, &ins, &param_size);

        /* Unhandled opcode, and its parameters */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            pToken += param_size;
            continue;
        }

        /* Handle declarations */
        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            struct wined3d_shader_semantic semantic;

            fe->shader_read_semantic(&pToken, &semantic);

            switch (semantic.reg.reg.type)
            {
                /* Mark input registers used. */
                case WINED3DSPR_INPUT:
                    reg_maps->input_registers |= 1 << semantic.reg.reg.idx;
                    shader_signature_from_semantic(&input_signature[semantic.reg.reg.idx], &semantic);
                    break;

                /* Vshader: mark 3.0 output registers used, save token */
                case WINED3DSPR_OUTPUT:
                    reg_maps->output_registers |= 1 << semantic.reg.reg.idx;
                    shader_signature_from_semantic(&output_signature[semantic.reg.reg.idx], &semantic);
                    if (semantic.usage == WINED3DDECLUSAGE_FOG) reg_maps->fog = 1;
                    break;

                /* Save sampler usage token */
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
            struct wined3d_shader_dst_param dst;
            struct wined3d_shader_src_param rel_addr;

            local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
            if (!lconst) return E_OUTOFMEMORY;

            fe->shader_read_dst_param(fe_data, &pToken, &dst, &rel_addr);
            lconst->idx = dst.reg.idx;

            memcpy(lconst->value, pToken, 4 * sizeof(DWORD));
            pToken += 4;

            /* In pixel shader 1.X shaders, the constants are clamped between [-1;1] */
            if (shader_version.major == 1 && shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
            {
                float *value = (float *) lconst->value;
                if (value[0] < -1.0f) value[0] = -1.0f;
                else if (value[0] > 1.0f) value[0] = 1.0f;
                if (value[1] < -1.0f) value[1] = -1.0f;
                else if (value[1] > 1.0f) value[1] = 1.0f;
                if (value[2] < -1.0f) value[2] = -1.0f;
                else if (value[2] > 1.0f) value[2] = 1.0f;
                if (value[3] < -1.0f) value[3] = -1.0f;
                else if (value[3] > 1.0f) value[3] = 1.0f;
            }

            list_add_head(&This->baseShader.constantsF, &lconst->entry);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            struct wined3d_shader_dst_param dst;
            struct wined3d_shader_src_param rel_addr;

            local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
            if (!lconst) return E_OUTOFMEMORY;

            fe->shader_read_dst_param(fe_data, &pToken, &dst, &rel_addr);
            lconst->idx = dst.reg.idx;

            memcpy(lconst->value, pToken, 4 * sizeof(DWORD));
            pToken += 4;

            list_add_head(&This->baseShader.constantsI, &lconst->entry);
            reg_maps->local_int_consts |= (1 << dst.reg.idx);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            struct wined3d_shader_dst_param dst;
            struct wined3d_shader_src_param rel_addr;

            local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
            if (!lconst) return E_OUTOFMEMORY;

            fe->shader_read_dst_param(fe_data, &pToken, &dst, &rel_addr);
            lconst->idx = dst.reg.idx;

            memcpy(lconst->value, pToken, sizeof(DWORD));
            ++pToken;

            list_add_head(&This->baseShader.constantsB, &lconst->entry);
            reg_maps->local_bool_consts |= (1 << dst.reg.idx);
        }
        /* If there's a loop in the shader */
        else if (ins.handler_idx == WINED3DSIH_LOOP
                || ins.handler_idx == WINED3DSIH_REP)
        {
            struct wined3d_shader_src_param src, rel_addr;

            fe->shader_read_src_param(fe_data, &pToken, &src, &rel_addr);

            /* Rep and Loop always use an integer constant for the control parameters */
            if (ins.handler_idx == WINED3DSIH_REP)
            {
                reg_maps->integer_constants |= 1 << src.reg.idx;
            }
            else
            {
                fe->shader_read_src_param(fe_data, &pToken, &src, &rel_addr);
                reg_maps->integer_constants |= 1 << src.reg.idx;
            }

            cur_loop_depth++;
            if(cur_loop_depth > max_loop_depth)
                max_loop_depth = cur_loop_depth;
        }
        else if (ins.handler_idx == WINED3DSIH_ENDLOOP
                || ins.handler_idx == WINED3DSIH_ENDREP)
        {
            cur_loop_depth--;
        }
        /* For subroutine prototypes */
        else if (ins.handler_idx == WINED3DSIH_LABEL)
        {
            struct wined3d_shader_src_param src, rel_addr;

            fe->shader_read_src_param(fe_data, &pToken, &src, &rel_addr);
            reg_maps->labels |= 1 << src.reg.idx;
        }
        /* Set texture, address, temporary registers */
        else
        {
            int i, limit;
            BOOL color0_mov = FALSE;

            /* This will loop over all the registers and try to
             * make a bitmask of the ones we're interested in.
             *
             * Relative addressing tokens are ignored, but that's
             * okay, since we'll catch any address registers when
             * they are initialized (required by spec) */

            if (ins.dst_count)
            {
                struct wined3d_shader_dst_param dst_param;
                struct wined3d_shader_src_param dst_rel_addr;

                fe->shader_read_dst_param(fe_data, &pToken, &dst_param, &dst_rel_addr);

                shader_record_register_usage(This, reg_maps, &dst_param.reg, shader_version.type);

                /* WINED3DSPR_TEXCRDOUT is the same as WINED3DSPR_OUTPUT. _OUTPUT can be > MAX_REG_TEXCRD and
                 * is used in >= 3.0 shaders. Filter 3.0 shaders to prevent overflows, and also filter pixel
                 * shaders because TECRDOUT isn't used in them, but future register types might cause issues */
                if (shader_version.type == WINED3D_SHADER_TYPE_VERTEX && shader_version.major < 3
                        && dst_param.reg.type == WINED3DSPR_TEXCRDOUT)
                {
                    reg_maps->texcoord_mask[dst_param.reg.idx] |= dst_param.write_mask;
                }

                if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
                {
                    IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *)This;

                    if(dst_param.reg.type == WINED3DSPR_COLOROUT && dst_param.reg.idx == 0)
                    {
                    /* Many 2.0 and 3.0 pixel shaders end with a MOV from a temp register to
                     * COLOROUT 0. If we know this in advance, the ARB shader backend can skip
                     * the mov and perform the sRGB write correction from the source register.
                     *
                     * However, if the mov is only partial, we can't do this, and if the write
                     * comes from an instruction other than MOV it is hard to do as well. If
                     * COLOROUT 0 is overwritten partially later, the marker is dropped again. */

                        ps->color0_mov = FALSE;
                        if (ins.handler_idx == WINED3DSIH_MOV)
                        {
                            /* Used later when the source register is read. */
                            color0_mov = TRUE;
                        }
                    }
                    /* Also drop the MOV marker if the source register is overwritten prior to the shader
                     * end
                     */
                    else if(dst_param.reg.type == WINED3DSPR_TEMP && dst_param.reg.idx == ps->color0_reg)
                    {
                        ps->color0_mov = FALSE;
                    }
                }

                /* Declare 1.X samplers implicitly, based on the destination reg. number */
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
                    /* Fake sampler usage, only set reserved bit and ttype */
                    DWORD sampler_code = dst_param.reg.idx;

                    TRACE("Setting fake 2D sampler for 1.x pixelshader\n");
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

            if (ins.handler_idx == WINED3DSIH_NRM)
            {
                reg_maps->usesnrm = 1;
            }
            else if (ins.handler_idx == WINED3DSIH_DSY)
            {
                reg_maps->usesdsy = 1;
            }
            else if (ins.handler_idx == WINED3DSIH_DSX)
            {
                reg_maps->usesdsx = 1;
            }
            else if(ins.handler_idx == WINED3DSIH_TEXLDD)
            {
                reg_maps->usestexldd = 1;
            }
            else if(ins.handler_idx == WINED3DSIH_TEXLDL)
            {
                reg_maps->usestexldl = 1;
            }
            else if(ins.handler_idx == WINED3DSIH_MOVA)
            {
                reg_maps->usesmova = 1;
            }
            else if(ins.handler_idx == WINED3DSIH_IFC)
            {
                reg_maps->usesifc = 1;
            }
            else if(ins.handler_idx == WINED3DSIH_CALL)
            {
                reg_maps->usescall = 1;
            }

            limit = ins.src_count + (ins.predicate ? 1 : 0);
            for (i = 0; i < limit; ++i)
            {
                struct wined3d_shader_src_param src_param, src_rel_addr;
                unsigned int count;

                fe->shader_read_src_param(fe_data, &pToken, &src_param, &src_rel_addr);
                count = get_instr_extra_regcount(ins.handler_idx, i);

                shader_record_register_usage(This, reg_maps, &src_param.reg, shader_version.type);
                while (count)
                {
                    ++src_param.reg.idx;
                    shader_record_register_usage(This, reg_maps, &src_param.reg, shader_version.type);
                    --count;
                }

                if(color0_mov)
                {
                    IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) This;
                    if(src_param.reg.type == WINED3DSPR_TEMP &&
                       src_param.swizzle == WINED3DSP_NOSWIZZLE)
                    {
                        ps->color0_mov = TRUE;
                        ps->color0_reg = src_param.reg.idx;
                    }
                }
            }
        }
    }
    reg_maps->loop_depth = max_loop_depth;

    This->baseShader.functionLength = ((const char *)pToken - (const char *)byte_code);

    return WINED3D_OK;
}

unsigned int shader_find_free_input_register(const struct shader_reg_maps *reg_maps, unsigned int max)
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
        /* Pixel shaders 3.0 don't have usage semantics */
        if (shader_version->major < 3 && shader_version->type == WINED3D_SHADER_TYPE_PIXEL)
            return;
        else
            TRACE("_");

        switch (semantic->usage)
        {
            case WINED3DDECLUSAGE_POSITION:
                TRACE("position%d", semantic->usage_idx);
                break;
            case WINED3DDECLUSAGE_BLENDINDICES:
                TRACE("blend");
                break;
            case WINED3DDECLUSAGE_BLENDWEIGHT:
                TRACE("weight");
                break;
            case WINED3DDECLUSAGE_NORMAL:
                TRACE("normal%d", semantic->usage_idx);
                break;
            case WINED3DDECLUSAGE_PSIZE:
                TRACE("psize");
                break;
            case WINED3DDECLUSAGE_COLOR:
                if (semantic->usage_idx == 0) TRACE("color");
                else TRACE("specular%d", (semantic->usage_idx - 1));
                break;
            case WINED3DDECLUSAGE_TEXCOORD:
                TRACE("texture%d", semantic->usage_idx);
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
                TRACE("positionT%d", semantic->usage_idx);
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
             * (WINED3DSPR_OUTPUT), which can include an address token */
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
            if (reg->idx > 1) FIXME("Unhandled misctype register %d\n", reg->idx);
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

        default:
            TRACE("unhandled_rtype(%#x)", reg->type);
            break;
    }

    if (reg->type == WINED3DSPR_IMMCONST)
    {
        TRACE("(");
        switch (reg->immconst_type)
        {
            case WINED3D_IMMCONST_FLOAT:
                TRACE("%.8e", *(const float *)reg->immconst_data);
                break;

            case WINED3D_IMMCONST_FLOAT4:
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
    else if (reg->type != WINED3DSPR_RASTOUT && reg->type != WINED3DSPR_MISCTYPE)
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

    if (write_mask != WINED3DSP_WRITEMASK_ALL)
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
    DWORD src_modifier = param->modifiers;
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
            default:
                                      TRACE("_unknown_modifier(%#x)", src_modifier);
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
 * NOTE: A description of how to parse tokens can be found on msdn */
void shader_generate_main(IWineD3DBaseShader *iface, struct wined3d_shader_buffer *buffer,
        const shader_reg_maps *reg_maps, const DWORD *pFunction, void *backend_ctx)
{
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device; /* To access shader backend callbacks */
    const struct wined3d_shader_frontend *fe = This->baseShader.frontend;
    void *fe_data = This->baseShader.frontend_data;
    struct wined3d_shader_src_param src_rel_addr[4];
    struct wined3d_shader_src_param src_param[4];
    struct wined3d_shader_version shader_version;
    struct wined3d_shader_src_param dst_rel_addr;
    struct wined3d_shader_dst_param dst_param;
    struct wined3d_shader_instruction ins;
    struct wined3d_shader_context ctx;
    const DWORD *pToken = pFunction;
    DWORD i;

    /* Initialize current parsing state */
    ctx.shader = iface;
    ctx.reg_maps = reg_maps;
    ctx.buffer = buffer;
    ctx.backend_data = backend_ctx;

    ins.ctx = &ctx;
    ins.dst = &dst_param;
    ins.src = src_param;
    This->baseShader.parse_state.current_row = 0;

    fe->shader_read_header(fe_data, &pToken, &shader_version);

    while (!fe->shader_is_end(fe_data, &pToken))
    {
        const char *comment;
        UINT param_size;

        /* Skip comment tokens */
        fe->shader_read_comment(&pToken, &comment);
        if (comment) continue;

        /* Read opcode */
        fe->shader_read_opcode(fe_data, &pToken, &ins, &param_size);

        /* Unknown opcode and its parameters */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            pToken += param_size;
            continue;
        }

        /* Nothing to do */
        if (ins.handler_idx == WINED3DSIH_DCL
                || ins.handler_idx == WINED3DSIH_NOP
                || ins.handler_idx == WINED3DSIH_DEF
                || ins.handler_idx == WINED3DSIH_DEFI
                || ins.handler_idx == WINED3DSIH_DEFB
                || ins.handler_idx == WINED3DSIH_PHASE)
        {
            pToken += param_size;
            continue;
        }

        /* Destination token */
        if (ins.dst_count) fe->shader_read_dst_param(fe_data, &pToken, &dst_param, &dst_rel_addr);

        /* Predication token */
        if (ins.predicate) ins.predicate = *pToken++;

        /* Other source tokens */
        for (i = 0; i < ins.src_count; ++i)
        {
            fe->shader_read_src_param(fe_data, &pToken, &src_param[i], &src_rel_addr[i]);
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
    if (mmask)
        FIXME("_unrecognized_modifier(%#x)", mmask);
}

void shader_trace_init(const struct wined3d_shader_frontend *fe, void *fe_data, const DWORD *pFunction)
{
    struct wined3d_shader_version shader_version;
    const DWORD* pToken = pFunction;
    const char *type_prefix;
    DWORD i;

    TRACE("Parsing %p\n", pFunction);

    fe->shader_read_header(fe_data, &pToken, &shader_version);

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

    while (!fe->shader_is_end(fe_data, &pToken))
    {
        struct wined3d_shader_instruction ins;
        const char *comment;
        UINT param_size;

        /* comment */
        fe->shader_read_comment(&pToken, &comment);
        if (comment)
        {
            TRACE("//%s\n", comment);
            continue;
        }

        fe->shader_read_opcode(fe_data, &pToken, &ins, &param_size);
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            pToken += param_size;
            continue;
        }

        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            struct wined3d_shader_semantic semantic;

            fe->shader_read_semantic(&pToken, &semantic);

            shader_dump_decl_usage(&semantic, &shader_version);
            shader_dump_ins_modifiers(&semantic.reg);
            TRACE(" ");
            shader_dump_dst_param(&semantic.reg, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            struct wined3d_shader_dst_param dst;
            struct wined3d_shader_src_param rel_addr;

            fe->shader_read_dst_param(fe_data, &pToken, &dst, &rel_addr);

            TRACE("def c%u = %f, %f, %f, %f", shader_get_float_offset(dst.reg.type, dst.reg.idx),
                    *(const float *)(pToken),
                    *(const float *)(pToken + 1),
                    *(const float *)(pToken + 2),
                    *(const float *)(pToken + 3));
            pToken += 4;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            struct wined3d_shader_dst_param dst;
            struct wined3d_shader_src_param rel_addr;

            fe->shader_read_dst_param(fe_data, &pToken, &dst, &rel_addr);

            TRACE("defi i%u = %d, %d, %d, %d", dst.reg.idx,
                    *(pToken),
                    *(pToken + 1),
                    *(pToken + 2),
                    *(pToken + 3));
            pToken += 4;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            struct wined3d_shader_dst_param dst;
            struct wined3d_shader_src_param rel_addr;

            fe->shader_read_dst_param(fe_data, &pToken, &dst, &rel_addr);

            TRACE("defb b%u = %s", dst.reg.idx, *pToken ? "true" : "false");
            ++pToken;
        }
        else
        {
            struct wined3d_shader_src_param dst_rel_addr, src_rel_addr;
            struct wined3d_shader_dst_param dst_param;
            struct wined3d_shader_src_param src_param;

            if (ins.dst_count)
            {
                fe->shader_read_dst_param(fe_data, &pToken, &dst_param, &dst_rel_addr);
            }

            /* Print out predication source token first - it follows
             * the destination token. */
            if (ins.predicate)
            {
                fe->shader_read_src_param(fe_data, &pToken, &src_param, &src_rel_addr);
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
                    case COMPARISON_GT: TRACE("_gt"); break;
                    case COMPARISON_EQ: TRACE("_eq"); break;
                    case COMPARISON_GE: TRACE("_ge"); break;
                    case COMPARISON_LT: TRACE("_lt"); break;
                    case COMPARISON_NE: TRACE("_ne"); break;
                    case COMPARISON_LE: TRACE("_le"); break;
                    default: TRACE("_(%u)", ins.flags);
                }
            }
            else if (ins.handler_idx == WINED3DSIH_TEX
                    && shader_version.major >= 2
                    && (ins.flags & WINED3DSI_TEXLD_PROJECT))
            {
                TRACE("p");
            }

            /* We already read the destination token, print it. */
            if (ins.dst_count)
            {
                shader_dump_ins_modifiers(&dst_param);
                TRACE(" ");
                shader_dump_dst_param(&dst_param, &shader_version);
            }

            /* Other source tokens */
            for (i = ins.dst_count; i < (ins.dst_count + ins.src_count); ++i)
            {
                fe->shader_read_src_param(fe_data, &pToken, &src_param, &src_rel_addr);
                TRACE(!i ? " " : ", ");
                shader_dump_src_param(&src_param, &shader_version);
            }
        }
        TRACE("\n");
    }
}

void shader_cleanup(IWineD3DBaseShader *iface)
{
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)iface;

    ((IWineD3DDeviceImpl *)This->baseShader.device)->shader_backend->shader_destroy(iface);
    HeapFree(GetProcessHeap(), 0, This->baseShader.reg_maps.constf);
    HeapFree(GetProcessHeap(), 0, This->baseShader.function);
    shader_delete_constant_list(&This->baseShader.constantsF);
    shader_delete_constant_list(&This->baseShader.constantsB);
    shader_delete_constant_list(&This->baseShader.constantsI);
    list_remove(&This->baseShader.shader_list_entry);

    if (This->baseShader.frontend && This->baseShader.frontend_data)
    {
        This->baseShader.frontend->shader_free(This->baseShader.frontend_data);
    }
}

static void shader_none_handle_instruction(const struct wined3d_shader_instruction *ins) {}
static void shader_none_select(const struct wined3d_context *context, BOOL usePS, BOOL useVS) {}
static void shader_none_select_depth_blt(IWineD3DDevice *iface, enum tex_types tex_type) {}
static void shader_none_deselect_depth_blt(IWineD3DDevice *iface) {}
static void shader_none_update_float_vertex_constants(IWineD3DDevice *iface, UINT start, UINT count) {}
static void shader_none_update_float_pixel_constants(IWineD3DDevice *iface, UINT start, UINT count) {}
static void shader_none_load_constants(const struct wined3d_context *context, char usePS, char useVS) {}
static void shader_none_load_np2fixup_constants(IWineD3DDevice *iface, char usePS, char useVS) {}
static void shader_none_destroy(IWineD3DBaseShader *iface) {}
static HRESULT shader_none_alloc(IWineD3DDevice *iface) {return WINED3D_OK;}
static void shader_none_free(IWineD3DDevice *iface) {}
static BOOL shader_none_dirty_const(IWineD3DDevice *iface) {return FALSE;}

static void shader_none_get_caps(WINED3DDEVTYPE devtype,
        const struct wined3d_gl_info *gl_info, struct shader_caps *pCaps)
{
    /* Set the shader caps to 0 for the none shader backend */
    pCaps->VertexShaderVersion  = 0;
    pCaps->PixelShaderVersion    = 0;
    pCaps->PixelShader1xMaxValue = 0.0f;
}

static BOOL shader_none_color_fixup_supported(struct color_fixup_desc fixup)
{
    if (TRACE_ON(d3d_shader) && TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    /* Faked to make some apps happy. */
    if (!is_yuv_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    TRACE("[FAILED]\n");
    return FALSE;
}

const shader_backend_t none_shader_backend = {
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
    shader_none_dirty_const,
    shader_none_get_caps,
    shader_none_color_fixup_supported,
};
