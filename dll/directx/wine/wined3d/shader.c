/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008, 2013 Stefan Dösinger for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

/* pow, mul_high, sub_high, mul_low */
const float wined3d_srgb_const0[] = {0.41666f, 1.055f, 0.055f, 12.92f};
/* cmp */
const float wined3d_srgb_const1[] = {0.0031308f, 0.0f, 0.0f, 0.0f};

static const char * const shader_opcode_names[] =
{
    /* WINED3DSIH_ABS                   */ "abs",
    /* WINED3DSIH_ADD                   */ "add",
    /* WINED3DSIH_AND                   */ "and",
    /* WINED3DSIH_BEM                   */ "bem",
    /* WINED3DSIH_BREAK                 */ "break",
    /* WINED3DSIH_BREAKC                */ "breakc",
    /* WINED3DSIH_BREAKP                */ "breakp",
    /* WINED3DSIH_CALL                  */ "call",
    /* WINED3DSIH_CALLNZ                */ "callnz",
    /* WINED3DSIH_CMP                   */ "cmp",
    /* WINED3DSIH_CND                   */ "cnd",
    /* WINED3DSIH_CRS                   */ "crs",
    /* WINED3DSIH_CUT                   */ "cut",
    /* WINED3DSIH_DCL                   */ "dcl",
    /* WINED3DSIH_DCL_CONSTANT_BUFFER   */ "dcl_constantBuffer",
    /* WINED3DSIH_DCL_INPUT_PRIMITIVE   */ "dcl_inputPrimitive",
    /* WINED3DSIH_DCL_OUTPUT_TOPOLOGY   */ "dcl_outputTopology",
    /* WINED3DSIH_DCL_VERTICES_OUT      */ "dcl_maxOutputVertexCount",
    /* WINED3DSIH_DEF                   */ "def",
    /* WINED3DSIH_DEFB                  */ "defb",
    /* WINED3DSIH_DEFI                  */ "defi",
    /* WINED3DSIH_DIV                   */ "div",
    /* WINED3DSIH_DP2                   */ "dp2",
    /* WINED3DSIH_DP2ADD                */ "dp2add",
    /* WINED3DSIH_DP3                   */ "dp3",
    /* WINED3DSIH_DP4                   */ "dp4",
    /* WINED3DSIH_DST                   */ "dst",
    /* WINED3DSIH_DSX                   */ "dsx",
    /* WINED3DSIH_DSY                   */ "dsy",
    /* WINED3DSIH_ELSE                  */ "else",
    /* WINED3DSIH_EMIT                  */ "emit",
    /* WINED3DSIH_ENDIF                 */ "endif",
    /* WINED3DSIH_ENDLOOP               */ "endloop",
    /* WINED3DSIH_ENDREP                */ "endrep",
    /* WINED3DSIH_EQ                    */ "eq",
    /* WINED3DSIH_EXP                   */ "exp",
    /* WINED3DSIH_EXPP                  */ "expp",
    /* WINED3DSIH_FRC                   */ "frc",
    /* WINED3DSIH_FTOI                  */ "ftoi",
    /* WINED3DSIH_GE                    */ "ge",
    /* WINED3DSIH_IADD                  */ "iadd",
    /* WINED3DSIH_IEQ                   */ "ieq",
    /* WINED3DSIH_IF                    */ "if",
    /* WINED3DSIH_IFC                   */ "ifc",
    /* WINED3DSIH_IGE                   */ "ige",
    /* WINED3DSIH_IMUL                  */ "imul",
    /* WINED3DSIH_ISHL                  */ "ishl",
    /* WINED3DSIH_ITOF                  */ "itof",
    /* WINED3DSIH_LABEL                 */ "label",
    /* WINED3DSIH_LD                    */ "ld",
    /* WINED3DSIH_LIT                   */ "lit",
    /* WINED3DSIH_LOG                   */ "log",
    /* WINED3DSIH_LOGP                  */ "logp",
    /* WINED3DSIH_LOOP                  */ "loop",
    /* WINED3DSIH_LRP                   */ "lrp",
    /* WINED3DSIH_LT                    */ "lt",
    /* WINED3DSIH_M3x2                  */ "m3x2",
    /* WINED3DSIH_M3x3                  */ "m3x3",
    /* WINED3DSIH_M3x4                  */ "m3x4",
    /* WINED3DSIH_M4x3                  */ "m4x3",
    /* WINED3DSIH_M4x4                  */ "m4x4",
    /* WINED3DSIH_MAD                   */ "mad",
    /* WINED3DSIH_MAX                   */ "max",
    /* WINED3DSIH_MIN                   */ "min",
    /* WINED3DSIH_MOV                   */ "mov",
    /* WINED3DSIH_MOVA                  */ "mova",
    /* WINED3DSIH_MOVC                  */ "movc",
    /* WINED3DSIH_MUL                   */ "mul",
    /* WINED3DSIH_NE                    */ "ne",
    /* WINED3DSIH_NOP                   */ "nop",
    /* WINED3DSIH_NRM                   */ "nrm",
    /* WINED3DSIH_OR                    */ "or",
    /* WINED3DSIH_PHASE                 */ "phase",
    /* WINED3DSIH_POW                   */ "pow",
    /* WINED3DSIH_RCP                   */ "rcp",
    /* WINED3DSIH_REP                   */ "rep",
    /* WINED3DSIH_RET                   */ "ret",
    /* WINED3DSIH_ROUND_NI              */ "round_ni",
    /* WINED3DSIH_RSQ                   */ "rsq",
    /* WINED3DSIH_SAMPLE                */ "sample",
    /* WINED3DSIH_SAMPLE_GRAD           */ "sample_d",
    /* WINED3DSIH_SAMPLE_LOD            */ "sample_l",
    /* WINED3DSIH_SETP                  */ "setp",
    /* WINED3DSIH_SGE                   */ "sge",
    /* WINED3DSIH_SGN                   */ "sgn",
    /* WINED3DSIH_SINCOS                */ "sincos",
    /* WINED3DSIH_SLT                   */ "slt",
    /* WINED3DSIH_SQRT                  */ "sqrt",
    /* WINED3DSIH_SUB                   */ "sub",
    /* WINED3DSIH_TEX                   */ "texld",
    /* WINED3DSIH_TEXBEM                */ "texbem",
    /* WINED3DSIH_TEXBEML               */ "texbeml",
    /* WINED3DSIH_TEXCOORD              */ "texcrd",
    /* WINED3DSIH_TEXDEPTH              */ "texdepth",
    /* WINED3DSIH_TEXDP3                */ "texdp3",
    /* WINED3DSIH_TEXDP3TEX             */ "texdp3tex",
    /* WINED3DSIH_TEXKILL               */ "texkill",
    /* WINED3DSIH_TEXLDD                */ "texldd",
    /* WINED3DSIH_TEXLDL                */ "texldl",
    /* WINED3DSIH_TEXM3x2DEPTH          */ "texm3x2depth",
    /* WINED3DSIH_TEXM3x2PAD            */ "texm3x2pad",
    /* WINED3DSIH_TEXM3x2TEX            */ "texm3x2tex",
    /* WINED3DSIH_TEXM3x3               */ "texm3x3",
    /* WINED3DSIH_TEXM3x3DIFF           */ "texm3x3diff",
    /* WINED3DSIH_TEXM3x3PAD            */ "texm3x3pad",
    /* WINED3DSIH_TEXM3x3SPEC           */ "texm3x3spec",
    /* WINED3DSIH_TEXM3x3TEX            */ "texm3x3tex",
    /* WINED3DSIH_TEXM3x3VSPEC          */ "texm3x3vspec",
    /* WINED3DSIH_TEXREG2AR             */ "texreg2ar",
    /* WINED3DSIH_TEXREG2GB             */ "texreg2gb",
    /* WINED3DSIH_TEXREG2RGB            */ "texreg2rgb",
    /* WINED3DSIH_UDIV                  */ "udiv",
    /* WINED3DSIH_UGE                   */ "uge",
    /* WINED3DSIH_USHR                  */ "ushr",
    /* WINED3DSIH_UTOF                  */ "utof",
    /* WINED3DSIH_XOR                   */ "xor",
};

static const char * const semantic_names[] =
{
    /* WINED3D_DECL_USAGE_POSITION      */ "SV_POSITION",
    /* WINED3D_DECL_USAGE_BLEND_WEIGHT  */ "BLENDWEIGHT",
    /* WINED3D_DECL_USAGE_BLEND_INDICES */ "BLENDINDICES",
    /* WINED3D_DECL_USAGE_NORMAL        */ "NORMAL",
    /* WINED3D_DECL_USAGE_PSIZE         */ "PSIZE",
    /* WINED3D_DECL_USAGE_TEXCOORD      */ "TEXCOORD",
    /* WINED3D_DECL_USAGE_TANGENT       */ "TANGENT",
    /* WINED3D_DECL_USAGE_BINORMAL      */ "BINORMAL",
    /* WINED3D_DECL_USAGE_TESS_FACTOR   */ "TESSFACTOR",
    /* WINED3D_DECL_USAGE_POSITIONT     */ "POSITIONT",
    /* WINED3D_DECL_USAGE_COLOR         */ "COLOR",
    /* WINED3D_DECL_USAGE_FOG           */ "FOG",
    /* WINED3D_DECL_USAGE_DEPTH         */ "DEPTH",
    /* WINED3D_DECL_USAGE_SAMPLE        */ "SAMPLE",
};

static const char *shader_semantic_name_from_usage(enum wined3d_decl_usage usage)
{
    if (usage >= sizeof(semantic_names) / sizeof(*semantic_names))
    {
        FIXME("Unrecognized usage %#x.\n", usage);
        return "UNRECOGNIZED";
    }

    return semantic_names[usage];
}

static enum wined3d_decl_usage shader_usage_from_semantic_name(const char *name)
{
    unsigned int i;

    for (i = 0; i < sizeof(semantic_names) / sizeof(*semantic_names); ++i)
    {
        if (!strcmp(name, semantic_names[i])) return i;
    }

    return ~0U;
}

BOOL shader_match_semantic(const char *semantic_name, enum wined3d_decl_usage usage)
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
    e->register_idx = s->reg.reg.idx[0].offset;
    e->mask = s->reg.write_mask;
}

static void shader_signature_from_usage(struct wined3d_shader_signature_element *e,
        enum wined3d_decl_usage usage, UINT usage_idx, UINT reg_idx, DWORD write_mask)
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

void string_buffer_clear(struct wined3d_string_buffer *buffer)
{
    buffer->buffer[0] = '\0';
    buffer->content_size = 0;
}

BOOL string_buffer_init(struct wined3d_string_buffer *buffer)
{
    buffer->buffer_size = 32;
    if (!(buffer->buffer = HeapAlloc(GetProcessHeap(), 0, buffer->buffer_size)))
    {
        ERR("Failed to allocate shader buffer memory.\n");
        return FALSE;
    }

    string_buffer_clear(buffer);
    return TRUE;
}

void string_buffer_free(struct wined3d_string_buffer *buffer)
{
    HeapFree(GetProcessHeap(), 0, buffer->buffer);
}

BOOL string_buffer_resize(struct wined3d_string_buffer *buffer, int rc)
{
    char *new_buffer;
    unsigned int new_buffer_size = buffer->buffer_size * 2;

    while (rc > 0 && (unsigned int)rc >= new_buffer_size - buffer->content_size)
        new_buffer_size *= 2;
    if (!(new_buffer = HeapReAlloc(GetProcessHeap(), 0, buffer->buffer, new_buffer_size)))
    {
        ERR("Failed to grow buffer.\n");
        buffer->buffer[buffer->content_size] = '\0';
        return FALSE;
    }
    buffer->buffer = new_buffer;
    buffer->buffer_size = new_buffer_size;
    return TRUE;
}

int shader_vaddline(struct wined3d_string_buffer *buffer, const char *format, va_list args)
{
    unsigned int rem;
    int rc;

    rem = buffer->buffer_size - buffer->content_size;
    rc = vsnprintf(&buffer->buffer[buffer->content_size], rem, format, args);
    if (rc < 0 /* C89 */ || (unsigned int)rc >= rem /* C99 */)
        return rc;

    buffer->content_size += rc;
    return 0;
}

int shader_addline(struct wined3d_string_buffer *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    for (;;)
    {
        va_start(args, format);
        ret = shader_vaddline(buffer, format, args);
        va_end(args);
        if (!ret)
            return ret;
        if (!string_buffer_resize(buffer, ret))
            return -1;
    }
}

struct wined3d_string_buffer *string_buffer_get(struct wined3d_string_buffer_list *list)
{
    struct wined3d_string_buffer *buffer;

    if (list_empty(&list->list))
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(*buffer));
        if (!buffer || !string_buffer_init(buffer))
        {
            ERR("Couldn't allocate buffer for temporary string.\n");
            if (buffer)
                HeapFree(GetProcessHeap(), 0, buffer);
            return NULL;
        }
    }
    else
    {
        buffer = LIST_ENTRY(list_head(&list->list), struct wined3d_string_buffer, entry);
        list_remove(&buffer->entry);
    }
    string_buffer_clear(buffer);
    return buffer;
}

static int string_buffer_vsprintf(struct wined3d_string_buffer *buffer, const char *format, va_list args)
{
    if (!buffer)
        return 0;
    string_buffer_clear(buffer);
    return shader_vaddline(buffer, format, args);
}

void string_buffer_sprintf(struct wined3d_string_buffer *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    for (;;)
    {
        va_start(args, format);
        ret = string_buffer_vsprintf(buffer, format, args);
        va_end(args);
        if (!ret)
            return;
        if (!string_buffer_resize(buffer, ret))
            return;
    }
}

void string_buffer_release(struct wined3d_string_buffer_list *list, struct wined3d_string_buffer *buffer)
{
    if (!buffer)
        return;
    list_add_head(&list->list, &buffer->entry);
}

void string_buffer_list_init(struct wined3d_string_buffer_list *list)
{
    list_init(&list->list);
}

void string_buffer_list_cleanup(struct wined3d_string_buffer_list *list)
{
    struct wined3d_string_buffer *buffer, *buffer_next;

    LIST_FOR_EACH_ENTRY_SAFE(buffer, buffer_next, &list->list, struct wined3d_string_buffer, entry)
    {
        string_buffer_free(buffer);
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    list_init(&list->list);
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
    struct wined3d_shader_lconst *constant, *constant_next;

    LIST_FOR_EACH_ENTRY_SAFE(constant, constant_next, clist, struct wined3d_shader_lconst, entry)
        HeapFree(GetProcessHeap(), 0, constant);
    list_init(clist);
}

static void shader_set_limits(struct wined3d_shader *shader)
{
    static const struct limits_entry
    {
        unsigned int min_version;
        unsigned int max_version;
        struct wined3d_shader_limits limits;
    }
    vs_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packed_input */
        {WINED3D_SHADER_VERSION(1, 0), WINED3D_SHADER_VERSION(1, 1), { 0,  0, 256,  0, 12,  0}},
        {WINED3D_SHADER_VERSION(2, 0), WINED3D_SHADER_VERSION(2, 1), { 0, 16, 256, 16, 12,  0}},
        /* DX10 cards on Windows advertise a D3D9 constant limit of 256
         * even though they are capable of supporting much more (GL
         * drivers advertise 1024). d3d9.dll and d3d8.dll clamp the
         * wined3d-advertised maximum. Clamp the constant limit for <= 3.0
         * shaders to 256. */
        {WINED3D_SHADER_VERSION(3, 0), WINED3D_SHADER_VERSION(3, 0), { 4, 16, 256, 16, 12,  0}},
        {WINED3D_SHADER_VERSION(4, 0), WINED3D_SHADER_VERSION(4, 0), {16,  0,   0,  0, 16,  0}},
        {0}
    },
    gs_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packed_input */
        {WINED3D_SHADER_VERSION(4, 0), WINED3D_SHADER_VERSION(4, 0), {16,  0,   0,  0, 32, 16}},
        {0}
    },
    ps_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packed_input */
        {WINED3D_SHADER_VERSION(1, 0), WINED3D_SHADER_VERSION(1, 3), { 4,  0,   8,  0,  0,  0}},
        {WINED3D_SHADER_VERSION(1, 4), WINED3D_SHADER_VERSION(1, 4), { 6,  0,   8,  0,  0,  0}},
        {WINED3D_SHADER_VERSION(2, 0), WINED3D_SHADER_VERSION(2, 0), {16,  0,  32,  0,  0,  0}},
        {WINED3D_SHADER_VERSION(2, 1), WINED3D_SHADER_VERSION(2, 1), {16, 16,  32, 16,  0,  0}},
        {WINED3D_SHADER_VERSION(3, 0), WINED3D_SHADER_VERSION(3, 0), {16, 16, 224, 16,  0, 12}},
        {WINED3D_SHADER_VERSION(4, 0), WINED3D_SHADER_VERSION(4, 0), {16,  0,   0,  0,  0, 32}},
        {0}
    };
    const struct limits_entry *limits_array;
    DWORD shader_version = WINED3D_SHADER_VERSION(shader->reg_maps.shader_version.major,
            shader->reg_maps.shader_version.minor);
    int i = 0;

    switch (shader->reg_maps.shader_version.type)
    {
        default:
            FIXME("Unexpected shader type %u found.\n", shader->reg_maps.shader_version.type);
            /* Fall-through. */
        case WINED3D_SHADER_TYPE_VERTEX:
            limits_array = vs_limits;
            break;
        case WINED3D_SHADER_TYPE_GEOMETRY:
            limits_array = gs_limits;
            break;
        case WINED3D_SHADER_TYPE_PIXEL:
            limits_array = ps_limits;
            break;
    }

    while (limits_array[i].min_version && limits_array[i].min_version <= shader_version)
    {
        if (shader_version <= limits_array[i].max_version)
        {
            shader->limits = &limits_array[i].limits;
            break;
        }
        ++i;
    }
    if (!shader->limits)
    {
        FIXME("Unexpected shader version \"%u.%u\".\n",
                shader->reg_maps.shader_version.major,
                shader->reg_maps.shader_version.minor);
        shader->limits = &limits_array[max(0, i - 1)].limits;
    }
}

static inline void set_bitmap_bit(DWORD *bitmap, DWORD bit)
{
    DWORD idx, shift;
    idx = bit >> 5;
    shift = bit & 0x1f;
    bitmap[idx] |= (1u << shift);
}

static BOOL shader_record_register_usage(struct wined3d_shader *shader, struct wined3d_shader_reg_maps *reg_maps,
        const struct wined3d_shader_register *reg, enum wined3d_shader_type shader_type, unsigned int constf_size)
{
    switch (reg->type)
    {
        case WINED3DSPR_TEXTURE: /* WINED3DSPR_ADDR */
            if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
                reg_maps->texcoord |= 1u << reg->idx[0].offset;
            else
                reg_maps->address |= 1u << reg->idx[0].offset;
            break;

        case WINED3DSPR_TEMP:
            reg_maps->temporary |= 1u << reg->idx[0].offset;
            break;

        case WINED3DSPR_INPUT:
            if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
            {
                if (reg->idx[0].rel_addr)
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
                    shader->u.ps.input_reg_used[reg->idx[0].offset] = TRUE;
                }
            }
            else
                reg_maps->input_registers |= 1u << reg->idx[0].offset;
            break;

        case WINED3DSPR_RASTOUT:
            if (reg->idx[0].offset == 1)
                reg_maps->fog = 1;
            if (reg->idx[0].offset == 2)
                reg_maps->point_size = 1;
            break;

        case WINED3DSPR_MISCTYPE:
            if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
            {
                if (!reg->idx[0].offset)
                    reg_maps->vpos = 1;
                else if (reg->idx[0].offset == 1)
                    reg_maps->usesfacing = 1;
            }
            break;

        case WINED3DSPR_CONST:
            if (reg->idx[0].rel_addr)
            {
                if (reg->idx[0].offset < reg_maps->min_rel_offset)
                    reg_maps->min_rel_offset = reg->idx[0].offset;
                if (reg->idx[0].offset > reg_maps->max_rel_offset)
                    reg_maps->max_rel_offset = reg->idx[0].offset;
                reg_maps->usesrelconstF = TRUE;
            }
            else
            {
                if (reg->idx[0].offset >= min(shader->limits->constant_float, constf_size))
                {
                    WARN("Shader using float constant %u which is not supported.\n", reg->idx[0].offset);
                    return FALSE;
                }
                else
                {
                    set_bitmap_bit(reg_maps->constf, reg->idx[0].offset);
                }
            }
            break;

        case WINED3DSPR_CONSTINT:
            if (reg->idx[0].offset >= shader->limits->constant_int)
            {
                WARN("Shader using integer constant %u which is not supported.\n", reg->idx[0].offset);
                return FALSE;
            }
            else
            {
                reg_maps->integer_constants |= (1u << reg->idx[0].offset);
            }
            break;

        case WINED3DSPR_CONSTBOOL:
            if (reg->idx[0].offset >= shader->limits->constant_bool)
            {
                WARN("Shader using bool constant %u which is not supported.\n", reg->idx[0].offset);
                return FALSE;
            }
            else
            {
                reg_maps->boolean_constants |= (1u << reg->idx[0].offset);
            }
            break;

        case WINED3DSPR_COLOROUT:
            reg_maps->rt_mask |= (1u << reg->idx[0].offset);
            break;

        default:
            TRACE("Not recording register of type %#x and [%#x][%#x].\n",
                    reg->type, reg->idx[0].offset, reg->idx[1].offset);
            break;
    }
    return TRUE;
}

static void shader_record_sample(struct wined3d_shader_reg_maps *reg_maps,
        unsigned int resource_idx, unsigned int sampler_idx, unsigned int bind_idx)
{
    struct wined3d_shader_sampler_map_entry *entries, *entry;
    struct wined3d_shader_sampler_map *map;
    unsigned int i;

    map = &reg_maps->sampler_map;
    entries = map->entries;
    for (i = 0; i < map->count; ++i)
    {
        if (entries[i].resource_idx == resource_idx && entries[i].sampler_idx == sampler_idx)
            return;
    }

    if (!map->size)
    {
        if (!(entries = HeapAlloc(GetProcessHeap(), 0, sizeof(*entries) * 4)))
        {
            ERR("Failed to allocate sampler map entries.\n");
            return;
        }
        map->size = 4;
        map->entries = entries;
    }
    else if (map->count == map->size)
    {
        size_t new_size = map->size * 2;

        if (sizeof(*entries) * new_size <= sizeof(*entries) * map->size
                || !(entries = HeapReAlloc(GetProcessHeap(), 0, entries, sizeof(*entries) * new_size)))
        {
            ERR("Failed to resize sampler map entries.\n");
            return;
        }
        map->size = new_size;
        map->entries = entries;
    }

    entry = &entries[map->count++];
    entry->resource_idx = resource_idx;
    entry->sampler_idx = sampler_idx;
    entry->bind_idx = bind_idx;
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
        struct wined3d_shader_reg_maps *reg_maps, struct wined3d_shader_signature *input_signature,
        struct wined3d_shader_signature *output_signature, const DWORD *byte_code, DWORD constf_size)
{
    struct wined3d_shader_signature_element input_signature_elements[max(MAX_ATTRIBS, MAX_REG_INPUT)];
    struct wined3d_shader_signature_element output_signature_elements[MAX_REG_OUTPUT];
    unsigned int cur_loop_depth = 0, max_loop_depth = 0;
    void *fe_data = shader->frontend_data;
    struct wined3d_shader_version shader_version;
    const DWORD *ptr = byte_code;
    unsigned int i;

    memset(reg_maps, 0, sizeof(*reg_maps));
    memset(input_signature_elements, 0, sizeof(input_signature_elements));
    memset(output_signature_elements, 0, sizeof(output_signature_elements));
    reg_maps->min_rel_offset = ~0U;

    fe->shader_read_header(fe_data, &ptr, &shader_version);
    reg_maps->shader_version = shader_version;

    shader_set_limits(shader);

    reg_maps->constf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(*reg_maps->constf) * ((min(shader->limits->constant_float, constf_size) + 31) / 32));
    if (!reg_maps->constf)
    {
        ERR("Failed to allocate constant map memory.\n");
        return E_OUTOFMEMORY;
    }

    while (!fe->shader_is_end(fe_data, &ptr))
    {
        struct wined3d_shader_instruction ins;

        /* Fetch opcode. */
        fe->shader_read_instruction(fe_data, &ptr, &ins);

        /* Unhandled opcode, and its parameters. */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            continue;
        }

        /* Handle declarations. */
        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            struct wined3d_shader_semantic *semantic = &ins.declaration.semantic;
            unsigned int reg_idx = semantic->reg.reg.idx[0].offset;

            switch (semantic->reg.reg.type)
            {
                /* Mark input registers used. */
                case WINED3DSPR_INPUT:
                    if (reg_idx >= MAX_REG_INPUT)
                    {
                        ERR("Invalid input register index %u.\n", reg_idx);
                        break;
                    }
                    if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL && shader_version.major == 3
                            && semantic->usage == WINED3D_DECL_USAGE_POSITION && !semantic->usage_idx)
                        return WINED3DERR_INVALIDCALL;
                    reg_maps->input_registers |= 1u << reg_idx;
                    shader_signature_from_semantic(&input_signature_elements[reg_idx], semantic);
                    break;

                /* Vertex shader: mark 3.0 output registers used, save token. */
                case WINED3DSPR_OUTPUT:
                    if (reg_idx >= MAX_REG_OUTPUT)
                    {
                        ERR("Invalid output register index %u.\n", reg_idx);
                        break;
                    }
                    reg_maps->output_registers |= 1u << reg_idx;
                    shader_signature_from_semantic(&output_signature_elements[reg_idx], semantic);
                    if (semantic->usage == WINED3D_DECL_USAGE_FOG)
                        reg_maps->fog = 1;
                    if (semantic->usage == WINED3D_DECL_USAGE_PSIZE)
                        reg_maps->point_size = 1;
                    break;

                case WINED3DSPR_SAMPLER:
                    shader_record_sample(reg_maps, reg_idx, reg_idx, reg_idx);
                case WINED3DSPR_RESOURCE:
                    if (reg_idx >= ARRAY_SIZE(reg_maps->resource_info))
                    {
                        ERR("Invalid resource index %u.\n", reg_idx);
                        break;
                    }
                    reg_maps->resource_info[reg_idx].type = semantic->resource_type;
                    reg_maps->resource_info[reg_idx].data_type = semantic->resource_data_type;
                    break;

                default:
                    TRACE("Not recording DCL register type %#x.\n", semantic->reg.reg.type);
                    break;
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_CONSTANT_BUFFER)
        {
            struct wined3d_shader_register *reg = &ins.declaration.src.reg;
            if (reg->idx[0].offset >= WINED3D_MAX_CBS)
                ERR("Invalid CB index %u.\n", reg->idx[0].offset);
            else
                reg_maps->cb_sizes[reg->idx[0].offset] = reg->idx[1].offset;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PRIMITIVE)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_GEOMETRY)
                shader->u.gs.input_type = ins.declaration.primitive_type;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_OUTPUT_TOPOLOGY)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_GEOMETRY)
                shader->u.gs.output_type = ins.declaration.primitive_type;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_VERTICES_OUT)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_GEOMETRY)
                shader->u.gs.vertices_out = ins.declaration.count;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
            float *value;
            if (!lconst) return E_OUTOFMEMORY;

            lconst->idx = ins.dst[0].reg.idx[0].offset;
            memcpy(lconst->value, ins.src[0].reg.immconst_data, 4 * sizeof(DWORD));
            value = (float *)lconst->value;

            /* In pixel shader 1.X shaders, the constants are clamped between [-1;1] */
            if (shader_version.major == 1 && shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
            {
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

            if (isinf(value[0]) || isnan(value[0]) || isinf(value[1]) || isnan(value[1])
                    || isinf(value[2]) || isnan(value[2]) || isinf(value[3]) || isnan(value[3]))
            {
                shader->lconst_inf_or_nan = TRUE;
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
            if (!lconst) return E_OUTOFMEMORY;

            lconst->idx = ins.dst[0].reg.idx[0].offset;
            memcpy(lconst->value, ins.src[0].reg.immconst_data, 4 * sizeof(DWORD));

            list_add_head(&shader->constantsI, &lconst->entry);
            reg_maps->local_int_consts |= (1u << lconst->idx);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
            if (!lconst) return E_OUTOFMEMORY;

            lconst->idx = ins.dst[0].reg.idx[0].offset;
            memcpy(lconst->value, ins.src[0].reg.immconst_data, sizeof(DWORD));

            list_add_head(&shader->constantsB, &lconst->entry);
            reg_maps->local_bool_consts |= (1u << lconst->idx);
        }
        /* For subroutine prototypes. */
        else if (ins.handler_idx == WINED3DSIH_LABEL)
        {
            reg_maps->labels |= 1u << ins.src[0].reg.idx[0].offset;
        }
        /* Set texture, address, temporary registers. */
        else
        {
            BOOL color0_mov = FALSE;
            unsigned int i;

            /* This will loop over all the registers and try to
             * make a bitmask of the ones we're interested in.
             *
             * Relative addressing tokens are ignored, but that's
             * okay, since we'll catch any address registers when
             * they are initialized (required by spec). */
            for (i = 0; i < ins.dst_count; ++i)
            {
                if (!shader_record_register_usage(shader, reg_maps, &ins.dst[i].reg,
                        shader_version.type, constf_size))
                    return WINED3DERR_INVALIDCALL;

                if (shader_version.type == WINED3D_SHADER_TYPE_VERTEX)
                {
                    UINT idx = ins.dst[i].reg.idx[0].offset;

                    switch (ins.dst[i].reg.type)
                    {
                        case WINED3DSPR_RASTOUT:
                            if (shader_version.major >= 3)
                                break;
                            switch (idx)
                            {
                                case 0: /* oPos */
                                    reg_maps->output_registers |= 1u << 10;
                                    shader_signature_from_usage(&output_signature_elements[10],
                                            WINED3D_DECL_USAGE_POSITION, 0, 10, WINED3DSP_WRITEMASK_ALL);
                                    break;

                                case 1: /* oFog */
                                    reg_maps->output_registers |= 1u << 11;
                                    shader_signature_from_usage(&output_signature_elements[11],
                                            WINED3D_DECL_USAGE_FOG, 0, 11, WINED3DSP_WRITEMASK_0);
                                    break;

                                case 2: /* oPts */
                                    reg_maps->output_registers |= 1u << 11;
                                    shader_signature_from_usage(&output_signature_elements[11],
                                            WINED3D_DECL_USAGE_PSIZE, 0, 11, WINED3DSP_WRITEMASK_1);
                                    break;
                            }
                            break;

                        case WINED3DSPR_ATTROUT:
                            if (shader_version.major >= 3)
                                break;
                            if (idx < 2)
                            {
                                idx += 8;
                                if (reg_maps->output_registers & (1u << idx))
                                {
                                    output_signature_elements[idx].mask |= ins.dst[i].write_mask;
                                }
                                else
                                {
                                    reg_maps->output_registers |= 1u << idx;
                                    shader_signature_from_usage(&output_signature_elements[idx],
                                            WINED3D_DECL_USAGE_COLOR, idx - 8, idx, ins.dst[i].write_mask);
                                }
                            }
                            break;

                        case WINED3DSPR_TEXCRDOUT:
                            if (shader_version.major >= 3)
                            {
                                reg_maps->u.output_registers_mask[idx] |= ins.dst[i].write_mask;
                                break;
                            }
                            reg_maps->u.texcoord_mask[idx] |= ins.dst[i].write_mask;
                            if (reg_maps->output_registers & (1u << idx))
                            {
                                output_signature_elements[idx].mask |= ins.dst[i].write_mask;
                            }
                            else
                            {
                                reg_maps->output_registers |= 1u << idx;
                                shader_signature_from_usage(&output_signature_elements[idx],
                                        WINED3D_DECL_USAGE_TEXCOORD, idx, idx, ins.dst[i].write_mask);
                            }
                            break;

                        default:
                            break;
                    }
                }

                if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
                {
                    if (ins.dst[i].reg.type == WINED3DSPR_COLOROUT && !ins.dst[i].reg.idx[0].offset)
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
                                && ins.dst[i].write_mask == WINED3DSP_WRITEMASK_ALL)
                        {
                            /* Used later when the source register is read. */
                            color0_mov = TRUE;
                        }
                    }
                    /* Also drop the MOV marker if the source register is overwritten prior to the shader
                     * end
                     */
                    else if (ins.dst[i].reg.type == WINED3DSPR_TEMP
                            && ins.dst[i].reg.idx[0].offset == shader->u.ps.color0_reg)
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
                    unsigned int reg_idx = ins.dst[i].reg.idx[0].offset;

                    TRACE("Setting fake 2D resource for 1.x pixelshader.\n");
                    reg_maps->resource_info[reg_idx].type = WINED3D_SHADER_RESOURCE_TEXTURE_2D;
                    reg_maps->resource_info[reg_idx].data_type = WINED3D_DATA_FLOAT;
                    shader_record_sample(reg_maps, reg_idx, reg_idx, reg_idx);

                    /* texbem is only valid with < 1.4 pixel shaders */
                    if (ins.handler_idx == WINED3DSIH_TEXBEM
                            || ins.handler_idx == WINED3DSIH_TEXBEML)
                    {
                        reg_maps->bumpmat |= 1u << reg_idx;
                        if (ins.handler_idx == WINED3DSIH_TEXBEML)
                        {
                            reg_maps->luminanceparams |= 1u << reg_idx;
                        }
                    }
                }
                else if (ins.handler_idx == WINED3DSIH_BEM)
                {
                    reg_maps->bumpmat |= 1u << ins.dst[i].reg.idx[0].offset;
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
            {
                --cur_loop_depth;
            }
            else if (ins.handler_idx == WINED3DSIH_SAMPLE
                    || ins.handler_idx == WINED3DSIH_SAMPLE_GRAD
                    || ins.handler_idx == WINED3DSIH_SAMPLE_LOD)
            {
                shader_record_sample(reg_maps, ins.src[1].reg.idx[0].offset,
                        ins.src[2].reg.idx[0].offset, reg_maps->sampler_map.count);
            }

            if (ins.predicate)
                if (!shader_record_register_usage(shader, reg_maps, &ins.predicate->reg,
                        shader_version.type, constf_size))
                    return WINED3DERR_INVALIDCALL;

            for (i = 0; i < ins.src_count; ++i)
            {
                unsigned int count = get_instr_extra_regcount(ins.handler_idx, i);
                struct wined3d_shader_register reg = ins.src[i].reg;

                if (!shader_record_register_usage(shader, reg_maps, &ins.src[i].reg,
                        shader_version.type, constf_size))
                    return WINED3DERR_INVALIDCALL;
                while (count)
                {
                    ++reg.idx[0].offset;
                    if (!shader_record_register_usage(shader, reg_maps, &reg,
                            shader_version.type, constf_size))
                        return WINED3DERR_INVALIDCALL;
                    --count;
                }

                if (color0_mov)
                {
                    if (ins.src[i].reg.type == WINED3DSPR_TEMP
                            && ins.src[i].swizzle == WINED3DSP_NOSWIZZLE)
                    {
                        shader->u.ps.color0_mov = TRUE;
                        shader->u.ps.color0_reg = ins.src[i].reg.idx[0].offset;
                    }
                }
            }
        }
    }
    reg_maps->loop_depth = max_loop_depth;

    /* PS before 2.0 don't have explicit color outputs. Instead the value of
     * R0 is written to the render target. */
    if (shader_version.major < 2 && shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
        reg_maps->rt_mask |= (1u << 0);

    shader->functionLength = ((const char *)ptr - (const char *)byte_code);

    if (input_signature->elements)
    {
        for (i = 0; i < input_signature->element_count; ++i)
        {
            reg_maps->input_registers |= 1u << input_signature->elements[i].register_idx;
            if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL
                    && input_signature->elements[i].sysval_semantic == WINED3D_SV_POSITION)
                reg_maps->vpos = 1;
        }
    }
    else if (!input_signature->elements && reg_maps->input_registers)
    {
        unsigned int count = count_bits(reg_maps->input_registers);
        struct wined3d_shader_signature_element *e;
        unsigned int i;

        if (!(input_signature->elements = HeapAlloc(GetProcessHeap(), 0, sizeof(*input_signature->elements) * count)))
            return E_OUTOFMEMORY;
        input_signature->element_count = count;

        e = input_signature->elements;
        for (i = 0; i < ARRAY_SIZE(input_signature_elements); ++i)
        {
            if (!(reg_maps->input_registers & (1u << i)))
                continue;
            input_signature_elements[i].register_idx = i;
            *e++ = input_signature_elements[i];
        }
    }

    if (output_signature->elements)
    {
        for (i = 0; i < output_signature->element_count; ++i)
        {
            reg_maps->output_registers |= 1u << output_signature->elements[i].register_idx;
        }
    }
    else if (reg_maps->output_registers)
    {
        unsigned int count = count_bits(reg_maps->output_registers);
        struct wined3d_shader_signature_element *e;

        if (!(output_signature->elements = HeapAlloc(GetProcessHeap(), 0, sizeof(*output_signature->elements) * count)))
            return E_OUTOFMEMORY;
        output_signature->element_count = count;

        e = output_signature->elements;
        for (i = 0; i < ARRAY_SIZE(output_signature_elements); ++i)
        {
            if (!(reg_maps->output_registers & (1u << i)))
                continue;
            *e++ = output_signature_elements[i];
        }
    }

    return WINED3D_OK;
}

unsigned int shader_find_free_input_register(const struct wined3d_shader_reg_maps *reg_maps, unsigned int max)
{
    DWORD map = 1u << max;
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
        switch (semantic->resource_type)
        {
            case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
                TRACE("_2d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
                TRACE("_3d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
                TRACE("_cube");
                break;

            default:
                TRACE("_unknown_ttype(0x%08x)", semantic->resource_type);
                break;
        }
    }
    else if (semantic->reg.reg.type == WINED3DSPR_RESOURCE)
    {
        TRACE("_resource_");
        switch (semantic->resource_type)
        {
            case WINED3D_SHADER_RESOURCE_BUFFER:
                TRACE("buffer");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1D:
                TRACE("texture1d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
                TRACE("texture2d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DMS:
                TRACE("texture2dms");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
                TRACE("texture3d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
                TRACE("texturecube");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY:
                TRACE("texture1darray");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY:
                TRACE("texture2darray");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY:
                TRACE("texture2dmsarray");
                break;

            default:
                TRACE("unknown");
                break;
        }
        switch (semantic->resource_data_type)
        {
            case WINED3D_DATA_FLOAT:
                TRACE(" (float)");
                break;

            case WINED3D_DATA_INT:
                TRACE(" (int)");
                break;

            case WINED3D_DATA_UINT:
                TRACE(" (uint)");
                break;

            case WINED3D_DATA_UNORM:
                TRACE(" (unorm)");
                break;

            case WINED3D_DATA_SNORM:
                TRACE(" (snorm)");
                break;

            default:
                TRACE(" (unknown)");
                break;
        }
    }
    else
    {
        /* Pixel shaders 3.0 don't have usage semantics. */
        if (shader_version->major < 3 && shader_version->type == WINED3D_SHADER_TYPE_PIXEL) return;
        else TRACE("_");

        switch (semantic->usage)
        {
            case WINED3D_DECL_USAGE_POSITION:
                TRACE("position%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_BLEND_INDICES:
                TRACE("blend");
                break;

            case WINED3D_DECL_USAGE_BLEND_WEIGHT:
                TRACE("weight");
                break;

            case WINED3D_DECL_USAGE_NORMAL:
                TRACE("normal%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_PSIZE:
                TRACE("psize");
                break;

            case WINED3D_DECL_USAGE_COLOR:
                if (!semantic->usage_idx) TRACE("color");
                else TRACE("specular%u", (semantic->usage_idx - 1));
                break;

            case WINED3D_DECL_USAGE_TEXCOORD:
                TRACE("texture%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_TANGENT:
                TRACE("tangent");
                break;

            case WINED3D_DECL_USAGE_BINORMAL:
                TRACE("binormal");
                break;

            case WINED3D_DECL_USAGE_TESS_FACTOR:
                TRACE("tessfactor");
                break;

            case WINED3D_DECL_USAGE_POSITIONT:
                TRACE("positionT%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_FOG:
                TRACE("fog");
                break;

            case WINED3D_DECL_USAGE_DEPTH:
                TRACE("depth");
                break;

            case WINED3D_DECL_USAGE_SAMPLE:
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
    UINT offset = reg->idx[0].offset;

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
            offset = shader_get_float_offset(reg->type, offset);
            break;

        case WINED3DSPR_TEXTURE: /* vs: case WINED3DSPR_ADDR */
            TRACE("%c", shader_version->type == WINED3D_SHADER_TYPE_PIXEL ? 't' : 'a');
            break;

        case WINED3DSPR_RASTOUT:
            TRACE("%s", rastout_reg_names[offset]);
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
            if (offset > 1)
                FIXME("Unhandled misctype register %u.\n", offset);
            else
                TRACE("%s", misctype_reg_names[offset]);
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

        case WINED3DSPR_PRIMID:
            TRACE("primID");
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
                switch (reg->data_type)
                {
                    case WINED3D_DATA_FLOAT:
                        TRACE("%.8e", *(const float *)reg->immconst_data);
                        break;
                    case WINED3D_DATA_INT:
                        TRACE("%d", reg->immconst_data[0]);
                        break;
                    case WINED3D_DATA_RESOURCE:
                    case WINED3D_DATA_SAMPLER:
                    case WINED3D_DATA_UINT:
                        TRACE("%u", reg->immconst_data[0]);
                        break;
                    default:
                        TRACE("<unhandled data type %#x>", reg->data_type);
                        break;
                }
                break;

            case WINED3D_IMMCONST_VEC4:
                switch (reg->data_type)
                {
                    case WINED3D_DATA_FLOAT:
                        TRACE("%.8e, %.8e, %.8e, %.8e",
                                *(const float *)&reg->immconst_data[0], *(const float *)&reg->immconst_data[1],
                                *(const float *)&reg->immconst_data[2], *(const float *)&reg->immconst_data[3]);
                        break;
                    case WINED3D_DATA_INT:
                        TRACE("%d, %d, %d, %d",
                                reg->immconst_data[0], reg->immconst_data[1],
                                reg->immconst_data[2], reg->immconst_data[3]);
                        break;
                    case WINED3D_DATA_RESOURCE:
                    case WINED3D_DATA_SAMPLER:
                    case WINED3D_DATA_UINT:
                        TRACE("%u, %u, %u, %u",
                                reg->immconst_data[0], reg->immconst_data[1],
                                reg->immconst_data[2], reg->immconst_data[3]);
                        break;
                    default:
                        TRACE("<unhandled data type %#x>", reg->data_type);
                        break;
                }
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
        if (offset != ~0U)
        {
            TRACE("[");
            if (reg->idx[0].rel_addr)
            {
                shader_dump_src_param(reg->idx[0].rel_addr, shader_version);
                TRACE(" + ");
            }
            TRACE("%u]", offset);

            if (reg->idx[1].offset != ~0U)
            {
                TRACE("[");
                if (reg->idx[1].rel_addr)
                {
                    shader_dump_src_param(reg->idx[1].rel_addr, shader_version);
                    TRACE(" + ");
                }
                TRACE("%u]", reg->idx[1].offset);
            }
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
        static const char write_mask_chars[] = "xyzw";

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

    if (swizzle != WINED3DSP_NOSWIZZLE)
    {
        static const char swizzle_chars[] = "xyzw";
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
void shader_generate_main(const struct wined3d_shader *shader, struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_reg_maps *reg_maps, const DWORD *byte_code, void *backend_ctx)
{
    struct wined3d_device *device = shader->device;
    const struct wined3d_shader_frontend *fe = shader->frontend;
    void *fe_data = shader->frontend_data;
    struct wined3d_shader_version shader_version;
    struct wined3d_shader_loop_state loop_state;
    struct wined3d_shader_instruction ins;
    struct wined3d_shader_tex_mx tex_mx;
    struct wined3d_shader_context ctx;
    const DWORD *ptr = byte_code;

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

    fe->shader_read_header(fe_data, &ptr, &shader_version);

    while (!fe->shader_is_end(fe_data, &ptr))
    {
        /* Read opcode. */
        fe->shader_read_instruction(fe_data, &ptr, &ins);

        /* Unknown opcode and its parameters. */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            continue;
        }

        if (ins.predicate)
            FIXME("Predicates not implemented.\n");

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

static void shader_dump_primitive_type(enum wined3d_primitive_type primitive_type)
{
    switch (primitive_type)
    {
        case WINED3D_PT_UNDEFINED:
            TRACE("undefined");
            break;
        case WINED3D_PT_POINTLIST:
            TRACE("pointlist");
            break;
        case WINED3D_PT_LINELIST:
            TRACE("linelist");
            break;
        case WINED3D_PT_LINESTRIP:
            TRACE("linestrip");
            break;
        case WINED3D_PT_TRIANGLELIST:
            TRACE("trianglelist");
            break;
        case WINED3D_PT_TRIANGLESTRIP:
            TRACE("trianglestrip");
            break;
        case WINED3D_PT_TRIANGLEFAN:
            TRACE("trianglefan");
            break;
        case WINED3D_PT_LINELIST_ADJ:
            TRACE("linelist_adj");
            break;
        case WINED3D_PT_LINESTRIP_ADJ:
            TRACE("linestrip_adj");
            break;
        case WINED3D_PT_TRIANGLELIST_ADJ:
            TRACE("trianglelist_adj");
            break;
        case WINED3D_PT_TRIANGLESTRIP_ADJ:
            TRACE("trianglestrip_adj");
            break;
        default:
            TRACE("<unrecognized_primitive_type %#x>", primitive_type);
            break;
    }
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

        fe->shader_read_instruction(fe_data, &ptr, &ins);
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            continue;
        }

        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            shader_dump_decl_usage(&ins.declaration.semantic, &shader_version);
            shader_dump_ins_modifiers(&ins.declaration.semantic.reg);
            TRACE(" ");
            shader_dump_dst_param(&ins.declaration.semantic.reg, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_CONSTANT_BUFFER)
        {
            TRACE("%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_src_param(&ins.declaration.src, &shader_version);
            TRACE(", %s", ins.flags & WINED3DSI_INDEXED_DYNAMIC ? "dynamicIndexed" : "immediateIndexed");
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PRIMITIVE
                || ins.handler_idx == WINED3DSIH_DCL_OUTPUT_TOPOLOGY)
        {
            TRACE("%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_primitive_type(ins.declaration.primitive_type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_VERTICES_OUT)
        {
            TRACE("%s %u", shader_opcode_names[ins.handler_idx], ins.declaration.count);
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            TRACE("def c%u = %f, %f, %f, %f", shader_get_float_offset(ins.dst[0].reg.type,
                    ins.dst[0].reg.idx[0].offset),
                    *(const float *)&ins.src[0].reg.immconst_data[0],
                    *(const float *)&ins.src[0].reg.immconst_data[1],
                    *(const float *)&ins.src[0].reg.immconst_data[2],
                    *(const float *)&ins.src[0].reg.immconst_data[3]);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            TRACE("defi i%u = %d, %d, %d, %d", ins.dst[0].reg.idx[0].offset,
                    ins.src[0].reg.immconst_data[0],
                    ins.src[0].reg.immconst_data[1],
                    ins.src[0].reg.immconst_data[2],
                    ins.src[0].reg.immconst_data[3]);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            TRACE("defb b%u = %s", ins.dst[0].reg.idx[0].offset, ins.src[0].reg.immconst_data[0] ? "true" : "false");
        }
        else
        {
            if (ins.predicate)
            {
                TRACE("(");
                shader_dump_src_param(ins.predicate, &shader_version);
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

            for (i = 0; i < ins.dst_count; ++i)
            {
                shader_dump_ins_modifiers(&ins.dst[i]);
                TRACE(!i ? " " : ", ");
                shader_dump_dst_param(&ins.dst[i], &shader_version);
            }

            /* Other source tokens */
            for (i = ins.dst_count; i < (ins.dst_count + ins.src_count); ++i)
            {
                TRACE(!i ? " " : ", ");
                shader_dump_src_param(&ins.src[i - ins.dst_count], &shader_version);
            }
        }
        TRACE("\n");
    }
}

#if defined(STAGING_CSMT)
void shader_cleanup(struct wined3d_shader *shader)
#else  /* STAGING_CSMT */
static void shader_cleanup(struct wined3d_shader *shader)
#endif /* STAGING_CSMT */
{
    HeapFree(GetProcessHeap(), 0, shader->output_signature.elements);
    HeapFree(GetProcessHeap(), 0, shader->input_signature.elements);
    HeapFree(GetProcessHeap(), 0, shader->signature_strings);
    shader->device->shader_backend->shader_destroy(shader);
    HeapFree(GetProcessHeap(), 0, shader->reg_maps.constf);
    HeapFree(GetProcessHeap(), 0, shader->reg_maps.sampler_map.entries);
    HeapFree(GetProcessHeap(), 0, shader->function);
    shader_delete_constant_list(&shader->constantsF);
    shader_delete_constant_list(&shader->constantsB);
    shader_delete_constant_list(&shader->constantsI);
    list_remove(&shader->shader_list_entry);

    if (shader->frontend && shader->frontend_data)
        shader->frontend->shader_free(shader->frontend_data);
}

struct shader_none_priv
{
    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct fragment_pipeline *fragment_pipe;
    BOOL ffp_proj_control;
};

static void shader_none_handle_instruction(const struct wined3d_shader_instruction *ins) {}
static void shader_none_select_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info,
        enum wined3d_gl_resource_type tex_type, const SIZE *ds_mask_size) {}
static void shader_none_deselect_depth_blt(void *shader_priv, const struct wined3d_gl_info *gl_info) {}
static void shader_none_update_float_vertex_constants(struct wined3d_device *device, UINT start, UINT count) {}
static void shader_none_update_float_pixel_constants(struct wined3d_device *device, UINT start, UINT count) {}
static void shader_none_load_constants(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state) {}
static void shader_none_destroy(struct wined3d_shader *shader) {}
static void shader_none_free_context_data(struct wined3d_context *context) {}
static void shader_none_init_context_state(struct wined3d_context *context) {}

/* Context activation is done by the caller. */
static void shader_none_select(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct shader_none_priv *priv = shader_priv;

    priv->vertex_pipe->vp_enable(gl_info, !use_vs(state));
    priv->fragment_pipe->enable_extension(gl_info, !use_ps(state));
}

/* Context activation is done by the caller. */
static void shader_none_disable(void *shader_priv, struct wined3d_context *context)
{
    struct shader_none_priv *priv = shader_priv;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    priv->vertex_pipe->vp_enable(gl_info, FALSE);
    priv->fragment_pipe->enable_extension(gl_info, FALSE);

    context->shader_update_mask = (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY);
}

static HRESULT shader_none_alloc(struct wined3d_device *device, const struct wined3d_vertex_pipe_ops *vertex_pipe,
        const struct fragment_pipeline *fragment_pipe)
{
    struct fragment_caps fragment_caps;
    void *vertex_priv, *fragment_priv;
    struct shader_none_priv *priv;

    if (!(priv = HeapAlloc(GetProcessHeap(), 0, sizeof(*priv))))
        return E_OUTOFMEMORY;

    if (!(vertex_priv = vertex_pipe->vp_alloc(&none_shader_backend, priv)))
    {
        ERR("Failed to initialize vertex pipe.\n");
        HeapFree(GetProcessHeap(), 0, priv);
        return E_FAIL;
    }

    if (!(fragment_priv = fragment_pipe->alloc_private(&none_shader_backend, priv)))
    {
        ERR("Failed to initialize fragment pipe.\n");
        vertex_pipe->vp_free(device);
        HeapFree(GetProcessHeap(), 0, priv);
        return E_FAIL;
    }

    priv->vertex_pipe = vertex_pipe;
    priv->fragment_pipe = fragment_pipe;
    fragment_pipe->get_caps(&device->adapter->gl_info, &fragment_caps);
    priv->ffp_proj_control = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_PROJ_CONTROL;

    device->vertex_priv = vertex_priv;
    device->fragment_priv = fragment_priv;
    device->shader_priv = priv;

    return WINED3D_OK;
}

static void shader_none_free(struct wined3d_device *device)
{
    struct shader_none_priv *priv = device->shader_priv;

    priv->fragment_pipe->free_private(device);
    priv->vertex_pipe->vp_free(device);
    HeapFree(GetProcessHeap(), 0, priv);
}

static BOOL shader_none_allocate_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void shader_none_get_caps(const struct wined3d_gl_info *gl_info, struct shader_caps *caps)
{
    /* Set the shader caps to 0 for the none shader backend */
    caps->vs_version = 0;
    caps->gs_version = 0;
    caps->ps_version = 0;
    caps->vs_uniform_count = 0;
    caps->ps_uniform_count = 0;
    caps->ps_1x_max_value = 0.0f;
    caps->varying_count = 0;
    caps->wined3d_caps = 0;
}

static BOOL shader_none_color_fixup_supported(struct color_fixup_desc fixup)
{
    /* We "support" every possible fixup, since we don't support any shader
     * model, and will never have to actually sample a texture. */
    return TRUE;
}

static BOOL shader_none_has_ffp_proj_control(void *shader_priv)
{
    struct shader_none_priv *priv = shader_priv;

    return priv->ffp_proj_control;
}

const struct wined3d_shader_backend_ops none_shader_backend =
{
    shader_none_handle_instruction,
    shader_none_select,
    shader_none_disable,
    shader_none_select_depth_blt,
    shader_none_deselect_depth_blt,
    shader_none_update_float_vertex_constants,
    shader_none_update_float_pixel_constants,
    shader_none_load_constants,
    shader_none_destroy,
    shader_none_alloc,
    shader_none_free,
    shader_none_allocate_context_data,
    shader_none_free_context_data,
    shader_none_init_context_state,
    shader_none_get_caps,
    shader_none_color_fixup_supported,
    shader_none_has_ffp_proj_control,
};

static HRESULT shader_set_function(struct wined3d_shader *shader, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, DWORD float_const_count,
        enum wined3d_shader_type type, unsigned int max_version)
{
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_shader_frontend *fe;
    HRESULT hr;
    unsigned int backend_version;
    const struct wined3d_d3d_info *d3d_info = &shader->device->adapter->d3d_info;

    TRACE("shader %p, byte_code %p, output_signature %p, float_const_count %u, type %#x, max_version %u.\n",
            shader, byte_code, output_signature, float_const_count, type, max_version);

    list_init(&shader->constantsF);
    list_init(&shader->constantsB);
    list_init(&shader->constantsI);
    shader->lconst_inf_or_nan = FALSE;

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

    /* Second pass: figure out which registers are used, what the semantics are, etc. */
    if (FAILED(hr = shader_get_registers_used(shader, fe, reg_maps, &shader->input_signature,
            &shader->output_signature, byte_code, float_const_count)))
        return hr;

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
            backend_version = d3d_info->limits.vs_version;
            break;
        case WINED3D_SHADER_TYPE_GEOMETRY:
            backend_version = d3d_info->limits.gs_version;
            break;
        case WINED3D_SHADER_TYPE_PIXEL:
            backend_version = d3d_info->limits.ps_version;
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

ULONG CDECL wined3d_shader_decref(struct wined3d_shader *shader)
{
    ULONG refcount = InterlockedDecrement(&shader->ref);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
#if defined(STAGING_CSMT)
        const struct wined3d_device *device = shader->device;

        shader->parent_ops->wined3d_object_destroyed(shader->parent);
        wined3d_cs_emit_shader_cleanup(device->cs, shader);
#else  /* STAGING_CSMT */
        shader_cleanup(shader);
        shader->parent_ops->wined3d_object_destroyed(shader->parent);
        HeapFree(GetProcessHeap(), 0, shader);
#endif /* STAGING_CSMT */
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

    if (end_idx > shader->limits->constant_float)
    {
        WARN("end_idx %u > float constants limit %u.\n",
                end_idx, shader->limits->constant_float);
        end_idx = shader->limits->constant_float;
    }

    for (i = start_idx; i < end_idx; ++i)
    {
        struct wined3d_shader_lconst *lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(*lconst));
        float *value;
        if (!lconst)
            return E_OUTOFMEMORY;

        lconst->idx = i;
        value = (float *)lconst->value;
        memcpy(value, src_data + (i - start_idx) * 4 /* 4 components */, 4 * sizeof(float));
        list_add_head(&shader->constantsF, &lconst->entry);

        if (isinf(value[0]) || isnan(value[0]) || isinf(value[1]) || isnan(value[1])
                || isinf(value[2]) || isnan(value[2]) || isinf(value[3]) || isnan(value[3]))
        {
            shader->lconst_inf_or_nan = TRUE;
        }
    }

    return WINED3D_OK;
}

void find_vs_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        WORD swizzle_map, struct vs_compile_args *args, const struct wined3d_d3d_info *d3d_info)
{
    args->fog_src = state->render_states[WINED3D_RS_FOGTABLEMODE]
            == WINED3D_FOG_NONE ? VS_FOG_COORD : VS_FOG_Z;
    args->clip_enabled = state->render_states[WINED3D_RS_CLIPPING]
            && state->render_states[WINED3D_RS_CLIPPLANEENABLE];
    args->point_size = state->gl_primitive_type == GL_POINTS;
    args->per_vertex_point_size = shader->reg_maps.point_size;
    args->swizzle_map = swizzle_map;
    if (d3d_info->emulated_flatshading)
        args->flatshading = state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT;
    else
        args->flatshading = FALSE;
}

static BOOL match_usage(BYTE usage1, BYTE usage_idx1, BYTE usage2, BYTE usage_idx2)
{
    if (usage_idx1 != usage_idx2)
        return FALSE;
    if (usage1 == usage2)
        return TRUE;
    if (usage1 == WINED3D_DECL_USAGE_POSITION && usage2 == WINED3D_DECL_USAGE_POSITIONT)
        return TRUE;
    if (usage2 == WINED3D_DECL_USAGE_POSITION && usage1 == WINED3D_DECL_USAGE_POSITIONT)
        return TRUE;

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

static HRESULT shader_signature_copy(struct wined3d_shader_signature *dst,
        const struct wined3d_shader_signature *src, char **signature_strings)
{
    struct wined3d_shader_signature_element *e;
    unsigned int i;
    SIZE_T len;
    char *ptr;

    ptr = *signature_strings;

    dst->element_count = src->element_count;
    if (!(dst->elements = HeapAlloc(GetProcessHeap(), 0, sizeof(*dst->elements) * dst->element_count)))
        return E_OUTOFMEMORY;

    for (i = 0; i < src->element_count; ++i)
    {
        e = &src->elements[i];
        dst->elements[i] = *e;

        len = strlen(e->semantic_name);
        memcpy(ptr, e->semantic_name, len + 1);
        dst->elements[i].semantic_name = ptr;
        ptr += len + 1;
    }

    *signature_strings = ptr;

    return WINED3D_OK;
}

static HRESULT shader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, DWORD float_const_count, enum wined3d_shader_type type,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    struct wined3d_shader_signature_element *e;
    SIZE_T total, len;
    unsigned int i;
    HRESULT hr;
    char *ptr;

    if (!desc->byte_code)
        return WINED3DERR_INVALIDCALL;

    shader->ref = 1;
    shader->device = device;
    shader->parent = parent;
    shader->parent_ops = parent_ops;

    total = 0;
    if (desc->input_signature)
    {
        for (i = 0; i < desc->input_signature->element_count; ++i)
        {
            e = &desc->input_signature->elements[i];
            len = strlen(e->semantic_name);
            if (len >= ~(SIZE_T)0 - total)
                return E_OUTOFMEMORY;

            total += len + 1;
        }
    }
    if (desc->output_signature)
    {
        for (i = 0; i < desc->output_signature->element_count; ++i)
        {
            e = &desc->output_signature->elements[i];
            len = strlen(e->semantic_name);
            if (len >= ~(SIZE_T)0 - total)
                return E_OUTOFMEMORY;

            total += len + 1;
        }
    }
    if (!(shader->signature_strings = HeapAlloc(GetProcessHeap(), 0, total)))
        return E_OUTOFMEMORY;
    ptr = shader->signature_strings;

    if (desc->input_signature && FAILED(hr = shader_signature_copy(&shader->input_signature,
            desc->input_signature, &ptr)))
    {
        HeapFree(GetProcessHeap(), 0, shader->signature_strings);
        return hr;
    }
    if (desc->output_signature && FAILED(hr = shader_signature_copy(&shader->output_signature,
            desc->output_signature, &ptr)))
    {
        HeapFree(GetProcessHeap(), 0, shader->input_signature.elements);
        HeapFree(GetProcessHeap(), 0, shader->signature_strings);
        return hr;
    }

    list_init(&shader->linked_programs);
    list_add_head(&device->shaders, &shader->shader_list_entry);

    if (FAILED(hr = shader_set_function(shader, desc->byte_code, desc->output_signature,
            float_const_count, type, desc->max_version)))
    {
        WARN("Failed to set function, hr %#x.\n", hr);
        shader_cleanup(shader);
    }

    return hr;
}

static HRESULT vertexshader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    unsigned int i;
    HRESULT hr;

    if (FAILED(hr = shader_init(shader, device, desc, device->adapter->d3d_info.limits.vs_uniform_count,
            WINED3D_SHADER_TYPE_VERTEX, parent, parent_ops)))
        return hr;

    for (i = 0; i < shader->input_signature.element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &shader->input_signature.elements[i];

        if (!(reg_maps->input_registers & (1u << input->register_idx)) || !input->semantic_name)
            continue;

        shader->u.vs.attributes[input->register_idx].usage =
                shader_usage_from_semantic_name(input->semantic_name);
        shader->u.vs.attributes[input->register_idx].usage_idx = input->semantic_idx;
    }

    shader->load_local_constsF = (reg_maps->usesrelconstF && !list_empty(&shader->constantsF)) ||
            shader->lconst_inf_or_nan;

    return WINED3D_OK;
}

static HRESULT geometryshader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    if (FAILED(hr = shader_init(shader, device, desc, 0,
            WINED3D_SHADER_TYPE_GEOMETRY, parent, parent_ops)))
        return hr;

    shader->load_local_constsF = shader->lconst_inf_or_nan;

    return WINED3D_OK;
}

void find_ps_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        BOOL position_transformed, struct ps_compile_args *args, const struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    const struct wined3d_texture *texture;
    UINT i;

    memset(args, 0, sizeof(*args)); /* FIXME: Make sure all bits are set. */
    if (!gl_info->supported[ARB_FRAMEBUFFER_SRGB] && state->render_states[WINED3D_RS_SRGBWRITEENABLE])
    {
#if defined(STAGING_CSMT)
        unsigned int rt_fmt_flags = state->fb.render_targets[0]->format_flags;
#else  /* STAGING_CSMT */
        unsigned int rt_fmt_flags = state->fb->render_targets[0]->format_flags;
#endif /* STAGING_CSMT */
        if (rt_fmt_flags & WINED3DFMT_FLAG_SRGB_WRITE)
        {
            static unsigned int warned = 0;

            args->srgb_correction = 1;
            if (state->render_states[WINED3D_RS_ALPHABLENDENABLE] && !warned++)
                WARN("Blending into a sRGB render target with no GL_ARB_framebuffer_sRGB "
                        "support, expect rendering artifacts.\n");
        }
    }

    if (shader->reg_maps.shader_version.major == 1
            && shader->reg_maps.shader_version.minor <= 3)
    {
        for (i = 0; i < shader->limits->sampler; ++i)
        {
            DWORD flags = state->texture_states[i][WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS];

            if (flags & WINED3D_TTFF_PROJECTED)
            {
                DWORD tex_transform = flags & ~WINED3D_TTFF_PROJECTED;

                if (!state->shader[WINED3D_SHADER_TYPE_VERTEX])
                {
                    enum wined3d_shader_resource_type resource_type = shader->reg_maps.resource_info[i].type;
                    unsigned int j;
                    unsigned int index = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];
                    DWORD max_valid = WINED3D_TTFF_COUNT4;

                    for (j = 0; j < state->vertex_declaration->element_count; ++j)
                    {
                        struct wined3d_vertex_declaration_element *element =
                                &state->vertex_declaration->elements[j];

                        if (element->usage == WINED3D_DECL_USAGE_TEXCOORD
                                && element->usage_idx == index)
                        {
                            max_valid = element->format->component_count;
                            break;
                        }
                    }
                    if (!tex_transform || tex_transform > max_valid)
                    {
                        WARN("Fixing up projected texture transform flags from %#x to %#x.\n",
                                tex_transform, max_valid);
                        tex_transform = max_valid;
                    }
                    if ((resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_1D && tex_transform > WINED3D_TTFF_COUNT1)
                            || (resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2D
                            && tex_transform > WINED3D_TTFF_COUNT2)
                            || (resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_3D
                            && tex_transform > WINED3D_TTFF_COUNT3))
                        tex_transform |= WINED3D_PSARGS_PROJECTED;
                    else
                    {
                        WARN("Application requested projected texture with unsuitable texture coordinates.\n");
                        WARN("(texture unit %u, transform flags %#x, sampler type %u).\n",
                                i, tex_transform, resource_type);
                    }
                }
                else
                    tex_transform = WINED3D_TTFF_COUNT4 | WINED3D_PSARGS_PROJECTED;

                args->tex_transform |= tex_transform << i * WINED3D_PSARGS_TEXTRANSFORM_SHIFT;
            }
        }
    }
    if (shader->reg_maps.shader_version.major == 1
            && shader->reg_maps.shader_version.minor <= 4)
    {
        for (i = 0; i < shader->limits->sampler; ++i)
        {
            const struct wined3d_texture *texture = state->textures[i];

            if (!shader->reg_maps.resource_info[i].type)
                continue;

            /* Treat unbound textures as 2D. The dummy texture will provide
             * the proper sample value. The tex_types bitmap defaults to
             * 2D because of the memset. */
            if (!texture)
                continue;

            switch (texture->target)
            {
                /* RECT textures are distinguished from 2D textures via np2_fixup */
                case GL_TEXTURE_RECTANGLE_ARB:
                case GL_TEXTURE_2D:
                    break;

                case GL_TEXTURE_3D:
                    args->tex_types |= WINED3D_SHADER_TEX_3D << i * WINED3D_PSARGS_TEXTYPE_SHIFT;
                    break;

                case GL_TEXTURE_CUBE_MAP_ARB:
                    args->tex_types |= WINED3D_SHADER_TEX_CUBE << i * WINED3D_PSARGS_TEXTYPE_SHIFT;
                    break;
            }
        }
    }

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
    {
        if (!shader->reg_maps.resource_info[i].type)
            continue;

        texture = state->textures[i];
        if (!texture)
        {
            args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
            continue;
        }
        args->color_fixup[i] = texture->resource.format->color_fixup;

        if (texture->resource.format_flags & WINED3DFMT_FLAG_SHADOW)
            args->shadow |= 1u << i;

        /* Flag samplers that need NP2 texcoord fixup. */
        if (!(texture->flags & WINED3D_TEXTURE_POW2_MAT_IDENT))
            args->np2_fixup |= (1u << i);
    }
    if (shader->reg_maps.shader_version.major >= 3)
    {
        if (position_transformed)
            args->vp_mode = pretransformed;
        else if (use_vs(state))
            args->vp_mode = vertexshader;
        else
            args->vp_mode = fixedfunction;
        args->fog = WINED3D_FFP_PS_FOG_OFF;
    }
    else
    {
        args->vp_mode = vertexshader;
        if (state->render_states[WINED3D_RS_FOGENABLE])
        {
            switch (state->render_states[WINED3D_RS_FOGTABLEMODE])
            {
                case WINED3D_FOG_NONE:
                    if (position_transformed || use_vs(state))
                    {
                        args->fog = WINED3D_FFP_PS_FOG_LINEAR;
                        break;
                    }

                    switch (state->render_states[WINED3D_RS_FOGVERTEXMODE])
                    {
                        case WINED3D_FOG_NONE: /* Fall through. */
                        case WINED3D_FOG_LINEAR: args->fog = WINED3D_FFP_PS_FOG_LINEAR; break;
                        case WINED3D_FOG_EXP:    args->fog = WINED3D_FFP_PS_FOG_EXP;    break;
                        case WINED3D_FOG_EXP2:   args->fog = WINED3D_FFP_PS_FOG_EXP2;   break;
                    }
                    break;

                case WINED3D_FOG_LINEAR: args->fog = WINED3D_FFP_PS_FOG_LINEAR; break;
                case WINED3D_FOG_EXP:    args->fog = WINED3D_FFP_PS_FOG_EXP;    break;
                case WINED3D_FOG_EXP2:   args->fog = WINED3D_FFP_PS_FOG_EXP2;   break;
            }
        }
        else
        {
            args->fog = WINED3D_FFP_PS_FOG_OFF;
        }
    }

    if (context->d3d_info->limits.varying_count < wined3d_max_compat_varyings(context->gl_info))
    {
        const struct wined3d_shader *vs = state->shader[WINED3D_SHADER_TYPE_VERTEX];

        args->texcoords_initialized = 0;
        for (i = 0; i < MAX_TEXTURES; ++i)
        {
            if (vs)
            {
                if (state->shader[WINED3D_SHADER_TYPE_VERTEX]->reg_maps.output_registers & (1u << i))
                    args->texcoords_initialized |= 1u << i;
            }
            else
            {
                const struct wined3d_stream_info *si = &context->stream_info;
                unsigned int coord_idx = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];

                if ((state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX] >> WINED3D_FFP_TCI_SHIFT)
                        & WINED3D_FFP_TCI_MASK
                        || (coord_idx < MAX_TEXTURES && (si->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + coord_idx)))))
                    args->texcoords_initialized |= 1u << i;
            }
        }
    }
    else
    {
        args->texcoords_initialized = (1u << MAX_TEXTURES) - 1;
    }

    args->pointsprite = state->render_states[WINED3D_RS_POINTSPRITEENABLE]
            && state->gl_primitive_type == GL_POINTS;

    if (d3d_info->emulated_flatshading)
        args->flatshading = state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT;
}

static HRESULT pixelshader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i, highest_reg_used = 0, num_regs_used = 0;
    HRESULT hr;

    if (FAILED(hr = shader_init(shader, device, desc, device->adapter->d3d_info.limits.ps_uniform_count,
            WINED3D_SHADER_TYPE_PIXEL, parent, parent_ops)))
        return hr;

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

    shader->load_local_constsF = shader->lconst_inf_or_nan;

    return WINED3D_OK;
}

void pixelshader_update_resource_types(struct wined3d_shader *shader, WORD tex_types)
{
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    struct wined3d_shader_resource_info *resource_info = reg_maps->resource_info;
    unsigned int i;

    if (reg_maps->shader_version.major != 1) return;

    for (i = 0; i < shader->limits->sampler; ++i)
    {
        /* We don't sample from this sampler. */
        if (!resource_info[i].type)
            continue;

        switch ((tex_types >> i * WINED3D_PSARGS_TEXTYPE_SHIFT) & WINED3D_PSARGS_TEXTYPE_MASK)
        {
            case WINED3D_SHADER_TEX_2D:
                resource_info[i].type = WINED3D_SHADER_RESOURCE_TEXTURE_2D;
                break;

            case WINED3D_SHADER_TEX_3D:
                resource_info[i].type = WINED3D_SHADER_RESOURCE_TEXTURE_3D;
                break;

            case WINED3D_SHADER_TEX_CUBE:
                resource_info[i].type = WINED3D_SHADER_RESOURCE_TEXTURE_CUBE;
                break;
        }
    }
}

HRESULT CDECL wined3d_shader_create_gs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, shader %p.\n",
            device, desc, parent, parent_ops, shader);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = geometryshader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize geometry shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created geometry shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_create_ps(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, shader %p.\n",
            device, desc, parent, parent_ops, shader);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = pixelshader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize pixel shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created pixel shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_create_vs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, shader %p.\n",
            device, desc, parent, parent_ops, shader);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = vertexshader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize vertex shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}
