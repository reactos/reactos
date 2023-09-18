/*
 * Fixed function pipeline replacement using GL_NV_register_combiners
 * and GL_NV_texture_shader
 *
 * Copyright 2006 Henri Verbeet
 * Copyright 2008 Stefan Dösinger(for CodeWeavers)
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* Context activation for state handlers is done by the caller. */

static void nvts_activate_dimensions(const struct wined3d_state *state,
        unsigned int stage, struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_texture *texture;
    BOOL bumpmap = FALSE;

    if (stage > 0
            && (state->texture_states[stage - 1][WINED3D_TSS_COLOR_OP] == WINED3D_TOP_BUMPENVMAP_LUMINANCE
            || state->texture_states[stage - 1][WINED3D_TSS_COLOR_OP] == WINED3D_TOP_BUMPENVMAP))
    {
        bumpmap = TRUE;
        context_gl->c.texShaderBumpMap |= (1u << stage);
    }
    else
    {
        context_gl->c.texShaderBumpMap &= ~(1u << stage);
    }

    if ((texture = state->textures[stage]))
    {
        switch (wined3d_texture_gl(texture)->target)
        {
            case GL_TEXTURE_2D:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV,
                        bumpmap ? GL_OFFSET_TEXTURE_2D_NV : GL_TEXTURE_2D);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, ...)");
                break;
            case GL_TEXTURE_RECTANGLE_ARB:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV,
                        bumpmap ? GL_OFFSET_TEXTURE_2D_NV : GL_TEXTURE_RECTANGLE_ARB);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, ...)");
                break;
            case GL_TEXTURE_3D:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D)");
                break;
            case GL_TEXTURE_CUBE_MAP_ARB:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_CUBE_MAP_ARB)");
                break;
            default:
                FIXME("Unhandled target %#x.\n", wined3d_texture_gl(texture)->target);
                break;
        }
    }
    else
    {
        gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
        checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE)");
    }
}

struct tex_op_args
{
    GLenum input[3];
    GLenum mapping[3];
    GLenum component_usage[3];
};

static GLenum d3dta_to_combiner_input(DWORD d3dta, DWORD stage, INT texture_idx) {
    switch (d3dta) {
        case WINED3DTA_DIFFUSE:
            return GL_PRIMARY_COLOR_NV;

        case WINED3DTA_CURRENT:
            if (stage) return GL_SPARE0_NV;
            else return GL_PRIMARY_COLOR_NV;

        case WINED3DTA_TEXTURE:
            if (texture_idx > -1) return GL_TEXTURE0_ARB + texture_idx;
            else return GL_PRIMARY_COLOR_NV;

        case WINED3DTA_TFACTOR:
            return GL_CONSTANT_COLOR0_NV;

        case WINED3DTA_SPECULAR:
            return GL_SECONDARY_COLOR_NV;

        case WINED3DTA_TEMP:
            return GL_SPARE1_NV;

        case WINED3DTA_CONSTANT:
            /* TODO: Support per stage constants (WINED3D_TSS_CONSTANT, NV_register_combiners2) */
            FIXME("WINED3DTA_CONSTANT, not properly supported.\n");
            return GL_CONSTANT_COLOR1_NV;

        default:
            FIXME("Unrecognized texture arg %#x\n", d3dta);
            return GL_TEXTURE;
    }
}

static GLenum invert_mapping(GLenum mapping) {
    if (mapping == GL_UNSIGNED_INVERT_NV) return GL_UNSIGNED_IDENTITY_NV;
    else if (mapping == GL_UNSIGNED_IDENTITY_NV) return GL_UNSIGNED_INVERT_NV;

    FIXME("Unhandled mapping %#x\n", mapping);
    return mapping;
}

static void get_src_and_opr_nvrc(DWORD stage, DWORD arg, BOOL is_alpha, GLenum* input, GLenum* mapping, GLenum *component_usage, INT texture_idx) {
    /* The WINED3DTA_COMPLEMENT flag specifies the complement of the input should
    * be used. */
    if (arg & WINED3DTA_COMPLEMENT) *mapping = GL_UNSIGNED_INVERT_NV;
    else *mapping = GL_UNSIGNED_IDENTITY_NV; /* Clamp all values to positive ranges */

    /* The WINED3DTA_ALPHAREPLICATE flag specifies the alpha component of the input
    * should be used for all input components. */
    if (is_alpha || arg & WINED3DTA_ALPHAREPLICATE) *component_usage = GL_ALPHA;
    else *component_usage = GL_RGB;

    *input = d3dta_to_combiner_input(arg & WINED3DTA_SELECTMASK, stage, texture_idx);
}

void set_tex_op_nvrc(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state, BOOL is_alpha,
        int stage, enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3, INT texture_idx, DWORD dst)
{
    struct tex_op_args tex_op_args = {{0}, {0}, {0}};
    GLenum portion = is_alpha ? GL_ALPHA : GL_RGB;
    GLenum target = GL_COMBINER0_NV + stage;
    GLenum output;

    TRACE("stage %d, is_alpha %d, op %s, arg1 %#x, arg2 %#x, arg3 %#x, texture_idx %d\n",
          stage, is_alpha, debug_d3dtop(op), arg1, arg2, arg3, texture_idx);

    /* If a texture stage references an invalid texture unit the stage just
    * passes through the result from the previous stage */
    if (is_invalid_op(state, stage, op, arg1, arg2, arg3))
    {
        arg1 = WINED3DTA_CURRENT;
        op = WINED3D_TOP_SELECT_ARG1;
    }

    get_src_and_opr_nvrc(stage, arg1, is_alpha, &tex_op_args.input[0],
                         &tex_op_args.mapping[0], &tex_op_args.component_usage[0], texture_idx);
    get_src_and_opr_nvrc(stage, arg2, is_alpha, &tex_op_args.input[1],
                         &tex_op_args.mapping[1], &tex_op_args.component_usage[1], texture_idx);
    get_src_and_opr_nvrc(stage, arg3, is_alpha, &tex_op_args.input[2],
                         &tex_op_args.mapping[2], &tex_op_args.component_usage[2], texture_idx);


    if(dst == WINED3DTA_TEMP) {
        output = GL_SPARE1_NV;
    } else {
        output = GL_SPARE0_NV;
    }

    switch (op)
    {
        case WINED3D_TOP_DISABLE:
            /* Only for alpha */
            if (!is_alpha)
                ERR("Shouldn't be called for WINED3D_TSS_COLOR_OP (WINED3DTOP_DISABLE).\n");
            /* Input, prev_alpha*1 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                       GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_SELECT_ARG1:
        case WINED3D_TOP_SELECT_ARG2:
            /* Input, arg*1 */
            if (op == WINED3D_TOP_SELECT_ARG1)
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                           tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            else
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                           tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                       GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_MODULATE:
        case WINED3D_TOP_MODULATE_2X:
        case WINED3D_TOP_MODULATE_4X:
            /* Input, arg1*arg2 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));

            /* Output */
            if (op == WINED3D_TOP_MODULATE)
                GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                           GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            else if (op == WINED3D_TOP_MODULATE_2X)
                GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                           GL_DISCARD_NV, GL_SCALE_BY_TWO_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            else if (op == WINED3D_TOP_MODULATE_4X)
                GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                           GL_DISCARD_NV, GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_ADD:
        case WINED3D_TOP_ADD_SIGNED:
        case WINED3D_TOP_ADD_SIGNED_2X:
            /* Input, arg1*1+arg2*1 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            if (op == WINED3D_TOP_ADD)
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                           output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            else if (op == WINED3D_TOP_ADD_SIGNED)
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                           output, GL_NONE, GL_BIAS_BY_NEGATIVE_ONE_HALF_NV, GL_FALSE, GL_FALSE, GL_FALSE));
            else if (op == WINED3D_TOP_ADD_SIGNED_2X)
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                           output, GL_SCALE_BY_TWO_NV, GL_BIAS_BY_NEGATIVE_ONE_HALF_NV, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_SUBTRACT:
            /* Input, arg1*1+-arg2*1 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[1], GL_SIGNED_NEGATE_NV, tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_ADD_SMOOTH:
            /* Input, arg1*1+(1-arg1)*arg2 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[0], invert_mapping(tex_op_args.mapping[0]), tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_BLEND_DIFFUSE_ALPHA:
        case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
        case WINED3D_TOP_BLEND_FACTOR_ALPHA:
        case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
        case WINED3D_TOP_BLEND_CURRENT_ALPHA:
        {
            GLenum alpha_src = GL_PRIMARY_COLOR_NV;
            if (op == WINED3D_TOP_BLEND_DIFFUSE_ALPHA)
                alpha_src = d3dta_to_combiner_input(WINED3DTA_DIFFUSE, stage, texture_idx);
            else if (op == WINED3D_TOP_BLEND_TEXTURE_ALPHA)
                alpha_src = d3dta_to_combiner_input(WINED3DTA_TEXTURE, stage, texture_idx);
            else if (op == WINED3D_TOP_BLEND_FACTOR_ALPHA)
                alpha_src = d3dta_to_combiner_input(WINED3DTA_TFACTOR, stage, texture_idx);
            else if (op == WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM)
                alpha_src = d3dta_to_combiner_input(WINED3DTA_TEXTURE, stage, texture_idx);
            else if (op == WINED3D_TOP_BLEND_CURRENT_ALPHA)
                alpha_src = d3dta_to_combiner_input(WINED3DTA_CURRENT, stage, texture_idx);
            else
                FIXME("Unhandled texture op %s, shouldn't happen.\n", debug_d3dtop(op));

            /* Input, arg1*alpha_src+arg2*(1-alpha_src) */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            if (op == WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM)
            {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                           GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            } else {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                           alpha_src, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA));
            }
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       alpha_src, GL_UNSIGNED_INVERT_NV, GL_ALPHA));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;
        }

        case WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR:
            /* Input, arg1_alpha*arg2_rgb+arg1_rgb*1 */
            if (is_alpha)
                ERR("Only supported for WINED3D_TSS_COLOR_OP (WINED3DTOP_MODULATEALPHA_ADDCOLOR).\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA:
            /* Input, arg1_rgb*arg2_rgb+arg1_alpha*1 */
            if (is_alpha)
                ERR("Only supported for WINED3D_TSS_COLOR_OP (WINED3DTOP_MODULATECOLOR_ADDALPHA).\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR:
            /* Input, (1-arg1_alpha)*arg2_rgb+arg1_rgb*1 */
            if (is_alpha)
                ERR("Only supported for WINED3D_TSS_COLOR_OP (WINED3DTOP_MODULATEINVALPHA_ADDCOLOR).\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], invert_mapping(tex_op_args.mapping[0]), GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA:
            /* Input, (1-arg1_rgb)*arg2_rgb+arg1_alpha*1 */
            if (is_alpha)
                ERR("Only supported for WINED3D_TSS_COLOR_OP (WINED3DTOP_MODULATEINVCOLOR_ADDALPHA).\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], invert_mapping(tex_op_args.mapping[0]), tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_DOTPRODUCT3:
            /* Input, arg1 . arg2 */
            /* FIXME: DX7 uses a different calculation? */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], GL_EXPAND_NORMAL_NV, tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[1], GL_EXPAND_NORMAL_NV, tex_op_args.component_usage[1]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                       GL_DISCARD_NV, GL_NONE, GL_NONE, GL_TRUE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_MULTIPLY_ADD:
            /* Input, arg3*1+arg1*arg2 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[2], tex_op_args.mapping[2], tex_op_args.component_usage[2]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_LERP:
            /* Input, arg3*arg1+(1-arg3)*arg2 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[2], tex_op_args.mapping[2], tex_op_args.component_usage[2]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                       tex_op_args.input[2], invert_mapping(tex_op_args.mapping[2]), tex_op_args.component_usage[2]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                       output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
        case WINED3D_TOP_BUMPENVMAP:
            if (!gl_info->supported[NV_TEXTURE_SHADER])
            {
                WARN("BUMPENVMAP requires GL_NV_texture_shader in this codepath\n");
                break;
            }

            /* The bump map stage itself isn't exciting, just read the texture. But tell the next stage to
             * perform bump mapping and source from the current stage. Pretty much a SELECTARG2.
             * ARG2 is passed through unmodified(apps will most likely use D3DTA_CURRENT for arg2, arg1
             * (which will most likely be D3DTA_TEXTURE) is available as a texture shader input for the
             * next stage */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                        tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                        GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            /* Always pass through to CURRENT, ignore temp arg */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                        GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        default:
            FIXME("Unhandled texture op: stage %d, is_alpha %d, op %s (%#x), arg1 %#x, arg2 %#x, arg3 %#x, texture_idx %d.\n",
                  stage, is_alpha, debug_d3dtop(op), op, arg1, arg2, arg3, texture_idx);
    }

    checkGLcall("set_tex_op_nvrc()");
}


static void nvrc_colorop(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    DWORD stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    BOOL tex_used = context->fixed_function_usage_map & (1u << stage);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int mapped_stage = context_gl->tex_unit_map[stage];

    TRACE("Setting color op for stage %u.\n", stage);

    /* Using a pixel shader? Don't care for anything here, the shader applying does it */
    if (use_ps(state)) return;

    if (stage != mapped_stage) WARN("Using non 1:1 mapping: %d -> %d!\n", stage, mapped_stage);

    if (mapped_stage != WINED3D_UNMAPPED_STAGE)
    {
        if (tex_used && mapped_stage >= gl_info->limits.textures)
        {
            FIXME("Attempt to enable unsupported stage!\n");
            return;
        }
        wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);
    }

    if (context->lowest_disabled_stage > 0)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_REGISTER_COMBINERS_NV);
        GL_EXTCALL(glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, context->lowest_disabled_stage));
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_REGISTER_COMBINERS_NV);
    }
    if (stage >= context->lowest_disabled_stage)
    {
        TRACE("Stage disabled\n");
        if (mapped_stage != WINED3D_UNMAPPED_STAGE)
        {
            /* Disable everything here */
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
            checkGLcall("glDisable(GL_TEXTURE_2D)");
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable(GL_TEXTURE_3D)");
            if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
            {
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
            }
            if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
            {
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
                checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
            }
            if (gl_info->supported[NV_TEXTURE_SHADER2] && mapped_stage < gl_info->limits.textures)
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
            }
        }
        /* All done */
        return;
    }

    /* The sampler will also activate the correct texture dimensions, so no need to do it here
     * if the sampler for this stage is dirty
     */
    if (!isStateDirty(context, STATE_SAMPLER(stage)))
    {
        if (tex_used)
        {
            if (gl_info->supported[NV_TEXTURE_SHADER2])
                nvts_activate_dimensions(state, stage, context_gl);
            else
                texture_activate_dimensions(state->textures[stage], gl_info);
        }
    }

    /* Set the texture combiners */
    set_tex_op_nvrc(gl_info, state, FALSE, stage,
            state->texture_states[stage][WINED3D_TSS_COLOR_OP],
            state->texture_states[stage][WINED3D_TSS_COLOR_ARG1],
            state->texture_states[stage][WINED3D_TSS_COLOR_ARG2],
            state->texture_states[stage][WINED3D_TSS_COLOR_ARG0],
            mapped_stage,
            state->texture_states[stage][WINED3D_TSS_RESULT_ARG]);

    /* In register combiners bump mapping is done in the stage AFTER the one that has the bump map operation set,
     * thus the texture shader may have to be updated
     */
    if (gl_info->supported[NV_TEXTURE_SHADER2])
    {
        BOOL usesBump = (state->texture_states[stage][WINED3D_TSS_COLOR_OP] == WINED3D_TOP_BUMPENVMAP_LUMINANCE
                || state->texture_states[stage][WINED3D_TSS_COLOR_OP] == WINED3D_TOP_BUMPENVMAP);
        BOOL usedBump = !!(context->texShaderBumpMap & 1u << (stage + 1));
        if (usesBump != usedBump)
        {
            wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage + 1);
            nvts_activate_dimensions(state, stage + 1, context_gl);
            wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);
        }
    }
}

static void nvrc_resultarg(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    DWORD stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);

    TRACE("Setting result arg for stage %u.\n", stage);

    if (!isStateDirty(context, STATE_TEXTURESTAGE(stage, WINED3D_TSS_COLOR_OP)))
    {
        context_apply_state(context, state, STATE_TEXTURESTAGE(stage, WINED3D_TSS_COLOR_OP));
    }
    if (!isStateDirty(context, STATE_TEXTURESTAGE(stage, WINED3D_TSS_ALPHA_OP)))
    {
        context_apply_state(context, state, STATE_TEXTURESTAGE(stage, WINED3D_TSS_ALPHA_OP));
    }
}

static void nvts_texdim(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    unsigned int sampler, mapped_stage;

    sampler = state_id - STATE_SAMPLER(0);
    mapped_stage = context_gl->tex_unit_map[sampler];

    /* No need to enable / disable anything here for unused samplers. The tex_colorop
    * handler takes care. Also no action is needed with pixel shaders, or if tex_colorop
    * will take care of this business. */
    if (mapped_stage == WINED3D_UNMAPPED_STAGE || mapped_stage >= context_gl->gl_info->limits.textures)
        return;
    if (sampler >= context->lowest_disabled_stage)
        return;
    if (isStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3D_TSS_COLOR_OP)))
        return;

    nvts_activate_dimensions(state, sampler, context_gl);
}

static void nvts_bumpenvmat(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    DWORD stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    unsigned int mapped_stage = context_gl->tex_unit_map[stage + 1];
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    float mat[2][2];

    /* Direct3D sets the matrix in the stage reading the perturbation map. The result is used to
     * offset the destination stage(always stage + 1 in d3d). In GL_NV_texture_shader, the bump
     * map offsetting is done in the stage reading the bump mapped texture, and the perturbation
     * map is read from a specified source stage(always stage - 1 for d3d). Thus set the matrix
     * for stage + 1. Keep the nvrc tex unit mapping in mind too
     */
    if (mapped_stage < gl_info->limits.textures)
    {
        wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);

        /* We can't just pass a pointer to the state to GL due to the
         * different matrix format (column major vs row major). */
        mat[0][0] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT00]);
        mat[1][0] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT01]);
        mat[0][1] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT10]);
        mat[1][1] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT11]);
        gl_info->gl_ops.gl.p_glTexEnvfv(GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV, (float *)mat);
        checkGLcall("glTexEnvfv(GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV, mat)");
    }
}

static void nvrc_texfactor(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_color color;

    wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_TEXTUREFACTOR]);
    GL_EXTCALL(glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, &color.r));
}

/* Context activation is done by the caller. */
static void nvrc_enable(const struct wined3d_context *context, BOOL enable)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_const(context)->gl_info;

    if (enable)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_REGISTER_COMBINERS_NV);
        checkGLcall("glEnable(GL_REGISTER_COMBINERS_NV)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_REGISTER_COMBINERS_NV);
        checkGLcall("glDisable(GL_REGISTER_COMBINERS_NV)");
    }
}

/* Context activation is done by the caller. */
static void nvts_enable(const struct wined3d_context *context, BOOL enable)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_const(context)->gl_info;

    nvrc_enable(context, enable);
    if (enable)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glEnable(GL_TEXTURE_SHADER_NV)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glDisable(GL_TEXTURE_SHADER_NV)");
    }
}

static void nvrc_fragment_get_caps(const struct wined3d_adapter *adapter, struct fragment_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    caps->wined3d_caps = 0;
    caps->PrimitiveMiscCaps = WINED3DPMISCCAPS_TSSARGTEMP;

    /* The caps below can be supported but aren't handled yet in utils.c
     * 'd3dta_to_combiner_input', disable them until support is fixed */
#if 0
    if (gl_info->supported[NV_REGISTER_COMBINERS2])
        caps->PrimitiveMiscCaps |=  WINED3DPMISCCAPS_PERSTAGECONSTANT;
#endif

    caps->TextureOpCaps = WINED3DTEXOPCAPS_ADD
            | WINED3DTEXOPCAPS_ADDSIGNED
            | WINED3DTEXOPCAPS_ADDSIGNED2X
            | WINED3DTEXOPCAPS_MODULATE
            | WINED3DTEXOPCAPS_MODULATE2X
            | WINED3DTEXOPCAPS_MODULATE4X
            | WINED3DTEXOPCAPS_SELECTARG1
            | WINED3DTEXOPCAPS_SELECTARG2
            | WINED3DTEXOPCAPS_DISABLE
            | WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA
            | WINED3DTEXOPCAPS_BLENDTEXTUREALPHA
            | WINED3DTEXOPCAPS_BLENDFACTORALPHA
            | WINED3DTEXOPCAPS_BLENDCURRENTALPHA
            | WINED3DTEXOPCAPS_LERP
            | WINED3DTEXOPCAPS_SUBTRACT
            | WINED3DTEXOPCAPS_ADDSMOOTH
            | WINED3DTEXOPCAPS_MULTIPLYADD
            | WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR
            | WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA
            | WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM
            | WINED3DTEXOPCAPS_DOTPRODUCT3
            | WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR
            | WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA;

    if (gl_info->supported[NV_TEXTURE_SHADER2])
    {
        /* Bump mapping is supported already in NV_TEXTURE_SHADER, but that extension does
         * not support 3D textures. This asks for trouble if an app uses both bump mapping
         * and 3D textures. It also allows us to keep the code simpler by having texture
         * shaders constantly enabled. */
        caps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAP;
        /* TODO: Luminance bump map? */
    }

#if 0
    /* FIXME: Add
            caps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE
            WINED3DTEXOPCAPS_PREMODULATE */
#endif

    caps->MaxTextureBlendStages = min(WINED3D_MAX_TEXTURES, gl_info->limits.general_combiners);
    caps->MaxSimultaneousTextures = gl_info->limits.textures;
}

static DWORD nvrc_fragment_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return GL_EXT_EMUL_ARB_MULTITEXTURE | GL_EXT_EMUL_EXT_FOG_COORD;
}

static void *nvrc_fragment_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    return shader_priv;
}

/* Context activation is done by the caller. */
static void nvrc_fragment_free(struct wined3d_device *device, struct wined3d_context *context) {}

/* Two fixed function pipeline implementations using GL_NV_register_combiners and
 * GL_NV_texture_shader. The nvts_fragment_pipeline assumes that both extensions
 * are available(geforce 3 and newer), while nvrc_fragment_pipeline uses only the
 * register combiners extension(Pre-GF3).
 */

static BOOL nvts_color_fixup_supported(struct color_fixup_desc fixup)
{
    /* We only support identity conversions. */
    return is_identity_fixup(fixup);
}

static const struct wined3d_state_entry_template nvrc_fragmentstate_template[] =
{
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        nvrc_colorop        }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_RESULT_ARG),      nvrc_resultarg      }, WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            apply_pixelshader   }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),           { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),             { STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),             nvrc_texfactor      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ALPHAFUNC),                 { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ALPHAREF),                  { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           state_alpha_test    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORKEYENABLE),            { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_COLOR_KEY,                                    { STATE_COLOR_KEY,                                    state_nop           }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGCOLOR),                  { STATE_RENDER(WINED3D_RS_FOGCOLOR),                  state_fogcolor      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGDENSITY),                { STATE_RENDER(WINED3D_RS_FOGDENSITY),                state_fogdensity    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGENABLE),                 { STATE_RENDER(WINED3D_RS_FOGENABLE),                 state_fog_fragpart  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGTABLEMODE),              { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),             { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGSTART),                  { STATE_RENDER(WINED3D_RS_FOGSTART),                  state_fogstartend   }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGEND),                    { STATE_RENDER(WINED3D_RS_FOGSTART),                  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SHADEMODE),                 { STATE_RENDER(WINED3D_RS_SHADEMODE),                 state_shademode     }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    {0 /* Terminate */,                                   { 0,                                                  0                   }, WINED3D_GL_EXT_NONE             },
};

static BOOL nvrc_context_alloc(struct wined3d_context *context)
{
    return TRUE;
}

static void nvrc_context_free(struct wined3d_context *context)
{
}


const struct wined3d_fragment_pipe_ops nvts_fragment_pipeline =
{
    nvts_enable,
    nvrc_fragment_get_caps,
    nvrc_fragment_get_emul_mask,
    nvrc_fragment_alloc,
    nvrc_fragment_free,
    nvrc_context_alloc,
    nvrc_context_free,
    nvts_color_fixup_supported,
    nvrc_fragmentstate_template,
};

const struct wined3d_fragment_pipe_ops nvrc_fragment_pipeline =
{
    nvrc_enable,
    nvrc_fragment_get_caps,
    nvrc_fragment_get_emul_mask,
    nvrc_fragment_alloc,
    nvrc_fragment_free,
    nvrc_context_alloc,
    nvrc_context_free,
    nvts_color_fixup_supported,
    nvrc_fragmentstate_template,
};
