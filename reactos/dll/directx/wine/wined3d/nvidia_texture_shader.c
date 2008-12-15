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

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define GLINFO_LOCATION stateblock->wineD3DDevice->adapter->gl_info
static void nvts_activate_dimensions(DWORD stage, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    BOOL bumpmap = FALSE;

    if(stage > 0 && (stateblock->textureState[stage - 1][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAPLUMINANCE ||
                     stateblock->textureState[stage - 1][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAP)) {
        bumpmap = TRUE;
        context->texShaderBumpMap |= (1 << stage);
    } else {
        context->texShaderBumpMap &= ~(1 << stage);
    }

    if(stateblock->textures[stage]) {
        switch(stateblock->textureDimensions[stage]) {
            case GL_TEXTURE_2D:
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, bumpmap ? GL_OFFSET_TEXTURE_2D_NV : GL_TEXTURE_2D);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, ...)");
                break;
            case GL_TEXTURE_RECTANGLE_ARB:
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, bumpmap ? GL_OFFSET_TEXTURE_2D_NV : GL_TEXTURE_RECTANGLE_ARB);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, ...)");
                break;
            case GL_TEXTURE_3D:
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D)");
                break;
            case GL_TEXTURE_CUBE_MAP_ARB:
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_CUBE_MAP_ARB)");
                break;
        }
    } else {
        glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
        checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE)");
    }
}

typedef struct {
    GLenum input[3];
    GLenum mapping[3];
    GLenum component_usage[3];
} tex_op_args;

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
            /* TODO: Support per stage constants (WINED3DTSS_CONSTANT, NV_register_combiners2) */
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

void set_tex_op_nvrc(IWineD3DDevice *iface, BOOL is_alpha, int stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3, INT texture_idx, DWORD dst) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl*)iface;
    tex_op_args tex_op_args = {{0}, {0}, {0}};
    GLenum portion = is_alpha ? GL_ALPHA : GL_RGB;
    GLenum target = GL_COMBINER0_NV + stage;
    GLenum output;
    IWineD3DStateBlockImpl *stateblock = This->stateBlock; /* For GLINFO_LOCATION */

    TRACE("stage %d, is_alpha %d, op %s, arg1 %#x, arg2 %#x, arg3 %#x, texture_idx %d\n",
          stage, is_alpha, debug_d3dtop(op), arg1, arg2, arg3, texture_idx);

    /* If a texture stage references an invalid texture unit the stage just
    * passes through the result from the previous stage */
    if (is_invalid_op(This, stage, op, arg1, arg2, arg3)) {
        arg1 = WINED3DTA_CURRENT;
        op = WINED3DTOP_SELECTARG1;
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

    /* This is called by a state handler which has the gl lock held and a context for the thread */
    switch(op)
    {
        case WINED3DTOP_DISABLE:
            /* Only for alpha */
            if (!is_alpha) ERR("Shouldn't be called for WINED3DTSS_COLOROP (WINED3DTOP_DISABLE)\n");
            /* Input, prev_alpha*1 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                       GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_SELECTARG1:
        case WINED3DTOP_SELECTARG2:
            /* Input, arg*1 */
            if (op == WINED3DTOP_SELECTARG1) {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                           tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            } else {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                           tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            }
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                       GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_MODULATE:
        case WINED3DTOP_MODULATE2X:
        case WINED3DTOP_MODULATE4X:
            /* Input, arg1*arg2 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                       tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));

            /* Output */
            if (op == WINED3DTOP_MODULATE) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                           GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_MODULATE2X) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                           GL_DISCARD_NV, GL_SCALE_BY_TWO_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_MODULATE4X) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, output, GL_DISCARD_NV,
                           GL_DISCARD_NV, GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            }
            break;

        case WINED3DTOP_ADD:
        case WINED3DTOP_ADDSIGNED:
        case WINED3DTOP_ADDSIGNED2X:
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
            if (op == WINED3DTOP_ADD) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                           output, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_ADDSIGNED) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                           output, GL_NONE, GL_BIAS_BY_NEGATIVE_ONE_HALF_NV, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_ADDSIGNED2X) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                           output, GL_SCALE_BY_TWO_NV, GL_BIAS_BY_NEGATIVE_ONE_HALF_NV, GL_FALSE, GL_FALSE, GL_FALSE));
            }
            break;

        case WINED3DTOP_SUBTRACT:
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

        case WINED3DTOP_ADDSMOOTH:
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

        case WINED3DTOP_BLENDDIFFUSEALPHA:
        case WINED3DTOP_BLENDTEXTUREALPHA:
        case WINED3DTOP_BLENDFACTORALPHA:
        case WINED3DTOP_BLENDTEXTUREALPHAPM:
        case WINED3DTOP_BLENDCURRENTALPHA:
        {
            GLenum alpha_src = GL_PRIMARY_COLOR_NV;
            if (op == WINED3DTOP_BLENDDIFFUSEALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_DIFFUSE, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDTEXTUREALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_TEXTURE, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDFACTORALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_TFACTOR, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDTEXTUREALPHAPM) alpha_src = d3dta_to_combiner_input(WINED3DTA_TEXTURE, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDCURRENTALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_CURRENT, stage, texture_idx);
            else FIXME("Unhandled WINED3DTOP %s, shouldn't happen\n", debug_d3dtop(op));

            /* Input, arg1*alpha_src+arg2*(1-alpha_src) */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                       tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            if (op == WINED3DTOP_BLENDTEXTUREALPHAPM)
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

        case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
            /* Input, arg1_alpha*arg2_rgb+arg1_rgb*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATEALPHA_ADDCOLOR)\n");
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

        case WINED3DTOP_MODULATECOLOR_ADDALPHA:
            /* Input, arg1_rgb*arg2_rgb+arg1_alpha*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATECOLOR_ADDALPHA)\n");
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

        case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
            /* Input, (1-arg1_alpha)*arg2_rgb+arg1_rgb*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATEINVALPHA_ADDCOLOR)\n");
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

        case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
            /* Input, (1-arg1_rgb)*arg2_rgb+arg1_alpha*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATEINVCOLOR_ADDALPHA)\n");
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

        case WINED3DTOP_DOTPRODUCT3:
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

        case WINED3DTOP_MULTIPLYADD:
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

        case WINED3DTOP_LERP:
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

        case WINED3DTOP_BUMPENVMAPLUMINANCE:
        case WINED3DTOP_BUMPENVMAP:
            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* The bump map stage itself isn't exciting, just read the texture. But tell the next stage to
                 * perform bump mapping and source from the current stage. Pretty much a SELECTARG2.
                 * ARG2 is passed through unmodified(apps will most likely use D3DTA_CURRENT for arg2, arg1
                 * (which will most likely be D3DTA_TEXTURE) is available as a texture shader input for the next stage
                 */
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                           tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                           GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
                /* Always pass through to CURRENT, ignore temp arg */
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                           GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
                break;
            }

        default:
            FIXME("Unhandled WINED3DTOP: stage %d, is_alpha %d, op %s (%#x), arg1 %#x, arg2 %#x, arg3 %#x, texture_idx %d\n",
                  stage, is_alpha, debug_d3dtop(op), op, arg1, arg2, arg3, texture_idx);
    }

    checkGLcall("set_tex_op_nvrc()\n");

}


static void nvrc_colorop(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[stage];
    BOOL tex_used = stateblock->wineD3DDevice->fixed_function_usage_map[stage];

    TRACE("Setting color op for stage %d\n", stage);

    if (stateblock->pixelShader && stateblock->wineD3DDevice->ps_selected_mode != SHADER_NONE &&
        ((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.function) {
        /* Using a pixel shader? Don't care for anything here, the shader applying does it */
        return;
    }

    if (stage != mapped_stage) WARN("Using non 1:1 mapping: %d -> %d!\n", stage, mapped_stage);

    if (mapped_stage != -1) {
        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
            if (tex_used && mapped_stage >= GL_LIMITS(textures)) {
                FIXME("Attempt to enable unsupported stage!\n");
                return;
            }
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
            checkGLcall("glActiveTextureARB");
        } else if (stage > 0) {
            WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            return;
        }
    }

    if(stateblock->lowest_disabled_stage > 0) {
        glEnable(GL_REGISTER_COMBINERS_NV);
        GL_EXTCALL(glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, stateblock->lowest_disabled_stage));
    } else {
        glDisable(GL_REGISTER_COMBINERS_NV);
    }
    if(stage >= stateblock->lowest_disabled_stage) {
        TRACE("Stage disabled\n");
        if (mapped_stage != -1) {
            /* Disable everything here */
            glDisable(GL_TEXTURE_2D);
            checkGLcall("glDisable(GL_TEXTURE_2D)");
            glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable(GL_TEXTURE_3D)");
            if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
            }
            if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
                glDisable(GL_TEXTURE_RECTANGLE_ARB);
                checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
            }
            if(GL_SUPPORT(NV_TEXTURE_SHADER2) && mapped_stage < GL_LIMITS(textures)) {
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
            }
        }
        /* All done */
        return;
    }

    /* The sampler will also activate the correct texture dimensions, so no need to do it here
     * if the sampler for this stage is dirty
     */
    if(!isStateDirty(context, STATE_SAMPLER(stage))) {
        if (tex_used) {
            if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
                nvts_activate_dimensions(stage, stateblock, context);
            } else {
                texture_activate_dimensions(stage, stateblock, context);
            }
        }
    }

    /* Set the texture combiners */
    set_tex_op_nvrc((IWineD3DDevice *)stateblock->wineD3DDevice, FALSE, stage,
                        stateblock->textureState[stage][WINED3DTSS_COLOROP],
                        stateblock->textureState[stage][WINED3DTSS_COLORARG1],
                        stateblock->textureState[stage][WINED3DTSS_COLORARG2],
                        stateblock->textureState[stage][WINED3DTSS_COLORARG0],
                        mapped_stage,
                        stateblock->textureState[stage][WINED3DTSS_RESULTARG]);

    /* In register combiners bump mapping is done in the stage AFTER the one that has the bump map operation set,
     * thus the texture shader may have to be updated
     */
    if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        BOOL usesBump = (stateblock->textureState[stage][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAPLUMINANCE ||
                            stateblock->textureState[stage][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAP) ? TRUE : FALSE;
        BOOL usedBump = (context->texShaderBumpMap & 1 << (stage + 1)) ? TRUE : FALSE;
        if(usesBump != usedBump) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage + 1));
            checkGLcall("glActiveTextureARB");
            nvts_activate_dimensions(stage + 1, stateblock, context);
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
            checkGLcall("glActiveTextureARB");
        }
    }
}

static void nvts_texdim(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD sampler = state - STATE_SAMPLER(0);
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[sampler];

    /* No need to enable / disable anything here for unused samplers. The tex_colorop
    * handler takes care. Also no action is needed with pixel shaders, or if tex_colorop
    * will take care of this business
    */
    if(mapped_stage == -1 || mapped_stage >= GL_LIMITS(textures)) return;
    if(sampler >= stateblock->lowest_disabled_stage) return;
    if(isStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3DTSS_COLOROP))) return;

    nvts_activate_dimensions(sampler, stateblock, context);
}

static void nvts_bumpenvmat(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[stage + 1];
    float mat[2][2];

    /* Direct3D sets the matrix in the stage reading the perturbation map. The result is used to
     * offset the destination stage(always stage + 1 in d3d). In GL_NV_texture_shader, the bump
     * map offsetting is done in the stage reading the bump mapped texture, and the perturbation
     * map is read from a specified source stage(always stage - 1 for d3d). Thus set the matrix
     * for stage + 1. Keep the nvrc tex unit mapping in mind too
     */
    if(mapped_stage < GL_LIMITS(textures)) {
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
        checkGLcall("GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage))");

        /* We can't just pass a pointer to the stateblock to GL due to the different matrix
         * format(column major vs row major)
         */
        mat[0][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT00]);
        mat[1][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT01]);
        mat[0][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT10]);
        mat[1][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT11]);
        glTexEnvfv(GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV, (float *) mat);
        checkGLcall("glTexEnvfv(GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV, mat)");
    }
}

static void nvrc_texfactor(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_TEXTUREFACTOR], col);
    GL_EXTCALL(glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, &col[0]));
}
#undef GLINFO_LOCATION

#define GLINFO_LOCATION (*gl_info)
static void nvrc_enable(IWineD3DDevice *iface, BOOL enable) { }

static void nvts_enable(IWineD3DDevice *iface, BOOL enable) {
    if(enable) {
        glEnable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glEnable(GL_TEXTURE_SHADER_NV)");
    } else {
        glDisable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glDisable(GL_TEXTURE_SHADER_NV)");
    }
}

static void nvrc_fragment_get_caps(WINED3DDEVTYPE devtype, const WineD3D_GL_Info *gl_info, struct fragment_caps *pCaps)
{
    pCaps->TextureOpCaps =  WINED3DTEXOPCAPS_ADD                        |
                            WINED3DTEXOPCAPS_ADDSIGNED                  |
                            WINED3DTEXOPCAPS_ADDSIGNED2X                |
                            WINED3DTEXOPCAPS_MODULATE                   |
                            WINED3DTEXOPCAPS_MODULATE2X                 |
                            WINED3DTEXOPCAPS_MODULATE4X                 |
                            WINED3DTEXOPCAPS_SELECTARG1                 |
                            WINED3DTEXOPCAPS_SELECTARG2                 |
                            WINED3DTEXOPCAPS_DISABLE                    |
                            WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA          |
                            WINED3DTEXOPCAPS_BLENDTEXTUREALPHA          |
                            WINED3DTEXOPCAPS_BLENDFACTORALPHA           |
                            WINED3DTEXOPCAPS_BLENDCURRENTALPHA          |
                            WINED3DTEXOPCAPS_LERP                       |
                            WINED3DTEXOPCAPS_SUBTRACT                   |
                            WINED3DTEXOPCAPS_ADDSMOOTH                  |
                            WINED3DTEXOPCAPS_MULTIPLYADD                |
                            WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR     |
                            WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA     |
                            WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM        |
                            WINED3DTEXOPCAPS_DOTPRODUCT3                |
                            WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR  |
                            WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA;

    if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        /* Bump mapping is supported already in NV_TEXTURE_SHADER, but that extension does
         * not support 3D textures. This asks for trouble if an app uses both bump mapping
         * and 3D textures. It also allows us to keep the code simpler by having texture
         * shaders constantly enabled.
         */
        pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAP;
        /* TODO: Luminance bump map? */
    }

#if 0
    /* FIXME: Add
            pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE
            WINED3DTEXOPCAPS_PREMODULATE */
#endif

    pCaps->MaxTextureBlendStages   = GL_LIMITS(texture_stages);
    pCaps->MaxSimultaneousTextures = GL_LIMITS(textures);

    pCaps->PrimitiveMiscCaps |=  WINED3DPMISCCAPS_TSSARGTEMP;

    /* The caps below can be supported but aren't handled yet in utils.c 'd3dta_to_combiner_input', disable them until support is fixed */
#if 0
    if (GL_SUPPORT(NV_REGISTER_COMBINERS2))
    pCaps->PrimitiveMiscCaps |=  WINED3DPMISCCAPS_PERSTAGECONSTANT;
#endif
}

static HRESULT nvrc_fragment_alloc(IWineD3DDevice *iface) { return WINED3D_OK; }
static void nvrc_fragment_free(IWineD3DDevice *iface) {}

/* Two fixed function pipeline implementations using GL_NV_register_combiners and
 * GL_NV_texture_shader. The nvts_fragment_pipeline assumes that both extensions
 * are available(geforce 3 and newer), while nvrc_fragment_pipeline uses only the
 * register combiners extension(Pre-GF3).
 */

static BOOL nvts_color_fixup_supported(struct color_fixup_desc fixup)
{
    if (TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    /* We only support identity conversions. */
    if (is_identity_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    TRACE("[FAILED]\n");
    return FALSE;
}

static const struct StateEntryTemplate nvrc_fragmentstate_template[] = {
    { STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(0, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(1, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(2, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(3, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(4, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(5, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(6, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          { STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG1),        { STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG2),        { STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG1),        { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG2),        { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT01),     { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT10),     { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT11),     { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     nvts_bumpenvmat     }, NV_TEXTURE_SHADER2              },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG0),        { STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG0),        { STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         }, 0                               },
    { STATE_TEXTURESTAGE(7, WINED3DTSS_RESULTARG),        { STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          nvrc_colorop        }, 0                               },
    { STATE_PIXELSHADER,                                  { STATE_PIXELSHADER,                                  apply_pixelshader   }, 0                               },
    { STATE_RENDER(WINED3DRS_SRGBWRITEENABLE),            { STATE_PIXELSHADER,                                  apply_pixelshader   }, 0                               },
    { STATE_RENDER(WINED3DRS_TEXTUREFACTOR),              { STATE_RENDER(WINED3DRS_TEXTUREFACTOR),              nvrc_texfactor      }, 0                               },
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   sampler_texdim      }, 0                               },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   sampler_texdim      }, 0                               },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   sampler_texdim      }, 0                               },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   sampler_texdim      }, 0                               },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   sampler_texdim      }, 0                               },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   sampler_texdim      }, 0                               },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   sampler_texdim      }, 0                               },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   nvts_texdim         }, NV_TEXTURE_SHADER2              },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   sampler_texdim      }, 0                               },
    {0 /* Terminate */,                                   { 0,                                                  0                   }, 0                               },
};

const struct fragment_pipeline nvts_fragment_pipeline = {
    nvts_enable,
    nvrc_fragment_get_caps,
    nvrc_fragment_alloc,
    nvrc_fragment_free,
    nvts_color_fixup_supported,
    nvrc_fragmentstate_template,
    FALSE /* we cannot disable projected textures. The vertex pipe has to do it */
};

const struct fragment_pipeline nvrc_fragment_pipeline = {
    nvrc_enable,
    nvrc_fragment_get_caps,
    nvrc_fragment_alloc,
    nvrc_fragment_free,
    nvts_color_fixup_supported,
    nvrc_fragmentstate_template,
    FALSE /* we cannot disable projected textures. The vertex pipe has to do it */
};
