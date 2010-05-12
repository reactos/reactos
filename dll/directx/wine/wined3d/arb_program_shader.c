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

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_constants);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

#define GLINFO_LOCATION      (*gl_info)

/* Extract a line. Note that this modifies the source string. */
static char *get_line(char **ptr)
{
    char *p, *q;

    p = *ptr;
    if (!(q = strstr(p, "\n")))
    {
        if (!*p) return NULL;
        *ptr += strlen(p);
        return p;
    }
    *q = '\0';
    *ptr = q + 1;

    return p;
}

static void shader_arb_dump_program_source(const char *source)
{
    unsigned long source_size;
    char *ptr, *line, *tmp;

    source_size = strlen(source) + 1;
    tmp = HeapAlloc(GetProcessHeap(), 0, source_size);
    if (!tmp)
    {
        ERR("Failed to allocate %lu bytes for shader source.\n", source_size);
        return;
    }
    memcpy(tmp, source, source_size);

    ptr = tmp;
    while ((line = get_line(&ptr))) FIXME("    %s\n", line);
    FIXME("\n");

    HeapFree(GetProcessHeap(), 0, tmp);
}

/* GL locking for state handlers is done by the caller. */
static BOOL need_mova_const(IWineD3DBaseShader *shader, const struct wined3d_gl_info *gl_info)
{
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) shader;
    if(!This->baseShader.reg_maps.usesmova) return FALSE;
    return !gl_info->supported[NV_VERTEX_PROGRAM2_OPTION];
}

/* Returns TRUE if result.clip from GL_NV_vertex_program2 should be used and FALSE otherwise */
static inline BOOL use_nv_clip(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[NV_VERTEX_PROGRAM2_OPTION]
            && !(gl_info->quirks & WINED3D_QUIRK_NV_CLIP_BROKEN);
}

static BOOL need_helper_const(const struct wined3d_gl_info *gl_info)
{
    if (!gl_info->supported[NV_VERTEX_PROGRAM] /* Need to init colors. */
            || gl_info->quirks & WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT /* Load the immval offset. */
            || gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W /* Have to init texcoords. */
            || (!use_nv_clip(gl_info)) /* Init the clip texcoord */)
    {
        return TRUE;
    }
    return FALSE;
}

static unsigned int reserved_vs_const(IWineD3DBaseShader *shader, const struct wined3d_gl_info *gl_info)
{
    unsigned int ret = 1;
    /* We use one PARAM for the pos fixup, and in some cases one to load
     * some immediate values into the shader
     */
    if(need_helper_const(gl_info)) ret++;
    if(need_mova_const(shader, gl_info)) ret++;
    return ret;
}

static inline BOOL ffp_clip_emul(IWineD3DStateBlockImpl *stateblock)
{
    return stateblock->lowest_disabled_stage < 7;
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
    /* For ARB we need a offset value:
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
    unsigned char                   loop_ctrl[MAX_CONST_I][3];
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
    struct stb_const_desc           bumpenvmatconst[MAX_TEXTURES];
    struct stb_const_desc           luminanceconst[MAX_TEXTURES];
    UINT                            int_consts[MAX_CONST_I];
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
            char                    clip_texcoord;
            char                    clipplane_mask;
        }                           boolclip;
        DWORD                       boolclip_compare;
    } clip;
    DWORD                           ps_signature;
    union
    {
        unsigned char               samplers[4];
        DWORD                       samplers_compare;
    } vertex;
    unsigned char                   loop_ctrl[MAX_CONST_I][3];
};

struct arb_vs_compiled_shader
{
    struct arb_vs_compile_args      args;
    GLuint                          prgId;
    UINT                            int_consts[MAX_CONST_I];
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
    char addr_reg[20];
    enum
    {
        /* plain GL_ARB_vertex_program or GL_ARB_fragment_program */
        ARB,
        /* GL_NV_vertex_progam2_option or GL_NV_fragment_program_option */
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
    struct wined3d_shader_signature_element *sig;
    DWORD                               idx;
    struct wine_rb_entry                entry;
};

struct arb_pshader_private {
    struct arb_ps_compiled_shader   *gl_shaders;
    UINT                            num_gl_shaders, shader_array_size;
    BOOL                            has_signature_idx;
    DWORD                           input_signature_idx;
    DWORD                           clipplane_emulation;
    BOOL                            clamp_consts;
};

struct arb_vshader_private {
    struct arb_vs_compiled_shader   *gl_shaders;
    UINT                            num_gl_shaders, shader_array_size;
};

struct shader_arb_priv
{
    GLuint                  current_vprogram_id;
    GLuint                  current_fprogram_id;
    const struct arb_ps_compiled_shader *compiled_fprog;
    const struct arb_vs_compiled_shader *compiled_vprog;
    GLuint                  depth_blt_vprogram_id;
    GLuint                  depth_blt_fprogram_id[tex_type_count];
    BOOL                    use_arbfp_fixed_func;
    struct wine_rb_tree     fragment_shaders;
    BOOL                    last_ps_const_clamped;
    BOOL                    last_vs_color_unclamp;

    struct wine_rb_tree     signature_tree;
    DWORD ps_sig_number;
};

/********************************************************
 * ARB_[vertex/fragment]_program helper functions follow
 ********************************************************/

/* Loads floating point constants into the currently set ARB_vertex/fragment_program.
 * When constant_list == NULL, it will load all the constants.
 *
 * @target_type should be either GL_VERTEX_PROGRAM_ARB (for vertex shaders)
 *  or GL_FRAGMENT_PROGRAM_ARB (for pixel shaders)
 */
/* GL locking is done by the caller */
static unsigned int shader_arb_load_constantsF(IWineD3DBaseShaderImpl *This, const struct wined3d_gl_info *gl_info,
        GLuint target_type, unsigned int max_constants, const float *constants, char *dirty_consts)
{
    local_constant* lconst;
    DWORD i, j;
    unsigned int ret;

    if (TRACE_ON(d3d_constants))
    {
        for(i = 0; i < max_constants; i++) {
            if(!dirty_consts[i]) continue;
            TRACE_(d3d_constants)("Loading constants %i: %f, %f, %f, %f\n", i,
                        constants[i * 4 + 0], constants[i * 4 + 1],
                        constants[i * 4 + 2], constants[i * 4 + 3]);
        }
    }

    i = 0;

    /* In 1.X pixel shaders constants are implicitly clamped in the range [-1;1] */
    if (target_type == GL_FRAGMENT_PROGRAM_ARB && This->baseShader.reg_maps.shader_version.major == 1)
    {
        float lcl_const[4];
        /* ps 1.x supports only 8 constants, clamp only those. When switching between 1.x and higher
         * shaders, the first 8 constants are marked dirty for reload
         */
        for(; i < min(8, max_constants); i++) {
            if(!dirty_consts[i]) continue;
            dirty_consts[i] = 0;

            j = 4 * i;
            if (constants[j + 0] > 1.0f) lcl_const[0] = 1.0f;
            else if (constants[j + 0] < -1.0f) lcl_const[0] = -1.0f;
            else lcl_const[0] = constants[j + 0];

            if (constants[j + 1] > 1.0f) lcl_const[1] = 1.0f;
            else if (constants[j + 1] < -1.0f) lcl_const[1] = -1.0f;
            else lcl_const[1] = constants[j + 1];

            if (constants[j + 2] > 1.0f) lcl_const[2] = 1.0f;
            else if (constants[j + 2] < -1.0f) lcl_const[2] = -1.0f;
            else lcl_const[2] = constants[j + 2];

            if (constants[j + 3] > 1.0f) lcl_const[3] = 1.0f;
            else if (constants[j + 3] < -1.0f) lcl_const[3] = -1.0f;
            else lcl_const[3] = constants[j + 3];

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
        for(; i < max_constants; i++) {
            if(!dirty_consts[i]) continue;

            /* Find the next block of dirty constants */
            dirty_consts[i] = 0;
            j = i;
            for(i++; (i < max_constants) && dirty_consts[i]; i++) {
                dirty_consts[i] = 0;
            }

            GL_EXTCALL(glProgramEnvParameters4fvEXT(target_type, j, i - j, constants + (j * 4)));
        }
    } else {
        for(; i < max_constants; i++) {
            if(dirty_consts[i]) {
                dirty_consts[i] = 0;
                GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, constants + (i * 4)));
            }
        }
    }
    checkGLcall("glProgramEnvParameter4fvARB()");

    /* Load immediate constants */
    if(This->baseShader.load_local_constsF) {
        if (TRACE_ON(d3d_shader)) {
            LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
                GLfloat* values = (GLfloat*)lconst->value;
                TRACE_(d3d_constants)("Loading local constants %i: %f, %f, %f, %f\n", lconst->idx,
                        values[0], values[1], values[2], values[3]);
            }
        }
        /* Immediate constants are clamped for 1.X shaders at loading times */
        ret = 0;
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
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

/**
 * Loads the texture dimensions for NP2 fixup into the currently set ARB_[vertex/fragment]_programs.
 */
static void shader_arb_load_np2fixup_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader) {

    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl *) device;
    const struct shader_arb_priv* const priv = (const struct shader_arb_priv *) deviceImpl->shader_priv;
    IWineD3DStateBlockImpl* stateBlock = deviceImpl->stateBlock;
    const struct wined3d_gl_info *gl_info = &deviceImpl->adapter->gl_info;

    if (!usePixelShader) {
        /* NP2 texcoord fixup is (currently) only done for pixelshaders. */
        return;
    }

    if (priv->compiled_fprog && priv->compiled_fprog->np2fixup_info.super.active) {
        const struct arb_ps_np2fixup_info* const fixup = &priv->compiled_fprog->np2fixup_info;
        UINT i;
        WORD active = fixup->super.active;
        GLfloat np2fixup_constants[4 * MAX_FRAGMENT_SAMPLERS];

        for (i = 0; active; active >>= 1, ++i) {
            const unsigned char idx = fixup->super.idx[i];
            const IWineD3DTextureImpl* const tex = (const IWineD3DTextureImpl*) stateBlock->textures[i];
            GLfloat* tex_dim = &np2fixup_constants[(idx >> 1) * 4];

            if (!(active & 1)) continue;

            if (!tex) {
                FIXME("Nonexistent texture is flagged for NP2 texcoord fixup\n");
                continue;
            }

            if (idx % 2) {
                tex_dim[2] = tex->baseTexture.pow2Matrix[0]; tex_dim[3] = tex->baseTexture.pow2Matrix[5];
            } else {
                tex_dim[0] = tex->baseTexture.pow2Matrix[0]; tex_dim[1] = tex->baseTexture.pow2Matrix[5];
            }
        }

        for (i = 0; i < fixup->super.num_consts; ++i) {
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,
                                                   fixup->offset + i, &np2fixup_constants[i * 4]));
        }
    }
}

/* GL locking is done by the caller. */
static inline void shader_arb_ps_local_constants(IWineD3DDeviceImpl* deviceImpl)
{
    const struct wined3d_context *context = context_get_current();
    IWineD3DStateBlockImpl* stateBlock = deviceImpl->stateBlock;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned char i;
    struct shader_arb_priv *priv = deviceImpl->shader_priv;
    const struct arb_ps_compiled_shader *gl_shader = priv->compiled_fprog;

    for(i = 0; i < gl_shader->numbumpenvmatconsts; i++)
    {
        int texunit = gl_shader->bumpenvmatconst[i].texunit;

        /* The state manager takes care that this function is always called if the bump env matrix changes */
        const float *data = (const float *)&stateBlock->textureState[texunit][WINED3DTSS_BUMPENVMAT00];
        GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader->bumpenvmatconst[i].const_num, data));

        if (gl_shader->luminanceconst[i].const_num != WINED3D_CONST_NUM_UNUSED)
        {
            /* WINED3DTSS_BUMPENVLSCALE and WINED3DTSS_BUMPENVLOFFSET are next to each other.
             * point gl to the scale, and load 4 floats. x = scale, y = offset, z and w are junk, we
             * don't care about them. The pointers are valid for sure because the stateblock is bigger.
             * (they're WINED3DTSS_TEXTURETRANSFORMFLAGS and WINED3DTSS_ADDRESSW, so most likely 0 or NaN
            */
            const float *scale = (const float *)&stateBlock->textureState[texunit][WINED3DTSS_BUMPENVLSCALE];
            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader->luminanceconst[i].const_num, scale));
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
        val[0] = context->render_offscreen ? 0.0f
                : ((IWineD3DSurfaceImpl *) deviceImpl->render_targets[0])->currentDesc.Height;
        val[1] = context->render_offscreen ? 1.0f : -1.0f;
        val[2] = 1.0f;
        val[3] = 0.0f;
        GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader->ycorrection, val));
        checkGLcall("y correction loading");
    }

    if(gl_shader->num_int_consts == 0) return;

    for(i = 0; i < MAX_CONST_I; i++)
    {
        if(gl_shader->int_consts[i] != WINED3D_CONST_NUM_UNUSED)
        {
            float val[4];
            val[0] = stateBlock->pixelShaderConstantI[4 * i];
            val[1] = stateBlock->pixelShaderConstantI[4 * i + 1];
            val[2] = stateBlock->pixelShaderConstantI[4 * i + 2];
            val[3] = -1.0f;

            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader->int_consts[i], val));
        }
    }
    checkGLcall("Load ps int consts");
}

/* GL locking is done by the caller. */
static inline void shader_arb_vs_local_constants(IWineD3DDeviceImpl* deviceImpl)
{
    IWineD3DStateBlockImpl* stateBlock;
    const struct wined3d_gl_info *gl_info = &deviceImpl->adapter->gl_info;
    unsigned char i;
    struct shader_arb_priv *priv = deviceImpl->shader_priv;
    const struct arb_vs_compiled_shader *gl_shader = priv->compiled_vprog;

    /* Upload the position fixup */
    GL_EXTCALL(glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, gl_shader->pos_fixup, deviceImpl->posFixup));

    if(gl_shader->num_int_consts == 0) return;

    stateBlock = deviceImpl->stateBlock;

    for(i = 0; i < MAX_CONST_I; i++)
    {
        if(gl_shader->int_consts[i] != WINED3D_CONST_NUM_UNUSED)
        {
            float val[4];
            val[0] = stateBlock->vertexShaderConstantI[4 * i];
            val[1] = stateBlock->vertexShaderConstantI[4 * i + 1];
            val[2] = stateBlock->vertexShaderConstantI[4 * i + 2];
            val[3] = -1.0f;

            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, gl_shader->int_consts[i], val));
        }
    }
    checkGLcall("Load vs int consts");
}

/**
 * Loads the app-supplied constants into the currently set ARB_[vertex/fragment]_programs.
 *
 * We only support float constants in ARB at the moment, so don't
 * worry about the Integers or Booleans
 */
/* GL locking is done by the caller (state handler) */
static void shader_arb_load_constants(const struct wined3d_context *context, char usePixelShader, char useVertexShader)
{
    IWineD3DDeviceImpl *device = ((IWineD3DSurfaceImpl *)context->surface)->resource.device;
    IWineD3DStateBlockImpl* stateBlock = device->stateBlock;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;

        /* Load DirectX 9 float constants for vertex shader */
        device->highest_dirty_vs_const = shader_arb_load_constantsF(vshader, gl_info, GL_VERTEX_PROGRAM_ARB,
                device->highest_dirty_vs_const, stateBlock->vertexShaderConstantF, context->vshader_const_dirty);
        shader_arb_vs_local_constants(device);
    }

    if (usePixelShader) {
        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;

        /* Load DirectX 9 float constants for pixel shader */
        device->highest_dirty_ps_const = shader_arb_load_constantsF(pshader, gl_info, GL_FRAGMENT_PROGRAM_ARB,
                device->highest_dirty_ps_const, stateBlock->pixelShaderConstantF, context->pshader_const_dirty);
        shader_arb_ps_local_constants(device);
    }
}

static void shader_arb_update_float_vertex_constants(IWineD3DDevice *iface, UINT start, UINT count)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct wined3d_context *context = context_get_current();

    /* We don't want shader constant dirtification to be an O(contexts), so just dirtify the active
     * context. On a context switch the old context will be fully dirtified */
    if (!context || ((IWineD3DSurfaceImpl *)context->surface)->resource.device != This) return;

    memset(context->vshader_const_dirty + start, 1, sizeof(*context->vshader_const_dirty) * count);
    This->highest_dirty_vs_const = max(This->highest_dirty_vs_const, start + count);
}

static void shader_arb_update_float_pixel_constants(IWineD3DDevice *iface, UINT start, UINT count)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct wined3d_context *context = context_get_current();

    /* We don't want shader constant dirtification to be an O(contexts), so just dirtify the active
     * context. On a context switch the old context will be fully dirtified */
    if (!context || ((IWineD3DSurfaceImpl *)context->surface)->resource.device != This) return;

    memset(context->pshader_const_dirty + start, 1, sizeof(*context->pshader_const_dirty) * count);
    This->highest_dirty_ps_const = max(This->highest_dirty_ps_const, start + count);
}

static DWORD *local_const_mapping(IWineD3DBaseShaderImpl *This)
{
    DWORD *ret;
    DWORD idx = 0;
    const local_constant *lconst;

    if(This->baseShader.load_local_constsF || list_empty(&This->baseShader.constantsF)) return NULL;

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD) * This->baseShader.limits.constant_float);
    if(!ret) {
        ERR("Out of memory\n");
        return NULL;
    }

    LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
        ret[lconst->idx] = idx++;
    }
    return ret;
}

/* Generate the variable & register declarations for the ARB_vertex_program output target */
static DWORD shader_generate_arb_declarations(IWineD3DBaseShader *iface, const shader_reg_maps *reg_maps,
        struct wined3d_shader_buffer *buffer, const struct wined3d_gl_info *gl_info, DWORD *lconst_map,
        DWORD *num_clipplanes, struct shader_arb_ctx_priv *ctx)
{
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    DWORD i, next_local = 0;
    char pshader = shader_is_pshader_version(reg_maps->shader_version.type);
    unsigned max_constantsF;
    const local_constant *lconst;
    DWORD map;

    /* In pixel shaders, all private constants are program local, we don't need anything
     * from program.env. Thus we can advertise the full set of constants in pixel shaders.
     * If we need a private constant the GL implementation will squeeze it in somewhere
     *
     * With vertex shaders we need the posFixup and on some GL implementations 4 helper
     * immediate values. The posFixup is loaded using program.env for now, so always
     * subtract one from the number of constants. If the shader uses indirect addressing,
     * account for the helper const too because we have to declare all availabke d3d constants
     * and don't know which are actually used.
     */
    if (pshader)
    {
        max_constantsF = gl_info->limits.arb_ps_native_constants;
    }
    else
    {
        if(This->baseShader.reg_maps.usesrelconstF) {
            DWORD highest_constf = 0, clip_limit;
            max_constantsF = gl_info->limits.arb_vs_native_constants - reserved_vs_const(iface, gl_info);
            max_constantsF -= count_bits(This->baseShader.reg_maps.integer_constants);

            for(i = 0; i < This->baseShader.limits.constant_float; i++)
            {
                DWORD idx = i >> 5;
                DWORD shift = i & 0x1f;
                if(reg_maps->constf[idx] & (1 << shift)) highest_constf = i;
            }

            if(use_nv_clip(gl_info) && ctx->target_version >= NV2)
            {
                if(ctx->cur_vs_args->super.clip_enabled)
                    clip_limit = gl_info->limits.clipplanes;
                else
                    clip_limit = 0;
            }
            else
            {
                unsigned int mask = ctx->cur_vs_args->clip.boolclip.clipplane_mask;
                clip_limit = min(count_bits(mask), 4);
            }
            *num_clipplanes = min(clip_limit, max_constantsF - highest_constf - 1);
            max_constantsF -= *num_clipplanes;
            if(*num_clipplanes < clip_limit)
            {
                WARN("Only %u clipplanes out of %u enabled\n", *num_clipplanes, gl_info->limits.clipplanes);
            }
        }
        else
        {
            if (ctx->target_version >= NV2) *num_clipplanes = gl_info->limits.clipplanes;
            else *num_clipplanes = min(gl_info->limits.clipplanes, 4);
            max_constantsF = gl_info->limits.arb_vs_native_constants;
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

    /* Load local constants using the program-local space,
     * this avoids reloading them each time the shader is used
     */
    if(lconst_map) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            shader_addline(buffer, "PARAM C%u = program.local[%u];\n", lconst->idx,
                           lconst_map[lconst->idx]);
            next_local = max(next_local, lconst_map[lconst->idx] + 1);
        }
    }

    /* After subtracting privately used constants from the hardware limit(they are loaded as
     * local constants), make sure the shader doesn't violate the env constant limit
     */
    if(pshader)
    {
        max_constantsF = min(max_constantsF, gl_info->limits.arb_ps_float_constants);
    }
    else
    {
        max_constantsF = min(max_constantsF, gl_info->limits.arb_vs_float_constants);
    }

    /* Avoid declaring more constants than needed */
    max_constantsF = min(max_constantsF, This->baseShader.limits.constant_float);

    /* we use the array-based constants array if the local constants are marked for loading,
     * because then we use indirect addressing, or when the local constant list is empty,
     * because then we don't know if we're using indirect addressing or not. If we're hardcoding
     * local constants do not declare the loaded constants as an array because ARB compilers usually
     * do not optimize unused constants away
     */
    if(This->baseShader.reg_maps.usesrelconstF) {
        /* Need to PARAM the environment parameters (constants) so we can use relative addressing */
        shader_addline(buffer, "PARAM C[%d] = { program.env[0..%d] };\n",
                    max_constantsF, max_constantsF - 1);
    } else {
        for(i = 0; i < max_constantsF; i++) {
            DWORD idx, mask;
            idx = i >> 5;
            mask = 1 << (i & 0x1f);
            if(!shader_constant_is_local(This, i) && (This->baseShader.reg_maps.constf[idx] & mask)) {
                shader_addline(buffer, "PARAM C%d = program.env[%d];\n",i, i);
            }
        }
    }

    return next_local;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;

    if(strcmp(priv->addr_reg, src) == 0) return;

    strcpy(priv->addr_reg, src);
    shader_addline(buffer, "ARL A0.x, %s;\n", src);
}

static void shader_arb_get_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *src, unsigned int tmpreg, char *outregstr);

static void shader_arb_get_register_name(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_register *reg, char *register_name, BOOL *is_color)
{
    /* oPos, oFog and oPts in D3D */
    static const char * const rastout_reg_names[] = {"TMP_OUT", "result.fogcoord", "result.pointsize"};
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    BOOL pshader = shader_is_pshader_version(This->baseShader.reg_maps.shader_version.type);
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;

    *is_color = FALSE;

    switch (reg->type)
    {
        case WINED3DSPR_TEMP:
            sprintf(register_name, "R%u", reg->idx);
            break;

        case WINED3DSPR_INPUT:
            if (pshader)
            {
                if(This->baseShader.reg_maps.shader_version.major < 3)
                {
                    if (reg->idx == 0) strcpy(register_name, "fragment.color.primary");
                    else strcpy(register_name, "fragment.color.secondary");
                }
                else
                {
                    if(reg->rel_addr)
                    {
                        char rel_reg[50];
                        shader_arb_get_src_param(ins, reg->rel_addr, 0, rel_reg);

                        if(strcmp(rel_reg, "**aL_emul**") == 0)
                        {
                            DWORD idx = ctx->aL + reg->idx;
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
                        else if(This->baseShader.reg_maps.input_registers & 0x0300)
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
                            sprintf(register_name, "fragment.texcoord[%s + %u]", rel_reg, reg->idx);
                        }
                        else if(ctx->cur_ps_args->super.vp_mode != vertexshader)
                        {
                            /* This is problematic because we'd have to consult the ctx->ps_input strings
                             * for where to find the varying. Some may be "0.0", others can be texcoords or
                             * colors. This needs either a pipeline replacement to make the vertex shader feed
                             * proper varyings, or loop unrolling
                             *
                             * For now use the texcoords and hope for the best
                             */
                            FIXME("Non-vertex shader varying input with indirect addressing\n");
                            sprintf(register_name, "fragment.texcoord[%s + %u]", rel_reg, reg->idx);
                        }
                        else
                        {
                            /* D3D supports indirect addressing only with aL in loop registers. The loop instruction
                             * pulls GL_NV_fragment_program2 in
                             */
                            sprintf(register_name, "fragment.texcoord[%s + %u]", rel_reg, reg->idx);
                        }
                    }
                    else
                    {
                        if(reg->idx < MAX_REG_INPUT)
                        {
                            strcpy(register_name, ctx->ps_input[reg->idx]);
                        }
                        else
                        {
                            ERR("Pixel shader input register out of bounds: %u\n", reg->idx);
                            sprintf(register_name, "out_of_bounds_%u", reg->idx);
                        }
                    }
                }
            }
            else
            {
                if (ctx->cur_vs_args->super.swizzle_map & (1 << reg->idx)) *is_color = TRUE;
                sprintf(register_name, "vertex.attrib[%u]", reg->idx);
            }
            break;

        case WINED3DSPR_CONST:
            if (!pshader && reg->rel_addr)
            {
                BOOL aL = FALSE;
                char rel_reg[50];
                UINT rel_offset = ((IWineD3DVertexShaderImpl *)This)->rel_offset;
                if(This->baseShader.reg_maps.shader_version.major < 2) {
                    sprintf(rel_reg, "A0.x");
                } else {
                    shader_arb_get_src_param(ins, reg->rel_addr, 0, rel_reg);
                    if(ctx->target_version == ARB) {
                        if(strcmp(rel_reg, "**aL_emul**") == 0) {
                            aL = TRUE;
                        } else {
                            shader_arb_request_a0(ins, rel_reg);
                            sprintf(rel_reg, "A0.x");
                        }
                    }
                }
                if(aL)
                    sprintf(register_name, "C[%u]", ctx->aL + reg->idx);
                else if (reg->idx >= rel_offset)
                    sprintf(register_name, "C[%s + %u]", rel_reg, reg->idx - rel_offset);
                else
                    sprintf(register_name, "C[%s - %u]", rel_reg, -reg->idx + rel_offset);
            }
            else
            {
                if (This->baseShader.reg_maps.usesrelconstF)
                    sprintf(register_name, "C[%u]", reg->idx);
                else
                    sprintf(register_name, "C%u", reg->idx);
            }
            break;

        case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
            if (pshader) {
                if(This->baseShader.reg_maps.shader_version.major == 1 &&
                   This->baseShader.reg_maps.shader_version.minor <= 3) {
                    /* In ps <= 1.3, Tx is a temporary register as destination to all instructions,
                     * and as source to most instructions. For some instructions it is the texcoord
                     * input. Those instructions know about the special use
                     */
                    sprintf(register_name, "T%u", reg->idx);
                } else {
                    /* in ps 1.4 and 2.x Tx is always a (read-only) varying */
                    sprintf(register_name, "fragment.texcoord[%u]", reg->idx);
                }
            }
            else
            {
                if(This->baseShader.reg_maps.shader_version.major == 1 || ctx->target_version >= NV2)
                {
                    sprintf(register_name, "A%u", reg->idx);
                }
                else
                {
                    sprintf(register_name, "A%u_SHADOW", reg->idx);
                }
            }
            break;

        case WINED3DSPR_COLOROUT:
            if(ctx->cur_ps_args->super.srgb_correction && reg->idx == 0)
            {
                strcpy(register_name, "TMP_COLOR");
            }
            else
            {
                if(ctx->cur_ps_args->super.srgb_correction) FIXME("sRGB correction on higher render targets\n");
                if(This->baseShader.reg_maps.highest_render_target > 0)
                {
                    sprintf(register_name, "result.color[%u]", reg->idx);
                }
                else
                {
                    strcpy(register_name, "result.color");
                }
            }
            break;

        case WINED3DSPR_RASTOUT:
            if(reg->idx == 1) sprintf(register_name, "%s", ctx->fog_output);
            else sprintf(register_name, "%s", rastout_reg_names[reg->idx]);
            break;

        case WINED3DSPR_DEPTHOUT:
            strcpy(register_name, "result.depth");
            break;

        case WINED3DSPR_ATTROUT:
        /* case WINED3DSPR_OUTPUT: */
            if (pshader) sprintf(register_name, "oD[%u]", reg->idx);
            else strcpy(register_name, ctx->color_output[reg->idx]);
            break;

        case WINED3DSPR_TEXCRDOUT:
            if (pshader)
            {
                sprintf(register_name, "oT[%u]", reg->idx);
            }
            else
            {
                if(This->baseShader.reg_maps.shader_version.major < 3)
                {
                    strcpy(register_name, ctx->texcrd_output[reg->idx]);
                }
                else
                {
                    strcpy(register_name, ctx->vs_output[reg->idx]);
                }
            }
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
            sprintf(register_name, "I%u", reg->idx);
            break;

        case WINED3DSPR_MISCTYPE:
            if(reg->idx == 0)
            {
                sprintf(register_name, "vpos");
            }
            else if(reg->idx == 1)
            {
                sprintf(register_name, "fragment.facing.x");
            }
            else
            {
                FIXME("Unknown MISCTYPE register index %u\n", reg->idx);
            }
            break;

        default:
            FIXME("Unhandled register type %#x[%u]\n", reg->type, reg->idx);
            sprintf(register_name, "unrecognized_register[%u]", reg->idx);
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

static void gen_color_correction(struct wined3d_shader_buffer *buffer, const char *reg,
        DWORD dst_mask, const char *one, const char *two, struct color_fixup_desc fixup)
{
    DWORD mask;

    if (is_complex_fixup(fixup))
    {
        enum complex_fixup complex_fixup = get_complex_fixup(fixup);
        FIXME("Complex fixup (%#x) not supported\n", complex_fixup);
        return;
    }

    mask = 0;
    if (fixup.x_source != CHANNEL_SOURCE_X) mask |= WINED3DSP_WRITEMASK_0;
    if (fixup.y_source != CHANNEL_SOURCE_Y) mask |= WINED3DSP_WRITEMASK_1;
    if (fixup.z_source != CHANNEL_SOURCE_Z) mask |= WINED3DSP_WRITEMASK_2;
    if (fixup.w_source != CHANNEL_SOURCE_W) mask |= WINED3DSP_WRITEMASK_3;
    mask &= dst_mask;

    if (mask)
    {
        shader_addline(buffer, "SWZ %s, %s, %s, %s, %s, %s;\n", reg, reg,
                shader_arb_get_fixup_swizzle(fixup.x_source), shader_arb_get_fixup_swizzle(fixup.y_source),
                shader_arb_get_fixup_swizzle(fixup.z_source), shader_arb_get_fixup_swizzle(fixup.w_source));
    }

    mask = 0;
    if (fixup.x_sign_fixup) mask |= WINED3DSP_WRITEMASK_0;
    if (fixup.y_sign_fixup) mask |= WINED3DSP_WRITEMASK_1;
    if (fixup.z_sign_fixup) mask |= WINED3DSP_WRITEMASK_2;
    if (fixup.w_sign_fixup) mask |= WINED3DSP_WRITEMASK_3;
    mask &= dst_mask;

    if (mask)
    {
        char reg_mask[6];
        char *ptr = reg_mask;

        if (mask != WINED3DSP_WRITEMASK_ALL)
        {
            *ptr++ = '.';
            if (mask & WINED3DSP_WRITEMASK_0) *ptr++ = 'x';
            if (mask & WINED3DSP_WRITEMASK_1) *ptr++ = 'y';
            if (mask & WINED3DSP_WRITEMASK_2) *ptr++ = 'z';
            if (mask & WINED3DSP_WRITEMASK_3) *ptr++ = 'w';
        }
        *ptr = '\0';

        shader_addline(buffer, "MAD %s%s, %s, %s, -%s;\n", reg, reg_mask, reg, two, one);
    }
}

static const char *shader_arb_get_modifier(const struct wined3d_shader_instruction *ins)
{
    DWORD mod;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    if (!ins->dst_count) return "";

    mod = ins->dst[0].modifiers;

    /* Silently ignore PARTIALPRECISION if its not supported */
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    DWORD sampler_type = ins->ctx->reg_maps->sampler_type[sampler_idx];
    const char *tex_type;
    BOOL np2_fixup = FALSE;
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    const char *mod;
    BOOL pshader = shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type);

    /* D3D vertex shader sampler IDs are vertex samplers(0-3), not global d3d samplers */
    if(!pshader) sampler_idx += MAX_FRAGMENT_SAMPLERS;

    switch(sampler_type) {
        case WINED3DSTT_1D:
            tex_type = "1D";
            break;

        case WINED3DSTT_2D:
            if(device->stateBlock->textures[sampler_idx] &&
               IWineD3DBaseTexture_GetTextureDimensions(device->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
                tex_type = "RECT";
            } else {
                tex_type = "2D";
            }
            if (shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type))
            {
                if (priv->cur_np2fixup_info->super.active & (1 << sampler_idx))
                {
                    if (flags) FIXME("Only ordinary sampling from NP2 textures is supported.\n");
                    else np2_fixup = TRUE;
                }
            }
            break;

        case WINED3DSTT_VOLUME:
            tex_type = "3D";
            break;

        case WINED3DSTT_CUBE:
            tex_type = "CUBE";
            break;

        default:
            ERR("Unexpected texture type %d\n", sampler_type);
            tex_type = "";
    }

    /* TEX, TXL, TXD and TXP do not support the "H" modifier,
     * so don't use shader_arb_get_modifier
     */
    if(ins->dst[0].modifiers & WINED3DSPDM_SATURATE) mod = "_SAT";
    else mod = "";

    /* Fragment samplers always have indentity mapping */
    if(sampler_idx >= MAX_FRAGMENT_SAMPLERS)
    {
        sampler_idx = priv->cur_vs_args->vertex.samplers[sampler_idx - MAX_FRAGMENT_SAMPLERS];
    }

    if (flags & TEX_DERIV)
    {
        if(flags & TEX_PROJ) FIXME("Projected texture sampling with custom derivatives\n");
        if(flags & TEX_BIAS) FIXME("Biased texture sampling with custom derivatives\n");
        shader_addline(buffer, "TXD%s %s, %s, %s, %s, texture[%u], %s;\n", mod, dst_str, coord_reg,
                       dsx, dsy,sampler_idx, tex_type);
    }
    else if(flags & TEX_LOD)
    {
        if(flags & TEX_PROJ) FIXME("Projected texture sampling with explicit lod\n");
        if(flags & TEX_BIAS) FIXME("Biased texture sampling with explicit lod\n");
        shader_addline(buffer, "TXL%s %s, %s, texture[%u], %s;\n", mod, dst_str, coord_reg,
                       sampler_idx, tex_type);
    }
    else if (flags & TEX_BIAS)
    {
        /* Shouldn't be possible, but let's check for it */
        if(flags & TEX_PROJ) FIXME("Biased and Projected texture sampling\n");
        /* TXB takes the 4th component of the source vector automatically, as d3d. Nothing more to do */
        shader_addline(buffer, "TXB%s %s, %s, texture[%u], %s;\n", mod, dst_str, coord_reg, sampler_idx, tex_type);
    }
    else if (flags & TEX_PROJ)
    {
        shader_addline(buffer, "TXP%s %s, %s, texture[%u], %s;\n", mod, dst_str, coord_reg, sampler_idx, tex_type);
    }
    else
    {
        if (np2_fixup)
        {
            const unsigned char idx = priv->cur_np2fixup_info->super.idx[sampler_idx];
            shader_addline(buffer, "MUL TA, np2fixup[%u].%s, %s;\n", idx >> 1,
                           (idx % 2) ? "zwxy" : "xyzw", coord_reg);

            shader_addline(buffer, "TEX%s %s, TA, texture[%u], %s;\n", mod, dst_str, sampler_idx, tex_type);
        }
        else
            shader_addline(buffer, "TEX%s %s, %s, texture[%u], %s;\n", mod, dst_str, coord_reg, sampler_idx, tex_type);
    }

    if (pshader)
    {
        gen_color_correction(buffer, dst_str, ins->dst[0].write_mask,
                "one", "coefmul.x", priv->cur_ps_args->super.color_fixup[sampler_idx]);
    }
}

static void shader_arb_get_src_param(const struct wined3d_shader_instruction *ins,
        const struct wined3d_shader_src_param *src, unsigned int tmpreg, char *outregstr)
{
    /* Generate a line that does the input modifier computation and return the input register to use */
    BOOL is_color = FALSE;
    char regstr[256];
    char swzstr[20];
    int insert_line;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;

    /* Assume a new line will be added */
    insert_line = 1;

    /* Get register name */
    shader_arb_get_register_name(ins, &src->reg, regstr, &is_color);
    shader_arb_get_swizzle(src, is_color, swzstr);

    switch (src->modifiers)
    {
    case WINED3DSPSM_NONE:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case WINED3DSPSM_NEG:
        sprintf(outregstr, "-%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case WINED3DSPSM_BIAS:
        shader_addline(buffer, "ADD T%c, %s, -coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_BIASNEG:
        shader_addline(buffer, "ADD T%c, -%s, coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_SIGN:
        shader_addline(buffer, "MAD T%c, %s, coefmul.x, -one.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_SIGNNEG:
        shader_addline(buffer, "MAD T%c, %s, -coefmul.x, one.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_COMP:
        shader_addline(buffer, "SUB T%c, one.x, %s;\n", 'A' + tmpreg, regstr);
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
            insert_line = 0;
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
        insert_line = 0;
        break;
    default:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = 0;
    }

    /* Return modified or original register, with swizzle */
    if (insert_line)
        sprintf(outregstr, "T%c%s", 'A' + tmpreg, swzstr);
}

static void pshader_hw_bem(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[2][50];
    DWORD sampler_code = dst->reg.idx;

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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[3][50];
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    BOOL is_color;

    shader_arb_get_dst_param(ins, dst, dst_name);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);

    /* The coissue flag changes the semantic of the cnd instruction in <= 1.3 shaders */
    if (shader_version <= WINED3D_SHADER_VERSION(1, 3) && ins->coissue)
    {
        shader_addline(buffer, "MOV%s %s, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name[1]);
    } else {
        struct wined3d_shader_src_param src0_copy = ins->src[0];
        char extra_neg;

        /* src0 may have a negate srcmod set, so we can't blindly add "-" to the name */
        src0_copy.modifiers = negate_modifiers(src0_copy.modifiers, &extra_neg);

        shader_arb_get_src_param(ins, &src0_copy, 0, src_name[0]);
        shader_arb_get_src_param(ins, &ins->src[2], 2, src_name[2]);
        shader_addline(buffer, "ADD TA, %c%s, coefdiv.x;\n", extra_neg, src_name[0]);
        /* No modifiers supported on CMP */
        shader_addline(buffer, "CMP %s, TA, %s, %s;\n", dst_name, src_name[1], src_name[2]);

        /* _SAT on CMP doesn't make much sense, but it is not a pure NOP */
        if(ins->dst[0].modifiers & WINED3DSPDM_SATURATE)
        {
            shader_arb_get_register_name(ins, &dst->reg, src_name[0], &is_color);
            shader_addline(buffer, "MOV_SAT %s, %s;\n", dst_name, dst_name);
        }
    }
}

static void pshader_hw_cmp(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[3][50];
    BOOL is_color;

    shader_arb_get_dst_param(ins, dst, dst_name);

    /* Generate input register names (with modifiers) */
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_name[0]);
    shader_arb_get_src_param(ins, &ins->src[1], 1, src_name[1]);
    shader_arb_get_src_param(ins, &ins->src[2], 2, src_name[2]);

    /* No modifiers are supported on CMP */
    shader_addline(buffer, "CMP %s, %s, %s, %s;\n", dst_name,
                   src_name[0], src_name[2], src_name[1]);

    if(ins->dst[0].modifiers & WINED3DSPDM_SATURATE)
    {
        shader_arb_get_register_name(ins, &dst->reg, src_name[0], &is_color);
        shader_addline(buffer, "MOV_SAT %s, %s;\n", dst_name, src_name[0]);
    }
}

/** Process the WINED3DSIO_DP2ADD instruction in ARB.
 * dst = dot2(src0, src1) + src2 */
static void pshader_hw_dp2add(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
            FIXME("Unhandled opcode %#x\n", ins->handler_idx);
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

static void shader_hw_nop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    shader_addline(buffer, "NOP;\n");
}

static void shader_hw_mov(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    BOOL pshader = shader_is_pshader_version(shader->baseShader.reg_maps.shader_version.type);
    struct shader_arb_ctx_priv *ctx = ins->ctx->backend_data;

    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char src0_param[256];

    if(ins->handler_idx == WINED3DSIH_MOVA) {
        char write_mask[6];

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
        shader_addline(buffer, "SGE A0_SHADOW%s, %s, mova_const.y;\n", write_mask, src0_param);
        shader_addline(buffer, "MAD A0_SHADOW%s, A0_SHADOW, mova_const.z, -mova_const.w;\n", write_mask);

        shader_addline(buffer, "ABS TA%s, %s;\n", write_mask, src0_param);
        shader_addline(buffer, "ADD TA%s, TA, mova_const.x;\n", write_mask);
        shader_addline(buffer, "FLR TA%s, TA;\n", write_mask);
        if (((IWineD3DVertexShaderImpl *)shader)->rel_offset)
        {
            shader_addline(buffer, "ADD TA%s, TA, helper_const.z;\n", write_mask);
        }
        shader_addline(buffer, "MUL A0_SHADOW%s, TA, A0_SHADOW;\n", write_mask);

        ((struct shader_arb_ctx_priv *)ins->ctx->backend_data)->addr_reg[0] = '\0';
    } else if (ins->ctx->reg_maps->shader_version.major == 1
          && !shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type)
          && ins->dst[0].reg.type == WINED3DSPR_ADDR)
    {
        src0_param[0] = '\0';
        if (((IWineD3DVertexShaderImpl *)shader)->rel_offset)
        {
            shader_arb_get_src_param(ins, &ins->src[0], 0, src0_param);
            shader_addline(buffer, "ADD TA.x, %s, helper_const.z;\n", src0_param);
            shader_addline(buffer, "ARL A0.x, TA.x;\n");
        }
        else
        {
            /* Apple's ARB_vertex_program implementation does not accept an ARL source argument
             * with more than one component. Thus replicate the first source argument over all
             * 4 components. For example, .xyzw -> .x (or better: .xxxx), .zwxy -> .z, etc) */
            struct wined3d_shader_src_param tmp_src = ins->src[0];
            tmp_src.swizzle = (tmp_src.swizzle & 0x3) * 0x55;
            shader_arb_get_src_param(ins, &tmp_src, 0, src0_param);
            shader_addline(buffer, "ARL A0.x, %s;\n", src0_param);
        }
    }
    else if(ins->dst[0].reg.type == WINED3DSPR_COLOROUT && ins->dst[0].reg.idx == 0 && pshader)
    {
        IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) shader;
        if(ctx->cur_ps_args->super.srgb_correction && ps->color0_mov)
        {
            shader_addline(buffer, "#mov handled in srgb write code\n");
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char reg_dest[40];

    /* No swizzles are allowed in d3d's texkill. PS 1.x ignores the 4th component as documented,
     * but >= 2.0 honors it(undocumented, but tested by the d3d9 testsuit)
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
    } else {
        /* ARB fp doesn't like swizzles on the parameter of the KIL instruction. To mask the 4th component,
         * copy the register into our general purpose TMP variable, overwrite .w and pass TMP to KIL
         *
         * ps_1_3 shaders use the texcoord incarnation of the Tx register. ps_1_4 shaders can use the same,
         * or pass in any temporary register(in shader phase 2)
         */
        if(ins->ctx->reg_maps->shader_version.minor <= 3) {
            sprintf(reg_dest, "fragment.texcoord[%u]", dst->reg.idx);
        } else {
            shader_arb_get_dst_param(ins, dst, reg_dest);
        }
        shader_addline(buffer, "SWZ TA, %s, x, y, z, 1;\n", reg_dest);
        shader_addline(buffer, "KIL TA;\n");
    }
}

static void pshader_hw_tex(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    struct wined3d_shader_src_param src;

    char reg_dest[40];
    char reg_coord[40];
    DWORD reg_sampler_code;
    DWORD myflags = 0;

    /* All versions have a destination register */
    shader_arb_get_dst_param(ins, dst, reg_dest);

    /* 1.0-1.4: Use destination register number as texture code.
       2.0+: Use provided sampler number as texure code. */
    if (shader_version < WINED3D_SHADER_VERSION(2,0))
        reg_sampler_code = dst->reg.idx;
    else
        reg_sampler_code = ins->src[1].reg.idx;

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
     * 1.1, 1.2, 1.3: Use WINED3DTSS_TEXTURETRANSFORMFLAGS
     * 1.4: Use WINED3DSPSM_DZ or WINED3DSPSM_DW on src[0]
     * 2.0+: Use WINED3DSI_TEXLD_PROJECT on the opcode
     */
    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        DWORD flags = 0;
        if(reg_sampler_code < MAX_TEXTURES) {
            flags = deviceImpl->stateBlock->textureState[reg_sampler_code][WINED3DTSS_TEXTURETRANSFORMFLAGS];
        }
        if (flags & WINED3DTTFF_PROJECTED) {
            myflags |= TEX_PROJ;
        }
    }
    else if (shader_version < WINED3D_SHADER_VERSION(2,0))
    {
        DWORD src_mod = ins->src[0].modifiers;
        if (src_mod == WINED3DSPSM_DZ) {
            /* TXP cannot handle DZ natively, so move the z coordinate to .w. reg_coord is a read-only
             * varying register, so we need a temp reg
             */
            shader_addline(ins->ctx->buffer, "SWZ TA, %s, x, y, z, z;\n", reg_coord);
            strcpy(reg_coord, "TA");
            myflags |= TEX_PROJ;
        } else if(src_mod == WINED3DSPSM_DW) {
            myflags |= TEX_PROJ;
        }
    } else {
        if (ins->flags & WINED3DSI_TEXLD_PROJECT) myflags |= TEX_PROJ;
        if (ins->flags & WINED3DSI_TEXLD_BIAS) myflags |= TEX_BIAS;
    }
    shader_hw_sample(ins, reg_sampler_code, reg_dest, reg_coord, myflags, NULL, NULL);
}

static void pshader_hw_texcoord(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    DWORD shader_version = WINED3D_SHADER_VERSION(ins->ctx->reg_maps->shader_version.major,
            ins->ctx->reg_maps->shader_version.minor);
    char dst_str[50];

    if (shader_version < WINED3D_SHADER_VERSION(1,4))
    {
        DWORD reg = dst->reg.idx;

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
     struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
     IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
     IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
     DWORD flags;

     DWORD reg1 = ins->dst[0].reg.idx;
     char dst_str[50];
     char src_str[50];

     /* Note that texreg2ar treats Tx as a temporary register, not as a varying */
     shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
     shader_arb_get_src_param(ins, &ins->src[0], 0, src_str);
     /* Move .x first in case src_str is "TA" */
     shader_addline(buffer, "MOV TA.y, %s.x;\n", src_str);
     shader_addline(buffer, "MOV TA.x, %s.w;\n", src_str);
     flags = reg1 < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg1][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
     shader_hw_sample(ins, reg1, dst_str, "TA", flags & WINED3DTTFF_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
}

static void pshader_hw_texreg2gb(const struct wined3d_shader_instruction *ins)
{
     struct wined3d_shader_buffer *buffer = ins->ctx->buffer;

     DWORD reg1 = ins->dst[0].reg.idx;
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
    DWORD reg1 = ins->dst[0].reg.idx;
    char dst_str[50];
    char src_str[50];

    /* Note that texreg2rg treats Tx as a temporary register, not as a varying */
    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src_str);
    shader_hw_sample(ins, reg1, dst_str, src_str, 0, NULL, NULL);
}

static void pshader_hw_texbem(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)shader->baseShader.device;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char reg_coord[40], dst_reg[50], src_reg[50];
    DWORD reg_dest_code;

    /* All versions have a destination register. The Tx where the texture coordinates come
     * from is the varying incarnation of the texture register
     */
    reg_dest_code = dst->reg.idx;
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
    if (device->stateBlock->textureState[reg_dest_code][WINED3DTSS_TEXTURETRANSFORMFLAGS] & WINED3DTTFF_PROJECTED)
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
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char src0_name[50], dst_name[50];
    BOOL is_color;
    struct wined3d_shader_register tmp_reg = ins->dst[0].reg;

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    /* The next instruction will be a texm3x2tex or texm3x2depth that writes to the uninitialized
     * T<reg+1> register. Use this register to store the calculated vector
     */
    tmp_reg.idx = reg + 1;
    shader_arb_get_register_name(ins, &tmp_reg, dst_name, &is_color);
    shader_addline(buffer, "DP3 %s.x, fragment.texcoord[%u], %s;\n", dst_name, reg, src0_name);
}

static void pshader_hw_texm3x2tex(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_str[50];
    char src0_name[50];
    char dst_reg[50];
    BOOL is_color;

    /* We know that we're writing to the uninitialized T<reg> register, so use it for temporary storage */
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_reg, &is_color);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 %s.y, fragment.texcoord[%u], %s;\n", dst_reg, reg, src0_name);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(ins, reg, dst_str, dst_reg, flags & WINED3DTTFF_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
}

static void pshader_hw_texm3x3pad(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    SHADER_PARSE_STATE *current_state = &shader->baseShader.parse_state;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char src0_name[50], dst_name[50];
    struct wined3d_shader_register tmp_reg = ins->dst[0].reg;
    BOOL is_color;

    /* There are always 2 texm3x3pad instructions followed by one texm3x3[tex,vspec, ...] instruction, with
     * incrementing ins->dst[0].register_idx numbers. So the pad instruction already knows the final destination
     * register, and this register is uninitialized(otherwise the assembler complains that it is 'redeclared')
     */
    tmp_reg.idx = reg + 2 - current_state->current_row;
    shader_arb_get_register_name(ins, &tmp_reg, dst_name, &is_color);

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 %s.%c, fragment.texcoord[%u], %s;\n",
                   dst_name, 'x' + current_state->current_row, reg, src0_name);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

static void pshader_hw_texm3x3tex(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
    SHADER_PARSE_STATE *current_state = &shader->baseShader.parse_state;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_str[50];
    char src0_name[50], dst_name[50];
    BOOL is_color;

    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_name, &is_color);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 %s.z, fragment.texcoord[%u], %s;\n", dst_name, reg, src0_name);

    /* Sample the texture using the calculated coordinates */
    shader_arb_get_dst_param(ins, &ins->dst[0], dst_str);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(ins, reg, dst_str, dst_name, flags & WINED3DTTFF_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
    current_state->current_row = 0;
}

static void pshader_hw_texm3x3vspec(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
    SHADER_PARSE_STATE *current_state = &shader->baseShader.parse_state;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    shader_addline(buffer, "MOV TB.x, fragment.texcoord[%u].w;\n", current_state->texcoord_w[0]);
    shader_addline(buffer, "MOV TB.y, fragment.texcoord[%u].w;\n", current_state->texcoord_w[1]);
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
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(ins, reg, dst_str, dst_reg, flags & WINED3DTTFF_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
    current_state->current_row = 0;
}

static void pshader_hw_texm3x3spec(const struct wined3d_shader_instruction *ins)
{
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *)shader->baseShader.device;
    SHADER_PARSE_STATE *current_state = &shader->baseShader.parse_state;
    DWORD flags;
    DWORD reg = ins->dst[0].reg.idx;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(ins, reg, dst_str, dst_reg, flags & WINED3DTTFF_PROJECTED ? TEX_PROJ : 0, NULL, NULL);
    current_state->current_row = 0;
}

static void pshader_hw_texdepth(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];

    /* texdepth has an implicit destination, the fragment depth value. It's only parameter,
     * which is essentially an input, is the destination register because it is the first
     * parameter. According to the msdn, this must be register r5, but let's keep it more flexible
     * here(writemasks/swizzles are not valid on texdepth)
     */
    shader_arb_get_dst_param(ins, dst, dst_name);

    /* According to the msdn, the source register(must be r5) is unusable after
     * the texdepth instruction, so we're free to modify it
     */
    shader_addline(buffer, "MIN %s.y, %s.y, one.y;\n", dst_name, dst_name);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP %s.y, %s.y;\n", dst_name, dst_name);
    shader_addline(buffer, "MUL TA.x, %s.x, %s.y;\n", dst_name, dst_name);
    shader_addline(buffer, "MIN TA.x, TA.x, one.x;\n");
    shader_addline(buffer, "MAX result.depth, TA.x, 0.0;\n");
}

/** Process the WINED3DSIO_TEXDP3TEX instruction in ARB:
 * Take a 3-component dot product of the TexCoord[dstreg] and src,
 * then perform a 1D texture lookup from stage dstregnum, place into dst. */
static void pshader_hw_texdp3tex(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    DWORD sampler_idx = ins->dst[0].reg.idx;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;

    /* Handle output register */
    shader_arb_get_dst_param(ins, dst, dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0);
    shader_addline(buffer, "DP3 %s, fragment.texcoord[%u], %s;\n", dst_str, dst->reg.idx, src0);
}

/** Process the WINED3DSIO_TEXM3X3 instruction in ARB
 * Perform the 3rd row of a 3x3 matrix multiply */
static void pshader_hw_texm3x3(const struct wined3d_shader_instruction *ins)
{
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_str[50], dst_name[50];
    char src0[50];
    BOOL is_color;

    shader_arb_get_dst_param(ins, dst, dst_str);
    shader_arb_get_src_param(ins, &ins->src[0], 0, src0);
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_name, &is_color);
    shader_addline(buffer, "DP3 %s.z, fragment.texcoord[%u], %s;\n", dst_name, dst->reg.idx, src0);
    shader_addline(buffer, "MOV %s, %s;\n", dst_str, dst_name);
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in ARB:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
static void pshader_hw_texm3x2depth(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    const struct wined3d_shader_dst_param *dst = &ins->dst[0];
    char src0[50], dst_name[50];
    BOOL is_color;

    shader_arb_get_src_param(ins, &ins->src[0], 0, src0);
    shader_arb_get_register_name(ins, &ins->dst[0].reg, dst_name, &is_color);
    shader_addline(buffer, "DP3 %s.y, fragment.texcoord[%u], %s;\n", dst_name, dst->reg.idx, src0);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP %s.y, %s.y;\n", dst_name, dst_name);
    shader_addline(buffer, "MUL %s.x, %s.x, %s.y;\n", dst_name, dst_name, dst_name);
    shader_addline(buffer, "MIN %s.x, %s.x, one.x;\n", dst_name, dst_name);
    shader_addline(buffer, "MAX result.depth, %s.x, 0.0;\n", dst_name);
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
            FIXME("Unhandled opcode %#x\n", ins->handler_idx);
            break;
    }

    tmp_dst = ins->dst[0];
    tmp_src[0] = ins->src[0];
    tmp_src[1] = ins->src[1];
    for (i = 0; i < nComponents; i++) {
        tmp_dst.write_mask = WINED3DSP_WRITEMASK_0 << i;
        shader_hw_map2gl(&tmp_ins);
        ++tmp_src[1].reg.idx;
    }
}

static void shader_hw_scalar_op(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    const char *instruction;

    char dst[50];
    char src[50];

    switch(ins->handler_idx)
    {
        case WINED3DSIH_RSQ:  instruction = "RSQ"; break;
        case WINED3DSIH_RCP:  instruction = "RCP"; break;
        case WINED3DSIH_EXP:  instruction = "EX2"; break;
        case WINED3DSIH_EXPP: instruction = "EXP"; break;
        default: instruction = "";
            FIXME("Unhandled opcode %#x\n", ins->handler_idx);
            break;
    }

    shader_arb_get_dst_param(ins, &ins->dst[0], dst); /* Destination */
    shader_arb_get_src_param(ins, &ins->src[0], 0, src);
    if (ins->src[0].swizzle == WINED3DSP_NOSWIZZLE)
    {
        /* Dx sdk says .x is used if no swizzle is given, but our test shows that
         * .w is used
         */
        strcat(src, ".w");
    }

    shader_addline(buffer, "%s%s %s, %s;\n", instruction, shader_arb_get_modifier(ins), dst, src);
}

static void shader_hw_nrm(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char dst_name[50];
    char src_name[50];
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    BOOL pshader = shader_is_pshader_version(ins->ctx->reg_maps->shader_version.type);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst_name);
    shader_arb_get_src_param(ins, &ins->src[0], 1 /* Use TB */, src_name);

    if(pshader && priv->target_version >= NV3)
    {
        shader_addline(buffer, "NRM%s %s, %s;\n", shader_arb_get_modifier(ins), dst_name, src_name);
    }
    else
    {
        shader_addline(buffer, "DP3 TA, %s, %s;\n", src_name, src_name);
        shader_addline(buffer, "RSQ TA, TA.x;\n");
        /* dst.w = src[0].w * 1 / (src.x^2 + src.y^2 + src.z^2)^(1/2) according to msdn*/
        shader_addline(buffer, "MUL%s %s, %s, TA;\n", shader_arb_get_modifier(ins), dst_name,
                    src_name);
    }
}

static void shader_hw_lrp(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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

static void shader_hw_log_pow(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    char src0[50], src1[50], dst[50];
    struct wined3d_shader_src_param src0_copy = ins->src[0];
    BOOL need_abs = FALSE;
    const char *instr;
    BOOL arg2 = FALSE;

    switch(ins->handler_idx)
    {
        case WINED3DSIH_LOG:  instr = "LG2"; break;
        case WINED3DSIH_LOGP: instr = "LOG"; break;
        case WINED3DSIH_POW:  instr = "POW"; arg2 = TRUE; break;
        default:
            ERR("Unexpected instruction %d\n", ins->handler_idx);
            return;
    }

    /* LOG, LOGP and POW operate on the absolute value of the input */
    src0_copy.modifiers = abs_modifier(src0_copy.modifiers, &need_abs);

    shader_arb_get_dst_param(ins, &ins->dst[0], dst);
    shader_arb_get_src_param(ins, &src0_copy, 0, src0);
    if(arg2) shader_arb_get_src_param(ins, &ins->src[1], 1, src1);

    if(need_abs)
    {
        shader_addline(buffer, "ABS TA, %s;\n", src0);
        if(arg2)
        {
            shader_addline(buffer, "%s%s %s, TA, %s;\n", instr, shader_arb_get_modifier(ins), dst, src1);
        }
        else
        {
            shader_addline(buffer, "%s%s %s, TA;\n", instr, shader_arb_get_modifier(ins), dst);
        }
    }
    else if(arg2)
    {
        shader_addline(buffer, "%s%s %s, %s, %s;\n", instr, shader_arb_get_modifier(ins), dst, src0, src1);
    }
    else
    {
        shader_addline(buffer, "%s%s %s, %s;\n", instr, shader_arb_get_modifier(ins), dst, src0);
    }
}

static void shader_hw_loop(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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

static const char *get_compare(COMPARISON_TYPE flags)
{
    switch (flags)
    {
        case COMPARISON_GT: return "GT";
        case COMPARISON_EQ: return "EQ";
        case COMPARISON_GE: return "GE";
        case COMPARISON_LT: return "LT";
        case COMPARISON_NE: return "NE";
        case COMPARISON_LE: return "LE";
        default:
            FIXME("Unrecognized comparison value: %u\n", flags);
            return "(\?\?)";
    }
}

static COMPARISON_TYPE invert_compare(COMPARISON_TYPE flags)
{
    switch (flags)
    {
        case COMPARISON_GT: return COMPARISON_LE;
        case COMPARISON_EQ: return COMPARISON_NE;
        case COMPARISON_GE: return COMPARISON_LT;
        case COMPARISON_LT: return COMPARISON_GE;
        case COMPARISON_NE: return COMPARISON_EQ;
        case COMPARISON_LE: return COMPARISON_GT;
        default:
            FIXME("Unrecognized comparison value: %u\n", flags);
            return -1;
    }
}

static void shader_hw_breakc(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
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
    DWORD sampler_idx = ins->src[1].reg.idx;
    char reg_dest[40];
    char reg_src[3][40];
    DWORD flags = TEX_DERIV;

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
    DWORD sampler_idx = ins->src[1].reg.idx;
    char reg_dest[40];
    char reg_coord[40];
    DWORD flags = TEX_LOD;

    shader_arb_get_dst_param(ins, &ins->dst[0], reg_dest);
    shader_arb_get_src_param(ins, &ins->src[0], 0, reg_coord);

    if (ins->flags & WINED3DSI_TEXLD_PROJECT) flags |= TEX_PROJ;
    if (ins->flags & WINED3DSI_TEXLD_BIAS) flags |= TEX_BIAS;

    shader_hw_sample(ins, sampler_idx, reg_dest, reg_coord, flags, NULL, NULL);
}

static void shader_hw_label(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;

    priv->in_main_func = FALSE;
    /* Call instructions activate the NV extensions, not labels and rets. If there is an uncalled
     * subroutine, don't generate a label that will make GL complain
     */
    if(priv->target_version == ARB) return;

    shader_addline(buffer, "l%u:\n", ins->src[0].reg.idx);
}

static void vshader_add_footer(IWineD3DVertexShaderImpl *This, struct wined3d_shader_buffer *buffer,
        const struct arb_vs_compile_args *args, struct shader_arb_ctx_priv *priv_ctx)
{
    const shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)This->baseShader.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i;

    /* The D3DRS_FOGTABLEMODE render state defines if the shader-generated fog coord is used
     * or if the fragment depth is used. If the fragment depth is used(FOGTABLEMODE != NONE),
     * the fog frag coord is thrown away. If the fog frag coord is used, but not written by
     * the shader, it is set to 0.0(fully fogged, since start = 1.0, end = 0.0)
     */
    if(args->super.fog_src == VS_FOG_Z) {
        shader_addline(buffer, "MOV result.fogcoord, TMP_OUT.z;\n");
    } else if (!reg_maps->fog) {
        /* posFixup.x is always 1.0, so we can savely use it */
        shader_addline(buffer, "ADD result.fogcoord, posFixup.x, -posFixup.x;\n");
    }

    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while d3d coords specify
     * the corner. The offsets are stored in z and w in posFixup. posFixup.y contains
     * 1.0 or -1.0 to turn the rendering upside down for offscreen rendering. PosFixup.x
     * contains 1.0 to allow a mad, but arb vs swizzles are too restricted for that.
     */
    shader_addline(buffer, "MUL TA, posFixup, TMP_OUT.w;\n");
    shader_addline(buffer, "ADD TMP_OUT.x, TMP_OUT.x, TA.z;\n");
    shader_addline(buffer, "MAD TMP_OUT.y, TMP_OUT.y, posFixup.y, TA.w;\n");

    if(use_nv_clip(gl_info) && priv_ctx->target_version >= NV2)
    {
        if(args->super.clip_enabled)
        {
            for(i = 0; i < priv_ctx->vs_clipplanes; i++)
            {
                shader_addline(buffer, "DP4 result.clip[%u].x, TMP_OUT, state.clip[%u].plane;\n", i, i);
            }
        }
    }
    else if(args->clip.boolclip.clip_texcoord)
    {
        unsigned int cur_clip = 0;
        char component[4] = {'x', 'y', 'z', 'w'};

        for (i = 0; i < gl_info->limits.clipplanes; ++i)
        {
            if(args->clip.boolclip.clipplane_mask & (1 << i))
            {
                shader_addline(buffer, "DP4 TA.%c, TMP_OUT, state.clip[%u].plane;\n",
                               component[cur_clip++], i);
            }
        }
        switch(cur_clip)
        {
            case 0:
                shader_addline(buffer, "MOV TA, -helper_const.w;\n");
                break;
            case 1:
                shader_addline(buffer, "MOV TA.yzw, -helper_const.w;\n");
                break;
            case 2:
                shader_addline(buffer, "MOV TA.zw, -helper_const.w;\n");
                break;
            case 3:
                shader_addline(buffer, "MOV TA.w, -helper_const.w;\n");
                break;
        }
        shader_addline(buffer, "MOV result.texcoord[%u], TA;\n",
                       args->clip.boolclip.clip_texcoord - 1);
    }

    /* Z coord [0;1]->[-1;1] mapping, see comment in transform_projection in state.c
     * and the glsl equivalent
     */
    if(need_helper_const(gl_info)) {
        shader_addline(buffer, "MAD TMP_OUT.z, TMP_OUT.z, helper_const.x, -TMP_OUT.w;\n");
    } else {
        shader_addline(buffer, "ADD TMP_OUT.z, TMP_OUT.z, TMP_OUT.z;\n");
        shader_addline(buffer, "ADD TMP_OUT.z, TMP_OUT.z, -TMP_OUT.w;\n");
    }

    shader_addline(buffer, "MOV result.position, TMP_OUT;\n");

    priv_ctx->footer_written = TRUE;
}

static void shader_hw_ret(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *) ins->ctx->shader;
    BOOL vshader = shader_is_vshader_version(ins->ctx->reg_maps->shader_version.type);

    if(priv->target_version == ARB) return;

    if(vshader)
    {
        if(priv->in_main_func) vshader_add_footer((IWineD3DVertexShaderImpl *) shader, buffer, priv->cur_vs_args, priv);
    }

    shader_addline(buffer, "RET;\n");
}

static void shader_hw_call(const struct wined3d_shader_instruction *ins)
{
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    shader_addline(buffer, "CAL l%u;\n", ins->src[0].reg.idx);
}

/* GL locking is done by the caller */
static GLuint create_arb_blt_vertex_program(const struct wined3d_gl_info *gl_info)
{
    GLuint program_id = 0;
    GLint pos;

    const char *blt_vprogram =
        "!!ARBvp1.0\n"
        "PARAM c[1] = { { 1, 0.5 } };\n"
        "MOV result.position, vertex.position;\n"
        "MOV result.color, c[0].x;\n"
        "MOV result.texcoord[0], vertex.texcoord[0];\n"
        "END\n";

    GL_EXTCALL(glGenProgramsARB(1, &program_id));
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, program_id));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            strlen(blt_vprogram), blt_vprogram));
    checkGLcall("glProgramStringARB()");

    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if (pos != -1)
    {
        FIXME("Vertex program error at position %d: %s\n\n", pos,
            debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        shader_arb_dump_program_source(blt_vprogram);
    }
    else
    {
        GLint native;

        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native));
        checkGLcall("glGetProgramivARB()");
        if (!native) WARN("Program exceeds native resource limits.\n");
    }

    return program_id;
}

/* GL locking is done by the caller */
static GLuint create_arb_blt_fragment_program(const struct wined3d_gl_info *gl_info, enum tex_types tex_type)
{
    GLuint program_id = 0;
    GLint pos;

    static const char * const blt_fprograms[tex_type_count] =
    {
        /* tex_1d */
        NULL,
        /* tex_2d */
        "!!ARBfp1.0\n"
        "TEMP R0;\n"
        "TEX R0.x, fragment.texcoord[0], texture[0], 2D;\n"
        "MOV result.depth.z, R0.x;\n"
        "END\n",
        /* tex_3d */
        NULL,
        /* tex_cube */
        "!!ARBfp1.0\n"
        "TEMP R0;\n"
        "TEX R0.x, fragment.texcoord[0], texture[0], CUBE;\n"
        "MOV result.depth.z, R0.x;\n"
        "END\n",
        /* tex_rect */
        "!!ARBfp1.0\n"
        "TEMP R0;\n"
        "TEX R0.x, fragment.texcoord[0], texture[0], RECT;\n"
        "MOV result.depth.z, R0.x;\n"
        "END\n",
    };

    if (!blt_fprograms[tex_type])
    {
        FIXME("tex_type %#x not supported\n", tex_type);
        tex_type = tex_2d;
    }

    GL_EXTCALL(glGenProgramsARB(1, &program_id));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program_id));
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            strlen(blt_fprograms[tex_type]), blt_fprograms[tex_type]));
    checkGLcall("glProgramStringARB()");

    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if (pos != -1)
    {
        FIXME("Fragment program error at position %d: %s\n\n", pos,
            debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        shader_arb_dump_program_source(blt_fprograms[tex_type]);
    }
    else
    {
        GLint native;

        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native));
        checkGLcall("glGetProgramivARB()");
        if (!native) WARN("Program exceeds native resource limits.\n");
    }

    return program_id;
}

static void arbfp_add_sRGB_correction(struct wined3d_shader_buffer *buffer, const char *fragcolor,
        const char *tmp1, const char *tmp2, const char *tmp3, const char *tmp4, BOOL condcode)
{
    /* Perform sRGB write correction. See GLX_EXT_framebuffer_sRGB */

    if(condcode)
    {
        /* Sigh. MOVC CC doesn't work, so use one of the temps as dummy dest */
        shader_addline(buffer, "SUBC %s, %s.x, srgb_consts1.y;\n", tmp1, fragcolor);
        /* Calculate the > 0.0031308 case */
        shader_addline(buffer, "POW %s.x (GE), %s.x, srgb_consts1.z;\n", fragcolor, fragcolor);
        shader_addline(buffer, "POW %s.y (GE), %s.y, srgb_consts1.z;\n", fragcolor, fragcolor);
        shader_addline(buffer, "POW %s.z (GE), %s.z, srgb_consts1.z;\n", fragcolor, fragcolor);
        shader_addline(buffer, "MUL %s.xyz (GE), %s, srgb_consts1.w;\n", fragcolor, fragcolor);
        shader_addline(buffer, "SUB %s.xyz (GE), %s, srgb_consts2.x;\n", fragcolor, fragcolor);
        /* Calculate the < case */
        shader_addline(buffer, "MUL %s.xyz (LT), srgb_consts1.x, %s;\n", fragcolor, fragcolor);
    }
    else
    {
        /* Calculate the > 0.0031308 case */
        shader_addline(buffer, "POW %s.x, %s.x, srgb_consts1.z;\n", tmp1, fragcolor);
        shader_addline(buffer, "POW %s.y, %s.y, srgb_consts1.z;\n", tmp1, fragcolor);
        shader_addline(buffer, "POW %s.z, %s.z, srgb_consts1.z;\n", tmp1, fragcolor);
        shader_addline(buffer, "MUL %s, %s, srgb_consts1.w;\n", tmp1, tmp1);
        shader_addline(buffer, "SUB %s, %s, srgb_consts2.x;\n", tmp1, tmp1);
        /* Calculate the < case */
        shader_addline(buffer, "MUL %s, srgb_consts1.x, %s;\n", tmp2, fragcolor);
        /* Get 1.0 / 0.0 masks for > 0.0031308 and < 0.0031308 */
        shader_addline(buffer, "SLT %s, srgb_consts1.y, %s;\n", tmp3, fragcolor);
        shader_addline(buffer, "SGE %s, srgb_consts1.y, %s;\n", tmp4, fragcolor);
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

static const DWORD *find_loop_control_values(IWineD3DBaseShaderImpl *This, DWORD idx)
{
    const local_constant *constant;

    LIST_FOR_EACH_ENTRY(constant, &This->baseShader.constantsI, local_constant, entry)
    {
        if (constant->idx == idx)
        {
            return constant->value;
        }
    }
    return NULL;
}

static void init_ps_input(const IWineD3DPixelShaderImpl *This, const struct arb_ps_compile_args *args,
                          struct shader_arb_ctx_priv *priv)
{
    const char *texcoords[8] =
    {
        "fragment.texcoord[0]", "fragment.texcoord[1]", "fragment.texcoord[2]", "fragment.texcoord[3]",
        "fragment.texcoord[4]", "fragment.texcoord[5]", "fragment.texcoord[6]", "fragment.texcoord[7]"
    };
    unsigned int i;
    const struct wined3d_shader_signature_element *sig = This->baseShader.input_signature;
    const char *semantic_name;
    DWORD semantic_idx;

    switch(args->super.vp_mode)
    {
        case pretransformed:
        case fixedfunction:
            /* The pixelshader has to collect the varyings on its own. In any case properly load
             * color0 and color1. In the case of pretransformed vertices also load texcoords. Set
             * other attribs to 0.0.
             *
             * For fixedfunction this behavior is correct, according to the tests. For pretransformed
             * we'd either need a replacement shader that can load other attribs like BINORMAL, or
             * load the texcoord attrib pointers to match the pixel shader signature
             */
            for(i = 0; i < MAX_REG_INPUT; i++)
            {
                semantic_name = sig[i].semantic_name;
                semantic_idx = sig[i].semantic_idx;
                if(semantic_name == NULL) continue;

                if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_COLOR))
                {
                    if(semantic_idx == 0) priv->ps_input[i] = "fragment.color.primary";
                    else if(semantic_idx == 1) priv->ps_input[i] = "fragment.color.secondary";
                    else priv->ps_input[i] = "0.0";
                }
                else if(args->super.vp_mode == fixedfunction)
                {
                    priv->ps_input[i] = "0.0";
                }
                else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_TEXCOORD))
                {
                    if(semantic_idx < 8) priv->ps_input[i] = texcoords[semantic_idx];
                    else priv->ps_input[i] = "0.0";
                }
                else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_FOG))
                {
                    if(semantic_idx == 0) priv->ps_input[i] = "fragment.fogcoord";
                    else priv->ps_input[i] = "0.0";
                }
                else
                {
                    priv->ps_input[i] = "0.0";
                }

                TRACE("v%u, semantic %s%u is %s\n", i, semantic_name, semantic_idx, priv->ps_input[i]);
            }
            break;

        case vertexshader:
            /* That one is easy. The vertex shaders provide v0-v7 in fragment.texcoord and v8 and v9 in
             * fragment.color
             */
            for(i = 0; i < 8; i++)
            {
                priv->ps_input[i] = texcoords[i];
            }
            priv->ps_input[8] = "fragment.color.primary";
            priv->ps_input[9] = "fragment.color.secondary";
            break;
    }
}

/* GL locking is done by the caller */
static GLuint shader_arb_generate_pshader(IWineD3DPixelShaderImpl *This, struct wined3d_shader_buffer *buffer,
        const struct arb_ps_compile_args *args, struct arb_ps_compiled_shader *compiled)
{
    const shader_reg_maps* reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    const struct wined3d_gl_info *gl_info = &((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info;
    const local_constant *lconst;
    GLuint retval;
    char fragcolor[16];
    DWORD *lconst_map = local_const_mapping((IWineD3DBaseShaderImpl *) This), next_local, cur;
    struct shader_arb_ctx_priv priv_ctx;
    BOOL dcl_tmp = args->super.srgb_correction, dcl_td = FALSE;
    BOOL want_nv_prog = FALSE;
    struct arb_pshader_private *shader_priv = This->baseShader.backend_data;
    GLint errPos;
    DWORD map;

    char srgbtmp[4][4];
    unsigned int i, found = 0;

    for (i = 0, map = reg_maps->temporary; map; map >>= 1, ++i)
    {
        if (!(map & 1)
                || (This->color0_mov && i == This->color0_reg)
                || (reg_maps->shader_version.major < 2 && i == 0))
            continue;

        sprintf(srgbtmp[found], "R%u", i);
        ++found;
        if (found == 4) break;
    }

    switch(found) {
        case 4: dcl_tmp = FALSE; break;
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
    }

    /*  Create the hw ARB shader */
    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_ps_args = args;
    priv_ctx.compiled_fprog = compiled;
    priv_ctx.cur_np2fixup_info = &compiled->np2fixup_info;
    init_ps_input(This, args, &priv_ctx);
    list_init(&priv_ctx.control_frames);

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

    if(This->baseShader.reg_maps.highest_render_target > 0)
    {
        shader_addline(buffer, "OPTION ARB_draw_buffers;\n");
    }

    if (reg_maps->shader_version.major < 3)
    {
        switch(args->super.fog) {
            case FOG_OFF:
                break;
            case FOG_LINEAR:
                shader_addline(buffer, "OPTION ARB_fog_linear;\n");
                break;
            case FOG_EXP:
                shader_addline(buffer, "OPTION ARB_fog_exp;\n");
                break;
            case FOG_EXP2:
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
    shader_addline(buffer, "PARAM one = { 1.0, 1.0, 1.0, 1.0 };\n");

    if (reg_maps->shader_version.major < 2)
    {
        strcpy(fragcolor, "R0");
    } else {
        if(args->super.srgb_correction) {
            if(This->color0_mov) {
                sprintf(fragcolor, "R%u", This->color0_reg);
            } else {
                shader_addline(buffer, "TEMP TMP_COLOR;\n");
                strcpy(fragcolor, "TMP_COLOR");
            }
        } else {
            strcpy(fragcolor, "result.color");
        }
    }

    if(args->super.srgb_correction) {
        shader_addline(buffer, "PARAM srgb_consts1 = {%f, %f, %f, %f};\n",
                       srgb_mul_low, srgb_cmp, srgb_pow, srgb_mul_high);
        shader_addline(buffer, "PARAM srgb_consts2 = {%f, %f, %f, %f};\n",
                       srgb_sub_high, 0.0, 0.0, 0.0);
    }

    /* Base Declarations */
    next_local = shader_generate_arb_declarations((IWineD3DBaseShader *)This,
            reg_maps, buffer, gl_info, lconst_map, NULL, &priv_ctx);

    for (i = 0, map = reg_maps->bumpmat; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        cur = compiled->numbumpenvmatconsts;
        compiled->bumpenvmatconst[cur].const_num = WINED3D_CONST_NUM_UNUSED;
        compiled->bumpenvmatconst[cur].texunit = i;
        compiled->luminanceconst[cur].const_num = WINED3D_CONST_NUM_UNUSED;
        compiled->luminanceconst[cur].texunit = i;

        /* We can fit the constants into the constant limit for sure because texbem, texbeml, bem and beml are only supported
         * in 1.x shaders, and GL_ARB_fragment_program has a constant limit of 24 constants. So in the worst case we're loading
         * 8 shader constants, 8 bump matrices and 8 luminance parameters and are perfectly fine. (No NP2 fixup on bumpmapped
         * textures due to conditional NP2 restrictions)
         *
         * Use local constants to load the bump env parameters, not program.env. This avoids collisions with d3d constants of
         * shaders in newer shader models. Since the bump env parameters have to share their space with NP2 fixup constants,
         * their location is shader dependent anyway and they cannot be loaded globally.
         */
        compiled->bumpenvmatconst[cur].const_num = next_local++;
        shader_addline(buffer, "PARAM bumpenvmat%d = program.local[%d];\n",
                       i, compiled->bumpenvmatconst[cur].const_num);
        compiled->numbumpenvmatconsts = cur + 1;

        if (!(reg_maps->luminanceparams & (1 << i))) continue;

        compiled->luminanceconst[cur].const_num = next_local++;
        shader_addline(buffer, "PARAM luminance%d = program.local[%d];\n",
                       i, compiled->luminanceconst[cur].const_num);
    }

    for(i = 0; i < MAX_CONST_I; i++)
    {
        compiled->int_consts[i] = WINED3D_CONST_NUM_UNUSED;
        if (reg_maps->integer_constants & (1 << i) && priv_ctx.target_version >= NV2)
        {
            const DWORD *control_values = find_loop_control_values((IWineD3DBaseShaderImpl *) This, i);

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
     * at most 8 (MAX_FRAGMENT_SAMPLERS / 2) parameters, which is highly unlikely, since the application had to
     * use 16 NP2 textures at the same time. In case that we run out of constants the fixup is simply not
     * applied / activated. This will probably result in wrong rendering of the texture, but will save us from
     * shader compilation errors and the subsequent errors when drawing with this shader. */
    if (priv_ctx.cur_ps_args->super.np2_fixup) {

        struct arb_ps_np2fixup_info* const fixup = priv_ctx.cur_np2fixup_info;
        const WORD map = priv_ctx.cur_ps_args->super.np2_fixup;
        const UINT max_lconsts = gl_info->limits.arb_ps_local_constants;

        fixup->offset = next_local;
        fixup->super.active = 0;

        cur = 0;
        for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i) {
            if (!(map & (1 << i))) continue;

            if (fixup->offset + (cur >> 1) < max_lconsts) {
                fixup->super.active |= (1 << i);
                fixup->super.idx[i] = cur++;
            } else {
                FIXME("No free constant found to load NP2 fixup data into shader. "
                      "Sampling from this texture will probably look wrong.\n");
                break;
            }
        }

        fixup->super.num_consts = (cur + 1) >> 1;
        if (fixup->super.num_consts) {
            shader_addline(buffer, "PARAM np2fixup[%u] = { program.env[%u..%u] };\n",
                           fixup->super.num_consts, fixup->offset, fixup->super.num_consts + fixup->offset - 1);
        }

        next_local += fixup->super.num_consts;
    }

    if (shader_priv->clipplane_emulation != ~0U && args->clip)
    {
        shader_addline(buffer, "KIL fragment.texcoord[%u];\n", shader_priv->clipplane_emulation);
    }

    /* Base Shader Body */
    shader_generate_main((IWineD3DBaseShader *)This, buffer, reg_maps, function, &priv_ctx);

    if(args->super.srgb_correction) {
        arbfp_add_sRGB_correction(buffer, fragcolor, srgbtmp[0], srgbtmp[1], srgbtmp[2], srgbtmp[3],
                                  priv_ctx.target_version >= NV2);
    }

    if(strcmp(fragcolor, "result.color")) {
        shader_addline(buffer, "MOV result.color, %s;\n", fragcolor);
    }
    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &retval));

    TRACE("Creating a hw pixel shader, prg=%d\n", retval);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, retval));

    TRACE("Created hw pixel shader, prg=%d\n", retval);
    /* Create the program and check for errors */
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
               buffer->bsize, buffer->buffer));
    checkGLcall("glProgramStringARB()");

    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
    if (errPos != -1)
    {
        FIXME("HW PixelShader Error at position %d: %s\n\n",
              errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        shader_arb_dump_program_source(buffer->buffer);
        retval = 0;
    }
    else
    {
        GLint native;

        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native));
        checkGLcall("glGetProgramivARB()");
        if (!native) WARN("Program exceeds native resource limits.\n");
    }

    /* Load immediate constants */
    if(lconst_map) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            const float *value = (const float *)lconst->value;
            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, lconst_map[lconst->idx], value));
            checkGLcall("glProgramLocalParameter4fvARB");
        }
        HeapFree(GetProcessHeap(), 0, lconst_map);
    }

    return retval;
}

static int compare_sig(const struct wined3d_shader_signature_element *sig1, const struct wined3d_shader_signature_element *sig2)
{
    unsigned int i;
    int ret;

    for(i = 0; i < MAX_REG_INPUT; i++)
    {
        if(sig1[i].semantic_name == NULL || sig2[i].semantic_name == NULL)
        {
            /* Compare pointers, not contents. One string is NULL(element does not exist), the other one is not NULL */
            if(sig1[i].semantic_name != sig2[i].semantic_name) return sig1[i].semantic_name < sig2[i].semantic_name ? -1 : 1;
            continue;
        }

        ret = strcmp(sig1[i].semantic_name, sig2[i].semantic_name);
        if(ret != 0) return ret;
        if(sig1[i].semantic_idx    != sig2[i].semantic_idx)    return sig1[i].semantic_idx    < sig2[i].semantic_idx    ? -1 : 1;
        if(sig1[i].sysval_semantic != sig2[i].sysval_semantic) return sig1[i].sysval_semantic < sig2[i].sysval_semantic ? -1 : 1;
        if(sig1[i].component_type  != sig2[i].component_type)  return sig1[i].sysval_semantic < sig2[i].component_type  ? -1 : 1;
        if(sig1[i].register_idx    != sig2[i].register_idx)    return sig1[i].register_idx    < sig2[i].register_idx    ? -1 : 1;
        if(sig1[i].mask            != sig2->mask)              return sig1[i].mask            < sig2[i].mask            ? -1 : 1;
    }
    return 0;
}

static struct wined3d_shader_signature_element *clone_sig(const struct wined3d_shader_signature_element *sig)
{
    struct wined3d_shader_signature_element *new;
    int i;
    char *name;

    new = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*new) * MAX_REG_INPUT);
    for(i = 0; i < MAX_REG_INPUT; i++)
    {
        if(sig[i].semantic_name == NULL)
        {
            continue;
        }

        new[i] = sig[i];
        /* Clone the semantic string */
        name = HeapAlloc(GetProcessHeap(), 0, strlen(sig[i].semantic_name) + 1);
        strcpy(name, sig[i].semantic_name);
        new[i].semantic_name = name;
    }
    return new;
}

static DWORD find_input_signature(struct shader_arb_priv *priv, const struct wined3d_shader_signature_element *sig)
{
    struct wine_rb_entry *entry = wine_rb_get(&priv->signature_tree, sig);
    struct ps_signature *found_sig;

    if(entry != NULL)
    {
        found_sig = WINE_RB_ENTRY_VALUE(entry, struct ps_signature, entry);
        TRACE("Found existing signature %u\n", found_sig->idx);
        return found_sig->idx;
    }
    found_sig = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sig));
    found_sig->sig = clone_sig(sig);
    found_sig->idx = priv->ps_sig_number++;
    TRACE("New signature stored and assigned number %u\n", found_sig->idx);
    if(wine_rb_put(&priv->signature_tree, sig, &found_sig->entry) == -1)
    {
        ERR("Failed to insert program entry.\n");
    }
    return found_sig->idx;
}

static void init_output_registers(IWineD3DVertexShaderImpl *shader, DWORD sig_num, struct shader_arb_ctx_priv *priv_ctx,
                                  struct arb_vs_compiled_shader *compiled)
{
    unsigned int i, j;
    static const char *texcoords[8] =
    {
        "result.texcoord[0]", "result.texcoord[1]", "result.texcoord[2]", "result.texcoord[3]",
        "result.texcoord[4]", "result.texcoord[5]", "result.texcoord[6]", "result.texcoord[7]"
    };
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) shader->baseShader.device;
    IWineD3DBaseShaderClass *baseshader = &shader->baseShader;
    const struct wined3d_shader_signature_element *sig;
    const char *semantic_name;
    DWORD semantic_idx, reg_idx;

    /* Write generic input varyings 0 to 7 to result.texcoord[], varying 8 to result.color.primary
     * and varying 9 to result.color.secondary
     */
    const char *decl_idx_to_string[MAX_REG_INPUT] =
    {
        texcoords[0], texcoords[1], texcoords[2], texcoords[3],
        texcoords[4], texcoords[5], texcoords[6], texcoords[7],
        "result.color.primary", "result.color.secondary"
    };

    if(sig_num == ~0)
    {
        TRACE("Pixel shader uses builtin varyings\n");
        /* Map builtins to builtins */
        for(i = 0; i < 8; i++)
        {
            priv_ctx->texcrd_output[i] = texcoords[i];
        }
        priv_ctx->color_output[0] = "result.color.primary";
        priv_ctx->color_output[1] = "result.color.secondary";
        priv_ctx->fog_output = "result.fogcoord";

        /* Map declared regs to builtins. Use "TA" to /dev/null unread output */
        for (i = 0; i < (sizeof(baseshader->output_signature) / sizeof(*baseshader->output_signature)); ++i)
        {
            semantic_name = baseshader->output_signature[i].semantic_name;
            if(semantic_name == NULL) continue;

            if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_POSITION))
            {
                TRACE("o%u is TMP_OUT\n", i);
                if (baseshader->output_signature[i].semantic_idx == 0) priv_ctx->vs_output[i] = "TMP_OUT";
                else priv_ctx->vs_output[i] = "TA";
            }
            else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_PSIZE))
            {
                TRACE("o%u is result.pointsize\n", i);
                if (baseshader->output_signature[i].semantic_idx == 0) priv_ctx->vs_output[i] = "result.pointsize";
                else priv_ctx->vs_output[i] = "TA";
            }
            else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_COLOR))
            {
                TRACE("o%u is result.color.?, idx %u\n", i, baseshader->output_signature[i].semantic_idx);
                if (baseshader->output_signature[i].semantic_idx == 0)
                    priv_ctx->vs_output[i] = "result.color.primary";
                else if (baseshader->output_signature[i].semantic_idx == 1)
                    priv_ctx->vs_output[i] = "result.color.secondary";
                else priv_ctx->vs_output[i] = "TA";
            }
            else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_TEXCOORD))
            {
                TRACE("o%u is %s\n", i, texcoords[baseshader->output_signature[i].semantic_idx]);
                if (baseshader->output_signature[i].semantic_idx >= 8) priv_ctx->vs_output[i] = "TA";
                else priv_ctx->vs_output[i] = texcoords[baseshader->output_signature[i].semantic_idx];
            }
            else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_FOG))
            {
                TRACE("o%u is result.fogcoord\n", i);
                if (baseshader->output_signature[i].semantic_idx > 0) priv_ctx->vs_output[i] = "TA";
                else priv_ctx->vs_output[i] = "result.fogcoord";
            }
            else
            {
                priv_ctx->vs_output[i] = "TA";
            }
        }
        return;
    }

    /* Instead of searching for the signature in the signature list, read the one from the current pixel shader.
     * Its maybe not the shader where the signature came from, but it is the same signature and faster to find
     */
    sig = ((IWineD3DPixelShaderImpl *)device->stateBlock->pixelShader)->baseShader.input_signature;
    TRACE("Pixel shader uses declared varyings\n");

    /* Map builtin to declared. /dev/null the results by default to the TA temp reg */
    for(i = 0; i < 8; i++)
    {
        priv_ctx->texcrd_output[i] = "TA";
    }
    priv_ctx->color_output[0] = "TA";
    priv_ctx->color_output[1] = "TA";
    priv_ctx->fog_output = "TA";

    for(i = 0; i < MAX_REG_INPUT; i++)
    {
        semantic_name = sig[i].semantic_name;
        semantic_idx = sig[i].semantic_idx;
        reg_idx = sig[i].register_idx;
        if(semantic_name == NULL) continue;

        /* If a declared input register is not written by builtin arguments, don't write to it.
         * GL_NV_vertex_program makes sure the input defaults to 0.0, which is correct with D3D
         *
         * Don't care about POSITION and PSIZE here - this is a builtin vertex shader, position goes
         * to TMP_OUT in any case
         */
        if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_TEXCOORD))
        {
            if(semantic_idx < 8) priv_ctx->texcrd_output[semantic_idx] = decl_idx_to_string[reg_idx];
        }
        else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_COLOR))
        {
            if(semantic_idx < 2) priv_ctx->color_output[semantic_idx] = decl_idx_to_string[reg_idx];
        }
        else if(shader_match_semantic(semantic_name, WINED3DDECLUSAGE_FOG))
        {
            if(semantic_idx == 0) priv_ctx->fog_output = decl_idx_to_string[reg_idx];
        }
        else
        {
            continue;
        }

        if(strcmp(decl_idx_to_string[reg_idx], "result.color.primary") == 0 ||
           strcmp(decl_idx_to_string[reg_idx], "result.color.secondary") == 0)
        {
            compiled->need_color_unclamp = TRUE;
        }
    }

    /* Map declared to declared */
    for (i = 0; i < (sizeof(baseshader->output_signature) / sizeof(*baseshader->output_signature)); ++i)
    {
        /* Write unread output to TA to throw them away */
        priv_ctx->vs_output[i] = "TA";
        semantic_name = baseshader->output_signature[i].semantic_name;
        if(semantic_name == NULL)
        {
            continue;
        }

        if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_POSITION)
                && baseshader->output_signature[i].semantic_idx == 0)
        {
            priv_ctx->vs_output[i] = "TMP_OUT";
            continue;
        }
        else if (shader_match_semantic(semantic_name, WINED3DDECLUSAGE_PSIZE)
                && baseshader->output_signature[i].semantic_idx == 0)
        {
            priv_ctx->vs_output[i] = "result.pointsize";
            continue;
        }

        for(j = 0; j < MAX_REG_INPUT; j++)
        {
            if(sig[j].semantic_name == NULL)
            {
                continue;
            }

            if (strcmp(sig[j].semantic_name, semantic_name) == 0
                    && sig[j].semantic_idx == baseshader->output_signature[i].semantic_idx)
            {
                priv_ctx->vs_output[i] = decl_idx_to_string[sig[j].register_idx];

                if(strcmp(priv_ctx->vs_output[i], "result.color.primary") == 0 ||
                   strcmp(priv_ctx->vs_output[i], "result.color.secondary") == 0)
                {
                    compiled->need_color_unclamp = TRUE;
                }
            }
        }
    }
}

/* GL locking is done by the caller */
static GLuint shader_arb_generate_vshader(IWineD3DVertexShaderImpl *This, struct wined3d_shader_buffer *buffer,
        const struct arb_vs_compile_args *args, struct arb_vs_compiled_shader *compiled)
{
    const shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)This->baseShader.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const local_constant *lconst;
    GLuint ret;
    DWORD next_local, *lconst_map = local_const_mapping((IWineD3DBaseShaderImpl *) This);
    struct shader_arb_ctx_priv priv_ctx;
    unsigned int i;
    GLint errPos;

    memset(&priv_ctx, 0, sizeof(priv_ctx));
    priv_ctx.cur_vs_args = args;
    list_init(&priv_ctx.control_frames);
    init_output_registers(This, args->ps_signature, &priv_ctx, compiled);

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
    if(need_helper_const(gl_info)) {
        shader_addline(buffer, "PARAM helper_const = { 2.0, -1.0, %d.0, 0.0 };\n", This->rel_offset);
    }
    if(need_mova_const((IWineD3DBaseShader *) This, gl_info)) {
        shader_addline(buffer, "PARAM mova_const = { 0.5, 0.0, 2.0, 1.0 };\n");
        shader_addline(buffer, "TEMP A0_SHADOW;\n");
    }

    shader_addline(buffer, "TEMP TA;\n");

    /* Base Declarations */
    next_local = shader_generate_arb_declarations((IWineD3DBaseShader *)This,
            reg_maps, buffer, gl_info, lconst_map, &priv_ctx.vs_clipplanes, &priv_ctx);

    for(i = 0; i < MAX_CONST_I; i++)
    {
        compiled->int_consts[i] = WINED3D_CONST_NUM_UNUSED;
        if(reg_maps->integer_constants & (1 << i) && priv_ctx.target_version >= NV2)
        {
            const DWORD *control_values = find_loop_control_values((IWineD3DBaseShaderImpl *) This, i);

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
        shader_addline(buffer, "MOV result.color.secondary, -helper_const.wwwy;\n");

        if (gl_info->quirks & WINED3D_QUIRK_SET_TEXCOORD_W && !device->frag_pipe->ffp_proj_control)
        {
            int i;
            for(i = 0; i < min(8, MAX_REG_TEXCRD); i++) {
                if(This->baseShader.reg_maps.texcoord_mask[i] != 0 &&
                This->baseShader.reg_maps.texcoord_mask[i] != WINED3DSP_WRITEMASK_ALL) {
                    shader_addline(buffer, "MOV result.texcoord[%u].w, -helper_const.y;\n", i);
                }
            }
        }
    }

    /* The shader starts with the main function */
    priv_ctx.in_main_func = TRUE;
    /* Base Shader Body */
    shader_generate_main((IWineD3DBaseShader *)This, buffer, reg_maps, function, &priv_ctx);

    if(!priv_ctx.footer_written) vshader_add_footer(This, buffer, args, &priv_ctx);

    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &ret));

    TRACE("Creating a hw vertex shader, prg=%d\n", ret);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ret));

    TRACE("Created hw vertex shader, prg=%d\n", ret);
    /* Create the program and check for errors */
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
               buffer->bsize, buffer->buffer));
    checkGLcall("glProgramStringARB()");

    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
    if (errPos != -1)
    {
        FIXME("HW VertexShader Error at position %d: %s\n\n",
              errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        shader_arb_dump_program_source(buffer->buffer);
        ret = -1;
    }
    else
    {
        GLint native;

        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native));
        checkGLcall("glGetProgramivARB()");
        if (!native) WARN("Program exceeds native resource limits.\n");

        /* Load immediate constants */
        if(lconst_map) {
            LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
                const float *value = (const float *)lconst->value;
                GL_EXTCALL(glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, lconst_map[lconst->idx], value));
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, lconst_map);

    return ret;
}

/* GL locking is done by the caller */
static struct arb_ps_compiled_shader *find_arb_pshader(IWineD3DPixelShaderImpl *shader, const struct arb_ps_compile_args *args)
{
    UINT i;
    DWORD new_size;
    struct arb_ps_compiled_shader *new_array;
    struct wined3d_shader_buffer buffer;
    struct arb_pshader_private *shader_data;
    GLuint ret;

    if (!shader->baseShader.backend_data)
    {
        IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) shader->baseShader.device;
        const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
        struct shader_arb_priv *priv = device->shader_priv;

        shader->baseShader.backend_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data));
        shader_data = shader->baseShader.backend_data;
        shader_data->clamp_consts = shader->baseShader.reg_maps.shader_version.major == 1;

        if(shader->baseShader.reg_maps.shader_version.major < 3) shader_data->input_signature_idx = ~0;
        else shader_data->input_signature_idx = find_input_signature(priv, shader->baseShader.input_signature);

        shader_data->has_signature_idx = TRUE;
        TRACE("Shader got assigned input signature index %u\n", shader_data->input_signature_idx);

        if (!device->vs_clipping)
            shader_data->clipplane_emulation = shader_find_free_input_register(&shader->baseShader.reg_maps,
                    gl_info->limits.texture_stages - 1);
        else
            shader_data->clipplane_emulation = ~0U;
    }
    shader_data = shader->baseShader.backend_data;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for(i = 0; i < shader_data->num_gl_shaders; i++) {
        if(memcmp(&shader_data->gl_shaders[i].args, args, sizeof(*args)) == 0) {
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
        } else {
            new_array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data->gl_shaders));
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

    pixelshader_update_samplers(&shader->baseShader.reg_maps,
            ((IWineD3DDeviceImpl *)shader->baseShader.device)->stateBlock->textures);

    if (!shader_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    ret = shader_arb_generate_pshader(shader, &buffer, args,
                                      &shader_data->gl_shaders[shader_data->num_gl_shaders]);
    shader_buffer_free(&buffer);
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

    return memcmp(stored->loop_ctrl, new->loop_ctrl, sizeof(stored->loop_ctrl)) == 0;
}

static struct arb_vs_compiled_shader *find_arb_vshader(IWineD3DVertexShaderImpl *shader, const struct arb_vs_compile_args *args)
{
    UINT i;
    DWORD new_size;
    struct arb_vs_compiled_shader *new_array;
    DWORD use_map = ((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.use_map;
    struct wined3d_shader_buffer buffer;
    struct arb_vshader_private *shader_data;
    GLuint ret;
    const struct wined3d_gl_info *gl_info = &((IWineD3DDeviceImpl *)shader->baseShader.device)->adapter->gl_info;

    if (!shader->baseShader.backend_data)
    {
        shader->baseShader.backend_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data));
    }
    shader_data = shader->baseShader.backend_data;

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
        } else {
            new_array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*shader_data->gl_shaders));
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

    if (!shader_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    ret = shader_arb_generate_vshader(shader, &buffer, args,
            &shader_data->gl_shaders[shader_data->num_gl_shaders]);
    shader_buffer_free(&buffer);
    shader_data->gl_shaders[shader_data->num_gl_shaders].prgId = ret;

    return &shader_data->gl_shaders[shader_data->num_gl_shaders++];
}

static inline void find_arb_ps_compile_args(IWineD3DPixelShaderImpl *shader, IWineD3DStateBlockImpl *stateblock,
        struct arb_ps_compile_args *args)
{
    int i;
    WORD int_skip;
    const struct wined3d_gl_info *gl_info = &((IWineD3DDeviceImpl *)shader->baseShader.device)->adapter->gl_info;
    find_ps_compile_args(shader, stateblock, &args->super);

    /* This forces all local boolean constants to 1 to make them stateblock independent */
    args->bools = shader->baseShader.reg_maps.local_bool_consts;

    for(i = 0; i < MAX_CONST_B; i++)
    {
        if(stateblock->pixelShaderConstantB[i]) args->bools |= ( 1 << i);
    }

    /* Only enable the clip plane emulation KIL if at least one clipplane is enabled. The KIL instruction
     * is quite expensive because it forces the driver to disable early Z discards. It is cheaper to
     * duplicate the shader than have a no-op KIL instruction in every shader
     */
    if((!((IWineD3DDeviceImpl *) shader->baseShader.device)->vs_clipping) && use_vs(stateblock) &&
       stateblock->renderState[WINED3DRS_CLIPPING] && stateblock->renderState[WINED3DRS_CLIPPLANEENABLE])
    {
        args->clip = 1;
    }
    else
    {
        args->clip = 0;
    }

    /* Skip if unused or local, or supported natively */
    int_skip = ~shader->baseShader.reg_maps.integer_constants | shader->baseShader.reg_maps.local_int_consts;
    if (int_skip == 0xffff || gl_info->supported[NV_FRAGMENT_PROGRAM_OPTION])
    {
        memset(&args->loop_ctrl, 0, sizeof(args->loop_ctrl));
        return;
    }

    for(i = 0; i < MAX_CONST_I; i++)
    {
        if(int_skip & (1 << i))
        {
            args->loop_ctrl[i][0] = 0;
            args->loop_ctrl[i][1] = 0;
            args->loop_ctrl[i][2] = 0;
        }
        else
        {
            args->loop_ctrl[i][0] = stateblock->pixelShaderConstantI[i * 4];
            args->loop_ctrl[i][1] = stateblock->pixelShaderConstantI[i * 4 + 1];
            args->loop_ctrl[i][2] = stateblock->pixelShaderConstantI[i * 4 + 2];
        }
    }
}

static inline void find_arb_vs_compile_args(IWineD3DVertexShaderImpl *shader, IWineD3DStateBlockImpl *stateblock,
        struct arb_vs_compile_args *args)
{
    int i;
    WORD int_skip;
    IWineD3DDeviceImpl *dev = (IWineD3DDeviceImpl *)shader->baseShader.device;
    const struct wined3d_gl_info *gl_info = &dev->adapter->gl_info;
    find_vs_compile_args(shader, stateblock, &args->super);

    args->clip.boolclip_compare = 0;
    if(use_ps(stateblock))
    {
        IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) stateblock->pixelShader;
        struct arb_pshader_private *shader_priv = ps->baseShader.backend_data;
        args->ps_signature = shader_priv->input_signature_idx;

        args->clip.boolclip.clip_texcoord = shader_priv->clipplane_emulation + 1;
    }
    else
    {
        args->ps_signature = ~0;
        if(!dev->vs_clipping)
        {
            args->clip.boolclip.clip_texcoord = ffp_clip_emul(stateblock) ? gl_info->limits.texture_stages : 0;
        }
        /* Otherwise: Setting boolclip_compare set clip_texcoord to 0 */
    }

    if(args->clip.boolclip.clip_texcoord)
    {
        if(stateblock->renderState[WINED3DRS_CLIPPING])
        {
            args->clip.boolclip.clipplane_mask = stateblock->renderState[WINED3DRS_CLIPPLANEENABLE];
        }
        /* clipplane_mask was set to 0 by setting boolclip_compare to 0 */
    }

    /* This forces all local boolean constants to 1 to make them stateblock independent */
    args->clip.boolclip.bools = shader->baseShader.reg_maps.local_bool_consts;
    /* TODO: Figure out if it would be better to store bool constants as bitmasks in the stateblock */
    for(i = 0; i < MAX_CONST_B; i++)
    {
        if(stateblock->vertexShaderConstantB[i]) args->clip.boolclip.bools |= ( 1 << i);
    }

    args->vertex.samplers[0] = dev->texUnitMap[MAX_FRAGMENT_SAMPLERS + 0];
    args->vertex.samplers[1] = dev->texUnitMap[MAX_FRAGMENT_SAMPLERS + 1];
    args->vertex.samplers[2] = dev->texUnitMap[MAX_FRAGMENT_SAMPLERS + 2];
    args->vertex.samplers[3] = 0;

    /* Skip if unused or local */
    int_skip = ~shader->baseShader.reg_maps.integer_constants | shader->baseShader.reg_maps.local_int_consts;
    /* This is about flow control, not clipping. */
    if (int_skip == 0xffff || gl_info->supported[NV_VERTEX_PROGRAM2_OPTION])
    {
        memset(&args->loop_ctrl, 0, sizeof(args->loop_ctrl));
        return;
    }

    for(i = 0; i < MAX_CONST_I; i++)
    {
        if(int_skip & (1 << i))
        {
            args->loop_ctrl[i][0] = 0;
            args->loop_ctrl[i][1] = 0;
            args->loop_ctrl[i][2] = 0;
        }
        else
        {
            args->loop_ctrl[i][0] = stateblock->vertexShaderConstantI[i * 4];
            args->loop_ctrl[i][1] = stateblock->vertexShaderConstantI[i * 4 + 1];
            args->loop_ctrl[i][2] = stateblock->vertexShaderConstantI[i * 4 + 2];
        }
    }
}

/* GL locking is done by the caller */
static void shader_arb_select(const struct wined3d_context *context, BOOL usePS, BOOL useVS)
{
    IWineD3DDeviceImpl *This = ((IWineD3DSurfaceImpl *)context->surface)->resource.device;
    struct shader_arb_priv *priv = This->shader_priv;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    int i;

    /* Deal with pixel shaders first so the vertex shader arg function has the input signature ready */
    if (usePS) {
        struct arb_ps_compile_args compile_args;
        struct arb_ps_compiled_shader *compiled;
        IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) This->stateBlock->pixelShader;

        TRACE("Using pixel shader %p\n", This->stateBlock->pixelShader);
        find_arb_ps_compile_args(ps, This->stateBlock, &compile_args);
        compiled = find_arb_pshader(ps, &compile_args);
        priv->current_fprogram_id = compiled->prgId;
        priv->compiled_fprog = compiled;

        /* Bind the fragment program */
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->current_fprogram_id));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->current_fprogram_id);");

        if(!priv->use_arbfp_fixed_func) {
            /* Enable OpenGL fragment programs */
            glEnable(GL_FRAGMENT_PROGRAM_ARB);
            checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB);");
        }
        TRACE("(%p) : Bound fragment program %u and enabled GL_FRAGMENT_PROGRAM_ARB\n", This, priv->current_fprogram_id);

        /* Pixel Shader 1.x constants are clamped to [-1;1], Pixel Shader 2.0 constants are not. If switching between
         * a 1.x and newer shader, reload the first 8 constants
         */
        if(priv->last_ps_const_clamped != ((struct arb_pshader_private *)ps->baseShader.backend_data)->clamp_consts)
        {
            priv->last_ps_const_clamped = ((struct arb_pshader_private *)ps->baseShader.backend_data)->clamp_consts;
            This->highest_dirty_ps_const = max(This->highest_dirty_ps_const, 8);
            for(i = 0; i < 8; i++)
            {
                context->pshader_const_dirty[i] = 1;
            }
            /* Also takes care of loading local constants */
            shader_arb_load_constants(context, TRUE, FALSE);
        }
        else
        {
            shader_arb_ps_local_constants(This);
        }

        /* Force constant reloading for the NP2 fixup (see comment in shader_glsl_select for more info) */
        if (compiled->np2fixup_info.super.active)
            shader_arb_load_np2fixup_constants((IWineD3DDevice *)This, usePS, useVS);
    }
    else if (gl_info->supported[ARB_FRAGMENT_PROGRAM] && !priv->use_arbfp_fixed_func)
    {
        /* Disable only if we're not using arbfp fixed function fragment processing. If this is used,
        * keep GL_FRAGMENT_PROGRAM_ARB enabled, and the fixed function pipeline will bind the fixed function
        * replacement shader
        */
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
        priv->current_fprogram_id = 0;
    }

    if (useVS) {
        struct arb_vs_compile_args compile_args;
        struct arb_vs_compiled_shader *compiled;
        IWineD3DVertexShaderImpl *vs = (IWineD3DVertexShaderImpl *) This->stateBlock->vertexShader;

        TRACE("Using vertex shader %p\n", This->stateBlock->vertexShader);
        find_arb_vs_compile_args(vs, This->stateBlock, &compile_args);
        compiled = find_arb_vshader(vs, &compile_args);
        priv->current_vprogram_id = compiled->prgId;
        priv->compiled_vprog = compiled;

        /* Bind the vertex program */
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->current_vprogram_id));
        checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->current_vprogram_id);");

        /* Enable OpenGL vertex programs */
        glEnable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glEnable(GL_VERTEX_PROGRAM_ARB);");
        TRACE("(%p) : Bound vertex program %u and enabled GL_VERTEX_PROGRAM_ARB\n", This, priv->current_vprogram_id);
        shader_arb_vs_local_constants(This);

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
    }
    else if (gl_info->supported[ARB_VERTEX_PROGRAM])
    {
        priv->current_vprogram_id = 0;
        glDisable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glDisable(GL_VERTEX_PROGRAM_ARB)");
    }
}

/* GL locking is done by the caller */
static void shader_arb_select_depth_blt(IWineD3DDevice *iface, enum tex_types tex_type) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = This->shader_priv;
    GLuint *blt_fprogram = &priv->depth_blt_fprogram_id[tex_type];
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;

    if (!priv->depth_blt_vprogram_id) priv->depth_blt_vprogram_id = create_arb_blt_vertex_program(gl_info);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->depth_blt_vprogram_id));
    glEnable(GL_VERTEX_PROGRAM_ARB);

    if (!*blt_fprogram) *blt_fprogram = create_arb_blt_fragment_program(gl_info, tex_type);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, *blt_fprogram));
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
}

/* GL locking is done by the caller */
static void shader_arb_deselect_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = This->shader_priv;
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;

    if (priv->current_vprogram_id) {
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->current_vprogram_id));
        checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertexShader->prgId);");

        TRACE("(%p) : Bound vertex program %u and enabled GL_VERTEX_PROGRAM_ARB\n", This, priv->current_vprogram_id);
    } else {
        glDisable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glDisable(GL_VERTEX_PROGRAM_ARB)");
    }

    if (priv->current_fprogram_id) {
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->current_fprogram_id));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pixelShader->prgId);");

        TRACE("(%p) : Bound fragment program %u and enabled GL_FRAGMENT_PROGRAM_ARB\n", This, priv->current_fprogram_id);
    } else if(!priv->use_arbfp_fixed_func) {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    }
}

static void shader_arb_destroy(IWineD3DBaseShader *iface) {
    IWineD3DBaseShaderImpl *baseShader = (IWineD3DBaseShaderImpl *) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)baseShader->baseShader.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    if (shader_is_pshader_version(baseShader->baseShader.reg_maps.shader_version.type))
    {
        struct arb_pshader_private *shader_data = baseShader->baseShader.backend_data;
        UINT i;

        if(!shader_data) return; /* This can happen if a shader was never compiled */

        if (shader_data->num_gl_shaders)
        {
            struct wined3d_context *context = context_acquire(device, NULL, CTXUSAGE_RESOURCELOAD);

            ENTER_GL();
            for (i = 0; i < shader_data->num_gl_shaders; ++i)
            {
                GL_EXTCALL(glDeleteProgramsARB(1, &shader_data->gl_shaders[i].prgId));
                checkGLcall("GL_EXTCALL(glDeleteProgramsARB(1, &shader_data->gl_shaders[i].prgId))");
            }
            LEAVE_GL();

            context_release(context);
        }

        HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders);
        HeapFree(GetProcessHeap(), 0, shader_data);
        baseShader->baseShader.backend_data = NULL;
    }
    else
    {
        struct arb_vshader_private *shader_data = baseShader->baseShader.backend_data;
        UINT i;

        if(!shader_data) return; /* This can happen if a shader was never compiled */

        if (shader_data->num_gl_shaders)
        {
            struct wined3d_context *context = context_acquire(device, NULL, CTXUSAGE_RESOURCELOAD);

            ENTER_GL();
            for (i = 0; i < shader_data->num_gl_shaders; ++i)
            {
                GL_EXTCALL(glDeleteProgramsARB(1, &shader_data->gl_shaders[i].prgId));
                checkGLcall("GL_EXTCALL(glDeleteProgramsARB(1, &shader_data->gl_shaders[i].prgId))");
            }
            LEAVE_GL();

            context_release(context);
        }

        HeapFree(GetProcessHeap(), 0, shader_data->gl_shaders);
        HeapFree(GetProcessHeap(), 0, shader_data);
        baseShader->baseShader.backend_data = NULL;
    }
}

static int sig_tree_compare(const void *key, const struct wine_rb_entry *entry)
{
    struct ps_signature *e = WINE_RB_ENTRY_VALUE(entry, struct ps_signature, entry);
    return compare_sig(key, e->sig);
}

static const struct wine_rb_functions sig_tree_functions =
{
    wined3d_rb_alloc,
    wined3d_rb_realloc,
    wined3d_rb_free,
    sig_tree_compare
};

static HRESULT shader_arb_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*priv));
    if(wine_rb_init(&priv->signature_tree, &sig_tree_functions) == -1)
    {
        ERR("RB tree init failed\n");
        HeapFree(GetProcessHeap(), 0, priv);
        return E_OUTOFMEMORY;
    }
    This->shader_priv = priv;
    return WINED3D_OK;
}

static void release_signature(struct wine_rb_entry *entry, void *context)
{
    struct ps_signature *sig = WINE_RB_ENTRY_VALUE(entry, struct ps_signature, entry);
    int i;
    for(i = 0; i < MAX_REG_INPUT; i++)
    {
        HeapFree(GetProcessHeap(), 0, (char *) sig->sig[i].semantic_name);
    }
    HeapFree(GetProcessHeap(), 0, sig->sig);
    HeapFree(GetProcessHeap(), 0, sig);
}

/* Context activation is done by the caller. */
static void shader_arb_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;
    struct shader_arb_priv *priv = This->shader_priv;
    int i;

    ENTER_GL();
    if(priv->depth_blt_vprogram_id) {
        GL_EXTCALL(glDeleteProgramsARB(1, &priv->depth_blt_vprogram_id));
    }
    for (i = 0; i < tex_type_count; ++i) {
        if (priv->depth_blt_fprogram_id[i]) {
            GL_EXTCALL(glDeleteProgramsARB(1, &priv->depth_blt_fprogram_id[i]));
        }
    }
    LEAVE_GL();

    wine_rb_destroy(&priv->signature_tree, release_signature, NULL);
    HeapFree(GetProcessHeap(), 0, This->shader_priv);
}

static BOOL shader_arb_dirty_const(IWineD3DDevice *iface) {
    return TRUE;
}

static void shader_arb_get_caps(const struct wined3d_gl_info *gl_info, struct shader_caps *pCaps)
{
    DWORD vs_consts = min(gl_info->limits.arb_vs_float_constants, gl_info->limits.arb_vs_native_constants);
    DWORD ps_consts = min(gl_info->limits.arb_ps_float_constants, gl_info->limits.arb_ps_native_constants);

    /* We don't have an ARB fixed function pipeline yet, so let the none backend set its caps,
     * then overwrite the shader specific ones
     */
    none_shader_backend.shader_get_caps(gl_info, pCaps);

    if (gl_info->supported[ARB_VERTEX_PROGRAM])
    {
        if (gl_info->supported[NV_VERTEX_PROGRAM3])
        {
            pCaps->VertexShaderVersion = WINED3DVS_VERSION(3,0);
            TRACE_(d3d_caps)("Hardware vertex shader version 3.0 enabled (NV_VERTEX_PROGRAM3)\n");
        }
        else if (vs_consts >= 256)
        {
            /* Shader Model 2.0 requires at least 256 vertex shader constants */
            pCaps->VertexShaderVersion = WINED3DVS_VERSION(2,0);
            TRACE_(d3d_caps)("Hardware vertex shader version 2.0 enabled (ARB_PROGRAM)\n");
        }
        else
        {
            pCaps->VertexShaderVersion = WINED3DVS_VERSION(1,1);
            TRACE_(d3d_caps)("Hardware vertex shader version 1.1 enabled (ARB_PROGRAM)\n");
        }
        pCaps->MaxVertexShaderConst = vs_consts;
    }

    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
    {
        if (gl_info->supported[NV_FRAGMENT_PROGRAM2])
        {
            pCaps->PixelShaderVersion    = WINED3DPS_VERSION(3,0);
            TRACE_(d3d_caps)("Hardware pixel shader version 3.0 enabled (NV_FRAGMENT_PROGRAM2)\n");
        }
        else if (ps_consts >= 32)
        {
            /* Shader Model 2.0 requires at least 32 pixel shader constants */
            pCaps->PixelShaderVersion    = WINED3DPS_VERSION(2,0);
            TRACE_(d3d_caps)("Hardware pixel shader version 2.0 enabled (ARB_PROGRAM)\n");
        }
        else
        {
            pCaps->PixelShaderVersion    = WINED3DPS_VERSION(1,4);
            TRACE_(d3d_caps)("Hardware pixel shader version 1.4 enabled (ARB_PROGRAM)\n");
        }
        pCaps->PixelShader1xMaxValue = 8.0f;
        pCaps->MaxPixelShaderConst = ps_consts;
    }

    pCaps->VSClipping = use_nv_clip(gl_info);
}

static BOOL shader_arb_color_fixup_supported(struct color_fixup_desc fixup)
{
    if (TRACE_ON(d3d_shader) && TRACE_ON(d3d))
    {
        TRACE("Checking support for color_fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    /* We support everything except complex conversions. */
    if (!is_complex_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    TRACE("[FAILED]\n");
    return FALSE;
}

static void shader_arb_add_instruction_modifiers(const struct wined3d_shader_instruction *ins) {
    DWORD shift;
    char write_mask[20], regstr[50];
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    BOOL is_color = FALSE;
    const struct wined3d_shader_dst_param *dst;

    if (!ins->dst_count) return;

    dst = &ins->dst[0];
    shift = dst->shift;
    if(shift == 0) return; /* Saturate alone is handled by the instructions */

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
    /* WINED3DSIH_ABS           */ shader_hw_map2gl,
    /* WINED3DSIH_ADD           */ shader_hw_map2gl,
    /* WINED3DSIH_BEM           */ pshader_hw_bem,
    /* WINED3DSIH_BREAK         */ shader_hw_break,
    /* WINED3DSIH_BREAKC        */ shader_hw_breakc,
    /* WINED3DSIH_BREAKP        */ NULL,
    /* WINED3DSIH_CALL          */ shader_hw_call,
    /* WINED3DSIH_CALLNZ        */ NULL,
    /* WINED3DSIH_CMP           */ pshader_hw_cmp,
    /* WINED3DSIH_CND           */ pshader_hw_cnd,
    /* WINED3DSIH_CRS           */ shader_hw_map2gl,
    /* WINED3DSIH_CUT           */ NULL,
    /* WINED3DSIH_DCL           */ NULL,
    /* WINED3DSIH_DEF           */ NULL,
    /* WINED3DSIH_DEFB          */ NULL,
    /* WINED3DSIH_DEFI          */ NULL,
    /* WINED3DSIH_DP2ADD        */ pshader_hw_dp2add,
    /* WINED3DSIH_DP3           */ shader_hw_map2gl,
    /* WINED3DSIH_DP4           */ shader_hw_map2gl,
    /* WINED3DSIH_DST           */ shader_hw_map2gl,
    /* WINED3DSIH_DSX           */ shader_hw_map2gl,
    /* WINED3DSIH_DSY           */ shader_hw_dsy,
    /* WINED3DSIH_ELSE          */ shader_hw_else,
    /* WINED3DSIH_EMIT          */ NULL,
    /* WINED3DSIH_ENDIF         */ shader_hw_endif,
    /* WINED3DSIH_ENDLOOP       */ shader_hw_endloop,
    /* WINED3DSIH_ENDREP        */ shader_hw_endrep,
    /* WINED3DSIH_EXP           */ shader_hw_scalar_op,
    /* WINED3DSIH_EXPP          */ shader_hw_scalar_op,
    /* WINED3DSIH_FRC           */ shader_hw_map2gl,
    /* WINED3DSIH_IADD          */ NULL,
    /* WINED3DSIH_IF            */ NULL /* Hardcoded into the shader */,
    /* WINED3DSIH_IFC           */ shader_hw_ifc,
    /* WINED3DSIH_IGE           */ NULL,
    /* WINED3DSIH_LABEL         */ shader_hw_label,
    /* WINED3DSIH_LIT           */ shader_hw_map2gl,
    /* WINED3DSIH_LOG           */ shader_hw_log_pow,
    /* WINED3DSIH_LOGP          */ shader_hw_log_pow,
    /* WINED3DSIH_LOOP          */ shader_hw_loop,
    /* WINED3DSIH_LRP           */ shader_hw_lrp,
    /* WINED3DSIH_LT            */ NULL,
    /* WINED3DSIH_M3x2          */ shader_hw_mnxn,
    /* WINED3DSIH_M3x3          */ shader_hw_mnxn,
    /* WINED3DSIH_M3x4          */ shader_hw_mnxn,
    /* WINED3DSIH_M4x3          */ shader_hw_mnxn,
    /* WINED3DSIH_M4x4          */ shader_hw_mnxn,
    /* WINED3DSIH_MAD           */ shader_hw_map2gl,
    /* WINED3DSIH_MAX           */ shader_hw_map2gl,
    /* WINED3DSIH_MIN           */ shader_hw_map2gl,
    /* WINED3DSIH_MOV           */ shader_hw_mov,
    /* WINED3DSIH_MOVA          */ shader_hw_mov,
    /* WINED3DSIH_MUL           */ shader_hw_map2gl,
    /* WINED3DSIH_NOP           */ shader_hw_nop,
    /* WINED3DSIH_NRM           */ shader_hw_nrm,
    /* WINED3DSIH_PHASE         */ NULL,
    /* WINED3DSIH_POW           */ shader_hw_log_pow,
    /* WINED3DSIH_RCP           */ shader_hw_scalar_op,
    /* WINED3DSIH_REP           */ shader_hw_rep,
    /* WINED3DSIH_RET           */ shader_hw_ret,
    /* WINED3DSIH_RSQ           */ shader_hw_scalar_op,
    /* WINED3DSIH_SETP          */ NULL,
    /* WINED3DSIH_SGE           */ shader_hw_map2gl,
    /* WINED3DSIH_SGN           */ shader_hw_sgn,
    /* WINED3DSIH_SINCOS        */ shader_hw_sincos,
    /* WINED3DSIH_SLT           */ shader_hw_map2gl,
    /* WINED3DSIH_SUB           */ shader_hw_map2gl,
    /* WINED3DSIH_TEX           */ pshader_hw_tex,
    /* WINED3DSIH_TEXBEM        */ pshader_hw_texbem,
    /* WINED3DSIH_TEXBEML       */ pshader_hw_texbem,
    /* WINED3DSIH_TEXCOORD      */ pshader_hw_texcoord,
    /* WINED3DSIH_TEXDEPTH      */ pshader_hw_texdepth,
    /* WINED3DSIH_TEXDP3        */ pshader_hw_texdp3,
    /* WINED3DSIH_TEXDP3TEX     */ pshader_hw_texdp3tex,
    /* WINED3DSIH_TEXKILL       */ pshader_hw_texkill,
    /* WINED3DSIH_TEXLDD        */ shader_hw_texldd,
    /* WINED3DSIH_TEXLDL        */ shader_hw_texldl,
    /* WINED3DSIH_TEXM3x2DEPTH  */ pshader_hw_texm3x2depth,
    /* WINED3DSIH_TEXM3x2PAD    */ pshader_hw_texm3x2pad,
    /* WINED3DSIH_TEXM3x2TEX    */ pshader_hw_texm3x2tex,
    /* WINED3DSIH_TEXM3x3       */ pshader_hw_texm3x3,
    /* WINED3DSIH_TEXM3x3DIFF   */ NULL,
    /* WINED3DSIH_TEXM3x3PAD    */ pshader_hw_texm3x3pad,
    /* WINED3DSIH_TEXM3x3SPEC   */ pshader_hw_texm3x3spec,
    /* WINED3DSIH_TEXM3x3TEX    */ pshader_hw_texm3x3tex,
    /* WINED3DSIH_TEXM3x3VSPEC  */ pshader_hw_texm3x3vspec,
    /* WINED3DSIH_TEXREG2AR     */ pshader_hw_texreg2ar,
    /* WINED3DSIH_TEXREG2GB     */ pshader_hw_texreg2gb,
    /* WINED3DSIH_TEXREG2RGB    */ pshader_hw_texreg2rgb,
};

static inline BOOL get_bool_const(const struct wined3d_shader_instruction *ins, IWineD3DBaseShaderImpl *This, DWORD idx)
{
    BOOL vshader = shader_is_vshader_version(This->baseShader.reg_maps.shader_version.type);
    WORD bools = 0;
    WORD flag = (1 << idx);
    const local_constant *constant;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;

    if(This->baseShader.reg_maps.local_bool_consts & flag)
    {
        /* What good is a if(bool) with a hardcoded local constant? I don't know, but handle it */
        LIST_FOR_EACH_ENTRY(constant, &This->baseShader.constantsB, local_constant, entry)
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
        IWineD3DBaseShaderImpl *This, UINT idx, struct wined3d_shader_loop_control *loop_control)
{
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;

    /* Integer constants can either be a local constant, or they can be stored in the shader
     * type specific compile args. */
    if (This->baseShader.reg_maps.local_int_consts & (1 << idx))
    {
        const local_constant *constant;

        LIST_FOR_EACH_ENTRY(constant, &This->baseShader.constantsI, local_constant, entry)
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

    switch (This->baseShader.reg_maps.shader_version.type)
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
            FIXME("Unhandled shader type %#x.\n", This->baseShader.reg_maps.shader_version.type);
            break;
    }
}

static void record_instruction(struct list *list, const struct wined3d_shader_instruction *ins)
{
    unsigned int i;
    struct wined3d_shader_dst_param *dst_param = NULL;
    struct wined3d_shader_src_param *src_param = NULL, *rel_addr = NULL;
    struct recorded_instruction *rec = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*rec));
    if(!rec)
    {
        ERR("Out of memory\n");
        return;
    }

    rec->ins = *ins;
    dst_param = HeapAlloc(GetProcessHeap(), 0, sizeof(*dst_param));
    if(!dst_param) goto free;
    *dst_param = *ins->dst;
    if(ins->dst->reg.rel_addr)
    {
        rel_addr = HeapAlloc(GetProcessHeap(), 0, sizeof(*dst_param->reg.rel_addr));
        if(!rel_addr) goto free;
        *rel_addr = *ins->dst->reg.rel_addr;
        dst_param->reg.rel_addr = rel_addr;
    }
    rec->ins.dst = dst_param;

    src_param = HeapAlloc(GetProcessHeap(), 0, sizeof(*src_param) * ins->src_count);
    if(!src_param) goto free;
    for(i = 0; i < ins->src_count; i++)
    {
        src_param[i] = ins->src[i];
        if(ins->src[i].reg.rel_addr)
        {
            rel_addr = HeapAlloc(GetProcessHeap(), 0, sizeof(*rel_addr));
            if(!rel_addr) goto free;
            *rel_addr = *ins->src[i].reg.rel_addr;
            src_param[i].reg.rel_addr = rel_addr;
        }
    }
    rec->ins.src = src_param;
    list_add_tail(list, &rec->entry);
    return;

free:
    ERR("Out of memory\n");
    if(dst_param)
    {
        HeapFree(GetProcessHeap(), 0, (void *) dst_param->reg.rel_addr);
        HeapFree(GetProcessHeap(), 0, dst_param);
    }
    if(src_param)
    {
        for(i = 0; i < ins->src_count; i++)
        {
            HeapFree(GetProcessHeap(), 0, (void *) src_param[i].reg.rel_addr);
        }
        HeapFree(GetProcessHeap(), 0, src_param);
    }
    HeapFree(GetProcessHeap(), 0, rec);
}

static void free_recorded_instruction(struct list *list)
{
    struct recorded_instruction *rec_ins, *entry2;
    unsigned int i;

    LIST_FOR_EACH_ENTRY_SAFE(rec_ins, entry2, list, struct recorded_instruction, entry)
    {
        list_remove(&rec_ins->entry);
        if(rec_ins->ins.dst)
        {
            HeapFree(GetProcessHeap(), 0, (void *) rec_ins->ins.dst->reg.rel_addr);
            HeapFree(GetProcessHeap(), 0, (void *) rec_ins->ins.dst);
        }
        if(rec_ins->ins.src)
        {
            for(i = 0; i < rec_ins->ins.src_count; i++)
            {
                HeapFree(GetProcessHeap(), 0, (void *) rec_ins->ins.src[i].reg.rel_addr);
            }
            HeapFree(GetProcessHeap(), 0, (void *) rec_ins->ins.src);
        }
        HeapFree(GetProcessHeap(), 0, rec_ins);
    }
}

static void shader_arb_handle_instruction(const struct wined3d_shader_instruction *ins) {
    SHADER_HANDLER hw_fct;
    struct shader_arb_ctx_priv *priv = ins->ctx->backend_data;
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)ins->ctx->shader;
    struct control_frame *control_frame;
    struct wined3d_shader_buffer *buffer = ins->ctx->buffer;
    BOOL bool_const;

    if(ins->handler_idx == WINED3DSIH_LOOP || ins->handler_idx == WINED3DSIH_REP)
    {
        control_frame = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*control_frame));
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
                get_loop_control_const(ins, This, ins->src[0].reg.idx, &control_frame->loop_control);
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
                int iteration, aL = 0;
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
                        shader_addline(buffer, "#Iteration %d, aL=%d\n", iteration, aL);
                    }
                    else
                    {
                        shader_addline(buffer, "#Iteration %d\n", iteration);
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
                HeapFree(GetProcessHeap(), 0, control_frame);
                return; /* Instruction is handled */
            }
            else
            {
                /* This is a nested loop. Proceed to the normal recording function */
                HeapFree(GetProcessHeap(), 0, control_frame);
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
        control_frame = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*control_frame));
        list_add_head(&priv->control_frames, &control_frame->entry);
        control_frame->type = IF;

        bool_const = get_bool_const(ins, This, ins->src[0].reg.idx);
        if(ins->src[0].modifiers == WINED3DSPSM_NOT) bool_const = !bool_const;
        if(!priv->muted && bool_const == FALSE)
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
        control_frame = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*control_frame));
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
            HeapFree(GetProcessHeap(), 0, control_frame);
            return; /* Instruction is handled */
        }
    }

    if(priv->muted) return;

    /* Select handler */
    hw_fct = shader_arb_instruction_handler_table[ins->handler_idx];

    /* Unhandled opcode */
    if (!hw_fct)
    {
        FIXME("Backend can't handle opcode %#x\n", ins->handler_idx);
        return;
    }
    hw_fct(ins);

    if(ins->handler_idx == WINED3DSIH_ENDLOOP || ins->handler_idx == WINED3DSIH_ENDREP)
    {
        struct list *e = list_head(&priv->control_frames);
        control_frame = LIST_ENTRY(e, struct control_frame, entry);
        list_remove(&control_frame->entry);
        HeapFree(GetProcessHeap(), 0, control_frame);
        priv->loop_depth--;
    }
    else if(ins->handler_idx == WINED3DSIH_ENDIF)
    {
        /* Non-ifc ENDIFs don't reach that place because of the return in the if block above */
        struct list *e = list_head(&priv->control_frames);
        control_frame = LIST_ENTRY(e, struct control_frame, entry);
        list_remove(&control_frame->entry);
        HeapFree(GetProcessHeap(), 0, control_frame);
    }


    shader_arb_add_instruction_modifiers(ins);
}

const shader_backend_t arb_program_shader_backend = {
    shader_arb_handle_instruction,
    shader_arb_select,
    shader_arb_select_depth_blt,
    shader_arb_deselect_depth_blt,
    shader_arb_update_float_vertex_constants,
    shader_arb_update_float_pixel_constants,
    shader_arb_load_constants,
    shader_arb_load_np2fixup_constants,
    shader_arb_destroy,
    shader_arb_alloc,
    shader_arb_free,
    shader_arb_dirty_const,
    shader_arb_get_caps,
    shader_arb_color_fixup_supported,
};

/* ARB_fragment_program fixed function pipeline replacement definitions */
#define ARB_FFP_CONST_TFACTOR           0
#define ARB_FFP_CONST_SPECULAR_ENABLE   ((ARB_FFP_CONST_TFACTOR) + 1)
#define ARB_FFP_CONST_CONSTANT(i)       ((ARB_FFP_CONST_SPECULAR_ENABLE) + 1 + i)
#define ARB_FFP_CONST_BUMPMAT(i)        ((ARB_FFP_CONST_CONSTANT(7)) + 1 + i)
#define ARB_FFP_CONST_LUMINANCE(i)      ((ARB_FFP_CONST_BUMPMAT(7)) + 1 + i)

struct arbfp_ffp_desc
{
    struct ffp_frag_desc parent;
    GLuint shader;
    unsigned int num_textures_used;
};

/* Context activation is done by the caller. */
static void arbfp_enable(IWineD3DDevice *iface, BOOL enable) {
    ENTER_GL();
    if(enable) {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB)");
    } else {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    }
    LEAVE_GL();
}

static HRESULT arbfp_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    struct shader_arb_priv *priv;
    /* Share private data between the shader backend and the pipeline replacement, if both
     * are the arb implementation. This is needed to figure out whether ARBfp should be disabled
     * if no pixel shader is bound or not
     */
    if(This->shader_backend == &arb_program_shader_backend) {
        This->fragment_priv = This->shader_priv;
    } else {
        This->fragment_priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_arb_priv));
        if(!This->fragment_priv) return E_OUTOFMEMORY;
    }
    priv = This->fragment_priv;
    if (wine_rb_init(&priv->fragment_shaders, &wined3d_ffp_frag_program_rb_functions) == -1)
    {
        ERR("Failed to initialize rbtree.\n");
        HeapFree(GetProcessHeap(), 0, This->fragment_priv);
        return E_OUTOFMEMORY;
    }
    priv->use_arbfp_fixed_func = TRUE;
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void arbfp_free_ffpshader(struct wine_rb_entry *entry, void *context)
{
    const struct wined3d_gl_info *gl_info = context;
    struct arbfp_ffp_desc *entry_arb = WINE_RB_ENTRY_VALUE(entry, struct arbfp_ffp_desc, parent.entry);

    ENTER_GL();
    GL_EXTCALL(glDeleteProgramsARB(1, &entry_arb->shader));
    checkGLcall("glDeleteProgramsARB(1, &entry_arb->shader)");
    HeapFree(GetProcessHeap(), 0, entry_arb);
    LEAVE_GL();
}

/* Context activation is done by the caller. */
static void arbfp_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    struct shader_arb_priv *priv = This->fragment_priv;

    wine_rb_destroy(&priv->fragment_shaders, arbfp_free_ffpshader, &This->adapter->gl_info);
    priv->use_arbfp_fixed_func = FALSE;

    if(This->shader_backend != &arb_program_shader_backend) {
        HeapFree(GetProcessHeap(), 0, This->fragment_priv);
    }
}

static void arbfp_get_caps(const struct wined3d_gl_info *gl_info, struct fragment_caps *caps)
{
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

    caps->MaxTextureBlendStages   = 8;
    caps->MaxSimultaneousTextures = min(gl_info->limits.fragment_samplers, 8);

    caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_TSSARGTEMP;
}
#undef GLINFO_LOCATION

#define GLINFO_LOCATION stateblock->device->adapter->gl_info
static void state_texfactor_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *context)
{
    IWineD3DDeviceImpl *device = stateblock->device;
    float col[4];

    /* Don't load the parameter if we're using an arbfp pixel shader, otherwise we'll overwrite
     * application provided constants
     */
    if(device->shader_backend == &arb_program_shader_backend) {
        if (use_ps(stateblock)) return;

        context->pshader_const_dirty[ARB_FFP_CONST_TFACTOR] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_TFACTOR + 1);
    }

    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_TEXTUREFACTOR], col);
    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_TFACTOR, col));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_TFACTOR, col)");

}

static void state_arb_specularenable(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *context)
{
    IWineD3DDeviceImpl *device = stateblock->device;
    float col[4];

    /* Don't load the parameter if we're using an arbfp pixel shader, otherwise we'll overwrite
     * application provided constants
     */
    if(device->shader_backend == &arb_program_shader_backend) {
        if (use_ps(stateblock)) return;

        context->pshader_const_dirty[ARB_FFP_CONST_SPECULAR_ENABLE] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_SPECULAR_ENABLE + 1);
    }

    if(stateblock->renderState[WINED3DRS_SPECULARENABLE]) {
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

static void set_bumpmat_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *context)
{
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    IWineD3DDeviceImpl *device = stateblock->device;
    float mat[2][2];

    if (use_ps(stateblock))
    {
        if (stage != 0
                && (((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.reg_maps.bumpmat & (1 << stage)))
        {
            /* The pixel shader has to know the bump env matrix. Do a constants update if it isn't scheduled
             * anyway
             */
            if(!isStateDirty(context, STATE_PIXELSHADERCONSTANT)) {
                device->StateTable[STATE_PIXELSHADERCONSTANT].apply(STATE_PIXELSHADERCONSTANT, stateblock, context);
            }
        }

        if(device->shader_backend == &arb_program_shader_backend) {
            /* Exit now, don't set the bumpmat below, otherwise we may overwrite pixel shader constants */
            return;
        }
    } else if(device->shader_backend == &arb_program_shader_backend) {
        context->pshader_const_dirty[ARB_FFP_CONST_BUMPMAT(stage)] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_BUMPMAT(stage) + 1);
    }

    mat[0][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT00]);
    mat[0][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT01]);
    mat[1][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT10]);
    mat[1][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT11]);

    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_BUMPMAT(stage), &mat[0][0]));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_BUMPMAT(stage), &mat[0][0])");
}

static void tex_bumpenvlum_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *context)
{
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    IWineD3DDeviceImpl *device = stateblock->device;
    float param[4];

    if (use_ps(stateblock))
    {
        if (stage != 0
                && (((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.reg_maps.luminanceparams & (1 << stage)))
        {
            /* The pixel shader has to know the luminance offset. Do a constants update if it
             * isn't scheduled anyway
             */
            if(!isStateDirty(context, STATE_PIXELSHADERCONSTANT)) {
                device->StateTable[STATE_PIXELSHADERCONSTANT].apply(STATE_PIXELSHADERCONSTANT, stateblock, context);
            }
        }

        if(device->shader_backend == &arb_program_shader_backend) {
            /* Exit now, don't set the bumpmat below, otherwise we may overwrite pixel shader constants */
            return;
        }
    } else if(device->shader_backend == &arb_program_shader_backend) {
        context->pshader_const_dirty[ARB_FFP_CONST_LUMINANCE(stage)] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_LUMINANCE(stage) + 1);
    }

    param[0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVLSCALE]);
    param[1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVLOFFSET]);
    param[2] = 0.0f;
    param[3] = 0.0f;

    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_LUMINANCE(stage), param));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_LUMINANCE(stage), param)");
}

static const char *get_argreg(struct wined3d_shader_buffer *buffer, DWORD argnum, unsigned int stage, DWORD arg)
{
    const char *ret;

    if(arg == ARG_UNUSED) return "unused"; /* This is the marker for unused registers */

    switch(arg & WINED3DTA_SELECTMASK) {
        case WINED3DTA_DIFFUSE:
            ret = "fragment.color.primary"; break;

        case WINED3DTA_CURRENT:
            if(stage == 0) ret = "fragment.color.primary";
            else ret = "ret";
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
            FIXME("Implement perstage constants\n");
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

static void gen_ffp_instr(struct wined3d_shader_buffer *buffer, unsigned int stage, BOOL color,
        BOOL alpha, DWORD dst, DWORD op, DWORD dw_arg0, DWORD dw_arg1, DWORD dw_arg2)
{
    const char *dstmask, *dstreg, *arg0, *arg1, *arg2;
    unsigned int mul = 1;
    BOOL mul_final_dest = FALSE;

    if(color && alpha) dstmask = "";
    else if(color) dstmask = ".xyz";
    else dstmask = ".w";

    if(dst == tempreg) dstreg = "tempreg";
    else dstreg = "ret";

    arg0 = get_argreg(buffer, 0, stage, dw_arg0);
    arg1 = get_argreg(buffer, 1, stage, dw_arg1);
    arg2 = get_argreg(buffer, 2, stage, dw_arg2);

    switch(op) {
        case WINED3DTOP_DISABLE:
            if(stage == 0) shader_addline(buffer, "MOV %s%s, fragment.color.primary;\n", dstreg, dstmask);
            break;

        case WINED3DTOP_SELECTARG2:
            arg1 = arg2;
        case WINED3DTOP_SELECTARG1:
            shader_addline(buffer, "MOV %s%s, %s;\n", dstreg, dstmask, arg1);
            break;

        case WINED3DTOP_MODULATE4X:
            mul = 2;
        case WINED3DTOP_MODULATE2X:
            mul *= 2;
            if(strcmp(dstreg, "result.color") == 0) {
                dstreg = "ret";
                mul_final_dest = TRUE;
            }
        case WINED3DTOP_MODULATE:
            shader_addline(buffer, "MUL %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3DTOP_ADDSIGNED2X:
            mul = 2;
            if(strcmp(dstreg, "result.color") == 0) {
                dstreg = "ret";
                mul_final_dest = TRUE;
            }
        case WINED3DTOP_ADDSIGNED:
            shader_addline(buffer, "SUB arg2, %s, const.w;\n", arg2);
            arg2 = "arg2";
        case WINED3DTOP_ADD:
            shader_addline(buffer, "ADD_SAT %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3DTOP_SUBTRACT:
            shader_addline(buffer, "SUB_SAT %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3DTOP_ADDSMOOTH:
            shader_addline(buffer, "SUB arg1, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg1, %s, %s;\n", dstreg, dstmask, arg2, arg1);
            break;

        case WINED3DTOP_BLENDCURRENTALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_CURRENT);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3DTOP_BLENDFACTORALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TFACTOR);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3DTOP_BLENDTEXTUREALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3DTOP_BLENDDIFFUSEALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_DIFFUSE);
            shader_addline(buffer, "LRP %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;

        case WINED3DTOP_BLENDTEXTUREALPHAPM:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "SUB arg0.w, const.x, %s.w;\n", arg0);
            shader_addline(buffer, "MAD_SAT %s%s, %s, arg0.w, %s;\n", dstreg, dstmask, arg2, arg1);
            break;

        /* D3DTOP_PREMODULATE ???? */

        case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
            shader_addline(buffer, "SUB arg0.w, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg0.w, %s, %s;\n", dstreg, dstmask, arg2, arg1);
            break;
        case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
            shader_addline(buffer, "MAD_SAT %s%s, %s.w, %s, %s;\n", dstreg, dstmask, arg1, arg2, arg1);
            break;
        case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
            shader_addline(buffer, "SUB arg0, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg0, %s, %s.w;\n", dstreg, dstmask, arg2, arg1);
            break;
        case WINED3DTOP_MODULATECOLOR_ADDALPHA:
            shader_addline(buffer, "MAD_SAT %s%s, %s, %s, %s.w;\n", dstreg, dstmask, arg1, arg2, arg1);
            break;

        case WINED3DTOP_DOTPRODUCT3:
            mul = 4;
            if(strcmp(dstreg, "result.color") == 0) {
                dstreg = "ret";
                mul_final_dest = TRUE;
            }
            shader_addline(buffer, "SUB arg1, %s, const.w;\n", arg1);
            shader_addline(buffer, "SUB arg2, %s, const.w;\n", arg2);
            shader_addline(buffer, "DP3_SAT %s%s, arg1, arg2;\n", dstreg, dstmask);
            break;

        case WINED3DTOP_MULTIPLYADD:
            shader_addline(buffer, "MAD_SAT %s%s, %s, %s, %s;\n", dstreg, dstmask, arg1, arg2, arg0);
            break;

        case WINED3DTOP_LERP:
            /* The msdn is not quite right here */
            shader_addline(buffer, "LRP %s%s, %s, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;

        case WINED3DTOP_BUMPENVMAP:
        case WINED3DTOP_BUMPENVMAPLUMINANCE:
            /* Those are handled in the first pass of the shader(generation pass 1 and 2) already */
            break;

        default:
            FIXME("Unhandled texture op %08x\n", op);
    }

    if(mul == 2) {
        shader_addline(buffer, "MUL_SAT %s%s, %s, const.y;\n", mul_final_dest ? "result.color" : dstreg, dstmask, dstreg);
    } else if(mul == 4) {
        shader_addline(buffer, "MUL_SAT %s%s, %s, const.z;\n", mul_final_dest ? "result.color" : dstreg, dstmask, dstreg);
    }
}

/* The stateblock is passed for GLINFO_LOCATION */
static GLuint gen_arbfp_ffp_shader(const struct ffp_frag_settings *settings, IWineD3DStateBlockImpl *stateblock)
{
    unsigned int stage;
    struct wined3d_shader_buffer buffer;
    BOOL tex_read[MAX_TEXTURES] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
    BOOL bump_used[MAX_TEXTURES] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
    BOOL luminance_used[MAX_TEXTURES] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
    const char *textype;
    const char *instr, *sat;
    char colorcor_dst[8];
    GLuint ret;
    DWORD arg0, arg1, arg2;
    BOOL tempreg_used = FALSE, tfactor_used = FALSE;
    BOOL op_equal;
    const char *final_combiner_src = "ret";
    GLint pos;

    /* Find out which textures are read */
    for(stage = 0; stage < MAX_TEXTURES; stage++) {
        if(settings->op[stage].cop == WINED3DTOP_DISABLE) break;
        arg0 = settings->op[stage].carg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].carg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].carg2 & WINED3DTA_SELECTMASK;
        if(arg0 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg1 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg2 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;

        if(settings->op[stage].cop == WINED3DTOP_BLENDTEXTUREALPHA) tex_read[stage] = TRUE;
        if(settings->op[stage].cop == WINED3DTOP_BLENDTEXTUREALPHAPM) tex_read[stage] = TRUE;
        if(settings->op[stage].cop == WINED3DTOP_BUMPENVMAP) {
            bump_used[stage] = TRUE;
            tex_read[stage] = TRUE;
        }
        if(settings->op[stage].cop == WINED3DTOP_BUMPENVMAPLUMINANCE) {
            bump_used[stage] = TRUE;
            tex_read[stage] = TRUE;
            luminance_used[stage] = TRUE;
        } else if(settings->op[stage].cop == WINED3DTOP_BLENDFACTORALPHA) {
            tfactor_used = TRUE;
        }

        if(arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR) {
            tfactor_used = TRUE;
        }

        if(settings->op[stage].dst == tempreg) tempreg_used = TRUE;
        if(arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP) {
            tempreg_used = TRUE;
        }

        if(settings->op[stage].aop == WINED3DTOP_DISABLE) continue;
        arg0 = settings->op[stage].aarg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].aarg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].aarg2 & WINED3DTA_SELECTMASK;
        if(arg0 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg1 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg2 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;

        if(arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP) {
            tempreg_used = TRUE;
        }
        if(arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR) {
            tfactor_used = TRUE;
        }
    }

    /* Shader header */
    if (!shader_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    shader_addline(&buffer, "!!ARBfp1.0\n");

    switch(settings->fog) {
        case FOG_OFF:                                                         break;
        case FOG_LINEAR: shader_addline(&buffer, "OPTION ARB_fog_linear;\n"); break;
        case FOG_EXP:    shader_addline(&buffer, "OPTION ARB_fog_exp;\n");    break;
        case FOG_EXP2:   shader_addline(&buffer, "OPTION ARB_fog_exp2;\n");   break;
        default: FIXME("Unexpected fog setting %d\n", settings->fog);
    }

    shader_addline(&buffer, "PARAM const = {1, 2, 4, 0.5};\n");
    shader_addline(&buffer, "TEMP TMP;\n");
    shader_addline(&buffer, "TEMP ret;\n");
    if(tempreg_used || settings->sRGB_write) shader_addline(&buffer, "TEMP tempreg;\n");
    shader_addline(&buffer, "TEMP arg0;\n");
    shader_addline(&buffer, "TEMP arg1;\n");
    shader_addline(&buffer, "TEMP arg2;\n");
    for(stage = 0; stage < MAX_TEXTURES; stage++) {
        if(!tex_read[stage]) continue;
        shader_addline(&buffer, "TEMP tex%u;\n", stage);
        if(!bump_used[stage]) continue;
        shader_addline(&buffer, "PARAM bumpmat%u = program.env[%u];\n", stage, ARB_FFP_CONST_BUMPMAT(stage));
        if(!luminance_used[stage]) continue;
        shader_addline(&buffer, "PARAM luminance%u = program.env[%u];\n", stage, ARB_FFP_CONST_LUMINANCE(stage));
    }
    if(tfactor_used) {
        shader_addline(&buffer, "PARAM tfactor = program.env[%u];\n", ARB_FFP_CONST_TFACTOR);
    }
        shader_addline(&buffer, "PARAM specular_enable = program.env[%u];\n", ARB_FFP_CONST_SPECULAR_ENABLE);

    if(settings->sRGB_write) {
        shader_addline(&buffer, "PARAM srgb_consts1 = {%f, %f, %f, %f};\n",
                       srgb_mul_low, srgb_cmp, srgb_pow, srgb_mul_high);
        shader_addline(&buffer, "PARAM srgb_consts2 = {%f, %f, %f, %f};\n",
                       srgb_sub_high, 0.0, 0.0, 0.0);
    }

    if(ffp_clip_emul(stateblock) && settings->emul_clipplanes) shader_addline(&buffer, "KIL fragment.texcoord[7];\n");

    /* Generate texture sampling instructions) */
    for(stage = 0; stage < MAX_TEXTURES && settings->op[stage].cop != WINED3DTOP_DISABLE; stage++) {
        if(!tex_read[stage]) continue;

        switch(settings->op[stage].tex_type) {
            case tex_1d:                    textype = "1D";     break;
            case tex_2d:                    textype = "2D";     break;
            case tex_3d:                    textype = "3D";     break;
            case tex_cube:                  textype = "CUBE";   break;
            case tex_rect:                  textype = "RECT";   break;
            default: textype = "unexpected_textype";   break;
        }

        if(settings->op[stage].cop == WINED3DTOP_BUMPENVMAP ||
           settings->op[stage].cop == WINED3DTOP_BUMPENVMAPLUMINANCE) {
            sat = "";
        } else {
            sat = "_SAT";
        }

        if(settings->op[stage].projected == proj_none) {
            instr = "TEX";
        } else if(settings->op[stage].projected == proj_count4 ||
                  settings->op[stage].projected == proj_count3) {
            instr = "TXP";
        } else {
            FIXME("Unexpected projection mode %d\n", settings->op[stage].projected);
            instr = "TXP";
        }

        if(stage > 0 &&
           (settings->op[stage - 1].cop == WINED3DTOP_BUMPENVMAP ||
            settings->op[stage - 1].cop == WINED3DTOP_BUMPENVMAPLUMINANCE)) {
            shader_addline(&buffer, "SWZ arg1, bumpmat%u, x, z, 0, 0;\n", stage - 1);
            shader_addline(&buffer, "DP3 ret.x, arg1, tex%u;\n", stage - 1);
            shader_addline(&buffer, "SWZ arg1, bumpmat%u, y, w, 0, 0;\n", stage - 1);
            shader_addline(&buffer, "DP3 ret.y, arg1, tex%u;\n", stage - 1);

            /* with projective textures, texbem only divides the static texture coord, not the displacement,
             * so multiply the displacement with the dividing parameter before passing it to TXP
             */
            if (settings->op[stage].projected != proj_none) {
                if(settings->op[stage].projected == proj_count4) {
                    shader_addline(&buffer, "MOV ret.w, fragment.texcoord[%u].w;\n", stage);
                    shader_addline(&buffer, "MUL ret.xyz, ret, fragment.texcoord[%u].w, fragment.texcoord[%u];\n", stage, stage);
                } else {
                    shader_addline(&buffer, "MOV ret.w, fragment.texcoord[%u].z;\n", stage);
                    shader_addline(&buffer, "MAD ret.xyz, ret, fragment.texcoord[%u].z, fragment.texcoord[%u];\n", stage, stage);
                }
            } else {
                shader_addline(&buffer, "ADD ret, ret, fragment.texcoord[%u];\n", stage);
            }

            shader_addline(&buffer, "%s%s tex%u, ret, texture[%u], %s;\n",
                           instr, sat, stage, stage, textype);
            if(settings->op[stage - 1].cop == WINED3DTOP_BUMPENVMAPLUMINANCE) {
                shader_addline(&buffer, "MAD_SAT ret.x, tex%u.z, luminance%u.x, luminance%u.y;\n",
                               stage - 1, stage - 1, stage - 1);
                shader_addline(&buffer, "MUL tex%u, tex%u, ret.x;\n", stage, stage);
            }
        } else if(settings->op[stage].projected == proj_count3) {
            shader_addline(&buffer, "MOV ret, fragment.texcoord[%u];\n", stage);
            shader_addline(&buffer, "MOV ret.w, ret.z;\n");
            shader_addline(&buffer, "%s%s tex%u, ret, texture[%u], %s;\n",
                            instr, sat, stage, stage, textype);
        } else {
            shader_addline(&buffer, "%s%s tex%u, fragment.texcoord[%u], texture[%u], %s;\n",
                            instr, sat, stage, stage, stage, textype);
        }

        sprintf(colorcor_dst, "tex%u", stage);
        gen_color_correction(&buffer, colorcor_dst, WINED3DSP_WRITEMASK_ALL, "const.x", "const.y",
                settings->op[stage].color_fixup);
    }

    /* Generate the main shader */
    for(stage = 0; stage < MAX_TEXTURES; stage++) {
        if(settings->op[stage].cop == WINED3DTOP_DISABLE) {
            if(stage == 0) {
                final_combiner_src = "fragment.color.primary";
            }
            break;
        }

        if(settings->op[stage].cop == WINED3DTOP_SELECTARG1 &&
           settings->op[stage].aop == WINED3DTOP_SELECTARG1) {
            op_equal = settings->op[stage].carg1 == settings->op[stage].aarg1;
        } else if(settings->op[stage].cop == WINED3DTOP_SELECTARG1 &&
                  settings->op[stage].aop == WINED3DTOP_SELECTARG2) {
            op_equal = settings->op[stage].carg1 == settings->op[stage].aarg2;
        } else if(settings->op[stage].cop == WINED3DTOP_SELECTARG2 &&
                  settings->op[stage].aop == WINED3DTOP_SELECTARG1) {
            op_equal = settings->op[stage].carg2 == settings->op[stage].aarg1;
        } else if(settings->op[stage].cop == WINED3DTOP_SELECTARG2 &&
                  settings->op[stage].aop == WINED3DTOP_SELECTARG2) {
            op_equal = settings->op[stage].carg2 == settings->op[stage].aarg2;
        } else {
            op_equal = settings->op[stage].aop   == settings->op[stage].cop &&
                       settings->op[stage].carg0 == settings->op[stage].aarg0 &&
                       settings->op[stage].carg1 == settings->op[stage].aarg1 &&
                       settings->op[stage].carg2 == settings->op[stage].aarg2;
        }

        if(settings->op[stage].aop == WINED3DTOP_DISABLE) {
            gen_ffp_instr(&buffer, stage, TRUE, FALSE, settings->op[stage].dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
            if(stage == 0) {
                shader_addline(&buffer, "MOV ret.w, fragment.color.primary.w;\n");
            }
        } else if(op_equal) {
            gen_ffp_instr(&buffer, stage, TRUE, TRUE, settings->op[stage].dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
        } else {
            gen_ffp_instr(&buffer, stage, TRUE, FALSE, settings->op[stage].dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
            gen_ffp_instr(&buffer, stage, FALSE, TRUE, settings->op[stage].dst,
                          settings->op[stage].aop, settings->op[stage].aarg0,
                          settings->op[stage].aarg1, settings->op[stage].aarg2);
        }
    }

    if(settings->sRGB_write) {
        shader_addline(&buffer, "MAD ret, fragment.color.secondary, specular_enable, %s;\n", final_combiner_src);
        arbfp_add_sRGB_correction(&buffer, "ret", "arg0", "arg1", "arg2", "tempreg", FALSE);
        shader_addline(&buffer, "MOV result.color, ret;\n");
    } else {
        shader_addline(&buffer, "MAD result.color, fragment.color.secondary, specular_enable, %s;\n", final_combiner_src);
    }

    /* Footer */
    shader_addline(&buffer, "END\n");

    /* Generate the shader */
    GL_EXTCALL(glGenProgramsARB(1, &ret));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ret));
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            strlen(buffer.buffer), buffer.buffer));
    checkGLcall("glProgramStringARB()");

    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if (pos != -1)
    {
        FIXME("Fragment program error at position %d: %s\n\n", pos,
              debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        shader_arb_dump_program_source(buffer.buffer);
    }
    else
    {
        GLint native;

        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native));
        checkGLcall("glGetProgramivARB()");
        if (!native) WARN("Program exceeds native resource limits.\n");
    }

    shader_buffer_free(&buffer);
    return ret;
}

static void fragment_prog_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *context)
{
    IWineD3DDeviceImpl *device = stateblock->device;
    struct shader_arb_priv *priv = device->fragment_priv;
    BOOL use_pshader = use_ps(stateblock);
    BOOL use_vshader = use_vs(stateblock);
    struct ffp_frag_settings settings;
    const struct arbfp_ffp_desc *desc;
    unsigned int i;

    TRACE("state %#x, stateblock %p, context %p\n", state, stateblock, context);

    if(isStateDirty(context, STATE_RENDER(WINED3DRS_FOGENABLE))) {
        if(!use_pshader && device->shader_backend == &arb_program_shader_backend && context->last_was_pshader) {
            /* Reload fixed function constants since they collide with the pixel shader constants */
            for(i = 0; i < MAX_TEXTURES; i++) {
                set_bumpmat_arbfp(STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT00), stateblock, context);
            }
            state_texfactor_arbfp(STATE_RENDER(WINED3DRS_TEXTUREFACTOR), stateblock, context);
            state_arb_specularenable(STATE_RENDER(WINED3DRS_SPECULARENABLE), stateblock, context);
        } else if(use_pshader && !isStateDirty(context, device->StateTable[STATE_VSHADER].representative)) {
            device->shader_backend->shader_select(context, use_pshader, use_vshader);
        }
        return;
    }

    if(!use_pshader) {
        /* Find or create a shader implementing the fixed function pipeline settings, then activate it */
        gen_ffp_frag_op(stateblock, &settings, FALSE);
        desc = (const struct arbfp_ffp_desc *)find_ffp_frag_shader(&priv->fragment_shaders, &settings);
        if(!desc) {
            struct arbfp_ffp_desc *new_desc = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_desc));
            if (!new_desc)
            {
                ERR("Out of memory\n");
                return;
            }
            new_desc->num_textures_used = 0;
            for (i = 0; i < context->gl_info->limits.texture_stages; ++i)
            {
                if(settings.op[i].cop == WINED3DTOP_DISABLE) break;
                new_desc->num_textures_used = i;
            }

            memcpy(&new_desc->parent.settings, &settings, sizeof(settings));
            new_desc->shader = gen_arbfp_ffp_shader(&settings, stateblock);
            add_ffp_frag_shader(&priv->fragment_shaders, &new_desc->parent);
            TRACE("Allocated fixed function replacement shader descriptor %p\n", new_desc);
            desc = new_desc;
        }

        /* Now activate the replacement program. GL_FRAGMENT_PROGRAM_ARB is already active(however, note the
         * comment above the shader_select call below). If e.g. GLSL is active, the shader_select call will
         * deactivate it.
         */
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, desc->shader));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, desc->shader)");
        priv->current_fprogram_id = desc->shader;

        if(device->shader_backend == &arb_program_shader_backend && context->last_was_pshader) {
            /* Reload fixed function constants since they collide with the pixel shader constants */
            for(i = 0; i < MAX_TEXTURES; i++) {
                set_bumpmat_arbfp(STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT00), stateblock, context);
            }
            state_texfactor_arbfp(STATE_RENDER(WINED3DRS_TEXTUREFACTOR), stateblock, context);
            state_arb_specularenable(STATE_RENDER(WINED3DRS_SPECULARENABLE), stateblock, context);
        }
        context->last_was_pshader = FALSE;
    } else {
        context->last_was_pshader = TRUE;
    }

    /* Finally, select the shader. If a pixel shader is used, it will be set and enabled by the shader backend.
     * If this shader backend is arbfp(most likely), then it will simply overwrite the last fixed function replace-
     * ment shader. If the shader backend is not ARB, it currently is important that the opengl implementation
     * type overwrites GL_ARB_fragment_program. This is currently the case with GLSL. If we really want to use
     * atifs or nvrc pixel shaders with arb fragment programs we'd have to disable GL_FRAGMENT_PROGRAM_ARB here
     *
     * Don't call shader_select if the vertex shader is dirty, because it will be called later on by the vertex
     * shader handler
     */
    if(!isStateDirty(context, device->StateTable[STATE_VSHADER].representative)) {
        device->shader_backend->shader_select(context, use_pshader, use_vshader);

        if (!isStateDirty(context, STATE_VERTEXSHADERCONSTANT) && (use_vshader || use_pshader)) {
            device->StateTable[STATE_VERTEXSHADERCONSTANT].apply(STATE_VERTEXSHADERCONSTANT, stateblock, context);
        }
    }
    if(use_pshader) {
        device->StateTable[STATE_PIXELSHADERCONSTANT].apply(STATE_PIXELSHADERCONSTANT, stateblock, context);
    }
}

/* We can't link the fog states to the fragment state directly since the vertex pipeline links them
 * to FOGENABLE. A different linking in different pipeline parts can't be expressed in the combined
 * state table, so we need to handle that with a forwarding function. The other invisible side effect
 * is that changing the fog start and fog end(which links to FOGENABLE in vertex) results in the
 * fragment_prog_arbfp function being called because FOGENABLE is dirty, which calls this function here
 */
static void state_arbfp_fog(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *context)
{
    enum fogsource new_source;

    TRACE("state %#x, stateblock %p, context %p\n", state, stateblock, context);

    if(!isStateDirty(context, STATE_PIXELSHADER)) {
        fragment_prog_arbfp(state, stateblock, context);
    }

    if(!stateblock->renderState[WINED3DRS_FOGENABLE]) return;

    if(stateblock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE) {
        if(use_vs(stateblock)) {
            new_source = FOGSOURCE_VS;
        } else {
            if(stateblock->renderState[WINED3DRS_FOGVERTEXMODE] == WINED3DFOG_NONE || context->last_was_rhw) {
                new_source = FOGSOURCE_COORD;
            } else {
                new_source = FOGSOURCE_FFP;
            }
        }
    } else {
        new_source = FOGSOURCE_FFP;
    }
    if(new_source != context->fog_source) {
        context->fog_source = new_source;
        state_fogstartend(STATE_RENDER(WINED3DRS_FOGSTART), stateblock, context);
    }
}

static void textransform(DWORD state, IWineD3DStateBlockImpl *stateblock, struct wined3d_context *context)
{
    if(!isStateDirty(context, STATE_PIXELSHADER)) {
        fragment_prog_arbfp(state, stateblock, context);
    }
}

#undef GLINFO_LOCATION

static const struct StateEntryTemplate arbfp_fragmentstate_template[] = {
    {STATE_RENDER(WINED3DRS_TEXTUREFACTOR),               { STATE_RENDER(WINED3DRS_TEXTUREFACTOR),              state_texfactor_arbfp   }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(0),                                    { STATE_SAMPLER(0),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(1),                                    { STATE_SAMPLER(1),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(2),                                    { STATE_SAMPLER(2),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(3),                                    { STATE_SAMPLER(3),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(4),                                    { STATE_SAMPLER(4),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(5),                                    { STATE_SAMPLER(5),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(6),                                    { STATE_SAMPLER(6),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_SAMPLER(7),                                    { STATE_SAMPLER(7),                                   sampler_texdim          }, WINED3D_GL_EXT_NONE             },
    {STATE_PIXELSHADER,                                   { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_FOGENABLE),                   { STATE_RENDER(WINED3DRS_FOGENABLE),                  state_arbfp_fog         }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_FOGTABLEMODE),                { STATE_RENDER(WINED3DRS_FOGENABLE),                  state_arbfp_fog         }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_FOGVERTEXMODE),               { STATE_RENDER(WINED3DRS_FOGENABLE),                  state_arbfp_fog         }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_FOGSTART),                    { STATE_RENDER(WINED3DRS_FOGSTART),                   state_fogstartend       }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_FOGEND),                      { STATE_RENDER(WINED3DRS_FOGSTART),                   state_fogstartend       }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_SRGBWRITEENABLE),             { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_FOGCOLOR),                    { STATE_RENDER(WINED3DRS_FOGCOLOR),                   state_fogcolor          }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_FOGDENSITY),                  { STATE_RENDER(WINED3DRS_FOGDENSITY),                 state_fogdensity        }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(0,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(0, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(1,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(1, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(2,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(2, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(3,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(3, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(4,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(4, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(5,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(5, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(6,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(6, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_TEXTURESTAGE(7,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(7, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, WINED3D_GL_EXT_NONE             },
    {STATE_RENDER(WINED3DRS_SPECULARENABLE),              { STATE_RENDER(WINED3DRS_SPECULARENABLE),             state_arb_specularenable}, WINED3D_GL_EXT_NONE             },
    {0 /* Terminate */,                                   { 0,                                                  0                       }, WINED3D_GL_EXT_NONE             },
};

const struct fragment_pipeline arbfp_fragment_pipeline = {
    arbfp_enable,
    arbfp_get_caps,
    arbfp_alloc,
    arbfp_free,
    shader_arb_color_fixup_supported,
    arbfp_fragmentstate_template,
    TRUE /* We can disable projected textures */
};

#define GLINFO_LOCATION device->adapter->gl_info

struct arbfp_blit_priv {
    GLenum yuy2_rect_shader, yuy2_2d_shader;
    GLenum uyvy_rect_shader, uyvy_2d_shader;
    GLenum yv12_rect_shader, yv12_2d_shader;
    GLenum p8_rect_shader, p8_2d_shader;
};

static HRESULT arbfp_blit_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    device->blit_priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct arbfp_blit_priv));
    if(!device->blit_priv) {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void arbfp_blit_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    struct arbfp_blit_priv *priv = device->blit_priv;

    ENTER_GL();
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yuy2_rect_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yuy2_2d_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->uyvy_rect_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->uyvy_2d_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yv12_rect_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yv12_2d_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->p8_rect_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->p8_2d_shader));
    checkGLcall("Delete yuv and p8 programs");
    LEAVE_GL();

    HeapFree(GetProcessHeap(), 0, device->blit_priv);
    device->blit_priv = NULL;
}

static BOOL gen_planar_yuv_read(struct wined3d_shader_buffer *buffer, enum complex_fixup fixup,
        GLenum textype, char *luminance)
{
    char chroma;
    const char *tex, *texinstr;

    if (fixup == COMPLEX_FIXUP_UYVY) {
        chroma = 'x';
        *luminance = 'w';
    } else {
        chroma = 'w';
        *luminance = 'x';
    }
    switch(textype) {
        case GL_TEXTURE_2D:             tex = "2D";     texinstr = "TXP"; break;
        case GL_TEXTURE_RECTANGLE_ARB:  tex = "RECT";   texinstr = "TEX"; break;
        default:
            /* This is more tricky than just replacing the texture type - we have to navigate
             * properly in the texture to find the correct chroma values
             */
            FIXME("Implement yuv correction for non-2d, non-rect textures\n");
            return FALSE;
    }

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
    if(textype != GL_TEXTURE_RECTANGLE_ARB) {
        shader_addline(buffer, "MUL texcrd.xy, fragment.texcoord[0], size.x;\n");
        shader_addline(buffer, "MOV texcrd.w, size.x;\n");
    } else {
        shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    }
    /* We must not allow filtering between pixel x and x+1, this would mix U and V
     * Vertical filtering is ok. However, bear in mind that the pixel center is at
     * 0.5, so add 0.5.
     */
    shader_addline(buffer, "FLR texcrd.x, texcrd.x;\n");
    shader_addline(buffer, "ADD texcrd.x, texcrd.x, coef.y;\n");

    /* Divide the x coordinate by 0.5 and get the fraction. This gives 0.25 and 0.75 for the
     * even and odd pixels respectively
     */
    shader_addline(buffer, "MUL texcrd2, texcrd, coef.y;\n");
    shader_addline(buffer, "FRC texcrd2, texcrd2;\n");

    /* Sample Pixel 1 */
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

    return TRUE;
}

static BOOL gen_yv12_read(struct wined3d_shader_buffer *buffer, GLenum textype, char *luminance)
{
    const char *tex;

    switch(textype) {
        case GL_TEXTURE_2D:             tex = "2D";     break;
        case GL_TEXTURE_RECTANGLE_ARB:  tex = "RECT";   break;
        default:
            FIXME("Implement yv12 correction for non-2d, non-rect textures\n");
            return FALSE;
    }

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
     *        |  U even rows   |  U odd rows     |
     *        |                |                 |   1
     *        +----------------+------------------   -
     *        |                |                 |   3
     *        |  V even rows   |  V odd rows     |
     *        |                |                 |
     *        +----------------+-----------------+----
     *        |                |                 |
     *        |     0.5        |       0.5       |
     *
     * So it appears as if there are 4 chroma images, but in fact the odd rows
     * in the chroma images are in the same row as the even ones. So its is
     * kinda tricky to read
     *
     * When reading from rectangle textures, keep in mind that the input y coordinates
     * go from 0 to d3d_height, whereas the opengl texture height is 1.5 * d3d_height
     */
    shader_addline(buffer, "PARAM yv12_coef = {%f, %f, %f, %f};\n",
            2.0f / 3.0f, 1.0f / 6.0f, (2.0f / 3.0f) + (1.0f / 6.0f), 1.0f / 3.0f);

    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    /* the chroma planes have only half the width */
    shader_addline(buffer, "MUL texcrd.x, texcrd.x, coef.y;\n");

    /* The first value is between 2/3 and 5/6th of the texture's height, so scale+bias
     * the coordinate. Also read the right side of the image when reading odd lines
     *
     * Don't forget to clamp the y values in into the range, otherwise we'll get filtering
     * bleeding
     */
    if(textype == GL_TEXTURE_2D) {

        shader_addline(buffer, "RCP chroma.w, size.y;\n");

        shader_addline(buffer, "MUL texcrd2.y, texcrd.y, size.y;\n");

        shader_addline(buffer, "FLR texcrd2.y, texcrd2.y;\n");
        shader_addline(buffer, "MAD texcrd.y, texcrd.y, yv12_coef.y, yv12_coef.x;\n");

        /* Read odd lines from the right side(add size * 0.5 to the x coordinate */
        shader_addline(buffer, "ADD texcrd2.x, texcrd2.y, yv12_coef.y;\n"); /* To avoid 0.5 == 0.5 comparisons */
        shader_addline(buffer, "FRC texcrd2.x, texcrd2.x;\n");
        shader_addline(buffer, "SGE texcrd2.x, texcrd2.x, coef.y;\n");
        shader_addline(buffer, "MAD texcrd.x, texcrd2.x, coef.y, texcrd.x;\n");

        /* clamp, keep the half pixel origin in mind */
        shader_addline(buffer, "MAD temp.y, coef.y, chroma.w, yv12_coef.x;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.w, yv12_coef.z;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
    } else {
        /* Read from [size - size+size/4] */
        shader_addline(buffer, "FLR texcrd.y, texcrd.y;\n");
        shader_addline(buffer, "MAD texcrd.y, texcrd.y, coef.w, size.y;\n");

        /* Read odd lines from the right side(add size * 0.5 to the x coordinate */
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
        shader_addline(buffer, "ADD temp.y, size.y, -coef.y;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
    }
    /* Read the texture, put the result into the output register */
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.x, temp.w;\n");

    /* The other chroma value is 1/6th of the texture lower, from 5/6th to 6/6th
     * No need to clamp because we're just reusing the already clamped value from above
     */
    if(textype == GL_TEXTURE_2D) {
        shader_addline(buffer, "ADD texcrd.y, texcrd.y, yv12_coef.y;\n");
    } else {
        shader_addline(buffer, "MAD texcrd.y, size.y, coef.w, texcrd.y;\n");
    }
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.y, temp.w;\n");

    /* Sample the luminance value. It is in the top 2/3rd of the texture, so scale the y coordinate.
     * Clamp the y coordinate to prevent the chroma values from bleeding into the sampled luminance
     * values due to filtering
     */
    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    if(textype == GL_TEXTURE_2D) {
        /* Multiply the y coordinate by 2/3 and clamp it */
        shader_addline(buffer, "MUL texcrd.y, texcrd.y, yv12_coef.x;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.w, yv12_coef.x;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    } else {
        /* Reading from texture_rectangles is pretty straightforward, just use the unmodified
         * texture coordinate. It is still a good idea to clamp it though, since the opengl texture
         * is bigger
         */
        shader_addline(buffer, "ADD temp.x, size.y, -coef.y;\n");
        shader_addline(buffer, "MIN texcrd.y, texcrd.y, size.x;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    }
    *luminance = 'a';

    return TRUE;
}

static GLuint gen_p8_shader(IWineD3DDeviceImpl *device, GLenum textype)
{
    GLenum shader;
    struct wined3d_shader_buffer buffer;
    struct arbfp_blit_priv *priv = device->blit_priv;
    GLint pos;

    /* Shader header */
    if (!shader_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    ENTER_GL();
    GL_EXTCALL(glGenProgramsARB(1, &shader));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    LEAVE_GL();
    if(!shader) {
        shader_buffer_free(&buffer);
        return 0;
    }

    shader_addline(&buffer, "!!ARBfp1.0\n");
    shader_addline(&buffer, "TEMP index;\n");

    /* { 255/256, 0.5/255*255/256, 0, 0 } */
    shader_addline(&buffer, "PARAM constants = { 0.996, 0.00195, 0, 0 };\n");

    /* The alpha-component contains the palette index */
    if(textype == GL_TEXTURE_RECTANGLE_ARB)
        shader_addline(&buffer, "TXP index, fragment.texcoord[0], texture[0], RECT;\n");
    else
        shader_addline(&buffer, "TEX index, fragment.texcoord[0], texture[0], 2D;\n");

    /* Scale the index by 255/256 and add a bias of '0.5' in order to sample in the middle */
    shader_addline(&buffer, "MAD index.a, index.a, constants.x, constants.y;\n");

    /* Use the alpha-component as an index in the palette to get the final color */
    shader_addline(&buffer, "TEX result.color, index.a, texture[1], 1D;\n");
    shader_addline(&buffer, "END\n");

    ENTER_GL();
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            strlen(buffer.buffer), buffer.buffer));
    checkGLcall("glProgramStringARB()");

    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if (pos != -1)
    {
        FIXME("Fragment program error at position %d: %s\n\n", pos,
              debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        shader_arb_dump_program_source(buffer.buffer);
    }

    if (textype == GL_TEXTURE_RECTANGLE_ARB)
        priv->p8_rect_shader = shader;
    else
        priv->p8_2d_shader = shader;

    shader_buffer_free(&buffer);
    LEAVE_GL();

    return shader;
}

/* Context activation is done by the caller. */
static GLuint gen_yuv_shader(IWineD3DDeviceImpl *device, enum complex_fixup yuv_fixup, GLenum textype)
{
    GLenum shader;
    struct wined3d_shader_buffer buffer;
    char luminance_component;
    struct arbfp_blit_priv *priv = device->blit_priv;
    GLint pos;

    /* Shader header */
    if (!shader_buffer_init(&buffer))
    {
        ERR("Failed to initialize shader buffer.\n");
        return 0;
    }

    ENTER_GL();
    GL_EXTCALL(glGenProgramsARB(1, &shader));
    checkGLcall("GL_EXTCALL(glGenProgramsARB(1, &shader))");
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader)");
    LEAVE_GL();
    if(!shader) {
        shader_buffer_free(&buffer);
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
     * Appart of avoiding filtering issues, the code has to know which value it just
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
    shader_addline(&buffer, "PARAM size = program.local[0];\n");

    switch (yuv_fixup)
    {
        case COMPLEX_FIXUP_UYVY:
        case COMPLEX_FIXUP_YUY2:
            if (!gen_planar_yuv_read(&buffer, yuv_fixup, textype, &luminance_component))
            {
                shader_buffer_free(&buffer);
                return 0;
            }
            break;

        case COMPLEX_FIXUP_YV12:
            if (!gen_yv12_read(&buffer, textype, &luminance_component))
            {
                shader_buffer_free(&buffer);
                return 0;
            }
            break;

        default:
            FIXME("Unsupported YUV fixup %#x\n", yuv_fixup);
            shader_buffer_free(&buffer);
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

    ENTER_GL();
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
            strlen(buffer.buffer), buffer.buffer));
    checkGLcall("glProgramStringARB()");

    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if (pos != -1)
    {
        FIXME("Fragment program error at position %d: %s\n\n", pos,
              debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        shader_arb_dump_program_source(buffer.buffer);
    }
    else
    {
        GLint native;

        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native));
        checkGLcall("glGetProgramivARB()");
        if (!native) WARN("Program exceeds native resource limits.\n");
    }

    shader_buffer_free(&buffer);
    LEAVE_GL();

    switch (yuv_fixup)
    {
        case COMPLEX_FIXUP_YUY2:
            if (textype == GL_TEXTURE_RECTANGLE_ARB) priv->yuy2_rect_shader = shader;
            else priv->yuy2_2d_shader = shader;
            break;

        case COMPLEX_FIXUP_UYVY:
            if (textype == GL_TEXTURE_RECTANGLE_ARB) priv->uyvy_rect_shader = shader;
            else priv->uyvy_2d_shader = shader;
            break;

        case COMPLEX_FIXUP_YV12:
            if (textype == GL_TEXTURE_RECTANGLE_ARB) priv->yv12_rect_shader = shader;
            else priv->yv12_2d_shader = shader;
            break;
        default:
            ERR("Unsupported complex fixup: %d\n", yuv_fixup);
    }

    return shader;
}

/* Context activation is done by the caller. */
static HRESULT arbfp_blit_set(IWineD3DDevice *iface, const struct GlPixelFormatDesc *format_desc,
        GLenum textype, UINT width, UINT height)
{
    GLenum shader;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    float size[4] = {width, height, 1, 1};
    struct arbfp_blit_priv *priv = device->blit_priv;
    enum complex_fixup fixup;

    if (!is_complex_fixup(format_desc->color_fixup))
    {
        TRACE("Fixup:\n");
        dump_color_fixup_desc(format_desc->color_fixup);
        /* Don't bother setting up a shader for unconverted formats */
        ENTER_GL();
        glEnable(textype);
        checkGLcall("glEnable(textype)");
        LEAVE_GL();
        return WINED3D_OK;
    }

    fixup = get_complex_fixup(format_desc->color_fixup);

    switch(fixup)
    {
        case COMPLEX_FIXUP_YUY2:
            shader = textype == GL_TEXTURE_RECTANGLE_ARB ? priv->yuy2_rect_shader : priv->yuy2_2d_shader;
            break;

        case COMPLEX_FIXUP_UYVY:
            shader = textype == GL_TEXTURE_RECTANGLE_ARB ? priv->uyvy_rect_shader : priv->uyvy_2d_shader;
            break;

        case COMPLEX_FIXUP_YV12:
            shader = textype == GL_TEXTURE_RECTANGLE_ARB ? priv->yv12_rect_shader : priv->yv12_2d_shader;
            break;

        case COMPLEX_FIXUP_P8:
            shader = textype == GL_TEXTURE_RECTANGLE_ARB ? priv->p8_rect_shader : priv->p8_2d_shader;
            if (!shader) shader = gen_p8_shader(device, textype);
            break;

        default:
            FIXME("Unsupported complex fixup %#x, not setting a shader\n", fixup);
            ENTER_GL();
            glEnable(textype);
            checkGLcall("glEnable(textype)");
            LEAVE_GL();
            return E_NOTIMPL;
    }

    if (!shader) shader = gen_yuv_shader(device, fixup, textype);

    ENTER_GL();
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB)");
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader)");
    GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, size));
    checkGLcall("glProgramLocalParameter4fvARB");
    LEAVE_GL();

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void arbfp_blit_unset(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    ENTER_GL();
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable(GL_TEXTURE_2D)");
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
    }
    LEAVE_GL();
}

static BOOL arbfp_blit_color_fixup_supported(struct color_fixup_desc fixup)
{
    enum complex_fixup complex_fixup;

    if (TRACE_ON(d3d_shader) && TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    if (is_identity_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    /* We only support YUV conversions. */
    if (!is_complex_fixup(fixup))
    {
        TRACE("[FAILED]\n");
        return FALSE;
    }

    complex_fixup = get_complex_fixup(fixup);
    switch(complex_fixup)
    {
        case COMPLEX_FIXUP_YUY2:
        case COMPLEX_FIXUP_UYVY:
        case COMPLEX_FIXUP_YV12:
        case COMPLEX_FIXUP_P8:
            TRACE("[OK]\n");
            return TRUE;

        default:
            FIXME("Unsupported YUV fixup %#x\n", complex_fixup);
            TRACE("[FAILED]\n");
            return FALSE;
    }
}

const struct blit_shader arbfp_blit = {
    arbfp_blit_alloc,
    arbfp_blit_free,
    arbfp_blit_set,
    arbfp_blit_unset,
    arbfp_blit_color_fixup_supported,
};

#undef GLINFO_LOCATION
