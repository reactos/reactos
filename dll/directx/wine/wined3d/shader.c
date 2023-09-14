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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

const struct wined3d_vec4 wined3d_srgb_const[] =
{
    /* pow, mul_high, sub_high, mul_low */
    {0.41666f, 1.055f, 0.055f, 12.92f},
    /* cmp */
    {0.0031308f, 0.0f, 0.0f, 0.0f},
};

static const char * const shader_opcode_names[] =
{
    /* WINED3DSIH_ABS                              */ "abs",
    /* WINED3DSIH_ADD                              */ "add",
    /* WINED3DSIH_AND                              */ "and",
    /* WINED3DSIH_ATOMIC_AND                       */ "atomic_and",
    /* WINED3DSIH_ATOMIC_CMP_STORE                 */ "atomic_cmp_store",
    /* WINED3DSIH_ATOMIC_IADD                      */ "atomic_iadd",
    /* WINED3DSIH_ATOMIC_IMAX                      */ "atomic_imax",
    /* WINED3DSIH_ATOMIC_IMIN                      */ "atomic_imin",
    /* WINED3DSIH_ATOMIC_OR                        */ "atomic_or",
    /* WINED3DSIH_ATOMIC_UMAX                      */ "atomic_umax",
    /* WINED3DSIH_ATOMIC_UMIN                      */ "atomic_umin",
    /* WINED3DSIH_ATOMIC_XOR                       */ "atomic_xor",
    /* WINED3DSIH_BEM                              */ "bem",
    /* WINED3DSIH_BFI                              */ "bfi",
    /* WINED3DSIH_BFREV                            */ "bfrev",
    /* WINED3DSIH_BREAK                            */ "break",
    /* WINED3DSIH_BREAKC                           */ "breakc",
    /* WINED3DSIH_BREAKP                           */ "breakp",
    /* WINED3DSIH_BUFINFO                          */ "bufinfo",
    /* WINED3DSIH_CALL                             */ "call",
    /* WINED3DSIH_CALLNZ                           */ "callnz",
    /* WINED3DSIH_CASE                             */ "case",
    /* WINED3DSIH_CMP                              */ "cmp",
    /* WINED3DSIH_CND                              */ "cnd",
    /* WINED3DSIH_CONTINUE                         */ "continue",
    /* WINED3DSIH_CONTINUEP                        */ "continuec",
    /* WINED3DSIH_COUNTBITS                        */ "countbits",
    /* WINED3DSIH_CRS                              */ "crs",
    /* WINED3DSIH_CUT                              */ "cut",
    /* WINED3DSIH_CUT_STREAM                       */ "cut_stream",
    /* WINED3DSIH_DCL                              */ "dcl",
    /* WINED3DSIH_DCL_CONSTANT_BUFFER              */ "dcl_constantBuffer",
    /* WINED3DSIH_DCL_FUNCTION_BODY                */ "dcl_function_body",
    /* WINED3DSIH_DCL_FUNCTION_TABLE               */ "dcl_function_table",
    /* WINED3DSIH_DCL_GLOBAL_FLAGS                 */ "dcl_globalFlags",
    /* WINED3DSIH_DCL_GS_INSTANCES                 */ "dcl_gs_instances",
    /* WINED3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT */ "dcl_hs_fork_phase_instance_count",
    /* WINED3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT */ "dcl_hs_join_phase_instance_count",
    /* WINED3DSIH_DCL_HS_MAX_TESSFACTOR            */ "dcl_hs_max_tessfactor",
    /* WINED3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER    */ "dcl_immediateConstantBuffer",
    /* WINED3DSIH_DCL_INDEX_RANGE                  */ "dcl_index_range",
    /* WINED3DSIH_DCL_INDEXABLE_TEMP               */ "dcl_indexableTemp",
    /* WINED3DSIH_DCL_INPUT                        */ "dcl_input",
    /* WINED3DSIH_DCL_INPUT_CONTROL_POINT_COUNT    */ "dcl_input_control_point_count",
    /* WINED3DSIH_DCL_INPUT_PRIMITIVE              */ "dcl_inputPrimitive",
    /* WINED3DSIH_DCL_INPUT_PS                     */ "dcl_input_ps",
    /* WINED3DSIH_DCL_INPUT_PS_SGV                 */ "dcl_input_ps_sgv",
    /* WINED3DSIH_DCL_INPUT_PS_SIV                 */ "dcl_input_ps_siv",
    /* WINED3DSIH_DCL_INPUT_SGV                    */ "dcl_input_sgv",
    /* WINED3DSIH_DCL_INPUT_SIV                    */ "dcl_input_siv",
    /* WINED3DSIH_DCL_INTERFACE                    */ "dcl_interface",
    /* WINED3DSIH_DCL_OUTPUT                       */ "dcl_output",
    /* WINED3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT   */ "dcl_output_control_point_count",
    /* WINED3DSIH_DCL_OUTPUT_SIV                   */ "dcl_output_siv",
    /* WINED3DSIH_DCL_OUTPUT_TOPOLOGY              */ "dcl_outputTopology",
    /* WINED3DSIH_DCL_RESOURCE_RAW                 */ "dcl_resource_raw",
    /* WINED3DSIH_DCL_RESOURCE_STRUCTURED          */ "dcl_resource_structured",
    /* WINED3DSIH_DCL_SAMPLER                      */ "dcl_sampler",
    /* WINED3DSIH_DCL_STREAM                       */ "dcl_stream",
    /* WINED3DSIH_DCL_TEMPS                        */ "dcl_temps",
    /* WINED3DSIH_DCL_TESSELLATOR_DOMAIN           */ "dcl_tessellator_domain",
    /* WINED3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE */ "dcl_tessellator_output_primitive",
    /* WINED3DSIH_DCL_TESSELLATOR_PARTITIONING     */ "dcl_tessellator_partitioning",
    /* WINED3DSIH_DCL_TGSM_RAW                     */ "dcl_tgsm_raw",
    /* WINED3DSIH_DCL_TGSM_STRUCTURED              */ "dcl_tgsm_structured",
    /* WINED3DSIH_DCL_THREAD_GROUP                 */ "dcl_thread_group",
    /* WINED3DSIH_DCL_UAV_RAW                      */ "dcl_uav_raw",
    /* WINED3DSIH_DCL_UAV_STRUCTURED               */ "dcl_uav_structured",
    /* WINED3DSIH_DCL_UAV_TYPED                    */ "dcl_uav_typed",
    /* WINED3DSIH_DCL_VERTICES_OUT                 */ "dcl_maxOutputVertexCount",
    /* WINED3DSIH_DEF                              */ "def",
    /* WINED3DSIH_DEFAULT                          */ "default",
    /* WINED3DSIH_DEFB                             */ "defb",
    /* WINED3DSIH_DEFI                             */ "defi",
    /* WINED3DSIH_DIV                              */ "div",
    /* WINED3DSIH_DP2                              */ "dp2",
    /* WINED3DSIH_DP2ADD                           */ "dp2add",
    /* WINED3DSIH_DP3                              */ "dp3",
    /* WINED3DSIH_DP4                              */ "dp4",
    /* WINED3DSIH_DST                              */ "dst",
    /* WINED3DSIH_DSX                              */ "dsx",
    /* WINED3DSIH_DSX_COARSE                       */ "deriv_rtx_coarse",
    /* WINED3DSIH_DSX_FINE                         */ "deriv_rtx_fine",
    /* WINED3DSIH_DSY                              */ "dsy",
    /* WINED3DSIH_DSY_COARSE                       */ "deriv_rty_coarse",
    /* WINED3DSIH_DSY_FINE                         */ "deriv_rty_fine",
    /* WINED3DSIH_ELSE                             */ "else",
    /* WINED3DSIH_EMIT                             */ "emit",
    /* WINED3DSIH_EMIT_STREAM                      */ "emit_stream",
    /* WINED3DSIH_ENDIF                            */ "endif",
    /* WINED3DSIH_ENDLOOP                          */ "endloop",
    /* WINED3DSIH_ENDREP                           */ "endrep",
    /* WINED3DSIH_ENDSWITCH                        */ "endswitch",
    /* WINED3DSIH_EQ                               */ "eq",
    /* WINED3DSIH_EVAL_SAMPLE_INDEX                */ "eval_sample_index",
    /* WINED3DSIH_EXP                              */ "exp",
    /* WINED3DSIH_EXPP                             */ "expp",
    /* WINED3DSIH_F16TOF32                         */ "f16tof32",
    /* WINED3DSIH_F32TOF16                         */ "f32tof16",
    /* WINED3DSIH_FCALL                            */ "fcall",
    /* WINED3DSIH_FIRSTBIT_HI                      */ "firstbit_hi",
    /* WINED3DSIH_FIRSTBIT_LO                      */ "firstbit_lo",
    /* WINED3DSIH_FIRSTBIT_SHI                     */ "firstbit_shi",
    /* WINED3DSIH_FRC                              */ "frc",
    /* WINED3DSIH_FTOI                             */ "ftoi",
    /* WINED3DSIH_FTOU                             */ "ftou",
    /* WINED3DSIH_GATHER4                          */ "gather4",
    /* WINED3DSIH_GATHER4_C                        */ "gather4_c",
    /* WINED3DSIH_GATHER4_PO                       */ "gather4_po",
    /* WINED3DSIH_GATHER4_PO_C                     */ "gather4_po_c",
    /* WINED3DSIH_GE                               */ "ge",
    /* WINED3DSIH_HS_CONTROL_POINT_PHASE           */ "hs_control_point_phase",
    /* WINED3DSIH_HS_DECLS                         */ "hs_decls",
    /* WINED3DSIH_HS_FORK_PHASE                    */ "hs_fork_phase",
    /* WINED3DSIH_HS_JOIN_PHASE                    */ "hs_join_phase",
    /* WINED3DSIH_IADD                             */ "iadd",
    /* WINED3DSIH_IBFE                             */ "ibfe",
    /* WINED3DSIH_IEQ                              */ "ieq",
    /* WINED3DSIH_IF                               */ "if",
    /* WINED3DSIH_IFC                              */ "ifc",
    /* WINED3DSIH_IGE                              */ "ige",
    /* WINED3DSIH_ILT                              */ "ilt",
    /* WINED3DSIH_IMAD                             */ "imad",
    /* WINED3DSIH_IMAX                             */ "imax",
    /* WINED3DSIH_IMIN                             */ "imin",
    /* WINED3DSIH_IMM_ATOMIC_ALLOC                 */ "imm_atomic_alloc",
    /* WINED3DSIH_IMM_ATOMIC_AND                   */ "imm_atomic_and",
    /* WINED3DSIH_IMM_ATOMIC_CMP_EXCH              */ "imm_atomic_cmp_exch",
    /* WINED3DSIH_IMM_ATOMIC_CONSUME               */ "imm_atomic_consume",
    /* WINED3DSIH_IMM_ATOMIC_EXCH                  */ "imm_atomic_exch",
    /* WINED3DSIH_IMM_ATOMIC_IADD                  */ "imm_atomic_iadd",
    /* WINED3DSIH_IMM_ATOMIC_IMAX                  */ "imm_atomic_imax",
    /* WINED3DSIH_IMM_ATOMIC_IMIN                  */ "imm_atomic_imin",
    /* WINED3DSIH_IMM_ATOMIC_OR                    */ "imm_atomic_or",
    /* WINED3DSIH_IMM_ATOMIC_UMAX                  */ "imm_atomic_umax",
    /* WINED3DSIH_IMM_ATOMIC_UMIN                  */ "imm_atomic_umin",
    /* WINED3DSIH_IMM_ATOMIC_XOR                   */ "imm_atomic_xor",
    /* WINED3DSIH_IMUL                             */ "imul",
    /* WINED3DSIH_INE                              */ "ine",
    /* WINED3DSIH_INEG                             */ "ineg",
    /* WINED3DSIH_ISHL                             */ "ishl",
    /* WINED3DSIH_ISHR                             */ "ishr",
    /* WINED3DSIH_ITOF                             */ "itof",
    /* WINED3DSIH_LABEL                            */ "label",
    /* WINED3DSIH_LD                               */ "ld",
    /* WINED3DSIH_LD2DMS                           */ "ld2dms",
    /* WINED3DSIH_LD_RAW                           */ "ld_raw",
    /* WINED3DSIH_LD_STRUCTURED                    */ "ld_structured",
    /* WINED3DSIH_LD_UAV_TYPED                     */ "ld_uav_typed",
    /* WINED3DSIH_LIT                              */ "lit",
    /* WINED3DSIH_LOD                              */ "lod",
    /* WINED3DSIH_LOG                              */ "log",
    /* WINED3DSIH_LOGP                             */ "logp",
    /* WINED3DSIH_LOOP                             */ "loop",
    /* WINED3DSIH_LRP                              */ "lrp",
    /* WINED3DSIH_LT                               */ "lt",
    /* WINED3DSIH_M3x2                             */ "m3x2",
    /* WINED3DSIH_M3x3                             */ "m3x3",
    /* WINED3DSIH_M3x4                             */ "m3x4",
    /* WINED3DSIH_M4x3                             */ "m4x3",
    /* WINED3DSIH_M4x4                             */ "m4x4",
    /* WINED3DSIH_MAD                              */ "mad",
    /* WINED3DSIH_MAX                              */ "max",
    /* WINED3DSIH_MIN                              */ "min",
    /* WINED3DSIH_MOV                              */ "mov",
    /* WINED3DSIH_MOVA                             */ "mova",
    /* WINED3DSIH_MOVC                             */ "movc",
    /* WINED3DSIH_MUL                              */ "mul",
    /* WINED3DSIH_NE                               */ "ne",
    /* WINED3DSIH_NOP                              */ "nop",
    /* WINED3DSIH_NOT                              */ "not",
    /* WINED3DSIH_NRM                              */ "nrm",
    /* WINED3DSIH_OR                               */ "or",
    /* WINED3DSIH_PHASE                            */ "phase",
    /* WINED3DSIH_POW                              */ "pow",
    /* WINED3DSIH_RCP                              */ "rcp",
    /* WINED3DSIH_REP                              */ "rep",
    /* WINED3DSIH_RESINFO                          */ "resinfo",
    /* WINED3DSIH_RET                              */ "ret",
    /* WINED3DSIH_RETP                             */ "retp",
    /* WINED3DSIH_ROUND_NE                         */ "round_ne",
    /* WINED3DSIH_ROUND_NI                         */ "round_ni",
    /* WINED3DSIH_ROUND_PI                         */ "round_pi",
    /* WINED3DSIH_ROUND_Z                          */ "round_z",
    /* WINED3DSIH_RSQ                              */ "rsq",
    /* WINED3DSIH_SAMPLE                           */ "sample",
    /* WINED3DSIH_SAMPLE_B                         */ "sample_b",
    /* WINED3DSIH_SAMPLE_C                         */ "sample_c",
    /* WINED3DSIH_SAMPLE_C_LZ                      */ "sample_c_lz",
    /* WINED3DSIH_SAMPLE_GRAD                      */ "sample_d",
    /* WINED3DSIH_SAMPLE_INFO                      */ "sample_info",
    /* WINED3DSIH_SAMPLE_LOD                       */ "sample_l",
    /* WINED3DSIH_SAMPLE_POS                       */ "sample_pos",
    /* WINED3DSIH_SETP                             */ "setp",
    /* WINED3DSIH_SGE                              */ "sge",
    /* WINED3DSIH_SGN                              */ "sgn",
    /* WINED3DSIH_SINCOS                           */ "sincos",
    /* WINED3DSIH_SLT                              */ "slt",
    /* WINED3DSIH_SQRT                             */ "sqrt",
    /* WINED3DSIH_STORE_RAW                        */ "store_raw",
    /* WINED3DSIH_STORE_STRUCTURED                 */ "store_structured",
    /* WINED3DSIH_STORE_UAV_TYPED                  */ "store_uav_typed",
    /* WINED3DSIH_SUB                              */ "sub",
    /* WINED3DSIH_SWAPC                            */ "swapc",
    /* WINED3DSIH_SWITCH                           */ "switch",
    /* WINED3DSIH_SYNC                             */ "sync",
    /* WINED3DSIH_TEX                              */ "texld",
    /* WINED3DSIH_TEXBEM                           */ "texbem",
    /* WINED3DSIH_TEXBEML                          */ "texbeml",
    /* WINED3DSIH_TEXCOORD                         */ "texcrd",
    /* WINED3DSIH_TEXDEPTH                         */ "texdepth",
    /* WINED3DSIH_TEXDP3                           */ "texdp3",
    /* WINED3DSIH_TEXDP3TEX                        */ "texdp3tex",
    /* WINED3DSIH_TEXKILL                          */ "texkill",
    /* WINED3DSIH_TEXLDD                           */ "texldd",
    /* WINED3DSIH_TEXLDL                           */ "texldl",
    /* WINED3DSIH_TEXM3x2DEPTH                     */ "texm3x2depth",
    /* WINED3DSIH_TEXM3x2PAD                       */ "texm3x2pad",
    /* WINED3DSIH_TEXM3x2TEX                       */ "texm3x2tex",
    /* WINED3DSIH_TEXM3x3                          */ "texm3x3",
    /* WINED3DSIH_TEXM3x3DIFF                      */ "texm3x3diff",
    /* WINED3DSIH_TEXM3x3PAD                       */ "texm3x3pad",
    /* WINED3DSIH_TEXM3x3SPEC                      */ "texm3x3spec",
    /* WINED3DSIH_TEXM3x3TEX                       */ "texm3x3tex",
    /* WINED3DSIH_TEXM3x3VSPEC                     */ "texm3x3vspec",
    /* WINED3DSIH_TEXREG2AR                        */ "texreg2ar",
    /* WINED3DSIH_TEXREG2GB                        */ "texreg2gb",
    /* WINED3DSIH_TEXREG2RGB                       */ "texreg2rgb",
    /* WINED3DSIH_UBFE                             */ "ubfe",
    /* WINED3DSIH_UDIV                             */ "udiv",
    /* WINED3DSIH_UGE                              */ "uge",
    /* WINED3DSIH_ULT                              */ "ult",
    /* WINED3DSIH_UMAX                             */ "umax",
    /* WINED3DSIH_UMIN                             */ "umin",
    /* WINED3DSIH_UMUL                             */ "umul",
    /* WINED3DSIH_USHR                             */ "ushr",
    /* WINED3DSIH_UTOF                             */ "utof",
    /* WINED3DSIH_XOR                              */ "xor",
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

static const struct
{
    enum wined3d_shader_input_sysval_semantic sysval_semantic;
    const char *sysval_name;
}
shader_input_sysval_semantic_names[] =
{
    {WINED3D_SIV_POSITION,                   "position"},
    {WINED3D_SIV_CLIP_DISTANCE,              "clip_distance"},
    {WINED3D_SIV_CULL_DISTANCE,              "cull_distance"},
    {WINED3D_SIV_RENDER_TARGET_ARRAY_INDEX,  "render_target_array_index"},
    {WINED3D_SIV_VIEWPORT_ARRAY_INDEX,       "viewport_array_index"},
    {WINED3D_SIV_VERTEX_ID,                  "vertex_id"},
    {WINED3D_SIV_INSTANCE_ID,                "instance_id"},
    {WINED3D_SIV_PRIMITIVE_ID,               "primitive_id"},
    {WINED3D_SIV_IS_FRONT_FACE,              "is_front_face"},
    {WINED3D_SIV_SAMPLE_INDEX,               "sample_index"},
    {WINED3D_SIV_QUAD_U0_TESS_FACTOR,        "finalQuadUeq0EdgeTessFactor"},
    {WINED3D_SIV_QUAD_V0_TESS_FACTOR,        "finalQuadVeq0EdgeTessFactor"},
    {WINED3D_SIV_QUAD_U1_TESS_FACTOR,        "finalQuadUeq1EdgeTessFactor"},
    {WINED3D_SIV_QUAD_V1_TESS_FACTOR,        "finalQuadVeq1EdgeTessFactor"},
    {WINED3D_SIV_QUAD_U_INNER_TESS_FACTOR,   "finalQuadUInsideTessFactor"},
    {WINED3D_SIV_QUAD_V_INNER_TESS_FACTOR,   "finalQuadVInsideTessFactor"},
    {WINED3D_SIV_TRIANGLE_U_TESS_FACTOR,     "finalTriUeq0EdgeTessFactor"},
    {WINED3D_SIV_TRIANGLE_V_TESS_FACTOR,     "finalTriVeq0EdgeTessFactor"},
    {WINED3D_SIV_TRIANGLE_W_TESS_FACTOR,     "finalTriWeq0EdgeTessFactor"},
    {WINED3D_SIV_TRIANGLE_INNER_TESS_FACTOR, "finalTriInsideTessFactor"},
    {WINED3D_SIV_LINE_DETAIL_TESS_FACTOR,    "finalLineDetailTessFactor"},
    {WINED3D_SIV_LINE_DENSITY_TESS_FACTOR,   "finalLineDensityTessFactor"},
};

static void shader_dump_src_param(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_src_param *param, const struct wined3d_shader_version *shader_version);

const char *debug_d3dshaderinstructionhandler(enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx)
{
    if (handler_idx >= ARRAY_SIZE(shader_opcode_names))
        return wine_dbg_sprintf("UNRECOGNIZED(%#x)", handler_idx);

    return shader_opcode_names[handler_idx];
}

static const char *shader_semantic_name_from_usage(enum wined3d_decl_usage usage)
{
    if (usage >= ARRAY_SIZE(semantic_names))
    {
        FIXME("Unrecognized usage %#x.\n", usage);
        return "UNRECOGNIZED";
    }

    return semantic_names[usage];
}

static enum wined3d_decl_usage shader_usage_from_semantic_name(const char *name)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(semantic_names); ++i)
    {
        if (!strcmp(name, semantic_names[i]))
            return i;
    }

    return ~0U;
}

static enum wined3d_sysval_semantic shader_sysval_semantic_from_usage(enum wined3d_decl_usage usage)
{
    switch (usage)
    {
        case WINED3D_DECL_USAGE_POSITION:
            return WINED3D_SV_POSITION;
        default:
            return 0;
    }
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
    e->stream_idx = 0;
    e->sysval_semantic = shader_sysval_semantic_from_usage(s->usage);
    e->component_type = WINED3D_TYPE_FLOAT;
    e->register_idx = s->reg.reg.idx[0].offset;
    e->mask = s->reg.write_mask;
}

static void shader_signature_from_usage(struct wined3d_shader_signature_element *e,
        enum wined3d_decl_usage usage, UINT usage_idx, UINT reg_idx, DWORD write_mask)
{
    e->semantic_name = shader_semantic_name_from_usage(usage);
    e->semantic_idx = usage_idx;
    e->stream_idx = 0;
    e->sysval_semantic = shader_sysval_semantic_from_usage(usage);
    e->component_type = WINED3D_TYPE_FLOAT;
    e->register_idx = reg_idx;
    e->mask = write_mask;
}

static const struct wined3d_shader_frontend *shader_select_frontend(enum wined3d_shader_byte_code_format format)
{
    switch (format)
    {
        case WINED3D_SHADER_BYTE_CODE_FORMAT_SM1:
            return &sm1_shader_frontend;

        case WINED3D_SHADER_BYTE_CODE_FORMAT_SM4:
            return &sm4_shader_frontend;

        default:
            WARN("Invalid byte code format %#x specified.\n", format);
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
    if (!(buffer->buffer = heap_alloc(buffer->buffer_size)))
    {
        ERR("Failed to allocate shader buffer memory.\n");
        return FALSE;
    }

    string_buffer_clear(buffer);
    return TRUE;
}

void string_buffer_free(struct wined3d_string_buffer *buffer)
{
    heap_free(buffer->buffer);
}

BOOL string_buffer_resize(struct wined3d_string_buffer *buffer, int rc)
{
    char *new_buffer;
    unsigned int new_buffer_size = buffer->buffer_size * 2;

    while (rc > 0 && (unsigned int)rc >= new_buffer_size - buffer->content_size)
        new_buffer_size *= 2;
    if (!(new_buffer = heap_realloc(buffer->buffer, new_buffer_size)))
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
        buffer = heap_alloc(sizeof(*buffer));
        if (!buffer || !string_buffer_init(buffer))
        {
            ERR("Couldn't allocate buffer for temporary string.\n");
            heap_free(buffer);
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
        heap_free(buffer);
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
        heap_free(constant);
    list_init(clist);
}

static void shader_set_limits(struct wined3d_shader *shader, BOOL swvp)
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
        {WINED3D_SHADER_VERSION(4, 1), WINED3D_SHADER_VERSION(5, 0), {16,  0,   0,  0, 32,  0}},
        {0}
    },
    vs_limits_swvp[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packed_input */
        {WINED3D_SHADER_VERSION(1, 0), WINED3D_SHADER_VERSION(1, 1),   { 0,  0, 8192,  0, 12,  0}},
        {WINED3D_SHADER_VERSION(2, 0), WINED3D_SHADER_VERSION(2, 255), { 0, 16, 8192, 16, 12,  0}},
        /* DX10 cards on Windows advertise a D3D9 constant limit of 256
         * even though they are capable of supporting much more (GL
         * drivers advertise 1024). d3d9.dll and d3d8.dll clamp the
         * wined3d-advertised maximum. Clamp the constant limit for <= 3.0
         * shaders to 256. */
        {WINED3D_SHADER_VERSION(3, 0), WINED3D_SHADER_VERSION(3, 255), { 4, 16, 8192, 16, 12,  0}},
        {0}
    },
    hs_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packet_input */
        {WINED3D_SHADER_VERSION(5, 0), WINED3D_SHADER_VERSION(5, 0), {16,  0,   0,  0, 32, 32}},
    },
    ds_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packet_input */
        {WINED3D_SHADER_VERSION(5, 0), WINED3D_SHADER_VERSION(5, 0), {16,  0,   0,  0, 32, 32}},
    },
    gs_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packed_input */
        {WINED3D_SHADER_VERSION(4, 0), WINED3D_SHADER_VERSION(4, 0), {16,  0,   0,  0, 32, 16}},
        {WINED3D_SHADER_VERSION(4, 1), WINED3D_SHADER_VERSION(5, 0), {16,  0,   0,  0, 32, 32}},
        {0}
    },
    ps_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packed_input */
        {WINED3D_SHADER_VERSION(1, 0), WINED3D_SHADER_VERSION(1, 3), { 4,  0,   8,  0,  0,  0}},
        {WINED3D_SHADER_VERSION(1, 4), WINED3D_SHADER_VERSION(1, 4), { 6,  0,   8,  0,  0,  0}},
        {WINED3D_SHADER_VERSION(2, 0), WINED3D_SHADER_VERSION(2, 0), {16,  0,  32,  0,  0,  0}},
        {WINED3D_SHADER_VERSION(2, 1), WINED3D_SHADER_VERSION(2, 1), {16, 16,  32, 16,  0,  0}},
        {WINED3D_SHADER_VERSION(3, 0), WINED3D_SHADER_VERSION(3, 0), {16, 16, 224, 16,  0, 10}},
        {WINED3D_SHADER_VERSION(4, 0), WINED3D_SHADER_VERSION(5, 0), {16,  0,   0,  0,  0, 32}},
        {0}
    },
    cs_limits[] =
    {
        /* min_version, max_version, sampler, constant_int, constant_float, constant_bool, packed_output, packed_input */
        {WINED3D_SHADER_VERSION(5, 0), WINED3D_SHADER_VERSION(5, 0), {16,  0,   0,  0,  0,  0}},
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
            limits_array = swvp ? vs_limits_swvp : vs_limits;
            break;
        case WINED3D_SHADER_TYPE_HULL:
            limits_array = hs_limits;
            break;
        case WINED3D_SHADER_TYPE_DOMAIN:
            limits_array = ds_limits;
            break;
        case WINED3D_SHADER_TYPE_GEOMETRY:
            limits_array = gs_limits;
            break;
        case WINED3D_SHADER_TYPE_PIXEL:
            limits_array = ps_limits;
            break;
        case WINED3D_SHADER_TYPE_COMPUTE:
            limits_array = cs_limits;
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
            if (reg->idx[0].rel_addr)
                reg_maps->input_rel_addressing = 1;
            if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
            {
                /* If relative addressing is used, we must assume that all
                 * registers are used. Even if it is a construct like v3[aL],
                 * we can't assume that v0, v1 and v2 aren't read because aL
                 * can be negative. */
                if (reg->idx[0].rel_addr)
                    shader->u.ps.input_reg_used = ~0u;
                else
                    shader->u.ps.input_reg_used |= 1u << reg->idx[0].offset;
            }
            else
            {
                reg_maps->input_registers |= 1u << reg->idx[0].offset;
            }
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
                    if (reg->idx[0].offset >= reg_maps->constant_float_count)
                        reg_maps->constant_float_count = reg->idx[0].offset + 1;
                    wined3d_insert_bits(reg_maps->constf, reg->idx[0].offset, 1, 0x1);
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

        case WINED3DSPR_OUTCONTROLPOINT:
            reg_maps->vocp = 1;
            break;

        case WINED3DSPR_SAMPLEMASK:
            reg_maps->sample_mask = 1;
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
        if (!(entries = heap_calloc(4, sizeof(*entries))))
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
                || !(entries = heap_realloc(entries, sizeof(*entries) * new_size)))
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

static HRESULT shader_reg_maps_add_tgsm(struct wined3d_shader_reg_maps *reg_maps,
        unsigned int register_idx, unsigned int size, unsigned int stride)
{
    struct wined3d_shader_tgsm *tgsm;

    if (register_idx >= MAX_TGSM_REGISTERS)
    {
        ERR("Invalid TGSM register index %u.\n", register_idx);
        return S_OK;
    }
    if (reg_maps->shader_version.type != WINED3D_SHADER_TYPE_COMPUTE)
    {
        FIXME("TGSM declarations are allowed only in compute shaders.\n");
        return S_OK;
    }

    if (!wined3d_array_reserve((void **)&reg_maps->tgsm, &reg_maps->tgsm_capacity,
            register_idx + 1, sizeof(*reg_maps->tgsm)))
        return E_OUTOFMEMORY;

    reg_maps->tgsm_count = max(register_idx + 1, reg_maps->tgsm_count);
    tgsm = &reg_maps->tgsm[register_idx];
    tgsm->size = size;
    tgsm->stride = stride;
    return S_OK;
}

static HRESULT shader_record_shader_phase(struct wined3d_shader *shader,
        struct wined3d_shader_phase **current_phase, const struct wined3d_shader_instruction *ins,
        const DWORD *current_instruction_ptr, const DWORD *previous_instruction_ptr)
{
    struct wined3d_shader_phase *phase;

    if ((phase = *current_phase))
    {
        phase->end = previous_instruction_ptr;
        *current_phase = NULL;
    }

    if (shader->reg_maps.shader_version.type != WINED3D_SHADER_TYPE_HULL)
    {
        ERR("Unexpected shader type %s.\n", debug_shader_type(shader->reg_maps.shader_version.type));
        return E_FAIL;
    }

    switch (ins->handler_idx)
    {
        case WINED3DSIH_HS_CONTROL_POINT_PHASE:
            if (shader->u.hs.phases.control_point)
            {
                FIXME("Multiple control point phases.\n");
                heap_free(shader->u.hs.phases.control_point);
            }
            if (!(shader->u.hs.phases.control_point = heap_alloc_zero(sizeof(*shader->u.hs.phases.control_point))))
                return E_OUTOFMEMORY;
            phase = shader->u.hs.phases.control_point;
            break;
        case WINED3DSIH_HS_FORK_PHASE:
            if (!wined3d_array_reserve((void **)&shader->u.hs.phases.fork,
                    &shader->u.hs.phases.fork_size, shader->u.hs.phases.fork_count + 1,
                    sizeof(*shader->u.hs.phases.fork)))
                return E_OUTOFMEMORY;
            phase = &shader->u.hs.phases.fork[shader->u.hs.phases.fork_count++];
            break;
        case WINED3DSIH_HS_JOIN_PHASE:
            if (!wined3d_array_reserve((void **)&shader->u.hs.phases.join,
                    &shader->u.hs.phases.join_size, shader->u.hs.phases.join_count + 1,
                    sizeof(*shader->u.hs.phases.join)))
                return E_OUTOFMEMORY;
            phase = &shader->u.hs.phases.join[shader->u.hs.phases.join_count++];
            break;
        default:
            ERR("Unexpected opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
            return E_FAIL;
    }

    phase->start = current_instruction_ptr;
    *current_phase = phase;

    return WINED3D_OK;
}

static HRESULT shader_calculate_clip_or_cull_distance_mask(
        const struct wined3d_shader_signature_element *e, unsigned int *mask)
{
    /* Clip and cull distances are packed in 4 component registers. 0 and 1 are
     * the only allowed semantic indices.
     */
    if (e->semantic_idx >= WINED3D_MAX_CLIP_DISTANCES / 4)
    {
        *mask = 0;
        WARN("Invalid clip/cull distance index %u.\n", e->semantic_idx);
        return WINED3DERR_INVALIDCALL;
    }

    *mask = (e->mask & WINED3DSP_WRITEMASK_ALL) << (4 * e->semantic_idx);
    return WINED3D_OK;
}

static void wined3d_insert_interpolation_mode(DWORD *packed_interpolation_mode,
        unsigned int register_idx, enum wined3d_shader_interpolation_mode mode)
{
    if (mode > WINED3DSIM_LINEAR_NOPERSPECTIVE_SAMPLE)
        FIXME("Unexpected interpolation mode %#x.\n", mode);

    wined3d_insert_bits(packed_interpolation_mode,
            register_idx * WINED3D_PACKED_INTERPOLATION_BIT_COUNT, WINED3D_PACKED_INTERPOLATION_BIT_COUNT, mode);
}

static HRESULT shader_scan_output_signature(struct wined3d_shader *shader)
{
    const struct wined3d_shader_signature *output_signature = &shader->output_signature;
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < output_signature->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *e = &output_signature->elements[i];
        unsigned int mask;

        reg_maps->output_registers |= 1u << e->register_idx;
        if (e->sysval_semantic == WINED3D_SV_CLIP_DISTANCE)
        {
            if (FAILED(hr = shader_calculate_clip_or_cull_distance_mask(e, &mask)))
                return hr;
            reg_maps->clip_distance_mask |= mask;
        }
        else if (e->sysval_semantic == WINED3D_SV_CULL_DISTANCE)
        {
            if (FAILED(hr = shader_calculate_clip_or_cull_distance_mask(e, &mask)))
                return hr;
            reg_maps->cull_distance_mask |= mask;
        }
        else if (e->sysval_semantic == WINED3D_SV_VIEWPORT_ARRAY_INDEX)
        {
            reg_maps->viewport_array = 1;
        }
    }

    return WINED3D_OK;
}

/* Note that this does not count the loop register as an address register. */
static HRESULT shader_get_registers_used(struct wined3d_shader *shader, DWORD constf_size, BOOL swvp)
{
    struct wined3d_shader_signature_element input_signature_elements[max(MAX_ATTRIBS, MAX_REG_INPUT)];
    struct wined3d_shader_signature_element output_signature_elements[MAX_REG_OUTPUT];
    struct wined3d_shader_signature *output_signature = &shader->output_signature;
    struct wined3d_shader_signature *input_signature = &shader->input_signature;
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_shader_frontend *fe = shader->frontend;
    unsigned int cur_loop_depth = 0, max_loop_depth = 0;
    struct wined3d_shader_version shader_version;
    struct wined3d_shader_phase *phase = NULL;
    const DWORD *ptr, *prev_ins, *current_ins;
    void *fe_data = shader->frontend_data;
    unsigned int i;
    HRESULT hr;

    memset(reg_maps, 0, sizeof(*reg_maps));
    memset(input_signature_elements, 0, sizeof(input_signature_elements));
    memset(output_signature_elements, 0, sizeof(output_signature_elements));
    reg_maps->min_rel_offset = ~0U;
    list_init(&reg_maps->indexable_temps);

    fe->shader_read_header(fe_data, &ptr, &shader_version);
    prev_ins = current_ins = ptr;
    reg_maps->shader_version = shader_version;

    shader_set_limits(shader, swvp);

    if (!(reg_maps->constf = heap_calloc(((min(shader->limits->constant_float, constf_size) + 31) / 32),
            sizeof(*reg_maps->constf))))
    {
        ERR("Failed to allocate constant map memory.\n");
        return E_OUTOFMEMORY;
    }

    while (!fe->shader_is_end(fe_data, &ptr))
    {
        struct wined3d_shader_instruction ins;

        current_ins = ptr;
        /* Fetch opcode. */
        fe->shader_read_instruction(fe_data, &ptr, &ins);

        /* Unhandled opcode, and its parameters. */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            WARN("Encountered unrecognised or invalid instruction.\n");
            return WINED3DERR_INVALIDCALL;
        }

        /* Handle declarations. */
        if (ins.handler_idx == WINED3DSIH_DCL
                || ins.handler_idx == WINED3DSIH_DCL_UAV_TYPED)
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

                case WINED3DSPR_UAV:
                    if (reg_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
                    {
                        ERR("Invalid UAV resource index %u.\n", reg_idx);
                        break;
                    }
                    reg_maps->uav_resource_info[reg_idx].type = semantic->resource_type;
                    reg_maps->uav_resource_info[reg_idx].data_type = semantic->resource_data_type;
                    if (ins.flags)
                        FIXME("Ignoring typed UAV flags %#x.\n", ins.flags);
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
        else if (ins.handler_idx == WINED3DSIH_DCL_GLOBAL_FLAGS)
        {
            if (ins.flags & WINED3DSGF_FORCE_EARLY_DEPTH_STENCIL)
            {
                if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
                    shader->u.ps.force_early_depth_stencil = TRUE;
                else
                    FIXME("Invalid instruction %#x for shader type %#x.\n",
                            ins.handler_idx, shader_version.type);
            }
            else
            {
                WARN("Ignoring global flags %#x.\n", ins.flags);
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_GS_INSTANCES)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_GEOMETRY)
                shader->u.gs.instance_count = ins.declaration.count;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT
                || ins.handler_idx == WINED3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT)
        {
            if (phase)
                phase->instance_count = ins.declaration.count;
            else
                FIXME("Instruction %s outside of shader phase.\n",
                        debug_d3dshaderinstructionhandler(ins.handler_idx));
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER)
        {
            if (reg_maps->icb)
                FIXME("Multiple immediate constant buffers.\n");
            reg_maps->icb = ins.declaration.icb;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INDEXABLE_TEMP)
        {
            if (phase)
            {
                FIXME("Indexable temporary registers not supported.\n");
            }
            else
            {
                struct wined3d_shader_indexable_temp *reg;

                if (!(reg = heap_alloc(sizeof(*reg))))
                    return E_OUTOFMEMORY;

                *reg = ins.declaration.indexable_temp;
                list_add_tail(&reg_maps->indexable_temps, &reg->entry);
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PRIMITIVE)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_GEOMETRY)
                shader->u.gs.input_type = ins.declaration.primitive_type.type;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PS)
        {
            unsigned int reg_idx = ins.declaration.dst.reg.idx[0].offset;
            if (reg_idx >= MAX_REG_INPUT)
            {
                ERR("Invalid register index %u.\n", reg_idx);
                break;
            }
            if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
                wined3d_insert_interpolation_mode(shader->u.ps.interpolation_mode, reg_idx, ins.flags);
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_OUTPUT)
        {
            if (ins.declaration.dst.reg.type == WINED3DSPR_DEPTHOUT
                    || ins.declaration.dst.reg.type == WINED3DSPR_DEPTHOUTGE
                    || ins.declaration.dst.reg.type == WINED3DSPR_DEPTHOUTLE)
            {
                if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
                    shader->u.ps.depth_output = ins.declaration.dst.reg.type;
                else
                    FIXME("Invalid instruction %#x for shader type %#x.\n",
                            ins.handler_idx, shader_version.type);
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_HULL)
                shader->u.hs.output_vertex_count = ins.declaration.count;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n", ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_OUTPUT_TOPOLOGY)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_GEOMETRY)
                shader->u.gs.output_type = ins.declaration.primitive_type.type;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_RESOURCE_RAW)
        {
            unsigned int reg_idx = ins.declaration.dst.reg.idx[0].offset;
            if (reg_idx >= ARRAY_SIZE(reg_maps->resource_info))
            {
                ERR("Invalid resource index %u.\n", reg_idx);
                break;
            }
            reg_maps->resource_info[reg_idx].type = WINED3D_SHADER_RESOURCE_BUFFER;
            reg_maps->resource_info[reg_idx].data_type = WINED3D_DATA_UINT;
            reg_maps->resource_info[reg_idx].flags = WINED3D_VIEW_BUFFER_RAW;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_RESOURCE_STRUCTURED)
        {
            unsigned int reg_idx = ins.declaration.structured_resource.reg.reg.idx[0].offset;
            if (reg_idx >= ARRAY_SIZE(reg_maps->resource_info))
            {
                ERR("Invalid resource index %u.\n", reg_idx);
                break;
            }
            reg_maps->resource_info[reg_idx].type = WINED3D_SHADER_RESOURCE_BUFFER;
            reg_maps->resource_info[reg_idx].data_type = WINED3D_DATA_UINT;
            reg_maps->resource_info[reg_idx].flags = 0;
            reg_maps->resource_info[reg_idx].stride = ins.declaration.structured_resource.byte_stride / 4;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_SAMPLER)
        {
            if (ins.flags & WINED3DSI_SAMPLER_COMPARISON_MODE)
                reg_maps->sampler_comparison_mode |= (1u << ins.declaration.dst.reg.idx[0].offset);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TEMPS)
        {
            if (phase)
                phase->temporary_count = ins.declaration.count;
            else
                reg_maps->temporary_count = ins.declaration.count;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TESSELLATOR_DOMAIN)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_DOMAIN)
                shader->u.ds.tessellator_domain = ins.declaration.tessellator_domain;
            else if (shader_version.type != WINED3D_SHADER_TYPE_HULL)
                FIXME("Invalid instruction %#x for shader type %#x.\n", ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_HULL)
                shader->u.hs.tessellator_output_primitive = ins.declaration.tessellator_output_primitive;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n", ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TESSELLATOR_PARTITIONING)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_HULL)
                shader->u.hs.tessellator_partitioning = ins.declaration.tessellator_partitioning;
            else
                FIXME("Invalid instruction %#x for shader type %#x.\n", ins.handler_idx, shader_version.type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TGSM_RAW)
        {
            if (FAILED(hr = shader_reg_maps_add_tgsm(reg_maps, ins.declaration.tgsm_raw.reg.reg.idx[0].offset,
                    ins.declaration.tgsm_raw.byte_count / 4, 0)))
                return hr;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TGSM_STRUCTURED)
        {
            unsigned int stride = ins.declaration.tgsm_structured.byte_stride / 4;
            unsigned int size = stride * ins.declaration.tgsm_structured.structure_count;
            if (FAILED(hr = shader_reg_maps_add_tgsm(reg_maps,
                    ins.declaration.tgsm_structured.reg.reg.idx[0].offset, size, stride)))
                return hr;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_THREAD_GROUP)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_COMPUTE)
            {
                shader->u.cs.thread_group_size = ins.declaration.thread_group_size;
            }
            else
            {
                FIXME("Invalid instruction %#x for shader type %#x.\n",
                        ins.handler_idx, shader_version.type);
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_UAV_RAW)
        {
            unsigned int reg_idx = ins.declaration.dst.reg.idx[0].offset;
            if (reg_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
            {
                ERR("Invalid UAV resource index %u.\n", reg_idx);
                break;
            }
            if (ins.flags)
                FIXME("Ignoring raw UAV flags %#x.\n", ins.flags);
            reg_maps->uav_resource_info[reg_idx].type = WINED3D_SHADER_RESOURCE_BUFFER;
            reg_maps->uav_resource_info[reg_idx].data_type = WINED3D_DATA_UINT;
            reg_maps->uav_resource_info[reg_idx].flags = WINED3D_VIEW_BUFFER_RAW;
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_UAV_STRUCTURED)
        {
            unsigned int reg_idx = ins.declaration.structured_resource.reg.reg.idx[0].offset;
            if (reg_idx >= ARRAY_SIZE(reg_maps->uav_resource_info))
            {
                ERR("Invalid UAV resource index %u.\n", reg_idx);
                break;
            }
            if (ins.flags)
                FIXME("Ignoring structured UAV flags %#x.\n", ins.flags);
            reg_maps->uav_resource_info[reg_idx].type = WINED3D_SHADER_RESOURCE_BUFFER;
            reg_maps->uav_resource_info[reg_idx].data_type = WINED3D_DATA_UINT;
            reg_maps->uav_resource_info[reg_idx].flags = 0;
            reg_maps->uav_resource_info[reg_idx].stride = ins.declaration.structured_resource.byte_stride / 4;
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
            struct wined3d_shader_lconst *lconst;
            float *value;

            if (!(lconst = heap_alloc(sizeof(*lconst))))
                return E_OUTOFMEMORY;

            lconst->idx = ins.dst[0].reg.idx[0].offset;
            memcpy(lconst->value, ins.src[0].reg.u.immconst_data, 4 * sizeof(DWORD));
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
            struct wined3d_shader_lconst *lconst;

            if (!(lconst = heap_alloc(sizeof(*lconst))))
                return E_OUTOFMEMORY;

            lconst->idx = ins.dst[0].reg.idx[0].offset;
            memcpy(lconst->value, ins.src[0].reg.u.immconst_data, 4 * sizeof(DWORD));

            list_add_head(&shader->constantsI, &lconst->entry);
            reg_maps->local_int_consts |= (1u << lconst->idx);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            struct wined3d_shader_lconst *lconst;

            if (!(lconst = heap_alloc(sizeof(*lconst))))
                return E_OUTOFMEMORY;

            lconst->idx = ins.dst[0].reg.idx[0].offset;
            memcpy(lconst->value, ins.src[0].reg.u.immconst_data, sizeof(DWORD));

            list_add_head(&shader->constantsB, &lconst->entry);
            reg_maps->local_bool_consts |= (1u << lconst->idx);
        }
        /* Handle shader phases. */
        else if (ins.handler_idx == WINED3DSIH_HS_CONTROL_POINT_PHASE
                || ins.handler_idx == WINED3DSIH_HS_FORK_PHASE
                || ins.handler_idx == WINED3DSIH_HS_JOIN_PHASE)
        {
            if (FAILED(hr = shader_record_shader_phase(shader, &phase, &ins, current_ins, prev_ins)))
                return hr;
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

                        case WINED3DSPR_TEXCRDOUT: /* WINED3DSPR_OUTPUT */
                            if (shader_version.major >= 3)
                            {
                                if (idx >= ARRAY_SIZE(reg_maps->u.output_registers_mask))
                                {
                                    WARN("Invalid output register index %u.\n", idx);
                                    break;
                                }
                                reg_maps->u.output_registers_mask[idx] |= ins.dst[i].write_mask;
                                break;
                            }
                            if (idx >= ARRAY_SIZE(reg_maps->u.texcoord_mask))
                            {
                                WARN("Invalid texcoord index %u.\n", idx);
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

                    if (reg_idx >= ARRAY_SIZE(reg_maps->resource_info))
                    {
                        WARN("Invalid 1.x sampler index %u.\n", reg_idx);
                        continue;
                    }

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

            if (ins.handler_idx == WINED3DSIH_IMM_ATOMIC_ALLOC || ins.handler_idx == WINED3DSIH_IMM_ATOMIC_CONSUME)
            {
                unsigned int reg_idx = ins.src[0].reg.idx[0].offset;
                if (reg_idx >= MAX_UNORDERED_ACCESS_VIEWS)
                {
                    ERR("Invalid UAV index %u.\n", reg_idx);
                    break;
                }
                reg_maps->uav_counter_mask |= (1u << reg_idx);
            }
            else if ((WINED3DSIH_ATOMIC_AND <= ins.handler_idx && ins.handler_idx <= WINED3DSIH_ATOMIC_XOR)
                    || (WINED3DSIH_IMM_ATOMIC_AND <= ins.handler_idx && ins.handler_idx <= WINED3DSIH_IMM_ATOMIC_XOR)
                    || (ins.handler_idx == WINED3DSIH_BUFINFO && ins.src[0].reg.type == WINED3DSPR_UAV)
                    || ins.handler_idx == WINED3DSIH_LD_UAV_TYPED
                    || (ins.handler_idx == WINED3DSIH_LD_RAW && ins.src[1].reg.type == WINED3DSPR_UAV)
                    || (ins.handler_idx == WINED3DSIH_LD_STRUCTURED && ins.src[2].reg.type == WINED3DSPR_UAV))
            {
                unsigned int reg_idx;
                if (ins.handler_idx == WINED3DSIH_LD_UAV_TYPED || ins.handler_idx == WINED3DSIH_LD_RAW)
                    reg_idx = ins.src[1].reg.idx[0].offset;
                else if (ins.handler_idx == WINED3DSIH_LD_STRUCTURED)
                    reg_idx = ins.src[2].reg.idx[0].offset;
                else if (WINED3DSIH_ATOMIC_AND <= ins.handler_idx && ins.handler_idx <= WINED3DSIH_ATOMIC_XOR)
                    reg_idx = ins.dst[0].reg.idx[0].offset;
                else if (ins.handler_idx == WINED3DSIH_BUFINFO)
                    reg_idx = ins.src[0].reg.idx[0].offset;
                else
                    reg_idx = ins.dst[1].reg.idx[0].offset;
                if (reg_idx >= MAX_UNORDERED_ACCESS_VIEWS)
                {
                    ERR("Invalid UAV index %u.\n", reg_idx);
                    break;
                }
                reg_maps->uav_read_mask |= (1u << reg_idx);
            }
            else if (ins.handler_idx == WINED3DSIH_NRM)
            {
                reg_maps->usesnrm = 1;
            }
            else if (ins.handler_idx == WINED3DSIH_DSY
                    || ins.handler_idx == WINED3DSIH_DSY_COARSE
                    || ins.handler_idx == WINED3DSIH_DSY_FINE)
            {
                reg_maps->usesdsy = 1;
            }
            else if (ins.handler_idx == WINED3DSIH_DSX
                    || ins.handler_idx == WINED3DSIH_DSX_COARSE
                    || ins.handler_idx == WINED3DSIH_DSX_FINE)
            {
                reg_maps->usesdsx = 1;
            }
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
            else if (ins.handler_idx == WINED3DSIH_GATHER4
                    || ins.handler_idx == WINED3DSIH_GATHER4_C
                    || ins.handler_idx == WINED3DSIH_SAMPLE
                    || ins.handler_idx == WINED3DSIH_SAMPLE_B
                    || ins.handler_idx == WINED3DSIH_SAMPLE_C
                    || ins.handler_idx == WINED3DSIH_SAMPLE_C_LZ
                    || ins.handler_idx == WINED3DSIH_SAMPLE_GRAD
                    || ins.handler_idx == WINED3DSIH_SAMPLE_LOD)
            {
                shader_record_sample(reg_maps, ins.src[1].reg.idx[0].offset,
                        ins.src[2].reg.idx[0].offset, reg_maps->sampler_map.count);
            }
            else if (ins.handler_idx == WINED3DSIH_GATHER4_PO
                    || ins.handler_idx == WINED3DSIH_GATHER4_PO_C)
            {
                shader_record_sample(reg_maps, ins.src[2].reg.idx[0].offset,
                        ins.src[3].reg.idx[0].offset, reg_maps->sampler_map.count);
            }
            else if ((ins.handler_idx == WINED3DSIH_BUFINFO && ins.src[0].reg.type == WINED3DSPR_RESOURCE)
                    || (ins.handler_idx == WINED3DSIH_SAMPLE_INFO && ins.src[0].reg.type == WINED3DSPR_RESOURCE))
            {
                shader_record_sample(reg_maps, ins.src[0].reg.idx[0].offset,
                        WINED3D_SAMPLER_DEFAULT, reg_maps->sampler_map.count);
            }
            else if (ins.handler_idx == WINED3DSIH_LD
                    || ins.handler_idx == WINED3DSIH_LD2DMS
                    || (ins.handler_idx == WINED3DSIH_LD_RAW && ins.src[1].reg.type == WINED3DSPR_RESOURCE)
                    || (ins.handler_idx == WINED3DSIH_RESINFO && ins.src[1].reg.type == WINED3DSPR_RESOURCE))
            {
                shader_record_sample(reg_maps, ins.src[1].reg.idx[0].offset,
                        WINED3D_SAMPLER_DEFAULT, reg_maps->sampler_map.count);
            }
            else if (ins.handler_idx == WINED3DSIH_LD_STRUCTURED
                    && ins.src[2].reg.type == WINED3DSPR_RESOURCE)
            {
                shader_record_sample(reg_maps, ins.src[2].reg.idx[0].offset,
                        WINED3D_SAMPLER_DEFAULT, reg_maps->sampler_map.count);
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

        prev_ins = current_ins;
    }
    reg_maps->loop_depth = max_loop_depth;

    if (phase)
    {
        phase->end = prev_ins;
        phase = NULL;
    }

    /* PS before 2.0 don't have explicit color outputs. Instead the value of
     * R0 is written to the render target. */
    if (shader_version.major < 2 && shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
        reg_maps->rt_mask |= (1u << 0);

    if (input_signature->elements)
    {
        for (i = 0; i < input_signature->element_count; ++i)
        {
            if (shader_version.type == WINED3D_SHADER_TYPE_VERTEX)
            {
                if (input_signature->elements[i].register_idx >= ARRAY_SIZE(shader->u.vs.attributes))
                {
                    WARN("Invalid input signature register index %u.\n", input_signature->elements[i].register_idx);
                    return WINED3DERR_INVALIDCALL;
                }
            }
            else if (shader_version.type == WINED3D_SHADER_TYPE_PIXEL)
            {
                if (input_signature->elements[i].sysval_semantic == WINED3D_SV_POSITION)
                    reg_maps->vpos = 1;
                else if (input_signature->elements[i].sysval_semantic == WINED3D_SV_IS_FRONT_FACE)
                    reg_maps->usesfacing = 1;
            }
            reg_maps->input_registers |= 1u << input_signature->elements[i].register_idx;
        }
    }
    else if (!input_signature->elements && reg_maps->input_registers)
    {
        unsigned int count = wined3d_popcount(reg_maps->input_registers);
        struct wined3d_shader_signature_element *e;
        unsigned int i;

        if (!(input_signature->elements = heap_calloc(count, sizeof(*input_signature->elements))))
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
        if (FAILED(hr = shader_scan_output_signature(shader)))
            return hr;
    }
    else if (reg_maps->output_registers)
    {
        unsigned int count = wined3d_popcount(reg_maps->output_registers);
        struct wined3d_shader_signature_element *e;

        if (!(output_signature->elements = heap_calloc(count, sizeof(*output_signature->elements))))
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

static void shader_cleanup_reg_maps(struct wined3d_shader_reg_maps *reg_maps)
{
    struct wined3d_shader_indexable_temp *reg, *reg_next;

    heap_free(reg_maps->constf);
    heap_free(reg_maps->sampler_map.entries);

    LIST_FOR_EACH_ENTRY_SAFE(reg, reg_next, &reg_maps->indexable_temps, struct wined3d_shader_indexable_temp, entry)
        heap_free(reg);
    list_init(&reg_maps->indexable_temps);

    heap_free(reg_maps->tgsm);
}

unsigned int shader_find_free_input_register(const struct wined3d_shader_reg_maps *reg_maps, unsigned int max)
{
    DWORD map = 1u << max;
    map |= map - 1;
    map &= reg_maps->shader_version.major < 3 ? ~reg_maps->texcoord : ~reg_maps->input_registers;

    return wined3d_log2i(map);
}

static void shader_dump_global_flags(struct wined3d_string_buffer *buffer, DWORD global_flags)
{
    if (global_flags & WINED3DSGF_REFACTORING_ALLOWED)
    {
        shader_addline(buffer, "refactoringAllowed");
        global_flags &= ~WINED3DSGF_REFACTORING_ALLOWED;
        if (global_flags)
            shader_addline(buffer, " | ");
    }

    if (global_flags & WINED3DSGF_FORCE_EARLY_DEPTH_STENCIL)
    {
        shader_addline(buffer, "forceEarlyDepthStencil");
        global_flags &= ~WINED3DSGF_FORCE_EARLY_DEPTH_STENCIL;
        if (global_flags)
            shader_addline(buffer, " | ");
    }

    if (global_flags & WINED3DSGF_ENABLE_RAW_AND_STRUCTURED_BUFFERS)
    {
        shader_addline(buffer, "enableRawAndStructuredBuffers");
        global_flags &= ~WINED3DSGF_ENABLE_RAW_AND_STRUCTURED_BUFFERS;
    }

    if (global_flags)
        shader_addline(buffer, "unknown_flags(%#x)", global_flags);
}

static void shader_dump_sync_flags(struct wined3d_string_buffer *buffer, DWORD sync_flags)
{
    if (sync_flags & WINED3DSSF_GROUP_SHARED_MEMORY)
    {
        shader_addline(buffer, "_g");
        sync_flags &= ~WINED3DSSF_GROUP_SHARED_MEMORY;
    }
    if (sync_flags & WINED3DSSF_THREAD_GROUP)
    {
        shader_addline(buffer, "_t");
        sync_flags &= ~WINED3DSSF_THREAD_GROUP;
    }

    if (sync_flags)
        shader_addline(buffer, "_unknown_flags(%#x)", sync_flags);
}

static void shader_dump_precise_flags(struct wined3d_string_buffer *buffer, DWORD flags)
{
    if (!(flags & WINED3DSI_PRECISE_XYZW))
        return;

    shader_addline(buffer, " [precise");
    if (flags != WINED3DSI_PRECISE_XYZW)
    {
        shader_addline(buffer, "(%s%s%s%s)",
                flags & WINED3DSI_PRECISE_X ? "x" : "",
                flags & WINED3DSI_PRECISE_Y ? "y" : "",
                flags & WINED3DSI_PRECISE_Z ? "z" : "",
                flags & WINED3DSI_PRECISE_W ? "w" : "");
    }
    shader_addline(buffer, "]");
}

static void shader_dump_uav_flags(struct wined3d_string_buffer *buffer, DWORD uav_flags)
{
    if (uav_flags & WINED3DSUF_GLOBALLY_COHERENT)
    {
        shader_addline(buffer, "_glc");
        uav_flags &= ~WINED3DSUF_GLOBALLY_COHERENT;
    }
    if (uav_flags & WINED3DSUF_ORDER_PRESERVING_COUNTER)
    {
        shader_addline(buffer, "_opc");
        uav_flags &= ~WINED3DSUF_ORDER_PRESERVING_COUNTER;
    }

    if (uav_flags)
        shader_addline(buffer, "_unknown_flags(%#x)", uav_flags);
}

static void shader_dump_tessellator_domain(struct wined3d_string_buffer *buffer,
        enum wined3d_tessellator_domain domain)
{
    switch (domain)
    {
        case WINED3D_TESSELLATOR_DOMAIN_LINE:
            shader_addline(buffer, "line");
            break;
        case WINED3D_TESSELLATOR_DOMAIN_TRIANGLE:
            shader_addline(buffer, "triangle");
            break;
        case WINED3D_TESSELLATOR_DOMAIN_QUAD:
            shader_addline(buffer, "quad");
            break;
        default:
            shader_addline(buffer, "unknown_tessellator_domain(%#x)", domain);
            break;
    }
}

static void shader_dump_tessellator_output_primitive(struct wined3d_string_buffer *buffer,
        enum wined3d_tessellator_output_primitive output_primitive)
{
    switch (output_primitive)
    {
        case WINED3D_TESSELLATOR_OUTPUT_POINT:
            shader_addline(buffer, "point");
            break;
        case WINED3D_TESSELLATOR_OUTPUT_LINE:
            shader_addline(buffer, "line");
            break;
        case WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CW:
            shader_addline(buffer, "triangle_cw");
            break;
        case WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CCW:
            shader_addline(buffer, "triangle_ccw");
            break;
        default:
            shader_addline(buffer, "unknown_tessellator_output_primitive(%#x)", output_primitive);
            break;
    }
}

static void shader_dump_tessellator_partitioning(struct wined3d_string_buffer *buffer,
        enum wined3d_tessellator_partitioning partitioning)
{
    switch (partitioning)
    {
        case WINED3D_TESSELLATOR_PARTITIONING_INTEGER:
            shader_addline(buffer, "integer");
            break;
        case WINED3D_TESSELLATOR_PARTITIONING_POW2:
            shader_addline(buffer, "pow2");
            break;
        case WINED3D_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD:
            shader_addline(buffer, "fractional_odd");
            break;
        case WINED3D_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN:
            shader_addline(buffer, "fractional_even");
            break;
        default:
            shader_addline(buffer, "unknown_tessellator_partitioning(%#x)", partitioning);
            break;
    }
}

static void shader_dump_shader_input_sysval_semantic(struct wined3d_string_buffer *buffer,
        enum wined3d_shader_input_sysval_semantic semantic)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(shader_input_sysval_semantic_names); ++i)
    {
        if (shader_input_sysval_semantic_names[i].sysval_semantic == semantic)
        {
            shader_addline(buffer, "%s", shader_input_sysval_semantic_names[i].sysval_name);
            return;
        }
    }

    shader_addline(buffer, "unknown_shader_input_sysval_semantic(%#x)", semantic);
}

static void shader_dump_decl_usage(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_semantic *semantic, unsigned int flags,
        const struct wined3d_shader_version *shader_version)
{
    shader_addline(buffer, "dcl");

    if (semantic->reg.reg.type == WINED3DSPR_SAMPLER)
    {
        switch (semantic->resource_type)
        {
            case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
                shader_addline(buffer, "_2d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
                shader_addline(buffer, "_3d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
                shader_addline(buffer, "_cube");
                break;

            default:
                shader_addline(buffer, "_unknown_resource_type(%#x)", semantic->resource_type);
                break;
        }
    }
    else if (semantic->reg.reg.type == WINED3DSPR_RESOURCE || semantic->reg.reg.type == WINED3DSPR_UAV)
    {
        if (semantic->reg.reg.type == WINED3DSPR_RESOURCE)
            shader_addline(buffer, "_resource_");
        else
            shader_addline(buffer, "_uav_");
        switch (semantic->resource_type)
        {
            case WINED3D_SHADER_RESOURCE_BUFFER:
                shader_addline(buffer, "buffer");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1D:
                shader_addline(buffer, "texture1d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
                shader_addline(buffer, "texture2d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DMS:
                shader_addline(buffer, "texture2dms");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
                shader_addline(buffer, "texture3d");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
                shader_addline(buffer, "texturecube");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY:
                shader_addline(buffer, "texture1darray");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY:
                shader_addline(buffer, "texture2darray");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY:
                shader_addline(buffer, "texture2dmsarray");
                break;

            case WINED3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY:
                shader_addline(buffer, "texturecubearray");
                break;

            default:
                shader_addline(buffer, "unknown");
                break;
        }
        if (semantic->reg.reg.type == WINED3DSPR_UAV)
            shader_dump_uav_flags(buffer, flags);
        switch (semantic->resource_data_type)
        {
            case WINED3D_DATA_FLOAT:
                shader_addline(buffer, " (float)");
                break;

            case WINED3D_DATA_INT:
                shader_addline(buffer, " (int)");
                break;

            case WINED3D_DATA_UINT:
                shader_addline(buffer, " (uint)");
                break;

            case WINED3D_DATA_UNORM:
                shader_addline(buffer, " (unorm)");
                break;

            case WINED3D_DATA_SNORM:
                shader_addline(buffer, " (snorm)");
                break;

            default:
                shader_addline(buffer, " (unknown)");
                break;
        }
    }
    else
    {
        /* Pixel shaders 3.0 don't have usage semantics. */
        if (shader_version->major < 3 && shader_version->type == WINED3D_SHADER_TYPE_PIXEL)
            return;
        else
            shader_addline(buffer, "_");

        switch (semantic->usage)
        {
            case WINED3D_DECL_USAGE_POSITION:
                shader_addline(buffer, "position%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_BLEND_INDICES:
                shader_addline(buffer, "blend");
                break;

            case WINED3D_DECL_USAGE_BLEND_WEIGHT:
                shader_addline(buffer, "weight");
                break;

            case WINED3D_DECL_USAGE_NORMAL:
                shader_addline(buffer, "normal%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_PSIZE:
                shader_addline(buffer, "psize");
                break;

            case WINED3D_DECL_USAGE_COLOR:
                if (!semantic->usage_idx)
                    shader_addline(buffer, "color");
                else
                    shader_addline(buffer, "specular%u", (semantic->usage_idx - 1));
                break;

            case WINED3D_DECL_USAGE_TEXCOORD:
                shader_addline(buffer, "texture%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_TANGENT:
                shader_addline(buffer, "tangent");
                break;

            case WINED3D_DECL_USAGE_BINORMAL:
                shader_addline(buffer, "binormal");
                break;

            case WINED3D_DECL_USAGE_TESS_FACTOR:
                shader_addline(buffer, "tessfactor");
                break;

            case WINED3D_DECL_USAGE_POSITIONT:
                shader_addline(buffer, "positionT%u", semantic->usage_idx);
                break;

            case WINED3D_DECL_USAGE_FOG:
                shader_addline(buffer, "fog");
                break;

            case WINED3D_DECL_USAGE_DEPTH:
                shader_addline(buffer, "depth");
                break;

            case WINED3D_DECL_USAGE_SAMPLE:
                shader_addline(buffer, "sample");
                break;

            default:
                shader_addline(buffer, "<unknown_semantic(%#x)>", semantic->usage);
                FIXME("Unrecognised semantic usage %#x.\n", semantic->usage);
        }
    }
}

static void shader_dump_register(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_register *reg, const struct wined3d_shader_version *shader_version)
{
    static const char * const rastout_reg_names[] = {"oPos", "oFog", "oPts"};
    static const char * const misctype_reg_names[] = {"vPos", "vFace"};
    UINT offset = reg->idx[0].offset;

    switch (reg->type)
    {
        case WINED3DSPR_TEMP:
            shader_addline(buffer, "r");
            break;

        case WINED3DSPR_INPUT:
            shader_addline(buffer, "v");
            break;

        case WINED3DSPR_CONST:
        case WINED3DSPR_CONST2:
        case WINED3DSPR_CONST3:
        case WINED3DSPR_CONST4:
            shader_addline(buffer, "c");
            offset = shader_get_float_offset(reg->type, offset);
            break;

        case WINED3DSPR_TEXTURE: /* vs: case WINED3DSPR_ADDR */
            shader_addline(buffer, "%c", shader_version->type == WINED3D_SHADER_TYPE_PIXEL ? 't' : 'a');
            break;

        case WINED3DSPR_RASTOUT:
            shader_addline(buffer, "%s", rastout_reg_names[offset]);
            break;

        case WINED3DSPR_COLOROUT:
            shader_addline(buffer, "oC");
            break;

        case WINED3DSPR_DEPTHOUT:
            shader_addline(buffer, "oDepth");
            break;

        case WINED3DSPR_DEPTHOUTGE:
            shader_addline(buffer, "oDepthGE");
            break;

        case WINED3DSPR_DEPTHOUTLE:
            shader_addline(buffer, "oDepthLE");
            break;

        case WINED3DSPR_ATTROUT:
            shader_addline(buffer, "oD");
            break;

        case WINED3DSPR_TEXCRDOUT:
            /* Vertex shaders >= 3.0 use general purpose output registers
             * (WINED3DSPR_OUTPUT), which can include an address token. */
            if (shader_version->major >= 3)
                shader_addline(buffer, "o");
            else
                shader_addline(buffer, "oT");
            break;

        case WINED3DSPR_CONSTINT:
            shader_addline(buffer, "i");
            break;

        case WINED3DSPR_CONSTBOOL:
            shader_addline(buffer, "b");
            break;

        case WINED3DSPR_LABEL:
            shader_addline(buffer, "l");
            break;

        case WINED3DSPR_LOOP:
            shader_addline(buffer, "aL");
            break;

        case WINED3DSPR_SAMPLER:
            shader_addline(buffer, "s");
            break;

        case WINED3DSPR_MISCTYPE:
            if (offset > 1)
            {
                FIXME("Unhandled misctype register %u.\n", offset);
                shader_addline(buffer, "<unhandled misctype %#x>", offset);
            }
            else
            {
                shader_addline(buffer, "%s", misctype_reg_names[offset]);
            }
            break;

        case WINED3DSPR_PREDICATE:
            shader_addline(buffer, "p");
            break;

        case WINED3DSPR_IMMCONST:
            shader_addline(buffer, "l");
            break;

        case WINED3DSPR_CONSTBUFFER:
            shader_addline(buffer, "cb");
            break;

        case WINED3DSPR_IMMCONSTBUFFER:
            shader_addline(buffer, "icb");
            break;

        case WINED3DSPR_PRIMID:
            shader_addline(buffer, "primID");
            break;

        case WINED3DSPR_NULL:
            shader_addline(buffer, "null");
            break;

        case WINED3DSPR_RASTERIZER:
            shader_addline(buffer, "rasterizer");
            break;

        case WINED3DSPR_RESOURCE:
            shader_addline(buffer, "t");
            break;

        case WINED3DSPR_UAV:
            shader_addline(buffer, "u");
            break;

        case WINED3DSPR_OUTPOINTID:
            shader_addline(buffer, "vOutputControlPointID");
            break;

        case WINED3DSPR_FORKINSTID:
            shader_addline(buffer, "vForkInstanceId");
            break;

        case WINED3DSPR_JOININSTID:
            shader_addline(buffer, "vJoinInstanceId");
            break;

        case WINED3DSPR_INCONTROLPOINT:
            shader_addline(buffer, "vicp");
            break;

        case WINED3DSPR_OUTCONTROLPOINT:
            shader_addline(buffer, "vocp");
            break;

        case WINED3DSPR_PATCHCONST:
            shader_addline(buffer, "vpc");
            break;

        case WINED3DSPR_TESSCOORD:
            shader_addline(buffer, "vDomainLocation");
            break;

        case WINED3DSPR_GROUPSHAREDMEM:
            shader_addline(buffer, "g");
            break;

        case WINED3DSPR_THREADID:
            shader_addline(buffer, "vThreadID");
            break;

        case WINED3DSPR_THREADGROUPID:
            shader_addline(buffer, "vThreadGroupID");
            break;

        case WINED3DSPR_LOCALTHREADID:
            shader_addline(buffer, "vThreadIDInGroup");
            break;

        case WINED3DSPR_LOCALTHREADINDEX:
            shader_addline(buffer, "vThreadIDInGroupFlattened");
            break;

        case WINED3DSPR_IDXTEMP:
            shader_addline(buffer, "x");
            break;

        case WINED3DSPR_STREAM:
            shader_addline(buffer, "m");
            break;

        case WINED3DSPR_FUNCTIONBODY:
            shader_addline(buffer, "fb");
            break;

        case WINED3DSPR_FUNCTIONPOINTER:
            shader_addline(buffer, "fp");
            break;

        case WINED3DSPR_COVERAGE:
            shader_addline(buffer, "vCoverage");
            break;

        case WINED3DSPR_SAMPLEMASK:
            shader_addline(buffer, "oMask");
            break;

        case WINED3DSPR_GSINSTID:
            shader_addline(buffer, "vGSInstanceID");
            break;

        default:
            shader_addline(buffer, "<unhandled_rtype(%#x)>", reg->type);
            break;
    }

    if (reg->type == WINED3DSPR_IMMCONST)
    {
        shader_addline(buffer, "(");
        switch (reg->immconst_type)
        {
            case WINED3D_IMMCONST_SCALAR:
                switch (reg->data_type)
                {
                    case WINED3D_DATA_FLOAT:
                        shader_addline(buffer, "%.8e", *(const float *)reg->u.immconst_data);
                        break;
                    case WINED3D_DATA_INT:
                        shader_addline(buffer, "%d", reg->u.immconst_data[0]);
                        break;
                    case WINED3D_DATA_RESOURCE:
                    case WINED3D_DATA_SAMPLER:
                    case WINED3D_DATA_UINT:
                        shader_addline(buffer, "%u", reg->u.immconst_data[0]);
                        break;
                    default:
                        shader_addline(buffer, "<unhandled data type %#x>", reg->data_type);
                        break;
                }
                break;

            case WINED3D_IMMCONST_VEC4:
                switch (reg->data_type)
                {
                    case WINED3D_DATA_FLOAT:
                        shader_addline(buffer, "%.8e, %.8e, %.8e, %.8e",
                                *(const float *)&reg->u.immconst_data[0], *(const float *)&reg->u.immconst_data[1],
                                *(const float *)&reg->u.immconst_data[2], *(const float *)&reg->u.immconst_data[3]);
                        break;
                    case WINED3D_DATA_INT:
                        shader_addline(buffer, "%d, %d, %d, %d",
                                reg->u.immconst_data[0], reg->u.immconst_data[1],
                                reg->u.immconst_data[2], reg->u.immconst_data[3]);
                        break;
                    case WINED3D_DATA_RESOURCE:
                    case WINED3D_DATA_SAMPLER:
                    case WINED3D_DATA_UINT:
                        shader_addline(buffer, "%u, %u, %u, %u",
                                reg->u.immconst_data[0], reg->u.immconst_data[1],
                                reg->u.immconst_data[2], reg->u.immconst_data[3]);
                        break;
                    default:
                        shader_addline(buffer, "<unhandled data type %#x>", reg->data_type);
                        break;
                }
                break;

            default:
                shader_addline(buffer, "<unhandled immconst_type %#x>", reg->immconst_type);
                break;
        }
        shader_addline(buffer, ")");
    }
    else if (reg->type != WINED3DSPR_RASTOUT
            && reg->type != WINED3DSPR_MISCTYPE
            && reg->type != WINED3DSPR_NULL)
    {
        if (offset != ~0u)
        {
            shader_addline(buffer, "[");
            if (reg->idx[0].rel_addr)
            {
                shader_dump_src_param(buffer, reg->idx[0].rel_addr, shader_version);
                shader_addline(buffer, " + ");
            }
            shader_addline(buffer, "%u]", offset);

            if (reg->idx[1].offset != ~0u)
            {
                shader_addline(buffer, "[");
                if (reg->idx[1].rel_addr)
                {
                    shader_dump_src_param(buffer, reg->idx[1].rel_addr, shader_version);
                    shader_addline(buffer, " + ");
                }
                shader_addline(buffer, "%u]", reg->idx[1].offset);
            }
        }

        if (reg->type == WINED3DSPR_FUNCTIONPOINTER)
            shader_addline(buffer, "[%u]", reg->u.fp_body_idx);
    }
}

static void shader_dump_dst_param(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_dst_param *param, const struct wined3d_shader_version *shader_version)
{
    DWORD write_mask = param->write_mask;

    shader_dump_register(buffer, &param->reg, shader_version);

    if (write_mask && write_mask != WINED3DSP_WRITEMASK_ALL)
    {
        static const char write_mask_chars[] = "xyzw";

        shader_addline(buffer, ".");
        if (write_mask & WINED3DSP_WRITEMASK_0)
            shader_addline(buffer, "%c", write_mask_chars[0]);
        if (write_mask & WINED3DSP_WRITEMASK_1)
            shader_addline(buffer, "%c", write_mask_chars[1]);
        if (write_mask & WINED3DSP_WRITEMASK_2)
            shader_addline(buffer, "%c", write_mask_chars[2]);
        if (write_mask & WINED3DSP_WRITEMASK_3)
            shader_addline(buffer, "%c", write_mask_chars[3]);
    }
}

static void shader_dump_src_param(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_src_param *param, const struct wined3d_shader_version *shader_version)
{
    enum wined3d_shader_src_modifier src_modifier = param->modifiers;
    DWORD swizzle = param->swizzle;

    if (src_modifier == WINED3DSPSM_NEG
            || src_modifier == WINED3DSPSM_BIASNEG
            || src_modifier == WINED3DSPSM_SIGNNEG
            || src_modifier == WINED3DSPSM_X2NEG
            || src_modifier == WINED3DSPSM_ABSNEG)
        shader_addline(buffer, "-");
    else if (src_modifier == WINED3DSPSM_COMP)
        shader_addline(buffer, "1-");
    else if (src_modifier == WINED3DSPSM_NOT)
        shader_addline(buffer, "!");

    if (src_modifier == WINED3DSPSM_ABS || src_modifier == WINED3DSPSM_ABSNEG)
        shader_addline(buffer, "abs(");

    shader_dump_register(buffer, &param->reg, shader_version);

    switch (src_modifier)
    {
        case WINED3DSPSM_NONE:    break;
        case WINED3DSPSM_NEG:     break;
        case WINED3DSPSM_NOT:     break;
        case WINED3DSPSM_BIAS:    shader_addline(buffer, "_bias"); break;
        case WINED3DSPSM_BIASNEG: shader_addline(buffer, "_bias"); break;
        case WINED3DSPSM_SIGN:    shader_addline(buffer, "_bx2"); break;
        case WINED3DSPSM_SIGNNEG: shader_addline(buffer, "_bx2"); break;
        case WINED3DSPSM_COMP:    break;
        case WINED3DSPSM_X2:      shader_addline(buffer, "_x2"); break;
        case WINED3DSPSM_X2NEG:   shader_addline(buffer, "_x2"); break;
        case WINED3DSPSM_DZ:      shader_addline(buffer, "_dz"); break;
        case WINED3DSPSM_DW:      shader_addline(buffer, "_dw"); break;
        case WINED3DSPSM_ABSNEG:  shader_addline(buffer, ")"); break;
        case WINED3DSPSM_ABS:     shader_addline(buffer, ")"); break;
        default:                  shader_addline(buffer, "_unknown_modifier(%#x)", src_modifier);
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
            shader_addline(buffer, ".%c", swizzle_chars[swizzle_x]);
        }
        else
        {
            shader_addline(buffer, ".%c%c%c%c", swizzle_chars[swizzle_x], swizzle_chars[swizzle_y],
                    swizzle_chars[swizzle_z], swizzle_chars[swizzle_w]);
        }
    }
}

/* Shared code in order to generate the bulk of the shader string. */
HRESULT shader_generate_code(const struct wined3d_shader *shader, struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_reg_maps *reg_maps, void *backend_ctx,
        const DWORD *start, const DWORD *end)
{
    struct wined3d_device *device = shader->device;
    const struct wined3d_shader_frontend *fe = shader->frontend;
    void *fe_data = shader->frontend_data;
    struct wined3d_shader_version shader_version;
    struct wined3d_shader_parser_state state;
    struct wined3d_shader_instruction ins;
    struct wined3d_shader_tex_mx tex_mx;
    struct wined3d_shader_context ctx;
    const DWORD *ptr;

    /* Initialize current parsing state. */
    tex_mx.current_row = 0;
    state.current_loop_depth = 0;
    state.current_loop_reg = 0;
    state.in_subroutine = FALSE;

    ctx.shader = shader;
    ctx.gl_info = &device->adapter->gl_info;
    ctx.reg_maps = reg_maps;
    ctx.buffer = buffer;
    ctx.tex_mx = &tex_mx;
    ctx.state = &state;
    ctx.backend_data = backend_ctx;
    ins.ctx = &ctx;

    fe->shader_read_header(fe_data, &ptr, &shader_version);
    if (start)
        ptr = start;

    while (!fe->shader_is_end(fe_data, &ptr) && ptr != end)
    {
        /* Read opcode. */
        fe->shader_read_instruction(fe_data, &ptr, &ins);

        /* Unknown opcode and its parameters. */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            WARN("Encountered unrecognised or invalid instruction.\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (ins.predicate)
            FIXME("Predicates not implemented.\n");

        /* Call appropriate function for output target */
        device->shader_backend->shader_handle_instruction(&ins);
    }

    return WINED3D_OK;
}

static void shader_dump_ins_modifiers(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_dst_param *dst)
{
    DWORD mmask = dst->modifiers;

    switch (dst->shift)
    {
        case 0: break;
        case 13: shader_addline(buffer, "_d8"); break;
        case 14: shader_addline(buffer, "_d4"); break;
        case 15: shader_addline(buffer, "_d2"); break;
        case 1: shader_addline(buffer, "_x2"); break;
        case 2: shader_addline(buffer, "_x4"); break;
        case 3: shader_addline(buffer, "_x8"); break;
        default: shader_addline(buffer, "_unhandled_shift(%d)", dst->shift); break;
    }

    if (mmask & WINED3DSPDM_SATURATE)         shader_addline(buffer, "_sat");
    if (mmask & WINED3DSPDM_PARTIALPRECISION) shader_addline(buffer, "_pp");
    if (mmask & WINED3DSPDM_MSAMPCENTROID)    shader_addline(buffer, "_centroid");

    mmask &= ~(WINED3DSPDM_SATURATE | WINED3DSPDM_PARTIALPRECISION | WINED3DSPDM_MSAMPCENTROID);
    if (mmask) FIXME("Unrecognised modifier %#x.\n", mmask);
}

static void shader_dump_primitive_type(struct wined3d_string_buffer *buffer,
        const struct wined3d_shader_primitive_type *primitive_type)
{
    switch (primitive_type->type)
    {
        case WINED3D_PT_UNDEFINED:
            shader_addline(buffer, "undefined");
            break;
        case WINED3D_PT_POINTLIST:
            shader_addline(buffer, "pointlist");
            break;
        case WINED3D_PT_LINELIST:
            shader_addline(buffer, "linelist");
            break;
        case WINED3D_PT_LINESTRIP:
            shader_addline(buffer, "linestrip");
            break;
        case WINED3D_PT_TRIANGLELIST:
            shader_addline(buffer, "trianglelist");
            break;
        case WINED3D_PT_TRIANGLESTRIP:
            shader_addline(buffer, "trianglestrip");
            break;
        case WINED3D_PT_TRIANGLEFAN:
            shader_addline(buffer, "trianglefan");
            break;
        case WINED3D_PT_LINELIST_ADJ:
            shader_addline(buffer, "linelist_adj");
            break;
        case WINED3D_PT_LINESTRIP_ADJ:
            shader_addline(buffer, "linestrip_adj");
            break;
        case WINED3D_PT_TRIANGLELIST_ADJ:
            shader_addline(buffer, "trianglelist_adj");
            break;
        case WINED3D_PT_TRIANGLESTRIP_ADJ:
            shader_addline(buffer, "trianglestrip_adj");
            break;
        case WINED3D_PT_PATCH:
            shader_addline(buffer, "patch%u", primitive_type->patch_vertex_count);
            break;
        default:
            shader_addline(buffer, "<unrecognized_primitive_type %#x>", primitive_type->type);
            break;
    }
}

static void shader_dump_interpolation_mode(struct wined3d_string_buffer *buffer,
        enum wined3d_shader_interpolation_mode interpolation_mode)
{
    switch (interpolation_mode)
    {
        case WINED3DSIM_CONSTANT:
            shader_addline(buffer, "constant");
            break;
        case WINED3DSIM_LINEAR:
            shader_addline(buffer, "linear");
            break;
        case WINED3DSIM_LINEAR_CENTROID:
            shader_addline(buffer, "linear centroid");
            break;
        case WINED3DSIM_LINEAR_NOPERSPECTIVE:
            shader_addline(buffer, "linear noperspective");
            break;
        case WINED3DSIM_LINEAR_SAMPLE:
            shader_addline(buffer, "linear sample");
            break;
        case WINED3DSIM_LINEAR_NOPERSPECTIVE_CENTROID:
            shader_addline(buffer, "linear noperspective centroid");
            break;
        case WINED3DSIM_LINEAR_NOPERSPECTIVE_SAMPLE:
            shader_addline(buffer, "linear noperspective sample");
            break;
        default:
            shader_addline(buffer, "<unrecognized_interpolation_mode %#x>", interpolation_mode);
            break;
    }
}

static void shader_trace_init(const struct wined3d_shader_frontend *fe, void *fe_data)
{
    struct wined3d_shader_version shader_version;
    struct wined3d_string_buffer buffer;
    const char *type_prefix;
    const char *p, *q;
    const DWORD *ptr;
    DWORD i;

    if (!string_buffer_init(&buffer))
    {
        ERR("Failed to initialize string buffer.\n");
        return;
    }

    fe->shader_read_header(fe_data, &ptr, &shader_version);

    TRACE("Parsing %p.\n", ptr);

    switch (shader_version.type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            type_prefix = "vs";
            break;

        case WINED3D_SHADER_TYPE_HULL:
            type_prefix = "hs";
            break;

        case WINED3D_SHADER_TYPE_DOMAIN:
            type_prefix = "ds";
            break;

        case WINED3D_SHADER_TYPE_GEOMETRY:
            type_prefix = "gs";
            break;

        case WINED3D_SHADER_TYPE_PIXEL:
            type_prefix = "ps";
            break;

        case WINED3D_SHADER_TYPE_COMPUTE:
            type_prefix = "cs";
            break;

        default:
            FIXME("Unhandled shader type %#x.\n", shader_version.type);
            type_prefix = "unknown";
            break;
    }

    shader_addline(&buffer, "%s_%u_%u\n", type_prefix, shader_version.major, shader_version.minor);

    while (!fe->shader_is_end(fe_data, &ptr))
    {
        struct wined3d_shader_instruction ins;

        fe->shader_read_instruction(fe_data, &ptr, &ins);
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            WARN("Skipping unrecognized instruction.\n");
            shader_addline(&buffer, "<unrecognized instruction>\n");
            continue;
        }

        if (ins.handler_idx == WINED3DSIH_DCL || ins.handler_idx == WINED3DSIH_DCL_UAV_TYPED)
        {
            shader_dump_decl_usage(&buffer, &ins.declaration.semantic, ins.flags, &shader_version);
            shader_dump_ins_modifiers(&buffer, &ins.declaration.semantic.reg);
            shader_addline(&buffer, " ");
            shader_dump_dst_param(&buffer, &ins.declaration.semantic.reg, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_CONSTANT_BUFFER)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_src_param(&buffer, &ins.declaration.src, &shader_version);
            shader_addline(&buffer, ", %s",
                    ins.flags & WINED3DSI_INDEXED_DYNAMIC ? "dynamicIndexed" : "immediateIndexed");
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_FUNCTION_BODY)
        {
            shader_addline(&buffer, "%s fb%u",
                    shader_opcode_names[ins.handler_idx], ins.declaration.index);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_FUNCTION_TABLE)
        {
            shader_addline(&buffer, "%s ft%u = {...}",
                    shader_opcode_names[ins.handler_idx], ins.declaration.index);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_GLOBAL_FLAGS)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_global_flags(&buffer, ins.flags);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_HS_MAX_TESSFACTOR)
        {
            shader_addline(&buffer, "%s %.8e", shader_opcode_names[ins.handler_idx],
                    ins.declaration.max_tessellation_factor);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER)
        {
            shader_addline(&buffer, "%s {\n", shader_opcode_names[ins.handler_idx]);
            for (i = 0; i < ins.declaration.icb->vec4_count; ++i)
            {
                shader_addline(&buffer, "    {0x%08x, 0x%08x, 0x%08x, 0x%08x},\n",
                        ins.declaration.icb->data[4 * i + 0],
                        ins.declaration.icb->data[4 * i + 1],
                        ins.declaration.icb->data[4 * i + 2],
                        ins.declaration.icb->data[4 * i + 3]);
            }
            shader_addline(&buffer, "}");
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INDEX_RANGE)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.index_range.first_register, &shader_version);
            shader_addline(&buffer, " %u", ins.declaration.index_range.last_register);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INDEXABLE_TEMP)
        {
            shader_addline(&buffer, "%s x[%u][%u], %u", shader_opcode_names[ins.handler_idx],
                    ins.declaration.indexable_temp.register_idx,
                    ins.declaration.indexable_temp.register_size,
                    ins.declaration.indexable_temp.component_count);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PS)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_interpolation_mode(&buffer, ins.flags);
            shader_addline(&buffer, " ");
            shader_dump_dst_param(&buffer, &ins.declaration.dst, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PS_SGV
                || ins.handler_idx == WINED3DSIH_DCL_INPUT_SGV
                || ins.handler_idx == WINED3DSIH_DCL_INPUT_SIV
                || ins.handler_idx == WINED3DSIH_DCL_OUTPUT_SIV)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.register_semantic.reg, &shader_version);
            shader_addline(&buffer, ", ");
            shader_dump_shader_input_sysval_semantic(&buffer, ins.declaration.register_semantic.sysval_semantic);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PS_SIV)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_interpolation_mode(&buffer, ins.flags);
            shader_addline(&buffer, " ");
            shader_dump_dst_param(&buffer, &ins.declaration.register_semantic.reg, &shader_version);
            shader_addline(&buffer, ", ");
            shader_dump_shader_input_sysval_semantic(&buffer, ins.declaration.register_semantic.sysval_semantic);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT
                || ins.handler_idx == WINED3DSIH_DCL_OUTPUT)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.dst, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INPUT_PRIMITIVE
                || ins.handler_idx == WINED3DSIH_DCL_OUTPUT_TOPOLOGY)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_primitive_type(&buffer, &ins.declaration.primitive_type);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_INTERFACE)
        {
            shader_addline(&buffer, "%s fp[%u][%u][%u] = {...}",
                    shader_opcode_names[ins.handler_idx], ins.declaration.fp.index,
                    ins.declaration.fp.array_size, ins.declaration.fp.body_count);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_RESOURCE_RAW)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.dst, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_RESOURCE_STRUCTURED)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.structured_resource.reg, &shader_version);
            shader_addline(&buffer, ", %u", ins.declaration.structured_resource.byte_stride);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_SAMPLER)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.dst, &shader_version);
            if (ins.flags == WINED3DSI_SAMPLER_COMPARISON_MODE)
                shader_addline(&buffer, ", comparisonMode");
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TEMPS
                || ins.handler_idx == WINED3DSIH_DCL_GS_INSTANCES
                || ins.handler_idx == WINED3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT
                || ins.handler_idx == WINED3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT
                || ins.handler_idx == WINED3DSIH_DCL_INPUT_CONTROL_POINT_COUNT
                || ins.handler_idx == WINED3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT
                || ins.handler_idx == WINED3DSIH_DCL_VERTICES_OUT)
        {
            shader_addline(&buffer, "%s %u", shader_opcode_names[ins.handler_idx], ins.declaration.count);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TESSELLATOR_DOMAIN)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_tessellator_domain(&buffer, ins.declaration.tessellator_domain);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_tessellator_output_primitive(&buffer, ins.declaration.tessellator_output_primitive);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TESSELLATOR_PARTITIONING)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_tessellator_partitioning(&buffer, ins.declaration.tessellator_partitioning);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TGSM_RAW)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.tgsm_raw.reg, &shader_version);
            shader_addline(&buffer, ", %u", ins.declaration.tgsm_raw.byte_count);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_TGSM_STRUCTURED)
        {
            shader_addline(&buffer, "%s ", shader_opcode_names[ins.handler_idx]);
            shader_dump_dst_param(&buffer, &ins.declaration.tgsm_structured.reg, &shader_version);
            shader_addline(&buffer, ", %u, %u", ins.declaration.tgsm_structured.byte_stride,
                    ins.declaration.tgsm_structured.structure_count);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_THREAD_GROUP)
        {
            shader_addline(&buffer, "%s %u, %u, %u", shader_opcode_names[ins.handler_idx],
                    ins.declaration.thread_group_size.x,
                    ins.declaration.thread_group_size.y,
                    ins.declaration.thread_group_size.z);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_UAV_RAW)
        {
            shader_addline(&buffer, "%s", shader_opcode_names[ins.handler_idx]);
            shader_dump_uav_flags(&buffer, ins.flags);
            shader_addline(&buffer, " ");
            shader_dump_dst_param(&buffer, &ins.declaration.dst, &shader_version);
        }
        else if (ins.handler_idx == WINED3DSIH_DCL_UAV_STRUCTURED)
        {
            shader_addline(&buffer, "%s", shader_opcode_names[ins.handler_idx]);
            shader_dump_uav_flags(&buffer, ins.flags);
            shader_addline(&buffer, " ");
            shader_dump_dst_param(&buffer, &ins.declaration.structured_resource.reg, &shader_version);
            shader_addline(&buffer, ", %u", ins.declaration.structured_resource.byte_stride);
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            shader_addline(&buffer, "def c%u = %.8e, %.8e, %.8e, %.8e", shader_get_float_offset(ins.dst[0].reg.type,
                    ins.dst[0].reg.idx[0].offset),
                    *(const float *)&ins.src[0].reg.u.immconst_data[0],
                    *(const float *)&ins.src[0].reg.u.immconst_data[1],
                    *(const float *)&ins.src[0].reg.u.immconst_data[2],
                    *(const float *)&ins.src[0].reg.u.immconst_data[3]);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            shader_addline(&buffer, "defi i%u = %d, %d, %d, %d", ins.dst[0].reg.idx[0].offset,
                    ins.src[0].reg.u.immconst_data[0],
                    ins.src[0].reg.u.immconst_data[1],
                    ins.src[0].reg.u.immconst_data[2],
                    ins.src[0].reg.u.immconst_data[3]);
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            shader_addline(&buffer, "defb b%u = %s",
                    ins.dst[0].reg.idx[0].offset, ins.src[0].reg.u.immconst_data[0] ? "true" : "false");
        }
        else
        {
            if (ins.predicate)
            {
                shader_addline(&buffer, "(");
                shader_dump_src_param(&buffer, ins.predicate, &shader_version);
                shader_addline(&buffer, ") ");
            }

            /* PixWin marks instructions with the coissue flag with a '+' */
            if (ins.coissue)
                shader_addline(&buffer, "+");

            shader_addline(&buffer, "%s", shader_opcode_names[ins.handler_idx]);

            if (ins.handler_idx == WINED3DSIH_BREAKP
                    || ins.handler_idx == WINED3DSIH_CONTINUEP
                    || ins.handler_idx == WINED3DSIH_IF
                    || ins.handler_idx == WINED3DSIH_RETP
                    || ins.handler_idx == WINED3DSIH_TEXKILL)
            {
                switch (ins.flags)
                {
                    case WINED3D_SHADER_CONDITIONAL_OP_NZ: shader_addline(&buffer, "_nz"); break;
                    case WINED3D_SHADER_CONDITIONAL_OP_Z:  shader_addline(&buffer, "_z"); break;
                    default: shader_addline(&buffer, "_unrecognized(%#x)", ins.flags); break;
                }
            }
            else if (ins.handler_idx == WINED3DSIH_IFC
                    || ins.handler_idx == WINED3DSIH_BREAKC)
            {
                switch (ins.flags)
                {
                    case WINED3D_SHADER_REL_OP_GT: shader_addline(&buffer, "_gt"); break;
                    case WINED3D_SHADER_REL_OP_EQ: shader_addline(&buffer, "_eq"); break;
                    case WINED3D_SHADER_REL_OP_GE: shader_addline(&buffer, "_ge"); break;
                    case WINED3D_SHADER_REL_OP_LT: shader_addline(&buffer, "_lt"); break;
                    case WINED3D_SHADER_REL_OP_NE: shader_addline(&buffer, "_ne"); break;
                    case WINED3D_SHADER_REL_OP_LE: shader_addline(&buffer, "_le"); break;
                    default: shader_addline(&buffer, "_(%u)", ins.flags);
                }
            }
            else if (ins.handler_idx == WINED3DSIH_TEX
                    && shader_version.major >= 2
                    && (ins.flags & WINED3DSI_TEXLD_PROJECT))
            {
                shader_addline(&buffer, "p");
            }
            else if (ins.handler_idx == WINED3DSIH_RESINFO && ins.flags)
            {
                switch (ins.flags)
                {
                    case WINED3DSI_RESINFO_RCP_FLOAT: shader_addline(&buffer, "_rcpFloat"); break;
                    case WINED3DSI_RESINFO_UINT: shader_addline(&buffer, "_uint"); break;
                    default: shader_addline(&buffer, "_unrecognized(%#x)", ins.flags);
                }
            }
            else if (ins.handler_idx == WINED3DSIH_SAMPLE_INFO && ins.flags)
            {
                switch (ins.flags)
                {
                    case WINED3DSI_SAMPLE_INFO_UINT: shader_addline(&buffer, "_uint"); break;
                    default: shader_addline(&buffer, "_unrecognized(%#x)", ins.flags);
                }
            }
            else if (ins.handler_idx == WINED3DSIH_SYNC)
            {
                shader_dump_sync_flags(&buffer, ins.flags);
            }
            else
            {
                shader_dump_precise_flags(&buffer, ins.flags);
            }

            if (wined3d_shader_instruction_has_texel_offset(&ins))
                shader_addline(&buffer, "(%d,%d,%d)", ins.texel_offset.u, ins.texel_offset.v, ins.texel_offset.w);

            for (i = 0; i < ins.dst_count; ++i)
            {
                shader_dump_ins_modifiers(&buffer, &ins.dst[i]);
                shader_addline(&buffer, !i ? " " : ", ");
                shader_dump_dst_param(&buffer, &ins.dst[i], &shader_version);
            }

            /* Other source tokens */
            for (i = ins.dst_count; i < (ins.dst_count + ins.src_count); ++i)
            {
                shader_addline(&buffer, !i ? " " : ", ");
                shader_dump_src_param(&buffer, &ins.src[i - ins.dst_count], &shader_version);
            }
        }
        shader_addline(&buffer, "\n");
    }

    for (p = buffer.buffer; *p; p = q)
    {
        if (!(q = strstr(p, "\n")))
            q = p + strlen(p);
        else
            ++q;
        TRACE("    %.*s", (int)(q - p), p);
    }

    string_buffer_free(&buffer);
}

static void shader_cleanup(struct wined3d_shader *shader)
{
    if (shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_HULL)
    {
        heap_free(shader->u.hs.phases.control_point);
        heap_free(shader->u.hs.phases.fork);
        heap_free(shader->u.hs.phases.join);
    }
    else if (shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_GEOMETRY)
    {
        heap_free((void *)shader->u.gs.so_desc.elements);
    }

    heap_free(shader->patch_constant_signature.elements);
    heap_free(shader->output_signature.elements);
    heap_free(shader->input_signature.elements);
    shader->device->shader_backend->shader_destroy(shader);
    shader_cleanup_reg_maps(&shader->reg_maps);
    heap_free(shader->byte_code);
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
    const struct wined3d_fragment_pipe_ops *fragment_pipe;
    BOOL ffp_proj_control;
};

static void shader_none_handle_instruction(const struct wined3d_shader_instruction *ins) {}
static void shader_none_precompile(void *shader_priv, struct wined3d_shader *shader) {}
static void shader_none_select_compute(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state) {}
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
    struct shader_none_priv *priv = shader_priv;

    priv->vertex_pipe->vp_enable(context, !use_vs(state));
    priv->fragment_pipe->fp_enable(context, !use_ps(state));
}

/* Context activation is done by the caller. */
static void shader_none_disable(void *shader_priv, struct wined3d_context *context)
{
    struct shader_none_priv *priv = shader_priv;

    priv->vertex_pipe->vp_enable(context, FALSE);
    priv->fragment_pipe->fp_enable(context, FALSE);

    context->shader_update_mask = (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY)
            | (1u << WINED3D_SHADER_TYPE_HULL)
            | (1u << WINED3D_SHADER_TYPE_DOMAIN)
            | (1u << WINED3D_SHADER_TYPE_COMPUTE);
}

static HRESULT shader_none_alloc(struct wined3d_device *device, const struct wined3d_vertex_pipe_ops *vertex_pipe,
        const struct wined3d_fragment_pipe_ops *fragment_pipe)
{
    struct fragment_caps fragment_caps;
    void *vertex_priv, *fragment_priv;
    struct shader_none_priv *priv;

    if (!(priv = heap_alloc(sizeof(*priv))))
        return E_OUTOFMEMORY;

    if (!(vertex_priv = vertex_pipe->vp_alloc(&none_shader_backend, priv)))
    {
        ERR("Failed to initialize vertex pipe.\n");
        heap_free(priv);
        return E_FAIL;
    }

    if (!(fragment_priv = fragment_pipe->alloc_private(&none_shader_backend, priv)))
    {
        ERR("Failed to initialize fragment pipe.\n");
        vertex_pipe->vp_free(device, NULL);
        heap_free(priv);
        return E_FAIL;
    }

    priv->vertex_pipe = vertex_pipe;
    priv->fragment_pipe = fragment_pipe;
    fragment_pipe->get_caps(device->adapter, &fragment_caps);
    priv->ffp_proj_control = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_PROJ_CONTROL;

    device->vertex_priv = vertex_priv;
    device->fragment_priv = fragment_priv;
    device->shader_priv = priv;

    return WINED3D_OK;
}

static void shader_none_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct shader_none_priv *priv = device->shader_priv;

    priv->fragment_pipe->free_private(device, context);
    priv->vertex_pipe->vp_free(device, context);
    heap_free(priv);
}

static BOOL shader_none_allocate_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void shader_none_get_caps(const struct wined3d_adapter *adapter, struct shader_caps *caps)
{
    /* Set the shader caps to 0 for the none shader backend */
    memset(caps, 0, sizeof(*caps));
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
    shader_none_precompile,
    shader_none_select,
    shader_none_select_compute,
    shader_none_disable,
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

static unsigned int shader_max_version_from_feature_level(enum wined3d_feature_level level)
{
    switch (level)
    {
        case WINED3D_FEATURE_LEVEL_11_1:
        case WINED3D_FEATURE_LEVEL_11:
            return 5;
        case WINED3D_FEATURE_LEVEL_10_1:
        case WINED3D_FEATURE_LEVEL_10:
            return 4;
        case WINED3D_FEATURE_LEVEL_9_3:
            return 3;
        case WINED3D_FEATURE_LEVEL_9_2:
        case WINED3D_FEATURE_LEVEL_9_1:
            return 2;
        default:
            return 1;
    }
}

static HRESULT shader_set_function(struct wined3d_shader *shader, struct wined3d_device *device,
        enum wined3d_shader_type type, unsigned int float_const_count, BOOL swvp)
{
    const struct wined3d_d3d_info *d3d_info = &shader->device->adapter->d3d_info;
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    const struct wined3d_shader_version *version = &reg_maps->shader_version;
    const struct wined3d_shader_frontend *fe;
    unsigned int backend_version;
    HRESULT hr;

    TRACE("shader %p, device %p, type %s, float_const_count %u.\n",
            shader, device, debug_shader_type(type), float_const_count);

    fe = shader->frontend;
    if (!(shader->frontend_data = fe->shader_init(shader->function,
            shader->functionLength, &shader->output_signature)))
    {
        FIXME("Failed to initialize frontend.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* First pass: trace shader. */
    if (TRACE_ON(d3d_shader))
        shader_trace_init(fe, shader->frontend_data);

    /* Second pass: figure out which registers are used, what the semantics are, etc. */
    if (FAILED(hr = shader_get_registers_used(shader, float_const_count, swvp)))
        return hr;

    if (version->type != type)
    {
        WARN("Wrong shader type %s.\n", debug_shader_type(reg_maps->shader_version.type));
        return WINED3DERR_INVALIDCALL;
    }
    if (version->major > shader_max_version_from_feature_level(device->feature_level))
    {
        WARN("Shader version %u not supported by this device.\n", version->major);
        return WINED3DERR_INVALIDCALL;
    }
    switch (type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            backend_version = d3d_info->limits.vs_version;
            break;
        case WINED3D_SHADER_TYPE_HULL:
            backend_version = d3d_info->limits.hs_version;
            break;
        case WINED3D_SHADER_TYPE_DOMAIN:
            backend_version = d3d_info->limits.ds_version;
            break;
        case WINED3D_SHADER_TYPE_GEOMETRY:
            backend_version = d3d_info->limits.gs_version;
            break;
        case WINED3D_SHADER_TYPE_PIXEL:
            backend_version = d3d_info->limits.ps_version;
            break;
        case WINED3D_SHADER_TYPE_COMPUTE:
            backend_version = d3d_info->limits.cs_version;
            break;
        default:
            FIXME("No backend version-checking for this shader type.\n");
            backend_version = 0;
    }
    if (version->major > backend_version)
    {
        WARN("Shader version %u.%u not supported by the current shader backend.\n",
                version->major, version->minor);
        return WINED3DERR_INVALIDCALL;
    }

    shader->load_local_constsF = shader->lconst_inf_or_nan;

    return WINED3D_OK;
}

ULONG CDECL wined3d_shader_incref(struct wined3d_shader *shader)
{
    ULONG refcount = InterlockedIncrement(&shader->ref);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    return refcount;
}

static void wined3d_shader_init_object(void *object)
{
    struct wined3d_shader *shader = object;
    struct wined3d_device *device = shader->device;

    list_add_head(&device->shaders, &shader->shader_list_entry);

    device->shader_backend->shader_precompile(device->shader_priv, shader);
}

static void wined3d_shader_destroy_object(void *object)
{
    shader_cleanup(object);
    heap_free(object);
}

ULONG CDECL wined3d_shader_decref(struct wined3d_shader *shader)
{
    ULONG refcount = InterlockedDecrement(&shader->ref);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        shader->parent_ops->wined3d_object_destroyed(shader->parent);
        wined3d_cs_destroy_object(shader->device->cs, wined3d_shader_destroy_object, shader);
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
        *byte_code_size = shader->byte_code_size;
        return WINED3D_OK;
    }

    if (*byte_code_size < shader->byte_code_size)
    {
        /* MSDN claims (for d3d8 at least) that if *byte_code_size is smaller
         * than the required size we should write the required size and
         * return D3DERR_MOREDATA. That's not actually true. */
        return WINED3DERR_INVALIDCALL;
    }

    memcpy(byte_code, shader->byte_code, shader->byte_code_size);

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
        struct wined3d_shader_lconst *lconst;
        float *value;

        if (!(lconst = heap_alloc(sizeof(*lconst))))
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

static void init_interpolation_compile_args(DWORD *interpolation_args,
        const struct wined3d_shader *pixel_shader, const struct wined3d_d3d_info *d3d_info)
{
    if (!d3d_info->shader_output_interpolation || !pixel_shader
            || pixel_shader->reg_maps.shader_version.major < 4)
    {
        memset(interpolation_args, 0, sizeof(pixel_shader->u.ps.interpolation_mode));
        return;
    }

    memcpy(interpolation_args, pixel_shader->u.ps.interpolation_mode,
            sizeof(pixel_shader->u.ps.interpolation_mode));
}

void find_vs_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        WORD swizzle_map, struct vs_compile_args *args, const struct wined3d_context *context)
{
    const struct wined3d_shader *geometry_shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY];
    const struct wined3d_shader *pixel_shader = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    const struct wined3d_shader *hull_shader = state->shader[WINED3D_SHADER_TYPE_HULL];
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;

    args->fog_src = state->render_states[WINED3D_RS_FOGTABLEMODE]
            == WINED3D_FOG_NONE ? VS_FOG_COORD : VS_FOG_Z;
    args->clip_enabled = state->render_states[WINED3D_RS_CLIPPING]
            && state->render_states[WINED3D_RS_CLIPPLANEENABLE];
    args->point_size = state->gl_primitive_type == GL_POINTS;
    args->per_vertex_point_size = shader->reg_maps.point_size;
    args->next_shader_type = hull_shader ? WINED3D_SHADER_TYPE_HULL
            : geometry_shader ? WINED3D_SHADER_TYPE_GEOMETRY : WINED3D_SHADER_TYPE_PIXEL;
    if (shader->reg_maps.shader_version.major >= 4)
        args->next_shader_input_count = hull_shader ? hull_shader->limits->packed_input
                : geometry_shader ? geometry_shader->limits->packed_input
                : pixel_shader ? pixel_shader->limits->packed_input : 0;
    else
        args->next_shader_input_count = 0;
    args->swizzle_map = swizzle_map;
    if (d3d_info->emulated_flatshading)
        args->flatshading = state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT;
    else
        args->flatshading = 0;

    init_interpolation_compile_args(args->interpolation_mode,
            args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL ? pixel_shader : NULL, d3d_info);
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

static HRESULT shader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    TRACE("byte_code %p, byte_code_size %#lx.\n", desc->byte_code, (long)desc->byte_code_size);

    if (!desc->byte_code)
        return WINED3DERR_INVALIDCALL;

    shader->ref = 1;
    shader->device = device;
    shader->parent = parent;
    shader->parent_ops = parent_ops;

    list_init(&shader->linked_programs);
    list_init(&shader->constantsF);
    list_init(&shader->constantsB);
    list_init(&shader->constantsI);
    shader->lconst_inf_or_nan = FALSE;
    list_init(&shader->reg_maps.indexable_temps);
    list_init(&shader->shader_list_entry);

    if (desc->byte_code_size == ~(size_t)0)
    {
        struct wined3d_shader_version shader_version;
        const struct wined3d_shader_frontend *fe;
        struct wined3d_shader_instruction ins;
        const DWORD *ptr;
        void *fe_data;

        if (!(shader->frontend = shader_select_frontend(WINED3D_SHADER_BYTE_CODE_FORMAT_SM1)))
        {
            FIXME("Unable to find frontend for shader.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto fail;
        }

        fe = shader->frontend;
        if (!(fe_data = fe->shader_init(desc->byte_code, desc->byte_code_size, &shader->output_signature)))
        {
            WARN("Failed to initialise frontend data.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto fail;
        }

        fe->shader_read_header(fe_data, &ptr, &shader_version);
        while (!fe->shader_is_end(fe_data, &ptr))
            fe->shader_read_instruction(fe_data, &ptr, &ins);

        fe->shader_free(fe_data);

        shader->byte_code_size = (ptr - desc->byte_code) * sizeof(*ptr);

        if (!(shader->byte_code = heap_alloc(shader->byte_code_size)))
        {
            hr = E_OUTOFMEMORY;
            goto fail;
        }
        memcpy(shader->byte_code, desc->byte_code, shader->byte_code_size);

        shader->function = shader->byte_code;
        shader->functionLength = shader->byte_code_size;
    }
    else
    {
        enum wined3d_shader_byte_code_format format;
        unsigned int max_version;

        if (!(shader->byte_code = heap_alloc(desc->byte_code_size)))
        {
            hr = E_OUTOFMEMORY;
            goto fail;
        }
        memcpy(shader->byte_code, desc->byte_code, desc->byte_code_size);
        shader->byte_code_size = desc->byte_code_size;

        max_version = shader_max_version_from_feature_level(device->feature_level);
        if (FAILED(hr = shader_extract_from_dxbc(shader, max_version, &format)))
            goto fail;

        if (!(shader->frontend = shader_select_frontend(format)))
        {
            FIXME("Unable to find frontend for shader.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto fail;
        }
    }

    return WINED3D_OK;

fail:
    shader_cleanup(shader);
    return hr;
}

static HRESULT vertex_shader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    unsigned int vs_uniform_count;
    unsigned int i;
    HRESULT hr;
    BOOL swvp = device->create_parms.flags & (WINED3DCREATE_SOFTWARE_VERTEXPROCESSING
            | WINED3DCREATE_MIXED_VERTEXPROCESSING);

    if (FAILED(hr = shader_init(shader, device, desc, parent, parent_ops)))
        return hr;

    vs_uniform_count = swvp ? device->adapter->d3d_info.limits.vs_uniform_count_swvp
            : device->adapter->d3d_info.limits.vs_uniform_count;
    if (FAILED(hr = shader_set_function(shader, device,
            WINED3D_SHADER_TYPE_VERTEX, vs_uniform_count, swvp)))
    {
        shader_cleanup(shader);
        return hr;
    }

    for (i = 0; i < shader->input_signature.element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &shader->input_signature.elements[i];

        if (!(reg_maps->input_registers & (1u << input->register_idx)) || !input->semantic_name)
            continue;

        shader->u.vs.attributes[input->register_idx].usage =
                shader_usage_from_semantic_name(input->semantic_name);
        shader->u.vs.attributes[input->register_idx].usage_idx = input->semantic_idx;
    }

    if (reg_maps->usesrelconstF && !list_empty(&shader->constantsF))
        shader->load_local_constsF = TRUE;

    return WINED3D_OK;
}

static struct wined3d_shader_signature_element *shader_find_signature_element(const struct wined3d_shader_signature *s,
        unsigned int stream_idx, const char *semantic_name, unsigned int semantic_idx)
{
    struct wined3d_shader_signature_element *e = s->elements;
    unsigned int i;

    for (i = 0; i < s->element_count; ++i)
    {
        if (e[i].stream_idx == stream_idx
                && !_strnicmp(e[i].semantic_name, semantic_name, -1)
                && e[i].semantic_idx == semantic_idx)
            return &e[i];
    }

    return NULL;
}

BOOL shader_get_stream_output_register_info(const struct wined3d_shader *shader,
        const struct wined3d_stream_output_element *so_element, unsigned int *register_idx, unsigned int *component_idx)
{
    const struct wined3d_shader_signature_element *output;
    unsigned int idx;

    if (!(output = shader_find_signature_element(&shader->output_signature,
            so_element->stream_idx, so_element->semantic_name, so_element->semantic_idx)))
        return FALSE;

    for (idx = 0; idx < 4; ++idx)
    {
        if (output->mask & (1u << idx))
            break;
    }
    idx += so_element->component_idx;

    *register_idx = output->register_idx;
    *component_idx = idx;
    return TRUE;
}

static HRESULT geometry_shader_init_stream_output(struct wined3d_shader *shader,
        const struct wined3d_stream_output_desc *so_desc)
{
    const struct wined3d_shader_frontend *fe = shader->frontend;
    const struct wined3d_shader_signature_element *output;
    unsigned int i, component_idx, register_idx, mask;
    struct wined3d_stream_output_element *elements;
    struct wined3d_shader_version shader_version;
    const DWORD *ptr;
    void *fe_data;
    HRESULT hr;

    if (!so_desc)
        return WINED3D_OK;

    if (!(fe_data = fe->shader_init(shader->function, shader->functionLength, &shader->output_signature)))
    {
        WARN("Failed to initialise frontend data.\n");
        return WINED3DERR_INVALIDCALL;
    }
    fe->shader_read_header(fe_data, &ptr, &shader_version);
    fe->shader_free(fe_data);

    switch (shader_version.type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
        case WINED3D_SHADER_TYPE_DOMAIN:
            shader->function = NULL;
            shader->functionLength = 0;
            break;
        case WINED3D_SHADER_TYPE_GEOMETRY:
            break;
        default:
            WARN("Wrong shader type %s.\n", debug_shader_type(shader_version.type));
            return E_INVALIDARG;
    }

    if (!shader->function)
    {
        shader->reg_maps.shader_version = shader_version;
        shader->reg_maps.shader_version.type = WINED3D_SHADER_TYPE_GEOMETRY;
        shader_set_limits(shader, 0);
        if (FAILED(hr = shader_scan_output_signature(shader)))
            return hr;
    }

    if (!(elements = heap_calloc(so_desc->element_count, sizeof(*elements))))
        return E_OUTOFMEMORY;

    shader->u.gs.so_desc = *so_desc;
    shader->u.gs.so_desc.elements = elements;
    memcpy(elements, so_desc->elements, so_desc->element_count * sizeof(*elements));

    for (i = 0; i < so_desc->element_count; ++i)
    {
        struct wined3d_stream_output_element *e = &elements[i];

        if (!e->semantic_name)
            continue;
        if (!(output = shader_find_signature_element(&shader->output_signature,
                e->stream_idx, e->semantic_name, e->semantic_idx))
                || !shader_get_stream_output_register_info(shader, e, &register_idx, &component_idx))
        {
            WARN("Failed to find output signature element for stream output entry.\n");
            return E_INVALIDARG;
        }

        e->semantic_name = output->semantic_name;

        mask = ((1u << e->component_count) - 1) << component_idx;
        if ((output->mask & 0xff & mask) != mask)
        {
            WARN("Invalid component range %u-%u (mask %#x), output mask %#x.\n",
                    component_idx, e->component_count, mask, output->mask & 0xff);
            return E_INVALIDARG;
        }
    }

    return WINED3D_OK;
}

static HRESULT geometry_shader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, const struct wined3d_stream_output_desc *so_desc,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    if (FAILED(hr = shader_init(shader, device, desc, parent, parent_ops)))
        return hr;

    if (FAILED(hr = geometry_shader_init_stream_output(shader, so_desc)))
        goto fail;

    if (shader->function
            && FAILED(hr = shader_set_function(shader, device, WINED3D_SHADER_TYPE_GEOMETRY, 0, 0)))
        goto fail;

    return WINED3D_OK;

fail:
    shader_cleanup(shader);
    return hr;
}

void find_ds_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        struct ds_compile_args *args, const struct wined3d_context *context)
{
    const struct wined3d_shader *geometry_shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY];
    const struct wined3d_shader *pixel_shader = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    const struct wined3d_shader *hull_shader = state->shader[WINED3D_SHADER_TYPE_HULL];

    args->tessellator_output_primitive = hull_shader->u.hs.tessellator_output_primitive;
    args->tessellator_partitioning = hull_shader->u.hs.tessellator_partitioning;

    args->output_count = geometry_shader ? geometry_shader->limits->packed_input
            : pixel_shader ? pixel_shader->limits->packed_input : shader->limits->packed_output;
    args->next_shader_type = geometry_shader ? WINED3D_SHADER_TYPE_GEOMETRY : WINED3D_SHADER_TYPE_PIXEL;

    args->render_offscreen = context->render_offscreen;

    init_interpolation_compile_args(args->interpolation_mode,
            args->next_shader_type == WINED3D_SHADER_TYPE_PIXEL ? pixel_shader : NULL, context->d3d_info);

    args->padding = 0;
}

void find_gs_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        struct gs_compile_args *args, const struct wined3d_context *context)
{
    const struct wined3d_shader *pixel_shader = state->shader[WINED3D_SHADER_TYPE_PIXEL];

    args->output_count = pixel_shader ? pixel_shader->limits->packed_input : shader->limits->packed_output;

    if (!(args->primitive_type = shader->u.gs.input_type))
        args->primitive_type = d3d_primitive_type_from_gl(state->gl_primitive_type);

    init_interpolation_compile_args(args->interpolation_mode, pixel_shader, context->d3d_info);
}

void find_ps_compile_args(const struct wined3d_state *state, const struct wined3d_shader *shader,
        BOOL position_transformed, struct ps_compile_args *args, const struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = &context->device->adapter->gl_info;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    struct wined3d_texture *texture;
    unsigned int i;

    memset(args, 0, sizeof(*args)); /* FIXME: Make sure all bits are set. */
    if (!d3d_info->srgb_write_control && needs_srgb_write(context, state, state->fb))
    {
        static unsigned int warned = 0;

        args->srgb_correction = 1;
        if (state->render_states[WINED3D_RS_ALPHABLENDENABLE] && !warned++)
            WARN("Blending into a sRGB render target with no GL_ARB_framebuffer_sRGB "
                    "support, expect rendering artifacts.\n");
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
            if (!shader->reg_maps.resource_info[i].type)
                continue;

            /* Treat unbound textures as 2D. The dummy texture will provide
             * the proper sample value. The tex_types bitmap defaults to
             * 2D because of the memset. */
            if (!(texture = state->textures[i]))
                continue;

            switch (wined3d_texture_gl(texture)->target)
            {
                /* RECT textures are distinguished from 2D textures via np2_fixup */
                default:
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
    else if (shader->reg_maps.shader_version.major <= 3)
    {
        for (i = 0; i < shader->limits->sampler; ++i)
        {
            enum wined3d_shader_resource_type resource_type;
            enum wined3d_shader_tex_types tex_type;

            if (!(resource_type = shader->reg_maps.resource_info[i].type))
                continue;

            switch (resource_type)
            {
                case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
                    tex_type = WINED3D_SHADER_TEX_3D;
                    break;
                case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
                    tex_type = WINED3D_SHADER_TEX_CUBE;
                    break;
                default:
                    tex_type = WINED3D_SHADER_TEX_2D;
                    break;
            }

            if ((texture = state->textures[i]))
            {
                if (texture->resource.type == WINED3D_RTYPE_TEXTURE_2D
                        && resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_3D
                        && !(texture->resource.usage & WINED3DUSAGE_LEGACY_CUBEMAP))
                    tex_type = WINED3D_SHADER_TEX_2D;
                else if (texture->resource.type == WINED3D_RTYPE_TEXTURE_3D
                        && resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2D)
                    tex_type = WINED3D_SHADER_TEX_3D;
            }
            args->tex_types |= tex_type << i * WINED3D_PSARGS_TEXTYPE_SHIFT;
        }
    }

    if (shader->reg_maps.shader_version.major >= 4)
    {
        /* In SM4+ we use dcl_sampler in order to determine if we should use shadow sampler. */
        args->shadow = 0;
        for (i = 0 ; i < WINED3D_MAX_FRAGMENT_SAMPLERS; ++i)
            args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
        args->np2_fixup = 0;
    }
    else
    {
        for (i = 0; i < WINED3D_MAX_FRAGMENT_SAMPLERS; ++i)
        {
            if (!shader->reg_maps.resource_info[i].type)
                continue;

            texture = state->textures[i];
            if (!texture)
            {
                args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
                continue;
            }
            if (can_use_texture_swizzle(d3d_info, texture->resource.format))
                args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
            else
                args->color_fixup[i] = texture->resource.format->color_fixup;

            if (texture->resource.format_flags & WINED3DFMT_FLAG_SHADOW)
                args->shadow |= 1u << i;

            /* Flag samplers that need NP2 texcoord fixup. */
            if (!(texture->flags & WINED3D_TEXTURE_POW2_MAT_IDENT))
                args->np2_fixup |= (1u << i);
        }
    }

    if (shader->reg_maps.shader_version.major >= 3)
    {
        if (position_transformed)
            args->vp_mode = WINED3D_VP_MODE_NONE;
        else if (use_vs(state))
            args->vp_mode = WINED3D_VP_MODE_SHADER;
        else
            args->vp_mode = WINED3D_VP_MODE_FF;
        args->fog = WINED3D_FFP_PS_FOG_OFF;
    }
    else
    {
        args->vp_mode = WINED3D_VP_MODE_SHADER;
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

    if (!d3d_info->full_ffp_varyings)
    {
        const struct wined3d_shader *vs = state->shader[WINED3D_SHADER_TYPE_VERTEX];

        args->texcoords_initialized = 0;
        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
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
                        || (coord_idx < WINED3D_MAX_TEXTURES && (si->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + coord_idx)))))
                    args->texcoords_initialized |= 1u << i;
            }
        }
    }
    else
    {
        args->texcoords_initialized = (1u << WINED3D_MAX_TEXTURES) - 1;
    }

    args->pointsprite = state->render_states[WINED3D_RS_POINTSPRITEENABLE]
            && state->gl_primitive_type == GL_POINTS;

    if (d3d_info->ffp_alpha_test)
        args->alpha_test_func = WINED3D_CMP_ALWAYS - 1;
    else
        args->alpha_test_func = (state->render_states[WINED3D_RS_ALPHATESTENABLE]
                ? wined3d_sanitize_cmp_func(state->render_states[WINED3D_RS_ALPHAFUNC])
                : WINED3D_CMP_ALWAYS) - 1;

    if (d3d_info->emulated_flatshading)
        args->flatshading = state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT;

    args->render_offscreen = shader->reg_maps.vpos && gl_info->supported[ARB_FRAGMENT_COORD_CONVENTIONS]
            ? context->render_offscreen : 0;

    for (i = 0; i < ARRAY_SIZE(state->fb->render_targets); ++i)
    {
        struct wined3d_rendertarget_view *rtv = state->fb->render_targets[i];
        if (rtv && rtv->format->id == WINED3DFMT_A8_UNORM && !is_identity_fixup(rtv->format->color_fixup))
            args->rt_alpha_swizzle |= 1u << i;
    }

    args->dual_source_blend = wined3d_dualblend_enabled(state, gl_info);
}

static HRESULT pixel_shader_init(struct wined3d_shader *shader, struct wined3d_device *device,
        const struct wined3d_shader_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i, highest_reg_used = 0, num_regs_used = 0;
    HRESULT hr;

    if (FAILED(hr = shader_init(shader, device, desc, parent, parent_ops)))
        return hr;

    if (FAILED(hr = shader_set_function(shader, device,
            WINED3D_SHADER_TYPE_PIXEL, device->adapter->d3d_info.limits.ps_uniform_count, 0)))
    {
        shader_cleanup(shader);
        return hr;
    }

    for (i = 0; i < MAX_REG_INPUT; ++i)
    {
        if (shader->u.ps.input_reg_used & (1u << i))
        {
            ++num_regs_used;
            highest_reg_used = i;
        }
    }

    /* Don't do any register mapping magic if it is not needed, or if we can't
     * achieve anything anyway */
    if (highest_reg_used < (gl_info->limits.glsl_varyings / 4)
            || num_regs_used > (gl_info->limits.glsl_varyings / 4)
            || shader->reg_maps.shader_version.major >= 4)
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
            if (shader->u.ps.input_reg_used & (1u << i))
                shader->u.ps.input_reg_map[i] = shader->u.ps.declared_in_count++;
            else shader->u.ps.input_reg_map[i] = ~0U;
        }
    }

    return WINED3D_OK;
}

enum wined3d_shader_resource_type pixelshader_get_resource_type(const struct wined3d_shader_reg_maps *reg_maps,
        unsigned int resource_idx, DWORD tex_types)
{
    static enum wined3d_shader_resource_type shader_resource_type_from_shader_tex_types[] =
    {
        WINED3D_SHADER_RESOURCE_TEXTURE_2D,     /* WINED3D_SHADER_TEX_2D     */
        WINED3D_SHADER_RESOURCE_TEXTURE_3D,     /* WINED3D_SHADER_TEX_3D     */
        WINED3D_SHADER_RESOURCE_TEXTURE_CUBE,   /* WINED3D_SHADER_TEX_CUBE   */
    };

    unsigned int idx;

    if (reg_maps->shader_version.major > 3)
        return reg_maps->resource_info[resource_idx].type;

    if (!reg_maps->resource_info[resource_idx].type)
        return 0;

    idx = (tex_types >> resource_idx * WINED3D_PSARGS_TEXTYPE_SHIFT) & WINED3D_PSARGS_TEXTYPE_MASK;
    assert(idx < ARRAY_SIZE(shader_resource_type_from_shader_tex_types));
    return shader_resource_type_from_shader_tex_types[idx];
}

HRESULT CDECL wined3d_shader_create_cs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, shader %p.\n",
            device, desc, parent, parent_ops, shader);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = shader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize compute shader, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    if (FAILED(hr = shader_set_function(object, device, WINED3D_SHADER_TYPE_COMPUTE, 0, 0)))
    {
        shader_cleanup(object);
        heap_free(object);
        return hr;
    }

    wined3d_cs_init_object(device->cs, wined3d_shader_init_object, object);

    TRACE("Created compute shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_create_ds(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, shader %p.\n",
            device, desc, parent, parent_ops, shader);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = shader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize domain shader, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    if (FAILED(hr = shader_set_function(object, device, WINED3D_SHADER_TYPE_DOMAIN, 0, 0)))
    {
        shader_cleanup(object);
        heap_free(object);
        return hr;
    }

    wined3d_cs_init_object(device->cs, wined3d_shader_init_object, object);

    TRACE("Created domain shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_create_gs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        const struct wined3d_stream_output_desc *so_desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, desc %p, so_desc %p, parent %p, parent_ops %p, shader %p.\n",
            device, desc, so_desc, parent, parent_ops, shader);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = geometry_shader_init(object, device, desc, so_desc, parent, parent_ops)))
    {
        WARN("Failed to initialize geometry shader, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    wined3d_cs_init_object(device->cs, wined3d_shader_init_object, object);

    TRACE("Created geometry shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_create_hs(struct wined3d_device *device, const struct wined3d_shader_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_shader **shader)
{
    struct wined3d_shader *object;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, shader %p.\n",
            device, desc, parent, parent_ops, shader);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = shader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize hull shader, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    if (FAILED(hr = shader_set_function(object, device, WINED3D_SHADER_TYPE_HULL, 0, 0)))
    {
        shader_cleanup(object);
        heap_free(object);
        return hr;
    }

    wined3d_cs_init_object(device->cs, wined3d_shader_init_object, object);

    TRACE("Created hull shader %p.\n", object);
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

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = pixel_shader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize pixel shader, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    wined3d_cs_init_object(device->cs, wined3d_shader_init_object, object);

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

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = vertex_shader_init(object, device, desc, parent, parent_ops)))
    {
        WARN("Failed to initialize vertex shader, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    wined3d_cs_init_object(device->cs, wined3d_shader_init_object, object);

    TRACE("Created vertex shader %p.\n", object);
    *shader = object;

    return WINED3D_OK;
}
