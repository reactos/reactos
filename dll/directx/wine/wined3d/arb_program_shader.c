/*
 * Pixel and vertex shaders implementation using ARB_vertex_program
 * and ARB_fragment_program GL extensions.
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2006 Jason Green
 * Copyright 2006 Henri Verbeet
 * Copyright 2007-2011, 2013-2014 Stefan Dösinger for CodeWeavers
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

#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_constants);
WINE_DECLARE_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

static BOOL shader_is_pshader_version(enum wined3d_shader_type type)
{
    return type == WINED3D_SHADER_TYPE_PIXEL;
}

static BOOL shader_is_vshader_version(enum wined3d_shader_type type)
{
    return type == WINED3D_SHADER_TYPE_VERTEX;
}

static const char *get_line(const char **ptr)
{
    const char *p, *q;

    p = *ptr;
    if (!(q = strstr(p, "\n")))
    {
        if (!*p) return NULL;
        *ptr += strlen(p);
        return p;
    }
    *ptr = q + 1;

    return p;
}

enum arb_helper_value
{
    ARB_ZERO,
    ARB_ONE,
    ARB_TWO,
    ARB_0001,
    ARB_EPS,

    ARB_VS_REL_OFFSET
};

static const char *arb_get_helper_value(enum wined3d_shader_type shader, enum arb_helper_value value)
{
    if (shader != WINED3D_SHADER_TYPE_VERTEX && shader != WINED3D_SHADER_TYPE_PIXEL)
    {
        ERR("Unsupported shader type '%s'.\n", debug_shader_type(shader));
        return "bad";
    }

    if (shader == WINED3D_SHADER_TYPE_PIXEL)
    {
        switch (value)
        {
            case ARB_ZERO: return "ps_helper_const.x";
            case ARB_ONE: return "ps_helper_const.y";
            case ARB_TWO: return "coefmul.x";
            case ARB_0001: return "ps_helper_const.xxxy";
            case ARB_EPS: return "ps_helper_const.z";
            default: break;
        }
    }
    else
    {
        switch (value)
        {
            case ARB_ZERO: return "helper_const.x";
            case ARB_ONE: return "helper_const.y";
            case ARB_TWO: return "helper_const.z";
            case ARB_EPS: return "helper_const.w";
            case ARB_0001: return "helper_const.xxxy";
            case ARB_VS_REL_OFFSET: return "rel_addr_const.y";
        }
    }
    FIXME("Unmanaged %s shader helper constant requested: %u.\n",
          shader == WINED3D_SHADER_TYPE_PIXEL ? "pixel" : "vertex", value);
    switch (value)
    {
        case ARB_ZERO: return "0.0";
        case ARB_ONE: return "1.0";
        case ARB_TWO: return "2.0";
        case ARB_0001: return "{0.0, 0.0, 0.0, 1.0}";
        case ARB_EPS: return "1e-8";
        default: return "bad";
    }
}

static inline BOOL ffp_clip_emul(const struct wined3d_context *context)
{
    return context->lowest_disabled_stage < 7;
}

/* ARB_program_shader private data */

struct control_frame
{
    struct                          list entry;
    enum
    {
        IF,
        IFC,
        LOOP,
        REP
    } type;
    BOOL                            muting;
    BOOL                            outer_loop;
    union
    {
        unsigned int                loop;
        unsigned int                ifc;
    } no;
    struct wined3d_shader_loop_control loop_control;
    BOOL                            had_else;
};

struct arb_ps_np2fixup_info
{
    struct ps_np2fixup_info         super;
    /* For ARB we need an offset value:
     * With both GLSL and ARB mode the NP2 fixup information (the texture dimensions) are stored in a
     * consecutive way (GLSL uses a uniform array). Since ARB doesn't know the notion of a "standalone"
     * array we need an offset to the index inside the program local parameter array. */
    UINT                            offset;
};

struct arb_ps_compile_args
{
    struct ps_compile_args          super;
    WORD                            bools;
    WORD                            clip;  /* only a boolean, use a WORD for alignment */
    unsigned char                   loop_ctrl[WINED3D_MAX_CONSTS_I][3];
};

struct stb_const_desc
{
    unsigned char           texunit;
    UINT                    const_num;
};

struct arb_ps_compiled_shader
{
    struct arb_ps_compile_args      args;
    struct arb_ps_np2fixup_info     np2fixup_info;
    struct stb_const_desc           bumpenvmatconst[WINED3D_MAX_TEXTURES];
    struct stb_const_desc           luminanceconst[WINED3D_MAX_TEXTURES];
    UINT                            int_consts[WINED3D_MAX_CONSTS_I];
    GLuint                          prgId;
    UINT                            ycorrection;
    unsigned char                   numbumpenvmatconsts;
    char                            num_int_consts;
};

struct arb_vs_compile_args
{
    struct vs_compile_args          super;
    union
    {
        struct
        {
            WORD                    bools;
            unsigned char           clip_texcoord;
            unsigned char           clipplane_mask;
        }                           boolclip;
        DWORD                       boolclip_compare;
    } clip;
    DWORD                           ps_signature;
    union
    {
        unsigned char               samplers[4];
        DWORD                       samplers_compare;
    } vertex;
    unsigned char                   loop_ctrl[WINED3D_MAX_CONSTS_I][3];
};

struct arb_vs_compiled_shader
{
    struct arb_vs_compile_args      args;
    GLuint                          prgId;
    UINT                            int_consts[WINED3D_MAX_CONSTS_I];
    char                            num_int_consts;
    char                            need_color_unclamp;
    UINT                            pos_fixup;
};

struct recorded_instruction
{
    struct wined3d_shader_instruction ins;
    struct list entry;
};

struct shader_arb_ctx_priv
{
    char addr_reg[50];
    enum
    {
        /* plain GL_ARB_vertex_program or GL_ARB_fragment_program */
        ARB,
        /* GL_NV_vertex_program2_option or GL_NV_fragment_program_option */
        NV2,
        /* GL_NV_vertex_program3 or GL_NV_fragment_program2 */
        NV3
    } target_version;

    const struct arb_vs_compile_args    *cur_vs_args;
    const struct arb_ps_compile_args    *cur_ps_args;
    const struct arb_ps_compiled_shader *compiled_fprog;
    const struct arb_vs_compiled_shader *compiled_vprog;
    struct arb_ps_np2fixup_info         *cur_np2fixup_info;
    struct list                         control_frames;
    struct list                         record;
    BOOL                                recording;
    BOOL                                muted;
    unsigned int                        num_loops, loop_depth, num_ifcs;
    int                                 aL;
    BOOL                                ps_post_process;

    unsigned int                        vs_clipplanes;
    BOOL                                footer_written;
    BOOL                                in_main_func;

    /* For 3.0 vertex shaders */
    const char                          *vs_output[MAX_REG_OUTPUT];
    /* For 2.x and earlier vertex shaders */
    const char                          *texcrd_output[8], *color_output[2], *fog_output;

    /* 3.0 pshader input for compatibility with fixed function */
    const char                          *ps_input[MAX_REG_INPUT];
};

struct ps_signature
{
    struct wined3d_shader_signature sig;
    DWORD                               idx;
    struct wine_rb_entry                entry;
};

struct arb_pshader_private {
    struct arb_ps_compiled_shader   *gl_shaders;
    UINT                            num_gl_shaders, shader_array_size;
    DWORD                           input_signature_idx;
    DWORD                           clipplane_emulation;
    BOOL                            clamp_consts;
};

struct arb_vshader_private {
    struct arb_vs_compiled_shader   *gl_shaders;
    UINT                            num_gl_shaders, shader_array_size;
    UINT rel_offset;
};

struct shader_arb_priv
{
    GLuint                  current_vprogram_id;
    GLuint                  current_fprogram_id;
    const struct arb_ps_compiled_shader *compiled_fprog;
    const struct arb_vs_compiled_shader *compiled_vprog;
    BOOL                    use_arbfp_fixed_func;
    struct wine_rb_tree     fragment_shaders;
    BOOL                    last_ps_const_clamped;
    BOOL                    last_vs_color_unclamp;

    struct wine_rb_tree     signature_tree;
    DWORD ps_sig_number;

    unsigned int highest_dirty_ps_const, highest_dirty_vs_const;
    char vshader_const_dirty[WINED3D_MAX_VS_CONSTS_F];
    char pshader_const_dirty[WINED3D_MAX_PS_CONSTS_F];
    const struct wined3d_context *last_context;

    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct wined3d_fragment_pipe_ops *fragment_pipe;
    BOOL ffp_proj_control;
};

/* Context activation for state handlers is done by the caller. */

static BOOL need_rel_addr_const(const struct arb_vshader_private *shader_data,
        const struct wined3d_shader_reg_maps *reg_maps, const struct wined3d_gl_info *gl_info)
{
    if (shader_data->rel_offset) return TRUE;
    if (!reg_maps->usesmova) return FALSE;
    return !gl_info->supported[NV_VERTEX_PROGRAM2_OPTION];
}

/* Returns TRUE if result.clip from GL_NV_vertex_program2 should be used and FALSE otherwise */
static inline BOOL use_nv_clip(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[NV_VERTEX_PROGRAM2_OPTION]
            && !(gl_info->quirks & WINED3D_QUIRK_NV_CLIP_BROKEN);
}

static BOOL need_helper_const(const struct arb_vshader_private *shader_data,
        const struct wined3d_shader_reg_maps *reg_maps, const struct wined3d_gl_info *gl_info)
{
    if (need_rel_addr_const(shader_data, reg_maps, gl_info)) return TRUE;
    if (!gl_info->supported[NV_VERTEX_PROGRAM]) return TRUE; /* Need to init colors. */
    if (gl_info->quirks & WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT) return TRUE; /* Load the immval offset. */
    if (gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W) return TRUE; /* Have to init texcoords. */
    if (!use_nv_clip(gl_info)) return TRUE; /* Init the clip texcoord */
    if (reg_maps->usesnrm) return TRUE; /* 0.0 */
    if (reg_maps->usespow) return TRUE; /* EPS, 0.0 and 1.0 */
    if (reg_maps->fog) return TRUE; /* Clamping fog coord, 0.0 and 1.0 */
    return FALSE;
}

static unsigned int reserved_vs_const(const struct arb_vshader_private *shader_data,
        const struct wined3d_shader_reg_maps *reg_maps, const struct wined3d_gl_info *gl_info)
{
    unsigned int ret = 1;
    /* We use one PARAM for the pos fixup, and in some cases one to load
     * some immediate values into the shader. */
    if (need_helper_const(shader_data, reg_maps, gl_info)) ++ret;
    if (need_rel_addr_const(shader_data, reg_maps, gl_info)) ++ret;
    return ret;
}

/* Loads floating point constants into the currently set ARB_vertex/fragment_program.
 * When constant_list == NULL, it will load all the constants.
 *
 * @target_type should be either GL_VERTEX_PROGRAM_ARB (for vertex shaders)
 *  or GL_FRAGMENT_PROGRAM_ARB (for pixel shaders)
 */
/* Context activation is done by the caller. */
static unsigned int shader_arb_load_constants_f(const struct wined3d_shader *shader,
        const struct wined3d_gl_info *gl_info, GLuint target_type, unsigned int max_constants,
        const struct wined3d_vec4 *constants, char *dirty_consts)
{
    struct wined3d_shader_lconst *lconst;
    unsigned int ret, i, j;

    if (TRACE_ON(d3d_constants))
    {
        for (i = 0; i < max_constants; ++i)
        {
            if (!dirty_consts[i])
                continue;
            TRACE_(d3d_constants)("Loading constant %u: %s.\n", i, debug_vec4(&constants[i]));
        }
    }

    i = 0;

    /* In 1.X pixel shaders constants are implicitly clamped in the range [-1;1] */
    if (target_type == GL_FRAGMENT_PROGRAM_ARB && shader->reg_maps.shader_version.major == 1)
    {
        float lcl_const[4];
        /* ps 1.x supports only 8 constants, clamp only those. When switching between 1.x and higher
         * shaders, the first 8 constants are marked dirty for reload
         */
        for (; i < min(8, max_constants); ++i)
        {
            if (!dirty_consts[i])
                continue;
            dirty_consts[i] = 0;

            if (constants[i].x > 1.0f)
                lcl_const[0] = 1.0f;
            else if (constants[i].x < -1.0f)
                lcl_const[0] = -1.0f;
            else
                lcl_const[0] = constants[i].x;

            if (constants[i].y > 1.0f)
                lcl_const[1] = 1.0f;
            else if (constants[i].y < -1.0f)
                lcl_const[1] = -1.0f;
            else
                lcl_const[1] = constants[i].y;

            if (constants[i].z > 1.0f)
                lcl_const[2] = 1.0f;
            else if (constants[i].z < -1.0f)
                lcl_const[2] = -1.0f;
            else
                lcl_const[2] = constants[i].z;

            if (constants[i].w > 1.0f)
                lcl_const[3] = 1.0f;
            else if (constants[i].w < -1.0f)
                lcl_const[3] = -1.0f;
            else
                lcl_const[3] = constants[i].w;

            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, lcl_const));
        }

        /* If further constants are dirty, reload them without clamping.
         *
         * The alternative is not to touch them, but then we cannot reset the dirty constant count
         * to zero. That's bad for apps that only use PS 1.x shaders, because in that case the code
         * above would always re-check the first 8 constants since max_constant remains at the init
         * value
         */
    }

    if (gl_info->supported[EXT_GPU_PROGRAM_PARAMETERS])
    {
        /* TODO: Benchmark if we're better of with finding the dirty constants ourselves,
         * or just reloading *all* constants at once
         *
        GL_EXTCALL(glProgramEnvParameters4fvEXT(target_type, i, max_constants, constants + (i * 4)));
         */
        for (; i < max_constants; ++i)
        {
            if (!dirty_consts[i])
                continue;

            /* Find the next block of dirty constants */
            dirty_consts[i] = 0;
            j = i;
            for (++i; (i < max_constants) && dirty_consts[i]; ++i)
            {
                dirty_consts[i] = 0;
            }

            GL_EXTCALL(glProgramEnvParameters4fvEXT(target_type, j, i - j, &constants[j].x));
        }
    }
    else
    {
        for (; i < max_constants; ++i)
        {
            if (dirty_consts[i])
            {
                dirty_consts[i] = 0;
                GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, &constants[i].x));
            }
        }
    }
    checkGLcall("glProgramEnvParameter4fvARB()");

    /* Load immediate constants */
    if (shader->load_local_constsF)
    {
        if (TRACE_ON(d3d_shader))
        {
            LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
            {
                GLfloat* values = (GLfloat*)lconst->value;
                TRACE_(d3d_constants)("Loading local constants %i: %f, %f, %f, %f\n", lconst->idx,
                        values[0], values[1], values[2], values[3]);
            }
        }
        /* Immediate constants are clamped for 1.X shaders at loading times */
        ret = 0;
        LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
        {
            dirty_consts[lconst->idx] = 1; /* Dirtify so the non-immediate constant overwrites it next time */
            ret = max(ret, lconst->idx + 1);
            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, lconst->idx, (GLfloat*)lconst->value));
        }
        checkGLcall("glProgramEnvParameter4fvARB()");
        return ret; /* The loaded immediate constants need reloading for the next shader */
    } else {
        return 0; /* No constants are dirty now */
    }
}

/* Loads the texture dimensions for NP2 fixup into the currently set
 * ARB_[vertex/fragment]_programs. */
static void shader_arb_load_np2fixup_constants(const struct arb_ps_np2fixup_info *fixup,
        const struct wined3d_gl_info *gl_info, const struct wined3d_state *state)
{
    GLfloat np2fixup_constants[4 * WINED3D_MAX_FRAGMENT_SAMPLERS];
    WORD active = fixup->super.active;
    UINT i;

    if (!active)
        return;

    for (i = 0; active; active >>= 1, ++i)
    {
        const struct wined3d_texture *tex = state->textures[i];
        unsigned char idx = fixup->super.idx[i];
        GLfloat *tex_dim = &np2fixup_constants[(idx >> 1) * 4];

        if (!(active & 1))
            continue;

        if (!tex)
        {
            ERR("Nonexistent texture is flagged for NP2 texcoord fixup.\n");
            continue;
        }

        if (idx % 2)
        {
            tex_dim[2] = tex->pow2_matrix[0];
            tex_dim[3] = tex->pow2_matrix[5];
        }
        else
        {
            tex_dim[0] = tex->pow2_matrix[0];
            tex_dim[1] = tex->pow2_matrix[5];
        }
    }

    for (i = 0; i < fixup->super.num_consts; ++i)
    {
        GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,
                fixup->offset + i, &np2fixup_constants[i * 4]));
    }
}

/* Context activation is done by the caller. */
static void shader_arb_ps_local_constants(const struct arb_ps_compiled_shader *gl_shader,
        const struct wined3d_context_gl *context_gl, const struct wined3d_state *state, unsigned int rt_height)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned char i;

    for(i = 0; i < gl_shader->numbumpenvmatconsts; i++)
    {
        int texunit = gl_shader->bumpenvmatconst[i].texunit;

        /* The state manager takes care that this function is always called if the bump env matrix changes */
        const float *data = (const float *)&state->texture_states[texunit][WINED3D_TSS_BUMPENV_MAT00];
        GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,
                gl_shader->bumpenvmatconst[i].const_num, data));

        if (gl_shader->luminanceconst[i].const_num != WINED3D_CONST_NUM_UNUSED)
        {
            /* WINED3D_TSS_BUMPENVLSCALE and WINED3D_TSS_BUMPENVLOFFSET are next to each other.
             * point gl to the scale, and load 4 floats. x = scale, y = offset, z and w are junk, we
             * don't care about them. The pointers are valid for sure because the stateblock is bigger.
             * (they're WINED3D_TSS_TEXTURETRANSFORMFLAGS and WINED3D_TSS_ADDRESSW, so most likely 0 or NaN
            */
            const float *scale = (const float *)&state->texture_states[texunit][WINED3D_TSS_BUMPENV_LSCALE];
            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,
                    gl_shader->luminanceconst[i].const_num, scale));
        }
    }
    checkGLcall("Load bumpmap consts");

    if(gl_shader->ycorrection != WINED3D_CONST_NUM_UNUSED)
    {
        /* ycorrection.x: Backbuffer height(onscreen) or 0(offscreen).
        * ycorrection.y: -1.0(onscreen), 1.0(offscreen)
        * ycorrection.z: 1.0
        * ycorrection.w: 0.0
        */
        float val[4];
        val[0] = context_gl->c.render_offscreen ? 0.0f : (float)rt_height;
        val[1] = context_gl->c.render_offscreen ? 1.0f : -1.0f;
        val[2] = 1.0f;
        val[3] = 0.0f;
        GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader->ycorrection, val));
        checkGLcall("y correction loading");
    }

    if (!gl_shader->num_int_consts) return;

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        if(gl_shader->int_consts[i] != WINED3D_CONST_NUM_UNUSED)
        {
            float val[4];
            val[0] = (float)state->ps_consts_i[i].x;
            val[1] = (float)state->ps_consts_i[i].y;
            val[2] = (float)state->ps_consts_i[i].z;
            val[3] = -1.0f;

            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader->int_consts[i], val));
        }
    }
    checkGLcall("Load ps int consts");
}

/* Context activation is done by the caller. */
static void shader_arb_vs_local_constants(const struct arb_vs_compiled_shader *gl_shader,
        const struct wined3d_context_gl *context_gl, const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    float position_fixup[4];
    unsigned char i;

    /* Upload the position fixup */
    shader_get_position_fixup(&context_gl->c, state, 1, position_fixup);
    GL_EXTCALL(glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, gl_shader->pos_fixup, position_fixup));

    if (!gl_shader->num_int_consts) return;

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        if(gl_shader->int_consts[i] != WINED3D_CONST_NUM_UNUSED)
        {
            float val[4];
            val[0] = (float)state->vs_consts_i[i].x;
            val[1] = (float)state->vs_consts_i[i].y;
            val[2] = (float)state->vs_consts_i[i].z;
            val[3] = -1.0f;

            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, gl_shader->int_consts[i], val));
        }
    }
    checkGLcall("Load vs int consts");
}

static void shader_arb_select(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state);

/**
 * Loads the app-supplied constants into the currently set ARB_[vertex/fragment]_programs.
 *
 * We only support float constants in ARB at the moment, so don't
 * worry about the Integers or Booleans
 */
/* Context activation is done by the caller (state handler). */
static void shader_arb_load_constants_internal(struct shader_arb_priv *priv, struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, BOOL use_ps, BOOL use_vs, BOOL from_shader_select)
{
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (!from_shader_select)
    {
        const struct wined3d_shader *vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];
        const struct wined3d_shader *pshader = state->shader[WINED3D_SHADER_TYPE_PIXEL];

        if (vshader
                && (vshader->reg_maps.boolean_constants
                || (!gl_info->supported[NV_VERTEX_PROGRAM2_OPTION]
                && (vshader->reg_maps.integer_constants & ~vshader->reg_maps.local_int_consts))))
        {
            TRACE("bool/integer vertex shader constants potentially modified, forcing shader reselection.\n");
            shader_arb_select(priv, &context_gl->c, state);
        }
        else if (pshader
                && (pshader->reg_maps.boolean_constants
                || (!gl_info->supported[NV_FRAGMENT_PROGRAM_OPTION]
                && (pshader->reg_maps.integer_constants & ~pshader->reg_maps.local_int_consts))))
        {
            TRACE("bool/integer pixel shader constants potentially modified, forcing shader reselection.\n");
            shader_arb_select(priv, &context_gl->c, state);
        }
    }

    if (&context_gl->c != priv->last_context)
    {
        memset(priv->vshader_const_dirty, 1,
                sizeof(*priv->vshader_const_dirty) * d3d_info->limits.vs_uniform_count);
        priv->highest_dirty_vs_const = d3d_info->limits.vs_uniform_count;

        memset(priv->pshader_const_dirty, 1,
                sizeof(*priv->pshader_const_dirty) * d3d_info->limits.ps_uniform_count);
        priv->highest_dirty_ps_const = d3d_info->limits.ps_uniform_count;

        priv->last_context = &context_gl->c;
    }

    if (use_vs)
    {
        const struct wined3d_shader *vshader = state->shader[WINED3D_SHADER_TYPE_VERTEX];
        const struct arb_vs_compiled_shader *gl_shader = priv->compiled_vprog;

        /* Load DirectX 9 float constants for vertex shader */
        priv->highest_dirty_vs_const = shader_arb_load_constants_f(vshader, gl_info, GL_VERTEX_PROGRAM_ARB,
                priv->highest_dirty_vs_const, state->vs_consts_f, priv->vshader_const_dirty);
        shader_arb_vs_local_constants(gl_shader, context_gl, state);
    }

    if (use_ps)
    {
        const struct wined3d_shader *pshader = state->shader[WINED3D_SHADER_TYPE_PIXEL];
        const struct arb_ps_compiled_shader *gl_shader = priv->compiled_fprog;
        UINT rt_height = state->fb->render_targets[0]->height;

        /* Load DirectX 9 float constants for pixel shader */
        priv->highest_dirty_ps_const = shader_arb_load_constants_f(pshader, gl_info, GL_FRAGMENT_PROGRAM_ARB,
                priv->highest_dirty_ps_const, state->ps_consts_f, priv->pshader_const_dirty);
        shader_arb_ps_local_constants(gl_shader, context_gl, state, rt_height);

        if (context_gl->c.constant_update_mask & WINED3D_SHADER_CONST_PS_NP2_FIXUP)
            shader_arb_load_np2fixup_constants(&gl_shader->np2fixup_info, gl_info, state);
    }
}

static void shader_arb_load_constants(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    shader_arb_load_constants_internal(shader_priv, wined3d_context_gl(context),
            state, use_ps(state), use_vs(state), FALSE);
}

static void shader_arb_update_float_vertex_constants(struct wined3d_device *device, UINT start, UINT count)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl_get_current();
    struct shader_arb_priv *priv = device->shader_priv;

    /* We don't want shader constant dirtification to be an O(contexts), so just dirtify the active
     * context. On a context switch the old context will be fully dirtified */
    if (!context_gl || context_gl->c.device != device)
        return;

    memset(priv->vshader_const_dirty + start, 1, sizeof(*priv->vshader_const_dirty) * count);
    priv->highest_dirty_vs_const = max(priv->highest_dirty_vs_const, start + count);
}

static void shader_arb_update_float_pixel_constants(struct wined3d_device *device, UINT start, UINT count)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl_get_current();
    struct shader_arb_priv *priv = device->shader_priv;

    /* We don't want shader constant dirtification to be an O(contexts), so just dirtify the active
     * context. On a context switch the old context will be fully dirtified */
    if (!context_gl || context_gl->c.device != device)
        return;

    memset(priv->pshader_const_dirty + start, 1, sizeof(*priv->pshader_const_dirty) * count);
    priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, start + count);
}

static void shader_arb_append_imm_vec4(struct wined3d_string_buffer *buffer, const float *values)
{
    char str[4][17];

    wined3d_ftoa(values[0], str[0]);
    wined3d_ftoa(values[1], str[1]);
    wined3d_ftoa(values[2], str[2]);
    wined3d_ftoa(values[3], str[3]);
    shader_addline(buffer, "{%s, %s, %s, %s}", str[0], str[1], str[2], str[3]);
}

/* Generate the variable & register declarations for the ARB_vertex_program output target */
static void shader_generate_arb_declarations(const struct wined3d_shader *shader,
        const struct wined3d_shader_reg_maps *reg_maps, struct wined3d_string_buffer *buffer,
        const struct wined3d_gl_info *gl_info, DWORD *num_clipplanes,
        const struct shader_arb_ctx_priv *ctx)
{
    DWORD i;
    char pshader = shader_is_pshader_version(reg_maps->shader_version.type);
    const struct wined3d_shader_lconst *lconst;
    unsigned max_constantsF;
    DWORD map;

    /* In pixel shaders, all private constants are program local, we don't need anything
     * from program.env. Thus we can advertise the full set of constants in pixel shaders.
     * If we need a private constant the GL implementation will squeeze it in somewhere
     *
     * With vertex shaders we need the posFixup and on some GL implementations 4 helper
     * immediate values. The posFixup is loaded using program.env for now, so always
     * subtract one from the number of constants. If the shader uses indirect addressing,
     * account for the helper const too because we have to declare all available d3d constants
     * and don't know which are actually used.
     */
    if (pshader)
    {
        max_constantsF = gl_info->limits.arb_ps_native_constants;
        /* 24 is the minimum MAX_PROGRAM_ENV_PARAMETERS_ARB value. */
        if (max_constantsF < 24)
            max_constantsF = gl_info->limits.arb_ps_float_constants;
    }
    else
    {
        const struct arb_vshader_private *shader_data = shader->backend_data;
        max_constantsF = gl_info->limits.arb_vs_native_constants;
        /* 96 is the minimum MAX_PROGRAM_ENV_PARAMETERS_ARB value.
         * Also prevents max_constantsF from becoming less than 0 and
         * wrapping . */
        if (max_constantsF < 96)
            max_constantsF = gl_info->limits.arb_vs_float_constants;

        if (reg_maps->usesrelconstF)
        {
            DWORD highest_constf = 0, clip_limit;

            max_constantsF -= reserved_vs_const(shader_data, reg_maps, gl_info);
            max_constantsF -= wined3d_popcount(reg_maps->integer_constants);
            max_constantsF -= gl_info->reserved_arb_constants;

            for (i = 0; i < shader->limits->constant_float; ++i)
            {
                DWORD idx = i >> 5;
                DWORD shift = i & 0x1f;
                if (reg_maps->constf[idx] & (1u << shift))
                    highest_constf = i;
            }

            if(use_nv_clip(gl_info) && ctx->target_version >= NV2)
            {
                if(ctx->cur_vs_args->super.clip_enabled)
                    clip_limit = gl_info->limits.user_clip_distances;
                else
                    clip_limit = 0;
            }
            else
            {
                unsigned int mask = ctx->cur_vs_args->clip.boolclip.clipplane_mask;
                clip_limit = min(wined3d_popcount(mask), 4);
            }
            *num_clipplanes = min(clip_limit, max_constantsF - highest_constf - 1);
            max_constantsF -= *num_clipplanes;
            if(*num_clipplanes < clip_limit)
            {
                WARN("Only %u clip planes out of %u enabled.\n", *num_clipplanes,
                        gl_info->limits.user_clip_distances);
            }
        }
        else
        {
            if (ctx->target_version >= NV2)
                *num_clipplanes = gl_info->limits.user_clip_distances;
            else
                *num_clipplanes = min(gl_info->limits.user_clip_distances, 4);
        }
    }

    for (i = 0, map = reg_maps->temporary; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "TEMP R%u;\n", i);
    }

    for (i = 0, map = reg_maps->address; map; map >>= 1, ++i)
    {
        if (map & 1) shader_addline(buffer, "ADDRESS A%u;\n", i);
    }

    if (pshader && reg_maps->shader_version.major == 1 && reg_maps->shader_version.minor <= 3)
    {
        for (i = 0, map = reg_maps->texcoord; map; map >>= 1, ++i)
        {
            if (map & 1) shader_addline(buffer, "TEMP T%u;\n", i);
        }
    }

    if (!shader->load_local_constsF)
    {
        LIST_FOR_EACH_ENTRY(lconst, &shader->constantsF, struct wined3d_shader_lconst, entry)
        {
            const float *value;
            value = (const float *)lconst->value;
            shader_addline(buffer, "PARAM C%u = ", lconst->idx);
            shader_arb_append_imm_vec4(buffer, value);
            shader_addline(buffer, ";\n");
        }
    }

    /* After subtracting privately used constants from the hardware limit(they are loaded as
     * local constants), make sure the shader doesn't violate the env constant limit
     */
    if (pshader)
    {
        max_constantsF = min(max_constantsF, gl_info->limits.arb_ps_float_constants);
    }
    else
    {
        max_constantsF = min(max_constantsF, gl_info->limits.arb_vs_float_constants);
    }

    /* Avoid declaring more constants than needed */
    max_constantsF = min(max_constantsF, shader->limits->constant_float);

    /* we use the array-based constants array if the local constants are marked for loading,
     * because then we use indirect addressing, or when the local constant list is empty,
     * because then we don't know if we're using indirect addressing or not. If we're hardcoding
     * local constants do not declare the loaded constants as an array because ARB compilers usually
     * do not optimize unused constants away
     */
    if (reg_maps->usesrelconstF)
    {
        /* Need to PARAM the environment parameters (constants) so we can use relative addressing */
        shader_addline(buffer, "PARAM C[%d] = { program.env[0..%d] };\n",
                    max_constantsF, max_constantsF - 1);
    }
    else
    {
        for (i = 0; i < max_constantsF; ++i)
        {
            if (!shader_constant_is_local(shader, i) && wined3d_extract_bits(reg_maps->constf, i, 1))
            {
                shader_addline(buffer, "PARAM C%d = program.env[%d];\n",i, i);
            }
        }
    }
}

static const char * const shift_tab[] = {
    "dummy",     /*  0 (none) */
    "coefmul.x", /*  1 (x2)   */
    "coefmul.y", /*  2 (x4)   */
    "coefmul.z", /*  3 (x8)   */
    "coefmul.w", /*  4 (x16)  */
    "dummy",     /*  5 (x32)  */
    "dummy",     /*  6 (x64)  */
    "dummy",     /*  7 (x128) */
    "dummy",     /*  8 (d256) */
    "dummy",     /*  9 (d128) */
    "dummy",     /* 10 (d64)  */
    "dummy",     /* 11 (d32)  */
    "coefdiv.w", /* 12 (d16)  */
    "coefdiv.z", /* 13 (d8)   */
    "coefdiv.y", /* 14 (d4)   */
    "coefdiv.x"  /* 15 (d2)   */
};

static void shader_arb_get_write_mask(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_dst_param *dst, char *write_mask)
{
    char *ptr = write_mask;

    if (dst->write_mask != WINED3DSP_WRITEMASK_ALL)
    {
        *ptr++ = '.';
        if (dst->write_mask & WINED3DSP_WRITEMASK_0) *ptr++ = 'x';
        if (dst->write_mask & WINED3DSP_WRITEMASK_1) *ptr++ = 'y';
        if (dst->write_mask & WINED3DSP_WRITEMASK_2) *ptr++ = 'z';
        if (dst->write_mask & WINED3DSP_WRITEMASK_3) *ptr++ = 'w';
    }

    *ptr = '\0';
}

static void shader_arb_get_swizzle(const struct wined3d_shader_src_param *param, BOOL fixup, char *swizzle_str)
{
    /* For registers of type WINED3DDECLTYPE_D3DCOLOR, data is stored as "bgra",
     * but addressed as "rgba". To fix this we need to swap the register's x
     * and z components. */
    const char *swizzle_chars = fixup ? "zyxw" : "xyzw";
    char *ptr = swizzle_str;

    /* swizzle bits fields: wwzzyyxx */
    DWORD swizzle = param->swizzle;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;

    /* If the swizzle is the default swizzle (ie, "xyzw"), we don't need to
     * generate a swizzle string. Unless we need to our own swizzling. */
    if (swizzle != WINED3DSP_NOSWIZZLE || fixup)
    {
        *ptr++ = '.';
        if (swizzle_x == swizzle_y && swizzle_x == swizzle_z && swizzle_x == swizzle_w) {
            *ptr++ = swizzle_chars[swizzle_x];
        } else {
            *ptr++ = swizzle_chars[swizzle_x];
            *ptr++ = swizzle_chars[swizzle_y];
            *ptr++ = swizzle_chars[swizzle_z];
            *ptr++ = swizzle_chars[swizzle_w];
        }
    }

    *ptr = '\0';
}

static void shader_arb_request_a0(const struct wined3d_shader_instruction *ins, const char *src)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;

    if (!strcmp(priv->addr_reg, src))
        return;

    snprintf(priv->addr_reg, sizeof(priv->addr_reg), "%s", src);
    shader_addline(buffer, "ARL A0.x, %s;\n", src);
}

static void shader_arb_get_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *src, unsigned int tmpreg, char *outregstr);

static void shader_arb_get_register_name(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_register *reg, char *register_name, BOOL *is_color)
{
    /* oPos, oFog and oPts in D3D */
    static const char * const rastout_reg_names[] = {"TMP_OUT", "TMP_FOGCOORD", "result.pointsize"};
    const struct wined3d_shader *shader = ins->ctx->shader;
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    BOOL pshader = shader_is_pshader_version(reg_maps->shader_version.type);
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;

    *is_color = FALSE;

    switch (reg->type)
    {
        case WINED3DSPR_TEMP:
            sprintf(register_name, "R%u", reg->idx[0].offset);
            break;

        case WINED3DSPR_INPUT:
            if (pshader)
            {
                if (reg_maps->shader_version.major < 3)
                {
                    if (!reg->idx[0].offset)
                        strcpy(register_name, "fragment.color.primary");
                    else
                        strcpy(register_name, "fragment.color.secondary");
                }
                else
                {
                    if (reg->idx[0].rel_addr)
                    {
                        char rel_reg[50];
                        shader_arb_get_src_param(ins, reg->idx[0].rel_addr, 0, rel_reg);

                        if (!strcmp(rel_reg, "**aL_emul**"))
                        {
                            DWORD idx = ctx->aL + reg->idx[0].offset;
                            if(idx < MAX_REG_INPUT)
                            {
                                strcpy(register_name, ctx->ps_input[idx]);
                            }
                            else
                            {
                                ERR("Pixel shader input register out of bounds: %u\n", idx);
                                sprintf(register_name, "out_of_bounds_%u", idx);
                            }
                        }
                        else if (reg_maps->input_registers & 0x0300)
                        {
                            /* There are two ways basically:
                             *
                             * 1) Use the unrolling code that is used for loop emulation and unroll the loop.
                             *    That means trouble if the loop also contains a breakc or if the control values
                             *    aren't local constants.
                             * 2) Generate an if block that checks if aL.y < 8, == 8 or == 9 and selects the
                             *    source dynamically. The trouble is that we cannot simply read aL.y because it
                             *    is an ADDRESS register. We could however push it, load .zw with a value and use
                             *    ADAC to load the condition code register and pop it again afterwards
                             */
                            FIXME("Relative input register addressing with more than 8 registers\n");

                            /* This is better than nothing for now */
                            sprintf(register_name, "fragment.texcoord[%s + %u]", rel_reg, reg->idx[0].offset);
                        }
                        else if(ctx->cur_ps_args->super.vp_mode != WINED3D_VP_MODE_SHADER)
                        {
                            /* This is problematic because we'd have to consult the ctx->ps_input strings
                             * for where to find the varying. Some may be "0.0", others can be texcoords or
                             * colors. This needs either a pipeline replacement to make the vertex shader feed
                             * proper varyings, or loop unrolling
                             *
                             * For now use the texcoords and hope for the best
                             */
                            FIXME("Non-vertex shader varying input with indirect addressing\n");
                            sprintf(register_name, "fragment.texcoord[%s + %u]", rel_reg, reg->idx[0].offset);
                        }
                        else
                        {
                            /* D3D supports indirect addressing only with aL in loop registers. The loop instruction
                             * pulls GL_NV_fragment_program2 in
                             */
                            sprintf(register_name, "fragment.texcoord[%s + %u]", rel_reg, reg->idx[0].offset);
                        }
                    }
                    else
                    {
                        if (reg->idx[0].offset < MAX_REG_INPUT)
                        {
                            strcpy(register_name, ctx->ps_input[reg->idx[0].offset]);
                        }
                        else
                        {
                            ERR("Pixel shader input register out of bounds: %u\n", reg->idx[0].offset);
                            sprintf(register_name, "out_of_bounds_%u", reg->idx[0].offset);
                        }
                    }
                }
            }
            else
            {
                if (ctx->cur_vs_args->super.swizzle_map & (1u << reg->idx[0].offset))
                    *is_color = TRUE;
                sprintf(register_name, "vertex.attrib[%u]", reg->idx[0].offset);
            }
            break;

        case WINED3DSPR_CONST:
            if (!pshader && reg->idx[0].rel_addr)
            {
                const struct arb_vshader_private *shader_data = shader->backend_data;
                UINT rel_offset = ctx->target_version == ARB ? shader_data->rel_offset : 0;
                BOOL aL = FALSE;
                char rel_reg[50];
                if (reg_maps->shader_version.major < 2)
                {
                    sprintf(rel_reg, "A0.x");
                }
                else
                {
                    shader_arb_get_src_param(ins, reg->idx[0].rel_addr, 0, rel_reg);
                    if (ctx->target_version == ARB)
                    {
                        if (!strcmp(rel_reg, "**aL_emul**"))
                        {
                            aL = TRUE;
                        } else {
                            shader_arb_request_a0(ins, rel_reg);
                            sprintf(rel_reg, "A0.x");
                        }
                    }
                }
                if (aL)
                    sprintf(register_name, "C[%u]", ctx->aL + reg->idx[0].offset);
                else if (reg->idx[0].offset >= rel_offset)
                    sprintf(register_name, "C[%s + %u]", rel_reg, reg->idx[0].offset - rel_offset);
                else
                    sprintf(register_name, "C[%s - %u]", rel_reg, rel_offset - reg->idx[0].offset);
            }
            else
            {
                if (reg_maps->usesrelconstF)
                    sprintf(register_name, "C[%u]", reg->idx[0].offset);
                else
                    sprintf(register_name, "C%u", reg->idx[0].offset);
            }
            break;

        case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
            if (pshader)
            {
                if (reg_maps->shader_version.major == 1
                        && reg_maps->shader_version.minor <= 3)
                    /* In ps <= 1.3, Tx is a temporary register as destination
                     * to all instructions, and as source to most instructions.
                     * For some instructions it is the texcoord input. Those
                     * instructions know about the special use. */
                    sprintf(register_name, "T%u", reg->idx[0].offset);
                else
                    /* In ps 1.4 and 2.x Tx is always a (read-only) varying. */
                    sprintf(register_name, "fragment.texcoord[%u]", reg->idx[0].offset);
            }
            else
            {
                if (reg_maps->shader_version.major == 1 || ctx->target_version >= NV2)
                    sprintf(register_name, "A%u", reg->idx[0].offset);
                else
                    sprintf(register_name, "A%u_SHADOW", reg->idx[0].offset);
            }
            break;

        case WINED3DSPR_COLOROUT:
            if (ctx->ps_post_process && !reg->idx[0].offset)
            {
                strcpy(register_name, "TMP_COLOR");
            }
            else
            {
                if (ctx->cur_ps_args->super.srgb_correction)
                    FIXME("sRGB correction on higher render targets.\n");
                if (reg_maps->rt_mask > 1)
                    sprintf(register_name, "result.color[%u]", reg->idx[0].offset);
                else
                    strcpy(register_name, "result.color");
            }
            break;

        case WINED3DSPR_RASTOUT:
            if (reg->idx[0].offset == 1)
                sprintf(register_name, "%s", ctx->fog_output);
            else
                sprintf(register_name, "%s", rastout_reg_names[reg->idx[0].offset]);
            break;

        case WINED3DSPR_DEPTHOUT:
            strcpy(register_name, "result.depth");
            break;

        case WINED3DSPR_ATTROUT:
        /* case WINED3DSPR_OUTPUT: */
            if (pshader)
                sprintf(register_name, "oD[%u]", reg->idx[0].offset);
            else
                strcpy(register_name, ctx->color_output[reg->idx[0].offset]);
            break;

        case WINED3DSPR_TEXCRDOUT:
            if (pshader)
                sprintf(register_name, "oT[%u]", reg->idx[0].offset);
            else if (reg_maps->shader_version.major < 3)
                strcpy(register_name, ctx->texcrd_output[reg->idx[0].offset]);
            else
                strcpy(register_name, ctx->vs_output[reg->idx[0].offset]);
            break;

        case WINED3DSPR_LOOP:
            if(ctx->target_version >= NV2)
            {
                /* Pshader has an implicitly declared loop index counter A0.x that cannot be renamed */
                if(pshader) sprintf(register_name, "A0.x");
                else sprintf(register_name, "aL.y");
            }
            else
            {
                /* Unfortunately this code cannot return the value of ctx->aL here. An immediate value
                 * would be valid, but if aL is used for indexing(its only use), there's likely an offset,
                 * thus the result would be something like C[15 + 30], which is not valid in the ARB program
                 * grammar. So return a marker for the emulated aL and intercept it in constant and varying
                 * indexing
                 */
                sprintf(register_name, "**aL_emul**");
            }

            break;

        case WINED3DSPR_CONSTINT:
            sprintf(register_name, "I%u", reg->idx[0].offset);
            break;

        case WINED3DSPR_MISCTYPE:
            if (!reg->idx[0].offset)
                sprintf(register_name, "vpos");
            else if (reg->idx[0].offset == 1)
                sprintf(register_name, "fragment.facing.x");
            else
                FIXME("Unknown MISCTYPE register index %u.\n", reg->idx[0].offset);
            break;

        default:
            FIXME("Unhandled register type %#x[%u].\n", reg->type, reg->idx[0].offset);
            sprintf(register_name, "unrecognized_register[%u]", reg->idx[0].offset);
            break;
    }
}

static void shader_arb_get_dst_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_dst_param *wined3d_dst, char *str)
{
    char register_name[255];
    char write_mask[6];
    BOOL is_color;

    shader_arb_get_register_name(ins, &wined3d_dst->reg, register_name, &is_color);
    strcpy(str, register_name);

    shader_arb_get_write_mask(ins, wined3d_dst, write_mask);
    strcat(str, write_mask);
}

static const char *shader_arb_get_fixup_swizzle(enum fixup_channel_source channel_source)
{
    switch(channel_source)
    {
        case CHANNEL_SOURCE_ZERO: return "0";
        case CHANNEL_SOURCE_ONE: return "1";
        case CHANNEL_SOURCE_X: return "x";
        case CHANNEL_SOURCE_Y: return "y";
        case CHANNEL_SOURCE_Z: return "z";
        case CHANNEL_SOURCE_W: return "w";
        default:
            FIXME("Unhandled channel source %#x\n", channel_source);
            return "undefined";
    }
}

struct color_fixup_masks
{
    DWORD source;
    DWORD sign;
};

static struct color_fixup_masks calc_color_correction(struct color_fixup_desc fixup, DWORD dst_mask)
{
    struct color_fixup_masks masks = {0, 0};

    if (is_complex_fixup(fixup))
    {
        enum complex_fixup complex_fixup = get_complex_fixup(fixup);
        FIXME("Complex fixup (%#x) not supported\n", complex_fixup);
        return masks;
    }

    if (fixup.x_source != CHANNEL_SOURCE_X)
        masks.source |= WINED3DSP_WRITEMASK_0;
    if (fixup.y_source != CHANNEL_SOURCE_Y)
        masks.source |= WINED3DSP_WRITEMASK_1;
    if (fixup.z_source != CHANNEL_SOURCE_Z)
        masks.source |= WINED3DSP_WRITEMASK_2;
    if (fixup.w_source != CHANNEL_SOURCE_W)
        masks.source |= WINED3DSP_WRITEMASK_3;
    masks.source &= dst_mask;

    if (fixup.x_sign_fixup)
        masks.sign |= WINED3DSP_WRITEMASK_0;
    if (fixup.y_sign_fixup)
        masks.sign |= WINED3DSP_WRITEMASK_1;
    if (fixup.z_sign_fixup)
        masks.sign |= WINED3DSP_WRITEMASK_2;
    if (fixup.w_sign_fixup)
        masks.sign |= WINED3DSP_WRITEMASK_3;
    masks.sign &= dst_mask;

    return masks;
}

static void gen_color_correction(struct wined3d_string_buffer *buffer, const char *dst,
        const char *src, const char *one, const char *two,
        struct color_fixup_desc fixup, struct color_fixup_masks masks)
{
    const char *sign_fixup_src = dst;

    if (masks.source)
    {
        if (masks.sign)
            sign_fixup_src = "TA";

        shader_addline(buffer, "SWZ %s, %s, %s, %s, %s, %s;\n", sign_fixup_src, src,
                shader_arb_get_fixup_swizzle(fixup.x_source), shader_arb_get_fixup_swizzle(fixup.y_source),
                shader_arb_get_fixup_swizzle(fixup.z_source), shader_arb_get_fixup_swizzle(fixup.w_source));
    }
    else if (masks.sign)
    {
        sign_fixup_src = src;
    }

    if (masks.sign)
    {
        char reg_mask[6];
        char *ptr = reg_mask;

        if (masks.sign != WINED3DSP_WRITEMASK_ALL)
        {
            *ptr++ = '.';
            if (masks.sign & WINED3DSP_WRITEMASK_0)
                *ptr++ = 'x';
            if (masks.sign & WINED3DSP_WRITEMASK_1)
                *ptr++ = 'y';
            if (masks.sign & WINED3DSP_WRITEMASK_2)
                *ptr++ = 'z';
            if (masks.sign & WINED3DSP_WRITEMASK_3)
                *ptr++ = 'w';
        }
        *ptr = '\0';

        shader_addline(buffer, "MAD %s%s, %s, %s, -%s;\n", dst, reg_mask, sign_fixup_src, two, one);
    }
}

static const char *shader_arb_get_modifier(const struct wined3d_shader_instruction *ins)
{
    DWORD mod;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    if (!ins->dst_count) return "";

    mod = ins->dst[0].modifiers;

    /* Silently ignore PARTIALPRECISION if it's not supported */
    if(priv->target_version == ARB) mod &= ~WINED3DSPDM_PARTIALPRECISION;

    if(mod & WINED3DSPDM_MSAMPCENTROID)
    {
        FIXME("Unhandled modifier WINED3DSPDM_MSAMPCENTROID\n");
        mod &= ~WINED3DSPDM_MSAMPCENTROID;
    }

    switch(mod)
    {
        case WINED3DSPDM_SATURATE | WINED3DSPDM_PARTIALPRECISION:
            return "H_SAT";

        case WINED3DSPDM_SATURATE:
            return "_SAT";

        case WINED3DSPDM_PARTIALPRECISION:
            return "H";

        case 0:
            return "";

        default:
            FIXME("Unknown modifiers 0x%08x\n", mod);
            return "";
    }
}

#define TEX_PROJ        0x1
#define TEX_BIAS        0x2
#define TEX_LOD         0x4
#define TEX_DERIV       0x10

static void shader_hw_sample(const struct wined3d_shader_instruction *ins, DWORD sampler_idx,
        const char *dst_str, const char *coord_reg, WORD flags, const char *dsx, const char *dsy)
{
    BOOL pshader = shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type);
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    enum wined3d_shader_resource_type resource_type;
    struct color_fixup_masks masks;
    const char *tex_dst = dst_str;
    BOOL np2_fixup = FALSE;
    const char *tex_type;
    const char *mod;

    if (pshader)
    {
        resource_type = pixelshader_get_resource_type(ins->ctx->reg_maps, sampler_idx,
                priv->cur_ps_args->super.tex_types);
    }
    else
    {
        resource_type = ins->ctx->reg_maps->resource_info[sampler_idx].type;

        /* D3D vertex shader sampler IDs are vertex samplers(0-3), not global d3d samplers */
        sampler_idx += WINED3D_MAX_FRAGMENT_SAMPLERS;
    }

    switch (resource_type)
    {
        case WINED3D_SHADER_RESOURCE_TEXTURE_1D:
            tex_type = "1D";
            break;

        case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
            if (pshader && priv->cur_ps_args->super.np2_fixup & (1u << sampler_idx)
                    && ins->ctx->gl_info->supported[ARB_TEXTURE_RECTANGLE])
                tex_type = "RECT";
            else
                tex_type = "2D";

            if (pshader)
            {
                if (priv->cur_np2fixup_info->super.active & (1u << sampler_idx))
                {
                    if (flags) FIXME("Only ordinary sampling from NP2 textures is supported.\n");
                    else np2_fixup = TRUE;
                }
            }
            break;

        case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
            tex_type = "3D";
            break;

        case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
            tex_type = "CUBE";
            break;

        default:
            ERR("Unexpected resource type %#x.\n", resource_type);
            tex_type = "";
    }

    /* TEX, TXL, TXD and TXP do not support the "H" modifier,
     * so don't use shader_arb_get_modifier
     */
    if(ins->dst[0].modifiers & WINED3DSPDM_SATURATE) mod = "_SAT";
    else mod = "";

    /* Fragment samplers always have indentity mapping */
    if(sampler_idx >= WINED3D_MAX_FRAGMENT_SAMPLERS)
    {
        sampler_idx = priv->cur_vs_args->vertex.samplers[sampler_idx - WINED3D_MAX_FRAGMENT_SAMPLERS];
    }

    if (pshader)
    {
        masks = calc_color_correction(priv->cur_ps_args->super.color_fixup[sampler_idx],
                ins->dst[0].write_mask);

        if (masks.source || masks.sign)
            tex_dst = "TA";
    }

    if (flags & TEX_DERIV)
    {
        if(flags & TEX_PROJ) FIXME("Projected texture sampling with custom derivatives\n");
        if(flags & TEX_BIAS) FIXME("Biased texture sampling with custom derivatives\n");
        shader_addline(buffer, "TXD%s %s, %s, %s, %s, texture[%u], %s;\n", mod, tex_dst, coord_reg,
                       dsx, dsy, sampler_idx, tex_type);
    }
    else if(flags & TEX_LOD)
    {
        if(flags & TEX_PROJ) FIXME("Projected texture sampling with explicit lod\n");
        if(flags & TEX_BIAS) FIXME("Biased texture sampling with explicit lod\n");
        shader_addline(buffer, "TXL%s %s, %s, texture[%u], %s;\n", mod, tex_dst, coord_reg,
                       sampler_idx, tex_type);
    }
    else if (flags & TEX_BIAS)
    {
        /* Shouldn't be possible, but let's check for it */
        if(flags & TEX_PROJ) FIXME("Biased and Projected texture sampling\n");
        /* TXB takes the 4th component of the source vector automatically, as d3d. Nothing more to do */
        shader_addline(buffer, "TXB%s %s, %s, texture[%u], %s;\n", mod, tex_dst, coord_reg, sampler_idx, tex_type);
    }
    else if (flags & TEX_PROJ)
    {
        shader_addline(buffer, "TXP%s %s, %s, texture[%u], %s;\n", mod, tex_dst, coord_reg, sampler_idx, tex_type);
    }
    else
    {
        if (np2_fixup)
        {
            const unsigned char idx = priv->cur_np2fixup_info->super.idx[sampler_idx];
            shader_addline(buffer, "MUL TA, np2fixup[%u].%s, %s;\n", idx >> 1,
                           (idx % 2) ? "zwxy" : "xyzw", coord_reg);

            shader_addline(buffer, "TEX%s %s, TA, texture[%u], %s;\n", mod, tex_dst, sampler_idx, tex_type);
        }
        else
            shader_addline(buffer, "TEX%s %s, %s, texture[%u], %s;\n", mod, tex_dst, coord_reg, sampler_idx, tex_type);
    }

    if (pshader)
    {
        gen_color_correction(buffer, dst_str, tex_dst,
                arb_get_helper_value(WINED3D_SHADER_TYPE_PIXEL, ARB_ONE),
                arb_get_helper_value(WINED3D_SHADER_TYPE_PIXEL, ARB_TWO),
                priv->cur_ps_args->super.color_fixup[sampler_idx], masks);
    }
}

static void shader_arb_get_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *src, unsigned int tmpreg, char *outregstr)
{
    /* Generate a line that does the input modifier computation and return the input register to use */
    BOOL is_color = FALSE, insert_line;
    char regstr[256];
    char swzstr[20];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;
    const char *one = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ONE);
    const char *two = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_TWO);

    /* Assume a new line will be added */
    insert_line = TRUE;

    /* Get register name */
    shader_arb_get_register_name(ins, &src->reg, regstr, &is_color);
    shader_arb_get_swizzle(src, is_color, swzstr);

    switch (src->modifiers)
    {
    case WINED3DSPSM_NONE:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = FALSE;
        break;
    case WINED3DSPSM_NEG:
        sprintf(outregstr, "-%s%s", regstr, swzstr);
        insert_line = FALSE;
        break;
    case WINED3DSPSM_BIAS:
        shader_addline(buffer, "ADD T%c, %s, -coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_BIASNEG:
        shader_addline(buffer, "ADD T%c, -%s, coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_SIGN:
        shader_addline(buffer, "MAD T%c, %s, %s, -%s;\n", 'A' + tmpreg, regstr, two, one);
        break;
    case WINED3DSPSM_SIGNNEG:
        shader_addline(buffer, "MAD T%c, %s, -%s, %s;\n", 'A' + tmpreg, regstr, two, one);
        break;
    case WINED3DSPSM_COMP:
        shader_addline(buffer, "SUB T%c, %s, %s;\n", 'A' + tmpreg, one, regstr);
        break;
    case WINED3DSPSM_X2:
        shader_addline(buffer, "ADD T%c, %s, %s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case WINED3DSPSM_X2NEG:
        shader_addline(buffer, "ADD T%c, -%s, -%s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case WINED3DSPSM_DZ:
        shader_addline(buffer, "RCP T%c, %s.z;\n", 'A' + tmpreg, regstr);
        shader_addline(buffer, "MUL T%c, %s, T%c;\n", 'A' + tmpreg, regstr, 'A' + tmpreg);
        break;
    case WINED3DSPSM_DW:
        shader_addline(buffer, "RCP T%c, %s.w;\n", 'A' + tmpreg, regstr);
        shader_addline(buffer, "MUL T%c, %s, T%c;\n", 'A' + tmpreg, regstr, 'A' + tmpreg);
        break;
    case WINED3DSPSM_ABS:
        if(ctx->target_version >= NV2) {
            sprintf(outregstr, "|%s%s|", regstr, swzstr);
            insert_line = FALSE;
        } else {
            shader_addline(buffer, "ABS T%c, %s;\n", 'A' + tmpreg, regstr);
        }
        break;
    case WINED3DSPSM_ABSNEG:
        if(ctx->target_version >= NV2) {
            sprintf(outregstr, "-|%s%s|", regstr, swzstr);
        } else {
            shader_addline(buffer, "ABS T%c, %s;\n", 'A' + tmpreg, regstr);
            sprintf(outregstr, "-T%c%s", 'A' + tmpreg, swzstr);
        }
        insert_line = FALSE;
        break;
    default:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = FALSE;
    }

    /* Return modified or original register, with swizzle */
    if (insert_line)
        sprintf(outregstr, "T%c%s", 'A' + tmpreg, swzstr);
}

static void pshader_hw_bem(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    DWORD sampler_code = dst->reg.idx[0].offset;
    char dst_name[50];
    char src_name[2][50];

    shader_arb_get_dst_param(ins, dst, dst_name);

    /* Sampling the perturbation map in Tsrc was done already, including the signedness correction if needed
     *
     * Keep in mind that src_name[1] can be "TB" and src_name[0] can be "TA" because modifiers like _x2 are valid
     * with bem. So delay loading the first parameter until after the perturbation calculation which needs two
     * temps is done.
     */
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);
    shader_addline(buffer, "SWZ TA, bumpenvmat%d, x, z, 0, 0;\n", sampler_code);
    shader_addline(buffer, "DP3 TC.r, TA, %s;\n", src_name[1]);
    shader_addline(buffer, "SWZ TA, bumpenvmat%d, y, w, 0, 0;\n", sampler_code);
    shader_addline(buffer, "DP3 TC.g, TA, %s;\n", src_name[1]);

    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name[0]);
    shader_addline(buffer, "ADD %s, %s, TC;\n", dst_name, src_name[0]);
}

static DWORD negate_modifiers(DWORD mod, char *extra_char)
{
    *extra_char = ' ';
    switch(mod)
    {
        case WINED3DSPSM_NONE:      return WINED3DSPSM_NEG;
        case WINED3DSPSM_NEG:       return WINED3DSPSM_NONE;
        case WINED3DSPSM_BIAS:      return WINED3DSPSM_BIASNEG;
        case WINED3DSPSM_BIASNEG:   return WINED3DSPSM_BIAS;
        case WINED3DSPSM_SIGN:      return WINED3DSPSM_SIGNNEG;
        case WINED3DSPSM_SIGNNEG:   return WINED3DSPSM_SIGN;
        case WINED3DSPSM_COMP:      *extra_char = '-'; return WINED3DSPSM_COMP;
        case WINED3DSPSM_X2:        return WINED3DSPSM_X2NEG;
        case WINED3DSPSM_X2NEG:     return WINED3DSPSM_X2;
        case WINED3DSPSM_DZ:        *extra_char = '-'; return WINED3DSPSM_DZ;
        case WINED3DSPSM_DW:        *extra_char = '-'; return WINED3DSPSM_DW;
        case WINED3DSPSM_ABS:       return WINED3DSPSM_ABSNEG;
        case WINED3DSPSM_ABSNEG:    return WINED3DSPSM_ABS;
    }
    FIXME("Unknown modifier %u\n", mod);
    return mod;
}

static void pshader_hw_cnd(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[3][50];
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);

    shader_arb_get_dst_param(ins, dst, dst_name);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);

    if (shader_version <= WINED3D_SHADER_VERSION(1, 3) && ins->coissue
            && ins->dst->write_mask != WINED3DSP_WRITEMASK_3)
    {
        shader_addline(buffer, "MOV%s %s, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name[1]);
    }
    else
    {
        struct wined3d_shader_src_param src0_copy = ins->src[0];
        char extra_neg;

        /* src0 may have a negate srcmod set, so we can't blindly add "-" to the name */
        src0_copy.modifiers = negate_modifiers(src0_copy.modifiers, &extra_neg);

        shader_arb_get_src_param(ins, &src0_copy, 0, src_name[0]);
        shader_arb_get_src_param(ins, &ins->src[2], 2, src_name[2]);
        shader_addline(buffer, "ADD TA, %c%s, coefdiv.x;\n", extra_neg, src_name[0]);
        shader_addline(buffer, "CMP%s %s, TA, %s, %s;\n", shader_arb_get_modifier(ins),
                dst_name, src_name[1], src_name[2]);
    }
}

static void pshader_hw_cmp(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[3][50];

    shader_arb_get_dst_param(ins, dst, dst_name);

    /* Generate input register names (with modifiers) */
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name[0]);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);
    shader_arb_get_src_param(ins, &ins->src[2], 2, src_name[2]);

    shader_addline(buffer, "CMP%s %s, %s, %s, %s;\n", shader_arb_get_modifier(ins),
            dst_name, src_name[0], src_name[2], src_name[1]);
}

/** Process the WINED3DSIO_DP2ADD instruction in ARB.
 * dst = dot2(src0, src1) + src2 */
static void pshader_hw_dp2add(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[3][50];
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;

    shader_arb_get_dst_param(ins, dst, dst_name);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name[0]);
    shader_arb_get_src_param(ins, &ins->src[2], 2, src_name[2]);

    if(ctx->target_version >= NV3)
    {
        /* GL_NV_fragment_program2 has a 1:1 matching instruction */
        shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);
        shader_addline(buffer, "DP2A%s %s, %s, %s, %s;\n", shader_arb_get_modifier(ins),
                       dst_name, src_name[0], src_name[1], src_name[2]);
    }
    else if(ctx->target_version >= NV2)
    {
        /* dst.x = src2.?, src0.x, src1.x + src0.y * src1.y
         * dst.y = src2.?, src0.x, src1.z + src0.y * src1.w
         * dst.z = src2.?, src0.x, src1.x + src0.y * src1.y
         * dst.z = src2.?, src0.x, src1.z + src0.y * src1.w
         *
         * Make sure that src1.zw = src1.xy, then we get a classic dp2add
         *
         * .xyxy and other swizzles that we could get with this are not valid in
         * plain ARBfp, but luckily the NV extension grammar lifts this limitation.
         */
        struct wined3d_shader_src_param tmp_param = ins->src[1];
        DWORD swizzle = tmp_param.swizzle & 0xf; /* Selects .xy */
        tmp_param.swizzle = swizzle | (swizzle << 4); /* Creates .xyxy */

        shader_arb_get_src_param(ins, &tmp_param, 1, src_name[1]);

        shader_addline(buffer, "X2D%s %s, %s, %s, %s;\n", shader_arb_get_modifier(ins),
                       dst_name, src_name[2], src_name[0], src_name[1]);
    }
    else
    {
        shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);
        /* Emulate a DP2 with a DP3 and 0.0. Don't use the dest as temp register, it could be src[1] or src[2]
        * src_name[0] can be TA, but TA is a private temp for modifiers, so it is save to overwrite
        */
        shader_addline(buffer, "MOV TA, %s;\n", src_name[0]);
        shader_addline(buffer, "MOV TA.z, 0.0;\n");
        shader_addline(buffer, "DP3 TA, TA, %s;\n", src_name[1]);
        shader_addline(buffer, "ADD%s %s, TA, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name[2]);
    }
}

/* Map the opcode 1-to-1 to the GL code */
static void shader_hw_map2gl(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    const char *instruction;
    char arguments[256], dst_str[50];
    unsigned int i;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];

    switch (ins->handler_idx)
    {
        case WINED3DSIH_ABS: instruction = "ABS"; break;
        case WINED3DSIH_ADD: instruction = "ADD"; break;
        case WINED3DSIH_CRS: instruction = "XPD"; break;
        case WINED3DSIH_DP3: instruction = "DP3"; break;
        case WINED3DSIH_DP4: instruction = "DP4"; break;
        case WINED3DSIH_DST: instruction = "DST"; break;
        case WINED3DSIH_FRC: instruction = "FRC"; break;
        case WINED3DSIH_LIT: instruction = "LIT"; break;
        case WINED3DSIH_LRP: instruction = "LRP"; break;
        case WINED3DSIH_MAD: instruction = "MAD"; break;
        case WINED3DSIH_MAX: instruction = "MAX"; break;
        case WINED3DSIH_MIN: instruction = "MIN"; break;
        case WINED3DSIH_MOV: instruction = "MOV"; break;
        case WINED3DSIH_MUL: instruction = "MUL"; break;
        case WINED3DSIH_SGE: instruction = "SGE"; break;
        case WINED3DSIH_SLT: instruction = "SLT"; break;
        case WINED3DSIH_SUB: instruction = "SUB"; break;
        case WINED3DSIH_MOVA:instruction = "ARR"; break;
        case WINED3DSIH_DSX: instruction = "DDX"; break;
        default: instruction = "";
            FIXME("Unhandled opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
            break;
    }

    /* Note that shader_arb_add_dst_param() adds spaces. */
    arguments[0] = '\0';
    shader_arb_get_dst_param(ins, dst, dst_str);
    for (i = 0; i < ins->src_count; ++i)
    {
        char operand[100];
        strcat(arguments, ", ");
        shader_arb_get_src_param(ins, &ins->src[i], i, operand);
        strcat(arguments, operand);
    }
    shader_addline(buffer, "%s%s %s%s;\n", instruction, shader_arb_get_modifier(ins), dst_str, arguments);
}

static void shader_hw_nop(const struct wined3d_shader_instruction *ins) {}

static DWORD shader_arb_select_component(DWORD swizzle, DWORD component)
{
    return ((swizzle >> 2 * component) & 0x3) * 0x55;
}

static void shader_hw_mov(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader *shader = ins->ctx->shader;
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    BOOL pshader = shader_is_pshader_version(reg_maps->shader_version.type);
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;
    const char *zero = arb_get_helper_value(reg_maps->shader_version.type, ARB_ZERO);
    const char *one = arb_get_helper_value(reg_maps->shader_version.type, ARB_ONE);
    const char *two = arb_get_helper_value(reg_maps->shader_version.type, ARB_TWO);

    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char src0_param[256];

    if (ins->handler_idx == WINED3DSIH_MOVA)
    {
        const struct arb_vshader_private *shader_data = shader->backend_data;
        char write_mask[6];
        const char *offset = arb_get_helper_value(WINED3D_SHADER_TYPE_VERTEX, ARB_VS_REL_OFFSET);

        if(ctx->target_version >= NV2) {
            shader_hw_map2gl(ins);
            return;
        }
        shader_arb_get_src_param(ins, &ins->src[0], 0, src0_param);
        shader_arb_get_write_mask(ins, &ins->dst[0], write_mask);

        /* This implements the mova formula used in GLSL. The first two instructions
         * prepare the sign() part. Note that it is fine to have my_sign(0.0) = 1.0
         * in this case:
         * mova A0.x, 0.0
         *
         * A0.x = arl(floor(abs(0.0) + 0.5) * 1.0) = floor(0.5) = 0.0 since arl does a floor
         *
         * The ARL is performed when A0 is used - the requested component is read from A0_SHADOW into
         * A0.x. We can use the overwritten component of A0_shadow as temporary storage for the sign.
         */
        shader_addline(buffer, "SGE A0_SHADOW%s, %s, %s;\n", write_mask, src0_param, zero);
        shader_addline(buffer, "MAD A0_SHADOW%s, A0_SHADOW, %s, -%s;\n", write_mask, two, one);

        shader_addline(buffer, "ABS TA%s, %s;\n", write_mask, src0_param);
        shader_addline(buffer, "ADD TA%s, TA, rel_addr_const.x;\n", write_mask);
        shader_addline(buffer, "FLR TA%s, TA;\n", write_mask);
        if (shader_data->rel_offset)
        {
            shader_addline(buffer, "ADD TA%s, TA, %s;\n", write_mask, offset);
        }
        shader_addline(buffer, "MUL A0_SHADOW%s, TA, A0_SHADOW;\n", write_mask);

        ((struct shader_arb_ctx_priv *)ins->ctx->backend_data)->addr_reg[0] = '\0';
    }
    else if (reg_maps->shader_version.major == 1
          && !shader_is_pshader_version(reg_maps->shader_version.type)
          && ins->dst[0].reg.type == WINED3DSPR_ADDR)
    {
        const struct arb_vshader_private *shader_data = shader->backend_data;
        src0_param[0] = '\0';

        if (shader_data->rel_offset && ctx->target_version == ARB)
        {
            const char *offset = arb_get_helper_value(WINED3D_SHADER_TYPE_VERTEX, ARB_VS_REL_OFFSET);
            shader_arb_get_src_param(ins, &ins->src[0], 0, src0_param);
            shader_addline(buffer, "ADD TA.x, %s, %s;\n", src0_param, offset);
            shader_addline(buffer, "ARL A0.x, TA.x;\n");
        }
        else
        {
            /* Apple's ARB_vertex_program implementation does not accept an ARL source argument
             * with more than one component. Thus replicate the first source argument over all
             * 4 components. For example, .xyzw -> .x (or better: .xxxx), .zwxy -> .z, etc) */
            struct wined3d_shader_src_param tmp_src = ins->src[0];
            tmp_src.swizzle = shader_arb_select_component(tmp_src.swizzle, 0);
            shader_arb_get_src_param(ins, &tmp_src, 0, src0_param);
            shader_addline(buffer, "ARL A0.x, %s;\n", src0_param);
        }
    }
    else if (ins->dst[0].reg.type == WINED3DSPR_COLOROUT && !ins->dst[0].reg.idx[0].offset && pshader)
    {
        if (ctx->ps_post_process && shader->u.ps.color0_mov)
        {
            shader_addline(buffer, "#mov handled in srgb write or fog code\n");
            return;
        }
        shader_hw_map2gl(ins);
    }
    else
    {
        shader_hw_map2gl(ins);
    }
}

static void pshader_hw_texkill(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char reg_dest[40];

    /* No swizzles are allowed in d3d's texkill. PS 1.x ignores the 4th component as documented,
     * but >= 2.0 honors it (undocumented, but tested by the d3d9 testsuite)
     */
    shader_arb_get_dst_param(ins, dst, reg_dest);

    if (ins->ctx->reg_maps->shader_version.major >= 2)
    {
        const char *kilsrc = "TA";
        BOOL is_color;

        shader_arb_get_register_name(ins, &dst->reg, reg_dest, &is_color);
        if(dst->write_mask == WINED3DSP_WRITEMASK_ALL)
        {
            kilsrc = reg_dest;
        }
        else
        {
            /* Sigh. KIL doesn't support swizzles/writemasks. KIL passes a writemask, but ".xy" for example
             * is not valid as a swizzle in ARB (needs ".xyyy"). Use SWZ to load the register properly, and set
             * masked out components to 0(won't kill)
             */
            char x = '0', y = '0', z = '0', w = '0';
            if(dst->write_mask & WINED3DSP_WRITEMASK_0) x = 'x';
            if(dst->write_mask & WINED3DSP_WRITEMASK_1) y = 'y';
            if(dst->write_mask & WINED3DSP_WRITEMASK_2) z = 'z';
            if(dst->write_mask & WINED3DSP_WRITEMASK_3) w = 'w';
            shader_addline(buffer, "SWZ TA, %s, %c, %c, %c, %c;\n", reg_dest, x, y, z, w);
        }
        shader_addline(buffer, "KIL %s;\n", kilsrc);
    }
    else
    {
        /* ARB fp doesn't like swizzles on the parameter of the KIL instruction. To mask the 4th component,
         * copy the register into our general purpose TMP variable, overwrite .w and pass TMP to KIL
         *
         * ps_1_3 shaders use the texcoord incarnation of the Tx register. ps_1_4 shaders can use the same,
         * or pass in any temporary register(in shader phase 2)
         */
        if (ins->ctx->reg_maps->shader_version.minor <= 3)
            sprintf(reg_dest, "fragment.texcoord[%u]", dst->reg.idx[0].offset);
        else
            shader_arb_get_dst_param(ins, dst, reg_dest);
        shader_addline(buffer, "SWZ TA, %s, x, y, z, 1;\n", reg_dest);
        shader_addline(buffer, "KIL TA;\n");
    }
}

static void pshader_hw_tex(const struct wined3d_shader_instruction *ins)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    struct wined3d_shader_src_param src;

    char reg_dest[40];
    char reg_coord[40];
    DWORD reg_sampler_code;
    WORD myflags = 0;
    BOOL swizzle_coord = FALSE;

    /* All versions have a destination register */
    shader_arb_get_dst_param(ins, dst, reg_dest);

    /* 1.0-1.4: Use destination register number as texture code.
       2.0+: Use provided sampler number as texture code. */
    if (shader_version < WINED3D_SHADER_VERSION(2,0))
        reg_sampler_code = dst->reg.idx[0].offset;
    else
        reg_sampler_code = ins->src[1].reg.idx[0].offset;

    /* 1.0-1.3: Use the texcoord varying.
       1.4+: Use provided coordinate source register. */
    if (shader_version < WINED3D_SHADER_VERSION(1,4))
        sprintf(reg_coord, "fragment.texcoord[%u]", reg_sampler_code);
    else {
        /* TEX is the only instruction that can handle DW and DZ natively */
        src = ins->src[0];
        if(src.modifiers == WINED3DSPSM_DW) src.modifiers = WINED3DSPSM_NONE;
        if(src.modifiers == WINED3DSPSM_DZ) src.modifiers = WINED3DSPSM_NONE;
        shader_arb_get_src_param(ins, &src, 0, reg_coord);
    }

    /* projection flag:
     * 1.1, 1.2, 1.3: Use WINED3D_TSS_TEXTURETRANSFORMFLAGS
     * 1.4: Use WINED3DSPSM_DZ or WINED3DSPSM_DW on src[0]
     * 2.0+: Use WINED3DSI_TEXLD_PROJECT on the opcode
     */
    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        DWORD flags = 0;
        if (reg_sampler_code < WINED3D_MAX_TEXTURES)
            flags = priv->cur_ps_args->super.tex_transform >> reg_sampler_code * WINED3D_PSARGS_TEXTRANSFORM_SHIFT;
        if (flags & WINED3D_PSARGS_PROJECTED)
        {
            myflags |= TEX_PROJ;
            if ((flags & ~WINED3D_PSARGS_PROJECTED) == WINED3D_TTFF_COUNT3)
                swizzle_coord = TRUE;
        }
    }
    else if (shader_version < WINED3D_SHADER_VERSION(2,0))
    {
        enum wined3d_shader_src_modifier src_mod = ins->src[0].modifiers;
        if (src_mod == WINED3DSPSM_DZ)
        {
            swizzle_coord = TRUE;
            myflags |= TEX_PROJ;
        } else if(src_mod == WINED3DSPSM_DW) {
            myflags |= TEX_PROJ;
        }
    } else {
        if (ins->flags & WINED3DSI_TEXLD_PROJECT) myflags |= TEX_PROJ;
        if (ins->flags & WINED3DSI_TEXLD_BIAS) myflags |= TEX_BIAS;
    }

    if (swizzle_coord)
    {
        /* TXP cannot handle DZ natively, so move the z coordinate to .w.
         * reg_coord is a read-only varying register, so we need a temp reg */
        shader_addline(ins->ctx->buffer, "SWZ TA, %s, x, y, z, z;\n", reg_coord);
        strcpy(reg_coord, "TA");
    }

    shader_hw_sample(ins, reg_sampler_code, reg_dest, reg_coord, myflags, NULL, NULL);
}

static void pshader_hw_texcoord(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    char dst_str[50];

    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        DWORD reg = dst->reg.idx[0].offset;

        shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
        shader_addline(buffer, "MOV_SAT %s, fragment.texcoord[%u];\n", dst_str, reg);
    } else {
        char reg_src[40];

        shader_arb_get_src_param(ins, &ins->src[0], 0, reg_src);
        shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
        shader_addline(buffer, "MOV %s, %s;\n", dst_str, reg_src);
    }
}

static void pshader_hw_texreg2ar(const struct wined3d_shader_instruction *ins)
{
     struct wined3d_string_buffer *buffer = ins->ctx->buffer;
     DWORD flags = 0;

     DWORD reg1 = ins->dst[0].reg.idx[0].offset;
     char dst_str[50];
     char src_str[50];

     /* Note that texreg2ar treats Tx as a temporary register, not as a varying */
     shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
     shader_arb_get_src_param(ins, &ins->src[0], 0, src_str);
     /* Move .x first in case src_str is "TA" */
     shader_addline(buffer, "MOV TA.y, %s.x;\n", src_str);
     shader_addline(buffer, "MOV TA.x, %s.w;\n", src_str);
     if (reg1 < WINED3D_MAX_TEXTURES)
     {
         struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
         flags = priv->cur_ps_args->super.tex_transform >> reg1 * WINED3D_PSARGS_TEXTRANSFORM_SHIFT;
     }
     shader_hw_sample(ins, reg1, dst_str, "TA", flags & WINED3D_PSARGS_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
}

static void pshader_hw_texreg2gb(const struct wined3d_shader_instruction *ins)
{
     struct wined3d_string_buffer *buffer = ins->ctx->buffer;

     DWORD reg1 = ins->dst[0].reg.idx[0].offset;
     char dst_str[50];
     char src_str[50];

     /* Note that texreg2gb treats Tx as a temporary register, not as a varying */
     shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
     shader_arb_get_src_param(ins, &ins->src[0], 0, src_str);
     shader_addline(buffer, "MOV TA.x, %s.y;\n", src_str);
     shader_addline(buffer, "MOV TA.y, %s.z;\n", src_str);
     shader_hw_sample(ins, reg1, dst_str, "TA", 0, NULL, NULL);
}

static void pshader_hw_texreg2rgb(const struct wined3d_shader_instruction *ins)
{
    DWORD reg1 = ins->dst[0].reg.idx[0].offset;
    char dst_str[50];
    char src_str[50];

    /* Note that texreg2rg treats Tx as a temporary register, not as a varying */
    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_str);
    shader_hw_sample(ins, reg1, dst_str, src_str, 0, NULL, NULL);
}

static void pshader_hw_texbem(const struct wined3d_shader_instruction *ins)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char reg_coord[40], dst_reg[50], src_reg[50];
    DWORD reg_dest_code;

    /* All versions have a destination register. The Tx where the texture coordinates come
     * from is the varying incarnation of the texture register
     */
    reg_dest_code = dst->reg.idx[0].offset;
    shader_arb_get_dst_param(ins, &ins->dst[0], dst_reg);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_reg);
    sprintf(reg_coord, "fragment.texcoord[%u]", reg_dest_code);

    /* Sampling the perturbation map in Tsrc was done already, including the signedness correction if needed
     * The Tx in which the perturbation map is stored is the tempreg incarnation of the texture register
     *
     * GL_NV_fragment_program_option could handle this in one instruction via X2D:
     * X2D TA.xy, fragment.texcoord, T%u, bumpenvmat%u.xzyw
     *
     * However, the NV extensions are never enabled for <= 2.0 shaders because of the performance penalty that
     * comes with it, and texbem is an 1.x only instruction. No 1.x instruction forces us to enable the NV
     * extension.
     */
    shader_addline(buffer, "SWZ TB, bumpenvmat%d, x, z, 0, 0;\n", reg_dest_code);
    shader_addline(buffer, "DP3 TA.x, TB, %s;\n", src_reg);
    shader_addline(buffer, "SWZ TB, bumpenvmat%d, y, w, 0, 0;\n", reg_dest_code);
    shader_addline(buffer, "DP3 TA.y, TB, %s;\n", src_reg);

    /* with projective textures, texbem only divides the static texture coord, not the displacement,
     * so we can't let the GL handle this.
     */
    if ((priv->cur_ps_args->super.tex_transform >> reg_dest_code * WINED3D_PSARGS_TEXTRANSFORM_SHIFT)
            & WINED3D_PSARGS_PROJECTED)
    {
        shader_addline(buffer, "RCP TB.w, %s.w;\n", reg_coord);
        shader_addline(buffer, "MUL TB.xy, %s, TB.w;\n", reg_coord);
        shader_addline(buffer, "ADD TA.xy, TA, TB;\n");
    } else {
        shader_addline(buffer, "ADD TA.xy, TA, %s;\n", reg_coord);
    }

    shader_hw_sample(ins, reg_dest_code, dst_reg, "TA", 0, NULL, NULL);

    if (ins->handler_idx == WINED3DSIH_TEXBEML)
    {
        /* No src swizzles are allowed, so this is ok */
        shader_addline(buffer, "MAD TA, %s.z, luminance%d.x, luminance%d.y;\n",
                       src_reg, reg_dest_code, reg_dest_code);
        shader_addline(buffer, "MUL %s, %s, TA;\n", dst_reg, dst_reg);
    }
}

static void pshader_hw_texm3x2pad(const struct wined3d_shader_instruction *ins)
{
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char src0_name[50], dst_name[50];
    BOOL is_color;
    struct wined3d_shader_register tmp_reg = ins->dst[0].reg;

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    /* The next instruction will be a texm3x2tex or texm3x2depth that writes to the uninitialized
     * T<reg+1> register. Use this register to store the calculated vector
     */
    tmp_reg.idx[0].offset = reg + 1;
    shader_arb_get_register_name(ins, &tmp_reg, dst_name, &is_color);
    shader_addline(buffer, "DP3 %s.x, fragment.texcoord[%u], %s;\n", dst_name, reg, src0_name);
}

static void pshader_hw_texm3x2tex(const struct wined3d_shader_instruction *ins)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_str[50];
    char src0_name[50];
    char dst_reg[50];
    BOOL is_color;

    /* We know that we're writing to the uninitialized T<reg> register, so use it for temporary storage */
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_reg, &is_color);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 %s.y, fragment.texcoord[%u], %s;\n", dst_reg, reg, src0_name);
    flags = reg < WINED3D_MAX_TEXTURES ? priv->cur_ps_args->super.tex_transform >> reg * WINED3D_PSARGS_TEXTRANSFORM_SHIFT : 0;
    shader_hw_sample(ins, reg, dst_str, dst_reg, flags & WINED3D_PSARGS_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
}

static void pshader_hw_texm3x3pad(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char src0_name[50], dst_name[50];
    struct wined3d_shader_register tmp_reg = ins->dst[0].reg;
    BOOL is_color;

    /* There are always 2 texm3x3pad instructions followed by one texm3x3[tex,vspec, ...] instruction, with
     * incrementing ins->dst[0].register_idx numbers. So the pad instruction already knows the final destination
     * register, and this register is uninitialized(otherwise the assembler complains that it is 'redeclared')
     */
    tmp_reg.idx[0].offset = reg + 2 - tex_mx->current_row;
    shader_arb_get_register_name(ins, &tmp_reg, dst_name, &is_color);

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 %s.%c, fragment.texcoord[%u], %s;\n",
                   dst_name, 'x' + tex_mx->current_row, reg, src0_name);
    tex_mx->texcoord_w[tex_mx->current_row++] = reg;
}

static void pshader_hw_texm3x3tex(const struct wined3d_shader_instruction *ins)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_str[50];
    char src0_name[50], dst_name[50];
    BOOL is_color;

    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_name, &is_color);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 %s.z, fragment.texcoord[%u], %s;\n", dst_name, reg, src0_name);

    /* Sample the texture using the calculated coordinates */
    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    flags = reg < WINED3D_MAX_TEXTURES ? priv->cur_ps_args->super.tex_transform >> reg * WINED3D_PSARGS_TEXTRANSFORM_SHIFT : 0;
    shader_hw_sample(ins, reg, dst_str, dst_name, flags & WINED3D_PSARGS_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
    tex_mx->current_row = 0;
}

static void pshader_hw_texm3x3vspec(const struct wined3d_shader_instruction *ins)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_str[50];
    char src0_name[50];
    char dst_reg[50];
    BOOL is_color;

    /* Get the dst reg without writemask strings. We know this register is uninitialized, so we can use all
     * components for temporary data storage
     */
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_reg, &is_color);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 %s.z, fragment.texcoord[%u], %s;\n", dst_reg, reg, src0_name);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "MOV TB.x, fragment.texcoord[%u].w;\n", tex_mx->texcoord_w[0]);
    shader_addline(buffer, "MOV TB.y, fragment.texcoord[%u].w;\n", tex_mx->texcoord_w[1]);
    shader_addline(buffer, "MOV TB.z, fragment.texcoord[%u].w;\n", reg);

    /* Calculate reflection vector
     */
    shader_addline(buffer, "DP3 %s.w, %s, TB;\n", dst_reg, dst_reg);
    /* The .w is ignored when sampling, so I can use TB.w to calculate dot(N, N) */
    shader_addline(buffer, "DP3 TB.w, %s, %s;\n", dst_reg, dst_reg);
    shader_addline(buffer, "RCP TB.w, TB.w;\n");
    shader_addline(buffer, "MUL %s.w, %s.w, TB.w;\n", dst_reg, dst_reg);
    shader_addline(buffer, "MUL %s, %s.w, %s;\n", dst_reg, dst_reg, dst_reg);
    shader_addline(buffer, "MAD %s, coefmul.x, %s, -TB;\n", dst_reg, dst_reg);

    /* Sample the texture using the calculated coordinates */
    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    flags = reg < WINED3D_MAX_TEXTURES ? priv->cur_ps_args->super.tex_transform >> reg * WINED3D_PSARGS_TEXTRANSFORM_SHIFT : 0;
    shader_hw_sample(ins, reg, dst_str, dst_reg, flags & WINED3D_PSARGS_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
    tex_mx->current_row = 0;
}

static void pshader_hw_texm3x3spec(const struct wined3d_shader_instruction *ins)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct wined3d_shader_tex_mx *tex_mx = ins->ctx->tex_mx;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx[0].offset;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_str[50];
    char src0_name[50];
    char src1_name[50];
    char dst_reg[50];
    BOOL is_color;

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_arb_get_src_param(ins, &ins->src[0], 1, src1_name);
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_reg, &is_color);
    /* Note: dst_reg.xy is input here, generated by two texm3x3pad instructions */
    shader_addline(buffer, "DP3 %s.z, fragment.texcoord[%u], %s;\n", dst_reg, reg, src0_name);

    /* Calculate reflection vector.
     *
     *                   dot(N, E)
     * dst_reg.xyz = 2 * --------- * N - E
     *                   dot(N, N)
     *
     * Which normalizes the normal vector
     */
    shader_addline(buffer, "DP3 %s.w, %s, %s;\n", dst_reg, dst_reg, src1_name);
    shader_addline(buffer, "DP3 TC.w, %s, %s;\n", dst_reg, dst_reg);
    shader_addline(buffer, "RCP TC.w, TC.w;\n");
    shader_addline(buffer, "MUL %s.w, %s.w, TC.w;\n", dst_reg, dst_reg);
    shader_addline(buffer, "MUL %s, %s.w, %s;\n", dst_reg, dst_reg, dst_reg);
    shader_addline(buffer, "MAD %s, coefmul.x, %s, -%s;\n", dst_reg, dst_reg, src1_name);

    /* Sample the texture using the calculated coordinates */
    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    flags = reg < WINED3D_MAX_TEXTURES ? priv->cur_ps_args->super.tex_transform >> reg * WINED3D_PSARGS_TEXTRANSFORM_SHIFT : 0;
    shader_hw_sample(ins, reg, dst_str, dst_reg, flags & WINED3D_PSARGS_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
    tex_mx->current_row = 0;
}

static void pshader_hw_texdepth(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    const char *zero = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ZERO);
    const char *one = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ONE);

    /* texdepth has an implicit destination, the fragment depth value. It's only parameter,
     * which is essentially an input, is the destination register because it is the first
     * parameter. According to the msdn, this must be register r5, but let's keep it more flexible
     * here(writemasks/swizzles are not valid on texdepth)
     */
    shader_arb_get_dst_param(ins, dst, dst_name);

    /* According to the msdn, the source register(must be r5) is unusable after
     * the texdepth instruction, so we're free to modify it
     */
    shader_addline(buffer, "MIN %s.y, %s.y, %s;\n", dst_name, dst_name, one);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP %s.y, %s.y;\n", dst_name, dst_name);
    shader_addline(buffer, "MUL TA.x, %s.x, %s.y;\n", dst_name, dst_name);
    shader_addline(buffer, "MIN TA.x, TA.x, %s;\n", one);
    shader_addline(buffer, "MAX result.depth, TA.x, %s;\n", zero);
}

/** Process the WINED3DSIO_TEXDP3TEX instruction in ARB:
 * Take a 3-component dot product of the TexCoord[dstreg] and src,
 * then perform a 1D texture lookup from stage dstregnum, place into dst. */
static void pshader_hw_texdp3tex(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    DWORD sampler_idx = ins->dst[0].reg.idx[0].offset;
    char src0[50];
    char dst_str[50];

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0);
    shader_addline(buffer, "MOV TB, 0.0;\n");
    shader_addline(buffer, "DP3 TB.x, fragment.texcoord[%u], %s;\n", sampler_idx, src0);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    shader_hw_sample(ins, sampler_idx, dst_str, "TB", 0 /* Only one coord, can't be projected */, NULL, NULL);
}

/** Process the WINED3DSIO_TEXDP3 instruction in ARB:
 * Take a 3-component dot product of the TexCoord[dstreg] and src. */
static void pshader_hw_texdp3(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    char src0[50];
    char dst_str[50];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;

    /* Handle output register */
    shader_arb_get_dst_param(ins, dst, dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0);
    shader_addline(buffer, "DP3 %s, fragment.texcoord[%u], %s;\n", dst_str, dst->reg.idx[0].offset, src0);
}

/** Process the WINED3DSIO_TEXM3X3 instruction in ARB
 * Perform the 3rd row of a 3x3 matrix multiply */
static void pshader_hw_texm3x3(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_str[50], dst_name[50];
    char src0[50];
    BOOL is_color;

    shader_arb_get_dst_param(ins, dst, dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0);
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_name, &is_color);
    shader_addline(buffer, "DP3 %s.z, fragment.texcoord[%u], %s;\n", dst_name, dst->reg.idx[0].offset, src0);
    shader_addline(buffer, "MOV %s, %s;\n", dst_str, dst_name);
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in ARB:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
static void pshader_hw_texm3x2depth(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    char src0[50], dst_name[50];
    BOOL is_color;
    const char *zero = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ZERO);
    const char *one = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ONE);

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0);
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_name, &is_color);
    shader_addline(buffer, "DP3 %s.y, fragment.texcoord[%u], %s;\n", dst_name, dst->reg.idx[0].offset, src0);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP %s.y, %s.y;\n", dst_name, dst_name);
    shader_addline(buffer, "MUL %s.x, %s.x, %s.y;\n", dst_name, dst_name, dst_name);
    shader_addline(buffer, "MIN %s.x, %s.x, %s;\n", dst_name, dst_name, one);
    shader_addline(buffer, "MAX result.depth, %s.x, %s;\n", dst_name, zero);
}

/** Handles transforming all WINED3DSIO_M?x? opcodes for
    Vertex/Pixel shaders to ARB_vertex_program codes */
static void shader_hw_mnxn(const struct wined3d_shader_instruction *ins)
{
    int i;
    int nComponents = 0;
    struct wined3d_shader_dst_param tmp_dst = {{0}};
    struct wined3d_shader_src_param tmp_src[2] = {{{0}}};
    struct wined3d_shader_instruction tmp_ins;

    memset(&tmp_ins, 0, sizeof(tmp_ins));

    /* Set constants for the temporary argument */
    tmp_ins.ctx = ins->ctx;
    tmp_ins.dst_count = 1;
    tmp_ins.dst = &tmp_dst;
    tmp_ins.src_count = 2;
    tmp_ins.src = tmp_src;

    switch(ins->handler_idx)
    {
        case WINED3DSIH_M4x4:
            nComponents = 4;
            tmp_ins.handler_idx = WINED3DSIH_DP4;
            break;
        case WINED3DSIH_M4x3:
            nComponents = 3;
            tmp_ins.handler_idx = WINED3DSIH_DP4;
            break;
        case WINED3DSIH_M3x4:
            nComponents = 4;
            tmp_ins.handler_idx = WINED3DSIH_DP3;
            break;
        case WINED3DSIH_M3x3:
            nComponents = 3;
            tmp_ins.handler_idx = WINED3DSIH_DP3;
            break;
        case WINED3DSIH_M3x2:
            nComponents = 2;
            tmp_ins.handler_idx = WINED3DSIH_DP3;
            break;
        default:
            FIXME("Unhandled opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
            break;
    }

    tmp_dst = ins->dst[0];
    tmp_src[0] = ins->src[0];
    tmp_src[1] = ins->src[1];
    for (i = 0; i < nComponents; ++i)
    {
        tmp_dst.write_mask = WINED3DSP_WRITEMASK_0 << i;
        shader_hw_map2gl(&tmp_ins);
        ++tmp_src[1].reg.idx[0].offset;
    }
}

static DWORD abs_modifier(DWORD mod, BOOL *need_abs)
{
    *need_abs = FALSE;

    switch(mod)
    {
        case WINED3DSPSM_NONE:      return WINED3DSPSM_ABS;
        case WINED3DSPSM_NEG:       return WINED3DSPSM_ABS;
        case WINED3DSPSM_BIAS:      *need_abs = TRUE; return WINED3DSPSM_BIAS;
        case WINED3DSPSM_BIASNEG:   *need_abs = TRUE; return WINED3DSPSM_BIASNEG;
        case WINED3DSPSM_SIGN:      *need_abs = TRUE; return WINED3DSPSM_SIGN;
        case WINED3DSPSM_SIGNNEG:   *need_abs = TRUE; return WINED3DSPSM_SIGNNEG;
        case WINED3DSPSM_COMP:      *need_abs = TRUE; return WINED3DSPSM_COMP;
        case WINED3DSPSM_X2:        *need_abs = TRUE; return WINED3DSPSM_X2;
        case WINED3DSPSM_X2NEG:     *need_abs = TRUE; return WINED3DSPSM_X2NEG;
        case WINED3DSPSM_DZ:        *need_abs = TRUE; return WINED3DSPSM_DZ;
        case WINED3DSPSM_DW:        *need_abs = TRUE; return WINED3DSPSM_DW;
        case WINED3DSPSM_ABS:       return WINED3DSPSM_ABS;
        case WINED3DSPSM_ABSNEG:    return WINED3DSPSM_ABS;
    }
    FIXME("Unknown modifier %u\n", mod);
    return mod;
}

static void shader_hw_scalar_op(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    const char *instruction;
    struct wined3d_shader_src_param src0_copy = ins->src[0];
    BOOL need_abs = FALSE;

    char dst[50];
    char src[50];

    switch(ins->handler_idx)
    {
        case WINED3DSIH_RSQ:  instruction = "RSQ"; break;
        case WINED3DSIH_RCP:  instruction = "RCP"; break;
        case WINED3DSIH_EXPP:
            if (ins->ctx->reg_maps->shader_version.major < 2)
            {
                instruction = "EXP";
                break;
            }
            /* Drop through. */
        case WINED3DSIH_EXP:
            instruction = "EX2";
            break;
        case WINED3DSIH_LOG:
        case WINED3DSIH_LOGP:
            /* The precision requirements suggest that LOGP matches ARBvp's LOG
             * instruction, but notice that the output of those instructions is
             * different. */
            src0_copy.modifiers = abs_modifier(src0_copy.modifiers, &need_abs);
            instruction = "LG2";
            break;
        default: instruction = "";
            FIXME("Unhandled opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
            break;
    }

    /* Dx sdk says .x is used if no swizzle is given, but our test shows that
     * .w is used. */
    src0_copy.swizzle = shader_arb_select_component(src0_copy.swizzle, 3);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst); /* Destination */
    shader_arb_get_src_param(ins, &src0_copy, 0, src);

    if(need_abs)
    {
        shader_addline(buffer, "ABS TA.w, %s;\n", src);
        shader_addline(buffer, "%s%s %s, TA.w;\n", instruction, shader_arb_get_modifier(ins), dst);
    }
    else
    {
        shader_addline(buffer, "%s%s %s, %s;\n", instruction, shader_arb_get_modifier(ins), dst, src);
    }

}

static void shader_hw_nrm(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[50];
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    BOOL pshader = shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type);
    const char *zero = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ZERO);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst_name);
    shader_arb_get_src_param(ins, &ins->src[0], 1 /* Use TB */, src_name);

    /* In D3D, NRM of a vector with length zero returns zero. Catch this situation, as
     * otherwise NRM or RSQ would return NaN */
    if(pshader && priv->target_version >= NV3)
    {
        /* GL_NV_fragment_program2's NRM needs protection against length zero vectors too
         *
         * TODO: Find out if DP3+NRM+MOV is really faster than DP3+RSQ+MUL
         */
        shader_addline(buffer, "DP3C TA, %s, %s;\n", src_name, src_name);
        shader_addline(buffer, "NRM%s %s, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name);
        shader_addline(buffer, "MOV %s (EQ), %s;\n", dst_name, zero);
    }
    else if(priv->target_version >= NV2)
    {
        shader_addline(buffer, "DP3C TA.x, %s, %s;\n", src_name, src_name);
        shader_addline(buffer, "RSQ TA.x (NE), TA.x;\n");
        shader_addline(buffer, "MUL%s %s, %s, TA.x;\n", shader_arb_get_modifier(ins), dst_name,
                       src_name);
    }
    else
    {
        const char *one = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ONE);

        shader_addline(buffer, "DP3 TA.x, %s, %s;\n", src_name, src_name);
        /* Pass any non-zero value to RSQ if the input vector has a length of zero. The
         * RSQ result doesn't matter, as long as multiplying it by 0 returns 0.
         */
        shader_addline(buffer, "SGE TA.y, -TA.x, %s;\n", zero);
        shader_addline(buffer, "MAD TA.x, %s, TA.y, TA.x;\n", one);

        shader_addline(buffer, "RSQ TA.x, TA.x;\n");
        /* dst.w = src[0].w * 1 / (src.x^2 + src.y^2 + src.z^2)^(1/2) according to msdn*/
        shader_addline(buffer, "MUL%s %s, %s, TA.x;\n", shader_arb_get_modifier(ins), dst_name,
                    src_name);
    }
}

static void shader_hw_lrp(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[3][50];

    /* ARB_fragment_program has a convenient LRP instruction */
    if(shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type)) {
        shader_hw_map2gl(ins);
        return;
    }

    shader_arb_get_dst_param(ins, &ins->dst[0], dst_name);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name[0]);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);
    shader_arb_get_src_param(ins, &ins->src[2], 2, src_name[2]);

    shader_addline(buffer, "SUB TA, %s, %s;\n", src_name[1], src_name[2]);
    shader_addline(buffer, "MAD%s %s, %s, TA, %s;\n", shader_arb_get_modifier(ins),
                   dst_name, src_name[0], src_name[2]);
}

static void shader_hw_sincos(const struct wined3d_shader_instruction *ins)
{
    /* This instruction exists in ARB, but the d3d instruction takes two extra parameters which
     * must contain fixed constants. So we need a separate function to filter those constants and
     * can't use map2gl
     */
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    char dst_name[50];
    char src_name0[50], src_name1[50], src_name2[50];
    BOOL is_color;

    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name0);
    if(shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type)) {
        shader_arb_get_dst_param(ins, &ins->dst[0], dst_name);
        /* No modifiers are supported on SCS */
        shader_addline(buffer, "SCS %s, %s;\n", dst_name, src_name0);

        if(ins->dst[0].modifiers & WINED3DSPDM_SATURATE)
        {
            shader_arb_get_register_name(ins, &dst->reg, src_name0, &is_color);
            shader_addline(buffer, "MOV_SAT %s, %s;\n", dst_name, src_name0);
        }
    } else if(priv->target_version >= NV2) {
        shader_arb_get_register_name(ins, &dst->reg, dst_name, &is_color);

        /* Sincos writemask must be .x, .y or .xy */
        if(dst->write_mask & WINED3DSP_WRITEMASK_0)
            shader_addline(buffer, "COS%s %s.x, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name0);
        if(dst->write_mask & WINED3DSP_WRITEMASK_1)
            shader_addline(buffer, "SIN%s %s.y, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name0);
    } else {
        /* Approximate sine and cosine with a taylor series, as per math textbook. The application passes 8
         * helper constants(D3DSINCOSCONST1 and D3DSINCOSCONST2) in src1 and src2.
         *
         * sin(x) = x - x^3/3! + x^5/5! - x^7/7! + ...
         * cos(x) = 1 - x^2/2! + x^4/4! - x^6/6! + ...
         *
         * The constants we get are:
         *
         *  +1   +1,     -1     -1     +1      +1      -1       -1
         *      ---- ,  ---- , ---- , ----- , ----- , ----- , ------
         *      1!*2    2!*4   3!*8   4!*16   5!*32   6!*64   7!*128
         *
         * If used with x^2, x^3, x^4 etc they calculate sin(x/2) and cos(x/2):
         *
         * (x/2)^2 = x^2 / 4
         * (x/2)^3 = x^3 / 8
         * (x/2)^4 = x^4 / 16
         * (x/2)^5 = x^5 / 32
         * etc
         *
         * To get the final result:
         * sin(x) = 2 * sin(x/2) * cos(x/2)
         * cos(x) = cos(x/2)^2 - sin(x/2)^2
         * (from sin(x+y) and cos(x+y) rules)
         *
         * As per MSDN, dst.z is undefined after the operation, and so is
         * dst.x and dst.y if they're masked out by the writemask. Ie
         * sincos dst.y, src1, c0, c1
         * returns the sine in dst.y. dst.x and dst.z are undefined, dst.w is not touched. The assembler
         * vsa.exe also stops with an error if the dest register is the same register as the source
         * register. This means we can use dest.xyz as temporary storage. The assembler vsa.exe output also
         * indicates that sincos consumes 8 instruction slots in vs_2_0(and, strangely, in vs_3_0).
         */
        shader_arb_get_src_param(ins, &ins->src[1], 1, src_name1);
        shader_arb_get_src_param(ins, &ins->src[2], 2, src_name2);
        shader_arb_get_register_name(ins, &dst->reg, dst_name, &is_color);

        shader_addline(buffer, "MUL %s.x, %s, %s;\n", dst_name, src_name0, src_name0);  /* x ^ 2 */
        shader_addline(buffer, "MUL TA.y, %s.x, %s;\n", dst_name, src_name0);           /* x ^ 3 */
        shader_addline(buffer, "MUL %s.y, TA.y, %s;\n", dst_name, src_name0);           /* x ^ 4 */
        shader_addline(buffer, "MUL TA.z, %s.y, %s;\n", dst_name, src_name0);           /* x ^ 5 */
        shader_addline(buffer, "MUL %s.z, TA.z, %s;\n", dst_name, src_name0);           /* x ^ 6 */
        shader_addline(buffer, "MUL TA.w, %s.z, %s;\n", dst_name, src_name0);           /* x ^ 7 */

        /* sin(x/2)
         *
         * Unfortunately we don't get the constants in a DP4-capable form. Is there a way to
         * properly merge that with MULs in the code above?
         * The swizzles .yz and xw however fit into the .yzxw swizzle added to ps_2_0. Maybe
         * we can merge the sine and cosine MAD rows to calculate them together.
         */
        shader_addline(buffer, "MUL TA.x, %s, %s.w;\n", src_name0, src_name2); /* x^1, +1/(1!*2) */
        shader_addline(buffer, "MAD TA.x, TA.y, %s.x, TA.x;\n", src_name2); /* -1/(3!*8) */
        shader_addline(buffer, "MAD TA.x, TA.z, %s.w, TA.x;\n", src_name1); /* +1/(5!*32) */
        shader_addline(buffer, "MAD TA.x, TA.w, %s.x, TA.x;\n", src_name1); /* -1/(7!*128) */

        /* cos(x/2) */
        shader_addline(buffer, "MAD TA.y, %s.x, %s.y, %s.z;\n", dst_name, src_name2, src_name2); /* -1/(2!*4), +1.0 */
        shader_addline(buffer, "MAD TA.y, %s.y, %s.z, TA.y;\n", dst_name, src_name1); /* +1/(4!*16) */
        shader_addline(buffer, "MAD TA.y, %s.z, %s.y, TA.y;\n", dst_name, src_name1); /* -1/(6!*64) */

        if(dst->write_mask & WINED3DSP_WRITEMASK_0) {
            /* cos x */
            shader_addline(buffer, "MUL TA.z, TA.y, TA.y;\n");
            shader_addline(buffer, "MAD %s.x, -TA.x, TA.x, TA.z;\n", dst_name);
        }
        if(dst->write_mask & WINED3DSP_WRITEMASK_1) {
            /* sin x */
            shader_addline(buffer, "MUL %s.y, TA.x, TA.y;\n", dst_name);
            shader_addline(buffer, "ADD %s.y, %s.y, %s.y;\n", dst_name, dst_name, dst_name);
        }
    }
}

static void shader_hw_sgn(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[50];
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;

    shader_arb_get_dst_param(ins, &ins->dst[0], dst_name);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name);

    /* SGN is only valid in vertex shaders */
    if(ctx->target_version >= NV2) {
        shader_addline(buffer, "SSG%s %s, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name);
        return;
    }

    /* If SRC > 0.0, -SRC < SRC = TRUE, otherwise false.
     * if SRC < 0.0,  SRC < -SRC = TRUE. If neither is true, src = 0.0
     */
    if(ins->dst[0].modifiers & WINED3DSPDM_SATURATE) {
        shader_addline(buffer, "SLT %s, -%s, %s;\n", dst_name, src_name, src_name);
    } else {
        /* src contains TA? Write to the dest first. This won't overwrite our destination.
         * Then use TA, and calculate the final result
         *
         * Not reading from TA? Store the first result in TA to avoid overwriting the
         * destination if src reg = dst reg
         */
        if(strstr(src_name, "TA"))
        {
            shader_addline(buffer, "SLT %s,  %s, -%s;\n", dst_name, src_name, src_name);
            shader_addline(buffer, "SLT TA, -%s, %s;\n", src_name, src_name);
            shader_addline(buffer, "ADD %s, %s, -TA;\n", dst_name, dst_name);
        }
        else
        {
            shader_addline(buffer, "SLT TA, -%s, %s;\n", src_name, src_name);
            shader_addline(buffer, "SLT %s,  %s, -%s;\n", dst_name, src_name, src_name);
            shader_addline(buffer, "ADD %s, TA, -%s;\n", dst_name, dst_name);
        }
    }
}

static void shader_hw_dsy(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char src[50];
    char dst[50];
    char dst_name[50];
    BOOL is_color;

    shader_arb_get_dst_param(ins, &ins->dst[0], dst);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src);
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_name, &is_color);

    shader_addline(buffer, "DDY %s, %s;\n", dst, src);
    shader_addline(buffer, "MUL%s %s, %s, ycorrection.y;\n", shader_arb_get_modifier(ins), dst, dst_name);
}

static void shader_hw_pow(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char src0[50], src1[50], dst[50];
    struct wined3d_shader_src_param src0_copy = ins->src[0];
    BOOL need_abs = FALSE;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    const char *one = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ONE);

    /* POW operates on the absolute value of the input */
    src0_copy.modifiers = abs_modifier(src0_copy.modifiers, &need_abs);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst);
    shader_arb_get_src_param(ins, &src0_copy, 0, src0);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src1);

    if (need_abs)
        shader_addline(buffer, "ABS TA.x, %s;\n", src0);
    else
        shader_addline(buffer, "MOV TA.x, %s;\n", src0);

    if (priv->target_version >= NV2)
    {
        shader_addline(buffer, "MOVC TA.y, %s;\n", src1);
        shader_addline(buffer, "POW%s %s, TA.x, TA.y;\n", shader_arb_get_modifier(ins), dst);
        shader_addline(buffer, "MOV %s (EQ.y), %s;\n", dst, one);
    }
    else
    {
        const char *zero = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_ZERO);
        const char *flt_eps = arb_get_helper_value(ins->ctx->reg_maps->shader_version.type, ARB_EPS);

        shader_addline(buffer, "ABS TA.y, %s;\n", src1);
        shader_addline(buffer, "SGE TA.y, -TA.y, %s;\n", zero);
        /* Possibly add flt_eps to avoid getting float special values */
        shader_addline(buffer, "MAD TA.z, TA.y, %s, %s;\n", flt_eps, src1);
        shader_addline(buffer, "POW%s TA.x, TA.x, TA.z;\n", shader_arb_get_modifier(ins));
        shader_addline(buffer, "MAD TA.x, -TA.x, TA.y, TA.x;\n");
        shader_addline(buffer, "MAD %s, TA.y, %s, TA.x;\n", dst, one);
    }
}

static void shader_hw_loop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char src_name[50];
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    /* src0 is aL */
    shader_arb_get_src_param(ins, &ins->src[1], 0, src_name);

    if(vshader)
    {
        struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
        struct list *e = list_head(&priv->control_frames);
        struct control_frame *control_frame = LIST_ENTRY(e, struct control_frame, entry);

        if(priv->loop_depth > 1) shader_addline(buffer, "PUSHA aL;\n");
        /* The constant loader makes sure to load -1 into iX.w */
        shader_addline(buffer, "ARLC aL, %s.xywz;\n", src_name);
        shader_addline(buffer, "BRA loop_%u_end (LE.x);\n", control_frame->no.loop);
        shader_addline(buffer, "loop_%u_start:\n", control_frame->no.loop);
    }
    else
    {
        shader_addline(buffer, "LOOP %s;\n", src_name);
    }
}

static void shader_hw_rep(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    char src_name[50];
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name);

    /* The constant loader makes sure to load -1 into iX.w */
    if(vshader)
    {
        struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
        struct list *e = list_head(&priv->control_frames);
        struct control_frame *control_frame = LIST_ENTRY(e, struct control_frame, entry);

        if(priv->loop_depth > 1) shader_addline(buffer, "PUSHA aL;\n");

        shader_addline(buffer, "ARLC aL, %s.xywz;\n", src_name);
        shader_addline(buffer, "BRA loop_%u_end (LE.x);\n", control_frame->no.loop);
        shader_addline(buffer, "loop_%u_start:\n", control_frame->no.loop);
    }
    else
    {
        shader_addline(buffer, "REP %s;\n", src_name);
    }
}

static void shader_hw_endloop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    if(vshader)
    {
        struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
        struct list *e = list_head(&priv->control_frames);
        struct control_frame *control_frame = LIST_ENTRY(e, struct control_frame, entry);

        shader_addline(buffer, "ARAC aL.xy, aL;\n");
        shader_addline(buffer, "BRA loop_%u_start (GT.x);\n", control_frame->no.loop);
        shader_addline(buffer, "loop_%u_end:\n", control_frame->no.loop);

        if(priv->loop_depth > 1) shader_addline(buffer, "POPA aL;\n");
    }
    else
    {
        shader_addline(buffer, "ENDLOOP;\n");
    }
}

static void shader_hw_endrep(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    if(vshader)
    {
        struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
        struct list *e = list_head(&priv->control_frames);
        struct control_frame *control_frame = LIST_ENTRY(e, struct control_frame, entry);

        shader_addline(buffer, "ARAC aL.xy, aL;\n");
        shader_addline(buffer, "BRA loop_%u_start (GT.x);\n", control_frame->no.loop);
        shader_addline(buffer, "loop_%u_end:\n", control_frame->no.loop);

        if(priv->loop_depth > 1) shader_addline(buffer, "POPA aL;\n");
    }
    else
    {
        shader_addline(buffer, "ENDREP;\n");
    }
}

static const struct control_frame *find_last_loop(const struct shader_arb_ctx_priv *priv)
{
    struct control_frame *control_frame;

    LIST_FOR_EACH_ENTRY(control_frame, &priv->control_frames, struct control_frame, entry)
    {
        if(control_frame->type == LOOP || control_frame->type == REP) return control_frame;
    }
    ERR("Could not find loop for break\n");
    return NULL;
}

static void shader_hw_break(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    const struct control_frame *control_frame = find_last_loop(ins->ctx->backend_data);
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    if(vshader)
    {
        shader_addline(buffer, "BRA loop_%u_end;\n", control_frame->no.loop);
    }
    else
    {
        shader_addline(buffer, "BRK;\n");
    }
}

static const char *get_compare(enum wined3d_shader_rel_op op)
{
    switch (op)
    {
        case WINED3D_SHADER_REL_OP_GT: return "GT";
        case WINED3D_SHADER_REL_OP_EQ: return "EQ";
        case WINED3D_SHADER_REL_OP_GE: return "GE";
        case WINED3D_SHADER_REL_OP_LT: return "LT";
        case WINED3D_SHADER_REL_OP_NE: return "NE";
        case WINED3D_SHADER_REL_OP_LE: return "LE";
        default:
            FIXME("Unrecognized operator %#x.\n", op);
            return "(\?\?)";
    }
}

static enum wined3d_shader_rel_op invert_compare(enum wined3d_shader_rel_op op)
{
    switch (op)
    {
        case WINED3D_SHADER_REL_OP_GT: return WINED3D_SHADER_REL_OP_LE;
        case WINED3D_SHADER_REL_OP_EQ: return WINED3D_SHADER_REL_OP_NE;
        case WINED3D_SHADER_REL_OP_GE: return WINED3D_SHADER_REL_OP_LT;
        case WINED3D_SHADER_REL_OP_LT: return WINED3D_SHADER_REL_OP_GE;
        case WINED3D_SHADER_REL_OP_NE: return WINED3D_SHADER_REL_OP_EQ;
        case WINED3D_SHADER_REL_OP_LE: return WINED3D_SHADER_REL_OP_GT;
        default:
            FIXME("Unrecognized operator %#x.\n", op);
            return -1;
    }
}

static void shader_hw_breakc(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);
    const struct control_frame *control_frame = find_last_loop(ins->ctx->backend_data);
    char src_name0[50];
    char src_name1[50];
    const char *comp = get_compare(ins->flags);

    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name0);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name1);

    if(vshader)
    {
        /* SUBC CC, src0, src1" works only in pixel shaders, so use TA to throw
         * away the subtraction result
         */
        shader_addline(buffer, "SUBC TA, %s, %s;\n", src_name0, src_name1);
        shader_addline(buffer, "BRA loop_%u_end (%s.x);\n", control_frame->no.loop, comp);
    }
    else
    {
        shader_addline(buffer, "SUBC TA, %s, %s;\n", src_name0, src_name1);
        shader_addline(buffer, "BRK (%s.x);\n", comp);
    }
}

static void shader_hw_ifc(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct list *e = list_head(&priv->control_frames);
    struct control_frame *control_frame = LIST_ENTRY(e, struct control_frame, entry);
    const char *comp;
    char src_name0[50];
    char src_name1[50];
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name0);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name1);

    if(vshader)
    {
        /* Invert the flag. We jump to the else label if the condition is NOT true */
        comp = get_compare(invert_compare(ins->flags));
        shader_addline(buffer, "SUBC TA, %s, %s;\n", src_name0, src_name1);
        shader_addline(buffer, "BRA ifc_%u_else (%s.x);\n", control_frame->no.ifc, comp);
    }
    else
    {
        comp = get_compare(ins->flags);
        shader_addline(buffer, "SUBC TA, %s, %s;\n", src_name0, src_name1);
        shader_addline(buffer, "IF %s.x;\n", comp);
    }
}

static void shader_hw_else(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct list *e = list_head(&priv->control_frames);
    struct control_frame *control_frame = LIST_ENTRY(e, struct control_frame, entry);
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    if(vshader)
    {
        shader_addline(buffer, "BRA ifc_%u_endif;\n", control_frame->no.ifc);
        shader_addline(buffer, "ifc_%u_else:\n", control_frame->no.ifc);
        control_frame->had_else = TRUE;
    }
    else
    {
        shader_addline(buffer, "ELSE;\n");
    }
}

static void shader_hw_endif(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct list *e = list_head(&priv->control_frames);
    struct control_frame *control_frame = LIST_ENTRY(e, struct control_frame, entry);
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    if(vshader)
    {
        if(control_frame->had_else)
        {
            shader_addline(buffer, "ifc_%u_endif:\n", control_frame->no.ifc);
        }
        else
        {
            shader_addline(buffer, "#No else branch. else is endif\n");
            shader_addline(buffer, "ifc_%u_else:\n", control_frame->no.ifc);
        }
    }
    else
    {
        shader_addline(buffer, "ENDIF;\n");
    }
}

static void shader_hw_texldd(const struct wined3d_shader_instruction *ins)
{
    DWORD sampler_idx = ins->src[1].reg.idx[0].offset;
    char reg_dest[40];
    char reg_src[3][40];
    WORD flags = TEX_DERIV;

    shader_arb_get_dst_param(ins, &ins->dst[0], reg_dest);
    shader_arb_get_src_param(ins, &ins->src[0], 0, reg_src[0]);
    shader_arb_get_src_param(ins, &ins->src[2], 1, reg_src[1]);
    shader_arb_get_src_param(ins, &ins->src[3], 2, reg_src[2]);

    if (ins->flags & WINED3DSI_TEXLD_PROJECT) flags |= TEX_PROJ;
    if (ins->flags & WINED3DSI_TEXLD_BIAS) flags |= TEX_BIAS;

    shader_hw_sample(ins, sampler_idx, reg_dest, reg_src[0], flags, reg_src[1], reg_src[2]);
}

static void shader_hw_texldl(const struct wined3d_shader_instruction *ins)
{
    DWORD sampler_idx = ins->src[1].reg.idx[0].offset;
    char reg_dest[40];
    char reg_coord[40];
    WORD flags = TEX_LOD;

    shader_arb_get_dst_param(ins, &ins->dst[0], reg_dest);
    shader_arb_get_src_param(ins, &ins->src[0], 0, reg_coord);

    if (ins->flags & WINED3DSI_TEXLD_PROJECT) flags |= TEX_PROJ;
    if (ins->flags & WINED3DSI_TEXLD_BIAS) flags |= TEX_BIAS;

    shader_hw_sample(ins, sampler_idx, reg_dest, reg_coord, flags, NULL, NULL);
}

static void shader_hw_label(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;

    priv->in_main_func = FALSE;
    /* Call instructions activate the NV extensions, not labels and rets. If there is an uncalled
     * subroutine, don't generate a label that will make GL complain
     */
    if(priv->target_version == ARB) return;

    shader_addline(buffer, "l%u:\n", ins->src[0].reg.idx[0].offset);
}

static void vshader_add_footer(struct shader_arb_ctx_priv *priv_ctx,
        const struct arb_vshader_private *shader_data, const struct arb_vs_compile_args *args,
        const struct wined3d_shader_reg_maps *reg_maps, const struct wined3d_gl_info *gl_info,
        struct wined3d_string_buffer *buffer)
{
    unsigned int i;

    /* The D3DRS_FOGTABLEMODE render state defines if the shader-generated fog coord is used
     * or if the fragment depth is used. If the fragment depth is used(FOGTABLEMODE != NONE),
     * the fog frag coord is thrown away. If the fog frag coord is used, but not written by
     * the shader, it is set to 0.0(fully fogged, since start = 1.0, end = 0.0)
     */
    if (args->super.fog_src == VS_FOG_Z)
    {
        shader_addline(buffer, "MOV result.fogcoord, TMP_OUT.z;\n");
    }
    else
    {
        if (!reg_maps->fog)
        {
            /* posFixup.x is always 1.0, so we can safely use it */
            shader_addline(buffer, "ADD result.fogcoord, posFixup.x, -posFixup.x;\n");
        }
        else
        {
            /* Clamp fogcoord */
            const char *zero = arb_get_helper_value(reg_maps->shader_version.type, ARB_ZERO);
            const char *one = arb_get_helper_value(reg_maps->shader_version.type, ARB_ONE);

            shader_addline(buffer, "MIN TMP_FOGCOORD.x, TMP_FOGCOORD.x, %s;\n", one);
            shader_addline(buffer, "MAX result.fogcoord.x, TMP_FOGCOORD.x, %s;\n", zero);
        }
    }

    /* Clipplanes are always stored without y inversion */
    if (use_nv_clip(gl_info) && priv_ctx->target_version >= NV2)
    {
        if (args->super.clip_enabled)
        {
            for (i = 0; i < priv_ctx->vs_clipplanes; i++)
            {
                shader_addline(buffer, "DP4 result.clip[%u].x, TMP_OUT, state.clip[%u].plane;\n", i, i);
            }
        }
    }
    else if (args->clip.boolclip.clip_texcoord)
    {
        static const char component[4] = {'x', 'y', 'z', 'w'};
        unsigned int cur_clip = 0;
        const char *zero = arb_get_helper_value(WINED3D_SHADER_TYPE_VERTEX, ARB_ZERO);

        for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
        {
            if (args->clip.boolclip.clipplane_mask & (1u << i))
            {
                shader_addline(buffer, "DP4 TA.%c, TMP_OUT, state.clip[%u].plane;\n",
                               component[cur_clip++], i);
            }
        }
        switch (cur_clip)
        {
            case 0:
                shader_addline(buffer, "MOV TA, %s;\n", zero);
                break;
            case 1:
                shader_addline(buffer, "MOV TA.yzw, %s;\n", zero);
                break;
            case 2:
                shader_addline(buffer, "MOV TA.zw, %s;\n", zero);
                break;
            case 3:
                shader_addline(buffer, "MOV TA.w, %s;\n", zero);
                break;
        }
        shader_addline(buffer, "MOV result.texcoord[%u], TA;\n",
                       args->clip.boolclip.clip_texcoord - 1);
    }

    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while d3d coords specify
     * the corner. The offsets are stored in z and w in posFixup. posFixup.y contains
     * 1.0 or -1.0 to turn the rendering upside down for offscreen rendering. PosFixup.x
     * contains 1.0 to allow a mad, but arb vs swizzles are too restricted for that.
     */
    if (!gl_info->supported[ARB_CLIP_CONTROL])
    {
        shader_addline(buffer, "MUL TA, posFixup, TMP_OUT.w;\n");
        shader_addline(buffer, "ADD TMP_OUT.x, TMP_OUT.x, TA.z;\n");
        shader_addline(buffer, "MAD TMP_OUT.y, TMP_OUT.y, posFixup.y, TA.w;\n");

        /* Z coord [0;1]->[-1;1] mapping, see comment in
         * get_projection_matrix() in utils.c. */
        if (need_helper_const(shader_data, reg_maps, gl_info))
        {
            const char *two = arb_get_helper_value(WINED3D_SHADER_TYPE_VERTEX, ARB_TWO);
            shader_addline(buffer, "MAD TMP_OUT.z, TMP_OUT.z, %s, -TMP_OUT.w;\n", two);
        }
        else
        {
            shader_addline(buffer, "ADD TMP_OUT.z, TMP_OUT.z, TMP_OUT.z;\n");
            shader_addline(buffer, "ADD TMP_OUT.z, TMP_OUT.z, -TMP_OUT.w;\n");
        }
    }

    shader_addline(buffer, "MOV result.position, TMP_OUT;\n");

    priv_ctx->footer_written = TRUE;
}

static void shader_hw_ret(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    const struct wined3d_shader *shader = ins->ctx->shader;
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    if(priv->target_version == ARB) return;

    if(vshader)
    {
        if (priv->in_main_func) vshader_add_footer(priv, shader->backend_data,
                priv->cur_vs_args, ins->ctx->reg_maps, ins->ctx->gl_info, buffer);
    }

    shader_addline(buffer, "RET;\n");
}

static void shader_hw_call(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    shader_addline(buffer, "CAL l%u;\n", ins->src[0].reg.idx[0].offset);
}

static BOOL shader_arb_compile(const struct wined3d_gl_info *gl_info, GLenum target, const char *src)
{
    const char *ptr, *line;
    GLint native, pos;

    if (TRACE_ON(d3d_shader))
    {
        ptr = src;
        while ((line = get_line(&ptr))) TRACE_(d3d_shader)("    %.*s", (int)(ptr - line), line);
    }

    GL_EXTCALL(glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(src), src));
    checkGLcall("glProgramStringARB()");

    if (FIXME_ON(d3d_shader))
    {
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
        if (pos != -1)
        {
            FIXME_(d3d_shader)("Program error at position %d: %s\n\n", pos,
                    debugstr_a((const char *)gl_info->gl_ops.gl.p_glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
            ptr = src;
            while ((line = get_line(&ptr))) FIXME_(d3d_shader)("    %.*s", (int)(ptr - line), line);
            FIXME_(d3d_shader)("\n");

            return FALSE;
        }
    }

    if (WARN_ON(d3d_perf))
    {
        GL_EXTCALL(glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native));
        checkGLcall("glGetProgramivARB()");
        if (!native)
            WARN_(d3d_perf)("Program exceeds native resource limits.\n");
    }

    return TRUE;
}

static void arbfp_add_sRGB_correction(struct wined3d_string_buffer *buffer, const char *fragcolor,
        const char *tmp1, const char *tmp2, const char *tmp3, const char *tmp4, BOOL condcode)
{
    /* Perform sRGB write correction. See GLX_EXT_framebuffer_sRGB */

    if(condcode)
    {
        /* Sigh. MOVC CC doesn't work, so use one of the temps as dummy dest */
        shader_addline(buffer, "SUBC %s, %s.x, srgb_consts1.x;\n", tmp1, fragcolor);
        /* Calculate the > 0.0031308 case */
        shader_addline(buffer, "POW %s.x (GE), %s.x, srgb_consts0.x;\n", fragcolor, fragcolor);
        shader_addline(buffer, "POW %s.y (GE), %s.y, srgb_consts0.x;\n", fragcolor, fragcolor);
        shader_addline(buffer, "POW %s.z (GE), %s.z, srgb_consts0.x;\n", fragcolor, fragcolor);
        shader_addline(buffer, "MUL %s.xyz (GE), %s, srgb_consts0.y;\n", fragcolor, fragcolor);
        shader_addline(buffer, "SUB %s.xyz (GE), %s, srgb_consts0.z;\n", fragcolor, fragcolor);
        /* Calculate the < case */
        shader_addline(buffer, "MUL %s.xyz (LT), srgb_consts0.w, %s;\n", fragcolor, fragcolor);
    }
    else
    {
        /* Calculate the > 0.0031308 case */
        shader_addline(buffer, "POW %s.x, %s.x, srgb_consts0.x;\n", tmp1, fragcolor);
        shader_addline(buffer, "POW %s.y, %s.y, srgb_consts0.x;\n", tmp1, fragcolor);
        shader_addline(buffer, "POW %s.z, %s.z, srgb_consts0.x;\n", tmp1, fragcolor);
        shader_addline(buffer, "MUL %s, %s, srgb_consts0.y;\n", tmp1, tmp1);
        shader_addline(buffer, "SUB %s, %s, srgb_consts0.z;\n", tmp1, tmp1);
        /* Calculate the < case */
        shader_addline(buffer, "MUL %s, srgb_consts0.w, %s;\n", tmp2, fragcolor);
        /* Get 1.0 / 0.0 masks for > 0.0031308 and < 0.0031308 */
        shader_addline(buffer, "SLT %s, srgb_consts1.x, %s;\n", tmp3, fragcolor);
        shader_addline(buffer, "SGE %s, srgb_consts1.x, %s;\n", tmp4, fragcolor);
        /* Store the components > 0.0031308 in the destination */
        shader_addline(buffer, "MUL %s.xyz, %s, %s;\n", fragcolor, tmp1, tmp3);
        /* Add the components that are < 0.0031308 */
        shader_addline(buffer, "MAD %s.xyz, %s, %s, %s;\n", fragcolor, tmp2, tmp4, fragcolor);
        /* Move everything into result.color at once. Nvidia hardware cannot handle partial
        * result.color writes(.rgb first, then .a), or handle overwriting already written
        * components. The assembler uses a temporary register in this case, which is usually
        * not allocated from one of our registers that were used earlier.
        */
    }
    /* [0.0;1.0] clamping. Not needed, this is done implicitly */
}

static const DWORD *find_loop_control_values(const struct wined3d_shader *shader, DWORD idx)
{
    const struct wined3d_shader_lconst *constant;

    LIST_FOR_EACH_ENTRY(constant, &shader->constantsI, struct wined3d_shader_lconst, entry)
    {
        if (constant->idx == idx)
        {
            return constant->value;
        }
    }
    return NULL;
}

static void init_ps_input(const struct wined3d_shader *shader,
        const struct arb_ps_compile_args *args, struct shader_arb_ctx_priv *priv)
{
    static const char * const texcoords[8] =
    {
        "fragment.texcoord[0]", "fragment.texcoord[1]", "fragment.texcoord[2]", "fragment.texcoord[3]",
        "fragment.texcoord[4]", "fragment.texcoord[5]", "fragment.texcoord[6]", "fragment.texcoord[7]"
    };
    unsigned int i;
    const struct wined3d_shader_signature_element *input;
    const char *semantic_name;
    DWORD semantic_idx;

    if (args->super.vp_mode == WINED3D_VP_MODE_SHADER)
    {
        /* That one is easy. The vertex shaders provide v0-v7 in
         * fragment.texcoord and v8 and v9 in fragment.color. */
        for (i = 0; i < 8; ++i)
        {
            priv->ps_input[i] = texcoords[i];
        }
        priv->ps_input[8] = "fragment.color.primary";
        priv->ps_input[9] = "fragment.color.secondary";
        return;
    }

    /* The fragment shader has to collect the varyings on its own. In any case
     * properly load color0 and color1. In the case of pre-transformed
     * vertices also load texture coordinates. Set other attributes to 0.0.
     *
     * For fixed-function this behavior is correct, according to the tests.
     * For pre-transformed we'd either need a replacement shader that can load
     * other attributes like BINORMAL, or load the texture coordinate
     * attribute pointers to match the fragment shader signature. */
    for (i = 0; i < shader->input_signature.element_count; ++i)
    {
        input = &shader->input_signature.elements[i];
        if (!(semantic_name = input->semantic_name))
            continue;
        semantic_idx = input->semantic_idx;

        if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_COLOR))
        {
            if (!semantic_idx)
                priv->ps_input[input->register_idx] = "fragment.color.primary";
            else if (semantic_idx == 1)
                priv->ps_input[input->register_idx] = "fragment.color.secondary";
            else
                priv->ps_input[input->register_idx] = "0.0";
        }
        else if (args->super.vp_mode == WINED3D_VP_MODE_FF)
        {
            priv->ps_input[input->register_idx] = "0.0";
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
        {
            if (semantic_idx < 8)
                priv->ps_input[input->register_idx] = texcoords[semantic_idx];
            else
                priv->ps_input[input->register_idx] = "0.0";
        }
        else if (shader_match_semantic(semantic_name, WINED3D_DECL_USAGE_FOG))
        {
            if (!semantic_idx)
                priv->ps_input[input->register_idx] = "fragment.fogcoord";
            else
                priv->ps_input[input->register_idx] = "0.0";
        }
        else
        {
            priv->ps_input[input->register_idx] = "0.0";
        }

        TRACE("v%u, semantic %s%u is %s\n", input->register_idx,
                semantic_name, semantic_idx, priv->ps_input[input->register_idx]);
    }
}

static void arbfp_add_linear_fog(struct wined3d_string_buffer *buffer,
        const char *fragcolor, const char *tmp)
{
    shader_addline(buffer, "SUB %s.x, state.fog.params.z, fragment.fogcoord.x;\n", tmp);
    shader_addline(buffer, "MUL_SAT %s.x, %s.x, state.fog.params.w;\n", tmp, tmp);
    shader_addline(buffer, "LRP %s.rgb, %s.x, %s, state.fog.color;\n", fragcolor, tmp, fragcolor);
}

/* Context activation is done by the caller. */
static GLuint shader_arb_generate_pshader(const struct wined3d_shader *shader,
        const struct wined3d_gl_info *gl_info, struct wined3d_string_buffer *buffer,
        const struct arb_ps_compile_args *args, struct arb_ps_compiled_shader *compiled)
{
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    GLuint retval;
    char fragcolor[16];
    DWORD next_local = 0;
    struct shader_arb_ctx_priv priv_ctx;
    BOOL dcl_td = FALSE;
    BOOL want_nv_prog = FALSE;
    struct arb_pshader_private *shader_priv = shader->backend_data;
    DWORD map;
    BOOL custom_linear_fog = FALSE;

    char srgbtmp[4][4];
    char ftoa_tmp[17];
    unsigned int i, found = 0;

    for (i = 0, map = reg_maps->temporary; map; map >>= 1, ++i)
    {
        if (!(map & 1)
                || (shader->u.ps.color0_mov && i == shader->u.ps.color0_reg)
                || (reg_maps->shader_version.major < 2 && !i))
            continue;

        sprintf(srgbtmp[found], "R%u", i);
        ++found;
        if (found == 4) break;
    }

    switch(found) {
        case 0:
            sprintf(srgbtmp[0], "TA");
            sprintf(srgbtmp[1], "TB");
            sprintf(srgbtmp[2], "TC");
            sprintf(srgbtmp[3], "TD");
            dcl_td = TRUE;
            break;
        case 1:
            sprintf(srgbtmp[1], "TA");
            sprintf(srgbtmp[2], "TB");
            sprintf(srgbtmp[3], "TC");
            break;
        case 2:
            sprintf(srgbtmp[2], "TA");
            sprintf(srgbtmp[3], "TB");
            break;
        case 3:
            sprintf(srgbtmp[3], "TA");
            break;
        case 4:
            break;
    }

    /*  Create the hw ARB shader */
    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_ps_args = args;
    priv_ctx.compiled_fprog = compiled;
    priv_ctx.cur_np2fixup_info = &compiled->np2fixup_info;
    init_ps_input(shader, args, &priv_ctx);
    list_init(&priv_ctx.control_frames);
    priv_ctx.ps_post_process = args->super.srgb_correction;

    /* Avoid enabling NV_fragment_program* if we do not need it.
     *
     * Enabling GL_NV_fragment_program_option causes the driver to occupy a temporary register,
     * and it slows down the shader execution noticeably(about 5%). Usually our instruction emulation
     * is faster than what we gain from using higher native instructions. There are some things though
     * that cannot be emulated. In that case enable the extensions.
     * If the extension is enabled, instruction handlers that support both ways will use it.
     *
     * Testing shows no performance difference between OPTION NV_fragment_program2 and NV_fragment_program.
     * So enable the best we can get.
     */
    if(reg_maps->usesdsx || reg_maps->usesdsy || reg_maps->loop_depth > 0 || reg_maps->usestexldd ||
       reg_maps->usestexldl || reg_maps->usesfacing || reg_maps->usesifc || reg_maps->usescall)
    {
        want_nv_prog = TRUE;
    }

    shader_addline(buffer, "!!ARBfp1.0\n");
    if (want_nv_prog && gl_info->supported[NV_FRAGMENT_PROGRAM2])
    {
        shader_addline(buffer, "OPTION NV_fragment_program2;\n");
        priv_ctx.target_version = NV3;
    }
    else if (want_nv_prog && gl_info->supported[NV_FRAGMENT_PROGRAM_OPTION])
    {
        shader_addline(buffer, "OPTION NV_fragment_program;\n");
        priv_ctx.target_version = NV2;
    } else {
        if(want_nv_prog)
        {
            /* This is an error - either we're advertising the wrong shader version, or aren't enforcing some
             * limits properly
             */
            ERR("The shader requires instructions that are not available in plain GL_ARB_fragment_program\n");
            ERR("Try GLSL\n");
        }
        priv_ctx.target_version = ARB;
    }

    if (reg_maps->rt_mask > 1)
    {
        shader_addline(buffer, "OPTION ARB_draw_buffers;\n");
    }

    if (reg_maps->shader_version.major < 3)
    {
        switch (args->super.fog)
        {
            case WINED3D_FFP_PS_FOG_OFF:
                break;
            case WINED3D_FFP_PS_FOG_LINEAR:
                if (gl_info->quirks & WINED3D_QUIRK_BROKEN_ARB_FOG)
                {
                    custom_linear_fog = TRUE;
                    priv_ctx.ps_post_process = TRUE;
                    break;
                }
                shader_addline(buffer, "OPTION ARB_fog_linear;\n");
                break;
            case WINED3D_FFP_PS_FOG_EXP:
                shader_addline(buffer, "OPTION ARB_fog_exp;\n");
                break;
            case WINED3D_FFP_PS_FOG_EXP2:
                shader_addline(buffer, "OPTION ARB_fog_exp2;\n");
                break;
        }
    }

    /* For now always declare the temps. At least the Nvidia assembler optimizes completely
     * unused temps away(but occupies them for the whole shader if they're used once). Always
     * declaring them avoids tricky bookkeeping work
     */
    shader_addline(buffer, "TEMP TA;\n");      /* Used for modifiers */
    shader_addline(buffer, "TEMP TB;\n");      /* Used for modifiers */
    shader_addline(buffer, "TEMP TC;\n");      /* Used for modifiers */
    if(dcl_td) shader_addline(buffer, "TEMP TD;\n"); /* Used for sRGB writing */
    shader_addline(buffer, "PARAM coefdiv = { 0.5, 0.25, 0.125, 0.0625 };\n");
    shader_addline(buffer, "PARAM coefmul = { 2, 4, 8, 16 };\n");
    wined3d_ftoa(eps, ftoa_tmp);
    shader_addline(buffer, "PARAM ps_helper_const = { 0.0, 1.0, %s, 0.0 };\n", ftoa_tmp);

    if (reg_maps->shader_version.major < 2)
    {
        strcpy(fragcolor, "R0");
    }
    else
    {
        if (priv_ctx.ps_post_process)
        {
            if (shader->u.ps.color0_mov)
            {
                sprintf(fragcolor, "R%u", shader->u.ps.color0_reg);
            }
            else
            {
                shader_addline(buffer, "TEMP TMP_COLOR;\n");
                strcpy(fragcolor, "TMP_COLOR");
            }
        } else {
            strcpy(fragcolor, "result.color");
        }
    }

    if (args->super.srgb_correction)
    {
        shader_addline(buffer, "PARAM srgb_consts0 = ");
        shader_arb_append_imm_vec4(buffer, &wined3d_srgb_const[0].x);
        shader_addline(buffer, ";\n");
        shader_addline(buffer, "PARAM srgb_consts1 = ");
        shader_arb_append_imm_vec4(buffer, &wined3d_srgb_const[1].x);
        shader_addline(buffer, ";\n");
    }

    /* Base Declarations */
    shader_generate_arb_declarations(shader, reg_maps, buffer, gl_info, NULL, &priv_ctx);

    for (i = 0, map = reg_maps->bumpmat; map; map >>= 1, ++i)
    {
        unsigned char bump_const;

        if (!(map & 1)) continue;

        bump_const = compiled->numbumpenvmatconsts;
        compiled->bumpenvmatconst[bump_const].const_num = WINED3D_CONST_NUM_UNUSED;
        compiled->bumpenvmatconst[bump_const].texunit = i;
        compiled->luminanceconst[bump_const].const_num = WINED3D_CONST_NUM_UNUSED;
        compiled->luminanceconst[bump_const].texunit = i;

        /* We can fit the constants into the constant limit for sure because texbem, texbeml, bem and beml are only supported
         * in 1.x shaders, and GL_ARB_fragment_program has a constant limit of 24 constants. So in the worst case we're loading
         * 8 shader constants, 8 bump matrices and 8 luminance parameters and are perfectly fine. (No NP2 fixup on bumpmapped
         * textures due to conditional NP2 restrictions)
         *
         * Use local constants to load the bump env parameters, not program.env. This avoids collisions with d3d constants of
         * shaders in newer shader models. Since the bump env parameters have to share their space with NP2 fixup constants,
         * their location is shader dependent anyway and they cannot be loaded globally.
         */
        compiled->bumpenvmatconst[bump_const].const_num = next_local++;
        shader_addline(buffer, "PARAM bumpenvmat%d = program.local[%d];\n",
                       i, compiled->bumpenvmatconst[bump_const].const_num);
        compiled->numbumpenvmatconsts = bump_const + 1;

        if (!(reg_maps->luminanceparams & (1u << i)))
            continue;

        compiled->luminanceconst[bump_const].const_num = next_local++;
        shader_addline(buffer, "PARAM luminance%d = program.local[%d];\n",
                       i, compiled->luminanceconst[bump_const].const_num);
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        compiled->int_consts[i] = WINED3D_CONST_NUM_UNUSED;
        if (reg_maps->integer_constants & (1u << i) && priv_ctx.target_version >= NV2)
        {
            const DWORD *control_values = find_loop_control_values(shader, i);

            if(control_values)
            {
                shader_addline(buffer, "PARAM I%u = {%u, %u, %u, -1};\n", i,
                                control_values[0], control_values[1], control_values[2]);
            }
            else
            {
                compiled->int_consts[i] = next_local;
                compiled->num_int_consts++;
                shader_addline(buffer, "PARAM I%u = program.local[%u];\n", i, next_local++);
            }
        }
    }

    if(reg_maps->vpos || reg_maps->usesdsy)
    {
        compiled->ycorrection = next_local;
        shader_addline(buffer, "PARAM ycorrection = program.local[%u];\n", next_local++);

        if(reg_maps->vpos)
        {
            shader_addline(buffer, "TEMP vpos;\n");
            /* ycorrection.x: Backbuffer height(onscreen) or 0(offscreen).
             * ycorrection.y: -1.0(onscreen), 1.0(offscreen)
             * ycorrection.z: 1.0
             * ycorrection.w: 0.0
             */
            shader_addline(buffer, "MAD vpos, fragment.position, ycorrection.zyww, ycorrection.wxww;\n");
            shader_addline(buffer, "FLR vpos.xy, vpos;\n");
        }
    }
    else
    {
        compiled->ycorrection = WINED3D_CONST_NUM_UNUSED;
    }

    /* Load constants to fixup NP2 texcoords if there are still free constants left:
     * Constants (texture dimensions) for the NP2 fixup are loaded as local program parameters. This will consume
     * at most 8 (WINED3D_MAX_FRAGMENT_SAMPLERS / 2) parameters, which is highly unlikely, since the application had to
     * use 16 NP2 textures at the same time. In case that we run out of constants the fixup is simply not
     * applied / activated. This will probably result in wrong rendering of the texture, but will save us from
     * shader compilation errors and the subsequent errors when drawing with this shader. */
    if (priv_ctx.cur_ps_args->super.np2_fixup) {
        unsigned char cur_fixup_sampler = 0;

        struct arb_ps_np2fixup_info* const fixup = priv_ctx.cur_np2fixup_info;
        const WORD map = priv_ctx.cur_ps_args->super.np2_fixup;
        const UINT max_lconsts = gl_info->limits.arb_ps_local_constants;

        fixup->offset = next_local;
        fixup->super.active = 0;

        for (i = 0; i < WINED3D_MAX_FRAGMENT_SAMPLERS; ++i)
        {
            if (!(map & (1u << i)))
                continue;

            if (fixup->offset + (cur_fixup_sampler >> 1) < max_lconsts)
            {
                fixup->super.active |= (1u << i);
                fixup->super.idx[i] = cur_fixup_sampler++;
            }
            else
            {
                FIXME("No free constant found to load NP2 fixup data into shader. "
                      "Sampling from this texture will probably look wrong.\n");
                break;
            }
        }

        fixup->super.num_consts = (cur_fixup_sampler + 1) >> 1;
        if (fixup->super.num_consts) {
            shader_addline(buffer, "PARAM np2fixup[%u] = { program.env[%u..%u] };\n",
                           fixup->super.num_consts, fixup->offset, fixup->super.num_consts + fixup->offset - 1);
        }
    }

    if (shader_priv->clipplane_emulation != ~0U && args->clip)
    {
        shader_addline(buffer, "KIL fragment.texcoord[%u];\n", shader_priv->clipplane_emulation);
    }

    /* Base Shader Body */
    if (FAILED(shader_generate_code(shader, buffer, reg_maps, &priv_ctx, NULL, NULL)))
        return 0;

    if(args->super.srgb_correction) {
        arbfp_add_sRGB_correction(buffer, fragcolor, srgbtmp[0], srgbtmp[1], srgbtmp[2], srgbtmp[3],
                                  priv_ctx.target_version >= NV2);
    }

    if (custom_linear_fog)
        arbfp_add_linear_fog(buffer, fragcolor, "TA");

    if(strcmp(fragcolor, "result.color")) {
        shader_addline(buffer, "MOV result.color, %s;\n", fragcolor);
    }
    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &retval));

    TRACE("Creating a hw pixel shader, prg=%d\n", retval);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, retval));

    TRACE("Created hw pixel shader, prg=%d\n", retval);
    if (!shader_arb_compile(gl_info, GL_FRAGMENT_PROGRAM_ARB, buffer->buffer))
        return 0;

    return retval;
}

static int compare_sig(const struct wined3d_shader_signature *sig1, const struct wined3d_shader_signature *sig2)
{
    unsigned int i;
    int ret;

    if (sig1->element_count != sig2->element_count)
        return sig1->element_count < sig2->element_count ? -1 : 1;

    for (i = 0; i < sig1->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *e1, *e2;

        e1 = &sig1->elements[i];
        e2 = &sig2->elements[i];

        if (!e1->semantic_name || !e2->semantic_name)
        {
            /* Compare pointers, not contents. One string is NULL (element
             * does not exist), the other one is not NULL. */
            if (e1->semantic_name != e2->semantic_name)
                return e1->semantic_name < e2->semantic_name ? -1 : 1;
            continue;
        }

        if ((ret = strcmp(e1->semantic_name, e2->semantic_name)))
            return ret;
        if (e1->semantic_idx != e2->semantic_idx)
            return e1->semantic_idx < e2->semantic_idx ? -1 : 1;
        if (e1->sysval_semantic != e2->sysval_semantic)
            return e1->sysval_semantic < e2->sysval_semantic ? -1 : 1;
        if (e1->component_type != e2->component_type)
            return e1->component_type < e2->component_type ? -1 : 1;
        if (e1->register_idx != e2->register_idx)
            return e1->register_idx < e2->register_idx ? -1 : 1;
        if (e1->mask != e2->mask)
            return e1->mask < e2->mask ? -1 : 1;
    }
    return 0;
}

static void clone_sig(struct wined3d_shader_signature *new, const struct wined3d_shader_signature *sig)
{
    unsigned int i;
    char *name;

    new->element_count = sig->element_count;
    new->elements = heap_calloc(new->element_count, sizeof(*new->elements));
    for (i = 0; i < sig->element_count; ++i)
    {
        new->elements[i] = sig->elements[i];

        if (!new->elements[i].semantic_name)
            continue;

        /* Clone the semantic string */
        name = heap_alloc(strlen(sig->elements[i].semantic_name) + 1);
        strcpy(name, sig->elements[i].semantic_name);
        new->elements[i].semantic_name = name;
    }
}

static DWORD find_input_signature(struct shader_arb_priv *priv, const struct wined3d_shader_signature *sig)
{
    struct wine_rb_entry *entry = wine_rb_get(&priv->signature_tree, sig);
    struct ps_signature *found_sig;

    if (entry)
    {
        found_sig = WINE_RB_ENTRY_VALUE(entry, struct ps_signature, entry);
        TRACE("Found existing signature %u\n", found_sig->idx);
        return found_sig->idx;
    }
    found_sig = heap_alloc_zero(sizeof(*found_sig));
    clone_sig(&found_sig->sig, sig);
    found_sig->idx = priv->ps_sig_number++;
    TRACE("New signature stored and assigned number %u\n", found_sig->idx);
    if(wine_rb_put(&priv->signature_tree, sig, &found_sig->entry) == -1)
    {
        ERR("Failed to insert program entry.\n");
    }
    return found_sig->idx;
}

static void init_output_registers(const struct wined3d_shader *shader,
        const struct wined3d_shader_signature *ps_input_sig,
        struct shader_arb_ctx_priv *priv_ctx, struct arb_vs_compiled_shader *compiled)
{
    unsigned int i, j;
    static const char * const texcoords[8] =
    {
        "result.texcoord[0]", "result.texcoord[1]", "result.texcoord[2]", "result.texcoord[3]",
        "result.texcoord[4]", "result.texcoord[5]", "result.texcoord[6]", "result.texcoord[7]"
    };
    /* Write generic input varyings 0 to 7 to result.texcoord[], varying 8 to result.color.primary
     * and varying 9 to result.color.secondary
     */
    static const char * const decl_idx_to_string[MAX_REG_INPUT] =
    {
        "result.texcoord[0]", "result.texcoord[1]", "result.texcoord[2]", "result.texcoord[3]",
        "result.texcoord[4]", "result.texcoord[5]", "result.texcoord[6]", "result.texcoord[7]",
        "result.color.primary", "result.color.secondary"
    };

    if (!ps_input_sig)
    {
        TRACE("Pixel shader uses builtin varyings\n");
        /* Map builtins to builtins */
        for(i = 0; i < 8; i++)
        {
            priv_ctx->texcrd_output[i] = texcoords[i];
        }
        priv_ctx->color_output[0] = "result.color.primary";
        priv_ctx->color_output[1] = "result.color.secondary";
        priv_ctx->fog_output = "TMP_FOGCOORD";

        /* Map declared regs to builtins. Use "TA" to /dev/null unread output */
        for (i = 0; i < shader->output_signature.element_count; ++i)
        {
            const struct wined3d_shader_signature_element *output = &shader->output_signature.elements[i];

            if (!output->semantic_name)
                continue;

            if (shader_match_semantic(output->semantic_name, WINED3D_DECL_USAGE_POSITION))
            {
                TRACE("o%u is TMP_OUT\n", output->register_idx);
                if (!output->semantic_idx)
                    priv_ctx->vs_output[output->register_idx] = "TMP_OUT";
                else
                    priv_ctx->vs_output[output->register_idx] = "TA";
            }
            else if (shader_match_semantic(output->semantic_name, WINED3D_DECL_USAGE_PSIZE))
            {
                TRACE("o%u is result.pointsize\n", output->register_idx);
                if (!output->semantic_idx)
                    priv_ctx->vs_output[output->register_idx] = "result.pointsize";
                else
                    priv_ctx->vs_output[output->register_idx] = "TA";
            }
            else if (shader_match_semantic(output->semantic_name, WINED3D_DECL_USAGE_COLOR))
            {
                TRACE("o%u is result.color.?, idx %u\n", output->register_idx, output->semantic_idx);
                if (!output->semantic_idx)
                    priv_ctx->vs_output[output->register_idx] = "result.color.primary";
                else if (output->semantic_idx == 1)
                    priv_ctx->vs_output[output->register_idx] = "result.color.secondary";
                else priv_ctx->vs_output[output->register_idx] = "TA";
            }
            else if (shader_match_semantic(output->semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
            {
                TRACE("o%u is result.texcoord[%u]\n", output->register_idx, output->semantic_idx);
                if (output->semantic_idx >= 8)
                    priv_ctx->vs_output[output->register_idx] = "TA";
                else
                    priv_ctx->vs_output[output->register_idx] = texcoords[output->semantic_idx];
            }
            else if (shader_match_semantic(output->semantic_name, WINED3D_DECL_USAGE_FOG))
            {
                TRACE("o%u is result.fogcoord\n", output->register_idx);
                if (output->semantic_idx > 0)
                    priv_ctx->vs_output[output->register_idx] = "TA";
                else
                    priv_ctx->vs_output[output->register_idx] = "result.fogcoord";
            }
            else
            {
                priv_ctx->vs_output[output->register_idx] = "TA";
            }
        }
        return;
    }

    TRACE("Pixel shader uses declared varyings\n");

    /* Map builtin to declared. /dev/null the results by default to the TA temp reg */
    for(i = 0; i < 8; i++)
    {
        priv_ctx->texcrd_output[i] = "TA";
    }
    priv_ctx->color_output[0] = "TA";
    priv_ctx->color_output[1] = "TA";
    priv_ctx->fog_output = "TA";

    for (i = 0; i < ps_input_sig->element_count; ++i)
    {
        const struct wined3d_shader_signature_element *input = &ps_input_sig->elements[i];

        if (!input->semantic_name)
            continue;

        /* If a declared input register is not written by builtin arguments, don't write to it.
         * GL_NV_vertex_program makes sure the input defaults to 0.0, which is correct with D3D
         *
         * Don't care about POSITION and PSIZE here - this is a builtin vertex shader, position goes
         * to TMP_OUT in any case
         */
        if (shader_match_semantic(input->semantic_name, WINED3D_DECL_USAGE_TEXCOORD))
        {
            if (input->semantic_idx < 8)
                priv_ctx->texcrd_output[input->semantic_idx] = decl_idx_to_string[input->register_idx];
        }
        else if (shader_match_semantic(input->semantic_name, WINED3D_DECL_USAGE_COLOR))
        {
            if (input->semantic_idx < 2)
                priv_ctx->color_output[input->semantic_idx] = decl_idx_to_string[input->register_idx];
        }
        else if (shader_match_semantic(input->semantic_name, WINED3D_DECL_USAGE_FOG))
        {
            if (!input->semantic_idx)
                priv_ctx->fog_output = decl_idx_to_string[input->register_idx];
        }
        else
        {
            continue;
        }

        if (!strcmp(decl_idx_to_string[input->register_idx], "result.color.primary")
                || !strcmp(decl_idx_to_string[input->register_idx], "result.color.secondary"))
        {
            compiled->need_color_unclamp = TRUE;
        }
    }

    /* Map declared to declared */
    for (i = 0; i < shader->output_signature.element_count; ++i)
    {
        const struct wined3d_shader_signature_element *output = &shader->output_signature.elements[i];

        /* Write unread output to TA to throw them away */
        priv_ctx->vs_output[output->register_idx] = "TA";

        if (!output->semantic_name)
            continue;

        if (shader_match_semantic(output->semantic_name, WINED3D_DECL_USAGE_POSITION) && !output->semantic_idx)
        {
            priv_ctx->vs_output[output->register_idx] = "TMP_OUT";
            continue;
        }
        else if (shader_match_semantic(output->semantic_name, WINED3D_DECL_USAGE_PSIZE) && !output->semantic_idx)
        {
            priv_ctx->vs_output[output->register_idx] = "result.pointsize";
            continue;
        }

        for (j = 0; j < ps_input_sig->element_count; ++j)
        {
            const struct wined3d_shader_signature_element *input = &ps_input_sig->elements[j];

            if (!input->semantic_name)
                continue;

            if (!strcmp(input->semantic_name, output->semantic_name)
                    && input->semantic_idx == output->semantic_idx)
            {
                priv_ctx->vs_output[output->register_idx] = decl_idx_to_string[input->register_idx];

                if (!strcmp(priv_ctx->vs_output[output->register_idx], "result.color.primary")
                        || !strcmp(priv_ctx->vs_output[output->register_idx], "result.color.secondary"))
                {
                    compiled->need_color_unclamp = TRUE;
                }
            }
        }
    }
}

/* Context activation is done by the caller. */
static GLuint shader_arb_generate_vshader(const struct wined3d_shader *shader,
        const struct wined3d_gl_info *gl_info, struct wined3d_string_buffer *buffer,
        const struct arb_vs_compile_args *args, struct arb_vs_compiled_shader *compiled,
        const struct wined3d_shader_signature *ps_input_sig)
{
    const struct arb_vshader_private *shader_data = shader->backend_data;
    const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;
    struct shader_arb_priv *priv = shader->device->shader_priv;
    GLuint ret;
    DWORD next_local = 0;
    struct shader_arb_ctx_priv priv_ctx;
    unsigned int i;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_vs_args = args;
    list_init(&priv_ctx.control_frames);
    init_output_registers(shader, ps_input_sig, &priv_ctx, compiled);

    /*  Create the hw ARB shader */
    shader_addline(buffer, "!!ARBvp1.0\n");

    /* Always enable the NV extension if available. Unlike fragment shaders, there is no
     * mesurable performance penalty, and we can always make use of it for clipplanes.
     */
    if (gl_info->supported[NV_VERTEX_PROGRAM3])
    {
        shader_addline(buffer, "OPTION NV_vertex_program3;\n");
        priv_ctx.target_version = NV3;
        shader_addline(buffer, "ADDRESS aL;\n");
    }
    else if (gl_info->supported[NV_VERTEX_PROGRAM2_OPTION])
    {
        shader_addline(buffer, "OPTION NV_vertex_program2;\n");
        priv_ctx.target_version = NV2;
        shader_addline(buffer, "ADDRESS aL;\n");
    } else {
        priv_ctx.target_version = ARB;
    }

    shader_addline(buffer, "TEMP TMP_OUT;\n");
    if (reg_maps->fog)
        shader_addline(buffer, "TEMP TMP_FOGCOORD;\n");
    if (need_helper_const(shader_data, reg_maps, gl_info))
    {
        char ftoa_tmp[17];
        wined3d_ftoa(eps, ftoa_tmp);
        shader_addline(buffer, "PARAM helper_const = { 0.0, 1.0, 2.0, %s};\n", ftoa_tmp);
    }
    if (need_rel_addr_const(shader_data, reg_maps, gl_info))
    {
        shader_addline(buffer, "PARAM rel_addr_const = { 0.5, %d.0, 0.0, 0.0 };\n", shader_data->rel_offset);
        shader_addline(buffer, "TEMP A0_SHADOW;\n");
    }

    shader_addline(buffer, "TEMP TA;\n");
    shader_addline(buffer, "TEMP TB;\n");

    /* Base Declarations */
    shader_generate_arb_declarations(shader, reg_maps, buffer, gl_info,
            &priv_ctx.vs_clipplanes, &priv_ctx);

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        compiled->int_consts[i] = WINED3D_CONST_NUM_UNUSED;
        if (reg_maps->integer_constants & (1u << i) && priv_ctx.target_version >= NV2)
        {
            const DWORD *control_values = find_loop_control_values(shader, i);

            if(control_values)
            {
                shader_addline(buffer, "PARAM I%u = {%u, %u, %u, -1};\n", i,
                                control_values[0], control_values[1], control_values[2]);
            }
            else
            {
                compiled->int_consts[i] = next_local;
                compiled->num_int_consts++;
                shader_addline(buffer, "PARAM I%u = program.local[%u];\n", i, next_local++);
            }
        }
    }

    /* We need a constant to fixup the final position */
    shader_addline(buffer, "PARAM posFixup = program.local[%u];\n", next_local);
    compiled->pos_fixup = next_local++;

    /* Initialize output parameters. GL_ARB_vertex_program does not require special initialization values
     * for output parameters. D3D in theory does not do that either, but some applications depend on a
     * proper initialization of the secondary color, and programs using the fixed function pipeline without
     * a replacement shader depend on the texcoord.w being set properly.
     *
     * GL_NV_vertex_program defines that all output values are initialized to {0.0, 0.0, 0.0, 1.0}. This
     * assertion is in effect even when using GL_ARB_vertex_program without any NV specific additions. So
     * skip this if NV_vertex_program is supported. Otherwise, initialize the secondary color. For the tex-
     * coords, we have a flag in the opengl caps. Many cards do not require the texcoord being set, and
     * this can eat a number of instructions, so skip it unless this cap is set as well
     */
    if (!gl_info->supported[NV_VERTEX_PROGRAM])
    {
        const char *color_init = arb_get_helper_value(WINED3D_SHADER_TYPE_VERTEX, ARB_0001);
        shader_addline(buffer, "MOV result.color.secondary, %s;\n", color_init);

        if (gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W && !priv->ffp_proj_control)
        {
            int i;
            const char *one = arb_get_helper_value(WINED3D_SHADER_TYPE_VERTEX, ARB_ONE);
            for(i = 0; i < MAX_REG_TEXCRD; i++)
            {
                if (reg_maps->u.texcoord_mask[i] && reg_maps->u.texcoord_mask[i] != WINED3DSP_WRITEMASK_ALL)
                    shader_addline(buffer, "MOV result.texcoord[%u].w, %s\n", i, one);
            }
        }
    }

    /* The shader starts with the main function */
    priv_ctx.in_main_func = TRUE;
    /* Base Shader Body */
    if (FAILED(shader_generate_code(shader, buffer, reg_maps, &priv_ctx, NULL, NULL)))
        return -1;

    if (!priv_ctx.footer_written) vshader_add_footer(&priv_ctx,
            shader_data, args, reg_maps, gl_info, buffer);

    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &ret));

    TRACE("Creating a hw vertex shader, prg=%d\n", ret);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ret));

    TRACE("Created hw vertex shader, prg=%d\n", ret);
    if (!shader_arb_compile(gl_info, GL_VERTEX_PROGRAM_ARB, buffer->buffer))
        return -1;

    return ret;
}

/* Context activation is done by the caller. */
static struct arb_ps_compiled_shader *find_arb_pshader(struct wined3d_context_gl *context_gl,
        struct wined3d_shader *shader, const struct arb_ps_compile_args *args)
{
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_device *device = shader->device;
    UINT i;
    DWORD new_size;
    struct arb_ps_compiled_shader *new_array;
    struct wined3d_string_buffer buffer;
    struct arb_pshader_private *shader_data;
    GLuint ret;

    if (!shader->backend_data)
    {
        struct shader_arb_priv *priv = device->shader_priv;

        shader->backend_data = heap_alloc_zero(sizeof(*shader_data));
        shader_data = shader->backend_data;
        shader_data->clamp_consts = shader->reg_maps.shader_version.major == 1;

        if (shader->reg_maps.shader_version.major < 3)
            shader_data->input_signature_idx = ~0U;
        else
            shader_data->input_signature_idx = find_input_signature(priv, &shader->input_signature);

        TRACE("Shader got assigned input signature index %u\n", shader_data->input_signature_idx);

        if (!d3d_info->vs_clipping)
            shader_data->clipplane_emulation = shader_find_free_input_register(&shader->reg_maps,
                    d3d_info->limits.ffp_blend_stages - 1);
        else
            shader_data->clipplane_emulation = ~0U;
    }
    shader_data = shader->backend_data;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for (i = 0; i < shader_data->num_gl_shaders; ++i)
    {
        if (!memcmp(&shader_data->gl_shaders[i].args, args, sizeof(*args)))
            return &shader_data->gl_shaders[i];
    }

    TRACE("No matching GL shader found, compiling a new shader\n");
    if(shader_data->shader_array_size == shader_data->num_gl_shaders) {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, shader_data->gl_shaders,
                                    new_size * sizeof(*shader_data->gl_shaders));
        }
        else
        {
            new_array = heap_alloc_zero(sizeof(*shader_data->gl_shaders));
            new_size = 1;
        }

        if(!new_array) {
            ERR("Out of memory\n");
            return 0;
        }
        shader_data->gl_shaders = new_array;
        shader_data->shader_array_size = new_size;
    }

    shader_data->gl_shaders[shader_data->num_gl_shaders].args = *args;

    if (!string_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    ret = shader_arb_generate_pshader(shader, gl_info, &buffer, args,
            &shader_data->gl_shaders[shader_data->num_gl_shaders]);
    string_buffer_free(&buffer);
    shader_data->gl_shaders[shader_data->num_gl_shaders].prgId = ret;

    return &shader_data->gl_shaders[shader_data->num_gl_shaders++];
}

static inline BOOL vs_args_equal(const struct arb_vs_compile_args *stored, const struct arb_vs_compile_args *new,
                                 const DWORD use_map, BOOL skip_int) {
    if((stored->super.swizzle_map & use_map) != new->super.swizzle_map) return FALSE;
    if(stored->super.clip_enabled != new->super.clip_enabled) return FALSE;
    if(stored->super.fog_src != new->super.fog_src) return FALSE;
    if(stored->clip.boolclip_compare != new->clip.boolclip_compare) return FALSE;
    if(stored->ps_signature != new->ps_signature) return FALSE;
    if(stored->vertex.samplers_compare != new->vertex.samplers_compare) return FALSE;
    if(skip_int) return TRUE;

    return !memcmp(stored->loop_ctrl, new->loop_ctrl, sizeof(stored->loop_ctrl));
}

static struct arb_vs_compiled_shader *find_arb_vshader(struct wined3d_shader *shader,
        const struct wined3d_gl_info *gl_info, DWORD use_map, const struct arb_vs_compile_args *args,
        const struct wined3d_shader_signature *ps_input_sig)
{
    UINT i;
    DWORD new_size;
    struct arb_vs_compiled_shader *new_array;
    struct wined3d_string_buffer buffer;
    struct arb_vshader_private *shader_data;
    GLuint ret;

    if (!shader->backend_data)
    {
        const struct wined3d_shader_reg_maps *reg_maps = &shader->reg_maps;

        shader->backend_data = heap_alloc_zero(sizeof(*shader_data));
        shader_data = shader->backend_data;

        if ((gl_info->quirks & WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT)
                && reg_maps->min_rel_offset <= reg_maps->max_rel_offset)
        {
            if (reg_maps->max_rel_offset - reg_maps->min_rel_offset > 127)
            {
                FIXME("The difference between the minimum and maximum relative offset is > 127.\n");
                FIXME("Which this OpenGL implementation does not support. Try using GLSL.\n");
                FIXME("Min: %u, Max: %u.\n", reg_maps->min_rel_offset, reg_maps->max_rel_offset);
            }
            else if (reg_maps->max_rel_offset - reg_maps->min_rel_offset > 63)
                shader_data->rel_offset = reg_maps->min_rel_offset + 63;
            else if (reg_maps->max_rel_offset > 63)
                shader_data->rel_offset = reg_maps->min_rel_offset;
        }
    }
    shader_data = shader->backend_data;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for(i = 0; i < shader_data->num_gl_shaders; i++) {
        if (vs_args_equal(&shader_data->gl_shaders[i].args, args,
                use_map, gl_info->supported[NV_VERTEX_PROGRAM2_OPTION]))
        {
            return &shader_data->gl_shaders[i];
        }
    }

    TRACE("No matching GL shader found, compiling a new shader\n");

    if(shader_data->shader_array_size == shader_data->num_gl_shaders) {
        if (shader_data->num_gl_shaders)
        {
            new_size = shader_data->shader_array_size + max(1, shader_data->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, shader_data->gl_shaders,
                                    new_size * sizeof(*shader_data->gl_shaders));
        }
        else
        {
            new_array = heap_alloc_zero(sizeof(*shader_data->gl_shaders));
            new_size = 1;
        }

        if(!new_array) {
            ERR("Out of memory\n");
            return 0;
        }
        shader_data->gl_shaders = new_array;
        shader_data->shader_array_size = new_size;
    }

    shader_data->gl_shaders[shader_data->num_gl_shaders].args = *args;

    if (!string_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    ret = shader_arb_generate_vshader(shader, gl_info, &buffer, args,
            &shader_data->gl_shaders[shader_data->num_gl_shaders],
            ps_input_sig);
    string_buffer_free(&buffer);
    shader_data->gl_shaders[shader_data->num_gl_shaders].prgId = ret;

    return &shader_data->gl_shaders[shader_data->num_gl_shaders++];
}

static void find_arb_ps_compile_args(const struct wined3d_state *state,
        const struct wined3d_context_gl *context_gl, const struct wined3d_shader *shader,
        struct arb_ps_compile_args *args)
{
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    int i;
    WORD int_skip;

    find_ps_compile_args(state, shader, context_gl->c.stream_info.position_transformed, &args->super, &context_gl->c);

    /* This forces all local boolean constants to 1 to make them stateblock independent */
    args->bools = shader->reg_maps.local_bool_consts;

    for (i = 0; i < WINED3D_MAX_CONSTS_B; ++i)
    {
        if (state->ps_consts_b[i])
            args->bools |= ( 1u << i);
    }

    /* Only enable the clip plane emulation KIL if at least one clipplane is enabled. The KIL instruction
     * is quite expensive because it forces the driver to disable early Z discards. It is cheaper to
     * duplicate the shader than have a no-op KIL instruction in every shader
     */
    if (!d3d_info->vs_clipping && use_vs(state)
            && state->render_states[WINED3D_RS_CLIPPING]
            && state->render_states[WINED3D_RS_CLIPPLANEENABLE])
        args->clip = 1;
    else
        args->clip = 0;

    /* Skip if unused or local, or supported natively */
    int_skip = ~shader->reg_maps.integer_constants | shader->reg_maps.local_int_consts;
    if (int_skip == 0xffff || gl_info->supported[NV_FRAGMENT_PROGRAM_OPTION])
    {
        memset(args->loop_ctrl, 0, sizeof(args->loop_ctrl));
        return;
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        if (int_skip & (1u << i))
        {
            args->loop_ctrl[i][0] = 0;
            args->loop_ctrl[i][1] = 0;
            args->loop_ctrl[i][2] = 0;
        }
        else
        {
            args->loop_ctrl[i][0] = state->ps_consts_i[i].x;
            args->loop_ctrl[i][1] = state->ps_consts_i[i].y;
            args->loop_ctrl[i][2] = state->ps_consts_i[i].z;
        }
    }
}

static void find_arb_vs_compile_args(const struct wined3d_state *state,
        const struct wined3d_context_gl *context_gl, const struct wined3d_shader *shader,
        struct arb_vs_compile_args *args)
{
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_device *device = shader->device;
    const struct wined3d_adapter *adapter = device->adapter;
    int i;
    WORD int_skip;

    find_vs_compile_args(state, shader, context_gl->c.stream_info.swizzle_map, &args->super, &context_gl->c);

    args->clip.boolclip_compare = 0;
    if (use_ps(state))
    {
        const struct wined3d_shader *ps = state->shader[WINED3D_SHADER_TYPE_PIXEL];
        const struct arb_pshader_private *shader_priv = ps->backend_data;
        args->ps_signature = shader_priv->input_signature_idx;

        args->clip.boolclip.clip_texcoord = shader_priv->clipplane_emulation + 1;
    }
    else
    {
        args->ps_signature = ~0;
        if (!d3d_info->vs_clipping && adapter->fragment_pipe == &arbfp_fragment_pipeline)
            args->clip.boolclip.clip_texcoord = ffp_clip_emul(&context_gl->c) ? d3d_info->limits.ffp_blend_stages : 0;
        /* Otherwise: Setting boolclip_compare set clip_texcoord to 0 */
    }

    if (args->clip.boolclip.clip_texcoord)
    {
        if (state->render_states[WINED3D_RS_CLIPPING])
            args->clip.boolclip.clipplane_mask = (unsigned char)state->render_states[WINED3D_RS_CLIPPLANEENABLE];
        /* clipplane_mask was set to 0 by setting boolclip_compare to 0 */
    }

    /* This forces all local boolean constants to 1 to make them stateblock independent */
    args->clip.boolclip.bools = shader->reg_maps.local_bool_consts;
    /* TODO: Figure out if it would be better to store bool constants as bitmasks in the stateblock */
    for (i = 0; i < WINED3D_MAX_CONSTS_B; ++i)
    {
        if (state->vs_consts_b[i])
            args->clip.boolclip.bools |= (1u << i);
    }

    args->vertex.samplers[0] = context_gl->tex_unit_map[WINED3D_MAX_FRAGMENT_SAMPLERS + 0];
    args->vertex.samplers[1] = context_gl->tex_unit_map[WINED3D_MAX_FRAGMENT_SAMPLERS + 1];
    args->vertex.samplers[2] = context_gl->tex_unit_map[WINED3D_MAX_FRAGMENT_SAMPLERS + 2];
    args->vertex.samplers[3] = 0;

    /* Skip if unused or local */
    int_skip = ~shader->reg_maps.integer_constants | shader->reg_maps.local_int_consts;
    /* This is about flow control, not clipping. */
    if (int_skip == 0xffff || gl_info->supported[NV_VERTEX_PROGRAM2_OPTION])
    {
        memset(args->loop_ctrl, 0, sizeof(args->loop_ctrl));
        return;
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        if (int_skip & (1u << i))
        {
            args->loop_ctrl[i][0] = 0;
            args->loop_ctrl[i][1] = 0;
            args->loop_ctrl[i][2] = 0;
        }
        else
        {
            args->loop_ctrl[i][0] = state->vs_consts_i[i].x;
            args->loop_ctrl[i][1] = state->vs_consts_i[i].y;
            args->loop_ctrl[i][2] = state->vs_consts_i[i].z;
        }
    }
}

/* Context activation is done by the caller. */
static void shader_arb_select(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct shader_arb_priv *priv = shader_priv;
    int i;

    /* Deal with pixel shaders first so the vertex shader arg function has the input signature ready */
    if (use_ps(state))
    {
        struct wined3d_shader *ps = state->shader[WINED3D_SHADER_TYPE_PIXEL];
        struct arb_ps_compile_args compile_args;
        struct arb_ps_compiled_shader *compiled;

        TRACE("Using pixel shader %p.\n", ps);
        find_arb_ps_compile_args(state, context_gl, ps, &compile_args);
        compiled = find_arb_pshader(context_gl, ps, &compile_args);
        priv->current_fprogram_id = compiled->prgId;
        priv->compiled_fprog = compiled;

        /* Bind the fragment program */
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->current_fprogram_id));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->current_fprogram_id);");

        if (!priv->use_arbfp_fixed_func)
            priv->fragment_pipe->fp_enable(context, FALSE);

        /* Enable OpenGL fragment programs. */
        gl_info->gl_ops.gl.p_glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB);");

        TRACE("Bound fragment program %u and enabled GL_FRAGMENT_PROGRAM_ARB\n", priv->current_fprogram_id);

        /* Pixel Shader 1.x constants are clamped to [-1;1], Pixel Shader 2.0 constants are not. If switching between
         * a 1.x and newer shader, reload the first 8 constants
         */
        if (priv->last_ps_const_clamped != ((struct arb_pshader_private *)ps->backend_data)->clamp_consts)
        {
            priv->last_ps_const_clamped = ((struct arb_pshader_private *)ps->backend_data)->clamp_consts;
            priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, 8);
            for(i = 0; i < 8; i++)
            {
                priv->pshader_const_dirty[i] = 1;
            }
            /* Also takes care of loading local constants */
            shader_arb_load_constants_internal(shader_priv, context_gl, state, TRUE, FALSE, TRUE);
        }
        else
        {
            UINT rt_height = state->fb->render_targets[0]->height;
            shader_arb_ps_local_constants(compiled, context_gl, state, rt_height);
        }

        /* Force constant reloading for the NP2 fixup (see comment in shader_glsl_select for more info) */
        if (compiled->np2fixup_info.super.active)
            context->constant_update_mask |= WINED3D_SHADER_CONST_PS_NP2_FIXUP;

        if (ps->load_local_constsF)
            context->constant_update_mask |= WINED3D_SHADER_CONST_PS_F;
    }
    else
    {
        if (gl_info->supported[ARB_FRAGMENT_PROGRAM] && !priv->use_arbfp_fixed_func)
        {
            /* Disable only if we're not using arbfp fixed function fragment
             * processing. If this is used, keep GL_FRAGMENT_PROGRAM_ARB
             * enabled, and the fixed function pipeline will bind the fixed
             * function replacement shader. */
            gl_info->gl_ops.gl.p_glDisable(GL_FRAGMENT_PROGRAM_ARB);
            checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
            priv->current_fprogram_id = 0;
        }
        priv->fragment_pipe->fp_enable(context, TRUE);
    }

    if (use_vs(state))
    {
        struct wined3d_shader *vs = state->shader[WINED3D_SHADER_TYPE_VERTEX];
        struct arb_vs_compile_args compile_args;
        struct arb_vs_compiled_shader *compiled;
        const struct wined3d_shader_signature *ps_input_sig;

        TRACE("Using vertex shader %p\n", vs);
        find_arb_vs_compile_args(state, context_gl, vs, &compile_args);

        /* Instead of searching for the signature in the signature list, read the one from the
         * current pixel shader. It's maybe not the shader where the signature came from, but it
         * is the same signature and faster to find. */
        if (compile_args.ps_signature == ~0U)
            ps_input_sig = NULL;
        else
            ps_input_sig = &state->shader[WINED3D_SHADER_TYPE_PIXEL]->input_signature;

        compiled = find_arb_vshader(vs, gl_info, context->stream_info.use_map,
                &compile_args, ps_input_sig);
        priv->current_vprogram_id = compiled->prgId;
        priv->compiled_vprog = compiled;

        /* Bind the vertex program */
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->current_vprogram_id));
        checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->current_vprogram_id);");

        priv->vertex_pipe->vp_enable(context, FALSE);

        /* Enable OpenGL vertex programs */
        gl_info->gl_ops.gl.p_glEnable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glEnable(GL_VERTEX_PROGRAM_ARB);");
        TRACE("Bound vertex program %u and enabled GL_VERTEX_PROGRAM_ARB\n", priv->current_vprogram_id);
        shader_arb_vs_local_constants(compiled, context_gl, state);

        if(priv->last_vs_color_unclamp != compiled->need_color_unclamp) {
            priv->last_vs_color_unclamp = compiled->need_color_unclamp;

            if (gl_info->supported[ARB_COLOR_BUFFER_FLOAT])
            {
                GL_EXTCALL(glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, !compiled->need_color_unclamp));
                checkGLcall("glClampColorARB");
            } else {
                FIXME("vertex color clamp needs to be changed, but extension not supported.\n");
            }
        }

        if (vs->load_local_constsF)
            context->constant_update_mask |= WINED3D_SHADER_CONST_VS_F;
    }
    else
    {
        if (gl_info->supported[ARB_VERTEX_PROGRAM])
        {
            priv->current_vprogram_id = 0;
            gl_info->gl_ops.gl.p_glDisable(GL_VERTEX_PROGRAM_ARB);
            checkGLcall("glDisable(GL_VERTEX_PROGRAM_ARB)");
        }
        priv->vertex_pipe->vp_enable(context, TRUE);
    }
}

static void shader_arb_select_compute(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    ERR("Compute pipeline not supported by the ARB shader backend.\n");
}

/* Context activation is done by the caller. */
static void shader_arb_disable(void *shader_priv, struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct shader_arb_priv *priv = shader_priv;

    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
        priv->current_fprogram_id = 0;
    }
    priv->fragment_pipe->fp_enable(context, FALSE);

    if (gl_info->supported[ARB_VERTEX_PROGRAM])
    {
        priv->current_vprogram_id = 0;
        gl_info->gl_ops.gl.p_glDisable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glDisable(GL_VERTEX_PROGRAM_ARB)");
    }
    priv->vertex_pipe->vp_enable(context, FALSE);

    if (gl_info->supported[ARB_COLOR_BUFFER_FLOAT] && priv->last_vs_color_unclamp)
    {
        GL_EXTCALL(glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FIXED_ONLY_ARB));
        checkGLcall("glClampColorARB");
        priv->last_vs_color_unclamp = FALSE;
    }

    context->shader_update_mask = (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY)
            | (1u << WINED3D_SHADER_TYPE_HULL)
            | (1u << WINED3D_SHADER_TYPE_DOMAIN)
            | (1u << WINED3D_SHADER_TYPE_COMPUTE);
}

static void shader_arb_destroy(struct wined3d_shader *shader)
{
    struct wined3d_device *device = shader->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    unsigned int i;

    /* This can happen if a shader was never compiled */
    if (!shader->backend_data)
        return;

    context = context_acquire(device, NULL, 0);
    gl_info = wined3d_context_gl(context)->gl_info;

    if (shader_is_pshader_version(shader->reg_maps.shader_version.type))
    {
        struct arb_pshader_private *shader_data = shader->backend_data;

        for (i = 0; i < shader_data->num_gl_shaders; ++i)
            GL_EXTCALL(glDeleteProgramsARB(1, &shader_data->gl_shaders[i].prgId));

        heap_free(shader_data->gl_shaders);
    }
    else
    {
        struct arb_vshader_private *shader_data = shader->backend_data;

        for (i = 0; i < shader_data->num_gl_shaders; ++i)
            GL_EXTCALL(glDeleteProgramsARB(1, &shader_data->gl_shaders[i].prgId));

        heap_free(shader_data->gl_shaders);
    }

    checkGLcall("delete programs");

    context_release(context);

    heap_free(shader->backend_data);
    shader->backend_data = NULL;
}

static int sig_tree_compare(const void *key, const struct wine_rb_entry *entry)
{
    struct ps_signature *e = WINE_RB_ENTRY_VALUE(entry, struct ps_signature, entry);
    return compare_sig(key, &e->sig);
}

static HRESULT shader_arb_alloc(struct wined3d_device *device, const struct wined3d_vertex_pipe_ops *vertex_pipe,
        const struct wined3d_fragment_pipe_ops *fragment_pipe)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    struct fragment_caps fragment_caps;
    void *vertex_priv, *fragment_priv;
    struct shader_arb_priv *priv;

    if (!(priv = heap_alloc_zero(sizeof(*priv))))
        return E_OUTOFMEMORY;

    if (!(vertex_priv = vertex_pipe->vp_alloc(&arb_program_shader_backend, priv)))
    {
        ERR("Failed to initialize vertex pipe.\n");
        heap_free(priv);
        return E_FAIL;
    }

    if (!(fragment_priv = fragment_pipe->alloc_private(&arb_program_shader_backend, priv)))
    {
        ERR("Failed to initialize fragment pipe.\n");
        vertex_pipe->vp_free(device, NULL);
        heap_free(priv);
        return E_FAIL;
    }

    memset(priv->vshader_const_dirty, 1,
           sizeof(*priv->vshader_const_dirty) * d3d_info->limits.vs_uniform_count);
    memset(priv->pshader_const_dirty, 1,
            sizeof(*priv->pshader_const_dirty) * d3d_info->limits.ps_uniform_count);

    wine_rb_init(&priv->signature_tree, sig_tree_compare);

    priv->vertex_pipe = vertex_pipe;
    priv->fragment_pipe = fragment_pipe;
    fragment_pipe->get_caps(device->adapter, &fragment_caps);
    priv->ffp_proj_control = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_PROJ_CONTROL;

    device->vertex_priv = vertex_priv;
    device->fragment_priv = fragment_priv;
    device->shader_priv = priv;

    return WINED3D_OK;
}

static void release_signature(struct wine_rb_entry *entry, void *context)
{
    struct ps_signature *sig = WINE_RB_ENTRY_VALUE(entry, struct ps_signature, entry);
    unsigned int i;

    for (i = 0; i < sig->sig.element_count; ++i)
    {
        heap_free((char *)sig->sig.elements[i].semantic_name);
    }
    heap_free(sig->sig.elements);
    heap_free(sig);
}

/* Context activation is done by the caller. */
static void shader_arb_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct shader_arb_priv *priv = device->shader_priv;

    wine_rb_destroy(&priv->signature_tree, release_signature, NULL);
    priv->fragment_pipe->free_private(device, context);
    priv->vertex_pipe->vp_free(device, context);
    heap_free(device->shader_priv);
}

static BOOL shader_arb_allocate_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void shader_arb_free_context_data(struct wined3d_context *context)
{
    struct shader_arb_priv *priv;

    priv = context->device->shader_priv;
    if (priv->last_context == context)
        priv->last_context = NULL;
}

static void shader_arb_init_context_state(struct wined3d_context *context) {}

static void shader_arb_get_caps(const struct wined3d_adapter *adapter, struct shader_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    if (gl_info->supported[ARB_VERTEX_PROGRAM])
    {
        DWORD vs_consts;
        UINT vs_version;

        /* 96 is the minimum allowed value of MAX_PROGRAM_ENV_PARAMETERS_ARB
         * for vertex programs. If the native limit is less than that it's
         * not very useful, and e.g. Mesa swrast returns 0, probably to
         * indicate it's a software implementation. */
        if (gl_info->limits.arb_vs_native_constants < 96)
            vs_consts = gl_info->limits.arb_vs_float_constants;
        else
            vs_consts = min(gl_info->limits.arb_vs_float_constants, gl_info->limits.arb_vs_native_constants);

        if (gl_info->supported[NV_VERTEX_PROGRAM3])
        {
            vs_version = 3;
            TRACE("Hardware vertex shader version 3.0 enabled (NV_VERTEX_PROGRAM3)\n");
        }
        else if (vs_consts >= 256)
        {
            /* Shader Model 2.0 requires at least 256 vertex shader constants */
            vs_version = 2;
            TRACE("Hardware vertex shader version 2.0 enabled (ARB_PROGRAM)\n");
        }
        else
        {
            vs_version = 1;
            TRACE("Hardware vertex shader version 1.1 enabled (ARB_PROGRAM)\n");
        }
        caps->vs_version = min(wined3d_settings.max_sm_vs, vs_version);
        caps->vs_uniform_count = min(WINED3D_MAX_VS_CONSTS_F, vs_consts);
    }
    else
    {
        caps->vs_version = 0;
        caps->vs_uniform_count = 0;
    }

    caps->hs_version = 0;
    caps->ds_version = 0;
    caps->gs_version = 0;
    caps->cs_version = 0;

    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
    {
        DWORD ps_consts;
        UINT ps_version;

        /* Similar as above for vertex programs, but the minimum for fragment
         * programs is 24. */
        if (gl_info->limits.arb_ps_native_constants < 24)
            ps_consts = gl_info->limits.arb_ps_float_constants;
        else
            ps_consts = min(gl_info->limits.arb_ps_float_constants, gl_info->limits.arb_ps_native_constants);

        if (gl_info->supported[NV_FRAGMENT_PROGRAM2])
        {
            ps_version = 3;
            TRACE("Hardware pixel shader version 3.0 enabled (NV_FRAGMENT_PROGRAM2)\n");
        }
        else if (ps_consts >= 32)
        {
            /* Shader Model 2.0 requires at least 32 pixel shader constants */
            ps_version = 2;
            TRACE("Hardware pixel shader version 2.0 enabled (ARB_PROGRAM)\n");
        }
        else
        {
            ps_version = 1;
            TRACE("Hardware pixel shader version 1.4 enabled (ARB_PROGRAM)\n");
        }
        caps->ps_version = min(wined3d_settings.max_sm_ps, ps_version);
        caps->ps_uniform_count = min(WINED3D_MAX_PS_CONSTS_F, ps_consts);
        caps->ps_1x_max_value = 8.0f;
    }
    else
    {
        caps->ps_version = 0;
        caps->ps_uniform_count = 0;
        caps->ps_1x_max_value = 0.0f;
    }

    caps->varying_count = 0;
    caps->wined3d_caps = WINED3D_SHADER_CAP_SRGB_WRITE;
    if (use_nv_clip(gl_info))
        caps->wined3d_caps |= WINED3D_SHADER_CAP_VS_CLIPPING;
}

static BOOL shader_arb_color_fixup_supported(struct color_fixup_desc fixup)
{
    /* We support everything except complex conversions. */
    return !is_complex_fixup(fixup);
}

static void shader_arb_add_instruction_modifiers(const struct wined3d_shader_instruction *ins) {
    DWORD shift;
    char write_mask[20], regstr[50];
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    BOOL is_color = FALSE;
    const struct wined3d_shader_dst_param *dst;

    if (!ins->dst_count) return;

    dst = &ins->dst[0];
    shift = dst->shift;
    if (!shift) return; /* Saturate alone is handled by the instructions */

    shader_arb_get_write_mask(ins, dst, write_mask);
    shader_arb_get_register_name(ins, &dst->reg, regstr, &is_color);

    /* Generate a line that does the output modifier computation
     * FIXME: _SAT vs shift? _SAT alone is already handled in the instructions, if this
     * maps problems in e.g. _d4_sat modify shader_arb_get_modifier
     */
    shader_addline(buffer, "MUL%s %s%s, %s, %s;\n", shader_arb_get_modifier(ins),
                   regstr, write_mask, regstr, shift_tab[shift]);
}

static const SHADER_HANDLER shader_arb_instruction_handler_table[WINED3DSIH_TABLE_SIZE] =
{
    /* WINED3DSIH_ABS                              */ shader_hw_map2gl,
    /* WINED3DSIH_ADD                              */ shader_hw_map2gl,
    /* WINED3DSIH_AND                              */ NULL,
    /* WINED3DSIH_ATOMIC_AND                       */ NULL,
    /* WINED3DSIH_ATOMIC_CMP_STORE                 */ NULL,
    /* WINED3DSIH_ATOMIC_IADD                      */ NULL,
    /* WINED3DSIH_ATOMIC_IMAX                      */ NULL,
    /* WINED3DSIH_ATOMIC_IMIN                      */ NULL,
    /* WINED3DSIH_ATOMIC_OR                        */ NULL,
    /* WINED3DSIH_ATOMIC_UMAX                      */ NULL,
    /* WINED3DSIH_ATOMIC_UMIN                      */ NULL,
    /* WINED3DSIH_ATOMIC_XOR                       */ NULL,
    /* WINED3DSIH_BEM                              */ pshader_hw_bem,
    /* WINED3DSIH_BFI                              */ NULL,
    /* WINED3DSIH_BFREV                            */ NULL,
    /* WINED3DSIH_BREAK                            */ shader_hw_break,
    /* WINED3DSIH_BREAKC                           */ shader_hw_breakc,
    /* WINED3DSIH_BREAKP                           */ NULL,
    /* WINED3DSIH_BUFINFO                          */ NULL,
    /* WINED3DSIH_CALL                             */ shader_hw_call,
    /* WINED3DSIH_CALLNZ                           */ NULL,
    /* WINED3DSIH_CASE                             */ NULL,
    /* WINED3DSIH_CMP                              */ pshader_hw_cmp,
    /* WINED3DSIH_CND                              */ pshader_hw_cnd,
    /* WINED3DSIH_CONTINUE                         */ NULL,
    /* WINED3DSIH_CONTINUEP                        */ NULL,
    /* WINED3DSIH_COUNTBITS                        */ NULL,
    /* WINED3DSIH_CRS                              */ shader_hw_map2gl,
    /* WINED3DSIH_CUT                              */ NULL,
    /* WINED3DSIH_CUT_STREAM                       */ NULL,
    /* WINED3DSIH_DCL                              */ shader_hw_nop,
    /* WINED3DSIH_DCL_CONSTANT_BUFFER              */ shader_hw_nop,
    /* WINED3DSIH_DCL_FUNCTION_BODY                */ NULL,
    /* WINED3DSIH_DCL_FUNCTION_TABLE               */ NULL,
    /* WINED3DSIH_DCL_GLOBAL_FLAGS                 */ NULL,
    /* WINED3DSIH_DCL_GS_INSTANCES                 */ NULL,
    /* WINED3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT */ NULL,
    /* WINED3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT */ NULL,
    /* WINED3DSIH_DCL_HS_MAX_TESSFACTOR            */ NULL,
    /* WINED3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER    */ NULL,
    /* WINED3DSIH_DCL_INDEX_RANGE                  */ NULL,
    /* WINED3DSIH_DCL_INDEXABLE_TEMP               */ NULL,
    /* WINED3DSIH_DCL_INPUT                        */ NULL,
    /* WINED3DSIH_DCL_INPUT_CONTROL_POINT_COUNT    */ NULL,
    /* WINED3DSIH_DCL_INPUT_PRIMITIVE              */ shader_hw_nop,
    /* WINED3DSIH_DCL_INPUT_PS                     */ NULL,
    /* WINED3DSIH_DCL_INPUT_PS_SGV                 */ NULL,
    /* WINED3DSIH_DCL_INPUT_PS_SIV                 */ NULL,
    /* WINED3DSIH_DCL_INPUT_SGV                    */ NULL,
    /* WINED3DSIH_DCL_INPUT_SIV                    */ NULL,
    /* WINED3DSIH_DCL_INTERFACE                    */ NULL,
    /* WINED3DSIH_DCL_OUTPUT                       */ NULL,
    /* WINED3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT   */ NULL,
    /* WINED3DSIH_DCL_OUTPUT_SIV                   */ NULL,
    /* WINED3DSIH_DCL_OUTPUT_TOPOLOGY              */ shader_hw_nop,
    /* WINED3DSIH_DCL_RESOURCE_RAW                 */ NULL,
    /* WINED3DSIH_DCL_RESOURCE_STRUCTURED          */ NULL,
    /* WINED3DSIH_DCL_SAMPLER                      */ NULL,
    /* WINED3DSIH_DCL_STREAM                       */ NULL,
    /* WINED3DSIH_DCL_TEMPS                        */ NULL,
    /* WINED3DSIH_DCL_TESSELLATOR_DOMAIN           */ NULL,
    /* WINED3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE */ NULL,
    /* WINED3DSIH_DCL_TESSELLATOR_PARTITIONING     */ NULL,
    /* WINED3DSIH_DCL_TGSM_RAW                     */ NULL,
    /* WINED3DSIH_DCL_TGSM_STRUCTURED              */ NULL,
    /* WINED3DSIH_DCL_THREAD_GROUP                 */ NULL,
    /* WINED3DSIH_DCL_UAV_RAW                      */ NULL,
    /* WINED3DSIH_DCL_UAV_STRUCTURED               */ NULL,
    /* WINED3DSIH_DCL_UAV_TYPED                    */ NULL,
    /* WINED3DSIH_DCL_VERTICES_OUT                 */ shader_hw_nop,
    /* WINED3DSIH_DEF                              */ shader_hw_nop,
    /* WINED3DSIH_DEFAULT                          */ NULL,
    /* WINED3DSIH_DEFB                             */ shader_hw_nop,
    /* WINED3DSIH_DEFI                             */ shader_hw_nop,
    /* WINED3DSIH_DIV                              */ NULL,
    /* WINED3DSIH_DP2                              */ NULL,
    /* WINED3DSIH_DP2ADD                           */ pshader_hw_dp2add,
    /* WINED3DSIH_DP3                              */ shader_hw_map2gl,
    /* WINED3DSIH_DP4                              */ shader_hw_map2gl,
    /* WINED3DSIH_DST                              */ shader_hw_map2gl,
    /* WINED3DSIH_DSX                              */ shader_hw_map2gl,
    /* WINED3DSIH_DSX_COARSE                       */ NULL,
    /* WINED3DSIH_DSX_FINE                         */ NULL,
    /* WINED3DSIH_DSY                              */ shader_hw_dsy,
    /* WINED3DSIH_DSY_COARSE                       */ NULL,
    /* WINED3DSIH_DSY_FINE                         */ NULL,
    /* WINED3DSIH_ELSE                             */ shader_hw_else,
    /* WINED3DSIH_EMIT                             */ NULL,
    /* WINED3DSIH_EMIT_STREAM                      */ NULL,
    /* WINED3DSIH_ENDIF                            */ shader_hw_endif,
    /* WINED3DSIH_ENDLOOP                          */ shader_hw_endloop,
    /* WINED3DSIH_ENDREP                           */ shader_hw_endrep,
    /* WINED3DSIH_ENDSWITCH                        */ NULL,
    /* WINED3DSIH_EQ                               */ NULL,
    /* WINED3DSIH_EVAL_SAMPLE_INDEX                */ NULL,
    /* WINED3DSIH_EXP                              */ shader_hw_scalar_op,
    /* WINED3DSIH_EXPP                             */ shader_hw_scalar_op,
    /* WINED3DSIH_F16TOF32                         */ NULL,
    /* WINED3DSIH_F32TOF16                         */ NULL,
    /* WINED3DSIH_FCALL                            */ NULL,
    /* WINED3DSIH_FIRSTBIT_HI                      */ NULL,
    /* WINED3DSIH_FIRSTBIT_LO                      */ NULL,
    /* WINED3DSIH_FIRSTBIT_SHI                     */ NULL,
    /* WINED3DSIH_FRC                              */ shader_hw_map2gl,
    /* WINED3DSIH_FTOI                             */ NULL,
    /* WINED3DSIH_FTOU                             */ NULL,
    /* WINED3DSIH_GATHER4                          */ NULL,
    /* WINED3DSIH_GATHER4_C                        */ NULL,
    /* WINED3DSIH_GATHER4_PO                       */ NULL,
    /* WINED3DSIH_GATHER4_PO_C                     */ NULL,
    /* WINED3DSIH_GE                               */ NULL,
    /* WINED3DSIH_HS_CONTROL_POINT_PHASE           */ NULL,
    /* WINED3DSIH_HS_DECLS                         */ NULL,
    /* WINED3DSIH_HS_FORK_PHASE                    */ NULL,
    /* WINED3DSIH_HS_JOIN_PHASE                    */ NULL,
    /* WINED3DSIH_IADD                             */ NULL,
    /* WINED3DSIH_IBFE                             */ NULL,
    /* WINED3DSIH_IEQ                              */ NULL,
    /* WINED3DSIH_IF                               */ NULL /* Hardcoded into the shader */,
    /* WINED3DSIH_IFC                              */ shader_hw_ifc,
    /* WINED3DSIH_IGE                              */ NULL,
    /* WINED3DSIH_ILT                              */ NULL,
    /* WINED3DSIH_IMAD                             */ NULL,
    /* WINED3DSIH_IMAX                             */ NULL,
    /* WINED3DSIH_IMIN                             */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_ALLOC                 */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_AND                   */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_CMP_EXCH              */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_CONSUME               */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_EXCH                  */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_IADD                  */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_IMAX                  */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_IMIN                  */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_OR                    */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_UMAX                  */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_UMIN                  */ NULL,
    /* WINED3DSIH_IMM_ATOMIC_XOR                   */ NULL,
    /* WINED3DSIH_IMUL                             */ NULL,
    /* WINED3DSIH_INE                              */ NULL,
    /* WINED3DSIH_INEG                             */ NULL,
    /* WINED3DSIH_ISHL                             */ NULL,
    /* WINED3DSIH_ISHR                             */ NULL,
    /* WINED3DSIH_ITOF                             */ NULL,
    /* WINED3DSIH_LABEL                            */ shader_hw_label,
    /* WINED3DSIH_LD                               */ NULL,
    /* WINED3DSIH_LD2DMS                           */ NULL,
    /* WINED3DSIH_LD_RAW                           */ NULL,
    /* WINED3DSIH_LD_STRUCTURED                    */ NULL,
    /* WINED3DSIH_LD_UAV_TYPED                     */ NULL,
    /* WINED3DSIH_LIT                              */ shader_hw_map2gl,
    /* WINED3DSIH_LOD                              */ NULL,
    /* WINED3DSIH_LOG                              */ shader_hw_scalar_op,
    /* WINED3DSIH_LOGP                             */ shader_hw_scalar_op,
    /* WINED3DSIH_LOOP                             */ shader_hw_loop,
    /* WINED3DSIH_LRP                              */ shader_hw_lrp,
    /* WINED3DSIH_LT                               */ NULL,
    /* WINED3DSIH_M3x2                             */ shader_hw_mnxn,
    /* WINED3DSIH_M3x3                             */ shader_hw_mnxn,
    /* WINED3DSIH_M3x4                             */ shader_hw_mnxn,
    /* WINED3DSIH_M4x3                             */ shader_hw_mnxn,
    /* WINED3DSIH_M4x4                             */ shader_hw_mnxn,
    /* WINED3DSIH_MAD                              */ shader_hw_map2gl,
    /* WINED3DSIH_MAX                              */ shader_hw_map2gl,
    /* WINED3DSIH_MIN                              */ shader_hw_map2gl,
    /* WINED3DSIH_MOV                              */ shader_hw_mov,
    /* WINED3DSIH_MOVA                             */ shader_hw_mov,
    /* WINED3DSIH_MOVC                             */ NULL,
    /* WINED3DSIH_MUL                              */ shader_hw_map2gl,
    /* WINED3DSIH_NE                               */ NULL,
    /* WINED3DSIH_NOP                              */ shader_hw_nop,
    /* WINED3DSIH_NOT                              */ NULL,
    /* WINED3DSIH_NRM                              */ shader_hw_nrm,
    /* WINED3DSIH_OR                               */ NULL,
    /* WINED3DSIH_PHASE                            */ shader_hw_nop,
    /* WINED3DSIH_POW                              */ shader_hw_pow,
    /* WINED3DSIH_RCP                              */ shader_hw_scalar_op,
    /* WINED3DSIH_REP                              */ shader_hw_rep,
    /* WINED3DSIH_RESINFO                          */ NULL,
    /* WINED3DSIH_RET                              */ shader_hw_ret,
    /* WINED3DSIH_RETP                             */ NULL,
    /* WINED3DSIH_ROUND_NE                         */ NULL,
    /* WINED3DSIH_ROUND_NI                         */ NULL,
    /* WINED3DSIH_ROUND_PI                         */ NULL,
    /* WINED3DSIH_ROUND_Z                          */ NULL,
    /* WINED3DSIH_RSQ                              */ shader_hw_scalar_op,
    /* WINED3DSIH_SAMPLE                           */ NULL,
    /* WINED3DSIH_SAMPLE_B                         */ NULL,
    /* WINED3DSIH_SAMPLE_C                         */ NULL,
    /* WINED3DSIH_SAMPLE_C_LZ                      */ NULL,
    /* WINED3DSIH_SAMPLE_GRAD                      */ NULL,
    /* WINED3DSIH_SAMPLE_INFO                      */ NULL,
    /* WINED3DSIH_SAMPLE_LOD                       */ NULL,
    /* WINED3DSIH_SAMPLE_POS                       */ NULL,
    /* WINED3DSIH_SETP                             */ NULL,
    /* WINED3DSIH_SGE                              */ shader_hw_map2gl,
    /* WINED3DSIH_SGN                              */ shader_hw_sgn,
    /* WINED3DSIH_SINCOS                           */ shader_hw_sincos,
    /* WINED3DSIH_SLT                              */ shader_hw_map2gl,
    /* WINED3DSIH_SQRT                             */ NULL,
    /* WINED3DSIH_STORE_RAW                        */ NULL,
    /* WINED3DSIH_STORE_STRUCTURED                 */ NULL,
    /* WINED3DSIH_STORE_UAV_TYPED                  */ NULL,
    /* WINED3DSIH_SUB                              */ shader_hw_map2gl,
    /* WINED3DSIH_SWAPC                            */ NULL,
    /* WINED3DSIH_SWITCH                           */ NULL,
    /* WINED3DSIH_SYNC                             */ NULL,
    /* WINED3DSIH_TEX                              */ pshader_hw_tex,
    /* WINED3DSIH_TEXBEM                           */ pshader_hw_texbem,
    /* WINED3DSIH_TEXBEML                          */ pshader_hw_texbem,
    /* WINED3DSIH_TEXCOORD                         */ pshader_hw_texcoord,
    /* WINED3DSIH_TEXDEPTH                         */ pshader_hw_texdepth,
    /* WINED3DSIH_TEXDP3                           */ pshader_hw_texdp3,
    /* WINED3DSIH_TEXDP3TEX                        */ pshader_hw_texdp3tex,
    /* WINED3DSIH_TEXKILL                          */ pshader_hw_texkill,
    /* WINED3DSIH_TEXLDD                           */ shader_hw_texldd,
    /* WINED3DSIH_TEXLDL                           */ shader_hw_texldl,
    /* WINED3DSIH_TEXM3x2DEPTH                     */ pshader_hw_texm3x2depth,
    /* WINED3DSIH_TEXM3x2PAD                       */ pshader_hw_texm3x2pad,
    /* WINED3DSIH_TEXM3x2TEX                       */ pshader_hw_texm3x2tex,
    /* WINED3DSIH_TEXM3x3                          */ pshader_hw_texm3x3,
    /* WINED3DSIH_TEXM3x3DIFF                      */ NULL,
    /* WINED3DSIH_TEXM3x3PAD                       */ pshader_hw_texm3x3pad,
    /* WINED3DSIH_TEXM3x3SPEC                      */ pshader_hw_texm3x3spec,
    /* WINED3DSIH_TEXM3x3TEX                       */ pshader_hw_texm3x3tex,
    /* WINED3DSIH_TEXM3x3VSPEC                     */ pshader_hw_texm3x3vspec,
    /* WINED3DSIH_TEXREG2AR                        */ pshader_hw_texreg2ar,
    /* WINED3DSIH_TEXREG2GB                        */ pshader_hw_texreg2gb,
    /* WINED3DSIH_TEXREG2RGB                       */ pshader_hw_texreg2rgb,
    /* WINED3DSIH_UBFE                             */ NULL,
    /* WINED3DSIH_UDIV                             */ NULL,
    /* WINED3DSIH_UGE                              */ NULL,
    /* WINED3DSIH_ULT                              */ NULL,
    /* WINED3DSIH_UMAX                             */ NULL,
    /* WINED3DSIH_UMIN                             */ NULL,
    /* WINED3DSIH_UMUL                             */ NULL,
    /* WINED3DSIH_USHR                             */ NULL,
    /* WINED3DSIH_UTOF                             */ NULL,
    /* WINED3DSIH_XOR                              */ NULL,
};

static BOOL get_bool_const(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader *shader, DWORD idx)
{
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    BOOL vshader = shader_is_vshader_version(reg_maps->shader_version.type);
    const struct wined3d_shader_lconst *constant;
    WORD bools = 0;
    WORD flag = (1u << idx);
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;

    if (reg_maps->local_bool_consts & flag)
    {
        /* What good is an if(bool) with a hardcoded local constant? I don't know, but handle it */
        LIST_FOR_EACH_ENTRY(constant, &shader->constantsB, struct wined3d_shader_lconst, entry)
        {
            if (constant->idx == idx)
            {
                return constant->value[0];
            }
        }
        ERR("Local constant not found\n");
        return FALSE;
    }
    else
    {
        if(vshader) bools = priv->cur_vs_args->clip.boolclip.bools;
        else bools = priv->cur_ps_args->bools;
        return bools & flag;
    }
}

static void get_loop_control_const(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader *shader, UINT idx, struct wined3d_shader_loop_control *loop_control)
{
    const struct wined3d_shader_reg_maps *reg_maps = ins->ctx->reg_maps;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;

    /* Integer constants can either be a local constant, or they can be stored in the shader
     * type specific compile args. */
    if (reg_maps->local_int_consts & (1u << idx))
    {
        const struct wined3d_shader_lconst *constant;

        LIST_FOR_EACH_ENTRY(constant, &shader->constantsI, struct wined3d_shader_lconst, entry)
        {
            if (constant->idx == idx)
            {
                loop_control->count = constant->value[0];
                loop_control->start = constant->value[1];
                /* Step is signed. */
                loop_control->step = (int)constant->value[2];
                return;
            }
        }
        /* If this happens the flag was set incorrectly */
        ERR("Local constant not found\n");
        loop_control->count = 0;
        loop_control->start = 0;
        loop_control->step = 0;
        return;
    }

    switch (reg_maps->shader_version.type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            /* Count and aL start value are unsigned */
            loop_control->count = priv->cur_vs_args->loop_ctrl[idx][0];
            loop_control->start = priv->cur_vs_args->loop_ctrl[idx][1];
            /* Step is signed. */
            loop_control->step = ((char)priv->cur_vs_args->loop_ctrl[idx][2]);
            break;

        case WINED3D_SHADER_TYPE_PIXEL:
            loop_control->count = priv->cur_ps_args->loop_ctrl[idx][0];
            loop_control->start = priv->cur_ps_args->loop_ctrl[idx][1];
            loop_control->step = ((char)priv->cur_ps_args->loop_ctrl[idx][2]);
            break;

        default:
            FIXME("Unhandled shader type %#x.\n", reg_maps->shader_version.type);
            break;
    }
}

static void record_instruction(struct list *list, const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_src_param *src_param = NULL, *rel_addr;
    struct wined3d_shader_dst_param *dst_param;
    struct recorded_instruction *rec;
    unsigned int i;

    if (!(rec = heap_alloc_zero(sizeof(*rec))))
    {
        ERR("Out of memory\n");
        return;
    }

    rec->ins = *ins;
    if (!(dst_param = heap_alloc(sizeof(*dst_param))))
        goto free;
    *dst_param = *ins->dst;
    if (ins->dst->reg.idx[0].rel_addr)
    {
        if (!(rel_addr = heap_alloc(sizeof(*rel_addr))))
            goto free;
        *rel_addr = *ins->dst->reg.idx[0].rel_addr;
        dst_param->reg.idx[0].rel_addr = rel_addr;
    }
    rec->ins.dst = dst_param;

    if (!(src_param = heap_calloc(ins->src_count, sizeof(*src_param))))
        goto free;
    for (i = 0; i < ins->src_count; ++i)
    {
        src_param[i] = ins->src[i];
        if (ins->src[i].reg.idx[0].rel_addr)
        {
            if (!(rel_addr = heap_alloc(sizeof(*rel_addr))))
                goto free;
            *rel_addr = *ins->src[i].reg.idx[0].rel_addr;
            src_param[i].reg.idx[0].rel_addr = rel_addr;
        }
    }
    rec->ins.src = src_param;
    list_add_tail(list, &rec->entry);
    return;

free:
    ERR("Out of memory\n");
    if (dst_param)
    {
        heap_free((void *)dst_param->reg.idx[0].rel_addr);
        heap_free(dst_param);
    }
    if (src_param)
    {
        for (i = 0; i < ins->src_count; ++i)
        {
            heap_free((void *)src_param[i].reg.idx[0].rel_addr);
        }
        heap_free(src_param);
    }
    heap_free(rec);
}

static void free_recorded_instruction(struct list *list)
{
    struct recorded_instruction *rec_ins, *entry2;
    unsigned int i;

    LIST_FOR_EACH_ENTRY_SAFE(rec_ins, entry2, list, struct recorded_instruction, entry)
    {
        list_remove(&rec_ins->entry);
        if (rec_ins->ins.dst)
        {
            heap_free((void *)rec_ins->ins.dst->reg.idx[0].rel_addr);
            heap_free((void *)rec_ins->ins.dst);
        }
        if (rec_ins->ins.src)
        {
            for (i = 0; i < rec_ins->ins.src_count; ++i)
            {
                heap_free((void *)rec_ins->ins.src[i].reg.idx[0].rel_addr);
            }
            heap_free((void *)rec_ins->ins.src);
        }
        heap_free(rec_ins);
    }
}

static void pop_control_frame(const struct wined3d_shader_instruction *ins)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    struct control_frame *control_frame;

    if (ins->handler_idx == WINED3DSIH_ENDLOOP || ins->handler_idx == WINED3DSIH_ENDREP)
    {
        struct list *e = list_head(&priv->control_frames);
        control_frame = LIST_ENTRY(e, struct control_frame, entry);
        list_remove(&control_frame->entry);
        heap_free(control_frame);
        priv->loop_depth--;
    }
    else if (ins->handler_idx == WINED3DSIH_ENDIF)
    {
        /* Non-ifc ENDIFs were already handled previously. */
        struct list *e = list_head(&priv->control_frames);
        control_frame = LIST_ENTRY(e, struct control_frame, entry);
        list_remove(&control_frame->entry);
        heap_free(control_frame);
    }
}

static void shader_arb_handle_instruction(const struct wined3d_shader_instruction *ins) {
    SHADER_HANDLER hw_fct;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    const struct wined3d_shader *shader = ins->ctx->shader;
    struct control_frame *control_frame;
    struct wined3d_string_buffer *buffer = ins->ctx->buffer;
    BOOL bool_const;

    if(ins->handler_idx == WINED3DSIH_LOOP || ins->handler_idx == WINED3DSIH_REP)
    {
        control_frame = heap_alloc_zero(sizeof(*control_frame));
        list_add_head(&priv->control_frames, &control_frame->entry);

        if(ins->handler_idx == WINED3DSIH_LOOP) control_frame->type = LOOP;
        if(ins->handler_idx == WINED3DSIH_REP) control_frame->type = REP;

        if(priv->target_version >= NV2)
        {
            control_frame->no.loop = priv->num_loops++;
            priv->loop_depth++;
        }
        else
        {
            /* Don't bother recording when we're in a not used if branch */
            if(priv->muted)
            {
                return;
            }

            if(!priv->recording)
            {
                list_init(&priv->record);
                priv->recording = TRUE;
                control_frame->outer_loop = TRUE;
                get_loop_control_const(ins, shader, ins->src[0].reg.idx[0].offset, &control_frame->loop_control);
                return; /* Instruction is handled */
            }
            /* Record this loop in the outer loop's recording */
        }
    }
    else if(ins->handler_idx == WINED3DSIH_ENDLOOP || ins->handler_idx == WINED3DSIH_ENDREP)
    {
        if(priv->target_version >= NV2)
        {
            /* Nothing to do. The control frame is popped after the HW instr handler */
        }
        else
        {
            struct list *e = list_head(&priv->control_frames);
            control_frame = LIST_ENTRY(e, struct control_frame, entry);
            list_remove(&control_frame->entry);

            if(control_frame->outer_loop)
            {
                unsigned int iteration;
                int aL = 0;
                struct list copy;

                /* Turn off recording before playback */
                priv->recording = FALSE;

                /* Move the recorded instructions to a separate list and get them out of the private data
                 * structure. If there are nested loops, the shader_arb_handle_instruction below will
                 * be recorded again, thus priv->record might be overwritten
                 */
                list_init(&copy);
                list_move_tail(&copy, &priv->record);
                list_init(&priv->record);

                if(ins->handler_idx == WINED3DSIH_ENDLOOP)
                {
                    shader_addline(buffer, "#unrolling loop: %u iterations, aL=%u, inc %d\n",
                                   control_frame->loop_control.count, control_frame->loop_control.start,
                                   control_frame->loop_control.step);
                    aL = control_frame->loop_control.start;
                }
                else
                {
                    shader_addline(buffer, "#unrolling rep: %u iterations\n", control_frame->loop_control.count);
                }

                for (iteration = 0; iteration < control_frame->loop_control.count; ++iteration)
                {
                    struct recorded_instruction *rec_ins;
                    if(ins->handler_idx == WINED3DSIH_ENDLOOP)
                    {
                        priv->aL = aL;
                        shader_addline(buffer, "#Iteration %u, aL=%d\n", iteration, aL);
                    }
                    else
                    {
                        shader_addline(buffer, "#Iteration %u\n", iteration);
                    }

                    LIST_FOR_EACH_ENTRY(rec_ins, &copy, struct recorded_instruction, entry)
                    {
                        shader_arb_handle_instruction(&rec_ins->ins);
                    }

                    if(ins->handler_idx == WINED3DSIH_ENDLOOP)
                    {
                        aL += control_frame->loop_control.step;
                    }
                }
                shader_addline(buffer, "#end loop/rep\n");

                free_recorded_instruction(&copy);
                heap_free(control_frame);
                return; /* Instruction is handled */
            }
            else
            {
                /* This is a nested loop. Proceed to the normal recording function */
                heap_free(control_frame);
            }
        }
    }

    if(priv->recording)
    {
        record_instruction(&priv->record, ins);
        return;
    }

    /* boolean if */
    if(ins->handler_idx == WINED3DSIH_IF)
    {
        control_frame = heap_alloc_zero(sizeof(*control_frame));
        list_add_head(&priv->control_frames, &control_frame->entry);
        control_frame->type = IF;

        bool_const = get_bool_const(ins, shader, ins->src[0].reg.idx[0].offset);
        if (ins->src[0].modifiers == WINED3DSPSM_NOT)
            bool_const = !bool_const;
        if (!priv->muted && !bool_const)
        {
            shader_addline(buffer, "#if(FALSE){\n");
            priv->muted = TRUE;
            control_frame->muting = TRUE;
        }
        else shader_addline(buffer, "#if(TRUE) {\n");

        return; /* Instruction is handled */
    }
    else if(ins->handler_idx == WINED3DSIH_IFC)
    {
        /* IF(bool) and if_cond(a, b) use the same ELSE and ENDIF tokens */
        control_frame = heap_alloc_zero(sizeof(*control_frame));
        control_frame->type = IFC;
        control_frame->no.ifc = priv->num_ifcs++;
        list_add_head(&priv->control_frames, &control_frame->entry);
    }
    else if(ins->handler_idx == WINED3DSIH_ELSE)
    {
        struct list *e = list_head(&priv->control_frames);
        control_frame = LIST_ENTRY(e, struct control_frame, entry);

        if(control_frame->type == IF)
        {
            shader_addline(buffer, "#} else {\n");
            if(!priv->muted && !control_frame->muting)
            {
                priv->muted = TRUE;
                control_frame->muting = TRUE;
            }
            else if(control_frame->muting) priv->muted = FALSE;
            return; /* Instruction is handled. */
        }
        /* In case of an ifc, generate a HW shader instruction */
        if (control_frame->type != IFC)
            ERR("Control frame does not match.\n");
    }
    else if(ins->handler_idx == WINED3DSIH_ENDIF)
    {
        struct list *e = list_head(&priv->control_frames);
        control_frame = LIST_ENTRY(e, struct control_frame, entry);

        if(control_frame->type == IF)
        {
            shader_addline(buffer, "#} endif\n");
            if(control_frame->muting) priv->muted = FALSE;
            list_remove(&control_frame->entry);
            heap_free(control_frame);
            return; /* Instruction is handled */
        }
        /* In case of an ifc, generate a HW shader instruction */
        if (control_frame->type != IFC)
            ERR("Control frame does not match.\n");
    }

    if(priv->muted)
    {
        pop_control_frame(ins);
        return;
    }

    /* Select handler */
    hw_fct = shader_arb_instruction_handler_table[ins->handler_idx];

    /* Unhandled opcode */
    if (!hw_fct)
    {
        FIXME("Backend can't handle opcode %s.\n", debug_d3dshaderinstructionhandler(ins->handler_idx));
        return;
    }
    hw_fct(ins);

    pop_control_frame(ins);

    shader_arb_add_instruction_modifiers(ins);
}

static BOOL shader_arb_has_ffp_proj_control(void *shader_priv)
{
    struct shader_arb_priv *priv = shader_priv;

    return priv->ffp_proj_control;
}

static void shader_arb_precompile(void *shader_priv, struct wined3d_shader *shader) {}

const struct wined3d_shader_backend_ops arb_program_shader_backend =
{
    shader_arb_handle_instruction,
    shader_arb_precompile,
    shader_arb_select,
    shader_arb_select_compute,
    shader_arb_disable,
    shader_arb_update_float_vertex_constants,
    shader_arb_update_float_pixel_constants,
    shader_arb_load_constants,
    shader_arb_destroy,
    shader_arb_alloc,
    shader_arb_free,
    shader_arb_allocate_context_data,
    shader_arb_free_context_data,
    shader_arb_init_context_state,
    shader_arb_get_caps,
    shader_arb_color_fixup_supported,
    shader_arb_has_ffp_proj_control,
};

/* ARB_fragment_program fixed function pipeline replacement definitions */
#define ARB_FFP_CONST_TFACTOR           0
#define ARB_FFP_CONST_COLOR_KEY_LOW     ((ARB_FFP_CONST_TFACTOR) + 1)
#define ARB_FFP_CONST_COLOR_KEY_HIGH    ((ARB_FFP_CONST_COLOR_KEY_LOW) + 1)
#define ARB_FFP_CONST_SPECULAR_ENABLE   ((ARB_FFP_CONST_COLOR_KEY_HIGH) + 1)
#define ARB_FFP_CONST_CONSTANT(i)       ((ARB_FFP_CONST_SPECULAR_ENABLE) + 1 + i)
#define ARB_FFP_CONST_BUMPMAT(i)        ((ARB_FFP_CONST_CONSTANT(7)) + 1 + i)
#define ARB_FFP_CONST_LUMINANCE(i)      ((ARB_FFP_CONST_BUMPMAT(7)) + 1 + i)

struct arbfp_ffp_desc
{
    struct ffp_frag_desc parent;
    GLuint shader;
};

/* Context activation is done by the caller. */
static void arbfp_enable(const struct wined3d_context *context, BOOL enable)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl_const(context)->gl_info;

    if (enable)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    }
}

static void *arbfp_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    struct shader_arb_priv *priv;

    /* Share private data between the shader backend and the pipeline
     * replacement, if both are the arb implementation. This is needed to
     * figure out whether ARBfp should be disabled if no pixel shader is bound
     * or not. */
    if (shader_backend == &arb_program_shader_backend)
        priv = shader_priv;
    else if (!(priv = heap_alloc_zero(sizeof(*priv))))
        return NULL;

    wine_rb_init(&priv->fragment_shaders, wined3d_ffp_frag_program_key_compare);
    priv->use_arbfp_fixed_func = TRUE;

    return priv;
}

/* Context activation is done by the caller. */
static void arbfp_free_ffpshader(struct wine_rb_entry *entry, void *param)
{
    struct arbfp_ffp_desc *entry_arb = WINE_RB_ENTRY_VALUE(entry, struct arbfp_ffp_desc, parent.entry);
    struct wined3d_context_gl *context_gl = param;
    const struct wined3d_gl_info *gl_info;

    gl_info = context_gl->gl_info;
    GL_EXTCALL(glDeleteProgramsARB(1, &entry_arb->shader));
    checkGLcall("delete ffp program");
    heap_free(entry_arb);
}

/* Context activation is done by the caller. */
static void arbfp_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct shader_arb_priv *priv = device->fragment_priv;

    wine_rb_destroy(&priv->fragment_shaders, arbfp_free_ffpshader, context_gl);
    priv->use_arbfp_fixed_func = FALSE;

    if (device->shader_backend != &arb_program_shader_backend)
        heap_free(device->fragment_priv);
}

static void arbfp_get_caps(const struct wined3d_adapter *adapter, struct fragment_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    caps->wined3d_caps = WINED3D_FRAGMENT_CAP_PROJ_CONTROL
            | WINED3D_FRAGMENT_CAP_SRGB_WRITE
            | WINED3D_FRAGMENT_CAP_COLOR_KEY;
    caps->PrimitiveMiscCaps = WINED3DPMISCCAPS_TSSARGTEMP;
    caps->TextureOpCaps =  WINED3DTEXOPCAPS_DISABLE                     |
                           WINED3DTEXOPCAPS_SELECTARG1                  |
                           WINED3DTEXOPCAPS_SELECTARG2                  |
                           WINED3DTEXOPCAPS_MODULATE4X                  |
                           WINED3DTEXOPCAPS_MODULATE2X                  |
                           WINED3DTEXOPCAPS_MODULATE                    |
                           WINED3DTEXOPCAPS_ADDSIGNED2X                 |
                           WINED3DTEXOPCAPS_ADDSIGNED                   |
                           WINED3DTEXOPCAPS_ADD                         |
                           WINED3DTEXOPCAPS_SUBTRACT                    |
                           WINED3DTEXOPCAPS_ADDSMOOTH                   |
                           WINED3DTEXOPCAPS_BLENDCURRENTALPHA           |
                           WINED3DTEXOPCAPS_BLENDFACTORALPHA            |
                           WINED3DTEXOPCAPS_BLENDTEXTUREALPHA           |
                           WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA           |
                           WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM         |
                           WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR      |
                           WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA      |
                           WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA   |
                           WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR   |
                           WINED3DTEXOPCAPS_DOTPRODUCT3                 |
                           WINED3DTEXOPCAPS_MULTIPLYADD                 |
                           WINED3DTEXOPCAPS_LERP                        |
                           WINED3DTEXOPCAPS_BUMPENVMAP                  |
                           WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE;

    /* TODO: Implement WINED3DTEXOPCAPS_PREMODULATE */

    caps->MaxTextureBlendStages   = WINED3D_MAX_TEXTURES;
    caps->MaxSimultaneousTextures = min(gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL], WINED3D_MAX_TEXTURES);
}

static DWORD arbfp_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return GL_EXT_EMUL_ARB_MULTITEXTURE | GL_EXT_EMUL_EXT_FOG_COORD;
}

static void state_texfactor_arbfp(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_device *device = context->device;
    struct wined3d_color color;

    if (device->shader_backend == &arb_program_shader_backend)
    {
        struct shader_arb_priv *priv;

        /* Don't load the parameter if we're using an arbfp pixel shader,
         * otherwise we'll overwrite application provided constants. */
        if (use_ps(state))
            return;

        priv = device->shader_priv;
        priv->pshader_const_dirty[ARB_FFP_CONST_TFACTOR] = 1;
        priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, ARB_FFP_CONST_TFACTOR + 1);
    }

    wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_TEXTUREFACTOR]);
    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_TFACTOR, &color.r));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_TFACTOR, &color.r)");
}

static void state_tss_constant_arbfp(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    DWORD stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_device *device = context->device;
    struct wined3d_color color;

    if (device->shader_backend == &arb_program_shader_backend)
    {
        struct shader_arb_priv *priv;

        /* Don't load the parameter if we're using an arbfp pixel shader, otherwise we'll overwrite
         * application provided constants.
         */
        if (use_ps(state))
            return;

        priv = device->shader_priv;
        priv->pshader_const_dirty[ARB_FFP_CONST_CONSTANT(stage)] = 1;
        priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, ARB_FFP_CONST_CONSTANT(stage) + 1);
    }

    wined3d_color_from_d3dcolor(&color, state->texture_states[stage][WINED3D_TSS_CONSTANT]);
    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_CONSTANT(stage), &color.r));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_CONSTANT(stage), &color.r)");
}

static void state_arb_specularenable(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_device *device = context->device;
    float col[4];

    if (device->shader_backend == &arb_program_shader_backend)
    {
        struct shader_arb_priv *priv;

        /* Don't load the parameter if we're using an arbfp pixel shader, otherwise we'll overwrite
         * application provided constants.
         */
        if (use_ps(state))
            return;

        priv = device->shader_priv;
        priv->pshader_const_dirty[ARB_FFP_CONST_SPECULAR_ENABLE] = 1;
        priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, ARB_FFP_CONST_SPECULAR_ENABLE + 1);
    }

    if (state->render_states[WINED3D_RS_SPECULARENABLE])
    {
        /* The specular color has no alpha */
        col[0] = 1.0f; col[1] = 1.0f;
        col[2] = 1.0f; col[3] = 0.0f;
    } else {
        col[0] = 0.0f; col[1] = 0.0f;
        col[2] = 0.0f; col[3] = 0.0f;
    }
    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_SPECULAR_ENABLE, col));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_SPECULAR_ENABLE, col)");
}

static void set_bumpmat_arbfp(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    DWORD stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_device *device = context->device;
    float mat[2][2];

    context->constant_update_mask |= WINED3D_SHADER_CONST_PS_BUMP_ENV;

    if (device->shader_backend == &arb_program_shader_backend)
    {
        struct shader_arb_priv *priv = device->shader_priv;

        /* Exit now, don't set the bumpmat below, otherwise we may overwrite pixel shader constants. */
        if (use_ps(state))
            return;

        priv->pshader_const_dirty[ARB_FFP_CONST_BUMPMAT(stage)] = 1;
        priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, ARB_FFP_CONST_BUMPMAT(stage) + 1);
    }

    mat[0][0] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT00]);
    mat[0][1] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT01]);
    mat[1][0] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT10]);
    mat[1][1] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_MAT11]);

    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_BUMPMAT(stage), &mat[0][0]));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_BUMPMAT(stage), &mat[0][0])");
}

static void tex_bumpenvlum_arbfp(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    DWORD stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_device *device = context->device;
    float param[4];

    context->constant_update_mask |= WINED3D_SHADER_CONST_PS_BUMP_ENV;

    if (device->shader_backend == &arb_program_shader_backend)
    {
        struct shader_arb_priv *priv = device->shader_priv;

        /* Exit now, don't set the luminance below, otherwise we may overwrite pixel shader constants. */
        if (use_ps(state))
            return;

        priv->pshader_const_dirty[ARB_FFP_CONST_LUMINANCE(stage)] = 1;
        priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, ARB_FFP_CONST_LUMINANCE(stage) + 1);
    }

    param[0] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_LSCALE]);
    param[1] = *((float *)&state->texture_states[stage][WINED3D_TSS_BUMPENV_LOFFSET]);
    param[2] = 0.0f;
    param[3] = 0.0f;

    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_LUMINANCE(stage), param));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_LUMINANCE(stage), param)");
}

static void alpha_test_arbfp(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    int glParm;
    float ref;

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (state->render_states[WINED3D_RS_ALPHATESTENABLE])
    {
        gl_info->gl_ops.gl.p_glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable GL_ALPHA_TEST");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable GL_ALPHA_TEST");
        return;
    }

    ref = wined3d_alpha_ref(state);
    glParm = wined3d_gl_compare_func(state->render_states[WINED3D_RS_ALPHAFUNC]);

    if (glParm)
    {
        gl_info->gl_ops.gl.p_glAlphaFunc(glParm, ref);
        checkGLcall("glAlphaFunc");
    }
}

static void color_key_arbfp(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_texture *texture = state->textures[0];
    struct wined3d_device *device = context->device;
    struct wined3d_color float_key[2];

    if (!texture)
        return;

    if (device->shader_backend == &arb_program_shader_backend)
    {
        struct shader_arb_priv *priv;

        /* Don't load the parameter if we're using an arbfp pixel shader,
         * otherwise we'll overwrite application provided constants. */
        if (use_ps(state))
            return;

        priv = device->shader_priv;
        priv->pshader_const_dirty[ARB_FFP_CONST_COLOR_KEY_LOW] = 1;
        priv->pshader_const_dirty[ARB_FFP_CONST_COLOR_KEY_HIGH] = 1;
        priv->highest_dirty_ps_const = max(priv->highest_dirty_ps_const, ARB_FFP_CONST_COLOR_KEY_HIGH + 1);
    }

    wined3d_format_get_float_color_key(texture->resource.format, &texture->async.src_blt_color_key, float_key);

    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_COLOR_KEY_LOW, &float_key[0].r));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_COLOR_KEY_LOW, &float_key[0].r)");
    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_COLOR_KEY_HIGH, &float_key[1].r));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_COLOR_KEY_HIGH, &float_key[1].r)");
}

static const char *get_argreg(struct wined3d_string_buffer *buffer, DWORD argnum, unsigned int stage, DWORD arg)
{
    const char *ret;

    if(arg == ARG_UNUSED) return "unused"; /* This is the marker for unused registers */

    switch(arg & WINED3DTA_SELECTMASK) {
        case WINED3DTA_DIFFUSE:
            ret = "fragment.color.primary"; break;

        case WINED3DTA_CURRENT:
            ret = "ret";
            break;

        case WINED3DTA_TEXTURE:
            switch(stage) {
                case 0: ret = "tex0"; break;
                case 1: ret = "tex1"; break;
                case 2: ret = "tex2"; break;
                case 3: ret = "tex3"; break;
                case 4: ret = "tex4"; break;
                case 5: ret = "tex5"; break;
                case 6: ret = "tex6"; break;
                case 7: ret = "tex7"; break;
                default: ret = "unknown texture";
            }
            break;

        case WINED3DTA_TFACTOR:
            ret = "tfactor"; break;

        case WINED3DTA_SPECULAR:
            ret = "fragment.color.secondary"; break;

        case WINED3DTA_TEMP:
            ret = "tempreg"; break;

        case WINED3DTA_CONSTANT:
            switch(stage) {
                case 0: ret = "const0"; break;
                case 1: ret = "const1"; break;
                case 2: ret = "const2"; break;
                case 3: ret = "const3"; break;
                case 4: ret = "const4"; break;
                case 5: ret = "const5"; break;
                case 6: ret = "const6"; break;
                case 7: ret = "const7"; break;
                default: ret = "unknown constant";
            }
            break;

        default:
            return "unknown";
    }

    if(arg & WINED3DTA_COMPLEMENT) {
        shader_addline(buffer, "SUB arg%u, const.x, %s;\n", argnum, ret);
        if(argnum == 0) ret = "arg0";
        if(argnum == 1) ret = "arg1";
        if(argnum == 2) ret = "arg2";
    }
    if(arg & WINED3DTA_ALPHAREPLICATE) {
        shader_addline(buffer, "MOV arg%u, %s.w;\n", argnum, ret);
        if(argnum == 0) ret = "arg0";
        if(argnum == 1) ret = "arg1";
        if(argnum == 2) ret = "arg2";
    }
    return ret;
}

static void gen_ffp_instr(struct wined3d_string_buffer *buffer, unsigned int stage, BOOL color,
        BOOL alpha, BOOL tmp_dst, DWORD op, DWORD dw_arg0, DWORD dw_arg1, DWORD dw_arg2)
{
    const char *dstmask, *dstreg, *arg0, *arg1, *arg2;
    unsigned int mul = 1;

    if (color && alpha)
        dstmask = "";
    else if (color)
        dstmask = ".xyz";
    else
        dstmask = ".w";

    dstreg = tmp_dst ? "tempreg" : "ret";

    arg0 = get_argreg(buffer, 0, stage, dw_arg0);
    arg1 = get_argreg(buffer, 1, stage, dw_arg1);
    arg2 = get_argreg(buffer, 2, stage, dw_arg2);

    switch (op)
    {
        case WINED3D_TOP_DISABLE:
            break;

        case WINED3D_TOP_SELECT_ARG2:
            arg1 = arg2;
            /* FALLTHROUGH */
        case WINED3D_TOP_SELECT_ARG1:
            shader_addline(buffer, "MOV %s%s, %s;\n", dstreg, dstmask, arg1);
            break;

        case WINED3D_TOP_MODULATE_4X:
            mul = 2;
            /* FALLTHROUGH */
        case WINED3D_TOP_MODULATE_2X:
            mul *= 2;
            /* FALLTHROUGH */
        case WINED3D_TOP_MODULATE:
            shader_addline(buffer, "MUL %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3D_TOP_ADD_SIGNED_2X:
            mul = 2;
            /* FALLTHROUGH */
        case WINED3D_TOP_ADD_SIGNED:
            shader_addline(buffer, "SUB arg2, %s, const.w;\n", arg2);
            arg2 = "arg2";
            /* FALLTHROUGH */
        case WINED3D_TOP_ADD:
            shader_addline(buffer, "ADD_SAT %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3D_TOP_SUBTRACT:
            shader_addline(buffer, "SUB_SAT %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3D_TOP_ADD_SMOOTH:
            shader_addline(buffer, "SUB arg1, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg1, %s, %s;\n", dstreg, dstmask, arg2, arg1);
            break;

        case WINED3D_TOP_BLEND_CURRENT_ALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_CURRENT);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3D_TOP_BLEND_FACTOR_ALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TFACTOR);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3D_TOP_BLEND_DIFFUSE_ALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_DIFFUSE);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;

        case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "SUB arg0.w, const.x, %s.w;\n", arg0);
            shader_addline(buffer, "MAD_SAT %s%s, %s, arg0.w, %s;\n", dstreg, dstmask, arg2, arg1);
            break;

        /* D3DTOP_PREMODULATE ???? */

        case WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR:
            shader_addline(buffer, "SUB arg0.w, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg0.w, %s, %s;\n", dstreg, dstmask, arg2, arg1);
            break;
        case WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR:
            shader_addline(buffer, "MAD_SAT %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg1, arg2, arg1);
            break;
        case WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA:
            shader_addline(buffer, "SUB arg0, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg0, %s, %s.w;\n", dstreg, dstmask, arg2, arg1);
            break;
        case WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA:
            shader_addline(buffer, "MAD_SAT %s%s, %s, %s, %s.w;\n", dstreg, dstmask, arg1, arg2, arg1);
            break;

        case WINED3D_TOP_DOTPRODUCT3:
            mul = 4;
            shader_addline(buffer, "SUB arg1, %s, const.w;\n", arg1);
            shader_addline(buffer, "SUB arg2, %s, const.w;\n", arg2);
            shader_addline(buffer, "DP3_SAT %s%s, arg1, arg2;\n", dstreg, dstmask);
            break;

        case WINED3D_TOP_MULTIPLY_ADD:
            shader_addline(buffer, "MAD_SAT %s%s, %s, %s, %s;\n", dstreg, dstmask, arg1, arg2, arg0);
            break;

        case WINED3D_TOP_LERP:
            /* The msdn is not quite right here */
            shader_addline(buffer, "LRP %s%s, %s, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;

        case WINED3D_TOP_BUMPENVMAP:
        case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
            /* Those are handled in the first pass of the shader(generation pass 1 and 2) already */
            break;

        default:
            FIXME("Unhandled texture op %08x\n", op);
    }

    if (mul == 2)
        shader_addline(buffer, "MUL_SAT %s%s, %s, const.y;\n", dstreg, dstmask, dstreg);
    else if (mul == 4)
        shader_addline(buffer, "MUL_SAT %s%s, %s, const.z;\n", dstreg, dstmask, dstreg);
}

static const char *arbfp_texture_target(enum wined3d_gl_resource_type type)
{
    switch(type)
    {
        case WINED3D_GL_RES_TYPE_TEX_1D:
            return "1D";
        case WINED3D_GL_RES_TYPE_TEX_2D:
            return "2D";
        case WINED3D_GL_RES_TYPE_TEX_3D:
            return "3D";
        case WINED3D_GL_RES_TYPE_TEX_CUBE:
            return "CUBE";
        case WINED3D_GL_RES_TYPE_TEX_RECT:
            return "RECT";
        default:
            return "unexpected_resource_type";
    }
}

static GLuint gen_arbfp_ffp_shader(const struct ffp_frag_settings *settings, const struct wined3d_gl_info *gl_info)
{
    BYTE tex_read = 0, bump_used = 0, luminance_used = 0, constant_used = 0;
    BOOL tempreg_used = FALSE, tfactor_used = FALSE;
    unsigned int stage, lowest_disabled_stage;
    struct wined3d_string_buffer buffer;
    struct color_fixup_masks masks;
    BOOL custom_linear_fog = FALSE;
    const char *textype, *instr;
    DWORD arg0, arg1, arg2;
    char colorcor_dst[8];
    BOOL op_equal;
    GLuint ret;

    if (!string_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    shader_addline(&buffer, "!!ARBfp1.0\n");

    if (settings->color_key_enabled)
    {
        shader_addline(&buffer, "PARAM color_key_low = program.env[%u];\n", ARB_FFP_CONST_COLOR_KEY_LOW);
        shader_addline(&buffer, "PARAM color_key_high = program.env[%u];\n", ARB_FFP_CONST_COLOR_KEY_HIGH);
        tex_read |= 1;
    }

    /* Find out which textures are read */
    for (stage = 0; stage < WINED3D_MAX_TEXTURES; ++stage)
    {
        if (settings->op[stage].cop == WINED3D_TOP_DISABLE)
            break;

        arg0 = settings->op[stage].carg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].carg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].carg2 & WINED3DTA_SELECTMASK;

        if (arg0 == WINED3DTA_TEXTURE || arg1 == WINED3DTA_TEXTURE || arg2 == WINED3DTA_TEXTURE)
            tex_read |= 1u << stage;
        if (settings->op[stage].tmp_dst)
            tempreg_used = TRUE;
        if (arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP)
            tempreg_used = TRUE;
        if (arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR)
            tfactor_used = TRUE;
        if (arg0 == WINED3DTA_CONSTANT || arg1 == WINED3DTA_CONSTANT || arg2 == WINED3DTA_CONSTANT)
            constant_used |= 1u << stage;

        switch (settings->op[stage].cop)
        {
            case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
                luminance_used |= 1u << stage;
                /* fall through */
            case WINED3D_TOP_BUMPENVMAP:
                bump_used |= 1u << stage;
                /* fall through */
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
                tex_read |= 1u << stage;
                break;

            case WINED3D_TOP_BLEND_FACTOR_ALPHA:
                tfactor_used = TRUE;
                break;

            default:
                break;
        }

        if (settings->op[stage].aop == WINED3D_TOP_DISABLE)
            continue;

        arg0 = settings->op[stage].aarg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].aarg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].aarg2 & WINED3DTA_SELECTMASK;

        if (arg0 == WINED3DTA_TEXTURE || arg1 == WINED3DTA_TEXTURE || arg2 == WINED3DTA_TEXTURE)
            tex_read |= 1u << stage;
        if (arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP)
            tempreg_used = TRUE;
        if (arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR)
            tfactor_used = TRUE;
        if (arg0 == WINED3DTA_CONSTANT || arg1 == WINED3DTA_CONSTANT || arg2 == WINED3DTA_CONSTANT)
            constant_used |= 1u << stage;
    }
    lowest_disabled_stage = stage;

    switch (settings->fog)
    {
        case WINED3D_FFP_PS_FOG_OFF:                                                         break;
        case WINED3D_FFP_PS_FOG_LINEAR:
            if (gl_info->quirks & WINED3D_QUIRK_BROKEN_ARB_FOG)
            {
                custom_linear_fog = TRUE;
                break;
            }
            shader_addline(&buffer, "OPTION ARB_fog_linear;\n");
            break;

        case WINED3D_FFP_PS_FOG_EXP:    shader_addline(&buffer, "OPTION ARB_fog_exp;\n");    break;
        case WINED3D_FFP_PS_FOG_EXP2:   shader_addline(&buffer, "OPTION ARB_fog_exp2;\n");   break;
        default: FIXME("Unexpected fog setting %d\n", settings->fog);
    }

    shader_addline(&buffer, "PARAM const = {1, 2, 4, 0.5};\n");
    shader_addline(&buffer, "TEMP TMP;\n");
    shader_addline(&buffer, "TEMP ret;\n");
    if (tempreg_used || settings->sRGB_write) shader_addline(&buffer, "TEMP tempreg;\n");
    shader_addline(&buffer, "TEMP arg0;\n");
    shader_addline(&buffer, "TEMP arg1;\n");
    shader_addline(&buffer, "TEMP arg2;\n");
    for (stage = 0; stage < WINED3D_MAX_TEXTURES; ++stage)
    {
        if (constant_used & (1u << stage))
            shader_addline(&buffer, "PARAM const%u = program.env[%u];\n", stage, ARB_FFP_CONST_CONSTANT(stage));

        if (!(tex_read & (1u << stage)))
            continue;

        shader_addline(&buffer, "TEMP tex%u;\n", stage);

        if (!(bump_used & (1u << stage)))
            continue;
        shader_addline(&buffer, "PARAM bumpmat%u = program.env[%u];\n", stage, ARB_FFP_CONST_BUMPMAT(stage));

        if (!(luminance_used & (1u << stage)))
            continue;
        shader_addline(&buffer, "PARAM luminance%u = program.env[%u];\n", stage, ARB_FFP_CONST_LUMINANCE(stage));
    }
    if (tfactor_used)
        shader_addline(&buffer, "PARAM tfactor = program.env[%u];\n", ARB_FFP_CONST_TFACTOR);
    shader_addline(&buffer, "PARAM specular_enable = program.env[%u];\n", ARB_FFP_CONST_SPECULAR_ENABLE);

    if (settings->sRGB_write)
    {
        shader_addline(&buffer, "PARAM srgb_consts0 = ");
        shader_arb_append_imm_vec4(&buffer, &wined3d_srgb_const[0].x);
        shader_addline(&buffer, ";\n");
        shader_addline(&buffer, "PARAM srgb_consts1 = ");
        shader_arb_append_imm_vec4(&buffer, &wined3d_srgb_const[1].x);
        shader_addline(&buffer, ";\n");
    }

    if (lowest_disabled_stage < 7 && settings->emul_clipplanes)
        shader_addline(&buffer, "KIL fragment.texcoord[7];\n");

    if (tempreg_used || settings->sRGB_write)
        shader_addline(&buffer, "MOV tempreg, 0.0;\n");

    /* Generate texture sampling instructions */
    for (stage = 0; stage < WINED3D_MAX_TEXTURES && settings->op[stage].cop != WINED3D_TOP_DISABLE; ++stage)
    {
        if (!(tex_read & (1u << stage)))
            continue;

        textype = arbfp_texture_target(settings->op[stage].tex_type);

        if (settings->op[stage].projected == WINED3D_PROJECTION_NONE)
        {
            instr = "TEX";
        }
        else if (settings->op[stage].projected == WINED3D_PROJECTION_COUNT4
                || settings->op[stage].projected == WINED3D_PROJECTION_COUNT3)
        {
            instr = "TXP";
        }
        else
        {
            FIXME("Unexpected projection mode %d\n", settings->op[stage].projected);
            instr = "TXP";
        }

        if (stage > 0
                && (settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP
                || settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE))
        {
            shader_addline(&buffer, "SWZ arg1, bumpmat%u, x, z, 0, 0;\n", stage - 1);
            shader_addline(&buffer, "DP3 ret.x, arg1, tex%u;\n", stage - 1);
            shader_addline(&buffer, "SWZ arg1, bumpmat%u, y, w, 0, 0;\n", stage - 1);
            shader_addline(&buffer, "DP3 ret.y, arg1, tex%u;\n", stage - 1);

            /* With projective textures, texbem only divides the static
             * texture coordinate, not the displacement, so multiply the
             * displacement with the dividing parameter before passing it to
             * TXP. */
            if (settings->op[stage].projected != WINED3D_PROJECTION_NONE)
            {
                if (settings->op[stage].projected == WINED3D_PROJECTION_COUNT4)
                {
                    shader_addline(&buffer, "MOV ret.w, fragment.texcoord[%u].w;\n", stage);
                    shader_addline(&buffer, "MUL ret.xyz, ret, fragment.texcoord[%u].w, fragment.texcoord[%u];\n",
                            stage, stage);
                }
                else
                {
                    shader_addline(&buffer, "MOV ret.w, fragment.texcoord[%u].z;\n", stage);
                    shader_addline(&buffer, "MAD ret.xyz, ret, fragment.texcoord[%u].z, fragment.texcoord[%u];\n",
                            stage, stage);
                }
            }
            else
            {
                shader_addline(&buffer, "ADD ret, ret, fragment.texcoord[%u];\n", stage);
            }

            shader_addline(&buffer, "%s tex%u, ret, texture[%u], %s;\n",
                    instr, stage, stage, textype);
            if (settings->op[stage - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
            {
                shader_addline(&buffer, "MAD_SAT ret.x, tex%u.z, luminance%u.x, luminance%u.y;\n",
                               stage - 1, stage - 1, stage - 1);
                shader_addline(&buffer, "MUL tex%u, tex%u, ret.x;\n", stage, stage);
            }
        }
        else if (settings->op[stage].projected == WINED3D_PROJECTION_COUNT3)
        {
            shader_addline(&buffer, "MOV ret, fragment.texcoord[%u];\n", stage);
            shader_addline(&buffer, "MOV ret.w, ret.z;\n");
            shader_addline(&buffer, "%s tex%u, ret, texture[%u], %s;\n",
                            instr, stage, stage, textype);
        }
        else
        {
            shader_addline(&buffer, "%s tex%u, fragment.texcoord[%u], texture[%u], %s;\n",
                            instr, stage, stage, stage, textype);
        }

        sprintf(colorcor_dst, "tex%u", stage);
        masks = calc_color_correction(settings->op[stage].color_fixup, WINED3DSP_WRITEMASK_ALL);
        gen_color_correction(&buffer, colorcor_dst, colorcor_dst, "const.x", "const.y",
                settings->op[stage].color_fixup, masks);
    }

    if (settings->color_key_enabled)
    {
        shader_addline(&buffer, "SLT TMP, tex0, color_key_low;\n"); /* below low key */
        shader_addline(&buffer, "SGE ret, tex0, color_key_high;\n"); /* above high key */
        shader_addline(&buffer, "ADD TMP, TMP, ret;\n"); /* or */
        shader_addline(&buffer, "DP4 TMP.b, TMP, TMP;\n"); /* on any channel */
        shader_addline(&buffer, "SGE TMP, -TMP.b, 0.0;\n"); /* logical not */
        shader_addline(&buffer, "KIL -TMP;\n"); /* discard if true */
    }

    shader_addline(&buffer, "MOV ret, fragment.color.primary;\n");

    /* Generate the main shader */
    for (stage = 0; stage < WINED3D_MAX_TEXTURES; ++stage)
    {
        if (settings->op[stage].cop == WINED3D_TOP_DISABLE)
            break;

        if (settings->op[stage].cop == WINED3D_TOP_SELECT_ARG1
                && settings->op[stage].aop == WINED3D_TOP_SELECT_ARG1)
            op_equal = settings->op[stage].carg1 == settings->op[stage].aarg1;
        else if (settings->op[stage].cop == WINED3D_TOP_SELECT_ARG1
                && settings->op[stage].aop == WINED3D_TOP_SELECT_ARG2)
            op_equal = settings->op[stage].carg1 == settings->op[stage].aarg2;
        else if (settings->op[stage].cop == WINED3D_TOP_SELECT_ARG2
                && settings->op[stage].aop == WINED3D_TOP_SELECT_ARG1)
            op_equal = settings->op[stage].carg2 == settings->op[stage].aarg1;
        else if (settings->op[stage].cop == WINED3D_TOP_SELECT_ARG2
                && settings->op[stage].aop == WINED3D_TOP_SELECT_ARG2)
            op_equal = settings->op[stage].carg2 == settings->op[stage].aarg2;
        else
            op_equal = settings->op[stage].aop   == settings->op[stage].cop
                    && settings->op[stage].carg0 == settings->op[stage].aarg0
                    && settings->op[stage].carg1 == settings->op[stage].aarg1
                    && settings->op[stage].carg2 == settings->op[stage].aarg2;

        if (settings->op[stage].aop == WINED3D_TOP_DISABLE)
        {
            gen_ffp_instr(&buffer, stage, TRUE, FALSE, settings->op[stage].tmp_dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
        }
        else if (op_equal)
        {
            gen_ffp_instr(&buffer, stage, TRUE, TRUE, settings->op[stage].tmp_dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
        }
        else if (settings->op[stage].cop != WINED3D_TOP_BUMPENVMAP
                && settings->op[stage].cop != WINED3D_TOP_BUMPENVMAP_LUMINANCE)
        {
            gen_ffp_instr(&buffer, stage, TRUE, FALSE, settings->op[stage].tmp_dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
            gen_ffp_instr(&buffer, stage, FALSE, TRUE, settings->op[stage].tmp_dst,
                          settings->op[stage].aop, settings->op[stage].aarg0,
                          settings->op[stage].aarg1, settings->op[stage].aarg2);
        }
    }

    if (settings->sRGB_write || custom_linear_fog)
    {
        shader_addline(&buffer, "MAD ret, fragment.color.secondary, specular_enable, ret;\n");
        if (settings->sRGB_write)
            arbfp_add_sRGB_correction(&buffer, "ret", "arg0", "arg1", "arg2", "tempreg", FALSE);
        if (custom_linear_fog)
            arbfp_add_linear_fog(&buffer, "ret", "arg0");
        shader_addline(&buffer, "MOV result.color, ret;\n");
    }
    else
    {
        shader_addline(&buffer, "MAD result.color, fragment.color.secondary, specular_enable, ret;\n");
    }

    /* Footer */
    shader_addline(&buffer, "END\n");

    /* Generate the shader */
    GL_EXTCALL(glGenProgramsARB(1, &ret));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ret));
    shader_arb_compile(gl_info, GL_FRAGMENT_PROGRAM_ARB, buffer.buffer);

    string_buffer_free(&buffer);
    return ret;
}

static void fragment_prog_arbfp(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_device *device = context->device;
    struct shader_arb_priv *priv = device->fragment_priv;
    BOOL use_pshader = use_ps(state);
    struct ffp_frag_settings settings;
    const struct arbfp_ffp_desc *desc;
    unsigned int i;

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (isStateDirty(context, STATE_RENDER(WINED3D_RS_FOGENABLE)))
    {
        if (!use_pshader && device->shader_backend == &arb_program_shader_backend && context->last_was_pshader)
        {
            /* Reload fixed function constants since they collide with the
             * pixel shader constants. */
            for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
            {
                set_bumpmat_arbfp(context, state, STATE_TEXTURESTAGE(i, WINED3D_TSS_BUMPENV_MAT00));
                state_tss_constant_arbfp(context, state, STATE_TEXTURESTAGE(i, WINED3D_TSS_CONSTANT));
            }
            state_texfactor_arbfp(context, state, STATE_RENDER(WINED3D_RS_TEXTUREFACTOR));
            state_arb_specularenable(context, state, STATE_RENDER(WINED3D_RS_SPECULARENABLE));
            color_key_arbfp(context, state, STATE_COLOR_KEY);
        }
        else if (use_pshader)
        {
            context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
        }
        return;
    }

    if (!use_pshader)
    {
        /* Find or create a shader implementing the fixed function pipeline
         * settings, then activate it. */
        gen_ffp_frag_op(context, state, &settings, FALSE);
        desc = (const struct arbfp_ffp_desc *)find_ffp_frag_shader(&priv->fragment_shaders, &settings);
        if (!desc)
        {
            struct arbfp_ffp_desc *new_desc;

            if (!(new_desc = heap_alloc(sizeof(*new_desc))))
            {
                ERR("Out of memory\n");
                return;
            }

            new_desc->parent.settings = settings;
            new_desc->shader = gen_arbfp_ffp_shader(&settings, gl_info);
            add_ffp_frag_shader(&priv->fragment_shaders, &new_desc->parent);
            TRACE("Allocated fixed function replacement shader descriptor %p\n", new_desc);
            desc = new_desc;
        }

        /* Now activate the replacement program. GL_FRAGMENT_PROGRAM_ARB is already active (however, note the
         * comment above the shader_select call below). If e.g. GLSL is active, the shader_select call will
         * deactivate it.
         */
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, desc->shader));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, desc->shader)");
        priv->current_fprogram_id = desc->shader;

        if (device->shader_backend == &arb_program_shader_backend && context->last_was_pshader)
        {
            /* Reload fixed function constants since they collide with the
             * pixel shader constants. */
            for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
            {
                set_bumpmat_arbfp(context, state, STATE_TEXTURESTAGE(i, WINED3D_TSS_BUMPENV_MAT00));
                state_tss_constant_arbfp(context, state, STATE_TEXTURESTAGE(i, WINED3D_TSS_CONSTANT));
            }
            state_texfactor_arbfp(context, state, STATE_RENDER(WINED3D_RS_TEXTUREFACTOR));
            state_arb_specularenable(context, state, STATE_RENDER(WINED3D_RS_SPECULARENABLE));
            color_key_arbfp(context, state, STATE_COLOR_KEY);
        }
        context->last_was_pshader = FALSE;
    }
    else if (!context->last_was_pshader)
    {
        if (device->shader_backend == &arb_program_shader_backend)
            context->constant_update_mask |= WINED3D_SHADER_CONST_PS_F;
        context->last_was_pshader = TRUE;
    }

    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
}

/* We can't link the fog states to the fragment state directly since the
 * vertex pipeline links them to FOGENABLE. A different linking in different
 * pipeline parts can't be expressed in the combined state table, so we need
 * to handle that with a forwarding function. The other invisible side effect
 * is that changing the fog start and fog end (which links to FOGENABLE in
 * vertex) results in the fragment_prog_arbfp function being called because
 * FOGENABLE is dirty, which calls this function here. */
static void state_arbfp_fog(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    enum fogsource new_source;
    DWORD fogstart = state->render_states[WINED3D_RS_FOGSTART];
    DWORD fogend = state->render_states[WINED3D_RS_FOGEND];

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (!isStateDirty(context, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL)))
        fragment_prog_arbfp(context, state, state_id);

    if (!state->render_states[WINED3D_RS_FOGENABLE])
        return;

    if (state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE)
    {
        if (use_vs(state))
        {
            new_source = FOGSOURCE_VS;
        }
        else
        {
            if (state->render_states[WINED3D_RS_FOGVERTEXMODE] == WINED3D_FOG_NONE || context->last_was_rhw)
                new_source = FOGSOURCE_COORD;
            else
                new_source = FOGSOURCE_FFP;
        }
    }
    else
    {
        new_source = FOGSOURCE_FFP;
    }

    if (new_source != context->fog_source || fogstart == fogend)
    {
        context->fog_source = new_source;
        state_fogstartend(context, state, STATE_RENDER(WINED3D_RS_FOGSTART));
    }
}

static void textransform(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (!isStateDirty(context, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL)))
        fragment_prog_arbfp(context, state, state_id);
}

static const struct wined3d_state_entry_template arbfp_fragmentstate_template[] =
{
    {STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),              { STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),             state_texfactor_arbfp   }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),         { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG1),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG2),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG0),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_RESULT_ARG),       { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT01),    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT10),    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT11),    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),  tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LOFFSET),  { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),             { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_ALPHAFUNC),                  { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_ALPHAREF),                   { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),            { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           alpha_test_arbfp        }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_COLORKEYENABLE),             { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_COLOR_KEY,                                     { STATE_COLOR_KEY,                                    color_key_arbfp         }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_FOGENABLE),                  { STATE_RENDER(WINED3D_RS_FOGENABLE),                 state_arbfp_fog         }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_FOGTABLEMODE),               { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),              { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_FOGSTART),                   { STATE_RENDER(WINED3D_RS_FOGSTART),                  state_fogstartend       }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_FOGEND),                     { STATE_RENDER(WINED3D_RS_FOGSTART),                  NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),            { STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),           state_srgbwrite         }, ARB_FRAMEBUFFER_SRGB            },
    {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),            { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                    }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_FOGCOLOR),                   { STATE_RENDER(WINED3D_RS_FOGCOLOR),                  state_fogcolor          }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_FOGDENSITY),                 { STATE_RENDER(WINED3D_RS_FOGDENSITY),                state_fogdensity        }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(0,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(1,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(2,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(3,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(4,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(5,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(6,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(7,WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), textransform}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(0, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(1, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(2, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(3, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(4, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(5, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(6, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3D_TSS_CONSTANT),         { STATE_TEXTURESTAGE(7, WINED3D_TSS_CONSTANT),        state_tss_constant_arbfp}, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),             { STATE_RENDER(WINED3D_RS_SPECULARENABLE),            state_arb_specularenable}, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3D_RS_SHADEMODE),                  { STATE_RENDER(WINED3D_RS_SHADEMODE),                 state_shademode         }, WINED3D_GL_EXT_NONE             },
    {0 /* Terminate */,                                   { 0,                                                  0                       }, WINED3D_GL_EXT_NONE             },
};

static BOOL arbfp_alloc_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void arbfp_free_context_data(struct wined3d_context *context)
{
}

const struct wined3d_fragment_pipe_ops arbfp_fragment_pipeline =
{
    arbfp_enable,
    arbfp_get_caps,
    arbfp_get_emul_mask,
    arbfp_alloc,
    arbfp_free,
    arbfp_alloc_context_data,
    arbfp_free_context_data,
    shader_arb_color_fixup_supported,
    arbfp_fragmentstate_template,
};

struct arbfp_blit_type
{
    enum complex_fixup fixup : 4;
    enum wined3d_gl_resource_type res_type : 3;
    DWORD use_color_key : 1;
    DWORD padding : 24;
};

struct arbfp_blit_desc
{
    GLuint shader;
    struct arbfp_blit_type type;
    struct wine_rb_entry entry;
};

#define ARBFP_BLIT_PARAM_SIZE 0
#define ARBFP_BLIT_PARAM_COLOR_KEY_LOW 1
#define ARBFP_BLIT_PARAM_COLOR_KEY_HIGH 2

struct wined3d_arbfp_blitter
{
    struct wined3d_blitter blitter;
    struct wine_rb_tree shaders;
    GLuint palette_texture;
};

static int arbfp_blit_type_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct arbfp_blit_type *ka = key;
    const struct arbfp_blit_type *kb = &WINE_RB_ENTRY_VALUE(entry, const struct arbfp_blit_desc, entry)->type;

    return memcmp(ka, kb, sizeof(*ka));
}

/* Context activation is done by the caller. */
static void arbfp_free_blit_shader(struct wine_rb_entry *entry, void *ctx)
{
    struct arbfp_blit_desc *entry_arb = WINE_RB_ENTRY_VALUE(entry, struct arbfp_blit_desc, entry);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;

    context_gl = ctx;
    gl_info = context_gl->gl_info;

    GL_EXTCALL(glDeleteProgramsARB(1, &entry_arb->shader));
    checkGLcall("glDeleteProgramsARB(1, &entry_arb->shader)");
    heap_free(entry_arb);
}

/* Context activation is done by the caller. */
static void arbfp_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_arbfp_blitter *arbfp_blitter;
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, &context_gl->c);

    arbfp_blitter = CONTAINING_RECORD(blitter, struct wined3d_arbfp_blitter, blitter);

    wine_rb_destroy(&arbfp_blitter->shaders, arbfp_free_blit_shader, context_gl);
    checkGLcall("Delete blit programs");

    if (arbfp_blitter->palette_texture)
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &arbfp_blitter->palette_texture);

    heap_free(arbfp_blitter);
}

static void gen_packed_yuv_read(struct wined3d_string_buffer *buffer,
        const struct arbfp_blit_type *type, char *luminance)
{
    char chroma;
    const char *tex, *texinstr = "TXP";

    if (type->fixup == COMPLEX_FIXUP_UYVY)
    {
        chroma = 'x';
        *luminance = 'w';
    }
    else
    {
        chroma = 'w';
        *luminance = 'x';
    }

    tex = arbfp_texture_target(type->res_type);
    if (type->res_type == WINED3D_GL_RES_TYPE_TEX_RECT)
        texinstr = "TEX";

    /* First we have to read the chroma values. This means we need at least two pixels(no filtering),
     * or 4 pixels(with filtering). To get the unmodified chromas, we have to rid ourselves of the
     * filtering when we sample the texture.
     *
     * These are the rules for reading the chroma:
     *
     * Even pixel: Cr
     * Even pixel: U
     * Odd pixel: V
     *
     * So we have to get the sampling x position in non-normalized coordinates in integers
     */
    if (type->res_type != WINED3D_GL_RES_TYPE_TEX_RECT)
    {
        shader_addline(buffer, "MUL texcrd.xy, fragment.texcoord[0], size.x;\n");
        shader_addline(buffer, "MOV texcrd.w, size.x;\n");
    }
    else
    {
        shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    }
    /* We must not allow filtering between pixel x and x+1, this would mix U and V
     * Vertical filtering is ok. However, bear in mind that the pixel center is at
     * 0.5, so add 0.5.
     */
    shader_addline(buffer, "FLR texcrd.x, texcrd.x;\n");
    shader_addline(buffer, "ADD texcrd.x, texcrd.x, coef.y;\n");

    /* Multiply the x coordinate by 0.5 and get the fraction. This gives 0.25
     * and 0.75 for the even and odd pixels respectively. */
    shader_addline(buffer, "MUL texcrd2, texcrd, coef.y;\n");
    shader_addline(buffer, "FRC texcrd2, texcrd2;\n");

    /* Sample Pixel 1. */
    shader_addline(buffer, "%s luminance, texcrd, texture[0], %s;\n", texinstr, tex);

    /* Put the value into either of the chroma values */
    shader_addline(buffer, "SGE temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MUL chroma.x, luminance.%c, temp.x;\n", chroma);
    shader_addline(buffer, "SLT temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MUL chroma.y, luminance.%c, temp.x;\n", chroma);

    /* Sample pixel 2. If we read an even pixel(SLT above returned 1), sample
     * the pixel right to the current one. Otherwise, sample the left pixel.
     * Bias and scale the SLT result to -1;1 and add it to the texcrd.x.
     */
    shader_addline(buffer, "MAD temp.x, temp.x, coef.z, -coef.x;\n");
    shader_addline(buffer, "ADD texcrd.x, texcrd, temp.x;\n");
    shader_addline(buffer, "%s luminance, texcrd, texture[0], %s;\n", texinstr, tex);

    /* Put the value into the other chroma */
    shader_addline(buffer, "SGE temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MAD chroma.y, luminance.%c, temp.x, chroma.y;\n", chroma);
    shader_addline(buffer, "SLT temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MAD chroma.x, luminance.%c, temp.x, chroma.x;\n", chroma);

    /* TODO: If filtering is enabled, sample a 2nd pair of pixels left or right of
     * the current one and lerp the two U and V values
     */

    /* This gives the correctly filtered luminance value */
    shader_addline(buffer, "TEX luminance, fragment.texcoord[0], texture[0], %s;\n", tex);
}

static void gen_yv12_read(struct wined3d_string_buffer *buffer,
        const struct arbfp_blit_type *type, char *luminance)
{
    const char *tex;
    static const float yv12_coef[]
            = {2.0f / 3.0f, 1.0f / 6.0f, (2.0f / 3.0f) + (1.0f / 6.0f), 1.0f / 3.0f};

    tex = arbfp_texture_target(type->res_type);

    /* YV12 surfaces contain a WxH sized luminance plane, followed by a (W/2)x(H/2)
     * V and a (W/2)x(H/2) U plane, each with 8 bit per pixel. So the effective
     * bitdepth is 12 bits per pixel. Since the U and V planes have only half the
     * pitch of the luminance plane, the packing into the gl texture is a bit
     * unfortunate. If the whole texture is interpreted as luminance data it looks
     * approximately like this:
     *
     *        +----------------------------------+----
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |   2
     *        |            LUMINANCE             |   -
     *        |                                  |   3
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        +----------------+-----------------+----
     *        |                |                 |
     *        |  V even rows   |  V odd rows     |
     *        |                |                 |   1
     *        +----------------+------------------   -
     *        |                |                 |   3
     *        |  U even rows   |  U odd rows     |
     *        |                |                 |
     *        +----------------+-----------------+----
     *        |                |                 |
     *        |     0.5        |       0.5       |
     *
     * So it appears as if there are 4 chroma images, but in fact the odd rows
     * in the chroma images are in the same row as the even ones. So it is
     * kinda tricky to read
     *
     * When reading from rectangle textures, keep in mind that the input y coordinates
     * go from 0 to d3d_height, whereas the opengl texture height is 1.5 * d3d_height
     */
    shader_addline(buffer, "PARAM yv12_coef = ");
    shader_arb_append_imm_vec4(buffer, yv12_coef);
    shader_addline(buffer, ";\n");

    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    /* the chroma planes have only half the width */
    shader_addline(buffer, "MUL texcrd.x, texcrd.x, coef.y;\n");

    /* The first value is between 2/3 and 5/6th of the texture's height, so scale+bias
     * the coordinate. Also read the right side of the image when reading odd lines
     *
     * Don't forget to clamp the y values in into the range, otherwise we'll get filtering
     * bleeding
     */
    if (type->res_type == WINED3D_GL_RES_TYPE_TEX_2D)
    {
        shader_addline(buffer, "RCP chroma.w, size.y;\n");

        shader_addline(buffer, "MUL texcrd2.y, texcrd.y, size.y;\n");

        shader_addline(buffer, "FLR texcrd2.y, texcrd2.y;\n");
        shader_addline(buffer, "MAD texcrd.y, texcrd.y, yv12_coef.y, yv12_coef.x;\n");

        /* Read odd lines from the right side (add size * 0.5 to the x coordinate). */
        shader_addline(buffer, "ADD texcrd2.x, texcrd2.y, yv12_coef.y;\n"); /* To avoid 0.5 == 0.5 comparisons */
        shader_addline(buffer, "FRC texcrd2.x, texcrd2.x;\n");
        shader_addline(buffer, "SGE texcrd2.x, texcrd2.x, coef.y;\n");
        shader_addline(buffer, "MAD texcrd.x, texcrd2.x, coef.y, texcrd.x;\n");

        /* clamp, keep the half pixel origin in mind */
        shader_addline(buffer, "MAD temp.y, coef.y, chroma.w, yv12_coef.x;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.w, yv12_coef.z;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
    }
    else
    {
        /* The y coordinate for V is in the range [size, size + size / 4). */
        shader_addline(buffer, "FLR texcrd.y, texcrd.y;\n");
        shader_addline(buffer, "MAD texcrd.y, texcrd.y, coef.w, size.y;\n");

        /* Read odd lines from the right side (add size * 0.5 to the x coordinate). */
        shader_addline(buffer, "ADD texcrd2.x, texcrd.y, yv12_coef.y;\n"); /* To avoid 0.5 == 0.5 comparisons */
        shader_addline(buffer, "FRC texcrd2.x, texcrd2.x;\n");
        shader_addline(buffer, "SGE texcrd2.x, texcrd2.x, coef.y;\n");
        shader_addline(buffer, "MUL texcrd2.x, texcrd2.x, size.x;\n");
        shader_addline(buffer, "MAD texcrd.x, texcrd2.x, coef.y, texcrd.x;\n");

        /* Make sure to read exactly from the pixel center */
        shader_addline(buffer, "FLR texcrd.y, texcrd.y;\n");
        shader_addline(buffer, "ADD texcrd.y, texcrd.y, coef.y;\n");

        /* Clamp */
        shader_addline(buffer, "MAD temp.y, size.y, coef.w, size.y;\n");
        shader_addline(buffer, "ADD temp.y, temp.y, -coef.y;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "ADD temp.y, size.y, coef.y;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
    }
    /* Read the texture, put the result into the output register */
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.x, temp.w;\n");

    /* The other chroma value is 1/6th of the texture lower, from 5/6th to 6/6th
     * No need to clamp because we're just reusing the already clamped value from above
     */
    if (type->res_type == WINED3D_GL_RES_TYPE_TEX_2D)
        shader_addline(buffer, "ADD texcrd.y, texcrd.y, yv12_coef.y;\n");
    else
        shader_addline(buffer, "MAD texcrd.y, size.y, coef.w, texcrd.y;\n");
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.y, temp.w;\n");

    /* Sample the luminance value. It is in the top 2/3rd of the texture, so scale the y coordinate.
     * Clamp the y coordinate to prevent the chroma values from bleeding into the sampled luminance
     * values due to filtering
     */
    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    if (type->res_type == WINED3D_GL_RES_TYPE_TEX_2D)
    {
        /* Multiply the y coordinate by 2/3 and clamp it */
        shader_addline(buffer, "MUL texcrd.y, texcrd.y, yv12_coef.x;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.w, yv12_coef.x;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    }
    else
    {
        /* Reading from texture_rectangles is pretty straightforward, just use the unmodified
         * texture coordinate. It is still a good idea to clamp it though, since the opengl texture
         * is bigger
         */
        shader_addline(buffer, "ADD temp.x, size.y, -coef.y;\n");
        shader_addline(buffer, "MIN texcrd.y, texcrd.y, size.x;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    }
    *luminance = 'a';
}

static void gen_nv12_read(struct wined3d_string_buffer *buffer,
        const struct arbfp_blit_type *type, char *luminance)
{
    const char *tex;
    static const float nv12_coef[]
            = {2.0f / 3.0f, 1.0f / 3.0f, 1.0f, 1.0f};

    tex = arbfp_texture_target(type->res_type);

    /* NV12 surfaces contain a WxH sized luminance plane, followed by a (W/2)x(H/2)
     * sized plane where each component is an UV pair. So the effective
     * bitdepth is 12 bits per pixel If the whole texture is interpreted as luminance
     * data it looks approximately like this:
     *
     *        +----------------------------------+----
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |   2
     *        |            LUMINANCE             |   -
     *        |                                  |   3
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        +----------------------------------+----
     *        |UVUVUVUVUVUVUVUVUVUVUVUVUVUVUVUVUV|
     *        |UVUVUVUVUVUVUVUVUVUVUVUVUVUVUVUVUV|
     *        |                                  |   1
     *        |                                  |   -
     *        |                                  |   3
     *        |                                  |
     *        |                                  |
     *        +----------------------------------+----
     *
     * When reading from rectangle textures, keep in mind that the input y coordinates
     * go from 0 to d3d_height, whereas the opengl texture height is 1.5 * d3d_height. */

    shader_addline(buffer, "PARAM nv12_coef = ");
    shader_arb_append_imm_vec4(buffer, nv12_coef);
    shader_addline(buffer, ";\n");

    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    /* We only have half the number of chroma pixels. */
    shader_addline(buffer, "MUL texcrd.x, texcrd.x, coef.y;\n");

    if (type->res_type == WINED3D_GL_RES_TYPE_TEX_2D)
    {
        shader_addline(buffer, "RCP chroma.w, size.x;\n");
        shader_addline(buffer, "RCP chroma.z, size.y;\n");

        shader_addline(buffer, "MAD texcrd.y, texcrd.y, nv12_coef.y, nv12_coef.x;\n");

        /* We must not allow filtering horizontally, this would mix U and V.
         * Vertical filtering is ok. However, bear in mind that the pixel center is at
         * 0.5, so add 0.5. */

        /* Convert to non-normalized coordinates so we can find the
         * individual pixel. */
        shader_addline(buffer, "MUL texcrd.x, texcrd.x, size.x;\n");
        shader_addline(buffer, "FLR texcrd.x, texcrd.x;\n");
        /* Multiply by 2 since chroma components are stored in UV pixel pairs,
         * add 0.5 to hit the center of the pixel. */
        shader_addline(buffer, "MAD texcrd.x, texcrd.x, coef.z, coef.y;\n");

        /* Convert back to normalized coordinates. */
        shader_addline(buffer, "MUL texcrd.x, texcrd.x, chroma.w;\n");

        /* Clamp, keep the half pixel origin in mind. */
        shader_addline(buffer, "MAD temp.y, coef.y, chroma.z, nv12_coef.x;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.z, nv12_coef.z;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
    }
    else
    {
        /* The y coordinate for chroma is in the range [size, size + size / 2). */
        shader_addline(buffer, "MAD texcrd.y, texcrd.y, coef.y, size.y;\n");

        shader_addline(buffer, "FLR texcrd.x, texcrd.x;\n");
        /* Multiply by 2 since chroma components are stored in UV pixel pairs,
         * add 0.5 to hit the center of the pixel. */
        shader_addline(buffer, "MAD texcrd.x, texcrd.x, coef.z, coef.y;\n");

        /* Clamp */
        shader_addline(buffer, "MAD temp.y, size.y, coef.y, size.y;\n");
        shader_addline(buffer, "ADD temp.y, temp.y, -coef.y;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "ADD temp.y, size.y, coef.y;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
    }
    /* Read the texture, put the result into the output register. */
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.y, temp.w;\n");

    if (type->res_type == WINED3D_GL_RES_TYPE_TEX_2D)
    {
        /* Add 1/size.x */
        shader_addline(buffer, "ADD texcrd.x, texcrd.x, chroma.w;\n");
    }
    else
    {
        /* Add 1 */
        shader_addline(buffer, "ADD texcrd.x, texcrd.x, coef.x;\n");
    }
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.x, temp.w;\n");

    /* Sample the luminance value. It is in the top 2/3rd of the texture, so scale the y coordinate.
     * Clamp the y coordinate to prevent the chroma values from bleeding into the sampled luminance
     * values due to filtering. */
    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    if (type->res_type == WINED3D_GL_RES_TYPE_TEX_2D)
    {
        /* Multiply the y coordinate by 2/3 and clamp it */
        shader_addline(buffer, "MUL texcrd.y, texcrd.y, nv12_coef.x;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.w, nv12_coef.x;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    }
    else
    {
        /* Reading from texture_rectangles is pretty straightforward, just use the unmodified
         * texture coordinate. It is still a good idea to clamp it though, since the opengl texture
         * is bigger
         */
        shader_addline(buffer, "ADD temp.x, size.y, -coef.y;\n");
        shader_addline(buffer, "MIN texcrd.y, texcrd.y, size.x;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    }
    *luminance = 'a';
}

/* Context activation is done by the caller. */
static GLuint gen_p8_shader(const struct wined3d_gl_info *gl_info, const struct arbfp_blit_type *type)
{
    GLuint shader;
    struct wined3d_string_buffer buffer;
    const char *tex_target = arbfp_texture_target(type->res_type);

    /* This should not happen because we only use this conversion for
     * present blits which don't use color keying. */
    if (type->use_color_key)
        FIXME("Implement P8 color keying.\n");

    /* Shader header */
    if (!string_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    GL_EXTCALL(glGenProgramsARB(1, &shader));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    if (!shader)
    {
        string_buffer_free(&buffer);
        return 0;
    }

    shader_addline(&buffer, "!!ARBfp1.0\n");
    shader_addline(&buffer, "TEMP index;\n");

    /* { 255/256, 0.5/255*255/256, 0, 0 } */
    shader_addline(&buffer, "PARAM constants = { 0.996, 0.00195, 0, 0 };\n");

    /* The alpha-component contains the palette index */
    shader_addline(&buffer, "TEX index, fragment.texcoord[0], texture[0], %s;\n", tex_target);

    /* Scale the index by 255/256 and add a bias of '0.5' in order to sample in the middle */
    shader_addline(&buffer, "MAD index.a, index.a, constants.x, constants.y;\n");

    /* Use the alpha-component as an index in the palette to get the final color */
    shader_addline(&buffer, "TEX result.color, index.a, texture[1], 1D;\n");
    shader_addline(&buffer, "END\n");

    shader_arb_compile(gl_info, GL_FRAGMENT_PROGRAM_ARB, buffer.buffer);

    string_buffer_free(&buffer);

    return shader;
}

/* Context activation is done by the caller. */
static void arbfp_blitter_upload_palette(struct wined3d_arbfp_blitter *blitter,
        const struct wined3d_texture_gl *texture_gl, struct wined3d_context_gl *context_gl)
{
    const struct wined3d_palette *palette = texture_gl->t.swapchain ? texture_gl->t.swapchain->palette : NULL;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (!blitter->palette_texture)
        gl_info->gl_ops.gl.p_glGenTextures(1, &blitter->palette_texture);

    GL_EXTCALL(glActiveTexture(GL_TEXTURE1));
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, blitter->palette_texture);

    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    /* Make sure we have discrete color levels. */
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    /* TODO: avoid unneeded uploads in the future by adding some SFLAG_PALETTE_DIRTY mechanism */
    if (palette)
    {
        gl_info->gl_ops.gl.p_glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 256, 0, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV, palette->colors);
    }
    else
    {
        static const DWORD black;
        FIXME("P8 surface loaded without a palette.\n");
        gl_info->gl_ops.gl.p_glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 1, 0, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV, &black);
    }

    /* Switch back to unit 0 in which the 2D texture will be stored. */
    wined3d_context_gl_active_texture(context_gl, gl_info, 0);
}

/* Context activation is done by the caller. */
static GLuint gen_yuv_shader(const struct wined3d_gl_info *gl_info, const struct arbfp_blit_type *type)
{
    GLuint shader;
    struct wined3d_string_buffer buffer;
    char luminance_component;

    if (type->use_color_key)
        FIXME("Implement YUV color keying.\n");

    /* Shader header */
    if (!string_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    GL_EXTCALL(glGenProgramsARB(1, &shader));
    checkGLcall("GL_EXTCALL(glGenProgramsARB(1, &shader))");
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader)");
    if (!shader)
    {
        string_buffer_free(&buffer);
        return 0;
    }

    /* The YUY2 and UYVY formats contain two pixels packed into a 32 bit macropixel,
     * giving effectively 16 bit per pixel. The color consists of a luminance(Y) and
     * two chroma(U and V) values. Each macropixel has two luminance values, one for
     * each single pixel it contains, and one U and one V value shared between both
     * pixels.
     *
     * The data is loaded into an A8L8 texture. With YUY2, the luminance component
     * contains the luminance and alpha the chroma. With UYVY it is vice versa. Thus
     * take the format into account when generating the read swizzles
     *
     * Reading the Y value is straightforward - just sample the texture. The hardware
     * takes care of filtering in the horizontal and vertical direction.
     *
     * Reading the U and V values is harder. We have to avoid filtering horizontally,
     * because that would mix the U and V values of one pixel or two adjacent pixels.
     * Thus floor the texture coordinate and add 0.5 to get an unfiltered read,
     * regardless of the filtering setting. Vertical filtering works automatically
     * though - the U and V values of two rows are mixed nicely.
     *
     * Apart of avoiding filtering issues, the code has to know which value it just
     * read, and where it can find the other one. To determine this, it checks if
     * it sampled an even or odd pixel, and shifts the 2nd read accordingly.
     *
     * Handling horizontal filtering of U and V values requires reading a 2nd pair
     * of pixels, extracting U and V and mixing them. This is not implemented yet.
     *
     * An alternative implementation idea is to load the texture as A8R8G8B8 texture,
     * with width / 2. This way one read gives all 3 values, finding U and V is easy
     * in an unfiltered situation. Finding the luminance on the other hand requires
     * finding out if it is an odd or even pixel. The real drawback of this approach
     * is filtering. This would have to be emulated completely in the shader, reading
     * up two 2 packed pixels in up to 2 rows and interpolating both horizontally and
     * vertically. Beyond that it would require adjustments to the texture handling
     * code to deal with the width scaling
     */
    shader_addline(&buffer, "!!ARBfp1.0\n");
    shader_addline(&buffer, "TEMP luminance;\n");
    shader_addline(&buffer, "TEMP temp;\n");
    shader_addline(&buffer, "TEMP chroma;\n");
    shader_addline(&buffer, "TEMP texcrd;\n");
    shader_addline(&buffer, "TEMP texcrd2;\n");
    shader_addline(&buffer, "PARAM coef = {1.0, 0.5, 2.0, 0.25};\n");
    shader_addline(&buffer, "PARAM yuv_coef = {1.403, 0.344, 0.714, 1.770};\n");
    shader_addline(&buffer, "PARAM size = program.local[%u];\n", ARBFP_BLIT_PARAM_SIZE);

    switch (type->fixup)
    {
        case COMPLEX_FIXUP_UYVY:
        case COMPLEX_FIXUP_YUY2:
            gen_packed_yuv_read(&buffer, type, &luminance_component);
            break;

        case COMPLEX_FIXUP_YV12:
            gen_yv12_read(&buffer, type, &luminance_component);
            break;

        case COMPLEX_FIXUP_NV12:
            gen_nv12_read(&buffer, type, &luminance_component);
            break;

        default:
            FIXME("Unsupported YUV fixup %#x\n", type->fixup);
            string_buffer_free(&buffer);
            return 0;
    }

    /* Calculate the final result. Formula is taken from
     * http://www.fourcc.org/fccyvrgb.php. Note that the chroma
     * ranges from -0.5 to 0.5
     */
    shader_addline(&buffer, "SUB chroma.xy, chroma, coef.y;\n");

    shader_addline(&buffer, "MAD result.color.x, chroma.x, yuv_coef.x, luminance.%c;\n", luminance_component);
    shader_addline(&buffer, "MAD temp.x, -chroma.y, yuv_coef.y, luminance.%c;\n", luminance_component);
    shader_addline(&buffer, "MAD result.color.y, -chroma.x, yuv_coef.z, temp.x;\n");
    shader_addline(&buffer, "MAD result.color.z, chroma.y, yuv_coef.w, luminance.%c;\n", luminance_component);
    shader_addline(&buffer, "END\n");

    shader_arb_compile(gl_info, GL_FRAGMENT_PROGRAM_ARB, buffer.buffer);

    string_buffer_free(&buffer);

    return shader;
}

/* Context activation is done by the caller. */
static GLuint arbfp_gen_plain_shader(const struct wined3d_gl_info *gl_info, const struct arbfp_blit_type *type)
{
    GLuint shader;
    struct wined3d_string_buffer buffer;
    const char *tex_target = arbfp_texture_target(type->res_type);

    /* Shader header */
    if (!string_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    GL_EXTCALL(glGenProgramsARB(1, &shader));
    if (!shader)
    {
        string_buffer_free(&buffer);
        return 0;
    }
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));

    shader_addline(&buffer, "!!ARBfp1.0\n");

    if (type->use_color_key)
    {
        shader_addline(&buffer, "TEMP color;\n");
        shader_addline(&buffer, "TEMP less, greater;\n");
        shader_addline(&buffer, "PARAM color_key_low = program.local[%u];\n", ARBFP_BLIT_PARAM_COLOR_KEY_LOW);
        shader_addline(&buffer, "PARAM color_key_high = program.local[%u];\n", ARBFP_BLIT_PARAM_COLOR_KEY_HIGH);
        shader_addline(&buffer, "TEX color, fragment.texcoord[0], texture[0], %s;\n", tex_target);
        shader_addline(&buffer, "SLT less, color, color_key_low;\n"); /* below low key */
        shader_addline(&buffer, "SGE greater, color, color_key_high;\n"); /* above high key */
        shader_addline(&buffer, "ADD less, less, greater;\n"); /* or */
        shader_addline(&buffer, "DP4 less.b, less, less;\n"); /* on any channel */
        shader_addline(&buffer, "SGE less, -less.b, 0.0;\n"); /* logical not */
        shader_addline(&buffer, "KIL -less;\n"); /* discard if true */
        shader_addline(&buffer, "MOV result.color, color;\n");
    }
    else
    {
        shader_addline(&buffer, "TEX result.color, fragment.texcoord[0], texture[0], %s;\n", tex_target);
    }

    shader_addline(&buffer, "END\n");

    shader_arb_compile(gl_info, GL_FRAGMENT_PROGRAM_ARB, buffer.buffer);

    string_buffer_free(&buffer);

    return shader;
}

/* Context activation is done by the caller. */
static HRESULT arbfp_blit_set(struct wined3d_arbfp_blitter *blitter, struct wined3d_context_gl *context_gl,
        const struct wined3d_texture_gl *texture_gl, unsigned int sub_resource_idx,
        const struct wined3d_color_key *color_key)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    enum complex_fixup fixup;
    struct wine_rb_entry *entry;
    struct arbfp_blit_type type;
    struct arbfp_blit_desc *desc;
    struct wined3d_color float_color_key[2];
    struct wined3d_vec4 size;
    unsigned int level;
    GLuint shader;

    level = sub_resource_idx % texture_gl->t.level_count;
    size.x = wined3d_texture_get_level_pow2_width(&texture_gl->t, level);
    size.y = wined3d_texture_get_level_pow2_height(&texture_gl->t, level);
    size.z = 1.0f;
    size.w = 1.0f;

    if (is_complex_fixup(texture_gl->t.resource.format->color_fixup))
        fixup = get_complex_fixup(texture_gl->t.resource.format->color_fixup);
    else
        fixup = COMPLEX_FIXUP_NONE;

    switch (texture_gl->target)
    {
        case GL_TEXTURE_1D:
            type.res_type = WINED3D_GL_RES_TYPE_TEX_1D;
            break;

        case GL_TEXTURE_2D:
            type.res_type = WINED3D_GL_RES_TYPE_TEX_2D;
            break;

        case GL_TEXTURE_3D:
            type.res_type = WINED3D_GL_RES_TYPE_TEX_3D;
            break;

        case GL_TEXTURE_CUBE_MAP_ARB:
            type.res_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            break;

        case GL_TEXTURE_RECTANGLE_ARB:
            type.res_type = WINED3D_GL_RES_TYPE_TEX_RECT;
            break;

        default:
            ERR("Unexpected GL texture type %#x.\n", texture_gl->target);
            type.res_type = WINED3D_GL_RES_TYPE_TEX_2D;
    }
    type.fixup = fixup;
    type.use_color_key = !!color_key;
    type.padding = 0;

    if ((entry = wine_rb_get(&blitter->shaders, &type)))
    {
        desc = WINE_RB_ENTRY_VALUE(entry, struct arbfp_blit_desc, entry);
        shader = desc->shader;
    }
    else
    {
        switch (fixup)
        {
            case COMPLEX_FIXUP_NONE:
                if (!is_identity_fixup(texture_gl->t.resource.format->color_fixup))
                    FIXME("Implement support for sign or swizzle fixups.\n");
                shader = arbfp_gen_plain_shader(gl_info, &type);
                break;

            case COMPLEX_FIXUP_P8:
                shader = gen_p8_shader(gl_info, &type);
                break;

            case COMPLEX_FIXUP_YUY2:
            case COMPLEX_FIXUP_UYVY:
            case COMPLEX_FIXUP_YV12:
            case COMPLEX_FIXUP_NV12:
                shader = gen_yuv_shader(gl_info, &type);
                break;
        }

        if (!shader)
        {
            FIXME("Unsupported complex fixup %#x, not setting a shader\n", fixup);
            return E_NOTIMPL;
        }

        if (!(desc = heap_alloc(sizeof(*desc))))
            goto err_out;

        desc->type = type;
        desc->shader = shader;
        if (wine_rb_put(&blitter->shaders, &desc->type, &desc->entry) == -1)
        {
err_out:
            ERR("Out of memory\n");
            GL_EXTCALL(glDeleteProgramsARB(1, &shader));
            checkGLcall("GL_EXTCALL(glDeleteProgramsARB(1, &shader))");
            GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0));
            checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0)");
            heap_free(desc);
            return E_OUTOFMEMORY;
        }
    }

    if (fixup == COMPLEX_FIXUP_P8)
        arbfp_blitter_upload_palette(blitter, texture_gl, context_gl);

    gl_info->gl_ops.gl.p_glEnable(GL_FRAGMENT_PROGRAM_ARB);
    checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB)");
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader)");
    GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARBFP_BLIT_PARAM_SIZE, &size.x));
    checkGLcall("glProgramLocalParameter4fvARB");
    if (type.use_color_key)
    {
        wined3d_format_get_float_color_key(texture_gl->t.resource.format, color_key, float_color_key);
        GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,
                ARBFP_BLIT_PARAM_COLOR_KEY_LOW, &float_color_key[0].r));
        GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,
                ARBFP_BLIT_PARAM_COLOR_KEY_HIGH, &float_color_key[1].r));
        checkGLcall("glProgramLocalParameter4fvARB");
    }

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void arbfp_blit_unset(const struct wined3d_gl_info *gl_info)
{
    gl_info->gl_ops.gl.p_glDisable(GL_FRAGMENT_PROGRAM_ARB);
    checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
}

static BOOL arbfp_blit_supported(enum wined3d_blit_op blit_op, const struct wined3d_context *context,
        const struct wined3d_resource *src_resource, DWORD src_location,
        const struct wined3d_resource *dst_resource, DWORD dst_location)
{
    const struct wined3d_format *src_format = src_resource->format;
    const struct wined3d_format *dst_format = dst_resource->format;
    enum complex_fixup src_fixup;
    BOOL decompress;

    if (src_resource->type != WINED3D_RTYPE_TEXTURE_2D)
        return FALSE;

    if (blit_op == WINED3D_BLIT_OP_RAW_BLIT && dst_format->id == src_format->id)
    {
        if (dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            blit_op = WINED3D_BLIT_OP_DEPTH_BLIT;
        else
            blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
    }

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT_CKEY:
            if (!context->d3d_info->shader_color_key)
            {
                /* The conversion modifies the alpha channel so the color key might no longer match. */
                TRACE("Color keying not supported with converted textures.\n");
                return FALSE;
            }
        case WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST:
        case WINED3D_BLIT_OP_COLOR_BLIT:
            break;

        default:
            TRACE("Unsupported blit_op=%d\n", blit_op);
            return FALSE;
    }

    decompress = (src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED)
            && !(dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED);
    if (!decompress && !(src_resource->access & dst_resource->access & WINED3D_RESOURCE_ACCESS_GPU))
        return FALSE;

    src_fixup = get_complex_fixup(src_format->color_fixup);
    if (TRACE_ON(d3d_shader) && TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(src_format->color_fixup);
    }

    if (!is_identity_fixup(dst_format->color_fixup)
            && (dst_format->id != src_format->id || dst_location != WINED3D_LOCATION_DRAWABLE))
    {
        TRACE("Destination fixups are not supported\n");
        return FALSE;
    }

    if (is_identity_fixup(src_format->color_fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

     /* We only support YUV conversions. */
    if (!is_complex_fixup(src_format->color_fixup))
    {
        if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
        {
            WARN("Claiming fixup support because of ORM_BACKBUFFER.\n");
            return TRUE;
        }

        TRACE("[FAILED]\n");
        return FALSE;
    }

    switch(src_fixup)
    {
        case COMPLEX_FIXUP_YUY2:
        case COMPLEX_FIXUP_UYVY:
        case COMPLEX_FIXUP_YV12:
        case COMPLEX_FIXUP_NV12:
        case COMPLEX_FIXUP_P8:
            TRACE("[OK]\n");
            return TRUE;

        default:
            FIXME("Unsupported YUV fixup %#x\n", src_fixup);
            TRACE("[FAILED]\n");
            return FALSE;
    }
}

static DWORD arbfp_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *color_key, enum wined3d_texture_filter_type filter)
{
    struct wined3d_texture_gl *src_texture_gl = wined3d_texture_gl(src_texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_device *device = dst_texture->resource.device;
    struct wined3d_texture *staging_texture = NULL;
    struct wined3d_arbfp_blitter *arbfp_blitter;
    struct wined3d_color_key alpha_test_key;
    struct wined3d_blitter *next;
    unsigned int src_level;
    RECT s, d;

    TRACE("blitter %p, op %#x, context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_rect %s, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s, colour_key %p, filter %s.\n",
            blitter, op, context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            wine_dbgstr_rect(src_rect), dst_texture, dst_sub_resource_idx, wined3d_debug_location(dst_location),
            wine_dbgstr_rect(dst_rect), color_key, debug_d3dtexturefiltertype(filter));

    if (!arbfp_blit_supported(op, context, &src_texture->resource, src_location,
            &dst_texture->resource, dst_location))
    {
        if (!(next = blitter->next))
        {
            ERR("No blitter to handle blit op %#x.\n", op);
            return dst_location;
        }

        TRACE("Forwarding to blitter %p.\n", next);
        return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, color_key, filter);
    }

    arbfp_blitter = CONTAINING_RECORD(blitter, struct wined3d_arbfp_blitter, blitter);

    if (!(src_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        struct wined3d_resource_desc desc;
        struct wined3d_box upload_box;
        HRESULT hr;

        TRACE("Source texture is not GPU accessible, creating a staging texture.\n");

        src_level = src_sub_resource_idx % src_texture->level_count;
        desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
        desc.format = src_texture->resource.format->id;
        desc.multisample_type = src_texture->resource.multisample_type;
        desc.multisample_quality = src_texture->resource.multisample_quality;
        desc.usage = WINED3DUSAGE_PRIVATE;
        desc.bind_flags = 0;
        desc.access = WINED3D_RESOURCE_ACCESS_GPU;
        desc.width = wined3d_texture_get_level_width(src_texture, src_level);
        desc.height = wined3d_texture_get_level_height(src_texture, src_level);
        desc.depth = 1;
        desc.size = 0;

        if (FAILED(hr = wined3d_texture_create(device, &desc, 1, 1, 0,
                NULL, NULL, &wined3d_null_parent_ops, &staging_texture)))
        {
            ERR("Failed to create staging texture, hr %#x.\n", hr);
            return dst_location;
        }

        wined3d_box_set(&upload_box, 0, 0, desc.width, desc.height, 0, desc.depth);
        wined3d_texture_upload_from_texture(staging_texture, 0, 0, 0, 0,
                src_texture, src_sub_resource_idx, &upload_box);

        src_texture = staging_texture;
        src_texture_gl = wined3d_texture_gl(src_texture);
        src_sub_resource_idx = 0;
    }
    else if (wined3d_settings.offscreen_rendering_mode != ORM_FBO
            && (src_texture->sub_resources[src_sub_resource_idx].locations
            & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_DRAWABLE)) == WINED3D_LOCATION_DRAWABLE
            && !wined3d_resource_is_offscreen(&src_texture->resource))
    {

        /* Without FBO blits transferring from the drawable to the texture is
         * expensive, because we have to flip the data in sysmem. Since we can
         * flip in the blitter, we don't actually need that flip anyway. So we
         * use the surface's texture as scratch texture, and flip the source
         * rectangle instead. */
        texture2d_load_fb_texture(src_texture_gl, src_sub_resource_idx, FALSE, context);

        s = *src_rect;
        src_level = src_sub_resource_idx % src_texture->level_count;
        s.top = wined3d_texture_get_level_height(src_texture, src_level) - s.top;
        s.bottom = wined3d_texture_get_level_height(src_texture, src_level) - s.bottom;
        src_rect = &s;
    }
    else
    {
        wined3d_texture_load(src_texture, context, FALSE);
    }

    wined3d_context_gl_apply_ffp_blit_state(context_gl, device);

    if (dst_location == WINED3D_LOCATION_DRAWABLE)
    {
        d = *dst_rect;
        wined3d_texture_translate_drawable_coords(dst_texture, context_gl->window, &d);
        dst_rect = &d;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        GLenum buffer;

        if (dst_location == WINED3D_LOCATION_DRAWABLE)
        {
            TRACE("Destination texture %p is onscreen.\n", dst_texture);
            buffer = wined3d_texture_get_gl_buffer(dst_texture);
        }
        else
        {
            TRACE("Destination texture %p is offscreen.\n", dst_texture);
            buffer = GL_COLOR_ATTACHMENT0;
        }
        wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_DRAW_FRAMEBUFFER,
                &dst_texture->resource, dst_sub_resource_idx, NULL, 0, dst_location);
        wined3d_context_gl_set_draw_buffer(context_gl, buffer);
        wined3d_context_gl_check_fbo_status(context_gl, GL_DRAW_FRAMEBUFFER);
        context_invalidate_state(context, STATE_FRAMEBUFFER);
    }

    if (op == WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST)
    {
        const struct wined3d_format *fmt = src_texture->resource.format;
        alpha_test_key.color_space_low_value = 0;
        alpha_test_key.color_space_high_value = ~(((1u << fmt->alpha_size) - 1) << fmt->alpha_offset);
        color_key = &alpha_test_key;
    }

    arbfp_blit_set(arbfp_blitter, context_gl, src_texture_gl, src_sub_resource_idx, color_key);

    /* Draw a textured quad */
    wined3d_context_gl_draw_textured_quad(context_gl, src_texture_gl,
            src_sub_resource_idx, src_rect, dst_rect, filter);

    /* Leave the opengl state valid for blitting */
    arbfp_blit_unset(context_gl->gl_info);

    if (dst_texture->swapchain && (dst_texture->swapchain->front_buffer == dst_texture))
        context_gl->gl_info->gl_ops.gl.p_glFlush();

    if (staging_texture)
        wined3d_texture_decref(staging_texture);

    return dst_location;
}

static void arbfp_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, DWORD flags, const struct wined3d_color *colour, float depth, DWORD stencil)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_clear(next, device, rt_count, fb, rect_count,
                clear_rects, draw_rect, flags, colour, depth, stencil);
}

static const struct wined3d_blitter_ops arbfp_blitter_ops =
{
    arbfp_blitter_destroy,
    arbfp_blitter_clear,
    arbfp_blitter_blit,
};

void wined3d_arbfp_blitter_create(struct wined3d_blitter **next, const struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_arbfp_blitter *blitter;

    if (device->shader_backend != &arb_program_shader_backend
            && device->shader_backend != &glsl_shader_backend)
        return;

    if (!gl_info->supported[ARB_FRAGMENT_PROGRAM])
        return;

    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        return;

    if (!(blitter = heap_alloc(sizeof(*blitter))))
    {
        ERR("Failed to allocate blitter.\n");
        return;
    }

    TRACE("Created blitter %p.\n", blitter);

    blitter->blitter.ops = &arbfp_blitter_ops;
    blitter->blitter.next = *next;
    wine_rb_init(&blitter->shaders, arbfp_blit_type_compare);
    blitter->palette_texture = 0;
    *next = &blitter->blitter;
}
